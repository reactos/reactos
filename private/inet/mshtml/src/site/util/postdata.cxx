#include "headers.hxx"

#ifndef X_POSTDATA_HXX_
#define X_POSTDATA_HXX_
#include "postdata.hxx"
#endif

#ifndef X_ENCODE_HXX_
#define X_ENCODE_HXX_
#include "encode.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef NO_MLANG
extern IMultiLanguage  *g_pMultiLanguage;
extern IMultiLanguage2 *g_pMultiLanguage2;
#endif

MtDefine(CPostData, Dwn, "CPostData")
MtDefine(CPostData_pv, CPostData, "CPostData::_pv")
MtDefine(CPostItem, Dwn, "CPostItem (vector)")
MtDefine(CPostItem_psz, CPostItem, "CPostItem::_pszAnsi/Wide")

// This is the number of characters we allocate by default on the stack
// for string buffers. Only if the number of characters in a string exceeds
// this will we need to allocate memory for string buffers.
#define STACK_ALLOCED_BUF_SIZE 64

// Helpers for Form Submit - copied from IE3 and modified approriately
//
static int x_hex_digit(int c)
{
    if (c >= 0 && c <= 9)
    {
        return c + '0';
    }
    if (c >= 10 && c <= 15)
    {
        return c - 10 + 'A';
    }
    return '0';
}



static const char s_achDisposition[] = "\r\nContent-Disposition: form-data; name=\"";

// MIME types constants
// Multipart/Form-Data are constructed in CreateHeader()
static const char s_achTextPlain[]  = "text/plain\r\n";
static const char s_achUrlEncoded[] = "application/x-www-form-urlencoded\r\n";



/*
   The following array was copied directly from NCSA Mosaic 2.2
 */
static const unsigned char isAcceptable[96] =
/*   0 1 2 3 4 5 6 7 8 9 A B C D E F */
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0,    /* 2x   !"#$%&'()*+,-./  */
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,    /* 3x  0123456789:;<=>?  */
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,    /* 4x  @ABCDEFGHIJKLMNO  */
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1,    /* 5x  PQRSTUVWXYZ[\]^_  */
 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,    /* 6x  `abcdefghijklmno  */
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0};   /* 7x  pqrstuvwxyz{\}~  DEL */

// Performs URL-encoding of null-terminated strings. Pass NULL in pbOut
// to find buffer length required. Note that '\0' is not written out.

int
URLEncode(unsigned char * pbOut, const char * pchIn)
{
    int     lenOut = 0;
    char *  pchOut = (char *)pbOut;

    Assert(pchIn);

    for (; *pchIn; pchIn++, lenOut++)
    {
        if (*pchIn == ' ')
        {
            if (pchOut)
                *pchOut++ = '+';
        }
        else if (*pchIn >= 32 && *pchIn <= 127 && isAcceptable[*pchIn - 32])
        {
            if (pchOut)
                *pchOut++ = (TCHAR)*pchIn;
        }
        else
        {
            if (pchOut)
                *pchOut++ = '%';
            lenOut++;

            if (pchOut)
                *pchOut++ = (char)x_hex_digit((*pchIn >> 4) & 0xf);
            lenOut++;

            if (pchOut)
                *pchOut++ = (char)x_hex_digit(*pchIn & 0xf);
        }
    }
    return lenOut;
}



// Support for internationalization
class CSubmitIntl : public CEncodeWriter
{
public:
    CSubmitIntl(CODEPAGE cp);
    HRESULT Convert(TCHAR * pchIn);
};

CSubmitIntl::CSubmitIntl(CODEPAGE cp)
    // just give a large enough block size
    : CEncodeWriter(NavigatableCodePage(cp), 256)
{
}



HRESULT
CSubmitIntl::Convert(TCHAR * pchIn)
{
    HRESULT hr=S_OK;
    int     cch;

    Assert(pchIn);

    // So MultiByteFromWideChar knows what to convert
    _pchBuffer = (TCHAR *)pchIn;
    _cchBuffer = _cchBufferMax = _tcslen(pchIn) + 1;

    hr = THR(MultiByteFromWideChar(FALSE, &cch));

    // Do this to prevent the destructor from trying to
    // free _pchBuffer
     _pchBuffer = NULL;
     _cchBuffer = _cchBufferMax = 0;

    return hr; 
}


//+---------------------------------------------------------------------------
//
//  Member:     CPostData::IsStringInCodePage
//
//  Synopsis:   Takes a wide char string and an code page and returns whether
//              all the charaters in the string are the code page
//
//----------------------------------------------------------------------------

BOOL 
CPostData::IsStringInCodePage(CODEPAGE cpMlang, LPCWSTR lpWideCharStr, int cchWideChar)
{
    BOOL        fRetVal = FALSE;
    HRESULT     hr = S_OK;
    DWORD       dwMode = 0;

    if (IsStraightToUnicodeCodePage( cpMlang ))
    {
        fRetVal = TRUE;
        goto Cleanup;
    }

    hr =  THR(EnsureMultiLanguage());
    if (hr)
        goto Cleanup;

    Assert (g_pMultiLanguage2);

    hr = THR( g_pMultiLanguage2->ConvertStringFromUnicodeEx( &dwMode, 
                                                             cpMlang,
                                                             (LPWSTR) lpWideCharStr, 
                                                             (UINT *) &cchWideChar,
                                                             NULL,
                                                             NULL,
                                                             MLCONVCHARF_NOBESTFITCHARS,
                                                             NULL) );
    if (FAILED(hr))
        goto Cleanup;

    fRetVal = hr == S_OK; // S_FALSE means some characters were outside the target codepage.

Cleanup: 
    return fRetVal;
}


CODEPAGE
CPostData::GetCP(CDoc * pDoc)
{
    if (_cp)
        return _cp;

    CSubmitIntl intl(_fUseUtf8 ? CP_UTF8 : pDoc->GetCodePage());
    _cp = intl.GetCodePage();

    return _cp;

}

///////////////////////////////////////////////////////////////////////////////
//  CPostData

HRESULT
CPostData::AppendEscaped(const TCHAR * pszWide, CDoc * pDoc)
{
    HRESULT     hr = S_OK;
    CODEPAGE    cp;
    long        cch = pszWide ? _tcslen(pszWide) : 0;

    IMLangCodePages * pMLangCodePages = NULL;

    if (!pszWide)
        return S_OK;

    // Internationalize to produce multi-byte stream
    Assert(pDoc);
    if (_fUseCustomCodePage)
    {
        cp = _cpInit;
    }
    else
    {
        cp = (_fUseUtf8 ? CP_UTF8 : pDoc->GetCodePage());
    }        
    CSubmitIntl intl(cp);

    // CSubmitIntl sometimes maps the doc's codepage to something else.
    // We must cache that cp for use in CFormElement::DoSubmit.
    _cp = intl.GetCodePage();

    if (!_fUseUtf8 && !IsStringInCodePage(_cp,
                        pszWide,
                        cch + 1) && !_fUseCustomCodePage)
    {
        DWORD       dwCodePages;
        long        cchCodePages;

        _fCharsetNotDefault = TRUE;

        hr = THR( EnsureMultiLanguage() );
        if (hr)
            goto Cleanup;

        Assert( g_pMultiLanguage );

        hr = THR( g_pMultiLanguage->QueryInterface( IID_IMLangCodePages, (void**)&pMLangCodePages ) );
        if (hr)
            goto Cleanup;

        hr = THR( pMLangCodePages->GetStrCodePages(pszWide, 
            cch,
            NULL,
            &dwCodePages,
            &cchCodePages));
        if (hr)
            goto Cleanup;

        if (cch > cchCodePages)
            _fCodePageError = TRUE;

        _dwCodePages |= dwCodePages;
    }

    hr = THR(intl.Convert((TCHAR*)pszWide));
    if ( hr )
        RRETURN(hr);

    {
        // Now, do the URL encoding
        const char * pszAnsi = (char *)intl._pbBuffer;
        UINT cbOld = Size();

        switch ( _encType )
        {
            case htmlEncodingURL:
                {    
                    UINT cbLen = URLEncode(NULL, pszAnsi);
                    if (0 == cbLen)
                        goto Cleanup;

                    hr = THR(Grow(cbOld + cbLen));
                    if ( hr )
                        goto Cleanup;

                    URLEncode(Base()+cbOld, pszAnsi);
                }
                break;

            default:
                {
                    //  BUGBUG(laszlog): Snap in the RFC1522 encoder here
                    //                   when we have one

                    hr = THR(Append(pszAnsi));
                }
                break;

        } // switch
    }

Cleanup:
    ReleaseInterface( pMLangCodePages );
    RRETURN(hr);
}

HRESULT
CPostData::Append(const char * pszAnsi)
{
    if (!pszAnsi || !pszAnsi[0])
        return S_OK;

    UINT cbOld = Size();
    UINT cbLen = strlen(pszAnsi);

    if (Grow(cbOld+cbLen) != S_OK)
        return E_OUTOFMEMORY;

    // don't copy '\0'
    memcpy((char *)Base() + cbOld, pszAnsi, cbLen);

    _fItemSeparatorIsLast = FALSE;
    return S_OK;
}

HRESULT
CPostData::Append(int num)
{
    char sz[256];

    wsprintfA(sz, "%d", num);
    return Append(sz);
}

HRESULT
CPostData::Terminate(BOOL fOverwriteLastChar)
{
    if (!fOverwriteLastChar || Size() == 0)
    {
        if (Grow(Size()+1) != S_OK)
            return E_OUTOFMEMORY;
    }
    (*this)[Size()-1] = 0;

    _fItemSeparatorIsLast = FALSE;
    return S_OK;
}



//+--------------------------------------------------------------------------
//
//  Method:     CPostData::AppendUnicode
//
//  Synopsis:   Appends a filename in Unicode
//
//  Arguments:  pszWideFilename     points to the null-terminated filename
//
//  Note:       As it is used for a single Unicode filename
//              this Append routine copies the whole string
//              including the terminating wide NULL
//
//---------------------------------------------------------------------------
HRESULT
CPostData::AppendUnicode(const TCHAR * pszWideFilename)
{
    UINT cbOld;
    UINT cbLen;

    Assert(_eCurrentKind == POSTDATA_FILENAME);
    Assert(pszWideFilename);

    cbOld = Size();
    cbLen = sizeof(TCHAR) * (1 + _tcslen(pszWideFilename));

    if ( S_OK != Grow(cbOld + cbLen) )
        return E_OUTOFMEMORY;

    memcpy((char*)Base()+cbOld, pszWideFilename, cbLen);

    _fItemSeparatorIsLast = FALSE;

    return S_OK;
}



//+--------------------------------------------------------------------------
//
//  Method:     CPostData::CreateHeader
//
//  Synopsis:   Creates the 'Content-type:' header value. It also has the
//              side-effect of creating the multipart boundary string that
//              we use in multipart form submissions.
//
//---------------------------------------------------------------------------
HRESULT
CPostData::CreateHeader(void)
{
    switch ( _encType )
    {
        case htmlEncodingMultipart:
        {
#ifdef WIN16
            //BUGWIN16: our systemtime is actually time_t. need to
            // write a function which does the below.
            Assert(0);
#else
            SYSTEMTIME time;

            GetLocalTime( &time );
            wsprintfA(_achBoundary,
                      "\r\n-----------------------------%x%x%x%x",  // CRLF removed from here
                      time.wYear,
                      time.wMilliseconds,
                      time.wSecond,
                      GetForegroundWindow());
    
            //  _achBoundary is the complete separator string written into the submit stream
            //  complete with the CRLF and double-hyphen "--" prefix
            //  the separator value proper is the string following that prefix;
            //  that's the string that goes into the multipart header.

            wsprintfA(_achEncoding,
                "multipart/form-data; boundary=%s\r\n",
                _achBoundary + 4 );

            Assert(1 + strlen(_achEncoding) <= sizeof(_achEncoding));
#endif
            break;
        }

        case htmlEncodingURL:
            Assert(sizeof(s_achUrlEncoded) <= sizeof(_achEncoding));
            memcpy(_achEncoding, s_achUrlEncoded, sizeof(s_achUrlEncoded));
            break;

        case htmlEncodingText:
            Assert(sizeof(s_achTextPlain) <= sizeof(_achEncoding));
            memcpy(_achEncoding, s_achTextPlain, sizeof(s_achUrlEncoded));
            break;
    }

    return S_OK;
}



//+--------------------------------------------------------------------------
//
//  Method:     CPostData::AppendItemSeparator
//
//  Synopsis:   Writes the item separator
//
//---------------------------------------------------------------------------
HRESULT
CPostData::AppendItemSeparator()
{
    HRESULT hr = S_OK;

    Assert(_eCurrentKind == POSTDATA_LITERAL);

    switch ( _encType )
    {
        case htmlEncodingMultipart:
            {
                if ( ! _fItemSeparatorIsLast )
                {
                    _lLastItemSeparator = Size();

                    //  Don't prepend the CR/LF if this is the first item
                    if ( _lLastItemSeparator || _cItems )
                    {
                        hr = THR(Append(_achBoundary));
                        if ( hr )
                            goto Cleanup;
                    }
                    else
                    {
                        hr = THR(Append(_achBoundary+2));
                        if ( hr )
                            goto Cleanup;
                    }
                    hr = THR(Append(s_achDisposition));
                    if ( hr )
                        goto Cleanup;

                    _fItemSeparatorIsLast = TRUE;
                }
            }
            break;

        case htmlEncodingText:
            {
                if (( Size() > 0 )
                   && ( ! _fItemSeparatorIsLast ))
                {
                    _lLastItemSeparator = Size();

                    hr = THR(Append("\r\n"));
                    if ( hr )
                        goto Cleanup;

                    _fItemSeparatorIsLast = TRUE;
                }
            }
            break;

        case htmlEncodingURL:
            {
                if ( Size() > 0 && *(Base()+(Size() - 1)) != '&' )
                {
                    _lLastItemSeparator = Size();

                    hr = THR(Append("&"));
                    if ( hr )
                        goto Cleanup;

                    _fItemSeparatorIsLast = TRUE;
                }
            }
            break;

    } // switch

    _cbOld = Size();


Cleanup:
    RRETURN(hr);
}



//+--------------------------------------------------------------------------
//
//  Method:     CPostData::RemoveLastItemSeparator
//
//  Synopsis:   Removes the separator appended after the last submit item
//
//---------------------------------------------------------------------------
HRESULT
CPostData::RemoveLastItemSeparator(void)
{
    Assert(_eCurrentKind == POSTDATA_LITERAL);

    if ( _fItemSeparatorIsLast )
    {
        Assert(_lLastItemSeparator <= Size());

        SetSize(_lLastItemSeparator);
        _fItemSeparatorIsLast = FALSE;
    }

    return S_OK;
}



//+--------------------------------------------------------------------------
//
//  Method:     CPostData::AppendValueSeparator
//
//  Synopsis:   Writes the separator between the name and the value
//              of the control
//
//  note:       This is an equal sign for URLEncoding and text/plain,
//              closing quote and a line break for multipart
//
//---------------------------------------------------------------------------
HRESULT
CPostData::AppendValueSeparator()
{
    HRESULT hr=S_OK;

    switch ( _encType )
    {
        case htmlEncodingMultipart:
            {
                hr = THR(Append("\"\r\n\r\n"));
            }
            break;

        case htmlEncodingText:
        case htmlEncodingURL:
            {
                hr = THR(Append("="));
            }
            break;

    } // switch

    _fItemSeparatorIsLast = FALSE;

    RRETURN(hr);
}




//+--------------------------------------------------------------------------
//
//  Method:     CPostData::AppendNameValuePair
//
//  Synopsis:   Appends a name-value submit pair to the submit stream
//              according to the current encoding
//
//  Arguments:  pchName     the name. If NULL->no submit.
//              pchValue    the value string
//              pDoc        the Doc. Used for Unicode->ANSI mapping
//
//---------------------------------------------------------------------------
HRESULT
CPostData::AppendNameValuePair(LPCTSTR pchName, LPCTSTR pchValue, CDoc * pDoc)
{
    HRESULT hr;

    Assert(pDoc);

    if ( ! pchName )
        return S_FALSE;

    hr = THR(AppendEscaped(pchName, pDoc));
    if (hr)
        goto Cleanup;

    hr = THR(AppendValueSeparator());
    if (hr)
        goto Cleanup;

    hr = THR(AppendEscaped(pchValue, pDoc));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN1(hr, S_FALSE);
}



//+--------------------------------------------------------------------------
//
//  Method:     CPostData::AppendNameFilePair
//
//  Synopsis:   Appends a name-file submit pair to the submit stream
//              according to the current encoding
//              Called by the File Upload control
//
//  Arguments:  pchName     the name. If NULL->no submit.
//              pchFileName the filename string
//              pDoc        the Doc. Used for Unicode->ANSI mapping
//
//---------------------------------------------------------------------------
HRESULT
CPostData::AppendNameFilePair(LPCTSTR pchName, LPCTSTR pchFileName, CDoc * pDoc)
{
    TCHAR * pszMimeType = NULL;
    HRESULT hr=S_OK;

    if ( ! pchName )
        return S_FALSE;

    Assert(pDoc);

    switch ( _encType )
    {
        case htmlEncodingMultipart:
            {
                hr = THR(AppendEscaped(pchName, pDoc));
                if (hr)
                    goto Cleanup;

                hr = THR(Append("\x22; filename=\""));
                if (hr)
                    goto Cleanup;

                if ( ! pchFileName )
                {
                    pchFileName = g_Zero.ach;
                }
#ifndef UNIX
                hr = THR(AppendEscaped(pchFileName, pDoc));
#else
                {
                    TCHAR *pchFileNameLastPart = _tcsrchr(pchFileName,_T('/'));
                    if(pchFileNameLastPart)
                        hr = THR(AppendEscaped(pchFileNameLastPart+1, pDoc));
                    else
                        hr = THR(AppendEscaped(pchFileName, pDoc));
                }
#endif // !UNIX
                if (hr)
                    goto Cleanup;

                hr = THR(Append("\"\r\nContent-Type: "));
                if (hr)
                    goto Cleanup;

                //  Look up the file type in the MIME database
                //  or use the extension

        #if JOHANN_FIXED_URLMON_TO_SNIFF_FILENAME

                TCHAR achFileMoniker[FORMS_BUFLEN];

                _tcscpy(achFileMoniker, _T("file://"));
                _tcscat(achFileMoniker, pchFileName);

                hr = THR(FindMimeFromData(NULL,             // bind context - can be NULL                                     
                                          pchFileName,   // url - can be null                                              
                                          NULL,             // buffer with data to sniff - can be null (pwzUrl must be valid) 
                                          0,                // size of buffer                                                 
                                          NULL,             // proposed mime if - can be null                                 
                                          0,                // will be defined                                                
                                          &pszMimeType,     // the suggested mime                                             
                                          0));   
        #else
                //  Use the achFileMoniker for data buffering, it's large enough
                HANDLE hFile = INVALID_HANDLE_VALUE;
                ULONG cb;
                char achFileBuf[FORMS_BUFLEN];

                hFile = CreateFile(pchFileName,
                                   GENERIC_READ,
                                   FILE_SHARE_READ,
                                   NULL,                //  security descriptor
                                   OPEN_EXISTING,
                                   0,
                                   NULL);

                if ( hFile == INVALID_HANDLE_VALUE )
                {
                    hr = E_FAIL;
                    goto CloseFile;
                }

#ifndef WIN16
                if (FILE_TYPE_DISK != GetFileType(hFile))
                {
                    hr = E_FAIL;
                    goto CloseFile;
                }
#endif

                //  Here the file should be open and ripe for consumption
                cb = 0;
                if ( !ReadFile(hFile, achFileBuf, sizeof(achFileBuf), &cb, NULL) )
                {
                    hr = E_FAIL;
                    goto CloseFile;
                }


                hr = THR(FindMimeFromData(NULL,             // bind context - can be NULL                                     
                                          pchFileName,      // url - can be null                                              
                                          achFileBuf,       // buffer with data to sniff - can be null (pwzUrl must be valid) 
                                          cb,               // size of buffer                                                 
                                          NULL,             // proposed mime if - can be null                                 
                                          0,                // will be defined                                                
                                          &pszMimeType,     // the suggested mime                                             
                                          0));

        CloseFile:
                if ( hFile != INVALID_HANDLE_VALUE )
                {
                    Verify(CloseHandle(hFile));
                }
        #endif
                // must be 0
                // Change the EXE sniff from application/x-msdownload because it's better in this case
                
                if ( SUCCEEDED(hr) && StrCmpIC(pszMimeType, _T("application/x-msdownload")) != 0 )
                {
                    hr = THR(AppendEscaped(pszMimeType, pDoc));
                    if (hr)
                        goto Cleanup;

                    hr = THR(Append("\r\n"));
                    if (hr)
                        goto Cleanup;
                }
                else
                {
                    hr = THR(Append("application/octet-stream\r\n"));
                    if (hr)
                        goto Cleanup;
                }



                //  Depending on the MIME type decide on the encoding
                //  If encoding is needed we'll do mime64

                //  Note: Neither Netscape 3 nor apparently IE3.02 do
                //        any encoding when sending binary stuff.
                //        They just send it straight up.

                //hr = pSubmitData->Append("Content-Transfer-Encoding: base64\r\n");
                //if (hr)
                //    goto Cleanup;

                // designates end of header
                hr = THR(Append("\r\n"));
                if (hr)
                    goto Cleanup;

                hr = THR(StartNewItem(POSTDATA_FILENAME));
                if (hr)
                    goto Cleanup;

                hr = THR(AppendUnicode(pchFileName));
                if (hr)
                    goto Cleanup;

                hr = THR(StartNewItem(POSTDATA_LITERAL));
                if (hr)
                    goto Cleanup;

            }
            break;

        case htmlEncodingText:
        case htmlEncodingURL:
            {
                hr = THR(AppendNameValuePair(pchName, pchFileName, pDoc));
            }
            break;

    } // switch

Cleanup:
    CoTaskMemFree(pszMimeType);
    RRETURN1(hr, S_FALSE);

}

//+--------------------------------------------------------------------------
//
//  Method:     CPostData::AppendFooter
//
//  Synopsis:   Writes the footer for multipart
//
//---------------------------------------------------------------------------
HRESULT
CPostData::AppendFooter()
{
    HRESULT hr;

    switch ( _encType )
    {
        case htmlEncodingMultipart:
            {   
                hr = THR(Append(_achBoundary));
                if ( hr )
                    goto Cleanup;

                hr = THR(Append("--\r\n"));

                _fItemSeparatorIsLast = FALSE;
            }
            break;

        case htmlEncodingText:
            {
                hr = AppendItemSeparator();
            }
            break;

        default:
            {
                hr = S_OK;
            }
            break;

    } // switch

Cleanup:
    RRETURN(hr);
}



//+--------------------------------------------------------------------------
//
//  Method:     CPostData::Finish
//
//  Synopsis:   Wraps up the construction of the submit data
//
//---------------------------------------------------------------------------
HRESULT
CPostData::Finish(void)
{
    HRESULT hr;

    // Eat up any trailing '&'; no need to add the NULL terminator
    hr = THR(RemoveLastItemSeparator());
    if ( hr )
        goto Cleanup;

    hr = THR(AppendFooter());
    if ( hr )
        goto Cleanup;

    //  Flush the last string into its own memory block

    hr = THR(StartNewItem(POSTDATA_UNKNOWN));
    if ( hr )
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}


//+--------------------------------------------------------------
//
//  Member:     CPostData::StartNewItem
//
//  Synopsis:   Flushes the collected data into a string
//              The new Item type will be remembered
//              as new data is being collected.
//
//---------------------------------------------------------------
HRESULT
CPostData::StartNewItem(POSTDATA_KIND ekindNewItem)
{
    HRESULT hr = S_OK;
    CPostItem * pItem;

    //  Create the new chunk if there is any data
    //  Grow the descriptor array as needed

    pItem = new(Mt(CPostItem)) CPostItem[_cItems + 1];
    if ( ! pItem )
        goto MemoryError;

    memcpy(pItem, _pItems, _cItems * sizeof(*pItem) );

    delete [] _pItems;

    _pItems = pItem;

    pItem = _pItems + _cItems;

    _cItems++;

    //  Dump the current data into the Item, set the flag

    pItem->_ePostDataType = _eCurrentKind;

    if ( _eCurrentKind == POSTDATA_FILENAME )
    {
        TCHAR * pszWide;
        UINT ctchAlloc = 1 + _tcslen((TCHAR *)Base());

        pszWide = new(Mt(CPostItem_psz)) TCHAR [ ctchAlloc ];
        if ( ! pszWide )
            goto MemoryError;

        memcpy(pszWide, Base(), ctchAlloc * sizeof(TCHAR));

        pItem->_pszWide = pszWide;
    }
    else if ( _eCurrentKind == POSTDATA_LITERAL )
    {
        char * pszAnsi;

        pszAnsi = new(Mt(CPostItem_psz)) char[1 + Size()];
        if ( ! pszAnsi )
            goto MemoryError;

        memcpy(pszAnsi, Base(), Size());
        pszAnsi[Size()] = '\0';

        pItem->_pszAnsi = pszAnsi;
    }

    //  Reset the data area

    _eCurrentKind = ekindNewItem;
    SetSize(0);
    _cbOld = 0;
    _lLastItemSeparator = 0;
    _fItemSeparatorIsLast = FALSE;

Cleanup:
    RRETURN(hr);

MemoryError:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}
