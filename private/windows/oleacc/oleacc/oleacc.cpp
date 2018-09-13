// Copyright (c) 1996-1999 Microsoft Corporation

// ===========================================================================
// File: O L E A C C . C P P
// 
// ===========================================================================

// Includes: ---------------------------------------------------------------
#include "oleacc_p.h"
#include "w95trace.h"
#include "memchk.h"

#define UNUSED(param) (param)

// Shared Globals: ----------------------------------------------------------
#pragma data_seg("Shared")
LONG        cProcessesMinus1 = -1;      // number of attached processes minus 1 cuz the Interlocked APIs suck
HANDLE      hheapShared = NULL;         // handle to the shared heap (Win95 only)
#pragma data_seg()


// %%Globals: ----------------------------------------------------------------
HANDLE      g_hMutexUnique = NULL;      // mutex used for access to uniqueness value in api.cpp
HINSTANCE   hinstDll;           // instance of this library
HINSTANCE   hinstResDll;		// instance of resource library
#ifdef _X86_
BOOL        fWindows95;         // running on Windows '95?
#endif // _X86_
BOOL        fCreateDefObjs;     // running with new USER32?

LPFNGETGUITHREADINFO    lpfnGuiThreadInfo;  // USER32 GetGUIThreadInfo()
LPFNGETCURSORINFO       lpfnCursorInfo;     // USER32 GetCursorInfo()
LPFNGETWINDOWINFO       lpfnWindowInfo;     // USER32 GetWindowInfo()
LPFNGETTITLEBARINFO     lpfnTitleBarInfo;   // USER32 GetTitleBarInfo()
LPFNGETSCROLLBARINFO    lpfnScrollBarInfo;  // USER32 GetScrollBarInfo()
LPFNGETCOMBOBOXINFO     lpfnComboBoxInfo;   // USER32 GetComboBoxInfo()
LPFNGETANCESTOR         lpfnGetAncestor;    // USER32 GetAncestor()
LPFNREALCHILDWINDOWFROMPOINT    lpfnRealChildWindowFromPoint;   // USER32 RealChildWindowFromPoint
LPFNREALGETWINDOWCLASS  lpfnRealGetWindowClass; // USER32 RealGetWindowClass()
LPFNGETALTTABINFO       lpfnAltTabInfo;     // USER32 GetAltTabInfo()
LPFNGETLISTBOXINFO      lpfnGetListBoxInfo; // USER32 GetListBoxInfo()
LPFNGETMENUBARINFO      lpfnMenuBarInfo;    // USER32 GetMenuBarInfo()
LPFNSENDINPUT           lpfnSendInput;      // USER32 SendInput()
LPFNBLOCKINPUT          lpfnBlockInput;      // USER32 BlockInput()
LPFNMAPLS               lpfnMapLS;          // KERNEL32 MapLS()
LPFNUNMAPLS             lpfnUnMapLS;        // KERNEL32 UnMapLS()
LPFNGETMODULEFILENAME	lpfnGetModuleFileName;	// KERNEL32 GetModuleFileName()
LPFNINTERLOCKCMPEXCH    lpfnInterlockedCompareExchange;  // NT KERNEL32 InterlockedCompareExchange
LPFNVIRTUALALLOCEX      lpfnVirtualAllocEx; // NT KERNEL32 VirtualAllocEx
LPFNVIRTUALFREEEX       lpfnVirtualFreeEx;  // NT KERNEL32 VirtualFreeEx

// this prototype is in default.h, but we don't need all that, we just need this.
extern DWORD	MyGetModuleFileName(HMODULE hModule,LPTSTR lpFilename,DWORD nSize);

// ===========================================================================
//                   C O M   . D L L   E N T R Y   P O I N T S
// ===========================================================================
STDAPI DllRegisterServer();



// ---------------------------------------------------------------------------
// %%Function: DllMain
// ---------------------------------------------------------------------------
 BOOL WINAPI
DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID pvReserved)
{
    OSVERSIONINFO osvi;
    HINSTANCE hModule;

    UNUSED(pvReserved);

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        // check platform version information
        osvi.dwOSVersionInfoSize = sizeof(osvi);
        GetVersionEx(&osvi);

        // refuse to run on Win32s
        if (osvi.dwPlatformId == VER_PLATFORM_WIN32s)
            return FALSE;

#ifdef _X86_
        fWindows95 = (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);
#endif // _X86_

        hinstDll = hinst;

		// Load the resource-only DLL
		hinstResDll = LoadLibraryEx( TEXT("OLEACCRC.DLL"), NULL, LOAD_LIBRARY_AS_DATAFILE );
		if( ! hinstResDll )
		{
			// Refuse to load if oleaccrc isn't present
			return FALSE;
		}


        // Create the global mutex object used to ensure that 
        // the uniqueness value used by SharedBuffer_Allocate, which 
        // is used in NT path of LResultFromObject.
        g_hMutexUnique = CreateMutex(NULL,NULL,__TEXT("SMD.MSAA.UniqueVal.Henry"));


        // Basically what we are doing here is stuff that only needs to
        // be done the first time the DLL is loaded, which is to set up
        // the values for things in the shared data segment.
        //
        // InterlockedIncrement() and Decrement() return 1 if the result is 
        // positive, 0 if  zero, and -1 if negative.  Therefore, the only
        // way to use them practically is to start with a counter at -1.  
        // Then incrementing from -1 to 0, the first time, will give you
        // a unique value of 0.  And decrementing the last time from 0 to -1
        // will give you a unique value of -1.
        //
        if (InterlockedIncrement(&cProcessesMinus1) == 0)
        {
            // if we are running on Win95, create a shared heap to use when
            // communicating with controls in other processes
#ifdef _X86_
            if (fWindows95)
            {
                hheapShared = HeapCreate(HEAP_SHARED, 0, 0);
                if (hheapShared == NULL)
                    return(FALSE);
            }
#endif // _X86_
            
        }

        // everything that follows is done for every process attach.

        // do a LoadLibrary of OLEAUT32.DLL to ensure that it stays
        // loaded in every process that OLEACC is attched to.
        // There is a slight chance of a crash. From Paul Vick:
        // The problem occurs when oleaut32.dll is loaded into a process, 
        // used, unloaded and then loaded again. When you try to use 
        // oleaut32.dll in this case, you crash because of a problem 
        // manging state between us (oleaut32) and OLE. What happens is that when 
        // oleaut32.dll is loaded, we register some state with OLE so 
        // we are notified when OLE is uninitialized. In some cases 
        // (esp. multithreading), we are not able to clean up this 
        // state when we’re unloaded. When you reload oleaut32.dll, 
        // we try to set the state in OLE again and OLE tries to free 
        // the old (invalid) state, causing a crash later on.
        LoadLibrary (TEXT("OLEAUT32.DLL"));

        // set the flag that causes proxies to work.
        fCreateDefObjs = TRUE;

        // get the entry points to private functions in USER and KERNEL
        hModule = GetModuleHandle(TEXT("USER32.DLL"));
        lpfnGuiThreadInfo = (LPFNGETGUITHREADINFO)GetProcAddress(hModule, "GetGUIThreadInfo");

        // Attempt to get the NT4 version of GetCursorInfo ... called GetAccCursorInfo.
        lpfnCursorInfo = (LPFNGETCURSORINFO)GetProcAddress(hModule, "GetAccCursorInfo");
        if (!lpfnCursorInfo)
            // We must not be running NT4 SP6 or later...
            lpfnCursorInfo = (LPFNGETCURSORINFO)GetProcAddress(hModule, "GetCursorInfo");

        lpfnWindowInfo = (LPFNGETWINDOWINFO)GetProcAddress(hModule, "GetWindowInfo");
        lpfnTitleBarInfo = (LPFNGETTITLEBARINFO)GetProcAddress(hModule, "GetTitleBarInfo");
        lpfnScrollBarInfo = (LPFNGETSCROLLBARINFO)GetProcAddress(hModule, "GetScrollBarInfo");
        lpfnComboBoxInfo = (LPFNGETCOMBOBOXINFO)GetProcAddress(hModule, "GetComboBoxInfo");
        lpfnGetAncestor = (LPFNGETANCESTOR)GetProcAddress(hModule, "GetAncestor");
        lpfnRealChildWindowFromPoint = (LPFNREALCHILDWINDOWFROMPOINT)GetProcAddress(hModule, "RealChildWindowFromPoint");
        
#ifdef UNICODE
        lpfnRealGetWindowClass = (LPFNREALGETWINDOWCLASS)GetProcAddress(hModule, "RealGetWindowClassW");
#else
        lpfnRealGetWindowClass = (LPFNREALGETWINDOWCLASS)GetProcAddress(hModule, "RealGetWindowClass");
        if (!lpfnRealGetWindowClass)
            lpfnRealGetWindowClass = (LPFNREALGETWINDOWCLASS)GetProcAddress(hModule, "RealGetWindowClassA");
#endif

#ifdef UNICODE
        lpfnAltTabInfo = (LPFNGETALTTABINFO)GetProcAddress(hModule, "GetAltTabInfoW");
#else
        lpfnAltTabInfo = (LPFNGETALTTABINFO)GetProcAddress(hModule, "GetAltTabInfo");
        if (!lpfnAltTabInfo )
            lpfnAltTabInfo = (LPFNGETALTTABINFO)GetProcAddress(hModule, "GetAltTabInfoA");
#endif

        lpfnGetListBoxInfo = (LPFNGETLISTBOXINFO)GetProcAddress(hModule, "GetListBoxInfo");
        lpfnMenuBarInfo = (LPFNGETMENUBARINFO)GetProcAddress(hModule, "GetMenuBarInfo");
        lpfnSendInput = (LPFNSENDINPUT)GetProcAddress(hModule,"SendInput");
        lpfnBlockInput = (LPFNBLOCKINPUT)GetProcAddress(hModule,"BlockInput");

        hModule = GetModuleHandle(TEXT("KERNEL32.DLL"));
        lpfnMapLS = (LPFNMAPLS)GetProcAddress(hModule, "MapLS");
        lpfnUnMapLS = (LPFNUNMAPLS)GetProcAddress(hModule, "UnMapLS");
#ifdef UNICODE
		lpfnGetModuleFileName = (LPFNGETMODULEFILENAME)GetProcAddress(hModule, "GetModuleFileNameW");
#else // UNICODE
		lpfnGetModuleFileName = (LPFNGETMODULEFILENAME)GetProcAddress(hModule, "GetModuleFileNameA");
#endif // UNICODE
        lpfnInterlockedCompareExchange = (LPFNINTERLOCKCMPEXCH)GetProcAddress(hModule,"InterlockedCompareExchange");
        lpfnVirtualAllocEx = (LPFNVIRTUALALLOCEX)GetProcAddress(hModule,"VirtualAllocEx");
        lpfnVirtualFreeEx = (LPFNVIRTUALFREEEX)GetProcAddress(hModule,"VirtualFreeEx");

#ifdef _DEBUG

		// Initialize the new/delete checker...
		InitMemChk();

        TCHAR szErrorMsg[1024];
        TCHAR szNames[1024];
        TCHAR szTitle[255];
        LPVOID  lpVersionData;
        DWORD   dwSize;
        DWORD   dwUseless;
        VS_FIXEDFILEINFO    *lpVersionInfo;
        DWORD   dwBytes;

		// Output to debug terminal - oleacc was attached to process x,
		// show which oleacc version is running, what directory it was
		// loaded from, etc.
        MyGetModuleFileName (NULL,szTitle,ARRAYSIZE(szTitle));
        DBPRINTF (TEXT("'%s' is loading "),szTitle);

        MyGetModuleFileName (hinst,szTitle,ARRAYSIZE(szTitle));
        dwSize = GetFileVersionInfoSize(szTitle,&dwUseless);
        if (dwSize)
        {
            lpVersionData = LocalAlloc(LPTR,(UINT)dwSize);
            if (GetFileVersionInfo(szTitle,dwUseless,dwSize,lpVersionData))
            {
                VerQueryValue(lpVersionData,TEXT("\\"),(void**)&lpVersionInfo,(UINT*)&dwBytes);
                wsprintf (szNames,TEXT("%d.%d.%d.%d"),HIWORD(lpVersionInfo->dwFileVersionMS),
                                                LOWORD(lpVersionInfo->dwFileVersionMS),
                                                HIWORD(lpVersionInfo->dwFileVersionLS),
                                                LOWORD(lpVersionInfo->dwFileVersionLS));
            } // end we got version data
			LocalFree((HLOCAL)lpVersionData);
        }
		DBPRINTF (TEXT("%s version %s\r\n"),szTitle,szNames);

        // Here I am showing a dialog box showing the full module
        // name of the DLL (including path) and any special 
        // USER/KERNEL functions that were not found with GetProcAddress. 

        // only show the message box on the first load.
        if (cProcessesMinus1 == 0)
        {
            lstrcpy (szErrorMsg,TEXT("WARNING: the following functions were not found:\r\n"));
            lstrcpy (szNames,TEXT(""));
            if (lpfnGuiThreadInfo == NULL)
                lstrcat (szNames,TEXT("GetGUIThreadInfo\r\n"));
            if (lpfnCursorInfo == NULL)
                lstrcat (szNames,TEXT("GetCursorInfo\r\n"));
            if (lpfnWindowInfo == NULL)
                lstrcat (szNames,TEXT("GetWindowInfo\r\n"));
            if (lpfnTitleBarInfo == NULL)
                lstrcat (szNames,TEXT("GetTitleBarInfo\r\n"));
            if (lpfnScrollBarInfo == NULL)
                lstrcat (szNames,TEXT("GetScrollBarInfo\r\n"));
            if (lpfnComboBoxInfo == NULL)
                lstrcat (szNames,TEXT("GetComboBoxInfo\r\n"));
            if (lpfnGetAncestor == NULL)
                lstrcat (szNames,TEXT("GetAncestor\r\n"));
            if (lpfnRealChildWindowFromPoint == NULL)
                lstrcat (szNames,TEXT("RealChildWindowFromPoint\r\n"));
            if (lpfnRealGetWindowClass == NULL)
                lstrcat (szNames,TEXT("RealGetWindowClass\r\n"));
            if (lpfnAltTabInfo == NULL)
                lstrcat (szNames,TEXT("GetAltTabInfo\r\n"));
            if (lpfnGetListBoxInfo == NULL)
                lstrcat (szNames,TEXT("GetListBoxInfo\r\n"));
            if (lpfnMenuBarInfo == NULL)
                lstrcat (szNames,TEXT("GetMenuBarInfo\r\n"));
            if (lpfnSendInput == NULL)
                lstrcat (szNames,TEXT("SendInput\r\n"));
            if (lpfnBlockInput == NULL)
                lstrcat (szNames,TEXT("BlockInput\r\n"));
            if (lpfnMapLS == NULL)
                lstrcat (szNames,TEXT("MapLS\r\n"));
            if (lpfnUnMapLS == NULL)
                lstrcat (szNames,TEXT("UnMapLS\r\n"));
            if (lpfnGetModuleFileName == NULL)
                lstrcat (szNames,TEXT("GetModuleFileName\r\n"));
            // the next three functios are only on NT, and we know it,
            // so only warn if they are not on NT! (Shouldn't ever happen!)
#ifdef _X86_
            if ( !fWindows95 )
#endif // _X86_
            {
                if (lpfnInterlockedCompareExchange == NULL)
                    lstrcat (szNames,TEXT("InterlockedCompareExchange\r\n"));
                if (lpfnVirtualAllocEx == NULL)
                    lstrcat (szNames,TEXT("VirtualAllocEx\r\n"));
                if (lpfnVirtualFreeEx == NULL)
                    lstrcat (szNames,TEXT("VirtualFreeEx\r\n"));
            }
            if (*szNames)
            {
                MyGetModuleFileName (hinst,szTitle,ARRAYSIZE(szTitle));
                lstrcat (szErrorMsg,szNames);
                MessageBeep (MB_ICONEXCLAMATION);
                MessageBox (NULL,szErrorMsg,szTitle,MB_OK|MB_ICONEXCLAMATION);
				DBPRINTF (szErrorMsg);
            }
        } // end if cProcessesMinus1 == 0

#endif
        InitWindowClasses();

        DisableThreadLibraryCalls(hinst);   // remove if DLL_THREAD_XXX notifications
                                            //  must be monitored.
        
        // Since Office 97 can screw up the registration, 
        // we'll call our self-registration function.
        // may be slight perf hit on load, but not that big a deal,
        // and this way we know we are always correctly registered..
        DllRegisterServer();
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
		// Release the resource DLL...
		if( hinstResDll )
			FreeLibrary( hinstResDll );

#ifdef _DEBUG

		// This reports the number of outstanding delete's,
		// if any, via DBPRINTF...
		UninitMemChk();

        TCHAR szNames[64];
        TCHAR szTitle[255];
        LPVOID  lpVersionData;
        DWORD   dwSize;
        DWORD   dwUseless;
        VS_FIXEDFILEINFO    *lpVersionInfo;
        DWORD   dwBytes;

		// Output to debug terminal - oleacc was detached from process x,
		// show which oleacc version is running, what directory it was
		// loaded from, etc.
        MyGetModuleFileName (NULL,szTitle,ARRAYSIZE(szTitle));
        DBPRINTF (TEXT("'%s' is unloading "),szTitle);

        MyGetModuleFileName (hinst,szTitle,ARRAYSIZE(szTitle));
        dwSize = GetFileVersionInfoSize(szTitle,&dwUseless);
        if (dwSize)
        {
            lpVersionData = LocalAlloc(LPTR,(UINT)dwSize);
            if (GetFileVersionInfo(szTitle,dwUseless,dwSize,lpVersionData))
            {
                VerQueryValue(lpVersionData,TEXT("\\"),(void**)&lpVersionInfo,(UINT*)&dwBytes);
                wsprintf (szNames,TEXT("%d.%d.%d.%d"),HIWORD(lpVersionInfo->dwFileVersionMS),
                                                LOWORD(lpVersionInfo->dwFileVersionMS),
                                                HIWORD(lpVersionInfo->dwFileVersionLS),
                                                LOWORD(lpVersionInfo->dwFileVersionLS));
            } // end we got version data
			LocalFree((HLOCAL)lpVersionData);
        }
		DBPRINTF (TEXT("%s version %s\r\n"),szTitle,szNames);
#endif //_DEBUG

        if (g_hMutexUnique)
        {
            CloseHandle(g_hMutexUnique);
        }

        // stuff that needs to be cleaned up on the last detach - clean
        // up stuff in the shared data segment.
        if (InterlockedDecrement(&cProcessesMinus1) == -1)
        {

#ifdef _X86_
            if (fWindows95)
            {
                if (hheapShared)
                    HeapDestroy(hheapShared);
                hheapShared = NULL;
            }
			UnInitWindowClasses();
#endif // _X86_
        }
    }

    return TRUE;
} 


// ===========================================================================
//                       S E L F   R E G I S T R A T I O N
// ===========================================================================

#define LENGTH_OF(sz)				(sizeof(sz)/sizeof(TCHAR))
#define TYPELIB_MAJORVER			1
#define TYPELIB_MINORVER			1

const TCHAR   szInterfaceRoot[]      = TEXT("Interface\\{618736E0-3C3D-11CF-810C-00AA00389B71}");
const TCHAR   szTypeLib[]            = TEXT("TypeLib");
const TCHAR   szTypeLibKey[]         = TEXT("Interface\\{618736E0-3C3D-11CF-810C-00AA00389B71}\\Typelib");
const TCHAR   szLibidOLEACC[]        = TEXT("{1EA4DBF0-3C3B-11CF-810C-00AA00389B71}");
const TCHAR   szLibidOff97[]         = TEXT("{2DF8D04C-5BFA-101B-BDE5-00AA0044DE52}");
const TCHAR   szCommandBarKey[]      = TEXT("Interface\\{000C0302-0000-0000-C000-000000000046}");
const TCHAR   szVersion2[]           = TEXT("2.0");

// ---------------------------------------------------------------------------
// %%Function: DllRegisterServer
// ---------------------------------------------------------------------------
STDAPI DllRegisterServer()
{
	ITypeLib	*pTypeLib = NULL;
	HRESULT		hr;
    OLECHAR		wszOleAcc[] = L"oleacc.dll";


	hr = LoadTypeLib( wszOleAcc, &pTypeLib );

	if ( SUCCEEDED(hr) )
	{
		hr = RegisterTypeLib( pTypeLib, wszOleAcc, NULL );
		if ( FAILED(hr) )
			DBPRINTF (TEXT("OLEACC: DllRegisterServer could not register type library hr=%lX\r\n"),hr);
		pTypeLib->Release();
	}
	else
	{
		DBPRINTF (TEXT("OLEACC: DllRegisterServer could not load type library hr=%lX\r\n"),hr);
	}

    return S_OK;
}  // DllRegisterServer

// ---------------------------------------------------------------------------
// %%Function: DllUnregisterServer
// ---------------------------------------------------------------------------
STDAPI DllUnregisterServer()
{
	//------------------------------------------------
	// The major/minor typelib version number determine
	//	which regisered version of OLEACC.DLL will get
	//	unregistered.
	//------------------------------------------------

	return UnRegisterTypeLib( LIBID_Accessibility, TYPELIB_MAJORVER, TYPELIB_MINORVER, 0, SYS_WIN32 );

}  // DllUnregisterServer
 



