//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       utils.h
//
//--------------------------------------------------------------------------

#ifndef _INC_CSCVIEW_UTILS_H
#define _INC_CSCVIEW_UTILS_H

#ifndef _INC_CSCVIEW_STRCLASS_H
#   include "strclass.h"
#endif
 
#ifndef ARRAYSIZE
#   define ARRAYSIZE(a)   (sizeof(a) / sizeof((a)[0]))
#endif

//
// Create a ULONGLONG from two DWORDs.
//
inline ULONGLONG 
MAKEULONGLONG(
    DWORD dwHighPart,
    DWORD dwLowPart
    ) throw()
{
    return (ULONGLONG)(((ULONGLONG)(dwHighPart) << 32) | (ULONGLONG)dwLowPart);
}

//
// Convert a value to a "bool".
// Lower-case "boolify" is intentional to enforce relationship
// to type "bool".
//
template <class T>
inline bool boolify(const T& x)
{
    return !!x;
}


template <class T>
void
SWAP(T& a, T& b)
{
    T temp(a);
    a = b;
    b = temp;
}

  
//-----------------------------------------------------------------------------
// class FindHandle
//-----------------------------------------------------------------------------
//
// Trivial class to ensure cleanup of FindFirst/FindNext handle.
//
class FindHandle
{
    public:
        explicit FindHandle(HANDLE hFind) throw()
            : m_hFind(hFind) { }

        ~FindHandle(void) throw()
            { 
                if (IsValidHandle()) 
                    FindClose(m_hFind);
            }

        operator HANDLE() const throw()
            { return m_hFind; }

        bool IsValidHandle(void) const throw()
            { return (INVALID_HANDLE_VALUE != m_hFind); }

        HANDLE Handle(void) const throw()
            { return m_hFind; }

    private:
        HANDLE m_hFind;
};


//-----------------------------------------------------------------------------
// class LoadedLibrary
//-----------------------------------------------------------------------------
//
// Trivial class to ensure cleanup of LoadLibrary handle.
//
class LoadedLibrary
{
    public:
        explicit LoadedLibrary(LPCTSTR pszModule) throw()
            : m_hModule(::LoadLibrary(pszModule)) { }

        ~LoadedLibrary(void) throw()
            { if (IsLoaded()) ::FreeLibrary(m_hModule); }

        operator HMODULE() throw()
            { return m_hModule; }

        bool IsLoaded(void) throw()
            { return (NULL != m_hModule); }

        FARPROC GetProcAddress(LPCSTR pszProcName) throw()
            { return ::GetProcAddress(m_hModule, pszProcName); }

    private:
        HMODULE m_hModule;
};


class DblNulTermListIter
{
    public:
        explicit DblNulTermListIter(LPCTSTR pszList = NULL) throw()
            : m_pszList(pszList),
              m_pszCurrent(pszList) { }

        void Attach(LPCTSTR pszList) throw()
            { m_pszList = m_pszCurrent = pszList; }

        bool Next(LPCTSTR *ppszItem) throw();

        void Reset(void) throw()
            { m_pszCurrent = m_pszList; }

    private:
        LPCTSTR m_pszList;
        LPCTSTR m_pszCurrent;
};

//
// Simple class for automating the display and resetting of a wait cursor.
//
class CAutoWaitCursor
{
    public:
        CAutoWaitCursor(void)
            : m_hCursor(SetCursor(LoadCursor(NULL, IDC_WAIT))) 
            { ShowCursor(TRUE); }

        ~CAutoWaitCursor(void)
            { Reset(); }

        void Reset(void);

    private:
        HCURSOR m_hCursor;
};


//
// Trivial class for ensuring window is enabled in the case
// of an exception.
//
class CAutoWndEnable
{
    public:
        CAutoWndEnable(HWND hwnd) throw()
            : m_hwnd(hwnd) { }

        CAutoWndEnable(HWND hwnd, bool bEnable) throw()
            : m_hwnd(hwnd) { Enable(bEnable); }

        ~CAutoWndEnable(void) throw()
            { Enable(true); }

        void Enable(bool bEnable) throw()
            { EnableWindow(m_hwnd, bEnable); }

    private:
        HWND m_hwnd;
};

class CAutoSetRedraw
{
    public:
        CAutoSetRedraw(HWND hwnd)
            : m_hwnd(hwnd) { }

        CAutoSetRedraw(HWND hwnd, bool bSet)
            : m_hwnd(hwnd) { Set(bSet); }

        ~CAutoSetRedraw(void)
            { Set(true); }

        void Set(bool bSet)
            { SendMessage(m_hwnd, WM_SETREDRAW, (WPARAM)bSet, 0); }

    private:
        HWND m_hwnd;
};


void CenterPopupWindow(HWND hwnd, HWND hwndParent = NULL);
bool UseWindowsHelp(int idCtl);
bool GetShellItemDisplayName(LPITEMIDLIST pidl, CString *pstrName);
void NotYetImplemented(HWND hwndParent = NULL);

#endif // _INC_CSCVIEW_UTILS_H

