
ifneq ($(HOST),mingw32-windows)
  ifneq ($(HOST),mingw32-linux)
    DLLTARGET=kernel32.a
  else
    DLLTARGET=kernel32.dll
  endif
else
  DLLTARGET=kernel32.dll
endif

all: $(DLLTARGET)

SYNCH_OBJECTS = synch/critical.o synch/event.o synch/wait.o

MISC_OBJECTS = misc/error.o misc/atom.o misc/handle.o misc/env.o misc/dllmain.o \
		misc/console.o misc/time.o

FILE_OBJECTS = file/file.o file/curdir.o file/lfile.o file/dir.o \
               file/iocompl.o file/volume.o file/deviceio.o file/dosdev.o \
	       file/create.o file/find.o file/copy.o

MEM_OBJECTS = mem/virtual.o mem/heap.o mem/utils.o

NLS_OBJECTS = # nls/mbtowc.o nls/wctomb.o

THREAD_OBJECTS = thread/thread.o

PROCESS_OBJECTS = process/proc.o process/cmdline.o

STRING_OBJECTS = string/lstring.o

INTERNAL_OBJECTS = internal/dprintf.o internal/string.o

EXCEPT_OBJECTS = except/except.o


OBJECTS = $(MISC_OBJECTS) $(FILE_OBJECTS) $(THREAD_OBJECTS) \
          $(PROCESS_OBJECTS) $(STRING_OBJECTS) $(MEM_OBJECTS) $(NLS_OBJECTS) \
	  $(INTERNAL_OBJECTS) $(SYNCH_OBJECTS) $(EXCEPT_OBJECTS)

kernel32.a: $(OBJECTS)
	$(AR) csr kernel32.a $(OBJECTS)

kernel32.dll: $(DLLMAIN) $(OBJECTS) kernel32.def
	$(LD) -r $(OBJECTS) -o kernel32.o
	$(DLLTOOL) --dllname kernel32.dll --def kernel32.def \
	           --output-lib kernel32.a
	$(CC) -specs=k32_specs -mdll -o junk.tmp \
	      -Wl,--base-file,base.tmp kernel32.o  ../ntdll/ntdll.a
	- $(RM) junk.tmp
	$(DLLTOOL) --dllname kernel32.dll --base-file base.tmp \
	           --output-exp temp.exp --def kernel32.def
	- $(RM) base.tmp
	$(CC) -specs=k32_specs -mdll -o kernel32.dll kernel32.o  ../ntdll/ntdll.a\
	      -Wl,--image-base,0x70000000 \
	      -Wl,--file-alignment,0x1000 \
	      -Wl,--section-alignment,0x1000 \
	      -Wl,temp.exp
	- $(RM) temp.exp
	$(NM) --numeric-sort kernel32.dll > kernel32.sym

include ../../rules.mak
