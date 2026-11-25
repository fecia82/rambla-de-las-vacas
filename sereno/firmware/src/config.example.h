// Sereno · configuración
// Copia este archivo como `config.h` (no se sube a git) y rellénalo.
#pragma once

// --- La finca
#define SERENO_LATITUD 37.3316
#define SERENO_LONGITUD -1.7271        // positiva al este
#define SERENO_PLANTAS_CENTINELA 40    // las que caben en la foto
#define SERENO_PLANTAS_TOTALES 1600

// --- La SIM (LTE-M / NB-IoT). El APN lo da el operador de la tarjeta.
#define SERENO_APN "pon-aqui-el-apn"

// --- A quién canta: un bot de Telegram.
// Crea uno con @BotFather y pega aquí su token y el identificador del chat.
// Con el token vacío, Sereno no intenta conectarse: sólo apunta en su libreta
// (memoria interna) y se descarga por wifi en modo visita.
#define SERENO_TELEGRAM_TOKEN ""
#define SERENO_TELEGRAM_CHAT ""

// --- Las sondas DS18B20, de abajo arriba: 0,1 m · 0,5 m · 1 m · 2 m.
// Cada una tiene un código único de 8 bytes. En modo visita la página los
// enseña con su temperatura: calienta una con la mano, mira cuál sube y apunta
// su código en la altura que le toca. Con todo a cero se usan en el orden en
// que aparecen en el bus (vale para probar, no para medir).
#define SERENO_SONDAS                                   \
  {                                                     \
    {0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0}, \
    {0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0}  \
  }

// Corrección de cada sonda en °C, medida en un vaso de hielo picado con agua
// (que está a 0,0 °C): si una marca 0,3, su corrección es −0,3. Una DS18B20
// tiene ±0,5 °C de fábrica, justo donde se decide todo; este paso lo arregla.
#define SERENO_AJUSTE_SONDAS {0.0f, 0.0f, 0.0f, 0.0f}

// --- Modo visita: wifi propia del aparato (mantén pulsado BOOT un segundo).
#define SERENO_WIFI_NOMBRE "Sereno"
#define SERENO_WIFI_CLAVE "cambia-esta-clave"
