@echo off
set processor=%processor_architecture%
if %processor%==x86 set processor=i386
rundll32 dll\obj\%processor%\netplwiz.dll,NetConnectWizard

