// Sereno · núcleo · la guardia de invierno
//
// Cuatro termómetros en una estaca miden el «lago» de aire frío que se embalsa
// en el fondo de un vallejo en las noches despejadas y sin viento. De ahí salen:
//   · el resumen de cada noche (mínimas por altura, minutos bajo cada umbral,
//     fuerza de la inversión, cuánto rato estuvo el cielo sereno),
//   · el acumulado de la temporada (noches de helada, horas frío, veredicto),
//   · un pronóstico de la mínima que el propio aparato aprende de sus noches
//     (método empírico de la FAO: regresión sobre temperatura y punto de rocío
//     dos horas después del ocaso, sólo con noches de helada de radiación).
//
// Lógica pura: sin Arduino, sin relojes, sin E/S. Se prueba en un PC.
#pragma once
#include <cstdint>
#include <ctime>

namespace sereno {

constexpr int kNiveles = 4;
// Alturas de las sondas sobre el suelo, en metros.
constexpr float kAlturasM[kNiveles] = {0.10f, 0.50f, 1.00f, 2.00f};
constexpr int kNivelSuelo = 0;
constexpr int kNivelRef = 2;   // 1 m: la altura que pide el estudio de la finca
constexpr int kNivelAlto = 3;

// Umbrales que importan a la pitaya (guía, §5): aguanta bajadas hasta 5 °C,
// 1 °C es el umbral de decisión malla/invernadero, 0 °C es helada y por debajo
// de −2 °C mantenidos la planta muere.
constexpr int kUmbrales = 4;
constexpr float kUmbralesC[kUmbrales] = {5.0f, 1.0f, 0.0f, -2.0f};
constexpr int kUmbralDecision = 1;  // índice de 1 °C
constexpr int kUmbralHelada = 2;    // índice de 0 °C
constexpr int kUmbralLetal = 3;     // índice de −2 °C

constexpr float kHorasFrioBaseC = 7.2f;  // horas frío clásicas: por debajo de 7,2 °C

struct Muestra {
  time_t t = 0;
  float T[kNiveles] = {};  // °C por altura; NAN si la sonda falla
  float Taire = 0;         // °C, sensor de precisión a 1 m (SHT4x); NAN si falla
  float HR = 0;            // % humedad relativa a 1 m; NAN si falla
  float Tcielo = 0;        // °C, temperatura infrarroja del cénit; NAN si falla
};

// Punto de rocío (Magnus) y temperatura de bulbo húmedo (Stull, 2011), en °C.
float puntoRocio(float T, float HR);
float bulboHumedo(float T, float HR);

enum class Cielo : uint8_t { Desconocido, Sereno, Velado, Nublado };

// Un cielo despejado «se ve» mucho más frío que el aire en infrarrojo; uno
// cubierto, casi a la misma temperatura. Los valores son un punto de partida:
// dependen de la humedad y de la ventana del sensor y se afinan con un par de
// noches de observación.
struct UmbralesCielo {
  float sereno = -18.0f;   // Tcielo − Taire por debajo de esto: despejado
  float nublado = -8.0f;   // por encima de esto: cubierto
};
Cielo clasificarCielo(float Tcielo, float Taire, const UmbralesCielo& u = {});

struct ResumenNoche {
  int muestras = 0;
  float minima[kNiveles];             // NAN si no hubo lectura válida
  time_t horaMinima[kNiveles];
  float minutosBajo[kNiveles][kUmbrales];
  float inversionMedia = 0;           // media de T(2 m) − T(0,1 m), °C
  float inversionMax = 0;
  float fraccionSerena = 0;           // 0..1 sobre las muestras con cielo conocido
  int muestrasCielo = 0;
  float rocioMinimo = 0;              // NAN si no hubo humedad

  // Mínima más baja entre todas las alturas y la altura donde ocurrió.
  float minimaAbsoluta(int* nivel = nullptr) const;
  bool helo() const;                  // alguna altura bajó de 0 °C
  // Noche de enfriamiento por radiación: cielo mayormente sereno y aire frío
  // embalsado (la inversión hace de testigo de la calma, sin anemómetro).
  bool radiativa() const;
};

class Noche {
 public:
  // Estado completo de la noche en curso: copiable byte a byte, para que
  // sobreviva al sueño profundo en la memoria del reloj.
  struct Estado {
    ResumenNoche r;
    double sumaInversion = 0;
    int32_t nInversion = 0;
    int32_t nSerenas = 0;
  };

  Noche() { reiniciar(); }
  void reiniciar();
  // `minutos`: lo que representa cada muestra (el periodo de muestreo).
  void anadir(const Muestra& m, float minutos, const UmbralesCielo& u = {});
  const ResumenNoche& resumen() const { return e_.r; }

  const Estado& estado() const { return e_; }
  void restaurar(const Estado& e) { e_ = e; }

 private:
  Estado e_;
};

enum class Veredicto : uint8_t { SinDatos, MallaPosible, InvernaderoObligatorio };

// Acumulado de la temporada. Es un POD para poder guardarlo tal cual en flash.
struct Temporada {
  int32_t noches = 0;
  int32_t nochesHelada = 0;        // alguna altura por debajo de 0 °C
  int32_t nochesBajoDecision = 0;  // 1 m por debajo de 1 °C
  int32_t nochesRadiativas = 0;
  float minimaRef = 1000.0f;       // mínima absoluta a 1 m
  int64_t fechaMinimaRef = 0;
  float minimaSuelo = 1000.0f;     // mínima absoluta a ras de suelo
  int64_t fechaMinimaSuelo = 0;
  float horasFrio = 0;             // horas con el aire a 1 m por debajo de 7,2 °C

  void anadirMuestra(const Muestra& m, float minutos);
  void cerrarNoche(const ResumenNoche& r);
  // Regla de la guía de la pitaya: si a 1 m aparecen mínimas por debajo de
  // 1 °C, el invernadero deja de ser opción y pasa a ser obligatorio.
  Veredicto veredicto() const;
};

// Pronóstico empírico de la mínima: Tmin = b0 + b1·T0 + b2·Td0, con T0 y Td0
// medidos dos horas después del ocaso. Se entrena noche a noche con mínimos
// cuadrados acumulados; el estado ocupa 80 bytes y sobrevive al sueño profundo.
class PronosticoMinima {
 public:
  static constexpr int kNochesMinimas = 8;

  struct Estado {
    uint32_t n = 0;
    // Sumas: T, Td, y, T², Td², T·Td, T·y, Td·y, y²
    double s[9] = {};
  };

  void entrenar(float T0, float Td0, float Tmin);
  // Devuelve false si aún no hay noches suficientes o el ajuste no es fiable.
  bool predecir(float T0, float Td0, float* Tmin, float* errorTipico) const;
  bool coeficientes(double b[3]) const;
  uint32_t noches() const { return e_.n; }

  const Estado& estado() const { return e_; }
  void restaurar(const Estado& e) { e_ = e; }

 private:
  Estado e_;
};

// Tendencia de la temperatura durante la noche (ley de la raíz cuadrada del
// tiempo): a `horas` de T0, camino de una mínima `Tmin` al cabo de `horasNoche`.
float tendenciaNocturna(float T0, float Tmin, float horas, float horasNoche);

}  // namespace sereno
