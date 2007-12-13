CC	= gcc
# prepare check for gcc 3.3, $(GCC_3.3) will either be 0 or 1
GCC_3.3 := $(shell expr `$(CC) -dumpversion` \>= 3.3)

ETHERBOOT := yes
INCLUDE = -I$(TOPDIR)/grub -I$(TOPDIR)/include -I$(TOPDIR)/ -I./ -I$(TOPDIR)/fs/cdrom \
	-I$(TOPDIR)/fs/fatx -I$(TOPDIR)/lib/eeprom -I$(TOPDIR)/lib/crypt \
	-I$(TOPDIR)/drivers/video -I$(TOPDIR)/drivers/ide -I$(TOPDIR)/drivers/flash -I$(TOPDIR)/lib/misc \
	-I$(TOPDIR)/boot_xbe/ -I$(TOPDIR)/fs/grub -I$(TOPDIR)/lib/font \
	-I$(TOPDIR)/startuploader -I$(TOPDIR)/drivers/cpu \
	-I$(TOPDIR)/lib/jpeg/

CFLAGS	= -O2 -mcpu=pentium -Werror $(INCLUDE) -Wstrict-prototypes -fomit-frame-pointer -pipe

# add the option for gcc 3.3 only
ifeq ($(GCC_3.3), 1)
CFLAGS += -fno-zero-initialized-in-bss
endif

LD      = ld
OBJCOPY = objcopy

export CC

TOPDIR  := $(shell /bin/pwd)
SUBDIRS	= boot_rom fs drivers lib boot
#### Etherboot specific stuff
ifeq ($(ETHERBOOT), yes)
ETH_SUBDIRS = etherboot
CFLAGS	+= -DETHERBOOT
ETH_INCLUDE = 	-I$(TOPDIR)/etherboot/include -I$(TOPDIR)/etherboot/arch/i386/include	
ETH_CFLAGS  = 	-O2 -mcpu=pentium -Werror $(ETH_INCLUDE) -Wstrict-prototypes -fomit-frame-pointer -pipe -Ui386
endif

LDFLAGS-ROM     = -s -S -T $(TOPDIR)/scripts/ldscript-crom.ld
LDFLAGS-XBEBOOT = -s -S -T $(TOPDIR)/scripts/xbeboot.ld
LDFLAGS-ROMBOOT = -s -S -T $(TOPDIR)/boot_rom/bootrom.ld
LDFLAGS-VMLBOOT = -s -S -T $(TOPDIR)/boot_vml/vml_start.ld
ifeq ($(ETHERBOOT), yes)
LDFLAGS-ETHBOOT = -s -S -T $(TOPDIR)/boot_eth/eth_start.ld
endif

# add the option for gcc 3.3 only
ifeq ($(GCC_3.3), 1)
ETH_CFLAGS += -fno-zero-initialized-in-bss
endif
#### End Etherboot specific stuff

OBJECTS-XBE = $(TOPDIR)/boot_xbe/xbeboot.o

OBJECTS-VML = $(TOPDIR)/boot_vml/vml_Startup.o
ifeq ($(ETHERBOOT), yes)
OBJECTS-ETH = $(TOPDIR)/boot_eth/eth_Startup.o
endif
                                             
OBJECTS-ROMBOOT = $(TOPDIR)/obj/2bBootStartup.o
OBJECTS-ROMBOOT += $(TOPDIR)/obj/2bPicResponseAction.o
OBJECTS-ROMBOOT += $(TOPDIR)/obj/2bBootStartBios.o
OBJECTS-ROMBOOT += $(TOPDIR)/obj/sha1.o
OBJECTS-ROMBOOT += $(TOPDIR)/obj/2bBootLibrary.o
OBJECTS-ROMBOOT += $(TOPDIR)/obj/misc.o
                                             
OBJECTS-CROM = $(TOPDIR)/obj/BootStartup.o
OBJECTS-CROM += $(TOPDIR)/obj/BootResetAction.o
OBJECTS-CROM += $(TOPDIR)/obj/i2cio.o
OBJECTS-CROM += $(TOPDIR)/obj/pci.o
OBJECTS-CROM += $(TOPDIR)/obj/BootVgaInitialization.o
OBJECTS-CROM += $(TOPDIR)/obj/VideoInitialization.o
OBJECTS-CROM += $(TOPDIR)/obj/conexant.o
OBJECTS-CROM += $(TOPDIR)/obj/focus.o
OBJECTS-CROM += $(TOPDIR)/obj/xcalibur.o
OBJECTS-CROM += $(TOPDIR)/obj/BootIde.o
OBJECTS-CROM += $(TOPDIR)/obj/BootHddKey.o
OBJECTS-CROM += $(TOPDIR)/obj/rc4.o
OBJECTS-CROM += $(TOPDIR)/obj/sha1.o
OBJECTS-CROM += $(TOPDIR)/obj/BootVideoHelpers.o
OBJECTS-CROM += $(TOPDIR)/obj/vsprintf.o
OBJECTS-CROM += $(TOPDIR)/obj/IconMenu.o
OBJECTS-CROM += $(TOPDIR)/obj/TextMenu.o
OBJECTS-CROM += $(TOPDIR)/obj/TextMenuInit.o
OBJECTS-CROM += $(TOPDIR)/obj/MenuActions.o
OBJECTS-CROM += $(TOPDIR)/obj/BootFromDevice.o
OBJECTS-CROM += $(TOPDIR)/obj/LoadLinux.o
OBJECTS-CROM += $(TOPDIR)/obj/LoadReactOS.o
OBJECTS-CROM += $(TOPDIR)/obj/setup.o
OBJECTS-CROM += $(TOPDIR)/obj/iso9660.o
OBJECTS-CROM += $(TOPDIR)/obj/BootLibrary.o
OBJECTS-CROM += $(TOPDIR)/obj/cputools.o
OBJECTS-CROM += $(TOPDIR)/obj/microcode.o
OBJECTS-CROM += $(TOPDIR)/obj/ioapic.o
OBJECTS-CROM += $(TOPDIR)/obj/BootInterrupts.o
OBJECTS-CROM += $(TOPDIR)/obj/fsys_reiserfs.o
OBJECTS-CROM += $(TOPDIR)/obj/fsys_ext2fs.o
OBJECTS-CROM += $(TOPDIR)/obj/char_io.o
OBJECTS-CROM += $(TOPDIR)/obj/disk_io.o
OBJECTS-CROM += $(TOPDIR)/obj/decode-jpg.o
OBJECTS-CROM += $(TOPDIR)/obj/BootFlash.o
OBJECTS-CROM += $(TOPDIR)/obj/BootFlashUi.o
OBJECTS-CROM += $(TOPDIR)/obj/BootEEPROM.o
OBJECTS-CROM += $(TOPDIR)/obj/BootParser.o
OBJECTS-CROM += $(TOPDIR)/obj/BootFATX.o
#USB
OBJECTS-CROM += $(TOPDIR)/obj/config.o 
OBJECTS-CROM += $(TOPDIR)/obj/hcd-pci.o
OBJECTS-CROM += $(TOPDIR)/obj/hcd.o
OBJECTS-CROM += $(TOPDIR)/obj/hub.o
OBJECTS-CROM += $(TOPDIR)/obj/message.o
OBJECTS-CROM += $(TOPDIR)/obj/ohci-hcd.o
OBJECTS-CROM += $(TOPDIR)/obj/buffer_simple.o
OBJECTS-CROM += $(TOPDIR)/obj/urb.o
OBJECTS-CROM += $(TOPDIR)/obj/usb-debug.o
OBJECTS-CROM += $(TOPDIR)/obj/usb.o
OBJECTS-CROM += $(TOPDIR)/obj/BootUSB.o
OBJECTS-CROM += $(TOPDIR)/obj/usbwrapper.o
OBJECTS-CROM += $(TOPDIR)/obj/linuxwrapper.o
OBJECTS-CROM += $(TOPDIR)/obj/xpad.o
OBJECTS-CROM += $(TOPDIR)/obj/xremote.o
OBJECTS-CROM += $(TOPDIR)/obj/usbkey.o
OBJECTS-CROM += $(TOPDIR)/obj/risefall.o
#ETHERBOOT
ifeq ($(ETHERBOOT), yes)
OBJECTS-CROM += $(TOPDIR)/obj/nfs.o
OBJECTS-CROM += $(TOPDIR)/obj/nic.o
OBJECTS-CROM += $(TOPDIR)/obj/osloader.o
OBJECTS-CROM += $(TOPDIR)/obj/xbox.o
OBJECTS-CROM += $(TOPDIR)/obj/forcedeth.o
OBJECTS-CROM += $(TOPDIR)/obj/xbox_misc.o
OBJECTS-CROM += $(TOPDIR)/obj/xbox_pci.o
OBJECTS-CROM += $(TOPDIR)/obj/etherboot_config.o
OBJECTS-CROM += $(TOPDIR)/obj/xbox_main.o
OBJECTS-CROM += $(TOPDIR)/obj/elf.o
endif

RESOURCES = $(TOPDIR)/obj/backdrop.elf

export INCLUDE
export TOPDIR

ifeq ($(ETHERBOOT), yes)
BOOT_ETH_DIR = boot_eth/ethboot
BOOT_ETH_SUBDIRS = ethsubdirs
endif

all: clean resources $(BOOT_ETH_SUBDIRS) cromsubdirs default.xbe vmlboot $(BOOT_ETH_DIR) image.bin imagecompress 

ifeq ($(ETHERBOOT), yes)
ethsubdirs: $(patsubst %, _dir_%, $(ETH_SUBDIRS))
$(patsubst %, _dir_%, $(ETH_SUBDIRS)) : dummy
	$(MAKE) CFLAGS="$(ETH_CFLAGS)" -C $(patsubst _dir_%, %, $@)
endif

cromsubdirs: $(patsubst %, _dir_%, $(SUBDIRS))
$(patsubst %, _dir_%, $(SUBDIRS)) : dummy
	$(MAKE) CFLAGS="$(CFLAGS)" -C $(patsubst _dir_%, %, $@)

dummy:

resources:
	# Background
	${LD} -r --oformat elf32-i386 -o $(TOPDIR)/obj/backdrop.elf -T $(TOPDIR)/scripts/backdrop.ld -b binary $(TOPDIR)/pics/backdrop.jpg	

install:
	lmilk -f -p $(TOPDIR)/image/image.bin
	lmilk -f -a c0000 -p $(TOPDIR)/image/image.bin -q	
clean:
	find . \( -name '*.[oas]' -o -name core -o -name '.*.flags' \) -type f -print \
		| grep -v lxdialog/ | xargs rm -f
	rm -f $(TOPDIR)/obj/*.gz 
	rm -f $(TOPDIR)/obj/*.bin 
	rm -f $(TOPDIR)/obj/*.elf
	rm -f $(TOPDIR)/image/*.bin 
	rm -f $(TOPDIR)/image/*.xbe 
	rm -f $(TOPDIR)/xbe/*.xbe $(TOPDIR)/xbe/*.bin
	rm -f $(TOPDIR)/xbe/*.elf
	rm -f $(TOPDIR)/image/*.bin
	rm -f $(TOPDIR)/bin/imagebld*
	rm -f $(TOPDIR)/boot_vml/disk/vmlboot
	rm -f boot_eth/ethboot
	mkdir -p $(TOPDIR)/xbe 
	mkdir -p $(TOPDIR)/image
	mkdir -p $(TOPDIR)/obj 
	mkdir -p $(TOPDIR)/bin

obj/image-crom.bin:
	${LD} -o obj/image-crom.elf ${OBJECTS-CROM} ${RESOURCES} ${LDFLAGS-ROM}
	${OBJCOPY} --output-target=binary --strip-all obj/image-crom.elf $@

vmlboot: ${OBJECTS-VML}
	${LD} -o $(TOPDIR)/obj/vmlboot.elf ${OBJECTS-VML} ${LDFLAGS-VMLBOOT}
	${OBJCOPY} --output-target=binary --strip-all $(TOPDIR)/obj/vmlboot.elf $(TOPDIR)/boot_vml/disk/$@

ifeq ($(ETHERBOOT), yes)
boot_eth/ethboot: ${OBJECTS-ETH} obj/image-crom.bin
	${LD} -o obj/ethboot.elf ${OBJECTS-ETH} -b binary obj/image-crom.bin ${LDFLAGS-ETHBOOT}
	${OBJCOPY} --output-target=binary --strip-all obj/ethboot.elf obj/ethboot.bin
	perl -I boot_eth boot_eth/mknbi.pl --output=$@ obj/ethboot.bin
endif

default.xbe: ${OBJECTS-XBE}
	${LD} -o $(TOPDIR)/obj/default.elf ${OBJECTS-XBE} ${LDFLAGS-XBEBOOT}
	${OBJCOPY} --output-target=binary --strip-all $(TOPDIR)/obj/default.elf $(TOPDIR)/xbe/$@

image.bin:
	${LD} -o $(TOPDIR)/obj/2lbimage.elf ${OBJECTS-ROMBOOT} ${LDFLAGS-ROMBOOT}
	${OBJCOPY} --output-target=binary --strip-all $(TOPDIR)/obj/2lbimage.elf $(TOPDIR)/obj/2blimage.bin

# This is a local executable, so don't use a cross compiler...
bin/imagebld: lib/imagebld/imagebld.c lib/crypt/sha1.c
	gcc -Ilib/crypt -o bin/sha1.o -c lib/crypt/sha1.c
	gcc -Ilib/crypt -o bin/imagebld.o -c lib/imagebld/imagebld.c
	gcc -o bin/imagebld bin/imagebld.o bin/sha1.o 
	
imagecompress: obj/image-crom.bin bin/imagebld
	cp obj/image-crom.bin obj/image-crom.bin.tmp
	gzip -9 obj/image-crom.bin.tmp
	bin/imagebld -rom obj/2blimage.bin obj/image-crom.bin.tmp.gz image/image.bin image/image_1024.bin
	bin/imagebld -xbe xbe/default.xbe obj/image-crom.bin
	bin/imagebld -vml boot_vml/disk/vmlboot obj/image-crom.bin 

