
//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       iesetreg.cpp
//
//  Contents:   Set Registry Key Values
//
//              See Usage() for syntax and list of options.
//
//  Functions:  wmain
//
//  History:    28-Jul-96   philh   created
//              02-May-97   xiaohs	updated for Localiztion and Consistency
//				28-July-97	xiaohs  reduce size for ie
//              31-Oct-97   pberkman    changed to be a Windows App instead of Console.
//--------------------------------------------------------------------------


#include <windows.h>
#include <assert.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>
#include <wchar.h>
#include "malloc.h"
#include "wintrust.h"
#include "cryptreg.h"
#include "unicode.h"
#include "resource.h"


typedef struct _FlagNames
{
    int			idsName;
    DWORD       dwMask;
} FlagNames;


static FlagNames SoftPubFlags[] =
{
    IDS_NAME_TEST_ROOT,				WTPF_TRUSTTEST | WTPF_TESTCANBEVALID,
    IDS_NAME_EXPIRATION,			WTPF_IGNOREEXPIRATION,
    IDS_NAME_REVOCATION,			WTPF_IGNOREREVOKATION,
    IDS_NAME_OFFLINE_INDIVIDUAL,	WTPF_OFFLINEOK_IND,
    IDS_NAME_OFFLINE_COMMERCIAL,	WTPF_OFFLINEOK_COM,
    IDS_NAME_JAVA_INDIVIDUAL,		WTPF_OFFLINEOKNBU_IND,
    IDS_NAME_JAVA_COMMERCIAL,		WTPF_OFFLINEOKNBU_COM,
    IDS_NAME_VERSION_ONE,			WTPF_VERIFY_V1_OFF,
    IDS_NAME_REVOCATIONONTS,        WTPF_IGNOREREVOCATIONONTS,
    IDS_NAME_ALLOWONLYPERTRUST,     WTPF_ALLOWONLYPERTRUST
};
#define NSOFTPUBFLAGS (sizeof(SoftPubFlags)/sizeof(SoftPubFlags[0]))

HMODULE	hModule=NULL;

static BOOL IsWinNt(void) {

    static BOOL fIKnow = FALSE;
    static BOOL fIsWinNT = FALSE;

    OSVERSIONINFO osVer;

    if(fIKnow)
        return(fIsWinNT);

    memset(&osVer, 0, sizeof(OSVERSIONINFO));
    osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if( GetVersionEx(&osVer) )
        fIsWinNT = (osVer.dwPlatformId == VER_PLATFORM_WIN32_NT);

    // even on an error, this is as good as it gets
    fIKnow = TRUE;

   return(fIsWinNT);
}


int __cdecl _mywcsicmp(const wchar_t * wsz1, const wchar_t * wsz2)
//
// REVIEW: Who calls this function, and should they be doing so?
//
// Return:
//       <0 if wsz1 < wsz2
//        0 if wsz1 = wsz2
//       >0 if wsz1 > wsz2
    {
    if(IsWinNt())
        {
        //
        // Just do the Unicode compare
        //
        return lstrcmpiW(wsz1, wsz2);
        }
    else
        {
        //
        // Convert to multibyte and let the system do it
        //
        int cch1 = lstrlenW(wsz1);
        int cch2 = lstrlenW(wsz2);
        int cb1 = (cch1+1) * sizeof(WCHAR);
        int cb2 = (cch2+1) * sizeof(WCHAR);
        char* sz1= (char*) _alloca(cb1);
        char* sz2= (char*) _alloca(cb2);
        WideCharToMultiByte(CP_ACP, 0, wsz1, -1, sz1, cb1, NULL, NULL);
        WideCharToMultiByte(CP_ACP, 0, wsz2, -1, sz2, cb2, NULL, NULL);

        return lstrcmpiA(sz1, sz2);
        }
    }


//---------------------------------------------------------------------------
//	 Set Software Publisher State Key Value
//	
//---------------------------------------------------------------------------
static void SetSoftPubKey(DWORD dwMask, BOOL fOn)
{
    DWORD	dwState;
    LONG	lErr;
    HKEY	hKey;
    DWORD	dwDisposition;
    DWORD	dwType;
    DWORD	cbData;
	//WCHAR	wszState[10];
    LPWSTR  wszState=REGNAME_WINTRUST_POLICY_FLAGS;

	//If load string failed, no need to flag the failure since
	//no output is possible
//	if(!LoadStringU(hModule, IDS_KEY_STATE,wszState, 10))
	//	return;


    // Set the State in the registry
    if (ERROR_SUCCESS != (lErr = RegCreateKeyExU(
            HKEY_CURRENT_USER,
            REGPATH_WINTRUST_POLICY_FLAGS,
            0,          // dwReserved
            NULL,       // lpszClass
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            NULL,       // lpSecurityAttributes
            &hKey,
            &dwDisposition)))
	{
        return;
    }

    dwState = 0;
    cbData = sizeof(dwState);
    lErr = RegQueryValueExU
	(
        hKey,
        wszState,
        0,          // dwReserved
        &dwType,
        (BYTE *) &dwState,
        &cbData
        );


    if (ERROR_SUCCESS != lErr)
	{
        if (lErr == ERROR_FILE_NOT_FOUND)
        {
             dwState = 0;
		}
		else
		{
			goto CLEANUP;
		}

    }
	else if ((dwType != REG_DWORD) && (dwType != REG_BINARY))
	{
        goto CLEANUP;
    }

    switch(dwMask)
	{
    case WTPF_IGNOREREVOCATIONONTS:
    case WTPF_IGNOREREVOKATION:
    case WTPF_IGNOREEXPIRATION:
        // Revocation and expiration are a double negative so the bit set
        // means revocation and expriation checking is off.
        fOn = !fOn;
        break;
    default:
        break;
    };

    if (fOn)
        dwState |= dwMask;
    else
        dwState &= ~dwMask;

    lErr = RegSetValueExU(
        hKey,
        wszState,
        0,          // dwReserved
        REG_DWORD,
        (BYTE *) &dwState,
        sizeof(dwState)
        );


CLEANUP:
	if(hKey)
		RegCloseKey(hKey);
}


//---------------------------------------------------------------------------
//	 wmain
//	
//---------------------------------------------------------------------------

#define MAX_ARGV_PARAMS         32

extern "C" int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd)
{
    WCHAR       *wargv1[MAX_ARGV_PARAMS];
    WCHAR       **wargv;
    WCHAR       *pwsz;
    int         argc;
    WCHAR       wszExeName[MAX_PATH];

    memset(wargv1, 0x00, sizeof(WCHAR *) * MAX_ARGV_PARAMS);
    wargv = &wargv1[0];

    wszExeName[0] = NULL;
    GetModuleFileNameU(GetModuleHandle(NULL), &wszExeName[0], MAX_PATH);

    argc        = 1;
    wargv[0]    = &wszExeName[0];
    wargv[1]    = NULL;

    if (lpCmdLine)
    {
        while (*lpCmdLine == L' ')
        {
            lpCmdLine++;
        }

        if (*lpCmdLine)
        {
            wargv[argc] = lpCmdLine;
            argc++;
            wargv[argc] = NULL;
        }
    }

    pwsz        = lpCmdLine;

    while ((pwsz) && (*pwsz) && (argc < MAX_ARGV_PARAMS))
    {
        if (*pwsz == L' ')
        {
            *pwsz = NULL;
            pwsz++;

            while (*pwsz == L' ')
            {
                pwsz++;
            }

            wargv[argc] = pwsz;
            argc++;
        }

        pwsz++;
    }

    //
    //  now that we have argv/argc style params, go into existing code ...
    //

    int		ReturnStatus = 0;

    LPWSTR	*prgwszKeyName=NULL;
    LPWSTR	*prgwszValue=NULL;
	DWORD	dwIndex=0;
	DWORD	dwCountKey=0;
	DWORD	dwCountValue=0;
    DWORD	dwMask = 0;
    BOOL	fOn=TRUE;
    BOOL	fQuiet = FALSE;
	DWORD	dwEntry=0;
	WCHAR	*pArg=NULL;
	WCHAR	wszTRUE[10];
	WCHAR	wszFALSE[10];


	if(!(hModule=GetModuleHandle(NULL)))
	{
		ReturnStatus=-1;
		goto CommonReturn;
	}

	//load the string
	if(!LoadStringU(hModule, IDS_TRUE, wszTRUE, 10) ||
		!LoadStringU(hModule, IDS_FALSE, wszFALSE, 10))
	{
		ReturnStatus=-1;
		goto CommonReturn;
	}

	//convert the multitype registry path to the wchar version
	prgwszKeyName=(LPWSTR *)malloc(sizeof(LPWSTR)*argc);
	prgwszValue=(LPWSTR *)malloc(sizeof(LPWSTR)*argc);

	if(!prgwszKeyName || !prgwszValue)
	{
		ReturnStatus = -1;
		goto CommonReturn;

	}

	//memset
	memset(prgwszKeyName, 0, sizeof(LPWSTR)*argc);
	memset(prgwszValue, 0, sizeof(LPWSTR)*argc);  	

    while (--argc>0)
    {
		pArg=*++wargv;

		if(dwCountKey==dwCountValue)
		{
			prgwszKeyName[dwCountKey]=pArg;
			dwCountKey++;
		}
		else
		{
			if(dwCountKey==(dwCountValue+1))
			{
				prgwszValue[dwCountValue]=pArg;
				dwCountValue++;
			}
			else
			{
				goto BadUsage;
			}
		}

     }

	if(dwCountKey!=dwCountValue)
	{
		goto BadUsage;
	}


	if(dwCountKey==0)
	{
	 	//Display the Software Publisher State Key Values
        //DisplaySoftPubKeys();
        goto CommonReturn;
	}


	for(dwIndex=0; dwIndex<dwCountKey; dwIndex++)
	{
		
		//the choice has to be one character long
		if((prgwszKeyName[dwIndex][0]==L'1') && (prgwszKeyName[dwIndex][1]==L'0') &&
			(prgwszKeyName[dwIndex][2]==L'\0'))
			dwEntry=10;
		else
		{
			if(prgwszKeyName[dwIndex][1]!=L'\0')
				goto BadUsage;

			//get the character
			dwEntry=(ULONG)(prgwszKeyName[dwIndex][0])-(ULONG)(L'0');
		}

		if((dwEntry < 1) || (dwEntry > NSOFTPUBFLAGS+1))
			goto BadUsage;

		//get the Key mask
		dwMask = SoftPubFlags[dwEntry-1].dwMask;

		if (0 == _mywcsicmp(prgwszValue[dwIndex], wszTRUE))
			fOn = TRUE;
		else if (0 == _mywcsicmp(prgwszValue[dwIndex], wszFALSE))
			fOn = FALSE;
		else
		{
			goto BadUsage;
		}

		SetSoftPubKey(dwMask, fOn);
	}


    goto CommonReturn;

BadUsage:
    ReturnStatus = -1;
CommonReturn:
	//free the memory
	if(prgwszKeyName)
		free(prgwszKeyName);

	if(prgwszValue)
		free(prgwszValue);	

    return ReturnStatus;
}
