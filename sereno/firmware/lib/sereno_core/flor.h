// Sereno · núcleo · la guardia de verano
//
// La flor de la pitaya abre una sola noche. Es blanca y mide un palmo: con el
// destello del farol, en una foto nocturna es una mancha clara sobre fondo
// negro. Contarlas no necesita redes neuronales ni nube:
//
//   1. Blancura: el mínimo de R, G y B (un pétalo blanco tiene los tres altos).
//   2. Resta de fondo: postes, alambres y gomas de riego brillan todas las
//      noches; una flor, sólo esta. Lo que anoche no estaba es candidato a flor.
//   3. Umbral, manchas conexas y filtro de tamaño y forma.
//   4. Flores pegadas: en cada mancha se cuentan los discos máximos que caben
//      sin pisarse (transformada de distancia), no «una mancha, una flor».
//
// De ese conteo en las plantas centinela sale la estimación para toda la finca
// (con su horquilla, porque es una muestra), las horas de pincel de la mañana
// siguiente y los kilos que habrá un mes después.
//
// Lógica pura: sin cámara, sin Arduino. Está probada con escenas sintéticas;
// con flores de verdad hay que afinar umbrales la primera temporada, y por eso
// el aparato manda siempre la foto junto al número.
#pragma once
#include <cstdint>
#include <ctime>
#include <vector>

namespace sereno {

struct ParamsFlor {
  int umbral = -1;             // blancura mínima 0..255; −1 = automático (Otsu)
  int umbralMinimo = 110;      // suelo del umbral automático: de noche, lo que
                               // no brilla de verdad no es una flor
  int cierre = 1;              // px: cose los cortes finos (un tallo por delante)
  int areaMin = 40;            // px², a la resolución de trabajo
  int areaMax = 12000;
  float rellenoMin = 0.20f;    // área / área de su caja: descarta hilos y ramas
  float alargamientoMax = 4.0f;  // lado largo / lado corto: descarta postes
  float separacion = 1.5f;     // dos flores distan al menos 1,5 radios
  float radioRelativoMin = 0.6f;  // una flor pegada a otra mide ≥ 60 % de su radio
  int floresPorManchaMax = 12;
};

struct Mancha {
  int area;
  int x0, y0, x1, y1;  // caja envolvente, inclusive
  float cx, cy;        // centro de masas
  int flores;          // cuántas flores se cuentan en esta mancha (≥ 1)
};

// Blancura de cada píxel de una imagen RGB888: el mínimo de los tres canales.
// Un pétalo blanco tiene los tres altos; una hoja verde o un poste oxidado, no.
void blancuraRGB888(const uint8_t* rgb, int ancho, int alto, uint8_t* salida);

// Reduce a la mitad promediando bloques 2×2 (ancho y alto pares).
void reducirMitad(const uint8_t* entrada, int ancho, int alto, uint8_t* salida);

// Umbral de Otsu sobre una imagen de 8 bits.
int umbralOtsu(const uint8_t* img, int n);

// Fondo: lo que brilla noche tras noche. Sube despacio (como mucho `subida`
// niveles por noche) y baja de golpe, así que una flor de una sola noche no
// deja rastro y un poste tarda unas cuantas noches en «aprenderse».
void actualizarFondo(uint8_t* fondo, const uint8_t* img, int n, int subida = 40);
// salida = max(0, img − fondo). `salida` puede ser el mismo búfer que `img`.
void restarFondo(const uint8_t* img, const uint8_t* fondo, int n, uint8_t* salida);

// Cuenta las flores de una imagen de blancura (ya con el fondo restado, si lo
// hay). Devuelve el total; en `manchas`, si se pide, el detalle.
int contarFlores(const uint8_t* img, int ancho, int alto, const ParamsFlor& p,
                 std::vector<Mancha>* manchas = nullptr);

// ------------------------------------------------------- de flores a kilos

struct ParcelaFlor {
  int plantasCentinela = 40;   // las que ve la cámara
  int plantasTotales = 1600;
  float floresPorHora = 325;   // una persona entrenada: 300–350 (guía, §5)
  float cuajado = 0.85f;       // fracción de flores polinizadas que dan fruto
  float gramosFruto = 489;     // con polen cruzado (guía, §5); 215 con el propio
  int diasACosechaMin = 30;    // de flor a fruta madura, en verano
  int diasACosechaMax = 35;
};

struct EstimacionFlor {
  int vistas = 0;         // flores contadas en las centinela
  int finca = 0;          // estimación para toda la plantación
  int fincaMin = 0;       // horquilla al 80 %
  int fincaMax = 0;
  float horasPincel = 0;  // trabajo de polinización de esa mañana
  float kilos = 0;        // fruta esperada de esa noche
  time_t cosechaDesde = 0;
  time_t cosechaHasta = 0;
};

// Intervalo de confianza de un conteo de Poisson (aproximación de Byar).
// `z` = 1,2816 para el 80 %, 1,96 para el 95 %.
void intervaloPoisson(int cuenta, double z, double* inferior, double* superior);

EstimacionFlor estimarFinca(int floresVistas, time_t noche, const ParcelaFlor& p);

// Calendario de cosecha: cada noche de flor es una cohorte que madura un mes
// después. Guarda las últimas noches con flor en un anillo de tamaño fijo.
class CalendarioCosecha {
 public:
  static constexpr int kCohortes = 48;

  struct Cohorte {
    int64_t noche = 0;
    int32_t floresFinca = 0;
  };
  struct Estado {
    Cohorte c[kCohortes];
    int32_t siguiente = 0;
  };

  void anotar(time_t noche, int floresFinca);
  // Kilos que maduran entre `desde` y `hasta` (epoch), repartiendo cada cohorte
  // de forma uniforme en su ventana de cosecha.
  float kilosEntre(time_t desde, time_t hasta, const ParcelaFlor& p) const;

  const Estado& estado() const { return e_; }
  void restaurar(const Estado& e) { e_ = e; }

 private:
  Estado e_;
};

}  // namespace sereno
