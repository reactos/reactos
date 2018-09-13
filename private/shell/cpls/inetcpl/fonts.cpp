//
// FONTS.C  - Selecting character set default fonts dialog
//
//      Copyright(c) Microsoft Corp., 1996 All rights reserved.
//
// History:
// 7/11/96  t-gpease    trashed old International subdialog to create the new
//                      improved codepage compatiable Fonts dialog.

// WAS

//
//  INTL.C - international dialog proc for inetcpl applet.
// 
//      Copyright(c) Microsoft Corp., 1996  All rights reserved.
//
//  HISTORY:
//  2/2/96  yutakan     created.
//  2/6/96  yutakan     ported most of functions from IE2.0i.
//  8/20/98 weiwu       add script base font dialog proc (UNICODE version only)

#include "inetcplp.h"

#include <mlang.h>
#include <mluisupp.h>

#ifdef UNIX
#include <mainwin.h>
#endif /*UNIX */

// Used for window property to remember the font created
static const TCHAR c_szPropDlgFont[] = TEXT("DefaultDlgFont");

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

#ifdef UNICODE
PMIMECPINFO g_pCPInfo   = NULL;
#else
PMIMECPINFO g_pCPInfoW = NULL;

typedef struct tagMIMECPINFOA
{
    DWORD   dwFlags;
    UINT    uiCodePage;
    UINT    uiFamilyCodePage;
    CHAR    wszDescription[MAX_MIMECP_NAME];        // NOTE: 
    CHAR    wszWebCharset[MAX_MIMECSET_NAME];       // To make it simple, it has wsz 
    CHAR    wszHeaderCharset[MAX_MIMECSET_NAME];    // prefix even though it's CHAR. So,
    CHAR    wszBodyCharset[MAX_MIMECSET_NAME];      //  we don't need to put #ifdef UNICODE
    CHAR    wszFixedWidthFont[MAX_MIMEFACE_NAME];   // in below code anymore except 
    CHAR    wszProportionalFont[MAX_MIMEFACE_NAME]; // conversion time.
    BYTE    bGDICharset;                               
} MIMECPINFOA, *PMIMECPINFOA;

PMIMECPINFOA g_pCPInfo = NULL;
#endif

ULONG g_cCPInfo     = 0;
ULONG g_cSidInfo    = 0;
IMLangFontLink2     *g_pMLFlnk2 = NULL;

typedef HRESULT (* PCOINIT) (LPVOID);
typedef VOID (* PCOUNINIT) (VOID);
typedef VOID (* PCOMEMFREE) (LPVOID);
typedef HRESULT (* PCOCREINST) (REFCLSID, LPUNKNOWN, DWORD,     REFIID, LPVOID * );

extern HMODULE hOLE32;
extern PCOINIT pCoInitialize;
extern PCOUNINIT pCoUninitialize;
extern PCOMEMFREE pCoTaskMemFree;
extern PCOCREINST pCoCreateInstance;

BOOL _StartOLE32();

#define IsVerticalFont(p)    (*(p) == '@')

typedef struct {
    TCHAR   szPropFont[MAX_MIMEFACE_NAME];
    TCHAR   szFixedFont[MAX_MIMEFACE_NAME];
    TCHAR   szFriendlyName[MAX_MIMECP_NAME];
    TCHAR   szMIMEFont[MAX_MIMECP_NAME];
    DWORD   dwFontSize;
}   CODEPAGEDATA;

typedef struct {
    HWND    hDlg;
    HWND    hwndPropCB;
    HWND    hwndFixedCB;
    HWND    hwndSizeCB;
    HWND    hwndMIMECB;
    HWND    hwndNamesLB;

    DWORD   dwDefaultCodePage;

    BOOL    bChanged;

    CODEPAGEDATA    *page;

    LPCTSTR lpszKeyPath;

}   FONTSDATA, *LPFONTSDATA;


typedef struct {
    HWND        hDlg;
    HWND        hwndPropLB;
    HWND        hwndFixedLB;
    HWND        hwndNamesCB;

    SCRIPT_ID   sidDefault;

    BOOL        bChanged;

    PSCRIPTINFO pSidInfo;

    LPCTSTR     lpszKeyPath;

}   FONTSCRIPTDATA, *LPFONTSCRIPTDATA;

//
// Initialize script table with resource string
//
BOOL InitScriptTable(LPFONTSCRIPTDATA pFnt)
{

    HRESULT hr;
    BOOL    bRet = FALSE;
    IMultiLanguage2 *   pML2;

    ASSERT(IS_VALID_CODE_PTR(pCoInitialize, PCOINIT));
    ASSERT(IS_VALID_CODE_PTR(pCoUninitialize, PCOUNINIT));
    ASSERT(IS_VALID_CODE_PTR(pCoTaskMemFree, PCOMEMFREE));
    ASSERT(IS_VALID_CODE_PTR(pCoCreateInstance, PCOCREINST));

    hr = pCoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, IID_IMultiLanguage2, (LPVOID *) &pML2);

    if (SUCCEEDED(hr))
    {
        hr = pML2->QueryInterface(IID_IMLangFontLink2, (LPVOID *) &g_pMLFlnk2);

        if (SUCCEEDED(hr))
        {
            IEnumScript *pEnumScript;

            if (SUCCEEDED(pML2->EnumScripts(SCRIPTCONTF_SCRIPT_USER, MLGetUILanguage(), &pEnumScript)))
            {
                UINT cNum = 0;

                pML2->GetNumberOfScripts(&cNum);

                pFnt->pSidInfo = (PSCRIPTINFO)LocalAlloc(LPTR, sizeof(SCRIPTINFO) * cNum);
                if (NULL != pFnt->pSidInfo)
                {
                    hr = pEnumScript->Next(cNum, pFnt->pSidInfo, &g_cSidInfo);
                    if (SUCCEEDED(hr))
                    {
                        bRet = TRUE;
                    }
                    else
                    {
                        LocalFree(pFnt->pSidInfo);
                        pFnt->pSidInfo = NULL;
                    }
                }
                pEnumScript->Release();
            }
        }

        if (pML2)
            pML2->Release();
    }

    return bRet;
}

//
// DrawSampleString()
//
// Draw the sample string with current font
//

void DrawSampleString(LPFONTSDATA pFnt, int idSample, LPCTSTR lpFace, BYTE bCharset, SCRIPT_ID ScriptId)
{
    HDC hDC;
    HFONT hFont, hTemp;
    LOGFONT lf = {0};
    DWORD rgbText, rgbBack;
    RECT rc;
    SIZE TextExtent;
    TEXTMETRIC tm;
    int len, x, y;
    TCHAR szFontSample[1024];

    if (!lpFace)
        return;

    MLLoadString(IDS_FONT_SAMPLE_DEFAULT+ScriptId, szFontSample, ARRAYSIZE(szFontSample));

    GetWindowRect(GetDlgItem(pFnt->hDlg, idSample), &rc);
    // Use MapWindowPoints() as it works for mirrored windows as well.
    MapWindowRect(NULL, pFnt->hDlg, &rc);  
    // ScreenToClient(pFnt->hDlg, (LPPOINT)&rc.left);
    // ScreenToClient(pFnt->hDlg, (LPPOINT)&rc.right);

    hDC = GetDC(pFnt->hDlg);

    rgbBack = SetBkColor(hDC, GetSysColor(COLOR_3DFACE));
    rgbText = GetSysColor(COLOR_WINDOWTEXT);
    rgbText = SetTextColor(hDC, rgbText);

    hFont = GetWindowFont(pFnt->hDlg);
    GetObject(hFont, sizeof(LOGFONT), &lf);

    lf.lfCharSet = bCharset;
    lf.lfHeight += lf.lfHeight/2;
    lf.lfWidth += lf.lfWidth/2;

    StrCpyN(lf.lfFaceName, lpFace, LF_FACESIZE);
    hFont = CreateFontIndirect(&lf);
    hTemp = (HFONT)SelectObject(hDC, hFont);

    GetTextMetrics(hDC, &tm);

    len = lstrlen(szFontSample);
    
    GetTextExtentPoint32(hDC, szFontSample, len, &TextExtent);
    TextExtent.cy = tm.tmAscent - tm.tmInternalLeading;

    DrawEdge(hDC, &rc, BDR_SUNKENOUTER, BF_RECT | BF_ADJUST);

    if ((TextExtent.cx >= (rc.right - rc.left)) || (TextExtent.cx <= 0))
        x = rc.left;
    else
        x = rc.left + ((rc.right - rc.left) - TextExtent.cx) / 2;

    y = min(rc.bottom, rc.bottom - ((rc.bottom - rc.top) - TextExtent.cy) / 2);

    if (lpFace[0])
        ExtTextOut(hDC, x, y - (tm.tmAscent), ETO_OPAQUE | ETO_CLIPPED,
               &rc, szFontSample, len, NULL );
    else
        ExtTextOut(hDC, x, y - (tm.tmAscent), ETO_OPAQUE | ETO_CLIPPED,
               &rc, TEXT(" "), 1, NULL );


    SetBkColor(hDC, rgbBack);
    SetTextColor(hDC, rgbText);

    if (hTemp)
        DeleteObject(SelectObject(hDC, hTemp));

    ReleaseDC(pFnt->hDlg, hDC);
}


//
// FillCharsetListBoxes()
//
// Fills the Web page and Plain text ListBoxes with the appropriate
// font data
//
BOOL FillScriptListBoxes(LPFONTSCRIPTDATA pFnt, SCRIPT_ID sid)
{

    
    UINT    i;
    UINT    nFonts = 0;
    int     iSidInfo = -1;
    PSCRIPTFONTINFO pSidFont = NULL;

    if (!pFnt->pSidInfo)
        return FALSE;

    // erase all the listboxes to start fresh
    SendMessage(pFnt->hwndPropLB,  LB_RESETCONTENT, 0, 0);
    SendMessage(pFnt->hwndFixedLB, LB_RESETCONTENT, 0, 0);


    for(i=0; i < g_cSidInfo; i++)
    {
        if (pFnt->pSidInfo[i].ScriptId == sid)
        {
            iSidInfo = i; 
            break;
        }
    }

    if (-1 == iSidInfo)
        return FALSE;

    if (g_pMLFlnk2)
    {

        g_pMLFlnk2->GetScriptFontInfo(sid, SCRIPTCONTF_PROPORTIONAL_FONT, &nFonts, NULL);

        if (nFonts)
        {            

            pSidFont = (PSCRIPTFONTINFO) LocalAlloc(LPTR, sizeof(SCRIPTFONTINFO)*nFonts);
            if (pSidFont)
            {
                g_pMLFlnk2->GetScriptFontInfo(sid, SCRIPTCONTF_PROPORTIONAL_FONT, &nFonts, pSidFont);
                for (i=0; i<nFonts; i++)
                {
                    if (LB_ERR == SendMessage(pFnt->hwndPropLB, LB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)((pSidFont+i)->wszFont)))
                    {
                        // add the font name to the combobox
                        SendMessage(pFnt->hwndPropLB, LB_ADDSTRING, 0, (LPARAM)((pSidFont+i)->wszFont));
                    }

                }
                // Hack PRC font problems on Win9x and NT4 (Bug #24641, #39946)
                // Win9x does not ship with GBK-supporting fixed-pitch fonts,
                // We provide user proportional fonts as plain text font candidates.
                if (sid == sidHan && GetACP() == 936 && !IsOS(OS_NT5))
                {
                    for (i=0; i<nFonts; i++)
                    {
                        // add the font name to the combobox
                        SendMessage(pFnt->hwndFixedLB, LB_ADDSTRING, 0, (LPARAM)((pSidFont+i)->wszFont));
                    }
                }

                LocalFree(pSidFont);
                pSidFont = NULL;
            }
        }

        // Get number of available fonts
        g_pMLFlnk2->GetScriptFontInfo(sid, SCRIPTCONTF_FIXED_FONT, &nFonts, NULL);
        if (nFonts)
        {
            pSidFont = (PSCRIPTFONTINFO) LocalAlloc(LPTR, sizeof(SCRIPTFONTINFO)*nFonts);
            if (pSidFont)
            {
                g_pMLFlnk2->GetScriptFontInfo(sid, SCRIPTCONTF_FIXED_FONT, &nFonts, pSidFont);

                if (!pFnt->pSidInfo[iSidInfo].wszFixedWidthFont[0])
                {
                    StrCpyN(pFnt->pSidInfo[iSidInfo].wszFixedWidthFont, pSidFont->wszFont, LF_FACESIZE);
                    pFnt->bChanged = TRUE;
                }

                // All fixedwidth and proportional fonts are web page font candidates
                for (i=0; i<nFonts; i++)
                {
                    if (LB_ERR == SendMessage(pFnt->hwndFixedLB, LB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)((pSidFont+i)->wszFont)))
                    {
                        // add the font name to the combobox
                        SendMessage(pFnt->hwndFixedLB, LB_ADDSTRING, 0, (LPARAM)((pSidFont+i)->wszFont));
                    }
                    if (LB_ERR == SendMessage(pFnt->hwndPropLB, LB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)((pSidFont+i)->wszFont)))
                    {
                        // add the font name to the combobox
                        SendMessage(pFnt->hwndPropLB, LB_ADDSTRING, 0, (LPARAM)((pSidFont+i)->wszFont));
                    }
                }

                LocalFree(pSidFont);
            }
        }
    }



    // Add fonts to combobox

#ifdef UNIX
    /* We would have called EnumFontFamiliesEx wherein we would have
     * have populated the fonts list boxes with substitute fonts if any
     *
     * So, before we populate the proportional and the fixed fonts below,
     * we must query and use substitute fonts if avbl.
     */
     {
        CHAR szSubstFont[MAX_MIMEFACE_NAME+1];
        DWORD cchSubstFont = MAX_MIMEFACE_NAME + 1;
        CHAR szFont[MAX_MIMEFACE_NAME + 1];
           
        WideCharToMultiByte(CP_ACP, 0, pFnt->pSidInfo[iSidInfo].wszProportionalFont, -1, szFont, 
               MAX_MIMEFACE_NAME + 1, NULL, NULL);
        if ((ERROR_SUCCESS == MwGetSubstituteFont(szFont, szSubstFont, &cchSubstFont)) && 
            cchSubstFont) 
        {
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szSubstFont, -1, 
               pFnt->pSidInfo[iSidInfo].wszProportionalFont, MAX_MIMEFACE_NAME + 1);
        }

        WideCharToMultiByte(CP_ACP, 0, pFnt->pSidInfo[iSidInfo].wszFixedWidthFont, -1, szFont, 
               MAX_MIMEFACE_NAME + 1, NULL, NULL);
        cchSubstFont = MAX_MIMEFACE_NAME + 1;
        if ((ERROR_SUCCESS == MwGetSubstituteFont(szFont, szSubstFont, &cchSubstFont)) && 
            cchSubstFont) 
        {
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szSubstFont, -1, 
               pFnt->pSidInfo[iSidInfo].wszFixedWidthFont, MAX_MIMEFACE_NAME + 1);
        }
    }
#endif /* UNIX */

    // select the current prop default
    if (pFnt->pSidInfo[iSidInfo].wszProportionalFont[0])
    {
        if (LB_ERR == SendMessage(pFnt->hwndPropLB, LB_SELECTSTRING, (WPARAM)-1,
            (LPARAM)pFnt->pSidInfo[iSidInfo].wszProportionalFont))
            pFnt->pSidInfo[iSidInfo].wszProportionalFont[0] = 0;
    }
    // Draw sample strings with current font
    DrawSampleString((FONTSDATA *)pFnt, IDC_FONTS_PROP_SAMPLE, pFnt->pSidInfo[iSidInfo].wszProportionalFont, DEFAULT_CHARSET, pFnt->pSidInfo[iSidInfo].ScriptId);

    // select the current fixed default
    if (pFnt->pSidInfo[iSidInfo].wszFixedWidthFont[0])
    {
        if (LB_ERR == SendMessage(pFnt->hwndFixedLB, LB_SELECTSTRING, (WPARAM)-1,
            (LPARAM)pFnt->pSidInfo[iSidInfo].wszFixedWidthFont))
            pFnt->pSidInfo[iSidInfo].wszFixedWidthFont[0] = 0;
    }
    // Draw sample strings with current font
    DrawSampleString((FONTSDATA *)pFnt, IDC_FONTS_FIXED_SAMPLE, pFnt->pSidInfo[iSidInfo].wszFixedWidthFont, DEFAULT_CHARSET, pFnt->pSidInfo[iSidInfo].ScriptId);


    // we handled it
    return TRUE;


}   // FillScriptListBoxes()

//
// FontsDlgInitEx()
//
// Initializes the script based font dialog, use same dialog box template.
//
BOOL FontsDlgInitEx(IN HWND hDlg, LPCTSTR lpszKeyPath)
{
    HKEY    hkey;
//  DWORD   dw;
    DWORD   cb;
    DWORD   i;

    TCHAR   szKey[1024];

    LPFONTSCRIPTDATA  pFnt;  // localize data

    if (!hDlg)
        return FALSE;   // nothing to initialize

    // get some space to store local data
    // NOTE: LocalAlloc already zeroes the memory
    pFnt = (LPFONTSCRIPTDATA)LocalAlloc(LPTR, sizeof(*pFnt));
    if (!pFnt)
    {
        EndDialog(hDlg, 0);
        return FALSE;
    }

    if (!InitScriptTable(pFnt))
    {
        EndDialog(hDlg, 0);
        return FALSE;
    }

    // associate the memory with the dialog window
    SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pFnt);

    // save the dialog handle
    pFnt->hDlg = hDlg;

    // get the dialog items
    pFnt->hwndPropLB  = GetDlgItem(pFnt->hDlg, IDC_FONTS_PROP_FONT_LIST);
    pFnt->hwndFixedLB = GetDlgItem(pFnt->hDlg, IDC_FONTS_FIXED_FONT_LIST);
    pFnt->hwndNamesCB = GetDlgItem(pFnt->hDlg, IDC_FONTS_CHAR_SET_COMBO);
    pFnt->lpszKeyPath = lpszKeyPath ? lpszKeyPath: REGSTR_PATH_INTERNATIONAL_SCRIPTS;

    if (!g_pMLFlnk2 || FAILED(g_pMLFlnk2->CodePageToScriptID(GetACP(), &(pFnt->sidDefault))))
        pFnt->sidDefault = sidAsciiLatin;
    
    // We shouldn't consider default script in registry since we no longer have UI to allow user to change default script    
#if 0    
    // get values from registry
    if (RegOpenKeyEx(HKEY_CURRENT_USER, pFnt->lpszKeyPath, NULL, KEY_READ, &hkey)
        == ERROR_SUCCESS)
    {
        cb = sizeof(dw);
        if (RegQueryValueEx(hkey, REGSTR_VAL_DEFAULT_SCRIPT, NULL, NULL, (LPBYTE)&dw, &cb)
          == ERROR_SUCCESS)
        {
            pFnt->sidDefault = (SCRIPT_ID)dw;
        }
        RegCloseKey(hkey);
    }
#endif

    for (i = 0; i < g_cSidInfo; i++)
    {
        wnsprintf(szKey, ARRAYSIZE(szKey), TEXT("%s\\%u"), pFnt->lpszKeyPath, pFnt->pSidInfo[i].ScriptId);
        if (RegOpenKeyEx(HKEY_CURRENT_USER, szKey, NULL, KEY_READ, &hkey) == ERROR_SUCCESS)
        {
            TCHAR szFont[MAX_MIMEFACE_NAME];

            cb = sizeof(szFont);

            if (RegQueryValueEx(hkey, REGSTR_VAL_FIXED_FONT, NULL, NULL,
                    (LPBYTE)szFont, &cb) == ERROR_SUCCESS)
            {
                StrCpyN(pFnt->pSidInfo[i].wszFixedWidthFont, szFont, ARRAYSIZE(pFnt->pSidInfo[i].wszFixedWidthFont));
            }
            
            cb = sizeof(szFont);
            if (RegQueryValueEx(hkey, REGSTR_VAL_PROP_FONT, NULL, NULL,
                    (LPBYTE)szFont, &cb) == ERROR_SUCCESS)
            {
                StrCpyN(pFnt->pSidInfo[i].wszProportionalFont, szFont, ARRAYSIZE(pFnt->pSidInfo[i].wszProportionalFont));
            }
            RegCloseKey(hkey);

        }

        // add the name to the listbox
        SendMessage(pFnt->hwndNamesCB, CB_ADDSTRING, 0, 
            (LPARAM)pFnt->pSidInfo[i].wszDescription);

        // check to see if it is the default code page
        if (pFnt->sidDefault == pFnt->pSidInfo[i].ScriptId)
        {
            SendMessage(pFnt->hwndNamesCB, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)pFnt->pSidInfo[i].wszDescription);
        }

    }

    pFnt->bChanged = FALSE;

    FillScriptListBoxes(pFnt, pFnt->sidDefault);
    

    if( g_restrict.fFonts )
    {
        EnableWindow( GetDlgItem( hDlg, IDC_FONTS_PROP_FONT_LIST ), FALSE);
        EnableWindow( GetDlgItem( hDlg, IDC_FONTS_FIXED_FONT_LIST ), FALSE);
        EnableWindow( GetDlgItem( hDlg, IDC_FONTS_CHAR_SET_COMBO ), FALSE);
#ifdef UNIX
        EnableWindow( GetDlgItem( hDlg, IDC_FONTS_UPDATE_BUTTON ), FALSE);
#endif
    }

    // everything ok
    return TRUE;

}   // FontsDlgInit()

//
// SaveFontsDataEx()
//
// Save the new fonts settings into regestry
//
void SaveFontsDataEx(LPFONTSCRIPTDATA pFnt)
{
    HKEY    hkeyScript;
    TCHAR   szScript[MAX_SCRIPT_NAME];

    HKEY    hkey;
    DWORD   dw;

    // get values from registry
    if (RegCreateKeyEx(HKEY_CURRENT_USER, pFnt->lpszKeyPath, NULL, NULL, NULL, KEY_WRITE, NULL, &hkey, &dw)
        == ERROR_SUCCESS)
    {
        UINT i;
 
        RegSetValueEx(hkey, REGSTR_VAL_DEFAULT_SCRIPT, NULL, REG_BINARY, (LPBYTE)&pFnt->sidDefault, sizeof(pFnt->sidDefault));
        
        for(i = 0; i < g_cSidInfo; i++)
        {
            wnsprintf(szScript, ARRAYSIZE(szScript), TEXT("%u"), pFnt->pSidInfo[i].ScriptId);
            if (RegCreateKeyEx(hkey, szScript, NULL, NULL, NULL, KEY_WRITE, NULL, &hkeyScript, &dw) == ERROR_SUCCESS)
            {
                // Currently, no need for script name, save registry space
#if 0
                RegSetValueEx(hkeyScript, REGSTR_VAL_FONT_SCRIPT_NAME, NULL, REG_SZ,
                            (LPBYTE)&pFnt->pSidInfo[i].wszDescription, 
                            (lstrlen(pFnt->pSidInfo[i].wszDescription)+1)*sizeof(TCHAR));
#endif
                    
                RegSetValueEx(hkeyScript, REGSTR_VAL_SCRIPT_FIXED_FONT, NULL, REG_SZ,
                            (LPBYTE)pFnt->pSidInfo[i].wszFixedWidthFont, 
                            (lstrlen(pFnt->pSidInfo[i].wszFixedWidthFont)+1)*sizeof(TCHAR));
                    
                RegSetValueEx(hkeyScript, REGSTR_VAL_SCRIPT_PROP_FONT, NULL, REG_SZ,
                            (LPBYTE)pFnt->pSidInfo[i].wszProportionalFont, 
                            (lstrlen(pFnt->pSidInfo[i].wszProportionalFont)+1)*sizeof(TCHAR));

                RegCloseKey(hkeyScript);
                    
            }   // if RegCreateKeyEx


        }   // for

        RegCloseKey(hkey);
  
    }   // if RegCreateKeyEx

}   // SaveFontsDataEx()

//
// FontsOnCommandEx()
//
// Handles WM_COMMAND message for the script based Fonts subdialog
//
BOOL FontsOnCommandEx(LPFONTSCRIPTDATA pFnt, UINT id, UINT nCmd)
{
    switch(id)
    {
        case IDOK:
            if (pFnt->bChanged)
            {
                SaveFontsDataEx(pFnt);
                
                // tell MSHTML to pick up changes and update
                UpdateAllWindows();
            }
            return TRUE;    // exit dialog

        case IDCANCEL:
            return TRUE;    // exit dialog

        case IDC_FONTS_PROP_FONT_LIST:
        case IDC_FONTS_FIXED_FONT_LIST:
            if (nCmd==LBN_SELCHANGE)
            {
                UINT i;
                TCHAR   szScript[MAX_SCRIPT_NAME];

                pFnt->bChanged = TRUE;  // we need to save
                
                // find the currently selected item in the list box
                GetDlgItemText(pFnt->hDlg, IDC_FONTS_CHAR_SET_COMBO, szScript, ARRAYSIZE(szScript));
                
                // find the code page from the text
                for(i=0; i < g_cSidInfo; i++)
                {
                    INT_PTR j;
                    if (!StrCmpI(szScript, pFnt->pSidInfo[i].wszDescription))
                    {             
                        // grab the new values
                        j = SendMessage(pFnt->hwndPropLB, LB_GETCURSEL, 0, 0);
                        SendMessage(pFnt->hwndPropLB, LB_GETTEXT, j, (LPARAM)(pFnt->pSidInfo[i].wszProportionalFont));
                        j = SendMessage(pFnt->hwndFixedLB, LB_GETCURSEL, 0, 0);
                        SendMessage(pFnt->hwndFixedLB, LB_GETTEXT, j, (LPARAM)(pFnt->pSidInfo[i].wszFixedWidthFont));
                        break;
                    }
                }

                // Redraw sample strings                
                DrawSampleString((LPFONTSDATA)pFnt, IDC_FONTS_PROP_SAMPLE, pFnt->pSidInfo[i].wszProportionalFont, DEFAULT_CHARSET, pFnt->pSidInfo[i].ScriptId);
                DrawSampleString((LPFONTSDATA)pFnt, IDC_FONTS_FIXED_SAMPLE, pFnt->pSidInfo[i].wszFixedWidthFont, DEFAULT_CHARSET, pFnt->pSidInfo[i].ScriptId);

                // if we don't find it... we are going to keep the default

                ASSERT(i < g_cSidInfo);  // something went wrong

            }
            break;

        case IDC_FONTS_CHAR_SET_COMBO:
            if (nCmd==CBN_SELCHANGE)
            {
                UINT i;
                TCHAR   szScript[MAX_SCRIPT_NAME];

                GetDlgItemText(pFnt->hDlg, IDC_FONTS_CHAR_SET_COMBO, szScript, ARRAYSIZE(szScript));
                
                // find the code page from the text
                for(i=0; i < g_cSidInfo; i++)
                {
                    if (!StrCmpI(szScript, pFnt->pSidInfo[i].wszDescription))
                    {
                        FillScriptListBoxes(pFnt, pFnt->pSidInfo[i].ScriptId);
                        break;
                    }
                }
            }
            break;
#ifdef UNIX
        case IDC_FONTS_UPDATE_BUTTON: 
    {
        HCURSOR hOldCursor = NULL;
        HCURSOR hNewCursor = NULL;

        hNewCursor = LoadCursor(NULL, IDC_WAIT);
        if (hNewCursor) 
            hOldCursor = SetCursor(hNewCursor);

        DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_FONTUPD_PROG), pFnt->hDlg,FontUpdDlgProc, NULL);

        if(hOldCursor)
            SetCursor(hOldCursor);
    }
        break; 
#endif
    }
    
    // don't exit dialog
    return FALSE;
}

//
// FontsDlgProcEx()
//
// Message handler for the script based "Fonts" subdialog.
//
INT_PTR CALLBACK FontsDlgProcEx(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPFONTSCRIPTDATA pFnt = (LPFONTSCRIPTDATA) GetWindowLongPtr(hDlg, DWLP_USER);
    PAINTSTRUCT ps;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            return FontsDlgInitEx(hDlg, (LPTSTR)lParam);
            break;

        case WM_DESTROY:
            // Free memory
            if (pFnt)
            {
                if (pFnt->pSidInfo)
                    LocalFree(pFnt->pSidInfo);
                LocalFree(pFnt);
            }

            break;
            
        case WM_PAINT:

            if (BeginPaint(hDlg, &ps))
            {
                UINT i;
                SCRIPT_ID sid = 0;
                TCHAR szScript[MAX_SCRIPT_NAME];

                GetDlgItemText(hDlg, IDC_FONTS_CHAR_SET_COMBO, szScript, ARRAYSIZE(szScript));
                
                // find the script id from the text
                for(i = 0; i < g_cSidInfo; i++)
                {
                    if (!StrCmpI(szScript, pFnt->pSidInfo[i].wszDescription))
                    {
                        sid = pFnt->pSidInfo[i].ScriptId;
                        break;
                    }
                }


                if (i < g_cSidInfo)
                {
                    // show sample strings with current font
                    DrawSampleString((LPFONTSDATA)pFnt, IDC_FONTS_PROP_SAMPLE, pFnt->pSidInfo[i].wszProportionalFont, DEFAULT_CHARSET, pFnt->pSidInfo[i].ScriptId);
                    DrawSampleString((LPFONTSDATA)pFnt, IDC_FONTS_FIXED_SAMPLE, pFnt->pSidInfo[i].wszFixedWidthFont, DEFAULT_CHARSET, pFnt->pSidInfo[i].ScriptId);
                }
                EndPaint(hDlg, &ps);
            }
            break;

        case WM_COMMAND:
            if (FontsOnCommandEx(pFnt, LOWORD(wParam), HIWORD(wParam)))
                EndDialog(hDlg, LOWORD(wParam) == IDOK? 1: 0);
            break;

        case WM_HELP:           // F1
            ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
                HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_CONTEXTMENU:    // right mouse click
            ResWinHelp( (HWND) wParam, IDS_HELPFILE,
                HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);        
            break;

#ifdef UNIX
        case WM_DRAWITEM:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
            case IDC_FONTS_UPDATE_BUTTON:
            DrawXFontButton(hDlg, (LPDRAWITEMSTRUCT)lParam);
            return TRUE;
        }
        return FALSE;
#endif

        default:
            return FALSE;
    }
    return TRUE;
}


//
// Back out old font dialog for OE4
//



//
// InitMimeCsetTable()
//
// Initialize MimeCharsetTable[]'s string field with resource string
//
BOOL InitMimeCsetTable(BOOL bIsOE5)
{
    IMultiLanguage *pML=NULL;
    IMultiLanguage2 *pML2=NULL;
    HRESULT hr;

    if(!hOLE32)
    {
        if(!_StartOLE32())
        {
            ASSERT(FALSE);
            return FALSE;
        }
    }
    hr = pCoInitialize(NULL);
    if (FAILED(hr))
        return FALSE;

    if (bIsOE5)        
        hr = pCoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, IID_IMultiLanguage2, (LPVOID *) &pML2);
    else
        hr = pCoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, IID_IMultiLanguage, (LPVOID *) &pML);
    
    if (SUCCEEDED(hr))
    {
        IEnumCodePage *pEnumCP;

        if (bIsOE5)
            // ignore MUI in old font dialog box
            hr = pML2->EnumCodePages(MIMECONTF_VALID, GetSystemDefaultLangID(), &pEnumCP);
        else
            hr = pML->EnumCodePages(MIMECONTF_VALID, &pEnumCP);
        

        if (SUCCEEDED(hr))
        {
            UINT cNum = 0;
            if (bIsOE5)
                pML2->GetNumberOfCodePageInfo(&cNum);
            else
                pML->GetNumberOfCodePageInfo(&cNum);

#ifdef UNICODE
            g_pCPInfo = (PMIMECPINFO)LocalAlloc(LPTR, sizeof(MIMECPINFO) * cNum);
            if (NULL != g_pCPInfo)
            {
                hr = pEnumCP->Next(cNum, g_pCPInfo, &g_cCPInfo);
                if (SUCCEEDED(hr))
                {
                    g_pCPInfo = (PMIMECPINFO)LocalReAlloc(g_pCPInfo, sizeof(MIMECPINFO) * g_cCPInfo, LMEM_MOVEABLE);
                }
                else
                {
                    LocalFree(g_pCPInfo);
                    g_pCPInfo = NULL;
                }
#else
            g_pCPInfoW = (PMIMECPINFO)LocalAlloc(LPTR, sizeof(MIMECPINFO) * cNum);
            if (NULL != g_pCPInfoW)
            {
                hr = pEnumCP->Next(cNum, g_pCPInfoW, &g_cCPInfo);
                if (SUCCEEDED(hr))
                {
                    g_pCPInfo = (PMIMECPINFOA)LocalAlloc(LPTR, sizeof(MIMECPINFOA) * g_cCPInfo);
                    if (NULL != g_pCPInfo)
                    {
                        UINT i;

                        for (i = 0; i < g_cCPInfo; i++)
                        {
                            g_pCPInfo[i].dwFlags = g_pCPInfoW[i].dwFlags;
                            g_pCPInfo[i].uiCodePage = g_pCPInfoW[i].uiCodePage;
                            g_pCPInfo[i].uiFamilyCodePage = g_pCPInfoW[i].uiFamilyCodePage;
                            WideCharToMultiByte(CP_ACP, 0, (WCHAR *)g_pCPInfoW[i].wszDescription, -1, g_pCPInfo[i].wszDescription, sizeof(g_pCPInfo[i].wszDescription), NULL, NULL);
                            WideCharToMultiByte(CP_ACP, 0, (WCHAR *)g_pCPInfoW[i].wszWebCharset, -1, g_pCPInfo[i].wszWebCharset, sizeof(g_pCPInfo[i].wszWebCharset), NULL, NULL);
                            WideCharToMultiByte(CP_ACP, 0, (WCHAR *)g_pCPInfoW[i].wszHeaderCharset, -1, g_pCPInfo[i].wszHeaderCharset, sizeof(g_pCPInfo[i].wszHeaderCharset), NULL, NULL);
                            WideCharToMultiByte(CP_ACP, 0, (WCHAR *)g_pCPInfoW[i].wszBodyCharset, -1, g_pCPInfo[i].wszBodyCharset, sizeof(g_pCPInfo[i].wszBodyCharset), NULL, NULL);
                            WideCharToMultiByte(CP_ACP, 0, (WCHAR *)g_pCPInfoW[i].wszFixedWidthFont, -1, g_pCPInfo[i].wszFixedWidthFont, sizeof(g_pCPInfo[i].wszFixedWidthFont), NULL, NULL);
                            WideCharToMultiByte(CP_ACP, 0, (WCHAR *)g_pCPInfoW[i].wszProportionalFont, -1, g_pCPInfo[i].wszProportionalFont, sizeof(g_pCPInfo[i].wszProportionalFont), NULL, NULL);
                            g_pCPInfo[i].bGDICharset = g_pCPInfoW[i].bGDICharset;                            
                        }
                    }                    
                }
                LocalFree(g_pCPInfoW);
                g_pCPInfoW = NULL;
#endif
            }
            pEnumCP->Release();
        }
        if (bIsOE5)        
            pML2->Release();
        else
            pML->Release();
    }
    pCoUninitialize();

    return TRUE;
}

//
// FreeMimeCsetTable()
//
// Free string buffer of MimeCharsetTable[]'s string field
//
void FreeMimeCsetTable(void)
{
    if (NULL != g_pCPInfo)
    {
        LocalFree(g_pCPInfo);
        g_pCPInfo = NULL;
        g_cCPInfo = 0;
    }
}

//
// EnumFontsProc()
//
// Selects only one font per style
//
int CALLBACK EnumFontsProc(
    ENUMLOGFONTEX FAR*  elf,    // address of logical-font data 
    TEXTMETRIC FAR*  tm,    // address of physical-font data 
    DWORD  dwFontType,  // type of font 
    LPARAM  lParam  // address of application-defined data  
   )
{
    LOGFONT FAR*  lf;
        LPFONTSDATA pFnt;

    ASSERT(lParam);
    ASSERT(elf);
    pFnt = (LPFONTSDATA)lParam;

    lf = &(elf->elfLogFont);
    if ( dwFontType == DEVICE_FONTTYPE || dwFontType == RASTER_FONTTYPE )
        return TRUE; // keep going but don't use this font

    /* We don't use the SYMBOL fonts */
    if( lf->lfCharSet == SYMBOL_CHARSET )
        return TRUE;

    // we don't handle Mac Charset
    if (lf->lfCharSet == MAC_CHARSET )
        return TRUE;

    if ( IsVerticalFont(lf->lfFaceName) )
        return TRUE;  // keep going but don't use this font

    if ( lf->lfPitchAndFamily & FIXED_PITCH  )
    {
        if (CB_ERR == SendMessage(pFnt->hwndFixedCB, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)elf->elfLogFont.lfFaceName))
        {
            // add the font name to the combobox
            SendMessage(pFnt->hwndFixedCB, CB_ADDSTRING, 0, (LPARAM)elf->elfLogFont.lfFaceName);            
        }
    }

    if (CB_ERR == SendMessage(pFnt->hwndPropCB, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)elf->elfLogFont.lfFaceName))
    {
        // add the font name to the combobox
        SendMessage(pFnt->hwndPropCB, CB_ADDSTRING, 0, (LPARAM)elf->elfLogFont.lfFaceName);
    }
    return TRUE;
}

//
// FillFontComboBox()
//
// Fills hwndCB with the names of fonts of family dwCodePage.
//
BOOL FillFontComboBox(IN LPFONTSDATA pFnt, IN BYTE CodePage)
{
    HDC     hDC;
    LOGFONT lf;
    HWND    hWnd;
    BOOL    fReturn = FALSE;

    // get system font info
    hWnd = GetTopWindow(GetDesktopWindow());
    hDC = GetDC(hWnd);

    if (hDC)
    {
        lf.lfFaceName[0]    = 0;
        lf.lfPitchAndFamily = 0;
        lf.lfCharSet = CodePage;

        EnumFontFamiliesEx(hDC, &lf, (FONTENUMPROC)EnumFontsProc,
            (LPARAM)pFnt, 0);

        // everthing went fine
        fReturn = TRUE;
    }

    ReleaseDC(hWnd, hDC);

    return fReturn;

}   // FillFontComboBox()

//
// FillSizeComboBox()
//
// Fills font size combobox with the size of fonts.
//
BOOL FillSizeComboBox(IN LPFONTSDATA pFnt)
{
    int i;

    for (i = IDS_FONT_SIZE_SMALLEST; i <= IDS_FONT_SIZE_LARGEST ; i++)
    {
        TCHAR szSize[MAX_MIMEFACE_NAME];

        MLLoadString(i, szSize, sizeof(szSize));
        SendMessage(pFnt->hwndSizeCB, CB_ADDSTRING, 0, (LPARAM)szSize);
    }

    return TRUE;
}

//
// FillCharsetComboBoxes()
//
// Fills the Fixed, Prop, and MIME comboboxes with the appropriate
// font data
//
BOOL FillCharsetComboBoxes(LPFONTSDATA pFnt, DWORD dwCodePage)
{
    UINT i;
    int iPageInfo = -1;
    DWORD grfFlag;

    // erase all the comboboxes to start fresh
    SendMessage(pFnt->hwndPropCB,  CB_RESETCONTENT, 0, 0);
    SendMessage(pFnt->hwndFixedCB, CB_RESETCONTENT, 0, 0);
    SendMessage(pFnt->hwndSizeCB, CB_RESETCONTENT, 0, 0);
    SendMessage(pFnt->hwndMIMECB,  CB_RESETCONTENT, 0, 0);

    // BUGBUG: What happens if other one calls OpenFontDialog except Athena?
    grfFlag = StrCmpI(pFnt->lpszKeyPath, REGSTR_PATH_INTERNATIONAL)? MIMECONTF_MAILNEWS: MIMECONTF_BROWSER;

    for(i=0; i < g_cCPInfo; i++)
    {
        // find the codepage in our table
        if (g_pCPInfo[i].uiFamilyCodePage == (UINT)dwCodePage)
        {
            //
            // populate MIME combobox
            //

            if (g_pCPInfo[i].uiCodePage == (UINT)dwCodePage)
                iPageInfo = i;          // we store info per family codepage here

            // add mime type to combobox
            if (grfFlag & g_pCPInfo[i].dwFlags)
            {
                // HACK: We need to remove Japanese JIS 1 Byte Kana and Korean for MAILNEWS.
                //         949 : Korean. We are using Korean (Auto Detect) instead
                //       50225 : Korean ISO
                //       50221 : Japanese JIS 1 byte Kana-ESC
                //       50222 : Japanese JIS 1 byte Kana-SIO
                if (grfFlag & MIMECONTF_MAILNEWS)
                {
                    if (g_pCPInfo[i].uiCodePage == 949 || g_pCPInfo[i].uiCodePage == 50221 || g_pCPInfo[i].uiCodePage == 50222 || g_pCPInfo[i].uiCodePage == 50225)
                        continue;
                }
                SendMessage(pFnt->hwndMIMECB, CB_ADDSTRING, 0, (LPARAM)g_pCPInfo[i].wszDescription);
            }

        }   // if CodePage

    }   // for i

    if (-1 != iPageInfo)
    {
        // if nothing is defined, then copy the first possible value that
        // we know of from our table
        if (!pFnt->page[iPageInfo].szMIMEFont[0])
        {
            if (grfFlag & g_pCPInfo[iPageInfo].dwFlags)
                StrCpyN(pFnt->page[iPageInfo].szMIMEFont, g_pCPInfo[iPageInfo].wszDescription, ARRAYSIZE(pFnt->page[iPageInfo].szMIMEFont));
            else
            {
                for (i = 0; i < g_cCPInfo; i++)
                {
                    if (g_pCPInfo[iPageInfo].uiCodePage == g_pCPInfo[i].uiFamilyCodePage)
                    {
                        if (grfFlag & g_pCPInfo[i].dwFlags)
                        {
                            StrCpyN(pFnt->page[iPageInfo].szMIMEFont, g_pCPInfo[i].wszDescription, ARRAYSIZE(pFnt->page[iPageInfo].szMIMEFont));
                            break;
                        }
                    }                        
                }
            }
        }

        // select the current default
        SendMessage(pFnt->hwndMIMECB, CB_SELECTSTRING, (WPARAM)-1,
            (LPARAM)pFnt->page[iPageInfo].szMIMEFont);

        // Enable/disable MIME is when there is only one possibility
        EnableWindow(pFnt->hwndMIMECB, (1 < SendMessage(pFnt->hwndMIMECB, CB_GETCOUNT, 0, (LPARAM)0)) && !g_restrict.fFonts);
                        
        // Add fonts to combobox
        FillFontComboBox(pFnt, g_pCPInfo[iPageInfo].bGDICharset);

#ifdef UNIX
        /* We would have called EnumFontFamiliesEx wherein we would have
         * have populated the fonts list boxes with substitute fonts if any
         *
         * So, before we populate the proportional and the fixed fonts below,
         * we must query and use substitute fonts if avbl.
         */
        {
            CHAR szSubstFont[MAX_MIMEFACE_NAME+1];
            DWORD cchSubstFont = MAX_MIMEFACE_NAME + 1;
        CHAR szFont[MAX_MIMEFACE_NAME + 1];
           
            WideCharToMultiByte(CP_ACP, 0, pFnt->page[iPageInfo].szPropFont, -1, szFont, 
                   MAX_MIMEFACE_NAME + 1, NULL, NULL);
            if ((ERROR_SUCCESS == MwGetSubstituteFont(szFont, szSubstFont, &cchSubstFont)) && 
                cchSubstFont) 
            {
                MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szSubstFont, -1, 
                   pFnt->page[iPageInfo].szPropFont, MAX_MIMEFACE_NAME + 1);
            }

            WideCharToMultiByte(CP_ACP, 0, pFnt->page[iPageInfo].szFixedFont, -1, szFont, 
                   MAX_MIMEFACE_NAME + 1, NULL, NULL);
            cchSubstFont = MAX_MIMEFACE_NAME + 1;
            if ((ERROR_SUCCESS == MwGetSubstituteFont(szFont, szSubstFont, &cchSubstFont)) && 
                cchSubstFont) 
            {
                MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szSubstFont, -1, 
                   pFnt->page[iPageInfo].szFixedFont, MAX_MIMEFACE_NAME + 1);
            }
        }
#endif /* UNIX */

        // select the current prop default
        SendMessage(pFnt->hwndPropCB, CB_SELECTSTRING, (WPARAM)-1,
            (LPARAM)pFnt->page[iPageInfo].szPropFont);

        // select the current fixed default
        SendMessage(pFnt->hwndFixedCB, CB_SELECTSTRING, (WPARAM)-1,
            (LPARAM)pFnt->page[iPageInfo].szFixedFont);

        // Add font sizes to combobox
        FillSizeComboBox(pFnt);

        // select the current size default
        SendMessage(pFnt->hwndSizeCB, CB_SETCURSEL, (WPARAM)pFnt->page[iPageInfo].dwFontSize, (LPARAM)0);

        // we handled it
        return TRUE;
    }

    return FALSE;

}   // FillCharsetComboBoxes()

//
// FontsDlgInit()
//
// Initializes the Fonts dialog.
//
BOOL FontsDlgInit(IN HWND hDlg, LPCTSTR lpszKeyPath)
{
    HKEY    hkey;
    DWORD   grfFlag;
    DWORD   dw;
    DWORD   cb;
    DWORD   i;
    BOOL    bIsOE5 = FALSE;

    TCHAR   szKey[1024];

    LPFONTSDATA  pFnt;  // localize data

    if (!hDlg)
        return FALSE;   // nothing to initialize

    // set system default character set where we possibly show
    // the strings in native language.
    SHSetDefaultDialogFont(hDlg, IDC_FONTS_PROP_FONT_COMBO);
    SHSetDefaultDialogFont(hDlg, IDC_FONTS_FIXED_FONT_COMBO);
    SHSetDefaultDialogFont(hDlg, IDC_FONTS_MIME_FONT_COMBO);
    SHSetDefaultDialogFont(hDlg, IDC_FONTS_DEFAULT_LANG_TEXT);
    SHSetDefaultDialogFont(hDlg, IDC_FONTS_CODE_PAGES_LIST);

    // get some space to store local data
    // NOTE: LocalAlloc already zeroes the memory
    pFnt = (LPFONTSDATA)LocalAlloc(LPTR, sizeof(*pFnt));
    if (!pFnt)
    {
        EndDialog(hDlg, 0);
        return FALSE;
    }

    // We distinguish OE5 and OE4 by searching for "5.0" in its registry path, 
    // It works as long as there is no spec. change in OE5
    if (NULL != StrStr(lpszKeyPath, TEXT("5.0")))
        bIsOE5 = TRUE;

    if (!InitMimeCsetTable(bIsOE5))
    {
        EndDialog(hDlg, 0);
        return FALSE;
    }

    if (NULL == pFnt->page)
    {
        pFnt->page = (CODEPAGEDATA*)LocalAlloc(LPTR, sizeof(CODEPAGEDATA) * g_cCPInfo);
        if (NULL == pFnt->page)
        {
            EndDialog(hDlg, 0);
            return FALSE;
        }
    }

    // associate the memory with the dialog window
    SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR) pFnt);

    // save the dialog handle
    pFnt->hDlg = hDlg;

    // get the dialog items
    pFnt->hwndPropCB  = GetDlgItem(pFnt->hDlg, IDC_FONTS_PROP_FONT_COMBO);
    pFnt->hwndFixedCB = GetDlgItem(pFnt->hDlg, IDC_FONTS_FIXED_FONT_COMBO);
    pFnt->hwndSizeCB = GetDlgItem(pFnt->hDlg, IDC_FONTS_SIZE_FONT_COMBO);
    pFnt->hwndMIMECB  = GetDlgItem(pFnt->hDlg, IDC_FONTS_MIME_FONT_COMBO);    
    pFnt->hwndNamesLB = GetDlgItem(pFnt->hDlg, IDC_FONTS_CODE_PAGES_LIST);    
    pFnt->lpszKeyPath = lpszKeyPath ? lpszKeyPath: REGSTR_PATH_INTERNATIONAL;
    pFnt->dwDefaultCodePage = GetACP();
    // get values from registry
    if (RegOpenKeyEx(HKEY_CURRENT_USER, pFnt->lpszKeyPath, NULL, KEY_READ, &hkey)
        == ERROR_SUCCESS)
    {
        cb = sizeof(dw);
        if (RegQueryValueEx(hkey, REGSTR_VAL_DEFAULT_CODEPAGE, NULL, NULL, (LPBYTE)&dw, &cb)
          == ERROR_SUCCESS)
        {
            pFnt->dwDefaultCodePage = dw;
        }
        RegCloseKey(hkey);
    }

    // BUGBUG: What happens if other one calls OpenFontDialog except Athena?
    grfFlag = StrCmpI(pFnt->lpszKeyPath, REGSTR_PATH_INTERNATIONAL)? MIMECONTF_MAILNEWS: MIMECONTF_BROWSER;

    for (i = 0; i < g_cCPInfo; i++)
    {
        if (g_pCPInfo[i].uiCodePage == g_pCPInfo[i].uiFamilyCodePage)
        {
            int iDef;
            UINT j;

            iDef = -1;
            if (0 == (grfFlag & g_pCPInfo[i].dwFlags))
            {
                for (j = 0; j < g_cCPInfo; j++)
                {
                    if (g_pCPInfo[i].uiCodePage == g_pCPInfo[j].uiFamilyCodePage)
                    {
                        if (grfFlag & g_pCPInfo[j].dwFlags)
                        {
                            iDef = j;
                            break;
                        }
                    }
                }
                if (-1 == iDef)
                    continue;
            }

            if (g_pCPInfo[i].uiCodePage == 50001) // skip CP_AUTO
                continue;

            wnsprintf(szKey, ARRAYSIZE(szKey), TEXT("%s\\%u"), pFnt->lpszKeyPath, g_pCPInfo[i].uiCodePage);
            if (RegOpenKeyEx(HKEY_CURRENT_USER, szKey, NULL, KEY_READ, &hkey) == ERROR_SUCCESS)
            {
                cb = sizeof(pFnt->page[i].szFriendlyName);
                if (RegQueryValueEx(hkey, REGSTR_VAL_FONT_SCRIPT, NULL, NULL,
                        (LPBYTE)&pFnt->page[i].szFriendlyName, &cb)
                    != ERROR_SUCCESS)
                {
                    TCHAR *p;

                    StrCpyN(pFnt->page[i].szFriendlyName, g_pCPInfo[i].wszDescription, ARRAYSIZE(pFnt->page[i].szFriendlyName));
                    for (p = pFnt->page[i].szFriendlyName; *p != TEXT('\0'); p = CharNext(p))
                    {
                        // BUGBUG: We'd better have a source of this string else where.
                        if (*p == TEXT('('))
                        {
                            *p = TEXT('\0');
                            break;
                        }
                    }
                }

                cb = sizeof(dw);
                if (RegQueryValueEx(hkey, REGSTR_VAL_DEF_INETENCODING, NULL, NULL, (LPBYTE)&dw, &cb)
                    != ERROR_SUCCESS)
                {
                    dw = (DWORD)g_pCPInfo[i].uiCodePage;
                    // HACK ! It's only for Japanese Auto Select as Japanese default.
                    if (dw == 932)      // 932 : Japanese Windows CodePage
                        dw = 50932;     // 50932 : Japanese Auto Select InternetEncoding
                }
                for (j = 0; j < g_cCPInfo; j++)
                {
                    if (g_pCPInfo[j].uiCodePage == (UINT)dw)
                    {
                        if (grfFlag & g_pCPInfo[j].dwFlags)
                            StrCpyN(pFnt->page[i].szMIMEFont, g_pCPInfo[j].wszDescription, ARRAYSIZE(pFnt->page[i].szMIMEFont));
                        else if (-1 != iDef)
                            StrCpyN(pFnt->page[i].szMIMEFont, g_pCPInfo[iDef].wszDescription, ARRAYSIZE(pFnt->page[i].szMIMEFont));
                        else
                            pFnt->page[i].szMIMEFont[0] = TEXT('\0');
                        break;
                    }
                }
            
                cb = sizeof(pFnt->page[i].szFixedFont);
                if (RegQueryValueEx(hkey, REGSTR_VAL_FIXED_FONT, NULL, NULL,
                        (LPBYTE)pFnt->page[i].szFixedFont, &cb)
                    != ERROR_SUCCESS)
                {
                    StrCpyN(pFnt->page[i].szFixedFont, g_pCPInfo[i].wszFixedWidthFont, ARRAYSIZE(pFnt->page[i].szFixedFont));
                }
            
                cb = sizeof(pFnt->page[i].szPropFont);
                if (RegQueryValueEx(hkey, REGSTR_VAL_PROP_FONT, NULL, NULL,
                        (LPBYTE)pFnt->page[i].szPropFont, &cb)
                    != ERROR_SUCCESS)
                {
                    StrCpyN(pFnt->page[i].szPropFont, g_pCPInfo[i].wszProportionalFont, ARRAYSIZE(pFnt->page[i].szPropFont));
                }

                cb = sizeof(pFnt->page[i].dwFontSize);
                if (RegQueryValueEx(hkey, REGSTR_VAL_FONT_SIZE, NULL, NULL,
                        (LPBYTE)&pFnt->page[i].dwFontSize, &cb)
                    != ERROR_SUCCESS)
                {
                    pFnt->page[i].dwFontSize = REGSTR_VAL_FONT_SIZE_DEF;
                }
                RegCloseKey(hkey);

            }
            else
            {
                UINT j;
                TCHAR *p;

                StrCpyN(pFnt->page[i].szFriendlyName, g_pCPInfo[i].wszDescription, ARRAYSIZE(pFnt->page[i].szFriendlyName));
                for (p = pFnt->page[i].szFriendlyName; *p != TEXT('\0'); p = CharNext(p))
                {
                    // BUGBUG: We'd better have a source of this string else where.
                    if (*p == TEXT('('))
                    {
                        *p = TEXT('\0');
                        break;
                    }
                }
                j = (grfFlag & g_pCPInfo[i].dwFlags)? i: iDef;
                // HACK ! It's only for Japanese Auto Select as Japanese default.
                if (g_pCPInfo[j].uiCodePage == 932) // 932 : Japanese Windows CodePage
                {
                    for (j = 0; j < g_cCPInfo; j++)
                    {
                        if (g_pCPInfo[j].uiCodePage == 50932)   // 50932 : Japanese Auto Select InternetEncoding
                            break;
                    }
                }
                StrCpyN(pFnt->page[i].szMIMEFont, g_pCPInfo[j].wszDescription, ARRAYSIZE(pFnt->page[i].szMIMEFont));
                StrCpyN(pFnt->page[i].szFixedFont, g_pCPInfo[i].wszFixedWidthFont, ARRAYSIZE(pFnt->page[i].szFixedFont));
                StrCpyN(pFnt->page[i].szPropFont, g_pCPInfo[i].wszProportionalFont, ARRAYSIZE(pFnt->page[i].szPropFont));
                pFnt->page[i].dwFontSize = REGSTR_VAL_FONT_SIZE_DEF;
            }

            // add the name to the listbox            
            SendMessage(pFnt->hwndNamesLB, LB_ADDSTRING, 0, (LPARAM)pFnt->page[i].szFriendlyName);            

            // check to see if it is the default code page
            if (pFnt->dwDefaultCodePage == g_pCPInfo[i].uiCodePage)
            {
                if (LB_ERR == SendMessage(pFnt->hwndNamesLB, LB_SELECTSTRING, (WPARAM)-1, (LPARAM)pFnt->page[i].szFriendlyName))
                {
                    // Hack shlwapi problems for Win9x.
                    CHAR szAnsiString[1024] = {0};
                    WideCharToMultiByte(CP_ACP, 0, pFnt->page[i].szFriendlyName, -1, szAnsiString, 1024, NULL, NULL);
                    SendMessageA(pFnt->hwndNamesLB, LB_SELECTSTRING, (WPARAM)-1, (LPARAM)szAnsiString);
                }

                SetDlgItemText(pFnt->hDlg, IDC_FONTS_DEFAULT_LANG_TEXT, pFnt->page[i].szFriendlyName);
            }
        }
    }
    
    FillCharsetComboBoxes(pFnt, pFnt->dwDefaultCodePage);

    pFnt->bChanged = FALSE;

    if( g_restrict.fFonts )
    {
        EnableWindow( GetDlgItem( hDlg, IDC_FONTS_PROP_FONT_COMBO ), FALSE);
        EnableWindow( GetDlgItem( hDlg, IDC_FONTS_FIXED_FONT_COMBO ), FALSE);
        EnableWindow( GetDlgItem( hDlg, IDC_FONTS_SIZE_FONT_COMBO ), FALSE);
        EnableWindow( GetDlgItem( hDlg, IDC_FONTS_MIME_FONT_COMBO ), FALSE);
        EnableWindow( GetDlgItem( hDlg, IDC_FONTS_CODE_PAGES_LIST ), FALSE);
        EnableWindow( GetDlgItem( hDlg, IDC_FONTS_SETDEFAULT_BUTTON ), FALSE);

    }

    // everything ok
    return TRUE;

}   // FontsDlgInit()


//
// SaveFontsData()
//
// Save the new fonts settings into regestry
//
void SaveFontsData(LPFONTSDATA pFnt)
{
    HKEY    hkeyCodePage;
    TCHAR   szCodePage      [MAX_MIMEFACE_NAME];

    HKEY    hkey;
    DWORD   dw;

    // get values from registry
    if (RegCreateKeyEx(HKEY_CURRENT_USER, pFnt->lpszKeyPath, NULL, NULL, NULL, KEY_WRITE, NULL, &hkey, &dw)
        == ERROR_SUCCESS)
    {
        UINT i;
 
        RegSetValueEx(hkey, REGSTR_VAL_DEFAULT_CODEPAGE, NULL, REG_BINARY, (LPBYTE)&pFnt->dwDefaultCodePage, sizeof(pFnt->dwDefaultCodePage));
        
        for(i = 0; i < g_cCPInfo; i++)
        {
            if (g_pCPInfo[i].uiCodePage == g_pCPInfo[i].uiFamilyCodePage)
            {
                wnsprintf(szCodePage, ARRAYSIZE(szCodePage), TEXT("%u"), g_pCPInfo[i].uiCodePage);
                if (RegCreateKeyEx(hkey, szCodePage, NULL, NULL, NULL, KEY_WRITE, NULL, &hkeyCodePage, &dw) == ERROR_SUCCESS)
                {
                    UINT j;

                    RegSetValueEx(hkeyCodePage, REGSTR_VAL_FONT_SCRIPT, NULL, REG_SZ,
                            (LPBYTE)&pFnt->page[i].szFriendlyName, 
                            (lstrlen(pFnt->page[i].szFriendlyName)+1)*sizeof(TCHAR));
                    
                    for (j = 0; j < g_cCPInfo; j++)
                    {
                        if (!StrCmpI(g_pCPInfo[j].wszDescription, pFnt->page[i].szMIMEFont))
                        {
                            dw = g_pCPInfo[j].uiCodePage;
                            break;
                        }
                    }
                    RegSetValueEx(hkeyCodePage, REGSTR_VAL_DEF_INETENCODING, NULL, REG_BINARY,
                            (LPBYTE)&dw, sizeof(dw));
                    
                    RegSetValueEx(hkeyCodePage, REGSTR_VAL_FIXED_FONT, NULL, REG_SZ,
                            (LPBYTE)pFnt->page[i].szFixedFont, 
                            (lstrlen(pFnt->page[i].szFixedFont)+1)*sizeof(TCHAR));
                    
                    RegSetValueEx(hkeyCodePage, REGSTR_VAL_PROP_FONT, NULL, REG_SZ,
                            (LPBYTE)pFnt->page[i].szPropFont, 
                            (lstrlen(pFnt->page[i].szPropFont)+1)*sizeof(TCHAR));
                    
                    RegSetValueEx(hkeyCodePage, REGSTR_VAL_FONT_SIZE, NULL, REG_BINARY,
                            (LPBYTE)&pFnt->page[i].dwFontSize,
                            sizeof(pFnt->page[i].dwFontSize));

                    RegCloseKey(hkeyCodePage);
                    
                }   // if RegCreateKeyEx

            }   // if uiCodePage == uiFamilyCodePage

        }   // for

        RegCloseKey(hkey);

    }   // if RegCreateKeyEx

}   // SaveFontsData()

//
// FontsOnCommand()
//
// Handles WM_COMMAN Dmessage for the Fonts subdialog
//
BOOL FontsOnCommand(LPFONTSDATA pFnt, UINT id, UINT nCmd)
{
    switch(id)
    {
        case IDOK:
            if (pFnt->bChanged)
            {
                SaveFontsData(pFnt);
                
                // tell MSHTML to pick up changes and update
                UpdateAllWindows();
            }
            return TRUE;    // exit dialog

        case IDCANCEL:
            return TRUE;    // exit dialog

        case IDC_FONTS_MIME_FONT_COMBO:
            if (nCmd==CBN_SELCHANGE)
            {
                g_fChangedMime = TRUE;   // tell MSHTML that the Mime has changed
            }
            // fall thru...

        case IDC_FONTS_PROP_FONT_COMBO:
        case IDC_FONTS_FIXED_FONT_COMBO:
        case IDC_FONTS_SIZE_FONT_COMBO:
            if (nCmd==CBN_SELCHANGE)
            {
                UINT i;
                TCHAR   szCodePage[MAX_MIMECP_NAME];

                pFnt->bChanged = TRUE;  // we need to save
                
                // find the currently selected item in the list box
                INT_PTR itmp = SendMessage(pFnt->hwndNamesLB, LB_GETCURSEL, 0, 0);
                SendMessage(pFnt->hwndNamesLB, LB_GETTEXT, itmp, (LPARAM)szCodePage);
                
                // find the code page from the text
                for(i=0; i < g_cCPInfo; i++)
                {
                    if (!StrCmpI(szCodePage, pFnt->page[i].szFriendlyName))
                    {             
                        // grab the new values
                        GetDlgItemText(pFnt->hDlg, IDC_FONTS_PROP_FONT_COMBO,
                            pFnt->page[i].szPropFont, ARRAYSIZE(pFnt->page[i].szPropFont));
                        GetDlgItemText(pFnt->hDlg, IDC_FONTS_FIXED_FONT_COMBO,
                            pFnt->page[i].szFixedFont, ARRAYSIZE(pFnt->page[i].szFixedFont));
                        pFnt->page[i].dwFontSize = (int) SendMessage(pFnt->hwndSizeCB, CB_GETCURSEL, 0, 0);
                        GetDlgItemText(pFnt->hDlg, IDC_FONTS_MIME_FONT_COMBO,
                            pFnt->page[i].szMIMEFont, ARRAYSIZE(pFnt->page[i].szMIMEFont));
                        break;
                    }
                }
                // if we don't find it... we are going to keep the default

                ASSERT(i < g_cCPInfo);  // something went wrong

            }
            break;

        case IDC_FONTS_SETDEFAULT_BUTTON:
            {
                UINT i;
                TCHAR   szCodePage[MAX_MIMECP_NAME];

                pFnt->bChanged = TRUE;  // we need to save

                // get the newly selected charset
                INT_PTR itmp = SendMessage(pFnt->hwndNamesLB, LB_GETCURSEL, 0, 0);
                SendMessage(pFnt->hwndNamesLB, LB_GETTEXT, itmp, (LPARAM)szCodePage);

                // set the newly selected charset text
                SetDlgItemText(pFnt->hDlg, IDC_FONTS_DEFAULT_LANG_TEXT, szCodePage);

                // find the code page from the text
                for (i = 0; i < g_cCPInfo; i++)
                {
                    if (!StrCmpI(szCodePage, pFnt->page[i].szFriendlyName))
                    {
                        pFnt->dwDefaultCodePage = g_pCPInfo[i].uiFamilyCodePage;
                        g_fChangedMime = TRUE;
                        break;
                    }
                }
                // if we don't find it... we are going to keep the default

                ASSERT(i < g_cCPInfo);  // something went wrong
            }
            break;
        
        case IDC_FONTS_CODE_PAGES_LIST:
            if (nCmd==LBN_SELCHANGE)
            {
                UINT i;
                TCHAR   szCodePage[MAX_MIMECP_NAME];

                INT_PTR itmp = SendMessage(pFnt->hwndNamesLB, LB_GETCURSEL, 0, 0);
                SendMessage(pFnt->hwndNamesLB, LB_GETTEXT, itmp, (LPARAM)szCodePage);
                
                // find the code page from the text
                for(i=0; i < g_cCPInfo; i++)
                {
                    if (!StrCmpI(szCodePage, pFnt->page[i].szFriendlyName))
                    {
                        FillCharsetComboBoxes(pFnt, g_pCPInfo[i].uiFamilyCodePage);
                        break;
                    }
                }
            }
            break;

    }
    
    // don't exit dialog
    return FALSE;
}

//
// FontsDlgProc()
//
// Message handler for the "Fonts" subdialog.
//
INT_PTR CALLBACK FontsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPFONTSDATA pFnt = (LPFONTSDATA) GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            return FontsDlgInit(hDlg, (LPTSTR)lParam);
            
        case WM_DESTROY:
            // give back the memory
            FreeMimeCsetTable();

            // destroy font if we created
            SHRemoveDefaultDialogFont(hDlg);

            if (pFnt)
            {
                if (pFnt->page)
                    LocalFree(pFnt->page);
                LocalFree(pFnt);
            }
            break;

        case WM_COMMAND:
            if (FontsOnCommand(pFnt, LOWORD(wParam), HIWORD(wParam)))
                EndDialog(hDlg, LOWORD(wParam) == IDOK? 1: 0);
            break;

        case WM_HELP:           // F1
            ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
                HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_CONTEXTMENU:    // right mouse click
            ResWinHelp( (HWND) wParam, IDS_HELPFILE,
                HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        default:
            return FALSE;
    }
    return TRUE;
}


//
// EXTERNAL API
//
STDAPI_(INT_PTR) OpenFontsDialog(HWND hDlg, LPCSTR lpszKeyPath)
{
#ifdef UNICODE
    WCHAR   wszKeyPath[1024];
    MultiByteToWideChar(CP_ACP, 0, (char *)lpszKeyPath, -1, wszKeyPath, ARRAYSIZE(wszKeyPath));
    return DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_FONTS_IE4), hDlg, FontsDlgProc, (LPARAM) wszKeyPath);
#else
    return DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_FONTS_IE4), hDlg, FontsDlgProc, (LPARAM) lpszKeyPath);
#endif // UNICODE
}

// provide script based font dialog
STDAPI_(INT_PTR) OpenFontsDialogEx(HWND hDlg, LPCTSTR lpszKeyPath)
{
    INT_PTR nRet = -1;
    HRESULT hr;
    BOOL    fOLEPresent;

    if (hOLE32 != NULL)
    {
        fOLEPresent = TRUE;
    }
    else
    {
        fOLEPresent = _StartOLE32();
    }

    ASSERT(fOLEPresent);
    if (fOLEPresent)
    {
        ASSERT(IS_VALID_HANDLE(hOLE32, MODULE));
        ASSERT(IS_VALID_CODE_PTR(pCoInitialize, PCOINIT));

        hr = pCoInitialize(NULL);
        if (SUCCEEDED(hr))
        {
            nRet = DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_FONTS), hDlg, FontsDlgProcEx, (LPARAM) lpszKeyPath);
        }
    }

    // Release interface
    if (g_pMLFlnk2)
    {
        g_pMLFlnk2->Release();
        g_pMLFlnk2 = NULL;
    }

    ASSERT(IS_VALID_CODE_PTR(pCoUninitialize, PCOUNIT));
    pCoUninitialize();

    return nRet;
}
