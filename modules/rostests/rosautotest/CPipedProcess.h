/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Class that creates a process and redirects its output to a pipe
 * COPYRIGHT:   Copyright 2015 Thomas Faber (thomas.faber@reactos.org)
 */

class CPipedProcess : public CProcess
{
private:
    STARTUPINFOW m_StartupInfo;

    LPSTARTUPINFOW InitStartupInfo(CPipe& Pipe);

public:
    CPipedProcess(const wstring& CommandLine, CPipe& Pipe);
};
