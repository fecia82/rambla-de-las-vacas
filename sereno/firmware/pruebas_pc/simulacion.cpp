// Simulación de una temporada completa con el núcleo real de Sereno.
//
//   make sim   →  ../docs/simulacion.json
//
// OJO: el TIEMPO es inventado. Un generador de juguete fabrica un invierno y un
// verano verosímiles para un fondo de vallejo (noches despejadas y en calma →
// aire frío embalsado junto al suelo) y unas oleadas de floración. Lo que NO es
// inventado es todo lo demás: las muestras pasan por la misma Guardia, el mismo
// pronóstico, el mismo contador de flores y los mismos partes que lleva la placa.
// Sirve para enseñar cómo se comporta el aparato, no para predecir esta finca.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "cante.h"
#include "flor.h"
#include "guardia.h"
#include "helada.h"
#include "sol.h"

using namespace sereno;

namespace {

const double kLat = 37.3316, kLon = -1.7271;
const double kPi = 3.14159265358979323846;

// --- azar reproducible
uint32_t g_x = 2463534242u;
double uniforme() {
  g_x ^= g_x << 13;
  g_x ^= g_x >> 17;
  g_x ^= g_x << 5;
  return (g_x >> 8) / 16777216.0;
}
double normal() {
  const double u = uniforme() + 1e-12, v = uniforme();
  return std::sqrt(-2.0 * std::log(u)) * std::cos(2 * kPi * v);
}
int poisson(double lambda) {
  if (lambda <= 0) return 0;
  if (lambda > 60) {
    const double v = lambda + std::sqrt(lambda) * normal();
    return v < 0 ? 0 : static_cast<int>(std::lround(v));
  }
  const double limite = std::exp(-lambda);
  int k = 0;
  double p = uniforme();
  while (p > limite) {
    ++k;
    p *= uniforme();
  }
  return k;
}
double acotar(double v, double lo, double hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}

std::string fechaISO(time_t t) {
  struct tm l;
  localtime_r(&t, &l);
  char b[40];
  snprintf(b, sizeof(b), "%04d-%02d-%02d", l.tm_year + 1900, l.tm_mon + 1,
           l.tm_mday);
  return b;
}

std::string json(const std::string& s) {
  std::string o = "\"";
  for (char c : s) {
    if (c == '"') o += "\\\"";
    else if (c == '\\') o += "\\\\";
    else if (c == '\n') o += "\\n";
    else o += c;
  }
  return o + "\"";
}

double hrDesdeRocio(double T, double Td) {
  const double a = 17.62, b = 243.12;
  return acotar(100.0 * std::exp(a * Td / (b + Td) - a * T / (b + T)), 5, 100);
}

// --- el tiempo de juguete -------------------------------------------------

struct TiempoNoche {
  double nubes = 0;      // 0 despejado … 1 cubierto
  double viento = 0;     // m/s
  double minRegional = 0;  // la mínima «de mapa», la que vería un satélite
  double tarde = 0;      // temperatura al ocaso
  double rocio = 0;
  double embalse = 0;    // cuánto más frío llega a estar el suelo que el aire libre
  double maxDia = 0;
};

struct Generador {
  double anomalia = 0, latenteNubes = 0;

  TiempoNoche siguiente(time_t ocaso) {
    struct tm g;
    gmtime_r(&ocaso, &g);
    const double estacional =
        10.5 - 4.0 * std::cos(2 * kPi * (g.tm_yday - 20) / 365.0) +
        (g.tm_mon >= 5 && g.tm_mon <= 8 ? 5.0 : 0.0);
    anomalia = 0.7 * anomalia + std::sqrt(1 - 0.49) * 2.5 * normal();
    latenteNubes = 0.6 * latenteNubes + std::sqrt(1 - 0.36) * 0.7 * normal();
    TiempoNoche n;
    n.nubes = acotar(0.25 + 0.8 * latenteNubes, 0, 1);
    n.viento = std::fabs(normal()) * 1.8;
    n.minRegional = estacional + anomalia + 2.0 * n.nubes - 0.6;
    n.tarde = n.minRegional + 6.5 + 1.5 * (1 - n.nubes) + 0.6 * normal();
    n.rocio = n.minRegional - 1.5 - 3.0 * (1 - n.nubes) + 0.8 * normal();
    n.embalse = 4.5 * std::pow(1 - n.nubes, 1.5) * std::exp(-n.viento / 1.5);
    n.maxDia = n.minRegional + 8.0 + 3.0 * (1 - n.nubes);
    return n;
  }
};

// Estado del tiempo continuo: la noche en curso y la tarde que viene.
struct Atmosfera {
  Generador gen;
  TiempoNoche noche;         // la noche en curso o la última
  time_t ocaso = 0, orto = 0;  // límites de esa noche
  TiempoNoche proxima;
  time_t proximoOcaso = 0;

  void preparar(time_t t) {
    // Noche que contiene t o la siguiente.
    const SolDia hoy = solDelDia(t, kLat, kLon);
    ocaso = hoy.ocaso;
    orto = solDelDia(t + 86400, kLat, kLon).orto;
    noche = gen.siguiente(ocaso);
    proximoOcaso = solDelDia(t + 86400, kLat, kLon).ocaso;
    proxima = gen.siguiente(proximoOcaso);
  }

  void avanzar(time_t t) {
    if (t >= proximoOcaso) {
      noche = proxima;
      ocaso = proximoOcaso;
      orto = solDelDia(ocaso + 86400, kLat, kLon).orto;
      if (orto < ocaso) orto = solDelDia(ocaso + 2 * 86400, kLat, kLon).orto;
      proximoOcaso = solDelDia(ocaso + 86400, kLat, kLon).ocaso;
      proxima = gen.siguiente(proximoOcaso);
    }
  }

  // Aire libre (por encima del embalse) y fuerza del embalse en el instante t.
  void estado(time_t t, double* aireLibre, double* embalse) const {
    if (t < ocaso) {  // primera tarde de la simulación
      *aireLibre = noche.tarde + 2.0;
      *embalse = 0;
      return;
    }
    if (t <= orto) {
      const double tau = static_cast<double>(t - ocaso) / (orto - ocaso);
      *aireLibre = noche.tarde + (noche.minRegional - noche.tarde) * std::sqrt(tau);
      *embalse = noche.embalse * std::sqrt(tau);
      return;
    }
    // De día: sube hasta la máxima a media tarde y baja hacia el próximo ocaso.
    const time_t cima = orto + (proximoOcaso - orto) * 55 / 100;
    if (t <= cima) {
      const double f = static_cast<double>(t - orto) / (cima - orto);
      *aireLibre = noche.minRegional +
                   (noche.maxDia - noche.minRegional) * 0.5 * (1 - std::cos(kPi * f));
    } else {
      const double f = static_cast<double>(t - cima) / (proximoOcaso - cima);
      *aireLibre = noche.maxDia +
                   (proxima.tarde - noche.maxDia) * 0.5 * (1 - std::cos(kPi * f));
    }
    const double desdeOrto = static_cast<double>(t - orto);
    *embalse = desdeOrto < 2700 ? noche.embalse * (1 - desdeOrto / 2700.0) : 0;
  }

  Muestra medir(time_t t, double* aireLibreSalida = nullptr) const {
    double libre = 0, embalse = 0;
    estado(t, &libre, &embalse);
    if (aireLibreSalida) *aireLibreSalida = libre;
    Muestra m;
    m.t = t;
    for (int i = 0; i < kNiveles; ++i) {
      const double real = libre - embalse * std::exp(-kAlturasM[i] / 1.2);
      // Una DS18B20: ruido pequeño y pasos de 1/16 de grado.
      m.T[i] = static_cast<float>(std::round((real + 0.05 * normal()) * 16.0) / 16.0);
    }
    const double aire1m = libre - embalse * std::exp(-1.0 / 1.2) + 0.04 * normal();
    m.Taire = static_cast<float>(aire1m);
    const double rocio = noche.rocio < aire1m - 0.3 ? noche.rocio : aire1m - 0.3;
    m.HR = static_cast<float>(hrDesdeRocio(aire1m, rocio));
    m.Tcielo = static_cast<float>(aire1m - 28.0 + 24.0 * noche.nubes + normal());
    return m;
  }
};

// --- la foto nocturna de juguete -------------------------------------------

struct FlorPintada {
  int x, y, r;
};

// Una flor no es un disco: aquí es un contorno con seis o siete lóbulos, y a una
// de cada cuatro le cruza un tallo oscuro por delante.
void pintarFlor(std::vector<uint8_t>& img, int w, int h, const FlorPintada& f) {
  const int lobulos = 6 + (uniforme() < 0.5 ? 0 : 1);
  const double fase = uniforme() * 2 * kPi;
  const int valor = 215 + static_cast<int>(uniforme() * 30);
  const int alcance = f.r + 3;
  for (int y = f.y - alcance; y <= f.y + alcance; ++y) {
    for (int x = f.x - alcance; x <= f.x + alcance; ++x) {
      if (x < 0 || y < 0 || x >= w || y >= h) continue;
      const double dx = x - f.x, dy = y - f.y;
      const double radio =
          f.r * (1.0 + 0.16 * std::cos(lobulos * std::atan2(dy, dx) + fase));
      if (dx * dx + dy * dy <= radio * radio) {
        img[y * w + x] = static_cast<uint8_t>(valor);
      }
    }
  }
  if (uniforme() < 0.25) {
    const int desvio = static_cast<int>((uniforme() - 0.5) * f.r);
    for (int y = f.y - alcance; y <= f.y + alcance; ++y) {
      for (int k = 0; k < 2; ++k) {
        const int x = f.x + desvio + k + (y - f.y) / 4;
        if (x < 0 || y < 0 || x >= w || y >= h) continue;
        img[y * w + x] = 30;
      }
    }
  }
}

// Escena: fondo oscuro con ruido, dos postes, una goma de riego, brillos
// sueltos y `n` flores de tamaño variable (las lejanas, más pequeñas). Postes
// y goma están en el mismo sitio todas las noches; las flores, no.
std::vector<FlorPintada> pintarEscena(std::vector<uint8_t>& img, int w, int h,
                                      int n) {
  for (auto& v : img) v = static_cast<uint8_t>(acotar(24 + 9 * normal(), 0, 80));
  for (int y = 20; y < h; ++y) {
    for (int x = 58; x < 64; ++x) img[y * w + x] = 205;
    for (int x = 262; x < 266; ++x) img[y * w + x] = 180;
  }
  for (int x = 0; x < w; ++x) {
    const int y = 208 + static_cast<int>(6 * std::sin(x / 50.0));
    img[y * w + x] = 170;
    img[(y + 1) * w + x] = 170;
  }
  for (int k = 0; k < 25; ++k) {
    const int x = 2 + static_cast<int>(uniforme() * (w - 4));
    const int y = 2 + static_cast<int>(uniforme() * (h - 4));
    img[y * w + x] = 250;
    if (uniforme() < 0.5) img[y * w + x + 1] = 240;
  }
  std::vector<FlorPintada> flores;
  for (int k = 0; k < n; ++k) {
    FlorPintada f;
    f.y = 40 + static_cast<int>(uniforme() * 150);
    // Cuanto más abajo en la imagen, más cerca y más grande.
    f.r = static_cast<int>(std::lround(5.0 + 6.5 * (f.y - 40) / 150.0 + uniforme()));
    f.x = 14 + static_cast<int>(uniforme() * (w - 28));
    pintarFlor(img, w, h, f);
    flores.push_back(f);
  }
  return flores;
}

}  // namespace

int main(int argc, char** argv) {
  setenv("TZ", kZonaPeninsular, 1);
  tzset();
  const char* ruta = argc > 1 ? argv[1] : "simulacion.json";
  FILE* f = fopen(ruta, "w");
  if (!f) {
    perror(ruta);
    return 1;
  }

  Config cfg;
  cfg.latitud = kLat;
  cfg.longitud = kLon;
  Guardia guardia(cfg);

  // ======================================================= INVIERNO
  struct RegistroNoche {
    time_t fin;
    double regional;
    ResumenNoche r;
    bool hayPron;
    float pron;
  };
  std::vector<RegistroNoche> noches;
  struct Punto {
    time_t t;
    float T[kNiveles];
    float libre, dCielo;
  };
  std::vector<std::vector<Punto>> series;  // una por noche
  std::vector<std::vector<std::pair<time_t, std::string>>> partes;

  Atmosfera atm;
  const time_t inicio = epochUTC(2026, 11, 1, 12, 0, 0);
  const time_t finInvierno = epochUTC(2027, 3, 31, 12, 0, 0);
  atm.preparar(inicio);

  std::vector<Punto> serieActual;
  std::vector<std::pair<time_t, std::string>> partesActual;
  bool estabaEnNoche = false, hayPron = false;
  float pron = 0;
  double regionalNoche = 0;
  int mensajesTotales = 0, avisosPrevios = 0, alarmas = 0;

  for (time_t t = inicio; t <= finInvierno; t += 600) {
    atm.avanzar(t);
    double libre = 0;
    const Muestra m = atm.medir(t, &libre);
    const uint8_t t0Antes = guardia.estado().t0Tomado;
    const Salida s = guardia.muestra(m);
    const Persistente& p = guardia.estado();

    for (const auto& txt : s.mensajes) {
      partesActual.emplace_back(t, txt);
      ++mensajesTotales;
      if (txt.rfind("Aviso", 0) == 0) ++avisosPrevios;
      if (txt.rfind("¡Helando!", 0) == 0) ++alarmas;
    }

    if (p.enNoche) {
      if (!estabaEnNoche) {
        serieActual.clear();
        hayPron = false;
        regionalNoche = atm.noche.minRegional;
        // Los avisos de esta muestra pertenecen ya a la noche nueva.
      }
      Punto pt;
      pt.t = t;
      for (int i = 0; i < kNiveles; ++i) pt.T[i] = m.T[i];
      pt.libre = static_cast<float>(libre);
      pt.dCielo = m.Tcielo - m.Taire;
      serieActual.push_back(pt);
      if (!t0Antes && p.t0Tomado) {
        PronosticoMinima pr;
        pr.restaurar(p.pronostico);
        float e = 0;
        hayPron = pr.predecir(p.T0, p.Td0, &pron, &e);
      }
    }
    if (s.cierreNoche) {
      Noche n;
      n.restaurar(p.noche);
      noches.push_back({t, regionalNoche, n.resumen(), hayPron, pron});
      series.push_back(serieActual);
      partes.push_back(partesActual);
      partesActual.clear();
    }
    estabaEnNoche = p.enNoche;
  }

  // La noche de muestra: la más fría a ras de suelo entre las radiativas.
  size_t elegida = 0;
  for (size_t i = 0; i < noches.size(); ++i) {
    if (noches[i].r.radiativa() &&
        (!noches[elegida].r.radiativa() ||
         noches[i].r.minima[0] < noches[elegida].r.minima[0])) {
      elegida = i;
    }
  }

  const Temporada temporada = guardia.estado().temporada;
  double sesgo = 0, sesgoRad = 0;
  int nRad = 0, heladasRegionales = 0;
  double sumaErr2 = 0;
  int nPron = 0;
  for (const auto& n : noches) {
    sesgo += n.r.minima[kNivelSuelo] - n.regional;
    if (n.regional < 0) ++heladasRegionales;
    if (n.r.radiativa()) {
      sesgoRad += n.r.minima[kNivelSuelo] - n.regional;
      ++nRad;
      if (n.hayPron) {
        const double e = n.pron - n.r.minima[kNivelRef];
        sumaErr2 += e * e;
        ++nPron;
      }
    }
  }

  fprintf(f, "{\n\"aviso\": %s,\n",
          json("Tiempo simulado. La lógica es la del firmware real.").c_str());
  fprintf(f, "\"invierno\": {\n  \"noches\": [\n");
  for (size_t i = 0; i < noches.size(); ++i) {
    const auto& n = noches[i];
    fprintf(f,
            "    {\"f\":\"%s\",\"reg\":%.1f,\"t\":[%.1f,%.1f,%.1f,%.1f],"
            "\"ser\":%.2f,\"rad\":%d,\"hel\":%d,\"pron\":%s}%s\n",
            fechaISO(n.fin).c_str(), n.regional, n.r.minima[0], n.r.minima[1],
            n.r.minima[2], n.r.minima[3], n.r.fraccionSerena,
            n.r.radiativa() ? 1 : 0, n.r.helo() ? 1 : 0,
            n.hayPron ? num(n.pron, 1).replace(num(n.pron, 1).find(','), 1, ".")
                            .c_str()
                      : "null",
            i + 1 < noches.size() ? "," : "");
  }
  fprintf(f, "  ],\n");
  fprintf(f,
          "  \"resumen\": {\"noches\":%d,\"heladas\":%d,\"bajoDecision\":%d,"
          "\"radiativas\":%d,\"minimaRef\":%.1f,\"minimaSuelo\":%.1f,"
          "\"horasFrio\":%.0f,\"heladasRegionales\":%d,\"sesgoMedio\":%.2f,"
          "\"sesgoRadiativas\":%.2f,\"errorPronostico\":%.2f,"
          "\"nochesPronosticadas\":%d,\"mensajes\":%d,\"avisos\":%d,"
          "\"alarmas\":%d,\"veredicto\":%d},\n",
          temporada.noches, temporada.nochesHelada, temporada.nochesBajoDecision,
          temporada.nochesRadiativas, temporada.minimaRef, temporada.minimaSuelo,
          temporada.horasFrio, heladasRegionales, sesgo / static_cast<double>(noches.size()),
          nRad ? sesgoRad / nRad : 0.0, nPron ? std::sqrt(sumaErr2 / nPron) : 0.0,
          nPron, mensajesTotales, avisosPrevios, alarmas,
          static_cast<int>(temporada.veredicto()));

  {
    const auto& n = noches[elegida];
    const auto& serie = series[elegida];
    time_t ocaso = 0, limite = 0;
    nocheQueContiene(serie[serie.size() / 2].t, kLat, kLon, 30, &ocaso, &limite);
    fprintf(f, "  \"noche\": {\"fecha\":\"%s\",\"ocaso\":%ld,\"orto\":%ld,\n",
            fechaISO(n.fin).c_str(), static_cast<long>(ocaso),
            static_cast<long>(limite - 1800));
    fprintf(f, "    \"serie\": [");
    for (size_t i = 0; i < serie.size(); ++i) {
      const auto& p = serie[i];
      fprintf(f, "%s[%ld,%.2f,%.2f,%.2f,%.2f,%.2f,%.1f]", i ? "," : "",
              static_cast<long>(p.t), p.T[0], p.T[1], p.T[2], p.T[3], p.libre,
              p.dCielo);
    }
    fprintf(f, "],\n    \"mensajes\": [");
    for (size_t i = 0; i < partes[elegida].size(); ++i) {
      fprintf(f, "%s{\"t\":%ld,\"hora\":\"%s\",\"txt\":%s}", i ? "," : "",
              static_cast<long>(partes[elegida][i].first),
              horaReloj(partes[elegida][i].first).c_str(),
              json(partes[elegida][i].second).c_str());
    }
    fprintf(f, "]\n  },\n");
    fprintf(f, "  \"ultimoParte\": %s\n},\n",
            json(partes.back().empty() ? "" : partes.back().back().second).c_str());
  }

  // ========================================================= VERANO
  struct NocheFlor {
    time_t t;
    int verdadFinca, reales, vistas;
    EstimacionFlor e;
  };
  std::vector<NocheFlor> flores;
  const int w = 320, h = 240;
  std::vector<uint8_t> img(w * h), fondo(w * h, 0), resta(w * h);
  ParamsFlor params;

  // Seis oleadas de siete noches. 11.000 flores en la temporada: con el cuajado
  // y el peso por fruto de la parcela, los ~4.500 kg del año 6 de la guía.
  const double floresTemporada = 11000.0;
  const int inicioOleada[6] = {4, 25, 44, 63, 82, 99};  // días desde el 1 de junio
  const double pesoOleada[6] = {0.10, 0.18, 0.24, 0.22, 0.16, 0.10};
  const double perfil[7] = {0.08, 0.20, 0.30, 0.22, 0.12, 0.06, 0.02};
  const time_t junio = epochUTC(2027, 6, 1, 12, 0, 0);
  const time_t finVerano = epochUTC(2027, 9, 20, 12, 0, 0);

  size_t mejorNoche = 0;
  std::vector<FlorPintada> escenaFlores;
  std::vector<Mancha> escenaCajas;
  std::string parteFlor;

  atm.preparar(junio);
  for (time_t t = junio; t <= finVerano; t += 600) {
    atm.avanzar(t);
    const Salida s = guardia.muestra(atm.medir(t));
    if (!s.pedirFoto) continue;

    const int dia = static_cast<int>((t - junio) / 86400);
    double esperadas = 0;
    for (int k = 0; k < 6; ++k) {
      const int d = dia - inicioOleada[k];
      if (d >= 0 && d < 7) esperadas += floresTemporada * pesoOleada[k] * perfil[d];
    }
    NocheFlor nf;
    nf.t = t;
    nf.verdadFinca =
        esperadas > 0 ? poisson(esperadas * std::exp(0.25 * normal())) : 0;
    nf.reales = poisson(nf.verdadFinca * static_cast<double>(
                                             cfg.parcela.plantasCentinela) /
                        cfg.parcela.plantasTotales);
    const std::vector<FlorPintada> pintadas = pintarEscena(img, w, h, nf.reales);
    // Como en la placa: restar lo que brilla todas las noches, contar y
    // después enseñarle al fondo la foto de hoy.
    std::vector<Mancha> manchas;
    restarFondo(img.data(), fondo.data(), w * h, resta.data());
    nf.vistas = contarFlores(resta.data(), w, h, params, &manchas);
    actualizarFondo(fondo.data(), img.data(), w * h);
    nf.e = estimarFinca(nf.vistas, t, cfg.parcela);
    const std::string parte = guardia.flores(t, nf.vistas);
    flores.push_back(nf);
    if (nf.reales >= 10 && nf.reales <= 16 &&
        (escenaFlores.empty() || nf.reales > flores[mejorNoche].reales)) {
      mejorNoche = flores.size() - 1;
      escenaFlores = pintadas;
      escenaCajas = manchas;
      parteFlor = parte;
    }
  }

  int totalVerdad = 0, totalEstimado = 0, nochesConFlor = 0, dentro = 0;
  int sumaReales = 0, sumaVistas = 0;
  for (const auto& n : flores) {
    totalVerdad += n.verdadFinca;
    totalEstimado += n.e.finca;
    sumaReales += n.reales;
    sumaVistas += n.vistas;
    if (n.verdadFinca > 0 || n.vistas > 0) {
      ++nochesConFlor;
      if (n.verdadFinca >= n.e.fincaMin && n.verdadFinca <= n.e.fincaMax) ++dentro;
    }
  }

  fprintf(f, "\"verano\": {\n  \"noches\": [\n");
  for (size_t i = 0; i < flores.size(); ++i) {
    const auto& n = flores[i];
    fprintf(f,
            "    {\"f\":\"%s\",\"verdad\":%d,\"reales\":%d,\"vistas\":%d,"
            "\"finca\":%d,\"min\":%d,\"max\":%d}%s\n",
            fechaISO(n.t).c_str(), n.verdadFinca, n.reales, n.vistas, n.e.finca,
            n.e.fincaMin, n.e.fincaMax, i + 1 < flores.size() ? "," : "");
  }
  fprintf(f, "  ],\n");
  fprintf(f,
          "  \"resumen\": {\"nochesConFlor\":%d,\"totalVerdad\":%d,"
          "\"totalEstimado\":%d,\"dentroHorquilla\":%d,\"floresReales\":%d,"
          "\"floresVistas\":%d},\n",
          nochesConFlor, totalVerdad, totalEstimado, dentro, sumaReales,
          sumaVistas);

  fprintf(f, "  \"noche\": {\"fecha\":\"%s\",\"ancho\":%d,\"alto\":%d,\"flores\":[",
          fechaISO(flores[mejorNoche].t).c_str(), w, h);
  for (size_t i = 0; i < escenaFlores.size(); ++i) {
    fprintf(f, "%s[%d,%d,%d]", i ? "," : "", escenaFlores[i].x, escenaFlores[i].y,
            escenaFlores[i].r);
  }
  fprintf(f, "],\"cajas\":[");
  for (size_t i = 0; i < escenaCajas.size(); ++i) {
    fprintf(f, "%s[%d,%d,%d,%d,%d]", i ? "," : "", escenaCajas[i].x0,
            escenaCajas[i].y0, escenaCajas[i].x1, escenaCajas[i].y1,
            escenaCajas[i].flores);
  }
  fprintf(f, "],\"reales\":%d,\"vistas\":%d,\"parte\":%s},\n",
          flores[mejorNoche].reales, flores[mejorNoche].vistas,
          json(parteFlor).c_str());

  // Calendario de cosecha por semanas, con el mismo calendario que lleva la placa.
  // (El anillo guarda las últimas 48 noches con flor: aquí caben todas.)
  CalendarioCosecha cal;
  cal.restaurar(guardia.estado().cosecha);
  fprintf(f, "  \"semanas\": [");
  bool primera = true;
  for (time_t s0 = epochUTC(2027, 6, 28, 0, 0, 0); s0 < epochUTC(2027, 11, 1, 0, 0, 0);
       s0 += 7 * 86400) {
    const float kg = cal.kilosEntre(s0, s0 + 7 * 86400, cfg.parcela);
    fprintf(f, "%s{\"desde\":\"%s\",\"kg\":%.0f}", primera ? "" : ",",
            fechaISO(s0 + 43200).c_str(), kg);
    primera = false;
  }
  fprintf(f, "]\n}\n}\n");
  fclose(f);

  // Resumen por pantalla.
  printf("INVIERNO simulado (%d noches)\n", temporada.noches);
  printf("  noches de helada (alguna sonda < 0 °C): %d   · helada «de mapa»: %d\n",
         temporada.nochesHelada, heladasRegionales);
  printf("  noches con 1 m por debajo de 1 °C:      %d\n",
         temporada.nochesBajoDecision);
  printf("  mínima a 1 m %.1f °C · a ras de suelo %.1f °C · horas frío %.0f\n",
         temporada.minimaRef, temporada.minimaSuelo, temporada.horasFrio);
  printf("  suelo − mapa: %.2f °C de media · %.2f °C en las %d noches radiativas\n",
         sesgo / static_cast<double>(noches.size()), nRad ? sesgoRad / nRad : 0.0, nRad);
  printf("  pronóstico: error típico %.2f °C en %d noches · %d avisos previos · "
         "%d alarmas\n",
         nPron ? std::sqrt(sumaErr2 / nPron) : 0.0, nPron, avisosPrevios, alarmas);
  printf("\nNoche de muestra (%s):\n", fechaISO(noches[elegida].fin).c_str());
  for (const auto& p : partes[elegida]) {
    printf("--- %s\n%s\n", horaReloj(p.first).c_str(), p.second.c_str());
  }
  printf("\nVERANO simulado: %d noches con flor\n", nochesConFlor);
  printf("  flores en la finca: %d reales · %d estimadas · horquilla acierta %d/%d\n",
         totalVerdad, totalEstimado, dentro, nochesConFlor);
  printf("  en las centinela: %d reales · %d contadas en la foto\n", sumaReales,
         sumaVistas);
  printf("--- parte de flor (%s)\n%s\n", fechaISO(flores[mejorNoche].t).c_str(),
         parteFlor.c_str());
  printf("\nEscrito %s\n", ruta);
  return 0;
}
