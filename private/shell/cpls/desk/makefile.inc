setmsg.h msg00001.bin setmsg.rc: ..\setmsg.mc
   mc -v ..\setmsg.mc

..\desk.rc : ..\selfreg.inf

!include $(CCSHELL_DIR)\makefile.inc

clean::
        -del setmsg.h setmsg.rc msg00001.bin
