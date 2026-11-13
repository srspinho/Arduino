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
screen = "lcd130";    // [lcd130, lcd147, oled091, oled096]  LCD 1.3" GMT130, LCD 1.47" GMT147SPI, OLED 0.91"/0.96"

/* [LCD 1.3" GMT130 240x240 (7 pinos) - valores da ficha tecnica] */
// Placa 27.78 x 39.22 mm; area ativa 23.4 x 23.4 mm, centralizada na largura e
// deslocada 1.58 mm para o lado dos pinos; 4 furos de 2 mm a 2.5 mm das bordas.
// lcd130_pins: de que lado ficam os pinos na caixa.
//   "left"   -> pinos para a esquerda: caixa mais curta (92 x 103 mm).
//               No firmware, gire a imagem com LCD_ROTATION (1 ou 3).
//   "bottom" -> pinos para baixo (na direcao das teclas): imagem como no
//               teste (LCD_ROTATION 0), mas a caixa fica 12 mm mais funda.
lcd130_pins = "left"; // [left, bottom]

/* [LCD 1.47" - valores do GMT147SPI (ficha tecnica GoldenMorning)] */
// O LCD fica deitado com os PINOS PARA A ESQUERDA e os 2 furos de fixacao
// para a direita (lado do encoder). No firmware: LCD_ROTATION 1.
lcd_pcb_w   = 44.0;   // comprimento da plaquinha
lcd_pcb_h   = 21.0;   // largura da plaquinha (20.96)
lcd_glass_t = 1.56;   // espessura do vidro (a placa fica 1.56 mm abaixo do plate)
// Janela = area ativa (17.39 x 32.35 mm) + ~0.4 mm de folga em cada lado.
// O vidro nao e centralizado na placa: fica a 4.4 mm da borda dos furos e
// a 3.3 mm da borda dos pinos. A area ativa tem ~1 mm de borda do lado dos
// furos e ~2.9 mm do lado do cabo flat (pinos).
lcd_win_w   = 33.2;
lcd_win_h   = 18.2;
lcd_win_dx  = 0.4;    // centro da area ativa em relacao ao centro da placa (+ = lado dos furos)
// Furos de fixacao (lado oposto aos pinos): pinos-guia na tampa
lcd_pegs    = true;
lcd_hole_x  = 20.05;  // distancia do centro da placa ate os furos, no comprimento
lcd_hole_y  = 8.3;    // distancia do centro da placa ate cada furo, na largura

/* [Teclas] */
pitch   = 19.05;      // espacamento padrao MX
cols    = 4;
rows    = 3;
sw_cut  = 14.0;       // furo do switch MX (14.0 = justo; 14.1 se ficar apertado)

/* [Caixa] */
edge     = 8;         // borda entre a grade de teclas e a lateral externa
strip    = screen == "lcd147" ? 30
         : screen == "lcd130" ? (lcd130_pins == "bottom" ? 44 : 32)
         : 22;                // faixa de tras para a tela + encoder
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
scr_pos  = [edge + 1.5 * pitch, strip_y];               // tela centralizada nas colunas 1-3

// GMT130 nas coordenadas da caixa (x = direita, y = para tras)
g130_left = lcd130_pins == "left";
g130_dims = g130_left ? [39.22, 27.78, 24, 24, -1.58, 0]
                      : [27.78, 39.22, 24, 24, 0, -1.58];
g130_pegs = [for (sx = [-1, 1], sy = [-1, 1])
               g130_left ? [sx * 17.11, sy * 11.39] : [sx * 11.39, sy * 17.11]];

// Tela: [largura PCB, altura PCB, largura janela, altura janela, desloc. x e y da janela]
scr_dims = screen == "lcd147"  ? [lcd_pcb_w, lcd_pcb_h, lcd_win_w, lcd_win_h, lcd_win_dx, 0]
         : screen == "lcd130"  ? g130_dims
         : screen == "oled096" ? [27.5, 27.5, 24, 13, 0, 0]
         :                       [38.5, 12.5, 24, 7, -2, 0];
// Pinos-guia nos furos de fixacao: posicoes (relativas ao centro da tela),
// diametro do pino, diametro do espacador e espessura do vidro
scr_pegs = screen == "lcd147" ? (lcd_pegs ? [[lcd_hole_x, lcd_hole_y], [lcd_hole_x, -lcd_hole_y]] : [])
         : screen == "lcd130" ? g130_pegs
         : [];
peg_d    = screen == "lcd130" ? 1.7 : 1.9;
spacer_d = screen == "lcd130" ? 3.2 : 3.4;
glass_t  = screen == "lcd130" ? 1.5 : lcd_glass_t;

// Placa: [largura, comprimento, altura do centro do USB acima da placa, largura do furo USB, altura do furo USB]
board_dims = board == "pico2" ? [21, 51, 1.3, 11, 7] : [18, 23.5, 1.6, 13, 7.5];
board_ledge = 1.5;      // altura dos apoios da placa
board_x = scr_pos[0];   // placa embaixo da tela, USB saindo por tras

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
  ow = scr_dims[0]; od = scr_dims[1];
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
      // 4 cantoneiras para posicionar a tela (fixe com cola quente ou fita dupla face).
      // So os cantos: os lados ficam livres para os pinos/fios, de qualquer lado.
      translate([scr_pos[0], scr_pos[1], -2.5])
        for (sx = [-1, 1], sy = [-1, 1])
          translate([sx * (ow / 2 + 0.3), sy * (od / 2 + 0.3), 0])
            mirror([sx < 0 ? 1 : 0, 0, 0]) mirror([0, sy < 0 ? 1 : 0, 0])
              difference() {
                translate([-5, -5, 0]) cube([6.2, 6.2, 2.5]);
                translate([-6, -6, -1]) cube([6, 6, 5]);
              }
      // pinos-guia nos 2 furos do LCD: um espacador da altura do vidro e um
      // pino de 1.9 mm que atravessa a placa
      for (pg = scr_pegs)
        translate([scr_pos[0] + pg[0], scr_pos[1] + pg[1], 0]) {
          translate([0, 0, -glass_t]) cylinder(d = spacer_d, h = glass_t + 0.01);
          translate([0, 0, -glass_t - 2.2]) cylinder(d = peg_d, h = 2.3, $fn = 24);
        }
    }
    // switches
    key_positions() translate([-sw_cut / 2, -sw_cut / 2, -5]) cube([sw_cut, sw_cut, 10]);
    // encoder
    translate([enc_pos[0], enc_pos[1], -5]) cylinder(d = enc_hole, h = 10);
    // janela da tela (chanfrada para nao fazer sombra)
    translate([scr_pos[0] + scr_dims[4], scr_pos[1] + scr_dims[5], -0.01])
      hull() {
        cube([scr_dims[2], scr_dims[3], 0.01], center = true);
        translate([0, 0, plate_t]) cube([scr_dims[2] + 2, scr_dims[3] + 2, 0.02], center = true);
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
