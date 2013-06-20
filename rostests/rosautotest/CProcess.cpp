/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Class able to create a new process and closing its handles on destruction (exception-safe)
 * COPYRIGHT:   Copyright 2009 Colin Finck <colin@reactos.org>
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
        FATAL("CreateProcessW failed\n");
}

/**
 * Destructs a CProcess object and closes all handles belonging to the process.
 */
CProcess::~CProcess()
{
    CloseHandle(m_ProcessInfo.hThread);
    CloseHandle(m_ProcessInfo.hProcess);
}
