// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  CLIENT.H
//
//  Default window client OLE accessible object class
//
// --------------------------------------------------------------------------


class CClient : public CAccessible
{
    public:
        // IAccessible
        virtual STDMETHODIMP        get_accChildCount(long * pcCount);

        virtual STDMETHODIMP        get_accName(VARIANT, BSTR*);
        virtual STDMETHODIMP        get_accRole(VARIANT, VARIANT*);
        virtual STDMETHODIMP        get_accState(VARIANT, VARIANT*);
        virtual STDMETHODIMP        get_accKeyboardShortcut(VARIANT, BSTR*);
        virtual STDMETHODIMP        get_accFocus(VARIANT * pvarFocus);

        virtual STDMETHODIMP        accLocation(long* pxLeft, long* pyTop,
            long *pcxWidth, long *pcyHeight, VARIANT varChild);
        virtual STDMETHODIMP        accNavigate(long dwNavDir, VARIANT varStart, VARIANT *pvarEnd);
        virtual STDMETHODIMP        accHitTest(long xLeft, long yTop, VARIANT *pvarHit);

        // IEnumVARIANT
        virtual STDMETHODIMP        Next(ULONG celt, VARIANT *rgelt, ULONG *pceltFetched);
        virtual STDMETHODIMP        Skip(ULONG celt);
        virtual STDMETHODIMP        Reset(void);
        virtual STDMETHODIMP        Clone(IEnumVARIANT **);

        void    Initialize(HWND hwnd, long idCurChild);
        BOOL    ValidateHwnd(VARIANT* pvar);

    protected:
        BOOL    m_fUseLabel;
};


extern HRESULT CreateClient(HWND hwnd, long idChild, REFIID riid, void** ppvObject);


//
// We need a way for HWND and non-HWND children to live in the same
// namespace together.  Since children pass up peer-to-peer navigation to
// their parent, we need a way for HWND children to identify themselves in
// the navigate call.  Since HWND children are always objects, it is fine
// for the client parent to not accept HWND ids in all other methods.  One
// can do it, but it is a lot of work.
//
// Examples to date of mixed:
//      (1) Comboboxes (dropdown always a window, cur item may or may not be,
//          button never is)
//      (2) Toolbars (dropdown is a window, buttons aren't)
//
// We want the client manager to handle IEnumVARIANT, validation, etc.
//

#ifdef _WIN64

// BUGBUG BOGUS HACK
// Using PtrToUlong lust to keep the compile happy.
// The code WILL NOT WORK under win64, as we don't know if
// childids should be 32 or 64 bit - currently leaving as 32 bit, but
// lose pointers...
#define HWNDIDFromHwnd(hwnd)        (PtrToUlong(hwnd) | 0x80000000)
#define HwndFromHWNDID(lId)         (HWND)((lId) & ~0x80000000)

#else

#define HWNDIDFromHwnd(hwnd)        ((DWORD)(hwnd) | 0x80000000)
#define HwndFromHWNDID(lId)         (HWND)((lId) & ~0x80000000)

#endif // _WIN64


#define IsHWNDID(lId)               ((lId) & 0x80000000)



//
// When enumerating, we loop through non-hwnd items first, then hwnd-children.
//
extern TCHAR    StripMnemonic(LPTSTR lpsz);
extern LPTSTR   GetTextString(HWND, BOOL);
extern LPTSTR   GetLabelString(HWND);
extern HRESULT  HrGetWindowName(HWND, BOOL, BSTR*);
extern HRESULT  HrGetWindowShortcut(HWND, BOOL, BSTR*);
extern HRESULT  HrMakeShortcut(LPTSTR, BSTR*);

#define SHELL_TRAY      1
#define SHELL_DESKTOP   2
#define SHELL_PROCESS   3
extern BOOL     InTheShell(HWND, int);

extern BOOL     IsComboEx(HWND);
extern HWND     IsInComboEx(HWND);

