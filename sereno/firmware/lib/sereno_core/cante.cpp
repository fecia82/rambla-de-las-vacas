#include "cante.h"

#include <cmath>
#include <cstdio>

namespace sereno {

namespace {

const char* const kHoras[12] = {"doce", "una",  "dos",  "tres", "cuatro", "cinco",
                                "seis", "siete", "ocho", "nueve", "diez",  "once"};
const char* const kMeses[12] = {"enero",      "febrero", "marzo",     "abril",
                                "mayo",       "junio",   "julio",     "agosto",
                                "septiembre", "octubre", "noviembre", "diciembre"};

struct tm local(time_t t) {
  struct tm l;
  localtime_r(&t, &l);
  return l;
}

std::string temp(float v) {
  return std::isnan(v) ? std::string("—") : num(v, 1);
}

}  // namespace

std::string num(float v, int decimales) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%.*f", decimales, static_cast<double>(v));
  std::string s(buf);
  // «-0,0» no es un número que nadie quiera leer.
  bool todoCeros = true;
  for (char c : s) {
    if (c != '-' && c != '0' && c != '.') todoCeros = false;
  }
  const bool negativo = !s.empty() && s[0] == '-';
  if (negativo) s.erase(0, 1);
  for (char& c : s) {
    if (c == '.') c = ',';
  }
  return (negativo && !todoCeros) ? "−" + s : s;
}

std::string miles(long v) {
  const bool negativo = v < 0;
  if (negativo) v = -v;
  std::string digitos = std::to_string(v);
  std::string s;
  int cuenta = 0;
  for (int i = static_cast<int>(digitos.size()) - 1; i >= 0; --i) {
    if (cuenta > 0 && cuenta % 3 == 0) s.insert(s.begin(), '.');
    s.insert(s.begin(), digitos[i]);
    ++cuenta;
  }
  return negativo ? "−" + s : s;
}

std::string horaCantada(int hora, int minuto) {
  int cuarto = (minuto + 7) / 15;  // redondeo al cuarto de hora más cercano
  if (cuarto >= 3) hora += 1;      // «menos cuarto» y «en punto» de la siguiente
  if (cuarto == 4) cuarto = 0;
  const int h = ((hora % 12) + 12) % 12;
  std::string s = (h == 1) ? "La " : "Las ";
  s += kHoras[h];
  if (cuarto == 1) s += " y cuarto";
  if (cuarto == 2) s += " y media";
  if (cuarto == 3) s += " menos cuarto";
  return s;
}

std::string horaReloj(time_t t) {
  const struct tm l = local(t);
  char buf[8];
  snprintf(buf, sizeof(buf), "%02d:%02d", l.tm_hour, l.tm_min);
  return buf;
}

std::string fechaCorta(time_t t) {
  const struct tm l = local(t);
  return std::to_string(l.tm_mday) + " de " + kMeses[l.tm_mon];
}

std::string duracion(float horas) {
  int minutos = static_cast<int>(std::lround(horas * 60.0f / 5.0f)) * 5;
  if (minutos < 5) minutos = 5;
  const int h = minutos / 60, m = minutos % 60;
  if (h == 0) return std::to_string(m) + " min";
  if (m == 0) return std::to_string(h) + " h";
  return std::to_string(h) + " h " + std::to_string(m) + " min";
}

int redondoHablado(int v) {
  if (v < 20) return v;
  const int paso = v < 100 ? 5 : (v < 1000 ? 10 : (v < 5000 ? 50 : 100));
  return ((v + paso / 2) / paso) * paso;
}

const char* nombreNivel(int nivel) {
  switch (nivel) {
    case 0: return "a ras de suelo";
    case 1: return "a medio metro";
    case 2: return "a 1 m";
    case 3: return "a 2 m";
    default: return "";
  }
}

std::string canteAmanecer(time_t ahora, const ResumenNoche& r,
                          const Temporada& t) {
  const struct tm l = local(ahora);
  std::string s = "¡" + horaCantada(l.tm_hour, l.tm_min) + " y ";
  if (r.helo()) {
    s += "helando";
  } else if (r.muestrasCielo == 0) {
    s += "sin novedad";
  } else if (r.fraccionSerena >= 0.7f) {
    s += "sereno";
  } else if (r.fraccionSerena <= 0.3f) {
    s += "nublado";
  } else {
    s += "con nubes";
  }
  s += "!\n";

  int nivel = -1;
  const float minima = r.minimaAbsoluta(&nivel);
  if (nivel < 0) {
    s += "Esta noche no hubo lecturas válidas: revisa las sondas.\n";
  } else {
    s += "Mínima de la noche: " + num(minima, 1) + " °C " + nombreNivel(nivel) +
         " (" + horaReloj(r.horaMinima[nivel]) + ").\n";
    s += "Perfil: " + temp(r.minima[0]) + " · " + temp(r.minima[1]) + " · " +
         temp(r.minima[2]) + " · " + temp(r.minima[3]) +
         " °C (a 0,1 · 0,5 · 1 · 2 m).\n";
    if (r.inversionMax >= 1.0f) {
      s += "Aire frío embalsado: hasta " + num(r.inversionMax, 1) +
           " °C más frío abajo que a 2 m.\n";
    } else {
      s += "Sin aire frío embalsado.\n";
    }
    if (r.helo()) {
      s += "Helada: " +
           std::to_string(static_cast<int>(
               std::lround(r.minutosBajo[kNivelSuelo][kUmbralHelada]))) +
           " min bajo cero a ras de suelo, " +
           std::to_string(static_cast<int>(
               std::lround(r.minutosBajo[kNivelRef][kUmbralHelada]))) +
           " a 1 m.\n";
    } else if (!std::isnan(r.minima[kNivelRef]) &&
               r.minima[kNivelRef] < kUmbralesC[kUmbralDecision]) {
      s += "Sin helada, pero a 1 m bajó de 1 °C.\n";
    } else {
      s += "Sin helada.\n";
    }
  }

  s += "Temporada: " + std::to_string(t.noches) +
       (t.noches == 1 ? " noche" : " noches") + " · " +
       std::to_string(t.nochesHelada) + " de helada";
  if (t.minimaRef < 999.0f) {
    s += " · mínima a 1 m " + num(t.minimaRef, 1) + " °C";
  }
  s += " · " + miles(std::lround(t.horasFrio)) + " horas frío.";

  switch (t.veredicto()) {
    case Veredicto::InvernaderoObligatorio:
      s += "\nVeredicto provisional: invernadero obligatorio (1 m bajó de 1 °C).";
      break;
    case Veredicto::MallaPosible:
      s += "\nVeredicto provisional: la malla sigue siendo posible.";
      break;
    case Veredicto::SinDatos:
      break;
  }
  return s;
}

std::string cantePronostico(time_t ahora, float TminRef, float errorTipico,
                            time_t orto) {
  std::string s = "Aviso de las " + horaReloj(ahora) +
                  ". Noche serena: mínima prevista a 1 m de " +
                  num(TminRef, 1) + " °C (±" + num(errorTipico, 1) +
                  "), hacia las " + horaReloj(orto) + ".";
  if (TminRef < 0.0f) {
    s += " Helada probable.";
  } else if (TminRef - errorTipico < 0.0f) {
    s += " Helada posible.";
  } else if (TminRef < kUmbralesC[kUmbralDecision]) {
    s += " Por debajo del umbral de 1 °C.";
  }
  return s;
}

std::string canteHelando(time_t ahora, float T, int nivel) {
  return "¡Helando! " + num(T, 1) + " °C " + nombreNivel(nivel) + " a las " +
         horaReloj(ahora) + ".";
}

std::string canteFlor(time_t ahora, const EstimacionFlor& e,
                      const ParcelaFlor& p) {
  const struct tm l = local(ahora);
  std::string s = "¡" + horaCantada(l.tm_hour, l.tm_min) + " y ";
  if (e.vistas == 0) {
    return s + "sin flores!";
  }
  s += "floreciendo!\n";
  s += std::to_string(e.vistas) +
       (e.vistas == 1 ? " flor abierta" : " flores abiertas") + " en las " +
       std::to_string(p.plantasCentinela) + " plantas centinela: unas " +
       miles(redondoHablado(e.finca)) + " en la finca (entre " +
       miles(redondoHablado(e.fincaMin)) + " y " +
       miles(redondoHablado(e.fincaMax)) + ").\n";
  s += "Mañana toca pincel: " + duracion(e.horasPincel) + ".\n";
  s += "Si cuajan, unos " + miles(redondoHablado(static_cast<int>(std::lround(e.kilos)))) +
       " kg entre el " + fechaCorta(e.cosechaDesde) + " y el " +
       fechaCorta(e.cosechaHasta) + ".";
  return s;
}

}  // namespace sereno
