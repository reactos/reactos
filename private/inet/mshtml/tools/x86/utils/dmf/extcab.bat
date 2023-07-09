if "%1"=="" goto syntax

extract /a /e /l %1 word1.cab
goto done

:syntax
echo.
echo Syntax: EXTDMF [destination directory for extracted files]
echo.

:done


