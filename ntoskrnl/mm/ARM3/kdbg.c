/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/kdbg.c
 * PURPOSE:         ARM Memory Manager Kernel Debugger routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 *                  Pierre Schweitzer
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include <mm/ARM3/miarm.h>

/* GLOBALS ********************************************************************/

typedef struct _IRP_FIND_CTXT
{
    ULONG_PTR RestartAddress;
    ULONG_PTR SData;
    ULONG Criteria;
} IRP_FIND_CTXT, *PIRP_FIND_CTXT;

extern PVOID MmNonPagedPoolEnd0;
extern SIZE_T PoolBigPageTableSize;
extern PPOOL_TRACKER_BIG_PAGES PoolBigPageTable;

#define POOL_BIG_TABLE_ENTRY_FREE 0x1

/* Pool block/header/list access macros */
#define POOL_ENTRY(x)       (PPOOL_HEADER)((ULONG_PTR)(x) - sizeof(POOL_HEADER))
#define POOL_FREE_BLOCK(x)  (PLIST_ENTRY)((ULONG_PTR)(x)  + sizeof(POOL_HEADER))
#define POOL_BLOCK(x, i)    (PPOOL_HEADER)((ULONG_PTR)(x) + ((i) * POOL_BLOCK_SIZE))
#define POOL_NEXT_BLOCK(x)  POOL_BLOCK((x), (x)->BlockSize)
#define POOL_PREV_BLOCK(x)  POOL_BLOCK((x), -((x)->PreviousSize))

VOID MiDumpPoolConsumers(BOOLEAN CalledFromDbg, ULONG Tag, ULONG Mask, ULONG Flags);

/* PRIVATE FUNCTIONS **********************************************************/

#if DBG && defined(KDBG)

BOOLEAN
ExpKdbgExtPool(
    ULONG Argc,
    PCHAR Argv[])
{
    ULONG_PTR Address = 0, Flags = 0;
    PVOID PoolPage;
    PPOOL_HEADER Entry;
    BOOLEAN ThisOne;
    PULONG Data;

    if (Argc > 1)
    {
        /* Get address */
        if (!KdbpGetHexNumber(Argv[1], &Address))
        {
            KdbpPrint("Invalid parameter: %s\n", Argv[0]);
            return TRUE;
        }
    }

    if (Argc > 2)
    {
        /* Get address */
        if (!KdbpGetHexNumber(Argv[1], &Flags))
        {
            KdbpPrint("Invalid parameter: %s\n", Argv[0]);
            return TRUE;
        }
    }

    /* Check if we got an address */
    if (Address != 0)
    {
        /* Get the base page */
        PoolPage = PAGE_ALIGN(Address);
    }
    else
    {
        KdbpPrint("Heap is unimplemented\n");
        return TRUE;
    }

    /* No paging support! */
    if (!MmIsAddressValid(PoolPage))
    {
        KdbpPrint("Address not accessible!\n");
        return TRUE;
    }

    /* Get pool type */
    if ((Address >= (ULONG_PTR)MmPagedPoolStart) && (Address <= (ULONG_PTR)MmPagedPoolEnd))
        KdbpPrint("Allocation is from PagedPool region\n");
    else if ((Address >= (ULONG_PTR)MmNonPagedPoolStart) && (Address <= (ULONG_PTR)MmNonPagedPoolEnd))
        KdbpPrint("Allocation is from NonPagedPool region\n");
    else
    {
        KdbpPrint("Address 0x%p is not within any pool!\n", (PVOID)Address);
        return TRUE;
    }

    /* Loop all entries of that page */
    Entry = PoolPage;
    do
    {
        /* Check if the address is within that entry */
        ThisOne = ((Address >= (ULONG_PTR)Entry) &&
                   (Address < (ULONG_PTR)(Entry + Entry->BlockSize)));

        if (!(Flags & 1) || ThisOne)
        {
            /* Print the line */
            KdbpPrint("%c%p size: %4d previous size: %4d  %s  %.4s\n",
                     ThisOne ? '*' : ' ', Entry, Entry->BlockSize, Entry->PreviousSize,
                     (Flags & 0x80000000) ? "" : (Entry->PoolType ? "(Allocated)" : "(Free)     "),
                     (Flags & 0x80000000) ? "" : (PCHAR)&Entry->PoolTag);
        }

        if (Flags & 1)
        {
            Data = (PULONG)(Entry + 1);
            KdbpPrint("    %p  %08lx %08lx %08lx %08lx\n"
                     "    %p  %08lx %08lx %08lx %08lx\n",
                     &Data[0], Data[0], Data[1], Data[2], Data[3],
                     &Data[4], Data[4], Data[5], Data[6], Data[7]);
        }

        /* Go to next entry */
        Entry = POOL_BLOCK(Entry, Entry->BlockSize);
    }
    while ((Entry->BlockSize != 0) && ((ULONG_PTR)Entry < (ULONG_PTR)PoolPage + PAGE_SIZE));

    return TRUE;
}

static
VOID
ExpKdbgExtPoolUsedGetTag(PCHAR Arg, PULONG Tag, PULONG Mask)
{
    CHAR Tmp[4];
    SIZE_T Len;
    USHORT i;

    /* Get the tag */
    Len = strlen(Arg);
    if (Len > 4)
    {
        Len = 4;
    }

    /* Generate the mask to have wildcards support */
    for (i = 0; i < Len; ++i)
    {
        Tmp[i] = Arg[i];
        if (Tmp[i] != '?')
        {
            *Mask |= (0xFF << i * 8);
        }
    }

    /* Get the tag in the ulong form */
    *Tag = *((PULONG)Tmp);
}

BOOLEAN
ExpKdbgExtPoolUsed(
    ULONG Argc,
    PCHAR Argv[])
{
    ULONG Tag = 0;
    ULONG Mask = 0;
    ULONG_PTR Flags = 0;

    if (Argc > 1)
    {
        /* If we have 2+ args, easy: flags then tag */
        if (Argc > 2)
        {
            ExpKdbgExtPoolUsedGetTag(Argv[2], &Tag, &Mask);
            if (!KdbpGetHexNumber(Argv[1], &Flags))
            {
                KdbpPrint("Invalid parameter: %s\n", Argv[0]);
            }
        }
        else
        {
            /* Otherwise, try to find out whether that's flags */
            if (strlen(Argv[1]) == 1 ||
                (strlen(Argv[1]) == 3 && Argv[1][0] == '0' && Argv[1][1] == 'x'))
            {
                /* Fallback: if reading flags failed, assume it's a tag */
                if (!KdbpGetHexNumber(Argv[1], &Flags))
                {
                    ExpKdbgExtPoolUsedGetTag(Argv[1], &Tag, &Mask);
                }
            }
            /* Or tag */
            else
            {
                ExpKdbgExtPoolUsedGetTag(Argv[1], &Tag, &Mask);
            }
        }
    }

    /* Call the dumper */
    MiDumpPoolConsumers(TRUE, Tag, Mask, Flags);

    return TRUE;
}

static
VOID
ExpKdbgExtPoolFindLargePool(
    ULONG Tag,
    ULONG Mask,
    VOID (NTAPI* FoundCallback)(PPOOL_TRACKER_BIG_PAGES, PVOID),
    PVOID CallbackContext)
{
    ULONG i;

    KdbpPrint("Scanning large pool allocation table for Tag: %.4s (%p : %p)\n", (PCHAR)&Tag, &PoolBigPageTable[0], &PoolBigPageTable[PoolBigPageTableSize - 1]);

    for (i = 0; i < PoolBigPageTableSize; i++)
    {
        /* Free entry? */
        if ((ULONG_PTR)PoolBigPageTable[i].Va & POOL_BIG_TABLE_ENTRY_FREE)
        {
            continue;
        }

        if ((PoolBigPageTable[i].Key & Mask) == (Tag & Mask))
        {
            if (FoundCallback != NULL)
            {
                FoundCallback(&PoolBigPageTable[i], CallbackContext);
            }
            else
            {
                /* Print the line */
                KdbpPrint("%p: tag %.4s, size: %I64x\n",
                          PoolBigPageTable[i].Va, (PCHAR)&PoolBigPageTable[i].Key,
                          PoolBigPageTable[i].NumberOfPages << PAGE_SHIFT);
            }
        }
    }
}

static
BOOLEAN
ExpKdbgExtValidatePoolHeader(
    PVOID BaseVa,
    PPOOL_HEADER Entry,
    POOL_TYPE BasePoolTye)
{
    /* Block size cannot be NULL or negative and it must cover the page */
    if (Entry->BlockSize <= 0)
    {
        return FALSE;
    }
    if (Entry->BlockSize * 8 + (ULONG_PTR)Entry - (ULONG_PTR)BaseVa > PAGE_SIZE)
    {
        return FALSE;
    }

    /*
     * PreviousSize cannot be 0 unless on page begin
     * And it cannot be bigger that our current
     * position in page
     */
    if (Entry->PreviousSize == 0 && BaseVa != Entry)
    {
        return FALSE;
    }
    if (Entry->PreviousSize * 8 > (ULONG_PTR)Entry - (ULONG_PTR)BaseVa)
    {
        return FALSE;
    }

    /* Must be paged pool */
    if (((Entry->PoolType - 1) & BASE_POOL_TYPE_MASK) != BasePoolTye)
    {
        return FALSE;
    }

    /* Match tag mask */
    if ((Entry->PoolTag & 0x00808080) != 0)
    {
        return FALSE;
    }

    return TRUE;
}

static
VOID
ExpKdbgExtPoolFindPagedPool(
    ULONG Tag,
    ULONG Mask,
    VOID (NTAPI* FoundCallback)(PPOOL_HEADER, PVOID),
    PVOID CallbackContext)
{
    ULONG i = 0;
    PPOOL_HEADER Entry;
    PVOID BaseVa;
    PMMPDE PointerPde;

    KdbpPrint("Searching Paged pool (%p : %p) for Tag: %.4s\n", MmPagedPoolStart, MmPagedPoolEnd, (PCHAR)&Tag);

    /*
     * To speed up paged pool search, we will use the allocation bipmap.
     * This is possible because we live directly in the kernel :-)
     */
    i = RtlFindSetBits(MmPagedPoolInfo.PagedPoolAllocationMap, 1, 0);
    while (i != 0xFFFFFFFF)
    {
        BaseVa = (PVOID)((ULONG_PTR)MmPagedPoolStart + (i << PAGE_SHIFT));
        Entry = BaseVa;

        /* Validate our address */
        if ((ULONG_PTR)BaseVa > (ULONG_PTR)MmPagedPoolEnd || (ULONG_PTR)BaseVa + PAGE_SIZE > (ULONG_PTR)MmPagedPoolEnd)
        {
            break;
        }

        /* Check whether we are beyond expansion */
        PointerPde = MiAddressToPde(BaseVa);
        if (PointerPde >= MmPagedPoolInfo.NextPdeForPagedPoolExpansion)
        {
            break;
        }

        /* Check if allocation is valid */
        if (MmIsAddressValid(BaseVa))
        {
            for (Entry = BaseVa;
                 (ULONG_PTR)Entry + sizeof(POOL_HEADER) < (ULONG_PTR)BaseVa + PAGE_SIZE;
                 Entry = (PVOID)((ULONG_PTR)Entry + 8))
            {
                /* Try to find whether we have a pool entry */
                if (!ExpKdbgExtValidatePoolHeader(BaseVa, Entry, PagedPool))
                {
                    continue;
                }

                if ((Entry->PoolTag & Mask) == (Tag & Mask))
                {
                    if (FoundCallback != NULL)
                    {
                        FoundCallback(Entry, CallbackContext);
                    }
                    else
                    {
                        /* Print the line */
                        KdbpPrint("%p size: %4d previous size: %4d  %s  %.4s\n",
                                  Entry, Entry->BlockSize, Entry->PreviousSize,
                                  Entry->PoolType ? "(Allocated)" : "(Free)     ",
                                  (PCHAR)&Entry->PoolTag);
                    }
                }
            }
        }

        i = RtlFindSetBits(MmPagedPoolInfo.PagedPoolAllocationMap, 1, i + 1);
    }
}

static
VOID
ExpKdbgExtPoolFindNonPagedPool(
    ULONG Tag,
    ULONG Mask,
    VOID (NTAPI* FoundCallback)(PPOOL_HEADER, PVOID),
    PVOID CallbackContext)
{
    PPOOL_HEADER Entry;
    PVOID BaseVa;

    KdbpPrint("Searching NonPaged pool (%p : %p) for Tag: %.4s\n", MmNonPagedPoolStart, MmNonPagedPoolEnd0, (PCHAR)&Tag);

    /* Brute force search: start browsing the whole non paged pool */
    for (BaseVa = MmNonPagedPoolStart;
         (ULONG_PTR)BaseVa + PAGE_SIZE <= (ULONG_PTR)MmNonPagedPoolEnd0;
         BaseVa = (PVOID)((ULONG_PTR)BaseVa + PAGE_SIZE))
    {
        Entry = BaseVa;

        /* Check whether we are beyond expansion */
        if (BaseVa >= MmNonPagedPoolExpansionStart)
        {
            break;
        }

        /* Check if allocation is valid */
        if (!MmIsAddressValid(BaseVa))
        {
            continue;
        }

        for (Entry = BaseVa;
             (ULONG_PTR)Entry + sizeof(POOL_HEADER) < (ULONG_PTR)BaseVa + PAGE_SIZE;
             Entry = (PVOID)((ULONG_PTR)Entry + 8))
        {
            /* Try to find whether we have a pool entry */
            if (!ExpKdbgExtValidatePoolHeader(BaseVa, Entry, NonPagedPool))
            {
                continue;
            }

            if ((Entry->PoolTag & Mask) == (Tag & Mask))
            {
                if (FoundCallback != NULL)
                {
                    FoundCallback(Entry, CallbackContext);
                }
                else
                {
                    /* Print the line */
                    KdbpPrint("%p size: %4d previous size: %4d  %s  %.4s\n",
                              Entry, Entry->BlockSize, Entry->PreviousSize,
                              Entry->PoolType ? "(Allocated)" : "(Free)     ",
                              (PCHAR)&Entry->PoolTag);
                }
            }
        }
    }
}

BOOLEAN
ExpKdbgExtPoolFind(
    ULONG Argc,
    PCHAR Argv[])
{
    ULONG Tag = 0;
    ULONG Mask = 0;
    ULONG PoolType = NonPagedPool;

    if (Argc == 1)
    {
        KdbpPrint("Specify a tag string\n");
        return TRUE;
    }

    /* First arg is tag */
    if (strlen(Argv[1]) != 1 || Argv[1][0] != '*')
    {
        ExpKdbgExtPoolUsedGetTag(Argv[1], &Tag, &Mask);
    }

    /* Second arg might be pool to search */
    if (Argc > 2)
    {
        PoolType = strtoul(Argv[2], NULL, 0);

        if (PoolType > 1)
        {
            KdbpPrint("Only (non) paged pool are supported\n");
            return TRUE;
        }
    }

    /* First search for large allocations */
    ExpKdbgExtPoolFindLargePool(Tag, Mask, NULL, NULL);

    if (PoolType == NonPagedPool)
    {
        ExpKdbgExtPoolFindNonPagedPool(Tag, Mask, NULL, NULL);
    }
    else if (PoolType == PagedPool)
    {
        ExpKdbgExtPoolFindPagedPool(Tag, Mask, NULL, NULL);
    }

    return TRUE;
}

VOID
NTAPI
ExpKdbgExtIrpFindPrint(
    PPOOL_HEADER Entry,
    PVOID Context)
{
    PIRP Irp;
    BOOLEAN IsComplete = FALSE;
    PIRP_FIND_CTXT FindCtxt = Context;
    PIO_STACK_LOCATION IoStack = NULL;
    PUNICODE_STRING DriverName = NULL;
    ULONG_PTR SData = FindCtxt->SData;
    ULONG Criteria = FindCtxt->Criteria;

    /* Free entry, ignore */
    if (Entry->PoolType == 0)
    {
        return;
    }

    /* Get the IRP */
    Irp = (PIRP)POOL_FREE_BLOCK(Entry);

    /* Bail out if not matching restart address */
    if ((ULONG_PTR)Irp < FindCtxt->RestartAddress)
    {
        return;
    }

    /* Avoid bogus IRP stack locations */
    if (Irp->CurrentLocation <= Irp->StackCount + 1)
    {
        IoStack = IoGetCurrentIrpStackLocation(Irp);

        /* Get associated driver */
        if (IoStack->DeviceObject && IoStack->DeviceObject->DriverObject)
            DriverName = &IoStack->DeviceObject->DriverObject->DriverName;
    }
    else
    {
        IsComplete = TRUE;
    }

    /* Display if: no data, no criteria or if criteria matches data */
    if (SData == 0 || Criteria == 0 ||
        (Criteria & 0x1 && IoStack && SData == (ULONG_PTR)IoStack->DeviceObject) ||
        (Criteria & 0x2 && SData == (ULONG_PTR)Irp->Tail.Overlay.OriginalFileObject) ||
        (Criteria & 0x4 && Irp->MdlAddress && SData == (ULONG_PTR)Irp->MdlAddress->Process) ||
        (Criteria & 0x8 && SData == (ULONG_PTR)Irp->Tail.Overlay.Thread) ||
        (Criteria & 0x10 && SData == (ULONG_PTR)Irp->UserEvent))
    {
        if (!IsComplete)
        {
            KdbpPrint("%p Thread %p current stack (%x, %x) belongs to %wZ\n", Irp, Irp->Tail.Overlay.Thread, IoStack->MajorFunction, IoStack->MinorFunction, DriverName);
        }
        else
        {
            KdbpPrint("%p Thread %p is complete (CurrentLocation %d > StackCount %d)\n", Irp, Irp->Tail.Overlay.Thread, Irp->CurrentLocation, Irp->StackCount + 1);
        }
    }
}

BOOLEAN
ExpKdbgExtIrpFind(
    ULONG Argc,
    PCHAR Argv[])
{
    ULONG PoolType = NonPagedPool;
    IRP_FIND_CTXT FindCtxt;

    /* Pool type */
    if (Argc > 1)
    {
        PoolType = strtoul(Argv[1], NULL, 0);

        if (PoolType > 1)
        {
            KdbpPrint("Only (non) paged pool are supported\n");
            return TRUE;
        }
    }

    RtlZeroMemory(&FindCtxt, sizeof(IRP_FIND_CTXT));

    /* Restart address */
    if (Argc > 2)
    {
        if (!KdbpGetHexNumber(Argv[2], &FindCtxt.RestartAddress))
        {
            KdbpPrint("Invalid parameter: %s\n", Argv[0]);
            FindCtxt.RestartAddress = 0;
        }
    }

    if (Argc > 4)
    {
        if (!KdbpGetHexNumber(Argv[4], &FindCtxt.SData))
        {
            FindCtxt.SData = 0;
        }
        else
        {
            if (strcmp(Argv[3], "device") == 0)
            {
                FindCtxt.Criteria = 0x1;
            }
            else if (strcmp(Argv[3], "fileobject") == 0)
            {
                FindCtxt.Criteria = 0x2;
            }
            else if (strcmp(Argv[3], "mdlprocess") == 0)
            {
                FindCtxt.Criteria = 0x4;
            }
            else if (strcmp(Argv[3], "thread") == 0)
            {
                FindCtxt.Criteria = 0x8;
            }
            else if (strcmp(Argv[3], "userevent") == 0)
            {
                FindCtxt.Criteria = 0x10;
            }
            else if (strcmp(Argv[3], "arg") == 0)
            {
                FindCtxt.Criteria = 0x1f;
            }
        }
    }

    if (PoolType == NonPagedPool)
    {
        ExpKdbgExtPoolFindNonPagedPool(TAG_IRP, 0xFFFFFFFF, ExpKdbgExtIrpFindPrint, &FindCtxt);
    }
    else if (PoolType == PagedPool)
    {
        ExpKdbgExtPoolFindPagedPool(TAG_IRP, 0xFFFFFFFF, ExpKdbgExtIrpFindPrint, &FindCtxt);
    }

    return TRUE;
}

#endif // DBG && KDBG

/* EOF */
