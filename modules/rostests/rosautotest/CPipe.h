/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Class that manages an unidirectional anonymous byte stream pipe
 * COPYRIGHT:   Copyright 2015 Thomas Faber (thomas.faber@reactos.org)
 *              Copyright 2019 Colin Finck (colin@reactos.org)
 */

class CPipe
{
private:
    static LONG m_lPipeCount;

    OVERLAPPED m_ReadOverlapped;
    HANDLE m_hReadPipe;
    HANDLE m_hWritePipe;

public:
    CPipe();
    ~CPipe();

    void CloseReadPipe();
    void CloseWritePipe();

    bool Peek(PVOID Buffer, DWORD BufferSize, PDWORD BytesRead, PDWORD TotalBytesAvailable);
    DWORD Read(PVOID Buffer, DWORD NumberOfBytesToRead, PDWORD NumberOfBytesRead, DWORD TimeoutMilliseconds);
    bool Write(LPCVOID Buffer, DWORD NumberOfBytesToWrite, PDWORD NumberOfBytesWritten);

    friend class CPipedProcess;
};
