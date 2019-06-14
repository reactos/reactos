for /d %%x in (objfre_*) do rmdir /S /Q %%x
for /d %%x in (objchk_*) do rmdir /S /Q %%x
rmdir /S /Q Win8Release
rmdir /S /Q Win8Debug
rmdir /S /Q x64

del /F *.log *.wrn *.err

cd WDF
call clean.bat
cd ..
