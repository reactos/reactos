#
# Global makefile
#

#
# Select your host
#
#HOST = mingw32-linux
#HOST = mingw32-windows

include rules.mak

#
# Required to run the system
#
COMPONENTS = iface_native iface_additional ntoskrnl
DLLS = ntdll kernel32 crtdll advapi32 fmifs gdi32 secur32 user32 ws2_32
SUBSYS = smss win32k csrss

#
# Select the server(s) you want to build
#
SERVERS = win32
# SERVERS = posix linux os2

#
# Select the loader(s) you want to build
#
LOADERS = dos
# LOADERS = boot

#
# Select the device drivers and filesystems you want
#
DEVICE_DRIVERS = vidport vga blue ide null floppy
# DEVICE_DRIVERS = beep event floppy ide_test mouse sound test test1 parallel serial

INPUT_DRIVERS = keyboard

FS_DRIVERS = vfat
# FS_DRIVERS = minix ext2 template

# ndis tdi tcpip tditest wshtcpip
NET_DRIVERS = ndis tcpip tditest wshtcpip

# ne2000
NET_DEVICE_DRIVERS = ne2000

#
# system applications (required for startup)
#
SYS_APPS = shell winlogon services

APPS = args hello test cat bench apc shm lpc thread event file gditest \
       pteb consume dump_shared_data vmtest regtest
#       objdir

# ping
NET_APPS = ping


KERNEL_SERVICES = $(DEVICE_DRIVERS) $(INPUT_DRIVERS) $(FS_DRIVERS) $(NET_DRIVERS) $(NET_DEVICE_DRIVERS)

all: buildno $(COMPONENTS) $(DLLS) $(SUBSYS) $(LOADERS) $(KERNEL_SERVICES) $(SYS_APPS) $(APPS) $(NET_APPS)

.PHONY: all

clean: buildno_clean $(COMPONENTS:%=%_clean) $(DLLS:%=%_clean) $(LOADERS:%=%_clean) \
       $(KERNEL_SERVICES:%=%_clean) $(SUBSYS:%=%_clean) $(SYS_APPS:%=%_clean) $(APPS:%=%_clean)

.PHONY: clean

ifeq ($(HOST),mingw32-linux)
rcopy$(EXE_POSTFIX): rcopy.c
	$(HOST_CC) -g -DUNIX_PATHS rcopy.c -o rcopy$(EXE_POSTFIX)
endif
ifeq ($(HOST),mingw32-windows)
rcopy$(EXE_POSTFIX): rcopy.c
	$(HOST_CC) -g -DDOS_PATHS rcopy.c -o rcopy$(EXE_POSTFIX)
endif

ifeq ($(HOST),mingw32-linux)
rmkdir$(EXE_POSTFIX): rmkdir.c
	$(HOST_CC) -g -DUNIX_PATHS rmkdir.c -o rmkdir$(EXE_POSTFIX)
endif
ifeq ($(HOST),mingw32-windows)
rmkdir$(EXE_POSTFIX): rmkdir.c
	$(HOST_CC) -g -DDOS_PATHS rmkdir.c -o rmkdir$(EXE_POSTFIX)
endif


install: rcopy$(EXE_POSTFIX) rmkdir$(EXE_POSTFIX) make_install_dirs autoexec_install $(COMPONENTS:%=%_install) \
        $(DLLS:%=%_install) $(LOADERS:%=%_install) \
        $(KERNEL_SERVICES:%=%_install) $(SUBSYS:%=%_install) \
        $(SYS_APPS:%=%_install) $(APPS:%=%_install)

dist: rcopy$(EXE_POSTFIX) clean_dist_dir make_dist_dirs $(COMPONENTS:%=%_dist) $(DLLS:%=%_dist) \
      $(LOADERS:%=%_dist) $(KERNEL_SERVICES:%=%_dist) $(SUBSYS:%=%_dist) \
      $(SYS_APPS:%=%_dist) $(APPS:%=%_dist)

#
# Build number generator
#
buildno: include/reactos/version.h
	make -C apps/buildno

buildno_clean:
	make -C apps/buildno clean

buildno_dist:

buildno_install:

.PHONY: buildno buildno_clean buildno_dist buildno_install

#
# System Applications
#
$(SYS_APPS): %:
	make -C apps/system/$*

$(SYS_APPS:%=%_clean): %_clean:
	make -C apps/system/$* clean

$(SYS_APPS:%=%_dist): %_dist:
	make -C apps/system/$* dist

$(SYS_APPS:%=%_install): %_install:
	make -C apps/system/$* install

.PHONY: $(SYS_APPS) $(SYS_APPS:%=%_clean) $(SYS_APPS:%=%_install) $(SYS_APPS:%=%_dist)

#
# Applications
#
$(APPS): %:
	make -C apps/$*

$(APPS:%=%_clean): %_clean:
	make -C apps/$* clean

$(APPS:%=%_dist): %_dist:
	make -C apps/$* dist

$(APPS:%=%_install): %_install:
	make -C apps/$* install

.PHONY: $(APPS) $(APPS:%=%_clean) $(APPS:%=%_install) $(APPS:%=%_dist)

#
# Network applications
#
$(NET_APPS): %:
	make -C apps/net/$*

$(NET_APPS:%=%_clean): %_clean:
	make -C apps/net/$* clean

$(NET_APPS:%=%_dist): %_dist:
	make -C apps/net/$* dist

$(NET_APPS:%=%_install): %_install:
	make -C apps/net/$* install

.PHONY: $(NET_APPS) $(NET_APPS:%=%_clean) $(NET_APPS:%=%_install) $(NET_APPS:%=%_dist)

#
# Interfaces
#
iface_native:
	make -C iface/native

iface_native_clean:
	make -C iface/native clean

iface_native_install:

iface_native_dist:

iface_additional:
	make -C iface/addsys

iface_additional_clean:
	make -C iface/addsys clean

iface_additional_install:

iface_additional_dist:

.PHONY: iface_native iface_native_clean iface_native_install \
        iface_native_dist \
        iface_additional iface_additional_clean iface_additional_install \
        iface_additional_dist

#
# Device driver rules
#
$(DEVICE_DRIVERS): %:
	make -C services/dd/$*

$(DEVICE_DRIVERS:%=%_clean): %_clean:
	make -C services/dd/$* clean

$(DEVICE_DRIVERS:%=%_install): %_install:
	make -C services/dd/$* install

$(DEVICE_DRIVERS:%=%_dist): %_dist:
	make -C services/dd/$* dist

.PHONY: $(DEVICE_DRIVERS) $(DEVICE_DRIVERS:%=%_clean) \
        $(DEVICE_DRIVERS:%=%_install) $(DEVICE_DRIVERS:%=%_dist)

#
# Input driver rules
#
$(INPUT_DRIVERS): %:
	make -C services/input/$*

$(INPUT_DRIVERS:%=%_clean): %_clean:
	make -C services/input/$* clean

$(INPUT_DRIVERS:%=%_install): %_install:
	make -C services/input/$* install

$(INPUT_DRIVERS:%=%_dist): %_dist:
	make -C services/input/$* dist

.PHONY: $(INPUT_DRIVERS) $(INPUT_DRIVERS:%=%_clean) \
        $(INPUT_DRIVERS:%=%_install) $(INPUT_DRIVERS:%=%_dist)

$(FS_DRIVERS): %:
	make -C services/fs/$*

$(FS_DRIVERS:%=%_clean): %_clean:
	make -C services/fs/$* clean

$(FS_DRIVERS:%=%_install): %_install:
	make -C services/fs/$* install

$(FS_DRIVERS:%=%_dist): %_dist:
	make -C services/fs/$* dist

.PHONY: $(FS_DRIVERS) $(FS_DRIVERS:%=%_clean) $(FS_DRIVERS:%=%_install) \
        $(FS_DRIVERS:%=%_dist)

$(NET_DRIVERS): %:
	make -C services/net/$*

$(NET_DRIVERS:%=%_clean): %_clean:
	make -C services/net/$* clean

$(NET_DRIVERS:%=%_install): %_install:
	make -C services/net/$* install

$(NET_DRIVERS:%=%_dist): %_dist:
	make -C services/net/$* dist

.PHONY: $(NET_DRIVERS) $(NET_DRIVERS:%=%_clean) $(NET_DRIVERS:%=%_install) \
        $(NET_DRIVERS:%=%_dist)

$(NET_DEVICE_DRIVERS): %:
	make -C services/net/dd/$*

$(NET_DEVICE_DRIVERS:%=%_clean): %_clean:
	make -C services/net/dd/$* clean

$(NET_DEVICE_DRIVERS:%=%_install): %_install:
	make -C services/net/dd/$* install

$(NET_DEVICE_DRIVERS:%=%_dist): %_dist:
	make -C services/net/dd/$* dist

.PHONY: $(NET_DEVICE_DRIVERS) $(NET_DEVICE_DRIVERS:%=%_clean) \
        $(NET_DEVICE_DRIVERS:%=%_install) $(NET_DEVICE_DRIVERS:%=%_dist)

#
# Kernel loaders
#

$(LOADERS): %:
	make -C loaders/$*

$(LOADERS:%=%_clean): %_clean:
	make -C loaders/$* clean

$(LOADERS:%=%_install): %_install:
	make -C loaders/$* install

$(LOADERS:%=%_dist): %_dist:
	make -C loaders/$* dist

.PHONY: $(LOADERS) $(LOADERS:%=%_clean) $(LOADERS:%=%_install) \
        $(LOADERS:%=%_dist)

#
# Required system components
#

ntoskrnl:
	make -C ntoskrnl

ntoskrnl_clean:
	make -C ntoskrnl clean

ntoskrnl_install:
	make -C ntoskrnl install

ntoskrnl_dist:
	make -C ntoskrnl dist

.PHONY: ntoskrnl ntoskrnl_clean ntoskrnl_install ntoskrnl_dist

#
# Required DLLs
#

$(DLLS): %:
	make -C lib/$*

$(DLLS:%=%_clean): %_clean:
	make -C lib/$* clean

$(DLLS:%=%_install): %_install:
	make -C lib/$* install

$(DLLS:%=%_dist): %_dist:
	make -C lib/$* dist

.PHONY: $(DLLS) $(DLLS:%=%_clean) $(DLLS:%=%_install) $(DLLS:%=%_dist)

#
# Kernel Subsystems
#

$(SUBSYS): %:
	make -C subsys/$*

$(SUBSYS:%=%_clean): %_clean:
	make -C subsys/$* clean

$(SUBSYS:%=%_install): %_install:
	make -C subsys/$* install

$(SUBSYS:%=%_dist): %_dist:
	make -C subsys/$* dist

.PHONY: $(SUBSYS) $(SUBSYS:%=%_clean) $(SUBSYS:%=%_install) \
        $(SUBSYS:%=%_dist)

#
# Make an install floppy
#
make_install_dirs:
	./rmkdir $(FLOPPY_DIR)/dlls
	./rmkdir $(FLOPPY_DIR)/apps
	./rmkdir $(FLOPPY_DIR)/drivers
	./rmkdir $(FLOPPY_DIR)/subsys


.PHONY: make_install_dirs

autoexec_install: $(FLOPPY_DIR)/autoexec.bat

$(FLOPPY_DIR)/autoexec.bat: bootflop.bat
	$(CP) bootflop.bat $(FLOPPY_DIR)/autoexec.bat

#
# Make a distribution saveset
#

clean_dist_dir:
ifeq ($(DOSCLI),yes)
	- $(RM) $(DIST_DIR)\dlls\*.dll
	- $(RM) $(DIST_DIR)\apps\*.exe
	- $(RM) $(DIST_DIR)\drivers\*.sys
	- $(RM) $(DIST_DIR)\subsys\*.exe
	- $(RMDIR) $(DIST_DIR)\dlls
	- $(RMDIR) $(DIST_DIR)\apps
	- $(RMDIR) $(DIST_DIR)\drivers
	- $(RMDIR) $(DIST_DIR)\subsys
	- $(RMDIR) $(DIST_DIR)
else
	$(RM) -r $(DIST_DIR)
endif

make_dist_dirs: ./rmkdir
	./rmkdir $(DIST_DIR)
	./rmkdir $(DIST_DIR)/dlls
	./rmkdir $(DIST_DIR)/apps
	./rmkdir $(DIST_DIR)/drivers
	./rmkdir $(DIST_DIR)/dlls
	./rmkdir $(DIST_DIR)/subsys

.PHONY: clean_dist_dir make_dist_dirs

#
#
#
etags:
	find . -name "*.[ch]" -print | etags --language=c -

# EOF
