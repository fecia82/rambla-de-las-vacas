// =====================================================================
//  SERENO — piezas imprimibles del chuzo de vela nocturna
// ---------------------------------------------------------------------
//  Un chuzo (poste Ø25–50 mm, ~2,3 m sobre el suelo, NO se imprime) lleva:
//    · 4 sondas de temperatura a 0,10 / 0,50 / 1,00 / 2,00 m, cada una
//      dentro de una "pagoda" (abrigo antirradiación tipo Gill),
//    · arriba un "farol": cuerpo con la electrónica, cámara + LED de
//      flash mirando en horizontal, sensor infrarrojo de cielo en la
//      chimenea y panel solar sobre el tejado a un agua.
//
//  Piezas imprimibles (variable `pieza`):
//    farol_cuerpo · farol_tejado · anillo_cielo · pagoda
//  Vistas (no se imprimen):
//    conjunto · cabeza · despiece · cabeza_corte · pagoda_montada · pagoda_corte
//
//  Exportar:  openscad -o ../stl/pagoda.stl -D 'pieza="pagoda"' sereno.scad
//
//  Unidades: mm.  Boquilla 0,4 · PETG o ASA · sin soportes.
//  NADA de esto se ha impreso todavía: comprobar medidas antes de imprimir.
// =====================================================================

/* [Pieza a generar] */
// farol_cuerpo | farol_tejado | anillo_cielo | pagoda | conjunto | cabeza | despiece | cabeza_corte | pagoda_montada | pagoda_corte
pieza = "conjunto";
// Giro del tejado sobre el cuerpo en pasos de 90° (0 = alero bajo sobre la ventana)
orientacion_tejado = 0;

/* [Impresión FDM] */
// Holgura de ajuste entre piezas
holgura = 0.3;
// Pared mínima (6 perímetros de 0,4)
pared = 2.4;
// Resolución de curvas (≤ 64)
$fn = 48;
// Resolución de taladros pequeños
fn_fino = 24;

/* [Placa LilyGO T-SIM7080G-S3 — SIN VERIFICAR: MEDIR TU PLACA] */
// MEDIR TU PLACA: largo de la envolvente (va en vertical)
placa_largo = 112;
// MEDIR TU PLACA: ancho de la envolvente (= ancho de la PCB)
placa_ancho = 38;
// MEDIR TU PLACA: grosor total con portapilas 18650
placa_grosor = 26;
// MEDIR TU PLACA: grosor de la PCB
pcb_grosor = 1.6;
// MEDIR TU PLACA: de la cara trasera de la envolvente (portapilas) a la cara trasera de la PCB
pcb_a_trasera = 21;

/* [Farol: cuerpo] */
// Lado exterior (planta cuadrada, para poder girar el tejado)
cuerpo_lado = 60;
// Radio de las aristas verticales
cuerpo_radio = 5;
// Grosor del suelo
suelo = 3;
// Pared frontal (= rebaje del cristal + pared mínima)
pared_frontal = 4.0;
// Hueco libre bajo la placa para cables
hueco_cables = 8;
// Margen libre sobre la placa
margen_sup = 7;
// Chaflán de la base (pata de elefante)
chaflan_base = 0.8;
// Zócalo del farol: vuelo (0 = sin zócalo)
zocalo = 3;
// Zócalo: altura del tramo recto (encima lleva un chaflán a 45°)
zocalo_alto = 4;
// Chaflán del borde superior (asiento cónico del tejado)
chaflan_borde = 1.6;
// Profundidad de la ranura de los carriles (lo que entra el canto de la PCB)
carril_fondo = 2.0;
// Grosor de cada labio del carril
carril_labio = 2.0;

/* [Farol: ventana (cámara + LED)] */
// Rebaje para el cristal de metacrilato: ancho
vent_ancho = 44;
// Rebaje: alto
vent_alto = 40;
// Rebaje: profundidad
vent_fondo = 1.6;
// Abertura de la cámara (objetivo OV2640)
cam_d = 8.5;
// Posición X de la cámara respecto al eje del cuerpo
cam_x = -12.5;
// Cuna interior en U para el módulo de cámara (SIN VERIFICAR)
cam_cuna = true;
// Lado del cuerpo del módulo OV2640 (SIN VERIFICAR)
cam_modulo = 8.6;
// Abertura del LED
led_d = 14;
// Posición X del LED
led_x = 8.5;
// Lado del asiento plano para la PCB "estrella" de 20 mm
led_asiento = 26;
// Resalte del asiento hacia el interior
led_resalte = 3;
// Separación entre los dos tornillos M3 de la estrella
led_torn_sep = 19;

/* [Farol: fondo] */
// Taladro del prensaestopas PG7
pg7_d = 12.5;
// Posición Y del prensaestopas (negativo = hacia la ventana)
pg7_y = -13;
// Drenajes / respiraderos
dren_d = 3;

/* [Farol: silla en V trasera] */
// Ancho de la V (admite chuzo redondo Ø25–50 o cuadrado)
silla_v_ancho = 34;
// Labio plano a cada lado de la V
silla_labio = 3;
// Ángulo interior de la V
silla_ang = 120;
// Alma entre el fondo de la V y el paso de la brida
silla_alma = 2;
// Paso para abrazadera o brida: ancho (vertical)
brida_ancho = 9;
// Paso para abrazadera o brida: grosor
brida_grosor = 3.5;

/* [Farol: unión cuerpo–tejado] */
// Altura que el faldón del tejado abraza del cuerpo
faldon = 12;
// Distancia de los tornillos por debajo del borde superior del cuerpo
torn_bajo_borde = 6;
// Taladro piloto M3 (rosca directa en plástico)
m3_piloto = 2.5;
// Taladro pasante M3
m3_paso = 3.4;
// Cajera para la cabeza del tornillo M3
m3_cabeza = 6.5;
// Tetón interior de refuerzo de cada piloto [ancho, alto, resalte]
teton = [10, 8, 3.6];

/* [Farol: tejado] */
// Inclinación del tejado a un agua (grados)
inclinacion = 40;
// Panel solar [largo, ancho, grosor]
panel = [110, 60, 3];
// Profundidad del rebaje del panel
panel_rebaje = 1.5;
// Holgura del panel por lado
panel_holg = 0.3;
// Marco alrededor del panel
panel_marco = 3.2;
// Taladro del cable del panel
panel_cable_d = 5;
// Largo en planta del tejado en la dirección de la pendiente
tej_largo = 78;
// Canto vertical del alero bajo
alero_canto = 3;
// Pared de la campana del tejado
pared_tejado = 2.4;
// Grosor mínimo (vertical) de la losa en la cumbrera
techo_min = 3.2;
// Ángulo del techo interior (45 = imprimible sin soportes; = inclinacion para losa uniforme)
techo_int_ang = 45;
// Radio de las esquinas del alero
tej_radio = 2;

/* [Farol: chimenea del sensor de cielo (MLX90614, TO-39)] */
// Taladro interior para la cápsula TO-39
cielo_d = 9.6;
// Labio superior que retiene la cápsula y apoya el film (radial y en altura, a 45°)
labio = 1.0;
// Tramo recto del labio
labio_plano = 0.4;
// Diámetro exterior del fuste
chim_d = 17;
// Diámetro del cuello donde encaja el anillo
cuello_d = 13;
// Altura del cuello
cuello_h = 3;
// Diámetro del goterón
goteron_d = 21;
// Canto del goterón
goteron_h = 1.2;
// Altura de la boca sobre la cumbrera
chim_sobre = 8.5;

/* [Anillo de cielo] */
// Holgura radial anillo–cuello (el film ocupa el resto)
anillo_holg = 0.15;
anillo_pared = 2.4;
anillo_h = 3;

/* [Pagoda (abrigo antirradiación)] */
// Diámetro exterior
pag_d = 76;
// Número de lamas cónicas (además de la tapa)
pag_lamas = 5;
// Ángulo de las lamas (45 = sin soportes)
pag_ang = 45;
// Grosor de lama (perpendicular)
lama_grosor = 1.2;
// Diámetro interior de la jaula
jaula_d = 16;
// Altura interior de la jaula
jaula_alto = 45;
// Pared de la jaula
jaula_pared = 2;
// Ancho de las ventanas de la jaula (dintel en punta a 45°)
jaula_vent = 12.5;
// Grosor de la tapa y de los anillos de arranque
tapa_grosor = 2;
// Número de nervios verticales
nervio_n = 3;
nervio_grosor = 2.4;
// Lo que sobresale el nervio de la jaula
nervio_radial = 3.5;
// Taladro del collarín para la sonda DS18B20 Ø6
collar_d = 6.6;
collar_ext = 11;
collar_alto = 8;
// Ranura para pasar de canto una plaquita SHT4x de 13 × 13 [largo, ancho]
ranura_sht = [13.6, 3.4];
// Largo libre del brazo (del borde de la pagoda a la silla)
brazo_largo = 50;
brazo_ancho = 16;
brazo_grosor = 3;
// Altura del alma del brazo
brazo_alma = 14;
// Silla en V de la pagoda
psilla_v_ancho = 28;
psilla_labio = 2;
psilla_alto = 30;
// Ranuras para bridas [ancho, grosor]
pbrida = [4.5, 2];

/* [Conjunto] */
estaca_d = 32;
estaca_alto = 2300;
// Alturas de las sondas sobre el suelo
sondas = [100, 500, 1000, 2000];
// Lo que queda el extremo del chuzo por debajo del tope de la silla del farol
estaca_bajo_silla = 5;

/* [Colores de las vistas] */
col_cuerpo = [0.91, 0.87, 0.76];
col_tejado = [0.71, 0.33, 0.23];
col_panel = [0.06, 0.14, 0.31];
col_anillo = [0.94, 0.69, 0.19];
col_estaca = [0.54, 0.42, 0.29];
col_pagoda = [0.80, 0.83, 0.85];
col_placa = [0.18, 0.56, 0.36];
col_suelo = [0.73, 0.68, 0.59];
col_rotulo = [0.15, 0.15, 0.15];
// Rótulos de alturas en la vista de conjunto
rotulos = true;
// Ángulos de la cámara (los mismos de --camera) para encarar los rótulos
vista_rx = 80;
vista_rz = 325;

/* [Oculto] */
eps = 0.01;
G = 600; // tamaño de los semiespacios de corte
// Solo para las imágenes: dibuja la vista reflejada en X. La imagen se voltea
// después en horizontal, con lo que la geometría sale correcta y la luz fija
// de OpenSCAD (arriba a la izquierda) pasa a venir de arriba a la derecha.
espejo = false;

// =====================================================================
//  Medidas derivadas
// =====================================================================
cuerpo_alto = suelo + hueco_cables + placa_largo + margen_sup;
int_x = cuerpo_lado - 2*pared;
int_y = cuerpo_lado - pared_frontal - pared;
x_int = int_x/2;                                  // cara interior lateral
y_int_frente = -cuerpo_lado/2 + pared_frontal;    // cara interior frontal
y_int_tras = cuerpo_lado/2 - pared;               // cara interior trasera
vent_z = 0.6*cuerpo_alto;                         // altura del centro de la ventana
// la placa se separa de la pared trasera lo justo para librar el tetón al entrar
placa_y_tras = y_int_tras - teton[2] - 0.5;       // cara trasera de la envolvente
placa_y_frente = placa_y_tras - placa_grosor;
pcb_y_tras = placa_y_tras - pcb_a_trasera;        // cara trasera de la PCB
carril_z_fin = cuerpo_alto - faldon - 6;
led_piloto_prof = led_resalte + (pared_frontal - vent_fondo) - 1.0;

silla_alto = cuerpo_alto - faldon - 2;
silla_v_fondo = (silla_v_ancho/2)/tan(silla_ang/2);
brida_sep = 0.4;                                   // piel extra entre la pared y el paso de brida
silla_vertice = brida_sep + brida_grosor + silla_alma;      // del dorso del cuerpo al vértice de la V
silla_fondo = silla_vertice + silla_v_fondo;
brida_z = [22, silla_alto - 22];
// eje del chuzo respecto al centro del cuerpo (chuzo redondo apoyado en la V)
function y_estaca(d) = cuerpo_lado/2 + silla_vertice + (d/2)/sin(silla_ang/2);

// --- tejado (origen: centro, z = 0 en el canto inferior del faldón) ---
tej_boca = cuerpo_lado + 2*holgura;                // lado interior del faldón
tej_base = tej_boca + 2*pared_tejado;              // lado exterior del faldón
tej_ancho = panel[0] + 2*panel_holg + 2*panel_marco;
z_alero = (tej_ancho - tej_base)/2;                // la campana abre a 45° hacia los lados
z_bajo = z_alero + alero_canto;                    // arranque de la pendiente
function z_tej(y) = z_bajo + (y + tej_largo/2)*tan(inclinacion);
z_cumbrera = z_tej(tej_largo/2);
tej_a = pared_tejado/sin(45);                                          // desfase horizontal lateral
tej_b = pared_tejado/sin(atan2(z_alero, (tej_largo - tej_base)/2));    // desfase frontal/trasero
z_asiento = faldon - chaflan_borde;                // arranque del cono de asiento
asiento_n = (tej_boca - int_x)/2;                  // lo que estrecha el cono de asiento
z_desvan = z_asiento + asiento_n + 1.2;            // suelo del desván
y_techo = tej_largo/2 - tej_b;                     // pared interior alta
z_techo = z_tej(y_techo) - techo_min;              // techo interior en la pared alta
chim_y = tej_largo/2 - chim_d/2 - 0.5;
z_chim = z_cumbrera + chim_sobre;                  // boca de la chimenea
z_cuello = z_chim - cuello_h;                      // hombro donde apoya el anillo
z_goteron = z_cuello - goteron_h - (goteron_d - chim_d)/2;
z_chim0 = z_tej(chim_y - chim_d/2) - 5;
panel_s = panel_marco + panel_holg;                // arranque del panel a lo largo de la pendiente
torn_z = faldon - torn_bajo_borde;                 // altura de los tornillos en el tejado

// --- pagoda (orientación de impresión: tapa abajo) ---
pag_R = pag_d/2;
r_j = jaula_d/2;
r_a = r_j + jaula_pared;                           // radio de arranque de las lamas
pag_k = tan(pag_ang);
lama_v = lama_grosor/cos(pag_ang);                 // grosor vertical de la lama
pag_paso = (jaula_alto + tapa_grosor)/pag_lamas;
lama_sube = (pag_R - r_a)*pag_k;
z_fondo = pag_lamas*pag_paso;                      // arranque de la última lama = fondo de la jaula
pag_alto = z_fondo + lama_sube + lama_v;
r_ce = collar_ext/2;
z_collar = z_fondo + tapa_grosor + (r_j - r_ce) + collar_alto;
r_n = r_a + nervio_radial;
psilla_x = pag_R + brazo_largo;                    // dorso de la silla
psilla_v_fondo = (psilla_v_ancho/2)/tan(silla_ang/2);
psilla_vertice = pared + pbrida[1] + silla_alma;
psilla_fondo = psilla_vertice + psilla_v_fondo;
function x_estaca_pag(d) = psilla_x + psilla_vertice + (d/2)/sin(silla_ang/2);
pag_z_sensor = pag_alto - (tapa_grosor + jaula_alto/2);   // centro de la jaula, ya montada

// --- comprobaciones ---
assert(int_x >= placa_ancho + 2*holgura + 1, "La placa no cabe a lo ancho: aumenta cuerpo_lado");
assert(placa_y_frente - y_int_frente >= led_resalte + 12, "Falta fondo delante de la placa para LED y cámara");
assert(pared_frontal - vent_fondo >= pared - eps, "La pared tras el cristal queda por debajo de la pared mínima");
assert(z_goteron > z_tej(chim_y + chim_d/2), "El goterón de la chimenea queda enterrado en el tejado: sube chim_sobre");
assert(tej_largo >= tej_base + 8, "El tejado no vuela sobre el cuerpo: aumenta tej_largo");
assert(tapa_grosor > lama_v, "tapa_grosor debe superar el grosor vertical de la lama");

// =====================================================================
//  Utilidades
// =====================================================================

// Prisma de planta rectangular l × a con esquinas de radio r, centrado en XY
module caja_red(l, a, r, h) {
    rr = max(r, 0.05);
    hull() for (sx = [-1, 1], sy = [-1, 1])
        translate([sx*(l/2 - rr), sy*(a/2 - rr), 0]) cylinder(r = rr, h = h);
}

// Igual, con chaflán inferior ci y superior cs
module prisma_chaflan(l, a, r, h, ci = 0, cs = 0) {
    hull() {
        caja_red(l - 2*ci, a - 2*ci, r - ci, eps);
        translate([0, 0, ci]) caja_red(l, a, r, h - ci - cs);
        translate([0, 0, h - eps]) caja_red(l - 2*cs, a - 2*cs, r - cs, eps);
    }
}

// Semiespacio por debajo de un plano que pasa por [·, y0, z0] y sube `ang` grados hacia +Y
module bajo_plano(y0, z0, ang) {
    translate([0, y0, z0]) rotate([ang, 0, 0]) translate([-G/2, -G/2, -G]) cube(G);
}

// Resalte sobre una pared vertical con la cara inferior a 45° (sin soportes).
// Crece hacia +Y desde y = 0, centrado en X, de z = 0 a h (más la cuña inferior).
module resalte45(w, h, t, solape = 0.5) {
    translate([-w/2, 0, 0]) rotate([90, 0, 90]) linear_extrude(w)
        polygon([[-solape, -t - solape], [t, 0], [t, h], [-solape, h]]);
}

// Perfil de silla en V (planta): dorso en y = -solape, V abierta hacia +Y
module perfil_silla(v_ancho, lab, vertice, v_fondo, solape = 1) {
    w = v_ancho + 2*lab;
    f = vertice + v_fondo;
    polygon([[-w/2, -solape], [w/2, -solape], [w/2, f], [v_ancho/2, f],
             [0, vertice], [-v_ancho/2, f], [-w/2, f]]);
}

// Sistema de coordenadas apoyado en la pendiente del tejado:
// X a lo ancho, Y pendiente arriba desde el alero bajo, Z normal al faldón
module en_pendiente() {
    translate([0, -tej_largo/2, z_bajo]) rotate([inclinacion, 0, 0]) children();
}

// =====================================================================
//  1 · FAROL: CUERPO   (se imprime tal cual, de pie, boca arriba)
// =====================================================================
module farol_cuerpo() {
    difference() {
        union() {
            difference() {
                union() {
                    prisma_chaflan(cuerpo_lado, cuerpo_lado, cuerpo_radio, cuerpo_alto,
                                   chaflan_base, chaflan_borde);
                    if (zocalo > 0) zocalo_farol();
                }
                translate([0, (pared_frontal - pared)/2, suelo])
                    caja_red(int_x, int_y, cuerpo_radio - pared, cuerpo_alto);
            }
            carriles();
            asiento_led();
            if (cam_cuna) cuna_camara();
            tetones();
            silla_farol();
        }
        union() {
            ventana();
            taladros_fondo();
            pilotos_tejado();
            pasos_brida();
        }
    }
}

// Zócalo: base algo más ancha rematada a 45° (lee como pie de farol y agarra mejor a la cama)
module zocalo_farol() {
    l = cuerpo_lado + 2*zocalo;
    hull() {
        prisma_chaflan(l, l, cuerpo_radio + zocalo, zocalo_alto, chaflan_base, 0);
        translate([0, 0, zocalo_alto + zocalo + 0.5])
            caja_red(cuerpo_lado - 1, cuerpo_lado - 1, cuerpo_radio - 0.5, eps);
    }
}

// Dos carriles verticales: la PCB baja de canto por las ranuras y descansa
// en el tope macizo, dejando `hueco_cables` libre por debajo.
module carriles() {
    x_ran = placa_ancho/2 + holgura;          // fondo de la ranura
    x_lab = x_ran - carril_fondo;             // cara interior del carril
    ran = pcb_grosor + 2*holgura;             // ancho de la ranura
    y0 = pcb_y_tras - pcb_grosor - holgura;   // inicio de la ranura en Y
    for (m = [0, 1]) mirror([m, 0, 0]) difference() {
        translate([x_lab, y0 - carril_labio, suelo - 0.5])
            cube([x_int - x_lab + 0.5, ran + 2*carril_labio, carril_z_fin - suelo + 0.5]);
        translate([x_lab - 1, y0, suelo + hueco_cables])
            cube([carril_fondo + 1, ran, cuerpo_alto]);
    }
}

// Asiento plano para la PCB estrella del LED (20 mm), por dentro de la pared frontal
module asiento_led() {
    translate([led_x, y_int_frente, vent_z - led_asiento/2])
        resalte45(led_asiento, led_asiento, led_resalte);
}

// Cuna en U (abierta por arriba) para el cuerpo del módulo de cámara
module cuna_camara() {
    w = cam_modulo + 0.3;
    m = 2;       // grosor de las paredes de la cuna
    t = 2.5;     // resalte
    difference() {
        translate([cam_x, y_int_frente, vent_z - w/2 - m]) resalte45(w + 2*m, w + m, t);
        translate([cam_x - w/2, y_int_frente - 1, vent_z - w/2]) cube([w, t + 2, w + m + 1]);
    }
}

// Tetones interiores que dan rosca a los 4 pilotos M3 del tejado (uno por cara)
module tetones() {
    for (i = [0:3]) rotate([0, 0, 90*i])
        translate([0, (i == 0) ? y_int_frente : -x_int, cuerpo_alto - torn_bajo_borde - teton[1]/2])
            resalte45(teton[0], teton[1], teton[2]);
}

// Silla en V trasera para chuzo redondo o cuadrado
module silla_farol() {
    translate([0, cuerpo_lado/2, 0]) linear_extrude(silla_alto, convexity = 4)
        perfil_silla(silla_v_ancho, silla_labio, silla_vertice, silla_v_fondo);
}

// Ventana: rebaje del cristal con dintel a 45°, aberturas de cámara y LED, pilotos del LED
module ventana() {
    yf = -cuerpo_lado/2;
    zt = vent_z + vent_alto/2;
    translate([-vent_ancho/2, yf - 1, vent_z - vent_alto/2])
        cube([vent_ancho, vent_fondo + 1, vent_alto]);
    // dintel a 45° para no puentear 42 mm (se rellena con el adhesivo del cristal)
    translate([-vent_ancho/2, 0, 0]) rotate([90, 0, 90]) linear_extrude(vent_ancho)
        polygon([[yf - 1, zt - 0.5], [yf + vent_fondo, zt - 0.5], [yf + vent_fondo, zt],
                 [yf - 1, zt + vent_fondo + 1]]);
    // cámara
    translate([cam_x, yf - 1, vent_z]) rotate([-90, 0, 0])
        cylinder(d = cam_d, h = pared_frontal + 2, $fn = 32);
    // LED (atraviesa pared y asiento)
    translate([led_x, yf - 1, vent_z]) rotate([-90, 0, 0])
        cylinder(d = led_d, h = pared_frontal + led_resalte + 2);
    // dos pilotos M3 ciegos para la estrella
    for (s = [-1, 1])
        translate([led_x, y_int_frente + led_resalte + 1, vent_z + s*led_torn_sep/2])
            rotate([90, 0, 0]) cylinder(d = m3_piloto, h = led_piloto_prof + 1, $fn = fn_fino);
}

// Fondo: prensaestopas PG7 y dos drenajes
module taladros_fondo() {
    translate([0, pg7_y, -1]) cylinder(d = pg7_d, h = suelo + 2);
    for (s = [-1, 1]) translate([s*(x_int - 7), y_int_frente + 6, -1])
        cylinder(d = dren_d, h = suelo + 2, $fn = fn_fino);
}

// 4 pilotos M3 radiales en el borde superior (patrón cuadrado: uno por cara)
module pilotos_tejado() {
    for (i = [0:3]) rotate([0, 0, 90*i])
        translate([0, -cuerpo_lado/2 - 1, cuerpo_alto - torn_bajo_borde]) rotate([-90, 0, 0])
            cylinder(d = m3_piloto, h = pared_frontal + teton[2] + 2, $fn = fn_fino);
}

// Dos pares de ranuras (túneles pasantes) para abrazaderas sinfín o bridas
module pasos_brida() {
    w = silla_v_ancho + 2*silla_labio;
    for (z = brida_z)
        translate([-w/2 - 1, cuerpo_lado/2 + brida_sep, z - brida_ancho/2])
            cube([w + 2, brida_grosor, brida_ancho]);
}

// =====================================================================
//  2 · FAROL: TEJADO   (se imprime tal cual, con el faldón sobre la cama)
// =====================================================================
module farol_tejado() {
    difference() {
        union() {
            tejado_macizo();
            chimenea_maciza();
        }
        union() {
            tejado_boca();
            tejado_desvan();
            rebaje_panel();
            paso_cable_panel();
            chimenea_hueco();
            taladros_faldon();
        }
    }
}

// Campana que abre desde el faldón hasta los aleros, cortada por el plano del faldón inclinado
module tejado_macizo() {
    intersection() {
        hull() {
            caja_red(tej_base, tej_base, cuerpo_radio + holgura + pared_tejado, eps);
            translate([0, 0, z_alero])
                caja_red(tej_ancho, tej_largo, tej_radio, z_cumbrera + 20 - z_alero);
        }
        bajo_plano(-tej_largo/2, z_bajo, inclinacion);
    }
}

// Boca: el faldón abraza el borde del cuerpo y asienta en un cono a 45°
module tejado_boca() {
    r = cuerpo_radio + holgura;
    c = 0.8;   // chaflán de entrada
    hull() {
        translate([0, 0, -1]) caja_red(tej_boca, tej_boca, r, eps);
        translate([0, 0, z_asiento]) caja_red(tej_boca, tej_boca, r, eps);
        translate([0, 0, z_asiento + asiento_n])
            caja_red(int_x, int_x, r - asiento_n, z_desvan + 1 - z_asiento - asiento_n);
    }
    hull() {
        translate([0, 0, -0.5]) caja_red(tej_boca + 2*c + 1, tej_boca + 2*c + 1, r + c + 0.5, eps);
        translate([0, 0, c + 0.5]) caja_red(tej_boca - 1, tej_boca - 1, r - 0.5, eps);
    }
}

// Desván hueco: paredes paralelas a la campana y techo a `techo_int_ang`
module tejado_desvan() {
    intersection() {
        hull() {
            caja_red(tej_base - 2*tej_a, tej_base - 2*tej_b, cuerpo_radio, eps);
            translate([0, 0, z_alero])
                caja_red(tej_ancho - 2*tej_a, tej_largo - 2*tej_b, 1, z_cumbrera + 20 - z_alero);
        }
        translate([-G/2, -G/2, z_desvan]) cube(G);
        bajo_plano(y_techo, z_techo, techo_int_ang);
    }
}

// Rebaje para el panel solar, con dos desagües en el marco bajo
module rebaje_panel() {
    en_pendiente() {
        translate([-(panel[0]/2 + panel_holg), panel_marco, -panel_rebaje])
            cube([panel[0] + 2*panel_holg, panel[1] + 2*panel_holg, panel_rebaje + 8]);
        for (s = [-1, 1]) translate([s*0.3*panel[0] - 2, -2, -panel_rebaje])
            cube([4, panel_marco + 3, panel_rebaje + 8]);
    }
}

// Paso vertical del cable del panel hacia el desván
module paso_cable_panel() {
    sc = panel_s + panel[1]/2;
    translate([0, -tej_largo/2 + sc*cos(inclinacion), z_bajo + sc*sin(inclinacion) - 30])
        cylinder(d = panel_cable_d, h = 34, $fn = fn_fino);
}

// Chimenea del sensor de cielo: fuste, goterón a 45° y cuello para el anillo
module chimenea_maciza() {
    translate([0, chim_y, 0]) rotate_extrude(convexity = 4) polygon([
        [0, z_chim0], [chim_d/2, z_chim0], [chim_d/2, z_goteron],
        [goteron_d/2, z_cuello - goteron_h], [goteron_d/2, z_cuello - 0.5],
        [goteron_d/2 - 0.5, z_cuello], [cuello_d/2, z_cuello], [cuello_d/2, z_chim],
        [0, z_chim]]);
}

// Taladro de la cápsula TO-39 (entra desde el desván) y labio de retención
module chimenea_hueco() {
    translate([0, chim_y, 0]) rotate_extrude(convexity = 4) polygon([
        [0, z_chim0 - 30], [cielo_d/2, z_chim0 - 30],
        [cielo_d/2, z_chim - labio_plano - labio], [cielo_d/2 - labio, z_chim - labio_plano],
        [cielo_d/2 - labio, z_chim + 1], [0, z_chim + 1]]);
}

// 4 taladros pasantes M3 radiales con cajera, uno por cara del faldón
module taladros_faldon() {
    for (i = [0:3]) rotate([0, 0, 90*i]) {
        translate([0, -tej_boca/2 + 1, torn_z]) rotate([90, 0, 0])
            cylinder(d = m3_paso, h = 40, $fn = fn_fino);
        translate([0, -tej_boca/2 - pared_tejado, torn_z]) rotate([90, 0, 0])
            cylinder(d = m3_cabeza, h = 40, $fn = 32);
    }
}

// =====================================================================
//  3 · ANILLO DE CIELO   (plano; el chaflán de entrada queda arriba)
// =====================================================================
module anillo_cielo() {
    ri = cuello_d/2 + anillo_holg;
    c = 0.6;
    difference() {
        cylinder(r = ri + anillo_pared, h = anillo_h);
        translate([0, 0, -1]) cylinder(r = ri, h = anillo_h + 2);
        // chaflán de entrada a 45°
        translate([0, 0, anillo_h - c - 1]) cylinder(r1 = ri - 1, r2 = ri + c + 1, h = c + 2);
    }
}

// =====================================================================
//  4 · PAGODA   (se imprime INVERTIDA: la tapa sobre la cama)
// =====================================================================
module pagoda() {
    difference() {
        union() {
            rotate_extrude(convexity = 16) pagoda_perfil();
            union() {
                pagoda_nervios();
                pagoda_brazo();
            }
        }
        union() {
            pagoda_ventanas();
            // paso de la sonda Ø6 y ranura para la plaquita SHT4x
            translate([0, 0, z_fondo - 1]) cylinder(d = collar_d, h = z_collar - z_fondo + 2, $fn = 32);
            translate([-ranura_sht[0]/2, -ranura_sht[1]/2, z_fondo - 1])
                cube([ranura_sht[0], ranura_sht[1], z_collar - z_fondo + 2]);
            pagoda_ranuras_brida();
        }
    }
}

// Perfil de revolución: tapa, lamas, tubo de la jaula, embudo de fondo y collarín
module pagoda_perfil() {
    rx = r_a + (tapa_grosor - lama_v)/pag_k;
    // tapa cónica cerrada
    polygon([[0, 0], [r_a, 0], [pag_R, lama_sube], [pag_R, lama_sube + lama_v],
             [rx, tapa_grosor], [0, tapa_grosor]]);
    // lamas
    for (k = [1:pag_lamas]) {
        c = k*pag_paso;
        polygon([[r_j + 0.5, c], [r_a, c], [pag_R, c + lama_sube],
                 [pag_R, c + lama_sube + lama_v], [rx, c + tapa_grosor],
                 [r_j + 0.5, c + tapa_grosor]]);
    }
    // tubo de la jaula
    translate([r_j, tapa_grosor/2]) square([jaula_pared, z_fondo + 0.5 - tapa_grosor/2]);
    // embudo de fondo a 45° y collarín con resalte para una brida fina
    polygon([[0, z_fondo + r_j], [r_j, z_fondo], [r_j, z_fondo + tapa_grosor],
             [r_ce, z_fondo + tapa_grosor + (r_j - r_ce)], [r_ce, z_collar - 2.0],
             [r_ce + 0.8, z_collar - 1.2], [r_ce + 0.8, z_collar], [0, z_collar]]);
}

// Nervios verticales que unen tapa, lamas y fondo
module pagoda_nervios() {
    for (i = [0:nervio_n - 1]) rotate([0, 0, i*360/nervio_n])
        rotate([90, 0, 0]) linear_extrude(nervio_grosor, center = true) polygon([
            [r_j + 0.6, tapa_grosor/2], [r_a, tapa_grosor/2],
            [r_n, (r_n - r_a)*pag_k + lama_v/2],
            [r_n, z_fondo + (r_n - r_a)*pag_k + lama_v/2],
            [r_a, z_fondo + tapa_grosor/2], [r_j + 0.6, z_fondo + tapa_grosor/2]]);
}

// Ventanas de la jaula entre nervios, con dintel en punta a 45° (sin puentes)
module pagoda_ventanas() {
    for (k = [0:pag_lamas - 1]) {
        z0 = k*pag_paso + tapa_grosor + 0.4;
        za = (k + 1)*pag_paso - 0.4;
        w = min(jaula_vent, 2*(za - z0 - 0.3));
        for (i = [0:nervio_n - 1]) rotate([0, 0, 90 + (i + 0.5)*360/nervio_n])
            rotate([90, 0, 0]) linear_extrude(r_a + 0.4) polygon([
                [-w/2, z0], [w/2, z0], [w/2, za - w/2], [0, za], [-w/2, za - w/2]]);
    }
}

// Brazo en T y silla en V hacia el chuzo
module pagoda_brazo() {
    // ala (sobre la cama)
    translate([r_j + 1, -brazo_ancho/2, 0]) cube([psilla_x - r_j, brazo_ancho, brazo_grosor]);
    // alma: sigue el cono de la tapa por dentro y sube hasta la silla
    rotate([90, 0, 0]) linear_extrude(brazo_grosor, center = true) polygon([
        [r_a, 0], [psilla_x + 1, 0], [psilla_x + 1, psilla_alto],
        [psilla_x - (psilla_alto - brazo_alma), brazo_alma],
        [r_a + (brazo_alma - lama_v/2)/pag_k, brazo_alma], [r_a, lama_v/2]]);
    // silla
    translate([psilla_x, 0, 0]) rotate([0, 0, -90]) linear_extrude(psilla_alto, convexity = 4)
        perfil_silla(psilla_v_ancho, psilla_labio, psilla_vertice, psilla_v_fondo, solape = 0);
}

module pagoda_ranuras_brida() {
    w = psilla_v_ancho + 2*psilla_labio;
    for (z = [psilla_alto*0.25, psilla_alto*0.75])
        translate([psilla_x + pared, -w/2 - 1, z - pbrida[0]/2]) cube([pbrida[1], w + 2, pbrida[0]]);
}

// Pagoda en posición de uso (tapa arriba, collarín abajo)
module pagoda_montada() {
    translate([0, 0, pag_alto]) rotate([180, 0, 0]) pagoda();
}

// =====================================================================
//  VISTAS
// =====================================================================

function oscuro(c) = [0.7*c[0], 0.7*c[1], 0.7*c[2]];

// Pinta una pieza ya evaluada (render). Con corte = true se queda con la mitad
// X < 0 y pinta la sección (plano X = 0) en un tono más oscuro.
module pinta(c, corte = false) {
    if (corte) {
        color(c) render(convexity = 12) intersection() {
            children();
            translate([-G, -G/2, -G/2]) cube(G);
        }
        color(oscuro(c)) rotate([0, 90, 0]) linear_extrude(0.2)
            projection(cut = true) rotate([0, -90, 0]) children();
    } else color(c) render(convexity = 12) children();
}

// Pagoda montada cortada por el plano Y = 0 (se mira desde -Y)
module pagoda_corte() {
    color(col_pagoda) render(convexity = 16) difference() {
        pagoda_montada();
        translate([-G/2, -G, -G/2]) cube(G);
    }
    color(col_tejado) translate([0, -0.2, 0]) rotate([-90, 0, 0]) linear_extrude(0.2)
        projection(cut = true) rotate([90, 0, 0]) pagoda_montada();
}

// Rótulo encarado a la cámara
module rotulo(t, tam = 55) {
    color(col_rotulo) rotate([vista_rx, 0, vista_rz]) linear_extrude(1)
        text(t, size = tam, font = "Liberation Sans:style=Bold", halign = "left", valign = "center");
}

// Cabeza: origen en el centro de la base del cuerpo. e = 0 montada, e = 1 despiece
module cabeza(e = 0, corte = false, con_estaca = true) {
    ye = y_estaca(estaca_d);
    pinta(col_cuerpo, corte) farol_cuerpo();

    // tejado, panel, anillo y film
    giro = 90*orientacion_tejado;
    zt = cuerpo_alto - faldon + e*165;
    pinta(col_tejado, corte) translate([0, 0, zt]) rotate([0, 0, giro]) farol_tejado();
    pinta(col_panel, corte) translate([0, 0, zt]) rotate([0, 0, giro]) en_pendiente()
        translate([-panel[0]/2, panel_s, -panel_rebaje + e*38]) cube(panel);
    pinta(col_anillo, corte) translate([0, 0, zt]) rotate([0, 0, giro])
        translate([0, chim_y, z_cuello + anillo_h + e*30]) rotate([180, 0, 0]) anillo_cielo();
    if (e > 0) color([1, 1, 1, 0.55]) translate([0, 0, zt]) rotate([0, 0, giro])
        translate([0, chim_y, z_chim + e*14]) cylinder(d = 28, h = 0.3);

    // placa (envolvente sin verificar)
    pinta(col_placa, corte)
        translate([-placa_ancho/2, placa_y_frente, suelo + hueco_cables + e*(cuerpo_alto + 10)])
            cube([placa_ancho, placa_grosor, placa_largo]);

    // chuzo
    if (con_estaca) pinta(col_estaca, corte)
        translate([0, ye + e*45, -90]) cylinder(d = estaca_d, h = 90 + silla_alto - estaca_bajo_silla);

    // cristal de metacrilato de la ventana
    color([0.62, 0.82, 0.95, 0.35])
        translate([-(vent_ancho - 0.6)/2, -cuerpo_lado/2 + vent_fondo - 1.5 - e*40,
                   vent_z - (vent_alto - 0.6)/2])
            cube([(vent_ancho - 0.6)/(corte ? 2 : 1), 1.5, vent_alto - 0.6]);
}

// Chuzo completo con las 4 pagodas y el farol
module conjunto() {
    color(col_suelo) translate([0, 0, -4]) cylinder(d = 900, h = 4, $fn = 64);
    color(col_estaca) cylinder(d = estaca_d, h = estaca_alto);
    xe = x_estaca_pag(estaca_d);
    for (h = sondas)
        translate([0, 0, h - pag_z_sensor]) rotate([0, 0, 180]) translate([-xe, 0, 0])
            color(col_pagoda) render(convexity = 16) pagoda_montada();
    zc = estaca_alto - (silla_alto - estaca_bajo_silla);
    translate([0, -y_estaca(estaca_d), zc]) cabeza(0, false, false);
    if (rotulos) {
        for (h = sondas) translate([xe + pag_R + 40, 0, h])
            rotulo(str(floor(h/1000), ",", floor(h/100)%10, floor(h/10)%10, " m"));
        translate([tej_ancho/2 + 40, -y_estaca(estaca_d), zc + cuerpo_alto/2 + 30]) rotulo("farol");
        translate([tej_ancho/2 + 40, -y_estaca(estaca_d), zc + cuerpo_alto/2 - 50])
            rotulo(str("chuzo Ø", estaca_d, " × ", estaca_alto), 34);
    }
}

// =====================================================================
//  Selector
// =====================================================================
if (pieza == "farol_cuerpo") farol_cuerpo();
else if (pieza == "farol_tejado") farol_tejado();
else if (pieza == "anillo_cielo") anillo_cielo();
else if (pieza == "pagoda") pagoda();
else mirror([espejo ? 1 : 0, 0, 0]) {
    if (pieza == "pagoda_montada") color(col_pagoda) render(convexity = 16) pagoda_montada();
    else if (pieza == "pagoda_corte") pagoda_corte();
    else if (pieza == "cabeza") cabeza(0);
    else if (pieza == "despiece") cabeza(1);
    else if (pieza == "cabeza_corte") cabeza(0, true);
    else if (pieza == "conjunto") conjunto();
    else echo(str("Pieza desconocida: ", pieza));
}

echo(str("SERENO · pieza = ", pieza,
         " · cuerpo ", cuerpo_lado, " × ", cuerpo_lado, " × ", cuerpo_alto,
         " · tejado ", tej_ancho, " × ", tej_largo, " × ", z_chim,
         " · pagoda Ø", pag_d, " × ", pag_alto, " · paso de lamas ", pag_paso));
