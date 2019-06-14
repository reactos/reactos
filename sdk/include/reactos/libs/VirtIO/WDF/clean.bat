@echo off

for /d %%x in (objfre_*) do rmdir /S /Q %%x
for /d %%x in (objchk_*) do rmdir /S /Q %%x

del /F *.log *.wrn *.err
