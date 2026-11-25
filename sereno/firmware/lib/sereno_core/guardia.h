// Sereno · núcleo · la guardia
//
// La máquina de estados de la noche, separada del hardware para poder probarla
// y simular temporadas enteras en un PC. Recibe una muestra cada diez minutos y
// devuelve qué hay que hacer: mandar un parte, hacer una foto, guardar el estado.
//
//   ocaso ───── +2 h ─────────── madrugada ─────────── orto ── +30 min
//     │          │                   │                           │
//  empieza    toma T0 y Td0      foto de flores            cierra la noche:
//  la noche   y pronostica       (de mayo a octubre)       resumen, aprende
//             la mínima                                    y canta el parte
//
// En cualquier momento de la noche: si una sonda cruza el cero, alarma.
#pragma once
#include <cstdint>
#include <ctime>
#include <string>
#include <vector>

#include "flor.h"
#include "helada.h"

namespace sereno {

struct Config {
  double latitud = 0;
  double longitud = 0;            // positiva al este
  float minutosMuestra = 10;
  float umbralAvisoC = 1.0f;      // avisa si la mínima prevista a 1 m baja de aquí
  int mesInicioFrio = 10;         // horas frío y parte diario: de octubre…
  int mesFinFrio = 3;             // …a marzo
  int mesInicioFlor = 5;          // foto nocturna: de mayo…
  int mesFinFlor = 10;            // …a octubre
  int horaFotoLocal = 1;          // la una de la madrugada
  int margenOrtoMin = 30;         // la noche se cierra media hora tras el orto
  UmbralesCielo cielo;
  ParcelaFlor parcela;
};

// Todo lo que tiene que sobrevivir entre un despertar y el siguiente. Es
// copiable byte a byte: vive en la memoria del reloj y, una vez al día, en flash.
struct Persistente {
  uint32_t magia = 0;
  int32_t anioTemporada = 0;
  Noche::Estado noche;
  Temporada temporada;
  PronosticoMinima::Estado pronostico;
  CalendarioCosecha::Estado cosecha;
  int64_t inicioNoche = 0;  // ocaso de la noche en curso
  int64_t finNoche = 0;     // orto + margen
  float T0 = 0;             // temperatura y rocío a las dos horas del ocaso
  float Td0 = 0;
  uint8_t enNoche = 0;
  uint8_t t0Tomado = 0;
  uint8_t avisoHelada = 0;
  uint8_t fotoHecha = 0;
};

constexpr uint32_t kMagia = 0x5E4E0001;  // «SEreNO», versión 1 del formato

struct Salida {
  std::vector<std::string> mensajes;  // partes listos para enviar
  bool urgente = false;     // enviar ahora, no esperar al amanecer
  bool pedirFoto = false;   // hacer la foto de flores y llamar a flores()
  bool cierreNoche = false; // buen momento para guardar el estado en flash
};

// Devuelve true si `t` cae dentro de una noche y da sus límites: del ocaso al
// orto siguiente más el margen.
bool nocheQueContiene(time_t t, double latitud, double longitud, int margenMin,
                      time_t* inicio, time_t* fin);

// ¿Está el mes (1–12) dentro del intervalo, que puede cruzar el fin de año?
bool mesEntre(int mes, int desde, int hasta);

class Guardia {
 public:
  explicit Guardia(const Config& c) : c_(c) { p_.magia = kMagia; }

  // Una muestra por periodo, con la hora ya fijada.
  Salida muestra(const Muestra& m);

  // Resultado de la foto pedida: flores contadas en las plantas centinela.
  // Devuelve el parte (vacío si no hay nada que contar a nadie).
  std::string flores(time_t ahora, int vistas);

  const Persistente& estado() const { return p_; }
  // Devuelve false (y no toca nada) si el bloque no es de este formato.
  bool restaurar(const Persistente& p);
  const Config& config() const { return c_; }

 private:
  void empezarNoche(time_t inicio, time_t fin);
  void cerrarNoche(time_t ahora, Salida* s);

  Config c_;
  Persistente p_;
};

}  // namespace sereno
