#ifndef __MSGINA_H
#define __MSGINA_H

#include <windows.h>
#include <winwlx.h>
#include "resource.h"

typedef struct {
	HANDLE hWlx;
	LPWSTR station;
	PWLX_DISPATCH_VERSION_1_3 pWlxFuncs;
	HANDLE hDllInstance;
	HANDLE UserToken;
	HWND hStatusWindow;
	BOOL SignaledStatusWindowCreated;
	BOOL DoAutoLogonOnce;

	/* Informations to be filled during logon */
	PLUID pAuthenticationId;
	PDWORD pdwOptions;
	PHANDLE phToken;
	PWLX_MPR_NOTIFY_INFO pNprNotifyInfo;
	PVOID *pProfile;

	/* Current logo to display */
	HBITMAP hBitmap;
} GINA_CONTEXT, *PGINA_CONTEXT;

HINSTANCE hDllInstance;

typedef BOOL (*PFGINA_INITIALIZE)(PGINA_CONTEXT);
typedef BOOL (*PFGINA_DISPLAYSTATUSMESSAGE)(PGINA_CONTEXT, HDESK, DWORD, PWSTR, PWSTR);
typedef VOID (*PFGINA_DISPLAYSASNOTICE)(PGINA_CONTEXT);
typedef INT (*PFGINA_LOGGEDONSAS)(PGINA_CONTEXT, DWORD);
typedef INT (*PFGINA_LOGGEDOUTSAS)(PGINA_CONTEXT);
typedef struct _GINA_UI
{
	PFGINA_INITIALIZE Initialize;
	PFGINA_DISPLAYSTATUSMESSAGE DisplayStatusMessage;
	PFGINA_DISPLAYSASNOTICE DisplaySASNotice;
	PFGINA_LOGGEDONSAS LoggedOnSAS;
	PFGINA_LOGGEDOUTSAS LoggedOutSAS;
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
