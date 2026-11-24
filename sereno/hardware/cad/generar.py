#!/usr/bin/env python3
"""Genera los STL y las imágenes de Sereno a partir de sereno.scad.

Uso:
    python3 generar.py            # todo: STL + imágenes
    python3 generar.py stl        # solo los STL (y su comprobación)
    python3 generar.py img        # solo las imágenes
    python3 generar.py cabeza     # una sola vista (o una sola pieza)

Necesita OpenSCAD (probado con 2021.01), numpy y Pillow. Sin pantalla se usa
xvfb-run. Los STL salen en ../stl y las imágenes en ../../docs/img.
"""
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageOps

AQUI = Path(__file__).resolve().parent
SCAD = AQUI / "sereno.scad"
DIR_STL = AQUI.parent / "stl"
DIR_IMG = AQUI.parent.parent / "docs" / "img"
TAM = (1600, 1200)

# Piezas imprimibles, cada una en su orientación de impresión
PIEZAS = ["farol_cuerpo", "farol_tejado", "anillo_cielo", "pagoda"]

# Vistas. camara = (centro x, y, z, giro x, y, z, distancia), como --camera.
# espejo: OpenSCAD ilumina siempre desde arriba a la izquierda; para que la
# cara frontal quede iluminada al mirar desde la izquierda se dibuja el modelo
# reflejado en X y después se voltea la imagen (la geometría sale correcta).
VISTAS = {
    "conjunto": dict(pieza="conjunto", camara=(70, -30, 1150, 80, 0, 325, 6350),
                     orto=True, espejo=True),
    "cabeza": dict(pieza="cabeza", camara=(0, 0, 105, 73, 0, 325, 760), espejo=True),
    "despiece": dict(pieza="despiece", camara=(0, 0, 215, 73, 0, 325, 1500), espejo=True),
    "cabeza_corte": dict(pieza="cabeza_corte", camara=(0, 8, 105, 90, 0, 90, 640), orto=True),
    "pagoda": dict(pieza="pagoda_montada", camara=(32, 0, 36, 74, 0, 325, 400), espejo=True),
    "pagoda_corte": dict(pieza="pagoda_corte", camara=(32, 0, 36, 84, 0, 0, 420), orto=True),
}


def openscad(args):
    """Lanza OpenSCAD (con xvfb-run si no hay pantalla) y devuelve su salida."""
    orden = ["openscad"] + args
    if not os.environ.get("DISPLAY") and shutil.which("xvfb-run"):
        orden = ["xvfb-run", "-a"] + orden
    r = subprocess.run(orden, cwd=AQUI, capture_output=True, text=True)
    salida = r.stdout + r.stderr
    if r.returncode != 0:
        sys.exit(f"OpenSCAD ha fallado:\n{salida}")
    return salida


def comprobar_stl(ruta):
    """Caja envolvente y comprobación de que la malla es un único sólido cerrado."""
    texto = ruta.read_bytes()
    nums = re.findall(rb"vertex\s+(\S+)\s+(\S+)\s+(\S+)", texto)
    tri = np.array(nums, dtype=float).reshape(-1, 3, 3)
    pts = tri.reshape(-1, 3)
    _, inv = np.unique(np.round(pts, 5), axis=0, return_inverse=True)
    caras = inv.reshape(-1, 3)
    aristas = np.concatenate([caras[:, [0, 1]], caras[:, [1, 2]], caras[:, [2, 0]]])
    _, veces = np.unique(np.sort(aristas, axis=1), axis=0, return_counts=True)
    # componentes conexas (propagación de etiquetas por las aristas)
    etiqueta = np.arange(inv.max() + 1)
    while True:
        nueva = etiqueta.copy()
        m = np.minimum(etiqueta[aristas[:, 0]], etiqueta[aristas[:, 1]])
        np.minimum.at(nueva, aristas[:, 0], m)
        np.minimum.at(nueva, aristas[:, 1], m)
        if (nueva == etiqueta).all():
            break
        etiqueta = nueva
    vol = np.einsum("ij,ij->i", tri[:, 0], np.cross(tri[:, 1], tri[:, 2])).sum() / 6000
    mn, mx = pts.min(0), pts.max(0)
    return dict(min=mn, max=mx, tam=mx - mn, abiertas=int((veces != 2).sum()),
                solidos=len(np.unique(etiqueta)), cm3=vol, triangulos=len(tri))


def exportar(pieza):
    destino = DIR_STL / f"{pieza}.stl"
    salida = openscad(["-o", str(destino), "-D", f'pieza="{pieza}"', str(SCAD)])
    avisos = [l for l in salida.splitlines()
              if re.search(r"WARNING|ERROR|manifold|Simple:\s+no", l, re.I)]
    c = comprobar_stl(destino)
    ok = not avisos and c["abiertas"] == 0 and c["solidos"] == 1
    t = c["tam"]
    print(f"{pieza:13s} {t[0]:7.2f} × {t[1]:6.2f} × {t[2]:6.2f} mm   z0 = {c['min'][2]:.2f}   "
          f"{c['cm3']:6.1f} cm³   {c['triangulos']} triángulos   "
          f"{'sólido único y cerrado' if ok else 'REVISAR'}")
    for l in avisos:
        print("   ", l)
    return ok


def dibujar(nombre):
    v = VISTAS[nombre]
    x, y, z, rx, ry, rz, d = v["camara"]
    espejo = v.get("espejo", False)
    camara = (-x, y, z, rx, ry, -rz % 360, d) if espejo else (x, y, z, rx, ry, rz, d)
    destino = DIR_IMG / f"{nombre}.png"
    args = ["-o", str(destino), f"--imgsize={TAM[0]},{TAM[1]}",
            "--camera=" + ",".join(f"{c:g}" for c in camara),
            "--projection=" + ("o" if v.get("orto") else "p"),
            "--colorscheme=Tomorrow",
            "-D", f'pieza="{v["pieza"]}"', "-D", f"espejo={'true' if espejo else 'false'}",
            "-D", f"vista_rx={rx}", "-D", f"vista_rz={rz}", str(SCAD)]
    openscad(args)
    if espejo:
        ImageOps.mirror(Image.open(destino)).save(destino)
    print(f"{nombre}.png")


def main():
    que = sys.argv[1:] or ["stl", "img"]
    DIR_STL.mkdir(parents=True, exist_ok=True)
    DIR_IMG.mkdir(parents=True, exist_ok=True)
    piezas = [p for p in PIEZAS if "stl" in que or p in que]
    vistas = [v for v in VISTAS if "img" in que or v in que]
    bien = all([exportar(p) for p in piezas])
    for v in vistas:
        dibujar(v)
    sys.exit(0 if bien else 1)


if __name__ == "__main__":
    main()
