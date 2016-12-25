/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Class that managed an unidirectional anonymous byte stream pipe
 * COPYRIGHT:   Copyright 2015 Thomas Faber <thomas.faber@reactos.org>
 */

#include "precomp.h"

/**
 * Constructs a CPipe object and initializes read and write handles.
 */
CPipe::CPipe()
{
    SECURITY_ATTRIBUTES SecurityAttributes;

    SecurityAttributes.nLength = sizeof(SecurityAttributes);
    SecurityAttributes.bInheritHandle = TRUE;
    SecurityAttributes.lpSecurityDescriptor = NULL;

    if(!CreatePipe(&m_hReadPipe, &m_hWritePipe, &SecurityAttributes, 0))
        FATAL("CreatePipe failed\n");
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
        FATAL("Trying to close already closed read pipe");
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
        FATAL("Trying to close already closed write pipe");
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
        FATAL("Trying to peek from a closed read pipe");

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
 * True on success, false on failure; call GetLastError for error information.
 *
 * @see ReadFile
 */
bool
CPipe::Read(PVOID Buffer, DWORD NumberOfBytesToRead, PDWORD NumberOfBytesRead)
{
    if (!m_hReadPipe)
        FATAL("Trying to read from a closed read pipe");

    return ReadFile(m_hReadPipe, Buffer, NumberOfBytesToRead, NumberOfBytesRead, NULL);
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
        FATAL("Trying to write to a closed write pipe");

    return WriteFile(m_hWritePipe, Buffer, NumberOfBytesToWrite, NumberOfBytesWritten, NULL);
}
