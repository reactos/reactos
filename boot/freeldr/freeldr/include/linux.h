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

#include <fs.h>
#include <oslist.h>

#ifndef __LINUX_H
#define __LINUX_H

#if defined(_M_IX86) || defined(_M_AMD64)

#define LINUX_LOADER_TYPE_LILO          0x01
#define LINUX_LOADER_TYPE_LOADLIN       0x11
#define LINUX_LOADER_TYPE_BOOTSECT      0x21
#define LINUX_LOADER_TYPE_SYSLINUX      0x31
#define LINUX_LOADER_TYPE_ETHERBOOT     0x41
#define LINUX_LOADER_TYPE_FREELOADER    0x81

#define LINUX_COMMAND_LINE_MAGIC        0xA33F

#define LINUX_SETUP_HEADER_ID           0x53726448  // 'HdrS'

#define LINUX_BOOT_SECTOR_MAGIC         0xAA55

#define LINUX_KERNEL_LOAD_ADDRESS       0x100000

#define LINUX_FLAG_LOAD_HIGH            0x01
#define LINUX_FLAG_CAN_USE_HEAP         0x80

#define LINUX_MAX_INITRD_ADDRESS        0x38000000

#include <pshpack1.h>
typedef struct
{
    UCHAR        BootCode1[0x20];

    USHORT        CommandLineMagic;
    USHORT        CommandLineOffset;

    UCHAR        BootCode2[0x1CD];

    UCHAR        SetupSectors;
    USHORT        RootFlags;
    USHORT        SystemSize;
    USHORT        SwapDevice;
    USHORT        RamSize;
    USHORT        VideoMode;
    USHORT        RootDevice;
    USHORT        BootFlag;            // 0xAA55

} LINUX_BOOTSECTOR, *PLINUX_BOOTSECTOR;

typedef struct
{
    UCHAR        JumpInstruction[2];
    ULONG        SetupHeaderSignature;    // Signature for SETUP-header
    USHORT        Version;                // Version number of header format
    USHORT        RealModeSwitch;            // Default switch
    USHORT        SetupSeg;                // SETUPSEG
    USHORT        StartSystemSeg;
    USHORT        KernelVersion;            // Offset to kernel version string
    UCHAR        TypeOfLoader;            // Loader ID
                                    // =0, old one (LILO, Loadlin,
                                    //     Bootlin, SYSLX, bootsect...)
                                    // else it is set by the loader:
                                    // 0xTV: T=0 for LILO
                                    //       T=1 for Loadlin
                                    //       T=2 for bootsect-loader
                                    //       T=3 for SYSLX
                                    //       T=4 for ETHERBOOT
                                    //       V = version

    UCHAR        LoadFlags;                // flags, unused bits must be zero (RFU)
                                    // LOADED_HIGH = 1
                                    // bit within loadflags,
                                    // if set, then the kernel is loaded high
                                    // CAN_USE_HEAP = 0x80
                                    // if set, the loader also has set heap_end_ptr
                                    // to tell how much space behind setup.S
                                    // can be used for heap purposes.
                                    // Only the loader knows what is free!

    USHORT        SetupMoveSize;            // size to move, when we (setup) are not
                                    // loaded at 0x90000. We will move ourselves
                                    // to 0x90000 then just before jumping into
                                    // the kernel. However, only the loader
                                    // know how much of data behind us also needs
                                    // to be loaded.

    ULONG        Code32Start;            // here loaders can put a different
                                    // start address for 32-bit code.
                                    //
                                    //   0x1000 = default for zImage
                                    //
                                    // 0x100000 = default for big kernel

    ULONG        RamdiskAddress;            // address of loaded ramdisk image
                                    // Here the loader (or kernel generator) puts
                                    // the 32-bit address were it loaded the image.
    ULONG        RamdiskSize;            // its size in bytes

    USHORT        BootSectKludgeOffset;
    USHORT        BootSectKludgeSegment;
    USHORT        HeapEnd;                // space from here (exclusive) down to
                                    // end of setup code can be used by setup
                                    // for local heap purposes.
    USHORT        Pad1;
    ULONG        CommandLinePointer;        // 32-bit pointer to the kernel command line
    ULONG        InitrdAddressMax;        // Highest legal initrd address


} LINUX_SETUPSECTOR, *PLINUX_SETUPSECTOR;
#include <poppack.h>

// Implemented in linux.S
DECLSPEC_NORETURN
VOID __cdecl
BootLinuxKernel(
    _In_ ULONG KernelSize,
    _In_ PVOID KernelCurrentLoadAddress,
    _In_ PVOID KernelTargetLoadAddress);

ARC_STATUS
LoadAndBootLinux(
    _In_ ULONG Argc,
    _In_ PCHAR Argv[],
    _In_ PCHAR Envp[]);

#endif /* _M_IX86 || _M_AMD64 */

#endif // defined __LINUX_H
