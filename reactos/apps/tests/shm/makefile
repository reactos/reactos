#
#
#
PATH_TO_TOP = ../../..

include $(PATH_TO_TOP)/rules.mak

SRV_OBJECTS= shmsrv.o
CLT_OBJECTS= shmclt.o

PROGS= shmsrv.exe shmclt.exe

BASE_CFLAGS = -I$(PATH_TO_TOP)/include

LIBS = $(SDK_PATH_LIB)/kernel32.a \
  $(SDK_PATH_LIB)/ntdll.a

all: $(PROGS)

.phony: all

implib:

clean:
	- $(RM) *.o *.exe *.sym

.phony: implib clean

ifneq ($(BOOTCD_INSTALL),)

install: $(PROGS:%=$(INSTALL_DIR)/%)

$(PROGS:%=$(INSTALL_DIR)/%): $(INSTALL_DIR)/%: %
	$(CP) $* $(INSTALL_DIR)/$*

else # BOOTCD_INSTALL

install: $(PROGS:%=$(INSTALL_DIR)/bin/%)

$(PROGS:%=$(INSTALL_DIR)/bin/%): $(INSTALL_DIR)/bin/%: %
	$(CP) $* $(INSTALL_DIR)/bin/$*

endif # BOOTCD_INSTALL

dist: $(PROGS:%=$(DIST_DIR)/apps/%)

$(PROGS:%=$(DIST_DIR)/apps/%): $(DIST_DIR)/apps/%: %
	$(CP) $* $(DIST_DIR)/apps/$*

shmsrv.exe: $(SRV_OBJECTS) $(LIBS)
	$(CC) $(SRV_OBJECTS) $(LIBS) -o shmsrv.exe
	$(NM) --numeric-sort shmsrv.exe > shmsrv.sym

shmclt.exe: $(CLT_OBJECTS) $(LIBS)
	$(CC) $(CLT_OBJECTS) $(LIBS) -o shmclt.exe
	$(NM) --numeric-sort shmsrv.exe > shmclt.sym

# EOF
