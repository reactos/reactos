/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Class able to create a new process and closing its handles on destruction (exception-safe)
 * COPYRIGHT:   Copyright 2009 Colin Finck (colin@reactos.org)
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
