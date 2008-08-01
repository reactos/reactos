#ifndef __MSGINA_H
#define __MSGINA_H

#include <windows.h>
#include <userenv.h>
#include <winwlx.h>
#include "resource.h"

/* Values for GINA_CONTEXT.AutoLogonState */
#define AUTOLOGON_CHECK_REGISTRY 1
#define AUTOLOGON_ONCE           2
#define AUTOLOGON_DISABLED       3

typedef struct
{
	HANDLE hWlx;
	LPWSTR station;
	PWLX_DISPATCH_VERSION_1_3 pWlxFuncs;
	HANDLE hDllInstance;
	HWND hStatusWindow;
	DWORD AutoLogonState;

	/* Informations to be filled during logon */
	HANDLE UserToken;
	PLUID pAuthenticationId;
	PDWORD pdwOptions;
	PWLX_MPR_NOTIFY_INFO pMprNotifyInfo;
	PVOID *pProfile;

	/* Current logo to display */
	HBITMAP hBitmap;
} GINA_CONTEXT, *PGINA_CONTEXT;

HINSTANCE hDllInstance;

typedef BOOL (*PFGINA_INITIALIZE)(PGINA_CONTEXT);
typedef BOOL (*PFGINA_DISPLAYSTATUSMESSAGE)(PGINA_CONTEXT, HDESK, DWORD, PWSTR, PWSTR);
typedef BOOL (*PFGINA_REMOVESTATUSMESSAGE)(PGINA_CONTEXT);
typedef VOID (*PFGINA_DISPLAYSASNOTICE)(PGINA_CONTEXT);
typedef INT (*PFGINA_LOGGEDONSAS)(PGINA_CONTEXT, DWORD);
typedef INT (*PFGINA_LOGGEDOUTSAS)(PGINA_CONTEXT);
typedef INT (*PFGINA_LOCKEDSAS)(PGINA_CONTEXT);
typedef struct _GINA_UI
{
	PFGINA_INITIALIZE Initialize;
	PFGINA_DISPLAYSTATUSMESSAGE DisplayStatusMessage;
	PFGINA_REMOVESTATUSMESSAGE RemoveStatusMessage;
	PFGINA_DISPLAYSASNOTICE DisplaySASNotice;
	PFGINA_LOGGEDONSAS LoggedOnSAS;
	PFGINA_LOGGEDOUTSAS LoggedOutSAS;
	PFGINA_LOCKEDSAS LockedSAS;
} GINA_UI, *PGINA_UI;

/* msgina.c */

BOOL
DoLoginTasks(
	IN OUT PGINA_CONTEXT pgContext,
	IN PWSTR UserName,
	IN PWSTR Domain,
	IN PWSTR Password);

#endif /* __MSGINA_H */

/* EOF */
