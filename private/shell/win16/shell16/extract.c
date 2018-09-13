/****************************************************************************/
/*                                                                          */
/*  EXTRACT.C - 							  */
/*                                                                          */
/*      Icon Extraction Routines                                            */
/*                                                                          */
/*  NOTE most icon functions are thunked									*/
/*                                                                          */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

#include "shprv.h"

/****************************************************************************
 * InternalExtractIcon PRIVATE WIN30
 *
 * returns a list of icons.
 *
 * in:
 *      hInst               Instance handle.
 *      lpszExeFileName     File to read from.
 *      nIconIndex          Index to first icon.
 *      nIcons              The number of icons to read.
 *
 * returns:
 *      handle to arrary of hicons
 *
 * note(s)
 *      called by old Progman
 *      we cant thunk this up to 32bits for a few reasons.
 *      it returns a 16bit HANDLE 32bit side returns a 32bit HANDLE.
 *      a LPHICON points to a WORD in 16bit land, DWORD in 32bit land
 *
 ****************************************************************************/

HGLOBAL WINAPI InternalExtractIcon(HINSTANCE hInst, LPCSTR lpszExeFileName, UINT nIconIndex, UINT nIcons)
{
    HGLOBAL hIconList;     // Handle of list of icons.
    HICON FAR *lpIcons;
    UINT nNumIcons;
    int i;

    // Caller doesn't want any icons...
    if (nIcons == 0)
	return NULL;

    // Create a list for the icons.
    hIconList = GlobalAlloc(GPTR, (DWORD)nIcons * sizeof(HICON));

    if (hIconList)
    {
        lpIcons = MAKELP(hIconList, 0);

        nNumIcons = (int)ExtractIcon(hInst, lpszExeFileName, (UINT)-1);

        if (nIconIndex == (UINT)-1)
        {
            lpIcons[0] = (HICON)nNumIcons;
        }
        else
        {
            if (nIcons > nNumIcons)
                nIcons = nNumIcons;

            for (i=0; i<(int)nIcons; i++)
                lpIcons[i] = ExtractIcon(hInst, lpszExeFileName, nIconIndex+i);
        }
    }

    return hIconList;
}
