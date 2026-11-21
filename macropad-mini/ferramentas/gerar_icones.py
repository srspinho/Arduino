#!/usr/bin/env python3
"""Gera firmware/MiniPad/icons.h com icones coloridos (Noto Color Emoji, SIL OFL).

No config.h, basta comecar o nome da acao com um dos emojis abaixo:
    MEDIA("⏯ Play/Pause", KEY_PLAY_PAUSE)
e a tecla mostra o icone na tela. Para ter outro icone, acrescente o
emoji na lista ICONES e rode o script de novo.

Uso:  python3 gerar_icones.py [caminho/NotoColorEmoji.ttf]
Precisa de: pip install pillow
"""
import os
import sys
from PIL import Image, ImageDraw, ImageFont

FONT = sys.argv[1] if len(sys.argv) > 1 else "/usr/share/fonts/truetype/noto/NotoColorEmoji.ttf"
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "firmware", "MiniPad", "icons.h")

ICONES = (
    "⏮⏯⏭⏹⏺⏪⏩🔉🔊🔇🎵🎧"        # midia
    "🖥💻🧮🔒📸🗂📁⌨🖱🌐"        # computador
    "🎤🎙📷📹✋📞🚪🔕💬"        # reuniao
    "📋📄✂💾↩↪🗑🔍✏📌"          # edicao
    "⭐🔥💡⚙🎮🎬🔴🟢😀👍☕🚀🏠"   # diversos
)
SIZES = [("L", 22), ("S", 14)]   # teclas / dicas do encoder


def render(ch, size, font):
    # a fonte so existe em 109 px: desenha grande e reduz com boa qualidade
    big = Image.new("RGBA", (160, 160), (0, 0, 0, 0))
    ImageDraw.Draw(big).text((8, 8), ch + "️", font=font, embedded_color=True)
    box = big.getbbox()
    if not box:
        return None
    crop = big.crop(box)
    side = max(crop.size)
    sq = Image.new("RGBA", (side, side), (0, 0, 0, 0))
    sq.paste(crop, ((side - crop.width) // 2, (side - crop.height) // 2))
    return sq.resize((size, size), Image.LANCZOS)


def main():
    font = ImageFont.truetype(FONT, 109)
    chars = []
    for ch in ICONES:
        if ch not in chars:
            chars.append(ch)
    out = [
        "// ARQUIVO GERADO por ferramentas/gerar_icones.py - nao edite a mao.",
        "// Icones: Noto Color Emoji (Google) - SIL Open Font License 1.1",
        "#pragma once",
        "#include <stdint.h>",
        "#ifndef PROGMEM",
        "#define PROGMEM",
        "#endif",
        "",
        "// Cada pixel: cor RGB565 (2 bytes) + transparencia (1 byte, 0-255)",
        "struct Icon {",
        "  uint32_t code;          // emoji (Unicode)",
        "  const uint16_t *color;",
        "  const uint8_t *alpha;",
        "};",
        "",
    ]
    total = 0
    for tag, size in SIZES:
        entries = []
        for ch in sorted(chars, key=ord):
            img = render(ch, size, font)
            if img is None:
                print("sem desenho para", ch, hex(ord(ch)))
                continue
            name = f"IC{tag}_{ord(ch):X}"
            pa = img.load()
            px = [pa[x, y] for y in range(size) for x in range(size)]
            cols = []
            for r, g, b, a in px:
                cols.append(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))
            out.append(f"static const uint16_t {name}_c[] PROGMEM = {{" +
                       ",".join(f"0x{c:04X}" for c in cols) + "};")
            out.append(f"static const uint8_t {name}_a[] PROGMEM = {{" +
                       ",".join(str(p[3]) for p in px) + "};")
            entries.append(f"  {{0x{ord(ch):X}, {name}_c, {name}_a}},  // {ch}")
            total += size * size * 3
        out.append(f"static const uint8_t ICON_{tag}_SIZE = {size};")
        out.append(f"static const Icon ICONS_{tag}[] PROGMEM = {{")
        out += entries
        out.append("};")
        out.append(f"static const uint16_t ICONS_{tag}_COUNT = {len(entries)};")
        out.append("")
    with open(OUT, "w", encoding="utf-8") as f:
        f.write("\n".join(out))
    print(f"{OUT}: {len(chars)} icones, {total} bytes")


if __name__ == "__main__":
    main()
