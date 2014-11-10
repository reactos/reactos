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


/* See consrv/include/rect.h */
#define ConioRectHeight(Rect) \
    (((Rect)->Top) > ((Rect)->Bottom) ? 0 : ((Rect)->Bottom) - ((Rect)->Top) + 1)
#define ConioRectWidth(Rect) \
    (((Rect)->Left) > ((Rect)->Right) ? 0 : ((Rect)->Right) - ((Rect)->Left) + 1)


/* PRIVATE FUNCTIONS **********************************************************/

/******************
 * Read functions *
 ******************/

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
     * Retrieve the (current) Input EXE name string and length,
     * not NULL-terminated (always in UNICODE format).
     */
    ReadConsoleRequest->ExeLength =
        GetCurrentExeName((PWCHAR)ReadConsoleRequest->StaticBuffer,
                          sizeof(ReadConsoleRequest->StaticBuffer));

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
    _SEH2_TRY
    {
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
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        // HACK
        if (CaptureBuffer) CsrFreeCaptureBuffer(CaptureBuffer);
        SetLastError(ERROR_INVALID_ACCESS);
        _SEH2_YIELD(return FALSE);
    }
    _SEH2_END;

    /* FIXME: Check for sanity */
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
    BOOL Success;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_READOUTPUT ReadOutputRequest = &ApiMessage.Data.ReadOutputRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer = NULL;

    SHORT SizeX, SizeY;
    ULONG NumCells;

    /* Set up the data to send to the Console Server */
    ReadOutputRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    ReadOutputRequest->OutputHandle  = hConsoleOutput;
    ReadOutputRequest->Unicode       = bUnicode;

    /* Update lpReadRegion */
    _SEH2_TRY
    {
        SizeX = min(dwBufferSize.X - dwBufferCoord.X, ConioRectWidth(lpReadRegion));
        SizeY = min(dwBufferSize.Y - dwBufferCoord.Y, ConioRectHeight(lpReadRegion));
        if (SizeX <= 0 || SizeY <= 0)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            _SEH2_YIELD(return FALSE);
        }
        lpReadRegion->Right  = lpReadRegion->Left + SizeX - 1;
        lpReadRegion->Bottom = lpReadRegion->Top  + SizeY - 1;

        ReadOutputRequest->ReadRegion = *lpReadRegion;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(ERROR_INVALID_ACCESS);
        _SEH2_YIELD(return FALSE);
    }
    _SEH2_END;

    NumCells = SizeX * SizeY;
    DPRINT("IntReadConsoleOutput: (%d x %d)\n", SizeX, SizeY);

    /*
     * For optimization purposes, Windows (and hence ReactOS, too, for
     * compatibility reasons) uses a static buffer if no more than one
     * cell is read. Otherwise a new buffer is allocated.
     * This behaviour is also expected in the server-side.
     */
    if (NumCells <= 1)
    {
        ReadOutputRequest->CharInfo = &ReadOutputRequest->StaticBuffer;
        // CaptureBuffer = NULL;
    }
    else
    {
        ULONG Size = NumCells * sizeof(CHAR_INFO);

        /* Allocate a Capture Buffer */
        CaptureBuffer = CsrAllocateCaptureBuffer(1, Size);
        if (CaptureBuffer == NULL)
        {
            DPRINT1("CsrAllocateCaptureBuffer failed with size %ld!\n", Size);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        /* Allocate space in the Buffer */
        CsrAllocateMessagePointer(CaptureBuffer,
                                  Size,
                                  (PVOID*)&ReadOutputRequest->CharInfo);
    }

    /* Call the server */
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepReadConsoleOutput),
                        sizeof(*ReadOutputRequest));

    /* Check for success */
    Success = NT_SUCCESS(ApiMessage.Status);

    /* Retrieve the results */
    _SEH2_TRY
    {
        *lpReadRegion = ReadOutputRequest->ReadRegion;

        if (Success)
        {
#if 0
            SHORT x, X;
#endif
            SHORT y, Y;

            /* Copy into the buffer */

            SizeX = ReadOutputRequest->ReadRegion.Right -
                    ReadOutputRequest->ReadRegion.Left + 1;

            for (y = 0, Y = ReadOutputRequest->ReadRegion.Top; Y <= ReadOutputRequest->ReadRegion.Bottom; ++y, ++Y)
            {
                RtlCopyMemory(lpBuffer + (y + dwBufferCoord.Y) * dwBufferSize.X + dwBufferCoord.X,
                              ReadOutputRequest->CharInfo + y * SizeX,
                              SizeX * sizeof(CHAR_INFO));
#if 0
                for (x = 0, X = ReadOutputRequest->ReadRegion.Left; X <= ReadOutputRequest->ReadRegion.Right; ++x, ++X)
                {
                    *(lpBuffer + (y + dwBufferCoord.Y) * dwBufferSize.X + (x + dwBufferCoord.X)) =
                    *(ReadOutputRequest->CharInfo + y * SizeX + x);
                }
#endif
            }
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
            _SEH2_YIELD(return FALSE);
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
            _SEH2_YIELD(return FALSE);
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
    BOOL Success;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_WRITEOUTPUT WriteOutputRequest = &ApiMessage.Data.WriteOutputRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer = NULL;

    SHORT SizeX, SizeY;
    ULONG NumCells;

    /* Set up the data to send to the Console Server */
    WriteOutputRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    WriteOutputRequest->OutputHandle  = hConsoleOutput;
    WriteOutputRequest->Unicode       = bUnicode;

    /* Update lpWriteRegion */
    _SEH2_TRY
    {
        SizeX = min(dwBufferSize.X - dwBufferCoord.X, ConioRectWidth(lpWriteRegion));
        SizeY = min(dwBufferSize.Y - dwBufferCoord.Y, ConioRectHeight(lpWriteRegion));
        if (SizeX <= 0 || SizeY <= 0)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            _SEH2_YIELD(return FALSE);
        }
        lpWriteRegion->Right  = lpWriteRegion->Left + SizeX - 1;
        lpWriteRegion->Bottom = lpWriteRegion->Top  + SizeY - 1;

        WriteOutputRequest->WriteRegion = *lpWriteRegion;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(ERROR_INVALID_ACCESS);
        _SEH2_YIELD(return FALSE);
    }
    _SEH2_END;

    NumCells = SizeX * SizeY;
    DPRINT("IntWriteConsoleOutput: (%d x %d)\n", SizeX, SizeY);

    /*
     * For optimization purposes, Windows (and hence ReactOS, too, for
     * compatibility reasons) uses a static buffer if no more than one
     * cell is written. Otherwise a new buffer is allocated.
     * This behaviour is also expected in the server-side.
     */
    if (NumCells <= 1)
    {
        WriteOutputRequest->CharInfo = &WriteOutputRequest->StaticBuffer;
        // CaptureBuffer = NULL;
        WriteOutputRequest->UseVirtualMemory = FALSE;
    }
    else
    {
        ULONG Size = NumCells * sizeof(CHAR_INFO);

        /* Allocate a Capture Buffer */
        CaptureBuffer = CsrAllocateCaptureBuffer(1, Size);
        if (CaptureBuffer)
        {
            /* Allocate space in the Buffer */
            CsrAllocateMessagePointer(CaptureBuffer,
                                      Size,
                                      (PVOID*)&WriteOutputRequest->CharInfo);
            WriteOutputRequest->UseVirtualMemory = FALSE;
        }
        else
        {
            /*
             * CsrAllocateCaptureBuffer failed because we tried to allocate
             * a too large (>= 64 kB, size of the CSR heap) data buffer.
             * To circumvent this, Windows uses a trick (that we reproduce for
             * compatibility reasons): we allocate a heap buffer in the process'
             * memory, and CSR will read it via NtReadVirtualMemory.
             */
            DPRINT1("CsrAllocateCaptureBuffer failed with size %ld, let's use local heap buffer...\n", Size);

            WriteOutputRequest->CharInfo = RtlAllocateHeap(RtlGetProcessHeap(), 0, Size);
            WriteOutputRequest->UseVirtualMemory = TRUE;

            /* Bail out if we still cannot allocate memory */
            if (WriteOutputRequest->CharInfo == NULL)
            {
                DPRINT1("Failed to allocate heap buffer with size %ld!\n", Size);
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
            }
        }
    }

    /* Capture the user buffer contents */
    _SEH2_TRY
    {
#if 0
        SHORT x, X;
#endif
        SHORT y, Y;

        /* Copy into the buffer */

        SizeX = WriteOutputRequest->WriteRegion.Right -
                WriteOutputRequest->WriteRegion.Left + 1;

        for (y = 0, Y = WriteOutputRequest->WriteRegion.Top; Y <= WriteOutputRequest->WriteRegion.Bottom; ++y, ++Y)
        {
            RtlCopyMemory(WriteOutputRequest->CharInfo + y * SizeX,
                          lpBuffer + (y + dwBufferCoord.Y) * dwBufferSize.X + dwBufferCoord.X,
                          SizeX * sizeof(CHAR_INFO));
#if 0
            for (x = 0, X = WriteOutputRequest->WriteRegion.Left; X <= WriteOutputRequest->WriteRegion.Right; ++x, ++X)
            {
                *(WriteOutputRequest->CharInfo + y * SizeX + x) =
                *(lpBuffer + (y + dwBufferCoord.Y) * dwBufferSize.X + (x + dwBufferCoord.X));
            }
#endif
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(ERROR_INVALID_ACCESS);
        _SEH2_YIELD(return FALSE);
    }
    _SEH2_END;

    /* Call the server */
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepWriteConsoleOutput),
                        sizeof(*WriteOutputRequest));

    /* Check for success */
    Success = NT_SUCCESS(ApiMessage.Status);

    /* Release the capture buffer if needed */
    if (CaptureBuffer)
    {
        CsrFreeCaptureBuffer(CaptureBuffer);
    }
    else
    {
        /* If we used a heap buffer, free it */
        if (WriteOutputRequest->UseVirtualMemory)
            RtlFreeHeap(RtlGetProcessHeap(), 0, WriteOutputRequest->CharInfo);
    }

    /* Retrieve the results */
    _SEH2_TRY
    {
        *lpWriteRegion = WriteOutputRequest->WriteRegion;

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
            _SEH2_YIELD(return FALSE);
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

/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
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


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
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


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
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


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
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


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
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


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
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


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
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


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
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


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
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


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
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


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
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


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
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


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
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

/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
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


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
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


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
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


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
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


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
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


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
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


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
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


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
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


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
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


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
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


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
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


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
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


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
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


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
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
