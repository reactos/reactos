# $Id: makefile,v 1.67 2003/06/08 21:00:04 ekohl Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = kernel32

TARGET_BASE = 0x77f00000

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = rosrtl.a ntdll.a

TARGET_GCCLIBS = gcc

TARGET_PCH = k32.h

TARGET_CLEAN = errcodes.rc msg?????.bin

SYNCH_OBJECTS = synch/critical.o synch/event.o synch/intrlck.o synch/mutex.o \
                synch/sem.o synch/timer.o synch/wait.o

MISC_OBJECTS = misc/error.o misc/atom.o misc/handle.o misc/env.o \
               misc/dllmain.o misc/comm.o misc/errormsg.o \
               misc/console.o misc/time.o misc/toolhelp.o \
               misc/stubs.o misc/ldr.o misc/res.o \
               misc/sysinfo.o misc/profile.o \
               misc/mbchars.o misc/muldiv.o misc/computername.o \
               misc/perfcnt.o

FILE_OBJECTS = file/file.o file/curdir.o file/lfile.o file/dir.o \
               file/iocompl.o file/volume.o file/deviceio.o file/dosdev.o \
               file/create.o file/find.o file/copy.o file/pipe.o \
               file/move.o file/lock.o file/rw.o file/delete.o \
               file/npipe.o file/tape.o file/mailslot.o file/backup.o \
               file/cnotify.o

MEM_OBJECTS = mem/global.o mem/heap.o mem/isbad.o mem/local.o \
              mem/procmem.o mem/section.o mem/virtual.o

NLS_OBJECTS =

THREAD_OBJECTS = \
	thread/fiber.o	\
	thread/thread.o	\
	thread/tls.o \
	thread/fls.o

THREAD_I386_OBJECTS = \
	thread/i386/fiber.o

PROCESS_OBJECTS = \
	process/proc.o \
	process/cmdline.o \
	process/create.o \
	process/session.o

STRING_OBJECTS = string/lstring.o

EXCEPT_OBJECTS = except/except.o

DEBUG_OBJECTS = debug/debugger.o debug/output.o debug/break.o

ARCH_OBJECTS = $(THREAD_I386_OBJECTS)

TARGET_OBJECTS = $(MISC_OBJECTS) $(FILE_OBJECTS) $(THREAD_OBJECTS) \
          	$(PROCESS_OBJECTS) $(STRING_OBJECTS) $(MEM_OBJECTS) \
          	$(SYNCH_OBJECTS) $(EXCEPT_OBJECTS) $(DEBUG_OBJECTS) \
		$(ARCH_OBJECTS)


DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

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
