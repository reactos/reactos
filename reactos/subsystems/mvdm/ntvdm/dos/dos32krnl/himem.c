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

#include "dos.h"
#include "dos/dem.h"
#include "device.h"
#include "himem.h"

#define XMS_DEVICE_NAME "XMSXXXX0"

/* BOP Identifiers */
#define BOP_XMS 0x52

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

    HandleEntry->Handle = i + 1;
    HandleEntry->LockCount = 0;
    HandleEntry->Size = Size;
    FreeBlocks -= Size;

    return XMS_STATUS_SUCCESS;
}

static CHAR XmsFree(WORD Handle)
{
    PXMS_HANDLE HandleEntry = GetHandleRecord(Handle);
    if (HandleEntry == NULL) return XMS_STATUS_INVALID_HANDLE;
    if (HandleEntry->LockCount) return XMS_STATUS_LOCKED;

    HandleEntry->Handle = 0;
    FreeBlocks += HandleEntry->Size;

    return XMS_STATUS_SUCCESS;
}

static CHAR XmsLock(WORD Handle, PDWORD Address)
{
    DWORD CurrentIndex = 0;
    PXMS_HANDLE HandleEntry = GetHandleRecord(Handle);

    if (HandleEntry == NULL) return XMS_STATUS_INVALID_HANDLE;
    if (HandleEntry->LockCount == 0xFF) return XMS_STATUS_LOCK_OVERFLOW;

    if (HandleEntry->LockCount)
    {
        /* Just increment the lock count */
        HandleEntry->LockCount++;
        return XMS_STATUS_SUCCESS;
    }

    while (CurrentIndex < XMS_BLOCKS)
    {
        ULONG RunStart;
        ULONG RunSize = RtlFindNextForwardRunClear(&AllocBitmap, CurrentIndex, &RunStart);
        if (RunSize == 0) break;

        if (RunSize >= HandleEntry->Size)
        {
            /* Lock it here */
            HandleEntry->LockCount++;
            HandleEntry->Address = XMS_ADDRESS + RunStart * XMS_BLOCK_SIZE;

            RtlSetBits(&AllocBitmap, RunStart, RunSize);
            *Address = HandleEntry->Address;
            return XMS_STATUS_SUCCESS;
        }

        /* Keep searching */
        CurrentIndex = RunStart + RunSize;
    }

    /* Can't find any suitable range */
    return XMS_STATUS_CANNOT_LOCK;
}

static CHAR XmsUnlock(WORD Handle)
{
    DWORD BlockNumber;
    PXMS_HANDLE HandleEntry = GetHandleRecord(Handle);

    if (HandleEntry == NULL) return XMS_STATUS_INVALID_HANDLE;
    if (!HandleEntry->LockCount) return XMS_STATUS_NOT_LOCKED;

    /* Decrement the lock count and exit early if it's still locked */
    if (--HandleEntry->LockCount) return XMS_STATUS_SUCCESS;

    BlockNumber = (HandleEntry->Address - XMS_ADDRESS) / XMS_BLOCK_SIZE;
    RtlClearBits(&AllocBitmap, BlockNumber, HandleEntry->Size);

    return XMS_STATUS_SUCCESS;
}

static VOID WINAPI XmsBopProcedure(LPWORD Stack)
{
    switch (getAH())
    {
        /* Get XMS Version */
        case 0x00:
        {
            setAX(0x0300); /* XMS version 3.0 */
            setDX(0x0001); /* HMA present */

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
