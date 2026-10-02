@if "%1"=="" echo Missing number of samples 0 or 2-16 & exit /b 1
reg add HKLM\SOFTWARE\Microsoft\Avalon.Graphics /v MaxMultisampleType /t REG_DWORD /d %1 /f
