// Sereno · núcleo · el cante
//
// El sereno recorría las calles de noche y cantaba la hora y el tiempo:
// «¡Las doce y sereno!». Este hace lo mismo con la finca: convierte los datos
// de la noche en un parte corto, en castellano, que cabe en un mensaje.
//
// Las horas se escriben en la hora local del proceso (variable TZ).
#pragma once
#include <ctime>
#include <string>

#include "flor.h"
#include "helada.h"

namespace sereno {

// Zona horaria peninsular en formato POSIX, para setenv("TZ", ...).
constexpr const char* kZonaPeninsular = "CET-1CEST,M3.5.0,M10.5.0/3";

std::string num(float v, int decimales);       // 1 decimal → «−0,8»
std::string miles(long v);                     // «1.600»
std::string horaCantada(int hora, int minuto); // «Las ocho menos cuarto»
std::string horaReloj(time_t t);               // «06:50»
std::string fechaCorta(time_t t);              // «18 de agosto»
std::string duracion(float horas);             // «1 h 45 min»
int redondoHablado(int v);                     // 563 → 560
const char* nombreNivel(int nivel);            // «a ras de suelo», «a 1 m»…

// Parte del amanecer: lo que pasó esta noche y cómo va la temporada.
std::string canteAmanecer(time_t ahora, const ResumenNoche& r,
                          const Temporada& t);

// Aviso de primera hora de la noche: mínima prevista para la madrugada.
std::string cantePronostico(time_t ahora, float TminRef, float errorTipico,
                            time_t orto);

// Alarma inmediata: una sonda acaba de cruzar el cero.
std::string canteHelando(time_t ahora, float T, int nivel);

// Parte de la noche de flor.
std::string canteFlor(time_t ahora, const EstimacionFlor& e,
                      const ParcelaFlor& p);

}  // namespace sereno
