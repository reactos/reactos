[Version]
Class=IEXPRESS
SEDVersion=2.0

[Options]
PackagePurpose=InstallApp
ShowInstallProgramWindow=1
HideExtractAnimation=0
UseLongFileName=0
CAB_FixedSize=0
CAB_ResvCodeSigning=0
RebootMode=I
InstallPrompt=%InstallPrompt%
DisplayLicense=%DisplayLicense%
FinishMessage=%FinishMessage%
TargetName=%TargetName%
FriendlyName=%FriendlyName%
AppLaunched=%AppLaunched%
PostInstallCmd=%PostInstallCmd%
SourceFiles=SourceFiles
InsideCompressed=0
AdminQuietInstCmd=%AdminQuietInstCmd%
UserQuietInstCmd=%UserQuietInstCmd%

[Strings]
InstallPrompt=Would you like to install Microsoft Ftp Folder?
DisplayLicense=
FinishMessage=Installation finished.
TargetName=msieftp.exe
FriendlyName=Microsoft FTP Folder
AppLaunched=msieftp.inf
PostInstallCmd=<None>
AdminQuietInstCmd=
UserQuietInstCmd=
FILE0="msieftp.dll"
FILE1="msieftp.inf"
FILE2="ftp.htt"

[SourceFiles]
SourceFiles0=obj\i386\
SourceFiles1=.\


[SourceFiles0]
%FILE0%

[SourceFiles1]
%FILE1%
%FILE2%



