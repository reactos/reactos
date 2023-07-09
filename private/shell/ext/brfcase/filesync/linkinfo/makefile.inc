#
# Copy shared source code.
#

doit: init.c serial.c memmgr.c valid.c comc.c inifile.c resstr.c debug.c

SHARED_DIR=..\core

init.c: $(SHARED_DIR)\init.c
   copy $(SHARED_DIR)\init.c

serial.c: $(SHARED_DIR)\serial.c
   copy $(SHARED_DIR)\serial.c

memmgr.c: $(SHARED_DIR)\memmgr.c
   copy $(SHARED_DIR)\memmgr.c

valid.c: $(SHARED_DIR)\valid.c
   copy $(SHARED_DIR)\valid.c

comc.c: $(SHARED_DIR)\comc.c
   copy $(SHARED_DIR)\comc.c

inifile.c: $(SHARED_DIR)\inifile.c
   copy $(SHARED_DIR)\inifile.c

resstr.c: $(SHARED_DIR)\resstr.c
   copy $(SHARED_DIR)\resstr.c

debug.c: $(SHARED_DIR)\debug.c
   copy $(SHARED_DIR)\debug.c
