# Nmake macros for building Windows 32-Bit apps

TARGETOS=WINNT

!include <win32.mak>

all: testdll.dll

# Update the object files if necessary

testdll.obj: testdll.c
	 $(cc) $(cflags) $(cvarsdll) $(cdebug) testdll.c

testmib.obj: testmib.c
	 $(cc) $(cflags) $(cvarsdll) $(cdebug) testmib.c


# Update the import library

testdll.lib: testdll.obj testmib.obj testdll.def
	 $(implib) -machine:$(CPU)     \
	 -def:testdll.def     \
	 testdll.obj          \
	 testmib.obj          \
	 -out:testdll.lib

# Update the dynamic link library

testdll.dll: testdll.lib testdll.obj testmib.obj testdll.def
	 
	 $(link) $(lflags) $(dlllflags) \
	 -base:0x1C000000  \
	 -out:testdll.dll   \
	 testdll.exp testdll.obj testmib.obj \
         $(guilibsdll) snmpapi.lib
