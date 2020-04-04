/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     APM BIOS detection routines
 * COPYRIGHT:   Copyright 2004 Eric Kohl (eric.kohl@reactos.org)
 */

#include <freeldr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(HWDETECT);

static BOOLEAN
FindApmBios(VOID)
{
    REGS RegsIn, RegsOut;

    /* APM BIOS - Installation check */
#if defined(SARCH_PC98)
    RegsIn.w.ax = 0x9A00;
    RegsIn.w.bx = 0x0000;
    Int386(0x1F, &RegsIn, &RegsOut);
#else
    RegsIn.w.ax = 0x5300;
    RegsIn.w.bx = 0x0000;
    Int386(0x15, &RegsIn, &RegsOut);
#endif
    if (INT386_SUCCESS(RegsOut) && RegsOut.w.bx == 'PM')
    {
        TRACE("Found APM BIOS\n");
        TRACE("AH: %x\n", RegsOut.b.ah);
        TRACE("AL: %x\n", RegsOut.b.al);
        TRACE("BH: %x\n", RegsOut.b.bh);
        TRACE("BL: %x\n", RegsOut.b.bl);
        TRACE("CX: %x\n", RegsOut.w.cx);

        return TRUE;
    }

    TRACE("No APM BIOS found\n");

    return FALSE;
}

VOID
DetectApmBios(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber)
{
    PCONFIGURATION_COMPONENT_DATA BiosKey;
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    ULONG Size;

    if (!FindApmBios())
        return;

    Size = sizeof(CM_PARTIAL_RESOURCE_LIST) -
           sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

    /* Set 'Configuration Data' value */
    PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
    if (PartialResourceList == NULL)
    {
        ERR("Failed to allocate resource descriptor\n");
        return;
    }
    RtlZeroMemory(PartialResourceList, Size);
    PartialResourceList->Version = 0;
    PartialResourceList->Revision = 0;
    PartialResourceList->Count = 0;

    /* FIXME: Add configuration data */

    /* Create new bus key */
    FldrCreateComponentKey(SystemKey,
                           AdapterClass,
                           MultiFunctionAdapter,
                           0x0,
                           0,
                           0xFFFFFFFF,
                           "APM",
                           PartialResourceList,
                           Size,
                           &BiosKey);

    /* Increment bus number */
    (*BusNumber)++;
}

/* EOF */
