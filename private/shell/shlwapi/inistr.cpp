//+---------------------------------------------------------------------------
//
//  File:       inistr.cpp
//
//  Contents:   SHGet/SetIniStringW implementations, which save strings into
//              INI files in a manner that survives the round trip to disk.
//
//----------------------------------------------------------------------------

#include "priv.h"
#define _SHELL32_
#define _SHDOCVW_
#include "unicwrap.h"

#include <platform.h>
#include <mlang.h>

//
//  Do this in every wrapper function that has an output parameter.
//  It raises assertion failures on the main code path so that
//  the same assertions are raised on NT and 95.  The CStrOut class
//  doesn't like it when you say that an output buffer is NULL yet
//  has nonzero length.  Without this macro, the bug would go undetected
//  on NT and appear only on Win95.
//
#define VALIDATE_OUTBUF(s, cch) ASSERT((s) != NULL || (cch) == 0)

//----------------------------------------------------------------------------
//
//  The basic problem is that INI files are ANSI-only, so any UNICODE
//  string you put into it won't round-trip.
//
//  So the solution is to record UNICODE strings in UTF7.  Why UTF7?
//  Because we can't use UTF8, since XxxPrivateProfileStringW will try
//  to convert the 8-bit values to/from UNICODE and screw them up.  Since
//  some of the 8-bit values might not even be valid (e.g., a DBCS lead
//  byte followed by an illegal trail byte), we cannot assume that the
//  string will survive the ANSI -> UNICODE -> ANSI round-trip.
//
//  The UTF7 string is stored in a [Section.W] section, under the
//  same key name.  The original ANSI string is stored in a [Section.A]
//  section, again, with the same key name.
//
//  (We separate the A/W from the section name with a dot so it is less
//  likely that we will accidentally collide with other section names.
//
//  We store the original ANSI string twice so we can compare the two
//  and see if a downlevel app (e.g., IE4) has changed the [Section]
//  version.  If so, then we ignore the [Section.W] version since it's stale.
//
//  If the original string is already 7-bit clean, then no UTF7 string is
//  recorded.
//

BOOL
Is7BitClean(LPCWSTR pwsz)
{
    for ( ; *pwsz; pwsz++) {
        if ((UINT)*pwsz > 127)
            return FALSE;
    }
    return TRUE;
}

//----------------------------------------------------------------------------
//
//  Yet another conversion class -- this one is for creating the
//  variants of a section name.
//
//  Note!  Since INI files are ASCII, section names are necessarily 7-bit
//  clean, so we can cheat a lot of stuff.
//

class CStrSectionX : public CConvertStrW
{
public:
    CStrSectionX(LPCWSTR pwszSection);
};

//
//  We append a dot an an A or W to the section name.
//
#define SECTION_SUFFIX_LEN  2

CStrSectionX::CStrSectionX(LPCWSTR pwszSection)
{
    ASSERT(_pwstr == NULL);
    if (pwszSection) {

        ASSERT(Is7BitClean(pwszSection));

        UINT cwchNeeded = lstrlenW(pwszSection) + SECTION_SUFFIX_LEN + 1;
        if (cwchNeeded > ARRAYSIZE(_awch)) {
            _pwstr = new WCHAR[cwchNeeded];
        } else {
            _pwstr = _awch;
        }

        if (_pwstr) {
            // Build the string initially with ".A" stuck on the end
            // It will later get changed to a ".W"
            lstrcpyW(_pwstr, pwszSection);
            lstrcatW(_pwstr, L".A");
        }
    }
}

//----------------------------------------------------------------------------
//
//  Mini-class for keeping track of UTF7 strings.  These are kept in ANSI
//  most of the time since that's what ConvertINetUnicodeToMultiByte uses.
//
//  The UTF7 shadow is prefixed by a checksum of the original string, which
//  we use on read-back to see if the shadow still corresponds to the
//  original string.
//

class CStrUTF7 : public CConvertStr
{
public:
    inline CStrUTF7() : CConvertStr(CP_ACP) { };
    void SetUnicode(LPCWSTR pwszValue);
};

//
//  Note that this can be slow since it happens only when we encounter
//  a non-ANSI character.
//
void CStrUTF7::SetUnicode(LPCWSTR pwszValue)
{
    int cwchLen = lstrlenW(pwszValue);
    HRESULT hres;
    DWORD dwMode;

    int cwchLenT = cwchLen;

    // Save room for terminating NULL.  We must convert the NULL separately
    // because UTF7 does not translate NULL to NULL.
    int cchNeeded = ARRAYSIZE(_ach) - 1;
    dwMode = 0;
    hres = ConvertINetUnicodeToMultiByte(&dwMode, CP_UTF7, pwszValue,
                                         &cwchLenT, _ach,
                                         &cchNeeded);
    if (SUCCEEDED(hres)) {
        ASSERT(cchNeeded + 1 <= ARRAYSIZE(_ach));
        _pstr = _ach;
    } else {
        _pstr = new CHAR[cchNeeded + 1];
        if (!_pstr)
            return;                 // No string - tough

        cwchLenT = cwchLen;
        dwMode = 0;
        hres = ConvertINetUnicodeToMultiByte(&dwMode, CP_UTF7, pwszValue,
                                    &cwchLenT, _pstr,
                                    &cchNeeded);
        if (FAILED(hres)) {         // Couldn't convert - tough
            Free();
            return;
        }
    }

    // Terminate explicitly since UTF7 doesn't.
    _pstr[cchNeeded] = '\0';
}

//
//  pwszSection  = section name into which to write pwszValue (UNICODE)
//  pwszSectionA = section name into which to write ANSI shadow
//  pwszKey      = key name for both pwszValue and strUTF7
//  pwszFileName = file name
//
//  pwszSectionA can be NULL if a low-memory condition was encountered.
//
//  strUTF7 can be NULL, meaning that the shadows should be deleted.
//
//  Write pwszSection first, followed by pwszSectionA, then pwszSectionW.
//  This ensures that the backwards-compatibility string comes first in
//  the file, in case there are apps that assume such.
//
//  pwszSectionW is computed from pwszSectionA by changing the last "A"
//  to a "W".  pwszSecionW gets the UTF7-encoded unicode string.
//  strUTF7 might be NULL, meaning that we should delete the shadow strings.
//
BOOL WritePrivateProfileStringMultiW(LPCWSTR pwszSection,  LPCWSTR pwszValue,
                                      LPWSTR pwszSectionA, CStrUTF7& strUTF7,
                                     LPCWSTR pwszKey,      LPCWSTR pwszFileName)
{
    BOOL fRc = WritePrivateProfileStringW(pwszSection, pwszKey, pwszValue, pwszFileName);

    if (pwszSectionA) {
        //
        //  Write the [Section.A] key, or delete it if there is no UTF7.
        //
        WritePrivateProfileStringW(pwszSectionA, pwszKey,
                                   strUTF7 ? pwszValue : NULL, pwszFileName);

        //
        //  Now change pwszSectionA to pwszSectionW so we can write out
        //  the UTF7 encoding.
        //
        pwszSectionA[lstrlenW(pwszSectionA) - 1] = TEXT('W');

        CStrInW strUTF7W(strUTF7);
        // This really writes [Section.W]
        WritePrivateProfileStringW(pwszSectionA, pwszKey, strUTF7W, pwszFileName);
    }

    return fRc;
}

BOOL WritePrivateProfileStringMultiA(LPCWSTR pwszSection,  LPCWSTR pwszValue,
                                      LPWSTR pwszSectionA, CStrUTF7& strUTF7,
                                     LPCWSTR pwszKey,      LPCWSTR pwszFileName)
{
    CStrIn strSection(pwszSection);
    CStrIn strSectionA(pwszSectionA);
    CStrIn strValue(pwszValue);
    CStrIn strKey(pwszKey);
    CPPFIn strFileName(pwszFileName); // PrivateProfile filename needs special class

    BOOL fRc = WritePrivateProfileStringA(strSection, strKey, strValue, strFileName);

    if (strSectionA) {
        //
        //  Write the [Section.A] key, or delete it if there is no UTF7.
        //
        WritePrivateProfileStringA(strSectionA, strKey,
#if defined(UNIX) && !defined(ux10)
                                   strUTF7 ? (LPTSTR)strValue : (LPTSTR)NULL, strFileName);
#else
                                   strUTF7 ? strValue : NULL, strFileName);
#endif

        //
        //  Now change strSectionA to strSectionW so we can write out
        //  the UTF7 encoding.
        //
        strSectionA[lstrlenA(strSectionA) - 1] = 'W';

        // This really writes [Section.W]
        WritePrivateProfileStringA(strSectionA, strKey, strUTF7, strFileName);
    }

    return fRc;
}

BOOL WINAPI
SHSetIniStringW(LPCWSTR pwszSection, LPCWSTR pwszKey, LPCWSTR pwszValue, LPCWSTR pwszFileName)
{
    // We have no way of encoding these two, so they had better by 7-bit clean
    // We also do not support "delete entire section"
    AssertMsg(pwszSection != NULL,
              TEXT("SHSetIniStringW: Section name cannot be NULL; bug in caller"));
    AssertMsg(Is7BitClean(pwszSection),
              TEXT("SHSetIniStringW: Section name not 7-bit clean; bug in caller"));
    AssertMsg(pwszKey != NULL,
              TEXT("SHSetIniStringW: Key name cannot be NULL; bug in caller"));
    AssertMsg(!pwszKey     || Is7BitClean(pwszKey),
              TEXT("SHSetIniStringW: Key name not 7-bit clean; bug in caller"));

    CStrSectionX strSectionX(pwszSection);
    CStrUTF7 strUTF7;               // Assume no UTF7 needed.

    if (strSectionX && pwszKey && pwszValue && !Is7BitClean(pwszValue)) {
        //
        //  The value is not 7-bit clean.  Must create a UTF7 version.
        //
        strUTF7.SetUnicode(pwszValue);
    }

    if (g_bRunningOnNT)
        return WritePrivateProfileStringMultiW(pwszSection, pwszValue,
                                               strSectionX, strUTF7,
                                               pwszKey,     pwszFileName);
    else
        return WritePrivateProfileStringMultiA(pwszSection, pwszValue,
                                               strSectionX, strUTF7,
                                               pwszKey,     pwszFileName);
}

//
//  Keep calling GetPrivateProfileString with bigger and bigger buffers
//  until we get the entire string.  Start with MAX_PATH, since that's
//  usually plenty big enough.
//
//  The returned buffer must be freed with LocalFree, not delete[].
//
LPVOID GetEntirePrivateProfileStringAorW(
    LPCVOID pszSection,
    LPCVOID pszKey,
    LPCVOID pszFileName,
    BOOL    fUnicode)
{
    int    CharSize = fUnicode ? sizeof(WCHAR) : sizeof(CHAR);
    UINT   cchResult = MAX_PATH;
    LPVOID pszResult = LocalAlloc(LMEM_FIXED, cchResult * CharSize);
    LPVOID pszFree = pszResult;

    while (pszResult) {
        UINT cchRc;
        if (fUnicode)
            cchRc = GetPrivateProfileStringW((LPCWSTR)pszSection,
                                             (LPCWSTR)pszKey,
                                             L"",
                                             (LPWSTR)pszResult, cchResult,
                                             (LPCWSTR)pszFileName);
        else
            cchRc = GetPrivateProfileStringA((LPCSTR)pszSection,
                                             (LPCSTR)pszKey,
                                             "",
                                             (LPSTR)pszResult, cchResult,
                                             (LPCSTR)pszFileName);

        if (cchRc < cchResult - 1)
            return pszResult;

        // Buffer too small - iterate
        cchResult *= 2;
        LPVOID pszNew = LocalReAlloc(pszResult, cchResult * CharSize, LMEM_MOVEABLE);
        pszFree = pszResult;
        pszResult = pszNew;
    }

    //
    //  Memory allocation failed; free pszFree while we still can.
    //
    if (pszFree)
        LocalFree(pszFree);
    return NULL;
}

DWORD GetPrivateProfileStringMultiW(LPCWSTR pwszSection, LPCWSTR pwszKey,
                                    LPWSTR pwszSectionA,
                                    LPWSTR pwszReturnedString, DWORD cchSize,
                                    LPCWSTR pwszFileName)
{
    LPWSTR pwszValue  = NULL;
    LPWSTR pwszValueA = NULL;
    LPWSTR pwszUTF7 = NULL;
    DWORD dwRc;

    pwszValue  = (LPWSTR)GetEntirePrivateProfileStringAorW(
                              pwszSection, pwszKey,
                              pwszFileName, TRUE);
    if (pwszValue) {

        //
        //  If the value is an empty string, then don't waste your
        //  time trying to get the UNICODE version - the UNICODE version
        //  of the empty string is the empty string.
        //
        //  Otherwise, get the ANSI shadow hidden in [Section.A]
        //  and see if it matches.  If not, then the file was edited
        //  by a downlevel app and we should just use pwszValue after all.

        if (pwszValue[0] &&
            (pwszValueA = (LPWSTR)GetEntirePrivateProfileStringAorW(
                                      pwszSectionA, pwszKey,
                                      pwszFileName, TRUE)) != NULL &&
            lstrcmpW(pwszValue, pwszValueA) == 0) {

            // our shadow is still good - run with it
            // Change [Section.A] to [Section.W]
            pwszSectionA[lstrlenW(pwszSectionA) - 1] = TEXT('W');

            pwszUTF7 = (LPWSTR)GetEntirePrivateProfileStringAorW(
                                      pwszSectionA, pwszKey,
                                      pwszFileName, TRUE);

            CStrIn strUTF7(pwszUTF7);

            dwRc = 0;                   // Assume something goes wrong

            if (strUTF7) {
                dwRc = SHAnsiToUnicodeCP(CP_UTF7, strUTF7, pwszReturnedString, cchSize);
            }

            if (dwRc == 0) {
                // Problem converting to UTF7 - just use the ANSI version
                dwRc = SHUnicodeToUnicode(pwszValue, pwszReturnedString, cchSize);
            }

        } else {
            // String was empty or couldn't get [Section.A] shadow or
            // shadow doesn't match.  Just use the regular string.
            dwRc = SHUnicodeToUnicode(pwszValue, pwszReturnedString, cchSize);
        }

        // The SHXxxToYyy functions include the terminating zero,
        // which we want to exclude.
        if (dwRc > 0)
            dwRc--;

    } else {
        // Fatal error reading values from file; just use the boring API
        dwRc = GetPrivateProfileStringW(pwszSection,
                                        pwszKey,
                                        L"",
                                        pwszReturnedString, cchSize,
                                        pwszFileName);
    }

    if (pwszValue)
        LocalFree(pwszValue);
    if (pwszValueA)
        LocalFree(pwszValueA);
    if (pwszUTF7)
        LocalFree(pwszUTF7);

    return dwRc;
}

DWORD GetPrivateProfileStringMultiA(LPCWSTR pwszSection, LPCWSTR pwszKey,
                                    LPWSTR pwszSectionA,
                                    LPWSTR pwszReturnedString, DWORD cchSize,
                                    LPCWSTR pwszFileName)
{
    CStrIn strSection(pwszSection);
    CStrIn strSectionA(pwszSectionA);
    CStrIn strKey(pwszKey);
    CPPFIn strFileName(pwszFileName); // PrivateProfile filename needs special class

    LPSTR pszValue  = NULL;
    LPSTR pszValueA = NULL;
    LPSTR pszUTF7   = NULL;
    DWORD dwRc;

    if (strSectionA &&
        (pszValue  = (LPSTR)GetEntirePrivateProfileStringAorW(
                                 strSection, strKey,
                                 strFileName, FALSE)) != NULL) {

        //
        //  If the value is an empty string, then don't waste your
        //  time trying to get the UNICODE version - the UNICODE version
        //  of the empty string is the empty string.
        //
        //  Otherwise, get the ANSI shadow hidden in [Section.A]
        //  and see if it matches.  If not, then the file was edited
        //  by a downlevel app and we should just use pwszValue after all.
        if (pszValue[0] &&
            (pszValueA = (LPSTR)GetEntirePrivateProfileStringAorW(
                                     strSectionA, strKey,
                                     strFileName, FALSE)) != NULL &&
            lstrcmpA(pszValue, pszValueA) == 0) {

            // our shadow is still good - run with it
            // Change [Section.A] to [Section.W]

            strSectionA[lstrlenA(strSectionA) - 1] = 'W';

            pszUTF7 = (LPSTR)GetEntirePrivateProfileStringAorW(
                                       strSectionA, strKey,
                                       strFileName, FALSE);
            dwRc = 0;                   // Assume something goes wrong

            if (pszUTF7) {
                dwRc = SHAnsiToUnicodeCP(CP_UTF7, pszUTF7, pwszReturnedString, cchSize);
            }

            if (dwRc == 0) {
                // Problem converting to UTF7 - just use the ANSI version
                dwRc = SHAnsiToUnicode(pszValue, pwszReturnedString, cchSize);
            }

        } else {
            // String was empty or couldn't get [Section.A] shadow or
            // shadow doesn't match.  Just use the regular string.
            dwRc = SHAnsiToUnicode(pszValue, pwszReturnedString, cchSize);
        }

        // The SHXxxToYyy functions include the terminating zero,
        // which we want to exclude.
        if (dwRc > 0)
            dwRc--;

    } else {
        // Fatal error reading values from file; just use the boring API
        CStrOut strOut(pwszReturnedString, cchSize);
        dwRc = GetPrivateProfileStringA(strSection,
                                        strKey,
                                        "",
                                        strOut, cchSize,
                                        strFileName);
        strOut.ConvertIncludingNul();
    }

    if (pszValue)
        LocalFree(pszValue);
    if (pszValueA)
        LocalFree(pszValueA);
    if (pszUTF7)
        LocalFree(pszUTF7);


    return dwRc;
}

DWORD WINAPI SHGetIniStringW(LPCWSTR pwszSection, LPCWSTR pwszKey, LPWSTR pwszReturnedString, DWORD cchSize, LPCWSTR pwszFileName)
{
    VALIDATE_OUTBUF(pwszReturnedString, cchSize);

    // We have no way of encoding these two, so they had better by 7-bit clean
    // We also do not support "get all section names" or "get entire section".
    AssertMsg(pwszSection != NULL,
              TEXT("SHGetIniStringW: Section name cannot be NULL; bug in caller"));
    AssertMsg(Is7BitClean(pwszSection),
              TEXT("SHGetIniStringW: Section name not 7-bit clean; bug in caller"));
    AssertMsg(pwszKey != NULL,
              TEXT("SHGetIniStringW: Key name cannot be NULL; bug in caller"));
    AssertMsg(Is7BitClean(pwszKey),
              TEXT("SHGetIniStringW: Key name not 7-bit clean; bug in caller"));

    CStrSectionX strSectionX(pwszSection);

    if (g_bRunningOnNT)
        return GetPrivateProfileStringMultiW(pwszSection, pwszKey,
                                             strSectionX,
                                             pwszReturnedString, cchSize,
                                             pwszFileName);
    else
        return GetPrivateProfileStringMultiA(pwszSection, pwszKey,
                                             strSectionX,
                                             pwszReturnedString, cchSize,
                                             pwszFileName);
}

//+---------------------------------------------------------------------------
//
//  CreateURLFileContents
//
//  shdocvw.dll and url.dll need to create in-memory images
//  of URL files, so this helper function does all the grunky work so they
//  can remain insulated from the way we encode Unicode strings into INI files.
//  The resulting memory should be freed via GlobalFree().

//
//  Writes a string into the URL file.  If fWrite is FALSE, then
//  then just do the math and don't actually write anything.  This lets us
//  use one function to handle both the measurement pass and the rendering
//  pass.
//
LPSTR AddToURLFileContents(LPSTR pszFile, LPCSTR psz, BOOL fWrite)
{
    int cch = lstrlenA(psz);
    if (fWrite) {
        memcpy(pszFile, psz, cch);
    }
    pszFile += cch;
    return pszFile;
}

LPSTR AddURLFileSection(LPSTR pszFile, LPCSTR pszSuffix, LPCSTR pszUrl, BOOL fWrite)
{
    pszFile = AddToURLFileContents(pszFile, "[InternetShortcut", fWrite);
    pszFile = AddToURLFileContents(pszFile, pszSuffix, fWrite);
    pszFile = AddToURLFileContents(pszFile, "]\r\nURL=", fWrite);
    pszFile = AddToURLFileContents(pszFile, pszUrl, fWrite);
    pszFile = AddToURLFileContents(pszFile, "\r\n", fWrite);
    return pszFile;
}

//
//  The file consists of an [InternetShortcut] section, followed if
//  necessary by [InternetShortcut.A] and [InternetShortcut.W].
//
LPSTR AddURLFileContents(LPSTR pszFile, LPCSTR pszUrl, LPCSTR pszUTF7, BOOL fWrite)
{
    pszFile = AddURLFileSection(pszFile, "", pszUrl, fWrite);
    if (pszUTF7) {
        pszFile = AddURLFileSection(pszFile, ".A", pszUrl, fWrite);
        pszFile = AddURLFileSection(pszFile, ".W", pszUTF7, fWrite);
    }
    return pszFile;
}

//
//  Returns number of bytes in file contents (not including trailing NULL),
//  or an OLE error code.  If ppszOut is NULL, then no contents are returned.
//
HRESULT GenerateURLFileContents(LPCWSTR pwszUrl, LPCSTR pszUrl, LPSTR *ppszOut)
{
    HRESULT hr = 0;

    if (ppszOut)
        *ppszOut = NULL;

    if (pwszUrl && pszUrl) {
        CStrUTF7 strUTF7;               // Assume no UTF7 needed.
        if (!Is7BitClean(pwszUrl)) {
            //
            //  The value is not 7-bit clean.  Must create a UTF7 version.
            //
            strUTF7.SetUnicode(pwszUrl);
        }

        hr = PtrToUlong(AddURLFileContents(NULL, pszUrl, strUTF7, FALSE));
        ASSERT(SUCCEEDED(hr));

        if (ppszOut) {
            *ppszOut = (LPSTR)GlobalAlloc(GMEM_FIXED, hr + 1);
            if (*ppszOut) {
                LPSTR pszEnd = AddURLFileContents(*ppszOut, pszUrl, strUTF7, TRUE);
                *pszEnd = '\0';
            } else {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    //
    //  Double-check the value we return.
    //
    if (SUCCEEDED(hr) && ppszOut) {
        ASSERT(hr == lstrlenA(*ppszOut));
    }

    return hr;
}


HRESULT CreateURLFileContentsW(LPCWSTR pwszUrl, LPSTR *ppszOut)
{
    CStrIn strUrl(pwszUrl);
    return GenerateURLFileContents(pwszUrl, strUrl, ppszOut);
}

HRESULT CreateURLFileContentsA(LPCSTR pszUrl, LPSTR *ppszOut)
{
    CStrInW strUrl(pszUrl);
    return GenerateURLFileContents(strUrl, pszUrl, ppszOut);
}
