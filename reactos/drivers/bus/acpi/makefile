# $Id: makefile,v 1.7 2001/08/23 17:32:03 chorns Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = driver

TARGET_NAME = acpi

TARGET_CFLAGS = -I./include -I./ospm/include

OBJECTS_PATH = objects

TARGET_OBJECTS = $(OBJECTS_PATH)/$(TARGET_NAME).o

TARGET_CLEAN = \
  $(OBJECTS_PATH)/*.o \
  dispatcher/*.o \
  events/*.o \
  executer/*.o \
  hardware/*.o \
  namespace/*.o \
  ospm/*.o \
  ospm/busmgr/*.o \
  parser/*.o \
  resource/*.o \
  tables/*.o \
  utils/*.o


OBJECTS_OSPM_BUSMGR = \
  ospm/busmgr/bm.o \
  ospm/busmgr/bmnotify.o \
  ospm/busmgr/bmpm.o \
  ospm/busmgr/bmpower.o \
  ospm/busmgr/bmrequest.o \
  ospm/busmgr/bmsearch.o \
  ospm/busmgr/bmutils.o \
  ospm/busmgr/bmxface.o
  
OBJECTS_OSPM = \
  $(OBJECTS_OSPM_BUSMGR) \
  ospm/acpienum.o \
  ospm/acpisys.o \
  ospm/bn.o \
  ospm/fdo.o \
  ospm/osl.o \
  ospm/pdo.o

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


include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk


#
# Build intermediate objects
#
$(OBJECTS_PATH)/ospm.o: $(OBJECTS_OSPM)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/ospm.o \
		$(OBJECTS_OSPM)

$(OBJECTS_PATH)/dispatcher.o: $(OBJECTS_DISPATCHER)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/dispatcher.o \
		$(OBJECTS_DISPATCHER)

$(OBJECTS_PATH)/events.o: $(OBJECTS_EVENTS)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/events.o \
		$(OBJECTS_EVENTS)

$(OBJECTS_PATH)/executer.o: $(OBJECTS_EXECUTER)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/executer.o \
		$(OBJECTS_EXECUTER)

$(OBJECTS_PATH)/hardware.o: $(OBJECTS_HARDWARE)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/hardware.o \
		$(OBJECTS_HARDWARE)

$(OBJECTS_PATH)/namespace.o: $(OBJECTS_NAMESPACE)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/namespace.o \
		$(OBJECTS_NAMESPACE)

$(OBJECTS_PATH)/parser.o: $(OBJECTS_PARSER)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/parser.o \
		$(OBJECTS_PARSER)

$(OBJECTS_PATH)/resource.o: $(OBJECTS_RESOURCE)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/resource.o \
		$(OBJECTS_RESOURCE)

$(OBJECTS_PATH)/tables.o: $(OBJECTS_TABLES)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/tables.o \
		$(OBJECTS_TABLES)

$(OBJECTS_PATH)/utils.o: $(OBJECTS_UTILS)
	$(LD) \
		-r \
		-o $(OBJECTS_PATH)/utils.o \
		$(OBJECTS_UTILS)


OBJECTS = \
  $(OBJECTS_PATH)/ospm.o \
	$(OBJECTS_PATH)/dispatcher.o \
	$(OBJECTS_PATH)/events.o \
	$(OBJECTS_PATH)/executer.o \
	$(OBJECTS_PATH)/hardware.o \
	$(OBJECTS_PATH)/namespace.o \
	$(OBJECTS_PATH)/parser.o \
	$(OBJECTS_PATH)/resource.o \
	$(OBJECTS_PATH)/tables.o \
	$(OBJECTS_PATH)/utils.o

$(OBJECTS_PATH)/$(TARGET_NAME).o: $(OBJECTS_PATH) $(OBJECTS)
	$(LD) -r $(OBJECTS) -o $(OBJECTS_PATH)/$(TARGET_NAME).o

$(OBJECTS_PATH):
	mkdir $(OBJECTS_PATH)
