set COPYCMD=/y
if exist ..\..\..\include\reactos\buildno.h copy ..\..\..\include\reactos\buildno.h . >nul
if not exist buildno.h copy buildno.h.templ buildno.h
