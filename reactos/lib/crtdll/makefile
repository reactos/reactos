# $Id: makefile,v 1.53 2003/04/06 13:10:21 gvg Exp $

PATH_TO_TOP = ../..

PATH_TO_MSVCRT = ../msvcrt

TARGET_DEFONLY = yes

TARGET_TYPE = dynlink

TARGET_NAME = crtdll

TARGET_BASE = 0x77630000

TARGET_LFLAGS = -nostartfiles

TARGET_CFLAGS = -D_MSVCRT_LIB_

TARGET_SDKLIBS = kernel32.a ntdll.a

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
	$(PATH_TO_MSVCRT)/conio/cgets.o \
	$(PATH_TO_MSVCRT)/conio/cprintf.o \
	$(PATH_TO_MSVCRT)/conio/cputs.o \
	$(PATH_TO_MSVCRT)/conio/cscanf.o \
	$(PATH_TO_MSVCRT)/conio/getch.o \
	$(PATH_TO_MSVCRT)/conio/getche.o \
	$(PATH_TO_MSVCRT)/conio/kbhit.o \
	$(PATH_TO_MSVCRT)/conio/putch.o \
	$(PATH_TO_MSVCRT)/conio/ungetch.o

CTYPE_OBJECTS = \
	$(PATH_TO_MSVCRT)/ctype/ctype.o \
	$(PATH_TO_MSVCRT)/ctype/isalnum.o \
	$(PATH_TO_MSVCRT)/ctype/isalpha.o \
	$(PATH_TO_MSVCRT)/ctype/isascii.o \
	$(PATH_TO_MSVCRT)/ctype/iscntrl.o \
	$(PATH_TO_MSVCRT)/ctype/isdigit.o \
	$(PATH_TO_MSVCRT)/ctype/isgraph.o \
	$(PATH_TO_MSVCRT)/ctype/islower.o \
	$(PATH_TO_MSVCRT)/ctype/isprint.o \
	$(PATH_TO_MSVCRT)/ctype/ispunct.o \
	$(PATH_TO_MSVCRT)/ctype/isspace.o \
	$(PATH_TO_MSVCRT)/ctype/isupper.o \
	$(PATH_TO_MSVCRT)/ctype/isxdigit.o \
	$(PATH_TO_MSVCRT)/ctype/toascii.o \
	$(PATH_TO_MSVCRT)/ctype/tolower.o \
	$(PATH_TO_MSVCRT)/ctype/toupper.o \
	$(PATH_TO_MSVCRT)/ctype/iscsym.o \
	ctype/isctype.o

DIRECT_OBJECTS = \
	$(PATH_TO_MSVCRT)/direct/chdir.o \
	$(PATH_TO_MSVCRT)/direct/chdrive.o \
	$(PATH_TO_MSVCRT)/direct/getcwd.o \
	$(PATH_TO_MSVCRT)/direct/getdcwd.o \
	$(PATH_TO_MSVCRT)/direct/getdfree.o \
	$(PATH_TO_MSVCRT)/direct/getdrive.o \
	$(PATH_TO_MSVCRT)/direct/mkdir.o \
	$(PATH_TO_MSVCRT)/direct/rmdir.o

EXCEPT_OBJECTS = \
	except/abnorter.o \
	except/exhand2.o \
	except/matherr.o \
	except/unwind.o

FLOAT_OBJECTS = \
	$(PATH_TO_MSVCRT)/float/chgsign.o \
	$(PATH_TO_MSVCRT)/float/clearfp.o \
	$(PATH_TO_MSVCRT)/float/cntrlfp.o \
	$(PATH_TO_MSVCRT)/float/copysign.o \
	$(PATH_TO_MSVCRT)/float/fpclass.o \
	$(PATH_TO_MSVCRT)/float/fpreset.o \
	$(PATH_TO_MSVCRT)/float/isnan.o \
	$(PATH_TO_MSVCRT)/float/logb.o \
	$(PATH_TO_MSVCRT)/float/nafter.o \
	$(PATH_TO_MSVCRT)/float/scalb.o \
	$(PATH_TO_MSVCRT)/float/statfp.o

IO_OBJECTS = \
	$(PATH_TO_MSVCRT)/io/access.o \
	$(PATH_TO_MSVCRT)/io/chmod.o \
	$(PATH_TO_MSVCRT)/io/chsize.o \
	$(PATH_TO_MSVCRT)/io/close.o \
	$(PATH_TO_MSVCRT)/io/commit.o \
	io/create.o \
	io/dup.o \
	$(PATH_TO_MSVCRT)/io/dup2.o \
	io/eof.o \
	$(PATH_TO_MSVCRT)/io/filelen.o \
	io/find.o \
	io/fmode.o \
	$(PATH_TO_MSVCRT)/io/isatty.o \
	$(PATH_TO_MSVCRT)/io/locking.o \
	io/lseek.o \
	$(PATH_TO_MSVCRT)/io/mktemp.o \
	io/open.o \
	io/pipe.o \
	io/read.o \
	$(PATH_TO_MSVCRT)/io/setmode.o \
	$(PATH_TO_MSVCRT)/io/sopen.o \
	$(PATH_TO_MSVCRT)/io/tell.o \
	$(PATH_TO_MSVCRT)/io/umask.o \
	io/unlink.o \
	$(PATH_TO_MSVCRT)/io/utime.o \
	io/write.o

LOCALE_OBJECTS = \
	$(PATH_TO_MSVCRT)/locale/locale.o 

MATH_OBJECTS = \
	$(PATH_TO_MSVCRT)/math/acos.o \
	math/acosh.o \
	$(PATH_TO_MSVCRT)/math/asin.o \
	math/asinh.o \
	$(PATH_TO_MSVCRT)/math/atan.o \
	$(PATH_TO_MSVCRT)/math/atan2.o\
	math/atanh.o \
	$(PATH_TO_MSVCRT)/math/cabs.o \
	math/ceil.o \
	$(PATH_TO_MSVCRT)/math/cos.o \
	$(PATH_TO_MSVCRT)/math/cosh.o \
	$(PATH_TO_MSVCRT)/math/exp.o \
	$(PATH_TO_MSVCRT)/math/fabs.o\
	math/floor.o \
	$(PATH_TO_MSVCRT)/math/fmod.o \
	$(PATH_TO_MSVCRT)/math/frexp.o \
	$(PATH_TO_MSVCRT)/math/ftol.o \
	math/huge_val.o \
	$(PATH_TO_MSVCRT)/math/hypot.o \
	$(PATH_TO_MSVCRT)/math/j0_y0.o \
	$(PATH_TO_MSVCRT)/math/j1_y1.o \
	$(PATH_TO_MSVCRT)/math/jn_yn.o \
	$(PATH_TO_MSVCRT)/math/ldexp.o \
	$(PATH_TO_MSVCRT)/math/log.o \
	$(PATH_TO_MSVCRT)/math/log10.o \
	$(PATH_TO_MSVCRT)/math/modf.o \
	$(PATH_TO_MSVCRT)/math/pow.o \
	$(PATH_TO_MSVCRT)/math/sin.o \
	$(PATH_TO_MSVCRT)/math/sinh.o \
	$(PATH_TO_MSVCRT)/math/sqrt.o \
	$(PATH_TO_MSVCRT)/math/stubs.o \
	$(PATH_TO_MSVCRT)/math/tan.o \
	$(PATH_TO_MSVCRT)/math/tanh.o

MALLOC_OBJECTS = \
	malloc/expand.o \
	malloc/heap.o

MBSTRING_OBJECTS = \
	$(PATH_TO_MSVCRT)/mbstring/hanzen.o \
	$(PATH_TO_MSVCRT)/mbstring/ischira.o \
	$(PATH_TO_MSVCRT)/mbstring/iskana.o \
	$(PATH_TO_MSVCRT)/mbstring/iskpun.o \
	$(PATH_TO_MSVCRT)/mbstring/islead.o \
	$(PATH_TO_MSVCRT)/mbstring/islwr.o \
	$(PATH_TO_MSVCRT)/mbstring/iskmoji.o \
	$(PATH_TO_MSVCRT)/mbstring/ismbal.o \
	$(PATH_TO_MSVCRT)/mbstring/ismbaln.o \
	$(PATH_TO_MSVCRT)/mbstring/ismbc.o \
	$(PATH_TO_MSVCRT)/mbstring/ismbgra.o \
	$(PATH_TO_MSVCRT)/mbstring/ismbkaln.o \
	$(PATH_TO_MSVCRT)/mbstring/ismblead.o \
	$(PATH_TO_MSVCRT)/mbstring/ismbpri.o \
	$(PATH_TO_MSVCRT)/mbstring/ismbpun.o \
	$(PATH_TO_MSVCRT)/mbstring/ismbtrl.o \
	$(PATH_TO_MSVCRT)/mbstring/isuppr.o \
	$(PATH_TO_MSVCRT)/mbstring/jistojms.o \
	$(PATH_TO_MSVCRT)/mbstring/jmstojis.o \
	$(PATH_TO_MSVCRT)/mbstring/mbbtype.o \
	$(PATH_TO_MSVCRT)/mbstring/mbccpy.o \
	$(PATH_TO_MSVCRT)/mbstring/mbclen.o \
	$(PATH_TO_MSVCRT)/mbstring/mbscat.o \
	$(PATH_TO_MSVCRT)/mbstring/mbschr.o \
	$(PATH_TO_MSVCRT)/mbstring/mbscmp.o \
	$(PATH_TO_MSVCRT)/mbstring/mbscoll.o \
	$(PATH_TO_MSVCRT)/mbstring/mbscpy.o \
	$(PATH_TO_MSVCRT)/mbstring/mbscspn.o \
	$(PATH_TO_MSVCRT)/mbstring/mbsdec.o \
	$(PATH_TO_MSVCRT)/mbstring/mbsdup.o \
	$(PATH_TO_MSVCRT)/mbstring/mbsicmp.o \
	$(PATH_TO_MSVCRT)/mbstring/mbsicoll.o \
	$(PATH_TO_MSVCRT)/mbstring/mbsinc.o \
	$(PATH_TO_MSVCRT)/mbstring/mbslen.o \
	$(PATH_TO_MSVCRT)/mbstring/mbslwr.o \
	$(PATH_TO_MSVCRT)/mbstring/mbsncat.o \
	$(PATH_TO_MSVCRT)/mbstring/mbsnccnt.o \
	$(PATH_TO_MSVCRT)/mbstring/mbsncmp.o \
	$(PATH_TO_MSVCRT)/mbstring/mbsncoll.o \
	$(PATH_TO_MSVCRT)/mbstring/mbsncpy.o \
	$(PATH_TO_MSVCRT)/mbstring/mbsnextc.o \
	$(PATH_TO_MSVCRT)/mbstring/mbsnicmp.o \
	$(PATH_TO_MSVCRT)/mbstring/mbsnicoll.o \
	$(PATH_TO_MSVCRT)/mbstring/mbsninc.o \
	$(PATH_TO_MSVCRT)/mbstring/mbsnset.o \
	$(PATH_TO_MSVCRT)/mbstring/mbspbrk.o \
	$(PATH_TO_MSVCRT)/mbstring/mbsrchr.o \
	$(PATH_TO_MSVCRT)/mbstring/mbsrev.o \
	$(PATH_TO_MSVCRT)/mbstring/mbsset.o \
	$(PATH_TO_MSVCRT)/mbstring/mbsspn.o \
	$(PATH_TO_MSVCRT)/mbstring/mbsspnp.o \
	$(PATH_TO_MSVCRT)/mbstring/mbsstr.o \
	$(PATH_TO_MSVCRT)/mbstring/mbstok.o \
	$(PATH_TO_MSVCRT)/mbstring/mbstrlen.o \
	$(PATH_TO_MSVCRT)/mbstring/mbsupr.o

MISC_OBJECTS = \
	misc/amsg.o \
	$(PATH_TO_MSVCRT)/misc/assert.o \
	misc/debug.o \
	misc/dllmain.o \
	misc/GetArgs.o \
	$(PATH_TO_MSVCRT)/misc/initterm.o \
	misc/purecall.o \
	misc/setnew.o

PROCESS_OBJECTS = \
	process/_cwait.o \
	process/_system.o\
	$(PATH_TO_MSVCRT)/process/dll.o \
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
	$(PATH_TO_MSVCRT)/process/procid.o \
	process/thread.o \
	$(PATH_TO_MSVCRT)/process/threadid.o

QUAD_OBJECTS = \
	quad/qdivrem.o \
	quad/divdi3.o \
	quad/moddi3.o \
	quad/udivdi3.o \
	quad/umoddi3.o

SEARCH_OBJECTS = \
	$(PATH_TO_MSVCRT)/search/lfind.o \
	$(PATH_TO_MSVCRT)/search/lsearch.o

SETJMP_OBJECTS = \
	$(PATH_TO_MSVCRT)/setjmp/i386/setjmp.o

SIGNAL_OBJECTS = \
	signal/xcptfil.o \
	signal/xcptinfo.o \
	$(PATH_TO_MSVCRT)/signal/signal.o

STDIO_OBJECTS = \
	$(PATH_TO_MSVCRT)/stdio/allocfil.o \
	stdio/getenv.o \
	$(PATH_TO_MSVCRT)/stdio/clearerr.o \
	$(PATH_TO_MSVCRT)/stdio/fclose.o \
	$(PATH_TO_MSVCRT)/stdio/fdopen.o \
	$(PATH_TO_MSVCRT)/stdio/feof.o \
	stdio/ferror.o \
	stdio/fflush.o \
	$(PATH_TO_MSVCRT)/stdio/fgetc.o \
	stdio/fgetchar.o \
	$(PATH_TO_MSVCRT)/stdio/fgetpos.o \
	stdio/fgets.o \
	stdio/filbuf.o \
	stdio/fileno.o \
	$(PATH_TO_MSVCRT)/stdio/flsbuf.o \
	stdio/fopen.o \
	$(PATH_TO_MSVCRT)/stdio/fprintf.o \
	$(PATH_TO_MSVCRT)/stdio/fputc.o \
	stdio/fputchar.o\
	stdio/fputs.o \
	stdio/fread.o \
	stdio/freopen.o \
	stdio/fscanf.o \
	$(PATH_TO_MSVCRT)/stdio/fseek.o \
	$(PATH_TO_MSVCRT)/stdio/fsetpos.o \
	stdio/frlist.o \
	stdio/fsopen.o \
	stdio/ftell.o \
	$(PATH_TO_MSVCRT)/stdio/fwalk.o \
	stdio/fwrite.o \
	stdio/getc.o \
	stdio/getchar.o \
	stdio/gets.o \
	$(PATH_TO_MSVCRT)/stdio/getw.o \
	stdio/perror.o \
	stdio/popen.o \
	stdio/printf.o \
	stdio/putc.o \
	stdio/putchar.o \
	stdio/puts.o \
	$(PATH_TO_MSVCRT)/stdio/putw.o \
	stdio/remove.o \
	$(PATH_TO_MSVCRT)/stdio/rename.o \
	$(PATH_TO_MSVCRT)/stdio/rewind.o \
	$(PATH_TO_MSVCRT)/stdio/rmtmp.o \
	stdio/scanf.o \
	$(PATH_TO_MSVCRT)/stdio/setbuf.o \
	stdio/setbuffe.o \
	stdio/setlineb.o \
	$(PATH_TO_MSVCRT)/stdio/setvbuf.o \
	$(PATH_TO_MSVCRT)/stdio/sprintf.o \
	$(PATH_TO_MSVCRT)/stdio/sscanf.o \
	$(PATH_TO_MSVCRT)/stdio/stdhnd.o \
	stdio/stdiohk.o \
	$(PATH_TO_MSVCRT)/stdio/tempnam.o \
	$(PATH_TO_MSVCRT)/stdio/tmpfile.o \
	$(PATH_TO_MSVCRT)/stdio/tmpnam.o \
	$(PATH_TO_MSVCRT)/stdio/ungetc.o \
	stdio/vfprintf.o \
	stdio/vfscanf.o \
	stdio/vfwprint.o \
	$(PATH_TO_MSVCRT)/stdio/vprintf.o \
	$(PATH_TO_MSVCRT)/stdio/vscanf.o \
	$(PATH_TO_MSVCRT)/stdio/vsprintf.o \
	$(PATH_TO_MSVCRT)/stdio/vsscanf.o
 
STDLIB_OBJECTS = \
	$(PATH_TO_MSVCRT)/stdlib/_exit.o \
	$(PATH_TO_MSVCRT)/stdlib/abort.o \
	$(PATH_TO_MSVCRT)/stdlib/abs.o \
	$(PATH_TO_MSVCRT)/stdlib/atexit.o \
	$(PATH_TO_MSVCRT)/stdlib/atof.o \
	$(PATH_TO_MSVCRT)/stdlib/atoi.o \
	$(PATH_TO_MSVCRT)/stdlib/atol.o \
	$(PATH_TO_MSVCRT)/stdlib/bsearch.o \
	$(PATH_TO_MSVCRT)/stdlib/div.o \
	$(PATH_TO_MSVCRT)/stdlib/ecvt.o \
	$(PATH_TO_MSVCRT)/stdlib/ecvtbuf.o \
	stdlib/errno.o \
	$(PATH_TO_MSVCRT)/stdlib/fcvt.o \
	$(PATH_TO_MSVCRT)/stdlib/fcvtbuf.o \
	stdlib/fullpath.o \
	$(PATH_TO_MSVCRT)/stdlib/gcvt.o \
	$(PATH_TO_MSVCRT)/stdlib/itoa.o \
	stdlib/itow.o \
	$(PATH_TO_MSVCRT)/stdlib/labs.o \
	$(PATH_TO_MSVCRT)/stdlib/ldiv.o \
	$(PATH_TO_MSVCRT)/stdlib/makepath.o \
	stdlib/malloc.o \
	stdlib/mbstowcs.o \
	$(PATH_TO_MSVCRT)/stdlib/obsol.o \
	stdlib/putenv.o \
	stdlib/qsort.o \
	stdlib/rand.o \
	$(PATH_TO_MSVCRT)/stdlib/rot.o \
	$(PATH_TO_MSVCRT)/stdlib/senv.o \
	$(PATH_TO_MSVCRT)/stdlib/splitp.o \
	$(PATH_TO_MSVCRT)/stdlib/strtod.o \
	$(PATH_TO_MSVCRT)/stdlib/strtol.o \
	$(PATH_TO_MSVCRT)/stdlib/strtoul.o \
	$(PATH_TO_MSVCRT)/stdlib/swab.o \
	$(PATH_TO_MSVCRT)/stdlib/wcstomb.o \

STRING_OBJECTS = \
	$(PATH_TO_MSVCRT)/string/memccpy.o \
	$(PATH_TO_MSVCRT)/string/memchr.o \
	$(PATH_TO_MSVCRT)/string/memcmp.o \
	$(PATH_TO_MSVCRT)/string/memcpy.o \
	$(PATH_TO_MSVCRT)/string/memicmp.o \
	$(PATH_TO_MSVCRT)/string/memmove.o \
	$(PATH_TO_MSVCRT)/string/memset.o \
	$(PATH_TO_MSVCRT)/string/strcat.o \
	$(PATH_TO_MSVCRT)/string/strchr.o \
	$(PATH_TO_MSVCRT)/string/strcmp.o \
	$(PATH_TO_MSVCRT)/string/strcoll.o \
	$(PATH_TO_MSVCRT)/string/strcpy.o \
	$(PATH_TO_MSVCRT)/string/strcspn.o \
	$(PATH_TO_MSVCRT)/string/strdup.o \
	string/strerror.o \
	$(PATH_TO_MSVCRT)/string/stricmp.o \
	$(PATH_TO_MSVCRT)/string/strlen.o \
	$(PATH_TO_MSVCRT)/string/strlwr.o \
	$(PATH_TO_MSVCRT)/string/strncat.o \
	$(PATH_TO_MSVCRT)/string/strncmp.o \
	$(PATH_TO_MSVCRT)/string/strncpy.o \
	$(PATH_TO_MSVCRT)/string/strnicmp.o \
	$(PATH_TO_MSVCRT)/string/strnlen.o \
	$(PATH_TO_MSVCRT)/string/strpbrk.o \
	$(PATH_TO_MSVCRT)/string/strrchr.o \
	$(PATH_TO_MSVCRT)/string/strrev.o \
	$(PATH_TO_MSVCRT)/string/strset.o \
	$(PATH_TO_MSVCRT)/string/strspn.o \
	$(PATH_TO_MSVCRT)/string/strstr.o \
	string/strtok.o \
	$(PATH_TO_MSVCRT)/string/strupr.o \
	string/str_old.o \
	$(PATH_TO_MSVCRT)/string/strxfrm.o

SYS_STAT_OBJECTS = \
	sys_stat/fstat.o \
	$(PATH_TO_MSVCRT)/sys_stat/futime.o \
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
	$(PATH_TO_MSVCRT)/time/clock.o \
	$(PATH_TO_MSVCRT)/time/ctime.o \
	$(PATH_TO_MSVCRT)/time/difftime.o \
	$(PATH_TO_MSVCRT)/time/strdate.o \
	$(PATH_TO_MSVCRT)/time/strftime.o \
	$(PATH_TO_MSVCRT)/time/strtime.o \
	$(PATH_TO_MSVCRT)/time/time.o \
	time/tz_vars.o \

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
