//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       headfoot.cxx
//
//  Contents:   CHeaderFooterInfo
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_HEADFOOT_HXX_
#define X_HEADFOOT_HXX_
#include "headfoot.hxx"
#endif

#ifndef X_FPRINT_HXX_
#define X_FPRINT_HXX_
#include "fprint.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X__FONTLNK_H_
#define X__FONTLNK_H_
#include "_fontlnk.h"
#endif

MtDefine(CHeaderFooterInfo, Printing, "CHeaderFooterInfo")
MtDefine(CHeaderFooterInfo_aryParts_pv, CHeaderFooterInfo, "CHeaderFooterInfo::_aryParts::_pv")
MtDefine(CHeaderFooterInfo_pURL, CHeaderFooterInfo, "CHeaderFooterInfo::_pURL")
MtDefine(CHeaderFooterInfo_pHeaderFooter, CHeaderFooterInfo, "CHeaderFooterInfo::_pHeaderFooter")
MtDefine(CHeaderFooterInfo_pTitle, CHeaderFooterInfo, "CHeaderFooterInfo::_pTitle")

MtDefine(CHeaderFooterInfoConvertNum_ppChars, Printing, "CHeaderFooterInfo::ConvertNum *ppChars")

MtDefine(CDescr, Printing, "CDescr")
MtDefine(CDescr_pPart, Printing, "CDescr::_pPart")

#define NUMNOTSET -1

enum PartKindType { pkText,pkPageNum,pkMultipleBlank };

//---------------------------------------------------------------------------
//
//  Class:      CDescr
//
//  Synopsis:   Describes one element in the Header or Footer
//          One element can be a text string or a Page number or MultipleBlank
//              if it is not a textstring then pPart is NULL.
//
//
//---------------------------------------------------------------------------


class CDescr
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CDescr))
    CDescr(void);
    ~CDescr();
    TCHAR*          pPart;
    PartKindType    PartKind;
    int             xPos;
    int             iPixelLen;
};

//---------------------------------------------------------------------------
//
//  Member:     CDescr::CDescr
//
//  Synopsis:   Constructor of CDescr
//
//  Arguments:  None
//
//---------------------------------------------------------------------------


CDescr::CDescr(void)
{
    pPart = NULL;
};

//---------------------------------------------------------------------------
//
//  Member:     CDescr::~CDescr
//
//  Synopsis:   Destructor of CDescr
//              Deletes allocated string containing part of Header or Footer
//
//  Arguments:  None
//
//---------------------------------------------------------------------------

CDescr::~CDescr()
{
    // because we do not store the whole text, only the pointer if it is
    // a pagenumber, therefore we should not delete it

    if (PartKind != pkPageNum)
        delete [] pPart;
};

//---------------------------------------------------------------------------
//
//  Member:     CHeaderFooterInfo::CHeaderFooterInfo
//
//  Synopsis:   Constructor of CHeaderFooterInfo
//              Initializes the pointers of the original (user typed) header of footer,
//              the title and the URL address.
//              Moves NUMNOTSET value into number members.
//
//  Arguments:  None
//
//---------------------------------------------------------------------------

CHeaderFooterInfo::CHeaderFooterInfo(CPrintDoc* pPrintDoc)
    : _aryParts(Mt(CHeaderFooterInfo_aryParts_pv))
{
    Assert(pPrintDoc);

    _pHeaderFooter = NULL;
    _iLastPageNum = NUMNOTSET;
    _iTotalPages = NUMNOTSET;
    _pTitle = NULL;
    _iNrOfMultipleBlanks = 0;
    _bParsed = FALSE;
    _pURL = NULL;
    _tcscpy(_PageNumChars,_T(""));
    if (pPrintDoc->_pPrimaryMarkup->GetTitleElement())
    {
        SetTitle(&(pPrintDoc->_pPrimaryMarkup->GetTitleElement()->_cstrTitle));
    }

    _pPrintDoc = pPrintDoc;
};

//---------------------------------------------------------------------------
//
//  Member:     CHeaderFooterInfo::~CHeaderFooterInfo
//
//  Synopsis:   Destructor of CHeaderFooterInfo
//              Deletes the original (user typed) header of footer,
//              the parsed and assembled string, the title and the URL address.
//              Deletes the array containing parts of the header or footer.
//
//  Arguments:  None
//
//---------------------------------------------------------------------------

CHeaderFooterInfo::~CHeaderFooterInfo()
{
    delete [] _pHeaderFooter;
    delete [] _pTitle;
    delete [] _pURL;
    DeleteArray();
};

//---------------------------------------------------------------------------
//
//  Member:     CHeaderFooterInfo::DeleteArray
//
//  Synopsis:   Deletes the array containing parts of the header or footer
//
//  Arguments:  None
//
//---------------------------------------------------------------------------

void
CHeaderFooterInfo::DeleteArray(void)
{
    CDescr*     pDescr;
    for (int i=0;i<_aryParts.Size();i++)
    {
        pDescr = _aryParts[i];
        delete pDescr;
    };
    _aryParts.DeleteAll();
};

//---------------------------------------------------------------------------
//
//  Member:     CHeaderFooterInfo::SetHeaderFooterURL
//
//  Synopsis:   Set the URL address of the Document
//
//  Arguments:  pURL        address of URL string
//
//---------------------------------------------------------------------------

void
CHeaderFooterInfo::SetHeaderFooterURL(TCHAR* pURL)
{
    delete _pURL;
    _pURL = NULL;

    if (!pURL)
        return;
    int iLen = _tcslen(pURL);
    if (iLen < 1)
        return;
    _pURL = new(Mt(CHeaderFooterInfo_pURL)) TCHAR[iLen+1];
    Assert(_pURL);
    if (!_pURL)
        return;
    _tcscpy(_pURL,pURL);
};

//---------------------------------------------------------------------------
//
//  Member:     CHeaderFooterInfo::ConvertNum
//
//  Synopsis:   Converts a number to a string. Allocates string.If number is not set
//              gives back an empty string.
//
//  Arguments:  iNum        integer number to be converted into string
//              ppChars     string the number will be converted into (max 8 chars)
//
//---------------------------------------------------------------------------

void
CHeaderFooterInfo::ConvertNum(int iNum,TCHAR** ppChars)
{
    *ppChars = new(Mt(CHeaderFooterInfoConvertNum_ppChars)) TCHAR[8];
    if (*ppChars)
    {
        if (iNum == NUMNOTSET)
        {
            _tcscpy(*ppChars,_T(""));
        }
        else
        {
            _itot(iNum,*ppChars,10);
        };
    };
};

//---------------------------------------------------------------------------
//
//  Member:     CHeaderFooterInfo::ConvertPageNum
//
//  Synopsis:   Convert the last page number by calling ConvertNum
//
//  Arguments:  ppNumChars              string the page number will be converted into (max 8 chars)
//
//---------------------------------------------------------------------------

inline void
CHeaderFooterInfo::ConvertPageNum(void)
{
    _itot(_iLastPageNum,(TCHAR*)&_PageNumChars,10);
};


//---------------------------------------------------------------------------
//
//  Member:     CHeaderFooterInfo::AddTextDescr
//
//  Synopsis:   Allocates a new CDescr object, allocates memory inside the object to store
//              the new Text and sets the PartKind member to pkText
//              Append the new CDescr object to _aryParts collection.
//
//  Arguments:  pText       text to be stored in a CDescr object
//
//---------------------------------------------------------------------------


void
CHeaderFooterInfo::AddTextDescr(TCHAR* pText)
{
    if (!pText) return;
    if (_tcslen(pText) < 1) return;
    CDescr*     pDescr;

    pDescr = new CDescr;
    Assert(pDescr);

    if (!pDescr) return;
    pDescr->pPart = new(Mt(CDescr_pPart)) TCHAR[_tcslen(pText)+1];
    Assert(pDescr->pPart);

    if (!pDescr->pPart)
    {
        delete pDescr;
        return;
    };
    _tcscpy(pDescr->pPart,pText);
    pDescr->PartKind = pkText;
    _aryParts.Append(pDescr);
};

//---------------------------------------------------------------------------
//
//  Member:     CHeaderFooterInfo::AddTotalPages
//
//  Synopsis:   Converts the total pages into a string and add the new string into
//              _aryParts collection.
//
//  Arguments:  None
//
//---------------------------------------------------------------------------


void
CHeaderFooterInfo::AddTotalPages(void)
{
    TCHAR*  pTotalPagesChars;

    ConvertNum(_iTotalPages,&pTotalPagesChars);
    AddTextDescr(pTotalPagesChars);
    delete pTotalPagesChars;
};

//---------------------------------------------------------------------------
//
//  Member:     CHeaderFooterInfo::AddTime
//
//  Synopsis:   Converts the current time into a string and adds
//              it to the _aryParts collection.
//
//  Arguments:  dFlag       0 or TIME_FORCE24HOURFORMAT
//
//---------------------------------------------------------------------------
void
CHeaderFooterInfo::AddTime(DWORD dFlag)
{
    TCHAR          TimeStr[DATE_STR_LENGTH];
    SYSTEMTIME currentSysTime;

    GetLocalTime(&currentSysTime);
#ifndef WIN16
    if (GetTimeFormat(LOCALE_USER_DEFAULT,dFlag,&currentSysTime,NULL,
                        (TCHAR*)&TimeStr,DATE_STR_LENGTH))
    {
        AddTextDescr((TCHAR*)&TimeStr);
    };
#else
    // BUGWIN16 mblain--we currently always use the US C format for time!

    int cchResult;
    struct tm *LocalTm;
    LocalTm = localtime(&currentSysTime);

    if (TIME_FORCE24HOURFORMAT == dFlag)
    {
        cchResult = wsprintf(TimeStr, "%2d:%2d:%2d",
            LocalTm->tm_hour,
            LocalTm->tm_min,
            LocalTm->tm_sec );
    }
    else
    {
        cchResult = wsprintf(TimeStr, "%2d:%2d:%2d %s",
            LocalTm->tm_hour % 12,
            LocalTm->tm_min,
            LocalTm->tm_sec,
            LocalTm->tm_hour < 12 ? "AM" : "PM" );
    }

    Assert(cchResult <= DATE_STR_LENGTH);

    AddTextDescr((TCHAR*)&TimeStr);

#endif // ndef WIN16 else

};

//---------------------------------------------------------------------------
//
//  Member:     CHeaderFooterInfo::AddShortTime
//
//  Synopsis:   Converts the current time (short format) into a string and adds
//              it to the _aryParts collection.
//
//  Arguments:  None
//
//---------------------------------------------------------------------------

inline void
CHeaderFooterInfo::AddShortTime(void)
{
     AddTime(0);    // zero means no 24 hour format
};

//---------------------------------------------------------------------------
//
//  Member:     CHeaderFooterInfo::AddLongTime
//
//  Synopsis:   Converts the current time (long format) into a string and adds
//              it to the _aryParts collection.
//
//  Arguments:  None
//
//---------------------------------------------------------------------------


inline void
CHeaderFooterInfo::AddLongTime(void)
{
    AddTime(TIME_FORCE24HOURFORMAT);
};

//---------------------------------------------------------------------------
//
//  Member:     CHeaderFooterInfo::AddShortDate
//
//  Synopsis:   Converts the current date into a string and adds
//              it to the _aryParts collection.
//
//
//  Arguments:  dFlag   DATE_SHORTDATE or DATE_LONGDATE
//
//---------------------------------------------------------------------------
void
CHeaderFooterInfo::AddDate(DWORD dFlag)
{
    TCHAR      DateStr[DATE_STR_LENGTH];
    SYSTEMTIME currentSysTime;

    GetLocalTime(&currentSysTime);
#ifndef WIN16
    if (GetDateFormat(LOCALE_USER_DEFAULT,dFlag,&currentSysTime,NULL,
                      (TCHAR*)&DateStr,DATE_STR_LENGTH))
    {
        AddTextDescr((TCHAR*)&DateStr);
    };
#else
    // BUGWIN16 mblain--we currently always use the US C format for date!
    // we also always use 'short' format -- 18feb97

    int cchResult;
    struct tm *LocalTm;
    LocalTm = localtime(&currentSysTime);

    //if (DATE_SHORTDATE == dFlag)
    {
        cchResult = wsprintf(DateStr, "%2d/%2d/%2d",
            LocalTm->tm_mon +1,
            LocalTm->tm_mday,
            LocalTm->tm_year % 100 );
    }
    // else // do long format.

    Assert(cchResult <= DATE_STR_LENGTH);

    AddTextDescr((TCHAR*)&DateStr);

#endif // ndef WIN16 else

};

//---------------------------------------------------------------------------
//
//  Member:     CHeaderFooterInfo::AddShortDate
//
//  Synopsis:   Converts the current date (short format) into a string and adds
//              it to the _aryParts collection.
//
//  Arguments:  None
//
//---------------------------------------------------------------------------


inline void
CHeaderFooterInfo::AddShortDate(void)
{
    AddDate(DATE_SHORTDATE);
};

//---------------------------------------------------------------------------
//
//  Member:     CHeaderFooterInfo::AddLongDate
//
//  Synopsis:   Converts the current date (long format) into a string and adds
//              it to the _aryParts collection.
//
//  Arguments:  None
//
//---------------------------------------------------------------------------


inline void
CHeaderFooterInfo::AddLongDate(void)
{
    AddDate(DATE_LONGDATE);
};

//---------------------------------------------------------------------------
//
//  Member:     CHeaderFooterInfo::AddTitle
//
//  Synopsis:   Add the title from CDoc to the _aryParts collection.
//
//  Arguments:  None
//
//---------------------------------------------------------------------------

inline void
CHeaderFooterInfo::AddTitle(void)
{
    AddTextDescr(_pTitle);
};

//---------------------------------------------------------------------------
//
//  Member:     CHeaderFooterInfo::AddURL
//
//  Synopsis:   Add the URL address from CDoc to the _aryParts collection.
//
//  Arguments:  None
//
//---------------------------------------------------------------------------

inline void
CHeaderFooterInfo::AddURL(void)
{
    AddTextDescr(_pURL);
};

//---------------------------------------------------------------------------
//
//  Member:     CHeaderFooterInfo::ParseIt
//
//  Synopsis:   Parses the user typed _pHeaderFooter string and builds up
//              _aryParts collection.
//
//  Special characters :
//              &p      page number
//              &P      total pages
//              &b      multiple blank
//              &t      time, short format
//              &T      time, 24 hour format
//              &d      date, short format
//              &D      date, long format
//              &w      title
//              &u      URL address
//
//  Arguments:  None
//
//---------------------------------------------------------------------------


void
CHeaderFooterInfo::ParseIt(void)
{
    Assert(_pHeaderFooter);

    if (!_pHeaderFooter)
        return;
    if (_bParsed)
        return;

    DeleteArray();

    int iLen = _tcslen(_pHeaderFooter);
    if (iLen > 0)
    {
        TCHAR*  pO = _pHeaderFooter;
        TCHAR*  pP = NULL;
        TCHAR   SaveChar;

        _iNrOfMultipleBlanks = 0;
        while (*pO)
        {
            pP = _tcschr(pO,_T('&'));
            if (!pP)
            {
                AddTextDescr(pO);
                return;
            };
            *pP = '\0';
            AddTextDescr(pO);
            *pP = _T('&');
	    pP++;
#ifdef UNIX
            pO = pP - 1;
            if (_tcslen(pP) < 1) 
            {
                AddTextDescr(pO);
                return;
            }
#endif
	    pO = pP;
            pO++;
	    switch(*pP)
            {
                case _T('b')    :
                case _T('p')    :
                    CDescr* pDescr;
                    pDescr = new CDescr;
                    Assert(pDescr);
                    if (pDescr)
                    {
                        pDescr->pPart = NULL;
                        if (*pP == _T('p'))
                        {
                            pDescr->PartKind = pkPageNum;
                        }
                        else
                        {
                            pDescr->PartKind = pkMultipleBlank;
                            _iNrOfMultipleBlanks++;
                        };
                        _aryParts.Append(pDescr);
                    };
                    break;
                case _T('P')    :
                    AddTotalPages();
                    break;
                case _T('d')    :
                    AddShortDate();
                    break;
                case _T('D')    :
                    AddLongDate();
                    break;
                case _T('t')    :
                    AddShortTime();
                    break;
                case _T('T')    :
                    AddLongTime();
                    break;
                case _T('w')    :
                    AddTitle();
                    break;
                case _T('u')    :
                    AddURL();
                    break;
                default         :
                    SaveChar = *pO;
                    *pO = '\0';
                    AddTextDescr(pP);
                    *pO = SaveChar;
                    break;
            };
        };
    };
    _bParsed = TRUE;
};

//---------------------------------------------------------------------------
//
//  Member:     CHeaderFooterInfo::SetHeaderFooter
//
//  Synopsis:   Stores the used typed Header or Footer and starts the parsing process
//
//  Arguments:  pHeaderFooter           user typed string
//
//---------------------------------------------------------------------------


HRESULT
CHeaderFooterInfo::SetHeaderFooter(TCHAR* pHeaderFooter)
{
    delete [] _pHeaderFooter;
    _pHeaderFooter = NULL;
    if (!pHeaderFooter)
    {
                _pHeaderFooter = new(Mt(CHeaderFooterInfo_pHeaderFooter)) TCHAR[1];
                if (_pHeaderFooter)
                        _tcscpy(_pHeaderFooter,_T(""));
    }
    else
    {
                int iLen = _tcslen(pHeaderFooter);
                _pHeaderFooter = new(Mt(CHeaderFooterInfo_pHeaderFooter)) TCHAR[iLen+1];
                Assert(_pHeaderFooter);
                if (_pHeaderFooter)
                _tcscpy(_pHeaderFooter,pHeaderFooter);
    };
    return S_OK;
};


//---------------------------------------------------------------------------
//
//  Member:     CHeaderFooterInfo::SetTitle
//
//  Synopsis:   Stores the document title into the _aryParts collection
//
//  Arguments:  pTitle      document title
//
//---------------------------------------------------------------------------


void
CHeaderFooterInfo::SetTitle(CStr* pTitle)
{
    LPTSTR  pStr;
    if (!pTitle)
        return;

    pStr = LPTSTR(*pTitle);
    delete [] _pTitle;
    _pTitle = NULL;
    int iLen = pTitle->Length();
    if (iLen > 0)
    {
        _pTitle = new(Mt(CHeaderFooterInfo_pTitle)) TCHAR[iLen + 1];
        Assert(_pTitle);
        if (_pTitle)
            _tcscpy(_pTitle,pStr);
    };
};

//---------------------------------------------------------------------------
//
//  Member:     CHeaderFooterInfo::CalcXPos
//
//  Synopsis:   Goes through the parts collection (_aryParts) and calculates the
//              x position of them
//
//  Arguments:  pDI     CDrawInfo
//              hFont   font handle
//
//---------------------------------------------------------------------------

void
CHeaderFooterInfo::CalcXPos(CDrawInfo* pDI)
{
    int     iPixelLen = 0;              // length of header or footer in pixels
    int     iPixelFree;                 // remaining pixels after calculating iPixelLen
    int     iPixelMultipleBlank = 0;    // pixels for 1 multiple blank
    SIZE    size;
    int     xPos;

    ConvertPageNum();

    int         iPageNumLen = _tcslen((TCHAR*)&_PageNumChars); // length of page number in chars
    int         iPixelPageNum = 0;  // length of page number in pixels
    int         i;
    CDescr*     pDescr;

    GetTextExtentPoint32(pDI->_hdc,(TCHAR*)&_PageNumChars,iPageNumLen,&size);
    iPixelPageNum = size.cx;    // we do it only once

    // calculate the length of parts in pixels

    for (i=0;i<_aryParts.Size();i++)
    {
        pDescr = _aryParts[i];
        if (pDescr->PartKind == pkText)
        {
            GetTextExtentPoint32(pDI->_hdc,pDescr->pPart,_tcslen(pDescr->pPart),&size);
            pDescr->iPixelLen = size.cx;
            iPixelLen += size.cx;
        }
        else
        {
            if (pDescr->PartKind == pkPageNum)
            {
                pDescr->pPart = (TCHAR*)&_PageNumChars;
                pDescr->iPixelLen = iPixelPageNum;
                iPixelLen += iPixelPageNum;
            };
        }
    }


    int iTabStopPos;
    int iMultipleBlankCounter = 0;

    // calculate complete width of page
    iPixelFree = _pPrintDoc->_rcClip.right - _pPrintDoc->_rcClip.left;
    xPos = iTabStopPos = _pPrintDoc->_rcClip.left;

    if (_iNrOfMultipleBlanks && (iPixelFree > 0))
    {
        iPixelMultipleBlank = iPixelFree / _iNrOfMultipleBlanks;
    }

    // calculate the x position of every part

    for (i=0;i<_aryParts.Size();i++)
    {
        pDescr = _aryParts[i];
        pDescr->xPos = xPos;
        if (pDescr->PartKind == pkMultipleBlank)
        {
            xPos += iPixelMultipleBlank;
            iTabStopPos += iPixelMultipleBlank;
            iMultipleBlankCounter++;
        }
        else
        {
            xPos += pDescr->iPixelLen;
            if (i > 0)
            {
                CDescr* pPrevDescr = _aryParts[i-1];
                if (pPrevDescr->PartKind == pkMultipleBlank)
                {
                    // previous element was a multipleblank
                    // now try to center align the text part
                    BOOL bTextCanBeCentered = (iTabStopPos + (pDescr->iPixelLen/2)) <= iPixelFree;
                    BOOL bRemainingPartsFit = (iTabStopPos - (pDescr->iPixelLen/2) + iPixelLen) <= iPixelFree;
                    if (bTextCanBeCentered && bRemainingPartsFit)
                    {
                        pDescr->xPos = iTabStopPos - (pDescr->iPixelLen / 2);
                        xPos = pDescr->xPos + pDescr->iPixelLen;
                    }
                    else
                    {
                        // try to right align
                        if ((iTabStopPos + iPixelLen) <= iPixelFree)
                        {
                            pDescr->xPos = iTabStopPos;
                            xPos = iTabStopPos + pDescr->iPixelLen;
                        }
                        else
                        {
                            // we have to left align
                            xPos = iTabStopPos - iPixelLen + pDescr->iPixelLen;
                            pDescr->xPos = iTabStopPos - iPixelLen; // we have to make room for the other
                                                                        // parts as well
                        }
                    }
                }
            }
            iPixelLen -= pDescr->iPixelLen; // now it contains the length of the remaining parts
        }
    }
}

//---------------------------------------------------------------------------
//
//  Member:     CHeaderFooterInfo::DrawIt
//
//  Synopsis:   Goes through the parts collection (_aryParts) and draws the text parts
//
//  Arguments:  hdc     device context handle
//              y       the vertical position to draw at
//
//---------------------------------------------------------------------------

void
CHeaderFooterInfo::DrawIt(
    CDrawInfo * pDI,                          
    const CCharFormat * pCF,
    int y,
    BOOL fHeader)
{
    CDescr*     pDescr;
    CDescr*     pNextDescr;
    SIZE        spaceSize;
    int         nParts = _aryParts.Size();
    int         j;
    int         nLen;

    GetTextExtentPoint32(pDI->_hdc,_T(" "),1,&spaceSize);

    for (int i=0;i<nParts;i++)
    {
        pDescr = _aryParts[i];
        if (pDescr->pPart)
        {
            SIZE    textsize;

            pNextDescr = NULL;
            int yPos;


            GetTextExtentPoint(pDI->_hdc,pDescr->pPart,_tcslen(pDescr->pPart),&textsize);
            if ((i < _aryParts.Size()-1) && (pDescr->PartKind != pkPageNum))
            {
                pNextDescr = NULL;
                for (j=i+1;j<nParts;j++)
                {
                    if (_aryParts[j]->pPart)
                    {
                        pNextDescr = _aryParts[j];
                        break;
                    }
                }
                if (pNextDescr)
                {
                    // Instead of doing >= I do > because if the user puts a space at the end
                    // of the text we do not want to truncate it.
                    // We do not know the user intentions wheather he/she wants to touch the next
                    // part or not.
                    if ((pDescr->xPos + textsize.cx ) > pNextDescr->xPos)
                    {
                        if (_tcslen(pDescr->pPart) < MAX_PATH)
                        {
                            // PathCompactPath assumes that the text at least MAX_PATH long
                            // otherwise it overwrite the memory behind it. It is a bug in that code
                            // BUGBUG if PathCompactPath will be fixed we do not need this whole if
                            // statement.
                            TCHAR*  pNew = new(Mt(CDescr_pPart)) TCHAR[MAX_PATH+1];
                            if (pNew)
                            {
                                _tcscpy(pNew,pDescr->pPart);
                                delete [] pDescr->pPart;
                                pDescr->pPart = pNew;
                                PathCompactPath(pDI->_hdc,pDescr->pPart,pNextDescr->xPos-pDescr->xPos-spaceSize.cx);
                            }
                        }
                        else
                        {
                            PathCompactPath(pDI->_hdc,pDescr->pPart,pNextDescr->xPos-pDescr->xPos-spaceSize.cx);
                        }
                        if ((pDescr->xPos + textsize.cx ) > pNextDescr->xPos)
                        {
                        // because if the last part (after the rightmost slash) is too long
                        // PathCompactPath does not truncate it right, therefore we have to
                        // truncate it ourselves.

                            nLen = _tcslen(pDescr->pPart);
                            pDescr->pPart[nLen-1] = 0;
                            while (nLen > 0)
                            {
                                GetTextExtentPoint(pDI->_hdc,pDescr->pPart,nLen-1,&textsize);
                                if ((pDescr->xPos + textsize.cx ) > pNextDescr->xPos)
                                {
                                    nLen--;
                                    pDescr->pPart[nLen] = 0;
                                }
                                else
                                {
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            // If we are drawing the footer, move the top of the text into the printable range.
            // To be sure that everything is visible, move up the footer by 120% of the text height:
            // MulDivQuick(spaceSize.cy, 6, 5).  One of many PCL drivers that have a problem with this
            // mainly on Win95 is HP LaserJet 4Si MX.


            yPos = y - (fHeader
                        ? 0
                        : MulDivQuick(spaceSize.cy, 6, 5)); // add 20% to the text height

            if (pCF)
            {
                FontLinkTextOut(pDI->_hdc,
                                pDescr->xPos,
                                yPos,
                                0,
                                NULL,
                                pDescr->pPart,
                                _tcslen(pDescr->pPart),
                                pDI,
                                pCF,
                                FLTO_TEXTOUTONLY);
            }
            else
            {
                TextOut(pDI->_hdc,
                        pDescr->xPos,
                        yPos,
                        pDescr->pPart,
                        _tcslen(pDescr->pPart));
            }
        }
    }
}

//---------------------------------------------------------------------------
//
//  Member:     CHeaderFooterInfo::Draw
//
//  Synopsis:   Draws the stored _pParsed string into the top or the bottom margin
//
//  Arguments:  pDI     pointer to DrawInfo
//              bHeader True if it is a header, false if it is a footer
//              hFont   font handle
//---------------------------------------------------------------------------


void
CHeaderFooterInfo::Draw(
    CDrawInfo* pDI,
    const CCharFormat * pCF,
    BOOL bHeader,
    HFONT hFont)
{
    if (!_pHeaderFooter)
        return;
    if (_tcslen(_pHeaderFooter) < 1)
        return;

    HRGN    hrgn = CreateRectRgn(0, 0, 0, 0);

    if (hrgn == NULL)
        return;

    if (SaveDC(pDI->_hdc))
    {
        int     y = 0;
        HFONT   hFontOld = NULL;
        COLORREF oldColor = SetTextColor(pDI->_hdc,RGB(0,0,0));               // black
        COLORREF oldBkColor = SetBkColor(pDI->_hdc,RGB(255,255,255));         // white

        if (GetClipRgn(pDI->_hdc,hrgn) != -1)
        {
            SelectClipRgn(pDI->_hdc, NULL);
            if (hFont)
                hFontOld = (HFONT)SelectObject(pDI->_hdc,hFont);

            if (!_bParsed)
                ParseIt();
            CalcXPos(pDI);
            if (bHeader)
            {
                IntersectClipRect(pDI->_hdc,
                                pDI->_ptDst.x,
                                0,
                                pDI->_ptDst.x + pDI->_sizeDst.cx,
                                pDI->_ptDst.y);
            }
            else
            {
                IntersectClipRect(pDI->_hdc,
                                pDI->_ptDst.x,
                                pDI->_ptDst.y + pDI->_sizeDst.cy,
                                pDI->_ptDst.x + pDI->_sizeDst.cx,
#ifdef WIN16
                        //BUGWIN16: we don't have the GetDeviceCaps for PHYSICALHEIGHT, so turning this
                        // back to the old code
                        pDI->_ptDst.y + pDI->_sizeDst.cy + _pPrintDoc->_PrintInfoBag.rtMargin.bottom);

                y = pDI->_ptDst.y+pDI->_sizeDst.cy;
#else
                                GetDeviceCaps(pDI->_hic, PHYSICALHEIGHT));

                // set the footer y to the end of the print range.  if not in clip-rect, we clip.
                y = MulDivQuick(100*GetDeviceCaps(pDI->_hic, VERTSIZE), GetDeviceCaps(pDI->_hic, LOGPIXELSY), 2540);
#endif
	    };

#ifdef UNIX
        //currently mainwin doesn't support initialization of hdcDesktop
        // try to find a temprary patch
        {
          if (!bHeader){
            SIZE spaceSize;
            GetTextExtentPoint32(pDI->_hdc, _T(" "),1,&spaceSize);
            if (spaceSize.cy == 0)
              y = pDI->_ptDst.y + pDI->_sizeDst.cy;
          }
        }
#endif  //unix
            DrawIt(pDI, pCF, y, bHeader);
            SelectClipRgn(pDI->_hdc,hrgn);
            DeleteObject(hrgn);
            if (hFontOld)
                SelectObject(pDI->_hdc,hFontOld);
        };
        if (oldColor != CLR_INVALID)
            SetTextColor(pDI->_hdc,oldColor);
        if (oldBkColor != CLR_INVALID)
            SetBkColor(pDI->_hdc,oldBkColor);
        RestoreDC(pDI->_hdc,-1);
    }
};

