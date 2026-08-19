# Blau Terranova Garbí — web de la casa

Landing de una página para el apartamento **Blau Terranova Garbi**, calle Faig 16,
Playa de Terranova, Oliva (Valencia).

- Publicada en: https://fecia82.github.io/rambla-de-las-vacas/blau-terranova/
- Un solo archivo (`index.html`): CSS y JS en línea, sin dependencias, sin build.
- Toda la dirección de arte es CSS/SVG generado — no hay ninguna imagen, así que
  la página pesa lo que pesa el HTML y no hay fotos de stock haciéndose pasar por
  fotos de la casa.

## Cómo está hecha

Mismo motor de scroll que el sitio de Rambla de las Vacas: un único bucle
`requestAnimationFrame` con scroll suavizado que reparte el progreso entre
"módulos" medidos en `medir()`. Cada módulo mapea su progreso 0→1 a estilos:

| Pieza | Qué hace el scroll |
|---|---|
| Hero | desplaza y funde el titular, mueve el degradado dentro de las letras |
| `.scrolly` | encadena mensajes a pantalla completa sobre fondos que se cruzan |
| `.full` | revela la escena de la urbanización con `clip-path` + parallax |
| `#daypin` | arrastra el sol por el arco del día: cielo, mar, reloj y frases |
| `.kobig` | mueve el degradado dentro de "TERRANOVA" |
| `.cnt` | cuenta los números al entrar en pantalla |

Sin JavaScript o con `prefers-reduced-motion`, el `<html>` no recibe la clase `mo`
y todas las piezas caen a un estado final estático y legible.

## Datos de la ficha

Verificados en fuentes públicas:

- Playa de Terranova–Burguera: ~1.750 m, arena fina y dorada, Bandera Azul,
  cordón dunar, manantial de agua dulce (El Tou), nidificación del chorlitejo
  patinegro, accesible, clásico del windsurf.
- Urbanización Garbí (Playa Terranova): zonas ajardinadas, piscina comunitaria y
  pista de pádel; ~280 m a la playa.
- Distancias a Font Salada (~10 min), Marjal de Pego-Oliva (~12 min) y
  Gandía (~25 min).

Pendientes de confirmar contra la ficha real antes de darla por buena — Booking
está bloqueado desde el entorno donde se generó esta página:

- Superficie en m², número de baños y reparto exacto de camas.
- Si el apartamento concreto tiene terraza, lavavajillas y plaza de garaje.
- Horarios de entrada y salida, y política de mascotas.

El precio y la disponibilidad no se tocan aquí a propósito: los tres CTA llevan a
la ficha de Booking, que es la que manda.

## Si algún día hay fotos

La escena CSS de la sección `.full` (`.esc-casa`) se puede sustituir por una foto
sin tocar el motor: basta con dejar dentro de `.plx` un `<img>` a pantalla completa
en lugar del `<div class="esc-casa">`. El revelado por `clip-path` y el parallax
siguen funcionando igual.
