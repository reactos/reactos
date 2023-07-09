!include "..\..\mfc\mfc\samples\ntsample.mak"

OBJS = symedit.obj symedit.res

all: symedit.exe

symedit.exe:  $(OBJS)
	$(LINK) $CONFLAGS) -out:symedit.exe $(OBJS) $(CONLIBS)

symedit.res: symedit.rc strings.i

symedit.obj: symedit.c ..\include\symcvt.h ..\include\cv.h \
        ..\include\cofftocv.h ..\include\symtocv.h strings.h

strings.i:
        $(CC) -E -DRESOURCES strings.c | findstr -v /C:"#" > strings.i
