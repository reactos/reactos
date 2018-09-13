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

CC   = cl -c -nologo -AS -G2s -Zp -W3 -Oxwti $(DEF) $(COPT)
RC   = rc $(DEF)
ASM  = masm -Mx -t $(MOPT) $(DEF)
LINK = link $(LOPT)/NOD/NOE/MAP/NOPACKC/AL:16

NAME= ddt
EXENAME=ddt.exe

OBJ  = ddt.obj profdisp.obj
LIBS = libw slibcew

.c.obj:
	$(CC) $*.c

.asm.obj:
        $(ASM) $*;

.rc.res:
        $(RC) -r $*.rc

goal: $(EXENAME)

$(EXENAME): $(OBJ) $(NAME).res $(NAME).def makefile
        $(LINK) @<<
$(OBJ),
$(EXENAME),
$(NAME),
$(LIBS),
$(NAME).def
<<
        -cvpack -p $(EXENAME)
        -mapsym $(NAME).map
        $(RC) $(NAME).res $(EXENAME)

profdisp.obj: ..\profdisp.c

clean:
        del $(EXENAME)
        del *.res
        del *.obj
        del *.map
        del *.sym
        del *.cod
