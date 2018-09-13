rem @echo off

rem
rem This utility is used to verify that changes to kbdtool.exe do not regress
rem keyboard layouts, by comparing old and new kbd layout C source files.
rem To use it:
rem First, build all kbd layout C sources using the old kbdtool then rename
rem the "all_kbds" directory to "all_kbds-"
rem Then ssync -fr the "all_kbds" directory and build all kbd layout C sources again 
rem using the new kbdtool.
rem Then run "diffit" from this directory, and look at the differences.
rem (there should be just version string differences)
rem

cd all_kbds
for %%e in (c h rc def) do for /D %%i in (*) do diff ..\all_kbds-\%%i\%%i.%%e ..\all_kbds\%%i\%%i.%%e
cd ..
