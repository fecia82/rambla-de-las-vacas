# Blau Terranova · Garbí — web de la casa

Landing de una página para el apartamento **Blau Terranova Garbi**, calle Faig 16,
Playa de Terranova, Oliva (Valencia).

- Publicada en: https://fecia82.github.io/rambla-de-las-vacas/blau-terranova/
- Un solo archivo (`index.html`): CSS y JS en línea, sin dependencias, sin build.
- Siete fotos reales del anuncio —seis del piso y la pasarela de las dunas—; el resto de la
  dirección de arte es CSS/SVG generado, así que la página funciona entera
  aunque falte cualquier imagen. Ver «Fotos» y `img/LEEME.txt`.

## Cómo está hecha

Mismo motor de scroll que el sitio de Rambla de las Vacas: un único bucle
`requestAnimationFrame` con scroll suavizado que reparte el progreso entre
"módulos" medidos en `medir()`. Cada módulo mapea su progreso 0→1 a estilos:

| Pieza | Qué hace el scroll |
|---|---|
| Hero | desplaza y funde el titular, mueve el degradado dentro de las letras |
| `.scrolly` | encadena mensajes a pantalla completa sobre fondos que se cruzan |
| `.full` | revela la escena de la urbanización con `clip-path` + parallax |
| `#daypin` | 24 h completas: el sol **sale del mar** y se pone tierra adentro (la playa de Oliva mira al este), la luna cubre la noche y al final vuelve a amanecer; el Montgó al fondo cambia de color y el reflejo del sol se mueve sobre el agua |
| `.kobig` | mueve el degradado dentro de "TERRANOVA" |
| `.cnt` | cuenta los números al entrar en pantalla |
| `.galeria` | rejilla de fotos con lightbox, oculta si no hay ninguna |
| `.banda` | bandas de foto con parallax corto, ocultas si falta el archivo |
| `#kite` | la cometa flamea, las bridas se arquean y las rachas cruzan |

Sin JavaScript o con `prefers-reduced-motion`, el `<html>` no recibe la clase `mo`
y todas las piezas caen a un estado final estático y legible.

## Datos de la ficha

De la propia ficha (título y fotos 1–7 de 23):

- **Apartamento de 3 dormitorios**, con cabeceros y armarios de caña.
- Salón con chimenea de leña y televisión; suelo de terrazo en toda la casa.
- Terraza cubierta con mesa larga de madera y sillas de enea.
- Galería acristalada con butacas, abierta a un balcón.
- Aire acondicionado, ventilador de techo, lavadero con lavadora, Wi-Fi.
- La primera foto del anuncio es la pasarela de madera que cruza el cordón dunar
  hasta la orilla: es la que confirma el acceso a pie que cuenta la página.

De fuentes públicas:

- Playa de Terranova–Burguera: ~1.750 m, arena fina y dorada, Bandera Azul,
  cordón dunar, manantial de agua dulce (El Tou), nidificación del chorlitejo
  patinegro, accesible, clásico del windsurf.
- Distancias a Font Salada (~10 min), Marjal de Pego-Oliva (~12 min) y
  Gandía (~25 min). Cada una enlaza a su ficha en Google Maps.

Contacto directo: **Sunsuite**, +34 604 529 655 (WhatsApp y teléfono), junto al
CTA de Booking.

Estimado, no verificado: los «2 min a la arena». Salen de las vistas aéreas de
la manzana, donde la calle Faig muere en una pasarela que cruza el cordón dunar.
Si la distancia real es otra, se cambia en el contador `data-to` de la sección de
cifras y en el croquis.

Deliberadamente fuera de la página por no poder atarlo a este anuncio: piscina
comunitaria y pista de pádel. Aparecían en otro apartamento del mismo edificio,
no en esta ficha.

Pendiente de confirmar: superficie en m², número de baños, reparto exacto de
camas, plaza de aparcamiento, horarios de entrada y salida y política de
mascotas. El precio y la disponibilidad no se tocan aquí a propósito: los tres
CTA llevan a la ficha de Booking, que es la que manda.

## Fotos

La página está cableada para admitirlas sin tocar código: los elementos con
`data-foto` / `data-foto-ko` prueban el archivo con un `new Image()` y, solo si
carga, lo superponen a la escena CSS con un fundido. La galería se construye
desde la lista `GALERIA` del principio del `<script>` y la sección entera
permanece `hidden` mientras no cargue ninguna foto.

Es decir: se dejan los JPG en `img/` con los nombres de `img/LEEME.txt` y la
página los recoge sola. Lo que falte se queda con su escena dibujada.

Las fotos que hay en `img/` están recortadas de capturas de pantalla de la ficha
(fotos 1–7 de 23), quitando la interfaz del navegador. No se
pudieron descargar del original: `www.booking.com` y su CDN `cf.bstatic.com`
están bloqueados por la política de red del entorno donde se generó esta página
(403 en el túnel CONNECT del proxy). Sustituirlas por las originales es
sobrescribir los archivos con el mismo nombre.
