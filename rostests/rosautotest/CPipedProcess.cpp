/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Class that creates a process and redirects its output to a pipe
 * COPYRIGHT:   Copyright 2015 Thomas Faber <thomas.faber@reactos.org>
 */

#include "precomp.h"

/**
 * Constructs a CPipedProcess object and starts the process with redirected output.
 *
 * @param CommandLine
 * A std::wstring containing the command line to run.
 *
 * @param Pipe
 * The CPipe instance to redirect the process's output to.
 * Note that only the read pipe is usable after the pipe was passed to this object.
 */
CPipedProcess::CPipedProcess(const wstring& CommandLine, CPipe& Pipe)
    : CProcess(CommandLine, InitStartupInfo(Pipe))
{
    Pipe.CloseWritePipe();
}

/**
 * Initializes the STARTUPINFO structure for use in CreateProcessW.
 *
 * @param Pipe
 * The CPipe instance to redirect the process's output to.
 */
LPSTARTUPINFOW
CPipedProcess::InitStartupInfo(CPipe& Pipe)
{
    ZeroMemory(&m_StartupInfo, sizeof(m_StartupInfo));
    m_StartupInfo.cb = sizeof(m_StartupInfo);
    m_StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    m_StartupInfo.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
    m_StartupInfo.hStdOutput = Pipe.m_hWritePipe;
    m_StartupInfo.hStdError  = Pipe.m_hWritePipe;
    return &m_StartupInfo;
}
