# $Id: Makefile,v 1.10 2002/08/23 12:40:56 ea Exp $

PATH_TO_TOP = ../../../reactos

PATH_TO_PSX_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = psxdll

TARGET_LIBPATH = $(PATH_TO_PSX_TOP)/dk/lib

TARGET_LFLAGS = -nostartfiles

TARGET_SDKLIBS = ntdll.a

TARGET_BASE = 0x68EB0000

TARGET_ENTRY = _DllMain@12

TARGET_CFLAGS =\
	-Wall \
	-nostdinc \
	-nostdlib \
	-fno-builtin \
	-I$(PATH_TO_PSX_TOP)/include \
	-I$(PATH_TO_TOP)/include \
	-D__PSXDLL__ \
	-D__PSX_DEBUG_WANT_ALL__

TARGET_OBJECTS = $(TARGET_NAME).o

TARGET_CLEAN = $(OBJECTS) csrapi.a

include $(PATH_TO_TOP)/rules.mak
include $(TOOLS_PATH)/helper.mk

OBJECTS_MISC = \
 misc/main.o \
 misc/init.o \
 misc/heap.o \
 misc/interlock.o \
 misc/safeobj.o \
 misc/spawn.o \
 misc/path.o \
 misc/pdata.o \
 misc/fdtable.o \
 misc/tls.o

OBJECTS_DIRENT = \
 dirent/opendir.o \
 dirent/readdir.o \
 dirent/closedir.o

OBJECTS_DLFCN = \
 dlfcn/dlopen.o \
 dlfcn/dlclose.o \
 dlfcn/dlsym.o \
 dlfcn/dlerror.o

OBJECTS_ERRNO = \
 errno/errno.o

OBJECTS_FCNTL = \
 fcntl/open.o \
 fcntl/fcntl.o

OBJECTS_LIBGEN = \
 libgen/basename.o

OBJECTS_SCHED = \
 sched/yield.o

OBJECTS_SIGNAL = \
 pthread/kill.o \
 signal/raise.o

OBJECTS_STDLIB = \
 stdlib/abort.o \
 stdlib/malloc.o \
 stdlib/exit.o

OBJECTS_STRING = \
 string/strdup.o \
 string/strerror.o

OBJECTS_PTHREAD = \
 pthread/create.o \
 pthread/exit.o \
 pthread/join.o \
 pthread/mutex.o \
 pthread/self.o

OBJECTS_SYS_STAT = \
 sys/stat/chmod.o \
 sys/stat/mkdir.o \
 sys/stat/mkfifo.o \
 sys/stat/mknod.o \
 sys/stat/stat.o \
 sys/stat/umask.o

OBJECTS_SYS_UTSNAME = \
 sys/utsname/uname.o
 
OBJECTS_UNISTD = \
 unistd/access.o \
 unistd/close.o \
 unistd/dup.o \
 unistd/fork.o \
 unistd/getcwd.o \
 unistd/getpid.o \
 unistd/getppid.o \
 unistd/read.o \
 unistd/sleep.o \
 unistd/write.o

OBJECTS = \
 $(OBJECTS_MISC) \
 $(OBJECTS_DIRENT) \
 $(OBJECTS_DLFCN) \
 $(OBJECTS_ERRNO) \
 $(OBJECTS_FCNTL) \
 $(OBJECTS_LIBGEN) \
 $(OBJECTS_SCHED) \
 $(OBJECTS_SIGNAL) \
 $(OBJECTS_STDLIB) \
 $(OBJECTS_STRING) \
 $(OBJECTS_PTHREAD) \
 $(OBJECTS_SYS_STAT) \
 $(OBJECTS_SYS_UTSNAME) \
 $(OBJECTS_UNISTD)

DEP_OBJECTS = $(OBJECTS)

include $(TOOLS_PATH)/depend.mk

DTFLAGS = -k -l $@

$(TARGET_NAME).a: $(TARGET_NAME).def
	$(DLLTOOL) \
		$(DTFLAGS) \
		-D $(TARGET_NAME).dll \
		-d $(TARGET_NAME).def

csrapi.a: csrapi.def
	$(DLLTOOL) \
		$(DTFLAGS) \
		-D ntdll.dll \
		-d csrapi.def

$(TARGET_NAME).o: csrapi.a $(OBJECTS)
	$(LD) -r $(OBJECTS) -o $(TARGET_NAME).o 


# EOF
