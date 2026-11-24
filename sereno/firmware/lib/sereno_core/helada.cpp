#include "helada.h"

#include <cmath>

namespace sereno {

float puntoRocio(float T, float HR) {
  if (std::isnan(T) || std::isnan(HR) || HR <= 0.0f) return NAN;
  const double a = 17.62, b = 243.12;  // Magnus sobre agua
  const double g = std::log(HR / 100.0) + a * T / (b + T);
  return static_cast<float>(b * g / (a - g));
}

float bulboHumedo(float T, float HR) {
  if (std::isnan(T) || std::isnan(HR)) return NAN;
  // Stull (2011), válido aprox. entre 5 % y 99 % de HR y de −20 a 50 °C.
  const double tw = T * std::atan(0.151977 * std::sqrt(HR + 8.313659)) +
                    std::atan(T + HR) - std::atan(HR - 1.676331) +
                    0.00391838 * std::pow(HR, 1.5) * std::atan(0.023101 * HR) -
                    4.686035;
  return static_cast<float>(tw);
}

Cielo clasificarCielo(float Tcielo, float Taire, const UmbralesCielo& u) {
  if (std::isnan(Tcielo) || std::isnan(Taire)) return Cielo::Desconocido;
  const float d = Tcielo - Taire;
  if (d <= u.sereno) return Cielo::Sereno;
  if (d >= u.nublado) return Cielo::Nublado;
  return Cielo::Velado;
}

// ---------------------------------------------------------------- la noche

float ResumenNoche::minimaAbsoluta(int* nivel) const {
  float m = NAN;
  int cual = -1;
  for (int i = 0; i < kNiveles; ++i) {
    if (std::isnan(minima[i])) continue;
    if (cual < 0 || minima[i] < m) {
      m = minima[i];
      cual = i;
    }
  }
  if (nivel) *nivel = cual;
  return m;
}

bool ResumenNoche::helo() const {
  const float m = minimaAbsoluta();
  return !std::isnan(m) && m < 0.0f;
}

bool ResumenNoche::radiativa() const {
  return muestrasCielo > 0 && fraccionSerena >= 0.7f && inversionMedia >= 0.5f;
}

void Noche::reiniciar() {
  e_ = Estado();
  ResumenNoche& r_ = e_.r;
  for (int i = 0; i < kNiveles; ++i) {
    r_.minima[i] = NAN;
    r_.horaMinima[i] = 0;
    for (int j = 0; j < kUmbrales; ++j) r_.minutosBajo[i][j] = 0;
  }
  r_.rocioMinimo = NAN;
}

void Noche::anadir(const Muestra& m, float minutos, const UmbralesCielo& u) {
  ResumenNoche& r_ = e_.r;
  ++r_.muestras;
  for (int i = 0; i < kNiveles; ++i) {
    const float t = m.T[i];
    if (std::isnan(t)) continue;
    if (std::isnan(r_.minima[i]) || t < r_.minima[i]) {
      r_.minima[i] = t;
      r_.horaMinima[i] = m.t;
    }
    for (int j = 0; j < kUmbrales; ++j) {
      if (t < kUmbralesC[j]) r_.minutosBajo[i][j] += minutos;
    }
  }

  if (!std::isnan(m.T[kNivelAlto]) && !std::isnan(m.T[kNivelSuelo])) {
    const float inv = m.T[kNivelAlto] - m.T[kNivelSuelo];
    e_.sumaInversion += inv;
    ++e_.nInversion;
    r_.inversionMedia = static_cast<float>(e_.sumaInversion / e_.nInversion);
    if (e_.nInversion == 1 || inv > r_.inversionMax) r_.inversionMax = inv;
  }

  const float aire = std::isnan(m.Taire) ? m.T[kNivelRef] : m.Taire;
  const Cielo c = clasificarCielo(m.Tcielo, aire, u);
  if (c != Cielo::Desconocido) {
    ++r_.muestrasCielo;
    if (c == Cielo::Sereno) ++e_.nSerenas;
    r_.fraccionSerena = static_cast<float>(e_.nSerenas) /
                        static_cast<float>(r_.muestrasCielo);
  }

  const float td = puntoRocio(aire, m.HR);
  if (!std::isnan(td) && (std::isnan(r_.rocioMinimo) || td < r_.rocioMinimo)) {
    r_.rocioMinimo = td;
  }
}

// ------------------------------------------------------------ la temporada

void Temporada::anadirMuestra(const Muestra& m, float minutos) {
  const float aire = std::isnan(m.Taire) ? m.T[kNivelRef] : m.Taire;
  if (!std::isnan(aire) && aire < kHorasFrioBaseC) horasFrio += minutos / 60.0f;
}

void Temporada::cerrarNoche(const ResumenNoche& r) {
  if (r.muestras == 0) return;
  ++noches;
  if (r.helo()) ++nochesHelada;
  if (r.radiativa()) ++nochesRadiativas;

  const float ref = r.minima[kNivelRef];
  if (!std::isnan(ref)) {
    if (ref < kUmbralesC[kUmbralDecision]) ++nochesBajoDecision;
    if (ref < minimaRef) {
      minimaRef = ref;
      fechaMinimaRef = static_cast<int64_t>(r.horaMinima[kNivelRef]);
    }
  }
  const float suelo = r.minima[kNivelSuelo];
  if (!std::isnan(suelo) && suelo < minimaSuelo) {
    minimaSuelo = suelo;
    fechaMinimaSuelo = static_cast<int64_t>(r.horaMinima[kNivelSuelo]);
  }
}

Veredicto Temporada::veredicto() const {
  if (noches == 0 || minimaRef > 999.0f) return Veredicto::SinDatos;
  return minimaRef < kUmbralesC[kUmbralDecision]
             ? Veredicto::InvernaderoObligatorio
             : Veredicto::MallaPosible;
}

// ------------------------------------------------------------ el pronóstico

void PronosticoMinima::entrenar(float T0, float Td0, float Tmin) {
  if (std::isnan(T0) || std::isnan(Td0) || std::isnan(Tmin)) return;
  const double t = T0, d = Td0, y = Tmin;
  ++e_.n;
  e_.s[0] += t;
  e_.s[1] += d;
  e_.s[2] += y;
  e_.s[3] += t * t;
  e_.s[4] += d * d;
  e_.s[5] += t * d;
  e_.s[6] += t * y;
  e_.s[7] += d * y;
  e_.s[8] += y * y;
}

bool PronosticoMinima::coeficientes(double b[3]) const {
  if (e_.n < 3) return false;
  const double n = e_.n;
  // Una pizca de regularización (0,001 °C² por noche) evita que dos predictores
  // casi colineales —pasa en inviernos monótonos— disparen los coeficientes.
  const double cresta = 1e-3 * n;
  double a[3][4] = {
      {n, e_.s[0], e_.s[1], e_.s[2]},
      {e_.s[0], e_.s[3] + cresta, e_.s[5], e_.s[6]},
      {e_.s[1], e_.s[5], e_.s[4] + cresta, e_.s[7]},
  };
  for (int col = 0; col < 3; ++col) {
    int piv = col;
    for (int f = col + 1; f < 3; ++f) {
      if (std::fabs(a[f][col]) > std::fabs(a[piv][col])) piv = f;
    }
    if (std::fabs(a[piv][col]) < 1e-9) return false;
    if (piv != col) {
      for (int k = 0; k < 4; ++k) {
        const double tmp = a[col][k];
        a[col][k] = a[piv][k];
        a[piv][k] = tmp;
      }
    }
    for (int f = 0; f < 3; ++f) {
      if (f == col) continue;
      const double factor = a[f][col] / a[col][col];
      for (int k = col; k < 4; ++k) a[f][k] -= factor * a[col][k];
    }
  }
  for (int i = 0; i < 3; ++i) b[i] = a[i][3] / a[i][i];
  return true;
}

bool PronosticoMinima::predecir(float T0, float Td0, float* Tmin,
                                float* errorTipico) const {
  if (e_.n < static_cast<uint32_t>(kNochesMinimas)) return false;
  if (std::isnan(T0) || std::isnan(Td0)) return false;
  double b[3];
  if (!coeficientes(b)) return false;
  if (Tmin) *Tmin = static_cast<float>(b[0] + b[1] * T0 + b[2] * Td0);
  if (errorTipico) {
    double sse = e_.s[8] - b[0] * e_.s[2] - b[1] * e_.s[6] - b[2] * e_.s[7];
    if (sse < 0) sse = 0;
    const double rmse = std::sqrt(sse / (e_.n - 3));
    // Nunca prometer más de lo que da el termómetro.
    *errorTipico = static_cast<float>(rmse < 0.3 ? 0.3 : rmse);
  }
  return true;
}

float tendenciaNocturna(float T0, float Tmin, float horas, float horasNoche) {
  if (horasNoche <= 0) return Tmin;
  float f = horas / horasNoche;
  if (f < 0) f = 0;
  if (f > 1) f = 1;
  return T0 + (Tmin - T0) * std::sqrt(f);
}

}  // namespace sereno
