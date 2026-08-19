# Romance sonámbulo

Una página para el *Romance sonámbulo* de Federico García Lorca, del **Romancero
gitano** (1928).

- Publicada en: https://fecia82.github.io/rambla-de-las-vacas/romance-sonambulo/
- Un solo archivo (`index.html`): CSS y JS en línea, sin build. La única
  dependencia externa son dos familias de Google Fonts, con pila de reserva
  declarada.
- No hay ni una fotografía. Todo el color y el movimiento sale de los versos.

## Las dos ideas

**El verde es el protagonista, no el adorno.** El fondo de la página se
interpola con el scroll: sube a verde en los tres estribillos y se desangra
hacia un pardo casi neutro en el trato de los compadres y en la llegada de los
guardias. Se puede comprobar recorriendo la página: `#tinte` pasa por
`rgb(14,44,30)` en el primer estribillo y por `rgb(22,20,16)` en los guardias.

**El poema sube mientras el lector baja.** «Dejadme subir al menos / hasta las
altas barandas». En la sección de la subida las barandas cruzan la pantalla
*hacia arriba* en bucle, así que bajar por la página se lee como trepar.

## Los movimientos

| Sección | Qué hace el scroll |
|---|---|
| Portada | desplaza y funde el título |
| Estribillos (×2) | el verso se fija a pantalla completa, entra y sale con el verde en su punto más alto |
| Tiradas | cada verso se enciende justo antes de entrar en cuadro, uno a uno |
| La subida | barandas en bucle hacia arriba y una mancha de sangre que crece |
| El aljibe | el agua sube, el carámbano de luna baja y el reflejo se mece |
| Los guardias | cinco golpes secos que descolocan el verso y meten un rojo desde los bordes |

Sin JavaScript o con `prefers-reduced-motion`, `<html>` no recibe la clase `mo`
y **los 86 versos quedan visibles y en orden**, sin animación. Está comprobado:
ningún verso se queda por debajo de opacidad 0,5 en ninguno de los dos casos.

## Tipografía

- **Bodoni Moda** para el título y los estribillos — didona, el filo de un libro
  de 1928. Reserva: Didot, Bodoni MT.
- **Cormorant Garamond** para el verso, grande, como en un libro. Reserva: Iowan
  Old Style, Palatino, Georgia.

Deliberadamente sin monoespaciada: esa era la voz de la otra página de este
repositorio y aquí habría sido pereza.

## El texto

Lorca murió en 1936 y en España los derechos de los autores fallecidos antes de
1987 caducan a los ochenta años, así que el *Romancero gitano* es de **dominio
público desde el 1 de enero de 2017**. El poema se reproduce entero, sin cortes:
86 versos.

Cotejado con [PoeMAS (UNED)](https://poemas.uned.es/poema/romance-sonambulo-federico-garcia-lorca/)
y la [Poetry Foundation](https://www.poetryfoundation.org/poems/161926/romance-sonambulo).
Las versiones que circulan por internet arrastran erratas y saltos de tirada
—una de las consultadas se saltaba entera la tirada central y escribía «mi
cuchillo *per* su manta»—. **Antes de darlo por bueno conviene contrastar contra
una edición en papel**, en particular:

- «desde los **puertos** de Cabra» (aparece también como «montes»)
- «**ese** trato se cerraba» (aparece también como «este»)

## Si algún día hay imágenes

No hay ninguna a propósito, pero el mecanismo de la otra página de este
repositorio (`blau-terranova/`) es trasplantable: elementos con `data-foto` que
prueban el archivo con un `new Image()` y solo lo superponen si carga. Aquí no
se ha puesto porque una fotografía sobre estos versos los aplanaría.
