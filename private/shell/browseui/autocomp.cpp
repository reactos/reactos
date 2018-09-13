// Copyright 1996-98 Microsoft
#include "priv.h"
#include "sccls.h"
#include "autocomp.h"
#include "dbgmem.h"
#include "itbar.h"
#include "address.h"
#include "addrlist.h"
#include "resource.h"
#include "mluisupp.h"

#ifdef UNIX
#include "unixstuff.h"
#endif

#include "apithk.h"

extern HRESULT CACLMRU_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi, LPCTSTR pszMRU);

#define WZ_REGKEY_QUICKCOMPLETE         L"Software\\Microsoft\\Internet Explorer\\Toolbar\\QuickComplete"
#define WZ_DEFAULTQUICKCOMPLETE         L"http://www.%s.com"


// Statics
const static TCHAR c_szAutoDefQuickComp[]   = TEXT("%s");
const static TCHAR c_szAutoCompleteProp[]   = TEXT("CAutoComplete_This");
const static TCHAR c_szParentWindowProp[]   = TEXT("CParentWindow_This");
const static TCHAR c_szAutoSuggest[]        = TEXT("AutoSuggest Drop-Down");
const static TCHAR c_szAutoSuggestTitle[]   = TEXT("Internet Explorer");

BOOL CAutoComplete::s_fNoActivate = FALSE;
HWND CAutoComplete::s_hwndDropDown = NULL;
HHOOK CAutoComplete::s_hhookMouse = NULL;


#define MAX_QUICK_COMPLETE_STRING   64
#define LISTVIEW_COLUMN_WIDTH   30000

//
// FLAGS for dwFlags
//
#define ACF_RESET               0x00000000
#define ACF_IGNOREUPDOWN        0x00000004

#define URL_SEPARATOR_CHAR      TEXT('/')

#ifdef UNIX
#define DIR_SEPARATOR_CHAR      TEXT('/')
#define DIR_SEPARATOR_STRING    TEXT("/")
#else
#define DIR_SEPARATOR_CHAR      TEXT('\\')
#define DIR_SEPARATOR_STRING    TEXT("\\")
#endif

/////////////////////////////////////////////////////////////////////////////
// Line Break Character Table
//
// This was swipped from mlang.  Special break characters added for URLs
// have an "IE:" in the comment.  Note that this table must be sorted!

const WCHAR g_szBreakChars[] = {
    0x0009, // TAB
    0x0020, // SPACE
    0x0021, // IE: !
    0x0022, // IE: "
    0x0023, // IE: #
    0x0024, // IE: $
    0x0025, // IE: %
    0x0026, // IE: &
    0x0027, // IE: '
    0x0028, // LEFT PARENTHESIS
    0x0029, // RIGHT PARENTHESIS
    0x002A, // IE: *
    0x002B, // IE: +
    0x002C, // IE: ,
    0x002D, // HYPHEN
    0x002E, // IE: .
    0x002F, // IE: /
    0x003A, // IE: :
    0x003B, // IE: ;
    0x003C, // IE: <
    0x003D, // IE: =
    0x003E, // IE: >
    0x003F, // IE: ?
    0x0040, // IE: @
    0x005B, // LEFT SQUARE BRACKET
    0x005C, // IE: '\'
    0x005D, // RIGHT SQUARE BRACKET
    0x005E, // IE: ^
    0x005F, // IE: _
    0x0060, // IE:`
    0x007B, // LEFT CURLY BRACKET
    0x007C, // IE: |
    0x007D, // RIGHT CURLY BRACKET
    0x007E, // IE: ~
    0x00AB, // LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
    0x00AD, // OPTIONAL HYPHEN
    0x00BB, // RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
    0x02C7, // CARON
    0x02C9, // MODIFIER LETTER MACRON
    0x055D, // ARMENIAN COMMA
    0x060C, // ARABIC COMMA
    0x2002, // EN SPACE
    0x2003, // EM SPACE
    0x2004, // THREE-PER-EM SPACE
    0x2005, // FOUR-PER-EM SPACE
    0x2006, // SIX-PER-EM SPACE
    0x2007, // FIGURE SPACE
    0x2008, // PUNCTUATION SPACE
    0x2009, // THIN SPACE
    0x200A, // HAIR SPACE
    0x200B, // ZERO WIDTH SPACE
    0x2013, // EN DASH
    0x2014, // EM DASH
    0x2016, // DOUBLE VERTICAL LINE
    0x2018, // LEFT SINGLE QUOTATION MARK
    0x201C, // LEFT DOUBLE QUOTATION MARK
    0x201D, // RIGHT DOUBLE QUOTATION MARK
    0x2022, // BULLET
    0x2025, // TWO DOT LEADER
    0x2026, // HORIZONTAL ELLIPSIS
    0x2027, // HYPHENATION POINT
    0x2039, // SINGLE LEFT-POINTING ANGLE QUOTATION MARK
    0x203A, // SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
    0x2045, // LEFT SQUARE BRACKET WITH QUILL
    0x2046, // RIGHT SQUARE BRACKET WITH QUILL
    0x207D, // SUPERSCRIPT LEFT PARENTHESIS
    0x207E, // SUPERSCRIPT RIGHT PARENTHESIS
    0x208D, // SUBSCRIPT LEFT PARENTHESIS
    0x208E, // SUBSCRIPT RIGHT PARENTHESIS
    0x226A, // MUCH LESS THAN
    0x226B, // MUCH GREATER THAN
    0x2574, // BOX DRAWINGS LIGHT LEFT
    0x3001, // IDEOGRAPHIC COMMA
    0x3002, // IDEOGRAPHIC FULL STOP
    0x3003, // DITTO MARK
    0x3005, // IDEOGRAPHIC ITERATION MARK
    0x3008, // LEFT ANGLE BRACKET
    0x3009, // RIGHT ANGLE BRACKET
    0x300A, // LEFT DOUBLE ANGLE BRACKET
    0x300B, // RIGHT DOUBLE ANGLE BRACKET
    0x300C, // LEFT CORNER BRACKET
    0x300D, // RIGHT CORNER BRACKET
    0x300E, // LEFT WHITE CORNER BRACKET
    0x300F, // RIGHT WHITE CORNER BRACKET
    0x3010, // LEFT BLACK LENTICULAR BRACKET
    0x3011, // RIGHT BLACK LENTICULAR BRACKET
    0x3014, // LEFT TORTOISE SHELL BRACKET
    0x3015, // RIGHT TORTOISE SHELL BRACKET
    0x3016, // LEFT WHITE LENTICULAR BRACKET
    0x3017, // RIGHT WHITE LENTICULAR BRACKET
    0x3018, // LEFT WHITE TORTOISE SHELL BRACKET
    0x3019, // RIGHT WHITE TORTOISE SHELL BRACKET
    0x301A, // LEFT WHITE SQUARE BRACKET
    0x301B, // RIGHT WHITE SQUARE BRACKET
    0x301D, // REVERSED DOUBLE PRIME QUOTATION MARK
    0x301E, // DOUBLE PRIME QUOTATION MARK
    0x3041, // HIRAGANA LETTER SMALL A
    0x3043, // HIRAGANA LETTER SMALL I
    0x3045, // HIRAGANA LETTER SMALL U
    0x3047, // HIRAGANA LETTER SMALL E
    0x3049, // HIRAGANA LETTER SMALL O
    0x3063, // HIRAGANA LETTER SMALL TU
    0x3083, // HIRAGANA LETTER SMALL YA
    0x3085, // HIRAGANA LETTER SMALL YU
    0x3087, // HIRAGANA LETTER SMALL YO
    0x308E, // HIRAGANA LETTER SMALL WA
    0x309B, // KATAKANA-HIRAGANA VOICED SOUND MARK
    0x309C, // KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK
    0x309D, // HIRAGANA ITERATION MARK
    0x309E, // HIRAGANA VOICED ITERATION MARK
    0x30A1, // KATAKANA LETTER SMALL A
    0x30A3, // KATAKANA LETTER SMALL I
    0x30A5, // KATAKANA LETTER SMALL U
    0x30A7, // KATAKANA LETTER SMALL E
    0x30A9, // KATAKANA LETTER SMALL O
    0x30C3, // KATAKANA LETTER SMALL TU
    0x30E3, // KATAKANA LETTER SMALL YA
    0x30E5, // KATAKANA LETTER SMALL YU
    0x30E7, // KATAKANA LETTER SMALL YO
    0x30EE, // KATAKANA LETTER SMALL WA
    0x30F5, // KATAKANA LETTER SMALL KA
    0x30F6, // KATAKANA LETTER SMALL KE
    0x30FC, // KATAKANA-HIRAGANA PROLONGED SOUND MARK
    0x30FD, // KATAKANA ITERATION MARK
    0x30FE, // KATAKANA VOICED ITERATION MARK
    0xFD3E, // ORNATE LEFT PARENTHESIS
    0xFD3F, // ORNATE RIGHT PARENTHESIS
    0xFE30, // VERTICAL TWO DOT LEADER
    0xFE31, // VERTICAL EM DASH
    0xFE33, // VERTICAL LOW LINE
    0xFE34, // VERTICAL WAVY LOW LINE
    0xFE35, // PRESENTATION FORM FOR VERTICAL LEFT PARENTHESIS
    0xFE36, // PRESENTATION FORM FOR VERTICAL RIGHT PARENTHESIS
    0xFE37, // PRESENTATION FORM FOR VERTICAL LEFT CURLY BRACKET
    0xFE38, // PRESENTATION FORM FOR VERTICAL RIGHT CURLY BRACKET
    0xFE39, // PRESENTATION FORM FOR VERTICAL LEFT TORTOISE SHELL BRACKET
    0xFE3A, // PRESENTATION FORM FOR VERTICAL RIGHT TORTOISE SHELL BRACKET
    0xFE3B, // PRESENTATION FORM FOR VERTICAL LEFT BLACK LENTICULAR BRACKET
    0xFE3C, // PRESENTATION FORM FOR VERTICAL RIGHT BLACK LENTICULAR BRACKET
    0xFE3D, // PRESENTATION FORM FOR VERTICAL LEFT DOUBLE ANGLE BRACKET
    0xFE3E, // PRESENTATION FORM FOR VERTICAL RIGHT DOUBLE ANGLE BRACKET
    0xFE3F, // PRESENTATION FORM FOR VERTICAL LEFT ANGLE BRACKET
    0xFE40, // PRESENTATION FORM FOR VERTICAL RIGHT ANGLE BRACKET
    0xFE41, // PRESENTATION FORM FOR VERTICAL LEFT CORNER BRACKET
    0xFE42, // PRESENTATION FORM FOR VERTICAL RIGHT CORNER BRACKET
    0xFE43, // PRESENTATION FORM FOR VERTICAL LEFT WHITE CORNER BRACKET
    0xFE44, // PRESENTATION FORM FOR VERTICAL RIGHT WHITE CORNER BRACKET
    0xFE4F, // WAVY LOW LINE
    0xFE50, // SMALL COMMA
    0xFE51, // SMALL IDEOGRAPHIC COMMA
    0xFE59, // SMALL LEFT PARENTHESIS
    0xFE5A, // SMALL RIGHT PARENTHESIS
    0xFE5B, // SMALL LEFT CURLY BRACKET
    0xFE5C, // SMALL RIGHT CURLY BRACKET
    0xFE5D, // SMALL LEFT TORTOISE SHELL BRACKET
    0xFE5E, // SMALL RIGHT TORTOISE SHELL BRACKET
    0xFF08, // FULLWIDTH LEFT PARENTHESIS
    0xFF09, // FULLWIDTH RIGHT PARENTHESIS
    0xFF0C, // FULLWIDTH COMMA
    0xFF0E, // FULLWIDTH FULL STOP
    0xFF1C, // FULLWIDTH LESS-THAN SIGN
    0xFF1E, // FULLWIDTH GREATER-THAN SIGN
    0xFF3B, // FULLWIDTH LEFT SQUARE BRACKET
    0xFF3D, // FULLWIDTH RIGHT SQUARE BRACKET
    0xFF40, // FULLWIDTH GRAVE ACCENT
    0xFF5B, // FULLWIDTH LEFT CURLY BRACKET
    0xFF5C, // FULLWIDTH VERTICAL LINE
    0xFF5D, // FULLWIDTH RIGHT CURLY BRACKET
    0xFF5E, // FULLWIDTH TILDE
    0xFF61, // HALFWIDTH IDEOGRAPHIC FULL STOP
    0xFF62, // HALFWIDTH LEFT CORNER BRACKET
    0xFF63, // HALFWIDTH RIGHT CORNER BRACKET
    0xFF64, // HALFWIDTH IDEOGRAPHIC COMMA
    0xFF67, // HALFWIDTH KATAKANA LETTER SMALL A
    0xFF68, // HALFWIDTH KATAKANA LETTER SMALL I
    0xFF69, // HALFWIDTH KATAKANA LETTER SMALL U
    0xFF6A, // HALFWIDTH KATAKANA LETTER SMALL E
    0xFF6B, // HALFWIDTH KATAKANA LETTER SMALL O
    0xFF6C, // HALFWIDTH KATAKANA LETTER SMALL YA
    0xFF6D, // HALFWIDTH KATAKANA LETTER SMALL YU
    0xFF6E, // HALFWIDTH KATAKANA LETTER SMALL YO
    0xFF6F, // HALFWIDTH KATAKANA LETTER SMALL TU
    0xFF70, // HALFWIDTH KATAKANA-HIRAGANA PROLONGED SOUND MARK
    0xFF9E, // HALFWIDTH KATAKANA VOICED SOUND MARK
    0xFF9F, // HALFWIDTH KATAKANA SEMI-VOICED SOUND MARK
    0xFFE9, // HALFWIDTH LEFTWARDS ARROW
    0xFFEB, // HALFWIDTH RIGHTWARDS ARROW
};

/*
//
// AutoComplete Common Functions / Structures
//
const struct {
    UINT    idMenu;
    UINT    idCmd;
} MenuToMessageId[] = {
    { IDM_AC_UNDO, WM_UNDO },
    { IDM_AC_CUT,  WM_CUT },
    { IDM_AC_COPY, WM_COPY },
    { IDM_AC_PASTE, WM_PASTE }
};
*/

//+-------------------------------------------------------------------------
// IUnknown methods
//--------------------------------------------------------------------------
HRESULT CAutoComplete::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IAutoComplete) ||
        IsEqualIID(riid, IID_IAutoComplete2))
    {
        *ppvObj = SAFECAST(this, IAutoComplete2*);
    }
    else if (IsEqualIID(riid, IID_IAutoCompleteDropDown))
    {
        *ppvObj = SAFECAST(this, IAutoCompleteDropDown*);
    }
    else if (IsEqualIID(riid, IID_IEnumString))
    {
        *ppvObj = SAFECAST(this, IEnumString*);
    }
    else
    {
        return _DefQueryInterface(riid, ppvObj);
    }

    AddRef();
    return S_OK;
}

ULONG CAutoComplete::AddRef(void)
{
    InterlockedIncrement((LPLONG)&m_dwRefCount);
    TraceMsg(AC_GENERAL, "CAutoComplete::AddRef() --- m_dwRefCount = %i ", m_dwRefCount);

    return m_dwRefCount;
}

ULONG CAutoComplete::Release(void)
{
    ASSERT(m_dwRefCount > 0);

    if (InterlockedDecrement((LPLONG)&m_dwRefCount))
    {
        TraceMsg(AC_GENERAL, "CAutoComplete::Release() --- m_dwRefCount = %i", m_dwRefCount);
        return m_dwRefCount;
    }

    TraceMsg(AC_GENERAL, "CAutoComplete::Release() --- m_dwRefCount = %i", m_dwRefCount);

    delete this;
    return 0;
}

/* IAutoComplete methods */
//+-------------------------------------------------------------------------
//  This object can be Inited in two ways.  This function will init it in
//  the first way, which works as follows:
//
//  1. The caller called CoInitialize or OleInitialize() and the corresponding
//     uninit will not be called until the control we are subclassing and
//     our selfs are long gone.
//  2. The caller calls us on their main thread and we create and destroy
//     the background thread as needed.
//--------------------------------------------------------------------------
HRESULT CAutoComplete::Init
(
    HWND hwndEdit,              // control to be subclassed
    IUnknown *punkACL,          // autocomplete list
    LPCOLESTR pwszRegKeyPath,   // reg location where ctrl-enter completion is stored stored
    LPCOLESTR pwszQuickComplete // default format string for ctrl-enter completion
)
{
    HRESULT hr = S_OK;

    TraceMsg(AC_GENERAL, "CAutoComplete::Init(hwndEdit=0x%x, punkACL = 0x%x, pwszRegKeyPath = 0x%x, pwszQuickComplete = 0x%x)",
        hwndEdit, punkACL, pwszRegKeyPath, pwszQuickComplete);

#ifdef DEBUG
    // Ensure that the Line Break Character Table is ordered
    WCHAR c = g_szBreakChars[0];
    for (int i = 1; i < ARRAYSIZE(g_szBreakChars); ++i)
    {
        ASSERT(c < g_szBreakChars[i]);
        c = g_szBreakChars[i];
    }
#endif

    if (m_hwndEdit != NULL)
    {
        // Can currently only be initialized once
        ASSERT(FALSE);
        return E_FAIL;
    }

    m_hwndEdit = hwndEdit;

#ifndef UNIX
    // Add our custom word-break callback so that we recognize URL delimitors when
    // ctrl-arrowing around.
    //
    // There is a bug with how USER handles WH_CALLWNDPROC global hooks in Win95 that
    // causes us to blow up if one is installed and a wordbreakproc is set.  Thus,
    // if an app is running that has one of these hooks installed (intellipoint 1.1 etc.) then
    // if we install our wordbreakproc the app will fault when the proc is called.  There
    // does not appear to be any way for us to work around it since USER's thunking code
    // trashes the stack so this API is disabled for Win95.
    //
    m_oldEditWordBreakProc = (EDITWORDBREAKPROC)SendMessage(m_hwndEdit, EM_GETWORDBREAKPROC, 0, 0);
    if (g_fRunningOnNT && IsWindowUnicode(m_hwndEdit))
    {
        SendMessage(m_hwndEdit, EM_SETWORDBREAKPROC, 0, (DWORD_PTR)EditWordBreakProcW);
    }
#endif

    //
    // bug 81414 : To avoid clashing with app messages used by the edit window, we 
    // use registered messages.
    //
    m_uMsgSearchComplete  = RegisterWindowMessageA("AC_SearchComplete");
    m_uMsgItemActivate    = RegisterWindowMessageA("AC_ItemActivate");

    if (m_uMsgSearchComplete == 0)
    {
        m_uMsgSearchComplete = WM_APP + 300;
    }
    if (m_uMsgItemActivate == 0)
    {
        m_uMsgItemActivate   = WM_APP + 301;
    }

    _SetQuickCompleteStrings(pwszRegKeyPath, pwszQuickComplete);

    // IEnumString required
    ASSERT(m_pes == NULL);
    EVAL(SUCCEEDED(punkACL->QueryInterface(IID_IEnumString, (void **)&m_pes)));

    // IACList optional
    ASSERT(m_pacl == NULL);
    punkACL->QueryInterface(IID_IACList, (void **)&m_pacl);

    AddRef();       // Hold on to a ref for our Subclass.

    // Initial creation should have failed if the thread object was not allocated!
    ASSERT(m_pThread);
    m_pThread->Init(m_pes, m_pacl);

    // subclass the edit window
    SetWindowSubclass(m_hwndEdit, &s_EditWndProc, 0, (DWORD_PTR)this);

    // See what autocomplete features are enabled
    _SeeWhatsEnabled();

    // See if hwndEdit is part of a combobox
    HWND hwndParent = GetParent(m_hwndEdit);
    WCHAR szClass[30];
    int nLen = GetClassName(hwndParent, szClass, ARRAYSIZE(szClass));
    if (nLen != 0 &&
        (StrCmpI(szClass, L"combobox") == 0 || StrCmpI(szClass, L"comboboxex") == 0))
    {
        m_hwndCombo = hwndParent;
    }

    // If we've already got focus, then we need to call GotFocus...
    if (GetFocus() == hwndEdit)
    {
        m_pThread->GotFocus();
    }

    return hr;
}

//+-------------------------------------------------------------------------
// Checks to see if autoappend or autosuggest freatures are enabled
//--------------------------------------------------------------------------
void CAutoComplete::_SeeWhatsEnabled()
{
#ifdef ALLOW_ALWAYS_DROP_UP
    m_fAlwaysDropUp = SHRegGetBoolUSValue(REGSTR_PATH_AUTOCOMPLETE,
                            TEXT("AlwaysDropUp"), FALSE, /*default:*/FALSE);
#endif

    // If autosuggest was just enabled, create the dropdown window
    if (_IsAutoSuggestEnabled() && NULL == m_hwndDropDown)
    {
        // Create the dropdown Window
        WNDCLASS wc = {0};

        wc.lpfnWndProc      = s_DropDownWndProc;
        wc.cbWndExtra       = SIZEOF(CAutoComplete*);
        wc.hInstance        = HINST_THISDLL;
        wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground    = (HBRUSH)(COLOR_WINDOW+1);
        wc.lpszClassName    = c_szAutoSuggestClass;

        SHRegisterClass(&wc);

        DWORD dwExStyle =  WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOPARENTNOTIFY;
        if(_IsRTLReadingEnabled())
        {
            dwExStyle |= WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR;
        }
#ifdef UNIX
        // BUGBUG
        // IEUNIX : Working around window activation problems in mainwin
        // Autocomplete is completely hosed without this flag on UNIX.
        dwExStyle |= WS_EX_MW_UNMANAGED_WINDOW;
#endif

        m_hwndDropDown = CreateWindowEx(dwExStyle,
                                        c_szAutoSuggestClass,
                                        c_szAutoSuggestTitle,   // GPF dialog is picking up this name!
                                        WS_POPUP | WS_BORDER | WS_CLIPCHILDREN,
                                        0, 0, 100, 100,
                                        NULL, NULL, HINST_THISDLL, this);

        if (m_hwndDropDown)
        {
            m_fDropDownResized = FALSE;
        }
    }
    else if (!_IsAutoSuggestEnabled() && NULL != m_hwndDropDown)
    {
        // We don't need the dropdown Window.
        if (m_hwndList)
        {
            DestroyWindow(m_hwndList);
        }
        DestroyWindow(m_hwndDropDown);
        m_hwndDropDown = NULL;
        
        m_hwndList = NULL;
        m_hwndScroll = NULL;
        m_hwndGrip = NULL;
    }
}

//+-------------------------------------------------------------------------
// Returns TRUE if autocomplete is currently enabled
//--------------------------------------------------------------------------
BOOL CAutoComplete::IsEnabled()
{
    BOOL fRet;

    //
    // If we have not used the new IAutoComplete2 interface, we revert
    // to the old IE4 global registry setting
    //
    if (m_dwOptions & ACO_UNINITIALIZED)
    {
        fRet = SHRegGetBoolUSValue(REGSTR_PATH_AUTOCOMPLETE,
                            REGSTR_VAL_USEAUTOCOMPLETE, FALSE, TRUE);
    }
    else
    {
        fRet = (m_dwOptions & (ACO_AUTOAPPEND | ACO_AUTOSUGGEST));
    }
    return fRet;
}

//+-------------------------------------------------------------------------
// Enables/disables the up down arrow for autocomplete.  Used by comboboxes
// to disable arrow keys when the combo box is dropped. (This function is
// now redundent because we check to see of the combo is dropped.)
//--------------------------------------------------------------------------
HRESULT CAutoComplete::Enable(BOOL fEnable)
{
    TraceMsg(AC_GENERAL, "CAutoComplete::Enable(0x%x)", fEnable);

    HRESULT hr = (m_dwFlags & ACF_IGNOREUPDOWN) ? S_FALSE : S_OK;

    if (fEnable)
        m_dwFlags &= ~ACF_IGNOREUPDOWN;
    else
        m_dwFlags |= ACF_IGNOREUPDOWN;

    return hr;
}

/* IAutocomplete2 methods */
//+-------------------------------------------------------------------------
// Enables/disables various autocomplete features (see ACO_* flags)
//--------------------------------------------------------------------------
HRESULT CAutoComplete::SetOptions(DWORD dwOptions)
{
    m_dwOptions = dwOptions;
    _SeeWhatsEnabled();
    return S_OK;
}

//+-------------------------------------------------------------------------
// Returns the current option settings
//--------------------------------------------------------------------------
HRESULT CAutoComplete::GetOptions(DWORD* pdwOptions)
{
    HRESULT hr = E_INVALIDARG;
    if (pdwOptions)
    {
        *pdwOptions = m_dwOptions;
        hr = S_OK;
    }

    return hr;
}

/* IAutocompleteDropDown methods */
//+-------------------------------------------------------------------------
// Returns the current dropdown status
//--------------------------------------------------------------------------
HRESULT CAutoComplete::GetDropDownStatus(DWORD *pdwFlags, LPWSTR *ppwszString)
{
    if (m_hwndDropDown && IsWindowVisible(m_hwndDropDown))
    {
        if (pdwFlags)
        {
            *pdwFlags = ACDD_VISIBLE;
        }

        if (ppwszString)
        {
            *ppwszString=NULL;

            if (m_hwndList)
            {
                int iCurSel = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);
                if (iCurSel != -1)
                {
                    WCHAR szBuf[MAX_URL_STRING];
                    _GetItem(iCurSel, szBuf, ARRAYSIZE(szBuf), FALSE);

                    *ppwszString = (LPWSTR) CoTaskMemAlloc((lstrlenW(szBuf)+1)*sizeof(WCHAR));
                    if (*ppwszString)
                    {
                        StrCpyW(*ppwszString, szBuf);
                    }
                }
            }
        }
    }
    else
    {
        if (pdwFlags)
        {
            *pdwFlags = 0;
        }

        if (ppwszString)
        {
            *ppwszString = NULL;
        }
    }

    return S_OK;
}

HRESULT CAutoComplete::ResetEnumerator()
{
    _StopSearch();
    _ResetSearch();
    _FreeDPAPtrs(m_hdpa);
    m_hdpa = NULL;

    // If the dropdown is currently visible, re-search the IEnumString
    //  and show the dropdown. Otherwise wait for user input.
    if (m_hwndDropDown && IsWindowVisible(m_hwndDropDown))
    {
        _StartCompletion(FALSE, TRUE);
    }

    return S_OK;
}

/* IEnumString methods */
//+-------------------------------------------------------------------------
// Resets the IEnumString functionality exposed for external users.
//--------------------------------------------------------------------------
HRESULT CAutoComplete::Reset()
{
    HRESULT hr = E_FAIL;

    if (!m_szEnumString)        // If we needed it once, we will most likely continue to need it.
        m_szEnumString = (LPTSTR) TrcLocalAlloc(LPTR, MAX_URL_STRING * SIZEOF(TCHAR));

    if (!m_szEnumString)
        return E_OUTOFMEMORY;

    GetWindowText(m_hwndEdit, m_szEnumString, MAX_URL_STRING);
    if (m_pesExtern)
        hr = m_pesExtern->Reset();
    else
    {
        hr = m_pes->Clone(&m_pesExtern);
        if (SUCCEEDED(hr))
            hr = m_pesExtern->Reset();
    }

    return hr;
}

//+-------------------------------------------------------------------------
// Returns the next BSTR from the autocomplete enumeration.
//
// For consistant results, the caller should not allow the AutoComplete text
// to change between one call to Next() and another call to Next().
// AutoComplete text should change only before Reset() is called.
//--------------------------------------------------------------------------
HRESULT CAutoComplete::Next
(
    ULONG celt,         // number items to fetch, needs to be 1
    LPOLESTR *rgelt,    // returned BSTR, caller must free
    ULONG *pceltFetched // number of items returned
)
{
    HRESULT hr = S_FALSE;
    LPOLESTR pwszUrl;
    ULONG cFetched;

    // Pre-init in case of error
    if (rgelt)
        *rgelt = NULL;
    if (pceltFetched)
        *pceltFetched = 0;

    if (!EVAL(rgelt) || (!EVAL(pceltFetched)) || (!EVAL(1 == celt)) || !EVAL(m_pesExtern))
        return E_INVALIDARG;

    while (S_OK == (hr = m_pesExtern->Next(1, &pwszUrl, &cFetched)))
    {
        if (!StrCmpNI(m_szEnumString, pwszUrl, lstrlen(m_szEnumString)))
        {
            TraceMsg(TF_BAND|TF_GENERAL, "CAutoComplete: Next(). AutoSearch Failed URL=%s.", pwszUrl);
            break;
        }
        else
        {
            // If the string can't be added to our list, we will free it.
            TraceMsg(TF_BAND|TF_GENERAL, "CAutoComplete: Next(). AutoSearch Match URL=%s.", pwszUrl);
            CoTaskMemFree(pwszUrl);
        }
    }

    if (S_OK == hr)
    {
        *rgelt = (LPOLESTR)pwszUrl;
        *pceltFetched = 1;  // We will always only fetch one.
    }

    return hr;
}

#pragma warning(disable:4355)  // using 'this' in constructor

/* Constructor / Destructor / CreateInstance */

//+-------------------------------------------------------------------------
// Constructor
//--------------------------------------------------------------------------
CAutoComplete::CAutoComplete()
{
    DllAddRef();
    TraceMsg(AC_GENERAL, "CAutoComplete::CAutoComplete()");

    // This class requires that this COM object be allocated in Zero INITed
    // memory.  If the asserts below go off, then this was violated.
    ASSERT(!m_dwFlags);
    ASSERT(!m_hwndEdit);
    ASSERT(!m_pszCurrent);
    ASSERT(!m_iCurrent);
    ASSERT(!m_dwLastSearchFlags);
    ASSERT(!m_pes);
    ASSERT(!m_pacl);
    ASSERT(!m_pesExtern);
    ASSERT(!m_szEnumString);
    ASSERT(!m_pThread);

    m_dwOptions = ACO_UNINITIALIZED;
    m_dwRefCount = 1;
}

//+-------------------------------------------------------------------------
// Destructor
//--------------------------------------------------------------------------
CAutoComplete::~CAutoComplete()
{
    TraceMsg(AC_GENERAL, "CAutoComplete::~CAutoComplete()");

    if (m_hwndDropDown)
    {
        DestroyWindow(m_hwndDropDown);
        m_hwndDropDown = NULL;
    }

    SAFERELEASE(m_pes);
    SAFERELEASE(m_pacl);
    SAFERELEASE(m_pesExtern);

    SetStr(&m_pszCurrent, NULL);

    if (m_szEnumString)
        TrcLocalFree(m_szEnumString);

    _FreeDPAPtrs(m_hdpa);

    if (m_pThread)
    {
        m_pThread->SyncShutDownBGThread();
        SAFERELEASE(m_pThread);
    }

    DllRelease();
}

STDMETHODIMP CAutoComplete::get_accName(VARIANT varChild, BSTR  *pszName)
{
    HRESULT hr;
    
    if (varChild.vt == VT_I4)
    {
        if (varChild.lVal > 0)
        {
            WCHAR szBuf[MAX_URL_STRING];
            
            _GetItem(varChild.lVal - 1, szBuf, ARRAYSIZE(szBuf), TRUE);
            *pszName = SysAllocString(szBuf);
        }
        else
        {
            *pszName = NULL;
        }
        hr = S_OK;
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    return hr;
}

//+-------------------------------------------------------------------------
// Private initialization
//--------------------------------------------------------------------------
BOOL CAutoComplete::_Init()
{
    m_pThread = new CACThread(*this);

#ifdef DEBUG
    // May get freed from background tread so disable leak checking
    if (m_pThread)
    {
        DbgRemoveFromMemList(m_pThread);
    }
#endif

    return (NULL != m_pThread);
}

//+-------------------------------------------------------------------------
// Creates and instance of CAutoComplete
//--------------------------------------------------------------------------
HRESULT CAutoComplete_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    // Note - Aggregation checking is handled in class factory

    *ppunk = NULL;
    CAutoComplete* p = new CAutoComplete();
    if (p)
    {
        if (p->_Init())
        {
            *ppunk = SAFECAST(p, IAutoComplete *);
            return S_OK;
        }

        delete p;
    }

    return E_OUTOFMEMORY;
}

//+-------------------------------------------------------------------------
// Helper function to add default autocomplete functionality to and edit
// window.
//--------------------------------------------------------------------------
HRESULT SHUseDefaultAutoComplete
(
    HWND hwndEdit,
    IBrowserService * pbs,          IN  OPTIONAL
    IAutoComplete2 ** ppac,         OUT OPTIONAL
    IShellService ** ppssACLISF,    OUT OPTIONAL
    BOOL fUseCMDMRU
)
{
    HRESULT hr;
    IUnknown * punkACLMulti;

    if (ppac)
        *ppac = NULL;
    if (ppssACLISF)
        *ppssACLISF = NULL;

    hr = CoCreateInstance(CLSID_ACLMulti, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void **)&punkACLMulti);
    if (SUCCEEDED(hr))
    {
        DbgRemoveFromMemList(punkACLMulti);

        IObjMgr * pomMulti;

        hr = punkACLMulti->QueryInterface(IID_IObjMgr, (LPVOID *)&pomMulti);
        if (SUCCEEDED(hr))
        {
            BOOL fReady = FALSE;   // Fail only if all we are not able to create at least one list.

            // ADD The MRU List
            IUnknown * punkACLMRU;
            hr = CACLMRU_CreateInstance(NULL, &punkACLMRU, NULL, fUseCMDMRU ? SZ_REGKEY_TYPEDURLMRU : SZ_REGKEY_TYPEDCMDMRU);
            if (SUCCEEDED(hr))
            {
                DbgRemoveFromMemList(punkACLMRU);
                pomMulti->Append(punkACLMRU);
                punkACLMRU->Release();
                fReady = TRUE;
            }

            // ADD The History List
            IUnknown * punkACLHist;
            hr = CoCreateInstance(CLSID_ACLHistory, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void **)&punkACLHist);
            if (SUCCEEDED(hr))
            {
                DbgRemoveFromMemList(punkACLHist);
                pomMulti->Append(punkACLHist);
                punkACLHist->Release();
                fReady = TRUE;
            }

            // ADD The ISF List
            IUnknown * punkACLISF;
            hr = CoCreateInstance(CLSID_ACListISF, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void **)&punkACLISF);
            if (SUCCEEDED(hr))
            {
                DbgRemoveFromMemList(punkACLISF);

                // We need to give the ISF AutoComplete List a pointer to the IBrowserService
                // so it can retrieve the current browser location to AutoComplete correctly.
                IShellService * pss;
                hr = punkACLISF->QueryInterface(IID_IShellService, (LPVOID *)&pss);
                if (SUCCEEDED(hr))
                {
                    if (pbs)
                        pss->SetOwner(pbs);

                    if (ppssACLISF)
                        *ppssACLISF = pss;
                    else
                        pss->Release();
                }

                //
                // Set options
                //
                IACList2* pacl;
                if (SUCCEEDED(punkACLISF->QueryInterface(IID_IACList2, (LPVOID *)&pacl)))
                {
                    // Specify directories to search
                    pacl->SetOptions(ACLO_CURRENTDIR | ACLO_FAVORITES | ACLO_MYCOMPUTER | ACLO_DESKTOP);
                    pacl->Release();
                }

                pomMulti->Append(punkACLISF);
                punkACLISF->Release();
                fReady = TRUE;
            }

            if (fReady)
            {
                IAutoComplete2 * pac;

                // Create the AutoComplete Object
                hr = CoCreateInstance(CLSID_AutoComplete, NULL, CLSCTX_INPROC_SERVER, IID_IAutoComplete2, (void **)&pac);
                if (SUCCEEDED(hr))
                {
                    hr = pac->Init(hwndEdit, punkACLMulti, WZ_REGKEY_QUICKCOMPLETE, WZ_DEFAULTQUICKCOMPLETE);
                    if (ppac)
                        *ppac = pac;
                    else
                        pac->Release();
                }
            }

            pomMulti->Release();
        }
        punkACLMulti->Release();
    }

    return hr;
}

/* Private functions */

//+-------------------------------------------------------------------------
// Removes anything that we appended to the edit text
//--------------------------------------------------------------------------
void CAutoComplete::_RemoveCompletion()
{
    TraceMsg(AC_GENERAL, "CAutoComplete::_RemoveCompletion()");
    if (m_fAppended)
    {
        // Remove any highlighted text that we displayed
        Edit_ReplaceSel(m_hwndEdit, TEXT(""));
        m_fAppended = FALSE;
    }
}

//+-------------------------------------------------------------------------
// Updates the text in the edit control
//--------------------------------------------------------------------------
void CAutoComplete::_SetEditText(LPCWSTR psz)
{
    //
    // We set a flag so that we can distinguish between us setting the text
    // and someone else doing it.  If someone else sets the text we hide our
    // dropdown.
    //
    m_fSettingText = TRUE;

    // Don't display our special wildcard search string
    if (psz[0] == CH_WILDCARD)
    {
        Edit_SetText(m_hwndEdit, L"");
    }
    else
    {
        Edit_SetText(m_hwndEdit, psz);
    }

    m_fSettingText = FALSE;
}

//+-------------------------------------------------------------------------
// Removed anything that we appended to the edit text and then updates
// m_pszCurrent with the current string.
//--------------------------------------------------------------------------
void CAutoComplete::_GetEditText(void)
{
    TraceMsg(AC_GENERAL, "CAutoComplete::_GetEditText()");

    _RemoveCompletion();  // remove anything we added

    int iCurrent = GetWindowTextLength(m_hwndEdit);

    //
    // If the current buffer is too small, delete it.
    //
    if (m_pszCurrent &&
         LocalSize(m_pszCurrent) <= (UINT)(iCurrent + 1) * sizeof(TCHAR))
    {
        SetStr(&m_pszCurrent, NULL);
    }

    //
    // If there is no current buffer, try to allocate one
    // with some room to grow.
    //
    if (!m_pszCurrent)
    {
        m_pszCurrent = (LPTSTR)LocalAlloc(LPTR, (iCurrent + (MAX_URL_STRING / 2)) * SIZEOF(TCHAR));
    }

    //
    // If we have a current buffer, get the text.
    //
    if (m_pszCurrent)
    {
        if (!GetWindowText(m_hwndEdit, m_pszCurrent, iCurrent + 1))
        {
            *m_pszCurrent = L'\0';
        }

        // On win9x GetWindowTextLength can return more than the # of characters
        m_iCurrent = lstrlen(m_pszCurrent);
    }
    else
    {
        m_iCurrent = 0;
    }
}

//+-------------------------------------------------------------------------
// Updates the text in the edit control
//--------------------------------------------------------------------------
void CAutoComplete::_UpdateText
(
    int iStartSel,      // start location for selected
    int iEndSel,        // end location of selected text
    LPCTSTR pszCurrent, // unselected text
    LPCTSTR pszNew      // autocompleted (selected) text
)
{
    TraceMsg(AC_GENERAL, "CAutoComplete::_UpdateText(iStart=%i;  iEndSel = %i,  pszCurrent=>%s<,  pszNew=>%s<)",
        iStartSel, iEndSel, (pszCurrent ? pszCurrent : TEXT("(null)")), (pszNew ? pszNew : TEXT("(null)")));

    //
    // Restore the old text.
    //
    _SetEditText(pszCurrent);

    //
    // Put the cursor at the insertion point.
    //
    Edit_SetSel(m_hwndEdit, iStartSel, iStartSel);

    //
    // Insert the new text.
    //
    Edit_ReplaceSel(m_hwndEdit, pszNew);

    //
    // Select the newly added text.
    //
    Edit_SetSel(m_hwndEdit, iStartSel, iEndSel);
}

//+-------------------------------------------------------------------------
// If pwszQuickComplete is NULL, we will use our internal default.
// pwszRegKeyValue can be NULL indicating that there is not a key.
//--------------------------------------------------------------------------
BOOL CAutoComplete::_SetQuickCompleteStrings(LPCOLESTR pwszRegKeyPath, LPCOLESTR pwszQuickComplete)
{
    TraceMsg(AC_GENERAL, "CAutoComplete::SetQuickCompleteStrings(pwszRegKeyPath=0x%x, pwszQuickComplete = 0x%x)",
        pwszRegKeyPath, pwszQuickComplete);


    if (pwszRegKeyPath)
    {
        lstrcpyn(m_szRegKeyPath, pwszRegKeyPath, ARRAYSIZE(m_szRegKeyPath));
    }
    else
    {
        // can be empty
        m_szRegKeyPath[0] = TEXT('\0');
    }

    if (pwszQuickComplete)
    {
        lstrcpyn(m_szQuickComplete, pwszQuickComplete, ARRAYSIZE(m_szQuickComplete));
    }
    else
    {
        // use default value
        lstrcpyn(m_szQuickComplete, c_szAutoDefQuickComp, ARRAYSIZE(m_szQuickComplete));
    }

    return TRUE;
}

//+-------------------------------------------------------------------------
// Formats the current contents of the edit box with the appropriate prefix
// and endfix and returns the completed string.
//--------------------------------------------------------------------------
LPTSTR CAutoComplete::_QuickEnter()
{
    //
    // If they shift-enter, then do the favorite pre/post-fix.
    //
    TCHAR  szFormat[MAX_QUICK_COMPLETE_STRING];
    TCHAR  szNewText[MAX_URL_STRING];
    int    iLen;

    TraceMsg(AC_GENERAL, "CAutoComplete::_QuickEnter()");

    if (NULL == m_pszCurrent)
    {
        return NULL;
    }

    lstrcpyn(szFormat, m_szQuickComplete, ARRAYSIZE(szFormat));
    DWORD cb = sizeof(szFormat);
    SHGetValue(HKEY_CURRENT_USER, m_szRegKeyPath, TEXT("QuickComplete"), NULL, &szFormat, &cb);

    //
    //  Remove preceeding and trailing white space
    //
    PathRemoveBlanks(m_pszCurrent);

    //
    // Make sure we don't GPF.
    //
    iLen = lstrlen(m_pszCurrent) + lstrlen(szFormat);
    if (iLen < ARRAYSIZE(szNewText))
    {
        // If the quick complete is already present, don't add it again
        LPWSTR pszInsertion = StrStrI(szFormat, L"%s");
        LPWSTR pszFormat = szFormat;
        if (pszInsertion)
        {
            // If prefix is already present, don't add it again.
            // (we could improve this to only add parts of the predfix that are missing)
            int iInsertion = (int)(pszInsertion - pszFormat);
            if (iInsertion == 0 || StrCmpNI(pszFormat, m_pszCurrent, iInsertion) == 0)
            {
                // Skip over prefix
                pszFormat = pszInsertion;
            }

            // If postfix is  already present, don't add it again.
            LPWSTR pszPostFix = pszInsertion + ARRAYSIZE(L"%s") - 1;
            int cchCurrent = lstrlen(m_pszCurrent);
            int cchPostFix = lstrlen(pszPostFix);
            if (cchPostFix > 0 && cchPostFix < cchCurrent &&
                StrCmpI(m_pszCurrent + (cchCurrent - cchPostFix), pszPostFix) == 0)
            {
                // Lop off postfix
                *pszPostFix = 0;
            }
        }

        wnsprintf(szNewText, ARRAYSIZE(szNewText), pszFormat, m_pszCurrent);

        SetStr(&m_pszCurrent, szNewText);
    }

    return m_pszCurrent;
}

BOOL CAutoComplete::_ResetSearch(void)
{
    TraceMsg(AC_GENERAL, "CAutoComplete::_ResetSearch()");

    m_dwFlags               = ACF_RESET;
    return TRUE;
}

//+-------------------------------------------------------------------------
// Returns TRUE if the char is a forward or backackwards slash
//--------------------------------------------------------------------------
BOOL CAutoComplete::_IsWhack(TCHAR ch)
{
    return (ch == TEXT('/')) || (ch == TEXT('\\'));
}


//+-------------------------------------------------------------------------
// Returns TRUE if the string points to a character used to separate words
//--------------------------------------------------------------------------
BOOL CAutoComplete::_IsBreakChar(WCHAR wch)
{
    // Do a binary search in our table of break characters
    int iMin = 0;
    int iMax = ARRAYSIZE(g_szBreakChars) - 1;

    while (iMax - iMin >= 2)
    {
        int iTry = (iMax + iMin + 1) / 2;
        if (wch < g_szBreakChars[iTry])
            iMax = iTry;
        else if  (wch > g_szBreakChars[iTry])
            iMin = iTry;
        else
            return TRUE;
    }

    return (wch == g_szBreakChars[iMin] || wch == g_szBreakChars[iMax]);
}

//+-------------------------------------------------------------------------
// Returns TRUE if we want to append to the current edit box contents
//--------------------------------------------------------------------------
BOOL CAutoComplete::_WantToAppendResults()
{
    //
    // Users get annoyed if we append real text after a
    // slash, because they type "c:\" and we complete
    // it to "c:\windows" when they aren't looking.
    //
    // Also, it's annoying to have "\" autocompleted to "\\"
    //
    return (m_pszCurrent &&
            (!(_IsWhack(m_pszCurrent[0]) && m_pszCurrent[1] == NULL) &&
             !_IsWhack(m_pszCurrent[lstrlen(m_pszCurrent)-1])));
}


#ifdef UNIX
extern "C" BOOL MwTranslateUnixKeyBinding( HWND hwnd, DWORD message,
                                           WPARAM *pwParam, DWORD *pModifiers );
#endif


//+-------------------------------------------------------------------------
// Callback routine used by the edit window to determine where to break
// words.  We install this custom callback dor the ctl arrow keys
// recognize our break characters.
//--------------------------------------------------------------------------
int CALLBACK CAutoComplete::EditWordBreakProcW
(
    LPWSTR pszEditText, // pointer to edit text
    int ichCurrent,     // index of starting point
    int cch,            // length in characters of edit text
    int code            // action to take
)
{
    LPWSTR psz = pszEditText + ichCurrent;
    int iIndex;
    BOOL fFoundNonDelimiter = FALSE;
    static BOOL fRight = FALSE;  // hack due to bug in USER

    switch (code)
    {
        case WB_ISDELIMITER:
            fRight = TRUE;
            // Simple case - is the current character a delimiter?
            iIndex = (int)_IsBreakChar(*psz);
            break;

        case WB_LEFT:
            // Move to the left to find the first delimiter.  If we are
            // currently at a delimiter, then skip delimiters until we
            // find the first non-delimiter, then start from there.
            //
            // Special case for fRight - if we are currently at a delimiter
            // then just return the current word!
            while ((psz = CharPrev(pszEditText, psz)) != pszEditText)
            {
                if (_IsBreakChar(*psz))
                {
                    if (fRight || fFoundNonDelimiter)
                        break;
                }
                else
                {
                    fFoundNonDelimiter = TRUE;
                    fRight = FALSE;
                }
            }
            iIndex = (int)(psz - pszEditText);

            // We are currently pointing at the delimiter, next character
            // is the beginning of the next word.
            if (iIndex > 0 && iIndex < cch)
                iIndex++;

            break;

        case WB_RIGHT:
            fRight = FALSE;

            // If we are not at a delimiter, then skip to the right until
            // we find the first delimiter.  If we started at a delimiter, or
            // we have just finished scanning to the first delimiter, then
            // skip all delimiters until we find the first non delimiter.
            //
            // Careful - the string passed in to us may not be NULL terminated!
            fFoundNonDelimiter = !_IsBreakChar(*psz);
            if (psz != (pszEditText + cch))
            {
                while ((psz = CharNext(psz)) != (pszEditText + cch))
                {
                    if (_IsBreakChar(*psz))
                    {
                        fFoundNonDelimiter = FALSE;
                    }
                    else
                    {
                        if (!fFoundNonDelimiter)
                            break;
                    }
                }
            }
            // We are currently pointing at the next word.
            iIndex = (int) (psz - pszEditText);
            break;
    }

    return iIndex;
}

//+-------------------------------------------------------------------------
// Returns the index of the next or previous break character in m_pszCurrent
//--------------------------------------------------------------------------
int CAutoComplete::_JumpToNextBreak
(
    int iLoc,       // current location
    DWORD dwFlags   // direction (WB_RIGHT or WB_LEFT)
)
{
    return EditWordBreakProcW(m_pszCurrent, iLoc, lstrlen(m_pszCurrent), dwFlags);
}

//+-------------------------------------------------------------------------
// Handles Horizontal cursor movement.  Returns TRUE if the message should
// passed on to the OS.  Note that we only call this on win9x.  On NT we 
// use EM_SETWORDBREAKPROC to set a callback instead because it sets the
// caret correctly. This callback can crash on win9x.
//--------------------------------------------------------------------------
BOOL CAutoComplete::_CursorMovement
(
    WPARAM wParam   // virtual key data from WM_KEYDOWN
)
{
    BOOL  fShift, fControl;
    DWORD dwKey = (DWORD)wParam;
    int iStart, iEnd;

    TraceMsg(AC_GENERAL, "CAutoComplete::_CursorMovement(wParam = 0x%x)",
        wParam);

    fShift   = (0 > GetKeyState(VK_SHIFT)) ;
    fControl = (0 > GetKeyState(VK_CONTROL));

    // We don't do anything special unless the SHIFT or CTRL
    // key is down so we don't want to mess up arrowing around
    // UNICODE character clusters. (INDIC o+d+j+d+k+w)
    if (!fShift && !fControl)
        return TRUE;   // let OS handle because of UNICODE char clusters


#ifdef UNIX
    {
        DWORD dwModifiers;
        dwModifiers = 0;
        if ( fShift ) {
            dwModifiers |= FSHIFT;
        }
        if ( fControl ) {
            dwModifiers |= FCONTROL;
        }

        MwTranslateUnixKeyBinding( m_hwndEdit, WM_KEYDOWN,
                                   (WPARAM*) &dwKey, &dwModifiers );

        fShift   = ( dwModifiers & FSHIFT );
        fControl = ( dwModifiers & FCONTROL );
    }
#endif

    // get the current selection
    SendMessage(m_hwndEdit, EM_GETSEL, (WPARAM)&iStart, (LPARAM)&iEnd);

    // the user is editting the text, so this is now invalid.
    m_dwFlags = ACF_RESET;

    _GetEditText();
    if (!m_pszCurrent)
        return TRUE;    // we didn't handle it... let the default wndproc try

    //  Determine the previous selection direction
    int dwSelectionDirection;
    if (iStart == iEnd)
    {
        // Nothing previously selected, so use new direction
        dwSelectionDirection = dwKey;
    }
    else
    {
        // Base the selection direction on whether the caret is positioned
        // at the beginning or the end of the selection
        POINT pt;
        int cchCaret = iEnd;
        if (GetCaretPos(&pt))
        {
            cchCaret = (int)SendMessage(m_hwndEdit, EM_CHARFROMPOS, 0, (LPARAM)MAKELPARAM(pt.x, 0));
        }

        dwSelectionDirection = (cchCaret >= iEnd) ? VK_RIGHT : VK_LEFT;
    }


    if (fControl)
    {
        if (dwKey == VK_RIGHT)
        {
            // did we orginally go to the left?
            if (dwSelectionDirection == VK_LEFT)
            {
                // yes...unselect
                iStart = _JumpToNextBreak(iStart, WB_RIGHT);
 //               if (!iStart)
 //                   iStart = m_iCurrent;
            }
            else if (iEnd != m_iCurrent)
            {
                // select or "jump over" characters
                iEnd = _JumpToNextBreak(iEnd, WB_RIGHT);
 //               if (!iEnd)
 //                   iEnd = m_iCurrent;
            }
        }
        else // dwKey == VK_LEFT
        {
            // did we orginally go to the right?
            if (dwSelectionDirection == VK_RIGHT)
            {
                // yes...unselect
//                int iRemember = iEnd;
                iEnd = _JumpToNextBreak(iEnd, WB_LEFT);
            }
            else if (iStart)  // != 0
            {
                // select or "jump over" characters
                iStart = _JumpToNextBreak(iStart, WB_LEFT);
            }
        } 
    }
    else // if !fControl
    {
        // This code is benign if the SHIFT key isn't down
        // because it has to do with modifying the selection.
        if (dwKey == VK_RIGHT)
        {
            if (dwSelectionDirection == VK_LEFT)
            {
                iStart++;
            }
            else
            {
                iEnd++;
            }
        }
        else // dwKey == VK_LEFT
        {
            LPTSTR pszPrev;
            if (dwSelectionDirection == VK_RIGHT)
            {
                pszPrev = CharPrev(m_pszCurrent, &m_pszCurrent[iEnd]);
                iEnd = (int)(pszPrev - m_pszCurrent);
            }
            else
            {
                pszPrev = CharPrev(m_pszCurrent, &m_pszCurrent[iStart]);
                iStart = (int)(pszPrev - m_pszCurrent);
            }
        }
    }

    // Are we selecting or moving?
    if (!fShift)
    {   // just moving...
        if (dwKey == VK_RIGHT)
        {
            iStart = iEnd;
        }
        else // pachi->dwSelectionDirection == VK_LEFT
        {
            iEnd = iStart;
        }
    }

    //
    // If we are selecting text to the left, we have to jump hoops
    // to get the caret on the left of the selection. Edit_SetSel
    // always places the caret on the right, and if we position the
    // caret ourselves the edit control still uses the old caret
    // position.  So we have to send VK_LEFT messages to the edit
    // control to get it to select things properly.
    //
    if (fShift && dwSelectionDirection == VK_LEFT && iStart < iEnd)
    {
        // Temporarily reset the control key (yuk!)
        BYTE keyState[256];
        if (fControl)
        {
            GetKeyboardState(keyState);
            keyState[VK_CONTROL] &= 0x7f;
            SetKeyboardState(keyState);
        }

        // Select the last character and select left
        // one character at a time.  Arrrggg.
        SendMessage(m_hwndEdit, WM_SETREDRAW, FALSE, 0);
        Edit_SetSel(m_hwndEdit, iEnd, iEnd);
        while (iEnd > iStart)
        {
            DefSubclassProc(m_hwndEdit, WM_KEYDOWN, VK_LEFT, 0);
            --iEnd;
        }
        SendMessage(m_hwndEdit, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(m_hwndEdit, NULL, FALSE);
        UpdateWindow(m_hwndEdit);

        // Restore the control key
        if (fControl)
        {
            keyState[VK_CONTROL] |= 0x80;
            SetKeyboardState(keyState);
        }
    }
    else
    {
        Edit_SetSel(m_hwndEdit, iStart, iEnd);
    }

    return FALSE;   // we handled it
}

//+-------------------------------------------------------------------------
// Process WM_KEYDOWN message.  Returns TRUE if the message should be passed 
// to the original wndproc.
//--------------------------------------------------------------------------
BOOL CAutoComplete::_OnKeyDown(WPARAM wParam)
{
    WPARAM wParamTranslated;

    TraceMsg(AC_GENERAL, "CAutoComplete::_OnKeyDown(wParam = 0x%x)",
        wParam);

    if (m_pThread->IsDisabled())
    {
        //
        // Let the original wndproc handle it.
        //
        return TRUE;
    }

    wParamTranslated = wParam;

#ifdef UNIX
    // Don't pass in HWND as the edit control will and we don't
    // want translation to cause two SendMessages for the same
    // control
    DWORD dwModifiers;
    dwModifiers = 0;
    if (GetKeyState(VK_CONTROL) < 0) {
        dwModifiers |= FCONTROL;
    }

    MwTranslateUnixKeyBinding( NULL, WM_KEYDOWN,
                               &wParamTranslated, &dwModifiers );
#endif

    switch (wParamTranslated)
    {
    case VK_RETURN:
    {
#ifndef UNIX
        if (0 > GetKeyState(VK_CONTROL))
#else
        if (dwModifiers & FCONTROL)
#endif
        {
            //
            // Ctrl-Enter does some quick formatting.
            //
            _GetEditText();
            _SetEditText(_QuickEnter());
        }
        else
        {
            //
            // Reset the search criteria.
            //
            _ResetSearch();

            //
            // Highlight entire text.
            //
            Edit_SetSel(m_hwndEdit, 0, (LPARAM)-1);
        }

        //
        // Stop any searches that are going on.
        //
        _StopSearch();

        //
        // For intelliforms, if the dropdown is visible and something
        // is selected in the dropdown, we simulate an activation event.
        //
        if (m_hwndList)
        {
            int iCurSel = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);
            if ((iCurSel != -1) && m_hwndDropDown && IsWindowVisible(m_hwndDropDown))
            {
                WCHAR szBuf[MAX_URL_STRING];
                _GetItem(iCurSel, szBuf, ARRAYSIZE(szBuf), FALSE);
                SendMessage(m_hwndEdit, m_uMsgItemActivate, 0, (LPARAM)szBuf);
            }
        }

        //
        // Hide the dropdown
        //
        _HideDropDown();

        // bugbug: For some reason, the original windproc is ignoring the return key.
        //         It should hide the dropdown!
        if (m_hwndCombo)
        {
            SendMessage(m_hwndCombo, CB_SHOWDROPDOWN, FALSE, 0);
        }

        //
        // Let the original wndproc handle it.
        //
        break;
    }
    case VK_ESCAPE:
        _StopSearch();
        _HideDropDown();

        // bugbug: For some reason, the original windproc is ignoring the enter key.
        //         It should hide the dropdown!
        if (m_hwndCombo)
        {
            SendMessage(m_hwndCombo, CB_SHOWDROPDOWN, FALSE, 0);
        }
        break;

    case VK_LEFT:
    case VK_RIGHT:
        // We do our own cursor movement on win9x because EM_SETWORDBREAKPROC is broken.
        if (!g_fRunningOnNT)
        {
            return _CursorMovement(wParam);
        }
        break;

    case VK_PRIOR:
    case VK_UP:
        if (!(m_dwFlags & ACF_IGNOREUPDOWN) && !_IsComboboxDropped())
        {
            //
            // If the dropdown is visible, the up-down keys navigate our list
            //
            if (m_hwndDropDown && IsWindowVisible(m_hwndDropDown))
            {
                int iCurSel = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);

                if (iCurSel == 0)
                {
                    // If at top, move back up into the edit box
                    // Deselect the dropdown and select the edit box
                    ListView_SetItemState(m_hwndList, 0, 0, 0x000f);
                    if (m_pszCurrent)
                    {
                        // Restore original text if they arrow out of listview
                        _SetEditText(m_pszCurrent);
                    }
                    Edit_SetSel(m_hwndEdit, MAX_URL_STRING, MAX_URL_STRING);
                }
                else if (iCurSel != -1)
                {
                    // If in middle or at bottom, move up
                    SendMessage(m_hwndList, WM_KEYDOWN, wParam, 0);
                    SendMessage(m_hwndList, WM_KEYUP, wParam, 0);
                }
                else
                {
                    int iSelect = ListView_GetItemCount(m_hwndList)-1;

                    // If in edit box, move to bottom
                    ListView_SetItemState(m_hwndList, iSelect, LVIS_SELECTED|LVIS_FOCUSED, 0x000f);
                    ListView_EnsureVisible(m_hwndList, iSelect, FALSE);
                }
                return FALSE;
            }

            //
            // If Autosuggest drop-down enabled but not popped up then start a search
            // based on the current edit box contents.  If the edit box is empty,
            // search for everything.
            //
            else if ((m_dwOptions & ACO_UPDOWNKEYDROPSLIST) && _IsAutoSuggestEnabled())
            {
                // Ensure the background thread knows we have focus
                _GotFocus();
                _StartCompletion(FALSE, TRUE);
                return FALSE;
            }

            //
            // Otherwise we see if we should append the completions in place
            //
            else if (_IsAutoAppendEnabled())
            {
                if (_AppendPrevious(FALSE))
                {
                    return FALSE;
                }
            }
        }
        break;
    case VK_NEXT:
    case VK_DOWN:
        if (!(m_dwFlags & ACF_IGNOREUPDOWN) && !_IsComboboxDropped())
        {
            //
            // If the dropdown is visible, the up-down keys navigate our list
            //
            if (m_hwndDropDown && IsWindowVisible(m_hwndDropDown))
            {
                ASSERT(m_hdpa);
                ASSERT(DPA_GetPtrCount(m_hdpa) != 0);
                ASSERT(m_iFirstMatch != -1);

                int iCurSel = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);
                if (iCurSel == -1)
                {
                    // If no item selected, first down arrow selects first item
                    ListView_SetItemState(m_hwndList, 0, LVIS_SELECTED | LVIS_FOCUSED, 0x000f);
                    ListView_EnsureVisible(m_hwndList, 0, FALSE);
                }
                else if (iCurSel == ListView_GetItemCount(m_hwndList)-1)
                {
                    // If last item selected, down arrow goes into edit box
                    ListView_SetItemState(m_hwndList, iCurSel, 0, 0x000f);
                    if (m_pszCurrent)
                    {
                        // Restore original text if they arrow out of listview
                        _SetEditText(m_pszCurrent);
                    }
                    Edit_SetSel(m_hwndEdit, MAX_URL_STRING, MAX_URL_STRING);
                }
                else
                {
                    // If first or middle item selected, down arrow selects next item
                    SendMessage(m_hwndList, WM_KEYDOWN, wParam, 0);
                    SendMessage(m_hwndList, WM_KEYUP, wParam, 0);
                }
                return FALSE;
            }

            //
            // If Autosuggest drop-down enabled but not popped up then start a search
            // based on the current edit box contents.  If the edit box is empty,
            // search for everything.
            //
            else if ((m_dwOptions & ACO_UPDOWNKEYDROPSLIST) && _IsAutoSuggestEnabled())
            {
                // Ensure the background thread knows we have focus
                _GotFocus();
                _StartCompletion(FALSE, TRUE);
                return FALSE;
            }

            //
            // Otherwise we see if we should append the completions in place
            //
            else if (_IsAutoAppendEnabled())
            {
                if (_AppendNext(FALSE))
                {
                    return FALSE;
                }
            }
        }
        break;

    case VK_END:
    case VK_HOME:
        _ResetSearch();
        break;

    case VK_BACK:
        //
        // Indicate that selection doesn't match m_psrCurrentlyDisplayed.
        //
#ifndef UNIX
        if (0 > GetKeyState(VK_CONTROL))
#else
        if (dwModifiers & FCONTROL)
#endif
        {
            //
            // Handle Ctrl-Backspace to delete word.
            //
            int iStart, iEnd;
            SendMessage(m_hwndEdit, EM_GETSEL, (WPARAM)&iStart, (LPARAM)&iEnd);

            //
            // Nothing else must be selected.
            //
            if (iStart == iEnd)
            {
                _GetEditText();
                if (!m_pszCurrent)
                {
                    //
                    // We didn't handle it, let the
                    // other wndprocs try.
                    //
                    return TRUE;
                }
                //
                // Erase the "word".
                //
                iStart = EditWordBreakProcW(m_pszCurrent, iStart, iStart+1, WB_LEFT);
                Edit_SetSel(m_hwndEdit, iStart, iEnd);
                Edit_ReplaceSel(m_hwndEdit, TEXT(""));
            }

            //
            // We handled it.
            //
            return FALSE;
        }
        break;
    }

    //
    // Let the original wndproc handle it.
    //
    return TRUE;
}

LRESULT CAutoComplete::_OnChar(WPARAM wParam, LPARAM lParam)
{
    LRESULT lres = 0;   // means nothing, but we handled the call
    TCHAR   cKey = (TCHAR) wParam;

    if (wParam == VK_TAB)
    {
        // Ignore tab characters
        return 0;
    }

    // Ensure the background thread knows we have focus
    _GotFocus();

    if (m_pThread->IsDisabled())
    {
        //
        // Just follow the chain.
        //
        return DefSubclassProc(m_hwndEdit, WM_CHAR, wParam, lParam);
    }

    if (cKey != 127 && cKey != VK_ESCAPE && cKey != VK_RETURN && cKey != 0x0a)    // control-backspace is ignored
    {
        // let the default edit wndproc do its thing first
        lres = DefSubclassProc(m_hwndEdit, WM_CHAR, wParam, lParam);

        // ctrl-c is generating a VK_CANCEL.  Don't bring up autosuggest in this case.
        if (cKey != VK_CANCEL)
        {
            BOOL fAppend = (cKey != VK_BACK);
            _StartCompletion(fAppend);
        }
    }
    else
    {
        _StopSearch();
        _HideDropDown();
    }

    return lres;
}

//+-------------------------------------------------------------------------
// Starts autocomplete based on the current editbox contents
//--------------------------------------------------------------------------
void CAutoComplete::_StartCompletion
(
    BOOL fAppend,       // Ok to append completion in edit box
    BOOL fEvenIfEmpty   // = FALSE, Completes to everything if edit box is empty
)
{
    // Get the text typed in
    WCHAR szCurrent[MAX_URL_STRING];
    int cchCurrent = GetWindowText(m_hwndEdit, szCurrent, ARRAYSIZE(szCurrent));

    // See if we want a wildcard search
    if (fEvenIfEmpty && cchCurrent == 0)
    {
        cchCurrent = 1;
        szCurrent[0] = CH_WILDCARD;
        szCurrent[1] = 0;
    }

    // If unchanged, we are done
    if (m_pszLastSearch && m_pszCurrent && StrCmpI(m_pszCurrent, szCurrent) == 0)
    {
        if (!(m_hwndDropDown && IsWindowVisible(m_hwndDropDown)) &&
            (-1 != m_iFirstMatch) && _IsAutoSuggestEnabled() &&

            // Don't show drop-down if only one exact match (IForms)
            (m_hdpa &&
             ((m_iLastMatch != m_iFirstMatch) || (((CACString*)DPA_GetPtr(m_hdpa, m_iFirstMatch))->StrCmpI(szCurrent) != 0))))
        {
            _ShowDropDown();
        }
        return;
    }

    // Save the current text
    if (szCurrent[0] == CH_WILDCARD)
    {
        SetStr(&m_pszCurrent, szCurrent);
    }
    else
    {
        _GetEditText();
    }

    //
    // Deselect the current selection in the dropdown
    //
    if (m_hwndList)
    {
        int iCurSel = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);
        if (iCurSel != -1)
        {
            ListView_SetItemState(m_hwndList, iCurSel, 0, 0x000f);
        }
    }

    //
    // If nothing typed in, stop any pending search
    //
    if (cchCurrent == 0)
    {
        if (m_pszCurrent)
        {
            _StopSearch();
            if (m_pszCurrent)
            {
                SetStr(&m_pszCurrent, NULL);
            }

            // bugbug: Free last completion
            _HideDropDown();
        }
    }

    //
    // See if we need to generate a new list
    //
    else
    {
        int iCompleted = m_pszLastSearch ? lstrlen(m_pszLastSearch) : 0;
        int iScheme;

        // Get length of common prefix (if any)
        int cchPrefix = IsFlagSet(m_dwOptions, ACO_FILTERPREFIXES) ?
                            CACThread::GetSpecialPrefixLen(szCurrent) : 0;

        if  (
             // If no previous completion, start a new search
             (0 == iCompleted) ||

             // If the list was truncated (reached limit), we need to refetch
             m_fNeedNewList ||

             // We purge matches to common prefixes ("www.", "http://" etc). If the
             // last search may have resulted in items being filtered out, and the
             // new string will not, then we need to refetch.
             (cchPrefix > 0 && cchPrefix < cchCurrent && CACThread::MatchesSpecialPrefix(m_pszLastSearch)) ||

             // If the portion we last completed to was altered, we need to refetch
             (StrCmpNI(m_pszLastSearch, szCurrent, iCompleted) != 0) ||

             // If we have entered a new folder, we need to refetch
             (StrChrI(szCurrent + iCompleted, DIR_SEPARATOR_CHAR) != NULL) ||

             // If we have entered a url folder, we need to refetch (ftp://shapitst/Bryanst/)
             ((StrChrI(szCurrent + iCompleted, URL_SEPARATOR_CHAR) != NULL) &&
              (URL_SCHEME_FTP == (iScheme = GetUrlScheme(szCurrent))))
            )
        {
            // If the last search was truncated, make sure we try the next search with more characters
            int cchMin = cchPrefix + 1;
            if (m_fNeedNewList)
            {
                cchMin = iCompleted + 1;
            }

            // Find the last '\\' (or '/' for ftp)
            int i = cchCurrent - 1;
            while ((szCurrent[i] != DIR_SEPARATOR_CHAR) &&
                    !((szCurrent[i] == URL_SEPARATOR_CHAR) && (iScheme == URL_SCHEME_FTP)) &&
                    (i >= cchMin))
            {
                --i;
            }

            // Start a new search
            szCurrent[i+1] = 0;
            if (_StartSearch(szCurrent))
                SetStr(&m_pszLastSearch, szCurrent);
        }

        // Otherwise we can simply update from our last completion list
        else
        {
            //
            if (m_hdpa)
            {
                _UpdateCompletion(szCurrent, -1, fAppend);
            }
            else
            {
                // Awaiting completion, cache new match...
            }
        }
    }
}

//+-------------------------------------------------------------------------
// Get the background thread to start a new search
//--------------------------------------------------------------------------
BOOL CAutoComplete::_StartSearch(LPCWSTR pszSeatch)
{
    // Empty the dropdown list.  To minimize flash, we don't hide it unless
    // the search comes up empty
    if (m_hwndList)
    {
        ListView_SetItemCountEx(m_hwndList, 0, 0);
    }

    return m_pThread->StartSearch(pszSeatch, m_dwOptions);
}

//+-------------------------------------------------------------------------
// Get the background thread to abort the last search
//--------------------------------------------------------------------------
void CAutoComplete::_StopSearch()
{
    SetStr(&m_pszLastSearch, NULL);
    m_pThread->StopSearch();
}

//+-------------------------------------------------------------------------
// Informs the background thread that we have focus.
//--------------------------------------------------------------------------
void CAutoComplete::_GotFocus()
{
    if (!m_pThread->HasFocus())
    {
        m_pThread->GotFocus();
    }
}

//+-------------------------------------------------------------------------
// Message from background thread indicating that the search was completed
//--------------------------------------------------------------------------
void CAutoComplete::_OnSearchComplete
(
    HDPA hdpa,          // New completion list
    BOOL fLimitReached  // if this is a partial list
)
{
    _FreeDPAPtrs(m_hdpa);
    m_hdpa = hdpa;
    m_fNeedNewList = fLimitReached;

    // Was it a wildcard search?
    BOOL fWildCard = m_pszLastSearch && (m_pszLastSearch[0] == CH_WILDCARD) && (m_pszLastSearch[1] == L'\0');

    //
    // See if we should add "Search for <stuff typed in>" to the end of
    // the list.
    //
    m_fSearchForAdded = FALSE;

    if (!fWildCard && (m_dwOptions & ACO_SEARCH))
    {
        // Add "Search for <stuff typed in>" to the end of the list

        // First make sure we have a dpa
        if (m_hdpa == NULL)
        {
            m_hdpa = DPA_Create(AC_LIST_GROWTH_CONST);
        }

        if (m_hdpa)
        {
            // Create a bogus entry and add to the end of the list. This place
            // holder makes sure the drop-down does not go away when there are no
            // matching entries.
            CACString* pStr = CreateACString(L"", 0);
            if (DPA_AppendPtr(m_hdpa, pStr) == -1)
            {
                pStr->Release();
            }
            else
            {
                m_fSearchForAdded = TRUE;
            }
        }
    }

    // If no search results, hide our dropdown
    if (NULL == m_hdpa || 0 == DPA_GetPtrCount(m_hdpa))
    {
        _HideDropDown();
        if (m_hwndList)
        {
            ListView_SetItemCountEx(m_hwndList, 0, 0);
        }
        m_iFirstMatch = -1;
    }
    else
    {
        if (m_pszCurrent)
        {
            // If we are still waiting for a completion, then update the completion list
            if (m_pszLastSearch)
            {
                _UpdateCompletion(m_pszCurrent, -1, TRUE);
            }
        }

        if (m_hwndDropDown && IsWindowVisible(m_hwndDropDown))
        {
            _PositionDropDown();        // Resize based on number of hits
            _UpdateScrollbar();
        }
    }
}

//+-------------------------------------------------------------------------
// Returns the text for an item in the autocomplete list
//--------------------------------------------------------------------------
BOOL CAutoComplete::_GetItem
(
    int iIndex,         // zero-based index
    LPWSTR pszText,     // location to return text
    int cchMax,         // size of pszText buffer
    BOOL fDisplayName   // TRUE = return name to display
                        // FALSE = return name to go to edit box
)
{
    // Check for special "Search for <typed in>" entry at end of the list
    if (m_fSearchFor && iIndex == m_iLastMatch - m_iFirstMatch)
    {
        ASSERT(NULL != m_pszCurrent);

        WCHAR szFormat[MAX_PATH];
        int id = fDisplayName ? IDS_SEARCHFOR : IDS_SEARCHFORCMD;

        MLLoadString(id, szFormat, ARRAYSIZE(szFormat));
        wnsprintf(pszText, cchMax, szFormat, m_pszCurrent);
    }

    // Normal list entry
    else
    {
        CACString* pStr = (CACString*)DPA_GetPtr(m_hdpa, iIndex + m_iFirstMatch);
        if (pStr)
        {
            StrCpyN(pszText, pStr->GetStr(), cchMax);
        }
        else if (cchMax >= 1)
        {
            pszText[0] = 0;
        }
    }
    return TRUE;
}

//+-------------------------------------------------------------------------
// Frees an item in our autocomplete list
//--------------------------------------------------------------------------
int CAutoComplete::_DPADestroyCallback(LPVOID p, LPVOID d)
{
    ((CACString*)p)->Release();
    return 1;
}

//+-------------------------------------------------------------------------
// Frees our last completion list
//--------------------------------------------------------------------------
void CAutoComplete::_FreeDPAPtrs(HDPA hdpa)
{
    TraceMsg(AC_GENERAL, "CAutoComplete::_FreeDPAPtrs(hdpa = 0x%x)", hdpa);

    if (hdpa)
    {
        DPA_DestroyCallback(hdpa, _DPADestroyCallback, 0);
    }
}

//+-------------------------------------------------------------------------
// Updates the matching completion
//--------------------------------------------------------------------------
void CAutoComplete::_UpdateCompletion
(
    LPCWSTR pszTyped,   // typed in string to match
    int iChanged,       // char added since last update or -1
    BOOL fAppend        // ok to append completion
)
{
    int iFirstMatch = -1;
    int iLastMatch = -1;
    int nChars = lstrlen(pszTyped);

    // Was it a wildcard search?
    BOOL fWildCard = pszTyped && (pszTyped[0] == CH_WILDCARD) && (pszTyped[1] == L'\0');
    if (fWildCard && DPA_GetPtrCount(m_hdpa))
    {
        // Everything matches
        iFirstMatch = 0;
        iLastMatch = DPA_GetPtrCount(m_hdpa) - 1;
    }
    else
    {
        // PERF: Special case where current == search string
    /*
        //
        // Find the first matching index
        //
        if (iChanged > 0)
        {
            // PERF: Get UC and LC versions of WC for compare below?
            WCHAR wc = pszTyped[iChanged];

            // A character was added so search from current location
            for (int i = m_iFirstMatch; i < DPA_GetPtrCount(m_hdpa); ++i)
            {
                CACString* pStr;

                pStr = (CACString*)DPA_GetPtr(m_hdpa, i);
                ASSERT(pStr);
                if (pStr && pStr->GetLength() >= iChanged && ChrCmpI((*pStr)[iChanged], wc) == 0)
                {
                    // This is the first match
                    iFirstMatch = i;
                    break;
                }
            }
        }
        else
    */
        {
            // Damn, we have to search the whole list
            // PERF: Switch to a binary search.
            for (int i = 0; i < DPA_GetPtrCount(m_hdpa); ++i)
            {
                CACString* pStr;

                pStr = (CACString*)DPA_GetPtr(m_hdpa, i);
                ASSERT(pStr);
                if (pStr &&
                    (pStr->StrCmpNI(pszTyped, nChars) == 0))
                {
                    iFirstMatch = i;
                    break;
                }
            }
        }

        if (-1 != iFirstMatch)
        {
            //
            // Find the last match
            //
            // PERF: Should we binary search up to the last end of list?
            for (iLastMatch = iFirstMatch; iLastMatch + 1 < DPA_GetPtrCount(m_hdpa); ++iLastMatch)
            {
                CACString* pStr;

                pStr = (CACString*)DPA_GetPtr(m_hdpa, iLastMatch + 1);
                ASSERT(pStr);
                if (NULL == pStr || (pStr->StrCmpNI(pszTyped, nChars) != 0))
                {
                    break;
                }
            }
        }
    }

    //
    // See if we should add "Search for <stuff typed in>" to the end of
    // the list.
    //
    int iSearchFor = 0;
    int nScheme;

    if (m_fSearchForAdded &&

        // Not a drive letter
        (*pszTyped && pszTyped[1] != L':') &&

        // Not a UNC path
        (pszTyped[0] != L'\\' && pszTyped[1] != L'\\') &&

        // Not a known scheme
        ((nScheme = GetUrlScheme(pszTyped)) == URL_SCHEME_UNKNOWN ||
        nScheme == URL_SCHEME_INVALID) &&

        // Ignore anything theat begins with "www"
        !(pszTyped[0] == L'w' && pszTyped[1] == L'w' && pszTyped[2] == L'w')

        // Not a search keyword
//        !Is
        )
    {
        // Add "Search for <stuff typed in>"
        iSearchFor = 1;
    }
    m_fSearchFor = iSearchFor;

    m_iLastMatch = iLastMatch + iSearchFor;
    m_iFirstMatch = iFirstMatch;
    if (iSearchFor && iFirstMatch == -1)
    {
        // There is one entry - the special "search for <>" entry
        m_iFirstMatch = 0;
    }

    if (_IsAutoSuggestEnabled())
    {
        // Update our drop-down list
        if ((m_iFirstMatch == -1) ||                // Hide if there are no matches
            ((m_iLastMatch == m_iFirstMatch) &&     // Or if only one match which we've already typed (IForms)
                (((CACString*)DPA_GetPtr(m_hdpa, m_iFirstMatch))->StrCmpI(pszTyped) == 0)))
        {
            _HideDropDown();
        }
        else
        {
            if (m_hwndList)
            {
                int cItems = m_iLastMatch - m_iFirstMatch + 1;
                ListView_SetItemCountEx(m_hwndList, cItems, 0);
            }

            _ShowDropDown();
            _UpdateScrollbar();
        }
    }

    if (_IsAutoAppendEnabled() && fAppend && m_iFirstMatch != -1 && !fWildCard)
    {
        // If caret is not at the end of the string, don't append
        DWORD dwSel = Edit_GetSel(m_hwndEdit);
        int iStartSel = LOWORD(dwSel);
        int iEndSel = HIWORD(dwSel);
        int iLen = lstrlen(pszTyped);
        if (iStartSel == iStartSel && iStartSel != iLen)
        {
            return;
        }

        // Bugbug: Should use the shortest match
        m_iAppended = -1;
        _AppendNext(TRUE);
    }
}

//+-------------------------------------------------------------------------
// Appends the next completion to the current edit text.  Returns TRUE if
// successful.
//--------------------------------------------------------------------------
BOOL CAutoComplete::_AppendNext
(
    BOOL fAppendToWhack  // Apend to next whack (false = append entire match)
)
{
    // Nothing to complete?
    if (NULL == m_hdpa || 0 == DPA_GetPtrCount(m_hdpa) ||
        m_iFirstMatch == -1 || !_WantToAppendResults())
        return FALSE;

    //
    // If nothing currently appended, init to the
    // last item so that we will wrap around to the
    // first item
    //
    if (m_iAppended == -1)
    {
        m_iAppended = m_iLastMatch;
    }

    int iAppend = m_iAppended;
    CACString* pStr;

    //
    // Loop through the items until we find one without a prefix
    //
    do
    {
        if (++iAppend > m_iLastMatch)
        {
            iAppend = m_iFirstMatch;
        }
        pStr = (CACString*)DPA_GetPtr(m_hdpa, iAppend);
        if (pStr && 

            // Don't append if match has as www. prefix
            (pStr->PrefixLength() < 4 || StrCmpNI(pStr->GetStr() + pStr->PrefixLength() - 4, L"www.", 4) != 0) &&

            // Ignore the "Search for" if present
            !(m_fSearchFor && iAppend == m_iLastMatch))
        {
            // We found one so append it
            _Append(*pStr, fAppendToWhack);
            m_iAppended = iAppend;
        }
    }
    while (iAppend != m_iAppended);
    return TRUE;
}

//+-------------------------------------------------------------------------
// Appends the previous completion to the current edit text.  Returns TRUE
// if successful.
//--------------------------------------------------------------------------
BOOL CAutoComplete::_AppendPrevious
(
    BOOL fAppendToWhack  // Append to next whack (false = append entire match)
)
{
    // Nothing to complete?
    if (NULL == m_hdpa || 0 == DPA_GetPtrCount(m_hdpa) ||
        m_iFirstMatch == -1 || !_WantToAppendResults())
        return FALSE;

    //
    // If nothing currently appended, init to the
    // first item so that we will wrap around to the
    // last item
    //
    if (m_iAppended == -1)
    {
        m_iAppended = m_iFirstMatch;
    }

    int iAppend = m_iAppended;
    CACString* pStr;

    //
    // Loop through the items until we find one without a prefix
    //
    do
    {
        if (--iAppend < m_iFirstMatch)
        {
            iAppend = m_iLastMatch;
        }
        pStr = (CACString*)DPA_GetPtr(m_hdpa, iAppend);
        if (pStr &&

            // Don't append if match has as www. prefix
            (pStr->PrefixLength() < 4 || StrCmpNI(pStr->GetStr() + pStr->PrefixLength() - 4, L"www.", 4) != 0) &&

            // Ignore the "Search for" if present
            !(m_fSearchFor && iAppend == m_iLastMatch))
        {
            // We found one so append it
            _Append(*pStr, fAppendToWhack);
            m_iAppended = iAppend;
        }
    }
    while (iAppend != m_iAppended);

    return TRUE;
}

//+-------------------------------------------------------------------------
// Appends the completion to the current edit text
//--------------------------------------------------------------------------
void CAutoComplete::_Append
(
    CACString& rStr,    // item to append to the editbox text
    BOOL fAppendToWhack  // Apend to next whack (false = append entire match)
)
{
    ASSERT(_IsAutoAppendEnabled());

    if (m_pszCurrent)
    {
        int cchCurrent = lstrlen(m_pszCurrent);
        LPCWSTR pszAppend = rStr.GetStrToCompare() + cchCurrent;
        int cchAppend;

        if (fAppendToWhack)
        {
            //
            // Advance to the whacks.
            //
            const WCHAR *pch = pszAppend;
            cchAppend = 0;

            while (*pch && !_IsWhack(*pch))
            {
                ++cchAppend;
                pch++;
            }

            //
            // Advance past the whacks.
            //
            while (*pch && _IsWhack(*pch))
            {
                ++cchAppend;
                pch++;
            }
        }
        else
        {
            // Append entire match
            cchAppend = lstrlen(pszAppend);
        }

        WCHAR szAppend[MAX_URL_STRING];
        StrCpyN(szAppend, pszAppend, cchAppend + 1);
        _UpdateText(cchCurrent, cchCurrent + cchAppend, m_pszCurrent, szAppend);

        m_fAppended = TRUE;
    }
}

//+-------------------------------------------------------------------------
// Hides the AutoSuggest dropdown
//--------------------------------------------------------------------------
void CAutoComplete::_HideDropDown()
{
    if (m_hwndDropDown)
    {
        ShowWindow(m_hwndDropDown, SW_HIDE);
    }
}

//+-------------------------------------------------------------------------
// Shows and positions the autocomplete dropdown
//--------------------------------------------------------------------------
void CAutoComplete::_ShowDropDown()
{
    if (m_hwndDropDown && !_IsComboboxDropped() && !m_fImeCandidateOpen)
    {
        // If the edit window is visible, it better have focus!
        // (Intelliforms uses an invisible window that doesn't
        // get focus.)
        if (IsWindowVisible(m_hwndEdit) && m_hwndEdit != GetFocus())
        {
            ShowWindow(m_hwndDropDown, SW_HIDE);
            return;
        }
    
        if (!IsWindowVisible(m_hwndDropDown))
        {
            // It should not be possible to open a new dropdown while
            // another dropdown is visible!  But to be safe we'll check ...
            if (s_hwndDropDown)
            {
                ASSERT(FALSE);
                ShowWindow(s_hwndDropDown, SW_HIDE);
            }

            s_hwndDropDown = m_hwndDropDown;

            //
            // Install a thread hook so that we can detect when something
            // happens that should hide the dropdown.
            //
            ENTERCRITICAL;
            if (s_hhookMouse)
            {
                // Should never happen because the hook is removed when the dropdown
                // is hidden.  But we can't afford to orphan a hook so we check just
                // in case!
                ASSERT(FALSE);
                UnhookWindowsHookEx(s_hhookMouse);
            }
            s_hhookMouse = SetWindowsHookEx(WH_MOUSE, s_MouseHook, HINST_THISDLL, NULL);
            LEAVECRITICAL;

            //
            // Subclass the parent windows so that we can detect when something
            // happens that should hide the dropdown
            //
            _SubClassParent(m_hwndEdit);
        }

        _PositionDropDown();
    }
}

//+-------------------------------------------------------------------------
// Positions dropdown based on edit window position
//--------------------------------------------------------------------------
void CAutoComplete::_PositionDropDown()
{
    RECT rcEdit;
    GetWindowRect(m_hwndEdit, &rcEdit);
    int x = rcEdit.left;
    int y = rcEdit.bottom;

    // Don't resize if user already has
    if (!m_fDropDownResized)
    {
#ifndef UNIX
        m_nDropHeight = 100;
#else
        m_nDropHeight = 150;
#endif
        MINMAXINFO mmi = {0};
        SendMessage(m_hwndDropDown, WM_GETMINMAXINFO, 0, (LPARAM)&mmi);
        m_nDropWidth = max(RECTWIDTH(rcEdit), mmi.ptMinTrackSize.x);

        // Calculate dropdown height based on number of string matches
        if (m_hdpa)
        {
/*
            int iDropDownHeight =
                    m_nStatusHeight +
                    ListView_GetItemSpacing(m_hwndList, FALSE) * DPA_GetPtrCount(m_hdpa);
*/

            int iDropDownHeight =
                    m_nStatusHeight - GetSystemMetrics(SM_CYBORDER) +
                    HIWORD(ListView_ApproximateViewRect(m_hwndList, -1, -1, -1));

            if (m_nDropHeight > iDropDownHeight)
            {
                m_nDropHeight = iDropDownHeight;
            }
        }
    }

    int w = m_nDropWidth;
    int h = m_nDropHeight;

    //
    // Make sure we don't go off the screen
    //
    HMONITOR hMonitor = MonitorFromWindow(m_hwndEdit, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi;
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hMonitor, &mi);
    RECT rcMon = mi.rcMonitor;
    int cxMax = rcMon.right - rcMon.left;
    if (w > cxMax)
    {
        w = cxMax;
    }

/*
    if (x < rcMon.left)
    {
        // Off the left edge, so move right
        x += rcMon.left - x;
    }
    else if (x + w > rcMon.right)
    {
        // Off the right edge, so move left
        x -= (x + w - rcMon.right);
    }
*/
    int cyMax = (RECTHEIGHT(rcMon) - RECTHEIGHT(rcEdit));
    if (h > cyMax)
    {
        h = cyMax;
    }

    BOOL fDroppedUp = FALSE;
    if (y + h > rcMon.bottom

#ifdef ALLOW_ALWAYS_DROP_UP
        || m_fAlwaysDropUp
#endif

        )
    {
        // Off the bottom of the screen, so see if there is more
        // room in the up direction
        if (rcEdit.top > rcMon.bottom - rcEdit.bottom)
        {
            // There's more room to pop up
            y = max(rcEdit.top - h, 0);
            h = rcEdit.top - y;
            fDroppedUp = TRUE;
        }
        else
        {
            // Don't let it go past the bottom
            h = rcMon.bottom - y;
        }
    }

    BOOL fFlipped = BOOLIFY(m_fDroppedUp) ^ BOOLIFY(fDroppedUp);
    m_fDroppedUp = fDroppedUp;

    SetWindowPos(m_hwndDropDown, HWND_TOP, x, y, w, h, SWP_SHOWWINDOW | SWP_NOACTIVATE);

    if (fFlipped)
    {
        _UpdateGrip();
    }
}

//+-------------------------------------------------------------------------
// Window procedure for the subclassed edit box
//--------------------------------------------------------------------------
LRESULT CAutoComplete::_EditWndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_SETTEXT:
        //
        // If the text is changed programmatically, we hide the dropdown.
        // This fixed a bug in the dialog at:
        //
        // Internet Options\security\Local Intranet\Sites\Advanced
        //
        //   - If you select something in the dropdown the press enter,
        //     the enter key is intercepeted by the dialog which clears
        //     the edit field, but the drop-down is not hidden.
        //
        if (!m_fSettingText)
        {
            _HideDropDown();
        }
        break;

    case WM_GETDLGCODE:
        {
            //
            // If the auto-suggest drop-down if up, we process
            // the tab key.
            //
            BOOL fDropDownVisible = m_hwndDropDown && IsWindowVisible(m_hwndDropDown);

            if (wParam == VK_TAB && IsFlagSet(m_dwOptions, ACO_USETAB))
            {
#ifndef UNIX
                if ((GetKeyState(VK_CONTROL) < 0) ||
                    !fDropDownVisible)
                {
                    break;
                }

#else
                //
                // On unix, an unmodified tab key is processed even if autosuggest
                // is not visible.
                //

                if ((GetKeyState(VK_CONTROL) < 0) ||
                    ((GetKeyState(VK_SHIFT) < 0) && !fDropDownVisible))
                {
                    break;
                }

                if (!fDropDownVisible)
                {
                    //
                    // Make tab key autocomplete key if at end of edit control
                    //
                    UINT cchTotal = SendMessage(m_hwndEdit, EM_LINELENGTH, 0, 0L);
                    DWORD ichMinSel;
                    SendMessage(m_hwndEdit, EM_GETSEL, (WPARAM) &ichMinSel, 0L);

                    if (ichMinSel == cchTotal)
                    {
                        break;
                    }
                }
#endif  // UNIX

                // We want the tab key
                return DLGC_WANTTAB;
            }
            else if (wParam == VK_ESCAPE && fDropDownVisible)
            {
                // eat escape so that dialog boxes (e.g. File Open) are not closed
                return DLGC_WANTALLKEYS;
            }
            break;
        }

    case WM_KEYDOWN:
        if (wParam == VK_TAB)
        {
            BOOL fDropDownVisible = m_hwndDropDown && IsWindowVisible(m_hwndDropDown);
#ifdef UNIX
            if (!fDropDownVisible &&
                (GetKeyState(VK_CONTROL) >= 0) &&
                (GetKeyState(VK_SHIFT) >= 0)
               )
            {
                wParam = VK_END;
            }
            else
#endif
            if (fDropDownVisible &&
                GetKeyState(VK_CONTROL) >= 0)
            {
                // Map tab to down-arrow and shift-tab to up-arrow
                wParam = (GetKeyState(VK_SHIFT) >= 0) ? VK_DOWN : VK_UP;
            }
            else
            {
                return 0;
            }
        }

        // Ensure the background thread knows we have focus
        _GotFocus();

//            ASSERT(m_hThread || m_pThread->IsDisabled());  // If this occurs then we didn't process a WM_SETFOCUS when we should have.  BryanSt.
        if (_OnKeyDown(wParam) == 0)
        {
            //
            // We handled it.
            //
            return 0;
        }

        if (wParam == VK_DELETE)
        {
            LRESULT lRes = DefSubclassProc(m_hwndEdit, uMsg, wParam, lParam);
            _StartCompletion(FALSE);
            return lRes;
        }
        break;

    case WM_CHAR:
        return _OnChar(wParam, lParam);

    case WM_CUT:
    case WM_PASTE:
    case WM_CLEAR:
    {
        LRESULT lRet = DefSubclassProc(m_hwndEdit, uMsg, wParam, lParam);

        // See if we need to update the completion
        if (!m_pThread->IsDisabled())
        {
            _GotFocus();
            _StartCompletion(TRUE);
        }
        return lRet;
    }
    case WM_SETFOCUS:
        m_pThread->GotFocus();
        break;

    case WM_KILLFOCUS:
        {
            HWND hwndGetFocus = (HWND)wParam;

            // Ignore focus change to ourselves
            if (m_hwndEdit != hwndGetFocus)
            {
                if (m_hwndDropDown && GetFocus() != m_hwndDropDown)
                {
                    _HideDropDown();
                }
                m_pThread->LostFocus();
            }
            break;
        }
    case WM_DESTROY:
        {
            HWND hwndEdit = m_hwndEdit;
            TraceMsg(AC_GENERAL, "CAutoComplete::_WndProc(WM_DESTROY) releasing subclass.");

            RemoveWindowSubclass(hwndEdit, s_EditWndProc, 0);

            if (m_hwndDropDown)
            {
                DestroyWindow(m_hwndDropDown);
                m_hwndDropDown = NULL;
                m_hwndList = NULL;
                m_hwndScroll = NULL;
                m_hwndGrip = NULL;
            }

            m_pThread->SyncShutDownBGThread();
            SAFERELEASE(m_pThread);
            Release();      // Release subclass Ref.

            // Pass it onto the old wndproc.
            return DefSubclassProc(hwndEdit, uMsg, wParam, lParam);
        }
        break;
    case WM_MOVE:
        {
            if (m_hwndDropDown && IsWindowVisible(m_hwndDropDown))
            {
                // Follow edit window, for example when intelliforms window scrolls w/intellimouse
                _PositionDropDown();
            }
        }
        break;

/*
    case WM_COMMAND:
        if (m_pThread->IsDisabled())
        {
            break;
        }
        return _OnCommand(wParam, lParam);
*/
/*
    case WM_CONTEXTMENU:
        if (m_pThread->IsDisabled())
        {
            break;
        }
        return _ContextMenu(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
*/

#ifndef UNIX
    case WM_LBUTTONDBLCLK:
    {
        //
        // Bypass our word break routine.  We only register this callback on NT because it
        // doesn't work right on win9x.
        //
        if (g_fRunningOnNT && IsWindowUnicode(m_hwndEdit))
        {
            //
            // We break words at url delimiters for ctrl-left & ctrl-right, but
            // we want double-click to use standard word selection so that it is easy
            // to select the URL.
            //
            SendMessage(m_hwndEdit, EM_SETWORDBREAKPROC, 0, (DWORD_PTR)m_oldEditWordBreakProc);

            LRESULT lres = DefSubclassProc(m_hwndEdit, uMsg, wParam, lParam);

            // Restore our word-break callback
            SendMessage(m_hwndEdit, EM_SETWORDBREAKPROC, 0, (DWORD_PTR)EditWordBreakProcW);
            return lres;
        }
        break;
    }
#endif // !UNIX

    case WM_SETFONT:
    {
        // If we have a dropdown, recreate it with the latest font
        if (m_hwndDropDown)
        {
            _StopSearch();
            DestroyWindow(m_hwndDropDown);
            m_hwndDropDown = NULL;
            m_hwndList = NULL;
            m_hwndScroll = NULL;
            m_hwndGrip = NULL;

            _SeeWhatsEnabled();
        }
        break;
    }
    case WM_IME_NOTIFY:
        {
            // We don't want autocomplete to obsure the IME candidate window
            DWORD dwCommand = (DWORD)wParam;
            if (dwCommand == IMN_OPENCANDIDATE)
            {
                m_fImeCandidateOpen = TRUE;
                _HideDropDown();
            }
            else if (dwCommand == IMN_CLOSECANDIDATE)
            {
                m_fImeCandidateOpen = FALSE;
            }
        }
        break;
    default:
        // Handle registered messages
        if (uMsg == m_uMsgSearchComplete)
        {
            _OnSearchComplete((HDPA)lParam, (BOOL)wParam);
            return 0;
        }

        // Pass mouse wheel messages to the drop-down if it is visible
        else if ((uMsg == WM_MOUSEWHEEL || uMsg == g_msgMSWheel) &&
            m_hwndDropDown && IsWindowVisible(m_hwndDropDown))
        {
            SendMessage(m_hwndList, uMsg, wParam, lParam);
            return 0;
        }
        break;
    }


    return DefSubclassProc(m_hwndEdit, uMsg, wParam, lParam);
}

//+-------------------------------------------------------------------------
// Static window procedure for the subclassed edit box
//--------------------------------------------------------------------------
LRESULT CALLBACK CAutoComplete::s_EditWndProc
(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    UINT_PTR uIdSubclass,   // always zero for us
    DWORD_PTR dwRefData     // -> CAutoComplete
)
{
    CAutoComplete* pac = (CAutoComplete*)dwRefData;
    ASSERT(pac);
    if (pac)
    {
        ASSERT(pac->m_hwndEdit == hwnd);
        return pac->_EditWndProc(uMsg, wParam, lParam);
    }
    else
    {
        return DefWindowProcWrap(hwnd, uMsg, wParam, lParam);
    }
}

//+-------------------------------------------------------------------------
// Draws the sizing grip. We do this ourselves rather than call
// DrawFrameControl because the standard API does not flip upside down on
// all platforms.  (NT and win98 seem to use a font and thus ignore the map
// mode)
//--------------------------------------------------------------------------
BOOL DrawGrip(register HDC hdc, LPRECT lprc, BOOL fEraseBackground)
{
    int x, y;
    int xMax, yMax;
    int dMin;
    HBRUSH hbrOld;
    HPEN hpen, hpenOld;
    DWORD rgbHilight, rgbShadow;

    //
    // The grip is really a pattern of 4 repeating diagonal lines:
    //      One glare
    //      Two raised
    //      One empty
    // These lines run from bottom left to top right, in the bottom right
    // corner of the square given by (lprc->left, lprc->top, dMin by dMin.
    //
    dMin = min(lprc->right-lprc->left, lprc->bottom-lprc->top);
    xMax = lprc->left + dMin;
    yMax = lprc->top + dMin;

    //
    // Setup colors
    //
    hbrOld      = GetSysColorBrush(COLOR_3DFACE);
    rgbHilight  = GetSysColor(COLOR_3DHILIGHT);
    rgbShadow   = GetSysColor(COLOR_3DSHADOW);

    //
    // Fill in background of ENTIRE rect
    //
    if (fEraseBackground)
    {
        hbrOld = SelectBrush(hdc, hbrOld);
        PatBlt(hdc, lprc->left, lprc->top, lprc->right-lprc->left,
                lprc->bottom-lprc->top, PATCOPY);
        SelectBrush(hdc, hbrOld);
    }
    else
    {
/*
        hpen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_WINDOW));
        if (hpen == NULL)
            return FALSE;
        hpenOld = SelectPen(hdc, hpen);

        x = lprc->left - 1;
        y = lprc->top - 1;
        MoveToEx(hdc, x, yMax, NULL);
        LineTo(hdc, xMax, y);

        SelectPen(hdc, hpenOld);
        DeletePen(hpen);
*/

        //
        // Draw background color directly under grip:
        //
        hpen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DFACE));
        if (hpen == NULL)
            return FALSE;
        hpenOld = SelectPen(hdc, hpen);

        x = lprc->left + 3;
        y = lprc->top + 3;
        while (x < xMax)
        {
            //
            // Since dMin is the same horz and vert, x < xMax and y < yMax
            // are interchangeable...
            //
            MoveToEx(hdc, x, yMax, NULL);
            LineTo(hdc, xMax, y);

            // Skip 3 lines in between
            x += 4;
            y += 4;
        }

        SelectPen(hdc, hpenOld);
        DeletePen(hpen);
    }

    //
    // Draw glare with COLOR_3DHILIGHT:
    //      Create proper pen
    //      Select into hdc
    //      Starting at lprc->left, draw a diagonal line then skip the
    //          next 3
    //      Select out of hdc
    //
    hpen = CreatePen(PS_SOLID, 1, rgbHilight);
    if (hpen == NULL)
        return FALSE;
    hpenOld = SelectPen(hdc, hpen);

    x = lprc->left;
    y = lprc->top;
    while (x < xMax)
    {
        //
        // Since dMin is the same horz and vert, x < xMax and y < yMax
        // are interchangeable...
        //

        MoveToEx(hdc, x, yMax, NULL);
        LineTo(hdc, xMax, y);

        // Skip 3 lines in between
        x += 4;
        y += 4;
    }

    SelectPen(hdc, hpenOld);
    DeletePen(hpen);

    //
    // Draw raised part with COLOR_3DSHADOW:
    //      Create proper pen
    //      Select into hdc
    //      Starting at lprc->left+1, draw 2 diagonal lines, then skip
    //          the next 2
    //      Select outof hdc
    //
    hpen = CreatePen(PS_SOLID, 1, rgbShadow);
    if (hpen == NULL)
        return FALSE;
    hpenOld = SelectPen(hdc, hpen);

    x = lprc->left+1;
    y = lprc->top+1;
    while (x < xMax)
    {
        //
        // Draw two diagonal lines touching each other.
        //

        MoveToEx(hdc, x, yMax, NULL);
        LineTo(hdc, xMax, y);

        x++;
        y++;

        MoveToEx(hdc, x, yMax, NULL);
        LineTo(hdc, xMax, y);

        //
        // Skip 2 lines inbetween
        //
        x += 3;
        y += 3;
    }

    SelectPen(hdc, hpenOld);
    DeletePen(hpen);

    return TRUE;
}

//+-------------------------------------------------------------------------
// Update the visible characteristics of the gripper depending on whether
// the dropdown is "dropped up" or the scrollbar is visible
//--------------------------------------------------------------------------
void CAutoComplete::_UpdateGrip()
{
    if (m_hwndGrip)
    {
        //
        // If we have a scrollbar the gripper has a rectangular shape.
        //
        if (m_hwndScroll && IsWindowVisible(m_hwndScroll))
        {
            SetWindowRgn(m_hwndGrip, NULL, FALSE);
        }
        //
        // Otherwise, give it  a trinagular window region
        //
        else
        {
            int nWidth = GetSystemMetrics(SM_CXVSCROLL);
            int nHeight = GetSystemMetrics(SM_CYHSCROLL);
            POINT rgpt[3] =
            {
                {nWidth, 0},
                {nWidth, nHeight},
                {0, nHeight},
            };

            //
            // If dropped up, convert the "bottom-Right" tringle into
            // a "top-right" triangle
            //
            if (m_fDroppedUp)
            {
                rgpt[2].y = 0;
            }

            HRGN hrgn = CreatePolygonRgn(rgpt, ARRAYSIZE(rgpt), WINDING);
            if (hrgn && !SetWindowRgn(m_hwndGrip, hrgn, TRUE))
                DeleteObject(hrgn);
        }
        InvalidateRect(m_hwndGrip, NULL, TRUE);
    }
}

//+-------------------------------------------------------------------------
// Transfer the listview scroll info into our scrollbar control
//--------------------------------------------------------------------------
void CAutoComplete::_UpdateScrollbar()
{
    if (m_hwndScroll)
    {
        SCROLLINFO si;
        si.cbSize = sizeof(si);
        si.fMask = SIF_ALL;
        BOOL fScrollVisible = IsWindowVisible(m_hwndScroll);

        if (GetScrollInfo(m_hwndList, SB_VERT, &si))
        {
            SetScrollInfo(m_hwndScroll, SB_CTL, &si, TRUE);
            UINT nRange = si.nMax - si.nMin;
            BOOL fShow = (nRange != 0) && (nRange != (UINT)(si.nPage - 1));
            ShowScrollBar(m_hwndScroll, SB_CTL, fShow);
            if (BOOLIFY(fScrollVisible) ^ BOOLIFY(fShow))
            {
                _UpdateGrip();
            }
        }
    }
}

//+-------------------------------------------------------------------------
// Window procedure for the AutoSuggest drop-down
//--------------------------------------------------------------------------
LRESULT CAutoComplete::_DropDownWndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_NCCREATE:
        {
            //
            // Add a listview to the dropdown
            //
            m_hwndList = CreateWindowEx(0,
                                        WC_LISTVIEW,
                                        c_szAutoSuggestTitle,
                                        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | LVS_REPORT | LVS_NOCOLUMNHEADER | LVS_SINGLESEL | LVS_OWNERDATA | LVS_OWNERDRAWFIXED,
                                        0, 0, 30000, 30000,
                                        m_hwndDropDown, NULL, HINST_THISDLL, NULL);

            if (m_hwndList)
            {
                // Subclass the listview window
                if (SetProp(m_hwndList, c_szAutoCompleteProp, this))
                {
                    // point it to our wndproc and save the old one
                    m_pOldListViewWndProc = (WNDPROC)SetWindowLongPtr(m_hwndList, GWLP_WNDPROC, (LONG_PTR) &s_ListViewWndProc);
                }

                ListView_SetExtendedListViewStyle(m_hwndList, LVS_EX_FULLROWSELECT | LVS_EX_ONECLICKACTIVATE | LVS_EX_TRACKSELECT);

                LV_COLUMN lvColumn;
                lvColumn.mask = LVCF_FMT | LVCF_WIDTH;
                lvColumn.fmt = LVCFMT_LEFT;
                lvColumn.cx = LISTVIEW_COLUMN_WIDTH;
                ListView_InsertColumn(m_hwndList, 0, &lvColumn);

                // We'll get the default dimensions when we first show it
                m_nDropWidth = 0;
                m_nDropHeight = 0;

                // Add a scrollbar
                m_hwndScroll = CreateWindowEx(0, L"scrollbar", NULL,
                                WS_CHILD | SBS_VERT | SBS_RIGHTALIGN,
                                0, 0, 20, 100, m_hwndDropDown, 0, HINST_THISDLL, NULL);

                // Add a sizebox
                m_hwndGrip = CreateWindowEx(0, L"scrollbar", NULL,
                                WS_CHILD | WS_VISIBLE | SBS_SIZEBOX | SBS_SIZEBOXBOTTOMRIGHTALIGN,
                                0, 0, 20, 100, m_hwndDropDown, 0, HINST_THISDLL, NULL);
                if (m_hwndGrip)
                {
                    SetWindowSubclass(m_hwndGrip, s_GripperWndProc, 0, (ULONG_PTR)this);
                    _UpdateGrip();
                }
            }
            return (m_hwndList != NULL);
        }
        case WM_DESTROY:
        {
            //
            // I'm paranoid - should happen when we're hidden
            //
            if (s_hwndDropDown != NULL && s_hwndDropDown == m_hwndDropDown)
            {
                // Should never happen, but we take extra care not to leak a window hook!
                ASSERT(FALSE);

                ENTERCRITICAL;
                if (s_hhookMouse)
                {
                    UnhookWindowsHookEx(s_hhookMouse);
                    s_hhookMouse = NULL;
                }
                LEAVECRITICAL;
                s_hwndDropDown = NULL;
            }
            _UnSubClassParent(m_hwndEdit);

            // Unsubclass this window
            SetWindowLongPtr(m_hwndDropDown, GWLP_USERDATA, (LONG_PTR)NULL);
            break;
        }
        case WM_SYSCOLORCHANGE:
            SendMessage(m_hwndList, uMsg, wParam, lParam);
            break;

        case WM_WININICHANGE:
            SendMessage(m_hwndList, uMsg, wParam, lParam);
            if (wParam == SPI_SETNONCLIENTMETRICS)
            {
                _UpdateGrip();
            }
            break;

        case WM_GETMINMAXINFO:
        {
            //
            // Don't shrink smaller than the size of the gripper 
            //
            LPMINMAXINFO pMmi = (LPMINMAXINFO)lParam;

            pMmi->ptMinTrackSize.x = GetSystemMetrics(SM_CXVSCROLL);
            pMmi->ptMinTrackSize.y = GetSystemMetrics(SM_CYHSCROLL);
            return 0;
        }
        case WM_MOVE:
        {
            //
            // Reposition the list view in case we switch between dropping-down
            // and dropping up.
            //
            RECT rc;
            GetClientRect(m_hwndDropDown, &rc);
            int nWidth = RECTWIDTH(rc);
            int nHeight = RECTHEIGHT(rc);

            int cxGrip = GetSystemMetrics(SM_CXVSCROLL);
            int cyGrip = GetSystemMetrics(SM_CYHSCROLL);

            if (m_fDroppedUp)
            {
                SetWindowPos(m_hwndGrip, HWND_TOP, nWidth - cxGrip, 0, cxGrip, cyGrip, SWP_NOACTIVATE);
                SetWindowPos(m_hwndScroll, HWND_TOP, nWidth - cxGrip, cyGrip, cxGrip, nHeight-cyGrip, SWP_NOACTIVATE);
            }
            else
            {
                SetWindowPos(m_hwndGrip, HWND_TOP, nWidth - cxGrip, nHeight - cyGrip, cxGrip, cyGrip, SWP_NOACTIVATE);
                SetWindowPos(m_hwndScroll, HWND_TOP, nWidth - cxGrip, 0, cxGrip, nHeight-cyGrip, SWP_NOACTIVATE);
            }
            break;
        }
        case WM_SIZE:
        {
            int nWidth  = LOWORD(lParam);
            int nHeight = HIWORD(lParam);

            int cxGrip = GetSystemMetrics(SM_CXVSCROLL);
            int cyGrip = GetSystemMetrics(SM_CYHSCROLL);

            if (m_fDroppedUp)
            {
                SetWindowPos(m_hwndGrip, HWND_TOP, nWidth - cxGrip, 0, cxGrip, cyGrip, SWP_NOACTIVATE);
                SetWindowPos(m_hwndScroll, HWND_TOP, nWidth - cxGrip, cyGrip, cxGrip, nHeight-cyGrip, SWP_NOACTIVATE);
            }
            else
            {
                SetWindowPos(m_hwndGrip, HWND_TOP, nWidth - cxGrip, nHeight - cyGrip, cxGrip, cyGrip, SWP_NOACTIVATE);
                SetWindowPos(m_hwndScroll, HWND_TOP, nWidth - cxGrip, 0, cxGrip, nHeight-cyGrip, SWP_NOACTIVATE);
            }

            // Save the new dimensions
            m_nDropWidth = nWidth + 2*GetSystemMetrics(SM_CXBORDER);
            m_nDropHeight = nHeight + 2*GetSystemMetrics(SM_CYBORDER);

            MoveWindow(m_hwndList, 0, 0, LISTVIEW_COLUMN_WIDTH + 10*cxGrip, nHeight, TRUE);
            _UpdateScrollbar();
            InvalidateRect(m_hwndList, NULL, FALSE);
            break;
        }

        case WM_NCHITTEST:
            {
                RECT rc;
                POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

                // If the in the grip, show the sizing cursor
                if (m_hwndGrip)
                {
                    GetWindowRect(m_hwndGrip, &rc);
                
                    if (PtInRect(&rc, pt))
                    {
                        if(IS_WINDOW_RTL_MIRRORED(m_hwndDropDown))
                        {
                            return (m_fDroppedUp) ? HTTOPLEFT : HTBOTTOMLEFT;                        
                        }
                        else
                        {
                            return (m_fDroppedUp) ? HTTOPRIGHT : HTBOTTOMRIGHT;                        
                        }
                    }
                }
            }
            break;

        case WM_SHOWWINDOW:
            {
                s_fNoActivate = FALSE;

                BOOL fShow = (BOOL)wParam;
                if (!fShow)
                {
                    //
                    // We are being hidden so we no longer need to
                    // subclass the parent windows.
                    //
                    _UnSubClassParent(m_hwndEdit);

                    //
                    // Remove the mouse hook.  We shouldn't need to protect this global with
                    // a critical section because another dropdown cannot be shown
                    // before we are hidden.  But we don't want to chance orphaning a hook
                    // so to be safe we protect write access to this variable.
                    //
                    ENTERCRITICAL;
                    if (s_hhookMouse)
                    {
                        UnhookWindowsHookEx(s_hhookMouse);
                        s_hhookMouse = NULL;
                    }
                    LEAVECRITICAL;

                    s_hwndDropDown = NULL;

                    // Deselect the current selection
                    int iCurSel = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);
                    if (iCurSel)
                    {
                        ListView_SetItemState(m_hwndList, iCurSel, 0, 0x000f);
                    }
                }
            }
            break;

        case WM_MOUSEACTIVATE:
            //
            // We don't want mouse clicks to activate us and
            // take focus from the edit box.
            //
            return (LRESULT)MA_NOACTIVATE;

        case WM_NCLBUTTONDOWN:
            //
            // We don't want resizing to activate us and deactivate the app.
            // The WM_MOUSEACTIVATE message above prevents mouse downs from
            // activating us, but mouse up after a resize still activates us.
            //
            if (wParam == HTBOTTOMRIGHT ||
                wParam == HTTOPRIGHT)
            {
                s_fNoActivate = TRUE;
            }
            break;

        case WM_VSCROLL:
        {
            ASSERT(m_hwndScroll);

            //
            // Pass the scroll messages from our control to the listview
            //
            WORD nScrollCode = LOWORD(wParam);
            if (nScrollCode == SB_THUMBTRACK || nScrollCode == SB_THUMBPOSITION)
            {
                //
                // The listview ignores the 16-bit position passed in and 
                // queries the internal window scrollbar for the tracking
                // position.  Since this returns the wrong track position,
                // we have to handle thumb tracking ourselves.
                //
                WORD nPos = HIWORD(wParam);

                SCROLLINFO si;
                si.cbSize = sizeof(si);
                si.fMask = SIF_ALL;

                if (GetScrollInfo(m_hwndScroll, SB_CTL, &si))
                {
                    //
                    // The track position is always at the top of the list.
                    // So, if we are scrolling up, make sure that the track
                    // position is visible.  Otherwise we need to ensure
                    // that a full page is visible below the track positon.
                    //
                    int nEnsureVisible = si.nTrackPos;
                    if (si.nTrackPos > si.nPos)
                    {
                        nEnsureVisible += si.nPage - 1;
                    }
                    SendMessage(m_hwndList, LVM_ENSUREVISIBLE, nEnsureVisible, FALSE);
                }
            }
            else         
            {
                // Let listview handle it
                SendMessage(m_hwndList, uMsg, wParam, lParam);
            }
            _UpdateScrollbar();
            return 0;
        }
        case WM_EXITSIZEMOVE:
            //
            // Resize operation is over so permit the app to lose acitvation
            //
            s_fNoActivate = FALSE;
            m_fDropDownResized = TRUE;
            return 0;

        case WM_DRAWITEM:
        {
            //
            // We need to draw the contents of the list view ourselves
            // so that we can show items in the selected state even
            // when the edit control has focus.
            //
            LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;
            if (pdis->itemID != -1)
            {
                HDC hdc = pdis->hDC;
                RECT rc = pdis->rcItem;
                BOOL fTextHighlight = pdis->itemState & ODS_SELECTED;

                // Setup the dc before we use it.
                BOOL fRTLReading = GetWindowLong(pdis->hwndItem, GWL_EXSTYLE) & WS_EX_RTLREADING;
                UINT uiOldTextAlign;
                if (fRTLReading)
                {
                    uiOldTextAlign = GetTextAlign(hdc);
                    SetTextAlign(hdc, uiOldTextAlign | TA_RTLREADING);
                }

                SetBkColor(hdc, GetSysColor(fTextHighlight ?
                                COLOR_HIGHLIGHT : COLOR_WINDOW));
                SetTextColor(hdc, GetSysColor(fTextHighlight ?
                                COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));

                // Center the string vertically within rc
                SIZE sizeText;
                WCHAR szText[MAX_URL_STRING];
                _GetItem(pdis->itemID, szText, ARRAYSIZE(szText), TRUE);
                int cch = lstrlen(szText);
                GetTextExtentPoint(hdc, szText, cch, &sizeText);
                int yMid = (rc.top + rc.bottom) / 2;
                int yString = yMid - (sizeText.cy/2);
                int xString = 5;

                //
                // If this is a .url string, don't display the extension
                //
                if (cch > 4 && StrCmpNI(szText + cch - 4, L".url", 4) == 0)
                {
                    cch -= 4;
                }

                ExtTextOut(hdc, xString, yString, ETO_OPAQUE | ETO_CLIPPED, &rc, szText, cch, NULL);

                // Restore the text align in the dc.
                if (fRTLReading)
                {
                    SetTextAlign(hdc, uiOldTextAlign);
                }
            }
            break;
        }
        case WM_NOTIFY:
        {
            //
            // Respond to notification messages from the list view
            //
            LPNMHDR pnmhdr = (LPNMHDR)lParam;
            switch (pnmhdr->code)
            {
                case LVN_GETDISPINFO:
                {
                    //
                    // Return the text for an autosuggest item
                    //
                    ASSERT(pnmhdr->hwndFrom == m_hwndList);
                    LV_DISPINFO* pdi = (LV_DISPINFO*)lParam;
                    if (pdi->item.mask & LVIF_TEXT)
                    {
                        _GetItem(pdi->item.iItem, pdi->item.pszText, pdi->item.cchTextMax + 1, TRUE);
                    }
                    break;
                }
                case LVN_ITEMCHANGED:
                {
                    //
                    // When an item is selected in the list view, we transfer it to the
                    // edit control.  But only if the selection was not caused by the
                    // mouse passing over an element (hot tracking).
                    //
                    if (!m_fInHotTracking)
                    {
                        LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
                        if ((pnmv->uChanged & LVIF_STATE) && (pnmv->uNewState & (LVIS_FOCUSED | LVIS_SELECTED)))
                        {
                            WCHAR szBuf[MAX_URL_STRING];
                            _GetItem(pnmv->iItem, szBuf, ARRAYSIZE(szBuf), FALSE);

                            // Copy the selection to the edit box and place caret at the end
                            _SetEditText(szBuf);
                            int cch = lstrlen(szBuf);
                            Edit_SetSel(m_hwndEdit, cch, cch);
                        }
                    }

                    //
                    // Update the scrollbar.  Note that we have to post a message to do this
                    // after returning from this function.  Otherwise we get old info
                    // from the listview about the scroll positon.
                    //
                    PostMessage(m_hwndDropDown, AM_UPDATESCROLLPOS, 0, 0);
                    break;
                }
                case LVN_ITEMACTIVATE:
                {
                    //
                    // Someone activated an item in the listview. We want to make sure that
                    // the items is selected (without hot tracking) so that the contents
                    // are moved to the edit box, and then simulate and enter key press.
                    //

                    LPNMITEMACTIVATE lpnmia = (LPNMITEMACTIVATE)lParam;
                    WCHAR szBuf[MAX_URL_STRING];
                    _GetItem(lpnmia->iItem, szBuf, ARRAYSIZE(szBuf), FALSE);

                    // Copy the selection to the edit box and place caret at the end
                    _SetEditText(szBuf);
                    int cch = lstrlen(szBuf);
                    Edit_SetSel(m_hwndEdit, cch, cch);

                    //
                    // Intelliforms don't want an enter key because this would submit the
                    // form, so first we try sending a notification.
                    //
                    if (SendMessage(m_hwndEdit, m_uMsgItemActivate, 0, (LPARAM)szBuf) == 0)
                    {
                        // Not an intelliform, so simulate an enter key instead.
                        SendMessage(m_hwndEdit, WM_KEYDOWN, VK_RETURN, 0);
                        SendMessage(m_hwndEdit, WM_KEYUP, VK_RETURN, 0);
                    }
                    _HideDropDown();
                    break;
                }
                case LVN_HOTTRACK:
                {
                    //
                    // Select items as we mouse-over them
                    //
                    LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW)lParam;
                    LVHITTESTINFO lvh;
                    lvh.pt = lpnmlv->ptAction;
                    int iItem = ListView_HitTest(m_hwndList, &lvh);
                    if (iItem != -1)
                    {
                        // Update the current selection. The m_fInHotTracking flag prevents the
                        // edit box contents from being updated
                        m_fInHotTracking = TRUE;
                        ListView_SetItemState(m_hwndList, iItem, LVIS_SELECTED|LVIS_FOCUSED, 0x000f);
                        SendMessage(m_hwndList, LVM_ENSUREVISIBLE, iItem, FALSE);
                        m_fInHotTracking = FALSE;
                    }

                    // We processed this...
                    return TRUE;
                }
            }
            break;
        }
        case AM_UPDATESCROLLPOS:
        {
            if (m_hwndScroll)
            {
                int nTop = ListView_GetTopIndex(m_hwndList);
                SetScrollPos(m_hwndScroll, SB_CTL, nTop, TRUE);
            }
            break;
        }
        case AM_BUTTONCLICK:
        {
            //
            // This message is sent by the thread hook when a mouse click is detected outside
            // the drop-down window.  Unless the click occurred inside the combobox, we will
            // hide the dropdown.
            //
            MOUSEHOOKSTRUCT *pmhs = (MOUSEHOOKSTRUCT*)lParam;
            HWND hwnd = pmhs->hwnd;
            RECT rc;

            if (hwnd != m_hwndCombo && hwnd != m_hwndEdit &&

                // See if we clicked within the bounds of the editbox.  This is
                // necessary for intelliforms.
                // bugbug: This assumes that the editbox is entirely visible!
                GetWindowRect(m_hwndEdit, &rc) && !PtInRect(&rc, pmhs->pt))
            {
                _HideDropDown();
            }
            return 0;
        }
    }

    return DefWindowProcWrap(m_hwndDropDown, uMsg, wParam, lParam);
}

//+-------------------------------------------------------------------------
// Static window procedure for the AutoSuggest drop-down
//--------------------------------------------------------------------------
LRESULT CALLBACK CAutoComplete::s_DropDownWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CAutoComplete* pThis;
    if (uMsg == WM_NCCREATE)
    {
        pThis = (CAutoComplete*)((LPCREATESTRUCT)lParam)->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) pThis);
        pThis->m_hwndDropDown = hwnd;
    }
    else
    {
        pThis = (CAutoComplete*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    if (pThis)
    {
        ASSERT(pThis->m_hwndDropDown == hwnd);
        return pThis->_DropDownWndProc(uMsg, wParam, lParam);
    }
    else
    {
        return DefWindowProcWrap(hwnd, uMsg, wParam, lParam);
    }
}

//+-------------------------------------------------------------------------
// We subclass the listview to prevent it from activating the drop-down
// when someone clicks on it.
//--------------------------------------------------------------------------
LRESULT CAutoComplete::_ListViewWndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRet;
    switch (uMsg)
    {
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
            //
            // Prevent mouse clicks from activating this view
            //
            s_fNoActivate = TRUE;
            lRet = CallWindowProc(m_pOldListViewWndProc, m_hwndList, uMsg, wParam, lParam);
            s_fNoActivate = FALSE;
            return 0;

        case WM_DESTROY:
            // Restore old wndproc.
            RemoveProp(m_hwndList, c_szAutoCompleteProp);
            if (m_pOldListViewWndProc)
            {
                SetWindowLongPtr(m_hwndList, GWLP_WNDPROC, (LONG_PTR) m_pOldListViewWndProc);
                lRet = CallWindowProc(m_pOldListViewWndProc, m_hwndList, uMsg, wParam, lParam);
                m_pOldListViewWndProc = NULL;
            }
            return 0;

        case WM_GETOBJECT:
            if (lParam == OBJID_CLIENT)
            {
                SAFERELEASE(m_pDelegateAccObj);

                if (SUCCEEDED(CreateStdAccessibleObject(m_hwndList, 
                                                        OBJID_CLIENT, 
                                                        IID_IAccessible, 
                                                        (void **)&m_pDelegateAccObj)))
                {
                    return LresultFromObject(IID_IAccessible, wParam, SAFECAST(this, IAccessible *));
                }
            }
            break;

        case WM_NCHITTEST:
        {
            RECT rc;
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

            // If in the grip area, let our parent handle it
            if (m_hwndGrip)
            {
                GetWindowRect(m_hwndGrip, &rc);
            
                if (PtInRect(&rc, pt))
                {
                    return HTTRANSPARENT;
                }
            }
            break;
        }
    }
    lRet = CallWindowProc(m_pOldListViewWndProc, m_hwndList, uMsg, wParam, lParam);
    return lRet;
}

//+-------------------------------------------------------------------------
// Static window procedure for the subclassed listview
//--------------------------------------------------------------------------
LRESULT CAutoComplete::s_ListViewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CAutoComplete* pac = (CAutoComplete*)GetProp(hwnd, c_szAutoCompleteProp);
    ASSERT(pac);
    if (pac)
    {
        return pac->_ListViewWndProc(uMsg, wParam, lParam);
    }
    else
    {
        return DefWindowProcWrap(hwnd, uMsg, wParam, lParam);
    }
}

//+-------------------------------------------------------------------------
// This message hook is only installed when the AutoSuggest dropdown
// is visible.  It hides the dropdown if you click on any window other than
// the dropdown or associated editbox/combobox.
//--------------------------------------------------------------------------
LRESULT CALLBACK CAutoComplete::s_MouseHook(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0)
    {
        MOUSEHOOKSTRUCT *pmhs = (MOUSEHOOKSTRUCT*)lParam;
        ASSERT(pmhs);

        switch (wParam)
        {
            case WM_LBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_NCLBUTTONDOWN:
            case WM_NCMBUTTONDOWN:
            case WM_NCRBUTTONDOWN:
            {
                HWND hwnd = pmhs->hwnd;

                // If the click was outside the edit/combobox/dropdown, then
                // hide the dropdown.
                if (hwnd != s_hwndDropDown)
                {
                    // Ignore if the button was clicked in the dropdown
                    RECT rc;
                    if (GetWindowRect(s_hwndDropDown, &rc) && !PtInRect(&rc, pmhs->pt))
                    {
                        // Inform the dropdown
                        SendMessage(s_hwndDropDown, AM_BUTTONCLICK, 0, (LPARAM)pmhs);
                    }
                }
                break;
            }
        }
    }

    return CallNextHookEx(s_hhookMouse, nCode, wParam, lParam);
}

//+-------------------------------------------------------------------------
// Subclasses all of the parents of hwnd so we can determine when they
// are moved, deactivated, or clicked on.  We use these events to signal
// the window that has focus to hide its autocomplete dropdown. This is
// similar to the CB_SHOWDROPDOWN message sent to comboboxes, but we cannot
// assume that we are autocompleting a combobox.
//--------------------------------------------------------------------------
void CAutoComplete::_SubClassParent
(
    HWND hwnd   // window to notify of events
)
{
    //
    // Subclass all the parent windows because any of them could cause
    // the position of hwnd to change which should hide the dropdown.
    //
    HWND hwndParent = hwnd;
    DWORD dwThread = GetCurrentThreadId();

    while (hwndParent = GetParent(hwndParent))
    {
        // Only subclass if this window is owned by our thread
        if (dwThread == GetWindowThreadProcessId(hwndParent, NULL))
        {
            SetWindowSubclass(hwndParent, s_ParentWndProc, 0, (ULONG_PTR)this);
        }
    }
}

//+-------------------------------------------------------------------------
// Unsubclasses all of the parents of hwnd.  We use the helper functions in
// comctl32 to safely unsubclass a window even if someone else subclassed
// the window after us.
//--------------------------------------------------------------------------
void CAutoComplete::_UnSubClassParent
(
    HWND hwnd   // window to notify of events
)
{
    HWND hwndParent = hwnd;
    DWORD dwThread = GetCurrentThreadId();

    while (hwndParent = GetParent(hwndParent))
    {
        // Only need to unsubclass if this window is owned by our thread
        if (dwThread == GetWindowThreadProcessId(hwndParent, NULL))
        {
            RemoveWindowSubclass(hwndParent, s_ParentWndProc, 0);
        }
    }
}

//+-------------------------------------------------------------------------
// Subclassed window procedure of the parents ot the editbox being
// autocompleted.  This intecepts messages that should case the autocomplete
// dropdown to be hidden.
//--------------------------------------------------------------------------
LRESULT CALLBACK CAutoComplete::s_ParentWndProc
(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    UINT_PTR uIdSubclass,   // always zero for us
    DWORD_PTR dwRefData     // -> CParentWindow
)
{
    CAutoComplete* pThis = (CAutoComplete*)dwRefData;

    if (!pThis)
        return DefSubclassProc(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
        case WM_WINDOWPOSCHANGED:
        {
            //
            // Check the elapsed time since this was last called.  We want to avoid an infinite loop
            // with another window that also wants to be on top.
            //
            static DWORD s_dwTicks = 0;
            DWORD dwTicks = GetTickCount();
            DWORD dwEllapsed = dwTicks - s_dwTicks;
            s_dwTicks = dwTicks;

            if (dwEllapsed > 100)
            {
                // Make sure our dropdown stays on top
                LPWINDOWPOS pwp = (LPWINDOWPOS)lParam;
                if (IsFlagClear(pwp->flags, SWP_NOZORDER) && IsWindowVisible(pThis->m_hwndDropDown))
                {
                    SetWindowPos(pThis->m_hwndDropDown, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
                }
            }
            break;
        }
        case WM_ACTIVATE:
        {
            // Ignore if we are not being deactivated
            WORD fActive = LOWORD(wParam);
            if (fActive != WA_INACTIVE)
            {
                break;
            }
            // Drop through
        }
        case WM_MOVING:
        case WM_SIZE:
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
            pThis->_HideDropDown();
            break;

        case WM_NCACTIVATE:
            //
            // While clicking on the autosuggest dropdown, we
            // want to prevent the dropdown from being activated.
            //
            if (s_fNoActivate)
                return FALSE;
            break;

        case WM_DESTROY:
            RemoveWindowSubclass(hwnd, s_ParentWndProc, 0);
            break;

        default:
            break;
    }

    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

//+-------------------------------------------------------------------------
// Subclassed window procedure fir the grip re-sizer control
//--------------------------------------------------------------------------
LRESULT CALLBACK CAutoComplete::s_GripperWndProc
(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    UINT_PTR uIdSubclass,   // always zero for us
    DWORD_PTR dwRefData     // -> CParentWindow
)
{
    CAutoComplete* pThis = (CAutoComplete*)dwRefData;

    if (!pThis)
        return DefSubclassProc(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
        case WM_NCHITTEST:
            return HTTRANSPARENT;

        case WM_PAINT:
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
            break;

        case WM_ERASEBKGND:
        {
            HDC hdc = (HDC)wParam;
            RECT rc;
            GetClientRect(hwnd, &rc);
            int nOldMapMode = 0;
            BOOL fScrollVisible = pThis->m_hwndScroll && IsWindowVisible(pThis->m_hwndScroll);

            //
            // See if we need to vertically flip the grip
            //
            if (pThis->m_fDroppedUp)
            {
                nOldMapMode = SetMapMode(hdc, MM_ANISOTROPIC);
                SetWindowOrgEx(hdc, 0, 0, NULL);
                SetWindowExtEx(hdc, 1, 1, NULL);
                SetViewportOrgEx(hdc, 0, GetSystemMetrics(SM_CYHSCROLL), NULL);
                SetViewportExtEx(hdc, 1, -1, NULL);
            }
            // The standard DrawFrameControl API does not draw upside down on all platforms
//            DrawFrameControl(hdc, &rc, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);
            DrawGrip(hdc, &rc, fScrollVisible);
            if (nOldMapMode)
            {
                SetViewportOrgEx(hdc, 0, 0, NULL);
                SetViewportExtEx(hdc, 1, 1, NULL);
                SetMapMode(hdc, nOldMapMode);
            }
            return 1;
        }
        case WM_DESTROY:
            RemoveWindowSubclass(hwnd, s_GripperWndProc, 0);
            break;

        default:
            break;
    }

    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}
