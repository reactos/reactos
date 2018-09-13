//+--------------------------------------------------------------------------
//
//  File:       file.cxx
//
//  Contents:   Import/export dialog helpers
//
//  History:    16-May-95   RobBear     Taken from formtool
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#include "cderr.h"

CCriticalSection    g_csFile;
TCHAR               g_achSavePath[MAX_PATH];

static HRESULT  SwitchToDirectory(LPTSTR lpwsz);

//+---------------------------------------------------------------------------
//
//  Function:   SwitchToDirectory
//
//  Synopsis:   Switches the current directory to that of the file.
//
//  Arguments:  [lptsz] -- The file whose directory we want to switch to.
//
//  Returns:    HRESULT.
//
//  History:    7-22-94   adams   Created
//
//----------------------------------------------------------------------------

static HRESULT
SwitchToDirectory(LPTSTR pstrFilePath)
{
    BOOL    fOK;
    TCHAR   path[_MAX_PATH];
    TCHAR * pch;

    _tcscpy(path, pstrFilePath);

    pch = _tcsrchr(path, _T(FILENAME_SEPARATOR));

    if (pch)
        *(pch + 1)= 0;

    fOK = SetCurrentDirectory(path);
    if (!fOK)
       RRETURN(GetLastWin32Error());

    return S_OK;
}


#ifndef NO_IME

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include <intl.hxx>
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

// Hook procedure for open file dialog.
UINT_PTR APIENTRY SaveOFNHookProc(HWND hdlg,
                              UINT uiMsg,
                              WPARAM wParam,
                              LPARAM lParam)
{
    ULONG i, iCurSel;

    switch (uiMsg)
    {
        // Populate the dropdown.
        case WM_INITDIALOG:
        {
            LPOPENFILENAME pofn = (LPOPENFILENAME)lParam;
            IEnumCodePage * pEnumCodePage = NULL;
            CODEPAGE        codepageDefault = *(CODEPAGE *)pofn->lCustData;
            
            if (MlangEnumCodePages( MIMECONTF_SAVABLE_BROWSER | MIMECONTF_VALID, 
                        &pEnumCodePage)== S_OK)
            {
                MIMECPINFO cpInfo;
                ULONG      ccpInfo;
                
                if ( (SlowMimeGetCodePageInfo(codepageDefault, &cpInfo) == S_OK) &&
                    !(cpInfo.dwFlags & MIMECONTF_SAVABLE_BROWSER))
                {
                    // If the codepage selected is not savable (eg JP_AUTO),
                    // use the family codepage.
                    codepageDefault = cpInfo.uiFamilyCodePage;
                }
                    
                // Can't have 4 billion languages.
                iCurSel = 0xffffffff;

                for (i = 0; pEnumCodePage->Next(1, &cpInfo, &ccpInfo) == S_OK; ++i)
                {
                    if (codepageDefault == cpInfo.uiCodePage)
                    {
                        iCurSel = i;
                    }

                    SendDlgItemMessage(hdlg, IDC_SAVE_CHARSET_MSHTML,
                                       CB_ADDSTRING, 0,
                                       (LPARAM)cpInfo.wszDescription);
                    SendDlgItemMessage(hdlg, IDC_SAVE_CHARSET_MSHTML,
                                       CB_SETITEMDATA, i,
                                       (LPARAM)cpInfo.uiCodePage);
                }
                if (iCurSel != 0xffffffff)
                    SendDlgItemMessage (hdlg, IDC_SAVE_CHARSET_MSHTML, CB_SETCURSEL,
                                        (WPARAM)iCurSel, (LPARAM)0);
            }
            ReleaseInterface(pEnumCodePage);
            break;
        }

        case WM_NOTIFY:
        {
            LPOFNOTIFY phdr = (LPOFNOTIFY)lParam;
            if (phdr->hdr.code == CDN_FILEOK)
            {
                iCurSel = SendDlgItemMessage (hdlg, IDC_SAVE_CHARSET_MSHTML, CB_GETCURSEL, 0, 0);
                *(CODEPAGE *)phdr->lpOFN->lCustData =
                    SendDlgItemMessage (hdlg, IDC_SAVE_CHARSET_MSHTML, CB_GETITEMDATA,
                                         (WPARAM)iCurSel, (LPARAM)0);
            }
            break;
        }
    }
    return (FALSE);
}

#endif //!NO_IME


//+---------------------------------------------------------------------------
//
//  Function:   FormsGetFileName
//
//  Synopsis:   Gets a file name using either the GetOpenFileName or
//              GetSaveFileName functions.
//
//  Arguments:  [fSaveFile]   -- TRUE means use GetSaveFileName
//                               FALSE means use GetOpenFileName
//
//              [idFilterRes] -- The string resource specifying text in the
//                                  dialog box.  It must have the
//                                  following format:
//                            Note: the string has to be _one_ contiguous string.
//                                  The example is broken up to make it fit
//                                  on-screen. The verical bar ("pipe") characters
//                                  are changed to '\0'-s on the fly.
//                                  This allows the strings to be localized
//                                  using Espresso.
//
//          IDS_FILENAMERESOURCE, "Save Dialog As|         // the title
//                                 odg|                    // default extension
//                                 Forms3 Dialog (*.odg)|  // pairs of filter strings
//                                 *.odg|
//                                 Any File (*.*)|
//                                 *.*|"
//
//              [pstrFile]    -- Buffer for file name.
//              [cchFile]     -- Size of buffer in characters.
//
//  Modifies:   [pstrFile]
//
//----------------------------------------------------------------------------
#ifdef _MAC
extern "C" {
char * __cdecl _p2cstr(unsigned char *);
}
#endif

HRESULT
FormsGetFileName(
        BOOL fSaveFile,
        HWND hwndOwner,
        int idFilterRes,
        LPTSTR pstrFile,
        int cchFile,
        LPARAM lCustData,
        DWORD *pnFilterIndex)
{
    HRESULT         hr  = S_OK;
    BOOL            fOK;
    DWORD           dwCommDlgErr;
    LPTSTR          pstr;
    OPENFILENAME    ofn;
    TCHAR           achBuffer[4096];    //  Max. size of a string resource
    TCHAR *         cp;
    TCHAR *         pstrExt;
    int             cbBuffer;
    TCHAR           achPath[MAX_PATH];

    // Initialize ofn struct
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize     = sizeof(ofn);
    ofn.hwndOwner       = hwndOwner;
    ofn.Flags           =   OFN_FILEMUSTEXIST   |
                            OFN_PATHMUSTEXIST   |
                            OFN_OVERWRITEPROMPT |
                            OFN_HIDEREADONLY    |
                            OFN_NOCHANGEDIR     |
                            OFN_EXPLORER;

    ofn.lpfnHook        = NULL;
    ofn.lpstrFile       = pstrFile;
    ofn.nMaxFile        = cchFile;
    ofn.lCustData       = lCustData;

#ifndef UNIX
#ifndef NO_IME
    // We add an extra control to the save file dialog.
    if (fSaveFile && lCustData)
    {
        ofn.Flags |= OFN_ENABLETEMPLATE | OFN_ENABLEHOOK;
        ofn.lpfnHook = SaveOFNHookProc;
        ofn.lpTemplateName = L"IDD_ADDTOSAVE_DIALOG_MSHTML";
        ofn.hInstance = GetResourceHInst();
    }
#endif
#endif // UNIX

    //
    // Find the extension and set the filter index based on what the
    // extension is.  After these loops pstrExt will either be NULL if
    // we didn't find an extension, or will point to the extension starting
    // at the '.'

    pstrExt = pstrFile;
    while (*pstrExt)
        pstrExt++;
    while ( pstrExt >= pstrFile )
    {
        if( *pstrExt == _T('.') )
            break;
        pstrExt--;
    }
    if( pstrExt < pstrFile )
        pstrExt = NULL;

    // Load the filter spec.
    // BUGBUG: Convert table to stringtable for localization

    cbBuffer = ::LoadString(GetResourceHInst(),idFilterRes,achBuffer,ARRAY_SIZE(achBuffer));
    Assert(cbBuffer > 0);
    if ( ! cbBuffer )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    ofn.lpstrTitle = achBuffer;

    for ( cp = achBuffer; *cp; cp++ )
    {
        if ( *cp == _T('|') )
        {
            *cp = _T('\0');
        }
    }

    Assert(ofn.lpstrTitle != NULL);

    // Default extension is second string.
    pstr = (LPTSTR) ofn.lpstrTitle;
    while (*pstr++)
    {
    }

    // N.B. (johnv) Here we assume that filter index one corresponds with the default
    //  extension, otherwise we would have to introduce a default filter index into
    //  the resource string.
    ofn.nFilterIndex    = ((pnFilterIndex)? *pnFilterIndex : 1);
    ofn.lpstrDefExt     = pstr;

    // Filter is third string.
    while(*pstr++)
    {
    }

    ofn.lpstrFilter = pstr;
    
    // Try to match the extension with an entry in the filter list
    // If we match, remove the extension from the incoming path string,
    //   set the default extension to the one we found, and appropriately
    //   set the filter index.

    if( pstrExt )
    {
        // N.B. (johnv) We are searching more than we need to.

        int    iIndex = 0;
        const TCHAR* pSearch = ofn.lpstrFilter;

        while( pSearch )
        {
            if( wcsstr ( pSearch, pstrExt ) )
            {
                ofn.nFilterIndex = (iIndex / 2) + 1;
                ofn.lpstrDefExt = pstrExt + 1;

#ifndef UNIX // We need this extension for UNIX
                // Remove the extension from the file name we pass in
                *pstrExt = _T('\0');
#endif /* UNIX */

                break;
            }
            pSearch += _tcslen(pSearch);
            if( pSearch[1] == 0 )
            {
                ofn.lpstrDefExt  = NULL;
                break;
            }

            pSearch++;
            iIndex++;
        }
    }
       
    // if given a directory, set the initial dir for GetOpenFileName
    achPath[0] = _T('\0');
    cp = _tcsrchr(pstrFile, _T(FILENAME_SEPARATOR));
    if(cp && *(cp+1) == _T('\0')) {
        DWORD dwAttrib = GetFileAttributes(pstrFile);
        if(dwAttrib != 0xffffffff && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
            _tcscpy(achPath, pstrFile);
           ofn.lpstrFile[0] = _T('\0');
        }
    } 

    {
        LOCK_SECTION(g_csFile);

        if (achPath[0] == _T('\0'))
            _tcscpy(achPath, g_achSavePath);
        ofn.lpstrInitialDir = *achPath ? achPath : NULL;
    }

    //
    // BUGBUG -- The following call hides memory leaks in GetOpenFileName
    //

    DbgMemoryTrackDisable(TRUE);

    // Call function
    fOK = (fSaveFile ? GetSaveFileName : GetOpenFileName)(&ofn);

    // try again without a file if the user gave us a bad file spec.
    if(!fOK) {
        dwCommDlgErr = CommDlgExtendedError();
        if(dwCommDlgErr == FNERR_INVALIDFILENAME) {
            ofn.lpstrFile[0]= _T('\0');
            fOK = (fSaveFile ? GetSaveFileName : GetOpenFileName)(&ofn);
        }
    }

    DbgMemoryTrackDisable(FALSE);

    if (fOK)
    {
        LOCK_SECTION(g_csFile);

        _tcscpy(g_achSavePath, ofn.lpstrFile);
        
        TCHAR * pchShortName = _tcsrchr(g_achSavePath, _T(FILENAME_SEPARATOR));

        if (pchShortName)
        {
            *(pchShortName + 1) = 0;
        }
        else
        {
            *g_achSavePath = 0;
        }

        if (pnFilterIndex)
            *pnFilterIndex = ofn.nFilterIndex;
    }
    else
    {
#ifndef WINCE
        dwCommDlgErr = CommDlgExtendedError();
        if (dwCommDlgErr)
        {
            hr = HRESULT_FROM_WIN32(dwCommDlgErr);
        }
        else
        {
            hr = S_FALSE;
        }
#else // WINCE
		hr = E_FAIL;
#endif // WINCE
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}

