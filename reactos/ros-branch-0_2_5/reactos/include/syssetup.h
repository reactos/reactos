/* $Id: syssetup.h,v 1.2 2004/01/14 22:15:09 gvg Exp $
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

#define SYSSETUP_SEVERITY_INFORMATION   0
#define SYSSETUP_SEVERITY_WARNING       1
#define SYSSETUP_SEVERITY_ERROR         2
#define SYSSETUP_SEVERITY_FATAL_ERROR   3


BOOL STDCALL
InitializeSetupActionLog (BOOL bDeleteOldLogFile);

VOID STDCALL
TerminateSetupActionLog (VOID);

BOOL STDCALL
LogItem (DWORD dwSeverity,
	 LPWSTR lpMessageText);


#endif /* __SYSSETUP_H_INCLUDED__ */

/* EOF */
