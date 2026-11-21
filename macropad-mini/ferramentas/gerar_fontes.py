#!/usr/bin/env python3
"""Gera firmware/MiniPad/fonts.h a partir das fontes Inter (licenca SIL OFL).

Cada letra vira um bitmap de 4 bits por pixel (16 niveis de transparencia),
o que deixa o texto suave (anti-aliasing) na tela colorida.

Uso:  python3 gerar_fontes.py [pasta_das_fontes_inter]
Precisa de: pip install pillow
"""
import os
import sys
from PIL import Image, ImageDraw, ImageFont

FONT_DIR = sys.argv[1] if len(sys.argv) > 1 else "/usr/share/fonts/opentype/inter"
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "firmware", "MiniPad", "fonts.h")

# nome no C, arquivo, tamanho em pixels
FONTS = [
    ("FONT_SMALL", "Inter-SemiBold.otf", 10),      # rotulos das teclas
    ("FONT_MED", "Inter-Bold.otf", 12),            # cabecalho e dicas
    ("FONT_BIG", "InterDisplay-Bold.otf", 19),     # ultima acao
    ("FONT_LOGO", "InterDisplay-Black.otf", 26),   # animacao de abertura
]
# a fonte do logo so precisa de letras e numeros (economiza ~30 KB de flash)
ONLY_ASCII = {"FONT_LOGO"}

# ASCII + acentos do portugues (Latin-1) + alguns simbolos
CHARS = [chr(c) for c in range(32, 127)] + [chr(c) for c in range(0xA0, 0x100)]
SYMBOLS = "←→↑↓◀▶●○•♪↺↻−×✓⏎–—…"


def has_glyph(font, ch):
    """A Inter nao tem todos os simbolos; compara com um caractere que nao existe."""
    if ch == " " or ord(ch) < 0x2000:
        return True
    def render(c):
        img = Image.new("L", (64, 64), 0)
        ImageDraw.Draw(img).text((8, 8), c, font=font, fill=255)
        return img.tobytes()
    return render(ch) != render(chr(0xE000))


def build(cname, path, size):
    font = ImageFont.truetype(path, size)
    ascent, descent = font.getmetrics()
    pool = [chr(c) for c in range(32, 127)] if cname in ONLY_ASCII else CHARS + list(SYMBOLS)
    chars = [c for c in pool if has_glyph(font, c)]
    glyphs, bitmap = [], bytearray()
    for ch in sorted(set(chars), key=ord):
        adv = int(round(font.getlength(ch)))
        bbox = font.getbbox(ch)
        w, h = bbox[2] - bbox[0], bbox[3] - bbox[1]
        if w <= 0 or h <= 0:
            glyphs.append((ord(ch), 0, 0, 0, 0, adv, len(bitmap)))
            continue
        img = Image.new("L", (w, h), 0)
        ImageDraw.Draw(img).text((-bbox[0], -bbox[1]), ch, font=font, fill=255)
        px = img.load()
        off = len(bitmap)
        for y in range(h):
            for x in range(0, w, 2):
                a = px[x, y] >> 4
                b = (px[x + 1, y] >> 4) if x + 1 < w else 0
                bitmap.append((a << 4) | b)
        # yoff: distancia da linha de base ate o topo do desenho (negativo = acima)
        glyphs.append((ord(ch), w, h, bbox[0], bbox[1] - ascent, adv, off))

    lines = [f"// {cname}: {os.path.basename(path)} {size}px, {len(glyphs)} caracteres, {len(bitmap)} bytes"]
    lines.append(f"static const uint8_t {cname}_bits[] PROGMEM = {{")
    for i in range(0, len(bitmap), 24):
        lines.append("  " + ",".join(f"0x{b:02X}" for b in bitmap[i:i + 24]) + ",")
    lines.append("};")
    lines.append(f"static const Glyph {cname}_glyphs[] PROGMEM = {{")
    for g in glyphs:
        lines.append("  {0x%04X,%d,%d,%d,%d,%d,%d}," % g)
    lines.append("};")
    lines.append(f"static const Font {cname} = {{ {cname}_bits, {cname}_glyphs, {len(glyphs)}, "
                 f"{ascent + descent}, {ascent} }};")
    return "\n".join(lines), len(bitmap)


def main():
    out = [
        "// ARQUIVO GERADO por ferramentas/gerar_fontes.py - nao edite a mao.",
        "// Fonte: Inter (https://rsms.me/inter) - SIL Open Font License 1.1",
        "#pragma once",
        "#include <stdint.h>",
        "#ifndef PROGMEM",
        "#define PROGMEM",
        "#endif",
        "",
        "struct Glyph {",
        "  uint16_t code;      // Unicode",
        "  uint8_t w, h;       // tamanho do bitmap",
        "  int8_t xoff, yoff;  // posicao em relacao ao cursor / linha de base",
        "  uint8_t adv;        // quanto o cursor anda",
        "  uint32_t offset;    // inicio no vetor de bits (4 bits por pixel)",
        "};",
        "",
        "struct Font {",
        "  const uint8_t *bits;",
        "  const Glyph *glyphs;",
        "  uint16_t count;",
        "  uint8_t lineH, ascent;",
        "};",
        "",
    ]
    total = 0
    for cname, fname, size in FONTS:
        text, n = build(cname, os.path.join(FONT_DIR, fname), size)
        out.append(text)
        out.append("")
        total += n
    with open(OUT, "w", encoding="utf-8") as f:
        f.write("\n".join(out))
    print(f"{OUT}: {total} bytes de bitmap")


if __name__ == "__main__":
    main()
