all: kernel32.a

SYNCH_OBJECTS = synch/critical.o synch/event.o synch/wait.o

MISC_OBJECTS = misc/error.o misc/atom.o misc/handle.o misc/env.o misc/dllmain.o

FILE_OBJECTS = file/file.o file/curdir.o file/lfile.o file/dir.o \
               file/iocompl.o file/volume.o file/deviceio.o file/dosdev.o \
	       file/create.o

MEM_OBJECTS = mem/virtual.o mem/heap.o mem/utils.o

THREAD_OBJECTS = thread/thread.o

PROCESS_OBJECTS = process/proc.o

STRING_OBJECTS = string/lstring.o

INTERNAL_OBJECTS = internal/dprintf.o

EXCEPT_OBJECTS = except/except.o




OBJECTS = $(MISC_OBJECTS) $(FILE_OBJECTS) $(THREAD_OBJECTS) \
          $(PROCESS_OBJECTS) $(STRING_OBJECTS) $(MEM_OBJECTS) \
	  $(INTERNAL_OBJECTS) $(SYNCH_OBJECTS) $(EXCEPT_OBJECTS)
  

kernel32.a: $(OBJECTS)
	$(AR) vrcs kernel32.a $(OBJECTS) 

dummy:

include ../../rules.mak
