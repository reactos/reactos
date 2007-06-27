/*
 * PROJECT:         ReactOS International Control Panel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/intl/setupreg.c
 * PURPOSE:         ReactOS International Control Panel
 * PROGRAMMERS:     Alexey Zavyalov (gen_x@mail.ru)
*/
 
/* INCLUDES *****************************************************************/

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>

#include "intl.h"
#include "resource.h"

/* GLOBALS ******************************************************************/

#define NUM_SHEETS           4

/* FUNCTIONS ****************************************************************/

/* Insert the space  */
WCHAR*
InsSpacePos(const WCHAR *wszInsStr, const int nPos)
{
	WCHAR* pwszDestStr;
	pwszDestStr=(WCHAR*) malloc(MAX_SAMPLES_STR_SIZE);
    
    int nDestStrCnt=0;
    int nStrCnt;
    int nStrSize;

	wcscpy(pwszDestStr,wszInsStr);  
    
    nStrSize = wcslen(wszInsStr);

    for(nStrCnt=0; nStrCnt<nStrSize; nStrCnt++)
    {
        if(nStrCnt==nStrSize-nPos)
        {
            pwszDestStr[nDestStrCnt]=' ';
            nDestStrCnt++;
        }
        pwszDestStr[nDestStrCnt]=wszInsStr[nStrCnt];
        nDestStrCnt++;
    }
    pwszDestStr[nDestStrCnt]='\0';
    
    return pwszDestStr;
}

/* Insert the spaces by format string separated by ';' */
WCHAR*
InsSpacesFmt(const WCHAR *wszSourceStr, const WCHAR *wszFmtStr)
{
	WCHAR* pwszDestStr;
	pwszDestStr=(WCHAR*) malloc(255);

 	WCHAR* pwszTempStr;

    WCHAR wszFmtVal[255];

	int nFmtCount=0;
	int nValCount=0;
    int nLastVal=0;
    int nSpaceOffset=0;
    
    BOOL wasNul=FALSE;

	wcscpy(pwszDestStr,wszSourceStr);

    /* if format is clean return source string */
    if(!*wszFmtStr) return pwszDestStr;

    /* Search for all format values */
    for(nFmtCount=0;nFmtCount<=(int)wcslen(wszFmtStr);nFmtCount++)
    {
        if(wszFmtStr[nFmtCount]==';' || wszFmtStr[nFmtCount]=='\0')
        {
            if(_wtoi(wszFmtVal)==0 && !wasNul)
            {
                wasNul=TRUE;
                break;
            }
            /* If was 0, repeat spaces */
            if(wasNul)
            {
                nSpaceOffset+=nLastVal;
            }
            else
            {
                nSpaceOffset+=_wtoi(wszFmtVal);
            }
            wszFmtVal[nValCount]='\0';
            nValCount=0;
            /* insert space to finded position plus all pos before */
            pwszTempStr=InsSpacePos(pwszDestStr,nSpaceOffset);
            wcscpy(pwszDestStr,pwszTempStr);
            free(pwszTempStr);
            /* num of spaces total increment */

            if(!wasNul)
            {
                nSpaceOffset++;
                nLastVal=_wtoi(wszFmtVal);
            }
              
        }
        else
        {
            wszFmtVal[nValCount++]=wszFmtStr[nFmtCount];
        }
    }

    /* Create spaces for rest part of string */
    if(wasNul && nLastVal!=0)
    {
        for(nFmtCount=nSpaceOffset+nLastVal;nFmtCount<wcslen(pwszDestStr);nFmtCount+=nLastVal+1)
        {
            pwszTempStr=InsSpacePos(pwszDestStr,nFmtCount);
            wcscpy(pwszDestStr,pwszTempStr);
            free(pwszTempStr);
        }
    }

    return pwszDestStr;
}

/* Replace given template in source string with string to replace and return recieved string */
WCHAR*
ReplaceSubStr(const WCHAR *wszSourceStr,
              const WCHAR *wszStrToReplace,
              const WCHAR *wszTempl)
{
	int nCharCnt;
	int nSubStrCnt;
	int nDestStrCnt;
    int nFirstCharCnt;

	WCHAR* wszDestStr;
	wszDestStr=(WCHAR*) malloc(MAX_SAMPLES_STR_SIZE*sizeof(WCHAR));

	nDestStrCnt=0;
    nFirstCharCnt=0;

    wcscpy(wszDestStr,L"");

    while(nFirstCharCnt<(int)wcslen(wszSourceStr))
	{
        if(wszSourceStr[nFirstCharCnt]==wszTempl[0])
        {
            nSubStrCnt=0;
            for(nCharCnt=nFirstCharCnt;nCharCnt<nFirstCharCnt+(int)wcslen(wszTempl);nCharCnt++)
            {
                if(wszSourceStr[nCharCnt]==wszTempl[nSubStrCnt])
                {
                    nSubStrCnt++;
                }
                else
                {
                    break;
                }
                if((int)wcslen(wszTempl)==nSubStrCnt)
                {
                    wcscat(wszDestStr,wszStrToReplace);
                    nDestStrCnt=(int)wcslen(wszDestStr);
                    nFirstCharCnt+=(int)wcslen(wszTempl)-1;
                    break;
                }
            }
        }
        else 
        {
            wszDestStr[nDestStrCnt++]=wszSourceStr[nFirstCharCnt];
            wszDestStr[nDestStrCnt]='\0';
        }
        nFirstCharCnt++;
	}

	return wszDestStr;
}

static
VOID
InitPropSheetPage(PROPSHEETPAGE *PsPage, WORD IdDlg, DLGPROC DlgProc)
{
    ZeroMemory(PsPage, sizeof(PROPSHEETPAGE));
    PsPage->dwSize = sizeof(PROPSHEETPAGE);
    PsPage->dwFlags = PSP_DEFAULT;
    PsPage->hInstance = hApplet;
    PsPage->pszTemplate = MAKEINTRESOURCE(IdDlg);
    PsPage->pfnDlgProc = DlgProc;
}

/* Create applets */
LONG
APIENTRY
SetupApplet(HWND hwnd, UINT uMsg, LONG wParam, LONG lParam)
{

    PROPSHEETPAGE PsPage[NUM_SHEETS];
    PROPSHEETHEADER psh;
    TCHAR Caption[MAX_STR_SIZE];

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(hwnd);

    LoadString(hApplet, IDS_CPLNAME, Caption, sizeof(Caption) / sizeof(TCHAR));

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_USECALLBACK | PSH_PROPTITLE;
    psh.hwndParent = NULL;
    psh.hInstance = hApplet;
    psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDC_CPLICON));
    psh.pszCaption = Caption;
    psh.nPages = sizeof(PsPage) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = PsPage;

    InitPropSheetPage(&PsPage[0], IDD_NUMSOPTSSETUP, NumsOptsSetProc);
    InitPropSheetPage(&PsPage[1], IDD_CURRENCYOPTSSETUP, CurrencyOptsSetProc);
    InitPropSheetPage(&PsPage[2], IDD_TIMEOPTSSETUP, TimeOptsSetProc);
    InitPropSheetPage(&PsPage[3], IDD_DATEOPTSSETUP, DateOptsSetProc);

    return (LONG)(PropertySheet(&psh) != -1);
}

/* EOF */
