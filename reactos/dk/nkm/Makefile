# $Id: Makefile,v 1.2 2002/06/19 15:45:48 robd Exp $
#
# PROJECT    : ReactOS Operating System
# FILE       : reactos/dk/nkm/Makefile
# DESCRIPTION: Build the development kit for kernel mode modules
# DESCRIPTION: and user mode native applications (subsystem 1).
# LICENSE    : GNU GPL V2.0
#
PATH_TO_TOP=../..

include $(PATH_TO_TOP)/rules.mak


TARGET_FOLDER_LIB=lib
TARGET_FOLDER_INC=include

DTFLAGS = -k -l $@

MODULES_CORE=\
	$(TARGET_FOLDER_LIB)/ntoskrnl.a
#	$(TARGET_FOLDER_LIB)/hal.a

MODULES_DISPLAY=\
	$(TARGET_FOLDER_LIB)/vgaddi.a\
	$(TARGET_FOLDER_LIB)/vidport.a

MODULES_NET=\
	$(TARGET_FOLDER_LIB)/afd.a\
	$(TARGET_FOLDER_LIB)/ndis.a\
	$(TARGET_FOLDER_LIB)/packet.a\
	$(TARGET_FOLDER_LIB)/tcpip.a

MODULES_STORAGE=\
	$(TARGET_FOLDER_LIB)/class2.a\
	$(TARGET_FOLDER_LIB)/scsiport.a

MODULES_NATIVE=\
	$(TARGET_FOLDER_LIB)/ntdll.a\
	$(TARGET_FOLDER_LIB)/fmifs.a

MODULES =\
	$(MODULES_CORE)\
	$(MODULES_DISPLAY)\
	$(MODULES_NET)\
	$(MODULES_STORAGE)\
	$(MODULES_NATIVE)


all: $(MODULES)

# --- CORE ---

$(TARGET_FOLDER_LIB)/ntoskrnl.a: $(PATH_TO_TOP)/ntoskrnl/ntoskrnl.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D ntoskrnl.exe\
		-d $(PATH_TO_TOP)/ntoskrnl/ntoskrnl.def

$(TARGET_FOLDER_LIB)/hal.a: $(PATH_TO_TOP)/ntoskrnl/hal/x86/hal.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D hal.dll\
		-d $(PATH_TO_TOP)/ntoskrnl/hal/x86/hal.def

# --- DISPLAY ---

$(TARGET_FOLDER_LIB)/vgaddi.a: $(PATH_TO_TOP)/drivers/dd/vga/display/vgaddi.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D vgaddi.dll\
		-d $(PATH_TO_TOP)/drivers/dd/vga/display/vgaddi.def

$(TARGET_FOLDER_LIB)/vidport.a: $(PATH_TO_TOP)/drivers/dd/vidport/vidport.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D vidport.sys\
		-d $(PATH_TO_TOP)/drivers/dd/vidport/vidport.def

# --- NET ---

$(TARGET_FOLDER_LIB)/afd.a: $(PATH_TO_TOP)/drivers/net/afd/afd.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D afd.sys\
		-d $(PATH_TO_TOP)/drivers/net/afd/afd.def

$(TARGET_FOLDER_LIB)/ndis.a: $(PATH_TO_TOP)/drivers/net/ndis/ndis.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D ndis.sys\
		-d $(PATH_TO_TOP)/drivers/net/ndis/ndis.def

$(TARGET_FOLDER_LIB)/tcpip.a: $(PATH_TO_TOP)/drivers/net/tcpip/tcpip.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D tcpip.sys\
		-d $(PATH_TO_TOP)/drivers/net/tcpip/tcpip.def

$(TARGET_FOLDER_LIB)/packet.a: $(PATH_TO_TOP)/drivers/net/packet/packet.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D packet.sys\
		-d $(PATH_TO_TOP)/drivers/net/packet/packet.def

$(TARGET_FOLDER_LIB)/tdi.a: $(PATH_TO_TOP)/drivers/net/tdi/misc/tdi.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D tdi.sys\
		-d $(PATH_TO_TOP)/drivers/net/tdi/misc/tdi.def

# --- STORAGE ---

$(TARGET_FOLDER_LIB)/class2.a: $(PATH_TO_TOP)/drivers/storage/class2/class2.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D class2.sys\
		-d $(PATH_TO_TOP)/drivers/storage/class2/class2.def

$(TARGET_FOLDER_LIB)/scsiport.a: $(PATH_TO_TOP)/drivers/storage/scsiport/scsiport.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D scsiport.sys\
		-d $(PATH_TO_TOP)/drivers/storage/scsiport/scsiport.def

# --- NATIVE ---

$(TARGET_FOLDER_LIB)/ntdll.a: $(PATH_TO_TOP)/lib/ntdll/def/ntdll.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D ntdll.dll\
		-d $(PATH_TO_TOP)/lib/ntdll/def/ntdll.def
	
$(TARGET_FOLDER_LIB)/fmifs.a: $(PATH_TO_TOP)/lib/fmifs/fmifs.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D fmifs.dll\
		-d $(PATH_TO_TOP)/lib/fmifs/fmifs.def
	
# --- Service rules ---

clean:
	- $(RM) $(TARGET_FOLDER_LIB)/*.a
	- $(RM) $(TARGET_FOLDER_INC)/*.h

install:

dist:

#EOF

