#
# Global makefile
#

PATH_TO_TOP = .

#
# Define to build ReactOS external targets
#
ifeq ($(ROS_BUILD_EXT),)
ROS_BUILD_EXT = no
else
ROS_BUILD_EXT = yes
endif

include $(PATH_TO_TOP)/rules.mak

# Required to run the system
COMPONENTS = iface_native iface_additional hallib ntoskrnl

# Hardware Abstraction Layers
# halx86
HALS = halx86

# Bus drivers
# acpi isapnp pci
BUS = acpi isapnp pci

# User mode libraries
# advapi32 crtdll fmifs gdi32 kernel32 libpcap packet msafd msvcrt ntdll ole32
# oleaut32 psapi rpcrt4 secur32 shell32 user32 version ws2help ws2_32 wsock32 wshirda
DLLS = advapi32 crtdll fmifs gdi32 kernel32 packet msafd msvcrt ntdll \
       secur32 user32 version winedbgc ws2help ws2_32 wshirda #winmm 

SUBSYS = smss win32k csrss ntvdm

#
# Select the server(s) you want to build
#
#SERVERS = posix linux os2
SERVERS = win32

# Boot loaders
# dos
LOADERS = dos

# Driver support libraries
#bzip2 zlib
DRIVERS_LIB = bzip2 zlib

# Kernel mode device drivers
# Obsolete: ide
# beep blue floppy null parallel ramdrv serenum serial vga vidport
DEVICE_DRIVERS = beep blue floppy null serial vga vidport

# Kernel mode input drivers
# keyboard mouclass psaux sermouse
INPUT_DRIVERS = keyboard mouclass psaux

# Kernel mode file system drivers
# cdfs ext2 fs_rec ms np vfat
FS_DRIVERS = cdfs fs_rec ms np vfat mup ntfs

# Kernel mode networking drivers
# afd ndis npf tcpip tdi wshtcpip
NET_DRIVERS = afd ndis npf tcpip tdi wshtcpip

# Kernel mode networking device drivers
# ne2000
NET_DEVICE_DRIVERS = ne2000

# Kernel mode storage drivers
# atapi cdrom class2 disk scsiport
STORAGE_DRIVERS = atapi cdrom class2 disk scsiport

# System applications
# autochk lsass services shell winlogon
SYS_APPS = autochk services shell winlogon gstart usetup

# System services
# rpcss eventlog
SYS_SVC = rpcss eventlog

# Test applications
# alive apc args atomtest bench consume copymove count dump_shared_data
# event file gditest hello isotest lpc mstest mutex nptest
# pteb regtest sectest shm simple thread vmtest winhello
TEST_APPS = alive apc args atomtest bench consume copymove count dump_shared_data \
            event file gditest hello isotest lpc mstest mutex nptest \
            pteb regtest sectest shm simple thread tokentest vmtest winhello dibtest

# Console system utilities
# cabman cat net objdir partinfo pice ps sc stats
UTIL_APPS = cat objdir partinfo sc stats

# External (sub)systems for ReactOS
# rosapps wine posix os2 (requires c++) java (non-existant)
EXTERNALS = rosapps wine posix os2

ifeq ($(ROS_BUILD_EXT),yes)
EXT_MODULES = $(EXTERNALS)
else
EXT_MODULES =
endif

KERNEL_DRIVERS = $(DRIVERS_LIB) $(DEVICE_DRIVERS) $(INPUT_DRIVERS) $(FS_DRIVERS) \
	$(NET_DRIVERS) $(NET_DEVICE_DRIVERS) $(STORAGE_DRIVERS)

all: tools dk implib $(COMPONENTS) $(HALS) $(BUS) $(DLLS) $(SUBSYS) \
     $(LOADERS) $(KERNEL_DRIVERS) $(SYS_APPS) $(SYS_SVC) \
     $(TEST_APPS) $(UTIL_APPS) $(EXT_MODULES)

#config: $(TOOLS:%=%_config)

depends: $(DLLS:%=%_depends) $(SUBSYS:%=%_depends) $(SYS_SVC:%=%_depends) \
         $(EXT_MODULES:%=%_depends) $(POSIX_LIBS:%=%_depends)

implib: $(COMPONENTS:%=%_implib) $(HALS:%=%_implib) $(BUS:%=%_implib) \
        $(DLLS:%=%_implib) $(LOADERS:%=%_implib) \
        $(KERNEL_DRIVERS:%=%_implib) $(SUBSYS:%=%_implib) \
        $(SYS_APPS:%=%_implib) $(SYS_SVC:%=%_implib) \
        $(TEST_APPS:%=%_implib) $(UTIL_APPS:%=%_implib) \
        $(EXT_MODULES:%=%_implib)

clean: tools dk_clean $(HALS:%=%_clean) \
       $(COMPONENTS:%=%_clean) $(BUS:%=%_clean) $(DLLS:%=%_clean) \
       $(LOADERS:%=%_clean) $(KERNEL_DRIVERS:%=%_clean) $(SUBSYS:%=%_clean) \
       $(SYS_APPS:%=%_clean) $(SYS_SVC:%=%_clean) $(TEST_APPS:%=%_clean) \
       $(UTIL_APPS:%=%_clean) $(NET_APPS:%=%_clean) $(EXT_MODULES:%=%_clean) \
       clean_after tools_clean

clean_after:
	$(RM) $(PATH_TO_TOP)/include/roscfg.h

install: tools install_dirs install_before \
         $(COMPONENTS:%=%_install) $(HALS:%=%_install) $(BUS:%=%_install) \
         $(DLLS:%=%_install) $(LOADERS:%=%_install) \
         $(KERNEL_DRIVERS:%=%_install) $(SUBSYS:%=%_install) \
         $(SYS_APPS:%=%_install) $(SYS_SVC:%=%_install) \
         $(TEST_APPS:%=%_install) $(UTIL_APPS:%=%_install) \
         $(EXT_MODULES:%=%_install)

dist: $(TOOLS_PATH)/rcopy$(EXE_POSTFIX) dist_clean dist_dirs \
      $(HALS:%=%_dist) $(COMPONENTS:%=%_dist) $(BUS:%=%_dist) $(DLLS:%=%_dist) \
      $(LOADERS:%=%_dist) $(KERNEL_DRIVERS:%=%_dist) $(SUBSYS:%=%_dist) \
      $(SYS_APPS:%=%_dist) $(SYS_SVC:%=%_dist) $(TEST_APPS:%=%_dist) \
      $(UTIL_APPS:%=%_dist) $(NET_APPS:%=%_dist) $(EXT_MODULES:%=%_dist)

.PHONY: all depends implib clean clean_before install dist


#
# System Applications
#
$(SYS_APPS): %:
	make -C subsys/system/$*

$(SYS_APPS:%=%_implib): %_implib:
	make -C subsys/system/$* implib

$(SYS_APPS:%=%_clean): %_clean:
	make -C subsys/system/$* clean

$(SYS_APPS:%=%_dist): %_dist:
	make -C subsys/system/$* dist

$(SYS_APPS:%=%_install): %_install:
	make -C subsys/system/$* install

.PHONY: $(SYS_APPS) $(SYS_APPS:%=%_implib) $(SYS_APPS:%=%_clean) $(SYS_APPS:%=%_install) $(SYS_APPS:%=%_dist)

#
# System Services
#
$(SYS_SVC): %:
	make -C services/$*

$(SYS_SVC:%=%_depends): %_depends:
	make -C services/$* depends

$(SYS_SVC:%=%_implib): %_implib:
	make -C services/$* implib

$(SYS_SVC:%=%_clean): %_clean:
	make -C services/$* clean

$(SYS_SVC:%=%_dist): %_dist:
	make -C services/$* dist

$(SYS_SVC:%=%_install): %_install:
	make -C services/$* install

.PHONY: $(SYS_SVC) $(SYS_SVC:%=%_depends) $(SYS_SVC:%=%_implib) $(SYS_SVC:%=%_clean) $(SYS_SVC:%=%_install) $(SYS_SVC:%=%_dist)


#
# Test Applications
#
$(TEST_APPS): %:
	make -C apps/tests/$*

$(TEST_APPS:%=%_implib): %_implib:
	make -C apps/tests/$* implib

$(TEST_APPS:%=%_clean): %_clean:
	make -C apps/tests/$* clean

$(TEST_APPS:%=%_dist): %_dist:
	make -C apps/tests/$* dist

$(TEST_APPS:%=%_install): %_install:
	make -C apps/tests/$* install

.PHONY: $(TEST_APPS) $(TEST_APPS:%=%_implib) $(TEST_APPS:%=%_clean) $(TEST_APPS:%=%_install) $(TEST_APPS:%=%_dist)


#
# Utility Applications
#
$(UTIL_APPS): %:
	make -C apps/utils/$*

$(UTIL_APPS:%=%_implib): %_implib:
	make -C apps/utils/$* implib

$(UTIL_APPS:%=%_clean): %_clean:
	make -C apps/utils/$* clean

$(UTIL_APPS:%=%_dist): %_dist:
	make -C apps/utils/$* dist

$(UTIL_APPS:%=%_install): %_install:
	make -C apps/utils/$* install

.PHONY: $(UTIL_APPS) $(UTIL_APPS:%=%_implib) $(UTIL_APPS:%=%_clean) $(UTIL_APPS:%=%_install) $(UTIL_APPS:%=%_dist)


#
# External ports and subsystem personalities
#
$(EXTERNALS): %:
	make -C $(ROOT_PATH)/$*

$(EXTERNALS:%=%_depends): %_depends:
	make -C $(ROOT_PATH)/$* depends

$(EXTERNALS:%=%_implib): %_implib:
	make -C $(ROOT_PATH)/$* implib

$(EXTERNALS:%=%_clean): %_clean:
	make -C $(ROOT_PATH)/$* clean

$(EXTERNALS:%=%_dist): %_dist:
	make -C $(ROOT_PATH)/$* dist

$(EXTERNALS:%=%_install): %_install:
	make -C $(ROOT_PATH)/$* install

.PHONY: $(EXTERNALS) $(EXTERNALS:%=%_depends) $(EXTERNALS:%=%_implib) $(EXTERNALS:%=%_clean) $(EXTERNALS:%=%_install) $(EXTERNALS:%=%_dist)


#
# Tools
#
tools:
	make -C tools

tools_implib:

tools_clean:
	make -C tools clean

tools_install:

tools_dist:

.PHONY: tools tools_implib tools_clean tools_install tools_dist


#
# Developer Kits
#
dk:
	$(RMKDIR) $(DK_PATH)
	$(RMKDIR) $(DDK_PATH)
	$(RMKDIR) $(DDK_PATH_LIB)
	$(RMKDIR) $(DDK_PATH_INC)
	$(RMKDIR) $(SDK_PATH)
	$(RMKDIR) $(SDK_PATH_LIB)
	$(RMKDIR) $(SDK_PATH_INC)
	$(RMKDIR) $(XDK_PATH)
	$(RMKDIR) $(XDK_PATH_LIB)
	$(RMKDIR) $(XDK_PATH_INC)

dk_implib:

# WARNING! Be very sure that there are no important files
#          in these directories before cleaning them!!!
dk_clean:
	$(RM) $(DDK_PATH_LIB)/*.a
# $(RM) $(DDK_PATH_INC)/*.h
	$(RMDIR) $(DDK_PATH_LIB)
#	$(RMDIR) $(DDK_PATH_INC)
	$(RM) $(SDK_PATH_LIB)/*.a
# $(RM) $(SDK_PATH_INC)/*.h
	$(RMDIR) $(SDK_PATH_LIB)
#	$(RMDIR) $(SDK_PATH_INC)
	$(RM) $(XDK_PATH_LIB)/*.a
#	$(RM) $(XDK_PATH_INC)/*.h
	$(RMDIR) $(XDK_PATH_LIB)
#	$(RMDIR) $(XDK_PATH_INC)

dk_install:

dk_dist:

.PHONY: dk dk_implib dk_clean dk_install dk_dist


#
# Interfaces
#
iface_native:
	make -C iface/native

iface_native_implib:

iface_native_clean:
	make -C iface/native clean

iface_native_install:

iface_native_dist:

iface_additional:
	make -C iface/addsys

iface_additional_implib:

iface_additional_clean:
	make -C iface/addsys clean

iface_additional_install:

iface_additional_dist:

.PHONY: iface_native iface_native_implib iface_native_clean iface_native_install \
        iface_native_dist \
        iface_additional iface_additional_implib iface_additional_clean \
        iface_additional_install iface_additional_dist

#
# Bus driver rules
#
$(BUS): %:
	make -C drivers/bus/$*

$(BUS:%=%_implib): %_implib:
	make -C drivers/bus/$* implib

$(BUS:%=%_clean): %_clean:
	make -C drivers/bus/$* clean

$(BUS:%=%_install): %_install:
	make -C drivers/bus/$* install

$(BUS:%=%_dist): %_dist:
	make -C drivers/bus/$* dist

.PHONY: $(BUS) $(BUS:%=%_implib) $(BUS:%=%_clean) \
        $(BUS:%=%_install) $(BUS:%=%_dist)

#
# Driver support libraries rules
#
$(DRIVERS_LIB): %:
	make -C drivers/lib/$*

$(DRIVERS_LIB:%=%_implib): %_implib:
	make -C drivers/lib/$* implib

$(DRIVERS_LIB:%=%_clean): %_clean:
	make -C drivers/lib/$* clean

$(DRIVERS_LIB:%=%_install): %_install:
	make -C drivers/lib/$* install

$(DRIVERS_LIB:%=%_dist): %_dist:
	make -C drivers/lib/$* dist

.PHONY: $(DRIVERS_LIB) $(DRIVERS_LIB:%=%_implib) $(DRIVERS_LIB:%=%_clean) \
        $(DRIVERS_LIB:%=%_install) $(DRIVERS_LIB:%=%_dist)

#
# Device driver rules
#
$(DEVICE_DRIVERS): %:
	make -C drivers/dd/$*

$(DEVICE_DRIVERS:%=%_implib): %_implib:
	make -C drivers/dd/$* implib

$(DEVICE_DRIVERS:%=%_clean): %_clean:
	make -C drivers/dd/$* clean

$(DEVICE_DRIVERS:%=%_install): %_install:
	make -C drivers/dd/$* install

$(DEVICE_DRIVERS:%=%_dist): %_dist:
	make -C drivers/dd/$* dist

.PHONY: $(DEVICE_DRIVERS) $(DEVICE_DRIVERS:%=%_implib) $(DEVICE_DRIVERS:%=%_clean) \
        $(DEVICE_DRIVERS:%=%_install) $(DEVICE_DRIVERS:%=%_dist)

#
# Input driver rules
#
$(INPUT_DRIVERS): %:
	make -C drivers/input/$*

$(INPUT_DRIVERS:%=%_implib): %_implib:
	make -C drivers/input/$* implib

$(INPUT_DRIVERS:%=%_clean): %_clean:
	make -C drivers/input/$* clean

$(INPUT_DRIVERS:%=%_install): %_install:
	make -C drivers/input/$* install

$(INPUT_DRIVERS:%=%_dist): %_dist:
	make -C drivers/input/$* dist

.PHONY: $(INPUT_DRIVERS) $(INPUT_DRIVERS:%=%_implib) $(INPUT_DRIVERS:%=%_clean)\
        $(INPUT_DRIVERS:%=%_install) $(INPUT_DRIVERS:%=%_dist)

$(FS_DRIVERS): %:
	make -C drivers/fs/$*

$(FS_DRIVERS:%=%_implib): %_implib:
	make -C drivers/fs/$* implib

$(FS_DRIVERS:%=%_clean): %_clean:
	make -C drivers/fs/$* clean

$(FS_DRIVERS:%=%_install): %_install:
	make -C drivers/fs/$* install

$(FS_DRIVERS:%=%_dist): %_dist:
	make -C drivers/fs/$* dist

.PHONY: $(FS_DRIVERS) $(FS_DRIVERS:%=%_implib) $(FS_DRIVERS:%=%_clean) \
        $(FS_DRIVERS:%=%_install) $(FS_DRIVERS:%=%_dist)

#
# Network driver rules
#
$(NET_DRIVERS): %:
	make -C drivers/net/$*

$(NET_DRIVERS:%=%_implib): %_implib:
	make -C drivers/net/$* implib

$(NET_DRIVERS:%=%_clean): %_clean:
	make -C drivers/net/$* clean

$(NET_DRIVERS:%=%_install): %_install:
	make -C drivers/net/$* install

$(NET_DRIVERS:%=%_dist): %_dist:
	make -C drivers/net/$* dist

.PHONY: $(NET_DRIVERS) $(NET_DRIVERS:%=%_implib) $(NET_DRIVERS:%=%_clean) \
        $(NET_DRIVERS:%=%_install) $(NET_DRIVERS:%=%_dist)

$(NET_DEVICE_DRIVERS): %:
	make -C drivers/net/dd/$*

$(NET_DEVICE_DRIVERS:%=%_implib): %_implib:
	make -C drivers/net/dd/$* implib

$(NET_DEVICE_DRIVERS:%=%_clean): %_clean:
	make -C drivers/net/dd/$* clean

$(NET_DEVICE_DRIVERS:%=%_install): %_install:
	make -C drivers/net/dd/$* install

$(NET_DEVICE_DRIVERS:%=%_dist): %_dist:
	make -C drivers/net/dd/$* dist

.PHONY: $(NET_DEVICE_DRIVERS) $(NET_DEVICE_DRIVERS:%=%_clean) $(NET_DEVICE_DRIVERS:%=%_implib) \
        $(NET_DEVICE_DRIVERS:%=%_install) $(NET_DEVICE_DRIVERS:%=%_dist)

#
# storage driver rules
#
$(STORAGE_DRIVERS): %:
	make -C drivers/storage/$*

$(STORAGE_DRIVERS:%=%_implib): %_implib:
	make -C drivers/storage/$* implib

$(STORAGE_DRIVERS:%=%_clean): %_clean:
	make -C drivers/storage/$* clean

$(STORAGE_DRIVERS:%=%_install): %_install:
	make -C drivers/storage/$* install

$(STORAGE_DRIVERS:%=%_dist): %_dist:
	make -C drivers/storage/$* dist

.PHONY: $(STORAGE_DRIVERS) $(STORAGE_DRIVERS:%=%_clean) \
        $(STORAGE_DRIVERS:%=%_install) $(STORAGE_DRIVERS:%=%_dist)

#
# Kernel loaders
#

$(LOADERS): %:
	make -C loaders/$*

$(LOADERS:%=%_implib): %_implib:

$(LOADERS:%=%_clean): %_clean:
	make -C loaders/$* clean

$(LOADERS:%=%_install): %_install:
	make -C loaders/$* install

$(LOADERS:%=%_dist): %_dist:
	make -C loaders/$* dist

.PHONY: $(LOADERS) $(LOADERS:%=%_implib) $(LOADERS:%=%_clean) $(LOADERS:%=%_install) \
        $(LOADERS:%=%_dist)

#
# Required system components
#

ntoskrnl:
	make -C ntoskrnl

ntoskrnl_implib:
	make -C ntoskrnl implib

ntoskrnl_clean:
	make -C ntoskrnl clean

ntoskrnl_install:
	make -C ntoskrnl install

ntoskrnl_dist:
	make -C ntoskrnl dist

.PHONY: ntoskrnl ntoskrnl_implib ntoskrnl_clean ntoskrnl_install ntoskrnl_dist

#
# Hardware Abstraction Layer import library
#

hallib:
	make -C hal/hal

hallib_implib:
	make -C hal/hal implib

hallib_clean:
	make -C hal/hal clean

hallib_install:
	make -C hal/hal install

hallib_dist:
	make -C hal/hal dist

.PHONY: hallib hallib_implib hallib_clean hallib_install hallib_dist

#
# Hardware Abstraction Layers
#

$(HALS): %:
	make -C hal/$*

$(HALS:%=%_implib): %_implib:
	make -C hal/$* implib

$(HALS:%=%_clean): %_clean:
	make -C hal/$* clean

$(HALS:%=%_install): %_install:
	make -C hal/$* install

$(HALS:%=%_dist): %_dist:
	make -C hal/$* dist

.PHONY: $(HALS) $(HALS:%=%_implib) $(HALS:%=%_clean) $(HALS:%=%_install) $(HALS:%=%_dist)

#
# Required DLLs
#

$(DLLS): %:
	make -C lib/$*

$(DLLS:%=%_depends): %_depends:
	make -C lib/$* depends

$(DLLS:%=%_implib): %_implib:
	make -C lib/$* implib

$(DLLS:%=%_clean): %_clean:
	make -C lib/$* clean

$(DLLS:%=%_install): %_install:
	make -C lib/$* install

$(DLLS:%=%_dist): %_dist:
	make -C lib/$* dist

.PHONY: $(DLLS) $(DLLS:%=%_depends) $(DLLS:%=%_implib) $(DLLS:%=%_clean) $(DLLS:%=%_install) $(DLLS:%=%_dist)

#
# Subsystem support modules
#

$(SUBSYS): %:
	make -C subsys/$*

$(SUBSYS:%=%_depends): %_depends:
	make -C subsys/$* depends

$(SUBSYS:%=%_implib): %_implib:
	make -C subsys/$* implib

$(SUBSYS:%=%_clean): %_clean:
	make -C subsys/$* clean

$(SUBSYS:%=%_install): %_install:
	make -C subsys/$* install

$(SUBSYS:%=%_dist): %_dist:
	make -C subsys/$* dist

.PHONY: $(SUBSYS) $(SUBSYS:%=%_depends) $(SUBSYS:%=%_implib) $(SUBSYS:%=%_clean) $(SUBSYS:%=%_install) \
        $(SUBSYS:%=%_dist)

#
# Create an installation
#

install_clean:
	$(RM) $(INSTALL_DIR)/system32/drivers/*.*
	$(RM) $(INSTALL_DIR)/system32/config/*.*
	$(RM) $(INSTALL_DIR)/system32/*.*
	$(RM) $(INSTALL_DIR)/symbols/*.*
	$(RM) $(INSTALL_DIR)/media/fonts/*.*
	$(RM) $(INSTALL_DIR)/media/*.*
	$(RM) $(INSTALL_DIR)/bin/*.*
	$(RM) $(INSTALL_DIR)/*.com
	$(RM) $(INSTALL_DIR)/*.bat
	$(RMDIR) $(INSTALL_DIR)/system32/drivers
	$(RMDIR) $(INSTALL_DIR)/system32/config
	$(RMDIR) $(INSTALL_DIR)/system32
	$(RMDIR) $(INSTALL_DIR)/symbols
	$(RMDIR) $(INSTALL_DIR)/media/fonts
	$(RMDIR) $(INSTALL_DIR)/media
	$(RMDIR) $(INSTALL_DIR)/bin
	$(RMDIR) $(INSTALL_DIR)

install_dirs:
	$(RMKDIR) $(INSTALL_DIR)
	$(RMKDIR) $(INSTALL_DIR)/bin
	$(RMKDIR) $(INSTALL_DIR)/media
	$(RMKDIR) $(INSTALL_DIR)/media/fonts
	$(RMKDIR) $(INSTALL_DIR)/symbols
	$(RMKDIR) $(INSTALL_DIR)/system32
	$(RMKDIR) $(INSTALL_DIR)/system32/config
	$(RMKDIR) $(INSTALL_DIR)/system32/drivers

install_before:
	$(CP) bootc.lst $(INSTALL_DIR)/bootc.lst
	$(CP) boot.bat $(INSTALL_DIR)/boot.bat
	$(CP) aboot.bat $(INSTALL_DIR)/aboot.bat
	$(CP) system.hiv $(INSTALL_DIR)/system32/config/system.hiv
	$(CP) media/fonts/helb____.ttf $(INSTALL_DIR)/media/fonts/helb____.ttf
	$(CP) media/fonts/timr____.ttf $(INSTALL_DIR)/media/fonts/timr____.ttf

.PHONY: install_clean install_dirs install_before


#
# Make a distribution saveset
#

dist_clean:
	$(RM) $(DIST_DIR)/symbols/*.sym
	$(RM) $(DIST_DIR)/drivers/*.sys
	$(RM) $(DIST_DIR)/subsys/*.exe
	$(RM) $(DIST_DIR)/dlls/*.dll
	$(RM) $(DIST_DIR)/apps/*.exe
	$(RM) $(DIST_DIR)/*.exe
	$(RMDIR) $(DIST_DIR)/symbols
	$(RMDIR) $(DIST_DIR)/subsys
	$(RMDIR) $(DIST_DIR)/drivers
	$(RMDIR) $(DIST_DIR)/dlls
	$(RMDIR) $(DIST_DIR)/apps
	$(RMDIR) $(DIST_DIR)

dist_dirs:
	$(RMKDIR) $(DIST_DIR)
	$(RMKDIR) $(DIST_DIR)/apps
	$(RMKDIR) $(DIST_DIR)/dlls
	$(RMKDIR) $(DIST_DIR)/drivers
	$(RMKDIR) $(DIST_DIR)/subsys
	$(RMKDIR) $(DIST_DIR)/symbols

.PHONY: dist_clean dist_dirs


etags:
	find . -name "*.[ch]" -print | etags --language=c -

# EOF

