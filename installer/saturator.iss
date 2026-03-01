[Setup]
AppName=Saturator
AppVersion={#AppVersion}
AppPublisher=Viste
DefaultDirName={commoncf64}\VST3\Saturator.vst3
DirExistsWarning=no
DisableProgramGroupPage=yes
OutputBaseFilename=Saturator-{#AppVersion}-Windows-x64-Setup
Compression=lzma2
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
UninstallDisplayIcon={app}\Contents\x86_64-win\Saturator.vst3
LicenseFile=
WizardStyle=modern
DisableWelcomePage=no

[Files]
Source: "{#VST3Source}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\Uninstall Saturator VST3"; Filename: "{uninstallexe}"
