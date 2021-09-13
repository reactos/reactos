/*
 * COPYRIGHT:       GPLv2+ - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/dos/dos32krnl/himem.c
 * PURPOSE:         DOS XMS Driver and UMB Provider
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 *
 * DOCUMENTATION:   Official specifications:
 *                  XMS v2.0: http://www.phatcode.net/res/219/files/xms20.txt
 *                  XMS v3.0: http://www.phatcode.net/res/219/files/xms30.txt
 *
 * About the implementation of UMBs in DOS:
 * ----------------------------------------
 * DOS needs a UMB provider to be able to use chunks of RAM in the C000-EFF0
 * memory region. A UMB provider detects regions of memory that do not contain
 * any ROMs or other system mapped area such as video RAM.
 *
 * Where can UMB providers be found?
 *
 * The XMS specification (see himem.c) provides three APIs to create, free, and
 * resize UMB blocks. As such, DOS performs calls into the XMS driver chain to
 * request UMB blocks and include them into the DOS memory arena.
 * However, is it only the HIMEM driver (implementing the XMS specification)
 * which can provide UMBs? It appears that this is not necessarily the case:
 * for example the MS HIMEM versions do not implement the UMB APIs; instead
 * it is the EMS driver (EMM386) which provides them, by hooking into the XMS
 * driver chain (see https://support.microsoft.com/en-us/kb/95555 : "MS-DOS 5.0
 * and later EMM386.EXE can also be configured to provide UMBs according to the
 * XMS. This causes EMM386.EXE to be a provider of the UMB portion of the XMS.").
 *
 * Some alternative replacements of HIMEM/EMM386 (for example in FreeDOS)
 * implement everything inside only one driver (XMS+EMS+UMB provider).
 * Finally there are some UMB providers that exist separately of XMS and EMS
 * drivers (for example, UMBPCI): they use hardware-specific tricks to discover
 * and provide UMBs.
 *
 * For more details, see:
 * http://www.freedos.org/technotes/technote/txt/169.txt
 * http://www.freedos.org/technotes/technote/txt/202.txt
 * http://www.uncreativelabs.net/textfiles/system/UMB.TXT
 *
 * This DOS XMS Driver provides the UMB APIs that are implemented on top
 * of the internal Upper Memory Area Manager, in umamgr.c
 */

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

#define NDEBUG
#include <debug.h>

#include "emulator.h"
#include "cpu/bop.h"
#include "../../memory.h"
#include "bios/umamgr.h"

#include "device.h"
#include "himem.h"

#define XMS_DEVICE_NAME "XMSXXXX0"

/* BOP Identifiers */
#define BOP_XMS 0x52

/* PRIVATE VARIABLES **********************************************************/

static const BYTE EntryProcedure[] =
{
    0xEB, // jmp short +0x03
    0x03,
    0x90, // nop
    0x90, // nop
    0x90, // nop
    LOBYTE(EMULATOR_BOP),
    HIBYTE(EMULATOR_BOP),
    BOP_XMS,
    0xCB // retf
};

static PDOS_DEVICE_NODE Node = NULL;
static XMS_HANDLE HandleTable[XMS_MAX_HANDLES];
static WORD FreeBlocks = XMS_BLOCKS;
static RTL_BITMAP AllocBitmap;
static ULONG BitmapBuffer[(XMS_BLOCKS + 31) / 32];

/*
 * This value is associated to HIMEM's "/HMAMIN=" switch. It indicates the
 * minimum account of space in the HMA a program can use, and is used in
 * conjunction with the "Request HMA" function.
 *
 * NOTE: The "/HMAMIN=" value is in kilo-bytes, whereas HmaMinSize is in bytes.
 *
 * Default value: 0. This causes the HMA to be allocated on a first come,
 * first served basis.
 */
static WORD HmaMinSize = 0;
/*
 * Flag used by "Request/Release HMA" functions, which indicates
 * whether the HMA was reserved or not.
 */
static BOOLEAN IsHmaReserved = FALSE;

/*
 * Flag used by "Global Enable/Disable A20" functions, so that they don't
 * need to re-change the state of A20 if it was already enabled/disabled.
 */
static BOOLEAN IsA20Enabled = FALSE;
/*
 * This flag is set to TRUE or FALSE when A20 line was already disabled or
 * enabled when XMS driver was loaded.
 * In case A20 was disabled, we are allowed to modify it. In case A20 was
 * already enabled, we are not allowed to touch it.
 */
static BOOLEAN CanChangeA20 = TRUE;
/*
 * Count for enabling or disabling the A20 line. The A20 line is enabled
 * only if the enabling count is greater than or equal to 0.
 */
static LONG A20EnableCount = 0;

/* A20 LINE HELPERS ***********************************************************/

static VOID XmsLocalEnableA20(VOID)
{
    /* Enable A20 only if we can do so, otherwise make the caller believe we enabled it */
    if (!CanChangeA20) goto Quit;

    /* The count is zero so enable A20 */
    if (A20EnableCount == 0) EmulatorSetA20(TRUE);

    ++A20EnableCount;

Quit:
    setAX(0x0001); /* Line successfully enabled */
    setBL(XMS_STATUS_SUCCESS);
    return;
}

static VOID XmsLocalDisableA20(VOID)
{
    UCHAR Result = XMS_STATUS_SUCCESS;

    /* Disable A20 only if we can do so, otherwise make the caller believe we disabled it */
    if (!CanChangeA20) goto Quit;

    /* If the count is already zero, fail */
    if (A20EnableCount == 0) goto Fail;

    --A20EnableCount;

    /* The count is zero so disable A20 */
    if (A20EnableCount == 0)
        EmulatorSetA20(FALSE); // Result = XMS_STATUS_SUCCESS;
    else
        Result = XMS_STATUS_A20_STILL_ENABLED;

Quit:
    setAX(0x0001); /* Line successfully disabled */
    setBL(Result);
    return;

Fail:
    setAX(0x0000); /* Line failed to be disabled */
    setBL(XMS_STATUS_A20_ERROR);
    return;
}

/* PRIVATE FUNCTIONS **********************************************************/

static inline PXMS_HANDLE GetXmsHandleRecord(WORD Handle)
{
    PXMS_HANDLE Entry;
    if (Handle == 0 || Handle >= XMS_MAX_HANDLES) return NULL;

    Entry = &HandleTable[Handle - 1];
    return Entry->Size ? Entry : NULL;
}

static inline BOOLEAN ValidateXmsHandle(PXMS_HANDLE HandleEntry)
{
    return (HandleEntry != NULL && HandleEntry->Handle != 0);
}

static WORD XmsGetLargestFreeBlock(VOID)
{
    WORD Result = 0;
    DWORD CurrentIndex = 0;
    ULONG RunStart;
    ULONG RunSize;

    while (CurrentIndex < XMS_BLOCKS)
    {
        RunSize = RtlFindNextForwardRunClear(&AllocBitmap, CurrentIndex, &RunStart);
        if (RunSize == 0) break;

        /* Update the maximum */
        if (RunSize > Result) Result = RunSize;

        /* Go to the next run */
        CurrentIndex = RunStart + RunSize;
    }

    return Result;
}

static UCHAR XmsAlloc(WORD Size, PWORD Handle)
{
    BYTE i;
    PXMS_HANDLE HandleEntry;
    DWORD CurrentIndex = 0;
    ULONG RunStart;
    ULONG RunSize;

    if (Size > FreeBlocks) return XMS_STATUS_OUT_OF_MEMORY;

    for (i = 0; i < XMS_MAX_HANDLES; i++)
    {
        HandleEntry = &HandleTable[i];
        if (HandleEntry->Handle == 0)
        {
            *Handle = i + 1;
            break;
        }
    }

    if (i == XMS_MAX_HANDLES) return XMS_STATUS_OUT_OF_HANDLES;

    /* Optimize blocks */
    for (i = 0; i < XMS_MAX_HANDLES; i++)
    {
        /* Skip free and locked blocks */
        if (HandleEntry->Handle == 0 || HandleEntry->LockCount > 0) continue;

        CurrentIndex = (HandleEntry->Address - XMS_ADDRESS) / XMS_BLOCK_SIZE;

        /* Check if there is any free space before this block */
        RunSize = RtlFindLastBackwardRunClear(&AllocBitmap, CurrentIndex, &RunStart);
        if (RunSize == 0) break;

        /* Move this block back */
        RtlMoveMemory((PVOID)REAL_TO_PHYS(HandleEntry->Address - RunSize * XMS_BLOCK_SIZE),
                      (PVOID)REAL_TO_PHYS(HandleEntry->Address),
                      RunSize * XMS_BLOCK_SIZE);

        /* Update the address */
        HandleEntry->Address -= RunSize * XMS_BLOCK_SIZE;
    }

    while (CurrentIndex < XMS_BLOCKS)
    {
        RunSize = RtlFindNextForwardRunClear(&AllocBitmap, CurrentIndex, &RunStart);
        if (RunSize == 0) break;

        if (RunSize >= HandleEntry->Size)
        {
            /* Allocate it here */
            HandleEntry->Handle = i + 1;
            HandleEntry->LockCount = 0;
            HandleEntry->Size = Size;
            HandleEntry->Address = XMS_ADDRESS + RunStart * XMS_BLOCK_SIZE;

            FreeBlocks -= Size;
            RtlSetBits(&AllocBitmap, RunStart, HandleEntry->Size);

            return XMS_STATUS_SUCCESS;
        }

        /* Keep searching */
        CurrentIndex = RunStart + RunSize;
    }

    return XMS_STATUS_OUT_OF_MEMORY;
}

static UCHAR XmsRealloc(WORD Handle, WORD NewSize)
{
    DWORD BlockNumber;
    PXMS_HANDLE HandleEntry = GetXmsHandleRecord(Handle);
    DWORD CurrentIndex = 0;
    ULONG RunStart;
    ULONG RunSize;

    if (!ValidateXmsHandle(HandleEntry))
        return XMS_STATUS_INVALID_HANDLE;

    if (HandleEntry->LockCount)
        return XMS_STATUS_LOCKED;

    /* Get the block number */
    BlockNumber = (HandleEntry->Address - XMS_ADDRESS) / XMS_BLOCK_SIZE;

    if (NewSize < HandleEntry->Size)
    {
        /* Just reduce the size of this block */
        RtlClearBits(&AllocBitmap, BlockNumber + NewSize, HandleEntry->Size - NewSize);
        FreeBlocks += HandleEntry->Size - NewSize;
        HandleEntry->Size = NewSize;
    }
    else if (NewSize > HandleEntry->Size)
    {
        /* Check if we can expand in-place */
        if (RtlAreBitsClear(&AllocBitmap,
                            BlockNumber + HandleEntry->Size,
                            NewSize - HandleEntry->Size))
        {
            /* Just increase the size of this block */
            RtlSetBits(&AllocBitmap,
                       BlockNumber + HandleEntry->Size,
                       NewSize - HandleEntry->Size);
            FreeBlocks -= NewSize - HandleEntry->Size;
            HandleEntry->Size = NewSize;

            /* We're done */
            return XMS_STATUS_SUCCESS;
        }

        /* Deallocate the current block range */
        RtlClearBits(&AllocBitmap, BlockNumber, HandleEntry->Size);

        /* Find a new place for this block */
        while (CurrentIndex < XMS_BLOCKS)
        {
            RunSize = RtlFindNextForwardRunClear(&AllocBitmap, CurrentIndex, &RunStart);
            if (RunSize == 0) break;

            if (RunSize >= NewSize)
            {
                /* Allocate the new range */
                RtlSetBits(&AllocBitmap, RunStart, NewSize);

                /* Move the data to the new location */
                RtlMoveMemory((PVOID)REAL_TO_PHYS(XMS_ADDRESS + RunStart * XMS_BLOCK_SIZE),
                              (PVOID)REAL_TO_PHYS(HandleEntry->Address),
                              HandleEntry->Size * XMS_BLOCK_SIZE);

                /* Update the handle entry */
                HandleEntry->Address = XMS_ADDRESS + RunStart * XMS_BLOCK_SIZE;
                HandleEntry->Size = NewSize;

                /* Update the free block counter */
                FreeBlocks -= NewSize - HandleEntry->Size;

                return XMS_STATUS_SUCCESS;
            }

            /* Keep searching */
            CurrentIndex = RunStart + RunSize;
        }

        /* Restore the old block range */
        RtlSetBits(&AllocBitmap, BlockNumber, HandleEntry->Size);
        return XMS_STATUS_OUT_OF_MEMORY;
    }

    return XMS_STATUS_SUCCESS;
}

static UCHAR XmsFree(WORD Handle)
{
    DWORD BlockNumber;
    PXMS_HANDLE HandleEntry = GetXmsHandleRecord(Handle);

    if (!ValidateXmsHandle(HandleEntry))
        return XMS_STATUS_INVALID_HANDLE;

    if (HandleEntry->LockCount)
        return XMS_STATUS_LOCKED;

    BlockNumber = (HandleEntry->Address - XMS_ADDRESS) / XMS_BLOCK_SIZE;
    RtlClearBits(&AllocBitmap, BlockNumber, HandleEntry->Size);

    HandleEntry->Handle = 0;
    FreeBlocks += HandleEntry->Size;

    return XMS_STATUS_SUCCESS;
}

static UCHAR XmsLock(WORD Handle, PDWORD Address)
{
    PXMS_HANDLE HandleEntry = GetXmsHandleRecord(Handle);

    if (!ValidateXmsHandle(HandleEntry))
        return XMS_STATUS_INVALID_HANDLE;

    if (HandleEntry->LockCount == 0xFF)
        return XMS_STATUS_LOCK_OVERFLOW;

    /* Increment the lock count */
    HandleEntry->LockCount++;
    *Address = HandleEntry->Address;

    return XMS_STATUS_SUCCESS;
}

static UCHAR XmsUnlock(WORD Handle)
{
    PXMS_HANDLE HandleEntry = GetXmsHandleRecord(Handle);

    if (!ValidateXmsHandle(HandleEntry))
        return XMS_STATUS_INVALID_HANDLE;

    if (!HandleEntry->LockCount)
        return XMS_STATUS_NOT_LOCKED;

    /* Decrement the lock count */
    HandleEntry->LockCount--;

    return XMS_STATUS_SUCCESS;
}

static VOID WINAPI XmsBopProcedure(LPWORD Stack)
{
    switch (getAH())
    {
        /* Get XMS Version */
        case 0x00:
        {
            setAX(0x0300); /*    XMS version 3.00 */
            setBX(0x0301); /* Driver version 3.01 */
            setDX(0x0001); /* HMA present */
            break;
        }

        /* Request HMA */
        case 0x01:
        {
            /* Check whether HMA is already reserved */
            if (IsHmaReserved)
            {
                /* It is, bail out */
                setAX(0x0000);
                setBL(XMS_STATUS_HMA_IN_USE);
                break;
            }

            // NOTE: We implicitely suppose that we always have HMA.
            // If not, we should fail there with the XMS_STATUS_HMA_DOES_NOT_EXIST
            // error code.

            /* Check whether the requested size is above the minimal allowed one */
            if (getDX() < HmaMinSize)
            {
                /* It is not, bail out */
                setAX(0x0000);
                setBL(XMS_STATUS_HMA_MIN_SIZE);
                break;
            }

            /* Reserve it */
            IsHmaReserved = TRUE;
            setAX(0x0001);
            setBL(XMS_STATUS_SUCCESS);
            break;
        }

        /* Release HMA */
        case 0x02:
        {
            /* Check whether HMA was reserved */
            if (!IsHmaReserved)
            {
                /* It was not, bail out */
                setAX(0x0000);
                setBL(XMS_STATUS_HMA_NOT_ALLOCATED);
                break;
            }

            /* Release it */
            IsHmaReserved = FALSE;
            setAX(0x0001);
            setBL(XMS_STATUS_SUCCESS);
            break;
        }

        /* Global Enable A20 */
        case 0x03:
        {
            /* Enable A20 if needed */
            if (!IsA20Enabled)
            {
                XmsLocalEnableA20();
                if (getAX() != 0x0001)
                {
                    /* XmsLocalEnableA20 failed and already set AX and BL to their correct values */
                    break;
                }

                IsA20Enabled = TRUE;
            }

            setAX(0x0001); /* Line successfully enabled */
            setBL(XMS_STATUS_SUCCESS);
            break;
        }

        /* Global Disable A20 */
        case 0x04:
        {
            UCHAR Result = XMS_STATUS_SUCCESS;

            /* Disable A20 if needed */
            if (IsA20Enabled)
            {
                XmsLocalDisableA20();
                if (getAX() != 0x0001)
                {
                    /* XmsLocalDisableA20 failed and already set AX and BL to their correct values */
                    break;
                }

                IsA20Enabled = FALSE;
                Result = getBL();
            }

            setAX(0x0001); /* Line successfully disabled */
            setBL(Result);
            break;
        }

        /* Local Enable A20 */
        case 0x05:
        {
            /* This call sets AX and BL to their correct values */
            XmsLocalEnableA20();
            break;
        }

        /* Local Disable A20 */
        case 0x06:
        {
            /* This call sets AX and BL to their correct values */
            XmsLocalDisableA20();
            break;
        }

        /* Query A20 State */
        case 0x07:
        {
            setAX(EmulatorGetA20());
            setBL(XMS_STATUS_SUCCESS);
            break;
        }

        /* Query Free Extended Memory */
        case 0x08:
        {
            setAX(XmsGetLargestFreeBlock());
            setDX(FreeBlocks);

            if (FreeBlocks > 0)
                setBL(XMS_STATUS_SUCCESS);
            else
                setBL(XMS_STATUS_OUT_OF_MEMORY);

            break;
        }

        /* Allocate Extended Memory Block */
        case 0x09:
        {
            WORD Handle;
            UCHAR Result = XmsAlloc(getDX(), &Handle);

            if (Result == XMS_STATUS_SUCCESS)
            {
                setAX(1);
                setDX(Handle);
            }
            else
            {
                setAX(0);
                setBL(Result);
            }

            break;
        }

        /* Free Extended Memory Block */
        case 0x0A:
        {
            UCHAR Result = XmsFree(getDX());

            setAX(Result == XMS_STATUS_SUCCESS);
            setBL(Result);
            break;
        }

        /* Move Extended Memory Block */
        case 0x0B:
        {
            PVOID SourceAddress, DestAddress;
            PXMS_COPY_DATA CopyData = (PXMS_COPY_DATA)SEG_OFF_TO_PTR(getDS(), getSI());
            PXMS_HANDLE HandleEntry;

            if (CopyData->SourceHandle)
            {
                HandleEntry = GetXmsHandleRecord(CopyData->SourceHandle);
                if (!ValidateXmsHandle(HandleEntry))
                {
                    setAX(0);
                    setBL(XMS_STATUS_BAD_SRC_HANDLE);
                    break;
                }

                if (CopyData->SourceOffset >= HandleEntry->Size * XMS_BLOCK_SIZE)
                {
                    setAX(0);
                    setBL(XMS_STATUS_BAD_SRC_OFFSET);
                }

                SourceAddress = (PVOID)REAL_TO_PHYS(HandleEntry->Address + CopyData->SourceOffset);
            }
            else
            {
                /* The offset is actually a 16-bit segment:offset pointer */
                SourceAddress = FAR_POINTER(CopyData->SourceOffset);
            }

            if (CopyData->DestHandle)
            {
                HandleEntry = GetXmsHandleRecord(CopyData->DestHandle);
                if (!ValidateXmsHandle(HandleEntry))
                {
                    setAX(0);
                    setBL(XMS_STATUS_BAD_DEST_HANDLE);
                    break;
                }

                if (CopyData->DestOffset >= HandleEntry->Size * XMS_BLOCK_SIZE)
                {
                    setAX(0);
                    setBL(XMS_STATUS_BAD_DEST_OFFSET);
                }

                DestAddress = (PVOID)REAL_TO_PHYS(HandleEntry->Address + CopyData->DestOffset);
            }
            else
            {
                /* The offset is actually a 16-bit segment:offset pointer */
                DestAddress = FAR_POINTER(CopyData->DestOffset);
            }

            /* Perform the move */
            RtlMoveMemory(DestAddress, SourceAddress, CopyData->Count);

            setAX(1);
            setBL(XMS_STATUS_SUCCESS);
            break;
        }

        /* Lock Extended Memory Block */
        case 0x0C:
        {
            DWORD Address;
            UCHAR Result = XmsLock(getDX(), &Address);

            if (Result == XMS_STATUS_SUCCESS)
            {
                setAX(1);

                /* Store the LINEAR address in DX:BX */
                setDX(HIWORD(Address));
                setBX(LOWORD(Address));
            }
            else
            {
                setAX(0);
                setBL(Result);
            }

            break;
        }

        /* Unlock Extended Memory Block */
        case 0x0D:
        {
            UCHAR Result = XmsUnlock(getDX());

            setAX(Result == XMS_STATUS_SUCCESS);
            setBL(Result);
            break;
        }

        /* Get Handle Information */
        case 0x0E:
        {
            PXMS_HANDLE HandleEntry = GetXmsHandleRecord(getDX());
            UINT i;
            UCHAR Handles = 0;

            if (!ValidateXmsHandle(HandleEntry))
            {
                setAX(0);
                setBL(XMS_STATUS_INVALID_HANDLE);
                break;
            }

            for (i = 0; i < XMS_MAX_HANDLES; i++)
            {
                if (HandleTable[i].Handle == 0) Handles++;
            }

            setAX(1);
            setBH(HandleEntry->LockCount);
            setBL(Handles);
            setDX(HandleEntry->Size);
            break;
        }

        /* Reallocate Extended Memory Block */
        case 0x0F:
        {
            UCHAR Result = XmsRealloc(getDX(), getBX());

            setAX(Result == XMS_STATUS_SUCCESS);
            setBL(Result);
            break;
        }

        /* Request UMB */
        case 0x10:
        {
            BOOLEAN Result;
            USHORT Segment = 0x0000; /* No preferred segment  */
            USHORT Size = getDX();   /* Size is in paragraphs */

            Result = UmaDescReserve(&Segment, &Size);
            if (Result)
                setBX(Segment);
            else
                setBL(Size > 0 ? XMS_STATUS_SMALLER_UMB : XMS_STATUS_OUT_OF_UMBS);

            setDX(Size);
            setAX(Result);
            break;
        }

        /* Release UMB */
        case 0x11:
        {
            BOOLEAN Result;
            USHORT Segment = getDX();

            Result = UmaDescRelease(Segment);
            if (!Result)
                setBL(XMS_STATUS_INVALID_UMB);

            setAX(Result);
            break;
        }

        /* Reallocate UMB */
        case 0x12:
        {
            BOOLEAN Result;
            USHORT Segment = getDX();
            USHORT Size = getBX(); /* Size is in paragraphs */

            Result = UmaDescReallocate(Segment, &Size);
            if (!Result)
            {
                if (Size > 0)
                {
                    setBL(XMS_STATUS_SMALLER_UMB);
                    setDX(Size);
                }
                else
                {
                    setBL(XMS_STATUS_INVALID_UMB);
                }
            }

            setAX(Result);
            break;
        }

        default:
        {
            DPRINT1("XMS command AH = 0x%02X NOT IMPLEMENTED\n", getAH());
            setBL(XMS_STATUS_NOT_IMPLEMENTED);
        }
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN XmsGetDriverEntry(PDWORD Pointer)
{
    if (Node == NULL) return FALSE;
    *Pointer = DEVICE_PRIVATE_AREA(Node->Driver);
    return TRUE;
}

VOID XmsInitialize(VOID)
{
    RtlZeroMemory(HandleTable, sizeof(HandleTable));
    RtlZeroMemory(BitmapBuffer, sizeof(BitmapBuffer));
    RtlInitializeBitMap(&AllocBitmap, BitmapBuffer, XMS_BLOCKS);

    Node = DosCreateDeviceEx(DOS_DEVATTR_IOCTL | DOS_DEVATTR_CHARACTER,
                             XMS_DEVICE_NAME,
                             sizeof(EntryProcedure));

    RegisterBop(BOP_XMS, XmsBopProcedure);

    /* Copy the entry routine to the device private area */
    RtlMoveMemory(FAR_POINTER(DEVICE_PRIVATE_AREA(Node->Driver)),
                  EntryProcedure,
                  sizeof(EntryProcedure));
}

VOID XmsCleanup(VOID)
{
    RegisterBop(BOP_XMS, NULL);
    DosDeleteDevice(Node);
}
