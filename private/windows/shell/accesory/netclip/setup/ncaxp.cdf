[Version]
Class=IEXPRESS
CDFVersion=2.0

[Options]
ExtractOnly=1
ShowInstallProgramWindow=0
HideExtractAnimation=0
UseLongFileName=0
RebootMode=I
InstallPrompt=%InstallPrompt%
DisplayLicense=%DisplayLicense%
FinishMessage=%FinishMessage%
TargetName=%TargetName%
FriendlyName=%FriendlyName%
AppLaunched=%AppLaunched%
PostInstallCmd=%PostInstallCmd%
SourceFiles=SourceFiles

[Strings]
InstallPrompt=""
DisplayLicense=""
FinishMessage="Remote Clipboard Viewer Installation Complete"
TargetName=".\ncaxp.exe"
FriendlyName="Remote Clipboard Viewer Installation"
AppLaunched=""
PostInstallCmd="<None>"
FILE0="netclip.inf"
FILE1="NetClip.exe"
FILE2="nclipps.dll"

[SourceFiles]
SourceFiles0=.\

[SourceFiles0]
%FILE0%
%FILE1%
%FILE2%
