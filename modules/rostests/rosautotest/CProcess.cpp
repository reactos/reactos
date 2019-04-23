/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Class able to create a new process and closing its handles on destruction (exception-safe)
 * COPYRIGHT:   Copyright 2009-2019 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

/**
 * Constructs a CProcess object and uses the CreateProcessW function to start the process immediately.
 *
 * @param CommandLine
 * A std::wstring containing the command line to run
 *
 * @param StartupInfo
 * Pointer to a STARTUPINFOW structure containing process startup information
 */
CProcess::CProcess(const wstring& CommandLine, LPSTARTUPINFOW StartupInfo)
{
    auto_array_ptr<WCHAR> CommandLinePtr(new WCHAR[CommandLine.size() + 1]);

    wcscpy(CommandLinePtr, CommandLine.c_str());

    if(!CreateProcessW(NULL, CommandLinePtr, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, StartupInfo, &m_ProcessInfo))
        TESTEXCEPTION("CreateProcessW failed\n");
}

/**
 * Destructs a CProcess object, terminates the process if running, and closes all handles belonging to the process.
 */
CProcess::~CProcess()
{
    TerminateProcess(m_ProcessInfo.hProcess, 255);
    CloseHandle(m_ProcessInfo.hThread);
    CloseHandle(m_ProcessInfo.hProcess);
}
