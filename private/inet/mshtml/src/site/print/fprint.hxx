//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       fprint.hxx
//
//  Contents:   Definition of the CPrintDoc class plus other related stuff
//
//  Classes:    CPrintDoc
//
//----------------------------------------------------------------------------

#ifndef I_FPRINT_HXX_
#define I_FPRINT_HXX_
#pragma INCMSG("--- Beg 'fprint.hxx'")

#ifndef X_DOCPRINT_HXX_
#define X_DOCPRINT_HXX_
#include "docprint.hxx"
#endif

#ifndef X_PUTIL_HXX_
#define X_PUTIL_HXX_
#include "putil.hxx"
#endif

// forward reference
class CHeaderFooterInfo;
class CSpooler;

MtExtern(CPrintDoc)

BOOL RegQueryPrintBackgroundColorOrBitmap();

class CPrintDoc : public CDoc
{
    friend class CHeaderFooterInfo;
    friend class CSpooler;
    typedef CDoc super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CPrintDoc))

    CPrintDoc();
    ~CPrintDoc();

    CDataAry<CPrintPage> *  _paryPrintPage;
    DVTARGETDEVICE *        _ptd;
    HDC                     _hdc;
    HDC                     _hic;
    RECT                    _rcClip;
    CStr                    _cstrPrettyUrl;

    virtual BOOL IsPrintDoc() { return TRUE; }
    virtual void OnLoadStatus(LOADSTATUS LoadStatus);
    virtual LPTSTR GetDocumentSecurityUrl() { return _cstrPrettyUrl; };
    void         AfterLoadComplete(void);
    HRESULT      InitForPrint();
    HRESULT      Print();

    void    SetPrintHeaderFooterURL(TCHAR* pURL);
    HRESULT ReadHeaderAndFooter(void);

    CHeaderFooterInfo * Header() const { Assert(_pHInfo); return _pHInfo; }
    CHeaderFooterInfo * Footer() const { Assert(_pFInfo); return _pFInfo; }
    BOOL                ShortcutTable();
    HRESULT             DoInit(PRINTINFO * pPICurrent, CSpooler *pSpoolerToNotify);
    HRESULT             DoLayout(int *pcPages, LONG * plViewWidth, LONG *pyLastPageHeight = NULL, LONG yInitialPageHeight = 0, BOOL fFillLastPage = TRUE);
    HRESULT             DoDraw(RECT *prc, POINT * pointOffset = NULL);

    unsigned int    _fCancelled:1;
    unsigned int    _fDontRunScripts:1;
    unsigned int    _fTempFile:1;
    unsigned int    _fLaidOut:1;
    unsigned int    _fRootDoc:1;
    unsigned int    _fHasPageBreaks:1;
    unsigned int    _fXML:1;
    PRINTINFOBAG    _PrintInfoBag;

private:

    CHeaderFooterInfo *     _pHInfo;
    CHeaderFooterInfo *     _pFInfo;
    CSpooler *              _pSpoolerToNotify;

    HRESULT ReadOldStyleHeaderOrFooter(HKEY hfKey,const TCHAR* pValuePrefix,CHeaderFooterInfo* pHF,TCHAR** ppHeaderFooter);
    HRESULT ReadNewStyleHeaderOrFooter(HKEY hfKey,const TCHAR* pValueName,CHeaderFooterInfo* pHF);
    inline HRESULT ReadOldStyleHeaderAndFooter(HKEY keyPS,TCHAR** ppHeader,TCHAR** ppFooter);
    inline HRESULT ReadNewStyleHeaderAndFooter(HKEY keyPS);
    inline void CreateNewHeaderFooter(HKEY keyPS,TCHAR* pHeader,TCHAR* pFooter);
    HRESULT PrintOnePage(int cPage, RECT rc);
    void    PrintNonCollated(RECT rcClip, int cCopies, int cPages);
    void    PrintCollated(RECT rcClip, int cCopies, int cPages);
    HRESULT PrintHeaderFooter(int cPage);
    void    GetPrintPageRange(int cPages, int *pnStartPage, int *pnEndPage);
};

#pragma INCMSG("--- End 'fprint.hxx'")
#else
#pragma INCMSG("*** Dup 'fprint.hxx'")
#endif
