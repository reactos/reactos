# $Id: makefile,v 1.44 2001/02/01 16:44:19 jean Exp $
#
# ReactOS Operating System
#
PATH_TO_TOP = ../..

TARGET=crtdll
DLLTARGET=$(TARGET).dll


CFLAGS = 

all: $(DLLTARGET)


ASSERT_OBJECTS = assert/assert.o

CTYPE_OBJECTS = ctype/isalnum.o \
		ctype/isalpha.o ctype/isascii.o ctype/iscntrl.o ctype/isdigit.o ctype/isgraph.o \
		ctype/islower.o ctype/isprint.o ctype/ispunct.o ctype/isspace.o ctype/isupper.o \
		ctype/isxdigit.o ctype/toascii.o ctype/tolower.o ctype/toupper.o\
		ctype/iscsym.o ctype/isctype.o

CONIO_OBJECTS = conio/cputs.o conio/getch.o conio/getche.o conio/putch.o conio/ungetch.o\
		conio/kbhit.o conio/cgets.o conio/cprintf.o conio/cscanf.o

DIRECT_OBJECTS = direct/chdir.o direct/chdrive.o direct/getcwd.o direct/getdrive.o \
		 direct/rmdir.o direct/mkdir.o direct/getdfree.o direct/getdcwd.o

EXCEPT_OBJECTS = except/unwind.o except/abnorter.o except/exhand2.o except/matherr.o

LOCALE_OBJECTS = locale/locale.o 

MALLOC_OBJECTS = malloc/expand.o malloc/heap.o


MISC_OBJECTS = misc/GetArgs.o misc/dllmain.o misc/setnew.o misc/purecall.o misc/initterm.o\
		misc/amsg.o

MBSTRING_OBJECTS = mbstring/mbsnicmp.o mbstring/mbsnset.o mbstring/mbsnextc.o mbstring/mbsnicoll.o mbstring/islead.o mbstring/mbsspnp.o \
		mbstring/mbspbrk.o mbstring/mbsspn.o mbstring/mbbtype.o mbstring/mbscat.o mbstring/mbschr.o \
		mbstring/mbccpy.o mbstring/mbslen.o mbstring/mbsrchr.o mbstring/mbsset.o mbstring/mbsncat.o mbstring/mbsncmp.o \
		mbstring/mbscmp.o mbstring/mbsncoll.o mbstring/mbscoll.o mbstring/mbsncpy.o mbstring/mbscpy.o mbstring/mbscspn.o \
		mbstring/mbsdup.o mbstring/mbsicmp.o mbstring/mbsicoll.o mbstring/mbsnccnt.o mbstring/mbsrev.o mbstring/mbsstr.o \
		mbstring/mbsinc.o mbstring/mbsdec.o mbstring/mbsninc.o mbstring/mbclen.o mbstring/iskana.o mbstring/jmstojis.o \
		mbstring/jistojms.o mbstring/iskpun.o mbstring/iskmoji.o mbstring/ismbgra.o mbstring/ismbpri.o mbstring/isuppr.o \
		mbstring/islwr.o mbstring/ismbkaln.o mbstring/mbstrlen.o mbstring/ismbc.o \
		mbstring/ismbtrl.o mbstring/ismblead.o mbstring/ischira.o mbstring/hanzen.o mbstring/ismbaln.o mbstring/ismbal.o \
		mbstring/ismbpun.o mbstring/mbslwr.o mbstring/mbsupr.o mbstring/mbstok.o

STRING_OBJECTS = string/memchr.o string/memcmp.o string/strcat.o \
                 string/strchr.o string/strcmp.o string/strcoll.o \
		 string/strcpy.o string/strcspn.o string/memcpy.o \
		 string/strlen.o string/strncat.o string/strncmp.o \
		 string/strncpy.o string/strpbrk.o string/strrchr.o \
		 string/strspn.o string/strstr.o string/strtok.o \
		 string/strxfrm.o string/memmove.o string/memset.o \
		 string/strdup.o string/strlwr.o string/strupr.o \
		 string/str_old.o string/strerror.o string/stricmp.o\
		 string/strnlen.o string/strnicmp.o string/strrev.o\
		 string/memccpy.o string/memicmp.o string/strset.o

WCHAR_OBJECTS = wchar/wcscat.o wchar/wcschr.o wchar/wcscmp.o \
		 wchar/wcscoll.o wchar/wcscpy.o wchar/wcscspn.o \
		 wchar/wcsdup.o wchar/wcsicmp.o wchar/wcslen.o \
		 wchar/wcslwr.o wchar/wcsncat.o wchar/wcsncmp.o \
		 wchar/wcsncpy.o wchar/wcsnlen.o wchar/wcspbrk.o wchar/wcsrchr.o\
		 wchar/wcsrev.o wchar/wcsset.o wchar/wcsspn.o wchar/wcsstr.o\
		 wchar/wcstod.o wchar/wcstok.o wchar/wcstol.o wchar/wcsupr.o\
		 wchar/wcsxfrm.o wchar/wtoi.o wchar/wcstombs.o wchar/wcsnicmp.o 

SETJMP_OBJECTS = setjmp/setjmp.o

STDIO_OBJECTS = stdio/getenv.o stdio/filbuf.o  \
		stdio/fclose.o stdio/feof.o stdio/ferror.o stdio/fileno.o\
		stdio/fflush.o stdio/fgetc.o stdio/fgetpos.o stdio/fgets.o stdio/flsbuf.o \
		stdio/fopen.o stdio/fprintf.o stdio/fputc.o stdio/fputs.o stdio/setvbuf.o\
		stdio/fread.o stdio/freopen.o  stdio/fscanf.o stdio/fseek.o \
		stdio/fsetpos.o stdio/ftell.o stdio/fwalk.o stdio/fwrite.o stdio/getc.o \
		stdio/getchar.o stdio/gets.o stdio/getw.o stdio/perror.o stdio/clearerr.o \
		stdio/putc.o stdio/putchar.o stdio/puts.o stdio/putw.o stdio/fputchar.o\
		stdio/remove.o stdio/rename.o stdio/rewind.o stdio/allocfil.o\
		stdio/setbuf.o stdio/setbuffe.o stdlib/obsol.o stdio/setlineb.o\
	 	stdio/scanf.o  stdio/sscanf.o stdio/vscanf.o stdio/vsscanf.o stdio/vfscanf.o\
		stdio/stdiohk.o stdio/stdhnd.o stdio/tempnam.o stdio/tmpfile.o stdio/tmpnam.o \
		stdio/ungetc.o stdio/printf.o stdio/vfprintf.o stdio/vprintf.o stdio/sprintf.o\
		stdio/fdopen.o stdio/vsprintf.o stdio/frlist.o stdio/fgetchar.o stdio/rmtmp.o\
		stdio/fsopen.o stdio/popen.o stdio/vfwprint.o
 
QUAD_OBJECTS =  quad/qdivrem.o quad/divdi3.o quad/moddi3.o quad/udivdi3.o quad/umoddi3.o

IO_OBJECTS = io/access.o io/close.o io/create.o io/dup.o io/dup2.o io/find.o io/isatty.o io/lseek.o \
		io/open.o io/read.o io/setmode.o io/unlink.o io/write.o io/fmode.o io/mktemp.o\
		io/chmod.o io/chsize.o io/commit.o io/locking.o io/pipe.o io/sopen.o io/filelen.o\
		io/umask.o io/tell.o io/eof.o io/utime.o 

SEARCH_OBJECTS = search/lsearch.o search/lfind.o

STDLIB_OBJECTS = stdlib/abort.o stdlib/abs.o stdlib/atexit.o stdlib/atof.o stdlib/atoi.o \
		 stdlib/bsearch.o stdlib/div.o stdlib/errno.o  stdlib/_exit.o \
		 stdlib/fullpath.o stdlib/labs.o stdlib/ldiv.o stdlib/itoa.o\
		 stdlib/makepath.o stdlib/malloc.o stdlib/putenv.o stdlib/qsort.o \
		 stdlib/rand.o stdlib/senv.o stdlib/splitp.o stdlib/strtod.o stdlib/strtol.o \
		 stdlib/strtoul.o stdlib/swab.o stdlib/atol.o stdlib/rot.o stdlib/wcstomb.o\
		 stdlib/ecvt.o stdlib/ecvtbuf.o stdlib/gcvt.o  stdlib/fcvt.o stdlib/fcvtbuf.o\
		 stdlib/mbstowcs.o stdlib/itow.o

SIGNAL_OBJECTS = signal/signal.o signal/xcptfil.o signal/xcptinfo.o

PROCESS_OBJECTS = process/_cwait.o process/dll.o process/spawnl.o process/spawnlp.o process/spawnlpe.o  process/spawnvp.o \
		process/spawnv.o process/spawnve.o process/spawnle.o process/execl.o process/execlp.o process/execlpe.o  \
		process/execvpe.o process/execvp.o process/execv.o process/execle.o process/_system.o\
		process/execve.o process/threadid.o process/thread.o process/procid.o

TCHAR_OBJECTS = tchar/strdec.o tchar/strinc.o tchar/strninc.o  tchar/strncnt.o tchar/strnextc.o tchar/strspnp.o

TIME_OBJECTS = time/ctime.o time/difftime.o time/strftime.o time/time.o time/clock.o time/strdate.o\
		time/strtime.o

FLOAT_OBJECTS = float/fpreset.o float/clearfp.o float/cntrlfp.o float/statfp.o float/logb.o\
		float/chgsign.o float/fpclass.o float/isnan.o float/nafter.o float/scalb.o\
		float/copysign.o

SYS_STAT_OBJECTS = sys_stat/fstat.o sys_stat/stat.o sys_stat/futime.o sys_stat/ftime.o\
		sys_stat/systime.o



MATH_OBJECTS = math/acos.o math/acosh.o math/asin.o math/asinh.o math/atan.o math/atan2.o\
		math/atanh.o math/ceil.o math/cos.o math/cosh.o math/exp.o math/fabs.o\
		math/floor.o math/fmod.o math/frexp.o math/huge_val.o math/hypot.o\
		math/ldexp.o math/log.o math/log10.o math/modf.o math/pow.o\
		math/sin.o math/sinh.o math/sqrt.o math/tan.o\
		math/tanh.o math/stubs.o math/j0_y0.o math/j1_y1.o math/jn_yn.o\
		math/cabs.o math/ftol.o

RESOURCE_OBJECT = $(TARGET).coff

OBJECTS = $(MISC_OBJECTS) $(STDLIB_OBJECTS) $(IO_OBJECTS) \
	  $(FLOAT_OBJECTS) $(ASSERT_OBJECTS) $(PROCESS_OBJECTS) \
	  $(STDIO_OBJECTS) $(CTYPE_OBJECTS) $(MATH_OBJECTS) \
	  $(STRING_OBJECTS) $(TIME_OBJECTS) $(WCHAR_OBJECTS) \
	  $(SYS_STAT_OBJECTS) $(MALLOC_OBJECTS) $(MBSTRING_OBJECTS)\
	  $(SEARCH_OBJECTS) $(CONIO_OBJECTS) $(DIRECT_OBJECTS) \
	  $(SIGNAL_OBJECTS) $(SETJMP_OBJECTS) $(RESOURCE_OBJECT) \
	  $(LOCALE_OBJECTS) $(EXCEPT_OBJECTS) $(TCHAR_OBJECTS)

ifeq ($(DOSCLI), yes)
CLEAN_FILES = assert\*.o conio\*.o ctype\*.o direct\*.o dirent\*.o \
	except\*.o float\*.o io\*.o libc\*.o locale\*.o malloc\*.o \
	math\*.o mbstring\*.o misc\*.o process\*.o quad\*.o search\*.o \
	setjmp\*.o signal\*.o stdio\*.o stdlib\*.o string\*.o sys_stat\*.o \
	tchar\*.o time\*.o wchar\*.o \
	$(TARGET).a $(TARGET).o $(TARGET).dll $(TARGET).sym \
	$(TARGET).coff
else
CLEAN_FILES = assert/*.o conio/*.o ctype/*.o direct/*.o dirent/*.o \
	except/*.o float/*.o io/*.o libc/*.o locale/*.o malloc/*.o \
	math/*.o mbstring/*.o misc/*.o process/*.o quad/*.o search/*.o \
	setjmp/*.o stdio/*.o stdlib/*.o string/*.o sys_stat/*.o tchar/*.o \
	time/*.o wchar/*.o $(TARGET).a $(TARGET).o $(TARGET).dll $(TARGET).sym \
	$(TARGET).coff $(SIGNAL_OBJECTS)
endif

$(TARGET).coff: $(TARGET).rc ../../include/reactos/resource.h

$(TARGET).a: $(OBJECTS)
	$(LD)  -r $(OBJECTS) -o $(TARGET).a

$(TARGET).dll: $(DLLMAIN) $(OBJECTS) $(TARGET).def
	$(LD) -r $(OBJECTS) -o $(TARGET).o
	$(DLLTOOL) \
		--dllname $(TARGET).dll \
		--def $(TARGET).def \
		--output-lib $(TARGET).a
	$(CC) \
		-nostartfiles -nostdlib \
		-mdll \
		-o junk.tmp \
		-Wl,--base-file,base.tmp \
		-Wl,--entry=_DllMain@12 \
		$(TARGET).o \
		../kernel32/kernel32.a -lgcc
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGET).dll \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def $(TARGET).def
	- $(RM) base.tmp
	$(CC) \
		-nostartfiles -nostdlib \
		-mdll \
		-o $(TARGET).dll \
		$(TARGET).o  \
		../kernel32/kernel32.a -lgcc \
		-Wl,--image-base,0x77630000 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,temp.exp \
		-Wl,--entry=_DllMain@12
	- $(RM) temp.exp
	$(NM) --numeric-sort $(TARGET).dll > $(TARGET).sym

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

install: $(FLOPPY_DIR)/dlls/$(TARGET).dll

$(FLOPPY_DIR)/dlls/$(TARGET).dll: $(TARGET).dll
	$(CP) $(TARGET).dll $(FLOPPY_DIR)/dlls/$(TARGET).dll

dist: $(DIST_DIR)/dlls/$(TARGET).dll

$(DIST_DIR)/dlls/$(TARGET).dll: $(TARGET).dll
	$(CP) $(TARGET).dll ../../$(DIST_DIR)/dlls/$(TARGET).dll

include ../../rules.mak

