#include "guardia.h"

#include <cmath>

#include "cante.h"
#include "sol.h"

namespace sereno {

bool nocheQueContiene(time_t t, double latitud, double longitud, int margenMin,
                      time_t* inicio, time_t* fin) {
  // El día civil UTC no coincide con el día solar lejos de Greenwich: se miran
  // los días vecinos y se elige el último ocaso ya pasado.
  time_t ocaso = 0;
  bool hayOcaso = false;
  for (int d = -1; d <= 1; ++d) {
    const SolDia s = solDelDia(t + d * 86400, latitud, longitud);
    if (!s.valido) continue;
    if (s.ocaso <= t && (!hayOcaso || s.ocaso > ocaso)) {
      ocaso = s.ocaso;
      hayOcaso = true;
    }
  }
  if (!hayOcaso) return false;

  time_t orto = 0;
  bool hayOrto = false;
  for (int d = -1; d <= 2; ++d) {
    const SolDia s = solDelDia(t + d * 86400, latitud, longitud);
    if (!s.valido) continue;
    if (s.orto > ocaso && (!hayOrto || s.orto < orto)) {
      orto = s.orto;
      hayOrto = true;
    }
  }
  if (!hayOrto) return false;

  const time_t limite = orto + margenMin * 60;
  if (t > limite) return false;
  if (inicio) *inicio = ocaso;
  if (fin) *fin = limite;
  return true;
}

bool mesEntre(int mes, int desde, int hasta) {
  if (desde <= hasta) return mes >= desde && mes <= hasta;
  return mes >= desde || mes <= hasta;
}

bool Guardia::restaurar(const Persistente& p) {
  if (p.magia != kMagia) return false;
  p_ = p;
  return true;
}

void Guardia::empezarNoche(time_t inicio, time_t fin) {
  Noche n;
  p_.noche = n.estado();
  p_.inicioNoche = static_cast<int64_t>(inicio);
  p_.finNoche = static_cast<int64_t>(fin);
  p_.enNoche = 1;
  p_.t0Tomado = 0;
  p_.avisoHelada = 0;
  p_.fotoHecha = 0;
  p_.T0 = 0;
  p_.Td0 = 0;
}

void Guardia::cerrarNoche(time_t ahora, Salida* s) {
  Noche n;
  n.restaurar(p_.noche);
  const ResumenNoche& r = n.resumen();
  p_.enNoche = 0;
  if (r.muestras == 0) return;

  p_.temporada.cerrarNoche(r);

  // Sólo las noches de helada de radiación enseñan algo al pronóstico.
  if (p_.t0Tomado && r.radiativa() && !std::isnan(r.minima[kNivelRef])) {
    PronosticoMinima pr;
    pr.restaurar(p_.pronostico);
    pr.entrenar(p_.T0, p_.Td0, r.minima[kNivelRef]);
    p_.pronostico = pr.estado();
  }

  // Parte diario en la temporada fría; fuera de ella, uno a la semana para
  // saber que sigue de guardia, o siempre que haya helado.
  struct tm l;
  localtime_r(&ahora, &l);
  const bool temporadaFria =
      mesEntre(l.tm_mon + 1, c_.mesInicioFrio, c_.mesFinFrio);
  if (temporadaFria || l.tm_wday == 0 || r.helo()) {
    s->mensajes.push_back(canteAmanecer(ahora, r, p_.temporada));
  }
  s->cierreNoche = true;
}

Salida Guardia::muestra(const Muestra& m) {
  Salida s;
  struct tm l;
  localtime_r(&m.t, &l);
  const int mes = l.tm_mon + 1;

  // La temporada de frío va de octubre a septiembre: al estrenar una, a cero.
  const int anioTemporada =
      (mes >= c_.mesInicioFrio) ? l.tm_year + 1900 : l.tm_year + 1899;
  if (p_.anioTemporada != anioTemporada) {
    p_.temporada = Temporada();
    p_.anioTemporada = anioTemporada;
  }
  if (mesEntre(mes, c_.mesInicioFrio, c_.mesFinFrio)) {
    p_.temporada.anadirMuestra(m, c_.minutosMuestra);
  }

  // Una noche que ya terminó (o que quedó colgada tras un apagón) se cierra.
  if (p_.enNoche && m.t > static_cast<time_t>(p_.finNoche)) {
    cerrarNoche(m.t, &s);
  }

  time_t inicio = 0, fin = 0;
  const bool deNoche = nocheQueContiene(m.t, c_.latitud, c_.longitud,
                                        c_.margenOrtoMin, &inicio, &fin);
  if (!deNoche) return s;
  if (!p_.enNoche) empezarNoche(inicio, fin);

  Noche n;
  n.restaurar(p_.noche);
  n.anadir(m, c_.minutosMuestra, c_.cielo);
  p_.noche = n.estado();

  const float aire = std::isnan(m.Taire) ? m.T[kNivelRef] : m.Taire;

  // Dos horas tras el ocaso: la foto fija de la que sale el pronóstico.
  const time_t horaT0 = static_cast<time_t>(p_.inicioNoche) + 2 * 3600;
  const time_t ventana = static_cast<time_t>(2 * c_.minutosMuestra * 60);
  if (!p_.t0Tomado && m.t >= horaT0 && m.t < horaT0 + ventana) {
    const float td = puntoRocio(aire, m.HR);
    if (!std::isnan(aire) && !std::isnan(td)) {
      p_.T0 = aire;
      p_.Td0 = td;
      p_.t0Tomado = 1;
      PronosticoMinima pr;
      pr.restaurar(p_.pronostico);
      float tmin = 0, error = 0;
      const bool sereno =
          clasificarCielo(m.Tcielo, aire, c_.cielo) == Cielo::Sereno;
      if (sereno && pr.predecir(p_.T0, p_.Td0, &tmin, &error) &&
          tmin < c_.umbralAvisoC) {
        const time_t orto =
            static_cast<time_t>(p_.finNoche) - c_.margenOrtoMin * 60;
        s.mensajes.push_back(cantePronostico(m.t, tmin, error, orto));
        s.urgente = true;
      }
    }
  }

  // Alarma: la primera sonda que cruza el cero.
  if (!p_.avisoHelada) {
    for (int i = 0; i < kNiveles; ++i) {
      if (!std::isnan(m.T[i]) && m.T[i] < 0.0f) {
        s.mensajes.push_back(canteHelando(m.t, m.T[i], i));
        s.urgente = true;
        p_.avisoHelada = 1;
        break;
      }
    }
  }

  // La foto de las flores, una vez por noche y en temporada.
  if (!p_.fotoHecha && mesEntre(mes, c_.mesInicioFlor, c_.mesFinFlor) &&
      l.tm_hour >= c_.horaFotoLocal && l.tm_hour < c_.horaFotoLocal + 3) {
    s.pedirFoto = true;
    p_.fotoHecha = 1;
  }

  return s;
}

std::string Guardia::flores(time_t ahora, int vistas) {
  if (vistas <= 0) return std::string();
  const EstimacionFlor e = estimarFinca(vistas, ahora, c_.parcela);
  CalendarioCosecha cal;
  cal.restaurar(p_.cosecha);
  cal.anotar(ahora, e.finca);
  p_.cosecha = cal.estado();
  return canteFlor(ahora, e, c_.parcela);
}

}  // namespace sereno
