#!/usr/bin/env python3
"""Informe de helada: de un invierno medido a veinte años estimados.

Un invierno de mediciones es una sola tirada de dados: puede salir suave y dar
una confianza falsa. Lo que un invierno sí enseña bien es CUÁNTO MÁS FRÍA es la
hondonada que el «mapa» (el reanálisis ERA5, que la ve promediada en una malla
de ~25 km) y de qué depende esa diferencia: del cielo despejado y de la calma.
Con esa relación ajustada se recorren veinte inviernos de reanálisis y se
estima con qué frecuencia hiela de verdad en el punto exacto.

Uso:
  # 1. Con los CSV de la libreta de Sereno y descargando la referencia:
  python3 informe_helada.py --datos datos/*.csv --lat 40.0 --lon -3.0

  # 2. Con una referencia ya descargada (JSON de la API de archivo de Open-Meteo):
  python3 informe_helada.py --datos datos/*.csv --referencia era5.json --lon -3.0

  # 3. Comprobar el método en un mundo de juguete donde la verdad se conoce:
  python3 informe_helada.py --autoprueba

Límites, sin letra pequeña:
  · Calibra la diferencia hondonada-mapa; no adivina olas de frío que el propio
    reanálisis no contenga.
  · Con menos de ~60 noches comunes, o menos de ~15 despejadas, el ajuste es
    endeble: el informe lo avisa.
  · La descarga de Open-Meteo no se pudo probar desde el entorno donde se
    escribió esto (red cerrada). La lectura de su formato sí está probada con
    datos sintéticos.
"""
import argparse
import csv
import datetime as dt
import json
import math
import sys
import urllib.parse
import urllib.request

import numpy as np

UMBRALES = (1.0, 0.0, -2.0)  # decisión del estudio · helada · letal para la pitaya
MESES_FRIOS = (11, 12, 1, 2, 3)
URL_ARCHIVO = "https://archive-api.open-meteo.com/v1/archive"


def fecha_noche(t):
    """Cada noche se nombra por la fecha de su amanecer (corte a mediodía UTC)."""
    return (t + dt.timedelta(hours=12)).date()


def temporada(fecha):
    """Año en que termina el invierno al que pertenece la noche."""
    return fecha.year + 1 if fecha.month >= 7 else fecha.year


# ------------------------------------------------------------------ lectura


def leer_sereno(rutas):
    """CSV de la libreta → {fecha: {'suelo': mínima a 0,1 m, 'ref': mínima a 1 m}}."""
    noches = {}
    for ruta in rutas:
        with open(ruta, newline="", encoding="utf-8") as f:
            for fila in csv.DictReader(f):
                try:
                    t = dt.datetime.fromtimestamp(int(fila["epoch"]), dt.timezone.utc)
                    suelo, ref = float(fila["T010"]), float(fila["T100"])
                except (KeyError, ValueError):
                    continue
                n = noches.setdefault(fecha_noche(t), {"suelo": math.inf, "ref": math.inf, "n": 0})
                if not math.isnan(suelo):
                    n["suelo"] = min(n["suelo"], suelo)
                if not math.isnan(ref):
                    n["ref"] = min(n["ref"], ref)
                n["n"] += 1
    # Una noche a medias (aparato recién puesto, batería agotada) no cuenta.
    return {
        f: n for f, n in noches.items()
        if n["n"] >= 100 and math.isfinite(n["suelo"]) and math.isfinite(n["ref"])
    }


def leer_referencia(obj, lon):
    """JSON horario de Open-Meteo → {fecha: {'tmin', 'despejado', 'calma'}}.

    Sólo cuentan las horas de noche (hora solar local de 18 a 8). El viento
    debe venir en m/s (se pide así en la descarga).
    """
    h = obj["hourly"]
    nube = h.get("cloud_cover", h.get("cloudcover"))
    viento = h.get("wind_speed_10m", h.get("windspeed_10m"))
    acum = {}
    for i, texto in enumerate(h["time"]):
        t = dt.datetime.fromisoformat(texto).replace(tzinfo=dt.timezone.utc)
        solar = (t.hour + lon / 15.0) % 24
        if 8 <= solar < 18:
            continue
        temp = h["temperature_2m"][i]
        if temp is None or nube[i] is None or viento[i] is None:
            continue
        a = acum.setdefault(fecha_noche(t), {"tmin": math.inf, "nube": 0.0, "viento": 0.0, "n": 0})
        a["tmin"] = min(a["tmin"], temp)
        a["nube"] += nube[i]
        a["viento"] += viento[i]
        a["n"] += 1
    noches = {}
    for f, a in acum.items():
        if a["n"] < 10:
            continue
        despejado = 1.0 - a["nube"] / a["n"] / 100.0
        noches[f] = {
            "tmin": a["tmin"],
            "despejado": despejado,
            "calma": math.exp(-(a["viento"] / a["n"]) / 3.0),
        }
    return noches


def descargar_referencia(lat, lon, desde, hasta):
    consulta = urllib.parse.urlencode({
        "latitude": f"{lat:.4f}",
        "longitude": f"{lon:.4f}",
        "start_date": desde.isoformat(),
        "end_date": hasta.isoformat(),
        "hourly": "temperature_2m,cloud_cover,wind_speed_10m",
        "wind_speed_unit": "ms",
        "timezone": "UTC",
    })
    with urllib.request.urlopen(f"{URL_ARCHIVO}?{consulta}", timeout=120) as r:
        return json.load(r)


# ------------------------------------------------------------------ modelo


def rasgos(n):
    """La hondonada se enfría de más cuando el cielo está raso y no corre aire."""
    return [1.0, n["despejado"], n["despejado"] * n["calma"]]


def ajustar(sitio, referencia, nivel):
    comunes = sorted(set(sitio) & set(referencia))
    if len(comunes) < 20:
        raise SystemExit(
            f"Sólo hay {len(comunes)} noches comunes entre el aparato y la "
            "referencia: hacen falta al menos 20 (y mejor más de 60).")
    X = np.array([rasgos(referencia[f]) for f in comunes])
    y = np.array([sitio[f][nivel] - referencia[f]["tmin"] for f in comunes])
    beta, *_ = np.linalg.lstsq(X, y, rcond=None)
    resid = y - X @ beta
    gl = max(len(y) - X.shape[1], 1)
    sigma = float(np.sqrt(resid @ resid / gl))
    cov = sigma ** 2 * np.linalg.pinv(X.T @ X)
    total = float(((y - y.mean()) ** 2).sum())
    return {
        "beta": beta, "cov": cov, "sigma": sigma, "n": len(comunes),
        "despejadas": int(sum(referencia[f]["despejado"] > 0.7 for f in comunes)),
        "r2": 1.0 - float(resid @ resid) / total if total > 0 else 0.0,
        "sesgo_medio": float(y.mean()),
        "sesgo_despejadas": float(np.mean(
            [sitio[f][nivel] - referencia[f]["tmin"] for f in comunes
             if referencia[f]["despejado"] > 0.7] or [float("nan")])),
    }


def proyectar(modelo, referencia, rng, tiradas=2000):
    """Recorre todos los inviernos de la referencia con el ajuste y su incertidumbre."""
    fechas = [f for f in sorted(referencia) if f.month in MESES_FRIOS]
    temporadas = np.array([temporada(f) for f in fechas])
    # Sólo inviernos completos (≥ 140 noches de noviembre a marzo).
    validas = [t for t in sorted(set(temporadas)) if (temporadas == t).sum() >= 140]
    if not validas:
        raise SystemExit("La referencia no contiene ningún invierno completo.")
    X = np.array([rasgos(referencia[f]) for f in fechas])
    tmin = np.array([referencia[f]["tmin"] for f in fechas])

    betas = rng.multivariate_normal(modelo["beta"], modelo["cov"], size=tiradas)
    pred = tmin[None, :] + betas @ X.T
    pred += rng.normal(0.0, modelo["sigma"], size=pred.shape)

    salida = {"inviernos": len(validas), "umbral": {}}
    minimas = np.stack([pred[:, temporadas == t].min(axis=1) for t in validas], axis=1)
    for u in UMBRALES:
        cuenta = np.stack(
            [(pred[:, temporadas == t] < u).sum(axis=1) for t in validas], axis=1)
        salida["umbral"][u] = {
            "noches_por_anio": float(cuenta.mean()),
            "noches_p10": float(np.percentile(cuenta.mean(axis=1), 10)),
            "noches_p90": float(np.percentile(cuenta.mean(axis=1), 90)),
            "prob_anio": float((cuenta > 0).mean()),
        }
    salida["minima_anual_mediana"] = float(np.median(minimas))
    salida["minima_anual_p10"] = float(np.percentile(minimas, 10))
    salida["minima_del_periodo"] = float(np.median(minimas.min(axis=1)))
    # Lo que diría el mapa sin corregir.
    salida["mapa"] = {
        u: float(np.mean([(tmin[temporadas == t] < u).sum() for t in validas]))
        for u in UMBRALES
    }
    return salida


# ----------------------------------------------------------------- informe


def es(v, dec=1):
    return f"{v:.{dec}f}".replace(".", ",").replace("-", "−")


def texto_informe(nombre_nivel, modelo, p):
    L = []
    L.append(f"INFORME DE HELADA · {nombre_nivel}")
    L.append("")
    L.append(f"Calibrado con {modelo['n']} noches medidas "
             f"({modelo['despejadas']} de cielo despejado).")
    L.append(f"  La hondonada marca de media {es(modelo['sesgo_medio'])} °C respecto al mapa;")
    if not math.isnan(modelo["sesgo_despejadas"]):
        L.append(f"  en las noches despejadas, {es(modelo['sesgo_despejadas'])} °C.")
    L.append(f"  El ajuste explica el {es(100 * modelo['r2'], 0)} % de esa diferencia; "
             f"error típico ±{es(modelo['sigma'])} °C.")
    if modelo["n"] < 60 or modelo["despejadas"] < 15:
        L.append("  AVISO: pocas noches para un ajuste sólido. Tómese como orientación.")
    L.append("")
    L.append(f"Proyección sobre {p['inviernos']} inviernos de reanálisis:")
    etiquetas = {1.0: "por debajo de 1 °C (umbral del estudio)",
                 0.0: "de helada (< 0 °C)",
                 -2.0: "por debajo de −2 °C (letal para la pitaya)"}
    for u in UMBRALES:
        d = p["umbral"][u]
        L.append(f"  Noches {etiquetas[u]}:")
        L.append(f"    {es(d['noches_por_anio'])} al año "
                 f"(entre {es(d['noches_p10'])} y {es(d['noches_p90'])}) · "
                 f"ocurre en {es(100 * d['prob_anio'], 0)} de cada 100 inviernos · "
                 f"el mapa sin corregir diría {es(p['mapa'][u])}")
    L.append(f"  Mínima de un invierno normal: {es(p['minima_anual_mediana'])} °C · "
             f"de uno de cada diez: {es(p['minima_anual_p10'])} °C · "
             f"la más baja del periodo: {es(p['minima_del_periodo'])} °C")
    return "\n".join(L)


# ------------------------------------------------------------- autoprueba


def mundo_de_juguete(rng, inviernos):
    """Veinte inviernos inventados con la verdad a la vista.

    Devuelve (referencia, verdad): la referencia es lo que vería el mapa; la
    verdad, la mínima real en la hondonada, con una ley de embalsamiento que NO
    es la del modelo que se ajusta (potencia 1,5 y otra escala de viento), para
    no hacernos trampas.
    """
    referencia, verdad = {}, {}
    for anio in range(2007, 2007 + inviernos):
        anomalia, latente = 0.0, 0.0
        dia = dt.date(anio - 1, 11, 1)
        while dia <= dt.date(anio, 3, 31):
            doy = dia.timetuple().tm_yday
            anomalia = 0.7 * anomalia + math.sqrt(0.51) * 2.5 * rng.normal()
            latente = 0.6 * latente + math.sqrt(0.64) * 0.7 * rng.normal()
            nubes = min(max(0.25 + 0.8 * latente, 0.0), 1.0)
            viento = abs(rng.normal()) * 1.8
            tmin = 10.5 - 4.0 * math.cos(2 * math.pi * (doy - 20) / 365) + anomalia \
                + 2.0 * nubes - 0.6
            embalse = 4.5 * (1 - nubes) ** 1.5 * math.exp(-viento / 1.5)
            referencia[dia] = {"tmin": tmin + 0.4 * rng.normal(),  # el mapa tampoco es perfecto
                               "despejado": 1 - nubes,
                               "calma": math.exp(-viento / 3.0)}
            verdad[dia] = {"suelo": tmin - embalse * math.exp(-0.1 / 1.2),
                           "ref": tmin - embalse * math.exp(-1.0 / 1.2)}
            dia += dt.timedelta(days=1)
    return referencia, verdad


def autoprueba():
    rng = np.random.default_rng(7)

    # 1. Los lectores, con archivos sintéticos en los dos formatos reales.
    import os
    import tempfile
    with tempfile.TemporaryDirectory() as d:
        ruta = os.path.join(d, "2027-01.csv")
        t0 = dt.datetime(2027, 1, 14, 12, 0, tzinfo=dt.timezone.utc)
        with open(ruta, "w", encoding="utf-8") as f:
            f.write("epoch,T010,T050,T100,T200,Taire,HR,Tcielo,mV\n")
            for i in range(144):
                t = t0 + dt.timedelta(minutes=10 * i)
                suelo = 5.0 - 6.0 * math.sin(math.pi * i / 143)  # mínimo −1,0 a media serie
                f.write(f"{int(t.timestamp())},{suelo:.2f},nan,{suelo + 1.5:.2f},"
                        f"{suelo + 3:.2f},{suelo + 1.5:.2f},70,-20,4000\n")
        sitio = leer_sereno([ruta])
        assert list(sitio) == [dt.date(2027, 1, 15)], sitio
        assert abs(sitio[dt.date(2027, 1, 15)]["suelo"] + 1.0) < 0.01
        assert abs(sitio[dt.date(2027, 1, 15)]["ref"] - 0.5) < 0.01
    horas = [dt.datetime(2027, 1, 14, 12) + dt.timedelta(hours=i) for i in range(24)]
    ref = leer_referencia({"hourly": {
        "time": [h.strftime("%Y-%m-%dT%H:%M") for h in horas],
        "temperature_2m": [10 - abs(i - 18) * 0.0 - (8 if 14 <= i <= 20 else 0) for i in range(24)],
        "cloud_cover": [25] * 24,
        "wind_speed_10m": [3.0] * 24,
    }}, lon=-1.7)
    n = ref[dt.date(2027, 1, 15)]
    assert abs(n["tmin"] - 2.0) < 1e-9 and abs(n["despejado"] - 0.75) < 1e-9
    assert abs(n["calma"] - math.exp(-1.0)) < 1e-9
    print("lectores: bien")

    # 2. El método, en bucle cerrado: se calibra con UN invierno y se compara la
    #    proyección a 20 años con la verdad del mundo de juguete.
    referencia, verdad = mundo_de_juguete(rng, 20)
    ultimo = {f: v for f, v in verdad.items() if temporada(f) == 2026}
    # El aparato mide con ±0,1 °C tras el ajuste en hielo.
    medido = {f: {k: v[k] + 0.1 * rng.normal() for k in v} for f, v in ultimo.items()}
    resumen = {}
    for nivel, nombre in (("ref", "a 1 m"), ("suelo", "a ras de suelo")):
        modelo = ajustar(medido, referencia, nivel)
        p = proyectar(modelo, referencia, rng)
        print()
        print(texto_informe(nombre, modelo, p))
        reales = {u: np.mean([sum(1 for f, v in verdad.items()
                                  if temporada(f) == t and v[nivel] < u)
                              for t in range(2007, 2027)]) for u in UMBRALES}
        print("  VERDAD del mundo de juguete (noches/año): " +
              " · ".join(f"<{es(u, 0)} °C: {es(reales[u])}" for u in UMBRALES))
        resumen[nivel] = {
            "n": modelo["n"], "sigma": modelo["sigma"],
            "estimado": {str(u): p["umbral"][u]["noches_por_anio"] for u in UMBRALES},
            "mapa": {str(u): p["mapa"][u] for u in UMBRALES},
            "verdad": {str(u): float(reales[u]) for u in UMBRALES},
        }
        for u in (1.0, 0.0):
            est = p["umbral"][u]["noches_por_anio"]
            # La estimación debe acercarse a la verdad mucho más que el mapa.
            assert abs(est - reales[u]) < max(0.6 * reales[u], 0.5), (nivel, u, est, reales[u])
            assert abs(est - reales[u]) < abs(p["mapa"][u] - reales[u]), (nivel, u)
    print("\nbucle cerrado: bien")
    return resumen


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--datos", nargs="+", help="CSV de la libreta de Sereno")
    ap.add_argument("--referencia", help="JSON horario de Open-Meteo ya descargado")
    ap.add_argument("--lat", type=float)
    ap.add_argument("--lon", type=float)
    ap.add_argument("--anios", type=int, default=20, help="inviernos de referencia")
    ap.add_argument("--autoprueba", action="store_true")
    ap.add_argument("--json", help="guardar el resultado de la autoprueba en este archivo")
    a = ap.parse_args()

    if a.autoprueba:
        resumen = autoprueba()
        if a.json:
            with open(a.json, "w", encoding="utf-8") as f:
                json.dump(resumen, f, indent=1)
        return
    if not a.datos or a.lon is None:
        ap.error("hacen falta --datos y --lon (y --lat o --referencia)")

    sitio = leer_sereno(a.datos)
    if not sitio:
        raise SystemExit("No hay noches completas en los CSV.")
    if a.referencia:
        with open(a.referencia, encoding="utf-8") as f:
            crudo = json.load(f)
    else:
        if a.lat is None:
            ap.error("sin --referencia hace falta --lat para descargarla")
        hasta = max(sitio) + dt.timedelta(days=1)
        desde = dt.date(hasta.year - a.anios, 10, 1)
        print(f"Descargando reanálisis {desde} → {hasta}…", file=sys.stderr)
        crudo = descargar_referencia(a.lat, a.lon, desde, hasta)
    referencia = leer_referencia(crudo, a.lon)

    rng = np.random.default_rng(1)
    for nivel, nombre in (("ref", "a 1 m"), ("suelo", "a ras de suelo")):
        modelo = ajustar(sitio, referencia, nivel)
        print(texto_informe(nombre, modelo, proyectar(modelo, referencia, rng)))
        print()


if __name__ == "__main__":
    main()
