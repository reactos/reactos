!if "$(OS)" == "Windows_NT"
!INCLUDE $(NTMAKEENV)\makefile.def
!else

CC   = cl -c -W2 -AS -Zip -G2 -Ows $(DEF)
LINK = link /NOE/CO/LI/NOPACK/MAP/NOD
LIBS = slibcer

.c.obj:
	$(CC) $*.c

goal : encrypt.exe people.cry

encrypt.exe : encrypt.obj
	$(LINK) encrypt.obj, encrypt, encrypt, $(LIBS) ;

people.cry : encrypt.exe people.txt
	encrypt people.txt people.cry

clean:
	del *.exe
	del *.sym
	del *.obj
	del *.map

!endif
