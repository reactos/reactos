
ifneq ($(HOST),mingw32-windows)
  ifneq ($(HOST),mingw32-linux)
    DLLTARGET=ntdll.a
    DLLMAIN=
  else
    DLLTARGET=ntdll.dll
    DLLMAIN=main/dllmain.o
  endif
else
  DLLTARGET=ntdll.dll
  DLLMAIN=main/dllmain.o
endif

all: $(DLLTARGET)

STDLIB_OBJECTS = stdlib/atoi.o stdlib/atol.o stdlib/splitp.o \
                 stdlib/strtol.o stdlib/strtoul.o

STRING_OBJECTS = string/ctype.o string/memcpy.o string/memmove.o \
                 string/memset.o string/strcat.o string/strchr.o \
                 string/strcmp.o string/strcspn.o \
                 string/strcpy.o string/stricmp.o string/strlen.o \
                 string/strncat.o string/strncmp.o string/strncpy.o \
                 string/strnicmp.o string/strnlen.o string/strrchr.o \
                 string/strstr.o string/strupr.o string/wstring.o

OBJECTS = napi.o ldr/startup.o rtl/largeint.o rtl/namespc.o rtl/unicode.o \
          stdio/vsprintf.o $(STDLIB_OBJECTS) $(STRING_OBJECTS) \
          stubs/stubs.o rtl/heap.o rtl/critical.o rtl/mem.o \
          ldr/utils.o

ifeq ($(DOSCLI),yes)
CLEAN_FILES = napi.o ldr\*.o rtl\*.o stdio\*.o string\*.o stubs\*.o ntdll.a  \
              main\dllmain.o ntdll.o ntdll.dll junk.tmp base.tmp temp.exp ntdll.sym
else
CLEAN_FILES = napi.o ldr/*.o rtl/*.o stdio/*.o string/*.o stubs/*.o ntall.a  \
              main/dllmain.o ntdll.o ntdll.dll junk.tmp base.tmp temp.exp ntdll.sym
endif

ntdll.a: $(OBJECTS)
	$(AR) csr ntdll.a $(OBJECTS)

ntdll.dll: $(DLLMAIN) $(OBJECTS) 
	$(LD) -r $(DLLMAIN) $(OBJECTS) -o ntdll.o
	$(DLLTOOL) --dllname ntdll.dll --def def/ntdll.def \
	           --output-lib ntdll.a
	$(CC) -specs=ntdll_specs -mdll -o junk.tmp \
	      -Wl,--base-file,base.tmp ntdll.o
	- $(RM) junk.tmp
	$(DLLTOOL) --dllname ntdll.dll --base-file base.tmp \
	           --output-exp temp.exp --def def/ntdll.def
	- $(RM) base.tmp
	$(CC) -specs=ntdll_specs -mdll -o ntdll.dll ntdll.o \
	      -Wl,--entry=_LdrStartup \
	      -Wl,--image-base,0x80000000 \
	      -Wl,--file-alignment,0x1000 \
	      -Wl,--section-alignment,0x1000 \
	      -Wl,temp.exp
	- $(RM) temp.exp
	$(NM) --numeric-sort ntdll.dll > ntdll.sym

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.PHONY: clean $(CLEAN_FILES:%=%_clean)

#WARNINGS_ARE_ERRORS = yes
include ../../rules.mak

