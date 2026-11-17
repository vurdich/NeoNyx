; NeoNyx Windows Installer — Inno Setup 6
#define AppName "NeoNyx"
#define AppVersion GetEnv("NEONYX_VERSION")
#define AppPublisher "Lunexxy"
#define AppExeName "NeoNyx.exe"

[Setup]
AppId={{B37C68D8-9D39-4A2D-A3F2-3E0A3A300001}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={autopf}\NeoNyx
DefaultGroupName=NeoNyx
OutputDir=..\dist
OutputBaseFilename=NeoNyx_Setup
SetupIconFile=..\neonyx.ico
UninstallDisplayIcon={app}\{#AppExeName}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
DisableProgramGroupPage=yes
CloseApplications=yes
RestartApplications=no

[Tasks]
Name: "desktopicon"; Description: "Создать ярлык на рабочем столе"; GroupDescription: "Дополнительные ярлыки:"

[Files]
Source: "..\build\NeoNyx.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\NeoNyx"; Filename: "{app}\{#AppExeName}"
Name: "{autodesktop}\NeoNyx"; Filename: "{app}\{#AppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExeName}"; Description: "Запустить NeoNyx"; Flags: postinstall nowait skipifsilent
