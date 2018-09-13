/*
 * @(#)CharEncoder.cxx 1.0 6/10/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
#include "core.hxx"
#pragma hdrstop

#include "charencoder.hxx"

#include <shlwapip.h>   // IsCharSpace
#ifdef UNIX
#include <lendian.hpp>
#endif

#ifdef UNIX
// Not needed under UNIX
#else
#ifndef _WIN64
#include <w95wraps.h>
#endif // _WIN64
#endif /* UNIX */

//
// Delegate other charsets to mlang
//
const EncodingEntry CharEncoder::charsetInfo [] = 
{
    { CP_1250, _T("WINDOWS-1250"), 1, wideCharFromMultiByteWin32, wideCharToMultiByteWin32 }, 
    { CP_1251, _T("WINDOWS-1251"), 1, wideCharFromMultiByteWin32, wideCharToMultiByteWin32 }, 
    { CP_1252, _T("WINDOWS-1252"), 1, wideCharFromMultiByteWin32, wideCharToMultiByteWin32 },
    { CP_1253, _T("WINDOWS-1253"), 1, wideCharFromMultiByteWin32, wideCharToMultiByteWin32 }, 
    { CP_1254, _T("WINDOWS-1254"), 1, wideCharFromMultiByteWin32, wideCharToMultiByteWin32 },
    { CP_1257, _T("WINDOWS-1257"), 1, wideCharFromMultiByteWin32, wideCharToMultiByteWin32 },
    { CP_UCS_4, _T("UCS-4"), 4, wideCharFromUcs4Bigendian, wideCharToUcs4Bigendian },
    { CP_UCS_2, _T("ISO-10646-UCS-2"), 2, wideCharFromUcs2Bigendian, wideCharToUcs2Bigendian },
    { CP_UCS_2, _T("UCS-2"), 2, wideCharFromUcs2Bigendian, wideCharToUcs2Bigendian },
    { CP_UCS_2, _T("UNICODE-2-0-UTF-16"), 2, wideCharFromUcs2Bigendian, wideCharToUcs2Bigendian },
    { CP_UCS_2, _T("UTF-16"), 2, wideCharFromUcs2Bigendian, wideCharToUcs2Bigendian },
    { CP_UTF_8, _T("UNICODE-1-1-UTF-8"), 3, wideCharFromUtf8, wideCharToUtf8 },
    { CP_UTF_8, _T("UNICODE-2-0-UTF-8"), 3, wideCharFromUtf8, wideCharToUtf8 },
    { CP_UTF_8, _T("UTF-8"), 3, wideCharFromUtf8, wideCharToUtf8 }
};

IMultiLanguage * CharEncoder::pMultiLanguage = NULL;

Encoding * Encoding::newEncoding(const TCHAR * s, ULONG len, bool endian, bool mark)
{
    Encoding * e = new_ne Encoding();
    if (e == NULL)
        return NULL;
    e->charset = new_ne TCHAR[len + 1];
    if (e->charset == NULL)
    {
        delete e;
        return NULL;
    }
    ::memcpy(e->charset, s, sizeof(TCHAR) * len);
    e->charset[len] = 0; // guarentee NULL termination.
    e->littleendian = endian;
    e->byteOrderMark = mark;
    return e;
}

Encoding::~Encoding()
{
    if (charset != NULL)
    {
        delete [] charset;
    }
}

int CharEncoder::getCharsetInfo(const TCHAR * charset, CODEPAGE * pcodepage, UINT * mCharSize)
{
    CPINFO cpinfo;

    for (int i = LENGTH(charsetInfo) - 1; i >= 0; i--)
    {
        if (StrCmpI(charset, charsetInfo[i].charset) == 0)
        {             
            //
            // test whether we can handle it locally or not
            // BUGBUG(HACK) the index number may change if we change charsetInfo
            //
            if (i > 5 || GetCPInfo(charsetInfo[i].codepage, &cpinfo))
            {
                *pcodepage = charsetInfo[i].codepage;
                *mCharSize = charsetInfo[i].maxCharSize;
                return i;
            }
            else
            {
                break;
            }
        }
    }

    //
    // delegate to MLANG then
    //
    MIMECSETINFO mimeCharsetInfo;
    HRESULT hr;

    hr = _EnsureMultiLanguage();
    if (hr == S_OK)
    {
        hr = pMultiLanguage->GetCharsetInfo((TCHAR*)charset, &mimeCharsetInfo);
        if (hr == S_OK)
        {
            *pcodepage = mimeCharsetInfo.uiInternetEncoding;
            if (GetCPInfo(*pcodepage, &cpinfo))
                *mCharSize = cpinfo.MaxCharSize;
            else // if we don't know the max size, assume a large size
                *mCharSize = 4;
            return -1;
        }
    }

    return -2;
}

extern HRESULT CreateMultiLanguage(IMultiLanguage ** ppUnk);

HRESULT CharEncoder::_EnsureMultiLanguage()
{
    return CreateMultiLanguage(&pMultiLanguage);
}

/**
 * get information about a code page identified by <code> encoding </code>
 */
HRESULT CharEncoder::getWideCharFromMultiByteInfo(Encoding * encoding, CODEPAGE * pcodepage, WideCharFromMultiByteFunc ** pfnWideCharFromMultiByte, UINT * mCharSize)
{
    HRESULT hr = S_OK;

    int i = getCharsetInfo(encoding->charset, pcodepage, mCharSize);
    if (i >= 0) // in our short list
    {
        switch (*pcodepage)
        {
        case CP_UCS_2:
            if (encoding->littleendian)
                *pfnWideCharFromMultiByte = wideCharFromUcs2Littleendian;
            else
                *pfnWideCharFromMultiByte = wideCharFromUcs2Bigendian;
            break;
        case CP_UCS_4:
            if (encoding->littleendian)
                *pfnWideCharFromMultiByte = wideCharFromUcs4Littleendian;
            else
                *pfnWideCharFromMultiByte = wideCharFromUcs4Bigendian;
            break;
        default:
            *pfnWideCharFromMultiByte = charsetInfo[i].pfnWideCharFromMultiByte;
            break;
        }
    }
    else if (i == -1) // delegate to MLANG
    {
        hr = pMultiLanguage->IsConvertible(*pcodepage, CP_UCS_2);
        if (S_OK == hr) 
            *pfnWideCharFromMultiByte = wideCharFromMultiByteMlang;
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

/**
 * get information about a code page identified by <code> encoding </code>
 */
HRESULT CharEncoder::getWideCharToMultiByteInfo(Encoding * encoding, CODEPAGE * pcodepage, WideCharToMultiByteFunc ** pfnWideCharToMultiByte, UINT * mCharSize)
{
    HRESULT hr = S_OK;

    int i = getCharsetInfo(encoding->charset, pcodepage, mCharSize);
    if (i >= 0) // in our short list
    {
        switch (*pcodepage)
        {
        case CP_UCS_2:
            if (encoding->littleendian)
                *pfnWideCharToMultiByte = wideCharToUcs2Littleendian;
            else
                *pfnWideCharToMultiByte = wideCharToUcs2Bigendian;
            break;
        case CP_UCS_4:
            if (encoding->littleendian)
                *pfnWideCharToMultiByte = wideCharToUcs4Littleendian;
            else
                *pfnWideCharToMultiByte = wideCharToUcs4Bigendian;
            break;
        default:
            *pfnWideCharToMultiByte = charsetInfo[i].pfnWideCharToMultiByte;
            break;
        }
    }
    else if (i == -1) // delegate to MLANG
    {
        hr = pMultiLanguage->IsConvertible(CP_UCS_2, *pcodepage);
        if (hr == S_OK)
            *pfnWideCharToMultiByte = wideCharToMultiByteMlang;
        else
            hr = E_FAIL;
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

/**
 * Scans rawbuffer and translates UTF8 characters into UNICODE characters 
 */
HRESULT CharEncoder::wideCharFromUtf8(DWORD* pdwMode, CODEPAGE codepage, BYTE* bytebuffer,
                                            UINT * cb, TCHAR * buffer, UINT * cch)
{

#if 0
    // Just for the record - I tried this and measured it and it's twice as
    // slow as our hand-crafted code.

    // Back up if end of buffer is the second or third byte of a multi-byte 
    // encoding since MultiByteToWideChar cannot handle this case.  These second
    // and third bytes are easy to identify - they always start with the bit
    // pattern 0x10xxxxxx.

    UINT remaining = 0;
    UINT count;
    int endpos = (int)*cb;

    while (endpos > 0 && (bytebuffer[endpos-1] & 0xc0) == 0x80)
    {
        endpos--;
        remaining++;
    }
    if (endpos > 0)
    {
        count = MultiByteToWideChar(CP_UTF8, 0, bytebuffer, endpos, buffer, *cch);
        if (count == 0)
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }
#else
    UINT remaining = *cb;
    UINT count = 0;
    UINT max = *cch;
    ULONG ucs4;

    // UTF-8 multi-byte encoding.  See Appendix A.2 of the Unicode book for more info.
    //
    // Unicode value    1st byte    2nd byte    3rd byte    4th byte
    // 000000000xxxxxxx 0xxxxxxx
    // 00000yyyyyxxxxxx 110yyyyy    10xxxxxx
    // zzzzyyyyyyxxxxxx 1110zzzz    10yyyyyy    10xxxxxx
    // 110110wwwwzzzzyy+ 11110uuu   10uuzzzz    10yyyyyy    10xxxxxx
    // 110111yyyyxxxxxx, where uuuuu = wwww + 1
    WCHAR c;
    bool valid = true;

    while (remaining > 0 && count < max)
    {
        // This is an optimization for straight runs of 7-bit ascii 
        // inside the UTF-8 data.
        c = *bytebuffer;
        if (c & 0x80)   // check 8th-bit and get out of here
            break;      // so we can do proper UTF-8 decoding.
        *buffer++ = c;
        bytebuffer++;
        count++;
        remaining--;
    }

    while (remaining > 0 && count < max)
    {
        UINT bytes = 0;
        for (c = *bytebuffer; c & 0x80; c <<= 1)
            bytes++;

        if (bytes == 0) 
            bytes = 1;

        if (remaining < bytes)
        {
            break;
        }
         
        c = 0;
        switch ( bytes )
        {
            case 6: bytebuffer++;    // We do not handle ucs4 chars
            case 5: bytebuffer++;    // except those on plane 1
                    valid = false;
                    // fall through
            case 4: 
                    // Do we have enough buffer?
                    if (count >= max - 1)
                        goto Cleanup;

                    // surrogate pairs
                    ucs4 = ULONG(*bytebuffer++ & 0x07) << 18;
                    if ((*bytebuffer & 0xc0) != 0x80)
                        valid = false;
                    ucs4 |= ULONG(*bytebuffer++ & 0x3f) << 12;
                    if ((*bytebuffer & 0xc0) != 0x80)
                        valid = false;
                    ucs4 |= ULONG(*bytebuffer++ & 0x3f) << 6;
                    if ((*bytebuffer & 0xc0) != 0x80)
                        valid = false;                    
                    ucs4 |= ULONG(*bytebuffer++ & 0x3f);

                    // For non-BMP code values of ISO/IEC 10646, 
                    // only those in plane 1 are valid xml characters
                    if (ucs4 > 0x10ffff)
                        valid = false;

                    if (valid)
                    {
                        // first ucs2 char
                        *buffer++ = (USHORT)((ucs4 - 0x10000) / 0x400 + 0xd800);
                        count++;
                        // second ucs2 char
                        c = (USHORT)((ucs4 - 0x10000) % 0x400 + 0xdc00);
                    }
                    break;

            case 3: c  = WCHAR(*bytebuffer++ & 0x0f) << 12;    // 0x0800 - 0xffff
                    if ((*bytebuffer & 0xc0) != 0x80)
                        valid = false;
                    // fall through
            case 2: c |= WCHAR(*bytebuffer++ & 0x3f) << 6;     // 0x0080 - 0x07ff
                    if ((*bytebuffer & 0xc0) != 0x80)
                        valid = false;
                    c |= WCHAR(*bytebuffer++ & 0x3f);
                    break;
                    
            case 1:
                c = TCHAR(*bytebuffer++);                      // 0x0000 - 0x007f
                break;

            default:
                valid = false; // not a valid UTF-8 character.
                break;
        }

        // If the multibyte sequence was illegal, store a FFFF character code.
        // The Unicode spec says this value may be used as a signal like this.
        // This will be detected later by the parser and an error generated.
        // We don't throw an exception here because the parser would not yet know
        // the line and character where the error occurred and couldn't produce a
        // detailed error message.

        if (! valid)
        {
            c = 0xffff;
            valid = true;
        }

        *buffer++ = c;
        count++;
        remaining -= bytes;
    }
#endif

Cleanup:
    // tell caller that there are bytes remaining in the buffer to
    // be processed next time around when we have more data.
    *cb -= remaining;
    *cch = count;
    return S_OK;
}


/**
 * Scans bytebuffer and translates UCS2 big endian characters into UNICODE characters 
 */
HRESULT CharEncoder::wideCharFromUcs2Bigendian(DWORD* pdwMode, CODEPAGE codepage, BYTE* bytebuffer,
                                            UINT * cb, TCHAR * buffer, UINT * cch)
{
    UINT num = *cb >> 1; 
    if (num > *cch)
        num = *cch;
    for (UINT i = num; i > 0; i--)
    {
        *buffer++ = ((*bytebuffer) << 8) | (*(bytebuffer + 1));
        bytebuffer += 2;
    }
    *cch = num;
    *cb = num << 1;
    return S_OK;
}


/**
 * Scans bytebuffer and translates UCS2 little endian characters into UNICODE characters 
 */
HRESULT CharEncoder::wideCharFromUcs2Littleendian(DWORD* pdwMode, CODEPAGE codepage, BYTE* bytebuffer,
                                            UINT * cb, TCHAR * buffer, UINT * cch)
{
    UINT num = *cb / 2; // Ucs2 is two byte unicode.
    if (num > *cch)
        num = *cch;


#ifndef UNIX
    // Optimization for windows platform where little endian maps directly to WCHAR.
    // (This increases overall parser performance by 5% for large unicode files !!)
    ::memcpy(buffer, bytebuffer, num * sizeof(TCHAR));
#else
    for (UINT i = num; i > 0 ; i--)
    {
        // we want the letter 'a' to be 0x0000006a.
        *buffer++ = (*(bytebuffer+1)<<8) | (*bytebuffer); 
        bytebuffer += 2;
    }
#endif
    *cch = num;
    *cb = num * 2;
    return S_OK;
}


/**
 * Scans bytebuffer and translates UCS4 big endian characters into UNICODE characters 
 */
HRESULT CharEncoder::wideCharFromUcs4Bigendian(DWORD* pdwMode, CODEPAGE codepage, BYTE* bytebuffer,
                                            UINT * cb, TCHAR * buffer, UINT * cch)
{
    UINT num = *cb >> 2;
    if (num > *cch)
        num = *cch;
    for (UINT i = num; i > 0; i--)
    {
#ifndef UNIX
        if (*bytebuffer != 0 || *(bytebuffer + 1) != 0)
        {
            return XML_E_INVALID_UNICODE;
        }
        *buffer++ = (*(bytebuffer + 2) << 8) | (*(bytebuffer + 3));
#else
        *buffer++ = ((*bytebuffer)<<24) | (*(bytebuffer+1)<<16) | (*(bytebuffer+2)<<8) | (*(bytebuffer+3));
#endif
        bytebuffer += 4;
    }
    *cch = num;
    *cb = num << 2;
    return S_OK;
}


/**
 * Scans bytebuffer and translates UCS4 little endian characters into UNICODE characters 
 */
HRESULT CharEncoder::wideCharFromUcs4Littleendian(DWORD* pdwMode, CODEPAGE codepage, BYTE* bytebuffer,
                                            UINT * cb, TCHAR * buffer, UINT * cch)
{
    UINT num = *cb >> 2; // Ucs4 is two byte unicode.
    if (num > *cch)
        num = *cch;
    for (UINT i = num; i > 0 ; i--)
    {
#ifndef UNIX
        *buffer++ = (*(bytebuffer+1)<<8) | (*bytebuffer);
        if (*(bytebuffer + 2) != 0 || *(bytebuffer + 3) != 0)
        {
            return XML_E_INVALID_UNICODE;
        }
#else
        *buffer++ = (*(bytebuffer+3)<<24) | (*(bytebuffer+2)<<16) | (*(bytebuffer+1)<<8) | (*bytebuffer);
#endif
        bytebuffer += 4;
    }
    *cch = num;
    *cb = num << 2;
    return S_OK;
}


/**
 * Scans bytebuffer and translates characters of charSet identified by 
 * <code> codepage </code> into UNICODE characters, 
 * using Win32 function MultiByteToWideChar() for encoding
 */
HRESULT CharEncoder::wideCharFromMultiByteWin32(DWORD* pdwMode, CODEPAGE codepage, BYTE* bytebuffer,
                                            UINT * cb, TCHAR * buffer, UINT * cch)
{
    HRESULT hr = S_OK;
    *cch = ::MultiByteToWideChar(codepage, MB_PRECOMPOSED,
                                 (char*)bytebuffer, *cb,
                                 buffer, *cch);
    if (*cch == 0)
        hr = GetLastError();
    return hr;
}


/**
 * Scans bytebuffer and translates multibyte characters into UNICODE characters,
 * using Mlang for encoding
 */
HRESULT CharEncoder::wideCharFromMultiByteMlang(DWORD* pdwMode, CODEPAGE codepage, BYTE* bytebuffer,
                                            UINT * cb, TCHAR * buffer, UINT * cch)
{
    HRESULT hr;
    checkhr2(_EnsureMultiLanguage());
    checkhr2(pMultiLanguage->ConvertStringToUnicode(pdwMode, codepage, 
                                 (char*)bytebuffer, cb, 
                                 buffer, cch ));
    return S_OK;
}


/**
 * Scans buffer and translates Unicode characters into Ucs2 big endian characters 
 */
HRESULT CharEncoder::wideCharToUcs2Bigendian(DWORD* pdwMode, CODEPAGE codepage, TCHAR * buffer, 
                                           UINT *cch, BYTE* bytebuffer, UINT * cb)
{
    UINT num = (*cb) >> 1; 
    if (num > *cch)
        num = *cch;
    // BUGBUG - what do we do about Unix where WCHAR is 4 bytes ?
    // Currently we just throw away the high WORD - but I don't know how else
    // to do it, since UCS2 is 2-byte unicode by definition.
    for (UINT i = num; i > 0; i--)
    {
        *bytebuffer++ = (*buffer) >> 8;
        *bytebuffer++ = (*buffer++) & 0xFF;
    }
    *cch = num;
    *cb = num << 1;
    return S_OK;
}


/**
 * Scans buffer and translates Unicode characters into Ucs2 little endian characters
 */
HRESULT CharEncoder::wideCharToUcs2Littleendian(DWORD* pdwMode, CODEPAGE codepage, TCHAR * buffer, 
                                              UINT *cch, BYTE* bytebuffer, UINT * cb)
{
    UINT num = (*cb) >> 1;
    if (num > *cch)
        num = *cch;

    // BUGBUG - what do we do about Unix where WCHAR is 4 bytes ?
    // Currently we just throw away the high WORD - but I don't know how else
    // to do it, since UCS2 is 2-byte unicode by definition.
#ifndef UNIX
    // Optimization for windows platform where little endian maps directly to WCHAR.
    // (This increases overall parser performance by 5% for large unicode files !!)
    ::memcpy(bytebuffer, buffer, num * sizeof(TCHAR));
#else
    for (UINT i = num; i > 0; i--)
    {
        *bytebuffer++ = (*buffer) & 0xFF;
        *bytebuffer++ = (*buffer++) >> 8;
    }
#endif
    *cch = num;
    *cb = num << 1;
    return S_OK;
}


/**
 * Scans buffer and translates Unicode characters into Ucs4 big endian characters 
 */
HRESULT CharEncoder::wideCharToUcs4Bigendian(DWORD* pdwMode, CODEPAGE codepage, TCHAR * buffer, 
                                           UINT *cch, BYTE* bytebuffer, UINT * cb)
{
    UINT num = (*cb) >> 2; 
    if (num > *cch)
        num = *cch;

    for (UINT i = num; i > 0; i--)
    {
#ifndef UNIX
        *bytebuffer++ = 0;
        *bytebuffer++ = 0;
        *bytebuffer++ = (*buffer) >> 8;
        *bytebuffer++ = (*buffer) & 0xFF;
#else
        *bytebuffer++ = (*buffer) >> 24;
        *bytebuffer++ = ((*buffer) >> 16) & 0xFF;
        *bytebuffer++ = ((*buffer) >> 8) & 0xFF;
        *bytebuffer++ = (*buffer) & 0xFF;
#endif
        buffer++;
    }
    *cch = num;
    *cb = num << 2;
    return S_OK;
}


/**
 * Scans buffer and translates Unicode characters into Ucs4 little endian characters
 */
HRESULT CharEncoder::wideCharToUcs4Littleendian(DWORD* pdwMode, CODEPAGE codepage, TCHAR * buffer, 
                                              UINT *cch, BYTE* bytebuffer, UINT * cb)
{
    UINT num = (*cb) >> 2;
    if (num > *cch)
        num = *cch;

    for (UINT i = num; i > 0; i--)
    {
#ifndef UNIX
        *bytebuffer++ = (*buffer) & 0xFF;
        *bytebuffer++ = (*buffer) >> 8;
        *bytebuffer++ = 0;
        *bytebuffer++ = 0;
#else
        *bytebuffer++ = (*buffer) & 0xFF;
        *bytebuffer++ = ((*buffer) >> 8) & 0xFF;
        *bytebuffer++ = ((*buffer) >> 16) & 0xFF;
        *bytebuffer++ = (*buffer) >> 24;
#endif
        buffer++;
    }
    *cch = num;
    *cb = num << 2;
    return S_OK;
}


/**
 * Scans buffer and translates Unicode characters into UTF8 characters
 */
HRESULT CharEncoder::wideCharToUtf8(DWORD* pdwMode, CODEPAGE codepage, TCHAR * buffer, 
                                       UINT *cch, BYTE* bytebuffer, UINT * cb)
{
    UINT count = 0, num = *cch, m1 = *cb, m2 = m1 - 1, m3 = m2 - 1, m4 = m3 - 1;
    DWORD dw1;
    bool surrogate = false;

    for (UINT i = num; i > 0; i--)
    {
#ifdef UNIX
          // Solaris a WCHAR is 4 bytes (DWORD)
        DWORD dw = 0;
        DWORD dwTemp[4];
        BYTE* pByte = (BYTE*)buffer;
        dwTemp[3] = (DWORD)pByte[0];
        dwTemp[2] = (DWORD)pByte[1];
        dwTemp[1] = (DWORD)pByte[2];
        dwTemp[0] = (DWORD)pByte[3];
        dw = dwTemp[0]+(dwTemp[1]<<8)+(dwTemp[2]<<16)+(dwTemp[3]<<24);
#else
        DWORD dw = *buffer;
#endif

        if (surrogate) //  is it the second char of a surrogate pair?
        {
            if (dw >= 0xdc00 && dw <= 0xdfff)
            {
                // four bytes 0x11110xxx 0x10xxxxxx 0x10xxxxxx 0x10xxxxxx
                if (count < m4)
                    count += 4;
                else
                    break;
                ULONG ucs4 = (dw1 - 0xd800) * 0x400 + (dw - 0xdc00) + 0x10000;
                *bytebuffer++ = (byte)(( ucs4 >> 18) | 0xF0);
                *bytebuffer++ = (byte)((( ucs4 >> 12) & 0x3F) | 0x80);
                *bytebuffer++ = (byte)((( ucs4 >> 6) & 0x3F) | 0x80);
                *bytebuffer++ = (byte)(( ucs4 & 0x3F) | 0x80);
                surrogate = false;
                buffer++;
                continue;
            }
            else // Then dw1 must be a three byte character
            {
                if (count < m3)
                    count += 3;
                else
                    break;
                *bytebuffer++ = (byte)(( dw1 >> 12) | 0xE0);
                *bytebuffer++ = (byte)((( dw1 >> 6) & 0x3F) | 0x80);
                *bytebuffer++ = (byte)(( dw1 & 0x3F) | 0x80);
            }
            surrogate = false;
        }

        if (dw  < 0x80) // one byte, 0xxxxxxx
        {
            if (count < m1)
                count++;
            else
                break;
            *bytebuffer++ = (byte)dw;
        }
        else if ( dw < 0x800) // two WORDS, 110xxxxx 10xxxxxx
        {
            if (count < m2)
                count += 2;
            else
                break;
            *bytebuffer++ = (byte)((dw >> 6) | 0xC0);
            *bytebuffer++ = (byte)((dw & 0x3F) | 0x80);
        }
        else if (dw >= 0xd800 && dw <= 0xdbff) // Assume that it is the first char of surrogate pair
        {
            if (i == 1) // last wchar in buffer
                break;
            dw1 = dw;
            surrogate = true;
        }
        else // three bytes, 1110xxxx 10xxxxxx 10xxxxxx
        {
            if (count < m3)
                count += 3;
            else
                break;
            *bytebuffer++ = (byte)(( dw >> 12) | 0xE0);
            *bytebuffer++ = (byte)((( dw >> 6) & 0x3F) | 0x80);
            *bytebuffer++ = (byte)(( dw & 0x3F) | 0x80);
        }
        buffer++;
    }

    *cch = surrogate ? num - i - 1 : num - i;
    *cb = count;

    return S_OK;
}

/**
 * Scans buffer and translates Unicode characters into characters identified
 * by <code> codepage </>, using Win32 function WideCharToMultiByte for encoding 
 */
HRESULT CharEncoder::wideCharToMultiByteWin32(DWORD* pdwMode, CODEPAGE codepage, TCHAR * buffer, 
                                              UINT *cch, BYTE* bytebuffer, UINT * cb)
{
    HRESULT hr = S_OK;
    BOOL fBadChar = false;
    *cb = ::WideCharToMultiByte(codepage, NULL, buffer, *cch, (char*)bytebuffer, *cb, NULL, &fBadChar);
    if (*cb == 0)
        hr = ::GetLastError();
    else if (fBadChar)
        // BUGBUG: how do we inform the caller which character failed?
        hr = S_FALSE;
    return hr;
}


/**
 * Scans buffer and translates Unicode characters into characters of charSet 
 * identified by <code> codepage </code>, using Mlang for encoding 
 */
HRESULT CharEncoder::wideCharToMultiByteMlang(DWORD* pdwMode, CODEPAGE codepage, TCHAR * buffer, 
                                              UINT *cch, BYTE* bytebuffer, UINT * cb)
{
    HRESULT hr;
    checkhr2(_EnsureMultiLanguage());
    checkhr2(pMultiLanguage->ConvertStringFromUnicode(pdwMode, codepage,
                                       buffer, cch, (char*)bytebuffer, cb ));
    return S_OK;
}