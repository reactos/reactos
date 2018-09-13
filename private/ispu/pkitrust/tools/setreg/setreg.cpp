
//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       setreg.cpp
//
//  Contents:   Set Registry Key Values
//
//              See Usage() for syntax and list of options.
//
//  Functions:  main
//
//  History:    28-Jul-96   philh   created
//              02-May-97   xiaohs	updated for Localiztion and Consistency
//--------------------------------------------------------------------------


#include <windows.h>
#include <assert.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>
#include <wchar.h>
#include <stdarg.h>
#include "wintrust.h"
#include "cryptreg.h"
#include "resource.h"
#include "unicode.h"


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


//Global Data for loading the string
#define MAX_STRING_RSC_SIZE 512
#define OPTION_SWITCH_SIZE	5


HMODULE	hModule=NULL;

WCHAR	wszBuffer[MAX_STRING_RSC_SIZE];
DWORD	dwBufferSize=sizeof(wszBuffer)/sizeof(wszBuffer[0]); 

WCHAR	wszBuffer2[MAX_STRING_RSC_SIZE];
WCHAR	wszBuffer3[MAX_STRING_RSC_SIZE];

//Global Data for wchar version of the registry path.


//---------------------------------------------------------------------------
// The private version of _wcsicmp
//----------------------------------------------------------------------------
int IDSwcsicmp(WCHAR *pwsz, int idsString)
{
	assert(pwsz);

	//load the string
	if(!LoadStringU(hModule, idsString, wszBuffer, dwBufferSize))
		return -1;

	return _wcsicmp(pwsz, wszBuffer);
}

//-------------------------------------------------------------------------
//
//	The private version of wprintf.  Input is an ID for a stirng resource
//  and the output is the standard output of wprintf.
//
//-------------------------------------------------------------------------
void IDSwprintf(int idsString, ...)
{
	va_list	vaPointer;

	va_start(vaPointer, idsString);

	//load the string
	LoadStringU(hModule, idsString, wszBuffer, dwBufferSize);

	vwprintf(wszBuffer,vaPointer);

	return;
}	


void IDS_IDS_DWwprintf(int idString, int idStringTwo, DWORD dw)
{
	//load the string
	LoadStringU(hModule, idString, wszBuffer, dwBufferSize);

	//load the string two
	LoadStringU(hModule, idStringTwo, wszBuffer2, dwBufferSize);

	wprintf(wszBuffer,wszBuffer2,dw);

	return;
}



void IDS_IDSwprintf(int idString, int idStringTwo)
{
	//load the string
	LoadStringU(hModule, idString, wszBuffer, dwBufferSize);

	//load the string two
	LoadStringU(hModule, idStringTwo, wszBuffer2, dwBufferSize);

	wprintf(wszBuffer,wszBuffer2);

	return;
}

void IDS_DW_IDS_IDSwprintf(int ids1,DWORD dw,int ids2,int ids3)
{


	//load the string
	LoadStringU(hModule, ids1, wszBuffer, dwBufferSize);

	//load the string two
	LoadStringU(hModule, ids2, wszBuffer2, dwBufferSize); 

	//load the string three
   	LoadStringU(hModule, ids3, wszBuffer3, dwBufferSize); 

	wprintf(wszBuffer,dw,wszBuffer2,wszBuffer3,dw);

	return;
}

//---------------------------------------------------------------------------
//
// Convert STR to WSTR
//---------------------------------------------------------------------------
BOOL SZtoWSZ(LPSTR szStr,LPWSTR *pwsz)
{
	DWORD	dwSize=0;

	assert(pwsz);

	*pwsz=NULL;

	//return NULL
	if(!szStr)
		return TRUE;

	dwSize=MultiByteToWideChar(0, 0,szStr, -1,NULL,0);

	if(dwSize==0)
		return FALSE;

	//allocate memory
	*pwsz=(LPWSTR)malloc(dwSize * sizeof(WCHAR));

	if(*pwsz==NULL)
		return FALSE;

	if(MultiByteToWideChar(0, 0,szStr, -1,
		*pwsz,dwSize))
	{
		return TRUE;
	}
	
	free(*pwsz);	 

	return FALSE;
}



//---------------------------------------------------------------------------
//	 Get the hModule hanlder and init two DLLMain.
//	 
//---------------------------------------------------------------------------
BOOL	InitModule()
{
	if(!(hModule=GetModuleHandle(NULL)))
	   return FALSE;
	
	return TRUE;
}


//---------------------------------------------------------------------------
//	Dispaly the usage 
//	 
//---------------------------------------------------------------------------

static void Usage(void)
{
	IDSwprintf(IDS_SYNTAX);
	IDSwprintf(IDS_OPTIONS);
	IDS_IDSwprintf(IDS_OPTION_Q_DESC, IDS_OPTION_Q);
	IDSwprintf(IDS_ENDLN);
	IDSwprintf(IDS_CHOICES);

    for (int i = 0; i < NSOFTPUBFLAGS; i++) 
    {
        IDS_IDS_DWwprintf(IDS_DESC,SoftPubFlags[i].idsName,(i+1)); 
    }

	IDSwprintf(IDS_VALUE);
	IDSwprintf(IDS_ENDLN);
}


//---------------------------------------------------------------------------
//	 Display Software Publisher State Key Value
//	 
//---------------------------------------------------------------------------
static void DisplaySoftPubKeys()
{
    DWORD	dwState = 0;
    LONG	lErr;
    HKEY	hKey = NULL;
	DWORD	dwType;
    DWORD	cbData = sizeof(dwState);
   // WCHAR	wszState[10];
	int		i=0;
    LPWSTR  wszState=REGNAME_WINTRUST_POLICY_FLAGS;

	//If load string failed, no need to flag the failure since
	//no output is possible
//	if(!LoadStringU(hModule, IDS_KEY_STATE,wszState, 10))
	//	return;


    lErr = RegOpenHKCUKeyExU(
            HKEY_CURRENT_USER,
            REGPATH_WINTRUST_POLICY_FLAGS,
            0,          // dwReserved
            KEY_READ,
            &hKey);

    if (ERROR_SUCCESS != lErr) 
	{
        if (lErr == ERROR_FILE_NOT_FOUND)
			IDSwprintf(IDS_NO_VALUE,REGPATH_WINTRUST_POLICY_FLAGS,NULL); 
        else
			IDSwprintf(IDS_REG_OPEN_FAILED,
                REGPATH_WINTRUST_POLICY_FLAGS, L" ", lErr);
		
		return;
    } 


    lErr = RegQueryValueExU(
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
			 IDSwprintf(IDS_NO_VALUE, REGPATH_WINTRUST_POLICY_FLAGS,NULL);
         else
			 IDSwprintf(IDS_REG_QUERY_FAILED, REGPATH_WINTRUST_POLICY_FLAGS,NULL, lErr);

        goto CLEANUP;

	} 

    //
    //  04-Aug-1997 pberkman:
    //      added check for reg_binary because on WIN95 OSR2 when the machine is changed 
    //      from mutli-user profiles to single user profile, the registry DWORD values 
    //      change to BINARY
    //
	if ((dwType != REG_DWORD) &&
        (dwType != REG_BINARY))
	{

		IDSwprintf(IDS_WRONG_TYPE, REGPATH_WINTRUST_POLICY_FLAGS,NULL, dwType);
		goto CLEANUP;

    }

	IDSwprintf(IDS_STATE, dwState);

    for (i=0; i < NSOFTPUBFLAGS; i++) 
	{
        BOOL fOn = (dwState & SoftPubFlags[i].dwMask);

        int		idsValue;

        switch(SoftPubFlags[i].dwMask) 
		{
            case WTPF_IGNOREREVOCATIONONTS:
			case WTPF_IGNOREREVOKATION:
			case WTPF_IGNOREEXPIRATION:
            // Revocation is a double negative so the bit set
            // means revocation is off.
				idsValue= fOn ? IDS_FALSE : IDS_TRUE;
				break;

			default:
				idsValue = fOn ? IDS_TRUE : IDS_FALSE;
        };

        if (i < 9)
        {
		    IDS_DW_IDS_IDSwprintf(IDS_DISPLAY_LT_10, (i + 1), SoftPubFlags[i].idsName, idsValue);
        }
        else
        {
		    IDS_DW_IDS_IDSwprintf(IDS_DISPLAY, (i + 1), SoftPubFlags[i].idsName, idsValue);
        }
    }

CLEANUP:
    if (hKey != NULL)
        RegCloseKey(hKey);
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
//	WCHAR	wszState[10];
    LPWSTR  wszState=L"State";

	//If load string failed, no need to flag the failure since
	//no output is possible
//	if(!LoadStringU(hModule, IDS_KEY_STATE,wszState, 10))
//		return;

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
		IDSwprintf(IDS_REG_CREATE_FAILED, REGPATH_WINTRUST_POLICY_FLAGS, L" ", lErr);
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
			 IDSwprintf(IDS_NO_VALUE,REGPATH_WINTRUST_POLICY_FLAGS,NULL);
        }
        else
        {
			 IDSwprintf(IDS_REG_QUERY_FAILED,REGPATH_WINTRUST_POLICY_FLAGS,NULL, lErr);
             goto CLEANUP;
        }

    } 
    else if ((dwType != REG_DWORD) && (dwType != REG_BINARY))
	{
		IDSwprintf(IDS_WRONG_TYPE,REGPATH_WINTRUST_POLICY_FLAGS,NULL, dwType);

        goto CLEANUP;
    }

    switch(dwMask) {
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

    if (ERROR_SUCCESS != lErr)
		IDSwprintf(IDS_REG_SET_FAILED, lErr);

CLEANUP:
	if(hKey)
		RegCloseKey(hKey);
}


//---------------------------------------------------------------------------
//	 wmain
//	 
//---------------------------------------------------------------------------
extern "C" int __cdecl wmain(int argc, WCHAR ** wargv) 
{
    int		ReturnStatus = 0;
    LPWSTR	*prgwszKeyName = NULL;
    LPWSTR	*prgwszValue = NULL;
	DWORD	dwIndex=0;
	DWORD	dwCountKey=0;
	DWORD	dwCountValue=0;
    DWORD	dwMask = 0;
    BOOL	fOn=TRUE;
    BOOL	fQuiet = FALSE;
	DWORD	dwEntry=0;
	WCHAR	*pArg=NULL;

	WCHAR	wszSwitch1[OPTION_SWITCH_SIZE];
	WCHAR	wszSwitch2[OPTION_SWITCH_SIZE];


	//get the module handle
	if(!InitModule())
		return -1;

	//load the strings necessary for parsing the parameters
	if( !LoadStringU(hModule, IDS_SWITCH1,	wszSwitch1, OPTION_SWITCH_SIZE)
	  ||!LoadStringU(hModule, IDS_SWITCH2,  wszSwitch2,	OPTION_SWITCH_SIZE)
	  )
		return -1;

	//convert the multitype registry path to the wchar version

	prgwszKeyName=(LPWSTR *)malloc(sizeof(LPWSTR)*argc);
	prgwszValue=(LPWSTR *)malloc(sizeof(LPWSTR)*argc);

	if(!prgwszKeyName || !prgwszValue)
	{
		IDSwprintf(IDS_FAILED);
		ReturnStatus = -1;
		goto CommonReturn;

	}

	//memset
	memset(prgwszKeyName, 0, sizeof(LPWSTR)*argc);
	memset(prgwszValue, 0, sizeof(LPWSTR)*argc);

    while (--argc>0)
    {
		pArg=*++wargv;

        if (*pArg == *wszSwitch1 || *pArg == *wszSwitch2)
        {
            if(IDSwcsicmp(&(pArg[1]),IDS_OPTION_Q)==0)
				fQuiet = TRUE;
			else
				goto BadUsage;
        } 
		else 
		{
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

     }

	if(dwCountKey!=dwCountValue)
	{
		IDSwprintf(IDS_MANY_ARG);
		goto BadUsage;
	}

    
	if(dwCountKey==0)
	{
	 	//Display the Software Publisher State Key Values
        DisplaySoftPubKeys();
        goto CommonReturn;
	}


	for(dwIndex=0; dwIndex<dwCountKey; dwIndex++)
	{
		dwEntry = _wtoi(prgwszKeyName[dwIndex]);

		if(dwEntry < 1 || dwEntry > NSOFTPUBFLAGS+1) 
		{
			IDSwprintf(IDS_INVALID_CHOICE);
			goto BadUsage;
		}           
 
		//get the Key mask
		dwMask = SoftPubFlags[dwEntry-1].dwMask;

		if (0 == IDSwcsicmp(prgwszValue[dwIndex], IDS_TRUE))
			fOn = TRUE;
		else if (0 == IDSwcsicmp(prgwszValue[dwIndex], IDS_FALSE))
			fOn = FALSE;
		else 
		{
			IDSwprintf(IDS_BAD_VALUE);
			goto BadUsage;
		}

		SetSoftPubKey(dwMask, fOn);
	}

    if (!fQuiet) 
	{
		IDSwprintf(IDS_UPDATED);
        DisplaySoftPubKeys();
    }

    goto CommonReturn;

BadUsage:
    Usage();
    ReturnStatus = -1;
CommonReturn:
	//free the memory

	if(prgwszKeyName)
		free(prgwszKeyName);

	if(prgwszValue)
		free(prgwszValue);

    return ReturnStatus;
}
