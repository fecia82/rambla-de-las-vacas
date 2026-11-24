#include "flor.h"

#include <cmath>

namespace sereno {

void blancuraRGB888(const uint8_t* rgb, int ancho, int alto, uint8_t* salida) {
  const int n = ancho * alto;
  for (int i = 0; i < n; ++i) {
    const uint8_t r = rgb[3 * i], g = rgb[3 * i + 1], b = rgb[3 * i + 2];
    uint8_t m = r < g ? r : g;
    salida[i] = m < b ? m : b;
  }
}

void reducirMitad(const uint8_t* entrada, int ancho, int alto, uint8_t* salida) {
  const int w2 = ancho / 2, h2 = alto / 2;
  for (int y = 0; y < h2; ++y) {
    const uint8_t* f0 = entrada + (2 * y) * ancho;
    const uint8_t* f1 = f0 + ancho;
    for (int x = 0; x < w2; ++x) {
      const int s = f0[2 * x] + f0[2 * x + 1] + f1[2 * x] + f1[2 * x + 1];
      salida[y * w2 + x] = static_cast<uint8_t>((s + 2) / 4);
    }
  }
}

int umbralOtsu(const uint8_t* img, int n) {
  uint32_t hist[256] = {};
  for (int i = 0; i < n; ++i) ++hist[img[i]];
  double total = 0;
  for (int v = 0; v < 256; ++v) total += static_cast<double>(v) * hist[v];

  double sumaFondo = 0, pesoFondo = 0, mejor = -1;
  int umbral = 0;
  for (int v = 0; v < 256; ++v) {
    pesoFondo += hist[v];
    if (pesoFondo == 0) continue;
    const double pesoFrente = n - pesoFondo;
    if (pesoFrente == 0) break;
    sumaFondo += static_cast<double>(v) * hist[v];
    const double mediaFondo = sumaFondo / pesoFondo;
    const double mediaFrente = (total - sumaFondo) / pesoFrente;
    const double entre = pesoFondo * pesoFrente * (mediaFondo - mediaFrente) *
                         (mediaFondo - mediaFrente);
    if (entre > mejor) {
      mejor = entre;
      umbral = v;
    }
  }
  return umbral + 1;  // «claro» es estrictamente por encima del corte
}

void actualizarFondo(uint8_t* fondo, const uint8_t* img, int n, int subida) {
  for (int i = 0; i < n; ++i) {
    if (img[i] <= fondo[i]) {
      fondo[i] = img[i];
    } else {
      const int v = fondo[i] + subida;
      fondo[i] = static_cast<uint8_t>(v < img[i] ? v : img[i]);
    }
  }
}

void restarFondo(const uint8_t* img, const uint8_t* fondo, int n, uint8_t* salida) {
  for (int i = 0; i < n; ++i) {
    salida[i] = img[i] > fondo[i] ? static_cast<uint8_t>(img[i] - fondo[i]) : 0;
  }
}

namespace {

// Máximo (dilatación) o mínimo (erosión) en una ventana cuadrada de radio r,
// separable: primero por filas y luego por columnas.
void morfologia(std::vector<uint8_t>& m, int w, int h, int r, bool dilatar,
                std::vector<uint8_t>& tmp) {
  if (r <= 0) return;
  tmp.assign(m.size(), 0);
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      uint8_t v = dilatar ? 0 : 1;
      for (int k = -r; k <= r; ++k) {
        const int xx = x + k;
        // Fuera de la imagen no hay nada: fondo.
        const uint8_t u = (xx < 0 || xx >= w) ? 0 : m[y * w + xx];
        v = dilatar ? (u > v ? u : v) : (u < v ? u : v);
      }
      tmp[y * w + x] = v;
    }
  }
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      uint8_t v = dilatar ? 0 : 1;
      for (int k = -r; k <= r; ++k) {
        const int yy = y + k;
        const uint8_t u = (yy < 0 || yy >= h) ? 0 : tmp[yy * w + x];
        v = dilatar ? (u > v ? u : v) : (u < v ? u : v);
      }
      m[y * w + x] = v;
    }
  }
}

// Transformada de distancia de chaflán 3-4: para cada píxel de la máscara, el
// triple de su distancia aproximada al fondo más cercano.
void distanciaChaflan(const std::vector<uint8_t>& m, int w, int h,
                      std::vector<uint16_t>& d) {
  const uint16_t lejos = 60000;
  d.assign(m.size(), 0);
  auto en = [&](int x, int y) -> uint16_t {
    return (x < 0 || y < 0 || x >= w || y >= h) ? 0 : d[y * w + x];
  };
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      if (!m[y * w + x]) continue;
      uint32_t v = lejos;
      const uint32_t a = en(x - 1, y) + 3u, b = en(x, y - 1) + 3u;
      const uint32_t c = en(x - 1, y - 1) + 4u, e = en(x + 1, y - 1) + 4u;
      if (a < v) v = a;
      if (b < v) v = b;
      if (c < v) v = c;
      if (e < v) v = e;
      d[y * w + x] = static_cast<uint16_t>(v);
    }
  }
  for (int y = h - 1; y >= 0; --y) {
    for (int x = w - 1; x >= 0; --x) {
      if (!m[y * w + x]) continue;
      uint32_t v = d[y * w + x];
      const uint32_t a = en(x + 1, y) + 3u, b = en(x, y + 1) + 3u;
      const uint32_t c = en(x + 1, y + 1) + 4u, e = en(x - 1, y + 1) + 4u;
      if (a < v) v = a;
      if (b < v) v = b;
      if (c < v) v = c;
      if (e < v) v = e;
      d[y * w + x] = static_cast<uint16_t>(v);
    }
  }
}

}  // namespace

int contarFlores(const uint8_t* img, int ancho, int alto, const ParamsFlor& p,
                 std::vector<Mancha>* manchas) {
  const int n = ancho * alto;
  int umbral = p.umbral;
  if (umbral < 0) {
    umbral = umbralOtsu(img, n);
    if (umbral < p.umbralMinimo) umbral = p.umbralMinimo;
  }
  std::vector<uint8_t> mascara(n), tmp;
  for (int i = 0; i < n; ++i) mascara[i] = img[i] >= umbral ? 1 : 0;

  // Cierre: dilatar y erosionar cose los cortes de un par de píxeles.
  morfologia(mascara, ancho, alto, p.cierre, true, tmp);
  morfologia(mascara, ancho, alto, p.cierre, false, tmp);

  std::vector<uint16_t> dist;
  distanciaChaflan(mascara, ancho, alto, dist);

  if (manchas) manchas->clear();
  std::vector<int32_t> pixeles;  // la mancha en curso (hace también de pila)
  int total = 0;
  for (int semilla = 0; semilla < n; ++semilla) {
    if (mascara[semilla] != 1) continue;

    // Relleno por inundación, vecindad de 8.
    pixeles.clear();
    pixeles.push_back(semilla);
    mascara[semilla] = 2;
    int x0 = ancho, y0 = alto, x1 = -1, y1 = -1;
    double sx = 0, sy = 0;
    for (size_t k = 0; k < pixeles.size(); ++k) {
      const int i = pixeles[k];
      const int x = i % ancho, y = i / ancho;
      sx += x;
      sy += y;
      if (x < x0) x0 = x;
      if (x > x1) x1 = x;
      if (y < y0) y0 = y;
      if (y > y1) y1 = y;
      for (int dy = -1; dy <= 1; ++dy) {
        const int yy = y + dy;
        if (yy < 0 || yy >= alto) continue;
        for (int dx = -1; dx <= 1; ++dx) {
          const int xx = x + dx;
          if (xx < 0 || xx >= ancho) continue;
          const int j = yy * ancho + xx;
          if (mascara[j] == 1) {
            mascara[j] = 2;
            pixeles.push_back(j);
          }
        }
      }
    }

    const int area = static_cast<int>(pixeles.size());
    if (area < p.areaMin || area > p.areaMax) continue;
    const int bw = x1 - x0 + 1, bh = y1 - y0 + 1;
    const float relleno =
        static_cast<float>(area) / static_cast<float>(bw * bh);
    if (relleno < p.rellenoMin) continue;
    const float largo = static_cast<float>(bw > bh ? bw : bh);
    const float corto = static_cast<float>(bw > bh ? bh : bw);
    if (largo / corto > p.alargamientoMax) continue;

    // ¿Cuántas flores hay en la mancha? Se busca el mayor disco inscrito, se
    // tacha su entorno y se repite mientras quepa otro de tamaño parecido.
    int flores = 0;
    float radioPrimero = 0;
    while (flores < p.floresPorManchaMax) {
      int mejor = -1;
      uint16_t dMejor = 0;
      for (int i : pixeles) {
        if (mascara[i] == 2 && dist[i] > dMejor) {
          dMejor = dist[i];
          mejor = i;
        }
      }
      if (mejor < 0) break;
      const float radio = static_cast<float>(dMejor) / 3.0f;
      if (flores == 0) {
        radioPrimero = radio;
      } else if (radio < p.radioRelativoMin * radioPrimero) {
        break;
      }
      ++flores;
      const int mx = mejor % ancho, my = mejor / ancho;
      const float alcance = p.separacion * radio + 1.0f;
      const float alcance2 = alcance * alcance;
      for (int i : pixeles) {
        if (mascara[i] != 2) continue;
        const float dx = static_cast<float>(i % ancho - mx);
        const float dy = static_cast<float>(i / ancho - my);
        if (dx * dx + dy * dy <= alcance2) mascara[i] = 3;
      }
    }
    if (flores < 1) flores = 1;

    total += flores;
    if (manchas) {
      manchas->push_back({area, x0, y0, x1, y1, static_cast<float>(sx / area),
                          static_cast<float>(sy / area), flores});
    }
  }
  return total;
}

// ------------------------------------------------------- de flores a kilos

void intervaloPoisson(int cuenta, double z, double* inferior, double* superior) {
  // Byar: muy cerca del intervalo exacto incluso con conteos pequeños.
  if (inferior) {
    if (cuenta <= 0) {
      *inferior = 0;
    } else {
      const double c = cuenta;
      const double f = 1.0 - 1.0 / (9.0 * c) - z / (3.0 * std::sqrt(c));
      *inferior = f > 0 ? c * f * f * f : 0;
    }
  }
  if (superior) {
    const double c = cuenta + 1.0;
    const double f = 1.0 - 1.0 / (9.0 * c) + z / (3.0 * std::sqrt(c));
    *superior = c * f * f * f;
  }
}

EstimacionFlor estimarFinca(int floresVistas, time_t noche,
                            const ParcelaFlor& p) {
  EstimacionFlor e;
  if (floresVistas < 0) floresVistas = 0;
  e.vistas = floresVistas;
  if (p.plantasCentinela <= 0) return e;

  const double escala =
      static_cast<double>(p.plantasTotales) / p.plantasCentinela;
  double lo, hi;
  intervaloPoisson(floresVistas, 1.2816, &lo, &hi);
  e.finca = static_cast<int>(std::lround(floresVistas * escala));
  e.fincaMin = static_cast<int>(std::lround(lo * escala));
  e.fincaMax = static_cast<int>(std::lround(hi * escala));
  const float finca = static_cast<float>(e.finca);
  e.horasPincel = p.floresPorHora > 0 ? finca / p.floresPorHora : 0;
  e.kilos = finca * p.cuajado * p.gramosFruto / 1000.0f;
  e.cosechaDesde = noche + static_cast<time_t>(p.diasACosechaMin) * 86400;
  e.cosechaHasta = noche + static_cast<time_t>(p.diasACosechaMax) * 86400;
  return e;
}

void CalendarioCosecha::anotar(time_t noche, int floresFinca) {
  if (floresFinca <= 0) return;
  e_.c[e_.siguiente] = {static_cast<int64_t>(noche), floresFinca};
  e_.siguiente = (e_.siguiente + 1) % kCohortes;
}

float CalendarioCosecha::kilosEntre(time_t desde, time_t hasta,
                                    const ParcelaFlor& p) const {
  if (hasta <= desde) return 0;
  double kilos = 0;
  for (int i = 0; i < kCohortes; ++i) {
    const Cohorte& c = e_.c[i];
    if (c.floresFinca <= 0) continue;
    const double ini = static_cast<double>(c.noche) + p.diasACosechaMin * 86400.0;
    const double fin =
        static_cast<double>(c.noche) + (p.diasACosechaMax + 1) * 86400.0;
    const double d = static_cast<double>(desde), h = static_cast<double>(hasta);
    const double a = ini > d ? ini : d;
    const double b = fin < h ? fin : h;
    if (b <= a) continue;
    const double fraccion = (b - a) / (fin - ini);
    kilos += fraccion * c.floresFinca * p.cuajado * p.gramosFruto / 1000.0;
  }
  return static_cast<float>(kilos);
}

}  // namespace sereno
