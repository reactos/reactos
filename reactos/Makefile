# $Id: Makefile,v 1.277 2004/12/30 18:31:43 ion Exp $
#
# Global makefile
#

PATH_TO_TOP = .

include $(PATH_TO_TOP)/rules.mak
include $(PATH_TO_TOP)/config

#
# Define to build ReactOS external targets
#
ifeq ($(ROS_BUILD_EXT),)
ROS_BUILD_EXT = no
else
ROS_BUILD_EXT = yes
endif

ifneq ($(MINIMALDEPENDENCIES),no)
IMPLIB =
else
IMPLIB = implib
endif

# Required to run the system
COMPONENTS = ntoskrnl

# Hardware Abstraction Layers
# halx86
HALS = halx86/up halx86/mp

# Bus drivers
# acpi isapnp pci
BUS = acpi isapnp pci

# Filesystem libraries
# vfatlib
LIB_FSLIB = vfatlib

# Static libraries
LIB_STATIC = string rosrtl epsapi uuid libwine zlib rtl tgetopt pseh adns dxguid strmiids

# Keyboard layout libraries
DLLS_KBD = kbdda kbddv kbdfr kbdgr kbdse kbduk kbdus 

# Control Panels
DLLS_CPL = cpl

# Shell extensions
DLLS_SHELLEXT = shellext

# User mode libraries
# libpcap packet epsapi
DLLS = acledit aclui advapi32 advpack cabinet cards comctl32 crtdll comdlg32 d3d8thk dbghelp expat fmifs freetype \
	gdi32 gdiplus glu32 hid imagehlp imm32 iphlpapi kernel32 lzexpand mesa32 midimap mmdrv mpr msacm msafd \
	msgina msimg32 msvcrt20 msvideo mswsock netapi32 ntdll ole32 oleaut32 oledlg olepro32 opengl32 \
	packet psapi riched20 richedit rpcrt4 samlib secur32 setupapi shell32 shlwapi snmpapi syssetup twain \
	unicode user32 userenv version wininet winmm winspool ws2help ws2_32 wsock32 wshirda dnsapi \
	dinput dinput8 dxdiagn devenum dsound $(DLLS_KBD) $(DLLS_CPL) $(DLLS_SHELLEXT)

SUBSYS = smss win32k csrss ntvdm

#
# Select the server(s) you want to build
#
#SERVERS = posix linux os2
SERVERS = win32

# Driver support libraries
#bzip2 zlib oskittcp
DRIVERS_LIB = bzip2 oskittcp ip csq

# Kernel mode device drivers
# Obsolete: ide
# beep blue floppy null parallel ramdrv serenum serial
DEVICE_DRIVERS = beep blue debugout null serial bootvid

# Kernel mode input drivers
INPUT_DRIVERS = keyboard mouclass psaux sermouse

# Kernel mode file system drivers
# cdfs ext2 fs_rec ms np vfat
FS_DRIVERS = cdfs fs_rec ms np vfat mup ntfs

# Kernel mode networking drivers
# afd ndis npf tcpip tdi wshtcpip
NET_DRIVERS = afd ndis npf tcpip tdi wshtcpip

# Kernel mode networking device drivers
# ne2000 pcnet
NET_DEVICE_DRIVERS = ne2000 pcnet

# Kernel mode storage drivers
# atapi cdrom class2 disk scsiport
STORAGE_DRIVERS = atapi cdrom class2 disk floppy scsiport diskdump

# System applications
# autochk cmd format services setup usetup welcome winlogon msiexec 
SYS_APPS = autochk calc cmd explorer expand format regedt32 regsvr32 \
  services setup taskmgr userinit usetup welcome vmwinst winlogon \
  regedit winefile notepad reactos

# System services
# rpcss eventlog
SYS_SVC = rpcss eventlog

APPS = testsets utils


# External modules and (sub)systems for ReactOS
# rosapps posix os2 (requires c++) java (non-existant)
EXTERNALS = rosapps

ifeq ($(ROS_BUILD_EXT),yes)
EXT_MODULES = $(EXTERNALS)
else
EXT_MODULES =
endif

KERNEL_DRIVERS = $(DRIVERS_LIB) $(DEVICE_DRIVERS) $(INPUT_DRIVERS) $(FS_DRIVERS) \
	$(NET_DRIVERS) $(NET_DEVICE_DRIVERS) $(STORAGE_DRIVERS) VIDEO_DRIVERS

# Regression tests
REGTESTS = regtests

all: bootstrap $(COMPONENTS) $(REGTESTS) $(HALS) $(BUS) $(LIB_FSLIB) $(DLLS) $(SUBSYS) \
     $(KERNEL_DRIVERS) $(SYS_APPS) $(SYS_SVC) \
     $(APPS) $(EXT_MODULES)

bootstrap: dk implib iface_native iface_additional

#config: $(TOOLS:%=%_config)

depends: $(LIB_STATIC:%=%_depends) $(LIB_FSLIB:%=%_depends) msvcrt_depends $(DLLS:%=%_depends) \
         $(SUBSYS:%=%_depends) $(SYS_SVC:%=%_depends) \
         $(EXT_MODULES:%=%_depends) $(POSIX_LIBS:%=%_depends)

implib: hallib $(LIB_STATIC) $(COMPONENTS:%=%_implib) $(HALS:%=%_implib) $(BUS:%=%_implib) \
	      $(LIB_STATIC:%=%_implib) $(LIB_FSLIB:%=%_implib) msvcrt_implib $(DLLS:%=%_implib) \
	      $(KERNEL_DRIVERS:%=%_implib) $(SUBSYS:%=%_implib) \
	      $(SYS_APPS:%=%_implib) $(SYS_SVC:%=%_implib) $(EXT_MODULES:%=%_implib) \
	      $(REGTESTS:%=%_implib)

test: $(COMPONENTS:%=%_test) $(HALS:%=%_test) $(BUS:%=%_test) \
	    $(LIB_STATIC:%=%_test) $(LIB_FSLIB:%=%_test) msvcrt_test $(DLLS:%=%_test) \
	    $(KERNEL_DRIVERS:%=%_test) $(SUBSYS:%=%_test) \
	    $(SYS_SVC:%=%_test) $(EXT_MODULES:%=%_test)

clean: tools dk_clean iface_native_clean iface_additional_clean hallib_clean \
       $(HALS:%=%_clean) $(COMPONENTS:%=%_clean) $(BUS:%=%_clean) \
       $(LIB_STATIC:%=%_clean) $(LIB_FSLIB:%=%_clean) msvcrt_clean \
       $(DLLS:%=%_clean) $(KERNEL_DRIVERS:%=%_clean) \
       $(SUBSYS:%=%_clean) $(SYS_APPS:%=%_clean) $(SYS_SVC:%=%_clean) \
       $(NET_APPS:%=%_clean) $(APPS:%=%_clean) $(EXT_MODULES:%=%_clean) \
       $(REGTESTS:%=%_clean) clean_after tools_clean

clean_after:
	$(HALFVERBOSEECHO) [RM]      /include/roscfg.h
	$(RM) $(PATH_TO_TOP)/include/roscfg.h

fastinstall: tools install_dirs install_before \
         $(COMPONENTS:%=%_install) $(HALS:%=%_install) $(BUS:%=%_install) \
         $(LIB_STATIC:%=%_install) $(LIB_FSLIB:%=%_install) msvcrt_install $(DLLS:%=%_install) \
         $(KERNEL_DRIVERS:%=%_install) $(SUBSYS:%=%_install) \
         $(SYS_APPS:%=%_install) $(SYS_SVC:%=%_install) \
         $(APPS:%=%_install) $(EXT_MODULES:%=%_install) $(REGTESTS:%=%_install)
install: fastinstall registry

FREELDR_DIR = ../freeldr

freeldr:
	$(MAKE) -C $(FREELDR_DIR)

bootcd_directory_layout:
	$(HALFVERBOSEECHO) [RMKDIR]  $(BOOTCD_DIR)
	$(RMKDIR) $(BOOTCD_DIR)
	$(HALFVERBOSEECHO) [RMKDIR]  $(BOOTCD_DIR)/bootdisk
	$(RMKDIR) $(BOOTCD_DIR)/bootdisk
	$(HALFVERBOSEECHO) [RMKDIR]  $(BOOTCD_DIR)/loader
	$(RMKDIR) $(BOOTCD_DIR)/loader
	$(HALFVERBOSEECHO) [RMKDIR]  $(BOOTCD_DIR)/reactos
	$(RMKDIR) $(BOOTCD_DIR)/reactos
	$(HALFVERBOSEECHO) [RMKDIR]  $(BOOTCD_DIR)/reactos/system32
	$(RMKDIR) $(BOOTCD_DIR)/reactos/system32
	$(HALFVERBOSEECHO) [COPY]    ${FREELDR_DIR}/bootsect/isoboot.bin to ${BOOTCD_DIR}/../isoboot.bin
	$(CP) ${FREELDR_DIR}/bootsect/isoboot.bin ${BOOTCD_DIR}/../isoboot.bin
	$(HALFVERBOSEECHO) [COPY]    ${FREELDR_DIR}/bootsect/dosmbr.bin to ${BOOTCD_DIR}/loader/dosmbr.bin
	$(CP) ${FREELDR_DIR}/bootsect/dosmbr.bin ${BOOTCD_DIR}/loader/dosmbr.bin
	$(HALFVERBOSEECHO) [COPY]    ${FREELDR_DIR}/bootsect/ext2.bin to ${BOOTCD_DIR}/loader/ext2.bin
	$(CP) ${FREELDR_DIR}/bootsect/ext2.bin ${BOOTCD_DIR}/loader/ext2.bin
	$(HALFVERBOSEECHO) [COPY]    ${FREELDR_DIR}/bootsect/fat.bin to ${BOOTCD_DIR}/loader/fat.bin
	$(CP) ${FREELDR_DIR}/bootsect/fat.bin ${BOOTCD_DIR}/loader/fat.bin
	$(HALFVERBOSEECHO) [COPY]    ${FREELDR_DIR}/bootsect/fat32.bin to ${BOOTCD_DIR}/loader/fat32.bin
	$(CP) ${FREELDR_DIR}/bootsect/fat32.bin ${BOOTCD_DIR}/loader/fat32.bin
	$(HALFVERBOSEECHO) [COPY]    ${FREELDR_DIR}/bootsect/isoboot.bin to ${BOOTCD_DIR}/loader/isoboot.bin
	$(CP) ${FREELDR_DIR}/bootsect/isoboot.bin ${BOOTCD_DIR}/loader/isoboot.bin
	$(HALFVERBOSEECHO) [COPY]    ${FREELDR_DIR}/freeldr/obj/i386/freeldr.sys to ${BOOTCD_DIR}/loader/freeldr.sys
	$(CP) ${FREELDR_DIR}/freeldr/obj/i386/freeldr.sys ${BOOTCD_DIR}/loader/freeldr.sys
	$(HALFVERBOSEECHO) [COPY]    ${FREELDR_DIR}/freeldr/obj/i386/setupldr.sys to ${BOOTCD_DIR}/loader/setupldr.sys
	$(CP) ${FREELDR_DIR}/freeldr/obj/i386/setupldr.sys ${BOOTCD_DIR}/loader/setupldr.sys

bootcd_bootstrap_files: $(COMPONENTS:%=%_bootcd) $(HALS:%=%_bootcd) $(BUS:%=%_bootcd) \
	$(LIB_STATIC:%=%_bootcd) $(LIB_FSLIB:%=%_bootcd) msvcrt_bootcd $(DLLS:%=%_bootcd) \
  $(KERNEL_DRIVERS:%=%_bootcd) $(SUBSYS:%=%_bootcd) $(SYS_APPS:%=%_bootcd)

bootcd_install_before:
	$(HALFVERBOSEECHO) [RLINE]   bootdata/autorun.inf to $(BOOTCD_DIR)/autorun.inf
	$(RLINE) bootdata/autorun.inf $(BOOTCD_DIR)/autorun.inf
	$(HALFVERBOSEECHO) [RLINE]   bootdata/readme.txt to $(BOOTCD_DIR)/readme.txt
	$(RLINE) bootdata/readme.txt $(BOOTCD_DIR)/readme.txt
	$(HALFVERBOSEECHO) [RLINE]   bootdata/hivecls.inf to $(BOOTCD_DIR)/reactos/hivecls.inf
	$(RLINE) bootdata/hivecls.inf $(BOOTCD_DIR)/reactos/hivecls.inf
	$(HALFVERBOSEECHO) [RLINE]   bootdata/hivedef.inf to $(BOOTCD_DIR)/reactos/hivedef.inf
	$(RLINE) bootdata/hivedef.inf $(BOOTCD_DIR)/reactos/hivedef.inf
	$(HALFVERBOSEECHO) [RLINE]   bootdata/hivesft.inf to $(BOOTCD_DIR)/reactos/hivesft.inf
	$(RLINE) bootdata/hivesft.inf $(BOOTCD_DIR)/reactos/hivesft.inf
	$(HALFVERBOSEECHO) [RLINE]   bootdata/hivesys.inf to $(BOOTCD_DIR)/reactos/hivesys.inf
	$(RLINE) bootdata/hivesys.inf $(BOOTCD_DIR)/reactos/hivesys.inf
	$(HALFVERBOSEECHO) [RLINE]   bootdata/txtsetup.sif to $(BOOTCD_DIR)/reactos/txtsetup.sif
	$(RLINE) bootdata/txtsetup.sif $(BOOTCD_DIR)/reactos/txtsetup.sif
	$(HALFVERBOSEECHO) [COPY]    bootdata/icon.ico to $(BOOTCD_DIR)/icon.ico
	$(CP) bootdata/icon.ico $(BOOTCD_DIR)/icon.ico
	$(HALFVERBOSEECHO) [COPY]    subsys/system/welcome/welcome.exe  to $(BOOTCD_DIR)/reactos/welcome.exe
	$(CP) subsys/system/welcome/welcome.exe $(BOOTCD_DIR)/reactos/welcome.exe
	$(HALFVERBOSEECHO) [COPY]    subsys/system/reactos/reactos.exe  to $(BOOTCD_DIR)/reactos/reactos.exe
	$(CP) subsys/system/reactos/reactos.exe $(BOOTCD_DIR)/reactos/reactos.exe
	$(HALFVERBOSEECHO) [COPY]    media/nls/c_1252.nls to $(BOOTCD_DIR)/reactos/c_1252.nls
	$(CP) media/nls/c_1252.nls $(BOOTCD_DIR)/reactos/c_1252.nls
	$(HALFVERBOSEECHO) [COPY]    media/nls/c_437.nls to $(BOOTCD_DIR)/reactos/c_437.nls
	$(CP) media/nls/c_437.nls $(BOOTCD_DIR)/reactos/c_437.nls
	$(HALFVERBOSEECHO) [COPY]    media/nls/l_intl.nls to $(BOOTCD_DIR)/reactos/l_intl.nls
	$(CP) media/nls/l_intl.nls $(BOOTCD_DIR)/reactos/l_intl.nls
	$(HALFVERBOSEECHO) [COPY]    media/drivers/etc/services to $(BOOTCD_DIR)/reactos/services
	$(CP) media/drivers/etc/services $(BOOTCD_DIR)/reactos/services

bootcd_basic: bootcd_directory_layout bootcd_bootstrap_files bootcd_install_before

bootcd_makecd:
	$(CABMAN) /C bootdata/packages/reactos.dff /L $(BOOTCD_DIR)/reactos /I
	$(CABMAN) /C bootdata/packages/reactos.dff /RC $(BOOTCD_DIR)/reactos/reactos.inf /L $(BOOTCD_DIR)/reactos /N
	- $(RM) $(BOOTCD_DIR)/reactos/reactos.inf
	$(HALFVERBOSEECHO) [CDMAKE]  ReactOS.iso
	$(CDMAKE) -v -m -b $(BOOTCD_DIR)/../isoboot.bin $(BOOTCD_DIR) REACTOS ReactOS.iso

ubootcd_unattend:
	$(HALFVERBOSEECHO) [COPY]    bootdata/unattend.inf to $(BOOTCD_DIR)/reactos/unattend.inf
	$(CP) bootdata/unattend.inf $(BOOTCD_DIR)/reactos/unattend.inf

livecd_directory_layout:
	$(HALFVERBOSEECHO) [RMKDIR]  $(LIVECD_DIR)
	$(RMKDIR) $(LIVECD_DIR)
	$(HALFVERBOSEECHO) [RMKDIR]  $(LIVECD_DIR)/loader
	$(RMKDIR) $(LIVECD_DIR)/loader
	$(HALFVERBOSEECHO) [RMKDIR]  $(LIVECD_DIR)/reactos
	$(RMKDIR) $(LIVECD_DIR)/reactos
	$(HALFVERBOSEECHO) [RMKDIR]  $(LIVECD_DIR)/Profiles/All\ Users/Desktop
	$(RMKDIR) $(LIVECD_DIR)/Profiles/All\ Users/Desktop
	$(HALFVERBOSEECHO) [RMKDIR]  $(LIVECD_DIR)/Profiles/Default\ User/Desktop
	$(RMKDIR) $(LIVECD_DIR)/Profiles/Default\ User/Desktop
	$(HALFVERBOSEECHO) [RMKDIR]  $(LIVECD_DIR)/Profiles/Default\ User/My\ Documents
	$(RMKDIR) $(LIVECD_DIR)/Profiles/Default\ User/My\ Documents
	$(HALFVERBOSEECHO) [COPY]    ${FREELDR_DIR}/bootsect/isoboot.bin to ${LIVECD_DIR}/../isoboot.bin
	$(CP) ${FREELDR_DIR}/bootsect/isoboot.bin ${LIVECD_DIR}/../isoboot.bin
	$(HALFVERBOSEECHO) [COPY]    ${FREELDR_DIR}/freeldr/obj/i386/freeldr.sys to ${LIVECD_DIR}/loader/setupldr.sys
	$(CP) ${FREELDR_DIR}/freeldr/obj/i386/freeldr.sys ${LIVECD_DIR}/loader/setupldr.sys
	$(HALFVERBOSEECHO) [RLINE]   bootdata/livecd.ini to $(LIVECD_DIR)/freeldr.ini
	$(RLINE) bootdata/livecd.ini $(LIVECD_DIR)/freeldr.ini

livecd_bootstrap_files:
	$(MAKE) LIVECD_INSTALL=yes fastinstall

livecd_install_before:
	$(MKHIVE) bootdata $(LIVECD_DIR)/reactos/system32/config bootdata/livecd.inf bootdata/hiveinst.inf

livecd_basic: livecd_directory_layout livecd_bootstrap_files livecd_install_before

livecd_makecd:
	$(HALFVERBOSEECHO) [CDMAKE]  roslive.iso
	$(CDMAKE) -m -j -b $(LIVECD_DIR)/../isoboot.bin $(LIVECD_DIR) REACTOS roslive.iso

bootcd: bootcd_basic bootcd_makecd

ubootcd: bootcd_basic ubootcd_unattend bootcd_makecd

livecd: livecd_basic livecd_makecd

registry: tools
	$(MKHIVE) bootdata $(INSTALL_DIR)/system32/config bootdata/hiveinst.inf

.PHONY: all bootstrap depends implib test clean clean_before install freeldr bootcd_directory_layout \
bootcd_bootstrap_files bootcd_install_before bootcd_basic bootcd_makecd ubootcd_unattend bootcd


$(COMPONENTS): dk

#
# System Applications
#
$(SYS_APPS): %: $(IMPLIB)
	$(MAKE) -C subsys/system/$*

$(SYS_APPS:%=%_implib): %_implib: dk
	$(MAKE) --silent -C subsys/system/$* implib

$(SYS_APPS:%=%_test): %_test:
	$(MAKE) -C subsys/system/$* test

$(SYS_APPS:%=%_clean): %_clean:
	$(MAKE) -C subsys/system/$* clean

$(SYS_APPS:%=%_install): %_install:
	$(MAKE) -C subsys/system/$* install

$(SYS_APPS:%=%_bootcd): %_bootcd:
	$(MAKE) -C subsys/system/$* bootcd

.PHONY: $(SYS_APPS) $(SYS_APPS:%=%_implib) $(SYS_APPS:%=%_test) \
  $(SYS_APPS:%=%_clean) $(SYS_APPS:%=%_install) $(SYS_APPS:%=%_bootcd)

#
# System Services
#
$(SYS_SVC): %: $(IMPLIB)
	$(MAKE) -C services/$*

$(SYS_SVC:%=%_depends): %_depends:
	$(MAKE) -C services/$* depends

$(SYS_SVC:%=%_implib): %_implib: dk
	$(MAKE) --silent -C services/$* implib

$(SYS_SVC:%=%_test): %_test:
	$(MAKE) -C services/$* test

$(SYS_SVC:%=%_clean): %_clean:
	$(MAKE) -C services/$* clean

$(SYS_SVC:%=%_install): %_install:
	$(MAKE) -C services/$* install

.PHONY: $(SYS_SVC) $(SYS_SVC:%=%_depends) $(SYS_SVC:%=%_implib) \
  $(SYS_SVC:%=%_test) $(SYS_SVC:%=%_clean) $(SYS_SVC:%=%_install)


#
# Applications
#
#
# Extra (optional system) Applications
#
$(APPS): %: $(IMPLIB)
	$(MAKE) -C apps/$*

# Not needed
# $(APPS:%=%_implib): %_implib: dk
#	$(MAKE) --silent -C apps/$* implib

$(APPS:%=%_test): %_test:
	$(MAKE) -C apps/$* test

$(APPS:%=%_clean): %_clean:
	$(MAKE) -C apps/$* clean

$(APPS:%=%_install): %_install:
	$(MAKE) -C apps/$* install

.PHONY: $(APPS) $(APPS:%=%_test) $(APPS:%=%_clean) $(APPS:%=%_install)


#
# External ports and subsystem personalities
#
$(EXTERNALS): %:
	$(MAKE) -C $(ROOT_PATH)/$*

$(EXTERNALS:%=%_depends): %_depends:
	$(MAKE) -C $(ROOT_PATH)/$* depends

$(EXTERNALS:%=%_implib): %_implib: dk
	$(MAKE) --silent -C $(ROOT_PATH)/$* implib

$(EXTERNALS:%=%_clean): %_clean:
	$(MAKE) -C $(ROOT_PATH)/$* clean

$(EXTERNALS:%=%_install): %_install:
	$(MAKE) -C $(ROOT_PATH)/$* install

.PHONY: $(EXTERNALS) $(EXTERNALS:%=%_depends) $(EXTERNALS:%=%_implib) $(EXTERNALS:%=%_clean) $(EXTERNALS:%=%_install)


#
# Tools
#
tools:
	$(MAKE) --silent -C tools

tools_implib:
	

tools_test:
	

tools_clean:
	$(MAKE) -C tools clean

tools_install:

.PHONY: tools tools_implib tools_test tools_clean tools_install


#
# Developer Kits
#
dk: tools
	@$(RMKDIR) $(DK_PATH)
	@$(RMKDIR) $(DDK_PATH)
	@$(RMKDIR) $(DDK_PATH_LIB)
	@$(RMKDIR) $(DDK_PATH_INC)
	@$(RMKDIR) $(SDK_PATH)
	@$(RMKDIR) $(SDK_PATH_LIB)
	@$(RMKDIR) $(SDK_PATH_INC)
	@$(RMKDIR) $(XDK_PATH)
	@$(RMKDIR) $(XDK_PATH_LIB)
#	@$(RMKDIR) $(XDK_PATH_INC)

dk_implib:

# WARNING! Be very sure that there are no important files
#          in these directories before cleaning them!!!
dk_clean:
	$(HALFVERBOSEECHO) [RM]      $(DDK_PATH_LIB)/*.a
	$(RM) $(DDK_PATH_LIB)/*.a
#	$(HALFVERBOSEECHO) [RM]      $(DDK_PATH_INC)/*.h
#	$(RM) $(DDK_PATH_INC)/*.h
	$(HALFVERBOSEECHO) [RMDIR]   $(DDK_PATH_LIB)
	$(RMDIR) $(DDK_PATH_LIB)
#	$(HALFVERBOSEECHO) [RMDIR]   $(DDK_PATH_INC)
#	$(RMDIR) $(DDK_PATH_INC)
	$(HALFVERBOSEECHO) [RM]      $(SDK_PATH_LIB)/*.a
	$(RM) $(SDK_PATH_LIB)/*.a
#	$(HALFVERBOSEECHO) [RM]      $(SDK_PATH_INC)/*.h
#	$(RM) $(SDK_PATH_INC)/*.h
	$(HALFVERBOSEECHO) [RMDIR]   $(SDK_PATH_LIB)
	$(RMDIR) $(SDK_PATH_LIB)
#	$(HALFVERBOSEECHO) [RMDIR]   $(SDK_PATH_INC)
#	$(RMDIR) $(SDK_PATH_INC)
	$(HALFVERBOSEECHO) [RM]      $(XDK_PATH_LIB)/*.a
	$(RM) $(XDK_PATH_LIB)/*.a
#	$(HALFVERBOSEECHO) [RM]      $(XDK_PATH_INC)/*.h
#	$(RM) $(XDK_PATH_INC)/*.h
	$(HALFVERBOSEECHO) [RMDIR]   $(XDK_PATH_LIB)
	$(RMDIR) $(XDK_PATH_LIB)
#	$(HALFVERBOSEECHO) [RMDIR]   $(XDK_PATH_INC)
#	$(RMDIR) $(XDK_PATH_INC)

dk_install:

.PHONY: dk dk_implib dk_clean dk_install


#
# Interfaces
#
iface_native:
	$(MAKE) --silent -C iface/native

iface_native_implib:
	
iface_native_test:
	
iface_native_clean:
	$(MAKE) --silent -C iface/native clean

iface_native_install:

iface_native_bootcd:

iface_additional:
	$(MAKE) --silent -C iface/addsys

iface_additional_implib:
	
iface_additional_test:
	
iface_additional_clean:
	$(MAKE) --silent -C iface/addsys clean

iface_additional_install:

iface_additional_bootcd:

.PHONY: iface_native iface_native_implib iface_native_test iface_native_clean \
        iface_native_install iface_native_bootcd iface_additional \
        iface_additional_implib iface_additional_test iface_additional_clean \
        iface_additional_install iface_additional_bootcd


#
# Bus driver rules
#
$(BUS): %: $(IMPLIB)
	$(MAKE) -C drivers/bus/$*

$(BUS:%=%_implib): %_implib: dk
	$(MAKE) --silent -C drivers/bus/$* implib

$(BUS:%=%_test): %_test:
	$(MAKE) -C drivers/bus/$* test

$(BUS:%=%_clean): %_clean:
	$(MAKE) -C drivers/bus/$* clean

$(BUS:%=%_install): %_install:
	$(MAKE) -C drivers/bus/$* install

$(BUS:%=%_bootcd): %_bootcd:
	$(MAKE) -C drivers/bus/$* bootcd

.PHONY: $(BUS) $(BUS:%=%_implib) $(BUS:%=%_clean) \
        $(BUS:%=%_install) $(BUS:%=%_bootcd)


#
# Driver support libraries rules
#
$(DRIVERS_LIB): %: $(IMPLIB)
	$(MAKE) -C drivers/lib/$*

$(DRIVERS_LIB:%=%_implib): %_implib: dk
	$(MAKE) --silent -C drivers/lib/$* implib

$(DRIVERS_LIB:%=%_test): %_test:
	$(MAKE) -C drivers/lib/$* test

$(DRIVERS_LIB:%=%_clean): %_clean:
	$(MAKE) -C drivers/lib/$* clean

$(DRIVERS_LIB:%=%_install): %_install:
	$(MAKE) -C drivers/lib/$* install

$(DRIVERS_LIB:%=%_bootcd): %_bootcd:
	$(MAKE) -C drivers/lib/$* bootcd

.PHONY: $(DRIVERS_LIB) $(DRIVERS_LIB:%=%_implib) $(DRIVERS_LIB:%=%_test) \
        $(DRIVERS_LIB:%=%_clean) $(DRIVERS_LIB:%=%_install) $(DRIVERS_LIB:%=%_bootcd)


#
# Device driver rules
#
$(DEVICE_DRIVERS): %: $(IMPLIB)
	$(MAKE) -C drivers/dd/$*

$(DEVICE_DRIVERS:%=%_implib): %_implib: dk
	$(MAKE) --silent -C drivers/dd/$* implib

$(DEVICE_DRIVERS:%=%_test): %_test:
	$(MAKE) -C drivers/dd/$* test

$(DEVICE_DRIVERS:%=%_clean): %_clean:
	$(MAKE) -C drivers/dd/$* clean

$(DEVICE_DRIVERS:%=%_install): %_install:
	$(MAKE) -C drivers/dd/$* install

$(DEVICE_DRIVERS:%=%_bootcd): %_bootcd:
	$(MAKE) -C drivers/dd/$* bootcd

.PHONY: $(DEVICE_DRIVERS) $(DEVICE_DRIVERS:%=%_implib) $(DEVICE_DRIVERS:%=%_test) \
        $(DEVICE_DRIVERS:%=%_clean) $(DEVICE_DRIVERS:%=%_install) $(DEVICE_DRIVERS:%=%_bootcd)


#
# Video device driver rules
#
VIDEO_DRIVERS: $(IMPLIB)
	$(MAKE) -C drivers/video

VIDEO_DRIVERS_implib: dk
	$(MAKE) --silent -C drivers/video implib

VIDEO_DRIVERS_test:
	$(MAKE) -C drivers/video test

VIDEO_DRIVERS_clean:
	$(MAKE) -C drivers/video clean

VIDEO_DRIVERS_install:
	$(MAKE) -C drivers/video install

VIDEO_DRIVERS_bootcd:
	$(MAKE) -C drivers/video bootcd

.PHONY: VIDEO_DRIVERS VIDEO_DRIVERS_implib VIDEO_DRIVERS_test\
        VIDEO_DRIVERS_clean VIDEO_DRIVERS_install VIDEO_DRIVERS_bootcd


#
# Input driver rules
#
$(INPUT_DRIVERS): %: $(IMPLIB)
	$(MAKE) -C drivers/input/$*

$(INPUT_DRIVERS:%=%_implib): %_implib: dk
	$(MAKE) --silent -C drivers/input/$* implib

$(INPUT_DRIVERS:%=%_test): %_test:
	$(MAKE) -C drivers/input/$* test

$(INPUT_DRIVERS:%=%_clean): %_clean:
	$(MAKE) -C drivers/input/$* clean

$(INPUT_DRIVERS:%=%_install): %_install:
	$(MAKE) -C drivers/input/$* install

$(INPUT_DRIVERS:%=%_bootcd): %_bootcd:
	$(MAKE) -C drivers/input/$* bootcd

.PHONY: $(INPUT_DRIVERS) $(INPUT_DRIVERS:%=%_implib) $(INPUT_DRIVERS:%=%_test) \
        $(INPUT_DRIVERS:%=%_clean) $(INPUT_DRIVERS:%=%_install) $(INPUT_DRIVERS:%=%_bootcd)

#
# Filesystem driver rules
#
$(FS_DRIVERS): %: $(IMPLIB)
	$(MAKE) -C drivers/fs/$*

$(FS_DRIVERS:%=%_implib): %_implib: dk
	$(MAKE) --silent -C drivers/fs/$* implib

$(FS_DRIVERS:%=%_test): %_test:
	$(MAKE) -C drivers/fs/$* test

$(FS_DRIVERS:%=%_clean): %_clean:
	$(MAKE) -C drivers/fs/$* clean

$(FS_DRIVERS:%=%_install): %_install:
	$(MAKE) -C drivers/fs/$* install

$(FS_DRIVERS:%=%_bootcd): %_bootcd:
	$(MAKE) -C drivers/fs/$* bootcd

.PHONY: $(FS_DRIVERS) $(FS_DRIVERS:%=%_implib) $(FS_DRIVERS:%=%_test) \
        $(FS_DRIVERS:%=%_clean) $(FS_DRIVERS:%=%_install) $(FS_DRIVERS:%=%_bootcd)


#
# Network driver rules
#
$(NET_DRIVERS): %: $(IMPLIB)
	$(MAKE) -C drivers/net/$*

$(NET_DRIVERS:%=%_implib): %_implib: dk
	$(MAKE) --silent -C drivers/net/$* implib

$(NET_DRIVERS:%=%_test): %_test:
	$(MAKE) -C drivers/net/$* test

$(NET_DRIVERS:%=%_clean): %_clean:
	$(MAKE) -C drivers/net/$* clean

$(NET_DRIVERS:%=%_install): %_install:
	$(MAKE) -C drivers/net/$* install

$(NET_DRIVERS:%=%_bootcd): %_bootcd:
	$(MAKE) -C drivers/net/$* bootcd

.PHONY: $(NET_DRIVERS) $(NET_DRIVERS:%=%_implib) $(NET_DRIVERS:%=%_test) \
        $(NET_DRIVERS:%=%_clean) $(NET_DRIVERS:%=%_install) $(NET_DRIVERS:%=%_bootcd)


#
# Network device driver rules
#
$(NET_DEVICE_DRIVERS): %: $(IMPLIB)
	$(MAKE) -C drivers/net/dd/$*

$(NET_DEVICE_DRIVERS:%=%_implib): %_implib: dk
	$(MAKE) --silent -C drivers/net/dd/$* implib

$(NET_DEVICE_DRIVERS:%=%_test): %_test:
	$(MAKE) -C drivers/net/dd/$* test

$(NET_DEVICE_DRIVERS:%=%_clean): %_clean:
	$(MAKE) -C drivers/net/dd/$* clean

$(NET_DEVICE_DRIVERS:%=%_install): %_install:
	$(MAKE) -C drivers/net/dd/$* install

$(NET_DEVICE_DRIVERS:%=%_bootcd): %_bootcd:
	$(MAKE) -C drivers/net/dd/$* bootcd

.PHONY: $(NET_DEVICE_DRIVERS) $(NET_DEVICE_DRIVERS:%=%_clean) \
        $(NET_DEVICE_DRIVERS:%=%_implib) $(NET_DEVICE_DRIVERS:%=%_test) \
        $(NET_DEVICE_DRIVERS:%=%_install) $(NET_DEVICE_DRIVERS:%=%_bootcd)


#
# storage driver rules
#
$(STORAGE_DRIVERS): %: $(IMPLIB)
	$(MAKE) -C drivers/storage/$*

$(STORAGE_DRIVERS:%=%_implib): %_implib: dk
	$(MAKE) --silent -C drivers/storage/$* implib

$(STORAGE_DRIVERS:%=%_test): %_test:
	$(MAKE) -C drivers/storage/$* test

$(STORAGE_DRIVERS:%=%_clean): %_clean:
	$(MAKE) -C drivers/storage/$* clean

$(STORAGE_DRIVERS:%=%_install): %_install:
	$(MAKE) -C drivers/storage/$* install

$(STORAGE_DRIVERS:%=%_bootcd): %_bootcd:
	$(MAKE) -C drivers/storage/$* bootcd

.PHONY: $(STORAGE_DRIVERS) $(STORAGE_DRIVERS:%=%_clean) \
        $(STORAGE_DRIVERS:%=%_implib) $(STORAGE_DRIVERS:%=%_test) \
        $(STORAGE_DRIVERS:%=%_install) $(STORAGE_DRIVERS:%=%_bootcd)


#
# Required system components
#
ntoskrnl: bootstrap
	$(MAKE) -C ntoskrnl

ntoskrnl_implib: dk
	$(MAKE) --silent -C ntoskrnl implib

ntoskrnl_test:
	$(MAKE) -C ntoskrnl test

ntoskrnl_clean:
	$(MAKE) -C ntoskrnl clean

ntoskrnl_install:
	$(MAKE) -C ntoskrnl install

ntoskrnl_bootcd:
	$(MAKE) -C ntoskrnl bootcd

.PHONY: ntoskrnl ntoskrnl_implib ntoskrnl_test \
        ntoskrnl_clean ntoskrnl_install ntoskrnl_bootcd


#
# Hardware Abstraction Layer import library
#
hallib: $(PATH_TO_TOP)/include/roscfg.h ntoskrnl_implib
	$(MAKE) --silent -C hal/hal

hallib_implib: dk ntoskrnl_implib
	$(MAKE) --silent -C hal/hal implib

hallib_test:
	$(MAKE) -C hal/hal test

hallib_clean:
	$(MAKE) -C hal/hal clean

hallib_install:
	$(MAKE) -C hal/hal install

hallib_bootcd:
	$(MAKE) -C hal/hal bootcd

.PHONY: hallib hallib_implib hallib_test hallib_clean \
        hallib_install hallib_bootcd


#
# Hardware Abstraction Layers
#
ifeq ($(MP),1)
halx86: halx86/mp
else
halx86: halx86/up
endif

$(HALS): %: $(IMPLIB)
	$(MAKE) -C hal/$*

$(HALS:%=%_implib): %_implib: dk
	$(MAKE) --silent -C hal/$* implib

$(HALS:%=%_test): %_test:
	$(MAKE) -C hal/$* test

$(HALS:%=%_clean): %_clean:
	$(MAKE) -C hal/$* clean

$(HALS:%=%_install): %_install:
	$(MAKE) -C hal/$* install

$(HALS:%=%_bootcd): %_bootcd:
	$(MAKE) -C hal/$* bootcd

.PHONY: $(HALS) $(HALS:%=%_implib) $(HALS:%=%_test) \
        $(HALS:%=%_clean) $(HALS:%=%_install) $(HALS:%=%_bootcd)


#
# File system libraries
#
$(LIB_FSLIB): %: dk
	$(MAKE) -C lib/fslib/$*

$(LIB_FSLIB:%=%_depends): %_depends:
	$(MAKE) -C lib/fslib/$* depends

$(LIB_FSLIB:%=%_implib): %_implib: dk
	$(MAKE) --silent -C lib/fslib/$* implib

$(LIB_FSLIB:%=%_test): %_test:
	$(MAKE) -C lib/fslib/$* test

$(LIB_FSLIB:%=%_clean): %_clean:
	$(MAKE) -C lib/fslib/$* clean

$(LIB_FSLIB:%=%_install): %_install:
	$(MAKE) -C lib/fslib/$* install

$(LIB_FSLIB:%=%_bootcd): %_bootcd:
	$(MAKE) -C lib/fslib/$* bootcd

.PHONY: $(LIB_FSLIB) $(LIB_FSLIB:%=%_depends) $(LIB_FSLIB:%=%_implib) \
        $(LIB_FSLIB:%=%_test) $(LIB_FSLIB:%=%_clean) \
        $(LIB_FSLIB:%=%_install) $(LIB_FSLIB:%=%_bootcd)


#
# Static libraries
#
$(LIB_STATIC): %: dk
	$(MAKE) --silent -C lib/$*

$(LIB_STATIC:%=%_depends): %_depends:
	$(MAKE) -C lib/string depends

$(LIB_STATIC:%=%_implib): %_implib: dk
	$(MAKE) --silent -C lib/$* implib

$(LIB_STATIC:%=%_test): %_test:
	$(MAKE) -C lib/$* test

$(LIB_STATIC:%=%_clean): %_clean:
	$(MAKE) -C lib/$* clean

$(LIB_STATIC:%=%_install): %_install:
	$(MAKE) -C lib/$* install

$(LIB_STATIC:%=%_bootcd): %_bootcd:
	$(MAKE) -C lib/$* bootcd

.PHONY: $(LIB_STATIC) $(LIB_STATIC:%=%_depends) $(LIB_STATIC:%=%_implib) \
        $(LIB_STATIC:%=%_test) $(LIB_STATIC:%=%_clean) \
        $(LIB_STATIC:%=%_install) $(LIB_STATIC:%=%_bootcd)


#
# MSVCRT is seperate since CRTDLL depend on this
#
msvcrt: $(IMPLIB)
	$(MAKE) -C lib/msvcrt

msvcrt_depends:
	$(MAKE) -C lib/msvcrt depends

msvcrt_implib: dk
	$(MAKE) --silent -C lib/msvcrt implib

msvcrt_test:
	$(MAKE) -C lib/msvcrt test

msvcrt_clean:
	$(MAKE) -C lib/msvcrt clean

msvcrt_install:
	$(MAKE) -C lib/msvcrt install

msvcrt_bootcd:
	$(MAKE) -C lib/msvcrt bootcd

.PHONY: msvcrt msvcrt_depends msvcrt_implib msvcrt_test \
        msvcrt_clean msvcrt_install msvcrt_bootcd


#
# DLLs
#
$(DLLS): %: $(IMPLIB) msvcrt
	$(MAKE) -C lib/$*

$(DLLS:%=%_depends): %_depends:
	$(MAKE) -C lib/$* depends

$(DLLS:%=%_implib): %_implib: dk
	$(MAKE) --silent -C lib/$* implib

$(DLLS:%=%_test): %_test:
	$(MAKE) -C lib/$* test

$(DLLS:%=%_clean): %_clean:
	$(MAKE) -C lib/$* clean

$(DLLS:%=%_install): %_install:
	$(MAKE) -C lib/$* install

$(DLLS:%=%_bootcd): %_bootcd:
	$(MAKE) -C lib/$* bootcd

.PHONY: $(DLLS) $(DLLS:%=%_depends) $(DLLS:%=%_implib) $(DLLS:%=%_test) \
        $(DLLS:%=%_clean) $(DLLS:%=%_install) $(DLLS:%=%_bootcd)


#
# Subsystem support modules
#
$(SUBSYS): %: $(IMPLIB)
	$(MAKE) -C subsys/$*

$(SUBSYS:%=%_depends): %_depends:
	$(MAKE) -C subsys/$* depends

$(SUBSYS:%=%_implib): %_implib: dk
	$(MAKE) --silent -C subsys/$* implib

$(SUBSYS:%=%_test): %_test:
	$(MAKE) -C subsys/$* test

$(SUBSYS:%=%_clean): %_clean:
	$(MAKE) -C subsys/$* clean

$(SUBSYS:%=%_install): %_install:
	$(MAKE) -C subsys/$* install

$(SUBSYS:%=%_bootcd): %_bootcd:
	$(MAKE) -C subsys/$* bootcd

.PHONY: $(SUBSYS) $(SUBSYS:%=%_depends) $(SUBSYS:%=%_implib) $(SUBSYS:%=%_test) \
        $(SUBSYS:%=%_clean) $(SUBSYS:%=%_install) $(SUBSYS:%=%_bootcd)

#
# Regression testsuite
#

$(REGTESTS): %: $(IMPLIB)
	$(MAKE) --silent -C regtests

$(REGTESTS:%=%_implib): %_implib: dk
	$(MAKE) --silent -C regtests implib

$(REGTESTS:%=%_clean): %_clean:
	$(MAKE) -C regtests clean

$(REGTESTS:%=%_install): %_install:
	$(MAKE) -C regtests install

.PHONY: $(REGTESTS) $(REGTESTS:%=%_depends) $(SUBSYS:%=%_clean) $(REGTESTS:%=%_install)


#
# Create an installation
#

install_clean:
	$(HALFVERBOSEECHO) [RM]      $(INSTALL_DIR)/system32/drivers/*.*
	$(RM) $(INSTALL_DIR)/system32/drivers/*.*
	$(HALFVERBOSEECHO) [RM]      $(INSTALL_DIR)/system32/config/*.*
	$(RM) $(INSTALL_DIR)/system32/config/*.*
	$(HALFVERBOSEECHO) [RM]      $(INSTALL_DIR)/system32/*.*
	$(RM) $(INSTALL_DIR)/system32/*.*
	$(HALFVERBOSEECHO) [RM]      $(INSTALL_DIR)/symbols/*.*
	$(RM) $(INSTALL_DIR)/symbols/*.*
	$(HALFVERBOSEECHO) [RM]      $(INSTALL_DIR)/media/fonts/*.*
	$(RM) $(INSTALL_DIR)/media/fonts/*.*
	$(HALFVERBOSEECHO) [RM]      $(INSTALL_DIR)/media/*.*
	$(RM) $(INSTALL_DIR)/media/*.*
	$(HALFVERBOSEECHO) [RM]      $(INSTALL_DIR)/inf/*.*
	$(RM) $(INSTALL_DIR)/inf/*.*
	$(HALFVERBOSEECHO) [RM]      $(INSTALL_DIR)/bin/*.*
	$(RM) $(INSTALL_DIR)/bin/*.*
	$(HALFVERBOSEECHO) [RMDIR]   $(INSTALL_DIR)/system32/drivers
	$(RMDIR) $(INSTALL_DIR)/system32/drivers
	$(HALFVERBOSEECHO) [RMDIR]   $(INSTALL_DIR)/system32/config
	$(RMDIR) $(INSTALL_DIR)/system32/config
	$(HALFVERBOSEECHO) [RMDIR]   $(INSTALL_DIR)/system32
	$(RMDIR) $(INSTALL_DIR)/system32
	$(HALFVERBOSEECHO) [RMDIR]   $(INSTALL_DIR)/symbols
	$(RMDIR) $(INSTALL_DIR)/symbols
	$(HALFVERBOSEECHO) [RMDIR]   $(INSTALL_DIR)/media/fonts
	$(RMDIR) $(INSTALL_DIR)/media/fonts
	$(HALFVERBOSEECHO) [RMDIR]   $(INSTALL_DIR)/media
	$(RMDIR) $(INSTALL_DIR)/media
	$(HALFVERBOSEECHO) [RMDIR]   $(INSTALL_DIR)/inf
	$(RMDIR) $(INSTALL_DIR)/inf
	$(HALFVERBOSEECHO) [RMDIR]   $(INSTALL_DIR)/bin
	$(RMDIR) $(INSTALL_DIR)/bin
	$(HALFVERBOSEECHO) [RMDIR]   $(INSTALL_DIR)
	$(RMDIR) $(INSTALL_DIR)

install_dirs:
	$(HALFVERBOSEECHO) [RMKDIR]  $(INSTALL_DIR)
	$(RMKDIR) $(INSTALL_DIR)
	$(HALFVERBOSEECHO) [RMKDIR]  $(INSTALL_DIR)/bin
	$(RMKDIR) $(INSTALL_DIR)/bin
	$(HALFVERBOSEECHO) [RMKDIR]  $(INSTALL_DIR)/inf
	$(RMKDIR) $(INSTALL_DIR)/inf
	$(HALFVERBOSEECHO) [RMKDIR]  $(INSTALL_DIR)/media
	$(RMKDIR) $(INSTALL_DIR)/media
	$(HALFVERBOSEECHO) [RMKDIR]  $(INSTALL_DIR)/media/fonts
	$(RMKDIR) $(INSTALL_DIR)/media/fonts
	$(HALFVERBOSEECHO) [RMKDIR]  $(INSTALL_DIR)/symbols
	$(RMKDIR) $(INSTALL_DIR)/symbols
	$(HALFVERBOSEECHO) [RMKDIR]  $(INSTALL_DIR)/system32
	$(RMKDIR) $(INSTALL_DIR)/system32
	$(HALFVERBOSEECHO) [RMKDIR]  $(INSTALL_DIR)/system32/config
	$(RMKDIR) $(INSTALL_DIR)/system32/config
	$(HALFVERBOSEECHO) [RMKDIR]  $(INSTALL_DIR)/system32/drivers
	$(RMKDIR) $(INSTALL_DIR)/system32/drivers
	$(HALFVERBOSEECHO) [RMKDIR]  $(INSTALL_DIR)/system32/drivers/etc
	$(RMKDIR) $(INSTALL_DIR)/system32/drivers/etc

install_before:
	$(HALFVERBOSEECHO) [INSTALL] media/inf to $(INSTALL_DIR)/inf
	$(CP) media/inf $(INSTALL_DIR)/inf
	$(HALFVERBOSEECHO) [INSTALL] media/fonts to $(INSTALL_DIR)/media/fonts
	$(CP) media/fonts $(INSTALL_DIR)/media/fonts
	$(HALFVERBOSEECHO) [INSTALL] media/nls to $(INSTALL_DIR)/system32
	$(CP) media/nls $(INSTALL_DIR)/system32
	$(HALFVERBOSEECHO) [INSTALL] media/nls/c_1252.nls to $(INSTALL_DIR)/system32/ansi.nls
	$(CP) media/nls/c_1252.nls $(INSTALL_DIR)/system32/ansi.nls
	$(HALFVERBOSEECHO) [INSTALL] media/nls/c_437.nls to $(INSTALL_DIR)/system32/oem.nls
	$(CP) media/nls/c_437.nls $(INSTALL_DIR)/system32/oem.nls
	$(HALFVERBOSEECHO) [INSTALL] media/nls/l_intl.nls to $(INSTALL_DIR)/system32/casemap.nls
	$(CP) media/nls/l_intl.nls $(INSTALL_DIR)/system32/casemap.nls
	$(HALFVERBOSEECHO) [INSTALL] media/drivers/etc/services to $(INSTALL_DIR)/system32/drivers/etc/services
	$(CP) media/drivers/etc/services $(INSTALL_DIR)/system32/drivers/etc/services

.PHONY: install_clean install_dirs install_before


etags:
	find . -name "*.[ch]" -print | etags --language=c -


docu:
	echo generating ReactOS NTOSKRNL documentation ...
	$(MAKE) -C ntoskrnl docu

	echo generating ReactOS drivers documentation ...
	$(MAKE) -C drivers docu

	echo generating ReactOS NTDLL documentation ...
	$(MAKE) -C lib/ntdll docu

	echo generating ReactOS Freetype documentation ...
	$(MAKE) -C lib/freetype docu

	echo generating ReactOS libs documentation ...
	$(MAKE) -C lib docu

	echo generating ReactOS WIN32K documentation ...
	$(MAKE) -C subsys/win32k docu

	echo generating ReactOS apps+tools documentation ...
	$(MAKE) -C apps docu

	echo generating ReactOS explorer documentation ...
	$(MAKE) -C subsys/system/explorer full-docu

	echo generating remaining ReactOS documentation ...
	doxygen Doxyfile

.PHONY: docu

include $(TOOLS_PATH)/config.mk
