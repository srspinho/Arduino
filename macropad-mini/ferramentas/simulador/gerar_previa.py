#!/usr/bin/env python3
"""Compila o simulador, roda e gera as imagens de previa em docs/.

Uso: python3 gerar_previa.py   (precisa de g++ e pillow)
"""
import glob
import os
import shutil
import subprocess
import sys
import tempfile
from PIL import Image

AQUI = os.path.dirname(os.path.abspath(__file__))
FIRMWARE = os.path.join(AQUI, "..", "..", "firmware", "MiniPad")
DOCS = os.path.join(AQUI, "..", "..", "docs")
ZOOM = 3
MODELOS = {"180": "1.8\" 160x128", "96": "0.96\" 160x80", "144": "1.44\" 128x128"}


def moldura(img):
    """Aumenta (pixels visiveis) e poe uma borda escura, como o modulo."""
    big = img.resize((img.width * ZOOM, img.height * ZOOM), Image.NEAREST)
    pad = 6 * ZOOM // 3
    out = Image.new("RGB", (big.width + pad * 2, big.height + pad * 2), (22, 22, 26))
    out.paste(big, (pad, pad))
    return out


def rodar(modelo, tmp):
    exe = os.path.join(tmp, "sim" + modelo)
    pasta = os.path.join(tmp, modelo)
    os.makedirs(pasta, exist_ok=True)
    subprocess.check_call(["g++", "-O2", "-std=c++17", f"-DLCD_MODEL={modelo}",
                           "-I" + os.path.join(AQUI, "stub"), "-I" + FIRMWARE,
                           os.path.join(AQUI, "simulador.cpp"), "-o", exe])
    subprocess.check_call([exe, pasta])
    return pasta


def main():
    os.makedirs(DOCS, exist_ok=True)
    tmp = tempfile.mkdtemp()
    try:
        # modelo principal: telas fixas + animacao
        pasta = rodar("180", tmp)
        nomes = ["abertura", "midia", "tecla", "encoder", "segurando",
                 "trocando", "reuniao", "edicao", "f13", "descanso"]
        telas = [moldura(Image.open(os.path.join(pasta, n + ".ppm")).convert("RGB")) for n in nomes]
        cols = 5
        w, h = telas[0].size
        folha = Image.new("RGB", (cols * (w + 12) + 12, 2 * (h + 12) + 12), (12, 12, 14))
        for i, t in enumerate(telas):
            folha.paste(t, (12 + (i % cols) * (w + 12), 12 + (i // cols) * (h + 12)))
        folha.save(os.path.join(DOCS, "telas.png"))

        quadros = sorted(glob.glob(os.path.join(pasta, "anim_*.ppm")))
        gif = [Image.open(q).convert("RGB").resize((160 * 2, 128 * 2), Image.NEAREST) for q in quadros]
        gif = [g.quantize(colors=255, method=Image.Quantize.MEDIANCUT) for g in gif]
        gif[0].save(os.path.join(DOCS, "animacao.gif"), save_all=True, append_images=gif[1:],
                    duration=40, loop=0, optimize=True)

        # outros tamanhos de tela
        outras = []
        for m in ("96", "144"):
            p = rodar(m, tmp)
            outras += [moldura(Image.open(os.path.join(p, n + ".ppm")).convert("RGB")) for n in ("midia", "encoder", "reuniao")]
        w2 = max(o.width for o in outras)
        h2 = max(o.height for o in outras)
        folha2 = Image.new("RGB", (3 * (w2 + 12) + 12, 2 * (h2 + 12) + 12), (12, 12, 14))
        for i, t in enumerate(outras):
            folha2.paste(t, (12 + (i % 3) * (w2 + 12), 12 + (i // 3) * (h2 + 12)))
        folha2.save(os.path.join(DOCS, "outras_telas.png"))
        print("imagens em", os.path.abspath(DOCS))
    finally:
        shutil.rmtree(tmp)


if __name__ == "__main__":
    sys.exit(main())
