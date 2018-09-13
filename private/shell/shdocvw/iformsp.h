// IForms.h : Declaration of the CIntelliForms class

#ifndef __IFORMS_H_
#define __IFORMS_H_

#include "iforms.h"

const TCHAR c_szRegKeySMIEM[] = TEXT("Software\\Microsoft\\Internet Explorer\\Main");
const TCHAR c_szRegKeyIntelliForms[] = TEXT("Software\\Microsoft\\Internet Explorer\\IntelliForms");
const WCHAR c_wszRegKeyIntelliFormsSPW[] = TEXT("Software\\Microsoft\\Internet Explorer\\IntelliForms\\SPW");
const TCHAR c_szRegKeyRestrict[] = TEXT("Software\\Policies\\Microsoft\\Internet Explorer\\Control Panel");

const TCHAR c_szRegValUseFormSuggest[] = TEXT("Use FormSuggest");
const TCHAR c_szRegValFormSuggestRestrict[] = TEXT("FormSuggest");
const TCHAR c_szRegValSavePasswords[] = TEXT("FormSuggest Passwords");
const TCHAR c_szRegValAskPasswords[] = TEXT("FormSuggest PW Ask");
const TCHAR c_szRegValAskUser[] = TEXT("AskUser");

interface IAutoComplete2;
interface IAutoCompleteDropDown;
class CStringList;

#define IF_CHAR             WM_APP  + 0x08
#define IF_KEYDOWN          WM_APP  + 0x09

/////////////////////////////////////////////////////////////////////////////
// CIntelliForms
class CEventSinkCallback
{
public:
    typedef enum
    {
        EVENT_BOGUS     = 100,
        EVENT_KEYDOWN   = 0,
        EVENT_KEYPRESS,
        EVENT_MOUSEDOWN,
        EVENT_DBLCLICK,
        EVENT_FOCUS,
        EVENT_BLUR,
        EVENT_SUBMIT,
        EVENT_SCROLL,
    }
    EVENTS;

    typedef struct
    {
        EVENTS                      Event;
        LPCWSTR                     pwszEventSubscribe;
        LPCWSTR                     pwszEventName;
    }
    EventSinkEntry;

    virtual HRESULT HandleEvent(IHTMLElement *pEle, EVENTS Event, IHTMLEventObj *pEventObj) = 0;

    static EventSinkEntry EventsToSink[];
};

class CIntelliForms : 
    public CEventSinkCallback,
    public IPropertyNotifySink
{
    long    m_cRef;

public:
    class CEventSink;
    class CAutoSuggest;
    friend CAutoSuggest;

    CIntelliForms();
    ~CIntelliForms();

public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // *** IPropertyNotifySink methods ***
    virtual STDMETHODIMP OnChanged(DISPID dispid);
    virtual STDMETHODIMP OnRequestEdit(DISPID dispid);

    // CEventSinkCallback
    HRESULT HandleEvent(IHTMLElement *pEle, EVENTS Event, IHTMLEventObj *pEventObj);

public:
    HRESULT Init(CIEFrameAuto::COmWindow *pOmWindow, IHTMLDocument2 *pDoc2, HWND hwnd);
    HRESULT UnInit();

    LPCWSTR GetUrl();

    HRESULT UserInput(IHTMLInputTextElement *pTextEle);

    HRESULT WriteToStore(LPCWSTR pwszName, CStringList *psl);
    HRESULT ReadFromStore(LPCWSTR pwszName, CStringList **ppsl, BOOL fPasswordList=FALSE);
    HRESULT DeleteFromStore(LPCWSTR pwszName);
    HRESULT ClearStore(DWORD dwClear);

    BOOL    IsRestricted() { return m_fRestricted; }
    BOOL    IsRestrictedPW() { return m_fRestrictedPW; }

    IUnknown *GetDocument() { return m_punkDoc2; }

    HRESULT ScriptSubmit(IHTMLFormElement *pForm);
    HRESULT HandleFormSubmit(IHTMLFormElement *pForm);

    // for CEnumString
    HRESULT GetPasswordStringList(CStringList **ppslPasswords);
    // for IntelliFormsSaveForm
    CIntelliForms *GetNext() { return m_pNext; }

    BOOL    IsEnabledForPage();

    static HRESULT GetName(IHTMLInputTextElement *pTextEle, BSTR *pbstrName);

    // Default to disabled, since we need to ask the user before enabling it
    static BOOL    IsEnabledInCPL() {
                        return IsEnabledInRegistry(c_szRegKeySMIEM, c_szRegValUseFormSuggest, FALSE); }
    // Default to enabled, since we prompt before saving passwords anyway
    static BOOL    IsEnabledRestorePW() {
                        return IsEnabledInRegistry(c_szRegKeySMIEM, c_szRegValSavePasswords, TRUE); }
    static BOOL    IsEnabledAskPW() {
                        return IsEnabledRestorePW() &&
                            IsEnabledInRegistry(c_szRegKeySMIEM, c_szRegValAskPasswords, TRUE); }

    static BOOL    IsAdminRestricted(LPCTSTR pszRegVal);

    BOOL AskedUserToEnable();
    
    typedef HRESULT (*PFN_ENUM_CALLBACK)(IDispatch *pDispEle, DWORD_PTR dwCBData);

protected:
    enum { LIST_DATA_PASSWORD = 1 };    // Flag to indicate a password list in store

    HRESULT AddToElementList(IHTMLInputTextElement *pITE);
    HRESULT FindInElementList(IHTMLInputTextElement *pITE);
    void    FreeElementList();

    HRESULT AddToFormList(IHTMLFormElement *pFormEle);
    HRESULT FindInFormList(IHTMLFormElement *pFormEle);
    void    FreeFormList();

    static BOOL IsElementEnabled(IHTMLElement *pEle);
    static HRESULT ShouldAttachToElement(IUnknown *, BOOL fCheckForm, 
                                IHTMLElement2**, IHTMLInputTextElement**, IHTMLFormElement**, BOOL *pfPassword);
    HRESULT GetBodyEle(IHTMLElement2 **ppEle2);

    HRESULT SubmitElement(IHTMLInputTextElement *pITE, FILETIME ft, BOOL fEnabledInCPL);

    LPCWSTR GetUrlHash();
    
    BOOL ArePasswordsSaved();
    BOOL LoadPasswords();
    void SavePasswords();
    HRESULT FindPasswordEntry(LPCWSTR pwszValue, int *piIndex);
    void SetPasswordsAreSaved(BOOL fSaved);
    HRESULT AutoFillPassword(IHTMLInputTextElement *pTextEle, LPCWSTR pwszUsername);
    HRESULT SavePassword(IHTMLFormElement *pFormEle, FILETIME ftSubmit, IHTMLInputTextElement *pFirstEle);
    HRESULT DeletePassword(LPCWSTR pwszUsername);

    HRESULT AttachToForm(IHTMLFormElement *pFormEle);

    HRESULT CreatePStore();
    HRESULT CreatePStoreAndType();
    void ReleasePStore();

    static BOOL IsEnabledInRegistry(LPCTSTR pszKey, LPCTSTR pszValue, BOOL fDefault);

    inline void EnterModalDialog();
    inline void LeaveModalDialog();

private:
    // CIntelliForms member variables
    CEventSink  *m_pSink;
    CAutoSuggest *m_pAutoSuggest;   // Can attach to one edit control at a time

    DWORD       m_dwConnectionPoint;    // Cookie for PropertyNotifySink

    HINSTANCE   m_hinstPStore;
    IPStore     *m_pPStore;
    BOOL        m_fPStoreTypeInit : 1;  // Our types initialized

    HDPA        m_hdpaElements;         // Elements user has modified
    HDPA        m_hdpaForms;            // Forms we are sinked to

    BOOL        m_fCheckedIfEnabled : 1; // Checked if we're enabled for this page?
    BOOL        m_fEnabledForPage : 1;   // We're enabled for this page (non-SSL)?
    BOOL        m_fHitPWField : 1;      // Went to a password field?
    BOOL        m_fCheckedPW  : 1;      // Checked if we have a password for this URL?
    CStringList *m_pslPasswords;        // Usernames && Passwords for page, if any
    int         m_iRestoredIndex;       // Index of restored password in m_pslPasswords (-1=none)
    BOOL        m_fRestricted : 1;      // Are we restricted for normal Intelliforms?
    BOOL        m_fRestrictedPW : 1;    // Are save passwords restricted?

    // Lifetime management - see Enter/LeaveModalDialog
    BOOL        m_fInModalDialog : 1;   // Are we in a dialog?
    BOOL        m_fUninitCalled : 1;    // Was Uninit called during dialog?

    // Useful stuff for the attached document
    HWND            m_hwndBrowser;
    IHTMLDocument2 *m_pDoc2;
    IUnknown       *m_punkDoc2;
    
    CIEFrameAuto::COmWindow   *m_pOmWindow;

    BSTR        m_bstrFullUrl;          // Full url if https: protocol (security check)
    BSTR        m_bstrUrl;              // Full url with anchor/query string stripped
    LPCWSTR     m_pwszUrlHash;          // String based on UrlHash(m_bstrUrl)

    // Linked list of objects, to find CIntelliForms object for IHTMLDocument2
    CIntelliForms *m_pNext;

public:
    // Helper classes
    template <class TYPE> class CEnumCollection
    {
    public:
        static HRESULT EnumCollection(TYPE *pCollection, PFN_ENUM_CALLBACK pfnCB, DWORD_PTR dwCBData);
    };

    class CEventSink : public IDispatch
    {
    public:
        ULONG   m_cRef;

        CEventSink(CEventSinkCallback *pParent);
        ~CEventSink();

        HRESULT SinkEvents(IHTMLElement2 *pEle2, int iNum, EVENTS *pEvents);
        HRESULT UnSinkEvents(IHTMLElement2 *pEle2, int iNum, EVENTS *pEvents);
        HRESULT SinkEvents(IHTMLWindow3 *pWin3, int iNum, EVENTS *pEvents);
        HRESULT UnSinkEvents(IHTMLWindow3 *pWin3, int iNum, EVENTS *pEvents);

        void SetParent(CEventSinkCallback *pParent) { m_pParent = pParent; }

        STDMETHODIMP QueryInterface(REFIID, void **);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IDispatch
        STDMETHODIMP GetTypeInfoCount(UINT* pctinfo);
        STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo);
        STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames,
            LCID lcid, DISPID *rgDispId);
        STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid,
            LCID lcid, WORD wFlags, DISPPARAMS  *pDispParams, VARIANT  *pVarResult,
            EXCEPINFO *pExcepInfo, UINT *puArgErr);

    private:
        CEventSinkCallback *m_pParent;
    };

    class CAutoSuggest : public CEventSinkCallback
    {
        class CEnumString;

    public:
        CAutoSuggest(CIntelliForms *pParent, BOOL fEnabled, BOOL fEnabledSPW);
        ~CAutoSuggest();

        void SetParent(CIntelliForms *pParent) { m_pParent = pParent; }

        HRESULT AttachToInput(IHTMLInputTextElement *pTextEle);
        HRESULT DetachFromInput();

        static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        static EVENTS s_EventsToSink[];

    protected:
        // Called by window to perform requests by CAutoComplete to MSHTML
        HRESULT GetText(int cchTextMax, LPWSTR pszTextOut, LRESULT *lcchCopied);
        HRESULT GetTextLength(int *pcch);
        HRESULT SetText(LPCWSTR pszTextIn);

        void CheckAutoFillPassword(LPCWSTR pwszUsername);

        inline void MarkDirty();

    public:
        // Called to pass on events from MSHTML to CAutoComplete
        HRESULT HandleEvent(IHTMLElement *pEle, EVENTS Event, IHTMLEventObj *pEventObj);
        HRESULT UpdateDropdownPosition();
        
    private:
        HRESULT CreateAutoComplete();
        
        HRESULT CleanUp();

        CIntelliForms  *m_pParent;          // No refcount
        CEventSink     *m_pEventSink;
        IAutoComplete2 *m_pAutoComplete;
        IAutoCompleteDropDown *m_pAutoCompleteDD;
        HWND            m_hwndEdit;
        IHTMLInputTextElement *m_pTextEle;
        CEnumString    *m_pEnumString;
        long        m_lCancelKeyPress;

        BOOL        m_fAddedToDirtyList : 1;        // Add to list once they hit a key

        BOOL        m_fAllowAutoFillPW : 1;         // Call AutoFillPassword?
        BSTR        m_bstrLastUsername;             // Last Username we called AutoFillPassword for

        BOOL        m_fInitAutoComplete : 1;        // Initialized Auto Complete?

        BOOL        m_fEnabled : 1;                 // Regular intelliforms enabled?
        BOOL        m_fEnabledPW : 1;               // Restore passwords enabled?

        BOOL        m_fEscapeHit : 1;               // Escape key used to dismiss dropdown?

        UINT        m_uMsgItemActivate;             // registered message from autocomplete
        static BOOL s_fRegisteredWndClass;

        // This object is thread-safed because AutoComplete calls on second thread
        class CEnumString : public IEnumString
        {
            long    m_cRef;

        public:
            CEnumString();
            ~CEnumString();

            HRESULT Init(IHTMLInputTextElement *pInputEle, CIntelliForms *pIForms);
            void UnInit();

            HRESULT ResetEnum();

            STDMETHODIMP QueryInterface(REFIID, void **);
            STDMETHODIMP_(ULONG) AddRef(void);
            STDMETHODIMP_(ULONG) Release(void);

            // IEnumString
            virtual STDMETHODIMP Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched);
            virtual STDMETHODIMP Skip(ULONG celt) { return E_NOTIMPL; }
            virtual STDMETHODIMP Reset();
            virtual STDMETHODIMP Clone(IEnumString **ppenum) { return E_NOTIMPL; }

        protected:
            HRESULT FillEnumerator();       // called on secondary thread

            CRITICAL_SECTION m_crit;
            CStringList *m_pslMain;
            BSTR    m_bstrName;         // name of input field
            LPWSTR  m_pszOpsValue;      // value from profile assistant
            CIntelliForms *m_pIntelliForms;

            int     m_iPtr;

            BOOL    m_fFilledStrings : 1;
            BOOL    m_fInit : 1;
        };
    };
};

template <class TYPE>
HRESULT CIntelliForms::CEnumCollection<TYPE>::EnumCollection(
            TYPE                   *pCollection,
            PFN_ENUM_CALLBACK       pfnCB,
            DWORD_PTR               dwCBData)
{
    IDispatch       *pDispItem;

    HRESULT hr;
    long    l, lCount;
    VARIANT vIndex, vEmpty;

    VariantInit(&vEmpty);
    VariantInit(&vIndex);

    hr = pCollection->get_length(&lCount);

    if (FAILED(hr))
        lCount = 0;

    for (l=0; l<lCount; l++)
    {
        vIndex.vt = VT_I4;
        vIndex.lVal = l;

        hr = pCollection->item(vIndex, vEmpty, &pDispItem);

        if (SUCCEEDED(hr) && pDispItem)
        {
            hr = pfnCB(pDispItem, dwCBData);

            pDispItem->Release();
        }

        if (E_ABORT == hr)
        {
            break;
        }
    }

    return hr;
}

inline void CIntelliForms::CAutoSuggest::MarkDirty()
{
    if (!m_fAddedToDirtyList && m_pParent)
    {
        m_fAddedToDirtyList = TRUE;
        m_pParent->UserInput(m_pTextEle);
    }
}

// These wrap modal dialogs, keeping us alive and attached to the document
//  even if something weird happens while our dlgbox messageloop is alive
inline void CIntelliForms::EnterModalDialog()
{
    ASSERT(!m_fInModalDialog);  // Don't support nested Enter/Leave
    ASSERT(!m_fUninitCalled);

    m_fInModalDialog = TRUE;    // Keep us attached to document
    
    AddRef();                   // Keep us alive
}

inline void CIntelliForms::LeaveModalDialog()
{
    ASSERT(m_fInModalDialog);

    m_fInModalDialog = FALSE;
    
    if (m_fUninitCalled)
    {
        UnInit();           // Detach from document
    }

    Release();
}

// HKCU/S/MS/Win/CV/IForms/Names  /[name]/ SIndex | SData

// CStringList is optimized for appending arbitrary amounts of strings and converting to and
//  from blobs. It is not optimized for deleting or inserting strings.
class CStringList
{
protected:
    CStringList();

public:
    ~CStringList();

    friend static HRESULT CStringList_New(CStringList **ppNew, BOOL fAutoDelete=TRUE);

    // E_FAIL, S_FALSE (duplicate), S_OK
    HRESULT AddString(LPCWSTR lpwstr, int *piNum = NULL);
    HRESULT AddString(LPCWSTR lpwstr, FILETIME ft, int *piNum = NULL);

    // E_FAIL, S_OK   Doesn't check for duplicates
    HRESULT AppendString(LPCWSTR lpwstr, int *piNum = NULL);
    HRESULT AppendString(LPCWSTR lpwstr, FILETIME ft, int *piNum = NULL);

    // iLen must be length in characters of string, not counting null term.
    // -1 if unknown. *piNum filled in with index if specified
    HRESULT FindString(LPCWSTR lpwstr, int iLen/*=-1*/, int *piNum/*=NULL*/, BOOL fCaseSensitive);

    inline int      NumStrings();
    inline LPCWSTR  GetString(int iIndex);
    inline DWORD    GetStringLen(int iIndex);
    inline HRESULT  GetStringTime(int iIndex, FILETIME *ft);
    inline HRESULT  SetStringTime(int iIndex, FILETIME ft);
    inline HRESULT  UpdateStringTime(int iIndex, FILETIME ft);
//  inline HRESULT  GetStringData(int iIndex, DWORD *pdwData);
//  inline HRESULT  SetStringData(int iIndex, DWORD dwData);

    HRESULT GetBSTR(int iIndex, BSTR *pbstrRet);
    HRESULT GetTaskAllocString(int iIndex, LPOLESTR *pRet);

    inline HRESULT  GetListData(INT64 *piData);
    inline HRESULT  SetListData(INT64 iData);

    // If set to TRUE, CStringList will delete old strings when full
    void SetAutoScavenge(BOOL fAutoScavenge) { m_fAutoScavenge=fAutoScavenge; }

    HRESULT DeleteString(int iIndex);
    HRESULT InsertString(int iIndex, LPCWSTR lpwstr);
    HRESULT ReplaceString(int iIndex, LPCWSTR lpwstr);

    // Functions to read/write to the store; converts to and from BLOBs
    // For efficiencies sake these take and return heap alloced blobs
    HRESULT WriteToBlobs(LPBYTE *ppBlob1, DWORD *pcbBlob1, LPBYTE *ppBlob2, DWORD *pcbBlob2);
    HRESULT ReadFromBlobs(LPBYTE *ppBlob1, DWORD cbBlob1, LPBYTE *ppBlob2, DWORD cbBlob2);

    static HRESULT GetFlagsFromIndex(LPBYTE pBlob1, INT64 *piFlags);

    // Warning: Don't set max strings past the MAX_STRINGS constant our ReadFromBlobs will fail
    //  if you save/restore the string list
    void SetMaxStrings(DWORD dwMaxStrings) { m_dwMaxStrings = dwMaxStrings; }
    DWORD GetMaxStrings() { return m_dwMaxStrings; }
    
    enum { MAX_STRINGS = 200 };
    
protected:
    enum { INDEX_SIGNATURE=0x4B434957 };        // WICK
    enum { INIT_BUF_SIZE=1024 };

#pragma warning (disable: 4200)     // zero-sized array warning
typedef struct 
{
    DWORD   dwSignature;
    DWORD   cbSize;         // up to not including first StringEntry
    DWORD   dwNumStrings;   // Num of StringEntry present
    INT64   iData;          // Extra data for string list user

    struct tagStringEntry
    {
        union
        {
            DWORD_PTR   dwStringPtr;    // When written to store
            LPWSTR      pwszString;     // When loaded in memory
        };
        FILETIME    ftLastSubmitted;
        DWORD       dwStringLen;        // Length of this string
//      DWORD       dwData;             // Extra data for string list user
    }
    StringEntry[];

} StringIndex;
#pragma warning (default: 4200)

// Value for cbSize in StringIndex
#define STRINGINDEX_CBSIZE PtrToUlong(&((StringIndex*)NULL)->StringEntry)
#define STRINGENTRY_SIZE (PtrToUlong(&((StringIndex*)NULL)->StringEntry[1]) - STRINGINDEX_CBSIZE )
// Size of StringIndex for given number of strings 
#define INDEX_SIZE(n) (STRINGINDEX_CBSIZE + (n)*STRINGENTRY_SIZE)


    void    CleanUp();
    HRESULT Init(DWORD dwBufSize=0);
    HRESULT ConvertToInternalFormat();
    HRESULT ConvertToExternalFormat();

    HRESULT EnsureBuffer(DWORD dwSizeNeeded);
    HRESULT EnsureIndex(DWORD dwNumStringsNeeded);

    HRESULT _AddString(LPCWSTR lpwstr, BOOL fCheckDuplicates, int *piNum);

private:
    StringIndex *m_psiIndex;            // Index of strings
    DWORD   m_dwIndexSize;              // size in bytes of m_psiIndex

    LPBYTE  m_pBuffer;                  // Holds all character data
    DWORD   m_dwBufEnd;                 // Last byte used in buffer
    DWORD   m_dwBufSize;                // Size of buffer in bytes

    DWORD   m_dwMaxStrings;             // Max # strings

    BOOL    m_fAutoScavenge:1;          // Automatically remove old strings when full?
};

#define FILETIME_TO_INT64(ft) (*((INT64 *) &(ft)))

inline int     CStringList::NumStrings()
{
    if (!m_psiIndex) return 0;
    return m_psiIndex->dwNumStrings;
}

inline LPCWSTR CStringList::GetString(int iIndex)
{
    if (!m_psiIndex) return NULL;
    ASSERT((DWORD)iIndex < m_psiIndex->dwNumStrings);
    return m_psiIndex->StringEntry[iIndex].pwszString;
}

inline DWORD CStringList::GetStringLen(int iIndex)
{ 
    if (!m_psiIndex) return E_FAIL;
    ASSERT((DWORD)iIndex < m_psiIndex->dwNumStrings);
    return m_psiIndex->StringEntry[iIndex].dwStringLen;
}

inline HRESULT CStringList::GetStringTime(int iIndex, FILETIME *ft)
{
    if (!m_psiIndex) return E_FAIL;
    ASSERT((DWORD)iIndex < m_psiIndex->dwNumStrings);
    *ft = m_psiIndex->StringEntry[iIndex].ftLastSubmitted;
    return S_OK;
}

inline HRESULT CStringList::SetStringTime(int iIndex, FILETIME ft)
{
    if (!m_psiIndex) return E_FAIL;
    ASSERT((DWORD)iIndex < m_psiIndex->dwNumStrings);
    ASSERT(-1 != CompareFileTime(&ft, &m_psiIndex->StringEntry[iIndex].ftLastSubmitted));
    m_psiIndex->StringEntry[iIndex].ftLastSubmitted = ft;
    return S_OK;
}

inline HRESULT CStringList::UpdateStringTime(int iIndex, FILETIME ft)
{
    if (!m_psiIndex) return E_FAIL;
    ASSERT((DWORD)iIndex < m_psiIndex->dwNumStrings);
    if (1 == CompareFileTime(&ft, &m_psiIndex->StringEntry[iIndex].ftLastSubmitted))
    {
        m_psiIndex->StringEntry[iIndex].ftLastSubmitted = ft;
        return S_OK;
    }
    return S_FALSE;
}
inline HRESULT CStringList::GetListData(INT64 *piData)
{
    if (m_psiIndex)
    {
        *piData = m_psiIndex->iData;
        return S_OK;
    }
    return E_FAIL;
}
inline HRESULT CStringList::SetListData(INT64 iData)
{
    if (!m_psiIndex && FAILED(Init()))
        return E_FAIL;

    m_psiIndex->iData = iData;
    return S_OK;
}
/*
inline HRESULT CStringList::GetStringData(int iIndex, DWORD *pdwData)
{
    if (!m_psiIndex) return E_FAIL;
    ASSERT((DWORD)iIndex < m_psiIndex->dwNumStrings);
    *pdwData = m_psiIndex->StringEntry[iIndex].dwData;
    return S_OK;
}

inline HRESULT CStringList::SetStringData(int iIndex, DWORD dwData)
{
    if (!m_psiIndex) return E_FAIL;
    ASSERT((DWORD)iIndex < m_psiIndex->dwNumStrings);
    m_psiIndex->StringEntry[iIndex].dwData = dwData;
    return S_OK;
}
*/
#endif //__IFORMS_H_
