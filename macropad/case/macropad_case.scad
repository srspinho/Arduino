// =====================================================================
//  MacroPad 4x3 + encoder - caixa para impressao 3D (OpenSCAD)
//
//  Pecas (mude "part" ou use o Customizer do OpenSCAD):
//    "all"     -> visualizacao montada
//    "bottom"  -> caixa inferior (imprimir com o fundo na mesa)
//    "plate"   -> tampa/plate dos switches (imprimir com o topo na mesa)
//    "knob"    -> botao do encoder (eixo D de 6 mm, EC11)
//
//  Fixacao: 4 parafusos M3 x 8 mm (cabeca panela) direto no plastico
//  (furo piloto de 2.6 mm). Se preferir insertos termicos M3, mude
//  post_hole para 4.2.
// =====================================================================

part  = "all";        // [all, bottom, plate, knob]
board = "zero";       // [zero, pico2]  Waveshare RP2350-Zero ou Raspberry Pi Pico 2
oled  = "091";        // [091, 096]     OLED 0.91" 128x32 ou 0.96" 128x64

/* [Teclas] */
pitch   = 19.05;      // espacamento padrao MX
cols    = 4;
rows    = 3;
sw_cut  = 14.0;       // furo do switch MX (14.0 = justo; 14.1 se ficar apertado)

/* [Caixa] */
edge     = 8;         // borda entre a grade de teclas e a lateral externa
strip    = 22;        // faixa de tras para OLED + encoder
corner_r = 4;
wall     = 2.4;
floor_t  = 2.4;
inner_h  = 18;        // altura interna livre (fundo -> baixo do plate)
plate_t  = 1.5;       // 1.5 mm = espessura padrao para os clipes do switch MX
lip_h    = 2.5;       // aba do plate que encaixa dentro das paredes
fit      = 0.3;       // folga de encaixe
post_d   = 7;
post_hole  = 2.6;     // M3 auto-atarraxante (4.2 para inserto termico)
screw_hole = 3.4;

/* [Encoder EC11] */
enc_hole = 7.4;

$fn = 48;

// ------------------------- medidas derivadas -------------------------
W = cols * pitch + 2 * edge;
D = rows * pitch + strip + 2 * edge;
H = floor_t + inner_h;

strip_y = edge + rows * pitch + strip / 2;            // centro da faixa
enc_pos  = [edge + (cols - 0.5) * pitch, strip_y];      // encoder em cima da coluna 4
oled_pos = [edge + 1.5 * pitch, strip_y];               // OLED centralizado nas colunas 1-3

// OLED: [largura PCB, altura PCB, largura janela, altura janela, desloc. x da janela]
oled_dims = oled == "096" ? [27.5, 27.5, 24, 13, 0] : [38.5, 12.5, 24, 7, -2];

// Placa: [largura, comprimento, altura do centro do USB acima da placa, largura do furo USB, altura do furo USB]
board_dims = board == "pico2" ? [21, 51, 1.3, 11, 7] : [18, 23.5, 1.6, 13, 7.5];
board_ledge = 1.5;      // altura dos apoios da placa
board_x = oled_pos[0];  // placa embaixo do OLED, USB saindo por tras

post_pos = [
  [wall + post_d / 2, wall + post_d / 2],
  [W - wall - post_d / 2, wall + post_d / 2],
  [wall + post_d / 2, D - wall - post_d / 2],
  [W - wall - post_d / 2, D - wall - post_d / 2],
];

// ------------------------------ helpers ------------------------------
module rounded_rect(w, d, h, r) {
  translate([r, r, 0]) linear_extrude(h) offset(r) square([w - 2 * r, d - 2 * r]);
}

module key_positions() {
  for (c = [0 : cols - 1], r = [0 : rows - 1])
    translate([edge + (c + 0.5) * pitch, edge + (r + 0.5) * pitch, 0]) children();
}

// ---------------------------- caixa inferior -------------------------
module bottom() {
  bw = board_dims[0]; bl = board_dims[1];
  usb_z = floor_t + board_ledge + 1.6 + board_dims[2];

  difference() {
    union() {
      difference() {
        rounded_rect(W, D, H, corner_r);
        translate([wall, wall, floor_t]) rounded_rect(W - 2 * wall, D - 2 * wall, H, max(0.5, corner_r - wall));
      }
      // postes dos parafusos
      for (p = post_pos) translate([p[0], p[1], 0]) cylinder(d = post_d, h = H - lip_h - 0.2);
      // berco da placa: dois apoios e duas guias laterais, encostado na parede de tras
      translate([board_x - bw / 2, D - wall - bl, floor_t]) {
        for (x = [0, bw - 2]) translate([x, 0, 0]) cube([2, bl, board_ledge]);
        for (x = [-1.6 - fit, bw + fit]) translate([x, 0, 0]) cube([1.6, bl, board_ledge + 3]);
      }
    }
    // furos dos parafusos
    for (p = post_pos) translate([p[0], p[1], floor_t]) cylinder(d = post_hole, h = H);
    // saida USB
    translate([board_x, D - wall / 2, usb_z])
      rotate([90, 0, 0])
        linear_extrude(wall + 2, center = true)
          offset(1.5) square([board_dims[3] - 3, board_dims[4] - 3], center = true);
    // rebaixos para pezinhos de borracha (8 mm)
    for (p = post_pos) translate([p[0] + (p[0] < W / 2 ? 4 : -4), p[1] + (p[1] < D / 2 ? 4 : -4), -0.01])
      cylinder(d = 8.5, h = 0.8);
  }
}

// -------------------------------- plate ------------------------------
module plate() {
  ow = oled_dims[0]; od = oled_dims[1];
  difference() {
    union() {
      rounded_rect(W, D, plate_t, corner_r);
      // aba que encaixa por dentro das paredes
      translate([0, 0, -lip_h]) difference() {
        translate([wall + fit, wall + fit, 0]) rounded_rect(W - 2 * (wall + fit), D - 2 * (wall + fit), lip_h, 1);
        translate([wall + fit + 1.6, wall + fit + 1.6, -1])
          rounded_rect(W - 2 * (wall + fit + 1.6), D - 2 * (wall + fit + 1.6), lip_h + 2, 1);
        for (p = post_pos) translate([p[0], p[1], -1]) cylinder(d = post_d + 1, h = lip_h + 2);
      }
      // moldura para segurar o OLED (fixe com cola quente ou fita dupla face)
      translate([oled_pos[0] - ow / 2 - 1.2, oled_pos[1] - od / 2 - 1.2, -2])
        difference() {
          cube([ow + 2.4, od + 2.4, 2]);
          translate([1.2, 1.2, -1]) cube([ow, od, 4]);
        }
    }
    // switches
    key_positions() translate([-sw_cut / 2, -sw_cut / 2, -5]) cube([sw_cut, sw_cut, 10]);
    // encoder
    translate([enc_pos[0], enc_pos[1], -5]) cylinder(d = enc_hole, h = 10);
    // janela do OLED (chanfrada para nao fazer sombra)
    translate([oled_pos[0] + oled_dims[4], oled_pos[1], -0.01])
      hull() {
        cube([oled_dims[2], oled_dims[3], 0.01], center = true);
        translate([0, 0, plate_t]) cube([oled_dims[2] + 2, oled_dims[3] + 2, 0.02], center = true);
      }
    // parafusos
    for (p = post_pos) translate([p[0], p[1], -5]) cylinder(d = screw_hole, h = 10);
  }
}

// -------------------------------- knob -------------------------------
module knob() {
  kd = 18; kh = 14;
  difference() {
    union() {
      cylinder(d = kd, h = kh, $fn = 64);
      // marcador
      translate([0, kd / 2 - 1.5, kh]) cylinder(d = 2, h = 0.6);
    }
    // eixo D: 6 mm com face plana a 4.5 mm
    translate([0, 0, -0.01]) intersection() {
      cylinder(d = 6.2, h = kh - 2);
      translate([-4, -3.1 + 1.5 + 0.1, 0]) cube([8, 8, kh]);
    }
    // ranhuras de pegada
    for (a = [0 : 15 : 359]) rotate(a) translate([kd / 2 + 0.3, 0, 2]) cylinder(d = 1.6, h = kh, $fn = 12);
  }
}

// ------------------------------- montagem ----------------------------
if (part == "bottom") bottom();
else if (part == "plate") translate([0, 0, plate_t]) rotate([180, 0, 0]) translate([0, -D, 0]) plate();
else if (part == "knob") knob();
else {
  color("dimgray") bottom();
  color("lightgray") translate([0, 0, H]) plate();
  color("black") translate([enc_pos[0], enc_pos[1], H + plate_t + 6]) knob();
  // switches e teclas (so para visualizacao)
  color("white", 0.6) translate([0, 0, H + plate_t]) key_positions() translate([-9, -9, 6]) cube([18, 18, 7]);
}
