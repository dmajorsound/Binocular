; Binocular uninstaller for Windows (Inno Setup 6): removes Binocular, VST3 and CLAP. Projects stay untouched.
[Setup]
AppId=Binocular_Uninstaller
AppName=Binocular Uninstaller
AppVersion=1.0.9
AppPublisher=dmajorsound
AppPublisherURL=https://dmajorsound.com
CreateAppDir=no
DisableProgramGroupPage=yes
DisableReadyPage=no
PrivilegesRequired=admin
OutputBaseFilename=Binocular_1.0.9_uninstaller_win64
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
Uninstallable=no
CloseApplications=no
WizardStyle=modern
WizardImageFile=wizard-100.bmp,wizard-200.bmp
WizardSmallImageFile=small-100.bmp,small-200.bmp
WizardImageStretch=no
DisableWelcomePage=no

[Files]
Source: "welcome-icon-100.bmp"; DestDir: "{tmp}"; Flags: dontcopy
Source: "welcome-icon-200.bmp"; DestDir: "{tmp}"; Flags: dontcopy
Source: "welcome-brand-100.bmp"; DestDir: "{tmp}"; Flags: dontcopy
Source: "welcome-brand-200.bmp"; DestDir: "{tmp}"; Flags: dontcopy

[Messages]
SetupWindowTitle=Uninstall Binocular
WelcomeLabel1=Uninstall Binocular
WelcomeLabel2=This removes Binocular as VST3 and CLAP plug-in from the common plug-in folders.%n%nYour projects stay untouched.%n%nQuit your DAW first.
WizardReady=Ready to uninstall
ReadyLabel1=Binocular will now be removed.
ReadyLabel2a=Click Uninstall to continue.
ReadyLabel2b=Click Uninstall to continue.
ButtonInstall=&Uninstall
WizardInstalling=Uninstalling
InstallingLabel=Please wait while Binocular is removed.
FinishedHeadingLabel=Binocular is removed
FinishedLabelNoIcons=Binocular has been removed. Rescan your plug-ins in your DAW.
FinishedLabel=Binocular has been removed. Rescan your plug-ins in your DAW.

[Code]
#include "branding.iss"

procedure InitializeWizard();
begin
  BrandWelcome('Uninstall Binocular',
    'This removes Binocular as VST3 and CLAP plug-in from the common plug-in folders. Your projects stay untouched.' + #13#10 + #13#10 +
    'Quit your DAW first.');
end;

procedure Gone(Path: String; IsDir: Boolean; var Failed: Boolean);
begin
  if IsDir then begin
    if DirExists(Path) and not DelTree(Path, True, True, True) then Failed := True;
  end else begin
    if FileExists(Path) and not DeleteFile(Path) then Failed := True;
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  Unins: String;
  Code: Integer;
  Failed: Boolean;
begin
  if CurStep <> ssInstall then exit;
  Failed := False;
  { the uninstaller the installer left behind also clears the Installed apps entry }
  Unins := ExpandConstant('{commonappdata}\dmajorsound\Binocular\unins000.exe');
  if FileExists(Unins) then
    Exec(Unins, '/VERYSILENT /SUPPRESSMSGBOXES /NORESTART', '', SW_HIDE, ewWaitUntilTerminated, Code);
  Gone(ExpandConstant('{commoncf64}\VST3\Binocular.vst3'), True, Failed);
  Gone(ExpandConstant('{commoncf64}\CLAP\Binocular.clap'), False, Failed);
  RegDeleteKeyIncludingSubkeys(HKLM, 'Software\Microsoft\Windows\CurrentVersion\Uninstall\{6B1E3C2A-8D4F-4B7A-9E21-B10C0A1A1000}_is1');
  DelTree(ExpandConstant('{commonappdata}\dmajorsound\Binocular'), True, True, True);
  if Failed then
    MsgBox('Binocular could not be removed completely because an audio application is still running.' + #13#10 +
           'Quit it and run this uninstaller again.', mbError, MB_OK);
end;
