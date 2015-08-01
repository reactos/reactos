/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            bios.c
 * PURPOSE:         VDM BIOS Support Library
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "emulator.h"
#include "cpu/callback.h"
#include "cpu/bop.h"

#include "bios.h"
#include "bios32/bios32.h"
#include "rom.h"
#include "umamgr.h"

#include "io.h"
#include "hardware/cmos.h"

/* DEFINES ********************************************************************/

/* BOP Identifiers */
#define BOP_RESET       0x00    // Windows NTVDM (SoftPC) BIOS calls BOP 0x00
                                // to let the virtual machine initialize itself
                                // the IVT and its hardware.
#define BOP_EQUIPLIST   0x11
#define BOP_GETMEMSIZE  0x12

/* PRIVATE VARIABLES **********************************************************/

static BOOLEAN Bios32Loaded = FALSE;

PBIOS_DATA_AREA Bda;
PBIOS_CONFIG_TABLE Bct;

/* PRIVATE FUNCTIONS **********************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/

VOID WINAPI BiosEquipmentService(LPWORD Stack)
{
    /* Return the equipment list */
    setAX(Bda->EquipmentList);
}

VOID WINAPI BiosGetMemorySize(LPWORD Stack)
{
    /* Return the conventional memory size in kB, typically 640 kB */
    setAX(Bda->MemorySize);
}

BOOLEAN
BiosInitialize(IN LPCSTR BiosFileName)
{
    BOOLEAN Success = FALSE;

    /* Disable interrupts */
    setIF(0);

    /* Initialize the BDA and the BCT pointers */
    Bda =    (PBIOS_DATA_AREA)SEG_OFF_TO_PTR(BDA_SEGMENT, 0x0000);
    // The BCT is found at F000:E6F5 for 100% compatible BIOSes.
    Bct = (PBIOS_CONFIG_TABLE)SEG_OFF_TO_PTR(BIOS_SEGMENT, 0xE6F5);

    /**** HACK! HACK! for Windows NTVDM BIOS ****/
    // WinNtVdmBiosSupportInitialize();

    // /* Register the BIOS support BOPs */
    // RegisterBop(BOP_EQUIPLIST , BiosEquipmentService);
    // RegisterBop(BOP_GETMEMSIZE, BiosGetMemorySize);

    if (BiosFileName && BiosFileName[0] != '\0')
    {
        PVOID BiosLocation = NULL;
        DWORD BiosSize = 0;

        Success = LoadBios(BiosFileName, &BiosLocation, &BiosSize);
        DPRINT1("BIOS loading %s ; GetLastError() = %u\n", Success ? "succeeded" : "failed", GetLastError());

        if (!Success) return FALSE;

        DisplayMessage(L"First bytes at 0x%p: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n"
                       L"3 last bytes at 0x%p: 0x%02x 0x%02x 0x%02x",
                       BiosLocation,
                       *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + 0),
                       *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + 1),
                       *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + 2),
                       *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + 3),
                       *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + 4),
                       *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + 5),
                       *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + 6),
                       *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + 7),
                       *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + 8),
                       *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + 9),

                        (PVOID)((ULONG_PTR)BiosLocation + BiosSize - 2),
                       *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + BiosSize - 2),
                       *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + BiosSize - 1),
                       *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + BiosSize - 0));

        DisplayMessage(L"POST at 0x%p: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x",
                       TO_LINEAR(getCS(), getIP()),
                       *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(getCS(), getIP()) + 0),
                       *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(getCS(), getIP()) + 1),
                       *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(getCS(), getIP()) + 2),
                       *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(getCS(), getIP()) + 3),
                       *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(getCS(), getIP()) + 4));

        /* Boot it up */

        /*
         * The CPU is already in reset-mode so that
         * CS:IP points to F000:FFF0 as required.
         */
        DisplayMessage(L"CS=0x%p ; IP=0x%p", getCS(), getIP());
        // setCS(0xF000);
        // setIP(0xFFF0);

        Success = TRUE;
    }
    else
    {
        WriteProtectRom((PVOID)ROM_AREA_START,
                        ROM_AREA_END - ROM_AREA_START + 1);

        Success = Bios32Loaded = Bios32Initialize();
    }

    // /* Enable interrupts */
    // setIF(1);

    /* Initialize the Upper Memory Area Manager */
    if (!UmaMgrInitialize())
    {
        wprintf(L"FATAL: Failed to initialize the UMA manager.\n");
        return FALSE;
    }

    return Success;
}

VOID
BiosCleanup(VOID)
{
    UmaMgrCleanup();

    if (Bios32Loaded) Bios32Cleanup();
}

/* EOF */
