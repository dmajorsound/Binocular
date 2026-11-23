; Binocular installer for Windows (Inno Setup 6)
[Setup]
AppId={{6B1E3C2A-8D4F-4B7A-9E21-B10C0A1A1000}
AppName=Binocular
AppVersion=1.0.1
AppPublisher=dmajorsound
AppPublisherURL=https://dmajorsound.com
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DirExistsWarning=no
DisableProgramGroupPage=yes
PrivilegesRequired=admin
OutputBaseFilename=Binocular_1.0.1_installer_win64
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
Uninstallable=yes
UninstallDisplayName=Binocular (VST3)
UninstallFilesDir={commonappdata}\dmajorsound\Binocular
WizardStyle=modern
WizardImageFile=wizard-100.bmp,wizard-200.bmp
WizardSmallImageFile=small-100.bmp,small-200.bmp
WizardImageStretch=no
DisableWelcomePage=no

[Files]
Source: "..\..\build\Binocular_artefacts\Release\VST3\Binocular.vst3\*"; DestDir: "{app}\Binocular.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[Messages]
WelcomeLabel2=This installs the Binocular VST3 plug-in into the common VST3 folder.%n%nAfterwards, rescan your plug-ins in REAPER.
