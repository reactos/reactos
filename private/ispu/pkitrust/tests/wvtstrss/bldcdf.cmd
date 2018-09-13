
@echo off

@SETLOCAL ENABLEEXTENSIONS

set CDF_FNAME=dtest.cdf
set CAT_FNAME=dtest.cat


echo # test cdf file> %CDF_FNAME%
echo [CatalogHeader]>> %CDF_FNAME%
echo Name=%CAT_FNAME%>> %CDF_FNAME%
echo ResultDir=>> %CDF_FNAME%
echo PublicVersion=0x00000100>> %CDF_FNAME%
echo CATATTR1=0x10010001:OSAttr:2:5.x, 2:4.x, 1:1.x>> %CDF_FNAME%
echo #>> %CDF_FNAME%
echo [CatalogFiles]>> %CDF_FNAME%

@dir /a-d /b %SystemRoot%\SYSTEM32\*.* > %COMPUTERNAME%.DIR

FOR /F %%i IN (%COMPUTERNAME%.DIR) DO call :AddToCDF %%i

goto bldcdf_end



:AddToCDF

echo %1=%SystemRoot%\SYSTEM32\%1>> %CDF_FNAME%

goto :EOF

:bldcdf_end

@del %COMPUTERNAME%.DIR

makecat %CDF_FNAME%

@ENDLOCAL
