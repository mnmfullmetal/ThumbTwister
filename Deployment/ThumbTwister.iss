[Setup]
; --- Application Information ---
AppName=ThumbTwister
AppVersion=1.0
AppPublisher= mnmFullmetal
DefaultDirName={autopf}\ThumbTwister
DefaultGroupName=ThumbTwister

; --- System & Output Settings ---
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
OutputDir=Output
OutputBaseFilename=ThumbTwister_Installer
Compression=lzma2
SolidCompression=yes
SetupIconFile=compiler:SetupClassicIcon.ico

[Files]
; --- Core Application Files ---
Source: "bin\ThumbTwister.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "dependencies\HidHideCLI.exe"; DestDir: "{app}"; Flags: ignoreversion

; --- Driver Dependencies (Extracted to a temporary folder during install) ---
Source: "dependencies\ViGEmBus_1.22.0_x64_x86_arm64.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall
Source: "dependencies\HidHide_1.5.230_x64.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall

[Icons]
; --- Shortcuts ---
Name: "{group}\ThumbTwister"; Filename: "{app}\ThumbTwister.exe"
Name: "{autodesktop}\ThumbTwister"; Filename: "{app}\ThumbTwister.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Run]
; --- Install Dependencies Silently ---
Filename: "{tmp}\ViGEmBus_1.22.0_x64_x86_arm64.exe"; Parameters: "/quiet /norestart"; StatusMsg: "Installing ViGEmBus Driver (Kernel Mode)..."; Flags: waituntilterminated
Filename: "{tmp}\HidHide_1.5.230_x64.exe"; Parameters: "/quiet /norestart"; StatusMsg: "Installing HidHide Filter Driver..."; Flags: waituntilterminated

; --- Whitelist ThumbTwister in HidHide ---
; runhidden prevents a console window from flashing during installation
Filename: "{app}\HidHideCLI.exe"; Parameters: "--app-reg ""{app}\ThumbTwister.exe"""; StatusMsg: "Configuring security whitelist..."; Flags: runhidden waituntilterminated

[UninstallRun]
; --- Cleanup: Remove from whitelist when the user uninstalls ---
Filename: "{app}\HidHideCLI.exe"; Parameters: "--app-unreg ""{app}\ThumbTwister.exe"""; RunOnceId: "RemoveHidHideWhitelist"; Flags: runhidden waituntilterminated

[Code]
// --- Force a reboot prompt at the end of installation ---
// Kernel drivers (especially HidHide) require a clean USB stack boot to hook properly.
function NeedRestart(): Boolean;
begin
  Result := True;
end;