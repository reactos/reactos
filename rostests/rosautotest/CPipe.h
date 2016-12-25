/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Class that managed an unidirectional anonymous byte stream pipe
 * COPYRIGHT:   Copyright 2015 Thomas Faber <thomas.faber@reactos.org>
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
