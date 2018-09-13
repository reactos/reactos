// NetClip.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "NetClip.h"
#include "Server.h"

#include "MainFrm.h"
#include "Doc.h"
#include "View.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNetClipApp

BEGIN_MESSAGE_MAP(CNetClipApp, CWinApp)
	//{{AFX_MSG_MAP(CNetClipApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

OSVERSIONINFO  g_osvi ;
BOOL g_fDCOM = FALSE;

BOOL RegisterSupportDLLs(CWnd* pParent,BOOL fForce /*=FALSE*/);
/////////////////////////////////////////////////////////////////////////////
// CNetClipApp construction

CNetClipApp::CNetClipApp()
{
    m_fNoUpdate = FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CNetClipApp object

CNetClipApp theApp;


#include <winreg.h>
#define ERROR_BADKEY_WIN16 2 // needed when running on Win32s

LONG RecursiveRegDeleteKey(HKEY hParentKey, LPCTSTR szKeyName)
{
	DWORD   dwIndex = 0L;
	TCHAR   szSubKeyName[256];
	HKEY    hCurrentKey;
	DWORD   dwResult;

	if ((dwResult = RegOpenKey(hParentKey, szKeyName, &hCurrentKey)) ==
		ERROR_SUCCESS)
	{
		// Remove all subkeys of the key to delete
		while ((dwResult = RegEnumKey(hCurrentKey, 0, szSubKeyName, 255)) ==
			ERROR_SUCCESS)
		{
			if ((dwResult = RecursiveRegDeleteKey(hCurrentKey,
				szSubKeyName)) != ERROR_SUCCESS)
				break;
		}

		// If all went well, we should now be able to delete the requested key
		if ((dwResult == ERROR_NO_MORE_ITEMS) || (dwResult == ERROR_BADKEY) ||
			(dwResult == ERROR_BADKEY_WIN16))
		{
			dwResult = RegDeleteKey(hParentKey, szKeyName);
		}
	}

	RegCloseKey(hCurrentKey);
	return dwResult;
}

#ifdef _FEATURE_SERVICE
// When we're started with the /service command line switch
// we run as a service (we assume we've been started via Network OLE's
// LocalService32 mechanism).
//
VOID WINAPI NetClipServiceMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
    // This is called on a new thread so we have to call
    // OleInitialize
    //
    HRESULT hr ;
    if (FAILED(hr = AfxOleInit())) //OleInitialize(NULL)))
    {
        ErrorMessage( _T("Could not initialize OLE; NetClip cannot run."), hr ) ;
        return ;
    }

	// Application was run with /Embedding or /Automation.  Don't show the
	//  main window in this case.
	theApp.m_fServing = TRUE;

	// Register all OLE server (factories) as running.  This enables the
	//  OLE libraries to create objects from other applications.
	COleObjectFactory::RegisterAll();

}
#endif

class CMyCommandLineInfo : public CCommandLineInfo
{
public:
#ifdef _FEATURE_SERVICE
    BOOL m_bRunAsService;
#endif
    BOOL m_bSelfReg;
    BOOL m_bSelfUnReg;
    BOOL m_bServer;
    CMyCommandLineInfo()
        {
#ifdef _FEATURE_SERVICE
          m_bRunAsService = FALSE;
#endif
          m_bSelfReg = FALSE;
          m_bSelfUnReg = FALSE;
          m_bServer = FALSE;
        };
    virtual ~CMyCommandLineInfo() {};
    virtual void ParseParam(const char* pszParam, BOOL bFlag, BOOL bLast );
};

void CMyCommandLineInfo::ParseParam(const char* pszParam, BOOL bFlag, BOOL bLast)
{
    CString strParam(pszParam);
#ifdef _FEATURE_SERVICE
    if (strParam.CompareNoCase(_T("service")) == 0)
        m_bRunAsService = TRUE;
    else
#endif
        if (strParam.CompareNoCase(_T("regserver")) == 0)
        m_bSelfReg = TRUE;
    else if (strParam.CompareNoCase(_T("unregserver")) == 0)
        m_bSelfUnReg = TRUE;
    else if (strParam.CompareNoCase(_T("server")) == 0)
        m_bServer = TRUE;
    else
        CCommandLineInfo::ParseParam(pszParam, bFlag, bLast);
}

/////////////////////////////////////////////////////////////////////////////
// CNetClipApp initialization

BOOL CNetClipApp::InitInstance()
{
	USES_CONVERSION;

    HRESULT hr;
    g_osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO) ;
    GetVersionEx(&g_osvi);

    HINSTANCE hinst ;
	hinst = LoadLibrary( _T("OLE32.DLL") ) ;
    if (hinst > (HINSTANCE)HINSTANCE_ERROR)
    {
        // See if we're DCOM enabled
        HRESULT (STDAPICALLTYPE * lpDllEntryPoint)(void);
        (FARPROC&)lpDllEntryPoint = GetProcAddress(hinst, "CoCreateInstanceEx");
        if (lpDllEntryPoint)
        {
            HKEY hkey=NULL;
            RegOpenKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Ole"), &hkey);
            if (hkey!=NULL)
            {
                TCHAR sz[16];
                DWORD cb = sizeof(sz)/sizeof(sz[0]);
                *sz = '\0';
                RegQueryValueEx(hkey, _T("EnableDCOM"), NULL, NULL, (BYTE*)sz, &cb);
                if (*sz == _T('Y') || *sz == _T('y'))
                    g_fDCOM = TRUE;
            }
        }
        FreeLibrary(hinst);
    }

    // Parse command line for standard shell commands, DDE, file open
	CMyCommandLineInfo cmdInfo;
#ifdef _NTBUILD
    // For some reason the implementation of ParseCommandLine in NT 4.0's MFC
    // does not pick up __argc.  Hence this hack.
    //
	for (int i = 1; i < __argc; i++)
	{
#ifdef _UNICODE
		LPCTSTR pszParam = __wargv[i];
#else
		LPCTSTR pszParam = __argv[i];
#endif
		BOOL bFlag = FALSE;
		BOOL bLast = ((i + 1) == __argc);
		if (pszParam[0] == _T('-') || pszParam[0] == _T('/'))
		{
			// remove flag specifier
			bFlag = TRUE;
			++pszParam;
		}
#if _MFC_VER >= 0x0420
		cmdInfo.ParseParam(T2CA(pszParam), bFlag, bLast);
#else
		cmdInfo.ParseParam(T2A(pszParam), bFlag, bLast);
#endif
	}
#else
	ParseCommandLine(cmdInfo);
#endif

    if (cmdInfo.m_bSelfUnReg == TRUE)
    {
        // Un-register nclipps.dll
	    HINSTANCE hinst = LoadLibrary(_T("NCLIPPS.DLL")) ;
        if (hinst > (HINSTANCE)HINSTANCE_ERROR)
        {
            // Get DllUnRegisterServer function
            HRESULT (STDAPICALLTYPE * lpDllEntryPoint)(void);
            (FARPROC&)lpDllEntryPoint = GetProcAddress(hinst, "DllUnregisterServer");
            if (lpDllEntryPoint)
                (*lpDllEntryPoint)() ;
            FreeLibrary(hinst);
        }

        // Un-register ourselves
        OLECHAR szCLSID[40];
        StringFromGUID2(CNetClipServer::guid, szCLSID, 40);
        CString strReg;
    #ifdef _UNICODE
        strReg.Format(_T("AppID\\%s"), szCLSID);
    #else
        strReg.Format(_T("AppID\\%S"), szCLSID);
    #endif
        RecursiveRegDeleteKey(HKEY_CLASSES_ROOT, strReg);

    #ifdef _UNICODE
        strReg.Format(_T("CLSID\\%s"), szCLSID);
    #else
        strReg.Format(_T("CLSID\\%S"), szCLSID);
    #endif
        RecursiveRegDeleteKey(HKEY_CLASSES_ROOT, strReg);

        strReg = _T("Remote Clipboard");
        RecursiveRegDeleteKey(HKEY_CLASSES_ROOT, strReg);

        return TRUE;
    }

    if (!(cmdInfo.m_bServer || cmdInfo.m_bRunEmbedded || cmdInfo.m_bRunAutomated))
    {
        // Run client
        HKEY hkey;
        OLECHAR szCLSID[40];
        StringFromGUID2(CNetClipServer::guid, szCLSID, 40);
        CString strReg;
        TCHAR sz[] = _T("Remote Clipboard");

    #ifdef _UNICODE
        strReg.Format(_T("AppID\\%s"), szCLSID);
    #else
        strReg.Format(_T("AppID\\%S"), szCLSID);
    #endif
        hr = RegCreateKey(HKEY_CLASSES_ROOT, strReg, &hkey);
        if (hr == ERROR_SUCCESS)
        {
            hr = RegSetValueEx(hkey, _T(""), 0, REG_SZ, (BYTE*)sz, lstrlen(sz));
            lstrcpy(sz, _T("Interactive User"));
            hr = RegSetValueEx(hkey, _T("RunAs"), 0, REG_SZ, (BYTE*)sz, lstrlen(sz));
            RegCloseKey(hkey);

            COleObjectFactory::UpdateRegistryAll();

    #ifdef _UNICODE
            strReg.Format(_T("CLSID\\%s"), szCLSID);
    #else
            strReg.Format(_T("CLSID\\%S"), szCLSID);
    #endif
            hr = RegCreateKey(HKEY_CLASSES_ROOT, strReg, &hkey);
            if (hr == ERROR_SUCCESS)
            {
                lstrcpy(sz, _T("Remote Clipboard"));
                TCHAR* p = W2T(szCLSID);
                hr = RegSetValueEx(hkey, _T(""), 0, REG_SZ, (BYTE*)sz, lstrlen(sz));
                if (hr == ERROR_SUCCESS)
                    hr = RegSetValueEx(hkey, _T("AppID"), 0, REG_SZ, (BYTE*)p, lstrlen(p));
                RegCloseKey(hkey);
            }
        }

        if (hr != ERROR_SUCCESS)
        {
            AfxMessageBox(IDS_SELFREFFAILED);
            return FALSE;
        }

        if (!RegisterSupportDLLs(AfxGetMainWnd(), FALSE))
        {
            //AfxMessageBox(IDS_SELFREFFAILED);
            //return FALSE;
        }

        if (cmdInfo.m_bSelfReg)
            return TRUE;
    }

#ifdef _FEATURE_SERVICE
    // Check to see if launched as an NT service
	if (cmdInfo.m_bRunAsService)
	{
        // We must run as a service
        //
        SERVICE_TABLE_ENTRY ste;
        ste.lpServiceName = _T("Remote OLE Clipboard Server");
        ste.lpServiceProc = NetClipServiceMain;

        if (!StartServiceCtrlDispatcher(&ste))
        {
            // Error!
            TRACE(_T("StartServiceCtrlDispatcher failed!"));
            return FALSE;
        }
	}
    else
#endif

    {
        // Standard initialization
        if (FAILED(hr = AfxOleInit())) //OleInitialize(NULL)))
        {
            ErrorMessage( _T("Could not initialize OLE; NetClip cannot run."), hr ) ;
            return FALSE;
        }

        if (cmdInfo.m_bServer || cmdInfo.m_bRunEmbedded || cmdInfo.m_bRunAutomated)
        {
	        // Application was run with /Embedding or /Automation.  Don't show the
	        //  main window in this case.
	        theApp.m_fServing = TRUE;

            // MFC is hard coded internally to set user control to true if
            // "Embedding" or "Automation" are not found on the cmd line. 
            // We like "server" better (more intuitive) so we force user control
            // to be false here if "server" is detected.
            //
            AfxOleSetUserCtrl(FALSE);

	        // Register all OLE server (factories) as running.  This enables the
	        //  OLE libraries to create objects from other applications.
	        COleObjectFactory::RegisterAll();
            return TRUE;
        }

        SetRegistryKey( IDS_REGISTRYKEY );

    #ifdef _AFXDLL
	    Enable3dControls();			// Call this when using MFC in a shared DLL
    #else
	    Enable3dControlsStatic();	// Call this when linking to MFC statically
    #endif
	    // Register the application's document templates.  Document templates
	    //  serve as the connection between documents, frame windows and views.
        //
        // Assume that we're running on a non-DCOM system. Check for CoCreateInstanceEx
        // to see if DCOM is around.

	    CSingleDocTemplate* pDocTemplate;
        UINT    nResources = IDR_NONETOLE;
        if (g_fDCOM)
            nResources = IDR_MAINFRAME;

	    pDocTemplate = new CSingleDocTemplate(
		    nResources,
		    RUNTIME_CLASS(CNetClipDoc),
		    RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		    RUNTIME_CLASS(CNetClipView));
	    AddDocTemplate(pDocTemplate);

        HRESULT hr = S_OK;
		m_fServing = FALSE;
        int nCmdShow = m_nCmdShow ;
        m_nCmdShow = SW_HIDE ;
        OnFileNew() ;
        CMainFrame* pfrm = (CMainFrame*)GetMainWnd();
        pfrm->RestorePosition(m_lpCmdLine, nCmdShow);
        if (m_lpCmdLine && *m_lpCmdLine)
        {
            if (SUCCEEDED(pfrm->Connect(CString(m_lpCmdLine))))
            {
                pfrm->m_strMachine = m_lpCmdLine;
                pfrm->SetWindowText(pfrm->m_strMachine + _T(" - NetClip"));
            }
            else
            {
                CString str;
                str.Format(_T("Could not connect to the remote clipboard object on %s."), m_lpCmdLine);
                ErrorMessage(str, hr);
            }
        }
        else
        {
            ((CMainFrame*)GetMainWnd())->OnConnectDisconnect();
        }
    }

    return TRUE;
}

CDocument* CNetClipApp::OpenDocumentFile(LPCTSTR lpszFileName)
{
    CNetClipDoc* pdoc;
    if (pdoc = (CNetClipDoc*)CWinApp::OpenDocumentFile(lpszFileName))
    {
	    pdoc->m_strPathName.Empty();      // no path name yet
    }
    return pdoc;
}

// App command to run the dialog
void CNetClipApp::OnAppAbout()
{
    TCHAR szVersion[64] ;
    BYTE* pdata=NULL;
	TCHAR szFileName[_MAX_PATH] ;
	::GetModuleFileName(NULL, szFileName, _MAX_PATH) ;

    wsprintf( szVersion, _T("NetClip build 1.00 - %s"),(LPTSTR)__DATE__  );
#ifndef _MAC
    DWORD dwDummy ;
    DWORD dw = ::GetFileVersionInfoSize(szFileName, &dwDummy) ;
    if (dw)
    {
        pdata = new BYTE[dw] ;
        if (pdata && ::GetFileVersionInfo(szFileName, NULL, dw, pdata))
        {
			DWORD* pdwBuffer ;
			// Get the translation information.
			BOOL bResult = ::VerQueryValue( pdata,
							  _T("\\VarFileInfo\\Translation"),
							  (void**)&pdwBuffer,
							  (UINT*)&dw);
		    if (!bResult || !dw) goto NastyGoto ;

			// Build the path to the OLESelfRegister key
			// using the translation information.
			TCHAR szName[64] ;
			wsprintf( szName,
					 _T("\\StringFileInfo\\%04hX%04hX\\FileVersion"),
					 LOWORD(*pdwBuffer),
					 HIWORD(*pdwBuffer)) ;

		    // Search for the key.
		    bResult = ::VerQueryValue( pdata,
									   szName,
									   (void**)&pdwBuffer,
									   (UINT*)&dw);
		    if (!bResult || !dw) goto NastyGoto ;

#ifdef _UNICODE
            wsprintf( szVersion, _T("NetClip build %s - %S"),  (LPCTSTR)pdwBuffer, (LPSTR)__DATE__ ) ;
#else
            wsprintf( szVersion, _T("NetClip build - %s"),  (LPCTSTR)pdwBuffer, (LPSTR)__DATE__ ) ;
#endif
        }
NastyGoto:
        if (pdata)
            delete []pdata ;
    }
#endif // !_MAC

#ifdef _DEBUG
	lstrcat(szVersion, _T(" Debug Build") ) ;
#endif
    lstrcat(szVersion, _T("\nWritten by Charlie Kindel"));
    ShellAbout(AfxGetMainWnd()->GetSafeHwnd(),AfxGetAppName( ), szVersion, LoadIcon(IDR_MAINFRAME));
}

/////////////////////////////////////////////////////////////////////////////
// CNetClipApp commands

int CNetClipApp::ExitInstance()
{
    OleFlushClipboard();

	return CWinApp::ExitInstance();
}


// This function attempts to register the any supporting DLLs
// such as NetClipPS.DLL
//
BOOL RegisterSupportDLLs(CWnd* pParent,BOOL fForce /*=FALSE*/)
{
//	CFileDialog

    CString str ;
    CString str2 ;
	CString strSupportDLL = _T("NCLIPPS.DLL") ;
    HINSTANCE hinst ;
	BOOL	fRet = FALSE ;

TryToLoad:	
	hinst = LoadLibrary( strSupportDLL ) ;
    if (hinst > (HINSTANCE)HINSTANCE_ERROR)
    {
        // Get DllRegisterServer function
        HRESULT (STDAPICALLTYPE * lpDllEntryPoint)(void);
        (FARPROC&)lpDllEntryPoint = GetProcAddress(hinst, "DllRegisterServer");
        if (lpDllEntryPoint)
        {
            HRESULT hr ;
            if (FAILED(hr = (*lpDllEntryPoint)()))
            {
                str.LoadString( IDS_AUTOREGFAILED ) ;
                str2.LoadString( IDS_AUTOREGFAILED2 ) ;
                str += str2 ;
                AfxMessageBox( str ) ;
            }
			else
				fRet = TRUE ;
        }
        else
        {
            str.LoadString( IDS_AUTOREGFAILED3 ) ;
            str2.LoadString( IDS_AUTOREGFAILED2 ) ;
            str += str2 ;
            AfxMessageBox( str ) ;
        }

        FreeLibrary( hinst ) ;
    }
    else
    {
        str.LoadString( IDS_AUTOREGFAILED1 ) ;
        str2.LoadString( IDS_AUTOREGFAILED2 ) ;
        str += str2 ;
        str2.LoadString( IDS_AUTOREGFAILED4 ) ;
        str += str2 ;
        if (AfxMessageBox( str, MB_YESNO ) == IDYES)
		{
			static TCHAR szFilter[] = _T("DLL Files (*.dll)|*.dll|AllFiles(*.*)|*.*|") ;

			CFileDialog dlg(TRUE, _T("NCLIPPS.DLL"), NULL,
							OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
							szFilter, pParent);
			if (IDOK == dlg.DoModal())
			{
				strSupportDLL = dlg.GetPathName() ;
				goto TryToLoad;
			}
		}
    }

	return fRet ;
}
