#include "sol.h"

#include <cmath>

namespace sereno {

namespace {
const double kPi = 3.14159265358979323846;
inline double rad(double grados) { return grados * kPi / 180.0; }
inline double mod360(double x) {
  double r = std::fmod(x, 360.0);
  return r < 0 ? r + 360.0 : r;
}
}  // namespace

long diaJuliano(int anio, int mes, int dia) {
  // Fliegel y Van Flandern (1968).
  long a = (14 - mes) / 12;
  long y = anio + 4800 - a;
  long m = mes + 12 * a - 3;
  return dia + (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 32045;
}

time_t epochUTC(int anio, int mes, int dia, int hora, int minuto, int segundo) {
  const long dias = diaJuliano(anio, mes, dia) - 2440588L;  // 1-1-1970
  return static_cast<time_t>(dias) * 86400 + hora * 3600 + minuto * 60 + segundo;
}

SolDia solDelDia(time_t t, double latitud, double longitud) {
  struct tm g;
  gmtime_r(&t, &g);
  const long jdn = diaJuliano(g.tm_year + 1900, g.tm_mon + 1, g.tm_mday);

  // «Ecuación del orto» (forma estándar de la NOAA simplificada).
  const double n = static_cast<double>(jdn - 2451545L) + 0.0008;
  const double jEstrella = n - longitud / 360.0;
  const double M = mod360(357.5291 + 0.98560028 * jEstrella);
  const double C = 1.9148 * std::sin(rad(M)) + 0.0200 * std::sin(rad(2 * M)) +
                   0.0003 * std::sin(rad(3 * M));
  const double lambda = mod360(M + C + 180.0 + 102.9372);
  const double jTransito = 2451545.0 + jEstrella + 0.0053 * std::sin(rad(M)) -
                           0.0069 * std::sin(rad(2 * lambda));
  const double senDec = std::sin(rad(lambda)) * std::sin(rad(23.4397));
  const double cosDec = std::cos(std::asin(senDec));
  const double cosOmega =
      (std::sin(rad(-0.833)) - std::sin(rad(latitud)) * senDec) /
      (std::cos(rad(latitud)) * cosDec);
  if (cosOmega < -1.0 || cosOmega > 1.0) return {0, 0, false};

  const double omega = std::acos(cosOmega) * 180.0 / kPi;
  auto aEpoch = [](double j) {
    return static_cast<time_t>(std::llround((j - 2440587.5) * 86400.0));
  };
  return {aEpoch(jTransito - omega / 360.0), aEpoch(jTransito + omega / 360.0),
          true};
}

}  // namespace sereno
