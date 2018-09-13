/*
 *  _DXFROBJ.H
 *
 *  Purpose:
 *      Class declaration for an OLE data transfer object (for use in
 *      drag drop and clipboard operations)
 *
 *  Author:
 *      alexgo (4/25/95)
 */

#ifndef I__DXFEROBJ_H_
#define I__DXFEROBJ_H_
#pragma INCMSG("--- Beg '_dxfrobj.h'")

class CFlowLayout;

#ifndef X_XBAG_HXX_
#define X_XBAG_HXX_
#include "xbag.hxx"
#endif

/*
 *  CTextXBag
 *
 *  Purpose:
 *      holds a "snapshot" of some text data that can be used
 *      for drag drop or clipboard operations
 *
 *  Notes:
 *      TODO (alexgo): add in support for TOM<-->TOM optimized data
 *      transfers
 */

MtExtern(CTextXBag)

class CSelDragDropSrcInfo;

typedef enum tagDataObjectInfo
{
    DOI_NONE            = 0,
    DOI_CANUSETOM       = 1,    // TOM<-->TOM optimized data transfers
    DOI_CANPASTEPLAIN   = 2,    // plain text pasting available
    DOI_CANPASTERICH    = 4,    // rich text pasting available  (TODO: alexgo)
    DOI_CANPASTEOLE     = 8     // object may be pasted as an OLE embedding
                                // (note that this flag may be combined with
                                // others). (TODO: alexgo)
    //TODO (alexgo): more possibilites:  CANPASTELINK, CANPASTESTATICOLE
} DataObjectInfo;


class CTextXBag : public CBaseBag
{
    typedef CBaseBag super;

public:

    STDMETHODIMP QueryInterface(REFIID iid, LPVOID * ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    STDMETHOD(EnumFormatEtc)( DWORD dwDirection,
            IEnumFORMATETC **ppenumFormatEtc);
    STDMETHOD(GetData)( FORMATETC *pformatetcIn, STGMEDIUM *pmedium );
    STDMETHOD(QueryGetData)( FORMATETC *pformatetc );
    STDMETHOD(SetData) (LPFORMATETC pformatetc, STGMEDIUM FAR * pmedium, BOOL fRelease);

    static HRESULT  Create(CMarkup *             pMarkup,
                           BOOL                  fSupportsHTML,
                           ISegmentList *        pSegmentList, 
                           BOOL                  fDragDrop,
                           CTextXBag **          ppTextXBag,
                           CSelDragDropSrcInfo * pSelDragDropSrcInfo = NULL);

    static HRESULT  GetDataObjectInfo(IDataObject *   pdo,      
                                      DWORD *         pDOIFlags);

    //
    // Others
    //

private:
    // NOTE: private cons/destructor, may not be allocated on the stack as
    // this would break OLE's current object liveness rules
    CTextXBag();
    virtual ~CTextXBag();

    HRESULT     SetKeyState();
    HRESULT     FillWithFormats(CMarkup *       pMarkup,
                                BOOL            fSupportsHTML,
                                ISegmentList *  pSegmentList );

    HRESULT     SetTextHelper(CMarkup *     pMarkup,
                              ISegmentList *pSegmentList,
                              DWORD         dwSaveHtmlFlags,
                              CODEPAGE      cp,
                              DWORD         dwStmWrBuffFlags,
                              HGLOBAL *     phGlobalText,
                              int           iFETCIndex);

    HRESULT     SetText         (CMarkup *      pMarkup,
                                 BOOL           fSupportsHTML,
                                 ISegmentList * pSegmentList );

#ifndef WIN16
    HRESULT     SetUnicodeText  (CMarkup *      pMarkup,
                                 BOOL           fSupportsHTML,
                                 ISegmentList * pSegmentList );

#endif // !WIN16
    HRESULT     SetCFHTMLText   (CMarkup *      pMarkup,
                                 BOOL           fSupportsHTML,
                                 ISegmentList * pSegmentList );

    HRESULT     SetRTFText      (CMarkup *      pMarkup,
                                 BOOL           fSupportsHTML,
                                 ISegmentList * pSegmentList );

    HRESULT     GetHTMLText    (HGLOBAL      *  phGlobal, 
                                ISegmentList *  pSegmentList,
                                CMarkup      *  pMarkup, 
                                DWORD           dwSaveHtmlMode,
                                CODEPAGE        codepage, 
                                DWORD           dwStrWrBuffFlags);

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTextXBag))

    long        _cFormatMax;    // maximum formats the array can store
    long        _cTotal;        // total number of formats in the array
    FORMATETC * _prgFormats;    // the array of supported formats
    CSelDragDropSrcInfo * _pSelDragDropSrcInfo;

public:
    HGLOBAL     _hText;             // handle to the ansi plain text
#ifndef WIN16
    HGLOBAL     _hUnicodeText;      // handle to the plain UNICODE text
#endif // !WIN16
    HGLOBAL     _hRTFText;          // handle to the RTF text
    HGLOBAL     _hCFHTMLText;       // handle to the CFHTML (in utf-8)
};


//
//  Some globally useful FORMATETCs

extern FORMATETC g_rgFETC[];
extern DWORD     g_rgDOI[];
extern int CFETC;

enum FETCINDEX                          // Keep in sync with g_rgFETC[]
{
    iHTML,                              // HTML (in ANSI)
    iRtfFETC,                           // RTF
#ifndef WIN16
    iUnicodeFETC,                       // Unicode plain text
#endif // !WIN16
    iAnsiFETC,                          // ANSI plain text
//    iFilename,                          // Filename
    iRtfAsTextFETC,                     // Pastes RTF as text
    iFileDescA,                         // FileGroupDescriptor
    iFileDescW,                         // FileGroupDescriptorW
    iFileContents,                      // FileContents
    iUniformResourceLocator             // UniformResourceLocator
//    iEmbObj,                            // Embedded Object
//    iEmbSrc,                            // Embed Source
//    iObtDesc,                           // Object Descriptor
//    iLnkSrc,                            // Link Source
//    iMfPict,                            // Metafile
//    iDIB,                               // DIB
//    iBitmap,                            // Bitmap
//    iRtfNoObjs,                         // RTF with no objects
//    iTxtObj,                            // Richedit Text
//    iRichEdit                           // RichEdit Text w/formatting
};

#define cf_HTML                     g_rgFETC[iHTML].cfFormat
//#define cf_RICHEDIT               g_rgFETC[iRichEdit].cfFormat
//#define cf_EMBEDDEDOBJECT         g_rgFETC[iEmbObj].cfFormat
//#define cf_EMBEDSOURCE            g_rgFETC[iEmbSrc].cfFormat
//#define cf_OBJECTDESCRIPTOR       g_rgFETC[iObtDesc].cfFormat
//#define cf_LINKSOURCE             g_rgFETC[iLnkSrc].cfFormat
#define cf_RTF                      g_rgFETC[iRtfFETC].cfFormat
//#define cf_RTFNOOBJS              g_rgFETC[iRtfNoObjs].cfFormat
//#define cf_TEXTOBJECT             g_rgFETC[iTxtObj].cfFormat
#define cf_RTFASTEXT                g_rgFETC[iRtfAsTextFETC].cfFormat
//#define cf_FILENAME               g_rgFETC[iFilename].cfFormat
#define cf_FILEDESCA                g_rgFETC[iFileDescA].cfFormat
#define cf_FILEDESCW                g_rgFETC[iFileDescW].cfFormat
#define cf_FILECONTENTS             g_rgFETC[iFileContents].cfFormat
#define cf_UNIFORMRESOURCELOCATOR   g_rgFETC[iUniformResourceLocator].cfFormat

#pragma INCMSG("--- End '_dxfrobj.h'")
#else
#pragma INCMSG("*** Dup '_dxfrobj.h'")
#endif
