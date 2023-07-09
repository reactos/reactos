desk.rc: settings.dlg setmsg.h

setmsg.h msg00001.bin setmsg.rc: setmsg.mc
    mc -v setmsg.mc

clean::
        -del setmsg.h setmsg.rc msg00001.bin
