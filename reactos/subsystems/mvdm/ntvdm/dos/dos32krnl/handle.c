/*
 * COPYRIGHT:       GPLv2+ - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            dos/dos32krnl/handle.c
 * PURPOSE:         DOS32 Handles (Job File Table)
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "emulator.h"

#include "dos.h"
#include "dos/dem.h"
#include "dosfiles.h"
#include "handle.h"
#include "memory.h"
#include "process.h"

/* PRIVATE FUNCTIONS **********************************************************/

/* Taken from base/shell/cmd/console.c */
static BOOL IsConsoleHandle(HANDLE hHandle)
{
    DWORD dwMode;

    /* Check whether the handle may be that of a console... */
    if ((GetFileType(hHandle) & FILE_TYPE_CHAR) == 0) return FALSE;

    /*
     * It may be. Perform another test... The idea comes from the
     * MSDN description of the WriteConsole API:
     *
     * "WriteConsole fails if it is used with a standard handle
     *  that is redirected to a file. If an application processes
     *  multilingual output that can be redirected, determine whether
     *  the output handle is a console handle (one method is to call
     *  the GetConsoleMode function and check whether it succeeds).
     *  If the handle is a console handle, call WriteConsole. If the
     *  handle is not a console handle, the output is redirected and
     *  you should call WriteFile to perform the I/O."
     */
    return GetConsoleMode(hHandle, &dwMode);
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID DosCopyHandleTable(LPBYTE DestinationTable)
{
    UINT i;
    PDOS_PSP PspBlock;
    LPBYTE SourceTable;
    PDOS_FILE_DESCRIPTOR Descriptor;

    /* Clear the table first */
    for (i = 0; i < DEFAULT_JFT_SIZE; i++) DestinationTable[i] = 0xFF;

    /* Check if this is the initial process */
    if (Sda->CurrentPsp == SYSTEM_PSP)
    {
        BYTE DescriptorId;
        HANDLE StandardHandles[3];

        /* Get the native standard handles */
        StandardHandles[0] = GetStdHandle(STD_INPUT_HANDLE);
        StandardHandles[1] = GetStdHandle(STD_OUTPUT_HANDLE);
        StandardHandles[2] = GetStdHandle(STD_ERROR_HANDLE);

        for (i = 0; i < 3; i++)
        {
            /* Find the corresponding SFT entry */
            if (IsConsoleHandle(StandardHandles[i]))
            {
                DescriptorId = DosFindDeviceDescriptor(SysVars->ActiveCon);
            }
            else
            {
                DescriptorId = DosFindWin32Descriptor(StandardHandles[i]);
            }

            if (DescriptorId != 0xFF)
            {
                Descriptor = DosGetFileDescriptor(DescriptorId);
            }
            else
            {
                /* Create a new SFT entry for it */
                DescriptorId = DosFindFreeDescriptor();
                if (DescriptorId == 0xFF)
                {
                    DPRINT1("Cannot create standard handle %d, the SFT is full!\n", i);
                    continue;
                }

                Descriptor = DosGetFileDescriptor(DescriptorId);
                ASSERT(Descriptor != NULL);
                RtlZeroMemory(Descriptor, sizeof(*Descriptor));

                if (IsConsoleHandle(StandardHandles[i]))
                {
                    PDOS_DEVICE_NODE Node = DosGetDriverNode(SysVars->ActiveCon);

                    Descriptor->DeviceInfo = Node->DeviceAttributes | FILE_INFO_DEVICE;
                    Descriptor->DevicePointer = SysVars->ActiveCon;
                    RtlFillMemory(Descriptor->FileName, sizeof(Descriptor->FileName), ' ');
                    RtlCopyMemory(Descriptor->FileName, Node->Name.Buffer, Node->Name.Length);

                    /* Call the open routine */
                    if (Node->OpenRoutine) Node->OpenRoutine(Node);
                }
                else
                {
                    Descriptor->Win32Handle = StandardHandles[i];
                }
            }

            Descriptor->RefCount++;
            DestinationTable[i] = DescriptorId;
        }
    }
    else
    {
        /* Get the parent PSP block and handle table */
        PspBlock = SEGMENT_TO_PSP(Sda->CurrentPsp);
        SourceTable = (LPBYTE)FAR_POINTER(PspBlock->HandleTablePtr);

        /* Copy the first 20 handles into the new table */
        for (i = 0; i < DEFAULT_JFT_SIZE; i++)
        {
            Descriptor = DosGetFileDescriptor(SourceTable[i]);
            DestinationTable[i] = SourceTable[i];

            /* Increase the reference count */
            Descriptor->RefCount++;
        }
    }
}

BOOLEAN DosResizeHandleTable(WORD NewSize)
{
    PDOS_PSP PspBlock;
    LPBYTE HandleTable;
    WORD Segment;

    /* Get the PSP block */
    PspBlock = SEGMENT_TO_PSP(Sda->CurrentPsp);

    if (NewSize == PspBlock->HandleTableSize)
    {
        /* No change */
        return TRUE;
    }

    if (PspBlock->HandleTableSize > DEFAULT_JFT_SIZE)
    {
        /* Get the segment of the current table */
        Segment = (LOWORD(PspBlock->HandleTablePtr) >> 4) + HIWORD(PspBlock->HandleTablePtr);

        if (NewSize <= DEFAULT_JFT_SIZE)
        {
            /* Get the current handle table */
            HandleTable = FAR_POINTER(PspBlock->HandleTablePtr);

            /* Copy it to the PSP */
            RtlCopyMemory(PspBlock->HandleTable, HandleTable, NewSize);

            /* Free the memory */
            DosFreeMemory(Segment);

            /* Update the handle table pointer and size */
            PspBlock->HandleTableSize = NewSize;
            PspBlock->HandleTablePtr = MAKELONG(0x18, Sda->CurrentPsp);
        }
        else
        {
            /* Resize the memory */
            if (!DosResizeMemory(Segment, NewSize, NULL))
            {
                /* Unable to resize, try allocating it somewhere else */
                Segment = DosAllocateMemory(NewSize, NULL);
                if (Segment == 0) return FALSE;

                /* Get the new handle table */
                HandleTable = SEG_OFF_TO_PTR(Segment, 0);

                /* Copy the handles to the new table */
                RtlCopyMemory(HandleTable,
                              FAR_POINTER(PspBlock->HandleTablePtr),
                              PspBlock->HandleTableSize);

                /* Update the handle table pointer */
                PspBlock->HandleTablePtr = MAKELONG(0, Segment);
            }

            /* Update the handle table size */
            PspBlock->HandleTableSize = NewSize;
        }
    }
    else if (NewSize > DEFAULT_JFT_SIZE)
    {
        Segment = DosAllocateMemory(NewSize, NULL);
        if (Segment == 0) return FALSE;

        /* Get the new handle table */
        HandleTable = SEG_OFF_TO_PTR(Segment, 0);

        /* Copy the handles from the PSP to the new table */
        RtlCopyMemory(HandleTable,
                      FAR_POINTER(PspBlock->HandleTablePtr),
                      PspBlock->HandleTableSize);

        /* Update the handle table pointer and size */
        PspBlock->HandleTableSize = NewSize;
        PspBlock->HandleTablePtr = MAKELONG(0, Segment);
    }

    return TRUE;
}


WORD DosOpenHandle(BYTE DescriptorId)
{
    WORD DosHandle;
    PDOS_PSP PspBlock;
    LPBYTE HandleTable;
    PDOS_FILE_DESCRIPTOR Descriptor = DosGetFileDescriptor(DescriptorId);

    DPRINT("DosOpenHandle: DescriptorId 0x%02X\n", DescriptorId);

    /* Make sure the descriptor ID is valid */
    if (Descriptor == NULL) return INVALID_DOS_HANDLE;

    /* The system PSP has no handle table */
    if (Sda->CurrentPsp == SYSTEM_PSP) return INVALID_DOS_HANDLE;

    /* Get a pointer to the handle table */
    PspBlock = SEGMENT_TO_PSP(Sda->CurrentPsp);
    HandleTable = (LPBYTE)FAR_POINTER(PspBlock->HandleTablePtr);

    /* Find a free entry in the JFT */
    for (DosHandle = 0; DosHandle < PspBlock->HandleTableSize; DosHandle++)
    {
        if (HandleTable[DosHandle] == 0xFF) break;
    }

    /* If there are no free entries, fail */
    if (DosHandle == PspBlock->HandleTableSize) return INVALID_DOS_HANDLE;

    /* Reference the descriptor */
    Descriptor->RefCount++;

    /* Set the JFT entry to that descriptor ID */
    HandleTable[DosHandle] = DescriptorId;

    /* Return the new handle */
    return DosHandle;
}

BYTE DosQueryHandle(WORD DosHandle)
{
    PDOS_PSP PspBlock;
    LPBYTE HandleTable;

    DPRINT("DosQueryHandle: DosHandle 0x%04X\n", DosHandle);

    /* The system PSP has no handle table */
    if (Sda->CurrentPsp == SYSTEM_PSP) return 0xFF;

    /* Get a pointer to the handle table */
    PspBlock = SEGMENT_TO_PSP(Sda->CurrentPsp);
    HandleTable = (LPBYTE)FAR_POINTER(PspBlock->HandleTablePtr);

    /* Return the descriptor ID */
    return HandleTable[DosHandle];
}

WORD DosDuplicateHandle(WORD DosHandle)
{
    BYTE DescriptorId = DosQueryHandle(DosHandle);

    if (DescriptorId == 0xFF)
    {
        Sda->LastErrorCode = ERROR_INVALID_HANDLE;
        return INVALID_DOS_HANDLE;
    }

    return DosOpenHandle(DescriptorId);
}

BOOLEAN DosForceDuplicateHandle(WORD OldHandle, WORD NewHandle)
{
    BYTE DescriptorId;
    PDOS_PSP PspBlock;
    LPBYTE HandleTable;
    PDOS_FILE_DESCRIPTOR Descriptor;

    DPRINT("DosForceDuplicateHandle: OldHandle 0x%04X, NewHandle 0x%04X\n",
           OldHandle,
           NewHandle);

    /* The system PSP has no handle table */
    if (Sda->CurrentPsp == SYSTEM_PSP) return FALSE;

    /* Get a pointer to the handle table */
    PspBlock = SEGMENT_TO_PSP(Sda->CurrentPsp);
    HandleTable = (LPBYTE)FAR_POINTER(PspBlock->HandleTablePtr);

    /* Make sure the old handle is open */
    if (HandleTable[OldHandle] == 0xFF) return FALSE;

    /* Check if the new handle is open */
    if (HandleTable[NewHandle] != 0xFF)
    {
        /* Close it */
        DosCloseHandle(NewHandle);
    }

    DescriptorId = HandleTable[OldHandle];
    Descriptor = DosGetFileDescriptor(DescriptorId);
    if (Descriptor == NULL) return FALSE;

    /* Increment the reference count of the descriptor */
    Descriptor->RefCount++;

    /* Make the new handle point to that descriptor */
    HandleTable[NewHandle] = DescriptorId;

    /* Return success */
    return TRUE;
}

BOOLEAN DosCloseHandle(WORD DosHandle)
{
    PDOS_PSP PspBlock;
    LPBYTE HandleTable;
    PDOS_FILE_DESCRIPTOR Descriptor;

    DPRINT("DosCloseHandle: DosHandle 0x%04X\n", DosHandle);

    /* The system PSP has no handle table */
    if (Sda->CurrentPsp == SYSTEM_PSP) return FALSE;

    /* Get a pointer to the handle table */
    PspBlock = SEGMENT_TO_PSP(Sda->CurrentPsp);
    HandleTable = (LPBYTE)FAR_POINTER(PspBlock->HandleTablePtr);

    /* Make sure the handle is open */
    if (HandleTable[DosHandle] == 0xFF) return FALSE;

    /* Make sure the descriptor is valid */
    Descriptor = DosGetFileDescriptor(HandleTable[DosHandle]);
    if (Descriptor == NULL) return FALSE;

    /* Decrement the reference count of the descriptor */
    Descriptor->RefCount--;

    /* Check if the reference count fell to zero */
    if (!Descriptor->RefCount)
    {
        if (Descriptor->DeviceInfo & (1 << 7))
        {
            PDOS_DEVICE_NODE Node = DosGetDriverNode(Descriptor->DevicePointer);

            /* Call the close routine, if it exists */
            if (Node->CloseRoutine) Node->CloseRoutine(Node);
        }
        else
        {
            /* Close the win32 handle */
            CloseHandle(Descriptor->Win32Handle);
        }
    }

    /* Clear the entry in the JFT */
    HandleTable[DosHandle] = 0xFF;

    return TRUE;
}
