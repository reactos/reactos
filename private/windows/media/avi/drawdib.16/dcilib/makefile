CC  = cl -c -W2 -Asnw -GD -G3 -Zp -Oxws $(DEF)
ASM = masm -Mx
LINK= link /NOE/NOD/LI/MAP/AL:16/nopackc/onerr:noexe
RC  = rc

!if "$(DEBUG)" != "YES"
DEF =
!else
DEF =-DDEBUG
LINK=$(LINK)/CO
ASM=$(ASM) -Zi
CC=$(CC) -Zi -Fc -Z7
!endif

.cpp.obj:
	$(CC) $*.c

.c.obj:
	$(CC) $*.c

.asm.obj:
        $(ASM) $*;

.rc.res:
        $(RC) -r $(DEF) $*.rc

LIBDIR=..

NAME =dcilib
OBJ  =dciman.obj \
      dva.obj vflat.obj dvavga.obj dvathun.obj dvavlb.obj \
      dvaati.obj dvadib.obj lockbm.obj

goal: $(LIBDIR)\$(NAME).lib

$(LIBDIR)\$(NAME).lib: $(NAME).lib
        copy $** $(LIBDIR)

$(NAME).lib: $(OBJ) makefile.
        del $(NAME).lib
        lib $(NAME).lib +$(OBJ) ;

clean:
        del $(NAME).dll
        del $(NAME).lib
        del *.res
        del *.obj
        del *.map
        del *.sym
        del *.cod
        del *.lst
        del *.pdb
