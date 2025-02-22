/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/envir.c
 * PURPOSE:         LLB Environment Variable Routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "precomp.h"

ULONG LlbEnvHwPageSize;
ULONG LlbEnvHwMemStart = 0;
ULONG LlbEnvHwMemSize = 0;
ULONG LlbEnvRamDiskStart = 0;
ULONG LlbEnvRamDiskSize = 0;
ULONG LlbEnvHwRevision;
CHAR LlbEnvCmdLine[256];
CHAR LlbValueData[32];

VOID
NTAPI
LlbEnvParseArguments(IN PATAG Arguments)
{
    PATAG Atag;

    /* Parse the ATAGs */
    Atag = Arguments;
    while (Atag->Hdr.Size)
    {
        /* Check tag type */
        switch (Atag->Hdr.Tag)
        {
            case ATAG_CORE:

                /* Save page size */
                LlbEnvHwPageSize = Atag->u.Core.PageSize;
                break;

            case ATAG_REVISION:

                /* Save page size */
                LlbEnvHwRevision = Atag->u.Revision.Rev;
                break;

            case ATAG_MEM:

                /* Save RAM start and size */
                if (!LlbEnvHwMemStart) LlbEnvHwMemStart = Atag->u.Mem.Start;
                LlbEnvHwMemSize += Atag->u.Mem.Size;
                break;

            case ATAG_INITRD2:

                /* Save RAMDISK start and size */
                LlbEnvRamDiskStart = Atag->u.InitRd2.Start;
                LlbEnvRamDiskSize = Atag->u.InitRd2.Size;

#ifdef _BEAGLE_
                /* Make sure it's 16MB-aligned */
                LlbEnvRamDiskSize = (LlbEnvRamDiskSize + (16 * 1024 * 1024) - 1)
                                    &~ ((16 * 1024 * 1024) - 1);

                /* The RAMDISK actually starts 16MB later */
                LlbEnvRamDiskStart += 16 * 1024 * 1024;
                LlbEnvRamDiskSize  -= 16 * 1024 * 1024;
#endif
                break;

            case ATAG_CMDLINE:

                /* Save command line */
                strncpy(LlbEnvCmdLine,
                        Atag->u.CmdLine.CmdLine,
                        Atag->Hdr.Size * sizeof(ULONG));
                break;

            /* Nothing left to handle */
            case ATAG_NONE:
            default:
                break;
        }

        /* Next tag */
        Atag = (PATAG)((PULONG)Atag + Atag->Hdr.Size);
    }

    /* For debugging */
    DbgPrint("[BOOTROM] Board Revision: %lx PageSize: %dKB RAM: %dMB CMDLINE: %s\n"
             "[RAMDISK] Base: %lx Size: %dMB\n",
             LlbEnvHwRevision,
             LlbEnvHwPageSize / 1024, LlbEnvHwMemSize / 1024 / 1024, LlbEnvCmdLine,
             LlbEnvRamDiskStart, LlbEnvRamDiskSize / 1024 / 1024);
}

VOID
NTAPI
LlbEnvGetMemoryInformation(IN PULONG Base,
                           IN PULONG Size)
{
    /* Return RAM information */
    *Base = LlbEnvHwMemStart;
    *Size = LlbEnvHwMemSize;
}

BOOLEAN
NTAPI
LlbEnvGetRamDiskInformation(IN PULONG Base,
                            IN PULONG Size)
{
    /* Do we have a ramdisk? */
    if (LlbEnvRamDiskSize == 0)
    {
        /* No */
        *Base = 0;
        *Size = 0;
        return FALSE;
    }

    /* Return ramdisk information */
    *Base = LlbEnvRamDiskStart;
    *Size = LlbEnvRamDiskSize;
    return TRUE;
}

PCHAR
NTAPI
LlbEnvRead(IN PCHAR ValueName)
{
    PCHAR ValuePointer;
    ULONG Length = 0;

    /* Search for the value name */
    ValuePointer = strstr(LlbEnvCmdLine, ValueName);
    if (ValuePointer)
    {
        /* Get the value data and its length */
        ValuePointer += strlen(ValueName) + 1;
        if (strchr(ValuePointer, ','))
        {
            /* Stop before next parameter */
            Length = strchr(ValuePointer, ',') - ValuePointer;
        }
        else
        {
            /* Stop before the string ends */
            Length = strlen(ValuePointer);
        }

        /* Copy it */
        strncpy(LlbValueData, ValuePointer, Length);
    }

    /* Terminate the data */
    LlbValueData[Length] = ANSI_NULL;

    /* Return the data */
    return LlbValueData;
}

/* EOF */

