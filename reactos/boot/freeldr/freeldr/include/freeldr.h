/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __FREELDR_H
#define __FREELDR_H

#define UINT64_C(val) val##ULL
#define RVA(m, b) ((PVOID)((ULONG_PTR)(b) + (ULONG_PTR)(m)))

#define ROUND_DOWN(n, align) \
    (((ULONG)n) & ~((align) - 1l))

#define ROUND_UP(n, align) \
    ROUND_DOWN(((ULONG)n) + (align) - 1, (align))

/* Public headers */
#ifdef __REACTOS__
#define NTOSAPI
#define printf TuiPrintf
#include <ntddk.h>
#include <ntifs.h>
#include <ioaccess.h>
#include <arc/arc.h>
#include <ketypes.h>
#include <mmtypes.h>
#include <ndk/asm.h>
#include <ndk/rtlfuncs.h>
#include <ndk/ldrtypes.h>
#include <ndk/halfuncs.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <ntdddisk.h>
#include <internal/hal.h>
#include <drivers/pci/pci.h>
#include <winerror.h>
#else
#include <ntsup.h>
#endif

/* Internal headers */
// #include <arcemul.h>
#include <bytesex.h>
#include <cache.h>
#include <cmdline.h>
#include <comm.h>
#include <disk.h>
#include <fs.h>
#include <hal.h>
#include <inffile.h>
#include <inifile.h>
#include <keycodes.h>
#include <linux.h>
#include <custom.h>
#include <miscboot.h>
#include <machine.h>
#include <mm.h>
#include <multiboot.h>
#include <options.h>
#include <oslist.h>
#include <ramdisk.h>
#include <reactos.h>
#include <registry.h>
#include <ui.h>
#include <ver.h>
#include <video.h>
#include <winldr.h>

/* File system headers */
#include <fs/ext2.h>
#include <fs/fat.h>
#include <fs/ntfs.h>
#include <fs/iso.h>
#include <fs/pxe.h>

/* UI support */
#include <ui/gui.h>
#include <ui/minitui.h>
#include <ui/noui.h>
#include <ui/tui.h>

/* Arch specific includes */
#if defined(_M_IX86) || defined(_M_AMD64)
#include <arch/pc/hardware.h>
#include <arch/pc/pcbios.h>
#include <arch/pc/machpc.h>
#include <arch/pc/x86common.h>
#include <arch/pc/pxe.h>
#include <arch/i386/drivemap.h>
#endif
#if defined(_M_IX86)
#include <arch/i386/i386.h>
#include <arch/i386/machxbox.h>
#include <internal/i386/intrin_i.h>
#elif defined(_M_AMD64)
#include <arch/amd64/amd64.h>
#include <internal/amd64/intrin_i.h>
#elif defined(_M_PPC)
#include <arch/powerpc/hardware.h>
#elif defined(_M_ARM)
#include <arch/arm/hardware.h>
#elif defined(_M_MIPS)
#include <arch/mips/arcbios.h>
#endif

VOID BootMain(LPSTR CmdLine);
VOID LoadOperatingSystem(IN OperatingSystemItem* OperatingSystem);
VOID RunLoader(VOID);
VOID FrLdrCheckCpuCompatiblity(VOID);

#endif  /* __FREELDR_H */
