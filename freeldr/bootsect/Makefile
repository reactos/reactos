#
#  FreeLoader
#  Copyright (C) 1999, 2000, 2001  Brian Palmer  <brianp@sginet.com>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

RM			= cmd /C del
CP			= copy /Y 
MKDIR		= md
NASM_CMD	= nasm
BIN2C		= ../tools/bin2c
TOOLSDIR	= ../tools
BOOTCD_DIR	= ..\..\bootcd

.PHONY : clean bootcd

all: $(BIN2C) fat.bin fat32.bin isoboot.bin ext2.bin


$(BIN2C) :
	@$(MAKE) --no-print-directory -C $(TOOLSDIR)

fat.bin : fat.asm $(BIN2C)
	@echo ===================================================== Assembling fat
	@$(NASM_CMD) -o fat.bin -f bin fat.asm
	@$(BIN2C) fat.bin fat.h fat_data


fat32.bin : fat32.asm $(BIN2C)
	@echo ===================================================== Assembling fat32
	@$(NASM_CMD) -o fat32.bin -f bin fat32.asm
	@$(BIN2C) fat32.bin fat32.h fat32_data

isoboot.bin : isoboot.asm
	@echo ===================================================== Assembling isoboot
	@$(NASM_CMD) -o isoboot.bin -f bin isoboot.asm

ext2.bin : ext2.asm
	@echo ===================================================== Assembling ext2
	@$(NASM_CMD) -o ext2.bin -f bin ext2.asm
	@$(BIN2C) ext2.bin ext2.h ext2_data


.PHONY : bootcd
bootcd: bootcd_dirs isoboot.bin
	$(CP) isoboot.bin $(BOOTCD_DIR)

.PHONY : bootcd_dirs
bootcd_dirs:
	$(MKDIR) $(BOOTCD_DIR)
	$(MKDIR) $(BOOTCD_DIR)\disk
	$(MKDIR) $(BOOTCD_DIR)\disk\reactos
	$(MKDIR) $(BOOTCD_DIR)\disk\install
	$(MKDIR) $(BOOTCD_DIR)\disk\bootdisk

clean:
	@-$(RM) *.bin
	@-$(RM) *.h
	@echo Clean ALL done.
