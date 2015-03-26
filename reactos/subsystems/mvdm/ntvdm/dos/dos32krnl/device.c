/*
 * COPYRIGHT:       GPLv2+ - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            device.c
 * PURPOSE:         DOS Device Support
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"
#include "dos.h"
#include "dos/dem.h"
#include "memory.h"
#include "device.h"

/* PRIVATE VARIABLES **********************************************************/

static LIST_ENTRY DeviceList = { &DeviceList, &DeviceList };

/* PRIVATE FUNCTIONS **********************************************************/

static VOID DosCallDriver(DWORD Driver, PDOS_REQUEST_HEADER Request)
{
    PDOS_DRIVER DriverBlock = (PDOS_DRIVER)FAR_POINTER(Driver);
    PDOS_REQUEST_HEADER RemoteRequest;

    /* Call the strategy routine first */
    Call16(HIWORD(Driver), DriverBlock->StrategyRoutine);
    RemoteRequest = (PDOS_REQUEST_HEADER)SEG_OFF_TO_PTR(getES(), getBX());

    /* Copy the request structure to ES:BX */
    RtlMoveMemory(RemoteRequest, Request, Request->RequestLength);

    /* Call the interrupt routine */
    Call16(HIWORD(Driver), DriverBlock->InterruptRoutine);

    /* Get the request structure from ES:BX */
    RtlMoveMemory(Request, RemoteRequest, RemoteRequest->RequestLength);
}

static inline WORD NTAPI DosDriverReadInternal(PDOS_DEVICE_NODE DeviceNode,
                                               DWORD Buffer,
                                               PWORD Length,
                                               BOOLEAN IoControl)
{
    DOS_RW_REQUEST Request;

    Request.Header.RequestLength = IoControl ? sizeof(DOS_IOCTL_RW_REQUEST)
                                             : sizeof(DOS_RW_REQUEST);
    Request.Header.CommandCode = IoControl ? DOS_DEVCMD_IOCTL_READ : DOS_DEVCMD_READ;
    Request.BufferPointer = Buffer;
    Request.Length = *Length;

    DosCallDriver(DeviceNode->Driver, &Request.Header);

    *Length = Request.Length;
    return Request.Header.Status;
}

static inline WORD NTAPI DosDriverWriteInternal(PDOS_DEVICE_NODE DeviceNode,
                                                DWORD Buffer,
                                                PWORD Length,
                                                BOOLEAN IoControl)
{
    DOS_RW_REQUEST Request;

    Request.Header.RequestLength = IoControl ? sizeof(DOS_IOCTL_RW_REQUEST)
                                             : sizeof(DOS_RW_REQUEST);
    Request.Header.CommandCode = IoControl ? DOS_DEVCMD_IOCTL_WRITE : DOS_DEVCMD_WRITE;
    Request.BufferPointer = Buffer;
    Request.Length = *Length;

    DosCallDriver(DeviceNode->Driver, &Request.Header);

    *Length = Request.Length;
    return Request.Header.Status;
}

static inline WORD NTAPI DosDriverGenericRequest(PDOS_DEVICE_NODE DeviceNode,
                                                 BYTE CommandCode)
{
    DOS_REQUEST_HEADER Request;

    Request.RequestLength = sizeof(DOS_REQUEST_HEADER);
    Request.CommandCode = CommandCode;

    DosCallDriver(DeviceNode->Driver, &Request);

    return Request.Status;
}

static WORD NTAPI DosDriverDispatchIoctlRead(PDOS_DEVICE_NODE DeviceNode,
                                             DWORD Buffer,
                                             PWORD Length)
{
    return DosDriverReadInternal(DeviceNode, Buffer, Length, TRUE);
}

static WORD NTAPI DosDriverDispatchRead(PDOS_DEVICE_NODE DeviceNode,
                                        DWORD Buffer,
                                        PWORD Length)
{
    return DosDriverReadInternal(DeviceNode, Buffer, Length, FALSE);
}

static WORD NTAPI DosDriverDispatchPeek(PDOS_DEVICE_NODE DeviceNode,
                                        PBYTE Character)
{
    DOS_PEEK_REQUEST Request;

    Request.Header.RequestLength = sizeof(DOS_PEEK_REQUEST);
    Request.Header.CommandCode = DOS_DEVCMD_PEEK;

    DosCallDriver(DeviceNode->Driver, &Request.Header);

    *Character = Request.Character;
    return Request.Header.Status;
}

static WORD NTAPI DosDriverDispatchInputStatus(PDOS_DEVICE_NODE DeviceNode)
{
    return DosDriverGenericRequest(DeviceNode, DOS_DEVCMD_INSTAT);
}

static WORD NTAPI DosDriverDispatchFlushInput(PDOS_DEVICE_NODE DeviceNode)
{
    return DosDriverGenericRequest(DeviceNode, DOS_DEVCMD_FLUSH_INPUT);
}

static WORD NTAPI DosDriverDispatchIoctlWrite(PDOS_DEVICE_NODE DeviceNode,
                                              DWORD Buffer,
                                              PWORD Length)
{
    return DosDriverWriteInternal(DeviceNode, Buffer, Length, TRUE);
}

static WORD NTAPI DosDriverDispatchWrite(PDOS_DEVICE_NODE DeviceNode,
                                         DWORD Buffer,
                                         PWORD Length)
{
    return DosDriverWriteInternal(DeviceNode, Buffer, Length, FALSE);
}

static WORD NTAPI DosDriverDispatchOutputStatus(PDOS_DEVICE_NODE DeviceNode)
{
    return DosDriverGenericRequest(DeviceNode, DOS_DEVCMD_OUTSTAT);
}

static WORD NTAPI DosDriverDispatchFlushOutput(PDOS_DEVICE_NODE DeviceNode)
{
    return DosDriverGenericRequest(DeviceNode, DOS_DEVCMD_FLUSH_OUTPUT);
}

static WORD NTAPI DosDriverDispatchOpen(PDOS_DEVICE_NODE DeviceNode)
{
    return DosDriverGenericRequest(DeviceNode, DOS_DEVCMD_OPEN);
}

static WORD NTAPI DosDriverDispatchClose(PDOS_DEVICE_NODE DeviceNode)
{
    return DosDriverGenericRequest(DeviceNode, DOS_DEVCMD_CLOSE);
}

static WORD NTAPI DosDriverDispatchOutputUntilBusy(PDOS_DEVICE_NODE DeviceNode,
                                                   DWORD Buffer,
                                                   PWORD Length)
{
    DOS_OUTPUT_BUSY_REQUEST Request;

    Request.Header.RequestLength = sizeof(DOS_OUTPUT_BUSY_REQUEST);
    Request.Header.CommandCode = DOS_DEVCMD_OUTPUT_BUSY;
    Request.BufferPointer = Buffer;
    Request.Length = *Length;

    DosCallDriver(DeviceNode->Driver, &Request.Header);

    *Length = Request.Length;
    return Request.Header.Status;
}

/* PUBLIC FUNCTIONS ***********************************************************/

PDOS_DEVICE_NODE DosGetDevice(LPCSTR DeviceName)
{
    PLIST_ENTRY i;
    PDOS_DEVICE_NODE Node;
    ANSI_STRING DeviceNameString;

    RtlInitAnsiString(&DeviceNameString, DeviceName);

    for (i = DeviceList.Flink; i != &DeviceList; i = i->Flink)
    {
        Node = CONTAINING_RECORD(i, DOS_DEVICE_NODE, Entry);
        if (RtlEqualString(&Node->Name, &DeviceNameString, TRUE)) return Node;
    }

    return NULL;
}

PDOS_DEVICE_NODE DosCreateDevice(WORD Attributes, PCHAR DeviceName)
{
    BYTE i;
    PDOS_DEVICE_NODE Node;

    /* Make sure this is a character device */
    if (!(Attributes & DOS_DEVATTR_CHARACTER))
    {
        DPRINT1("ERROR: Block devices are not supported.\n");
        return FALSE;
    }

    Node = (PDOS_DEVICE_NODE)RtlAllocateHeap(RtlGetProcessHeap(),
                                             HEAP_ZERO_MEMORY,
                                             sizeof(DOS_DEVICE_NODE));
    if (Node == NULL) return NULL;

    Node->DeviceAttributes = Attributes;

    /* Initialize the name string */
    Node->Name.Buffer = Node->NameBuffer;
    Node->Name.MaximumLength = MAX_DEVICE_NAME;

    for (i = 0; i < MAX_DEVICE_NAME; i++)
    {
        if (DeviceName[i] == '\0' || DeviceName[i] == ' ') break;
        Node->Name.Buffer[i] = DeviceName[i];
    }

    Node->Name.Length = i;

    InsertTailList(&DeviceList, &Node->Entry);
    return Node;
}

VOID DosDeleteDevice(PDOS_DEVICE_NODE DeviceNode)
{
    RemoveEntryList(&DeviceNode->Entry);
    RtlFreeHeap(RtlGetProcessHeap(), 0, DeviceNode);
}

DWORD DosLoadDriver(LPCSTR DriverFile)
{
    DWORD Result = ERROR_SUCCESS;
    HANDLE FileHandle = INVALID_HANDLE_VALUE, FileMapping = NULL;
    LPBYTE Address = NULL;
    DWORD Driver;
    PDOS_DRIVER DriverHeader;
    WORD Segment = 0;
    DWORD FileSize;
    DWORD DriversLoaded = 0;
    DOS_INIT_REQUEST Request;
    PDOS_DEVICE_NODE DeviceNode;

    /* Open a handle to the driver file */
    FileHandle = CreateFileA(DriverFile,
                             GENERIC_READ,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);
    if (FileHandle == INVALID_HANDLE_VALUE)
    {
        Result = GetLastError();
        goto Cleanup;
    }

    /* Get the file size */
    FileSize = GetFileSize(FileHandle, NULL);

    /* Allocate DOS memory for the driver */
    Segment = DosAllocateMemory(FileSize >> 4, NULL);
    if (Segment == 0)
    {
        Result = DosLastError;
        goto Cleanup;
    }

    /* Create a mapping object for the file */
    FileMapping = CreateFileMapping(FileHandle,
                                    NULL,
                                    PAGE_READONLY,
                                    0,
                                    0,
                                    NULL);
    if (FileMapping == NULL)
    {
        Result = GetLastError();
        goto Cleanup;
    }

    /* Map the file into memory */
    Address = (LPBYTE)MapViewOfFile(FileMapping, FILE_MAP_READ, 0, 0, 0);
    if (Address == NULL)
    {
        Result = GetLastError();
        goto Cleanup;
    }

    /* Copy the entire file to the DOS memory */
    Driver = MAKELONG(0, Segment);
    DriverHeader = (PDOS_DRIVER)FAR_POINTER(Driver);
    RtlCopyMemory(DriverHeader, Address, FileSize);

    /* Loop through all the drivers in this file */
    while (TRUE)
    {
        if (!(DriverHeader->DeviceAttributes & DOS_DEVATTR_CHARACTER))
        {
            DPRINT1("Error loading driver at %04X:%04X: "
                    "Block device drivers are not supported.\n",
                    HIWORD(Driver),
                    LOWORD(Driver));
            goto Next;
        }

        /* Send the driver an init request */
        RtlZeroMemory(&Request, sizeof(Request));
        Request.Header.RequestLength = sizeof(DOS_INIT_REQUEST);
        Request.Header.CommandCode = DOS_DEVCMD_INIT;
        // TODO: Set Request.DeviceString to the appropriate line in CONFIG.NT!
        DosCallDriver(Driver, &Request.Header);

        if (Request.Header.Status & DOS_DEVSTAT_ERROR)
        {
            DPRINT1("Error loading driver at %04X:%04X: "
                    "Initialization routine returned error %u.\n",
                    HIWORD(Driver),
                    LOWORD(Driver),
                    Request.Header.Status & 0x7F);
            goto Next;
        }

        /* Create the device */
        DeviceNode = DosCreateDevice(DriverHeader->DeviceAttributes,
                                     DriverHeader->DeviceName);
        DeviceNode->Driver = Driver;
        DeviceNode->IoctlReadRoutine = DosDriverDispatchIoctlRead;
        DeviceNode->ReadRoutine = DosDriverDispatchRead;
        DeviceNode->PeekRoutine = DosDriverDispatchPeek;
        DeviceNode->InputStatusRoutine = DosDriverDispatchInputStatus;
        DeviceNode->FlushInputRoutine = DosDriverDispatchFlushInput;
        DeviceNode->IoctlWriteRoutine = DosDriverDispatchIoctlWrite;
        DeviceNode->WriteRoutine = DosDriverDispatchWrite;
        DeviceNode->OutputStatusRoutine = DosDriverDispatchOutputStatus;
        DeviceNode->FlushOutputRoutine = DosDriverDispatchFlushOutput;
        DeviceNode->OpenRoutine = DosDriverDispatchOpen;
        DeviceNode->CloseRoutine = DosDriverDispatchClose;
        DeviceNode->OutputUntilBusyRoutine = DosDriverDispatchOutputUntilBusy;

        DriversLoaded++;

Next:
        if (LOWORD(DriverHeader->Link) == 0xFFFF) break;
        Driver = DriverHeader->Link;
        DriverHeader = (PDOS_DRIVER)FAR_POINTER(Driver);
    }

    DPRINT1("%u drivers loaded from %s.\n", DriversLoaded, DriverFile);

Cleanup:
    if (Result != ERROR_SUCCESS)
    {
        /* It was not successful, cleanup the DOS memory */
        if (Segment) DosFreeMemory(Segment);
    }

    /* Unmap the file */
    if (Address != NULL) UnmapViewOfFile(Address);

    /* Close the file mapping object */
    if (FileMapping != NULL) CloseHandle(FileMapping);

    /* Close the file handle */
    if (FileHandle != INVALID_HANDLE_VALUE) CloseHandle(FileHandle);

    return Result;
}

/* EOF */
