@SETLOCAL ENABLEEXTENSIONS

@set __TIMESTAMPURL=http://timestamp.verisign.com/scripts/timstamp.dll
@set __FILE=driver
@set __CATALOGFILE=catalog

@rem
@rem create the catalog file
@rem
@REM bin\makecat %__CATALOGFILE%.cdf

@if exist %__FILE%.cer  del /q %__FILE%.cer
@if exist %__FILE%.pvk  del /q %__FILE%.pvk
@if exist %__FILE%.spc  del /q %__FILE%.spc

makecert -eku "1.3.6.1.4.1.311.10.3.5" -sv %__FILE%.pvk -n "CN=Microsoft Windows NT Build Lab TEST" %__FILE%.cer

cert2spc %__FILE%.cer %__FILE%.spc

setreg -q 1 TRUE

signcode -v %__FILE%.pvk -spc %__FILE%.spc -n "Microsoft Windows NT Driver Catalog TEST" -i "http://ntbuilds" -t "http://timestamp.verisign.com/scripts/timstamp.dll" -tr 2 -tw 2 %__CATALOGFILE%.CAT

@ENDLOCAL