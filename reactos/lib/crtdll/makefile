
all: crtdll.a


CTYPE_OBJECTS = ctype/isalnum.o \
		ctype/isalpha.o ctype/isascii.o ctype/iscntrl.o ctype/isdigit.o ctype/isgraph.o \
		ctype/islower.o ctype/isprint.o ctype/ispunct.o ctype/isspace.o ctype/isupper.o \
		ctype/isxdigit.o ctype/toascii.o ctype/tolower.o ctype/toupper.o\
		ctype/iscsym.o ctype/isctype.o

CONIO_OBJECTS = conio/cputs.o conio/getch.o conio/getche.o conio/putch.o conio/ungetch.o

DIRECT_OBJECTS = direct/chdir.o direct/chdrive.o direct/getcwd.o direct/getdrive.o \
		 direct/rmdir.o direct/mkdir.o direct/getdfree.o


MISC_OBJECTS =  misc/sleep.o  misc/getargs.o misc/crtfmode.o misc/crtglob.o

STRING_OBJECTS = string/memchr.o string/memcmp.o string/strcat.o \
                 string/strchr.o string/strcmp.o string/strcoll.o \
		 string/strcpy.o string/strcspn.o string/memcpy.o \
		 string/strlen.o string/strncat.o string/strncmp.o \
		 string/strncpy.o string/strpbrk.o string/strrchr.o \
		 string/strspn.o string/strstr.o string/strtok.o \
		 string/strxfrm.o string/memmove.o string/memset.o \
		 string/strdup.o string/strlwr.o string/strupr.o \
		 string/str_old.o string/strerror.o

WCHAR_OBJECTS = wchar/wcscat.o wchar/wcschr.o wchar/wcscmp.o \
		 wchar/wcscoll.o wchar/wcscpy.o wchar/wcscspn.o \
		 wchar/wcsdup.o wchar/wcsicmp.o wchar/wcslen.o \
		 wchar/wcslwr.o wchar/wcsncat.o wchar/wcsncmp.o \
		 wchar/wcsncpy.o

STDIO_OBJECTS = stdio/getenv.o stdio/doprnt.o stdio/doscan.o stdio/filbuf.o  \
		stdio/fclose.o stdio/feof.o stdio/ferror.o stdio/fileno.o\
		stdio/fflush.o stdio/fgetc.o stdio/fgetpos.o stdio/fgets.o stdio/flsbuf.o \
		stdio/fopen.o stdio/fprintf.o stdio/fputc.o stdio/fputs.o \
		stdio/fread.o stdio/freopen.o  stdio/fscanf.o stdio/fseek.o \
		stdio/fsetpos.o stdio/ftell.o stdio/fwalk.o stdio/fwrite.o stdio/getc.o \
		stdio/getchar.o stdio/gets.o stdio/getw.o stdio/perror.o stdio/printf.o \
		stdio/putc.o stdio/putchar.o stdio/puts.o stdio/putw.o \
		stdio/remove.o stdio/rename.o stdio/rewind.o stdio/allocfil.o\
		stdio/scanf.o stdio/setbuf.o stdio/setbuffe.o \
		stdio/setlineb.o stdio/setvbuf.o stdio/sprintf.o stdio/sscanf.o \
		stdio/stdiohk.o stdio/stdhnd.o stdio/tempnam.o stdio/tmpfile.o stdio/tmpnam.o \
		stdio/ungetc.o stdio/vfprintf.o stdio/vprintf.o stdio/vsprintf.o


IO_OBJECTS = io/access.o io/close.o io/create.o io/dup.o io/dup2.o io/find.o io/isatty.o io/lseek.o \
		io/open.o io/read.o io/setmode.o io/unlink.o io/write.o io/fmode.o io/mktemp.o\
		io/chmod.o io/chsize.o io/commit.o io/locking.o io/pipe.o io/sopen.o io/filelen.o\
		io/umask.o io/tell.o io/eof.o

STDLIB_OBJECTS = stdlib/abort.o stdlib/abs.o stdlib/atexit.o stdlib/atof.o stdlib/atoi.o stdlib/atold.o \
		 stdlib/bsearch.o stdlib/div.o stdlib/errno.o  stdlib/exit.o \
		 stdlib/fullpath.o stdlib/labs.o stdlib/ldiv.o stdlib/llabs.o stdlib/lldiv.o \
		 stdlib/makepath.o stdlib/malloc.o stdlib/putenv.o stdlib/qsort.o \
		 stdlib/rand.o stdlib/senv.o stdlib/splitp.o stdlib/strtod.o stdlib/strtol.o \
		 stdlib/strtoul.o stdlib/strtold.o 

SIGNAL_OBJECTS = signal/signal.o

PROCESS_OBJECTS = process/cwait.o process/dll.o process/spawnl.o process/spawnlp.o process/spawnlpe.o  process/spawnvpe.o process/spawnvp.o \
		process/spawnv.o process/spawnve.o process/spawnle.o process/execl.o process/execlp.o process/execlpe.o  \
		process/execvpe.o process/execvp.o process/execv.o process/execle.o

TIME_OBJECTS = time/ctime.o time/difftime.o time/strftime.o 

FLOAT_OBJECTS = float/fpreset.o float/clearfp.o float/cntrlfp.o float/statfp.o

SYS_STAT_OBJECTS = sys_stat/fstat.o sys_stat/stat.o 

MATH_OBJECTS = math/acos.o math/acosh.o math/asin.o math/asinh.o math/atan.o math/atan2.o\
		math/atanh.o math/ceil.o math/cos.o math/cosh.o math/exp.o math/fabs.o\
		math/floor.o math/fmod.o math/frexp.o math/huge_val.o math/hypot.o\
		math/ldexp.o math/log.o math/log10.o math/modf.o math/modfl.o math/pow.o\
		math/sin.o math/sinh.o math/sqrt.o math/tan.o\
		math/tanh.o


OBJECTS = $(CTYPE_OBJECTS) $(CONIO_OBJECTS) $(DIRECT_OBJECTS) $(MISC_OBJECTS) \
	  $(STRING_OBJECTS)  $(STDIO_OBJECTS) $(STDLIB_OBJECTS) \
	  $(IO_OBJECTS) $(PROCESS_OBJECTS) $(TIME_OBJECTS)  \
	  $(SYS_STAT_OBJECTS) $(SIGNAL_OBJECTS) $(MATH_OBJECTS) $(FLOAT_OBJECTS)


crtdll.a: $(OBJECTS)
	$(LD) -r $(OBJECTS) -o crtdll.a

dummy:

include ../../Rules.mak

