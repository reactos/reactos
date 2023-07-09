erase /s client\obj\csend.obj
erase /s client\obj\crecv.obj
erase /s client\obj\clmsg.obj
erase /s server\obj\ssend.obj
erase /s server\obj\srecv.obj
erase /s server\obj\srecv2.obj
erase /s server\obj\srvmsg.obj

erase client\dispcb.c
erase server\dispcf.c
erase inc\callback.h
erase inc\csuser.h

nmake -f makefil0
