all:
	gcc -c notepad.c -o notepad.o
	windres -i notepad.rc -o res.o
	gcc notepad.o res.o -o notepad.nostrip.exe -mwindows
	strip notepad.nostrip.exe -o notepad.exe
clean:
	del *.o
	del *.exe