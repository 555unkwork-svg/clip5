// clip5 — maqueta de la nueva interfaz (1040 × 640 px lógicos), parametrizada por tema.
(function (g) {
  const W = 1040, H = 640;

  // ---------------------------------------------------------------- utilidades
  function rnd(seed) { return () => { seed |= 0; seed = seed + 0x6D2B79F5 | 0; let t = Math.imul(seed ^ seed >>> 15, 1 | seed); t = t + Math.imul(t ^ t >>> 7, 61 | t) ^ t; return ((t ^ t >>> 14) >>> 0) / 4294967296; }; }
  const esc = s => String(s);

  // curvas (mismas fórmulas que el motor)
  function knee(v, w) { if (w <= 0) return Math.min(v, 1); const a = 1 - w / 2; if (v <= a) return v; const t = Math.min((v - a) / w, 1); return a + w * (t - t ** 3 + .5 * t ** 4); }
  function curve(u, mode) { if (mode === 3) return u; const w = [.5, .16, 0][mode]; return knee(u, w); }
  function curveIcon(mode, color, size) {
    let d = "";
    for (let i = 0; i <= 40; i++) { const u = 1.5 * i / 40, y = Math.min(1.5, curve(u, mode)); d += (i ? "L" : "M") + (4 + (size - 8) * u / 1.5).toFixed(1) + " " + (size - 4 - (size - 8) * y / 1.5).toFixed(1); }
    const yC = size - 4 - (size - 8) / 1.5;
    return `<svg width="${size}" height="${size}" viewBox="0 0 ${size} ${size}"><path d="M4 ${yC.toFixed(1)} H${size - 4}" stroke="${color}" stroke-opacity=".35" stroke-dasharray="2 2"/><path d="${d}" fill="none" stroke="${color}" stroke-width="2.2" stroke-linecap="round"/></svg>`;
  }

  function knob(size, value01, c, opts = {}) {
    const r = size / 2, cx = r, cy = r, R = r - 3;
    const a0 = 135, a1 = 405, a = a0 + (a1 - a0) * value01;
    const pt = (ang, rr) => [cx + rr * Math.cos(ang * Math.PI / 180), cy + rr * Math.sin(ang * Math.PI / 180)];
    const arc = (from, to, rr) => { const [x0, y0] = pt(from, rr), [x1, y1] = pt(to, rr); return `M${x0.toFixed(2)} ${y0.toFixed(2)} A${rr} ${rr} 0 ${to - from > 180 ? 1 : 0} 1 ${x1.toFixed(2)} ${y1.toFixed(2)}`; };
    const from = opts.bipolar ? 270 : a0;
    const [ix, iy] = pt(a, R * .62), [jx, jy] = pt(a, R * .28);
    const sw = Math.max(2.5, size * .055);
    return `<svg width="${size}" height="${size}" viewBox="0 0 ${size} ${size}">
      <defs><radialGradient id="kg${size}" cx=".4" cy=".35" r=".75"><stop offset="0" stop-color="#fff" stop-opacity=".18"/><stop offset="1" stop-color="#000" stop-opacity=".18"/></radialGradient></defs>
      <path d="${arc(a0, a1, R)}" stroke="${c.txt2}" stroke-opacity=".28" stroke-width="${sw}" fill="none" stroke-linecap="round"/>
      ${Math.abs(a - from) > .5 ? `<path d="${arc(Math.min(a, from), Math.max(a, from), R)}" stroke="${c.sel}" stroke-width="${sw}" fill="none" stroke-linecap="round"/>` : ""}
      <circle cx="${cx}" cy="${cy}" r="${R - sw * 1.7}" fill="${c.perilla}"/>
      <circle cx="${cx}" cy="${cy}" r="${R - sw * 1.7}" fill="url(#kg${size})"/>
      <line x1="${jx}" y1="${jy}" x2="${ix}" y2="${iy}" stroke="#fff" stroke-width="${Math.max(2, size * .04)}" stroke-linecap="round"/>
      ${opts.focus ? `<circle cx="${cx}" cy="${cy}" r="${R + 1}" fill="none" stroke="${c.foco}" stroke-width="2.5"/>` : ""}
    </svg>`;
  }

  function signal(seed = 7, n = 300) {
    const r = rnd(seed); const raw = [];
    let base = -17;
    for (let i = 0; i < n; i++) {
      base += (r() - .5) * 1.1; base = Math.max(-22, Math.min(-13, base));
      const beat = i % 37, kick = beat < 7 ? 15 * Math.exp(-beat / 2.2) : 0;
      const sn = (i + 18) % 37, snare = sn < 5 ? 11 * Math.exp(-sn / 1.8) : 0;
      raw.push(base + Math.max(kick, snare) + (r() - .5) * 2.2);
    }
    // suavizado ligero (lectura de picos por columna, como el historial real)
    return raw.map((v, i) => Math.max(v, ((raw[i - 1] ?? v) + v + (raw[i + 1] ?? v)) / 3 - .5));
  }

  // ---------------------------------------------------------------- gráfico de niveles
  function graph(c, x, y, w, h, opts) {
    const hyb = !!c.grafBase;
    const panel = hyb ? c.grafBase : c.graf, plot = c.graf;
    const tx = hyb ? c.grafTxt : c.txt, sc = hyb ? c.grafEscala : c.txt2, btn = hyb ? c.grafBoton : c.sup;
    const px = x + 12, py = y + 54, pw = w - 58, ph = h - 118;
    const dbTop = 3, dbBot = -33, Y = d => py + (dbTop - Math.max(dbBot, Math.min(dbTop, d))) / (dbTop - dbBot) * ph;
    const ceil = -1, gain = 3;
    const s = signal(opts.seed || 7, 300);
    const X = i => px + i / (s.length - 1) * pw;
    let inPath = `M${px} ${py + ph}`, outPath = "", over = "", dens = "";
    s.forEach((v, i) => {
      inPath += ` L${X(i).toFixed(1)} ${Y(v).toFixed(1)}`;
      const pre = Math.min(ceil + 4, v + gain + (v < -10 ? 2 * Math.min(1, (-10 - v) / 10) : 0));
      const out = Math.min(pre, ceil);
      outPath += (i ? " L" : "M") + `${X(i).toFixed(1)} ${Y(out).toFixed(1)}`;
      if (pre > ceil + .2) over += `<rect x="${(X(i) - 1.2).toFixed(1)}" y="${Y(pre).toFixed(1)}" width="2.6" height="${(Y(ceil) - Y(pre)).toFixed(1)}" fill="${c.overArea}"/>`;
      const dg = Math.max(0, Math.min(1, (-v - 6) / 14));
      dens += `<rect x="${(X(i) - 1.1).toFixed(1)}" y="${(py + ph + 4 + 12 * (1 - dg)).toFixed(1)}" width="2.4" height="${(12 * dg + 1).toFixed(1)}" fill="${c.dens}"/>`;
    });
    inPath += ` L${px + pw} ${py + ph} Z`;
    let grid = "", lab = "";
    for (let d = 0; d >= -30; d -= 6) { grid += `<line x1="${px}" x2="${px + pw}" y1="${Y(d)}" y2="${Y(d)}" stroke="${sc}" stroke-opacity=".18"/>`; lab += `<text x="${px + pw + 8}" y="${Y(d) + 4}" font-family="Mono" font-size="10" fill="${sc}">${d === 0 ? "0" : "−" + Math.abs(d)}</text>`; }
    for (let k = 0; k <= 4; k++) { const xx = px + pw * k / 4; grid += `<line x1="${xx}" x2="${xx}" y1="${py}" y2="${py + ph}" stroke="${sc}" stroke-opacity=".12"/>`; lab += `<text x="${xx - (k === 4 ? 10 : 0)}" y="${py - 6}" font-family="Mono" font-size="10" fill="${sc}">${k === 4 ? "0 s" : "−" + (8 - 2 * k) + " s"}</text>`; }
    lab += `<text x="${px + pw + 8}" y="${py + ph + 2}" font-family="Mono" font-size="9" fill="${sc}">dBFS</text>`;
    const b = (label, bx, active) => `<g><rect x="${bx}" y="${y + 12}" width="${label.length * 6.6 + 22}" height="24" rx="6" fill="${active ? c.sel : btn}" stroke="${hyb ? "#5d77b3" : c.txt2}" stroke-opacity="${active ? 0 : .35}"/><text x="${bx + 11}" y="${y + 28}" font-family="Inter" font-size="11.5" fill="${active ? c.selTxt : tx}">${label}</text></g>`;
    const bw = l => l.length * 6.6 + 22;
    let bx = x + w - 12; const labels = ["Avanzado", "Medidores", "Curva E/S"]; let btns = "";
    labels.forEach(l => { bx -= bw(l); btns += b(l, bx, opts.panel === l); bx -= 8; });
    const legY = y + h - 16;
    const legend = `<g font-family="Inter" font-size="11" fill="${sc}">
      <rect x="${px}" y="${legY - 8}" width="10" height="8" fill="${c.inFill}" stroke="${c.inBorde}"/><text x="${px + 15}" y="${legY}">Entrada</text>
      <line x1="${px + 72}" x2="${px + 84}" y1="${legY - 4}" y2="${legY - 4}" stroke="${c.outCont}" stroke-width="4"/><line x1="${px + 72}" x2="${px + 84}" y1="${legY - 4}" y2="${legY - 4}" stroke="${c.outLine}" stroke-width="2"/><text x="${px + 89}" y="${legY}">Salida</text>
      <line x1="${px + 140}" x2="${px + 156}" y1="${legY - 4}" y2="${legY - 4}" stroke="${c.over}" stroke-width="2" stroke-dasharray="4 3"/><text x="${px + 161}" y="${legY}">Sobre el techo</text>
      <rect x="${px + 250}" y="${legY - 9}" width="3" height="9" fill="${c.dens}"/><rect x="${px + 254}" y="${legY - 6}" width="3" height="6" fill="${c.dens}"/><text x="${px + 262}" y="${legY}">Densidad</text></g>
      <g><rect x="${px + pw - 118}" y="${legY - 14}" width="130" height="20" rx="5" fill="${hyb ? c.grafBoton : c.sup}"/><circle cx="${px + pw - 108}" cy="${legY - 4}" r="3" fill="${c.over}"/><text x="${px + pw - 100}" y="${legY}" font-family="Mono" font-size="10.5" fill="${c.over}">Techo −1,0 dBFS</text></g>`;
    return `<svg x="${x}" y="${y}" width="${w}" height="${h}" viewBox="${x} ${y} ${w} ${h}" overflow="visible" style="position:absolute;left:${x}px;top:${y}px">
      <rect x="${x}" y="${y}" width="${w}" height="${h}" rx="10" fill="${panel}"/>
      <rect x="${px}" y="${py}" width="${pw}" height="${ph}" fill="${plot}"/>
      ${grid}${lab}
      <path d="${inPath}" fill="${c.inFill}" fill-opacity=".9" stroke="${c.inBorde}" stroke-width="1.2"/>
      <path d="${outPath}" fill="none" stroke="${c.outCont}" stroke-width="3.6" stroke-linejoin="round"/>
      <path d="${outPath}" fill="none" stroke="${c.outLine}" stroke-width="1.6" stroke-linejoin="round"/>
      ${over}
      <line x1="${px}" x2="${px + pw}" y1="${Y(ceil)}" y2="${Y(ceil)}" stroke="${c.over}" stroke-width="1.6" stroke-dasharray="6 4"/>
      <rect x="${px}" y="${py + ph + 2}" width="${pw}" height="18" fill="${plot}"/>${dens}
      ${btns}${legend}
    </svg>`;
  }

  // ---------------------------------------------------------------- piezas
  const card = (x, y, w, h, c, extra = "") => `<div class="card" style="left:${x}px;top:${y}px;width:${w}px;height:${h}px;background:${c.sup};${extra}"></div>`;
  const T = (x, y, txt, style) => `<div class="t" style="left:${x}px;top:${y}px;${style}">${txt}</div>`;
  const valBox = (x, y, w, txt, c, focus) => `<div class="vb" style="left:${x}px;top:${y}px;width:${w}px;background:${c.proc};color:${c.txt};${focus ? `box-shadow:0 0 0 2px ${c.foco}` : ""}">${txt}</div>`;

  function ui(c, opts = {}) {
    const o = Object.assign({ mode: 1, panel: null, sticker: null, focus: null, hover: null, osSel: 1 }, opts);
    let h = `<div class="ui" style="width:${W}px;height:${H}px;background:${c.fondo};color:${c.txt}">`;
    // ---- columna izquierda
    h += card(14, 14, 184, 74, c);
    h += T(14, 22, `CLIP<span style="color:${c.sel}">5</span>`, `width:184px;text-align:center;font:34px Bevan;letter-spacing:.5px`);
    h += `<div class="t" style="left:30px;top:68px;width:152px;border-top:1px solid ${c.txt2}55"></div>`;
    h += T(14, 72, "MASTERING CLIPPER", `width:184px;text-align:center;font:8.5px InterSemi;letter-spacing:2.2px;color:${c.txt2}`);
    h += card(14, 98, 184, 528, c, `background:${c.proc}`);
    h += T(26, 110, "CURVA DE RECORTE", `font:11px Bevan;letter-spacing:.3px`);
    for (let m = 0; m < 4; m++) {
      const on = m === o.mode, x = 26 + m * 41;
      const fo = o.focus === "curva" && m === 2;
      h += `<div class="sq" style="left:${x}px;top:132px;background:${on ? c.sel : c.sup};border-color:${on ? c.sel : c.txt2 + "40"};${fo ? `box-shadow:0 0 0 2px ${c.foco}` : ""}">${curveIcon(m, on ? c.selTxt : c.txt, 34)}</div>`;
    }
    h += T(14, 190, "Techo", `width:184px;text-align:center;font:15px InterSemi`);
    h += `<div class="t" style="left:50px;top:212px">${knob(112, .94, c, { focus: o.focus === "techo" })}</div>`;
    h += valBox(56, 330, 100, "−1,0 dBFS", c, o.focus === "techoEdit");
    h += T(14, 372, "Densidad", `width:184px;text-align:center;font:13px InterSemi`);
    h += `<div class="t" style="left:70px;top:392px">${knob(72, .33, c)}</div>`;
    h += valBox(70, 470, 72, "+2,0 dB", c);
    h += `<div class="t" style="left:26px;top:510px;width:160px;border-top:1px solid ${c.txt2}40"></div>`;
    h += T(28, 522, "Entrada", `width:70px;text-align:center;font:12px Inter`) + T(114, 522, "Salida", `width:70px;text-align:center;font:12px Inter`);
    h += `<div class="t" style="left:39px;top:540px">${knob(48, .625, c, { bipolar: true })}</div><div class="t" style="left:125px;top:540px">${knob(48, .5, c, { bipolar: true })}</div>`;
    h += valBox(32, 594, 62, "+3,0 dB", c) + valBox(118, 594, 62, "0,0 dB", c);
    // ---- barra superior
    h += card(208, 14, 818, 42, c);
    h += T(222, 26, "PRESET", `font:12px Bevan`);
    h += `<div class="pill" style="left:282px;top:23px;width:22px;background:${c.proc}">‹</div><div class="pill" style="left:308px;top:23px;width:22px;background:${c.proc}">›</div>`;
    h += `<div class="pill" style="left:338px;top:23px;width:290px;background:${c.proc};text-align:left;padding-left:10px">Mastering pop<span style="float:right;margin-right:10px;color:${c.txt2}">⌄</span></div>`;
    h += T(712, 28, "A/B enlazados", `font:10.5px Inter;color:${c.txt2}`);
    [["↶", 786, 24], ["↷", 814, 24], ["A", 842, 24], ["B", 870, 24], ["Unificar A/B", 898, 92], ["⚙", 996, 24]].forEach(([t, x, w]) =>
      h += `<div class="pill" style="left:${x}px;top:23px;width:${w}px;background:${c.proc};${o.hover === t ? `outline:1px solid ${c.txt2}` : ""}">${t}</div>`);
    // ---- gráfico
    h += graph(c, 208, 66, 818, 400, { panel: o.panel, seed: o.seed });
    // ---- fila inferior
    h += card(208, 476, 420, 150, c);
    h += T(222, 490, "MEZCLA PARALELA", `font:12px Bevan`);
    h += T(222, 526, "Procesada", `font:12px Inter`) + valBox(330, 520, 48, "0,0", c);
    h += `<div class="sl" style="left:222px;top:562px;width:156px;background:${c.txt2}40"><div style="width:82%;background:${c.sel}"></div><i style="left:82%;background:${c.sel}"></i></div>`;
    h += `<div class="pill" style="left:392px;top:534px;width:54px;height:26px;line-height:26px;background:${c.sel};color:${c.selTxt};font-family:InterSemi">Enlace</div>`;
    h += T(460, 526, "Original", `font:12px Inter`) + valBox(560, 520, 48, "−∞", c);
    h += `<div class="sl" style="left:460px;top:562px;width:148px;background:${c.txt2}40"><div style="width:0%;background:${c.sel}"></div><i style="left:0%;background:${c.sel}"></i></div>`;
    h += card(638, 476, 196, 150, c);
    h += T(652, 490, "SMOOTH", `font:12px Bevan`);
    h += `<div class="t" style="left:660px;top:520px">${knob(64, .35, c)}</div>` + valBox(740, 540, 70, "35 %", c);
    h += card(844, 476, 182, 150, c);
    h += T(858, 490, "OVERSAMPLING", `font:12px Bevan`);
    ["1×", "2×", "4×", "8×"].forEach((t, i) => {
      const on = i === o.osSel;
      h += `<div class="pill" style="left:${858 + i * 39}px;top:530px;width:34px;height:28px;line-height:28px;background:${on ? c.sel : c.proc};color:${on ? c.selTxt : c.txt};font-family:${on ? "InterSemi" : "Inter"}">${t}</div>`;
    });
    h += T(858, 580, `CPU <span style="font-family:Mono">0,6 %</span> · Export <span style="font-family:Mono">8×</span>`, `font:10.5px Inter;color:${c.txt2}`);
    // ---- panel superpuesto
    if (o.panel) h += overlay(c, o.panel);
    // ---- sticker
    if (o.sticker) h += `<div class="stk" style="left:${o.stickerX ?? 200}px;top:${o.stickerY ?? 46}px;width:${o.stickerW ?? 124}px;height:${o.stickerW ?? 124}px">${Stickers[o.sticker]()}</div>`;
    h += `</div>`;
    return h;
  }

  // ---------------------------------------------------------------- paneles (Medidores, Curva E/S, Avanzado)
  function overlay(c, which) {
    const x = 640, y = 108, w = 374, hh = 318;
    let h = `<div class="ov" style="left:${x}px;top:${y}px;width:${w}px;height:${hh}px;background:${c.sup};color:${c.txt};border-color:${c.txt2}55">`;
    const row = (k, a, b, bold) => `<div class="orow" style="${bold ? "font-family:InterSemi" : ""}"><span>${k}</span><span class="m">${a}</span><span class="m">${b}</span></div>`;
    if (which === "Medidores") {
      h += `<div class="oh">MEDIDORES<span>Esc cierra</span></div>
        <div class="orow hd" style="color:${c.txt2}"><span></span><span>Entrada</span><span>Salida</span></div>
        ${row("Pico máximo · dBFS", "−4,8", "−1,3")}${row("Pico real máx. · dBTP", "—", "−1,0")}
        <div class="osub">SONORIDAD · LUFS</div>
        ${row("M · Momentánea", "−17,8", "−14,5")}${row("S · Corto plazo", "−18,2", "−15,1")}${row("I · Integrada", "−18,9", "−15,8", true)}
        <div class="obtns"><span style="border-color:${c.txt2}66">Reiniciar medidores</span></div>
        <div class="orow" style="margin-top:6px"><span>Exceso máximo</span><span class="m" style="color:${c.over}">2,6 dB</span><span class="m" style="color:${c.over}">01:12</span></div>
        <div class="obtns"><span style="border-color:${c.txt2}66">Reiniciar máximo</span></div>`;
    } else if (which === "Curva E/S") {
      let d = "";
      for (let i = 0; i <= 80; i++) { const din = -36 + 42 * i / 80, u = 10 ** (din / 20); const dens = u < 1 ? u * (1 + (10 ** (2 / 20) - 1) * (1 - u * u) ** 2) : u; const out = 20 * Math.log10(Math.max(1e-6, curve(dens, 1))); const X = 30 + (din + 36) / 42 * 300, Yv = 250 - (Math.min(out, 6) + 36) / 42 * 210; d += (i ? "L" : "M") + X.toFixed(1) + " " + Yv.toFixed(1); }
      h += `<div class="oh">CURVA E/S<span>dB relativos al techo</span></div>
        <svg width="350" height="260" style="margin:4px 0 0 4px"><rect x="30" y="40" width="300" height="210" fill="${c.proc}"/>
        <line x1="30" y1="250" x2="330" y2="40" stroke="${c.txt2}" stroke-dasharray="3 3"/>
        <line x1="30" y1="${250 - 36 / 42 * 210}" x2="330" y2="${250 - 36 / 42 * 210}" stroke="${c.over}" stroke-width="1.5" stroke-dasharray="6 4"/>
        <path d="${d}" stroke="${c.sel}" stroke-width="3" fill="none"/>
        <text x="34" y="34" font-family="Inter" font-size="11" fill="${c.txt2}">Curva Medio · Densidad +2,0 dB</text></svg>`;
    } else {
      const opt = (k, v) => `<div class="orow2"><span>${k}</span><span class="seg" style="background:${c.proc}">${v}</span></div>`;
      h += `<div class="oh">AVANZADO<span>Esc cierra</span></div>
        ${opt("Oversampling al exportar", `<b style="background:${c.sel};color:${c.selTxt}">8×</b> Igual · 1× · 2× · 4×`)}
        ${opt("Fase del filtrado", `<b style="background:${c.sel};color:${c.selTxt}">Lineal</b> Mínima`)}
        ${opt("Limitador True Peak", `<b style="background:${c.sel};color:${c.selTxt}">Activado</b>`)}
        ${opt("Atenuación fija de agudos", `<b style="background:${c.sel};color:${c.selTxt}">Plano</b> Seda · Terciopelo`)}
        ${opt("Rango de Smooth", `<b style="background:${c.sel};color:${c.selTxt}">−2 dB</b> −3 dB`)}
        ${opt("Compensación de entrada", `Desactivada`)}
        ${opt("Compensación de Densidad", `Desactivada`)}`;
    }
    return h + `</div>`;
  }

  g.UI = { ui, knob, curveIcon, W, H };
})(window);
