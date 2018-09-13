
/*----------------------------------------------------------
Purpose: Unix-menus related functions.

*/

#include <tchar.h>
#include <process.h>
#include <ieverp.h>

#include "priv.h"
#include "unixstuff.h"
#include "resource.h"
#include "dochost.h"
#include "winlist.h"
#include "runonnt.h"
#include "impexp.h"

#include "mluisupp.h"

#define WINDOWS_MENU_ARROW_WIDTH  4
#define WINDOWS_MENU_ARROW_HEIGHT 8
#define MOTIF_ARROW_TEXT_MARGIN   4

// From download.cpp
#define TITLE_LEN    (256 + 32)
#define MAX_DISPLAY_LEN 96

#ifdef NO_MARSHALLING
extern DWORD g_TLSThreadWindowInfo;
#endif

const TCHAR c_szExploreClass[] = TEXT("ExploreWClass");
const TCHAR c_szIExploreClass[] = TEXT("IEFrame");
const TCHAR c_szCabinetClass[] = 
#ifdef IE3CLASSNAME
    TEXT("IEFrame");
#else
    TEXT("CabinetWClass");
#endif


extern HMENU g_hmenuFullSB;

extern "C" {

    /* Common controls */
    void UnixPaintArrow(HDC hDC, BOOL bHoriz, BOOL bDown, int nXCenter, int nYCenter, int nWidth, int nHeight);

    /* mainwin */
    void MwDrawMotifPopupArrowParams(BOOL bDrawAsDown, HDC hDC, int nShadowThickness, DWORD cButtonSunny, 
                                     DWORD cButtonShadow, DWORD cSelect, DWORD cBackground, const RECT* rItemSize, 
                                     int nIndicatorSize);

    void MwPaintMotifPushButtonGadgetUpDown(HDC hDC, int nButtonWidth, int nButtonHeight, BOOL bFocus, 
                                            BOOL bDrawDefaultPB, BOOL bDown, BOOL bDrawTopBottomShadowOnly,
                                            BOOL bMenu,	 void* /* NULL */, int nOffsetx, int nOffsety);

    BOOL MwGetMotifPullDownMenuResourcesExport(int *lpnShadowThickness,	int *lpnMarginHeight, int *lpnMarginWidth,
                                               int *lpnButtonSpacing, DWORD *lpcBackground, DWORD *lpcForeground,
                                               DWORD *lpcButtonSunny, DWORD *lpcButtonShadow, HFONT * phFont);

    COLORREF MwGetMotifColor(int index);

};

static void
GetMotifMenuParams(int* pnShadowThickness, DWORD* pcBackground, DWORD* pcForeground, 
                    DWORD* pcButtonSunny, DWORD* pcButtonShadow, DWORD* pcSelect, 
                    int* pnIndicatorSize)
{
    const int nIndicatorSize = 12;
    const int MR_PULLDOWNMENU_CHECKBOX_TOGGLEBUTTON_SELECTCOLOR=167;

    MwGetMotifPullDownMenuResourcesExport(pnShadowThickness, NULL, NULL, NULL, pcBackground, pcForeground,
                                          pcButtonSunny, pcButtonShadow, NULL);          
   
    if (pcSelect != NULL)
        *pcSelect = MwGetMotifColor(MR_PULLDOWNMENU_CHECKBOX_TOGGLEBUTTON_SELECTCOLOR);

    if (pnIndicatorSize != NULL)
    	*pnIndicatorSize = nIndicatorSize;
}

EXTERN_C void SelectMotifMenu(HDC hdc, const RECT* prc, BOOL bSelected)
{
    MwPaintMotifPushButtonGadgetUpDown(hdc, prc->right - prc->left, prc->bottom - prc->top,
                                       FALSE, FALSE, bSelected, FALSE, TRUE, NULL,
                                       prc->left, prc->top);
}

EXTERN_C void PaintMotifMenuArrow(HDC hdc, const RECT* prc, BOOL bSelected)
{
    int nShadowThickness, nIndicatorSize;
    DWORD cBackground, cForeground, cButtonSunny, cButtonShadow, cSelect;
    RECT rect = *prc;
    
    GetMotifMenuParams(&nShadowThickness, &cBackground, &cForeground, &cButtonSunny, 
                       &cButtonShadow, &cSelect, &nIndicatorSize);

    // Compensation for space added in the indicatorSize.
    rect.left += MOTIF_ARROW_TEXT_MARGIN;

    MwDrawMotifPopupArrowParams(bSelected, hdc, nShadowThickness, cButtonSunny, cButtonShadow,
                                cSelect, cBackground, &rect, nIndicatorSize);
}

EXTERN_C void PaintWindowsMenuArrow(HDC hdc, const RECT* prc)
{
     int cy = prc->bottom - prc->top;
     UnixPaintArrow( hdc, TRUE, TRUE, prc->left, prc->top + cy/2, 
         WINDOWS_MENU_ARROW_WIDTH, WINDOWS_MENU_ARROW_HEIGHT );
}

EXTERN_C void PaintUnixMenuArrow(HDC hdc, const RECT* prc, DWORD data)
{ 
    if (MwCurrentLook() == LOOK_MOTIF) {
         COLORREF rgbText = (COLORREF) data;
         BOOL bSelected = (rgbText == GetSysColor(COLOR_HIGHLIGHTTEXT));

         PaintMotifMenuArrow(hdc, prc, bSelected);
     }
     else {
         PaintWindowsMenuArrow( hdc, prc );
     }
}

EXTERN_C int GetUnixMenuArrowWidth()
{
    if (MwCurrentLook() == LOOK_MOTIF)
    {
        int nShadowThickness, nIndicatorSize;
        DWORD cBackground, cForeground, cButtonSunny, cButtonShadow, cSelect;

        GetMotifMenuParams(&nShadowThickness, &cBackground, &cForeground, &cButtonSunny,
                       &cButtonShadow, &cSelect, &nIndicatorSize);

        // Indicator Size increased for space between
        return (nIndicatorSize + MOTIF_ARROW_TEXT_MARGIN);
    }
    else
    {
        return ( WINDOWS_MENU_ARROW_WIDTH );
    }
}

EXTERN_C BOOL CheckAndExecNewsScript( HWND hwnd )
{
     HKEY hkey;
     DWORD dwType;
     DWORD dwSize = sizeof(DWORD);
     DWORD dwUseOENews = FALSE;
     HRESULT hres = S_FALSE;
     BOOL  fRet = FALSE;
     TCHAR tszCommandLine[ INTERNET_MAX_URL_LENGTH + 1];
     TCHAR tszExpandedCommand[INTERNET_MAX_URL_LENGTH + 1];

#ifndef DISABLE_OE

     SHGetValue(IE_USE_OE_NEWS_HKEY, IE_USE_OE_NEWS_KEY, IE_USE_OE_NEWS_VALUE,
           &dwType, (void*)&dwUseOENews, &dwSize);

     if (dwType != REG_DWORD)
     {
         // The default value for mail is FALSE
         dwUseOENews = FALSE;
     }

#endif

     if (! dwUseOENews )
     {
         hres = RegOpenKeyEx(
            HKEY_CURRENT_USER,
            REGSTR_PATH_NEWSCLIENTS,
            0,
            KEY_READ,
            &hkey);

         if (hres == ERROR_SUCCESS)
         {
             dwSize = sizeof( tszExpandedCommand );
             hres = RegQueryValueEx(hkey, REGSTR_PATH_CURRENT, NULL, NULL,
                   (LPBYTE)tszCommandLine, &dwSize);

             if (hres == ERROR_SUCCESS)
             {
                  STARTUPINFO stInfo;
                  memset(&stInfo, 0, sizeof(stInfo));
                  stInfo.cb = sizeof(stInfo);
                  SHExpandEnvironmentStrings(tszCommandLine, tszExpandedCommand,
                      INTERNET_MAX_URL_LENGTH);
                  if( CreateProcess(tszExpandedCommand, tszCommandLine, NULL, NULL,
                        FALSE, DETACHED_PROCESS, NULL,
                        NULL, &stInfo, NULL) )
                      fRet = TRUE;
             }
           
             RegCloseKey( hkey );
         }
     }

     if( !dwUseOENews && !fRet )
     {
         ShellMessageBox(
             MLGetHinst(),
             hwnd,
             MAKEINTRESOURCE(IDS_NEWS_SCRIPT_ERROR),
             MAKEINTRESOURCE(IDS_NEWS_SCRIPT_ERROR_TITLE),
             MB_ICONWARNING|MB_OK);
         fRet = TRUE;
     }

     return fRet;
}

EXTERN_C int  ConvertModuleNameToUnix( LPTSTR path )
{
    TCHAR drive[_MAX_DRIVE];   
    TCHAR dir[_MAX_DIR];
    TCHAR fname[_MAX_FNAME];   
    TCHAR ext[_MAX_EXT];

    if( !path || !*path )
        return 0;

#ifndef UNICODE
    _splitpath( path, drive, dir, fname, ext );
#else
    _wsplitpath( path, drive, dir, fname, ext );
#endif

    if(StrCmpNI( fname, TEXT("lib"), 3 ))
       StrCpyN( path, fname, _MAX_FNAME );
    else
       StrCpyN( path, fname+3, (_MAX_FNAME-3) );

    if(StrCmpNI( ext, TEXT(".so"), 3 ))
       StrCatBuff( path, ext, _MAX_EXT );
    else
       StrCatBuff( path, TEXT(".dll"), _MAX_EXT );

    return lstrlen( path );
}

void ContentHelp( IShellBrowser* psb )
{
    UnixHelp("Content Index", psb);
}

EXTERN_C void UnixHelp(LPWSTR pszName, IShellBrowser* psb )
{
    WCHAR szPathTemp[MAX_URL_STRING+1];
    
    HRESULT hres = E_FAIL;
    UINT  cbTempSize = ARRAYSIZE(szPathTemp);

    // Get the path for installed pages from registry. This entry
    // in registry is modified by the setup   program to point to
    // location where help files are installed.

    hres = URLSubRegQuery(L"Software\\Microsoft\\Internet Explorer\\Unix",
           pszName, TRUE, szPathTemp, cbTempSize, URLSUB_ALL);

    if( FAILED(hres) ) return;

    if(psb)
    {
        LPITEMIDLIST searchPidl;

        // searchPidl = SHSimpleIDListFromPath( szPathTemp );

        ::IEParseDisplayName(CP_ACP, szPathTemp, &searchPidl );
        if(searchPidl)
        {
            psb->BrowseObject(searchPidl,SBSP_NEWBROWSER|SBSP_HELPMODE|SBSP_NOTRANSFERHIST);
            ILFree( searchPidl );
        }
    }
}


EXTERN_C BOOL  OEHandlesMail( void )
{
#ifdef DISABLE_OE
    return FALSE;
#endif

    DWORD dwType;
    DWORD dwSize;
    DWORD dwUseOEMail;
    HRESULT hr;

    dwSize = sizeof(DWORD);

    hr = SHGetValue(IE_USE_OE_MAIL_HKEY, IE_USE_OE_MAIL_KEY, IE_USE_OE_MAIL_VALUE, &dwType, (void*)&dwUseOEMail, &dwSize);
    if ((hr) && (dwType != REG_DWORD))
    {
        // The default value for mail is FALSE
        dwUseOEMail = FALSE;
    }

    if (dwUseOEMail)
    {
        return TRUE; 
    }
    else
    {
        return FALSE;
    }
}


// PORT QSY SendDocToMailRecipient has already been used to
//	invoke native UNIX mailers. Invent a new function to
//	let OE do the job.
//	Most of the code copied from WAB and  shell/ext/sendmail/sendmail.c
//	We probably should port sendmail.dll after preview 1.
#include <mapi.h>

#define MRPARAM_DOC          0x00000001
#define MRPARAM_DELETEFILE   0x00000002 // The temporary file was created by us and need to be deleted
#define MRPARAM_USECODEPAGE  0x00000004
typedef struct {
    DWORD dwFlags;    // Attributes passed to the MAPI apis
    LPTSTR pszFiles;  // All files names separated by NULL;
    LPTSTR pszURL;    // All files names of URLs separated by ';', this will
                      // be the title;
    CHAR paszFiles[MAX_PATH + 1];
    CHAR paszURL[INTERNET_MAX_URL_LENGTH + 1];
                      // be the title;
    int nFiles;       // The number of files being sent.
    UINT uiCodePage;  // Code page
} MRPARAM;

const LPTSTR szDefMailKey = TEXT("Software\\Clients\\Mail");
const LPTSTR szOEDllPathKey =  TEXT("DllPath");
const LPTSTR szOEName = TEXT("Outlook Express");

BOOL CheckForOutlookExpress(LPTSTR szDllPath)
{
#ifdef DISABLE_OE
    return FALSE;
#endif

    HKEY hKeyMail   = NULL;
    HKEY hKeyOE     = NULL;
    DWORD dwErr     = 0;
    DWORD dwSize    = 0;
    TCHAR szBuf[MAX_PATH];
    DWORD dwType    = 0;
    BOOL bRet = FALSE;


    StrCpyN(szDllPath, TEXT(""), 1);

    // Open the key for default internet mail client
    // HKLM\Software\Clients\Mail

    dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                szDefMailKey,
                                0,
                                KEY_ALL_ACCESS,
                                &hKeyMail);
    if(dwErr != ERROR_SUCCESS)
    {
        goto out;
    }

    dwSize = sizeof(szBuf);         // Expect ERROR_MORE_DATA

    dwErr = RegQueryValueEx(
                                hKeyMail,
                                NULL,
                                NULL,
                                &dwType,
                                (LPBYTE)szBuf,
                                &dwSize);
    if(dwErr != ERROR_SUCCESS)
    {
        goto out;
    }

    if(!StrCmpI(szBuf, szOEName))
    {
        // Yes its outlook express ..
        bRet = TRUE;

        // Get the DLL Path
        dwErr = RegOpenKeyEx(hKeyMail,
                                    szOEName,
                                    0,
                                    KEY_ALL_ACCESS,
                                    &hKeyOE);
        if(dwErr != ERROR_SUCCESS)
        {
            goto out;
        }

        dwSize = sizeof(szBuf);
        StrCpyN(szBuf, TEXT(""), 1);

        dwErr = RegQueryValueEx(
                                hKeyOE,
                                szOEDllPathKey,
                                NULL,
                                &dwType,
                                (LPBYTE)szBuf,
                                &dwSize);

        if(dwErr != ERROR_SUCCESS)
        {
            goto out;
        }

        if(lstrlen(szBuf))
            StrCpyN(szDllPath, szBuf, lstrlen(szBuf) + 1);
    }

out:
    if(hKeyOE)
        RegCloseKey(hKeyOE);
    if(hKeyMail)
        RegCloseKey(hKeyMail);

    return bRet;

}

HANDLE LoadMailProvider()
{
    TCHAR szBuf[MAX_PATH];
    HANDLE hLibMapi = NULL;

    if(CheckForOutlookExpress(szBuf) == TRUE)
    {
        hLibMapi = LoadLibrary(szBuf);
	return hLibMapi;
    }
    else
        return 0;
}

BOOL AllocatePMP(MRPARAM * pmp, DWORD cchUrlBufferSize, DWORD cchFileBufferSize)
{
    pmp->pszURL = (LPTSTR) GlobalAlloc(GPTR, cchUrlBufferSize * sizeof(TCHAR));
    if (!pmp->pszURL)
        return FALSE;
 
    pmp->pszFiles = (LPTSTR) GlobalAlloc(GPTR, cchFileBufferSize * sizeof(TCHAR));
    if (!pmp->pszFiles)
        return FALSE;
 
    return TRUE;
}

BOOL CleanupPMP(MRPARAM * pmp)
{
    if (pmp->pszFiles)
        GlobalFree((LPVOID)pmp->pszFiles);
    pmp->pszFiles = NULL;
    if (pmp->pszURL)
        GlobalFree((LPVOID)pmp->pszURL);
    pmp->pszURL = NULL;
    pmp->nFiles = 0;
    GlobalFree(pmp);
    return TRUE;
}

HRESULT _CreateShortcutToPath(LPCTSTR pszPath, LPCTSTR pszTarget)
{
    IShellLink *psl;
    HRESULT hres = CoCreateInstance(CLSID_ShellLink, 
		NULL, 
		CLSCTX_INPROC_SERVER, 
		IID_IShellLink, 
		(LPVOID*)&psl);

    if (SUCCEEDED(hres))
    {
        IPersistFile *ppf;

        psl->SetPath(pszTarget);

        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID *)&ppf);
        if (SUCCEEDED(hres))
        {
            WCHAR wszPath[MAX_PATH];
#ifndef UNICODE
            AnsiToUnicode(pszPath, wszPath, ARRAYSIZE(wszPath));
#else
	    StrCpyN(wszPath, pszPath, MAX_PATH);
#endif
 
            hres = ppf->Save(wszPath, TRUE);
            ppf->Release();
        }
        psl->Release();
    }
    return hres;
}

// create a temporary shortcut to a file
// BUGBUG: Colision is not handled here
BOOL _CreateTempFileShortcut(LPCTSTR IN pszFile, LPTSTR OUT pszShortcut)
{
    TCHAR szPath[MAX_PATH];
    
    if (GetTempPath(ARRAYSIZE(szPath), szPath))
    {
        StrCatBuff(szPath, PathFindFileName(pszFile), lstrlen(pszFile) + 1);
        PathRenameExtension(szPath, TEXT(".lnk"));

        if (SUCCEEDED(_CreateShortcutToPath(szPath, pszFile)))
        {
            StrCpyN(pszShortcut, szPath, MAX_PATH);
            return TRUE;
        }
    }
    return FALSE;
} 

HRESULT CopyHtmlFile(HANDLE hOrig, HANDLE hNew, DWORD dwSize)
{
    BYTE Buff[4096];
    int iBlock, nBlocks = dwSize / SIZEOF(Buff);
 
    for (iBlock = 0; iBlock <= nBlocks; iBlock++)
    {
        DWORD dwRead, dwWrite;
 
        ReadFile(hOrig, Buff, SIZEOF(Buff), &dwRead, NULL);
        if ((iBlock < nBlocks) && (dwRead != SIZEOF(Buff)))
            return E_FAIL;
        WriteFile(hNew, Buff, dwRead, &dwWrite, NULL);
        if (dwWrite != dwRead)
            return E_FAIL;
    }
 
    return S_OK;
}

typedef BOOL (RETRIEVE_URL_CACHE_ENTRY_FILEA) (
	LPCSTR lpszUrlName,
	LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
	LPDWORD lpdwCacheEntryInfoBufferSize,
	DWORD dwReserved
);
typedef RETRIEVE_URL_CACHE_ENTRY_FILEA FAR *LP_RETRIEVE_URL_CACHE_ENTRY_FILEA;
	
typedef BOOL (RETRIEVE_URL_CACHE_ENTRY_FILEW) (
        LPCWSTR lpszUrlName,
        LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
        LPDWORD lpdwCacheEntryInfoBufferSize,
        DWORD dwReserved
);
typedef RETRIEVE_URL_CACHE_ENTRY_FILEW *LP_RETRIEVE_URL_CACHE_ENTRY_FILEW;

HRESULT CreateHtmlFileFromUrl(LPCTSTR pszURL, LPCTSTR pszFile)
{
    DWORD dwSize;
    HRESULT hr = E_FAIL;
    INTERNET_CACHE_ENTRY_INFO *picei=NULL;
    #ifdef UNICODE
    LP_RETRIEVE_URL_CACHE_ENTRY_FILEW lpfnRetrieveUrlCacheEntryFile;
    #else
    LP_RETRIEVE_URL_CACHE_ENTRY_FILEA lpfnRetrieveUrlCacheEntryFile;
    #endif

    HINSTANCE hLibWininet = LoadLibrary(TEXT("wininet.dll"));
 
    if (!hLibWininet)
        return E_FAIL;

    #ifdef UNICODE
    lpfnRetrieveUrlCacheEntryFile = 
	(LP_RETRIEVE_URL_CACHE_ENTRY_FILEW) GetProcAddress(hLibWininet, "RetrieveUrlCacheEntryFileW");
    #else
    lpfnRetrieveUrlCacheEntryFile = 
	(LP_RETRIEVE_URL_CACHE_ENTRY_FILEA)GetProcAddress(hLibWininet, "RetrieveUrlCacheEntryFileA");
    #endif

    if (!lpfnRetrieveUrlCacheEntryFile)
	return E_FAIL;
 
  
    // Find size of Cache Entry File struct for this url. Call fails with
    // error insufficient buffer and puts size in dwSize. Any other failure
    // value implies the item isn't in the cache.
    dwSize = 0;
    if (!lpfnRetrieveUrlCacheEntryFile(pszURL, picei, &dwSize, 0))
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            picei = (INTERNET_CACHE_ENTRY_INFO *) GlobalAlloc(GPTR,  dwSize);
    }
    
    if (picei)
    {
        if (lpfnRetrieveUrlCacheEntryFile(pszURL, picei, &dwSize, 0))
        {
            HANDLE hOrigFile = CreateFile(picei->lpszLocalFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                                          OPEN_EXISTING, 0, NULL);
            if (hOrigFile != INVALID_HANDLE_VALUE)
            {
                HANDLE hNewFile = CreateFile(pszFile, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                                             CREATE_ALWAYS, 0, NULL);
                if (hNewFile != INVALID_HANDLE_VALUE)
                {
                    DWORD dwWritten;

                    TCHAR szBaseTag[INTERNET_MAX_URL_LENGTH + 20];
                    wnsprintf(szBaseTag, ARRAYSIZE(szBaseTag), TEXT("<BASE HREF=\"%s\">\n"), pszURL);
                    
                    if (WriteFile(hNewFile, szBaseTag, lstrlen(szBaseTag)*SIZEOF(TCHAR), &dwWritten, NULL))
                        hr = CopyHtmlFile(hOrigFile, hNewFile, picei->dwSizeLow);
                    CloseHandle(hNewFile);
                }
                CloseHandle(hOrigFile);
            }
        }
        GlobalFree(picei);
    }
    return hr;
}

extern HRESULT PersistShortcut(IUniformResourceLocator * purl, LPCWSTR pwszFile);

/*
 * pcszURL -> "ftp://ftp.microsoft.com"
 * pcszPath -> "c:\windows\desktop\internet\Microsoft FTP.url"
 */
HRESULT CreateNewURLShortcut(LPCTSTR pcszURL, LPCTSTR pcszURLFile)
{
    HRESULT hr;
    WCHAR wszFile[INTERNET_MAX_URL_LENGTH];

#ifndef UNICODE
    if (AnsiToUnicode(pcszURLFile,
                      wszFile, ARRAYSIZE(wszFile)) > 0)
#else
    if (StrCpyN(wszFile, pcszURLFile, ARRAYSIZE(wszFile)) > 0)
#endif
    {
        IUniformResourceLocator *purl;

        hr = CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER,
                              IID_IUniformResourceLocator, (LPVOID*)&purl);

        if (SUCCEEDED(hr))
        {
            hr = purl->SetURL( pcszURL, 0);
            if (SUCCEEDED(hr))
                hr = PersistShortcut(purl, wszFile);
            purl->Release();
        }
    }
    else
        hr = E_FAIL;

    return(hr);
}

HRESULT _GetFileDescription(IDataObject * pdtobj, LPTSTR pszDescription)
{
    STGMEDIUM medium;
    HRESULT hres;
    FORMATETC fmteURL = {0, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    UINT g_cfFileDesc;

    InitClipboardFormats();

    #ifdef UNICODE
    g_cfFileDesc = g_cfFileDescW;
    #else
    g_cfFileDesc = g_cfFileDescA;
    #endif
    
    fmteURL.cfFormat = g_cfFileDesc;

    hres = E_FAIL;
    if (SUCCEEDED(pdtobj->GetData(&fmteURL, &medium)))
    {
#ifdef UNICODE
        FILEGROUPDESCRIPTORW * pfgd = (FILEGROUPDESCRIPTORW *)GlobalLock(medium.hGlobal);
#else   
        FILEGROUPDESCRIPTORA * pfgd = (FILEGROUPDESCRIPTORA *)GlobalLock(medium.hGlobal);
#endif
        if (pfgd)
        {
#ifdef UNICODE
            FILEDESCRIPTORW * pfd = &(pfgd->fgd[0]);
#else           
            FILEDESCRIPTORA * pfd = &(pfgd->fgd[0]);
#endif                  

            StrCpyN(pszDescription, (LPTSTR)pfd->cFileName, lstrlen((LPTSTR)pfd->cFileName) + 1);
            GlobalUnlock(medium.hGlobal);
            PathRemoveExtension(pszDescription);
            hres = S_OK;
        }
    }
    ReleaseStgMedium(&medium);
    return hres;
}       

#define STR_SENDMAIL_URL_FILENAME     TEXT("Send Mail Message.url")
#define STR_SENDMAIL_HTM_FILENAME      TEXT("Send Mail Message.htm")
#define STR_SENDMAIL_URL_SHORTFILENAME TEXT("SendMail.url")
#define STR_SENDMAIL_HTM_SHORTFILENAME TEXT("SendMaihtm")
#define STR_SENDMAIL_FAILUREMSG        TEXT("The current document is not available. Would  you like to send the URL instead?")
#define STR_SENDMAIL_FAILUREMSGTITLE   TEXT("SendMail Failed")

HRESULT _CreateFileToSend(IDataObject *pdtobj, MRPARAM * pmp, DWORD grfKeyState)
{
    HRESULT hr;
    BOOL bCreateUrl = FALSE;

    hr = E_FAIL;
    if (grfKeyState != FORCE_LINK)
    {
        hr = CreateHtmlFileFromUrl(pmp->pszURL, pmp->pszFiles);
        if (FAILED(hr))
        {
            int iRet;
                        
            iRet = ShellMessageBox(MLGetHinst(),
                                   NULL,
                                   STR_SENDMAIL_FAILUREMSG,
                                   STR_SENDMAIL_FAILUREMSGTITLE,
                                   MB_YESNO);
            if (iRet == IDYES)
            {
                PathRenameExtension(pmp->pszFiles, TEXT(".url"));
                PathYetAnotherMakeUniqueName(pmp->pszFiles, pmp->pszFiles, pmp->pszFiles, pmp->pszFiles);      
                bCreateUrl = TRUE;
            }
        }
        else
        {
            // We have succeeded in creating the HTML DOC to send
            IQueryCodePage * pQcp;

            // First set the flag for sending a doc
            pmp->dwFlags |= MRPARAM_DOC;

            // Then get the code page if there is one
            if (SUCCEEDED(pdtobj->QueryInterface(IID_IQueryCodePage, (LPVOID *)&pQcp)))
            {
                if (SUCCEEDED(pQcp->GetCodePage( &pmp->uiCodePage)))
                    pmp->dwFlags |= MRPARAM_USECODEPAGE;
                pQcp->Release();
            }
        }
    }
    else
        bCreateUrl = TRUE;
    
    if (bCreateUrl)
        hr = CreateNewURLShortcut(pmp->pszURL, pmp->pszFiles);

    pmp->dwFlags |= MRPARAM_DELETEFILE;

    // Get the title of this homepage
    if (SUCCEEDED(hr))
        _GetFileDescription(pdtobj, pmp->pszURL);
    
    return hr;
}

HRESULT _GetFilesFromDataObj(IDataObject *pdtobj, DWORD grfKeyState, MRPARAM * pmp)
{
    HRESULT hr = E_FAIL;
    STGMEDIUM medium;
    FORMATETC fmteHDROP = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    FORMATETC fmteURL = {g_cfURL, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    
    if (SUCCEEDED(pdtobj->GetData(&fmteHDROP, &medium)))
    {
        TCHAR szPath[MAX_PATH];

        DWORD cchFileBufferSize = 0;   // This will be the size of the buffer that holds all files names
        DWORD cchUrlBufferSize = 0;

        for (pmp->nFiles = 0; DragQueryFile((HDROP)medium.hGlobal, pmp->nFiles, szPath, ARRAYSIZE(szPath)); pmp->nFiles++)         
            cchFileBufferSize += (lstrlen(szPath) + 1) * SIZEOF(TCHAR);
    
        cchUrlBufferSize += MAX_PATH * SIZEOF(TCHAR) * pmp->nFiles;

        if (cchFileBufferSize && AllocatePMP(pmp, cchFileBufferSize, cchUrlBufferSize))
        {
            int i;
            LPTSTR psz, pszNames;
            for (i = 0, psz = pmp->pszFiles, pszNames = pmp->pszURL; DragQueryFile((HDROP)medium.hGlobal, i, szPath, ARRAYSIZE(szPath)); i++)            {
                // This is used to separate the names
                if (pszNames != pmp->pszURL)
                    *pszNames++ = TEXT(';');
                
                if (grfKeyState == FORCE_LINK)
                {
                    // Want to send a link even for the real file, we will create links to the real files
                    // and send it.
                    TCHAR szShortCut[MAX_PATH];
                    if (_CreateTempFileShortcut(szPath, szShortCut))
                        StrCpyN(psz, szShortCut, MAX_PATH);
                    pmp->dwFlags |= MRPARAM_DELETEFILE;
                }
                else
                    StrCpyN(psz, szPath, MAX_PATH);
                psz += lstrlen(psz) + 1;
                
                StrCpyN(pszNames, szPath, MAX_PATH);
                pszNames += lstrlen(szPath);
            }
            hr = S_OK;
        }
    }
    else if (SUCCEEDED(pdtobj->GetData( &fmteURL, &medium)))
    {
        // This DataObj is from the internet -----------------
        // NOTE: We only allow to send one file here.
        pmp->nFiles = 1;
        if (AllocatePMP(pmp, max(SIZEOF(TCHAR)*GlobalSize(medium.hGlobal), MAX_PATH)
						, MAX_PATH))
        {
            if ((fmteURL.cfFormat == g_cfURL) && (TYMED_HGLOBAL == medium.tymed))
            {

#ifdef UNICODE
                // medium.hGlobal is an ansi string
                LPSTR pchBuf = NULL;
                int cchLen = strlen((LPSTR)GlobalLock(medium.hGlobal)) + 1;
                int cchBufLen = max(SIZEOF(TCHAR)*GlobalSize(medium.hGlobal), MAX_PATH);
                if (!(pchBuf = (LPSTR)GlobalAlloc(GPTR, max(SIZEOF(TCHAR)*GlobalSize(medium.hGlobal), MAX_PATH) * sizeof(CHAR))))
                	return E_FAIL;

                strncpy(pchBuf, (LPSTR)GlobalLock(medium.hGlobal), cchLen);
                AnsiToUnicode(pchBuf, pmp->pszURL, cchBufLen);
                GlobalFree(pchBuf);
#else
                StrCpyN(pmp->pszURL, (LPTSTR)GlobalLock(medium.hGlobal), lstrlen((LPTSTR)GlobalLock(medium.hGlobal)) + 1);
#endif
                GlobalUnlock(medium.hGlobal);
            }
	
            if (pmp->pszURL[0])
            {
                // Note some of these functions depend on which OS we
                // are running on to know if we should pass ansi or unicode strings
                // to it
                // Windows 95
                if (GetTempPath(MAX_PATH, pmp->pszFiles))
                {
                    TCHAR szFileName[MAX_PATH];
                    TCHAR szShortFileName[20];

                    StrCpyN(szFileName, (grfKeyState == FORCE_LINK) ? STR_SENDMAIL_URL_FILENAME : STR_SENDMAIL_HTM_FILENAME, MAX_PATH);
                    StrCpyN(szShortFileName, (grfKeyState == FORCE_LINK) ? STR_SENDMAIL_URL_SHORTFILENAME : STR_SENDMAIL_HTM_SHORTFILENAME, 20);

                    if (PathYetAnotherMakeUniqueName(pmp->pszFiles, pmp->pszFiles, szShortFileName, szFileName)) 
                        hr = _CreateFileToSend(pdtobj, pmp, grfKeyState);
                }
                
            }
            
        }
    }   

    ReleaseStgMedium(&medium);
    return hr;
}

typedef struct {
    CHAR szTempShortcut[MAX_PATH];
    WCHAR szTempShortcutW[MAX_PATH];
    MapiMessage mm;
    MapiFileDesc mfd[1];
} MAPI_FILES;

void _FreeMapiFiles(MAPI_FILES *pmf)
{
    if (pmf->szTempShortcut[0])
        DeleteFile(pmf->szTempShortcutW);
    GlobalFree(pmf);
}

MAPI_FILES *_AllocMapiFiles(int nFiles, LPCTSTR pszFiles, MRPARAM *pmp)
{
    MAPI_FILES *pmf;

    pmf = (MAPI_FILES *)GlobalAlloc(GPTR, SIZEOF(*pmf) + (nFiles * SIZEOF(pmf->mfd[0])));
    if (pmf)
    {
        int i;

        pmf->mm.nFileCount = nFiles;
        if (nFiles)
        {
            LPCTSTR psz;

            pmf->mm.lpFiles = pmf->mfd;

            for (i = 0, psz = pszFiles; ((i < nFiles) && (*psz)); psz += lstrlen(psz) + 1, i++)
            {
                // if the first item is a folder, we will create a shortcut to
                // that instead of trying to mail the folder (that MAPI does
                // not support)

                if ((i == 0) && PathIsDirectory(pszFiles) &&
#ifndef UNICODE
                    _CreateTempFileShortcut(pszFiles, pmf->szTempShortcut))
#else
                    _CreateTempFileShortcut(pszFiles, pmf->szTempShortcutW))
#endif
                {
#ifdef UNICODE
		    WCHAR wszTemp;
		    WideCharToMultiByte(CP_ACP, 0, pmf->szTempShortcutW, -1, 
					pmf->szTempShortcut, MAX_PATH, 
					NULL, NULL);
#endif
                    pmf->mfd[i].lpszPathName = pmf->szTempShortcut;
#ifndef UNICODE
                    pmf->mfd[i].lpszFileName = PathFindFileName(pszFiles);
#else
		    WideCharToMultiByte(CP_ACP, 0, pszFiles, -1, 
					pmp->paszFiles, MAX_PATH, 
					NULL, NULL);
                    pmf->mfd[i].lpszFileName = PathFindFileNameA(pmp->paszFiles);
#endif
                }
                else
                {
#ifndef UNICODE
                    pmf->mfd[i].lpszPathName = (LPTSTR)psz; // const -> non const
                    pmf->mfd[i].lpszFileName = PathFindFileName(psz);
#else
		    WideCharToMultiByte(CP_ACP, 0, psz, -1, 
					pmp->paszFiles, MAX_PATH, 
					NULL, NULL);
                    pmf->mfd[i].lpszPathName = pmp->paszFiles; // const -> non const
                    pmf->mfd[i].lpszFileName = PathFindFileNameA(pmp->paszFiles);
#endif
                }
                pmf->mfd[i].nPosition = (UINT)-1;
            }
        }
    }
    return pmf;
}

BOOL _IsShortcut(LPCTSTR pszFile)
{
    SHFILEINFO sfi;
    return SHGetFileInfo(pszFile, 0, &sfi, sizeof(sfi), SHGFI_ATTRIBUTES) && (sfi.dwAttributes & SFGAO_LINK);
}


// get the target of a shortcut. this uses IShellLink which 
// Internet Shortcuts (.URL) and Shell Shortcuts (.LNK) support so
// it should work generally
//
BOOL _GetShortcutTarget(LPCTSTR pszPath, LPTSTR pszTarget, UINT cch)
{
    IUnknown *psl;
    CLSID clsid;

    *pszTarget = 0;     // assume none

    if (!_IsShortcut(pszPath))
        return FALSE;

    // BUGBUG: we really should call GetClassFile() but this could
    // slow this down a lot... so chicken out and just look in the registry

    if (FAILED(_CLSIDFromExtension(PathFindExtension(pszPath), &clsid)))
        clsid = CLSID_ShellLink;        // assume it's a shell link

    if (SUCCEEDED(CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (LPVOID *)&psl)))
    {
        IPersistFile *ppf;

        if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (LPVOID *)&ppf)))
        {
#ifdef UNICODE
            ppf->Load(pszPath, 0);
#else
            WCHAR wszPath[MAX_PATH];
            AnsiToUnicode(pszPath, wszPath, ARRAYSIZE(wszPath));

            ppf->Load(wszPath, 0);
#endif
            ppf->Release();
        }
        IShellLink_GetPath(psl, pszTarget, cch, SLGP_UNCPRIORITY);
        psl->Release();

        return TRUE;
    }

    return FALSE;
}

void _DeleteMultipleFiles(LPCTSTR pszFiles)
{
    int i;
    LPCTSTR psz;
    for (i = 0, psz = pszFiles; *psz; psz += lstrlen(psz) + 1, i++)
        DeleteFile(psz);
}

const TCHAR c_szPad[] = TEXT(" \r\n ");

STDAPI_(DWORD) MailRecipientThreadProc(void *pmp)
{
    TCHAR szText[2148];     // hold a URL/FilePath + some formatting text
#ifdef UNICODE
    CHAR aszText[2148];     // hold a URL/FilePath + some formatting text
#endif
    MAPI_FILES *pmf;

    DWORD dwFlags = ((MRPARAM *)pmp)->dwFlags;
    LPCTSTR pv = ((MRPARAM *)pmp)->pszFiles;
    LPCTSTR pszTitle = ((MRPARAM *)pmp)->pszURL; 

    CoInitialize(NULL);     // we are going to do some COM stuff

    pmf = _AllocMapiFiles(((MRPARAM *)pmp)->nFiles, pv, (MRPARAM *)pmp);
    if (pmf)
    {
        TCHAR szTitle[80]; // because the title is supposed to be non-const
#ifdef UNICODE
        CHAR aszTitle[80]; // because the title is supposed to be non-const
#endif
        HMODULE hmodMail;

        if (pmf->mm.nFileCount)
        {
            StrCpyN(szText, c_szPad, lstrlen(c_szPad) + 1);    // init the buffer with some stuff
            if (_IsShortcut(pv))
                _GetShortcutTarget(pv, szText + ARRAYSIZE(c_szPad) - 1, ARRAYSIZE(szText) - ARRAYSIZE(c_szPad));
            else
                StrCpyN(szText + ARRAYSIZE(c_szPad) - 1, pszTitle, ARRAYSIZE(szText) - ARRAYSIZE(c_szPad));

            // Don't fill in lpszNoteText if we know we are sending documents because Athena puke on it 
            if (!(dwFlags & MRPARAM_DOC))
	    {
#ifndef UNICODE
                pmf->mm.lpszNoteText = szText;
#else
		WideCharToMultiByte(CP_ACP, 0, szText, -1, 
				    aszText, 2148, 
				    NULL, NULL);
                pmf->mm.lpszNoteText = aszText;
#endif
	    }

            if (pszTitle) {
                StrCpyN(szTitle, pszTitle, ARRAYSIZE(szTitle));
#ifndef UNICODE
                pmf->mm.lpszSubject = szTitle;
#else
		WideCharToMultiByte(CP_ACP, 0, szTitle, -1, 
				    aszTitle, 80, 
				    NULL, NULL);
                pmf->mm.lpszSubject = aszTitle;
#endif
            } else
#ifndef UNICODE
                pmf->mm.lpszSubject = szText + ARRAYSIZE(c_szPad) - 1;
#else
                pmf->mm.lpszSubject = aszText + ARRAYSIZE(c_szPad) - 1;
#endif
        }

        hmodMail = (HINSTANCE) LoadMailProvider();
        if (dwFlags & MRPARAM_USECODEPAGE)
        {
            // When this flag is set, we know that we have just one file to send and we have a code page
            // Athena will then look at ulReserved for the code page
            // BUGBUG: Will the other MAPI handlers puke on this?  -- dli
            ASSERT(pmf->mm.nFileCount == 1);
            pmf->mfd[0].ulReserved = ((MRPARAM *)pmp)->uiCodePage;
        }
        

        if (hmodMail)
        {
            LPMAPISENDMAIL pfnSendMail = (LPMAPISENDMAIL)GetProcAddress(hmodMail, "MAPISendMail");
            if (pfnSendMail)
                pfnSendMail(0, 0, &pmf->mm, MAPI_LOGON_UI | MAPI_DIALOG, 0);

            FreeLibrary(hmodMail);
        }
        _FreeMapiFiles(pmf);
    }
    
    if (pv && (dwFlags & MRPARAM_DELETEFILE))
        _DeleteMultipleFiles(pv);
    
    if (pmp)
        CleanupPMP((MRPARAM *)pmp);

    CoUninitialize();

    // SetCursor(hcurOld);

    return 0;
}

STDAPI MailRecipientDropHandler(IDataObject *pdtobj, DWORD grfKeyState, DWORD dwEffect)
{
    HANDLE hThread;
    DWORD idThread;
    MRPARAM * pmp = (MRPARAM *)GlobalAlloc(GPTR, SIZEOF(*pmp));
    if (pmp)
    {        
	InitClipboardFormats();

        if (!pdtobj || SUCCEEDED(_GetFilesFromDataObj(pdtobj, grfKeyState, pmp)))
        {
		

            hThread = CreateThread(NULL, 0, MailRecipientThreadProc, pmp, 0, &idThread);
            if (hThread)
            {
                CloseHandle(hThread);
                return S_OK;
            }
        }
        
        CleanupPMP(pmp);
 
	return S_OK;
    }
 
    return E_OUTOFMEMORY;       
}

HRESULT _UnixDropOnMailRecipient(IDataObject *pdtobj, DWORD grfKeyState)
{
    HRESULT hres = ERROR_SUCCESS;

    MailRecipientDropHandler(pdtobj, grfKeyState, 0);
    return hres;
}
 

EXTERN_C HRESULT _UnixSendDocToOE(LPCITEMIDLIST pidl, UINT uiCodePage, DWORD grfKeyState)
{
    IDataObject *pdtobj;
    HRESULT hres;

    if (!pidl)
        hres = _UnixDropOnMailRecipient(NULL, grfKeyState);
    else 
    {
        hres = GetDataObjectForPidl(pidl, &pdtobj);
        if (SUCCEEDED(hres))
        {
            IQueryCodePage * pQcp;
            if (SUCCEEDED(pdtobj->QueryInterface(IID_IQueryCodePage, (LPVOID *)&pQcp)))
            {
                pQcp->SetCodePage(uiCodePage);
                pQcp->Release();
            }
            hres = _UnixDropOnMailRecipient(pdtobj, grfKeyState);
            pdtobj->Release();
        }
    }

    return hres;
}
// PORT


EXTERN_C void   UnixAdjustWindowSize( HWND hwnd, IETHREADPARAM* piei )
{
        // IEUNIX - Default window size is too small
        // We come up with this size if there is no saved stream for window
        // placement, in which case piei->wb.length is zero
       char szGeometry[MAX_PATH];

       /* If the geometry was specified on the command line use it */
       if (MwGetXInvocationParam("-geometry", szGeometry, MAX_PATH)) {
          int   wMask      = 0;
          int   wXOffset   = 0;
          int   wYOffset   = 0;
          UINT  wHeight    = 0;
          UINT  wWidth     = 0;
          RECT  rc;        /* info about browser window */
          RECT  rcDisplay; /* to get info about desktop */
         
          /* Mask is mainwin mask */
          wMask = MwProtectedParseXGeometry(szGeometry, 
                                            &wXOffset, &wYOffset,
                                            &wWidth, &wHeight);
 
          GetWindowRect( hwnd, &rc );
          GetWindowRect( GetDesktopWindow(), &rcDisplay );

          if (!(wMask & MwHeightValue))
          {
             if (!(piei->wp.length))
             {
                wHeight = BROWSER_DEFAULT_HEIGHT;
             }
             else
             {
                wHeight = rc.bottom - rc.top;
             }
          }

          if (!(wMask & MwWidthValue))
          {
             if (!(piei->wp.length))
             {
                wWidth = BROWSER_DEFAULT_WIDTH;
             }
             else
             {
                wWidth = rc.right - rc.left;
             }
          }

          if (!(wMask & MwXValue))
          {
             wXOffset = rc.left;
          }
          else
          if (wMask & MwXNegative)
          {
             /* wXOffset will be negative */
             wXOffset = max( 0, rcDisplay.right - rcDisplay.left - wWidth + wXOffset );
          }

          if (!(wMask & MwYValue))
          {
             wYOffset = rc.top;
          }
          else
          if (wMask & MwYNegative)
          {
             /* wYOffset will be negative */
             wYOffset = max( 0, rcDisplay.bottom - rcDisplay.top - wHeight + wYOffset );
          }

          if (!wYOffset)
          {
             wYOffset = 1;
          }

          if (!wXOffset)
          {
             wXOffset = 1;
          }

          SetWindowPos(hwnd, 0, 
                       wXOffset, wYOffset, wWidth, wHeight,
                       SWP_NOZORDER);
       }
       else
       if (piei->wp.length == 0) {
           RECT rc;
           GetWindowRect( hwnd, &rc );
           SetWindowPos ( hwnd, 0, rc.left, rc.top, BROWSER_DEFAULT_WIDTH, BROWSER_DEFAULT_HEIGHT, SWP_NOZORDER );
       }
}

#if 0 //def NO_MARSHALLING
void IEFrame_NewWindow_SameThread(IETHREADPARAM* piei)
{
    DWORD uFlags = piei->uFlags;
    
    LPSTR pszCloseEvent = NULL;
    if (uFlags & COF_FIREEVENTONCLOSE) {
        ASSERT(piei->szCloseEvent[0]);
        pszCloseEvent = StrDup(piei->szCloseEvent);
    }

    THREADWINDOWINFO *lpThreadWindowInfo;
    ASSERT (~0 != g_TLSThreadWindowInfo)
    lpThreadWindowInfo = (THREADWINDOWINFO *) TlsGetValue(g_TLSThreadWindowInfo);

    ASSERT (lpThreadWindowInfo); // if we don't have this, then a thread isn't running, and we're hosed.
    if (!lpThreadWindowInfo)
    {
        goto Done;
    }

#define DWSTYLE WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN

#ifdef DEBUG_EXPLORER
    piei->wv.bTree = TRUE;
    piei->TreeSplit = 200; // some random default
#endif

    TraceMsg(TF_COCREATE, "IEFrame_NewWindow_SameThread calling CreateWindowEx");

    LPCTSTR pszClass;
    if (piei->uFlags & COF_EXPLORE) {
        pszClass = c_szExploreClass;
    } else if (piei->uFlags & COF_IEXPLORE) {
        pszClass = c_szIExploreClass;
    } else {
        pszClass = c_szCabinetClass;
    }
    
    {
    HMENU hmenu = g_hmenuFullSB;
    if (!(g_bBrowserFlags & BROWF_CLASSICMENUS))
        hmenu = NULL;

    HWND hwnd = CreateWindowEx(
#ifdef WIN16
			       0,
#else
			       WS_EX_WINDOWEDGE,
#endif
                               pszClass,
                               NULL, DWSTYLE,
                               CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                               NULL, 
                               hmenu, 
                               HINST_THISDLL, piei);

    if (hwnd)
    {
        // Enable File Manager drag-drop for win16
        DragAcceptFiles(hwnd, TRUE);

        CShellBrowser* psb = (CShellBrowser*)SendMessage(hwnd, WMC_GETTHISPTR, 0, 0);

        if (psb)
        {
            psb->_AfterWindowCreated(piei);

            lpThreadWindowInfo->cWindowCount++;
            // tack new window on end of list
            CShellBrowser** rgpsb = (CShellBrowser**)
                LocalReAlloc(lpThreadWindowInfo->rgpsb, 
                             lpThreadWindowInfo->cWindowCount * sizeof(CShellBrowser *),
                             LMEM_MOVEABLE);
            ASSERT(rgpsb);
            if (rgpsb)
            {
                lpThreadWindowInfo->rgpsb = rgpsb;
                rgpsb[lpThreadWindowInfo->cWindowCount-1] = psb;
                psb->AddRef();
            }

            // The message pump would go here.
            // Let the threadproc handle it.

            psb->Release();
        }
        
    } else {
        // Unregister any pending that may be there
        WinList_Revoke(piei->dwRegister);
        TraceMsg(DM_ERROR, "IEFrame_NewWindow_SameThread CreateWindow failed");
    }
    }

Done:
    if (pszCloseEvent)
    {
        FireEventSz(pszCloseEvent);
    }

    if (piei)
        delete piei;
}
#endif // NO_MARSHALLING

BOOL CheckForValidSourceFile(HWND hDlg, LPTSTR szPath, LPTSTR szDisplay )
{
    BOOL fRet = FALSE, err = FALSE;

    if(  szPath && *szPath && PathIsFilePath(szPath) && PathFileExists(szPath) )
    {
         HANDLE hFile = CreateFile( szPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, 
                                   FILE_ATTRIBUTE_NORMAL, NULL);
         if( hFile != INVALID_HANDLE_VALUE ) 
         {
             if( GetFileSize( hFile, NULL ) <= (DWORD)0 )
             {
                 err = TRUE; 
             }

             CloseHandle(hFile);
         }
         else
         {
             err = TRUE;
         } 
    }
    else
    {
        err = TRUE;
    }

    if( err )
    {
        TCHAR message[ MAX_URL_STRING + MAX_DISPLAY_LEN  + 3];
        TCHAR title [TITLE_LEN];
        StrCpyN( message, szDisplay, lstrlen(szDisplay) + 1 );
        message[ lstrlen( szDisplay ) ] = TEXT('\n');
        message[ lstrlen( szDisplay ) + 1 ] = TEXT('\n');
        MLLoadString(IDS_DOWNLOAD_BADCACHE,
                      message + lstrlen(szDisplay) + 2,
                      ARRAYSIZE(message) - lstrlen(szDisplay) - 2);
        MLLoadString(IDS_DOWNLOADFAILED, title, ARRAYSIZE(title));
        MessageBox( hDlg, message, title, MB_ICONWARNING|MB_OK);
        return fRet;
    }

    return TRUE;
}

STDAPI UnixSendDocToMailRecipient(LPCITEMIDLIST pidl, UINT uiCodePage, DWORD grfKeyState)
{
    HRESULT hres;
    IUniformResourceLocator *purl;
    IShellLink *psl;
    LPTSTR lptszURLName;
    CHAR szTitle[MAX_URL_STRING];
    IShellFolder *psfParent;
    LPCITEMIDLIST pidlChild;
    TCHAR           tszCommandLine[INTERNET_MAX_URL_LENGTH];
    TCHAR           tszExpandedCommand[INTERNET_MAX_URL_LENGTH];
    TCHAR tszTempFile1[MAX_PATH];
    TCHAR tszTempFile2[MAX_PATH];
    TCHAR tszTempFile[MAX_PATH];
    UINT            nCommandSize;
    int             i;
    HKEY    hkey;
    DWORD   dw;
    TCHAR *pchPos;
    BOOL bMailed;
    STARTUPINFO stInfo;
    SYSTEMTIME systemtime;
    int pid;
    UINT uUnique;

    szTitle[0] = '\0';

    hres = RegCreateKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_MAILCLIENTS, 0, NULL, 0, 
                          KEY_READ, NULL, &hkey, &dw);
    if (hres == ERROR_SUCCESS)
    {
        dw = INTERNET_MAX_URL_LENGTH;
        hres = RegQueryValueEx(hkey, REGSTR_PATH_CURRENT, NULL, NULL, 
                               (LPBYTE)tszCommandLine, &dw);
        RegCloseKey(hkey);

        if (hres == ERROR_SUCCESS)
        {           
            dw = SHExpandEnvironmentStrings(tszCommandLine, tszExpandedCommand, 
                                          INTERNET_MAX_URL_LENGTH);
             _tcscpy(tszCommandLine, TEXT("X "));
             _tcscat(tszCommandLine, tszExpandedCommand);
             memset(&stInfo, 0, sizeof(stInfo));
             stInfo.cb = sizeof(stInfo);
             for (i = _tcslen(tszCommandLine); i > 0; i--)
             {
                 if (tszCommandLine[i] == '/')
                 {
                      tszCommandLine[i] = '\0';
                      break;
                 }
             }
             if (!pidl)
             {
                 if (grfKeyState == MAIL_ACTION_SEND)
                 {
                     _tcscat(tszCommandLine, MAILTO_0_OPTION);
                 }
                 else if (grfKeyState == MAIL_ACTION_READ)
                 {
                     _tcscat(tszCommandLine, READMAIL_OPTION);
                 }
                 bMailed = CreateProcess(tszExpandedCommand, tszCommandLine, NULL, NULL, 
                                        FALSE, DETACHED_PROCESS, NULL, 
                                        NULL, &stInfo, NULL);
                 return hres;
             }
         }
         else
         {
             return hres;
         }
    }
    else
    {
         return hres;
    }

    hres = CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER, 
                            IID_IUniformResourceLocator, (LPVOID*)&purl);
    if (SUCCEEDED(hres))
    {
        hres = purl->QueryInterface(IID_IShellLink, (LPVOID*)&psl);
        if (SUCCEEDED(hres))
        {
            hres = psl->SetIDList(pidl);
            psl->Release();
        }

        hres = purl->GetURL(&lptszURLName);
        if (SUCCEEDED(hres))
        {
            hres = IEBindToParentFolder(pidl, &psfParent, &pidlChild);
            if (SUCCEEDED(hres))
            {
                STRRET sr;

                hres = psfParent->GetDisplayNameOf(pidlChild, SHGDN_INFOLDER | SHGDN_NORMAL, &sr);
                if (SUCCEEDED(hres))
                    hres = StrRetToBufA(&sr, pidlChild, szTitle, ARRAYSIZE(szTitle));
                psfParent->Release();
            }       

            tszTempFile[0] = '\0';
            GetTempPath(MAX_PATH, tszTempFile);

            _tcscpy(tszTempFile1, tszTempFile);
            _tcscpy(tszTempFile2, tszTempFile);
            
            if (grfKeyState == FORCE_LINK)
                _tcscat(tszCommandLine, MAILLINK_OPTION);
            else if (grfKeyState == FORCE_COPY)
                _tcscat(tszCommandLine, MAILPAGE_OPTION);

            _tcscat(tszTempFile1, TEXT(".#temp1"));
            _tcscat(tszTempFile2, TEXT(".#temp2"));

            pid = _getpid();
            GetSystemTime(&systemtime);

            uUnique = systemtime.wMilliseconds + systemtime.wSecond * pid;
            
            if (uUnique == GetTempFileName(TEXT("."), NULL, uUnique, tszTempFile))
            {
                HANDLE hTempFile1, hTempFile2;
                
                _tcscat(tszTempFile1, tszTempFile);
                _tcscat(tszTempFile2, tszTempFile);
                
                hTempFile1 = CreateFile(tszTempFile1, GENERIC_WRITE, FILE_SHARE_READ, 
                                        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hTempFile1 != INVALID_HANDLE_VALUE)
                {
                    hTempFile2 = CreateFile(tszTempFile2, GENERIC_WRITE, FILE_SHARE_READ, 
                                            NULL, CREATE_ALWAYS, 
                                            FILE_ATTRIBUTE_NORMAL, NULL);
                    if (hTempFile2 != INVALID_HANDLE_VALUE)
                    {
                        CHAR szURLName[MAX_URL_STRING];
                        DWORD dw;

                        WriteFile(hTempFile1, szTitle, 
                                  lstrlenA(szTitle) + 1, 
                                  &dw, NULL);
                        WideCharToMultiByte(CP_ACP, 0, lptszURLName, -1, 
                                            szURLName, MAX_URL_STRING, NULL, NULL);
                        WriteFile(hTempFile2, szURLName, 
                                  lstrlenA(szURLName) + 1, &dw, NULL);
                        _tcscat(tszCommandLine, TEXT(" "));
                        _tcscat(tszCommandLine, tszTempFile1);
                        _tcscat(tszCommandLine, TEXT(" "));
                        _tcscat(tszCommandLine, tszTempFile2);
                        bMailed = CreateProcess(tszExpandedCommand, tszCommandLine, NULL, NULL, 
                                                FALSE, DETACHED_PROCESS, NULL, 
                                                NULL, &stInfo, NULL);
                        CloseHandle(hTempFile2);
                    }
                    else
                    {
                        DeleteFile(tszTempFile1);
                    }
                    CloseHandle(hTempFile1);
                }
            }

            HRESULT hres2;
            IMalloc *pMalloc;
            hres2 = SHGetMalloc(&pMalloc);
            if (SUCCEEDED(hres2))
            {
                pMalloc->Free(lptszURLName);
                pMalloc->Release();
            }
        }
        purl->Release();
    }
    
    return hres;
}


//////////////////////////////////////////////////////////////////////
// Importing Bookmarks on startup
//////////////////////////////////////////////////////////////////////

extern TCHAR * szIEFavoritesRegSub; 
extern TCHAR * szIEFavoritesRegKey;

BOOL ImportBookmarksStartup(HINSTANCE hInstWithStr)
{
    TCHAR    szFavoritesDir[MAX_PATH];
    TCHAR    szBookmarksDir[MAX_PATH];
    HANDLE  hBookmarksFile        = INVALID_HANDLE_VALUE;
    BOOL    fSuccess                = FALSE;
    

    // Initialize Variables
    szBookmarksDir[0] = TEXT('\0');
    szFavoritesDir[0] = TEXT('\0');

    // Get Bookmarks Dir
    if (TRUE == GetNavBkMkDir( szBookmarksDir, sizeof(szBookmarksDir) ) )
    {
        if ((NULL != szBookmarksDir) && (szBookmarksDir[0] != TEXT('\0')))
        {
            hBookmarksFile = CreateFile(szBookmarksDir, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
            
            if ( hBookmarksFile != INVALID_HANDLE_VALUE ) 
            {
                // Get Favorites Dir
                if (TRUE == GetPathFromRegistry(szFavoritesDir, MAX_PATH, HKEY_CURRENT_USER,
                    szIEFavoritesRegSub, szIEFavoritesRegKey))
                {
                    if ((NULL != szFavoritesDir) && (szFavoritesDir[0] != TEXT('\0')))
                    {
                        // Verify it's a valid Bookmarks file
                        if (TRUE == VerifyBookmarksFile( hBookmarksFile ))
                        {
                            TCHAR szSubDir[MAX_PATH];
                            szSubDir[0] = TEXT('\0');

                            // Import under a specific folder.
                            if (0 != MLLoadString(IDS_NS_BOOKMARKS_DIR, szSubDir, ARRAYSIZE(szSubDir)))
                            {
                                StrCatBuff(szFavoritesDir, szSubDir, MAX_PATH);
                            }
                      
                            // Do the importing...
                            fSuccess = ImportBookmarks(szFavoritesDir, szBookmarksDir, NULL);
                        }
                    }
                }
            }
        }
    }

    if (INVALID_HANDLE_VALUE != hBookmarksFile)
    {
        CloseHandle(hBookmarksFile);
    }

    return(fSuccess);
}


STDAPI_(BOOL) IsLocalFolderPidl( LPCITEMIDLIST pidl )
{
    BOOL bCheckForFilePath = FALSE;
 
    if( IsURLChild( pidl, TRUE ) )
    {
             // Get Path from pidl
         TCHAR szTmpUrl[ MAX_URL_STRING ];
         LPITEMIDLIST pidlTmp = ILClone( pidl );
         ::IEGetDisplayName(pidlTmp, szTmpUrl, SHGDN_FORPARSING);
         if (GetUrlScheme(szTmpUrl) == URL_SCHEME_FILE )
              bCheckForFilePath = TRUE;
         ILFree( pidlTmp );
    }
    else
    {
         bCheckForFilePath = TRUE;
    }

    if( bCheckForFilePath )
    {
        if( ILIsFileSysFolder(pidl) )
             return TRUE;

        // we are not browsing the web
        LPITEMIDLIST pidlNew;
        DWORD dwAttrib = SFGAO_FOLDER;

        if (SUCCEEDED(IEGetAttributesOf(pidl, &dwAttrib)) &&
           (dwAttrib & SFGAO_FOLDER))
        {
            return TRUE;
        }
    }

    return FALSE;
}


STDAPI_(LONG) StrLenUnalignedW( UNALIGNED WCHAR * str )
{
   LONG count = 0;
   if( !str ) return 0;

   while( *str++ ) count++;

   return count;
}


STDAPI_(LONG) StrLenUnalignedA( UNALIGNED CHAR * str )
{
   LONG count = 0;
   if( !str ) return 0;

   while( *str++ ) count++;

   return count;
}

#ifdef UNIX_FEATURE_ALIAS

STDAPI_(HDPA) GetGlobalAliasList()
{
    static HDPA s_hdpa = NULL;

    if( !s_hdpa )
    {
        HDPA hdpa = DPA_Create(4);

        if (hdpa)
        {
            LoadAliases( hdpa );
            return hdpa;
        }
        else
            return NULL;

    }
    else
        return s_hdpa;
}

STDAPI RefreshGlobalAliasList()
{
    HDPA list = GetGlobalAliasList();
    if( list )
    {
        FreeAliases(list);
        LoadAliases(list);
    }
}

#endif /* UNIX_FEATURE_ALIAS */

extern "C" 
BOOL APIENTRY DllMain_Internal(HINSTANCE hDll, DWORD dwReason, void *lpReserved);

extern "C" 
BOOL APIENTRY DllMain(HINSTANCE hDll, DWORD dwReason, void *lpReserved)
{
    return DllMain_Internal( hDll, dwReason, lpReserved  );
}

STDAPI SafeGetItemObject(LPSHELLVIEW psv, UINT uItem, REFIID riid, LPVOID *ppv)
{
    return (HRESULT)(psv->GetItemObject(uItem, riid, ppv));
}

/* The following is needed so that we will have the latest version number for
 * the browser under HKLM\Software\Microsoft\Active Setup\Installed Components
 */

#define REGSTRACTIVESETUP TEXT("Software\\Microsoft\\Active Setup\\Installed Components\\")

HRESULT UnixRegisterBrowserInActiveSetup()
{
    HRESULT hr = E_FAIL;
    HKEY    hKey = NULL;
    HKEY    hKeyOldVer = NULL;
    DWORD   dw;
    CLSID   browserClsid = COMPID_IE4;
    LPOLESTR pszGUID = NULL;
    TCHAR   szKeyName[MAX_PATH];
    TCHAR   szVersion[30];
    TCHAR   szOldVersion[30];
    DWORD   dwType;
    DWORD   dwLen;

    if (FAILED((hr = StringFromCLSID(browserClsid, &pszGUID))))
       goto Cleanup;

    StrCpyN(szKeyName, REGSTRACTIVESETUP, ARRAYSIZE(szKeyName));
    StrCatBuff(szKeyName, pszGUID, ARRAYSIZE(szKeyName));

    hr = RegCreateKeyEx(
            HKEY_LOCAL_MACHINE,
            szKeyName,
            0,
            NULL,
            0,
            KEY_ALL_ACCESS,
            NULL,
            &hKey,
            &dw);
 
    if (hr != ERROR_SUCCESS)
       goto Cleanup;

    wnsprintf(szVersion,ARRAYSIZE(szVersion), TEXT("%d,%02d,%04d,%04d"),
              VER_MAJOR_PRODUCTVER,
              VER_MINOR_PRODUCTVER,
              VER_PRODUCTBUILD,
              VER_PRODUCTBUILD_QFE);

    hr = RegQueryValueEx(hKey,
                         TEXT("Version"),
                         NULL,
                         &dwType, 
                         (LPBYTE)szOldVersion,
                         &dwLen);

    if (hr == ERROR_SUCCESS)
    {
       hr = RegSetValueEx(hKey,
                          TEXT("OldVersion"),
                          NULL,
                          REG_SZ,
                          (LPBYTE)szVersion,
                          ((1+lstrlen(szVersion))*sizeof(TCHAR)));
    }

    hr = RegSetValueEx(hKey,
                       TEXT("Version"),
                       NULL,
                       REG_SZ,
                       (LPBYTE)szVersion, ((1+lstrlen(szVersion))*sizeof(TCHAR)));
Cleanup:

    if (hKey)
       RegCloseKey(hKey);

    CoTaskMemFree(pszGUID);

    return hr;
}

EXTERN_C LPITEMIDLIST UnixUrlToPidl(UINT uiCP, LPCTSTR pszUrl, LPCWSTR pszLocation)
{
    LPCITEMIDLIST pidlUrl = UrlToPidl(uiCP, pszUrl, pszLocation);
    return ILCombine(c_pidlURLRoot, pidlUrl);
}
