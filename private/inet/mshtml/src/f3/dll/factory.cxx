//+------------------------------------------------------------------------
//
//  File:       factory.cxx
//
//  Contents:   Class factories.
//
//  History:    20-Dec-94   GaryBu Created
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_OCMM_H_
#define X_OCMM_H_
#include <ocmm.h>
#endif

#ifndef X_COREGUID_H_
#define X_COREGUID_H_
#include "coreguid.h"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_SITEGUID_H_
#define X_SITEGUID_H_
#include "siteguid.h"
#endif

#ifdef WIN16
#ifndef X_EXDISP_H_
#define X_EXDISP_H_
#include <exdisp.h>
#endif

#ifndef X_SHLGUID_H_
#define X_SHLGUID_H_
#include <shlguid.h>
#endif

#define NO_DEBUG_HOOK
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef X_IIMGCTX_H_
#define X_IIMGCTX_H_
#include "iimgctx.h"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include <formkrnl.hxx>
#endif

#ifndef X_HTMLDLG_HXX_
#define X_HTMLDLG_HXX_
#include <htmldlg.hxx>
#endif

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include <uwininet.h>
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#ifndef X_CLSFAC3_HXX_
#define X_CLSFAC3_HXX_
#include "clsfac3.hxx"
#endif

#ifndef X_OPTARY_H_
#define X_OPTARY_H_
#include <optary.h>
#endif

#ifndef X_SHLGUIDP_H_
#define X_SHLGUIDP_H_
#include <shlguidp.h>
#endif

#ifdef UNIX
EXTERN_C const CLSID CLSID_CRowPosition;
extern CBaseCF::FNCREATE CreateRowPosition;
#endif

#pragma warning(disable: 4041)

EXTERN_C const GUID CLSID_DataObject;
EXTERN_C const GUID CLSID_HTMLWindowProxy;
EXTERN_C const GUID CLSID_HTMLServerDoc;
EXTERN_C const GUID CLSID_Scriptlet;
EXTERN_C const GUID CLSID_MHTMLDocument;
EXTERN_C const GUID CLSID_HTMLPluginDocument;
EXTERN_C const GUID CLSID_HTADoc;

#if defined(QUILL) || defined(TREE_SYNC)
// {3FCCC080-AA1E-11d1-8E02-00A0C91BC8EC}
const CLSID CLSID_QHTMLDocument = {0x3fccc080, 0xaa1e, 0x11d1, 0x8e, 0x2, 0x0, 0xa0, 0xc9, 0x1b, 0xc8, 0xec};
#endif

// CHROME
// A private GUID between Trident3D and Trident to instantiate a windowless CDoc.
// {65014010-9F62-11d1-A651-00600811D5CE}
const CLSID CLSID_ChromeHTMLDocument = {0x65014010, 0x9f62, 0x11d1, 0xa6, 0x51, 0x0, 0x60, 0x8, 0x11, 0xd5, 0xce};

//+------------------------------------------------------------------------
//
//  Create instance functions
//
//-------------------------------------------------------------------------

extern CBaseCF::FNCREATE CreateDoc;
extern CBaseCF::FNCREATE CreateScriptlet;
extern CBaseCF::FNCREATE CreateMhtmlDoc;
extern CBaseCF::FNCREATE CreateServerDoc;
extern CBaseCF::FNINITCLASS InitDocClass;
extern CBaseCF::FNCREATE CreateDocFullWindowEmbed;  // ref to plugin handle file mechanism.
extern CBaseCF::FNCREATE CreateHTADoc;
extern CBaseCF::FNCREATE CreatePropertyFrame;
// CHROME
// Entry point to create an MSHTML doc that is hosted windowless by Chrome
extern CBaseCF::FNCREATE CreateChromeDoc;

#ifndef NO_PROPERTY_PAGE
extern CStaticCF::FNCREATE CreateGenericPropertyPage;
extern CStaticCF::FNCREATE CreateInlineStylePropertyPage;
#endif // NO_PROPERTY_PAGE
extern CStaticCF::FNCREATE CreateDwnBindInfo;
extern CStaticCF::FNCREATE CreateIImgCtx;
extern CStaticCF::FNCREATE CreateIImageDecodeFilter;
extern CStaticCF::FNCREATE CreateIIntDitherer;
extern CStaticCF::FNCREATE CreateSecurityProxy;
extern CStaticCF::FNCREATE CreateHtmlLoadOptions;
extern CStaticCF::FNCREATE CreateRecalcEngine;
extern CStaticCF::FNCREATE CreateTridentAPI;

#ifndef WIN16
#else
//extern CStaticCF::FNCREATE CreateWebBrowser16;
//extern CStaticCF::FNCREATE CreateShDocViewSkel;
//extern CStaticCF::FNCREATE CreateUrlHistoryStg;
#endif

#ifndef NO_DEBUG_HOOK
class CHook;

extern CHook * CreateHook();
#endif

//+------------------------------------------------------------------------
//
//  Class factories
//
//-------------------------------------------------------------------------

// Use CBaseLockCF for documents so that clients can addref thread state
// without holding on to an instance of the document.
CBaseLockCF g_cfDoc                     (CreateDoc, InitDocClass);
#if defined(QUILL) || defined(TREE_SYNC)
CBaseLockCF g_cfQDoc                    (CreateDoc, InitDocClass);
#endif
// CHROME 
CBaseLockCF g_cfChromeDoc               (CreateChromeDoc, InitDocClass);
CBaseLockCF g_cfHTADoc                  (CreateHTADoc, InitDocClass);
CBaseLockCF g_cfScriptlet               (CreateScriptlet, InitDocClass);

CBaseCF     g_cfMhtmlDoc                (CreateMhtmlDoc, InitDocClass);
CBaseCF     g_cfDocFullWindowEmbed      (CreateDocFullWindowEmbed, InitDocClass);
CBaseCF     g_cfServerDoc               (CreateServerDoc, InitDocClass);

#if !defined(NO_PROPERTY_PAGE) && (DBG==1)
CStaticCF   g_cfGenericPropertyPage     (CreateGenericPropertyPage);
CStaticCF   g_cfInlineStylePropertyPage (CreateInlineStylePropertyPage);
#endif // NO_PROPERTY_PAGE

CStaticCF   g_cfCDwnBindInfo            (CreateDwnBindInfo);
CStaticCF   g_cfIImgCtx                 (CreateIImgCtx);
CStaticCF   g_cfIImageDecodeFilter      (CreateIImageDecodeFilter);
CStaticCF   g_cfIIntDitherer            (CreateIIntDitherer);
CStaticCF   g_cfSecurityProxy           (CreateSecurityProxy);
CStaticCF   g_cfHtmlLoadOptions         (CreateHtmlLoadOptions);
CStaticCF   g_cfRecalcEngine            (CreateRecalcEngine);
CStaticCF   g_cfTridentAPI              (CreateTridentAPI);

#ifdef UNIX
CBaseCF     g_cfRowPosition             (CreateRowPosition);
#endif

#define ZEROIMPACTON

#ifdef ZEROIMPACTON
extern class CHtmlComponentConstructor g_cfHtmlComponentConstructor;
#else
extern CStaticCF::FNCREATE CreateHtmlComponentConstructor;
CStaticCF   g_cfHtmlComponentConstructor (CreateHtmlComponentConstructor);
#endif

extern class CResProtocolCF g_cfResProtocol;
extern class CJSProtocolCF  g_cfJSProtocol;
extern class CSysimageProtocolCF  g_cfSysimageProtocol;
#ifndef WIN16
extern class CMailtoFactory g_cfMailtoProtocol;
#endif

extern class CAboutProtocolCF g_cfAboutProtocol;
//+------------------------------------------------------------------------
//
//  Class factory cache
//
//-------------------------------------------------------------------------

struct CLSCACHE
{
    const CLSID *   pclsid;
    IClassFactory * pCF;
};

extern CLSID CLSID_CHtmlComponentConstructor;

CLSCACHE g_aclscache[] =
{
    &CLSID_HTMLDocument,                        &g_cfDoc,
#if defined(QUILL) || defined(TREE_SYNC)
    &CLSID_QHTMLDocument,                       &g_cfQDoc,
#endif
    // CHROME
    // Another special way to create a CDoc (this time a windowless doc
    // hosted by Chrome)
    &CLSID_ChromeHTMLDocument,                  &g_cfChromeDoc,
    &CLSID_Scriptlet,                           &g_cfScriptlet,
    &CLSID_MHTMLDocument,                       &g_cfMhtmlDoc,
#if DBG==1    
    &CLSID_CCDGenericPropertyPage,              &g_cfGenericPropertyPage,
    &CLSID_CInlineStylePropertyPage,            &g_cfInlineStylePropertyPage,
#endif //  DBG==1
    &CLSID_CDwnBindInfo,                        &g_cfCDwnBindInfo,
    &CLSID_IImgCtx,                             &g_cfIImgCtx,
    &CLSID_IImageDecodeFilter,                  &g_cfIImageDecodeFilter,
    &CLSID_IntDitherer,                         &g_cfIIntDitherer,
    &CLSID_JSProtocol,                          (IClassFactory *)(void *)&g_cfJSProtocol,
    &CLSID_ResProtocol,                         (IClassFactory *)(void *)&g_cfResProtocol,
    &CLSID_SysimageProtocol,                    (IClassFactory *)(void *)&g_cfSysimageProtocol,
#ifndef WIN16
    &CLSID_MailtoProtocol,                      (IClassFactory *)(void *)&g_cfMailtoProtocol,
#endif // ndef WIN16
    &CLSID_AboutProtocol,                       (IClassFactory *)(void *)&g_cfAboutProtocol,
    &CLSID_HTMLWindowProxy,                     &g_cfSecurityProxy,
    &CLSID_HTMLServerDoc,                       &g_cfServerDoc,

    // Special way to create a CDoc based on this alternative magic clsid:
    &CLSID_HTMLPluginDocument,                  &g_cfDocFullWindowEmbed,
    &CLSID_HTMLLoadOptions,                     &g_cfHtmlLoadOptions,
    &CLSID_CRecalcEngine,                       &g_cfRecalcEngine,
    &CLSID_HTADoc,                              &g_cfHTADoc,
    &CLSID_CHtmlComponentConstructor,           (CStaticCF*)&g_cfHtmlComponentConstructor,

    &CLSID_TridentAPI,                          &g_cfTridentAPI,
    
#ifdef UNIX
    &CLSID_CRowPosition,                        &g_cfRowPosition,
#endif
};

//+---------------------------------------------------------------
//
//  Function:   LocalGetClassObject
//
//  Synopsis:   Local function for locating class factories
//
//----------------------------------------------------------------

HRESULT
LocalGetClassObject(REFCLSID clsid, REFIID iid, LPVOID FAR* ppv)
{
    HRESULT         hr;
    int             i;
    CLSCACHE *      pcc;    

    //
    // First try the class cache.
    //

    for (i = ARRAY_SIZE(g_aclscache) - 1, pcc = g_aclscache;
         i >= 0;
         i--, pcc++)
    {
        Assert(pcc->pclsid && pcc->pCF);
        if (IsEqualCLSID(clsid, *pcc->pclsid))
        {
            hr = pcc->pCF->QueryInterface(iid, ppv);
            RRETURN(hr);
        }
    }


    //
    // Second, check of it is a Property Page CLSID shdocvw understands, if so
    // we create a CPropPageCF specific to the resource moniker identified
    // by the CLSID
    //

#ifndef NO_HTML_DIALOG         
    IOleCommandTarget * pCommandTarget = NULL;
    IMoniker          * pmk = NULL;
    CHTMLPropPageCF   * pHTMLPropPageCF = NULL;
    VARIANT             varIn;
    VARIANT             varOut;
    BOOL                fIsPropPageClsid = FALSE;
    
    VariantInit(&varOut);

    // co-create shdocvw
    hr = THR(CoCreateInstance(
            CLSID_DocHostUIHandler,                              
            NULL,            
            CLSCTX_INPROC_SERVER,            
            IID_IOleCommandTarget,            
            (void**)&pCommandTarget));
    if (hr)
        goto PropPageCleanup;
    
    // check if the clsid is a property page supported by shdocvw

    V_VT(&varIn) = VT_UINT_PTR;
    V_BYREF(&varIn) = (void *)&clsid;
    hr = pCommandTarget->Exec(
            &CGID_DocHostCommandHandler, 
            SHDVID_CLSIDTOMONIKER, 
            0, 
            &varIn, 
            &varOut);       
    fIsPropPageClsid = !hr;
    if (hr)
        goto PropPageCleanup;   

    // extract the IMoniker *
    if (V_VT(&varOut) == VT_UNKNOWN)
        hr = V_UNKNOWN(&varOut)->QueryInterface(IID_IMoniker, (void**)&pmk);
    else
        hr = E_FAIL;
    if (hr)
        goto PropPageCleanup;

    // create a class factory specific to the moniker
    pHTMLPropPageCF = new CHTMLPropPageCF(pmk);
    hr = pHTMLPropPageCF->QueryInterface(iid, ppv);
    if (hr)
        goto PropPageCleanup;

PropPageCleanup:
    ReleaseInterface(pHTMLPropPageCF);
    ReleaseInterface(pmk);
    ReleaseInterface(pCommandTarget);
    VariantClear(&varOut);

    // stop if the clsid was a prop page's clsid
    if (fIsPropPageClsid)
        RRETURN(hr);

#endif // NO_HTML_DIALOG


#ifndef NO_DEBUG_HOOK
    //
    // Maybe its a debug hook request
    //
    if (IsEqualCLSID(clsid, CLSID_CHook))
    {
        *ppv = CreateHook();

        RRETURN(*ppv ? S_OK : E_OUTOFMEMORY);
    }
#endif // NO_DEBUG_HOOK

    *ppv = NULL;
    RRETURN(CLASS_E_CLASSNOTAVAILABLE);
}


//+---------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Standard DLL entrypoint for locating class factories
//
//----------------------------------------------------------------

STDAPI
DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID FAR* ppv)
{
    RRETURN_NOTRACE(LocalGetClassObject(clsid, iid, ppv));
}


//+---------------------------------------------------------------
//
//  Function:   DllEnumClassObjects
//
//  Synopsis:   
//
//----------------------------------------------------------------

STDAPI
DllEnumClassObjects(int i, CLSID *pclsid, IUnknown **ppUnk)
{
    if ((UINT)i >= ARRAY_SIZE(g_aclscache))
        return S_FALSE;

    *pclsid = *g_aclscache[i].pclsid;
    *ppUnk = g_aclscache[i].pCF;
    (*ppUnk)->AddRef();

    return S_OK;
}
