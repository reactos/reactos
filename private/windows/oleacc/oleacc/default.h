// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  DEFAULT.H
//
//  Standard OLE accessible object class
//
// --------------------------------------------------------------------------

class   CAccessible :  public  IAccessible, public IEnumVARIANT, public IOleWindow
{
    public:

		// Virtual dtor ensures that dtors of derived classes
		// are called correctly when objects are deleted
		virtual ~CAccessible();

        // IUnknown
        virtual STDMETHODIMP            QueryInterface(REFIID riid, void** ppv);
        virtual STDMETHODIMP_(ULONG)    AddRef(void);
        virtual STDMETHODIMP_(ULONG)    Release(void);

        // IDispatch
        virtual STDMETHODIMP            GetTypeInfoCount(UINT* pctinfo);
        virtual STDMETHODIMP            GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
        virtual STDMETHODIMP            GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames,
            LCID lcid, DISPID* rgdispid);
        virtual STDMETHODIMP            Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
            DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo,
            UINT* puArgErr);

        // IAccessible
        virtual STDMETHODIMP            get_accParent(IDispatch ** ppdispParent);
        virtual STDMETHODIMP            get_accChildCount(long* pChildCount);
        virtual STDMETHODIMP            get_accChild(VARIANT varChild, IDispatch ** ppdispChild);

        virtual STDMETHODIMP            get_accName(VARIANT varChild, BSTR* pszName) = 0;
        virtual STDMETHODIMP            get_accValue(VARIANT varChild, BSTR* pszValue);
        virtual STDMETHODIMP            get_accDescription(VARIANT varChild, BSTR* pszDescription);
        virtual STDMETHODIMP            get_accRole(VARIANT varChild, VARIANT *pvarRole) = 0;
        virtual STDMETHODIMP            get_accState(VARIANT varChild, VARIANT *pvarState) = 0;
        virtual STDMETHODIMP            get_accHelp(VARIANT varChild, BSTR* pszHelp);
        virtual STDMETHODIMP            get_accHelpTopic(BSTR* pszHelpFile, VARIANT varChild, long* pidTopic);
        virtual STDMETHODIMP            get_accKeyboardShortcut(VARIANT varChild, BSTR* pszKeyboardShortcut);
        virtual STDMETHODIMP			get_accFocus(VARIANT * pvarFocusChild);
        virtual STDMETHODIMP			get_accSelection(VARIANT * pvarSelectedChildren);
        virtual STDMETHODIMP			get_accDefaultAction(VARIANT varChild, BSTR* pszDefaultAction);

        virtual STDMETHODIMP			accSelect(long flagsSel, VARIANT varChild);
        virtual STDMETHODIMP			accLocation(long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild) = 0;
        virtual STDMETHODIMP			accNavigate(long navDir, VARIANT varStart, VARIANT * pvarEndUpAt);
        virtual STDMETHODIMP			accHitTest(long xLeft, long yTop, VARIANT * pvarChildAtPoint) = 0;
        virtual STDMETHODIMP			accDoDefaultAction(VARIANT varChild);

        virtual STDMETHODIMP			put_accName(VARIANT varChild, BSTR szName);
        virtual STDMETHODIMP			put_accValue(VARIANT varChild, BSTR pszValue);

        // IEnumVARIANT
        virtual STDMETHODIMP            Next(ULONG celt, VARIANT* rgvar, ULONG * pceltFetched);
        virtual STDMETHODIMP            Skip(ULONG celt);
        virtual STDMETHODIMP            Reset(void);
        virtual STDMETHODIMP            Clone(IEnumVARIANT ** ppenum) = 0;

        // IOleWindow
        virtual STDMETHODIMP            GetWindow(HWND* phwnd);
        virtual STDMETHODIMP            ContextSensitiveHelp(BOOL fEnterMode);

        virtual void SetupChildren(void);
        virtual BOOL ValidateChild(VARIANT*);

        HRESULT InitTypeInfo(void);
        void    TermTypeInfo(void);

    protected:
        HWND        m_hwnd;
        ULONG       m_cRef;
        long        m_cChildren;        // Count of index-based children
        long        m_idChildCur;       // ID of current child in enum (may be index or hwnd based)
        ITypeInfo*  m_pTypeInfo;        // TypeInfo for IDispatch junk
};



//
// Validate and initialization macros
//
#define ValidateFlags(flags, valid)         (!((flags) & ~(valid)))
#define ValidateRange(lValue, lMin, lMax)   (((lValue) > (lMin)) && ((lValue) < (lMax)))
        
BOOL ValidateNavDir(long lFlags, long idChild);
BOOL ValidateSelFlags(long flags);


#define InitPv(pv)              *pv = NULL
#define InitPlong(plong)        *plong = 0
#define InitPvar(pvar)           pvar->vt = VT_EMPTY
#define InitAccLocation(px, py, pcx, pcy)   {InitPlong(px); InitPlong(py); InitPlong(pcx); InitPlong(pcy);}



#define UNUSED(param)   (param)

#define CCH_STRING_MAX  256

#define E_NOT_APPLICABLE            DISP_E_MEMBERNOTFOUND

#define ChildFromIndex(index)       (1 << ((index)-1))
#define ChildPresent(mask, index)   (((mask) & ChildFromIndex(index)) != 0)

// defines used for SendKey function
#define KEYPRESS    0
#define KEYRELEASE  KEYEVENTF_KEYUP
#define VK_VIRTUAL  0
#define VK_CHAR     1

//
// Helper functions
//

extern BSTR     TCharSysAllocString(LPTSTR lpszString);
extern HRESULT  _GetTypeInfo(LCID lcid, UUID uuid, ITypeInfo **ppITypeInfo);
extern BOOL     ClickOnTheRect(LPRECT lprcLoc,HWND hwndToCheck,BOOL fDblClick);
extern BOOL     SendKey (int nEvent,int nKeyType,WORD wKeyCode,TCHAR cChar);

//
// For GetProcAddress() in USER and KERNEL32
//
extern BOOL     MyGetGUIThreadInfo(DWORD, PGUITHREADINFO);
extern BOOL     MyGetCursorInfo(LPCURSORINFO);
extern BOOL     MyGetWindowInfo(HWND, LPWINDOWINFO);
extern BOOL     MyGetTitleBarInfo(HWND, LPTITLEBARINFO);
extern BOOL     MyGetScrollBarInfo(HWND, LONG, LPSCROLLBARINFO);
extern BOOL     MyGetComboBoxInfo(HWND, LPCOMBOBOXINFO);
extern BOOL     MyGetAltTabInfo(HWND, int, LPALTTABINFO, LPTSTR, UINT);
extern BOOL     MyGetMenuBarInfo(HWND, long, long, LPMENUBARINFO);
extern HWND     MyGetAncestor(HWND, UINT);
extern HWND     MyGetFocus(void);
extern HWND     MyRealChildWindowFromPoint(HWND, POINT);
extern UINT     MyGetWindowClass(HWND, LPTSTR, UINT);
extern DWORD    MyGetListBoxInfo(HWND);
extern void     MyGetRect(HWND, LPRECT, BOOL);
extern DWORD	MyGetModuleFileName(HMODULE hModule,LPTSTR lpFilename,DWORD nSize);
extern PVOID    MyInterlockedCompareExchange(PVOID *,PVOID,PVOID);
extern LPVOID   MyVirtualAllocEx(HANDLE,LPVOID,DWORD,DWORD,DWORD);
extern BOOL     MyVirtualFreeEx(HANDLE,LPVOID,DWORD,DWORD);
extern BOOL     MyBlockInput(BOOL);
extern BOOL		MySendInput(UINT cInputs, LPINPUT pInputs, INT cbSize);

extern HRESULT  HrCreateString(int istr, BSTR* pszResult);

extern HRESULT  GetWindowObject(HWND hwndChild, VARIANT* lpvar);
extern HRESULT  GetNoncObject(HWND hwndFrame, LONG idObject, VARIANT* lpvar);
extern HRESULT  GetParentToNavigate(long, HWND, long, long, VARIANT*);


typedef HRESULT (* LPFNCREATE)(HWND, long, REFIID, void**);

//-----------------------------------------------------------------
// [v-jaycl, 4/2/97] Expanded rgAtomClasses for registered
//	 handlers.  TODO: Should make array dynamic.
//-----------------------------------------------------------------
extern ATOM         rgAtomClasses[CSTR_CLIENT_CLASSES + TOTAL_REG_HANDLERS];
extern LPFNCREATE   rgClientTypes[CSTR_CLIENT_CLASSES];
extern LPFNCREATE   rgWindowTypes[CSTR_WINDOW_CLASSES];
extern LPTSTR		rgClassNames[CSTR_CLIENT_CLASSES];

extern BOOL CompareWindowClass( HWND hWnd, LPFNCREATE pfnQuery );

extern HRESULT FindAndCreateWindowClass( HWND        hWnd,
                                         BOOL        fWindow,
                                         LPFNCREATE  pfnDefault,
                                         REFIID      riid,
                                         long        idObject,
                                         void **     ppvObject );


typedef struct tagREGTYPEINFO
{
    CLSID   clsid;  // CLSID for this registered handler
    BOOL    bOK;    // used if there is an error - set to false if so
    TCHAR   DllName[MAX_PATH];
    TCHAR   ClassName[MAX_PATH];
    LPVOID  pClassFactory;
} REGTYPEINFO;

//
// Creation stuff
//
extern HRESULT CreateCaretObject(HWND, long, REFIID, void**);
extern HRESULT CreateClientObject(HWND, long, REFIID, void**);
extern HRESULT CreateCursorObject(HWND, long, REFIID, void**);
extern HRESULT CreateMenuBarObject(HWND, long, REFIID, void**);
extern HRESULT CreateScrollBarObject(HWND, long, REFIID, void**);
extern HRESULT CreateSizeGripObject(HWND, long, REFIID, void**);
extern HRESULT CreateSysMenuBarObject(HWND, long, REFIID, void**);
extern HRESULT CreateTitleBarObject(HWND, long, REFIID, void**);
extern HRESULT CreateWindowObject(HWND, long, REFIID, void**);


//
// Client types
//

// SYSTEM
extern HRESULT CreateButtonClient(HWND, long, REFIID, void**);
extern HRESULT CreateComboClient(HWND, long, REFIID, void**);
extern HRESULT CreateDialogClient(HWND, long, REFIID, void**);
extern HRESULT CreateDesktopClient(HWND, long, REFIID, void**);
extern HRESULT CreateEditClient(HWND, long, REFIID, void**);
extern HRESULT CreateListBoxClient(HWND, long, REFIID, void**);
extern HRESULT CreateMDIClient(HWND, long, REFIID, void**);
extern HRESULT CreateMenuPopupClient(HWND, long, REFIID, void**);
extern HRESULT CreateScrollBarClient(HWND, long, REFIID, void**);
extern HRESULT CreateStaticClient(HWND, long, REFIID, void**);
extern HRESULT CreateSwitchClient(HWND, long, REFIID, void**);

// COMCTL32
extern HRESULT CreateStatusBarClient(HWND, long, REFIID, void**);
extern HRESULT CreateToolBarClient(HWND, long, REFIID, void**);
extern HRESULT CreateProgressBarClient(HWND, long, REFIID, void**);
extern HRESULT CreateAnimatedClient(HWND, long, REFIID, void**);
extern HRESULT CreateTabControlClient(HWND, long, REFIID, void**);
extern HRESULT CreateHotKeyClient(HWND, long, REFIID, void**);
extern HRESULT CreateHeaderClient(HWND, long, REFIID, void**);
extern HRESULT CreateSliderClient(HWND, long, REFIID, void**);
extern HRESULT CreateListViewClient(HWND, long, REFIID, void**);
extern HRESULT CreateUpDownClient(HWND, long, REFIID, void**);
extern HRESULT CreateToolTipsClient(HWND, long, REFIID, void**);
extern HRESULT CreateTreeViewClient(HWND, long, REFIID, void**);
extern HRESULT CreateCalendarClient(HWND, long, REFIID, void**);
extern HRESULT CreateDatePickerClient(HWND, long, REFIID, void**);
#ifdef _X86_
extern HRESULT CreateHtmlClient(HWND, long, REFIID, void**);
#endif

// SDM32
extern HRESULT CreateSdmClientA(HWND, long, REFIID, void**);


//
// Window types
//
extern HRESULT CreateListBoxWindow(HWND, long, REFIID, void**);
extern HRESULT CreateMenuPopupWindow(HWND, long, REFIID, void**);

#include "w95trace.h"
