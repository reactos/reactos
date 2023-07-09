!ifdef NTMAKEENV

all:
clean:

!else
DOMAKE=$(MAKE) /$(MAKEFLAGS: =) /nologo -f f3\make\master.mak root SRCROOT=$(MAKEDIR) _RELEASE=$(_RELEASE)

!if "$(_RELEASE)" == ""
!if exist(build.inc)
!include build.inc
!endif
!endif

LOGCMD=
!ifdef LOGFILE
LOGCMD= /x - 2>&1 | tee $(MAKEDIR)\$(LOGFILE)
DLOGCMD= $(LOGCMD)_d
!endif

default: fall



corelibs:
        $(DOMAKE:root=corelibs) TARGET=all

formlibs:
        $(DOMAKE:root=formlibs) TARGET=all

f3libs:
        $(DOMAKE:root=f3libs) TARGET=all

ddoclibs:
        $(DOMAKE:root=ddoclibs) TARGET=all

ctrllibs:
        $(DOMAKE:root=ctrllibs) TARGET=all

sitelibs:
        $(DOMAKE:root=sitelibs) TARGET=all

uuidlibs:
        $(DOMAKE:root=uuid) TARGET=all

fclean:
        $(DOMAKE) TARGET=clean
        @echo Clean finished.

fdepend:
        $(DOMAKE) TARGET=depend $(DLOGCMD)
        @echo Dependency finished.

ffall:
        $(DOMAKE) TARGET=all $(LOGCMD)

fall: ffall beep drop

ffresh: fclean fdepend fall

fast: kill fall fdepend

fmaccopy:
        $(DOMAKE) TARGET=maccopy
        @ECHO MFile Copy finished.

beep:
   @echo Build Finished.
   @ECHO 


ROOT=$(MAKEDIR:\src=)
MODULE=rootmake
!include f3\make\make1.inc

kill:
        $(ECHOTIME) ------ /H:M:S ------- Doing Quick Clean of $(BLDROOT)...
        $(DELTREE) $(BLDROOT)

drop :
!if "$(_DROPDIR)" != ""
        @ECHO Copying binaries to $(_DROPDIR)
        -$(MKDIR) $(_DROPDIR)
        $(COPY) $(BINDIR)\mshtm*.exe $(_DROPDIR) $(EAT_ERRORS)
        $(COPY) $(BINDIR)\mshtm*.dll $(_DROPDIR) $(EAT_ERRORS)
        -$(COPY) $(BINDIR)\mshtm*.pdb $(_DROPDIR) $(EAT_ERRORS)
        -$(COPY) $(BINDIR)\mshtm*.dbg $(_DROPDIR) $(EAT_ERRORS)
!endif

cleandrop :
!if "$(_DROPDIR)" != ""
        @ECHO Deleting directory $(_DROPDIR)
        -$(DELTREE) $(_DROPDIR)
        -$(RMDIR) $(_DROPDIR) $(EAT_ERRORS)
!endif

!endif		# NTMAKEENV
