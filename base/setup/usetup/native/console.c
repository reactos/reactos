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
 * FILE:            base/setup/usetup/native/utils/console.c
 * PURPOSE:         Console support functions
 * PROGRAMMER:      Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include <usetup.h>
/* Blue Driver Header */
#include <blue/ntddblue.h>
#include "keytrans.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

BOOL
WINAPI
AllocConsole(VOID)
{
    UNICODE_STRING ScreenName = RTL_CONSTANT_STRING(L"\\??\\BlueScreen");
    UNICODE_STRING KeyboardName = RTL_CONSTANT_STRING(L"\\Device\\KeyboardClass0");
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

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
        return FALSE;

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
    LARGE_INTEGER Offset, Timeout;
    IO_STATUS_BLOCK IoStatusBlock;
    KEYBOARD_INPUT_DATA InputData;
    NTSTATUS Status;

    do
    {
        Offset.QuadPart = 0;
        Status = NtReadFile(hConsoleInput,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            &InputData,
                            sizeof(KEYBOARD_INPUT_DATA),
                            &Offset,
                            0);
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
ReadConsoleInput(
    IN HANDLE hConsoleInput,
    OUT PINPUT_RECORD lpBuffer,
    IN DWORD nLength,
    OUT LPDWORD lpNumberOfEventsRead)
{
    LARGE_INTEGER Offset;
    IO_STATUS_BLOCK IoStatusBlock;
    KEYBOARD_INPUT_DATA InputData;
    NTSTATUS Status;

    Offset.QuadPart = 0;
    Status = NtReadFile(hConsoleInput,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        &InputData,
                        sizeof(KEYBOARD_INPUT_DATA),
                        &Offset,
                        0);
    if (Status == STATUS_PENDING)
    {
        Status = NtWaitForSingleObject(hConsoleInput, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    if (!NT_SUCCESS(Status))
        return FALSE;

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
    UnicodeString.Buffer = (LPWSTR)lpCharacter;

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
    HANDLE hConsoleOutput;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);

    Status = NtDeviceIoControlFile(hConsoleOutput,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_CONSOLE_LOADFONT,
                                   &wCodepage,
                                   sizeof(ULONG),
                                   NULL,
                                   0);
    return NT_SUCCESS(Status);
}


/* EOF */
