/* filedlgs.c - Handles the Windows 3.1 common dialogs.
 *
 * Created by Microsoft Corporation.
 */

#include "packager.h"
#include <commdlg.h>


static CHAR szCustFilterSpec[CBFILTERMAX];
static CHAR szFilterSpec[CBFILTERMAX];
static CHAR szLinkCaption[CBMESSAGEMAX];
static CHAR szImportFile[CBMESSAGEMAX];
static CHAR szExportFile[CBMESSAGEMAX];
static OPENFILENAME gofn;


static VOID AddExtension(LPOPENFILENAME lpOFN);



/* OfnInit() - Initializes the standard file dialog gofn structure.
 */
VOID
OfnInit(
    VOID
    )
{
    LPSTR lpstr;

    gofn.lStructSize         = sizeof(OPENFILENAME);
    gofn.hInstance           = ghInst;
    gofn.nMaxCustFilter      = CBFILTERMAX;
    gofn.nMaxFile            = CBPATHMAX;
    gofn.lCustData           = 0;
    gofn.lpfnHook            = NULL;
    gofn.lpTemplateName      = NULL;
    gofn.lpstrFileTitle      = NULL;

    LoadString(ghInst, IDS_IMPORTFILE, szImportFile, CBMESSAGEMAX);
    LoadString(ghInst, IDS_EXPORTFILE, szExportFile, CBMESSAGEMAX);
    LoadString(ghInst, IDS_CHANGELINK, szLinkCaption, CBMESSAGEMAX);
    LoadString(ghInst, IDS_ALLFILTER,  szFilterSpec, CBMESSAGEMAX);

    // Construct the all filter string
    lpstr = (LPSTR)szFilterSpec;
    lpstr += lstrlen(lpstr) + 1;

    lstrcpy(lpstr, "*.*");
    lpstr += lstrlen(lpstr) + 1;
    *lpstr = 0;
}



/* OfnGetName() - Calls the standard file dialogs to get a file name
 */
BOOL
OfnGetName(
    HWND hwnd,
    UINT msg
    )
{
    gofn.hwndOwner           = hwnd;
    gofn.nFilterIndex        = 1;
    gofn.lpstrCustomFilter   = szCustFilterSpec;
    gofn.lpstrDefExt         = NULL;
    gofn.lpstrFile           = gszFileName;
    gofn.lpstrFilter         = szFilterSpec;
    gofn.lpstrInitialDir     = NULL;
    gofn.Flags               = OFN_HIDEREADONLY;

    Normalize(gszFileName);

    switch (msg)
    {
        case IDM_IMPORT:
            gofn.lpstrTitle = szImportFile;
            gofn.Flags |= OFN_FILEMUSTEXIST;

            return GetOpenFileName(&gofn);

        case IDM_EXPORT:
            gofn.lpstrTitle = szExportFile;
            gofn.Flags |= (OFN_PATHMUSTEXIST | OFN_NOREADONLYRETURN);

            return GetSaveFileName(&gofn);

        default:
            break;
    }

    return FALSE;
}



/* OfnGetNewLinkName() - Sets up the "Change Link..." dialog box
 */
HANDLE
OfnGetNewLinkName(
    HWND hwnd,
    HANDLE hData
    )
{
    BOOL fSuccess = FALSE;
    HANDLE hData2 = NULL;
    HANDLE hData3 = NULL;
    LPSTR lpstrData = NULL;
    LPSTR lpstrFile = NULL;
    LPSTR lpstrLink = NULL;
    LPSTR lpstrPath = NULL;
    LPSTR lpstrTemp = NULL;
    CHAR szDocFile[CBPATHMAX];
    CHAR szDocPath[CBPATHMAX];
    CHAR szServerFilter[4 * CBPATHMAX];

    // this may have to GlobalAlloc(), if a class supports
    // multiple extensions, like Pbrush then we could be in
    // trouble. I covered PBRUSH case by making array size 256

    // Get the link information
    if (!(lpstrData = GlobalLock(hData)))
        goto Error;

    // Figure out the link's path name and file name
    lpstrTemp = lpstrData;
    while (*lpstrTemp++)
        ;

    lpstrPath = lpstrFile = lpstrTemp;

    while (*(lpstrTemp = CharNext(lpstrTemp)))
    {
        if (*lpstrTemp == '\\')
            lpstrFile = lpstrTemp + 1;
    }

    // Copy the document name
    lstrcpy(szDocFile, lpstrFile);
    *(lpstrFile - 1) = 0;

    // Copy the path name
    lstrcpy(szDocPath, ((lpstrPath != lpstrFile) ? lpstrPath : ""));

    // If no directory, be sure the path points to the root
    if (lstrlen(szDocPath) == 2)
        lstrcat(szDocPath, "\\");

    if (lpstrPath != lpstrFile)                 /* Restore the backslash */
        *(lpstrFile - 1) = '\\';

    while (*lpstrFile != '.' && *lpstrFile)     /* Get the extension */
        lpstrFile++;

    // Make a filter that respects the link's class name
    gofn.hwndOwner           = hwnd;
    gofn.nFilterIndex        = RegMakeFilterSpec(lpstrData, lpstrFile, szServerFilter);
    gofn.lpstrDefExt         = NULL;
    gofn.lpstrFile           = szDocFile;
    gofn.lpstrFilter         = szServerFilter;
    gofn.lpstrInitialDir     = szDocPath;
    gofn.lpstrTitle          = szLinkCaption;
    gofn.lpstrCustomFilter   = szCustFilterSpec;
    gofn.Flags               = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;

    // If we get a file...
    if (GetOpenFileName(&gofn))
    {
        if (!(hData2 = GlobalAlloc(GMEM_ZEROINIT, CBPATHMAX *
            2)) || !(lpstrLink = lpstrTemp = GlobalLock(hData2)))
            goto Error;

        // ...add on the correct extension
        AddExtension(&gofn);

        // ... copy the server name
        while (*lpstrTemp++ = *lpstrData++)
            ;

        // ... copy the document name
        lstrcpy(lpstrTemp, szDocFile);
        lpstrTemp += lstrlen(lpstrTemp) + 1;
        lpstrData += lstrlen(lpstrData) + 1;

        // ... copy the item name
        while (*lpstrTemp++ = *lpstrData++)
            ;

        *lpstrTemp = 0;

        // ... and compress the memory block to minimal size
        GlobalUnlock(hData2);
        hData3 = GlobalReAlloc(hData2, (DWORD)(lpstrTemp - lpstrLink + 1), 0);

        if (!hData3)
            hData3 = hData2;

        fSuccess = TRUE;
    }

Error:
    if (!fSuccess)
    {
        if (lpstrLink)
            GlobalUnlock(hData2);

        if (hData2)
            GlobalFree(hData2);

        hData3 = NULL;
    }

    if (lpstrData)
        GlobalUnlock(hData);

    return hData3;
}



/* Normalize() - Removes the path specification from the file name.
 *
 * Note:  It isn't possible to get "<drive>:<filename>" as input because
 *        the path received will always be fully qualified.
 */
VOID
Normalize(
    LPSTR lpstrFile
    )
{
    LPSTR lpstrBackslash = NULL;
    LPSTR lpstrTemp = lpstrFile;
    BOOL fInQuote = FALSE;
    BOOL fQState = FALSE;

    while (*lpstrTemp)
    {
        if (*lpstrTemp == CHAR_QUOTE)
            fInQuote = !fInQuote;

        if (*lpstrTemp == '\\') {
            fQState = fInQuote;
            lpstrBackslash = lpstrTemp;
        }

        if (gbDBCS)
        {
            lpstrTemp = CharNext(lpstrTemp);
        }
        else
        {
            lpstrTemp++;
        }
    }

    if (lpstrBackslash) {
        if (fQState)
            *lpstrFile++ = CHAR_QUOTE;

        MoveMemory(lpstrFile, lpstrBackslash + 1,
            lstrlen(lpstrBackslash) * sizeof(lpstrBackslash[0]) );
    }
}



/* AddExtension() - Adds the extension corresponding to the filter dropdown.
 */
static VOID
AddExtension(
    LPOPENFILENAME lpOFN
    )
{
    LPSTR lpstrFilter = (LPSTR)lpOFN->lpstrFilter;

    // If the user didn't specify an extension, use the default
    if (lpOFN->nFileExtension == (UINT)lstrlen(lpOFN->lpstrFile)
        && lpOFN->nFilterIndex)
    {
        // Skip to the appropriate filter
        while (*lpstrFilter && --lpOFN->nFilterIndex)
        {
            while (*lpstrFilter++)
                ;

            while (*lpstrFilter++)
                ;
        }

        // If we got to the filter, retrieve the extension
        if (*lpstrFilter)
        {
            while (*lpstrFilter++)
                ;

            lpstrFilter++;

            // Copy the extension
            if (lpstrFilter[1] != '*')
                lstrcat(lpOFN->lpstrFile, lpstrFilter);
        }
    }
}
