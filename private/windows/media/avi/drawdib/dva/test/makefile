CC  = cl -c -W3 -AS -G2sw -Zip -Oxwt -Fc -I.. -I..\.. $(DEF)
ASM = masm -Mx -Zi
LINK= link /CO/NOE/NOD/LI/MAP/AL:16
NAME= qa
DEF =-DDEBUG
RC  = rc

OBJ  = qa.obj fill.obj lockbm.obj
LIBS = libw slibcew mmsystem ..\dos\usa\dva

.c.obj:
	$(CC) $*.c

.asm.obj:
        $(ASM) $*;

goal: $(NAME).exe

$(NAME).exe: $(OBJ) $(NAME).res $(NAME).def makefile
        $(LINK) @<<
        $(OBJ), $(NAME), $(NAME),$(LIBS), $(NAME).def
<<
        rc -31 $(NAME).res
        -cvpack -p $(NAME).exe
        -mapsym $(NAME).map

$(NAME).res: $(NAME).rc $(NAME).ico
        $(RC) -r $(NAME).rc

lockbm.obj: ..\..\lockbm.c
        $(CC) ..\..\$*.c -Fo$*.obj

clean:
        del $(NAME).exe
        del *.res
        del *.err
        del *.obj
        del *.map
        del *.sym
        del *.cod
        del *.pdb

depend:
	mv makefile makefile.old
	sed "/^# START Dependencies/,/^# END Dependencies/D" makefile.old > makefile
	del makefile.old
	echo # START Dependencies >> makefile
	includes -l *.c *.asm >> makefile
	echo # END Dependencies >> makefile

# START Dependencies  
# END Dependencies  
