PATH_TO_TOP = ../../..

TARGET=acpi

CFLAGS = -I./include -I./ospm/include -g

LIBS = $(PATH_TO_TOP)/ntoskrnl/ntoskrnl.a

OBJECTS_PATH = objects


OBJECTS_OSPM = \
  ospm/acpienum.o \
  ospm/acpisys.o \
  ospm/osl.o

OBJECTS_DISPATCHER = \
  dispatcher/dsfield.o \
  dispatcher/dsmethod.o \
  dispatcher/dsmthdat.o \
  dispatcher/dsobject.o \
  dispatcher/dsopcode.o \
  dispatcher/dsutils.o \
  dispatcher/dswexec.o \
  dispatcher/dswload.o \
  dispatcher/dswscope.o \
  dispatcher/dswstate.o

OBJECTS_EVENTS = \
  events/evevent.o \
  events/evmisc.o \
  events/evregion.o \
  events/evrgnini.o \
  events/evsci.o \
  events/evxface.o \
  events/evxfevnt.o \
  events/evxfregn.o

OBJECTS_EXECUTER = \
  executer/amconfig.o \
  executer/amconvrt.o \
  executer/amcreate.o \
  executer/amdump.o \
  executer/amdyadic.o \
  executer/amfield.o \
  executer/amfldio.o \
  executer/ammisc.o \
  executer/ammonad.o \
  executer/ammutex.o \
  executer/amnames.o \
  executer/amprep.o \
  executer/amregion.o \
  executer/amresnte.o \
  executer/amresolv.o \
  executer/amresop.o \
  executer/amstore.o \
  executer/amstoren.o \
  executer/amstorob.o \
  executer/amsystem.o \
  executer/amutils.o \
  executer/amxface.o

OBJECTS_HARDWARE = \
  hardware/hwacpi.o \
  hardware/hwgpe.o \
  hardware/hwregs.o \
  hardware/hwsleep.o \
  hardware/hwtimer.o

OBJECTS_NAMESPACE = \
  namespace/nsaccess.o \
  namespace/nsalloc.o \
  namespace/nseval.o \
  namespace/nsinit.o \
  namespace/nsload.o \
  namespace/nsnames.o \
  namespace/nsobject.o \
  namespace/nssearch.o \
  namespace/nsutils.o \
  namespace/nswalk.o \
  namespace/nsxfname.o \
  namespace/nsxfobj.o

OBJECTS_PARSER = \
  parser/psargs.o \
  parser/psopcode.o \
  parser/psparse.o \
  parser/psscope.o \
  parser/pstree.o \
  parser/psutils.o \
  parser/pswalk.o \
  parser/psxface.o

OBJECTS_RESOURCE = \
  resource/rsaddr.o \
  resource/rscalc.o \
  resource/rscreate.o \
  resource/rsdump.o \
  resource/rsio.o \
  resource/rsirq.o \
  resource/rslist.o \
  resource/rsmemory.o \
  resource/rsmisc.o \
  resource/rsutils.o \
  resource/rsxface.o

OBJECTS_TABLES = \
  tables/tbconvrt.o \
  tables/tbget.o \
  tables/tbinstal.o \
  tables/tbutils.o \
  tables/tbxface.o \
  tables/tbxfroot.o

OBJECTS_UTILS = \
  utils/cmalloc.o \
  utils/cmclib.o \
  utils/cmcopy.o \
  utils/cmdebug.o \
  utils/cmdelete.o \
  utils/cmeval.o \
  utils/cmglobal.o \
  utils/cminit.o \
  utils/cmobject.o \
  utils/cmutils.o \
  utils/cmxface.o


all: $(TARGET).sys

.phony: all


$(OBJECTS_PATH):
	mkdir $(OBJECTS_PATH)


#
# Build intermediate objects
#
$(OBJECTS_PATH)/ospm.o: $(OBJECTS_PATH) $(OBJECTS_OSPM)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/ospm.o \
		$(OBJECTS_OSPM)

$(OBJECTS_PATH)/dispatcher.o: $(OBJECTS_PATH) $(OBJECTS_DISPATCHER)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/dispatcher.o \
		$(OBJECTS_DISPATCHER)

$(OBJECTS_PATH)/events.o: $(OBJECTS_PATH) $(OBJECTS_EVENTS)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/events.o \
		$(OBJECTS_EVENTS)

$(OBJECTS_PATH)/executer.o: $(OBJECTS_PATH) $(OBJECTS_EXECUTER)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/executer.o \
		$(OBJECTS_EXECUTER)

$(OBJECTS_PATH)/hardware.o: $(OBJECTS_PATH) $(OBJECTS_HARDWARE)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/hardware.o \
		$(OBJECTS_HARDWARE)

$(OBJECTS_PATH)/namespace.o: $(OBJECTS_PATH) $(OBJECTS_NAMESPACE)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/namespace.o \
		$(OBJECTS_NAMESPACE)

$(OBJECTS_PATH)/parser.o: $(OBJECTS_PATH) $(OBJECTS_PARSER)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/parser.o \
		$(OBJECTS_PARSER)

$(OBJECTS_PATH)/resource.o: $(OBJECTS_PATH) $(OBJECTS_RESOURCE)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/resource.o \
		$(OBJECTS_RESOURCE)

$(OBJECTS_PATH)/tables.o: $(OBJECTS_PATH) $(OBJECTS_TABLES)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/tables.o \
		$(OBJECTS_TABLES)

$(OBJECTS_PATH)/utils.o: $(OBJECTS_PATH) $(OBJECTS_UTILS)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/utils.o \
		$(OBJECTS_UTILS)


OBJECTS := \
  $(OBJECTS_PATH)/ospm.o \
	$(OBJECTS_PATH)/dispatcher.o \
	$(OBJECTS_PATH)/events.o \
	$(OBJECTS_PATH)/executer.o \
	$(OBJECTS_PATH)/hardware.o \
	$(OBJECTS_PATH)/namespace.o \
	$(OBJECTS_PATH)/parser.o \
	$(OBJECTS_PATH)/resource.o \
	$(OBJECTS_PATH)/tables.o \
	$(OBJECTS_PATH)/utils.o \
	$(TARGET).coff


$(TARGET).coff: $(TARGET).rc $(PATH_TO_TOP)/include/reactos/resource.h

$(TARGET).sys: $(OBJECTS_PATH) $(OBJECTS) $(LIBS)
	$(CC) \
		-nostartfiles -nostdlib \
		--subsystem=native \
		-mdll \
		--dll \
    -Wl,-e,_DriverEntry@8 \
    -Wl,--base-file,base.tmp \
    -Wl,--defsym,_end=end \
    -Wl,--defsym,_edata=__data_end__ \
    -Wl,--defsym,_etext=etext \
    $(OBJECTS) $(LIBS) \
    -o junk.tmp
	- $(RM) junk.tmp
	$(DLLTOOL) \
    --dllname $(TARGET).sys \
    --base-file base.tmp \
    --output-exp temp.exp \
    --kill-at
	- $(RM) base.tmp
	$(CC) \
		-nostartfiles -nostdlib \
		--subsystem=native \
		-mdll \
		--dll \
    -Wl,--image-base,0x10000 \
    -Wl,-e,_DriverEntry@8 \
    -Wl,temp.exp \
    -o $(TARGET).sys \
    $(OBJECTS) $(LIBS)
	- $(RM) temp.exp
	- $(NM) --numeric-sort $(TARGET).sys > $(TARGET).sym

#	$(STRIP) --strip-debug $(TARGET).sys


ifeq ($(DOSCLI),yes)
CLEAN_FILES = $(OBJECTS_PATH)\*.o dispatcher\*.o events\*.o executer\*.o hardware\*.o \
              namespace\*.o parser\*.o resource\*.o tables\*.o utils\*.o \
              $(TARGET).o $(TARGET).a junk.tmp base.tmp temp.exp \
              $(TARGET).sys $(TARGET).sym $(TARGET).coff
else
CLEAN_FILES = $(OBJECTS_PATH)/*.o dispatcher/*.o events/*.o executer/*.o hardware/*.o \
              namespace/*.o parser/*.o resource/*.o tables/*.o utils/*.o \
              $(TARGET).o $(TARGET).a junk.tmp base.tmp temp.exp \
              $(TARGET).sys $(TARGET).sym $(TARGET).coff
endif


clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.PHONY: clean $(CLEAN_FILES:%=%_clean)


install: $(FLOPPY_DIR)/drivers/$(TARGET).sys

$(FLOPPY_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys $(FLOPPY_DIR)/drivers/$(TARGET).sys

dist: $(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGET).sys

$(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGET).sys: $(TARGET).sys
	$(CP) $(TARGET).sys $(PATH_TO_TOP)/$(DIST_DIR)/drivers/$(TARGET).sys

WITH_DEBUGGING=yes
WARNINGS_ARE_ERRORS = yes

include $(PATH_TO_TOP)/rules.mak
