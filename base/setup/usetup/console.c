/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/console.c
 * PURPOSE:         Console support functions
 * PROGRAMMER:
 */

/* INCLUDES ******************************************************************/

#include <usetup.h>
/* Blue Driver Header */
#include <blue/ntddblue.h>
#include "keytrans.h"

#define NDEBUG
#include <debug.h>

/* DATA **********************************************************************/

static BOOLEAN InputQueueEmpty;
static BOOLEAN WaitForInput;
static KEYBOARD_INPUT_DATA InputDataQueue; // Only one element!
static IO_STATUS_BLOCK InputIosb;
static UINT LastLoadedCodepage;

/* FUNCTIONS *****************************************************************/

typedef struct _CONSOLE_CABINET_CONTEXT
{
    CABINET_CONTEXT CabinetContext;
    PVOID Data;
    ULONG Size;
} CONSOLE_CABINET_CONTEXT, *PCONSOLE_CABINET_CONTEXT;

static PVOID
ConsoleCreateFileHandler(
    IN PCABINET_CONTEXT CabinetContext,
    IN ULONG FileSize)
{
    PCONSOLE_CABINET_CONTEXT ConsoleCabinetContext;

    ConsoleCabinetContext = (PCONSOLE_CABINET_CONTEXT)CabinetContext;
    ConsoleCabinetContext->Data = RtlAllocateHeap(ProcessHeap, 0, FileSize);
    if (!ConsoleCabinetContext->Data)
    {
        DPRINT("Failed to allocate %d bytes\n", FileSize);
        return NULL;
    }
    ConsoleCabinetContext->Size = FileSize;
    return ConsoleCabinetContext->Data;
}

BOOL
WINAPI
AllocConsole(VOID)
{
    NTSTATUS Status;
    UNICODE_STRING ScreenName = RTL_CONSTANT_STRING(L"\\??\\BlueScreen");
    UNICODE_STRING KeyboardName = RTL_CONSTANT_STRING(L"\\Device\\KeyboardClass0");
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG Enable;

    /* Open the screen */
    InitializeObjectAttributes(&ObjectAttributes,
                               &ScreenName,
                               0,
                               NULL,
                               NULL);
    Status = NtOpenFile(&StdOutput,
                        FILE_ALL_ACCESS,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_OPEN,
                        FILE_SYNCHRONOUS_IO_ALERT);
    if (!NT_SUCCESS(Status))
        return FALSE;

    /* Enable it */
    Enable = TRUE;
    Status = NtDeviceIoControlFile(StdOutput,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_CONSOLE_RESET_SCREEN,
                                   &Enable,
                                   sizeof(Enable),
                                   NULL,
                                   0);
    if (!NT_SUCCESS(Status))
    {
        NtClose(StdOutput);
        return FALSE;
    }

    /* Default to en-US output codepage */
    SetConsoleOutputCP(437);

    /* Open the keyboard */
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyboardName,
                               0,
                               NULL,
                               NULL);
    Status = NtOpenFile(&StdInput,
                        FILE_ALL_ACCESS,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_OPEN,
                        0);
    if (!NT_SUCCESS(Status))
    {
        NtClose(StdOutput);
        return FALSE;
    }

    /* Reset the queue state */
    InputQueueEmpty = TRUE;
    WaitForInput = FALSE;

    return TRUE;
}


BOOL
WINAPI
AttachConsole(
    IN DWORD dwProcessId)
{
    return FALSE;
}


BOOL
WINAPI
FreeConsole(VOID)
{
    /* Reset the queue state */
    InputQueueEmpty = TRUE;
    WaitForInput = FALSE;

    if (StdInput != INVALID_HANDLE_VALUE)
        NtClose(StdInput);

    if (StdOutput != INVALID_HANDLE_VALUE)
        NtClose(StdOutput);

    return TRUE;
}


BOOL
WINAPI
WriteConsole(
    IN HANDLE hConsoleOutput,
    IN const VOID *lpBuffer,
    IN DWORD nNumberOfCharsToWrite,
    OUT LPDWORD lpNumberOfCharsWritten,
    IN LPVOID lpReserved)
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    Status = NtWriteFile(hConsoleOutput,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         (PVOID)lpBuffer,
                         nNumberOfCharsToWrite,
                         NULL,
                         NULL);
    if (!NT_SUCCESS(Status))
        return FALSE;

    *lpNumberOfCharsWritten = IoStatusBlock.Information;
    return TRUE;
}


HANDLE
WINAPI
GetStdHandle(
    IN DWORD nStdHandle)
{
    switch (nStdHandle)
    {
        case STD_INPUT_HANDLE:
            return StdInput;
        case STD_OUTPUT_HANDLE:
            return StdOutput;
        default:
            return INVALID_HANDLE_VALUE;
    }
}


BOOL
WINAPI
FlushConsoleInputBuffer(
    IN HANDLE hConsoleInput)
{
    NTSTATUS Status;
    LARGE_INTEGER Offset, Timeout;
    IO_STATUS_BLOCK IoStatusBlock;
    KEYBOARD_INPUT_DATA InputData;

    /* Cancel any pending read */
    if (WaitForInput)
        NtCancelIoFile(hConsoleInput, &IoStatusBlock);

    /* Reset the queue state */
    InputQueueEmpty = TRUE;
    WaitForInput = FALSE;

    /* Flush the keyboard buffer */
    do
    {
        Offset.QuadPart = 0;
        Status = NtReadFile(hConsoleInput,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            &InputData,
                            sizeof(InputData),
                            &Offset,
                            NULL);
        if (Status == STATUS_PENDING)
        {
            Timeout.QuadPart = -100;
            Status = NtWaitForSingleObject(hConsoleInput, FALSE, &Timeout);
            if (Status == STATUS_TIMEOUT)
            {
                NtCancelIoFile(hConsoleInput, &IoStatusBlock);
                return TRUE;
            }
        }
    } while (NT_SUCCESS(Status));
    return FALSE;
}


BOOL
WINAPI
PeekConsoleInput(
    IN HANDLE hConsoleInput,
    OUT PINPUT_RECORD lpBuffer,
    IN DWORD nLength,
    OUT LPDWORD lpNumberOfEventsRead)
{
    NTSTATUS Status;
    LARGE_INTEGER Offset, Timeout;
    KEYBOARD_INPUT_DATA InputData;

    if (InputQueueEmpty)
    {
        /* Read the keyboard for an event, without waiting */
        if (!WaitForInput)
        {
            Offset.QuadPart = 0;
            Status = NtReadFile(hConsoleInput,
                                NULL,
                                NULL,
                                NULL,
                                &InputIosb,
                                &InputDataQueue,
                                sizeof(InputDataQueue),
                                &Offset,
                                NULL);
            if (!NT_SUCCESS(Status))
                return FALSE;
            if (Status == STATUS_PENDING)
            {
                /* No input yet, we will have to wait next time */
                *lpNumberOfEventsRead = 0;
                WaitForInput = TRUE;
                return TRUE;
            }
        }
        else
        {
            /*
             * We already tried to read from the keyboard and are
             * waiting for data, check whether something showed up.
             */
            Timeout.QuadPart = -100; // Wait just a little bit.
            Status = NtWaitForSingleObject(hConsoleInput, FALSE, &Timeout);
            if (Status == STATUS_TIMEOUT)
            {
                /* Nothing yet, continue waiting next time */
                *lpNumberOfEventsRead = 0;
                WaitForInput = TRUE;
                return TRUE;
            }
            WaitForInput = FALSE;
            if (!NT_SUCCESS(Status))
                return FALSE;
        }

        /* We got something in the queue */
        InputQueueEmpty = FALSE;
        WaitForInput = FALSE;
    }

    /* Fetch from the queue but keep it inside */
    InputData = InputDataQueue;

    lpBuffer->EventType = KEY_EVENT;
    Status = IntTranslateKey(hConsoleInput, &InputData, &lpBuffer->Event.KeyEvent);
    if (!NT_SUCCESS(Status))
        return FALSE;

    *lpNumberOfEventsRead = 1;
    return TRUE;
}


BOOL
WINAPI
ReadConsoleInput(
    IN HANDLE hConsoleInput,
    OUT PINPUT_RECORD lpBuffer,
    IN DWORD nLength,
    OUT LPDWORD lpNumberOfEventsRead)
{
    NTSTATUS Status;
    LARGE_INTEGER Offset;
    KEYBOARD_INPUT_DATA InputData;

    if (InputQueueEmpty)
    {
        /* Read the keyboard and wait for an event, skipping the queue */
        if (!WaitForInput)
        {
            Offset.QuadPart = 0;
            Status = NtReadFile(hConsoleInput,
                                NULL,
                                NULL,
                                NULL,
                                &InputIosb,
                                &InputDataQueue,
                                sizeof(InputDataQueue),
                                &Offset,
                                NULL);
            if (Status == STATUS_PENDING)
            {
                /* Block and wait for input */
                WaitForInput = TRUE;
                Status = NtWaitForSingleObject(hConsoleInput, FALSE, NULL);
                WaitForInput = FALSE;
                Status = InputIosb.Status;
            }
            if (!NT_SUCCESS(Status))
                return FALSE;
        }
        else
        {
            /*
             * We already tried to read from the keyboard and are
             * waiting for data, block and wait for input.
             */
            Status = NtWaitForSingleObject(hConsoleInput, FALSE, NULL);
            WaitForInput = FALSE;
            Status = InputIosb.Status;
            if (!NT_SUCCESS(Status))
                return FALSE;
        }
    }

    /* Fetch from the queue and empty it */
    InputData = InputDataQueue;
    InputQueueEmpty = TRUE;

    lpBuffer->EventType = KEY_EVENT;
    Status = IntTranslateKey(hConsoleInput, &InputData, &lpBuffer->Event.KeyEvent);
    if (!NT_SUCCESS(Status))
        return FALSE;

    *lpNumberOfEventsRead = 1;
    return TRUE;
}


BOOL
WINAPI
WriteConsoleOutputCharacterA(
    HANDLE hConsoleOutput,
    IN LPCSTR lpCharacter,
    IN DWORD nLength,
    IN COORD dwWriteCoord,
    OUT LPDWORD lpNumberOfCharsWritten)
{
    IO_STATUS_BLOCK IoStatusBlock;
    PCHAR Buffer;
    COORD *pCoord;
    PCHAR pText;
    NTSTATUS Status;

    Buffer = (CHAR*)RtlAllocateHeap(ProcessHeap,
                                    0,
                                    nLength + sizeof(COORD));
    pCoord = (COORD *)Buffer;
    pText = (PCHAR)(pCoord + 1);

    *pCoord = dwWriteCoord;
    memcpy(pText, lpCharacter, nLength);

    Status = NtDeviceIoControlFile(hConsoleOutput,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_CONSOLE_WRITE_OUTPUT_CHARACTER,
                                   NULL,
                                   0,
                                   Buffer,
                                   nLength + sizeof(COORD));

    RtlFreeHeap(ProcessHeap, 0, Buffer);
    if (!NT_SUCCESS(Status))
        return FALSE;

    *lpNumberOfCharsWritten = IoStatusBlock.Information;
    return TRUE;
}


BOOL
WINAPI
WriteConsoleOutputCharacterW(
    HANDLE hConsoleOutput,
    IN LPCWSTR lpCharacter,
    IN DWORD nLength,
    IN COORD dwWriteCoord,
    OUT LPDWORD lpNumberOfCharsWritten)
{
    IO_STATUS_BLOCK IoStatusBlock;
    PCHAR Buffer;
    COORD *pCoord;
    PCHAR pText;
    NTSTATUS Status;
//    ULONG i;

    UNICODE_STRING UnicodeString;
    OEM_STRING OemString;
    ULONG OemLength;

    UnicodeString.Length = nLength * sizeof(WCHAR);
    UnicodeString.MaximumLength = nLength * sizeof(WCHAR);
    UnicodeString.Buffer = (PWSTR)lpCharacter;

    OemLength = RtlUnicodeStringToOemSize(&UnicodeString);


    Buffer = (CHAR*)RtlAllocateHeap(ProcessHeap,
                                    0,
                                    OemLength + sizeof(COORD));
//                                    nLength + sizeof(COORD));
    if (Buffer== NULL)
        return FALSE;

    pCoord = (COORD *)Buffer;
    pText = (PCHAR)(pCoord + 1);

    *pCoord = dwWriteCoord;

    OemString.Length = 0;
    OemString.MaximumLength = OemLength;
    OemString.Buffer = pText;

    Status = RtlUnicodeStringToOemString(&OemString,
                                         &UnicodeString,
                                         FALSE);
    if (!NT_SUCCESS(Status))
        goto done;

    /* FIXME: use real unicode->oem conversion */
//    for (i = 0; i < nLength; i++)
//        pText[i] = (CHAR)lpCharacter[i];

    Status = NtDeviceIoControlFile(hConsoleOutput,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_CONSOLE_WRITE_OUTPUT_CHARACTER,
                                   NULL,
                                   0,
                                   Buffer,
                                   nLength + sizeof(COORD));

done:
    RtlFreeHeap(ProcessHeap, 0, Buffer);
    if (!NT_SUCCESS(Status))
        return FALSE;

    *lpNumberOfCharsWritten = IoStatusBlock.Information;
    return TRUE;
}


BOOL
WINAPI
FillConsoleOutputAttribute(
    IN HANDLE hConsoleOutput,
    IN WORD wAttribute,
    IN DWORD nLength,
    IN COORD dwWriteCoord,
    OUT LPDWORD lpNumberOfAttrsWritten)
{
    IO_STATUS_BLOCK IoStatusBlock;
    OUTPUT_ATTRIBUTE Buffer;
    NTSTATUS Status;

    Buffer.wAttribute = wAttribute;
    Buffer.nLength    = nLength;
    Buffer.dwCoord    = dwWriteCoord;

    Status = NtDeviceIoControlFile(hConsoleOutput,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_CONSOLE_FILL_OUTPUT_ATTRIBUTE,
                                   &Buffer,
                                   sizeof(OUTPUT_ATTRIBUTE),
                                   &Buffer,
                                   sizeof(OUTPUT_ATTRIBUTE));
    if (!NT_SUCCESS(Status))
        return FALSE;

    *lpNumberOfAttrsWritten = Buffer.dwTransfered;
    return TRUE;
}


BOOL
WINAPI
FillConsoleOutputCharacterA(
    IN HANDLE hConsoleOutput,
    IN CHAR cCharacter,
    IN DWORD nLength,
    IN COORD dwWriteCoord,
    OUT LPDWORD lpNumberOfCharsWritten)
{
    IO_STATUS_BLOCK IoStatusBlock;
    OUTPUT_CHARACTER Buffer;
    NTSTATUS Status;

    Buffer.cCharacter = cCharacter;
    Buffer.nLength = nLength;
    Buffer.dwCoord = dwWriteCoord;

    Status = NtDeviceIoControlFile(hConsoleOutput,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_CONSOLE_FILL_OUTPUT_CHARACTER,
                                   &Buffer,
                                   sizeof(OUTPUT_CHARACTER),
                                   &Buffer,
                                   sizeof(OUTPUT_CHARACTER));
    if (!NT_SUCCESS(Status))
        return FALSE;

    *lpNumberOfCharsWritten = Buffer.dwTransfered;
    return TRUE;
}


BOOL
WINAPI
GetConsoleScreenBufferInfo(
    IN HANDLE hConsoleOutput,
    OUT PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo)
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    Status = NtDeviceIoControlFile(hConsoleOutput,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_CONSOLE_GET_SCREEN_BUFFER_INFO,
                                   NULL,
                                   0,
                                   lpConsoleScreenBufferInfo,
                                   sizeof(CONSOLE_SCREEN_BUFFER_INFO));
    return NT_SUCCESS(Status);
}


BOOL
WINAPI
SetConsoleCursorInfo(
    IN HANDLE hConsoleOutput,
    IN const CONSOLE_CURSOR_INFO *lpConsoleCursorInfo)
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    Status = NtDeviceIoControlFile(hConsoleOutput,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_CONSOLE_SET_CURSOR_INFO,
                                   (PCONSOLE_CURSOR_INFO)lpConsoleCursorInfo,
                                   sizeof(CONSOLE_CURSOR_INFO),
                                   NULL,
                                   0);
    return NT_SUCCESS(Status);
}


BOOL
WINAPI
SetConsoleCursorPosition(
    IN HANDLE hConsoleOutput,
    IN COORD dwCursorPosition)
{
    CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    Status = GetConsoleScreenBufferInfo(hConsoleOutput, &ConsoleScreenBufferInfo);
    if (!NT_SUCCESS(Status))
        return FALSE;

    ConsoleScreenBufferInfo.dwCursorPosition.X = dwCursorPosition.X;
    ConsoleScreenBufferInfo.dwCursorPosition.Y = dwCursorPosition.Y;

    Status = NtDeviceIoControlFile(hConsoleOutput,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_CONSOLE_SET_SCREEN_BUFFER_INFO,
                                   &ConsoleScreenBufferInfo,
                                   sizeof(CONSOLE_SCREEN_BUFFER_INFO),
                                   NULL,
                                   0);
    return NT_SUCCESS(Status);
}


BOOL
WINAPI
SetConsoleTextAttribute(
    IN HANDLE hConsoleOutput,
    IN WORD wAttributes)
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    Status = NtDeviceIoControlFile(hConsoleOutput,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_CONSOLE_SET_TEXT_ATTRIBUTE,
                                   &wAttributes,
                                   sizeof(USHORT),
                                   NULL,
                                   0);
    return NT_SUCCESS(Status);
}


BOOL
WINAPI
SetConsoleOutputCP(
    IN UINT wCodepage)
{
    static PCWSTR FontFile = L"\\SystemRoot\\vgafonts.cab";
    WCHAR FontName[20];
    CONSOLE_CABINET_CONTEXT ConsoleCabinetContext;
    PCABINET_CONTEXT CabinetContext = &ConsoleCabinetContext.CabinetContext;
    CAB_SEARCH Search;
    ULONG CabStatus;
    HANDLE hConsoleOutput;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    if (wCodepage == LastLoadedCodepage)
        return TRUE;

    hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);

    CabinetInitialize(CabinetContext);
    CabinetSetEventHandlers(CabinetContext,
                            NULL, NULL, NULL, ConsoleCreateFileHandler);
    CabinetSetCabinetName(CabinetContext, FontFile);

    CabStatus = CabinetOpen(CabinetContext);
    if (CabStatus != CAB_STATUS_SUCCESS)
    {
        DPRINT("CabinetOpen('%S') returned 0x%08x\n", FontFile, CabStatus);
        return FALSE;
    }

    RtlStringCbPrintfW(FontName, sizeof(FontName),
                       L"%u-8x8.bin", wCodepage);
    CabStatus = CabinetFindFirst(CabinetContext, FontName, &Search);
    if (CabStatus != CAB_STATUS_SUCCESS)
    {
        DPRINT("CabinetFindFirst('%S', '%S') returned 0x%08x\n", FontFile, FontName, CabStatus);
        CabinetClose(CabinetContext);
        return FALSE;
    }

    CabStatus = CabinetExtractFile(CabinetContext, &Search);
    CabinetClose(CabinetContext);
    if (CabStatus != CAB_STATUS_SUCCESS)
    {
        DPRINT("CabinetExtractFile('%S', '%S') returned 0x%08x\n", FontFile, FontName, CabStatus);
        if (ConsoleCabinetContext.Data)
            RtlFreeHeap(ProcessHeap, 0, ConsoleCabinetContext.Data);
        return FALSE;
    }
    ASSERT(ConsoleCabinetContext.Data);

    Status = NtDeviceIoControlFile(hConsoleOutput,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_CONSOLE_LOADFONT,
                                   ConsoleCabinetContext.Data,
                                   ConsoleCabinetContext.Size,
                                   NULL,
                                   0);

    RtlFreeHeap(ProcessHeap, 0, ConsoleCabinetContext.Data);

    if (!NT_SUCCESS(Status))
        return FALSE;

    LastLoadedCodepage = wCodepage;
    return TRUE;
}


/* EOF */
