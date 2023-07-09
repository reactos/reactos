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
CAB_ResvCodeSigning=0
RebootMode=A
InstallPrompt=%InstallPrompt%
DisplayLicense=%DisplayLicense%
FinishMessage=%FinishMessage%
TargetName=%TargetName%
FriendlyName=%FriendlyName%
AppLaunched=%AppLaunched%
PostInstallCmd=%PostInstallCmd%
AdminQuietInstCmd=%AdminQuietInstCmd%
UserQuietInstCmd=%UserQuietInstCmd%
SourceFiles=SourceFiles

### TargetFileVersion=#S\SHDOCVW.DLL:4.70.0.1300:This update requires Internet Explorer version 3.02 :OK


[Strings]
InstallPrompt=
DisplayLicense=
FinishMessage=Microsoft Internet Security Client has been installed.
TargetName=R:\cryptclt.exe
FriendlyName=Microsoft Internet Security Client
AppLaunched=cryptclt.inf
PostInstallCmd=<none>
;;;;PostInstallCmd=cryptreg.bat
AdminQuietInstCmd=
UserQuietInstCmd=
FILE0="Regsvr32.exe"
FILE1="cryptclt.inf"
FILE2="wintrust.dll"
FILE3="mssip32.dll"
FILE4="softpub.dll"
FILE5="vsrevoke.dll"
FILE6="softpub.hlp"
FILE7="crypt32.dll"
FILE8="cryptreg.bat"
FILE9="mscat32.dll

[SourceFiles]
SourceFiles0=\nt\private\ispu_rel\cabs\i386\
SourceFiles1=\nt\private\ispu_rel\cabs\client\
SourceFiles2=r:\
SourceFiles4=\NT\PRIVATE\ISPU_rel\cabs\


[SourceFiles0]
%FILE0%=

[SourceFiles1]
%FILE1%=

[SourceFiles2]
%FILE2%=
%FILE3%=
%FILE4%=
%FILE5%=
%FILE6%=
%FILE7%=
%FILE9%=

[SourceFiles4]
%FILE8%=



[CatalogHeader]
Name=regress.cat
ResultDir=
PublicVersion=0x00000100
CATATTR1=0x10010001:TESTAttrCatalog1:This is a test value.
CATATTR2=0x10010001:TESTAttrCatalog2:This is a test value.

[CatalogFiles]
TestSignedEXE=testrev.exe
TestSignedEXEATTR1=0x10010001:Type:EXE Signed File -- revoked.
TestSignedEXEATTR2=0x10010001:TESTAttr2File1:This is a test value.
TestSignedEXEATTR3=0x10010001:TESTAttr3File1:This is a test value.

TestSignedEXENoAttr=test2.exe

TestUnsignedCAB=nosntest.cab
TestUnsignedCABATTR1=0x10010001:Type:CAB Unsigned File.

TestSignedCAB=signtest.cab
TestSignedCABATTR1=0x10010001:Type:CAB Signed File.

TestFlat=create.bat
TestFlatATTR1=0x10010001:Type:Flat unsigned file.


