/*
 *  FreeLoader
 *
 * Copyright ... ... (See below.)
 * Copyright 2017 Serge Gautherie <reactos-git_serge_171003@gautherie.fr>
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
 * Note: Most of this code comes from the old file "i386mem.c", which
 *       was Copyright (C) 1998-2003 Brian Palmer <brianp@sginet.com>
 */

#include <freeldr.h>
#include <arch/pc/x86common.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(MEMORY);

#define ULONGLONG_ALIGN_DOWN_BY(size, align) \
    ((ULONGLONG)(size) & ~((ULONGLONG)(align) - 1))

#define ULONGLONG_ALIGN_UP_BY(size, align) \
    (ULONGLONG_ALIGN_DOWN_BY(((ULONGLONG)(size) + align - 1), align))

BIOS_MEMORY_MAP PcBiosMemoryMap[MAX_BIOS_DESCRIPTORS];
ULONG PcBiosMapCount;

FREELDR_MEMORY_DESCRIPTOR PcMemoryMap[MAX_BIOS_DESCRIPTORS + 1];
ULONG PcMapCount;

ULONG
AddMemoryDescriptor(
    IN OUT PFREELDR_MEMORY_DESCRIPTOR List,
    IN ULONG MaxCount,
    IN PFN_NUMBER BasePage,
    IN PFN_NUMBER PageCount,
    IN TYPE_OF_MEMORY MemoryType);

static
BOOLEAN
GetExtendedMemoryConfiguration(ULONG* pMemoryAtOneMB /* in KB */, ULONG* pMemoryAtSixteenMB /* in 64KB */)
{
    REGS     RegsIn;
    REGS     RegsOut;

    TRACE("GetExtendedMemoryConfiguration()\n");

    *pMemoryAtOneMB = 0;
    *pMemoryAtSixteenMB = 0;

    // Int 15h AX=E801h
    // Phoenix BIOS v4.0 - GET MEMORY SIZE FOR >64M CONFIGURATIONS
    //
    // AX = E801h
    // Return:
    // CF clear if successful
    // AX = extended memory between 1M and 16M, in K (max 3C00h = 15MB)
    // BX = extended memory above 16M, in 64K blocks
    // CX = configured memory 1M to 16M, in K
    // DX = configured memory above 16M, in 64K blocks
    // CF set on error
    RegsIn.w.ax = 0xE801;
    Int386(0x15, &RegsIn, &RegsOut);

    TRACE("Int15h AX=E801h\n");
    TRACE("AX = 0x%x\n", RegsOut.w.ax);
    TRACE("BX = 0x%x\n", RegsOut.w.bx);
    TRACE("CX = 0x%x\n", RegsOut.w.cx);
    TRACE("DX = 0x%x\n", RegsOut.w.dx);
    TRACE("CF set = %s\n", (RegsOut.x.eflags & EFLAGS_CF) ? "TRUE" : "FALSE");

    if (INT386_SUCCESS(RegsOut))
    {
        // If AX=BX=0000h the use CX and DX
        if (RegsOut.w.ax == 0)
        {
            // Return extended memory size in K
            *pMemoryAtSixteenMB = RegsOut.w.dx;
            *pMemoryAtOneMB = RegsOut.w.cx;
            return TRUE;
        }
        else
        {
            // Return extended memory size in K
            *pMemoryAtSixteenMB = RegsOut.w.bx;
            *pMemoryAtOneMB = RegsOut.w.ax;
            return TRUE;
        }
    }

    // If we get here then Int15 Func E801h didn't work
    // So try Int15 Func 88h
    // Int 15h AH=88h
    // SYSTEM - GET EXTENDED MEMORY SIZE (286+)
    //
    // AH = 88h
    // Return:
    // CF clear if successful
    // AX = number of contiguous KB starting at absolute address 100000h
    // CF set on error
    // AH = status
    // 80h invalid command (PC,PCjr)
    // 86h unsupported function (XT,PS30)
    RegsIn.b.ah = 0x88;
    Int386(0x15, &RegsIn, &RegsOut);

    TRACE("Int15h AH=88h\n");
    TRACE("AX = 0x%x\n", RegsOut.w.ax);
    TRACE("CF set = %s\n", (RegsOut.x.eflags & EFLAGS_CF) ? "TRUE" : "FALSE");

    if (INT386_SUCCESS(RegsOut) && RegsOut.w.ax != 0)
    {
        *pMemoryAtOneMB = RegsOut.w.ax;
        return TRUE;
    }

    // If we get here then Int15 Func 88h didn't work
    // So try reading the CMOS
    WRITE_PORT_UCHAR((PUCHAR)0x70, 0x31);
    *pMemoryAtOneMB = READ_PORT_UCHAR((PUCHAR)0x71);
    *pMemoryAtOneMB = (*pMemoryAtOneMB & 0xFFFF);
    *pMemoryAtOneMB = (*pMemoryAtOneMB << 8);

    TRACE("Int15h Failed\n");
    TRACE("CMOS reports: 0x%lx\n", *pMemoryAtOneMB);

    if (*pMemoryAtOneMB != 0)
    {
        return TRUE;
    }

    return FALSE;
}

static
ULONG
PcMemGetConventionalMemorySize(VOID)
{
    REGS Regs;

    TRACE("PcMemGetConventionalMemorySize()\n");

    /* Int 12h
     * BIOS - GET MEMORY SIZE
     *
     * Return:
     * AX = kilobytes of contiguous memory starting at absolute address 00000h
     *
     * This call returns the contents of the word at 0040h:0013h;
     * in PC and XT, this value is set from the switches on the motherboard
     */
    Regs.w.ax = 0;
    Int386(0x12, &Regs, &Regs);

    TRACE("Int12h\n");
    TRACE("AX = 0x%x\n", Regs.w.ax);

    return (ULONG)Regs.w.ax;
}

static
BOOLEAN
GetEbdaLocation(
    PULONG BaseAddress,
    PULONG Size)
{
    REGS Regs;

    TRACE("GetEbdaLocation()\n");

    /* Get the address of the Extended BIOS Data Area (EBDA).
     * Int 15h, AH=C1h
     * SYSTEM - RETURN EXTENDED-BIOS DATA-AREA SEGMENT ADDRESS (PS)
     *
     * Return:
     * CF set on error
     * CF clear if successful
     * ES = segment of data area
     */
    Regs.x.eax = 0x0000C100;
    Int386(0x15, &Regs, &Regs);

    /* If the function fails, there is no EBDA */
    if (!INT386_SUCCESS(Regs))
    {
        return FALSE;
    }

    /* Get Base address and (maximum) size */
    *BaseAddress = (ULONG)Regs.w.es << 4;
    *Size = 0xA0000 - *BaseAddress;
    return TRUE;
}

static
VOID
PcMemCheckUsableMemorySize(VOID)
{
    ULONG Size, RequiredSize;

    TRACE("PcMemCheckUsableMemorySize()\n");

    /* Make sure the usable memory is large enough. To do this we check the 16
       bit value at address 0x413 inside the BDA, which gives us the usable size
       in KB */
    Size = (*(PUSHORT)(ULONG_PTR)0x413) * 1024;
    RequiredSize = FREELDR_BASE + FrLdrImageSize + PAGE_SIZE;
    if (Size < RequiredSize)
    {
        FrLdrBugCheckWithMessage(
            MEMORY_INIT_FAILURE,
            __FILE__,
            __LINE__,
            "The BIOS reported a usable memory range up to 0x%lx, which is too small!\n"
            "Required size is 0x%lx\n\n"
            "If you see this, please report to the ReactOS team!",
            Size, RequiredSize);
    }
}

static
ULONG
PcMemGetBiosMemoryMap(PFREELDR_MEMORY_DESCRIPTOR MemoryMap, ULONG MaxMemoryMapSize)
{
    REGS Regs;
    ULONGLONG RealBaseAddress, EndAddress, RealSize;
    TYPE_OF_MEMORY MemoryType;

    ASSERT(PcBiosMapCount == 0);

    TRACE("PcMemGetBiosMemoryMap()\n");

    /* Int 15h AX=E820h
     * Newer BIOSes - GET SYSTEM MEMORY MAP
     *
     * AX = E820h
     * EAX = 0000E820h
     * EDX = 534D4150h ('SMAP')
     * EBX = continuation value or 00000000h to start at beginning of map
     * ECX = size of buffer for result, in bytes (should be >= 20 bytes)
     * ES:DI -> buffer for result
     * Return:
     * CF clear if successful
     * EAX = 534D4150h ('SMAP')
     * ES:DI buffer filled
     * EBX = next offset from which to copy or 00000000h if all done
     * ECX = actual length returned in bytes
     * CF set on error
     * AH = error code (86h)
     */
    Regs.x.ebx = 0x00000000;

    while (PcBiosMapCount < MAX_BIOS_DESCRIPTORS)
    {
        /* ACPI 3.0/4.0: Set Extended Attributes to enabled/valid by default, in case entry has no E.A.. */
        ((PBIOS_MEMORY_MAP)BIOSCALLBUFFER)->ExtendedAttributesAsULONG = 0;
        ((PBIOS_MEMORY_MAP)BIOSCALLBUFFER)->ExtendedAttributes.Enabled_Reserved = 1;

        /* Setup the registers for the BIOS call */
        Regs.x.eax = 0x0000E820;
        Regs.x.edx = 0x534D4150; /* ('SMAP') */
        /* Regs.x.ebx = 0x00000001;  Continuation value already set */
        Regs.x.ecx = sizeof(BIOS_MEMORY_MAP);
        Regs.w.es = BIOSCALLBUFSEGMENT;
        Regs.w.di = BIOSCALLBUFOFFSET;
        Int386(0x15, &Regs, &Regs);

        TRACE("Memory Map Entry %lu\n", PcBiosMapCount);
        TRACE("Int15h AX=E820h\n");
        TRACE("EAX = 0x%lx\n", Regs.x.eax);
        TRACE("EBX = 0x%lx\n", Regs.x.ebx);
        TRACE("ECX = %lu\n", Regs.x.ecx);
        TRACE("CF set = %s\n", (Regs.x.eflags & EFLAGS_CF) ? "TRUE" : "FALSE");

        /* If the BIOS didn't return 'SMAP' in EAX then
         * it doesn't support this call. */
        if (Regs.x.eax != 0x534D4150)
        {
            WARN("BIOS doesn't support Int15h AX=E820h!\n");
            break;
        }

        /* If the carry flag is set,
         * then this call was past the last entry, so we're done. */
        if (!INT386_SUCCESS(Regs))
        {
            TRACE("End of System Memory Map! (Past last)\n");
            break;
        }

        if (Regs.x.ecx == 0)
        {
            TRACE("Discarding empty entry. (would-be-PcBiosMapCount = %lu)\n",
                  PcBiosMapCount);
            goto nextRange;
        }

        /* Extra safety: unexpected entry length.
         * All in-between values are valid too, as x86 is little-indian
         * and only lower byte is used per ACPI 6.2-A.
         */
        if (Regs.x.ecx < RTL_SIZEOF_THROUGH_FIELD(BIOS_MEMORY_MAP, Type) ||
            Regs.x.ecx > sizeof(BIOS_MEMORY_MAP))
        {
            ERR("Int 15h AX=E820h returned an invalid entry length! (would-be-PcBiosMapCount = %lu, Entry length = (%Iu <=) %lu (<= %Iu))\n",
                PcBiosMapCount, RTL_SIZEOF_THROUGH_FIELD(BIOS_MEMORY_MAP, Type), Regs.x.ecx, sizeof(BIOS_MEMORY_MAP));
            /* Warn user, unless wrong case is "first and not too big entry", which is otherwise harmless. */
            if (PcBiosMapCount > 0 || Regs.x.ecx > sizeof(BIOS_MEMORY_MAP))
            {
                ASSERTMSG("Int 15h AX=E820h returned an invalid entry length!\n", FALSE);
            }
            /* We keep previous entries (if any), but do not dare trying next entries.
             * We assume these entries are good to use as is. If they are not, we are in trouble...
             * (And don't ask what happens if BIOS actually overflowed our entry buffer...)
             *
             * FIXME: Safer = revert previous entries, Safest = blacklist this BIOS.
             */
            break;
        }

        if (((PBIOS_MEMORY_MAP)BIOSCALLBUFFER)->ExtendedAttributes.Enabled_Reserved == 0)
        {
            WARN("Discarding disabled/invalid entry. (would-be-PcBiosMapCount = %lu)\n",
                 PcBiosMapCount);
            /* This unlikely case was correct between ACPI 3.0 and 4.0, so assume all is fine.
             * Unless we would be ready to drop ACPI 3.0 compatibility.
             */
            goto nextRange;
        }

        /*
         * Other deprecated ExtendedAttributes flags such as NonVolatile_Deprecated_Reserved
         * or SlowAccess_Deprecated_Reserved are simply ignored.
         */

        /* Copy data to global buffer */
        RtlCopyMemory(&PcBiosMemoryMap[PcBiosMapCount], (PVOID)BIOSCALLBUFFER, sizeof(BIOS_MEMORY_MAP));

        TRACE("BaseAddress: 0x%llx\n", PcBiosMemoryMap[PcBiosMapCount].BaseAddress);
        TRACE("Length: 0x%llx\n", PcBiosMemoryMap[PcBiosMapCount].Length);
        TRACE("Type: 0x%lx\n", PcBiosMemoryMap[PcBiosMapCount].Type);
        TRACE("ExtendedAttributesAsULONG: 0x%08lx\n", PcBiosMemoryMap[PcBiosMapCount].ExtendedAttributesAsULONG);

        if (PcBiosMemoryMap[PcBiosMapCount].ExtendedAttributes.ErrorLog == 1)
        {
            FIXME("EA.ErrorLog = 1. Please report this to CORE-14150. "
                   "(PcBiosMapCount = %lu, BaseAddress = 0x%llx, Length = 0x%llx, Type = 0x%lx, ExtendedAttributesAsULONG = 0x%08lx)\n",
                  PcBiosMapCount,
                  PcBiosMemoryMap[PcBiosMapCount].BaseAddress,
                  PcBiosMemoryMap[PcBiosMapCount].Length,
                  PcBiosMemoryMap[PcBiosMapCount].Type,
                  PcBiosMemoryMap[PcBiosMapCount].ExtendedAttributesAsULONG);
            // NotWantedForPublicBuilds: ASSERTMSG("EA.ErrorLog = 1. Check/Report then CONTinue.\n", FALSE);
        }

        if (PcBiosMemoryMap[PcBiosMapCount].Length == 0)
        {
            TRACE("Discarding empty range. (would-be-PcBiosMapCount = %lu, BaseAddress = 0x%llx, Length = 0)\n",
                  PcBiosMapCount, PcBiosMemoryMap[PcBiosMapCount].BaseAddress);
            goto nextRange;
        }

        /* Check if this is free memory */
        if (PcBiosMemoryMap[PcBiosMapCount].Type == BiosMemoryUsable)
        {
            MemoryType = LoaderFree;

            /* Align up base of memory range */
            RealBaseAddress = ULONGLONG_ALIGN_UP_BY(
                                PcBiosMemoryMap[PcBiosMapCount].BaseAddress,
                                PAGE_SIZE);

            /* Calculate aligned EndAddress */
            EndAddress = PcBiosMemoryMap[PcBiosMapCount].BaseAddress +
                         PcBiosMemoryMap[PcBiosMapCount].Length;
            EndAddress = ULONGLONG_ALIGN_DOWN_BY(EndAddress, PAGE_SIZE);

            /* Check if there is anything left */
            if (EndAddress <= RealBaseAddress)
            {
                /* This doesn't span any page, so continue with next range */
                TRACE("Skipping aligned range < PAGE_SIZE. (would-be-PcBiosMapCount = %lu, BaseAddress = 0x%llx, Length = 0x%llx)\n",
                      PcBiosMapCount,
                      PcBiosMemoryMap[PcBiosMapCount].BaseAddress,
                      PcBiosMemoryMap[PcBiosMapCount].Length);
                goto nextRange;
            }

            /* Calculate the length of the aligned range */
            RealSize = EndAddress - RealBaseAddress;
        }
        else
        {
            if (PcBiosMemoryMap[PcBiosMapCount].Type == BiosMemoryReserved)
            {
                MemoryType = LoaderFirmwarePermanent;
            }
            else
            {
                MemoryType = LoaderSpecialMemory;
            }

            /* Align down base of memory area */
            RealBaseAddress = ULONGLONG_ALIGN_DOWN_BY(
                                PcBiosMemoryMap[PcBiosMapCount].BaseAddress,
                                PAGE_SIZE);

            /* Calculate the length after aligning the base */
            RealSize = PcBiosMemoryMap[PcBiosMapCount].BaseAddress +
                       PcBiosMemoryMap[PcBiosMapCount].Length - RealBaseAddress;
            RealSize = ULONGLONG_ALIGN_UP_BY(RealSize, PAGE_SIZE);
        }

        /* Check if we can add this descriptor */
        if (RealSize < MM_PAGE_SIZE)
        {
            TRACE("Skipping aligned range < MM_PAGE_SIZE. (PcBiosMapCount = %lu, BaseAddress = 0x%llx, Length = 0x%llx)\n",
                  PcBiosMapCount,
                  PcBiosMemoryMap[PcBiosMapCount].BaseAddress,
                  PcBiosMemoryMap[PcBiosMapCount].Length);
        }
        else if (PcMapCount >= MaxMemoryMapSize)
        {
            ERR("PcMemoryMap is already full! (PcBiosMapCount = %lu, PcMapCount = %lu (>= %lu))\n",
                PcBiosMapCount, PcMapCount, MaxMemoryMapSize);
            // NotWantedForPublicBuilds: ASSERTMSG("PcMemoryMap is already full!\n", FALSE);
            /* We keep previous entries, and half-retrieve current/next entries.
             * We assume all these entries are good to use as is. If they are not, we are in trouble...
             *
             * FIXME: Safer = revert (half-)retrieved entries, Safest = increase MaxMemoryMapSize.
             */
        }
        else
        {
            /* Add the descriptor */
            PcMapCount = AddMemoryDescriptor(PcMemoryMap,
                                           MAX_BIOS_DESCRIPTORS,
                                           (PFN_NUMBER)(RealBaseAddress / MM_PAGE_SIZE),
                                           (PFN_NUMBER)(RealSize / MM_PAGE_SIZE),
                                           MemoryType);
        }

        PcBiosMapCount++;

nextRange:
        /* If the continuation value is zero,
         * then this was the last entry, so we're done. */
        if (Regs.x.ebx == 0x00000000)
        {
            TRACE("End of System Memory Map! (Reset)\n");
            break;
        }
    }
    /* Check whether there would be more entries to process. */
    if (PcBiosMapCount >= MAX_BIOS_DESCRIPTORS && Regs.x.ebx != 0x00000000)
    {
        ERR("PcBiosMemoryMap is already full! (PcBiosMapCount = %lu (>= %lu), PcMapCount = %lu)\n",
            PcBiosMapCount, MAX_BIOS_DESCRIPTORS, PcMapCount);
        // NotWantedForPublicBuilds: ASSERTMSG("PcBiosMemoryMap is already full!\n", FALSE);
        /* We keep retrieved entries, but ignore next entries.
         * We assume these entries are good to use as is. If they are not, we are in trouble...
         *
         * FIXME: Safer = revert retrieved entries, Safest = increase MAX_BIOS_DESCRIPTORS.
         */
    }

    TRACE("PcMemGetBiosMemoryMap end: PcBiosMapCount = %lu\n", PcBiosMapCount);
    return PcBiosMapCount;
}

VOID
ReserveMemory(
    PFREELDR_MEMORY_DESCRIPTOR MemoryMap,
    ULONG_PTR BaseAddress,
    SIZE_T Size,
    TYPE_OF_MEMORY MemoryType,
    PCHAR Usage)
{
    ULONG_PTR BasePage, PageCount;
    ULONG i;

    BasePage = BaseAddress / PAGE_SIZE;
    PageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(BaseAddress, Size);

    for (i = 0; i < PcMapCount; i++)
    {
        /* Check for conflicting descriptor */
        if ((MemoryMap[i].BasePage < BasePage + PageCount) &&
            (MemoryMap[i].BasePage + MemoryMap[i].PageCount > BasePage))
        {
            /* Check if the memory is free */
            if (MemoryMap[i].MemoryType != LoaderFree)
            {
                FrLdrBugCheckWithMessage(
                    MEMORY_INIT_FAILURE,
                    __FILE__,
                    __LINE__,
                    "Failed to reserve memory in the range 0x%Ix - 0x%Ix for %s",
                    BaseAddress,
                    Size,
                    Usage);
            }
        }
    }

    /* Add the memory descriptor */
    PcMapCount = AddMemoryDescriptor(MemoryMap,
                                     MAX_BIOS_DESCRIPTORS,
                                     BasePage,
                                     PageCount,
                                     MemoryType);
}

VOID
SetMemory(
    PFREELDR_MEMORY_DESCRIPTOR MemoryMap,
    ULONG_PTR BaseAddress,
    SIZE_T Size,
    TYPE_OF_MEMORY MemoryType)
{
    ULONG_PTR BasePage, PageCount;

    BasePage = BaseAddress / PAGE_SIZE;
    PageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(BaseAddress, Size);

    /* Add the memory descriptor */
    PcMapCount = AddMemoryDescriptor(MemoryMap,
                                     MAX_BIOS_DESCRIPTORS,
                                     BasePage,
                                     PageCount,
                                     MemoryType);
}

ULONG
PcMemFinalizeMemoryMap(
    PFREELDR_MEMORY_DESCRIPTOR MemoryMap)
{
    ULONG i;

    /* Reserve some static ranges for freeldr */
    ReserveMemory(MemoryMap, 0x1000, STACKLOW - 0x1000, LoaderFirmwareTemporary, "BIOS area");
    ReserveMemory(MemoryMap, STACKLOW, STACKADDR - STACKLOW, LoaderOsloaderStack, "FreeLdr stack");
    ReserveMemory(MemoryMap, FREELDR_BASE, FrLdrImageSize, LoaderLoadedProgram, "FreeLdr image");

    /* Default to 1 page above freeldr for the disk read buffer */
    DiskReadBuffer = (PUCHAR)ALIGN_UP_BY(FREELDR_BASE + FrLdrImageSize, PAGE_SIZE);
    DiskReadBufferSize = PAGE_SIZE;

    /* Scan for free range above freeldr image */
    for (i = 0; i < PcMapCount; i++)
    {
        if ((MemoryMap[i].BasePage > (FREELDR_BASE / PAGE_SIZE)) &&
            (MemoryMap[i].MemoryType == LoaderFree))
        {
            /* Use this range for the disk read buffer */
            DiskReadBuffer = (PVOID)(MemoryMap[i].BasePage * PAGE_SIZE);
            DiskReadBufferSize = min(MemoryMap[i].PageCount * PAGE_SIZE,
                                     MAX_DISKREADBUFFER_SIZE);
            break;
        }
    }

    TRACE("DiskReadBuffer=0x%p, DiskReadBufferSize=0x%lx\n",
          DiskReadBuffer, DiskReadBufferSize);

    ASSERT(DiskReadBufferSize > 0);

    /* Now reserve the range for the disk read buffer */
    ReserveMemory(MemoryMap,
                  (ULONG_PTR)DiskReadBuffer,
                  DiskReadBufferSize,
                  LoaderFirmwareTemporary,
                  "Disk read buffer");

    TRACE("Dumping resulting memory map:\n");
    for (i = 0; i < PcMapCount; i++)
    {
        TRACE("BasePage=0x%lx, PageCount=0x%lx, Type=%s\n",
              MemoryMap[i].BasePage,
              MemoryMap[i].PageCount,
              MmGetSystemMemoryMapTypeString(MemoryMap[i].MemoryType));
    }
    return PcMapCount;
}

PFREELDR_MEMORY_DESCRIPTOR
PcMemGetMemoryMap(ULONG *MemoryMapSize)
{
    ULONG EntryCount;
    ULONG ExtendedMemorySizeAtOneMB;
    ULONG ExtendedMemorySizeAtSixteenMB;
    ULONG EbdaBase, EbdaSize;

    TRACE("PcMemGetMemoryMap()\n");

    PcMemCheckUsableMemorySize();

    EntryCount = PcMemGetBiosMemoryMap(PcMemoryMap, MAX_BIOS_DESCRIPTORS);

    /* If the BIOS didn't provide a memory map, synthesize one */
    if (EntryCount == 0)
    {
        GetExtendedMemoryConfiguration(&ExtendedMemorySizeAtOneMB,
                                       &ExtendedMemorySizeAtSixteenMB);

        /* Conventional memory */
        AddMemoryDescriptor(PcMemoryMap,
                            MAX_BIOS_DESCRIPTORS,
                            0,
                            PcMemGetConventionalMemorySize() * 1024 / PAGE_SIZE,
                            LoaderFree);

        /* Extended memory */
        PcMapCount = AddMemoryDescriptor(PcMemoryMap,
                                         MAX_BIOS_DESCRIPTORS,
                                         1024 * 1024 / PAGE_SIZE,
                                         ExtendedMemorySizeAtOneMB * 1024 / PAGE_SIZE,
                                         LoaderFree);

        if (ExtendedMemorySizeAtSixteenMB != 0)
        {
            /* Extended memory at 16MB */
            PcMapCount = AddMemoryDescriptor(PcMemoryMap,
                                             MAX_BIOS_DESCRIPTORS,
                                             0x1000000 / PAGE_SIZE,
                                             ExtendedMemorySizeAtSixteenMB * 64 * 1024 / PAGE_SIZE,
                                             LoaderFree);
        }

        /* Check if we have an EBDA and get it's location */
        if (GetEbdaLocation(&EbdaBase, &EbdaSize))
        {
            /* Add the descriptor */
            PcMapCount = AddMemoryDescriptor(PcMemoryMap,
                                             MAX_BIOS_DESCRIPTORS,
                                             (EbdaBase / PAGE_SIZE),
                                             ADDRESS_AND_SIZE_TO_SPAN_PAGES(EbdaBase, EbdaSize),
                                             LoaderFirmwarePermanent);
        }
    }

    /* Setup some protected ranges */
    SetMemory(PcMemoryMap, 0x000000, 0x01000, LoaderFirmwarePermanent); // Realmode IVT / BDA
    SetMemory(PcMemoryMap, 0x0A0000, 0x50000, LoaderFirmwarePermanent); // Video memory
    SetMemory(PcMemoryMap, 0x0F0000, 0x10000, LoaderSpecialMemory); // ROM
    SetMemory(PcMemoryMap, 0xFFF000, 0x01000, LoaderSpecialMemory); // unusable memory (do we really need this?)

    *MemoryMapSize = PcMemFinalizeMemoryMap(PcMemoryMap);
    return PcMemoryMap;
}


/* EOF */
