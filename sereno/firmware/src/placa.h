// Sereno · patillaje de la LilyGO T-SIM7080G-S3
//
// Los pines del módem, la cámara, la PMU y la tarjeta SD están copiados del
// `utilities.h` del fabricante (repo Xinyuan-LilyGO/LilyGo-T-SIM7080G, variante
// LILYGO_ESP32S3_CAM_SIM7080G). Con la cámara conectada, el fabricante indica
// que sólo quedan libres GPIO45 y GPIO46: por eso todo lo que se añade cuelga
// de un bus 1-Wire, de un transistor y del I2C que ya usa la PMU.
#pragma once

// --- Módem SIM7080G (LTE-M / NB-IoT; no tiene 2G ni 4G convencional)
#define PIN_MODEM_PWR 41
#define PIN_MODEM_DTR 42
#define PIN_MODEM_RI 3
#define PIN_MODEM_RXD 4
#define PIN_MODEM_TXD 5

// --- I2C de la PMU AXP2101 (0x34). Aquí cuelgan también el SHT4x (0x44) y el
//     MLX90614 (0x5A). OJO: comprueba que tu placa saca estos dos pines al
//     cabezal; la documentación del fabricante no lo detalla.
#define PIN_I2C_SDA 15
#define PIN_I2C_SCL 7
#define PIN_PMU_IRQ 6

// --- Cámara (conector FPC)
#define PIN_CAM_PWDN -1
#define PIN_CAM_RESET 18
#define PIN_CAM_XCLK 8
#define PIN_CAM_SIOD 2
#define PIN_CAM_SIOC 1
#define PIN_CAM_VSYNC 16
#define PIN_CAM_HREF 17
#define PIN_CAM_PCLK 12
#define PIN_CAM_Y9 9
#define PIN_CAM_Y8 10
#define PIN_CAM_Y7 11
#define PIN_CAM_Y6 13
#define PIN_CAM_Y5 21
#define PIN_CAM_Y4 48
#define PIN_CAM_Y3 47
#define PIN_CAM_Y2 14

// --- Lo que añade Sereno
// GPIO46: bus 1-Wire de las cuatro DS18B20, con 4,7 kΩ a 3V3. Es un pin de
//   arranque del ESP32-S3: la resistencia lo deja en alto, lo que sólo estorba
//   al entrar en modo descarga. Si la placa no se deja grabar, desenchufa el
//   conector de sondas mientras flasheas.
// GPIO45: enciende el destello. Alimenta la patilla VDD de un driver AMC7135
//   (350 mA constantes) que regula un LED blanco de 1 W tomado de VSYS. Así la
//   corriente del LED no depende de si VSYS está a 3,5 V (de noche, a pila) o
//   a casi 6 V (de día, con el panel al sol). GPIO45 también es pin de
//   arranque (elige la tensión de la flash) y DEBE estar en bajo al encender:
//   la resistencia de 10 kΩ a masa lo garantiza. Nunca cuelgues de aquí nada
//   con resistencia a positivo.
#define PIN_SONDAS 46
#define PIN_DESTELLO 45

// --- Botón BOOT de la placa: despierta al aparato en «modo visita».
#define PIN_BOTON 0
