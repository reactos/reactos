# $Id: Makefile,v 1.22 2002/09/07 15:12:29 chorns Exp $

PATH_TO_TOP = ../..

TARGET_DEFONLY = yes

TARGET_TYPE = dynlink

TARGET_NAME = msvcrt

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = kernel32.a

TARGET_GCCLIBS = gcc

TARGET_BASE = 0x77600000

TARGET_CFLAGS = -D__MSVCRT__ -I./include

TARGET_OBJECTS = $(TARGET_NAME).o

TARGET_CLEAN = \
  conio/*.o ctype/*.o direct/*.o except/*.o float/*.o io/*.o \
	locale/*.o math/*.o mbstring/*.o misc/*.o process/*.o search/*.o \
	setjmp/*.o signal/*.o stdio/*.o stdlib/*.o string/*.o sys_stat/*.o \
	time/*.o wstring/*.o


include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk


FILES_CONIO = \
	conio/cgets.c \
	conio/cprintf.c \
	conio/cputs.c \
	conio/cscanf.c \
	conio/getch.c \
	conio/getche.c \
	conio/kbhit.c \
	conio/putch.c \
	conio/ungetch.c

FILES_CTYPE = \
	ctype/isalnum.c \
	ctype/isalpha.c \
	ctype/isascii.c \
	ctype/iscntrl.c \
	ctype/isdigit.c \
	ctype/isgraph.c \
	ctype/islower.c \
	ctype/isprint.c \
	ctype/ispunct.c \
	ctype/isspace.c \
	ctype/isupper.c \
	ctype/isxdigit.c \
	ctype/toascii.c \
	ctype/tolower.c \
	ctype/toupper.c \
	ctype/iscsym.c \
	ctype/isctype.c

FILES_DIRECT = \
	direct/chdir.c \
	direct/chdrive.c \
	direct/getcwd.c \
	direct/getdcwd.c \
	direct/getdfree.c \
	direct/getdrive.c \
	direct/mkdir.c \
	direct/rmdir.c

FILES_EXCEPT = \
	except/abnorter.c \
	except/exhand2.c \
	except/exhand3.c \
	except/matherr.c \
	except/unwind.c \
	except/xcptfil.c

FILES_FLOAT = \
	float/chgsign.c \
	float/clearfp.c \
	float/cntrlfp.c \
	float/copysign.c \
	float/fpclass.c \
	float/fpecode.c \
	float/fpreset.c \
	float/isnan.c \
	float/logb.c \
	float/nafter.c \
	float/scalb.c \
	float/statfp.c

FILES_IO = \
	io/access.c \
	io/chmod.c \
	io/chsize.c \
	io/close.c \
	io/commit.c \
	io/create.c \
	io/dup.c \
	io/dup2.c \
	io/eof.c \
	io/filelen.c \
	io/find.c \
	io/fmode.c \
	io/isatty.c \
	io/locking.c \
	io/lseek.c \
	io/mktemp.c \
	io/open.c \
	io/pipe.c \
	io/read.c \
	io/setmode.c \
	io/sopen.c \
	io/tell.c \
	io/umask.c \
	io/unlink.c \
	io/utime.c \
	io/write.c

FILES_LOCALE = \
	locale/locale.c

FILES_MATH = \
	math/acos.c \
	math/adjust.c \
	math/asin.c \
	math/atan.c \
	math/atan2.c \
	math/cabs.c \
	math/ceil.c \
	math/cos.c \
	math/cosh.c \
	math/exp.c \
	math/fabs.c \
	math/floor.c \
	math/fmod.c \
	math/frexp.c \
	math/ftol.c \
	math/huge_val.c \
	math/hypot.c \
	math/j0_y0.c \
	math/j1_y1.c \
	math/jn_yn.c \
	math/ldexp.c \
	math/log.c \
	math/log10.c \
	math/modf.c \
	math/pow.c \
	math/sin.c \
	math/sinh.c \
	math/sqrt.c \
	math/stubs.c \
	math/tan.c \
	math/tanh.c

FILES_MBSTRING = \
	mbstring/hanzen.c \
	mbstring/ischira.c \
	mbstring/iskana.c \
	mbstring/iskpun.c \
	mbstring/islead.c \
	mbstring/islwr.c \
	mbstring/ismbal.c \
	mbstring/ismbaln.c \
	mbstring/ismbc.c \
	mbstring/ismbgra.c \
	mbstring/ismbkaln.c \
	mbstring/ismblead.c \
	mbstring/ismbpri.c \
	mbstring/ismbpun.c \
	mbstring/ismbtrl.c \
	mbstring/isuppr.c \
	mbstring/jistojms.c \
	mbstring/jmstojis.c \
	mbstring/mbbtype.c \
	mbstring/mbccpy.c \
	mbstring/mbclen.c \
	mbstring/mbscat.c \
	mbstring/mbschr.c \
	mbstring/mbscmp.c \
	mbstring/mbscoll.c \
	mbstring/mbscpy.c \
	mbstring/mbscspn.c \
	mbstring/mbsdec.c \
	mbstring/mbsdup.c \
	mbstring/mbsicmp.c \
	mbstring/mbsicoll.c \
	mbstring/mbsinc.c \
	mbstring/mbslen.c \
	mbstring/mbslwr.c \
	mbstring/mbsncat.c \
	mbstring/mbsnccnt.c \
	mbstring/mbsncmp.c \
	mbstring/mbsncoll.c \
	mbstring/mbsncpy.c \
	mbstring/mbsnextc.c \
	mbstring/mbsnicmp.c \
	mbstring/mbsnicoll.c \
	mbstring/mbsninc.c \
	mbstring/mbsnset.c \
	mbstring/mbspbrk.c \
	mbstring/mbsrchr.c \
	mbstring/mbsrev.c \
	mbstring/mbsset.c \
	mbstring/mbsspn.c \
	mbstring/mbsspnp.c \
	mbstring/mbsstr.c \
	mbstring/mbstok.c \
	mbstring/mbstrlen.c \
	mbstring/mbsupr.c

FILES_MISC = \
	misc/amsg.c \
	misc/assert.c \
	misc/dllmain.c \
	misc/getargs.c \
	misc/initterm.c \
	misc/purecall.c \
	misc/tls.c

FILES_PROCESS = \
	process/_cwait.c \
	process/_system.c \
	process/dll.c \
	process/execl.c \
	process/execle.c \
	process/execlp.c \
	process/execlpe.c \
	process/execv.c \
	process/execve.c \
	process/execvp.c \
	process/execvpe.c \
	process/procid.c \
	process/spawnl.c \
	process/spawnle.c \
	process/spawnlp.c \
	process/spawnlpe.c \
	process/spawnv.c \
	process/spawnve.c \
	process/spawnvp.c \
	process/spawnvpe.c \
	process/thread.c \
	process/threadid.c

FILES_SEARCH = \
	search/lfind.c \
	search/lsearch.c

FILES_SETJMP = \
	setjmp/setjmp.c
	
FILES_SIGNAL = \
	signal/signal.c

FILES_STDIO = \
	stdio/allocfil.c \
	stdio/clearerr.c \
	stdio/fclose.c \
	stdio/fdopen.c \
	stdio/feof.c \
	stdio/ferror.c \
	stdio/fflush.c \
	stdio/fgetc.c \
	stdio/fgetchar.c \
	stdio/fgetpos.c \
	stdio/fgets.c \
	stdio/filbuf.c \
	stdio/fileno.c \
	stdio/flsbuf.c \
	stdio/fopen.c \
	stdio/fprintf.c \
	stdio/fputc.c \
	stdio/fputchar.c \
	stdio/fputs.c \
	stdio/fread.c \
	stdio/freopen.c \
	stdio/fscanf.c \
	stdio/fseek.c \
	stdio/fsetpos.c \
	stdio/fsopen.c \
	stdio/ftell.c \
	stdio/fwalk.c \
	stdio/fwrite.c \
	stdio/getc.c \
	stdio/getchar.c \
	stdio/gets.c \
	stdio/getw.c \
	stdio/perror.c \
	stdio/popen.c \
	stdio/printf.c \
	stdio/putc.c \
	stdio/putchar.c \
	stdio/puts.c \
	stdio/putw.c \
	stdio/remove.c \
	stdio/rename.c \
	stdio/rewind.c \
	stdio/rmtmp.c \
	stdio/scanf.c \
	stdio/setbuf.c \
	stdio/setvbuf.c \
	stdio/sprintf.c \
	stdio/sscanf.c \
	stdio/stdhnd.c \
	stdio/tempnam.c \
	stdio/tmpfile.c \
	stdio/tmpnam.c \
	stdio/ungetc.c \
	stdio/vfprintf.c \
	stdio/vfscanf.c \
	stdio/vfwprint.c \
	stdio/vprintf.c \
	stdio/vscanf.c \
	stdio/vsprintf.c \
	stdio/vsscanf.c

FILES_STDLIB = \
	stdlib/_exit.c \
	stdlib/abort.c \
	stdlib/abs.c \
	stdlib/atexit.c \
	stdlib/atof.c \
	stdlib/atoi.c \
	stdlib/atoi64.c \
	stdlib/atol.c \
	stdlib/bsearch.c \
	stdlib/div.c \
	stdlib/ecvt.c \
	stdlib/ecvtbuf.c \
	stdlib/errno.c \
	stdlib/fcvt.c \
	stdlib/fcvtbuf.c \
	stdlib/fullpath.c \
	stdlib/gcvt.c \
	stdlib/getenv.c \
	stdlib/itoa.c \
	stdlib/itow.c \
	stdlib/labs.c \
	stdlib/ldiv.c \
	stdlib/makepath.c \
	stdlib/malloc.c \
	stdlib/obsol.c \
	stdlib/putenv.c \
	stdlib/qsort.c \
	stdlib/rand.c \
	stdlib/rot.c \
	stdlib/senv.c \
	stdlib/splitp.c \
	stdlib/strtod.c \
	stdlib/strtol.c \
	stdlib/strtoul.c \
	stdlib/swab.c \
	stdlib/wcstod.c \
	stdlib/wcstol.c \
	stdlib/wcstoul.c \
	stdlib/wtoi.c \
	stdlib/wtoi64.c

FILES_STRING = \
	string/memccpy.c \
	string/memchr.c \
	string/memcmp.c \
	string/memcpy.c \
	string/memicmp.c \
	string/memmove.c \
	string/memset.c \
	string/strcat.c \
	string/strchr.c \
	string/strcmp.c \
	string/strcoll.c \
	string/strcpy.c \
	string/strcspn.c \
	string/strdup.c \
	string/strerror.c \
	string/stricmp.c \
	string/strlen.c \
	string/strlwr.c \
	string/strncat.c \
	string/strncmp.c \
	string/strncpy.c \
	string/strnicmp.c \
	string/strnlen.c \
	string/strpbrk.c \
	string/strrchr.c \
	string/strrev.c\
	string/strset.c \
	string/strspn.c \
	string/strstr.c \
	string/strtok.c \
	string/strupr.c \
	string/strxfrm.c 

FILES_SYS_STAT = \
	sys_stat/fstat.c \
	sys_stat/futime.c \
	sys_stat/stat.c

FILES_TIME = \
	time/clock.c \
	time/ctime.c \
	time/difftime.c \
	time/strdate.c \
	time/strftime.c \
	time/strtime.c \
	time/time.c

FILES_WSTRING = \
	wstring/wcscat.c \
	wstring/wcschr.c \
	wstring/wcscmp.c \
	wstring/wcscoll.c \
	wstring/wcscpy.c \
	wstring/wcscspn.c \
	wstring/wcsdup.c \
	wstring/wcsicmp.c \
	wstring/wcslen.c \
	wstring/wcslwr.c \
	wstring/wcsncat.c \
	wstring/wcsncmp.c \
	wstring/wcsncpy.c \
	wstring/wcsnicmp.c \
	wstring/wcsnlen.c \
	wstring/wcspbrk.c \
	wstring/wcsrchr.c \
	wstring/wcsrev.c \
	wstring/wcsset.c \
	wstring/wcsspn.c \
	wstring/wcsstr.c \
	wstring/wcstok.c \
	wstring/wcsupr.c \
	wstring/wcsxfrm.c

FILES = \
  $(FILES_CONIO) \
	$(FILES_CTYPE) \
	$(FILES_DIRECT) \
	$(FILES_EXCEPT) \
	$(FILES_FLOAT) \
	$(FILES_IO) \
	$(FILES_LOCALE) \
	$(FILES_MATH) \
	$(FILES_MBSTRING) \
	$(FILES_MISC) \
	$(FILES_PROCESS) \
	$(FILES_SEARCH) \
	$(FILES_SETJMP) \
	$(FILES_SIGNAL) \
	$(FILES_STDIO) \
	$(FILES_STDLIB) \
	$(FILES_STRING) \
	$(FILES_SYS_STAT) \
	$(FILES_TIME) \
	$(FILES_WSTRING)

OBJECTS = \
  conio.o \
  ctype.o \
  direct.o \
  except.o \
  float.o \
  io.o \
  locale/locale.o \
  math.o \
  mbstring.o \
  misc.o \
  process.o \
	search/lfind.o \
	search/lsearch.o \
	setjmp/setjmp.o \
	signal/signal.o \
  stdio.o \
  stdio/vprintf.o \
  stdio/vfprintf.o \
  stdio/vfwprint.o \
  stdlib.o \
  string.o \
  sys_stat.o \
  time.o \
  time/ctime.o \
  wstring.o

$(TARGET_NAME).o: $(OBJECTS) $(FILES)
	$(LD) -r $(OBJECTS) -o $(TARGET_NAME).o

# EOF
