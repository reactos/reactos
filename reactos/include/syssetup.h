/* $Id: syssetup.h,v 1.1 2003/05/02 18:07:10 ekohl Exp $
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

DWORD STDCALL
InstallReactOS (HINSTANCE hInstance);


/* Log File APIs */

#define SEVERITY_INFORMATION   0
#define SEVERITY_WARNING       1
#define SEVERITY_ERROR         2
#define SEVERITY_FATAL_ERROR   3


BOOL STDCALL
InitializeSetupActionLog (BOOL bDeleteOldLogFile);

VOID STDCALL
TerminateSetupActionLog (VOID);

BOOL STDCALL
LogItem (DWORD dwSeverity,
	 LPWSTR lpMessageText);


#endif /* __SYSSETUP_H_INCLUDED__ */

/* EOF */
