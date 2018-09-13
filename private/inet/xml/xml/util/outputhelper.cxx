/*
 * @(#)OutputHelper.cxx 1.0 6/10/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

#include "outputhelper.hxx"

DEFINE_CLASS_MEMBERS(OutputHelper, _T("OutputHelper"), Base);

const int OutputHelper::BUFFERSIZE = 2048;

OutputHelper* OutputHelper::newOutputHelper(IStream* out, int style, String * encoding)
{
    OutputHelper* o = new OutputHelper();
    o->_pStm = out;
    o->_iStyle = style;
    o->_iIndent = 0;
    o->_chLast = 0;
    o->_pchBuf = new WCHAR[BUFFERSIZE];
    o->_iUsed = 0;   
    o->_fEncoding = true;
    o->_pEncoding = encoding;
    o->_fWriteError = false;
    return o;
}

/**
 * Write an Unicode character.
 */
void OutputHelper::_hardWrite()
{
    ULONG l;
    HRESULT hr = S_OK;

    int iUsed = _iUsed;
    _iUsed = 0;

    // Do not do any more writes if _pStm has already failed.
    if (! _fWriteError && NULL != (IStream*)_pStm)
    {
        hr = _pStm->Write((void*)(_pchBuf), sizeof(WCHAR)*iUsed, &l);
        if (FAILED(hr))
        {
            _fWriteError = true;
            if (_pEncoding && E_UNEXPECTED == hr)
                Exception::throwE(E_FAIL, XML_BAD_ENCODING, _pEncoding, null);
            else 
                Exception::throwE(hr);
        }
    }
}

/**
 * Write the Unicode character.
 */
#if NEVER
void OutputHelper::_write(WCHAR c)
{
    _pchBuf[_iUsed++] = c;
    _chLast = c;
    if (_iUsed == BUFFERSIZE)
        _hardWrite();
}
#endif

/**
 * Write the Unicode character.
 */
void OutputHelper::write(WCHAR c)
{
    if (_fHoldingIndent)
        actuallyWriteIndent();
    _write( c);
}

/**
 * Write the given string.
 */
void OutputHelper::write(const String * str)
{
    const WCHAR * chars = str->getWCHARPtr();
    write(chars, str->length());
}


void OutputHelper::write(const WCHAR * c)
{
    write(c, lstrlen(c));
}


void OutputHelper::write(const WCHAR * c, int len)
{
    if (_fHoldingIndent)
        actuallyWriteIndent();

    while (len--)
    {
#ifdef WIN32
       // This is new code that goes hand in hand with the code in EntityReader
        // for putting newlines back out in their correct format for windows
        // platform.
        if (*c == 0xa && _chLast != 0xd)
            _write(0xd); // put 0xd back in...
#endif
        _write(*c++);
    }
}

/**
 * This method writes out the fully qualified name, using
 * the appropriate short name syntax. For example: "foo::bar".
 * @param n the name being written
 * @param ns the name space which defines the context
 * in which the name was defined.
 */
void OutputHelper::write(NameDef * n)
{
    if (_fHoldingIndent)
        actuallyWriteIndent();
    write(n->toString());
}

/**
 * Write out the string with quotes around it.
 * This method uses the quotes that are appropriate for the string.
 * I.E. if the string contains a ' then it uses ", & vice versa.
 */
void OutputHelper::writeString(const String * str, bool fIsPCData)
{
    if (str->length() > 0)
        writeString(str->getWCHARPtr(), str->length(), fIsPCData);
}

void OutputHelper::writeString(const WCHAR * c, int len, bool fIsPCData)
{
    if (_fHoldingIndent)
        actuallyWriteIndent();

    if (len < 0)
        len = lstrlen(c);

    while (len--)
    {
#ifndef UNIX
        if (*c == 0xa && _chLast != 0xd)
            _write(0xd); // put 0xd back in...
#endif
        int ch = *c++;
        switch (ch)
        {
        case _T('"'): 
            if (fIsPCData)
                _write((TCHAR)ch);
            else
                write(_T("&quot;"));
            break;

        case _T('&'):
            write(_T("&amp;"));
            break;

        case _T('<'):
            write(_T("&lt;"));
            break;

        case _T('>'):
            write(_T("&gt;"));
            break;
            
        default:
            _write((TCHAR)ch);
        }
    }
}

/**
 * Write a new line or do nothing if output style is COMPACT.
 */
void OutputHelper::writeNewLine()
{
    if (_iStyle == PRETTY)
    {
        _write('\r');
        _write('\n');
        _fHoldingIndent = true;
    }
}

/**
 * Write the appropriate indent - given current indent level,
 * or do nothing if output style is COMPACT.
 */
void OutputHelper::writeIndent()
{
    if (_iStyle == PRETTY)
        _fHoldingIndent = true;
}

void 
OutputHelper::actuallyWriteIndent()
{
    _fHoldingIndent = false;
    for (int i = 0; i < _iIndent; i++)
        _write('\t');
}void 
OutputHelper::close()
{
	TRY
	{
	    flush();
	}
	CATCH
	{
		_iUsed = 0;
		// BUGBUG: we loose the exception....
	}
	ENDTRY

    _pStm = null;
}

void 
OutputHelper::finalize()
{
	Assert(_iUsed == 0);

    delete [] _pchBuf;
    _pchBuf = null;

    super::finalize();
}
