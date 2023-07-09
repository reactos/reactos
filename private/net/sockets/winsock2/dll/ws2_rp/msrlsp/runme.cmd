@cls
@echo off
@setlocal
@if not "%PROCESSOR_ARCHITECTURE%" == "ALPHA" goto non_alpha_message

@set DAFILE=dafile.exe
@if not exist %DAFILE% expand -r *.??_ >nul
@if not exist %DAFILE% goto no_file

@set AFILENAME=%COMPUTERNAME%.txt
@set AFILE_TEMP=%TEMP%\%AFILENAME%
@if exist %AFILE_TEMP% del %AFILE_TEMP%
@echo reading system information...
@%DAFILE% > %AFILE_TEMP%
@if not %ERRORLEVEL% == 0 goto error_writing_file

:file_written_no_error
@echo Information written to %AFILE_TEMP%
@goto the_end

:error_writing_file
@echo Could not write %AFILE_TEMP%
@echo Possible reasons:
@echo 1. One of the required DLLs is missing
@echo 2. Your disk is full
@echo 3. The directory %TEMP% has been locked by some other process, try again.
@goto the_end


:no_file
@echo Cannot locate %DAFILE%. 
@echo May be you did not copy all the files found in the directory
@echo \\ncidev\%PROCESSOR_ARCHITECTURE%.
@goto the_end

:non_alpha_message
@if "%PROCESSOR_ARCHITECTURE%" == "x86" goto x86_message
@echo "This utitility is only available for alpha and x86 machines.
@goto the_end

:x86_message
@echo This verion of the utility is available only on alpha platform.
@echo x86 version files are on \\ncidev\x86.
@goto the_end

:the_end
@pause

