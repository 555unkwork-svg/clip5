"""Genera LEEME.html (y LEEME.pdf con Chromium, si está disponible) a partir de LEEME.md.
Ejecutar:  python3 installer/generar_manual.py"""
import base64
import os
import shutil
import subprocess

import markdown

HERE = os.path.dirname(os.path.abspath(__file__))
md = open(os.path.join(HERE, "LEEME.md"), encoding="utf-8").read()
body = markdown.markdown(md, extensions=["tables", "fenced_code", "toc"])
shot = os.path.join(HERE, "..", "docs", "img", "clip5_ui.png")
img = ""
if os.path.exists(shot):
    b = base64.b64encode(open(shot, "rb").read()).decode()
    img = f'<img class="hero" src="data:image/png;base64,{b}" alt="Interfaz de clip5">'
body = body.replace("<hr />", img + "<hr />", 1)
# imágenes del manual embebidas (el HTML viaja solo dentro de los instaladores)
import re
def _embed(m):
    f = os.path.normpath(os.path.join(HERE, m.group(1)))
    return 'src="data:image/png;base64,' + base64.b64encode(open(f, "rb").read()).decode() + '"'
body = re.sub(r'src="(\.\./docs/img/[^"]+\.png)"', _embed, body)

html = f"""<!doctype html><html lang="es"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>clip5 — Manual de usuario</title>
<style>
@page {{ size: A4; margin: 16mm 14mm; }}
body {{ font: 10.5pt/1.55 -apple-system, "Segoe UI", Inter, Helvetica, Arial, sans-serif; color: #1b1f22;
       max-width: 860px; margin: 0 auto; padding: 24px 16px; background: #fff; }}
h1 {{ font-size: 24pt; margin: 0 0 4px; letter-spacing: -0.5px; }}
h2 {{ font-size: 15pt; margin: 28px 0 8px; padding-top: 10px; border-top: 2px solid #6FE3C1; break-after: avoid; }}
h3 {{ font-size: 12pt; margin: 18px 0 6px; break-after: avoid; }}
a {{ color: #0d7a5d; text-decoration: none; }}
table {{ border-collapse: collapse; width: 100%; margin: 8px 0 14px; font-size: 9.5pt; break-inside: avoid; }}
th, td {{ border-bottom: 1px solid #d9dee2; padding: 5px 8px; text-align: left; vertical-align: top; }}
th {{ background: #f2f5f6; font-weight: 600; }}
code {{ font: 9pt "JetBrains Mono", Menlo, Consolas, monospace; background: #f2f5f6; padding: 1px 4px; border-radius: 3px; }}
pre {{ background: #121416; color: #ECE8E1; padding: 12px; border-radius: 6px; overflow-x: auto; font-size: 8pt; line-height: 1.35; break-inside: avoid; }}
pre code {{ background: none; color: inherit; padding: 0; }}
blockquote {{ margin: 10px 0; padding: 8px 12px; border-left: 3px solid #FF6A4D; background: #fff4f1; }}
hr {{ border: 0; border-top: 1px solid #d9dee2; margin: 18px 0; }}
img.hero {{ width: 100%; border-radius: 8px; margin: 14px 0 4px; }}
img {{ max-width: 100%; border-radius: 8px; }}
ol, ul {{ padding-left: 22px; }}
</style></head><body>{body}</body></html>"""
out = os.path.join(HERE, "LEEME.html")
open(out, "w", encoding="utf-8").write(html)
print("LEEME.html")

node = shutil.which("node")
if node:
    js = """const {chromium}=require('playwright');(async()=>{const b=await chromium.launch();const p=await b.newPage();
await p.goto('file://'+process.argv[1]);await p.pdf({path:process.argv[2],format:'A4',printBackground:true,
displayHeaderFooter:true,headerTemplate:'<span></span>',
footerTemplate:'<div style="font-size:8px;width:100%;text-align:center;color:#8C949A">clip5 · Manual de usuario · <span class=pageNumber></span>/<span class=totalPages></span></div>',
margin:{top:'14mm',bottom:'16mm',left:'12mm',right:'12mm'}});await b.close();})();"""
    try:
        subprocess.run([node, "-e", js, out, os.path.join(HERE, "LEEME.pdf")], check=True)
        print("LEEME.pdf")
    except Exception as e:  # sin Chromium: solo HTML
        print("PDF no generado:", e)
