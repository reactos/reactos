#
#
#
OBJECTS = ../common/crt0.o hello.o
LIBS = ../../lib/kernel32/kernel32.a ../../lib/ntdll/ntdll.a
CLEAN_FILES = hello.o hello.exe

all: hello.exe

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.phony: clean $(CLEAN_FILES:%=%_clean)

floppy: $(FLOPPY_DIR)/apps/hello.exe

$(FLOPPY_DIR)/apps/hello.exe: hello.exe
ifeq ($(DOSCLI),yes)
	$(CP) hello.exe $(FLOPPY_DIR)\apps\hello.exe
else
	$(CP) hello.exe $(FLOPPY_DIR)/apps\hello.exe
endif

hello.exe: $(OBJECTS) $(LIBS)
	$(LD) $(OBJECTS) $(LIBS) -o hello.exe

include ../../rules.mak
