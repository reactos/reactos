# $Id: makefile,v 1.60 2003/02/16 18:47:41 hbirr Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = kernel32

TARGET_BASE = 0x77f00000

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = ntdll.a kernel32.a

TARGET_GCCLIBS = gcc

TARGET_PCH = k32.h

TARGET_CLEAN = errcodes.rc msg?????.bin

SYNCH_OBJECTS = synch/critical.o synch/event.o synch/intrlck.o synch/mutex.o \
                synch/sem.o synch/timer.o synch/wait.o

MISC_OBJECTS = misc/error.o misc/atom.o misc/handle.o misc/env.o \
	       misc/dllmain.o misc/comm.o misc/errormsg.o \
	       misc/console.o misc/time.o misc/toolhelp.o \
	       misc/stubs.o misc/ldr.o misc/res.o \
	       misc/debug.o misc/sysinfo.o misc/profile.o \
	       misc/mbchars.o misc/muldiv.o misc/getname.o \
	       misc/perfcnt.o

FILE_OBJECTS = file/file.o file/curdir.o file/lfile.o file/dir.o \
               file/iocompl.o file/volume.o file/deviceio.o file/dosdev.o \
	       file/create.o file/find.o file/copy.o file/pipe.o \
	       file/move.o file/lock.o file/rw.o file/delete.o \
	       file/npipe.o file/tape.o file/mailslot.o file/backup.o \
	       file/cnotify.o

MEM_OBJECTS = mem/global.o mem/heap.o mem/isbad.o mem/local.o \
              mem/procmem.o mem/section.o mem/virtual.o

NLS_OBJECTS =  nls/codepage.o nls/cpmisc.o \
 nls/cptable.o nls/lctable.o \
 nls/lcAFK.o nls/lcBEL.o nls/lcBGR.o nls/lcCAT.o \
 nls/lcCSY.o nls/lcDAN.o nls/lcDEA.o nls/lcDEC.o \
 nls/lcDEL.o nls/lcDES.o nls/lcDEU.o nls/lcELL.o \
 nls/lcENA.o nls/lcENB.o nls/lcENC.o nls/lcENG.o \
 nls/lcENI.o nls/lcENJ.o nls/lcENL.o nls/lcENS.o \
 nls/lcENT.o nls/lcENU.o nls/lcENZ.o nls/lcESA.o \
 nls/lcESB.o nls/lcESC.o nls/lcESD.o nls/lcESE.o \
 nls/lcESF.o nls/lcESG.o nls/lcESH.o nls/lcESI.o \
 nls/lcESL.o nls/lcESM.o nls/lcESN.o nls/lcESO.o \
 nls/lcESP.o nls/lcESR.o nls/lcESS.o nls/lcESU.o \
 nls/lcESV.o nls/lcESY.o nls/lcESZ.o nls/lcETI.o \
 nls/lcEUQ.o nls/lcFIN.o nls/lcFOS.o nls/lcFRA.o \
 nls/lcFRB.o nls/lcFRC.o nls/lcFRL.o nls/lcFRS.o \
 nls/lcHRV.o nls/lcHUN.o nls/lcIND.o nls/lcISL.o \
 nls/lcITA.o nls/lcITS.o nls/lcLTH.o nls/lcLVI.o \
 nls/lcNLB.o nls/lcNLD.o nls/lcNON.o nls/lcNOR.o \
 nls/lcPLK.o nls/lcPTB.o nls/lcPTG.o nls/lcROM.o \
 nls/lcRUS.o nls/lcSKY.o nls/lcSLV.o nls/lcSQI.o \
 nls/lcSRB.o nls/lcSRL.o nls/lcSVE.o nls/lcSVF.o \
 nls/lcTRK.o nls/lcUKR.o\
 nls/locale.o 

NLS_UNUSED_OBJECTS =  nls/mbtowc.o nls/wctomb.o nls/ole2nls.o

NLS_CP_OBJECTS =  \
 nls/cp37.o nls/cp437.o nls/cp500.o nls/cp737.o nls/cp775.o \
 nls/cp850.o nls/cp852.o nls/cp855.o nls/cp857.o \
 nls/cp860.o nls/cp861.o nls/cp863.o nls/cp865.o \
 nls/cp866.o nls/cp869.o nls/cp875.o nls/cp1026.o \
 nls/cp1250.o nls/cp1251.o nls/cp1252.o nls/cp1253.o \
 nls/cp1254.o nls/cp1255.o nls/cp1256.o nls/cp1257.o \
 nls/cp10000.o nls/cp10006.o nls/cp10007.o \
 nls/cp10029.o nls/cp10079.o nls/cp10081.o

THREAD_OBJECTS = \
	thread/fiber.o	\
	thread/thread.o	\
	thread/tls.o

PROCESS_OBJECTS = \
	process/proc.o \
	process/cmdline.o \
	process/create.o \
	process/session.o

STRING_OBJECTS = string/lstring.o

EXCEPT_OBJECTS = except/except.o

TARGET_OBJECTS = $(MISC_OBJECTS) $(FILE_OBJECTS) $(THREAD_OBJECTS) \
          	$(PROCESS_OBJECTS) $(STRING_OBJECTS) $(MEM_OBJECTS) \
          	$(SYNCH_OBJECTS) $(EXCEPT_OBJECTS)

#$(NLS_OBJECTS)


include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

$(TARGET_NAME).coff: errcodes.rc

errcodes.rc: $(TARGET_NAME).mc
	$(MC) \
		-H $(PATH_TO_TOP)/include/reactos/errcodes.h \
		-o errcodes.rc \
		$(TARGET_NAME).mc

%/TAGS:
	etags -o $(@D)/TAGS $(@D)/\*.c

etags: except/TAGS file/TAGS mem/TAGS misc/TAGS nls/TAGS process/TAGS string/TAGS synch/TAGS thread/TAGS
	etags -i except/TAGS -i file/TAGS -i mem/TAGS -i misc/TAGS -i nls/TAGS -i process/TAGS -i string/TAGS -i synch/TAGS -i thread/TAGS
