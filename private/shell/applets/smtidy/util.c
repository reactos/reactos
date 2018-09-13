//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
#include "smtidy.h"
#include "util.h"

#define INITGUID
#include <initguid.h>

#pragma data_seg(".text")
#include <coguid.h>
#include <oleguid.h>
#pragma data_seg()

//----------------------------------------------------------------------------
BOOL Cursor_Wait(HCURSOR *phcur)
{
    Assert(phcur);

    ShowCursor(TRUE);
    *phcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
    
    return TRUE;
}

//----------------------------------------------------------------------------
BOOL Cursor_UnWait(HCURSOR hCur)
{
    Assert(hCur);

    SetCursor(hCur);
    ShowCursor(FALSE);
    
    return TRUE;
}

//----------------------------------------------------------------------------
HRESULT PASCAL ICoCreateInstance(REFCLSID rclsid, REFIID riid, LPVOID FAR* ppv)
{
    LPCLASSFACTORY pcf;
    HRESULT hres = SHDllGetClassObject(rclsid, &IID_IClassFactory, &pcf);
    if (SUCCEEDED(hres))
    {
	hres = pcf->lpVtbl->CreateInstance(pcf, NULL, riid, ppv);
	pcf->lpVtbl->Release(pcf);
    }
    return hres;
}

//----------------------------------------------------------------------------
// Alloc a copy of the given string.
BOOL WINAPI Sz_AllocCopy(LPCTSTR pszSrc, LPTSTR *ppszDst)
{
    BOOL fRet = FALSE;
    
    if (pszSrc && ppszDst)
    {
        if (LAlloc(Sz_Cb(pszSrc), ppszDst))
        {
            lstrcpy(*ppszDst, pszSrc);
            fRet = TRUE;
        }
    }
        
    return fRet;
}

#ifdef DEBUG

//----------------------------------------------------------------------------
void WINAPI AssertFailed(LPCTSTR pszFile, int line)
{
    LPCTSTR psz;

    for (psz = pszFile+lstrlen(pszFile); psz!=pszFile; psz=AnsiPrev(pszFile, psz))
    {
        if ((AnsiPrev(pszFile, psz)!= (psz-2)) && *(psz - 1) == '\\')
        break;
    }

    Dbg(TEXT("Assertion failed in %s on line %d."), psz, line);

    DebugBreak();
}

//----------------------------------------------------------------------------
void __cdecl _Dbg(LPCSTR pszMsg, ...)
{
    char sz[1024];
    va_list ArgList;

    va_start (ArgList, pszMsg);    
    wvsprintf(sz, pszMsg, ArgList);
    va_end (ArgList);
    OutputDebugString(sz);
    OutputDebugString("\r\n");
}

#else

#ifndef NO_LOGGING
//----------------------------------------------------------------------------
HANDLE g_hFile = INVALID_HANDLE_VALUE;

//----------------------------------------------------------------------------
BOOL WINAPI _Dbg_OpenLog(void)
{
    BOOL fRet = FALSE;
    
    if (g_hFile == INVALID_HANDLE_VALUE)
    {   
        TCHAR sz[MAX_PATH];
        GetWindowsDirectory(sz, ARRAYSIZE(sz));
        PathAppend(sz, TEXT("smwiz.log"));
        g_hFile = CreateFile(sz, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (g_hFile != INVALID_HANDLE_VALUE)
            fRet = TRUE;
    }

    return fRet;
}

//----------------------------------------------------------------------------
BOOL WINAPI _Dbg_CloseLog(void)
{
    BOOL fRet = FALSE;
    
    if (g_hFile != INVALID_HANDLE_VALUE)
    {   
        CloseHandle(g_hFile);
        g_hFile = INVALID_HANDLE_VALUE;
        fRet = TRUE;
    }

    return fRet;
}

//----------------------------------------------------------------------------
void __cdecl _Dbg(LPCTSTR pszMsg, ...)
{
    TCHAR sz[1024];
    va_list ArgList;

    va_start (ArgList, pszMsg);    
    wvsprintf(sz, pszMsg, ArgList);
    va_end (ArgList);
    lstrcat(sz, TEXT("\r\n"));

    if (g_hFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwWritten;
        WriteFile(g_hFile, sz, lstrlen(sz), &dwWritten, NULL);
    }
}
#endif
#endif
