// =====================================================================
//  MiniPad: caixa COMPACTA (tela 0.96" + encoder em plaquinha KY-040)
//
//  ~102 x 54 x 14 mm (a caixa normal tem 111 x 67 x 20 mm)
//
//  Como ficou menor:
//   - bordas minimas em volta das teclas (3.2 mm)
//   - faixa de tras so com a altura da tela (24 mm)
//   - parafusos M2 nos cantos (cabeca menor que a M3, cabe ao lado das teclas)
//   - altura interna de 11 mm: o necessario para o switch e o encoder
//
//  IMPORTANTE: nesta versao os fios precisam ser SOLDADOS direto nos pinos
//  da tela, da plaquinha do encoder e da RP2040-Zero. Conectores dupont
//  (os de jumper) nao cabem: so a capinha deles tem 14 mm.
//
//  Pecas (mude "part" ou use o Customizer do OpenSCAD):
//    "all"     -> visualizacao montada (com os modulos em transparencia)
//    "bottom"  -> caixa inferior (imprimir com o fundo na mesa)
//    "plate"   -> tampa (ja sai virada: imprimir com o topo na mesa)
//    "knob"    -> botao do encoder (mais baixo que o da caixa normal)
//
//  Fixacao: 4 parafusos M2 x 8 mm (auto-atarraxantes, cabeca panela)
// =====================================================================

part = "all";         // [all, bottom, plate, knob]

/* [Tela 0.96" (medidas confirmadas: placa 30 x 24 mm)] */
// [largura da placa, altura da placa, largura da area ativa, altura da area
//  ativa, deslocamento da area ativa (x: + = longe dos pinos; y), vidro]
scr = [30.0, 24.0, 21.70, 10.80, 1.0, 0, 1.6];
win_margin = 0.4;

/* [Encoder na plaquinha KY-040 (CLK DT SW + GND) - MECA A SUA] */
ky_pcb = [32.0, 19.0];    // comprimento x largura (medido: 32 x 19 mm)
ky_enc_from_end = 11.0;   // centro do eixo ate a ponta da placa SEM pinos (com 1 mm de sobra)
ky_pins = 7.0;            // quanto os pinos (deitados) saem da placa
enc_hole = 7.4;

/* [Teclas] */
pitch      = 19.05;
keys       = 5;
sw_cut     = 14.2;        // mesmo valor da caixa normal
sw_chamfer = 0.4;

/* [Caixa] */
edge     = 3.2;           // da lateral externa ate a grade das teclas
strip    = 28;            // faixa de tras (tela + encoder)
corner_r = 3;
wall     = 2.0;
floor_t  = 1.6;
inner_h  = 11;            // fundo -> baixo da tampa
plate_t  = 1.5;
lip_h    = 2.0;
lip_t    = 1.2;
fit      = 0.3;
post_d   = 5;
post_hole  = 1.7;         // piloto para parafuso M2 auto-atarraxante
screw_hole = 2.4;
screw_head = 4.0;

/* [RP2040-Zero] */
board_dims = [18, 23.5];
usb_w = 13; usb_h = 7.5;
board_ledge = 1.0;

$fn = 48;

// ------------------------- medidas derivadas -------------------------
W = keys * pitch + 2 * edge;
D = pitch + strip + 2 * edge;
H = floor_t + inner_h;
strip_y = edge + pitch + strip / 2;
cxy = corner_r;                                   // centro dos postes (nos cantos)
post_pos = [[cxy, cxy], [W - cxy, cxy], [cxy, D - cxy], [W - cxy, D - cxy]];

enc_pos = [W - wall - fit - (ky_enc_from_end + 0.5), strip_y];   // placa encostada na lateral direita
scr_pos = [edge + 1.5 * pitch, strip_y];                         // tela em cima das teclas 1 e 2
win_pos = [scr_pos[0] + scr[4], scr_pos[1] + scr[5]];
win = [scr[2] + 2 * win_margin, scr[3] + 2 * win_margin];
glass_t = scr[6];
ky_x0 = enc_pos[0] - (ky_pcb[0] - ky_enc_from_end);             // ponta da placa do encoder (lado dos pinos)
board_x = scr_pos[0] + 6;   // RP2040 embaixo da tela (lado oposto aos pinos dela), USB por tras

echo(str("Caixa compacta: ", W, " x ", D, " mm, altura ", H + plate_t, " mm"));

// ---- conferencias: avisa se algo nao couber ----
sw_half = sw_cut / 2;
assert(cxy + screw_head / 2 < edge + (pitch - sw_cut) / 2 + 1.2,
       "cabeca do parafuso encosta no furo da tecla");
assert(enc_pos[0] + ky_enc_from_end < W - wall,
       "placa do encoder nao cabe na lateral direita");
usb_top = floor_t + board_ledge + 1.6 + 3.3;              // topo do conector USB-C
scr_bottom = H - glass_t - 1.6 - 1.0;                       // baixo das cantoneiras da tela
echo(str("Folga entre o USB e as cantoneiras da tela: ", scr_bottom - usb_top, " mm"));
assert(scr_bottom - usb_top >= 0.8, "RP2040 encosta na tela: aumente inner_h");
assert(board_x + board_dims[0] / 2 + 3 < ky_x0 - ky_pins, "RP2040 encosta nos pinos do encoder");
assert(strip_y - ky_pcb[1] / 2 > edge + pitch - 1,
       "placa do encoder invade a fileira de teclas");

// ------------------------------ ajudantes ----------------------------
module rounded_rect(w, d, h, r) {
  translate([r, r, 0]) linear_extrude(h) offset(r) square([w - 2 * r, d - 2 * r]);
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

// ---------------------------- caixa inferior -------------------------
module bottom() {
  bw = board_dims[0]; bl = board_dims[1];
  usb_z = floor_t + board_ledge + 1.6 + 1.6;
  difference() {
    union() {
      difference() {
        rounded_rect(W, D, H, corner_r);
        translate([wall, wall, floor_t]) rounded_rect(W - 2 * wall, D - 2 * wall, H, max(0.5, corner_r - wall));
      }
      for (p = post_pos) translate([p[0], p[1], 0]) cylinder(d = post_d, h = H - lip_h - 0.2);
      // berco da RP2040-Zero, encostado na parede de tras
      translate([board_x - bw / 2, D - wall - bl, floor_t]) {
        for (x = [0, bw - 2]) translate([x, 0, 0]) cube([2, bl, board_ledge]);
        for (x = [-1.2 - fit, bw + fit]) translate([x, bl - 8, 0]) cube([1.2, 8, board_ledge + 2.5]);
        translate([0, -1.2 - fit, 0]) cube([bw, 1.2, board_ledge + 2]);
      }
    }
    for (p = post_pos) translate([p[0], p[1], floor_t]) cylinder(d = post_hole, h = H);
    translate([board_x, D - wall / 2, usb_z]) rotate([90, 0, 0])
      linear_extrude(wall + 2, center = true) offset(1.5) square([usb_w - 3, usb_h - 3], center = true);
  }
}

// -------------------------------- tampa ------------------------------
module plate() {
  ow = scr[0]; od = scr[1];
  hh = glass_t + 1.6 + 1.0;
  difference() {
    union() {
      rounded_rect(W, D, plate_t, corner_r);
      // aba que encaixa por dentro das paredes
      translate([0, 0, -lip_h]) difference() {
        translate([wall + fit, wall + fit, 0]) rounded_rect(W - 2 * (wall + fit), D - 2 * (wall + fit), lip_h, 1);
        translate([wall + fit + lip_t, wall + fit + lip_t, -1])
          rounded_rect(W - 2 * (wall + fit + lip_t), D - 2 * (wall + fit + lip_t), lip_h + 2, 1);
        for (p = post_pos) translate([p[0], p[1], -1]) cylinder(d = post_d + 1.2, h = lip_h + 2);
      }
      // 4 cantoneiras da tela
      translate([scr_pos[0], scr_pos[1], -hh])
        for (sx = [-1, 1], sy = [-1, 1])
          translate([sx * (ow / 2 + fit), sy * (od / 2 + fit), 0])
            mirror([sx < 0 ? 1 : 0, 0, 0]) mirror([0, sy < 0 ? 1 : 0, 0])
              difference() {
                translate([-5, -5, 0]) cube([6.2, 6.2, hh]);
                translate([-6, -6, -1]) cube([6, 6, hh + 2]);
              }
    }
    key_positions() switch_hole(sw_cut);
    translate([enc_pos[0], enc_pos[1], -5]) cylinder(d = enc_hole, h = 10);
    // janela chanfrada
    translate([win_pos[0], win_pos[1], -0.01]) hull() {
      cube([win[0], win[1], 0.01], center = true);
      translate([0, 0, plate_t]) cube([win[0] + 2 * plate_t, win[1] + 2 * plate_t, 0.02], center = true);
    }
    for (p = post_pos) {
      translate([p[0], p[1], -5]) cylinder(d = screw_hole, h = 10);
      translate([p[0], p[1], plate_t - 0.6]) cylinder(d1 = screw_hole, d2 = screw_hole + 1.2, h = 0.61);
    }
  }
}

// -------------------------------- knob -------------------------------
module knob() {
  kd = 16; kh = 11;
  difference() {
    union() {
      cylinder(d = kd, h = kh, $fn = 64);
      translate([0, kd / 2 - 1.5, kh]) cylinder(d = 1.8, h = 0.5);   // marcador
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
  top = H;   // baixo da tampa
  // tela
  color("darkred", 0.8) translate([scr_pos[0] - scr[0] / 2, scr_pos[1] - scr[1] / 2, top - glass_t - 1.6]) cube([scr[0], scr[1], 1.6]);
  color("#123") translate([win_pos[0] - scr[2] / 2, win_pos[1] - scr[3] / 2, top - glass_t]) cube([scr[2], scr[3], glass_t - 0.05]);
  // encoder: corpo EC11 + plaquinha + pinos
  color("silver") translate([enc_pos[0] - 6, enc_pos[1] - 6, top - 6.5]) cube([12, 12, 6.5]);
  color("darkblue", 0.8) translate([ky_x0, enc_pos[1] - ky_pcb[1] / 2, top - 8.1]) cube([ky_pcb[0], ky_pcb[1], 1.6]);
  color("gold") translate([ky_x0 - ky_pins, enc_pos[1] - 5, top - 7.6]) cube([ky_pins, 10, 0.6]);
  // RP2040-Zero
  color("black", 0.8) translate([board_x - board_dims[0] / 2, D - wall - board_dims[1], floor_t + board_ledge]) cube([board_dims[0], board_dims[1], 1.6]);
  // switches (parte de baixo)
  color("gray", 0.5) key_positions() translate([-7, -7, top - 8.3]) cube([14, 14, 8.3]);
}

// ------------------------------- montagem ----------------------------
if (part == "bottom") bottom();
else if (part == "plate") translate([0, 0, plate_t]) rotate([180, 0, 0]) translate([0, -D, 0]) plate();
else if (part == "knob") knob();
else {
  color("dimgray") bottom();
  ghosts();
  color("lightgray") translate([0, 0, H]) plate();
  color("black") translate([enc_pos[0], enc_pos[1], H + plate_t + 5]) knob();
  color("whitesmoke") translate([0, 0, H + plate_t]) key_positions() translate([-9, -9, 6]) cube([18, 18, 7]);
}
