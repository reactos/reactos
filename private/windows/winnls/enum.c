/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    enum.c

Abstract:

    This file contains functions that enumerate the user's portion of the
    registry for installed and supported locale ids and code page ids.

    APIs found in this file:
      EnumSystemLanguageGroupsW
      EnumLanguageGroupLocalesW
      EnumUILanguagesW
      EnumSystemLocalesW
      EnumSystemCodePagesW
      EnumCalendarInfoW
      EnumCalendarInfoExW
      EnumTimeFormatsW
      EnumDateFormatsW
      EnumDateFormatsExW

Revision History:

    08-02-93    JulieB    Created.

--*/



//
//  Include Files.
//

#include "nls.h"




//
//  Constant Declarations
//

#define ENUM_BUF_SIZE        9    // buffer size (wchar) for lcid or cpid (incl null)
#define ENUM_MAX_CP_SIZE     5    // max size (wchar) for cp id in registry
#define ENUM_LOCALE_SIZE     8    // buffer size (wchar) for locale id in registry
#define ENUM_MAX_LG_SIZE     2    // max size (wchar) for language group id in registry
#define ENUM_MAX_UILANG_SIZE 4    // max size (wchar) for UI langguage id in registry




//
//  Forward Declarations.
//

int
GetLocalizedLanguageGroupName(
    LGRPID LanguageGroup,
    LPWSTR pLangGroupName,
    int cchName);

BOOL
EnumDateTime(
    NLS_ENUMPROC lpDateTimeFmtEnumProc,
    LCID Locale,
    LCTYPE LCType,
    DWORD dwFlags,
    LPWSTR pCacheValue,
    LPWSTR pRegValue,
    PLOCALE_VAR pLocaleHdr,
    LPWSTR pDateTime,
    LPWSTR pEndDateTime,
    ULONG CalDateOffset,
    ULONG EndCalDateOffset,
    BOOL fCalendarInfo,
    BOOL fUnicodeVer,
    BOOL fExVersion);





//-------------------------------------------------------------------------//
//                            INTERNAL MACROS                              //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  NLS_CALL_ENUMPROC_BREAK
//
//  Calls the appropriate EnumProc routine.  If the fUnicodeVer flag is TRUE,
//  then it calls the Unicode version of the callback function.  Otherwise,
//  it calls the Ansi dispatch routine to translate the string to Ansi and
//  then call the Ansi version of the callback function.
//
//  This macro will do a break if the enumeration routine returns FALSE.
//
//  DEFINED AS A MACRO.
//
//  11-10-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define NLS_CALL_ENUMPROC_BREAK( Locale,                                   \
                                 lpNlsEnumProc,                            \
                                 dwFlags,                                  \
                                 pUnicodeBuffer,                           \
                                 fUnicodeVer )                             \
{                                                                          \
    /*                                                                     \
     *  Call the appropriate callback function.                            \
     */                                                                    \
    if (fUnicodeVer)                                                       \
    {                                                                      \
        /*                                                                 \
         *  Call the Unicode callback function.                            \
         */                                                                \
        if (((*lpNlsEnumProc)(pUnicodeBuffer)) != TRUE)                    \
        {                                                                  \
            break;                                                         \
        }                                                                  \
    }                                                                      \
    else                                                                   \
    {                                                                      \
        /*                                                                 \
         *  Call the Ansi callback function.                               \
         */                                                                \
        if (NlsDispatchAnsiEnumProc( Locale,                               \
                                     lpNlsEnumProc,                        \
                                     dwFlags,                              \
                                     pUnicodeBuffer,                       \
                                     NULL,                                 \
                                     0,                                    \
                                     0,                                    \
                                     0,                                    \
                                     0 ) != TRUE)                          \
        {                                                                  \
            break;                                                         \
        }                                                                  \
    }                                                                      \
}


////////////////////////////////////////////////////////////////////////////
//
//  NLS_CALL_ENUMPROC_BREAK_2
//
//  Calls the appropriate EnumProc routine.  If the fUnicodeVer flag is TRUE,
//  then it calls the Unicode version of the callback function.  Otherwise,
//  it calls the Ansi dispatch routine to translate the strings to Ansi and
//  then call the Ansi version of the callback function.
//
//  This macro will do a break if the enumeration routine returns FALSE.
//
//  DEFINED AS A MACRO.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define NLS_CALL_ENUMPROC_BREAK_2( Locale,                                 \
                                   lpNlsEnumProc,                          \
                                   dwFlags,                                \
                                   LanguageGroup,                          \
                                   EnumLocale,                             \
                                   pUnicodeBuffer,                         \
                                   lParam,                                 \
                                   fUnicodeVer )                           \
{                                                                          \
    /*                                                                     \
     *  Call the appropriate callback function.                            \
     */                                                                    \
    if (fUnicodeVer)                                                       \
    {                                                                      \
        /*                                                                 \
         *  Call the Unicode callback function.                            \
         */                                                                \
        if (((*((NLS_ENUMPROC2)lpNlsEnumProc))( LanguageGroup,             \
                                                EnumLocale,                \
                                                pUnicodeBuffer,            \
                                                lParam )) != TRUE)         \
        {                                                                  \
            break;                                                         \
        }                                                                  \
    }                                                                      \
    else                                                                   \
    {                                                                      \
        /*                                                                 \
         *  Call the Ansi callback function.                               \
         */                                                                \
        if (NlsDispatchAnsiEnumProc( Locale,                               \
                                     lpNlsEnumProc,                        \
                                     dwFlags,                              \
                                     pUnicodeBuffer,                       \
                                     NULL,                                 \
                                     LanguageGroup,                        \
                                     EnumLocale,                           \
                                     lParam,                               \
                                     2 ) != TRUE)                          \
        {                                                                  \
            break;                                                         \
        }                                                                  \
    }                                                                      \
}


////////////////////////////////////////////////////////////////////////////
//
//  NLS_CALL_ENUMPROC_BREAK_3
//
//  Calls the appropriate EnumProc routine.  If the fUnicodeVer flag is TRUE,
//  then it calls the Unicode version of the callback function.  Otherwise,
//  it calls the Ansi dispatch routine to translate the strings to Ansi and
//  then call the Ansi version of the callback function.
//
//  This macro will do a break if the enumeration routine returns FALSE.
//
//  DEFINED AS A MACRO.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define NLS_CALL_ENUMPROC_BREAK_3( Locale,                                 \
                                   lpNlsEnumProc,                          \
                                   dwFlags,                                \
                                   LanguageGroup,                          \
                                   pUnicodeBuffer1,                        \
                                   pUnicodeBuffer2,                        \
                                   dwInstall,                              \
                                   lParam,                                 \
                                   fUnicodeVer )                           \
{                                                                          \
    /*                                                                     \
     *  Call the appropriate callback function.                            \
     */                                                                    \
    if (fUnicodeVer)                                                       \
    {                                                                      \
        /*                                                                 \
         *  Call the Unicode callback function.                            \
         */                                                                \
        if (((*((NLS_ENUMPROC3)lpNlsEnumProc))( LanguageGroup,             \
                                                pUnicodeBuffer1,           \
                                                pUnicodeBuffer2,           \
                                                (dwInstall),               \
                                                lParam )) != TRUE)         \
        {                                                                  \
            break;                                                         \
        }                                                                  \
    }                                                                      \
    else                                                                   \
    {                                                                      \
        /*                                                                 \
         *  Call the Ansi callback function.                               \
         */                                                                \
        if (NlsDispatchAnsiEnumProc( Locale,                               \
                                     lpNlsEnumProc,                        \
                                     dwFlags,                              \
                                     pUnicodeBuffer1,                      \
                                     pUnicodeBuffer2,                      \
                                     LanguageGroup,                        \
                                     (dwInstall),                          \
                                     lParam,                               \
                                     3 ) != TRUE)                          \
        {                                                                  \
            break;                                                         \
        }                                                                  \
    }                                                                      \
}

////////////////////////////////////////////////////////////////////////////
//
//  NLS_CALL_ENUMPROC_BREAK_4
//
//  Calls the appropriate EnumProc routine.  If the fUnicodeVer flag is TRUE,
//  then it calls the Unicode version of the callback function.  Otherwise,
//  it calls the Ansi dispatch routine to translate the string to Ansi and
//  then call the Ansi version of the callback function.
//
//  This macro will do a break if the enumeration routine returns FALSE.
//  Used by EnumUILanguages.
//
//  DEFINED AS A MACRO.
//
//  12-03-98    SamerA    Created.
////////////////////////////////////////////////////////////////////////////

#define NLS_CALL_ENUMPROC_BREAK_4( Locale,                                 \
                                   lpNlsEnumProc,                          \
                                   dwFlags,                                \
                                   pUnicodeBuffer,                         \
                                   lParam,                                 \
                                   fUnicodeVer )                           \
{                                                                          \
    /*                                                                     \
     *  Call the appropriate callback function.                            \
     */                                                                    \
    if (fUnicodeVer)                                                       \
    {                                                                      \
        /*                                                                 \
         *  Call the Unicode callback function.                            \
         */                                                                \
        if (((*((NLS_ENUMPROC4)lpNlsEnumProc))(pUnicodeBuffer,             \
                              lParam)) != TRUE)                            \
        {                                                                  \
            break;                                                         \
        }                                                                  \
    }                                                                      \
    else                                                                   \
    {                                                                      \
        /*                                                                 \
         *  Call the Ansi callback function.                               \
         */                                                                \
        if (NlsDispatchAnsiEnumProc( Locale,                               \
                                     lpNlsEnumProc,                        \
                                     dwFlags,                              \
                                     pUnicodeBuffer,                       \
                                     NULL,                                 \
                                     0,                                    \
                                     0,                                    \
                                     lParam,                               \
                                     4 ) != TRUE)                          \
        {                                                                  \
            break;                                                         \
        }                                                                  \
    }                                                                      \
}

////////////////////////////////////////////////////////////////////////////
//
//  NLS_CALL_ENUMPROC_TRUE_4
//
//  Calls the appropriate EnumProc routine.  If the fUnicodeVer flag is TRUE,
//  then it calls the Unicode version of the callback function.  Otherwise,
//  it calls the Ansi dispatch routine to translate the string to Ansi and
//  then call the Ansi version of the callback function.
//
//  This macro will do a break if the enumeration routine returns FALSE.
//  Used by EnumUILanguages.
//
//  DEFINED AS A MACRO.
//
//  12-03-98    SamerA    Created.
////////////////////////////////////////////////////////////////////////////

#define NLS_CALL_ENUMPROC_TRUE_4( Locale,                                  \
                                  lpNlsEnumProc,                           \
                                  dwFlags,                                 \
                                  pUnicodeBuffer,                          \
                                  lParam,                                  \
                                  fUnicodeVer )                            \
{                                                                          \
    /*                                                                     \
     *  Call the appropriate callback function.                            \
     */                                                                    \
    if (fUnicodeVer)                                                       \
    {                                                                      \
        /*                                                                 \
         *  Call the Unicode callback function.                            \
         */                                                                \
        if (((*((NLS_ENUMPROC4)lpNlsEnumProc))(pUnicodeBuffer,             \
                              lParam)) != TRUE)                            \
        {                                                                  \
            return (TRUE);                                                 \
        }                                                                  \
    }                                                                      \
    else                                                                   \
    {                                                                      \
        /*                                                                 \
         *  Call the Ansi callback function.                               \
         */                                                                \
        if (NlsDispatchAnsiEnumProc( Locale,                               \
                                     lpNlsEnumProc,                        \
                                     dwFlags,                              \
                                     pUnicodeBuffer,                       \
                                     NULL,                                 \
                                     0,                                    \
                                     0,                                    \
                                     lParam,                               \
                                     4 ) != TRUE)                          \
        {                                                                  \
            return (TRUE);                                                 \
        }                                                                  \
    }                                                                      \
}



////////////////////////////////////////////////////////////////////////////
//
//  NLS_CALL_ENUMPROC_TRUE
//
//  Calls the appropriate EnumProc routine.  If the fUnicodeVer flag is TRUE,
//  then it calls the Unicode version of the callback function.  Otherwise,
//  it calls the Ansi dispatch routine to translate the string to Ansi and
//  then call the Ansi version of the callback function.
//
//  This macro will return TRUE if the enumeration routine returns FALSE.
//
//  DEFINED AS A MACRO.
//
//  11-10-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define NLS_CALL_ENUMPROC_TRUE( Locale,                                    \
                                lpNlsEnumProc,                             \
                                dwFlags,                                   \
                                pUnicodeBuffer,                            \
                                CalId,                                     \
                                fUnicodeVer,                               \
                                fVer )                                     \
{                                                                          \
    /*                                                                     \
     *  Call the appropriate callback function.                            \
     */                                                                    \
    if (fUnicodeVer)                                                       \
    {                                                                      \
        /*                                                                 \
         *  Call the Unicode callback function.                            \
         */                                                                \
        if (fVer == 1)                                                     \
        {                                                                  \
            if (((*((NLS_ENUMPROCEX)lpNlsEnumProc))( pUnicodeBuffer,       \
                                                     CalId )) != TRUE)     \
            {                                                              \
                return (TRUE);                                             \
            }                                                              \
        }                                                                  \
        else   /* fVer == 0 */                                             \
        {                                                                  \
            if (((*lpNlsEnumProc)(pUnicodeBuffer)) != TRUE)                \
            {                                                              \
                return (TRUE);                                             \
            }                                                              \
        }                                                                  \
    }                                                                      \
    else                                                                   \
    {                                                                      \
        /*                                                                 \
         *  Call the Ansi callback function.                               \
         */                                                                \
        if (NlsDispatchAnsiEnumProc( Locale,                               \
                                     lpNlsEnumProc,                        \
                                     dwFlags,                              \
                                     pUnicodeBuffer,                       \
                                     NULL,                                 \
                                     CalId,                                \
                                     0,                                    \
                                     0,                                    \
                                     fVer ) != TRUE)                       \
        {                                                                  \
            return (TRUE);                                                 \
        }                                                                  \
    }                                                                      \
}




//-------------------------------------------------------------------------//
//                             API ROUTINES                                //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  EnumSystemLanguageGroupsW
//
//  Enumerates the system language groups that are installed or supported,
//  based on the dwFlags parameter.  It does so by passing the pointer to
//  the string buffer containing the language group id to an
//  application-defined callback function.  It continues until the last
//  language group id is found or the callback function returns FALSE.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI EnumSystemLanguageGroupsW(
    LANGUAGEGROUP_ENUMPROCW lpLanguageGroupEnumProc,
    DWORD dwFlags,
    LONG_PTR lParam)
{
    return (Internal_EnumSystemLanguageGroups(
                                       (NLS_ENUMPROC)lpLanguageGroupEnumProc,
                                       dwFlags,
                                       lParam,
                                       TRUE ));
}


////////////////////////////////////////////////////////////////////////////
//
//  EnumLanguageGroupLocalesW
//
//  Enumerates the locales in a given language group.  It does so by
//  passing the appropriate information to an application-defined
//  callback function.  It continues until the last locale in the language
//  group is found or the callback function returns FALSE.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI EnumLanguageGroupLocalesW(
    LANGGROUPLOCALE_ENUMPROCW lpLangGroupLocaleEnumProc,
    LGRPID LanguageGroup,
    DWORD dwFlags,
    LONG_PTR lParam)
{
    return (Internal_EnumLanguageGroupLocales(
                                       (NLS_ENUMPROC)lpLangGroupLocaleEnumProc,
                                       LanguageGroup,
                                       dwFlags,
                                       lParam,
                                       TRUE ));
}


////////////////////////////////////////////////////////////////////////////
//
//  EnumUILanguagesW
//
//  Enumerates the system UI languages that are installed.  It does so by
//  passing the pointer to the string buffer containing the UI language id
//  to an application-defined callback function.  It continues until the
//  last UI language id is found or the callback function returns FALSE.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI EnumUILanguagesW(
    UILANGUAGE_ENUMPROCW lpUILanguageEnumProc,
    DWORD dwFlags,
    LONG_PTR lParam)
{
    return (Internal_EnumUILanguages( (NLS_ENUMPROC)lpUILanguageEnumProc,
                                      dwFlags,
                                      lParam,
                                      TRUE ));
}


////////////////////////////////////////////////////////////////////////////
//
//  EnumSystemLocalesW
//
//  Enumerates the system locales that are installed or supported, based on
//  the dwFlags parameter.  It does so by passing the pointer to the string
//  buffer containing the locale id to an application-defined callback
//  function.  It continues until the last locale id is found or the
//  callback function returns FALSE.
//
//  08-02-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI EnumSystemLocalesW(
    LOCALE_ENUMPROCW lpLocaleEnumProc,
    DWORD dwFlags)
{
    return (Internal_EnumSystemLocales( (NLS_ENUMPROC)lpLocaleEnumProc,
                                        dwFlags,
                                        TRUE ));
}


////////////////////////////////////////////////////////////////////////////
//
//  EnumSystemCodePagesW
//
//  Enumerates the system code pages that are installed or supported, based on
//  the dwFlags parameter.  It does so by passing the pointer to the string
//  buffer containing the code page id to an application-defined callback
//  function.  It continues until the last code page is found or the
//  callback function returns FALSE.
//
//  08-02-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI EnumSystemCodePagesW(
    CODEPAGE_ENUMPROCW lpCodePageEnumProc,
    DWORD dwFlags)
{
    return (Internal_EnumSystemCodePages( (NLS_ENUMPROC)lpCodePageEnumProc,
                                          dwFlags,
                                          TRUE ));
}


////////////////////////////////////////////////////////////////////////////
//
//  EnumCalendarInfoW
//
//  Enumerates the specified calendar information that is available for the
//  specified locale, based on the CalType parameter.  It does so by
//  passing the pointer to the string buffer containing the calendar info
//  to an application-defined callback function.  It continues until the
//  last calendar info is found or the callback function returns FALSE.
//
//  10-14-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI EnumCalendarInfoW(
    CALINFO_ENUMPROCW lpCalInfoEnumProc,
    LCID Locale,
    CALID Calendar,
    CALTYPE CalType)
{
    return (Internal_EnumCalendarInfo( (NLS_ENUMPROC)lpCalInfoEnumProc,
                                       Locale,
                                       Calendar,
                                       CalType,
                                       TRUE,
                                       FALSE ));
}


////////////////////////////////////////////////////////////////////////////
//
//  EnumCalendarInfoExW
//
//  Enumerates the specified calendar information that is available for the
//  specified locale, based on the CalType parameter.  It does so by
//  passing the pointer to the string buffer containing the calendar info
//  and the calendar id to an application-defined callback function.  It
//  continues until the last calendar info is found or the callback function
//  returns FALSE.
//
//  10-14-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI EnumCalendarInfoExW(
    CALINFO_ENUMPROCEXW lpCalInfoEnumProcEx,
    LCID Locale,
    CALID Calendar,
    CALTYPE CalType)
{
    return (Internal_EnumCalendarInfo( (NLS_ENUMPROC)lpCalInfoEnumProcEx,
                                       Locale,
                                       Calendar,
                                       CalType,
                                       TRUE,
                                       TRUE ));
}


////////////////////////////////////////////////////////////////////////////
//
//  EnumTimeFormatsW
//
//  Enumerates the time formats that are available for the
//  specified locale, based on the dwFlags parameter.  It does so by
//  passing the pointer to the string buffer containing the time format
//  to an application-defined callback function.  It continues until the
//  last time format is found or the callback function returns FALSE.
//
//  10-14-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI EnumTimeFormatsW(
    TIMEFMT_ENUMPROCW lpTimeFmtEnumProc,
    LCID Locale,
    DWORD dwFlags)
{
    return (Internal_EnumTimeFormats( (NLS_ENUMPROC)lpTimeFmtEnumProc,
                                       Locale,
                                       dwFlags,
                                       TRUE ));
}


////////////////////////////////////////////////////////////////////////////
//
//  EnumDateFormatsW
//
//  Enumerates the short date, long date, or year/month formats that are
//  available for the specified locale, based on the dwFlags parameter.
//  It does so by passing the pointer to the string buffer containing the
//  date format to an application-defined callback function.  It continues
//  until the last date format is found or the callback function returns
//  FALSE.
//
//  10-14-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI EnumDateFormatsW(
    DATEFMT_ENUMPROCW lpDateFmtEnumProc,
    LCID Locale,
    DWORD dwFlags)
{
    return (Internal_EnumDateFormats( (NLS_ENUMPROC)lpDateFmtEnumProc,
                                       Locale,
                                       dwFlags,
                                       TRUE,
                                       FALSE ));
}


////////////////////////////////////////////////////////////////////////////
//
//  EnumDateFormatsExW
//
//  Enumerates the short date, long date, or year/month formats that are
//  available for the specified locale, based on the dwFlags parameter.
//  It does so by passing the pointer to the string buffer containing the
//  date format and the calendar id to an application-defined callback
//  function.  It continues until the last date format is found or the
//  callback function returns FALSE.
//
//  10-14-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI EnumDateFormatsExW(
    DATEFMT_ENUMPROCEXW lpDateFmtEnumProcEx,
    LCID Locale,
    DWORD dwFlags)
{
    return (Internal_EnumDateFormats( (NLS_ENUMPROC)lpDateFmtEnumProcEx,
                                       Locale,
                                       dwFlags,
                                       TRUE,
                                       TRUE ));
}




//-------------------------------------------------------------------------//
//                           EXTERNAL ROUTINES                             //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  Internal_EnumSystemLanguageGroups
//
//  Enumerates the system language groups that are installed or supported,
//  based on the dwFlags parameter.  It does so by passing the pointer to
//  the string buffer containing the language group id to an
//  application-defined callback function.  It continues until the last
//  language group id is found or the callback function returns FALSE.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL Internal_EnumSystemLanguageGroups(
    NLS_ENUMPROC lpLanguageGroupEnumProc,
    DWORD dwFlags,
    LONG_PTR lParam,
    BOOL fUnicodeVer)
{
    PKEY_VALUE_FULL_INFORMATION pKeyValueFull = NULL;
    BYTE pStatic[MAX_KEY_VALUE_FULLINFO];

    BOOL fInstalled;                   // if installed flag set
    ULONG Index;                       // index for enumeration
    ULONG ResultLength;                // # bytes written
    WCHAR wch;                         // first char of name
    LPWSTR pName;                      // ptr to name string from registry
    WCHAR szLGName[MAX_PATH];          // language group name
    UNICODE_STRING ObUnicodeStr;       // registry data value string
    DWORD Data;                        // registry data value
    ULONG NameLen;                     // length of name string
    LGRPID LangGroup;                    // language group id
    ULONG rc = 0L;                     // return code


    //
    //  Invalid Parameter Check:
    //    - function pointer is null
    //
    if (lpLanguageGroupEnumProc == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    //
    //  Invalid Flags Check:
    //    - flags other than valid ones
    //    - more than one of either supported or installed
    //
    if ( (dwFlags & ESLG_INVALID_FLAG) ||
         (MORE_THAN_ONE(dwFlags, ESLG_SINGLE_FLAG)) )
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return (FALSE);
    }

    //
    //  Initialize flag option.
    //
    fInstalled = dwFlags & LGRPID_INSTALLED;

    //
    //  Initialize key handles.
    //
    OPEN_LANG_GROUPS_KEY(FALSE);

    //
    //  Loop through the language group ids in the registry, call the
    //  function pointer for each one that meets the flag criteria.
    //
    //  End loop if either FALSE is returned from the callback function
    //  or the end of the list is reached.
    //
    Index = 0;
    pKeyValueFull = (PKEY_VALUE_FULL_INFORMATION)pStatic;
    RtlZeroMemory(pKeyValueFull, MAX_KEY_VALUE_FULLINFO);
    rc = NtEnumerateValueKey( hLangGroupsKey,
                              Index,
                              KeyValueFullInformation,
                              pKeyValueFull,
                              MAX_KEY_VALUE_FULLINFO,
                              &ResultLength );

    while (rc != STATUS_NO_MORE_ENTRIES)
    {
        if (!NT_SUCCESS(rc))
        {
            //
            //  If we get a different error, then the registry
            //  is corrupt.  Just return FALSE.
            //
            KdPrint(("NLSAPI: Language Group Enumeration Error - registry corrupt. - %lx.\n",
                     rc));
            SetLastError(ERROR_BADDB);
            return (FALSE);
        }

        //
        //  Skip over any entry that does not have data associated with it
        //  if the LGRPID_INSTALLED flag is set.
        //
        pName = pKeyValueFull->Name;
        wch = *pName;
        NameLen = pKeyValueFull->NameLength / sizeof(WCHAR);
        if ( (NameLen <= ENUM_MAX_LG_SIZE) &&
             (((wch >= NLS_CHAR_ZERO) && (wch <= NLS_CHAR_NINE)) ||
              (((wch | 0x0020) >= L'a') && ((wch | 0x0020) <= L'f'))) &&
              (!((fInstalled) && (pKeyValueFull->DataLength <= 2))) )
        {
            //
            //  See if the language group is installed or not.
            //
            Data = 0;
            if (pKeyValueFull->DataLength > 2)
            {
                RtlInitUnicodeString( &ObUnicodeStr,
                                      GET_VALUE_DATA_PTR(pKeyValueFull) );

                if (RtlUnicodeStringToInteger(&ObUnicodeStr, 16, &Data))
                {
                    Data = 0;
                }
            }

            //
            //  If the installed flag is set, then skip the language group
            //  if it is not already installed.
            //
            if ((fInstalled) && (Data != 1))
            {
                goto EnumNextLanguageGroup;
            }

            //
            //  Store the language group id string in the callback buffer.
            //
            pName[NameLen] = 0;

            //
            //  Get the language group id as a value and the localized
            //  language group name.
            //
            RtlInitUnicodeString(&ObUnicodeStr, pName);
            if ((RtlUnicodeStringToInteger(&ObUnicodeStr, 16, &LangGroup)) ||
                (!GetLocalizedLanguageGroupName(LangGroup, szLGName, MAX_PATH)))
            {
                goto EnumNextLanguageGroup;
            }

            //
            //  Call the appropriate callback function.
            //
            NLS_CALL_ENUMPROC_BREAK_3( gSystemLocale,
                                       lpLanguageGroupEnumProc,
                                       dwFlags,
                                       LangGroup,
                                       pName,
                                       szLGName,
                                       (Data == 1)
                                           ? LGRPID_INSTALLED
                                           : LGRPID_SUPPORTED,
                                       lParam,
                                       fUnicodeVer );
        }

EnumNextLanguageGroup:
        //
        //  Increment enumeration index value and get the next enumeration.
        //
        Index++;
        RtlZeroMemory(pKeyValueFull, MAX_KEY_VALUE_FULLINFO);
        rc = NtEnumerateValueKey( hLangGroupsKey,
                                  Index,
                                  KeyValueFullInformation,
                                  pKeyValueFull,
                                  MAX_KEY_VALUE_FULLINFO,
                                  &ResultLength );
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Internal_EnumLanguageGroupLocales
//
//  Enumerates the locales in a given language group.  It does so by
//  passing the appropriate information to an application-defined
//  callback function.  It continues until the last locale in the language
//  group is found or the callback function returns FALSE.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL Internal_EnumLanguageGroupLocales(
    NLS_ENUMPROC lpLangGroupLocaleEnumProc,
    LGRPID LanguageGroup,
    DWORD dwFlags,
    LONG_PTR lParam,
    BOOL fUnicodeVer)
{
    UNICODE_STRING ObUnicodeStr;            // locale string
    WCHAR szSectionName[MAX_PATH];          // section name in inf file
    WCHAR szBuffer[MAX_PATH * 4];           // buffer
    WCHAR szInfPath[MAX_PATH_LEN];          // inf file
    LPWSTR pStr, pEndStr;                   // ptr to szBuffer
    DWORD LocaleValue;                      // locale id value
    int Length;                             // length of string in buffer


    //
    //  Invalid Parameter Check:
    //    - function pointer is null
    //
    if (lpLangGroupLocaleEnumProc == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    //
    //  Invalid Flags Check:
    //    - flags must be 0
    //
    if (dwFlags != 0)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return (FALSE);
    }

    //
    //  Get INTL.INF section name - LOCALE_LIST_#.
    //
    if (NlsConvertIntegerToString( LanguageGroup,
                                   10,
                                   1,
                                   szBuffer,
                                   ENUM_BUF_SIZE ) != NO_ERROR)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }
    NlsStrCpyW(szSectionName, L"LOCALE_LIST_");
    NlsStrCatW(szSectionName, szBuffer);

    //
    //  Get the locale list from the intl.inf file.
    //
    szBuffer[0] = 0;
    GetSystemWindowsDirectory(szInfPath, MAX_PATH_LEN);
    NlsStrCatW(szInfPath, L"\\INF\\INTL.INF");
    Length = GetPrivateProfileSection( szSectionName,
                                       szBuffer,
                                       MAX_PATH * 4,
                                       szInfPath );
    if (Length == 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    //
    //  Parse the buffer and call the callback function for each locale
    //  in the list.  The buffer is double null terminated.
    //
    pStr = szBuffer;
    pEndStr = szBuffer + Length;
    while ((pStr < pEndStr) && (*pStr))
    {
        //
        //  See if the value starts with 0x or 0X.  If so, go past it.
        //
        if ((*pStr == L'0') &&
            ((*(pStr + 1) == L'x') || (*(pStr + 1) == L'X')))
        {
            pStr += 2;
        }

        //
        //  Convert the string to an integer.
        //
        RtlInitUnicodeString(&ObUnicodeStr, pStr);
        if (RtlUnicodeStringToInteger(&ObUnicodeStr, 16, &LocaleValue) != NO_ERROR)
        {
            KdPrint(("NLSAPI: Language Group Locale Enumeration Error - intl.inf corrupt.\n"));
            SetLastError(ERROR_BADDB);
            return (FALSE);
        }

        //
        //  Call the appropriate callback function.
        //
        NLS_CALL_ENUMPROC_BREAK_2( gSystemLocale,
                                   lpLangGroupLocaleEnumProc,
                                   dwFlags,
                                   LanguageGroup,
                                   LocaleValue,
                                   pStr,
                                   lParam,
                                   fUnicodeVer );

        //
        //  Increment the pointer to the next string.
        //
        while (*pStr)
        {
            pStr++;
        }
        pStr++;
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Internal_EnumUILanguages
//
//  Enumerates the system UI languages that are installed.  It does so by
//  passing the pointer to the string buffer containing the UI language id
//  to an application-defined callback function.  It continues until the
//  last UI language id is found or the callback function returns FALSE.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL Internal_EnumUILanguages(
    NLS_ENUMPROC lpUILanguageEnumProc,
    DWORD dwFlags,
    LONG_PTR lParam,
    BOOL fUnicodeVer)
{
    PKEY_VALUE_FULL_INFORMATION pKeyValueFull = NULL;
    BYTE pStatic[MAX_KEY_VALUE_FULLINFO];

    LANGID LangID;                     // language id
    WCHAR szLang[MAX_PATH];            // language id string
    HANDLE hKey = NULL;                // handle to muilang key
    ULONG Index;                       // index for enumeration
    ULONG ResultLength;                // # bytes written
    WCHAR wch;                         // first char of name
    LPWSTR pName;                      // ptr to name string from registry
    ULONG NameLen;                     // length of name string
    ULONG rc = 0L;                     // return code


    //
    //  Invalid Parameter Check:
    //    - function pointer is null
    //
    if (lpUILanguageEnumProc == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    //
    //  Invalid Flags Check:
    //    - flags must be 0
    //
    if (dwFlags != 0)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return (FALSE);
    }

    //
    //  Call the appropriate callback function with the user's UI
    //  language.
    //
    LangID = GetSystemDefaultUILanguage();
    if (NlsConvertIntegerToString(LangID, 16, 4, szLang, MAX_PATH) == NO_ERROR)
    {
        NLS_CALL_ENUMPROC_TRUE_4( gSystemLocale,
                                  lpUILanguageEnumProc,
                                  dwFlags,
                                  szLang,
                                  lParam,
                                  fUnicodeVer);
    }
    else
    {
        szLang[0] = 0;
    }

    //
    //  Open the MUILanguages registry key.  It is acceptable if the key
    //  does not exist, so return TRUE as there are no items to enumerate.
    //
    OPEN_MUILANG_KEY(hKey, TRUE);

    //
    //  Loop through the MUILanguage ids in the registry, call the
    //  function pointer for each.
    //
    //  End loop if either FALSE is returned from the callback function
    //  or the end of the list is reached.
    //
    Index = 0;
    pKeyValueFull = (PKEY_VALUE_FULL_INFORMATION)pStatic;
    RtlZeroMemory(pKeyValueFull, MAX_KEY_VALUE_FULLINFO);
    rc = NtEnumerateValueKey( hKey,
                              Index,
                              KeyValueFullInformation,
                              pKeyValueFull,
                              MAX_KEY_VALUE_FULLINFO,
                              &ResultLength );

    while (rc != STATUS_NO_MORE_ENTRIES)
    {
        if (!NT_SUCCESS(rc))
        {
            //
            //  If we get a different error, then the registry
            //  is corrupt.  Just return FALSE.
            //
            KdPrint(("NLSAPI: MUI Languages Enumeration Error - registry corrupt. - %lx.\n",
                     rc));
            SetLastError(ERROR_BADDB);
            return (FALSE);
        }

        //
        //  Skip over any entry that does not have data associated with it.
        //
        pName = pKeyValueFull->Name;
        wch = *pName;
        NameLen = pKeyValueFull->NameLength / sizeof(WCHAR);
        if ( (NameLen == ENUM_MAX_UILANG_SIZE) &&
             (((wch >= NLS_CHAR_ZERO) && (wch <= NLS_CHAR_NINE)) ||
              (((wch | 0x0020) >= L'a') && ((wch | 0x0020) <= L'f'))) &&
              (pKeyValueFull->DataLength > 2) )
        {
            //
            //  Make sure the UI language is zero terminated.
            //
            pName[NameLen] = 0;

            //
            //  Make sure it's not the same as the user UI language
            //  that we already enumerated.
            //
            if (lstrcmp(szLang, pName) != 0)
            {
                //
                //  Call the appropriate callback function.
                //
                NLS_CALL_ENUMPROC_BREAK_4( gSystemLocale,
                                           lpUILanguageEnumProc,
                                           dwFlags,
                                           pName,
                                           lParam,
                                           fUnicodeVer );
            }
        }

        //
        //  Increment enumeration index value and get the next enumeration.
        //
        Index++;
        RtlZeroMemory(pKeyValueFull, MAX_KEY_VALUE_FULLINFO);
        rc = NtEnumerateValueKey( hKey,
                                  Index,
                                  KeyValueFullInformation,
                                  pKeyValueFull,
                                  MAX_KEY_VALUE_FULLINFO,
                                  &ResultLength );
    }

    //
    //  Close the registry key.
    //
    CLOSE_REG_KEY(hKey);

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Internal_EnumSystemLocales
//
//  Enumerates the system locales that are installed or supported, based on
//  the dwFlags parameter.  It does so by passing the pointer to the string
//  buffer containing the locale id to an application-defined callback
//  function.  It continues until the last locale id is found or the
//  callback function returns FALSE.
//
//  08-02-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL Internal_EnumSystemLocales(
    NLS_ENUMPROC lpLocaleEnumProc,
    DWORD dwFlags,
    BOOL fUnicodeVer)
{
    PKEY_VALUE_FULL_INFORMATION pKeyValueFull1 = NULL;
    PKEY_VALUE_FULL_INFORMATION pKeyValueFull2 = NULL;
    BYTE pStatic1[MAX_KEY_VALUE_FULLINFO];
    BYTE pStatic2[MAX_KEY_VALUE_FULLINFO];

    BOOL fInstalled;                   // if installed flag set
    ULONG Index;                       // index for enumeration
    ULONG ResultLength;                // # bytes written
    WCHAR wch;                         // first char of name
    WCHAR pBuffer[ENUM_BUF_SIZE];      // ptr to callback string buffer
    LPWSTR pName;                      // ptr to name string from registry
    LPWSTR pData;                      // ptr to data string from registry
    UNICODE_STRING ObUnicodeStr;       // registry data value string
    DWORD Data;                        // registry data value
    HKEY hKey;                         // handle to registry key
    int Ctr;                           // loop counter
    ULONG rc = 0L;                     // return code


    //
    //  Invalid Parameter Check:
    //    - function pointer is null
    //
    if (lpLocaleEnumProc == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    //
    //  Invalid Flags Check:
    //    - flags other than valid ones
    //    - more than one of either supported or installed
    //
    if ( (dwFlags & ESL_INVALID_FLAG) ||
         (MORE_THAN_ONE(dwFlags, ESL_SINGLE_FLAG)) )
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return (FALSE);
    }

    //
    //  Initialize flag option.
    //
    fInstalled = dwFlags & LCID_INSTALLED;

    //
    //  Initialize key handles.
    //
    OPEN_LOCALE_KEY(FALSE);
    OPEN_ALT_SORTS_KEY(FALSE);
    OPEN_LANG_GROUPS_KEY(FALSE);

    //
    //  Initialize the variables for the loop.
    //
    Ctr = 0;
    if (dwFlags & LCID_ALTERNATE_SORTS)
    {
        Ctr++;
        hKey = hAltSortsKey;
    }
    if (dwFlags != LCID_ALTERNATE_SORTS)
    {
        Ctr++;
        hKey = hLocaleKey;
    }

    //
    //  Loop through the locale ids and/or the alternate sort ids.
    //
    for (; Ctr > 0; Ctr--)
    {
        //
        //  Loop through the locale ids in the registry, call the function
        //  pointer for each one that meets the flag criteria.
        //
        //  End loop if either FALSE is returned from the callback function
        //  or the end of the list is reached.
        //
        //  Always need to ignore the DEFAULT entry.
        //
        Index = 0;
        pKeyValueFull1 = (PKEY_VALUE_FULL_INFORMATION)pStatic1;
        pKeyValueFull2 = (PKEY_VALUE_FULL_INFORMATION)pStatic2;
        RtlZeroMemory(pKeyValueFull1, MAX_KEY_VALUE_FULLINFO);
        rc = NtEnumerateValueKey( hKey,
                                  Index,
                                  KeyValueFullInformation,
                                  pKeyValueFull1,
                                  MAX_KEY_VALUE_FULLINFO,
                                  &ResultLength );

        while (rc != STATUS_NO_MORE_ENTRIES)
        {
            if (!NT_SUCCESS(rc))
            {
                //
                //  If we get a different error, then the registry
                //  is corrupt.  Just return FALSE.
                //
                KdPrint(("NLSAPI: LCID Enumeration Error - registry corrupt. - %lx.\n",
                         rc));
                SetLastError(ERROR_BADDB);
                return (FALSE);
            }

            //
            //  Skip over the Default entry in the registry and any
            //  entry that does not have data associated with it if the
            //  LCID_INSTALLED flag is set.
            //
            pName = pKeyValueFull1->Name;
            wch = *pName;
            if ((pKeyValueFull1->NameLength == (ENUM_LOCALE_SIZE * sizeof(WCHAR))) &&
                (((wch >= NLS_CHAR_ZERO) && (wch <= NLS_CHAR_NINE)) ||
                 (((wch | 0x0020) >= L'a') && ((wch | 0x0020) <= L'f'))))
            {
                //
                //  If the installed flag is set, then do some extra
                //  validation before calling the function proc.
                //
                if (fInstalled)
                {
                    if (pKeyValueFull1->DataLength <= 2)
                    {
                        goto EnumNextLocale;
                    }

                    RtlInitUnicodeString( &ObUnicodeStr,
                                          GET_VALUE_DATA_PTR(pKeyValueFull1) );

                    if ((RtlUnicodeStringToInteger(&ObUnicodeStr, 16, &Data)) ||
                        (Data == 0) ||
                        (QueryRegValue( hLangGroupsKey,
                                        ObUnicodeStr.Buffer,
                                        &pKeyValueFull2,
                                        MAX_KEY_VALUE_FULLINFO,
                                        NULL ) != NO_ERROR) ||
                        (pKeyValueFull2->DataLength <= 2))
                    {
                        goto EnumNextLocale;
                    }
                    pData = GET_VALUE_DATA_PTR(pKeyValueFull2);
                    if ((pData[0] != L'1') || (pData[1] != 0))
                    {
                        goto EnumNextLocale;
                    }
                }

                //
                //  Store the locale id in the callback buffer.
                //
                *(pBuffer) = *pName;
                *(pBuffer + 1) = *(pName + 1);
                *(pBuffer + 2) = *(pName + 2);
                *(pBuffer + 3) = *(pName + 3);
                *(pBuffer + 4) = *(pName + 4);
                *(pBuffer + 5) = *(pName + 5);
                *(pBuffer + 6) = *(pName + 6);
                *(pBuffer + 7) = *(pName + 7);

                *(pBuffer + 8) = 0;

                //
                //  Call the appropriate callback function.
                //
                NLS_CALL_ENUMPROC_BREAK( gSystemLocale,
                                         lpLocaleEnumProc,
                                         dwFlags,
                                         pBuffer,
                                         fUnicodeVer );
            }

EnumNextLocale:
            //
            //  Increment enumeration index value and get the next enumeration.
            //
            Index++;
            RtlZeroMemory(pKeyValueFull1, MAX_KEY_VALUE_FULLINFO);
            rc = NtEnumerateValueKey( hKey,
                                      Index,
                                      KeyValueFullInformation,
                                      pKeyValueFull1,
                                      MAX_KEY_VALUE_FULLINFO,
                                      &ResultLength );
        }

        //
        //  The counter can be either 1 or 2 at this point.  If it's 2, then
        //  we've just done the Locale key and we need to do the alternate
        //  sorts key.  If it's 1, then it doesn't matter what this is set to
        //  since we're done with the loop.
        //
        hKey = hAltSortsKey;
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Internal_EnumSystemCodePages
//
//  Enumerates the system code pages that are installed or supported, based
//  on the dwFlags parameter.  It does so by passing the pointer to the
//  string buffer containing the code page id to an application-defined
//  callback function.  It continues until the last code page is found or
//  the callback function returns FALSE.
//
//  08-02-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL Internal_EnumSystemCodePages(
    NLS_ENUMPROC lpCodePageEnumProc,
    DWORD dwFlags,
    BOOL fUnicodeVer)
{
    PKEY_VALUE_FULL_INFORMATION pKeyValueFull = NULL;
    BYTE pStatic[MAX_KEY_VALUE_FULLINFO];

    BOOL fInstalled;              // if installed flag set
    ULONG Index = 0;              // index for enumeration
    ULONG ResultLength;           // # bytes written
    WCHAR wch;                    // first char of name
    LPWSTR pName;                 // ptr to name string from registry
    ULONG NameLen;                // length of name string
    ULONG rc = 0L;                // return code


    //
    //  Invalid Parameter Check:
    //    - function pointer is null
    //
    if (lpCodePageEnumProc == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    //
    //  Invalid Flags Check:
    //    - flags other than valid ones
    //    - more than one of either supported or installed
    //
    if ( (dwFlags & ESCP_INVALID_FLAG) ||
         (MORE_THAN_ONE(dwFlags, ESCP_SINGLE_FLAG)) )
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return (FALSE);
    }

    //
    //  Initialize flag option.
    //
    fInstalled = dwFlags & CP_INSTALLED;

    //
    //  Loop through the code page ids in the registry, call the function
    //  pointer for each one that meets the flag criteria.
    //
    //  End loop if either FALSE is returned from the callback function
    //  or the end of the list is reached.
    //
    //  Always need to ignore the ACP, OEMCP, MACCP, and OEMHAL entries.
    //
    OPEN_CODEPAGE_KEY(FALSE);

    pKeyValueFull = (PKEY_VALUE_FULL_INFORMATION)pStatic;
    RtlZeroMemory(pKeyValueFull, MAX_KEY_VALUE_FULLINFO);
    rc = NtEnumerateValueKey( hCodePageKey,
                              Index,
                              KeyValueFullInformation,
                              pKeyValueFull,
                              MAX_KEY_VALUE_FULLINFO,
                              &ResultLength );

    while (rc != STATUS_NO_MORE_ENTRIES)
    {
        if (!NT_SUCCESS(rc))
        {
            //
            //  If we get a different error, then the registry
            //  is corrupt.  Just return FALSE.
            //
            KdPrint(("NLSAPI: CP Enumeration Error - registry corrupt. - %lx.\n",
                     rc));
            SetLastError(ERROR_BADDB);
            return (FALSE);
        }

        //
        //  Skip over the ACP, OEMCP, MACCP, and OEMHAL entries in the
        //  registry, and any entry that does not have data associated
        //  with it if the CP_INSTALLED flag is set.
        //
        pName = pKeyValueFull->Name;
        wch = *pName;
        NameLen = pKeyValueFull->NameLength / sizeof(WCHAR);
        if ( (NameLen <= ENUM_MAX_CP_SIZE) &&
             (wch >= NLS_CHAR_ZERO) && (wch <= NLS_CHAR_NINE) &&
             (!((fInstalled) && (pKeyValueFull->DataLength <= 2))) )
        {
            //
            //  Store the code page id string in the callback buffer.
            //
            pName[NameLen] = 0;

            //
            //  Call the appropriate callback function.
            //
            NLS_CALL_ENUMPROC_BREAK( gSystemLocale,
                                     lpCodePageEnumProc,
                                     dwFlags,
                                     pName,
                                     fUnicodeVer );
        }

        //
        //  Increment enumeration index value and get the next enumeration.
        //
        Index++;
        RtlZeroMemory(pKeyValueFull, MAX_KEY_VALUE_FULLINFO);
        rc = NtEnumerateValueKey( hCodePageKey,
                                  Index,
                                  KeyValueFullInformation,
                                  pKeyValueFull,
                                  MAX_KEY_VALUE_FULLINFO,
                                  &ResultLength );
    }

    //
    //  Include UTF-7 and UTF-8 code pages in the enumeration -
    //  both installed and supported.
    //
    NLS_CALL_ENUMPROC_TRUE( gSystemLocale,
                            lpCodePageEnumProc,
                            dwFlags,
                            L"65000",
                            0,
                            fUnicodeVer,
                            0 );
    NLS_CALL_ENUMPROC_TRUE( gSystemLocale,
                            lpCodePageEnumProc,
                            dwFlags,
                            L"65001",
                            0,
                            fUnicodeVer,
                            0 );

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Internal_EnumCalendarInfo
//
//  Enumerates the specified calendar information that is available for the
//  specified locale, based on the CalType parameter.  It does so by
//  passing the pointer to the string buffer containing the calendar info
//  to an application-defined callback function.  It continues until the
//  last calendar info is found or the callback function returns FALSE.
//
//  10-14-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL Internal_EnumCalendarInfo(
    NLS_ENUMPROC lpCalInfoEnumProc,
    LCID Locale,
    CALID Calendar,
    CALTYPE CalType,
    BOOL fUnicodeVer,
    BOOL fExVersion)
{
    PLOC_HASH pHashN;             // ptr to LOC hash node
    ULONG CalFieldOffset;         // field offset in calendar structure
    ULONG EndCalFieldOffset;      // field offset in calendar structure
    ULONG LocFieldOffset;         // field offset in locale structure
    ULONG EndLocFieldOffset;      // field offset in locale structure
    LPWSTR pOptCal;               // ptr to optional calendar values
    LPWSTR pEndOptCal;            // ptr to end of optional calendars
    PCAL_INFO pCalInfo;           // ptr to calendar info
    BOOL fIfName = FALSE;         // if caltype is a name
    UINT fEra = 0;                // if era caltype
    BOOL fLocaleInfo = TRUE;      // if locale information
    LPWSTR pString;               // ptr to enumeration string
    LPWSTR pEndString;            // ptr to end of enumeration string
    CALID CalNum;                 // calendar number
    DWORD UseCPACP;               // original caltype - if use system ACP
    WCHAR pTemp[MAX_REG_VAL_SIZE];// temp buffer to hold two-digit-year-max


    //
    //  Invalid Parameter Check:
    //    - validate LCID
    //    - function pointer is null
    //
    //    - CalType will be checked in switch statement below.
    //
    VALIDATE_LOCALE(Locale, pHashN, FALSE);
    if ((pHashN == NULL) || (lpCalInfoEnumProc == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    //
    //  Initialize the pointers to the optional calendar data.
    //
    if (Calendar == ENUM_ALL_CALENDARS)
    {
        pOptCal = (LPWORD)(pHashN->pLocaleHdr) + pHashN->pLocaleHdr->IOptionalCal;
        pEndOptCal = (LPWORD)(pHashN->pLocaleHdr) + pHashN->pLocaleHdr->SDayName1;
    }
    else
    {
        //
        //  Validate the Calendar parameter.
        //
        if ((pOptCal = IsValidCalendarType(pHashN, Calendar)) == NULL)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return (FALSE);
        }
        pEndOptCal = pOptCal + ((POPT_CAL)pOptCal)->Offset;
    }

    //
    //  Enumerate the information based on CalType.
    //
    UseCPACP = (DWORD)CalType;
    CalType = NLS_GET_CALTYPE_VALUE(CalType);
    switch (CalType)
    {
        case ( CAL_ICALINTVALUE ) :
        {
            //
            //  Get the integer value for each of the alternate
            //  calendars (as a string).
            //
            while (pOptCal < pEndOptCal)
            {
                if (((POPT_CAL)pOptCal)->CalId != CAL_NO_OPTIONAL)
                {
                    //
                    //  Call the appropriate callback function.
                    //
                    NLS_CALL_ENUMPROC_TRUE( Locale,
                                            lpCalInfoEnumProc,
                                            UseCPACP,
                                            ((POPT_CAL)pOptCal)->pCalStr,
                                            ((POPT_CAL)pOptCal)->CalId,
                                            fUnicodeVer,
                                            fExVersion );
                }

                //
                //  Advance ptr to next optional calendar.
                //
                pOptCal += ((POPT_CAL)pOptCal)->Offset;
            }

            return (TRUE);

            break;
        }
        case ( CAL_SCALNAME ) :
        {
            //
            //  Get the calendar name for each of the alternate
            //  calendars.
            //
            while (pOptCal < pEndOptCal)
            {
                if (((POPT_CAL)pOptCal)->CalId != CAL_NO_OPTIONAL)
                {
                    //
                    //  Call the appropriate callback function.
                    //
                    NLS_CALL_ENUMPROC_TRUE(
                            Locale,
                            lpCalInfoEnumProc,
                            UseCPACP,
                            ((POPT_CAL)pOptCal)->pCalStr +
                            NlsStrLenW(((POPT_CAL)pOptCal)->pCalStr) + 1,
                            ((POPT_CAL)pOptCal)->CalId,
                            fUnicodeVer,
                            fExVersion );
                }

                //
                //  Advance ptr to next optional calendar.
                //
                pOptCal += ((POPT_CAL)pOptCal)->Offset;
            }

            return (TRUE);

            break;
        }
        case ( CAL_ITWODIGITYEARMAX ) :
        {
            fLocaleInfo = FALSE;
            CalFieldOffset    = FIELD_OFFSET(CALENDAR_VAR, STwoDigitYearMax);
            EndCalFieldOffset = FIELD_OFFSET(CALENDAR_VAR, SEraRanges);

            if (!(UseCPACP & CAL_NOUSEROVERRIDE))
            {
                while (pOptCal < pEndOptCal)
                {
                    CalNum = ((POPT_CAL)pOptCal)->CalId;

                    if (CalNum != CAL_NO_OPTIONAL)
                    {
                        //
                        // Look into the registry first
                        //
                        if (GetTwoDigitYearInfo(CalNum, pTemp, NLS_POLICY_TWO_DIGIT_YEAR_KEY) ||
                            GetTwoDigitYearInfo(CalNum, pTemp, NLS_TWO_DIGIT_YEAR_KEY))
                        {
                            NLS_CALL_ENUMPROC_TRUE(
                                    Locale,
                                    lpCalInfoEnumProc,
                                    UseCPACP,
                                    pTemp,
                                    CalNum,
                                    fUnicodeVer,
                                    fExVersion );
                        }
                        else
                        {
                            //
                            // Try to find the system default if we couldn't find the
                            // user setting in the registry or the user has asked for
                            // system default.
                            //
                            if (GetCalendar(CalNum, &pCalInfo) == NO_ERROR)
                            {
                                pString = (LPWORD)pCalInfo +
                                          *((LPWORD)((LPBYTE)(pCalInfo) + CalFieldOffset));
                                pEndString = (LPWORD)pCalInfo +
                                             *((LPWORD)((LPBYTE)(pCalInfo) + EndCalFieldOffset));

                                if (*pString)
                                {
                                   while (pString < pEndString)
                                   {
                                        //
                                        //  Make sure the string is NOT empty.
                                        //
                                        if (*pString)
                                        {
                                            //
                                            //  Call the appropriate callback function.
                                            //
                                            NLS_CALL_ENUMPROC_TRUE(
                                                    Locale,
                                                    lpCalInfoEnumProc,
                                                    UseCPACP,
                                                    pString,
                                                    CalNum,
                                                    fUnicodeVer,
                                                    fExVersion );
                                        }

                                        //
                                        //  Advance pointer to next string.
                                        //
                                        pString += NlsStrLenW(pString) + 1;
                                    }
                                }
                            }
                        }
                    }

                    //
                    //  Advance ptr to next optional calendar.
                    //
                    pOptCal += ((POPT_CAL)pOptCal)->Offset;
                }

                return (TRUE);
            }

            break;
        }
        case ( CAL_IYEAROFFSETRANGE ) :
        case ( CAL_SERASTRING ) :
        {
            fEra = CalType;
            CalFieldOffset    = FIELD_OFFSET(CALENDAR_VAR, SEraRanges);
            EndCalFieldOffset = FIELD_OFFSET(CALENDAR_VAR, SShortDate);

            break;
        }
        case ( CAL_SSHORTDATE ) :
        {
            CalFieldOffset    = FIELD_OFFSET(CALENDAR_VAR, SShortDate);
            EndCalFieldOffset = FIELD_OFFSET(CALENDAR_VAR, SYearMonth);
            LocFieldOffset    = FIELD_OFFSET(LOCALE_VAR, SShortDate);
            EndLocFieldOffset = FIELD_OFFSET(LOCALE_VAR, SDate);

            break;
        }
        case ( CAL_SLONGDATE ) :
        {
            CalFieldOffset    = FIELD_OFFSET(CALENDAR_VAR, SLongDate);
            EndCalFieldOffset = FIELD_OFFSET(CALENDAR_VAR, SDayName1);
            LocFieldOffset    = FIELD_OFFSET(LOCALE_VAR, SLongDate);
            EndLocFieldOffset = FIELD_OFFSET(LOCALE_VAR, IOptionalCal);

            break;
        }
        case ( CAL_SYEARMONTH ) :
        {
            CalFieldOffset    = FIELD_OFFSET(CALENDAR_VAR, SYearMonth);
            EndCalFieldOffset = FIELD_OFFSET(CALENDAR_VAR, SLongDate);
            LocFieldOffset    = FIELD_OFFSET(LOCALE_VAR, SYearMonth);
            EndLocFieldOffset = FIELD_OFFSET(LOCALE_VAR, SLongDate);

            break;
        }
        case ( CAL_SDAYNAME1 ) :
        case ( CAL_SDAYNAME2 ) :
        case ( CAL_SDAYNAME3 ) :
        case ( CAL_SDAYNAME4 ) :
        case ( CAL_SDAYNAME5 ) :
        case ( CAL_SDAYNAME6 ) :
        case ( CAL_SDAYNAME7 ) :
        case ( CAL_SABBREVDAYNAME1 ) :
        case ( CAL_SABBREVDAYNAME2 ) :
        case ( CAL_SABBREVDAYNAME3 ) :
        case ( CAL_SABBREVDAYNAME4 ) :
        case ( CAL_SABBREVDAYNAME5 ) :
        case ( CAL_SABBREVDAYNAME6 ) :
        case ( CAL_SABBREVDAYNAME7 ) :
        case ( CAL_SMONTHNAME1 ) :
        case ( CAL_SMONTHNAME2 ) :
        case ( CAL_SMONTHNAME3 ) :
        case ( CAL_SMONTHNAME4 ) :
        case ( CAL_SMONTHNAME5 ) :
        case ( CAL_SMONTHNAME6 ) :
        case ( CAL_SMONTHNAME7 ) :
        case ( CAL_SMONTHNAME8 ) :
        case ( CAL_SMONTHNAME9 ) :
        case ( CAL_SMONTHNAME10 ) :
        case ( CAL_SMONTHNAME11 ) :
        case ( CAL_SMONTHNAME12 ) :
        case ( CAL_SMONTHNAME13 ) :
        case ( CAL_SABBREVMONTHNAME1 ) :
        case ( CAL_SABBREVMONTHNAME2 ) :
        case ( CAL_SABBREVMONTHNAME3 ) :
        case ( CAL_SABBREVMONTHNAME4 ) :
        case ( CAL_SABBREVMONTHNAME5 ) :
        case ( CAL_SABBREVMONTHNAME6 ) :
        case ( CAL_SABBREVMONTHNAME7 ) :
        case ( CAL_SABBREVMONTHNAME8 ) :
        case ( CAL_SABBREVMONTHNAME9 ) :
        case ( CAL_SABBREVMONTHNAME10 ) :
        case ( CAL_SABBREVMONTHNAME11 ) :
        case ( CAL_SABBREVMONTHNAME12 ) :
        case ( CAL_SABBREVMONTHNAME13 ) :
        {
            fIfName = TRUE;
            CalFieldOffset    = FIELD_OFFSET(CALENDAR_VAR, SDayName1) +
                                ((CalType - CAL_SDAYNAME1) * sizeof(WORD));
            EndCalFieldOffset = FIELD_OFFSET(CALENDAR_VAR, SDayName1) +
                                ((CalType - CAL_SDAYNAME1 + 1) * sizeof(WORD));
            LocFieldOffset    = FIELD_OFFSET(LOCALE_VAR, SDayName1) +
                                ((CalType - CAL_SDAYNAME1) * sizeof(WORD));
            EndLocFieldOffset = FIELD_OFFSET(LOCALE_VAR, SDayName1) +
                                ((CalType - CAL_SDAYNAME1 + 1) * sizeof(WORD));

            break;
        }
        default :
        {
            SetLastError(ERROR_INVALID_FLAGS);
            return (FALSE);
        }
    }

    //
    //  Get the requested information for each of the alternate calendars.
    //
    //  This loop is used for the following CalTypes:
    //
    //     iYearOffsetRange         (fEra = TRUE)
    //     sEraString               (fEra = TRUE)
    //
    //     sShortDate
    //     sLongDate
    //     sYearMonth
    //
    //     sDayName1-7              (fIfName = TRUE)
    //     sAbbrevDayName1-7        (fIfName = TRUE)
    //     sMonthName1-7            (fIfName = TRUE)
    //     sAbbrevMonthName1-7      (fIfName = TRUE)
    //
    while (pOptCal < pEndOptCal)
    {
        //
        //  Get the pointer to the appropriate calendar.
        //
        CalNum = ((POPT_CAL)pOptCal)->CalId;
        if (GetCalendar(CalNum, &pCalInfo) == NO_ERROR)
        {
            //
            //  Check era information flag.
            //
            if (fEra)
            {
                //
                //  Get the pointer to the appropriate calendar string.
                //
                pString = (LPWORD)pCalInfo +
                          *((LPWORD)((LPBYTE)(pCalInfo) + CalFieldOffset));

                pEndString = (LPWORD)pCalInfo +
                             *((LPWORD)((LPBYTE)(pCalInfo) + EndCalFieldOffset));

                //
                //  Make sure the string is NOT empty.
                //
                if (*pString)
                {
                    //
                    //  See which era information to get.
                    //
                    if (fEra == CAL_IYEAROFFSETRANGE)
                    {
                        while (pString < pEndString)
                        {
                            //
                            //  Call the appropriate callback function.
                            //
                            NLS_CALL_ENUMPROC_TRUE(
                                    Locale,
                                    lpCalInfoEnumProc,
                                    UseCPACP,
                                    ((PERA_RANGE)pString)->pYearStr,
                                    CalNum,
                                    fUnicodeVer,
                                    fExVersion );

                            //
                            //  Advance pointer to next era range.
                            //
                            pString += ((PERA_RANGE)pString)->Offset;
                        }
                    }
                    else
                    {
                        while (pString < pEndString)
                        {
                            //
                            //  Call the appropriate callback function.
                            //
                            NLS_CALL_ENUMPROC_TRUE(
                                    Locale,
                                    lpCalInfoEnumProc,
                                    UseCPACP,
                                    ((PERA_RANGE)pString)->pYearStr +
                                    NlsStrLenW(((PERA_RANGE)pString)->pYearStr) + 1,
                                    CalNum,
                                    fUnicodeVer,
                                    fExVersion );

                            //
                            //  Advance pointer to next era range.
                            //
                            pString += ((PERA_RANGE)pString)->Offset;
                        }
                    }
                }
            }
            else
            {
                //
                //  Get the pointer to the appropriate calendar string.
                //
                if ((!fIfName) ||
                    (((PCALENDAR_VAR)pCalInfo)->IfNames))
                {
                    pString = (LPWORD)pCalInfo +
                              *((LPWORD)((LPBYTE)(pCalInfo) + CalFieldOffset));

                    pEndString = (LPWORD)pCalInfo +
                                 *((LPWORD)((LPBYTE)(pCalInfo) + EndCalFieldOffset));
                }
                else
                {
                    pString = L"";
                }

                //
                //  Make sure we have a string.  Otherwise, use the
                //  information from the locale section (if appropriate).
                //
                if ((*pString == 0) && (fLocaleInfo) &&
                    ((CalNum == CAL_GREGORIAN) ||
                     (Calendar != ENUM_ALL_CALENDARS)))
                {
                    //
                    //  Use the default locale string.
                    //
                    pString = (LPWORD)(pHashN->pLocaleHdr) +
                              *((LPWORD)((LPBYTE)(pHashN->pLocaleHdr) +
                                         LocFieldOffset));

                    pEndString = (LPWORD)(pHashN->pLocaleHdr) +
                                 *((LPWORD)((LPBYTE)(pHashN->pLocaleHdr) +
                                            EndLocFieldOffset));
                }

                //
                //  Go through each of the strings.
                //
                if (*pString)
                {
                    while (pString < pEndString)
                    {
                        //
                        //  Make sure the string is NOT empty.
                        //
                        if (*pString)
                        {
                            //
                            //  Call the appropriate callback function.
                            //
                            NLS_CALL_ENUMPROC_TRUE( Locale,
                                                    lpCalInfoEnumProc,
                                                    UseCPACP,
                                                    pString,
                                                    CalNum,
                                                    fUnicodeVer,
                                                    fExVersion );
                        }

                        //
                        //  Advance pointer to next string.
                        //
                        pString += NlsStrLenW(pString) + 1;
                    }
                }
            }
        }

        //
        //  Advance ptr to next optional calendar.
        //
        pOptCal += ((POPT_CAL)pOptCal)->Offset;
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Internal_EnumTimeFormats
//
//  Enumerates the time formats that are available for the
//  specified locale, based on the dwFlags parameter.  It does so by
//  passing the pointer to the string buffer containing the time format
//  to an application-defined callback function.  It continues until the
//  last time format is found or the callback function returns FALSE.
//
//  10-14-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL Internal_EnumTimeFormats(
    NLS_ENUMPROC lpTimeFmtEnumProc,
    LCID Locale,
    DWORD dwFlags,
    BOOL fUnicodeVer)
{
    PLOC_HASH pHashN;             // ptr to LOC hash node


    //
    //  Invalid Parameter Check:
    //    - validate LCID
    //    - function pointer is null
    //
    VALIDATE_LOCALE(Locale, pHashN, FALSE);
    if ((pHashN == NULL) || (lpTimeFmtEnumProc == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    //
    //  Invalid Flags Check:
    //    - flags other than valid ones
    //
    if (dwFlags & ETF_INVALID_FLAG)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return (FALSE);
    }

    //
    //  Enumerate the time formats.
    //
    return ( EnumDateTime( lpTimeFmtEnumProc,
                           Locale,
                           LOCALE_STIMEFORMAT,
                           dwFlags,
                           pNlsUserInfo->sTimeFormat,
                           NLS_VALUE_STIMEFORMAT,
                           pHashN->pLocaleHdr,
                           (LPWORD)(pHashN->pLocaleHdr) +
                             pHashN->pLocaleHdr->STimeFormat,
                           (LPWORD)(pHashN->pLocaleHdr) +
                             pHashN->pLocaleHdr->STime,
                           (ULONG)0,
                           (ULONG)0,
                           FALSE,
                           fUnicodeVer,
                           FALSE ) );
}


////////////////////////////////////////////////////////////////////////////
//
//  Internal_EnumDateFormats
//
//  Enumerates the short date, long date, or year/month formats that are
//  available for the specified locale, based on the dwFlags parameter.
//  It does so by passing the pointer to the string buffer containing the
//  date format (and the calendar id if called from the Ex version) to an
//  application-defined callback function.  It continues until the last
//  date format is found or the callback function returns FALSE.
//
//  10-14-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL Internal_EnumDateFormats(
    NLS_ENUMPROC lpDateFmtEnumProc,
    LCID Locale,
    DWORD dwFlags,
    BOOL fUnicodeVer,
    BOOL fExVersion)
{
    PLOC_HASH pHashN;             // ptr to LOC hash node


    //
    //  Invalid Parameter Check:
    //    - validate LCID
    //    - function pointer is null
    //
    //    - flags will be validated in switch statement below
    //
    VALIDATE_LOCALE(Locale, pHashN, FALSE);
    if ((pHashN == NULL) || (lpDateFmtEnumProc == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    //
    //  Enumerate the date pictures based on the flags.
    //
    switch (dwFlags & (~LOCALE_USE_CP_ACP))
    {
        case ( 0 ) :
        case ( DATE_SHORTDATE ) :
        {
            //
            //  Enumerate the short date formats.
            //
            return ( EnumDateTime( lpDateFmtEnumProc,
                                   Locale,
                                   LOCALE_SSHORTDATE,
                                   dwFlags,
                                   pNlsUserInfo->sShortDate,
                                   NLS_VALUE_SSHORTDATE,
                                   pHashN->pLocaleHdr,
                                   (LPWORD)(pHashN->pLocaleHdr) +
                                     pHashN->pLocaleHdr->SShortDate,
                                   (LPWORD)(pHashN->pLocaleHdr) +
                                     pHashN->pLocaleHdr->SDate,
                                   (ULONG)FIELD_OFFSET(CALENDAR_VAR, SShortDate),
                                   (ULONG)FIELD_OFFSET(CALENDAR_VAR, SYearMonth),
                                   TRUE,
                                   fUnicodeVer,
                                   fExVersion ) );

            break;
        }

        case ( DATE_LONGDATE ) :
        {
            //
            //  Enumerate the long date formats.
            //
            return ( EnumDateTime( lpDateFmtEnumProc,
                                   Locale,
                                   LOCALE_SLONGDATE,
                                   dwFlags,
                                   pNlsUserInfo->sLongDate,
                                   NLS_VALUE_SLONGDATE,
                                   pHashN->pLocaleHdr,
                                   (LPWORD)(pHashN->pLocaleHdr) +
                                     pHashN->pLocaleHdr->SLongDate,
                                   (LPWORD)(pHashN->pLocaleHdr) +
                                     pHashN->pLocaleHdr->IOptionalCal,
                                   (ULONG)FIELD_OFFSET(CALENDAR_VAR, SLongDate),
                                   (ULONG)FIELD_OFFSET(CALENDAR_VAR, SDayName1),
                                   TRUE,
                                   fUnicodeVer,
                                   fExVersion ) );

            break;
        }

        case ( DATE_YEARMONTH ) :
        {
            //
            //  Enumerate the year month formats.
            //
            return ( EnumDateTime( lpDateFmtEnumProc,
                                   Locale,
                                   LOCALE_SYEARMONTH,
                                   dwFlags,
                                   pNlsUserInfo->sYearMonth,
                                   NLS_VALUE_SYEARMONTH,
                                   pHashN->pLocaleHdr,
                                   (LPWORD)(pHashN->pLocaleHdr) +
                                     pHashN->pLocaleHdr->SYearMonth,
                                   (LPWORD)(pHashN->pLocaleHdr) +
                                     pHashN->pLocaleHdr->SLongDate,
                                   (ULONG)FIELD_OFFSET(CALENDAR_VAR, SYearMonth),
                                   (ULONG)FIELD_OFFSET(CALENDAR_VAR, SLongDate),
                                   TRUE,
                                   fUnicodeVer,
                                   fExVersion ) );

            break;
        }

        default :
        {
            SetLastError(ERROR_INVALID_FLAGS);
            return (FALSE);
        }
    }
}




//-------------------------------------------------------------------------//
//                           INTERNAL ROUTINES                             //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  GetLocalizedLanguageGroupName
//
//  Returns the localized version of the language group name for the given
//  language group id.  It gets the information from the resource file in
//  the language that the current user is using.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetLocalizedLanguageGroupName(
    LGRPID LanguageGroup,
    LPWSTR pLangGroupName,
    int cchName)
{
    LPWSTR pString;
    int Length;

    //
    //  Make sure we have a language group id that is not equal to 0x0000
    //  and is less than 0x0400.
    //
    if ((LanguageGroup == 0) || (LanguageGroup >= LANG_USER_DEFAULT))
    {
        //
        //  We have an invalid language group id, so return 0.
        //
        return (0);
    }

    //
    //  Get the language name from the string table.
    //
    Length = GetNameFromStringTable((WORD)LanguageGroup, &pString);

    //
    //  If the length is too big for the buffer, then reset the length
    //  to the size of the given buffer.
    //
    if (Length >= cchName)
    {
        Length = cchName - 1;
    }

    //
    //  Copy the string to the buffer and zero terminate it.
    //
    wcsncpy(pLangGroupName, pString, Length);
    pLangGroupName[Length] = 0;

    //
    //  Return the number of characters.
    //
    return (Length);
}


////////////////////////////////////////////////////////////////////////////
//
//  EnumDateTime
//
//  Enumerates the short date, long date, year/month, or time formats that
//  are available for the specified locale.  This is the worker routine for
//  the EnumTimeFormats and EnumDateFormats apis.
//
//  10-14-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL EnumDateTime(
    NLS_ENUMPROC lpDateTimeFmtEnumProc,
    LCID Locale,
    LCTYPE LCType,
    DWORD dwFlags,
    LPWSTR pCacheValue,
    LPWSTR pRegValue,
    PLOCALE_VAR pLocaleHdr,
    LPWSTR pDateTime,
    LPWSTR pEndDateTime,
    ULONG CalDateOffset,
    ULONG EndCalDateOffset,
    BOOL fCalendarInfo,
    BOOL fUnicodeVer,
    BOOL fExVersion)
{
    LPWSTR pUser = NULL;               // ptr to user date/time string
    LPWSTR pOptCal;                    // ptr to optional calendar values
    LPWSTR pEndOptCal;                 // ptr to end of optional calendars
    PCAL_INFO pCalInfo;                // ptr to calendar info
    CALID CalNum = 1;                  // calendar number
    WCHAR pTemp[MAX_REG_VAL_SIZE];     // temp buffer
    UNICODE_STRING ObUnicodeStr;       // calendar id string


    //
    //  Get the user's Calendar ID.
    //
    if (fExVersion)
    {
        if (GetUserInfo( Locale,
                         LOCALE_ICALENDARTYPE,
                         pNlsUserInfo->iCalType,
                         NLS_VALUE_ICALENDARTYPE,
                         pTemp,
                         TRUE ))
        {
            RtlInitUnicodeString(&ObUnicodeStr, pTemp);
            if ((RtlUnicodeStringToInteger(&ObUnicodeStr, 10, &CalNum)) ||
                (CalNum < 1) || (CalNum > CAL_LAST))
            {
                CalNum = 1;
            }
        }
    }

    //
    //  Get the user defined string.
    //
    if (GetUserInfo( Locale,
                     LCType,
                     pCacheValue,
                     pRegValue,
                     pTemp,
                     TRUE ))
    {
        pUser = pTemp;

        //
        //  Call the appropriate callback function.
        //
        NLS_CALL_ENUMPROC_TRUE( Locale,
                                lpDateTimeFmtEnumProc,
                                dwFlags,
                                pUser,
                                CalNum,
                                fUnicodeVer,
                                fExVersion );
    }

    //
    //  Get the default strings defined for the Gregorian
    //  calendar.
    //
    while (pDateTime < pEndDateTime)
    {
        //
        //  Call the callback function if the string is not
        //  the same as the user string.
        //
        if ((!pUser) || (!NlsStrEqualW(pUser, pDateTime)))
        {
            //
            //  Call the appropriate callback function.
            //
            NLS_CALL_ENUMPROC_TRUE( Locale,
                                    lpDateTimeFmtEnumProc,
                                    dwFlags,
                                    pDateTime,
                                    CAL_GREGORIAN,
                                    fUnicodeVer,
                                    fExVersion );
        }

        //
        //  Advance pDateTime pointer.
        //
        pDateTime += NlsStrLenW(pDateTime) + 1;
    }

    if (fCalendarInfo)
    {
        //
        //  Get any alternate calendar dates.
        //
        pOptCal = (LPWORD)(pLocaleHdr) + pLocaleHdr->IOptionalCal;
        if (((POPT_CAL)pOptCal)->CalId == CAL_NO_OPTIONAL)
        {
            //
            //  No optional calendars, so done.
            //
            return (TRUE);
        }

        //
        //  Get the requested information for each of the alternate
        //  calendars.
        //
        pEndOptCal = (LPWORD)(pLocaleHdr) + pLocaleHdr->SDayName1;
        while (pOptCal < pEndOptCal)
        {
            //
            //  Get the pointer to the calendar information.
            //
            CalNum = ((POPT_CAL)pOptCal)->CalId;
            if (GetCalendar(CalNum, &pCalInfo) == NO_ERROR)
            {
                //
                //  Get the pointer to the date/time information for the
                //  current calendar.
                //
                pDateTime = (LPWORD)pCalInfo +
                            *((LPWORD)((LPBYTE)(pCalInfo) + CalDateOffset));

                pEndDateTime = (LPWORD)pCalInfo +
                               *((LPWORD)((LPBYTE)(pCalInfo) + EndCalDateOffset));

                //
                //  Go through each of the strings.
                //
                while (pDateTime < pEndDateTime)
                {
                    //
                    //  Make sure the string is NOT empty and that it is
                    //  NOT the same as the user's string.
                    //
                    if ((*pDateTime) &&
                        ((!pUser) || (!NlsStrEqualW(pUser, pDateTime))))
                    {
                        //
                        //  Call the appropriate callback function.
                        //
                        NLS_CALL_ENUMPROC_TRUE( Locale,
                                                lpDateTimeFmtEnumProc,
                                                dwFlags,
                                                pDateTime,
                                                CalNum,
                                                fUnicodeVer,
                                                fExVersion );
                    }

                    //
                    //  Advance pointer to next date string.
                    //
                    pDateTime += NlsStrLenW(pDateTime) + 1;
                }
            }

            //
            //  Advance ptr to next optional calendar.
            //
            pOptCal += ((POPT_CAL)pOptCal)->Offset;
        }
    }

    //
    //  Return success.
    //
    return (TRUE);
}
