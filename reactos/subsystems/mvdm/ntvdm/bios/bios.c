/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/bios/bios.c
 * PURPOSE:         VDM BIOS Support Library
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#include "ntvdm.h"

#define NDEBUG
#include <debug.h>

#include <stdlib.h>

/* DEFINES ********************************************************************/

/* BOP Identifiers */
#define BOP_RESET       0x00    // Windows NTVDM (SoftPC) BIOS calls BOP 0x00
                                // to let the virtual machine perform the POST.
#define BOP_EQUIPLIST   0x11
#define BOP_GETMEMSIZE  0x12

/* PRIVATE VARIABLES **********************************************************/

static BOOLEAN Bios32Loaded = FALSE;

PBIOS_DATA_AREA Bda;
PBIOS_CONFIG_TABLE Bct;

/* PRIVATE FUNCTIONS **********************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/

VOID WINAPI
WinNtVdmBiosReset(LPWORD Stack)
{
    DisplayMessage(L"You are loading Windows NTVDM BIOS!\n");
    // Bios32Post(Stack);

    DisplayMessage(L"ReactOS NTVDM doesn't support Windows NTVDM BIOS at the moment. The VDM will shut down.");
    EmulatorTerminate();
}

BOOLEAN
BiosInitialize(IN LPCSTR BiosFileName,
               IN LPCSTR RomFiles OPTIONAL)
{
    BOOLEAN Success = FALSE;
    BOOLEAN Success2 = FALSE;
    LPCSTR RomFile;
    LPSTR ptr;
    ULONG RomAddress;
    CHAR RomFileName[MAX_PATH + 10 + 1];

    /* Disable interrupts */
    setIF(0);

    /* Initialize the BDA and the BCT pointers */
    Bda =    (PBIOS_DATA_AREA)SEG_OFF_TO_PTR(BDA_SEGMENT , 0x0000);
    // The BCT is found at F000:E6F5 for 100% compatible BIOSes.
    Bct = (PBIOS_CONFIG_TABLE)SEG_OFF_TO_PTR(BIOS_SEGMENT, 0xE6F5);

    /* Register the BIOS support BOPs */
    RegisterBop(BOP_RESET, WinNtVdmBiosReset);          // Needed for Windows NTVDM (SoftPC) BIOS.
    RegisterBop(BOP_EQUIPLIST , BiosEquipmentService);  // Needed by Windows NTVDM (SoftPC) BIOS
    RegisterBop(BOP_GETMEMSIZE, BiosGetMemorySize);     // and also NTDOS!!

    if (BiosFileName == NULL)
    {
        Success = Bios32Loaded = Bios32Initialize();
    }
    else if (BiosFileName[0] != '\0')
    {
        PVOID BiosLocation = NULL;

        Success = LoadBios(BiosFileName, &BiosLocation, NULL);
        DPRINT1("BIOS file '%s' loading %s at address 0x%08x; GetLastError() = %u\n",
                BiosFileName, Success ? "succeeded" : "failed", BiosLocation, GetLastError());
    }
    else // if (BiosFileName[0] == '\0')
    {
        /* Do nothing */
        Success = TRUE;
    }

    /* Bail out now if we failed to load any BIOS file */
    if (!Success) return FALSE;

    /* Load optional ROMs */
    if (RomFiles)
    {
        RomFile = RomFiles;
        while (*RomFile)
        {
            strncpy(RomFileName, RomFile, ARRAYSIZE(RomFileName));
            RomFileName[ARRAYSIZE(RomFileName)-1] = '\0';

            ptr = strchr(RomFileName, '|'); // Since '|' is forbidden as a valid file name, we use it as a separator for the ROM address.
            if (!ptr) goto Skip;
            *ptr++ = '\0';

            RomAddress = strtoul(ptr, NULL, 0); // ROM segment
            RomAddress <<= 4; // Convert to real address
            if (RomAddress == 0) goto Skip;

            Success2 = LoadRom(RomFileName, (PVOID)RomAddress, NULL);
            DPRINT1("ROM file '%s' loading %s at address 0x%08x; GetLastError() = %u\n",
                    RomFileName, Success2 ? "succeeded" : "failed", RomAddress, GetLastError());

Skip:
            RomFile += strlen(RomFile) + 1;
        }
    }

    /*
     * Boot it up.
     * The CPU is already in reset-mode so that
     * CS:IP points to F000:FFF0 as required.
     */
    // DisplayMessage(L"CS:IP=%04X:%04X", getCS(), getIP());
    // setCS(0xF000);
    // setIP(0xFFF0);

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
