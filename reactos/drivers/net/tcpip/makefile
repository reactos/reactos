# TCPIP.SYS - TCP/IP protocol driver

TARGETNAME=tcpip

BASE_CFLAGS = -I./include -I../../../include

RESOURCE_OBJECT  = $(TARGETNAME).coff
TCPIP_OBJECTS    = tcpip/main.o tcpip/address.o tcpip/checksum.o \
                   tcpip/dispatch.o tcpip/fileobjs.o tcpip/info.o \
                   tcpip/pool.o tcpip/routines.o
DATALINK_OBJECTS = datalink/arp.o datalink/lan.o datalink/loopback.o
NETWORK_OBJECTS  = network/icmp.o network/ip.o network/neighbor.o \
                   network/receive.o network/route.o network/router.o \
                   network/transmit.o
DATAGRAM_OBJECTS = transport/datagram/datagram.o
RAWIP_OBJECTS    = transport/rawip/rawip.o
TCP_OBJECTS      = transport/tcp/tcp.o
UDP_OBJECTS      = transport/udp/udp.o

all: objects $(TARGETNAME).sys

objects:
	mkdir objects

objects/tcpip.o: $(TCPIP_OBJECTS)
	$(LD) -r -o objects/tcpip.o $(TCPIP_OBJECTS)

objects/datalink.o: $(DATALINK_OBJECTS)
	$(LD) -r -o objects/datalink.o $(DATALINK_OBJECTS)

objects/network.o: $(NETWORK_OBJECTS)
	$(LD) -r -o objects/network.o $(NETWORK_OBJECTS)

objects/datagram.o: $(DATAGRAM_OBJECTS)
	$(LD) -r -o objects/datagram.o $(DATAGRAM_OBJECTS)

objects/rawip.o: $(RAWIP_OBJECTS)
	$(LD) -r -o objects/rawip.o $(RAWIP_OBJECTS)

objects/tcp.o: $(TCP_OBJECTS)
	$(LD) -r -o objects/tcp.o $(TCP_OBJECTS)

objects/udp.o: $(UDP_OBJECTS)
	$(LD) -r -o objects/udp.o $(UDP_OBJECTS)

OBJECTS = objects/tcpip.o objects/datalink.o objects/network.o \
          objects/datagram.o objects/rawip.o objects/tcp.o objects/udp.o \
          $(RESOURCE_OBJECT) \
          ../../../ntoskrnl/ntoskrnl.a ../ndis/ndis.a

$(TARGETNAME).coff: $(TARGETNAME).rc ../../../include/reactos/resource.h

ifeq ($(DOSCLI),yes)
CLEAN_FILES = \
		*.o objects\*.o tcpip\*.o datalink\*.o network\*.o \
        transport\datagram\*.o transport\rawip\*.o \
        transport\tcp\*.o transport\udp\*.o $(TARGETNAME).coff \
        $(TARGETNAME).a junk.tmp base.tmp temp.exp \
		$(TARGETNAME).sys $(TARGETNAME).sym
else
CLEAN_FILES = \
		*.o objects/*.o tcpip/*.o datalink/*.o network/*.o \
        transport/datagram/*.o transport/rawip/*.o \
        transport/tcp/*.o transport/udp/*.o $(TARGETNAME).coff \
        $(TARGETNAME).a junk.tmp base.tmp temp.exp \
		$(TARGETNAME).sys $(TARGETNAME).sym
endif


$(TARGETNAME).sys: $(OBJECTS)
	$(DLLTOOL) \
		--dllname $(TARGETNAME).sys \
		--def $(TARGETNAME).def \
		--kill-at \
		--output-lib $(TARGETNAME).a
	$(CC) \
        -mdll \
        -specs=../../svc_specs \
        -Wl,-e,_DriverEntry@8 \
        -Wl,--base-file,base.tmp \
        -Wl,--defsym,_end=end \
        -Wl,--defsym,_edata=__data_end__ \
        -Wl,--defsym,_etext=etext \
		$(OBJECTS) \
        -o junk.tmp
	- $(RM) junk.tmp
	$(DLLTOOL) \
        --dllname $(TARGETNAME).sys \
        --base-file base.tmp \
        --output-exp temp.exp \
        --def $(TARGETNAME).edf
	- $(RM) base.tmp
	$(CC) \
        -mdll \
        -specs=../../svc_specs \
        -Wl,--image-base,0x10000 \
        -Wl,-e,_DriverEntry@8 \
        -Wl,temp.exp \
		$(OBJECTS) \
        -o $(TARGETNAME).sys
	- $(RM) temp.exp
	$(NM) --numeric-sort $(TARGETNAME).sys > $(TARGETNAME).sym

clean: $(CLEAN_FILES:%=%_clean)

$(CLEAN_FILES:%=%_clean): %_clean:
	- $(RM) $*

.PHONY: clean $(CLEAN_FILES:%=%_clean)

floppy: $(FLOPPY_DIR)/drivers/$(TARGETNAME).sys

$(FLOPPY_DIR)/drivers/$(TARGETNAME).sys: $(TARGETNAME).sys
ifeq ($(DOSCLI),yes)
	$(CP) $(TARGETNAME).sys $(FLOPPY_DIR)\drivers\$(TARGETNAME).sys
else
	$(CP) $(TARGETNAME).sys $(FLOPPY_DIR)/drivers/$(TARGETNAME).sys
endif

dist: $(DIST_DIR)/drivers/$(TARGETNAME).sys

$(DIST_DIR)/drivers/$(TARGETNAME).sys: $(TARGETNAME).sys
ifeq ($(DOSCLI),yes)
	$(CP) $(TARGETNAME).sys ..\..\$(DIST_DIR)\drivers\$(TARGETNAME).sys
else
	$(CP) $(TARGETNAME).sys ../../$(DIST_DIR)/drivers/$(TARGETNAME).sys
endif

#WITH_DEBUGGING      = yes
#WIN32_LEAN_AND_MEAN = yes
#WARNINGS_ARE_ERRORS = yes
include ../../../rules.mak
