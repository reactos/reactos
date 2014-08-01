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

DWORD
WINAPI
GetConsoleInputExeNameW(DWORD nBufferLength, LPWSTR lpBuffer);

static
BOOL
IntReadConsole(IN HANDLE hConsoleInput,
               OUT PVOID lpBuffer,
               IN DWORD nNumberOfCharsToRead,
               OUT LPDWORD lpNumberOfCharsRead,
               IN PCONSOLE_READCONSOLE_CONTROL pInputControl OPTIONAL,
               IN BOOLEAN bUnicode)
{
    BOOL Success;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_READCONSOLE ReadConsoleRequest = &ApiMessage.Data.ReadConsoleRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer = NULL;
    ULONG CharSize, SizeBytes;

    DPRINT("IntReadConsole\n");

    /* Set up the data to send to the Console Server */
    ReadConsoleRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    ReadConsoleRequest->InputHandle   = hConsoleInput;
    ReadConsoleRequest->Unicode       = bUnicode;

    /*
     * Retrieve the current console executable name string and length (always
     * in UNICODE format).
     * FIXME: Do not use GetConsoleInputExeNameW but use something else...
     */
    // 1- Get the exe name length in characters, including NULL character.
    ReadConsoleRequest->ExeLength =
        GetConsoleInputExeNameW(0, (PWCHAR)ReadConsoleRequest->StaticBuffer);
    // 2- Get the exe name (GetConsoleInputExeNameW returns 1 in case of success).
    if (GetConsoleInputExeNameW(ReadConsoleRequest->ExeLength,
                                (PWCHAR)ReadConsoleRequest->StaticBuffer) != 1)
    {
        // Nothing
        ReadConsoleRequest->ExeLength = 0;
    }
    else
    {
        // Remove the NULL character, and convert in number of bytes.
        ReadConsoleRequest->ExeLength--;
        ReadConsoleRequest->ExeLength *= sizeof(WCHAR);
    }

    /*** For DEBUGGING purposes ***/
    {
        UNICODE_STRING ExeName;
        ExeName.Length = ExeName.MaximumLength = ReadConsoleRequest->ExeLength;
        ExeName.Buffer = (PWCHAR)ReadConsoleRequest->StaticBuffer;
        DPRINT1("IntReadConsole(ExeName = %wZ)\n", &ExeName);
    }
    /******************************/

    /* Determine the needed size */
    CharSize  = (bUnicode ? sizeof(WCHAR) : sizeof(CHAR));
    SizeBytes = nNumberOfCharsToRead * CharSize;

    ReadConsoleRequest->CaptureBufferSize =
    ReadConsoleRequest->NumBytes          = SizeBytes;

    /*
     * For optimization purposes, Windows (and hence ReactOS, too, for
     * compatibility reasons) uses a static buffer if no more than eighty
     * bytes are read. Otherwise a new buffer is allocated.
     * This behaviour is also expected in the server-side.
     */
    if (SizeBytes <= sizeof(ReadConsoleRequest->StaticBuffer))
    {
        ReadConsoleRequest->Buffer = ReadConsoleRequest->StaticBuffer;
        // CaptureBuffer = NULL;
    }
    else
    {
        /* Allocate a Capture Buffer */
        CaptureBuffer = CsrAllocateCaptureBuffer(1, SizeBytes);
        if (CaptureBuffer == NULL)
        {
            DPRINT1("CsrAllocateCaptureBuffer failed!\n");
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        /* Allocate space in the Buffer */
        CsrAllocateMessagePointer(CaptureBuffer,
                                  SizeBytes,
                                  (PVOID*)&ReadConsoleRequest->Buffer);
    }

    ReadConsoleRequest->InitialNumBytes = 0;
    ReadConsoleRequest->CtrlWakeupMask  = 0;
    ReadConsoleRequest->ControlKeyState = 0;

    /*
     * From MSDN (ReadConsole function), the description
     * for pInputControl says:
     * "This parameter requires Unicode input by default.
     * For ANSI mode, set this parameter to NULL."
     */
    if (bUnicode && pInputControl &&
        pInputControl->nLength == sizeof(CONSOLE_READCONSOLE_CONTROL))
    {
        /* Sanity check */
        if (pInputControl->nInitialChars <= nNumberOfCharsToRead)
        {
            ReadConsoleRequest->InitialNumBytes =
                pInputControl->nInitialChars * sizeof(WCHAR); // CharSize

            if (pInputControl->nInitialChars != 0)
            {
                /*
                 * It is possible here to overwrite the static buffer, in case
                 * the number of bytes to read was smaller than the static buffer.
                 * In this case, this means we are continuing a pending read,
                 * and we do not need in fact the executable name that was
                 * stored in the static buffer because it was first grabbed when
                 * we started the first read.
                 */
                RtlCopyMemory(ReadConsoleRequest->Buffer,
                              lpBuffer,
                              ReadConsoleRequest->InitialNumBytes);
            }

            ReadConsoleRequest->CtrlWakeupMask = pInputControl->dwCtrlWakeupMask;
        }
        else
        {
            // Status = STATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        /* We are in a situation where pInputControl has no meaning */
        pInputControl = NULL;
    }

    /* Check for sanity */
/*
    if (!NT_SUCCESS(Status) && pInputControl)
    {
        // Free CaptureBuffer if needed
        // Set last error to last status
        // Return FALSE
    }
*/

    /* Call the server */
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepReadConsole),
                        sizeof(*ReadConsoleRequest));

    /* Check for success */
    Success = NT_SUCCESS(ApiMessage.Status);

    /* Retrieve the results */
    if (Success)
    {
        _SEH2_TRY
        {
            *lpNumberOfCharsRead = ReadConsoleRequest->NumBytes / CharSize;

            if (bUnicode && pInputControl)
                pInputControl->dwControlKeyState = ReadConsoleRequest->ControlKeyState;

            RtlCopyMemory(lpBuffer,
                          ReadConsoleRequest->Buffer,
                          ReadConsoleRequest->NumBytes);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            SetLastError(ERROR_INVALID_ACCESS);
            Success = FALSE;
        }
        _SEH2_END;
    }
    else
    {
        BaseSetLastNTError(ApiMessage.Status);
    }

    /* Release the capture buffer if needed */
    if (CaptureBuffer) CsrFreeCaptureBuffer(CaptureBuffer);

    if (Success)
    {
        /* Yield execution to another thread if Ctrl-C or Ctrl-Break happened */
        if (ApiMessage.Status == STATUS_ALERTED /* || ApiMessage.Status == STATUS_CANCELLED */)
        {
            NtYieldExecution();
            SetLastError(ERROR_OPERATION_ABORTED); // STATUS_CANCELLED
        }
    }

    /* Return success status */
    return Success;
}


static
BOOL
IntGetConsoleInput(IN HANDLE hConsoleInput,
                   OUT PINPUT_RECORD lpBuffer,
                   IN DWORD nLength,
                   OUT LPDWORD lpNumberOfEventsRead,
                   IN WORD wFlags,
                   IN BOOLEAN bUnicode)
{
    BOOL Success;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETINPUT GetInputRequest = &ApiMessage.Data.GetInputRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer = NULL;

    if (!IsConsoleHandle(hConsoleInput))
    {
        _SEH2_TRY
        {
            *lpNumberOfEventsRead = 0;
            SetLastError(ERROR_INVALID_HANDLE);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            SetLastError(ERROR_INVALID_ACCESS);
        }
        _SEH2_END;

        return FALSE;
    }

    DPRINT("IntGetConsoleInput: %lx %p\n", nLength, lpNumberOfEventsRead);

    /* Set up the data to send to the Console Server */
    GetInputRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    GetInputRequest->InputHandle   = hConsoleInput;
    GetInputRequest->NumRecords    = nLength;
    GetInputRequest->Flags         = wFlags;
    GetInputRequest->Unicode       = bUnicode;

    /*
     * For optimization purposes, Windows (and hence ReactOS, too, for
     * compatibility reasons) uses a static buffer if no more than five
     * input records are read. Otherwise a new buffer is allocated.
     * This behaviour is also expected in the server-side.
     */
    if (nLength <= sizeof(GetInputRequest->RecordStaticBuffer)/sizeof(INPUT_RECORD))
    {
        GetInputRequest->RecordBufPtr = GetInputRequest->RecordStaticBuffer;
        // CaptureBuffer = NULL;
    }
    else
    {
        ULONG Size = nLength * sizeof(INPUT_RECORD);

        /* Allocate a Capture Buffer */
        CaptureBuffer = CsrAllocateCaptureBuffer(1, Size);
        if (CaptureBuffer == NULL)
        {
            DPRINT1("CsrAllocateCaptureBuffer failed!\n");
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        /* Allocate space in the Buffer */
        CsrAllocateMessagePointer(CaptureBuffer,
                                  Size,
                                  (PVOID*)&GetInputRequest->RecordBufPtr);
    }

    /* Call the server */
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetConsoleInput),
                        sizeof(*GetInputRequest));

    /* Check for success */
    Success = NT_SUCCESS(ApiMessage.Status);

    /* Retrieve the results */
    _SEH2_TRY
    {
        DPRINT("Events read: %lx\n", GetInputRequest->NumRecords);
        *lpNumberOfEventsRead = GetInputRequest->NumRecords;

        if (Success)
        {
            RtlCopyMemory(lpBuffer,
                          GetInputRequest->RecordBufPtr,
                          GetInputRequest->NumRecords * sizeof(INPUT_RECORD));
        }
        else
        {
            BaseSetLastNTError(ApiMessage.Status);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(ERROR_INVALID_ACCESS);
        Success = FALSE;
    }
    _SEH2_END;

    /* Release the capture buffer if needed */
    if (CaptureBuffer) CsrFreeCaptureBuffer(CaptureBuffer);

    /* Return success status */
    return Success;
}


static
BOOL
IntReadConsoleOutput(IN HANDLE hConsoleOutput,
                     OUT PCHAR_INFO lpBuffer,
                     IN COORD dwBufferSize,
                     IN COORD dwBufferCoord,
                     IN OUT PSMALL_RECT lpReadRegion,
                     IN BOOLEAN bUnicode)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_READOUTPUT ReadOutputRequest = &ApiMessage.Data.ReadOutputRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    DWORD Size, SizeX, SizeY;

    if (lpBuffer == NULL)
    {
        SetLastError(ERROR_INVALID_ACCESS);
        return FALSE;
    }

    Size = dwBufferSize.X * dwBufferSize.Y * sizeof(CHAR_INFO);

    DPRINT("IntReadConsoleOutput: %lx %p\n", Size, lpReadRegion);

    /* Allocate a Capture Buffer */
    CaptureBuffer = CsrAllocateCaptureBuffer(1, Size);
    if (CaptureBuffer == NULL)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed with size 0x%x!\n", Size);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Allocate space in the Buffer */
    CsrAllocateMessagePointer(CaptureBuffer,
                              Size,
                              (PVOID*)&ReadOutputRequest->CharInfo);

    /* Set up the data to send to the Console Server */
    ReadOutputRequest->OutputHandle = hConsoleOutput;
    ReadOutputRequest->Unicode = bUnicode;
    ReadOutputRequest->BufferSize = dwBufferSize;
    ReadOutputRequest->BufferCoord = dwBufferCoord;
    ReadOutputRequest->ReadRegion = *lpReadRegion;

    /* Call the server */
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepReadConsoleOutput),
                        sizeof(*ReadOutputRequest));

    /* Check for success */
    if (NT_SUCCESS(ApiMessage.Status))
    {
        /* Copy into the buffer */
        DPRINT("Copying to buffer\n");
        SizeX = ReadOutputRequest->ReadRegion.Right -
                ReadOutputRequest->ReadRegion.Left + 1;
        SizeY = ReadOutputRequest->ReadRegion.Bottom -
                ReadOutputRequest->ReadRegion.Top + 1;
        RtlCopyMemory(lpBuffer,
                      ReadOutputRequest->CharInfo,
                      sizeof(CHAR_INFO) * SizeX * SizeY);
    }
    else
    {
        BaseSetLastNTError(ApiMessage.Status);
    }

    /* Return the read region */
    DPRINT("read region: %p\n", ReadOutputRequest->ReadRegion);
    *lpReadRegion = ReadOutputRequest->ReadRegion;

    /* Release the capture buffer */
    CsrFreeCaptureBuffer(CaptureBuffer);

    /* Return TRUE or FALSE */
    return NT_SUCCESS(ApiMessage.Status);
}


static
BOOL
IntReadConsoleOutputCode(IN HANDLE hConsoleOutput,
                         IN CODE_TYPE CodeType,
                         OUT PVOID pCode,
                         IN DWORD nLength,
                         IN COORD dwReadCoord,
                         OUT LPDWORD lpNumberOfCodesRead)
{
    BOOL Success;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_READOUTPUTCODE ReadOutputCodeRequest = &ApiMessage.Data.ReadOutputCodeRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer = NULL;
    ULONG CodeSize, SizeBytes;

    DPRINT("IntReadConsoleOutputCode\n");

    if ( (CodeType != CODE_ASCII    ) &&
         (CodeType != CODE_UNICODE  ) &&
         (CodeType != CODE_ATTRIBUTE) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Set up the data to send to the Console Server */
    ReadOutputCodeRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    ReadOutputCodeRequest->OutputHandle  = hConsoleOutput;
    ReadOutputCodeRequest->Coord         = dwReadCoord;
    ReadOutputCodeRequest->NumCodes      = nLength;

    /* Determine the needed size */
    ReadOutputCodeRequest->CodeType = CodeType;
    switch (CodeType)
    {
        case CODE_ASCII:
            CodeSize = RTL_FIELD_SIZE(CODE_ELEMENT, AsciiChar);
            break;

        case CODE_UNICODE:
            CodeSize = RTL_FIELD_SIZE(CODE_ELEMENT, UnicodeChar);
            break;

        case CODE_ATTRIBUTE:
            CodeSize = RTL_FIELD_SIZE(CODE_ELEMENT, Attribute);
            break;
    }
    SizeBytes = nLength * CodeSize;

    /*
     * For optimization purposes, Windows (and hence ReactOS, too, for
     * compatibility reasons) uses a static buffer if no more than eighty
     * bytes are read. Otherwise a new buffer is allocated.
     * This behaviour is also expected in the server-side.
     */
    if (SizeBytes <= sizeof(ReadOutputCodeRequest->CodeStaticBuffer))
    {
        ReadOutputCodeRequest->pCode = ReadOutputCodeRequest->CodeStaticBuffer;
        // CaptureBuffer = NULL;
    }
    else
    {
        /* Allocate a Capture Buffer */
        CaptureBuffer = CsrAllocateCaptureBuffer(1, SizeBytes);
        if (CaptureBuffer == NULL)
        {
            DPRINT1("CsrAllocateCaptureBuffer failed!\n");
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        /* Allocate space in the Buffer */
        CsrAllocateMessagePointer(CaptureBuffer,
                                  SizeBytes,
                                  (PVOID*)&ReadOutputCodeRequest->pCode);
    }

    /* Call the server */
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepReadConsoleOutputString),
                        sizeof(*ReadOutputCodeRequest));

    /* Check for success */
    Success = NT_SUCCESS(ApiMessage.Status);

    /* Retrieve the results */
    _SEH2_TRY
    {
        *lpNumberOfCodesRead = ReadOutputCodeRequest->NumCodes;

        if (Success)
        {
            RtlCopyMemory(pCode,
                          ReadOutputCodeRequest->pCode,
                          ReadOutputCodeRequest->NumCodes * CodeSize);
        }
        else
        {
            BaseSetLastNTError(ApiMessage.Status);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(ERROR_INVALID_ACCESS);
        Success = FALSE;
    }
    _SEH2_END;

    /* Release the capture buffer if needed */
    if (CaptureBuffer) CsrFreeCaptureBuffer(CaptureBuffer);

    /* Return success status */
    return Success;
}


/*******************
 * Write functions *
 *******************/

static
BOOL
IntWriteConsole(IN HANDLE hConsoleOutput,
                IN PVOID lpBuffer,
                IN DWORD nNumberOfCharsToWrite,
                OUT LPDWORD lpNumberOfCharsWritten,
                LPVOID lpReserved,
                IN BOOLEAN bUnicode)
{
    BOOL Success;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_WRITECONSOLE WriteConsoleRequest = &ApiMessage.Data.WriteConsoleRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer = NULL;
    ULONG CharSize, SizeBytes;

    DPRINT("IntWriteConsole\n");

    /* Set up the data to send to the Console Server */
    WriteConsoleRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    WriteConsoleRequest->OutputHandle  = hConsoleOutput;
    WriteConsoleRequest->Unicode       = bUnicode;

    /* Those members are unused by the client, on Windows */
    WriteConsoleRequest->Reserved1 = 0;
    // WriteConsoleRequest->Reserved2 = {0};

    /* Determine the needed size */
    CharSize  = (bUnicode ? sizeof(WCHAR) : sizeof(CHAR));
    SizeBytes = nNumberOfCharsToWrite * CharSize;

    WriteConsoleRequest->NumBytes = SizeBytes;

    /*
     * For optimization purposes, Windows (and hence ReactOS, too, for
     * compatibility reasons) uses a static buffer if no more than eighty
     * bytes are written. Otherwise a new buffer is allocated.
     * This behaviour is also expected in the server-side.
     */
    if (SizeBytes <= sizeof(WriteConsoleRequest->StaticBuffer))
    {
        WriteConsoleRequest->Buffer = WriteConsoleRequest->StaticBuffer;
        // CaptureBuffer = NULL;
        WriteConsoleRequest->UsingStaticBuffer = TRUE;

        _SEH2_TRY
        {
            RtlCopyMemory(WriteConsoleRequest->Buffer,
                          lpBuffer,
                          SizeBytes);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            SetLastError(ERROR_INVALID_ACCESS);
            return FALSE;
        }
        _SEH2_END;
    }
    else
    {
        /* Allocate a Capture Buffer */
        CaptureBuffer = CsrAllocateCaptureBuffer(1, SizeBytes);
        if (CaptureBuffer == NULL)
        {
            DPRINT1("CsrAllocateCaptureBuffer failed!\n");
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        /* Capture the buffer to write */
        CsrCaptureMessageBuffer(CaptureBuffer,
                                (PVOID)lpBuffer,
                                SizeBytes,
                                (PVOID*)&WriteConsoleRequest->Buffer);
        WriteConsoleRequest->UsingStaticBuffer = FALSE;
    }

    /* Call the server */
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepWriteConsole),
                        sizeof(*WriteConsoleRequest));

    /* Check for success */
    Success = NT_SUCCESS(ApiMessage.Status);

    /* Release the capture buffer if needed */
    if (CaptureBuffer) CsrFreeCaptureBuffer(CaptureBuffer);

    /* Retrieve the results */
    if (Success)
    {
        _SEH2_TRY
        {
            *lpNumberOfCharsWritten = WriteConsoleRequest->NumBytes / CharSize;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            SetLastError(ERROR_INVALID_ACCESS);
            Success = FALSE;
        }
        _SEH2_END;
    }
    else
    {
        BaseSetLastNTError(ApiMessage.Status);
    }

    /* Return success status */
    return Success;
}


static
BOOL
IntWriteConsoleInput(IN HANDLE hConsoleInput,
                     IN PINPUT_RECORD lpBuffer,
                     IN DWORD nLength,
                     OUT LPDWORD lpNumberOfEventsWritten,
                     IN BOOLEAN bUnicode,
                     IN BOOLEAN bAppendToEnd)
{
    BOOL Success;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_WRITEINPUT WriteInputRequest = &ApiMessage.Data.WriteInputRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer = NULL;

    DPRINT("IntWriteConsoleInput: %lx %p\n", nLength, lpNumberOfEventsWritten);

    /* Set up the data to send to the Console Server */
    WriteInputRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    WriteInputRequest->InputHandle   = hConsoleInput;
    WriteInputRequest->NumRecords    = nLength;
    WriteInputRequest->Unicode       = bUnicode;
    WriteInputRequest->AppendToEnd   = bAppendToEnd;

    /*
     * For optimization purposes, Windows (and hence ReactOS, too, for
     * compatibility reasons) uses a static buffer if no more than five
     * input records are written. Otherwise a new buffer is allocated.
     * This behaviour is also expected in the server-side.
     */
    if (nLength <= sizeof(WriteInputRequest->RecordStaticBuffer)/sizeof(INPUT_RECORD))
    {
        WriteInputRequest->RecordBufPtr = WriteInputRequest->RecordStaticBuffer;
        // CaptureBuffer = NULL;

        _SEH2_TRY
        {
            RtlCopyMemory(WriteInputRequest->RecordBufPtr,
                          lpBuffer,
                          nLength * sizeof(INPUT_RECORD));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            SetLastError(ERROR_INVALID_ACCESS);
            return FALSE;
        }
        _SEH2_END;
    }
    else
    {
        ULONG Size = nLength * sizeof(INPUT_RECORD);

        /* Allocate a Capture Buffer */
        CaptureBuffer = CsrAllocateCaptureBuffer(1, Size);
        if (CaptureBuffer == NULL)
        {
            DPRINT1("CsrAllocateCaptureBuffer failed!\n");
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        /* Capture the user buffer */
        CsrCaptureMessageBuffer(CaptureBuffer,
                                lpBuffer,
                                Size,
                                (PVOID*)&WriteInputRequest->RecordBufPtr);
    }

    /* Call the server */
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepWriteConsoleInput),
                        sizeof(*WriteInputRequest));

    /* Check for success */
    Success = NT_SUCCESS(ApiMessage.Status);

    /* Release the capture buffer if needed */
    if (CaptureBuffer) CsrFreeCaptureBuffer(CaptureBuffer);

    /* Retrieve the results */
    _SEH2_TRY
    {
        DPRINT("Events written: %lx\n", WriteInputRequest->NumRecords);
        *lpNumberOfEventsWritten = WriteInputRequest->NumRecords;

        if (!Success)
            BaseSetLastNTError(ApiMessage.Status);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(ERROR_INVALID_ACCESS);
        Success = FALSE;
    }
    _SEH2_END;

    /* Return success status */
    return Success;
}


static
BOOL
IntWriteConsoleOutput(IN HANDLE hConsoleOutput,
                      IN CONST CHAR_INFO *lpBuffer,
                      IN COORD dwBufferSize,
                      IN COORD dwBufferCoord,
                      IN OUT PSMALL_RECT lpWriteRegion,
                      IN BOOLEAN bUnicode)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_WRITEOUTPUT WriteOutputRequest = &ApiMessage.Data.WriteOutputRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    ULONG Size;

    if ((lpBuffer == NULL) || (lpWriteRegion == NULL))
    {
        SetLastError(ERROR_INVALID_ACCESS);
        return FALSE;
    }
    /*
    if (lpWriteRegion == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    */

    Size = dwBufferSize.Y * dwBufferSize.X * sizeof(CHAR_INFO);

    DPRINT("IntWriteConsoleOutput: %lx %p\n", Size, lpWriteRegion);

    /* Allocate a Capture Buffer */
    CaptureBuffer = CsrAllocateCaptureBuffer(1, Size);
    if (CaptureBuffer == NULL)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Capture the user buffer */
    CsrCaptureMessageBuffer(CaptureBuffer,
                            (PVOID)lpBuffer,
                            Size,
                            (PVOID*)&WriteOutputRequest->CharInfo);

    /* Set up the data to send to the Console Server */
    WriteOutputRequest->OutputHandle = hConsoleOutput;
    WriteOutputRequest->Unicode = bUnicode;
    WriteOutputRequest->BufferSize = dwBufferSize;
    WriteOutputRequest->BufferCoord = dwBufferCoord;
    WriteOutputRequest->WriteRegion = *lpWriteRegion;

    /* Call the server */
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepWriteConsoleOutput),
                        sizeof(*WriteOutputRequest));

    /* Check for success */
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
    }

    /* Return the read region */
    DPRINT("read region: %p\n", WriteOutputRequest->WriteRegion);
    *lpWriteRegion = WriteOutputRequest->WriteRegion;

    /* Release the capture buffer */
    CsrFreeCaptureBuffer(CaptureBuffer);

    /* Return TRUE or FALSE */
    return NT_SUCCESS(ApiMessage.Status);
}


static
BOOL
IntWriteConsoleOutputCode(IN HANDLE hConsoleOutput,
                          IN CODE_TYPE CodeType,
                          IN CONST VOID *pCode,
                          IN DWORD nLength,
                          IN COORD dwWriteCoord,
                          OUT LPDWORD lpNumberOfCodesWritten)
{
    BOOL Success;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_WRITEOUTPUTCODE WriteOutputCodeRequest = &ApiMessage.Data.WriteOutputCodeRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer = NULL;
    ULONG CodeSize, SizeBytes;

    if ( (CodeType != CODE_ASCII    ) &&
         (CodeType != CODE_UNICODE  ) &&
         (CodeType != CODE_ATTRIBUTE) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    DPRINT("IntWriteConsoleOutputCode\n");

    /* Set up the data to send to the Console Server */
    WriteOutputCodeRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    WriteOutputCodeRequest->OutputHandle  = hConsoleOutput;
    WriteOutputCodeRequest->Coord         = dwWriteCoord;
    WriteOutputCodeRequest->NumCodes      = nLength;

    /* Determine the needed size */
    WriteOutputCodeRequest->CodeType = CodeType;
    switch (CodeType)
    {
        case CODE_ASCII:
            CodeSize = RTL_FIELD_SIZE(CODE_ELEMENT, AsciiChar);
            break;

        case CODE_UNICODE:
            CodeSize = RTL_FIELD_SIZE(CODE_ELEMENT, UnicodeChar);
            break;

        case CODE_ATTRIBUTE:
            CodeSize = RTL_FIELD_SIZE(CODE_ELEMENT, Attribute);
            break;
    }
    SizeBytes = nLength * CodeSize;

    /*
     * For optimization purposes, Windows (and hence ReactOS, too, for
     * compatibility reasons) uses a static buffer if no more than eighty
     * bytes are written. Otherwise a new buffer is allocated.
     * This behaviour is also expected in the server-side.
     */
    if (SizeBytes <= sizeof(WriteOutputCodeRequest->CodeStaticBuffer))
    {
        WriteOutputCodeRequest->pCode = WriteOutputCodeRequest->CodeStaticBuffer;
        // CaptureBuffer = NULL;

        _SEH2_TRY
        {
            RtlCopyMemory(WriteOutputCodeRequest->pCode,
                          pCode,
                          SizeBytes);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            SetLastError(ERROR_INVALID_ACCESS);
            return FALSE;
        }
        _SEH2_END;
    }
    else
    {
        /* Allocate a Capture Buffer */
        CaptureBuffer = CsrAllocateCaptureBuffer(1, SizeBytes);
        if (CaptureBuffer == NULL)
        {
            DPRINT1("CsrAllocateCaptureBuffer failed!\n");
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        /* Capture the buffer to write */
        CsrCaptureMessageBuffer(CaptureBuffer,
                                (PVOID)pCode,
                                SizeBytes,
                                (PVOID*)&WriteOutputCodeRequest->pCode);
    }

    /* Call the server */
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepWriteConsoleOutputString),
                        sizeof(*WriteOutputCodeRequest));

    /* Check for success */
    Success = NT_SUCCESS(ApiMessage.Status);

    /* Release the capture buffer if needed */
    if (CaptureBuffer) CsrFreeCaptureBuffer(CaptureBuffer);

    /* Retrieve the results */
    _SEH2_TRY
    {
        *lpNumberOfCodesWritten = WriteOutputCodeRequest->NumCodes;

        if (!Success)
            BaseSetLastNTError(ApiMessage.Status);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(ERROR_INVALID_ACCESS);
        Success = FALSE;
    }
    _SEH2_END;

    /* Return success status */
    return Success;
}


static
BOOL
IntFillConsoleOutputCode(IN HANDLE hConsoleOutput,
                         IN CODE_TYPE CodeType,
                         IN CODE_ELEMENT Code,
                         IN DWORD nLength,
                         IN COORD dwWriteCoord,
                         OUT LPDWORD lpNumberOfCodesWritten)
{
    BOOL Success;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_FILLOUTPUTCODE FillOutputRequest = &ApiMessage.Data.FillOutputRequest;

    DPRINT("IntFillConsoleOutputCode\n");

    if ( (CodeType != CODE_ASCII    ) &&
         (CodeType != CODE_UNICODE  ) &&
         (CodeType != CODE_ATTRIBUTE) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Set up the data to send to the Console Server */
    FillOutputRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    FillOutputRequest->OutputHandle  = hConsoleOutput;
    FillOutputRequest->WriteCoord    = dwWriteCoord;
    FillOutputRequest->CodeType = CodeType;
    FillOutputRequest->Code     = Code;
    FillOutputRequest->NumCodes = nLength;

    /* Call the server */
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepFillConsoleOutput),
                        sizeof(*FillOutputRequest));

    /* Check for success */
    Success = NT_SUCCESS(ApiMessage.Status);

    /* Retrieve the results */
    _SEH2_TRY
    {
        *lpNumberOfCodesWritten = FillOutputRequest->NumCodes;

        if (!Success)
            BaseSetLastNTError(ApiMessage.Status);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(ERROR_INVALID_ACCESS);
        Success = FALSE;
    }
    _SEH2_END;

    /* Return success status */
    return Success;
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
ReadConsoleW(IN HANDLE hConsoleInput,
             OUT LPVOID lpBuffer,
             IN DWORD nNumberOfCharsToRead,
             OUT LPDWORD lpNumberOfCharsRead,
             IN PCONSOLE_READCONSOLE_CONTROL pInputControl OPTIONAL)
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
ReadConsoleA(IN HANDLE hConsoleInput,
             OUT LPVOID lpBuffer,
             IN DWORD nNumberOfCharsToRead,
             OUT LPDWORD lpNumberOfCharsRead,
             IN PCONSOLE_READCONSOLE_CONTROL pInputControl OPTIONAL)
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
PeekConsoleInputW(IN HANDLE hConsoleInput,
                  OUT PINPUT_RECORD lpBuffer,
                  IN DWORD nLength,
                  OUT LPDWORD lpNumberOfEventsRead)
{
    return IntGetConsoleInput(hConsoleInput,
                              lpBuffer,
                              nLength,
                              lpNumberOfEventsRead,
                              CONSOLE_READ_KEEPEVENT | CONSOLE_READ_CONTINUE,
                              TRUE);
}


/*--------------------------------------------------------------
 *     PeekConsoleInputA
 *
 * @implemented
 */
BOOL
WINAPI
PeekConsoleInputA(IN HANDLE hConsoleInput,
                  OUT PINPUT_RECORD lpBuffer,
                  IN DWORD nLength,
                  OUT LPDWORD lpNumberOfEventsRead)
{
    return IntGetConsoleInput(hConsoleInput,
                              lpBuffer,
                              nLength,
                              lpNumberOfEventsRead,
                              CONSOLE_READ_KEEPEVENT | CONSOLE_READ_CONTINUE,
                              FALSE);
}


/*--------------------------------------------------------------
 *     ReadConsoleInputW
 *
 * @implemented
 */
BOOL
WINAPI
ReadConsoleInputW(IN HANDLE hConsoleInput,
                  OUT PINPUT_RECORD lpBuffer,
                  IN DWORD nLength,
                  OUT LPDWORD lpNumberOfEventsRead)
{
    return IntGetConsoleInput(hConsoleInput,
                              lpBuffer,
                              nLength,
                              lpNumberOfEventsRead,
                              0,
                              TRUE);
}


/*--------------------------------------------------------------
 *     ReadConsoleInputA
 *
 * @implemented
 */
BOOL
WINAPI
ReadConsoleInputA(IN HANDLE hConsoleInput,
                  OUT PINPUT_RECORD lpBuffer,
                  IN DWORD nLength,
                  OUT LPDWORD lpNumberOfEventsRead)
{
    return IntGetConsoleInput(hConsoleInput,
                              lpBuffer,
                              nLength,
                              lpNumberOfEventsRead,
                              0,
                              FALSE);
}


/*--------------------------------------------------------------
 *     ReadConsoleInputExW
 *
 * @implemented
 */
BOOL
WINAPI
ReadConsoleInputExW(IN HANDLE hConsoleInput,
                    OUT PINPUT_RECORD lpBuffer,
                    IN DWORD nLength,
                    OUT LPDWORD lpNumberOfEventsRead,
                    IN WORD wFlags)
{
    return IntGetConsoleInput(hConsoleInput,
                              lpBuffer,
                              nLength,
                              lpNumberOfEventsRead,
                              wFlags,
                              TRUE);
}


/*--------------------------------------------------------------
 *     ReadConsoleInputExA
 *
 * @implemented
 */
BOOL
WINAPI
ReadConsoleInputExA(IN HANDLE hConsoleInput,
                    OUT PINPUT_RECORD lpBuffer,
                    IN DWORD nLength,
                    OUT LPDWORD lpNumberOfEventsRead,
                    IN WORD wFlags)
{
    return IntGetConsoleInput(hConsoleInput,
                              lpBuffer,
                              nLength,
                              lpNumberOfEventsRead,
                              wFlags,
                              FALSE);
}


/*--------------------------------------------------------------
 *     ReadConsoleOutputW
 *
 * @implemented
 */
BOOL
WINAPI
ReadConsoleOutputW(IN HANDLE hConsoleOutput,
                   OUT PCHAR_INFO lpBuffer,
                   IN COORD dwBufferSize,
                   IN COORD dwBufferCoord,
                   IN OUT PSMALL_RECT lpReadRegion)
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
ReadConsoleOutputA(IN HANDLE hConsoleOutput,
                   OUT PCHAR_INFO lpBuffer,
                   IN COORD dwBufferSize,
                   IN COORD dwBufferCoord,
                   IN OUT PSMALL_RECT lpReadRegion)
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
ReadConsoleOutputCharacterW(IN HANDLE hConsoleOutput,
                            OUT LPWSTR lpCharacter,
                            IN DWORD nLength,
                            IN COORD dwReadCoord,
                            OUT LPDWORD lpNumberOfCharsRead)
{
    return IntReadConsoleOutputCode(hConsoleOutput,
                                    CODE_UNICODE,
                                    lpCharacter,
                                    nLength,
                                    dwReadCoord,
                                    lpNumberOfCharsRead);
}


/*--------------------------------------------------------------
 *     ReadConsoleOutputCharacterA
 *
 * @implemented
 */
BOOL
WINAPI
ReadConsoleOutputCharacterA(IN HANDLE hConsoleOutput,
                            OUT LPSTR lpCharacter,
                            IN DWORD nLength,
                            IN COORD dwReadCoord,
                            OUT LPDWORD lpNumberOfCharsRead)
{
    return IntReadConsoleOutputCode(hConsoleOutput,
                                    CODE_ASCII,
                                    lpCharacter,
                                    nLength,
                                    dwReadCoord,
                                    lpNumberOfCharsRead);
}


/*--------------------------------------------------------------
 *     ReadConsoleOutputAttribute
 *
 * @implemented
 */
BOOL
WINAPI
ReadConsoleOutputAttribute(IN HANDLE hConsoleOutput,
                           OUT LPWORD lpAttribute,
                           IN DWORD nLength,
                           IN COORD dwReadCoord,
                           OUT LPDWORD lpNumberOfAttrsRead)
{
    return IntReadConsoleOutputCode(hConsoleOutput,
                                    CODE_ATTRIBUTE,
                                    lpAttribute,
                                    nLength,
                                    dwReadCoord,
                                    lpNumberOfAttrsRead);
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
WriteConsoleW(IN HANDLE hConsoleOutput,
              IN CONST VOID *lpBuffer,
              IN DWORD nNumberOfCharsToWrite,
              OUT LPDWORD lpNumberOfCharsWritten,
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
WriteConsoleA(IN HANDLE hConsoleOutput,
              IN CONST VOID *lpBuffer,
              IN DWORD nNumberOfCharsToWrite,
              OUT LPDWORD lpNumberOfCharsWritten,
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
WriteConsoleInputW(IN HANDLE hConsoleInput,
                   IN CONST INPUT_RECORD *lpBuffer,
                   IN DWORD nLength,
                   OUT LPDWORD lpNumberOfEventsWritten)
{
    return IntWriteConsoleInput(hConsoleInput,
                                (PINPUT_RECORD)lpBuffer,
                                nLength,
                                lpNumberOfEventsWritten,
                                TRUE,
                                TRUE);
}


/*--------------------------------------------------------------
 *     WriteConsoleInputA
 *
 * @implemented
 */
BOOL
WINAPI
WriteConsoleInputA(IN HANDLE hConsoleInput,
                   IN CONST INPUT_RECORD *lpBuffer,
                   IN DWORD nLength,
                   OUT LPDWORD lpNumberOfEventsWritten)
{
    return IntWriteConsoleInput(hConsoleInput,
                                (PINPUT_RECORD)lpBuffer,
                                nLength,
                                lpNumberOfEventsWritten,
                                FALSE,
                                TRUE);
}


/*--------------------------------------------------------------
 *     WriteConsoleInputVDMW
 *
 * @implemented
 */
BOOL
WINAPI
WriteConsoleInputVDMW(IN HANDLE hConsoleInput,
                      IN CONST INPUT_RECORD *lpBuffer,
                      IN DWORD nLength,
                      OUT LPDWORD lpNumberOfEventsWritten)
{
    return IntWriteConsoleInput(hConsoleInput,
                                (PINPUT_RECORD)lpBuffer,
                                nLength,
                                lpNumberOfEventsWritten,
                                TRUE,
                                FALSE);
}


/*--------------------------------------------------------------
 *     WriteConsoleInputVDMA
 *
 * @implemented
 */
BOOL
WINAPI
WriteConsoleInputVDMA(IN HANDLE hConsoleInput,
                      IN CONST INPUT_RECORD *lpBuffer,
                      IN DWORD nLength,
                      OUT LPDWORD lpNumberOfEventsWritten)
{
    return IntWriteConsoleInput(hConsoleInput,
                                (PINPUT_RECORD)lpBuffer,
                                nLength,
                                lpNumberOfEventsWritten,
                                FALSE,
                                FALSE);
}


/*--------------------------------------------------------------
 *     WriteConsoleOutputW
 *
 * @implemented
 */
BOOL
WINAPI
WriteConsoleOutputW(IN HANDLE hConsoleOutput,
                    IN CONST CHAR_INFO *lpBuffer,
                    IN COORD dwBufferSize,
                    IN COORD dwBufferCoord,
                    IN OUT PSMALL_RECT lpWriteRegion)
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
WriteConsoleOutputA(IN HANDLE hConsoleOutput,
                    IN CONST CHAR_INFO *lpBuffer,
                    IN COORD dwBufferSize,
                    IN COORD dwBufferCoord,
                    IN OUT PSMALL_RECT lpWriteRegion)
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
WriteConsoleOutputCharacterW(IN HANDLE hConsoleOutput,
                             IN LPCWSTR lpCharacter,
                             IN DWORD nLength,
                             IN COORD dwWriteCoord,
                             OUT LPDWORD lpNumberOfCharsWritten)
{
    return IntWriteConsoleOutputCode(hConsoleOutput,
                                     CODE_UNICODE,
                                     lpCharacter,
                                     nLength,
                                     dwWriteCoord,
                                     lpNumberOfCharsWritten);
}


/*--------------------------------------------------------------
 *     WriteConsoleOutputCharacterA
 *
 * @implemented
 */
BOOL
WINAPI
WriteConsoleOutputCharacterA(IN HANDLE hConsoleOutput,
                             IN LPCSTR lpCharacter,
                             IN DWORD nLength,
                             IN COORD dwWriteCoord,
                             OUT LPDWORD lpNumberOfCharsWritten)
{
    return IntWriteConsoleOutputCode(hConsoleOutput,
                                     CODE_ASCII,
                                     lpCharacter,
                                     nLength,
                                     dwWriteCoord,
                                     lpNumberOfCharsWritten);
}


/*--------------------------------------------------------------
 *     WriteConsoleOutputAttribute
 *
 * @implemented
 */
BOOL
WINAPI
WriteConsoleOutputAttribute(IN HANDLE hConsoleOutput,
                            IN CONST WORD *lpAttribute,
                            IN DWORD nLength,
                            IN COORD dwWriteCoord,
                            OUT LPDWORD lpNumberOfAttrsWritten)
{
    return IntWriteConsoleOutputCode(hConsoleOutput,
                                     CODE_ATTRIBUTE,
                                     lpAttribute,
                                     nLength,
                                     dwWriteCoord,
                                     lpNumberOfAttrsWritten);
}


/*--------------------------------------------------------------
 *    FillConsoleOutputCharacterW
 *
 * @implemented
 */
BOOL
WINAPI
FillConsoleOutputCharacterW(IN HANDLE hConsoleOutput,
                            IN WCHAR cCharacter,
                            IN DWORD nLength,
                            IN COORD dwWriteCoord,
                            OUT LPDWORD lpNumberOfCharsWritten)
{
    CODE_ELEMENT Code;
    Code.UnicodeChar = cCharacter;
    return IntFillConsoleOutputCode(hConsoleOutput,
                                    CODE_UNICODE,
                                    Code,
                                    nLength,
                                    dwWriteCoord,
                                    lpNumberOfCharsWritten);
}


/*--------------------------------------------------------------
 *    FillConsoleOutputCharacterA
 *
 * @implemented
 */
BOOL
WINAPI
FillConsoleOutputCharacterA(IN HANDLE hConsoleOutput,
                            IN CHAR cCharacter,
                            IN DWORD nLength,
                            IN COORD dwWriteCoord,
                            LPDWORD lpNumberOfCharsWritten)
{
    CODE_ELEMENT Code;
    Code.AsciiChar = cCharacter;
    return IntFillConsoleOutputCode(hConsoleOutput,
                                    CODE_ASCII,
                                    Code,
                                    nLength,
                                    dwWriteCoord,
                                    lpNumberOfCharsWritten);
}


/*--------------------------------------------------------------
 *     FillConsoleOutputAttribute
 *
 * @implemented
 */
BOOL
WINAPI
FillConsoleOutputAttribute(IN HANDLE hConsoleOutput,
                           IN WORD wAttribute,
                           IN DWORD nLength,
                           IN COORD dwWriteCoord,
                           OUT LPDWORD lpNumberOfAttrsWritten)
{
    CODE_ELEMENT Code;
    Code.Attribute = wAttribute;
    return IntFillConsoleOutputCode(hConsoleOutput,
                                    CODE_ATTRIBUTE,
                                    Code,
                                    nLength,
                                    dwWriteCoord,
                                    lpNumberOfAttrsWritten);
}

/* EOF */
