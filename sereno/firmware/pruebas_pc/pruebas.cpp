// Pruebas del núcleo de Sereno en un PC:  make prueba
// Sin dependencias: un par de macros y a correr.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

#include "cante.h"
#include "flor.h"
#include "guardia.h"
#include "helada.h"
#include "sol.h"

using namespace sereno;

static int g_pruebas = 0, g_fallos = 0;

#define COMPROBAR(cond)                                              \
  do {                                                               \
    ++g_pruebas;                                                     \
    if (!(cond)) {                                                   \
      ++g_fallos;                                                    \
      printf("  FALLO %s:%d  %s\n", __FILE__, __LINE__, #cond);      \
    }                                                                \
  } while (0)

#define CERCA(a, b, tol)                                             \
  do {                                                               \
    ++g_pruebas;                                                     \
    const double _a = (a), _b = (b);                                 \
    if (!(std::fabs(_a - _b) <= (tol))) {                            \
      ++g_fallos;                                                    \
      printf("  FALLO %s:%d  %s = %.4f, esperado %.4f ± %.4f\n",     \
             __FILE__, __LINE__, #a, _a, _b, (double)(tol));         \
    }                                                                \
  } while (0)

#define IGUAL_TEXTO(a, b)                                            \
  do {                                                               \
    ++g_pruebas;                                                     \
    const std::string _a = (a), _b = (b);                            \
    if (_a != _b) {                                                  \
      ++g_fallos;                                                    \
      printf("  FALLO %s:%d\n    obtenido: [%s]\n    esperado: [%s]\n", \
             __FILE__, __LINE__, _a.c_str(), _b.c_str());            \
    }                                                                \
  } while (0)

static time_t utc(int a, int m, int d, int h, int min) {
  struct tm t = {};
  t.tm_year = a - 1900;
  t.tm_mon = m - 1;
  t.tm_mday = d;
  t.tm_hour = h;
  t.tm_min = min;
  return timegm(&t);
}

// Ruido determinista en [-1, 1): las pruebas dan siempre lo mismo.
static uint32_t g_semilla = 12345;
static float ruido() {
  g_semilla = g_semilla * 1664525u + 1013904223u;
  return static_cast<float>(g_semilla >> 8) / 8388608.0f - 1.0f;
}

// ----------------------------------------------------------------- el sol

static void pruebaSol() {
  printf("sol\n");
  // Referencia: librería astral (Python) para 37,3316 N · 1,7271 O.
  const double lat = 37.3316, lon = -1.7271;
  struct { int a, m, d; long orto, ocaso; } ref[] = {
      {2026, 12, 21, 1797837452L, 1797871945L},
      {2027, 1, 15, 1799997621L, 1800033143L},
      {2027, 3, 20, 1805523073L, 1805566691L},
      {2027, 6, 21, 1813553207L, 1813606234L},
      {2027, 8, 15, 1818307387L, 1818356352L},
  };
  for (const auto& r : ref) {
    const SolDia s = solDelDia(utc(r.a, r.m, r.d, 12, 0), lat, lon);
    COMPROBAR(s.valido);
    CERCA(static_cast<double>(s.orto), static_cast<double>(r.orto), 180);
    CERCA(static_cast<double>(s.ocaso), static_cast<double>(r.ocaso), 180);
  }
  // Da igual a qué hora del día UTC se pregunte.
  const SolDia a = solDelDia(utc(2027, 1, 15, 0, 5), lat, lon);
  const SolDia b = solDelDia(utc(2027, 1, 15, 23, 50), lat, lon);
  COMPROBAR(a.orto == b.orto && a.ocaso == b.ocaso);
  // Noche polar: Tromsø en diciembre.
  COMPROBAR(!solDelDia(utc(2026, 12, 21, 12, 0), 69.65, 18.96).valido);
  COMPROBAR(diaJuliano(2000, 1, 1) == 2451545L);
  COMPROBAR(epochUTC(1970, 1, 1, 0, 0, 0) == 0);
  COMPROBAR(epochUTC(2027, 7, 14, 23, 0, 5) == utc(2027, 7, 14, 23, 0) + 5);
  COMPROBAR(epochUTC(2028, 2, 29, 12, 30, 0) == utc(2028, 2, 29, 12, 30));
}

// ------------------------------------------------------------- la helada

static void pruebaPsicrometria() {
  printf("psicrometría\n");
  CERCA(puntoRocio(20.0f, 50.0f), 9.26, 0.1);
  CERCA(puntoRocio(5.0f, 100.0f), 5.0, 0.05);
  CERCA(puntoRocio(2.0f, 60.0f), -4.9, 0.2);
  CERCA(bulboHumedo(20.0f, 50.0f), 13.7, 0.1);  // ejemplo del artículo de Stull
  COMPROBAR(std::isnan(puntoRocio(NAN, 50.0f)));
  COMPROBAR(std::isnan(puntoRocio(10.0f, 0.0f)));

  COMPROBAR(clasificarCielo(-25.0f, 3.0f) == Cielo::Sereno);
  COMPROBAR(clasificarCielo(-2.0f, 3.0f) == Cielo::Nublado);
  COMPROBAR(clasificarCielo(-10.0f, 3.0f) == Cielo::Velado);
  COMPROBAR(clasificarCielo(NAN, 3.0f) == Cielo::Desconocido);
}

// Una noche sintética: muestras cada 10 min de 18:00 a 08:00 UTC.
// El suelo baja en línea recta hasta `minSuelo` a las 06:00 y luego remonta.
static ResumenNoche nocheSintetica(float minSuelo, float inversion, bool serena,
                                   Temporada* temporada = nullptr) {
  Noche n;
  const time_t t0 = utc(2027, 1, 14, 18, 0);
  for (int i = 0; i <= 84; ++i) {
    Muestra m;
    m.t = t0 + i * 600;
    const float fi = static_cast<float>(i);
    const float f = i <= 72 ? fi / 72.0f : 1.0f - (fi - 72.0f) / 24.0f;
    const float suelo = 8.0f + (minSuelo - 8.0f) * f;
    const float inv = inversion * f;
    m.T[0] = suelo;
    m.T[1] = suelo + inv * 0.45f;
    m.T[2] = suelo + inv * 0.70f;
    m.T[3] = suelo + inv;
    m.Taire = m.T[2];
    m.HR = 70.0f;
    m.Tcielo = m.Taire + (serena ? -24.0f : -3.0f);
    n.anadir(m, 10.0f);
    if (temporada) temporada->anadirMuestra(m, 10.0f);
  }
  return n.resumen();
}

static void pruebaNoche() {
  printf("noche\n");
  const ResumenNoche r = nocheSintetica(-1.3f, 3.0f, true);
  COMPROBAR(r.muestras == 85);
  CERCA(r.minima[0], -1.3, 0.01);
  CERCA(r.minima[2], -1.3 + 2.1, 0.01);
  CERCA(r.minima[3], 1.7, 0.01);
  COMPROBAR(r.horaMinima[0] == utc(2027, 1, 15, 6, 0));
  int nivel = -1;
  CERCA(r.minimaAbsoluta(&nivel), -1.3, 0.01);
  COMPROBAR(nivel == 0);
  COMPROBAR(r.helo());
  CERCA(r.inversionMax, 3.0, 0.01);
  CERCA(r.fraccionSerena, 1.0, 0.001);
  COMPROBAR(r.radiativa());
  // suelo = 8 − 9,3·f < 0 ⇔ f > 0,860: bajando i = 62..72 y subiendo 73..75,
  // catorce muestras de diez minutos.
  CERCA(r.minutosBajo[0][kUmbralHelada], 140.0, 0.1);
  CERCA(r.minutosBajo[3][kUmbralHelada], 0.0, 0.1);
  COMPROBAR(r.minutosBajo[0][0] > r.minutosBajo[0][1]);  // <5 °C dura más que <1 °C
  COMPROBAR(!std::isnan(r.rocioMinimo));

  const ResumenNoche nublada = nocheSintetica(6.0f, 0.2f, false);
  COMPROBAR(!nublada.helo());
  COMPROBAR(!nublada.radiativa());
  CERCA(nublada.fraccionSerena, 0.0, 0.001);

  // El estado de la noche se copia byte a byte (memoria del reloj).
  {
    Noche a, b2;
    Muestra m0;
    m0.t = 500;
    for (int i = 0; i < kNiveles; ++i) m0.T[i] = 3.0f + static_cast<float>(i);
    m0.Taire = 5.0f;
    m0.HR = 80.0f;
    m0.Tcielo = -20.0f;
    a.anadir(m0, 10.0f);
    static_assert(std::is_trivially_copyable<Noche::Estado>::value, "POD");
    unsigned char crudo[sizeof(Noche::Estado)];
    std::memcpy(crudo, &a.estado(), sizeof(crudo));
    Noche::Estado e;
    std::memcpy(&e, crudo, sizeof(crudo));
    b2.restaurar(e);
    m0.T[0] = 1.0f;
    a.anadir(m0, 10.0f);
    b2.anadir(m0, 10.0f);
    CERCA(b2.resumen().minima[0], 1.0, 1e-6);
    COMPROBAR(b2.resumen().muestras == 2);
    CERCA(b2.resumen().inversionMedia, a.resumen().inversionMedia, 1e-6);
    CERCA(b2.resumen().fraccionSerena, 1.0, 1e-6);
  }

  // Una sonda rota no tumba el resumen.
  Noche n;
  Muestra m;
  m.t = 1000;
  m.T[0] = NAN;
  m.T[1] = 2.0f;
  m.T[2] = 2.5f;
  m.T[3] = NAN;
  m.Taire = NAN;
  m.HR = NAN;
  m.Tcielo = NAN;
  n.anadir(m, 10.0f);
  COMPROBAR(std::isnan(n.resumen().minima[0]));
  CERCA(n.resumen().minimaAbsoluta(), 2.0, 0.001);
  COMPROBAR(n.resumen().muestrasCielo == 0);
  COMPROBAR(!n.resumen().radiativa());
}

static void pruebaTemporada() {
  printf("temporada\n");
  Temporada t;
  COMPROBAR(t.veredicto() == Veredicto::SinDatos);

  t.cerrarNoche(nocheSintetica(3.0f, 2.0f, true, &t));  // 1 m: 4,4 °C
  COMPROBAR(t.noches == 1 && t.nochesHelada == 0);
  COMPROBAR(t.veredicto() == Veredicto::MallaPosible);
  COMPROBAR(t.horasFrio > 0);

  t.cerrarNoche(nocheSintetica(-0.5f, 2.0f, true, &t));  // suelo hiela, 1 m: 0,9 °C
  COMPROBAR(t.nochesHelada == 1);
  COMPROBAR(t.nochesBajoDecision == 1);
  CERCA(t.minimaRef, 0.9, 0.01);
  CERCA(t.minimaSuelo, -0.5, 0.01);
  COMPROBAR(t.veredicto() == Veredicto::InvernaderoObligatorio);
  COMPROBAR(t.nochesRadiativas == 2);

  // Horas frío: 6 h a 5 °C y 6 h a 10 °C → 6 horas.
  Temporada h;
  Muestra m;
  for (int i = 0; i < 72; ++i) {
    m.Taire = i < 36 ? 5.0f : 10.0f;
    h.anadirMuestra(m, 10.0f);
  }
  CERCA(h.horasFrio, 6.0, 0.01);
}

static void pruebaPronostico() {
  printf("pronóstico\n");
  PronosticoMinima p;
  float tmin, err;
  COMPROBAR(!p.predecir(8.0f, 2.0f, &tmin, &err));

  // Verdad oculta: Tmin = −4 + 0,6·T0 + 0,3·Td0 (+ ruido de ±0,4 °C).
  for (int i = 0; i < 40; ++i) {
    const float T0 = 9.0f + 5.0f * ruido();
    const float Td0 = 2.0f + 4.0f * ruido();
    p.entrenar(T0, Td0, -4.0f + 0.6f * T0 + 0.3f * Td0 + 0.4f * ruido());
    if (i == 5) COMPROBAR(!p.predecir(8.0f, 2.0f, &tmin, &err));
  }
  double b[3];
  COMPROBAR(p.coeficientes(b));
  CERCA(b[0], -4.0, 0.35);
  CERCA(b[1], 0.6, 0.05);
  CERCA(b[2], 0.3, 0.05);
  COMPROBAR(p.predecir(8.0f, 1.0f, &tmin, &err));
  CERCA(tmin, -4.0 + 4.8 + 0.3, 0.2);
  COMPROBAR(err >= 0.3f && err < 0.5f);  // el ruido uniforme ±0,4 tiene σ ≈ 0,23

  // NAN no entrena.
  const uint32_t antes = p.noches();
  p.entrenar(NAN, 1.0f, 2.0f);
  COMPROBAR(p.noches() == antes);

  // El estado se guarda y se recupera tal cual.
  PronosticoMinima q;
  q.restaurar(p.estado());
  float tmin2, err2;
  COMPROBAR(q.predecir(8.0f, 1.0f, &tmin2, &err2));
  CERCA(tmin2, tmin, 1e-5);

  // Predictores perfectamente colineales: no explota y predice con cordura.
  PronosticoMinima c;
  for (int i = 0; i < 12; ++i) {
    const float T0 = 6.0f + static_cast<float>(i) * 0.5f;
    c.entrenar(T0, T0 - 5.0f, 0.5f * T0 - 2.0f);
  }
  COMPROBAR(c.predecir(9.0f, 4.0f, &tmin, &err));
  CERCA(tmin, 2.5, 0.2);

  CERCA(tendenciaNocturna(8.0f, 0.0f, 0.0f, 12.0f), 8.0, 1e-6);
  CERCA(tendenciaNocturna(8.0f, 0.0f, 12.0f, 12.0f), 0.0, 1e-6);
  CERCA(tendenciaNocturna(8.0f, 0.0f, 3.0f, 12.0f), 4.0, 1e-6);
}

// --------------------------------------------------------------- la flor

static void disco(std::vector<uint8_t>& img, int w, int h, int cx, int cy, int r,
                  uint8_t v) {
  for (int y = cy - r; y <= cy + r; ++y) {
    for (int x = cx - r; x <= cx + r; ++x) {
      if (x < 0 || y < 0 || x >= w || y >= h) continue;
      if ((x - cx) * (x - cx) + (y - cy) * (y - cy) <= r * r) img[y * w + x] = v;
    }
  }
}

static void pruebaFlor() {
  printf("flor\n");
  const int w = 320, h = 240;
  std::vector<uint8_t> img(w * h);
  ParamsFlor p;

  // Noche cerrada con algo de ruido de sensor: ninguna flor.
  for (auto& v : img) v = static_cast<uint8_t>(20 + 15 * ruido());
  COMPROBAR(contarFlores(img.data(), w, h, p) == 0);

  // Cinco flores (discos de 9 px de radio)…
  const int centros[5][2] = {{40, 50}, {120, 70}, {200, 40}, {260, 150}, {80, 190}};
  for (const auto& c : centros) disco(img, w, h, c[0], c[1], 9, 235);
  // …un poste vertical brillante (largo y estrecho)…
  for (int y = 10; y < 230; ++y)
    for (int x = 300; x < 305; ++x) img[y * w + x] = 240;
  // …un alambre en diagonal (caja grande, casi vacía)…
  for (int i = 0; i < 100; ++i) {
    img[(120 + i) * w + (100 + i)] = 230;
    img[(120 + i) * w + (101 + i)] = 230;
  }
  // …gotas y brillos sueltos (demasiado pequeños)…
  for (int k = 0; k < 12; ++k) {
    const int x = 10 + 24 * k, y = 225;
    img[y * w + x] = img[y * w + x + 1] = 250;
    img[(y + 1) * w + x] = img[(y + 1) * w + x + 1] = 250;
  }
  // …y una hoja gris que no llega a blanca.
  disco(img, w, h, 160, 200, 14, 90);

  std::vector<Mancha> m;
  const int n = contarFlores(img.data(), w, h, p, &m);
  COMPROBAR(n == 5);
  COMPROBAR(m.size() == 5);
  for (const Mancha& x : m) {
    bool hallada = false;
    for (const auto& c : centros) {
      if (std::fabs(x.cx - static_cast<float>(c[0])) < 1.0f &&
          std::fabs(x.cy - static_cast<float>(c[1])) < 1.0f)
        hallada = true;
    }
    COMPROBAR(hallada);
    COMPROBAR(x.flores == 1);
    CERCA(x.area, 253.0, 8.0);
  }

  // Dos flores que se pisan (centros a 12 px, radio 9): una mancha, dos flores.
  disco(img, w, h, 52, 50, 9, 235);
  COMPROBAR(contarFlores(img.data(), w, h, p, &m) == 6);
  COMPROBAR(m.size() == 5);
  // Tres en fila, pegadas.
  disco(img, w, h, 215, 100, 8, 235);
  disco(img, w, h, 229, 102, 8, 235);
  disco(img, w, h, 243, 100, 8, 235);
  COMPROBAR(contarFlores(img.data(), w, h, p) == 9);
  // Una flor grande y sola sigue siendo una, y una alargada (1,5:1) también.
  std::vector<uint8_t> sola(w * h, 20);
  disco(sola, w, h, 160, 120, 30, 235);
  COMPROBAR(contarFlores(sola.data(), w, h, p) == 1);
  std::vector<uint8_t> ovalo(w * h, 20);
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const float dx = static_cast<float>(x - 160) / 18.0f;
      const float dy = static_cast<float>(y - 120) / 12.0f;
      if (dx * dx + dy * dy <= 1.0f) ovalo[y * w + x] = 235;
    }
  }
  COMPROBAR(contarFlores(ovalo.data(), w, h, p) == 1);
  // Un tallo oscuro de 2 px que cruza una flor por delante no la parte en dos.
  for (int y = 100; y < 140; ++y) sola[y * w + 160] = sola[y * w + 161] = 20;
  COMPROBAR(contarFlores(sola.data(), w, h, p) == 1);

  // --- Resta de fondo: una flor apoyada en un poste.
  std::vector<uint8_t> fondo(w * h, 0), noche(w * h), resta(w * h);
  auto escena = [&](bool conFlor) {
    for (auto& v : noche) v = static_cast<uint8_t>(22 + 8 * ruido());
    for (int y = 10; y < 230; ++y)
      for (int x = 150; x < 156; ++x) noche[y * w + x] = 210;  // el poste
    if (conFlor) disco(noche, w, h, 165, 120, 10, 235);        // lo toca
  };
  // Sin fondo aprendido, la flor se funde con el poste y se pierde.
  escena(true);
  COMPROBAR(contarFlores(noche.data(), w, h, p) == 0);
  // Seis noches sin flores: el aparato se aprende el poste.
  for (int k = 0; k < 6; ++k) {
    escena(false);
    actualizarFondo(fondo.data(), noche.data(), w * h);
  }
  COMPROBAR(fondo[120 * w + 152] >= 200);
  escena(true);
  restarFondo(noche.data(), fondo.data(), w * h, resta.data());
  COMPROBAR(contarFlores(resta.data(), w, h, p) == 1);
  // La flor de esta noche sube el fondo sólo 40 niveles y mañana se olvida.
  actualizarFondo(fondo.data(), noche.data(), w * h);
  COMPROBAR(fondo[120 * w + 170] > 40 && fondo[120 * w + 170] <= 75);
  escena(false);
  actualizarFondo(fondo.data(), noche.data(), w * h);
  COMPROBAR(fondo[120 * w + 170] < 40);
  // Y a la noche siguiente, otra flor en el mismo sitio se vuelve a contar.
  escena(true);
  restarFondo(noche.data(), fondo.data(), w * h, resta.data());
  COMPROBAR(contarFlores(resta.data(), w, h, p) == 1);

  // Blancura: blanco alto, verde y rojo bajos.
  const uint8_t rgb[9] = {250, 245, 240, 40, 200, 60, 220, 30, 30};
  uint8_t bl[3];
  blancuraRGB888(rgb, 3, 1, bl);
  COMPROBAR(bl[0] == 240 && bl[1] == 40 && bl[2] == 30);

  const uint8_t bloque[8] = {10, 20, 100, 100, 30, 40, 100, 104};
  uint8_t mitad[2];
  reducirMitad(bloque, 4, 2, mitad);
  COMPROBAR(mitad[0] == 25 && mitad[1] == 101);

  // Otsu separa dos poblaciones claras.
  std::vector<uint8_t> dos(1000);
  for (int i = 0; i < 1000; ++i) dos[i] = i < 900 ? 30 : 220;
  const int u = umbralOtsu(dos.data(), 1000);
  COMPROBAR(u > 30 && u <= 220);
}

static void pruebaEstimacion() {
  printf("estimación\n");
  double lo, hi;
  // Referencia: intervalo exacto de Poisson (bisección sobre la CDF).
  intervaloPoisson(0, 1.2816, &lo, &hi);
  CERCA(lo, 0.0, 1e-9);
  CERCA(hi, 2.303, 0.05);
  intervaloPoisson(1, 1.2816, &lo, &hi);
  CERCA(lo, 0.105, 0.02);
  CERCA(hi, 3.890, 0.05);
  intervaloPoisson(4, 1.2816, &lo, &hi);
  CERCA(lo, 1.745, 0.05);
  CERCA(hi, 7.994, 0.05);
  intervaloPoisson(14, 1.2816, &lo, &hi);
  CERCA(lo, 9.470, 0.05);
  CERCA(hi, 20.128, 0.05);
  intervaloPoisson(50, 1.96, &lo, &hi);
  CERCA(lo, 37.111, 0.05);
  CERCA(hi, 65.919, 0.05);

  ParcelaFlor p;
  const time_t noche = utc(2027, 7, 14, 23, 0);  // la una de la madrugada del 15
  const EstimacionFlor e = estimarFinca(14, noche, p);
  COMPROBAR(e.finca == 560);
  CERCA(e.fincaMin, 379, 3);
  CERCA(e.fincaMax, 805, 3);
  CERCA(e.horasPincel, 560.0 / 325.0, 0.001);
  CERCA(e.kilos, 560 * 0.85 * 0.489, 0.01);
  COMPROBAR(e.cosechaDesde == noche + 30 * 86400);
  COMPROBAR(estimarFinca(0, noche, p).finca == 0);

  CalendarioCosecha c;
  c.anotar(noche, 560);
  c.anotar(noche + 86400, 400);
  c.anotar(noche + 2 * 86400, 0);  // sin flores: no ocupa sitio
  const float total = c.kilosEntre(noche, noche + 60 * 86400, p);
  CERCA(total, 960 * 0.85 * 0.489, 0.05);
  // La ventana de la primera cohorte dura 6 días: la mitad, la mitad de kilos.
  const float media = c.kilosEntre(noche + 30 * 86400, noche + 33 * 86400, p);
  const float segunda = 400 * 0.85f * 0.489f * (2.0f / 6.0f);
  CERCA(media, 560 * 0.85 * 0.489 * 0.5 + segunda, 0.05);
  COMPROBAR(c.kilosEntre(noche, noche + 86400, p) == 0.0f);

  // El anillo no se desborda.
  CalendarioCosecha lleno;
  for (int i = 0; i < 200; ++i) lleno.anotar(noche + i * 86400, 10);
  COMPROBAR(lleno.estado().siguiente == 200 % CalendarioCosecha::kCohortes);
}

// --------------------------------------------------------------- el cante

static void pruebaCante() {
  printf("cante\n");
  IGUAL_TEXTO(num(-0.8f, 1), "−0,8");
  IGUAL_TEXTO(num(-0.04f, 1), "0,0");
  IGUAL_TEXTO(num(12.34f, 1), "12,3");
  IGUAL_TEXTO(num(3.0f, 0), "3");
  IGUAL_TEXTO(miles(1600), "1.600");
  IGUAL_TEXTO(miles(999), "999");
  IGUAL_TEXTO(miles(1234567), "1.234.567");
  IGUAL_TEXTO(horaCantada(7, 40), "Las ocho menos cuarto");
  IGUAL_TEXTO(horaCantada(7, 0), "Las siete");
  IGUAL_TEXTO(horaCantada(7, 10), "Las siete y cuarto");
  IGUAL_TEXTO(horaCantada(7, 30), "Las siete y media");
  IGUAL_TEXTO(horaCantada(0, 55), "La una");
  IGUAL_TEXTO(horaCantada(13, 2), "La una");
  IGUAL_TEXTO(horaCantada(23, 58), "Las doce");
  IGUAL_TEXTO(duracion(1.72f), "1 h 45 min");
  IGUAL_TEXTO(duracion(0.4f), "25 min");
  IGUAL_TEXTO(duracion(2.0f), "2 h");
  COMPROBAR(redondoHablado(563) == 560);
  COMPROBAR(redondoHablado(14) == 14);
  COMPROBAR(redondoHablado(233) == 230);
  COMPROBAR(redondoHablado(5920) == 5900);

  // Enero es CET (UTC+1); julio, CEST (UTC+2).
  IGUAL_TEXTO(horaReloj(utc(2027, 1, 15, 6, 0)), "07:00");
  IGUAL_TEXTO(horaReloj(utc(2027, 7, 15, 6, 0)), "08:00");
  IGUAL_TEXTO(fechaCorta(utc(2027, 8, 17, 23, 0)), "18 de agosto");

  ParcelaFlor p;
  const time_t noche = utc(2027, 7, 14, 23, 0);
  IGUAL_TEXTO(canteFlor(noche, estimarFinca(14, noche, p), p),
              "¡La una y floreciendo!\n"
              "14 flores abiertas en las 40 plantas centinela: unas 560 en la "
              "finca (entre 380 y 810).\n"
              "Mañana toca pincel: 1 h 45 min.\n"
              "Si cuajan, unos 230 kg entre el 14 de agosto y el 19 de agosto.");
  IGUAL_TEXTO(canteFlor(noche, estimarFinca(0, noche, p), p),
              "¡La una y sin flores!");

  Temporada t;
  const ResumenNoche r = nocheSintetica(-1.3f, 3.0f, true, &t);
  t.cerrarNoche(r);
  IGUAL_TEXTO(canteAmanecer(utc(2027, 1, 15, 7, 50), r, t),
              "¡Las nueve menos cuarto y helando!\n"
              "Mínima de la noche: −1,3 °C a ras de suelo (07:00).\n"
              "Perfil: −1,3 · 0,0 · 0,8 · 1,7 °C (a 0,1 · 0,5 · 1 · 2 m).\n"
              "Aire frío embalsado: hasta 3,0 °C más frío abajo que a 2 m.\n"
              "Helada: 140 min bajo cero a ras de suelo, 0 a 1 m.\n"
              // Aire a 1 m = 8 − 7,2·f < 7,2 ⇔ f > 0,111: 76 muestras = 12,7 h.
              "Temporada: 1 noche · 1 de helada · mínima a 1 m 0,8 °C · 13 horas "
              "frío.\n"
              "Veredicto provisional: invernadero obligatorio (1 m bajó de 1 °C).");

  Temporada t2;
  const ResumenNoche suave = nocheSintetica(6.0f, 0.2f, false, &t2);
  t2.cerrarNoche(suave);
  const std::string parte = canteAmanecer(utc(2027, 1, 15, 7, 0), suave, t2);
  COMPROBAR(parte.find("¡Las ocho y nublado!") == 0);
  COMPROBAR(parte.find("Sin aire frío embalsado.") != std::string::npos);
  COMPROBAR(parte.find("Sin helada.") != std::string::npos);
  COMPROBAR(parte.find("la malla sigue siendo posible") != std::string::npos);

  IGUAL_TEXTO(cantePronostico(utc(2027, 1, 14, 19, 10), 0.4f, 0.8f,
                              utc(2027, 1, 15, 7, 20)),
              "Aviso de las 20:10. Noche serena: mínima prevista a 1 m de 0,4 °C "
              "(±0,8), hacia las 08:20. Helada posible.");
  IGUAL_TEXTO(canteHelando(utc(2027, 1, 15, 4, 20), -0.3f, 0),
              "¡Helando! −0,3 °C a ras de suelo a las 05:20.");
}

// -------------------------------------------------------------- la guardia

static const double kLat = 37.3316, kLon = -1.7271;

// Muestra sintética: el suelo va de `tarde` al ocaso a `minSuelo` en el orto,
// con la ley de la raíz del tiempo; de día, 12 °C. `fuerza` es la inversión
// máxima entre 2 m y el suelo.
static Muestra muestraSintetica(time_t t, float tarde, float minSuelo,
                                float fuerza, bool serena) {
  Muestra m;
  m.t = t;
  time_t ini = 0, fin = 0;
  float suelo = 12.0f, inv = 0.0f;
  if (nocheQueContiene(t, kLat, kLon, 0, &ini, &fin)) {
    const float f = std::sqrt(static_cast<float>(t - ini) /
                              static_cast<float>(fin - ini));
    suelo = tarde + (minSuelo - tarde) * f;
    inv = fuerza * f;
  }
  m.T[0] = suelo;
  m.T[1] = suelo + inv * 0.35f;
  m.T[2] = suelo + inv * 0.65f;
  m.T[3] = suelo + inv;
  m.Taire = m.T[2];
  m.HR = 75.0f;
  m.Tcielo = m.Taire + (serena ? -25.0f : -3.0f);
  return m;
}

struct Recuento {
  int mensajes = 0, urgentes = 0, cierres = 0, fotos = 0;
  std::vector<std::string> textos;
};

static void recorrer(Guardia& g, time_t desde, time_t hasta, float tarde,
                     float minSuelo, float fuerza, bool serena, Recuento* r) {
  for (time_t t = desde; t <= hasta; t += 600) {
    const Salida s = g.muestra(muestraSintetica(t, tarde, minSuelo, fuerza, serena));
    r->mensajes += static_cast<int>(s.mensajes.size());
    for (const auto& x : s.mensajes) r->textos.push_back(x);
    if (s.urgente) ++r->urgentes;
    if (s.cierreNoche) ++r->cierres;
    if (s.pedirFoto) ++r->fotos;
  }
}

static void pruebaGuardia() {
  printf("guardia\n");
  COMPROBAR(mesEntre(11, 10, 3) && mesEntre(2, 10, 3) && !mesEntre(4, 10, 3));
  COMPROBAR(mesEntre(6, 5, 10) && !mesEntre(11, 5, 10));

  // Límites de la noche del 14 al 15 de enero (referencia astral).
  time_t ini = 0, fin = 0;
  COMPROBAR(nocheQueContiene(utc(2027, 1, 14, 22, 0), kLat, kLon, 30, &ini, &fin));
  CERCA(static_cast<double>(fin), 1799997621.0 + 1800, 180);
  CERCA(static_cast<double>(ini), 1800033143.0 - 86400 - 62, 240);  // ocaso del 14
  COMPROBAR(!nocheQueContiene(utc(2027, 1, 15, 12, 0), kLat, kLon, 30, &ini, &fin));
  COMPROBAR(nocheQueContiene(utc(2027, 1, 15, 7, 40), kLat, kLon, 30, &ini, &fin));
  COMPROBAR(!nocheQueContiene(utc(2027, 1, 15, 8, 0), kLat, kLon, 30, &ini, &fin));
  // Lejos de Greenwich el día UTC y el solar no coinciden: California, 19 h local.
  COMPROBAR(nocheQueContiene(utc(2027, 1, 15, 3, 0), 37.0, -120.0, 30, &ini, &fin));
  COMPROBAR(ini <= utc(2027, 1, 15, 3, 0) && ini > utc(2027, 1, 15, 0, 0));
  COMPROBAR(fin > utc(2027, 1, 15, 14, 0) && fin < utc(2027, 1, 15, 17, 0));
  COMPROBAR(!nocheQueContiene(utc(2027, 1, 15, 20, 0), 37.0, -120.0, 30, &ini, &fin));

  Config c;
  c.latitud = kLat;
  c.longitud = kLon;

  // --- Una noche de helada completa, de mediodía a mediodía.
  {
    Guardia g(c);
    Recuento r;
    recorrer(g, utc(2027, 1, 14, 12, 0), utc(2027, 1, 15, 12, 0), 9.0f, -1.5f,
             3.0f, true, &r);
    COMPROBAR(r.cierres == 1);
    COMPROBAR(r.fotos == 0);       // enero no es temporada de flor
    COMPROBAR(r.urgentes == 1);    // la alarma de helada, una sola vez
    COMPROBAR(r.mensajes == 2);
    COMPROBAR(r.textos.size() == 2 && r.textos[0].find("¡Helando!") == 0);
    COMPROBAR(r.textos.size() == 2 &&
              r.textos[1].find("y helando!") != std::string::npos);
    const Persistente& p = g.estado();
    COMPROBAR(p.enNoche == 0);
    COMPROBAR(p.temporada.noches == 1 && p.temporada.nochesHelada == 1);
    COMPROBAR(p.pronostico.n == 1);  // noche radiativa: el pronóstico aprende
    COMPROBAR(p.anioTemporada == 2026);
    COMPROBAR(p.temporada.horasFrio > 5.0f);
    CERCA(p.temporada.minimaSuelo, -1.5, 0.15);
  }

  // --- Noche nublada: ni alarma ni lección para el pronóstico.
  {
    Guardia g(c);
    Recuento r;
    recorrer(g, utc(2027, 1, 14, 12, 0), utc(2027, 1, 15, 12, 0), 11.0f, 7.0f,
             0.2f, false, &r);
    COMPROBAR(r.cierres == 1 && r.urgentes == 0 && r.mensajes == 1);
    COMPROBAR(g.estado().pronostico.n == 0);
    COMPROBAR(g.estado().temporada.nochesHelada == 0);
  }

  // --- Con el pronóstico ya aprendido, avisa a primera hora de la noche.
  {
    Guardia g(c);
    PronosticoMinima pr;
    for (int i = 0; i < 12; ++i) {
      const float T0 = 5.0f + static_cast<float>(i) * 0.5f;
      pr.entrenar(T0, T0 - 4.0f + 0.3f * ruido(), 0.8f * T0 - 6.0f);
    }
    Persistente p = g.estado();
    p.pronostico = pr.estado();
    COMPROBAR(g.restaurar(p));
    Recuento r;
    recorrer(g, utc(2027, 1, 14, 12, 0), utc(2027, 1, 14, 21, 0), 9.0f, -1.5f,
             3.0f, true, &r);
    COMPROBAR(r.urgentes == 1 && r.mensajes == 1);
    COMPROBAR(r.textos.size() == 1 && r.textos[0].find("Aviso de las 20:") == 0);
    COMPROBAR(g.estado().t0Tomado == 1);
    // Misma noche pero cubierta: no hay aviso (el método es para noches serenas).
    Guardia h(c);
    COMPROBAR(h.restaurar(p));
    Recuento r2;
    recorrer(h, utc(2027, 1, 14, 12, 0), utc(2027, 1, 14, 21, 0), 9.0f, 4.0f,
             0.2f, false, &r2);
    COMPROBAR(r2.mensajes == 0);
  }

  // --- Interrumpir la noche (sueño profundo, memoria copiada byte a byte)
  //     no cambia el resultado.
  {
    Guardia entera(c), a(c);
    Recuento re, ra;
    recorrer(entera, utc(2027, 1, 14, 12, 0), utc(2027, 1, 15, 12, 0), 9.0f,
             -1.5f, 3.0f, true, &re);
    recorrer(a, utc(2027, 1, 14, 12, 0), utc(2027, 1, 15, 1, 0), 9.0f, -1.5f,
             3.0f, true, &ra);
    static_assert(std::is_trivially_copyable<Persistente>::value, "POD");
    std::vector<unsigned char> crudo(sizeof(Persistente));
    std::memcpy(crudo.data(), &a.estado(), crudo.size());
    Persistente p;
    std::memcpy(&p, crudo.data(), crudo.size());
    Guardia b(c);
    COMPROBAR(b.restaurar(p));
    recorrer(b, utc(2027, 1, 15, 1, 10), utc(2027, 1, 15, 12, 0), 9.0f, -1.5f,
             3.0f, true, &ra);
    COMPROBAR(ra.textos == re.textos);
    COMPROBAR(std::memcmp(&b.estado().temporada, &entera.estado().temporada,
                          sizeof(Temporada)) == 0);

    Persistente malo = p;
    malo.magia = 0xDEADBEEF;
    Guardia d(c);
    COMPROBAR(!d.restaurar(malo));
  }

  // --- Noche de verano: una foto, y el parte de flores.
  {
    Guardia g(c);
    Recuento r;
    // Del miércoles 14 al jueves 15 de julio de 2027.
    recorrer(g, utc(2027, 7, 14, 12, 0), utc(2027, 7, 15, 12, 0), 26.0f, 19.0f,
             1.0f, true, &r);
    COMPROBAR(r.fotos == 1);
    COMPROBAR(r.cierres == 1);
    COMPROBAR(r.mensajes == 0);  // fuera de la temporada fría no hay parte diario
    COMPROBAR(g.estado().temporada.horasFrio == 0.0f);
    const std::string parte = g.flores(utc(2027, 7, 14, 23, 0), 14);
    COMPROBAR(parte.find("¡La una y floreciendo!") == 0);
    COMPROBAR(g.flores(utc(2027, 7, 15, 23, 0), 0).empty());
    CalendarioCosecha cal;
    cal.restaurar(g.estado().cosecha);
    CERCA(cal.kilosEntre(utc(2027, 8, 1, 0, 0), utc(2027, 9, 1, 0, 0), c.parcela),
          560 * 0.85 * 0.489, 0.1);
    // El domingo sí hay parte semanal: del sábado 17 al domingo 18 de julio.
    Recuento dom;
    recorrer(g, utc(2027, 7, 17, 12, 0), utc(2027, 7, 18, 12, 0), 26.0f, 19.0f,
             1.0f, true, &dom);
    COMPROBAR(dom.mensajes == 1 && dom.fotos == 1);
  }

  // --- La temporada se estrena en octubre.
  {
    Guardia g(c);
    Recuento r;
    recorrer(g, utc(2027, 1, 14, 12, 0), utc(2027, 1, 15, 12, 0), 9.0f, -1.5f,
             3.0f, true, &r);
    COMPROBAR(g.estado().temporada.noches == 1);
    recorrer(g, utc(2027, 10, 1, 12, 0), utc(2027, 10, 2, 12, 0), 20.0f, 14.0f,
             1.0f, true, &r);
    COMPROBAR(g.estado().anioTemporada == 2027);
    COMPROBAR(g.estado().temporada.noches == 1);       // sólo la de octubre
    COMPROBAR(g.estado().temporada.nochesHelada == 0);
    COMPROBAR(g.estado().pronostico.n >= 1);           // lo aprendido se conserva
  }
}

int main() {
  setenv("TZ", kZonaPeninsular, 1);
  tzset();
  pruebaSol();
  pruebaPsicrometria();
  pruebaNoche();
  pruebaTemporada();
  pruebaPronostico();
  pruebaFlor();
  pruebaEstimacion();
  pruebaCante();
  pruebaGuardia();
  printf("\n%d comprobaciones, %d fallos\n", g_pruebas, g_fallos);
  return g_fallos == 0 ? 0 : 1;
}
