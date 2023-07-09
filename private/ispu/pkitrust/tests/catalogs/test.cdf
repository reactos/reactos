

[CatalogHeader]
Name=test.p7s
ResultDir=
PublicVersion=0x00000100


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

regress.cdf=regress.cdf
regress2.cdf=regress2.cdf
create.bat=create.bat
testrev.exe=testrev.exe
test2.exe=test2.exe
signtest.cab=signtest.cab
nosntest.cab=nosntest.cab
publish.pvk=publish.pvk
publish.spc=publish.spc
