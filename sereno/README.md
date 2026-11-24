# Sereno

Una estaca con cuatro termómetros, un sensor que mira al cielo y un farol con cámara.
Vela la finca cuando no hay nadie: en invierno mide el aire frío que se embalsa en la
hondonada; en verano cuenta las flores de pitaya que abren esa noche. Al amanecer lo
canta en un mensaje, como el sereno cantaba la hora y el tiempo.

```
¡Las nueve y helando!
Mínima de la noche: −1,1 °C a ras de suelo (08:10).
Perfil: −1,1 · −0,4 · 0,0 · 0,6 °C (a 0,1 · 0,5 · 1 · 2 m).
Aire frío embalsado: hasta 1,7 °C más frío abajo que a 2 m.
Helada: 200 min bajo cero a ras de suelo, 0 a 1 m.
Temporada: 70 noches · 1 de helada · mínima a 1 m 0,0 °C · 256 horas frío.
Veredicto provisional: invernadero obligatorio (1 m bajó de 1 °C).
```

Ese parte lo generó el firmware real sobre un invierno simulado. **Esto es un prototipo
v0: un diseño completo con su lógica probada en un PC. No se ha impreso, soldado ni
encendido nada.** Más abajo se detalla qué está comprobado y qué no.

## Por qué existe

El estudio de la parcela deja dos incógnitas que solo se resuelven midiendo y una tarea
con horario, y las tres ocurren de noche:

| El estudio dice | Sereno hace |
|---|---|
| «0,1 días de helada al año», pero es un dato de satélite en malla de ~25 km que no ve la hondonada (estudio §2, guía §5) | Mide a 0,1 · 0,5 · 1 · 2 m del suelo, más temperatura del cielo para saber si la noche es serena |
| «Hace falta un invierno entero» (guía §7) | Con un invierno calibra cuánto se aparta la hondonada del mapa y lo proyecta sobre veinte inviernos de reanálisis |
| «Cada flor abre una sola noche»; unas 35 mañanas de pincel (guía §5) | A la una fotografía las plantas centinela, cuenta las flores y estima las de la finca, las horas de pincel y los kilos de un mes después |

## Qué hay en esta carpeta

```
firmware/
  lib/sereno_core/   la lógica, sin dependencias de hardware
    sol       orto y ocaso
    helada    resumen de la noche, temporada, pronóstico de mínima (método FAO)
    flor      conteo de flores, estimación de la finca, calendario de cosecha
    cante     los partes en castellano
    guardia   la máquina de estados de la noche
  src/               el firmware de la placa (LilyGO T-SIM7080G-S3)
  pruebas_pc/        pruebas y simulación, se ejecutan en un PC
tools/informe_helada.py   de un invierno medido a veinte años estimados
hardware/cad/        carcasa paramétrica en OpenSCAD
hardware/stl/        piezas exportadas
docs/                resultados de la simulación y renders
```

## Qué está comprobado y qué no

| Parte | Estado | Qué se hizo |
|---|---|---|
| Lógica (`sereno_core`) | Probada en PC | `make prueba`: 222 comprobaciones, 0 fallos |
| Temporada simulada | Ejecutada | `make sim`: 150 noches de invierno y un verano por el mismo código que lleva la placa. El tiempo es inventado |
| Informe a veinte años | A medias | `--autoprueba` en bucle cerrado superada. La descarga de Open-Meteo no se pudo probar (red cerrada) |
| Firmware de la placa | **Sin compilar para ESP32** | Sintaxis comprobada en PC con cabeceras simuladas; cada llamada a librería contrastada con las fuentes de las versiones fijadas en `platformio.ini`. El entorno donde se escribió no deja descargar el compilador |
| Patillaje | Casi | Módem, cámara, PMU y SD copiados del `utilities.h` del fabricante. Falta confirmar que GPIO15/GPIO7 (I2C) salen al cabezal |
| Carcasa | Modelada | Exportada a STL sin errores. No impresa; la caja de la placa (112 × 38 × 26 mm) es una suposición a medir |
| Contar flores reales | Sin probar | Solo escenas sintéticas |
| Cobertura en la parcela | Sin comprobar | El módem solo habla LTE-M y NB-IoT |
| Consumo | Estimado | Unos 50 mAh/día, sin medir |

## Probarlo sin hardware

```sh
cd firmware
make prueba     # pruebas del núcleo
make sim        # simula invierno y verano, escribe ../docs/simulacion.json
python3 ../tools/informe_helada.py --autoprueba
```

Resultados de la última ejecución (simulados, no predicen esta finca):

- Invierno: 2 noches de helada a ras de suelo donde el «mapa» no vio ninguna. En las 71
  noches serenas el suelo quedó de media 2,0 °C por debajo del mapa. El pronóstico que
  el aparato aprende solo tuvo un error típico de 0,5 °C en 63 noches.
- Verano: 298 flores contadas en las fotos frente a 287 pintadas. La horquilla al 80 %
  contuvo el valor real de la finca en 37 de 42 noches.
- Informe: calibrando con un invierno, estima 3,0 noches de helada al año a ras de suelo
  frente a 3,1 reales en el mundo de juguete; el mapa sin corregir diría 0,4.

Una versión anterior del contador de flores, de «una mancha, una flor», veía 375 de 573.
La simulación lo destapó y por eso ahora resta el fondo entre noches y separa las flores
que se tocan.

## Piezas

Precios orientativos, de memoria, sin IVA ni envío.

| Pieza | Uds. | € |
|---|---:|---:|
| LilyGO T-SIM7080G-S3 con cámara y antenas | 1 | 34 |
| Pila 18650 protegida, ≥ 2.500 mAh | 1 | 6 |
| Panel solar 6 V, 1–2 W (110 × 60 mm) | 1 | 5 |
| Sonda DS18B20 encapsulada, cable de 3 m | 4 | 10 |
| SHT45 en placa | 1 | 9 |
| MLX90614 (GY-906) | 1 | 9 |
| LED blanco 1 W en estrella + driver AMC7135 | 1 | 3 |
| Resistencias (4,7 kΩ, 10 kΩ), prensaestopas PG7, cable, bornas | — | 5 |
| SIM IoT prepago LTE-M / NB-IoT | 1 | 10 |
| Filamento PETG o ASA, tornillos M3, metacrilato de 2 mm, film de polietileno | — | 8 |
| Estaca o tutor de 2,5 m, Ø 25–50 mm | 1 | 6 |
| **Total** | | **≈ 105** |

## Conexiones

Con la cámara conectada, el fabricante indica que a la placa solo le quedan libres
GPIO45 y GPIO46. Todo lo demás se cuelga de buses:

| Señal | Pin | Qué cuelga |
|---|---|---|
| 1-Wire | GPIO46, con 4,7 kΩ a 3V3 | Las cuatro DS18B20 |
| I2C | GPIO15 (SDA), GPIO7 (SCL), el mismo de la PMU | SHT45 (0x44) y MLX90614 (0x5A) |
| Destello | GPIO45, con 10 kΩ a masa | VDD del AMC7135; el LED toma corriente de VSYS |
| Solar | Entrada solar de la placa, 4,4–6 V | Panel |

GPIO45 y GPIO46 son pines de arranque del ESP32-S3. GPIO45 debe estar en bajo al
encender (de ahí la resistencia a masa). La resistencia a positivo de GPIO46 puede
impedir entrar en modo descarga: si la placa no se deja grabar, desenchufa las sondas.

El sensor de cielo va en la chimenea del tejado, tras una ventana de film de polietileno,
que deja pasar el infrarrojo. Los umbrales de «sereno» y «nublado» son un punto de
partida y hay que afinarlos con un par de noches.

## Montarlo

1. Mide la placa con calibre y ajusta `placa_largo`, `placa_ancho` y `placa_grosor` en
   `hardware/cad/sereno.scad` antes de imprimir. Detalles en `hardware/cad/README.md`.
2. Copia `firmware/src/config.example.h` como `config.h` y rellénalo. Con el token de
   Telegram vacío no intenta conectarse: solo apunta en su memoria.
3. `cd firmware && pio run -t upload`. Es el primer paso que no se pudo dar aquí.
4. Prueba en la mesa, de uno en uno: sondas, SHT45, MLX90614, cámara y módem.
5. Mantén pulsado BOOT un segundo: el aparato levanta la wifi «Sereno» diez minutos.
   En `http://192.168.4.1` hay lecturas en vivo, los códigos de las sondas, una foto de
   prueba para apuntar la cámara y la descarga de los CSV. Al abrir la página, el móvil
   le pone la hora.
6. Calibra las sondas en un vaso de hielo picado con agua (0,0 °C) y apunta la
   corrección de cada una en `config.h`.
7. Clava la estaca en el punto más bajo de la parcela, con el tejado hacia el sur.

## Límites que conviene saber

- **Seguridad.** La conexión con Telegram va cifrada pero el módem no valida el
  certificado del servidor (la librería, en la versión fijada, no carga ninguno). Quien
  pudiera interponerse en la red vería el token del bot. Para un parte de temperaturas
  es un riesgo pequeño; para algo más serio, hay que cargar el certificado raíz.
- **Privacidad.** La cámara apunta a tu plantación. Si en el encuadre entra un camino o
  la finca del vecino, cámbialo.
- **El pronóstico** solo avisa en noches serenas y cuando ya ha aprendido de ocho noches
  de radiación. No sustituye a un parte meteorológico.
- **El conteo** depende de que el fondo no cambie. Mover la cámara obliga a reaprenderlo
  (unas seis noches) borrando `/fondo.bin`.
- **La rambla.** Medir la escorrentía (punto 09 de la diligencia previa) no está en esta
  versión: en la placa no quedan pines y una sonda en el cauce toca dominio público
  hidráulico.

## ¿Nunca hecho?

No se puede demostrar. Cinco búsquedas no encontraron ningún aparato que avise de la
noche de flor de la pitaya ni que la polinice; lo más cercano es un artículo de conteo
de flores en vídeo y un proyecto de mantenimiento de la iluminación de floración. Para
la helada sí existe mucho: estaciones comerciales, redes de registradores y el método
empírico de la FAO que aquí se usa. Lo que no apareció es la combinación en una estaca
de unos cien euros, con el conteo y el pronóstico hechos en el propio aparato.

Fuentes consultadas:
[polinización, 2025](https://www.mdpi.com/2223-7747/14/19/3102) ·
[‘Royal Red’ en cultivo protegido](https://www.ncbi.nlm.nih.gov/pmc/articles/PMC12526123/) ·
[conteo de flores en vídeo](https://libmast.utm.my/Record/doaj.art-8c1fdbd67ae94b22a90c6c0b525ce304) ·
[BloomGuard](https://www.hackster.io/pramudyadickisaputra/bloomguard-predictive-maintenance-for-dragon-fruit-lighting-09c87f) ·
[manual de heladas de la FAO](https://www.fao.org/4/y7223e/y7223e0b.htm) ·
[redes de registradores](https://www.mssanz.org.au/modsim2017/H9/gobbett.pdf) ·
[conservación de polen](https://journals.ashs.org/downloadpdf/view/journals/hortsci/35/2/article-p199.pdf) ·
[repositorio del fabricante de la placa](https://github.com/Xinyuan-LilyGO/LilyGo-T-SIM7080G)

## Negocio (hipótesis sin validar)

| Oferta | Precio supuesto | Argumento, con cifras del estudio |
|---|---|---|
| Invierno de prueba: alquiler nov–mar + informe | 79–99 € | En esta finca la respuesta separa malla de invernadero: 30.600 € |
| Kit en propiedad | 189–249 € | Una flor sin polinizar son unos 2 € de fruta (489 g × 4,50 €/kg × 85 % de cuajado) |
| Servicio | 5–9 €/mes | El canal directo vale 7 €/kg frente a 4,50 y necesita previsión de kilos por semana |

Lo primero que habría que averiguar: cuántos productores de pitaya hay en España (la guía
solo da el crecimiento, +205 % en tres años) y cuánta gente pagaría por medir una
parcela antes de plantarla.
