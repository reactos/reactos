/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            services/winlogon/winlogon.c
 * PURPOSE:         Logon 
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
//#include <stdio.h>

/* GLOBALS ******************************************************************/

HANDLE	SM		= INVALID_HANDLE_VALUE;	/* SM API LPC port */
HANDLE	SasEvent	= INVALID_HANDLE_VALUE;	/* int 0x19 */


/* FUNCTIONS *****************************************************************/


/***********************************************************************
 *	SmTerminationRequestHandler
 */
VOID
STDCALL
SmTerminationRequestHandler (VOID)
{
	/* Should winlogon perform any action
	 * before committing suicide?
	 */
	NtTerminateProcess(
		NtCurrentProcess(),
		0
		);
}


/***********************************************************************
 * 	HasSystemGui
 * 	
 * DESCRIPTION
 * 	Call the Session Manager to know if user I/O is via a GUI or
 * 	via a CUI.
 *
 * RETURN VALUE
 * 	TRUE	GUI active
 * 	FALSE	CUI only
 */
BOOL
HasSystemGui (VOID)
{
	/* FIXME: call smss.exe to know, since it
	 * controls environment subsystem servers
	 * waking up. ReactOS has only text mode
	 * now, therefore we can answer (EA.19990608).
	 */
	return FALSE; /* NO GUI */
}


/***********************************************************************
 *	HasSystemActiveSession
 * 	
 * DESCRIPTION
 * 	Call the Session Manager to know if there is already an active
 * 	session.
 *
 * RETURN VALUE
 * 	TRUE	a session is active
 * 	FALSE	no sessions
 */
BOOL
HasSystemActiveSession (VOID)
{
	/* FIXME: call smss.exe to know */
	return FALSE; /* NO SESSIONS */
}


/***********************************************************************
 * 	GuiLogin
 * 	
 * DESCRIPTION
 * 	Graphical login procedure
 */
VOID
GuiLogin (VOID)
{
	/* FIXME: Open logon dialog */
}


/***********************************************************************
 * 	CuiLogin
 * 	
 * DESCRIPTION
 * 	Text mode (console) login procedure
 */
VOID
CuiLogin (VOID)
{
	char username[255];
	char password[255];

	/* FIXME: to be used ntdll.dll only? */
	printf("Winlogon\n");
	printf("login: ");
	fgets(username, 255, stdin);
	printf("Password: ");
	fgets(password, 255, stdin);
}


/* Native process entry point */
void
NtProcessStartup( PSTARTUP_ARGUMENT StartupArgument )
{
	NTSTATUS	Status = STATUS_SUCCESS;

	/* FIXME: connnect to the Session Manager
	 * for LPC calls
	 */
	Status = NtConnectPort(
			"\\SmApiPort",
			& SM
			);
	if (!NT_SUCCESS(Status))
	{
		NtTerminateProcess(
			NtCurrentProcess(),
			0 /* FIXME: return a proper value to SM */
			);
	}
	/* FIXME: register a termination callback
	 * for smss.exe
	 */
	/* ??? register SmTerminationRequestHandler */
	/* FIXME: hook Ctrl+Alt+Del (int 0x19)
	 * (SAS = Secure Attention Sequence)
	 */
	/* ??? SasEvent = ? */
	while (TRUE)
	{
		/*
		 * Make the main thread wait
		 * for SAS indefinitely.
		 */
		NtWaitForSingleObject(
			SasEvent
			/* ... */
			);
		/*
		 * If there is no local session, begin
		 * the login procedure; otherwise print
		 * display
		 */
		if (TRUE == HasSystemActiveSession())
		{
			continue;
		}
		/* LOGON */
		if (TRUE == HasSystemGui())
		{
			/* GUI active, login in graphical mode */
			GuiLogin();
			continue;
		}
		/* No GUI, login in console mode */
		CuiLogin();
	}
}


/* EOF */
