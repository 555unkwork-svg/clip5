// clip5 — stickers originales (vectoriales, diseño propio). Cada función devuelve un <svg>.
// Efecto "troquelado": borde blanco y sombra generados con un filtro sobre el alfa del dibujo.
(function (global) {
  let uid = 0;
  const DIE = (id, r = 7) => `
    <filter id="${id}" x="-25%" y="-25%" width="150%" height="150%" color-interpolation-filters="sRGB">
      <feMorphology in="SourceAlpha" operator="dilate" radius="${r}" result="d"/>
      <feFlood flood-color="#ffffff" result="wc"/><feComposite in="wc" in2="d" operator="in" result="w"/>
      <feGaussianBlur in="d" stdDeviation="4" result="b"/><feOffset in="b" dx="2" dy="5" result="o"/>
      <feFlood flood-color="#000" flood-opacity=".32" result="sc"/><feComposite in="sc" in2="o" operator="in" result="s"/>
      <feMerge><feMergeNode in="s"/><feMergeNode in="w"/><feMergeNode in="SourceGraphic"/></feMerge>
    </filter>`;

  // firma "5unk" (tag de marcador) — aparece pequeña en cada sticker
  const firma = (x, y, size = 15, color = "#111", rot = -8) =>
    `<text x="${x}" y="${y}" transform="rotate(${rot} ${x} ${y})" font-family="Graffiti" font-size="${size}" fill="${color}">5unk</text>`;

  const star4 = (cx, cy, r, fill) =>
    `<path d="M${cx} ${cy - r} Q${cx + r * .18} ${cy - r * .18} ${cx + r} ${cy} Q${cx + r * .18} ${cy + r * .18} ${cx} ${cy + r} Q${cx - r * .18} ${cy + r * .18} ${cx - r} ${cy} Q${cx - r * .18} ${cy - r * .18} ${cx} ${cy - r}Z" fill="${fill}"/>`;

  const wrap = (vb, body, rot = 0) => {
    const id = "die" + (++uid);
    const [, , w, h] = vb.split(" ").map(Number);
    return `<svg xmlns="http://www.w3.org/2000/svg" viewBox="${vb}" class="sticker"><defs>${DIE(id)}</defs>
      <g filter="url(#${id})" transform="rotate(${rot} ${w / 2} ${h / 2})">${body}</g></svg>`;
  };

  // ------------------------------------------------------------------ 0 · tag 5unk (marca)
  function tag5unk() {
    return wrap("0 0 260 140", `
      <path d="M30 28 H212 l26 42 -26 42 H30 a14 14 0 0 1 -14 -14 V42 a14 14 0 0 1 14 -14Z" fill="#111"/>
      <circle cx="210" cy="70" r="8" fill="#fff"/>
      <text x="36" y="96" font-family="Graffiti" font-size="70" fill="#F4F1E8">5unk</text>
      <path d="M58 96 q2 18 0 26" stroke="#F4F1E8" stroke-width="5" stroke-linecap="round" fill="none"/>
      <path d="M150 98 q2 12 0 16" stroke="#F4F1E8" stroke-width="4" stroke-linecap="round" fill="none"/>`, -6);
  }

  // ------------------------------------------------------------------ 1 · ice: cubo de hielo
  function ice() {
    return wrap("0 0 240 240", `
      <defs><linearGradient id="iceT" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#F4FCFF"/><stop offset="1" stop-color="#CBEBFB"/></linearGradient>
      <linearGradient id="iceL" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#9FD6F4"/><stop offset="1" stop-color="#6FB7E6"/></linearGradient>
      <linearGradient id="iceR" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#7CC0EC"/><stop offset="1" stop-color="#3F8FCB"/></linearGradient></defs>
      <path d="M120 40 L198 82 L120 124 L42 82Z" fill="url(#iceT)" stroke="#2E6E9E" stroke-width="4" stroke-linejoin="round"/>
      <path d="M42 82 L120 124 L120 210 L42 168Z" fill="url(#iceL)" stroke="#2E6E9E" stroke-width="4" stroke-linejoin="round"/>
      <path d="M120 124 L198 82 L198 168 L120 210Z" fill="url(#iceR)" stroke="#2E6E9E" stroke-width="4" stroke-linejoin="round"/>
      <path d="M62 92 L62 150" stroke="#fff" stroke-opacity=".75" stroke-width="7" stroke-linecap="round"/>
      <path d="M76 112 L76 128" stroke="#fff" stroke-opacity=".6" stroke-width="5" stroke-linecap="round"/>
      <path d="M92 70 L132 52" stroke="#fff" stroke-width="6" stroke-linecap="round"/>
      <path d="M148 140 l18 -10 m-26 34 l10 -6" stroke="#E6F7FF" stroke-opacity=".7" stroke-width="3" stroke-linecap="round"/>
      <path d="M150 104 l14 18 m-6 -6 l10 -2" stroke="#1F5F8F" stroke-opacity=".45" stroke-width="2.5" stroke-linecap="round" fill="none"/>
      ${star4(204, 46, 16, "#fff")}${star4(28, 64, 10, "#fff")}${star4(214, 190, 9, "#E9F8FF")}
      ${firma(140, 196, 15, "#0E3A5C", -28)}`, -4);
  }

  // ------------------------------------------------------------------ 2 · raw: librillo de papel crudo
  function raw() {
    return wrap("0 0 260 200", `
      <path d="M58 30 q30 -10 60 0 t60 0 q20 -6 26 -2 l-2 30 H58Z" fill="#F2E6CF" stroke="#8A6A43" stroke-width="3"/>
      <rect x="34" y="52" width="192" height="122" rx="10" fill="#C9A26B" stroke="#6E4E2A" stroke-width="4"/>
      <path d="M34 62 a10 10 0 0 1 10 -10 H216 a10 10 0 0 1 10 10 v22 L130 106 L34 84Z" fill="#B88D55" stroke="#6E4E2A" stroke-width="4" stroke-linejoin="round"/>
      <g fill="#6E4E2A" opacity=".35"><circle cx="60" cy="140" r="2"/><circle cx="180" cy="150" r="1.6"/><circle cx="96" cy="160" r="1.4"/><circle cx="200" cy="124" r="1.8"/><circle cx="150" cy="132" r="1.2"/></g>
      <text x="130" y="158" text-anchor="middle" font-family="Bevan" font-size="54" fill="#C2262E" letter-spacing="2">raw</text>
      <text x="130" y="94" text-anchor="middle" font-family="InterSemi" font-size="10" fill="#F6EEDC" letter-spacing="3">PAPEL CRUDO · 5UNK</text>
      ${firma(176, 186, 14, "#3B2A16", -6)}`, 5);
  }

  // ------------------------------------------------------------------ 3 · violeta: cogollo violeta
  function violeta() {
    // brácteas en gota apuntando hacia arriba, en filas que forman un cono
    const bract = (x, y, w, h, rot, fill) =>
      `<path transform="translate(${x} ${y}) rotate(${rot})" d="M0 0 C${-w} -${h * .15} ${-w * .9} -${h * .7} 0 -${h} C${w * .9} -${h * .7} ${w} -${h * .15} 0 0Z" fill="${fill}" stroke="#3D1F66" stroke-width="2.6" stroke-linejoin="round"/>
       <path transform="translate(${x} ${y}) rotate(${rot})" d="M0 -${h * .2} Q${w * .25} -${h * .55} 0 -${h * .85}" stroke="#C7A8EE" stroke-width="2" fill="none" stroke-linecap="round"/>`;
    const cols = ["#5E3398", "#7445B0", "#8A58C4", "#6A3BA6", "#9466CF", "#7C4BB8"];
    let s = `<path d="M120 200 q-3 16 3 28" stroke="#5D7A3A" stroke-width="7" stroke-linecap="round" fill="none"/>`;
    // hojas de azúcar
    s += `<path d="M118 176 C80 172 58 150 50 122 C78 136 100 146 116 160Z" fill="#6E9A47" stroke="#3E5C26" stroke-width="3" stroke-linejoin="round"/>`;
    s += `<path d="M122 176 C160 172 182 150 190 122 C162 136 140 146 124 160Z" fill="#7FAA52" stroke="#3E5C26" stroke-width="3" stroke-linejoin="round"/>`;
    s += `<path d="M104 120 C84 104 80 84 84 66 C96 82 104 94 108 108Z" fill="#7FAA52" stroke="#3E5C26" stroke-width="2.6" stroke-linejoin="round"/>`;
    const rows = [[200, 5, 30, 30], [184, 6, 34, 32], [164, 6, 36, 32], [144, 6, 36, 31], [124, 5, 32, 30], [104, 5, 28, 28], [86, 4, 22, 26], [68, 3, 16, 24], [52, 1, 10, 22]];
    let k = 0;
    rows.forEach(([y, n, spread, h]) => {
      for (let i = 0; i < n; i++) {
        const t = n === 1 ? 0 : i / (n - 1) - .5;
        const x = 120 + t * spread * 2.0 + ((k % 2) ? 3 : -3), rot = t * 40;
        s += bract(x, y, h * .72, h * 1.05, rot, cols[(k++) % cols.length]);
      }
    });
    const hairs = [[104, 150], [136, 146], [120, 128], [96, 112], [146, 116], [112, 92], [132, 88], [120, 66], [104, 176], [140, 174], [122, 52]];
    hairs.forEach(([x, y], i) => s += `<path d="M${x} ${y} q${i % 2 ? 9 : -9} -4 ${i % 2 ? 5 : -4} -13" stroke="#F39A3A" stroke-width="2.8" fill="none" stroke-linecap="round"/>`);
    for (let i = 0; i < 40; i++) { const a = i * 2.39, r = 6 + (i * 29 % 54), x = 120 + Math.cos(a) * r * .7, y = 124 + Math.sin(a) * r * 1.1; s += `<circle cx="${x.toFixed(1)}" cy="${y.toFixed(1)}" r="1.6" fill="#F5EEFF"/>`; }
    s += firma(146, 222, 16, "#2A1446", -10);
    return wrap("0 0 240 240", s, 6);
  }

  // ------------------------------------------------------------------ 4 · pro: sello "professional certified"
  function pro() {
    const n = 28, R1 = 104, R2 = 92;
    let pts = [];
    for (let i = 0; i < n * 2; i++) { const r = i % 2 ? R2 : R1, a = Math.PI * i / n; pts.push(`${120 + r * Math.cos(a)},${112 + r * Math.sin(a)}`); }
    return wrap("0 0 240 260", `
      <path d="M78 170 L58 246 L84 232 L98 256 L112 188Z" fill="#B23A3A" stroke="#6E1F1F" stroke-width="3" stroke-linejoin="round"/>
      <path d="M162 170 L182 246 L156 232 L142 256 L128 188Z" fill="#2F5C8F" stroke="#1B3556" stroke-width="3" stroke-linejoin="round"/>
      <polygon points="${pts.join(" ")}" fill="#E7B94A" stroke="#8A6114" stroke-width="3" stroke-linejoin="round"/>
      <circle cx="120" cy="112" r="80" fill="#14293F" stroke="#F3D27A" stroke-width="4"/>
      <circle cx="120" cy="112" r="56" fill="none" stroke="#F3D27A" stroke-width="2" stroke-dasharray="3 4"/>
      <path id="arcT" d="M54 112 A66 66 0 0 1 186 112" fill="none"/>
      <path id="arcB" d="M50 112 A70 70 0 0 0 190 112" fill="none"/>
      <text font-family="InterSemi" font-size="15" letter-spacing="3" fill="#F7E7B5"><textPath href="#arcT" startOffset="50%" text-anchor="middle">PROFESSIONAL</textPath></text>
      <text font-family="InterSemi" font-size="15" letter-spacing="3" fill="#F7E7B5"><textPath href="#arcB" startOffset="50%" text-anchor="middle">★ CERTIFIED ★</textPath></text>
      <text x="120" y="122" text-anchor="middle" font-family="Bevan" font-size="40" fill="#fff">PRO</text>
      <path d="M100 136 l12 12 l26 -24" stroke="#7BE0A5" stroke-width="7" fill="none" stroke-linecap="round" stroke-linejoin="round"/>
      ${firma(146, 236, 14, "#FFFFFF", -14)}`, -5);
  }

  // ------------------------------------------------------------------ 5 · cafe: latte art (vista superior)
  function cafe() {
    const heart = (cx, cy, s) => `<path d="M${cx} ${cy + 12 * s} C${cx - 22 * s} ${cy - 2 * s} ${cx - 16 * s} ${cy - 20 * s} ${cx} ${cy - 8 * s} C${cx + 16 * s} ${cy - 20 * s} ${cx + 22 * s} ${cy - 2 * s} ${cx} ${cy + 12 * s}Z" fill="#FFF6E6"/>`;
    return wrap("0 0 260 240", `
      <defs><radialGradient id="crema" cx=".5" cy=".45" r=".6"><stop offset="0" stop-color="#C8834A"/><stop offset=".7" stop-color="#9A5A2D"/><stop offset="1" stop-color="#6B3A1C"/></radialGradient></defs>
      <ellipse cx="124" cy="124" rx="108" ry="104" fill="#EEE2D1" stroke="#7A5B3E" stroke-width="4"/>
      <ellipse cx="124" cy="124" rx="86" ry="83" fill="none" stroke="#D8C7AE" stroke-width="3"/>
      <rect x="196" y="104" width="52" height="34" rx="17" fill="#FFFDF8" stroke="#7A5B3E" stroke-width="4"/>
      <circle cx="122" cy="120" r="74" fill="#FFFDF8" stroke="#7A5B3E" stroke-width="4"/>
      <circle cx="122" cy="120" r="62" fill="url(#crema)"/>
      ${heart(122, 154, 1.25)}${heart(122, 124, 1.05)}${heart(122, 98, .85)}${heart(122, 78, .6)}
      <path d="M122 70 V176" stroke="#9A5A2D" stroke-width="3" stroke-linecap="round"/>
      <circle cx="122" cy="68" r="4" fill="#FFF6E6"/>
      <path d="M160 72 q10 8 8 22" stroke="#fff" stroke-opacity=".55" stroke-width="5" fill="none" stroke-linecap="round"/>
      ${firma(36, 226, 15, "#4A2E18", -4)}`, 3);
  }

  // ------------------------------------------------------------------ 6 · marfil: rifle de asalto (silueta)
  function marfil() {
    return wrap("0 0 340 180", `
      <path d="M18 92 L94 80 L104 98 L100 116 L30 132 Q18 134 16 122Z" fill="#8B5A2B" stroke="#3B2412" stroke-width="3.5" stroke-linejoin="round"/>
      <path d="M28 100 L84 92" stroke="#B07A45" stroke-width="4" stroke-linecap="round"/>
      <rect x="94" y="72" width="104" height="30" rx="4" fill="#2B2B2B" stroke="#111" stroke-width="3.5"/>
      <path d="M112 102 L104 140 Q103 146 109 146 L120 146 L126 102Z" fill="#7A4A22" stroke="#3B2412" stroke-width="3.5" stroke-linejoin="round"/>
      <path d="M148 102 Q150 128 138 154 Q134 162 142 164 L160 166 Q166 166 168 160 Q180 132 172 102Z" fill="#2B2B2B" stroke="#111" stroke-width="3.5" stroke-linejoin="round"/>
      <path d="M150 112 Q152 128 146 146" stroke="#555" stroke-width="3" fill="none"/>
      <path d="M128 102 Q134 118 146 112" stroke="#111" stroke-width="3" fill="none"/>
      <rect x="196" y="76" width="64" height="22" rx="6" fill="#8B5A2B" stroke="#3B2412" stroke-width="3.5"/>
      <rect x="200" y="66" width="70" height="10" rx="3" fill="#2B2B2B" stroke="#111" stroke-width="3"/>
      <rect x="258" y="80" width="64" height="9" rx="2" fill="#2B2B2B" stroke="#111" stroke-width="3"/>
      <rect x="276" y="62" width="7" height="20" fill="#2B2B2B" stroke="#111" stroke-width="2.5"/>
      <rect x="318" y="77" width="12" height="14" rx="2" fill="#2B2B2B" stroke="#111" stroke-width="2.5"/>
      <rect x="164" y="64" width="12" height="9" fill="#2B2B2B" stroke="#111" stroke-width="2.5"/>
      <path d="M100 78 H192" stroke="#4A4A4A" stroke-width="3"/>
      <path d="M210 82 h40 M210 92 h40" stroke="#B07A45" stroke-width="2.5"/>
      ${firma(214, 150, 22, "#1A1A19", -6)}`, -7);
  }

  // ------------------------------------------------------------------ 7 · sonic: anillo dorado a toda velocidad
  function sonic() {
    return wrap("0 0 280 220", `
      <defs><linearGradient id="gold" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#FFF1A6"/><stop offset=".45" stop-color="#FFC92E"/><stop offset="1" stop-color="#D98A0B"/></linearGradient></defs>
      <path d="M18 70 H118 M8 104 H112 M26 138 H116" stroke="#1E4FD8" stroke-width="12" stroke-linecap="round"/>
      <path d="M40 86 H120 M44 122 H118" stroke="#55A8FF" stroke-width="7" stroke-linecap="round"/>
      <path fill-rule="evenodd" d="M170 22 a72 82 0 1 0 0.01 0Z M170 46 a50 58 0 1 1 -0.01 0Z" fill="url(#gold)" stroke="#9A5E05" stroke-width="5"/>
      <ellipse cx="170" cy="104" rx="61" ry="70" fill="none" stroke="#FFE27A" stroke-width="3" opacity=".7"/>
      <path d="M126 64 Q140 34 172 30" stroke="#FFFBE0" stroke-width="8" fill="none" stroke-linecap="round"/>
      <path d="M210 150 Q200 170 180 176" stroke="#B66F06" stroke-width="5" fill="none" stroke-linecap="round" opacity=".6"/>
      ${star4(250, 36, 18, "#FFF7C2")}${star4(228, 180, 11, "#FFFFFF")}${star4(110, 30, 9, "#FFF7C2")}
      <text x="150" y="214" text-anchor="middle" font-family="Bevan" font-size="26" fill="#1E4FD8" transform="skewX(-14)">GO!</text>
      ${firma(222, 214, 15, "#0D2A7A", -8)}`, 4);
  }

  // ------------------------------------------------------------------ 8 · rose gold: gatita kawaii original
  function roseGold() {
    return wrap("0 0 260 240", `
      <defs><linearGradient id="rg" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#F7D3C8"/><stop offset=".5" stop-color="#D99A88"/><stop offset="1" stop-color="#B9705F"/></linearGradient></defs>
      <path d="M58 92 L48 26 L104 58Z" fill="#fff" stroke="#3A2A33" stroke-width="5" stroke-linejoin="round"/>
      <path d="M202 92 L212 26 L156 58Z" fill="#fff" stroke="#3A2A33" stroke-width="5" stroke-linejoin="round"/>
      <path d="M62 78 L58 42 L88 60Z" fill="url(#rg)"/><path d="M198 78 L202 42 L172 60Z" fill="url(#rg)"/>
      <ellipse cx="130" cy="128" rx="98" ry="80" fill="#fff" stroke="#3A2A33" stroke-width="5"/>
      <path d="M86 124 q14 -16 28 0" stroke="#3A2A33" stroke-width="6" fill="none" stroke-linecap="round"/>
      <path d="M146 124 q14 -16 28 0" stroke="#3A2A33" stroke-width="6" fill="none" stroke-linecap="round"/>
      <ellipse cx="82" cy="150" rx="16" ry="9" fill="#F4A9B2" opacity=".75"/><ellipse cx="178" cy="150" rx="16" ry="9" fill="#F4A9B2" opacity=".75"/>
      <path d="M124 140 h12 l-6 7Z" fill="#E7858F" stroke="#3A2A33" stroke-width="2.5" stroke-linejoin="round"/>
      <path d="M118 152 q6 8 12 0 q6 8 12 0" stroke="#3A2A33" stroke-width="3.5" fill="none" stroke-linecap="round"/>
      <path d="M30 128 l34 6 M28 150 l36 -4 M230 128 l-34 6 M232 150 l-36 -4" stroke="#3A2A33" stroke-width="3.5" stroke-linecap="round"/>
      <path d="M150 52 C150 36 172 34 174 50 C176 34 198 38 196 54 C194 70 174 76 174 86 C172 76 152 70 150 52Z" fill="url(#rg)" stroke="#8C4E40" stroke-width="3.5"/>
      <path d="M160 48 q4 -6 10 -4" stroke="#FFF2EC" stroke-width="3" fill="none" stroke-linecap="round"/>
      ${firma(158, 228, 15, "#7A3F4E", -6)}`, -4);
  }

  // ------------------------------------------------------------------ 9 · 420: insignia con hoja
  function c420() {
    const leaflet = (ang, len, wid) => {
      const a = ang * Math.PI / 180, x = 120 + Math.sin(a) * len, y = 150 - Math.cos(a) * len;
      const nx = Math.cos(a) * wid, ny = Math.sin(a) * wid;
      const mx = 120 + Math.sin(a) * len * .5, my = 150 - Math.cos(a) * len * .5;
      return `<path d="M120 150 Q${mx + nx} ${my + ny} ${x} ${y} Q${mx - nx} ${my - ny} 120 150Z" fill="#5FB862" stroke="#1F5A2A" stroke-width="3" stroke-linejoin="round"/>`;
    };
    let leaf = "";
    [[-92, 62, 12], [92, 62, 12], [-62, 92, 16], [62, 92, 16], [-30, 112, 19], [30, 112, 19], [0, 128, 21]].forEach(l => leaf += leaflet(...l));
    return wrap("0 0 240 240", `
      <circle cx="120" cy="120" r="108" fill="#13251F" stroke="#2E7D4F" stroke-width="10"/>
      <circle cx="120" cy="120" r="94" fill="none" stroke="#E5DCAA" stroke-width="2" stroke-dasharray="2 6"/>
      <g opacity=".95">${leaf}</g>
      <path d="M120 150 V196" stroke="#1F5A2A" stroke-width="5" stroke-linecap="round"/>
      <text x="124" y="148" text-anchor="middle" font-family="Bevan" font-size="64" fill="#0A1713" opacity=".55">420</text>
      <text x="120" y="144" text-anchor="middle" font-family="Bevan" font-size="64" fill="#F4EFC9">420</text>
      ${firma(132, 204, 15, "#E5DCAA", -10)}`, 8);
  }

  // ------------------------------------------------------------------ 10 · serum: gotero con forma de onda
  function serum() {
    let wave = "M66 168";
    for (let i = 0; i <= 60; i++) { const x = 66 + i * 1.8, y = 168 - Math.sin(i / 60 * Math.PI * 4) * 12 * Math.exp(-i / 70) - (i % 15 === 0 ? 6 : 0); wave += ` L${x.toFixed(1)} ${y.toFixed(1)}`; }
    return wrap("0 0 240 260", `
      <defs><linearGradient id="liq" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#5BB8FF"/><stop offset="1" stop-color="#1F5FD0"/></linearGradient>
      <linearGradient id="glass" x1="0" y1="0" x2="1" y2="0"><stop offset="0" stop-color="#E9F3FF" stop-opacity=".9"/><stop offset=".5" stop-color="#C7DDF5" stop-opacity=".55"/><stop offset="1" stop-color="#9CB9DA" stop-opacity=".8"/></linearGradient></defs>
      <path d="M96 18 Q96 4 120 4 Q144 4 144 18 L146 56 H94Z" fill="#1E2228" stroke="#05070A" stroke-width="4" stroke-linejoin="round"/>
      <rect x="86" y="54" width="68" height="20" rx="5" fill="#3A404A" stroke="#05070A" stroke-width="4"/>
      <path d="M78 76 H162 Q176 76 176 92 V112 Q190 124 190 146 V226 Q190 248 166 248 H74 Q50 248 50 226 V146 Q50 124 64 112 V92 Q64 76 78 76Z" fill="url(#glass)" stroke="#2A3B55" stroke-width="4" stroke-linejoin="round"/>
      <path d="M54 150 Q120 138 186 150 V226 Q186 244 166 244 H74 Q54 244 54 226Z" fill="url(#liq)"/>
      <path d="${wave}" stroke="#E8F6FF" stroke-width="3.5" fill="none" stroke-linecap="round" stroke-linejoin="round"/>
      <path d="M66 200 H176" stroke="#FFB13B" stroke-width="2" stroke-dasharray="4 4" opacity=".85"/>
      <path d="M70 100 V220" stroke="#fff" stroke-opacity=".7" stroke-width="6" stroke-linecap="round"/>
      <circle cx="150" cy="186" r="4" fill="#E8F6FF" opacity=".8"/><circle cx="160" cy="210" r="2.6" fill="#E8F6FF" opacity=".7"/>
      ${firma(96, 134, 18, "#0E1013", -6)}`, 6);
  }

  global.Stickers = { tag5unk, ice, raw, violeta, pro, cafe, marfil, sonic, roseGold, c420, serum };
})(window);
