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
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_READCONSOLE ReadConsoleRequest = &ApiMessage.Data.ReadConsoleRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    ULONG CharSize;

    /* Determine the needed size */
    CharSize = (bUnicode ? sizeof(WCHAR) : sizeof(CHAR));
    ReadConsoleRequest->BufferSize = nNumberOfCharsToRead * CharSize;

    /* Allocate a Capture Buffer */
    CaptureBuffer = CsrAllocateCaptureBuffer(1, ReadConsoleRequest->BufferSize);
    if (CaptureBuffer == NULL)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Allocate space in the Buffer */
    CsrAllocateMessagePointer(CaptureBuffer,
                              ReadConsoleRequest->BufferSize,
                              (PVOID*)&ReadConsoleRequest->Buffer);

    /* Set up the data to send to the Console Server */
    ReadConsoleRequest->InputHandle = hConsoleInput;
    ReadConsoleRequest->Unicode = bUnicode;
    ReadConsoleRequest->NrCharactersToRead = nNumberOfCharsToRead;
    ReadConsoleRequest->NrCharactersRead = 0;
    ReadConsoleRequest->CtrlWakeupMask = 0;
    if (pInputControl && pInputControl->nLength == sizeof(CONSOLE_READCONSOLE_CONTROL))
    {
        /*
         * From MSDN (ReadConsole function), the description
         * for pInputControl says:
         * "This parameter requires Unicode input by default.
         * For ANSI mode, set this parameter to NULL."
         */
        ReadConsoleRequest->NrCharactersRead = pInputControl->nInitialChars;
        RtlCopyMemory(ReadConsoleRequest->Buffer,
                      lpBuffer,
                      pInputControl->nInitialChars * sizeof(WCHAR));
        ReadConsoleRequest->CtrlWakeupMask = pInputControl->dwCtrlWakeupMask;
    }

    /* Call the server */
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepReadConsole),
                        sizeof(*ReadConsoleRequest));

    /* Check for success */
    if (NT_SUCCESS(ApiMessage.Status))
    {
        RtlCopyMemory(lpBuffer,
                      ReadConsoleRequest->Buffer,
                      ReadConsoleRequest->NrCharactersRead * CharSize);

        if (lpNumberOfCharsRead != NULL)
            *lpNumberOfCharsRead = ReadConsoleRequest->NrCharactersRead;

        if (pInputControl && pInputControl->nLength == sizeof(CONSOLE_READCONSOLE_CONTROL))
            pInputControl->dwControlKeyState = ReadConsoleRequest->ControlKeyState;
    }
    else
    {
        DPRINT1("CSR returned error in ReadConsole\n");

        if (lpNumberOfCharsRead != NULL)
            *lpNumberOfCharsRead = 0;

        /* Error out */
        BaseSetLastNTError(ApiMessage.Status);
    }

    CsrFreeCaptureBuffer(CaptureBuffer);

    /* Return TRUE or FALSE */
    // return TRUE;
    return (ReadConsoleRequest->NrCharactersRead > 0);
    // return NT_SUCCESS(ApiMessage.Status);
}


static
BOOL
IntGetConsoleInput(HANDLE hConsoleInput,
                   PINPUT_RECORD lpBuffer,
                   DWORD nLength,
                   LPDWORD lpNumberOfEventsRead,
                   WORD wFlags,
                   BOOLEAN bUnicode)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETINPUT GetInputRequest = &ApiMessage.Data.GetInputRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer = NULL;

    if (lpBuffer == NULL)
    {
        SetLastError(ERROR_INVALID_ACCESS);
        return FALSE;
    }

    if (!IsConsoleHandle(hConsoleInput))
    {
        SetLastError(ERROR_INVALID_HANDLE);

        if (lpNumberOfEventsRead != NULL)
            *lpNumberOfEventsRead = 0;

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
    if (NT_SUCCESS(ApiMessage.Status))
    {
        /* Return the number of events read */
        DPRINT("Events read: %lx\n", GetInputRequest->NumRecords);

        if (lpNumberOfEventsRead != NULL)
            *lpNumberOfEventsRead = GetInputRequest->NumRecords;

        /* Copy into the buffer */
        RtlCopyMemory(lpBuffer,
                      GetInputRequest->RecordBufPtr,
                      GetInputRequest->NumRecords * sizeof(INPUT_RECORD));
    }
    else
    {
        if (lpNumberOfEventsRead != NULL)
            *lpNumberOfEventsRead = 0;

        /* Error out */
        BaseSetLastNTError(ApiMessage.Status);
    }

    /* Release the capture buffer if needed */
    if (CaptureBuffer) CsrFreeCaptureBuffer(CaptureBuffer);

    /* Return TRUE or FALSE */
    return NT_SUCCESS(ApiMessage.Status);
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
        /* Error out */
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
IntReadConsoleOutputCode(HANDLE hConsoleOutput,
                         CODE_TYPE CodeType,
                         PVOID pCode,
                         DWORD nLength,
                         COORD dwReadCoord,
                         LPDWORD lpNumberOfCodesRead)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_READOUTPUTCODE ReadOutputCodeRequest = &ApiMessage.Data.ReadOutputCodeRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer = NULL;
    ULONG SizeBytes, CodeSize;

    DPRINT("IntReadConsoleOutputCode\n");

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
            CodeSize = sizeof(CHAR);
            break;

        case CODE_UNICODE:
            CodeSize = sizeof(WCHAR);
            break;

        case CODE_ATTRIBUTE:
            CodeSize = sizeof(WORD);
            break;

        default:
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
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
        ReadOutputCodeRequest->pCode.pCode = ReadOutputCodeRequest->CodeStaticBuffer;
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
                                  (PVOID*)&ReadOutputCodeRequest->pCode.pCode);
    }

    /* Call the server */
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepReadConsoleOutputString),
                        sizeof(*ReadOutputCodeRequest));

    /* Check for success */
    if (NT_SUCCESS(ApiMessage.Status))
    {
        DWORD NumCodes = ReadOutputCodeRequest->NumCodes;
        RtlCopyMemory(pCode,
                      ReadOutputCodeRequest->pCode.pCode,
                      NumCodes * CodeSize);

        if (lpNumberOfCodesRead != NULL)
            *lpNumberOfCodesRead = NumCodes;
    }
    else
    {
        if (lpNumberOfCodesRead != NULL)
            *lpNumberOfCodesRead = 0;

        /* Error out */
        BaseSetLastNTError(ApiMessage.Status);
    }

    /* Release the capture buffer if needed */
    if (CaptureBuffer) CsrFreeCaptureBuffer(CaptureBuffer);

    /* Return TRUE or FALSE */
    return NT_SUCCESS(ApiMessage.Status);
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
    BOOL bRet = TRUE;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_WRITECONSOLE WriteConsoleRequest = &ApiMessage.Data.WriteConsoleRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    ULONG CharSize;

    /* Determine the needed size */
    CharSize = (bUnicode ? sizeof(WCHAR) : sizeof(CHAR));
    WriteConsoleRequest->BufferSize = nNumberOfCharsToWrite * CharSize;

    /* Allocate a Capture Buffer */
    CaptureBuffer = CsrAllocateCaptureBuffer(1, WriteConsoleRequest->BufferSize);
    if (CaptureBuffer == NULL)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Capture the buffer to write */
    CsrCaptureMessageBuffer(CaptureBuffer,
                            (PVOID)lpBuffer,
                            WriteConsoleRequest->BufferSize,
                            (PVOID*)&WriteConsoleRequest->Buffer);

    /* Start writing */
    WriteConsoleRequest->NrCharactersToWrite = nNumberOfCharsToWrite;
    WriteConsoleRequest->OutputHandle = hConsoleOutput;
    WriteConsoleRequest->Unicode = bUnicode;

    /* Call the server */
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepWriteConsole),
                        sizeof(*WriteConsoleRequest));

    /* Check for success */
    if (NT_SUCCESS(ApiMessage.Status))
    {
        if (lpNumberOfCharsWritten != NULL)
            *lpNumberOfCharsWritten = WriteConsoleRequest->NrCharactersWritten;

        bRet = TRUE;
    }
    else
    {
        if (lpNumberOfCharsWritten != NULL)
            *lpNumberOfCharsWritten = 0;

        /* Error out */
        BaseSetLastNTError(ApiMessage.Status);
        bRet = FALSE;
    }

    CsrFreeCaptureBuffer(CaptureBuffer);

    return bRet;
}


static
BOOL
IntWriteConsoleInput(HANDLE hConsoleInput,
                     PINPUT_RECORD lpBuffer,
                     DWORD nLength,
                     LPDWORD lpNumberOfEventsWritten,
                     BOOLEAN bUnicode,
                     BOOLEAN bAppendToEnd)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_WRITEINPUT WriteInputRequest = &ApiMessage.Data.WriteInputRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer = NULL;

    if (lpBuffer == NULL)
    {
        SetLastError(ERROR_INVALID_ACCESS);
        return FALSE;
    }

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

        RtlCopyMemory(WriteInputRequest->RecordBufPtr,
                      lpBuffer,
                      nLength * sizeof(INPUT_RECORD));
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

    /* Release the capture buffer if needed */
    if (CaptureBuffer) CsrFreeCaptureBuffer(CaptureBuffer);

    /* Check for success */
    if (NT_SUCCESS(ApiMessage.Status))
    {
        /* Return the number of events written */
        DPRINT("Events written: %lx\n", WriteInputRequest->NumRecords);

        if (lpNumberOfEventsWritten != NULL)
            *lpNumberOfEventsWritten = WriteInputRequest->NumRecords;
    }
    else
    {
        if (lpNumberOfEventsWritten != NULL)
            *lpNumberOfEventsWritten = 0;

        /* Error out */
        BaseSetLastNTError(ApiMessage.Status);
    }

    /* Return TRUE or FALSE */
    return NT_SUCCESS(ApiMessage.Status);
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
        /* Error out */
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
IntWriteConsoleOutputCode(HANDLE hConsoleOutput,
                          CODE_TYPE CodeType,
                          CONST VOID *pCode,
                          DWORD nLength,
                          COORD dwWriteCoord,
                          LPDWORD lpNumberOfCodesWritten)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_WRITEOUTPUTCODE WriteOutputCodeRequest = &ApiMessage.Data.WriteOutputCodeRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer = NULL;
    ULONG SizeBytes, CodeSize;

    if (pCode == NULL)
    {
        SetLastError(ERROR_INVALID_ACCESS);
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
            CodeSize = sizeof(CHAR);
            break;

        case CODE_UNICODE:
            CodeSize = sizeof(WCHAR);
            break;

        case CODE_ATTRIBUTE:
            CodeSize = sizeof(WORD);
            break;

        default:
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
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
        WriteOutputCodeRequest->pCode.pCode = WriteOutputCodeRequest->CodeStaticBuffer;
        // CaptureBuffer = NULL;

        RtlCopyMemory(WriteOutputCodeRequest->pCode.pCode,
                      pCode,
                      SizeBytes);
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
                                (PVOID*)&WriteOutputCodeRequest->pCode.pCode);
    }

    /* Call the server */
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepWriteConsoleOutputString),
                        sizeof(*WriteOutputCodeRequest));

    /* Release the capture buffer if needed */
    if (CaptureBuffer) CsrFreeCaptureBuffer(CaptureBuffer);

    /* Check for success */
    if (NT_SUCCESS(ApiMessage.Status))
    {
        if (lpNumberOfCodesWritten != NULL)
            *lpNumberOfCodesWritten = WriteOutputCodeRequest->NumCodes;
    }
    else
    {
        if (lpNumberOfCodesWritten != NULL)
            *lpNumberOfCodesWritten = 0;

        /* Error out */
        BaseSetLastNTError(ApiMessage.Status);
    }

    /* Return TRUE or FALSE */
    return NT_SUCCESS(ApiMessage.Status);
}


static
BOOL
IntFillConsoleOutputCode(HANDLE hConsoleOutput,
                         CODE_TYPE CodeType,
                         PVOID pCode,
                         DWORD nLength,
                         COORD dwWriteCoord,
                         LPDWORD lpNumberOfCodesWritten)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_FILLOUTPUTCODE FillOutputRequest = &ApiMessage.Data.FillOutputRequest;

    /* Set up the data to send to the Console Server */
    FillOutputRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    FillOutputRequest->OutputHandle  = hConsoleOutput;
    FillOutputRequest->WriteCoord    = dwWriteCoord;
    FillOutputRequest->NumCodes      = nLength;

    FillOutputRequest->CodeType = CodeType;
    switch (CodeType)
    {
        case CODE_ASCII:
            FillOutputRequest->Code.AsciiChar = *(PCHAR)pCode;
            break;

        case CODE_UNICODE:
            FillOutputRequest->Code.UnicodeChar = *(PWCHAR)pCode;
            break;

        case CODE_ATTRIBUTE:
            FillOutputRequest->Code.Attribute = *(PWORD)pCode;
            break;

        default:
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
    }

    /* Call the server */
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepFillConsoleOutput),
                        sizeof(*FillOutputRequest));

    /* Check for success */
    if (NT_SUCCESS(ApiMessage.Status))
    {
        if (lpNumberOfCodesWritten != NULL)
            *lpNumberOfCodesWritten = FillOutputRequest->NumCodes;
    }
    else
    {
        if (lpNumberOfCodesWritten != NULL)
            *lpNumberOfCodesWritten = 0;

        BaseSetLastNTError(ApiMessage.Status);
    }

    /* Return TRUE or FALSE */
    return NT_SUCCESS(ApiMessage.Status);
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
PeekConsoleInputA(HANDLE hConsoleInput,
                  PINPUT_RECORD lpBuffer,
                  DWORD nLength,
                  LPDWORD lpNumberOfEventsRead)
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
ReadConsoleInputW(HANDLE hConsoleInput,
                  PINPUT_RECORD lpBuffer,
                  DWORD nLength,
                  LPDWORD lpNumberOfEventsRead)
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
ReadConsoleInputA(HANDLE hConsoleInput,
                  PINPUT_RECORD lpBuffer,
                  DWORD nLength,
                  LPDWORD lpNumberOfEventsRead)
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
ReadConsoleInputExW(HANDLE hConsoleInput,
                    PINPUT_RECORD lpBuffer,
                    DWORD nLength,
                    LPDWORD lpNumberOfEventsRead,
                    WORD wFlags)
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
ReadConsoleInputExA(HANDLE hConsoleInput,
                    PINPUT_RECORD lpBuffer,
                    DWORD nLength,
                    LPDWORD lpNumberOfEventsRead,
                    WORD wFlags)
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
ReadConsoleOutputCharacterA(HANDLE hConsoleOutput,
                            LPSTR lpCharacter,
                            DWORD nLength,
                            COORD dwReadCoord,
                            LPDWORD lpNumberOfCharsRead)
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
ReadConsoleOutputAttribute(HANDLE hConsoleOutput,
                           LPWORD lpAttribute,
                           DWORD nLength,
                           COORD dwReadCoord,
                           LPDWORD lpNumberOfAttrsRead)
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
WriteConsoleInputA(HANDLE hConsoleInput,
                   CONST INPUT_RECORD *lpBuffer,
                   DWORD nLength,
                   LPDWORD lpNumberOfEventsWritten)
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
WriteConsoleInputVDMW(HANDLE hConsoleInput,
                      CONST INPUT_RECORD *lpBuffer,
                      DWORD nLength,
                      LPDWORD lpNumberOfEventsWritten)
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
WriteConsoleInputVDMA(HANDLE hConsoleInput,
                      CONST INPUT_RECORD *lpBuffer,
                      DWORD nLength,
                      LPDWORD lpNumberOfEventsWritten)
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
WriteConsoleOutputCharacterA(HANDLE hConsoleOutput,
                             LPCSTR lpCharacter,
                             DWORD nLength,
                             COORD dwWriteCoord,
                             LPDWORD lpNumberOfCharsWritten)
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
WriteConsoleOutputAttribute(HANDLE hConsoleOutput,
                            CONST WORD *lpAttribute,
                            DWORD nLength,
                            COORD dwWriteCoord,
                            LPDWORD lpNumberOfAttrsWritten)
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
FillConsoleOutputCharacterW(HANDLE hConsoleOutput,
                            WCHAR cCharacter,
                            DWORD nLength,
                            COORD dwWriteCoord,
                            LPDWORD lpNumberOfCharsWritten)
{
    return IntFillConsoleOutputCode(hConsoleOutput,
                                    CODE_UNICODE,
                                    &cCharacter,
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
FillConsoleOutputCharacterA(HANDLE hConsoleOutput,
                            CHAR cCharacter,
                            DWORD nLength,
                            COORD dwWriteCoord,
                            LPDWORD lpNumberOfCharsWritten)
{
    return IntFillConsoleOutputCode(hConsoleOutput,
                                    CODE_ASCII,
                                    &cCharacter,
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
FillConsoleOutputAttribute(HANDLE hConsoleOutput,
                           WORD wAttribute,
                           DWORD nLength,
                           COORD dwWriteCoord,
                           LPDWORD lpNumberOfAttrsWritten)
{
    return IntFillConsoleOutputCode(hConsoleOutput,
                                    CODE_ATTRIBUTE,
                                    &wAttribute,
                                    nLength,
                                    dwWriteCoord,
                                    lpNumberOfAttrsWritten);
}

/* EOF */
