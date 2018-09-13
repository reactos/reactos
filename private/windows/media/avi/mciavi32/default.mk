#
# 16- or 32-bit compilation.  Put WIN32=TRUE on nmake command
# line for 32-bit. (e.g. nmake /f make32c WIN32=TRUE)
#
#
!if "$(WIN32)" != "TRUE"
WIN16	=TRUE
!endif

#
# Debug options
#
!if ("$(DEBUG)" == "retail")
DEF     =
CDEBUG	=$(DEF)
ADEBUG	=$(DEF)
RDEBUG	=$(DEF)
L16DEBUG=
L32DEBUG=-debug:none
!else
!if ("$(DEBUG)" == "debug")
DEF     =-DRDEBUG
CDEBUG	=$(DEF)
ADEBUG	=$(DEF)
RDEBUG	=$(DEF)
L16DEBUG=
L32DEBUG=-debug:none
!else
DEF     =-DDEBUG
CDEBUG	=$(DEF)
ADEBUG	=-Zi $(DEF)
RDEBUG	=$(DEF)
L16DEBUG=/CO/LI
L32DEBUG=-debug:full -debugtype:cv
!endif
!endif

#
# Strict type checking option
#
#
!if "$(STRICT)" == "NO"
ZSTRICT =
!else
!if "$(STRICT)" == "FULL"
ZSTRICT = -DSTRICT -Tp
!else
ZSTRICT = -DSTRICT
!endif
!endif

#
#
#
BASE	= mciavi
!if "$(WIN32)" == "TRUE"
NAME	=$(BASE)32
DEFFILE	=$(BASE)32
EXT	=dll
!else
NAME	=$(BASE)
DEFFILE = $(BASE)
EXT	=drv
!endif
#ROOT	=..\..\..

#
#
#
!if "$(WIN32)" == "TRUE"
OBJ1    = avidraw.obj aviplay.obj avisound.obj avitask.obj aviopen.obj common.obj mciup32.obj 
OBJ2	= config.obj device.obj drvproc.obj graphic.obj window.obj fullproc.obj aviread.obj drawproc.obj profile.obj init.obj
!else
OBJ1    = libinit.obj graph16.obj mciup16.obj init.obj
OBJ2	= 
!endif

!if ("$(DEBUG)" == "retail") || ("$(DEBUG)" == "debug")
OBJDEB	=
!else
OBJDEB	= 
!endif

OBJS    = $(OBJ1) $(OBJ2) $(OBJDEB)

#
#
#
!if "$(WIN32)" == "TRUE"
LIBS	=gdi32.lib user32.lib kernel32.lib winmm.lib msvfw32.lib advapi32.lib crtdll.lib
!else
LIBS	=libw.lib mdllcew.lib mmsystem.lib
!endif

#
#
#
GOALS   =$(PBIN)\$(NAME).$(EXT) $(PBIN)\$(NAME).sym $(PLIB)\$(NAME).lib $(PINC)\mciavi.H


#
#
#
!if "$(WIN32)" == "TRUE"
#
# Chicago 32-bit build
#
CFLAGS	=-Oxt -Gf -Zp -D_X86_ -D_MT -DWIN32 -DWINVER=0x0400 -DWIN4 $(CDEBUG) -I$(LRES) -Fo$@
AFLAGS	=$(ADEBUG)
RCFLAGS =-v -DWIN32 -DWINVER=0x400 $(RDEBUG)
L32FLAGS=$(L32DEBUG)
IS_32	=TRUE
WANT_C832 = TRUE

!else
#
# Chicago 16-bit build
#
CFLAGS	=-GD -Oxwt -Alnw -DWINVER=0x400 -DWIN4 $(CDEBUG) -I$(PVER) -Fo$@
#CFLAGS	=-GD -Oxwt -Alnw -DWINVER=0x30a $(CDEBUG) -I$(PVER) -Fo$@
AFLAGS	=-D?MEDIUM -D?QUIET $(ADEBUG)
RCFLAGS =-z $(RDEBUG) -v -DWINVER=0X400
#RCFLAGS =-z $(RDEBUG) -v -DWINVER=0X30a
L16FLAGS=/AL:16/NOD $(L16DEBUG)
RESFLAGS=
IS_16	=TRUE
!endif

WANT_MASM61 = TRUE
IS_OEM	=TRUE


!include $(ROOT)\build\project.mk

#
#
#
#   Dependencies
#
#
#

!ifdef WIN32
SEG=
!else
SEG= -NT S
!endif


#
# Chicago thunk module
#
types.thk:	..\..\$$@
    copy %s $@

mciup.asm: types.thk ..\..\mciup.thk
    thunk -t mciup -o mciup.asm ..\..\mciup.thk

mciup16.obj:	mciup.asm
    ml -c -DIS_16 -W3 -Zi -nologo -Fo mciup16.obj mciup.asm

mciup32.obj:	mciup.asm 
    ml -c -DIS_32 -W3 -Zi -nologo -Fo mciup32.obj mciup.asm

#
#
#
$(BASE).res: ..\..\$$(@B).rc ..\..\$$(@B).h \
		$(PVER)\verinfo.h $(PVER)\verinfo.ver
	@$(RC) $(RCFLAGS) -fo$@ -I$(PVER) -I..\.. ..\..\$(@B).rc

$(NAME).$(EXT) $(NAME).map: $(OBJS) $(BASE).res ..\..\$(DEFFILE).def
!if "$(WIN32)" == "TRUE"
	@$(LINK32) $(L32FLAGS) @<<
-out:$(@B).$(EXT)
-machine:$(OS)
-subsystem:windows,4.0
-dll
-entry:DllEntryPoint
-base:0x40010000
-map:$(@B).map
-def:..\..\$(DEFFILE).def
$(BASE).res
$(LIBS)
$(OBJS)
<<
!else
	$(LINK16) @<<
$(OBJ1) +
$(OBJ2) +
$(OBJDEB),
$(@B).$(EXT) $(L16FLAGS),
$(@B).map,
$(LIBS),
..\..\$(DEFFILE).def
<<
	$(RC) $(RESFLAGS) $(BASE).res $*.$(EXT)
!endif
