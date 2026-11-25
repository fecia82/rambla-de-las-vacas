// Sereno · núcleo · orto y ocaso
// Lógica pura, sin dependencias de hardware: se compila igual en el ESP32 y en un PC.
#pragma once
#include <ctime>

namespace sereno {

struct SolDia {
  time_t orto;   // salida del sol, epoch UTC
  time_t ocaso;  // puesta del sol, epoch UTC
  bool valido;   // false en día o noche polar
};

// Día juliano (entero, a mediodía UT) de una fecha gregoriana.
long diaJuliano(int anio, int mes, int dia);

// Epoch de una fecha y hora UTC (el `timegm` que no trae la libc del ESP32).
time_t epochUTC(int anio, int mes, int dia, int hora, int minuto, int segundo);

// Orto y ocaso del día civil UTC que contiene `t`.
// Longitud positiva al este. Precisión de un par de minutos, de sobra para
// decidir cuándo empieza la noche y cuándo son «dos horas tras el ocaso».
SolDia solDelDia(time_t t, double latitud, double longitud);

}  // namespace sereno
