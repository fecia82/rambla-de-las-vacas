#!/bin/sh
# Deja en px/ las fotos ya ampliadas y enfocadas.
#
# Las originales son pequeñas (hero.jpg mide 1170x823) y el recorte vertical de
# 1080x1920 las estiraba dos veces y media: el cielo se empastaba. Aquí se
# amplían antes con lanczos y un enfoque suave, de modo que el navegador acabe
# REDUCIENDO en vez de ampliar. Es la diferencia entre un plano nítido y uno sucio.
#
#   FF=/ruta/a/ffmpeg sh preparar.sh
set -e
cd "$(dirname "$0")"
FF="${FF:-ffmpeg}"
mkdir -p px
for par in "hero.jpg 2450" "04.jpg 2400" "03.jpg 2400" "08.jpg 2400" "descalzo.jpg 2450"; do
  f=${par% *}; alto=${par#* }
  "$FF" -y -loglevel error -i "../../img/$f" \
        -vf "scale=-2:$alto:flags=lanczos,unsharp=5:5:0.62:3:3:0.28" -q:v 2 "px/$f"
  echo "  px/$f"
done
echo "listo"
