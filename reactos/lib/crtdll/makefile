# $Id: makefile,v 1.50 2002/11/24 18:42:12 robd Exp $

PATH_TO_TOP = ../..

PATH_TO_MSVCRT = ../msvcrt

TARGET_DEFONLY = yes

TARGET_TYPE = dynlink

TARGET_NAME = crtdll

TARGET_BASE = 0x77630000

TARGET_LFLAGS = -nostartfiles

TARGET_SDKLIBS = kernel32.a

TARGET_OBJECTS = $(TARGET_NAME).o

TARGET_CLEAN = \
	conio/*.o \
	ctype/*.o \
	direct/*.o \
	dirent/*.o \
	except/*.o \
	float/*.o \
	io/*.o \
	libc/*.o \
	locale/*.o \
	malloc/*.o \
	math/*.o \
	mbstring/*.o \
	misc/*.o \
	process/*.o \
	quad/*.o \
	search/*.o \
	setjmp/*.o \
	signal/*.o \
	stdio/*.o \
	stdlib/*.o \
	string/*.o \
	sys_stat/*.o \
	tchar/*.o \
	time/*.o \
	wchar/*.o


include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk


CONIO_OBJECTS = \
	conio/cgets.o \
	conio/cprintf.o \
	conio/cputs.o \
	conio/cscanf.o \
	conio/getch.o \
	conio/getche.o \
	conio/kbhit.o \
	conio/putch.o \
	conio/ungetch.o

CTYPE_OBJECTS = \
	ctype/ctype.o \
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

DIRECT_OBJECTS = \
	direct/chdir.o \
	direct/chdrive.o \
	direct/getcwd.o \
	direct/getdcwd.o \
	direct/getdfree.o \
	direct/getdrive.o \
	direct/mkdir.o \
	direct/rmdir.o

EXCEPT_OBJECTS = \
	except/abnorter.o \
	except/exhand2.o \
	except/matherr.o \
	except/unwind.o

FLOAT_OBJECTS = \
	float/chgsign.o \
	float/clearfp.o \
	float/cntrlfp.o \
	float/copysign.o \
	float/fpclass.o \
	float/fpreset.o \
	float/isnan.o \
	float/logb.o \
	float/nafter.o \
	float/scalb.o \
	float/statfp.o

IO_OBJECTS = \
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

LOCALE_OBJECTS = \
	locale/locale.o 

MATH_OBJECTS = \
	math/acos.o \
	math/acosh.o \
	math/asin.o \
	math/asinh.o \
	math/atan.o \
	math/atan2.o\
	math/atanh.o \
	math/cabs.o \
	math/ceil.o \
	math/cos.o \
	math/cosh.o \
	math/exp.o \
	math/fabs.o\
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

MALLOC_OBJECTS = \
	malloc/expand.o \
	malloc/heap.o

MBSTRING_OBJECTS = \
	mbstring/hanzen.o \
	mbstring/ischira.o \
	mbstring/iskana.o \
	mbstring/iskpun.o \
	mbstring/islead.o \
	mbstring/islwr.o \
	mbstring/iskmoji.o \
	mbstring/ismbal.o \
	mbstring/ismbaln.o \
	mbstring/ismbc.o \
	mbstring/ismbgra.o \
	mbstring/ismbkaln.o \
	mbstring/ismblead.o \
	mbstring/ismbpri.o \
	mbstring/ismbpun.o \
	mbstring/ismbtrl.o \
	mbstring/isuppr.o \
	mbstring/jistojms.o \
	mbstring/jmstojis.o \
	mbstring/mbbtype.o \
	mbstring/mbccpy.o \
	mbstring/mbclen.o \
	mbstring/mbscat.o \
	mbstring/mbschr.o \
	mbstring/mbscmp.o \
	mbstring/mbscoll.o \
	mbstring/mbscpy.o \
	mbstring/mbscspn.o \
	mbstring/mbsdec.o \
	mbstring/mbsdup.o \
	mbstring/mbsicmp.o \
	mbstring/mbsicoll.o \
	mbstring/mbsinc.o \
	mbstring/mbslen.o \
	mbstring/mbslwr.o \
	mbstring/mbsncat.o \
	mbstring/mbsnccnt.o \
	mbstring/mbsncmp.o \
	mbstring/mbsncoll.o \
	mbstring/mbsncpy.o \
	mbstring/mbsnextc.o \
	mbstring/mbsnicmp.o \
	mbstring/mbsnicoll.o \
	mbstring/mbsninc.o \
	mbstring/mbsnset.o \
	mbstring/mbspbrk.o \
	mbstring/mbsrchr.o \
	mbstring/mbsrev.o \
	mbstring/mbsset.o \
	mbstring/mbsspn.o \
	mbstring/mbsspnp.o \
	mbstring/mbsstr.o \
	mbstring/mbstok.o \
	mbstring/mbstrlen.o \
	mbstring/mbsupr.o

MISC_OBJECTS = \
	misc/amsg.o \
	misc/assert.o \
	misc/debug.o \
	misc/dllmain.o \
	misc/GetArgs.o \
	misc/initterm.o \
	misc/purecall.o \
	misc/setnew.o

PROCESS_OBJECTS = \
	process/_cwait.o \
	process/_system.o\
	process/dll.o \
	process/spawnl.o \
	process/spawnlp.o \
	process/spawnlpe.o \
	process/spawnvp.o \
	process/spawnv.o \
	process/spawnve.o \
	process/spawnle.o \
	process/execl.o \
	process/execlp.o \
	process/execlpe.o \
	process/execvpe.o \
	process/execvp.o \
	process/execv.o \
	process/execle.o \
	process/execve.o \
	process/procid.o \
	process/thread.o \
	process/threadid.o

QUAD_OBJECTS = \
	quad/qdivrem.o \
	quad/divdi3.o \
	quad/moddi3.o \
	quad/udivdi3.o \
	quad/umoddi3.o

SEARCH_OBJECTS = \
	search/lfind.o \
	search/lsearch.o

SETJMP_OBJECTS = \
	setjmp/setjmp.o

SIGNAL_OBJECTS = \
	signal/xcptfil.o \
	signal/xcptinfo.o \
	signal/signal.o

STDIO_OBJECTS = \
	stdio/allocfil.o \
	stdio/getenv.o \
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
	stdio/fputchar.o\
	stdio/fputs.o \
	stdio/fread.o \
	stdio/freopen.o \
	stdio/fscanf.o \
	stdio/fseek.o \
	stdio/fsetpos.o \
	stdio/frlist.o \
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
	stdio/setbuffe.o \
	stdlib/obsol.o \
	stdio/setlineb.o \
	stdio/scanf.o \
	stdio/setbuf.o \
	stdio/setvbuf.o \
	stdio/sprintf.o \
	stdio/sscanf.o \
	stdio/stdhnd.o \
	stdio/stdiohk.o \
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
 
STDLIB_OBJECTS = \
	stdlib/_exit.o \
	stdlib/abort.o \
	stdlib/abs.o \
	stdlib/atexit.o \
	stdlib/atof.o \
	stdlib/atoi.o \
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
	stdlib/itoa.o \
	stdlib/itow.o \
	stdlib/labs.o \
	stdlib/ldiv.o \
	stdlib/makepath.o \
	stdlib/malloc.o \
	stdlib/mbstowcs.o \
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
	stdlib/wcstomb.o \

STRING_OBJECTS = \
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
	string/strrev.o \
	string/strset.o \
	string/strspn.o \
	string/strstr.o \
	string/strtok.o \
	string/strupr.o \
	string/str_old.o \
	string/strxfrm.o

SYS_STAT_OBJECTS = \
	sys_stat/fstat.o \
	sys_stat/futime.o \
	sys_stat/ftime.o \
	sys_stat/systime.o \
	sys_stat/stat.o

TCHAR_OBJECTS = \
	tchar/strdec.o \
	tchar/strinc.o \
	tchar/strninc.o \
	tchar/strncnt.o \
	tchar/strnextc.o \
	tchar/strspnp.o

TIME_OBJECTS = \
	time/clock.o \
	time/ctime.o \
	time/difftime.o \
	time/strdate.o \
	time/strftime.o \
	time/strtime.o \
	time/time.o

WSTRING_OBJECTS = \
	$(PATH_TO_MSVCRT)/wstring/wcscat.o \
	$(PATH_TO_MSVCRT)/wstring/wcschr.o \
	$(PATH_TO_MSVCRT)/wstring/wcscmp.o \
	wchar/wcscoll.o \
	$(PATH_TO_MSVCRT)/wstring/wcscpy.o \
	$(PATH_TO_MSVCRT)/wstring/wcscspn.o \
	$(PATH_TO_MSVCRT)/wstring/wcsdup.o \
	$(PATH_TO_MSVCRT)/wstring/wcsicmp.o \
	wchar/wcslen.o \
	$(PATH_TO_MSVCRT)/wstring/wcslwr.o \
	$(PATH_TO_MSVCRT)/wstring/wcsncat.o \
	$(PATH_TO_MSVCRT)/wstring/wcsncmp.o \
	$(PATH_TO_MSVCRT)/wstring/wcsncpy.o \
	$(PATH_TO_MSVCRT)/wstring/wcsnlen.o \
	$(PATH_TO_MSVCRT)/wstring/wcspbrk.o \
	$(PATH_TO_MSVCRT)/wstring/wcsrchr.o \
	$(PATH_TO_MSVCRT)/wstring/wcsrev.o \
	$(PATH_TO_MSVCRT)/wstring/wcsset.o \
	$(PATH_TO_MSVCRT)/wstring/wcsspn.o \
	$(PATH_TO_MSVCRT)/wstring/wcsstr.o \
	wchar/wcstod.o \
	wchar/wcstok.o \
	wchar/wcstol.o \
	$(PATH_TO_MSVCRT)/wstring/wcsupr.o \
	$(PATH_TO_MSVCRT)/wstring/wcsxfrm.o \
	wchar/wtoi.o \
	wchar/wcstombs.o \
	$(PATH_TO_MSVCRT)/wstring/wcsnicmp.o 


OBJECTS = \
	$(CONIO_OBJECTS) \
	$(CTYPE_OBJECTS) \
	$(DIRECT_OBJECTS) \
	$(EXCEPT_OBJECTS) \
	$(FLOAT_OBJECTS) \
	$(IO_OBJECTS) \
	$(LOCALE_OBJECTS) \
	$(MALLOC_OBJECTS) \
	$(MATH_OBJECTS) \
	$(MBSTRING_OBJECTS)\
	$(MISC_OBJECTS) \
	$(PROCESS_OBJECTS) \
	$(SEARCH_OBJECTS) \
	$(SETJMP_OBJECTS) \
	$(SIGNAL_OBJECTS) \
	$(STDIO_OBJECTS) \
	$(STDLIB_OBJECTS) \
	$(STRING_OBJECTS) \
	$(SYS_STAT_OBJECTS) \
	$(TCHAR_OBJECTS) \
	$(TIME_OBJECTS) \
	$(WSTRING_OBJECTS)

$(TARGET_NAME).o: $(OBJECTS)
	$(LD) -r $(OBJECTS) -o $(TARGET_NAME).o

# EOF
