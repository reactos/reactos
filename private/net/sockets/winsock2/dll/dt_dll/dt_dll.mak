TARGETOS=WIN95
APPVER=4.0

!include <ntwin32.mak>

all: dt_dll.dll

dt_dll.dll: dt_dll.obj huerror.obj handlers.obj dt_dll.res 
	$(link) $(linkdebug) $(dlllflags) \
	-export:WSAPreApiNotify -export:WSAPostApiNotify \
	-out:$*.dll $** $(guilibsdll)
	

.cpp.obj:
    $(cc) $(cdebug) $(cflags) $(cvarsdll) $*.cpp

dt_dll.res: dt_dll.rc
    $(rc) $(rcflags) $(rcvars)  dt_dll.rc
