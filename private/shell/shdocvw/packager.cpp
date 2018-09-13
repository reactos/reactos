//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994
//
//  File:       persist.cxx
//
//  Contents:   Implmentation of Office9 Thicket Save API
//
//----------------------------------------------------------------------------


#include "priv.h"
#include "shellp.h"
#include <mshtml.h>
#include <winineti.h>
#include <mlang.h>
// fake out mimeole.h's dll linkage directives for our delay load stuff in dllload.c
#define _MIMEOLE_
#define DEFINE_STRCONST
#include <mimeole.h>
#include "resource.h"
#include "packager.h"
#include "reload.h"

#include <mluisupp.h>

#define DEFINE_STRING_CONSTANTS
#pragma warning( disable : 4207 ) 
#include "htmlstr.h"
#pragma warning( default : 4207 )

const GUID CLSID_IMimeInternational =
{0xfd853cd9, 0x7f86, 0x11d0, {0x82, 0x52, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4}};

const GUID IID_IMimeInternational =
{0xc5588349, 0x7f86, 0x11d0, {0x82, 0x52, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4}};

const GUID IID_IMimeBody =
{0xc558834c, 0x7f86, 0x11d0, {0x82, 0x52, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4}};

// Trident legacy defines...

#define RRETURN(hr) return hr;
#define ReleaseInterface(punk) { if (punk) punk->Release(); punk = NULL; }

// Local prototypes

void RemoveBookMark(WCHAR *pwzURL, WCHAR **ppwzBookMark);
void RestoreBookMark(WCHAR *pwzBookMark);

HRESULT HrGetElement(IHTMLDocument2 *pDoc, LPCSTR pszName, IHTMLElement **ppElem);
HRESULT HrGetBodyElement(IHTMLDocument2 *pDoc, IHTMLBodyElement **ppBody);
HRESULT HrSetMember(LPUNKNOWN pUnk, BSTR bstrMember, BSTR bstrValue);
HRESULT HrGetCollectionOf(IHTMLDocument2 *pDoc, BSTR bstrTagName, IHTMLElementCollection **ppCollect);
HRESULT HrGetCollectionItem(IHTMLElementCollection *pCollect, ULONG uIndex, REFIID riid, LPVOID *ppvObj);
ULONG UlGetCollectionCount(IHTMLElementCollection *pCollect);
HRESULT HrGetMember(LPUNKNOWN pUnk, BSTR bstrMember,LONG lFlags, BSTR *pbstr);
HRESULT HrLPSZToBSTR(LPCSTR lpsz, BSTR *pbstr);
HRESULT HrBSTRToLPSZ(BSTR bstr, LPSTR *lplpsz);
HRESULT HrGetCombinedURL( IHTMLElementCollection *pCollBase,
                          LONG cBase,
                          LONG lElemPos,
                          BSTR bstrRelURL,
                          BSTR bstrDocURL,
                          BSTR *pbstrBaseURL);

class CHashEntry {
public:
    CHashEntry(void) : m_bstrKey(NULL), m_bstrValue(NULL), m_pheNext(NULL) {};
    ~CHashEntry(void)
    {
        if (m_bstrKey)
            SysFreeString(m_bstrKey);
        if (m_bstrValue)
            SysFreeString(m_bstrValue);
    }

    BOOL SetKey(BSTR bstrKey)
    {
        ASSERT(m_bstrKey==NULL);
        m_bstrKey = SysAllocString(bstrKey);
        return m_bstrKey != NULL;
    }

    BOOL SetValue(BSTR bstrValue)
    {
        ASSERT(m_bstrValue==NULL || !StrCmpIW(m_bstrValue, c_bstr_BLANK) ||
               !StrCmpIW(m_bstrValue, bstrValue));
        m_bstrValue = SysAllocString(bstrValue);
        return m_bstrValue != NULL;
    }

    BSTR       m_bstrKey;
    BSTR       m_bstrValue;
    CHashEntry  *m_pheNext;        
};


class CWebArchive
{
public:

    CWebArchive(CThicketProgress* ptp=NULL);
    ~CWebArchive(void);

    virtual HRESULT Init( LPCTSTR lpstrDoc, DWORD dwHashSize );

    virtual HRESULT AddURL( BSTR bstrURL, CHashEntry **pphe ) = 0;
    virtual HRESULT AddFrameOrStyleEntry( BSTR bstrURL, CHashEntry **pphe, LPTSTR lpstrFrameDoc ) = 0;
    virtual HRESULT Find(BSTR bstrF, CHashEntry **pphe);

    virtual HRESULT Commit(void);
    virtual HRESULT Revert(void);

    virtual HRESULT ArchiveDocumentText(IHTMLDocument2 *pDoc, UINT cpDoc, BOOL fFrameDoc) = 0;
    virtual HRESULT ArchiveCSSText( BSTR bstrCSSUrl, LPCSTR lpszSSText, LPCTSTR lpszStyleDoc ) = 0;

protected:

    LPTSTR m_lpstrDoc;          // Desintation file for thicket document  
    LPTSTR m_lpstrSafeDoc;      // Temp name of original file, which we delete on Commit()

    CThicketProgress*   m_ptp;

    enum ThURLType {
        thurlMisc,
        thurlHttp,
        thurlFile
    };

    ThURLType _GetURLType( BSTR bstrURL );

    HRESULT _BackupOldFile(void);

    // hash table stuff stolen from MIMEEDIT
    HRESULT _Insert(BSTR bstrI, BSTR bstrThicket, CHashEntry **pphe);
    inline DWORD Hash(LPWSTR psz);

    DWORD       m_cBins;
    CHashEntry  *m_rgBins;
};


class CThicketArchive : public CWebArchive
{
public:

    CThicketArchive(CThicketProgress* ptp=NULL);
    ~CThicketArchive(void);

    virtual HRESULT Init( LPCTSTR lpstrDoc, DWORD dwHashSize );

    virtual HRESULT AddURL( BSTR bstrURL, CHashEntry **pphe );
    virtual HRESULT AddFrameOrStyleEntry( BSTR bstrURL, CHashEntry **pphe, LPTSTR lpstrFrameDoc );

    virtual HRESULT Commit(void);
    virtual HRESULT Revert(void);

    virtual HRESULT ArchiveDocumentText(IHTMLDocument2 *pDoc, UINT cpDoc, BOOL fFrameDoc);
    virtual HRESULT ArchiveCSSText( BSTR bstrCSSUrl, LPCSTR lpszSSText, LPCTSTR lpszStyleDoc );

protected:

    LPTSTR m_lpstrFilesDir;     // directory for document's supporting files.
    LPTSTR m_lpstrFilesDirName; // suffix of m_lpstrFilesDir
    LPTSTR m_lpstrSafeDir;      // Temp name of original files directory, which we delete on Commit()
    BOOL   m_fFilesDir;         // TRUE if m_lpstrFilesDir has been created.


    HRESULT _ApplyMarkOfTheWeb( IHTMLDocument2 *pDoc, LPSTREAM pstm, BOOL fUnicode );

    HRESULT _AddHttpEntry( BSTR bstrURL, CHashEntry **pphe, LPTSTR lpstrDstFile, LPTSTR lpstrSrcFile=NULL );
    HRESULT _AddFileEntry( BSTR bstrURL, CHashEntry **pphe, LPTSTR lpstrDstFile, LPTSTR lpstrSrcFile=NULL );
    HRESULT _AddMiscEntry( BSTR bstrURL, CHashEntry **pphe, LPTSTR lpstrDstFile, int cchDstFile );

    HRESULT _PersistHttpURL( BSTR bstrURL, CHashEntry **pphe );
    HRESULT _PersistFileURL( BSTR bstrURL, CHashEntry **pphe );
    HRESULT _PersistMiscURL( BSTR bstrURL, CHashEntry **pphe );

    HRESULT _BackupOldDirectory(void);
    HRESULT _RemoveOldDirectoryAndChildren( LPCWSTR pszDir );

    HRESULT _Insert(BSTR bstrI, LPTSTR lpszFile, int cchFile, CHashEntry **pphe);
};

class CMHTMLArchive : public CWebArchive
{
public:

    CMHTMLArchive(CThicketProgress* ptp=NULL);
    ~CMHTMLArchive(void);

    virtual HRESULT Init( LPCTSTR lpstrDoc, DWORD dwHashSize );

    virtual HRESULT AddURL( BSTR bstrURL, CHashEntry **pphe );
    virtual HRESULT AddFrameOrStyleEntry( BSTR bstrURL, CHashEntry **pphe, LPTSTR lpstrFrameDoc );

    virtual HRESULT ArchiveDocumentText(IHTMLDocument2 *pDoc, UINT cpDoc, BOOL fFrameDoc);
    virtual HRESULT ArchiveCSSText( BSTR bstrCSSUrl, LPCSTR lpszSSText, LPCTSTR lpszStyleDoc );

    virtual HRESULT SetCharset(UINT uiCharset, CSETAPPLYTYPE csat, IMimeBody *pBody);

protected:

    HBODY m_hBodyAlt;
    IMimeMessage *m_pimm;
};

/*
 * The following classes implement extended Save As MTHML functionality.
 * Access to the extended functionality is controlled by new MECD_ flags
 * defined in mimeole.h. Clients of the C API in this module should notice
 * mimimal change in its behavior. ( limited to the additional inclusion
 * table and table cell background images ).
 *
 * The root idea is that of a collection packager, which takes a subset
 * of the document.all collection, filters the elements of that subcollection,
 * and marshall's the element data into the MIMEOle document This is patterned
 * after the existing PackageImageData routine, and relies heavily on
 * HrAddImageToMessage, which is much more general than its name implies.
 *
 *
 * Stylesheets introduce some repetition, as the stylesheet OM is similar,
 * but not similar enough, to support common base classes specialized via
 * templates.
 *
 * The process of adding new packagers is pretty straight-forward.
 * [1]  (a) if the packaged attribute is a complete URL, derive from CCollectionPackager
 *      (b) if the attribute is a relative URL, derive from CRelativeURLPackager
 * [2] Implement InitFromCollection. Have it call _InitSubCollection() with the tag name.
 *     See CImagePackager::InitFromCollection() as a simple example.
 * [3] Implement _GetTargetAttribute() to return the attribute you want to package.
 *     You may want to add the string constants for [2] and [3] to htmlstr.h
 * [4] Define an MECD_ control flag, if the thing you're packaging is new.
 * [5] Add a local var of your packager type to CDocumentPackager::PackageDocument.
 * [6] Follow the pattern of the other packagers in CDocumentPackager::PackageDocument
 *
 * For elements with multiple persisted attributes, it's dealer's choice as to how
 * to approach it. Write seperate, simpler packagers for each attribute or write
 * one packager that deals with all of the target element's attributes.
 */



/*
 *  CCollectionPackager - abstract base class for HTML element packagers.
 *      Implements subsampling from the all collection, iteration over the
 *  collection, and basic packaging functionality.
 *
 *      Derived classes must implement InitFromCollection and _GetTargetAttribute.
 *  InitFromCollection - derived class should store the desired subset of the
 *      input collection into the m_pColl data member. _InitSubCollection is
 *      a useful method for this purpose.
 *  _GetTargetAttribute - derived class should return a BSTR naming the attribute
 *      of the element to be packaged.
 *
 */
class CCollectionPackager
{
public:
    virtual ~CCollectionPackager(void);
    virtual HRESULT InitFromCollection(IHTMLElementCollection *pColl,
                                       ULONG *pcElems = NULL) = 0;
    virtual HRESULT PackageData(CWebArchive *pwa, BOOL *pfCancel = NULL,
                                CThicketProgress *ptp = NULL, ULONG progLow = 0, ULONG progHigh = 100)
    { 
        return _PackageData( pwa, m_pColl, pfCancel, ptp, progLow, progHigh );
    }

protected:

    CCollectionPackager(void) : m_pColl(NULL), m_fAddCntLoc(FALSE) {};

    HRESULT _InitSubCollection(IHTMLElementCollection *pAll,
                              BSTR bstrTagName,
                              IHTMLElementCollection **ppSub,
                              ULONG *pcElems = NULL);

    virtual BSTR _GetTargetAttribute(void) = 0;

    virtual HRESULT _GetElementURL(IHTMLElement *pElem, BSTR *pbstrURL);
    virtual HRESULT _PackageData(CWebArchive *pwa,
                                 IHTMLElementCollection *pColl,
                                 BOOL *pfCancel = NULL,
                                 CThicketProgress *ptp = NULL, ULONG progLow = 0, ULONG progHigh = 100);
    virtual HRESULT _PackageElement(CWebArchive *pwa,
                                    IHTMLElement *pElem);

    IHTMLElementCollection *m_pColl; 
    BOOL                    m_fAddCntLoc;
};

/*
 * CImagePackager - packages the src's of IMG tags.
 */
class CImagePackager : public CCollectionPackager
{
public:
    CImagePackager(void) {};
    virtual ~CImagePackager(void) {};
 
    virtual HRESULT InitFromCollection(IHTMLElementCollection *pColl,
                                       ULONG *pcElems = NULL);
protected:

    virtual BSTR _GetTargetAttribute(void);

};

/*
 * CInputImgPackager - packages INPUT type="image"
 */

class CInputImgPackager : public CImagePackager
{
public:
    CInputImgPackager() {}
    virtual ~CInputImgPackager() {}

    virtual HRESULT InitFromCollection(IHTMLElementCollection *pColl,
                                       ULONG *pcElems = NULL);
    
};

/*
 * CBGSoundsPackager - packages background sounds
 */

class CBGSoundsPackager : public CCollectionPackager
{
    public:
        CBGSoundsPackager() {};
        virtual ~CBGSoundsPackager() {};

        virtual HRESULT InitFromCollection(IHTMLElementCollection *pColl,
                                           ULONG *pcElems = NULL);
protected:

    virtual BSTR _GetTargetAttribute(void);

};
     
/*
 * CAnchorAdjustor - modifies anchor hrefs.
 *
 * Makes them absolute if they point out of the collection.
 */

class CAnchorAdjustor : public CCollectionPackager
{
public:
    CAnchorAdjustor(void) {};
    virtual ~CAnchorAdjustor(void) {};
 
    virtual HRESULT InitFromCollection(IHTMLElementCollection *pColl,
                                       ULONG *pcElems = NULL);
protected:

    virtual BSTR _GetTargetAttribute(void);
    virtual HRESULT _PackageElement(CWebArchive *pwa,
                                    IHTMLElement *pElem);
};

/*
 * CAreaAdjustor - modifies AREA hrefs.
 *
 * Makes them absolute if they point out of the collection. Same filter
 * as the anchor adjustor, but different tag.
 */

class CAreaAdjustor : public CAnchorAdjustor
{
public:
    CAreaAdjustor(void) {};
    virtual ~CAreaAdjustor(void) {};
 
    virtual HRESULT InitFromCollection(IHTMLElementCollection *pColl,
                                       ULONG *pcElems = NULL);
};

/*
 * CBaseNeutralizer - resets any and all <BASE> tags to the d.
 *
 * No actual packaging goes on here, but we do remap the 
 * <BASE> href.
 */

class CBaseNeutralizer : public CCollectionPackager
{
public:
    CBaseNeutralizer(void) : m_bstrLocal(NULL), m_pTree(NULL) {};
    virtual ~CBaseNeutralizer(void);

    virtual HRESULT InitFromCollection(IHTMLElementCollection *pColl,
                                       ULONG *pcElems = NULL )
        { return InitFromCollection( pColl, pcElems, NULL ); };
    HRESULT InitFromCollection(IHTMLElementCollection *pColl,
                                       ULONG *pcElems = NULL,
                                       IHTMLDocument2 *pDoc = NULL);
    virtual HRESULT PackageData(CWebArchive *pwa, BOOL *pfCancel = NULL,
                                CThicketProgress *ptp = NULL, ULONG progLow = 0, ULONG progHigh = 100);

protected:

    virtual BSTR _GetTargetAttribute(void);
    virtual HRESULT _PackageElement(CWebArchive *pwa,
                                    IHTMLElement *pElem);

    BSTR m_bstrLocal;
    IMarkupServices *m_pTree;
};

/*
 *  CRelativeURLPackager - abstract base class for packagers
 *      whose element's source attribute returns a relative URL.
 *  This class implements triutils.pp's GetBackgroundImageUrl's
 *  process of attempting to combine the (relative) element URL
 *  with the nearest <BASE> URL. If no <BASE> is availaible, it
 *  uses the document URL.
 *
 *  This class is an abstract base because it does not implement
 *  _GetTargetAttribute. It's implementation of InitFromCollection
 *  isn't very useful and will probably be overridden by derived
 *  classes.
 */

class CRelativeURLPackager : public CCollectionPackager
{
public:
    CRelativeURLPackager(void) : m_pCollBase(NULL), m_cBase(0), m_bstrDocURL(NULL) {};
    virtual ~CRelativeURLPackager(void);
 
    virtual HRESULT InitFromCollection(IHTMLElementCollection *pColl,
                                       ULONG *pcElems = NULL)
    {
        return Init( pColl, pcElems, NULL );
    }

    virtual HRESULT Init(IHTMLElementCollection *pColl,
                         ULONG *pcElems,
                         IHTMLDocument2 *pDoc);

protected:

    virtual HRESULT _GetElementURL(IHTMLElement *pElem, BSTR *pbstrURL);

    IHTMLElementCollection  *m_pCollBase; // collection of BASE tags used to complete URLs
    ULONG                   m_cBase;
    BSTR                    m_bstrDocURL;
};

/*
 * CBackgroundPackager - packages the background of BODY, TABLE, TD, and TH.
 *
 * These three tags have a common target attribute.
 */

class CBackgroundPackager : public CRelativeURLPackager
{
public:
    CBackgroundPackager(void) {};
    ~CBackgroundPackager(void) {};
 
    virtual HRESULT PackageData(CWebArchive *pwa, BOOL *pfCancel,
                                CThicketProgress *ptp = NULL, ULONG progLow = 0, ULONG progHigh = 100);
protected:

    virtual BSTR _GetTargetAttribute(void);
};

/*
 * CDynSrcPackager - packages the dynsrc of IMG and INPUT.
 *
 * These two tags have a common target attribute.
 */

class CDynSrcPackager : public CRelativeURLPackager
{
public:
    CDynSrcPackager(void) {};
    ~CDynSrcPackager(void) {};
 
    virtual HRESULT PackageData(CWebArchive *pwa, BOOL *pfCancel,
                                CThicketProgress *ptp = NULL, ULONG progLow = 0, ULONG progHigh = 100);
protected:

    virtual BSTR _GetTargetAttribute(void);
};


/*
 * CScriptPackager - packages the dynsrc of IMG and INPUT.
 *
 * These two tags have a common target attribute.
 */

class CScriptPackager : public CRelativeURLPackager
{
public:
    CScriptPackager(void) : m_pCollScripts(NULL) {};
    ~CScriptPackager(void) { if (m_pCollScripts) m_pCollScripts->Release(); };
 
    virtual HRESULT PackageData(CWebArchive *pwa, BOOL *pfCancel = NULL,
                                CThicketProgress *ptp = NULL, ULONG progLow = 0, ULONG progHigh = 100)
    { 
        return _PackageData( pwa, m_pCollScripts, pfCancel, ptp, progLow, progHigh );
    }

    virtual HRESULT Init(IHTMLElementCollection *pColl,
                         ULONG *pcElems = NULL,
                         IHTMLDocument2 *pDoc = NULL);
protected:

    virtual BSTR _GetTargetAttribute(void);

    IHTMLElementCollection *m_pCollScripts;

};


/*
 * CFramesPackager - packages the <FRAME> and <IFRAME> sub-documents.
 *
 *  This process is recursive, so all nested frames will be packaged.
 */

class CFramesPackager : public CRelativeURLPackager
{
public:
    CFramesPackager(void) :
        m_pCollFrames(NULL),
        m_pframes2(NULL),
        m_cFrames(0),
        m_iFrameCur(0),
        m_pfCancel(0),
        m_ptp(NULL),
        m_uLow(0),
        m_uHigh(0),
        m_uRangeDoc(0) {};

        virtual ~CFramesPackager(void)
            { 
                if (m_pCollFrames) m_pCollFrames->Release();
                if (m_pframes2) m_pframes2->Release();
            };
 
    virtual HRESULT InitFromCollection(IHTMLElementCollection *pColl,
                                       ULONG *pcElems = NULL)
    {
        return CRelativeURLPackager::Init( pColl, pcElems, NULL );
    }

    virtual HRESULT Init(IHTMLElementCollection *pColl,
                         ULONG *pcElems,
                         IHTMLDocument2 *pDoc,
                         IHTMLDocument2 *pDocDesign,
                         CDocumentPackager *pdp);

    virtual HRESULT PackageData(CWebArchive *pwa, BOOL *pfCancel,
                                CThicketProgress *ptp = NULL, ULONG progLow = 0, ULONG progHigh = 100);

protected:

    virtual BSTR _GetTargetAttribute(void);
    virtual HRESULT _PackageElement(CWebArchive *pwa,
                                    IHTMLElement *pElem);

    IHTMLElementCollection *m_pCollFrames;
    IHTMLFramesCollection2 *m_pframes2;
    ULONG   m_cFrames;
    ULONG   m_iFrameCur;
    BOOL    *m_pfCancel;
    CThicketProgress*    m_ptp;
    ULONG   m_uLow;
    ULONG   m_uHigh;
    ULONG   m_uRangeDoc;
    CDocumentPackager *m_pdp;
};

/*
 * CSSPackager - packages imported stylesheets.
 *
 *  Stylesheets have a different OM than document elements, so
 *  we have a packager that looks similar, but works differently
 *  than the other element packagers.
 *
 *  We derive from CRelativeURLPackager for the convenience of 
 *  its Init method and <BASE> collection functionality, which
 *  we also need because the hrefs in style sheets can be relative.
 *
 *  Since we aren't actually packaging elments, the _GetTargetAttribute()
 *  implementation is a formality to satisfy the abstract base class.
 */

class CSSPackager : public CRelativeURLPackager
{
public:
    CSSPackager(void) : m_pDoc(NULL) {};
    ~CSSPackager(void) {};

    HRESULT Init( IHTMLElementCollection *pColl,
                         ULONG *pcElems = NULL,
                         IHTMLDocument2 *pDoc = NULL);

    HRESULT PackageStyleSheets(IHTMLDocument2 *pDoc2, CWebArchive *pwa);

protected:

    BSTR _GetTargetAttribute(void) { ASSERT(FALSE); return NULL; };

    HRESULT _PackageSSCollection(IHTMLStyleSheetsCollection *pssc,
                                         CWebArchive *pwa);
    HRESULT _PackageSS(IHTMLStyleSheet *pss, CWebArchive *pwa);

    IHTMLDocument2 *m_pDoc;
};


// possible hash-table sizes, chosen from primes not close to powers of 2
static const DWORD s_rgPrimes[] = { 29, 53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593 };

/*
*  class implementation
*/

/*
*  CWebArchive ##################################################
*/

CWebArchive::CWebArchive(CThicketProgress *ptp)
{
    m_lpstrDoc = NULL;
    m_lpstrSafeDoc = NULL;
    
    m_cBins = 0;
    m_rgBins = NULL;

    m_ptp = ptp;
}


CWebArchive::~CWebArchive(void)
{
    CHashEntry *phe, *pheTemp;
    
    if (m_lpstrDoc != NULL)
        LocalFree( m_lpstrDoc );

    if (m_lpstrSafeDoc != NULL)
        LocalFree( m_lpstrSafeDoc );
        
    // m_ptp is on loan to us, don't delete it
    
    for (DWORD dw = 0; dw < m_cBins; dw++)
    {
        if (m_rgBins[dw].m_pheNext)
        {
            phe = m_rgBins[dw].m_pheNext;
            while (phe)
            {
                pheTemp = phe;
                phe = phe->m_pheNext;
                delete pheTemp;
            }
        }
    }
    delete[] m_rgBins;
}


HRESULT
CWebArchive::Init( LPCTSTR lpstrDoc, DWORD dwHashSize )
{
    HRESULT hr = S_OK;
    int     i = 0;
    
    m_lpstrDoc = StrDup(lpstrDoc);

    // check for replacement of old file
    if (PathFileExists(m_lpstrDoc))
        hr = _BackupOldFile();
    if (FAILED(hr))
        goto error; 
    
    // Initialize the hash table.
    for (i = 0; i < (ARRAYSIZE(s_rgPrimes) - 1) && s_rgPrimes[i] < dwHashSize; i++);
    ASSERT(s_rgPrimes[i] >= dwHashSize || i == (ARRAYSIZE(s_rgPrimes)-1));
    m_cBins = s_rgPrimes[i];
    
    m_rgBins = new CHashEntry[m_cBins];
    if (m_rgBins==NULL)
        hr = E_OUTOFMEMORY;
    
error:
    
    RRETURN(hr);
}


HRESULT
CWebArchive::Commit()
{
    // clean up old version of file
    if (m_lpstrSafeDoc)
        DeleteFile(m_lpstrSafeDoc);

    return S_OK;
}

HRESULT
CWebArchive::Revert()
{
    if (m_lpstrSafeDoc)
    {
         if (!MoveFileEx(m_lpstrSafeDoc, m_lpstrDoc, MOVEFILE_REPLACE_EXISTING))
        {
            ASSERT(FALSE);
            // We shouldn't get into this situtation because we've pre-checked that
            // the original file is not read-only.
            DeleteFile(m_lpstrSafeDoc);
        }
    }

   return S_OK;
}

CWebArchive::ThURLType
CWebArchive::_GetURLType( BSTR bstrURL )
{
//    _tcsncmpi(bstrURL, 4, _T("http",4)
    if ( bstrURL[0] == TEXT('h') &&
         bstrURL[1] == TEXT('t') &&
         bstrURL[2] == TEXT('t') &&
         bstrURL[3] == TEXT('p') )
        return thurlHttp;
    else if ( bstrURL[0] == TEXT('f') &&
              bstrURL[1] == TEXT('i') &&
              bstrURL[2] == TEXT('l') &&
              bstrURL[3] == TEXT('e') )
        return thurlFile;
    else
        return thurlMisc;
}



HRESULT
CWebArchive::_Insert(BSTR bstrI, BSTR bstrThicket, CHashEntry **pphe )
{
    HRESULT hr = S_OK;

    CHashEntry *phe = &m_rgBins[Hash(bstrI)];
    
    ASSERT(pphe != NULL);

    *pphe = NULL;

 
    if (phe->m_bstrKey)
    {        
        CHashEntry *pheNew = new CHashEntry;
        
        if (pheNew==NULL)
            return E_OUTOFMEMORY;

        if (pheNew->SetKey(bstrI) && pheNew->SetValue(bstrThicket))
            *pphe = pheNew;
        else
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        pheNew->m_pheNext = phe->m_pheNext;
        phe->m_pheNext = pheNew;
        phe = pheNew;
    } 
    else if (phe->SetKey(bstrI) && phe->SetValue(bstrThicket))
        *pphe = phe;
    else
        hr = E_OUTOFMEMORY;
        
Cleanup:

    return hr;
}

HRESULT
CWebArchive::Find(BSTR bstrF, CHashEntry **pphe)
{
    CHashEntry *phe = &m_rgBins[Hash(bstrF)];

    if (!pphe)
        return E_POINTER;

    *pphe = NULL;

    if (phe->m_bstrKey)
    {
        do
        {
            if (!StrCmpW(phe->m_bstrKey, bstrF))
            {
                ASSERT(phe->m_bstrValue!=NULL);
                *pphe = phe;
                return NOERROR;
            }
            phe = phe->m_pheNext;
        }
        while (phe);
    }
    return E_INVALIDARG;
}


DWORD
CWebArchive::Hash(BSTR bstr)
{
    DWORD h = 0;
    WCHAR *pwch = bstr;
    
    while (*pwch)
        h = ((h << 4) + *pwch++ + (h >> 28));
    return (h % m_cBins);
}

HRESULT
CWebArchive::_BackupOldFile()
{
    HRESULT hr = S_OK;
    TCHAR   chT;
    LPTSTR  lpstrT;
    TCHAR   szT[MAX_PATH];
    DWORD   dwAttrib = GetFileAttributes(m_lpstrDoc);

    if (dwAttrib & FILE_ATTRIBUTE_READONLY)
        return E_ACCESSDENIED;

    lpstrT = PathFindFileName(m_lpstrDoc);
    ASSERT(lpstrT);

    lpstrT--; // back up to the slash
    chT = *lpstrT;
    *lpstrT = 0;
    if (GetTempFileName( m_lpstrDoc, &lpstrT[1], 0,szT ))
    {
        *lpstrT = chT;
        if (CopyFile(m_lpstrDoc, szT, FALSE))
        {
            int cchSafeDoc = lstrlen(szT) + 1;
            m_lpstrSafeDoc = (LPTSTR)LocalAlloc( LMEM_FIXED, sizeof(TCHAR) * cchSafeDoc);
            if (m_lpstrSafeDoc)
                StrCpyN(m_lpstrSafeDoc, szT, cchSafeDoc);
            else
            {
                hr = E_OUTOFMEMORY;
                DeleteFile(szT);
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto error;
        }
    }
    else
    {

        hr = HRESULT_FROM_WIN32(GetLastError());
        goto error;
    }

error:
    *lpstrT = chT;
    RRETURN(hr);
}

/*
*  CThicketArchive ##################################################
*/

CThicketArchive::CThicketArchive(CThicketProgress *ptp) : CWebArchive(ptp)
{
    m_lpstrFilesDir = NULL;
    m_lpstrFilesDirName = NULL;
    m_lpstrSafeDir = NULL;
    m_fFilesDir = FALSE;   // TRUE when m_lpstrFilesDir has been created
}


CThicketArchive::~CThicketArchive(void)
{    
    if (m_lpstrFilesDir != NULL)
        LocalFree( m_lpstrFilesDir );

    if (m_lpstrSafeDir != NULL)
        LocalFree( m_lpstrSafeDir );
    
    // m_lpstrFilesDirName points into m_lpstrFilesDir
}


HRESULT
CThicketArchive::Init( LPCTSTR lpstrDoc, DWORD dwHashSize )
{
    HRESULT hr = CWebArchive::Init( lpstrDoc, dwHashSize );
    int     i = 0;
    TCHAR   chT;
    LPTSTR  lpstrT;
    TCHAR   szFmt[MAX_PATH];
    int     cch;
    
    if (FAILED(hr))
        goto error;  
    
    // Build the path to the directory for stored files, like 'Document1 files'.
    lpstrT = PathFindExtension(m_lpstrDoc);
    chT = *lpstrT;
    *lpstrT = 0;
    MLLoadString(IDS_THICKETDIRFMT, szFmt, ARRAYSIZE(szFmt));
    cch = lstrlen(m_lpstrDoc) + lstrlen(szFmt) + 1;
    m_lpstrFilesDir = (LPTSTR)LocalAlloc( LMEM_FIXED, sizeof(TCHAR) * cch );
    if (m_lpstrFilesDir==NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    StrCpyN( m_lpstrFilesDir, m_lpstrDoc, cch);
    StrCatBuff( m_lpstrFilesDir, szFmt, cch );

    *lpstrT = chT;  

    // make m_lpstrFilesDirName point to the last component of m_lpstrFilesDir
    for ( i = lstrlen(m_lpstrFilesDir) - 1; i > 0 && m_lpstrFilesDirName == NULL; i-- )
    {
        if ( m_lpstrFilesDir[i-1] == FILENAME_SEPARATOR )
            m_lpstrFilesDirName = &m_lpstrFilesDir[i];
    }

    // check to see if the files dir already exists. If it does, rename the original.
    if (PathFileExists(m_lpstrFilesDir))
        hr = _BackupOldDirectory();
    if (FAILED(hr))
        goto error;
    
error:
    
    RRETURN(hr);
}


HRESULT
CThicketArchive::AddURL( BSTR bstrURL, CHashEntry **pphe )
{
    HRESULT hr;
    
    hr = THR(Find(bstrURL, pphe));
    
    if (FAILED(hr))
    {
        // first, lets put our document dir in place, if it isn't already
        if (!m_fFilesDir)
            m_fFilesDir = CreateDirectory(m_lpstrFilesDir,NULL);
        
        if (m_fFilesDir)
        {
            switch (_GetURLType(bstrURL))
            {
            case thurlMisc:
                hr = _PersistMiscURL(bstrURL, pphe);
                break;

            case thurlHttp:
                hr = _PersistHttpURL(bstrURL, pphe);
                break;

            case thurlFile:
                hr = _PersistFileURL(bstrURL, pphe);
                break;
            }
        }
        else
            hr = E_FAIL;
    }
    
    RRETURN(hr);
}

HRESULT
CThicketArchive::AddFrameOrStyleEntry( BSTR bstrURL, CHashEntry **pphe, LPTSTR lpstrFrameDoc )
{
    HRESULT hr;
    
    hr = THR(Find(bstrURL, pphe)); // there's always a slim chance we're reusing a frame.
    
    if (FAILED(hr))
    {
        // first, lets put our document dir in place, if it isn't already
        if (!m_fFilesDir)
            m_fFilesDir = CreateDirectory(m_lpstrFilesDir,NULL);
        
        if (m_fFilesDir)
        {
            switch (_GetURLType(bstrURL))
            {
            case thurlMisc:
                //hr = _AddMiscEntry(bstrURL, pphe, lpstrFrameDoc);
                // It would be nice if we could just _AddMiscEntry, but if set a frame src
                // to one of the temp files that this produces, we get a 'Do you want to open'
                // prompt, so instead, we'll just keep this funky protocol URL.
                hr = CWebArchive::_Insert( bstrURL, bstrURL, pphe );
                lpstrFrameDoc[0] = 0; // shouldn't be used, anyway
                hr = S_FALSE;         // I told him we all-reddy got one! <snicker>
                break;

            case thurlHttp:
                hr = _AddHttpEntry(bstrURL, pphe, lpstrFrameDoc);
                break;

            case thurlFile:
                hr = _AddFileEntry(bstrURL, pphe, lpstrFrameDoc);
                break;
            }

            if (m_ptp)
                m_ptp->SetSaving( PathFindFileName(lpstrFrameDoc), m_lpstrFilesDir );

        }
        else
        {
            hr = (GetLastError() == ERROR_DISK_FULL) ? (HRESULT_FROM_WIN32(ERROR_DISK_FULL))
                                                     : (E_FAIL);
        }
    }
    else
    {
        LPTSTR lpszThicket;
        lpszThicket = (*pphe)->m_bstrValue;
        PathCombine( lpstrFrameDoc, m_lpstrFilesDir, lpszThicket );
        hr = S_FALSE;
    }
    
    return hr; // no RRETURN - may return S_FALSE
}

HRESULT
CThicketArchive::Commit()
{
    CWebArchive::Commit();

    // clean up obsolete files dir.
    if (m_lpstrSafeDir)
    {
        _RemoveOldDirectoryAndChildren(m_lpstrSafeDir);
    }

    return S_OK;
}

HRESULT
CThicketArchive::Revert()
{
    // clean up file dir

    _RemoveOldDirectoryAndChildren(m_lpstrFilesDir);

    // restore old files dir.
    if (m_lpstrSafeDir)
        MoveFile(m_lpstrSafeDir,m_lpstrFilesDir);
    
    return CWebArchive::Revert();;
}

HRESULT CThicketArchive::ArchiveDocumentText(IHTMLDocument2 *pDoc, UINT cpDoc, BOOL fFrameDoc)
{
    HRESULT             hr = S_OK;
    IPersistStreamInit* ppsi = NULL;
    IStream*            pstm = NULL;

    hr = SHCreateStreamOnFile(m_lpstrDoc, STGM_WRITE | STGM_CREATE, &pstm);
    if (SUCCEEDED(hr))
    {
        hr = pDoc->QueryInterface(IID_IPersistStreamInit, (void**)&ppsi);
        if (SUCCEEDED(hr))
        {          
            hr = _ApplyMarkOfTheWeb( pDoc, pstm, cpDoc == CP_UNICODE );

            if ( SUCCEEDED(hr) )
                hr = ppsi->Save(pstm, FALSE);
        }
    }
   
    ReleaseInterface(ppsi);
    ReleaseInterface(pstm);
    
    RRETURN(hr);
}

HRESULT CThicketArchive::ArchiveCSSText( BSTR bstrCSSUrl, LPCSTR lpszSSText, LPCTSTR lpszStyleDoc )
{
    HRESULT hr = S_OK;
    HANDLE  hfile;

    hfile = CreateFile( lpszStyleDoc, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hfile!=INVALID_HANDLE_VALUE) 
    {
        ULONG   cbWrite, cbWritten;

        cbWrite = lstrlenA(lpszSSText);
        if (!WriteFile( hfile, lpszSSText, cbWrite, &cbWritten, NULL ))
             hr = HRESULT_FROM_WIN32(GetLastError());

        CloseHandle(hfile);
    }
    else
        hr = HRESULT_FROM_WIN32(hr);

    return hr;
}

EXTERN_C HRESULT GetMarkOfTheWeb( LPCSTR, LPCSTR, DWORD, LPSTR *);

HRESULT CThicketArchive::_ApplyMarkOfTheWeb( IHTMLDocument2 *pDoc, LPSTREAM pstm, BOOL fUnicode )
{
    HRESULT hr;
    IInternetSecurityManager *pism = NULL;
    DWORD   dwZone;
    BSTR    bstrURL = NULL;

    hr = pDoc->get_URL( &bstrURL );
    if (FAILED(hr))
        return hr;

    // We only want to mark the document if it isn't already coming from the local
    // file system. If  ( minus the mark ) the file is in the local machine zone,
    // then it was made here, saved with a mark, or created outside our control.
    // If it was saved with a mark, then we want to leave that in place, rather
    // than mark it with the local copy's file: URL.

    hr = CoInternetCreateSecurityManager( NULL, &pism, 0 );
    if (SUCCEEDED(hr) && 
        SUCCEEDED(pism->MapUrlToZone( bstrURL, &dwZone, MUTZ_NOSAVEDFILECHECK)) &&
        dwZone != URLZONE_LOCAL_MACHINE )
    {
        LPSTR   pszMark;
        DWORD   cchURL = WideCharToMultiByte(CP_ACP, 0, bstrURL, -1, NULL, 0, NULL, NULL);
        LPSTR   pszURL = new CHAR[cchURL];

        if (pszURL)
        {
            if (WideCharToMultiByte(CP_ACP, 0, bstrURL, -1, pszURL, cchURL, NULL, NULL))
            {
                int   cch = lstrlen(m_lpstrDoc) + 1;
                LPSTR psz = new char[cch];

                if (psz)
                {
                    SHUnicodeToAnsi(m_lpstrDoc, psz, cch);
                    
                    hr = GetMarkOfTheWeb( pszURL, psz, 0, &pszMark);

                    delete [] psz;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }


                IMarkupServices              *pims = NULL;
                IMarkupPointer               *pimp = NULL;
                IMarkupContainer             *pimc = NULL;
                IHTMLElement                 *pihe = NULL;
                IHTMLElement                 *piheBody = NULL;
                IDispatch                    *pidDocument = NULL;
                IHTMLCommentElement          *pihce = NULL;
                LPWSTR                        pwszMark = NULL;
                BSTR                          bstrMark = NULL;

                hr = pDoc->QueryInterface(IID_IMarkupServices, (void **)&pims);

                if (SUCCEEDED(hr)) {
                    hr = pims->CreateElement(TAGID_COMMENT, NULL, &pihe);

                    if (SUCCEEDED(hr)) {
                        hr = pihe->QueryInterface(IID_IHTMLCommentElement, (void **)&pihce);
                    }

                    if (SUCCEEDED(hr)) {
                        int cbWrite = 0;
                        int cchMark = MultiByteToWideChar(CP_ACP, 0, pszMark, -1, NULL, 0);

                        // cchMark includes the null terminator.
                    
                        pwszMark = new WCHAR[cchMark];
                        if ( pwszMark != NULL )
                        {
                            MultiByteToWideChar( CP_ACP, 0, pszMark, -1, pwszMark, cchMark);
                            cbWrite = (cchMark - 1) * sizeof(WCHAR);
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                        }

                        if (SUCCEEDED(hr))
                        {
                            // force <!-- ... --> style comment
                            hr = pihce->put_atomic(1);
                        }

                    }

                    if (SUCCEEDED(hr)) {
                        bstrMark = SysAllocString(pwszMark);
                        if (NULL != bstrMark)
                        {
                            hr = pihce->put_text(bstrMark);
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                        }
                        
                    }

                    if (SUCCEEDED(hr)) {
                        hr = pims->CreateMarkupPointer(&pimp);
                    }

                    if (SUCCEEDED(hr)) {
                        hr = pDoc->get_body(&piheBody);
                    }

                    if (SUCCEEDED(hr)) {
                        hr = piheBody->get_document(&pidDocument);
                    }

                    if (SUCCEEDED(hr)) {
                        hr = pidDocument->QueryInterface(IID_IMarkupContainer, (void **)&pimc);
                    }

                    if (SUCCEEDED(hr)) {
                        // Move to beginning of doc and insert it
                        hr = pimp->MoveToContainer(pimc, TRUE);

                        if (SUCCEEDED(hr)) {
                            hr = pims->InsertElement(pihe, pimp, pimp);
                        }
                    }
                }

                SAFERELEASE(pims);
                SAFERELEASE(pimc);
                SAFERELEASE(pihe);
                SAFERELEASE(pimp);
                SAFERELEASE(piheBody);
                SAFERELEASE(pidDocument);
                SAFERELEASE(pihce);

                if (bstrMark)
                {
                    SysFreeString(bstrMark);
                }

                if (pwszMark)
                {
                    delete[] pwszMark;
                }
            }
            else
                 hr = HRESULT_FROM_WIN32(GetLastError());

            delete[] pszURL;
        }
        else
            hr = E_OUTOFMEMORY;
    }



    ReleaseInterface(pism);
    if (bstrURL)
        SysFreeString(bstrURL);

    return hr;
}

HRESULT
CThicketArchive::_AddHttpEntry( BSTR bstrURL, CHashEntry **pphe, LPTSTR lpstrDstFile, LPTSTR lpstrSrcFile )
{
    HRESULT hr;
    TCHAR   szCacheFile[MAX_PATH];
    LPTSTR  lpszDst;
    LPTSTR  lpszFile;
    int     cchFile;
    LPTSTR  lpszURL;

    lpszURL = bstrURL;

    hr = URLDownloadToCacheFile(NULL, lpszURL, szCacheFile,
                                ARRAYSIZE(szCacheFile), BINDF_FWD_BACK,
                                NULL);
    if (FAILED(hr))
        goto Cleanup;

    if (lpstrSrcFile)
        StrCpyN(lpstrSrcFile, szCacheFile, MAX_PATH);

    PathUndecorate( szCacheFile );

    lpszFile = PathFindFileName( szCacheFile );
    ASSERT(lpszFile != NULL);

    cchFile = ARRAYSIZE(szCacheFile) - (int)(lpszFile-szCacheFile);

    hr = _Insert( bstrURL, lpszFile, cchFile, pphe ); 

    lpszDst = PathCombine( lpstrDstFile, m_lpstrFilesDir, lpszFile );
    ASSERT( lpszDst );

Cleanup:

    RRETURN(hr);
}


HRESULT
CThicketArchive::_AddFileEntry( BSTR bstrURL, CHashEntry **pphe, LPTSTR lpstrDstFile, LPTSTR lpstrSrcFile )
{
    HRESULT hr;
    LPTSTR  lpszDst;
    LPTSTR  lpszFile;
    int     cchFile;
    LPTSTR  lpszPath;
    WCHAR   rgchUrlPath[MAX_PATH];
    DWORD   dwLen;

    dwLen = ARRAYSIZE(rgchUrlPath);

    hr = PathCreateFromUrlW(bstrURL, rgchUrlPath, &dwLen, 0);
    if (FAILED(hr))
        return E_FAIL;

    lpszPath = rgchUrlPath;

    if (lpstrSrcFile)
        StrCpyN( lpstrSrcFile, lpszPath, MAX_PATH );

    lpszFile = PathFindFileName( lpszPath );
    ASSERT(lpszFile != NULL);
    cchFile = ARRAYSIZE(rgchUrlPath) - (int)(lpszFile-rgchUrlPath);

    hr = THR(_Insert( bstrURL, lpszFile, cchFile, pphe )); 

    lpszDst = PathCombine( lpstrDstFile, m_lpstrFilesDir, lpszFile );
    ASSERT( lpszDst );

    RRETURN(hr);
}

HRESULT
CThicketArchive::_AddMiscEntry( BSTR bstrURL, CHashEntry **pphe, LPTSTR lpstrDstFile, int cchDstFile )
{
    HRESULT hr;
    TCHAR   szT[MAX_PATH];
    LPTSTR  lpszPrefix;
    LPTSTR  lpszDst;

    lpszPrefix = bstrURL;

    if (GetTempFileName( m_lpstrFilesDir, lpszPrefix, 0,szT ))
    {
        lpszDst = PathCombine( lpstrDstFile, m_lpstrFilesDir, szT );
        ASSERT(lpszDst);

        LPTSTR pszFile = PathFindFileName(lpstrDstFile);
        hr = THR(_Insert( bstrURL, pszFile, cchDstFile - (int)(pszFile-lpstrDstFile), pphe ));
    }
    else
        hr = HRESULT_FROM_WIN32(GetLastError());

    RRETURN(hr);
}

HRESULT
CThicketArchive::_PersistHttpURL( BSTR bstrURL, CHashEntry **pphe )
{
    HRESULT hr;
    TCHAR   szDst[MAX_PATH];
    TCHAR   szSrc[MAX_PATH];

    hr = THR(_AddHttpEntry(  bstrURL, pphe, szDst, szSrc ));
    if (FAILED(hr))
        goto Error;

    if (m_ptp)
        m_ptp->SetSaving( PathFindFileName(szSrc), m_lpstrFilesDir );

    if (!CopyFile(szSrc,szDst, FALSE))
        hr = HRESULT_FROM_WIN32(GetLastError());

Error:
    RRETURN(hr);
}

HRESULT
CThicketArchive::_PersistFileURL( BSTR bstrURL, CHashEntry **pphe )
{
    HRESULT hr;
    TCHAR   szDst[MAX_PATH];
    TCHAR   szSrc[MAX_PATH];

    hr = THR(_AddFileEntry(  bstrURL, pphe, szDst, szSrc ));
    if (FAILED(hr))
        goto Error;

    if (m_ptp)
        m_ptp->SetSaving( PathFindFileName(szSrc), m_lpstrFilesDir );

    if (!CopyFile(szSrc,szDst, FALSE))
        hr = HRESULT_FROM_WIN32(GetLastError());

Error:
    RRETURN(hr);
}

HRESULT
CThicketArchive::_PersistMiscURL( BSTR bstrURL, CHashEntry **pphe )
{
    HRESULT hr;
    TCHAR   szDst[MAX_PATH];
    LPTSTR  lpszURL;

    lpszURL = bstrURL;

    hr = THR(_AddMiscEntry(  bstrURL, pphe, szDst, ARRAYSIZE(szDst) ));
    if (FAILED(hr))
        goto Error;

    if (m_ptp)
        m_ptp->SetSaving( PathFindFileName(szDst), m_lpstrFilesDir );

    hr = URLDownloadToFile(NULL, lpszURL, szDst,0, NULL);

Error:
    RRETURN(hr);
}


HRESULT
CThicketArchive::_Insert(BSTR bstrI, LPTSTR lpszFile, int cchFile, CHashEntry **pphe )
{
    HRESULT hr = S_OK;
    BSTR    bstrThicket = NULL;
    TCHAR   buf[MAX_PATH];
    int     i = 0;

    CHashEntry *phe = &m_rgBins[Hash(bstrI)];
    
    ASSERT(pphe != NULL);

    *pphe = NULL;

    if (lstrlen(m_lpstrFilesDir) + lstrlen(lpszFile) + 1 < MAX_PATH)
        wnsprintf( buf, ARRAYSIZE(buf), TEXT("%s") TEXT(FILENAME_SEPARATOR_STR) TEXT("%s"), m_lpstrFilesDir, lpszFile );
    else
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // Defend against bug 18160 - collision of file names in the thicket.
    if ( PathFileExists(buf) )
    {
        TCHAR *pszExt = PathFindExtension(lpszFile);
        int   i = 0;

        // chop the file name into name and extenstion
        if ( pszExt )
        {
            *pszExt = 0;
            pszExt++;
        }

        do
        {
            i++;

            if ( pszExt )
                wnsprintf( buf, ARRAYSIZE(buf), TEXT("%s") TEXT(FILENAME_SEPARATOR_STR) TEXT("%s(%d).%s"), m_lpstrFilesDir, lpszFile, i, pszExt );
            else
                wnsprintf( buf, ARRAYSIZE(buf), TEXT("%s") TEXT(FILENAME_SEPARATOR_STR) TEXT("%s(%d)"), m_lpstrFilesDir, lpszFile, i );

        } while ( PathFileExists(buf) && i < 1000 );


        // deviously rewrite the file name for the caller
        StrCpyN( lpszFile, PathFindFileName(buf), cchFile );
    }
    else
        wnsprintf( buf, ARRAYSIZE(buf), TEXT("%s/%s"), m_lpstrFilesDirName, lpszFile );
    
    bstrThicket = SysAllocString(buf);
    if (bstrThicket == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if (phe->m_bstrKey)
    {        
        CHashEntry *pheNew = new CHashEntry;
        
        if (pheNew==NULL)
            return E_OUTOFMEMORY;

        if (pheNew->SetKey(bstrI) && pheNew->SetValue(bstrThicket))
            *pphe = pheNew;
        else
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        pheNew->m_pheNext = phe->m_pheNext;
        phe->m_pheNext = pheNew;
        phe = pheNew;
    } 
    else if (phe->SetKey(bstrI) && phe->SetValue(bstrThicket))
        *pphe = phe;
    else
        hr = E_OUTOFMEMORY;
        
Cleanup:
    if (bstrThicket)
        SysFreeString(bstrThicket);

    return hr;
}


HRESULT 
CThicketArchive::_BackupOldDirectory()
{
    int n = 1;
    HRESULT hr = S_OK;
    TCHAR szFmt[MAX_PATH];

    // Do we need to do this under critical section?
    MLLoadString(IDS_THICKETTEMPFMT, szFmt, ARRAYSIZE(szFmt));

    do {
        if (m_lpstrSafeDir)
        {
            LocalFree( m_lpstrSafeDir );
            m_lpstrSafeDir = NULL;
        }

        if (n > 100)    // avoid infinite loop!
            break;

        DWORD cchSafeDir = lstrlen(m_lpstrFilesDir) + lstrlen(szFmt) + 1;
        m_lpstrSafeDir = (LPTSTR)LocalAlloc( LMEM_FIXED, sizeof(TCHAR) * cchSafeDir );
        if (m_lpstrSafeDir!=NULL)
        {
            wnsprintf( m_lpstrSafeDir, cchSafeDir, szFmt, m_lpstrFilesDir, n++ );
        }
        else
            hr = E_OUTOFMEMORY;

    } while (SUCCEEDED(hr) && GetFileAttributes(m_lpstrSafeDir) != -1 && n < 1000);

    // rename the old version of the supporting files directory
    if (SUCCEEDED(hr) && !MoveFile(m_lpstrFilesDir, m_lpstrSafeDir))
    {
        LocalFree( m_lpstrSafeDir );
        m_lpstrSafeDir = NULL;

        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    RRETURN(hr);
}

HRESULT
CThicketArchive::_RemoveOldDirectoryAndChildren( LPCWSTR pwzDir )
{
    HRESULT hr = S_OK;
    HANDLE hf = INVALID_HANDLE_VALUE;
    WCHAR wzBuf[MAX_PATH];
    WIN32_FIND_DATAW fd;

    if (!pwzDir)
        goto Exit;

    if (RemoveDirectoryW(pwzDir))
        goto Exit;

    // FindNextFile returns 120, not implemented on OSR2, so we'll have to do all
    // this stuff multibyte

    StrCpyNW(wzBuf, pwzDir, ARRAYSIZE(wzBuf));
    StrCatBuffW(wzBuf, FILENAME_SEPARATOR_STR_W L"*", ARRAYSIZE(wzBuf));

    if ((hf = FindFirstFileW(wzBuf, &fd)) == INVALID_HANDLE_VALUE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    do {

        if ( (StrCmpW(fd.cFileName, L".") == 0) ||
             (StrCmpW(fd.cFileName, L"..") == 0))
            continue;

        wnsprintfW(wzBuf, ARRAYSIZE(wzBuf), L"%s" FILENAME_SEPARATOR_STR_W L"%s", pwzDir, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

            SetFileAttributesW(wzBuf, 
                FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_NORMAL);

            if (FAILED((hr=_RemoveOldDirectoryAndChildren(wzBuf)))) {
                goto Exit;
            }

        } else {

            SetFileAttributesW(wzBuf, FILE_ATTRIBUTE_NORMAL);
            if (!DeleteFileW(wzBuf)) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;
            }
        }


    } while (FindNextFileW(hf, &fd));


    if (GetLastError() != ERROR_NO_MORE_FILES) {

        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    if (hf != INVALID_HANDLE_VALUE) {
        FindClose(hf);
        hf = INVALID_HANDLE_VALUE;
    }

    // here if all subdirs/children removed
    /// re-attempt to remove the main dir
    if (!RemoveDirectoryW(pwzDir)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

Exit:

    if (hf != INVALID_HANDLE_VALUE)
        FindClose(hf);

    RRETURN(hr);
}



/*
 *  CMHTMLArchive ##################################################
 */

CMHTMLArchive::CMHTMLArchive(CThicketProgress *ptp) :
    CWebArchive(ptp),
    m_hBodyAlt(NULL),
    m_pimm(NULL)
{
}


CMHTMLArchive::~CMHTMLArchive(void)
{  
    ReleaseInterface(m_pimm);
}


HRESULT
CMHTMLArchive::Init( LPCTSTR lpstrDoc, DWORD dwHashSize )
{
    HRESULT hr = S_OK;
  
    MimeOleSetCompatMode(MIMEOLE_COMPAT_MLANG2);
  
    if ( m_pimm == NULL )
    {
        hr = CWebArchive::Init( lpstrDoc, dwHashSize );
        if (SUCCEEDED(hr))
            hr = MimeOleCreateMessage(NULL, &m_pimm);
    }

    RRETURN(hr);
}


HRESULT
CMHTMLArchive::AddURL( BSTR bstrURL, CHashEntry **pphe )
{
    HRESULT hr;
    
    hr = THR(Find(bstrURL, pphe));
    
    if (FAILED(hr))
    {       
        IStream     *pstm = NULL;
        CHAR        szUrl[INTERNET_MAX_URL_LENGTH];
        WCHAR       wzArchiveText[MAX_SAVING_STATUS_TEXT + 1];
        WCHAR       wzBuf[INTERNET_MAX_URL_LENGTH + MAX_SAVING_STATUS_TEXT + 1];
        LPSTR       lpszCID=0;
        DWORD       dwAttach = URL_ATTACH_SET_CNTTYPE; 

        SHUnicodeToAnsi(bstrURL, szUrl, ARRAYSIZE(szUrl));

        // hack: if it's an MHTML: url then we have to fixup to get the cid:
        if (StrCmpNIA(szUrl, "mhtml:", 6)==0)
        {
            LPSTR lpszBody;

            if (SUCCEEDED(MimeOleParseMhtmlUrl(szUrl, NULL, &lpszBody)))
            {
                StrCpyNA(szUrl, lpszBody, INTERNET_MAX_URL_LENGTH);
                CoTaskMemFree(lpszBody);
            }
        }

        MLLoadStringW(IDS_SAVING_STATUS_TEXT, wzArchiveText,
                      ARRAYSIZE(wzArchiveText));

        wnsprintfW(wzBuf, ARRAYSIZE(wzBuf), L"%ws: %ws", wzArchiveText, bstrURL);
        m_ptp->SetSaveText(wzBuf);

#ifndef WIN16  //RUN16_BLOCK - NOT YET AVAILABLE
        hr = URLOpenBlockingStreamW(NULL, bstrURL, &pstm, 0, NULL);
#else
        hr = MIME_E_URL_NOTFOUND;
#endif

        if (SUCCEEDED(hr))
        {
            HBODY hBody;

            hr = m_pimm->AttachURL(NULL, szUrl, dwAttach, pstm, &lpszCID, &hBody);

            if (SUCCEEDED(hr))
                hr = _Insert( bstrURL, bstrURL, pphe );
        }

        ReleaseInterface(pstm);
    }
    
    RRETURN(hr);
}

HRESULT
CMHTMLArchive::AddFrameOrStyleEntry( BSTR bstrURL, CHashEntry **pphe, LPTSTR lpstrFrameDoc )
{
    HRESULT hr;
    
    hr = THR(Find(bstrURL, pphe)); // there's always a slim chance we're reusing a frame.
    
    if (FAILED(hr))
    {     
        // insert place-holder
        hr = _Insert(bstrURL, c_bstr_BLANK, pphe);  
    }
    
    return hr; // no RRETURN - may return S_FALSE
}



HRESULT
CMHTMLArchive::ArchiveDocumentText(IHTMLDocument2 *pDoc, UINT cpDoc, BOOL fFrameDoc)
{
    HRESULT             hr = S_OK;
    IPersistStreamInit* ppsi = NULL;
    PROPVARIANT             variant;
    FILETIME                filetime;
    WCHAR                   wzBuffer[MAX_BUFFER_LEN];
    WCHAR                   wzArchiveText[MAX_SAVING_STATUS_TEXT + 1];
    WCHAR                   wzBuf[INTERNET_MAX_URL_LENGTH + MAX_SAVING_STATUS_TEXT + 1];

    // Set the MIME subject header

    PropVariantClear(&variant);
    variant.vt = VT_LPWSTR;
    hr = pDoc->get_title(&variant.pwszVal);
    if (SUCCEEDED(hr))
    {
        hr = m_pimm->SetBodyProp(HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT), 0,
                                 &variant);
        SAFEFREEBSTR(variant.pwszVal);
    }

    // Set the MIME date header

    if (SUCCEEDED(hr))
    {
        hr = CoFileTimeNow(&filetime);
    }

    if (SUCCEEDED(hr))
    {
        PropVariantClear(&variant);
        variant.vt = VT_FILETIME;
        variant.filetime = filetime;
        hr = m_pimm->SetBodyProp(HBODY_ROOT, PIDTOSTR(PID_HDR_DATE), 0,
                                 &variant);
    }

    // Set the MIME from header

    if (SUCCEEDED(hr))
    {
        MLLoadStringW(IDS_MIME_SAVEAS_HEADER_FROM, wzBuffer,
                      ARRAYSIZE(wzBuffer));

        PropVariantClear(&variant);
        variant.vt = VT_LPWSTR;
        variant.pwszVal = wzBuffer;
        hr = m_pimm->SetBodyProp(HBODY_ROOT, PIDTOSTR(PID_HDR_FROM), 0,
                                 &variant);
    }
    
    hr = pDoc->QueryInterface(IID_IPersistStreamInit, (void**)&ppsi);
    if (SUCCEEDED(hr))
    {
        IStream *pstm = NULL;

        hr = MimeOleCreateVirtualStream( &pstm );
        if ( SUCCEEDED(hr) )
        {
            HBODY hBody;
            hr = ppsi->Save(pstm, FALSE);


            if (SUCCEEDED(hr))
            {
                BSTR    bstrDocURL = NULL;
                WCHAR  *pwzBookMark = NULL;
                
                pDoc->get_URL(&bstrDocURL);
                RemoveBookMark(bstrDocURL, &pwzBookMark);

                if (!StrCmpIW(bstrDocURL, URL_ABOUT_BLANK))
                {
                    // We got about:blank as the URL (because the doc has
                    // document.write's etc in it). We can't save this!
                    hr = E_FAIL;
                    goto Exit;
                }

                MLLoadStringW(IDS_SAVING_STATUS_TEXT, wzArchiveText,
                              ARRAYSIZE(wzArchiveText));
        
                wnsprintfW(wzBuf, ARRAYSIZE(wzBuf), L"%ws: %ws", wzArchiveText, bstrDocURL);
                m_ptp->SetSaveText(wzBuf);


                if (fFrameDoc)
                {
                    CHAR    szURL[INTERNET_MAX_URL_LENGTH];
                    LPSTR   lpszCID = NULL;
                    DWORD   dwAttach = URL_ATTACH_SET_CNTTYPE; 
        
                    szURL[0] = 0;

        
                    if (WideCharToMultiByte(CP_ACP, 0, bstrDocURL, -1, szURL, INTERNET_MAX_URL_LENGTH, NULL, NULL))
                    {

                        hr = m_pimm->AttachURL(NULL, szURL, dwAttach,
                                               pstm, &lpszCID, &hBody);

                        if (SUCCEEDED(hr) && cpDoc)
                        {
                            IMimeBody         *pBody = NULL;

                            hr = m_pimm->BindToObject(hBody, IID_IMimeBody,
                                                      (LPVOID *)&pBody);
                            if (SUCCEEDED(hr))
                            {
                                hr = SetCharset(cpDoc, CSET_APPLY_TAG_ALL, pBody);
                            }
                            pBody->Release();
                        }

                        if (SUCCEEDED(hr))
                        {
                            CHashEntry *phe;

                            LPWSTR  pwz = NULL;
                            int     iLen = 0;

                            // If it is ASP, it is actually HTML
            
                            iLen = lstrlenW(bstrDocURL);
            
                            if (iLen) {
                                pwz = StrRChrW(bstrDocURL, bstrDocURL + iLen, L'.');
                            }
            
            
                            if (pwz && !StrCmpIW(pwz, TEXT(".asp")))
                            {
                                PROPVARIANT             propvar;

                                PropVariantClear(&propvar);
                                propvar.vt = VT_LPSTR;
                                propvar.pszVal = "text/html";
                                hr = m_pimm->SetBodyProp(hBody,
                                                         PIDTOSTR(PID_HDR_CNTTYPE),
                                                         0, &propvar);
                            }

                            if ( m_hBodyAlt == NULL )
                                m_hBodyAlt = hBody;

                            // update the place-holder hash entry
                            hr = Find( bstrDocURL, &phe);

                            ASSERT(SUCCEEDED(hr) && phe != NULL);

                            phe->SetValue( bstrDocURL );

                        }
                    }
                    else
                         hr = HRESULT_FROM_WIN32(GetLastError());

                }
                else
                {
                    hr = m_pimm->SetTextBody( TXT_HTML, IET_INETCSET, m_hBodyAlt, pstm, &hBody);
                    // The main text was the last thing we were waiting for
                    if (SUCCEEDED(hr) && cpDoc)
                    {
                        IMimeBody         *pBody = NULL;

                        hr = m_pimm->BindToObject(hBody, IID_IMimeBody,
                                                  (LPVOID *)&pBody);
                        if (SUCCEEDED(hr))
                        {
                            hr = SetCharset(cpDoc, CSET_APPLY_TAG_ALL, pBody);
                        }
                        pBody->Release();
                    }

                    if (SUCCEEDED(hr))
                    {
                        IPersistFile *pipf = NULL;
                        // Initialzie PropVariant
                        PROPVARIANT rVariant;
                        rVariant.vt = VT_LPWSTR;
                        rVariant.pwszVal = (LPWSTR)bstrDocURL;
                        // Add a content location, so we can use it for security later.
                        hr = m_pimm->SetBodyProp( hBody, STR_HDR_CNTLOC, 0, &rVariant );
                        if (SUCCEEDED(hr))
                        {
                            hr = m_pimm->QueryInterface(IID_IPersistFile, (LPVOID *)&pipf);
                            if (SUCCEEDED(hr))
                            {
                                LPWSTR lpwszFile;
                                lpwszFile = m_lpstrDoc;
                                hr = pipf->Save(lpwszFile, FALSE);

                                SAFERELEASE(pipf);
                            }
                        }

                        ReleaseInterface(pstm);
                    }
                }

                if ( bstrDocURL )
                {
                    // Restore Bookmark
                    RestoreBookMark(pwzBookMark);
                    SysFreeString(bstrDocURL);
                }
            }

            ReleaseInterface(pstm);
        }
    }
   
    ReleaseInterface(ppsi);

Exit:
    
    RRETURN(hr);
}

HRESULT
CMHTMLArchive::ArchiveCSSText( BSTR bstrCSSUrl, LPCSTR lpszSSText, LPCTSTR lpszStyleDoc )
{
    HRESULT hr;
    BSTR    bstrDocURL = NULL;
    CHAR    szURL[INTERNET_MAX_URL_LENGTH];
    LPSTR   lpszCID = NULL;
    DWORD   dwAttach = URL_ATTACH_SET_CNTTYPE;
    HBODY   hBody;
    IStream *pstm = NULL;
    ULONG cbWrite, cbWritten;

    hr = MimeOleCreateVirtualStream( &pstm );
    if (FAILED(hr))
        return hr;

    cbWrite = lstrlenA(lpszSSText);
    pstm->Write(lpszSSText, cbWrite, &cbWritten);
    ASSERT(cbWritten==cbWrite);

    //if (dwFlags & MECD_CNTLOCATIONS)
    //    dwAttach |= URL_ATTACH_SET_CNTLOCATION;

    szURL[0] = 0;

    if (WideCharToMultiByte(CP_ACP, 0, bstrCSSUrl, -1, szURL, INTERNET_MAX_URL_LENGTH, NULL, NULL))
    {

        hr = m_pimm->AttachURL(NULL, szURL, dwAttach,
                                 pstm, &lpszCID, &hBody);

        if (SUCCEEDED(hr))
        {
            CHashEntry *phe;

            // update the place-holder hash entry
            hr = Find(bstrCSSUrl, &phe);

            ASSERT(SUCCEEDED(hr) && phe != NULL);

            phe->SetValue( bstrCSSUrl );
        }
    }
    else
        hr = HRESULT_FROM_WIN32(GetLastError());

    ReleaseInterface(pstm);

    return hr;
}

HRESULT CMHTMLArchive::SetCharset(UINT uiCharset, CSETAPPLYTYPE csat,
                                  IMimeBody *pBody)
{
    HRESULT                        hr = E_FAIL;
    HCHARSET                       hCharset;
    IMimeInternational            *pimi = NULL;
    
    hr = CoCreateInstance(CLSID_IMimeInternational,
                          NULL, CLSCTX_INPROC_SERVER,
                          IID_IMimeInternational, (LPVOID*)&pimi);
    if (SUCCEEDED(hr))
    {
        hr = pimi->GetCodePageCharset(uiCharset, CHARSET_WEB, &hCharset);
    }

    if (SUCCEEDED(hr))
    {
        hr = pBody->SetCharset(hCharset, csat);
    }
                            
    if (pimi)
    {
        pimi->Release();
    }

    return hr;
}

/*
*  CThicketProgress ##################################################
*/

CThicketProgress::CThicketProgress( HWND hDlg )
{
    TCHAR szFmt[MAX_PATH];
    int   cchPctFmt;

    m_hDlg = hDlg;
    m_hwndProg = GetDlgItem(hDlg, IDC_THICKETPROGRESS);

    MLLoadString(IDS_THICKETSAVINGFMT, szFmt, ARRAYSIZE(szFmt) );
    m_cchSavingFmt = lstrlen(szFmt);
    m_pszSavingFmt = new TCHAR[m_cchSavingFmt+1];
    if (m_pszSavingFmt != NULL)
    {
        StrCpyN( m_pszSavingFmt, szFmt, m_cchSavingFmt+1 );
    }

    MLLoadString(IDS_THICKETPCTFMT, szFmt, ARRAYSIZE(szFmt));
    cchPctFmt = lstrlen(szFmt);
    m_pszPctFmt = new TCHAR[cchPctFmt+1];
    if (m_pszPctFmt != NULL)
    {
        StrCpyN( m_pszPctFmt, szFmt, cchPctFmt+1 );
    }

    m_ulPct = 0;
}

CThicketProgress::~CThicketProgress(void)
{
    if (m_pszSavingFmt)
        delete[] m_pszSavingFmt;

    if (m_pszPctFmt)
        delete[] m_pszPctFmt;

}

void CThicketProgress::SetPercent( ULONG ulPct )
{
    TCHAR szBuf[MAX_PATH];

    szBuf[0] = TEXT('\0');

    if ( ulPct > 100 )
        ulPct = 100;

    if ( ulPct > m_ulPct ) // prevent retrograde motion.
    {
        m_ulPct = ulPct;
        if (m_pszPctFmt != NULL)
        {
            wnsprintf( szBuf, ARRAYSIZE(szBuf), m_pszPctFmt, m_ulPct );
        }
        SetDlgItemText(m_hDlg, IDC_THICKETPCT, szBuf);
        SendMessage(m_hwndProg, PBM_SETPOS, m_ulPct, 0);
    }
}

void CThicketProgress::SetSaving( LPCTSTR szFile, LPCTSTR szDst )
{
    TCHAR szPath[30];
    TCHAR szBuf[MAX_PATH*2];
    LPCTSTR psz;

    szBuf[0] = TEXT('\0');

    if (PathCompactPathEx( szPath, szDst, 30, 0 ))
    {
        psz = szPath;
    }
    else
    {
        psz = szDst;
    }

    if (m_pszSavingFmt != NULL)
    {
        wnsprintf( szBuf, ARRAYSIZE(szBuf), m_pszSavingFmt, szFile, psz );
    }

    SetDlgItemText(m_hDlg, IDC_THICKETSAVING, szBuf);
}

void CThicketProgress::SetSaveText(LPCTSTR szText)
{
    if (szText)
    {
        SetDlgItemText(m_hDlg, IDC_THICKETSAVING, szText);
    }
}

/*
*  CCollectionPackager ##################################################
*/

CCollectionPackager::~CCollectionPackager(void)
{
    if (m_pColl)
        m_pColl->Release();
}


HRESULT CCollectionPackager::_GetElementURL(IHTMLElement *pElem, BSTR *pbstrURL)
{
    HRESULT         hr;
    VARIANT         rVar;
    
    ASSERT (pElem);
    
    rVar.vt = VT_BSTR;
    
    // Note that _GetTargetAttribute is a virtual method, so the derived class
    // specifies what attribute to fetch.
    
    hr = THR(pElem->getAttribute(_GetTargetAttribute(), VARIANT_FALSE, &rVar));
    if (SUCCEEDED(hr))
    {
        if (rVar.vt == VT_BSTR && rVar.bstrVal != NULL)
            *pbstrURL = rVar.bstrVal;
        else
            hr = S_FALSE;
    }
    
    return hr; // no RRETURN - may return S_FALSE
}


HRESULT CCollectionPackager::_PackageData(CWebArchive *pwa,
                                          IHTMLElementCollection *pColl,
                                          BOOL *pfCancel,
                                          CThicketProgress *ptp, ULONG progLow, ULONG progHigh)
{
    HRESULT        hr = S_OK;
    ULONG          uElem,
                   cElems,
                   uRange = progHigh - progLow;
    IHTMLElement   *pElem;
    
    cElems = UlGetCollectionCount(pColl);
    
    // Iterate over the collection, packaging each element in turn.
    
    for (uElem=0; uElem<cElems && SUCCEEDED(hr) ; uElem++)
    {
        hr = THR(HrGetCollectionItem(pColl, uElem, IID_IHTMLElement, (LPVOID *)&pElem));
        if (SUCCEEDED(hr))
        {
            hr = _PackageElement(pwa, pElem ); // no THR - may return S_FALSE
            pElem->Release();
        }

        if (pfCancel && *pfCancel)
            hr = E_ABORT;

        if (ptp && uRange)
            ptp->SetPercent( progLow + (uRange * uElem) / cElems );
    }
    
    return hr; // no RRETURN - may return S_FALSE
}

HRESULT CCollectionPackager::_PackageElement(CWebArchive *pwa,
                                             IHTMLElement *pElem)
{
    HRESULT        hr = S_OK;
    BSTR           bstrURL = NULL;
    BOOL           fBadLinks=FALSE;
    CHashEntry     *phe;
    
    hr = _GetElementURL(pElem, &bstrURL);
    if (hr == S_OK && bstrURL && bstrURL[0])
    {
        // PTH hr = HrAddImageToMessage(pMsgSrc, pMsgDst, pHash, bstrURL, &bstrURLThicket, m_fAddCntLoc);
        hr = pwa->AddURL( bstrURL, &phe );

        if (SUCCEEDED(hr))
        {
            hr = THR(HrSetMember(pElem, _GetTargetAttribute(), phe->m_bstrValue));
        }
        else
            hr = THR(HrSetMember(pElem, _GetTargetAttribute(), c_bstr_EMPTY));
    }

    if (bstrURL)
        SysFreeString(bstrURL);


    return hr; 
}


HRESULT CCollectionPackager::_InitSubCollection(IHTMLElementCollection *pAll,
                                                BSTR bstrTagName,
                                                IHTMLElementCollection **ppSub,
                                                ULONG *pcElems)
{
    IDispatch              *pDisp=NULL;
    VARIANT                 TagName;
    HRESULT                 hr = S_FALSE;

    ASSERT (ppSub);
    ASSERT(pAll);

    *ppSub = NULL;
    
    TagName.vt = VT_BSTR;
    TagName.bstrVal = bstrTagName;
    if (NULL == TagName.bstrVal)
        hr = E_INVALIDARG;
    else
    {
        hr = pAll->tags(TagName, &pDisp);
    }
    
    if (pDisp)
    {
        hr = pDisp->QueryInterface(IID_IHTMLElementCollection,
            (void **)ppSub);
        pDisp->Release();
    }
    
    if (pcElems)
    {
        if (hr == S_OK)
            *pcElems = UlGetCollectionCount(*ppSub);
        else
            *pcElems = 0;
    }
    
    RRETURN(hr);
}

/*
*  CImagePackager ##################################################
*/


HRESULT CImagePackager::InitFromCollection(IHTMLElementCollection *pColl,
                                           ULONG *pcElems)
{
    return _InitSubCollection(pColl, (BSTR)c_bstr_IMG, &m_pColl, pcElems);
}

BSTR CImagePackager::_GetTargetAttribute(void)
{
    return (BSTR)c_bstr_SRC;
}

/*
*  CInputImgPackager ##################################################
*/

HRESULT CInputImgPackager::InitFromCollection(IHTMLElementCollection *pColl,
                                              ULONG *pcElems)
{
    return _InitSubCollection(pColl, (BSTR)c_bstr_INPUT, &m_pColl, pcElems);
}

/*
*  CBGSoundsPackager ##################################################
*/

HRESULT CBGSoundsPackager::InitFromCollection(IHTMLElementCollection *pColl,
                                              ULONG *pcElems)
{
    return _InitSubCollection(pColl, (BSTR)c_bstr_BGSOUND, &m_pColl, pcElems);
}

BSTR CBGSoundsPackager::_GetTargetAttribute(void)
{
    return (BSTR)c_bstr_SRC;
}

/*
*  CAnchorAdjustor ##################################################
*/


HRESULT CAnchorAdjustor::InitFromCollection(IHTMLElementCollection *pColl,
                                            ULONG *pcElems)
{
    return _InitSubCollection(pColl, (BSTR)c_bstr_ANCHOR, &m_pColl, pcElems);
}

BSTR CAnchorAdjustor::_GetTargetAttribute(void)
{
    return (BSTR)c_bstr_HREF;
}

HRESULT CAnchorAdjustor::_PackageElement(CWebArchive *pwa,
                                         IHTMLElement *pElem)
{
    HRESULT        hr = S_OK;
    BSTR           bstrURL = NULL;
    BSTR           bstrThicket = NULL;
    BOOL           fBadLinks=FALSE;
    CHashEntry     *phe;
    
    // leave intra-doc urls and <A name=> alone
    // BUGBUG seanf(2/11/98) : haven't seen a local # link come through here yet. 
    hr = _GetElementURL(pElem, &bstrURL);
    if (hr != S_OK || bstrURL == NULL || bstrURL[0] == '#' || bstrURL[0] == 0)
        goto error;
    
    // See if the target is something we have in the thicket, like an <A> in frame A
    // targetting the page saved for frame B.
    ASSERT(pwa);

    hr = pwa->Find(bstrURL, &phe);
    if (SUCCEEDED(hr))
        bstrThicket = phe->m_bstrValue;
    else
    {
        // not in the thicket, so make both URLs the same.
        bstrThicket = bstrURL;
        hr = S_OK;
    }

    if (hr == S_OK)
        hr = THR(HrSetMember(pElem, _GetTargetAttribute(), bstrThicket));
    
error:
    
    if (bstrURL)
        SysFreeString(bstrURL);

    // don't free bstrThicket, its either bstrURL, or belongs to the thicket hash table.
    
    return hr; 
}

/*
*  CAreaAdjustor ##################################################
*/


HRESULT CAreaAdjustor::InitFromCollection(IHTMLElementCollection *pColl,
                                            ULONG *pcElems)
{
    return _InitSubCollection(pColl, (BSTR)c_bstr_AREA, &m_pColl, pcElems);
}

/*
*  CBaseNeutralizer ##################################################
*/

CBaseNeutralizer::~CBaseNeutralizer(void)
{
    if (m_bstrLocal)
        SysFreeString(m_bstrLocal);
 
    if (m_pTree)
        m_pTree->Release();
}

HRESULT CBaseNeutralizer::InitFromCollection(IHTMLElementCollection *pColl,
                                             ULONG *pcElems,
                                             IHTMLDocument2 *pDoc )
{
    if ( pDoc != NULL )
    {
        if ( m_pTree )
        {
            m_pTree->Release();
            m_pTree = NULL;
        }
        pDoc->QueryInterface(IID_IMarkupServices, (void**)&m_pTree);
    }

    return _InitSubCollection(pColl, (BSTR)c_bstr_BASE, &m_pColl, pcElems);
}


BSTR CBaseNeutralizer::_GetTargetAttribute(void)
{
    return (BSTR)c_bstr_HREF;
}

HRESULT CBaseNeutralizer::PackageData(CWebArchive *pwa, BOOL *pfCancel,
                                      CThicketProgress *ptp,
                                      ULONG progLow, ULONG progHigh)
{
    HRESULT        hr = S_OK;
    ULONG          uElem,
                   cElems,
                   uRange = progHigh - progLow;
    IHTMLElement   *pElem;
    
    cElems = UlGetCollectionCount(m_pColl);
    
    // Iterate over the collection, packaging each element in turn.
    
    for (uElem=0; uElem<cElems && SUCCEEDED(hr) ; uElem++)
    {
        hr = THR(HrGetCollectionItem(m_pColl, 0, IID_IHTMLElement, (LPVOID *)&pElem));
        if (SUCCEEDED(hr))
        {
            hr = _PackageElement(pwa, pElem ); // no THR - may return S_FALSE
            pElem->Release();
        }

        if (pfCancel && *pfCancel)
            hr = E_ABORT;

        if (ptp && uRange)
            ptp->SetPercent( progLow + (uRange * uElem) / cElems );
    }
    
    return hr; // no RRETURN - may return S_FALSE
}

HRESULT CBaseNeutralizer::_PackageElement(CWebArchive *pwa,
                                          IHTMLElement *pElem)
{
    HRESULT        hr = S_FALSE;

    // BUGBUG: There's seems to be no retouching that will make this work.
    //         Tried setting BASE to ".", ".\", "". It has to be absolute,
    //         which would anchor the thicket to one location in the file
    //         system. The solution here is to use the base to fix the
    //         other rel URLs in the doc, then whack the base tags.
    if ( m_pTree )
    {
        //BUGBUG: Tree Services can't remove a head element yet, so 
        //        wait to enable this pending Joe Beda/EricVas work.
        hr = m_pTree->RemoveElement( pElem );
    }

    return hr; // no RRETURN - may return S_FALSE
}

/*
*  CRelativeURLPackager ##################################################
*/


CRelativeURLPackager::~CRelativeURLPackager(void)
{
    if (m_pCollBase)
        m_pCollBase->Release();
    
    if (m_bstrDocURL)
        SysFreeString(m_bstrDocURL);
}


HRESULT CRelativeURLPackager::Init(IHTMLElementCollection *pColl,
                                   ULONG *pcElems,
                                   IHTMLDocument2 *pDoc)
{
    HRESULT hr = S_OK;
    
    // Hold on to the outer collection, we'll subsample it later.
    m_pColl = pColl;
    
    if (m_pColl)
    {
        m_pColl->AddRef();
        hr = _InitSubCollection( m_pColl, (BSTR)c_bstr_BASE, &m_pCollBase, &m_cBase );
    }
    
    if (SUCCEEDED(hr) && pDoc)
    {
        hr = pDoc->get_URL( &m_bstrDocURL );
    }
    
    RRETURN(hr);
}

HRESULT CRelativeURLPackager::_GetElementURL(IHTMLElement *pElem, BSTR *pbstrURL)
{
    HRESULT             hr = S_FALSE;
    LONG                lElemPos;
    BSTR                bstr = NULL;
    
    
    ASSERT (pbstrURL);
    *pbstrURL = 0;
    
    hr = CCollectionPackager::_GetElementURL(pElem, &bstr);
    if (hr==S_OK)
    {
        if (bstr==NULL)
            hr = S_FALSE;
        else
        {
            hr = pElem->get_sourceIndex(&lElemPos);
            ASSERT(SUCCEEDED(hr));
            hr = HrGetCombinedURL(m_pCollBase, m_cBase, lElemPos, bstr, m_bstrDocURL, pbstrURL);
            SysFreeString(bstr);
        }
    }
    
    return hr; // no RRETURN - may return S_FALSE
}

/*
*  CBackgroundPackager ##################################################
*/


HRESULT CBackgroundPackager::PackageData(CWebArchive *pwa,
                                         BOOL *pfCancel,
                                         CThicketProgress *ptp, ULONG progLow, ULONG progHigh)
{
    HRESULT hr = S_OK;
    IHTMLElementCollection *pColl = NULL;
    
    hr = _InitSubCollection(m_pColl, (BSTR)c_bstr_BODY, &pColl);
    if (SUCCEEDED(hr))
    {
        if (hr==S_OK)
            hr = _PackageData( pwa, pColl, pfCancel );
        if (FAILED(hr))
            goto error;
        pColl->Release();
        pColl = NULL;
    }
    
    hr = _InitSubCollection(m_pColl, (BSTR)c_bstr_TABLE, &pColl);
    if (SUCCEEDED(hr))
    {
        if (hr==S_OK)
            hr = _PackageData( pwa, pColl, pfCancel);
        if (FAILED(hr))
            goto error;
        pColl->Release();
        pColl = NULL;
    }
    
    hr = _InitSubCollection(m_pColl, (BSTR)c_bstr_TD, &pColl);
    if (SUCCEEDED(hr))
    {
        if (hr==S_OK)
            hr = _PackageData( pwa, pColl, pfCancel );
        if (FAILED(hr))
            goto error;
        pColl->Release();
        pColl = NULL;
    }
    
    hr = _InitSubCollection(m_pColl, (BSTR)c_bstr_TH, &pColl);
    if (SUCCEEDED(hr))
    {
        if (hr==S_OK)
            hr = _PackageData( pwa, pColl, pfCancel );
        if (FAILED(hr))
            goto error;
        pColl->Release();
        pColl = NULL;
    }
    
error:
    
    if (pColl)
        pColl->Release();
    
    return hr; // no RRETURN - may return S_FALSE
}


BSTR CBackgroundPackager::_GetTargetAttribute(void)
{
    return (BSTR)c_bstr_BACKGROUND;
}


/*
*  CDynSrcPackager ##################################################
*/


HRESULT CDynSrcPackager::PackageData(CWebArchive *pwa,
                                          BOOL *pfCancel,
                                          CThicketProgress *ptp, ULONG progLow, ULONG progHigh)
{
    HRESULT hr = S_OK;
    IHTMLElementCollection *pColl = NULL;
    
    hr = _InitSubCollection(m_pColl, (BSTR)c_bstr_IMG, &pColl);
    if (SUCCEEDED(hr))
    {
        if (hr==S_OK)
            hr = _PackageData( pwa, pColl, pfCancel );
        if (FAILED(hr))
            goto error;
        pColl->Release();
        pColl = NULL;
    }
    
    hr = _InitSubCollection(m_pColl, (BSTR)c_bstr_INPUT, &pColl);
    if (SUCCEEDED(hr))
    {
        if (hr==S_OK)
            hr = _PackageData( pwa, pColl, pfCancel );
        if (FAILED(hr))
            goto error;
        pColl->Release();
        pColl = NULL;
    }
    
    
error:
    
    if (pColl)
        pColl->Release();
    
    return hr; // no RRETURN - may return S_FALSE
}


BSTR CDynSrcPackager::_GetTargetAttribute(void)
{
    return (BSTR)c_bstr_DYNSRC;
}


/*
*  CScriptPackager ##################################################
*/


HRESULT CScriptPackager::Init(IHTMLElementCollection *pColl,
                              ULONG *pcElems,
                              IHTMLDocument2 *pDoc)
{
    HRESULT hr = CRelativeURLPackager::Init(pColl, NULL, pDoc);

    if (SUCCEEDED(hr))
        hr = _InitSubCollection(m_pColl, (BSTR)c_bstr_SCRIPT, &m_pCollScripts, pcElems );

    return hr;
}


BSTR CScriptPackager::_GetTargetAttribute(void)
{
    return (BSTR)c_bstr_SRC;
}

/*
*  CFramesPackager ##################################################
*/

HRESULT CFramesPackager::Init(IHTMLElementCollection *pColl,
                              ULONG *pcElems,
                              IHTMLDocument2 *pDoc,
                              IHTMLDocument2 *pDocDesign,
                              CDocumentPackager *pdp)
{
    HRESULT hr = CRelativeURLPackager::Init(pColl, NULL, pDocDesign);

    if (SUCCEEDED(hr))
    {
        m_pdp = pdp;
        // Get the element collection for the frames.
        // Note: If documents have frames, they are either all
        // <FRAME>s _OR_ all <IFRAME>s.
        hr = _InitSubCollection(m_pColl, (BSTR)c_bstr_FRAME, &m_pCollFrames, &m_cFrames);
        if (FAILED(hr) || m_cFrames == 0)
        {
            if (m_pCollFrames)
                m_pCollFrames->Release();
            hr = _InitSubCollection(m_pColl, (BSTR)c_bstr_IFRAME, &m_pCollFrames, &m_cFrames);
        }

        if (pcElems)
            *pcElems = m_cFrames;

        // To traverse a framseset that spans multiple domains, we need to approach it
        // via the "unsecured" window object, which is only accessible via Invoke.
        if (SUCCEEDED(hr) && m_cFrames > 0)
        {
            DISPPARAMS dispparams;
            VARIANT VarResult;
            VariantInit(&VarResult);
            ZeroMemory(&dispparams, sizeof(dispparams));

            hr = pDoc->Invoke(DISPID_WINDOWOBJECT,
                              IID_NULL,
                              0,
                              DISPATCH_PROPERTYGET,
                              &dispparams,
                              &VarResult,
                              NULL,
                              NULL );

            if( SUCCEEDED(hr) )
            {
                // Code in iedisp.cpp's  GetDelegateOnIDispatch was really paranoid about this,
                // so we'll be similarly cautious.
                if( (VarResult.vt == VT_DISPATCH || VarResult.vt == VT_UNKNOWN)
                    && VarResult.pdispVal )
                {
                    IHTMLWindow2 *pwin2 = NULL;

                    hr = VarResult.pdispVal->QueryInterface( IID_IHTMLWindow2, (LPVOID*)&pwin2);
                    if (SUCCEEDED(hr))
                    {
                        hr = pwin2->get_frames(&m_pframes2);
                        pwin2->Release();
                    }
                } // if we really got an interface
                else
                    hr = E_FAIL;

                VariantClearLazy( &VarResult );
            } // if we can get the un-secured window object
        } // if we have frames
    } // if base initialization succeeded

    return hr;
}


HRESULT CFramesPackager::PackageData(CWebArchive *pwa,
                                          BOOL *pfCancel,
                                          CThicketProgress *ptp, ULONG progLow, ULONG progHigh)
{
    HRESULT hr = S_OK;
    //ULONG   cColl = 0;
    
    if (m_cFrames == 0)
        return S_OK; // Trident will freak if we return a non-S_OK success code
    
    m_iFrameCur = 0; // index of frame in window.frames and all.tags("FRAME");
    m_pfCancel = pfCancel;
    m_ptp = ptp;
    m_uLow = progLow;
    m_uHigh = progHigh; 
    
    m_uRangeDoc = (progHigh - progLow) / m_cFrames;
    hr = _PackageData( pwa, m_pCollFrames, pfCancel );
        
    return hr; // no RRETURN - may return S_FALSE
}


BSTR CFramesPackager::_GetTargetAttribute(void)
{
    return (BSTR)c_bstr_SRC;
}

HRESULT CFramesPackager::_PackageElement(CWebArchive *pwa,
                                         IHTMLElement *pElem)
{
    HRESULT        hr = S_OK;
    BSTR           bstrURL = NULL;
    BOOL           fBadLinks=FALSE;
    IHTMLDocument2 *pDocFrame = NULL;
    //IWebBrowser    *pwb = NULL;
    IDispatch      *pDisp = NULL;
    IHTMLWindow2   *pwin2 = NULL;
    VARIANT        varIndex;
    VARIANT        varFrame;
    WCHAR         *pwzBookMark = NULL;
    
    ASSERT(pElem);
    ASSERT(pwa);

    varIndex.vt = VT_I4;
    varIndex.lVal = m_iFrameCur;
    hr = m_pframes2->item( &varIndex, &varFrame );
    if (FAILED(hr))
        goto error;
    // The variant should give us an IHTMLWindow2, but we'll treat it as a Disp anyway
    ASSERT(varFrame.vt & VT_DISPATCH);
    pDisp = varFrame.pdispVal;
    hr = pDisp->QueryInterface(IID_IHTMLWindow2, (LPVOID*)&pwin2 );
    if (FAILED(hr))
        goto error;

    hr = pwin2->get_document(&pDocFrame);

#ifdef OLD_THICKET

    hr = pElem->QueryInterface(IID_IWebBrowser, (void**)&pwb);
    if (FAILED(hr))
        goto error;
    
    hr = pwb->get_Document( &pDisp );
    if (FAILED(hr))
        goto error;
    else if ( pDisp == NULL )
    {
        hr = S_FALSE;
        goto error;
    }
    
    hr = pDisp->QueryInterface(IID_IHTMLDocument2, (void**)&pDocFrame);
    if (FAILED(hr))
        goto error;

#endif // OLD_THICKET

    if (SUCCEEDED(hr) && SUCCEEDED(pDocFrame->get_URL(&bstrURL)) && bstrURL && bstrURL[0])
    {
        TCHAR       szFrameDoc[MAX_PATH];
        CHashEntry  *phe;
        
        RemoveBookMark(bstrURL, &pwzBookMark);

        hr = pwa->AddFrameOrStyleEntry( bstrURL, &phe, szFrameDoc );
        if (hr==S_OK)
        {
            ULONG uLowDoc = m_uLow + m_iFrameCur * m_uRangeDoc;
            ULONG uHighDoc = uLowDoc + m_uRangeDoc;
            CWebArchive *pwaFrame = m_pdp->GetFrameDocArchive( pwa );
            
            if ( pwaFrame != NULL )
            {
                BSTR               bstrCharSetSrc = NULL;
                MIMECSETINFO       csetInfo;
                IMultiLanguage2   *pMultiLanguage = NULL;

                hr = pDocFrame->get_charset(&bstrCharSetSrc);
                if (FAILED(hr))
                    goto error;
            
                hr = CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER,
                                      IID_IMultiLanguage2, (void**)&pMultiLanguage);
                if (FAILED(hr))
                {
                    goto error;
                }
            
                hr = pMultiLanguage->GetCharsetInfo(bstrCharSetSrc, &csetInfo);
                pMultiLanguage->Release();

                if (FAILED(hr))
                {
                    goto error;
                }

                hr = m_pdp->_PackageDocument(pDocFrame, szFrameDoc, m_pfCancel, m_ptp, 
                                             uLowDoc, uHighDoc, csetInfo.uiInternetEncoding,
                                             pwaFrame, m_pdp, TRUE );
                if (SUCCEEDED(hr))
                    hr = THR(HrSetMember(pElem, _GetTargetAttribute(), phe->m_bstrValue));
                else
                    fBadLinks = TRUE;

                if ( pwaFrame != pwa ) // only delete if new one was made (thicket)
                    delete pwaFrame;
            }
            else
                hr = E_OUTOFMEMORY;
        } // if the location matched the element URL
        else if (hr==S_FALSE)
        {
            // This is a repeat - we don't need to do most of the work, but we
            // do need to record the element for remapping.
            hr = THR(HrSetMember(pElem, _GetTargetAttribute(), phe->m_bstrValue));
        }
    } // if we got the frame's doc's URL
    else // if ( hr == DISP_E_MEMBERNOTFOUND ) // frame is non-trident docobj
    {
        IHTMLLocation *ploc = NULL;

        // For a non-trident doc-obj, get the file, if possible, and put it in the thicket.

        hr = pwin2->get_location( &ploc );
        if (SUCCEEDED(hr) &&
            SUCCEEDED(hr = ploc->get_href( &bstrURL )))
        {
            if (bstrURL && bstrURL[0])
            {
                CHashEntry  *phe;
                // PTH hr = HrAddImageToMessage(pMsgSrc, pMsgDst, pHash, bstrURL, &bstrURLThicket, m_fAddCntLoc);
                hr = pwa->AddURL( bstrURL, &phe );
                if (!FAILED(hr))
                {
                    hr = THR(HrSetMember(pElem, _GetTargetAttribute(), phe->m_bstrValue));
                }
            }
            else
                hr = S_FALSE;
        }
        ReleaseInterface(ploc);
    }
    
error:
    //ReleaseInterface(pwb);
    ReleaseInterface(pwin2);
    ReleaseInterface(pDisp);
    ReleaseInterface(pDocFrame);

    if (bstrURL) {
        RestoreBookMark(pwzBookMark);
        SysFreeString(bstrURL); // bstrFrameURL);
    }
    
    m_iFrameCur++;

    return hr;
}

/*
*  CDocumentPackager ##################################################
*/

HRESULT CDocumentPackager::PackageDocument(IHTMLDocument2 *pDoc,
                                           LPCTSTR lpstrDoc,
                                           BOOL *pfCancel, CThicketProgress *ptp,
                                           ULONG progLow, ULONG progHigh,
                                           UINT cpDst,
                                           CWebArchive *pwa)
{
    HRESULT hr = S_OK;

    m_ptp = ptp;

    switch (m_iPackageStyle)
    {
    case PACKAGE_THICKET:
        {
            CThicketArchive thicket(ptp);

            hr = _PackageDocument( pDoc, lpstrDoc, pfCancel, ptp, progLow, progHigh, cpDst, &thicket, this, FALSE );
        }
        break;

    case PACKAGE_MHTML:
        {
            CMHTMLArchive *pmhtmla = (CMHTMLArchive *)pwa; // sleazy downcast
    
            if (pwa == NULL)
                pmhtmla = new CMHTMLArchive(ptp);

            if (pmhtmla != NULL)
            {
                hr = _PackageDocument( pDoc, lpstrDoc, pfCancel, ptp, progLow, progHigh, cpDst,
                                       pmhtmla, this, FALSE );

                // if pwa is NULL, then we created a CMHTMLArchive for
                // use in _PackageDocument which we now need to clean up
                if (pwa == NULL)
                    delete pmhtmla;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        break;

    case PACKAGE_HTML:
        // fall through - Trident will do the right thing by sniffing the 
        // extension.
    case PACKAGE_TEXT:
        {
            if (SUCCEEDED(hr))
            {
                IHTMLDocument2 *pDocDesign = NULL;
                IHTMLDocument2 *pDocSave = NULL;
                IPersistFile *ppf = NULL;

                if (cpDst == CP_ACP)
                {
                    // No encoding change, use the browse doc
                    pDocSave = pDoc;
                }
                else
                {
                    hr = _GetDesignDoc( pDoc, &pDocDesign, pfCancel, ptp, cpDst);

                    if (SUCCEEDED(hr))
                    {
                        pDocSave = pDocDesign;
                    }
                    else
                    {
                        return E_FAIL;
                    }
                }
                
                // Trident IPersistFile::Save looks at the extension to determine if it's
                // an HTML or text save.

                hr = pDocSave->QueryInterface(IID_IPersistFile, (void**)&ppf);

                if (SUCCEEDED(hr))
                {
                    LPCWSTR lpwszFile;
                    lpwszFile = lpstrDoc;
                    BSTR bstrURL = NULL;
                    WCHAR wzSavingText[MAX_SAVING_STATUS_TEXT + 1];
                    WCHAR wzBuf[INTERNET_MAX_URL_LENGTH + MAX_SAVING_STATUS_TEXT + 1];

                    hr = pDocSave->get_URL(&bstrURL);

                    if (SUCCEEDED(hr))
                    {

                        MLLoadStringW(IDS_SAVING_STATUS_TEXT, wzSavingText,
                                      ARRAYSIZE(wzSavingText));
                
                        wnsprintfW(wzBuf, ARRAYSIZE(wzBuf),
                                   L"%ws: %ws", wzSavingText, bstrURL);
                        ptp->SetSaveText(wzBuf);
                    
                        if (bstrURL)
                        {
                            SysFreeString(bstrURL);
                        }
                    }

                    hr = ppf->Save( lpwszFile, FALSE );
                    ppf->SaveCompleted(lpwszFile);

                    ppf->Release();
                }

                if (cpDst != CP_ACP)
                {
                    pDocSave->Release();
                }

                // If we used the browse-time pDoc, we don't need to release
                // it because it is released by CThicketUI::ThicketUIThreadProc
            }
        }
        break;

    default:
        ASSERT(FALSE);
        break;
    }

    return hr;
}

HRESULT CDocumentPackager::_PackageDocument(IHTMLDocument2 *pDoc,
                                            LPCTSTR lpstrDoc,
                                            BOOL *pfCancel, CThicketProgress *ptp,
                                            ULONG progLow, ULONG progHigh,
                                            UINT cpDst,
                                            CWebArchive *pwa,
                                            CDocumentPackager *pdpFrames,
                                            BOOL fFrameDoc)
{
    HRESULT                 hr = S_OK;
    ULONG                   cImages;
    ULONG                   cInputImgs;
    ULONG                   cBGSounds;
    ULONG                   cFrames;
    ULONG                   uRange = progHigh - progLow;
    ULONG                   uRangeThis;
    ULONG                   uLow, uHigh;
    IHTMLElementCollection  *pCollect = NULL;
    CImagePackager          imgPkgr;
    CInputImgPackager       inputimgPkgr;
    CBGSoundsPackager       bgsPkgr;
    CBackgroundPackager     bkgndPkgr;
    CBaseNeutralizer        baseNeut;
    CAnchorAdjustor         anchorAdj;
    CAreaAdjustor           areaAdj;
    CFramesPackager         framesPkgr;
    CSSPackager             stylesheetPkgr;
    CDynSrcPackager         dynsrcPkgr;
    CScriptPackager         scriptPkgr;
    IHTMLDocument2          *pDocDesign = NULL;
    BYTE                     abBuffer[MAX_BUFFER_LEN];
    DWORD                    dwType = 0;
    DWORD                    dwSize = 0;
    BOOL                     bDLImages = TRUE;
    HKEY                     hkey = 0;
    IOleCommandTarget       *pIOCT = NULL;

    if (pDoc==NULL)
        return E_INVALIDARG;

    hr = _GetDesignDoc( pDoc, &pDocDesign, pfCancel, ptp, cpDst );
    if (FAILED(hr))
        goto error;

    // HACK! If you have a unicode character in the filename, when we
    // call put_href on the CSS, trident tries to download this. The
    // invalid character is sent to the server, who sends badddddd
    // stuff, which the CSS parser doesn't understand. The result is
    // that trident falls on the floor. This tells trident not to download
    // the CSS hence avoiding the problem.

    hr = pDocDesign->QueryInterface(IID_IOleCommandTarget, (void **)&pIOCT);

    if (SUCCEEDED(hr) && pIOCT)
    {
        pIOCT->Exec(NULL, OLECMDID_DONTDOWNLOADCSS, OLECMDID_DONTDOWNLOADCSS,
                    NULL, NULL);
        pIOCT->Release();
    }

    hr = pDocDesign->get_all(&pCollect);
    if (FAILED(hr))
        RRETURN(hr);
    
    dwSize = MAX_BUFFER_LEN;
    if (RegOpenKey(HKEY_CURRENT_USER, REGPATH_MSIE_MAIN, &hkey) == ERROR_SUCCESS)
    {
        if (SHQueryValueExA(hkey, REGVALUE_DOWNLOAD_IMAGES, 0, &dwType,
                            abBuffer, &dwSize) == NO_ERROR)
        {
            bDLImages = !StrCmpIA((char *)abBuffer, "yes");
        }

        RegCloseKey(hkey);
    }

    if (bDLImages)
    {
        // pack all the images into the message and remember the Thicket mappings
        hr = imgPkgr.InitFromCollection(pCollect, &cImages);
        if (FAILED(hr))
            goto error;

        hr = inputimgPkgr.InitFromCollection(pCollect, &cInputImgs);
        if (FAILED(hr))
            goto error;

    }

    hr = bgsPkgr.InitFromCollection(pCollect, &cBGSounds);
    if (FAILED(hr))
        goto error;
   
    hr = bkgndPkgr.Init(pCollect, NULL, pDocDesign);
    if (FAILED(hr))
        goto error;

    hr = dynsrcPkgr.Init(pCollect, NULL, pDocDesign);
    if (FAILED(hr))
        goto error;

    hr = stylesheetPkgr.Init(pCollect, NULL, pDocDesign);
    if (FAILED(hr))
        goto error;

    hr = framesPkgr.Init(pCollect, &cFrames, pDoc, pDocDesign, this);
    if (FAILED(hr))
        goto error;

    hr = scriptPkgr.Init(pCollect, NULL, pDocDesign);
    if (FAILED(hr))
        goto error;

    hr = pwa->Init(lpstrDoc, cImages + cInputImgs + cFrames);
    if (FAILED(hr))
        goto error;

    // herewith commences the hackery to drive the progess bar.
    // If we have frames we devide the progress range among all the docs involved.
    // We'll neglect style sheets and devote the range for the immediate
    // document to the image collection.
    uRangeThis = uRange / (cFrames + 1);

    uLow = progLow;
    uHigh = progLow + uRangeThis;
    
    if (bDLImages)
    {
        hr = imgPkgr.PackageData(pwa, pfCancel, ptp, uLow, uHigh);
        if (FAILED(hr))
            goto error;

        hr = inputimgPkgr.PackageData(pwa, pfCancel, ptp, uLow, uHigh);
        if (FAILED(hr))
            goto error;
    }

    hr = bgsPkgr.PackageData(pwa, pfCancel, ptp, uLow, uHigh);
    if (FAILED(hr))
        goto error;
     
    hr = bkgndPkgr.PackageData(pwa, pfCancel);
    if (FAILED(hr))
        goto error;

    hr = dynsrcPkgr.PackageData(pwa, pfCancel);
    if (FAILED(hr))
        goto error;
    
    hr = stylesheetPkgr.PackageStyleSheets(pDocDesign, pwa);
    if (FAILED(hr))
        goto error;
 
    uLow = progHigh - uRangeThis;
    uHigh = progHigh;

    hr = framesPkgr.PackageData(pwa, pfCancel, ptp, uLow, uHigh);
    if (FAILED(hr))
        goto error;

    hr = scriptPkgr.PackageData(pwa, pfCancel);
    if (FAILED(hr))
        goto error;
                
    // we want to do this after frames s.t. the frame docs will be in the thicket
    // and we can correctly direct a targetted hyperlink from frame A to frame B
    // if the href is in the thicket vs. still out on the Web.
    hr = anchorAdj.InitFromCollection(pCollect);
    if (FAILED(hr))
        goto error;
    
    hr = anchorAdj.PackageData(pwa, pfCancel); // not that we need the thicket...
    if (FAILED(hr))
        goto error;  
    
    hr = areaAdj.InitFromCollection(pCollect);
    if (FAILED(hr))
        goto error;
    
    hr = areaAdj.PackageData(pwa, pfCancel); // not that we need the thicket...
    if (FAILED(hr))
        goto error;   

 
    // Now that we've got everybody remapped, short-circuit the base tags
    // and redirect to the current directory.
    hr = baseNeut.InitFromCollection(pCollect, NULL, pDocDesign );
    if (FAILED(hr))
        goto error;
    
    hr = baseNeut.PackageData(pwa, pfCancel);
    if (FAILED(hr))
        goto error;
        
    //if(dwFlags & MECD_HTML || dwFlags & MECD_PLAINTEXT)
    {
        hr = pwa->ArchiveDocumentText( pDocDesign, cpDst, fFrameDoc );
        if (FAILED(hr))
            goto error;
    }

    
error:
    
    if (pCollect)
        pCollect->Release();

    if (pDocDesign)
        pDocDesign->Release();

    if (pfCancel && *pfCancel)
        hr = E_ABORT;

    if (SUCCEEDED(hr))
        pwa->Commit();
    else
        pwa->Revert();
    
    return hr;
}

CWebArchive *CDocumentPackager::GetFrameDocArchive(CWebArchive *pwaSrc)
{
    CWebArchive *pwa = NULL;

    if (m_iPackageStyle == PACKAGE_THICKET)
        pwa = new CThicketArchive(m_ptp);
    else if (m_iPackageStyle == PACKAGE_MHTML)
        pwa = pwaSrc;
    else
        ASSERT(FALSE);

    return pwa;
}

HRESULT CDocumentPackager::_GetDesignDoc( IHTMLDocument2 *pDocSrc, IHTMLDocument2 **ppDocDesign,
                                          BOOL *pfCancel, CThicketProgress *ptp, UINT cpDst )
{
    HRESULT            hr;
    DWORD              dwFlags;
    BSTR               bstrURL = NULL;
    BSTR               bstrCharSetSrc = NULL;
    MIMECSETINFO       csetInfo;
    IMultiLanguage2   *pMultiLanguage = NULL;
    CUrlDownload      *pud = NULL;
    ULONG              cRef = 0;
    DWORD              dwUrlEncodingDisableUTF8;
    DWORD              dwSize = SIZEOF(dwUrlEncodingDisableUTF8);
    BOOL               fDefault = FALSE;

    hr = pDocSrc->get_charset(&bstrCharSetSrc);
    if (FAILED(hr))
        goto Cleanup;

    hr = CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER,
                          IID_IMultiLanguage2, (void**)&pMultiLanguage);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    hr = pMultiLanguage->GetCharsetInfo(bstrCharSetSrc, &csetInfo);
    if (FAILED(hr))
    {
        goto Cleanup;
    }
    
    pud = new CUrlDownload( ptp, &hr, csetInfo.uiInternetEncoding );

    if (pud == NULL)
    {
        return E_OUTOFMEMORY;
    }

   if (FAILED(pDocSrc->get_URL( &bstrURL )))
        goto Cleanup;

    *ppDocDesign = NULL;

    //BUGBUG seanf(2/6/98): Review DLCTL_ flags.
    dwFlags = DLCTL_NO_SCRIPTS | DLCTL_NO_JAVA | DLCTL_NO_RUNACTIVEXCTLS | DLCTL_NO_FRAMEDOWNLOAD |
              DLCTL_SILENT | DLCTL_OFFLINE;

    SHRegGetUSValue(REGSTR_PATH_INTERNET_SETTINGS,
        TEXT("UrlEncoding"), NULL, (LPBYTE) &dwUrlEncodingDisableUTF8, &dwSize, FALSE, (LPVOID) &fDefault, SIZEOF(fDefault));

    if (dwUrlEncodingDisableUTF8)
    {
        dwFlags |= DLCTL_URL_ENCODING_DISABLE_UTF8;
    }
    else
    {
        dwFlags |= DLCTL_URL_ENCODING_ENABLE_UTF8;
    }

    hr = pud->SetDLCTL(dwFlags);
    if (SUCCEEDED(hr))
        hr = pud->BeginDownloadURL2( bstrURL, BDU2_BROWSER, BDU2_NONE, NULL, 0xF0000000 );

    if (SUCCEEDED(hr))
    {
        MSG msg;

        hr = S_FALSE;

        while (hr==S_FALSE)
        {
            GetMessage(&msg, NULL, 0, 0);
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (*pfCancel)
            {
                pud->AbortDownload();
                hr = E_ABORT;
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = pud->GetDocument( ppDocDesign );

            // Set the document to the codepage the user has selected.
            // Don't bother if it's no specific page has been directed, as is the case
            // with frame documents and in cases where the user kept the default
            // code page selected in the Save As... dialog.
            if (SUCCEEDED(hr) && cpDst != CP_ACP)
            {
                MIMECPINFO  cpInfo;
                BSTR        bstrCharSet = NULL;
                LANGID      langid;

                langid = MLGetUILanguage();

                if ( SUCCEEDED(pMultiLanguage->GetCodePageInfo(cpDst, langid, &cpInfo)) &&
                     (bstrCharSet = SysAllocString(cpInfo.wszWebCharset)) != NULL )
                    hr = (*ppDocDesign)->put_charset(bstrCharSet);

                ASSERT(SUCCEEDED(hr));

                if (bstrCharSet)
                    SysFreeString(bstrCharSet);
            }
        }
    }

    pud->DoneDownloading();

    cRef = pud->Release();

    if (SUCCEEDED(hr))
    {
        IOleCommandTarget *pioct;

        hr = pDocSrc->QueryInterface(IID_IOleCommandTarget, (LPVOID*)&pioct);
        if (SUCCEEDED(hr))
        {
            VARIANTARG v;

            v.vt = VT_UNKNOWN;
            v.punkVal = *ppDocDesign;

            hr = pioct->Exec( &CGID_ShortCut, CMDID_SAVEASTHICKET, OLECMDEXECOPT_DODEFAULT, &v, NULL );

            pioct->Release();
        }
    }

Cleanup:

    SAFERELEASE(pMultiLanguage);

    if (bstrURL)
        SysFreeString(bstrURL);

    if (bstrCharSetSrc)
        SysFreeString(bstrCharSetSrc);

    if (FAILED(hr))
    {
        if (ppDocDesign != NULL)
        {
            ReleaseInterface((*ppDocDesign));
        }
    }

    return hr;
}

/*
*  CSSPackager ##################################################
*/

HRESULT CSSPackager::Init(IHTMLElementCollection *pColl,
                          ULONG *pcElems,
                          IHTMLDocument2 *pDoc)
{
    HRESULT hr = CRelativeURLPackager::Init( pColl, pcElems, pDoc );
            
    m_pDoc = pDoc;

    RRETURN(hr);
}


HRESULT CSSPackager::PackageStyleSheets(IHTMLDocument2 *pDoc2,
                                        CWebArchive *pwa)
{
    HRESULT hr = S_OK;
    IHTMLStyleSheetsCollection *pssc = NULL;
    
    ASSERT(pDoc2);
    ASSERT(pwa);
    
    // process the inline style sheets
    hr = pDoc2->get_styleSheets( &pssc );
    if (SUCCEEDED(hr))
    {
        hr = _PackageSSCollection(pssc, pwa);
        pssc->Release();
    }
    
    return hr; // no RRETURN - may return S_FALSE
}


HRESULT CSSPackager::_PackageSSCollection(IHTMLStyleSheetsCollection *pssc,
                                          CWebArchive *pwa)
{
    HRESULT hr;
    LONG cSS;
    
    hr = pssc->get_length( &cSS );
    if (SUCCEEDED(hr))
    {
        LONG iSS;
        
        for (iSS = 0; iSS < cSS && SUCCEEDED(hr); iSS++ )
        {
            VARIANT varIndex;
            VARIANT varSS;
            
            varIndex.vt = VT_I4;
            varIndex.lVal = iSS;
            varSS.vt = VT_EMPTY;
            hr = pssc->item( &varIndex, &varSS );
            if (SUCCEEDED(hr) && varSS.vt == VT_DISPATCH && varSS.pdispVal != NULL)
            {
                IHTMLStyleSheet *pss = NULL;
                if(SUCCEEDED(varSS.pdispVal->QueryInterface(IID_IHTMLStyleSheet, (void**)&pss)))
                {
                    hr = _PackageSS(pss, pwa); 
                    pss->Release();
                }
                varSS.pdispVal->Release();
            }
        }
    }
    return hr; // no RRETURN - may return S_FALSE
}


HRESULT CSSPackager::_PackageSS(IHTMLStyleSheet *pss,
                                CWebArchive *pwa)
{
    HRESULT     hr;
    BSTR        bstrRelURL = NULL;
    BSTR        bstrAbsURL = NULL;
    LONG        lElemPos;
    IHTMLElement *pElemOwner = NULL;
    IHTMLStyleSheetsCollection *pssc = NULL;
    BOOL        fStyleTag = FALSE;
    
    if (pss == NULL || pwa == NULL)
        return E_INVALIDARG;
    
    hr = pss->get_href(&bstrRelURL);
    if (FAILED(hr))
        goto error;
    fStyleTag = bstrRelURL == NULL || *bstrRelURL == 0;
    
    hr = pss->get_owningElement(&pElemOwner);
    if (FAILED(hr))
        goto error;
    
    hr = pElemOwner->get_sourceIndex(&lElemPos);
    if (FAILED(hr))
        goto error;
    
    hr = HrGetCombinedURL(m_pCollBase, m_cBase, lElemPos, bstrRelURL, m_bstrDocURL, &bstrAbsURL);
    if (FAILED(hr))
        goto error;
    
    // First we do the defualt processing, gathering the imports into _our_
    
    // process the inline style sheets
    hr = pss->get_imports( &pssc );
    if (SUCCEEDED(hr))
    {
        long cSS;
        
        hr = pssc->get_length( &cSS );
        if (SUCCEEDED(hr) && cSS > 0)
        {
            CSSPackager importPkgr;
            
            hr = importPkgr.Init(m_pCollBase, NULL, m_pDoc);
            
            hr = importPkgr._PackageSSCollection(pssc, pwa);
        }
        pssc->Release();
    }
    
    // oh, yeah, if we want to do background-image and list-style-image, we'd enumerate this ss's rule styles
    // here, find the ones with these attributes, and build a list of IHTML rule style, maybe using some sub-obj
    // like an image packager.
    
    if (SUCCEEDED(hr) && !fStyleTag)
    {
        BSTR    bstrSSText;
        
        // Now we grab our modified text and add it to the document.
        hr = pss->get_cssText(&bstrSSText);
        if (SUCCEEDED(hr) && bstrSSText != NULL)
        {
            LPSTR lpszSSText;
            
            // This text needs to be ANSI before we put it into the stream.
            hr = HrBSTRToLPSZ( bstrSSText, &lpszSSText );
            if (SUCCEEDED(hr))
            {
                // PTH hr = MimeOleCreateVirtualStream(&pstm);
                TCHAR       szStyleDoc[MAX_PATH];
                CHashEntry  *phe;

                hr = pwa->AddFrameOrStyleEntry( bstrAbsURL, &phe, szStyleDoc );
                
                if (hr==S_OK)
                {
                    hr = pwa->ArchiveCSSText( bstrAbsURL, lpszSSText, szStyleDoc );

                    if ( SUCCEEDED(hr) )
                        hr = pss->put_href(phe->m_bstrValue);
                }
                else if (hr==S_FALSE)
                {
                    // repeated style sheet, don't need to do all the work, but do need to note
                    // the ss for remapping
                    hr = pss->put_href( phe->m_bstrValue);
                }
                delete lpszSSText;
            }
            SysFreeString(bstrSSText);
        }
    }
    
error:
    if (pElemOwner)
        pElemOwner->Release();
    if (bstrRelURL)
        SysFreeString(bstrRelURL);
    if (bstrAbsURL)
        SysFreeString(bstrAbsURL);
    
    return hr; // no RRETURN - may return S_FALSE
}

//
// Functions ##############################################################
//

HRESULT HrGetElement(IHTMLDocument2 *pDoc, LPCSTR pszName, IHTMLElement **ppElem)
{
    HRESULT                 hr = E_FAIL;
    IHTMLElementCollection *pCollect = NULL;
    IDispatch              *pDisp = NULL;
    VARIANTARG              va1, va2;
    
    if (pDoc)
    {
        pDoc->get_all(&pCollect);
        if (pCollect)
        {
            if (SUCCEEDED(HrLPSZToBSTR(pszName, &va1.bstrVal)))
            {
                va1.vt = VT_BSTR;
                va2.vt = VT_EMPTY;
                pCollect->item(va1, va2, &pDisp);
                if (pDisp)
                {
                    hr = pDisp->QueryInterface(IID_IHTMLElement, (LPVOID*)ppElem);
                    pDisp->Release();
                }
                SysFreeString(va1.bstrVal);
            }
            pCollect->Release();
        }
    }
    return hr; // no RRETURN - may return S_FALSE
    
}


HRESULT HrGetBodyElement(IHTMLDocument2 *pDoc, IHTMLBodyElement **ppBody)
{
    HRESULT         hr=E_FAIL;
    IHTMLElement    *pElem=0;
    
    if (ppBody == NULL)
        return E_INVALIDARG;
    
    *ppBody = 0;
    if (pDoc)
    {
        pDoc->get_body(&pElem);
        if (pElem)
        {
            hr = pElem->QueryInterface(IID_IHTMLBodyElement, (LPVOID *)ppBody);
            pElem->Release();
        }
    }
    return hr; // no RRETURN - may return S_FALSE
}

HRESULT HrGetMember(LPUNKNOWN pUnk, BSTR bstrMember,LONG lFlags, BSTR *pbstr)
{
    IHTMLElement    *pObj;
    HRESULT         hr;
    VARIANT         rVar;
    
    hr = pUnk->QueryInterface(IID_IHTMLElement, (LPVOID *)&pObj);
    if (SUCCEEDED(hr))
    {
        ASSERT (pObj);
        
        rVar.vt = VT_BSTR;
        hr = pObj->getAttribute(bstrMember, lFlags, &rVar);
        if (SUCCEEDED(hr) && rVar.vt == VT_BSTR && rVar.bstrVal != NULL)
        {
            *pbstr = rVar.bstrVal;
        }
        pObj->Release();
    }
    return hr; // no RRETURN - may return S_FALSE
}



ULONG UlGetCollectionCount(IHTMLElementCollection *pCollect)
{
    ULONG   ulCount=0;
    
    if (pCollect)
        pCollect->get_length((LONG *)&ulCount);
    
    return ulCount;
}

HRESULT HrGetCollectionItem(IHTMLElementCollection *pCollect, ULONG uIndex, REFIID riid, LPVOID *ppvObj)
{
    HRESULT     hr=E_FAIL;
    IDispatch   *pDisp=0;
    VARIANTARG  va1,
        va2;
    
    va1.vt = VT_I4;
    va2.vt = VT_EMPTY;
    va1.lVal = (LONG)uIndex;
    
    pCollect->item(va1, va2, &pDisp);
    if (pDisp)
    {
        hr = pDisp->QueryInterface(riid, ppvObj);
        pDisp->Release();
    }
    return hr; // no RRETURN - may return S_FALSE
}

HRESULT HrGetCollectionOf(IHTMLDocument2 *pDoc, BSTR bstrTagName, IHTMLElementCollection **ppCollect)
{
    VARIANT                 v;
    IDispatch               *pDisp=0;
    IHTMLElementCollection  *pCollect=0;
    HRESULT                 hr;
    
    ASSERT(ppCollect);
    ASSERT(bstrTagName);
    ASSERT(pDoc);
    
    *ppCollect = NULL;
    
    hr = pDoc->get_all(&pCollect);
    if (pCollect)
    {
        v.vt = VT_BSTR;
        v.bstrVal = bstrTagName;
        pCollect->tags(v, &pDisp);
        if (pDisp)
        {
            hr = pDisp->QueryInterface(IID_IHTMLElementCollection, (LPVOID *)ppCollect);
            pDisp->Release();
        }
        pCollect->Release();
    }
    else if (S_OK == hr)
        hr = E_FAIL;
    
    return hr; // no RRETURN - may return S_FALSE
}

HRESULT HrSetMember(LPUNKNOWN pUnk, BSTR bstrMember, BSTR bstrValue)
{
    IHTMLElement    *pObj;
    HRESULT         hr;
    VARIANT         rVar;
    
    ASSERT(pUnk);

    hr = pUnk->QueryInterface(IID_IHTMLElement, (LPVOID *)&pObj);
    if (SUCCEEDED(hr))
    {
        ASSERT (pObj);
        rVar.vt = VT_BSTR;
        rVar.bstrVal = bstrValue;
        hr = pObj->setAttribute(bstrMember, rVar, FALSE);
        pObj->Release();
    }
    return hr; // no RRETURN - may return S_FALSE
}


/*
* HrGetCombinedURL does some of the things that GetBackgroundImageUrl
* does, but in a more general way. It relies on the caller to have
* isolated the <BASE> collection and to supply the root document URL.
* While a trifle awkward, it is more efficient if the caller is going
* to combine many URLS.
*/

HRESULT HrGetCombinedURL( IHTMLElementCollection *pCollBase,
                         LONG cBase,
                         LONG lElemPos,
                         BSTR bstrRelURL,
                         BSTR bstrDocURL,
                         BSTR *pbstrBaseURL)
{
    HRESULT             hr = S_FALSE;
    IHTMLElement        *pElemBase;
    IHTMLBaseElement    *pBase;
    LONG                lBasePos=0,
        lBasePosSoFar=0;
    BSTR                bstr = NULL;
    LPWSTR              pszUrlW=0;
    WCHAR               szBaseW[INTERNET_MAX_URL_LENGTH];
    WCHAR               szUrlW[INTERNET_MAX_URL_LENGTH];
    DWORD               cch=INTERNET_MAX_URL_LENGTH;
    LONG                i;
    
    *pbstrBaseURL = 0;
    *szBaseW = 0;
    
    for (i=0; i<cBase; i++)
    {
        if (SUCCEEDED(HrGetCollectionItem(pCollBase, i, IID_IHTMLElement, (LPVOID *)&pElemBase)))
        {
            pElemBase->get_sourceIndex(&lBasePos);
            if (lBasePos < lElemPos &&
                lBasePos >= lBasePosSoFar)
            {
                if (SUCCEEDED(pElemBase->QueryInterface(IID_IHTMLBaseElement, (LPVOID *)&pBase)))
                {
                    bstr = NULL;
                    if (pBase->get_href(&bstr)==S_OK && bstr != NULL)
                    {
                        ASSERT (bstr);
                        if (*bstr)
                        {
                            StrCpyNW(szBaseW, bstr, ARRAYSIZE(szBaseW));
                            lBasePosSoFar = lBasePos;
                        }
                        SysFreeString(bstr);
                    }
                    pBase->Release();
                }
            }
            pElemBase->Release();
        }
    }
    
    if (szBaseW[0] == 0 && bstrDocURL)
    {
        // We didn't find a <BASE> tag before our element, so fall back to using
        // the document's location as basis for the base
        StrCpyNW( szBaseW, bstrDocURL, ARRAYSIZE(szBaseW) );
    }
    
#ifndef WIN16  //RUN16_BLOCK - UrlCombineW is not available
    // if there's a <BASE> then do the combine
    if (*szBaseW && 
        SUCCEEDED(UrlCombineW(szBaseW, bstrRelURL, szUrlW, &cch, 0)))
        pszUrlW = szUrlW;
#endif //!WIN16
    
    // pszUrlW contains the combined <BODY> and <BASE> tag, return this.
    if (pszUrlW)
        *pbstrBaseURL = SysAllocString(pszUrlW);
    
    return (*pbstrBaseURL == NULL ? S_FALSE : S_OK);
}

HRESULT HrLPSZToBSTR(LPCSTR lpsz, BSTR *pbstr)
{
    HRESULT hr = NOERROR;
    BSTR    bstr=0;
    ULONG   cch = 0, ccb,
        cchRet;
    UINT cp = GetACP();
    
    // get byte count
    ccb = lstrlenA(lpsz);
    
    // get character count - DBCS string ccb may not equal to cch
    cch=MultiByteToWideChar(cp, 0, lpsz, ccb, NULL, 0);
    if(cch==0 && ccb!=0)
    {
        ASSERT(FALSE);
        hr=E_FAIL;
        goto error;
    }
    
    // allocate a wide-string with enough character to hold string - use character count
    bstr=SysAllocStringLen(NULL, cch);
    if (!bstr)
    {
        hr=E_OUTOFMEMORY;
        goto error;
    }
    
    cchRet=MultiByteToWideChar(cp, 0, lpsz, ccb, (LPWSTR)bstr, cch);
    if (cchRet==0 && ccb!=0)
    {
        hr=E_FAIL;
        goto error;
    }
    
    *pbstr = bstr;
    bstr=0;             // freed by caller
    
error:
    if (bstr)
        SysFreeString(bstr);   
    RRETURN(hr);
    
}

HRESULT HrBSTRToLPSZ(BSTR bstr, LPSTR *lplpsz)
{
    ULONG     cch = 0;
    
    ASSERT (bstr && lplpsz);
    
    cch = WideCharToMultiByte(CP_ACP, 0, bstr, -1, NULL, 0, NULL, NULL);

    if (!cch)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    *lplpsz = new char[cch + 1];

    if (!*lplpsz)
    {
        return E_OUTOFMEMORY;
    }
    
    if (WideCharToMultiByte(CP_ACP, 0, bstr, -1, *lplpsz, cch+1, NULL, NULL))
        return S_OK;
    else
        return HRESULT_FROM_WIN32(GetLastError());
}

void RemoveBookMark(WCHAR *pwzURL, WCHAR **ppwzBookMark)
{
    if (pwzURL && ppwzBookMark)
    {
        *ppwzBookMark = pwzURL;

        while (**ppwzBookMark)
        {
            if (**ppwzBookMark == L'#')
            {
                **ppwzBookMark = L'\0';
                break;
            }

            (*ppwzBookMark)++;
        }
    }
}

void RestoreBookMark(WCHAR *pwzBookMark)
{
    if (pwzBookMark)
    {
        *pwzBookMark = L'#';
    }
}

