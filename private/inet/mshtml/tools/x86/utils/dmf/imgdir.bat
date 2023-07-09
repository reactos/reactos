@echo off
MD DskImage
FOR %%I IN (2 3 4 5 6 7 8) DO dmfimage disk%%I .\DskImage\DISK%%I.IMG /f:dmf /label:%1%%I
copy c:\binr\dmfwrite.exe .\DskImage\DMFWrite.exe
copy c:\batch\makedisk.bat .\DskImage\MakeDisk.bat
copy c:\batch\image.bat .\DskImage\Image.bat

