/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    locale.c

Abstract:

    This file contains functions that return information about a
    language group, a UI language, a locale, or a calendar.

    APIs found in this file:
      IsValidLanguageGroup
      IsValidLocale
      ConvertDefaultLocale
      GetThreadLocale
      SetThreadLocale
      GetSystemDefaultUILanguage
      GetUserDefaultUILanguage
      GetSystemDefaultLangID
      GetUserDefaultLangID
      GetSystemDefaultLCID
      GetUserDefaultLCID
      VerLanguageNameW
      VerLanguageNameA
      GetLocaleInfoW
      SetLocaleInfoW
      GetCalendarInfoW
      SetCalendarInfoW

Revision History:

    05-31-91    JulieB    Created.

--*/



//
//  Include Files.
//

#include "nls.h"

//
// Global Vars
//
LCID gProcessLocale;


//
//  Allow this file to build without warnings when the DUnicode switch
//  is turned off.
//
#undef MAKEINTRESOURCE
#define MAKEINTRESOURCE MAKEINTRESOURCEW




//
//  Forward Declarations.
//

int
GetLocalizedLanguageName(
    LANGID Language,
    LPWSTR *ppLangName);

BOOL
GetLocalizedCountryName(
    LANGID Language,
    LPWSTR *ppCtryName);

BOOL
GetLocalizedSortName(
    LCID Locale,
    LPWSTR *ppSortName);

BOOL
SetUserInfo(
    LCTYPE LCType,
    LPWSTR pData,
    ULONG DataLength);

BOOL SetCurrentUserRegValue(
    LCTYPE   LCType,
    LPWSTR pData,
    ULONG DataLength);

BOOL
SetMultipleUserInfo(
    DWORD dwFlags,
    int cchData,
    LPCWSTR pPicture,
    LPCWSTR pSeparator,
    LPCWSTR pOrder,
    LPCWSTR pTLZero,
    LPCWSTR pTimeMarkPosn);


BOOL
SetTwoDigitYearInfo(
    CALID Calendar,
    LPCWSTR pYearInfo,
    int cchData);

void
GetInstallLanguageFromRegistry();




//-------------------------------------------------------------------------//
//                          PRIVATE API ROUTINES                           //
//-------------------------------------------------------------------------//
void NlsResetProcessLocale(void)
{

    //
    // If the thread isn't impersonating, then re-read the
    // process locale from the current user's registry
    //
    if (NtCurrentTeb()->IsImpersonating == 0L)
    {
        NlsFlushProcessCache( LOCALE_SLOCALE );
        NlsGetUserLocale( &gProcessLocale );
    }

    return;
}



//-------------------------------------------------------------------------//
//                             API ROUTINES                                //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  IsValidLanguageGroup
//
//  Determines whether or not a language group is installed in the system
//  if the LGRPID_INSTALLED flag is set, or whether or not a language group
//  is supported in the system if the LGRPID_SUPPORTED flag is set.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI IsValidLanguageGroup(
    LGRPID LanguageGroup,
    DWORD dwFlags)
{
    PKEY_VALUE_FULL_INFORMATION pKeyValueFull;
    BYTE pStatic[MAX_KEY_VALUE_FULLINFO];

    WCHAR pTmpBuf[MAX_PATH];           // temp buffer
    UNICODE_STRING ObUnicodeStr;       // registry data value string
    LPWSTR pData;                      // ptr to registry data


    //
    //  Invalid Flags Check:
    //     - flags other than valid ones
    //     - more than one of either supported or installed
    //
    if ((dwFlags & IVLG_INVALID_FLAG) ||
        (MORE_THAN_ONE(dwFlags, IVLG_SINGLE_FLAG)))
    {
        return (FALSE);
    }

    //
    //  Open the Language Groups registry key.
    //
    OPEN_LANG_GROUPS_KEY(FALSE);

    //
    //  Convert language group value to Unicode string.
    //
    if (NlsConvertIntegerToString(LanguageGroup, 16, 1, pTmpBuf, MAX_PATH))
    {
        return (FALSE);
    }

    //
    //  Query the registry for the value.
    //
    pKeyValueFull = (PKEY_VALUE_FULL_INFORMATION)pStatic;
    if ((QueryRegValue( hLangGroupsKey,
                        pTmpBuf,
                        &pKeyValueFull,
                        MAX_KEY_VALUE_FULLINFO,
                        NULL ) != NO_ERROR))
    {
        return (FALSE);
    }

    //
    //  Language Group is SUPPORTED.  If the INSTALLED flag is NOT set, then
    //  return success.
    //
    if (!(dwFlags & LGRPID_INSTALLED))
    {
        return (TRUE);
    }

    //
    //  Need to find out if it's installed.
    //
    if (pKeyValueFull->DataLength > 2)
    {
        pData = GET_VALUE_DATA_PTR(pKeyValueFull);
        if ((pData[0] == L'1') && (pData[1] == 0))
        {
            return (TRUE);
        }
    }

    //
    //  Return result.
    //
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  IsValidLocale
//
//  Determines whether or not a locale is installed in the system if the
//  LCID_INSTALLED flag is set, or whether or not a locale is supported in
//  the system if the LCID_SUPPORTED flag is set.
//
//  07-26-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI IsValidLocale(
    LCID Locale,
    DWORD dwFlags)
{
    PKEY_VALUE_FULL_INFORMATION pKeyValueFull;
    BYTE pStatic1[MAX_KEY_VALUE_FULLINFO];
    BYTE pStatic2[MAX_KEY_VALUE_FULLINFO];

    WCHAR pTmpBuf[MAX_PATH];           // temp buffer
    UNICODE_STRING ObUnicodeStr;       // registry data value string
    DWORD Data;                        // registry data value
    LPWSTR pData;                      // ptr to registry data
    BOOL bResult = FALSE;              // result value


    //
    //  Invalid Flags Check:
    //     - flags other than valid ones
    //     - more than one of either supported or installed
    //
    if ((dwFlags & IVL_INVALID_FLAG) ||
        (MORE_THAN_ONE(dwFlags, IVL_SINGLE_FLAG)))
    {
        //
        //  The ME release of NT 4 did a really bad thing and allowed 0x39
        //  to be passed in as a valid flag value for Arabic and Hebrew.
        //  As a result, we need to allow this flag combination for
        //  the Arabic and Hebrew locales.
        //
        if ((dwFlags == 0x39) &&
            ((Locale == MAKELCID(MAKELANGID(LANG_ARABIC, SUBLANG_DEFAULT), SORT_DEFAULT)) ||
             (Locale == MAKELCID(MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT), SORT_DEFAULT))))
        {
            dwFlags = LCID_INSTALLED;
        }
        else
        {
            return (FALSE);
        }
    }

    //
    //  Invalid Locale Check.
    //
    if (IS_INVALID_LOCALE(Locale))
    {
        return (FALSE);
    }

    //
    //  See if the LOCALE information is in the system for the
    //  given locale.
    //
    if (GetLocHashNode(Locale) == NULL)
    {
        //
        //  Return failure.
        //
        return (FALSE);
    }

    //
    //  Locale is SUPPORTED.  If the INSTALLED flag is NOT set, then
    //  return success.
    //
    if (!(dwFlags & LCID_INSTALLED))
    {
        return (TRUE);
    }

    //
    //  Open the Locale, the Alternate Sorts, and the Language Groups
    //  registry keys.
    //
    OPEN_LOCALE_KEY(FALSE);
    OPEN_ALT_SORTS_KEY(FALSE);
    OPEN_LANG_GROUPS_KEY(FALSE);

    //
    //  Convert locale value to Unicode string.
    //
    if (NlsConvertIntegerToString(Locale, 16, 8, pTmpBuf, MAX_PATH))
    {
        return (FALSE);
    }

    //
    //  Query the registry for the value.
    //
    pKeyValueFull = (PKEY_VALUE_FULL_INFORMATION)pStatic1;
    if (((QueryRegValue( hLocaleKey,
                         pTmpBuf,
                         &pKeyValueFull,
                         MAX_KEY_VALUE_FULLINFO,
                         NULL ) == NO_ERROR) ||
         (QueryRegValue( hAltSortsKey,
                         pTmpBuf,
                         &pKeyValueFull,
                         MAX_KEY_VALUE_FULLINFO,
                         NULL ) == NO_ERROR)) &&
        (pKeyValueFull->DataLength > 2))
    {
        RtlInitUnicodeString(&ObUnicodeStr, GET_VALUE_DATA_PTR(pKeyValueFull));
        if ((RtlUnicodeStringToInteger(&ObUnicodeStr, 16, &Data) == NO_ERROR) &&
            (Data != 0))
        {
            pKeyValueFull = (PKEY_VALUE_FULL_INFORMATION)pStatic2;
            if ((QueryRegValue( hLangGroupsKey,
                                ObUnicodeStr.Buffer,
                                &pKeyValueFull,
                                MAX_KEY_VALUE_FULLINFO,
                                NULL ) == NO_ERROR) &&
                (pKeyValueFull->DataLength > 2))
            {
                pData = GET_VALUE_DATA_PTR(pKeyValueFull);
                if ((pData[0] == L'1') && (pData[1] == 0))
                {
                    bResult = TRUE;
                }
            }
        }
    }

    //
    //  Return result.
    //
    return (bResult);
}


////////////////////////////////////////////////////////////////////////////
//
//  ConvertDefaultLocale
//
//  Converts any of the special case locale values to an actual locale id.
//  If none of the special case locales was given, the given locale id
//  is returned.
//
//  09-01-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

LCID WINAPI ConvertDefaultLocale(
    LCID Locale)
{
    //
    //  Check for the special locale values.
    //
    CHECK_SPECIAL_LOCALES(Locale, FALSE);

    //
    //  Return the locale id.
    //
    return (Locale);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetThreadLocale
//
//  Returns the locale id for the current thread.
//
//  03-11-93    JulieB    Moved from base\client.
////////////////////////////////////////////////////////////////////////////

LCID WINAPI GetThreadLocale()
{
    //
    //  Return the locale id stored in the TEB.
    //
    return ((LCID)(NtCurrentTeb()->CurrentLocale));
}


////////////////////////////////////////////////////////////////////////////
//
//  SetThreadLocale
//
//  Resets the locale id for the current thread.  Any locale-dependent
//  functions will reflect the new locale.  If the locale passed in is
//  not a valid locale id, then FALSE is returned.
//
//  03-11-93    JulieB    Moved from base\client; Added Locale Validation.
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI SetThreadLocale(
    LCID Locale)
{
    PLOC_HASH pHashN;             // ptr to hash node


    //
    //  Validate locale id.
    //
    VALIDATE_LANGUAGE(Locale, pHashN, 0, FALSE);
    if (pHashN == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    //
    //  Set the locale id in the TEB.
    //
    NtCurrentTeb()->CurrentLocale = (ULONG)Locale;

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetSystemDefaultUILanguage
//
//  Returns the language of the original install.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

LANGID WINAPI GetSystemDefaultUILanguage()
{
    //
    //  Get the original install language and return it.
    //
    if (gSystemInstallLang == 0)
    {
        if (NtQueryInstallUILanguage(&gSystemInstallLang) != STATUS_SUCCESS)
        {
            gSystemInstallLang = 0;
            return (NLS_DEFAULT_UILANG);
        }
    }

    return (gSystemInstallLang);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetUserDefaultUILanguage
//
//  Returns the current User's UI language selection.  If the UI language
//  is not available, then the chosen default UI language is used
//  (NLS_DEFAULT_UILANG).
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

LANGID WINAPI GetUserDefaultUILanguage()
{
    LANGID DefaultUILang;

    if (NtQueryDefaultUILanguage(&DefaultUILang) != STATUS_SUCCESS)
    {
        if (GetSystemDefaultUILanguage() == 0)
        {
            return (NLS_DEFAULT_UILANG);
        }
    }

    return (DefaultUILang);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetSystemDefaultLangID
//
//  Returns the default language for the system.  If the registry value is
//  not readable, then the chosen default language is used
//  (NLS_DEFAULT_LANGID).
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

LANGID WINAPI GetSystemDefaultLangID()
{
    //
    //  Get the language id from the locale id stored in the cache
    //  and return it.
    //
    return (LANGIDFROMLCID(gSystemLocale));
}


////////////////////////////////////////////////////////////////////////////
//
//  GetUserDefaultLangID
//
//  Returns the default language for the current user.  If the current user's
//  language is not set, then the system default language id is returned.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

LANGID WINAPI GetUserDefaultLangID()
{
    //
    //  Get the language id from the locale id stored in the cache
    //  and return it.
    //
    return (LANGIDFROMLCID(GetUserDefaultLCID()));
}


////////////////////////////////////////////////////////////////////////////
//
//  GetSystemDefaultLCID
//
//  Returns the default locale for the system.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

LCID WINAPI GetSystemDefaultLCID()
{
    //
    //  Return the locale id stored in the cache.
    //
    return (gSystemLocale);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetUserDefaultLCID
//
//  Returns the default locale for the current user.  If current user's locale
//  is not set, then the system default locale id is returned.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

LCID WINAPI GetUserDefaultLCID()
{
    LCID Lcid = NtCurrentTeb()->ImpersonationLocale;

    switch (Lcid)
    {
        case ( -1 ) :
        {
            //
            //  Thread is being impersonated.
            //
            if (NT_SUCCESS( NlsGetUserLocale(&Lcid) ))
            {
                NtCurrentTeb()->ImpersonationLocale = Lcid;
            }
            else
            {
                //
                // If we can't get it from the registry, then let's use the
                // system locale since it won't be resolved by calling
                // GetUserDefaultLCID() again.
                //
                Lcid = NtCurrentTeb()->ImpersonationLocale = gSystemLocale;
            }
            break;
        }
        case ( 0 ) :
        {
            //
            //  Thread hasn't been impersonated.
            //  If we are running in the interactive logged on user, then
            //  use the one cached in CSRSS if the cache is valid.  Otherwise,
            //  use the process cached locale.
            //
            if (gInteractiveLogonUserProcess == (BOOL) -1)
            {
                NlsIsInteractiveUserProcess();
            }

            if ((gInteractiveLogonUserProcess != FALSE) &&
                (pNlsUserInfo->fCacheValid != FALSE))
            {
                Lcid = pNlsUserInfo->UserLocaleId;
            }
            else
            {
                if (!gProcessLocale)
                {
                    if (!NT_SUCCESS (NlsGetUserLocale(&gProcessLocale)) )
                    {
                        gProcessLocale = gSystemLocale;
                    }
                }

                Lcid = gProcessLocale;
            }

            break;
        }
    }

    return (Lcid);
}


////////////////////////////////////////////////////////////////////////////
//
//  VerLanguageNameW
//
//  Returns the language name of the given language id in the language of
//  the current user.
//
//  05-31-91    JulieB    Moved and Rewrote from Version Library.
////////////////////////////////////////////////////////////////////////////

DWORD WINAPI VerLanguageNameW(
    DWORD wLang,
    LPWSTR szLang,
    DWORD wSize)
{
    DWORD Length;            // length of string
    LPWSTR pString;          // pointer to string


    //
    //  Try to get the localized language name for the given ID.
    //
    if (!(Length = GetLocalizedLanguageName((LANGID)wLang, &pString)))
    {
        //
        //  Can't get the name of the language id passed in, so get
        //  the "language neutral" name.
        //
        Length = GetLocalizedLanguageName((LANGID)LANG_NEUTRAL, &pString);
    }

    //
    //  If the length is too big for the buffer, then reset the length
    //  to the size of the given buffer.
    //
    if (Length >= wSize)
    {
        Length = wSize - 1;
    }

    //
    //  Copy the string to the buffer and zero terminate it.
    //
    wcsncpy(szLang, pString, Length);
    szLang[Length] = 0;

    //
    //  Return the number of characters in the string, NOT including
    //  the null termination.
    //
    return (Length);
}


////////////////////////////////////////////////////////////////////////////
//
//  VerLanguageNameA
//
//  Returns the language name of the given language id in the language of
//  the current user.
//
//  05-31-91    JulieB    Moved from Version Library.
////////////////////////////////////////////////////////////////////////////

DWORD WINAPI VerLanguageNameA(
    DWORD wLang,
    LPSTR szLang,
    DWORD wSize)
{
    UNICODE_STRING Language;           // unicode string buffer
    ANSI_STRING AnsiString;            // ansi string buffer
    DWORD Status;                      // return status


    //
    //  Allocate Unicode string structure and set the fields with the
    //  given parameters.
    //
    Language.Buffer = RtlAllocateHeap( RtlProcessHeap(),
                                       0,
                                       sizeof(WCHAR) * wSize );

    Language.MaximumLength = (USHORT)(wSize * sizeof(WCHAR));

    //
    //  Make sure the allocation succeeded.
    //
    if (Language.Buffer == NULL)
    {
        return (FALSE);
    }

    //
    //  Get the language name (in Unicode).
    //
    Status = VerLanguageNameW( wLang,
                               Language.Buffer,
                               wSize );

    Language.Length = (USHORT)(Status * sizeof(WCHAR));

    //
    //  Convert unicode string to ansi.
    //
    AnsiString.Buffer = szLang;
    AnsiString.Length = AnsiString.MaximumLength = (USHORT)wSize;
    RtlUnicodeStringToAnsiString(&AnsiString, &Language, FALSE);
    Status = AnsiString.Length;
    RtlFreeUnicodeString(&Language);

    //
    //  Return the value returned from VerLanguageNameW.
    //
    return (Status);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetLocaleInfoW
//
//  Returns one of the various pieces of information about a particular
//  locale by querying the configuration registry.  This call also indicates
//  how much memory is necessary to contain the desired information.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WINAPI GetLocaleInfoW(
    LCID Locale,
    LCTYPE LCType,
    LPWSTR lpLCData,
    int cchData)
{
    PLOC_HASH pHashN;                       // ptr to LOC hash node
    int Length = 0;                         // length of info string
    LPWSTR pString;                         // ptr to the info string
    LPWORD pStart;                          // ptr to starting point
    BOOL UserOverride = TRUE;               // use user override
    BOOL ReturnNum = FALSE;                 // return number instead of string
    LPWSTR pTmp;                            // tmp ptr to info string
    int Repeat;                             // # repetitions of same letter
    WCHAR pTemp[MAX_REG_VAL_SIZE];          // temp buffer
    UNICODE_STRING ObUnicodeStr;            // value string
    int Base = 0;                           // base for str to int conversion


    //
    //  Invalid Parameter Check:
    //    - validate LCID
    //    - count is negative
    //    - NULL data pointer AND count is not zero
    //
    //  NOTE: invalid type is checked in the switch statement below.
    //
    VALIDATE_LOCALE(Locale, pHashN, FALSE);
    if ( (pHashN == NULL) ||
         (cchData < 0) ||
         ((lpLCData == NULL) && (cchData != 0)) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (0);
    }

    //
    //  Set the base value to add to in order to get the variable
    //  length strings.
    //
    pStart = (LPWORD)(pHashN->pLocaleHdr);

    //
    //  Check for NO USER OVERRIDE flag and remove the USE CP ACP flag.
    //
    if (LCType & LOCALE_NOUSEROVERRIDE)
    {
        //
        //  Flag is set, so set the boolean value and remove the flag
        //  from the LCType parameter (for switch statement).
        //
        UserOverride = FALSE;
    }
    if (LCType & LOCALE_RETURN_NUMBER)
    {
        //
        //  Flag is set, so set the boolean value and remove the flag
        //  from the LCType parameter (for switch statement).
        //
        ReturnNum = TRUE;
    }
    LCType = NLS_GET_LCTYPE_VALUE(LCType);

    //
    //  Return the appropriate information for the given LCTYPE.
    //  If user information exists for the given LCTYPE, then
    //  the user default is returned instead of the system default.
    //
    switch (LCType)
    {
        case ( LOCALE_ILANGUAGE ) :
        {
            Base = 16;
            pString = pHashN->pLocaleFixed->szILanguage;
            break;
        }
        case ( LOCALE_SLANGUAGE ) :
        {
            //
            //  Get the information from the RC file.
            //
            //  The pString pointer is set after GetLocalizedLanguageName
            //  is called.
            //
            Length = GetLocalizedLanguageName( LANGIDFROMLCID(Locale),
                                               &pString );
            if (Length == 0)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (0);
            }
            break;
        }
        case ( LOCALE_SENGLANGUAGE ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SEngLanguage;
            break;
        }
        case ( LOCALE_SABBREVLANGNAME ) :
        {
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->sAbbrevLangName,
                             NLS_VALUE_SLANGUAGE,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->SAbbrevLang;
            }
            break;
        }
        case ( LOCALE_SISO639LANGNAME ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevLangISO;
            break;
        }
        case ( LOCALE_SNATIVELANGNAME ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SNativeLang;
            break;
        }
        case ( LOCALE_ICOUNTRY ) :
        {
            Base = 10;
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->iCountry,
                             NLS_VALUE_ICOUNTRY,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pHashN->pLocaleFixed->szICountry;
            }
            break;
        }
        case ( LOCALE_SCOUNTRY ) :
        {
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->sCountry,
                             NLS_VALUE_SCOUNTRY,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;
            }
            else
            {
                //
                //  Get the information from the RC file.
                //
                //  The pString pointer is set after GetLocalizedCountryName
                //  is called.
                //
                if (!GetLocalizedCountryName( LANGIDFROMLCID(Locale),
                                              &pString ))
                {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    return (0);
                }
            }
            break;
        }
        case ( LOCALE_SENGCOUNTRY ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SEngCountry;
            break;
        }
        case ( LOCALE_SABBREVCTRYNAME ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevCtry;
            break;
        }
        case ( LOCALE_SISO3166CTRYNAME ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevCtryISO;
            break;
        }
        case ( LOCALE_SNATIVECTRYNAME ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SNativeCtry;
            break;
        }
        case ( LOCALE_SSORTNAME ) :
        {
            //
            //  Get the information from the RC file.
            //
            //  The pString pointer is set after GetLocalizedSortName
            //  is called.
            //
            if (!GetLocalizedSortName(Locale, &pString))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (0);
            }
            break;
        }
        case ( LOCALE_IDEFAULTLANGUAGE ) :
        {
            Base = 16;
            pString = pHashN->pLocaleFixed->szIDefaultLang;
            break;
        }
        case ( LOCALE_IDEFAULTCOUNTRY ) :
        {
            Base = 10;
            pString = pHashN->pLocaleFixed->szIDefaultCtry;
            break;
        }
        case ( LOCALE_IDEFAULTANSICODEPAGE ) :
        {
            if (ReturnNum)
            {
                if (cchData < 2)
                {
                    if (cchData == 0)
                    {
                        //
                        //  DWORD is needed for this option (2 WORDS),
                        //  so return 2.
                        //
                        return (2);
                    }

                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    return (0);
                }

                //
                //  Copy the value to lpLCData and return 2
                //  (2 WORDS = 1 DWORD).
                //
                *((LPDWORD)lpLCData) = (DWORD)(pHashN->pLocaleFixed->DefaultACP);
                return (2);
            }

            pString = pHashN->pLocaleFixed->szIDefaultACP;
            break;
        }
        case ( LOCALE_IDEFAULTCODEPAGE ) :
        {
            Base = 10;
            pString = pHashN->pLocaleFixed->szIDefaultOCP;
            break;
        }
        case ( LOCALE_IDEFAULTMACCODEPAGE ) :
        {
            Base = 10;
            pString = pHashN->pLocaleFixed->szIDefaultMACCP;
            break;
        }
        case ( LOCALE_IDEFAULTEBCDICCODEPAGE ) :
        {
            Base = 10;
            pString = pHashN->pLocaleFixed->szIDefaultEBCDICCP;
            break;
        }
        case ( LOCALE_SLIST ) :
        {
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->sList,
                             NLS_VALUE_SLIST,
                             pTemp,
                             FALSE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->SList;
            }
            break;
        }
        case ( LOCALE_IMEASURE ) :
        {
            Base = 10;
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->iMeasure,
                             NLS_VALUE_IMEASURE,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pHashN->pLocaleFixed->szIMeasure;
            }
            break;
        }
        case ( LOCALE_IPAPERSIZE ) :
        {
            Base = 10;
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->iPaperSize,
                             NLS_VALUE_IPAPERSIZE,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pHashN->pLocaleFixed->szIPaperSize;
            }
            break;
        }
        case ( LOCALE_SDECIMAL ) :
        {
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->sDecimal,
                             NLS_VALUE_SDECIMAL,
                             pTemp,
                             FALSE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->SDecimal;
            }
            break;
        }
        case ( LOCALE_STHOUSAND ) :
        {
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->sThousand,
                             NLS_VALUE_STHOUSAND,
                             pTemp,
                             FALSE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->SThousand;
            }
            break;
        }
        case ( LOCALE_SGROUPING ) :
        {
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->sGrouping,
                             NLS_VALUE_SGROUPING,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->SGrouping;
            }
            break;
        }
        case ( LOCALE_IDIGITS ) :
        {
            Base = 10;
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->iDigits,
                             NLS_VALUE_IDIGITS,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pHashN->pLocaleFixed->szIDigits;
            }
            break;
        }
        case ( LOCALE_ILZERO ) :
        {
            Base = 10;
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->iLZero,
                             NLS_VALUE_ILZERO,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pHashN->pLocaleFixed->szILZero;
            }
            break;
        }
        case ( LOCALE_INEGNUMBER ) :
        {
            Base = 10;
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->iNegNumber,
                             NLS_VALUE_INEGNUMBER,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pHashN->pLocaleFixed->szINegNumber;
            }
            break;
        }
        case ( LOCALE_SNATIVEDIGITS ) :
        {
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->sNativeDigits,
                             NLS_VALUE_SNATIVEDIGITS,
                             pTemp,
                             FALSE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->SNativeDigits;
            }
            break;
        }
        case ( LOCALE_IDIGITSUBSTITUTION ) :
        {
            Base = 10;
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->iDigitSubstitution,
                             NLS_VALUE_IDIGITSUBST,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pHashN->pLocaleFixed->szIDigitSubstitution;
            }
            break;
        }
        case ( LOCALE_SCURRENCY ) :
        {
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->sCurrency,
                             NLS_VALUE_SCURRENCY,
                             pTemp,
                             FALSE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->SCurrency;
            }
            break;
        }
        case ( LOCALE_SINTLSYMBOL ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SIntlSymbol;
            break;
        }
        case ( LOCALE_SENGCURRNAME ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SEngCurrName;
            break;
        }
        case ( LOCALE_SNATIVECURRNAME ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SNativeCurrName;
            break;
        }
        case ( LOCALE_SMONDECIMALSEP ) :
        {
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->sMonDecSep,
                             NLS_VALUE_SMONDECIMALSEP,
                             pTemp,
                             FALSE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->SMonDecSep;
            }
            break;
        }
        case ( LOCALE_SMONTHOUSANDSEP ) :
        {
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->sMonThouSep,
                             NLS_VALUE_SMONTHOUSANDSEP,
                             pTemp,
                             FALSE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->SMonThousSep;
            }
            break;
        }
        case ( LOCALE_SMONGROUPING ) :
        {
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->sMonGrouping,
                             NLS_VALUE_SMONGROUPING,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->SMonGrouping;
            }
            break;
        }
        case ( LOCALE_ICURRDIGITS ) :
        {
            Base = 10;
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->iCurrDigits,
                             NLS_VALUE_ICURRDIGITS,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pHashN->pLocaleFixed->szICurrDigits;
            }
            break;
        }
        case ( LOCALE_IINTLCURRDIGITS ) :
        {
            Base = 10;
            pString = pHashN->pLocaleFixed->szIIntlCurrDigits;
            break;
        }
        case ( LOCALE_ICURRENCY ) :
        {
            Base = 10;
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->iCurrency,
                             NLS_VALUE_ICURRENCY,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pHashN->pLocaleFixed->szICurrency;
            }
            break;
        }
        case ( LOCALE_INEGCURR ) :
        {
            Base = 10;
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->iNegCurr,
                             NLS_VALUE_INEGCURR,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pHashN->pLocaleFixed->szINegCurr;
            }
            break;
        }
        case ( LOCALE_SPOSITIVESIGN ) :
        {
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->sPosSign,
                             NLS_VALUE_SPOSITIVESIGN,
                             pTemp,
                             FALSE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->SPositiveSign;
            }
            break;
        }
        case ( LOCALE_SNEGATIVESIGN ) :
        {
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->sNegSign,
                             NLS_VALUE_SNEGATIVESIGN,
                             pTemp,
                             FALSE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->SNegativeSign;
            }
            break;
        }
        case ( LOCALE_IPOSSIGNPOSN ) :
        {
            //
            //  Since there is no positive sign in any of the ICURRENCY
            //  options, use the INEGCURR options instead.  All known
            //  locales would use the positive sign in the same position
            //  as the negative sign.
            //
            //  NOTE:  For the 2 options that use parenthesis, put the
            //         positive sign at the beginning of the string
            //         (where the opening parenthesis is).
            //
            //      1  =>  4, 5, 8, 15
            //      2  =>  3, 11
            //      3  =>  0, 1, 6, 9, 13, 14
            //      4  =>  2, 7, 10, 12
            //
            Base = 10;
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->iNegCurr,
                             NLS_VALUE_INEGCURR,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;

                //
                //  Set the appropriate value in pString.
                //
                switch (*pString)
                {
                    case ( L'4' ) :
                    case ( L'5' ) :
                    case ( L'8' ) :
                    {
                        *pString = L'1';
                        *(pString + 1) = 0;
                        break;
                    }
                    case ( L'3' ) :
                    {
                        *pString = L'2';
                        *(pString + 1) = 0;
                        break;
                    }
                    case ( L'0' ) :
                    case ( L'6' ) :
                    case ( L'9' ) :
                    {
                        *pString = L'3';
                        *(pString + 1) = 0;
                        break;
                    }
                    case ( L'2' ) :
                    case ( L'7' ) :
                    {
                        *pString = L'4';
                        *(pString + 1) = 0;
                        break;
                    }
                    case ( L'1' ) :
                    {
                        switch (*(pString + 1))
                        {
                            case ( 0 ) :
                            case ( L'3' ) :
                            case ( L'4' ) :
                            default :
                            {
                                *pString = L'3';
                                *(pString + 1) = 0;
                                break;
                            }
                            case ( L'0' ) :
                            case ( L'2' ) :
                            {
                                *pString = L'4';
                                *(pString + 1) = 0;
                                break;
                            }
                            case ( L'1' ) :
                            {
                                *pString = L'2';
                                *(pString + 1) = 0;
                                break;
                            }
                            case ( L'5' ) :
                            {
                                *pString = L'1';
                                *(pString + 1) = 0;
                                break;
                            }
                        }
                        break;
                    }
                    default :
                    {
                        pString = pHashN->pLocaleFixed->szIPosSignPosn;
                        break;
                    }
                }
            }
            else
            {
                pString = pHashN->pLocaleFixed->szIPosSignPosn;
            }
            break;
        }
        case ( LOCALE_INEGSIGNPOSN ) :
        {
            //
            //  Use the INEGCURR value from the user portion of the
            //  registry, if it exists.
            //
            //      0  =>  0, 4, 14, 15
            //      1  =>  5, 8
            //      2  =>  3, 11
            //      3  =>  1, 6, 9, 13
            //      4  =>  2, 7, 10, 12
            //
            Base = 10;
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->iNegCurr,
                             NLS_VALUE_INEGCURR,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;

                //
                //  Set the appropriate value in pString.
                //
                switch (*pString)
                {
                    case ( L'0' ) :
                    case ( L'4' ) :
                    {
                        *pString = L'0';
                        *(pString + 1) = 0;
                        break;
                    }
                    case ( L'5' ) :
                    case ( L'8' ) :
                    {
                        *pString = L'1';
                        *(pString + 1) = 0;
                        break;
                    }
                    case ( L'3' ) :
                    {
                        *pString = L'2';
                        *(pString + 1) = 0;
                        break;
                    }
                    case ( L'6' ) :
                    case ( L'9' ) :
                    {
                        *pString = L'3';
                        *(pString + 1) = 0;
                        break;
                    }
                    case ( L'2' ) :
                    case ( L'7' ) :
                    {
                        *pString = L'4';
                        *(pString + 1) = 0;
                        break;
                    }
                    case ( L'1' ) :
                    {
                        switch (*(pString + 1))
                        {
                            case ( 0 ) :
                            case ( L'3' ) :
                            default :
                            {
                                *pString = L'3';
                                *(pString + 1) = 0;
                                break;
                            }
                            case ( L'0' ) :
                            case ( L'2' ) :
                            {
                                *pString = L'4';
                                *(pString + 1) = 0;
                                break;
                            }
                            case ( L'1' ) :
                            {
                                *pString = L'2';
                                *(pString + 1) = 0;
                                break;
                            }
                            case ( L'4' ) :
                            case ( L'5' ) :
                            {
                                *pString = L'0';
                                *(pString + 1) = 0;
                                break;
                            }
                        }
                        break;
                    }
                    default :
                    {
                        pString = pHashN->pLocaleFixed->szINegSignPosn;
                        break;
                    }
                }
            }
            else
            {
                pString = pHashN->pLocaleFixed->szINegSignPosn;
            }
            break;
        }
        case ( LOCALE_IPOSSYMPRECEDES ) :
        {
            //
            //  Use the ICURRENCY value from the user portion of the
            //  registry, if it exists.
            //
            //      0  =>  1, 3
            //      1  =>  0, 2
            //
            Base = 10;
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->iCurrency,
                             NLS_VALUE_ICURRENCY,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;

                //
                //  Set the appropriate value in pString.
                //
                switch (*pString)
                {
                    case ( L'1' ) :
                    case ( L'3' ) :
                    {
                        *pString = L'0';
                        *(pString + 1) = 0;
                        break;
                    }
                    case ( L'0' ) :
                    case ( L'2' ) :
                    {
                        *pString = L'1';
                        *(pString + 1) = 0;
                        break;
                    }
                    default :
                    {
                        pString = pHashN->pLocaleFixed->szIPosSymPrecedes;
                        break;
                    }
                }
            }
            else
            {
                pString = pHashN->pLocaleFixed->szIPosSymPrecedes;
            }
            break;
        }
        case ( LOCALE_IPOSSEPBYSPACE ) :
        {
            //
            //  Use the ICURRENCY value from the user portion of the
            //  registry, if it exists.
            //
            //      0  =>  0, 1
            //      1  =>  2, 3
            //
            Base = 10;
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->iCurrency,
                             NLS_VALUE_ICURRENCY,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;

                //
                //  Set the appropriate value in pString.
                //
                switch (*pString)
                {
                    case ( L'0' ) :
                    case ( L'1' ) :
                    {
                        *pString = L'0';
                        *(pString + 1) = 0;
                        break;
                    }
                    case ( L'2' ) :
                    case ( L'3' ) :
                    {
                        *pString = L'1';
                        *(pString + 1) = 0;
                        break;
                    }
                    default :
                    {
                        pString = pHashN->pLocaleFixed->szIPosSepBySpace;
                        break;
                    }
                }
            }
            else
            {
                pString = pHashN->pLocaleFixed->szIPosSepBySpace;
            }
            break;
        }
        case ( LOCALE_INEGSYMPRECEDES ) :
        {
            //
            //  Use the INEGCURR value from the user portion of the
            //  registry, if it exists.
            //
            //      0  =>  4, 5, 6, 7, 8, 10, 13, 15
            //      1  =>  0, 1, 2, 3, 9, 11, 12, 14
            //
            Base = 10;
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->iNegCurr,
                             NLS_VALUE_INEGCURR,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;

                //
                //  Set the appropriate value in pString.
                //
                switch (*pString)
                {
                    case ( L'4' ) :
                    case ( L'5' ) :
                    case ( L'6' ) :
                    case ( L'7' ) :
                    case ( L'8' ) :
                    {
                        *pString = L'0';
                        *(pString + 1) = 0;
                        break;
                    }
                    case ( L'0' ) :
                    case ( L'2' ) :
                    case ( L'3' ) :
                    case ( L'9' ) :
                    {
                        *pString = L'1';
                        *(pString + 1) = 0;
                        break;
                    }
                    case ( L'1' ) :
                    {
                        if ((*(pString + 1) == L'0') ||
                            (*(pString + 1) == L'3') ||
                            (*(pString + 1) == L'5'))
                        {
                            *pString = L'0';
                            *(pString + 1) = 0;
                        }
                        else
                        {
                            *pString = L'1';
                            *(pString + 1) = 0;
                        }
                        break;
                    }
                    default :
                    {
                        pString = pHashN->pLocaleFixed->szINegSymPrecedes;
                        break;
                    }
                }
            }
            else
            {
                pString = pHashN->pLocaleFixed->szINegSymPrecedes;
            }
            break;
        }
        case ( LOCALE_INEGSEPBYSPACE ) :
        {
            //
            //  Use the INEGCURR value from the user portion of the
            //  registry, if it exists.
            //
            //      0  =>  0, 1, 2, 3, 4, 5, 6, 7
            //      1  =>  8, 9, 10, 11, 12, 13, 14, 15
            //
            Base = 10;
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->iNegCurr,
                             NLS_VALUE_INEGCURR,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;

                //
                //  Set the appropriate value in pString.
                //
                switch (*pString)
                {
                    case ( L'0' ) :
                    case ( L'2' ) :
                    case ( L'3' ) :
                    case ( L'4' ) :
                    case ( L'5' ) :
                    case ( L'6' ) :
                    case ( L'7' ) :
                    {
                        *pString = L'0';
                        *(pString + 1) = 0;
                        break;
                    }
                    case ( L'8' ) :
                    case ( L'9' ) :
                    {
                        *pString = L'1';
                        *(pString + 1) = 0;
                        break;
                    }
                    case ( L'1' ) :
                    {
                        if (*(pString + 1) == 0)
                        {
                            *pString = L'0';
                            *(pString + 1) = 0;
                        }
                        else
                        {
                            *pString = L'1';
                            *(pString + 1) = 0;
                        }
                        break;
                    }
                    default :
                    {
                        pString = pHashN->pLocaleFixed->szINegSepBySpace;
                        break;
                    }
                }
            }
            else
            {
                pString = pHashN->pLocaleFixed->szINegSepBySpace;
            }
            break;
        }
        case ( LOCALE_STIMEFORMAT ) :
        {
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->sTimeFormat,
                             NLS_VALUE_STIMEFORMAT,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->STimeFormat;
            }
            break;
        }
        case ( LOCALE_STIME ) :
        {
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->sTime,
                             NLS_VALUE_STIME,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->STime;
            }
            break;
        }
        case ( LOCALE_ITIME ) :
        {
            Base = 10;
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->iTime,
                             NLS_VALUE_ITIME,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pHashN->pLocaleFixed->szITime;
            }
            break;
        }
        case ( LOCALE_ITLZERO ) :
        {
            Base = 10;
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->iTLZero,
                             NLS_VALUE_ITLZERO,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pHashN->pLocaleFixed->szITLZero;
            }
            break;
        }
        case ( LOCALE_ITIMEMARKPOSN ) :
        {
            Base = 10;
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->iTimeMarkPosn,
                             NLS_VALUE_ITIMEMARKPOSN,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pHashN->pLocaleFixed->szITimeMarkPosn;
            }
            break;
        }
        case ( LOCALE_S1159 ) :
        {
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->s1159,
                             NLS_VALUE_S1159,
                             pTemp,
                             FALSE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->S1159;
            }
            break;
        }
        case ( LOCALE_S2359 ) :
        {
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->s2359,
                             NLS_VALUE_S2359,
                             pTemp,
                             FALSE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->S2359;
            }
            break;
        }
        case ( LOCALE_SSHORTDATE ) :
        {
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->sShortDate,
                             NLS_VALUE_SSHORTDATE,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->SShortDate;
            }
            break;
        }
        case ( LOCALE_SDATE ) :
        {
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->sDate,
                             NLS_VALUE_SDATE,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->SDate;
            }
            break;
        }
        case ( LOCALE_IDATE ) :
        {
            Base = 10;
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->iDate,
                             NLS_VALUE_IDATE,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pHashN->pLocaleFixed->szIDate;
            }
            break;
        }
        case ( LOCALE_ICENTURY ) :
        {
            //
            //  Use the short date picture to get this information.
            //
            Base = 10;
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->sShortDate,
                             NLS_VALUE_SSHORTDATE,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;

                //
                //  Find out how many y's in string.
                //  No need to ignore quotes in short date.
                //
                pTmp = pString;
                while ((*pTmp) &&
                       (*pTmp != L'y'))
                {
                    pTmp++;
                }

                //
                //  Set the appropriate value in pString.
                //
                if (*pTmp == L'y')
                {
                    //
                    //  Get the number of 'y' repetitions in the format string.
                    //
                    pTmp++;
                    for (Repeat = 0; (*pTmp == L'y'); Repeat++, pTmp++)
                        ;

                    switch (Repeat)
                    {
                        case ( 0 ) :
                        case ( 1 ) :
                        {
                            //
                            //  Two-digit century with leading zero.
                            //
                            *pString = L'0';
                            *(pString + 1) = 0;

                            break;
                        }

                        case ( 2 ) :
                        case ( 3 ) :
                        default :
                        {
                            //
                            //  Full century.
                            //
                            *pString = L'1';
                            *(pString + 1) = 0;

                            break;
                        }
                    }

                    break;
                }
            }

            //
            //  Use the system default value.
            //
            pString = pHashN->pLocaleFixed->szICentury;

            break;
        }
        case ( LOCALE_IDAYLZERO ) :
        {
            //
            //  Use the short date picture to get this information.
            //
            Base = 10;
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->sShortDate,
                             NLS_VALUE_SSHORTDATE,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;

                //
                //  Find out how many d's in string.
                //  No need to ignore quotes in short date.
                //
                pTmp = pString;
                while ((*pTmp) &&
                       (*pTmp != L'd'))
                {
                    pTmp++;
                }

                //
                //  Set the appropriate value in pString.
                //
                if (*pTmp == L'd')
                {
                    //
                    //  Get the number of 'd' repetitions in the format string.
                    //
                    pTmp++;
                    for (Repeat = 0; (*pTmp == L'd'); Repeat++, pTmp++)
                        ;

                    switch (Repeat)
                    {
                        case ( 0 ) :
                        {
                            //
                            //  No leading zero.
                            //
                            *pString = L'0';
                            *(pString + 1) = 0;

                            break;
                        }

                        case ( 1 ) :
                        default :
                        {
                            //
                            //  Use leading zero.
                            //
                            *pString = L'1';
                            *(pString + 1) = 0;

                            break;
                        }
                    }

                    break;
                }
            }

            //
            //  Use the system default value.
            //
            pString = pHashN->pLocaleFixed->szIDayLZero;

            break;
        }
        case ( LOCALE_IMONLZERO ) :
        {
            //
            //  Use the short date picture to get this information.
            //
            Base = 10;
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->sShortDate,
                             NLS_VALUE_SSHORTDATE,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;

                //
                //  Find out how many M's in string.
                //  No need to ignore quotes in short date.
                //
                pTmp = pString;
                while ((*pTmp) &&
                       (*pTmp != L'M'))
                {
                    pTmp++;
                }

                //
                //  Set the appropriate value in pString.
                //
                if (*pTmp == L'M')
                {
                    //
                    //  Get the number of 'M' repetitions in the format string.
                    //
                    pTmp++;
                    for (Repeat = 0; (*pTmp == L'M'); Repeat++, pTmp++)
                        ;

                    switch (Repeat)
                    {
                        case ( 0 ) :
                        {
                            //
                            //  No leading zero.
                            //
                            *pString = L'0';
                            *(pString + 1) = 0;

                            break;
                        }

                        case ( 1 ) :
                        default :
                        {
                            //
                            //  Use leading zero.
                            //
                            *pString = L'1';
                            *(pString + 1) = 0;

                            break;
                        }
                    }

                    break;
                }
            }

            //
            //  Use the system default value.
            //
            pString = pHashN->pLocaleFixed->szIMonLZero;

            break;
        }
        case ( LOCALE_SYEARMONTH ) :
        {
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->sYearMonth,
                             NLS_VALUE_SYEARMONTH,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->SYearMonth;
            }
            break;
        }
        case ( LOCALE_SLONGDATE ) :
        {
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->sLongDate,
                             NLS_VALUE_SLONGDATE,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->SLongDate;
            }
            break;
        }
        case ( LOCALE_ILDATE ) :
        {
            //
            //  Use the long date picture to get this information.
            //
            Base = 10;
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->sLongDate,
                             NLS_VALUE_SLONGDATE,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;

                //
                //  Find out if d, M, or y is first, but ignore quotes.
                //  Also, if "ddd" or "dddd" is found, then skip it.  Only
                //  want "d" or "dd".
                //
                pTmp = pString;
                while (pTmp = wcspbrk(pTmp, L"dMy'"))
                {
                    //
                    //  Check special cases.
                    //
                    if (*pTmp == L'd')
                    {
                        //
                        //  Check for d's.  Ignore more than 2 d's.
                        //
                        for (Repeat = 0; (*pTmp == L'd'); Repeat++, pTmp++)
                            ;

                        if (Repeat < 3)
                        {
                            //
                            //  Break out of while loop.  Found "d" or "dd".
                            //
                            pTmp--;
                            break;
                        }
                    }
                    else if (*pTmp == NLS_CHAR_QUOTE)
                    {
                        //
                        //  Ignore quotes.
                        //
                        pTmp++;
                        while ((*pTmp) && (*pTmp != NLS_CHAR_QUOTE))
                        {
                            pTmp++;
                        }
                        pTmp++;
                    }
                    else
                    {
                        //
                        //  Found one of the values, so break out of
                        //  while loop.
                        //
                        break;
                    }
                }

                //
                //  Set the appropriate value in pString.
                //
                if (pTmp)
                {
                    switch (*pTmp)
                    {
                        case ( L'd' ) :
                        {
                            *pString = L'1';
                            break;
                        }
                        case ( L'M' ) :
                        {
                            *pString = L'0';
                            break;
                        }
                        case ( L'y' ) :
                        {
                            *pString = L'2';
                            break;
                        }
                    }

                    //
                    //  Null terminate the string.
                    //
                    *(pString + 1) = 0;

                    break;
                }
            }

            //
            //  Use the default value.
            //
            pString = pHashN->pLocaleFixed->szILDate;

            break;
        }
        case ( LOCALE_ICALENDARTYPE ) :
        {
            Base = 10;
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->iCalType,
                             NLS_VALUE_ICALENDARTYPE,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pHashN->pLocaleFixed->szICalendarType;
            }
            break;
        }
        case ( LOCALE_IOPTIONALCALENDAR ) :
        {
            Base = 10;
            pString = pStart + pHashN->pLocaleHdr->IOptionalCal;
            pString = ((POPT_CAL)pString)->pCalStr;
            break;
        }
        case ( LOCALE_IFIRSTDAYOFWEEK ) :
        {
            Base = 10;
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->iFirstDay,
                             NLS_VALUE_IFIRSTDAYOFWEEK,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pHashN->pLocaleFixed->szIFirstDayOfWk;
            }
            break;
        }
        case ( LOCALE_IFIRSTWEEKOFYEAR ) :
        {
            Base = 10;
            if (UserOverride &&
                GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->iFirstWeek,
                             NLS_VALUE_IFIRSTWEEKOFYEAR,
                             pTemp,
                             TRUE ))
            {
                pString = pTemp;
            }
            else
            {
                pString = pHashN->pLocaleFixed->szIFirstWkOfYr;
            }
            break;
        }
        case ( LOCALE_SDAYNAME1 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SDayName1;
            break;
        }
        case ( LOCALE_SDAYNAME2 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SDayName2;
            break;
        }
        case ( LOCALE_SDAYNAME3 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SDayName3;
            break;
        }
        case ( LOCALE_SDAYNAME4 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SDayName4;
            break;
        }
        case ( LOCALE_SDAYNAME5 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SDayName5;
            break;
        }
        case ( LOCALE_SDAYNAME6 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SDayName6;
            break;
        }
        case ( LOCALE_SDAYNAME7 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SDayName7;
            break;
        }
        case ( LOCALE_SABBREVDAYNAME1 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevDayName1;
            break;
        }
        case ( LOCALE_SABBREVDAYNAME2 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevDayName2;
            break;
        }
        case ( LOCALE_SABBREVDAYNAME3 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevDayName3;
            break;
        }
        case ( LOCALE_SABBREVDAYNAME4 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevDayName4;
            break;
        }
        case ( LOCALE_SABBREVDAYNAME5 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevDayName5;
            break;
        }
        case ( LOCALE_SABBREVDAYNAME6 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevDayName6;
            break;
        }
        case ( LOCALE_SABBREVDAYNAME7 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevDayName7;
            break;
        }
        case ( LOCALE_SMONTHNAME1 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SMonthName1;
            break;
        }
        case ( LOCALE_SMONTHNAME2 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SMonthName2;
            break;
        }
        case ( LOCALE_SMONTHNAME3 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SMonthName3;
            break;
        }
        case ( LOCALE_SMONTHNAME4 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SMonthName4;
            break;
        }
        case ( LOCALE_SMONTHNAME5 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SMonthName5;
            break;
        }
        case ( LOCALE_SMONTHNAME6 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SMonthName6;
            break;
        }
        case ( LOCALE_SMONTHNAME7 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SMonthName7;
            break;
        }
        case ( LOCALE_SMONTHNAME8 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SMonthName8;
            break;
        }
        case ( LOCALE_SMONTHNAME9 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SMonthName9;
            break;
        }
        case ( LOCALE_SMONTHNAME10 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SMonthName10;
            break;
        }
        case ( LOCALE_SMONTHNAME11 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SMonthName11;
            break;
        }
        case ( LOCALE_SMONTHNAME12 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SMonthName12;
            break;
        }
        case ( LOCALE_SMONTHNAME13 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SMonthName13;
            break;
        }
        case ( LOCALE_SABBREVMONTHNAME1 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevMonthName1;
            break;
        }
        case ( LOCALE_SABBREVMONTHNAME2 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevMonthName2;
            break;
        }
        case ( LOCALE_SABBREVMONTHNAME3 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevMonthName3;
            break;
        }
        case ( LOCALE_SABBREVMONTHNAME4 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevMonthName4;
            break;
        }
        case ( LOCALE_SABBREVMONTHNAME5 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevMonthName5;
            break;
        }
        case ( LOCALE_SABBREVMONTHNAME6 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevMonthName6;
            break;
        }
        case ( LOCALE_SABBREVMONTHNAME7 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevMonthName7;
            break;
        }
        case ( LOCALE_SABBREVMONTHNAME8 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevMonthName8;
            break;
        }
        case ( LOCALE_SABBREVMONTHNAME9 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevMonthName9;
            break;
        }
        case ( LOCALE_SABBREVMONTHNAME10 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevMonthName10;
            break;
        }
        case ( LOCALE_SABBREVMONTHNAME11 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevMonthName11;
            break;
        }
        case ( LOCALE_SABBREVMONTHNAME12 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevMonthName12;
            break;
        }
        case ( LOCALE_SABBREVMONTHNAME13 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevMonthName13;
            break;
        }
        case ( LOCALE_FONTSIGNATURE ) :
        {
            //
            //  Check cchData for size of given buffer.
            //
            if (cchData == 0)
            {
                return (MAX_FONTSIGNATURE);
            }
            else if (cchData < MAX_FONTSIGNATURE)
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return (0);
            }

            //
            //  This string does NOT get zero terminated.
            //
            pString = pHashN->pLocaleFixed->szFontSignature;

            //
            //  Copy the string to lpLCData and return the number of
            //  characters copied.
            //
            RtlMoveMemory(lpLCData, pString, MAX_FONTSIGNATURE * sizeof(WCHAR));
            return (MAX_FONTSIGNATURE);

            break;
        }
        default :
        {
            SetLastError(ERROR_INVALID_FLAGS);
            return (0);
        }
    }

    //
    //  See if the caller wants the value in the form of a number instead
    //  of a string.
    //
    if (ReturnNum)
    {
        //
        //  Make sure the flags are valid and there is enough buffer
        //  space.
        //
        if (Base == 0)
        {
            SetLastError(ERROR_INVALID_FLAGS);
            return (0);
        }
        if (cchData < 2)
        {
            if (cchData == 0)
            {
                //
                //  DWORD is needed for this option (2 WORDS), so return 2.
                //
                return (2);
            }

            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return (0);
        }

        //
        //  Convert the string to an int and return 2 (1 DWORD = 2 WORDS).
        //
        RtlInitUnicodeString(&ObUnicodeStr, pString);
        if (RtlUnicodeStringToInteger(&ObUnicodeStr, Base, (LPDWORD)lpLCData))
        {
            SetLastError(ERROR_INVALID_FLAGS);
            return (0);
        }
        return (2);
    }

    //
    //  Get the length (in characters) of the string to copy.
    //
    if (Length == 0)
    {
        Length = NlsStrLenW(pString);
    }

    //
    //  Add one for the null termination.  All strings should be null
    //  terminated.
    //
    Length++;

    //
    //  Check cchData for size of given buffer.
    //
    if (cchData == 0)
    {
        //
        //  If cchData is 0, then we can't use lpLCData.  In this
        //  case, we simply want to return the length (in characters) of
        //  the string to be copied.
        //
        return (Length);
    }
    else if (cchData < Length)
    {
        //
        //  The buffer is too small for the string, so return an error
        //  and zero bytes written.
        //
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return (0);
    }

    //
    //  Copy the string to lpLCData and null terminate it.
    //  Return the number of characters copied.
    //
    wcsncpy(lpLCData, pString, Length - 1);
    lpLCData[Length - 1] = 0;
    return (Length);
}


////////////////////////////////////////////////////////////////////////////
//
//  SetLocaleInfoW
//
//  Sets one of the various pieces of information about a particular
//  locale by making an entry in the user's portion of the configuration
//  registry.  This will only affect the user override portion of the locale
//  settings.  The system defaults will never be reset.
//
//  07-14-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI SetLocaleInfoW(
    LCID Locale,
    LCTYPE LCType,
    LPCWSTR lpLCData)
{
    PLOC_HASH pHashN;                       // ptr to LOC hash node
    int cchData;                            // length of lpLCData
    LPWSTR pString;                         // ptr to info string to change
    LPWSTR pPos;                            // ptr to position in info string
    LPWSTR pPos2;                           // ptr to position in info string
    LPWSTR pSep;                            // ptr to separator string
    WCHAR pTemp[MAX_PATH_LEN];              // ptr to temp storage buffer
    WCHAR pOutput[MAX_REG_VAL_SIZE];        // ptr to output for GetInfo call
    WCHAR pOutput2[MAX_REG_VAL_SIZE];       // ptr to output for GetInfo call
    UINT Order;                             // date or time order value
    UINT TLZero;                            // time leading zero value
    UINT TimeMarkPosn;                      // time mark position value
    WCHAR pFind[3];                         // ptr to chars to find
    int SepLen;                             // length of separator string
    UNICODE_STRING ObUnicodeStr;            // value string
    int Value;                              // value


    //
    //  Invalid Parameter Check:
    //    - validate LCID
    //    - NULL data pointer
    //
    //  NOTE: invalid type is checked in the switch statement below.
    //
    VALIDATE_LOCALE(Locale, pHashN, FALSE);
    if ((pHashN == NULL) || (lpLCData == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    //
    //  Get the length of the buffer.
    //
    cchData = NlsStrLenW(lpLCData) + 1;

    //
    //  Set the appropriate user information for the given LCTYPE.
    //
    LCType &= (~LOCALE_USE_CP_ACP);
    switch (LCType)
    {
        case ( LOCALE_SLIST ) :
        {
            //
            //  Validate the new value.  It should be no longer than
            //  MAX_SLIST wide characters in length.
            //
            if (cchData > MAX_SLIST)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Set the registry with the new SLIST string.
            //
            return (SetUserInfo( LOCALE_SLIST,
                                 (LPWSTR)lpLCData,
                                 cchData ));
            break;
        }
        case ( LOCALE_IMEASURE ) :
        {
            //
            //  Validate the new value.  It should be no longer than
            //  MAX_IMEASURE wide characters in length.
            //  It should be between 0 and MAX_VALUE_IMEASURE.
            //
            //  NOTE: The string may not be NULL, so it must be at least
            //        2 chars long (includes null).
            //
            //        Optimized - since MAX_IMEASURE is 2.
            //
            if ((cchData != MAX_IMEASURE) ||
                (*lpLCData < NLS_CHAR_ZERO) ||
                (*lpLCData > MAX_CHAR_IMEASURE))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Set the registry with the new IMEASURE string.
            //
            return (SetUserInfo( LOCALE_IMEASURE,
                                 (LPWSTR)lpLCData,
                                 cchData ));
            break;
        }
        case ( LOCALE_IPAPERSIZE ) :
        {
            //
            //  Validate the new value.
            //  It should be between DMPAPER_LETTER and DMPAPER_LAST.
            //
            //  NOTE: The string may not be NULL, so it must be at least
            //        2 chars long (includes null).
            //
            RtlInitUnicodeString(&ObUnicodeStr, lpLCData);
            if ((cchData < 2) ||
                (RtlUnicodeStringToInteger(&ObUnicodeStr, 10, &Value)) ||
                (Value < DMPAPER_LETTER) || (Value > DMPAPER_LAST))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Set the registry with the new IPAPERSIZE string.
            //
            return (SetUserInfo( LOCALE_IPAPERSIZE,
                                 (LPWSTR)lpLCData,
                                 cchData ));
            break;
        }
        case ( LOCALE_SDECIMAL ) :
        {
            //
            //  Validate the new value.  It should be no longer than
            //  MAX_SDECIMAL wide characters in length and should not
            //  contain any integer values (L'0' thru L'9').
            //
            if (!IsValidSeparatorString( lpLCData,
                                         MAX_SDECIMAL,
                                         FALSE ))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Set the registry with the new SDECIMAL string.
            //
            return (SetUserInfo( LOCALE_SDECIMAL,
                                 (LPWSTR)lpLCData,
                                 cchData ));
            break;
        }
        case ( LOCALE_STHOUSAND ) :
        {
            //
            //  Validate the new value.  It should be no longer than
            //  MAX_STHOUSAND wide characters in length and should not
            //  contain any integer values (L'0' thru L'9').
            //
            if (!IsValidSeparatorString( lpLCData,
                                         MAX_STHOUSAND,
                                         FALSE ))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Set the registry with the new STHOUSAND string.
            //
            return (SetUserInfo( LOCALE_STHOUSAND,
                                 (LPWSTR)lpLCData,
                                 cchData ));
            break;
        }
        case ( LOCALE_SGROUPING ) :
        {
            //
            //  Validate the new value.  It should be no longer than
            //  MAX_SGROUPING wide characters in length and should
            //  contain alternating integer and semicolon values.
            //       (eg. 3;2;0  or  3;0  or  0)
            //
            //  NOTE: The string may not be NULL, so it must be at least
            //        2 chars long (includes null).
            //
            if (!IsValidGroupingString( lpLCData,
                                        MAX_SGROUPING,
                                        TRUE ))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Set the registry with the new SGROUPING string.
            //
            return (SetUserInfo( LOCALE_SGROUPING,
                                 (LPWSTR)lpLCData,
                                 cchData ));
            break;
        }
        case ( LOCALE_IDIGITS ) :
        {
            //
            //  Validate the new value.  It should be no longer than
            //  MAX_IDIGITS wide characters in length.
            //  The value should be between 0 and MAX_VALUE_IDIGITS.
            //
            //  NOTE: The string may not be NULL, so it must be at least
            //        2 chars long (includes null).
            //
            //        Optimized - since MAX_IDIGITS is 2.
            //
            if ((cchData != MAX_IDIGITS) ||
                (*lpLCData < NLS_CHAR_ZERO) ||
                (*lpLCData > MAX_CHAR_IDIGITS))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Set the registry with the new IDIGITS string.
            //
            return (SetUserInfo( LOCALE_IDIGITS,
                                 (LPWSTR)lpLCData,
                                 cchData ));
            break;
        }
        case ( LOCALE_ILZERO ) :
        {
            //
            //  Validate the new value.  It should be no longer than
            //  MAX_ILZERO wide characters in length.
            //  The value should be between 0 and MAX_VALUE_ILZERO.
            //
            //  NOTE: The string may not be NULL, so it must be at least
            //        2 chars long (includes null).
            //
            //        Optimized - since MAX_ILZERO is 2.
            //
            if ((cchData != MAX_ILZERO) ||
                (*lpLCData < NLS_CHAR_ZERO) ||
                (*lpLCData > MAX_CHAR_ILZERO))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Set the registry with the new ILZERO string.
            //
            return (SetUserInfo( LOCALE_ILZERO,
                                 (LPWSTR)lpLCData,
                                 cchData ));
            break;
        }
        case ( LOCALE_INEGNUMBER ) :
        {
            //
            //  Validate the new value.  It should be no longer than
            //  MAX_INEGNUMBER wide characters in length.
            //  The value should be between 0 and MAX_VALUE_INEGNUMBER.
            //
            //  NOTE: The string may not be NULL, so it must be at least
            //        2 chars long (includes null).
            //
            //        Optimized - since MAX_INEGNUMBER is 2.
            //
            if ((cchData != MAX_INEGNUMBER) ||
                (*lpLCData < NLS_CHAR_ZERO) ||
                (*lpLCData > MAX_CHAR_INEGNUMBER))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Set the registry with the new INEGNUMBER string.
            //
            return (SetUserInfo( LOCALE_INEGNUMBER,
                                 (LPWSTR)lpLCData,
                                 cchData ));
            break;
        }
        case ( LOCALE_SNATIVEDIGITS ) :
        {
            //
            //  Validate the new value.  It should be exactly
            //  MAX_SNATIVEDIGITS wide characters in length.
            //
            if (cchData != MAX_SNATIVEDIGITS)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Set the registry with the new SNATIVEDIGITS string.
            //
            return (SetUserInfo( LOCALE_SNATIVEDIGITS,
                                 (LPWSTR)lpLCData,
                                 cchData ));
            break;
        }
        case ( LOCALE_IDIGITSUBSTITUTION ) :
        {
            //
            //  Validate the new value.  It should be no longer than
            //  MAX_IDIGITSUBST wide characters in length.
            //  The value should be between 0 and MAX_VALUE_IDIGITSUBST.
            //
            //  NOTE: The string may not be NULL, so it must be at least
            //        2 chars long (includes null).
            //
            //        Optimized - since MAX_IDIGITSUBST is 2.
            //
            if ((cchData != MAX_IDIGITSUBST) ||
                (*lpLCData < NLS_CHAR_ZERO) ||
                (*lpLCData > MAX_CHAR_IDIGITSUBST))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Set the registry with the new IDIGITSUBST string.
            //
            return (SetUserInfo( LOCALE_IDIGITSUBSTITUTION,
                                 (LPWSTR)lpLCData,
                                 cchData ));
            break;
        }
        case ( LOCALE_SCURRENCY ) :
        {
            //
            //  Validate the new value.  It should be no longer than
            //  MAX_SCURRENCY wide characters in length and should not
            //  contain any integer values (L'0' thru L'9').
            //
            if (!IsValidSeparatorString( lpLCData,
                                         MAX_SCURRENCY,
                                         FALSE ))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Set the registry with the new SCURRENCY string.
            //
            return (SetUserInfo( LOCALE_SCURRENCY,
                                 (LPWSTR)lpLCData,
                                 cchData ));
            break;
        }
        case ( LOCALE_SMONDECIMALSEP ) :
        {
            //
            //  Validate the new value.  It should be no longer than
            //  MAX_SMONDECSEP wide characters in length and should not
            //  contain any integer values (L'0' thru L'9').
            //
            if (!IsValidSeparatorString( lpLCData,
                                         MAX_SMONDECSEP,
                                         FALSE ))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Set the registry with the new SMONDECIMALSEP string.
            //
            return (SetUserInfo( LOCALE_SMONDECIMALSEP,
                                 (LPWSTR)lpLCData,
                                 cchData ));
            break;
        }
        case ( LOCALE_SMONTHOUSANDSEP ) :
        {
            //
            //  Validate the new value.  It should be no longer than
            //  MAX_SMONTHOUSEP wide characters in length and should not
            //  contain any integer values (L'0' thru L'9').
            //
            if (!IsValidSeparatorString( lpLCData,
                                         MAX_SMONTHOUSEP,
                                         FALSE ))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Set the registry with the new SMONTHOUSANDSEP string.
            //
            return (SetUserInfo( LOCALE_SMONTHOUSANDSEP,
                                 (LPWSTR)lpLCData,
                                 cchData ));
            break;
        }
        case ( LOCALE_SMONGROUPING ) :
        {
            //
            //  Validate the new value.  It should be no longer than
            //  MAX_SMONGROUPING wide characters in length and should
            //  contain alternating integer and semicolon values.
            //       (eg. 3;2;0  or  3;0  or  0)
            //
            //  NOTE: The string may not be NULL, so it must be at least
            //        2 chars long (includes null).
            //
            if (!IsValidGroupingString( lpLCData,
                                        MAX_SMONGROUPING,
                                        TRUE ))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Set the registry with the new SMONGROUPING string.
            //
            return (SetUserInfo( LOCALE_SMONGROUPING,
                                 (LPWSTR)lpLCData,
                                 cchData ));
            break;
        }
        case ( LOCALE_ICURRDIGITS ) :
        {
            //
            //  Validate the new value.
            //  The value should be between 0 and MAX_VALUE_ICURRDIGITS.
            //
            //  NOTE: The string may not be NULL, so it must be at least
            //        2 chars long (includes null).
            //
            RtlInitUnicodeString(&ObUnicodeStr, lpLCData);
            if ((cchData < 2) ||
                (RtlUnicodeStringToInteger(&ObUnicodeStr, 10, &Value)) ||
                (Value < 0) || (Value > MAX_VALUE_ICURRDIGITS) ||
                ((Value == 0) &&
                 ((*lpLCData != NLS_CHAR_ZERO) || (cchData != 2))))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Set the registry with the new ICURRDIGITS string.
            //
            return (SetUserInfo( LOCALE_ICURRDIGITS,
                                 (LPWSTR)lpLCData,
                                 cchData ));
            break;
        }
        case ( LOCALE_ICURRENCY ) :
        {
            //
            //  Validate the new value.  It should be no longer than
            //  MAX_ICURRENCY wide characters in length.
            //  The value should be between 0 and MAX_VALUE_ICURRENCY.
            //
            //  NOTE: The string may not be NULL, so it must be at least
            //        2 chars long (includes null).
            //
            //        Optimized - since MAX_ICURRENCY is 2.
            //
            if ((cchData != MAX_ICURRENCY) ||
                (*lpLCData < NLS_CHAR_ZERO) ||
                (*lpLCData > MAX_CHAR_ICURRENCY))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Set the registry with the new ICURRENCY string.
            //
            return (SetUserInfo( LOCALE_ICURRENCY,
                                 (LPWSTR)lpLCData,
                                 cchData ));
            break;
        }
        case ( LOCALE_INEGCURR ) :
        {
            //
            //  Validate the new value.
            //  The value should be between 0 and MAX_VALUE_INEGCURR.
            //
            //  NOTE: The string may not be NULL, so it must be at least
            //        2 chars long (includes null).
            //
            RtlInitUnicodeString(&ObUnicodeStr, lpLCData);
            if ((cchData < 2) ||
                (RtlUnicodeStringToInteger(&ObUnicodeStr, 10, &Value)) ||
                (Value < 0) || (Value > MAX_VALUE_INEGCURR) ||
                ((Value == 0) &&
                 ((*lpLCData != NLS_CHAR_ZERO) || (cchData != 2))))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Set the registry with the new INEGCURR string.
            //
            return (SetUserInfo( LOCALE_INEGCURR,
                                 (LPWSTR)lpLCData,
                                 cchData ));
            break;
        }
        case ( LOCALE_SPOSITIVESIGN ) :
        {
            //
            //  Validate the new value.  It should be no longer than
            //  MAX_SPOSSIGN wide characters in length and should not
            //  contain any integer values (L'0' thru L'9').
            //
            if (!IsValidSeparatorString( lpLCData,
                                         MAX_SPOSSIGN,
                                         FALSE ))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Set the registry with the new SPOSITIVESIGN string.
            //
            return (SetUserInfo( LOCALE_SPOSITIVESIGN,
                                 (LPWSTR)lpLCData,
                                 cchData ));
            break;
        }
        case ( LOCALE_SNEGATIVESIGN ) :
        {
            //
            //  Validate the new value.  It should be no longer than
            //  MAX_SNEGSIGN wide characters in length and should not
            //  contain any integer values (L'0' thru L'9').
            //
            if (!IsValidSeparatorString( lpLCData,
                                         MAX_SNEGSIGN,
                                         FALSE ))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Set the registry with the new SNEGATIVESIGN string.
            //
            return (SetUserInfo( LOCALE_SNEGATIVESIGN,
                                 (LPWSTR)lpLCData,
                                 cchData ));
            break;
        }
        case ( LOCALE_STIMEFORMAT ) :
        {
            //
            //  Validate the new value.  It should be no longer than
            //  MAX_STIMEFORMAT wide characters in length.
            //
            //  NOTE: The string may not be NULL, so it must be at least
            //        2 chars long (includes null).  This is checked below
            //        in the check for whether or not there is an hour
            //        delimeter.
            //
            if (cchData > MAX_STIMEFORMAT)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  NOTE: Must link the STIME, ITIME, ITLZERO, and
            //        ITIMEMARKPOSN values in the registry.
            //

            //
            //  Search for H or h, so that iTime and iTLZero can be
            //  set.  If no H or h exists, return an error.
            //
            pPos = wcspbrk(lpLCData, L"Hh");
            if (!pPos)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Get the appropriate ITIME value.
            //
            Order = (*pPos == L'h') ? 0 : 1;

            //
            //  Get the appropriate ITLZERO value.
            //
            TLZero = ((*(pPos + 1) != L'h') && (*(pPos + 1) != L'H')) ? 0 : 1;

            //
            //  Search for tt, so that ITIMEMARKPOSN can be
            //  set.  If no tt exists, do not change the value.
            //
            pPos = (LPWSTR)lpLCData;
            while ((pPos = wcspbrk(pPos, L"t")) && (*(pPos + 1) != L't'))
            {
                pPos++;
            }
            if (pPos)
            {
                //
                //  Get the appropriate ITIMEMARKPOSN value.
                //
                pPos2 = wcspbrk(lpLCData, L"Hhmst");
                TimeMarkPosn = (pPos == pPos2) ? 1 : 0;
            }

            //
            //  Find the time separator so that STIME can be set.
            //
            pPos = (LPWSTR)lpLCData;
            while (pPos = wcspbrk(pPos, L"Hhms"))
            {
                //
                //  Go to next position past sequence of H, h, m, or s.
                //
                pPos++;
                while ((*pPos) && (wcschr( L"Hhms", *pPos )))
                {
                    pPos++;
                }

                if (*pPos)
                {
                    //
                    //  Find the end of the separator string.
                    //
                    pPos2 = wcspbrk(pPos, L"Hhmst");
                    if (pPos2)
                    {
                        if (*pPos2 == L't')
                        {
                            //
                            //  Found a time marker, so need to start over
                            //  in search for separator.  There are no
                            //  separators around the time marker.
                            //
                            pPos = pPos2 + 1;
                        }
                        else
                        {
                            //
                            //  Found end of separator, so break out of
                            //  while loop.
                            //
                            break;
                        }
                    }
                }
            }

            //
            //  Get the appropriate STIME string.
            //
            if (pPos)
            {
                //
                //  Copy to temp buffer so that it's zero terminated.
                //
                pString = pTemp;
                while (pPos != pPos2)
                {
                    //
                    // If there is a quoted string in the separator, then
                    // just punch in a white space, since there is no meaning
                    // for time field separator anymore.
                    //
                    if (*pPos == L'\'')
                    {
                        pString = pTemp;
                        *pString++ = L' ';
                        break;
                    }

                    *pString = *pPos;
                    pPos++;
                    pString++;
                }
                *pString = 0;
            }
            else
            {
                //
                //  There is no time separator, so use NULL.
                //
                *pTemp = 0;
            }

            //
            //  Validate the new value.  It should be no longer than
            //  MAX_STIME wide characters in length and should not
            //  contain any integer values (L'0' thru L'9').
            //
            //  NOTE: The string may not be NULL, so it must be at least
            //        2 chars long (includes null).
            //
            if (!IsValidSeparatorString( pTemp,
                                         MAX_STIME,
                                         TRUE ))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Make sure that the time separator does NOT contain any
            //  of the special time picture characters - h, H, m, s, t, '.
            //
            if (wcspbrk(pTemp, L"Hhmst'"))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Call the server to set the registry.
            //
            return (SetMultipleUserInfo( LCType,
                                         cchData,
                                         lpLCData,
                                         pTemp,
                                         (Order == 0) ? L"0" : L"1",
                                         (TLZero == 0) ? L"0" : L"1",
                                         (TimeMarkPosn == 0) ? L"0" : L"1" ));
            break;
        }
        case ( LOCALE_STIME ) :
        {
            //
            //  NOTE: Must link the STIMEFORMAT value in the registry.
            //

            //
            //  Validate the new value.  It should be no longer than
            //  MAX_STIME wide characters in length and should not
            //  contain any integer values (L'0' thru L'9').
            //
            //  NOTE: The string may not be NULL, so it must be at least
            //        2 chars long (includes null).
            //
            if (!IsValidSeparatorString( lpLCData,
                                         MAX_STIME,
                                         TRUE ))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Make sure that the time separator does NOT contain any
            //  of the special time picture characters - h, H, m, s, t, '.
            //
            if (wcspbrk(lpLCData, L"Hhmst'"))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Get the current setting for STIMEFORMAT.
            //
            if (GetUserInfo( Locale,
                             LOCALE_STIMEFORMAT,
                             pNlsUserInfo->sTimeFormat,
                             NLS_VALUE_STIMEFORMAT,
                             pOutput,
                             TRUE ))
            {
                pString = pOutput;
            }
            else
            {
                pString = (LPWORD)(pHashN->pLocaleHdr) +
                          pHashN->pLocaleHdr->STimeFormat;
            }

            //
            //  Get the current setting for STIME.
            //
            if (GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->sTime,
                             NLS_VALUE_STIME,
                             pOutput2,
                             TRUE ))
            {
                pSep = pOutput2;
            }
            else
            {
                pSep = (LPWORD)(pHashN->pLocaleHdr) +
                       pHashN->pLocaleHdr->STime;
            }

            //
            //  Get the length of the separator string.
            //
            SepLen = NlsStrLenW(pSep);

            //
            //  Setup the string containing the characters to find in
            //  the timeformat string.
            //
            pFind[0] = NLS_CHAR_QUOTE;
            pFind[1] = *pSep;
            pFind[2] = 0;

            //
            //  Find the time separator in the STIMEFORMAT string and
            //  replace it with the new time separator.
            //
            //  The new separator may be a different length than
            //  the old one, so must use a static buffer for the new
            //  time format string.
            //
            pPos = pTemp;
            while (pPos2 = wcspbrk(pString, pFind))
            {
                //
                //  Copy format string up to pPos2.
                //
                while (pString < pPos2)
                {
                    *pPos = *pString;
                    pPos++;
                    pString++;
                }

                switch (*pPos2)
                {
                    case ( NLS_CHAR_QUOTE ) :
                    {
                        //
                        //  Copy the quote.
                        //
                        *pPos = *pString;
                        pPos++;
                        pString++;

                        //
                        //  Copy what's inside the quotes.
                        //
                        while ((*pString) && (*pString != NLS_CHAR_QUOTE))
                        {
                            *pPos = *pString;
                            pPos++;
                            pString++;
                        }

                        //
                        //  Copy the end quote.
                        //
                        *pPos = NLS_CHAR_QUOTE;
                        pPos++;
                        if (*pString)
                        {
                            pString++;
                        }

                        break;
                    }
                    default :
                    {
                        //
                        //  Make sure it's the old separator.
                        //
                        if (NlsStrNEqualW(pString, pSep, SepLen))
                        {
                            //
                            //  Adjust pointer to skip over old separator.
                            //
                            pString += SepLen;

                            //
                            //  Copy the new separator.
                            //
                            pPos2 = (LPWSTR)lpLCData;
                            while (*pPos2)
                            {
                                *pPos = *pPos2;
                                pPos++;
                                pPos2++;
                            }
                        }
                        else
                        {
                            //
                            //  Copy the code point and continue.
                            //
                            *pPos = *pString;
                            pPos++;
                            pString++;
                        }

                        break;
                    }
                }
            }

            //
            //  Copy to the end of the string and null terminate it.
            //
            while (*pString)
            {
                *pPos = *pString;
                pPos++;
                pString++;
            }
            *pPos = 0;

            //
            //  Call the server to set the registry.
            //
            return (SetMultipleUserInfo( LCType,
                                         cchData,
                                         pTemp,
                                         lpLCData,
                                         NULL,
                                         NULL,
                                         NULL ));
            break;
        }
        case ( LOCALE_ITIME ) :
        {
            //
            //  NOTE: Must link the STIMEFORMAT value in the registry.
            //

            //
            //  Validate the new value.  It should be no longer than
            //  MAX_ITIME wide characters in length.
            //  The value should be either 0 or MAX_VALUE_ITIME.
            //
            //  NOTE: The string may not be NULL, so it must be at least
            //        2 chars long (includes null).
            //
            //        Optimized - since MAX_ITIME is 2.
            //
            if ((cchData != MAX_ITIME) ||
                (*lpLCData < NLS_CHAR_ZERO) ||
                (*lpLCData > MAX_CHAR_ITIME))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Get the current setting for STIMEFORMAT.
            //
            if (GetUserInfo( Locale,
                             LOCALE_STIMEFORMAT,
                             pNlsUserInfo->sTimeFormat,
                             NLS_VALUE_STIMEFORMAT,
                             pOutput,
                             TRUE ))
            {
                pString = pOutput;
            }
            else
            {
                //
                //  Copy system default to temp buffer.
                //
                NlsStrCpyW( pTemp,
                            (LPWORD)(pHashN->pLocaleHdr) +
                              pHashN->pLocaleHdr->STimeFormat );
                pString = pTemp;
            }

            //
            //  Search down the STIMEFORMAT string.
            //  If iTime = 0, then H -> h.
            //  If iTime = 1, then h -> H.
            //
            pPos = pString;
            if (*lpLCData == NLS_CHAR_ZERO)
            {
                while (*pPos)
                {
                    if (*pPos == L'H')
                    {
                        *pPos = L'h';
                    }
                    pPos++;
                }
            }
            else
            {
                while (*pPos)
                {
                    if (*pPos == L'h')
                    {
                        *pPos = L'H';
                    }
                    pPos++;
                }
            }

            //
            //  Call the server to set the registry.
            //
            return (SetMultipleUserInfo( LCType,
                                         cchData,
                                         pString,
                                         NULL,
                                         lpLCData,
                                         NULL,
                                         NULL ));
            break;
        }
        case ( LOCALE_S1159 ) :
        {
            //
            //  Validate the new value.  It should be no longer than
            //  MAX_S1159 wide characters in length.
            //
            if (cchData > MAX_S1159)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Set the registry with the new S1159 string.
            //
            return (SetUserInfo( LOCALE_S1159,
                                 (LPWSTR)lpLCData,
                                 cchData ));
            break;
        }
        case ( LOCALE_S2359 ) :
        {
            //
            //  Validate the new value.  It should be no longer than
            //  MAX_S2359 wide characters in length.
            //
            if (cchData > MAX_S2359)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Set the registry with the new S2359 string.
            //
            return (SetUserInfo( LOCALE_S2359,
                                 (LPWSTR)lpLCData,
                                 cchData ));
            break;
        }
        case ( LOCALE_SSHORTDATE ) :
        {
            //
            //  Validate the new value.  It should be no longer than
            //  MAX_SSHORTDATE wide characters in length.
            //
            //  NOTE: The string may not be NULL, so it must be at least
            //        2 chars long (includes null).  This is checked below
            //        in the check for whether or not there is a date,
            //        month, or year delimeter.
            //
            if (cchData > MAX_SSHORTDATE)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  NOTE: Must link the IDATE and SDATE values in the registry.
            //

            //
            //  Search for the 'd' or 'M' or 'y' sequence in the date format
            //  string to set the new IDATE value.
            //
            //  If none of these symbols exist in the date format string,
            //  then return an error.
            //
            pPos = wcspbrk(lpLCData, L"dMy");
            if (!pPos)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Set the registry with the appropriate IDATE string.
            //
            switch (*pPos)
            {
                case ( L'M' ) :
                {
                    Order = 0;
                    break;
                }

                case ( L'd' ) :
                {
                    Order = 1;
                    break;
                }

                case ( L'y' ) :
                {
                    Order = 2;
                    break;
                }
            }

            //
            //  Set the registry with the appropriate SDATE string.
            //
            //  The ptr "pPos" is pointing at either d, M, or y.
            //  Go to the next position past sequence of d, M, or y.
            //
            pPos++;
            while ((*pPos) && (wcschr( L"dMy", *pPos )))
            {
                pPos++;
            }

            *pTemp = 0;
            if (*pPos)
            {
                //
                //  Find the end of the separator string.
                //
                pPos2 = wcspbrk(pPos, L"dMy");
                if (pPos2)
                {
                    //
                    //  Copy to temp buffer so that it's zero terminated.
                    //
                    pString = pTemp;
                    while (pPos != pPos2)
                    {
                        //
                        // If there is a quoted string in the separator, then
                        // just punch in a white space, since there is no meaning
                        // for short date field separator anymore.
                        //
                        if (*pPos == L'\'')
                        {
                            pString = pTemp;
                            *pString++ = L' ';
                            break;
                        }
                        *pString = *pPos;
                        pPos++;
                        pString++;
                    }
                    *pString = 0;
                }
            }

            //
            // Since the date separator (LOCALE_SDATE) is being set here, we
            // should do the same validation as LOCALE_SDATE.
            //
            if (!IsValidSeparatorString( pTemp,
                                         MAX_SDATE,
                                         TRUE ))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Make sure that the date separator does NOT contain any
            //  of the special date picture characters - d, M, y, g, '.
            //
            if (wcspbrk(pTemp, L"dMyg'"))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }



            //
            //  Call the server to set the registry.
            //
            return (SetMultipleUserInfo( LCType,
                                         cchData,
                                         lpLCData,
                                         pTemp,
                                         (Order == 0) ? L"0" :
                                             ((Order == 1) ? L"1" : L"2"),
                                         NULL,
                                         NULL ));
            break;
        }
        case ( LOCALE_SDATE ) :
        {
            //
            //  NOTE: Must link the SSHORTDATE value in the registry.
            //

            //
            //  Validate the new value.  It should be no longer than
            //  MAX_SDATE wide characters in length and should not
            //  contain any integer values (L'0' thru L'9').
            //
            //  NOTE: The string may not be NULL, so it must be at least
            //        2 chars long (includes null).
            //
            if (!IsValidSeparatorString( lpLCData,
                                         MAX_SDATE,
                                         TRUE ))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Make sure that the date separator does NOT contain any
            //  of the special date picture characters - d, M, y, g, '.
            //
            if (wcspbrk(lpLCData, L"dMyg'"))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Get the current setting for SSHORTDATE.
            //
            if (GetUserInfo( Locale,
                             LOCALE_SSHORTDATE,
                             pNlsUserInfo->sShortDate,
                             NLS_VALUE_SSHORTDATE,
                             pOutput,
                             TRUE ))
            {
                pString = pOutput;
            }
            else
            {
                pString = (LPWORD)(pHashN->pLocaleHdr) +
                          pHashN->pLocaleHdr->SShortDate;
            }

            //
            //  Get the current setting for SDATE.
            //
            if (GetUserInfo( Locale,
                             LCType,
                             pNlsUserInfo->sDate,
                             NLS_VALUE_SDATE,
                             pOutput2,
                             TRUE ))
            {
                pSep = pOutput2;
            }
            else
            {
                pSep = (LPWORD)(pHashN->pLocaleHdr) +
                       pHashN->pLocaleHdr->SDate;
            }

            //
            //  Get the length of the separator string.
            //
            SepLen = NlsStrLenW(pSep);

            //
            //  Setup the string containing the characters to find in
            //  the shortdate string.
            //
            pFind[0] = NLS_CHAR_QUOTE;
            pFind[1] = *pSep;
            pFind[2] = 0;

            //
            //  Find the date separator in the SSHORTDATE string and
            //  replace it with the new date separator.
            //
            //  The new separator may be a different length than
            //  the old one, so must use a static buffer for the new
            //  short date format string.
            //
            pPos = pTemp;
            while (pPos2 = wcspbrk(pString, pFind))
            {
                //
                //  Copy format string up to pPos2.
                //
                while (pString < pPos2)
                {
                    *pPos = *pString;
                    pPos++;
                    pString++;
                }

                switch (*pPos2)
                {
                    case ( NLS_CHAR_QUOTE ) :
                    {
                        //
                        //  Copy the quote.
                        //
                        *pPos = *pString;
                        pPos++;
                        pString++;

                        //
                        //  Copy what's inside the quotes.
                        //
                        while ((*pString) && (*pString != NLS_CHAR_QUOTE))
                        {
                            *pPos = *pString;
                            pPos++;
                            pString++;
                        }

                        //
                        //  Copy the end quote.
                        //
                        *pPos = NLS_CHAR_QUOTE;
                        pPos++;
                        if (*pString)
                        {
                            pString++;
                        }

                        break;
                    }
                    default :
                    {
                        //
                        //  Make sure it's the old separator.
                        //
                        if (NlsStrNEqualW(pString, pSep, SepLen))
                        {
                            //
                            //  Adjust pointer to skip over old separator.
                            //
                            pString += SepLen;

                            //
                            //  Copy the new separator.
                            //
                            pPos2 = (LPWSTR)lpLCData;
                            while (*pPos2)
                            {
                                *pPos = *pPos2;
                                pPos++;
                                pPos2++;
                            }
                        }
                        else
                        {
                            //
                            //  Copy the code point and continue.
                            //
                            *pPos = *pString;
                            pPos++;
                            pString++;
                        }

                        break;
                    }
                }
            }

            //
            //  Copy to the end of the string and null terminate it.
            //
            while (*pString)
            {
                *pPos = *pString;
                pPos++;
                pString++;
            }
            *pPos = 0;

            //
            //  Call the server to set the registry.
            //
            return (SetMultipleUserInfo( LCType,
                                         cchData,
                                         pTemp,
                                         lpLCData,
                                         NULL,
                                         NULL,
                                         NULL ));
            break;
        }
        case ( LOCALE_SYEARMONTH ) :
        {
            //
            //  Validate the new value.  It should be no longer than
            //  MAX_SYEARMONTH wide characters in length.
            //
            //  NOTE: The string may not be NULL, so it must be at least
            //        2 chars long (includes null).  This is checked below
            //        in the check for whether or not there is a date,
            //        month, or year delimeter.
            //
            if (cchData > MAX_SYEARMONTH)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Make sure one of 'M' or 'y' exists in the date
            //  format string.  If it does not, then return an error.
            //
            pPos = wcspbrk(lpLCData, L"My");
            if (!pPos)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Set the registry with the new SYEARMONTH string.
            //
            return (SetUserInfo( LOCALE_SYEARMONTH,
                                 (LPWSTR)lpLCData,
                                 cchData ));
            break;
        }
        case ( LOCALE_SLONGDATE ) :
        {
            //
            //  Validate the new value.  It should be no longer than
            //  MAX_SLONGDATE wide characters in length.
            //
            //  NOTE: The string may not be NULL, so it must be at least
            //        2 chars long (includes null).  This is checked below
            //        in the check for whether or not there is a date,
            //        month, or year delimeter.
            //
            if (cchData > MAX_SLONGDATE)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Make sure one of 'd' or 'M' or 'y' exists in the date
            //  format string.  If it does not, then return an error.
            //
            pPos = wcspbrk(lpLCData, L"dMy");
            if (!pPos)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Set the registry with the new SLONGDATE string.
            //
            return (SetUserInfo( LOCALE_SLONGDATE,
                                 (LPWSTR)lpLCData,
                                 cchData ));
            break;
        }
        case ( LOCALE_ICALENDARTYPE ) :
        {
            //
            //  Validate the new value.  It should be no longer than
            //  MAX_ICALTYPE wide characters in length.
            //
            //  NOTE: The string may not be NULL, so it must be at least
            //        2 chars long (includes null).
            //
            if ((cchData < 2) || (cchData > MAX_ICALTYPE) ||
                (!IsValidCalendarTypeStr(pHashN, lpLCData)))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Set the registry with the new ICALENDARTYPE string.
            //
            return (SetUserInfo( LOCALE_ICALENDARTYPE,
                                 (LPWSTR)lpLCData,
                                 cchData ));
            break;
        }
        case ( LOCALE_IFIRSTDAYOFWEEK ) :
        {
            //
            //  Validate the new value.  It should be no longer than
            //  MAX_IFIRSTDAY wide characters in length.
            //  The value should be between 0 and MAX_VALUE_IFIRSTDAY.
            //
            //  NOTE: The string may not be NULL, so it must be at least
            //        2 chars long (includes null).
            //
            //        Optimized - since MAX_IFIRSTDAY is 2.
            //
            if ((cchData != MAX_IFIRSTDAY) ||
                (*lpLCData < NLS_CHAR_ZERO) ||
                (*lpLCData > MAX_CHAR_IFIRSTDAY))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Set the registry with the new IFIRSTDAYOFWEEK string.
            //
            return (SetUserInfo( LOCALE_IFIRSTDAYOFWEEK,
                                 (LPWSTR)lpLCData,
                                 cchData ));
            break;
        }
        case ( LOCALE_IFIRSTWEEKOFYEAR ) :
        {
            //
            //  Validate the new value.  It should be no longer than
            //  MAX_IFIRSTWEEK wide characters in length.
            //  The value should be between 0 and MAX_VALUE_IFIRSTWEEK.
            //
            //  NOTE: The string may not be NULL, so it must be at least
            //        2 chars long (includes null).
            //
            //        Optimized - since MAX_IFIRSTWEEK is 2.
            //
            if ((cchData != MAX_IFIRSTWEEK) ||
                (*lpLCData < NLS_CHAR_ZERO) ||
                (*lpLCData > MAX_CHAR_IFIRSTWEEK))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Set the registry with the new IFIRSTWEEKOFYEAR string.
            //
            return (SetUserInfo( LOCALE_IFIRSTWEEKOFYEAR,
                                 (LPWSTR)lpLCData,
                                 cchData ));
            break;
        }
        default :
        {
            SetLastError(ERROR_INVALID_FLAGS);
            return (FALSE);
        }
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetCalendarInfoW
//
//  Returns one of the various pieces of information about a particular
//  calendar by querying the configuration registry.  This call also
//  indicates how much memory is necessary to contain the desired
//  information.
//
//  12-17-97    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WINAPI GetCalendarInfoW(
    LCID Locale,
    CALID Calendar,
    CALTYPE CalType,
    LPWSTR lpCalData,
    int cchData,
    LPDWORD lpValue)
{
    PLOC_HASH pHashN;                       // ptr to LOC hash node
    int Length = 0;                         // length of info string
    LPWSTR pString;                         // ptr to the info string
    BOOL UserOverride = TRUE;               // use user override
    BOOL ReturnNum = FALSE;                 // return number instead of string
    WCHAR pTemp[MAX_REG_VAL_SIZE];          // temp buffer
    UNICODE_STRING ObUnicodeStr;            // value string
    int Base = 0;                           // base for str to int conversion
    LPWSTR pOptCal;                         // ptr to optional calendar values
    PCAL_INFO pCalInfo;                     // ptr to calendar info


    //
    //  Invalid Parameter Check:
    //    - validate LCID
    //    - count is negative
    //    - NULL data pointer AND count is not zero
    //
    VALIDATE_LOCALE(Locale, pHashN, FALSE);
    if ((pHashN == NULL) ||
        (cchData < 0) ||
        ((lpCalData == NULL) && (cchData != 0)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (0);
    }

    //
    //  Need to check the parameters based on the CAL_RETURN_NUMBER
    //  CalType.
    //
    if (CalType & CAL_RETURN_NUMBER)
    {
        if ((lpCalData != NULL) || (cchData != 0) || (lpValue == NULL))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return (0);
        }
    }
    else
    {
        if ((lpValue != NULL) ||
            (cchData < 0) ||
            ((lpCalData == NULL) && (cchData != 0)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return (0);
        }
    }

    //
    //  Check for NO USER OVERRIDE flag and remove the USE CP ACP flag.
    //
    if (CalType & CAL_NOUSEROVERRIDE)
    {
        //
        //  Flag is set, so set the boolean value and remove the flag
        //  from the CalType parameter (for switch statement).
        //
        UserOverride = FALSE;
    }
    if (CalType & CAL_RETURN_NUMBER)
    {
        //
        //  Flag is set, so set the boolean value and remove the flag
        //  from the CalType parameter (for switch statement).
        //
        ReturnNum = TRUE;
    }
    CalType &= (~(CAL_NOUSEROVERRIDE | CAL_USE_CP_ACP | CAL_RETURN_NUMBER));

    //
    //  Validate the Calendar parameter.
    //
    if (((CalType != CAL_ITWODIGITYEARMAX) &&
         ((pOptCal = IsValidCalendarType(pHashN, Calendar)) == NULL)) ||
        (GetCalendar(Calendar, &pCalInfo) != NO_ERROR))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (0);
    }

    //
    //  Return the appropriate information for the given CALTYPE.
    //  If user information exists for the given CALTYPE, then
    //  the user default is returned instead of the system default.
    //
    switch (CalType)
    {
        case ( CAL_ICALINTVALUE ) :
        {
            Base = 10;

            //
            //  Get the integer value for the given calendar.
            //
            pString = ((POPT_CAL)pOptCal)->pCalStr;

            break;
        }
        case ( CAL_SCALNAME ) :
        {
            //
            //  Get the calendar name for the given calendar.
            //
            pString = ((POPT_CAL)pOptCal)->pCalStr +
                      NlsStrLenW(((POPT_CAL)pOptCal)->pCalStr) + 1;

            break;
        }
        case ( CAL_ITWODIGITYEARMAX ) :
        {
            Base = 10;

            //
            // Check if a policy is enforced for the current user,
            // and if so, let's use it.
            //
            if (GetTwoDigitYearInfo(Calendar, pTemp, NLS_POLICY_TWO_DIGIT_YEAR_KEY))
            {
                pString = pTemp;
            }
            else
            {
                if (UserOverride &&
                    GetTwoDigitYearInfo(Calendar, pTemp, NLS_TWO_DIGIT_YEAR_KEY))
                {
                    pString = pTemp;
                }
                else
                {
                    //
                    //  Use the default.
                    //
                    pString = (LPWORD)pCalInfo +
                              (((PCALENDAR_VAR)pCalInfo)->STwoDigitYearMax);
                }
            }

            break;
        }
        case ( CAL_IYEAROFFSETRANGE ) :
        {
            Base = 10;

            //
            //  Get the pointer to the appropriate calendar string.
            //
            pString = (LPWORD)pCalInfo +
                      (((PCALENDAR_VAR)pCalInfo)->SEraRanges);

            //
            //  Make sure the string is NOT empty.
            //
            if (*pString)
            {
                pString = ((PERA_RANGE)pString)->pYearStr;
            }
            else
            {
                pString = L"0";
            }

            break;
        }
        case ( CAL_SERASTRING ) :
        {
            //
            //  Get the pointer to the appropriate calendar string.
            //
            pString = (LPWORD)pCalInfo +
                      (((PCALENDAR_VAR)pCalInfo)->SEraRanges);

            //
            //  Make sure the string is NOT empty.  If it is, return the
            //  empty string.
            //
            if (*pString)
            {
                pString = ((PERA_RANGE)pString)->pYearStr +
                          NlsStrLenW(((PERA_RANGE)pString)->pYearStr) + 1;
            }

            break;
        }
        case ( CAL_SSHORTDATE ) :
        {
            //
            //  Get the pointer to the appropriate calendar string.
            //
            pString = (LPWORD)pCalInfo +
                      (((PCALENDAR_VAR)pCalInfo)->SShortDate);

            //
            //  Make sure the string is NOT empty.  If it is, use the
            //  locale's short date string.
            //
            if (*pString == 0)
            {
                pString = (LPWORD)(pHashN->pLocaleHdr) +
                          pHashN->pLocaleHdr->SShortDate;
            }

            break;
        }
        case ( CAL_SLONGDATE ) :
        {
            //
            //  Get the pointer to the appropriate calendar string.
            //
            pString = (LPWORD)pCalInfo +
                      (((PCALENDAR_VAR)pCalInfo)->SLongDate);

            //
            //  Make sure the string is NOT empty.  If it is, use the
            //  locale's long date string.
            //
            if (*pString == 0)
            {
                pString = (LPWORD)(pHashN->pLocaleHdr) +
                          pHashN->pLocaleHdr->SLongDate;
            }

            break;
        }
        case ( CAL_SYEARMONTH ) :
        {
            //
            //  Get the pointer to the appropriate calendar string.
            //
            pString = (LPWORD)pCalInfo +
                      (((PCALENDAR_VAR)pCalInfo)->SYearMonth);

            //
            //  Make sure the string is NOT empty.  If it is, use the
            //  locale's year month string.
            //
            if (*pString == 0)
            {
                pString = (LPWORD)(pHashN->pLocaleHdr) +
                          pHashN->pLocaleHdr->SYearMonth;
            }

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
            //
            //  Get the pointer to the appropriate calendar string if the
            //  IfNames flag is set for the calendar.
            //
            pString = NULL;
            if (((PCALENDAR_VAR)pCalInfo)->IfNames)
            {
                pString = (LPWORD)pCalInfo +
                          *((LPWORD)((LPBYTE)(pCalInfo) +
                                     (FIELD_OFFSET(CALENDAR_VAR, SDayName1) +
                                      ((CalType - CAL_SDAYNAME1) * sizeof(WORD)))));
            }

            //
            //  Make sure the string is NOT empty.  If it is, use the
            //  locale's string.
            //
            if ((pString == NULL) || (*pString == 0))
            {
                pString = (LPWORD)(pHashN->pLocaleHdr) +
                          *((LPWORD)((LPBYTE)(pHashN->pLocaleHdr) +
                                     (FIELD_OFFSET(LOCALE_VAR, SDayName1) +
                                      ((CalType - CAL_SDAYNAME1) * sizeof(WORD)))));
            }

            break;
        }
        default :
        {
            SetLastError(ERROR_INVALID_FLAGS);
            return (0);
        }
    }

    //
    //  See if the caller wants the value in the form of a number instead
    //  of a string.
    //
    if (ReturnNum)
    {
        //
        //  Make sure the flags are valid and that the DWORD buffer
        //  is not NULL.
        //
        if (Base == 0)
        {
            SetLastError(ERROR_INVALID_FLAGS);
            return (0);
        }

        if (lpValue == NULL)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return (0);
        }

        //
        //  Convert the string to an int and return 2 (1 DWORD = 2 WORDS).
        //
        RtlInitUnicodeString(&ObUnicodeStr, pString);
        if (RtlUnicodeStringToInteger(&ObUnicodeStr, Base, lpValue))
        {
            SetLastError(ERROR_INVALID_FLAGS);
            return (0);
        }
        return (2);
    }

    //
    //  Get the length (in characters) of the string to copy.
    //
    if (Length == 0)
    {
        Length = NlsStrLenW(pString);
    }

    //
    //  Add one for the null termination.  All strings should be null
    //  terminated.
    //
    Length++;

    //
    //  Check cchData for size of given buffer.
    //
    if (cchData == 0)
    {
        //
        //  If cchData is 0, then we can't use lpCalData.  In this
        //  case, we simply want to return the length (in characters) of
        //  the string to be copied.
        //
        return (Length);
    }
    else if (cchData < Length)
    {
        //
        //  The buffer is too small for the string, so return an error
        //  and zero bytes written.
        //
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return (0);
    }

    //
    //  Copy the string to lpCalData and null terminate it.
    //  Return the number of characters copied.
    //
    wcsncpy(lpCalData, pString, Length - 1);
    lpCalData[Length - 1] = 0;
    return (Length);
}


////////////////////////////////////////////////////////////////////////////
//
//  SetCalendarInfoW
//
//  Sets one of the various pieces of information about a particular
//  calendar by making an entry in the user's portion of the configuration
//  registry.  This will only affect the user override portion of the
//  calendar settings.  The system defaults will never be reset.
//
//  12-17-97    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI SetCalendarInfoW(
    LCID Locale,
    CALID Calendar,
    CALTYPE CalType,
    LPCWSTR lpCalData)
{
    PLOC_HASH pHashN;                       // ptr to LOC hash node
    int cchData;                            // length of lpLCData
    PCAL_INFO pCalInfo;                     // ptr to calendar info
    UNICODE_STRING ObUnicodeStr;            // value string
    DWORD Value;                            // value


    //
    //  Invalid Parameter Check:
    //    - validate LCID
    //    - NULL data pointer
    //
    //  NOTE: invalid type is checked in the switch statement below.
    //
    VALIDATE_LOCALE(Locale, pHashN, FALSE);
    if ((pHashN == NULL) || (lpCalData == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    //
    //  Get the length of the buffer.
    //
    cchData = NlsStrLenW(lpCalData) + 1;

    //
    //  Validate the Calendar parameter.
    //
    if (GetCalendar(Calendar, &pCalInfo) != NO_ERROR)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    //
    //  Set the appropriate user information for the given CALTYPE.
    //
    CalType &= (~CAL_USE_CP_ACP);
    switch (CalType)
    {
        case ( CAL_ITWODIGITYEARMAX ) :
        {
            //
            //  Get the default value to make sure the calendar is
            //  allowed to be set.  Things like the Japanese Era calendar
            //  may not be set.
            //
            RtlInitUnicodeString( &ObUnicodeStr,
                                  ((LPWORD)pCalInfo +
                                   (((PCALENDAR_VAR)pCalInfo)->STwoDigitYearMax)) );
            RtlUnicodeStringToInteger(&ObUnicodeStr, 10, &Value);
            if (Value <= 99)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Validate the new value.  It should be no longer than
            //  MAX_ITWODIGITYEAR wide characters in length.
            //  It should be between 99 and 9999.
            //
            if ((cchData > MAX_ITWODIGITYEAR) ||
                (cchData < 3) ||
                ((cchData == 3) &&
                 ((*lpCalData != NLS_CHAR_NINE) ||
                  (*(lpCalData + 1) != NLS_CHAR_NINE))))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }

            //
            //  Set the registry with the new TwoDigitYearMax string.
            //
            return (SetTwoDigitYearInfo(Calendar, lpCalData, cchData));
            break;
        }
        default :
        {
            SetLastError(ERROR_INVALID_FLAGS);
            return (FALSE);
        }
    }

    //
    //  Return success.
    //
    return (TRUE);
}




//-------------------------------------------------------------------------//
//                           INTERNAL ROUTINES                             //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  GetLocalizedLanguageName
//
//  Returns the localized version of the language name for the given
//  language id.  It gets the information from the resource file in the
//  language that the current user is using.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetLocalizedLanguageName(
    LANGID Language,
    LPWSTR *ppLangName)
{
    //
    //  Make sure we have a language id that is either 0x0000 or greater
    //  than 0x0400.
    //
    if ((Language == 0) || (Language >= LANG_USER_DEFAULT))
    {
        //
        //  Get the language name from the string table.
        //
        return (GetNameFromStringTable(Language, ppLangName));
    }

    //
    //  If we get here, there is an invalid language id, so return 0.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetLocalizedCountryName
//
//  Returns the localized version of the country name for the given
//  language id.  It gets the information from the resource file in the
//  language that the current user is using.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL GetLocalizedCountryName(
    LANGID Language,
    LPWSTR *ppCtryName)
{
    HANDLE hFindRes;                   // handle from find resource
    HANDLE hLoadRes;                   // handle from load resource
    LANGID LangId;                     // language id


    //
    //  Find the resource.
    //
    LangId = LANGIDFROMLCID(GetUserDefaultLCID());
    if ((!(hFindRes = FindResourceExW( hModule,
                                       RT_RCDATA,
                                       MAKEINTRESOURCEW(Language),
                                       (WORD)LangId ))))
    {
        //
        //   Could not find resource.  Try NEUTRAL language id.
        //
        if ((!(hFindRes = FindResourceExW( hModule,
                                           RT_RCDATA,
                                           MAKEINTRESOURCEW(Language),
                                           (WORD)0 ))))
        {
            //
            //  Could not find resource.  Return failure.
            //
            return (FALSE);
        }
    }

    //
    //  Load the resource.
    //
    if (hLoadRes = LoadResource(hModule, hFindRes))
    {
        //
        //  Lock the resource.  Store the found pointer in the given
        //  country name pointer.
        //
        if (*ppCtryName = (LPWSTR)LockResource(hLoadRes))
        {
            return (TRUE);
        }
    }

    //
    //  Return failure.
    //
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetLocalizedSortName
//
//  Returns the localized version of the sort name for the given locale
//  id.  It gets the information from the resource file in the language
//  that the current user is using.
//
//  11-15-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL GetLocalizedSortName(
    LCID Locale,
    LPWSTR *ppSortName)
{
    HANDLE hFindRes;                   // handle from find resource
    HANDLE hLoadRes;                   // handle from load resource
    LANGID LangId, Lang;               // language id
    WCHAR pName[MAX_PATH];             // ptr to resource name string
    LPWSTR pTemp;                      // temp prt to resource name string
    UINT Offset;


    //
    //  Get the resource name string.
    //
    NlsStrCpyW(pName, NLS_SORT_RES_PREFIX);
    Offset = NlsStrLenW(pName);
    pTemp = pName + Offset;
    if (NlsConvertIntegerToString(Locale, 16, 8, pTemp, MAX_PATH - Offset))
    {
        return (FALSE);
    }

    //
    //  Find the resource.
    //
    LangId = LANGIDFROMLCID(GetUserDefaultLCID());
    if ((!(hFindRes = FindResourceExW( hModule,
                                       RT_RCDATA,
                                       pName,
                                       (WORD)LangId ))))
    {
        //
        //  Could not find resource.  Try NEUTRAL language id.
        //
        if ((!(hFindRes = FindResourceExW( hModule,
                                           RT_RCDATA,
                                           pName,
                                           (WORD)0 ))))
        {
            //
            //  Try the neutral sublanguage in the resource name.
            //
            Lang = LANGIDFROMLCID(Locale);
            Lang = MAKELANGID(PRIMARYLANGID(Lang), 0);
            Locale = MAKELCID(Lang, SORTIDFROMLCID(Locale));
            if (NlsConvertIntegerToString(Locale, 16, 8, pTemp, MAX_PATH - Offset))
            {
                return (FALSE);
            }

            if ((!(hFindRes = FindResourceExW( hModule,
                                               RT_RCDATA,
                                               pName,
                                               (WORD)LangId ))))
            {
                //
                //  Could not find resource.  Try NEUTRAL language id.
                //
                if ((!(hFindRes = FindResourceExW( hModule,
                                                   RT_RCDATA,
                                                   pName,
                                                   (WORD)0 ))))
                {
                    //
                    //  Could not find resource.  Try getting the Default
                    //  Alternate Sort string.
                    //
                    if ((!(hFindRes = FindResourceExW( hModule,
                                                       RT_RCDATA,
                                                       NLS_SORT_RES_DEFAULT,
                                                       (WORD)LangId ))))
                    {
                        //
                        //  Could not find resource.  Try NEUTRAL language id.
                        //
                        if ((!(hFindRes = FindResourceExW( hModule,
                                                       RT_RCDATA,
                                                       NLS_SORT_RES_DEFAULT,
                                                       (WORD)0 ))))
                        {
                            //
                            //  Could not find resource.  Return failure.
                            //
                            return (FALSE);
                        }
                    }
                }
            }
        }
    }

    //
    //  Load the resource.
    //
    if (hLoadRes = LoadResource(hModule, hFindRes))
    {
        //
        //  Lock the resource.  Store the found pointer in the given
        //  sort name pointer.
        //
        if (*ppSortName = (LPWSTR)LockResource(hLoadRes))
        {
            return (TRUE);
        }
    }

    //
    //  Return failure.
    //
    return (FALSE);
}



////////////////////////////////////////////////////////////////////////////
//
//  SetUserInfo
//
//  This routine sets the given value in the registry with the given data.
//  All values must be of the type REG_SZ.
//
//  NOTE: The handle to the registry key must be closed by the CALLER if
//        the return value is NO_ERROR.
//
//  07-14-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL SetUserInfo(
    LCTYPE   LCType,
    LPWSTR pData,
    ULONG DataLength)
{

    NTSTATUS Status;

    //
    //  Get the length of the value string.
    //
    DataLength *= sizeof(WCHAR);

    //
    // If there is no logged on user or the current security context
    // isn't the logged-on interactive user, then set the registry
    // value directly.
    //
    if ((! pNlsUserInfo->fCacheValid ) ||
        (! NT_SUCCESS( NlsCheckForInteractiveUser() )))
    {
        return (SetCurrentUserRegValue(LCType, pData, DataLength));
    }

    Status = CsrBasepNlsSetUserInfo(LCType,
                                    pData,
                                    DataLength);

    //
    //  Check to see if the "set" operation succeeded.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        //  We got a failure.  Try using just the registry apis to set the
        //  registry.  It's possible that the cache is not valid yet if this
        //  is called from setup or winlogon.
        //
        return (SetCurrentUserRegValue(LCType, pData, DataLength));
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  SetCurrentUserRegValue
//
//  Set the registry value for the current security context. This routine
//  is called when the current security context is different from the logged
//  on user.
//
//  12-26-98    SamerA    Created.
////////////////////////////////////////////////////////////////////////////

BOOL SetCurrentUserRegValue(
    LCTYPE   LCType,
    LPWSTR pData,
    ULONG DataLength)
{
    HANDLE hKey = NULL;
    LPWSTR pValue;
    LPWSTR pCache;

    if (0 == ValidateLCType(pNlsUserInfo, LCType, &pValue, &pCache))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Open the registry for the current security context
    //
    OPEN_CPANEL_INTL_KEY(hKey, FALSE, KEY_READ | KEY_WRITE);
    if (SetRegValue(hKey, pValue, pData, DataLength) != NO_ERROR)
    {
        CLOSE_REG_KEY(hKey);
        SetLastError(ERROR_INVALID_ACCESS);
        return (FALSE);
    }

    CLOSE_REG_KEY(hKey);

    //
    // Flush the process cache entry, if needed.
    //
    NlsFlushProcessCache(LCType);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  SetMultipleUserInfoInRegistry
//
//  This routine sets the given multiple values in the registry with the
//  given data.  All values must be of the type REG_SZ.
//
//  06-11-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL SetMultipleUserInfoInRegistry(
    DWORD dwFlags,
    int cchData,
    LPCWSTR pPicture,
    LPCWSTR pSeparator,
    LPCWSTR pOrder,
    LPCWSTR pTLZero,
    LPCWSTR pTimeMarkPosn)
{
    HANDLE hKey = NULL;
    ULONG rc = 0L;


    //
    //  Open the Control Panel International registry key.
    //
    OPEN_CPANEL_INTL_KEY(hKey, FALSE, KEY_READ | KEY_WRITE);

    //
    //  Save the appropriate values in the registry based on the flags.
    //
    switch (dwFlags)
    {
        case ( LOCALE_STIMEFORMAT ) :
        {
            rc = SetRegValue( hKey,
                              NLS_VALUE_STIMEFORMAT,
                              pPicture,
                              cchData * sizeof(WCHAR) );
            if (NT_SUCCESS(rc))
            {
                NlsFlushProcessCache(LOCALE_STIMEFORMAT);

                rc = SetRegValue( hKey,
                                  NLS_VALUE_STIME,
                                  pSeparator,
                                  (lstrlen(pSeparator) + 1) * sizeof(WCHAR) );
            }
            if (NT_SUCCESS(rc))
            {
                NlsFlushProcessCache(LOCALE_STIME);

                rc = SetRegValue( hKey,
                                  NLS_VALUE_ITIME,
                                  pOrder,
                                  (lstrlen(pOrder) + 1) * sizeof(WCHAR) );
            }
            if (NT_SUCCESS(rc))
            {
                NlsFlushProcessCache(LOCALE_ITIME);

                rc = SetRegValue( hKey,
                                  NLS_VALUE_ITLZERO,
                                  pTLZero,
                                  (lstrlen(pTLZero) + 1) * sizeof(WCHAR) );
            }
            if (NT_SUCCESS(rc))
            {
                NlsFlushProcessCache(LOCALE_ITLZERO);

                rc = SetRegValue( hKey,
                                  NLS_VALUE_ITIMEMARKPOSN,
                                  pTimeMarkPosn,
                                  (lstrlen(pTimeMarkPosn) + 1) * sizeof(WCHAR) );

                if (NT_SUCCESS(rc))
                {
                    NlsFlushProcessCache(LOCALE_ITIMEMARKPOSN);
                }
            }

            break;
        }
        case ( LOCALE_STIME ) :
        {
            rc = SetRegValue( hKey,
                              NLS_VALUE_STIME,
                              pSeparator,
                              cchData * sizeof(WCHAR) );
            if (NT_SUCCESS(rc))
            {
                NlsFlushProcessCache(LOCALE_STIME);

                rc = SetRegValue( hKey,
                                  NLS_VALUE_STIMEFORMAT,
                                  pPicture,
                                  (lstrlen(pPicture) + 1) * sizeof(WCHAR) );

                if (NT_SUCCESS(rc))
                {
                    NlsFlushProcessCache(LOCALE_STIMEFORMAT);
                }
            }

            break;
        }
        case ( LOCALE_ITIME ) :
        {
            rc = SetRegValue( hKey,
                              NLS_VALUE_ITIME,
                              pOrder,
                              cchData * sizeof(WCHAR) );
            if (NT_SUCCESS(rc))
            {
                NlsFlushProcessCache(LOCALE_ITIME);

                rc = SetRegValue( hKey,
                                  NLS_VALUE_STIMEFORMAT,
                                  pPicture,
                                  (lstrlen(pPicture) + 1) * sizeof(WCHAR) );

                if (NT_SUCCESS(rc))
                {
                    NlsFlushProcessCache(LOCALE_STIMEFORMAT);
                }
            }

            break;
        }
        case ( LOCALE_SSHORTDATE ) :
        {
            rc = SetRegValue( hKey,
                              NLS_VALUE_SSHORTDATE,
                              pPicture,
                              cchData * sizeof(WCHAR) );
            if (NT_SUCCESS(rc))
            {
                NlsFlushProcessCache(LOCALE_SSHORTDATE);

                rc = SetRegValue( hKey,
                                  NLS_VALUE_SDATE,
                                  pSeparator,
                                  (lstrlen(pSeparator) + 1) * sizeof(WCHAR) );
            }
            if (NT_SUCCESS(rc))
            {
                NlsFlushProcessCache(LOCALE_SDATE);

                rc = SetRegValue( hKey,
                                  NLS_VALUE_IDATE,
                                  pOrder,
                                  (lstrlen(pOrder) + 1) * sizeof(WCHAR) );

                if (NT_SUCCESS(rc))
                {
                    NlsFlushProcessCache(LOCALE_IDATE);
                }
            }

            break;
        }
        case ( LOCALE_SDATE ) :
        {
            rc = SetRegValue( hKey,
                              NLS_VALUE_SDATE,
                              pSeparator,
                              cchData * sizeof(WCHAR) );
            if (NT_SUCCESS(rc))
            {
                NlsFlushProcessCache(LOCALE_SDATE);

                rc = SetRegValue( hKey,
                                  NLS_VALUE_SSHORTDATE,
                                  pPicture,
                                  (lstrlen(pPicture) + 1) * sizeof(WCHAR) );

                if (NT_SUCCESS(rc))
                {
                    NlsFlushProcessCache(LOCALE_SSHORTDATE);
                }
            }

            break;
        }
        default :
        {
            CLOSE_REG_KEY(hKey);
            return (FALSE);
        }
    }

    //
    //  Close the registry key.
    //
    CLOSE_REG_KEY(hKey);

    //
    //  Return the result.
    //
    return (rc == NO_ERROR);
}


////////////////////////////////////////////////////////////////////////////
//
//  SetMultipleUserInfo
//
//  This routine calls the server to set multiple registry values.  This way,
//  only one client/server transition is necessary.
//
//  08-19-94    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL SetMultipleUserInfo(
    DWORD dwFlags,
    int cchData,
    LPCWSTR pPicture,
    LPCWSTR pSeparator,
    LPCWSTR pOrder,
    LPCWSTR pTLZero,
    LPCWSTR pTimeMarkPosn)
{
    NTSTATUS Status;

    //
    // If there is no logged on user or the current security context
    // isn't the logged-on interactive user, then set the registry
    // value directly.
    //
    if ((! pNlsUserInfo->fCacheValid ) ||
        (! NT_SUCCESS( NlsCheckForInteractiveUser() )))
    {
        if (SetMultipleUserInfoInRegistry( dwFlags,
                                           cchData,
                                           pPicture,
                                           pSeparator,
                                           pOrder,
                                           pTLZero,
                                           pTimeMarkPosn ) == FALSE)
        {
            SetLastError(ERROR_INVALID_ACCESS);
            return (FALSE);
        }

        return (TRUE);
    }

    Status = CsrBasepNlsSetMultipleUserInfo(dwFlags,
                                            cchData,
                                            pPicture,
                                            pSeparator,
                                            pOrder,
                                            pTLZero,
                                            pTimeMarkPosn
                                            );
    //
    //  Check to see if the "set" operation succeeded.
    //

    if (!NT_SUCCESS(Status))
    {
        //
        //  We got a failure.  Try using just the registry apis to set the
        //  registry.  It's possible that the cache is not valid yet if this
        //  is called from setup or winlogon.
        //
        if (SetMultipleUserInfoInRegistry( dwFlags,
                                           cchData,
                                           pPicture,
                                           pSeparator,
                                           pOrder,
                                           pTLZero,
                                           pTimeMarkPosn ) == FALSE)
        {
            SetLastError(ERROR_INVALID_ACCESS);
            return (FALSE);
        }
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetTwoDigitYearInfo
//
//  This routine gets the two digit year info from the registry.
//
//  12-17-97    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL GetTwoDigitYearInfo(
    CALID Calendar,
    LPWSTR pYearInfo,
    PWSTR pwszKeyPath)
{
    HANDLE hKey = NULL;                          // handle to key
    WCHAR pCalStr[MAX_PATH];                     // ptr to calendar id string
    PKEY_VALUE_FULL_INFORMATION pKeyValueFull;   // ptr to query info
    BYTE pStatic[MAX_KEY_VALUE_FULLINFO];        // ptr to static buffer
    BOOL IfAlloc = FALSE;                        // if buffer was allocated
    ULONG rc = 0L;                               // return code
    BOOL bResult = FALSE;                        // result
    UNICODE_STRING ObUnicodeStr;                 // year string
    DWORD Year;                                  // year value


    //
    //  Open the Control Panel International registry key.
    //
    if (OpenRegKey( &hKey,
                    NULL,
                    pwszKeyPath,
                    KEY_READ ) != NO_ERROR)
    {
        return (FALSE);
    }

    //
    //  Convert calendar value to Unicode string.
    //
    if (NlsConvertIntegerToString(Calendar, 10, 0, pCalStr, MAX_PATH))
    {
        NtClose(hKey);
        return (FALSE);
    }

    //
    //  Query the registry for the TwoDigitYearMax value.
    //
    pKeyValueFull = (PKEY_VALUE_FULL_INFORMATION)pStatic;
    rc = QueryRegValue( hKey,
                        pCalStr,
                        &pKeyValueFull,
                        MAX_KEY_VALUE_FULLINFO,
                        &IfAlloc );

    //
    //  Close the registry key.
    //
    NtClose(hKey);

    //
    //  See if the TwoDigitYearMax value is present.
    //
    if (rc != NO_ERROR)
    {
        return (FALSE);
    }

    //
    //  See if the TwoDigitYearMax data is present.
    //
    if (pKeyValueFull->DataLength > 2)
    {
        //
        //  Copy the info.
        //
        NlsStrCpyW(pYearInfo, GET_VALUE_DATA_PTR(pKeyValueFull));

        //
        //  Make sure the value is between 99 and 9999.
        //
        RtlInitUnicodeString(&ObUnicodeStr, pYearInfo);
        if ((RtlUnicodeStringToInteger(&ObUnicodeStr, 10, &Year) == NO_ERROR) &&
            (Year >= 99) && (Year <= 9999))
        {
            bResult = TRUE;
        }
    }

    //
    //  Free the buffer used for the query.
    //
    if (IfAlloc)
    {
        NLS_FREE_MEM(pKeyValueFull);
    }

    //
    //  Return the result.
    //
    return (bResult);
}


////////////////////////////////////////////////////////////////////////////
//
//  SetTwoDigitYearInfo
//
//  This routine sets the two digit year info in the registry.
//
//  12-17-97    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL SetTwoDigitYearInfo(
    CALID Calendar,
    LPCWSTR pYearInfo,
    int cchData)
{
    HANDLE hKey = NULL;                          // handle to key
    WCHAR pCalStr[MAX_PATH];                     // ptr to calendar id string
    ULONG rc = 0L;                               // return code


    //
    //  Open the Control Panel International registry key.
    //  If it doesn't exist, then we have to create each subkey
    //  separately.
    //
    if (OpenRegKey( &hKey,
                    NULL,
                    NLS_TWO_DIGIT_YEAR_KEY,
                    KEY_READ | KEY_WRITE ) != NO_ERROR)
    {
        //
        //  Registry key does not exist, so create each subkey
        //  separately.
        //
        if (CreateRegKey( &hKey,
                          NULL,
                          NLS_CALENDARS_KEY,
                          KEY_READ | KEY_WRITE ) == NO_ERROR)
        {
            if (CreateRegKey( &hKey,
                              NULL,
                              NLS_TWO_DIGIT_YEAR_KEY,
                              KEY_READ | KEY_WRITE ) != NO_ERROR)
            {
                return (FALSE);
            }
        }
        else
        {
            return (FALSE);
        }
    }

    //
    //  Make sure all Gregorian calendars are set to the same value.
    //
    switch (Calendar)
    {
        case ( 1 ) :
        case ( 2 ) :
        case ( 9 ) :
        case ( 10 ) :
        case ( 11 ) :
        case ( 12 ) :
        {
            rc = SetRegValue(hKey, L"1", pYearInfo, (ULONG)cchData * sizeof(WCHAR));
            if (rc == NO_ERROR)
            {
                rc = SetRegValue(hKey, L"2", pYearInfo, (ULONG)cchData * sizeof(WCHAR));
            }
            if (rc == NO_ERROR)
            {
                rc = SetRegValue(hKey, L"9",  pYearInfo, (ULONG)cchData * sizeof(WCHAR));
            }
            if (rc == NO_ERROR)
            {
                rc = SetRegValue(hKey, L"10", pYearInfo, (ULONG)cchData * sizeof(WCHAR));
            }
            if (rc == NO_ERROR)
            {
                rc = SetRegValue(hKey, L"11", pYearInfo, (ULONG)cchData * sizeof(WCHAR));
            }
            if (rc == NO_ERROR)
            {
                rc = SetRegValue(hKey, L"12", pYearInfo, (ULONG)cchData * sizeof(WCHAR));
            }

            break;
        }
        default :
        {
            //
            //  Convert calendar value to Unicode string.
            //
            if (NlsConvertIntegerToString(Calendar, 10, 0, pCalStr, MAX_PATH))
            {
                NtClose(hKey);
                return (FALSE);
            }

            //
            //  Set the TwoDigitYearMax value in the registry.
            //
            rc = SetRegValue(hKey, pCalStr, pYearInfo, (ULONG)cchData * sizeof(WCHAR));

            break;
        }
    }

    //
    // Update the NlsCacheUpdateCount inside csrss
    //
    if (rc == NO_ERROR)
    {
        CsrBasepNlsUpdateCacheCount();
    }

    //
    //  Close the registry key.
    //
    NtClose(hKey);

    //
    //  Return the result.
    //
    return (rc == NO_ERROR);
}
