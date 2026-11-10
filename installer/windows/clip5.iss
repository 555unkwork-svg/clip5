; clip5 — instalador para Windows (Inno Setup 6)
; Compilar:  iscc /DAppVersion=0.9.0 /DSrcDir=<carpeta con los binarios> /DOutDir=<salida> clip5.iss
; <SrcDir> debe contener: VST3\clip5.vst3\ (bundle), CLAP\clip5.clap, Standalone\clip5.exe,
; opcionalmente AAX\clip5.aaxplugin\, y LEEME.pdf / LEEME.md.

#ifndef AppVersion
  #define AppVersion "0.9.0"
#endif
#ifndef SrcDir
  #define SrcDir "..\..\build\plugin\clip5_artefacts\Release"
#endif
#ifndef OutDir
  #define OutDir "..\..\entrega"
#endif
#ifndef DocDir
  #define DocDir ".."
#endif

[Setup]
AppId={{6F3C5A1E-5C1B-4E8A-9B7D-C11905000001}
AppName=clip5
AppVersion={#AppVersion}
AppVerName=clip5 {#AppVersion}
AppPublisher=clip5
AppPublisherURL=https://clip5.audio
DefaultDirName={autopf}\clip5
DefaultGroupName=clip5
DisableProgramGroupPage=yes
OutputDir={#OutDir}
OutputBaseFilename=clip5-{#AppVersion}-Windows-Setup
Compression=lzma2/max
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern
UninstallDisplayName=clip5 {#AppVersion}
UninstallDisplayIcon={app}\clip5.exe
InfoBeforeFile={#DocDir}\windows\antes.txt
ShowLanguageDialog=no

[Languages]
Name: "es"; MessagesFile: "compiler:Languages\Spanish.isl"

[Types]
Name: "completa"; Description: "Instalación completa (recomendada)"
Name: "personalizada"; Description: "Instalación personalizada"; Flags: iscustom

[Components]
Name: "vst3"; Description: "Plugin VST3 (Ableton, Cubase, Reaper, FL Studio, Studio One…)"; Types: completa personalizada
Name: "clap"; Description: "Plugin CLAP (Reaper, Bitwig, FL Studio…)"; Types: completa personalizada
Name: "app"; Description: "Aplicación independiente (sin DAW)"; Types: completa personalizada
#if DirExists(SrcDir + "\AAX\clip5.aaxplugin")
Name: "aax"; Description: "Plugin AAX (Pro Tools)"; Types: completa personalizada
#endif

[Files]
Source: "{#SrcDir}\VST3\clip5.vst3\*"; DestDir: "{commoncf64}\VST3\clip5.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SrcDir}\CLAP\clip5.clap"; DestDir: "{commoncf64}\CLAP"; Components: clap; Flags: ignoreversion
Source: "{#SrcDir}\Standalone\clip5.exe"; DestDir: "{app}"; Components: app; Flags: ignoreversion
#if DirExists(SrcDir + "\AAX\clip5.aaxplugin")
Source: "{#SrcDir}\AAX\clip5.aaxplugin\*"; DestDir: "{commoncf64}\Avid\Audio\Plug-Ins\clip5.aaxplugin"; Components: aax; Flags: ignoreversion recursesubdirs createallsubdirs
#endif
#if FileExists(DocDir + "\LEEME.pdf")
Source: "{#DocDir}\LEEME.pdf"; DestDir: "{app}"; Flags: ignoreversion isreadme
#endif
Source: "{#DocDir}\LEEME.md"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\clip5"; Filename: "{app}\clip5.exe"; Components: app
#if FileExists(DocDir + "\LEEME.pdf")
Name: "{group}\Manual de clip5"; Filename: "{app}\LEEME.pdf"
#endif
Name: "{group}\Desinstalar clip5"; Filename: "{uninstallexe}"

[Run]
#if FileExists(DocDir + "\LEEME.pdf")
Filename: "{app}\LEEME.pdf"; Description: "Abrir el manual"; Flags: postinstall shellexec skipifsilent unchecked
#endif
Filename: "{app}\clip5.exe"; Description: "Abrir clip5 (aplicación independiente)"; Components: app; Flags: postinstall nowait skipifsilent unchecked

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\clip5.vst3"
