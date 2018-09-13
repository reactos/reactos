[Version]
Class=IEXPRESS
CDFVersion=3

[Options]
PackagePurpose=InstallApp
ShowInstallProgramWindow=0
HideExtractAnimation=0
UseLongFileName=0
InsideCompressed=0
CAB_FixedSize=0
CAB_ResvCodeSigning=6144
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
InstallPrompt=Install/Upgrade Trident MSHTML engine?
DisplayLicense=
FinishMessage=
TargetName=C:\setup\Minimum.EXE
FriendlyName=Trident Minimum Installation
AppLaunched=trident.inf
PostInstallCmd=<None>
FILE0="advpack.dll"
FILE1="mshtmenu.dll"
FILE2="mshtml.dll"
FILE3="W95inf16.dll"
FILE4="w95inf32.dll"
FILE5="trident.txt"
FILE6="htmlpath.exe"
FILE7="trident.inf"

[SourceFiles]
SourceFiles0=C:\setup\
[SourceFiles0]
%FILE0%=
%FILE1%=
%FILE2%=
%FILE3%=
%FILE4%=
%FILE5%=
%FILE6%=
%FILE7%=

