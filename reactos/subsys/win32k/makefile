#
# WIN32K.SYS build spec
#

MAIN_OBJECTS = main/dllmain.o
MISC_OBJECTS = misc/driver.o
OBJECTS_OBJECTS = objects/bitmaps.o objects/brush.o objects/cliprgn.o  \
                  objects/color.o objects/coord.o objects/dc.o  \
                  objects/fillshap.o objects/line.o objects/text.o
RESOURCE_OBJECT = win32k.coff
STUBS_OBJECTS = stubs/stubs.o

OBJECTS = $(MAIN_OBJECTS) $(MISC_OBJECTS) $(OBJECTS_OBJECTS)  \
          $(RESOURCE_OBJECT) $(STUBS_OBJECTS) 

all: win32k.sys

win32k.coff: win32k.rc ../../include/reactos/resource.h
	$(RC) win32k.rc win32k.coff

ifeq ($(DOSCLI),yes)
CLEAN_FILES = main\*.o misc\*.o stubs\*.o objects\*.o win32k.coff win32k.o  \
              win32k.a junk.tmp base.tmp temp.exp win32k.sys win32k.sym
else
CLEAN_FILES = main/*.o misc/*.o stubs/*.o objects/*.o win32k.coff win32k.o  \
              win32k.a junk.tmp base.tmp temp.exp win32k.sys win32k.sym
endif

win32k.sys: $(OBJECTS) win32k.def
	$(LD) -r $(OBJECTS) -o win32k.o
	$(DLLTOOL) --dllname win32k.sys --def win32k.def \
	           --output-lib win32k.a
	$(CC) --subsystem=windows -mdll --dll -e _DllMain@12 -o junk.tmp \
	      -Wl,--image-base,0x0 \
	      -Wl,--file-alignment,0x1000 \
	      -Wl,--section-alignment,0x1000 \
	      -Wl,--defsym,_end=end \
	      -Wl,--defsym,_edata=__data_end__ \
	      -Wl,--defsym,_etext=etext -Wl,--base-file,base.tmp win32k.o \
              ../../ntoskrnl/ntoskrnl.a
	- $(RM) junk.tmp
	$(DLLTOOL) --dllname win32k.sys --base-file base.tmp \
	           --output-exp temp.exp --def win32k.def
	- $(RM) base.tmp
	$(CC) --subsystem=windows -mdll --dll -e _DllMain@12 \
              -o win32k.sys win32k.o ../../ntoskrnl/ntoskrnl.a \
	      -Wl,--image-base,0x0 \
	      -Wl,--file-alignment,0x1000 \
	      -Wl,--section-alignment,0x1000 \
	      -Wl,--defsym,_end=end \
	      -Wl,--defsym,_edata=__data_end__ \
	      -Wl,--defsym,_etext=etext -Wl,temp.exp
              
	- $(RM) temp.exp
	$(NM) --numeric-sort win32k.sys > win32k.sym

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.PHONY: clean $(CLEAN_FILES:%=%_clean)

#WITH_DEBUGGING = yes
WIN32_LEAN_AND_MEAN = yes
#WARNINGS_ARE_ERRORS = yes
include ../../rules.mak

