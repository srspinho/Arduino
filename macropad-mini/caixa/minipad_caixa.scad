// =====================================================================
//  MiniPad (5 teclas + encoder + LCD ST7735S): caixa para impressao 3D
//
//  Pecas (mude "part" ou use o Customizer do OpenSCAD):
//    "all"         -> visualizacao montada
//    "bottom"      -> caixa inferior (imprimir com o fundo na mesa)
//    "plate"       -> tampa/plate (ja sai virada: imprimir com o topo na mesa)
//    "knob"        -> botao do encoder (eixo D de 6 mm, EC11)
//    "teste_furos" -> 5 furos de switch de 14.0 a 14.4 mm (ache o seu sw_cut)
//    "teste_tela"  -> so o pedaco da tampa com a janela da tela: imprima
//                     primeiro (5 min) e confira se o seu modulo encaixa
//
//  Fixacao: 4 parafusos M3 x 8 mm (cabeca panela) direto no plastico
//  (furo piloto de 2.6 mm). Para insertos termicos M3, use post_hole = 4.2.
//
//  Medidas (tela 0.96"): caixa de 111 x 67 mm e 20 mm de altura (sem as teclas).
// =====================================================================

part   = "all";       // [all, bottom, plate, knob, teste_furos, teste_tela]
screen = "lcd096";    // [lcd096, lcd180, lcd144]  0.96" 80x160, 1.8" 128x160, 1.44" 128x128

/* [Tela - MECA O SEU MODULO com paquimetro!] */
// Os modulos ST7735S variam um pouco entre fabricantes. Os valores abaixo sao
// os mais comuns. Confira com a peca "teste_tela" antes de imprimir a tampa.
//
// A tela fica DEITADA, com a fileira de pinos para a ESQUERDA
// (no firmware: LCD_ROTATION 1; se a imagem sair invertida, use 3).
//
// [largura da placa (lado dos pinos ate o outro lado), altura da placa,
//  largura da area ativa, altura da area ativa,
//  deslocamento da area ativa em relacao ao centro da placa (x: + = para a
//  direita, longe dos pinos; y: + = para tras),
//  altura do vidro acima da placa (de cima da placa ate o topo do vidro)]
lcd180 = [56.0, 34.0, 35.04, 28.03, 2.6, 0, 2.6];   // 1.8" placa vermelha de 8 pinos
lcd096 = [30.0, 24.0, 21.70, 10.80, 1.0, 0, 1.6];   // 0.96" IPS 80x160
lcd144 = [38.0, 32.0, 25.50, 26.50, 2.0, 0, 2.4];   // 1.44" 128x128
win_margin = 0.4;     // folga da janela em volta da area ativa (cada lado)
// Furos de fixacao do modulo: pinos-guia na tampa (desligado: as medidas
// variam muito; com eles, a tela fica presa sem cola)
lcd_pegs = false;
lcd_hole_inset = 2.5; // distancia do centro dos furos ate as bordas da placa
lcd_peg_d = 2.3;      // diametro do pino (furo M2.5/M3 = 2.6-3.2 mm)

/* [Teclas] */
pitch   = 19.05;      // espacamento padrao MX
keys    = 5;
sw_cut  = 14.2;       // mesmo valor que funcionou no MacroPad 4x3
sw_chamfer = 0.4;     // chanfro na entrada do furo (compensa o "pe de elefante")

/* [Caixa] */
edge     = 8;         // borda entre as teclas e a lateral externa
// faixa de tras para a tela + encoder (cresce com o tamanho da tela)
strip    = screen == "lcd180" ? 44 : screen == "lcd144" ? 42 : 32;
corner_r = 4;
wall     = 2.4;
floor_t  = 2.4;
inner_h  = 16;        // altura interna livre (fundo -> baixo do plate)
plate_t  = 1.5;       // 1.5 mm = espessura padrao para os clipes do switch MX
lip_h    = 2.5;       // aba do plate que encaixa dentro das paredes
fit      = 0.3;       // folga de encaixe
post_d   = 7;
post_hole  = 2.6;     // M3 auto-atarraxante (4.2 para inserto termico)
screw_hole = 3.4;

/* [Encoder EC11] */
enc_hole = 7.4;

/* [Placa RP2040-Zero] */
board_dims = [18, 23.5];   // largura, comprimento
usb_w = 13; usb_h = 7.5;   // furo do USB-C
board_ledge = 1.5;         // altura dos apoios da placa

$fn = 48;

// ------------------------- medidas derivadas -------------------------
scr = screen == "lcd096" ? lcd096 : screen == "lcd144" ? lcd144 : lcd180;
W = keys * pitch + 2 * edge;
D = pitch + strip + 2 * edge;
H = floor_t + inner_h;

strip_y = edge + pitch + strip / 2;                    // centro da faixa de tras
enc_pos = [edge + (keys - 0.5) * pitch, strip_y];      // encoder em cima da tecla 5
// tela centralizada no espaco entre a borda e o encoder
scr_pos = [(edge + enc_pos[0] - 11) / 2 + 1, strip_y];
win_pos = [scr_pos[0] + scr[4], scr_pos[1] + scr[5]];
win = [scr[2] + 2 * win_margin, scr[3] + 2 * win_margin];
glass_t = scr[6];

board_x = scr_pos[0];      // placa embaixo da tela, USB saindo por tras

post_pos = [
  [wall + post_d / 2, wall + post_d / 2],
  [W - wall - post_d / 2, wall + post_d / 2],
  [wall + post_d / 2, D - wall - post_d / 2],
  [W - wall - post_d / 2, D - wall - post_d / 2],
];

echo(str("Caixa: ", W, " x ", D, " mm, altura ", H + plate_t, " mm"));

// ------------------------------ ajudantes ----------------------------
module rounded_rect(w, d, h, r) {
  translate([r, r, 0]) linear_extrude(h) offset(r) square([w - 2 * r, d - 2 * r]);
}

module key_positions() {
  for (c = [0 : keys - 1])
    translate([edge + (c + 0.5) * pitch, edge + pitch / 2, 0]) children();
}

// ---------------------------- caixa inferior -------------------------
module bottom() {
  bw = board_dims[0]; bl = board_dims[1];
  usb_z = floor_t + board_ledge + 1.6 + 1.6;   // centro do conector USB-C

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
        // batente na frente da placa (ela nao escorrega para dentro ao plugar o cabo)
        translate([0, -1.6 - fit, 0]) cube([bw, 1.6, board_ledge + 2.5]);
      }
    }
    // furos dos parafusos
    for (p = post_pos) translate([p[0], p[1], floor_t]) cylinder(d = post_hole, h = H);
    // saida USB
    translate([board_x, D - wall / 2, usb_z])
      rotate([90, 0, 0])
        linear_extrude(wall + 2, center = true)
          offset(1.5) square([usb_w - 3, usb_h - 3], center = true);
    // rebaixos para pezinhos de borracha (8 mm)
    for (p = post_pos) translate([p[0] + (p[0] < W / 2 ? 4 : -4), p[1] + (p[1] < D / 2 ? 4 : -4), -0.01])
        cylinder(d = 8.5, h = 0.8);
  }
}

// Furo do switch: quadrado reto + chanfro na face de cima
module switch_hole(size) {
  translate([-size / 2, -size / 2, -5]) cube([size, size, 10]);
  hull() {
    translate([-size / 2, -size / 2, plate_t - sw_chamfer]) cube([size, size, 0.01]);
    translate([-size / 2 - sw_chamfer, -size / 2 - sw_chamfer, plate_t])
      cube([size + 2 * sw_chamfer, size + 2 * sw_chamfer, 0.02]);
  }
}

module test_holes() {
  n = 5;
  difference() {
    rounded_rect(n * pitch + 6, pitch + 10, plate_t, 2);
    for (i = [0 : n - 1]) {
      translate([3 + (i + 0.5) * pitch, 3 + pitch / 2, 0]) switch_hole(14.0 + i * 0.1);
      // marcas: 1 furinho = 14.0, 2 = 14.1 ... 5 = 14.4
      for (k = [0 : i])
        translate([3 + (i + 0.5) * pitch - i * 1.25 + k * 2.5, pitch + 6.5, -1])
          cylinder(d = 1.4, h = 5, $fn = 12);
    }
  }
}

// ----------------------- suporte da tela (tampa) ---------------------
// Fica embaixo da tampa: 4 cantoneiras que seguram a PLACA do modulo
// (o vidro encosta na tampa por baixo) e, opcional, pinos nos furos.
module screen_holder() {
  ow = scr[0]; od = scr[1];
  hh = glass_t + 1.6 + 1.2;   // vidro + placa + um pouco de sobra para a cola
  translate([scr_pos[0], scr_pos[1], -hh])
    for (sx = [-1, 1], sy = [-1, 1])
      translate([sx * (ow / 2 + fit), sy * (od / 2 + fit), 0])
        mirror([sx < 0 ? 1 : 0, 0, 0]) mirror([0, sy < 0 ? 1 : 0, 0])
          difference() {
            translate([-6, -6, 0]) cube([7.4, 7.4, hh]);
            translate([-7, -7, -1]) cube([7, 7, hh + 2]);
          }
  if (lcd_pegs)
    for (sx = [-1, 1], sy = [-1, 1])
      translate([scr_pos[0] + sx * (ow / 2 - lcd_hole_inset), scr_pos[1] + sy * (od / 2 - lcd_hole_inset), 0]) {
        translate([0, 0, -glass_t]) cylinder(d = lcd_peg_d + 2, h = glass_t + 0.01);
        translate([0, 0, -glass_t - 2.2]) cylinder(d = lcd_peg_d, h = 2.3, $fn = 24);
      }
}

// Janela da tela, chanfrada (45 graus) para nao fazer sombra na imagem
module screen_window() {
  translate([win_pos[0], win_pos[1], -0.01])
    hull() {
      cube([win[0], win[1], 0.01], center = true);
      translate([0, 0, plate_t]) cube([win[0] + 2 * plate_t, win[1] + 2 * plate_t, 0.02], center = true);
    }
}

// -------------------------------- plate ------------------------------
module plate() {
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
      screen_holder();
    }
    key_positions() switch_hole(sw_cut);
    translate([enc_pos[0], enc_pos[1], -5]) cylinder(d = enc_hole, h = 10);
    screen_window();
    for (p = post_pos) translate([p[0], p[1], -5]) cylinder(d = screw_hole, h = 10);
    // escareado leve na cabeca dos parafusos
    for (p = post_pos) translate([p[0], p[1], plate_t - 0.6]) cylinder(d1 = screw_hole, d2 = screw_hole + 1.2, h = 0.61);
  }
}

// Pedaco da tampa so com a tela, para testar o encaixe do modulo
module screen_test() {
  m = 9;
  ow = scr[0] + 2 * m; od = scr[1] + 2 * m;
  intersection() {
    plate();
    translate([scr_pos[0] - ow / 2, scr_pos[1] - od / 2, -20]) cube([ow, od, 40]);
  }
}

// -------------------------------- knob -------------------------------
module knob() {
  kd = 18; kh = 14;
  difference() {
    union() {
      cylinder(d = kd, h = kh, $fn = 64);
      translate([0, kd / 2 - 1.5, kh]) cylinder(d = 2, h = 0.6);   // marcador
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
module flip() { translate([0, 0, plate_t]) rotate([180, 0, 0]) translate([0, -D, 0]) children(); }

if (part == "bottom") bottom();
else if (part == "plate") flip() plate();
else if (part == "knob") knob();
else if (part == "teste_furos") translate([0, 0, plate_t]) rotate([180, 0, 0]) test_holes();
else if (part == "teste_tela") flip() screen_test();
else {
  {
    color("dimgray") bottom();
    color("lightgray") translate([0, 0, H]) plate();
    color("black") translate([enc_pos[0], enc_pos[1], H + plate_t + 6]) knob();
    // tela (so para visualizacao)
    translate([0, 0, H]) {
      color("darkred") translate([scr_pos[0] - scr[0] / 2, scr_pos[1] - scr[1] / 2, -glass_t - 1.6]) cube([scr[0], scr[1], 1.6]);
      color("#123") translate([win_pos[0] - scr[2] / 2, win_pos[1] - scr[3] / 2, -glass_t]) cube([scr[2], scr[3], glass_t - 0.05]);
    }
    // teclas (so para visualizacao)
    color("whitesmoke") translate([0, 0, H + plate_t]) key_positions() translate([-9, -9, 6]) cube([18, 18, 7]);
  }
}
