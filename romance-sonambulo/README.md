# Romance sonámbulo

Una página para el *Romance sonámbulo* de Federico García Lorca, del **Romancero
gitano** (1928).

- Publicada en: https://fecia82.github.io/rambla-de-las-vacas/romance-sonambulo/
- Un solo archivo (`index.html`): CSS y JS en línea, sin build. La única
  dependencia externa son dos familias de Google Fonts, con pila de reserva
  declarada.
- Doce ilustraciones aportadas por el autor de la página, en `img/`. Ninguna
  fotografía.

## Las cuatro ideas

**El agua sabe el final desde el principio.** La muchacha lleva ahogada en el
aljibe desde el primer verso: por eso es verde. Así que el agua no es una sección,
es toda la página. Un lienzo fijo pinta una lámina de agua que sube con el poema,
y **su rostro va reflejado boca abajo bajo la superficie desde la portada**,
ondulando, ilegible. Solo cuando el nivel alcanza el aljibe se endereza y se ve.
Al cerrarse el poema el agua se retira, pero ella sigue debajo.

La agitación del agua la pone el lector: se calcula del desfase entre el scroll
real y el suavizado, o sea de **la prisa con que se baja**. Hay que ir despacio
para verla. Los cinco golpes de los guardias civiles mandan anillos por la
superficie.

El lienzo va en `z-index:1`: por encima de las láminas y los fondos, por debajo
del verso. Las ilustraciones se sumergen; la voz no.

**El scroll es la voz.** Las palabras no aparecen por versos sino de una en una,
y la que se está diciendo brilla mientras las ya dichas se retiran a un tono
menor. Las marcadas con `<em>` en el HTML son las que un actor sostendría:
cuestan 2,4 golpes de voz en vez de 1, y se van al verde mientras se dicen. En
las tiradas de sangre el foco es rojo. El presupuesto es de unos 24 px de scroll
por golpe, que es lo que separa un recitado de un parpadeo.

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
| Portada | desplaza y funde el título sobre una plancha que se aleja |
| Láminas | se abren por una banda que crece, sostienen con un zoom lento y se cierran |
| Estribillo I | el verso se fija a pantalla completa con el verde en su punto más alto |
| Estribillo II | lo mismo, con la tipografía **calada sobre la lámina crema**, que se recorre |
| Tiradas | el verso se fija y la voz lo recorre palabra a palabra |
| La subida | barandas en bucle hacia arriba y una mancha de sangre que crece |
| Díptico | las dos láminas del rastro se abren desfasadas, una detrás de otra |
| El aljibe | el agua sube, el carámbano de luna baja y el reflejo se mece invertido |
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

## Las láminas

Doce ilustraciones, troceadas de dos hojas de contacto 3×2 aportadas por el autor
de la página. Se reparten en dos oficios:

- **`l1`–`l6`** llevan el verso impreso. Van solas, a pantalla completa, como
  láminas de un libro ilustrado. Su `alt` describe la imagen y recoge el verso.
- **`f1`–`f6`** van limpias. Se usan de fondo, atenuadas y con parallax, detrás
  del verso recitado — y una de ellas rellena la tipografía calada del segundo
  estribillo.

Que una lámina repita el verso que la voz dice a continuación no es un descuido:
el romance vive de la repetición y del eco. «Verde que te quiero verde» aparece
cuatro veces en el propio poema, y «Pero yo ya no soy yo» y «Dejando un rastro
de…» dos cada uno.

Para el calado hace falta una imagen de brillo parejo: con una plancha oscura las
letras desaparecen sobre el fondo negro. Por eso el segundo estribillo se rellena
con `l1`, que es la única con un campo crema grande, y el recorrido se queda por
encima de su texto impreso.
