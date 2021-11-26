/*
 * COPYRIGHT:       GPLv2+ - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/dos/dos32krnl/emsdrv.c
 * PURPOSE:         DOS EMS Driver
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *
 * DOCUMENTATION:   Official specification:
 *                  LIM EMS v4.0: http://www.phatcode.net/res/218/files/limems40.txt
 */

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

#define NDEBUG
#include <debug.h>

#include "emulator.h"
#include "../../memory.h"
#include "bios/umamgr.h"

#include "dos.h"
#include "dos/dem.h"
#include "device.h"

#include "emsdrv.h"

#define EMS_DEVICE_NAME     "EMMXXXX0"

#define EMS_SEGMENT_SIZE    ((EMS_PHYSICAL_PAGES * EMS_PAGE_SIZE) >> 4)
#define EMS_SYSTEM_HANDLE   0

/* PRIVATE VARIABLES **********************************************************/

static PDOS_DEVICE_NODE Node;
static RTL_BITMAP AllocBitmap;
static PULONG EmsBitmapBuffer = NULL;
static PEMS_PAGE EmsPageTable = NULL;
static EMS_HANDLE EmsHandleTable[EMS_MAX_HANDLES];
static PVOID Mapping[EMS_PHYSICAL_PAGES] = { NULL };
static PVOID MappingBackup[EMS_PHYSICAL_PAGES] = { NULL };
static ULONG EmsTotalPages = 0;
static PVOID EmsMemory = NULL;
static USHORT EmsSegment = EMS_SEGMENT;

/* PRIVATE FUNCTIONS **********************************************************/

static VOID InitHandlesTable(VOID)
{
    USHORT i;

    for (i = 0; i < ARRAYSIZE(EmsHandleTable); i++)
    {
        EmsHandleTable[i].Allocated = FALSE;
        EmsHandleTable[i].PageCount = 0;
        RtlZeroMemory(EmsHandleTable[i].Name, sizeof(EmsHandleTable[i].Name));
        InitializeListHead(&EmsHandleTable[i].PageList);
    }
}

static PEMS_HANDLE CreateHandle(PUSHORT Handle)
{
    PEMS_HANDLE HandleEntry;
    USHORT i;

    /* Handle 0 is reserved (system handle) */
    for (i = 1; i < ARRAYSIZE(EmsHandleTable); i++)
    {
        HandleEntry = &EmsHandleTable[i];
        if (!HandleEntry->Allocated)
        {
            *Handle = i;
            HandleEntry->Allocated = TRUE;
            return HandleEntry;
        }
    }

    return NULL;
}

static VOID FreeHandle(PEMS_HANDLE HandleEntry)
{
    HandleEntry->Allocated = FALSE;
    HandleEntry->PageCount = 0;
    RtlZeroMemory(HandleEntry->Name, sizeof(HandleEntry->Name));
    // InitializeListHead(&HandleEntry->PageList);
}

static inline PEMS_HANDLE GetEmsHandleRecord(USHORT Handle)
{
    if (Handle >= ARRAYSIZE(EmsHandleTable)) return NULL;
    return &EmsHandleTable[Handle];
}

static inline BOOLEAN ValidateHandle(PEMS_HANDLE HandleEntry)
{
    return (HandleEntry != NULL && HandleEntry->Allocated);
}

static UCHAR EmsFree(USHORT Handle)
{
    PLIST_ENTRY Entry;
    PEMS_HANDLE HandleEntry = GetEmsHandleRecord(Handle);

    if (!ValidateHandle(HandleEntry))
        return EMS_STATUS_INVALID_HANDLE;

    for (Entry = HandleEntry->PageList.Flink;
         Entry != &HandleEntry->PageList;
         Entry = Entry->Flink)
    {
        PEMS_PAGE PageEntry = (PEMS_PAGE)CONTAINING_RECORD(Entry, EMS_PAGE, Entry);
        ULONG PageNumber = ARRAY_INDEX(PageEntry, EmsPageTable);

        /* Free the page */
        RtlClearBits(&AllocBitmap, PageNumber, 1);
    }

    InitializeListHead(&HandleEntry->PageList);

    if (Handle != EMS_SYSTEM_HANDLE)
        FreeHandle(HandleEntry);

    return EMS_STATUS_SUCCESS;
}

static UCHAR EmsAlloc(USHORT NumPages, PUSHORT Handle)
{
    ULONG i, CurrentIndex = 0;
    PEMS_HANDLE HandleEntry;

    if (NumPages == 0) return EMS_STATUS_ZERO_PAGES;

    HandleEntry = CreateHandle(Handle);
    if (!HandleEntry)  return EMS_STATUS_NO_MORE_HANDLES;

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
            EmsPageTable[RunStart + i].Handle = *Handle;
            InsertTailList(&HandleEntry->PageList, &EmsPageTable[RunStart + i].Entry);
        }
    }

    return EMS_STATUS_SUCCESS;
}

static UCHAR InitSystemHandle(USHORT NumPages)
{
    //
    // FIXME: This is an adapted copy of EmsAlloc!!
    //

    ULONG i, CurrentIndex = 0;
    PEMS_HANDLE HandleEntry = &EmsHandleTable[EMS_SYSTEM_HANDLE];

    /* The system handle must never have been initialized before */
    ASSERT(!HandleEntry->Allocated);

    /* Now allocate it */
    HandleEntry->Allocated = TRUE;

    while (HandleEntry->PageCount < NumPages)
    {
        ULONG RunStart;
        ULONG RunSize = RtlFindNextForwardRunClear(&AllocBitmap, CurrentIndex, &RunStart);

        if (RunSize == 0)
        {
            /* Free what's been allocated already and report failure */
            EmsFree(EMS_SYSTEM_HANDLE);
            // FIXME: For this function (and EmsAlloc as well),
            // use instead an internal function that just uses
            // PEMS_HANDLE pointers instead. It's only in the
            // EMS interrupt handler that we should do the
            // unfolding.
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
            EmsPageTable[RunStart + i].Handle = EMS_SYSTEM_HANDLE;
            InsertTailList(&HandleEntry->PageList, &EmsPageTable[RunStart + i].Entry);
        }
    }

    return EMS_STATUS_SUCCESS;
}

static PEMS_PAGE GetLogicalPage(PEMS_HANDLE HandleEntry, USHORT LogicalPage)
{
    PLIST_ENTRY Entry = HandleEntry->PageList.Flink;

    while (LogicalPage)
    {
        if (Entry == &HandleEntry->PageList) return NULL;
        LogicalPage--;
        Entry = Entry->Flink;
    }

    return (PEMS_PAGE)CONTAINING_RECORD(Entry, EMS_PAGE, Entry);
}

static UCHAR EmsMap(USHORT Handle, UCHAR PhysicalPage, USHORT LogicalPage)
{
    PEMS_PAGE PageEntry;
    PEMS_HANDLE HandleEntry = GetEmsHandleRecord(Handle);

    if (!ValidateHandle(HandleEntry))
        return EMS_STATUS_INVALID_HANDLE;

    if (PhysicalPage >= EMS_PHYSICAL_PAGES)
        return EMS_STATUS_INV_PHYSICAL_PAGE;

    if (LogicalPage == 0xFFFF)
    {
        /* Unmap */
        Mapping[PhysicalPage] = NULL;
        return EMS_STATUS_SUCCESS;
    }

    PageEntry = GetLogicalPage(HandleEntry, LogicalPage);
    if (!PageEntry) return EMS_STATUS_INV_LOGICAL_PAGE;

    Mapping[PhysicalPage] = (PVOID)((ULONG_PTR)EmsMemory
                            + ARRAY_INDEX(PageEntry, EmsPageTable) * EMS_PAGE_SIZE);
    return EMS_STATUS_SUCCESS;
}

static VOID WINAPI EmsIntHandler(LPWORD Stack)
{
    switch (getAH())
    {
        /* Get Manager Status */
        case 0x40:
        {
            setAH(EMS_STATUS_SUCCESS);
            break;
        }

        /* Get Page Frame Segment */
        case 0x41:
        {
            setAH(EMS_STATUS_SUCCESS);
            setBX(EmsSegment);
            break;
        }

        /* Get Number of Unallocated Pages */
        case 0x42:
        {
            setAH(EMS_STATUS_SUCCESS);
            setBX(RtlNumberOfClearBits(&AllocBitmap));
            setDX(EmsTotalPages);
            break;
        }

        /* Get Handle and Allocate Memory */
        case 0x43:
        {
            USHORT Handle;
            UCHAR Status = EmsAlloc(getBX(), &Handle);

            if (Status == EMS_STATUS_SUCCESS)
                setDX(Handle);

            setAH(Status);
            break;
        }

        /* Map Memory */
        case 0x44:
        {
            setAH(EmsMap(getDX(), getAL(), getBX()));
            break;
        }

        /* Release Handle and Memory */
        case 0x45:
        {
            setAH(EmsFree(getDX()));
            break;
        }

        /* Get EMM Version */
        case 0x46:
        {
            setAH(EMS_STATUS_SUCCESS);
            setAL(EMS_VERSION_NUM);
            break;
        }

        /* Save Page Map */
        case 0x47:
        {
            // FIXME: This depends on an EMS handle given in DX
            RtlCopyMemory(MappingBackup, Mapping, sizeof(Mapping));
            setAH(EMS_STATUS_SUCCESS);
            break;
        }

        /* Restore Page Map */
        case 0x48:
        {
            // FIXME: This depends on an EMS handle given in DX
            RtlCopyMemory(Mapping, MappingBackup, sizeof(Mapping));
            setAH(EMS_STATUS_SUCCESS);
            break;
        }

        /* Get Number of Opened Handles */
        case 0x4B:
        {
            USHORT NumOpenHandles = 0;
            USHORT i;

            for (i = 0; i < ARRAYSIZE(EmsHandleTable); i++)
            {
                if (EmsHandleTable[i].Allocated)
                    ++NumOpenHandles;
            }

            setAH(EMS_STATUS_SUCCESS);
            setBX(NumOpenHandles);
            break;
        }

        /* Get Handle Number of Pages */
        case 0x4C:
        {
            PEMS_HANDLE HandleEntry = GetEmsHandleRecord(getDX());

            if (!ValidateHandle(HandleEntry))
            {
                setAH(EMS_STATUS_INVALID_HANDLE);
                break;
            }

            setAH(EMS_STATUS_SUCCESS);
            setBX(HandleEntry->PageCount);
            break;
        }

        /* Get All Handles Number of Pages */
        case 0x4D:
        {
            PEMS_HANDLE_PAGE_INFO HandlePageInfo = (PEMS_HANDLE_PAGE_INFO)SEG_OFF_TO_PTR(getES(), getDI());
            USHORT NumOpenHandles = 0;
            USHORT i;

            for (i = 0; i < ARRAYSIZE(EmsHandleTable); i++)
            {
                if (EmsHandleTable[i].Allocated)
                {
                    HandlePageInfo->Handle = i;
                    HandlePageInfo->PageCount = EmsHandleTable[i].PageCount;
                    ++HandlePageInfo;
                    ++NumOpenHandles;
                }
            }

            setAH(EMS_STATUS_SUCCESS);
            setBX(NumOpenHandles);
            break;
        }

        /* Get or Set Page Map */
        case 0x4E:
        {
            switch (getAL())
            {
                /* Get Mapping Registers  */
                // case 0x00: // TODO: NOT IMPLEMENTED

                /* Set Mapping Registers */
                // case 0x01: // TODO: NOT IMPLEMENTED

                /* Get and Set Mapping Registers At Once */
                // case 0x02: // TODO: NOT IMPLEMENTED

                /* Get Size of Page-Mapping Array */
                case 0x03:
                {
                    setAH(EMS_STATUS_SUCCESS);
                    setAL(sizeof(Mapping));
                    break;
                }

                default:
                {
                    DPRINT1("EMS function AH = 0x4E, subfunction AL = %02X NOT IMPLEMENTED\n", getAL());
                    setAH(EMS_STATUS_UNKNOWN_FUNCTION);
                    break;
                }
            }

            break;
        }

        /* Get/Set Handle Name */
        case 0x53:
        {
            PEMS_HANDLE HandleEntry = GetEmsHandleRecord(getDX());

            if (!ValidateHandle(HandleEntry))
            {
                setAH(EMS_STATUS_INVALID_HANDLE);
                break;
            }

            if (getAL() == 0x00)
            {
                /* Retrieve the name */
                RtlCopyMemory(SEG_OFF_TO_PTR(getES(), getDI()),
                              HandleEntry->Name,
                              sizeof(HandleEntry->Name));
                setAH(EMS_STATUS_SUCCESS);
            }
            else if (getAL() == 0x01)
            {
                /* Store the name */
                RtlCopyMemory(HandleEntry->Name,
                              SEG_OFF_TO_PTR(getDS(), getSI()),
                              sizeof(HandleEntry->Name));
                setAH(EMS_STATUS_SUCCESS);
            }
            else
            {
                DPRINT1("Invalid subfunction %02X for EMS function AH = 53h\n", getAL());
                setAH(EMS_STATUS_INVALID_SUBFUNCTION);
            }

            break;
        }

        /* Handle Directory functions */
        case 0x54:
        {
            if (getAL() == 0x00)
            {
                /* Get Handle Directory */

                PEMS_HANDLE_DIR_ENTRY HandleDir = (PEMS_HANDLE_DIR_ENTRY)SEG_OFF_TO_PTR(getES(), getDI());
                USHORT NumOpenHandles = 0;
                USHORT i;

                for (i = 0; i < ARRAYSIZE(EmsHandleTable); i++)
                {
                    if (EmsHandleTable[i].Allocated)
                    {
                        HandleDir->Handle = i;
                        RtlCopyMemory(HandleDir->Name,
                                      EmsHandleTable[i].Name,
                                      sizeof(HandleDir->Name));
                        ++HandleDir;
                        ++NumOpenHandles;
                    }
                }

                setAH(EMS_STATUS_SUCCESS);
                setAL((UCHAR)NumOpenHandles);
            }
            else if (getAL() == 0x01)
            {
                /* Search for Named Handle */

                PUCHAR HandleName = (PUCHAR)SEG_OFF_TO_PTR(getDS(), getSI());
                PEMS_HANDLE HandleFound = NULL;
                USHORT i;

                for (i = 0; i < ARRAYSIZE(EmsHandleTable); i++)
                {
                    if (EmsHandleTable[i].Allocated &&
                        RtlCompareMemory(HandleName,
                                         EmsHandleTable[i].Name,
                                         sizeof(EmsHandleTable[i].Name)) == sizeof(EmsHandleTable[i].Name))
                    {
                        HandleFound = &EmsHandleTable[i];
                        break;
                    }
                }

                /* Bail out if no handle was found */
                if (i >= ARRAYSIZE(EmsHandleTable)) // HandleFound == NULL
                {
                    setAH(EMS_STATUS_HANDLE_NOT_FOUND);
                    break;
                }

                /* Return the handle number */
                setDX(i);

                /* Sanity check: Check whether the handle was unnamed */
                i = 0;
                while ((i < sizeof(HandleFound->Name)) && (HandleFound->Name[i] == '\0'))
                    ++i;

                if (i >= sizeof(HandleFound->Name))
                {
                    setAH(EMS_STATUS_UNNAMED_HANDLE);
                }
                else
                {
                    setAH(EMS_STATUS_SUCCESS);
                }
            }
            else if (getAL() == 0x02)
            {
                /*
                 * Get Total Number of Handles
                 *
                 * This function retrieves the maximum number of handles
                 * (allocated or not) the memory manager supports, which
                 * a program may request.
                 */
                setAH(EMS_STATUS_SUCCESS);
                setBX(ARRAYSIZE(EmsHandleTable));
            }
            else
            {
                DPRINT1("Invalid subfunction %02X for EMS function AH = 54h\n", getAL());
                setAH(EMS_STATUS_INVALID_SUBFUNCTION);
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
                HandleEntry = GetEmsHandleRecord(Data->SourceHandle);
                if (!ValidateHandle(HandleEntry))
                {
                    setAH(EMS_STATUS_INVALID_HANDLE);
                    break;
                }

                PageEntry = GetLogicalPage(HandleEntry, Data->SourceSegment);
                if (!PageEntry)
                {
                    setAH(EMS_STATUS_INV_LOGICAL_PAGE);
                    break;
                }

                SourcePtr = (PUCHAR)((ULONG_PTR)EmsMemory
                                     + ARRAY_INDEX(PageEntry, EmsPageTable) * EMS_PAGE_SIZE
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
                HandleEntry = GetEmsHandleRecord(Data->DestHandle);
                if (!ValidateHandle(HandleEntry))
                {
                    setAH(EMS_STATUS_INVALID_HANDLE);
                    break;
                }

                PageEntry = GetLogicalPage(HandleEntry, Data->DestSegment);
                if (!PageEntry)
                {
                    setAH(EMS_STATUS_INV_LOGICAL_PAGE);
                    break;
                }

                DestPtr = (PUCHAR)((ULONG_PTR)EmsMemory
                                   + ARRAY_INDEX(PageEntry, EmsPageTable) * EMS_PAGE_SIZE
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

            setAH(EMS_STATUS_SUCCESS);
            break;
        }

        /* Get Mappable Physical Address Array */
        case 0x58:
        {
            if (getAL() == 0x00)
            {
                PEMS_MAPPABLE_PHYS_PAGE PageArray = (PEMS_MAPPABLE_PHYS_PAGE)SEG_OFF_TO_PTR(getES(), getDI());
                ULONG i;

                for (i = 0; i < EMS_PHYSICAL_PAGES; i++)
                {
                    PageArray->PageSegment = EMS_SEGMENT + i * (EMS_PAGE_SIZE >> 4);
                    PageArray->PageNumber  = i;
                    ++PageArray;
                }

                setAH(EMS_STATUS_SUCCESS);
                setCX(EMS_PHYSICAL_PAGES);
            }
            else if (getAL() == 0x01)
            {
                setAH(EMS_STATUS_SUCCESS);
                setCX(EMS_PHYSICAL_PAGES);
            }
            else
            {
                DPRINT1("Invalid subfunction %02X for EMS function AH = 58h\n", getAL());
                setAH(EMS_STATUS_INVALID_SUBFUNCTION);
            }

            break;
        }

        /* Get Expanded Memory Hardware Information */
        case 0x59:
        {
            if (getAL() == 0x00)
            {
                PEMS_HARDWARE_INFO HardwareInfo = (PEMS_HARDWARE_INFO)SEG_OFF_TO_PTR(getES(), getDI());

                /* Return the hardware information */
                HardwareInfo->RawPageSize         = EMS_PAGE_SIZE >> 4;
                HardwareInfo->AlternateRegSets    = 0;
                HardwareInfo->ContextAreaSize     = sizeof(Mapping);
                HardwareInfo->DmaRegisterSets     = 0;
                HardwareInfo->DmaChannelOperation = 0;

                setAH(EMS_STATUS_SUCCESS);
            }
            else if (getAL() == 0x01)
            {
                /* Same as function AH = 42h */
                setAH(EMS_STATUS_SUCCESS);
                setBX(RtlNumberOfClearBits(&AllocBitmap));
                setDX(EmsTotalPages);
            }
            else
            {
                DPRINT1("Invalid subfunction %02X for EMS function AH = 59h\n", getAL());
                setAH(EMS_STATUS_INVALID_SUBFUNCTION);
            }

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

static VOID FASTCALL EmsReadMemory(ULONG Address, PVOID Buffer, ULONG Size)
{
    ULONG i;
    ULONG RelativeAddress = Address - TO_LINEAR(EmsSegment, 0);
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

static BOOLEAN FASTCALL EmsWriteMemory(ULONG Address, PVOID Buffer, ULONG Size)
{
    ULONG i;
    ULONG RelativeAddress = Address - TO_LINEAR(EmsSegment, 0);
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

static WORD NTAPI EmsDrvDispatchIoctlRead(PDOS_DEVICE_NODE Device, DWORD Buffer, PWORD Length)
{
    // TODO: NOT IMPLEMENTED
    UNIMPLEMENTED;
    return DOS_DEVSTAT_DONE;
}

/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN EmsDrvInitialize(USHORT Segment, ULONG TotalPages)
{
    USHORT Size;

    /* Try to allocate our page table in UMA at the given segment */
    EmsSegment = (Segment != 0 ? Segment : EMS_SEGMENT);
    Size = EMS_SEGMENT_SIZE; // Size in paragraphs
    if (!UmaDescReserve(&EmsSegment, &Size)) return FALSE;

    EmsTotalPages = TotalPages;
    EmsBitmapBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                      HEAP_ZERO_MEMORY,
                                      ((TotalPages + 31) / 32) * sizeof(ULONG));
    if (EmsBitmapBuffer == NULL)
    {
        UmaDescRelease(EmsSegment);
        return FALSE;
    }

    RtlInitializeBitMap(&AllocBitmap, EmsBitmapBuffer, TotalPages);

    EmsPageTable = (PEMS_PAGE)RtlAllocateHeap(RtlGetProcessHeap(),
                                              HEAP_ZERO_MEMORY,
                                              TotalPages * sizeof(EMS_PAGE));
    if (EmsPageTable == NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, EmsBitmapBuffer);
        EmsBitmapBuffer = NULL;

        UmaDescRelease(EmsSegment);
        return FALSE;
    }

    EmsMemory = (PVOID)RtlAllocateHeap(RtlGetProcessHeap(), 0, TotalPages * EMS_PAGE_SIZE);
    if (EmsMemory == NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, EmsPageTable);
        EmsPageTable = NULL;
        RtlFreeHeap(RtlGetProcessHeap(), 0, EmsBitmapBuffer);
        EmsBitmapBuffer = NULL;

        UmaDescRelease(EmsSegment);
        return FALSE;
    }

    InitHandlesTable();
    /*
     * FIXME: We should ensure that the system handle is associated
     * with mapped pages from conventional memory. DosEmu seems to do
     * it correctly. 384kB of memory mapped.
     */
    if (InitSystemHandle(384/16) != EMS_STATUS_SUCCESS)
    {
        DPRINT1("Impossible to allocate pages for the system handle!\n");

        RtlFreeHeap(RtlGetProcessHeap(), 0, EmsMemory);
        EmsMemory = NULL;
        RtlFreeHeap(RtlGetProcessHeap(), 0, EmsPageTable);
        EmsPageTable = NULL;
        RtlFreeHeap(RtlGetProcessHeap(), 0, EmsBitmapBuffer);
        EmsBitmapBuffer = NULL;

        UmaDescRelease(EmsSegment);
        return FALSE;
    }

    MemInstallFastMemoryHook(UlongToPtr(TO_LINEAR(EmsSegment, 0)),
                             EMS_PHYSICAL_PAGES * EMS_PAGE_SIZE,
                             EmsReadMemory,
                             EmsWriteMemory);

    /* Create the device */
    Node = DosCreateDeviceEx(DOS_DEVATTR_IOCTL | DOS_DEVATTR_CHARACTER,
                             EMS_DEVICE_NAME,
                             Int16To32StubSize);
    Node->IoctlReadRoutine = EmsDrvDispatchIoctlRead;

    RegisterInt32(DEVICE_PRIVATE_AREA(Node->Driver),
                  EMS_INTERRUPT_NUM, EmsIntHandler, NULL);

    return TRUE;
}

VOID EmsDrvCleanup(VOID)
{
    /* Delete the device */
    DosDeleteDevice(Node);

    MemRemoveFastMemoryHook(UlongToPtr(TO_LINEAR(EmsSegment, 0)),
                            EMS_PHYSICAL_PAGES * EMS_PAGE_SIZE);

    if (EmsMemory)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, EmsMemory);
        EmsMemory = NULL;
    }

    if (EmsPageTable)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, EmsPageTable);
        EmsPageTable = NULL;
    }

    if (EmsBitmapBuffer)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, EmsBitmapBuffer);
        EmsBitmapBuffer = NULL;
    }

    UmaDescRelease(EmsSegment);
}
