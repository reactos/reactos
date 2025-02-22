::
:: PROJECT:     ReactOS CMD Testing Suite
:: LICENSE:     GPL v2 or any later version
:: FILE:        tests/at.cmd
:: PURPOSE:     Tests for the correct parsing of the "@" character
:: COPYRIGHT:   Copyright 2008 Colin Finck <mail@colinfinck.de>
::

:: Calling a command should work with any number of "@" characters prepended
call :_test "@echo Test >nul"
call :_test "@@echo Test >nul"

:: Files with an "@" sign at the beginning should work as well
echo @^@echo Test > "temp\@file.cmd"
call :_test "call temp\@file.cmd >nul"

echo ^@echo Test > "temp\@file.cmd"
call :_test "call temp\@file.cmd >nul"

goto :EOF
