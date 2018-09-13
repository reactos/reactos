//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       headfoot.hxx
//
//  Contents:   Definition of the CHeaderFooterInfo
//
//  Classes:    CHeaderFooterInfo, CDescr
//
//  Purpose:    Print a header and/or footer on every page of the document
//
//----------------------------------------------------------------------------

#ifndef I_HEADFOOT_HXX_
#define I_HEADFOOT_HXX_
#pragma INCMSG("--- Beg 'headfoot.hxx'")

MtExtern(CHeaderFooterInfo) 

class CDescr;
typedef CPtrAry<CDescr*>        AryPNumbers;

class CHeaderFooterInfo
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CHeaderFooterInfo))
    CHeaderFooterInfo(CPrintDoc* pDoc);
    ~CHeaderFooterInfo();
    HRESULT SetHeaderFooter(TCHAR* pHeader);
    inline void UpdatePageNumber(int iPageNumber);
    inline void SetTotalPages(int cPages);
    void Draw(CDrawInfo* pDI, const CCharFormat* pCF, BOOL bHeader,HFONT hFont);
    void SetHeaderFooterURL(TCHAR* pURL);
private:
    void SetTitle(CStr* pTitle);
//    void SetTitle(LPCTSTR pTitle);
    void ParseIt(void);
    void ChangePageNumber(int iPageNumber,TCHAR* pHF,AryPNumbers* pAry);
    void DeleteArray(void);
    void ConvertNum(int iNum,TCHAR** ppChars);
    void ConvertPageNum(void);
    void CalcXPos(CDrawInfo* pDI);
    void DrawIt(CDrawInfo * pDI, const CCharFormat * pCF, int y, BOOL fHeader);
    void AddTextDescr(TCHAR* pText);
    void AddDate(DWORD dFlag);
    void AddShortDate(void);
    void AddLongDate(void);
    void AddTime(DWORD dFlag);
    void AddShortTime(void);
    void AddLongTime(void);
    void AddTotalPages(void);
    void AddTitle(void);
    void AddURL(void);
    void SetTextMember(TCHAR** ppMember,TCHAR* pNewText);


    TCHAR*          _pHeaderFooter;
    AryPNumbers     _aryParts;
    int             _iLastPageNum;
    TCHAR*          _pTitle;
    int             _iTotalPages;
    int             _iNrOfMultipleBlanks;
    CPrintDoc*      _pPrintDoc;
    BOOL            _bParsed;
    TCHAR           _PageNumChars[8];
    TCHAR*          _pURL;
};

//---------------------------------------------------------------------------
//
//  Member:     CHeaderFooterInfo::UpdatePageNumber
//
//  Synopsis:   Updates the stored _iLastPageNumber to the new value and
//              starts the process which generates the _pParsed string.
//
//  Arguments:  iPageNumber     The new page number
//              
//---------------------------------------------------------------------------


inline void
CHeaderFooterInfo::UpdatePageNumber(int iPageNumber)
{
    _iLastPageNum = iPageNumber;
};

//---------------------------------------------------------------------------
//
//  Member:     CHeaderFooterInfo::SetTotalPages
//
//  Synopsis:   Stores the total page number in the _aryParts collection
//
//  Arguments:  cPages      total number of pages
//              
//---------------------------------------------------------------------------


inline void
CHeaderFooterInfo::SetTotalPages(int cPages)
{
    _iTotalPages = cPages;
};

#pragma INCMSG("--- End 'headfoot.hxx'")
#else
#pragma INCMSG("*** Dup 'headfoot.hxx'")
#endif
