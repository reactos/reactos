ifneq ($(HOST),mingw32-windows)
  ifneq ($(HOST),mingw32-linux)
    DLLTARGET=crtdll.a
  else
    DLLTARGET=crtdll.dll
  endif
else
  DLLTARGET=crtdll.dll
endif

all: $(DLLTARGET)


ASSERT_OBJECTS = assert/assert.o

CTYPE_OBJECTS = ctype/isalnum.o \
		ctype/isalpha.o ctype/isascii.o ctype/iscntrl.o ctype/isdigit.o ctype/isgraph.o \
		ctype/islower.o ctype/isprint.o ctype/ispunct.o ctype/isspace.o ctype/isupper.o \
		ctype/isxdigit.o ctype/toascii.o ctype/tolower.o ctype/toupper.o\
		ctype/iscsym.o ctype/isctype.o

CONIO_OBJECTS = conio/cputs.o conio/getch.o conio/getche.o conio/putch.o conio/ungetch.o

DIRECT_OBJECTS = direct/chdir.o direct/chdrive.o direct/getcwd.o direct/getdrive.o \
		 direct/rmdir.o direct/mkdir.o direct/getdfree.o

MALLOC_OBJECTS = malloc/expand.o malloc/heap.o

#MISC_OBJECTS =   misc/GetArgs.o misc/setnew.o misc/purecall.o
MISC_OBJECTS = misc/GetArgs.o

STRING_OBJECTS = string/memchr.o string/memcmp.o string/strcat.o \
                 string/strchr.o string/strcmp.o string/strcoll.o \
		 string/strcpy.o string/strcspn.o string/memcpy.o \
		 string/strlen.o string/strncat.o string/strncmp.o \
		 string/strncpy.o string/strpbrk.o string/strrchr.o \
		 string/strspn.o string/strstr.o string/strtok.o \
		 string/strxfrm.o string/memmove.o string/memset.o \
		 string/strdup.o string/strlwr.o string/strupr.o \
		 string/str_old.o string/strerror.o string/stricmp.o\
		 string/strnlen.o string/strnicmp.o

WCHAR_OBJECTS = wchar/wcscat.o wchar/wcschr.o wchar/wcscmp.o \
		 wchar/wcscoll.o wchar/wcscpy.o wchar/wcscspn.o \
		 wchar/wcsdup.o wchar/wcsicmp.o wchar/wcslen.o \
		 wchar/wcslwr.o wchar/wcsncat.o wchar/wcsncmp.o \
		 wchar/wcsncpy.o  

STDIO_OBJECTS = stdio/getenv.o stdio/filbuf.o  \
		stdio/fclose.o stdio/feof.o stdio/ferror.o stdio/fileno.o\
		stdio/fflush.o stdio/fgetc.o stdio/fgetpos.o stdio/fgets.o stdio/flsbuf.o \
		stdio/fopen.o stdio/fprintf.o stdio/fputc.o stdio/fputs.o stdio/setvbuf.o\
		stdio/fread.o stdio/freopen.o  stdio/fscanf.o stdio/fseek.o \
		stdio/fsetpos.o stdio/ftell.o stdio/fwalk.o stdio/fwrite.o stdio/getc.o \
		stdio/getchar.o stdio/gets.o stdio/getw.o stdio/perror.o stdio/clearerr.o \
		stdio/putc.o stdio/putchar.o stdio/puts.o stdio/putw.o \
		stdio/remove.o stdio/rename.o stdio/rewind.o stdio/allocfil.o\
		stdio/setbuf.o stdio/setbuffe.o stdlib/obsol.o stdio/setlineb.o\
	 	stdio/scanf.o  stdio/sscanf.o stdio/vscanf.o stdio/vsscanf.o stdio/vfscanf.o\
		stdio/stdiohk.o stdio/stdhnd.o stdio/tempnam.o stdio/tmpfile.o stdio/tmpnam.o \
		stdio/ungetc.o stdio/printf.o stdio/vfprintf.o stdio/vprintf.o stdio/sprintf.o\
		stdio/fdopen.o stdio/vsprintf.o stdio/frlist.o \
 
QUAD_OBJECTS =  quad/qdivrem.o quad/divdi3.o quad/moddi3.o quad/udivdi3.o quad/umoddi3.o

IO_OBJECTS = io/access.o io/close.o io/create.o io/dup.o io/dup2.o io/find.o io/isatty.o io/lseek.o \
		io/open.o io/read.o io/setmode.o io/unlink.o io/write.o io/fmode.o io/mktemp.o\
		io/chmod.o io/chsize.o io/commit.o io/locking.o io/pipe.o io/sopen.o io/filelen.o\
		io/umask.o io/tell.o io/eof.o io/utime.o

SEARCH_OBJECTS = search/lsearch.o search/lfind.o

STDLIB_OBJECTS = stdlib/abort.o stdlib/abs.o stdlib/atexit.o stdlib/atof.o stdlib/atoi.o \
		 stdlib/bsearch.o stdlib/div.o stdlib/errno.o  stdlib/_exit.o \
		 stdlib/fullpath.o stdlib/labs.o stdlib/ldiv.o \
		 stdlib/makepath.o stdlib/malloc.o stdlib/putenv.o stdlib/qsort.o \
		 stdlib/rand.o stdlib/senv.o stdlib/splitp.o stdlib/strtod.o stdlib/strtol.o \
		 stdlib/strtoul.o stdlib/swab.o stdlib/atol.o 

SIGNAL_OBJECTS = signal/signal.o

PROCESS_OBJECTS = process/_cwait.o process/dll.o process/spawnl.o process/spawnlp.o process/spawnlpe.o  process/spawnvpe.o process/spawnvp.o \
		process/spawnv.o process/spawnve.o process/spawnle.o process/execl.o process/execlp.o process/execlpe.o  \
		process/execvpe.o process/execvp.o process/execv.o process/execle.o process/_system.o\
		process/threadid.o

TCHAR_OBJECTS = tchar/strdec.o tchar/strinc.o tchar/strninc.o  tchar/strncnt.o tchar/strnextc.o tchar/strspnp.o

TIME_OBJECTS = time/ctime.o time/difftime.o time/strftime.o time/time.o time/clock.o

FLOAT_OBJECTS = float/fpreset.o float/clearfp.o float/cntrlfp.o float/statfp.o float/logb.o\
		float/chgsign.o float/fpclass.o float/isnan.o

SYS_STAT_OBJECTS = sys_stat/fstat.o sys_stat/stat.o sys_stat/futime.o



MATH_OBJECTS = math/acos.o math/acosh.o math/asin.o math/asinh.o math/atan.o math/atan2.o\
		math/atanh.o math/ceil.o math/cos.o math/cosh.o math/exp.o math/fabs.o\
		math/floor.o math/fmod.o math/frexp.o math/huge_val.o math/hypot.o\
		math/ldexp.o math/log.o math/log10.o math/modf.o math/pow.o\
		math/sin.o math/sinh.o math/sqrt.o math/tan.o\
		math/tanh.o math/stubs.o math/j0_y0.o math/j1_y1.o math/jn_yn.o


#OBJECTS = $(ASSERT_OBJECTS) $(CTYPE_OBJECTS) $(CONIO_OBJECTS) $(DIRECT_OBJECTS)\
#	  $(MISC_OBJECTS) $(STRING_OBJECTS)  $(STDIO_OBJECTS) $(STDLIB_OBJECTS) \
#	  $(IO_OBJECTS)  $(PROCESS_OBJECTS) $(TIME_OBJECTS)  $(MALLOC_OBJECTS)\
#	  $(SYS_STAT_OBJECTS) $(SIGNAL_OBJECTS) $(MATH_OBJECTS) $(FLOAT_OBJECTS)\
#	  $(SEARCH_OBJECTS) $(QUAD_OBJECTS)


OLD_OBJECTS = $(MISC_OBJECTS) stdlib/malloc.o stdlib/abort.o \
	  stdlib/_exit.o stdlib/atexit.o stdio/fileno.o io/fmode.o \
	  float/fpreset.o stdio/stdhnd.o io/setmode.o io/open.o \
	  stdio/vsprintf.o $(CTYPE_OBJECTS) stdlib/atoi.o stdlib/strtol.o \
	  stdio/flsbuf.o stdio/putc.o stdio/vfprintf.o $(STRING_OBJECTS)\
	  io/write.o io/isatty.o sys_stat/fstat.o misc/dllmain.o \
	  stdlib/errno.o stdio/printf.o stdio/vprintf.o\
	  $(QUAD_OBJECTS)

RESOURCE_OBJECT = crtdll.coff

OBJECTS = $(MISC_OBJECTS) $(STDLIB_OBJECTS) $(IO_OBJECTS) \
	  $(FLOAT_OBJECTS) $(ASSERT_OBJECTS) $(PROCESS_OBJECTS) \
	  $(STDIO_OBJECTS) $(CTYPE_OBJECTS) $(MATH_OBJECTS) \
	  $(STRING_OBJECTS) $(TIME_OBJECTS) $(WCHAR_OBJECTS) \
	  $(SYS_STAT_OBJECTS) misc/dllmain.o $(MALLOC_OBJECTS) \
	  $(SEARCH_OBJECTS) $(CONIO_OBJECTS) $(DIRECT_OBJECTS) \
	  $(SIGNAL_OBJECTS) $(RESOURCE_OBJECT)

ifeq ($(DOSCLI), yes)
CLEAN_FILES = assert\*.o conio\*.o ctype\*.o direct\*.o dirent\*.o \
	except\*.o float\*.o io\*.o libc\*.o locale\*.o malloc\*.o \
	math\*.o mbstring\*.o misc\*.o process\*.o quad\*.o search\*.o \
	setjmp\*.o stdio\*.o stdlib\*.o string\*.o sys_stat\*.o tchar\*.o \
	time\*.o wchar\*.o crtdll.a crtdll.o crtdll.dll crtdll.sym
else
CLEAN_FILES = assert/*.o conio/*.o ctype/*.o direct/*.o dirent/*.o \
	except/*.o float/*.o io/*.o libc/*.o locale/*.o malloc/*.o \
	math/*.o mbstring/*.o misc/*.o process/*.o quad/*.o search/*.o \
	setjmp/*.o stdio/*.o stdlib/*.o string/*.o sys_stat/*.o tchar/*.o \
	time/*.o wchar/*.o crtdll.a crtdll.o crtdll.dll crtdll.sym
endif

crtdll.coff: crtdll.rc ../../include/reactos/resource.h
	windres crtdll.rc crtdll.coff

crtdll.a: $(OBJECTS)
	$(LD)  -r $(OBJECTS) -o crtdll.a

crtdll.dll: $(DLLMAIN) $(OBJECTS) crtdll.def
	$(LD) -r $(OBJECTS) -o crtdll.o
	$(DLLTOOL) --dllname crtdll.dll --def crtdll.def \
	           --output-lib crtdll.a
	$(CC) -specs=crt_specs -mdll -o junk.tmp \
	      -Wl,--base-file,base.tmp crtdll.o  ../kernel32/kernel32.a 
	- $(RM) junk.tmp
	$(DLLTOOL) --dllname crtdll.dll --base-file base.tmp \
	           --output-exp temp.exp --def crtdll.def
	- $(RM) base.tmp
	$(CC) -specs=crt_specs -mdll -o crtdll.dll crtdll.o  \
	       ../kernel32/kernel32.a \
	      -Wl,--image-base,0x20000000 \
	      -Wl,--file-alignment,0x1000 \
	      -Wl,--section-alignment,0x1000 \
	      -Wl,temp.exp
	- $(RM) temp.exp
	$(NM) --numeric-sort crtdll.dll > crtdll.sym

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*


include ../../rules.mak

