/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Class able to create a new process and closing its handles on destruction (exception-safe)
 * COPYRIGHT:   Copyright 2009 Colin Finck <colin@reactos.org>
 */

class CProcess
{
private:
    PROCESS_INFORMATION m_ProcessInfo;

public:
    CProcess(const wstring& CommandLine, LPSTARTUPINFOW StartupInfo);
    ~CProcess();

    HANDLE GetProcessHandle() const { return m_ProcessInfo.hProcess; }
};
