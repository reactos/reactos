/* Copyright 1996 Microsoft */

#ifndef _AUTOCOMP_HPP_
#define _AUTOCOMP_HPP_

#include "accdel.h"
#include "asuggest.h"

// TODO List:
// 1. Convert AutoComplete to be a Free Threaded object and move it and all it's
//    lists into MultiApartment model.
// 2. Get thread out of ThreadPool API in shlwapi.dll instead of creating thead our
//    selves.
// 3. See if SHWaitForSendMessageThread() will cause a lock if bg thread is in SendMessage().
//    If so, make sure the bg loop's hard loop doesn't call SendMessage() in this case without
//    first looking for QUIT message.



// WARNING On Usage:
//   This object is marked Apartment model and this abuses COM.  These are rules 
// that need to be followed to prevent bugs.  This object will be used within three scopes.  
// The first scope is the caller doing: 1a) CoInitialize(), 1b) CoCreateInstance(). 
// 1c) p->Init(), 1d) p->Release(), 1e) CoUninitialize().
// The second scope is the object doing: 2a) Subclass();AddRef(), 2b) WM_DESTROY;Release();
// 1c) p->Init(), 1d) p->Release(), 1e) CoUninitialize().
// The third scope is the background thread doing: 3a) (in thread proc) CoInitialize(), 
// 3b) CoUninitialize(). 
// This object requires that 1E come after 2B and that should be the only requirement
// for the use of this object.


//
// PRIVATE
//
#define AC_LIST_GROWTH_CONST         50
const WCHAR CH_WILDCARD = L'\1';    // indicates a wildcard search

//
// Debug Flags
//
#define AC_WARNING          TF_WARNING + TF_AUTOCOMPLETE
#define AC_ERROR            TF_ERROR   + TF_AUTOCOMPLETE
#define AC_GENERAL          TF_GENERAL + TF_AUTOCOMPLETE
#define AC_FUNC             TF_FUNC    + TF_AUTOCOMPLETE

// Enable test regkey
#define ALLOW_ALWAYS_DROP_UP

//
// WndProc messages to the dropdown window
//
enum
{
    AM_BUTTONCLICK = WM_APP + 400,
    AM_UPDATESCROLLPOS,
};


#define ACO_UNINITIALIZED       0x80000000    // if autocomplete options have not been initialized

//
// PUBLIC
//
HRESULT SHUseDefaultAutoComplete(HWND hwndEdit, 
                               IBrowserService * pbs,       IN  OPTIONAL
                               IAutoComplete2 ** ppac,      OUT OPTIONAL
                               IShellService ** ppssACLISF, OUT OPTIONAL
                               BOOL fUseCMDMRU);

// Forward references
class CAutoComplete;
class CACString* CreateACString(LPCWSTR pszStr);

//+-------------------------------------------------------------------------
// CACString - Autocomplete string shared by foreground & background threads
//--------------------------------------------------------------------------
class CACString
{
public:
    ULONG AddRef();
    ULONG Release();
    LPCWSTR GetStr() const { return m_sz; }
    LPCWSTR GetStrToCompare() const { return m_sz + m_iIgnore; }
    int GetLength() const { return m_cChars; }
    int GetLengthToCompare() const { return m_cChars - m_iIgnore; }
    const WCHAR& operator [] (int nIndex) const { return m_sz[nIndex]; }
    operator LPCWSTR() { return m_sz; }
    BOOL HasPrefix() { return m_iIgnore; }
    BOOL PrefixLength() { return m_iIgnore; }

    // Note, the following compare functions ignore the prefix of the CACString
    int StrCmpI(LPCWSTR psz) { return ::StrCmpI(m_sz + m_iIgnore, psz); }
    int StrCmpI(CACString& r) { return ::StrCmpI(m_sz + m_iIgnore, r.m_sz + r.m_iIgnore); }
    int StrCmpNI(LPCWSTR psz, int cch) { return ::StrCmpNI(m_sz + m_iIgnore, psz, cch); }

protected:
    friend CACString* CreateACString(LPCWSTR pszStr, int iIgnore);

    // Prevent creation on stack
    CACString();

    // Member variables
    ULONG m_dwRefCount;  // reference count
    int   m_cChars;      // length of string (excluding null)
    int   m_iIgnore;     // # prefix characters to ignore when comparing strings
    WCHAR m_sz[1];       // first character of the string
};

//+-------------------------------------------------------------------------
// CACThread - Autocomplete thread that runs in the background
//--------------------------------------------------------------------------
class CACThread : public IUnknown
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

                CACThread(CAutoComplete& rAutoComp);
               ~CACThread();

    BOOL        Init(IEnumString* pes, IACList* pacl);

    void        GotFocus(); 
    void        LostFocus();
    BOOL        HasFocus() { return m_fWorkItemQueued != 0; }
    BOOL        StartSearch(LPCWSTR pszSearch, DWORD dwOptions);
    void        StopSearch();
    void        SyncShutDownBGThread();
    BOOL        IsDisabled() { return m_fDisabled; }

    // Helper functions
    static BOOL MatchesSpecialPrefix(LPCWSTR pszSearch);
    static int  GetSpecialPrefixLen(LPCWSTR psz);

protected:
    DWORD           m_dwRefCount;
    CAutoComplete*  m_pAutoComp;     // portion of autocomplete that runs on main thread
    LONG            m_fWorkItemQueued; // if request made to shlwapi thread pool
    LONG            m_idThread;
    HANDLE          m_hCreateEvent;  // thread startup syncronizatrion
    BOOL            m_fDisabled:1;   // is autocomplete disabled?
    LPWSTR          m_pszSearch;     // String we are currently searching for
    HDPA            m_hdpa_list;     // list of completions
    IEnumString*    m_pes;           // Used internally for real AutoComplete functionality.
    IACList*        m_pacl;          // Additional methods for autocomplete lists (optional).

    void        _SendAsyncShutDownMsg();
    void        _FreeThreadData();
    HRESULT     _ThreadLoop();
    void        _Search(LPWSTR pszSearch, DWORD dwOptions);
    BOOL        _AddToList(LPTSTR pszUrl, int cchMatch);
    void        _DoExpand(LPCWSTR pszSearch);
    static DWORD WINAPI _ThreadProc(LPVOID lpv);
    static int CALLBACK _DpaCompare(LPVOID p1, LPVOID p2, LPARAM lParam);
};

//+-------------------------------------------------------------------------
// CAutoComplete - Main autocomplete class that runs on the main UI thread
//--------------------------------------------------------------------------
class CAutoComplete
                : public IAutoComplete2
                , public IAutoCompleteDropDown
                , public IEnumString
                , public CDelegateAccessibleImpl
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    // *** IEnumString ***
    virtual STDMETHODIMP Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched);
    virtual STDMETHODIMP Skip(ULONG celt) { return E_NOTIMPL; }
    virtual STDMETHODIMP Reset();
    virtual STDMETHODIMP Clone(IEnumString **ppenum) { return E_NOTIMPL; }

    // *** IAutoComplete ***
    virtual STDMETHODIMP Init(HWND hwnd, IUnknown *punkACL, LPCOLESTR pwszRegKeyPath, LPCOLESTR pwszQuickCompleteString);
    virtual STDMETHODIMP Enable(BOOL fEnable);

    // *** IAutoComplete2 ***
    virtual STDMETHODIMP SetOptions(DWORD dwFlag);
    virtual STDMETHODIMP GetOptions(DWORD* pdwFlag);

    // *** IAutoCompleteDropDown ***
    virtual STDMETHODIMP GetDropDownStatus(DWORD *pdwFlags, LPWSTR *ppwszString);
    virtual STDMETHODIMP ResetEnumerator();

    // *** IAccessible ***
    STDMETHODIMP get_accName(VARIANT varChild, BSTR  *pszName);

protected:
    // Methods called by the background thread
    friend CACThread;
    void SearchComplete(HDPA hdpa, BOOL fLimitReached) { PostMessage(m_hwndEdit, m_uMsgSearchComplete, fLimitReached, (LPARAM)hdpa); }
    BOOL IsEnabled();

    // Constructor / Destructor (protected so we can't create on stack)
    CAutoComplete();
    ~CAutoComplete();

    // Instance creator
    friend HRESULT CAutoComplete_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
    BOOL _Init();

    // Private variables
    DWORD           m_dwRefCount;

    CACThread*      m_pThread;              // background autocomplete thread
    TCHAR           m_szQuickComplete[MAX_PATH];
    TCHAR           m_szRegKeyPath[MAX_PATH];
    DWORD           m_dwFlags;
    HWND            m_hwndEdit;
    HWND            m_hwndCombo;            // if m_hwndEdit is part of a combobox
    LPTSTR          m_pszCurrent;
    int             m_iCurrent;
    DWORD           m_dwLastSearchFlags;
    WNDPROC         m_pOldListViewWndProc;

    IEnumString *   m_pes;                  // Used internally for real AutoComplete functionality.
    IACList *       m_pacl;                 // Additional methods for autocomplete lists (optional).

    HDPA            m_hdpa;                 // sorted completions list
    LPWSTR          m_pszLastSearch;        // string last sent for completion
    int             m_iFirstMatch;          // first match in list (-1 if no matches)
    int             m_iLastMatch;           // last match in list (-1 if no matches)
    int             m_iAppended;            // item completed in the edit box
    BITBOOL         m_fNeedNewList:1;       // last search was truncated
    BITBOOL         m_fDropDownResized:1;   // user has resized drop down
    BITBOOL         m_fAppended:1;          // if something currently appended
    BITBOOL         m_fSearchForAdded:1;    // if last item in dpa is "Search for <>"
    BITBOOL         m_fSearchFor:1;         // if "Search for <>" is to be displayed
    BITBOOL         m_fImeCandidateOpen:1;  // if the IME's candidate window is visible
    DWORD           m_dwOptions;            // autocomplete options (ACO_*)
    EDITWORDBREAKPROC m_oldEditWordBreakProc; // original word break proc for m_hwndEdit

    // Member variables for drop-down auto-suggest window
    HWND            m_hwndDropDown;         // Shows completions in drop-down window
    HWND            m_hwndList;             // Shows completions in drop-down window
    HWND            m_hwndScroll;           // scrollbar
    HWND            m_hwndGrip;             // gripper for resizing the dropdown
    int             m_nStatusHeight;        // height of status in drop-down
    int             m_nDropWidth;           // width of drop-down window
    int             m_nDropHeight;          // height of drop-down window
    int             m_cxGripper;            // width/height of gripper
    BITBOOL         m_fDroppedUp:1;         // if dropdown is over top the edit box
#ifdef ALLOW_ALWAYS_DROP_UP
    BITBOOL         m_fAlwaysDropUp:1;      // TEST regkey to always drop up
#endif
    BITBOOL         m_fSettingText:1;       // if setting the edit text
    BITBOOL         m_fInHotTracking:1;     // if new selection is due to hot-tracking 

    // Member Variables used for external IEnumString
    IEnumString *   m_pesExtern;            // Used internally for real AutoComplete functionality.
    LPTSTR          m_szEnumString;

    // Registered messages sent to edit window
    UINT            m_uMsgSearchComplete;
    UINT            m_uMsgItemActivate;

    static HHOOK    s_hhookMouse;           // windows hook installed when drop-down visible
    static HWND     s_hwndDropDown;         // dropdown currently visible
    static BOOL     s_fNoActivate;          // keep topmost-window from losing activation

    void        _OnSearchComplete(HDPA hdpa, BOOL fLimitReached);
    BOOL        _GetItem(int iIndex, LPWSTR pswText, int cchMax, BOOL fDisplayName);
    void        _UpdateCompletion(LPCWSTR pszTyped, int iChanged, BOOL fAppend);
    void        _HideDropDown();
    void        _ShowDropDown();
    void        _PositionDropDown();
    void        _SeeWhatsEnabled();
    BOOL        _IsAutoSuggestEnabled() { return m_dwOptions & ACO_AUTOSUGGEST; }
    BOOL        _IsRTLReadingEnabled() { return m_dwOptions & ACO_RTLREADING; }
    BOOL        _IsAutoAppendEnabled() { return (m_dwOptions & ACO_AUTOAPPEND) || (m_dwOptions & ACO_UNINITIALIZED); }
    BOOL        _IsComboboxDropped() { return (m_hwndCombo && ComboBox_GetDroppedState(m_hwndCombo)); }
    void        _UpdateGrip();
    void        _UpdateScrollbar();

    static BOOL _IsWhack(TCHAR ch);
    static BOOL _IsBreakChar(WCHAR wch);
    BOOL        _WantToAppendResults();
    int         _JumpToNextBreak(int iLoc, DWORD dwFlags);
    BOOL        _CursorMovement(WPARAM wParam);
    void        _RemoveCompletion();
    void        _GetEditText();
    void        _SetEditText(LPCWSTR psz);
    void        _UpdateText(int iStartSel, int iEndSel, LPCTSTR pszCurrent, LPCTSTR pszNew);
    BOOL        _OnKeyDown(WPARAM wParam);
    LRESULT     _OnChar(WPARAM wParam, LPARAM lParam);
    void        _StartCompletion(BOOL fAppend, BOOL fEvenIfEmpty = FALSE);
    BOOL        _StartSearch(LPCWSTR pszSearch);
    void        _StopSearch();
    BOOL        _ResetSearch();
    void        _GotFocus();
    LPTSTR      _QuickEnter();
    BOOL        _AppendNext(BOOL fAppendToWhack);
    BOOL        _AppendPrevious(BOOL fAppendToWhack);
    void        _Append(CACString& rStr, BOOL fAppendToWhack);
    BOOL        _SetQuickCompleteStrings(LPCOLESTR pcszRegKeyPath, LPCOLESTR pcszQuickCompleteString);
    void        _SubClassParent(HWND hwnd);
    void        _UnSubClassParent(HWND hwnd);

    LRESULT     _DropDownWndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT     _EditWndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT     _ListViewWndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

    static int  _DPADestroyCallback(LPVOID p, LPVOID d);
    static void _FreeDPAPtrs(HDPA hdpa);
    static int CALLBACK EditWordBreakProcW(LPWSTR lpch, int ichCurrent, int cch, int code);
    static LRESULT CALLBACK s_EditWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    static LRESULT CALLBACK s_DropDownWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK s_ListViewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK s_ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    static LRESULT CALLBACK s_GripperWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    static LRESULT CALLBACK s_MouseHook(int nCode, WPARAM wParam, LPARAM lParam);
};


#endif
