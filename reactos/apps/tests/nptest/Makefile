#
#
#
PATH_TO_TOP = ../../..

include $(PATH_TO_TOP)/rules.mak

SRV_OBJECTS = npserver.o
CLT_OBJECTS = npclient.o

PROGS = npserver.exe npclient.exe

BASE_CFLAGS = -I$(PATH_TO_TOP)/include

LIBS = $(SDK_PATH_LIB)/kernel32.a

all: $(PROGS)

.phony: all

implib:

clean:
	- $(RM) *.o *.exe *.sym

.phony: implib clean

install: $(PROGS:%=$(INSTALL_DIR)/bin/%)

$(PROGS:%=$(INSTALL_DIR)/bin/%): $(INSTALL_DIR)/bin/%: %
	$(CP) $* $(INSTALL_DIR)/bin/$*

dist: $(PROGS:%=$(DIST_DIR)/apps/%)

$(PROGS:%=$(DIST_DIR)/apps/%): $(DIST_DIR)/apps/%: %
	$(CP) $* $(DIST_DIR)/apps/$*

npserver.exe: $(SRV_OBJECTS) $(LIBS)
	$(CC) $(SRV_OBJECTS) $(LIBS) -o npserver.exe
	$(NM) --numeric-sort npserver.exe > npserver.sym

npclient.exe: $(CLT_OBJECTS) $(LIBS)
	$(CC) $(CLT_OBJECTS) $(LIBS) -o npclient.exe
	$(NM) --numeric-sort npclient.exe > npclient.sym
