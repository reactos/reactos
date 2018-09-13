//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       strbuf.cxx
//
//  Contents:   CStreamWriteBuff
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_UNICWRAP_HXX_
#define X_UNICWRAP_HXX_
#include "unicwrap.hxx"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_XMLNS_HXX_
#define X_XMLNS_HXX_
#include "xmlns.hxx"
#endif

MtDefine(CStreamReadBuff,  Utilities, "CStreamReadBuff")
MtDefine(CStreamWriteBuff, Utilities, "CStreamWriteBuff")

extern const TCHAR* LookUpErTable(TCHAR ch, BOOL fCp1252);

BOOL ChrLower(TCHAR ch) { return _T('a') <= ch && ch <= _T('z'); } ;

//+---------------------------------------------------------------------------
//
//  Member:     CStreamWriteBuff::CStreamWriteBuff
//
//  Synopsis:   constructor
//
//----------------------------------------------------------------------------

CStreamWriteBuff::CStreamWriteBuff(IStream *pStm, CODEPAGE cp) :
                   CEncodeWriter( cp, WBUFF_SIZE )
{
    Assert(pStm);

    _pStm               = pStm;
    _pStm->AddRef( );

    _ichLastNewLine     = 0;        // index of the last newline char in the
                                    // wide char buffer
    _iLastValidBreak    = 0;        // index of the last valid break char in the
                                    // wide char buffer
    _cPreFormat         = 0;        // indicates the level of preformatted mode
    _cSuppress          = 0;        // level of suppression
    _cchIndent          = 0;        // no. of indentation chars at the begining
                                    // of the line
    _nCurListItemIndex  = 1;        // Keeps track of the list item value during
                                    // plain text save
    _dwFlags            = WBF_ENTITYREF;
                                    // Turn on entity reference conversion mode.

    _pElementContext    = NULL;     // Element which we are saving (null if no
                                    // specific element)

    _fNeedIndent        = TRUE;     // Keep track of indent state

    // Allocate a wide char buffer (this will allocate _nBlockSize wchars)
    IGNORE_HR( PrepareToEncode( ) );
    Assert( _pchBuffer );

    // Allocate a MB buffer (this may grow if necessary)
    IGNORE_HR( MakeRoomForChars( _nBlockSize * 2 ) );
    Assert( _pbBuffer );

    _pXmlNamespaceTable = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStreamWriteBuff::~CStreamWriteBuff
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------

CStreamWriteBuff::~CStreamWriteBuff()
{
//    AssertSz(_cchIndent == 0, "Improper indentation");
    AssertSz(_cPreFormat == 0, "Pre-formated block(s) not terminated");
    AssertSz(_cSuppress == 0, "Suppress mode not terminated");

    Flush();
    _pStm->Release( );

    delete _pXmlNamespaceTable;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStreamWriteBuff::Terminate
//
//  Synopsis:   Appends a null character at the end of the stream
//
//----------------------------------------------------------------------------
HRESULT CStreamWriteBuff::Terminate()
 {
    HRESULT hr = S_OK;

    hr = FlushWBuffer(FALSE, FALSE);   // flush from wchar to multibyte
    if( hr )
        goto Cleanup;

    Assert( _pbBuffer != NULL );

    hr = WriteDirectToMultibyte('\0', 1);

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CStreamWriteBuff::FlushMBBuffer()
//
//  Synopsis:   Flush the ansi buffer on to an IStream object.
//
//  Returns:    Returns S_OK, if successful else the error
//
//----------------------------------------------------------------------------

HRESULT
CStreamWriteBuff::FlushMBBuffer()
{
    HRESULT hr = S_OK;

    if (_pbBuffer && _cbBuffer)
    {
        hr = THR(_pStm->Write(_pbBuffer, _cbBuffer, NULL));

        _cbBuffer = 0;                           // reset index to 0
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CStreamWriteBuff::Write
//
//  Synopsis:   Write a string into the buffer
//
//  Arguments:  const   TCHAR *     output string
//                      int         length of the string
//
//  Returns:    Returns S_OK, if successful else the error
//
//----------------------------------------------------------------------------

HRESULT
CStreamWriteBuff::Write(const TCHAR *pch, int cch)
{
            HRESULT hr = S_OK;
    const   TCHAR*  pchER;      // entity reference
            TCHAR   chCur;
            TCHAR   chCR[20];   // character reference
            int     cchWAvail;
            int     nLen;
            // contain indentation
            int     cchIndent = (_cchIndent > 32) ? 32 : _cchIndent;
    const   BOOL    fCp1252 = (_uiWinCodepage == 1252);
    const   TCHAR   chNBSPMaybe = (   TestFlag(WBF_FOR_RTF_CONV)
                                   || TestFlag(WBF_SAVE_PLAINTEXT))
                                  ? WCH_NBSP : 0;
    const   BOOL    fVerbatim = _cPreFormat || TestFlag(WBF_SAVE_VERBATIM);

    // Check if we are suppressing output
    if (_cSuppress)
        return S_OK;

    if( !TestFlag(WBF_FORMATTED) )
        cchIndent = 0;

    if ( !pch || !cch )
        return S_OK;

    if(cch < 0)
        cch = _tcslen(pch);

    chCR[0] = _T('#');

    while (cch)
    {
        if (_fNeedIndent)
        {
            _fNeedIndent = FALSE;

            if (!fVerbatim)
            {
                hr = THR(WriteDirectToMultibyte(' ', cchIndent));
                if (hr)
                    goto CleanUp;
            }
        }

        cchWAvail = _nBlockSize - _cchBuffer;
        // BUGBUG - Removed the following assert as a fix for #484
        // now we are allowing any characters to be saved that the
        // parser parses in
        // AssertSz (*pch, "Illegal String");
        AssertSz (_cchBuffer <= _nBlockSize, "CStreamWriteBuff::Wide Char Buffer overflow");

        chCur = *pch;
        pchER = NULL;

        // If we are in enitise mode and not saving in plaintext,
        // entitise the current char - otherwise, we may convert NBSPs to
        // spaces in two cases:
        // 1. if we're on a FE system and writing plaintext, since those
        //    charsets do not have a multibyte NBSP character.
        // 2. if we're saving for the rtf converter

        nLen = 1;

        if (chCur == chNBSPMaybe)
        {
            chCur = L' ';  // (cthrash) this is a bit of a hack.
        }
        else if (TestFlag(WBF_ENTITYREF) && !TestFlag(WBF_SAVE_PLAINTEXT))
        {
            BOOL fEntitize;

            if (IsAscii(chCur))
            {
                fEntitize =    (chCur == WCH_QUOTATIONMARK && !TestFlag(WBF_NO_DQ_ENTITY))
                            || (chCur == WCH_AMPERSAND     && !TestFlag(WBF_NO_AMP_ENTITY))
                            || (chCur == WCH_LESSTHAN      && !TestFlag(WBF_NO_LT_ENTITY))
                            || (chCur == WCH_GREATERTHAN   && !TestFlag(WBF_NO_LT_ENTITY))
                            || (chCur <  _T(' ')           &&  TestFlag(WBF_CRLF_ENTITY));
            }
            else 
            {
                fEntitize =    !TestFlag(WBF_NO_NAMED_ENTITIES)
                            && (   chCur == WCH_NBSP
                                || chCur == WCH_NONREQHYPHEN);
            }

            if (fEntitize)
            {
                // N.B. (johnv) If we do not have an entry in the entity table,
                // just let the character through as is.  Our superclass
                // CEncodeWriter will take care of entitizing characters not
                // available in the current character set.

                if (chCur < _T(' ') && TestFlag(WBF_CRLF_ENTITY))
                {
                    pchER = chCR;
                    _ultot(chCur, chCR + 1, 10);
                }
                else
                {
                    pchER = LookUpErTable(chCur, fCp1252);
                    Assert(pchER);
                }
                nLen = _tcslen(pchER) + 2; // for & and ;
            }
        }

        // If we do not have enough space in the wide char buffer,
        // move the contents from lastnewline to the end of the
        // buffer to the begining of the buffer
        while(cchWAvail < nLen + 1)
        {
            // If we did not find a place to break in the entire buffer
            // flush the entire buffer.
            if(!_ichLastNewLine)
            {
                // If we need to make space then we should have flushed
                // the line when we went over 80 chars.  We should never
                // have the case where we have a line under 80 chars and
                // we need to make space.
                Assert( _iLastValidBreak == 0 );

                // full buffer without a valid line break
                _iLastValidBreak = _cchBuffer;    // to flush the entire buffer

                hr = FlushWBuffer(FALSE, FALSE);
                if( hr )
                    goto CleanUp;

                _cchBuffer = 0;
                _ichLastNewLine = 0;
                _iLastValidBreak = 0;
                cchWAvail = _nBlockSize;

                Assert( cchWAvail >= nLen + 1 );
            }
            else
            {
                // move the memory from lastnewline
                _cchBuffer = _nBlockSize - _ichLastNewLine - cchWAvail;
                memmove(&_pchBuffer[0],
                        &_pchBuffer[_ichLastNewLine],
                        _cchBuffer * sizeof(TCHAR));
                _iLastValidBreak = _iLastValidBreak - _ichLastNewLine;
                _ichLastNewLine = 0;
                cchWAvail = _nBlockSize - _cchBuffer;
            }
        }

        // If we can break, and we are not saving pre formatted text
        // updated the valid break index.
        if(!TestFlag(WBF_NO_WRAP) && !_cPreFormat)
        {
            if (chCur == _T(' '))           // space is a valid break
            {
                _iLastValidBreak = _cchBuffer+1;
            }
        }

        // Ignore cariage returns and linefeed if we are not
        // are not saving preformatted text
        if(!_cPreFormat && !TestFlag(WBF_KEEP_BREAKS) &&
                (chCur == _T('\r') || chCur == _T('\n')))
        {
            _iLastValidBreak = _cchBuffer;

            pch++;
            cch--;
            continue;
        }

        // write the entitised string or char in to the wide char buffer.
        if(pchER)
        {
            _pchBuffer[_cchBuffer++] = _T('&');
            memcpy(&_pchBuffer[_cchBuffer], pchER, sizeof(TCHAR) *(nLen - 2));
            _cchBuffer += nLen - 2;
            _pchBuffer[_cchBuffer++] = _T(';');
        }
        else
        {
            _pchBuffer[_cchBuffer++] = chCur;

            // For preformatted we have written the \r, now we have to write \n
            //  (unless already present)
            // BUGBUG \r's need \n's when saving from textarea (paulpark)
            if(chCur == _T('\r') && _cPreFormat)
            {
                if (!TestFlag(WBF_SAVE_VERBATIM) &&
                    (cch == 1 ||
                    (cch > 1 && pch[1] != _T('\n'))))
                {
                    _pchBuffer[_cchBuffer++] = _T('\n');
                }
                _iLastValidBreak = _cchBuffer;
                NewLine();
            }
        }

        // If we have more than 80 chars in the wide char buff,
        // and have seen a break, flush them to the ansi char buffer
        if(_ichLastNewLine != _iLastValidBreak &&
                _cchBuffer - _ichLastNewLine + cchIndent > 80)
        {
            hr = NewLine();
            if(hr)
                goto CleanUp;
        }

        cch--;
        pch++;
    }

CleanUp:
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStreamWriteBuff::WriteDirectToMultibyte
//
//  Synopsis:   This function throws characters as is into the multibyte character,
//              unless we are writing out unicode, in which case we add a null
//              character after each byte.
//
//  Returns:    Returns S_OK, if successful else the error
//
//----------------------------------------------------------------------------

HRESULT
CStreamWriteBuff::WriteDirectToMultibyte(CHAR ch, int iRepeat)
{
    HRESULT hr = S_OK;

    // Make sure we have something to do
    if( _cSuppress || iRepeat <= 0 )
        return S_OK;

    if(     _cp == NATIVE_UNICODE_CODEPAGE 
        ||  _cp == NATIVE_UNICODE_CODEPAGE_BIGENDIAN )
    {
        // CONSIDER: support for NONNATIVE (UCS_4 on Win32) support
        WCHAR  tch = WCHAR(ch);
        WCHAR *pch;
        int    cch;

        // Shift for BIGENDIAN -- 8 for Win32, 24 for UNIX
        if (_cp == NATIVE_UNICODE_CODEPAGE_BIGENDIAN )
        {
#ifdef UNIX
            tch <<= 24;
#else
            tch <<= 8;
#endif
        }

        if (_cbBuffer + int(iRepeat * sizeof(TCHAR)) >= _cbBufferMax )
        {
            hr = FlushMBBuffer();
            if( hr )
                goto Cleanup;
        }

        Assert(_cbBufferMax - _cbBuffer >= int(iRepeat * sizeof(TCHAR)));

        cch = min( int((_cbBufferMax - _cbBuffer) / sizeof(TCHAR)), iRepeat);
        pch = (WCHAR *)(_pbBuffer + _cbBuffer);
        _cbBuffer += cch * sizeof(TCHAR);
        while(cch-- > 0)
        {
            *pch++ = tch;
        }
    }
    else
    {
        int cb;

        if (_cbBuffer + iRepeat >= _cbBufferMax)
        {
            hr = FlushMBBuffer();
            if( hr )
                goto Cleanup;
        }

        Assert(_cbBufferMax - _cbBuffer >= iRepeat);

        cb = min(_cbBufferMax - _cbBuffer, iRepeat);
        memset(_pbBuffer + _cbBuffer, ch, cb);
        _cbBuffer += cb;
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CStreamWriteBuff::FlushWBuffer
//
//  Synopsis:   Writes the current line from wide char buffer to the ansi
//              buffer and write new line and spaces if there is any indentation
//
//  Returns:    Returns S_OK, if successful else the error
//
//----------------------------------------------------------------------------

HRESULT
CStreamWriteBuff::FlushWBuffer(BOOL fIndent, BOOL fNewLine)
{
    HRESULT hr = S_OK;
    int     nLineLength;
    int     cchIndent = (_cchIndent > 32) ? 32 : _cchIndent;

    if( !TestFlag(WBF_FORMATTED) )
        cchIndent = 0;

    // new line is called explictly or when the number of chars in the
    // wide char buffer exceed 80.
    if(_cchBuffer - _ichLastNewLine + cchIndent > 80 &&
        _ichLastNewLine != _iLastValidBreak)
    {
        nLineLength = _iLastValidBreak  - _ichLastNewLine;
    }
    else
    {
        // line without a word break or in a pre
        nLineLength = _cchBuffer - _ichLastNewLine;
    }

    // convert wide char to ansi and save in the ansi buffer
    if(nLineLength)
    {
        int cch;

        if(int(sizeof(TCHAR) * nLineLength) > _cbBufferMax - _cbBuffer)
        {
            hr = THR( FlushMBBuffer( ) );
            if( hr )
                goto Cleanup;

            Assert( _pbBuffer != NULL );
        }

        {
            // N.B. (johnv) This will be cleaned up when this class gets
            // re-worked.
            TCHAR* pchBufferPtrSave = _pchBuffer;
            int    cchBufferSave = _cchBuffer;

            _cchBuffer  = nLineLength;
            _pchBuffer += _ichLastNewLine;

            hr = THR( MultiByteFromWideChar( TRUE, &cch ) );

            _pchBuffer = pchBufferPtrSave;
            _cchBuffer = cchBufferSave;

            // N.B. (johnv) Make sure _pchBuffer and _cchBuffer are restored
            // before cleaning up, or you may free invalid memory.
            if( hr )
                goto Cleanup;

        }
    }

    _ichLastNewLine += nLineLength;
    _iLastValidBreak = _ichLastNewLine;

    // If we are not in a preformatted block then write a carriage return,
    // linefeed and spaces for indentation if any.
    if (fNewLine)
    {
        if (!_cPreFormat && !TestFlag(WBF_SAVE_VERBATIM))
        {
            IGNORE_HR(WriteDirectToMultibyte('\r', 1));
            IGNORE_HR(WriteDirectToMultibyte('\n', 1));

            _fNeedIndent = TRUE;
        }
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CStreamWriteBuff::WriteRule
//
//  Synopsis:   Writes a horizontal rule
//
//  Returns:    Returns S_OK, if successful else the error
//
//----------------------------------------------------------------------------

HRESULT CStreamWriteBuff::WriteRule()
{
    HRESULT hr = S_OK;
    int     cchIndent = (_cchIndent > 32) ? 32 : _cchIndent;

    // Check to see if we are in suppress mode
    if (_cSuppress)
        return S_OK;

    if( !TestFlag(WBF_FORMATTED) )
        cchIndent = 0;

    hr = THR(WriteDirectToMultibyte('-', 80 - cchIndent));

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CStreamWriteBuff::Flush()
//
//  Synopsis:   Flush the contents of the stream buffer to the IStream object.
//
//  Returns:    Returns S_OK, if successful else the error
//
//----------------------------------------------------------------------------

HRESULT
CStreamWriteBuff::Flush()
{
    HRESULT hr;

    hr = FlushWBuffer(FALSE, FALSE);
    if(!hr)
    {
        hr = FlushMBBuffer();
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStreamWriteBuff::WriteQuotedText( TCHAR* pch )
//
//  Synopsis:   This function writes the specified text in either single or
//              double quotes.  If necessary, it entitizes the contents
//              of pch.  The rules are as follows:
//
//              1. If pch contains no single quotes or spaces, do not
//                 entitize, and output pch in double quotes.
//              2. If pch contains double quotes but no single quotes,
//                 do not entitize, and output pch in single quotes.
//              3. If pch contains both double and single quotes, output
//                 an entitized pch in double quotes.
//
//  Returns:    Returns S_OK, if successful else the error
//
//----------------------------------------------------------------------------

HRESULT
CStreamWriteBuff::WriteQuotedText( const TCHAR * pch, BOOL fAlwaysQuote )
{
    HRESULT hr = S_OK;
    const TCHAR*  pbuf;
    BOOL    fHasDoubleQuotes = FALSE, fHasSingleQuotes = FALSE;

    // First search for double quotes, single quotes, and characters less
    // than the space character.
    pbuf = pch;
    while( *pbuf )
    {
        if( *pbuf == _T('"') )
            fHasDoubleQuotes = TRUE;
        else if( *pbuf == _T('\'') )
            fHasSingleQuotes = TRUE;
        else if( *pbuf <= _T(' ') )
            fAlwaysQuote = TRUE;        // if we have spaces, we must quote
        else if( *pbuf == _T('<') || *pbuf == _T('>') )
            fAlwaysQuote = TRUE;        // if we have an ASP script, we must quote

        pbuf++;
    }

    if( fAlwaysQuote || fHasDoubleQuotes || fHasSingleQuotes )
    {
        TCHAR   szQuote[2];
        DWORD   dwOldFlags = 0;
        DWORD   dwNewFlags = 0;
    
        if (fHasDoubleQuotes && !fHasSingleQuotes)
        {
            // Although IE5 26994 indicates we should never use single quotes,
            // IE5 53563 indicates we should use single quotes if double quotes are inside
            // the string; 53563 won, so we use single quotes (dbau)
            szQuote[0] = _T('\'');
            szQuote[1] = 0;
            dwNewFlags = WBF_ENTITYREF | WBF_CRLF_ENTITY | WBF_NO_WRAP | WBF_NO_LT_ENTITY | WBF_KEEP_BREAKS | WBF_NO_DQ_ENTITY;
        }
        else
        {
            szQuote[0] = _T('"');
            szQuote[1] = 0;
            // We should not be entitizing LT and GT, since ASP editing using VID counts on that.
            // Although it is not IE40 behavior, this is necessary for ASP editing. (ferhane)
            dwNewFlags = WBF_ENTITYREF | WBF_CRLF_ENTITY | WBF_NO_WRAP | WBF_NO_LT_ENTITY | WBF_KEEP_BREAKS;
        }

 
        hr = Write(szQuote, 1*sizeof(TCHAR), 0);
        if( hr )
            goto Cleanup;

        // Special entity rules when quoting
        dwOldFlags = SetFlags(dwNewFlags);

        hr = Write(pch, _tcslen(pch)*sizeof(TCHAR), 0);
        if( hr )
            goto Cleanup;

        RestoreFlags(dwOldFlags);

        hr = Write(szQuote, 1*sizeof(TCHAR), 0);
        if( hr )
            goto Cleanup;
    }
    else
    {
        // If we do not use quotes, we must entitize all quotes and LT/GT symbols

        DWORD dwOldFlags = SetFlags(WBF_ENTITYREF | WBF_CRLF_ENTITY | WBF_KEEP_BREAKS);

        hr = Write( pch, _tcslen(pch)*sizeof(TCHAR), 0 );

        RestoreFlags( dwOldFlags );
    }


Cleanup:
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     CStreamWriteBuff::SaveNamespaceAttr
//
//----------------------------------------------------------------------------

HRESULT
CStreamWriteBuff::SaveNamespaceAttr(LPTSTR pchNamespace, LPTSTR pchUrn)
{
    HRESULT     hr;
    LPTSTR      pch_XmlNS;

    Assert (pchNamespace);

    // if appears to be originally lower-case
    if (ChrLower(pchNamespace[0]))
    {
        pch_XmlNS = _T(" xmlns:");
    }
    else // if appears to be originally upper-case
    {
        pch_XmlNS = _T(" XMLNS:");
    }

    hr = THR(Write(pch_XmlNS));
    if (hr)
        goto Cleanup;

    hr = THR(Write(pchNamespace));
    if (hr)
        goto Cleanup;

    if (pchUrn)
    {
        hr = THR(Write(_T(" = \"")));
        if (hr)
            goto Cleanup;

        hr = THR(Write(pchUrn));
        if (hr)
            goto Cleanup;

        hr = THR(Write(_T("\"")));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CStreamWriteBuff::SaveNamespaceTag
//
//----------------------------------------------------------------------------

HRESULT
CStreamWriteBuff::SaveNamespaceTag(LPTSTR pchNamespace, LPTSTR pchUrn)
{
    HRESULT     hr;
    LPTSTR      pch_Xml_Namespace_Prefix;
    LPTSTR      pch_Urn;
    DWORD       dwOldFlags;

    dwOldFlags = ClearFlags(WBF_ENTITYREF);

    // if appears to be originally lower-case
    if (ChrLower(pchNamespace[0]))
    {
        pch_Xml_Namespace_Prefix = _T("<?xml:namespace prefix = ");
        pch_Urn                  = _T(" ns = \"");
    }
    else // if appears to be originally upper-case
    {
        pch_Xml_Namespace_Prefix = _T("<?XML:NAMESPACE PREFIX = ");
        pch_Urn                  = _T(" NS = \"");
    }

    hr = THR(Write(pch_Xml_Namespace_Prefix));
    if (hr)
        goto Cleanup;

    hr = THR(Write(pchNamespace));
    if (hr)
        goto Cleanup;

    if (pchUrn && pchUrn[0])
    {
        hr = THR(Write(pch_Urn));
        if (hr)
            goto Cleanup;

        hr = THR(Write(pchUrn));
        if (hr)
            goto Cleanup;

        hr = THR(Write(_T("\"")));
        if (hr)
            goto Cleanup;
    }

    hr = THR(Write(_T(" />")));
    if (hr)
        goto Cleanup;

#if 0
    hr = THR(NewLine());
    if (hr)
        goto Cleanup;
#endif

    RestoreFlags(dwOldFlags);

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CStreamWriteBuff::EnsureNamespaceSaved
//
//----------------------------------------------------------------------------

HRESULT
CStreamWriteBuff::EnsureNamespaceSaved (
    CDoc * pDoc, LPTSTR pchNamespace, LPTSTR pchUrn, XMLNAMESPACETYPE namespaceType)
{
    HRESULT     hr;
    BOOL        fChangeDetected;

    //
    // ensure namespace table
    //

    if (!_pXmlNamespaceTable)
    {
        _pXmlNamespaceTable = new CXmlNamespaceTable(pDoc);
        if (!_pXmlNamespaceTable)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    //
    // register
    //

    hr = THR(_pXmlNamespaceTable->RegisterNamespace(pchNamespace, pchUrn, namespaceType, &fChangeDetected));
    if (hr)
        goto Cleanup;

    //
    // output 
    //

    if (fChangeDetected)
    {
        switch (namespaceType)
        {
        case XMLNAMESPACETYPE_ATTR:

            hr = THR(SaveNamespaceAttr(pchNamespace, pchUrn));
            if (hr)
                goto Cleanup;

            break;

        case XMLNAMESPACETYPE_TAG:

            hr = THR(SaveNamespaceTag(pchNamespace, pchUrn));
            if (hr)
                goto Cleanup;

            break;

        default:
            Assert (0 && "not implemented");
        }
    }

Cleanup:

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CStreamReadBuff::CStreamReadBuff
//
//  Synopsis:   Instantiates our class from a IStream pointer.
//
//  Returns:
//
//----------------------------------------------------------------------------

CStreamReadBuff::CStreamReadBuff( IStream* pIStream, CODEPAGE cp )
                    : _pStm( pIStream ),
                       CEncodeReader( cp, WBUFF_SIZE )
{
    Assert( pIStream );
    _pStm->AddRef( );
    _eof              = FALSE;
    _lastGetCharError = S_OK;
    _iIndex           = -1;
    _cchChunk         = 0;
}

CStreamReadBuff::~CStreamReadBuff( )
{
    _pStm->Release( );
}

//+---------------------------------------------------------------------------
//
//  Member:     CStreamReadBuff::Get
//
//  Synopsis:   Copies at most cbBuffer bytes into pBuffer, but less if we encounter
//              end of file or an error first.  *pcbRead gets set to the number of
//              bytes copied.  No null termination.
//
//  Returns:    Returns S_OK, if successful else the error
//
//----------------------------------------------------------------------------

HRESULT
CStreamReadBuff::Get( TCHAR* pBuffer, int cchAvailable, int* pchRead )
{
    int lCharsToCopy;
    TCHAR* pBufCur = pBuffer;

    *pchRead = 0;

    if( _eof )
        RRETURN1( S_FALSE, S_FALSE );

    // Loop until we have exhausted the size of pBuffer or encounter an error/end of file
    while( cchAvailable > 0 && !_eof )
    {
        if( _iIndex < 0 || _iIndex >= _cchChunk )
        {
            // If the _iIndex is set to an invalid position, we need to read another chunk.
            HRESULT hr = ReadChunk( );
            if( hr )
            {
                if( !(*pchRead) )
                    // If we didn't read anything at all and get an error in ReadChunk return it.
                    RRETURN1( hr, S_FALSE );
                else
                    break;
            }
        }
        // Copy into pBuffer but be careful not to overflow it
        lCharsToCopy = min( cchAvailable, _cchChunk - _iIndex );
        memcpy( (void*)pBufCur, _achBuffer+_iIndex, lCharsToCopy * sizeof(TCHAR) );
        cchAvailable -= lCharsToCopy;
        _iIndex   += lCharsToCopy;
        *pchRead  += lCharsToCopy;
        pBufCur   += lCharsToCopy;
    }

    RRETURN( S_OK );
}

//+---------------------------------------------------------------------------
//
//  Member:     CStreamReadBuff::GetLine
//
//  Synopsis:   Copies the next non-empty 'line' of text in the supplied buffer,
//              copying at most cchBuffer-1 bytes.  Null terminates the buffer.
//
//  Returns:    Returns S_OK, if successful else the error
//
//----------------------------------------------------------------------------

HRESULT
CStreamReadBuff::GetLine( TCHAR* pBuffer, int cchBuffer )
{
    int  index = 0;

    if( _eof )
        RRETURN1( S_FALSE, S_FALSE );

    // Read until we get to end of file or to a cr or lf, copying into the buffer
    while( GetChar() != 0 && GetChar() != _T('\r') && GetChar() != _T('\n') &&
            index < cchBuffer - 1)
    {
        pBuffer[ index++ ] = GetChar( );
        Advance( );
    }

    pBuffer[ index ] = 0;

    // Skip over cr/lfs
    if( GetChar() == _T('\r') )
        Advance( );
    if( GetChar() == _T('\n') )
        Advance( );

    RRETURN( S_OK );
}

//+---------------------------------------------------------------------------
//
//  Member:     CStreamReadBuff::ReadChunk
//
//  Synopsis:   Reads a chunk of data from the stream at the current position,
//              and populates the _achBuffer and _abBuffer internal buffers.
//              Performs a MultiByteToWideChar conversion to fill _achBuffer
//              from _abBuffer, depending on the current mapping mode.
//
//  Returns:    Returns S_OK, if successful else the error
//
//----------------------------------------------------------------------------

HRESULT
CStreamReadBuff::ReadChunk( )
{
    ULONG cbRead;
    HRESULT hr;

    if( _eof )
        RRETURN1( S_FALSE, S_FALSE );

    hr = THR( PrepareToEncode( ) );
    if( hr )
        RRETURN( hr );

    // Read a chunk from the stream

    Assert( _pbBuffer );
    Assert( _cbBuffer + BlockSize() <= _cbBufferMax );

    hr = _pStm->Read(_pbBufferPtr, BlockSize(), &cbRead);
    if( hr == S_FALSE )
    {
        Assert(cbRead == 0);
        cbRead = 0;
    }
    else if( hr )
        RRETURN( hr );

    _cbBuffer += cbRead;
    _pbBufferPtr = _pbBuffer;

    // If we didn't read anything at all, we have an end of file condition
    if( cbRead == 0 )
    {
        _iIndex   = -1;
        _cchChunk = 0;
        _eof      = TRUE;
        RRETURN1( S_FALSE, S_FALSE );
    }

    WideCharFromMultiByte( TRUE, &_cchChunk );

    _achBuffer = _pchBuffer;
    _iIndex    = 0;

    RRETURN( S_OK );
}


//+---------------------------------------------------------------------------
//
//  Member:     CStreamReadBuff::MakeRoomForChars
//
//  Synopsis:   Make sure we have enough room for at least 'cch' characters in
//              out wide char buffer.
//
//  Returns:    Returns S_OK, if successful else the error
//
//----------------------------------------------------------------------------

// BUGBUG (johnv) Move this to the CEncodeReader class as the default MakeRoomForChars.

HRESULT
CStreamReadBuff::MakeRoomForChars(int cch)
{
    HRESULT hr = S_OK;

    if (!_pchBuffer)
    {
        // round up to block size multiple >= cch+1
        _cchBuffer = (cch + BlockSize()) & ~(BlockSize()-1);

        _pchBuffer = (TCHAR*)MemAlloc(Mt(CStreamReadBuff), _cchBuffer * sizeof(TCHAR));
        if (!_pchBuffer)
            return(E_OUTOFMEMORY);

        _pchEnd = _pchBuffer;
    }
    else
    {
        int cchCur  = (_pchEnd - _pchBuffer);
        int cchNeed = cchCur + cch;

        if (cchNeed >= _cchBuffer)
        {
            TCHAR *pchNewBuffer = _pchBuffer;

            cchNeed = (cchNeed + BlockSize() - 1) & ~(BlockSize()-1);

            hr = THR(MemRealloc(Mt(CStreamReadBuff), (void**)&pchNewBuffer, cchNeed * sizeof(TCHAR)));
            if (hr)
                goto Cleanup;

            _pchBuffer = pchNewBuffer;
            _pchEnd    = _pchBuffer + cchCur;
            _cchBuffer = cchNeed;
        }
    }

    CEncodeReader::MakeRoomForChars(cch);

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CStreamReadBuff::GetChar
//
//  Synopsis:   Returns the character at the current position in the stream,
//              or 0 if an error has occurred.
//
//  Returns:
//
//----------------------------------------------------------------------------

TCHAR
CStreamReadBuff::GetChar( )
{
    if( _eof )
        return 0;

    // If we are not within range, advance to get there
    if( _iIndex < 0 || _iIndex >= _cchChunk )
    {
        HRESULT hr = Advance( );
        if( hr )
        {
            _lastGetCharError = hr;
            return 0;
        }
    }

    return _achBuffer[ _iIndex ];
}

//+---------------------------------------------------------------------------
//
//  Member:     CStreamReadBuff::Advance
//
//  Synopsis:   Move the current position one character to the right.  Note that
//              Advance() when the chunk buffer is empty will keep us at position
//              zero.
//
//  Returns:
//
//----------------------------------------------------------------------------
HRESULT
CStreamReadBuff::Advance( )
{
    if( _eof || _lastGetCharError )
        RRETURN1( S_FALSE, S_FALSE );

    if( _iIndex < 0 || _iIndex >= _cchChunk - 1)
    {
        HRESULT hr = ReadChunk( );
        if( hr )
            RRETURN1( hr, S_FALSE );
    }
    else
        ++_iIndex;

    RRETURN( S_OK );
}

//+---------------------------------------------------------------------------
//
//  Member:     CStreamReadBuff::SetPosition
//
//  Synopsis:   Set the current stream position to the supplied absolute offset.
//              If mode==STREAM_SEEK_CUR, lPosition is relative to the current
//              position in the file.  mode==STREAM_SEEK_SET, lPosition is
//              absolute.
//
//  Returns:    Returns S_OK, if successful else the error
//
//----------------------------------------------------------------------------

HRESULT
CStreamReadBuff::SetPosition( LONG lPosition, DWORD dwOrigin )
{
    LARGE_INTEGER liPosition;
    HRESULT hr;

#ifdef UNIX
    QUAD_PART(liPosition) = (LONGLONG)lPosition;
#else
    liPosition.QuadPart = (LONGLONG)lPosition;
#endif

    hr = _pStm->Seek( liPosition, dwOrigin, NULL );
    if( hr )
        RRETURN( hr );

    _iIndex   = -1;       // Signal Read routines that we need to read again
    _eof      = FALSE;

    RRETURN( S_OK );
}

//+---------------------------------------------------------------------------
//
//  Member:     CStreamReadBuff::GetPosition
//
//  Synopsis:   Find the current absolue position as a long.
//
//  Returns:    Returns S_OK, if successful else the error
//
//----------------------------------------------------------------------------

HRESULT
CStreamReadBuff::GetPosition( LONG* plPositionRet )
{
    HRESULT hr;
    ULARGE_INTEGER lPosition;
    LARGE_INTEGER   zero = { 0, 0 };

    // Get our current position in the stream
    hr = _pStm->Seek( zero, STREAM_SEEK_CUR, &lPosition );
    if( hr )
        RRETURN( hr );

#ifdef UNIX
    *plPositionRet = (LONG) U_QUAD_PART(lPosition);
#else
    *plPositionRet = (LONG) lPosition.QuadPart;
#endif

    // Adjust for any characters we have not yet eaten from the chunk buffer
    if( _iIndex >= 0 && _iIndex < _cchChunk )
        *plPositionRet -= _cchChunk - _iIndex;

    RRETURN( S_OK );
}

//+---------------------------------------------------------------------------
//
//  Member:     CStreamReadBuff::GetStringValue
//
//  Synopsis:   Look for Tag:Value, and copy Value into pString (though at most
//              cchString-1) if you find it.  Otherwise pString will be empty.
//
//  Returns:    Returns S_OK, if successful else the error
//
//----------------------------------------------------------------------------

HRESULT
CStreamReadBuff::GetStringValue( TCHAR* pTag, TCHAR* pString, int cchString )
{
    HRESULT hr;
    TCHAR buffer[ 256 ];
    int  cbTag;

    pString[0] = 0;

    // Read in a line from the stream
    hr = GetLine( buffer, ARRAY_SIZE( buffer ) );
    if( hr )
        RRETURN( hr );

    cbTag = _tcsclen( pTag );

    // Make sure the tag matches
    if( memcmp( pTag, buffer, cbTag * sizeof(TCHAR) ) )
        RRETURN1( S_FALSE, S_FALSE );

    // Make sure we have a semicolon after in
    if( buffer[cbTag] != _T(':') )
        RRETURN1( S_FALSE, S_FALSE );

    // Copy contents after the colon into pString
    int n;
    for( n = 0; n < cchString - 1 && buffer[ cbTag + n + 1]; ++n )
        pString[ n ]  = buffer[ cbTag + n + 1 ];

    pString[ n ] = 0;

    RRETURN( S_OK );
}

//+---------------------------------------------------------------------------
//
//  Member:     CStreamReadBuff::GetLongValue
//
//  Synopsis:   Same as GetStringValue, except returns a number.
//
//  Returns:    Returns S_OK, if successful else the error
//
//----------------------------------------------------------------------------

HRESULT
CStreamReadBuff::GetLongValue( TCHAR* pTag, LONG* pLong )
{
    TCHAR numberAsString[ 33 ];
    HRESULT hr;

    // Simply call GetString value and then convert what it returns to a long
    hr = GetStringValue( pTag, numberAsString, ARRAY_SIZE(numberAsString) );
    if( hr )
    {
        *pLong = 0;
        RRETURN1( hr, S_FALSE );
    }
    else
    {
        IGNORE_HR(ttol_with_error( numberAsString, pLong));
        RRETURN( S_OK );
    }
}
