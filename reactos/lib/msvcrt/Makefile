# $Id: Makefile,v 1.17 2001/07/12 16:31:03 ekohl Exp $
#
# ReactOS Operating System
#
PATH_TO_TOP = ../..

TARGET_NAME=msvcrt
TARGET_DLL=$(TARGET_NAME).dll

CFLAGS = -I../../include -D__MSVCRT__

all: $(TARGET_DLL)


OBJECTS_CONIO = \
	conio/cgets.o \
	conio/cprintf.o \
	conio/cputs.o \
	conio/cscanf.o \
	conio/getch.o \
	conio/getche.o \
	conio/kbhit.o \
	conio/putch.o \
	conio/ungetch.o

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

OBJECTS_EXCEPT = \
	except/abnorter.o \
	except/exhand2.o \
	except/exhand3.o \
	except/matherr.o \
	except/unwind.o \
	except/xcptfil.o

OBJECTS_FLOAT = \
	float/chgsign.o \
	float/clearfp.o \
	float/cntrlfp.o \
	float/copysign.o \
	float/fpclass.o \
	float/fpecode.o \
	float/fpreset.o \
	float/isnan.o \
	float/logb.o \
	float/nafter.o \
	float/scalb.o \
	float/statfp.o

OBJECTS_IO = \
	io/access.o \
	io/chmod.o \
	io/chsize.o \
	io/close.o \
	io/commit.o \
	io/create.o \
	io/dup.o \
	io/dup2.o \
	io/eof.o \
	io/filelen.o \
	io/find.o \
	io/fmode.o \
	io/isatty.o \
	io/locking.o \
	io/lseek.o \
	io/mktemp.o \
	io/open.o \
	io/pipe.o \
	io/read.o \
	io/setmode.o \
	io/sopen.o \
	io/tell.o \
	io/umask.o \
	io/unlink.o \
	io/utime.o \
	io/write.o

OBJECTS_LOCALE = \
	locale/locale.o

OBJECTS_MATH = \
	math/acos.o \
	math/adjust.o \
	math/asin.o \
	math/atan.o \
	math/atan2.o \
	math/cabs.o \
	math/ceil.o \
	math/cos.o \
	math/cosh.o \
	math/exp.o \
	math/fabs.o \
	math/floor.o \
	math/fmod.o \
	math/frexp.o \
	math/ftol.o \
	math/huge_val.o \
	math/hypot.o \
	math/j0_y0.o \
	math/j1_y1.o \
	math/jn_yn.o \
	math/ldexp.o \
	math/log.o \
	math/log10.o \
	math/modf.o \
	math/pow.o \
	math/sin.o \
	math/sinh.o \
	math/sqrt.o \
	math/stubs.o \
	math/tan.o \
	math/tanh.o

OBJECTS_MISC = \
	misc/amsg.o \
	misc/assert.o \
	misc/dllmain.o \
	misc/getargs.o \
	misc/initterm.o \
	misc/purecall.o \
	misc/tls.o

OBJECTS_PROCESS = \
	process/_cwait.o \
	process/_system.o \
	process/dll.o \
	process/procid.o \
	process/thread.o \
	process/threadid.o

OBJECTS_SEARCH = \
	search/lfind.o \
	search/lsearch.o

OBJECTS_SETJMP = \
	setjmp/setjmp.o
	
OBJECTS_SIGNAL = \
	signal/signal.o

OBJECTS_STDIO = \
	stdio/allocfil.o \
	stdio/clearerr.o \
	stdio/fclose.o \
	stdio/fdopen.o \
	stdio/feof.o \
	stdio/ferror.o \
	stdio/fflush.o \
	stdio/fgetc.o \
	stdio/fgetchar.o \
	stdio/fgetpos.o \
	stdio/fgets.o \
	stdio/filbuf.o \
	stdio/fileno.o \
	stdio/flsbuf.o \
	stdio/fopen.o \
	stdio/fprintf.o \
	stdio/fputc.o \
	stdio/fputchar.o \
	stdio/fputs.o \
	stdio/fread.o \
	stdio/freopen.o \
	stdio/fscanf.o \
	stdio/fseek.o \
	stdio/fsetpos.o \
	stdio/fsopen.o \
	stdio/ftell.o \
	stdio/fwalk.o \
	stdio/fwrite.o \
	stdio/getc.o \
	stdio/getchar.o \
	stdio/gets.o \
	stdio/getw.o \
	stdio/perror.o \
	stdio/popen.o \
	stdio/printf.o \
	stdio/putc.o \
	stdio/putchar.o \
	stdio/puts.o \
	stdio/putw.o \
	stdio/remove.o \
	stdio/rename.o \
	stdio/rewind.o \
	stdio/rmtmp.o \
	stdio/scanf.o \
	stdio/setbuf.o \
	stdio/setvbuf.o \
	stdio/sprintf.o \
	stdio/sscanf.o \
	stdio/stdhnd.o \
	stdio/tempnam.o \
	stdio/tmpfile.o \
	stdio/tmpnam.o \
	stdio/ungetc.o \
	stdio/vfprintf.o \
	stdio/vfscanf.o \
	stdio/vfwprint.o \
	stdio/vprintf.o \
	stdio/vscanf.o \
	stdio/vsprintf.o \
	stdio/vsscanf.o

OBJECTS_STDLIB = \
	stdlib/_exit.o \
	stdlib/abort.o \
	stdlib/abs.o \
	stdlib/atexit.o \
	stdlib/atof.o \
	stdlib/atoi.o \
	stdlib/atoi64.o \
	stdlib/atol.o \
	stdlib/bsearch.o \
	stdlib/div.o \
	stdlib/ecvt.o \
	stdlib/ecvtbuf.o \
	stdlib/errno.o \
	stdlib/fcvt.o \
	stdlib/fcvtbuf.o \
	stdlib/fullpath.o \
	stdlib/gcvt.o \
	stdlib/getenv.o \
	stdlib/itoa.o \
	stdlib/itow.o \
	stdlib/labs.o \
	stdlib/ldiv.o \
	stdlib/makepath.o \
	stdlib/malloc.o \
	stdlib/obsol.o \
	stdlib/putenv.o \
	stdlib/qsort.o \
	stdlib/rand.o \
	stdlib/rot.o \
	stdlib/senv.o \
	stdlib/splitp.o \
	stdlib/strtod.o \
	stdlib/strtol.o \
	stdlib/strtoul.o \
	stdlib/swab.o \
	stdlib/wcstod.o \
	stdlib/wcstol.o \
	stdlib/wcstoul.o \
	stdlib/wtoi.o \
	stdlib/wtoi64.o

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
	sys_stat/fstat.o \
	sys_stat/futime.o \
	sys_stat/stat.o

OBJECTS_TIME = \
	time/clock.o \
	time/ctime.o \
	time/difftime.o \
	time/strdate.o \
	time/strftime.o \
	time/strtime.o \
	time/time.o

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
	$(OBJECTS_CONIO) \
	$(OBJECTS_CTYPE) \
	$(OBJECTS_DIRECT) \
	$(OBJECTS_EXCEPT) \
	$(OBJECTS_FLOAT) \
	$(OBJECTS_IO) \
	$(OBJECTS_LOCALE) \
	$(OBJECTS_MATH) \
	$(OBJECTS_MISC) \
	$(OBJECTS_PROCESS) \
	$(OBJECTS_SEARCH) \
	$(OBJECTS_SETJMP) \
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
	conio\*.o \
	ctype\*.o \
	direct\*.o \
	except\*.o \
	float\*.o \
	io\*.o \
	locale\*.o \
	math\*.o \
	misc\*.o \
	process\*.o \
	search\*.o \
	setjmp\*.o \
	signal\*.o \
	stdio\*.o \
	stdlib\*.o \
	string\*.o \
	sys_stat\*.o \
	time\*.o \
	wstring\*.o \
	$(TARGET_NAME).a \
	$(TARGET_NAME).coff \
	$(TARGET_NAME).o \
	$(TARGET_NAME).dll \
	$(TARGET_NAME).sym \
	base.tmp
else
CLEAN_FILES = \
	conio/*.o \
	ctype/*.o \
	direct/*.o \
	except/*.o \
	float/*.o \
	io/*.o \
	locale/*.o \
	math/*.o \
	misc/*.o \
	process/*.o \
	search/*.o \
	setjmp/*.o \
	signal/*.o \
	stdio/*.o \
	stdlib/*.o \
	string/*.o \
	sys_stat/*.o \
	time/*.o \
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
		-nostartfiles -nostdlib \
		-mdll \
		-o junk.tmp \
		-Wl,--base-file,base.tmp \
		-Wl,--entry=_DllMain@12 \
		$(TARGET_NAME).o \
		../kernel32/kernel32.a -lgcc
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGET_NAME).dll \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def $(TARGET_NAME).def
	- $(RM) base.tmp
	$(CC) \
		-nostartfiles -nostdlib \
		-mdll \
		-o $(TARGET_NAME).dll \
		$(TARGET_NAME).o  \
		../kernel32/kernel32.a -lgcc \
		-Wl,--image-base,0x78000000 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,temp.exp \
		-Wl,--entry=_DllMain@12
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

