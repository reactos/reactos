/*
 * COPYRIGHT:       GPLv2+ - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/dos/dos32krnl/device.c
 * PURPOSE:         DOS Device Support
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

#define NDEBUG
#include <debug.h>

#include "emulator.h"
#include "cpu/bop.h"
#include "device.h"

#include "dos.h"
#include "dos/dem.h"
#include "memory.h"

/* PRIVATE VARIABLES **********************************************************/

static const BYTE StrategyRoutine[] = {
    LOBYTE(EMULATOR_BOP),
    HIBYTE(EMULATOR_BOP),
    BOP_DOS,
    BOP_DRV_STRATEGY,
    0xCB // retf
};

static const BYTE InterruptRoutine[] = {
    LOBYTE(EMULATOR_BOP),
    HIBYTE(EMULATOR_BOP),
    BOP_DOS,
    BOP_DRV_INTERRUPT,
    0xCB // retf
};

C_ASSERT((sizeof(StrategyRoutine) + sizeof(InterruptRoutine)) == DEVICE_CODE_SIZE);

static LIST_ENTRY DeviceList = { &DeviceList, &DeviceList };
static PDOS_REQUEST_HEADER DeviceRequest;

/* PRIVATE FUNCTIONS **********************************************************/

static VOID DosCallDriver(DWORD Driver, PDOS_REQUEST_HEADER Request)
{
    PDOS_DRIVER DriverBlock = (PDOS_DRIVER)FAR_POINTER(Driver);
    WORD AX = getAX();
    WORD CX = getCX();
    WORD DX = getDX();
    WORD BX = getBX();
    WORD BP = getBP();
    WORD SI = getSI();
    WORD DI = getDI();
    WORD DS = getDS();
    WORD ES = getES();

    /* Set ES:BX to the location of the request */
    setES(DOS_DATA_SEGMENT);
    setBX(DOS_DATA_OFFSET(Sda.Request));

    /* Copy the request structure to ES:BX */
    RtlMoveMemory(&Sda->Request, Request, Request->RequestLength);

    /* Call the strategy routine, and then the interrupt routine */
    RunCallback16(&DosContext, MAKELONG(DriverBlock->StrategyRoutine , HIWORD(Driver)));
    RunCallback16(&DosContext, MAKELONG(DriverBlock->InterruptRoutine, HIWORD(Driver)));

    /* Get the request structure from ES:BX */
    RtlMoveMemory(Request, &Sda->Request, Request->RequestLength);

    /* Restore the registers */
    setAX(AX);
    setCX(CX);
    setDX(DX);
    setBX(BX);
    setBP(BP);
    setSI(SI);
    setDI(DI);
    setDS(DS);
    setES(ES);
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

static VOID DosAddDriver(DWORD Driver)
{
    PDOS_DRIVER LastDriver = &SysVars->NullDevice;

    /* Find the last driver in the list */
    while (LOWORD(LastDriver->Link) != 0xFFFF)
    {
        LastDriver = (PDOS_DRIVER)FAR_POINTER(LastDriver->Link);
    }

    /* Add the new driver to the list */
    LastDriver->Link = Driver;
    LastDriver = (PDOS_DRIVER)FAR_POINTER(Driver);

    if (LastDriver->DeviceAttributes & DOS_DEVATTR_CLOCK)
    {
        /* Update the active CLOCK driver */
        SysVars->ActiveClock = Driver;
    }

    if (LastDriver->DeviceAttributes
        & (DOS_DEVATTR_STDIN | DOS_DEVATTR_STDOUT | DOS_DEVATTR_CON))
    {
        /* Update the active CON driver */
        SysVars->ActiveCon = Driver;
    }
}

static VOID DosRemoveDriver(DWORD Driver)
{
    DWORD CurrentDriver = MAKELONG(DOS_DATA_OFFSET(SysVars.NullDevice), DOS_DATA_SEGMENT);

    while (LOWORD(CurrentDriver) != 0xFFFF)
    {
        PDOS_DRIVER DriverHeader = (PDOS_DRIVER)FAR_POINTER(CurrentDriver);

        if (DriverHeader->Link == Driver)
        {
            /* Remove it from the list */
            DriverHeader->Link = ((PDOS_DRIVER)FAR_POINTER(DriverHeader->Link))->Link;
            return;
        }

        CurrentDriver = DriverHeader->Link;
    }
}

static PDOS_DEVICE_NODE DosCreateDeviceNode(DWORD Driver)
{
    BYTE i;
    PDOS_DRIVER DriverHeader = (PDOS_DRIVER)FAR_POINTER(Driver);
    PDOS_DEVICE_NODE Node = RtlAllocateHeap(RtlGetProcessHeap(),
                                            HEAP_ZERO_MEMORY,
                                            sizeof(*Node));
    if (Node == NULL) return NULL;

    Node->Driver = Driver;
    Node->DeviceAttributes = DriverHeader->DeviceAttributes;

    /* Initialize the name string */
    Node->Name.Buffer = Node->NameBuffer;
    Node->Name.MaximumLength = MAX_DEVICE_NAME;

    for (i = 0; i < MAX_DEVICE_NAME; i++)
    {
        if (DriverHeader->DeviceName[i] == ' ') break;
        Node->Name.Buffer[i] = DriverHeader->DeviceName[i];
    }

    Node->Name.Length = i;

    InsertTailList(&DeviceList, &Node->Entry);
    return Node;
}

/* PUBLIC FUNCTIONS ***********************************************************/

PDOS_DEVICE_NODE DosGetDriverNode(DWORD Driver)
{
    PLIST_ENTRY i;
    PDOS_DEVICE_NODE Node;

    for (i = DeviceList.Flink; i != &DeviceList; i = i->Flink)
    {
        Node = CONTAINING_RECORD(i, DOS_DEVICE_NODE, Entry);
        if (Node->Driver == Driver) break;
    }

    if (i == &DeviceList)
    {
        DPRINT1("The driver at %04X:%04X has no associated device node. "
                "Installing automagically.\n",
                HIWORD(Driver),
                LOWORD(Driver));

        /* Create the device node */
        Node = DosCreateDeviceNode(Driver);
        Node->IoctlReadRoutine = DosDriverDispatchIoctlRead;
        Node->ReadRoutine = DosDriverDispatchRead;
        Node->PeekRoutine = DosDriverDispatchPeek;
        Node->InputStatusRoutine = DosDriverDispatchInputStatus;
        Node->FlushInputRoutine = DosDriverDispatchFlushInput;
        Node->IoctlWriteRoutine = DosDriverDispatchIoctlWrite;
        Node->WriteRoutine = DosDriverDispatchWrite;
        Node->OutputStatusRoutine = DosDriverDispatchOutputStatus;
        Node->FlushOutputRoutine = DosDriverDispatchFlushOutput;
        Node->OpenRoutine = DosDriverDispatchOpen;
        Node->CloseRoutine = DosDriverDispatchClose;
        Node->OutputUntilBusyRoutine = DosDriverDispatchOutputUntilBusy;
    }

    return Node;
}

PDOS_DEVICE_NODE DosGetDevice(LPCSTR DeviceName)
{
    DWORD CurrentDriver = MAKELONG(DOS_DATA_OFFSET(SysVars.NullDevice), DOS_DATA_SEGMENT);
    ANSI_STRING DeviceNameString;

    RtlInitAnsiString(&DeviceNameString, DeviceName);

    while (LOWORD(CurrentDriver) != 0xFFFF)
    {
        PDOS_DEVICE_NODE Node = DosGetDriverNode(CurrentDriver);
        PDOS_DRIVER DriverHeader = (PDOS_DRIVER)FAR_POINTER(CurrentDriver);

        if (RtlEqualString(&Node->Name, &DeviceNameString, TRUE)) return Node;
        CurrentDriver = DriverHeader->Link;
    }

    return NULL;
}

PDOS_DEVICE_NODE DosCreateDeviceEx(WORD Attributes, PCHAR DeviceName, WORD PrivateDataSize)
{
    BYTE i;
    WORD Segment;
    PDOS_DRIVER DriverHeader;
    PDOS_DEVICE_NODE Node;

    /* Make sure this is a character device */
    if (!(Attributes & DOS_DEVATTR_CHARACTER))
    {
        DPRINT1("ERROR: Block devices are not supported.\n");
        return NULL;
    }

    /* Create a driver header for this device */
    Segment = DosAllocateMemory(sizeof(DOS_DRIVER) + DEVICE_CODE_SIZE + PrivateDataSize, NULL);
    if (Segment == 0) return NULL;

    /* Fill the header with data */
    DriverHeader = SEG_OFF_TO_PTR(Segment, 0);
    DriverHeader->Link = MAXDWORD;
    DriverHeader->DeviceAttributes = Attributes;
    DriverHeader->StrategyRoutine = sizeof(DOS_DRIVER);
    DriverHeader->InterruptRoutine = sizeof(DOS_DRIVER) + sizeof(StrategyRoutine);

    RtlFillMemory(DriverHeader->DeviceName, MAX_DEVICE_NAME, ' ');
    for (i = 0; i < MAX_DEVICE_NAME; i++)
    {
        if (DeviceName[i] == '\0' || DeviceName[i] == ' ') break;
        DriverHeader->DeviceName[i] = DeviceName[i];
    }

    /* Write the routines */
    RtlMoveMemory(SEG_OFF_TO_PTR(Segment, DriverHeader->StrategyRoutine),
                  StrategyRoutine,
                  sizeof(StrategyRoutine));
    RtlMoveMemory(SEG_OFF_TO_PTR(Segment, DriverHeader->InterruptRoutine),
                  InterruptRoutine,
                  sizeof(InterruptRoutine));

    /* Create the node */
    Node = DosCreateDeviceNode(MAKELONG(0, Segment));
    if (Node == NULL)
    {
        DosFreeMemory(Segment);
        return NULL;
    }

    DosAddDriver(Node->Driver);
    return Node;
}

PDOS_DEVICE_NODE DosCreateDevice(WORD Attributes, PCHAR DeviceName)
{
    /* Call the extended API */
    return DosCreateDeviceEx(Attributes, DeviceName, 0);
}

VOID DosDeleteDevice(PDOS_DEVICE_NODE DeviceNode)
{
    DosRemoveDriver(DeviceNode->Driver);

    ASSERT(LOWORD(DeviceNode->Driver) == 0);
    DosFreeMemory(HIWORD(DeviceNode->Driver));

    RemoveEntryList(&DeviceNode->Entry);
    RtlFreeHeap(RtlGetProcessHeap(), 0, DeviceNode);
}

VOID DeviceStrategyBop(VOID)
{
    /* Save ES:BX */
    DeviceRequest = (PDOS_REQUEST_HEADER)SEG_OFF_TO_PTR(getES(), getBX());
}

VOID DeviceInterruptBop(VOID)
{
    PLIST_ENTRY i;
    PDOS_DEVICE_NODE Node;
    DWORD DriverAddress = (getCS() << 4) + getIP() - sizeof(DOS_DRIVER) - 9;

    /* Get the device node for this driver */
    for (i = DeviceList.Flink; i != &DeviceList; i = i->Flink)
    {
        Node = CONTAINING_RECORD(i, DOS_DEVICE_NODE, Entry);
        if (TO_LINEAR(HIWORD(Node->Driver), LOWORD(Node->Driver)) == DriverAddress) break;
    }

    if (i == &DeviceList)
    {
        DPRINT1("Device interrupt BOP from an unknown location.\n");
        return;
    }

    switch (DeviceRequest->CommandCode)
    {
        case DOS_DEVCMD_IOCTL_READ:
        {
            PDOS_IOCTL_RW_REQUEST Request = (PDOS_IOCTL_RW_REQUEST)DeviceRequest;

            DeviceRequest->Status = Node->IoctlReadRoutine(
                Node,
                Request->BufferPointer,
                &Request->Length
            );

            break;
        }

        case DOS_DEVCMD_READ:
        {
            PDOS_RW_REQUEST Request = (PDOS_RW_REQUEST)DeviceRequest;

            DeviceRequest->Status = Node->ReadRoutine(
                Node,
                Request->BufferPointer,
                &Request->Length
            );

            break;
        }

        case DOS_DEVCMD_PEEK:
        {
            PDOS_PEEK_REQUEST Request = (PDOS_PEEK_REQUEST)DeviceRequest;
            DeviceRequest->Status = Node->PeekRoutine(Node, &Request->Character);
            break;
        }

        case DOS_DEVCMD_INSTAT:
        {
            DeviceRequest->Status = Node->InputStatusRoutine(Node);
            break;
        }

        case DOS_DEVCMD_FLUSH_INPUT:
        {
            DeviceRequest->Status = Node->FlushInputRoutine(Node);
            break;
        }

        case DOS_DEVCMD_IOCTL_WRITE:
        {
            PDOS_IOCTL_RW_REQUEST Request = (PDOS_IOCTL_RW_REQUEST)DeviceRequest;

            DeviceRequest->Status = Node->IoctlWriteRoutine(
                Node,
                Request->BufferPointer,
                &Request->Length
            );

            break;
        }

        case DOS_DEVCMD_WRITE:
        {
            PDOS_RW_REQUEST Request = (PDOS_RW_REQUEST)DeviceRequest;

            DeviceRequest->Status = Node->WriteRoutine(Node,
                Request->BufferPointer,
                &Request->Length
            );

            break;
        }

        case DOS_DEVCMD_OUTSTAT:
        {
            DeviceRequest->Status = Node->OutputStatusRoutine(Node);
            break;
        }

        case DOS_DEVCMD_FLUSH_OUTPUT:
        {
            DeviceRequest->Status = Node->FlushOutputRoutine(Node);
            break;
        }

        case DOS_DEVCMD_OPEN:
        {
            DeviceRequest->Status = Node->OpenRoutine(Node);
            break;
        }

        case DOS_DEVCMD_CLOSE:
        {
            DeviceRequest->Status = Node->CloseRoutine(Node);
            break;
        }

        case DOS_DEVCMD_OUTPUT_BUSY:
        {
            PDOS_OUTPUT_BUSY_REQUEST Request = (PDOS_OUTPUT_BUSY_REQUEST)DeviceRequest;

            DeviceRequest->Status = Node->OutputUntilBusyRoutine(
                Node,
                Request->BufferPointer,
                &Request->Length
            );

            break;
        }

        default:
        {
            DPRINT1("Unknown device command code: %u\n", DeviceRequest->CommandCode);
        }
    }
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
        Result = Sda->LastErrorCode;
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

        /* Create the device node */
        DeviceNode = DosCreateDeviceNode(Driver);
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

        DosAddDriver(Driver);
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
