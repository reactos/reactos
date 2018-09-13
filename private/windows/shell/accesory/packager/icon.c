/* icon.c - Handles Icon + Caption objects.
 */

#include "packager.h"
#include <shellapi.h>
// #include <shlapip.h>


static LPIC IconCreate(VOID);
void GetDisplayName(LPSTR szName, LPCSTR szPath);



/* IconClone() -
 *
 * Clones an appearance pane icon.
 */
LPIC
IconClone(
    LPIC lpic
    )
{
    LPIC lpicNew;

    if (lpicNew = IconCreate())
    {
        // Get the icon
        lstrcpy(lpicNew->szIconPath, lpic->szIconPath);
        lpicNew->iDlgIcon = lpic->iDlgIcon;
        GetCurrentIcon(lpicNew);

        // Get the icon text
        lstrcpy(lpicNew->szIconText, lpic->szIconText);
    }

    return lpicNew;
}



/* IconCreate() -
 */
static LPIC
IconCreate(
    VOID
    )
{
    HANDLE hdata = NULL;
    LPIC lpic = NULL;

    if (!(hdata = GlobalAlloc(GMEM_MOVEABLE, sizeof(IC)))
        || !(lpic = (LPIC)GlobalLock(hdata)))
        goto errRtn;

    // Store the data in the window itself
    lpic->hdata = hdata;
    lpic->hDlgIcon = NULL;
    lpic->iDlgIcon = 0;
    *lpic->szIconPath = 0;
    *lpic->szIconText = 0;

    return lpic;

errRtn:
    ErrorMessage(E_FAILED_TO_CREATE_CHILD_WINDOW);

    if (lpic)
        GlobalUnlock(hdata);

    if (hdata)
        GlobalFree(hdata);

    return NULL;
}



/* IconCreateFromFile() -
 *
 * Allows an appearance pane icon to be created automatically if
 * a file is dropped, imported, or pasted into the packager.
 */
LPIC
IconCreateFromFile(
    LPSTR lpstrFile
    )
{
    LPIC lpic;

    if (lpic = IconCreate())
    {
        // Get the icon
        lstrcpy(lpic->szIconPath, lpstrFile);
        lpic->iDlgIcon = 0;

        if (*(lpic->szIconPath))
            GetCurrentIcon(lpic);

        // Get the icon text
        GetDisplayName(lpic->szIconText, lpstrFile);
    }

    return lpic;
}



/* IconCreateFromObject() -
 *
 * Allows an appearance pane icon to be created automatically if an
 * OLE object is dropped into the appearance pane.
 */
LPIC
IconCreateFromObject(
    LPOLEOBJECT lpObject
    )
{
    DWORD otObject;
    HANDLE hdata;
    LPIC lpic = NULL;
    LPSTR lpdata;

    OleQueryType(lpObject, &otObject);

    if ((otObject == OT_LINK
        && Error(OleGetData(lpObject, gcfLink, &hdata)))
        || (otObject == OT_EMBEDDED
        && Error(OleGetData(lpObject, gcfOwnerLink, &hdata))))
        hdata = NULL;

    if (hdata && (lpdata = GlobalLock(hdata)))
    {
        if (lpic = IconCreate())
        {
            // Get the icon
            RegGetExeName(lpic->szIconPath, lpdata, CBPATHMAX);
            lpic->iDlgIcon = 0;
            GetCurrentIcon(lpic);

            // Get the icon text
            switch (otObject)
            {
            case OT_LINK:
                while (*lpdata++)
                    ;

                lstrcpy(lpic->szIconText, lpdata);
                Normalize(lpic->szIconText);
                break;

            case OT_EMBEDDED:
                RegGetClassId(lpic->szIconText, lpdata);
                break;
            }

            GlobalUnlock(hdata);
        }
    }

    return lpic;
}



/* IconDelete() - Used to clear the appearance pane of icon stuff.
 */
VOID
IconDelete(
    LPIC lpic
    )
{
    HANDLE hdata;

    if (!lpic)
        return;

    if (lpic->hDlgIcon)
        DestroyIcon(lpic->hDlgIcon);

    GlobalUnlock(hdata = lpic->hdata);
    GlobalFree(hdata);
}



/* IconDraw() - Used to draw the icon and its caption.
 */
VOID
IconDraw(
    LPIC lpic,
    HDC hdc,
    LPRECT lprc,
    BOOL fFocus,
    INT cxImage,
    INT cyImage
    )
{
    BOOL fMF;
    HFONT hfont = NULL;
    RECT rcText;
    DWORD dwLayout;

    hfont = SelectObject(hdc, ghfontTitle);
    if (!(fMF = (cxImage && cyImage)))
    {
        // Figure out how large the text region will be
        if (*(lpic->szIconText))
        {
            SetRect(&rcText, 0, 0, gcxArrange - 1, gcyArrange - 1);
            DrawText(hdc, lpic->szIconText, -1, &rcText,
                DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX | DT_SINGLELINE);
        }
        else
        {
            SetRect(&rcText, 0, 0, 0, 0);
        }

        // Figure out the image dimensions
        cxImage = (gcxIcon > rcText.right) ? gcxIcon : rcText.right;
        cyImage = gcyIcon + rcText.bottom;
    }

    // Draw the icon
    if (lpic->hDlgIcon)
    {
        // Do not mirror the Icon.
        dwLayout = GetLayout(hdc);
        if ((dwLayout != GDI_ERROR) && (dwLayout & LAYOUT_RTL)) {
            SetLayout(hdc, dwLayout | LAYOUT_BITMAPORIENTATIONPRESERVED);
        }
        DrawIcon(hdc, (lprc->left + lprc->right - gcxIcon) / 2,
            (lprc->top + lprc->bottom - cyImage) / 2, lpic->hDlgIcon);
        if ((dwLayout != GDI_ERROR) && (dwLayout & LAYOUT_RTL)) {
            SetLayout(hdc, dwLayout);
        }
    }

    // Draw the icon text
    if (*(lpic->szIconText))
    {
        if (fMF)
        {
            SetBkMode(hdc, TRANSPARENT);
            SetTextAlign(hdc, TA_CENTER);
            TextOut(hdc, cxImage / 2, gcyIcon + 1, lpic->szIconText,
                lstrlen(lpic->szIconText));
        }
        else
        {
            OffsetRect(&rcText, (lprc->left + lprc->right - cxImage) / 2,
                (lprc->top + lprc->bottom - cyImage) / 2 + gcyIcon);
            DrawText(hdc, lpic->szIconText, -1, &rcText,
                DT_CENTER | DT_WORDBREAK | DT_NOPREFIX | DT_SINGLELINE);
        }
    }

    if (hfont)
        SelectObject(hdc, hfont);

    if (fFocus && cxImage && cyImage)
    {
        RECT rcFocus;

        SetRect(&rcFocus, (lprc->left + lprc->right - cxImage) / 2, (lprc->top +
            lprc->bottom - cyImage) / 2, (lprc->left + lprc->right + cxImage) /
            2, (lprc->top + lprc->bottom + cyImage) / 2);
        DrawFocusRect(hdc, &rcFocus);
    }
}



/* IconReadFromNative() - Used to retrieve the icon object from memory.
 */
LPIC
IconReadFromNative(
    LPSTR *lplpstr
    )
{
    LPIC lpic;
    WORD w;

    if (lpic = IconCreate())
    {
        lstrcpy(lpic->szIconText, *lplpstr);
        *lplpstr += lstrlen(lpic->szIconText) + 1;
        lstrcpy(lpic->szIconPath, *lplpstr);
        *lplpstr += lstrlen(lpic->szIconPath) + 1;
        MemRead(lplpstr, (LPSTR)&w, sizeof(WORD));
        lpic->iDlgIcon = (INT)w;
        GetCurrentIcon(lpic);
    }

    return lpic;
}



/* IconWriteToNative() - Used to write the icon object to memory.
 */
DWORD
IconWriteToNative(
    LPIC lpic,
    LPSTR *lplpstr
    )
{
    DWORD cBytes;
    WORD w;

    if (lplpstr)
    {
        // Now, write out the icon text and the icon
        cBytes = lstrlen(lpic->szIconText) + 1;
        MemWrite(lplpstr, (LPSTR)lpic->szIconText, cBytes);

        cBytes = lstrlen(lpic->szIconPath) + 1;
        MemWrite(lplpstr, (LPSTR)lpic->szIconPath, cBytes);
        w = (WORD)lpic->iDlgIcon;
        MemWrite(lplpstr, (LPSTR)&w, sizeof(WORD));
    }

    return (lstrlen(lpic->szIconText) + 1 + lstrlen(lpic->szIconPath) + 1 +
        sizeof(WORD));
}



VOID
GetCurrentIcon(
    LPIC lpic
    )
{
    WORD wIcon = (WORD)lpic->iDlgIcon;

    if (lpic->hDlgIcon)
        DestroyIcon(lpic->hDlgIcon);

    if (!(lpic->hDlgIcon = ExtractAssociatedIcon(ghInst, lpic->szIconPath,
            &wIcon)))
        lpic->hDlgIcon = LoadIcon(ghInst, MAKEINTRESOURCE(ID_APPLICATION));

}

//
//  get the nice name to show to the user given a filename
//
//  BUGBUG: we realy should just call the shell!!!
//
void GetDisplayName(LPSTR szName, LPCSTR szPath)
{
    WIN32_FIND_DATA fd;
    HANDLE h;
    BOOL   IsLFN;

    lstrcpy(szName, szPath);

    h = FindFirstFile(szPath, &fd);

    if (h != INVALID_HANDLE_VALUE)
    {
        FindClose(h);
        lstrcpy(szName, fd.cFileName);

        IsLFN = !(fd.cAlternateFileName[0] == 0 ||
            lstrcmp(fd.cFileName, fd.cAlternateFileName) == 0);

        if (!IsLFN)
        {
            AnsiLower(szName);
            AnsiUpperBuff(szName, 1);
        }
    }
    else
    {
        Normalize(szName);          // strip path part
    }
}
