all: kernel32.a

SYNCH_OBJECTS = synch/critical.o

#MISC_OBJECTS = misc/atom.o
MISC_OBJECTS = misc/error.o

#FILE_OBJECTS = file/file.o file/lfile.o
FILE_OBJECTS = file/file.o file/curdir.o

MEM_OBJECTS = mem/virtual.o mem/heap.o mem/utils.o

#THREAD_OBJECTS = thread/thread.o

PROCESS_OBJECTS = process/proc.o

STRING_OBJECTS = string/lstring.o

INTERNAL_OBJECTS = internal/dprintf.o internal/vsprintf.o

OBJECTS = $(MISC_OBJECTS) $(FILE_OBJECTS) $(THREAD_OBJECTS) \
          $(PROCESS_OBJECTS) $(STRING_OBJECTS) $(MEM_OBJECTS) \
	  $(INTERNAL_OBJECTS) $(SYNCH_OBJECTS)
  

kernel32.a: $(OBJECTS)
	$(AR) vrcs kernel32.a $(OBJECTS) 

dummy:

include ../../rules.mak
