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

PATH_TO_TOP = ../..

include rules.mak

all:
	$(MAKE) -C tools
	$(MAKE) -C bootsect
	$(MAKE) -C freeldr
	$(MAKE) -C install
	$(MAKE) -C fdebug

test:

clean:
	$(MAKE) -C bootsect clean
	$(MAKE) -C freeldr clean
	$(MAKE) -C install clean
	$(MAKE) -C fdebug clean
	$(MAKE) -C tools clean

bootcd:
	$(CP) bootsect/isoboot.bin ${BOOTCD_DIR}/../isoboot.bin
	$(CP) bootsect/dosmbr.bin ${BOOTCD_DIR}/loader/dosmbr.bin
	$(CP) bootsect/ext2.bin ${BOOTCD_DIR}/loader/ext2.bin
	$(CP) bootsect/fat.bin ${BOOTCD_DIR}/loader/fat.bin
	$(CP) bootsect/fat32.bin ${BOOTCD_DIR}/loader/fat32.bin
	$(CP) bootsect/isoboot.bin ${BOOTCD_DIR}/loader/isoboot.bin
	$(CP) freeldr/freeldr.sys ${BOOTCD_DIR}/loader/freeldr.sys
	$(CP) freeldr/setupldr.sys ${BOOTCD_DIR}/loader/setupldr.sys

.PHONY : clean
