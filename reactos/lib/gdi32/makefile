
ifneq ($(HOST),mingw32-windows)
  ifneq ($(HOST),mingw32-linux)
    DLLTARGET=gdi32.a
  else
    DLLTARGET=gdi32.dll
  endif
else
  DLLTARGET=gdi32.dll
endif

all: $(DLLTARGET)

MAIN_OBJECTS = main/dllmain.o

OBJECTS = $(MAIN_OBJECTS)

gdi32.a: $(OBJECTS)
        $(AR) csr gdi32.a $(OBJECTS)

gdi32.dll: $(DLLMAIN) $(OBJECTS) gdi32.def
        $(LD) -r $(OBJECTS) -o gdi32.o
        $(DLLTOOL) --dllname gdi32.dll --def gdi32.def \
                   --output-lib gdi32.a
	$(CC) -specs=k32_specs -mdll -o junk.tmp \
              -Wl,--base-file,base.tmp gdi32.o  ../ntdll/ntdll.a
	- $(RM) junk.tmp
        $(DLLTOOL) --dllname gdi32.dll --base-file base.tmp \
                   --output-exp temp.exp --def gdi32.def
	- $(RM) base.tmp
        $(CC) -specs=k32_specs -mdll -o gdi32.dll gdi32.o  ../ntdll/ntdll.a\
	      -Wl,--image-base,0x70000000 \
	      -Wl,--file-alignment,0x1000 \
	      -Wl,--section-alignment,0x1000 \
	      -Wl,temp.exp
	- $(RM) temp.exp
        $(NM) --numeric-sort gdi32.dll > gdi32.sym

include ../../rules.mak
