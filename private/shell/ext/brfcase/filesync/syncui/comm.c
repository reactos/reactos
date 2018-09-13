//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: comm.c
//
//  This files contains all common utility routines
//
// History:
//  08-06-93 ScottH     Transferred from twin code
//
//---------------------------------------------------------------------------

/////////////////////////////////////////////////////  INCLUDES

#include "brfprv.h"     // common s
#include "res.h"


// Some of these are replacements for the C runtime routines.
//  This is so we don't have to link to the CRT libs.
//

/*----------------------------------------------------------
Purpose: memset

         Swiped from the C 7.0 runtime sources.

Returns:
Cond:
*/
CHAR * PUBLIC lmemset(      // DO NO UNICODIZE
    CHAR * dst,
    CHAR val,
    UINT count)
    {
    CHAR * start = dst;

    while (count--)
        *dst++ = val;
    return(start);
    }


/*----------------------------------------------------------
Purpose: memmove

         Swiped from the C 7.0 runtime sources.

Returns:
Cond:
*/
CHAR * PUBLIC lmemmove(
    CHAR *  dst,
    CHAR * src,
    int count)
    {
    CHAR * ret = dst;

    if (dst <= src || dst >= (src + count)) {
        /*
         * Non-Overlapping Buffers
         * copy from lower addresses to higher addresses
         */
        while (count--)
            *dst++ = *src++;
        }
    else {
        /*
         * Overlapping Buffers
         * copy from higher addresses to lower addresses
         */
        dst += count - 1;
        src += count - 1;

        while (count--)
            *dst-- = *src--;
        }

    return(ret);
    }


/*----------------------------------------------------------
Purpose: My verion of atoi.  Supports hexadecimal too.
Returns: integer
Cond:    --
*/
int PUBLIC AnsiToInt(
    LPCTSTR pszString)
    {
    int n;
    BOOL bNeg = FALSE;
    LPCTSTR psz;
    LPCTSTR pszAdj;

    // Skip leading whitespace
    //
    for (psz = pszString; *psz == TEXT(' ') || *psz == TEXT('\n') || *psz == TEXT('\t'); psz = CharNext(psz))
        ;

    // Determine possible explicit signage
    //
    if (*psz == TEXT('+') || *psz == TEXT('-'))
        {
        bNeg = (*psz == TEXT('+')) ? FALSE : TRUE;
        psz = CharNext(psz);
        }

    // Or is this hexadecimal?
    //
    pszAdj = CharNext(psz);
    if (*psz == TEXT('0') && (*pszAdj == TEXT('x') || *pszAdj == TEXT('X')))
        {
        bNeg = FALSE;   // Never allow negative sign with hexadecimal numbers
        psz = CharNext(pszAdj);

        // Do the conversion
        //
        for (n = 0; ; psz = CharNext(psz))
            {
            if (*psz >= TEXT('0') && *psz <= TEXT('9'))
                n = 0x10 * n + *psz - TEXT('0');
            else
                {
                TCHAR ch = *psz;
                int n2;

                if (ch >= TEXT('a'))
                    ch -= TEXT('a') - TEXT('A');

                n2 = ch - TEXT('A') + 0xA;
                if (n2 >= 0xA && n2 <= 0xF)
                    n = 0x10 * n + n2;
                else
                    break;
                }
            }
        }
    else
        {
        for (n = 0; *psz >= TEXT('0') && *psz <= TEXT('9'); psz = CharNext(psz))
            n = 10 * n + *psz - TEXT('0');
        }

    return bNeg ? -n : n;
    }


/*----------------------------------------------------------
Purpose: General front end to invoke dialog boxes
Returns: result from EndDialog
Cond:    --
*/
INT_PTR PUBLIC DoModal(
    HWND hwndParent,            // owner of dialog
    DLGPROC lpfnDlgProc,        // dialog proc
    UINT uID,                   // dialog template ID
    LPARAM lParam)              // extra parm to pass to dialog (may be NULL)
    {
    INT_PTR nResult = -1;

    nResult = DialogBoxParam(g_hinst, MAKEINTRESOURCE(uID), hwndParent,
        lpfnDlgProc, lParam);

    return nResult;
    }


/*----------------------------------------------------------
Purpose: Sets the rectangle with the bounding extent of the given string.
Returns: Rectangle
Cond:    --
*/
void PUBLIC SetRectFromExtent(
    HDC hdc,
    LPRECT lprect,
    LPCTSTR lpcsz)
    {
    SIZE size;

    GetTextExtentPoint(hdc, lpcsz, lstrlen(lpcsz), &size);
    SetRect(lprect, 0, 0, size.cx, size.cy);
    }


/*----------------------------------------------------------
Purpose: Sees whether the entire string will fit in *prc.
         If not, compute the numbder of chars that will fit
         (including ellipses).  Returns length of string in
         *pcchDraw.

         Taken from COMMCTRL.

Returns: TRUE if the string needed ellipses
Cond:    --
*/
BOOL PRIVATE NeedsEllipses(
    HDC hdc,
    LPCTSTR pszText,
    RECT * prc,
    int * pcchDraw,
    int cxEllipses)
    {
    int cchText;
    int cxRect;
    int ichMin, ichMax, ichMid;
    SIZE siz;

    cxRect = prc->right - prc->left;

    cchText = lstrlen(pszText);

    if (cchText == 0)
        {
        *pcchDraw = cchText;
        return FALSE;
        }

    GetTextExtentPoint(hdc, pszText, cchText, &siz);

    if (siz.cx <= cxRect)
        {
        *pcchDraw = cchText;
        return FALSE;
        }

    cxRect -= cxEllipses;

    // If no room for ellipses, always show first character.
    //
    ichMax = 1;
    if (cxRect > 0)
        {
        // Binary search to find character that will fit
        ichMin = 0;
        ichMax = cchText;
        while (ichMin < ichMax)
            {
            // Be sure to round up, to make sure we make progress in
            // the loop if ichMax == ichMin + 1.
            //
            ichMid = (ichMin + ichMax + 1) / 2;

            GetTextExtentPoint(hdc, &pszText[ichMin], ichMid - ichMin, &siz);

            if (siz.cx < cxRect)
                {
                ichMin = ichMid;
                cxRect -= siz.cx;
                }
            else if (siz.cx > cxRect)
                {
                ichMax = ichMid - 1;
                }
            else
                {
                // Exact match up up to ichMid: just exit.
                //
                ichMax = ichMid;
                break;
                }
            }

        // Make sure we always show at least the first character...
        //
        if (ichMax < 1)
            ichMax = 1;
        }

    *pcchDraw = ichMax;
    return TRUE;
    }


#define CCHELLIPSES     3
#define DT_LVWRAP       (DT_CENTER | DT_WORDBREAK | DT_NOPREFIX | DT_EDITCONTROL)

/*----------------------------------------------------------
Purpose: Draws text the shell's way.

         Taken from COMMCTRL.

Returns: --

Cond:    This function requires TRANSPARENT background mode
         and a properly selected font.
*/
void PUBLIC MyDrawText(
    HDC hdc, 
    LPCTSTR pszText, 
    RECT * prc, 
    UINT flags, 
    int cyChar, 
    int cxEllipses, 
    COLORREF clrText, 

    COLORREF clrTextBk)
    {
    int cchText;
    COLORREF clrSave;
    COLORREF clrSaveBk;
    UINT uETOFlags = 0;
    RECT rc;
    TCHAR ach[MAX_PATH + CCHELLIPSES];

    // REVIEW: Performance idea:
    // We could cache the currently selected text color
    // so we don't have to set and restore it each time
    // when the color is the same.
    //
    if (!pszText)
        return;

    rc = *prc;

    // If needed, add in a little extra margin...
    //
    if (IsFlagSet(flags, MDT_EXTRAMARGIN))
        {
        rc.left  += g_cxLabelMargin * 3;
        rc.right -= g_cxLabelMargin * 3;
        }
    else
        {
        rc.left  += g_cxLabelMargin;
        rc.right -= g_cxLabelMargin;
        }

    if (IsFlagSet(flags, MDT_ELLIPSES) &&
        NeedsEllipses(hdc, pszText, &rc, &cchText, cxEllipses))
        {
        hmemcpy(ach, pszText, cchText * sizeof(TCHAR));
        lstrcpy(ach + cchText, c_szEllipses);

        pszText = ach;

        // Left-justify, in case there's no room for all of ellipses
        //
        ClearFlag(flags, (MDT_RIGHT | MDT_CENTER));
        SetFlag(flags, MDT_LEFT);

        cchText += CCHELLIPSES;
        }
    else
        {
        cchText = lstrlen(pszText);
        }

    if (IsFlagSet(flags, MDT_TRANSPARENT))
        {
        clrSave = SetTextColor(hdc, 0x000000);
        }
    else
        {
        uETOFlags |= ETO_OPAQUE;

        if (IsFlagSet(flags, MDT_SELECTED))
            {
            clrSave = SetTextColor(hdc, g_clrHighlightText);
            clrSaveBk = SetBkColor(hdc, g_clrHighlight);

            if (IsFlagSet(flags, MDT_DRAWTEXT))
                {
                FillRect(hdc, prc, g_hbrHighlight);
                }
            }
        else
            {
            if (clrText == CLR_DEFAULT && clrTextBk == CLR_DEFAULT)
                {
                clrSave = SetTextColor(hdc, g_clrWindowText);
                clrSaveBk = SetBkColor(hdc, g_clrWindow);

                if (IsFlagSet(flags, MDT_DRAWTEXT | MDT_DESELECTED))
                    {
                    FillRect(hdc, prc, g_hbrWindow);
                    }
                }
            else
                {
                HBRUSH hbr;

                if (clrText == CLR_DEFAULT)
                    clrText = g_clrWindowText;

                if (clrTextBk == CLR_DEFAULT)
                    clrTextBk = g_clrWindow;

                clrSave = SetTextColor(hdc, clrText);
                clrSaveBk = SetBkColor(hdc, clrTextBk);

                if (IsFlagSet(flags, MDT_DRAWTEXT | MDT_DESELECTED))
                    {
                    hbr = CreateSolidBrush(GetNearestColor(hdc, clrTextBk));
                    if (hbr)
                        {
                        FillRect(hdc, prc, hbr);
                        DeleteObject(hbr);
                        }
                    else
                        FillRect(hdc, prc, GetStockObject(WHITE_BRUSH));
                    }
                }
            }
        }

    // If we want the item to display as if it was depressed, we will
    // offset the text rectangle down and to the left
    if (IsFlagSet(flags, MDT_DEPRESSED))
        OffsetRect(&rc, g_cxBorder, g_cyBorder);

    if (IsFlagSet(flags, MDT_DRAWTEXT))
        {
        UINT uDTFlags = DT_LVWRAP;

        if (IsFlagClear(flags, MDT_CLIPPED))
            uDTFlags |= DT_NOCLIP;

        DrawText(hdc, pszText, cchText, &rc, uDTFlags);
        }
    else
        {
        if (IsFlagClear(flags, MDT_LEFT))
            {
            SIZE siz;

            GetTextExtentPoint(hdc, pszText, cchText, &siz);

            if (IsFlagSet(flags, MDT_CENTER))
                rc.left = (rc.left + rc.right - siz.cx) / 2;
            else
                {
                ASSERT(IsFlagSet(flags, MDT_RIGHT));
                rc.left = rc.right - siz.cx;
                }
            }

        if (IsFlagSet(flags, MDT_VCENTER))
            {
            // Center vertically
            rc.top += (rc.bottom - rc.top - cyChar) / 2;
            }

        if (IsFlagSet(flags, MDT_CLIPPED))
            uETOFlags |= ETO_CLIPPED;

        ExtTextOut(hdc, rc.left, rc.top, uETOFlags, prc, pszText, cchText, NULL);
        }

    if (flags & (MDT_SELECTED | MDT_DESELECTED | MDT_TRANSPARENT))
        {
        SetTextColor(hdc, clrSave);
        if (IsFlagClear(flags, MDT_TRANSPARENT))
            SetBkColor(hdc, clrSaveBk);
        }
    }


/*----------------------------------------------------------
Purpose: Takes a DWORD value and converts it to a string, adding
         commas on the way.

         This was taken from the shell.

Returns: Pointer to buffer

Cond:    --
*/

// BUGBUG The shell has an AddCommas.  Can it be used instead?

LPTSTR PRIVATE BrfAddCommas(
    DWORD dw,
    LPTSTR pszBuffer,
    UINT cbBuffer)
    {
    TCHAR  szTemp[30];
    TCHAR  szSep[5];
    NUMBERFMT nfmt;

    nfmt.NumDigits=0;
    nfmt.LeadingZero=0;
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, szSep, ARRAYSIZE(szSep));
    nfmt.Grouping = StrToInt(szSep);
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szSep, ARRAYSIZE(szSep));
    nfmt.lpDecimalSep = nfmt.lpThousandSep = szSep;
    nfmt.NegativeOrder= 0;

    wsprintf(szTemp, TEXT("%lu"), dw);

    GetNumberFormat(LOCALE_USER_DEFAULT, 0, szTemp, &nfmt, pszBuffer, cbBuffer);
    return pszBuffer;
    }


const short s_rgidsOrders[] = {IDS_BYTES, IDS_ORDERKB, IDS_ORDERMB, IDS_ORDERGB, IDS_ORDERTB};

// BUGBUG This is in the shell too, isn't it?

/*----------------------------------------------------------
Purpose: Converts a number into a short, string format.

         This code was taken from the shell.

            532     -> 523 bytes
            1340    -> 1.3KB
            23506   -> 23.5KB
                    -> 2.4MB
                    -> 5.2GB

Returns: pointer to buffer
Cond:    --
*/
LPTSTR PRIVATE ShortSizeFormat64(
    __int64 dw64,
    LPTSTR szBuf)
    {
    int i;
    UINT wInt, wLen, wDec;
    TCHAR szTemp[10], szOrder[20], szFormat[5];

    if (dw64 < 1000)
        {
        wsprintf(szTemp, TEXT("%d"), LODWORD(dw64));
        i = 0;
        goto AddOrder;
        }

    for (i = 1; i<ARRAYSIZE(s_rgidsOrders)-1 && dw64 >= 1000L * 1024L; dw64 >>= 10, i++);
        /* do nothing */

    wInt = LODWORD(dw64 >> 10);
    BrfAddCommas(wInt, szTemp, ARRAYSIZE(szTemp));
    wLen = lstrlen(szTemp);
    if (wLen < 3)
        {
        wDec = LODWORD(dw64 - (__int64)wInt * 1024L) * 1000 / 1024;
        // At this point, wDec should be between 0 and 1000
        // we want get the top one (or two) digits.
        wDec /= 10;
        if (wLen == 2)
            wDec /= 10;

        // Note that we need to set the format before getting the
        // intl char.
        lstrcpy(szFormat, TEXT("%02d"));

        szFormat[2] = TEXT('0') + 3 - wLen;
        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL,
                szTemp+wLen, ARRAYSIZE(szTemp)-wLen);
        wLen = lstrlen(szTemp);
        wLen += wsprintf(szTemp+wLen, szFormat, wDec);
        }

AddOrder:
    LoadString(g_hinst, s_rgidsOrders[i], szOrder, ARRAYSIZE(szOrder));
    wsprintf(szBuf, szOrder, (LPTSTR)szTemp);

    return szBuf;
    }



/*----------------------------------------------------------
Purpose: Converts a number into a short, string format.

         This code was taken from the shell.

            532     -> 523 bytes
            1340    -> 1.3KB
            23506   -> 23.5KB
                    -> 2.4MB
                    -> 5.2GB

Returns: pointer to buffer
Cond:    --
*/
LPTSTR PRIVATE ShortSizeFormat(DWORD dw, LPTSTR szBuf)
    {
    return(ShortSizeFormat64((__int64)dw, szBuf));
    }


#ifdef NOTUSED
/*----------------------------------------------------------
Purpose: Gets the index of the image of the file

Returns: index into the shell's cached imagelist
Cond:    --
*/
int PUBLIC GetImageIndex(
    LPCTSTR pszPath)
    {
    int iImage = 0;
    LPITEMIDLIST pidl;

    pidl = ILCreateFromPath(pszPath);
    if (pidl)
        {
        LPITEMIDLIST pidlParent = ILClone(pidl);
        if (pidlParent)
            {
            IShellFolder * psf;
            IShellFolder * psfDesktop;

            ILRemoveLastID(pidlParent);

            psfDesktop = GetDesktopShellFolder();
            if (SUCCEEDED(psfDesktop->lpVtbl->BindToObject(psfDesktop, pidlParent, NULL, &IID_IShellFolder, &psf)))
                {
                // Use the shell's cached system image list
                iImage = SHMapPIDLToSystemImageListIndex(psf, ILFindLastID(pidl), NULL);
                if (0 > iImage)
                    {
                    // Use the non-associated document image on error
                    iImage = 0;
                    }

                psf->lpVtbl->Release(psf);
                }
            ILFree(pidlParent);
            }
        ILFree(pidl);
        }

    return iImage;
    }
#endif


/*----------------------------------------------------------
Purpose: Gets the file info given a path.  If the path refers
         to a directory, then simply the path field is filled.

         If himl != NULL, then the function will add the file's
         image to the provided image list and set the image index
         field in the *ppfi.

Returns: standard hresult
Cond:    --
*/
HRESULT PUBLIC FICreate(
    LPCTSTR pszPath,
    FileInfo ** ppfi,
    UINT uFlags)
    {
    HRESULT hres = ResultFromScode(E_OUTOFMEMORY);
    int cchPath;
    SHFILEINFO sfi;
    UINT uInfoFlags = SHGFI_DISPLAYNAME | SHGFI_ATTRIBUTES;
    DWORD dwAttr;

    ASSERT(pszPath);
    ASSERT(ppfi);

    // Get shell file info
    if (IsFlagSet(uFlags, FIF_ICON))
        uInfoFlags |= SHGFI_ICON;
    if (IsFlagSet(uFlags, FIF_DONTTOUCH))
        {
        uInfoFlags |= SHGFI_USEFILEATTRIBUTES;

        // Today, FICreate is not called for folders, so this is ifdef'd out
#ifdef SUPPORT_FOLDERS
        dwAttr = IsFlagSet(uFlags, FIF_FOLDER) ? FILE_ATTRIBUTE_DIRECTORY : 0;
#else
        dwAttr = 0;
#endif
        }
    else
        dwAttr = 0;

    if (SHGetFileInfo(pszPath, dwAttr, &sfi, sizeof(sfi), uInfoFlags))
        {
        // Allocate enough for the structure, plus buffer for the fully qualified
        // path and buffer for the display name (and extra null terminator).
        cchPath = lstrlen(pszPath);

        *ppfi = GAlloc(sizeof(FileInfo) +
                      (cchPath+1) * sizeof(TCHAR) -
                      sizeof((*ppfi)->szPath) +
                      (lstrlen(sfi.szDisplayName)+1) * sizeof(TCHAR));
        if (*ppfi)
            {
            FileInfo * pfi = *ppfi;

            pfi->pszDisplayName = pfi->szPath+cchPath+1;
            lstrcpy(pfi->pszDisplayName, sfi.szDisplayName);

            if (IsFlagSet(uFlags, FIF_ICON))
                pfi->hicon = sfi.hIcon;

            pfi->dwAttributes = sfi.dwAttributes;

            // Does the path refer to a directory?
            if (FIIsFolder(pfi))
                {
                // Yes; just fill in the path field
                lstrcpy(pfi->szPath, pszPath);
                hres = NOERROR;
                }
            else
                {
                // No; assume the file exists?
                if (IsFlagClear(uFlags, FIF_DONTTOUCH))
                    {
                    // Yes; get the time, date and size of the file
                    HANDLE hfile = CreateFile(pszPath, GENERIC_READ, 
                                FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                NULL);

                    if (hfile == INVALID_HANDLE_VALUE)
                        {
                        GFree(*ppfi);
                        hres = ResultFromScode(E_HANDLE);
                        }
                    else
                        {
                        hres = NOERROR;

                        lstrcpy(pfi->szPath, pszPath);
                        pfi->dwSize = GetFileSize(hfile, NULL);
                        GetFileTime(hfile, NULL, NULL, &pfi->ftMod);
                        CloseHandle(hfile);
                        }
                    }
                else
                    {
                    // No; use what we have
                    hres = NOERROR;
                    lstrcpy(pfi->szPath, pszPath);
                    }
                }
            }
        }
    else if (!PathExists(pszPath))
        {
        // Differentiate between out of memory and file not found
        hres = E_FAIL;
        }

    return hres;
    }


/*----------------------------------------------------------
Purpose: Get some file info of the given path.
         The returned string is of the format "# bytes <date>"

         If the path is a folder, the string is empty.

Returns: FALSE if path is not found
Cond:    --
*/
BOOL PUBLIC FIGetInfoString(
    FileInfo * pfi,
    LPTSTR pszBuf,
    int cchBuf)
    {
    BOOL bRet;

    ASSERT(pfi);
    ASSERT(pszBuf);

    *pszBuf = NULL_CHAR;

    if (pfi)
        {
        // Is this a file?
        if ( !FIIsFolder(pfi) )
            {
            // Yes
            TCHAR szSize[MAXMEDLEN];
            TCHAR szDate[MAXMEDLEN];
            TCHAR szTime[MAXMEDLEN];
            LPTSTR pszMsg;
            SYSTEMTIME st;
            FILETIME ftLocal;

            // Construct the string
            FileTimeToLocalFileTime(&pfi->ftMod, &ftLocal);
            FileTimeToSystemTime(&ftLocal, &st);
            GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL, szDate, ARRAYSIZE(szDate));
            GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, szTime, ARRAYSIZE(szTime));

            if (ConstructMessage(&pszMsg, g_hinst, MAKEINTRESOURCE(IDS_DATESIZELINE),
                ShortSizeFormat(FIGetSize(pfi), szSize), szDate, szTime))
                {
                lstrcpy(pszBuf, pszMsg);
                GFree(pszMsg);
                }
            else
                *pszBuf = 0;

            bRet = TRUE;
            }
        else
            bRet = FALSE;
        }
    else
        bRet = FALSE;

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Set the path entry.  This can move the pfi.

Returns: FALSE on out of memory
Cond:    --
*/
BOOL PUBLIC FISetPath(
    FileInfo ** ppfi,
    LPCTSTR pszPathNew,
    UINT uFlags)
    {
    ASSERT(ppfi);
    ASSERT(pszPathNew);

    FIFree(*ppfi);

    return SUCCEEDED(FICreate(pszPathNew, ppfi, uFlags));
    }


/*----------------------------------------------------------
Purpose: Free our file info struct
Returns: --
Cond:    --
*/
void PUBLIC FIFree(
    FileInfo * pfi)
    {
    if (pfi)
        {
        if (pfi->hicon)
            DestroyIcon(pfi->hicon);

        GFree(pfi);     // This macro already checks for NULL pfi condition
        }
    }


/*----------------------------------------------------------
Purpose: Convert FILETIME struct to a readable string

Returns: String
Cond:    --
*/
void PUBLIC FileTimeToDateTimeString(
    LPFILETIME pft,
    LPTSTR pszBuf,
    int cchBuf)
    {
    SYSTEMTIME st;
    FILETIME ftLocal;

    FileTimeToLocalFileTime(pft, &ftLocal);
    FileTimeToSystemTime(&ftLocal, &st);

    // BUGBUG RTL: how do you know date comes before time???
    GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, pszBuf, cchBuf/2);
    pszBuf += lstrlen(pszBuf);
    *pszBuf++ = TEXT(' ');
    GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, pszBuf, cchBuf/2);
    }


/*----------------------------------------------------------
Purpose: Copies psz into *ppszBuf.  Will alloc or realloc *ppszBuf
         accordingly.

Returns: TRUE on success
Cond:    --
*/
BOOL PUBLIC GSetString(
    LPTSTR * ppszBuf,
    LPCTSTR psz)
    {
    BOOL bRet = FALSE;
    DWORD cb;

    ASSERT(ppszBuf);
    ASSERT(psz);

    cb = CbFromCch(lstrlen(psz)+CCH_NUL);

    if (*ppszBuf)
        {
        // Need to reallocate?
        if (cb > GGetSize(*ppszBuf))
            {
            // Yes
            LPTSTR pszT = GReAlloc(*ppszBuf, cb);
            if (pszT)
                {
                *ppszBuf = pszT;
                bRet = TRUE;
                }
            }
        else
            {
            // No
            bRet = TRUE;
            }
        }
    else
        {
        *ppszBuf = (LPTSTR)GAlloc(cb);
        if (*ppszBuf)
            {
            bRet = TRUE;
            }
        }

    if (bRet)
        {
        ASSERT(*ppszBuf);
        lstrcpy(*ppszBuf, psz);
        }
    return bRet;
    }


/*----------------------------------------------------------
Purpose: Concatenates psz onto *ppszBuf.  Will alloc or realloc *ppszBuf
         accordingly.

Returns: TRUE on success
Cond:    --
*/
BOOL PUBLIC GCatString(
    LPTSTR * ppszBuf,
    LPCTSTR psz)
    {
    BOOL bRet = FALSE;
    DWORD cb;

    ASSERT(ppszBuf);
    ASSERT(psz);

    cb = CbFromCch(lstrlen(psz)+CCH_NUL);

    if (*ppszBuf)
        {
        // (Don't need to count nul because it is already counted in cb)
        DWORD cbExisting = CbFromCch(lstrlen(*ppszBuf));

        // Need to reallocate?
        if ((cb+cbExisting) > GGetSize(*ppszBuf))
            {
            // Yes; realloc at least MAXBUFLEN to cut down on the amount
            // of calls in the future
            LPTSTR pszT = GReAlloc(*ppszBuf, cbExisting+max(cb, MAXBUFLEN));
            if (pszT)
                {
                *ppszBuf = pszT;
                bRet = TRUE;
                }
            }
        else
            {
            // No
            bRet = TRUE;
            }
        }
    else
        {
        *ppszBuf = (LPTSTR)GAlloc(max(cb, MAXBUFLEN));
        if (*ppszBuf)
            {
            bRet = TRUE;
            }
        }

    if (bRet)
        {
        ASSERT(*ppszBuf);
        lstrcat(*ppszBuf, psz);
        }
    return bRet;
    }


/*----------------------------------------------------------
Purpose: Waits for on object to signal.  This function "does
         the right thing" to prevent deadlocks which can occur
         because the calculation thread calls SendMessage.

Returns: value of MsgWaitForMultipleObjects
Cond:    --
*/
DWORD PUBLIC MsgWaitObjectsSendMessage(
    DWORD cObjects,
    LPHANDLE phObjects,
    DWORD dwTimeout)
    {
    DWORD dwRet;

    while (TRUE)
        {
        dwRet = MsgWaitForMultipleObjects(cObjects, phObjects, FALSE,
                                        dwTimeout, QS_SENDMESSAGE);

        // If it is not a message, return
        if ((WAIT_OBJECT_0 + cObjects) != dwRet)
            {
            return dwRet;
            }
        else
            {
            // Process all the sent messages
            MSG msg;
            PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
            }
        }
    }


/*----------------------------------------------------------
Purpose: Call this if PeekMessage is going to be called during
         an expensive operation and a new window has (or is)
         appeared.

         Details: simply calling SetCursor to change the cursor
         to an hourglass, then calling an expensive operation
         which will call PeekMessage, will result in the cursor
         changing back prematurely.  The reason is because SetCursorPos
         inserts a fake WM_MOUSEMOVE to set the cursor to the
         window class when a window appears for the first time.
         Since PeekMessage is processing this message, the cursor
         gets changed to the window class cursor.

         The trick is to remove the WM_MOUSEMOVE messages from
         the queue.

Returns: Previous cursor
Cond:    --
*/
HCURSOR PUBLIC SetCursorRemoveWigglies(
    HCURSOR hcur)
    {
    MSG msg;

    // Remove any mouse moves
    while (PeekMessage(&msg, NULL, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE))
        ;

    return SetCursor(hcur);
    }


/*----------------------------------------------------------
Purpose: Load the string (if necessary) and format the string
         properly.

Returns: A pointer to the allocated string containing the formatted
         message or
         NULL if out of memory

Cond:    --
*/
LPTSTR PUBLIC _ConstructMessageString(
    HINSTANCE hinst,
    LPCTSTR pszMsg,
    va_list *ArgList)
    {
    TCHAR szTemp[MAXBUFLEN];
    LPTSTR pszRet;
    LPTSTR pszRes;

    if (HIWORD(pszMsg))
        pszRes = (LPTSTR)pszMsg;
    else if (LOWORD(pszMsg) && LoadString(hinst, LOWORD(pszMsg), szTemp, ARRAYSIZE(szTemp)))
        pszRes = szTemp;
    else
        pszRes = NULL;

    if (pszRes)
        {
        if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                           pszRes, 0, 0, (LPTSTR)&pszRet, 0, ArgList))
            {
            pszRet = NULL;
            }
        }
    else
        {
        // Bad parameter
        pszRet = NULL;
        }

    return pszRet;      // free with LocalFree()
    }


/*----------------------------------------------------------
Purpose: Constructs a formatted string.  The returned string
         must be freed using GFree().

Returns: TRUE on success
Cond:    --
*/
BOOL PUBLIC ConstructMessage(
    LPTSTR * ppsz,
    HINSTANCE hinst,
    LPCTSTR pszMsg, ...)
    {
    BOOL bRet;
    LPTSTR pszRet;
    va_list ArgList;

    va_start(ArgList, pszMsg);

    pszRet = _ConstructMessageString(hinst, pszMsg, &ArgList);

    va_end(ArgList);

    *ppsz = NULL;

    if (pszRet)
        {
        bRet = GSetString(ppsz, pszRet);
        LocalFree(pszRet);
        }
    else
        bRet = FALSE;

    return bRet;
    }
