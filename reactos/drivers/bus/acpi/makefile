# $Id$

PATH_TO_TOP = ../../..

TARGET_TYPE = driver

TARGET_NAME = acpi

TARGET_CFLAGS = -I./include -I./ospm/include

TARGET_DDKLIBS = ntoskrnl.a

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

TARGET_OBJECTS = \
  $(OBJECTS_OSPM) $(OBJECTS_DISPATCHER) $(OBJECTS_EVENTS) \
  $(OBJECTS_EXECUTER) $(OBJECTS_HARDWARE) $(OBJECTS_NAMESPACE) \
  $(OBJECTS_PARSER) $(OBJECTS_RESOURCE) $(OBJECTS_TABLES) \
  $(OBJECTS_UTILS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
