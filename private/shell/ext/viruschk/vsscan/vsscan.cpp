// vsscan.cpp : Implementation of WinMain


// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f vsscanps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "vsscan.h"
#include "..\\..\\..\\..\\..\\public\\sdk\\inc\\vrsscan.h"

#include "..\\..\\..\\..\\..\\private\\iedev\\uuid\\vrsscan_i.c"


LONG CExeModule::Unlock()
{
	LONG l = CComModule::Unlock();
	if (l == 0)
	{
#if _WIN32_WINNT >= 0x0400
		if (CoSuspendClassObjects() == S_OK)
			PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
#else
		PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
#endif
	}
	return l;
}

CExeModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()


LPCTSTR FindOneOf(LPCTSTR p1, LPCTSTR p2)
{
	while (*p1 != NULL)
	{
		LPCTSTR p = p2;
		while (*p != NULL)
		{
			if (*p1 == *p++)
				return p1+1;
		}
		p1++;
	}
	return NULL;
}

LPWSTR MakeWideStrFromAnsi(LPSTR psz)
{
    LPWSTR pwsz;
    int i;

    // arg checking.
    //
    if (!psz) return NULL;

    // compute the length of the required BSTR
    //
    if ((i = MultiByteToWideChar(CP_ACP, 0, psz, -1, NULL, 0)) <= 0)
        return NULL;

    // allocate the widestr, +1 for terminating null
    //
    pwsz = (LPWSTR) CoTaskMemAlloc(i * sizeof(WCHAR));
    
    if (!pwsz) return NULL;
    MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, i);
    pwsz[i - 1] = 0;
    return pwsz;
}

/////////////////////////////////////////////////////////////////////////////
//
extern "C" int WINAPI _tWinMain(HINSTANCE hInstance, 
	HINSTANCE /*hPrevInstance*/, LPTSTR lpCmdLine, int /*nShowCmd*/)
{
    char szTmp[2048];
    LPWSTR wszDesc;
    TCHAR szSep[] = _T(" ");
    DWORD dwFlags = 0;

    lpCmdLine = GetCommandLine(); //this line necessary for _ATL_MIN_CRT
	HRESULT hRes = CoInitialize(NULL);
//  If you are running on NT 4.0 or higher you can use the following call
//	instead to make the EXE free threaded.
//  This means that calls come in on a random RPC thread
//	HRESULT hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	_ASSERTE(SUCCEEDED(hRes));
	_Module.Init(ObjectMap, hInstance);
	_Module.dwThreadID = GetCurrentThreadId();
	TCHAR szTokens[] = _T("-/");

	int nRet = 0;
	BOOL bRun = TRUE;
	LPCTSTR lpszToken = FindOneOf(lpCmdLine, szTokens);
	while (lpszToken != NULL)
	{
        if (lstrcmpi(lpszToken, _T("UI")) == 0) dwFlags |= SFV_DOUI;
        if (lstrcmpi(lpszToken, _T("NOUI")) == 0) dwFlags |= SFV_DONTDOUI;
        if (lstrcmpi(lpszToken, _T("DEL")) == 0) dwFlags |= SFV_DELETE;
        if (lstrcmpi(lpszToken, _T("ICON")) == 0) dwFlags |= SFV_WANTVENDORICON;
        if (lstrcmpi(lpszToken, _T("?")) == 0) {
            MessageBox(NULL,"vsscan [/UI] [/NOUI] [/DEL] [/ICON] [file1] [file2] ...",NULL,MB_OK);
        }

		if (lstrcmpi(lpszToken, _T("UnregServer"))==0)
		{
			_Module.UpdateRegistryFromResource(IDR_Vsscan, FALSE);
			nRet = _Module.UnregisterServer();
			bRun = FALSE;
			break;
		}
		if (lstrcmpi(lpszToken, _T("RegServer"))==0)
		{
			_Module.UpdateRegistryFromResource(IDR_Vsscan, TRUE);
			nRet = _Module.RegisterServer(TRUE);
			bRun = FALSE;
			break;
		}
		lpszToken = FindOneOf(lpszToken, szTokens);
	}

/*
	if (bRun)
	{
		hRes = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, 
			REGCLS_MULTIPLEUSE);
		_ASSERTE(SUCCEEDED(hRes));

		MSG msg;
		while (GetMessage(&msg, 0, 0, 0))
			DispatchMessage(&msg);

		_Module.RevokeClassObjects();
	}

*/

    // Activate virus scan.

    IUnknown *pUnk = NULL;
    IVirusScanner *pvs = NULL;

    if (FAILED(CoCreateInstance(CLSID_VirusScan, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void **)&pUnk))) {
        MessageBox(NULL,"Failed CoCreateInstance of VirusScan object.\n\nEnsure VIRUSCHK.DLL is registered.",NULL,MB_OK);
        goto Exit;
    }

    if (FAILED(pUnk->QueryInterface(IID_IVirusScanner, (void **)&pvs))) {
        MessageBox(NULL,"Failed QueryInterface for IID_IVirusScanner.",NULL,MB_OK);
        goto Exit;
    }

    lpCmdLine = GetCommandLine(); //this line necessary for _ATL_MIN_CRT
	lpszToken = lpCmdLine;
    
	while (lpszToken != NULL)
	{
        LPTSTR lpszNextToken = (LPTSTR)(LPCTSTR)FindOneOf(lpszToken, szSep);

        if (*lpszToken != '\\' && *lpszToken != '/') {
        
            STGMEDIUM stg;
            HWND hw = GetActiveWindow();
            VIRUSINFO vi;
            LPSTR szResp = "<Unknown>";
            if (lpszNextToken != NULL) {
                lpszNextToken--;
                *lpszNextToken = '\0';
                lpszNextToken++;
            }

            wsprintf(szTmp,"Performing Virus Scan on: \"%s\".", lpszToken);
            MessageBox(NULL,szTmp,NULL,MB_OK);

            // filename
            wszDesc = MakeWideStrFromAnsi((char *)lpszToken);

            stg.tymed = TYMED_FILE;
            stg.lpszFileName = wszDesc;

            vi.cbSize = sizeof(VIRUSINFO);

            HRESULT hr = pvs->ScanForVirus(hw, &stg, wszDesc, dwFlags | SFV_ENGINE_DOUI, &vi);
            switch(hr) {
            case VSCAN_E_NOPROVIDERS: szResp = "No virus scanning providers found."; break;
            case VSCAN_E_CHECKPARTIAL: szResp = "Virus found - but not by all providers."; break;
            case VSCAN_E_CHECKFAIL: szResp = "Virus found - scan failed."; break;
            case VSCAN_E_DELETEFAIL: szResp = "Virus found - tried deleting file but failed."; break;
            case S_FALSE: szResp = "Virus found. & VIRUSINFO returned."; break;
            case S_OK: szResp = "No viruses found."; break;
            default: szResp = "<Unknown Response>";
            }
            wsprintf(szTmp,"Result: hr = %08X - %s",hr, szResp);
            MessageBox(NULL,szTmp,NULL,MB_OK);
        }

        lpszToken = lpszNextToken;
    }

Exit:
    if (pUnk != NULL) pUnk->Release();
    if (pvs != NULL) pvs->Release();

	CoUninitialize();
	return nRet;
}
