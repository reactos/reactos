#include "headers.hxx"

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

MtDefine(MbcsFromUnicode_aryBuf_pv, Locals, "MbcsFromUnicode aryBuf::_pv")

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
//              [codepage]  -- the code page to use (CP_ACP is default)
//              [flags]     -- indicates if WCH_EMBEDDING should be treated
//                          with special handling.
//
//  Returns:    If [pstr] is NULL or [cch] is 0, 0 is returned.  Otherwise,
//              the number of characters converted, including the terminating
//              NULL, is returned (note that converting the empty string will
//              return 1).  If the conversion fails, 0 is returned.
//
//  Modifies:   [pstr].
//
//----------------------------------------------------------------------------

int
MbcsFromUnicode(LPSTR pstr, int cch, LPCWSTR pwstr, int cwch, UINT codepage)
{
    Assert(cch >= 0);
    
    if (!pstr || cch == 0)
        return 0;

    Assert(pwstr);
    
    Assert(cwch == -1 || cwch > 0);

    return WideCharToMultiByte(codepage, 0, pwstr, cwch, pstr, cch, NULL, NULL);
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

int
UnicodeFromMbcs(LPWSTR pwstr, int cwch, LPCSTR pstr, int cch, UINT uiCodePage)
{
    int ret;

    Assert(cwch >= 0);

    if (!pwstr || cwch == 0)
        return 0;

    Assert(pstr);
    Assert(cch == -1 || cch >= 0);

    ret = MultiByteToWideChar(uiCodePage, 0, pstr, cch, pwstr, cwch);

    return ret;
}


