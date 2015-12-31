/*
 * syssetup.h
 *
 * System setup API, native interface
 *
 * This file is part of the ReactOS Operating System.
 *
 * Contributors:
 *  Created by Eric Kohl
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __SYSSETUP_H_INCLUDED__
#define __SYSSETUP_H_INCLUDED__

/* System setup APIs */

DWORD WINAPI
InstallReactOS (HINSTANCE hInstance);


/* Log File APIs */

BOOL WINAPI
InitializeSetupActionLog(IN BOOL bDeleteOldLogFile);

VOID WINAPI
TerminateSetupActionLog(VOID);

VOID
CDECL
pSetupDebugPrint(
    IN PCWSTR pszFileName,
    IN INT nLineNumber,
    IN PCWSTR pszTag,
    IN PCWSTR pszMessage,
    ...);

#define __WFILE__ TOWL1(__FILE__)
#define TOWL1(p) TOWL2(p)
#define TOWL2(p) L##p

#define LogItem(lpTag, lpMessageText...) \
    pSetupDebugPrint(__WFILE__, __LINE__, lpTag, lpMessageText)

#endif /* __SYSSETUP_H_INCLUDED__ */

/* EOF */
