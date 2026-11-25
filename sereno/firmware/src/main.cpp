// Sereno · firmware de la placa (LilyGO T-SIM7080G-S3)
//
// El aparato duerme casi siempre. Cada diez minutos despierta un par de
// segundos, lee las sondas, se lo cuenta a la Guardia (lib/sereno_core, la
// lógica probada en PC) y vuelve a dormirse. Sólo cuando la Guardia lo pide
// enciende la cámara o el módem.
//
//   despertar → sensores → libreta (CSV) → Guardia → [foto] → [módem] → dormir
//
// ESTADO: las llamadas a librerías están contrastadas con sus fuentes en las
// versiones fijadas en platformio.ini, y el patillaje con el repo del
// fabricante. Pero NO se ha compilado para ESP32 ni probado en la placa. Antes
// de fiarse de él: compilar, y verificar sensores, cámara y módem uno a uno
// (ver LEEME).
#include <Arduino.h>
#include <LittleFS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Wire.h>

#include <Adafruit_MLX90614.h>
#include <Adafruit_SHT4x.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <TinyGsmClient.h>
#include <XPowersLib.h>

#include <driver/rtc_gpio.h>
#include <esp_camera.h>
#include <esp_sleep.h>
#include <img_converters.h>
#include <sys/time.h>

#include <cmath>
#include <cstring>
#include <string>
#include <vector>

#include "cante.h"
#include "flor.h"
#include "guardia.h"
#include "helada.h"
#include "placa.h"
#include "sol.h"

#if __has_include("config.h")
#include "config.h"
#else
#include "config.example.h"
#endif

using namespace sereno;

namespace {

const int kPeriodoS = 600;               // una muestra cada diez minutos
const time_t kHoraValida = 1767225600;   // 1-1-2026: antes de esto, reloj sin fijar
const char* kRutaEstado = "/estado.bin";
const char* kRutaCola = "/cola.txt";
const char* kRutaFoto = "/foto.jpg";
const char* kRutaFondo = "/fondo.bin";
const char kSeparador = '\x1e';          // entre partes, en la cola
const int kFotoAncho = 640, kFotoAlto = 480;  // VGA; se cuenta a la mitad

// Lo que sobrevive al sueño profundo. Son bytes crudos a propósito: un objeto
// con constructor se reiniciaría en cada despertar.
RTC_DATA_ATTR uint8_t g_rtcEstado[sizeof(Persistente)];
RTC_DATA_ATTR uint32_t g_rtcMagia;
RTC_DATA_ATTR uint32_t g_rtcDespertares;
RTC_DATA_ATTR uint8_t g_rtcFotoPendiente;

XPowersPMU g_pmu;
OneWire g_bus(PIN_SONDAS);
DallasTemperature g_sondas(&g_bus);
Adafruit_SHT4x g_sht;
Adafruit_MLX90614 g_mlx;
TinyGsm g_modem(Serial1);

const uint8_t kSondas[kNiveles][8] = SERENO_SONDAS;
const float kAjuste[kNiveles] = SERENO_AJUSTE_SONDAS;

Config configuracion() {
  Config c;
  c.latitud = SERENO_LATITUD;
  c.longitud = SERENO_LONGITUD;
  c.minutosMuestra = kPeriodoS / 60.0f;
  c.parcela.plantasCentinela = SERENO_PLANTAS_CENTINELA;
  c.parcela.plantasTotales = SERENO_PLANTAS_TOTALES;
  return c;
}

Guardia g_guardia(configuracion());

bool horaFijada() { return time(nullptr) > kHoraValida; }
bool hayDestino() { return strlen(SERENO_TELEGRAM_TOKEN) > 0; }

// ------------------------------------------------------------ alimentación

bool iniciarPMU() {
  if (!g_pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, PIN_I2C_SDA, PIN_I2C_SCL)) {
    return false;
  }
  Wire.setClock(100000);  // el MLX90614 habla SMBus: no más de 100 kHz
  // Sin esto la PMU no carga la batería (nota del fabricante).
  g_pmu.disableTSPinMeasure();
  // BLDO1 alimenta el conversor de niveles entre el ESP32 y el módem: siempre.
  g_pmu.enableBLDO1();
  g_pmu.enableBattVoltageMeasure();
  // Panel de 6 V y 1–2 W: 500 mA de carga es el tope que recomienda el fabricante.
  g_pmu.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_500MA);
  g_pmu.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);
  return true;
}

void apagarPerifericos() {
  g_pmu.disableDC3();    // módem
  g_pmu.disableBLDO2();  // antena GPS
  g_pmu.disableALDO1();  // cámara
  g_pmu.disableALDO2();
  g_pmu.disableALDO4();
  g_pmu.disableALDO3();  // tarjeta SD (no se usa)
  g_pmu.disableDC2();
  g_pmu.disableDC4();
  g_pmu.disableDC5();
  g_pmu.disableCPUSLDO();
  g_pmu.disableDLDO1();
  g_pmu.disableDLDO2();
}

// --------------------------------------------------------------- sensores

bool sondaConfigurada(int i) {
  for (int k = 0; k < 8; ++k) {
    if (kSondas[i][k] != 0) return true;
  }
  return false;
}

Muestra leerSensores() {
  Muestra m;
  m.t = time(nullptr);

  g_sondas.begin();
  g_sondas.setResolution(12);
  g_sondas.requestTemperatures();  // bloquea ~750 ms a 12 bits
  for (int i = 0; i < kNiveles; ++i) {
    const float t = sondaConfigurada(i) ? g_sondas.getTempC(kSondas[i])
                                        : g_sondas.getTempCByIndex(i);
    // −127 es «no contesta»; 85,0 exacto es el valor de encendido sin convertir.
    m.T[i] = (t == DEVICE_DISCONNECTED_C || t == 85.0f) ? NAN : t + kAjuste[i];
  }

  m.Taire = NAN;
  m.HR = NAN;
  if (g_sht.begin(&Wire)) {
    g_sht.setPrecision(SHT4X_HIGH_PRECISION);
    g_sht.setHeater(SHT4X_NO_HEATER);
    sensors_event_t humedad, temperatura;
    if (g_sht.getEvent(&humedad, &temperatura)) {
      m.Taire = temperatura.temperature;
      m.HR = humedad.relative_humidity;
    }
  }

  m.Tcielo = NAN;
  if (g_mlx.begin(MLX90614_I2CADDR, &Wire)) {
    const double cielo = g_mlx.readObjectTempC();
    // Fuera de rango físico = lectura corrupta.
    if (!std::isnan(cielo) && cielo > -90.0 && cielo < 60.0) {
      m.Tcielo = static_cast<float>(cielo);
    }
  }
  return m;
}

// ---------------------------------------------------------------- libreta

// Según la versión del núcleo, File::name() trae o no la ruta: nos quedamos
// con lo que hay tras la última barra.
String nombreBase(const char* nombre) {
  const String n = nombre;
  return n.substring(n.lastIndexOf('/') + 1);
}

void apuntar(const Muestra& m) {
  if (!LittleFS.exists("/datos")) LittleFS.mkdir("/datos");

  // Si queda poco sitio se borra el mes más antiguo.
  if (LittleFS.totalBytes() - LittleFS.usedBytes() < 200 * 1024) {
    String masViejo;
    File dir = LittleFS.open("/datos");
    for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
      const String nombre = nombreBase(f.name());
      if (masViejo.isEmpty() || nombre < masViejo) masViejo = nombre;
    }
    if (!masViejo.isEmpty()) LittleFS.remove("/datos/" + masViejo);
  }

  struct tm l;
  localtime_r(&m.t, &l);
  char ruta[32];
  snprintf(ruta, sizeof(ruta), "/datos/%04d-%02d.csv", l.tm_year + 1900,
           l.tm_mon + 1);
  const bool nuevo = !LittleFS.exists(ruta);
  File f = LittleFS.open(ruta, FILE_APPEND);
  if (!f) return;
  if (nuevo) f.println("epoch,T010,T050,T100,T200,Taire,HR,Tcielo,mV");
  f.printf("%ld,%.2f,%.2f,%.2f,%.2f,%.2f,%.1f,%.1f,%u\n",
           static_cast<long>(m.t), m.T[0], m.T[1], m.T[2], m.T[3], m.Taire, m.HR,
           m.Tcielo, static_cast<unsigned>(g_pmu.getBattVoltage()));
  f.close();
}

void guardarEstado() {
  File f = LittleFS.open(kRutaEstado, FILE_WRITE);
  if (!f) return;
  f.write(reinterpret_cast<const uint8_t*>(&g_guardia.estado()),
          sizeof(Persistente));
  f.close();
}

void recuperarEstado() {
  Persistente p;
  if (g_rtcMagia == kMagia) {  // venimos de dormir: la memoria del reloj manda
    memcpy(&p, g_rtcEstado, sizeof(p));
    if (g_guardia.restaurar(p)) return;
  }
  File f = LittleFS.open(kRutaEstado, FILE_READ);  // arranque en frío
  if (f && f.size() == sizeof(p)) {
    f.read(reinterpret_cast<uint8_t*>(&p), sizeof(p));
    g_guardia.restaurar(p);
  }
  if (f) f.close();
}

void encolar(const std::string& parte) {
  File f = LittleFS.open(kRutaCola, FILE_APPEND);
  if (!f) return;
  f.write(reinterpret_cast<const uint8_t*>(parte.data()), parte.size());
  f.write(static_cast<uint8_t>(kSeparador));
  f.close();
}

std::vector<std::string> leerCola() {
  std::vector<std::string> partes;
  File f = LittleFS.open(kRutaCola, FILE_READ);
  if (!f) return partes;
  std::string actual;
  while (f.available()) {
    const char c = static_cast<char>(f.read());
    if (c == kSeparador) {
      if (!actual.empty()) partes.push_back(actual);
      actual.clear();
    } else {
      actual += c;
    }
  }
  f.close();
  return partes;
}

void reescribirCola(const std::vector<std::string>& partes, size_t desde) {
  LittleFS.remove(kRutaCola);
  for (size_t i = desde; i < partes.size(); ++i) encolar(partes[i]);
}

// ----------------------------------------------------------------- cámara

bool encenderCamara() {
  // Tensiones de la cámara, tal cual las fija el ejemplo del fabricante.
  g_pmu.setALDO1Voltage(1800);
  g_pmu.enableALDO1();
  g_pmu.setALDO2Voltage(2800);
  g_pmu.enableALDO2();
  g_pmu.setALDO4Voltage(3000);
  g_pmu.enableALDO4();
  delay(100);

  camera_config_t c = {};
  c.ledc_channel = LEDC_CHANNEL_0;
  c.ledc_timer = LEDC_TIMER_0;
  c.pin_d0 = PIN_CAM_Y2;
  c.pin_d1 = PIN_CAM_Y3;
  c.pin_d2 = PIN_CAM_Y4;
  c.pin_d3 = PIN_CAM_Y5;
  c.pin_d4 = PIN_CAM_Y6;
  c.pin_d5 = PIN_CAM_Y7;
  c.pin_d6 = PIN_CAM_Y8;
  c.pin_d7 = PIN_CAM_Y9;
  c.pin_xclk = PIN_CAM_XCLK;
  c.pin_pclk = PIN_CAM_PCLK;
  c.pin_vsync = PIN_CAM_VSYNC;
  c.pin_href = PIN_CAM_HREF;
  c.pin_sccb_sda = PIN_CAM_SIOD;
  c.pin_sccb_scl = PIN_CAM_SIOC;
  c.pin_pwdn = PIN_CAM_PWDN;
  c.pin_reset = PIN_CAM_RESET;
  c.xclk_freq_hz = 20000000;
  c.pixel_format = PIXFORMAT_JPEG;
  c.frame_size = FRAMESIZE_VGA;
  c.jpeg_quality = 12;
  c.fb_count = 1;
  c.fb_location = CAMERA_FB_IN_PSRAM;
  c.grab_mode = CAMERA_GRAB_LATEST;
  return esp_camera_init(&c) == ESP_OK;
}

void apagarCamara() {
  esp_camera_deinit();
  g_pmu.disableALDO1();
  g_pmu.disableALDO2();
  g_pmu.disableALDO4();
}

// Foto con el farol encendido. Devuelve el fotograma (hay que devolverlo con
// esp_camera_fb_return) o nullptr.
camera_fb_t* fotografiar() {
  digitalWrite(PIN_DESTELLO, HIGH);
  // La exposición automática necesita unos fotogramas para asentarse con luz.
  for (int i = 0; i < 4; ++i) {
    camera_fb_t* descarte = esp_camera_fb_get();
    if (descarte) esp_camera_fb_return(descarte);
  }
  camera_fb_t* fb = esp_camera_fb_get();
  digitalWrite(PIN_DESTELLO, LOW);
  return fb;
}

// Cuenta las flores del fotograma. −1 si no se pudo.
int contarEnFoto(camera_fb_t* fb, bool aprenderFondo) {
  const int n = kFotoAncho * kFotoAlto, n4 = n / 4;
  if (fb->width != static_cast<size_t>(kFotoAncho) ||
      fb->height != static_cast<size_t>(kFotoAlto)) {
    return -1;
  }
  uint8_t* rgb = static_cast<uint8_t*>(ps_malloc(n * 3));
  uint8_t* gris = static_cast<uint8_t*>(ps_malloc(n));
  uint8_t* media = static_cast<uint8_t*>(ps_malloc(n4));
  uint8_t* fondo = static_cast<uint8_t*>(ps_malloc(n4));
  uint8_t* resta = static_cast<uint8_t*>(ps_malloc(n4));
  int flores = -1;
  if (rgb && gris && media && fondo && resta &&
      fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, rgb)) {
    blancuraRGB888(rgb, kFotoAncho, kFotoAlto, gris);
    reducirMitad(gris, kFotoAncho, kFotoAlto, media);

    memset(fondo, 0, n4);
    File f = LittleFS.open(kRutaFondo, FILE_READ);
    if (f && f.size() == static_cast<size_t>(n4)) f.read(fondo, n4);
    if (f) f.close();

    restarFondo(media, fondo, n4, resta);
    flores = contarFlores(resta, kFotoAncho / 2, kFotoAlto / 2, ParamsFlor());

    if (aprenderFondo) {
      actualizarFondo(fondo, media, n4);
      File g = LittleFS.open(kRutaFondo, FILE_WRITE);
      if (g) {
        g.write(fondo, n4);
        g.close();
      }
    }
  }
  free(rgb);
  free(gris);
  free(media);
  free(fondo);
  free(resta);
  return flores;
}

// La foto de la noche: cuenta, guarda el JPEG para enviarlo y devuelve el conteo.
int fotoDeFlores() {
  int flores = -1;
  if (encenderCamara()) {
    camera_fb_t* fb = fotografiar();
    if (fb) {
      flores = contarEnFoto(fb, true);
      File f = LittleFS.open(kRutaFoto, FILE_WRITE);
      if (f) {
        f.write(fb->buf, fb->len);
        f.close();
      }
      esp_camera_fb_return(fb);
    }
  }
  apagarCamara();
  return flores;
}

// ------------------------------------------------------------------ módem

bool encenderModem() {
  // Tensión del módem, como en los ejemplos del fabricante.
  g_pmu.setDC3Voltage(3000);
  g_pmu.enableDC3();
  Serial1.begin(115200, SERIAL_8N1, PIN_MODEM_RXD, PIN_MODEM_TXD);
  pinMode(PIN_MODEM_PWR, OUTPUT);
  pinMode(PIN_MODEM_DTR, OUTPUT);
  pinMode(PIN_MODEM_RI, INPUT);
  digitalWrite(PIN_MODEM_DTR, LOW);

  for (int intento = 0; intento < 28; ++intento) {
    if (g_modem.testAT(1000)) return true;
    if (intento % 7 == 6) {  // pulso de encendido, secuencia del fabricante
      digitalWrite(PIN_MODEM_PWR, LOW);
      delay(100);
      digitalWrite(PIN_MODEM_PWR, HIGH);
      delay(1000);
      digitalWrite(PIN_MODEM_PWR, LOW);
    }
  }
  return false;
}

void apagarModem() {
  g_modem.gprsDisconnect();
  g_modem.poweroff();
  delay(1500);
  Serial1.end();
  g_pmu.disableDC3();
}

bool conectarRed() {
  if (g_modem.getSimStatus() != SIM_READY) return false;
  g_modem.sendAT("+CFUN=0");
  g_modem.waitResponse(20000L);
  g_modem.setNetworkMode(2);    // automático
  g_modem.setPreferredMode(3);  // LTE-M y NB-IoT, lo que haya
  g_modem.sendAT("+CFUN=1");
  g_modem.waitResponse(20000L);
  if (!g_modem.waitForNetwork(120000L)) return false;
  return g_modem.gprsConnect(SERENO_APN);
}

// El reloj del ESP32 deriva un par de minutos al día: se pone en hora en cada
// conexión.
void ponerEnHora() {
  g_modem.NTPServerSync("pool.ntp.org", 0);
  int anio = 0, mes = 0, dia = 0, hora = 0, minuto = 0, segundo = 0;
  float zona = 0;
  if (!g_modem.getNetworkTime(&anio, &mes, &dia, &hora, &minuto, &segundo,
                              &zona)) {
    return;
  }
  if (anio < 2026 || anio > 2099) return;
  struct timeval tv = {};
  tv.tv_sec = epochUTC(anio, mes, dia, hora, minuto, segundo) -
              static_cast<time_t>(zona * 3600.0f);
  settimeofday(&tv, nullptr);
}

String codificarURL(const std::string& s) {
  static const char* hex = "0123456789ABCDEF";
  String o;
  o.reserve(s.size() * 3);
  for (unsigned char c : s) {
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      o += static_cast<char>(c);
    } else {
      o += '%';
      o += hex[c >> 4];
      o += hex[c & 15];
    }
  }
  return o;
}

// Lee la primera línea de la respuesta y devuelve el código HTTP (0 si no llega).
int codigoHTTP(Client& c) {
  const uint32_t limite = millis() + 30000;
  String linea;
  while (millis() < limite) {
    while (c.available()) {
      const char ch = static_cast<char>(c.read());
      if (ch == '\n') {
        const int espacio = linea.indexOf(' ');
        return espacio > 0 ? linea.substring(espacio + 1).toInt() : 0;
      }
      linea += ch;
    }
    if (!c.connected() && !c.available()) break;
    delay(20);
  }
  return 0;
}

bool telegramTexto(Client& c, const std::string& texto) {
  const String cuerpo = String("chat_id=") + SERENO_TELEGRAM_CHAT + "&text=" +
                        codificarURL(texto);
  if (!c.connect("api.telegram.org", 443)) return false;
  c.print(String("POST /bot") + SERENO_TELEGRAM_TOKEN +
          "/sendMessage HTTP/1.1\r\n"
          "Host: api.telegram.org\r\n"
          "Content-Type: application/x-www-form-urlencoded\r\n"
          "Connection: close\r\n"
          "Content-Length: " +
          String(cuerpo.length()) + "\r\n\r\n");
  c.print(cuerpo);
  const bool ok = codigoHTTP(c) == 200;
  c.stop();
  return ok;
}

bool telegramFoto(Client& c, const char* ruta, const std::string& pie) {
  File f = LittleFS.open(ruta, FILE_READ);
  if (!f) return false;
  const char* limite = "----sereno7d1c2a";
  String antes;
  antes += String("--") + limite +
           "\r\nContent-Disposition: form-data; name=\"chat_id\"\r\n\r\n" +
           SERENO_TELEGRAM_CHAT + "\r\n";
  antes += String("--") + limite +
           "\r\nContent-Disposition: form-data; name=\"caption\"\r\n\r\n" +
           pie.substr(0, 900).c_str() + "\r\n";
  antes += String("--") + limite +
           "\r\nContent-Disposition: form-data; name=\"photo\"; "
           "filename=\"flor.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
  const String despues = String("\r\n--") + limite + "--\r\n";

  bool ok = false;
  if (c.connect("api.telegram.org", 443)) {
    c.print(String("POST /bot") + SERENO_TELEGRAM_TOKEN +
            "/sendPhoto HTTP/1.1\r\n"
            "Host: api.telegram.org\r\n"
            "Content-Type: multipart/form-data; boundary=" +
            limite +
            "\r\n"
            "Connection: close\r\n"
            "Content-Length: " +
            String(antes.length() + f.size() + despues.length()) + "\r\n\r\n");
    c.print(antes);
    uint8_t trozo[512];
    while (f.available()) {
      const size_t leidos = f.read(trozo, sizeof(trozo));
      if (leidos == 0 || c.write(trozo, leidos) != leidos) break;
    }
    c.print(despues);
    ok = codigoHTTP(c) == 200;
    c.stop();
  }
  f.close();
  return ok;
}

// Enciende el módem, pone el reloj en hora y vacía la cola de partes.
// Devuelve true si no queda nada por enviar.
bool subir() {
  bool vacia = false;
  if (encenderModem() && conectarRed()) {
    ponerEnHora();
    if (hayDestino()) {
      TinyGsmClientSecure cliente(g_modem);
      std::vector<std::string> partes = leerCola();
      size_t enviados = 0;
      for (; enviados < partes.size(); ++enviados) {
        // El parte de flores va con su foto; si la foto no sale, va el texto.
        const bool esFlor =
            partes[enviados].find("floreciendo") != std::string::npos;
        bool ok = false;
        if (esFlor && g_rtcFotoPendiente && LittleFS.exists(kRutaFoto)) {
          ok = telegramFoto(cliente, kRutaFoto, partes[enviados]);
          if (ok) g_rtcFotoPendiente = 0;
        }
        if (!ok) ok = telegramTexto(cliente, partes[enviados]);
        if (!ok) break;
      }
      reescribirCola(partes, enviados);
      vacia = enviados == partes.size();
    }
  }
  apagarModem();
  return vacia;
}

// ------------------------------------------------------------ modo visita
// Pulsando BOOT el aparato levanta su propia wifi durante diez minutos:
// lecturas en vivo, códigos de las sondas, foto de prueba para apuntar la
// cámara, descarga de la libreta y puesta en hora desde el móvil. Es la forma
// de trabajar sin cobertura.

void modoVisita() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(SERENO_WIFI_NOMBRE, SERENO_WIFI_CLAVE);
  WebServer web(80);
  uint32_t ultimaActividad = millis();

  web.on("/", [&]() {
    ultimaActividad = millis();
    const Muestra m = leerSensores();
    String h = "<!doctype html><meta charset=utf-8><meta name=viewport "
               "content='width=device-width'><title>Sereno</title>"
               "<body style='font:16px system-ui;max-width:34em;margin:1em auto;"
               "padding:0 1em'><h1>Sereno</h1>";
    // Sin cobertura no hay quien le dé la hora: se la da el móvil al entrar.
    h += "<script>fetch('/hora?t='+Math.floor(Date.now()/1000))</script>";
    h += horaFijada() ? "<p>Reloj en hora.</p>"
                      : "<p><b>Reloj sin fijar</b>: recarga la página.</p>";
    h += "<h2>Ahora</h2><table>";
    for (int i = 0; i < kNiveles; ++i) {
      h += "<tr><td>" + String(kAlturasM[i], 1) + " m<td>" + String(m.T[i], 2) +
           " °C";
    }
    h += "<tr><td>Aire 1 m<td>" + String(m.Taire, 2) + " °C · " +
         String(m.HR, 0) + " % HR";
    h += "<tr><td>Cielo<td>" + String(m.Tcielo, 1) + " °C";
    h += "<tr><td>Batería<td>" + String(g_pmu.getBattVoltage()) + " mV</table>";

    h += "<h2>Sondas en el bus</h2><p>Calienta una con la mano y recarga: la "
         "que suba es esa.</p><pre>";
    DeviceAddress dir;
    for (int i = 0; i < g_sondas.getDeviceCount(); ++i) {
      if (!g_sondas.getAddress(dir, i)) continue;
      h += "{";
      for (int k = 0; k < 8; ++k) {
        h += "0x" + String(dir[k], HEX) + (k < 7 ? ", " : "");
      }
      h += "}  " + String(g_sondas.getTempC(dir), 2) + " °C\n";
    }
    h += "</pre><h2>Cámara</h2><p><a href=/foto>Foto de prueba con destello</a> "
         "(tarda unos segundos; no cambia el fondo aprendido)</p>"
         "<h2>Libreta</h2><ul>";
    File d = LittleFS.open("/datos");
    for (File f = d.openNextFile(); f; f = d.openNextFile()) {
      const String nombre = nombreBase(f.name());
      h += "<li><a href='/datos?f=" + nombre + "'>" + nombre + "</a> (" +
           String(static_cast<unsigned long>(f.size() / 1024)) + " kB)";
    }
    const Persistente& p = g_guardia.estado();
    h += "</ul><h2>Temporada</h2><p>" + String(p.temporada.noches) +
         " noches · " + String(p.temporada.nochesHelada) + " de helada · " +
         String(p.temporada.horasFrio, 0) + " horas frío · pronóstico con " +
         String(p.pronostico.n) + " noches aprendidas</p>";
    web.send(200, "text/html; charset=utf-8", h);
  });

  web.on("/hora", [&]() {
    ultimaActividad = millis();
    const long t = web.arg("t").toInt();
    if (t > static_cast<long>(kHoraValida)) {
      struct timeval tv = {};
      tv.tv_sec = static_cast<time_t>(t);
      settimeofday(&tv, nullptr);
      web.send(200, "text/plain", "ok");
    } else {
      web.send(400, "text/plain", "hora no valida");
    }
  });

  web.on("/foto", [&]() {
    ultimaActividad = millis();
    if (!encenderCamara()) {
      apagarCamara();
      web.send(500, "text/plain", "La camara no arranca");
      return;
    }
    camera_fb_t* fb = fotografiar();
    if (!fb) {
      apagarCamara();
      web.send(500, "text/plain", "Sin fotograma");
      return;
    }
    web.sendHeader("X-Flores", String(contarEnFoto(fb, false)));
    web.send_P(200, "image/jpeg", reinterpret_cast<const char*>(fb->buf),
               fb->len);
    esp_camera_fb_return(fb);
    apagarCamara();
  });

  web.on("/datos", [&]() {
    ultimaActividad = millis();
    const String nombre = web.arg("f");
    if (nombre.indexOf('/') >= 0 || nombre.indexOf("..") >= 0) {
      web.send(400, "text/plain", "Nombre no valido");
      return;
    }
    File f = LittleFS.open("/datos/" + nombre, FILE_READ);
    if (!f) {
      web.send(404, "text/plain", "No existe");
      return;
    }
    web.streamFile(f, "text/csv");
    f.close();
  });

  web.begin();
  while (millis() - ultimaActividad < 10UL * 60UL * 1000UL) {
    web.handleClient();
    delay(5);
  }
  web.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
}

// ------------------------------------------------------------------ dormir

[[noreturn]] void dormir() {
  memcpy(g_rtcEstado, &g_guardia.estado(), sizeof(Persistente));
  g_rtcMagia = kMagia;
  apagarPerifericos();
  digitalWrite(PIN_DESTELLO, LOW);

  // Hasta el siguiente múltiplo de diez minutos de reloj.
  uint64_t segundos = kPeriodoS;
  if (horaFijada()) {
    segundos = kPeriodoS - static_cast<uint64_t>(time(nullptr) % kPeriodoS);
    if (segundos < 20) segundos += kPeriodoS;
  }
  esp_sleep_enable_timer_wakeup(segundos * 1000000ULL);
  rtc_gpio_pullup_en(static_cast<gpio_num_t>(PIN_BOTON));
  rtc_gpio_pulldown_dis(static_cast<gpio_num_t>(PIN_BOTON));
  esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(PIN_BOTON), 0);
  Serial.flush();
  esp_deep_sleep_start();
  while (true) {
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  setenv("TZ", kZonaPeninsular, 1);
  tzset();
  pinMode(PIN_DESTELLO, OUTPUT);
  digitalWrite(PIN_DESTELLO, LOW);
  ++g_rtcDespertares;

  if (!iniciarPMU()) {
    Serial.println("PMU no responde");
    esp_sleep_enable_timer_wakeup(60ULL * 1000000ULL);
    esp_deep_sleep_start();
  }
  if (!LittleFS.begin(true)) Serial.println("LittleFS no monta");
  recuperarEstado();

  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) modoVisita();

  // Sin hora no hay noche ni día: se pide a la red (al arrancar y, si falla,
  // una vez por hora).
  if (!horaFijada()) {
    if (g_rtcDespertares % 6 == 1) subir();
    if (!horaFijada()) dormir();
  }

  const Muestra m = leerSensores();
  apuntar(m);
  Salida s = g_guardia.muestra(m);

  bool urgente = s.urgente;
  if (s.pedirFoto) {
    const int vistas = fotoDeFlores();
    const std::string parte = g_guardia.flores(m.t, vistas);
    if (!parte.empty()) {
      s.mensajes.push_back(parte);
      g_rtcFotoPendiente = 1;
      urgente = true;
    }
  }
  for (const std::string& parte : s.mensajes) {
    Serial.println(parte.c_str());
    if (hayDestino()) encolar(parte);  // sin destino, la cola no haría más que crecer
  }
  if (s.cierreNoche) guardarEstado();

  // Con destino configurado se conecta cuando hay algo urgente, al cerrar la
  // noche (aunque no haya parte: así se pone en hora) y, si quedó algo sin
  // enviar, una vez por hora. Sin destino no enciende el módem: trabaja de
  // libreta y la hora se la da el móvil en modo visita.
  const bool pendiente = LittleFS.exists(kRutaCola);
  if (hayDestino() &&
      (urgente || s.cierreNoche || (pendiente && g_rtcDespertares % 6 == 0))) {
    subir();
  }
  dormir();
}

void loop() {}
