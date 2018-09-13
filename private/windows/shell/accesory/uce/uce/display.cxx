/****************************************************************************

    Display.c

    PURPOSE: manage dispaly buffer for UCE

    Copyright (c) 1997-1999 Microsoft Corporation.
****************************************************************************/

#include "windows.h"
#include "commctrl.h"

#include "UCE.h"
#include "stdlib.h"
#include "tchar.h"
#include "stdio.h"
#include "winuser.h"
#include "string.h"
#include "search.h"
#include "getuname.h"

#include "winnls.h"
#include "wingdi.h"

LPWSTR Display_pList  = NULL;
INT    Display_nCount = 0;

/****************************************************************************

    Display_DeleteList

****************************************************************************/
BOOL
Display_DeleteList()
{
    if (Display_pList)
    {
        free(Display_pList);
    }
    Display_pList  = NULL;
    Display_nCount = 0;
    return TRUE;
}

/****************************************************************************

    Display_InitList

****************************************************************************/
BOOL
Display_InitList()
{
    Display_pList = NULL;
    Display_nCount = 0;
    return TRUE;
}

/****************************************************************************

    Display_IsInCMapTable
    Optimised verion using binary search [v-nirnay]  Dec 25, 1997

****************************************************************************/
__inline BOOL
Display_IsInCMapTable(
    WCHAR  wCode,
    URANGE *pFontCMapTab,
    UINT    uFontCMapTabNum
    )
{
    INT uFirst=0, uLast=uFontCMapTabNum, uMiddle;
    BOOL fFound=FALSE;

    while ((uFirst <= uLast) && (fFound == FALSE))
    {
        uMiddle = (uFirst + uLast) / 2;
        if ((wCode >= pFontCMapTab[uMiddle].wcFrom) &&
            (wCode <= pFontCMapTab[uMiddle].wcTo))
        {
            fFound = TRUE;
        }
        else if (pFontCMapTab[uMiddle].wcFrom < wCode)
        {
            uFirst = uMiddle + 1;
        }
        else
        {
            uLast = uMiddle - 1;
        }
    }

    return fFound;
}

extern INT cchSymRow;

/****************************************************************************

    Display_CreateDispBuffer

****************************************************************************/
LPWSTR
Display_CreateDispBuffer(
    LPWSTR lpszSubsetChars,
    INT    nSubsetChars,
    URANGE *pCMapTab,
    INT    nNumofCMapTab,
    BOOL   bSubstitute )
{
    INT i,j;

    Display_DeleteList();

    if(nSubsetChars == 0)
    {
        return NULL;
    }

    Display_pList = (LPWSTR) malloc(sizeof(WCHAR) * (nSubsetChars + 1));
    if (! Display_pList)
    {
        return NULL;
    }

    if(lpszSubsetChars != NULL)
    {
        // Split so that bsubs does not burden normal loading process
        if (!bSubstitute)
        {
            for (i=0,j=0; i<nSubsetChars; i++)
            {
                if (Display_IsInCMapTable(lpszSubsetChars[i],pCMapTab,nNumofCMapTab))
                {
                    Display_pList[j] =  lpszSubsetChars[i];
                    j++;
                }
            }
            Display_pList[j] = (WCHAR)0;
        }
        else
        {
            for (i=0,j=0; i<nSubsetChars; i++)
            {
                // If character is space it means we have reached end of
                // line if there are no valid characters on this line 
                // proceed, else fill up all characters from current
                // point to end of column with spaces. Skip all the 
                // spaces in input buffer
                if ( (lpszSubsetChars[i] != (WCHAR)0x20 ) 
                && (Display_IsInCMapTable(lpszSubsetChars[i],pCMapTab,nNumofCMapTab)) )
                {
                    Display_pList[j] =  lpszSubsetChars[i];
                    j++;
                }
                else  // This is a hack for the grid control
                {
                    if( IsAnyListWindow() && (lpszSubsetChars[i] == (WCHAR)0x20))
                    {
                        DWORD dwLeft = ((j)%cchSymRow);
                        if (dwLeft == 0L)
                            continue;
                        dwLeft = cchSymRow - dwLeft;
                        while (dwLeft)
                        {
                            Display_pList[j] =  (WCHAR)' ';
                            j++; dwLeft--;
                        }
                        while ( (i < nSubsetChars) && (lpszSubsetChars[i] ==
                          (WCHAR)0x20) )
                        {
                            i++;
                        }
			i--;
                    }
                }
            }
            Display_pList[j] = (WCHAR)0;
        }
    }

    return Display_pList;
}

/****************************************************************************

    Display_CreateSubsetDispBuffer

****************************************************************************/
LPWSTR
Display_CreateSubsetDispBuffer(
    LPWSTR lpszSubsetChars,
    INT    nSubsetChars,
    URANGE *pCMapTab,
    INT    nNumofCMapTab,
    BOOL   bSubstitute,
    int    iFrom,
    int    iTo )
{
    INT i,j, nChars;

    Display_DeleteList();

    if(nSubsetChars == 0)
    {
        return NULL;
    }

    nChars = min(nSubsetChars, (iTo-iFrom+2));

    Display_pList = (LPWSTR) malloc(sizeof(WCHAR) * nChars);
    if (! Display_pList)
    {
        return NULL;
    }

    if(lpszSubsetChars != NULL)
    {
        i = 0;
        while ((i<nSubsetChars) && (lpszSubsetChars[i] < iFrom))
        {
            i++;
        }
        
        j = 0;
        while ((i<nSubsetChars) && (lpszSubsetChars[i] <= iTo))
        {
            if (Display_IsInCMapTable(lpszSubsetChars[i],pCMapTab,nNumofCMapTab))
            {
                Display_pList[j] =  lpszSubsetChars[i];
                j++;
            }
            else  // This is a hack for the grid control
            {
                if( bSubstitute && IsAnyListWindow() )
                {
                    Display_pList[j] = (WCHAR)' ';
                    j++;
                }
            }
            i++;
        }

        Display_pList[j] = (WCHAR)0;
    }

    return Display_pList;
}

