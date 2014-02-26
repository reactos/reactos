/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            rom.c
 * PURPOSE:         ROM Support Functions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"
#include "callback.h"

#include "rom.h"

/* PRIVATE FUNCTIONS **********************************************************/

static HANDLE
OpenRomFile(IN  LPCWSTR RomFileName,
            OUT PDWORD  RomSize OPTIONAL)
{
    HANDLE hRomFile;
    DWORD  dwRomSize;

    /* Open the ROM image file */
    SetLastError(0); // For debugging purposes
    hRomFile = CreateFileW(RomFileName,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
    DPRINT1("ROM opening %s ; GetLastError() = %u\n", hRomFile != INVALID_HANDLE_VALUE ? "succeeded" : "failed", GetLastError());

    /* We failed, bail out */
    if (hRomFile == INVALID_HANDLE_VALUE) return NULL;

    /* OK, we have a handle to the ROM image file */

    /*
     * Retrieve the size of the file.
     *
     * The size of the ROM image file is at most 256kB. For instance,
     * the SeaBIOS image, which includes also expansion ROMs inside it,
     * covers the range C000:0000 to F000:FFFF .
     *
     * We therefore can use GetFileSize.
     */
    dwRomSize = GetFileSize(hRomFile, NULL);
    if ( (dwRomSize == INVALID_FILE_SIZE && GetLastError() != ERROR_SUCCESS) ||
         (dwRomSize > 0x40000) )
    {
        /* We failed, bail out */
        DPRINT1("Error when retrieving ROM size, or size too large (%d)\n", dwRomSize);

        /* Close the ROM image file */
        CloseHandle(hRomFile);

        return NULL;
    }

    /* Success, return file handle and size if needed */
    if (RomSize) *RomSize = dwRomSize;
    return hRomFile;
}

static BOOL
LoadRomFileByHandle(IN HANDLE RomFileHandle,
                    IN PVOID  RomLocation,
                    IN ULONG  RomSize)
{
    /*
     * The size of the ROM image file is at most 256kB. For instance,
     * the SeaBIOS image, which includes also expansion ROMs inside it,
     * covers the range C000:0000 to F000:FFFF .
     */
    if (RomSize > 0x40000)
    {
        DPRINT1("Wrong ROM image size 0x%lx, expected at most 0x40000 (256kB)", RomSize);
        return FALSE;
    }

    /* Attempt to load the ROM image file into memory */
    SetLastError(0); // For debugging purposes
    return ReadFile(RomFileHandle,
                    REAL_TO_PHYS(RomLocation),
                    RomSize,
                    &RomSize,
                    NULL);
}

static UCHAR
ComputeChecksum(IN ULONG RomLocation,
                IN ULONG RomSize)
{
    ULONG RomLastAddress = RomLocation + RomSize;
    UCHAR Sum = 0x00;   // Using a UCHAR guarantees that we wrap at 0xFF i.e. we do a sum modulo 0x100.

    while (RomLocation < RomLastAddress)
    {
        Sum += *(PUCHAR)REAL_TO_PHYS(RomLocation);
        ++RomLocation;
    }

    return Sum;
}

static VOID
InitRomRange(IN PCALLBACK16 Context,
             IN ULONG Start,
             IN ULONG End,
             IN ULONG Increment)
{
    ULONG Address, AddressBoot;
    ULONG RomSize;
    UCHAR Checksum;

    for (Address = Start; Address < End; Address += Increment)
    {
        /* Does the ROM have a valid signature? */
        if (*(PUSHORT)REAL_TO_PHYS(Address) == OPTION_ROM_SIGNATURE)
        {
            /* Check the control sum of the ROM */

            /*
             * If this is an adapter ROM (Start: C8000, End: E0000), its
             * reported size is stored in byte 2 of the ROM.
             *
             * If this is an expansion ROM (Start: E0000, End: F0000),
             * its real length is 64kB.
             */
            RomSize = *(PUCHAR)REAL_TO_PHYS(Address + 2) * 512;
            if (Address >= 0xE0000) RomSize = 0x10000;

            Checksum = ComputeChecksum(Address, RomSize);
            if (Checksum == 0x00)
            {
                AddressBoot = Address + 3;
                DPRINT1("Going to run @ address 0x%p\n", AddressBoot);

                AddressBoot = MAKELONG((AddressBoot & 0xFFFF), (AddressBoot & 0xF0000) >> 4);
                // setDS((Address & 0xF0000) >> 4);
                setDS((Address & 0xFF000) >> 4);
                RunCallback16(Context, AddressBoot);
                // Call16((AddressBoot & 0xF0000) >> 4, (AddressBoot & 0xFFFF));

                DPRINT1("Rom @ address 0x%p initialized\n", Address);
            }
            else
            {
                DPRINT1("Rom @ address 0x%p has invalid checksum of 0x%02x\n", Address, Checksum);
            }
        }
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN LoadBios(IN  LPCWSTR BiosFileName,
                 OUT PVOID*  BiosLocation OPTIONAL,
                 OUT PDWORD  BiosSize     OPTIONAL)
{
    BOOL   Success;
    HANDLE hBiosFile;
    DWORD  dwBiosSize = 0;
    PVOID  pBiosLocation;

    /* Open the BIOS image file */
    hBiosFile = OpenRomFile(BiosFileName, &dwBiosSize);

    /* If we failed, bail out */
    if (hBiosFile == NULL) return FALSE;

    /* BIOS location needs to be aligned on 32-bit boundary */
    // (PVOID)((ULONG_PTR)BaseAddress + ROM_AREA_END + 1 - dwBiosSize)
    pBiosLocation = MEM_ALIGN_DOWN(TO_LINEAR(0xF000, 0xFFFF) + 1 - dwBiosSize, sizeof(ULONG));

    /* Attempt to load the BIOS image file into memory */
    Success = LoadRomFileByHandle(hBiosFile, pBiosLocation, dwBiosSize);
    DPRINT1("BIOS loading %s ; GetLastError() = %u\n", Success ? "succeeded" : "failed", GetLastError());

    /* Close the BIOS image file */
    CloseHandle(hBiosFile);

    /* In case of success, return BIOS location and size if needed */
    if (Success)
    {
        if (BiosLocation) *BiosLocation = pBiosLocation;
        if (BiosSize)     *BiosSize     = dwBiosSize;
    }

    return (BOOLEAN)Success;
}

BOOLEAN LoadRom(IN  LPCWSTR RomFileName,
                IN  PVOID   RomLocation,
                OUT PDWORD  RomSize OPTIONAL)
{
    BOOL   Success;
    HANDLE hRomFile;
    DWORD  dwRomSize = 0;

    /* Open the ROM image file */
    hRomFile = OpenRomFile(RomFileName, &dwRomSize);

    /* If we failed, bail out */
    if (hRomFile == NULL) return FALSE;

    /* Attempt to load the ROM image file into memory */
    Success = LoadRomFileByHandle(hRomFile, RomLocation, dwRomSize);
    DPRINT1("ROM loading %s ; GetLastError() = %u\n", Success ? "succeeded" : "failed", GetLastError());

    /* Close the ROM image file and return */
    CloseHandle(hRomFile);

    /* In case of success, return ROM size if needed */
    if (Success)
    {
        if (RomSize) *RomSize = dwRomSize;
    }

    return (BOOLEAN)Success;
}

VOID SearchAndInitRoms(IN PCALLBACK16 Context)
{
    /* Adapters ROMs -- Start: C8000, End: E0000, 2kB blocks */
    InitRomRange(Context, 0xC8000, 0xE0000, 0x0800);

    /* Expansion ROM -- Start: E0000, End: F0000, 64kB block */
    InitRomRange(Context, 0xE0000, 0xEFFFF, 0x10000);
}

/* EOF */
