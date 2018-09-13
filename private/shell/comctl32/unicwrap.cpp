//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       unicwrap.cpp
//
//  Contents:   Wrappers for all Unicode functions used in MSHTML.
//              Any Unicode parameters/structure fields/buffers are converted
//              to ANSI, and then the corresponding ANSI version of the function
//              is called.
//
//----------------------------------------------------------------------------
#include "ctlspriv.h"

#ifdef  UNICODE
#ifndef WINNT

#include "unicwrap.h"

#undef TextOutW
#undef ExtTextOutW

int MbcsFromUnicode(LPSTR pstr, int cch, LPCWSTR pwstr, int cwch = -1);
int UnicodeFromMbcs(LPWSTR pwstr, int cwch, LPCSTR pstr, int cch = -1);

//+---------------------------------------------------------------------------
//
//  Member:     CConvertStr::Free
//
//  Synopsis:   Frees string if alloc'd and initializes to NULL.
//
//----------------------------------------------------------------------------

void CConvertStr::Free()
{
    if (_pstr != _ach && HIWORD64(_pstr) != 0)
    {
        delete [] _pstr;
    }

    _pstr = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConvertStrW::Free
//
//  Synopsis:   Frees string if alloc'd and initializes to NULL.
//
//----------------------------------------------------------------------------

void CConvertStrW::Free()
{
    if (_pwstr != _awch && HIWORD64(_pwstr) != 0)
    {
        delete [] _pwstr;
    }

    _pwstr = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrInW::Init
//
//  Synopsis:   Converts a LPSTR function argument to a LPWSTR.
//
//  Arguments:  [pstr] -- The function argument.  May be NULL or an atom
//                              (HIWORD(pwstr) == 0).
//
//              [cch]  -- The number of characters in the string to
//                          convert.  If -1, the string is assumed to be
//                          NULL terminated and its length is calculated.
//
//  Modifies:   [this]
//
//----------------------------------------------------------------------------

void CStrInW::Init(LPCSTR pstr, int cch)
{
    int cchBufReq;

    _cwchLen = 0;

    // Check if string is NULL or an atom.
    if (HIWORD64(pstr) == 0)
    {
        _pwstr = (LPWSTR) pstr;
        return;
    }

    ASSERT(cch == -1 || cch > 0);

    //
    // Convert string to preallocated buffer, and return if successful.
    //

    _cwchLen = MultiByteToWideChar(
            CP_ACP, 0, pstr, cch, _awch, ARRAYSIZE(_awch));

    if (_cwchLen > 0)
    {
        if(_awch[_cwchLen-1] == 0)
            _cwchLen--;                // account for terminator
        _pwstr = _awch;
        return;
    }

    //
    // Alloc space on heap for buffer.
    //

    cchBufReq = MultiByteToWideChar( CP_ACP, 0, pstr, cch, NULL, 0 );

    ASSERT(cchBufReq > 0);
    _pwstr = new WCHAR[cchBufReq];
    if (!_pwstr)
    {
        // On failure, the argument will point to the empty string.
        _awch[0] = 0;
        _pwstr = _awch;
        return;
    }

    ASSERT(HIWORD64(_pwstr));
    _cwchLen = -1 + MultiByteToWideChar( 
            CP_ACP, 0, pstr, cch, _pwstr, cchBufReq );
    ASSERT(_cwchLen >= 0);
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrIn::CStrIn
//
//  Synopsis:   Inits the class.
//
//  NOTE:       Don't inline this function or you'll increase code size
//              by pushing -1 on the stack for each call.
//
//----------------------------------------------------------------------------

CStrIn::CStrIn(LPCWSTR pwstr) : CConvertStr(CP_ACP)
{
    Init(pwstr, -1);
}

CStrIn::CStrIn(UINT uCP, LPCWSTR pwstr) : CConvertStr(uCP)
{
    Init(pwstr, -1);
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrIn::Init
//
//  Synopsis:   Converts a LPWSTR function argument to a LPSTR.
//
//  Arguments:  [pwstr] -- The function argument.  May be NULL or an atom
//                              (HIWORD(pwstr) == 0).
//
//              [cwch]  -- The number of characters in the string to
//                          convert.  If -1, the string is assumed to be
//                          NULL terminated and its length is calculated.
//
//  Modifies:   [this]
//
//----------------------------------------------------------------------------

void CStrIn::Init(LPCWSTR pwstr, int cwch)
{
    int cchBufReq;

#if DBG == 1 /* { */
    int errcode;
#endif /* } */

    _cchLen = 0;

    // Check if string is NULL or an atom.
    if (HIWORD64(pwstr) == 0)
    {
        _pstr = (LPSTR) pwstr;
        return;
    }

    if ( cwch == 0 )
    {
        *_ach = '\0';
        _pstr = _ach;
        return;
    }

    ASSERT(cwch == -1 || cwch > 0);
    //
    // Convert string to preallocated buffer, and return if successful.
    //

    _cchLen = WideCharToMultiByte(
            _uCP, 0, pwstr, cwch, _ach, ARRAYSIZE(_ach), NULL, NULL);
    if (_cchLen > 0)
    {
        if (_ach[_cchLen-1]==0) _cchLen--;          // account for terminator
        _pstr = _ach;
        return;
    }


    cchBufReq = WideCharToMultiByte(
            CP_ACP, 0, pwstr, cwch, NULL, 0, NULL, NULL);

    ASSERT(cchBufReq > 0);
    _pstr = new char[cchBufReq];
    if (!_pstr)
    {
        // On failure, the argument will point to the empty string.
        _ach[0] = 0;
        _pstr = _ach;
        return;
    }

    ASSERT(HIWORD64(_pstr));
    _cchLen = -1 + WideCharToMultiByte(
            _uCP, 0, pwstr, cwch, _pstr, cchBufReq, NULL, NULL);

#if DBG == 1 /* { */
    if (_cchLen < 0)
    {
        errcode = GetLastError();
        ASSERT(0 && "WideCharToMultiByte failed in unicode wrapper.");
    }
#endif /* } */
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrInMulti::CStrInMulti
//
//  Synopsis:   Converts mulitple LPWSTRs to a multiple LPSTRs.
//
//  Arguments:  [pwstr] -- The strings to convert.
//
//  Modifies:   [this]
//
//----------------------------------------------------------------------------

CStrInMulti::CStrInMulti(LPCWSTR pwstr)
{
    LPCWSTR pwstrT;

    // We don't handle atoms because we don't need to.
    ASSERT(HIWORD64(pwstr));

    //
    // Count number of characters to convert.
    //

    pwstrT = pwstr;
    if (pwstr)
    {
        do {
            while (*pwstrT++)
                ;

        } while (*pwstrT++);
    }

    Init(pwstr, (int)(pwstrT - pwstr));
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrOut::CStrOut
//
//  Synopsis:   Allocates enough space for an out buffer.
//
//  Arguments:  [pwstr]   -- The Unicode buffer to convert to when destroyed.
//                              May be NULL.
//
//              [cwchBuf] -- The size of the buffer in characters.
//
//  Modifies:   [this].
//
//----------------------------------------------------------------------------

CStrOut::CStrOut(LPWSTR pwstr, int cwchBuf) : CConvertStr(CP_ACP)
{
    ASSERT(cwchBuf >= 0);

    if (!cwchBuf)
        pwstr = NULL;

    _pwstr = pwstr;
    _cwchBuf = cwchBuf;

    if (!pwstr)
    {
        ASSERT(cwchBuf == 0);
        _pstr = NULL;
        return;
    }

    ASSERT(HIWORD64(pwstr));

    // Initialize buffer in case Windows API returns an error.
    _ach[0] = 0;

    // Use preallocated buffer if big enough.
    if (cwchBuf * 2 <= ARRAYSIZE(_ach))
    {
        _pstr = _ach;
        return;
    }

    // Allocate buffer.
    _pstr = new char[cwchBuf * 2];
    if (!_pstr)
    {
        //
        // On failure, the argument will point to a zero-sized buffer initialized
        // to the empty string.  This should cause the Windows API to fail.
        //

        ASSERT(cwchBuf > 0);
        _pwstr[0] = 0;
        _cwchBuf = 0;
        _pstr = _ach;
        return;
    }

    ASSERT(HIWORD64(_pstr));
    _pstr[0] = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrOut::ConvertIncludingNul
//
//  Synopsis:   Converts the buffer from MBCS to Unicode
//
//  Return:     Character count INCLUDING the trailing '\0'
//
//----------------------------------------------------------------------------

int CStrOut::ConvertIncludingNul()
{
    int cwch;

    if (!_pstr)
        return 0;

    ASSERT(_cwchBuf);

    // Preinit to null string in case of horrible catastrophe
    _pwstr[0] = TEXT('\0');

    cwch = MultiByteToWideChar(_uCP, 0, _pstr, -1, _pwstr, _cwchBuf);

    if (!cwch) {
        // Output buffer was short.  Must double-buffer (yuck)
        int cwchNeeded = MultiByteToWideChar(_uCP, 0, _pstr, -1, NULL, 0);
        if (cwchNeeded) {
            LPWSTR pwsz = (LPWSTR)LocalAlloc(LMEM_FIXED,
                                             cwchNeeded * SIZEOF(WCHAR));
            if (pwsz) {
                cwch = MultiByteToWideChar(_uCP, 0, _pstr, -1,
                                           pwsz, cwchNeeded);
                if (cwch) {
                    StrCpyNW(_pwstr, pwsz, _cwchBuf);
                    cwch = _cwchBuf;
                }
                LocalFree(pwsz);
            }
        } else {
#if DBG == 1 /* { */
            DWORD errcode = GetLastError();
            ASSERT(0 && "MultiByteToWideChar failed in unicode wrapper.");
#endif /* } */
        }
    }

    Free();
    return cwch;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrOut::ConvertExcludingNul
//
//  Synopsis:   Converts the buffer from MBCS to Unicode
//
//  Return:     Character count EXCLUDING the trailing '\0'
//
//----------------------------------------------------------------------------

int CStrOut::ConvertExcludingNul()
{
    int ret = ConvertIncludingNul();
    if (ret)
    {
        ret -= 1;
    }
    return ret;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrOut::~CStrOut
//
//  Synopsis:   Converts the buffer from MBCS to Unicode.
//
//  Note:       Don't inline this function, or you'll increase code size as
//              both ConvertIncludingNul() and CConvertStr::~CConvertStr will be
//              called inline.
//
//----------------------------------------------------------------------------

CStrOut::~CStrOut()
{
    ConvertIncludingNul();
}

//+---------------------------------------------------------------------------
//
//  Function:   MbcsFromUnicode
//
//  Synopsis:   Converts a string to MBCS from Unicode.
//
//  Arguments:  [pstr]  -- The buffer for the MBCS string.
//              [cch]   -- The size of the MBCS buffer, including space for
//                              NULL terminator.
//
//              [pwstr] -- The Unicode string to convert.
//              [cwch]  -- The number of characters in the Unicode string to
//                              convert, including NULL terminator.  If this
//                              number is -1, the string is assumed to be
//                              NULL terminated.  -1 is supplied as a
//                              default argument.
//
//  Returns:    If [pstr] is NULL or [cch] is 0, 0 is returned.  Otherwise,
//              the number of characters converted, including the terminating
//              NULL, is returned (note that converting the empty string will
//              return 1).  If the conversion fails, 0 is returned.
//
//  Modifies:   [pstr].
//
//----------------------------------------------------------------------------

int MbcsFromUnicode(LPSTR pstr, int cch, LPCWSTR pwstr, int cwch)
{
    int ret;

#if DBG == 1 /* { */
    int errcode;
#endif /* } */

    ASSERT(cch >= 0);
    if (!pstr || cch == 0)
        return 0;

    ASSERT(pwstr);
    ASSERT(cwch == -1 || cwch > 0);

    ret = WideCharToMultiByte(CP_ACP, 0, pwstr, cwch, pstr, cch, NULL, NULL);

#if DBG == 1 /* { */
    if (ret <= 0)
    {
        errcode = GetLastError();
        ASSERT(0 && "WideCharToMultiByte failed in unicode wrapper.");
    }
#endif /* } */

    return ret;
}

//+---------------------------------------------------------------------------
//
//  Function:   UnicodeFromMbcs
//
//  Synopsis:   Converts a string to Unicode from MBCS.
//
//  Arguments:  [pwstr] -- The buffer for the Unicode string.
//              [cwch]  -- The size of the Unicode buffer, including space for
//                              NULL terminator.
//
//              [pstr]  -- The MBCS string to convert.
//              [cch]  -- The number of characters in the MBCS string to
//                              convert, including NULL terminator.  If this
//                              number is -1, the string is assumed to be
//                              NULL terminated.  -1 is supplied as a
//                              default argument.
//
//  Returns:    If [pwstr] is NULL or [cwch] is 0, 0 is returned.  Otherwise,
//              the number of characters converted, including the terminating
//              NULL, is returned (note that converting the empty string will
//              return 1).  If the conversion fails, 0 is returned.
//
//  Modifies:   [pwstr].
//
//----------------------------------------------------------------------------

int UnicodeFromMbcs(LPWSTR pwstr, int cwch, LPCSTR pstr, int cch)
{
    int ret;

#if DBG == 1 /* { */
    int errcode;
#endif /* } */

    ASSERT(cwch >= 0);

    if (!pwstr || cwch == 0)
        return 0;

    ASSERT(pstr);
    ASSERT(cch == -1 || cch > 0);

    ret = MultiByteToWideChar(CP_ACP, 0, pstr, cch, pwstr, cwch);

#if DBG == 1 /* { */
    if (ret <= 0)
    {
        errcode = GetLastError();
        ASSERT(0 && "MultiByteToWideChar failed in unicode wrapper.");
    }
#endif /* } */

    return ret;
}

//+------------------------------------------------------------------------
//
//  Contents:   widechar character type function (CT_CTYPE1) and (CT_CTYPE3)
//
//  Synopsis:   We do not have wide char support for IsChar functions
//              under Win95.  The Unicode-Wrapper functions we have
//              in core\wrappers all convert to CP_ACP and then call
//              the A version, which means we will have invalid results
//              for any characters which aren't in CP_ACP.
//
//              The solution is to roll our own, which result in these
//              unfortunately large tables.  Here's how it works:
//
//              bits:   fedc ba98 7654 3210
//                      pppp pppp iiib bbbb
//
//              The 'b' bits form a 32-bit bit mask into our data.  The data
//              entrys boolean, and are thus 4-bytes long.  Of the 2^32
//              possible combinations, we in fact have only 218 distinct
//              values of data.  These are stored in adwData.
//
//              The 'p' bits represent a page.  Each page has eight
//              possible entries, represent by 'i'.  In most pages, the
//              bitfields and data are both uniform.
//
//              adwData[abIndex[abType[page]][index]] represents the data
//
//              1 << bits represents the bitmask.
//
//-------------------------------------------------------------------------

#define __BIT_SHIFT 0
#define __INDEX_SHIFT 5
#define __PAGE_SHIFT 8

#define __BIT_MASK 31
#define __INDEX_MASK 7

// straight lookup functions are inlined.

#define ISCHARFUNC(type, wch) \
    (adwData[abIndex[abType1##type[wch>>__PAGE_SHIFT]] \
                          [(wch>>__INDEX_SHIFT)&__INDEX_MASK]] \
            >> (wch&__BIT_MASK)) & 1 
    
//
// To avoid header file conflicts with IsCharAlphaW, IsCharAlphaNumericW, ... defined in
// winuser.h, the functions names end in "Wrap".  SHLWAPI.DEF exports these functions with 
// the correct name.
//

STDAPI_(BOOL) IsCharAlphaWrap(WCHAR wch);
STDAPI_(BOOL) IsCharAlphaNumericWrap(WCHAR wch);
STDAPI_(BOOL) IsCharUpperWrap(WCHAR wch);
STDAPI_(BOOL) IsCharLowerWrap(WCHAR wch);

const DWORD adwData[218] =
{
    0x00000000, 0x07fffffe, 0xff7fffff, 0xffffffff,  // 0x00-0x03
    0xfc3fffff, 0x00ffffff, 0xffff0000, 0x000001ff,  // 0x04-0x07
    0xffffd740, 0xfffffffb, 0x547f7fff, 0x000ffffd,  // 0x08-0x0b
    0xffffdffe, 0xdffeffff, 0xffff0003, 0xffff199f,  // 0x0c-0x0f
    0x033fcfff, 0xfffe0000, 0x007fffff, 0xfffffffe,  // 0x10-0x13
    0x000000ff, 0x000707ff, 0x000007fe, 0x7cffffff,  // 0x14-0x17
    0x002f7fff, 0xffffffe0, 0x03ffffff, 0xff000000,  // 0x18-0x1b
    0x00000003, 0xfff99fe0, 0x03c5fdff, 0xb0000000,  // 0x1c-0x1f
    0x00030003, 0xfff987e0, 0x036dfdff, 0x5e000000,  // 0x20-0x23
    0xfffbafe0, 0x03edfdff, 0x00000001, 0x03cdfdff,  // 0x24-0x27
    0xd63dc7e0, 0x03bfc718, 0xfffddfe0, 0x03effdff,  // 0x28-0x2b
    0x40000000, 0x03fffdff, 0x000d7fff, 0x0000003f,  // 0x2c-0x2f
    0xfef02596, 0x00006cae, 0x30000000, 0xffff003f,  // 0x30-0x33
    0x83ffffff, 0xffffff07, 0x07ffffff, 0x3f3fffff,  // 0x34-0x37
    0xaaff3f3f, 0x3fffffff, 0x1fdfffff, 0x0fcf1fdc,  // 0x38-0x3b
    0x1fdc1fff, 0xf0000000, 0x000003ff, 0x00000020,  // 0x3c-0x3f
    0x781fffff, 0x77ffffff, 0xfffe1fff, 0x00007fff,  // 0x40-0x43
    0x0000000f, 0x00003fff, 0x80f8007f, 0x5f7fffff,  // 0x44-0x47
    0xffffffdb, 0x0003ffff, 0xfff80000, 0xfffffdff,  // 0x48-0x4b
    0xfffffffd, 0xfffcffff, 0x0fff0000, 0x1fffffff,  // 0x4c-0x4f
    0xffffffc0, 0x7ffffffe, 0x1cfcfcfc, 0x00003e00,  // 0x50-0x53
    0x00000fff, 0x80000000, 0xfc00fffe, 0xf8000001,  // 0x54-0x57
    0x78000001, 0x00800000, 0x00040000, 0x7fffffff,  // 0x58-0x5b
    0x44300003, 0x000000b0, 0x0000007c, 0xfe000000,  // 0x5c-0x5f
    0x00000200, 0x00180000, 0x88001000, 0x0007f801,  // 0x60-0x63
    0x00013c00, 0xffd00000, 0x0000000e, 0x001f3fff,  // 0x64-0x67
    0x0001003c, 0xd0000000, 0x0080399f, 0x07fc000c,  // 0x68-0x6b
    0x00000004, 0x00003987, 0x001f0000, 0x00013bbf,  // 0x6c-0x6f
    0x00c0398f, 0x00010000, 0x0000000c, 0xc0000000,  // 0x70-0x73
    0x00803dc7, 0x00603ddf, 0x00803dcf, 0x87f28000,  // 0x74-0x77
    0x0c00ffc0, 0x3bff8000, 0x00003f5f, 0x08000000,  // 0x78-0x7b
    0xe0000000, 0xe000e003, 0x6000e000, 0xffff7fff,  // 0x7c-0x7f
    0x0000007f, 0xfc00fc00, 0x00007c00, 0x01ffffff,  // 0x80-0x83
    0xffff0007, 0x000007ff, 0x0000001f, 0x003fffff,  // 0x84-0x87
    0xffffdfff, 0x0000ffff, 0xfc0fffff, 0xfffff3de,  // 0x88-0x8b
    0xfffffeff, 0x7f47afff, 0xffc000fe, 0xff1fffff,  // 0x8c-0x8f
    0x7ffeffff, 0x80ffffff, 0x7e000000, 0x78000000,  // 0x90-0x93
    0x8fffffff, 0x0001ffff, 0xffff0fff, 0xf87fffff,  // 0x94-0x97
    0xffff000f, 0xfff7fe1f, 0xffd70f7f, 0x0001003e,  // 0x98-0x9b
    0x00007f7f, 0x03ff0000, 0x020c0000, 0x0000ffc0,  // 0x9c-0x9f
    0x0007ff80, 0x03f10000, 0x0000007e, 0x7f7fffff,  // 0xa0-0xa3
    0x55555555, 0xaa555555, 0x555554aa, 0x2b555555,  // 0xa4-0xa7
    0xb1dbced6, 0x11aed295, 0x4aaaadb0, 0x54165555,  // 0xa8-0xab
    0x00555555, 0xfffed740, 0x00000ffb, 0x541c0000,  // 0xac-0xaf
    0x00005555, 0x55550001, 0x5555088a, 0x01154555,  // 0xb0-0xb3
    0x00155555, 0x01555555, 0x3f00ff00, 0xff00ff00,  // 0xb4-0xb7
    0xaa003f00, 0x0000ff00, 0x1f00ff00, 0x0f001f00,  // 0xb8-0xbb
    0x1f001f00, 0xffc00000, 0xaaaaaaaa, 0x55aaaaaa,  // 0xbc-0xbf
    0xaaaaab55, 0xd4aaaaaa, 0x4e243129, 0x2651292a,  // 0xc0-0xc3
    0xb5555b60, 0xa82daaaa, 0x00aaaaaa, 0xffaffbfb,  // 0xc4-0xc7
    0x640f7ffc, 0x000001f9, 0xfffff000, 0x00637fff,  // 0xc8-0xcb
    0x000faaa8, 0xaaaa0002, 0xaaaa1114, 0x022a8aaa,  // 0xcc-0xcf
    0x07eaaaaa, 0x02aaaaaa, 0x003f00ff, 0x00ff00ff,  // 0xd0-0xd3
    0x00ff003f, 0x3fff00ff, 0x00df00ff, 0x00cf00dc,  // 0xd4-0xd7
    0x00dc00ff, 0x00f8007f
};

const BYTE abIndex[98][8] =
{
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 0x00
    { 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x02, 0x02 }, // 0x01
    { 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x04 }, // 0x02
    { 0x05, 0x00, 0x06, 0x03, 0x03, 0x07, 0x00, 0x00 }, // 0x03
    { 0x00, 0x00, 0x00, 0x00, 0x08, 0x09, 0x0a, 0x0b }, // 0x04
    { 0x0c, 0x03, 0x0d, 0x03, 0x0e, 0x03, 0x0f, 0x10 }, // 0x05
    { 0x00, 0x11, 0x12, 0x13, 0x14, 0x00, 0x06, 0x15 }, // 0x06
    { 0x00, 0x01, 0x16, 0x11, 0x03, 0x17, 0x18, 0x00 }, // 0x07
    { 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20 }, // 0x08
    { 0x21, 0x22, 0x23, 0x00, 0x24, 0x25, 0x00, 0x26 }, // 0x09
    { 0x1d, 0x27, 0x1f, 0x1c, 0x28, 0x29, 0x00, 0x00 }, // 0x0a
    { 0x2a, 0x2b, 0x00, 0x1c, 0x2a, 0x2b, 0x2c, 0x1c }, // 0x0b
    { 0x2a, 0x2d, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00 }, // 0x0c
    { 0x13, 0x2e, 0x2f, 0x00, 0x30, 0x31, 0x32, 0x00 }, // 0x0d
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x33, 0x12 }, // 0x0e
    { 0x03, 0x03, 0x34, 0x03, 0x03, 0x35, 0x03, 0x1a }, // 0x0f
    { 0x03, 0x03, 0x03, 0x03, 0x36, 0x03, 0x03, 0x1a }, // 0x10
    { 0x37, 0x03, 0x38, 0x39, 0x03, 0x3a, 0x3b, 0x3c }, // 0x11
    { 0x00, 0x00, 0x00, 0x00, 0x3d, 0x03, 0x03, 0x3e }, // 0x12
    { 0x3f, 0x00, 0x13, 0x03, 0x40, 0x13, 0x03, 0x41 }, // 0x13
    { 0x19, 0x42, 0x03, 0x03, 0x43, 0x00, 0x00, 0x00 }, // 0x14
    { 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03 }, // 0x15
    { 0x03, 0x03, 0x03, 0x03, 0x03, 0x2f, 0x00, 0x00 }, // 0x16
    { 0x03, 0x03, 0x03, 0x03, 0x03, 0x44, 0x00, 0x00 }, // 0x17
    { 0x03, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 0x18
    { 0x46, 0x47, 0x48, 0x03, 0x03, 0x49, 0x4a, 0x4b }, // 0x19
    { 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x4c }, // 0x1a
    { 0x03, 0x39, 0x06, 0x03, 0x4d, 0x03, 0x14, 0x4e }, // 0x1b
    { 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x4f }, // 0x1c
    { 0x00, 0x01, 0x01, 0x50, 0x03, 0x51, 0x52, 0x00 }, // 0x1d
    { 0x53, 0x26, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00 }, // 0x1e
    { 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 0x1f
    { 0x26, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 0x20
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55 }, // 0x21
    { 0x00, 0x56, 0x57, 0x58, 0x00, 0x13, 0x59, 0x59 }, // 0x22
    { 0x00, 0x00, 0x00, 0x00, 0x5a, 0x00, 0x00, 0x00 }, // 0x23
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x5b, 0x3e }, // 0x24
    { 0x03, 0x03, 0x2f, 0x5c, 0x5d, 0x00, 0x00, 0x00 }, // 0x25
    { 0x00, 0x00, 0x00, 0x00, 0x5e, 0x00, 0x00, 0x00 }, // 0x26
    { 0x00, 0x00, 0x5f, 0x00, 0x60, 0x06, 0x44, 0x61 }, // 0x27
    { 0x62, 0x00, 0x63, 0x64, 0x00, 0x00, 0x65, 0x45 }, // 0x28
    { 0x66, 0x3d, 0x67, 0x68, 0x66, 0x69, 0x6a, 0x6b }, // 0x29
    { 0x6c, 0x69, 0x6d, 0x6e, 0x66, 0x3d, 0x6f, 0x00 }, // 0x2a
    { 0x66, 0x3d, 0x70, 0x71, 0x72, 0x73, 0x74, 0x00 }, // 0x2b
    { 0x66, 0x73, 0x75, 0x00, 0x72, 0x73, 0x75, 0x00 }, // 0x2c
    { 0x72, 0x73, 0x76, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 0x2d
    { 0x00, 0x77, 0x78, 0x00, 0x00, 0x79, 0x7a, 0x00 }, // 0x2e
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7b }, // 0x2f
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x7d, 0x7e }, // 0x30
    { 0x03, 0x7f, 0x80, 0x81, 0x82, 0x54, 0x06, 0x1c }, // 0x31
    { 0x03, 0x83, 0x4a, 0x03, 0x84, 0x03, 0x03, 0x85 }, // 0x32
    { 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x49 }, // 0x33
    { 0x4c, 0x03, 0x03, 0x36, 0x00, 0x00, 0x00, 0x00 }, // 0x34
    { 0x03, 0x86, 0x85, 0x03, 0x03, 0x03, 0x03, 0x85 }, // 0x35
    { 0x03, 0x03, 0x03, 0x03, 0x87, 0x88, 0x03, 0x89 }, // 0x36
    { 0x8a, 0x03, 0x03, 0x89, 0x00, 0x00, 0x00, 0x00 }, // 0x37
    { 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x00, 0x00 }, // 0x38
    { 0x13, 0x91, 0x00, 0x00, 0x92, 0x00, 0x00, 0x93 }, // 0x39
    { 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00 }, // 0x3a
    { 0x4f, 0x03, 0x44, 0x94, 0x03, 0x95, 0x96, 0x5b }, // 0x3b
    { 0x03, 0x03, 0x03, 0x97, 0x03, 0x03, 0x39, 0x5b }, // 0x3c
    { 0x2c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 0x3d
    { 0x00, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 0x3e
    { 0x00, 0x98, 0x99, 0x9a, 0x03, 0x03, 0x03, 0x4f }, // 0x3f
    { 0x56, 0x57, 0x58, 0x9b, 0x73, 0x26, 0x00, 0x9c }, // 0x40
    { 0x00, 0x9d, 0x00, 0x00, 0x00, 0x9e, 0x00, 0x00 }, // 0x41
    { 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x9d }, // 0x42
    { 0x00, 0x00, 0x00, 0x9f, 0x00, 0x00, 0x00, 0x9f }, // 0x43
    { 0x00, 0x00, 0x00, 0x9f, 0x00, 0x00, 0x00, 0xa0 }, // 0x44
    { 0x00, 0x00, 0x00, 0x9f, 0x00, 0x00, 0x00, 0x00 }, // 0x45
    { 0x00, 0x00, 0x9d, 0x00, 0x00, 0x00, 0x9d, 0x00 }, // 0x46
    { 0x00, 0x00, 0x00, 0xa1, 0x3e, 0x00, 0x00, 0x00 }, // 0x47
    { 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 0x48
    { 0x00, 0x9d, 0xa2, 0xa2, 0x00, 0x00, 0x00, 0x00 }, // 0x49
    { 0x9d, 0xa2, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 0x4a
    { 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xa3, 0x00 }, // 0x4b
    { 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab }, // 0x4c
    { 0xac, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 0x4d
    { 0x00, 0x00, 0x00, 0x00, 0xad, 0xae, 0xaf, 0xb0 }, // 0x4e
    { 0x0c, 0x89, 0x00, 0xa4, 0xb1, 0xa4, 0xb2, 0xb3 }, // 0x4f
    { 0x00, 0x11, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 0x50
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x2f, 0x00 }, // 0x51
    { 0xa4, 0xa4, 0xa4, 0xa4, 0xb4, 0xa4, 0xa4, 0xb5 }, // 0x52
    { 0xb6, 0xb7, 0xb8, 0xb9, 0xb7, 0xba, 0xbb, 0xbc }, // 0x53
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0xbd, 0x89, 0x00 }, // 0x54
    { 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 0x55
    { 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x55, 0x02 }, // 0x56
    { 0xbe, 0xbf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5 }, // 0x57
    { 0xc6, 0x00, 0x06, 0xc7, 0xc8, 0xc9, 0x00, 0x00 }, // 0x58
    { 0x00, 0x00, 0x00, 0x00, 0x71, 0xca, 0xcb, 0xcc }, // 0x59
    { 0x00, 0x06, 0x0d, 0xbe, 0xcd, 0xbe, 0xce, 0xcf }, // 0x5a
    { 0x00, 0x00, 0x00, 0x13, 0x14, 0x00, 0x00, 0x00 }, // 0x5b
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x12 }, // 0x5c
    { 0xbe, 0xbe, 0xbe, 0xbe, 0xd0, 0xbe, 0xbe, 0xd1 }, // 0x5d
    { 0xd2, 0xd3, 0xd4, 0xd5, 0xd3, 0xd6, 0xd7, 0xd8 }, // 0x5e
    { 0x00, 0x00, 0x00, 0x00, 0x3d, 0x87, 0x06, 0x3e }, // 0x5f
    { 0xd9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 0x60
    { 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 }  // 0x61
};

const BYTE abType1Alpha[256] = // 154
{
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x00,
    0x00, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x00,
    0x0e, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x11,
    0x00, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x13, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0x15,
    0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15,
    0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15,
    0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15,
    0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15,
    0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15,
    0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15,
    0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15,
    0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15,
    0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15,
    0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x16,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x15, 0x15, 0x15, 0x15,
    0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15,
    0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15,
    0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15,
    0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15,
    0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x17,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x15, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d
};

BOOL IsCharSpaceW(WCHAR wch)
{
    int nType;

    switch(wch>>8)
    {
        case 0x00: nType = 0x1e; break;
        case 0x20: nType = 0x1f; break;
        case 0x30: nType = 0x20; break;
        case 0xfe: nType = 0x21; break;
        default:   nType = 0x00; break;
    }

    return (adwData[abIndex[nType][(wch>>__INDEX_SHIFT)&__INDEX_MASK]]
            >>(wch&__BIT_MASK)) & 1;
}

const BYTE abType1Punct[256] = // 32
{
    0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x00,
    0x00, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x00,
    0x2f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30,
    0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x39, 0x3a, 0x3b, 0x3c, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x3d, 0x00, 0x3e, 0x3f, 0x40
};

const BYTE abType1Digit[256] = // 11
{
    0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x00,
    0x00, 0x43, 0x43, 0x44, 0x43, 0x45, 0x46, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x47, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48
};

BOOL IsCharDigitW(WCHAR wch) { return ISCHARFUNC(Digit, wch); }

BOOL IsCharXDigitW(WCHAR wch)
{
    int nType;

    switch(wch>>8)
    {
        case 0x00: nType = 0x49; break;
        case 0xff: nType = 0x4a; break;
        default:   nType = 0x00; break;
    }

    return (adwData[abIndex[nType][(wch>>__INDEX_SHIFT)&__INDEX_MASK]]
            >> (wch&__BIT_MASK)) & 1;
}

const BYTE abType1Upper[256] = // 12
{
    0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x51, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x52, 0x53,
    0x00, 0x00, 0x00, 0x00, 0x54, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55
};

const BYTE abType1Lower[256] = // 13
{
    0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x5c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5d, 0x5e,
    0x00, 0x00, 0x00, 0x00, 0x5f, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x61
};

BOOL IsCharPunctW(WCHAR wch) { return ISCHARFUNC(Punct, wch); }

BOOL IsCharCntrlW(WCHAR wch)
{
    return    (unsigned)(wch - 0x0000) <= (0x001f - 0x0000)
           || (unsigned)(wch - 0x007f) <= (0x009f - 0x007f);
}

// NB (cthrash) WCH_NBSP is considered blank, for compatibility.

BOOL IsCharBlankW(WCHAR wch)
{
    return    wch == 0x0009
           || wch == 0x0020
           || wch == 0x00a0
           || wch == 0x3000
           || wch == 0xfeff;
}

BOOL IsCharAlphaWrap(WCHAR wch) { return ISCHARFUNC(Alpha, wch); }
BOOL IsCharUpperWrap(WCHAR wch) { return ISCHARFUNC(Upper, wch); }
BOOL IsCharLowerWrap(WCHAR wch) { return ISCHARFUNC(Lower, wch); }

BOOL IsCharAlphaNumericWrap(WCHAR wch)
{
    return ISCHARFUNC(Alpha, wch) || ISCHARFUNC(Digit, wch);
}

static const BYTE abType3PageSub[256] = 
{
    0x00, 0x80, 0x81, 0x82, 0x00, 0x83, 0x84, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x85, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86, 
    0x00, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x8e, 0x8f, 0x90, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x91, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x20, 0x92, 0x00, 0x00, 0x93, 0x94, 0x00
};

static const BYTE abType3Page0[256] = 
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x08, 
    0x09, 0x09, 0x09, 0x09, 0x09, 0x08, 0x09, 0x09, 
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 
    0x08, 0x08, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 
    0x09, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 
    0x08, 0x08, 0x08, 0x09, 0x09, 0x09, 0x09, 0x09, 
    0x09, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 
    0x08, 0x08, 0x08, 0x09, 0x09, 0x09, 0x09, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x01, 0x01, 0x09, 0x09, 0x01, 0x09, 0x09, 0x01, 
    0x01, 0x01, 0x00, 0x01, 0x09, 0x01, 0x01, 0x09, 
    0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 
    0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const BYTE abType3Page32[256] = 
{
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
    0x11, 0x11, 0x01, 0x01, 0x11, 0x11, 0x01, 0x01, 
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 
    0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x01, 0x00, 0x01, 0x01, 0x01, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x01, 0x00, 0x01, 0x01, 0x01, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x01, 0x09, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const BYTE abType3Page48[256] = 
{
    0x11, 0x11, 0x11, 0x00, 0x00, 0x20, 0x01, 0x01, 
    0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x01, 0x01, 
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x01, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x06, 0x06, 0x06, 0x06, 0x06, 0x01, 0x01, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 
    0x00, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
    0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00, 
    0x00, 0x06, 0x06, 0x16, 0x16, 0x04, 0x04, 0x00, 
    0x00, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 
    0x12, 0x12, 0x12, 0x12, 0x02, 0x12, 0x02, 0x12, 
    0x02, 0x12, 0x02, 0x12, 0x02, 0x12, 0x02, 0x12, 
    0x02, 0x12, 0x02, 0x12, 0x02, 0x12, 0x02, 0x12, 
    0x02, 0x12, 0x02, 0x12, 0x12, 0x02, 0x12, 0x02, 
    0x12, 0x02, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 
    0x02, 0x02, 0x12, 0x02, 0x02, 0x12, 0x02, 0x02, 
    0x12, 0x02, 0x02, 0x12, 0x02, 0x02, 0x12, 0x12, 
    0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 
    0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x02, 0x12, 
    0x02, 0x02, 0x12, 0x12, 0x02, 0x02, 0x02, 0x02, 
    0x02, 0x02, 0x02, 0x13, 0x06, 0x02, 0x02, 0x00
};

static const BYTE abType3Page255[256] = 
{
    0x00, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x10, 
    0x11, 0x11, 0x11, 0x11, 0x11, 0x10, 0x11, 0x11, 
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
    0x10, 0x10, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 
    0x11, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
    0x10, 0x10, 0x10, 0x11, 0x11, 0x11, 0x11, 0x11, 
    0x11, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
    0x10, 0x10, 0x10, 0x11, 0x11, 0x11, 0x11, 0x00, 
    0x00, 0x09, 0x09, 0x09, 0x09, 0x09, 0x0a, 0x0a, 
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 
    0x0e, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 
    0x00, 0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 
    0x00, 0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 
    0x00, 0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 
    0x00, 0x00, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x00, 
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

struct tagType3DualValue
{
    DWORD   adwBitfield[8];
    DWORD   adwValue[2];
}

const aType3DualValue[21] =
{
    { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,   // Page1
      0x00000000, 0x0000000f, 0x00000000, 0x00000000, 0x00000001 },
    { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,   // Page2
      0x00000000, 0x3f000000, 0x00000000, 0x00000000, 0x00000001 },
    { 0x00000000, 0x00000000, 0x00000000, 0x04000000, 0x000000b0,   // Page3
      0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001 },
    { 0x00000000, 0x00000000, 0xf8000000, 0x00000000, 0x00000200,   // Page5
      0x40000000, 0x00000009, 0x00180000, 0x00000000, 0x00000001 },
    { 0x88001000, 0x00000000, 0x00000000, 0x00003c00, 0x00000000,   // Page6
      0x00000000, 0x00100000, 0x00000200, 0x00000000, 0x00000001 },
    { 0x00000000, 0x80008000, 0x0c008040, 0x00000000, 0x00000000,   // Page14
      0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001 },
    { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,   // Page31
      0xe0000000, 0xe000e003, 0x6000e000, 0x00000000, 0x00000001 },
    { 0x00800000, 0x00000000, 0x00000000, 0x00000000, 0xffff0000,   // Page33
      0xffffffff, 0xffffffff, 0x000007ff, 0x00000000, 0x00000001 },
    { 0x40000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,   // Page34
      0x00000000, 0x00000000, 0xfffc0000, 0x00000001, 0x00000000 },
    { 0x00000002, 0x00000000, 0x00000000, 0xf8000000, 0xffffffff,   // Page35
      0xffffffff, 0xffffffff, 0xffffffff, 0x00000001, 0x00000000 },
    { 0x00000000, 0xffffffe0, 0xfffff800, 0xffffffff, 0xffffffff,   // Page36
      0xffffffff, 0xffffffff, 0xffffffff, 0x00000001, 0x00000000 },
    { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffc00000,   // Page37
      0x00002000, 0x00000000, 0xffff8000, 0x00000001, 0x00000000 },
    { 0x03f00000, 0x00000000, 0x00000000, 0xffff0000, 0xffffffff,   // Page38
      0xffffffff, 0xffffffff, 0xffffffff, 0x00000001, 0x00000000 },
    { 0xfffff3de, 0xfffffeff, 0x7f47afff, 0x000000fe, 0xff100000,   // Page39
      0x7ffeffff, 0x00000000, 0x00000000, 0x00000000, 0x00000001 },
    { 0x00000000, 0xfffe0000, 0xffffffff, 0x0000001f, 0x00000000,   // Page49
      0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000010 },
    { 0x00000000, 0x00000000, 0x00000000, 0x80000000, 0x00000000,   // Page50
      0x00000000, 0x00000fff, 0x00000000, 0x00000000, 0x00000001 },
    { 0x00000000, 0x00000000, 0xff000000, 0x0001ffff, 0x00000000,   // Page51
      0x00000000, 0x00000000, 0x7fffffff, 0x00000000, 0x00000001 },
    { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,   // Page159
      0xffffffc0, 0xffffffff, 0xffffffff, 0x00000020, 0x00000000 },
    { 0x00000000, 0xffffc000, 0xffffffff, 0xffffffff, 0xffffffff,   // Page250
      0xffffffff, 0xffffffff, 0xffffffff, 0x00000020, 0x00000000 },
    { 0x00000000, 0xc0000000, 0x00000000, 0x00000000, 0x00000000,   // Page253
      0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001 },
    { 0x00000000, 0xfff90000, 0xfef7fe1f, 0x00000f77, 0x00000000,   // Page254
      0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001 }
};

//
//  CType 3 Flag Bits.
//
//  In the interest of reducing our table complexity, we've here a reduced
//  bitfield.  Only those bits currently used by IE4 are returned by
//  GetStringType3Ex().
//

// These are the flags are they are defined in winnls.h
//

// C3_NONSPACING    0x0001
// C3_DIACRITIC     0x0002
// C3_VOWELMARK     0x0004
// C3_SYMBOL        0x0008
// C3_KATAKANA      0x0010
// C3_HIRAGANA      0x0020
// C3_HALFWIDTH     0x0040
// C3_FULLWIDTH     0x0080
// C3_IDEOGRAPH     0x0100
// C3_KASHIDA       0x0200
// C3_LEXICAL       0x0400
// C3_ALPHA         0x8000

// The supported flags are encoded by shifting them to the right 3 bits.

// C3_SYMBOL       0x0001
// C3_KATAKANA     0x0002
// C3_HIRAGANA     0x0004
// C3_HALFWIDTH    0x0008
// C3_FULLWIDTH    0x0010
// C3_IDEOGRAPH    0x0020

// GetStringType3Ex returns the correct Win32 flags NOT the compressed flags.

BOOL GetStringType3ExW(
    LPCWSTR lpSrcStr,    // string arg
    int     cchSrc,      // length (or -1)
    LPWORD  lpCharType ) // output buffer
{
    LPCWSTR  lpStop = lpSrcStr + ((cchSrc == -1) ? MAXLONG : cchSrc);

    while (lpSrcStr < lpStop)
    {
        WCHAR wch = *lpSrcStr++;
        WORD wCharType;
        BYTE bPageSub;

        if (!wch && cchSrc == -1)
            break;

        switch (wch & (unsigned int)0xff00)
        {
            case 0x0000:
                wCharType = abType3Page0[wch];         // Page0: 4 values
                break;
            case 0x2000:
                wCharType = abType3Page32[wch & 0xff]; // Page32: 4 values
                break;
            case 0x3000:
                wCharType = abType3Page48[wch & 0xff];  // Page48: 10 values
                break;
            case 0xff00:
                wCharType = abType3Page255[wch & 0xff]; // Page255: 7 values
                break;
            default:
                bPageSub = abType3PageSub[wch>>8];

                if (bPageSub & 0x80)                  // 21 pages have 2 values
                {
                    const struct tagType3DualValue *p = aType3DualValue +
                        (bPageSub & 0x7f);

                    wCharType = (BYTE) p->adwValue[(p->adwBitfield[(wch>>5)&7]
                        >> (wch & 0x1f)) & 1];
                }
                else                                  // 231 pages have 1 value
                {
                    wCharType = bPageSub;
                }
                break;
        }

        *lpCharType++ = wCharType << 3;
    }
    
    return TRUE;
}

//
//  Str Functions from SHLWAPI
//
int StrCmpW(
    IN LPCWSTR pwsz1,
    IN LPCWSTR pwsz2)
{
    int iRet = -1;  // arbitrary on failure

    ASSERT(IS_VALID_STRING_PTRW(pwsz1, -1));
    ASSERT(IS_VALID_STRING_PTRW(pwsz2, -1));
    
    if (pwsz1 && pwsz2)
    {
        CStrIn psz1(pwsz1);
        CStrIn psz2(pwsz2);
        
        iRet = lstrcmpA(psz1, psz2);
    }
    return iRet;
}

int StrCmpIW(
    IN LPCWSTR pwsz1,
    IN LPCWSTR pwsz2)
{
    int iRet = -1;  // arbitrary on failure

    ASSERT(IS_VALID_STRING_PTRW(pwsz1, -1));
    ASSERT(IS_VALID_STRING_PTRW(pwsz2, -1));
    
    if (pwsz1 && pwsz2)
    {
        CStrIn psz1(pwsz1);
        CStrIn psz2(pwsz2);

        iRet = lstrcmpiA(psz1, psz2);
    }

    return iRet;
}

#if 0   // BUGBUG: We have another StrCpyW in strings.c
LPWSTR StrCpyW(LPWSTR psz1, LPCWSTR psz2)
{
    LPWSTR psz = psz1;

    ASSERT(psz1);
    ASSERT(psz2);

    while (*psz1++ = *psz2++)
        ;

    return psz;
}
#endif


LPWSTR StrCatW(LPWSTR psz1, LPCWSTR psz2)
{
    LPWSTR psz = psz1;

    ASSERT(psz1);
    ASSERT(psz2);

    while (0 != *psz1)
        psz1++;

    while (*psz1++ = *psz2++)
        ;

    return psz;
}


//+------------------------------------------------------------------------
//
//  Implementation of the wrapped functions
//
//-------------------------------------------------------------------------

BOOL AppendMenuWrap(
        HMENU   hMenu,
        UINT    uFlags,
        UINT    uIDnewItem,
        LPCWSTR lpnewItem)
{
    ASSERT(!(uFlags & MF_BITMAP) && !(uFlags & MF_OWNERDRAW));

    CStrIn  str(lpnewItem);

    return AppendMenuA(hMenu, uFlags, uIDnewItem, str);
}

BOOL CallMsgFilterWrap(LPMSG lpMsg, int nCode)
{
    return CallMsgFilterA(lpMsg, nCode);
}

LRESULT CallWindowProcWrap(
    WNDPROC lpPrevWndFunc,
    HWND    hWnd,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam)
{
    return CallWindowProcA(lpPrevWndFunc, hWnd, Msg, wParam, lParam);
}

//----------------------------------------------------------------------
//
// function:    CharLowerWrap( LPWSTR pch )
//
// purpose:     Converts character to lowercase.  Takes either a pointer
//              to a string, or a character masquerading as a pointer.
//              In the later case, the HIWORD must be zero.  This is
//              as spec'd for Win32.
//
// returns:     Lowercased character or string.  In the string case,
//              the lowercasing is done inplace.
//
//----------------------------------------------------------------------

LPWSTR CharLowerWrap( LPWSTR pch )
{
    if (!HIWORD64(pch))
    {
        WCHAR ch = (WCHAR)(LONG_PTR)pch;

        CharLowerBuffWrap( &ch, 1 );

        pch = (LPWSTR)MAKEINTATOM(ch);
    }
    else
    {
        CharLowerBuffWrap( pch, lstrlenW(pch) );
    }

    return pch;
}

//----------------------------------------------------------------------
//
// function:    CharLowerBuffWrap( LPWSTR pch, DWORD cch )
//
// purpose:     Converts a string to lowercase.  String must be cch
//              characters in length.
//
// returns:     Character count (cch).  The lowercasing is done inplace.
//
//----------------------------------------------------------------------

DWORD CharLowerBuffWrap( LPWSTR pch, DWORD cchLength )
{
    DWORD cch;

    for ( cch = cchLength; cch-- ; pch++ )
    {
        WCHAR ch = *pch;

        if (IsCharUpperWrap(ch))
        {
            if (ch < 0x0100)
            {
                *pch += 32;             // Get Latin-1 out of the way first
            }
            else if (ch < 0x0531)
            {
                if (ch < 0x0391)
                {
                    if (ch < 0x01cd)
                    {
                        if (ch <= 0x178)
                        {
                            if (ch < 0x0178)
                            {
                                *pch += (ch == 0x0130) ? 0 : 1;
                            }
                            else
                            {
                                *pch -= 121;
                            }
                        }
                        else
                        {
                            static const BYTE abLookup[] =
                            {  // 0/8  1/9  2/a  3/b  4/c  5/d  6/e  7/f
            /* 0x0179-0x17f */           1,   0,   1,   0,   1,   0,   0,
            /* 0x0180-0x187 */      0, 210,   1,   0,   1,   0, 206,   1,
            /* 0x0188-0x18f */      0, 205, 205,   1,   0,   0,  79, 202,
            /* 0x0190-0x197 */    203,   1,   0, 205, 207,   0, 211, 209,
            /* 0x0198-0x19f */      1,   0,   0,   0, 211, 213,   0, 214,
            /* 0x01a0-0x1a7 */      1,   0,   1,   0,   1,   0,   0,   1,
            /* 0x01a8-0x1af */      0, 218,   0,   0,   1,   0, 218,   1,
            /* 0x01b0-0x1b7 */      0, 217, 217,   1,   0,   1,   0, 219,
            /* 0x01b8-0x1bf */      1,   0,   0,   0,   1,   0,   0,   0,
            /* 0x01c0-0x1c7 */      0,   0,   0,   0,   2,   0,   0,   2,
            /* 0x01c8-0x1cb */      0,   0,   2,   0
                            };

                            *pch += abLookup[ch-0x0179];
                        }
                    }
                    else if (ch < 0x0386)
                    {
                        switch (ch)
                        {
                            case 0x01f1: *pch += 2; break;
                            case 0x01f2: break;
                            default: *pch += 1;
                        }
                    }
                    else
                    {
                        static const BYTE abLookup[] =
                            { 38, 0, 37, 37, 37, 0, 64, 0, 63, 63 };

                        *pch += abLookup[ch-0x0386];
                    }
                }
                else
                {
                    if (ch < 0x0410)
                    {
                        if (ch < 0x0401)
                        {
                            if (ch < 0x03e2)
                            {
                                if (!InRange(ch, 0x03d2, 0x03d4) &&
                                    !(InRange(ch, 0x3da, 0x03e0) & !(ch & 1)))
                                {
                                    *pch += 32;
                                }
                            }
                            else
                            {
                                *pch += 1;
                            }
                        }
                        else
                        {
                            *pch += 80;
                        }
                    }
                    else
                    {
                        if (ch < 0x0460)
                        {
                            *pch += 32;
                        }
                        else
                        {
                            *pch += 1;
                        }
                    }
                }
            }
            else
            {
                if (ch < 0x2160)
                {
                    if (ch < 0x1fba)
                    {
                        if (ch < 0x1f08)
                        {
                            if (ch < 0x1e00)
                            {
                                *pch += 48;
                            }
                            else
                            {
                                *pch += 1;
                            }
                        }
                        else if (!(InRange(ch, 0x1f88, 0x1faf) && (ch & 15)>7))
                        {
                            *pch -= 8;
                        }
                    }
                    else
                    {
                        static const BYTE abLookup[] =
                        {  // 8    9    a    b    c    d    e    f
                              0,   0,  74,  74,   0,   0,   0,   0,
                             86,  86,  86,  86,   0,   0,   0,   0,
                              8,   8, 100, 100,   0,   0,   0,   0,
                              8,   8, 112, 112,   7,   0,   0,   0,
                            128, 128, 126, 126,   0,   0,   0,   0
                        };
                        int i = (ch-0x1fb0);

                        *pch -= (int)abLookup[((i>>1) & ~7) | (i & 7)];
                    }
                }
                else
                {
                    if (ch < 0xff21)
                    {
                        if (ch < 0x24b6)
                        {
                            *pch += 16;
                        }
                        else
                        {
                            *pch += 26;
                        }
                    }
                    else
                    {
                        *pch += 32;
                    }
                }
            }
        }
        else
        {
            // These are Unicode Number Forms.  They have lowercase counter-
            // parts, but are not considered uppercase.  Why, I don't know.

            if (InRange(ch, 0x2160, 0x216f))
            {
                *pch += 16;
            }
        }
    }

    return cchLength;
}

//
// BUGBUG - Do CharNextWrap and CharPrevWrap need to call the 
//          CharNextW, CharPrevW on WinNT?  Couldn't these be MACROS?
 
LPWSTR CharNextWrap(LPCWSTR lpszCurrent)
{
    if (*lpszCurrent)
    {
        return (LPWSTR) lpszCurrent + 1;
    }
    else
    {
        return (LPWSTR) lpszCurrent;
    }
}

LPWSTR CharPrevWrap(LPCWSTR lpszStart, LPCWSTR lpszCurrent)
{
    if (lpszCurrent == lpszStart)
    {
        return (LPWSTR) lpszStart;
    }
    else
    {
        return (LPWSTR) lpszCurrent - 1;
    }
}

BOOL CharToOemWrap(LPCWSTR lpszSrc, LPSTR lpszDst)
{
    CStrIn  str(lpszSrc);

    return CharToOemA(str, lpszDst);
}

//----------------------------------------------------------------------
//
// function:    CharUpperWrap( LPWSTR pch )
//
// purpose:     Converts character to uppercase.  Takes either a pointer
//              to a string, or a character masquerading as a pointer.
//              In the later case, the HIWORD must be zero.  This is
//              as spec'd for Win32.
//
// returns:     Uppercased character or string.  In the string case,
//              the uppercasing is done inplace.
//
//----------------------------------------------------------------------

LPWSTR CharUpperWrap( LPWSTR pch )
{
    if (!HIWORD64(pch))
    {
        WCHAR ch = (WCHAR)(LONG_PTR)pch;

        CharUpperBuffWrap( &ch, 1 );

        pch = (LPWSTR)MAKEINTATOM(ch);
    }
    else
    {
        CharUpperBuffWrap( pch, lstrlenW(pch) );
    }

    return pch;
}

//----------------------------------------------------------------------
//
// function:    CharUpperBuffWrap( LPWSTR pch, DWORD cch )
//
// purpose:     Converts a string to uppercase.  String must be cch
//              characters in length.  Note that this function is
//              is messier that CharLowerBuffWrap, and the reason for
//              this is many Unicode characters are considered uppercase,
//              even when they don't have an uppercase counterpart.
//
// returns:     Character count (cch).  The uppercasing is done inplace.
//
//----------------------------------------------------------------------

DWORD CharUpperBuffWrap( LPWSTR pch, DWORD cchLength )
{
    DWORD cch;
    
    for ( cch = cchLength; cch-- ; pch++ )
    {
        WCHAR ch = *pch;

        if (IsCharLowerWrap(ch))
        {
            if (ch < 0x00ff)
            {
                *pch -= ((ch != 0xdf) << 5);
            }
            else if (ch < 0x03b1)
            {
                if (ch < 0x01f5)
                {
                    if (ch < 0x01ce)
                    {
                        if (ch < 0x017f)
                        {
                            if (ch < 0x0101)
                            {
                                *pch += 121;
                            }
                            else
                            {
                                *pch -= (ch != 0x0131 &&
                                         ch != 0x0138 &&
                                         ch != 0x0149);
                            }
                        }
                        else if (ch < 0x01c9)
                        {
                            static const BYTE abMask[] =
                            {                       // 6543210f edcba987
                                0xfc, 0xbf,         // 11111100 10111111
                                0xbf, 0x67,         // 10111111 01100111
                                0xff, 0xef,         // 11111111 11101111
                                0xff, 0xf7,         // 11111111 11110111
                                0xbf, 0xfd          // 10111111 11111101
                            };

                            int i = ch - 0x017f;

                            *pch -= ((abMask[i>>3] >> (i&7)) & 1) +
                                    (ch == 0x01c6);                
                        }
                        else 
                        {
                            *pch -= ((ch != 0x01cb)<<1);
                        }
                    }
                    else
                    {
                        if (ch < 0x01df)
                        {
                            if (ch < 0x01dd)
                            {
                                *pch -= 1;
                            }
                            else 
                            {
                                *pch -= 79;
                            }
                        }
                        else 
                        {
                            *pch -= 1 + (ch == 0x01f3) -
                                    InRange(ch,0x01f0,0x01f2);
                        }
                    }
                }
                else if (ch < 0x0253)
                {
                    *pch -= (ch < 0x0250);
                }
                else if (ch < 0x03ac)
                {
                    static const BYTE abLookup[] =
                    {// 0/8  1/9  2/a  3/b  4/c  5/d  6/e  7/f
    /* 0x0253-0x0257 */                210, 206,   0, 205, 205,
    /* 0x0258-0x025f */   0, 202,   0, 203,   0,   0,   0,   0,
    /* 0x0260-0x0267 */ 205,   0,   0, 207,   0,   0,   0,   0,
    /* 0x0268-0x026f */ 209, 211,   0,   0,   0,   0,   0, 211,
    /* 0x0270-0x0277 */   0,   0, 213,   0,   0, 214,   0,   0,
    /* 0x0278-0x027f */   0,   0,   0,   0,   0,   0,   0,   0,
    /* 0x0280-0x0287 */   0,   0,   0, 218,   0,   0,   0,   0,
    /* 0x0288-0x028f */ 218,   0, 217, 217,   0,   0,   0,   0,
    /* 0x0290-0x0297 */   0,   0, 219
                    };

                    if (ch <= 0x0292)
                    {
                        *pch -= abLookup[ch - 0x0253];
                    }
                }
                else 
                {
                    *pch -= (ch == 0x03b0) ? 0 : (37 + (ch == 0x03ac));
                }
            }
            else
            {
                if (ch < 0x0561)
                {
                    if (ch < 0x0451)
                    {
                        if (ch < 0x03e3)
                        {
                            if (ch < 0x03cc)
                            {
                                *pch -= (ch != 0x03c2)<<5;
                            }
                            else 
                            {
                                int i = (ch < 0x03d0);
                                *pch -= (i<<6) - i + (ch == 0x03cc);
                            }
                        }
                        else if (ch < 0x0430)
                        {
                            *pch -= (ch < 0x03f0);
                        }
                        else 
                        {
                            *pch -= 32;
                        }
                    }
                    else if (ch < 0x0461)
                    {
                        *pch -= 80;
                    }
                    else 
                    {
                        *pch -= 1;
                    }
                }
                else
                {
                    if (ch < 0x1fb0)
                    {
                        if (ch < 0x1f70)
                        {
                            if (ch < 0x1e01)
                            {
                                int i = ch != 0x0587 && ch != 0x10f6;
                                *pch -= ((i<<5)+(i<<4)); /* 48 */
                            }
                            else if (ch < 0x1f00)
                            {
                                *pch -= !InRange(ch, 0x1e96, 0x1e9a);
                            }
                            else 
                            {
                                int i = !InRange(ch, 0x1f50, 0x1f56)||(ch & 1);
                                *pch += (i<<3);
                            }
                        }
                        else 
                        {
                            static const BYTE abLookup[] =
                                { 74, 86, 86, 100, 128, 112, 126 };

                            if ( ch <= 0x1f7d )
                            {
                                *pch += abLookup[(ch-0x1f70)>>1];
                            }
                        }
                    }
                    else
                    {
                        if (ch < 0x24d0)
                        {
                            if (ch < 0x1fe5)
                            {
                                *pch += (0x0023 & (1<<(ch&15))) ? 8 : 0;
                            }
                            else if (ch < 0x2170)
                            {
                                *pch += (0x0023 & (1<<(ch&15))) ? 7 : 0;
                            }                
                            else 
                            {
                                *pch -= ((ch > 0x24b5)<<4);
                            }
                        }
                        else if (ch < 0xff41)
                        {   
                            int i = !InRange(ch, 0xfb00, 0xfb17);
                            *pch -= (i<<4)+(i<<3)+(i<<1); /* 26 */
                        }
                        else
                        {
                            *pch -= 32;
                        }
                    }
                }
            }
        }
        else
        {
            int i = InRange(ch, 0x2170, 0x217f);
            *pch -= (i<<4);
        }
    }

    return cchLength;
}

int CopyAcceleratorTableWrap(
        HACCEL  hAccelSrc,
        LPACCEL lpAccelDst,
        int     cAccelEntries)
{
    return CopyAcceleratorTableA(hAccelSrc, lpAccelDst, cAccelEntries);
}

HACCEL CreateAcceleratorTableWrap(LPACCEL lpAccel, int cEntries)
{
    return CreateAcceleratorTableA(lpAccel, cEntries);
}

typedef HDC (*FnCreateHDCA)(LPCSTR, LPCSTR, LPCSTR, CONST DEVMODEA *);

HDC CreateHDCWrap(
        LPCWSTR             lpszDriver,
        LPCWSTR             lpszDevice,
        LPCWSTR             lpszOutput,
        CONST DEVMODEW *    lpInitData,
        FnCreateHDCA        pfn)
{
    DEVMODEA *  pdevmode = NULL;
    CStrIn      strDriver(lpszDriver);
    CStrIn      strDevice(lpszDevice);
    CStrIn      strOutput(lpszOutput);
    HDC         hdcReturn = 0;

    if (lpInitData)
    {
        pdevmode = (DEVMODEA *) LocalAlloc( LPTR, lpInitData->dmSize + lpInitData->dmDriverExtra );

        if (pdevmode)
        {
            MbcsFromUnicode((CHAR *)pdevmode->dmDeviceName, CCHDEVICENAME, lpInitData->dmDeviceName);
            memcpy(&pdevmode->dmSpecVersion,
                    &lpInitData->dmSpecVersion,
                    FIELD_OFFSET(DEVMODEW,dmFormName) - FIELD_OFFSET(DEVMODEW,dmSpecVersion));
            MbcsFromUnicode((CHAR *)pdevmode->dmFormName, CCHFORMNAME, lpInitData->dmFormName);
            memcpy(&pdevmode->dmLogPixels,
                    &lpInitData->dmLogPixels,
                    lpInitData->dmDriverExtra + lpInitData->dmSize - FIELD_OFFSET(DEVMODEW, dmLogPixels));

            pdevmode->dmSize -= (sizeof(BCHAR) - sizeof(char)) * (CCHDEVICENAME + CCHFORMNAME);
        }
    }

    hdcReturn = (*pfn)(strDriver, strDevice, strOutput, pdevmode);

    if (pdevmode)
    {
        LocalFree(pdevmode);
    }

    return hdcReturn;
}

HDC CreateDCWrap(
        LPCWSTR             lpszDriver,
        LPCWSTR             lpszDevice,
        LPCWSTR             lpszOutput,
        CONST DEVMODEW *    lpInitData)
{
    return CreateHDCWrap(lpszDriver, lpszDevice, lpszOutput, lpInitData, CreateDCA);
}

HDC CreateICWrap(
        LPCWSTR             lpszDriver,
        LPCWSTR             lpszDevice,
        LPCWSTR             lpszOutput,
        CONST DEVMODEW *    lpInitData)
{
    return CreateHDCWrap(lpszDriver, lpszDevice, lpszOutput, lpInitData, CreateICA);
}


BOOL CreateDirectoryWrap(
        LPCWSTR                 lpPathName,
        LPSECURITY_ATTRIBUTES   lpSecurityAttributes)
{
    CStrIn  str(lpPathName);

    ASSERT(!lpSecurityAttributes);
    return CreateDirectoryA(str, lpSecurityAttributes);
}

HANDLE CreateEventWrap(
        LPSECURITY_ATTRIBUTES   lpEventAttributes,
        BOOL                    bManualReset,
        BOOL                    bInitialState,
        LPCWSTR                 lpName)
{
    return CreateEventA(lpEventAttributes, bManualReset, bInitialState, (LPCSTR) lpName);
}

HANDLE CreateFileWrap(
        LPCWSTR                 lpFileName,
        DWORD                   dwDesiredAccess,
        DWORD                   dwShareMode,
        LPSECURITY_ATTRIBUTES   lpSecurityAttributes,
        DWORD                   dwCreationDisposition,
        DWORD                   dwFlagsAndAttributes,
        HANDLE                  hTemplateFile)
{
    CStrIn  str(lpFileName);

    return CreateFileA(
            str,
            dwDesiredAccess,
            dwShareMode,
            lpSecurityAttributes,
            dwCreationDisposition,
            dwFlagsAndAttributes,
            hTemplateFile);
}

HANDLE CreateFileMappingWrap(
        HANDLE hFile,
        LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
        DWORD flProtect,
        DWORD dwMaxSizeHigh,
        DWORD dwMaxSizeLow,
        LPCWSTR lpName)
{
    CStrIn str(lpName);

    return CreateFileMappingA(
            hFile,
            lpFileMappingAttributes,
            flProtect,
            dwMaxSizeHigh,
            dwMaxSizeLow,
            str);
}

HFONT CreateFontWrap(
        int nHeight,
        int nWidth,
        int nEscapement,
        int nOrientation,
        int fnWeight,
        DWORD fdwItalic,
        DWORD fdwUnderline,
        DWORD fdwStrikeOut,
        DWORD fdwCharSet,
        DWORD fdwOutputPrecision,
        DWORD fdwClipPrecision,
        DWORD fdwQuality,
        DWORD fdwPitchAndFamily,
        LPCWSTR lpszFace)
{
    CStrIn str(lpszFace);

    return CreateFontA(
        nHeight,
        nWidth,
        nEscapement,
        nOrientation,
        fnWeight,
        fdwItalic,
        fdwUnderline,
        fdwStrikeOut,
        fdwCharSet,
        fdwOutputPrecision,
        fdwClipPrecision,
        fdwQuality,
        fdwPitchAndFamily,
        str);
}

HFONT CreateFontIndirectWrap(CONST LOGFONTW * plfw)
{
    LOGFONTA  lfa;
    HFONT     hFont;

    memcpy(&lfa, plfw, FIELD_OFFSET(LOGFONTA, lfFaceName));
    MbcsFromUnicode(lfa.lfFaceName, ARRAYSIZE(lfa.lfFaceName), plfw->lfFaceName);
    hFont = CreateFontIndirectA(&lfa);

    return hFont;
}

HWND CreateWindowExWrap(
        DWORD       dwExStyle,
        LPCWSTR     lpClassName,
        LPCWSTR     lpWindowName,
        DWORD       dwStyle,
        int         X,
        int         Y,
        int         nWidth,
        int         nHeight,
        HWND        hWndParent,
        HMENU       hMenu,
        HINSTANCE   hInstance,
        LPVOID      lpParam)
{
    CStrIn  strClass(lpClassName);
    CStrIn  strWindow(lpWindowName);

    return CreateWindowExA(
            dwExStyle,
            strClass,
            strWindow,
            dwStyle,
            X,
            Y,
            nWidth,
            nHeight,
            hWndParent,
            hMenu,
            hInstance,
            lpParam);
}

LRESULT DefWindowProcWrap(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

BOOL DeleteFileWrap(LPCWSTR pwsz)
{
    CStrIn  str(pwsz);

    return DeleteFileA(str);
}

LRESULT DispatchMessageWrap(CONST MSG * lpMsg)
{
    return DispatchMessageA(lpMsg);
}

#ifndef FONT_LINK
int DrawTextWrap(
        HDC     hDC,
        LPCWSTR lpString,
        int     nCount,
        LPRECT  lpRect,
        UINT    uFormat)
{
    CStrIn  str(lpString, nCount);

    return DrawTextA(hDC, str, str.strlen(), lpRect, uFormat);
}

// shlwapi also have this.
int DrawTextExPrivWrap(
        HDC     hDC,
        LPWSTR  lpString,
        int     nCount,
        LPRECT  lpRect,
        UINT    uFormat,
        LPDRAWTEXTPARAMS lpDTParams)
{
    CStrIn  str(lpString, nCount);

    return DrawTextExA(hDC, str, str.strlen(), lpRect, uFormat, lpDTParams);
}
#endif

struct EFFSTAT
{
    LPARAM          lParam;
    FONTENUMPROC    lpEnumFontProc;
    BOOL            fFamilySpecified;
};

int CALLBACK EnumFontFamiliesCallbackWrap(
        ENUMLOGFONTA *  lpelf,
        NEWTEXTMETRIC * lpntm,
        DWORD           FontType,
        LPARAM          lParam)
{
    ENUMLOGFONTW    elf;

    //  Convert strings from ANSI to Unicode
    if (((EFFSTAT *)lParam)->fFamilySpecified && (FontType & TRUETYPE_FONTTYPE) )
    {
        UnicodeFromMbcs(
                        elf.elfFullName,
                        ARRAYSIZE(elf.elfFullName),
                        (LPCSTR) lpelf->elfFullName);
        UnicodeFromMbcs(
                        elf.elfStyle,
                        ARRAYSIZE(elf.elfStyle),
                        (LPCSTR) lpelf->elfStyle);
    }
    else
    {
        elf.elfStyle[0] = L'\0';
        elf.elfFullName[0] = L'\0';
    }

    UnicodeFromMbcs(
            elf.elfLogFont.lfFaceName,
            ARRAYSIZE(elf.elfLogFont.lfFaceName),
            (LPCSTR) lpelf->elfLogFont.lfFaceName);

    //  Copy the non-string data
    memcpy(
            &elf.elfLogFont,
            &lpelf->elfLogFont,
            FIELD_OFFSET(LOGFONTA, lfFaceName));

    //  Chain to the original callback function
    return (*((EFFSTAT *) lParam)->lpEnumFontProc)(
            (const LOGFONTW *) &elf,
            (const TEXTMETRICW *) lpntm,
            FontType,
            ((EFFSTAT *) lParam)->lParam);
}

int EnumFontFamiliesWrap(
        HDC          hdc,
        LPCWSTR      lpszFamily,
        FONTENUMPROC lpEnumFontProc,
        LPARAM       lParam)
{
    CStrIn  str(lpszFamily);
    EFFSTAT effstat;

    effstat.lParam = lParam;
    effstat.lpEnumFontProc = lpEnumFontProc;
    effstat.fFamilySpecified = lpszFamily != NULL;

    return EnumFontFamiliesA(
            hdc,
            str,
            (FONTENUMPROCA) EnumFontFamiliesCallbackWrap,
            (LPARAM) &effstat);
}

int EnumFontFamiliesExWrap(
        HDC          hdc,
        LPLOGFONTW   lplfw,
        FONTENUMPROC lpEnumFontProc,
        LPARAM       lParam,
        DWORD        dwFlags )
{
    LOGFONTA lfa;
    CStrIn   str(lplfw->lfFaceName);
    EFFSTAT  effstat;

    ASSERT( FIELD_OFFSET(LOGFONTW, lfFaceName) == FIELD_OFFSET(LOGFONTA, lfFaceName) );
    
    memcpy( &lfa, lplfw, sizeof(LOGFONTA) - FIELD_OFFSET(LOGFONTA, lfFaceName) );
    memcpy( lfa.lfFaceName, str, LF_FACESIZE );

    effstat.lParam = lParam;
    effstat.lpEnumFontProc = lpEnumFontProc;
    effstat.fFamilySpecified = lplfw->lfFaceName != NULL;

    return EnumFontFamiliesExA(
            hdc,
            &lfa,
            (FONTENUMPROCA) EnumFontFamiliesCallbackWrap,
            (LPARAM) &effstat,
            dwFlags );
}

BOOL EnumResourceNamesWrap(
        HINSTANCE        hModule,
        LPCWSTR          lpType,
        ENUMRESNAMEPROCW lpEnumFunc,
        LONG             lParam)
{
    ASSERT(HIWORD64(lpType) == 0);

    return EnumResourceNamesA(hModule, (LPCSTR) lpType, (ENUMRESNAMEPROCA)lpEnumFunc, lParam);
}

#ifndef FONT_LINK
//
//  There's an app that patches Win95 GDI and their ExtTextOutW handler
//  is broken.  It always dereferences the lpStr parameter, even if
//  cb is zero.  Consequently, any time we are about to pass NULL as
//  the lpStr, we have to change our mind and pass a null UNICODE string
//  instead.
//
//  The name of this app:  Lotus SmartSuite ScreenCam 97.
//
BOOL ExtTextOutWrap(HDC hdc, int x, int y, UINT fuOptions, CONST RECT *lprc, LPCWSTR lpStr, UINT cch, CONST INT *lpDx)
{
    // Force a thunk to ANSI if running Win95 + ME
    if (g_fMEEnabled && !g_bRunOnMemphis)
    {
        CStrIn str(lpStr, cch);

        return ExtTextOutA(hdc, x, y, fuOptions, lprc, str, str.strlen(), lpDx);
    }
    else
    {
        if (lpStr == NULL)              // Stupid workaround
            lpStr = TEXT("");           // for ScreenCam 97
        return ExtTextOutW(hdc, x, y, fuOptions, lprc, lpStr, cch, lpDx);
    }
}    
#endif

HANDLE FindFirstFileWrap(
        LPCWSTR             lpFileName,
        LPWIN32_FIND_DATAW  pwszFd)
{
    CStrIn              str(lpFileName);
    WIN32_FIND_DATAA    fd;
    HANDLE              ret;

    memcpy(&fd, pwszFd, sizeof(FILETIME)*3+sizeof(DWORD)*5);

    ret = FindFirstFileA(str, &fd);

    memcpy(pwszFd, &fd, sizeof(FILETIME)*3+sizeof(DWORD)*5);

    UnicodeFromMbcs(pwszFd->cFileName, ARRAYSIZE(pwszFd->cFileName), fd.cFileName);
    UnicodeFromMbcs(pwszFd->cAlternateFileName, ARRAYSIZE(pwszFd->cAlternateFileName), fd.cAlternateFileName);

    return ret;
}

//
// Although Win95 implements FindResource[Ex]W, its implementation is buggy
// if you pass a string parameter, so we must thunk to the ANSI side.
//
// The bug is that FindResource[Ex]W will accidentally
// call LocalFree(lpName) and LocalFree(lpType), so if lpName and lpType
// point to heap memory, Kernel32 secretly freed your memory and you fault
// five minutes later.
//
HRSRC FindResourceExWrap(HINSTANCE hModule, LPCWSTR lpType, LPCWSTR lpName, WORD wLang)
{
    CStrIn  strType(lpType);            // rlefile.cpp passes TEXT("AVI")
    CStrIn  strName(lpName);

    return FindResourceExA(hModule, strType, strName, wLang);
}

HWND FindWindowWrap(LPCWSTR lpClassName, LPCWSTR lpWindowName)
{
    CStrIn  strClass(lpClassName);
    CStrIn  strWindow(lpWindowName);

    return FindWindowA(strClass, strWindow);
}

DWORD FormatMessageWrap(
    DWORD       dwFlags,
    LPCVOID     lpSource,
    DWORD       dwMessageId,
    DWORD       dwLanguageId,
    LPWSTR      lpBuffer,
    DWORD       nSize,
    va_list *   Arguments)
{
    //This assert is only valid on Windows 95.
    ASSERT(!(dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER));

    CStrOut str(lpBuffer, nSize);

    FormatMessageA(
            dwFlags,
            lpSource,
            dwMessageId,
            dwLanguageId,
            str,
            str.BufSize(),
            Arguments);

    return str.ConvertExcludingNul();
}

BOOL GetClassInfoWrap(HINSTANCE hModule, LPCWSTR lpClassName, LPWNDCLASSW lpWndClassW)
{
    BOOL    ret;

    CStrIn  strClassName(lpClassName);

    ASSERT(sizeof(WNDCLASSA) == sizeof(WNDCLASSW));

    ret = GetClassInfoA(hModule, strClassName, (LPWNDCLASSA) lpWndClassW);

    lpWndClassW->lpszMenuName = NULL;
    lpWndClassW->lpszClassName = NULL;
    return ret;
}

DWORD GetClassLongWrap(HWND hWnd, int nIndex)
{
    return GetClassLongA(hWnd, nIndex);
}

int GetClassNameWrap(HWND hWnd, LPWSTR lpClassName, int nMaxCount)
{
    CStrOut strClassName(lpClassName, nMaxCount);

    GetClassNameA(hWnd, strClassName, strClassName.BufSize());
    return strClassName.ConvertIncludingNul();
}

int GetClipboardFormatNameWrap(UINT format, LPWSTR lpFormatName, int cchFormatName)
{
    CStrOut strFormatName(lpFormatName, cchFormatName);

    GetClipboardFormatNameA(format, strFormatName, strFormatName.BufSize());
    return strFormatName.ConvertIncludingNul();
}

DWORD GetCurrentDirectoryWrap(DWORD nBufferLength, LPWSTR lpBuffer)
{
    CStrOut str(lpBuffer, nBufferLength);

    GetCurrentDirectoryA(str.BufSize(), str);
    return str.ConvertExcludingNul();
}

int GetDateFormatWrap(
        LCID Locale,
        DWORD dwFlags,
        CONST SYSTEMTIME *lpDate,
        LPCWSTR lpFormat,
        LPWSTR lpDateStr,
        int cchDate)
{
    CStrIn strFormat(lpFormat);
    CStrOut str(lpDateStr, cchDate);

    ASSERT(cchDate != 0 || lpDateStr == NULL);

    int iRc = GetDateFormatA(Locale, dwFlags, lpDate, strFormat, str, str.BufSize());

    // If app was merely querying, then return size and stop
    if (!str)
        return iRc;

    return str.ConvertIncludingNul();
}

UINT GetDlgItemTextWrap(
        HWND    hWndDlg,
        int     idControl,
        LPWSTR  lpsz,
        int     cchMax)
{
    CStrOut str(lpsz, cchMax);

    GetDlgItemTextA(hWndDlg, idControl, str, str.BufSize());
    return str.ConvertExcludingNul();
}

DWORD GetFileAttributesWrap(LPCWSTR lpFileName)
{
    CStrIn  str(lpFileName);

    return GetFileAttributesA(str);
}

int GetKeyNameTextWrap(LONG lParam, LPWSTR lpsz, int nSize)
{
    CStrOut str(lpsz, nSize);

    GetKeyNameTextA(lParam, str, str.BufSize());
    return str.ConvertExcludingNul();
}

int GetLocaleInfoWrap(LCID Locale, LCTYPE LCType, LPWSTR lpsz, int cchData)
{
    CStrOut str(lpsz, cchData);

    GetLocaleInfoA(Locale, LCType, str, str.BufSize());
    return str.ConvertIncludingNul();
}

BOOL GetMenuItemInfoWrap(
    HMENU hMenu,
    UINT uItem,
    BOOL fByPosition,
    LPMENUITEMINFOW lpmiiW)
{
    BOOL fRet;
    
    ASSERT( sizeof(MENUITEMINFOW) == sizeof(MENUITEMINFOA) &&
            FIELD_OFFSET(MENUITEMINFOW, dwTypeData) ==
            FIELD_OFFSET(MENUITEMINFOA, dwTypeData) );

    if ( (MIIM_TYPE & lpmiiW->fMask) &&
         0 == (lpmiiW->fType & (MFT_BITMAP | MFT_SEPARATOR)))
    {
        MENUITEMINFOA miiA;
        CStrOut str(lpmiiW->dwTypeData, lpmiiW->cch);

        memcpy( &miiA, lpmiiW, sizeof(MENUITEMINFOA) );
        miiA.dwTypeData = str;
        miiA.cch = str.BufSize();
                
        fRet = GetMenuItemInfoA( hMenu, uItem, fByPosition, &miiA );

        memcpy(lpmiiW, &miiA, FIELD_OFFSET(MENUITEMINFOW, dwTypeData));
    }
    else
    {
        fRet = GetMenuItemInfoA( hMenu, uItem, fByPosition,
                                 (LPMENUITEMINFOA)lpmiiW );
    }

    return fRet;
}



int GetMenuStringWrap(
        HMENU   hMenu,
        UINT    uIDItem,
        LPWSTR  lpString,
        int     nMaxCount,
        UINT    uFlag)
{
    CStrOut str(lpString, nMaxCount);

    GetMenuStringA(hMenu, uIDItem, str, str.BufSize(), uFlag);
    return str.ConvertExcludingNul();
}

BOOL GetMessageWrap(
        LPMSG   lpMsg,
        HWND    hWnd,
        UINT    wMsgFilterMin,
        UINT    wMsgFilterMax)
{
    return GetMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
}

DWORD GetModuleFileNameWrap(HINSTANCE hModule, LPWSTR pwszFilename, DWORD nSize)
{
    CStrOut str(pwszFilename, nSize);

    GetModuleFileNameA(hModule, str, str.BufSize());
    return str.ConvertIncludingNul();
}


int GetNumberFormatWrap(
        LCID Locale,
        DWORD dwFlags,
        LPCWSTR lpValue,
        CONST NUMBERFMTW *lpFormat,
        LPWSTR lpNumberStr,
        int cchNumber)
{
    CStrIn strValue(lpValue);
    CStrOut str(lpNumberStr, cchNumber);

    ASSERT(cchNumber != 0);

    NUMBERFMTA nfA;
    CopyMemory(&nfA, lpFormat, sizeof(nfA));

    CStrIn strDec(lpFormat->lpDecimalSep);
    nfA.lpDecimalSep = strDec;

    CStrIn strThou(lpFormat->lpThousandSep);
    nfA.lpThousandSep = strThou;

    GetNumberFormatA(Locale, dwFlags, strValue, &nfA, str, str.BufSize());
    return str.ConvertIncludingNul();
}


UINT GetSystemDirectoryWrap(LPWSTR lpBuffer, UINT uSize)
{
    CStrOut str(lpBuffer, uSize);

    GetSystemDirectoryA(str, str.BufSize());
    return str.ConvertExcludingNul();
}

DWORD SearchPathWrap(
        LPCWSTR lpPathName,
        LPCWSTR lpFileName,
        LPCWSTR lpExtension,
        DWORD   cchReturnBuffer,
        LPWSTR  lpReturnBuffer,
        LPWSTR *  plpfilePart)
{
    CStrIn  strPath(lpPathName);
    CStrIn  strFile(lpFileName);
    CStrIn  strExtension(lpExtension);
    CStrOut strReturnBuffer(lpReturnBuffer, cchReturnBuffer);

    DWORD dwLen = SearchPathA(
            strPath,
            strFile,
            strExtension,
            strReturnBuffer.BufSize(),
            strReturnBuffer,
            (LPSTR *)plpfilePart);

    //
    // Getting the correct value for plpfilePart requires
    // a strrchr on the converted string.  If this value
    // is needed, just add the code to do it here.
    //

    *plpfilePart = NULL;

    if (cchReturnBuffer == 0)
        dwLen = 2*dwLen;
    else
        dwLen = strReturnBuffer.ConvertExcludingNul();

    return dwLen;
}

HMODULE GetModuleHandleWrap(LPCWSTR lpModuleName)
{
    CStrIn  str(lpModuleName);
    return GetModuleHandleA(str);
}

int GetObjectWrap(HGDIOBJ hgdiObj, int cbBuffer, LPVOID lpvObj)
{
    int nRet;

    if(cbBuffer != sizeof(LOGFONTW))
    {
        nRet = GetObjectA(hgdiObj, cbBuffer, lpvObj);
    }
    else
    {
        LOGFONTA lfa;

        nRet = GetObjectA(hgdiObj, sizeof(lfa), &lfa);
        if (nRet > 0)
        {
            memcpy(lpvObj, &lfa, FIELD_OFFSET(LOGFONTW, lfFaceName));
            UnicodeFromMbcs(((LOGFONTW*)lpvObj)->lfFaceName, ARRAYSIZE(((LOGFONTW*)lpvObj)->lfFaceName),
                            lfa.lfFaceName, -1);
            nRet = sizeof(LOGFONTW);
        }
    }

    return nRet;
}

//--------------------------------------------------------------
//      GetFullPathNameWrap
//--------------------------------------------------------------

DWORD GetFullPathNameWrap( LPCWSTR lpFileName,
                     DWORD  nBufferLength,
                     LPWSTR lpBuffer,
                     LPWSTR *lpFilePart)
{
    CStrIn  strIn(lpFileName);
    CStrOut  strOut(lpBuffer,nBufferLength);
    LPSTR   pFile;
    DWORD   dwRet;

    dwRet = GetFullPathNameA(strIn, nBufferLength, strOut, &pFile);
    strOut.ConvertIncludingNul();
    *lpFilePart = lpBuffer + (pFile - strOut);
    return dwRet;
}

BOOL GetStringTypeExWrap(LCID lcid, DWORD dwInfoType, LPCTSTR lpSrcStr, int cchSrc, LPWORD lpCharType)
{
    CStrIn  str(lpSrcStr, cchSrc);
    return GetStringTypeExA(lcid, dwInfoType, str, str.strlen(), lpCharType);
}

UINT GetPrivateProfileIntWrap(
        LPCWSTR lpAppName,
        LPCWSTR lpKeyName,
        INT     nDefault,
        LPCWSTR lpFileName)
{
    CStrIn  strApp(lpAppName);
    CStrIn  strKey(lpKeyName);
    CStrIn  strFile(lpFileName);

    return GetPrivateProfileIntA(strApp, strKey, nDefault, strFile);
}

UINT GetProfileIntWrap(
        LPCWSTR lpAppName,
        LPCWSTR lpKeyName,
        INT     nDefault)
{
    CStrIn  strApp(lpAppName);
    CStrIn  strKey(lpKeyName);
    
    return GetProfileIntA(strApp, strKey, nDefault);
}

DWORD GetProfileStringWrap(
        LPCWSTR lpAppName,
        LPCWSTR lpKeyName,
        LPCWSTR lpDefault, 
        LPWSTR  lpBuffer, 
        DWORD   dwBuffersize)
{
    CStrIn  strApp(lpAppName);
    CStrIn  strKey(lpKeyName);
    CStrIn  strDefault(lpDefault);
    CStrOut strBuffer(lpBuffer, dwBuffersize);
    
    GetProfileStringA(strApp, strKey, strDefault, strBuffer, dwBuffersize);
    return strBuffer.ConvertIncludingNul();
}



HANDLE GetPropWrap(HWND hWnd, LPCWSTR lpString)
{
    CStrIn  str(lpString);

    return GetPropA(hWnd, str);
}

UINT GetTempFileNameWrap(
        LPCWSTR lpPathName,
        LPCWSTR lpPrefixString,
        UINT    uUnique,
        LPWSTR  lpTempFileName)
{
    CStrIn  strPath(lpPathName);
    CStrIn  strPrefix(lpPrefixString);
    CStrOut strFileName(lpTempFileName, MAX_PATH);

    return GetTempFileNameA(strPath, strPrefix, uUnique, strFileName);
}

DWORD GetTempPathWrap(DWORD nBufferLength, LPWSTR lpBuffer)
{
    CStrOut str(lpBuffer, nBufferLength);

    GetTempPathA(str.BufSize(), str);
    return str.ConvertExcludingNul();
}

#ifndef FONT_LINK
BOOL GetTextExtentPointWrap(
        HDC     hdc,
        LPCWSTR pwsz,
        int     cb,
        LPSIZE  pSize)
{
    CStrIn str(pwsz,cb);

    return GetTextExtentPointA(hdc, str, str.strlen(), pSize);
}

BOOL GetTextExtentPoint32Wrap(
        HDC     hdc,
        LPCWSTR pwsz,
        int     cb,
        LPSIZE  pSize)
{
    CStrIn str(pwsz,cb);

    return GetTextExtentPoint32A(hdc, str, str.strlen(), pSize);
}
#endif

int GetTextFaceWrap(
        HDC    hdc,
        int    cch,
        LPWSTR lpFaceName)
{
    CStrOut str(lpFaceName, cch);

    GetTextFaceA(hdc, str.BufSize(), str);
    return str.ConvertIncludingNul();
}

BOOL GetTextMetricsWrap(HDC hdc, LPTEXTMETRICW lptm)
{
   BOOL         ret;
   TEXTMETRICA  tm;

    ret = GetTextMetricsA(hdc, &tm);

    if (ret)
    {
        lptm->tmHeight              = tm.tmHeight;
        lptm->tmAscent              = tm.tmAscent;
        lptm->tmDescent             = tm.tmDescent;
        lptm->tmInternalLeading     = tm.tmInternalLeading;
        lptm->tmExternalLeading     = tm.tmExternalLeading;
        lptm->tmAveCharWidth        = tm.tmAveCharWidth;
        lptm->tmMaxCharWidth        = tm.tmMaxCharWidth;
        lptm->tmWeight              = tm.tmWeight;
        lptm->tmOverhang            = tm.tmOverhang;
        lptm->tmDigitizedAspectX    = tm.tmDigitizedAspectX;
        lptm->tmDigitizedAspectY    = tm.tmDigitizedAspectY;
        lptm->tmItalic              = tm.tmItalic;
        lptm->tmUnderlined          = tm.tmUnderlined;
        lptm->tmStruckOut           = tm.tmStruckOut;
        lptm->tmPitchAndFamily      = tm.tmPitchAndFamily;
        lptm->tmCharSet             = tm.tmCharSet;

        UnicodeFromMbcs(&lptm->tmFirstChar, 1, (LPSTR) &tm.tmFirstChar, 1);
        UnicodeFromMbcs(&lptm->tmLastChar, 1, (LPSTR) &tm.tmLastChar, 1);
        UnicodeFromMbcs(&lptm->tmDefaultChar, 1, (LPSTR) &tm.tmDefaultChar, 1);
        UnicodeFromMbcs(&lptm->tmBreakChar, 1, (LPSTR) &tm.tmBreakChar, 1);
    }

    return ret;
}

int GetTimeFormatWrap(
        LCID Locale,
        DWORD dwFlags,
        CONST SYSTEMTIME *lpTime,
        LPCWSTR lpFormat,
        LPWSTR lpTimeStr,
        int cchTime)
{
    CStrIn strFormat(lpFormat);
    CStrOut str(lpTimeStr, cchTime);

    ASSERT(cchTime != 0);

    GetTimeFormatA(Locale, dwFlags, lpTime, strFormat, str, str.BufSize());
    return str.ConvertIncludingNul();
}

LONG GetWindowLongWrap(HWND hWnd, int nIndex)
{
    return GetWindowLongA(hWnd, nIndex);
}

int GetWindowTextWrap(HWND hWnd, LPWSTR lpString, int nMaxCount)
{
    CStrOut str(lpString, nMaxCount);

    GetWindowTextA(hWnd, str, str.BufSize());
    return str.ConvertExcludingNul();
}

int GetWindowTextLengthWrap(HWND hWnd)
{
    WCHAR wstr[MAX_PATH];

    return GetWindowTextWrap(hWnd, wstr, ARRAYSIZE(wstr));
}

UINT GetWindowsDirectoryWrap(LPWSTR lpWinPath, UINT cch)
{
    CStrOut str(lpWinPath, cch);

    GetWindowsDirectoryA(str, str.BufSize());

    return str.ConvertExcludingNul();
}

ATOM GlobalAddAtomWrap(LPCWSTR lpString)
{
    CStrIn str(lpString);
 
    return GlobalAddAtomA(str);
}

BOOL GrayStringWrap(
        HDC hDC,
        HBRUSH hBrush,
        GRAYSTRINGPROC lpOutputFunc,
        LPARAM lpData,
        int nCount,
        int x,
        int y,
        int nWidth,
        int nHeight)
{
    CStrIn str((LPWSTR)lpData);
    
    return GrayStringA(hDC, hBrush, lpOutputFunc, (LPARAM)(LPCSTR)str, str.strlen(), x, y, nWidth, nHeight);    
}

LONG ImmGetCompositionStringWrap(HIMC hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen)
{
    int cb = 0;

    if ((dwIndex & GCS_COMPSTR) || (dwIndex & GCS_RESULTSTR))
    {
        if (dwBufLen)
        {
            CStrOut str((LPWSTR)lpBuf, dwBufLen/sizeof(WCHAR) + 1);

            cb = ImmGetCompositionStringA(hIMC, dwIndex, str, str.BufSize());
            *(WCHAR*)((LPSTR)str + cb) = L'\0';
            return str.ConvertExcludingNul() * sizeof(WCHAR);
        }
        else
        {
            LPWSTR lpStr;

            cb = ImmGetCompositionStringA(hIMC, dwIndex, lpBuf, dwBufLen);
            lpStr = (LPWSTR)LocalAlloc(LPTR, (cb + 1) * sizeof(WCHAR));
            if (lpStr)
            {
                CStrOut str(lpStr, cb + 1);

                cb = ImmGetCompositionStringA(hIMC, dwIndex, str, str.BufSize());
                *(WCHAR*)((LPSTR)str + cb) = L'\0';
                cb = str.ConvertExcludingNul() * sizeof(WCHAR);
                LocalFree(lpStr);
                return cb;
            }
        }
    }
    else if (dwIndex & GCS_COMPATTR)
    {
        if (dwBufLen)
        {
            LPSTR lpStr, lpAttr;
            UINT i = 0;
    
            lpStr = (LPSTR)LocalAlloc(LPTR, dwBufLen);
            if (lpStr)
            {
                lpAttr = (LPSTR)LocalAlloc(LPTR, dwBufLen);
                if (lpAttr)
                {
                    LPSTR lpNext = lpStr;

                    cb = ImmGetCompositionStringA(hIMC, GCS_COMPSTR, lpStr, dwBufLen);
                    ImmGetCompositionStringA(hIMC, GCS_COMPATTR, lpAttr, dwBufLen);

                    for (i = 0; (lpNext - lpStr < cb) && (i < dwBufLen); i++)
                    {
                        ((LPSTR)lpBuf)[i] = lpAttr[lpNext - lpStr];
                        lpNext = CharNextA(lpNext);
                    }
                    LocalFree(lpAttr);
                }
                LocalFree(lpStr);
            }
            return i;
        }
    }
    return ImmGetCompositionStringA(hIMC, dwIndex, lpBuf, dwBufLen);
}

LONG ImmSetCompositionStringWrap(HIMC hIMC, DWORD dwIndex, LPVOID lpComp, DWORD dwCompLen, LPVOID lpRead, DWORD dwReadLen)
{
    if (dwIndex & SCS_SETSTR)
    {
        CStrIn str((LPWSTR)lpComp);

        ASSERT(!lpRead);

        return ImmSetCompositionStringA(hIMC, dwIndex, str, str.strlen(), lpRead, dwReadLen);
    }
    return ImmSetCompositionStringA(hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen);
}

BOOL InsertMenuWrap(
        HMENU   hMenu,
        UINT    uPosition,
        UINT    uFlags,
        UINT    uIDNewItem,
        LPCWSTR lpNewItem)
{
    CStrIn  str(lpNewItem);

    return InsertMenuA(hMenu, uPosition, uFlags, uIDNewItem, str);
}

BOOL IsDialogMessageWrap(HWND hWndDlg, LPMSG lpMsg)
{
    return IsDialogMessageA(hWndDlg, lpMsg);
}

HACCEL LoadAcceleratorsWrap(HINSTANCE hInstance, LPCWSTR lpTableName)
{
    CStrIn str(lpTableName);

    return LoadAcceleratorsA(hInstance, (LPCSTR) str);
}

HBITMAP LoadBitmapWrap(HINSTANCE hInstance, LPCWSTR lpBitmapName)
{
    CStrIn str(lpBitmapName);

    return LoadBitmapA(hInstance, str);
}

HCURSOR LoadCursorWrap(HINSTANCE hInstance, LPCWSTR lpCursorName)
{
    CStrIn str(lpCursorName);

    return LoadCursorA(hInstance, (LPCSTR) str);
}

HICON LoadIconWrap(HINSTANCE hInstance, LPCWSTR lpIconName)
{
    CStrIn str(lpIconName);

    return LoadIconA(hInstance, str);
}

HANDLE LoadImageWrap(
        HINSTANCE hInstance,
        LPCWSTR lpName,
        UINT uType,
        int cxDesired,
        int cyDesired,
        UINT fuLoad)
{
    CStrIn str(lpName);

    return LoadImageA(
            hInstance,
            str,
            uType,
            cxDesired,
            cyDesired,
            fuLoad);
}

HINSTANCE LoadLibraryWrap(LPCWSTR lpLibFileName)
{
    CStrIn  str(lpLibFileName);

    return LoadLibraryA(str);
}

HINSTANCE LoadLibraryExWrap(
        LPCWSTR lpLibFileName,
        HANDLE  hFile,
        DWORD   dwFlags)
{
    CStrIn  str(lpLibFileName);

    return LoadLibraryExA(str, hFile, dwFlags);
}

HMENU LoadMenuWrap(HINSTANCE hInstance, LPCWSTR lpMenuName)
{
    ASSERT(HIWORD64(lpMenuName) == 0);

    return LoadMenuA(hInstance, (LPCSTR) lpMenuName);
}

int LoadStringWrap(HINSTANCE hInstance, UINT uID, LPWSTR lpBuffer, int nBufferMax)
{
    //
    //  Do it manually.  The old code used to call LoadStringA and then
    //  convert it up to unicode, which is stupid since resources are
    //  physically already Unicode!  Just suck it out directly.
    //

    if (nBufferMax <= 0) return 0;                  // sanity check

    PWCHAR pwch;

    /*
     *  String tables are broken up into "bundles" of 16 strings each.
     */
    HRSRC hrsrc;
    int cwch = 0;

    hrsrc = FindResourceA(hInstance, (LPSTR)(LONG_PTR)(1 + uID / 16), (LPSTR)RT_STRING);
    if (hrsrc) {
        pwch = (PWCHAR)LoadResource(hInstance, hrsrc);
        if (pwch) {
            /*
             *  Now skip over the strings in the resource until we
             *  hit the one we want.  Each entry is a counted string,
             *  just like Pascal.
             */
            for (uID %= 16; uID; uID--) {
                pwch += *pwch + 1;
            }
            cwch = min(*pwch, nBufferMax - 1);
            memcpy(lpBuffer, pwch+1, cwch * sizeof(WCHAR)); /* Copy the goo */
        }
    }
    lpBuffer[cwch] = L'\0';                 /* Terminate the string */
    return cwch;
}

UINT MapVirtualKeyWrap(UINT uCode, UINT uMapType)
{
    return MapVirtualKeyA(uCode, uMapType);
}

//----------------------------------------------------------------------
//
// function:    TransformCharNoOp1( WCHAR **ppch )
//
// purpose:     Stand-in for TransformCharWidth.  Used by the function
//              CompareStringString.
//
// returns:     Character at *ppch.  The value *ppch is incremented.
//
//----------------------------------------------------------------------

static WCHAR TransformCharNoOp1( LPCWSTR *ppch, int )
{
    WCHAR ch = **ppch;

    (*ppch)++;

    return ch;
}

//----------------------------------------------------------------------
//
// function:    TransformCharWidth( WCHAR **ppch, cchRemaining )
//
// purpose:     Converts halfwidth characters to fullwidth characters.
//              Also combines voiced (dakuon) and semi-voiced (handakuon)
//              characters.  *pch is advanced by one, unless there is a
//              (semi)voiced character, in which case it is advanced by
//              two characters.
//
//              Note that unlike the full widechar version, we do not
//              combine other characters, notably the combining Hiragana
//              characters (U+3099 and U+309A.)  This is to keep the
//              tables from getting unnecessarily large.
//
//              cchRemaining is passed so as to not include the voiced
//              marks if it's passed the end of the specified buffer.
//
// returns:     Full width character. *pch is incremented.
//
//----------------------------------------------------------------------

static WCHAR TransformCharWidth( LPCWSTR *ppch, int cchRemaining )
{
    WCHAR ch = **ppch;

    (*ppch)++;

    if (ch == 0x0020)
    {
        ch = 0x3000;
    }
    else if (ch == 0x005c)
    {
        // REVERSE SOLIDUS (aka BACKSLASH) maps to itself
    }
    else if (InRange(ch, 0x0021, 0x07e))
    {
        ch += 65248;
    }
    else if (InRange(ch, 0x00a2, 0x00af))
    {
        static const WCHAR achFull[] =
        {
            0xffe0, 0xffe1, 0x00a4, 0xffe5, 0xffe4, 0x00a7, 0x00a8, // 0xa2-0xa8
            0x00a9, 0x00aa, 0x00ab, 0xffe2, 0x00ad, 0x00ae, 0xffe3  // 0xa9-0xaf
        };

        ch = achFull[ch - 0x00a2];
    }
    else if (ch == 0x20a9) // WON SIGN
    {
        ch = 0xffe6;
    }
    else if (InRange(ch, 0xff61, 0xffdc))
    {
        WCHAR chNext = (cchRemaining > 1) ? **ppch : 0;

        if (chNext == 0xff9e && InRange(ch, 0xff73, 0xff8e))
        {
            if (cchRemaining != 1)
            {
                static const WCHAR achFull[] =
                {
/* 0xff73-0xff79 */  0xb0f4, 0x30a8, 0x30aa, 0xb0ac, 0xb0ae, 0xb0b0, 0xb0b2,                  
/* 0xff7a-0xff80 */  0xb0b4, 0xb0b6, 0xb0b8, 0xb0ba, 0xb0bc, 0xb0be, 0xb0c0,                  
/* 0xff81-0xff87 */  0xb0c2, 0xb0c5, 0xb0c7, 0xb0c9, 0x30ca, 0x30cb, 0x30cc,                  
/* 0xff88-0xff8e */  0x30cd, 0x30ce, 0xb0d0, 0xb0d3, 0xb0d6, 0xb0d9, 0xb0dc                  
                };

                // HALFWIDTH KATAKANA VOICED SOUND MARK

                WCHAR chTemp = achFull[ch - 0xff73];

                // Some in the range absorb the sound mark.
                // These are indicated by the set high-bit.

                ch = chTemp & 0x7fff;

                if (chTemp & 0x8000)
                {
                    (*ppch)++;
                }
            }
        }
        else if (chNext == 0xff9f && InRange(ch, 0xff8a, 0xff8e))
        {
            // HALFWIDTH KATAKANA SEMI-VOICED SOUND MARK

            ch = 0x30d1 + (ch - 0xff8a) * 3;
            (*ppch)++;
        }
        else
        {
            static const WCHAR achMapFullFFxx[] =
            {
                0x3002, 0x300c, 0x300d, 0x3001, 0x30fb, 0x30f2, 0x30a1,  // 0xff61-0xff67
                0x30a3, 0x30a5, 0x30a7, 0x30a9, 0x30e3, 0x30e5, 0x30e7,  // 0xff68-0xff6e
                0x30c3, 0x30fc, 0x30a2, 0x30a4, 0x30a6, 0x30a8, 0x30aa,  // 0xff6f-0xff75
                0x30ab, 0x30ad, 0x30af, 0x30b1, 0x30b3, 0x30b5, 0x30b7,  // 0xff76-0xff7c
                0x30b9, 0x30bb, 0x30bd, 0x30bf, 0x30c1, 0x30c4, 0x30c6,  // 0xff7d-0xff83
                0x30c8, 0x30ca, 0x30cb, 0x30cc, 0x30cd, 0x30ce, 0x30cf,  // 0xff84-0xff8a
                0x30d2, 0x30d5, 0x30d8, 0x30db, 0x30de, 0x30df, 0x30e0,  // 0xff8b-0xff91
                0x30e1, 0x30e2, 0x30e4, 0x30e6, 0x30e8, 0x30e9, 0x30ea,  // 0xff92-0xff98
                0x30eb, 0x30ec, 0x30ed, 0x30ef, 0x30f3, 0x309b, 0x309c,  // 0xff99-0xff9f
                0x3164, 0x3131, 0x3132, 0x3133, 0x3134, 0x3135, 0x3136,  // 0xffa0-0xffa6
                0x3137, 0x3138, 0x3139, 0x313a, 0x313b, 0x313c, 0x313d,  // 0xffa7-0xffad
                0x313e, 0x313f, 0x3140, 0x3141, 0x3142, 0x3143, 0x3144,  // 0xffae-0xffb4
                0x3145, 0x3146, 0x3147, 0x3148, 0x3149, 0x314a, 0x314b,  // 0xffb5-0xffbb
                0x314c, 0x314d, 0x314e, 0xffbf, 0xffc0, 0xffc1, 0x314f,  // 0xffbc-0xffc2
                0x3150, 0x3151, 0x3152, 0x3153, 0x3154, 0xffc8, 0xffc9,  // 0xffc3-0xffc9
                0x3155, 0x3156, 0x3157, 0x3158, 0x3159, 0x315a, 0xffd0,  // 0xffca-0xffd0
                0xffd1, 0x315b, 0x315c, 0x315d, 0x315e, 0x315f, 0x3160,  // 0xffd1-0xffd7
                0xffd8, 0xffd9, 0x3161, 0x3162, 0x3163                   // 0xffd8-0xffac
            };

            ch = achMapFullFFxx[ch - 0xff61];
        }
    }

    return ch;
}

//----------------------------------------------------------------------
//
// function:    TransformaCharNoOp2( WCHAR ch )
//
// purpose:     Stand-in for CharLowerBuffWrap.  Used by the function
//              CompareStringString.
//
// returns:     Original character
//
//----------------------------------------------------------------------

static WCHAR TransformCharNoOp2( WCHAR ch )
{
    return ch;
}

//----------------------------------------------------------------------
//
// function:    TransformaCharKana( WCHAR ch )
//
// purpose:     Converts Hiragana characters to Katakana characters
//
// returns:     Original character if not Hiragana,
//              Katanaka character if Hiragana
//
//----------------------------------------------------------------------

static WCHAR TransformCharKana( WCHAR ch )
{
    if (((ch & 0xff00) == 0x3000) &&
        (InRange(ch, 0x3041, 0x3094) || InRange(ch, 0x309d, 0x309e)))
    {
        ch += 0x060;
    }

    return ch;
}

//----------------------------------------------------------------------
//
// function:    TransformCharNoOp3( LPWSTR pch, DWORD cch )
//
// purpose:     Stand-in for CharLowerBuffWrap.  Used by the function
//              CompareStringString.
//
// returns:     Character count (cch).
//
//----------------------------------------------------------------------

static DWORD TransformCharNoOp3( LPWSTR, DWORD cch )
{
    return cch;
}

//----------------------------------------------------------------------
//
// function:    TransformaCharFinal( WCHAR ch )
//
// purpose:     Converts "final" forms to regular forms
//
// returns:     Original character if not Hiragana,
//              Katanaka character if Hiragana
//
//----------------------------------------------------------------------

// BUGBUG (cthrash) We do not fold Presentation Forms (Alphabetic or Arabic)

static WCHAR TransformCharFinal( WCHAR ch )
{
    WCHAR chRet = ch;
    
    if (ch >= 0x3c2)                    // short-circuit ASCII +
    {
        switch (ch)
        {
            case 0x03c2:                // GREEK SMALL LETTER FINAL SIGMA
            case 0x05da:                // HEBREW LETTER FINAL KAF
            case 0x05dd:                // HEBREW LETTER FINAL MEM
            case 0x05df:                // HEBREW LETTER FINAL NUN
            case 0x05e3:                // HEBREW LETTER FINAL PE
            case 0x05e5:                // HEBREW LETTER FINAL TSADI
            case 0xfb26:                // HEBREW LETTER WIDE FINAL MEM
            case 0xfb3a:                // HEBREW LETTER FINAL KAF WITH DAGESH
            case 0xfb43:                // HEBREW LETTER FINAL PE WITH DAGESH
                chRet++;
                break;
        }
    }

    return ch;
}

//----------------------------------------------------------------------
//
// function:    CompareStringString( ... )
//
// purpose:     Helper for CompareStringWrap.
//
//              We handle the string comparsion for CompareStringWrap.
//              We can convert each character to (1) fullwidth,
//              (2) Katakana, and (3) lowercase, as necessary.
//
// returns:     1 - string A is less in lexical value as string B
//              2 - string B is equal in lexical value as string B
//              3 - string B is greater in lexical value as string B
//
//----------------------------------------------------------------------

static int CompareStringString(
    DWORD   dwFlags,
    LPCWSTR lpA,
    int     cchA,
    LPCWSTR lpB,
    int     cchB )
{
    int nRet = 0;
    WCHAR wchIgnoreNulA = cchA == -1 ? 0 : -1;
    WCHAR wchIgnoreNulB = cchB == -1 ? 0 : -1;
    WCHAR (*pfnTransformWidth)(LPCWSTR *, int);
    WCHAR (*pfnTransformKana)(WCHAR);
    DWORD (*pfnTransformLower)(LPWSTR, DWORD);
    WCHAR (*pfnTransformFinal)(WCHAR);
    

    pfnTransformWidth = (dwFlags & NORM_IGNOREWIDTH)
                        ? TransformCharWidth : TransformCharNoOp1;
    pfnTransformKana  = (dwFlags & NORM_IGNOREKANATYPE)
                        ? TransformCharKana : TransformCharNoOp2;
    pfnTransformLower = (dwFlags & NORM_IGNORECASE)
                        ? CharLowerBuffWrap : TransformCharNoOp3;
    pfnTransformFinal = (dwFlags & NORM_IGNORECASE)
                        ? TransformCharFinal : TransformCharNoOp2;

    while (   !nRet
           && cchA
           && cchB
           && (*lpA | wchIgnoreNulA)
           && (*lpB | wchIgnoreNulB) )
    {
        WCHAR chA, chB;
        LPCWSTR lpAOld = lpA;
        LPCWSTR lpBOld = lpB;

        chA = (*pfnTransformWidth)(&lpA, cchA);
        chA = (*pfnTransformKana)(chA);
        (*pfnTransformLower)(&chA, 1);
        chA = (*pfnTransformFinal)(chA);

        chB = (*pfnTransformWidth)(&lpB, cchB);
        chB = (*pfnTransformKana)(chB);
        (*pfnTransformLower)(&chB, 1);
        chB = (*pfnTransformFinal)(chB);

        nRet = (int)chA - (int)chB;
        cchA -= (int) (lpA-lpAOld);
        cchB -= (int) (lpB-lpBOld);
    }

    if (!nRet)
    {
        nRet = cchA - cchB;
    }

    if (nRet)
    {
        nRet = nRet > 0 ? 1 : -1;
    }

    return nRet + 2;
}

//----------------------------------------------------------------------
//
// function:    CompareStringWord( ... )
//
// purpose:     Helper for CompareStringWrap.
//
//              We handle the word comparsion for CompareStringWrap.
//
// returns:     1 - string A is less in lexical value as string B
//              2 - string B is equal in lexical value as string B
//              3 - string B is greater in lexical value as string B
//
//----------------------------------------------------------------------

static int CompareStringWord(
    LCID    lcid,
    DWORD   dwFlags,
    LPCWSTR lpA,
    int     cchA,
    LPCWSTR lpB,
    int     cchB )
{
    // BUGBUG (cthrash) We won't properly support word compare for the
    // time being.  Do the same old CP_ACP trick, which should cover
    // enough cases.

    // fail if either string is NULL, as it causes assert on debug windows
    if (!lpA || !lpB)
        return 0;

    CStrIn strA(lpA, cchA);
    CStrIn strB(lpB, cchB);

    cchA = strA.strlen();
    cchB = strB.strlen();

    return CompareStringA(lcid, dwFlags, strA, cchA, strB, cchB);
}

//----------------------------------------------------------------------
//
// function:    CompareStringWrap( ... )
//
// purpose:     Unicode wrapper of CompareString for Win95.
//
//              Note not all bits in dwFlags are honored; specifically,
//              since we don't do a true widechar word compare, we
//              won't properly handle NORM_IGNORENONSPACE or
//              NORM_IGNORESYMBOLS for arbitrary widechar strings.
//
// returns:     1 - string A is less in lexical value as string B
//              2 - string B is equal in lexical value as string B
//              3 - string B is greater in lexical value as string B
//
//----------------------------------------------------------------------

LWSTDAPI_(int) CompareStringAltW(
    LCID    lcid,
    DWORD   dwFlags,
    LPCWSTR lpA,
    int     cchA,
    LPCWSTR lpB,
    int     cchB )
{
    int nRet;

    if (dwFlags & SORT_STRINGSORT)
    {
        nRet = CompareStringString(dwFlags, lpA, cchA, lpB, cchB);
    }
    else
    {
        nRet = CompareStringWord(lcid, dwFlags, lpA, cchA, lpB, cchB);
    }

    return nRet;
}

int CompareStringWrap(
    LCID     Locale,
    DWORD    dwCmpFlags,
    LPCWSTR lpString1,
    int      cchCount1,
    LPCWSTR lpString2,
    int      cchCount2)
{
    // fail if either string is NULL, as it causes assert on debug windows
    if (!lpString1 || !lpString2)
        return 0;
    
    CStrIn      strString1(lpString1, cchCount1);
    CStrIn      strString2(lpString2, cchCount2);

    cchCount1 = strString1.strlen();

    cchCount2 = strString2.strlen();


    return CompareStringA(Locale, dwCmpFlags,
                          strString1,cchCount1,
                          strString2,cchCount2);
}

BOOL MessageBoxIndirectWrap(MSGBOXPARAMS *pmbp)
{
    CStrIn        strText(pmbp->lpszText);
    CStrIn        strCaption(pmbp->lpszCaption);
    MSGBOXPARAMSA mbp;

    memcpy(&mbp, pmbp, sizeof(mbp));
    mbp.lpszText = strText;
    mbp.lpszCaption = strCaption;
    ASSERT(HIWORD64(mbp.lpszIcon) == 0);

    return MessageBoxIndirectA(&mbp);
}

DWORD GetCharacterPlacementWrap(
    HDC hdc,            // handle to device context
    LPCTSTR lpString,   // pointer to string
    int nCount,         // number of characters in string
    int nMaxExtent,     // maximum extent for displayed string
    LPGCP_RESULTS lpResults, // pointer to buffer for placement result
    DWORD dwFlags       // placement flags
   )
{
    CStrIn strText(lpString);
    DWORD dwRet;

    // Leave for someone else.
    ASSERT (lpResults->lpOutString == NULL);
    ASSERT (lpResults->lpClass == NULL);

    dwRet = GetCharacterPlacementA (hdc, strText, nCount, nMaxExtent,
                                    (LPGCP_RESULTSA)lpResults,
                                    dwFlags);
    return dwRet;
}

#ifndef FONT_LINK
BOOL GetCharWidthWrap (
     HDC hdc,
     UINT iFirstChar,
     UINT iLastChar,
     LPINT lpBuffer)
{
    // Note that we expect to do only one character at a time for anything but
    // ISO Latin 1.
    if (iFirstChar > 255)
    {
        UINT mbChar=0;
        WCHAR ch;

        // Convert string
        ch = (WCHAR)iFirstChar;
        WideCharToMultiByte(CP_ACP, 0, &ch, 1,
                            (char *)&mbChar, 2, NULL, NULL);
    }

    return (GetCharWidthA (hdc, iFirstChar, iLastChar, lpBuffer));
}
#endif

BOOL ModifyMenuWrap(
        HMENU   hMenu,
        UINT    uPosition,
        UINT    uFlags,
        UINT    uIDNewItem,
        LPCWSTR lpNewItem)
{
    ASSERT(!(uFlags & MF_BITMAP) && !(uFlags & MF_OWNERDRAW));

    CStrIn  str(lpNewItem);

    return ModifyMenuA(hMenu, uPosition, uFlags, uIDNewItem, str);
}

BOOL CopyFileWrap(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, BOOL bFailIfExists)
{
    CStrIn  strOld(lpExistingFileName);
    CStrIn  strNew(lpNewFileName);

    return CopyFileA(strOld, strNew, bFailIfExists);
}

BOOL MoveFileWrap(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName)
{
    CStrIn  strOld(lpExistingFileName);
    CStrIn  strNew(lpNewFileName);

    return MoveFileA(strOld, strNew);
}

BOOL OemToCharWrap(LPCSTR lpszSrc, LPWSTR lpszDst)
{
    CStrOut strDst(lpszDst, lstrlenA(lpszSrc));

    return OemToCharA(lpszSrc, strDst);
}

VOID OutputDebugStringWrap(LPCWSTR lpOutputString)
{
    CStrIn  str(lpOutputString);

    OutputDebugStringA(str);
}

BOOL PeekMessageWrap(
        LPMSG   lpMsg,
        HWND    hWnd,
        UINT    wMsgFilterMin,
        UINT    wMsgFilterMax,
        UINT    wRemoveMsg)
{
    return PeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
}

BOOL PostMessageWrap(
        HWND    hWnd,
        UINT    Msg,
        WPARAM  wParam,
        LPARAM  lParam)
{
    return PostMessageA(hWnd, Msg, wParam, lParam);
}

BOOL PostThreadMessageWrap(
        DWORD idThread,
        UINT Msg,
        WPARAM wParam,
        LPARAM lParam)
{
    return PostThreadMessageA(idThread, Msg, wParam, lParam);
}

LONG RegCreateKeyWrap(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult)
{
    CStrIn  str(lpSubKey);

    return RegCreateKeyA(hKey, str, phkResult);
}

LONG RegCreateKeyExWrap(HKEY hKey, LPCTSTR lpSubKey, DWORD Reserved, LPTSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition)
{
    CStrIn strSubKey(lpSubKey);
    CStrIn strClass(lpClass);

    return RegCreateKeyExA(hKey, strSubKey, Reserved, strClass, dwOptions, samDesired, lpSecurityAttributes,  phkResult, lpdwDisposition);
}

LONG RegDeleteKeyWrap(HKEY hKey, LPCWSTR pwszSubKey)
{
    CStrIn  str(pwszSubKey);

    return RegDeleteKeyA(hKey, str);
}

LONG RegEnumKeyWrap(
        HKEY    hKey,
        DWORD   dwIndex,
        LPWSTR  lpName,
        DWORD   cbName)
{
    CStrOut str(lpName, cbName);

    return RegEnumKeyA(hKey, dwIndex, str, str.BufSize());
}

LONG RegEnumKeyExWrap(
        HKEY        hKey,
        DWORD       dwIndex,
        LPWSTR      lpName,
        LPDWORD     lpcbName,
        LPDWORD     lpReserved,
        LPWSTR      lpClass,
        LPDWORD     lpcbClass,
        PFILETIME   lpftLastWriteTime)
{
    long    ret;
    DWORD   dwClass = 0;

    if (!lpcbClass)
    {
        lpcbClass = &dwClass;
    }

    CStrOut strName(lpName, *lpcbName);
    CStrOut strClass(lpClass, *lpcbClass);

    ret = RegEnumKeyExA(
            hKey,
            dwIndex,
            strName,
            lpcbName,
            lpReserved,
            strClass,
            lpcbClass,
            lpftLastWriteTime);

    *lpcbName = strName.ConvertExcludingNul();
    *lpcbClass = strClass.ConvertExcludingNul();

    return ret;
}

LONG RegOpenKeyWrap(HKEY hKey, LPCWSTR pwszSubKey, PHKEY phkResult)
{
    CStrIn  str(pwszSubKey);

    return RegOpenKeyA(hKey, str, phkResult);
}

LONG RegOpenKeyExWrap(
        HKEY    hKey,
        LPCWSTR lpSubKey,
        DWORD   ulOptions,
        REGSAM  samDesired,
        PHKEY   phkResult)
{
    CStrIn  str(lpSubKey);

    return RegOpenKeyExA(hKey, str, ulOptions, samDesired, phkResult);
}

LONG RegQueryInfoKeyWrap(
        HKEY hKey,
        LPWSTR lpClass,
        LPDWORD lpcbClass,
        LPDWORD lpReserved,
        LPDWORD lpcSubKeys,
        LPDWORD lpcbMaxSubKeyLen,
        LPDWORD lpcbMaxClassLen,
        LPDWORD lpcValues,
        LPDWORD lpcbMaxValueNameLen,
        LPDWORD lpcbMaxValueLen,
        LPDWORD lpcbSecurityDescriptor,
        PFILETIME lpftLastWriteTime)
{
    CStrIn  str(lpClass);

    return RegQueryInfoKeyA(
                hKey,
                str,
                lpcbClass,
                lpReserved,
                lpcSubKeys,
                lpcbMaxSubKeyLen,
                lpcbMaxClassLen,
                lpcValues,
                lpcbMaxValueNameLen,
                lpcbMaxValueLen,
                lpcbSecurityDescriptor,
                lpftLastWriteTime);
}

LONG RegQueryValueWrap(
        HKEY    hKey,
        LPCWSTR pwszSubKey,
        LPWSTR  pwszValue,
        PLONG   lpcbValue)
{
    long    ret;
    long    cb;
    CStrIn  strKey(pwszSubKey);
    CStrOut strValue(pwszValue, (*lpcbValue) / sizeof(WCHAR));

    cb = strValue.BufSize();
    ret = RegQueryValueA(hKey, strKey, strValue, &cb);
    if (ret != ERROR_SUCCESS)
        goto Cleanup;

    if (strValue)
    {
        cb = strValue.ConvertIncludingNul();
    }

    *lpcbValue = cb * sizeof(WCHAR);

Cleanup:
    return ret;
}

LONG RegQueryValueExWrap(
        HKEY    hKey,
        LPCWSTR lpValueName,
        LPDWORD lpReserved,
        LPDWORD lpType,
        LPBYTE  lpData,
        LPDWORD lpcbData)
{
    LONG    ret;
    CStrIn  strValueName(lpValueName);
    DWORD   dwTempType;
    DWORD   cb;

    //
    // Determine the type of buffer needed
    //

    ret = RegQueryValueExA(hKey, strValueName, lpReserved, &dwTempType, NULL, &cb);
    if (ret != ERROR_SUCCESS)
        goto Cleanup;

    ASSERT(dwTempType != REG_MULTI_SZ);

    switch (dwTempType)
    {
    case REG_EXPAND_SZ:
    case REG_SZ:
        {
            CStrOut strData((LPWSTR) lpData, (*lpcbData) / sizeof(WCHAR));

            cb = strData.BufSize();
            ret = RegQueryValueExA(hKey, strValueName, lpReserved, lpType, (LPBYTE)(LPSTR)strData, &cb);
            if (ret != ERROR_SUCCESS)
                break;

            if (strData)
            {
                cb = strData.ConvertIncludingNul();
            }

            *lpcbData = cb * sizeof(WCHAR);
            break;
        }

    default:
        {
            ret = RegQueryValueExA(
                    hKey,
                    strValueName,
                    lpReserved,
                    lpType,
                    lpData,
                    lpcbData);

            break;
        }
    }

Cleanup:
    return ret;
}

LONG RegSetValueWrap(
        HKEY    hKey,
        LPCWSTR lpSubKey,
        DWORD   dwType,
        LPCWSTR lpData,
        DWORD   cbData)
{
    CStrIn  strKey(lpSubKey);
    CStrIn  strValue(lpData);

    return RegSetValueA(hKey, strKey, dwType, strValue, cbData);
}

LONG RegSetValueExWrap(
        HKEY        hKey,
        LPCWSTR     lpValueName,
        DWORD       Reserved,
        DWORD       dwType,
        CONST BYTE* lpData,
        DWORD       cbData)
{
    ASSERT(dwType != REG_MULTI_SZ);

    CStrIn      strKey(lpValueName);
    CStrIn      strSZ((dwType == REG_SZ || dwType == REG_EXPAND_SZ) ? (LPCWSTR) lpData : NULL);

    if (strSZ)
    {
        lpData = (LPBYTE) (LPSTR) strSZ;
        cbData = strSZ.strlen() + 1;
    }

    return RegSetValueExA(
            hKey,
            strKey,
            Reserved,
            dwType,
            lpData,
            cbData);
}

ATOM RegisterClassWrap(CONST WNDCLASSW * lpWndClass)
{
    WNDCLASSA   wc;
    CStrIn      strMenuName(lpWndClass->lpszMenuName);
    CStrIn      strClassName(lpWndClass->lpszClassName);

    ASSERT(sizeof(wc) == sizeof(*lpWndClass));
    memcpy(&wc, lpWndClass, sizeof(wc));

    wc.lpszMenuName = strMenuName;
    wc.lpszClassName = strClassName;

    return RegisterClassA(&wc);
}

UINT RegisterClipboardFormatWrap(LPCWSTR lpString)
{
    CStrIn  str(lpString);

    return RegisterClipboardFormatA(str);
}

UINT RegisterWindowMessageWrap(LPCWSTR lpString)
{
    CStrIn  str(lpString);

    return RegisterWindowMessageA(str);
}

HANDLE RemovePropWrap(
        HWND    hWnd,
        LPCWSTR lpString)
{
    CStrIn  str(lpString);

    return RemovePropA(hWnd, str);
}

//  NOTE (SumitC) Instead of calling SendDlgItemMessageA below, I'm forwarding to
//       SendMessageWrap so as not to have to re-do the special-case processing.
LRESULT SendDlgItemMessageWrap(
        HWND    hDlg,
        int     nIDDlgItem,
        UINT    Msg,
        WPARAM  wParam,
        LPARAM  lParam)
{
    HWND hWnd;

    hWnd = GetDlgItem(hDlg, nIDDlgItem);

    return SendMessageWrap(hWnd, Msg, wParam, lParam);
}

// AdjustECPosition
//
// Convert mulitbyte position to unicode number of character position in EDIT control
//
// iType:   ADJUST_TO_WCHAR_POS or ADJUST_TO_CHAR_POS
//
#define ADJUST_TO_WCHAR_POS     0
#define ADJUST_TO_CHAR_POS      1

int AdjustECPosition(char *psz, int iPos, int iType)
{
    char *pstr = psz;
    int iNewPos = iPos;

    if (ADJUST_TO_WCHAR_POS == iType)
    {
        iNewPos = 0;
        while (*pstr && (pstr - psz != iPos))
        {
            pstr = CharNextA(pstr);
            iNewPos++;
        }
    }
    else if (ADJUST_TO_CHAR_POS == iType)
    {
        while (*pstr && iPos--)
            pstr = CharNextA(pstr);
        iNewPos = (int) (pstr-psz);
    }
    return iNewPos;
}

//
//  Edit controls can get really huge, so the MAX_PATH buffer in
//  SendMessageWrap just doesn't cut it when push comes to shove.
//
//  Try to use the small buffer, and switch to an allocated buffer
//  only if the small buffer doesn't work.
//
//  Use the handy CConvertStr class as our basis.
//

class CStrA : public CConvertStr
{
public:
    CStrA(int cch);
    inline int bufsize() { return _cchLen; }

protected:
    int _cchLen;
};

CStrA::CStrA(int cch) : CConvertStr(CP_ACP)
{
    _cchLen = cch;

    if (cch <= ARRAYSIZE(_ach))
    {
        // It fits in our small buffer
        _pstr = _ach;
    }
    else
    {
        // Need to allocate a big buffer
        _pstr = new char[cch];
        if (!_pstr)
        {
            // On failure, use the small buffer after all.
            _pstr = _ach;
            _cchLen = ARRAYSIZE(_ach);
        }
    }
}

LRESULT SendEditMessageWrap(
        HWND    hWnd,
        UINT    Msg,
        WPARAM  wParam,
        LPARAM  lParam)
{
    WORD wStart, wEnd;
    DWORD dwPos;

    //
    //  EM_SETSEL is special - We can often handle it without having to
    //  get the client text, which is good since some clients are stupid
    //  and return bogus data.
    //
    //  If the start and end positions are both either 0 or -1, then we
    //  don't need to do adjustment since 0 is always 0 and -1 is always -1.
    //
    if (Msg == EM_SETSEL) {
        if ((wParam == 0 || (DWORD)wParam == 0xFFFFFFFF) &&
            (lParam == 0 || (DWORD)lParam == 0xFFFFFFFF))
        return SendMessageA(hWnd, Msg, wParam, lParam);
    }

    // Get the current window text, since we will be studying it
    CStrA sz(GetWindowTextLengthA(hWnd) + 1);
    GetWindowTextA(hWnd, sz, sz.bufsize());

    switch (Msg)
    {
    case EM_GETSEL:
        {
            DWORD_PTR dwPos;
            
            dwPos = SendMessageA(hWnd, Msg, wParam, lParam);
            wStart = (WORD)AdjustECPosition(sz, GET_X_LPARAM(dwPos), ADJUST_TO_WCHAR_POS);
            wEnd = (WORD)AdjustECPosition(sz, GET_Y_LPARAM(dwPos), ADJUST_TO_WCHAR_POS);
            return MAKELONG(wStart, wEnd);
        }

    case EM_SETSEL:
        wStart = (WORD)AdjustECPosition(sz, wParam, ADJUST_TO_CHAR_POS);
        wEnd = (WORD)AdjustECPosition(sz, lParam, ADJUST_TO_CHAR_POS);
        return SendMessageA(hWnd, Msg, wStart, wEnd);

    case EM_LINEINDEX:
        dwPos = SendMessageA(hWnd, Msg, wParam, lParam);
        return AdjustECPosition(sz, dwPos, ADJUST_TO_WCHAR_POS);

    case EM_LINELENGTH:
        wStart = (WORD)AdjustECPosition(sz, wParam, ADJUST_TO_CHAR_POS);
        dwPos = SendMessageA(hWnd, Msg, wStart, lParam);
        return AdjustECPosition(sz + wStart, dwPos, ADJUST_TO_WCHAR_POS);

    case EM_LINEFROMCHAR:
    case EM_POSFROMCHAR:
        wStart = (WORD)AdjustECPosition(sz, wParam, ADJUST_TO_CHAR_POS);
        return SendMessageA(hWnd, Msg, wStart, lParam);

    default:
        AssertMsg(FALSE, TEXT("error: unknown message leaked into SendEditMessageWrap"));
        return SendMessageA(hWnd, Msg, wParam, lParam);

    }
}


#ifndef UNIX
  #define SHLWAPI_SENDMESSAGEWRAPW_ORD      136
#else
  #define SHLWAPI_SENDMESSAGEWRAPW_ORD      "SendMessageWrapW"
#endif

typedef LRESULT (* PFNSENDMESSAGEWRAPW)(HWND, UINT, WPARAM, LPARAM);

LRESULT SendMessageWrap(
        HWND    hWnd,
        UINT    Msg,
        WPARAM  wParam,
        LPARAM  lParam)
{
    // For XCP PlugUI:
    //     1)If shlwapi is in memory, ask it to do the work, otherwise let it
    //          fall through to original comctl32 wrap implementation.
    //     2)This implementation only apply to LB for fixing #67837
    switch (Msg)
    {
        case LB_ADDSTRING:
        case LB_FINDSTRING:
        case LB_FINDSTRINGEXACT:
        case LB_INSERTSTRING:
        case LB_GETTEXT:
        case LB_GETTEXTLEN:
        case LB_SELECTSTRING:
        {
            extern HMODULE GetShlwapiHModule();
            PFNSENDMESSAGEWRAPW pfnSndMsgWrapW = NULL;
            HMODULE hShlwapi;

            hShlwapi = GetShlwapiHModule();
            if (hShlwapi)
                pfnSndMsgWrapW = (PFNSENDMESSAGEWRAPW)GetProcAddress(hShlwapi, (LPCSTR)SHLWAPI_SENDMESSAGEWRAPW_ORD);

            if (pfnSndMsgWrapW)
                return pfnSndMsgWrapW(hWnd, Msg, wParam, lParam);
            else
                break;   // fall through the regular comctl32's wrap
        }
    }

    // original comctl32's wrap implementation
    CHAR sz[MAX_PATH];  // BUGBUG: It's big enough current comctl32 usage until now ...

    switch (Msg)
    {
    case WM_GETTEXT:
    {
        CStrOut str((LPWSTR)lParam, (int) wParam);
        SendMessageA(hWnd, Msg, (WPARAM) str.BufSize(), (LPARAM) (LPSTR) str);
        return str.ConvertExcludingNul();
    }

    // The sz[] buffer is not large enough for these guys, so use a
    // separate helper function.
    case EM_GETSEL:
    case EM_SETSEL:
    case EM_LINEINDEX:
    case EM_LINELENGTH:
    case EM_LINEFROMCHAR:
    case EM_POSFROMCHAR:
        return SendEditMessageWrap(hWnd, Msg, wParam, lParam);

    // BUGBUG raymondc - This is wrong.  EM_GETLIMITTEXT returns the number
    // of characters, not bytes.  But the only place we use it is in our
    // IME composition code, and maybe they really meant to divide by two...

    case EM_GETLIMITTEXT:
        return SendMessageA(hWnd, Msg, wParam, lParam) / sizeof(WCHAR);

    case EM_GETLINE:
    {
        LRESULT nLen;

        CStrOut str((LPWSTR) lParam, (* (SHORT *) lParam) + 1);
        * (SHORT *) (LPSTR) str = * (SHORT *) lParam;
        nLen = SendMessageA(hWnd, Msg, (WPARAM) wParam, (LPARAM) (LPSTR) str);
        if(nLen > 0)
            ((LPSTR) str)[nLen] = '\0';

        return nLen;
    }

    // BUGBUG: Always assume lParam points structure, not string buffer
    case CB_INSERTSTRING:
    {
        return SendMessageA(hWnd, Msg, wParam, (LPARAM) lParam);
    }

    case WM_SETTEXT:
    case LB_ADDSTRING:
    case CB_ADDSTRING:
    case EM_REPLACESEL:
        ASSERT(wParam == 0 && "wParam should be 0 for these messages");
        // fall through
    case CB_SELECTSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_FINDSTRING:
    case LB_INSERTSTRING:
    case LB_FINDSTRINGEXACT:
    {
        CStrIn  str((LPWSTR) lParam);
        return SendMessageA(hWnd, Msg, wParam, (LPARAM) (LPSTR) str);
    }

    case LB_GETTEXTLEN:
    case CB_GETLBTEXTLEN:
        ASSERT((LB_GETTEXTLEN - LB_GETTEXT) == (CB_GETLBTEXTLEN - CB_GETLBTEXT));
        lParam = (LPARAM)sz;    // use temp buffer
        Msg -= (LB_GETTEXTLEN - LB_GETTEXT);
        // fall through ...

    case LB_GETTEXT:
    case CB_GETLBTEXT:
    {
        CStrOut str((LPWSTR)lParam, 255);
        SendMessageA(hWnd, Msg, wParam, (LPARAM) (LPSTR) str);
        return str.ConvertExcludingNul();
    }

    case EM_SETPASSWORDCHAR:
    {
        WPARAM  wp;

        ASSERT(HIWORD64(wParam) == 0);
        MbcsFromUnicode((LPSTR) &wp, sizeof(wp), (LPWSTR) &wParam);
        ASSERT(HIWORD64(wp) == 0);

        return SendMessageA(hWnd, Msg, wp, lParam);
    }

    default:
        return SendMessageA(hWnd, Msg, wParam, lParam);
    }
}


BOOL SendNotifyMessageWrap(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    // BUGBUG: Should we use SendMessageWarp like SendDlgItemMessageWrap above?
    return SendNotifyMessageA(hWnd, Msg, wParam, lParam);
}

BOOL SetCurrentDirectoryWrap(LPCWSTR lpszCurDir)
{
    CStrIn  str(lpszCurDir);

    return SetCurrentDirectoryA(str);
}

BOOL SetDlgItemTextWrap(HWND hDlg, int nIDDlgItem, LPCWSTR lpString)
{
    CStrIn  str(lpString);

    return SetDlgItemTextA(hDlg, nIDDlgItem, str);
}

BOOL SetMenuItemInfoWrap(
    HMENU hMenu,
    UINT uItem,
    BOOL fByPosition,
    LPCMENUITEMINFOW lpmiiW)
{
    BOOL fRet;

    ASSERT( sizeof(MENUITEMINFOW) == sizeof(MENUITEMINFOA) &&
            FIELD_OFFSET(MENUITEMINFOW, dwTypeData) ==
            FIELD_OFFSET(MENUITEMINFOA, dwTypeData) );

    if ( (MIIM_TYPE & lpmiiW->fMask) &&
         0 == (lpmiiW->fType & (MFT_BITMAP | MFT_SEPARATOR)))
    {
        MENUITEMINFOA miiA;
        CStrIn str(lpmiiW->dwTypeData, lpmiiW->cch);

        memcpy( &miiA, lpmiiW, sizeof(MENUITEMINFOA) );
        miiA.dwTypeData = str;
        miiA.cch = str.strlen();
                
        fRet = SetMenuItemInfoA( hMenu, uItem, fByPosition, &miiA );
    }
    else
    {
        fRet = SetMenuItemInfoA( hMenu, uItem, fByPosition,
                                 (LPCMENUITEMINFOA)lpmiiW );
    }

    return fRet;
}

BOOL SetPropWrap(
    HWND    hWnd,
    LPCWSTR lpString,
    HANDLE  hData)
{
    CStrIn  str(lpString);

    return SetPropA(hWnd, str, hData);
}

LONG SetWindowLongWrap(HWND hWnd, int nIndex, LONG dwNewLong)
{
    return SetWindowLongA(hWnd, nIndex, dwNewLong);
}

HHOOK SetWindowsHookExWrap(
    int idHook,
    HOOKPROC lpfn,
    HINSTANCE hmod,
    DWORD dwThreadId)
{
    return SetWindowsHookExA(idHook, lpfn, hmod, dwThreadId);
}

BOOL SetWindowTextWrap(HWND hWnd, LPCWSTR lpString)
{
    CStrIn  str(lpString);

    return SetWindowTextA(hWnd, str);
}

BOOL SystemParametersInfoWrap(
        UINT    uiAction,
        UINT    uiParam,
        PVOID   pvParam,
        UINT    fWinIni)
{
    BOOL        ret;
    char        ach[LF_FACESIZE];

    if (uiAction == SPI_SETDESKWALLPAPER)
    {
        CStrIn str((LPCWSTR) pvParam);

        ret = SystemParametersInfoA(
                        uiAction,
                        uiParam,
                        str,
                        fWinIni);
    }
    else if (uiAction == SPI_GETNONCLIENTMETRICS)
    {
        NONCLIENTMETRICSA ncmA;
        NONCLIENTMETRICS *pncm = (NONCLIENTMETRICS *)pvParam;

        ASSERT(uiParam == sizeof(NONCLIENTMETRICS) && pncm->cbSize == sizeof(NONCLIENTMETRICS));

        ncmA.cbSize = sizeof(ncmA);
        ret = SystemParametersInfoA(
                        uiAction,
                        sizeof(ncmA),
                        &ncmA,
                        fWinIni);

        pncm->iBorderWidth = ncmA.iBorderWidth;
        pncm->iScrollWidth = ncmA.iScrollWidth;
        pncm->iScrollHeight = ncmA.iScrollHeight;
        pncm->iCaptionWidth = ncmA.iCaptionWidth;
        pncm->iCaptionHeight = ncmA.iCaptionHeight;
        pncm->iSmCaptionWidth = ncmA.iSmCaptionWidth;
        pncm->iSmCaptionHeight = ncmA.iSmCaptionHeight;
        pncm->iMenuWidth = ncmA.iMenuWidth;
        pncm->iMenuHeight = ncmA.iMenuHeight;

        memcpy(&pncm->lfCaptionFont, &ncmA.lfCaptionFont, FIELD_OFFSET(LOGFONTW, lfFaceName));
        UnicodeFromMbcs(pncm->lfCaptionFont.lfFaceName, ARRAYSIZE(pncm->lfCaptionFont.lfFaceName), ncmA.lfCaptionFont.lfFaceName);

        memcpy(&pncm->lfSmCaptionFont, &ncmA.lfSmCaptionFont, FIELD_OFFSET(LOGFONTW, lfFaceName));
        UnicodeFromMbcs(pncm->lfSmCaptionFont.lfFaceName, ARRAYSIZE(pncm->lfSmCaptionFont.lfFaceName), ncmA.lfSmCaptionFont.lfFaceName);

        memcpy(&pncm->lfMenuFont, &ncmA.lfMenuFont, FIELD_OFFSET(LOGFONTW, lfFaceName));
        UnicodeFromMbcs(pncm->lfMenuFont.lfFaceName, ARRAYSIZE(pncm->lfMenuFont.lfFaceName), ncmA.lfMenuFont.lfFaceName);

        memcpy(&pncm->lfStatusFont, &ncmA.lfStatusFont, FIELD_OFFSET(LOGFONTW, lfFaceName));
        UnicodeFromMbcs(pncm->lfStatusFont.lfFaceName, ARRAYSIZE(pncm->lfStatusFont.lfFaceName), ncmA.lfStatusFont.lfFaceName);

        memcpy(&pncm->lfMessageFont, &ncmA.lfMessageFont, FIELD_OFFSET(LOGFONTW, lfFaceName));
        UnicodeFromMbcs(pncm->lfMessageFont.lfFaceName, ARRAYSIZE(pncm->lfMessageFont.lfFaceName), ncmA.lfMessageFont.lfFaceName);
    }
    else
        ret = SystemParametersInfoA(
                        uiAction,
                        uiParam,
                        pvParam,
                        fWinIni);

    if ((uiAction == SPI_GETICONTITLELOGFONT) && ret)
    {
        strcpy(ach, ((LPLOGFONTA)pvParam)->lfFaceName);
        UnicodeFromMbcs(
                ((LPLOGFONTW)pvParam)->lfFaceName,
                ARRAYSIZE(((LPLOGFONTW)pvParam)->lfFaceName),
                ach);
    }

    return ret;
}

#ifndef FONT_LINK
BOOL TextOutWrap(HDC hdc, int x, int y, LPCWSTR lpStr, int cb)
{
    if (g_fMEEnabled && !g_bRunOnMemphis)
    {
        CStrIn str(lpStr);

        return TextOutA(hdc, x, y, str, str.strlen());
    }
    else
        return TextOutW(hdc, x, y, lpStr, cb);
}    
#endif

int TranslateAcceleratorWrap(HWND hWnd, HACCEL hAccTable, LPMSG lpMsg)
{
    return TranslateAcceleratorA(hWnd, hAccTable, lpMsg);
}

BOOL UnregisterClassWrap(LPCWSTR lpClassName, HINSTANCE hInstance)
{
    CStrIn  str(lpClassName);

    return UnregisterClassA(str, hInstance);
}

SHORT VkKeyScanWrap(WCHAR ch)
{
    CStrIn str(&ch, 1);

    return VkKeyScanA(*(char *)str);
}

BOOL WinHelpWrap(HWND hwnd, LPCWSTR szFile, UINT uCmd, DWORD dwData)
{
    CStrIn  str(szFile);

    return WinHelpA(hwnd, str, uCmd, dwData);
}


#define DBCS_CHARSIZE   (2)

//  N versions of wsprintf and wvsprintf which take an output buffer size to prevent overflow
//  bugs.  Taken from the NT wsprintf source code.

//  _MBToWCS and _WCSToMB are actually macros which call ntrtl functions in the NT version.
int _MBToWCS(LPCSTR pszIn, int cchIn, LPWSTR *ppwszOut)
{
    int cch = 0;
    int cbAlloc;

    if ((0 != cchIn) && (NULL != ppwszOut))
    {
        cchIn++;
        cbAlloc = cchIn * sizeof(WCHAR);

        *ppwszOut = (LPWSTR)LocalAlloc(LMEM_FIXED, cbAlloc);

        if (NULL != *ppwszOut)
        {
            cch = MultiByteToWideChar(CP_ACP, 0, pszIn, cchIn, *ppwszOut, cchIn);

            if (!cch)
            {
                LocalFree(*ppwszOut);
                *ppwszOut = NULL;
            }
            else
            {
                cch--;  //  Just return the number of characters
            }
        }
    }

    return cch;
}

/****************************** Module Header ******************************\
* Module Name: wsprintf.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*  sprintf.c
*
*  Implements Windows friendly versions of sprintf and vsprintf
*
*  History:
*   2-15-89  craigc     Initial
*  11-12-90  MikeHar    Ported from windows 3
\***************************************************************************/

/* Max number of characters. Doesn't include termination character */

#define out(c) if (cchLimit) {*lpOut++=(c); cchLimit--;} else goto errorout

/***************************************************************************\
* SP_GetFmtValueW
*
*  reads a width or precision value from the format string
*
* History:
*  11-12-90  MikeHar    Ported from windows 3
*  07-27-92  GregoryW   Created Unicode version (copied from SP_GetFmtValue)
\***************************************************************************/

LPCWSTR SP_GetFmtValueW(
    LPCWSTR lpch,
    int *lpw)
{
    int ii = 0;

    /* It might not work for some locales or digit sets */
    while (*lpch >= L'0' && *lpch <= L'9') {
        ii *= 10;
        ii += (int)(*lpch - L'0');
        lpch++;
    }

    *lpw = ii;

    /*
     * return the address of the first non-digit character
     */
    return lpch;
}

/***************************************************************************\
* SP_PutNumberW
*
* Takes an unsigned long integer and places it into a buffer, respecting
* a buffer limit, a radix, and a case select (upper or lower, for hex).
*
*
* History:
*  11-12-90  MikeHar    Ported from windows 3 asm --> C
*  12-11-90  GregoryW   need to increment lpstr after assignment of mod
*  02-11-92  GregoryW   temporary version until we have C runtime support
\***************************************************************************/

int SP_PutNumberW(
    LPWSTR lpstr,
    DWORD n,
    int   limit,
    DWORD radix,
    int   uppercase)
{
    DWORD mod;
    int count = 0;

    /* It might not work for some locales or digit sets */
    if(uppercase)
        uppercase =  'A'-'0'-10;
    else
        uppercase = 'a'-'0'-10;

    if (count < limit) {
        do  {
            mod =  n % radix;
            n /= radix;

            mod += '0';
            if (mod > '9')
            mod += uppercase;
            *lpstr++ = (WCHAR)mod;
            count++;
        } while((count < limit) && n);
    }

    return count;
}

/***************************************************************************\
* SP_ReverseW
*
*  reverses a string in place
*
* History:
*  11-12-90  MikeHar    Ported from windows 3 asm --> C
*  12-11-90  GregoryW   fixed boundary conditions; removed count
*  02-11-92  GregoryW   temporary version until we have C runtime support
\***************************************************************************/

void SP_ReverseW(
    LPWSTR lpFirst,
    LPWSTR lpLast)
{
    WCHAR ch;

    while(lpLast > lpFirst){
        ch = *lpFirst;
        *lpFirst++ = *lpLast;
        *lpLast-- = ch;
    }
}


/***************************************************************************\
* wvsprintfW (API)
*
* wsprintfW() calls this function.
*
* History:
*    11-Feb-1992 GregoryW copied xwvsprintf
*         Temporary hack until we have C runtime support
* 1-22-97 tnoonan       Converted to wvnsprintfW
\***************************************************************************/

int wvnsprintfW(
    LPWSTR lpOut,
    int cchLimitIn,
    LPCWSTR lpFmt,
    va_list arglist)
{
    BOOL fAllocateMem = FALSE;
    WCHAR prefix, fillch;
    int left, width, prec, size, sign, radix, upper, hprefix;
    int cchLimit = --cchLimitIn, cch;
    LPWSTR lpT, lpTWC;
    LPCSTR psz;
    va_list varglist = arglist;
    union {
        long l;
        unsigned long ul;
        char sz[2];
        WCHAR wsz[2];
    } val;

    if (cchLimit < 0)
        return 0;

    while (*lpFmt != 0) {
        if (*lpFmt == L'%') {

            /*
             * read the flags.  These can be in any order
             */
            left = 0;
            prefix = 0;
            while (*++lpFmt) {
                if (*lpFmt == L'-')
                    left++;
                else if (*lpFmt == L'#')
                    prefix++;
                else
                    break;
            }

            /*
             * find fill character
             */
            if (*lpFmt == L'0') {
                fillch = L'0';
                lpFmt++;
            } else
                fillch = L' ';

            /*
             * read the width specification
             */
            lpFmt = SP_GetFmtValueW(lpFmt, &cch);
            width = cch;

            /*
             * read the precision
             */
            if (*lpFmt == L'.') {
                lpFmt = SP_GetFmtValueW(++lpFmt, &cch);
                prec = cch;
            } else
                prec = -1;

            /*
             * get the operand size
             * default size: size == 0
             * long number:  size == 1
             * wide chars:   size == 2
             * It may be a good idea to check the value of size when it
             * is tested for non-zero below (IanJa)
             */
            hprefix = 0;
            if ((*lpFmt == L'w') || (*lpFmt == L't')) {
                size = 2;
                lpFmt++;
            } else if (*lpFmt == L'l') {
                size = 1;
                lpFmt++;
            } else {
                size = 0;
                if (*lpFmt == L'h') {
                    lpFmt++;
                    hprefix = 1;
                }
            }

            upper = 0;
            sign = 0;
            radix = 10;

            switch (*lpFmt) {
            case 0:
                goto errorout;

            case L'i':
            case L'd':
                size=1;
                sign++;

                /*** FALL THROUGH to case 'u' ***/

            case L'u':
                /* turn off prefix if decimal */
                prefix = 0;
donumeric:
                /* special cases to act like MSC v5.10 */
                if (left || prec >= 0)
                    fillch = L' ';

                /*
                 * if size == 1, "%lu" was specified (good)
                 * if size == 2, "%wu" was specified (bad)
                 */
                if (size) {
                    val.l = va_arg(varglist, LONG);
                } else if (sign) {
                    val.l = va_arg(varglist, SHORT);
                } else {
                    val.ul = va_arg(varglist, unsigned);
                }

                if (sign && val.l < 0L)
                    val.l = -val.l;
                else
                    sign = 0;

                lpT = lpOut;

                /*
                 * blast the number backwards into the user buffer
                 */
                cch = SP_PutNumberW(lpOut, val.l, cchLimit, radix, upper);
                if (!(cchLimit -= cch))
                    goto errorout;

                lpOut += cch;
                width -= cch;
                prec -= cch;
                if (prec > 0)
                    width -= prec;

                /*
                 * fill to the field precision
                 */
                while (prec-- > 0)
                    out(L'0');

                if (width > 0 && !left) {
                    /*
                     * if we're filling with spaces, put sign first
                     */
                    if (fillch != L'0') {
                        if (sign) {
                            sign = 0;
                            out(L'-');
                            width--;
                        }

                        if (prefix) {
                            out(prefix);
                            out(L'0');
                            prefix = 0;
                        }
                    }

                    if (sign)
                        width--;

                    /*
                     * fill to the field width
                     */
                    while (width-- > 0)
                        out(fillch);

                    /*
                     * still have a sign?
                     */
                    if (sign)
                        out(L'-');

                    if (prefix) {
                        out(prefix);
                        out(L'0');
                    }

                    /*
                     * now reverse the string in place
                     */
                    SP_ReverseW(lpT, lpOut - 1);
                } else {
                    /*
                     * add the sign character
                     */
                    if (sign) {
                        out(L'-');
                        width--;
                    }

                    if (prefix) {
                        out(prefix);
                        out(L'0');
                    }

                    /*
                     * reverse the string in place
                     */
                    SP_ReverseW(lpT, lpOut - 1);

                    /*
                     * pad to the right of the string in case left aligned
                     */
                    while (width-- > 0)
                        out(fillch);
                }
                break;

            case L'X':
                upper++;

                /*** FALL THROUGH to case 'x' ***/

            case L'x':
                radix = 16;
                if (prefix)
                    if (upper)
                        prefix = L'X';
                    else
                        prefix = L'x';
                goto donumeric;

            case L'c':
            case L'C':
                if (!size && !hprefix) {
                    size = 1;           // force WCHAR
                }

                /*** FALL THROUGH to case 'C' ***/

                /*
                 * if size == 0, "%C" or "%hc" was specified (CHAR)
                 * if size == 1, "%c" or "%lc" was specified (WCHAR)
                 * if size == 2, "%wc" or "%tc" was specified (WCHAR)
                 */
                cch = 1; /* One character must be copied to the output buffer */
                if (size) {
                    val.wsz[0] = va_arg(varglist, WCHAR);
                    val.wsz[1] = 0;
                    lpT = val.wsz;
                    goto putwstring;
                } else {
                    val.sz[0] = va_arg(varglist, CHAR);
                    val.sz[1] = 0;
                    psz = val.sz;
                    goto putstring;
                }

            case L's':
            case L'S':
                if (!size && !hprefix) {
                    size = 1;           // force LPWSTR
                }

                /*** FALL THROUGH to case 'S' ***/

                /*
                 * if size == 0, "%S" or "%hs" was specified (LPSTR)
                 * if size == 1, "%s" or "%ls" was specified (LPWSTR)
                 * if size == 2, "%ws" or "%ts" was specified (LPWSTR)
                 */
                if (size) {
                    lpT = va_arg(varglist, LPWSTR);
                    cch = lstrlenW(lpT);    // Win95 supports lstrlenW!
                } else {
                    psz = va_arg(varglist, LPSTR);
                    cch = lstrlenA(psz);
putstring:
                    cch = _MBToWCS(psz, cch, &lpTWC);
                    fAllocateMem = (BOOL) cch;
                    lpT = lpTWC;
                }
putwstring:
                if (prec >= 0 && cch > prec)
                    cch = prec;
                width -= cch;

                if (left) {
                    while (cch--)
                        out(*lpT++);
                    while (width-- > 0)
                        out(fillch);
                } else {
                    while (width-- > 0)
                        out(fillch);
                    while (cch--)
                        out(*lpT++);
                }

                if (fAllocateMem) {
                     LocalFree(lpTWC);
                     fAllocateMem = FALSE;
                }

                break;

            default:
normalch:
                out((WCHAR)*lpFmt);
                break;
            }  /* END OF SWITCH(*lpFmt) */
        }  /* END OF IF(%) */ else
            goto normalch;  /* character not a '%', just do it */

        /*
         * advance to next format string character
         */
        lpFmt++;
    }  /* END OF OUTER WHILE LOOP */

errorout:
    *lpOut = 0;

    if (fAllocateMem)
    {
        LocalFree(lpTWC);
    }

    return cchLimitIn - cchLimit;
}

LWSTDAPIV_(int) wnsprintfW(
    LPWSTR lpOut,
    int cchLimitIn,
    LPCWSTR lpFmt,
    ...)
{
    va_list arglist;
    int ret;

    va_start(arglist, lpFmt);
    ret = wvnsprintfW(lpOut, cchLimitIn, lpFmt, arglist);
    va_end(arglist);
    return ret;
}

LWSTDAPIV_(int) wsprintfW(
    LPWSTR lpOut,
    LPCWSTR lpFmt,
    ...)
{
    // unsafe printf.  arbitrary max of 0x10000 length
    va_list arglist;
    int ret;

    va_start(arglist, lpFmt);
    ret = wvnsprintfW(lpOut, 0x10000, lpFmt, arglist);
    va_end(arglist);
    return ret;
}


//+---------------------------------------------------------------------------
//      StartDoc
//----------------------------------------------------------------------------

int StartDocWrap( HDC hDC, const DOCINFO * lpdi )
{
    CStrIn  strDocName( lpdi->lpszDocName );
    CStrIn  strOutput( lpdi->lpszOutput );
    CStrIn  strDatatype( lpdi->lpszDatatype );
    DOCINFOA dia;

    dia.cbSize = sizeof(DOCINFO);
    dia.lpszDocName = strDocName;
    dia.lpszOutput = strOutput;
    dia.lpszDatatype = strDatatype;
    dia.fwType = lpdi->fwType;

    return StartDocA( hDC, &dia );
}

#endif  // !WINNT


////////////////////////////////////////////////////////////////////
//
//  Plug UI support with SHLWAPI
//

typedef HRESULT (*PFNDLLGETVERSION)(DLLVERSIONINFO * pinfo);
HMODULE GetShlwapiHModule()
{
    HMODULE hShlwapi = GetModuleHandle(TEXT("SHLWAPI"));
    if (hShlwapi)
    {
        PFNDLLGETVERSION pfnDllGetVersion = (PFNDLLGETVERSION)GetProcAddress(hShlwapi, "DllGetVersion");
        if (pfnDllGetVersion)
        {
            DLLVERSIONINFO dllinfo;

            dllinfo.cbSize = sizeof(DLLVERSIONINFO);
            if (pfnDllGetVersion(&dllinfo) == NOERROR)
            {
                if (dllinfo.dwMajorVersion < 5)
                {
                    // This guy doesn't support ML functions
                    hShlwapi = NULL;
                }
            }
        }
    }
    return hShlwapi;
}

// First, we need access to some helper functions:
//
#ifndef UNIX
#define SHLWAPIMLISMLHINSTANCE_ORD     429
#else
#define SHLWAPIMLISMLHINSTANCE_ORD     "MLIsMLHInstance"
#endif
typedef BOOL (* PFNMLISMLHINSTANCE)(HINSTANCE);
BOOL MLIsMLHInstanceWrap(HINSTANCE hInst)
{
    HMODULE hShlwapi = GetShlwapiHModule();
    if (hShlwapi)
    {
        PFNMLISMLHINSTANCE pfn;
        pfn = (PFNMLISMLHINSTANCE)GetProcAddress(hShlwapi, (LPCSTR)SHLWAPIMLISMLHINSTANCE_ORD);
        if (pfn)
            return pfn(hInst);
    }

    // BUGBUG:  What if an app told comctl32 to be PlugUI and we picked
    //          a resource that cannot be displayed on Win9x without
    //          shlwapi's font linking?  Seems like we need to foricbly
    //          load shlwapi in that case...
    //
    // No shlwapi? then this can't be an ML hinstance.
    return FALSE;
}

#ifndef UNIX
#define SHLWAPIMLSETMLHINSTANCE_ORD 430
#else
#define SHLWAPIMLSETMLHINSTANCE_ORD "MLSetMLHInstance"
#endif
typedef HRESULT (* PFNMLSETMLHINSTANCE)(HINSTANCE, LANGID);
HRESULT MLSetMLHInstanceWrap(HINSTANCE hInst, LANGID lidUI)
{
    HMODULE hShlwapi;
    PFNMLSETMLHINSTANCE pfnMLSet = NULL;

    hShlwapi = GetShlwapiHModule();
    if (hShlwapi)
    {
        pfnMLSet = (PFNMLSETMLHINSTANCE)GetProcAddress(hShlwapi, (LPCSTR)SHLWAPIMLSETMLHINSTANCE_ORD);
        if (pfnMLSet)
            return pfnMLSet(hInst, lidUI);
    }

    return E_FAIL;
}

#ifndef UNIX
#define SHLWAPIMLCLEARMLHINSTANCE_ORD 431
#else
#define SHLWAPIMLCLEARMLHINSTANCE_ORD "MLClearMLHInstance"
#endif
typedef HRESULT (* PFNMLCLEARMLHINSTANCE)(HINSTANCE);
HRESULT MLClearMLHinstanceWrap(HINSTANCE hInst)
{
    HMODULE hShlwapi;
    PFNMLCLEARMLHINSTANCE pfnMLClear = NULL;

    hShlwapi = GetShlwapiHModule();
    if (hShlwapi)
    {
        pfnMLClear = (PFNMLCLEARMLHINSTANCE)GetProcAddress(hShlwapi, (LPCSTR)SHLWAPIMLCLEARMLHINSTANCE_ORD);
        if (pfnMLClear)
            return pfnMLClear(hInst);
    }

    return E_FAIL;
}

//
// And now, when shlwapi is around we delegate to it's ML-enabled implementations:
//

//
//  Make sure we get the real USER32 versions.
//
#undef CreateDialogIndirectParamW

#ifndef UNIX
#define SHLWAPICREATEDIALOGINDIRECTPARAM_ORD     393
#else
#define SHLWAPICREATEDIALOGINDIRECTPARAM_ORD     "CreateDialogIndirectParamWrapW"
#endif
typedef HWND (* PFNCREATEDIALOGINDIRECTPARAM)(HINSTANCE, LPCDLGTEMPLATE, HWND, DLGPROC, LPARAM);
HWND CreateDialogIndirectParamWrap(
        HINSTANCE       hInstance,
        LPCDLGTEMPLATE  lpTemplate,
        HWND            hWndParent,
        DLGPROC         lpDialogFunc,
        LPARAM          dwInitParam)
{
    HMODULE hShlwapi;
    PFNCREATEDIALOGINDIRECTPARAM pfnCDIP = NULL;
    HWND hwndRet;

    // If shlwapi is in memory, ask it to create the dialog,
    // as then we get ML dialogs on downlevel platforms (if
    // the hInstance is from MLLoadLibrary -- otherwise it
    // thunks to the real A/W api for us).
    //
    hShlwapi = GetShlwapiHModule();
    if (hShlwapi)
    {
        pfnCDIP = (PFNCREATEDIALOGINDIRECTPARAM)GetProcAddress(hShlwapi, (LPCSTR)SHLWAPICREATEDIALOGINDIRECTPARAM_ORD);
    }

    if (!pfnCDIP)
    {
        if (g_bRunOnNT)
            pfnCDIP = CreateDialogIndirectParamW;
        else
            pfnCDIP = CreateDialogIndirectParamA;
    }

    // If this is from comctl32, assume it was loaded via the MUI-Language
    if (HINST_THISDLL == hInstance)
        MLSetMLHInstanceWrap(hInstance, GetMUILanguage());

    hwndRet = pfnCDIP(hInstance, lpTemplate, hWndParent, lpDialogFunc, dwInitParam);

    if (HINST_THISDLL == hInstance)
        MLClearMLHinstanceWrap(hInstance);

    return(hwndRet);
}

//
//  Make sure we get the real USER32 versions.
//
#undef DialogBoxIndirectParamW

#ifndef UNIX
#define SHLWAPIDIALOGBOXINDIRECTPARAM_ORD     58
#else
#define SHLWAPIDIALOGBOXINDIRECTPARAM_ORD     "DialogBoxIndirectParamWrapW"
#endif
typedef INT_PTR (* PFNDIALOGBOXINDIRECTPARAM)(HINSTANCE, LPCDLGTEMPLATE, HWND, DLGPROC, LPARAM);
INT_PTR DialogBoxIndirectParamWrap(
        HINSTANCE       hInstance,
        LPCDLGTEMPLATEW hDialogTemplate,
        HWND            hWndParent,
        DLGPROC         lpDialogFunc,
        LPARAM          dwInitParam)
{
    HMODULE hShlwapi;
    INT_PTR iRet;
    PFNDIALOGBOXINDIRECTPARAM pfnDBIP = NULL;
   
    // If shlwapi is in memory, ask it to create the dialog,
    // as then we get ML dialogs on downlevel platforms (if
    // the hInstance is from MLLoadLibrary -- otherwise it
    // thunks to the real A/W api for us).
    //
    hShlwapi = GetShlwapiHModule();
    if (hShlwapi)
        pfnDBIP = (PFNDIALOGBOXINDIRECTPARAM)GetProcAddress(hShlwapi, (LPCSTR)SHLWAPIDIALOGBOXINDIRECTPARAM_ORD);

    if (!pfnDBIP)
    {
        if (g_bRunOnNT)
            pfnDBIP = DialogBoxIndirectParamW;
        else
            pfnDBIP = DialogBoxIndirectParamA;
    }

    // If this is from comctl32, assume it was loaded via the MUI-Language
    if (HINST_THISDLL == hInstance)
        MLSetMLHInstanceWrap(hInstance, GetMUILanguage());

    iRet = pfnDBIP(hInstance, hDialogTemplate, hWndParent, lpDialogFunc, dwInitParam);
    
    if (HINST_THISDLL == hInstance)
        MLClearMLHinstanceWrap(hInstance);

    return iRet;
}

#endif  // UNICODE
