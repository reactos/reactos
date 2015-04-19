/*
 * COPYRIGHT:       GPLv2+ - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            ems.c
 * PURPOSE:         Expanded Memory Support
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "emulator.h"
#include "bios/bios32/bios32p.h"
#include "ems.h"
#include "memory.h"

/* PRIVATE VARIABLES **********************************************************/

static RTL_BITMAP AllocBitmap;
static ULONG BitmapBuffer[(EMS_TOTAL_PAGES + sizeof(ULONG) - 1) / sizeof(ULONG)];
static EMS_PAGE PageTable[EMS_TOTAL_PAGES];
static EMS_HANDLE HandleTable[EMS_MAX_HANDLES];
static PVOID Mapping[EMS_PHYSICAL_PAGES] = { NULL };

/* PRIVATE FUNCTIONS **********************************************************/

static USHORT EmsFree(USHORT Handle)
{
    PLIST_ENTRY Entry;
    PEMS_HANDLE HandleEntry = &HandleTable[Handle];

    if (Handle >= EMS_MAX_HANDLES || !HandleEntry->Allocated)
    {
        return EMS_STATUS_INVALID_HANDLE;
    }

    for (Entry = HandleEntry->PageList.Flink;
         Entry != &HandleEntry->PageList;
         Entry = Entry->Flink)
    {
        PEMS_PAGE PageEntry = (PEMS_PAGE)CONTAINING_RECORD(Entry, EMS_PAGE, Entry);
        ULONG PageNumber = ARRAY_INDEX(PageEntry, PageTable);

        /* Free the page */
        RtlClearBits(&AllocBitmap, PageNumber, 1);
    }

    HandleEntry->Allocated = FALSE;
    HandleEntry->PageCount = 0;
    InitializeListHead(&HandleEntry->PageList);

    return EMS_STATUS_OK;
}

static UCHAR EmsAlloc(USHORT NumPages, PUSHORT Handle)
{
    ULONG i, CurrentIndex = 0;
    PEMS_HANDLE HandleEntry;

    if (NumPages == 0) return EMS_STATUS_ZERO_PAGES;

    for (i = 0; i < EMS_MAX_HANDLES; i++)
    {
        HandleEntry = &HandleTable[i];
        if (!HandleEntry->Allocated)
        {
            *Handle = i;
            break;
        }
    }

    if (i == EMS_MAX_HANDLES) return EMS_STATUS_NO_MORE_HANDLES;
    HandleEntry->Allocated = TRUE;

    while (HandleEntry->PageCount < NumPages)
    {
        ULONG RunStart;
        ULONG RunSize = RtlFindNextForwardRunClear(&AllocBitmap, CurrentIndex, &RunStart);

        if (RunSize == 0)
        {
            /* Free what's been allocated already and report failure */
            EmsFree(*Handle);
            return EMS_STATUS_INSUFFICIENT_PAGES;
        }
        else if ((HandleEntry->PageCount + RunSize) > NumPages)
        {
            /* We don't need the entire run */
            RunSize = NumPages - HandleEntry->PageCount;
        }

        CurrentIndex = RunStart + RunSize;
        HandleEntry->PageCount += RunSize;
        RtlSetBits(&AllocBitmap, RunStart, RunSize);

        for (i = 0; i < RunSize; i++)
        {
            PageTable[RunStart + i].Handle = *Handle;
            InsertTailList(&HandleEntry->PageList, &PageTable[RunStart + i].Entry);
        }
    }

    return EMS_STATUS_OK;
}

static PEMS_PAGE GetLogicalPage(PEMS_HANDLE Handle, USHORT LogicalPage)
{
    PLIST_ENTRY Entry = Handle->PageList.Flink;

    while (LogicalPage)
    {
        if (Entry == &Handle->PageList) return NULL;
        LogicalPage--;
        Entry = Entry->Flink;
    }

    return (PEMS_PAGE)CONTAINING_RECORD(Entry, EMS_PAGE, Entry);
}

static USHORT EmsMap(USHORT Handle, UCHAR PhysicalPage, USHORT LogicalPage)
{
    PEMS_PAGE PageEntry;
    PEMS_HANDLE HandleEntry = &HandleTable[Handle];

    if (PhysicalPage >= EMS_PHYSICAL_PAGES) return EMS_STATUS_INV_PHYSICAL_PAGE;
    if (LogicalPage == 0xFFFF)
    {
        /* Unmap */
        Mapping[PhysicalPage] = NULL;
        return EMS_STATUS_OK;
    }

    if (Handle >= EMS_MAX_HANDLES || !HandleEntry->Allocated) return EMS_STATUS_INVALID_HANDLE;

    PageEntry = GetLogicalPage(HandleEntry, LogicalPage);
    if (!PageEntry) return EMS_STATUS_INV_LOGICAL_PAGE; 

    Mapping[PhysicalPage] = (PVOID)(EMS_ADDRESS + ARRAY_INDEX(PageEntry, PageTable) * EMS_PAGE_SIZE);
    return EMS_STATUS_OK;
}

static VOID WINAPI EmsIntHandler(LPWORD Stack)
{
    switch (getAH())
    {
        /* Get Manager Status */
        case 0x40:
        {
            setAH(EMS_STATUS_OK);
            break;
        }

        /* Get Page Frame Segment */
        case 0x41:
        {
            setAH(EMS_STATUS_OK);
            setBX(EMS_SEGMENT);
            break;
        }

        /* Get Number Of Pages */
        case 0x42:
        {
            setAH(EMS_STATUS_OK);
            setBX(RtlNumberOfClearBits(&AllocBitmap));
            setDX(EMS_TOTAL_PAGES);
            break;
        }

        /* Get Handle And Allocate Memory */
        case 0x43:
        {
            USHORT Handle;
            UCHAR Status = EmsAlloc(getBX(), &Handle);

            setAH(Status);
            if (Status == EMS_STATUS_OK) setDX(Handle);
            break;
        }

        /* Map Memory */
        case 0x44:
        {
            setAH(EmsMap(getDX(), getAL(), getBX()));
            break;
        }

        /* Release Handle And Memory */
        case 0x45:
        {
            setAH(EmsFree(getDX()));
            break;
        }

        /* Get EMM Version */
        case 0x46:
        {
            setAH(EMS_STATUS_OK);
            setAL(EMS_VERSION_NUM);
            break;
        }

        /* Move/Exchange Memory */
        case 0x57:
        {
            PUCHAR SourcePtr, DestPtr;
            PEMS_HANDLE HandleEntry;
            PEMS_PAGE PageEntry;
            BOOLEAN Exchange = getAL();
            PEMS_COPY_DATA Data = (PEMS_COPY_DATA)SEG_OFF_TO_PTR(getDS(), getSI());

            if (Data->SourceType)
            {
                /* Expanded memory */
                HandleEntry = &HandleTable[Data->SourceHandle];

                if (Data->SourceHandle >= EMS_MAX_HANDLES || !HandleEntry->Allocated)
                {
                    setAL(EMS_STATUS_INVALID_HANDLE);
                    break;
                }

                PageEntry = GetLogicalPage(HandleEntry, Data->SourceSegment);

                if (!PageEntry)
                {
                    setAL(EMS_STATUS_INV_LOGICAL_PAGE);
                    break;
                }

                SourcePtr = (PUCHAR)REAL_TO_PHYS(EMS_ADDRESS
                                                 + ARRAY_INDEX(PageEntry, PageTable)
                                                 * EMS_PAGE_SIZE
                                                 + Data->SourceOffset);
            }
            else
            {
                /* Conventional memory */
                SourcePtr = (PUCHAR)SEG_OFF_TO_PTR(Data->SourceSegment, Data->SourceOffset);
            }

            if (Data->DestType)
            {
                /* Expanded memory */
                HandleEntry = &HandleTable[Data->DestHandle];

                if (Data->SourceHandle >= EMS_MAX_HANDLES || !HandleEntry->Allocated)
                {
                    setAL(EMS_STATUS_INVALID_HANDLE);
                    break;
                }

                PageEntry = GetLogicalPage(HandleEntry, Data->DestSegment);

                if (!PageEntry)
                {
                    setAL(EMS_STATUS_INV_LOGICAL_PAGE);
                    break;
                }

                DestPtr = (PUCHAR)REAL_TO_PHYS(EMS_ADDRESS
                                               + ARRAY_INDEX(PageEntry, PageTable)
                                               * EMS_PAGE_SIZE
                                               + Data->DestOffset);
            }
            else
            {
                /* Conventional memory */
                DestPtr = (PUCHAR)SEG_OFF_TO_PTR(Data->DestSegment, Data->DestOffset);
            }

            if (Exchange)
            {
                ULONG i;

                /* Exchange */
                for (i = 0; i < Data->RegionLength; i++)
                {
                    UCHAR Temp = DestPtr[i];
                    DestPtr[i] = SourcePtr[i];
                    SourcePtr[i] = Temp;
                }
            }
            else
            {
                /* Move */
                RtlMoveMemory(DestPtr, SourcePtr, Data->RegionLength);
            }

            setAL(EMS_STATUS_OK);
            break;
        }

        default:
        {
            DPRINT1("EMS function AH = %02X NOT IMPLEMENTED\n", getAH());
            setAH(EMS_STATUS_UNKNOWN_FUNCTION);
            break;
        }
    }
}

static VOID NTAPI EmsReadMemory(ULONG Address, PVOID Buffer, ULONG Size)
{
    ULONG i;
    ULONG RelativeAddress = Address - TO_LINEAR(EMS_SEGMENT, 0);
    ULONG FirstPage = RelativeAddress / EMS_PAGE_SIZE;
    ULONG LastPage = (RelativeAddress + Size - 1) / EMS_PAGE_SIZE;
    ULONG Offset, Length;

    for (i = FirstPage; i <= LastPage; i++)
    {
        Offset = (i == FirstPage) ? RelativeAddress & (EMS_PAGE_SIZE - 1) : 0;
        Length = ((i == LastPage)
                 ? (RelativeAddress + Size - (LastPage << EMS_PAGE_BITS))
                 : EMS_PAGE_SIZE) - Offset;

        if (Mapping[i]) RtlCopyMemory(Buffer, (PVOID)((ULONG_PTR)Mapping[i] + Offset), Length);
        Buffer = (PVOID)((ULONG_PTR)Buffer + Length);
    }
}

static BOOLEAN NTAPI EmsWriteMemory(ULONG Address, PVOID Buffer, ULONG Size)
{
    ULONG i;
    ULONG RelativeAddress = Address - TO_LINEAR(EMS_SEGMENT, 0);
    ULONG FirstPage = RelativeAddress / EMS_PAGE_SIZE;
    ULONG LastPage = (RelativeAddress + Size - 1) / EMS_PAGE_SIZE;
    ULONG Offset, Length;

    for (i = FirstPage; i <= LastPage; i++)
    {
        Offset = (i == FirstPage) ? RelativeAddress & (EMS_PAGE_SIZE - 1) : 0;
        Length = ((i == LastPage)
                 ? (RelativeAddress + Size - (LastPage << EMS_PAGE_BITS))
                 : EMS_PAGE_SIZE) - Offset;

        if (Mapping[i]) RtlCopyMemory((PVOID)((ULONG_PTR)Mapping[i] + Offset), Buffer, Length);
        Buffer = (PVOID)((ULONG_PTR)Buffer + Length);
    }

    return TRUE;
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID EmsInitialize(VOID)
{
    ULONG i;

    RtlZeroMemory(BitmapBuffer, sizeof(BitmapBuffer));
    RtlInitializeBitMap(&AllocBitmap, BitmapBuffer, EMS_TOTAL_PAGES);

    for (i = 0; i < EMS_MAX_HANDLES; i++)
    {
        HandleTable[i].Allocated = FALSE;
        HandleTable[i].PageCount = 0;
        InitializeListHead(&HandleTable[i].PageList);
    }

    MemInstallFastMemoryHook((PVOID)TO_LINEAR(EMS_SEGMENT, 0),
                             EMS_PHYSICAL_PAGES * EMS_PAGE_SIZE,
                             EmsReadMemory,
                             EmsWriteMemory);

    RegisterBiosInt32(EMS_INTERRUPT_NUM, EmsIntHandler);
}

VOID EmsCleanup(VOID)
{
    MemRemoveFastMemoryHook((PVOID)TO_LINEAR(EMS_SEGMENT, 0),
                            EMS_PHYSICAL_PAGES * EMS_PAGE_SIZE);
}
