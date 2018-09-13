
!if "$(NTMAKEENV)" != ""
!include $(NTMAKEENV)\makefile.def
!else

#===========================================================================;
#
#   AVIView -- AVI Viewer sample application
#
#   Copyright (C) Microsoft Corporation 1992-1993. All Rights reserved.
#
#---------------------------------------------------------------------------;
#
#   to make a NON-DEBUG build, type the following line:
#       nmake DEBUG=NO
#   just typing 'nmake' will build a DEBUG build
#
#
#===========================================================================;


#---------------------------------------------------------------------------;
# DEBUG information
#---------------------------------------------------------------------------;
!if "$(DEBUG)" == "NO"
COPT =
MOPT =
LOPT =
DEF  =
!else
COPT = -Zi -Fc
MOPT = -Zi -L
LOPT = /CO
DEF  = -DDEBUG
!endif


#-------------------------------------------------------------------------;
# Build Defaults
#-------------------------------------------------------------------------;
CC   = cl -c -nologo -AM -Zp -Od -G2s -GA $(DEF) $(COPT)
RC   = rc $(DEF) -31
ASM  = masm -Mx  -t $(MOPT) $(DEF)
LINK = link $(LOPT)/NOD/NOE/MAP/NOPACKC/AL:16



#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
#
#   Goals and objects
#
#
#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;

NAME	= aviview
EXT	= exe
GOALS   = $(NAME).$(EXT)

OBJS	=$(NAME).obj audplay.obj aviball.obj muldiv32.obj aviplay.obj
LIBS	= libw mlibcew commdlg mmsystem vfw compobj msacm


#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
#
#   Build rules
#
#
#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
.c.obj:
	$(CC) $*.c

.asm.obj:
	$(ASM) $*;

.rc.res:
	$(RC) -r $*.rc


goal: $(GOALS)
        @echo ***** Finished building $(NAME).$(EXT) *****


$(NAME).$(EXT): $(OBJS) $(NAME).res $(NAME).def
	$(LINK) @<<
$(OBJS),
$(NAME).$(EXT),
$(NAME),
$(LIBS),
$(NAME).def
<<
	-cvpack -p $(NAME).$(EXT)
        mapsym $(NAME).map
        $(RC) $(NAME).res $(NAME).$(EXT)


clean:
	del $(NAME).$(EXT)
	del *.res
	del *.obj
	del *.map
	del *.sym
	del *.cod
	del *.pdb
	del *.bsc
	del *.lst

!endif
