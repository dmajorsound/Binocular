; Binocular installer for Windows (Inno Setup 6)
[Setup]
AppId={{6B1E3C2A-8D4F-4B7A-9E21-B10C0A1A1000}
AppName=Binocular
AppVersion=1.0.2
AppPublisher=dmajorsound
AppPublisherURL=https://dmajorsound.com
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DirExistsWarning=no
DisableProgramGroupPage=yes
PrivilegesRequired=admin
OutputBaseFilename=Binocular_1.0.2_installer_win64
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
Uninstallable=yes
UninstallDisplayName=Binocular
UninstallFilesDir={commonappdata}\dmajorsound\Binocular
WizardStyle=modern
WizardImageFile=wizard-100.bmp,wizard-200.bmp
WizardSmallImageFile=small-100.bmp,small-200.bmp
WizardImageStretch=no
DisableWelcomePage=no

[Files]
Source: "..\..\build\Binocular_artefacts\Release\VST3\Binocular.vst3\*"; DestDir: "{app}\Binocular.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: vst3
Source: "..\..\build\Binocular_artefacts\Release\CLAP\Binocular.clap"; DestDir: "{commoncf64}\CLAP"; Flags: ignoreversion; Components: clap

[Types]
Name: "full"; Description: "VST3 and CLAP"
Name: "custom"; Description: "Choose formats"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 (REAPER, Cubase, Nuendo, Ableton Live, ...)"; Types: full custom
Name: "clap"; Description: "CLAP (REAPER, Bitwig Studio, ...)"; Types: full custom

[Messages]
WelcomeLabel2=This installs the Binocular plug-in as VST3 and CLAP into the common plug-in folders.%n%nAfterwards, rescan your plug-ins in REAPER.
