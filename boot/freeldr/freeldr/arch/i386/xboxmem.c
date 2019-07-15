/*
 *  FreeLoader
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
 *
 *  Note: much of this code was based on knowledge and/or code developed
 *  by the Xbox Linux group: http://www.xbox-linux.org
 */

#include <freeldr.h>
#include <debug.h>

DBG_DEFAULT_CHANNEL(MEMORY);

static ULONG InstalledMemoryMb = 0;
static ULONG AvailableMemoryMb = 0;

#define TEST_SIZE     0x200
#define TEST_PATTERN1 0xAA
#define TEST_PATTERN2 0x55

extern VOID
SetMemory(
    PFREELDR_MEMORY_DESCRIPTOR MemoryMap,
    ULONG_PTR BaseAddress,
    SIZE_T Size,
    TYPE_OF_MEMORY MemoryType);

extern ULONG
PcMemFinalizeMemoryMap(
    PFREELDR_MEMORY_DESCRIPTOR MemoryMap);

VOID
XboxMemInit(VOID)
{
    UCHAR ControlRegion[TEST_SIZE];
    PVOID MembaseTop = (PVOID)(64 * 1024 * 1024);
    PVOID MembaseLow = (PVOID)0;

    (*(PULONG)(0xfd000000 + 0x100200)) = 0x03070103;
    (*(PULONG)(0xfd000000 + 0x100204)) = 0x11448000;

    WRITE_PORT_ULONG((ULONG*) 0xcf8, CONFIG_CMD(0, 0, 0x84));
    WRITE_PORT_ULONG((ULONG*) 0xcfc, 0x7ffffff);             /* Prep hardware for 128 Mb */

    InstalledMemoryMb = 64;
    memset(ControlRegion, TEST_PATTERN1, TEST_SIZE);
    memset(MembaseTop, TEST_PATTERN1, TEST_SIZE);
    __wbinvd();

    if (memcmp(MembaseTop, ControlRegion, TEST_SIZE) == 0)
    {
        /* Looks like there is memory .. maybe a 128MB box */
        memset(ControlRegion, TEST_PATTERN2, TEST_SIZE);
        memset(MembaseTop, TEST_PATTERN2, TEST_SIZE);
        __wbinvd();
        if (memcmp(MembaseTop, ControlRegion, TEST_SIZE) == 0)
        {
            /* Definitely looks like there is memory */
            if (memcmp(MembaseLow, ControlRegion, TEST_SIZE) == 0)
            {
                /* Hell, we find the Test-string at 0x0 too! */
                InstalledMemoryMb = 64;
            }
            else
            {
                InstalledMemoryMb = 128;
            }
        }
    }

    /* Set hardware for amount of memory detected */
    WRITE_PORT_ULONG((ULONG*) 0xcf8, CONFIG_CMD(0, 0, 0x84));
    WRITE_PORT_ULONG((ULONG*) 0xcfc, InstalledMemoryMb * 1024 * 1024 - 1);

    /* 4 MB video framebuffer is reserved later using XboxMemReserveMemory() */
    AvailableMemoryMb = InstalledMemoryMb;
}

FREELDR_MEMORY_DESCRIPTOR XboxMemoryMap[2];

PFREELDR_MEMORY_DESCRIPTOR
XboxMemGetMemoryMap(ULONG *MemoryMapSize)
{
    TRACE("XboxMemGetMemoryMap()\n");

    /* Synthesize memory map */

    /* Available RAM block */
    SetMemory(XboxMemoryMap,
              0,
              AvailableMemoryMb * 1024 * 1024,
              LoaderFree);

    /* Video memory */
    SetMemory(XboxMemoryMap,
              AvailableMemoryMb * 1024 * 1024,
              (InstalledMemoryMb - AvailableMemoryMb) * 1024 * 1024,
              LoaderFirmwarePermanent);

    *MemoryMapSize = PcMemFinalizeMemoryMap(XboxMemoryMap);
    return XboxMemoryMap;
}

PVOID
XboxMemReserveMemory(ULONG MbToReserve)
{
    /* This function is used to reserve video framebuffer in XboxVideoInit() */

    if (InstalledMemoryMb == 0)
    {
        /* Hmm, seems we're not initialized yet */
        XboxMemInit();
    }

    if (MbToReserve > AvailableMemoryMb)
    {
        /* Can't satisfy the request */
        return NULL;
    }

    AvailableMemoryMb -= MbToReserve;

    /* Top of available memory points to the space just reserved */
    return (PVOID)(AvailableMemoryMb * 1024 * 1024);
}

/* EOF */
