#
#
#
PATH_TO_TOP = ../../..

include $(PATH_TO_TOP)/rules.mak

SRV_OBJECTS = npserver.o
CLT_OBJECTS = npclient.o

PROGS = npserver.exe npclient.exe

CFLAGS = $(STD_CFLAGS)

LIBS = $(SDK_PATH_LIB)/kernel32.a

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

npserver.exe: $(SRV_OBJECTS) $(LIBS)
	$(CC) $(SRV_OBJECTS) $(LIBS) -o npserver.exe
	$(NM) --numeric-sort npserver.exe > npserver.sym

npclient.exe: $(CLT_OBJECTS) $(LIBS)
	$(CC) $(CLT_OBJECTS) $(LIBS) -o npclient.exe
	$(NM) --numeric-sort npclient.exe > npclient.sym
