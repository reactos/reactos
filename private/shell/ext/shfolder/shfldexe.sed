
[Version]
Class=IEXPRESS
SEDVersion=3
[Options]
PackagePurpose=InstallApp
ShowInstallProgramWindow=1
HideExtractAnimation=1
UseLongFileName=0
InsideCompressed=0
CAB_FixedSize=0
CAB_ResvCodeSigning=1
RebootMode=I
DisplayLicense=%DisplayLicense%
TargetName=%TargetName%
TargetNTVersion=4-
FriendlyName=%FriendlyName%
AppLaunched=%AppLaunched%
PostInstallCmd=%PostInstallCmd%
AdminQuietInstCmd=%AdminQuietInstCmd%
UserQuietInstCmd=%UserQuietInstCmd%
SourceFiles=SourceFiles

[Strings]
DisplayLicense=
TargetName=.\shfolder.exe
FriendlyName=Shfolder Binary Installer
AppLaunched=shfolder.inf
PostInstallCmd=<None>
AdminQuietInstCmd=
UserQuietInstCmd=

FILE0="shfolder.dll"
FILE1="shfolder.inf"

[SourceFiles]
SourceFiles0=.\
[SourceFiles0]
%FILE0%=
%FILE1%=
