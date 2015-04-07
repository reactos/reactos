/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            samples/testvdd/testvdd.c
 * PURPOSE:         Testing VDD for NTVDM
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include <stdio.h>

#include <windows.h>
#include <vddsvc.h>

#define NDEBUG
#include <debug.h>

// Enable this define to use DPRINT1 instead of MessageBox
// #define DBG_SILENT

/* GLOBALS ********************************************************************/

#ifdef DBG_SILENT

    #define VDD_DBG(...) \
    do { \
        DPRINT1(__VA_ARGS__); \
        DbgPrint("\n");       \
    } while(0)

#else

    static VOID
    VddDbgMsg(LPCSTR Format, ...)
    {
    #ifndef WIN2K_COMPLIANT
        CHAR  StaticBuffer[256];
        LPSTR Buffer = StaticBuffer; // Use the static buffer by default.
    #else
        CHAR  Buffer[2048]; // Large enough. If not, increase it by hand.
    #endif
        size_t MsgLen;
        va_list Parameters;

        va_start(Parameters, Format);

    #ifndef WIN2K_COMPLIANT
        /*
         * Retrieve the message length and if it is too long, allocate
         * an auxiliary buffer; otherwise use the static buffer.
         */
        MsgLen = _vscprintf(Format, Parameters) + 1; // NULL-terminated
        if (MsgLen > ARRAYSIZE(StaticBuffer))
        {
            Buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MsgLen * sizeof(WCHAR));
            if (Buffer == NULL)
            {
                /* Allocation failed, use the static buffer and display a suitable error message */
                Buffer = StaticBuffer;
                Format = "DisplayMessage()\nOriginal message is too long and allocating an auxiliary buffer failed.";
                MsgLen = strlen(Format);
            }
        }
    #else
        MsgLen = ARRAYSIZE(Buffer);
    #endif

        /* Display the message */
        _vsnprintf(Buffer, MsgLen, Format, Parameters);
        MessageBoxA(NULL, Buffer, "Test VDD", MB_OK);

    #ifndef WIN2K_COMPLIANT
        /* Free the buffer if needed */
        if (Buffer != StaticBuffer) HeapFree(GetProcessHeap(), 0, Buffer);
    #endif

        va_end(Parameters);
    }

    #define VDD_DBG VddDbgMsg
#endif

HANDLE hVdd = NULL;

/*
 * Port hooks (serial ports) -- Each port range is for testing different port handlers.
 */
#define NUM_PORTS 4

VDD_IO_PORTRANGE PortDefs[NUM_PORTS] =
{
    {0x3F8, 0x3FF},
    {0x2F8, 0x2FF},
    {0x3E8, 0x3EF},
    {0x2E8, 0x2EF}
};

// PFNVDD_INB   PortInB;
// PFNVDD_INW   PortInW;
// PFNVDD_INSB  PortInsB;
// PFNVDD_INSW  PortInsW;
// PFNVDD_OUTB  PortOutB;
// PFNVDD_OUTW  PortOutW;
// PFNVDD_OUTSB PortOutsB;
// PFNVDD_OUTSW PortOutsW;

// VDD_IO_HANDLERS PortHandlers[NUM_PORTS] =
// {
    // {PortInB, NULL   , NULL    , NULL    , PortOutB, NULL    , NULL     , NULL     },
    // {PortInB, PortInW, NULL    , NULL    , PortOutB, PortOutW, NULL     , NULL     },
    // {PortInB, NULL   , PortInsB, NULL    , PortOutB, NULL    , PortOutsB, NULL     },
    // {PortInB, NULL   , NULL    , PortInsW, PortOutB, NULL    , NULL     , PortOutsW},
// };


/* PRIVATE FUNCTIONS **********************************************************/

VOID
WINAPI
PortInB(IN  USHORT Port,
        OUT PUCHAR Data)
{
    *Data = 0;
    VDD_DBG("0x%08x (BYTE 0x%02x) <-- Port 0x%04x", Data, *Data, Port);
}

VOID
WINAPI
PortOutB(IN USHORT Port,
         IN UCHAR  Data)
{
    VDD_DBG("(BYTE 0x%02x) --> Port 0x%04x", Data, Port);
}

VOID
WINAPI
PortInW(IN  USHORT  Port,
        OUT PUSHORT Data)
{
    *Data = 0;
    VDD_DBG("0x%08x (WORD 0x%04x) <-- Port 0x%04x", Data, *Data, Port);
}

VOID
WINAPI
PortOutW(IN USHORT Port,
         IN USHORT Data)
{
    VDD_DBG("(WORD 0x%04x) --> Port 0x%04x", Data, Port);
}




VOID
WINAPI
PortInsB(IN  USHORT Port,
         OUT PUCHAR Data,
         IN  USHORT Count)
{
    VDD_DBG("0x%08x (BYTESTR[%u]) <-- Port 0x%04x", Data, Count, Port);
    while (Count--) *Data++ = 0;
}

VOID
WINAPI
PortOutsB(IN USHORT Port,
          IN PUCHAR Data,
          IN USHORT Count)
{
    VDD_DBG("0x%08x (BYTESTR[%u]) --> Port 0x%04x", Data, Count, Port);
}

VOID
WINAPI
PortInsW(IN  USHORT  Port,
         OUT PUSHORT Data,
         IN  USHORT  Count)
{
    VDD_DBG("0x%08x (WORDSTR[%u]) <-- Port 0x%04x", Data, Count, Port);
    while (Count--) *Data++ = 0;
}

VOID
WINAPI
PortOutsW(IN USHORT  Port,
          IN PUSHORT Data,
          IN USHORT  Count)
{
    VDD_DBG("0x%08x (WORDSTR[%u]) --> Port 0x%04x", Data, Count, Port);
}



VDD_IO_HANDLERS PortHandlers[NUM_PORTS] =
{
    {PortInB, NULL   , NULL    , NULL    , PortOutB, NULL    , NULL     , NULL     },
    {PortInB, PortInW, NULL    , NULL    , PortOutB, PortOutW, NULL     , NULL     },
    {PortInB, NULL   , PortInsB, NULL    , PortOutB, NULL    , PortOutsB, NULL     },
    {PortInB, NULL   , NULL    , PortInsW, PortOutB, NULL    , NULL     , PortOutsW},
};

/*
 * Memory hooking. Everything should be page-rounded.
 */

#ifndef PAGE_SIZE
#define PAGE_SIZE   0x1000
#endif

#ifndef PAGE_ROUND_DOWN
#define PAGE_ROUND_DOWN(x)  \
    ( ((ULONG_PTR)(x)) & (~(PAGE_SIZE-1)) )
#endif

#ifndef PAGE_ROUND_UP
#define PAGE_ROUND_UP(x)    \
    ( (((ULONG_PTR)(x)) + PAGE_SIZE-1) & (~(PAGE_SIZE-1)) )
#endif

#define MEM_SEG_START   0x0000
#define MEM_SIZE        PAGE_SIZE

USHORT HookedSegment = 0x0000;
ULONG  HookedOffset  = 0x0000;
PVOID  HookedAddress = NULL;

VOID
WINAPI
MemoryHandler(IN PVOID FaultAddress,
              IN ULONG RWMode)
{
    BOOLEAN Success = FALSE;

    VDD_DBG("MemoryHandler(0x%08x, %s)", FaultAddress, (RWMode == 1) ? "Write" : "Read");
    // VDDTerminateVDM();

    Success = VDDAllocMem(hVdd, HookedAddress, MEM_SIZE);
    if (!Success) VDD_DBG("Unable to allocate memory");
}

PVOID
FindHookableMemory(IN  USHORT  StartSegment,
                   IN  ULONG   StartOffset,
                   OUT PUSHORT HookedSegment,
                   OUT PULONG  HookedOffset)
{
    BOOLEAN Success;
    PVOID  PhysMemStart = NULL;
    USHORT Segment = StartSegment;
    ULONG  Offset  = PAGE_ROUND_DOWN(StartOffset);

    *HookedSegment = 0x0000;
    *HookedOffset  = 0x0000;

    while (Segment <= 0xF000)
    {
        // PhysMemStart = GetVDMPointer(GetVDMAddress(Segment, Offset), MEM_SIZE, (getMSW() & MSW_PE));
        PhysMemStart = VdmMapFlat(Segment, Offset, getMODE());

        /* Try to hook this memory area... */
        Success = VDDInstallMemoryHook(hVdd, PhysMemStart, MEM_SIZE, MemoryHandler);
        if (!Success)
        {
            /* ... it didn't work. Free PhysMemStart, increase segment/offset and try again. */
            DPRINT1("%04lX:%08lX hooking failed, continue...\n", Segment, Offset);

            VdmUnmapFlat(Segment, Offset, PhysMemStart, getMODE());
            // FreeVDMPointer(GetVDMAddress(Segment, Offset), MEM_SIZE, PhysMemStart, (getMSW() & MSW_PE));
            PhysMemStart = NULL;

            Offset += MEM_SIZE;
            if (Offset + MEM_SIZE > 0xFFFF)
            {
                Segment += 0x1000;
                Offset = 0x0000;
            }
        }
        else
        {
            /* ... it worked. We'll free PhysMemStart later on. */
            DPRINT1("%04lX:%08lX hooking succeeded!\n", Segment, Offset);
            break;
        }
    }

    if (PhysMemStart)
    {
        VDD_DBG("We hooked at %04lX:%08lX (0x%p)", Segment, Offset, PhysMemStart);
        *HookedSegment = Segment;
        *HookedOffset  = Offset;
    }
    else
    {
        VDD_DBG("Hooking attempt failed!");
    }

    return PhysMemStart;
}


BOOLEAN
RegisterVDD(BOOLEAN Register)
{
    BOOLEAN Success = FALSE;

    if (Register)
    {
        /* Hook some IO ports */
        VDD_DBG("VDDInstallIOHook");
        Success = VDDInstallIOHook(hVdd, NUM_PORTS, PortDefs, PortHandlers);
        if (!Success)
        {
            VDD_DBG("Unable to hook IO ports, terminate...");
            VDDTerminateVDM();
        }

        /* Add a memory handler */
        VDD_DBG("FindHookableMemory");
        HookedAddress = FindHookableMemory(MEM_SEG_START, 0x0000,
                                           &HookedSegment, &HookedOffset);
        if (HookedAddress == NULL)
        {
            VDD_DBG("Unable to install memory handler, terminate...");
            VDDTerminateVDM();
        }
        VDD_DBG("Initialization finished!");
    }
    else
    {
        Success = VDDFreeMem(hVdd, HookedAddress, MEM_SIZE);
        if (!Success) VDD_DBG("Unable to free memory");

        /* Uninstall the memory handler */
        VDD_DBG("VDDDeInstallMemoryHook");
        Success = VDDDeInstallMemoryHook(hVdd, HookedAddress, MEM_SIZE);
        if (!Success) VDD_DBG("Memory handler uninstall failed");

        VDD_DBG("VdmUnmapFlat");
        Success = VdmUnmapFlat(HookedSegment, HookedOffset, HookedAddress, getMODE());
        // FreeVDMPointer(GetVDMAddress(HookedSegment, HookedOffset), MEM_SIZE, HookedAddress, (getMSW() & MSW_PE));
        if (!Success) VDD_DBG("VdmUnmapFlat failed!");

        /* Deregister the hooked IO ports */
        VDD_DBG("VDDDeInstallIOHook");
        VDDDeInstallIOHook(hVdd, NUM_PORTS, PortDefs);

        VDD_DBG("Cleanup finished!");
        Success = TRUE;
    }

    return Success;
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID
WINAPI
TestVDDRegister(VOID)
{
    VDD_DBG("TestVDDRegister");

    /* Clear the Carry Flag: success */
    setCF(0);
}

VOID
WINAPI
TestVDDUnRegister(VOID)
{
    VDD_DBG("TestVDDUnRegister");

    /* Clear the Carry Flag: success */
    setCF(0);
}

VOID
WINAPI
TestVDDDispatch(VOID)
{
    VDD_DBG("TestVDDDispatch");

    /* Clear the Carry Flag: success */
    setCF(0);
}

/* ENTRY-POINT ****************************************************************/

BOOL
WINAPI // VDDInitialize
DllMain(IN HINSTANCE hInstanceDll,
        IN DWORD dwReason,
        IN LPVOID lpReserved)
{
    BOOLEAN Success;

    UNREFERENCED_PARAMETER(lpReserved);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            VDD_DBG("DLL_PROCESS_ATTACH");

            /* Save our global VDD handle */
            hVdd = hInstanceDll;

            /* Register VDD */
            Success = RegisterVDD(TRUE);
            if (!Success) VDD_DBG("Failed to register the VDD...");

            break;
        }

        case DLL_PROCESS_DETACH:
        {
            VDD_DBG("DLL_PROCESS_DETACH");

            /* Unregister VDD */
            Success = RegisterVDD(FALSE);
            if (!Success) VDD_DBG("Failed to unregister the VDD...");

            break;
        }

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        default:
            break;
    }

    return TRUE;
}

/* EOF */
