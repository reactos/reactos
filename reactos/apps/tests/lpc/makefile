#
#
#
PATH_TO_TOP = ../../..

include $(PATH_TO_TOP)/rules.mak

SRV_OBJECTS= lpcsrv.o
CLT_OBJECTS= lpcclt.o

PROGS= lpcsrv.exe lpcclt.exe

BASE_CFLAGS = -I$(PATH_TO_TOP)/include

LIBS = $(SDK_PATH_LIB)/kernel32.a \
  $(SDK_PATH_LIB)/ntdll.a

all: $(PROGS)

.phony: all

implib:

clean:
	- $(RM) lpcsrv.o lpcsrv.exe lpcsrv.sym lpcclt.o lpcclt.exe lpcsrv.sym
	
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

lpcsrv.exe: $(SRV_OBJECTS) $(LIBS)
	$(CC) $(SRV_OBJECTS) $(LIBS) -o lpcsrv.exe
	$(NM) --numeric-sort lpcsrv.exe > lpcsrv.sym

lpcclt.exe: $(CLT_OBJECTS) $(LIBS)
	$(CC) $(CLT_OBJECTS) $(LIBS) -o lpcclt.exe
	$(NM) --numeric-sort lpcclt.exe > lpcclt.sym
