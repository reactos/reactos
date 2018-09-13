//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       errtbl.cxx
//
//  Contents:   Error table for GetErrorText
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef X_OLECTL_H_
#define X_OLECTL_H_
#include "olectl.h"
#endif

//
// Each entry in the table has the following format:
//     StartError, NumberOfErrorsToMap, ErrorMessageResourceID
//
// The last entry must be all 0s.
//

struct ERRTOMSG
{
    SCODE   error;                      // the starting error code
    USHORT  cErrors;                    // number of errors in range to map
    USHORT  ids;                        // the error msg string id
};

//
// Disable warning: Truncation of constant value.  There is
// no way to specify a short integer constant.
//

#pragma warning(disable:4305 4309)

ERRTOMSG g_aetmError[] =
{
#ifdef PRODUCT_97
    FORMS_E_NOPAGESSPECIFIED,   1,  IDS_E_NOPAGESSPECIFIED,
    FORMS_E_NOPAGESINTERSECT,   1,  IDS_E_NOPAGESINTERSECT,
#endif
    E_UNEXPECTED,               1,  IDS_EE_UNEXPECTED,
    E_FAIL,                     1,  IDS_EE_FAIL,
    E_INVALIDARG,               1,  IDS_EE_INVALIDARG,
    CTL_E_CANTMOVEFOCUSTOCTRL,          1,  IDS_EE_CANTMOVEFOCUSTOCTRL,
    CTL_E_CONTROLNEEDSFOCUS,            1,  IDS_EE_CONTROLNEEDSFOCUS,
    CTL_E_INVALIDPICTURE,               1,  IDS_EE_INVALIDPICTURE,
    CTL_E_INVALIDPICTURETYPE,           1,  IDS_EE_INVALIDPICTURETYPE,
    CTL_E_INVALIDPROPERTYARRAYINDEX,    1,  IDS_EE_INVALIDPROPERTYARRAYINDEX,
    CTL_E_INVALIDPROPERTYVALUE,         1,  IDS_EE_INVALIDPROPERTYVALUE,
    CTL_E_METHODNOTAPPLICABLE,          1,  IDS_EE_METHODNOTAPPLICABLE,
    CTL_E_OVERFLOW,                     1,  IDS_EE_OVERFLOW,
    CTL_E_PERMISSIONDENIED,             1,  IDS_EE_PERMISSIONDENIED,
    CTL_E_SETNOTSUPPORTEDATRUNTIME,     1,  IDS_EE_SETNOTSUPPORTEDATRUNTIME,
    CTL_E_INVALIDPASTETARGET,           1,  IDS_EE_INVALIDPASTETARGET,
    CTL_E_INVALIDPASTESOURCE,           1,  IDS_EE_INVALIDPASTESOURCE,
    CTL_E_MISMATCHEDTAG,                1,  IDS_EE_MISMATCHEDTAG,
    CTL_E_INCOMPATIBLEPOINTERS,         1,  IDS_EE_INCOMPATIBLEPOINTERS,
    CTL_E_UNPOSITIONEDPOINTER,          1,  IDS_EE_UNPOSITIONEDPOINTER,
    CTL_E_UNPOSITIONEDELEMENT,          1,  IDS_EE_UNPOSITIONEDELEMENT,
    CLASS_E_NOTLICENSED,                1,  IDS_EE_NOTLICENSED,
    MSOCMDERR_E_NOTSUPPORTED,           1,  IDS_E_CMDNOTSUPPORTED,
    HRESULT_FROM_WIN32(ERROR_INTERNET_INVALID_URL),       1, IDS_EE_INTERNET_INVALID_URL,
    HRESULT_FROM_WIN32(ERROR_INTERNET_NAME_NOT_RESOLVED), 1, IDS_EE_INTERNET_NAME_NOT_RESOLVED,
    // Terminate table with nulls.
    0,                          0,  0,
};

ERRTOMSG g_aetmSolution[] =
{

    // Terminate table with nulls.
    0,                          0,  0,
};


#pragma warning(default:4305 4309)

//+---------------------------------------------------------------------------
//
//  Function:   GetErrorText
//
//  Synopsis:   Gets the text of an error from a message resource.
//
//  Arguments:  [hr]   --   The error.  Must not be 0.
//              [pstr] --   Buffer for the message.
//              [cch]  --   Size of the buffer in characters.
//
//  Returns:    HRESULT.
//
//  Modifies:   [*pstr]
//
//----------------------------------------------------------------------------

HRESULT
GetErrorText(HRESULT hrError, LPTSTR pstr, int cch)
{
    HRESULT     hr  = S_OK;
    ERRTOMSG *  petm;
    DWORD       dwFlags;
    LPCVOID     pvSource;
    BOOL        fHrCode = FALSE;
    

    Assert(pstr);
    Assert(cch >= 0);

    //
    // Check if we handle the message in our table.
    //
#ifdef WIN16
    //E_ACCESSDENIED is different in Win32
    if (hrError == E_ACCESSDENIED)
        hrError =   CTL_E_PERMISSIONDENIED;
#endif

    for (petm = g_aetmError; petm->error; petm++)
    {
        if (petm->error <= hrError && hrError < petm->error + petm->cErrors)
        {


            if (!LoadString(GetResourceHInst(), petm->ids, pstr, cch))
                RRETURN(GetLastWin32Error());

            return S_OK;
        }
    }

    //
    // Check the system for the message.
    //

#ifndef WIN16
    if (hrError >= HRESULT_FROM_WIN32(INTERNET_ERROR_BASE) &&
        hrError <= HRESULT_FROM_WIN32(INTERNET_ERROR_LAST))
    {
        dwFlags = FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS;
        pvSource = GetModuleHandleA("wininet.dll");
        fHrCode = TRUE;
    }
    else if (hrError >= INET_E_ERROR_FIRST && hrError <= INET_E_ERROR_LAST)
    {
        dwFlags = FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS;
        pvSource = GetModuleHandleA("urlmon.dll");
    }
    else
#endif // ndef WIN16
    {
        dwFlags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
        pvSource = NULL;
    }

    if (FormatMessage(
            dwFlags,
            pvSource,
            fHrCode ? HRESULT_CODE(hrError) : hrError,
            LANG_SYSTEM_DEFAULT,
            pstr,
            cch,
            NULL))
    {
        return S_OK;
    }

    //
    // Show the error string and the hex code.
    //

    hr = Format(0, pstr, cch, MAKEINTRESOURCE(IDS_UNKNOWN_ERROR), hrError);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   GetSolutionText
//
//  Synopsis:   Gets the text of an error from a message resource.
//
//  Arguments:  [hr]   --   The error.  Must not be 0.
//              [pstr] --   Buffer for the message.
//              [cch]  --   Size of the buffer in characters.
//
//  Returns:    HRESULT.
//
//  Modifies:   [*pstr]
//
//----------------------------------------------------------------------------

HRESULT
GetSolutionText(HRESULT hrError, LPTSTR pstr, int cch)
{
    ERRTOMSG *  petm;

    Assert(pstr);
    Assert(cch >= 0);

    //
    // Check if we handle the message in our table.
    //

    for (petm = g_aetmSolution; petm->error; petm++)
    {
        if (petm->error <= hrError && hrError < petm->error + petm->cErrors)
        {


            if (!LoadString(GetResourceHInst(), petm->ids, pstr, cch))
                RRETURN(GetLastWin32Error());

            return S_OK;
        }
    }

    *pstr = 0;
    return S_OK;
}
