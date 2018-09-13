// iforms.cpp : Implementation of CIntelliForms

#include "priv.h"
#include <iehelpid.h>
#include <pstore.h>
#include "hlframe.h"
#include "iformsp.h"
#include "shldisp.h"
#include "asuggest.h"
#include "opsprof.h"
#include "resource.h"

#include <mluisupp.h>

#define TF_IFORMS TF_CUSTOM2

// ALLOW_SHELLUIOC_HOST code will allow us to host intelliforms
//  from the Shell UI OC (shuioc.cpp). This is used for the
//  HTML Find dialog
#define ALLOW_SHELLUIOC_HOST

CIntelliForms *GetIntelliFormsFromDoc(IHTMLDocument2 *pDoc2);

inline void MyToLower(LPWSTR pwszStr)
{
    if (g_fRunningOnNT)
    {
        CharLowerBuffW(pwszStr, lstrlenW(pwszStr));
    }
    else
    {
        // Ideally we would use the code page contained in the string instead of
        //  the system code page.
        CHAR chBuf[MAX_PATH];
        SHUnicodeToAnsi(pwszStr, chBuf, ARRAYSIZE(chBuf));
        CharLowerBuffA(chBuf, lstrlenA(chBuf));
        SHAnsiToUnicode(chBuf, pwszStr, lstrlenW(pwszStr)+1);
    }
}


//=================== Exported functions =====================
// Exported for inetCPL
HRESULT ClearAutoSuggestForForms(DWORD dwClear)
{
    CIntelliForms *pObj = new CIntelliForms();

    if (pObj)
    {
        HRESULT hr;

        hr = pObj->ClearStore(dwClear);

        pObj->Release();

        return hr;
    }

    return E_OUTOFMEMORY;
}

// called from iedisp.cpp
void AttachIntelliForms(void *pvOmWindow, HWND hwnd, IHTMLDocument2 *pDoc2, void **ppIntelliForms)
{
static DWORD s_dwAdminRestricted = 0xFE;

    CIEFrameAuto::COmWindow *pOmWindow = (CIEFrameAuto::COmWindow *)pvOmWindow;

    ASSERT(ppIntelliForms && *ppIntelliForms==NULL);

    if (s_dwAdminRestricted == 0xFE)
    {
        s_dwAdminRestricted = CIntelliForms::IsAdminRestricted(c_szRegValFormSuggestRestrict) &&
                              CIntelliForms::IsAdminRestricted(c_szRegValSavePasswords);
    }

    if (s_dwAdminRestricted)
    {
        return;
    }

    // If we're not hosted by internet explorer, we don't want to enable Intelliforms
    //  unless dochost explicitly overrides this
    if (!IsInternetExplorerApp() &&
        !(pOmWindow && (DOCHOSTUIFLAG_ENABLE_FORMS_AUTOCOMPLETE & pOmWindow->IEFrameAuto()->GetDocHostFlags())))
    {
        return;
    }

    if (!hwnd && pOmWindow)
    {
        pOmWindow->IEFrameAuto()->get_HWND((long *)&hwnd);
    }

    if (!hwnd || !pDoc2 || !ppIntelliForms || (*ppIntelliForms != NULL))
    {
        return;
    }

#ifndef ALLOW_SHELLUIOC_HOST
    if (!pOmWindow)
    {
        return;
    }
#else
    if (!pOmWindow)
    {
        // Script is asking to attach to this document
        // Deny their request if another CIntelliForms is already attached
        if (NULL != GetIntelliFormsFromDoc(pDoc2))
        {
            return;
        }
    }
#endif

    CIntelliForms *pForms = new CIntelliForms();

    if (pForms)
    {
        if (SUCCEEDED(pForms->Init(pOmWindow, pDoc2, hwnd)))
        {
            *ppIntelliForms = pForms;
        }
        else
        {
            pForms->Release();
        }
    }
}

void ReleaseIntelliForms(void *pIntelliForms)
{
    CIntelliForms *pForms = (CIntelliForms *) pIntelliForms;

    if (pForms)
    {
        pForms->UnInit();
        pForms->Release();
    }
}

INT_PTR CALLBACK AskUserDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
HRESULT IncrementAskCount();

HRESULT IntelliFormsDoAskUser(HWND hwndBrowser, void *pv)
{
    // Make sure that we haven't asked them yet
    if (S_OK == IncrementAskCount())
    {
        // Modal dialog to ask the user our little question
        DialogBox(MLGetHinst(), MAKEINTRESOURCE(IDD_AUTOSUGGEST_ASK_USER),
                hwndBrowser, AskUserDlgProc);
    }

    return S_OK;
}

// Linked list of active CIntelliform objects to translate from
//  IHTMLDocument2->CIntelliforms when script calls window.external.saveforms
// Protected by g_csDll
CIntelliForms *g_pIntelliFormsFirst=NULL;

// Translate this pDoc2 to an existing instance of CIntelliForms
// Will return NULL if no CIntelliForms attached to this doc
// NO REFCOUNT IS ADDED TO THE RETURN
CIntelliForms *GetIntelliFormsFromDoc(IHTMLDocument2 *pDoc2)
{
    if (!pDoc2)
    {
        return NULL;
    }

    ENTERCRITICAL;
    CIntelliForms *pNext = g_pIntelliFormsFirst;
    IUnknown *punkDoc;
    CIntelliForms *pIForms=NULL;

    pDoc2->QueryInterface(IID_IUnknown, (void **)&punkDoc);

    if (punkDoc)
    {
        // We use this interface only for comparing identity
        punkDoc->Release();

        while (pNext)
        {
            if (pNext->GetDocument() == punkDoc)
            {
                pIForms = pNext;
                break;
            }
            pNext=pNext->GetNext();
        }
    }

    LEAVECRITICAL;

    return pIForms;
}

// called from shuioc.cpp
HRESULT IntelliFormsSaveForm(IHTMLDocument2 *pDoc2, VARIANT *pvarForm)
{
    HRESULT hrRet = S_FALSE;
    IHTMLFormElement *pForm=NULL;
    CIntelliForms *pIForms=NULL;

    if (pvarForm->vt == VT_DISPATCH)
    {
        pvarForm->pdispVal->QueryInterface(IID_IHTMLFormElement, (void **)&pForm);
    }

    if (pForm)
    {
        pIForms = GetIntelliFormsFromDoc(pDoc2);

        if (pIForms)
        {
            // Should validate that pIForms was created on this thread
            hrRet = pIForms->ScriptSubmit(pForm);
        }

        pForm->Release();
    }

    return hrRet;
}

const TCHAR c_szYes[] = TEXT("yes");
const TCHAR c_szNo[] = TEXT("no");

INT_PTR AutoSuggestDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

#ifdef CHECKBOX_HELP
const DWORD c_aIFormsHelpIds[] = {
        IDC_AUTOSUGGEST_NEVER, IDH_INTELLIFORM_PW_PROMPT,
        0, 0
};
#endif

const WCHAR c_wszVCardPrefix[] = L"vCard.";

BOOL CIntelliForms::CAutoSuggest::s_fRegisteredWndClass = FALSE;

// Must be in same order as EVENT enum type
// All events we need to sink anywhere
CEventSinkCallback::EventSinkEntry CEventSinkCallback::EventsToSink[] =
{
    { EVENT_KEYDOWN,    L"onkeydown",   L"keydown"  },
    { EVENT_KEYPRESS,   L"onkeypress",  L"keypress" },
    { EVENT_MOUSEDOWN,  L"onmousedown", L"mousedown"},
    { EVENT_DBLCLICK,   L"ondblclick",  L"dblclick" },
    { EVENT_FOCUS,      L"onfocus",     L"focus"    },
    { EVENT_BLUR,       L"onblur",      L"blur"     },
    { EVENT_SUBMIT,     L"onsubmit",    L"submit"   },
    { EVENT_SCROLL,     L"onscroll",    L"scroll",  },
};

// Fake edit window class
const WCHAR c_szEditWndClass[] = TEXT("IntelliFormClass");

// Minimum dropdown width
const int MINIMUM_WIDTH=100;

// Submit number to ask user to enable us
const int ASK_USER_ON_SUBMIT_N = 2;

void GetStuffFromEle(IUnknown *punkEle, IHTMLWindow2 **ppWin2, IHTMLDocument2 **ppDoc2)
{
    if (ppWin2)
        *ppWin2=NULL;

    if (ppDoc2)
        *ppDoc2=NULL;

    IHTMLElement *pEle=NULL;
    punkEle->QueryInterface(IID_IHTMLElement, (void **)&pEle);

    if (pEle)
    {
        IDispatch *pDisp=NULL;
        pEle->get_document(&pDisp);
        if (pDisp)
        {
            IHTMLDocument2 *pDoc2 = NULL;
            pDisp->QueryInterface(IID_IHTMLDocument2, (void **)&pDoc2);
            if (pDoc2)
            {
                if (ppWin2)
                {
                    pDoc2->get_parentWindow(ppWin2);
                }

                if (ppDoc2)
                {
                    *ppDoc2 = pDoc2;
                }
                else
                {
                    pDoc2->Release();
                }
            }
            pDisp->Release();
        }

        pEle->Release();
    }
}

void Win3FromDoc2(IHTMLDocument2 *pDoc2, IHTMLWindow3 **ppWin3)
{
    *ppWin3=NULL;

    IHTMLWindow2 *pWin2=NULL;

    if (SUCCEEDED(pDoc2->get_parentWindow(&pWin2)) && pWin2)
    {
        pWin2->QueryInterface(IID_IHTMLWindow3, (void **)ppWin3);
        pWin2->Release();
    }
}

// Increment the count of whether we've asked the user to enable us or not. We won't
//  ask them on the first form submit since installing ie5.
HRESULT IncrementAskCount()
{
    DWORD dwData, dwSize, dwType;
    dwSize = sizeof(dwData);

    // c_szRegValAskUser contains the number of form submits
    //  0 means we've already asked user whether to enable us
    //  1 means we've already had one form submit, and should ask the user this time
    //  value not present means we haven't had any form submits

    if ((ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER,
            c_szRegKeyIntelliForms, c_szRegValAskUser, &dwType, &dwData, &dwSize)) &&
        dwType == REG_DWORD)
    {
        if (dwData == 0)
        {
            // Shouldn't get this far
            TraceMsg(TF_IFORMS|TF_WARNING, "IntelliFormsDoAskUser: Already asked user");
            return E_FAIL;      // Already asked user
        }
    }
    else
    {
        dwData = 0;
    }

    if (dwData+1 < ASK_USER_ON_SUBMIT_N)
    {
        dwData ++;
        SHSetValue(HKEY_CURRENT_USER, c_szRegKeyIntelliForms, c_szRegValAskUser,
            REG_DWORD, &dwData, sizeof(dwData));

        TraceMsg(TF_IFORMS, "IntelliFormsDoAskUser incrementing submit count. Not asking user.");

        return E_FAIL;      // Don't ask the user
    }

    return S_OK;        // Let's ask the user
}


/////////////////////////////////////////////////////////////////////////////
// CIntelliForms

CIntelliForms::CIntelliForms()
{
    TraceMsg(TF_IFORMS, "CIntelliForms::CIntelliForms");

    m_cRef = 1;

    m_iRestoredIndex = -1;

    m_fRestricted = IsAdminRestricted(c_szRegValFormSuggestRestrict);
    m_fRestrictedPW = IsAdminRestricted(c_szRegValSavePasswords);

    // Add us to global linked list
    ENTERCRITICAL;
    m_pNext = g_pIntelliFormsFirst;
    g_pIntelliFormsFirst = this;
    LEAVECRITICAL;
}

CIntelliForms::~CIntelliForms()
{
    // Remove us from global linked list
    ENTERCRITICAL;

    CIntelliForms *pLast=NULL, *pNext = g_pIntelliFormsFirst;

    while (pNext && pNext != this)
    {
        pLast = pNext;
        pNext=pNext->m_pNext;
    }

    ASSERT(pNext == this);

    if (pNext)
    {
        if (pLast)
        {
            pLast->m_pNext = m_pNext;
        }
        else
        {
            g_pIntelliFormsFirst = m_pNext;
        }
    }
    LEAVECRITICAL;

    TraceMsg(TF_IFORMS, "CIntelliForms::~CIntelliForms");
}

// Called when document is ready to attach to
// We don't support re-initting
HRESULT CIntelliForms::Init(CIEFrameAuto::COmWindow *pOmWindow, IHTMLDocument2 *pDoc2, HWND hwndBrowser)
{
    HRESULT hr;

    ASSERT(pDoc2 && hwndBrowser);

#ifndef ALLOW_SHELLUIOC_HOST
    if (pOmWindow == NULL)
    {
        return E_INVALIDARG;
    }
#endif

    // Connect to get active element changed notifications
    hr = ConnectToConnectionPoint(SAFECAST(this, IPropertyNotifySink*),
            IID_IPropertyNotifySink, TRUE, pDoc2, &m_dwConnectionPoint, NULL);

    if (SUCCEEDED(hr))
    {

        m_pOmWindow = pOmWindow;
        if (pOmWindow)
        {
            pOmWindow->AddRef();
        }

        m_pDoc2 = pDoc2;
        pDoc2->AddRef();
        pDoc2->QueryInterface(IID_IUnknown, (void **)&m_punkDoc2);

        m_hwndBrowser = hwndBrowser;

        m_iRestoredIndex = -1;

        hr = S_OK;
    }

#ifdef ALLOW_SHELLUIOC_HOST
    if (!pOmWindow && (hr == S_OK))
    {
        // Check for the current active element since the page is requesting
        //  us to attach to an existing document
        OnChanged(DISPID_IHTMLDOCUMENT2_ACTIVEELEMENT);
    }
#endif

    GetUrl();       // Init Url member variables so we don't get the url on the
                    //   wrong thread in the FillEnumerator call

    TraceMsg(TF_IFORMS, "CIntelliForms::Init hr=%08x", hr);

    return hr;
}

HRESULT CIntelliForms::UnInit()
{
    if (m_fInModalDialog)
    {
        // Lifetime management. If UnInit is called during modal dialog, we keep ourself
        //  alive. Use Enter/LeaveModalDialog to ensure correct use
        ASSERT(m_fUninitCalled == FALSE);       // Should only be called once...
        m_fUninitCalled = TRUE;
        return S_FALSE;
    }

    // Free connection point
    if (m_dwConnectionPoint)
    {
        ASSERT(m_pDoc2);
        ConnectToConnectionPoint(NULL, IID_IPropertyNotifySink, FALSE, m_pDoc2, &m_dwConnectionPoint, NULL);
    }

    // Destroy this now, before we free other member variables, to ensure CAutoSuggest doesn't
    //  try to access us on a second thread.
    if (m_pAutoSuggest)
    {
        m_pAutoSuggest->SetParent(NULL);
        m_pAutoSuggest->DetachFromInput();
        delete m_pAutoSuggest;
        m_pAutoSuggest = NULL;
    }

    if (m_hdpaForms && m_pSink)
    {
        IHTMLElement2 *pEle2;
        EVENTS events[] = { EVENT_SUBMIT };

        for (int i=DPA_GetPtrCount(m_hdpaForms)-1; i>=0; i--)
        {
            ((IHTMLFormElement *)(DPA_FastGetPtr(m_hdpaForms, i)))->QueryInterface(IID_IHTMLElement2, (void **)&pEle2);
            m_pSink->UnSinkEvents(pEle2, ARRAYSIZE(events), events);
            pEle2->Release();
        }
    }

    SysFreeString(m_bstrFullUrl);
    m_bstrFullUrl = NULL;

    SysFreeString(m_bstrUrl);
    m_bstrUrl = NULL;

    if (m_pwszUrlHash)
    {
        LocalFree((void *)m_pwszUrlHash);
        m_pwszUrlHash = NULL;
    }

    if (m_pSink)
    {
#ifndef ALLOW_SHELLUIOC_HOST
        ASSERT(m_pOmWindow);
#endif
        if (m_pOmWindow)
        {
            IHTMLWindow3 *pWin3=NULL;

            Win3FromDoc2(m_pDoc2, &pWin3);

            if (pWin3)
            {
                EVENTS events[] = { EVENT_SCROLL };
                m_pSink->UnSinkEvents(pWin3, ARRAYSIZE(events), events);
                pWin3->Release();
            }
        }

        m_pSink->SetParent(NULL);
        m_pSink->Release();
        m_pSink=NULL;
    }

    // SAFERELEASE (and ATOMICRELEASE) macro in shdocvw is actually function which requires IUnknown
    ATOMICRELEASET(m_pOmWindow, CIEFrameAuto::COmWindow);
    SAFERELEASE(m_pDoc2);
    SAFERELEASE(m_punkDoc2);

    FreeElementList();
    FreeFormList();

    if (m_pslPasswords)
    {
        delete m_pslPasswords;
        m_pslPasswords = NULL;
    }

    ReleasePStore();

    TraceMsg(TF_IFORMS, "CIntelliForms::UnInit");

    return S_OK;
}

STDMETHODIMP CIntelliForms::QueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;

    if ((IID_IPropertyNotifySink == riid) ||
        (IID_IUnknown == riid))
    {
        *ppv = (IPropertyNotifySink *)this;
    }

    if (NULL != *ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CIntelliForms::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CIntelliForms::Release(void)
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}


// IPropertyNotifySink
//
HRESULT CIntelliForms::OnRequestEdit(DISPID dispid)
{
    return E_NOTIMPL;
}

HRESULT CIntelliForms::OnChanged(DISPID dispid)
{
    ASSERT(m_pDoc2);

    if (DISPID_IHTMLDOCUMENT2_ACTIVEELEMENT == dispid)
    {
        // Detach the AutoSuggest object and destroy it
        if (m_pAutoSuggest)
        {
            m_pAutoSuggest->DetachFromInput();
            delete m_pAutoSuggest;
            m_pAutoSuggest=NULL;
        }

        if (m_pDoc2)
        {
            IHTMLElement *pEle=NULL;
            m_pDoc2->get_activeElement(&pEle);

            if (pEle)
            {
                BOOL fPassword=FALSE;
                IHTMLInputTextElement *pTextEle = NULL;

                if (SUCCEEDED(ShouldAttachToElement(pEle, TRUE, NULL, &pTextEle, NULL, &fPassword)))
                {
                    BOOL fEnabledInCPL = IsEnabledInCPL();
                    BOOL fEnabledPW = IsEnabledRestorePW();

                    // We need to watch user activity if...
                    if (fEnabledInCPL ||        // Intelliforms is enabled
                        fEnabledPW ||           // Or Restore Passwords is enabled
                        !AskedUserToEnable())   // Or we may ask them to enable us
                    {
                        m_pAutoSuggest = new CAutoSuggest(this, fEnabledInCPL, fEnabledPW);

                        if (m_pAutoSuggest)
                        {
                            if (!m_pSink)
                            {
                                m_pSink = new CEventSink(this);

                                if (m_pSink)
                                {
#ifndef ALLOW_SHELLUIOC_HOST
                                    // Don't sink scroll event if hosted by ShellUIOC
                                    //  or jscript.dll asserts on unload
                                    ASSERT(m_pOmWindow);
#endif
                                    if (m_pOmWindow)
                                    {
                                        IHTMLWindow3 *pWin3=NULL;

                                        Win3FromDoc2(m_pDoc2, &pWin3);

                                        if (pWin3)
                                        {
                                            EVENTS events[] = { EVENT_SCROLL };
                                            m_pSink->SinkEvents(pWin3, ARRAYSIZE(events), events);
                                            pWin3->Release();
                                        }
                                    }
                                }
                            }

                            if (!m_pSink || FAILED(m_pAutoSuggest->AttachToInput(pTextEle)))
                            {
                                delete m_pAutoSuggest;
                                m_pAutoSuggest = NULL;
                            }
                        }
                    }

                    pTextEle->Release();
                }
                else
                {
                    ASSERT(!pTextEle);

                    if (fPassword)
                    {
                        m_fHitPWField = TRUE;
                    }
                }

                pEle->Release();
            }
        }
    }

    return S_OK;
}

// Helper functions
BOOL CIntelliForms::AskedUserToEnable()
{
    DWORD dwType, dwSize;
    DWORD dwVal;
    DWORD dwRet;

    dwSize = sizeof(dwVal);

    dwRet = SHGetValue(HKEY_CURRENT_USER, c_szRegKeyIntelliForms, c_szRegValAskUser,
                            &dwType, &dwVal, &dwSize);

    if ((dwRet == ERROR_SUCCESS) && (dwType == REG_DWORD))
    {
        return (dwVal == 0) ? TRUE : FALSE;
    }

    return FALSE;
}

BOOL CIntelliForms::IsEnabledInRegistry(LPCTSTR pszKey, LPCTSTR pszValue, BOOL fDefault)
{
    DWORD dwType, dwSize;
    TCHAR szEnabled[16];
    DWORD dwRet;

    dwSize = sizeof(szEnabled);

    dwRet = SHGetValue(HKEY_CURRENT_USER, pszKey, pszValue, &dwType, szEnabled, &dwSize);

    if (dwRet == ERROR_INSUFFICIENT_BUFFER)
    {
        // Invalid value in registry.
        ASSERT(dwRet == ERROR_SUCCESS);
        return FALSE;
    }

    if (dwRet == ERROR_SUCCESS)
    {
        if ((dwType == REG_SZ) &&
            (!StrCmp(szEnabled, TEXT("yes"))))
        {
            // Enabled
            return TRUE;
        }
        else
        {
            // Disabled
            return FALSE;
        }
    }

    // Value not found
    return fDefault;
}

BOOL CIntelliForms::IsAdminRestricted(LPCTSTR pszRegVal)
{
    DWORD lSize;
    DWORD  lValue;

    lValue = 0; // clear it
    lSize = sizeof(lValue);
    if (ERROR_SUCCESS !=
        SHGetValue(HKEY_CURRENT_USER, c_szRegKeyRestrict, pszRegVal, NULL, (LPBYTE)&lValue, &lSize ))
    {
        return FALSE;
    }

    ASSERT(lSize == sizeof(lValue));

    return (0 != lValue) ? TRUE : FALSE;
}

BOOL CIntelliForms::IsEnabledForPage()
{
    if (!m_fCheckedIfEnabled)
    {
        m_fCheckedIfEnabled = TRUE;

        // We will have our Url in m_bstrFullUrl, only if it is https: protocol
        if (m_bstrFullUrl)
        {
            ASSERT(!StrCmpNIW(m_bstrFullUrl, L"https:", 5));

            m_fEnabledForPage = TRUE;

            // See if this page is in the internet cache. If not, we won't intelliform
            //  for this page either.
            if (!GetUrlCacheEntryInfoW(m_bstrFullUrl, NULL, NULL) && (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
            {
                // Failed - it's not in the cache
                m_fEnabledForPage = FALSE;
            }
        }
        else
        {
            // Url is not https: so always enable Intelliforms
            m_fEnabledForPage = TRUE;
        }
    }

    return m_fEnabledForPage;
}

HRESULT CIntelliForms::GetBodyEle(IHTMLElement2 **ppEle2)
{
    if (!m_pDoc2 || !ppEle2)
    {
        return E_INVALIDARG;
    }

    *ppEle2=NULL;

    IHTMLElement *pBodyEle=NULL;

    m_pDoc2->get_body(&pBodyEle);

    if (pBodyEle)
    {
        pBodyEle->QueryInterface(IID_IHTMLElement2, (void **)ppEle2);
        pBodyEle->Release();
    }

    return (*ppEle2) ? S_OK : E_FAIL;
}

// static
BOOL CIntelliForms::IsElementEnabled(IHTMLElement *pEle)
{
    BOOL fEnabled=TRUE;
    BSTR bstrAttribute;

    VARIANT varVal;
    varVal.vt = VT_EMPTY;

    // First check "AutoComplete=OFF"
    bstrAttribute=SysAllocString(L"AutoComplete");

    if (bstrAttribute &&
        SUCCEEDED(pEle->getAttribute(bstrAttribute, 0, &varVal)))
    {
        if (varVal.vt == VT_BSTR)
        {
            if (!StrCmpIW(varVal.bstrVal, L"off"))
            {
                // We are disabled.
                fEnabled=FALSE;
            }
        }

        VariantClear(&varVal);
    }

    SysFreeString(bstrAttribute);

    // Then check "READONLY" attribute
    if (fEnabled)
    {
        IHTMLInputElement *pInputEle=NULL;

        pEle->QueryInterface(IID_IHTMLInputElement, (void **)&pInputEle);

        if (pInputEle)
        {
            VARIANT_BOOL vbReadOnly=VARIANT_FALSE;

            pInputEle->get_readOnly(&vbReadOnly);

            if (vbReadOnly)
            {
                // We are read only.
                fEnabled=FALSE;
            }

            pInputEle->Release();
        }
    }

    return fEnabled;
}

// static
HRESULT CIntelliForms::ShouldAttachToElement(IUnknown                *punkEle,
                                             BOOL                     fCheckForm,
                                             IHTMLElement2          **ppEle2,
                                             IHTMLInputTextElement  **ppITE,
                                             IHTMLFormElement       **ppFormEle,
                                             BOOL                    *pfPassword)
{
    IHTMLInputTextElement *pITE = NULL;

    if (ppEle2)
    {
        *ppEle2 = NULL;
    }

    if (ppITE)
    {
        *ppITE = NULL;
    }

    if (ppFormEle)
    {
        *ppFormEle = NULL;
    }

    punkEle->QueryInterface(IID_IHTMLInputTextElement, (void **)&pITE);

    if (NULL == pITE)
    {
        // Not an input text element. Do not attach.
        return E_FAIL;
    }

    HRESULT hr = E_FAIL;

    IHTMLElement2 *pEle2        = NULL;
    IHTMLElement *pEle          = NULL;
    IHTMLFormElement *pFormEle  = NULL;

    punkEle->QueryInterface(IID_IHTMLElement2, (void **)&pEle2);
    punkEle->QueryInterface(IID_IHTMLElement, (void **)&pEle);

    if (pEle2 && pEle)
    {
        // type=text is all that's allowed
        BSTR bstrType=NULL;

        if (SUCCEEDED(pITE->get_type(&bstrType)) && bstrType)
        {
            if (!StrCmpICW(bstrType, L"text"))
            {
                // FormSuggest=off attribute turns us off for this element
                if (IsElementEnabled(pEle))
                {
                    IHTMLElement *pFormHTMLEle=NULL;

                    if (fCheckForm || ppFormEle)
                    {
                        pITE->get_form(&pFormEle);

                        if (pFormEle)
                        {
                            pFormEle->QueryInterface(IID_IHTMLElement, (void **)&pFormHTMLEle);
                        }
                        else
                        {
                            // This may be valid if element is not in form
                            TraceMsg(TF_IFORMS, "Iforms: pITE->get_form() returned NULL");
                        }
                    }

                    // FormSuggest=off for form turns us off for this form
                    if (pFormEle &&
                        (!fCheckForm || (pFormHTMLEle && IsElementEnabled(pFormHTMLEle))))
                    {
                        hr = S_OK;
                        if (ppEle2)
                        {
                            *ppEle2 = pEle2;
                            pEle2->AddRef();
                        }
                        if (ppFormEle)
                        {
                            *ppFormEle = pFormEle;
                            pFormEle->AddRef();
                        }
                        if (ppITE)
                        {
                            *ppITE = pITE;
                            pITE->AddRef();
                        }
                    }

                    SAFERELEASE(pFormHTMLEle);
                    SAFERELEASE(pFormEle);
                }
            }
            else
            {
                if (pfPassword && !StrCmpICW(bstrType, L"password") && IsElementEnabled(pEle))
                {
                    TraceMsg(TF_IFORMS, "IForms: Password field detected.");
                    *pfPassword = TRUE;
                }
            }

            SysFreeString(bstrType);
        }
        else
        {
            TraceMsg(TF_IFORMS, "IntelliForms disabled for single element via attribute");
        }
    }

    SAFERELEASE(pITE);
    SAFERELEASE(pEle2);
    SAFERELEASE(pEle);

    return hr;
}

// Get the URL that we're located at, with query string/anchor stripped.
LPCWSTR CIntelliForms::GetUrl()
{
    if (m_bstrUrl)
    {
        return m_bstrUrl;
    }

    if (m_pOmWindow)
    {
        m_pOmWindow->IEFrameAuto()->get_LocationURL(&m_bstrUrl);
    }
#ifdef ALLOW_SHELLUIOC_HOST
    else
    {
        IHTMLLocation *pHTMLLocation=NULL;

        m_pDoc2->get_location(&pHTMLLocation);

        if (NULL != pHTMLLocation)
        {
            pHTMLLocation->get_href(&m_bstrUrl);
            pHTMLLocation->Release();
        }
    }
#endif

    if (m_bstrUrl)
    {
        PARSEDURLW puW = {0};
        puW.cbSize = sizeof(puW);

        // Save the full url for a security check, if we are https protocol
        if (SUCCEEDED(ParseURLW(m_bstrUrl, &puW)))
        {
            if (puW.nScheme == URL_SCHEME_HTTPS)
            {
                m_bstrFullUrl = SysAllocString(m_bstrUrl);
                if (!m_bstrFullUrl)
                {
                    SysFreeString(m_bstrUrl);
                    m_bstrUrl=NULL;
                }
            }
        }
    }

    if (m_bstrUrl)
    {
        // Strip off any query string or anchor
        LPWSTR lpUrl = m_bstrUrl;
        while (*lpUrl)
        {
            if ((*lpUrl == L'?') || (*lpUrl == L'#'))
            {
                *lpUrl = L'\0';
                break;
            }
            lpUrl ++;
        }

        return m_bstrUrl;
    }

    TraceMsg(TF_WARNING|TF_IFORMS, "CIntelliForms::GetUrl() failing!");
    return L"";     // We can assume non-NULL pointer
}

// hook our "Submit" event sink to this form
HRESULT CIntelliForms::AttachToForm(IHTMLFormElement *pFormEle)
{
    ASSERT(m_pSink);

    if (m_pSink)
    {
        IHTMLElement2 *pEle2 = NULL;

        pFormEle->QueryInterface(IID_IHTMLElement2, (void **)&pEle2);

        if (pEle2)
        {
            // Sink event for the form
            EVENTS events[] = { EVENT_SUBMIT };
            m_pSink->SinkEvents(pEle2, ARRAYSIZE(events), events);
        }

        SAFERELEASE(pEle2);

        return S_OK;
    }

    return E_OUTOFMEMORY;
}

// Returns TRUE if nothing but spaces in string
inline BOOL IsEmptyString(LPCWSTR lpwstr)
{
    while (*lpwstr && (*lpwstr == L' ')) lpwstr++;
    return (*lpwstr == 0);
}

// called for each element in the form we are submitting
HRESULT CIntelliForms::SubmitElement(IHTMLInputTextElement *pITE, FILETIME ftSubmit, BOOL fEnabledInCPL)
{
    if (m_fRestricted) return E_FAIL;

    HRESULT hrRet = S_OK;

    BSTR bstrName;

    CIntelliForms::GetName(pITE, &bstrName);

    if (bstrName && bstrName[0])
    {
        BSTR bstrValue=NULL;

        pITE->get_value(&bstrValue);

        if (bstrValue && bstrValue[0] && !IsEmptyString(bstrValue))
        {
            if (fEnabledInCPL)
            {
                TraceMsg(TF_IFORMS, "IForms: Saving field \"%ws\" as \"%ws\"", bstrName, bstrValue);

                CStringList *psl;

                if (FAILED(ReadFromStore(bstrName, &psl)))
                {
                    CStringList_New(&psl);
                }

                if (psl)
                {
                    HRESULT hr;

                    if (SUCCEEDED(hr = psl->AddString(bstrValue, ftSubmit)))
                    {
                        if ((S_OK == hr) ||
                            (psl->NumStrings() > CStringList::MAX_STRINGS / 4))
                        {
                            // We added a non-duplicate string, or we updated the
                            //  last submit time of an existing string
                            WriteToStore(bstrName, psl);
                        }
                    }

                    delete psl;
                }
            }
            else
            {
                hrRet = S_FALSE;   // Tell caller that we didn't save because we were disabled
            }
        }

        SysFreeString(bstrValue);
    }

    SysFreeString(bstrName);

    return hrRet;
}

HRESULT CIntelliForms::HandleFormSubmit(IHTMLFormElement *pForm)
{
    IUnknown *punkForm=NULL;

    if (!pForm)
    {
        // We currently require a form element even from script
        return E_INVALIDARG;
    }

    if (!m_hdpaElements || !m_hdpaForms)
    {
        return S_OK;
    }

    // Make sure we're enabled
    BOOL fEnabledInCPL = IsEnabledInCPL();
    if (fEnabledInCPL || IsEnabledRestorePW() || !AskedUserToEnable())
    {
        pForm->QueryInterface(IID_IUnknown, (void **)&punkForm);

        if (punkForm)
        {
            IHTMLFormElement *pThisFormEle;
            IUnknown *punkThisForm;
            FILETIME ftSubmit;
            int     iCount=0;
            BOOL    fShouldAskUser=FALSE;
            IHTMLInputTextElement *pFirstEle=NULL;

            GetSystemTimeAsFileTime(&ftSubmit);

            // Go through list of 'changed' elements and save their values
            //  make sure we loop backwards since we nuke elements as we find them
            for (int i=DPA_GetPtrCount(m_hdpaElements)-1; i>=0; i--)
            {
                IHTMLInputTextElement *pITE = ((IHTMLInputTextElement *)(DPA_FastGetPtr(m_hdpaElements, i)));

                if (SUCCEEDED(pITE->get_form(&pThisFormEle)) && pThisFormEle)
                {
                    if (SUCCEEDED(pThisFormEle->QueryInterface(IID_IUnknown, (void **)&punkThisForm)))
                    {
                        if (punkThisForm == punkForm)
                        {
                            // Verify that we're still allowed to save this element
                            if (SUCCEEDED(ShouldAttachToElement(pITE, TRUE, NULL, NULL, NULL, NULL)))
                            {
                                iCount ++;

                                if (!pFirstEle)
                                {
                                    pFirstEle = pITE;
                                    pFirstEle->AddRef();
                                }

                                // Don't save non-password stuff for non-cached pages
                                if (IsEnabledForPage())
                                {
                                    // Won't actually save the value if fEnabledInCPL is FALSE
                                    if (S_FALSE == SubmitElement(pITE, ftSubmit, fEnabledInCPL))
                                    {
                                        // We would have saved this if we were enabled
                                        fShouldAskUser = TRUE;
                                    }
                                }

                                // Remove this element from the DPA to prevent any possibility of
                                //  saving before more user input takes place
                                pITE->Release();
                                DPA_DeletePtr(m_hdpaElements, i);
                            }
                        }
                        else
                        {
                            TraceMsg(TF_IFORMS, "IForms: User input in different form than was submitted...?");
                        }

                        punkThisForm->Release();
                    }

                    pThisFormEle->Release();
                }
                else
                {
                    // It shouldn't be in our DPA if it isn't in a form...
                    TraceMsg(TF_WARNING|TF_IFORMS, "Iforms: pITE->get_form() returned NULL!");
                }
            }

            if (0 == DPA_GetPtrCount(m_hdpaElements))
            {
                DPA_Destroy(m_hdpaElements);
                m_hdpaElements=NULL;
            }

            if (m_fHitPWField || (m_iRestoredIndex != -1))
            {
                // ?? why not check iCount==1 here?
                if (pFirstEle)
                {
                    // May have restored PW and may have changed or entered it
                    SavePassword(pForm, ftSubmit, pFirstEle);

                    // WARNING - after returning from "SavePassword" our object may be invalid
                    //  if we got released/detached during modal dialog
                }
            }
            else if (fShouldAskUser)
            {
                // Possibly ask user if they want to enable intelliforms, only if
                //  this isn't a login
                if (m_pOmWindow)
                {
                    m_pOmWindow->IntelliFormsAskUser(NULL);
                }
                fShouldAskUser = FALSE;
            }

            if (fShouldAskUser)
            {
                // If we should ask the user but we're not going to (login form),
                //  increment our count anyway so that we ask them as soon as we can
                IncrementAskCount();
            }

            punkForm->Release();
            SAFERELEASE(pFirstEle);
        }
    }

    return S_OK;
}

HRESULT CIntelliForms::HandleEvent(IHTMLElement *pEle, EVENTS Event, IHTMLEventObj *pEventObj)
{
    TraceMsg(TF_IFORMS, "CIntelliForms::HandleEvent Event=%ws", EventsToSink[Event].pwszEventName);

    if (Event == EVENT_SUBMIT)
    {
        // Save strings for modified text inputs when appropriate
        IHTMLFormElement *pFormEle = NULL;

        if (pEle)
        {
            pEle->QueryInterface(IID_IHTMLFormElement, (void **)&pFormEle);
            if (pFormEle)
            {
                HandleFormSubmit(pFormEle);
                // Warning - "this" may be detached/destroyed at this point
                pFormEle->Release();
            }
        }
    }
    else
    {
        ASSERT(Event == EVENT_SCROLL);
        if (m_pAutoSuggest)
            m_pAutoSuggest->UpdateDropdownPosition();
    }

    return S_OK;
}

// Our passwords are stored in username/value pairs
// Search every other string for the username
HRESULT CIntelliForms::FindPasswordEntry(LPCWSTR pwszValue, int *piIndex)
{
    ASSERT(m_pslPasswords);
    ASSERT(!(m_pslPasswords->NumStrings() & 1));   // Should be even number

    int i;

    for (i=0; i<m_pslPasswords->NumStrings(); i += 2)
    {
        if (!StrCmpIW(pwszValue, m_pslPasswords->GetString(i)))
        {
            // Found it
            *piIndex = i+1;
            return S_OK;
        }
    }

    return E_FAIL;
}

// Convert url to string based on shlwapi UrlHash return
LPCWSTR CIntelliForms::GetUrlHash()
{
    BYTE bBuf[15];

    if (m_pwszUrlHash)
    {
        return m_pwszUrlHash;
    }

    LPCWSTR pwszUrl = GetUrl();

    if (!pwszUrl || !*pwszUrl)
    {
        return NULL;
    }

    if (SUCCEEDED(UrlHashW(pwszUrl, bBuf, ARRAYSIZE(bBuf))))
    {
        // Translate this array of bytes into 7-bit chars
        m_pwszUrlHash = (LPWSTR)LocalAlloc(LMEM_FIXED, sizeof(WCHAR)*(ARRAYSIZE(bBuf)+1));

        if (m_pwszUrlHash)
        {
            for (int i=0; i<ARRAYSIZE(bBuf); i++)
            {
                // Translate each char into 32-96 range
                ((LPWSTR)m_pwszUrlHash)[i] = (WCHAR)((bBuf[i] & 0x3F) + 0x20);
            }
            ((LPWSTR)m_pwszUrlHash)[i] = L'\0';
        }

        return m_pwszUrlHash;
    }

    return NULL;
}

// Tells us if passwords are present for this url
BOOL CIntelliForms::ArePasswordsSaved()
{
    if (!m_fRestrictedPW)
    {
        DWORD dwVal, dwSize=sizeof(dwVal);
        LPCWSTR pwsz = GetUrlHash();

        if (pwsz && (ERROR_SUCCESS == SHGetValueW(HKEY_CURRENT_USER, c_wszRegKeyIntelliFormsSPW, pwsz, NULL, &dwVal, &dwSize)))
        {
            return TRUE;
        }
    }

    return FALSE;
}

// Will return password list in m_pslPasswords, if passwords are saved
BOOL CIntelliForms::LoadPasswords()
{
    if (!m_fCheckedPW)
    {
        m_fCheckedPW = TRUE;

        // Check if passwords are present without hitting pstore
        if (ArePasswordsSaved())
        {
            // We should have passwords for this url. Hit PStore.
            ReadFromStore(GetUrl(), &m_pslPasswords, TRUE);

            m_iRestoredIndex = -1;
        }
    }
    else if (m_pslPasswords)
    {
        // If we already have passwords, double check the registry in case the user
        //  nuked saved stuff via inetcpl
        if (!ArePasswordsSaved())
        {
            delete m_pslPasswords;
            m_pslPasswords=NULL;
            m_iRestoredIndex = -1;
        }
    }

    return (m_pslPasswords != NULL);
}

void CIntelliForms::SavePasswords()
{
    if (m_pslPasswords && m_bstrUrl)
    {
        WriteToStore(m_bstrUrl, m_pslPasswords);
        SetPasswordsAreSaved(TRUE);
    }
}

// Mark that we have passwords saved for this url
void CIntelliForms::SetPasswordsAreSaved(BOOL fSaved)
{
    LPCWSTR pwsz = GetUrlHash();

    if (pwsz)
    {
        if (fSaved)
        {
            DWORD dwSize = sizeof(DWORD);
            DWORD dw = 0;
            SHSetValueW(HKEY_CURRENT_USER, c_wszRegKeyIntelliFormsSPW, pwsz, REG_DWORD, &dw, sizeof(dw));
        }
        else
        {
            SHDeleteValueW(HKEY_CURRENT_USER, c_wszRegKeyIntelliFormsSPW, pwsz);
        }

    }
}

// enumerates form & gets password fields
class CDetectLoginForm
{
public:
    CDetectLoginForm() { m_pNameEle=m_pPasswordEle=m_pPasswordEle2=NULL; }
    ~CDetectLoginForm() { SAFERELEASE(m_pNameEle); SAFERELEASE(m_pPasswordEle); }

    HRESULT ParseForm(IHTMLFormElement *pFormEle, BOOL fRestoring);

    IHTMLInputTextElement *GetNameEle() { return m_pNameEle; }
    IHTMLInputTextElement *GetPasswordEle() { return m_pPasswordEle; }

protected:
    IHTMLInputTextElement  *m_pNameEle;
    IHTMLInputTextElement  *m_pPasswordEle;

    IHTMLInputTextElement  *m_pPasswordEle2;

    static HRESULT s_PasswordCB(IDispatch *pDispEle, DWORD_PTR dwCBData);
};

// if SUCCEEDED(hr), GetNameEle and GetPasswordEle are guaranteed non-NULL
HRESULT CDetectLoginForm::ParseForm(IHTMLFormElement *pFormEle, BOOL fRestoring)
{
    if (m_pPasswordEle || m_pNameEle || m_pPasswordEle2)
    {
        return E_FAIL;
    }

    CIntelliForms::CEnumCollection<IHTMLFormElement>::EnumCollection(pFormEle, s_PasswordCB, (DWORD_PTR)this);

    // For forms with two password fields (possibly used for login *and* new accounts)
    //  we clear the second field on PW restore and require it to be blank for saving.
    // Ideally, we would detect this as a password change situation as well.
    if (m_pPasswordEle2)
    {
        if (fRestoring)
        {
            BSTR bstrEmpty=SysAllocString(L"");
            if (bstrEmpty)
            {
                m_pPasswordEle2->put_value(bstrEmpty);
                SysFreeString(bstrEmpty);
            }
        }
        else
        {
            BSTR bstrVal=NULL;

            m_pPasswordEle2->get_value(&bstrVal);

            if (bstrVal && bstrVal[0])
            {
                // Failure! Second password field isn't empty.
                SAFERELEASE(m_pNameEle);
                SAFERELEASE(m_pPasswordEle);
            }

            SysFreeString(bstrVal);
        }

        SAFERELEASE(m_pPasswordEle2);   // Always release this
    }

    if (m_pPasswordEle && m_pNameEle)
    {
        return S_OK;
    }

    SAFERELEASE(m_pNameEle);
    SAFERELEASE(m_pPasswordEle);
    ASSERT(!m_pPasswordEle2);

    return E_FAIL;
}

// Password callback for CEnumCollection to find username and password fields
//   in a login form
HRESULT CDetectLoginForm::s_PasswordCB(IDispatch *pDispEle, DWORD_PTR dwCBData)
{
    CDetectLoginForm *pThis = (CDetectLoginForm *)dwCBData;

    HRESULT hr=S_OK;

    IHTMLInputTextElement *pTextEle=NULL;

    pDispEle->QueryInterface(IID_IHTMLInputTextElement, (void **)&pTextEle);

    if (pTextEle)
    {
        BSTR bstrType;

        pTextEle->get_type(&bstrType);

        if (bstrType)
        {
            if (!StrCmpICW(bstrType, L"text"))
            {
                // Assume this is the 'name' field
                if (pThis->m_pNameEle)
                {
                    // Whoops, we've already got a name field. Can't have two...
                    hr = E_ABORT;
                }
                else
                {
                    pThis->m_pNameEle = pTextEle;
                    pTextEle->AddRef();
                }
            }
            else if (!StrCmpICW(bstrType, L"password"))
            {
                // Assume this is the 'password' field
                if (pThis->m_pPasswordEle)
                {
                    // Whoops, we've already got a password field. Can't have two...
                    //  ...oh wait, yes we can...
                    if (pThis->m_pPasswordEle2)
                    {
                        // ...but we definitely can't have three!!!
                        hr = E_ABORT;
                    }
                    else
                    {
                        pThis->m_pPasswordEle2 = pTextEle;
                        pTextEle->AddRef();
                    }
                }
                else
                {
                    pThis->m_pPasswordEle = pTextEle;
                    pTextEle->AddRef();
                }
            }

            SysFreeString(bstrType);
        }

        pTextEle->Release();
    }

    if (hr == E_ABORT)
    {
        SAFERELEASE(pThis->m_pNameEle);
        SAFERELEASE(pThis->m_pPasswordEle);
        SAFERELEASE(pThis->m_pPasswordEle2);
    }

    return hr;
}

// Fill in passwords for this username, if one is available
HRESULT CIntelliForms::AutoFillPassword(IHTMLInputTextElement *pTextEle, LPCWSTR pwszUsername)
{
    BSTR bstrUrl = NULL;

    if (!pTextEle || !pwszUsername)
        return E_INVALIDARG;

    if (!IsEnabledRestorePW() || !LoadPasswords())
    {
        // We have no passwords for this url
        return S_FALSE;
    }

    int iIndex;

    if (SUCCEEDED(FindPasswordEntry(pwszUsername, &iIndex)))
    {
        // Returns index of password in m_pslPasswords
        ASSERT(iIndex>=0 && iIndex<m_pslPasswords->NumStrings() && (iIndex&1));

        FILETIME ft;

        // StringTime==0 indicates user said "no" to saving password
        if (SUCCEEDED(m_pslPasswords->GetStringTime(iIndex, &ft)) && (FILETIME_TO_INT64(ft) != 0))
        {
            TraceMsg(TF_IFORMS, "IntelliForms found saved password");

            // We have a password saved for this specific username. Fill it in.
            CDetectLoginForm LoginForm;
            IHTMLFormElement *pFormEle=NULL;
            HRESULT hr = E_FAIL;

            pTextEle->get_form(&pFormEle);
            if (pFormEle)
            {
                // See if this is a valid form: One plain text input, One password input. Find the fields.
                hr = LoginForm.ParseForm(pFormEle, TRUE);

                pFormEle->Release();
            }
            else
            {
                // Shouldn't get this far if we don't have a form for this element
                TraceMsg(TF_WARNING|TF_IFORMS, "Iforms: pITE->get_form() returned NULL!");
            }

            if (SUCCEEDED(hr))
            {
                BSTR bstrPW=NULL;
                m_pslPasswords->GetBSTR(iIndex, &bstrPW);
                if (bstrPW)
                {
                    LoginForm.GetPasswordEle()->put_value(bstrPW);
                    SysFreeString(bstrPW);
                    m_iRestoredIndex = iIndex;

                    // We restored this password. sink the SUBMIT for this form (if we haven't yet)
                    UserInput(pTextEle);
                }
            }
        }
        else
        {
            // User previously said 'no' to remembering passwords
            m_iRestoredIndex = -1;
        }
    }

    return S_OK;
}

HRESULT CIntelliForms::DeletePassword(LPCWSTR pwszUsername)
{
    // If we have a password, ask them if they want to delete it.
    if (LoadPasswords())
    {
        int iIndex;

        if (SUCCEEDED(FindPasswordEntry(pwszUsername, &iIndex)))
        {
            // If they previously said "no", delete without asking - they don't actually
            //  have a password saved
            // Otherwise, ask and delete only if they say "yes"
            FILETIME ft;
            if (FAILED(m_pslPasswords->GetStringTime(iIndex, &ft)) ||
                (0 == FILETIME_TO_INT64(ft)) ||
                (IDYES == DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_AUTOSUGGEST_DELETEPASSWORD),
                                m_hwndBrowser, AutoSuggestDlgProc, IDD_AUTOSUGGEST_DELETEPASSWORD)))
            {
                // Delete username then password from string list
                if (SUCCEEDED(m_pslPasswords->DeleteString(iIndex-1)) &&
                    SUCCEEDED(m_pslPasswords->DeleteString(iIndex-1)))
                {
                    TraceMsg(TF_IFORMS, "Deleting password for user \"%ws\"", pwszUsername);
                    ASSERT(!(m_pslPasswords->NumStrings() & 1));

                    if (m_iRestoredIndex == iIndex)
                    {
                        m_iRestoredIndex = -1;
                    }
                    else if (m_iRestoredIndex > iIndex)
                    {
                        m_iRestoredIndex -= 2;
                    }

                    if (m_pslPasswords->NumStrings() == 0)
                    {
                        // No more strings for this url. Nuke it.
                        DeleteFromStore(GetUrl());
                        SetPasswordsAreSaved(FALSE);
                        delete m_pslPasswords;
                        m_pslPasswords = NULL;
                        ASSERT(m_iRestoredIndex == -1);
                    }
                    else
                    {
                        SavePasswords();
                    }
                }
            }
        }
    }

    return S_OK;
}

HRESULT CIntelliForms::SavePassword(IHTMLFormElement *pFormEle, FILETIME ftSubmit, IHTMLInputTextElement *pFirstEle)
{
    if (m_fRestrictedPW ||
        !IsEnabledRestorePW())
    {
        return S_FALSE;
    }

    BOOL fAskUser = TRUE;

    // First let's check for previously saved entries for this username
    if (LoadPasswords())
    {
        int iIndex;

        BSTR bstrUserName=NULL;

        pFirstEle->get_value(&bstrUserName);

        if (bstrUserName)
        {
            if (SUCCEEDED(FindPasswordEntry(bstrUserName, &iIndex)))
            {
                FILETIME ft;
                if (SUCCEEDED(m_pslPasswords->GetStringTime(iIndex, &ft)))
                {
                    if (FILETIME_TO_INT64(ft) == (INT64)0)
                    {
                        // StringTime==0 means user previously said "no".
                        TraceMsg(TF_IFORMS, "IForms not asking about saving password");
                        fAskUser = FALSE;
                    }
                    else if (m_iRestoredIndex != iIndex)
                    {
                        // User previously said "yes" - but we didn't restore it for some reason
                        // Can happen with "back" button then submit
                        TraceMsg(TF_WARNING|TF_IFORMS, "IForms - user saved password and we didn't restore it");

                        // Write regkey in case that was the problem - we'll work next time
                        SetPasswordsAreSaved(TRUE);
                        m_iRestoredIndex = iIndex;
                    }
                }
            }
            else
            {
                m_iRestoredIndex = -1;
            }

            SysFreeString(bstrUserName);
        }
    }

    // Then lets ask the user if they'd like to save the password for this username
    if (fAskUser)
    {
        CDetectLoginForm LoginForm;

        // See if this is a valid form: One plain text input, One password input. Find the fields.

        if (SUCCEEDED(LoginForm.ParseForm(pFormEle, FALSE)))
        {
            TraceMsg(TF_IFORMS, "IForms Successfully detected 'save password' form");
            BSTR bstrUsername=NULL;
            BSTR bstrPassword=NULL;

            LoginForm.GetNameEle()->get_value(&bstrUsername);
            LoginForm.GetPasswordEle()->get_value(&bstrPassword);

            if (bstrUsername && bstrPassword)
            {
                if (m_iRestoredIndex != -1)
                {
                    // We have a previously saved password. See if our current entry is the same.
                    if (!StrCmpW(bstrPassword, m_pslPasswords->GetString(m_iRestoredIndex)))
                    {
                        // They're the same... nothing to do...
                        TraceMsg(TF_IFORMS, "IForms - user entered PW same as saved PW - nothing to do");
                        // Check to see that the username case is the same, just to be sure
                        if (StrCmpW(bstrUsername, m_pslPasswords->GetString(m_iRestoredIndex-1)))
                        {
                            TraceMsg(TF_IFORMS, "IForms - except change the username's case");
                            if (SUCCEEDED(m_pslPasswords->ReplaceString(m_iRestoredIndex-1, bstrUsername)))
                            {
                                SavePasswords();
                            }
                            else
                            {
                                // Something went horribly wrong!
                                delete m_pslPasswords;
                                m_pslPasswords=NULL;
                            }
                        }
                    }
                    else
                    {
                        // Ask the user if we want to change the saved password
                        INT_PTR iMB;

                        EnterModalDialog();

                        iMB = DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_AUTOSUGGEST_CHANGEPASSWORD),
                                    m_hwndBrowser, AutoSuggestDlgProc, IDD_AUTOSUGGEST_CHANGEPASSWORD);

                        if (IDYES == iMB)
                        {
                            // Delete the old one and add the new one. Update filetimes.
                            if (SUCCEEDED(m_pslPasswords->ReplaceString(m_iRestoredIndex, bstrPassword)))
                            {
                                m_pslPasswords->SetStringTime(m_iRestoredIndex, ftSubmit);
                                SavePasswords();
                                TraceMsg(TF_IFORMS, "IForms successfully saved changed password");
                            }
                            else
                            {
                                TraceMsg(TF_IFORMS|TF_WARNING, "IForms couldn't change password!");
                                delete m_pslPasswords;
                                m_pslPasswords = NULL;
                            }
                        }

                        LeaveModalDialog();
                    }
                }
                else
                {
                    // We don't have a previously saved password for this user. See if they want to save it.
                    // If the password is empty, don't bother asking or saving
                    if (IsEnabledAskPW() && bstrPassword[0])
                    {
                        EnterModalDialog();

                        INT_PTR iMB = DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_AUTOSUGGEST_SAVEPASSWORD),
                                        m_hwndBrowser, AutoSuggestDlgProc, IDD_AUTOSUGGEST_SAVEPASSWORD);

                        // If we can't load passwords, then create a new list
                        if (!LoadPasswords())
                        {
                            CStringList_New(&m_pslPasswords);
                            if (m_pslPasswords)
                                m_pslPasswords->SetListData(LIST_DATA_PASSWORD);
                        }

                        if (m_pslPasswords)
                        {
                            if ((IDCANCEL == iMB) || ((IDNO == iMB) && (!IsEnabledAskPW())))
                            {
                                // If they hit the close box or said "no" and checked "don't ask",
                                //  don't even save the username; we may ask them again next time
                            }
                            else
                            {
                                if (IDYES != iMB)
                                {
                                    // User said "no" but we save the username (no password) and
                                    //   set filetime to 0 which means they said "no"
                                    bstrPassword[0] = L'\0';
                                    FILETIME_TO_INT64(ftSubmit) = (INT64) 0;
                                }
                                else
                                {
                                    TraceMsg(TF_IFORMS, "IForms saving password for user %ws", bstrUsername);
                                }

                                m_pslPasswords->SetAutoScavenge(FALSE);

                                // Save the username and password, or just the username if they said "no"
                                if (SUCCEEDED(m_pslPasswords->AppendString(bstrUsername, ftSubmit)) &&
                                    SUCCEEDED(m_pslPasswords->AppendString(bstrPassword, ftSubmit)))
                                {
                                    SavePasswords();
                                }
                                else
                                {
                                    TraceMsg(TF_WARNING, "IForms couldn't save username/password");
                                    delete m_pslPasswords;
                                    m_pslPasswords=NULL;
                                }
                            }
                        }

                        LeaveModalDialog();
                    }
                }
            }

            SysFreeString(bstrUsername);
            SysFreeString(bstrPassword);
        } // if (SUCCEEDED(ParseForm()))
    }

    return S_OK;
}

// Returns reference to password string list if present. Return value must be used
//  immediately and not destroyed. Used only by CEnumString.
HRESULT CIntelliForms::GetPasswordStringList(CStringList **ppslPasswords)
{
    if (LoadPasswords())
    {
        *ppslPasswords = m_pslPasswords;
        return S_OK;
    }

    *ppslPasswords = NULL;
    return E_FAIL;
}


HRESULT CIntelliForms::CreatePStore()
{
    if (!m_pPStore)
    {
        if (!m_hinstPStore)
        {
            m_hinstPStore = LoadLibrary(TEXT("PSTOREC.DLL"));
        }

        if (m_hinstPStore)
        {
            HRESULT (* pfn)(IPStore **, PST_PROVIDERID *, void *, DWORD) = NULL;

            *(FARPROC *)&pfn = GetProcAddress(m_hinstPStore, "PStoreCreateInstance");

            if (pfn)
            {
                pfn(&m_pPStore, NULL, NULL, 0);
            }
        }
    }

    return m_pPStore ? S_OK : E_FAIL;
}

void CIntelliForms::ReleasePStore()
{
    SAFERELEASE(m_pPStore);
    if (m_hinstPStore)
    {
        FreeLibrary(m_hinstPStore);
        m_hinstPStore = NULL;
    }

    m_fPStoreTypeInit=FALSE;
}

// {E161255A-37C3-11d2-BCAA-00C04FD929DB}
static const GUID c_PStoreType =
{ 0xe161255a, 0x37c3, 0x11d2, { 0xbc, 0xaa, 0x0, 0xc0, 0x4f, 0xd9, 0x29, 0xdb } };
const TCHAR c_szIntelliForms[] = TEXT("Internet Explorer");

HRESULT CIntelliForms::CreatePStoreAndType()
{
    HRESULT hr;

    hr = CreatePStore();

    if (SUCCEEDED(hr) && !m_fPStoreTypeInit)
    {
        PST_TYPEINFO    typeInfo;

        typeInfo.cbSize = sizeof(typeInfo);
        typeInfo.szDisplayName = (LPTSTR)c_szIntelliForms;

        hr = m_pPStore->CreateType(PST_KEY_CURRENT_USER, &c_PStoreType, &typeInfo, 0);

        if (hr == PST_E_TYPE_EXISTS)
        {
            hr = S_OK;
        }

        if (SUCCEEDED(hr))
        {
            hr = m_pPStore->CreateSubtype(PST_KEY_CURRENT_USER, &c_PStoreType, &c_PStoreType, &typeInfo, NULL, 0);

            if (hr == PST_E_TYPE_EXISTS)
            {
                hr = S_OK;
            }
        }

        if (SUCCEEDED(hr))
        {
            m_fPStoreTypeInit = TRUE;
        }
    }

    return hr;
}

const WCHAR c_szBlob1Value[] = L"StringIndex";
const WCHAR c_szBlob2Value[] = L"StringData";

HRESULT CIntelliForms::WriteToStore(LPCWSTR pwszName, CStringList *psl)
{
    HRESULT hr = E_FAIL;

    if (SUCCEEDED(CreatePStoreAndType()))
    {
        LPBYTE pBlob1, pBlob2;
        DWORD  cbBlob1, cbBlob2;

        if (SUCCEEDED(psl->WriteToBlobs(&pBlob1, &cbBlob1, &pBlob2, &cbBlob2)))
        {
            PST_PROMPTINFO  promptInfo;

            promptInfo.cbSize = sizeof(promptInfo);
            promptInfo.dwPromptFlags = 0;
            promptInfo.hwndApp = NULL;
            promptInfo.szPrompt = NULL;

            LPWSTR pwszValue;

            int iValLen = lstrlenW(c_szBlob1Value) + lstrlenW(pwszName) + 10;

            pwszValue = (LPWSTR) LocalAlloc(LMEM_FIXED, iValLen * sizeof(WCHAR));

            if (pwszValue)
            {
                wnsprintfW(pwszValue, iValLen, L"%s:%s", pwszName, c_szBlob1Value);
                hr = m_pPStore->WriteItem(PST_KEY_CURRENT_USER, &c_PStoreType, &c_PStoreType, pwszValue,
                                        cbBlob1,
                                        pBlob1,
                                        &promptInfo, PST_CF_NONE, 0);

                if (SUCCEEDED(hr))
                {
                    wnsprintfW(pwszValue, iValLen, L"%s:%s", pwszName, c_szBlob2Value);
                    hr = m_pPStore->WriteItem(PST_KEY_CURRENT_USER, &c_PStoreType, &c_PStoreType, pwszValue,
                                            cbBlob2,
                                            pBlob2,
                                            &promptInfo, PST_CF_NONE, 0);
                }
                else
                {
                    // Delete bogus Blob1
                    m_pPStore->DeleteItem(PST_KEY_CURRENT_USER, &c_PStoreType, &c_PStoreType, pwszValue, &promptInfo, 0);
                }

                LocalFree(pwszValue);
            }

            if (pBlob1) LocalFree(pBlob1);
            if (pBlob2) LocalFree(pBlob2);
        }
    }

    return hr;
}

HRESULT CIntelliForms::ReadFromStore(LPCWSTR pwszName, CStringList **ppsl, BOOL fPasswordList/*=FALSE*/)
{
    HRESULT hr = E_FAIL;

    *ppsl=NULL;

    if (SUCCEEDED(CreatePStore()))
    {
        PST_PROMPTINFO  promptInfo;

        promptInfo.cbSize = sizeof(promptInfo);
        promptInfo.dwPromptFlags = 0;
        promptInfo.hwndApp = NULL;
        promptInfo.szPrompt = NULL;

        LPWSTR pwszValue;

        int iValLen = lstrlenW(c_szBlob1Value) + lstrlenW(pwszName) + 10;

        pwszValue = (LPWSTR) LocalAlloc(LMEM_FIXED, iValLen * sizeof(WCHAR));

        if (pwszValue)
        {
            DWORD dwBlob1Size, dwBlob2Size;
            LPBYTE pBlob1=NULL, pBlob2=NULL;

            wnsprintfW(pwszValue, iValLen, L"%s:%s", pwszName, c_szBlob1Value);
            hr = m_pPStore->ReadItem(PST_KEY_CURRENT_USER, &c_PStoreType, &c_PStoreType, pwszValue,
                                    &dwBlob1Size,
                                    &pBlob1,
                                    &promptInfo, 0);

            if (SUCCEEDED(hr))
            {
                wnsprintfW(pwszValue, iValLen, L"%s:%s", pwszName, c_szBlob2Value);
                hr = m_pPStore->ReadItem(PST_KEY_CURRENT_USER, &c_PStoreType, &c_PStoreType, pwszValue,
                                        &dwBlob2Size,
                                        &pBlob2,
                                        &promptInfo, 0);

                if (SUCCEEDED(hr))
                {
                    // bogus... have to reallocate here... bogus... bogus...
                    LPBYTE pBlob1b, pBlob2b;

                    pBlob1b=(LPBYTE)LocalAlloc(LMEM_FIXED, dwBlob1Size);
                    pBlob2b=(LPBYTE)LocalAlloc(LMEM_FIXED, dwBlob2Size);

                    if (pBlob1b && pBlob2b)
                    {
                        memcpy(pBlob1b, pBlob1, dwBlob1Size);
                        memcpy(pBlob2b, pBlob2, dwBlob2Size);

                        CStringList_New(ppsl);
                        if (*ppsl)
                        {
                            hr = (*ppsl)->ReadFromBlobs(&pBlob1b, dwBlob1Size, &pBlob2b, dwBlob2Size);

                            if (SUCCEEDED(hr))
                            {
                                INT64 i;

                                if (FAILED((*ppsl)->GetListData(&i)) ||
                                    ((fPasswordList && !(i & LIST_DATA_PASSWORD)) ||
                                     (!fPasswordList && (i & LIST_DATA_PASSWORD))))
                                {
                                    TraceMsg(TF_WARNING|TF_IFORMS, "IForms: Password/nonpassword lists mixed up");
                                    hr = E_FAIL;    // don't allow malicious site to access PW data
                                }
                            }

                            if (FAILED(hr))
                            {
                                delete *ppsl;
                                *ppsl=NULL;
                            }
                        }
                    }
                    else
                    {
                        if (pBlob1b) LocalFree(pBlob1b);
                        if (pBlob2b) LocalFree(pBlob2b);
                    }
                }
            }

            LocalFree(pwszValue);
            if (pBlob1) CoTaskMemFree(pBlob1);
            if (pBlob2) CoTaskMemFree(pBlob2);
        }
    }

    return hr;
}

HRESULT CIntelliForms::DeleteFromStore(LPCWSTR pwszName)
{
    HRESULT hr=E_FAIL;

    if (SUCCEEDED(CreatePStore()))
    {
        HRESULT hr1, hr2;
        LPWSTR pwszValue;

        int iValLen = lstrlenW(c_szBlob1Value) + lstrlenW(pwszName) + 10;

        pwszValue = (LPWSTR) LocalAlloc(LMEM_FIXED, iValLen * sizeof(WCHAR));

        if (pwszValue)
        {
            PST_PROMPTINFO  promptInfo;

            promptInfo.cbSize = sizeof(promptInfo);
            promptInfo.dwPromptFlags = 0;
            promptInfo.hwndApp = NULL;
            promptInfo.szPrompt = NULL;

            wnsprintfW(pwszValue, iValLen, L"%s:%s", pwszName, c_szBlob1Value);
            hr1 = m_pPStore->DeleteItem(PST_KEY_CURRENT_USER, &c_PStoreType, &c_PStoreType, pwszValue, &promptInfo, 0);

            wnsprintfW(pwszValue, iValLen, L"%s:%s", pwszName, c_szBlob2Value);
            hr2 = m_pPStore->DeleteItem(PST_KEY_CURRENT_USER, &c_PStoreType, &c_PStoreType, pwszValue, &promptInfo, 0);

            if (SUCCEEDED(hr1) && SUCCEEDED(hr2))
            {
                hr = S_OK;
            }

            LocalFree(pwszValue);
        }
    }

    return hr;
}


const int c_iEnumSize=256;

HRESULT CIntelliForms::ClearStore(DWORD dwClear)
{
    BOOL fReleasePStore = (m_pPStore == NULL);

    ASSERT(dwClear <= 2);

    if (dwClear > 2)
    {
        return E_INVALIDARG;
    }

    if (SUCCEEDED(CreatePStoreAndType()))
    {
        IEnumPStoreItems *pEnumItems;
        ULONG cFetched=0;

        do
        {
            if (SUCCEEDED(m_pPStore->EnumItems(PST_KEY_CURRENT_USER, &c_PStoreType, &c_PStoreType, 0, &pEnumItems)))
            {
                LPWSTR pwszName[c_iEnumSize];
                PST_PROMPTINFO  promptInfo;

                promptInfo.cbSize = sizeof(promptInfo);
                promptInfo.dwPromptFlags = 0;
                promptInfo.hwndApp = NULL;
                promptInfo.szPrompt = NULL;

                // Enumerator doesn't keep its state - deleting items while we enumerate makes us
                //  miss some. It does support celt>1... but returns failure codes when it succeeds.
                cFetched = 0;

                pEnumItems->Next(c_iEnumSize, pwszName, &cFetched);

                if (cFetched)
                {
                    for (ULONG i=0; i<cFetched; i++)
                    {
                        ASSERT(pwszName[i]);
                        if (pwszName[i])
                        {
                            BOOL fDelete = TRUE;

                            // Hack to work around PStore string-case bug: first take their
                            //  enum value literally, then convert to lowercase and do it
                            //  again; IE5.0 #71001
                            for (int iHack=0; iHack<2; iHack++)
                            {
                                if (iHack == 1)
                                {
                                    // Convert the pwszName[i] to lowercase... only before
                                    //  the colon...
                                    WCHAR *pwch = StrRChrW(pwszName[i], NULL, L':');
                                    if (pwch)
                                    {
                                        *pwch = L'\0';
                                        MyToLower(pwszName[i]);
                                        *pwch = L':';
                                    }
                                    else
                                        break;
                                }

                                if (dwClear != IECMDID_ARG_CLEAR_FORMS_ALL)
                                {
                                    fDelete = FALSE;

                                    // See if this is a password item or not
                                    // This is pretty annoying. Since our string lists are split
                                    //  into two blobs, we need to find out which one this is and
                                    //  load the index for it.
                                    WCHAR *pwch = StrRChrW(pwszName[i], NULL, L':');
                                    if (pwch)
                                    {
                                        LPWSTR pwszIndexName=NULL;
                                        if (!StrCmpCW(pwch+1, c_szBlob2Value))
                                        {
                                            int iSize = sizeof(WCHAR) * (lstrlenW(pwszName[i])+10);
                                            pwszIndexName = (LPWSTR) LocalAlloc(LMEM_FIXED, iSize);
                                            if (pwszIndexName)
                                            {
                                                *pwch = L'\0';
                                                wnsprintfW(pwszIndexName, iSize, L"%s:%s", pwszName[i], c_szBlob1Value);
                                                *pwch = L':';
                                            }
                                        }

                                        DWORD dwBlob1Size;
                                        LPBYTE pBlob1=NULL;
                                        INT64 iFlags;

                                        if (SUCCEEDED(m_pPStore->ReadItem(
                                                PST_KEY_CURRENT_USER,
                                                &c_PStoreType, &c_PStoreType,
                                                (pwszIndexName) ? pwszIndexName : pwszName[i],
                                                &dwBlob1Size,
                                                &pBlob1,
                                                &promptInfo, 0)) && pBlob1)
                                        {
                                            if (SUCCEEDED(CStringList::GetFlagsFromIndex(pBlob1, &iFlags)))
                                            {
                                                if (((iFlags & LIST_DATA_PASSWORD) && (dwClear == IECMDID_ARG_CLEAR_FORMS_PASSWORDS_ONLY)) ||
                                                    (!(iFlags & LIST_DATA_PASSWORD) && (dwClear == IECMDID_ARG_CLEAR_FORMS_ALL_BUT_PASSWORDS)))
                                                {
                                                    // Delete this item
                                                    fDelete = TRUE;
                                                }
                                            }

                                            CoTaskMemFree(pBlob1);
                                        }
                                        else
                                        {
                                            // The index is already deleted
                                            fDelete = TRUE;
                                        }

                                        if (pwszIndexName)
                                        {
                                            LocalFree(pwszIndexName);
                                        }
                                    }
                                } // if (dwClear != CLEAR_INTELLIFORMS_ALL)

                                if (fDelete)
                                {
                                    m_pPStore->DeleteItem(PST_KEY_CURRENT_USER, &c_PStoreType, &c_PStoreType, pwszName[i], &promptInfo, 0);
                                }
                            } // for (iHack)

                            CoTaskMemFree(pwszName[i]);
                        } // if (pwszName[i])
                    }
                }

                pEnumItems->Release();
            }
        }
        while (cFetched == c_iEnumSize);  // In case we didn't cover everything in one pass

        if (dwClear == IECMDID_ARG_CLEAR_FORMS_ALL)
        {
            m_pPStore->DeleteSubtype(PST_KEY_CURRENT_USER, &c_PStoreType, &c_PStoreType, 0);
            m_pPStore->DeleteType(PST_KEY_CURRENT_USER, &c_PStoreType, 0);
        }

        if ((dwClear == IECMDID_ARG_CLEAR_FORMS_ALL) ||
            (dwClear == IECMDID_ARG_CLEAR_FORMS_PASSWORDS_ONLY))
        {
            // Delete the urlhash key storing which urls we have passwords saved for
            SHDeleteKey(HKEY_CURRENT_USER, c_szRegKeyIntelliForms);
        }

        TraceMsg(TF_IFORMS, "IForms: ClearStore cleared at least %d entries", cFetched);
    }

    if (fReleasePStore)
    {
        ReleasePStore();
    }

    return S_OK;
}

// static: Get the name from an input element - uses VCARD_NAME attribute if present.
HRESULT CIntelliForms::GetName(IHTMLInputTextElement *pTextEle, BSTR *pbstrName)
{
    IHTMLElement *pEle=NULL;

    *pbstrName = NULL;

    pTextEle->QueryInterface(IID_IHTMLElement, (void **)&pEle);

    if (pEle)
    {
        BSTR bstrAttr = SysAllocString(L"VCARD_NAME");

        if (bstrAttr)
        {
            VARIANT var;
            var.vt = VT_EMPTY;

            pEle->getAttribute(bstrAttr, 0, &var);

            if (var.vt == VT_BSTR && var.bstrVal)
            {
                *pbstrName = var.bstrVal;
            }
            else
            {
                VariantClear(&var);
            }

            SysFreeString(bstrAttr);
        }

        pEle->Release();
    }

    if (!*pbstrName)
    {
        pTextEle->get_name(pbstrName);
    }

    // Convert the name to lowercase
    if (*pbstrName)
    {
        // Call "MyToLower" instead
        if (g_fRunningOnNT)
        {
            CharLowerBuffW(*pbstrName, lstrlenW(*pbstrName));
        }
        else
        {
            // Ideally we would use the code page contained in the string instead of
            //  the system code page.
            CHAR chBuf[MAX_PATH];
            SHUnicodeToAnsi(*pbstrName, chBuf, ARRAYSIZE(chBuf));
            CharLowerBuffA(chBuf, lstrlenA(chBuf));
            SHAnsiToUnicode(chBuf, *pbstrName, SysStringLen(*pbstrName)+1);
        }
    }

    return (*pbstrName) ? S_OK : E_FAIL;
}

// Called when script calls window.external.AutoCompleteSaveForm
HRESULT CIntelliForms::ScriptSubmit(IHTMLFormElement *pForm)
{
    HRESULT hr = E_FAIL;

    if (pForm)
    {
        hr = HandleFormSubmit(pForm);
    }

    return SUCCEEDED(hr) ? S_OK : S_FALSE;
}


// Called when user changes a text field. Mark it "dirty" and sink submit event for form
HRESULT CIntelliForms::UserInput(IHTMLInputTextElement *pTextEle)
{
    AddToElementList(pTextEle);

    IHTMLFormElement *pForm=NULL;
    pTextEle->get_form(&pForm);

    if (pForm)
    {
        if (S_OK == AddToFormList(pForm))
        {
            AttachToForm(pForm);
        }

        pForm->Release();
    }
    else
    {
        TraceMsg(TF_WARNING|TF_IFORMS, "Iforms: pITE->get_form() returned NULL!");
    }


    return S_OK;
}

HRESULT CIntelliForms::AddToElementList(IHTMLInputTextElement *pITE)
{
    if (m_hdpaElements)
    {
        if (SUCCEEDED(FindInElementList(pITE)))
        {
            return S_FALSE;
        }
    }
    else
    {
        m_hdpaElements = DPA_Create(4);
    }

    if (m_hdpaElements)
    {
        TraceMsg(TF_IFORMS, "CIntelliForms::AddToElementList adding");

        if (DPA_AppendPtr(m_hdpaElements, pITE) >= 0)
        {
            pITE->AddRef();
            return S_OK;
        }
    }

    return E_OUTOFMEMORY;
}

HRESULT CIntelliForms::FindInElementList(IHTMLInputTextElement *pITE)
{
    IUnknown *punk;
    HRESULT hr = E_FAIL;

    pITE->QueryInterface(IID_IUnknown, (void **)&punk);

    if (m_hdpaElements)
    {
        for (int i=DPA_GetPtrCount(m_hdpaElements)-1; i>=0; i--)
        {
            IUnknown *punk2;

            ((IUnknown *)DPA_FastGetPtr(m_hdpaElements, i))->QueryInterface(IID_IUnknown, (void **)&punk2);

            if (punk == punk2)
            {
                punk2->Release();
                break;
            }

            punk2->Release();
        }

        if (i >= 0)
        {
            hr = S_OK;
        }
    }

    punk->Release();

    return hr;
}

void CIntelliForms::FreeElementList()
{
    if (m_hdpaElements)
    {
        for (int i=DPA_GetPtrCount(m_hdpaElements)-1; i>=0; i--)
        {
            ((IUnknown *)(DPA_FastGetPtr(m_hdpaElements, i)))->Release();
        }

        DPA_Destroy(m_hdpaElements);
        m_hdpaElements=NULL;
    }
}

HRESULT CIntelliForms::AddToFormList(IHTMLFormElement *pFormEle)
{
    if (m_hdpaForms)
    {
        if (SUCCEEDED(FindInFormList(pFormEle)))
        {
            return S_FALSE;
        }
    }
    else
    {
        m_hdpaForms = DPA_Create(2);
    }

    if (m_hdpaForms)
    {
        if (DPA_AppendPtr(m_hdpaForms, pFormEle) >= 0)
        {
            TraceMsg(TF_IFORMS, "CIntelliForms::AddToFormList adding");

            pFormEle->AddRef();
            return S_OK;
        }
    }

    return E_OUTOFMEMORY;
}

HRESULT CIntelliForms::FindInFormList(IHTMLFormElement *pFormEle)
{
    IUnknown *punk;
    HRESULT hr = E_FAIL;

    pFormEle->QueryInterface(IID_IUnknown, (void **)&punk);

    if (m_hdpaForms)
    {
        for (int i=DPA_GetPtrCount(m_hdpaForms)-1; i>=0; i--)
        {
            IUnknown *punk2;

            ((IUnknown *)DPA_FastGetPtr(m_hdpaForms, i))->QueryInterface(IID_IUnknown, (void **)&punk2);

            if (punk == punk2)
            {
                punk2->Release();
                break;
            }

            punk2->Release();
        }

        if (i >= 0)
        {
            hr = S_OK;
        }
    }

    punk->Release();

    return hr;
}

void CIntelliForms::FreeFormList()
{
    if (m_hdpaForms)
    {
        for (int i=DPA_GetPtrCount(m_hdpaForms)-1; i>=0; i--)
        {
            ((IUnknown *)(DPA_FastGetPtr(m_hdpaForms, i)))->Release();
        }

        DPA_Destroy(m_hdpaForms);
        m_hdpaForms = NULL;
    }
}

//=========================================================================
//
// Event sinking class
//
//  We simply implement IDispatch and make a call into our parent when
//   we receive a sinked event.
//
//=========================================================================
CIntelliForms::CEventSink::CEventSink(CEventSinkCallback *pParent)
{
    TraceMsg(TF_IFORMS, "CIntelliForms::CEventSink::CEventSink");
    DllAddRef();
    m_cRef = 1;
    m_pParent = pParent;
}

CIntelliForms::CEventSink::~CEventSink()
{
    TraceMsg(TF_IFORMS, "CIntelliForms::CEventSink::~CEventSink");
    ASSERT( m_cRef == 0 );
    DllRelease();
}

STDMETHODIMP CIntelliForms::CEventSink::QueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;

    if ((IID_IDispatch == riid) ||
        (IID_IUnknown == riid))
    {
        *ppv = (IDispatch *)this;
    }

    if (NULL != *ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CIntelliForms::CEventSink::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CIntelliForms::CEventSink::Release(void)
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

HRESULT CIntelliForms::CEventSink::SinkEvents(IHTMLElement2 *pEle2, int iNum, EVENTS *pEvents)
{
    VARIANT_BOOL bSuccess = VARIANT_TRUE;

    for (int i=0; i<iNum; i++)
    {
        BSTR bstrEvent = SysAllocString(CEventSinkCallback::EventsToSink[(int)(pEvents[i])].pwszEventSubscribe);

        if (bstrEvent)
        {
            pEle2->attachEvent(bstrEvent, (IDispatch *)this, &bSuccess);

            SysFreeString(bstrEvent);
        }
        else
        {
            bSuccess = VARIANT_FALSE;
        }

        if (!bSuccess)
            break;
    }

    return (bSuccess) ? S_OK : E_FAIL;
}

HRESULT CIntelliForms::CEventSink::SinkEvents(IHTMLWindow3 *pWin3, int iNum, EVENTS *pEvents)
{
    VARIANT_BOOL bSuccess = VARIANT_TRUE;

    for (int i=0; i<iNum; i++)
    {
        BSTR bstrEvent = SysAllocString(CEventSinkCallback::EventsToSink[(int)(pEvents[i])].pwszEventSubscribe);

        if (bstrEvent)
        {
            pWin3->attachEvent(bstrEvent, (IDispatch *)this, &bSuccess);

            SysFreeString(bstrEvent);
        }
        else
        {
            bSuccess = VARIANT_FALSE;
        }

        if (!bSuccess)
            break;
    }

    return (bSuccess) ? S_OK : E_FAIL;
}

HRESULT CIntelliForms::CEventSink::UnSinkEvents(IHTMLElement2 *pEle2, int iNum, EVENTS *pEvents)
{
    for (int i=0; i<iNum; i++)
    {
        BSTR bstrEvent = SysAllocString(CEventSinkCallback::EventsToSink[(int)(pEvents[i])].pwszEventSubscribe);

        if (bstrEvent)
        {
            pEle2->detachEvent(bstrEvent, (IDispatch *)this);

            SysFreeString(bstrEvent);
        }
    }

    return S_OK;
}

HRESULT CIntelliForms::CEventSink::UnSinkEvents(IHTMLWindow3 *pWin3, int iNum, EVENTS *pEvents)
{
    for (int i=0; i<iNum; i++)
    {
        BSTR bstrEvent = SysAllocString(CEventSinkCallback::EventsToSink[(int)(pEvents[i])].pwszEventSubscribe);

        if (bstrEvent)
        {
            pWin3->detachEvent(bstrEvent, (IDispatch *)this);

            SysFreeString(bstrEvent);
        }
    }

    return S_OK;
}

// IDispatch
STDMETHODIMP CIntelliForms::CEventSink::GetTypeInfoCount(UINT* /*pctinfo*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP CIntelliForms::CEventSink::GetTypeInfo(/* [in] */ UINT /*iTInfo*/,
            /* [in] */ LCID /*lcid*/,
            /* [out] */ ITypeInfo** /*ppTInfo*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP CIntelliForms::CEventSink::GetIDsOfNames(
                REFIID          riid,
                OLECHAR**       rgszNames,
                UINT            cNames,
                LCID            lcid,
                DISPID*         rgDispId)
{
    return E_NOTIMPL;
}

STDMETHODIMP CIntelliForms::CEventSink::Invoke(
            DISPID dispIdMember,
            REFIID, LCID,
            WORD wFlags,
            DISPPARAMS* pDispParams,
            VARIANT* pVarResult,
            EXCEPINFO*,
            UINT* puArgErr)
{
    if (m_pParent && pDispParams && pDispParams->cArgs>=1)
    {
        if (pDispParams->rgvarg[0].vt == VT_DISPATCH)
        {
            IHTMLEventObj *pObj=NULL;

            if (SUCCEEDED(pDispParams->rgvarg[0].pdispVal->QueryInterface(IID_IHTMLEventObj, (void **)&pObj) && pObj))
            {
                EVENTS Event=EVENT_BOGUS;
                BSTR bstrEvent=NULL;

                pObj->get_type(&bstrEvent);

                if (bstrEvent)
                {
                    for (int i=0; i<ARRAYSIZE(CEventSinkCallback::EventsToSink); i++)
                    {
                        if (!StrCmpCW(bstrEvent, CEventSinkCallback::EventsToSink[i].pwszEventName))
                        {
                            Event = (EVENTS) i;
                            break;
                        }
                    }

                    SysFreeString(bstrEvent);
                }

                if (Event != EVENT_BOGUS)
                {
                    IHTMLElement *pEle=NULL;

                    pObj->get_srcElement(&pEle);

                    // EVENT_SCROLL comes from our window so we won't have an
                    //  element for it
                    if (pEle || (Event == EVENT_SCROLL))
                    {
                        // Call the event handler here
                        m_pParent->HandleEvent(pEle, Event, pObj);

                        if (pEle)
                        {
                            pEle->Release();
                        }
                    }
                }

                pObj->Release();
            }
        }
    }

    return S_OK;
}


//=========================================================================
//
// AutoSuggest class
//
// Handles connecting and disconnecting the AutoComplete object, as well
//  as translating between Trident OM and Edit window messages
//=========================================================================

CIntelliForms::CAutoSuggest::CAutoSuggest(CIntelliForms *pParent, BOOL fEnabled, BOOL fEnabledPW)
{
    TraceMsg(TF_IFORMS, "CIntelliForms::CAutoSuggest::CAutoSuggest");

    m_pParent = pParent;
    m_fEnabled = fEnabled;
    m_fEnabledPW = fEnabledPW;

    ASSERT(m_pEventSink == NULL);
    ASSERT(m_pAutoComplete == NULL);
    ASSERT(m_hwndEdit == NULL);
    ASSERT(m_pTextEle == NULL);

    //
    // bug 81414 : To avoid clashing with app messages used by the edit window, we 
    // use registered messages.
    //
    m_uMsgItemActivate = RegisterWindowMessageA("AC_ItemActivate");
    if (m_uMsgItemActivate == 0)
    {
        m_uMsgItemActivate = WM_APP + 301;
    }

    // Register our window class if necessary
    if (!s_fRegisteredWndClass)
    {
        s_fRegisteredWndClass = TRUE;

        WNDCLASSEXW wndclass =
        {
            sizeof(WNDCLASSEX),
            0,
            CIntelliForms::CAutoSuggest::WndProc,
            0,
            sizeof(DWORD_PTR),
            g_hinst,
            NULL,
            NULL,
            NULL,
            NULL,
            c_szEditWndClass
        };

        if (!RegisterClassEx(&wndclass))
        {
            TraceMsg(TF_IFORMS, "Intelliforms failed to register wnd class!");
        }
    }
}

CIntelliForms::CAutoSuggest::~CAutoSuggest()
{
    TraceMsg(TF_IFORMS, "CIntelliForms::CAutoSuggest::~CAutoSuggest");
    CleanUp();
}

HRESULT CIntelliForms::CAutoSuggest::CleanUp()
{
    SetParent(NULL);
    DetachFromInput();

    return S_OK;
}

// List of all events we sink for an individual INPUT tag
CEventSinkCallback::EVENTS CIntelliForms::CAutoSuggest::s_EventsToSink[] =
        {
            EVENT_KEYPRESS,
            EVENT_KEYDOWN,
            EVENT_MOUSEDOWN,
            EVENT_DBLCLICK,
            EVENT_FOCUS,
            EVENT_BLUR
        };

HRESULT CIntelliForms::CAutoSuggest::AttachToInput(IHTMLInputTextElement *pTextEle)
{
    HRESULT hr;

    TraceMsg(TF_IFORMS, "CIntelliForms::CAutoSuggest::AttachToInput");

    if (!pTextEle)
        return E_INVALIDARG;

    hr = DetachFromInput();

    if (SUCCEEDED(hr))
    {
        m_pTextEle = pTextEle;
        pTextEle->AddRef();

        if (!m_pEventSink)
        {
            m_pEventSink = new CEventSink(this);

            if (!m_pEventSink)
            {
                hr = E_OUTOFMEMORY;
            }
        }

        if (SUCCEEDED(hr))
        {
            // Hook up our event sink
            IHTMLElement2 *pEle2=NULL;

            hr = pTextEle->QueryInterface(IID_IHTMLElement2, (void **)&pEle2);

            if (pEle2)
            {
                hr = m_pEventSink->SinkEvents(pEle2, ARRAYSIZE(s_EventsToSink), s_EventsToSink);

                pEle2->Release();
            }
        }
    }

    if (FAILED(hr))
    {
        TraceMsg(TF_IFORMS, "IForms: AttachToInput failed");
        DetachFromInput();
    }

    return hr;
}

HRESULT CIntelliForms::CAutoSuggest::DetachFromInput()
{
    if (!m_pTextEle)
        return S_FALSE;

    TraceMsg(TF_IFORMS, "CIntelliForms::CAutoSuggest::DetachFromInput");

    // Auto Fill Password here, since we get ACTIVEELEMENT change before blur event
    BSTR bstrUsername=NULL;
    m_pTextEle->get_value(&bstrUsername);
    if (bstrUsername)
    {
        CheckAutoFillPassword(bstrUsername);
        SysFreeString(bstrUsername);
    }

    if (m_bstrLastUsername)
    {
        SysFreeString(m_bstrLastUsername);
        m_bstrLastUsername=NULL;
    }

    if (m_hwndEdit)
    {
        // This is for subclass wndproc
        SendMessage(m_hwndEdit, WM_KILLFOCUS, 0, 0);
    }

    if (m_pEnumString)
    {
        m_pEnumString->UnInit();
        m_pEnumString->Release();
        m_pEnumString = NULL;
    }

    if (m_pEventSink)
    {
        IHTMLElement2 *pEle2=NULL;

        m_pTextEle->QueryInterface(IID_IHTMLElement2, (void **)&pEle2);

        if (pEle2)
        {
            m_pEventSink->UnSinkEvents(pEle2, ARRAYSIZE(s_EventsToSink), s_EventsToSink);
            pEle2->Release();
        }

        m_pEventSink->SetParent(NULL);
        m_pEventSink->Release();
        m_pEventSink=NULL;
    }

    SAFERELEASE(m_pAutoComplete);
    SAFERELEASE(m_pAutoCompleteDD);
    if (m_hwndEdit)
    {
        DestroyWindow(m_hwndEdit);
        m_hwndEdit = NULL;
    }
    SAFERELEASE(m_pTextEle);

    m_fInitAutoComplete = FALSE;

    return S_OK;
}

// Creates autocomplete and string enumerator.
HRESULT CIntelliForms::CAutoSuggest::CreateAutoComplete()
{
    if (m_fInitAutoComplete)
    {
        return (m_pAutoCompleteDD != NULL) ? S_OK : E_FAIL;
    }

    HRESULT hr=S_OK;

    ASSERT(!m_hwndEdit && !m_pEnumString && !m_pAutoComplete && !m_pAutoCompleteDD);

    // Create the edit window
#ifndef UNIX
    m_hwndEdit = CreateWindowEx(0, c_szEditWndClass, TEXT("IntelliFormProxy"), WS_POPUP,
#else
    m_hwndEdit = CreateWindowEx(WS_EX_MW_UNMANAGED_WINDOW, c_szEditWndClass, TEXT("IntelliFormProxy"), WS_POPUP,
#endif
        300, 200, 200, 50, m_pParent->m_hwndBrowser, NULL, g_hinst, this);

    if (!m_hwndEdit)
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        // Create our enumerator
        m_pEnumString = new CEnumString();

        if (m_pEnumString)
        {
            m_pEnumString->Init(m_pTextEle, m_pParent);

            // Create the AutoComplete Object
            if (!m_pAutoComplete)
            {
                hr = CoCreateInstance(CLSID_AutoComplete, NULL, CLSCTX_INPROC_SERVER, IID_IAutoComplete2, (void **)&m_pAutoComplete);
                if (m_pAutoComplete)
                {
                    m_pAutoComplete->QueryInterface(IID_IAutoCompleteDropDown, (void **)&m_pAutoCompleteDD);
                    if (!m_pAutoCompleteDD)
                    {
                        SAFERELEASE(m_pAutoComplete);
                    }
                }
            }

            if (m_pAutoComplete)
            {
                hr = m_pAutoComplete->Init(m_hwndEdit, (IUnknown *) m_pEnumString, NULL, NULL);

                DWORD dwOptions = ACO_AUTOSUGGEST | ACO_UPDOWNKEYDROPSLIST;

                // Add the RTLREADING option to the dropdown, if the element is RTL
                BSTR bstrDir = NULL;

                IHTMLElement2 *pEle2=NULL;
                m_pTextEle->QueryInterface(IID_IHTMLElement2, (void **)&pEle2);
                if (pEle2)
                {
                    pEle2->get_dir(&bstrDir);
                    pEle2->Release();
                }

                if (bstrDir)
                {
                    if (!StrCmpIW(bstrDir, L"RTL"))
                    {
                        dwOptions |= ACO_RTLREADING;
                    }

                    SysFreeString(bstrDir);
                }

                m_pAutoComplete->SetOptions(dwOptions);
            }
        }
    }

    m_fInitAutoComplete = TRUE;

    ASSERT_MSG(SUCCEEDED(hr), "IForms: CreateAutoComplete failed");

    return hr;
}

void CIntelliForms::CAutoSuggest::CheckAutoFillPassword(LPCWSTR pwszUsername)
{
    // We don't autofill their password unless we know they've hit a key
    if (m_pParent && m_fEnabledPW && m_fAllowAutoFillPW)
    {
        if (m_bstrLastUsername && !StrCmpCW(pwszUsername, m_bstrLastUsername))
        {
            return;
        }

        SysFreeString(m_bstrLastUsername);
        m_bstrLastUsername = SysAllocString(pwszUsername);

        m_pParent->AutoFillPassword(m_pTextEle, pwszUsername);
    }
}

HRESULT GetScreenCoordinates(IUnknown *punkEle, HWND hwnd, long *plLeft, long *plTop, long *plWidth, long *plHeight)
{
    long lScreenLeft=0, lScreenTop=0;
    HRESULT hr = E_FAIL;

    *plLeft = *plTop = *plWidth = *plHeight = 0;

    IHTMLElement2 *pEle2;
    if (SUCCEEDED(punkEle->QueryInterface(IID_IHTMLElement2, (void **)&pEle2)) && pEle2)
    {
        IHTMLRect *pRect=NULL;
        if (SUCCEEDED(pEle2->getBoundingClientRect(&pRect)) && pRect)
        {
            IHTMLWindow2 *pWin2;

            long lLeft, lRight, lTop, lBottom;

            pRect->get_left(&lLeft);
            pRect->get_right(&lRight);
            pRect->get_top(&lTop);
            pRect->get_bottom(&lBottom);

            lBottom -= 2;           // put dropdown on top of edit box
            if (lBottom < lTop)
            {
                lBottom = lTop;
            }

            if (lTop >= 0 && lLeft >= 0)
            {
                GetStuffFromEle(punkEle, &pWin2, NULL);

                if (pWin2)
                {
                    IHTMLWindow3 *pWin3;

                    if (SUCCEEDED(pWin2->QueryInterface(IID_IHTMLWindow3, (void **)&pWin3)) && pWin3)
                    {
                        RECT rcBrowserWnd;

                        pWin3->get_screenLeft(&lScreenLeft);
                        pWin3->get_screenTop(&lScreenTop);

                        if (GetWindowRect(hwnd, &rcBrowserWnd))
                        {
                            // Clip the right edge to the window
                            if (lRight+lScreenLeft > rcBrowserWnd.right)
                            {
                                lRight = rcBrowserWnd.right - lScreenLeft;
                            }

                            *plLeft = lScreenLeft + lLeft;
                            *plWidth = lRight-lLeft;
                            *plTop = lScreenTop + lTop;
                            *plHeight = lBottom-lTop;

                            hr = S_OK;

                            if (*plWidth < MINIMUM_WIDTH)
                            {
                                // Primitive minimum width for now
                                *plWidth = MINIMUM_WIDTH;
                            }
                        }

                        pWin3->Release();
                    }

                    pWin2->Release();
                }
            }

            pRect->Release();
        }
        pEle2->Release();
    }

    return hr;
}

HRESULT CIntelliForms::CAutoSuggest::UpdateDropdownPosition()
{
    if (m_pTextEle && m_pParent && m_hwndEdit)
    {
        long lLeft, lTop, lWidth, lHeight;

        if (SUCCEEDED(GetScreenCoordinates(m_pTextEle, m_pParent->m_hwndBrowser, &lLeft, &lTop, &lWidth, &lHeight)))
        {
            MoveWindow(m_hwndEdit, lLeft, lTop, lWidth, lHeight, FALSE);
        }
        else
        {
            // Send "escape" key to autocomplete so that it hides the dropdown.
            // This will happen if dropdown moves outside of parent window, for example.
            SendMessage(m_hwndEdit, IF_CHAR, (WPARAM) VK_ESCAPE, 0);
        }
    }

    return S_OK;
}

HRESULT CIntelliForms::CAutoSuggest::HandleEvent(IHTMLElement *pEle, EVENTS Event, IHTMLEventObj *pEventObj)
{
    TraceMsg(TF_IFORMS, "CIntelliForms::CAutoSuggest::HandleEvent Event=%ws", EventsToSink[Event].pwszEventName);

    ASSERT(SHIsSameObject(pEle, m_pTextEle));

    long lKey=0;

    if (!m_pParent)
    {
        TraceMsg(TF_WARNING|TF_IFORMS, "IForms autosuggest receiving events while invalid");
        return E_FAIL;
    }

    if (Event == EVENT_KEYPRESS || Event == EVENT_KEYDOWN)
    {
        pEventObj->get_keyCode(&lKey);
    }

    if (!m_fEnabled && !m_fEnabledPW)
    {
        // If the dropdown isn't enabled, our only purpose is to tell Intelliforms when
        //  user activity occurs for the first-time enable dialog box.
        if (Event == EVENT_KEYPRESS && lKey != VK_TAB)
        {
            // Add this element to the master list so we save it when we submit
            //  and sink the submit event for this element's form
            MarkDirty();
        }

        return S_OK;
    }

    if (Event == EVENT_KEYDOWN || Event == EVENT_KEYPRESS ||
        Event == EVENT_MOUSEDOWN || Event == EVENT_DBLCLICK)
    {
        m_fAllowAutoFillPW = TRUE;

        // Create our autocomplete object if it hasn't happened yet.
        // If it's "tab" we don't create it; we're leaving the field
        if (lKey != VK_TAB)
        {
            if (FAILED(CreateAutoComplete()))
                return E_FAIL;
        }
        else
        {
            // Add this element to the master list so we save it when we submit
            //  and sink the submit event for this element's form
            MarkDirty();
        }

        ASSERT((m_pEnumString && m_hwndEdit) || (lKey==VK_TAB));
    }

    // If AutoComplete hasn't been initialized there's nothing for us to do
    if (!m_pAutoCompleteDD || !m_hwndEdit)
    {
        return E_FAIL;
    }

    // Update the position of our hidden edit box
    long lLeft, lTop, lWidth, lHeight;

    // BUGBUG call UpdateDropdownPosition instead
    if (SUCCEEDED(GetScreenCoordinates(pEle, m_pParent->m_hwndBrowser, &lLeft, &lTop, &lWidth, &lHeight)))
    {
        MoveWindow(m_hwndEdit, lLeft, lTop, lWidth, lHeight, FALSE);
    }

    if (Event == EVENT_FOCUS)
    {
        SendMessage(m_hwndEdit, WM_SETFOCUS, 0, 0);
    }
    else if (Event == EVENT_BLUR)
    {
        if (m_hwndEdit)
        {
            SendMessage(m_hwndEdit, WM_KILLFOCUS, 0, 0);
        }

        // BUGBUG ensure that script hasn't changed value of edit field
        BSTR bstrUsername=NULL;
        m_pTextEle->get_value(&bstrUsername);
        if (bstrUsername)
        {
            CheckAutoFillPassword(bstrUsername);
            SysFreeString(bstrUsername);
        }
    }
    else if (Event == EVENT_MOUSEDOWN || Event == EVENT_DBLCLICK)
    {
        // If the dropdown is invisible, give AutoComplete a downarrow
        long lButton=0;
        pEventObj->get_button(&lButton);
        if ((Event == EVENT_DBLCLICK) ||
            (lButton & 1))                      // Left button down?
        {
            DWORD dwFlags;

            if (SUCCEEDED(m_pAutoCompleteDD->GetDropDownStatus(&dwFlags, NULL)) &&
                !(dwFlags & ACDD_VISIBLE))
            {
                TraceMsg(TF_IFORMS, "IForms sending downarrow because of mouse click");
                PostMessage(m_hwndEdit, IF_KEYDOWN, (WPARAM)VK_DOWN, 0);
                m_fEscapeHit = FALSE;
            }
        }
    }
    else if (Event == EVENT_KEYPRESS)
    {
        // Add this element to the master list so we save it when we submit
        //  and sink the submit event for this element's form
        MarkDirty();

        // Ignore ctrl-enter (quickcomplete) (may be unnecessary)
        if (lKey == VK_RETURN)
        {
            VARIANT_BOOL bCtrl;
            if (SUCCEEDED(pEventObj->get_ctrlKey(&bCtrl)) && bCtrl)
            {
                lKey = 0;
            }
        }

        if (lKey != 0)
        {
            if (lKey == m_lCancelKeyPress)
            {
                // tell MSHTML to ignore this keystroke (may be tab, enter, escape)
                TraceMsg(TF_IFORMS, "Intelliforms cancelling default action for EVENT_KEYPRESS=%d", lKey);

                VARIANT v;
                v.vt = VT_BOOL;
                v.boolVal = VARIANT_FALSE;
                pEventObj->put_returnValue(v);
                if(!(lKey == VK_DOWN || lKey == VK_UP))
                    pEventObj->put_cancelBubble(VARIANT_TRUE);
            }

            m_lCancelKeyPress = 0;

            // Tell AutoComplete about this keystroke
            if (!m_fEscapeHit)
            {
//              TraceMsg(TF_IFORMS, "IForms: Posting IF_CHAR CharCode = %d", lKey);
                PostMessage(m_hwndEdit, IF_CHAR, (WPARAM)lKey, 0);
            }
        }
    }
    else if (Event == EVENT_KEYDOWN)
    {
        long    lKey;
        BOOL    fCancelEvent=FALSE,         // Cancel default MSHTML action?
                fForwardKeystroke=TRUE;     // Forward keystroke to AutoComplete?

        pEventObj->get_keyCode(&lKey);

        if (m_fEscapeHit)
        {
            // They dismissed the dropdown; don't bring it back unless they ask for it
            if (lKey == VK_DOWN)
            {
                m_fEscapeHit = FALSE;
            }
            else
            {
                fForwardKeystroke = FALSE;
            }
        }

        if (lKey != 0)
        {
            if ((lKey == VK_RETURN) || (lKey == VK_TAB))
            {
                fForwardKeystroke=FALSE;

                LPWSTR pwszString=NULL;

                if (SUCCEEDED(m_pAutoCompleteDD->GetDropDownStatus(NULL, &pwszString)) && pwszString)
                {
                    // User is inside dropdown
                    fForwardKeystroke=TRUE;

                    // Set this value into our edit field
                    SetText(pwszString);

                    // We will fill in their password if they asked for it in m_uMsgItemActivate

                    if (lKey == VK_RETURN)
                    {
                        // Avoid submitting this form
                        fCancelEvent = TRUE;
                    }

                    CoTaskMemFree(pwszString);
                }
                else if (lKey == VK_RETURN)
                {
                    // User's gonna submit. Give 'em their password first.
                    // BUGBUG ensure that script hasn't changed value of edit field
                    BSTR bstrUsername=NULL;
                    m_pTextEle->get_value(&bstrUsername);
                    if (bstrUsername)
                    {
                        CheckAutoFillPassword(bstrUsername);
                        SysFreeString(bstrUsername);
                    }
                }
            }
            else if (lKey == VK_DELETE)
            {
                LPWSTR pwszString=NULL;

                if (SUCCEEDED(m_pAutoCompleteDD->GetDropDownStatus(NULL, &pwszString)) && pwszString)
                {
                    // User is inside dropdown
                    fForwardKeystroke=FALSE;

                    // Delete this value from our string lists
                    CStringList *psl=NULL;
                    BSTR bstrName;

                    CIntelliForms::GetName(m_pTextEle, &bstrName);

                    if (bstrName)
                    {
                        int iIndex;

                        if (SUCCEEDED(m_pParent->ReadFromStore(bstrName, &psl)) &&
                            SUCCEEDED(psl->FindString(pwszString, -1, &iIndex, FALSE)))
                        {
                            TraceMsg(TF_IFORMS, "IForms: Deleting string \"%ws\"", pwszString);
                            psl->DeleteString(iIndex);

                            // We deleted string.
                            if (psl->NumStrings() > 0)
                            {
                                m_pParent->WriteToStore(bstrName, psl);
                            }
                            else
                            {
                                m_pParent->DeleteFromStore(bstrName);
                            }
                        }
                    }

                    SysFreeString(bstrName);
                    if (psl) delete psl;

                    // avoid deleting a character from the edit window; user was inside dropdown
                    fCancelEvent = TRUE;

                    // Check this url to see if we should maybe delete a password entry
                    m_pParent->DeletePassword(pwszString);

                    // Get AutoComplete to fill in the dropdown again
                    m_pEnumString->ResetEnum();
                    m_pAutoCompleteDD->ResetEnumerator();

                    CoTaskMemFree(pwszString);
                }
            }

            if (lKey == VK_ESCAPE)
            {
                DWORD dwFlags;

                if (SUCCEEDED(m_pAutoCompleteDD->GetDropDownStatus(&dwFlags, NULL)) &&
                    (dwFlags & ACDD_VISIBLE))
                {
                    fCancelEvent = TRUE;
                    m_fEscapeHit = TRUE;
                }
            }

            if (lKey == VK_DOWN || lKey == VK_UP)
            {
                // Cancel the MSHTML events. This will cause MSHTML to return
                //  S_OK instead of S_FALSE from its TranslateAccelerator, and we
                //  won't get multiple keystrokes in different panes
                fCancelEvent = TRUE;
            }

            if (fForwardKeystroke)
            {
                PostMessage(m_hwndEdit, IF_KEYDOWN, lKey, 0);

                if (lKey == VK_BACK)
                {
                    // Never get OnKeyPress for this guy
                    PostMessage(m_hwndEdit, IF_CHAR, lKey, 0);
                }
            }

            if (fCancelEvent)
            {
                TraceMsg(TF_IFORMS, "Intelliforms cancelling default action for EVENT_KEYDOWN=%d", lKey);

                m_lCancelKeyPress = lKey;    // Cancel the EVENT_KEYPRESS when it comes

                VARIANT v;
                v.vt = VT_BOOL;
                v.boolVal = VARIANT_FALSE;
                pEventObj->put_returnValue(v);
                if(!(lKey == VK_DOWN || lKey == VK_UP))
                    pEventObj->put_cancelBubble(VARIANT_TRUE);
            }
            else
            {
                m_lCancelKeyPress = 0;
            }
        }
    }

    return S_OK;
}

HRESULT CIntelliForms::CAutoSuggest::GetText(int cchTextMax, LPWSTR pszTextOut, LRESULT *lcchCopied)
{
    *pszTextOut = TEXT('\0');
    *lcchCopied = 0;

    if (m_pTextEle)
    {
        BSTR bstr=NULL;
        m_pTextEle->get_value(&bstr);
        if (bstr)
        {
            StrCpyN(pszTextOut, bstr, cchTextMax);
            *lcchCopied = lstrlenW(pszTextOut);     // needed for NT

            SysFreeString(bstr);
        }
    }

//  TraceMsg(TF_IFORMS, "IForms: Handling WM_GETTEXT return = %ws", pszTextOut);

    return (*pszTextOut) ? S_OK : E_FAIL;
}

HRESULT CIntelliForms::CAutoSuggest::GetTextLength(int *pcch)
{
    *pcch = 0;

    if (m_pTextEle)
    {
        BSTR bstr=NULL;
        m_pTextEle->get_value(&bstr);
        if (bstr)
        {
            *pcch = SysStringLen(bstr);

            SysFreeString(bstr);
        }
    }

//  TraceMsg(TF_IFORMS, "IForms: Handling WM_GETTEXTLENGTH (%d)", *pcch);

    return S_OK;
}

HRESULT CIntelliForms::CAutoSuggest::SetText(LPCWSTR pszTextIn)
{
    if (m_pTextEle && pszTextIn)
    {
        BSTR bstr=SysAllocString(pszTextIn);

        if (bstr)
        {
            // Even though we know we already have this string in our dropdown, mark
            //  it as dirty so that we sink submit event; can be necessary in saved
            //  password situation.
            MarkDirty();

            // Make sure we don't put a string longer than the max length in this field
            long lMaxLen=-1;
            m_pTextEle->get_maxLength(&lMaxLen);
            if ((lMaxLen >= 0) && (lstrlenW(bstr) > lMaxLen))
            {
                bstr[lMaxLen] = L'\0';
            }

            m_pTextEle->put_value(bstr);
            SysFreeString(bstr);
        }
    }

    TraceMsg(TF_IFORMS, "CIntelliForms::CAutoSuggest::SetText \"%ws\"", pszTextIn);

    return S_OK;
}

#define MY_GWL_THISPTR 0

LRESULT CALLBACK CIntelliForms::CAutoSuggest::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CIntelliForms::CAutoSuggest *pThis = (CIntelliForms::CAutoSuggest *)GetWindowLongPtr(hwnd, MY_GWL_THISPTR);

    switch (uMsg)
    {
    case WM_CREATE:
        {
            LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;

            if (!pcs || !(pcs->lpCreateParams))
            {
                return -1;
            }
            SetWindowLongPtr(hwnd, MY_GWL_THISPTR, (LONG_PTR) pcs->lpCreateParams);
            return 0;
        }

    case WM_GETTEXT:
        if (pThis)
        {
            LRESULT lcchCopied=0;

            if (g_fRunningOnNT)
            {
                pThis->GetText((int)wParam, (LPWSTR) lParam, &lcchCopied);
            }
            else
            {
                // We are actually an ANSI window. Convert.
                LPWSTR pwszOutBuf = (LPWSTR) LocalAlloc(LPTR, (wParam+1)*sizeof(WCHAR));

                if (pwszOutBuf)
                {
                    pThis->GetText((int)wParam, pwszOutBuf, &lcchCopied);

                    SHUnicodeToAnsi(pwszOutBuf, (LPSTR) lParam, (int)(wParam+1));

                    LocalFree((HLOCAL)pwszOutBuf);
                }
            }
            return lcchCopied;
        }

        return 0;

    case WM_GETTEXTLENGTH:
        if (pThis)
        {
            int iLen;
            pThis->GetTextLength(&iLen);
            return iLen;
        }

        return 0;

    case EM_GETSEL:
        // Must return zeroes here or autocomp will use uninitialized
        //  values and crash
        if (wParam) (*(DWORD *)wParam) = 0;
        if (lParam) (*(DWORD *)lParam) = 0;
        break;

    case IF_CHAR:
        SendMessage(hwnd, WM_CHAR, wParam, lParam);
        break;

    case IF_KEYDOWN:
        SendMessage(hwnd, WM_KEYDOWN, wParam, lParam);
        break;

    case WM_KEYDOWN:
    case WM_CHAR:
        return 0;       // eat it (see notes at top of file)

    default:

        // Check registered message
        if (pThis && uMsg == pThis->m_uMsgItemActivate)
        {
            TraceMsg(TF_IFORMS, "IForms: Received AM_ITEMACTIVATE(WM_APP+2)");
            pThis->SetText((LPCWSTR)lParam);
            pThis->CheckAutoFillPassword((LPCWSTR)lParam);

            return 0;
        }

        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 1;
}

CIntelliForms::CAutoSuggest::CEnumString::CEnumString()
{
//  TraceMsg(TF_IFORMS, "CIntelliForms::CAutoSuggest::CEnumString::CEnumString");
    DllAddRef();

    InitializeCriticalSection(&m_crit);

    // Remove from dbg mem list since it's released on autocomplete thread
    remove_from_memlist(this);

    m_cRef = 1;
}

CIntelliForms::CAutoSuggest::CEnumString::~CEnumString()
{
//  TraceMsg(TF_IFORMS, "CIntelliForms::CAutoSuggest::CEnumString::~CEnumString");
    if (m_pslMain)
    {
        delete m_pslMain;
    }
    SysFreeString(m_bstrName);
    if (m_pszOpsValue)
    {
        CoTaskMemFree(m_pszOpsValue);
    }

    DeleteCriticalSection(&m_crit);

    DllRelease();
}

HRESULT CIntelliForms::CAutoSuggest::CEnumString::Init(IHTMLInputTextElement *pInputEle, CIntelliForms *pIntelliForms)
{
    if (m_fInit ||              // Can only init once
        !pInputEle || !pIntelliForms)       // Need both pointers
    {
        return E_FAIL;
    }

    m_fInit=TRUE;
    m_pIntelliForms = pIntelliForms;

    // Take care of things that must be done on the main thread. Autocomplete will
    //  call us on a secondary thread to do the enumeration.
    CIntelliForms::GetName(pInputEle, &m_bstrName);

    if (m_bstrName && m_bstrName[0])
    {
        // See if this specifies the "vcard." format
        if (IsEnabledInCPL() &&
            !StrCmpNICW(m_bstrName, c_wszVCardPrefix, ARRAYSIZE(c_wszVCardPrefix)-1))
        {
            // It does. Retrieve string from the profile assistant store.
            IHTMLWindow2 *pWin2;

            GetStuffFromEle(pInputEle, &pWin2, NULL);

            if (pWin2)
            {
                IOmNavigator *pNav=NULL;
                pWin2->get_navigator(&pNav);
                if (pNav)
                {
                    IHTMLOpsProfile *pProfile=NULL;
                    pNav->get_userProfile(&pProfile);
                    if (pProfile)
                    {
                        IOpsProfileSimple *pSimple=NULL;
                        pProfile->QueryInterface(IID_IOpsProfileSimple, (void **)&pSimple);
                        if (pSimple)
                        {
                            pSimple->ReadProperties(1, &m_bstrName, &m_pszOpsValue);
                            pSimple->Release();
                        }
                        pProfile->Release();
                    }
                    pNav->Release();
                }
                pWin2->Release();
            }
        }
    }

    return S_OK;
}

void CIntelliForms::CAutoSuggest::CEnumString::UnInit()
{
    EnterCriticalSection(&m_crit);

    m_pIntelliForms = NULL;

    LeaveCriticalSection(&m_crit);
}

HRESULT CIntelliForms::CAutoSuggest::CEnumString::ResetEnum()
{
    EnterCriticalSection(&m_crit);

    if (m_pslMain)
    {
        delete m_pslMain;
        m_pslMain = NULL;
    }

    m_fFilledStrings = FALSE;

    LeaveCriticalSection(&m_crit);

    return S_OK;
}

STDMETHODIMP CIntelliForms::CAutoSuggest::CEnumString::QueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;

    if ((IID_IEnumString == riid) ||
        (IID_IUnknown == riid))
    {
        *ppv = (IEnumString *)this;
    }

    if (NULL != *ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CIntelliForms::CAutoSuggest::CEnumString::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CIntelliForms::CAutoSuggest::CEnumString::Release(void)
{
    if (InterlockedDecrement(&m_cRef) != 0)
    {
        return 1;
    }

    delete this;
    return 0;
}

STDMETHODIMP CIntelliForms::CAutoSuggest::CEnumString::Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
{
    EnterCriticalSection(&m_crit);

    if (!m_fFilledStrings)
    {
        FillEnumerator();
    }

    if (m_pslMain)
    {
        int iNewPtr = m_iPtr + celt;

        if (iNewPtr > m_pslMain->NumStrings())
        {
            iNewPtr = m_pslMain->NumStrings();
        }

        *pceltFetched = iNewPtr - m_iPtr;

        LPOLESTR lpstr;

        for (; m_iPtr < iNewPtr; m_iPtr ++)
        {
             m_pslMain->GetTaskAllocString(m_iPtr, &lpstr);

             if (!lpstr) break;

             *(rgelt ++) = lpstr;
        }

        if (m_iPtr < iNewPtr)
        {
            *pceltFetched += (m_iPtr - iNewPtr);
        }
    }

    LeaveCriticalSection(&m_crit);

    if (!m_pslMain)
    {
        return E_FAIL;
    }

    return (*pceltFetched) ? S_OK : S_FALSE;
}

STDMETHODIMP CIntelliForms::CAutoSuggest::CEnumString::Reset()
{
    EnterCriticalSection(&m_crit);

    m_iPtr = 0;

    LeaveCriticalSection(&m_crit);

    return S_OK;
}

HRESULT CIntelliForms::CAutoSuggest::CEnumString::FillEnumerator()
{
    // Already in critical section
    ASSERT(!m_pslMain);

    if (m_fFilledStrings)
    {
        return S_FALSE;
    }

    if (!m_bstrName || !m_bstrName[0] || !m_pIntelliForms)
    {
        return E_FAIL;
    }

    m_fFilledStrings = TRUE;

    m_iPtr = 0;

    // Fill the enumerator based on our name
    TraceMsg(TF_IFORMS, "IForms: Intelliforms filling enumerator");

    // Open any previously saved strings
    if (!m_pIntelliForms->IsRestricted() &&
        IsEnabledInCPL() &&
        m_pIntelliForms->IsEnabledForPage())
    {
        m_pIntelliForms->ReadFromStore(m_bstrName, &m_pslMain);

        // Add in profile assistant value, if any
        if (m_pszOpsValue && m_pszOpsValue[0])
        {
            if (!m_pslMain)
            {
                CStringList_New(&m_pslMain);
            }
            else
            {
                // don't risk a scavenge (perf)
                m_pslMain->SetMaxStrings(CStringList::MAX_STRINGS+4);
            }

            if (m_pslMain)
            {
                m_pslMain->AddString(m_pszOpsValue);
            }
        }
    }

    // Next fill with any usernames that have saved passwords
    CStringList *pslPasswords;

    if (!m_pIntelliForms->IsRestrictedPW() &&
        CIntelliForms::IsEnabledRestorePW() &&
        SUCCEEDED(m_pIntelliForms->GetPasswordStringList(&pslPasswords)))
    {
        ASSERT(!(pslPasswords->NumStrings() & 1));

        FILETIME ft;

        if (pslPasswords->NumStrings() > 0)
        {
            if (!m_pslMain)
            {
                CStringList_New(&m_pslMain);
            }
            else
            {
                // avoid expensive scavenging while adding usernames to string list
                m_pslMain->SetMaxStrings(m_pslMain->GetMaxStrings() + pslPasswords->NumStrings()/2);
            }

            if (m_pslMain)
            {
                for (int i=0; i<pslPasswords->NumStrings(); i+=2)
                {
                    if (SUCCEEDED(pslPasswords->GetStringTime(i, &ft)) &&
                        FILETIME_TO_INT64(ft) != 0)
                    {
                        // We have a saved password for this username. Add username to enumerator.
                        m_pslMain->AddString(pslPasswords->GetString(i));
                    }
                }
            }
        }

        // do not delete pslPasswords
    }

    return (m_pslMain) ? ((m_pslMain->NumStrings()) ? S_OK : S_FALSE) : E_FAIL;
}

// Static helper. Pretty basic.
HRESULT CStringList_New(CStringList **ppNew, BOOL fAutoDelete/*=TRUE*/)
{
    *ppNew = new CStringList();

    if (*ppNew)
    {
        (*ppNew)->SetAutoScavenge(fAutoDelete);
    }

    return (*ppNew) ? S_OK : E_OUTOFMEMORY;
}

CStringList::CStringList()
{
    TraceMsg(TF_IFORMS, "IForms: CStringList::CStringList");
    m_fAutoScavenge = TRUE;
    m_dwMaxStrings = MAX_STRINGS;
}

CStringList::~CStringList()
{
    TraceMsg(TF_IFORMS, "IForms: CStringList::~CStringList");
    CleanUp();
}

void CStringList::CleanUp()
{
    if (m_psiIndex)
    {
        LocalFree(m_psiIndex);
        m_psiIndex = NULL;
    }
    if (m_pBuffer)
    {
        LocalFree(m_pBuffer);
        m_pBuffer = NULL;
    }
    m_dwIndexSize = 0;
    m_dwBufEnd = m_dwBufSize = 0;
}

HRESULT CStringList::WriteToBlobs(LPBYTE *ppBlob1, DWORD *pcbBlob1, LPBYTE *ppBlob2, DWORD *pcbBlob2)
{
    HRESULT hr = E_FAIL;

    if (SUCCEEDED(ConvertToExternalFormat()))
    {
        DWORD dwIndexSize;

        dwIndexSize = INDEX_SIZE(m_psiIndex->dwNumStrings);
        ASSERT(dwIndexSize <= m_dwIndexSize);

        *ppBlob1 = (LPBYTE) LocalAlloc(LMEM_FIXED, dwIndexSize);
        if (*ppBlob1)
        {
            *ppBlob2 = (LPBYTE) LocalAlloc(LMEM_FIXED, m_dwBufEnd);

            if (*ppBlob2)
            {
                memcpy(*ppBlob1, m_psiIndex, dwIndexSize);
                *pcbBlob1=dwIndexSize;

                memcpy(*ppBlob2, m_pBuffer, m_dwBufEnd);
                *pcbBlob2=m_dwBufEnd;

                hr = S_OK;
            }
        }

        ConvertToInternalFormat();
    }

    if (FAILED(hr))
    {
        if (*ppBlob1)
        {
            LocalFree(*ppBlob1);
            *ppBlob1=NULL;
        }
        if (*ppBlob2)
        {
            LocalFree(*ppBlob2);
            *ppBlob2=NULL;
        }
        *pcbBlob1=0;
        *pcbBlob2=0;
    }

    return hr;
}

// Take the blobs and use as our buffer
HRESULT CStringList::ReadFromBlobs(LPBYTE *ppBlob1, DWORD cbBlob1, LPBYTE *ppBlob2, DWORD cbBlob2)
{
    HRESULT hr = E_FAIL;

    if (m_psiIndex)
    {
        TraceMsg(TF_IFORMS, "IForms: CStringList::ReadFromRegistry called with initialized instance.");
        CleanUp();
    }

    // Allocate our buffers.
    m_psiIndex = (StringIndex *) (*ppBlob1);
    m_pBuffer = (LPBYTE) (*ppBlob2);

    *ppBlob1 = NULL;
    *ppBlob2 = NULL;

    if (!m_psiIndex || !m_pBuffer || !cbBlob1 || !cbBlob2)
    {
        // Nothing to do
        CleanUp();

        return S_FALSE;
    }

    // Validate our string index.
    if ((m_psiIndex->dwSignature == INDEX_SIGNATURE) &&
        (m_psiIndex->cbSize == STRINGINDEX_CBSIZE) &&
        (m_psiIndex->dwNumStrings <= MAX_STRINGS))
    {
        m_dwBufEnd = m_dwBufSize = cbBlob2;
        m_dwIndexSize = cbBlob1;

        if (SUCCEEDED(ConvertToInternalFormat()))
        {
            // Everything worked. Amazing.
            hr = S_OK;
        }
    }

    if (FAILED(hr))
    {
        // Release buffers if necessary.
        CleanUp();
    }

    return hr;
}

// static
HRESULT CStringList::GetFlagsFromIndex(LPBYTE pBlob1, INT64 *piFlags)
{
    StringIndex *psiIndex = (StringIndex *)pBlob1;

    if ((psiIndex->dwSignature == INDEX_SIGNATURE) &&
        (psiIndex->cbSize == STRINGINDEX_CBSIZE))
    {
        *piFlags = psiIndex->iData;

        return S_OK;
    }

    return E_FAIL;
}

// Internal helpers to convert from internal to registry format
HRESULT CStringList::ConvertToExternalFormat()
{
    if (!m_psiIndex || !m_pBuffer)
    {
        return E_FAIL;
    }

    DWORD dw;
    DWORD_PTR dwStringPtr;

    for (dw=0; dw < m_psiIndex->dwNumStrings; dw++)
    {
        // Convert from pointer to offset
        dwStringPtr = (LPBYTE)(m_psiIndex->StringEntry[dw].pwszString) - (LPBYTE)m_pBuffer;
        ASSERT(dwStringPtr < m_dwBufSize);
        m_psiIndex->StringEntry[dw].dwStringPtr = dwStringPtr;
    }

    return S_OK;
}

HRESULT CStringList::ConvertToInternalFormat()
{
    if (!m_psiIndex || !m_pBuffer)
    {
        return E_FAIL;
    }

    DWORD       dw;

    for (dw=0; dw < m_psiIndex->dwNumStrings; dw++)
    {
        DWORD_PTR dwPtr = m_psiIndex->StringEntry[dw].dwStringPtr;
        if (dwPtr >= m_dwBufSize)
        {
            ASSERT(dwPtr < m_dwBufSize);

            return E_FAIL;      // Bad
        }

        // Convert from offset to pointer
        m_psiIndex->StringEntry[dw].pwszString = (LPWSTR)(dwPtr + (DWORD_PTR)m_pBuffer);
    }

    return S_OK;
}

HRESULT CStringList::Init(DWORD dwBufSize /* =0 */)
{
    DWORD dwMaxStrings=0;
    DWORD dwIndexSize=0;

    if (m_psiIndex)
    {
        TraceMsg(TF_IFORMS, "IForms: CStringList::Init called when already initialized");
        CleanUp();
    }

    if (dwBufSize == 0)
    {
        dwBufSize = INIT_BUF_SIZE;
    }

    dwMaxStrings = dwBufSize >> 5;  // this is relatively arbitrary but doesn't matter much

    if (dwMaxStrings == 0)
        dwMaxStrings = 1;

    dwIndexSize = INDEX_SIZE(dwMaxStrings);

    m_pBuffer = (LPBYTE)LocalAlloc(LMEM_FIXED, dwBufSize);
    m_psiIndex = (StringIndex *)LocalAlloc(LMEM_FIXED, dwIndexSize);

    if ((NULL == m_psiIndex) ||
        (NULL == m_pBuffer))
    {
        TraceMsg(TF_IFORMS, "IForms: CStringList::Init memory allocation failed");

        CleanUp();
        return E_OUTOFMEMORY;
    }

    *((WCHAR *)m_pBuffer) = L'\0';

    m_dwBufSize = dwBufSize;
    m_dwBufEnd = 0;

    m_psiIndex->dwSignature = INDEX_SIGNATURE;
    m_psiIndex->cbSize = STRINGINDEX_CBSIZE;
    m_psiIndex->dwNumStrings = 0;
    m_psiIndex->iData = 0;
    m_dwIndexSize = dwIndexSize;

    TraceMsg(TF_IFORMS, "IForms: CStringList::Init succeeded");

    return S_OK;
}

HRESULT CStringList::GetBSTR(int iIndex, BSTR *pbstrRet)
{
    LPCWSTR lpwstr = GetString(iIndex);

    if (!lpwstr)
    {
        *pbstrRet = NULL;
        return E_INVALIDARG;
    }

    *pbstrRet = SysAllocString(lpwstr);

    return (*pbstrRet) ? S_OK : E_OUTOFMEMORY;
}

HRESULT CStringList::GetTaskAllocString(int iIndex, LPOLESTR *pRet)
{
    LPCWSTR lpwstr = GetString(iIndex);

    if (!lpwstr)
    {
        *pRet = NULL;
        return E_INVALIDARG;
    }

    DWORD dwSize = (GetStringLen(iIndex)+1) * sizeof(WCHAR);

    *pRet = (LPOLESTR)CoTaskMemAlloc(dwSize);

    if (!*pRet)
    {
        return E_OUTOFMEMORY;
    }

    memcpy(*pRet, lpwstr, dwSize);

    return S_OK;
}

HRESULT CStringList::FindString(LPCWSTR lpwstr, int iLen, int *piNum, BOOL fCaseSensitive)
{
    if (!m_psiIndex) return E_FAIL;

    DWORD dw;

    if (!lpwstr)
    {
        return E_INVALIDARG;
    }

    if (iLen <= 0)
    {
        iLen = lstrlenW(lpwstr);
    }

    if (piNum)
    {
        *piNum = -1;
    }

    for (dw=0; dw<m_psiIndex->dwNumStrings; dw++)
    {
        if (m_psiIndex->StringEntry[dw].dwStringLen == (DWORD)iLen)
        {
            if ((fCaseSensitive && (!StrCmpW(m_psiIndex->StringEntry[dw].pwszString, lpwstr))) ||
                (!fCaseSensitive && (!StrCmpIW(m_psiIndex->StringEntry[dw].pwszString, lpwstr))))
            {
                // Match!
                if (piNum)
                {
                    *piNum = (int) dw;
                }

                return S_OK;
            }
        }
    }

    return E_FAIL;      // Couldn't find it
}

// CStringList is not optimized for deleting
HRESULT CStringList::DeleteString(int iIndex)
{
    if (!m_psiIndex)
    {
        return E_FAIL;
    }

    if ((iIndex<0) || ((DWORD)iIndex >= m_psiIndex->dwNumStrings))
    {
        return E_INVALIDARG;
    }

    if ((DWORD)iIndex == (m_psiIndex->dwNumStrings-1))
    {
        // Simple case - deleting last string
        m_dwBufEnd -= (sizeof(WCHAR) * (GetStringLen(iIndex) + 1));
        m_psiIndex->dwNumStrings --;
        return S_OK;
    }

    DWORD cbSizeDeleted;
    LPCWSTR pwszString1, pwszString2;

    pwszString1 = m_psiIndex->StringEntry[iIndex].pwszString;
    pwszString2 = m_psiIndex->StringEntry[iIndex+1].pwszString;

    // Size in bytes of string to be deleted including null terminator
    cbSizeDeleted = (DWORD)((DWORD_PTR)pwszString2 - (DWORD_PTR)pwszString1);

    ASSERT(cbSizeDeleted == (sizeof(WCHAR) * (lstrlenW(GetString(iIndex))+1)));

    // Delete entry in index
    memcpy(&(m_psiIndex->StringEntry[iIndex]), &(m_psiIndex->StringEntry[iIndex+1]),
                STRINGENTRY_SIZE*(m_psiIndex->dwNumStrings - iIndex - 1));
    m_psiIndex->dwNumStrings --;

    // Delete string in buffer
    memcpy((LPWSTR)pwszString1, pwszString2, m_dwBufEnd-(int)PtrDiff(pwszString2, m_pBuffer));
    m_dwBufEnd -= cbSizeDeleted;

    // Fix up pointers in index
    for (int i=iIndex; (DWORD)i < m_psiIndex->dwNumStrings; i++)
    {
        m_psiIndex->StringEntry[i].dwStringPtr -= cbSizeDeleted;
    }

    return S_OK;
}

HRESULT CStringList::InsertString(int iIndex, LPCWSTR lpwstr)
{
    if (!m_psiIndex)
    {
        return E_FAIL;
    }

    if ((iIndex<0) || ((DWORD)iIndex > m_psiIndex->dwNumStrings))
    {
        return E_INVALIDARG;
    }

    if ((DWORD)iIndex == m_psiIndex->dwNumStrings)
    {
        // Simple case - inserting to end
        return _AddString(lpwstr, FALSE, NULL);
    }

    DWORD dwLen = (DWORD)lstrlenW(lpwstr);
    DWORD dwSizeInserted = sizeof(WCHAR) * (dwLen + 1);

    if (FAILED(EnsureBuffer(m_dwBufEnd + dwSizeInserted)) ||
        FAILED(EnsureIndex(m_psiIndex->dwNumStrings + 1)))
    {
        return E_OUTOFMEMORY;
    }

    // Insert into buffer
    LPWSTR pwszBufLoc = m_psiIndex->StringEntry[iIndex].pwszString;

    memcpy((LPBYTE)pwszBufLoc + dwSizeInserted, pwszBufLoc, m_dwBufEnd - (int) PtrDiff(pwszBufLoc, m_pBuffer));
    memcpy(pwszBufLoc, lpwstr, dwSizeInserted);
    m_dwBufEnd += dwSizeInserted;

    // Insert into index
    memcpy(&(m_psiIndex->StringEntry[iIndex+1]), &(m_psiIndex->StringEntry[iIndex]),
                STRINGENTRY_SIZE*(m_psiIndex->dwNumStrings - iIndex));
    struct StringIndex::tagStringEntry *pse=&(m_psiIndex->StringEntry[iIndex]);
    pse->pwszString = pwszBufLoc;
    FILETIME_TO_INT64(pse->ftLastSubmitted) = 0;
    pse->dwStringLen = dwLen;
    m_psiIndex->dwNumStrings ++;

    // Fix up pointers after inserted string
    for (int i=iIndex+1; (DWORD)i<m_psiIndex->dwNumStrings; i++)
    {
        m_psiIndex->StringEntry[i].dwStringPtr += dwSizeInserted;
    }

    return S_OK;
}

HRESULT CStringList::ReplaceString(int iIndex, LPCWSTR lpwstr)
{
    if (!m_psiIndex)
    {
        return E_FAIL;
    }

    if ((iIndex<0) || ((DWORD)iIndex >= m_psiIndex->dwNumStrings))
    {
        return E_INVALIDARG;
    }

    if ((DWORD)lstrlenW(lpwstr) == m_psiIndex->StringEntry[iIndex].dwStringLen)
    {
        // Simple case - strings equal length
        memcpy( m_psiIndex->StringEntry[iIndex].pwszString,
                lpwstr,
                 (m_psiIndex->StringEntry[iIndex].dwStringLen)*sizeof(WCHAR));

        return S_OK;
    }

    // Delete old string, then insert new one
    DeleteString(iIndex);

    return InsertString(iIndex, lpwstr);
}

HRESULT CStringList::AddString(LPCWSTR lpwstr, FILETIME ft, int *piNum /*=NULL*/)
{
    int iNum;
    HRESULT hr;

    hr = _AddString(lpwstr, TRUE, &iNum);

    if (piNum)
    {
        *piNum = iNum;
    }

    if (SUCCEEDED(hr))
    {
        UpdateStringTime(iNum, ft);
    }

    return hr;
}


HRESULT CStringList::AddString(LPCWSTR lpwstr, int *piNum /*=NULL*/)
{
    return _AddString(lpwstr, TRUE, piNum);
}

HRESULT CStringList::AppendString(LPCWSTR lpwstr, int *piNum /*=NULL*/)
{
    return _AddString(lpwstr, FALSE, piNum);
}

HRESULT CStringList::AppendString(LPCWSTR lpwstr, FILETIME ft, int *piNum /*=NULL*/)
{
    int iNum;
    HRESULT hr;

    hr = _AddString(lpwstr, FALSE, &iNum);

    if (piNum)
    {
        *piNum = iNum;
    }

    if (SUCCEEDED(hr))
    {
        SetStringTime(iNum, ft);
    }

    return hr;
}

HRESULT CStringList::_AddString(LPCWSTR lpwstr, BOOL fCheckDuplicates, int *piNum)
{
    DWORD dwSize, dwLen;
    int iNum = -1;
    WCHAR wchBufTruncated[MAX_URL_STRING];
    LPCWSTR lpwstrTruncated=lpwstr;

    if (piNum)
    {
        *piNum = -1;
    }

    if (!lpwstr)
    {
        return E_INVALIDARG;
    }

    if (!m_psiIndex)
    {
        if (FAILED(Init()))
        {
            return E_FAIL;
        }
    }

    dwLen = (DWORD) lstrlenW(lpwstr);

    // Explicitly truncate strings to MAX_URL characters. If we don't do this, browseui
    //  autocomplete code truncates it anyway and then we have problems removing
    //  duplicates and deleting these long strings. All IntelliForms code can handle
    //  arbitrary length strings.
    if (dwLen >= ARRAYSIZE(wchBufTruncated))
    {
        StrCpyNW(wchBufTruncated, lpwstr, ARRAYSIZE(wchBufTruncated));
        lpwstrTruncated = wchBufTruncated;
        dwLen = lstrlenW(wchBufTruncated);
    }

    dwSize = (dwLen+1)*sizeof(WCHAR);

    if (fCheckDuplicates && SUCCEEDED(FindString(lpwstrTruncated, (int)dwLen, &iNum, FALSE)))
    {
        if (piNum)
        {
            *piNum = iNum;
        }

        if (!StrCmpW(lpwstrTruncated, m_psiIndex->StringEntry[iNum].pwszString))
        {
            return S_FALSE;             // String is an exact duplicate
        }

        // String is a duplicate but has different case. Replace.
        ASSERT(m_psiIndex->StringEntry[iNum].dwStringLen == dwLen);
        memcpy(m_psiIndex->StringEntry[iNum].pwszString, lpwstrTruncated, dwSize);

        return S_OK;                    // String was different in case
    }

    if (m_psiIndex->dwNumStrings >= m_dwMaxStrings)
    {
        if (m_fAutoScavenge)
        {
            // Remove the oldest string from our list.
            DWORD dwIndex;
            int iOldest=-1;
            FILETIME ftOldest = { 0xFFFFFFFF, 0x7FFFFFFF };
            for (dwIndex=0; dwIndex<m_psiIndex->dwNumStrings; dwIndex++)
            {
                if ((FILETIME_TO_INT64(m_psiIndex->StringEntry[dwIndex].ftLastSubmitted) != 0) &&
                    (1 == CompareFileTime(&ftOldest, &m_psiIndex->StringEntry[dwIndex].ftLastSubmitted)))
                {
                    ftOldest = m_psiIndex->StringEntry[dwIndex].ftLastSubmitted;
                    iOldest = (int)dwIndex;
                }
            }

            if (iOldest != -1)
            {
                DeleteString(iOldest);
            }
            else
            {
                // User must not be setting string times.
                return E_OUTOFMEMORY;
            }
        }
        else
        {
            // Auto-scavenge is disabled.
            return E_OUTOFMEMORY;
        }
    }

    if (FAILED(EnsureBuffer(m_dwBufEnd + dwSize)) ||
        FAILED(EnsureIndex(m_psiIndex->dwNumStrings + 1)))
    {
        return E_OUTOFMEMORY;
    }

    // Our buffers are large enough. Do it.
    if (piNum)
    {
        *piNum = (int) m_psiIndex->dwNumStrings;
    }

    LPWSTR pwszNewString = (LPWSTR)(m_pBuffer + m_dwBufEnd);

    memcpy(pwszNewString, lpwstrTruncated, dwSize);
    m_dwBufEnd += dwSize;

    struct StringIndex::tagStringEntry *pse=&(m_psiIndex->StringEntry[m_psiIndex->dwNumStrings]);
    pse->pwszString = pwszNewString;
    FILETIME_TO_INT64(pse->ftLastSubmitted) = 0;
    pse->dwStringLen = dwLen;

    m_psiIndex->dwNumStrings ++;

    return S_OK;           // We added a new string
}

HRESULT CStringList::EnsureBuffer(DWORD dwSizeNeeded)
{
    if (dwSizeNeeded <= m_dwBufSize)
    {
        return S_OK;        // Already big enough
    }

    if (!m_pBuffer)
    {
        return E_FAIL;
    }

    DWORD dwNewBufSize = m_dwBufSize * 2;

    // Grow buffer.
    if (dwSizeNeeded > dwNewBufSize)
    {
        TraceMsg(TF_IFORMS, "IForms: StringList special growing size (big string)");
        dwNewBufSize = dwSizeNeeded;
    }

    TraceMsg(TF_IFORMS, "IForms: CStringList growing");

    LPBYTE pBuf = (LPBYTE)LocalReAlloc(m_pBuffer, dwNewBufSize, LMEM_MOVEABLE);
    if (!pBuf)
    {
        TraceMsg(TF_IFORMS, "IForms: CStringList: ReAlloc failure");
        // Realloc failure: our old memory is still present
        return E_FAIL;
    }

    m_dwBufSize = dwNewBufSize;

    // Fix index pointers if necessary
    if (m_pBuffer != pBuf)
    {
        DWORD dw;

        struct StringIndex::tagStringEntry *pse;
        for (dw=0, pse=&m_psiIndex->StringEntry[0]; dw<m_psiIndex->dwNumStrings; dw++, pse++)
        {
            pse->pwszString = (LPWSTR)(((LPBYTE)pse->pwszString - m_pBuffer) + pBuf);
        }

        m_pBuffer = pBuf;
    }

    // Successfully realloced to bigger buffer
    return S_OK;
}

// grow psiIndex if needed
HRESULT CStringList::EnsureIndex(DWORD dwNumStringsNeeded)
{
    if (!m_psiIndex)
    {
        return E_FAIL;
    }

    if (INDEX_SIZE(dwNumStringsNeeded) > m_dwIndexSize)
    {
        DWORD dwNewMaxStrings = (m_psiIndex->dwNumStrings) * 2;
        DWORD dwNewIndexSize = INDEX_SIZE(dwNewMaxStrings);

        TraceMsg(TF_IFORMS, "IForms: CStringList growing max strings");

        StringIndex *psiBuf =
            (StringIndex *)LocalReAlloc(m_psiIndex, dwNewIndexSize, LMEM_MOVEABLE);

        if (!psiBuf)
        {
            // Realloc failure: Old memory still present
            TraceMsg(TF_IFORMS, "IForms: CStringList ReAlloc failure");
            return E_OUTOFMEMORY;
        }

        // Success. Don't need to fix any pointers in index (buffer is unchanged)
        m_psiIndex = psiBuf;
        m_dwIndexSize = dwNewIndexSize;
    }

    return S_OK;
}

// This dlg proc is used for password save, change, delete dialogs
INT_PTR AutoSuggestDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        CenterWindow(hDlg, GetParent(hDlg));

        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR) lParam);

        if (lParam == IDD_AUTOSUGGEST_SAVEPASSWORD)
        {
            // For "Save" password we default to no. For "Change" and "Delete" we default to yes.
            SetFocus(GetDlgItem(hDlg, IDNO));
            return FALSE;
        }
        return TRUE;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
            case IDCANCEL:          // close box
            case IDYES:             // yes button
            case IDNO:              // no button
                if (IDD_AUTOSUGGEST_SAVEPASSWORD == GetWindowLongPtr(hDlg, DWLP_USER))
                {
                    // Check the "don't ask me again" checkbox for the save password dlg
                    if (IsDlgButtonChecked(hDlg, IDC_AUTOSUGGEST_NEVER))
                    {
                        SHSetValue(HKEY_CURRENT_USER, c_szRegKeySMIEM, c_szRegValAskPasswords,
                                REG_SZ, c_szNo, sizeof(c_szNo));
                    }
                }

                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
        }
        break;

#ifdef CHECKBOX_HELP
    case WM_HELP:
        // Only process WM_HELP for save password dlg
        if (IDD_AUTOSUGGEST_SAVEPASSWORD == GetWindowLong(hDlg, DWL_USER))
        {
            SHWinHelpOnDemandWrap((HWND) ((LPHELPINFO) lParam)->hItemHandle, c_szHelpFile,
                HELP_WM_HELP, (DWORD_PTR)(LPTSTR) c_aIFormsHelpIds);
        }
        break;

    case WM_CONTEXTMENU:      // right mouse click
        // Only process WM_HELP for save password dlg
        if (IDD_AUTOSUGGEST_SAVEPASSWORD == GetWindowLong(hDlg, DWL_USER))
        {
            SHWinHelpOnDemandWrap((HWND) wParam, c_szHelpFile, HELP_CONTEXTMENU,
                (DWORD_PTR)(LPTSTR) c_aIFormsHelpIds);
        }
        break;
#endif
    }

    return FALSE;
}


//================================================================================

INT_PTR CALLBACK AskUserDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        CenterWindow(hDlg, GetParent(hDlg));
        Animate_OpenEx(GetDlgItem(hDlg, IDD_ANIMATE), HINST_THISDLL, MAKEINTRESOURCE(IDA_AUTOSUGGEST));
        return TRUE;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_AUTOSUGGEST_HELP:
                    SHHtmlHelpOnDemandWrap(GetParent(hDlg), TEXT("iexplore.chm > iedefault"),
                        HH_DISPLAY_TOPIC, (DWORD_PTR) TEXT("autocomp.htm"), ML_CROSSCODEPAGE);
                    break;

                case IDYES:
                case IDNO:
                {
                    LPCTSTR pszData;
                    DWORD  cbData;
                    DWORD  dwData=0;

                    if (LOWORD(wParam) == IDYES)
                    {
                        pszData = c_szYes;
                        cbData = sizeof(c_szYes);
                    }
                    else
                    {
                        pszData = c_szNo;
                        cbData = sizeof(c_szNo);
                    }

                    // Write the enabled state into our CPL regkey
                    SHSetValue(HKEY_CURRENT_USER, c_szRegKeySMIEM, c_szRegValUseFormSuggest,
                        REG_SZ, pszData, cbData);

                    // Flag it as "asked user" so we don't ask them again
                    SHSetValue(HKEY_CURRENT_USER, c_szRegKeyIntelliForms, c_szRegValAskUser,
                        REG_DWORD, &dwData, sizeof(dwData));
                }

                // Fall through
                case IDCANCEL:
                {
                    EndDialog(hDlg, LOWORD(wParam));
                }
                break;
            }
        }
        return TRUE;

    case WM_DESTROY:
        break;
    }

    return FALSE;
}

