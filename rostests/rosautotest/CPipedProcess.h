/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Class that creates a process and redirects its output to a pipe
 * COPYRIGHT:   Copyright 2015 Thomas Faber <thomas.faber@reactos.org>
 */

class CPipedProcess : public CProcess
{
private:
    STARTUPINFOW m_StartupInfo;

    LPSTARTUPINFOW InitStartupInfo(CPipe& Pipe);

public:
    CPipedProcess(const wstring& CommandLine, CPipe& Pipe);
};
