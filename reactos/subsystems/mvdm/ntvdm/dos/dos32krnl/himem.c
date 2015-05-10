/*
 * COPYRIGHT:       GPLv2+ - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            himem.c
 * PURPOSE:         DOS XMS Driver
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "emulator.h"
#include "cpu/bop.h"
#include "io.h"
#include "hardware/ps2.h"

#include "dos.h"
#include "dos/dem.h"
#include "device.h"
#include "himem.h"

#define XMS_DEVICE_NAME "XMSXXXX0"

/* BOP Identifiers */
#define BOP_XMS 0x52

ULONG
NTAPI
RtlFindLastBackwardRunClear
(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG FromIndex,
    OUT PULONG StartingRunIndex
);

/* PRIVATE VARIABLES **********************************************************/

static const BYTE EntryProcedure[] = {
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
 * Flag used by Global Enable/Disable A20 functions, so that they don't
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

/* HELPERS FOR A20 LINE *******************************************************/

static BOOLEAN PCAT_A20Control(BYTE Control, PBOOLEAN A20Status)
{
    BYTE ControllerOutput;

    /* Retrieve PS/2 controller output byte */
    IOWriteB(PS2_CONTROL_PORT, 0xD0);
    ControllerOutput = IOReadB(PS2_DATA_PORT);

    switch (Control)
    {
        case 0: /* Disable A20 line */
            ControllerOutput &= ~0x02;
            IOWriteB(PS2_CONTROL_PORT, 0xD1);
            IOWriteB(PS2_DATA_PORT, ControllerOutput);
            break;

        case 1: /* Enable A20 line */
            ControllerOutput |= 0x02;
            IOWriteB(PS2_CONTROL_PORT, 0xD1);
            IOWriteB(PS2_DATA_PORT, ControllerOutput);
            break;

        default: /* Get A20 status */
            break;
    }

    if (A20Status)
        *A20Status = (ControllerOutput & 0x02) != 0;

    /* Return success */
    return TRUE;
}

static VOID XmsLocalEnableA20(VOID)
{
    /* Enable A20 only if we can do so, otherwise make the caller believe we enabled it */
    if (!CanChangeA20) goto Quit;

    /* The count is zero so enable A20 */
    if (A20EnableCount == 0 && !PCAT_A20Control(1, NULL))
        goto Fail;

    ++A20EnableCount;

Quit:
    setAX(0x0001); /* Line successfully enabled */
    setBL(XMS_STATUS_SUCCESS);
    return;

Fail:
    setAX(0x0000); /* Line failed to be enabled */
    setBL(XMS_STATUS_A20_ERROR);
    return;
}

static VOID XmsLocalDisableA20(VOID)
{
    /* Disable A20 only if we can do so, otherwise make the caller believe we disabled it */
    if (!CanChangeA20) goto Quit;

    /* If the count is already zero, fail */
    if (A20EnableCount == 0) goto Fail;

    --A20EnableCount;

    /* The count is zero so disable A20 */
    if (A20EnableCount == 0 && !PCAT_A20Control(0, NULL))
        goto Fail;

Quit:
    setAX(0x0001); /* Line successfully disabled */
    setBL(XMS_STATUS_SUCCESS);
    return;

Fail:
    setAX(0x0000); /* Line failed to be enabled */
    setBL(XMS_STATUS_A20_ERROR);
    return;
}

static VOID XmsGetA20State(VOID)
{
    BOOLEAN A20Status = FALSE;

    /*
     * NOTE: The XMS specification explicitely says that this check is done
     * in a hardware-independent manner, by checking whether high memory wraps.
     * For our purposes we just call the emulator API.
     */

    /* Get A20 status */
    if (PCAT_A20Control(2, &A20Status))
        setBL(XMS_STATUS_SUCCESS);
    else
        setBL(XMS_STATUS_A20_ERROR);

    setAX(A20Status);
}

/* PRIVATE FUNCTIONS **********************************************************/

static inline PXMS_HANDLE GetHandleRecord(WORD Handle)
{
    PXMS_HANDLE Entry;
    if (Handle == 0 || Handle >= XMS_MAX_HANDLES) return NULL;

    Entry = &HandleTable[Handle - 1];
    return Entry->Size ? Entry : NULL;
}

static CHAR XmsAlloc(WORD Size, PWORD Handle)
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

static CHAR XmsFree(WORD Handle)
{
    DWORD BlockNumber;
    PXMS_HANDLE HandleEntry = GetHandleRecord(Handle);

    if (HandleEntry == NULL) return XMS_STATUS_INVALID_HANDLE;
    if (HandleEntry->LockCount) return XMS_STATUS_LOCKED;

    BlockNumber = (HandleEntry->Address - XMS_ADDRESS) / XMS_BLOCK_SIZE;
    RtlClearBits(&AllocBitmap, BlockNumber, HandleEntry->Size);

    HandleEntry->Handle = 0;
    FreeBlocks += HandleEntry->Size;

    return XMS_STATUS_SUCCESS;
}

static CHAR XmsLock(WORD Handle, PDWORD Address)
{
    PXMS_HANDLE HandleEntry = GetHandleRecord(Handle);

    if (HandleEntry == NULL) return XMS_STATUS_INVALID_HANDLE;
    if (HandleEntry->LockCount == 0xFF) return XMS_STATUS_LOCK_OVERFLOW;

    /* Increment the lock count */
    HandleEntry->LockCount++;
    *Address = HandleEntry->Address;

    return XMS_STATUS_SUCCESS;
}

static CHAR XmsUnlock(WORD Handle)
{
    PXMS_HANDLE HandleEntry = GetHandleRecord(Handle);

    if (HandleEntry == NULL) return XMS_STATUS_INVALID_HANDLE;
    if (!HandleEntry->LockCount) return XMS_STATUS_NOT_LOCKED;

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

        /* Global Enable A20 */
        case 0x03:
        {
            /* Enable A20 if needed */
            if (!IsA20Enabled)
            {
                XmsLocalEnableA20();
                if (getAX() != 1)
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
            /* Disable A20 if needed */
            if (IsA20Enabled)
            {
                XmsLocalDisableA20();
                if (getAX() != 1)
                {
                    /* XmsLocalDisableA20 failed and already set AX and BL to their correct values */
                    break;
                }

                IsA20Enabled = FALSE;
            }

            setAX(0x0001); /* Line successfully disabled */
            setBL(XMS_STATUS_SUCCESS);
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
            /* This call sets AX and BL to their correct values */
            XmsGetA20State();
            break;
        }

        /* Query Free Extended Memory */
        case 0x08:
        {
            setAX(FreeBlocks);
            setDX(XMS_BLOCKS);
            setBL(XMS_STATUS_SUCCESS);
            break;
        }

        /* Allocate Extended Memory Block */
        case 0x09:
        {
            WORD Handle;
            CHAR Result = XmsAlloc(getDX(), &Handle);

            if (Result >= 0)
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
            CHAR Result = XmsFree(getDX());

            setAX(Result >= 0);
            setBL(Result);

            break;
        }

        /* Move Extended Memory Block */
        case 0x0B:
        {
            PVOID SourceAddress, DestAddress;
            PXMS_COPY_DATA CopyData = (PXMS_COPY_DATA)SEG_OFF_TO_PTR(getDS(), getSI());

            if (CopyData->SourceHandle)
            {
                PXMS_HANDLE Entry = GetHandleRecord(CopyData->SourceHandle);
                if (!Entry)
                {
                    setAX(0);
                    setBL(XMS_STATUS_BAD_SRC_HANDLE);
                    break;
                }

                if (CopyData->SourceOffset >= Entry->Size * XMS_BLOCK_SIZE)
                {
                    setAX(0);
                    setBL(XMS_STATUS_BAD_SRC_OFFSET);
                }

                SourceAddress = (PVOID)REAL_TO_PHYS(Entry->Address + CopyData->SourceOffset);
            }
            else
            {
                /* The offset is actually a 16-bit segment:offset pointer */
                SourceAddress = SEG_OFF_TO_PTR(HIWORD(CopyData->SourceOffset),
                                               LOWORD(CopyData->SourceOffset));
            }

            if (CopyData->DestHandle)
            {
                PXMS_HANDLE Entry = GetHandleRecord(CopyData->DestHandle);
                if (!Entry)
                {
                    setAX(0);
                    setBL(XMS_STATUS_BAD_DEST_HANDLE);
                    break;
                }

                if (CopyData->DestOffset >= Entry->Size * XMS_BLOCK_SIZE)
                {
                    setAX(0);
                    setBL(XMS_STATUS_BAD_DEST_OFFSET);
                }

                DestAddress = (PVOID)REAL_TO_PHYS(Entry->Address + CopyData->DestOffset);
            }
            else
            {
                /* The offset is actually a 16-bit segment:offset pointer */
                DestAddress = SEG_OFF_TO_PTR(HIWORD(CopyData->DestOffset),
                                             LOWORD(CopyData->DestOffset));
            }

            setAX(1);
            setBL(XMS_STATUS_SUCCESS);
            RtlMoveMemory(DestAddress, SourceAddress, CopyData->Count);
            break;
        }

        /* Lock Extended Memory Block */
        case 0x0C:
        {
            DWORD Address;
            CHAR Result = XmsLock(getDX(), &Address);

            if (Result >= 0)
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
            CHAR Result = XmsUnlock(getDX());

            setAX(Result >= 0);
            setBL(Result);

            break;
        }

        /* Get Handle Information */
        case 0x0E:
        {
            PXMS_HANDLE Entry = GetHandleRecord(getDX());

            if (Entry)
            {
                INT i;
                UCHAR Handles = 0;

                for (i = 0; i < XMS_MAX_HANDLES; i++)
                {
                    if (HandleTable[i].Handle == 0) Handles++;
                }

                setAX(1);
                setBH(Entry->LockCount);
                setBL(Handles);
                setDX(Entry->Size);
            }
            else
            {
                setAX(0);
                setBL(XMS_STATUS_INVALID_HANDLE);
            }

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
