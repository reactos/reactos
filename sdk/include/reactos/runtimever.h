/*
 * PROJECT:     VerOS NT
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        sdk/include/reactos/runtimever.h
 * PURPOSE:     Runtime NT version reported to an application
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 */

#ifndef _RUNTIMEVER_H
#define _RUNTIMEVER_H

/* Windows 10 22H2. Independent of the 5.2 PE stamp and the 0.4 product version. */
#define RUNTIME_NT_MAJOR_VERSION 10
#define RUNTIME_NT_MINOR_VERSION 0
#define RUNTIME_NT_BUILD_NUMBER  19045

/* GetVersion lie for an image with no Windows 8.1 or Windows 10 manifest. */
#define RUNTIME_NT_LIE_MAJOR_VERSION 6
#define RUNTIME_NT_LIE_MINOR_VERSION 2
#define RUNTIME_NT_LIE_BUILD_NUMBER  9200

/* HKLM\...\CurrentVersion stays the Windows 8.1 compatibility string. */
#define RUNTIME_NT_CURRENT_VERSION_STRING "6.3"

/*
 * NtBuildNumber in KUSER_SHARED_DATA. The _WIN32_WINNT=0x502 view of that
 * slot is Reserved2[6].
 */
#define RUNTIME_NT_SHARED_BUILD_OFFSET 0x260

#endif /* _RUNTIMEVER_H */
