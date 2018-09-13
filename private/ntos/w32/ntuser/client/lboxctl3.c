/****************************************************************************\
*
*  LBOXCTL3.C -
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
*      Directory List Box Routines
*
* ??-???-???? ianja    Ported from Win 3.0 sources
* 14-Feb-1991 mikeke   Added Revalidation code
\****************************************************************************/

#define CTLMGR
#define LSTRING

#include "precomp.h"
#pragma hdrstop

#define DATESEPARATOR TEXT('-')
#define TIMESEPARATOR  TEXT(':')
#define TABCHAR        TEXT('\t')

#define MAXDIGITSINSIZE 9

void LB_CreateLBLine(LPWIN32_FIND_DATA, LPWSTR);

#define DDL_PRIVILEGES  (DDL_READONLY | DDL_HIDDEN | DDL_SYSTEM | DDL_ARCHIVE)
#define DDL_TYPE        (DDL_DRIVES | DDL_DIRECTORY | DDL_POSTMSGS)

/***************************************************************************\
* ChopText
*
* Chops the given path at 'lpchBuffer' + CCH_CHOPTEXT_EXTRA to fit in the
* field of the static control with id 'idStatic' in the dialog box 'hwndDlg'.
* If the path is too long, an ellipsis prefix is added to the beginning of the
* chopped text ("x:\...\")
*
* If the supplied path does not fit and the last directory appended to
* ellipsis (i.e. "c:\...\eee" in the case of "c:\aaa\bbb\ccc\ddd\eee")
* does not fit, then "x:\..." is returned.
*
* Pathological case:
* "c:\SW\MW\R2\LIB\SERVICES\NT" almost fits into static control, while
* "c:\...\MW\R2\LIB\SERVICES\NT" does fit - although it is more characters.
* In this case, ChopText substitutes the first 'n' characters of the path with
* a prefix containing MORE than 'n' characters!  The extra characters will
* be put in front of lpch, so there must be space reserved for them or they
* will trash the stack.  lpch contains CCH_CHOPTEXT_EXTRA chars followed by
* the path.
*
* History:
\***************************************************************************/

/*
 * In practice CCH_CHOPTEXT_EXTRA probably never has to be more than 1 or 2,
 * but in case the font is weird, set it to the number of chars in the prefix.
 * This guarantees enough space to prepend the prefix.
 */
#define CCH_CHOPTEXT_EXTRA 7

LPWSTR ChopText(
    PWND pwndDlg,
    int idStatic,
    LPWSTR lpchBuffer)
{
#define AWCHLEN(a) ((sizeof(a)/sizeof(a[0])) - 1)

    /*
     * Declaring szPrefix this way ensures CCH_CHOPTEXT_EXTRA is big enough
     */
    WCHAR szPrefix[CCH_CHOPTEXT_EXTRA + 1] = L"x:\\...\\";
    int cxField;
    RECT rc;
    SIZE size;
    PWND pwndStatic;
    PSTAT pstat;
    HDC hdc;
    HFONT hOldFont;
    int cchPath;
    PWCHAR lpch;
    PWCHAR lpchPath;

    /*
     * Get length of static field.
     */
    pwndStatic = _GetDlgItem(pwndDlg, idStatic);
    if (pwndStatic == NULL)
        return NULL;

    _GetClientRect(pwndStatic, &rc);
    cxField = rc.right - rc.left;

    /*
     * Set up DC appropriately for the static control.
     */
    hdc = NtUserGetDC(HWq(pwndStatic));

    /*
     * Only assume this is a static window if this window uses the static
     * window wndproc.
     */
    hOldFont = NULL;
    if (GETFNID(pwndStatic) == FNID_STATIC) {
        pstat = ((PSTATWND)pwndStatic)->pstat;
        if (pstat != NULL && pstat != (PSTAT)-1 && pstat->hFont)
            hOldFont = SelectObject(hdc, pstat->hFont);
    }

    /*
     * Check horizontal extent of string.
     */
    lpch = lpchPath = lpchBuffer + CCH_CHOPTEXT_EXTRA;
    cchPath = wcslen(lpchPath);
    GetTextExtentPoint(hdc, lpchPath, cchPath, &size);
    if (size.cx > cxField) {

        /*
         * String is too long to fit in the static control; chop it.
         * Set up new prefix and determine remaining space in control.
         */
        szPrefix[0] = *lpchPath;
        GetTextExtentPoint(hdc, szPrefix, AWCHLEN(szPrefix), &size);

        /*
         * If the field is too small to display all of the prefix,
         * copy only the prefix.
         */
        if (cxField < size.cx) {
            RtlCopyMemory(lpch, szPrefix, sizeof(szPrefix));
            goto DoneChop;
        } else
            cxField -= size.cx;

        /*
         * Advance a directory at a time until the remainder of the
         * string fits into the static control after the "x:\...\" prefix.
         */
        while (TRUE) {
            int cchT;
            while (*lpch && (*lpch++ != L'\\')) {
                ;
            }
            cchT = cchPath - (int)(lpch - lpchPath);
            GetTextExtentPoint(hdc, lpch, cchT, &size);
            if (*lpch == 0 || size.cx <= cxField) {

                if (*lpch == 0) {

                    /*
                     * Nothing could fit after the prefix; remove the
                     * final "\" from the prefix
                     */
                    szPrefix[AWCHLEN(szPrefix) - 1] = 0;
                }

                /*
                 * rest of string fits -- back up and stick prefix on front
                 * We are guaranteed to have at least CCH_CHOPTEXT_EXTRA chars
                 * backing up space, so we won't trash any stack. #26453
                 */
                lpch -= AWCHLEN(szPrefix);

                UserAssert(lpch >= lpchBuffer);

                RtlCopyMemory(lpch, szPrefix, sizeof(szPrefix) - sizeof(WCHAR));
                goto DoneChop;
            }
        }
    }

DoneChop:
    if (hOldFont)
        SelectObject(hdc, hOldFont);

    ReleaseDC(HWq(pwndStatic), hdc);

    return lpch;
}


/***************************************************************************\
* xxxDlgDirListHelper
*
*  NOTE:  If idStaticPath is < 0, then that parameter contains the details
*         about what should be in each line of the list box
*
* History:
\***************************************************************************/

BOOL xxxDlgDirListHelper(
    PWND pwndDlg,
    LPWSTR lpszPathSpec,
    LPBYTE lpszPathSpecClient,
    int idListBox,
    int idStaticPath,
    UINT attrib,
    BOOL fListBox)  /* Listbox or ComboBox? */
{
    PWND pwndLB;
    TL tlpwndLB;
    BOOL fDir = TRUE;
    BOOL fRoot, bRet;
    BOOL fPostIt;
    INT   cch;
    WCHAR ch;
    WCHAR szStaticPath[CCH_CHOPTEXT_EXTRA + MAX_PATH];
    PWCHAR pszCurrentDir;
    UINT wDirMsg;
    LPWSTR lpchFile;
    LPWSTR lpchDirectory;
    PLBIV plb;
    BOOL fWasVisible = FALSE;
    BOOL        fWin40Compat;
    PCBOX pcbox;

    CheckLock(pwndDlg);

    /*
     * Strip the private bit DDL_NOFILES out - KidPix passes it in my mistake!
     */
    if (attrib & ~DDL_VALID) {
        RIPERR2(ERROR_INVALID_FLAGS, RIP_WARNING, "Invalid flags, %x & ~%x != 0",
              attrib, DDL_VALID);
        return FALSE;
    }

    if (attrib & DDL_NOFILES) {
        RIPMSG0(RIP_WARNING, "DlgDirListHelper: stripping DDL_NOFILES");
        attrib &= ~DDL_NOFILES;
    }

    /*
     * Case:Works is an app that calls DlgDirList with a NULL has hwndDlg;
     * This is allowed because he uses NULL for idStaticPath and idListBox.
     * So, the validation layer has been modified to allow a NULL for hwndDlg.
     * But, we catch the bad apps with the following check.
     * Fix for Bug #11864 --SANKAR-- 08/22/91 --
     */
    if (!pwndDlg && (idStaticPath || idListBox)) {
        RIPERR0(ERROR_INVALID_PARAMETER, RIP_VERBOSE, "");
        return FALSE;
    }

    plb = NULL;

    /*
     * Do we need to add date, time, size or attribute info?
     * Windows checks the Atom but misses if the class has been sub-classed
     * as in VB.
     */
    if (pwndLB = (PWND)_GetDlgItem(pwndDlg, idListBox)) {
        WORD fnid = GETFNID(pwndLB);

        if ((fnid == FNID_LISTBOX && fListBox) ||
                (fnid == FNID_COMBOBOX && !fListBox) ||
                (fnid == FNID_COMBOLISTBOX && fListBox)) {
            if (fListBox) {
                plb = ((PLBWND)pwndLB)->pLBIV;
            } else {

                pcbox = ((PCOMBOWND)pwndLB)->pcbox;
                plb = ((PLBWND)(pcbox->spwndList))->pLBIV;
            }
        } else {
            RIPERR0(ERROR_LISTBOX_ID_NOT_FOUND, RIP_VERBOSE, "");
        }
    } else if (idListBox != 0) {

        /*
         * Yell if the app passed an invalid list box id and keep from using a
         * bogus plb.  PLB is NULLed above.
         */
        RIPERR0(ERROR_LISTBOX_ID_NOT_FOUND, RIP_VERBOSE, "");
    }

    if (idStaticPath < 0 && plb != NULL) {

        /*
         * Clear idStaticPath because its purpose is over.
         */
        idStaticPath = 0;

    }

    fPostIt = (attrib & DDL_POSTMSGS);

    if (lpszPathSpec) {
        cch = lstrlenW(lpszPathSpec);
        if (!cch) {
            if (lpszPathSpecClient != (LPBYTE)lpszPathSpec) {
                lpszPathSpecClient = achSlashStar;
            }
            lpszPathSpec = awchSlashStar;
        } else {
            /*
             * Make sure we won't overflow our buffers...
             */
            if (cch > CCHFILEMAX)
                return FALSE;

            /*
             * Convert lpszPathSpec into an upper case, OEM string.
             */
            CharUpper(lpszPathSpec);
            lpchDirectory = lpszPathSpec;

            lpchFile = szSLASHSTARDOTSTAR + 1;

            if (*lpchDirectory) {

                cch = wcslen(lpchDirectory);

                /*
                 * If the directory name has a * or ? in it, don't bother trying
                 * the (slow) SetCurrentDirectory.
                 */
                if (((INT)FindCharPosition(lpchDirectory, TEXT('*')) != cch) ||
                    ((INT)FindCharPosition(lpchDirectory, TEXT('?')) != cch) ||
                    !SetCurrentDirectory(lpchDirectory)) {

                    /*
                     * Set 'fDir' and 'fRoot' accordingly.
                     */
                    lpchFile = lpchDirectory + cch;
                    fDir = *(lpchFile - 1) == TEXT('\\');
                    fRoot = 0;
                    while (cch--) {
                        ch = *(lpchFile - 1);
                        if (ch == TEXT('*') || ch == TEXT('?'))
                            fDir = TRUE;

                        if (ch == TEXT('\\') || ch == TEXT('/') || ch == TEXT(':')) {
                            fRoot = (cch == 0 || *(lpchFile - 2) == TEXT(':') ||
                                    (ch == TEXT(':')));
                            break;
                        }
                        lpchFile--;
                    }

                    /*
                     * To remove Bug #16, the following error return is to be removed.
                     * In order to prevent the existing apps from breaking up, it is
                     * decided that the bug will not be fixed and will be mentioned
                     * in the documentation.
                     * --SANKAR-- Sep 21
                     */

                    /*
                     * If no wildcard characters, return error.
                     */
                    if (!fDir) {
                        RIPERR0(ERROR_NO_WILDCARD_CHARACTERS, RIP_VERBOSE, "");
                        return FALSE;
                    }

                    /*
                     * Special case for lpchDirectory == "\"
                     */
                    if (fRoot)
                        lpchFile++;

                    /*
                     * Do we need to change directories?
                     */
                    if (fRoot || cch >= 0) {

                        /*
                         * Replace the Filename's first char with a nul.
                         */
                        ch = *--lpchFile;
                        *lpchFile = TEXT('\0');

                        /*
                         * Change the current directory.
                         */
                        if (*lpchDirectory) {
                            bRet = SetCurrentDirectory(lpchDirectory);
                            if (!bRet) {

                                /*
                                 * Restore the filename before we return...
                                 */
                                *((LPWSTR)lpchFile)++ = ch;
                                return FALSE;
                            }
                        }

                        /*
                         * Restore the filename's first character.
                         */
                        *lpchFile++ = ch;
                    }

                    /*
                     * Undo damage caused by special case above.
                     */
                    if (fRoot) {
                        lpchFile--;
                    }
                }
            }

            /*
             * This is copying on top of the data the client passed us! Since
             * the LB_DIR or CB_DIR could be posted, and since we need to
             * pass a client side string pointer when we do that, we need
             * to copy this new data back to the client!
             */
            if (fPostIt && lpszPathSpecClient != (LPBYTE)lpszPathSpec) {
                WCSToMB(lpchFile, -1, &lpszPathSpecClient, MAXLONG, FALSE);
            }
            wcscpy(lpszPathSpec, lpchFile);
        }
    }
    /*
     * In some cases, the ChopText requires extra space ahead of the path:
     * Give it CCH_CHOPTEXT_EXTRA extra spaces. (See ChopText() above).
     */
    pszCurrentDir = szStaticPath + CCH_CHOPTEXT_EXTRA;
    GetCurrentDirectory(
            sizeof(szStaticPath)/sizeof(WCHAR) - CCH_CHOPTEXT_EXTRA,
            pszCurrentDir);

    /*
     * If we have a listbox, lock it down
     */
    if (pwndLB != NULL) {
        ThreadLockAlways(pwndLB, &tlpwndLB);
    }

    /*
     * Fill in the static path item.
     */
    if (idStaticPath) {

        /*
         * To fix a bug OemToAnsi() call is inserted; SANKAR--Sep 16th
         */
// OemToChar(szCurrentDir, szCurrentDir);
        CharLower(pszCurrentDir);
        SetDlgItemText(HWq(pwndDlg), idStaticPath, ChopText(pwndDlg, idStaticPath, szStaticPath));
    }

    /*
     * Fill in the directory List/ComboBox if it exists.
     */
    if (idListBox && pwndLB != NULL) {

        HWND hwndLB = HWq(pwndLB);

        wDirMsg = (UINT)(fListBox ? LB_RESETCONTENT : CB_RESETCONTENT);

        if (fPostIt) {
            PostMessage(hwndLB, wDirMsg, 0, 0L);
        } else {
            if (plb != NULL && (fWasVisible = IsLBoxVisible(plb))) {
                SendMessage(hwndLB, WM_SETREDRAW, FALSE, 0L);
            }
            SendMessage(hwndLB, wDirMsg, 0, 0L);
        }

        wDirMsg = (UINT)(fListBox ? LB_DIR : CB_DIR);

        if (attrib == DDL_DRIVES)
            attrib |= DDL_EXCLUSIVE;

        //
        // Hack for DDL_EXCLUSIVE to REALLY work.
        //
        fWin40Compat = TestWF(pwndLB, WFWIN40COMPAT);

        //
        // BACKWARDS COMPATIBILITY HACK
        //
        // We want DDL_EXCLUSIVE to _really_ work for new apps.  I.E., we
        // want apps to be able to specify DDL_DRIVES/DDL_VOLUMES with
        // DDL_EXCLUSIVE and privilege bits -- and have only those items
        // matching show up, w/out files.
        //
        if (attrib & DDL_EXCLUSIVE)
        {
            if (fWin40Compat)
            {
                if (attrib & (DDL_DRIVES | DDL_DIRECTORY))
                    attrib |= DDL_NOFILES;
            }
            else
            {
                if (attrib == (DDL_DRIVES | DDL_EXCLUSIVE))
                    attrib |= DDL_NOFILES;
            }
        }

        if (!(attrib & DDL_NOFILES)) {

            /*
             * Add everything except the subdirectories and disk drives.
             */
            if (fPostIt) {
                /*
                 * Post lpszPathSpecClient, the client side pointer.
                 */
#ifdef WASWIN31
                PostMessage(hwndLB, wDirMsg, attrib &
                        ~(DDL_DIRECTORY | DDL_DRIVES | DDL_POSTMSGS),
                        (LPARAM)lpszPathSpecClient);
#else
                /*
                 * On NT, keep DDL_POSTMSGS in wParam because we need to know
                 * in the wndproc whether the pointer is clientside or server
                 * side.
                 */
                PostMessage(hwndLB, wDirMsg,
                        attrib & ~(DDL_DIRECTORY | DDL_DRIVES),
                        (LPARAM)lpszPathSpecClient);
#endif

            } else {

                /*
                 * IanJa: #ifndef WIN16 (32-bit Windows), attrib gets extended
                 * to LONG wParam automatically by the compiler
                 */
                SendMessage(hwndLB, wDirMsg,
                        attrib & ~(DDL_DIRECTORY | DDL_DRIVES),
                        (LPARAM)lpszPathSpec);
            }

#ifdef WASWIN31
            /*
             * Strip out just the subdirectory and drive bits.
             */
            attrib &= (DDL_DIRECTORY | DDL_DRIVES);
#else
            //
            // B#1433
            // The old code stripped out read-only, hidden, system, and archive
            // information for subdirectories, making it impossible to have
            // a listbox w/ hidden directories!
            //

            /*
             * Strip out just the subdirectory and drive bits. ON NT, keep
             * the DDL_POSTMSG bit so we know how to thunk this message.
             */
            if (!fWin40Compat)
                attrib &= DDL_TYPE;
            else
            {
                attrib &= (DDL_TYPE | (attrib & DDL_PRIVILEGES));
                attrib |= DDL_NOFILES;
            }
//            attrib &= (DDL_DIRECTORY | DDL_DRIVES | DDL_POSTMSGS);
#endif
        }

        //
        // Add directories and volumes to the listbox.
        //
        if (attrib & DDL_TYPE) {

            /*
             * Add the subdirectories and disk drives.
             */
            lpszPathSpec = szSLASHSTARDOTSTAR + 1;

            attrib |= DDL_EXCLUSIVE;

            if (fPostIt) {
                /*
                 * Post lpszPathSpecClient, the client side pointer (see text
                 * above).
                 */
                PostMessage(hwndLB, wDirMsg, attrib, (LPARAM)lpszPathSpecClient);
            } else {
                SendMessage(hwndLB, wDirMsg, attrib, (LPARAM)lpszPathSpec);
            }
        }

        if (!fPostIt && fWasVisible) {
            SendMessage(hwndLB, WM_SETREDRAW, TRUE, 0L);
            NtUserInvalidateRect(hwndLB, NULL, TRUE);
        }
    }

    if (pwndLB != NULL) {
        ThreadUnlock(&tlpwndLB);
    }

    return TRUE;
}


/***************************************************************************\
* xxxDlgDirList
*
* History:
\***************************************************************************/

BOOL DlgDirListA(
    HWND hwndDlg,
    LPSTR lpszPathSpecClient,
    int idListBox,
    int idStaticPath,
    UINT attrib)
{
    LPWSTR lpszPathSpec;
    PWND pwndDlg;
    TL tlpwndDlg;
    BOOL fRet;

    pwndDlg = ValidateHwnd(hwndDlg);

    if (pwndDlg == NULL)
        return FALSE;

    lpszPathSpec = NULL;
    if (lpszPathSpecClient) {
        if (!MBToWCS(lpszPathSpecClient, -1, &lpszPathSpec, -1, TRUE))
            return FALSE;
    }

    /*
     * The last parameter is TRUE to indicate ListBox (not ComboBox)
     */
    ThreadLock(pwndDlg, &tlpwndDlg);
    fRet = xxxDlgDirListHelper(pwndDlg, lpszPathSpec, lpszPathSpecClient,
            idListBox, idStaticPath, attrib, TRUE);
    ThreadUnlock(&tlpwndDlg);

    if (lpszPathSpec) {
        if (fRet) {
            /*
             * Non-zero retval means some text to copy out.  Copy out up to
             * the nul terminator (buffer will be big enough).
             */
            WCSToMB(lpszPathSpec, -1, &lpszPathSpecClient, MAXLONG, FALSE);
        }
        UserLocalFree(lpszPathSpec);
    }

    return fRet;
}

BOOL DlgDirListW(
    HWND hwndDlg,
    LPWSTR lpszPathSpecClient,
    int idListBox,
    int idStaticPath,
    UINT attrib)
{
    LPWSTR lpszPathSpec;
    PWND pwndDlg;
    TL tlpwndDlg;
    BOOL fRet;

    pwndDlg = ValidateHwnd(hwndDlg);

    if (pwndDlg == NULL)
        return FALSE;

    lpszPathSpec = lpszPathSpecClient;

    /*
     * The last parameter is TRUE to indicate ListBox (not ComboBox)
     */
    ThreadLock(pwndDlg, &tlpwndDlg);
    fRet = xxxDlgDirListHelper(pwndDlg, lpszPathSpec, (LPBYTE)lpszPathSpecClient,
            idListBox, idStaticPath, attrib, TRUE);
    ThreadUnlock(&tlpwndDlg);

    return fRet;
}


/***************************************************************************\
* DlgDirSelectHelper
*
* History:
\***************************************************************************/

BOOL DlgDirSelectHelper(
    LPWSTR lpszPathSpec,
    int chCount,
    HWND hwndListBox)
{
    INT   cch;
    LPWSTR lpchFile;
    BOOL fDir;
    INT sItem;
    LPWSTR lpchT;
    WCHAR rgch[CCHFILEMAX + 2];
    int cchT;
    LARGE_UNICODE_STRING str;

    /*
     * Callers such as DlgDirSelectEx do not validate the existance
     * of hwndListBox
     */
    if (hwndListBox == NULL) {
        RIPERR0(ERROR_CONTROL_ID_NOT_FOUND, RIP_VERBOSE, "");
        return 0;
    }

    sItem = (INT)SendMessage(hwndListBox, LB_GETCURSEL, 0, 0L);
    if (sItem < 0)
        return FALSE;

    cchT = (INT)SendMessage(hwndListBox, LB_GETTEXT, sItem, (LPARAM)rgch);
    UserAssert(cchT < (sizeof(rgch)/sizeof(rgch[0])));

    lpchFile = rgch;
    fDir = (*rgch == TEXT('['));

    /*
     * Check if all details along with file name are to be returned.  Make sure
     * we can find the listbox because with drop down combo boxes, the
     * GetDlgItem will fail.
     *
     * Make sure this window has been using the listbox window proc because
     * we store some data as a window long.
     */

    /*
     * Only the file name is to be returned.  Find the end of the filename.
     */
    lpchT = lpchFile;
    while ((*lpchT) && (*lpchT != TABCHAR))
        lpchT++;
    *lpchT = TEXT('\0');

    cch = wcslen(lpchFile);

    /*
     * Selection is drive or directory.
     */
    if (fDir) {
        lpchFile++;
        cch--;
        *(lpchFile + cch - 1) = TEXT('\\');

        /*
         * Selection is drive
         */
        if (rgch[1] == TEXT('-')) {
            lpchFile++;
            cch--;
            *(lpchFile + 1) = TEXT(':');
            *(lpchFile + 2) = 0;
        }
    } else {

        /*
         * Selection is file.  If filename has no extension, append '.'
         */
        lpchT = lpchFile;
        for (; (cch > 0) && (*lpchT != TABCHAR);
                cch--, lpchT++) {
            if (*lpchT == TEXT('.'))
                goto Exit;
        }
        if (*lpchT == TABCHAR) {
            memmove(lpchT + 1, lpchT, CHARSTOBYTES(cch + 1));
            *lpchT = TEXT('.');
        } else {
            *lpchT++ = TEXT('.');
            *lpchT = 0;
        }
    }

Exit:
    RtlInitLargeUnicodeString(&str, lpchFile, (UINT)-1);
    TextCopy(&str, lpszPathSpec, (UINT)chCount);
    return fDir;
}


/***************************************************************************\
* DlgDirSelectEx
*
* History:
\***************************************************************************/

BOOL DlgDirSelectExA(
    HWND hwndDlg,
    LPSTR lpszPathSpec,
    int chCount,
    int idListBox)
{
    LPWSTR lpwsz;
    BOOL fRet;

    lpwsz = (LPWSTR)UserLocalAlloc(HEAP_ZERO_MEMORY, chCount * sizeof(WCHAR));
    if (!lpwsz) {
        return FALSE;
    }

    fRet = DlgDirSelectHelper(lpwsz, chCount, GetDlgItem(hwndDlg, idListBox));

    WCSToMB(lpwsz, -1, &lpszPathSpec, chCount, FALSE);

    UserLocalFree(lpwsz);

    return fRet;
}

BOOL DlgDirSelectExW(
    HWND hwndDlg,
    LPWSTR lpszPathSpec,
    int chCount,
    int idListBox)
{
    return DlgDirSelectHelper(lpszPathSpec, chCount, GetDlgItem(hwndDlg, idListBox));
}


/***************************************************************************\
* xxxLbDir
*
* History:
\***************************************************************************/

/*
 * Note that these FILE_ATTRIBUTE_* values map directly with
 * their DDL_* counterparts, with the exception of FILE_ATTRIBUTE_NORMAL.
 */
#define FIND_ATTR ( \
        FILE_ATTRIBUTE_NORMAL | \
        FILE_ATTRIBUTE_DIRECTORY | \
        FILE_ATTRIBUTE_HIDDEN | \
        FILE_ATTRIBUTE_SYSTEM | \
        FILE_ATTRIBUTE_ARCHIVE | \
        FILE_ATTRIBUTE_READONLY )
#define EXCLUDE_ATTR ( \
        FILE_ATTRIBUTE_DIRECTORY | \
        FILE_ATTRIBUTE_HIDDEN | \
        FILE_ATTRIBUTE_SYSTEM )

INT xxxLbDir(
    PLBIV plb,
    UINT attrib,
    LPWSTR lhszFileSpec)
{
    INT result;
    BOOL fWasVisible, bRet;
    WCHAR Buffer[CCHFILEMAX + 1];
    WCHAR Buffer2[CCHFILEMAX + 1];
    HANDLE hFind;
    WIN32_FIND_DATA ffd;
    UINT attribFile;
    DWORD mDrives;
    INT cDrive;
    UINT attribInclMask, attribExclMask;

    CheckLock(plb->spwnd);

    /*
     * Make sure the buffer is valid and copy it onto the stack. Why? Because
     * there is a chance that lhszFileSpec is pointing to an invalid string
     * because some app posted a CB_DIR or LB_DIR without the DDL_POSTMSGS
     * bit set.
     */
    try {
        wcscpy(Buffer2, lhszFileSpec);
        lhszFileSpec = Buffer2;
    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        return -1;
    }

    result = -1;

#ifndef UNICODE
    CharToOem(lhszFileSpec, lhszFileSpec);
#endif

    if (fWasVisible = IsLBoxVisible(plb)) {
        SendMessage(HWq(plb->spwnd), WM_SETREDRAW, FALSE, 0);
    }

    /*
     * First we add the files then the directories and drives.
     * If they only wanted drives then skip the file query
     * Also under Windows specifing only 0x8000 (DDL_EXCLUSIVE) adds no files).
     */


//    if ((attrib != (DDL_EXCLUSIVE | DDL_DRIVES)) && (attrib != DDL_EXCLUSIVE) &&
    if (attrib != (DDL_EXCLUSIVE | DDL_DRIVES | DDL_NOFILES)) {
        hFind = FindFirstFile(lhszFileSpec, &ffd);

        if (hFind != INVALID_HANDLE_VALUE) {

            /*
             * If this is not an exclusive search, include normal files.
             */
            attribInclMask = attrib & FIND_ATTR;
            if (!(attrib & DDL_EXCLUSIVE))
                attribInclMask |= FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_READONLY |
                        FILE_ATTRIBUTE_ARCHIVE;

            /*
             * Make a mask of the attributes to be excluded from
             * the search.
             */
            attribExclMask = ~attrib & EXCLUDE_ATTR;

// LATER BUG - scottlu
// Win3 assumes doing a LoadCursor here will return the same wait cursor that
// has already been created, whereas calling ServerLoadCursor creates a new
// one every time!
// hCursorT = NtUserSetCursor(ServerLoadCursor(NULL, IDC_WAIT));


// FindFirst/Next works different in NT then DOS.  Under DOS you passed in
// a set of attributes under NT you get back a set of attributes and have
// to test for those attributes (Dos input attributes were Hidden, System
// and Directoy) the dos find first always returned ReadOnly and archive files

// we are going to select a file in one of two cases.
// 1) if any of the attrib bits are set on the file.
// 2) if we want normal files and the file is a notmal file (the file attrib
//    bits don't contain any NOEXCLBITS

            do {
                attribFile = (UINT)ffd.dwFileAttributes;
                if (attribFile == FILE_ATTRIBUTE_COMPRESSED) {
                    attribFile = FILE_ATTRIBUTE_NORMAL;
                }
                attribFile &= ~FILE_ATTRIBUTE_COMPRESSED;

                /*
                 * Accept those files that have only the
                 * attributes that we are looking for.
                 */
                if ((attribFile & attribInclMask) != 0 &&
                        (attribFile & attribExclMask) == 0) {
                    if (attribFile & DDL_DIRECTORY) {

                        /*
                         * Don't include '.' (current directory) in list.
                         */
                        if (*((LPDWORD)&ffd.cFileName[0]) == 0x0000002E)
                            goto cfnf;

                        /*
                         * If we're not looking for dirs, ignore it
                         */
                        if (!(attrib & DDL_DIRECTORY))
                            goto cfnf;

                    } else if (attrib & DDL_NOFILES) {
                        /*
                         * Don't include files if DDL_NOFILES is set.
                         */
                        goto cfnf;
                    }

                    LB_CreateLBLine(&ffd,
                            Buffer);
                    result = xxxLBInsertItem(plb, Buffer, 0, MSGFLAG_SPECIAL_THUNK | LBI_ADD);
                }
cfnf:
                bRet = FindNextFile(hFind, &ffd);

            } while (result >= -1 && bRet);
            FindClose(hFind);

// LATER see above comment
// NtUserSetCursor(hCursorT);
        }
    }

    /*
     * If drive bit set, include drives in the list.
     */
    if (result != LB_ERRSPACE && (attrib & DDL_DRIVES)) {
        ffd.cFileName[0] = TEXT('[');
        ffd.cFileName[1] = ffd.cFileName[3] = TEXT('-');
        ffd.cFileName[4] = TEXT(']');
        ffd.cFileName[5] = 0;
        mDrives = GetLogicalDrives();
        for (cDrive = 0; mDrives; mDrives >>= 1, cDrive++) {
            if (mDrives & 1) {
                ffd.cFileName[2] = (WCHAR)(TEXT('A') + cDrive);

                /*
                 * We have to set the SPECIAL_THUNK bit because we are
                 * adding a server side string to a list box that may not
                 * be HASSTRINGS so we have to force the server-client
                 * string thunk.
                 */
                if ((result = xxxLBInsertItem(plb, CharLower(ffd.cFileName), -1,
                        MSGFLAG_SPECIAL_THUNK)) < 0) {
                    break;
                }
            }
        }
    }

    if (result == LB_ERRSPACE) {
        xxxNotifyOwner(plb, LB_ERRSPACE);
    }

    if (fWasVisible) {
        SendMessage(HWq(plb->spwnd), WM_SETREDRAW, TRUE, 0);
    }

    xxxLBShowHideScrollBars(plb);

    xxxCheckRedraw(plb, FALSE, 0);

    if (result != LB_ERRSPACE) {

        /*
         * Return index of last item in the listbox.  We can't just return
         * result because that is the index of the last item added which may
         * be in the middle somewhere if the LBS_SORT style is on.
         */
        return plb->cMac - 1;
    } else {
        return result;
    }
}

/***************************************************************************\
* xxxLbInsertFile
*
* Yet another CraigC shell hack... This responds to LB_ADDFILE messages
* sent to directory windows in the file system as a response to the
* WM_FILESYSCHANGE message.  That way, we don't reread the whole
* directory when we copy files.
*
* History:
\***************************************************************************/

INT xxxLbInsertFile(
    PLBIV plb,
    LPWSTR lpFile)
{
    WCHAR chBuffer[CCHFILEMAX + 1];
    INT result = -1;
    HANDLE hFind;
    WIN32_FIND_DATA ffd;

    CheckLock(plb->spwnd);

    hFind = FindFirstFile(lpFile, &ffd);
    if (hFind != INVALID_HANDLE_VALUE) {
        FindClose(hFind);
        LB_CreateLBLine(&ffd, chBuffer);
        result = xxxLBInsertItem(plb, chBuffer, 0, MSGFLAG_SPECIAL_THUNK | LBI_ADD);
    }

    if (result == LB_ERRSPACE) {
        xxxNotifyOwner(plb, result);
    }

    xxxCheckRedraw(plb, FALSE, 0);
    return result;
}

/***************************************************************************\
* LB_CreateLBLine
*
* This creates a character string that contains all the required
* details of a file;( Name)
*
* History:
\***************************************************************************/

void LB_CreateLBLine(
    PWIN32_FIND_DATA pffd,
    LPWSTR lpBuffer)
{
    BYTE bAttribute;
    LPWSTR lpch;

    lpch = lpBuffer;

    bAttribute = (BYTE)pffd->dwFileAttributes;
    if (bAttribute & DDL_DIRECTORY)  /* Is it a directory */
        *lpch++ = TEXT('[');

    /*
     * Copy the file name
     *
     * If we are running from wow, check if the shortname exists
     */
    if (GetClientInfo()->dwTIFlags & TIF_16BIT) {
        UNICODE_STRING Name;
        BOOLEAN fSpace = FALSE;

        RtlInitUnicodeString(&Name, pffd->cFileName);
        if (RtlIsNameLegalDOS8Dot3(&Name, NULL, &fSpace) && !fSpace) {
            /*
             * Legal 8.3 name and no spaces, so use the principal
             * file name.
             */
            wcscpy(lpch, pffd->cFileName);
        } else {
            if (pffd->cAlternateFileName[0] == 0)
                wcscpy(lpch, pffd->cFileName);
            else
                /*
                 * Use the alternate file name.
                 */
                wcscpy(lpch, pffd->cAlternateFileName);
        }
        /*
         * Make filename lower-case for 16-bit apps.  Some Corel apps
         * require this.
         */
        CharLower(lpch);

    }
    else
       wcscpy(lpch, pffd->cFileName);

    lpch = (LPWSTR)(lpch + wcslen(lpch));

    if (bAttribute & DDL_DIRECTORY)  /* Is it a directory */
        *lpch++ = TEXT(']');

    *lpch = TEXT('\0');

#ifndef UNICODE
    OemToChar(lpBuffer, lpBuffer);
#endif

    *lpch = TEXT('\0');  /* Null terminate */
}
