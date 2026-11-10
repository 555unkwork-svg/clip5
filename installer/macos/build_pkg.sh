#!/usr/bin/env bash
# clip5 — genera el instalador .pkg para macOS (Universal: Apple Silicon + Intel).
#
# Uso:  installer/macos/build_pkg.sh <carpeta de binarios> <versión> <carpeta de salida>
#   <carpeta de binarios> = build/plugin/clip5_artefacts/Release (contiene AU/, VST3/, CLAP/, Standalone/)
#
# Firma y notarización (opcionales; sin ellas el .pkg funciona, pero macOS pide "Abrir" con clic derecho):
#   CLIP5_SIGN_APP="Developer ID Application: …"     firma los plugins y la app
#   CLIP5_SIGN_INSTALLER="Developer ID Installer: …" firma el .pkg
#   CLIP5_NOTARY_PROFILE=<perfil de notarytool>      notariza y "grapa" el ticket
set -euo pipefail

SRC="${1:?carpeta de binarios}"
VERSION="${2:?versión}"
OUT="${3:?carpeta de salida}"
HERE="$(cd "$(dirname "$0")" && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
mkdir -p "$OUT"
ID="audio.clip5.clip5"

stage () { # componente  origen  destino-en-el-sistema
    local name="$1" src="$2" dest="$3"
    if [ ! -e "$src" ]; then echo "falta $src (se omite $name)"; return 1; fi
    mkdir -p "$WORK/root-$name$dest"
    cp -R "$src" "$WORK/root-$name$dest/"
    if [ -n "${CLIP5_SIGN_APP:-}" ]; then
        codesign --force --deep --timestamp --options runtime --sign "$CLIP5_SIGN_APP" "$WORK/root-$name$dest/$(basename "$src")"
    fi
    pkgbuild --root "$WORK/root-$name" --identifier "$ID.$name" --version "$VERSION" \
             --install-location / "$WORK/$name.pkg" >/dev/null
    echo "componente: $name"
}

CHOICES=""
add () { stage "$@" && CHOICES="$CHOICES $1"; }
add au   "$SRC/AU/clip5.component"   "/Library/Audio/Plug-Ins/Components" || true
add vst3 "$SRC/VST3/clip5.vst3"      "/Library/Audio/Plug-Ins/VST3"       || true
add clap "$SRC/CLAP/clip5.clap"      "/Library/Audio/Plug-Ins/CLAP"       || true
add app  "$SRC/Standalone/clip5.app" "/Applications"                      || true
[ -d "$SRC/AAX/clip5.aaxplugin" ] && { add aax "$SRC/AAX/clip5.aaxplugin" "/Library/Application Support/Avid/Audio/Plug-Ins" || true; }

describe () {
    case "$1" in
        au)   echo "Audio Unit (Logic Pro, GarageBand, Ableton Live…)";;
        vst3) echo "VST3 (Ableton Live, Cubase, Reaper, FL Studio, Studio One…)";;
        clap) echo "CLAP (Reaper, Bitwig, FL Studio…)";;
        app)  echo "Aplicación independiente";;
        aax)  echo "AAX (Pro Tools)";;
    esac
}

{
    echo '<?xml version="1.0" encoding="utf-8"?>'
    echo '<installer-gui-script minSpecVersion="2">'
    echo "  <title>clip5 $VERSION</title>"
    echo '  <welcome file="bienvenida.html" mime-type="text/html"/>'
    echo '  <readme file="LEEME.html" mime-type="text/html"/>'
    echo '  <conclusion file="fin.html" mime-type="text/html"/>'
    echo '  <options customize="allow" require-scripts="false" hostArchitectures="arm64,x86_64"/>'
    echo '  <domains enable_localSystem="true"/>'
    echo '  <os-version min="10.13"/>'
    echo '  <choices-outline>'
    for c in $CHOICES; do echo "    <line choice=\"$c\"/>"; done
    echo '  </choices-outline>'
    for c in $CHOICES; do
        echo "  <choice id=\"$c\" title=\"$(describe "$c")\" visible=\"true\" start_selected=\"true\">"
        echo "    <pkg-ref id=\"$ID.$c\"/>"
        echo "  </choice>"
        echo "  <pkg-ref id=\"$ID.$c\" version=\"$VERSION\">$c.pkg</pkg-ref>"
    done
    echo '</installer-gui-script>'
} > "$WORK/distribution.xml"

cp "$HERE/resources/"*.html "$WORK/"
[ -f "$HERE/../LEEME.html" ] && cp "$HERE/../LEEME.html" "$WORK/LEEME.html"

PKG="$OUT/clip5-$VERSION-macOS.pkg"
SIGN_ARGS=()
[ -n "${CLIP5_SIGN_INSTALLER:-}" ] && SIGN_ARGS=(--sign "$CLIP5_SIGN_INSTALLER")
productbuild --distribution "$WORK/distribution.xml" --resources "$WORK" --package-path "$WORK" \
             ${SIGN_ARGS[@]+"${SIGN_ARGS[@]}"} "$PKG"

if [ -n "${CLIP5_NOTARY_PROFILE:-}" ] && [ -n "${CLIP5_SIGN_INSTALLER:-}" ]; then
    xcrun notarytool submit "$PKG" --keychain-profile "$CLIP5_NOTARY_PROFILE" --wait
    xcrun stapler staple "$PKG"
fi
echo "instalador: $PKG"
