# Nmake macros for building Windows 32-Bit apps

APPVER=4.0

!include <ntwin32.mak>

objs=ws2_chat.obj chatdlg.obj chatsock.obj queue.obj
chatcflags=$(cflags) -DCALLBACK_NOTIFICATION

all: ws2_chat.exe

# Update the resource if necessary

ws2_chat.res: ws2_chat.rc ws2_chat.h
    $(rc) $(rcflags) $(rcvars) ws2_chat.rc

# Update the object file if necessary

.c.obj:
    $(cc) $(cdebug) $(chatcflags) $(cvars) $*.c

# Update the executable file if necessary, and if so, add the resource back in.

ws2_chat.exe: $(objs) ws2_chat.res
    $(link) $(linkdebug) $(guilflags) -out:ws2_chat.exe $(objs) \
	ws2_chat.res $(guilibs) ws2_32.lib
