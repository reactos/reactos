if exist ..\..\..\include\reactos\buildno.h copy /y ..\..\..\include\reactos\buildno.h . >nul
if not exist buildno.h copy /y buildno.h.templ buildno.h
