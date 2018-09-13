res.rc: fvmsg.h

fvmsg.h msg00001.bin fvmsg.rc: fvmsg.mc
    mc -v fvmsg.mc

clean::
        -del fvmsg.h fvmsg.rc msg00001.bin
