/*****************************************************************************
 *  FullFAT - High Performance, Thread-Safe Embedded FAT File-System         *
 *  Copyright (C) 2009  James Walmsley (james@worm.me.uk)                    *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                           *
 *  IMPORTANT NOTICE:                                                        *
 *  =================                                                        *
 *  Alternative Licensing is available directly from the Copyright holder,   *
 *  (James Walmsley). For more information consult LICENSING.TXT to obtain   *
 *  a Commercial license.                                                    *
 *                                                                           *
 *  See RESTRICTIONS.TXT for extra restrictions on the use of FullFAT.       *
 *                                                                           *
 *  Removing the above notice is illegal and will invalidate this license.   *
 *****************************************************************************
 *  See http://worm.me.uk/fullfat for more information.                      *
 *  Or  http://fullfat.googlecode.com/ for latest releases and the wiki.     *
 *****************************************************************************/
#ifndef _FF_FATDEF_H_
#define _FF_FATDEF_H_

/*
	This file defines offsets to various data for the FAT specification.
*/

// MBR / PBR Offsets

#define FF_FAT_BYTES_PER_SECTOR		0x00B
#define FF_FAT_SECTORS_PER_CLUS		0x00D
#define FF_FAT_RESERVED_SECTORS		0x00E
#define FF_FAT_NUMBER_OF_FATS		0x010
#define FF_FAT_ROOT_ENTRY_COUNT		0x011
#define FF_FAT_16_TOTAL_SECTORS		0x013
#define FF_FAT_32_TOTAL_SECTORS		0x020
#define FF_FAT_16_SECTORS_PER_FAT	0x016
#define FF_FAT_32_SECTORS_PER_FAT	0x024
#define FF_FAT_ROOT_DIR_CLUSTER		0x02C

#define FF_FAT_16_VOL_LABEL			0x02B
#define FF_FAT_32_VOL_LABEL			0x047

#define FF_FAT_PTBL					0x1BE
#define FF_FAT_PTBL_LBA				0x008
#define FF_FAT_PTBL_ACTIVE          0x000
#define FF_FAT_PTBL_ID              0x004

#define FF_FAT_MBR_SIGNATURE        0x1FE

#define FF_FAT_DELETED				0xE5

// Directory Entry Offsets
#define FF_FAT_DIRENT_SHORTNAME		0x000
#define FF_FAT_DIRENT_ATTRIB		0x00B
#define FF_FAT_DIRENT_CREATE_TIME	0x00E	///< Creation Time.
#define FF_FAT_DIRENT_CREATE_DATE	0x010	///< Creation Date.
#define FF_FAT_DIRENT_LASTACC_DATE	0x012	///< Date of Last Access.
#define FF_FAT_DIRENT_CLUS_HIGH		0x014
#define FF_FAT_DIRENT_LASTMOD_TIME	0x016	///< Time of Last modification.
#define FF_FAT_DIRENT_LASTMOD_DATE	0x018	///< Date of Last modification.
#define FF_FAT_DIRENT_CLUS_LOW		0x01A
#define FF_FAT_DIRENT_FILESIZE		0x01C
#define FF_FAT_LFN_ORD				0x000
#define FF_FAT_LFN_NAME_1			0x001
#define	FF_FAT_LFN_CHECKSUM			0x00D
#define FF_FAT_LFN_NAME_2			0x00E
#define FF_FAT_LFN_NAME_3			0x01C

// Dirent Attributes
#define FF_FAT_ATTR_READONLY		0x01
#define FF_FAT_ATTR_HIDDEN			0x02
#define FF_FAT_ATTR_SYSTEM			0x04
#define FF_FAT_ATTR_VOLID			0x08
#define FF_FAT_ATTR_DIR				0x10
#define FF_FAT_ATTR_ARCHIVE			0x20
#define FF_FAT_ATTR_LFN				0x0F

/**
 * -- Hein_Tibosch additions for mixed case in shortnames --
 *
 * Specifically, bit 4 means lowercase extension and bit 3 lowercase basename,
 * which allows for combinations such as "example.TXT" or "HELLO.txt" but not "Mixed.txt"
 */

#define FF_FAT_CASE_OFFS			0x0C	///< After NT/XP : 2 case bits
#define FF_FAT_CASE_ATTR_BASE		0x08
#define FF_FAT_CASE_ATTR_EXT		0x10

#if defined(FF_LFN_SUPPORT) && defined(FF_INCLUDE_SHORT_NAME)
#define FF_FAT_ATTR_IS_LFN			0x40	///< artificial attribute, for debugging only
#endif

#endif

