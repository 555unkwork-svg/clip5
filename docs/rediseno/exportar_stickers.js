// Exporta los stickers (SVG con troquelado) a PNG transparentes de 512 px para el plugin.
// Uso: node docs/rediseno/exportar_stickers.js plugin/Resources/stickers
const { chromium } = require('playwright');
const path = require('path');
(async () => {
  const out = path.resolve(process.argv[2]);
  const b = await chromium.launch();
  const p = await b.newPage({ viewport: { width: 600, height: 600 } });
  await p.goto('file://' + path.join(__dirname, 'sticker_render.html'));
  await p.waitForTimeout(800);
  const names = await p.evaluate(() => Object.keys(Stickers));
  for (const n of names) {
    await p.evaluate(n => { document.getElementById('s').innerHTML = Stickers[n](); }, n);
    await p.waitForTimeout(120);
    const el = await p.$('#s');
    await el.screenshot({ path: path.join(out, n + '.png'), omitBackground: true });
  }
  console.log('exportados:', names.join(', '));
  await b.close();
})();
