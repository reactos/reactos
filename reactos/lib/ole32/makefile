# ReactOS 0.17
# Jurgen Van Gael [jurgen.vangael@student.kuleuven.ac.be]
#
# Makefile for ReactOS ole32.dll
#
PATH_TO_TOP = ../..

CFLAGS = -fno-rtti -I../../include/

TARGETNAME=ole32

DLLTARGET=$(TARGETNAME).dll


OBJECTS=\
	DllMain.o \
	CoXxx.o \
	Moniker.o \
	Ole2.o \
	OleAuto.o \
	Misc.o \
	Storage.o


CLEAN_FILES = *.o \
              $(TARGETNAME).o $(TARGETNAME).a junk.tmp base.tmp temp.exp \
	      $(TARGETNAME).dll $(TARGETNAME).sym

all: $(DLLTARGET)

$(TARGETNAME).a: $(OBJECTS)
	$(LD) -r $(OBJECTS) -o $(TARGETNAME).a

$(TARGETNAME).dll: $(DLLMAIN) $(OBJECTS) $(TARGETNAME).def
	$(LD) -r $(OBJECTS) -o $(TARGETNAME).o
	$(DLLTOOL) \
		--dllname $(TARGETNAME).dll \
		--def $(TARGETNAME).def \
		--kill-at \
		--output-lib $(TARGETNAME).a
	$(CC) \
		$(TARGETNAME).o \
		../kernel32/kernel32.a \
		-nostartfiles \
		-nostdlib \
		-mdll \
		-o junk.tmp \
		-Wl,--entry=_DllMain@12\
		-Wl,--base-file,base.tmp 
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGETNAME).dll \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def $(TARGETNAME).edf
	- $(RM) base.tmp
	$(CC) \
		$(TARGETNAME).o \
		../kernel32/kernel32.a \
		-nostartfiles \
		-nostdlib \
		-mdll \
		-o $(TARGETNAME).dll \
		-Wl,--entry=_DllMain@12\
		-Wl,--image-base,0x77A50000 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,temp.exp
	- $(RM) temp.exp
	$(NM) --numeric-sort $(TARGETNAME).dll > $(TARGETNAME).sym


clean: 
	- $(RM) $(CLEAN_FILES)

.PHONY: clean 

include ../../rules.mak
