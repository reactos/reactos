/*
 * COPYRIGHT:       GPLv2+ - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            emsdrv.c
 * PURPOSE:         DOS EMS Driver
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "emulator.h"

#include "dos.h"
#include "dos/dem.h"
#include "device.h"

#include "../../memory.h"
#include "emsdrv.h"

#define EMS_DEVICE_NAME "EMMXXXX0"

/* PRIVATE VARIABLES **********************************************************/

static PDOS_DEVICE_NODE Node;
static RTL_BITMAP AllocBitmap;
static PULONG BitmapBuffer = NULL;
static PEMS_PAGE PageTable = NULL;
static EMS_HANDLE HandleTable[EMS_MAX_HANDLES];
static PVOID Mapping[EMS_PHYSICAL_PAGES] = { NULL };
static ULONG EmsTotalPages = 0;
static PVOID EmsMemory = NULL;

/* PRIVATE FUNCTIONS **********************************************************/

static inline PEMS_HANDLE GetHandleRecord(USHORT Handle)
{
    if (Handle >= EMS_MAX_HANDLES) return NULL;
    return &HandleTable[Handle];
}

static USHORT EmsFree(USHORT Handle)
{
    PLIST_ENTRY Entry;
    PEMS_HANDLE HandleEntry = GetHandleRecord(Handle);

    if (HandleEntry == NULL || !HandleEntry->Allocated)
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
    PEMS_HANDLE HandleEntry = GetHandleRecord(Handle);

    if (PhysicalPage >= EMS_PHYSICAL_PAGES) return EMS_STATUS_INV_PHYSICAL_PAGE;
    if (LogicalPage == 0xFFFF)
    {
        /* Unmap */
        Mapping[PhysicalPage] = NULL;
        return EMS_STATUS_OK;
    }

    if (HandleEntry == NULL || !HandleEntry->Allocated)
    {
        return EMS_STATUS_INVALID_HANDLE;
    }

    PageEntry = GetLogicalPage(HandleEntry, LogicalPage);
    if (!PageEntry) return EMS_STATUS_INV_LOGICAL_PAGE; 

    Mapping[PhysicalPage] = (PVOID)((ULONG_PTR)EmsMemory
                            + ARRAY_INDEX(PageEntry, PageTable) * EMS_PAGE_SIZE);
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
            setDX(EmsTotalPages);
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

        /* Get/Set Handle Name */
        case 0x53:
        {
            PEMS_HANDLE HandleEntry = GetHandleRecord(getDX());
            if (HandleEntry == NULL || !HandleEntry->Allocated)
            {
                setAL(EMS_STATUS_INVALID_HANDLE);
                break;
            }

            if (getAL() == 0x00)
            {
                /* Retrieve the name */
                RtlCopyMemory(SEG_OFF_TO_PTR(getES(), getDI()),
                              HandleEntry->Name,
                              sizeof(HandleEntry->Name));
                setAH(EMS_STATUS_OK);
            }
            else if (getAL() == 0x01)
            {
                /* Store the name */
                RtlCopyMemory(HandleEntry->Name,
                              SEG_OFF_TO_PTR(getDS(), getSI()),
                              sizeof(HandleEntry->Name));
                setAH(EMS_STATUS_OK);
            }
            else
            {
                DPRINT1("Invalid subfunction %02X for EMS function AH = 53h\n", getAL());
                setAH(EMS_STATUS_UNKNOWN_FUNCTION);
            }

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
                HandleEntry = GetHandleRecord(Data->SourceHandle);

                if (HandleEntry == NULL || !HandleEntry->Allocated)
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

                SourcePtr = (PUCHAR)((ULONG_PTR)EmsMemory
                                     + ARRAY_INDEX(PageEntry, PageTable) * EMS_PAGE_SIZE
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
                HandleEntry = GetHandleRecord(Data->DestHandle);

                if (HandleEntry == NULL || !HandleEntry->Allocated)
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

                DestPtr = (PUCHAR)((ULONG_PTR)EmsMemory
                                   + ARRAY_INDEX(PageEntry, PageTable) * EMS_PAGE_SIZE
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


WORD NTAPI EmsDrvDispatchIoctlRead(PDOS_DEVICE_NODE Device, DWORD Buffer, PWORD Length)
{
    // TODO: NOT IMPLEMENTED
    UNIMPLEMENTED;

    return DOS_DEVSTAT_DONE;
}

/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN EmsDrvInitialize(ULONG TotalPages)
{
    ULONG i;

    for (i = 0; i < EMS_MAX_HANDLES; i++)
    {
        HandleTable[i].Allocated = FALSE;
        HandleTable[i].PageCount = 0;
        InitializeListHead(&HandleTable[i].PageList);
    }

    EmsTotalPages = TotalPages;
    BitmapBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                   HEAP_ZERO_MEMORY,
                                   ((TotalPages + 31) / 32) * sizeof(ULONG));
    if (BitmapBuffer == NULL) return FALSE;

    RtlInitializeBitMap(&AllocBitmap, BitmapBuffer, TotalPages);

    PageTable = (PEMS_PAGE)RtlAllocateHeap(RtlGetProcessHeap(),
                                           HEAP_ZERO_MEMORY,
                                           TotalPages * sizeof(EMS_PAGE));
    if (PageTable == NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, BitmapBuffer);
        BitmapBuffer = NULL;

        return FALSE;
    }

    EmsMemory = (PVOID)RtlAllocateHeap(RtlGetProcessHeap(), 0, TotalPages * EMS_PAGE_SIZE);
    if (EmsMemory == NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, PageTable);
        PageTable = NULL;
        RtlFreeHeap(RtlGetProcessHeap(), 0, BitmapBuffer);
        BitmapBuffer = NULL;

        return FALSE;
    }

    MemInstallFastMemoryHook((PVOID)TO_LINEAR(EMS_SEGMENT, 0),
                             EMS_PHYSICAL_PAGES * EMS_PAGE_SIZE,
                             EmsReadMemory,
                             EmsWriteMemory);


    /* Create the device */
    Node = DosCreateDeviceEx(DOS_DEVATTR_IOCTL | DOS_DEVATTR_CHARACTER,
                             EMS_DEVICE_NAME,
                             32);
    Node->IoctlReadRoutine = EmsDrvDispatchIoctlRead;

    RegisterInt32(MAKELONG(sizeof(DOS_DRIVER) + DEVICE_CODE_SIZE, HIWORD(Node->Driver)),
                  EMS_INTERRUPT_NUM,
                  EmsIntHandler,
                  NULL);

    return TRUE;
}

VOID EmsDrvCleanup(VOID)
{
    /* Delete the device */
    DosDeleteDevice(Node);

    MemRemoveFastMemoryHook((PVOID)TO_LINEAR(EMS_SEGMENT, 0),
                            EMS_PHYSICAL_PAGES * EMS_PAGE_SIZE);

    if (EmsMemory)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, EmsMemory);
        EmsMemory = NULL;
    }

    if (PageTable)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, PageTable);
        PageTable = NULL;
    }

    if (BitmapBuffer)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, BitmapBuffer);
        BitmapBuffer = NULL;
    }
}
