#
# Global makefile
#

#
# Select your host
#
#HOST = mingw32-linux
HOST = mingw32-windows

include rules.mak

#
# Required to run the system
#
COMPONENTS = iface_native iface_additional ntoskrnl
DLLS = ntdll kernel32 crtdll advapi32 fmifs gdi32 secur32
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
DEVICE_DRIVERS = vidport vga blue ide keyboard null parallel serial floppy
# DEVICE_DRIVERS = beep event floppy ide_test mouse sound test test1

FS_DRIVERS = vfat
# FS_DRIVERS = minix ext2 template

# ndis tdi tcpip tditest
NET_DRIVERS = ndis tcpip tditest

KERNEL_SERVICES = $(DEVICE_DRIVERS) $(FS_DRIVERS) $(NET_DRIVERS)

APPS = args hello shell test cat bench apc shm lpc thread event file gditest \
       pteb consume dump_shared_data

#       objdir

all: buildno $(COMPONENTS) $(DLLS) $(SUBSYS) $(LOADERS) $(KERNEL_SERVICES) $(APPS)

.PHONY: all

clean: buildno_clean $(COMPONENTS:%=%_clean) $(DLLS:%=%_clean) $(LOADERS:%=%_clean) \
       $(KERNEL_SERVICES:%=%_clean) $(SUBSYS:%=%_clean) $(APPS:%=%_clean)

.PHONY: clean

install: make_install_dirs autoexec_install $(COMPONENTS:%=%_install) \
        $(DLLS:%=%_install) $(LOADERS:%=%_install) \
        $(KERNEL_SERVICES:%=%_install) $(SUBSYS:%=%_install) \
        $(APPS:%=%_install)

dist: clean_dist_dir make_dist_dirs $(COMPONENTS:%=%_dist) $(DLLS:%=%_dist) \
      $(LOADERS:%=%_dist) $(KERNEL_SERVICES:%=%_dist) $(SUBSYS:%=%_dist) \
      $(APPS:%=%_dist)

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

install: all
	./install.sh /mnt/hda1
	./install.sh /mnt/hda4
	./install.bochs

make_install_dirs:
ifeq ($(DOSCLI),yes)
	mkdir $(FLOPPY_DIR)\dlls
	mkdir $(FLOPPY_DIR)\apps
	mkdir $(FLOPPY_DIR)\drivers
	mkdir $(FLOPPY_DIR)\subsys
else
	mkdir $(FLOPPY_DIR)/dlls $(FLOPPY_DIR)/apps $(FLOPPY_DIR)/drivers
	mkdir $(FLOPPY_DIR)/subsys
endif

.PHONY: make_install_dirs

autoexec_install: $(FLOPPY_DIR)/autoexec.bat

$(FLOPPY_DIR)/autoexec.bat: bootflop.bat
ifeq ($(DOSCLI),yes)
	$(CP) bootflop.bat $(FLOPPY_DIR)\autoexec.bat
else
	$(CP) bootflop.bat $(FLOPPY_DIR)/autoexec.bat
endif

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

make_dist_dirs:
ifeq ($(DOSCLI),yes)
	mkdir $(DIST_DIR)
	mkdir $(DIST_DIR)\dlls
	mkdir $(DIST_DIR)\apps
	mkdir $(DIST_DIR)\drivers
	mkdir $(DIST_DIR)\dlls
	mkdir $(DIST_DIR)\subsys
else
	mkdir $(DIST_DIR) $(DIST_DIR)/dlls $(DIST_DIR)/apps $(DIST_DIR)/drivers
	mkdir $(DIST_DIR)/subsys
endif

.PHONY: clean_dist_dir make_dist_dirs

#
#
#
etags:
	find . -name "*.[ch]" -print | etags --language=c -

# EOF
