/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Class that manages an unidirectional anonymous byte stream pipe
 * COPYRIGHT:   Copyright 2015 Thomas Faber (thomas.faber@reactos.org)
 */

class CPipe
{
private:
    HANDLE m_hReadPipe;
    HANDLE m_hWritePipe;

public:
    CPipe();
    ~CPipe();

    void CloseReadPipe();
    void CloseWritePipe();

    bool Peek(PVOID Buffer, DWORD BufferSize, PDWORD BytesRead, PDWORD TotalBytesAvailable);
    bool Read(PVOID Buffer, DWORD NumberOfBytesToRead, PDWORD NumberOfBytesRead);
    bool Write(LPCVOID Buffer, DWORD NumberOfBytesToWrite, PDWORD NumberOfBytesWritten);

    friend class CPipedProcess;
};
