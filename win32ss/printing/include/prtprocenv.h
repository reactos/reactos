/*
 * PROJECT:     ReactOS Printing Include files
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Provide a constant for the current Printing Processor Environment based on the architecture
 * COPYRIGHT:   Copyright 2016 Colin Finck (colin@reactos.org)
 */

#ifndef _REACTOS_PRTPROCENV_H
#define _REACTOS_PRTPROCENV_H

const WCHAR wszCurrentEnvironment[] =
#if defined(_X86_)
    L"Windows NT x86";
#elif defined(_AMD64_)
    L"Windows x64";
#elif defined(_ARM_)
    L"Windows ARM";
#else
    #error Unsupported architecture
#endif

const DWORD cbCurrentEnvironment = sizeof(wszCurrentEnvironment);

#endif
