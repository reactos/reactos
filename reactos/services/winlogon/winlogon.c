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
 * 	GuiMonitor
 * 	
 * DESCRIPTION
 * 	Graphical monitor procedure
 */
VOID
GuiMonitor (VOID)
{
	/* FIXME: Open Monitor dialog */
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
	/*
	 * 
	 */
	NtCreateProcess(
		L"\\\\??\\C:\\reactos\\system\\userinit.exe",
		);
	/*
	 *	Security issue: buffers are cleared.
	 */
	NtZeroMemory(username, sizeof username);
	NtZeroMemory(password, sizeof password);
}


/***********************************************************************
 * 	CuiMonitor
 * 	
 * DESCRIPTION
 * 	Text mode (console) Monitor procedure
 */
VOID
CuiMonitor (VOID)
{
	WCHAR	HostName [64];
	WCHAR	UserName [64];
	WCHAR	FormattedDate [64];
	WCHAR	InputKey = L'\0';

	/* FIXME: query the system to get these info */
	wcscpy( HostName, L"BACH" );
	wcscpy( UserName, L"Administrator" );

	/* FIXME: use locale info to format */
	NtGetSystemtime(
		);

	/* Print info and Monitor menu */
	NtDisplayString(L"\
ReactOS Security:\n\
\tYou are logged on as %s/%s\n\
\yLogon date: %s\n\n\
Use the Task Manager to close an application that is not responding.\n\n\
1) Lock Workstation\n\
2) Change Password\n\
3) Logoff...\n\
4) Task Manager...\n\
5) Shut Down...\n\
6) Cancel\n\n? ",
		HostName,
		UserName,
		FormattedDate
		);
	while (TRUE)
	{
		/* FIXME: get a char and perform the requested action */
		switch (InputKey)
		{
		case L'1':
			DisplayString(L"Workstation locked...\n");
			return;
		case L'2':
			DisplayString(L"Changing Password:\n");
			return;
		case L'3':
			DisplayString(L"Logging off...\n");
			return;
		case L'4':
			DisplayString(L"Task Manager:\n");
			return;
		case L'5':
			DisplayString(L"Shutting Down:\n");
			DisplayString(L"1) Shutdown\n");
			DisplayString(L"2) Restart\n");
			DisplayString(L"3) Logoff\n");
			DisplayString(L"4) Cancel\n");
			return;
		case 27L: /* ESC */
		case L'6':
			return;
		default:
			DisplayString(L"Invalid key (1-6).\n");
		}
	}
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
		 * the logon procedure; otherwise open
		 * the monitor dialog.
		 */
		if (TRUE == HasSystemActiveSession())
		{
			/* MONITOR */
			if (TRUE == HasSystemGui())
			{
				/* GUI active: monitor in graphical mode */
				GuiMonitor();
				continue;
			}
			/* No GUI: monitor in text mode */
			CuiMonitor();
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
