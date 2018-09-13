/*****************************************************************/
/**               Microsoft Windows for Workgroups              **/
/**           Copyright (C) Microsoft Corp., 1991-1992          **/
/*****************************************************************/

/*
    uiassert.c
    Environment specific stuff for the UIASSERT & REQUIRE macro

    This file contains the environment specific (windows vs. OS/2/DOS)
    features of the assert macro, specifically, the output method
    (everything is hidden by the standard C-Runtime).

    FILE HISTORY:
        johnl       10/17/90    Created
        johnl       10/18/90    Added OutputDebugString
        beng        04/30/91    Made a 'C' file
        beng        08/05/91    Withdrew expressions; reprototyped
                                all functions
        beng        09/17/91    Withdrew additional consistency checks
        beng        09/26/91    Withdrew stupid nprintf calls
        gregj       03/23/93    Ported to Chicago environment
*/

#include "npcommon.h"
#include "npassert.h"

extern "C" {

const CHAR szShouldBeAnsi[] = "String should be ANSI but isn't";
const CHAR szShouldBeOEM[] = "String should be OEM but isn't";

static CHAR szFmt0[] = "File %.40s, Line %u";
static CHAR szFmt1[] = "%.60s: File %.40s, Line %u";
static CHAR szMBCaption[] = "ASSERTION FAILED";
static CHAR szFAE[] = "ASSERTION FAILURE IN APP";

VOID UIAssertHelper(
    const CHAR* pszFileName,
    UINT    nLine )
{
    CHAR szBuff[sizeof(szFmt0)+60+40];

    wsprintf(szBuff, szFmt0, pszFileName, nLine);
    MessageBox(NULL, szBuff, szMBCaption,
           (MB_TASKMODAL | MB_ICONSTOP | MB_OK) );

    FatalAppExit(0, szFAE);
}


VOID UIAssertSzHelper(
    const CHAR* pszMessage,
    const CHAR* pszFileName,
    UINT    nLine )
{
    CHAR szBuff[sizeof(szFmt1)+60+40];

    wsprintf(szBuff, szFmt1, pszMessage, pszFileName, nLine);
    MessageBox(NULL, szBuff, szMBCaption,
           (MB_TASKMODAL | MB_ICONSTOP | MB_OK) );

    FatalAppExit(0, szFAE);
}

//========== Debug output routines =========================================

UINT uiNetDebugMask = 0xffff;

UINT WINAPI NetSetDebugMask(UINT mask)
{
#ifdef DEBUG
    UINT uiOld = uiNetDebugMask;
    uiNetDebugMask = mask;

    return uiOld;
#else
    return 0;
#endif
}

UINT WINAPI NetGetDebugMask()
{
#ifdef DEBUG
    return uiNetDebugMask;
#else
    return 0;
#endif
}

#ifndef WINCAPI
#ifdef WIN32
#define WINCAPI __cdecl
#else
#define WINCAPI __far __cdecl
#endif
#endif

#ifdef DEBUG

/* debug message output log file */

UINT 	g_uSpewLine = 0;
PCSTR 	g_pcszSpewFile = NULL;
CHAR	s_cszLogFile[MAX_PATH] = {'\0'};
CHAR	s_cszDebugName[MAX_PATH] = {'\0'};

UINT WINAPI  NetSetDebugParameters(PSTR pszName,PSTR pszLogFile)
{
	lstrcpy(s_cszLogFile,pszLogFile);
	lstrcpy(s_cszDebugName,pszName);

	return 0;
}


BOOL LogOutputDebugString(PCSTR pcsz)
{
   BOOL 	bResult = FALSE;
   UINT 	ucb;
   char 	rgchLogFile[MAX_PATH];

   if (IS_EMPTY_STRING(s_cszLogFile) )
	   return FALSE;

   ucb = GetWindowsDirectory(rgchLogFile, sizeof(rgchLogFile));

   if (ucb > 0 && ucb < sizeof(rgchLogFile)) {

      HANDLE hfLog;

      lstrcat(rgchLogFile, "\\");
      lstrcat(rgchLogFile, s_cszLogFile);

      hfLog = ::CreateFile(rgchLogFile,
						   GENERIC_WRITE,
						   0,
						   NULL,
						   OPEN_ALWAYS,
						   0,
						   NULL);

      if (hfLog != INVALID_HANDLE_VALUE) {

         if (SetFilePointer(hfLog, 0, NULL, FILE_END) != INVALID_FILE_SIZE) {
            DWORD dwcbWritten;

            bResult = WriteFile(hfLog, pcsz, lstrlen(pcsz), &dwcbWritten, NULL);

            if (! CloseHandle(hfLog) && bResult)
               bResult = FALSE;
         }
      }
   }

   return(bResult);
}

CHAR	*achDebugDisplayPrefix[] = {"t ","w ","e ","a ","t ","t ","t ","t ","t ","t ","t "};

void WINCAPI NetDebugMsg(UINT mask, LPCSTR pszMsg, ...)
{
    char 	ach[1024];
	UINT	uiDisplayMask = mask & 0xff;

	// Determine prefix
	*ach = '\0';
	if (uiNetDebugMask & DM_PREFIX) {
		// Add trace type
		::lstrcat(ach,achDebugDisplayPrefix[uiDisplayMask]);

		// Add component name
		::lstrcat(ach,s_cszDebugName);

		// Add thread ID
		CHAR	szThreadId[16];
		::wsprintf(szThreadId,"[%#lx] ",::GetCurrentThreadId());
		::lstrcat(ach,szThreadId);
	}

    ::wvsprintf(ach+::lstrlen(ach), pszMsg, (va_list)((&pszMsg) + 1));
	::lstrcat(ach,"\r\n");

	if (uiNetDebugMask & DM_LOG_FILE) {
		 LogOutputDebugString(ach);
	}

	// Check if we need to display this trace
    if (uiNetDebugMask & uiDisplayMask) {
        OutputDebugString(ach);
    }
}

#endif

}   /* extern "C" */


