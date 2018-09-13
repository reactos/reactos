//================================================================
//
//  (C) Copyright MICROSOFT Corp., 1994
//
//  TITLE:       FILETYPE.CPP
//  VERSION:     1.0
//  DATE:        5/10/94
//  AUTHOR:      Vince Roggero (vincentr)
//
//================================================================
//
//  CHANGE LOG:
//
//  DATE         REV DESCRIPTION
//  -----------  --- ---------------------------------------------
//               VMR Original version
//================================================================

//
// Note: this file has been moved from explorer to shell32, then
//       to shdocvw.  It works like this:
//
//          Win95           - implemented in explorer
//          IE 1.0 - 3.0    - implemented in URL.DLL with MIME type support
//          Old Nashville   - moved to shell32, w/ MIME type support
//          NT 4            - ditto
//          IE4.01          - moved to shdocvw
//          NT5/IE5         - moved back to shell32 (home sweet home), changed to cpp file
//

//================================================================
//  View.Options.File Types
//================================================================

#include "shellprv.h"

#include "filetype.h"
#include "ftprop.h"
#include "ids.h"
#include <shellids.h>  // more ids
#include <limits.h>

#ifdef WINNT
#define CreateFileTypePage  CreateFileTypePageW
#else
#define CreateFileTypePage  CreateFileTypePageA
#endif

HRESULT CreateFileTypePageW(HPROPSHEETPAGE * phpsp, LPVOID pvReserved);
HRESULT CreateFileTypePageA(HPROPSHEETPAGE * phpsp, LPVOID pvReserved);

/* stuff a point value packed in an LPARAM into a POINT */

static HIMAGELIST  g_himlSysSmall = NULL;
static HIMAGELIST  g_himlSysLarge = NULL;

#ifndef NO_FILETYPES

STDAPI_(DWORD) GetFileTypeAttributes(HKEY hkeyFT)
{
    DWORD dwAttributeValue = 0;
    DWORD dwAttributeSize = SIZEOF(dwAttributeValue);
    
    if (hkeyFT == NULL)
        return 0;
    
    SHQueryValueEx(hkeyFT, TEXT("EditFlags"), NULL, NULL, (LPBYTE)&dwAttributeValue, &dwAttributeSize);
    
    return(dwAttributeValue);
}

BOOL ExtToTypeNameAndId( LPCTSTR pszExt, LPTSTR pszDesc, DWORD *pdwDesc, LPTSTR pszId, DWORD *pdwId )
{
    LONG err;
    DWORD dwNm;
    BOOL bRC = TRUE;

    // NOTE pdwDesc is count of BYTES

    err = SHGetValue(HKEY_CLASSES_ROOT, pszExt, NULL, NULL, (LPVOID)pszId, pdwId);
    if (err == ERROR_SUCCESS && *pszId)
    {
        dwNm = *pdwDesc;        // if we fail we will still have name size to use
        err = SHGetValue(HKEY_CLASSES_ROOT, pszId, NULL, NULL, (LPVOID)pszDesc, &dwNm);
        if (err != ERROR_SUCCESS || !*pszDesc)
                goto Error;
        *pdwDesc = dwNm;
    }
    else
    {
        TCHAR szExt[MAX_PATH];  // "TXT"
        TCHAR szTemplate[128];   // "%s File"
        TCHAR szRet[MAX_PATH+20];        // "TXT File"
Error:
        bRC = FALSE;

        lstrcpy(pszId, pszExt);

        pszExt++;
        lstrcpy(szExt, pszExt);
        CharUpper(szExt);
        LoadString(HINST_THISDLL, IDS_EXTTYPETEMPLATE, szTemplate, ARRAYSIZE(szTemplate));
        wsprintf(szRet, szTemplate, szExt);
        lstrcpyn(pszDesc, szRet, (*pdwDesc) / SIZEOF(TCHAR));
        *pdwDesc = lstrlen(pszDesc) * SIZEOF(TCHAR);
    }
    return(bRC);
}

UINT CALLBACK FileTypePageCallback( HWND hwnd, 
                                    UINT uMsg,
                                    LPPROPSHEETPAGE ppsp)
{
    UINT    uResult = 0;
    
    switch (uMsg)
    {
        case PSPCB_CREATE:
            uResult = 1;
            break;

        case PSPCB_RELEASE:
            if (ppsp->lParam)
            {
                CFTPropDlg* pPropDlg = (CFTPropDlg*)ppsp->lParam;

                delete pPropDlg;
            }
            break;
    }
    
    return uResult;
}

/*----------------------------------------------------------
Purpose: Create the File Type property page and return a handle
to the page.

  The instance data related to this page is freed in the
  page's callback, as specified
  
    Returns: NO_ERROR if the page is created
    
      Cond:    --
*/
HRESULT CreateFileTypePage(OUT HPROPSHEETPAGE * phpsp,
                           IN  LPVOID           pvReserved)    // Must be NULL
{
    HRESULT hres;
    
    ASSERT(phpsp);
    ASSERT( !pvReserved );      // right now this must be NULL
    
    if ( !phpsp || pvReserved )
    {
        hres = E_INVALIDARG;
    }
    else
    {
        CFTPropDlg* pPropDlg = new CFTPropDlg(FALSE);            // FALSE means don't auto-delete pPropDlg on WM_DESTROY
        PROPSHEETPAGE psp;

        *phpsp = NULL;
        
        Shell_GetImageLists(&g_himlSysLarge, &g_himlSysSmall);
        
        psp.dwSize      = SIZEOF(psp);
        psp.dwFlags     = PSP_DEFAULT | PSP_USECALLBACK;
        psp.hInstance   = g_hinst;
        psp.pfnCallback = FileTypePageCallback;
        psp.pszTemplate = MAKEINTRESOURCE(DLG_FILETYPEOPTIONS);
        psp.pfnDlgProc  = CFTDlg::FTDlgWndProc;

        if (!pPropDlg)
        {
            hres = E_OUTOFMEMORY;
        }
        else
        {
            psp.lParam = (LPARAM)pPropDlg;

            *phpsp = CreatePropertySheetPage(&psp);
            
            if (*phpsp)
            {
                hres = NO_ERROR;
            }
            else
            {
                hres = E_OUTOFMEMORY;
                delete pPropDlg;
            }
        }
    }
    
    return hres;
}


//========================================================================
// CFileTypes Class definition
//========================================================================

class CFileTypes : public IShellPropSheetExt
{
public:
    STDMETHOD (QueryInterface)(REFIID, void **);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD (AddPages)(LPFNADDPROPSHEETPAGE, LPARAM);
    STDMETHOD (ReplacePage)(UINT, LPFNADDPROPSHEETPAGE, LPARAM);
    
    UINT                cRef;
};


/*----------------------------------------------------------
Purpose: QueryInterface method

  Returns:
  Cond:    --
*/
STDMETHODIMP CFileTypes::QueryInterface(REFIID riid, void **ppvObj)
{
    HRESULT hres;
    
    ASSERT(IS_VALID_WRITE_PTR(this, CFileTypes));
    
    if (IsEqualIID(riid, IID_IShellPropSheetExt) ||
        IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = this;
        this->cRef++;
        
        hres = NOERROR;
    }
    else
    {
        *ppvObj = NULL;
        hres = E_NOINTERFACE;
    }
    
    return hres;
}


/*----------------------------------------------------------
Purpose: AddRef method

  Returns:
  Cond:    --
*/
STDMETHODIMP_(ULONG) CFileTypes::AddRef()
{
    ASSERT(IS_VALID_WRITE_PTR(this, CFileTypes));
    
    this->cRef++;
    return this->cRef;
}


/*----------------------------------------------------------
Purpose: Release method

  Returns:
  Cond:    --
*/
STDMETHODIMP_(ULONG) CFileTypes::Release()
{
    ASSERT(IS_VALID_WRITE_PTR(this, CFileTypes));
    
    this->cRef--;
    if (this->cRef > 0)
    {
        return this->cRef;
    }
    
    delete this;
    DllRelease();
    return 0;
}


/*----------------------------------------------------------
Purpose: IShellPropSheetExt::AddPages for CFileTypes

  This is called for full-shell installations, via CLSID
  {B091E540-83E3-11CF-A713-0020AFD79762}.
  
*/
STDMETHODIMP CFileTypes::AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    HRESULT hres;
    HPROPSHEETPAGE hpsp;
    
    ASSERT(IS_VALID_WRITE_PTR(this, CFileTypes));
    
    // Make sure the FileIconTable is init properly.  If brought in
    // by inetcpl we need to set this true...
    FileIconInit(TRUE);
    
    // We need to run the unicode version on NT, to avoid all bugs
    // that occur with the ANSI version (due to unicode-to-ansi 
    // conversions of file names).
    
#ifdef WINNT
    hres = CreateFileTypePageW(&hpsp, NULL);
#else
    hres = CreateFileTypePageA(&hpsp, NULL);
#endif
    
    if (SUCCEEDED(hres) && !pfnAddPage(hpsp, lParam) )
    {
        DestroyPropertySheetPage(hpsp);
        hres = E_FAIL;
    }
    
    return hres;
}


/*----------------------------------------------------------
Purpose: IShellPropSheetExt::ReplacePage for CFileTypes

  This is called on browser-only installations, via 
  the FileTypes Hook.  Our hook CLSID is 
  {FBF23B41-E3F0-101B-8488-00AA003E56F8}.
  
*/
STDMETHODIMP CFileTypes::ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE pfnReplaceWith, LPARAM lParam)
{
    HRESULT hres = E_NOTIMPL;
    
    ASSERT(IS_VALID_WRITE_PTR(this, CFileTypes));
    
    if (EXPPS_FILETYPES == uPageID)
    {
        HPROPSHEETPAGE hpsp;
        
        // We need to run the unicode version on NT, to avoid all bugs
        // that occur with the ANSI version (due to unicode-to-ansi 
        // conversions of file names).
        
#ifdef WINNT
        hres = CreateFileTypePageW(&hpsp, NULL);
#else
        hres = CreateFileTypePageA(&hpsp, NULL);
#endif
        
        if (SUCCEEDED(hres) && !pfnReplaceWith(hpsp, lParam) )
        {
            hres = E_FAIL;
        }
    }
    
    return hres;
}


/*----------------------------------------------------------
Purpose: Create a FileTypes object that provides an IShellPropSheet

  The file types object implements the file types page.
  This page is accessed in a couple different ways, depending
  on the platform and version.
  
    Win95/NT4  
    ---------
    
      The shell will bind to the CLSID registered in "HKLM\Software\
      Microsoft\CurrentVersion\Explorer\FileTypesPropertySheetHook", 
      if it exists.  This object replaces whatever old implementation
      the shell has.  This was added originally to Win95 close to 
      shipping so IE 1.0 could override it with a MIME-enabled dialog.
      
        URL.DLL implemented the MIME-enabled dialog originally 
        for IE 1.0, using CLSID {FBF23B41-E3F0-101B-8488-00AA003E56F8}
        (the "FileTypes Hook" CLSID).
        
          The NT shell followed in the footsteps of the original Nashville 
          shell code, which made the shell-implemented page accessible via 
          the CLSID {B091E540-83E3-11CF-A713-0020AFD79762}, (the 
          "FileTypes" CLSID) and included the MIME-enabled changes that 
          were in URL.DLL.  This allowed us to move the File Types 
          dialog from explorer.exe to shell32, and each view is 
          responsible to add it to the View/Options page (notice file-
          oriented folders include it, while the Control Panel does not
          now).
          
            Since URL.DLL is an ansi DLL, and there are bugs wrt ansi-unicode 
            conversion in it, the IE 3.0 version (when running on NT) of 
            URL.DLL's FileTypes Hook CLSID simply bound to the FileTypes 
            CLSID implemented in shell32.  On Win95, URL.DLL used its 
            own implementaion.
            
              IE4
              ---------
              
                The filetype dialog code is moved to shdocvw, so there is one 
                binary base for this dialog in both browser-only and 
                integrated-shell installations.  The same code is accessed 
                thru two different CLSIDs (the FileTypes Hook CLSID and the 
                FileTypes cLSID).
                
                  The FileTypes hook mechanism is removed from explorer/shell32
                  on the full-shell install.  So in this case, the page is 
                  accessed via the FileTypes CLSID.
                  
                    On the browser-only install, the page is accessed via the 
                    FileTypes Hook CLSID.  The NT version no longer forwards to
                    NT shell's implementation because we build a unicode version
                    of this page as well.
                    
*/
STDAPI CFileTypes_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppvOut)
{
    HRESULT hres;   
    CFileTypes * pft = new CFileTypes;
    if (pft)
    {
        DllAddRef(); // DllRelease is called when object is destroyed
        
        pft->cRef = 1;
        
        hres = pft->QueryInterface(riid, ppvOut);
        pft->Release();
        
        return hres;
    }
    
    *ppvOut = NULL;
    return E_OUTOFMEMORY;
}

#endif // NO_FILETYPES
