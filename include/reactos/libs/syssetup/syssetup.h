/* $Id$
*/
/*
 * syssetup.h
 *
 * System setup API, native interface
 *
 * This file is part of the ReactOS Operating System.
 *
 * Contributors:
 *  Created by Eric Kohl <ekohl@rz-online.de>
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

#define SYSSETUP_SEVERITY_INFORMATION   0
#define SYSSETUP_SEVERITY_WARNING       1
#define SYSSETUP_SEVERITY_ERROR         2
#define SYSSETUP_SEVERITY_FATAL_ERROR   3


BOOL WINAPI
InitializeSetupActionLog(IN BOOL bDeleteOldLogFile);

VOID WINAPI
TerminateSetupActionLog(VOID);

BOOL WINAPI
SYSSETUP_LogItem(IN const LPSTR lpFileName,
                 IN DWORD dwLineNumber,
                 IN DWORD dwSeverity,
                 IN LPWSTR lpMessageText);

#define LogItem(dwSeverity, lpMessageText) \
    SYSSETUP_LogItem(__FILE__, __LINE__, dwSeverity, lpMessageText)

#endif /* __SYSSETUP_H_INCLUDED__ */

/* EOF */
