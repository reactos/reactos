# $Id: Makefile,v 1.10 2000/11/20 19:59:13 ekohl Exp $
#
# TDI.SYS build spec
#
PATH_TO_TOP = ../../../

TARGETNAME=tdi

CFLAGS = 

OBJECTS_CTE = cte/stubs.o cte/string.o
OBJECTS_TDI = tdi/stubs.o tdi/obsolete.o tdi/handler.o tdi/dereg.o

OBJECTS_MISC = misc/main.o misc/$(TARGETNAME).coff

OBJECTS = $(OBJECTS_CTE) $(OBJECTS_TDI) $(OBJECTS_MISC)

all: $(TARGETNAME).sys

misc/$(TARGETNAME).coff: misc/$(TARGETNAME).rc ../../../include/reactos/resource.h

ifeq ($(DOSCLI),yes)
CLEAN_FILES = \
		cte\*.o tdi\*.o misc\*.o misc\*.coff \
		$(TARGETNAME).o $(TARGETNAME).a \
		junk.tmp base.tmp temp.exp \
		$(TARGETNAME).sys $(TARGETNAME).sym
else
CLEAN_FILES = \
		cte/*.o tdi/*.o misc/*.o misc/*.coff \
		$(TARGETNAME).o $(TARGETNAME).a \
		junk.tmp base.tmp temp.exp \
		$(TARGETNAME).sys $(TARGETNAME).sym
endif

$(TARGETNAME).sys: $(OBJECTS) misc/$(TARGETNAME).def
	$(LD) -r $(OBJECTS) -o $(TARGETNAME).o
	$(DLLTOOL) \
		--dllname $(TARGETNAME).sys \
		--def misc/$(TARGETNAME).def \
		--output-lib $(TARGETNAME).a \
		--kill-at
	$(CC) \
		--subsystem=native \
		-mdll \
		--dll \
		-e _DllMain@8 \
		-o junk.tmp \
		-Wl,--image-base,0x0 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,--defsym,_end=end \
		-Wl,--defsym,_edata=__data_end__ \
		-Wl,--defsym,_etext=etext \
		-Wl,--base-file,base.tmp $(TARGETNAME).o \
		-specs=../../../specs \
		../../../ntoskrnl/ntoskrnl.a
	- $(RM) junk.tmp
	$(DLLTOOL) \
		--dllname $(TARGETNAME).sys \
		--base-file base.tmp \
		--output-exp temp.exp \
		--def misc/$(TARGETNAME).edf \
		--kill-at
	- $(RM) base.tmp
	$(CC) \
		--subsystem=native \
		-mdll \
		--dll \
		-e _DriverEntry@8 \
		-o $(TARGETNAME).sys \
		$(TARGETNAME).o \
		../../../ntoskrnl/ntoskrnl.a \
		-Wl,--image-base,0x0 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,--defsym,_end=end \
		-Wl,--defsym,_edata=__data_end__ \
		-Wl,--defsym,_etext=etext \
		-Wl,temp.exp \
		-specs=../../../specs
	- $(RM) temp.exp
	$(NM) --numeric-sort $(TARGETNAME).sys > $(TARGETNAME).sym

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.PHONY: clean $(CLEAN_FILES:%=%_clean)

install: $(FLOPPY_DIR)/drivers/$(TARGETNAME).sys

$(FLOPPY_DIR)/drivers/$(TARGETNAME).sys: $(TARGETNAME).sys
	$(CP) $(TARGETNAME).sys $(FLOPPY_DIR)/drivers/$(TARGETNAME).sys

dist: $(DIST_DIR)/drivers/$(TARGETNAME).sys

$(DIST_DIR)/drivers/$(TARGETNAME).sys: $(TARGETNAME).sys
	$(CP) $(TARGETNAME).sys ../../../$(DIST_DIR)/drivers/$(TARGETNAME).sys

#WITH_DEBUGGING = yes
#WIN32_LEAN_AND_MEAN = yes
#WARNINGS_ARE_ERRORS = yes
include ../../../rules.mak

