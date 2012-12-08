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
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCSRSS_READ_CONSOLE ReadConsoleRequest = &ApiMessage.Data.ReadConsoleRequest;
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
    ReadConsoleRequest->ConsoleHandle = hConsoleInput;
    ReadConsoleRequest->Unicode = bUnicode;
    ReadConsoleRequest->NrCharactersToRead = (WORD)nNumberOfCharsToRead;
    ReadConsoleRequest->NrCharactersRead = 0;
    ReadConsoleRequest->CtrlWakeupMask = 0;
    if (pInputControl && pInputControl->nLength == sizeof(CONSOLE_READCONSOLE_CONTROL))
    {
        ReadConsoleRequest->NrCharactersRead = pInputControl->nInitialChars;
        memcpy(ReadConsoleRequest->Buffer,
               lpBuffer,
               pInputControl->nInitialChars * sizeof(WCHAR));
        ReadConsoleRequest->CtrlWakeupMask = pInputControl->dwCtrlWakeupMask;
    }

    /* Call the server */
    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 CaptureBuffer,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepReadConsole),
                                 sizeof(CSRSS_READ_CONSOLE));

    /* Check for success */
    if (NT_SUCCESS(Status) || NT_SUCCESS(Status = ApiMessage.Status))
    {
        memcpy(lpBuffer,
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
        BaseSetLastNTError(Status /* ApiMessage.Status */);
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
                   BOOL bRead,
                   PINPUT_RECORD lpBuffer,
                   DWORD nLength,
                   LPDWORD lpNumberOfEventsRead,
                   BOOL bUnicode)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCSRSS_GET_CONSOLE_INPUT GetConsoleInputRequest = &ApiMessage.Data.GetConsoleInputRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    ULONG Size;

    if (lpBuffer == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Size = nLength * sizeof(INPUT_RECORD);

    DPRINT("IntGetConsoleInput: %lx %p\n", Size, lpNumberOfEventsRead);

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
                              (PVOID*)&GetConsoleInputRequest->InputRecord);

    /* Set up the data to send to the Console Server */
    GetConsoleInputRequest->ConsoleHandle = hConsoleInput;
    GetConsoleInputRequest->Unicode = bUnicode;
    GetConsoleInputRequest->bRead = bRead;
    GetConsoleInputRequest->InputsRead = 0;
    GetConsoleInputRequest->Length = nLength;

    /* Call the server */
    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 CaptureBuffer,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetConsoleInput),
                                 sizeof(CSRSS_GET_CONSOLE_INPUT));
    DPRINT("Server returned: %x\n", ApiMessage.Status);

    /* Check for success */
    if (NT_SUCCESS(Status) || NT_SUCCESS(Status = ApiMessage.Status))
    {
        /* Return the number of events read */
        DPRINT("Events read: %lx\n", GetConsoleInputRequest->InputsRead);

        if (lpNumberOfEventsRead != NULL)
            *lpNumberOfEventsRead = GetConsoleInputRequest->InputsRead;

        /* Copy into the buffer */
        DPRINT("Copying to buffer\n");
        RtlCopyMemory(lpBuffer,
                      GetConsoleInputRequest->InputRecord,
                      sizeof(INPUT_RECORD) * GetConsoleInputRequest->InputsRead);
    }
    else
    {
        if (lpNumberOfEventsRead != NULL)
            *lpNumberOfEventsRead = 0;

        /* Error out */
        BaseSetLastNTError(ApiMessage.Status);

/*********
        // BaseSetLastNTError(Status); ????
        if (GetConsoleInputRequest->InputsRead == 0)
        {
            /\* we couldn't read a single record, fail *\/
            BaseSetLastNTError(Status);
            return FALSE;
        }
        else
        {
            /\* FIXME - fail gracefully in case we already read at least one record? *\/
            // break;
        }
*********/
    }

    /* Release the capture buffer */
    CsrFreeCaptureBuffer(CaptureBuffer);

    /* Return TRUE or FALSE */
    return (GetConsoleInputRequest->InputsRead > 0);
    // return NT_SUCCESS(ApiMessage.Status);
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
    PCSRSS_READ_CONSOLE_OUTPUT ReadConsoleOutputRequest = &ApiMessage.Data.ReadConsoleOutputRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    DWORD Size, SizeX, SizeY;

    if (lpBuffer == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
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
                              (PVOID*)&ReadConsoleOutputRequest->CharInfo);

    /* Set up the data to send to the Console Server */
    ReadConsoleOutputRequest->ConsoleHandle = hConsoleOutput;
    ReadConsoleOutputRequest->Unicode = bUnicode;
    ReadConsoleOutputRequest->BufferSize = dwBufferSize;
    ReadConsoleOutputRequest->BufferCoord = dwBufferCoord;
    ReadConsoleOutputRequest->ReadRegion = *lpReadRegion;

    /* Call the server */
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepReadConsoleOutput),
                        sizeof(CSRSS_READ_CONSOLE_OUTPUT));
    DPRINT("Server returned: %x\n", ApiMessage.Status);

    /* Check for success*/
    if (NT_SUCCESS(ApiMessage.Status))
    {
        /* Copy into the buffer */
        DPRINT("Copying to buffer\n");
        SizeX = ReadConsoleOutputRequest->ReadRegion.Right -
                ReadConsoleOutputRequest->ReadRegion.Left + 1;
        SizeY = ReadConsoleOutputRequest->ReadRegion.Bottom -
                ReadConsoleOutputRequest->ReadRegion.Top + 1;
        RtlCopyMemory(lpBuffer,
                      ReadConsoleOutputRequest->CharInfo,
                      sizeof(CHAR_INFO) * SizeX * SizeY);
    }
    else
    {
        /* Error out */
        BaseSetLastNTError(ApiMessage.Status);
    }

    /* Return the read region */
    DPRINT("read region: %lx\n", ReadConsoleOutputRequest->ReadRegion);
    *lpReadRegion = ReadConsoleOutputRequest->ReadRegion;

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
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCSRSS_READ_CONSOLE_OUTPUT_CODE ReadConsoleOutputCodeRequest = &ApiMessage.Data.ReadConsoleOutputCodeRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    ULONG SizeBytes, CodeSize;
    DWORD /*CodesRead = 0,*/ BytesRead;

    /* Determine the needed size */
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
                              (PVOID*)&ReadConsoleOutputCodeRequest->pCode.pCode);

    /* Start reading */
    ReadConsoleOutputCodeRequest->ConsoleHandle = hConsoleOutput;
    ReadConsoleOutputCodeRequest->CodeType = CodeType;
    ReadConsoleOutputCodeRequest->ReadCoord = dwReadCoord;

    // while (nLength > 0)
    {
        ReadConsoleOutputCodeRequest->NumCodesToRead = nLength;
        // SizeBytes = ReadConsoleOutputCodeRequest->NumCodesToRead * CodeSize;

        Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                     CaptureBuffer,
                                     CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepReadConsoleOutputString),
                                     sizeof(CSRSS_READ_CONSOLE_OUTPUT_CODE));
        if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = ApiMessage.Status))
        {
            BaseSetLastNTError(Status);
            CsrFreeCaptureBuffer(CaptureBuffer);
            return FALSE;
        }

        BytesRead = ReadConsoleOutputCodeRequest->CodesRead * CodeSize;
        memcpy(pCode, ReadConsoleOutputCodeRequest->pCode.pCode, BytesRead);
        // pCode = (PVOID)((ULONG_PTR)pCode + /*(ULONG_PTR)*/BytesRead);
        // nLength -= ReadConsoleOutputCodeRequest->CodesRead;
        // CodesRead += ReadConsoleOutputCodeRequest->CodesRead;

        ReadConsoleOutputCodeRequest->ReadCoord = ReadConsoleOutputCodeRequest->EndCoord;
    }

    if (lpNumberOfCodesRead != NULL)
        *lpNumberOfCodesRead = /*CodesRead;*/ ReadConsoleOutputCodeRequest->CodesRead;

    CsrFreeCaptureBuffer(CaptureBuffer);

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
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCSRSS_WRITE_CONSOLE WriteConsoleRequest = &ApiMessage.Data.WriteConsoleRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    // USHORT nChars;
    ULONG /* SizeBytes, */ CharSize;
    // DWORD Written = 0;

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
    WriteConsoleRequest->ConsoleHandle = hConsoleOutput;
    WriteConsoleRequest->Unicode = bUnicode;

    // while (nNumberOfCharsToWrite > 0)
    {
        //// nChars = (USHORT)min(nNumberOfCharsToWrite, CSRSS_MAX_WRITE_CONSOLE / CharSize);
        // nChars = nNumberOfCharsToWrite;
        // WriteConsoleRequest->NrCharactersToWrite = nChars;

        // SizeBytes = nChars * CharSize;

        // memcpy(WriteConsoleRequest->Buffer, lpBuffer, SizeBytes);

        Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                     CaptureBuffer,
                                     CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepWriteConsole),
                                     sizeof(CSRSS_WRITE_CONSOLE));
/** FIXME: Added in 47359 for pausing

        if (Status == STATUS_PENDING)
        {
            WaitForSingleObject(WriteConsoleRequest->UnpauseEvent, INFINITE);
            CloseHandle(WriteConsoleRequest->UnpauseEvent);
            continue;
        }
**/
        if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = ApiMessage.Status))
        {
            CsrFreeCaptureBuffer(CaptureBuffer);
            BaseSetLastNTError(Status);
            return FALSE;
        }

        // nNumberOfCharsToWrite -= nChars;
        // lpBuffer = (PVOID)((ULONG_PTR)lpBuffer + (ULONG_PTR)SizeBytes);
        // Written += WriteConsoleRequest->NrCharactersWritten;
    }

    if (lpNumberOfCharsWritten != NULL)
        // *lpNumberOfCharsWritten = Written;
        *lpNumberOfCharsWritten = WriteConsoleRequest->NrCharactersWritten;

    CsrFreeCaptureBuffer(CaptureBuffer);

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
    CONSOLE_API_MESSAGE ApiMessage;
    PCSRSS_WRITE_CONSOLE_INPUT WriteConsoleInputRequest = &ApiMessage.Data.WriteConsoleInputRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    DWORD Size;

    if (lpBuffer == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Size = nLength * sizeof(INPUT_RECORD);

    DPRINT("IntWriteConsoleInput: %lx %p\n", Size, lpNumberOfEventsWritten);

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
                            (PVOID*)&WriteConsoleInputRequest->InputRecord);

    /* Set up the data to send to the Console Server */
    WriteConsoleInputRequest->ConsoleHandle = hConsoleInput;
    WriteConsoleInputRequest->Unicode = bUnicode;
    WriteConsoleInputRequest->Length = nLength;

    /* Call the server */
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepWriteConsoleInput),
                        sizeof(CSRSS_WRITE_CONSOLE_INPUT));
    DPRINT("Server returned: %x\n", ApiMessage.Status);

    /* Check for success*/
    if (NT_SUCCESS(ApiMessage.Status))
    {
        /* Return the number of events read */
        DPRINT("Events read: %lx\n", WriteConsoleInputRequest->Length);

        if (lpNumberOfEventsWritten != NULL)
            *lpNumberOfEventsWritten = WriteConsoleInputRequest->Length;
    }
    else
    {
        if (lpNumberOfEventsWritten != NULL)
            *lpNumberOfEventsWritten = 0;

        /* Error out */
        BaseSetLastNTError(ApiMessage.Status);
    }

    /* Release the capture buffer */
    CsrFreeCaptureBuffer(CaptureBuffer);

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
    PCSRSS_WRITE_CONSOLE_OUTPUT WriteConsoleOutputRequest = &ApiMessage.Data.WriteConsoleOutputRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    ULONG Size;

    if ((lpBuffer == NULL) || (lpWriteRegion == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

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
                            (PVOID*)&WriteConsoleOutputRequest->CharInfo);

    /* Set up the data to send to the Console Server */
    WriteConsoleOutputRequest->ConsoleHandle = hConsoleOutput;
    WriteConsoleOutputRequest->Unicode = bUnicode;
    WriteConsoleOutputRequest->BufferSize = dwBufferSize;
    WriteConsoleOutputRequest->BufferCoord = dwBufferCoord;
    WriteConsoleOutputRequest->WriteRegion = *lpWriteRegion;

    /* Call the server */
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepWriteConsoleOutput),
                        sizeof(CSRSS_WRITE_CONSOLE_OUTPUT));
    DPRINT("Server returned: %x\n", ApiMessage.Status);

    /* Check for success*/
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        /* Error out */
        BaseSetLastNTError(ApiMessage.Status);
    }

    /* Return the read region */
    DPRINT("read region: %lx\n", WriteConsoleOutputRequest->WriteRegion);
    *lpWriteRegion = WriteConsoleOutputRequest->WriteRegion;

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
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCSRSS_WRITE_CONSOLE_OUTPUT_CODE WriteConsoleOutputCodeRequest = &ApiMessage.Data.WriteConsoleOutputCodeRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    ULONG CodeSize; //, nChars;
    // ULONG SizeBytes;
    // DWORD Written = 0;

    /* Determine the needed size */
/*
    CodeSize = (bUnicode ? sizeof(WCHAR) : sizeof(CHAR));
    nChars = min(nLength, CSRSS_MAX_WRITE_CONSOLE_OUTPUT_CHAR / CodeSize);
    SizeBytes = nChars * CodeSize;
*/
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
    WriteConsoleOutputCodeRequest->BufferSize = nLength * CodeSize;

    /* Allocate a Capture Buffer */
    CaptureBuffer = CsrAllocateCaptureBuffer(1, WriteConsoleOutputCodeRequest->BufferSize);
    if (CaptureBuffer == NULL)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

/*
    /\* Allocate space in the Buffer *\/
    CsrAllocateMessagePointer(CaptureBuffer,
                              SizeBytes,
                              (PVOID*)&WriteConsoleOutputCodeRequest->pCode.pCode);
*/
    /* Capture the buffer to write */
    CsrCaptureMessageBuffer(CaptureBuffer,
                            (PVOID)pCode,
                            WriteConsoleOutputCodeRequest->BufferSize,
                            (PVOID*)&WriteConsoleOutputCodeRequest->pCode.pCode);

    /* Start writing */
    WriteConsoleOutputCodeRequest->ConsoleHandle = hConsoleOutput;
    WriteConsoleOutputCodeRequest->CodeType = CodeType;
    WriteConsoleOutputCodeRequest->Coord = dwWriteCoord;

    /**
     ** TODO: HACK: Surely it has to go into CONSRV !!
     **/
    // while (nLength > 0)
    {
        // DWORD BytesWrite;

        WriteConsoleOutputCodeRequest->Length = nLength; // (WORD)min(nLength, nChars);
        // BytesWrite = WriteConsoleOutputCodeRequest->Length * CodeSize;

        // memcpy(WriteConsoleOutputCodeRequest->pCode.pCode, pCode, BytesWrite);

        Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                     CaptureBuffer,
                                     CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepWriteConsoleOutputString),
                                     sizeof(CSRSS_WRITE_CONSOLE_OUTPUT_CODE));
        if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = ApiMessage.Status))
        {
            CsrFreeCaptureBuffer(CaptureBuffer);
            BaseSetLastNTError(Status);
            return FALSE;
        }

        // nLength -= WriteConsoleOutputCodeRequest->NrCharactersWritten;
        // pCode = (PVOID)((ULONG_PTR)pCode + /*(ULONG_PTR)(*/WriteConsoleOutputCodeRequest->NrCharactersWritten * CodeSize/*)*/);
        // Written += WriteConsoleOutputCodeRequest->NrCharactersWritten;

        WriteConsoleOutputCodeRequest->Coord = WriteConsoleOutputCodeRequest->EndCoord;
    }

    if (lpNumberOfCodesWritten != NULL)
        // *lpNumberOfCodesWritten = Written;
        // *lpNumberOfCodesWritten = WriteConsoleOutputCodeRequest->NrCharactersWritten;
        *lpNumberOfCodesWritten = WriteConsoleOutputCodeRequest->Length;

    CsrFreeCaptureBuffer(CaptureBuffer);

    return TRUE;
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
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCSRSS_FILL_OUTPUT FillOutputRequest = &ApiMessage.Data.FillOutputRequest;

    FillOutputRequest->ConsoleHandle = hConsoleOutput;
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

    FillOutputRequest->Coord = dwWriteCoord;
    FillOutputRequest->Length = nLength;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepFillConsoleOutput),
                                 sizeof(CSRSS_FILL_OUTPUT));
    if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = ApiMessage.Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    if (lpNumberOfCodesWritten)
        *lpNumberOfCodesWritten = FillOutputRequest->Length;
        // *lpNumberOfCodesWritten = Request.Data.FillOutputRequest.NrCharactersWritten;

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
    return IntGetConsoleInput(hConsoleInput,
                              FALSE,
                              lpBuffer,
                              nLength,
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
    return IntGetConsoleInput(hConsoleInput,
                              FALSE,
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
    return IntGetConsoleInput(hConsoleInput,
                              TRUE,
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
    return IntGetConsoleInput(hConsoleInput,
                              TRUE,
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
