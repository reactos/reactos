#
# ncftp 3.0.3 for reactos ported from MS-Vc++
# sedwards 12-20-01

PATH_TO_TOP=../../../reactos

ROS_DIR=../../../reactos

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = ncftp

TARGET_SDKLIBS = kernel32.a ws2_32.a

TARGET_CFLAGS = -Dncftp

TARGET_CLEAN = \
sio/*.o Strn/*.o libncftp/*.o ncftp/*.o

OBJECTS_SIO = \
sio\PRead.o \
sio\PWrite.o \
sio\SAcceptA.o \
sio\SAcceptS.o \
sio\SBind.o \
sio\SClose.o \
sio\SConnect.o \
sio\SConnectByName.o \
sio\SNew.o \
sio\SRead.o \
sio\SReadline.o \
sio\SRecv.o \
sio\SRecvfrom.o \
sio\SRecvmsg.o \
sio\SSelect.o \
sio\SSend.o \
sio\SSendto.o \
sio\SSendtoByName.o \
sio\SWrite.o \
sio\SocketUtil.o \
sio\StrAddr.o \
sio\UAcceptA.o \
sio\UAcceptS.o \
sio\UBind.o \
sio\UConnect.o \
sio\UConnectByName.o \
sio\UNew.o \
sio\URecvfrom.o \
sio\USendto.o \
sio\USendtoByName.o \
sio\SError.o \
sio\main.o \
sio\SWait.o 

OBJECTS_STRN = \
Strn\Dynscat.o \
Strn\Strncpy.o \
Strn\Strncat.o \
Strn\Strntok.o \
Strn\Strnpcpy.o \
Strn\Strnpcat.o \
Strn\strtokc.o \
Strn\version.o 

OBJECTS_LIBNCFTP = \
libncftp\open.o \
libncftp\cmds.o \
libncftp\util.o \
libncftp\rcmd.o \
libncftp\ftp.o \
libncftp\io.o \
libncftp\errno.o \
libncftp\linelist.o \
libncftp\glob.o 

OBJECTS_NCFTP = \
ncftp\cmds.o \
ncftp\cmdlist.o \
ncftp\getopt.o \
ncftp\ls.o \
ncftp\main.o \
ncftp\version.o \
ncftp\shell.o \
ncftp\util.o \
ncftp\readln.o \
ncftp\progress.o \
ncftp\bookmark.o \
ncftp\pref.o \
ncftp\preffw.o \
ncftp\trace.o \
ncftp\spool.o \
ncftp\log.o \
ncftp\getline.o

TARGET_OBJECTS = \
	$(OBJECTS_SIO) \
	$(OBJECTS_STRN) \
	$(OBJECTS_LIBNCFTP) \
	$(OBJECTS_NCFTP)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF