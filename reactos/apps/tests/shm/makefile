#
#
#
PATH_TO_TOP = ../..

include $(PATH_TO_TOP)/rules.mak

SRV_OBJECTS= shmsrv.o
CLT_OBJECTS= shmclt.o

PROGS= shmsrv.exe shmclt.exe

BASE_CFLAGS = -I$(PATH_TO_TOP)/include

LIBS = $(SDK_PATH_LIB)/crtdll.a \
  $(SDK_PATH_LIB)/kernel32.a \
  $(SDK_PATH_LIB)/ntdll.a

all: $(PROGS)

.phony: all

clean:
	- $(RM) *.o *.exe *.sym

.phony: clean

install: $(PROGS:%=$(INSTALL_DIR)/bin/%)

$(PROGS:%=$(INSTALL_DIR)/bin/%): $(INSTALL_DIR)/bin/%: %
	$(CP) $* $(INSTALL_DIR)/bin/$*

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
