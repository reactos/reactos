/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/console/readwrite.c
 * PURPOSE:         Win32 Console Client read-write functions
 * PROGRAMMERS:     Emanuele Aliberti
 *                  Marty Dill
 *                  Filip Navara (xnavara@volny.cz)
 *                  Thomas Weidenmueller (w3seek@reactos.org)
 *                  Jeffrey Morlan
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>


/* PRIVATE FUNCTIONS **********************************************************/

/******************
 * Read functions *
 ******************/

static
BOOL
IntReadConsole(HANDLE hConsoleInput,
               PVOID lpBuffer,
               DWORD nNumberOfCharsToRead,
               LPDWORD lpNumberOfCharsRead,
               PCONSOLE_READCONSOLE_CONTROL pInputControl,
               BOOL bUnicode)
{
    CSR_API_MESSAGE Request;
    ULONG CsrRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG CharSize = (bUnicode ? sizeof(WCHAR) : sizeof(CHAR));

    CaptureBuffer = CsrAllocateCaptureBuffer(1, nNumberOfCharsToRead * CharSize);
    if (CaptureBuffer == NULL)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    CsrAllocateMessagePointer(CaptureBuffer,
                              nNumberOfCharsToRead * CharSize,
                              &Request.Data.ReadConsoleRequest.Buffer);

    Request.Data.ReadConsoleRequest.ConsoleHandle = hConsoleInput;
    Request.Data.ReadConsoleRequest.Unicode = bUnicode;
    Request.Data.ReadConsoleRequest.NrCharactersToRead = (WORD)nNumberOfCharsToRead;
    Request.Data.ReadConsoleRequest.NrCharactersRead = 0;
    Request.Data.ReadConsoleRequest.CtrlWakeupMask = 0;
    if (pInputControl && pInputControl->nLength == sizeof(CONSOLE_READCONSOLE_CONTROL))
    {
        Request.Data.ReadConsoleRequest.NrCharactersRead = pInputControl->nInitialChars;
        memcpy(Request.Data.ReadConsoleRequest.Buffer,
               lpBuffer,
               pInputControl->nInitialChars * sizeof(WCHAR));
        Request.Data.ReadConsoleRequest.CtrlWakeupMask = pInputControl->dwCtrlWakeupMask;
    }

    CsrRequest = CSR_CREATE_API_NUMBER(CSR_CONSOLE, READ_CONSOLE);

    do
    {
        if (Status == STATUS_PENDING)
        {
            Status = NtWaitForSingleObject(Request.Data.ReadConsoleRequest.EventHandle,
                                           FALSE,
                                           0);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Wait for console input failed!\n");
                break;
            }
        }

        Status = CsrClientCallServer(&Request,
                                     CaptureBuffer,
                                     CsrRequest,
                                     sizeof(CSR_API_MESSAGE));
        if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Request.Status))
        {
            DPRINT1("CSR returned error in ReadConsole\n");
            CsrFreeCaptureBuffer(CaptureBuffer);
            BaseSetLastNTError(Status);
            return FALSE;
        }
    }
    while (Status == STATUS_PENDING);

    memcpy(lpBuffer,
           Request.Data.ReadConsoleRequest.Buffer,
           Request.Data.ReadConsoleRequest.NrCharactersRead * CharSize);

    if (lpNumberOfCharsRead != NULL)
        *lpNumberOfCharsRead = Request.Data.ReadConsoleRequest.NrCharactersRead;

    if (pInputControl && pInputControl->nLength == sizeof(CONSOLE_READCONSOLE_CONTROL))
        pInputControl->dwControlKeyState = Request.Data.ReadConsoleRequest.ControlKeyState;

    CsrFreeCaptureBuffer(CaptureBuffer);

    return TRUE;
}


static
BOOL
IntPeekConsoleInput(HANDLE hConsoleInput,
                    PINPUT_RECORD lpBuffer,
                    DWORD nLength,
                    LPDWORD lpNumberOfEventsRead,
                    BOOL bUnicode)
{
    CSR_API_MESSAGE Request;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    ULONG Size;

    if (lpBuffer == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Size = nLength * sizeof(INPUT_RECORD);

    /* Allocate a Capture Buffer */
    DPRINT("IntPeekConsoleInput: %lx %p\n", Size, lpNumberOfEventsRead);
    CaptureBuffer = CsrAllocateCaptureBuffer(1, Size);
    if (CaptureBuffer == NULL)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Allocate space in the Buffer */
    CsrCaptureMessageBuffer(CaptureBuffer,
                            NULL,
                            Size,
                            (PVOID*)&Request.Data.PeekConsoleInputRequest.InputRecord);

    /* Set up the data to send to the Console Server */
    Request.Data.PeekConsoleInputRequest.ConsoleHandle = hConsoleInput;
    Request.Data.PeekConsoleInputRequest.Unicode = bUnicode;
    Request.Data.PeekConsoleInputRequest.Length = nLength;

    /* Call the server */
    CsrClientCallServer(&Request,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CSR_CONSOLE, PEEK_CONSOLE_INPUT),
                        sizeof(CSR_API_MESSAGE));
    DPRINT("Server returned: %x\n", Request.Status);

    /* Check for success*/
    if (NT_SUCCESS(Request.Status))
    {
        /* Return the number of events read */
        DPRINT("Events read: %lx\n", Request.Data.PeekConsoleInputRequest.Length);
        *lpNumberOfEventsRead = Request.Data.PeekConsoleInputRequest.Length;

        /* Copy into the buffer */
        DPRINT("Copying to buffer\n");
        RtlCopyMemory(lpBuffer,
                      Request.Data.PeekConsoleInputRequest.InputRecord,
                      sizeof(INPUT_RECORD) * *lpNumberOfEventsRead);
    }
    else
    {
        /* Error out */
       *lpNumberOfEventsRead = 0;
       BaseSetLastNTError(Request.Status);
    }

    /* Release the capture buffer */
    CsrFreeCaptureBuffer(CaptureBuffer);

    /* Return TRUE or FALSE */
    return NT_SUCCESS(Request.Status);
}


static
BOOL
IntReadConsoleInput(HANDLE hConsoleInput,
                    PINPUT_RECORD lpBuffer,
                    DWORD nLength,
                    LPDWORD lpNumberOfEventsRead,
                    BOOL bUnicode)
{
    CSR_API_MESSAGE Request;
    ULONG CsrRequest;
    ULONG Read;
    NTSTATUS Status;

    CsrRequest = CSR_CREATE_API_NUMBER(CSR_CONSOLE, READ_INPUT);
    Read = 0;

    while (nLength > 0)
    {
        Request.Data.ReadInputRequest.ConsoleHandle = hConsoleInput;
        Request.Data.ReadInputRequest.Unicode = bUnicode;

        Status = CsrClientCallServer(&Request,
                                     NULL,
                                     CsrRequest,
                                     sizeof(CSR_API_MESSAGE));
        if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Request.Status))
        {
            if (Read == 0)
            {
                /* we couldn't read a single record, fail */
                BaseSetLastNTError(Status);
                return FALSE;
            }
            else
            {
                /* FIXME - fail gracefully in case we already read at least one record? */
                break;
            }
        }
        else if (Status == STATUS_PENDING)
        {
            if (Read == 0)
            {
                Status = NtWaitForSingleObject(Request.Data.ReadInputRequest.Event, FALSE, 0);
                if (!NT_SUCCESS(Status))
                {
                    BaseSetLastNTError(Status);
                    break;
                }
            }
            else
            {
                /* nothing more to read (waiting for more input??), let's just bail */
                break;
            }
        }
        else
        {
            lpBuffer[Read++] = Request.Data.ReadInputRequest.Input;
            nLength--;

            if (!Request.Data.ReadInputRequest.MoreEvents)
            {
                /* nothing more to read, bail */
                break;
            }
        }
    }

    if (lpNumberOfEventsRead != NULL)
    {
        *lpNumberOfEventsRead = Read;
    }

    return (Read > 0);
}


static
BOOL
IntReadConsoleOutput(HANDLE hConsoleOutput,
                     PCHAR_INFO lpBuffer,
                     COORD dwBufferSize,
                     COORD dwBufferCoord,
                     PSMALL_RECT lpReadRegion,
                     BOOL bUnicode)
{
    CSR_API_MESSAGE Request;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    DWORD Size, SizeX, SizeY;

    if (lpBuffer == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Size = dwBufferSize.X * dwBufferSize.Y * sizeof(CHAR_INFO);

    /* Allocate a Capture Buffer */
    DPRINT("IntReadConsoleOutput: %lx %p\n", Size, lpReadRegion);
    CaptureBuffer = CsrAllocateCaptureBuffer(1, Size);
    if (CaptureBuffer == NULL)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed with size 0x%x!\n", Size);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Allocate space in the Buffer */
    CsrCaptureMessageBuffer(CaptureBuffer,
                            NULL,
                            Size,
                            (PVOID*)&Request.Data.ReadConsoleOutputRequest.CharInfo);

    /* Set up the data to send to the Console Server */
    Request.Data.ReadConsoleOutputRequest.ConsoleHandle = hConsoleOutput;
    Request.Data.ReadConsoleOutputRequest.Unicode = bUnicode;
    Request.Data.ReadConsoleOutputRequest.BufferSize = dwBufferSize;
    Request.Data.ReadConsoleOutputRequest.BufferCoord = dwBufferCoord;
    Request.Data.ReadConsoleOutputRequest.ReadRegion = *lpReadRegion;

    /* Call the server */
    CsrClientCallServer(&Request,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CSR_CONSOLE, READ_CONSOLE_OUTPUT),
                        sizeof(CSR_API_MESSAGE));
    DPRINT("Server returned: %x\n", Request.Status);

    /* Check for success*/
    if (NT_SUCCESS(Request.Status))
    {
        /* Copy into the buffer */
        DPRINT("Copying to buffer\n");
        SizeX = Request.Data.ReadConsoleOutputRequest.ReadRegion.Right -
                Request.Data.ReadConsoleOutputRequest.ReadRegion.Left + 1;
        SizeY = Request.Data.ReadConsoleOutputRequest.ReadRegion.Bottom -
                Request.Data.ReadConsoleOutputRequest.ReadRegion.Top + 1;
        RtlCopyMemory(lpBuffer,
                      Request.Data.ReadConsoleOutputRequest.CharInfo,
                      sizeof(CHAR_INFO) * SizeX * SizeY);
    }
    else
    {
        /* Error out */
        BaseSetLastNTError(Request.Status);
    }

    /* Return the read region */
    DPRINT("read region: %lx\n", Request.Data.ReadConsoleOutputRequest.ReadRegion);
    *lpReadRegion = Request.Data.ReadConsoleOutputRequest.ReadRegion;

    /* Release the capture buffer */
    CsrFreeCaptureBuffer(CaptureBuffer);

    /* Return TRUE or FALSE */
    return NT_SUCCESS(Request.Status);
}


static
BOOL
IntReadConsoleOutputCharacter(HANDLE hConsoleOutput,
                              PVOID lpCharacter,
                              DWORD nLength,
                              COORD dwReadCoord,
                              LPDWORD lpNumberOfCharsRead,
                              BOOL bUnicode)
{
    PCSR_API_MESSAGE Request;
    ULONG CsrRequest;
    NTSTATUS Status;
    ULONG SizeBytes, CharSize;
    DWORD CharsRead = 0;

    CharSize = (bUnicode ? sizeof(WCHAR) : sizeof(CHAR));

    nLength = min(nLength, CSRSS_MAX_READ_CONSOLE_OUTPUT_CHAR / CharSize);
    SizeBytes = nLength * CharSize;

    Request = RtlAllocateHeap(RtlGetProcessHeap(), 0,
                              max(sizeof(CSR_API_MESSAGE),
                              CSR_API_MESSAGE_HEADER_SIZE(CSRSS_READ_CONSOLE_OUTPUT_CHAR) + SizeBytes));
    if (Request == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    CsrRequest = CSR_CREATE_API_NUMBER(CSR_CONSOLE, READ_CONSOLE_OUTPUT_CHAR);
    Request->Data.ReadConsoleOutputCharRequest.ReadCoord = dwReadCoord;

    while (nLength > 0)
    {
        DWORD BytesRead;

        Request->Data.ReadConsoleOutputCharRequest.ConsoleHandle = hConsoleOutput;
        Request->Data.ReadConsoleOutputCharRequest.Unicode = bUnicode;
        Request->Data.ReadConsoleOutputCharRequest.NumCharsToRead = nLength;
        SizeBytes = Request->Data.ReadConsoleOutputCharRequest.NumCharsToRead * CharSize;

        Status = CsrClientCallServer(Request,
                                     NULL,
                                     CsrRequest,
                                     max(sizeof(CSR_API_MESSAGE),
                                     CSR_API_MESSAGE_HEADER_SIZE(CSRSS_READ_CONSOLE_OUTPUT_CHAR) + SizeBytes));
        if (!NT_SUCCESS(Status) || !NT_SUCCESS(Request->Status))
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, Request);
            BaseSetLastNTError(Status);
            break;
        }

        BytesRead = Request->Data.ReadConsoleOutputCharRequest.CharsRead * CharSize;
        memcpy(lpCharacter, Request->Data.ReadConsoleOutputCharRequest.String, BytesRead);
        lpCharacter = (PVOID)((ULONG_PTR)lpCharacter + (ULONG_PTR)BytesRead);
        CharsRead += Request->Data.ReadConsoleOutputCharRequest.CharsRead;
        nLength -= Request->Data.ReadConsoleOutputCharRequest.CharsRead;

        Request->Data.ReadConsoleOutputCharRequest.ReadCoord = Request->Data.ReadConsoleOutputCharRequest.EndCoord;
    }

    if (lpNumberOfCharsRead != NULL)
    {
        *lpNumberOfCharsRead = CharsRead;
    }

    RtlFreeHeap(RtlGetProcessHeap(), 0, Request);

    return TRUE;
}


/*******************
 * Write functions *
 *******************/

static
BOOL
IntWriteConsole(HANDLE hConsoleOutput,
                PVOID lpBuffer,
                DWORD nNumberOfCharsToWrite,
                LPDWORD lpNumberOfCharsWritten,
                LPVOID lpReserved,
                BOOL bUnicode)
{
    PCSR_API_MESSAGE Request;
    ULONG CsrRequest;
    NTSTATUS Status;
    USHORT nChars;
    ULONG SizeBytes, CharSize;
    DWORD Written = 0;

    CharSize = (bUnicode ? sizeof(WCHAR) : sizeof(CHAR));
    Request = RtlAllocateHeap(RtlGetProcessHeap(),
                              0,
                              max(sizeof(CSR_API_MESSAGE),
                              CSR_API_MESSAGE_HEADER_SIZE(CSRSS_WRITE_CONSOLE) + min(nNumberOfCharsToWrite,
                              CSRSS_MAX_WRITE_CONSOLE / CharSize) * CharSize));
    if (Request == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    CsrRequest = CSR_CREATE_API_NUMBER(CSR_CONSOLE, WRITE_CONSOLE);

    while (nNumberOfCharsToWrite > 0)
    {
        Request->Data.WriteConsoleRequest.ConsoleHandle = hConsoleOutput;
        Request->Data.WriteConsoleRequest.Unicode = bUnicode;

        nChars = (USHORT)min(nNumberOfCharsToWrite, CSRSS_MAX_WRITE_CONSOLE / CharSize);
        Request->Data.WriteConsoleRequest.NrCharactersToWrite = nChars;

        SizeBytes = nChars * CharSize;

        memcpy(Request->Data.WriteConsoleRequest.Buffer, lpBuffer, SizeBytes);

        Status = CsrClientCallServer(Request,
                                     NULL,
                                     CsrRequest,
                                     max(sizeof(CSR_API_MESSAGE),
                                     CSR_API_MESSAGE_HEADER_SIZE(CSRSS_WRITE_CONSOLE) + SizeBytes));

        if (Status == STATUS_PENDING)
        {
            WaitForSingleObject(Request->Data.WriteConsoleRequest.UnpauseEvent, INFINITE);
            CloseHandle(Request->Data.WriteConsoleRequest.UnpauseEvent);
            continue;
        }
        if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Request->Status))
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, Request);
            BaseSetLastNTError(Status);
            return FALSE;
        }

        nNumberOfCharsToWrite -= nChars;
        lpBuffer = (PVOID)((ULONG_PTR)lpBuffer + (ULONG_PTR)SizeBytes);
        Written += Request->Data.WriteConsoleRequest.NrCharactersWritten;
    }

    if (lpNumberOfCharsWritten != NULL)
    {
        *lpNumberOfCharsWritten = Written;
    }
    RtlFreeHeap(RtlGetProcessHeap(), 0, Request);

    return TRUE;
}


static
BOOL
IntWriteConsoleInput(HANDLE hConsoleInput,
                     PINPUT_RECORD lpBuffer,
                     DWORD nLength,
                     LPDWORD lpNumberOfEventsWritten,
                     BOOL bUnicode)
{
    CSR_API_MESSAGE Request;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    DWORD Size;

    if (lpBuffer == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Size = nLength * sizeof(INPUT_RECORD);

    /* Allocate a Capture Buffer */
    DPRINT("IntWriteConsoleInput: %lx %p\n", Size, lpNumberOfEventsWritten);
    CaptureBuffer = CsrAllocateCaptureBuffer(1, Size);
    if (CaptureBuffer == NULL)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Allocate space in the Buffer */
    CsrCaptureMessageBuffer(CaptureBuffer,
                            lpBuffer,
                            Size,
                            (PVOID*)&Request.Data.WriteConsoleInputRequest.InputRecord);

    /* Set up the data to send to the Console Server */
    Request.Data.WriteConsoleInputRequest.ConsoleHandle = hConsoleInput;
    Request.Data.WriteConsoleInputRequest.Unicode = bUnicode;
    Request.Data.WriteConsoleInputRequest.Length = nLength;

    /* Call the server */
    CsrClientCallServer(&Request,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CSR_CONSOLE, WRITE_CONSOLE_INPUT),
                        sizeof(CSR_API_MESSAGE));
    DPRINT("Server returned: %x\n", Request.Status);

    /* Check for success*/
    if (NT_SUCCESS(Request.Status))
    {
        /* Return the number of events read */
        DPRINT("Events read: %lx\n", Request.Data.WriteConsoleInputRequest.Length);
        *lpNumberOfEventsWritten = Request.Data.WriteConsoleInputRequest.Length;
    }
    else
    {
        /* Error out */
        *lpNumberOfEventsWritten = 0;
        BaseSetLastNTError(Request.Status);
    }

    /* Release the capture buffer */
    CsrFreeCaptureBuffer(CaptureBuffer);

    /* Return TRUE or FALSE */
    return NT_SUCCESS(Request.Status);
}


static
BOOL
IntWriteConsoleOutput(HANDLE hConsoleOutput,
                      CONST CHAR_INFO *lpBuffer,
                      COORD dwBufferSize,
                      COORD dwBufferCoord,
                      PSMALL_RECT lpWriteRegion,
                      BOOL bUnicode)
{
    CSR_API_MESSAGE Request;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    ULONG Size;

    Size = dwBufferSize.Y * dwBufferSize.X * sizeof(CHAR_INFO);

    /* Allocate a Capture Buffer */
    DPRINT("IntWriteConsoleOutput: %lx %p\n", Size, lpWriteRegion);
    CaptureBuffer = CsrAllocateCaptureBuffer(1, Size);
    if (CaptureBuffer == NULL)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Allocate space in the Buffer */
    CsrCaptureMessageBuffer(CaptureBuffer,
                            NULL,
                            Size,
                            (PVOID*)&Request.Data.WriteConsoleOutputRequest.CharInfo);

    /* Copy from the buffer */
    RtlCopyMemory(Request.Data.WriteConsoleOutputRequest.CharInfo, lpBuffer, Size);

    /* Set up the data to send to the Console Server */
    Request.Data.WriteConsoleOutputRequest.ConsoleHandle = hConsoleOutput;
    Request.Data.WriteConsoleOutputRequest.Unicode = bUnicode;
    Request.Data.WriteConsoleOutputRequest.BufferSize = dwBufferSize;
    Request.Data.WriteConsoleOutputRequest.BufferCoord = dwBufferCoord;
    Request.Data.WriteConsoleOutputRequest.WriteRegion = *lpWriteRegion;

    /* Call the server */
    CsrClientCallServer(&Request,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CSR_CONSOLE, WRITE_CONSOLE_OUTPUT),
                        sizeof(CSR_API_MESSAGE));
    DPRINT("Server returned: %x\n", Request.Status);

    /* Check for success*/
    if (!NT_SUCCESS(Request.Status))
    {
        /* Error out */
        BaseSetLastNTError(Request.Status);
    }

    /* Return the read region */
    DPRINT("read region: %lx\n", Request.Data.WriteConsoleOutputRequest.WriteRegion);
    *lpWriteRegion = Request.Data.WriteConsoleOutputRequest.WriteRegion;

    /* Release the capture buffer */
    CsrFreeCaptureBuffer(CaptureBuffer);

    /* Return TRUE or FALSE */
    return NT_SUCCESS(Request.Status);
}


static
BOOL
IntWriteConsoleOutputCharacter(HANDLE hConsoleOutput,
                               PVOID lpCharacter,
                               DWORD nLength,
                               COORD dwWriteCoord,
                               LPDWORD lpNumberOfCharsWritten,
                               BOOL bUnicode)
{
    PCSR_API_MESSAGE Request;
    ULONG CsrRequest;
    NTSTATUS Status;
    ULONG CharSize, nChars;
    //ULONG SizeBytes;
    DWORD Written = 0;

    CharSize = (bUnicode ? sizeof(WCHAR) : sizeof(CHAR));

    nChars = min(nLength, CSRSS_MAX_WRITE_CONSOLE_OUTPUT_CHAR / CharSize);
    //SizeBytes = nChars * CharSize;

    Request = RtlAllocateHeap(RtlGetProcessHeap(), 0,
                              max(sizeof(CSR_API_MESSAGE),
                              CSR_API_MESSAGE_HEADER_SIZE(CSRSS_WRITE_CONSOLE_OUTPUT_CHAR)
                                + min (nChars, CSRSS_MAX_WRITE_CONSOLE_OUTPUT_CHAR / CharSize) * CharSize));
    if (Request == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    CsrRequest = CSR_CREATE_API_NUMBER(CSR_CONSOLE, WRITE_CONSOLE_OUTPUT_CHAR);
    Request->Data.WriteConsoleOutputCharRequest.Coord = dwWriteCoord;

    while (nLength > 0)
    {
        DWORD BytesWrite;

        Request->Data.WriteConsoleOutputCharRequest.ConsoleHandle = hConsoleOutput;
        Request->Data.WriteConsoleOutputCharRequest.Unicode = bUnicode;
        Request->Data.WriteConsoleOutputCharRequest.Length = (WORD)min(nLength, nChars);
        BytesWrite = Request->Data.WriteConsoleOutputCharRequest.Length * CharSize;

        memcpy(Request->Data.WriteConsoleOutputCharRequest.String, lpCharacter, BytesWrite);

        Status = CsrClientCallServer(Request,
                                     NULL,
                                     CsrRequest,
                                     max(sizeof(CSR_API_MESSAGE),
                                     CSR_API_MESSAGE_HEADER_SIZE(CSRSS_WRITE_CONSOLE_OUTPUT_CHAR) + BytesWrite));

        if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Request->Status))
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, Request);
            BaseSetLastNTError(Status);
            return FALSE;
        }

        nLength -= Request->Data.WriteConsoleOutputCharRequest.NrCharactersWritten;
        lpCharacter = (PVOID)((ULONG_PTR)lpCharacter + (ULONG_PTR)(Request->Data.WriteConsoleOutputCharRequest.NrCharactersWritten * CharSize));
        Written += Request->Data.WriteConsoleOutputCharRequest.NrCharactersWritten;

        Request->Data.WriteConsoleOutputCharRequest.Coord = Request->Data.WriteConsoleOutputCharRequest.EndCoord;
    }

    if (lpNumberOfCharsWritten != NULL)
    {
        *lpNumberOfCharsWritten = Written;
    }

    RtlFreeHeap(RtlGetProcessHeap(), 0, Request);

    return TRUE;
}


static
BOOL
IntFillConsoleOutputCharacter(HANDLE hConsoleOutput,
                              PVOID cCharacter,
                              DWORD nLength,
                              COORD dwWriteCoord,
                              LPDWORD lpNumberOfCharsWritten,
                              BOOL bUnicode)
{
    CSR_API_MESSAGE Request;
    NTSTATUS Status;

    Request.Data.FillOutputRequest.ConsoleHandle = hConsoleOutput;
    Request.Data.FillOutputRequest.Unicode = bUnicode;

    if(bUnicode)
        Request.Data.FillOutputRequest.Char.UnicodeChar = *((WCHAR*)cCharacter);
    else
        Request.Data.FillOutputRequest.Char.AsciiChar = *((CHAR*)cCharacter);

    Request.Data.FillOutputRequest.Position = dwWriteCoord;
    Request.Data.FillOutputRequest.Length = (WORD)nLength;

    Status = CsrClientCallServer(&Request,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CSR_CONSOLE, FILL_OUTPUT),
                                 sizeof(CSR_API_MESSAGE));

    if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Request.Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    if(lpNumberOfCharsWritten != NULL)
    {
        *lpNumberOfCharsWritten = Request.Data.FillOutputRequest.NrCharactersWritten;
    }

    return TRUE;
}


/* FUNCTIONS ******************************************************************/

/******************
 * Read functions *
 ******************/

/*--------------------------------------------------------------
 *    ReadConsoleW
 *
 * @implemented
 */
BOOL
WINAPI
ReadConsoleW(HANDLE hConsoleInput,
             LPVOID lpBuffer,
             DWORD nNumberOfCharsToRead,
             LPDWORD lpNumberOfCharsRead,
             PCONSOLE_READCONSOLE_CONTROL pInputControl)
{
    return IntReadConsole(hConsoleInput,
                          lpBuffer,
                          nNumberOfCharsToRead,
                          lpNumberOfCharsRead,
                          pInputControl,
                          TRUE);
}


/*--------------------------------------------------------------
 *    ReadConsoleA
 *
 * @implemented
 */
BOOL
WINAPI
ReadConsoleA(HANDLE hConsoleInput,
             LPVOID lpBuffer,
             DWORD nNumberOfCharsToRead,
             LPDWORD lpNumberOfCharsRead,
             PCONSOLE_READCONSOLE_CONTROL pInputControl)
{
    return IntReadConsole(hConsoleInput,
                          lpBuffer,
                          nNumberOfCharsToRead,
                          lpNumberOfCharsRead,
                          NULL,
                          FALSE);
}


/*--------------------------------------------------------------
 *     PeekConsoleInputW
 *
 * @implemented
 */
BOOL
WINAPI
PeekConsoleInputW(HANDLE hConsoleInput,
                  PINPUT_RECORD lpBuffer,
                  DWORD nLength,
                  LPDWORD lpNumberOfEventsRead)
{
    return IntPeekConsoleInput(hConsoleInput,
                               lpBuffer, nLength,
                               lpNumberOfEventsRead,
                               TRUE);
}


/*--------------------------------------------------------------
 *     PeekConsoleInputA
 *
 * @implemented
 */
BOOL
WINAPI
PeekConsoleInputA(HANDLE hConsoleInput,
                  PINPUT_RECORD lpBuffer,
                  DWORD nLength,
                  LPDWORD lpNumberOfEventsRead)
{
    return IntPeekConsoleInput(hConsoleInput,
                               lpBuffer,
                               nLength,
                               lpNumberOfEventsRead,
                               FALSE);
}


/*--------------------------------------------------------------
 *     ReadConsoleInputW
 *
 * @implemented
 */
BOOL
WINAPI
ReadConsoleInputW(HANDLE hConsoleInput,
                  PINPUT_RECORD lpBuffer,
                  DWORD nLength,
                  LPDWORD lpNumberOfEventsRead)
{
    return IntReadConsoleInput(hConsoleInput,
                               lpBuffer,
                               nLength,
                               lpNumberOfEventsRead,
                               TRUE);
}


/*--------------------------------------------------------------
 *     ReadConsoleInputA
 *
 * @implemented
 */
BOOL
WINAPI
ReadConsoleInputA(HANDLE hConsoleInput,
                  PINPUT_RECORD lpBuffer,
                  DWORD nLength,
                  LPDWORD lpNumberOfEventsRead)
{
    return IntReadConsoleInput(hConsoleInput,
                               lpBuffer,
                               nLength,
                               lpNumberOfEventsRead,
                               FALSE);
}


BOOL
WINAPI
ReadConsoleInputExW(HANDLE hConsole, LPVOID lpBuffer, DWORD dwLen, LPDWORD Unknown1, DWORD Unknown2)
{
    STUB;
    return FALSE;
}


BOOL
WINAPI
ReadConsoleInputExA(HANDLE hConsole, LPVOID lpBuffer, DWORD dwLen, LPDWORD Unknown1, DWORD Unknown2)
{
    STUB;
    return FALSE;
}


/*--------------------------------------------------------------
 *     ReadConsoleOutputW
 *
 * @implemented
 */
BOOL
WINAPI
ReadConsoleOutputW(HANDLE hConsoleOutput,
                   PCHAR_INFO lpBuffer,
                   COORD dwBufferSize,
                   COORD dwBufferCoord,
                   PSMALL_RECT lpReadRegion)
{
    return IntReadConsoleOutput(hConsoleOutput,
                                lpBuffer,
                                dwBufferSize,
                                dwBufferCoord,
                                lpReadRegion,
                                TRUE);
}


/*--------------------------------------------------------------
 *     ReadConsoleOutputA
 *
 * @implemented
 */
BOOL
WINAPI
ReadConsoleOutputA(HANDLE hConsoleOutput,
                   PCHAR_INFO lpBuffer,
                   COORD dwBufferSize,
                   COORD dwBufferCoord,
                   PSMALL_RECT lpReadRegion)
{
    return IntReadConsoleOutput(hConsoleOutput,
                                lpBuffer,
                                dwBufferSize,
                                dwBufferCoord,
                                lpReadRegion,
                                FALSE);
}


/*--------------------------------------------------------------
 *      ReadConsoleOutputCharacterW
 *
 * @implemented
 */
BOOL
WINAPI
ReadConsoleOutputCharacterW(HANDLE hConsoleOutput,
                            LPWSTR lpCharacter,
                            DWORD nLength,
                            COORD dwReadCoord,
                            LPDWORD lpNumberOfCharsRead)
{
    return IntReadConsoleOutputCharacter(hConsoleOutput,
                                         (PVOID)lpCharacter,
                                         nLength,
                                         dwReadCoord,
                                         lpNumberOfCharsRead,
                                         TRUE);
}


/*--------------------------------------------------------------
 *     ReadConsoleOutputCharacterA
 *
 * @implemented
 */
BOOL
WINAPI
ReadConsoleOutputCharacterA(HANDLE hConsoleOutput,
                            LPSTR lpCharacter,
                            DWORD nLength,
                            COORD dwReadCoord,
                            LPDWORD lpNumberOfCharsRead)
{
    return IntReadConsoleOutputCharacter(hConsoleOutput,
                                         (PVOID)lpCharacter,
                                         nLength,
                                         dwReadCoord,
                                         lpNumberOfCharsRead,
                                         FALSE);
}


/*--------------------------------------------------------------
 *     ReadConsoleOutputAttribute
 *
 * @implemented
 */
BOOL
WINAPI
ReadConsoleOutputAttribute(HANDLE hConsoleOutput,
                           LPWORD lpAttribute,
                           DWORD nLength,
                           COORD dwReadCoord,
                           LPDWORD lpNumberOfAttrsRead)
{
    PCSR_API_MESSAGE Request;
    ULONG CsrRequest;
    NTSTATUS Status;
    DWORD Size;

    if (lpNumberOfAttrsRead != NULL)
        *lpNumberOfAttrsRead = nLength;

    Request = RtlAllocateHeap(RtlGetProcessHeap(),
                              0,
                              max(sizeof(CSR_API_MESSAGE),
                              CSR_API_MESSAGE_HEADER_SIZE(CSRSS_READ_CONSOLE_OUTPUT_ATTRIB)
                                  + min (nLength, CSRSS_MAX_READ_CONSOLE_OUTPUT_ATTRIB / sizeof(WORD)) * sizeof(WORD)));
    if (Request == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    CsrRequest = CSR_CREATE_API_NUMBER(CSR_CONSOLE, READ_CONSOLE_OUTPUT_ATTRIB);

    while (nLength != 0)
    {
        Request->Data.ReadConsoleOutputAttribRequest.ConsoleHandle = hConsoleOutput;
        Request->Data.ReadConsoleOutputAttribRequest.ReadCoord = dwReadCoord;

        if (nLength > CSRSS_MAX_READ_CONSOLE_OUTPUT_ATTRIB / sizeof(WORD))
            Size = CSRSS_MAX_READ_CONSOLE_OUTPUT_ATTRIB / sizeof(WCHAR);
        else
            Size = nLength;

        Request->Data.ReadConsoleOutputAttribRequest.NumAttrsToRead = Size;

        Status = CsrClientCallServer(Request,
                                     NULL,
                                     CsrRequest,
                                     max(sizeof(CSR_API_MESSAGE),
                                     CSR_API_MESSAGE_HEADER_SIZE(CSRSS_READ_CONSOLE_OUTPUT_ATTRIB) + Size * sizeof(WORD)));
        if (!NT_SUCCESS(Status) || !NT_SUCCESS(Request->Status))
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, Request);
            BaseSetLastNTError(Status);
            return FALSE;
        }

        memcpy(lpAttribute, Request->Data.ReadConsoleOutputAttribRequest.Attribute, Size * sizeof(WORD));
        lpAttribute += Size;
        nLength -= Size;
        Request->Data.ReadConsoleOutputAttribRequest.ReadCoord = Request->Data.ReadConsoleOutputAttribRequest.EndCoord;
    }

    RtlFreeHeap(RtlGetProcessHeap(), 0, Request);

    return TRUE;
}


/*******************
 * Write functions *
 *******************/

/*--------------------------------------------------------------
 *    WriteConsoleW
 *
 * @implemented
 */
BOOL
WINAPI
WriteConsoleW(HANDLE hConsoleOutput,
              CONST VOID *lpBuffer,
              DWORD nNumberOfCharsToWrite,
              LPDWORD lpNumberOfCharsWritten,
              LPVOID lpReserved)
{
    return IntWriteConsole(hConsoleOutput,
                           (PVOID)lpBuffer,
                           nNumberOfCharsToWrite,
                           lpNumberOfCharsWritten,
                           lpReserved,
                           TRUE);
}


/*--------------------------------------------------------------
 *    WriteConsoleA
 *
 * @implemented
 */
BOOL
WINAPI
WriteConsoleA(HANDLE hConsoleOutput,
              CONST VOID *lpBuffer,
              DWORD nNumberOfCharsToWrite,
              LPDWORD lpNumberOfCharsWritten,
              LPVOID lpReserved)
{
    return IntWriteConsole(hConsoleOutput,
                           (PVOID)lpBuffer,
                           nNumberOfCharsToWrite,
                           lpNumberOfCharsWritten,
                           lpReserved,
                           FALSE);
}


/*--------------------------------------------------------------
 *     WriteConsoleInputW
 *
 * @implemented
 */
BOOL
WINAPI
WriteConsoleInputW(HANDLE hConsoleInput,
                   CONST INPUT_RECORD *lpBuffer,
                   DWORD nLength,
                   LPDWORD lpNumberOfEventsWritten)
{
    return IntWriteConsoleInput(hConsoleInput,
                                (PINPUT_RECORD)lpBuffer,
                                nLength,
                                lpNumberOfEventsWritten,
                                TRUE);
}


/*--------------------------------------------------------------
 *     WriteConsoleInputA
 *
 * @implemented
 */
BOOL
WINAPI
WriteConsoleInputA(HANDLE hConsoleInput,
                   CONST INPUT_RECORD *lpBuffer,
                   DWORD nLength,
                   LPDWORD lpNumberOfEventsWritten)
{
    return IntWriteConsoleInput(hConsoleInput,
                                (PINPUT_RECORD)lpBuffer,
                                nLength,
                                lpNumberOfEventsWritten,
                                FALSE);
}


/*--------------------------------------------------------------
 *     WriteConsoleOutputW
 *
 * @implemented
 */
BOOL
WINAPI
WriteConsoleOutputW(HANDLE hConsoleOutput,
                    CONST CHAR_INFO *lpBuffer,
                    COORD dwBufferSize,
                    COORD dwBufferCoord,
                    PSMALL_RECT lpWriteRegion)
{
    return IntWriteConsoleOutput(hConsoleOutput,
                                 lpBuffer,
                                 dwBufferSize,
                                 dwBufferCoord,
                                 lpWriteRegion,
                                 TRUE);
}


/*--------------------------------------------------------------
 *     WriteConsoleOutputA
 *
 * @implemented
 */
BOOL
WINAPI
WriteConsoleOutputA(HANDLE hConsoleOutput,
                    CONST CHAR_INFO *lpBuffer,
                    COORD dwBufferSize,
                    COORD dwBufferCoord,
                    PSMALL_RECT lpWriteRegion)
{
    return IntWriteConsoleOutput(hConsoleOutput,
                                 lpBuffer,
                                 dwBufferSize,
                                 dwBufferCoord,
                                 lpWriteRegion,
                                 FALSE);
}


/*--------------------------------------------------------------
 *     WriteConsoleOutputCharacterW
 *
 * @implemented
 */
BOOL
WINAPI
WriteConsoleOutputCharacterW(HANDLE hConsoleOutput,
                             LPCWSTR lpCharacter,
                             DWORD nLength,
                             COORD dwWriteCoord,
                             LPDWORD lpNumberOfCharsWritten)
{
    return IntWriteConsoleOutputCharacter(hConsoleOutput,
                                          (PVOID)lpCharacter,
                                          nLength,
                                          dwWriteCoord,
                                          lpNumberOfCharsWritten,
                                          TRUE);
}


/*--------------------------------------------------------------
 *     WriteConsoleOutputCharacterA
 *
 * @implemented
 */
BOOL
WINAPI
WriteConsoleOutputCharacterA(HANDLE hConsoleOutput,
                             LPCSTR lpCharacter,
                             DWORD nLength,
                             COORD dwWriteCoord,
                             LPDWORD lpNumberOfCharsWritten)
{
    return IntWriteConsoleOutputCharacter(hConsoleOutput,
                                          (PVOID)lpCharacter,
                                          nLength,
                                          dwWriteCoord,
                                          lpNumberOfCharsWritten,
                                          FALSE);
}


/*--------------------------------------------------------------
 *     WriteConsoleOutputAttribute
 *
 * @implemented
 */
BOOL
WINAPI
WriteConsoleOutputAttribute(HANDLE hConsoleOutput,
                            CONST WORD *lpAttribute,
                            DWORD nLength,
                            COORD dwWriteCoord,
                            LPDWORD lpNumberOfAttrsWritten)
{
    PCSR_API_MESSAGE Request;
    ULONG CsrRequest;
    NTSTATUS Status;
    WORD Size;

    Request = RtlAllocateHeap(RtlGetProcessHeap(),
                              0,
                              max(sizeof(CSR_API_MESSAGE),
                              CSR_API_MESSAGE_HEADER_SIZE(CSRSS_WRITE_CONSOLE_OUTPUT_ATTRIB)
                                + min(nLength, CSRSS_MAX_WRITE_CONSOLE_OUTPUT_ATTRIB / sizeof(WORD)) * sizeof(WORD)));
    if (Request == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    CsrRequest = CSR_CREATE_API_NUMBER(CSR_CONSOLE, WRITE_CONSOLE_OUTPUT_ATTRIB);
    Request->Data.WriteConsoleOutputAttribRequest.Coord = dwWriteCoord;

    if (lpNumberOfAttrsWritten)
        *lpNumberOfAttrsWritten = nLength;
    while (nLength)
    {
        Size = (WORD)min(nLength, CSRSS_MAX_WRITE_CONSOLE_OUTPUT_ATTRIB / sizeof(WORD));
        Request->Data.WriteConsoleOutputAttribRequest.ConsoleHandle = hConsoleOutput;
        Request->Data.WriteConsoleOutputAttribRequest.Length = Size;
        memcpy(Request->Data.WriteConsoleOutputAttribRequest.Attribute, lpAttribute, Size * sizeof(WORD));

        Status = CsrClientCallServer(Request,
                                     NULL,
                                     CsrRequest,
                                     max(sizeof(CSR_API_MESSAGE),
                                     CSR_API_MESSAGE_HEADER_SIZE(CSRSS_WRITE_CONSOLE_OUTPUT_ATTRIB) + Size * sizeof(WORD)));

        if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Request->Status))
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, Request);
            BaseSetLastNTError (Status);
            return FALSE;
        }
        nLength -= Size;
        lpAttribute += Size;
        Request->Data.WriteConsoleOutputAttribRequest.Coord = Request->Data.WriteConsoleOutputAttribRequest.EndCoord;
    }

    RtlFreeHeap(RtlGetProcessHeap(), 0, Request);

    return TRUE;
}


/*--------------------------------------------------------------
 *    FillConsoleOutputCharacterW
 *
 * @implemented
 */
BOOL
WINAPI
FillConsoleOutputCharacterW(HANDLE hConsoleOutput,
                            WCHAR cCharacter,
                            DWORD nLength,
                            COORD dwWriteCoord,
                            LPDWORD lpNumberOfCharsWritten)
{
    return IntFillConsoleOutputCharacter(hConsoleOutput,
                                         &cCharacter,
                                         nLength,
                                         dwWriteCoord,
                                         lpNumberOfCharsWritten,
                                         TRUE);
}


/*--------------------------------------------------------------
 *    FillConsoleOutputCharacterA
 *
 * @implemented
 */
BOOL
WINAPI
FillConsoleOutputCharacterA(HANDLE hConsoleOutput,
                            CHAR cCharacter,
                            DWORD nLength,
                            COORD dwWriteCoord,
                            LPDWORD lpNumberOfCharsWritten)
{
    return IntFillConsoleOutputCharacter(hConsoleOutput,
                                         &cCharacter,
                                         nLength,
                                         dwWriteCoord,
                                         lpNumberOfCharsWritten,
                                         FALSE);
}


/*--------------------------------------------------------------
 *     FillConsoleOutputAttribute
 *
 * @implemented
 */
BOOL
WINAPI
FillConsoleOutputAttribute(HANDLE hConsoleOutput,
                           WORD wAttribute,
                           DWORD nLength,
                           COORD dwWriteCoord,
                           LPDWORD lpNumberOfAttrsWritten)
{
    CSR_API_MESSAGE Request;
    NTSTATUS Status;

    Request.Data.FillOutputAttribRequest.ConsoleHandle = hConsoleOutput;
    Request.Data.FillOutputAttribRequest.Attribute = (CHAR)wAttribute;
    Request.Data.FillOutputAttribRequest.Coord = dwWriteCoord;
    Request.Data.FillOutputAttribRequest.Length = (WORD)nLength;

    Status = CsrClientCallServer(&Request,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CSR_CONSOLE, FILL_OUTPUT_ATTRIB),
                                 sizeof(CSR_API_MESSAGE));
    if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Request.Status))
    {
        BaseSetLastNTError ( Status );
        return FALSE;
    }

    if (lpNumberOfAttrsWritten)
        *lpNumberOfAttrsWritten = nLength;

    return TRUE;
}

/* EOF */
