# $Id: Makefile,v 1.5 2002/03/10 17:10:30 hyperion Exp $

PATH_TO_TOP = ../../../..

TARGET_TYPE = dynlink

TARGET_NAME = psxdll

TARGET_LIBPATH = $(PATH_TO_TOP)/dk/psx/lib

TARGET_LFLAGS = -nostartfiles

TARGET_SDKLIBS = ntdll.a

TARGET_BASE = 0x68EB0000

TARGET_ENTRY = _DllMain@12

TARGET_CFLAGS =\
	-W \
	-nostdinc \
	-nostdlib \
	-fno-builtin \
	-I$(PATH_TO_TOP)/subsys/psx/include \
	-I$(PATH_TO_TOP)/include \
	-D__PSXDLL__ \
	-D__PSX_DEBUG_WANT_ALL__

TARGET_OBJECTS = $(TARGET_NAME).o

TARGET_CLEAN = $(OBJECTS)

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
	misc/fdtable.o

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

OBJECTS_SYS_UTSNAME = \
	sys/utsname/uname.o
 
OBJECTS_UNISTD = \
 unistd/close.o \
 unistd/dup.o \
 unistd/getcwd.o \
 unistd/getpid.o \
 unistd/getppid.o

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
	$(OBJECTS_SYS_UTSNAME) \
 $(OBJECTS_UNISTD)

DEP_OBJECTS = $(OBJECTS)

include $(TOOLS_PATH)/depend.mk

$(TARGET_NAME).o: $(OBJECTS)
	$(LD) -r $(OBJECTS) -o $(TARGET_NAME).o 

DTFLAGS = -k -l $@

$(TARGET_NAME).a: $(TARGET_NAME).def
	$(DLLTOOL) \
		$(DTFLAGS) \
		-D $(TARGET_NAME).dll \
		-d $(TARGET_NAME).def

# EOF
