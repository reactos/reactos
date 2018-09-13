/*
 *  @doc INTERNAL
 *
 *  @module - DXFROBJ.C |
 *
 *      implementation of a generic IDataObject data transfer object.
 *      This object is suitable for use in OLE clipboard and drag drop
 *      operations
 *
 *  Author: <nl>
 *      alexgo (4/25/95)
 *
 *  Revisions: <nl>
 *      murrays (7/13/95) autodoc'd and added cf_RTF
 */

#include "headers.hxx"

#ifndef X_CODEPAGE_H_
#define X_CODEPAGE_H_
#include "codepage.h"
#endif

#ifndef X__DXFROBJ_H_
#define X__DXFROBJ_H_
#include "_dxfrobj.h"
#endif

#ifndef X__TXTSAVE_H_
#define X__TXTSAVE_H_
#include "_txtsave.h"
#endif

#ifndef X_HASH_H_
#define X_HASH_H_
#include "hash.h"
#endif

#ifndef X_RTFTOHTM_HXX_
#define X_RTFTOHTM_HXX_
#include "rtftohtm.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef _X_SELDRAG_HXX_
#define _X_SELDRAG_HXX_
#include "seldrag.hxx"
#endif

#ifndef X__TXTSAVE_H_
#define X__TXTSAVE_H_
#include "_txtsave.h"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

MtDefine(CTextXBag, Tree, "CTextXBag")
MtDefine(CTextXBag_prgFormats, CTextXBag, "CTextXBag::_prgFormats")

//
//  Common Data types
//

// If you change g_rgFETC[], change g_rgDOI[] and enum FETCINDEX and CFETC in
// _dxfrobj.h accordingly, and register nonstandard clipboard formats in
// RegisterFETCs(). Order entries in order of most desirable to least, e.g.,
// RTF in front of plain text.

FORMATETC g_rgFETC[] =
{
    {0,                 NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL}, // CF_HTML
    {0,                 NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL}, // CF_RTF
#ifndef WIN16
    {CF_UNICODETEXT,    NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
#endif // !WIN16
    {CF_TEXT,           NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
//    {0,                 NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL}, // Filename
    {0,                 NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL}, // CF_RTFASTEXT
    {0,                 NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL}, // CF_FILEDESCRIPTORA
    {0,                 NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL}, // CF_FILEDESCRIPTORW
    {0,                 NULL, DVASPECT_CONTENT,  0, TYMED_HGLOBAL}, // CF_FILECONTENTS
    {0,                 NULL, DVASPECT_CONTENT,  0, TYMED_HGLOBAL}, // CF_UNIFORMRESOURCELOCATOR
//    {0,                 NULL, DVASPECT_CONTENT, -1, TYMED_ISTORAGE},// EmbObject
//    {0,                 NULL, DVASPECT_CONTENT, -1, TYMED_ISTORAGE},// EmbSource
//    {0,                 NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL}, // ObjDesc
//    {0,                 NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL}, // LnkSource
//    {CF_METAFILEPICT,   NULL, DVASPECT_CONTENT, -1, TYMED_MFPICT},
//    {CF_DIB,            NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
//    {CF_BITMAP,         NULL, DVASPECT_CONTENT, -1, TYMED_GDI},
//    {0,                 NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL}, // RTF with no objs
//    {0,                 NULL, DVASPECT_CONTENT, -1, TYMED_ISTORAGE},// Text with objs
//    {0,                 NULL, DVASPECT_CONTENT, -1, TYMED_ISTORAGE},// Richedit
};

int CFETC = ARRAY_SIZE( g_rgFETC );



//TODO v-richa check the added members for correctness
DWORD g_rgDOI[] =
{
    DOI_CANPASTEPLAIN,
    DOI_CANPASTEPLAIN,
#ifndef WIN16
    DOI_CANPASTEPLAIN,
#endif // !WIN16
    DOI_CANPASTEPLAIN,
//    DOI_CANPASTEOLE,
    DOI_CANPASTEPLAIN,
    DOI_NONE,
    DOI_NONE,
    DOI_NONE,
    DOI_NONE,
//    DOI_CANPASTEOLE,
//    DOI_CANPASTEOLE,
//    DOI_NONE,
//    DOI_CANPASTEOLE,
//    DOI_CANPASTEOLE,
//    DOI_CANPASTEOLE,
//    DOI_CANPASTEOLE,
//    DOI_CANPASTERICH,
//    DOI_CANPASTERICH,
//    DOI_CANPASTERICH
};


/*
 *  RegisterFETCs()
 *
 *  @func
 *      Register nonstandard format ETCs.  Called when DLL is loaded
 *
 *  @todo
 *      Register other RTF formats (and add places for them in g_rgFETC[])
 */
void RegisterFETCs()
{
#ifdef RTF_HASHCACHE
    HashKeyword_Init();         // init the rtf control keyword hash table.
#endif

    if(!g_rgFETC[iHTML].cfFormat)
        g_rgFETC[iHTML].cfFormat
            = (CLIPFORMAT)RegisterClipboardFormatA("HTML Format");

    if(!g_rgFETC[iRtfFETC].cfFormat)
        g_rgFETC[iRtfFETC].cfFormat
            = (CLIPFORMAT)RegisterClipboardFormatA("Rich Text Format");

    if(!g_rgFETC[iRtfAsTextFETC].cfFormat)
        g_rgFETC[iRtfAsTextFETC].cfFormat
            = (CLIPFORMAT)RegisterClipboardFormatA("RTF As Text");

    if(!g_rgFETC[iFileDescA].cfFormat)
        g_rgFETC[iFileDescA].cfFormat
            = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILEDESCRIPTORA);

    if(!g_rgFETC[iFileDescW].cfFormat)
        g_rgFETC[iFileDescW].cfFormat
            = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILEDESCRIPTORW);

    if(!g_rgFETC[iFileContents].cfFormat)
        g_rgFETC[iFileContents].cfFormat
            = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILECONTENTS);

    if(!g_rgFETC[iUniformResourceLocator].cfFormat)
        g_rgFETC[iUniformResourceLocator].cfFormat
            = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_SHELLURL);

//    if(!g_rgFETC[iRichEdit].cfFormat)
//        g_rgFETC[iRichEdit].cfFormat
//            = (CLIPFORMAT)RegisterClipboardFormatA("RICHEDIT");

//    if(!g_rgFETC[iObtDesc].cfFormat)
//        g_rgFETC[iObtDesc].cfFormat
//            = (CLIPFORMAT)RegisterClipboardFormatA(CF_OBJECTDESCRIPTOR);

//    if(!g_rgFETC[iEmbObj].cfFormat)
//        g_rgFETC[iEmbObj].cfFormat
//            = (CLIPFORMAT)RegisterClipboardFormatA(CF_EMBEDDEDOBJECT);

//    if(!g_rgFETC[iEmbSrc].cfFormat)
//        g_rgFETC[iEmbSrc].cfFormat
//            = (CLIPFORMAT)RegisterClipboardFormatA(CF_EMBEDSOURCE);

//    if(!g_rgFETC[iLnkSrc].cfFormat)
//        g_rgFETC[iLnkSrc].cfFormat
//            = (CLIPFORMAT)RegisterClipboardFormatA(CF_LINKSOURCE);

//    if(!g_rgFETC[iRtfNoObjs].cfFormat)
//        g_rgFETC[iRtfNoObjs].cfFormat
//            = (CLIPFORMAT)RegisterClipboardFormatA("Rich Text Format Without Objects");

//    if(!g_rgFETC[iTxtObj].cfFormat)
//        g_rgFETC[iTxtObj].cfFormat
//            = (CLIPFORMAT)RegisterClipboardFormatA("RichEdit Text and Objects");

//    if(!g_rgFETC[iFilename].cfFormat)
//        g_rgFETC[iFilename].cfFormat
//            = (CLIPFORMAT)RegisterClipboardFormatA(CF_FILENAME);
}


//
//  CTextXBag PUBLIC methods
//

/*
 *  CTextXBag::EnumFormatEtc (dwDirection, ppenumFormatEtc)
 *
 *  @mfunc
 *      returns an enumerator which lists all of the available formats in
 *      this data transfer object
 *
 *  @rdesc
 *      HRESULT
 *
 *  @devnote
 *      we have no 'set' formats for this object
 */
STDMETHODIMP CTextXBag::EnumFormatEtc(
    DWORD dwDirection,                  // @parm DATADIR_GET/SET
    IEnumFORMATETC **ppenumFormatEtc)   // @parm out parm for enum FETC interface
{
    HRESULT hr = NOERROR;

    *ppenumFormatEtc = NULL;

    if (_pLinkDataObj && (_cTotal < _cFormatMax))
    {
        _prgFormats[ _cTotal++ ] = g_rgFETC[iFileDescA];
        _prgFormats[ _cTotal++ ] = g_rgFETC[iFileDescW];
        _prgFormats[ _cTotal++ ] = g_rgFETC[iFileContents];
        _prgFormats[ _cTotal++ ] = g_rgFETC[iUniformResourceLocator];
    }

    if (dwDirection == DATADIR_GET)
    {
        hr = CEnumFormatEtc::Create(_prgFormats, _cTotal, ppenumFormatEtc);
    }

    return hr;
}

/*
 *  CTextXBag::GetData (pformatetcIn, pmedium)
 *
 *  @mfunc
 *      retrieves data of the specified format
 *
 *  @rdesc
 *      HRESULT
 *
 *  @devnote
 *      The three formats currently supported are CF_UNICODETEXT on
 *      an hglobal, CF_TEXT on an hglobal, and CF_RTF on an hglobal
 *
 *  @todo (alexgo): handle all of the other formats as well
 */
STDMETHODIMP CTextXBag::GetData(
    FORMATETC *pformatetcIn,
    STGMEDIUM *pmedium )
{
    CLIPFORMAT  cf = pformatetcIn->cfFormat;
    HRESULT     hr = DV_E_FORMATETC;
    HGLOBAL     hGlobal;
    
    if (! (pformatetcIn->tymed & TYMED_HGLOBAL) )
        goto Cleanup;

    memset(pmedium, '\0', sizeof(STGMEDIUM));
    pmedium->tymed = TYMED_NULL;

    if (cf == cf_HTML)
        hGlobal = _hCFHTMLText;

#ifndef WIN16
    else if (cf == CF_UNICODETEXT)
        hGlobal = _hUnicodeText;
#endif // !WIN16

    else if (cf == CF_TEXT)
        hGlobal = _hText;

    else if (cf == cf_RTF || cf == cf_RTFASTEXT)
        hGlobal = _hRTFText;

    else
        goto Cleanup;

    if (hGlobal)
    {
        pmedium->tymed   = TYMED_HGLOBAL;
        pmedium->hGlobal = DuplicateHGlobal(hGlobal);
        if (!pmedium->hGlobal)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = S_OK;
    }

Cleanup:
    if (hr && _pLinkDataObj)
        hr = _pLinkDataObj->GetData(pformatetcIn, pmedium);
    RRETURN(hr);
}

/*
 *  CTextXBag::QueryGetData (pformatetc)
 *
 *  @mfunc
 *      Queries whether the given format is available in this data object
 *
 *  @rdesc
 *      HRESULT
 */
STDMETHODIMP CTextXBag::QueryGetData(
    FORMATETC *pformatetc )     // @parm FETC to look for
{
    DWORD   cFETC = _cTotal;
    CLIPFORMAT cf = pformatetc->cfFormat;

    while (cFETC--)             // Maybe faster to search from start
    {
        if( cf == _prgFormats[cFETC].cfFormat && 
            pformatetc->tymed & TYMED_HGLOBAL )
        {
            // Check for RTF handle even if we claim to support the format
            if (_hRTFText ||
                (cf != cf_RTF && cf != cf_RTFASTEXT))
            {
                return NOERROR;
            }
        }
    }

    if (_pLinkDataObj)
        return _pLinkDataObj->QueryGetData(pformatetc);

    return DV_E_FORMATETC;
}

STDMETHODIMP CTextXBag::SetData(LPFORMATETC pformatetc, STGMEDIUM FAR * pmedium, BOOL fRelease)
{
    _hUnicodeText = pmedium->hGlobal;
    if (!pmedium->hGlobal)
        _hText = NULL;

    return S_OK;
}

STDMETHODIMP CTextXBag::QueryInterface(REFIID iid, LPVOID * ppv)
{
    if (    _pSelDragDropSrcInfo
        &&  (iid == IID_IUnknown))
        return _pSelDragDropSrcInfo->QueryInterface(iid, ppv);
    else
        return super::QueryInterface(iid, ppv);
}

STDMETHODIMP_(ULONG) CTextXBag::AddRef()
{
    return _pSelDragDropSrcInfo ? _pSelDragDropSrcInfo->AddRef() : super::AddRef();
}

STDMETHODIMP_(ULONG) CTextXBag::Release()
{
    return _pSelDragDropSrcInfo ? _pSelDragDropSrcInfo->Release() : super::Release();
}

//+------------------------------------------------------------------------
//
//  Member:     CTextXBag::Create
//
//  Synopsis:   Static creator of text xbags
//
//  Arguments:  pMarkup         The markup that owns the selection
//              fSupportsHTML   Can the selection be treated as HTML?
//              ppRange         Array of ptrs to ranges
//              cRange          Number of items in above array
//              ppTextXBag      Returned xbag.
//
//-------------------------------------------------------------------------

HRESULT
CTextXBag::Create(CMarkup *             pMarkup,
                  BOOL                  fSupportsHTML,
                  ISegmentList *        pSegmentList,
                  BOOL                  fDragDrop,
                  CTextXBag **          ppTextXBag,
                  CSelDragDropSrcInfo * pSelDragDropSrcInfo /* = NULL */)
{
    HRESULT hr;

    CTextXBag * pTextXBag = new CTextXBag();

    if (!pTextXBag)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pTextXBag->_pSelDragDropSrcInfo = pSelDragDropSrcInfo;

    if (fDragDrop)
    {
        pTextXBag->_pDoc = pMarkup->Doc();
    }

    hr = THR(pTextXBag->SetKeyState());
    if (hr)
        goto Error;

    hr = THR(pTextXBag->FillWithFormats(pMarkup, fSupportsHTML, pSegmentList));
    if (hr)
        goto Error;

Cleanup:
    *ppTextXBag = pTextXBag;

    RRETURN(hr);

Error:
    delete pTextXBag;
    pTextXBag = NULL;
    goto Cleanup;
}

//
//  CTextXBag PRIVATE methods
//

//+---------------------------------------------------------------------------
//
//  Member:     CTextXBag::CTextXBag
//
//  Synopsis:   Private ctor
//
//----------------------------------------------------------------------------

CTextXBag::CTextXBag()
{
    _ulRefs       = 1;
    _cTotal       = CFETC;
    _cFormatMax   = 1;
    _prgFormats   = g_rgFETC;
    _hText        = NULL;
#ifndef WIN16
    _hUnicodeText = NULL;
#endif // !WIN16
    _hRTFText     = NULL;
    _hCFHTMLText  = NULL;
    _pLinkDataObj = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTextXBag::~CTextXBag
//
//  Synopsis:   Private dtor
//
//----------------------------------------------------------------------------

CTextXBag::~CTextXBag()
{
    if( _prgFormats && _prgFormats != g_rgFETC)
    {
        delete [] _prgFormats;
    }

    if (_hText)
        GlobalFree(_hText);

#ifndef WIN16
    if (_hUnicodeText)
        GlobalFree(_hUnicodeText);
#endif // !WIN16

    if (_hRTFText)
        GlobalFree(_hRTFText);

    if (_hCFHTMLText)
        GlobalFree(_hCFHTMLText);

    TLS(pDataClip) = NULL;
}


//+------------------------------------------------------------------------/
//
//  Member:     CTextXBag::SetKeyState
//
//  Synopsis:   Sets the _dwButton member of the CTextXBag
//
//-------------------------------------------------------------------------

HRESULT
CTextXBag::SetKeyState()
{
    static int  vk[] =
    {
        VK_LBUTTON,     // MK_LBUTTON = 0x0001
        VK_RBUTTON,     // MK_RBUTTON = 0x0002
        VK_SHIFT,       // MK_SHIFT   = 0x0004
        VK_CONTROL,     // MK_CONTROL = 0x0008
        VK_MBUTTON,     // MK_MBUTTON = 0x0010
        VK_MENU,        // MK_ALT     = 0x0020
    };

    int     i;
    DWORD   dwKeyState = 0;

    for (i = 0; i < ARRAY_SIZE(vk); i++)
    {
        if (GetKeyState(vk[i]) & 0x8000)
        {
            dwKeyState |= (1 << i);
        }
    }

    _dwButton = dwKeyState & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON);

    return S_OK;
}


//+------------------------------------------------------------------------/
//
//  Member:     CTextXBag::FillWithFormats
//
//  Synopsis:   Fills the text bag with the formats it supports
//
//-------------------------------------------------------------------------

HRESULT
CTextXBag::FillWithFormats(CMarkup *    pMarkup,
                           BOOL         fSupportsHTML,
                           ISegmentList * pSegmentList )
{
    typedef HRESULT (CTextXBag::*FnSet)(CMarkup*, BOOL, ISegmentList *);

    static FnSet aFnSet[] = {
        SetText,
#ifndef WIN16
        SetUnicodeText,
#endif // !WIN16
        SetCFHTMLText,
       //bugwin16: Need to fix the HTML to RTF convertor.
#ifndef NO_RTF
        SetRTFText
#endif // !NO_RTF
    };

    HRESULT hr = S_OK;
    int     n;

    // Allocate our _prgFormats array
    _cFormatMax = ARRAY_SIZE(aFnSet) + 3;
    _cTotal     = 0;
    _prgFormats = new(Mt(CTextXBag_prgFormats)) FORMATETC[_cFormatMax];
    if (NULL == _prgFormats)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    for (n = 0; n < ARRAY_SIZE(aFnSet); ++n)
    {
        // If one format fails to copy, do not abort all formats.
        IGNORE_HR(CALL_METHOD( this, aFnSet[n], (pMarkup, fSupportsHTML, pSegmentList)));
    }
    
Cleanup:
    RRETURN(hr);
}


HRESULT 
CTextXBag::GetDataObjectInfo(IDataObject *   pdo,        // @parm data object to get info on
                             DWORD *         pDOIFlags)  // @parm out parm for info
{
    DWORD       i;
    FORMATETC * pfetc = g_rgFETC;

    for( i = 0; i < DWORD(CFETC); i++, pfetc++ )
    {
        if( pdo->QueryGetData(pfetc) == NOERROR )
            *pDOIFlags |= g_rgDOI[i];
    }
    return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Member:     CTextXBag::GetHTMLText
//
//  Synopsis:   Converts the SegmentList into text stored in an hglobal
//
//----------------------------------------------------------------------------


HRESULT
CTextXBag::GetHTMLText(
    HGLOBAL      *  phGlobal, 
    ISegmentList *  pSegmentList,
    CMarkup      *  pMarkup, 
    DWORD           dwSaveHtmlMode,
    CODEPAGE        codepage, 
    DWORD           dwStrWrBuffFlags )
{
    HGLOBAL     hGlobal  = NULL;   // Global memory handle
    LPSTREAM    pIStream = NULL;   // IStream pointer
    HRESULT     hr;
    int         iSegmentCount;
    CDoc           * pDoc;

    //
    // Do the prep work
    //
    hr = THR(CreateStreamOnHGlobal(NULL, FALSE, &pIStream));
    if (hr)
        goto Error;

    pDoc = pMarkup->Doc();
    Assert( pDoc );


    //
    // Use a scope to clean up the StreamWriteBuff
    //

    {
        CMarkupPointer      mpStart( pDoc ), mpEnd( pDoc );
        CStreamWriteBuff    StreamWriteBuff(pIStream, codepage);
        long                i;

        StreamWriteBuff.SetFlags(dwStrWrBuffFlags);

        hr = THR( pSegmentList->GetSegmentCount( & iSegmentCount, NULL ) );
        if( hr )
            goto Cleanup;
      
        //
        // Save the segments using the range saver
        //
        for ( i = 0; i < iSegmentCount; i++ )
        {
            hr = THR( pSegmentList->MovePointersToSegment(i, &mpStart, &mpEnd) );        

            CRangeSaver rs( &mpStart, &mpEnd, dwSaveHtmlMode, &StreamWriteBuff, mpStart.Markup() );

            hr = THR( rs.Save());
            if (hr)
                goto Error;
        }

        StreamWriteBuff.Terminate();    // appends a null character
    }

    //
    // Wrap it up
    //
    hr = THR(GetHGlobalFromStream(pIStream, &hGlobal));
    if (hr)
        goto Error;

Cleanup:
    ReleaseInterface( pIStream );
    *phGlobal = hGlobal;
    RRETURN(hr);

Error:
    if (hGlobal)
    {
        GlobalFree(hGlobal);
        hGlobal = NULL;
    }
    goto Cleanup;
}

//+------------------------------------------------------------------------
//
//  Member:     CTextXBag::SetTextHelper
//
//  Synopsis:   Gets text in a variety of formats
//
//  Argument:   pTxtSite:           The text site under which to get the text from
//              ppRanges:           Text ranges to save text from
//              cRanges:            Count of ppRanges
//              dwSaveHtmlFlags:    format to save in
//              cp:                 codepage to save in
//              dwStmWrBuffFlags:   stream write buffer flags
//              phGlobalText:       hGlobal to get back
//              iFETCIndex:         _prgFormat index, or -1 to not set it
//
//-------------------------------------------------------------------------

HRESULT
CTextXBag::SetTextHelper(CMarkup *      pMarkup,
                         ISegmentList * pSegmentList,
                         DWORD          dwSaveHtmlFlags,
                         CODEPAGE       cp,
                         DWORD          dwStmWrBuffFlags,
                         HGLOBAL *      phGlobalText,
                         int            iFETCIndex)
{
    HRESULT hr;
    HGLOBAL hText = NULL;

    Assert(_cTotal < _cFormatMax);

    // Make sure not to crash if we are out of space for this format.
    if (_cTotal >= _cFormatMax)
        return S_OK;

    {
        hr = THR(GetHTMLText( 
                    &hText, pSegmentList, pMarkup, 
                    dwSaveHtmlFlags, cp, dwStmWrBuffFlags));
        if (hr)
            goto Error;
    }

    // if the text length is zero, pretend as if the format is
    // unavailable (see bug #52407)
    if (iUnicodeFETC == iFETCIndex || iAnsiFETC == iFETCIndex)
    {
        BOOL    fEmpty;

        Assert(hText);

        LPVOID pText= GlobalLock(hText);
        if( iAnsiFETC == iFETCIndex ) 
        {
            fEmpty= *((char *)pText) == 0;
        }
        else
        {
            // Please don't use strlen on unicode strings.
            fEmpty= *((TCHAR *)pText) == 0;
        }
        GlobalUnlock(hText);
        if (fEmpty)
        {
            GlobalFree(hText);
            hText = NULL;
            goto Cleanup;
        }
    }

    if (iFETCIndex != -1)
    {
        _prgFormats[ _cTotal++ ] = g_rgFETC[iFETCIndex];
    }

Cleanup:

    *phGlobalText = hText;

    RRETURN(hr);

Error:
    if (hText)
    {
        GlobalFree(hText);
        hText = NULL;
    }

    goto Cleanup;
}

//+------------------------------------------------------------------------
//
//  Member:     CTextXBag::SetText
//
//  Synopsis:   Gets ansi plaintext in CP_ACP
//
//-------------------------------------------------------------------------

HRESULT
CTextXBag::SetText(CMarkup *      pMarkup,
                   BOOL           fSupportsHTML,
                   ISegmentList * pSegmentList )
{
    DWORD dwFlags = WBF_SAVE_PLAINTEXT|WBF_NO_WRAP|WBF_FORMATTED_PLAINTEXT;

    if (!fSupportsHTML)
    {
        dwFlags |= WBF_KEEP_BREAKS;
    }

    RRETURN( SetTextHelper( pMarkup, pSegmentList,
                            RSF_SELECTION|RSF_NO_ENTITIZE_UNKNOWN,
                            g_cpDefault, dwFlags, &_hText, iAnsiFETC ) );
}

#ifndef WIN16
//+------------------------------------------------------------------------
//
//  Member:     CTextXBag::SetUnicodeText
//
//  Synopsis:   Gets unicode plaintext
//
//-------------------------------------------------------------------------

HRESULT
CTextXBag::SetUnicodeText(CMarkup *     pMarkup,
                          BOOL          fSupportsHTML,
                          ISegmentList * pSegmentList )
{
    DWORD dwFlags = WBF_SAVE_PLAINTEXT|WBF_NO_WRAP|WBF_FORMATTED_PLAINTEXT;

    if (!fSupportsHTML)
    {
        dwFlags |= WBF_KEEP_BREAKS;
    }

    RRETURN(SetTextHelper(pMarkup, pSegmentList,
                          RSF_SELECTION, CP_UCS_2,
                          dwFlags, &_hUnicodeText, iUnicodeFETC));
}
#endif // !WIN16

//+------------------------------------------------------------------------
//
//  Member:     CTextXBag::SetCFHTMLText
//
//  Synopsis:   Gets HTML with CF_HTML header in UTF-8
//
//-------------------------------------------------------------------------

HRESULT
CTextXBag::SetCFHTMLText(CMarkup *      pMarkup,
                         BOOL           fSupportsHTML,
                         ISegmentList * pSegmentList )
{
    HRESULT hr = S_OK;

    if (fSupportsHTML)
    {
        hr = THR(SetTextHelper(pMarkup, pSegmentList,
            RSF_CFHTML, CP_UTF_8, WBF_NO_NAMED_ENTITIES, &_hCFHTMLText, iHTML));
    }

    RRETURN(hr);
}

#ifndef NO_RTF
//+------------------------------------------------------------------------
//
//  Member:     CTextXBag::SetRTFText
//
//  Synopsis:   Gets RTF from the HTML
//
//-------------------------------------------------------------------------

HRESULT
CTextXBag::SetRTFText(CMarkup *     pMarkup,
                      BOOL          fSupportsHTML,
                      ISegmentList * pSegmentList )
{
#ifdef WINCE
    return S_OK;
#else
    if (!pMarkup->Doc()->RtfConverterEnabled())
        return S_OK;

    HRESULT  hr               = S_OK;
    CRtfToHtmlConverter* pcnv = NULL;
    HGLOBAL                     hHTMLText = NULL, hRTFText = NULL;
    LPSTR                       pszHtml;

    Assert(_cTotal < _cFormatMax);

    // Do not dump out RTF for intrinsics
    if (!fSupportsHTML)
        return S_OK;

    // Make sure not to crash if we are out of space for this format.
    if (_cTotal >= _cFormatMax)
        return S_OK;

    pcnv = new CRtfToHtmlConverter(NULL);
    if (!pcnv)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    //
    //  For the RTF converter, do not use name entities, since our name list
    //  can be more recent.  Save in UTF-8 so that we can at least represent
    //  the unicode for every character.
    //
    hr = THR(SetTextHelper(pMarkup, pSegmentList,
        RSF_FOR_RTF_CONV|RSF_FRAGMENT, CP_UTF_8,
        WBF_NO_NAMED_ENTITIES, &hHTMLText, -1));

    if (hr)
        goto Cleanup;

    pszHtml = (LPSTR)GlobalLock(hHTMLText);
    if (!pszHtml)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    hr = THR(pcnv->StringHtmltoStringRtf(pszHtml, &hRTFText));

    GlobalUnlock(hHTMLText);

    if (hr)
    {
        // RTF conversion failed, do not make this an error since
        //  the RTF converter is a bit unpredictable.
        hr = S_OK;
    }
    else
    {
        // Add RTF to the clipboard formats only if conversion succeeded
        _prgFormats[ _cTotal++ ] = g_rgFETC[iRtfFETC];
    }

Cleanup:
    delete pcnv;

    if (hHTMLText)
        GlobalFree(hHTMLText);

    _hRTFText = hRTFText;

    RRETURN(hr);

Error:
    if (hRTFText)
    {
        GlobalFree(hRTFText);
        hRTFText = NULL;
    }

    goto Cleanup;
#endif // WINCE
}
#endif // ndef NO_RTF

