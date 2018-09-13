//+----------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       postdata.hxx
//
//  Contents:   Manipulates post/submit data.
//
//-----------------------------------------------------------------------------

#ifndef I_POSTDATA_HXX_
#define I_POSTDATA_HXX_
#pragma INCMSG("--- Beg 'postdata.hxx'")

#define POSTDATA_DEFAULT_BUFFER_SIZE    1024
#define BOUNDARY_SIZE 64
#define ENCODING_SIZE 64 + BOUNDARY_SIZE

MtExtern(CPostItem)
MtExtern(CPostItem_psz)
MtExtern(CPostData)
MtExtern(CPostData_pv)

class CDoc;

enum POSTDATA_KIND
{
    POSTDATA_UNKNOWN = 0,
    POSTDATA_LITERAL,
    POSTDATA_FILENAME
};

class CPostItem
{
public:
    POSTDATA_KIND _ePostDataType;
    union
    {
        char * _pszAnsi;
        WCHAR * _pszWide;
    };
};

//+----------------------------------------------------------------------------
//
//  Class:      CPostData
//
//+----------------------------------------------------------------------------

class CPostData : public CStackDataAry<BYTE, POSTDATA_DEFAULT_BUFFER_SIZE>
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CPostData))
    CPostData() : CStackDataAry<BYTE, POSTDATA_DEFAULT_BUFFER_SIZE>(Mt(CPostData_pv))
    {
        _cItems = 0;
        _pItems = 0;
        _lLastItemSeparator = 0;
        _cbOld = 0;
        _cp = 0;
        _cpInit = 0;
        _dwCodePages = 0;
        
        _fAddAmpersand = FALSE;
        _fItemSeparatorIsLast = FALSE;
        _fCharsetNotDefault = FALSE;
        _fUseUtf8 = FALSE;
        _fCodePageError = FALSE;
        _fUseCustomCodePage = FALSE;

        *_achBoundary = 0;
        *_achEncoding = 0;
        _eCurrentKind = POSTDATA_LITERAL;
    };

    HRESULT AppendEscaped(const TCHAR * pszWide, CDoc * pDoc);
    HRESULT Append(const char * pszAnsi);
    HRESULT Append(int num);
    HRESULT Terminate(BOOL fOverwriteLastChar);

    HRESULT AppendNameValuePair(LPCTSTR pchName, LPCTSTR pchValue, CDoc * pDoc);
    HRESULT AppendNameFilePair(LPCTSTR pchName, LPCTSTR pchFileName, CDoc * pDoc);

    HRESULT Finish(void);

    HRESULT CreateHeader();
    HRESULT AppendItemSeparator();
    HRESULT AppendValueSeparator();

    HRESULT StartNewItem(POSTDATA_KIND ekindNewItem);

    char * GetEncodingString(void) { return _achEncoding; };

    BOOL IsStringInCodePage(CODEPAGE cp, LPCWSTR lpWideCharStr, int cchWideChar);

    BYTE * Base() { return (BYTE*)PData(); }

    CODEPAGE GetCP(CDoc * pDoc);

    long            _encType;
    long            _cbOld;
    long            _lLastItemSeparator;
    DWORD           _dwCodePages;

    POSTDATA_KIND   _eCurrentKind;
    UINT            _cItems;
    CPostItem *     _pItems;
    CODEPAGE        _cp;
    CODEPAGE        _cpInit;                // Codepage used to init CSubmitIntl

    unsigned        _fAddAmpersand : 1;
    unsigned        _fItemSeparatorIsLast : 1;
    unsigned        _fCharsetNotDefault : 1;
    unsigned        _fUseUtf8: 1;
    unsigned        _fUseCustomCodePage: 1;
    unsigned        _fCodePageError: 1;

    char            _achBoundary[BOUNDARY_SIZE];
    char            _achEncoding[ENCODING_SIZE];

protected:
    HRESULT AppendUnicode(const TCHAR * pszWideFilename);
    HRESULT RemoveLastItemSeparator(void);
    HRESULT AppendFooter();

};

#pragma INCMSG("--- End 'postdata.hxx'")
#else
#pragma INCMSG("*** Dup 'postdata.hxx'")
#endif
