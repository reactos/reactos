//****************************************************************************
//
//  Module:     RNAUI.DLL
//  File:       conutil.c
//  Content:    This file contains common container's utilities
//  History:
//      Tue 30-Nov-1993 07:42:02  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1991-1994
//
//****************************************************************************

#include "rnaui.h"
#include "contain.h"

#ifdef LINK_ENABLED

typedef struct tagENUMBROWSER
{
	DWORD dwData;
	ENUMOBJECTS lpfnEnumObjects;
	IShellView FAR * psv;
	FILEINFO fi;
} ENUMBROWSER ;

FILEINFO FAR * CALLBACK EnumBrowserObjects(ENUMBROWSER FAR *lpData, LPSTR pszObject, LPSTR pszSubObject, LPPOINT pptScreen)
{
	char szWorkingDir[MAXPATHLEN];

	if ((*(lpData->lpfnEnumObjects))(lpData->psv, &(lpData->dwData),
		&(lpData->fi), FCE_NEXTOBJECT | FCE_SELECTED, pszObject, pszSubObject,
		szWorkingDir, pptScreen))
	{
		return(&lpData->fi);
	}
	else
	{
		return(NULL);
	}
}


void FAR PASCAL Generic_HandleLink(IShellView FAR * psv, HWND hWnd, ENUMOBJECTS lpfnEnumObjects)
{
	ENUMBROWSER sData;

	sData.dwData = 0L;
	sData.lpfnEnumObjects = lpfnEnumObjects;
	sData.psv = psv;

	(*(sData.lpfnEnumObjects))(sData.psv, &(sData.dwData),
		NULL, FCE_INIT, NULL, NULL, NULL, NULL);

	Shell_HandleCommand(EnumBrowserObjects, &sData, hWnd, FCIDM_LINK);

	(*(sData.lpfnEnumObjects))(sData.psv, &(sData.dwData),
		NULL, FCE_TERM, NULL, NULL, NULL, NULL);
}

#endif  // LINK_ENABLED

// convert screen coords to listview view coordinates
//
void FAR PASCAL ScreenToLV(HWND hwndLV, LPPOINT ppt)
{
    POINT ptOrigin;

    ListView_GetOrigin(hwndLV, &ptOrigin);

    ScreenToClient(hwndLV, ppt);

    ppt->x += ptOrigin.x;
    ppt->y += ptOrigin.y;
}

