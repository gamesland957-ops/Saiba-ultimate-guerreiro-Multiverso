#!/usr/bin/env python3
"""Gera atlas T8 com cobertura antialias para o renderizador de texto da PSP."""
from PIL import Image, ImageDraw, ImageFont
from pathlib import Path

SIZE=512
FONT_SIZE=22
CHARS=(" "+ "".join(chr(i) for i in range(33,127)) +
       "ÁÀÂÃÉÊÍÓÔÕÚÜÇáàâãéêíóôõúüç")
font_path=Path("/usr/share/fonts/TTF/Lato-Bold.ttf")
if not font_path.exists():
    font_path=Path("/usr/share/fonts/truetype/lato/Lato-Bold.ttf")
if not font_path.exists():
    raise SystemExit("Lato Bold não encontrada; instale o pacote ttf-lato.")
font=ImageFont.truetype(str(font_path),FONT_SIZE)
atlas=Image.new("L",(SIZE,SIZE),0)
draw=ImageDraw.Draw(atlas)
glyphs=[]
x=y=2
row_h=0
for ch in CHARS:
    if any(g["code"]==ord(ch) for g in glyphs):
        continue
    bbox=font.getbbox(ch)
    left,top,right,bottom=bbox
    w=max(0,right-left); h=max(0,bottom-top)
    advance=max(1,int(round(font.getlength(ch))))
    if w and h:
        if x+w+2>SIZE:
            x=2; y+=row_h+3; row_h=0
        if y+h+2>SIZE:
            raise SystemExit("Atlas de fonte excedeu 512x512.")
        draw.text((x-left,y-top),ch,font=font,fill=255)
        gx,gy,gw,gh=x,y,w,h
        x+=w+3; row_h=max(row_h,h)
    else:
        gx=gy=gw=gh=0
    glyphs.append((ord(ch),gx,gy,gw,gh,left,top,advance))
    if ch==" ":
        glyphs[-1]=(ord(ch),0,0,0,0,0,0,advance)
out=Path("src/font_atlas.h")
with out.open("w",encoding="ascii") as f:
    f.write("#ifndef SOBA_FONT_ATLAS_H\n#define SOBA_FONT_ATLAS_H\n#include <stdint.h>\n")
    f.write("#define FONT_ATLAS_SIZE 512\n")
    f.write("typedef struct { uint32_t code; uint16_t x,y,w,h; int8_t left,top; uint8_t advance; } FontGlyph;\n")
    f.write("static const unsigned int FONT_GLYPH_COUNT=%d;\n"%len(glyphs))
    f.write("static const FontGlyph fontGlyphs[FONT_GLYPH_COUNT]={\n")
    for g in glyphs:
        f.write("{%d,%d,%d,%d,%d,%d,%d,%d},\n"%g)
    f.write("};\nstatic const uint8_t __attribute__((aligned(16))) fontAtlas[FONT_ATLAS_SIZE*FONT_ATLAS_SIZE]={\n")
    pix=atlas.tobytes()
    for i in range(0,len(pix),32):
        f.write(",".join(str(v) for v in pix[i:i+32])+",\n")
    f.write("};\n#endif\n")
print("Atlas Lato Bold gerado:",len(glyphs),"glifos")
