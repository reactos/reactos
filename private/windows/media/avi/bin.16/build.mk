!ifndef NAME
NAME	=default
!endif
!ifndef ROOT
ROOT	=..
!endif

# Need to use *.mak because 32 bit nmake is broken
goal:	debug.mak retail.mak

all:	internal.mak debug.mak retail.mak

ntwow:
	@nmake MAKE1632= -nologo NTWOW=1

docs:
	@autodoc /x $(DOCS) /o help\$(NAME).doc *.c *.d

apidocs:
	@autodoc /x $(APIDOCS) /o help\apidoc.doc *.c *.d

apistructdocs:
	@autodoc /x $(APISTRUCTDOCS) /o help\apistrct.doc *.c *.d

messagedocs:
	@autodoc /x $(MESSAGEDOCS) /o help\message.doc *.c *.d

messagestructdocs:
	@autodoc /x $(MESSAGESTRUCTDOCS) /o help\messtrct.doc *.c *.d

!ifndef FMT
FMT	=autodoc.fmt
!endif
help:
	@autodoc /rh /x $(HELP) /f $(FMT) /o help\$(NAME).doc *.c *.d
	hc help\$(NAME).hpj

debug internal retail:	$@.mak

debug.mak internal.mak retail.mak:
	@if not exist $(@B)\nul md $(@B)
	@if not exist $(@B)\win16\nul md $(@B)\win16
	@if not exist $(ROOT)\mciavi32\vfw16\$(@B)\nul        md $(ROOT)\mciavi32\vfw16\$(@B)
        @if not exist $(ROOT)\mciavi32\vfw16\$(@B)\inc.16\nul md $(ROOT)\mciavi32\vfw16\$(@B)\inc.16
 	@if not exist $(ROOT)\mciavi32\vfw16\$(@B)\lib.16\nul md $(ROOT)\mciavi32\vfw16\$(@B)\lib.16
	@cd $(@B)\win16
	@nmake -nologo -f ..\..\$(NAME).mk DEBUG="$(@B)"
        @cd ..\..


clean:	debug.cln internal.cln retail.cln

debug.cln internal.cln retail.cln:
!if "$(NO_OBJ)" != "TRUE"
!if "$(WIN32)" == "TRUE" || "$(MAKE1632)" == "TRUE"
	@if exist $(@B)\win32\nul del $(@B)\win32 <$(ROOT)\build\yes >nul
	@if exist $(@B)\win32\nul rd $(@B)\win32 >nul
!endif
!if "$(WIN32)" != "TRUE" || "$(MAKE1632)" == "TRUE"
	@if exist $(@B)\win16\nul del $(@B)\win16 <$(ROOT)\build\yes >nul
	@if exist $(@B)\win16\nul rd $(@B)\win16 >nul
!endif
	@if exist $(@B)\nul rd $(@B) >nul
!endif
	@echo ***** Finished cleaning $(@B) *****
