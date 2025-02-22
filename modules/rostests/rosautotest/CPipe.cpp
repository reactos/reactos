/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Class that manages an unidirectional anonymous byte stream pipe
 * COPYRIGHT:   Copyright 2015 Thomas Faber (thomas.faber@reactos.org)
 *              Copyright 2019 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

LONG CPipe::m_lPipeCount = 0;


/**
 * Constructs a CPipe object and initializes read and write handles.
 */
CPipe::CPipe()
{
    SECURITY_ATTRIBUTES SecurityAttributes;

    SecurityAttributes.nLength = sizeof(SecurityAttributes);
    SecurityAttributes.bInheritHandle = TRUE;
    SecurityAttributes.lpSecurityDescriptor = NULL;

    // Construct a unique pipe name.
    WCHAR wszPipeName[MAX_PATH];
    InterlockedIncrement(&m_lPipeCount);
    swprintf(wszPipeName, L"\\\\.\\pipe\\TestmanPipe%ld", m_lPipeCount);

    // Create a named pipe with the default settings, but overlapped (asynchronous) operations.
    // Latter feature is why we can't simply use CreatePipe.
    const DWORD dwDefaultBufferSize = 4096;
    const DWORD dwDefaultTimeoutMilliseconds = 120000;

    m_hReadPipe = CreateNamedPipeW(wszPipeName,
        PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        1,
        dwDefaultBufferSize,
        dwDefaultBufferSize,
        dwDefaultTimeoutMilliseconds,
        &SecurityAttributes);
    if (m_hReadPipe == INVALID_HANDLE_VALUE)
    {
        FATAL("CreateNamedPipe failed\n");
    }

    // Use CreateFileW to get the write handle to the pipe.
    // Writing is done synchronously, so no FILE_FLAG_OVERLAPPED here!
    m_hWritePipe = CreateFileW(wszPipeName,
        GENERIC_WRITE,
        0,
        &SecurityAttributes,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (m_hWritePipe == INVALID_HANDLE_VALUE)
    {
        FATAL("CreateFileW failed\n");
    }

    // Prepare the OVERLAPPED structure for reading.
    ZeroMemory(&m_ReadOverlapped, sizeof(m_ReadOverlapped));
    m_ReadOverlapped.hEvent = CreateEventW(NULL, TRUE, TRUE, NULL);
    if (!m_ReadOverlapped.hEvent)
    {
        FATAL("CreateEvent failed\n");
    }
}

/**
 * Destructs a CPipe object and closes all open handles.
 */
CPipe::~CPipe()
{
    if (m_hReadPipe)
        CloseHandle(m_hReadPipe);
    if (m_hWritePipe)
        CloseHandle(m_hWritePipe);
}

/**
 * Closes a CPipe's read pipe, leaving the write pipe unchanged.
 */
void
CPipe::CloseReadPipe()
{
    if (!m_hReadPipe)
        FATAL("Trying to close already closed read pipe\n");
    CloseHandle(m_hReadPipe);
    m_hReadPipe = NULL;
}

/**
 * Closes a CPipe's write pipe, leaving the read pipe unchanged.
 */
void
CPipe::CloseWritePipe()
{
    if (!m_hWritePipe)
        FATAL("Trying to close already closed write pipe\n");
    CloseHandle(m_hWritePipe);
    m_hWritePipe = NULL;
}

/**
 * Reads data from a pipe without advancing the read offset and/or retrieves information about available data.
 *
 * This function must not be called after CloseReadPipe.
 *
 * @param Buffer
 * An optional buffer to read pipe data into.
 *
 * @param BufferSize
 * The size of the buffer specified in Buffer, or 0 if no read should be performed.
 *
 * @param BytesRead
 * On return, the number of bytes actually read from the pipe into Buffer.
 *
 * @param TotalBytesAvailable
 * On return, the total number of bytes available to read from the pipe.
 *
 * @return
 * True on success, false on failure; call GetLastError for error information.
 *
 * @see PeekNamedPipe
 */
bool
CPipe::Peek(PVOID Buffer, DWORD BufferSize, PDWORD BytesRead, PDWORD TotalBytesAvailable)
{
    if (!m_hReadPipe)
        FATAL("Trying to peek from a closed read pipe\n");

    return PeekNamedPipe(m_hReadPipe, Buffer, BufferSize, BytesRead, TotalBytesAvailable, NULL);
}

/**
 * Reads data from the read pipe, advancing the read offset accordingly.
 *
 * This function must not be called after CloseReadPipe.
 *
 * @param Buffer
 * Buffer to read pipe data into.
 *
 * @param NumberOfBytesToRead
 * The number of bytes to read into Buffer.
 *
 * @param NumberOfBytesRead
 * On return, the number of bytes actually read from the pipe into Buffer.
 *
 * @return
 * Returns a Win32 error code. Expected error codes include:
 *   - ERROR_SUCCESS:     The read has completed successfully.
 *   - WAIT_TIMEOUT:      The given timeout has elapsed before any data was read.
 *   - ERROR_BROKEN_PIPE: The other end of the pipe has been closed.
 *
 * @see ReadFile
 */
DWORD
CPipe::Read(PVOID Buffer, DWORD NumberOfBytesToRead, PDWORD NumberOfBytesRead, DWORD TimeoutMilliseconds)
{
    if (!m_hReadPipe)
    {
        FATAL("Trying to read from a closed read pipe\n");
    }

    if (ReadFile(m_hReadPipe, Buffer, NumberOfBytesToRead, NumberOfBytesRead, &m_ReadOverlapped))
    {
        // The asynchronous read request could be satisfied immediately.
        return ERROR_SUCCESS;
    }

    DWORD dwLastError = GetLastError();
    if (dwLastError == ERROR_IO_PENDING)
    {
        // The asynchronous read request could not be satisfied immediately, so wait for it with the given timeout.
        DWORD dwWaitResult = WaitForSingleObject(m_ReadOverlapped.hEvent, TimeoutMilliseconds);
        if (dwWaitResult == WAIT_OBJECT_0)
        {
            // Fill NumberOfBytesRead.
            if (GetOverlappedResult(m_hReadPipe, &m_ReadOverlapped, NumberOfBytesRead, FALSE))
            {
                // We successfully read NumberOfBytesRead bytes.
                return ERROR_SUCCESS;
            }

            dwLastError = GetLastError();
            if (dwLastError == ERROR_BROKEN_PIPE)
            {
                // The other end of the pipe has been closed.
                return ERROR_BROKEN_PIPE;
            }
            else
            {
                // An unexpected error.
                FATAL("GetOverlappedResult failed\n");
            }
        }
        else
        {
            // This may be WAIT_TIMEOUT or an unexpected error.
            return dwWaitResult;
        }
    }
    else
    {
        // This may be ERROR_BROKEN_PIPE or an unexpected error.
        return dwLastError;
    }
}

/**
 * Writes data to the write pipe.
 *
 * This function must not be called after CloseWritePipe.
 *
 * @param Buffer
 * Buffer containing the data to write.
 *
 * @param NumberOfBytesToWrite
 * The number of bytes to write to the pipe from Buffer.
 *
 * @param NumberOfBytesWritten
 * On return, the number of bytes actually written to the pipe.
 *
 * @return
 * True on success, false on failure; call GetLastError for error information.
 *
 * @see WriteFile
 */
bool
CPipe::Write(LPCVOID Buffer, DWORD NumberOfBytesToWrite, PDWORD NumberOfBytesWritten)
{
    if (!m_hWritePipe)
        FATAL("Trying to write to a closed write pipe\n");

    return WriteFile(m_hWritePipe, Buffer, NumberOfBytesToWrite, NumberOfBytesWritten, NULL);
}
