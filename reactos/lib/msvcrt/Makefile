# $Id: Makefile,v 1.7 2000/12/28 11:43:07 jean Exp $
#
# ReactOS Operating System
#
PATH_TO_TOP = ../..

TARGET_NAME=msvcrt
TARGET_DLL=$(TARGET_NAME).dll

BASE_CFLAGS = -I../../include

all: $(TARGET_DLL)


OBJECTS_CTYPE = \
	ctype/isalnum.o \
	ctype/isalpha.o \
	ctype/isascii.o \
	ctype/iscntrl.o \
	ctype/isdigit.o \
	ctype/isgraph.o \
	ctype/islower.o \
	ctype/isprint.o \
	ctype/ispunct.o \
	ctype/isspace.o \
	ctype/isupper.o \
	ctype/isxdigit.o \
	ctype/toascii.o \
	ctype/tolower.o \
	ctype/toupper.o \
	ctype/iscsym.o \
	ctype/isctype.o

OBJECTS_DIRECT = \
	direct/chdir.o \
	direct/chdrive.o \
	direct/getcwd.o \
	direct/getdcwd.o \
	direct/getdfree.o \
	direct/getdrive.o \
	direct/mkdir.o \
	direct/rmdir.o

OBJECTS_FLOAT = \
	float/isnan.o \

OBJECTS_IO = \
	io/close.o \
	io/fmode.o \
	io/isatty.o \
	io/lseek.o \
	io/open.o \
	io/read.o \
	io/setmode.o \
	io/write.o

OBJECTS_MATH = \
	math/modf.o \
	math/pow.o \

OBJECTS_MISC = \
	misc/dllmain.o \
	misc/tls.o

OBJECTS_PROCESS = \
	process/thread.o
	
OBJECTS_SIGNAL = \
	signal/signal.o \

OBJECTS_STDIO = \
	stdio/allocfil.o \
	stdio/fclose.o \
	stdio/feof.o \
	stdio/ferror.o \
	stdio/fflush.o \
	stdio/fgets.o \
	stdio/filbuf.o \
	stdio/flsbuf.o \
	stdio/fopen.o \
	stdio/fprintf.o \
	stdio/fputc.o \
	stdio/fputs.o \
	stdio/fread.o \
	stdio/fwalk.o \
	stdio/fwrite.o \
	stdio/getc.o \
	stdio/printf.o \
	stdio/putc.o \
	stdio/putchar.o \
	stdio/puts.o \
	stdio/remove.o \
	stdio/setvbuf.o \
	stdio/sprintf.o \
	stdio/stdhnd.o \
	stdio/vfprintf.o \
	stdio/vfwprint.o \
	stdio/vprintf.o \
	stdio/vsprintf.o \

#incomplete
OBJECTS_STDLIB = \
	stdlib/_exit.o \
	stdlib/abort.o \
	stdlib/abs.o \
	stdlib/atexit.o \
	stdlib/atof.o \
	stdlib/atoi.o \
	stdlib/atol.o \
	stdlib/bsearch.o \
	stdlib/div.o \
	stdlib/errno.o \
	stdlib/fullpath.o \
	stdlib/getenv.o \
	stdlib/itoa.o \
	stdlib/itow.o \
	stdlib/labs.o \
	stdlib/ldiv.o \
	stdlib/makepath.o \
	stdlib/malloc.o \
	stdlib/putenv.o \
	stdlib/qsort.o \
	stdlib/rand.o \
	stdlib/rot.o \
	stdlib/senv.o \
	stdlib/splitp.o \
	stdlib/strtod.o \
	stdlib/strtol.o \
	stdlib/strtoul.o \
	stdlib/wcstod.o \
	stdlib/wcstol.o \
	stdlib/wcstoul.o \
	stdlib/wtoi.o

OBJECTS_STRING = \
	string/memccpy.o \
	string/memchr.o \
	string/memcmp.o \
	string/memcpy.o \
	string/memicmp.o \
	string/memmove.o \
	string/memset.o \
	string/strcat.o \
	string/strchr.o \
	string/strcmp.o \
	string/strcoll.o \
	string/strcpy.o \
	string/strcspn.o \
	string/strdup.o \
	string/strerror.o \
	string/stricmp.o \
	string/strlen.o \
	string/strlwr.o \
	string/strncat.o \
	string/strncmp.o \
	string/strncpy.o \
	string/strnicmp.o \
	string/strnlen.o \
	string/strpbrk.o \
	string/strrchr.o \
	string/strrev.o\
	string/strset.o \
	string/strspn.o \
	string/strstr.o \
	string/strtok.o \
	string/strupr.o \
	string/strxfrm.o 

OBJECTS_SYS_STAT = \
	sys_stat/fstat.o

OBJECTS_TIME = \
	time/time.o \
	time/ctime.o

OBJECTS_WSTRING = \
	wstring/wcscat.o \
	wstring/wcschr.o \
	wstring/wcscmp.o \
	wstring/wcscoll.o \
	wstring/wcscpy.o \
	wstring/wcscspn.o \
	wstring/wcsdup.o \
	wstring/wcsicmp.o \
	wstring/wcslen.o \
	wstring/wcslwr.o \
	wstring/wcsncat.o \
	wstring/wcsncmp.o \
	wstring/wcsncpy.o \
	wstring/wcsnicmp.o \
	wstring/wcsnlen.o \
	wstring/wcspbrk.o \
	wstring/wcsrchr.o \
	wstring/wcsrev.o \
	wstring/wcsset.o \
	wstring/wcsspn.o \
	wstring/wcsstr.o \
	wstring/wcstok.o \
	wstring/wcsupr.o \
	wstring/wcsxfrm.o

OBJECTS = \
	$(OBJECTS_CTYPE) \
	$(OBJECTS_DIRECT) \
	$(OBJECTS_FLOAT) \
	$(OBJECTS_IO) \
	$(OBJECTS_MATH) \
	$(OBJECTS_MISC) \
	$(OBJECTS_PROCESS) \
	$(OBJECTS_SIGNAL) \
	$(OBJECTS_STDIO) \
	$(OBJECTS_STDLIB) \
	$(OBJECTS_STRING) \
	$(OBJECTS_SYS_STAT) \
	$(OBJECTS_TIME) \
	$(OBJECTS_WSTRING) \
	msvcrt.coff

ifeq ($(DOSCLI), yes)
CLEAN_FILES = \
	ctype\*.o \
	direct\*.o \
	misc\*.o \
	process\*.o \
	stdlib\*.o \
	string\*.o \
	wstring\*.o \
	$(TARGET_NAME).a \
	$(TARGET_NAME).coff \
	$(TARGET_NAME).o \
	$(TARGET_NAME).dll \
	$(TARGET_NAME).sym \
	base.tmp
else
CLEAN_FILES = \
	ctype/*.o \
	direct/*.o \
	misc/*.o \
	process/*.o \
	stdlib/*.o \
	string/*.o \
	wstring/*.o \
	$(TARGET_NAME).a \
	$(TARGET_NAME).coff \
	$(TARGET_NAME).o \
	$(TARGET_NAME).dll \
	$(TARGET_NAME).sym
endif

$(TARGET_NAME).coff: $(TARGET_NAME).rc ../../include/reactos/resource.h

$(TARGET_NAME).a: $(OBJECTS)
	$(LD)  -r $(OBJECTS) -o $(TARGET_NAME).a

$(TARGET_NAME).dll: $(DLLMAIN) $(OBJECTS) $(TARGET_NAME).def
	$(LD) -r $(OBJECTS) -o $(TARGET_NAME).o
	$(DLLTOOL) \
		--dllname $(TARGET_NAME).dll \
		--def $(TARGET_NAME).def \
		--output-lib $(TARGET_NAME).a
	$(CC) \
		-specs=misc/$(TARGET_NAME)_specs \
		-mdll \
		-o junk.tmp \
		-Wl,--base-file,base.tmp \
		$(TARGET_NAME).o \
		../kernel32/kernel32.a 
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGET_NAME).dll \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def $(TARGET_NAME).def
	- $(RM) base.tmp
	$(CC) \
		-specs=misc/$(TARGET_NAME)_specs \
		-mdll \
		-o $(TARGET_NAME).dll \
		$(TARGET_NAME).o  \
		../kernel32/kernel32.a \
		-Wl,--image-base,0x78000000 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,temp.exp
	- $(RM) temp.exp
	$(NM) --numeric-sort $(TARGET_NAME).dll > $(TARGET_NAME).sym

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

install: $(FLOPPY_DIR)/dlls/$(TARGET_NAME).dll

$(FLOPPY_DIR)/dlls/$(TARGET_NAME).dll: $(TARGET_NAME).dll
	$(CP) $(TARGET_NAME).dll $(FLOPPY_DIR)/dlls/$(TARGET_NAME).dll

dist: $(DIST_DIR)/dlls/$(TARGET_NAME).dll

$(DIST_DIR)/dlls/$(TARGET_NAME).dll: $(TARGET_NAME).dll
	$(CP) $(TARGET_NAME).dll ../../$(DIST_DIR)/dlls/$(TARGET_NAME).dll

include ../../rules.mak

