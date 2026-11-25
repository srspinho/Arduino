// =====================================================================
//  MiniPad: caixa COMPACTA com PAINEL INCLINADO (tela + encoder)
//
//  As teclas continuam retas. A tela e o encoder ficam num painel que sobe
//  em rampa para tras, virado para quem usa: a tela nao fica mais escondida
//  atras dos keycaps.
//
//  Sao 3 pecas de cima/baixo + o knob:
//    "bottom" -> caixa (imprimir com o fundo na mesa; as paredes ja tem a rampa)
//    "plate"  -> tampa das teclas (ja sai virada: topo na mesa)
//    "panel"  -> painel da tela e do encoder (ja sai virado: frente na mesa)
//    "knob"   -> botao do encoder
//    "all"    -> visualizacao montada (com os modulos em transparencia)
//
//  Fixacao: 8 parafusos M2 x 8 mm (4 na tampa das teclas, 4 no painel).
//  Como na versao compacta: fios SOLDADOS direto (dupont nao cabe).
// =====================================================================

part = "all";         // [all, bottom, plate, panel, knob]
tilt = 25;            // inclinacao do painel, em graus (20 a 35)

/* [Tela 0.96" (placa 30 x 24 mm)] */
// [largura da placa, altura da placa, largura da area ativa, altura da area
//  ativa, deslocamento da area ativa (x: + = longe dos pinos; y), vidro]
scr = [30.0, 24.0, 21.70, 10.80, -1.2, 0, 1.6];   // -1.2: medido nas fotos do modulo montado
win_margin = 0.5;

/* [Encoder na plaquinha KY-040 (medida: 32 x 19 mm, eixo a 9 mm da ponta)] */
ky_pcb = [32.0, 19.0];
ky_enc_from_end = 11.0;   // 9 medido + 2 de sobra
ky_pins = 7.0;
enc_hole = 7.4;

/* [Teclas] */
pitch      = 19.05;
keys       = 5;
sw_cut     = 14.2;
sw_chamfer = 0.4;

/* [Caixa] */
edge     = 3.2;
strip    = 32;            // profundidade (em planta) da parte inclinada
corner_r = 3;
wall     = 2.0;
floor_t  = 1.6;
inner_h  = 11;            // fundo -> baixo da tampa das teclas
plate_t  = 1.5;
lip_h    = 2.0;
lip_t    = 1.2;
fit      = 0.3;
post_d   = 5;
post_hole  = 1.7;         // M2 auto-atarraxante
screw_hole = 2.4;

/* [RP2040-Zero] */
board_dims = [18, 23.5];
usb_w = 13; usb_h = 7.5;
board_ledge = 1.0;

$fn = 48;

// ------------------------- medidas derivadas -------------------------
W = keys * pitch + 2 * edge;
y_k = edge + pitch + 2;                 // linha onde comeca a rampa
D = y_k + strip;
H = floor_t + inner_h;                  // altura das paredes na parte reta
L = strip / cos(tilt);                  // comprimento do painel (na rampa)
H_back = H + strip * tan(tilt);         // altura da parede de tras
cxy = corner_r;

function zwall(y) = y <= y_k ? H : H + (y - y_k) * tan(tilt);
// ponto do painel (x, s ao longo da rampa, t perpendicular) -> mundo
function P(x, s, t) = [x, y_k + s * cos(tilt) - t * sin(tilt), H + s * sin(tilt) + t * cos(tilt)];

module on_panel() translate([0, y_k, H]) rotate([tilt, 0, 0]) children();

// posicoes (no proprio painel: x e s)
scr_c = [edge + 1.5 * pitch, L / 2 - 1];   // 1 mm abaixo do meio: atras nao cabe (parede)
win_c = [scr_c[0] + scr[4], scr_c[1] + scr[5]];
win = [scr[2] + 2 * win_margin, scr[3] + 2 * win_margin];
glass_t = scr[6];
enc_c = [W - wall - fit - (ky_enc_from_end + 0.5), L / 2];
ky_x0 = enc_c[0] - (ky_pcb[0] - ky_enc_from_end);

plate_posts = [[cxy, cxy], [W - cxy, cxy], [cxy, y_k - 3.5], [W - cxy, y_k - 3.5]];
panel_posts = [[cxy, 3.5], [W - cxy, 3.5], [cxy, L - 3.5], [W - cxy, L - 3.5]];   // (x, s)

board_x = scr_c[0] + 6;

echo(str("Caixa inclinada: ", W, " x ", D, " mm; altura ", H + plate_t, " mm na frente e ",
         H_back + plate_t, " mm atras; painel de ", L, " mm a ", tilt, " graus"));

// ---- conferencias ----
assert(enc_c[0] + ky_enc_from_end < W - wall, "placa do encoder nao cabe na lateral direita");
assert(ky_x0 - ky_pins > scr_c[0] + scr[0] / 2 + 3, "pinos do encoder encostam na tela");
assert(enc_c[1] - ky_pcb[1] / 2 > 3.5 + post_d / 2 && enc_c[1] + ky_pcb[1] / 2 < L - 3.5 - post_d / 2,
       "placa do encoder encosta nos parafusos do painel: aumente strip");
// cantoneira de tras da tela x parede de tras (o painel e inclinado, a parede e reta)
scr_back = P(0, scr_c[1] + scr[1] / 2 + fit + 1.2, -(glass_t + 1.6 + 1.0));
assert(scr_back[1] < D - wall - 0.5, "cantoneira da tela bate na parede de tras");
ky_back = P(0, enc_c[1] + ky_pcb[1] / 2, -8.6);
assert(ky_back[1] < D - wall - 0.5, "plaquinha do encoder bate na parede de tras");
// ponto mais baixo da plaquinha do encoder (canto da frente, 8.6 mm abaixo do painel)
ky_low = P(0, enc_c[1] - ky_pcb[1] / 2, -8.6);
assert(ky_low[2] > floor_t + 1, "plaquinha do encoder bate no fundo: aumente inner_h");
assert(ky_low[1] > edge + pitch, "plaquinha do encoder bate nos switches");

// ------------------------------ ajudantes ----------------------------
module rounded_rect(w, d, h, r) {
  translate([r, r, 0]) linear_extrude(h) offset(r) square([w - 2 * r, d - 2 * r]);
}
// retangulo com cantos arredondados so de um lado (front = arredonda y=0)
module half_round(w, d, h, r, front) {
  linear_extrude(h) hull() {
    for (x = [r, w - r]) translate([x, front ? r : d - r]) circle(r);
    for (x = [0, w - 0.01]) translate([x, front ? d - 0.01 : 0]) square(0.01);
  }
}
module key_positions() {
  for (c = [0 : keys - 1]) translate([edge + (c + 0.5) * pitch, edge + pitch / 2, 0]) children();
}
module switch_hole(size) {
  translate([-size / 2, -size / 2, -5]) cube([size, size, 10]);
  hull() {
    translate([-size / 2, -size / 2, plate_t - sw_chamfer]) cube([size, size, 0.01]);
    translate([-size / 2 - sw_chamfer, -size / 2 - sw_chamfer, plate_t])
      cube([size + 2 * sw_chamfer, size + 2 * sw_chamfer, 0.02]);
  }
}
// perfil lateral (y, z): reto na frente, rampa atras
module side_profile() {
  rotate([90, 0, 90]) linear_extrude(W + 2) polygon([[0, 0], [D, 0], [D, H_back], [y_k, H], [0, H]]);
}

// ---------------------------- caixa inferior -------------------------
module bottom() {
  bw = board_dims[0]; bl = board_dims[1];
  usb_z = floor_t + board_ledge + 1.6 + 1.6;
  intersection() {
    difference() {
      union() {
        difference() {
          rounded_rect(W, D, H_back + 1, corner_r);
          translate([wall, wall, floor_t]) rounded_rect(W - 2 * wall, D - 2 * wall, H_back + 5, max(0.5, corner_r - wall));
        }
        // postes da tampa das teclas
        for (p = plate_posts) translate([p[0], p[1], 0]) cylinder(d = post_d, h = H - lip_h - 0.2);
        // postes do painel: seguem a perpendicular do painel ate o fundo
        for (p = panel_posts) {
          top = P(p[0], p[1], -lip_h - 0.2);
          down = (top[2] - floor_t) / cos(tilt);
          bot = P(p[0], p[1], -lip_h - 0.2 - down);
          hull() {
            translate(top) rotate([tilt, 0, 0]) cylinder(d = post_d, h = 0.01);
            translate([bot[0], bot[1], 0]) cylinder(d = post_d, h = floor_t + 0.01);
          }
        }
        // berco da RP2040-Zero, encostado na parede de tras
        translate([board_x - bw / 2, D - wall - bl, floor_t]) {
          for (x = [0, bw - 2]) translate([x, 0, 0]) cube([2, bl, board_ledge]);
          for (x = [-1.2 - fit, bw + fit]) translate([x, bl - 8, 0]) cube([1.2, 8, board_ledge + 2.5]);
          translate([0, -1.2 - fit, 0]) cube([bw, 1.2, board_ledge + 2]);
        }
      }
      for (p = plate_posts) translate([p[0], p[1], floor_t]) cylinder(d = post_hole, h = H);
      for (p = panel_posts) on_panel() translate([p[0], p[1], -10]) cylinder(d = post_hole, h = 10.5);
      translate([board_x, D - wall / 2, usb_z]) rotate([90, 0, 0])
        linear_extrude(wall + 2, center = true) offset(1.5) square([usb_w - 3, usb_h - 3], center = true);
    }
    translate([-1, 0, 0]) side_profile();
  }
}

// ------------------------ tampa das teclas ---------------------------
module plate() {
  d = y_k - 0.7;   // folga para o painel inclinado
  difference() {
    union() {
      half_round(W, d, plate_t, corner_r, true);
      // aba na frente e nas laterais
      translate([0, 0, -lip_h]) difference() {
        translate([wall + fit, wall + fit, 0]) cube([W - 2 * (wall + fit), d - wall - fit, lip_h]);
        translate([wall + fit + lip_t, wall + fit + lip_t, -1]) cube([W - 2 * (wall + fit + lip_t), d, lip_h + 2]);
        for (p = plate_posts) translate([p[0], p[1], -1]) cylinder(d = post_d + 1.2, h = lip_h + 2);
      }
    }
    key_positions() switch_hole(sw_cut);
    for (p = plate_posts) {
      translate([p[0], p[1], -5]) cylinder(d = screw_hole, h = 10);
      translate([p[0], p[1], plate_t - 0.6]) cylinder(d1 = screw_hole, d2 = screw_hole + 1.2, h = 0.61);
    }
  }
}

// ---------------- painel inclinado (coordenadas do painel) -----------
module panel() {
  ow = scr[0]; od = scr[1];
  hh = glass_t + 1.6 + 1.0;
  difference() {
    union() {
      half_round(W, L, plate_t, corner_r, false);
      // abas nas laterais (encaixam por dentro das paredes)
      for (x = [wall + fit, W - wall - fit - lip_t])
        translate([x, 4, -lip_h]) cube([lip_t, L - 8 - wall, lip_h]);
      // cantoneiras da tela
      translate([scr_c[0], scr_c[1], -hh])
        for (sx = [-1, 1], sy = [-1, 1])
          translate([sx * (ow / 2 + fit), sy * (od / 2 + fit), 0])
            mirror([sx < 0 ? 1 : 0, 0, 0]) mirror([0, sy < 0 ? 1 : 0, 0])
              difference() {
                translate([-5, -5, 0]) cube([6.2, 6.2, hh]);
                translate([-6, -6, -1]) cube([6, 6, hh + 2]);
              }
    }
    translate([enc_c[0], enc_c[1], -5]) cylinder(d = enc_hole, h = 10);
    translate([win_c[0], win_c[1], -0.01]) hull() {
      cube([win[0], win[1], 0.01], center = true);
      translate([0, 0, plate_t]) cube([win[0] + 2 * plate_t, win[1] + 2 * plate_t, 0.02], center = true);
    }
    for (p = panel_posts) {
      translate([p[0], p[1], -5]) cylinder(d = screw_hole, h = 10);
      translate([p[0], p[1], plate_t - 0.6]) cylinder(d1 = screw_hole, d2 = screw_hole + 1.2, h = 0.61);
      translate([p[0], p[1], -lip_h - 1]) cylinder(d = post_d + 1.2, h = lip_h + 1);   // abas livres nos postes
    }
  }
}

// -------------------------------- knob -------------------------------
module knob() {
  kd = 16; kh = 11;
  difference() {
    union() {
      cylinder(d = kd, h = kh, $fn = 64);
      translate([0, kd / 2 - 1.5, kh]) cylinder(d = 1.8, h = 0.5);
    }
    translate([0, 0, -0.01]) intersection() {
      cylinder(d = 6.2, h = kh - 2);
      translate([-4, -3.1 + 1.5 + 0.1, 0]) cube([8, 8, kh]);
    }
    for (a = [0 : 15 : 359]) rotate(a) translate([kd / 2 + 0.3, 0, 2]) cylinder(d = 1.5, h = kh, $fn = 12);
  }
}

// --------------------- modulos (so para visualizar) ------------------
module ghosts() {
  on_panel() {
    color("darkred", 0.8) translate([scr_c[0] - scr[0] / 2, scr_c[1] - scr[1] / 2, -glass_t - 1.6]) cube([scr[0], scr[1], 1.6]);
    color("#123") translate([win_c[0] - scr[2] / 2, win_c[1] - scr[3] / 2, -glass_t]) cube([scr[2], scr[3], glass_t - 0.05]);
    color("silver") translate([enc_c[0] - 6, enc_c[1] - 6, -6.5]) cube([12, 12, 6.5]);
    color("darkblue", 0.8) translate([ky_x0, enc_c[1] - ky_pcb[1] / 2, -8.1]) cube([ky_pcb[0], ky_pcb[1], 1.6]);
    color("gold") translate([ky_x0 - ky_pins, enc_c[1] - 5, -7.6]) cube([ky_pins, 10, 0.6]);
  }
  color("black", 0.8) translate([board_x - board_dims[0] / 2, D - wall - board_dims[1], floor_t + board_ledge]) cube([board_dims[0], board_dims[1], 1.6]);
  color("gray", 0.5) key_positions() translate([-7, -7, H - 8.3]) cube([14, 14, 8.3]);
}

// ------------------------------- montagem ----------------------------
if (part == "bottom") bottom();
else if (part == "plate") translate([0, 0, plate_t]) rotate([180, 0, 0]) translate([0, -y_k, 0]) plate();
else if (part == "panel") translate([0, 0, plate_t]) rotate([180, 0, 0]) translate([0, -L, 0]) panel();
else if (part == "knob") knob();
else if (part == "all") {
  color("dimgray") bottom();
  ghosts();
  color("lightgray") translate([0, 0, H]) plate();
  color("lightgray") on_panel() panel();
  color("black") on_panel() translate([enc_c[0], enc_c[1], plate_t + 5]) knob();
  color("whitesmoke") translate([0, 0, H + plate_t]) key_positions() translate([-9, -9, 6]) cube([18, 18, 7]);
}
