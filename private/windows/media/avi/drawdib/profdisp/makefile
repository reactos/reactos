!if "$(OS)" == "Windows_NT"
!INCLUDE $(NTMAKEENV)\makefile.def
!else

CC  = cl -c -W3 -AS -G2sw -Zip -Oxas $(DEF)
ASM = masm -Mx -Zi
LINK= link /CO/NOE/NOD/LI/MAP/AL:16
NAME= profdisp
DEF =-DDEBUG
RC  = rc

OBJ  = profdisp.obj
LIBS = libw slibcew msvideo

.c.obj:
	$(CC) $*.c

.asm.obj:
	$(ASM) $*;

goal: $(NAME).exe

$(NAME).exe: $(OBJ) $(NAME).res $(NAME).def makefile
        $(LINK) $(OBJ), $(NAME), $(NAME),$(LIBS), $(NAME).def
        rc $(NAME).res
        -cvpack -p $(NAME).exe
        -mapsym $(NAME).map

$(NAME).res: $(NAME).rc
        $(RC) -r $(NAME).rc

clean:
        del $(NAME).exe
        del *.res
        del *.err
        del *.obj
        del *.map
        del *.sym

depend:
	mv makefile makefile.old
	sed "/^# START Dependencies/,/^# END Dependencies/D" makefile.old > makefile
	del makefile.old
	echo # START Dependencies >> makefile
	includes -l *.c *.asm >> makefile
	echo # END Dependencies >> makefile

# START Dependencies
# END Dependencies

!endif
