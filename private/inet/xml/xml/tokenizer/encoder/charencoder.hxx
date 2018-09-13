/*
 * @(#)CharEncoder.hxx 1.0 6/10/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
#ifndef _CHARENCODER_HXX
#define _CHARENCODER_HXX

#include "codepage.h"
#include "mlang.h"

typedef HRESULT WideCharFromMultiByteFunc(DWORD* pdwMode, CODEPAGE codepage, BYTE * bytebuffer, 
                         UINT * cb, TCHAR * buffer, UINT * cch);

typedef HRESULT WideCharToMultiByteFunc(DWORD* pdwMode, CODEPAGE codepage, TCHAR * buffer, 
                         UINT *cch, BYTE * bytebuffer, UINT * cb);

struct EncodingEntry
{
    UINT codepage;
    TCHAR * charset;
    UINT  maxCharSize;
    WideCharFromMultiByteFunc * pfnWideCharFromMultiByte;
    WideCharToMultiByteFunc * pfnWideCharToMultiByte;
};

class Encoding
{
protected: 
    Encoding() {};

public:

    // default encoding is UTF-8.
    static Encoding* newEncoding(const TCHAR * s = _T("UTF-8"), ULONG len = 5, bool endian = false, bool mark = false);
    virtual ~Encoding();
    TCHAR * charset;        // charset 
    bool    littleendian;   // endian flag for UCS-2/UTF-16 encoding, true: little endian, false: big endian
    bool    byteOrderMark;  // byte order mark (BOM) flag, BOM appears when true
};

/**
 * 
 * An Encoder specifically for dealing with different encoding formats 
 * @version 1.0, 6/10/97
 */

class NOVTABLE CharEncoder
{
    //
    // class CharEncoder is a utility class, makes sure no instance can be defined
    //
    private: virtual charEncoder() = 0;

public:

    static HRESULT getWideCharFromMultiByteInfo(Encoding * encoding, CODEPAGE * pcodepage, WideCharFromMultiByteFunc ** pfnWideCharFromMultiByte, UINT * mCharSize);
    static HRESULT getWideCharToMultiByteInfo(Encoding * encoding, CODEPAGE * pcodepage, WideCharToMultiByteFunc ** pfnWideCharToMultiByte, UINT * mCharSize);

    /**
     * Encoding functions: get Unicode from other encodings
     */

    static WideCharFromMultiByteFunc wideCharFromUcs2Bigendian;
    static WideCharFromMultiByteFunc wideCharFromUcs2Littleendian;
    static WideCharFromMultiByteFunc wideCharFromUcs4Bigendian;
    static WideCharFromMultiByteFunc wideCharFromUcs4Littleendian;
    static WideCharFromMultiByteFunc wideCharFromUtf8;
    static WideCharFromMultiByteFunc wideCharFromUtf7;
    static WideCharFromMultiByteFunc wideCharFromAnsiLatin1;
    static WideCharFromMultiByteFunc wideCharFromMultiByteWin32;
    static WideCharFromMultiByteFunc wideCharFromMultiByteMlang;

    /**
     * Encoding functions: from Unicode to other encodings
     */

    static WideCharToMultiByteFunc wideCharToUcs2Bigendian;
    static WideCharToMultiByteFunc wideCharToUcs2Littleendian;
    static WideCharToMultiByteFunc wideCharToUcs4Bigendian;
    static WideCharToMultiByteFunc wideCharToUcs4Littleendian;
    static WideCharToMultiByteFunc wideCharToUtf8;
    static WideCharToMultiByteFunc wideCharToUtf7;
    static WideCharToMultiByteFunc wideCharToAnsiLatin1;
    static WideCharToMultiByteFunc wideCharToMultiByteWin32;
    static WideCharToMultiByteFunc wideCharToMultiByteMlang;

    static int getCharsetInfo(const TCHAR * charset, CODEPAGE * pcodepage, UINT * mCharSize);

private: 

    static HRESULT _EnsureMultiLanguage();

private:

    static IMultiLanguage * pMultiLanguage;
    static const EncodingEntry charsetInfo [];
};

#endif _CHARENCODER_HXX













