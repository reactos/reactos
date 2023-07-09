#----------------------------------------------------------------------------
# /Build the file cabinet app.
#----------------------------------------------------------------------------

!ifdef NTMAKEENV    # Prevent NT's build.exe from executing this makefile

#
# Build using BUILD.EXE (Do not edit this section of this file, edit SOURCES)
#
!INCLUDE $(NTMAKEENV)\makefile.def

!else  # NTMAKEENV


ROOT            = ..\..\..
RES_DIR         = ..\..
SRCDIR          = ..\..
WIN32           = TRUE
IS_DDK          = TRUE
DEFENTRY        = ModuleEntry@0
CCSHELL_DIR     = $(ROOT)\shell

!ifdef VERDIR
ROOT            = ..\$(ROOT)
!endif

NAME            = Explorer
RESNAME         = cabinet.res
RCNAME          = cabinet.rc
RCVNAME         = cabinet.rcv
PRIVINC         = cabinet
DEFNAME         = cabinet.def

CLEANLIST       = *.pdb

L32FLAGS        = $(L32FLAGS) -entry:ModuleEntry@0 -heap:0x400000,0x1000

!ifndef NOFILESYNC
# By default, the briefcase is enabled now
CFLAGS          = $(CFLAGS) -DSYNC_BRIEFCASE
RCFLAGS         = $(RCFLAGS) -DSYNC_BRIEFCASE
!endif

#
#   list all .cpp files here, they will use a different precompiled header
#   file than all the .c files.
#
CPPPCHOBJS      = tlog.obj

!ifdef  WINDOWS_ME
CFLAGS          = $(CFLAGS) -DWINDOWS_ME -D$(WINDOWS_ME)
!endif

PCHOBJ0         = mainloop.obj cstrings.obj options.obj
PCHOBJ1         = fcext.obj fldset.obj debug.obj
PCHOBJ2         = dde.obj desktop.obj tray.obj trayclok.obj task.obj     \
                  traynot.obj
PCHOBJ3         = cabwnd.obj command.obj drivlist.obj tree.obj \
                  onetree.obj treedrop.obj fldrlist.obj dllload.obj\

!ifndef NEW_FILE_MENU_STUFF
PCHOBJ4         = initcab.obj cfgstart.obj sfcache.obj \
                  message.obj recent.obj
!else
PCHOBJ4         = initcab.obj cfgstart.obj sfcache.obj \
                  message.obj startmnu.obj hotkeys.obj  recent.obj
!endif

#
# WARNING: shell32.lib MUST be placed before mpr.lib to pick DllGetClassObject
#         from shell32.lib correctly.
#
LIB0            = user32.lib gdi32.lib shlwapi.lib shell32.lib mpr.lib kernel32.lib \
                  comctl32.lib advapi32.lib uuid.lib libcmt.lib \
                  shguid.lib ole32.lib oleaut32.lib
LIB1            = $(CCSHELL_DIR)\lib\$(VERDIR)\stocklib.lib shguidp.lib

!ifdef DBCS
LIB0            = $(LIB0) imm32.lib
!endif

!include $(ROOT)\shell\common.mk


!ifdef VERDIR

!if !exist($(CCSHELL_DIR)\lib\$(VERDIR)\stocklib.lib)
!message ****
!message **** Warning: You need to ssync -ur ccshell\lib
!message ****
!error
!endif


$(NAME).res:	$(SRCDIR)\cab32.ico $(SRCDIR)\start.bmp

#
# Rule to generate source file dependencies
#

$(SRCDIR)\depend.mk::
!if "$(BUILD)" == "depend"
        echo # > $@
        echo # Warning: This file is generated automatically. >> $@
        echo # >> $@
        echo. >> $@
        $(INCLUDES) $(INCLUDES_SWITCHES) @<< | sed "s/pch.obj/pch.obj .\\cabinet.pch .\\pchcpps.obj .\\pchcpps.pch/" >> $@
-D..\..
pch.c
<<
        $(INCLUDES) $(INCLUDES_SWITCHES) @<< >> $@
-D..\..
-ncabinet.h ..\..\*.c ..\..\*.asm
<<
        $(INCLUDES) $(INCLUDES_SWITCHES) @<< >> $@
-D..\..
..\..\*.cpp
<<
!if exist($(SRCDIR)\$(NAME).rc)
        $(INCLUDES) $(INCLUDES_SWITCHES) -C=acl -C=dlg -C=mnu -C=rc -C=stb -C=ver -sres @<< >> $@
$(NAME).rc
<<
!endif
!else
        @echo Use "nmake BUILD=depend" to build dependencies.
!endif

!endif  # VERDIR

!endif # NTMAKEENV
