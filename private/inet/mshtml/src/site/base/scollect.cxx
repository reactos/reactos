//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       scollect.cxx
//
//  Contents:   Implementation of CScriptCollection class
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_SCRIPT_HXX_
#define X_SCRIPT_HXX_
#include "script.hxx"
#endif

#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"
#endif

#ifndef X_SAFETY_HXX_
#define X_SAFETY_HXX_
#include "safety.hxx"
#endif

#ifndef X_SHOLDER_HXX_
#define X_SHOLDER_HXX_
#include "sholder.hxx"
#endif

#ifndef X_ESCRIPT_HXX_
#define X_ESCRIPT_HXX_
#include "escript.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"
#endif

#ifndef X_DEBUGGER_HXX_
#define X_DEBUGGER_HXX_
#include "debugger.hxx"
#endif

#define MAX_PROGID_LENGTH 39

MtDefine(CScriptCollection, ObjectModel, "CScriptCollection")
MtDefine(CScriptCollection_aryHolder_pv, CScriptCollection, "CScriptCollection::_aryHolder::_pv")
MtDefine(CScriptCollection_aryNamedItems_pv, CScriptCollection, "CScriptCollection::_aryNamedItems::_pv")
MtDefine(CNamedItemsTable_CItemsArray, CScriptCollection, "CNamedItemsTable::CItemsArray")
MtDefine(CScriptCollection_CScriptMethodsArray, CScriptCollection, "CScriptCollection::CScriptMethodsArray")

const GUID CATID_ActiveScriptParse = { 0xf0b7a1a2, 0x9847, 0x11cf, { 0x8f, 0x20, 0x0, 0x80, 0x5f, 0x2c, 0xd0, 0x64 } };

const CLSID CLSID_VBScript = {0xB54F3741, 0x5B07, 0x11CF, 0xA4, 0xB0, 0x00, 0xAA, 0x00, 0x4A, 0x55, 0xE8};
const CLSID CLSID_JScript  = {0xF414C260, 0x6AC0, 0x11CF, 0xB6, 0xD1, 0x00, 0xAA, 0x00, 0xBB, 0xBB, 0x58};

#ifndef NO_SCRIPT_DEBUGGER
interface IProcessDebugManager *g_pPDM;
interface IDebugApplication *g_pDebugApp;


static BOOL g_fScriptDebuggerInitFailed;
static DWORD g_dwAppCookie;

HRESULT InitScriptDebugging();
void DeinitScriptDebugging();
#endif

DeclareTag(tagScriptCollection, "Script Collection", "Script collection methods")

//---------------------------------------------------------------------------
//
//  Function:   CLSIDFromLanguage
//
//  Synopsis:   Given name of script language, find clsid of script engine.
//
//---------------------------------------------------------------------------

HRESULT
CLSIDFromLanguage(TCHAR *pchLanguage, REFGUID catid, CLSID *pclsid)
{
    HKEY    hkey;
    HRESULT hr;
    TCHAR   achBuf[256];
    long    lResult;

    if ( _tcslen(pchLanguage)> MAX_PROGID_LENGTH)
    {
        // Progid can ONLY have no more than 39 characters.
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(CLSIDFromProgID(pchLanguage, pclsid));
    if (hr)
        goto Cleanup;
#if !defined(WIN16) && !defined(WINCE) && !defined(_MAC)
    // Check to see that class supports required category.

    hr = THR(Format(0,
            achBuf,
            ARRAY_SIZE(achBuf),
            _T("CLSID\\<0g>\\Implemented Categories\\<1g>"),
            pclsid,
            &catid));
    if (hr)
        goto Cleanup;

    lResult = RegOpenKey(HKEY_CLASSES_ROOT, achBuf, &hkey);
    if (lResult == ERROR_SUCCESS)
    {
        RegCloseKey(hkey);
    }
    else
    {
        hr = REGDB_E_CLASSNOTREG;
        goto Cleanup;
    }
#endif // WINCE

Cleanup:
    RRETURN(hr);
}

//+--------------------------------------------------------------------------
//
//  Member:     CScriptCollection::CDebugDocumentStack constructor
//
//---------------------------------------------------------------------------

CScriptCollection::CDebugDocumentStack::CDebugDocumentStack(CScriptCollection * pScriptCollection)
{
    _pScriptCollection = pScriptCollection;
    _pDebugDocumentPrevious = pScriptCollection->_pCurrentDebugDocument;
}

//+--------------------------------------------------------------------------
//
//  Member:     CScriptCollection::CDebugDocumentStack destructor
//
//---------------------------------------------------------------------------

CScriptCollection::CDebugDocumentStack::~CDebugDocumentStack()
{
    if (_pScriptCollection->_pCurrentDebugDocument)
        _pScriptCollection->_pCurrentDebugDocument->UpdateDocumentSize();

    _pScriptCollection->_pCurrentDebugDocument = _pDebugDocumentPrevious;
}

//+--------------------------------------------------------------------------
//
//  Function:   CScriptCollection::CScriptCollection
//
//---------------------------------------------------------------------------

CScriptCollection::CScriptCollection()
    : _aryHolder(Mt(CScriptCollection_aryHolder_pv))
{
    _ulRefs = 1;
    _ulAllRefs = 1;
    Assert (!_fInEnginesGetDispID);
}

//---------------------------------------------------------------------------
//
//  Function:   CScriptCollection::Init
//
//---------------------------------------------------------------------------

HRESULT
CScriptCollection::Init(CDoc * pDoc)
{
    HRESULT hr = S_OK;

    TraceTag((tagScriptCollection, "Init"));

    MemSetName((this, "ScrptColl pDoc=%08x %ls", pDoc, pDoc->_cstrUrl ? (TCHAR *)pDoc->_cstrUrl : _T("")));

    Assert (pDoc && !_pDoc);

    _pDoc = pDoc;
    _ss = SCRIPTSTATE_STARTED;
    _pDoc->SubAddRef();
    
#ifndef NO_SCRIPT_DEBUGGER
    // Has the user has chosen to disable script debugging?
    if (!_pDoc->_pOptionSettings->fDisableScriptDebugger)
    {
        // If this fails we just won't have smart host debugging
        IGNORE_HR(InitScriptDebugging());
    }
#endif

    RRETURN (hr);
}

//---------------------------------------------------------------------------
//
//  Function:   CScriptCollection::~CScriptCollection
//
//---------------------------------------------------------------------------

CScriptCollection::~CScriptCollection()
{
    int             c;

    _NamedItemsTable.FreeAll();

    for (c = _aryHolder.Size(); --c >= 0; )
    {
        IGNORE_HR(_aryHolder[c]->Close());
        _aryHolder[c]->Release();
    }
    _aryHolder.DeleteAll();

    _pDoc->SubRelease();
}


//---------------------------------------------------------------------------
//
//  Function:   CScriptCollection::Release
//
//  Synposis:   Per IUnknown
//
//---------------------------------------------------------------------------

ULONG
CScriptCollection::Release()
{
    ULONG ulRefs;
    if (--_ulRefs == 0)
    {
        _ulRefs = ULREF_IN_DESTRUCTOR;
        Deinit();
        _ulRefs = 0;
    }
    ulRefs = _ulRefs;
    SubRelease();
    return ulRefs;
}


//---------------------------------------------------------------------------
//
//  Method:     CScriptCollection::SubRelease
//
//---------------------------------------------------------------------------

ULONG
CScriptCollection::SubRelease()
{
#ifndef NO_SCRIPT_DEBUGGER
    if (_pDoc->_dwTID != GetCurrentThreadId())
    {
        Assert(0 && "Debugger called across thread boundary (not an MSHTML bug)");
        return TRUE;
    }
#endif //NO_SCRIPT_DEBUGGER

    if (--_ulAllRefs == 0)
    {
        _ulRefs = ULREF_IN_DESTRUCTOR;
        _ulAllRefs = ULREF_IN_DESTRUCTOR;
        delete this;
        return 0;
    }
    return _ulAllRefs;
}


//---------------------------------------------------------------------------
//
//  Function:   CScriptCollection::Deinit
//
//---------------------------------------------------------------------------

void
CScriptCollection::Deinit()
{
    TraceTag((tagScriptCollection, "Deinit"));

    //
    // Disconnect any VBScript event sinks.  Need to do this to ensure that
    // other events are not continually fired if the document is reused.
    //
    SetState(SCRIPTSTATE_DISCONNECTED);
}

//---------------------------------------------------------------------------
//
//  Function:   CScriptCollection::AddNamedItem
//
//  Synopsis:   Let script engine know about any named items that were
//              added.
//
//  Notes:      Assumed to be added with SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE
//
//---------------------------------------------------------------------------

HRESULT
CScriptCollection::AddNamedItem(CElement *pElement)
{
    TraceTag((tagScriptCollection, "AddNamedItem"));

    HRESULT             hr = S_OK;
    CStr                cstr;
    LPTSTR              pch;
    BOOL                fDidCreate;
    CDoc *              pDoc;
    CCollectionCache *  pCollectionCache;

    Assert(pElement->Tag() == ETAG_FORM);

    pch = (LPTSTR) pElement->GetIdentifier();
    if (!pch || !*pch)
    {
        hr = THR(pElement->GetUniqueIdentifier(&cstr, TRUE, &fDidCreate));
        if (hr)
            goto Cleanup;
        pch = cstr;
        pDoc = Doc();

        pCollectionCache = pDoc->PrimaryMarkup()->CollectionCache();
        if ( fDidCreate && pCollectionCache)
            pCollectionCache->InvalidateItem(CMarkup::WINDOW_COLLECTION);
    }

    hr = THR(AddNamedItem(
        pch, (IUnknown*)(IPrivateUnknown*)pElement, /*fExcludeParseProcedureEngines = */ TRUE));

Cleanup:

    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Function:   CScriptCollection::AddNamedItem
//
//---------------------------------------------------------------------------

HRESULT
CScriptCollection::AddNamedItem(LPTSTR pchName, IUnknown * pUnkItem, BOOL /*fExcludeParseProcedureEngines*/)
{
    HRESULT             hr;
    int                 c;
    CScriptHolder **    ppHolder;

    hr = THR(_NamedItemsTable.AddItem(pchName, pUnkItem));
    if (hr)
        goto Cleanup;

    for (c = _aryHolder.Size(), ppHolder = _aryHolder; c > 0; c--, ppHolder++)
    {

        // do not pass in the ISVISIBLE flag.  This will break form access by name
        // from the window collection.
        IGNORE_HR((*ppHolder)->_pScript->AddNamedItem(
                pchName,
                SCRIPTITEM_ISSOURCE));
    }

Cleanup:
    RRETURN (hr);
}

//---------------------------------------------------------------------------
//
//  Function:   CScriptCollection::SetState
//
//---------------------------------------------------------------------------

HRESULT
CScriptCollection::SetState(SCRIPTSTATE ss)
{
    TraceTag((tagScriptCollection, "SetState"));
    HRESULT         hr = S_OK;
    HRESULT         hr2;
    int             c;

    CDoc::CLock Lock(_pDoc);

    for (c = _aryHolder.Size(); --c >= 0; )
    {
        hr2 = THR(_aryHolder[c]->SetScriptState(ss));
        if (hr2)
            hr = hr2;
    }

    _ss = ss;

    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Function:   CScriptCollection::AddHolderForObject
//
//---------------------------------------------------------------------------

HRESULT
CScriptCollection::AddHolderForObject(CBase *pBase, IActiveScript *pScript, CLSID *pClsid)
{
    TraceTag((tagScriptCollection, "AddHolderForObject"));

    HRESULT                 hr=S_OK;
    CScriptHolder *         pHolder;
    IActiveScriptParse *    pScriptParse = NULL;
    BOOL                    fRunScripts;
    IUnknown *              pUnk;
    
    hr = THR(Doc()->ProcessURLAction(URLACTION_SCRIPT_RUN, &fRunScripts));
    if (hr || !fRunScripts)
        goto Cleanup;
        
    // Ok for this to fail
    THR_NOTRACE(pScript->QueryInterface(
        IID_IActiveScriptParse,
        (void **)&pScriptParse));

    if (pScriptParse)
    {
        pUnk = pScriptParse;
    }
    else
    {
        pUnk = pScript;
    }
    
    if (!IsSafeToRunScripts(pClsid, pUnk))
        goto Cleanup;
        
    {
        CDoc::CLock Lock(_pDoc);

        pHolder = new CScriptHolder(this);
        if (!pHolder)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(pHolder->Init(pBase, pScript, pScriptParse, pClsid));
        if (hr)
            goto Error;

        hr = THR(_aryHolder.Append(pHolder));
        if (hr)
            goto Error;
    }

Cleanup:
    ReleaseInterface(pScriptParse);
    RRETURN(hr);

Error:
    delete pHolder;
    pHolder = NULL;
    goto Cleanup;
}


//---------------------------------------------------------------------------
//
//  Function:   CScriptCollection::RemoveHolderForObject
//
//---------------------------------------------------------------------------

HRESULT
CScriptCollection::RemoveHolderForObject(CBase *pBase)
{
    TraceTag((tagScriptCollection, "RemoveHolderForObject"));

    int c;

    CDoc::CLock Lock(_pDoc);

    for (c = _aryHolder.Size(); --c >= 0; )
    {
        if (_aryHolder[c]->_pBase == pBase)
        {
            _aryHolder[c]->Close();
            _aryHolder[c]->Release();
        }
    }

    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Method:     CScriptCollection::IsSafeToRunScripts
//
//  Synopsis:   Decide whether script engine is safe to instantiate.
//
//---------------------------------------------------------------------------

BOOL 
CScriptCollection::IsSafeToRunScripts(CLSID *pClsid, IUnknown *pUnk)
{
    BOOL    fSafe = FALSE;
    HRESULT hr;
    
    // We need a clsid and an interface pointer!
    if (!pClsid || !pUnk)
        goto Cleanup;

    hr = THR(Doc()->ProcessURLAction(
            URLACTION_SCRIPT_OVERRIDE_SAFETY,
            &fSafe));
    if (hr || fSafe)
        goto Cleanup;

	// WINCEREVIEW - ignore script safety checking !!!!!!!!!!!!!!!!
#ifndef WINCE
    fSafe = ::IsSafeTo(
                SAFETY_SCRIPTENGINE, 
                IID_IActiveScriptParse, 
                *pClsid, 
                pUnk, 
                Doc());
    if (fSafe)
        goto Cleanup;

    fSafe = ::IsSafeTo(
                SAFETY_SCRIPTENGINE, 
                IID_IActiveScript, 
                *pClsid, 
                pUnk, 
                Doc());
#else // !WINCE
	// blindly say that this is safe.
	fSafe = TRUE;
#endif // !WINCE
    
Cleanup:
    return fSafe;
}


//---------------------------------------------------------------------------
//
//  Function:   CScriptCollection::GetHolderForLanguage
//
//---------------------------------------------------------------------------

HRESULT
CScriptCollection::GetHolderForLanguage(
    TCHAR *             pchLanguage,
    CMarkup *           pMarkup,
    TCHAR *             pchType,
    TCHAR *             pchCode,
    CScriptHolder **    ppHolder,
    TCHAR **            ppchCleanCode)
{
    HRESULT     hr;
    TCHAR *     pchColon;

    if (pchCode)
    {
        // check if this is a case like "javascript: alert('hello')"

        pchColon = _tcschr (pchCode, _T(':'));

        if (pchColon)
        {
            // we assume here that we can modify string at pchColon temporarily
            *pchColon = 0;

            // We shouldn't get a "fooLanguage:" w/ a type.
            Assert(pchType == NULL);
            hr = THR_NOTRACE(GetHolderForLanguageHelper(pchCode, pMarkup, pchType, ppHolder));

            *pchColon = _T(':');

            if (S_OK == hr)                         // if successful
            {
                if (ppchCleanCode)
                {
                    *ppchCleanCode = pchColon + 1;  // adjust ppchCleanCode so to skip prefix 'fooLanguage:'
                }
                goto Cleanup;                       // and nothing more to do
            }
        }
    }

    if (ppchCleanCode)
    {
        *ppchCleanCode = pchCode;
    }

    hr = THR_NOTRACE(GetHolderForLanguageHelper(pchLanguage, pMarkup, pchType, ppHolder));

Cleanup:

    RRETURN (hr);
}


//---------------------------------------------------------------------------
//
//  Function:   CScriptCollection::GetHolderForLanguageHelper
//
//---------------------------------------------------------------------------

HRESULT
CScriptCollection::GetHolderForLanguageHelper(
    TCHAR *             pchLanguage,
    CMarkup *           pMarkup,
    TCHAR *             pchType,
    CScriptHolder **    ppHolder)
{
    HRESULT                 hr = S_OK;
    int                     idx = -1;
    int                     cnt;
    CLSID                   clsid;
    IActiveScript *         pScript = NULL;
    IActiveScriptParse *    pScriptParse = NULL;
    CScriptHolder *         pHolderCreated = NULL;
    CMarkupScriptContext *  pMarkupScriptContext = NULL;

    if (pMarkup)
    {
        hr = THR(pMarkup->EnsureScriptContext(&pMarkupScriptContext));
        if (hr)
            goto Cleanup;
    }

    //
    // The type attribute should be of the form:
    // text/script-type.  If it is not of this 
    // type, then it's invalid.
    // Additionally, the type attribute takes precedence
    // over the language attribute, so if it's present,
    // we use it instead.
    //
    if(pchType && *pchType)
    {
        // BUGBUG (t-johnh): Maybe we should do a check on the
        // type attribute as a MIME type instead of checking
        // for text/fooLanguage

        // Make sure we've got text/fooLanguage
        if(!_tcsnipre(_T("text"), 4, pchType, 4))
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        // Skip to what's past the "text/"
        pchLanguage = pchType + 5;
    }
    else if (!pchLanguage || !pchLanguage[0]) // if no language nor type specified 
    {
        if (pMarkup)
        {
            if (pMarkupScriptContext &&                                 // if default holder is indicated
                pMarkupScriptContext->_idxDefaultScriptHolder != -1)    // in the markup context
            {
                Assert (0 <= pMarkupScriptContext->_idxDefaultScriptHolder &&
                             pMarkupScriptContext->_idxDefaultScriptHolder < _aryHolder.Size());

                idx = pMarkupScriptContext->_idxDefaultScriptHolder;

                Assert (_aryHolder[idx]->_pScriptParse);

                goto Cleanup;   // done
            }
        }
        else
        {
            // pick the first script holder that supports scripting
            for (idx = 0, cnt = _aryHolder.Size(); idx < cnt; idx++)
            {
                if (_aryHolder[idx]->_pScriptParse)
                {
                    goto Cleanup;   // done
                }
            }
        }

        // if not found: there were no script holders (for scripting) created so use JavaScript as default
        pchLanguage = _T("JavaScript");
    }
    else if (0 == StrCmpIC(pchLanguage, _T("LiveScript")))
    {
        // LiveScript is the old name for JavaScript, so convert if necessary
        pchLanguage = _T("JavaScript");
    }

    //
    // Get the clsid for this language.
    //

    // Perf optimization for Win98 and Win95 to not hit the registry with
    // CLSIDFromPROGID.

    // BUGBUG: ***TLL*** better solution is to remember
    // the language name and clsid in the script holder as a cache and check
    // in the holder first.
    if ((*pchLanguage == _T('j') || *pchLanguage == _T('J'))    &&
        (0 == StrCmpIC(pchLanguage, _T("jscript"))          ||
         0 == StrCmpIC(pchLanguage, _T("javascript"))))
    {
        clsid = CLSID_JScript;
    }
    else if ((*pchLanguage == _T('v') || *pchLanguage == _T('V'))    &&
             (0 == StrCmpIC(pchLanguage, _T("vbs"))         ||
              0 == StrCmpIC(pchLanguage, _T("vbscript"))))
    {
        clsid = CLSID_VBScript;
    }
    else
    {
        hr = THR(CLSIDFromLanguage(pchLanguage, CATID_ActiveScriptParse, &clsid));
        if (hr)
        {
            goto Cleanup;
        }
    }

    //
    // Do we already have one on hand?
    //

    for (idx = 0, cnt = _aryHolder.Size(); idx < cnt; idx++)
    {
        if (_aryHolder[idx]->_clsid == clsid)
        {
            goto Cleanup;   // done - found
        }
    }

    // Create one.

    if (IsEqualGUID(clsid, CLSID_VBScript))
    {
        uCLSSPEC classpec;

        classpec.tyspec = TYSPEC_CLSID;
        classpec.tagged_union.clsid = clsid;

        hr = THR(FaultInIEFeatureHelper(_pDoc->GetHWND(), &classpec, NULL, 0));
        // BUGBUG (lmollico): should Assert(hr != S_FALSE) before we ship
        if (FAILED(hr))
        {
            hr = REGDB_E_CLASSNOTREG;
            goto Cleanup;
        }
    }

    hr = THR(CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IActiveScript, (void **)&pScript));
    if (hr)
        goto Cleanup;

    hr = THR(pScript->QueryInterface(IID_IActiveScriptParse, (void **)&pScriptParse));
    if (hr)
        goto Cleanup;

    if (!IsSafeToRunScripts(&clsid, pScript))
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    *ppHolder = pHolderCreated = new CScriptHolder(this);
    if (!pHolderCreated)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR((*ppHolder)->Init(NULL, pScript, pScriptParse, &clsid));
    if (hr)
        goto Cleanup;

    hr = THR(_aryHolder.Append(*ppHolder));
    if (hr)
        goto Cleanup;

    Assert (idx == _aryHolder.Size() - 1);

Cleanup:

    if (S_OK == hr)
    {
        Assert (0 <= idx && idx < _aryHolder.Size());

        (*ppHolder) = _aryHolder[idx];

        if (pMarkupScriptContext &&                                 // if default holder is not yet set in the
            pMarkupScriptContext->_idxDefaultScriptHolder == -1)    // markup context
        {
            pMarkupScriptContext->_idxDefaultScriptHolder = idx;
        }
    }
    else // if (hr)
    {
        *ppHolder = NULL;
        delete pHolderCreated;
    }

    ReleaseInterface(pScript);
    ReleaseInterface(pScriptParse);

    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Function:   CScriptCollection::AddScriptlet
//
//---------------------------------------------------------------------------

HRESULT
CScriptCollection::AddScriptlet(
    LPTSTR      pchLanguage,
    CMarkup *   pMarkup,
    LPTSTR      pchType,
    LPTSTR      pchCode,
    LPTSTR      pchItemName,
    LPTSTR      pchSubItemName,
    LPTSTR      pchEventName,
    LPTSTR      pchDelimiter,
    ULONG       ulOffset,
    ULONG       ulStartingLine,
    CBase *     pSourceObject,
    DWORD       dwFlags,
    BSTR *      pbstrName)
{
    TraceTag((tagScriptCollection, "AddScriplet"));

    HRESULT                 hr = S_OK;
    CScriptHolder *         pHolder;
    CExcepInfo              ExcepInfo;
    DWORD                   dwSourceContextCookie = NO_SOURCE_CONTEXT;
    TCHAR *                 pchCleanCode = NULL;
    CDebugDocumentStack     debugDocumentStack(this);
    CDoc::CLock             Lock(_pDoc);

    hr = THR(GetHolderForLanguage(pchLanguage, pMarkup, pchType, pchCode, &pHolder, &pchCleanCode));
    if (hr)
        goto Cleanup;

    if (!pchCleanCode)
    {
        Assert (!hr);
        goto Cleanup;
    }

    Assert(pHolder->_pScriptParse);
    if(!pHolder->_pScriptParse)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    // If the engine supports IParseProcedure then we don't need to do AddScriptlet
    // instead we'll use function pointers.
    if (pchCleanCode && !pHolder->_pParseProcedure)
    {

        hr = THR(CreateSourceContextCookie(
            pHolder->_pScript, pchCode, ulOffset, /* fScriptlet = */ TRUE, pSourceObject, dwFlags, &dwSourceContextCookie));
        if (hr)
            goto Cleanup;

        hr = THR(pHolder->_pScriptParse->AddScriptlet (
                         NULL,
                         pchCleanCode,
                         pchItemName,
                         pchSubItemName,
                         pchEventName,
                         pchDelimiter,
                         dwSourceContextCookie,
                         ulStartingLine,
                         dwFlags,
                         pbstrName,
                         &ExcepInfo));
    }

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Function:   CScriptCollection::ParseScriptText
//
//---------------------------------------------------------------------------

HRESULT
CScriptCollection::ParseScriptText(
    LPTSTR          pchLanguage,
    CMarkup *       pMarkup,
    LPTSTR          pchType,
    LPTSTR          pchCode,
    LPTSTR          pchItemName,
    LPTSTR          pchDelimiter,
    ULONG           ulOffset,
    ULONG           ulStartingLine,
    CBase *         pSourceObject,
    DWORD           dwFlags,
    VARIANT  *      pvarResult,
    EXCEPINFO *     pexcepinfo)
{
    TraceTag((tagScriptCollection, "ParseScriptText"));

    HRESULT                 hr = S_OK;
    CScriptHolder *         pHolder;
    BOOL                    fRunScripts;
    DWORD                   dwSourceContextCookie;
    CDebugDocumentStack     debugDocumentStack(this);

    Doc()->PeerDequeueTasks(0); // this call is important to prevent dequeueing while in the middle of
                                // executing inline escripts. See scenario listed past the end of this method

    hr = THR(Doc()->ProcessURLAction(URLACTION_SCRIPT_RUN, &fRunScripts));
    if (hr || !fRunScripts)
        goto Cleanup;

    {
        CDoc::CLock     Lock(_pDoc);

        hr = THR(GetHolderForLanguage(pchLanguage, pMarkup, pchType, NULL, &pHolder));
        if (hr)
            goto Cleanup;

        Assert(pHolder->_pScriptParse);
        if(!pHolder->_pScriptParse)
        {
            hr = E_UNEXPECTED;
            goto Cleanup;
        }

        hr = THR(CreateSourceContextCookie(
            pHolder->_pScript, pchCode, ulOffset, /* fScriptlet = */ FALSE,
            pSourceObject, dwFlags, &dwSourceContextCookie));
        if (hr)
            goto Cleanup;

        hr = THR(pHolder->_pScriptParse->ParseScriptText(
                         STRVAL(pchCode),
                         pchItemName,
                         NULL,
                         pchDelimiter,
                         dwSourceContextCookie,
                         ulStartingLine,
                         dwFlags,
                         pvarResult,
                         pexcepinfo));
    }


Cleanup:

    RRETURN(hr);
}

//
// (alexz)
//
// Scenario why dequeuing is necessary before doing ParseScriptText:
//
// <PROPERTY name = foo put = put_foo />
// <SCRIPT>
//    alert (0);
//    function put_foo()
//    {
//      alert (1);
//    }
// </SCRIPT>
//
//      - HTC PROPERTY element is in the queue waiting for PROPERTY behavior to be attached
//      - inline script runs, and executes "alert(0)"
//      - HTC DD is being asked for name "alert"
//      - HTC DD asks element for name "alert"
//      - element makes call to dequeue the queue
//      - HTC PROPERTY is constructed
//      - it attempts to load property from element (EnsureHtmlLoad)
//      - it successfully finds property to load, finds putter to invoke, and calls put_foo
//      - now we are trying to execute put_foo, before even inline script completed executing - which is bad
//
//

//---------------------------------------------------------------------------
//
//  Member:     CScriptCollection::CreateSourceContextCookie
//
//---------------------------------------------------------------------------

HRESULT
CScriptCollection::CreateSourceContextCookie(
    IActiveScript *     pActiveScript, 
    LPTSTR              pchSource,
    ULONG               ulOffset, 
    BOOL                fScriptlet, 
    CBase *             pSourceObject,
    DWORD               dwFlags,
    DWORD *             pdwSourceContextCookie)
{
    HRESULT                 hr = S_OK;
    CScriptCookieTable *    pScriptCookieTable;

    *pdwSourceContextCookie = NO_SOURCE_CONTEXT;

    if (!pSourceObject)
        goto Cleanup;

    hr = THR(_pDoc->EnsureScriptCookieTable(&pScriptCookieTable));
    if (hr)
        goto Cleanup;

    //
    // if there is script debugger installed and we want to use it
    //

#ifndef NO_SCRIPT_DEBUGGER

    hr = THR(GetScriptDebugDocument(pSourceObject, &_pCurrentDebugDocument));
    if (hr)
        goto Cleanup;

    if ((dwFlags & SCRIPTPROC_HOSTMANAGESSOURCE) && _pCurrentDebugDocument)
    {
        ULONG ulCodeLen = _tcslen(STRVAL(pchSource));

        hr = THR(_pCurrentDebugDocument->DefineScriptBlock(
            pActiveScript, ulOffset, ulCodeLen, fScriptlet, pdwSourceContextCookie));
        if (hr)
        {
            hr = S_OK;      // return S_OK and NO_SOURCE_CONTEXT
            goto Cleanup;
        }

        hr = THR(_pCurrentDebugDocument->RequestDocumentSize(ulOffset + ulCodeLen));
        if (hr)
            goto Cleanup;

        hr = THR(pScriptCookieTable->MapCookieToSourceObject(*pdwSourceContextCookie, pSourceObject));

        goto Cleanup;// done
    }
#endif // NO_SCRIPT_DEBUGGER

    //
    // otherwise assign the cookie ourselves
    //

    hr = THR(pScriptCookieTable->CreateCookieForSourceObject(pdwSourceContextCookie, pSourceObject));

Cleanup:
    RRETURN (hr);
}

#ifndef NO_SCRIPT_DEBUGGER
//+---------------------------------------------------------------------------
//
//  Member:     CScriptCollection::ViewSourceInDebugger
//
//  Synopsis:   Launches the script debugger at a particular line
//
//----------------------------------------------------------------------------

HRESULT
CScriptCollection::ViewSourceInDebugger (const ULONG ulLine, const ULONG ulOffsetInLine)
{
    HRESULT                 hr = S_OK;
    CScriptDebugDocument *  pDebugDocument = NULL;
    CMarkupScriptContext *  pMarkupScriptContext = _pDoc->PrimaryMarkup()->ScriptContext();

    pDebugDocument = pMarkupScriptContext ? pMarkupScriptContext->_pScriptDebugDocument : NULL;

    if (pDebugDocument)
    {
        hr = THR(pDebugDocument->ViewSourceInDebugger(ulLine, ulOffsetInLine));
    }

    RRETURN(hr);
}
#endif // !NO_SCRIPT_DEBUGGER

//+---------------------------------------------------------------------------
//
//  Member:     CScriptCollection::ConstructCode
//
//----------------------------------------------------------------------------

HRESULT
CScriptCollection::ConstructCode(
    TCHAR *      pchScope,
    TCHAR *      pchCode,
    TCHAR *      pchFormalParams,
    TCHAR *      pchLanguage,
    CMarkup *    pMarkup,
    TCHAR *      pchType,
    ULONG        ulOffset,
    ULONG        ulStartingLine,
    CBase *      pSourceObject,
    DWORD        dwFlags,
    IDispatch ** ppDispCode,
    BOOL         fSingleLine)
{
    TraceTag((tagScriptCollection, "ConstructCode"));

    HRESULT                 hr = S_OK;
    CScriptHolder *         pHolder;
    DWORD                   dwSourceContextCookie;
    TCHAR *                 pchCleanSource;
    CDebugDocumentStack     debugDocumentStack(this);

    dwFlags |= SCRIPTPROC_IMPLICIT_THIS | SCRIPTPROC_IMPLICIT_PARENTS | SCRIPTPROC_HOSTMANAGESSOURCE;

    if (!ppDispCode)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    ClearInterface(ppDispCode);

    if (!pchCode)
        goto Cleanup;

    if (fSingleLine)
    {
        hr = THR (GetHolderForLanguage (pchLanguage, pMarkup, pchType, pchCode, &pHolder, &pchCleanSource));
        if (hr)
            goto Cleanup;
    }
    else
    {
        pchCleanSource = pchCode;

        if (!pchCleanSource || !*pchCleanSource)
            goto Cleanup;

        hr = THR (GetHolderForLanguage (pchLanguage, pMarkup, pchType, NULL, &pHolder));
        if (hr)
            goto Cleanup;
    }

    Assert (pchCleanSource);

    if (!pHolder->_pParseProcedure)
    {
        hr = E_NOTIMPL;
        goto Cleanup;
    }

    hr = THR(CreateSourceContextCookie(
        pHolder->_pScript, pchCleanSource, ulOffset, /* fScriptlet = */ TRUE,
        pSourceObject, dwFlags, &dwSourceContextCookie));
    if (hr)
        goto Cleanup;

    hr = THR(pHolder->_pParseProcedure->ParseProcedureText(
        pchCleanSource,
        pchFormalParams,
        _T("\0"),                                   // procedure name
        pchScope,                                   // item name
        NULL,                                       // pUnkContext
        fSingleLine ? _T("\"") : _T("</SCRIPT>"),   // delimiter
        dwSourceContextCookie,                      // source context cookie
        ulStartingLine,                             // starting line number
        dwFlags,
        ppDispCode));

Cleanup:
    RRETURN (hr);
}


#ifndef NO_SCRIPT_DEBUGGER

//+---------------------------------------------------------------------------
//
//  Function:     InitScriptDebugging
//
//----------------------------------------------------------------------------

// don't hold the global lock because InitScriptDebugging makes RPC calls (bug 26308)
static CCriticalSection g_csInitScriptDebugger;

HRESULT 
InitScriptDebugging()
{
   TraceTag((tagScriptCollection, "InitScriptDebugging"));
    if (g_fScriptDebuggerInitFailed || g_pDebugApp)
        return S_OK;

    HRESULT hr = S_OK;
    HKEY hkeyProcessDebugManager = NULL;
    LPOLESTR pszClsid = NULL;
    TCHAR *pchAppName = NULL;
    LONG lResult;
    TCHAR pchKeyName[MAX_PROGID_LENGTH+8];   // only needs to be MAX_PROGID_LENGTH+6, but let's be paranoid

    g_csInitScriptDebugger.Enter();

    // Need to check again after locking the globals.
    if (g_fScriptDebuggerInitFailed || g_pDebugApp)
        goto Cleanup;

    //
    // Check to see if the ProcessDebugManager is registered before
    // trying to CoCreate it, as CoCreating can be expensive.
    //
    hr = THR(StringFromCLSID(CLSID_ProcessDebugManager, &pszClsid));
    if (hr)
        goto Cleanup;

    hr = THR(Format(0, &pchKeyName, MAX_PROGID_LENGTH+8, _T("CLSID\\<0s>"), pszClsid));
    if (hr)
        goto Cleanup;

    lResult = RegOpenKeyEx(HKEY_CLASSES_ROOT, pchKeyName, 0, KEY_READ, &hkeyProcessDebugManager);
    if (lResult != ERROR_SUCCESS)
    {
        hr = GetLastWin32Error();
        goto Cleanup;
    }

    hr = THR_NOTRACE(CoCreateInstance(
            CLSID_ProcessDebugManager,
            NULL,
            CLSCTX_ALL,
            IID_IProcessDebugManager,
            (void **)&g_pPDM));
    if (hr)
        goto Cleanup;

    hr = THR(g_pPDM->CreateApplication(&g_pDebugApp));
    if (hr)
        goto Cleanup;

    hr = THR(Format(FMT_OUT_ALLOC, &pchAppName, 0, MAKEINTRESOURCE(IDS_MESSAGE_BOX_TITLE)));
    if (hr)
        goto Cleanup;

    hr = THR(g_pDebugApp->SetName(pchAppName));
    if (hr)
        goto Cleanup;

    // This will fail if there is no MDM on the machine. That is OK.
    THR_NOTRACE(g_pPDM->AddApplication(g_pDebugApp, &g_dwAppCookie));

Cleanup:
    CoTaskMemFree(pszClsid);
    delete pchAppName;

    if (hkeyProcessDebugManager)
        RegCloseKey(hkeyProcessDebugManager);

    if (hr)
    {
        g_fScriptDebuggerInitFailed = TRUE;
        DeinitScriptDebugging();
    }

    g_csInitScriptDebugger.Leave();
    
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:     DeinitScriptDebugging
//
//----------------------------------------------------------------------------

void 
DeinitScriptDebugging()
{
   TraceTag((tagScriptCollection, "DeinitScriptDebugging"));

    if (g_pPDM)
        g_pPDM->RemoveApplication( g_dwAppCookie );

    if (g_pDebugApp)
        g_pDebugApp->Close();

    ClearInterface(&g_pPDM);
    ClearInterface(&g_pDebugApp);
}

#endif // NO_SCRIPT_DEBUGGER

/*
IMPLEMENT_SUBOBJECT_IUNKNOWN(CScriptDebugHost, CScriptCollection, ScriptCollection, _DebugHost)

//---------------------------------------------------------------------------
//
//  Function:   CScriptDebugHost::IllegalCall
//
//  Synposis:   Makes sure that we're called in the correct thread.
//              (there have been PDM-related bugs about this in the past).
//
//---------------------------------------------------------------------------

BOOL
CScriptCollection::IllegalCall()
{
    Assert(_pDoc);
    if (_pDoc->_dwTID != GetCurrentThreadId())
    {
        Assert(!"Script debugger called across thread boundry (not a MSHTML bug)!");
        return TRUE;
    }

    return FALSE;
}

//---------------------------------------------------------------------------
//
//  Function:   CScriptDebugHost::QueryInterface
//
//  Synposis:   As per IUnknown
//
//---------------------------------------------------------------------------

STDMETHODIMP
CScriptDebugHost::QueryInterface( REFIID iid, void **ppv )
{
    if (ScriptCollection()->IllegalCall())
        RRETURN(E_NOINTERFACE);

    if (!ppv)
        return E_POINTER;

    if (IsEqualIID(iid, IID_IUnknown) ||
        IsEqualIID(iid, IID_IDebugDocumentHost))
    {
        *ppv = static_cast<IDebugDocumentHost*>(this);
    }
    else if (IsEqualIID(iid, IID_IRemoteDebugApplicationEvents))
    {
        *ppv = static_cast<IRemoteDebugApplicationEvents*>(this);
    }
    else
    {
        *ppv = NULL;
        RRETURN(E_NOINTERFACE);
    }

    AddRef();
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Function:   CScriptDebugHost::GetDeferredText
//
//  Synopsis:   As per IDebugDocumentHost
//
//----------------------------------------------------------------------------

HRESULT
CScriptDebugHost::GetDeferredText(
    DWORD dwTextStartCookie,
    WCHAR *pcharText,
    SOURCE_TEXT_ATTR *pstaTextAttr,
    ULONG *pcNumChars,
    ULONG cMaxChars )
{
    TraceTag((tagScriptDebugHost, "GetDeferredText"));

    HRESULT hr = S_OK;

    if (pcharText && pcNumChars && ScriptCollection()->_pHtmCtx)
    {
        hr = THR(ScriptCollection()->_pHtmCtx->ReadUnicodeSource(
                pcharText,
                dwTextStartCookie,
                cMaxChars,
                pcNumChars));
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:   CScriptDebugHost::GetPathName
//
//  Synopsis:   As per IDebugDocumentHost
//
//----------------------------------------------------------------------------

HRESULT
CScriptDebugHost::GetPathName(BSTR *pbstrLongName, BOOL *pfIsOriginalFile)
{
    TraceTag((tagScriptDebugHost, "GetPathName"));

    TCHAR szPath[MAX_PATH];

    if (!pbstrLongName)
        RRETURN(E_POINTER);

    *pbstrLongName = 0;

    //
    // Figure out a file name. It would be nice to use GetUrlComponentHelper,
    // but GetUrlComponentHelper can only grab one component per call and we
    // will often require two components.
    //

    URL_COMPONENTS uc;
    ZeroMemory( &uc, sizeof(uc) );
    uc.dwStructSize = sizeof(uc);

    uc.lpszUrlPath      = szPath;
    uc.dwUrlPathLength  = ARRAY_SIZE(szPath);

    if (InternetCrackUrl (
            ScriptCollection()->_cstrDocUrl, 
            ScriptCollection()->_cstrDocUrl.Length(), 
            0, 
            &uc))
    {
        if (pfIsOriginalFile)
            *pfIsOriginalFile = (INTERNET_SCHEME_FILE == uc.nScheme ? TRUE : FALSE);

        TCHAR *pchFile = _tcsrchr (szPath, _T('/'));

        // If there was no path or the path has only a single '/'
        if (!*szPath || (pchFile && !pchFile[1]))
            *pbstrLongName = SysAllocString(_T("default.htm"));
        else
            *pbstrLongName = SysAllocString(pchFile ? (pchFile + 1) : szPath);
    }

    return *pbstrLongName ? S_OK : E_FAIL;
}


//+---------------------------------------------------------------------------
//
//  Function:   CScriptDebugHost::GetFileName
//
//  Synopsis:   As per IDebugDocumentHost
//
//----------------------------------------------------------------------------

HRESULT
CScriptDebugHost::GetFileName(BSTR *pbstrFileName)
{
    TraceTag((tagScriptDebugHost, "GetFileName"));

    if (!pbstrFileName)
        RRETURN(E_POINTER);

    HRESULT hr = E_FAIL;
    TCHAR szPath[MAX_PATH];
    URL_COMPONENTS uc;

    *pbstrFileName = 0;
    ZeroMemory (&uc, sizeof(uc));
    uc.dwStructSize = sizeof(uc);

    uc.lpszUrlPath      = szPath;
    uc.dwUrlPathLength  = ARRAY_SIZE(szPath);

    if (InternetCrackUrl(
        ScriptCollection()->_cstrDocUrl, 
        ScriptCollection()->_cstrDocUrl.Length(), 
        0, 
        &uc))
    {
        TCHAR *pchFile = _tcsrchr (szPath, _T('/'));

        if (pchFile && pchFile[1])
        {
            *pbstrFileName = SysAllocString(pchFile + 1);
            hr = pbstrFileName ? S_OK : E_OUTOFMEMORY;
            goto Cleanup;
        }
    }
    *pbstrFileName = SysAllocString(_T("default.htm"));
    hr = pbstrFileName ? S_OK : E_OUTOFMEMORY;

Cleanup:
    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Function:   CScriptDebugHost::OnConnectDebugger
//
//  Synopsis:   As per IRemoteDebugApplicationEvents
//
//---------------------------------------------------------------------------

HRESULT
CScriptDebugHost::OnConnectDebugger( IApplicationDebugger *pad )
{
    TraceTag((tagScriptDebuggerEvents, "OnConnectDebugger"));

    ScriptCollection()->_fDebuggerAttached = TRUE;
    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Function:   CScriptDebugHost::OnDisconnectDebugger
//
//  Synopsis:   As per IRemoteDebugApplicationEvents
//
//---------------------------------------------------------------------------

HRESULT
CScriptDebugHost::OnDisconnectDebugger()
{
    TraceTag((tagScriptDebuggerEvents, "OnDisconnectDebugger"));

    ScriptCollection()->_fDebuggerAttached = FALSE;
    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Function:   CScriptDebugHost::OnSetName
//
//  Synopsis:   As per IRemoteDebugApplicationEvents
//
//---------------------------------------------------------------------------

HRESULT
CScriptDebugHost::OnSetName( LPCOLESTR pstrName )
{
    TraceTag((tagScriptDebuggerEvents, "OnSetName"));

    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Function:   CScriptDebugHost::OnDebugOutput
//
//  Synopsis:   As per IRemoteDebugApplicationEvents
//
//---------------------------------------------------------------------------

HRESULT
CScriptDebugHost::OnDebugOutput( LPCOLESTR pstr )
{
    TraceTag((tagScriptDebuggerEvents, "OnDebugOutput"));

    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Function:   CScriptDebugHost::OnClose
//
//  Synopsis:   As per IRemoteDebugApplicationEvents
//
//---------------------------------------------------------------------------

HRESULT
CScriptDebugHost::OnClose()
{
    TraceTag((tagScriptDebuggerEvents, "OnClose"));

    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Function:   CScriptDebugHost::OnEnterBreakPoint
//
//  Synopsis:   As per IRemoteDebugApplicationEvents
//
//---------------------------------------------------------------------------

HRESULT
CScriptDebugHost::OnEnterBreakPoint( IRemoteDebugApplicationThread *prdat )
{
    TraceTag((tagScriptDebuggerEvents, "OnEnterBreakPoint"));

    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Function:   CScriptDebugHost::OnLeaveBreakPoint
//
//  Synopsis:   As per IRemoteDebugApplicationEvents
//
//---------------------------------------------------------------------------

HRESULT
CScriptDebugHost::OnLeaveBreakPoint( IRemoteDebugApplicationThread *prdat )
{
    TraceTag((tagScriptDebuggerEvents, "OnLeaveBreakPoint"));

    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Function:   CScriptDebugHost::OnCreateThread
//
//  Synopsis:   As per IRemoteDebugApplicationEvents
//
//---------------------------------------------------------------------------

HRESULT
CScriptDebugHost::OnCreateThread( IRemoteDebugApplicationThread *prdat )
{
    TraceTag((tagScriptDebuggerEvents, "OnCreateThread"));

    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Function:   CScriptDebugHost::OnDestroyThread
//
//  Synopsis:   As per IRemoteDebugApplicationEvents
//
//---------------------------------------------------------------------------

HRESULT
CScriptDebugHost::OnDestroyThread( IRemoteDebugApplicationThread *prdat )
{
    TraceTag((tagScriptDebuggerEvents, "OnDestroyThread"));

    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Function:   CScriptDebugHost::CScriptDebugHost::OnBreakFlagChange
//
//  Synopsis:   As per IRemoteDebugApplicationEvents
//
//---------------------------------------------------------------------------

HRESULT
CScriptDebugHost::OnBreakFlagChange( APPBREAKFLAGS abf, IRemoteDebugApplicationThread *prdatSteppingThread )
{
    TraceTag((tagScriptDebuggerEvents, "OnBreakFlagChange"));
    return S_OK;
}
*/

//---------------------------------------------------------------------------
//
//  Member:   CNamedItemsTable::AddItem
//
//---------------------------------------------------------------------------

HRESULT
CNamedItemsTable::AddItem(LPTSTR pchName, IUnknown * pUnkItem)
{
    HRESULT         hr;
    CNamedItem *    pItem;

    pItem = new CNamedItem(pchName, pUnkItem);
    if (!pItem)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(Append(pItem));

Cleanup:
    RRETURN (hr);
}

//---------------------------------------------------------------------------
//
//  Member:   CNamedItemsTable::GetItem
//
//---------------------------------------------------------------------------

HRESULT
CNamedItemsTable::GetItem(LPTSTR pchName, IUnknown ** ppUnkItem)
{
    int             c;
    CNamedItem **   ppItem;

    for (c = Size(), ppItem = (CNamedItem**)PData(); c; c--, ppItem++)
    {
        if (0 == StrCmpIC(pchName, (*ppItem)->_cstrName))
        {
            *ppUnkItem = (*ppItem)->_pUnkItem;
            (*ppUnkItem)->AddRef();
            RRETURN (S_OK);
        }
    }
    RRETURN (DISP_E_MEMBERNOTFOUND);
}

//---------------------------------------------------------------------------
//
//  Member:   CNamedItemsTable::FreeAll
//
//---------------------------------------------------------------------------

HRESULT
CNamedItemsTable::FreeAll()
{
    int             c;
    CNamedItem **   ppItem;

    for (c = Size(), ppItem = (CNamedItem**)PData(); c; c--, ppItem++)
    {
        delete (*ppItem);
    }
    super::DeleteAll();

    RRETURN (S_OK);
}

//---------------------------------------------------------------------------
//
//  Member:   CScriptMethodsTable::FreeAll
//
//---------------------------------------------------------------------------

CScriptMethodsTable::~CScriptMethodsTable()
{
    int             c;
    SCRIPTMETHOD *  pScriptMethod;

    for (c = Size(), pScriptMethod = (SCRIPTMETHOD*)PData();
         c > 0;
         c--, pScriptMethod++)
    {
        pScriptMethod->cstrName.Free();
    }
    DeleteAll();
}

//---------------------------------------------------------------------------
//
//  Member:   CScriptCollection::GetDispID
//
//---------------------------------------------------------------------------

HRESULT
CScriptCollection::GetDispID(
    CMarkup *               pMarkup,
    BSTR                    bstrName,
    DWORD                   grfdex,
    DISPID *                pdispid)
{
    HRESULT                 hr;
    HRESULT                 hr2;
    LPTSTR                  pchNamespace;
    CScriptMethodsTable *   pScriptMethodsTable;
    int                     i, c;
    STRINGCOMPAREFN         pfnStrCmp;
    CScriptHolder **        ppHolder;
    SCRIPTMETHOD            scriptMethod;
    SCRIPTMETHOD *          pScriptMethod;
    IDispatch *             pdispEngine = NULL;
    IDispatchEx *           pdexEngine  = NULL;
    int                     idx;
    DISPID                  dispidEngine;

    //
    // startup
    //

    grfdex &= (~fdexNameEnsure);    // don't allow name to be ensured here
    pfnStrCmp = (grfdex & fdexNameCaseSensitive) ? StrCmpC : StrCmpIC;

    hr = THR(pMarkup->EnsureScriptContext());
    if (hr)
        goto Cleanup;

    pchNamespace        =  pMarkup->ScriptContext()->_cstrNamespace;
    pScriptMethodsTable = &pMarkup->ScriptContext()->_ScriptMethodsTable;

    //
    // try existing cached names
    //

    for (i = 0, c = pScriptMethodsTable->Size(); i < c; i++)
    {
        if (0 == pfnStrCmp((*pScriptMethodsTable)[i].cstrName, bstrName))
        {
            *pdispid = DISPID_OMWINDOWMETHODS + i;
            hr = S_OK;
            goto Cleanup;
        }
    }

    //
    // query all the engines for the name
    //

    hr = DISP_E_UNKNOWNNAME;

    for (c = _aryHolder.Size(), ppHolder = _aryHolder; c > 0; c--, ppHolder++)
    {
        Assert (!pdispEngine && !pdexEngine);

        // get IDispatch and IDispatchEx

        hr2 = THR((*ppHolder)->_pScript->GetScriptDispatch(pchNamespace, &pdispEngine));
        if (hr2)
            continue;

        _fInEnginesGetDispID = TRUE;

        if (0 == (grfdex & fdexFromGetIdsOfNames))
        {
            IGNORE_HR(pdispEngine->QueryInterface(IID_IDispatchEx, (void **)&pdexEngine));
        }

        // query for the name

        if (pdexEngine)
        {
            hr2 = THR_NOTRACE(pdexEngine->GetDispID(bstrName, grfdex, &dispidEngine));
        }
        else
        {
            hr2 = THR_NOTRACE(pdispEngine->GetIDsOfNames(IID_NULL, &bstrName, 1, LCID_SCRIPTING, &dispidEngine));
        }

        _fInEnginesGetDispID = FALSE;

        if (S_OK != hr2)        // if name is unknown to this engine
            goto LoopCleanup;   // this is not a fatal error; goto loop cleanup and then continue

        // name is known; assign it our own dispid, and append all the info to our list for remapping

        hr = THR(pScriptMethodsTable->AppendIndirect(&scriptMethod));
        if (hr)
            goto Cleanup;

        idx = pScriptMethodsTable->Size() - 1;
        *pdispid = DISPID_OMWINDOWMETHODS + idx;

        pScriptMethod = &((*pScriptMethodsTable)[idx]);
        pScriptMethod->dispid  = dispidEngine;
        pScriptMethod->pHolder = *ppHolder;
        hr = THR(pScriptMethod->cstrName.Set(bstrName));
        if (hr)
            goto Cleanup;

        // loop cleanup

LoopCleanup:
        ClearInterface(&pdispEngine);
        ClearInterface(&pdexEngine);
    }

Cleanup:
    RRETURN (hr);
}

//---------------------------------------------------------------------------
//
//  Member:   CScriptCollection::InvokeEx
//
//---------------------------------------------------------------------------

HRESULT
CScriptCollection::InvokeEx(
    CMarkup *               pMarkup,
    DISPID                  dispid,
    LCID                    lcid,
    WORD                    wFlags,
    DISPPARAMS *            pDispParams,
    VARIANT *               pvarRes,
    EXCEPINFO *             pExcepInfo,
    IServiceProvider *      pServiceProvider)
{
    HRESULT                 hr;
    HRESULT                 hr2;
    LPTSTR                  pchNamespace;
    CScriptMethodsTable *   pScriptMethodsTable;
    IDispatch *             pdispEngine = NULL;
    IDispatchEx *           pdexEngine  = NULL;
    SCRIPTMETHOD *          pScriptMethod;
    int                     idx = dispid - DISPID_OMWINDOWMETHODS;

    //
    // startup
    //
        
    hr = THR(pMarkup->EnsureScriptContext());
    if (hr)
        goto Cleanup;

    pchNamespace        =  pMarkup->ScriptContext()->_cstrNamespace;
    pScriptMethodsTable = &pMarkup->ScriptContext()->_ScriptMethodsTable;

    if (idx < 0 || pScriptMethodsTable->Size() <= idx)
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    //
    // invoke
    //

    pScriptMethod = &((*pScriptMethodsTable)[idx]);

    hr = THR(pScriptMethod->pHolder->_pScript->GetScriptDispatch(pchNamespace, &pdispEngine));
    if (hr)
        goto Cleanup;

    hr2 = THR_NOTRACE(pdispEngine->QueryInterface(IID_IDispatchEx, (void**)&pdexEngine));

    if (pdexEngine)
    {
        hr = THR(pdexEngine->InvokeEx(
                pScriptMethod->dispid, lcid, wFlags, pDispParams,pvarRes, pExcepInfo, pServiceProvider));

        ReleaseInterface(pdexEngine);
    }
    else
    {
        hr = THR(pdispEngine->Invoke(
                pScriptMethod->dispid, IID_NULL, lcid, wFlags, pDispParams, pvarRes, pExcepInfo, NULL));
    }

Cleanup:
    ReleaseInterface (pdispEngine);

    RRETURN (hr);
}

//---------------------------------------------------------------------------
//
//  Member:   CScriptCollection::InvokeName
//
//---------------------------------------------------------------------------

HRESULT
CScriptCollection::InvokeName(
    CMarkup *               pMarkup,
    LPTSTR                  pchName,
    LCID                    lcid,
    WORD                    wFlags,
    DISPPARAMS *            pDispParams,
    VARIANT *               pvarRes,
    EXCEPINFO *             pExcepInfo,
    IServiceProvider *      pServiceProvider)
{
    HRESULT             hr;
    HRESULT             hr2;
    LPTSTR              pchNamespace;
    CScriptHolder **    ppHolder;
    IDispatch *         pdispEngine = NULL;
    IDispatchEx *       pdexEngine  = NULL;
    BSTR                bstrName = NULL;
    DISPID              dispid;
    int                 c;

    //
    // startup
    //

    c = _aryHolder.Size();
    if (0 == c)
    {
        hr = DISP_E_UNKNOWNNAME;
        goto Cleanup;
    }

    hr = THR(FormsAllocString (pchName, &bstrName));
    if (hr)
        goto Cleanup;

    hr = THR(pMarkup->EnsureScriptContext());
    if (hr)
        goto Cleanup;

    pchNamespace = pMarkup->ScriptContext()->_cstrNamespace;

    //
    // find an engine that knows the name and invoke it
    //

    hr = DISP_E_UNKNOWNNAME;
    for (ppHolder = _aryHolder; c > 0; c--, ppHolder++)
    {
        // get IDispatch / IDispatchEx

        hr2 = THR((*ppHolder)->_pScript->GetScriptDispatch(pchNamespace, &pdispEngine));
        if (hr2)
            continue;

        hr2 = THR_NOTRACE(pdispEngine->QueryInterface(IID_IDispatchEx, (void **)&pdexEngine));
        if (S_OK == hr2)
        {
            // invoke via IDispatchEx

            hr2 = THR_NOTRACE(pdexEngine->GetDispID(bstrName, fdexNameCaseInsensitive, &dispid));
            if (S_OK == hr2)
            {
                hr = THR(pdexEngine->InvokeEx(
                    dispid, lcid, wFlags, pDispParams, pvarRes, pExcepInfo, pServiceProvider));

                goto Cleanup; // done
            }
            ClearInterface(&pdexEngine);
        }
        else
        {
            // invoke via IDispatch

            hr2 = THR_NOTRACE(pdispEngine->GetIDsOfNames(IID_NULL, &pchName, 1, lcid, &dispid));
            if (S_OK == hr2)
            {
                hr = THR(pdispEngine->Invoke(
                    dispid, IID_NULL, lcid, wFlags, pDispParams, pvarRes, pExcepInfo, NULL));

                goto Cleanup; // done
            }
        }

        // loop cleanup

        ClearInterface(&pdispEngine);
    }

Cleanup:

    ReleaseInterface(pdispEngine);
    ReleaseInterface(pdexEngine);

    FormsFreeString(bstrName);

    RRETURN (hr);
}
