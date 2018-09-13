/*
 * urlexec.cpp - IUnknown implementation for Intshcut class.
 */

#include "project.hpp"
#include "urlshell.h"
#include "clsfact.h"
#include "resource.h"

#include <mluisupp.h>

// URL Exec Hook

class CURLExec : public IShellExecuteHookA, public IShellExecuteHookW
{
private:

    ULONG       m_cRef;

    ~CURLExec(void);    // Prevent this class from being allocated on the stack or it will fault.

public:
    CURLExec(void);

    // IShellExecuteHook methods

	// Ansi
    STDMETHODIMP Execute(LPSHELLEXECUTEINFOA pei);
    // Unicode
    STDMETHODIMP Execute(LPSHELLEXECUTEINFOW pei);

    // IUnknown methods
    
    STDMETHODIMP  QueryInterface(REFIID riid, PVOID *ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
#ifdef DEBUG
    friend BOOL IsValidPCURLExec(const CURLExec * pue);
#endif
};


#ifdef DEBUG

BOOL IsValidPCURLExec(CURLExec * pue)
{
    return (IS_VALID_READ_PTR(pue, CURLExec));
}

#endif


CURLExec::CURLExec(void) : m_cRef(1)
{
    // CURLExec objects should always be allocated

    ASSERT(IS_VALID_STRUCT_PTR(this, CURLExec));

    DLLAddRef();
}

CURLExec::~CURLExec(void)
{
    ASSERT(IS_VALID_STRUCT_PTR(this, CURLExec));

    DLLRelease();
}


/*----------------------------------------------------------
Purpose: IUnknown::QueryInterface handler for CURLExec

*/
STDMETHODIMP CURLExec::QueryInterface(REFIID riid, PVOID *ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IShellExecuteHookA))
    {
        *ppvObj = SAFECAST(this, IShellExecuteHookA *);
    }
    else if (IsEqualIID(riid, IID_IShellExecuteHookW))
    {
    	*ppvObj = SAFECAST(this, IShellExecuteHookW *);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;
}


STDMETHODIMP_(ULONG) CURLExec::AddRef()
{
    return ++m_cRef;
}


STDMETHODIMP_(ULONG) CURLExec::Release()
{
    m_cRef--;
    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}


typedef BOOL (* PFNVERQUERYVALUEA)(const LPVOID pBlock,
        LPCSTR lpSubBlock, LPVOID * lplpBuffer, LPDWORD lpuLen);
typedef DWORD (* PFNGETFILEVERSIONINFOSIZEA) (
        LPCSTR lptstrFilename, LPDWORD lpdwHandle);
typedef BOOL (* PFNGETFILEVERSIONINFOA) (
        LPCSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData);

#define BLOCK_FILE2          TEXT("infinst2.exe") //for 3.02
#define BLOCK_FILE           TEXT("infinst.exe")  //for 3.0 and 3.01
#define BLOCK_FILE_VERSION   0x00040046  //4.70
// from shlexec.c
#define SEE_MASK_CLASS (SEE_MASK_CLASSNAME|SEE_MASK_CLASSKEY)
/*----------------------------------------------------------
Purpose: IShellExecuteHook::Execute handler for CURLExec

*/
STDMETHODIMP CURLExec::Execute(LPSHELLEXECUTEINFOA pei)
{
    HRESULT hres;
    
    ASSERT(IS_VALID_STRUCT_PTR(this, CURLExec));
    ASSERT(IS_VALID_READ_PTR(pei, SHELLEXECUTEINFO));
    
    if (! pei->lpVerb ||
        ! lstrcmpi(pei->lpVerb, TEXT("open")))
    {
        if (pei->lpFile)
        {
            LPTSTR pszURL;
            LPTSTR psz = PathFindFileName(pei->lpFile);

            if (lstrcmpi(psz, BLOCK_FILE2) == 0 ||
                lstrcmpi(psz, BLOCK_FILE) == 0)
            {
                HINSTANCE hinst = LoadLibrary("VERSION.DLL");
                
                if(hinst)
                {
                    PFNVERQUERYVALUEA pfnVerQueryValue = 
                        (PFNVERQUERYVALUEA)GetProcAddress(hinst, "VerQueryValueA");
                    PFNGETFILEVERSIONINFOSIZEA pfnGetFileVersionInfoSize = 
                        (PFNGETFILEVERSIONINFOSIZEA)GetProcAddress(hinst, "GetFileVersionInfoSizeA");
                    PFNGETFILEVERSIONINFOA pfnGetFileVersionInfo = 
                        (PFNGETFILEVERSIONINFOA)GetProcAddress(hinst, "GetFileVersionInfoA");

                    if (pfnVerQueryValue      &&
                        pfnGetFileVersionInfo &&
                        pfnGetFileVersionInfoSize)
                    {
                        CHAR   chBuffer[2048]; // hopefully this is enough                
                        DWORD  cb;
                        DWORD  dwHandle;
                        VS_FIXEDFILEINFO *pffi = NULL;
                        
                        cb = pfnGetFileVersionInfoSize(pei->lpFile, &dwHandle); 
                        if (cb <= ARRAYSIZE(chBuffer) &&
                            pfnGetFileVersionInfo(pei->lpFile, dwHandle, ARRAYSIZE(chBuffer), chBuffer) &&
                            pfnVerQueryValue(chBuffer, TEXT("\\"), (LPVOID*)&pffi, &cb))
                        {
                            if (pffi->dwProductVersionMS == BLOCK_FILE_VERSION &&
                                pei->fMask & SEE_MASK_NOCLOSEPROCESS)                            
                            {
                                STARTUPINFO         si;
                                PROCESS_INFORMATION pi;
                                TCHAR               szText[256];
                                TCHAR               szTitle[80];

                                memset(&si, 0, sizeof(si));
                                si.cb = sizeof(si);
                        
                                if (CreateProcess(NULL, TEXT("rundll32.exe url.dll,DummyEntryPoint"),                                                   
                                    NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
                                {
                                    pei->hProcess = pi.hProcess;
                                }
                                // inform user that we won't run this exe
                                MLLoadString(IDS_IE3_INSTALL_BLOCK_TEXT,  szText,  ARRAYSIZE(szText));
                                MLLoadString(IDS_IE3_INSTALL_BLOCK_TITLE, szTitle, ARRAYSIZE(szTitle));

                                MessageBox(pei->hwnd, szText, szTitle, MB_OK | MB_ICONSTOP);
                                // say that we ran the file and everything went fine
                                // 42 copied from the end of the function
                                // one comment in ShellExecuteNormal implies that 42 prevents any
                                // messages from poping up -- and that's what we need since we have
                                // our own little message
                                pei->hInstApp = (HINSTANCE)42; 
                                FreeLibrary(hinst);
                                return S_OK;
                            }
                        }
                    }
                    FreeLibrary(hinst);
                }
            }
            
            // This should succeed only for real URLs.  We should fail
            // for file paths and let the shell handle those.

            hres = TranslateURL(pei->lpFile, 
                                TRANSLATEURL_FL_GUESS_PROTOCOL | TRANSLATEURL_FL_CANONICALIZE,
                                &pszURL);
            
            if (SUCCEEDED(hres))
            {
                LPCTSTR pszURLToUse;
                
                pszURLToUse = (hres == S_OK) ? pszURL : pei->lpFile;
                
                hres = ValidateURL(pszURLToUse);
                
                if (SUCCEEDED(hres))
                {
                    IUniformResourceLocator * purl;

                    hres = SHCoCreateInstance(NULL, &CLSID_InternetShortcut, NULL, IID_IUniformResourceLocator, (void **)&purl);
                    if (SUCCEEDED(hres))
                    {
                        hres = purl->SetURL(pszURLToUse, 0);
                        if (hres == S_OK)
                        {
                            IShellLink * psl;

                            hres = purl->QueryInterface(IID_IShellLink, (void **)&psl);
                            if (SUCCEEDED(hres))
                            {
                                URLINVOKECOMMANDINFO urlici;

                                EVAL(psl->SetShowCmd(pei->nShow) == S_OK);
                                
                                urlici.dwcbSize = SIZEOF(urlici);
                                urlici.hwndParent = pei->hwnd;
                                urlici.pcszVerb = NULL;
                                
                                urlici.dwFlags = IURL_INVOKECOMMAND_FL_USE_DEFAULT_VERB;
                                
                                if (IsFlagClear(pei->fMask, SEE_MASK_FLAG_NO_UI))
                                    SetFlag(urlici.dwFlags, IURL_INVOKECOMMAND_FL_ALLOW_UI);
                                
                                hres = purl->InvokeCommand(&urlici);
                                
                                if (hres != S_OK)
                                    SetFlag(pei->fMask, SEE_MASK_FLAG_NO_UI);

                                psl->Release();
                            }
                        }
                        purl->Release();
                    }
                }

                if (pszURL)
                    LocalFree(pszURL);
            }
        }
        else
            // BUGBUG (scotth): This hook only handles execution of file string, not IDList.
            hres = S_FALSE;
    }
    else
        // Unrecognized verb.
        hres = S_FALSE;
    
    if (hres == S_OK)
        pei->hInstApp = (HINSTANCE)42;  // BUGBUG (scotth): huh??
    else if (FAILED(hres))
    {
        switch (hres)
        {
        case URL_E_INVALID_SYNTAX:
        case URL_E_UNREGISTERED_PROTOCOL:
            hres = S_FALSE;
            break;
            
        case E_OUTOFMEMORY:
            pei->hInstApp = (HINSTANCE)SE_ERR_OOM;
            hres = E_FAIL;
            break;
            
        case IS_E_EXEC_FAILED:
            // Translate execution failure into "file not found".
            pei->hInstApp = (HINSTANCE)SE_ERR_FNF;
            hres = E_FAIL;
            break;
            
        default:
            // pei->lpFile is bogus.  Treat as file not found.
            ASSERT(hres == E_POINTER);

            pei->hInstApp = (HINSTANCE)SE_ERR_FNF;
            hres = E_FAIL;
            break;
        }
    }
    else
        ASSERT(hres == S_FALSE);
    
    ASSERT(hres == S_OK ||
        hres == S_FALSE ||
        hres == E_FAIL);
    
    return hres;
}

STDMETHODIMP CURLExec::Execute(LPSHELLEXECUTEINFOW pei)
{
	// thunk stuff copied from shlexec.c InvokeShellExecuteHook
	SHELLEXECUTEINFOA seia;
    UINT cchVerb = 0;
    UINT cchFile = 0;
    UINT cchParameters = 0;
    UINT cchDirectory  = 0;
    UINT cchClass = 0;
    LPSTR lpszBuffer;
    HRESULT hres = E_FAIL;

    seia = *(SHELLEXECUTEINFOA*)pei;    // Copy all of the binary data

    if (pei->lpVerb)
    {
        cchVerb = WideCharToMultiByte(CP_ACP,0,
                                      pei->lpVerb, -1,
                                      NULL, 0,
                                      NULL, NULL)+1;
    }

    if (pei->lpFile)
        cchFile = WideCharToMultiByte(CP_ACP,0,
                                      pei->lpFile, -1,
                                      NULL, 0,
                                      NULL, NULL)+1;

    if (pei->lpParameters)
        cchParameters = WideCharToMultiByte(CP_ACP,0,
	                                        pei->lpParameters, -1,
                                            NULL, 0,
                                            NULL, NULL)+1;

    if (pei->lpDirectory)
        cchDirectory = WideCharToMultiByte(CP_ACP,0,
                                           pei->lpDirectory, -1,
                                           NULL, 0,
                                           NULL, NULL)+1;
    if (((pei->fMask & SEE_MASK_CLASS) == SEE_MASK_CLASSNAME) && pei->lpClass)
        cchClass = WideCharToMultiByte(CP_ACP,0,
                                       pei->lpClass, -1,
                                       NULL, 0,
                                       NULL, NULL)+1;

	// what is this (alloca)? InvokeShellExecuteHook is not freeing lpszBuffer
    //lpszBuffer = alloca(cchVerb+cchFile+cchParameters+cchDirectory+cchClass);
    lpszBuffer = (LPSTR)LocalAlloc(LPTR, cchVerb+cchFile+cchParameters+cchDirectory+cchClass);
    if (lpszBuffer)
	{
		LPSTR lpsz = lpszBuffer;
		
    	seia.lpVerb = NULL;
	    seia.lpFile = NULL;
    	seia.lpParameters = NULL;
	    seia.lpDirectory = NULL;
    	seia.lpClass = NULL;

	    //
    	// Convert all of the strings to ANSI
	    //
    	if (pei->lpVerb)
	    {
    	    WideCharToMultiByte(CP_ACP, 0, pei->lpVerb, -1,
        	                    lpszBuffer, cchVerb, NULL, NULL);
	        seia.lpVerb = lpszBuffer;
    	    lpszBuffer += cchVerb;
	    }
    	if (pei->lpFile)
	    {
    	    WideCharToMultiByte(CP_ACP, 0, pei->lpFile, -1,
	                            lpszBuffer, cchFile, NULL, NULL);
    	    seia.lpFile = lpszBuffer;
	        lpszBuffer += cchFile;
    	}
	    if (pei->lpParameters)
    	{
	        WideCharToMultiByte(CP_ACP, 0,
    	                        pei->lpParameters, -1,
	                            lpszBuffer, cchParameters, NULL, NULL);
    	    seia.lpParameters = lpszBuffer;
	        lpszBuffer += cchParameters;
    	}
	    if (pei->lpDirectory)
    	{
	        WideCharToMultiByte(CP_ACP, 0,
    	                        pei->lpDirectory, -1,
	                            lpszBuffer, cchDirectory, NULL, NULL);
    	    seia.lpDirectory = lpszBuffer;
	        lpszBuffer += cchDirectory;
    	}
	    if (((pei->fMask & SEE_MASK_CLASS) == SEE_MASK_CLASSNAME) && pei->lpClass)
    	{
        	WideCharToMultiByte(CP_ACP, 0,
	                            pei->lpClass, -1,
    	                        lpszBuffer, cchClass, NULL, NULL);
	        seia.lpClass = lpszBuffer;
    	}

    	hres = Execute(&seia);
    	// now thunk the possible new stuff back
    	pei->hInstApp = seia.hInstApp;
    	if (pei->fMask & SEE_MASK_NOCLOSEPROCESS)
        	pei->hProcess = seia.hProcess;


    	LocalFree(lpsz);
	}

    return hres;
}

STDAPI CreateInstance_URLExec(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut)
{
    *ppvOut = NULL;

    if (punkOuter)
        return CLASS_E_NOAGGREGATION;

    CURLExec *pue = new(CURLExec);
    if (!pue)
        return E_OUTOFMEMORY;

    HRESULT hres = pue->QueryInterface(riid, ppvOut);
    pue->Release();
    return hres;
}

// the following are defined somewhere in ieak
#define RC_WEXTRACT_AWARE   0xAA000000  // means cabpack aware func return code
#define REBOOT_SILENT       0x00000004  // this bit 0 means prompt user before reboot
// this is what ieak looks for to decide if it should not install the rest of the stuff 
#define DO_NOT_REBOOT       (RC_WEXTRACT_AWARE | REBOOT_SILENT)

// the following entry point is used to fake failure by ie 3.02 setup when installing
// quicken 98 on top of ie4 browser only install. 
// we need it to return a specific error code that will prevent ieak from
// installing java vm, and other stuff.
STDAPI_(void) DummyEntryPoint(HWND hwnd, HINSTANCE hAppInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    ExitProcess(DO_NOT_REBOOT);
}
