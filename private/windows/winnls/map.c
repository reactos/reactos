/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    map.c

Abstract:

    This file contains functions that deal with map tables.

    APIs found in this file:
      FoldStringW
      LCMapStringW

Revision History:

    05-31-91    JulieB    Created.

--*/



//
//  Include Files.
//

#include "nls.h"




//
//  Constant Declarations.
//

//
//  Invalid weight value.
//
#define MAP_INVALID_UW       0xffff

//
//  Number of bytes in each weight.
//
#define NUM_BYTES_UW         2
#define NUM_BYTES_DW         1
#define NUM_BYTES_CW         1
#define NUM_BYTES_XW         4
#define NUM_BYTES_SW         4

//
//  Flags to drop the 3rd weight (CW).
//
#define NORM_DROP_CW         (NORM_IGNORECASE | NORM_IGNOREWIDTH)


//
//  XW Values.
//
BYTE pXWDrop[] =                  // values to drop from XW
{
    0xc6,                         // weight 4
    0x03,                         // weight 5
    0xe4,                         // weight 6
    0xc5                          // weight 7
};
BYTE pXWSeparator[] =             // separator values for XW
{
    0xff,                         // weight 4
    0x02,                         // weight 5
    0xff,                         // weight 6
    0xff                          // weight 7
};




//
//  Forward Declarations.
//

int
FoldCZone(
    LPCWSTR pSrc,
    int cchSrc,
    LPWSTR pDest,
    int cchDest);

int
FoldDigits(
    LPCWSTR pSrc,
    int cchSrc,
    LPWSTR pDest,
    int cchDest);

int
FoldCZone_Digits(
    LPCWSTR pSrc,
    int cchSrc,
    LPWSTR pDest,
    int cchDest);

int FoldLigatures(
    LPCWSTR pSrc,
    int cchSrc,
    LPWSTR pDest,
    int cchDest);

int
FoldPreComposed(
    LPCWSTR pSrc,
    int cchSrc,
    LPWSTR pDest,
    int cchDest);

int
FoldComposite(
    LPCWSTR pSrc,
    int cchSrc,
    LPWSTR pDest,
    int cchDest);

int
MapCase(
    PLOC_HASH pHashN,
    LPCWSTR pSrc,
    int cchSrc,
    LPWSTR pDest,
    int cchDest,
    PCASE pCaseTbl);

int
MapSortKey(
    PLOC_HASH pHashN,
    DWORD dwFlags,
    LPCWSTR pSrc,
    int cchSrc,
    LPBYTE pDest,
    int cchDest,
    BOOL fModify);

int
MapNormalization(
    PLOC_HASH pHashN,
    DWORD dwFlags,
    LPCWSTR pSrc,
    int cchSrc,
    LPWSTR pDest,
    int cchDest);

int
MapKanaWidth(
    PLOC_HASH pHashN,
    DWORD dwFlags,
    LPCWSTR pSrc,
    int cchSrc,
    LPWSTR pDest,
    int cchDest);

int
MapHalfKana(
    LPCWSTR pSrc,
    int cchSrc,
    LPWSTR pDest,
    int cchDest,
    PKANA pKana,
    PCASE pCase);

int
MapFullKana(
    LPCWSTR pSrc,
    int cchSrc,
    LPWSTR pDest,
    int cchDest,
    PKANA pKana,
    PCASE pCase);

int
MapTraditionalSimplified(
    PLOC_HASH pHashN,
    DWORD dwFlags,
    LPCWSTR pSrc,
    int cchSrc,
    LPWSTR pDest,
    int cchDest,
    PCHINESE pChinese);





//-------------------------------------------------------------------------//
//                             API ROUTINES                                //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  FoldStringW
//
//  Maps one wide character string to another performing the specified
//  translation.  This mapping routine only takes flags that are locale
//  independent.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WINAPI FoldStringW(
    DWORD dwMapFlags,
    LPCWSTR lpSrcStr,
    int cchSrc,
    LPWSTR lpDestStr,
    int cchDest)
{
    int Count = 0;                // word count


    //
    //  Invalid Parameter Check:
    //     - length of src string is 0
    //     - either buffer size is negative (except cchSrc == -1)
    //     - src string is NULL
    //     - length of dest string is NOT zero AND dest string is NULL
    //     - same buffer - src = destination
    //
    //     - flags are checked in switch statement below
    //
    if ((cchSrc == 0) || (cchDest < 0) ||
        (lpSrcStr == NULL) ||
        ((cchDest != 0) && (lpDestStr == NULL)) ||
        (lpSrcStr == lpDestStr))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (0);
    }

    //
    //  If cchSrc is -1, then the source string is null terminated and we
    //  need to get the length of the source string.  Add one to the
    //  length to include the null termination.
    //  (This will always be at least 1.)
    //
    if (cchSrc <= -1)
    {
        cchSrc = NlsStrLenW(lpSrcStr) + 1;
    }

    //
    //  Map the string based on the given flags.
    //
    switch (dwMapFlags)
    {
        case ( MAP_FOLDCZONE ) :
        {
            //
            //  Map the string to fold the Compatibility Zone.
            //
            Count = FoldCZone( lpSrcStr,
                               cchSrc,
                               lpDestStr,
                               cchDest );
            break;
        }

        case ( MAP_FOLDDIGITS ) :
        {
            //
            //  Map the string to fold the Ascii Digits.
            //
            Count = FoldDigits( lpSrcStr,
                                cchSrc,
                                lpDestStr,
                                cchDest );
            break;
        }

        case ( MAP_EXPAND_LIGATURES ) :
        {
            //
            //  Map the string to expand all Ligatures.
            //
            Count = FoldLigatures( lpSrcStr,
                                   cchSrc,
                                   lpDestStr,
                                   cchDest );
            break;
        }

        case ( MAP_PRECOMPOSED ) :
        {
            //
            //  Map the string to compress all composite forms of
            //  characters to their precomposed form.
            //
            Count = FoldPreComposed( lpSrcStr,
                                     cchSrc,
                                     lpDestStr,
                                     cchDest );
            break;
        }

        case ( MAP_COMPOSITE ) :
        {
            //
            //  Map the string to expand out all precomposed characters
            //  to their composite form.
            //
            Count = FoldComposite( lpSrcStr,
                                   cchSrc,
                                   lpDestStr,
                                   cchDest );
            break;
        }

        case ( MAP_FOLDCZONE | MAP_FOLDDIGITS ) :
        {
            //
            //  Map the string to fold the Compatibility Zone and fold the
            //  Ascii Digits.
            //
            Count = FoldCZone_Digits( lpSrcStr,
                                      cchSrc,
                                      lpDestStr,
                                      cchDest );
            break;
        }

        case ( MAP_EXPAND_LIGATURES | MAP_FOLDCZONE ) :
        {
            //
            //  Map the string to expand the ligatures and fold the
            //  Compatibility Zone.
            //
            Count = FoldLigatures( lpSrcStr,
                                   cchSrc,
                                   lpDestStr,
                                   cchDest );
            Count = FoldCZone( lpDestStr,
                               Count,
                               lpDestStr,
                               cchDest );
            break;
        }

        case ( MAP_EXPAND_LIGATURES | MAP_FOLDDIGITS ) :
        {
            //
            //  Map the string to expand the ligatures and fold the
            //  Ascii Digits.
            //
            Count = FoldLigatures( lpSrcStr,
                                   cchSrc,
                                   lpDestStr,
                                   cchDest );
            Count = FoldDigits( lpDestStr,
                                Count,
                                lpDestStr,
                                cchDest );
            break;
        }

        case ( MAP_EXPAND_LIGATURES | MAP_FOLDCZONE | MAP_FOLDDIGITS ) :
        {
            //
            //  Map the string to expand the ligatures, fold the
            //  Compatibility Zone and fold the Ascii Digits.
            //
            Count = FoldLigatures( lpSrcStr,
                                   cchSrc,
                                   lpDestStr,
                                   cchDest );
            Count = FoldCZone_Digits( lpDestStr,
                                      Count,
                                      lpDestStr,
                                      cchDest );
            break;
        }

        case ( MAP_PRECOMPOSED | MAP_FOLDCZONE ) :
        {
            //
            //  Map the string to convert to precomposed forms and to
            //  fold the Compatibility Zone.
            //
            Count = FoldPreComposed( lpSrcStr,
                                     cchSrc,
                                     lpDestStr,
                                     cchDest );
            Count = FoldCZone( lpDestStr,
                               Count,
                               lpDestStr,
                               cchDest );
            break;
        }

        case ( MAP_PRECOMPOSED | MAP_FOLDDIGITS ) :
        {
            //
            //  Map the string to convert to precomposed forms and to
            //  fold the Ascii Digits.
            //
            Count = FoldPreComposed( lpSrcStr,
                                     cchSrc,
                                     lpDestStr,
                                     cchDest );
            Count = FoldDigits( lpDestStr,
                                Count,
                                lpDestStr,
                                cchDest );
            break;
        }

        case ( MAP_PRECOMPOSED | MAP_FOLDCZONE | MAP_FOLDDIGITS ) :
        {
            //
            //  Map the string to convert to precomposed forms,
            //  fold the Compatibility Zone, and fold the Ascii Digits.
            //
            Count = FoldPreComposed( lpSrcStr,
                                     cchSrc,
                                     lpDestStr,
                                     cchDest );
            Count = FoldCZone_Digits( lpDestStr,
                                      Count,
                                      lpDestStr,
                                      cchDest );
            break;
        }

        case ( MAP_COMPOSITE | MAP_FOLDCZONE ) :
        {
            //
            //  Map the string to convert to composite forms and to
            //  fold the Compatibility Zone.
            //
            Count = FoldComposite( lpSrcStr,
                                   cchSrc,
                                   lpDestStr,
                                   cchDest );
            Count = FoldCZone( lpDestStr,
                               Count,
                               lpDestStr,
                               cchDest );
            break;
        }

        case ( MAP_COMPOSITE | MAP_FOLDDIGITS ) :
        {
            //
            //  Map the string to convert to composite forms and to
            //  fold the Ascii Digits.
            //
            Count = FoldComposite( lpSrcStr,
                                   cchSrc,
                                   lpDestStr,
                                   cchDest );
            Count = FoldDigits( lpDestStr,
                                Count,
                                lpDestStr,
                                cchDest );
            break;
        }

        case ( MAP_COMPOSITE | MAP_FOLDCZONE | MAP_FOLDDIGITS ) :
        {
            //
            //  Map the string to convert to composite forms,
            //  fold the Compatibility Zone, and fold the Ascii Digits.
            //
            Count = FoldComposite( lpSrcStr,
                                   cchSrc,
                                   lpDestStr,
                                   cchDest );
            Count = FoldCZone_Digits( lpDestStr,
                                      Count,
                                      lpDestStr,
                                      cchDest );
            break;
        }

        default :
        {
            SetLastError(ERROR_INVALID_FLAGS);
            return (0);
        }
    }

    //
    //  Return the number of characters written to the buffer.
    //  Or, if cchDest == 0, then return the number of characters
    //  that would have been written to the buffer.
    //
    return (Count);
}


////////////////////////////////////////////////////////////////////////////
//
//  LCMapStringW
//
//  Maps one wide character string to another performing the specified
//  translation.  This mapping routine only takes flags that are locale
//  dependent.
//
//  05-31-91    JulieB    Created.
//  07-26-93    JulieB    Added new flags for NT-J.
////////////////////////////////////////////////////////////////////////////

int WINAPI LCMapStringW(
    LCID Locale,
    DWORD dwMapFlags,
    LPCWSTR lpSrcStr,
    int cchSrc,
    LPWSTR lpDestStr,
    int cchDest)
{
    PLOC_HASH pHashN;             // ptr to LOC hash node
    int Count = 0;                // word count or byte count
    int ctr;                      // loop counter


    //
    //  Invalid Parameter Check:
    //     - validate LCID
    //     - length of src string is 0
    //     - destination buffer size is negative
    //     - src string is NULL
    //     - length of dest string is NOT zero AND dest string is NULL
    //     - same buffer - src = destination
    //              if not UPPER or LOWER or
    //              UPPER or LOWER used with Japanese flags
    //
    VALIDATE_LANGUAGE(Locale, pHashN, dwMapFlags & LCMAP_LINGUISTIC_CASING, TRUE);
    if ( (pHashN == NULL) ||
         (cchSrc == 0) || (cchDest < 0) || (lpSrcStr == NULL) ||
         ((cchDest != 0) && (lpDestStr == NULL)) ||
         ((lpSrcStr == lpDestStr) &&
          ((!(dwMapFlags & (LCMAP_UPPERCASE | LCMAP_LOWERCASE))) ||
           (dwMapFlags & (LCMAP_HIRAGANA | LCMAP_KATAKANA |
                          LCMAP_HALFWIDTH | LCMAP_FULLWIDTH)))) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (0);
    }

    //
    //  Invalid Flags Check:
    //     - flags other than valid ones or 0
    //     - (any NORM_ flag) AND (any LCMAP_ flag except byterev and sortkey)
    //     - (NORM_ flags for sortkey) AND (NOT LCMAP_SORTKEY)
    //     - more than one of lower, upper, sortkey
    //     - more than one of hiragana, katakana, sortkey
    //     - more than one of half width, full width, sortkey
    //     - more than one of traditional, simplified, sortkey
    //     - (LINGUISTIC flag) AND (NOT LCMAP_UPPER OR LCMAP_LOWER)
    //
    dwMapFlags &= (~LOCALE_USE_CP_ACP);
    if ( (dwMapFlags & LCMS_INVALID_FLAG) || (dwMapFlags == 0) ||
         ((dwMapFlags & (NORM_ALL | SORT_STRINGSORT)) &&
          (dwMapFlags & LCMAP_NO_NORM)) ||
         ((dwMapFlags & NORM_SORTKEY_ONLY) &&
          (!(dwMapFlags & LCMAP_SORTKEY))) ||
         (MORE_THAN_ONE(dwMapFlags, LCMS1_SINGLE_FLAG)) ||
         (MORE_THAN_ONE(dwMapFlags, LCMS2_SINGLE_FLAG)) ||
         (MORE_THAN_ONE(dwMapFlags, LCMS3_SINGLE_FLAG)) ||
         (MORE_THAN_ONE(dwMapFlags, LCMS4_SINGLE_FLAG)) ||
         ((dwMapFlags & LCMAP_LINGUISTIC_CASING) &&
          (!(dwMapFlags & (LCMAP_UPPERCASE | LCMAP_LOWERCASE)))) )
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return (0);
    }

    //
    //  If cchSrc is -1, then the source string is null terminated and we
    //  need to get the length of the source string.  Add one to the
    //  length to include the null termination.
    //  (This will always be at least 1.)
    //
    if (cchSrc <= -1)
    {
        cchSrc = NlsStrLenW(lpSrcStr) + 1;
    }

    //
    //  Map the string based on the given flags.
    //
    if (dwMapFlags & LCMAP_SORTKEY)
    {
        //
        //  Map the string to its sortkey.
        //
        //  NOTE:  This returns the number of BYTES, instead of the
        //         number of wide characters (words).
        //
        Count = MapSortKey( pHashN,
                            dwMapFlags,
                            lpSrcStr,
                            cchSrc,
                            (LPBYTE)lpDestStr,
                            cchDest,
                            IS_KOREAN(Locale) );
    }
    else
    {
        switch (dwMapFlags & ~(LCMAP_BYTEREV | LCMAP_LINGUISTIC_CASING))
        {
            case ( LCMAP_LOWERCASE ) :
            {
                //
                //  Map the string to Lower Case.
                //
                Count = MapCase( pHashN,
                                 lpSrcStr,
                                 cchSrc,
                                 lpDestStr,
                                 cchDest,
                                 (dwMapFlags & LCMAP_LINGUISTIC_CASING)
                                     ? pHashN->pLowerLinguist
                                     : pHashN->pLowerCase );
                break;
            }

            case ( LCMAP_UPPERCASE ) :
            {
                //
                //  Map the string to Upper Case.
                //
                Count = MapCase( pHashN,
                                 lpSrcStr,
                                 cchSrc,
                                 lpDestStr,
                                 cchDest,
                                 (dwMapFlags & LCMAP_LINGUISTIC_CASING)
                                     ? pHashN->pUpperLinguist
                                     : pHashN->pUpperCase );
                break;
            }

            case ( NORM_IGNORENONSPACE )                      :
            case ( NORM_IGNORESYMBOLS )                       :
            case ( NORM_IGNORENONSPACE | NORM_IGNORESYMBOLS ) :
            {
                //
                //  Map the string to strip out nonspace marks and/or symbols.
                //
                Count = MapNormalization( pHashN,
                                          dwMapFlags & ~LCMAP_BYTEREV,
                                          lpSrcStr,
                                          cchSrc,
                                          lpDestStr,
                                          cchDest );
                break;
            }

            case ( LCMAP_TRADITIONAL_CHINESE ) :
            case ( LCMAP_TRADITIONAL_CHINESE | LCMAP_LOWERCASE ) :
            case ( LCMAP_TRADITIONAL_CHINESE | LCMAP_UPPERCASE) :
            {
                //
                //  Map the string to Traditional Chinese.
                //
                Count = MapTraditionalSimplified( pHashN,
                                                  dwMapFlags & ~LCMAP_BYTEREV,
                                                  lpSrcStr,
                                                  cchSrc,
                                                  lpDestStr,
                                                  cchDest,
                                                  pTblPtrs->pTraditional );
                break;
            }

            case ( LCMAP_SIMPLIFIED_CHINESE )  :
            case ( LCMAP_SIMPLIFIED_CHINESE | LCMAP_LOWERCASE )  :
            case ( LCMAP_SIMPLIFIED_CHINESE | LCMAP_UPPERCASE )  :
            {
                //
                //  Map the string to Simplified Chinese.
                //
                Count = MapTraditionalSimplified( pHashN,
                                                  dwMapFlags & ~LCMAP_BYTEREV,
                                                  lpSrcStr,
                                                  cchSrc,
                                                  lpDestStr,
                                                  cchDest,
                                                  pTblPtrs->pSimplified );
                break;
            }

            default :
            {
                //
                //  Make sure the Chinese flags are not used with the
                //  Japanese flags.
                //
                if (dwMapFlags &
                     (LCMAP_TRADITIONAL_CHINESE | LCMAP_SIMPLIFIED_CHINESE))
                {
                    SetLastError(ERROR_INVALID_FLAGS);
                    return (0);
                }

                //
                //  The only flags not yet handled are the variations
                //  containing the Kana and/or Width flags.
                //  This handles all variations for:
                //      LCMAP_HIRAGANA
                //      LCMAP_KATAKANA
                //      LCMAP_HALFWIDTH
                //      LCMAP_FULLWIDTH
                //
                //      Allow LCMAP_LOWERCASE and LCMAP_UPPERCASE
                //      in combination with the kana and width flags.
                //
                Count = MapKanaWidth( pHashN,
                                      dwMapFlags & ~LCMAP_BYTEREV,
                                      lpSrcStr,
                                      cchSrc,
                                      lpDestStr,
                                      cchDest );
                break;
            }
        }
    }

    //
    //  Always check LCMAP_BYTEREV last and do it in place.
    //  LCMAP_BYTEREV may be used in combination with any other flag
    //  (except ignore case without sortkey) or by itself.
    //
    if (dwMapFlags & LCMAP_BYTEREV)
    {
        //
        //  Reverse the bytes of each word in the string.
        //
        if (dwMapFlags == LCMAP_BYTEREV)
        {
            //
            //  Byte Reversal flag is used by itself.
            //
            //  Make sure that the size of the destination buffer is
            //  larger than zero.  If it is zero, return the size of
            //  the source string only.  Do NOT touch lpDestStr.
            //
            if (cchDest != 0)
            {
                //
                //  Flag is used by itself.  Reverse the bytes from
                //  the source string and store them in the destination
                //  string.
                //
                if (cchSrc > cchDest)
                {
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    return (0);
                }

                for (ctr = 0; ctr < cchSrc; ctr++)
                {
                    lpDestStr[ctr] = MAKEWORD( HIBYTE(lpSrcStr[ctr]),
                                               LOBYTE(lpSrcStr[ctr]) );
                }
            }

            //
            //  Return the size of the source string.
            //
            Count = cchSrc;
        }
        else
        {
            //
            //  Make sure that the size of the destination buffer is
            //  larger than zero.  If it is zero, return the count and
            //  do NOT touch lpDestStr.
            //
            if (cchDest != 0)
            {
                //
                //  Check for sortkey flag.
                //
                if (dwMapFlags & LCMAP_SORTKEY)
                {
                    //
                    //  Sortkey flag is also set, so 'Count' contains the
                    //  number of BYTES instead of the number of words.
                    //
                    //  Reverse the bytes in place in the destination string.
                    //  No need to check the size of the destination buffer
                    //  here - it's been done elsewhere.
                    //
                    for (ctr = 0; ctr < Count / 2; ctr++)
                    {
                        lpDestStr[ctr] = MAKEWORD( HIBYTE(lpDestStr[ctr]),
                                                   LOBYTE(lpDestStr[ctr]) );
                    }
                }
                else
                {
                    //
                    //  Flag is used in combination with another flag.
                    //  Reverse the bytes in place in the destination string.
                    //  No need to check the size of the destination buffer
                    //  here - it's been done elsewhere.
                    //
                    for (ctr = 0; ctr < Count; ctr++)
                    {
                        lpDestStr[ctr] = MAKEWORD( HIBYTE(lpDestStr[ctr]),
                                                   LOBYTE(lpDestStr[ctr]) );
                    }
                }
            }
        }
    }

    //
    //  Return the number of characters (or number of bytes for sortkey)
    //  written to the buffer.
    //
    return (Count);
}




//-------------------------------------------------------------------------//
//                           INTERNAL ROUTINES                             //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  FoldCZone
//
//  Stores the compatibility zone values for the given string in the
//  destination buffer, and returns the number of wide characters
//  written to the buffer.
//
//  02-01-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int FoldCZone(
    LPCWSTR pSrc,
    int cchSrc,
    LPWSTR pDest,
    int cchDest)
{
    int ctr;                      // loop counter


    //
    //  If the destination value is zero, then just return the
    //  length of the source string.  Do NOT touch pDest.
    //
    if (cchDest == 0)
    {
        return (cchSrc);
    }

    //
    //  If cchSrc is greater than cchDest, then the destination buffer
    //  is too small to hold the new string.  Return an error.
    //
    if (cchSrc > cchDest)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return (0);
    }

    //
    //  Fold the Compatibility Zone and store it in the destination string.
    //
    for (ctr = 0; ctr < cchSrc; ctr++)
    {
        pDest[ctr] = GET_FOLD_CZONE(pTblPtrs->pCZone, pSrc[ctr]);
    }

    //
    //  Return the number of wide characters written.
    //
    return (ctr);
}


////////////////////////////////////////////////////////////////////////////
//
//  FoldDigits
//
//  Stores the ascii digits values for the given string in the
//  destination buffer, and returns the number of wide characters
//  written to the buffer.
//
//  02-01-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int FoldDigits(
    LPCWSTR pSrc,
    int cchSrc,
    LPWSTR pDest,
    int cchDest)
{
    int ctr;                      // loop counter


    //
    //  If the destination value is zero, then just return the
    //  length of the source string.  Do NOT touch pDest.
    //
    if (cchDest == 0)
    {
        return (cchSrc);
    }

    //
    //  If cchSrc is greater than cchDest, then the destination buffer
    //  is too small to hold the new string.  Return an error.
    //
    if (cchSrc > cchDest)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return (0);
    }

    //
    //  Fold the Ascii Digits and store it in the destination string.
    //
    for (ctr = 0; ctr < cchSrc; ctr++)
    {
        pDest[ctr] = GET_ASCII_DIGITS(pTblPtrs->pADigit, pSrc[ctr]);
    }

    //
    //  Return the number of wide characters written.
    //
    return (ctr);
}


////////////////////////////////////////////////////////////////////////////
//
//  FoldCZone_Digits
//
//  Stores the compatibility zone and ascii digits values for the given
//  string in the destination buffer, and returns the number of wide
//  characters written to the buffer.
//
//  02-01-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int FoldCZone_Digits(
    LPCWSTR pSrc,
    int cchSrc,
    LPWSTR pDest,
    int cchDest)
{
    int ctr;                      // loop counter


    //
    //  If the destination value is zero, then just return the
    //  length of the source string.  Do NOT touch pDest.
    //
    if (cchDest == 0)
    {
        return (cchSrc);
    }

    //
    //  If cchSrc is greater than cchDest, then the destination buffer
    //  is too small to hold the new string.  Return an error.
    //
    if (cchSrc > cchDest)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return (0);
    }

    //
    //  Fold the compatibility zone and the ascii digits values and store
    //  it in the destination string.
    //
    for (ctr = 0; ctr < cchSrc; ctr++)
    {
        pDest[ctr] = GET_FOLD_CZONE(pTblPtrs->pCZone, pSrc[ctr]);
        pDest[ctr] = GET_ASCII_DIGITS(pTblPtrs->pADigit, pDest[ctr]);
    }

    //
    //  Return the number of wide characters written.
    //
    return (ctr);
}


////////////////////////////////////////////////////////////////////////////
//
//  FoldLigatures
//
//  Stores the expanded ligature values for the given string in the
//  destination buffer, and returns the number of wide characters
//  written to the buffer.
//
//  10-15-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int FoldLigatures(
    LPCWSTR pSrc,
    int cchSrc,
    LPWSTR pDest,
    int cchDest)
{
    int ctr  = 0;                 // source char counter
    int ctr2 = 0;                 // destination char counter
    DWORD Weight;                 // sort weight - used for expansions


    //
    //  If the destination value is zero, then just return the
    //  length of the string that would be returned.  Do NOT touch pDest.
    //
    if (cchDest == 0)
    {
        //
        //  Convert the source string to expand all ligatures and calculate
        //  the number of characters that would have been written to a
        //  destination buffer.
        //
        while (ctr < cchSrc)
        {
            Weight = MAKE_SORTKEY_DWORD((pTblPtrs->pDefaultSortkey)[pSrc[ctr]]);
            if (GET_SCRIPT_MEMBER(&Weight) == EXPANSION)
            {
                do
                {
                    ctr2++;
                    Weight = MAKE_SORTKEY_DWORD(
                        (pTblPtrs->pDefaultSortkey)[GET_EXPANSION_2(&Weight)]);
                } while (GET_SCRIPT_MEMBER(&Weight) == EXPANSION);
                ctr2++;
            }
            else
            {
                ctr2++;
            }
            ctr++;
        }
    }
    else
    {
        //
        //  Convert the source string to expand all ligatures and store
        //  the result in the destination buffer.
        //
        while ((ctr < cchSrc) && (ctr2 < cchDest))
        {
            Weight = MAKE_SORTKEY_DWORD((pTblPtrs->pDefaultSortkey)[pSrc[ctr]]);
            if (GET_SCRIPT_MEMBER(&Weight) == EXPANSION)
            {
                do
                {
                    if ((ctr2 + 1) < cchDest)
                    {
                        pDest[ctr2]     = GET_EXPANSION_1(&Weight);
                        pDest[ctr2 + 1] = GET_EXPANSION_2(&Weight);
                        ctr2++;
                    }
                    else
                    {
                        ctr2++;
                        break;
                    }
                    Weight = MAKE_SORTKEY_DWORD(
                                 (pTblPtrs->pDefaultSortkey)[pDest[ctr2]]);
                } while (GET_SCRIPT_MEMBER(&Weight) == EXPANSION);

                if (ctr2 >= cchDest)
                {
                    break;
                }
                ctr2++;
            }
            else
            {
                pDest[ctr2] = pSrc[ctr];
                ctr2++;
            }
            ctr++;
        }
    }

    //
    //  Make sure destination buffer was large enough.
    //
    if (ctr < cchSrc)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return (0);
    }

    //
    //  Return the number of wide characters written.
    //
    return (ctr2);
}


////////////////////////////////////////////////////////////////////////////
//
//  FoldPreComposed
//
//  Stores the precomposed values for the given string in the
//  destination buffer, and returns the number of wide characters
//  written to the buffer.
//
//  02-01-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int FoldPreComposed(
    LPCWSTR pSrc,
    int cchSrc,
    LPWSTR pDest,
    int cchDest)
{
    int ctr  = 0;                 // source char counter
    int ctr2 = 0;                 // destination char counter
    WCHAR wch = 0;                // wchar holder


    //
    //  If the destination value is zero, then just return the
    //  length of the string that would be returned.  Do NOT touch pDest.
    //
    if (cchDest == 0)
    {
        //
        //  Convert the source string to precomposed and calculate the
        //  number of characters that would have been written to a
        //  destination buffer.
        //
        while (ctr < cchSrc)
        {
            if ((ctr2 != 0) &&
                (IS_NONSPACE_ONLY(pTblPtrs->pDefaultSortkey, pSrc[ctr])))
            {
                //
                //  Composite form.  Write the precomposed form.
                //
                //  If the precomposed character is written to the buffer,
                //  do NOT increment the destination pointer or the
                //  character count (the precomposed character was
                //  written over the previous character).
                //
                if (wch)
                {
                    if ((wch = GetPreComposedChar(pSrc[ctr], wch)) == 0)
                    {
                        //
                        //  No translation for composite form, so just
                        //  increment the destination counter.
                        //
                        ctr2++;
                    }
                }
                else
                {
                    if ((wch = GetPreComposedChar( pSrc[ctr],
                                                   pSrc[ctr - 1] )) == 0)
                    {
                        //
                        //  No translation for composite form, so just
                        //  increment the destination counter.
                        //
                        ctr2++;
                    }
                }
            }
            else
            {
                //
                //  Not part of a composite character, so just
                //  increment the destination counter.
                //
                wch = 0;
                ctr2++;
            }
            ctr++;
        }
    }
    else
    {
        //
        //  Convert the source string to precomposed and store it in the
        //  destination string.
        //
        while ((ctr < cchSrc) && (ctr2 < cchDest))
        {
            if ((ctr2 != 0) &&
                (IS_NONSPACE_ONLY(pTblPtrs->pDefaultSortkey, pSrc[ctr])))
            {
                //
                //  Composite form.  Write the precomposed form.
                //
                //  If the precomposed character is written to the buffer,
                //  do NOT increment the destination pointer or the
                //  character count (the precomposed character was
                //  written over the previous character).
                //
                wch = pDest[ctr2 - 1];
                if ((pDest[ctr2 - 1] =
                         GetPreComposedChar( pSrc[ctr],
                                             pDest[ctr2 - 1] )) == 0)
                {
                    //
                    //  No translation for composite form, so must
                    //  rewrite the base character and write the
                    //  composite character.
                    //
                    pDest[ctr2 - 1] = wch;
                    pDest[ctr2] = pSrc[ctr];
                    ctr2++;
                }
            }
            else
            {
                //
                //  Not part of a composite character, so just write
                //  the character to the destination string.
                //
                pDest[ctr2] = pSrc[ctr];
                ctr2++;
            }
            ctr++;
        }
    }

    //
    //  Make sure destination buffer was large enough.
    //
    if (ctr < cchSrc)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return (0);
    }

    //
    //  Return the number of wide characters written.
    //
    return (ctr2);
}


////////////////////////////////////////////////////////////////////////////
//
//  FoldComposite
//
//  Stores the composite values for the given string in the
//  destination buffer, and returns the number of wide characters
//  written to the buffer.
//
//  02-01-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int FoldComposite(
    LPCWSTR pSrc,
    int cchSrc,
    LPWSTR pDest,
    int cchDest)
{
    int ctr  = 0;                 // source char counter
    int ctr2 = 0;                 // destination char counter
    LPWSTR pEndDest;              // ptr to end of destination string
    WCHAR pTmp[MAX_COMPOSITE];    // tmp buffer for composite chars


    //
    //  If the destination value is zero, then just return the
    //  length of the string that would be returned.  Do NOT touch pDest.
    //
    if (cchDest == 0)
    {
        //
        //  Get the end of the tmp buffer.
        //
        pEndDest = (LPWSTR)pTmp + MAX_COMPOSITE;

        //
        //  Convert the source string to precomposed and calculate the
        //  number of characters that would have been written to a
        //  destination buffer.
        //
        while (ctr < cchSrc)
        {
            //
            //  Write the character to the destination string.
            //
            *pTmp = pSrc[ctr];

            //
            //  See if it needs to be expanded to its composite form.
            //
            //  If no composite form is found, the routine returns 1 for
            //  the base character.  Simply increment by the return value.
            //
            ctr2 += InsertCompositeForm(pTmp, pEndDest);

            //
            //  Increment the source string counter.
            //
            ctr++;
        }
    }
    else
    {
        //
        //  Get the end of the destination string.
        //
        pEndDest = (LPWSTR)pDest + cchDest;

        //
        //  Convert the source string to precomposed and store it in the
        //  destination string.
        //
        while ((ctr < cchSrc) && (ctr2 < cchDest))
        {
            //
            //  Write the character to the destination string.
            //
            pDest[ctr2] = pSrc[ctr];

            //
            //  See if it needs to be expanded to its composite form.
            //
            //  If no composite form is found, the routine returns 1 for
            //  the base character.  Simply increment by the return value.
            //
            ctr2 += InsertCompositeForm(&(pDest[ctr2]), pEndDest);

            //
            //  Increment the source string counter.
            //
            ctr++;
        }
    }

    //
    //  Make sure destination buffer was large enough.
    //
    if (ctr < cchSrc)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return (0);
    }

    //
    //  Return the number of wide characters written.
    //
    return (ctr2);
}


////////////////////////////////////////////////////////////////////////////
//
//  MapCase
//
//  Stores the lower or upper case values for the given string in the
//  destination buffer, and returns the number of wide characters written to
//  the buffer.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int MapCase(
    PLOC_HASH pHashN,
    LPCWSTR pSrc,
    int cchSrc,
    LPWSTR pDest,
    int cchDest,
    PCASE pCaseTbl)
{
    int ctr;                      // loop counter


    //
    //  If the destination value is zero, then just return the
    //  length of the source string.  Do NOT touch pDest.
    //
    if (cchDest == 0)
    {
        return (cchSrc);
    }

    //
    //  If cchSrc is greater than cchDest, then the destination buffer
    //  is too small to hold the lower or upper case string.  Return an
    //  error.
    //
    if (cchSrc > cchDest)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return (0);
    }

    //
    //  Lower or Upper case the source string and store it in the
    //  destination string.
    //
    for (ctr = 0; ctr < cchSrc; ctr++)
    {
        pDest[ctr] = GET_LOWER_UPPER_CASE(pCaseTbl, pSrc[ctr]);
    }

    //
    //  Return the number of wide characters written.
    //
    return (ctr);
}


////////////////////////////////////////////////////////////////////////////
//
//  SPECIAL_CASE_HANDLER
//
//  Handles all of the special cases for each character.  This includes only
//  the valid values less than or equal to MAX_SPECIAL_CASE.
//
//  DEFINED AS A MACRO.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define EXTRA_WEIGHT_POS(WtNum)        (*(pPosXW + (WtNum * WeightLen)))

#define SPECIAL_CASE_HANDLER( SM,                                           \
                              pWeight,                                      \
                              pSortkey,                                     \
                              pExpand,                                      \
                              Position,                                     \
                              fStringSort,                                  \
                              fIgnoreSymbols,                               \
                              pCur,                                         \
                              pBegin,                                       \
                              fModify )                                     \
{                                                                           \
    PSORTKEY pExpWt;              /* weight of 1 expansion char */          \
    BYTE AW;                      /* alphanumeric weight */                 \
    BYTE XW;                      /* case weight value with extra bits */   \
    DWORD PrevWt;                 /* previous weight */                     \
    BYTE PrevSM;                  /* previous script member */              \
    BYTE PrevAW;                  /* previuos alphanumeric weight */        \
    BYTE PrevCW;                  /* previuos case weight */                \
    LPWSTR pPrev;                 /* ptr to previous char */                \
                                                                            \
                                                                            \
    switch (SM)                                                             \
    {                                                                       \
        case ( UNSORTABLE ) :                                               \
        {                                                                   \
            /*                                                              \
             *  Character is unsortable, so skip it.                        \
             */                                                             \
            break;                                                          \
        }                                                                   \
                                                                            \
        case ( NONSPACE_MARK ) :                                            \
        {                                                                   \
            /*                                                              \
             *  Character is a nonspace mark, so only store                 \
             *  the diacritic weight.                                       \
             */                                                             \
            if (pPosDW > pDW)                                               \
            {                                                               \
                (*(pPosDW - 1)) += GET_DIACRITIC(pWeight);                  \
            }                                                               \
            else                                                            \
            {                                                               \
                *pPosDW = GET_DIACRITIC(pWeight);                           \
                pPosDW++;                                                   \
            }                                                               \
                                                                            \
            break;                                                          \
        }                                                                   \
                                                                            \
        case ( EXPANSION ) :                                                \
        {                                                                   \
            /*                                                              \
             *  Expansion character - one character has 2                   \
             *  different weights.  Store each weight separately.           \
             */                                                             \
            pExpWt = &(pSortkey[(pExpand[GET_EXPAND_INDEX(pWeight)]).UCP1]); \
            *pPosUW = GET_UNICODE_MOD(pExpWt, fModify);                     \
            *pPosDW = GET_DIACRITIC(pExpWt);                                \
            *pPosCW = GET_CASE(pExpWt) & CaseMask;                          \
            pPosUW++;                                                       \
            pPosDW++;                                                       \
            pPosCW++;                                                       \
                                                                            \
            pExpWt = &(pSortkey[(pExpand[GET_EXPAND_INDEX(pWeight)]).UCP2]); \
            while (GET_SCRIPT_MEMBER(pExpWt) == EXPANSION)                  \
            {                                                               \
                pWeight = pExpWt;                                           \
                pExpWt = &(pSortkey[(pExpand[GET_EXPAND_INDEX(pWeight)]).UCP1]); \
                *pPosUW = GET_UNICODE_MOD(pExpWt, fModify);                 \
                *pPosDW = GET_DIACRITIC(pExpWt);                            \
                *pPosCW = GET_CASE(pExpWt) & CaseMask;                      \
                pPosUW++;                                                   \
                pPosDW++;                                                   \
                pPosCW++;                                                   \
                pExpWt = &(pSortkey[(pExpand[GET_EXPAND_INDEX(pWeight)]).UCP2]); \
            }                                                               \
            *pPosUW = GET_UNICODE_MOD(pExpWt, fModify);                     \
            *pPosDW = GET_DIACRITIC(pExpWt);                                \
            *pPosCW = GET_CASE(pExpWt) & CaseMask;                          \
            pPosUW++;                                                       \
            pPosDW++;                                                       \
            pPosCW++;                                                       \
                                                                            \
            break;                                                          \
        }                                                                   \
                                                                            \
        case ( PUNCTUATION ) :                                              \
        {                                                                   \
            if (!fStringSort)                                               \
            {                                                               \
                /*                                                          \
                 *  Word Sort Method.                                       \
                 *                                                          \
                 *  Character is punctuation, so only store the special     \
                 *  weight.                                                 \
                 */                                                         \
                *((LPBYTE)pPosSW)       = HIBYTE(GET_POSITION_SW(Position)); \
                *(((LPBYTE)pPosSW) + 1) = LOBYTE(GET_POSITION_SW(Position)); \
                pPosSW++;                                                   \
                *pPosSW = GET_SPECIAL_WEIGHT(pWeight);                      \
                pPosSW++;                                                   \
                                                                            \
                break;                                                      \
            }                                                               \
                                                                            \
            /*                                                              \
             *  If using STRING sort method, treat punctuation the same     \
             *  as symbol.  So, FALL THROUGH to the symbol cases.           \
             */                                                             \
        }                                                                   \
                                                                            \
        case ( SYMBOL_1 ) :                                                 \
        case ( SYMBOL_2 ) :                                                 \
        case ( SYMBOL_3 ) :                                                 \
        case ( SYMBOL_4 ) :                                                 \
        case ( SYMBOL_5 ) :                                                 \
        {                                                                   \
            /*                                                              \
             *  Character is a symbol.                                      \
             *  Store the Unicode weights ONLY if the NORM_IGNORESYMBOLS    \
             *  flag is NOT set.                                            \
             */                                                             \
            if (!fIgnoreSymbols)                                            \
            {                                                               \
                *pPosUW = GET_UNICODE_MOD(pWeight, fModify);                \
                *pPosDW = GET_DIACRITIC(pWeight);                           \
                *pPosCW = GET_CASE(pWeight) & CaseMask;                     \
                pPosUW++;                                                   \
                pPosDW++;                                                   \
                pPosCW++;                                                   \
            }                                                               \
                                                                            \
            break;                                                          \
        }                                                                   \
                                                                            \
        case ( FAREAST_SPECIAL ) :                                          \
        {                                                                   \
            /*                                                              \
             *  Get the alphanumeric weight and the case weight of the      \
             *  current code point.                                         \
             */                                                             \
            AW = GET_ALPHA_NUMERIC(pWeight);                                \
            XW = (GET_CASE(pWeight) & CaseMask) | CASE_XW_MASK;             \
                                                                            \
            /*                                                              \
             *  Special case Repeat and Cho-On.                             \
             *    AW = 0  =>  Repeat                                        \
             *    AW = 1  =>  Cho-On                                        \
             *    AW = 2+ =>  Kana                                          \
             */                                                             \
            if (AW <= MAX_SPECIAL_AW)                                       \
            {                                                               \
                /*                                                          \
                 *  If the script member of the previous character is       \
                 *  invalid, then give the special character an             \
                 *  invalid weight (highest possible weight) so that it     \
                 *  will sort AFTER everything else.                        \
                 */                                                         \
                pPrev = pCur - 1;                                           \
                *pPosUW = MAP_INVALID_UW;                                   \
                while (pPrev >= pBegin)                                     \
                {                                                           \
                    PrevWt = GET_DWORD_WEIGHT(pHashN, *pPrev);              \
                    PrevSM = GET_SCRIPT_MEMBER(&PrevWt);                    \
                    if (PrevSM < FAREAST_SPECIAL)                           \
                    {                                                       \
                        if (PrevSM != EXPANSION)                            \
                        {                                                   \
                            /*                                              \
                             *  UNSORTABLE or NONSPACE_MARK.                \
                             *                                              \
                             *  Just ignore these, since we only care       \
                             *  about the previous UW value.                \
                             */                                             \
                            pPrev--;                                        \
                            continue;                                       \
                        }                                                   \
                    }                                                       \
                    else if (PrevSM == FAREAST_SPECIAL)                     \
                    {                                                       \
                        PrevAW = GET_ALPHA_NUMERIC(&PrevWt);                \
                        if (PrevAW <= MAX_SPECIAL_AW)                       \
                        {                                                   \
                            /*                                              \
                             *  Handle case where two special chars follow  \
                             *  each other.  Keep going back in the string. \
                             */                                             \
                            pPrev--;                                        \
                            continue;                                       \
                        }                                                   \
                                                                            \
                        *pPosUW = MAKE_UNICODE_WT(KANA, PrevAW, fModify);   \
                                                                            \
                        /*                                                  \
                         *  Only build weights 4, 5, 6, and 7 if the        \
                         *  previous character is KANA.                     \
                         *                                                  \
                         *  Always:                                         \
                         *    4W = previous CW  &  ISOLATE_SMALL            \
                         *    6W = previous CW  &  ISOLATE_KANA             \
                         *                                                  \
                         */                                                 \
                        PrevCW = (GET_CASE(&PrevWt) & CaseMask) |           \
                                 CASE_XW_MASK;                              \
                        EXTRA_WEIGHT_POS(0) = PrevCW & ISOLATE_SMALL;       \
                        EXTRA_WEIGHT_POS(2) = PrevCW & ISOLATE_KANA;        \
                                                                            \
                        if (AW == AW_REPEAT)                                \
                        {                                                   \
                            /*                                              \
                             *  Repeat:                                     \
                             *    UW = previous UW   (set above)            \
                             *    5W = WT_FIVE_REPEAT                       \
                             *    7W = previous CW  &  ISOLATE_WIDTH        \
                             */                                             \
                            EXTRA_WEIGHT_POS(1) = WT_FIVE_REPEAT;           \
                            EXTRA_WEIGHT_POS(3) = PrevCW & ISOLATE_WIDTH;   \
                        }                                                   \
                        else                                                \
                        {                                                   \
                            /*                                              \
                             *  Cho-On:                                     \
                             *    UW = previous UW  &  CHO_ON_UW_MASK       \
                             *    5W = WT_FIVE_CHO_ON                       \
                             *    7W = current  CW  &  ISOLATE_WIDTH        \
                             */                                             \
                            *pPosUW &= CHO_ON_UW_MASK;                      \
                            EXTRA_WEIGHT_POS(1) = WT_FIVE_CHO_ON;           \
                            EXTRA_WEIGHT_POS(3) = XW & ISOLATE_WIDTH;       \
                        }                                                   \
                                                                            \
                        pPosXW++;                                           \
                    }                                                       \
                    else                                                    \
                    {                                                       \
                        *pPosUW = GET_UNICODE_MOD(&PrevWt, fModify);        \
                    }                                                       \
                                                                            \
                    break;                                                  \
                }                                                           \
                                                                            \
                /*                                                          \
                 *  Make sure there is a valid UW.  If not, quit out        \
                 *  of switch case.                                         \
                 */                                                         \
                if (*pPosUW == MAP_INVALID_UW)                              \
                {                                                           \
                    pPosUW++;                                               \
                    break;                                                  \
                }                                                           \
            }                                                               \
            else                                                            \
            {                                                               \
                /*                                                          \
                 *  Kana:                                                   \
                 *    SM = KANA                                             \
                 *    AW = current AW                                       \
                 *    4W = current CW  &  ISOLATE_SMALL                     \
                 *    5W = WT_FIVE_KANA                                     \
                 *    6W = current CW  &  ISOLATE_KANA                      \
                 *    7W = current CW  &  ISOLATE_WIDTH                     \
                 */                                                         \
                *pPosUW = MAKE_UNICODE_WT(KANA, AW, fModify);               \
                EXTRA_WEIGHT_POS(0) = XW & ISOLATE_SMALL;                   \
                EXTRA_WEIGHT_POS(1) = WT_FIVE_KANA;                         \
                EXTRA_WEIGHT_POS(2) = XW & ISOLATE_KANA;                    \
                EXTRA_WEIGHT_POS(3) = XW & ISOLATE_WIDTH;                   \
                                                                            \
                pPosXW++;                                                   \
            }                                                               \
                                                                            \
            /*                                                              \
             *  Always:                                                     \
             *    DW = current DW                                           \
             *    CW = minimum CW                                           \
             */                                                             \
            *pPosDW = GET_DIACRITIC(pWeight);                               \
            *pPosCW = MIN_CW;                                               \
                                                                            \
            pPosUW++;                                                       \
            pPosDW++;                                                       \
            pPosCW++;                                                       \
                                                                            \
            break;                                                          \
        }                                                                   \
                                                                            \
        case ( RESERVED_2 ) :                                               \
        case ( RESERVED_3 ) :                                               \
        {                                                                   \
            /*                                                              \
             *  Fill out the case statement so the compiler                 \
             *  will use a jump table.                                      \
             */                                                             \
            ;                                                               \
        }                                                                   \
    }                                                                       \
}


////////////////////////////////////////////////////////////////////////////
//
//  MapSortKey
//
//  Stores the sortkey weights for the given string in the destination
//  buffer and returns the number of BYTES written to the buffer.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int MapSortKey(
    PLOC_HASH pHashN,
    DWORD dwFlags,
    LPCWSTR pSrc,
    int cchSrc,
    LPBYTE pDest,
    int cbDest,
    BOOL fModify)
{
    register int WeightLen;       // length of one set of weights
    LPWSTR pUW;                   // ptr to Unicode Weights
    LPBYTE pDW;                   // ptr to Diacritic Weights
    LPBYTE pCW;                   // ptr to Case Weights
    LPBYTE pXW;                   // ptr to Extra Weights
    LPWSTR pSW;                   // ptr to Special Weights
    LPWSTR pPosUW;                // ptr to position in pUW buffer
    LPBYTE pPosDW;                // ptr to position in pDW buffer
    LPBYTE pPosCW;                // ptr to position in pCW buffer
    LPBYTE pPosXW;                // ptr to position in pXW buffer
    LPWSTR pPosSW;                // ptr to position in pSW buffer
    PSORTKEY pWeight;             // ptr to weight of character
    BYTE SM;                      // script member value
    BYTE CaseMask;                // mask for case weight
    int PosCtr;                   // position counter in string
    LPWSTR pPos;                  // ptr to position in string
    LPBYTE pTmp;                  // ptr to go through UW, XW, and SW
    LPBYTE pPosTmp;               // ptr to tmp position in XW
    PCOMPRESS_2 pComp2;           // ptr to compression 2 list
    PCOMPRESS_3 pComp3;           // ptr to compression 3 list
    WORD pBuffer[MAX_SKEYBUFLEN]; // buffer to hold weights
    int ctr;                      // loop counter
    BOOL IfDblCompress;           // if double compress possibility
    BOOL fStringSort;             // if using string sort method
    BOOL fIgnoreSymbols;          // if ignore symbols flag is set


    //
    //  See if the length of the string is too large for the static
    //  buffer.  If so, allocate a buffer that is large enough.
    //
    if (cchSrc > MAX_STRING_LEN)
    {
        //
        //  Allocate buffer to hold all of the weights.
        //     (cchSrc) * (max # of expansions) * (# of weights)
        //
        WeightLen = cchSrc * MAX_EXPANSION;
        if ((pUW = (LPWSTR)NLS_ALLOC_MEM( WeightLen * MAX_WEIGHTS *
                                          sizeof(WCHAR) )) == NULL)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return (0);
        }
    }
    else
    {
        WeightLen = MAX_STRING_LEN * MAX_EXPANSION;
        pUW = (LPWSTR)pBuffer;
    }

    //
    //  Set the case weight mask based on the given flags.
    //  If none or all of the ignore case flags are set, then
    //  just leave the mask as 0xff.
    //
    CaseMask = 0xff;
    switch (dwFlags & NORM_ALL_CASE)
    {
        case ( NORM_IGNORECASE ) :
        {
            CaseMask &= CASE_UPPER_MASK;
            break;
        }
        case ( NORM_IGNOREKANATYPE ) :
        {
            CaseMask &= CASE_KANA_MASK;
            break;
        }
        case ( NORM_IGNOREWIDTH ) :
        {
            CaseMask &= CASE_WIDTH_MASK;
            break;
        }
        case ( NORM_IGNORECASE | NORM_IGNOREKANATYPE ) :
        {
            CaseMask &= (CASE_UPPER_MASK & CASE_KANA_MASK);
            break;
        }
        case ( NORM_IGNORECASE | NORM_IGNOREWIDTH ) :
        {
            CaseMask &= (CASE_UPPER_MASK & CASE_WIDTH_MASK);
            break;
        }
        case ( NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH ) :
        {
            CaseMask &= (CASE_KANA_MASK & CASE_WIDTH_MASK);
            break;
        }
        case ( NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH ) :
        {
            CaseMask &= (CASE_UPPER_MASK & CASE_KANA_MASK & CASE_WIDTH_MASK);
            break;
        }
    }

    //
    //  Set pointers to positions of weights in buffer.
    //
    //      UW  =>  word   length
    //      DW  =>  byte   length
    //      CW  =>  byte   length
    //      XW  =>  4 byte length  (4 weights, 1 byte each)
    //      SW  =>  dword  length  (2 words each)
    //
    pDW = (LPBYTE)(pUW + (WeightLen * (NUM_BYTES_UW / sizeof(WCHAR))));
    pCW = (LPBYTE)(pDW + (WeightLen * NUM_BYTES_DW));
    pXW = (LPBYTE)(pCW + (WeightLen * NUM_BYTES_CW));
    pSW = (LPWSTR)(pXW + (WeightLen * NUM_BYTES_XW));
    pPosUW = pUW;
    pPosDW = pDW;
    pPosCW = pCW;
    pPosXW = pXW;
    pPosSW = pSW;

    //
    //  Initialize flags and loop values.
    //
    fStringSort = dwFlags & SORT_STRINGSORT;
    fIgnoreSymbols = dwFlags & NORM_IGNORESYMBOLS;
    pPos = (LPWSTR)pSrc;
    PosCtr = 1;

    //
    //  Check if given locale has compressions.
    //
    if (pHashN->IfCompression == FALSE)
    {
        //
        //  Go through string, code point by code point.
        //
        //  No compressions exist in the given locale, so
        //  DO NOT check for them.
        //
        for (; PosCtr <= cchSrc; PosCtr++, pPos++)
        {
            //
            //  Get weights.
            //
            pWeight = &((pHashN->pSortkey)[*pPos]);
            SM = GET_SCRIPT_MEMBER(pWeight);

            if (SM > MAX_SPECIAL_CASE)
            {
                //
                //  No special case on character, so store the
                //  various weights for the character.
                //
                *pPosUW = GET_UNICODE_MOD(pWeight, fModify);
                *pPosDW = GET_DIACRITIC(pWeight);
                *pPosCW = GET_CASE(pWeight) & CaseMask;
                pPosUW++;
                pPosDW++;
                pPosCW++;
            }
            else
            {
                SPECIAL_CASE_HANDLER( SM,
                                      pWeight,
                                      pHashN->pSortkey,
                                      pTblPtrs->pExpansion,
                                      pPosUW - pUW + 1,
                                      fStringSort,
                                      fIgnoreSymbols,
                                      pPos,
                                      (LPWSTR)pSrc,
                                      fModify );
            }
        }
    }
    else if (pHashN->IfDblCompression == FALSE)
    {
        //
        //  Go through string, code point by code point.
        //
        //  Compressions DO exist in the given locale, so
        //  check for them.
        //
        //  No double compressions exist in the given locale,
        //  so DO NOT check for them.
        //
        for (; PosCtr <= cchSrc; PosCtr++, pPos++)
        {
            //
            //  Get weights.
            //
            pWeight = &((pHashN->pSortkey)[*pPos]);
            SM = GET_SCRIPT_MEMBER(pWeight);

            if (SM > MAX_SPECIAL_CASE)
            {
                //
                //  No special case on character, but must check for
                //  compression characters.
                //
                switch (GET_COMPRESSION(pWeight))
                {
                    case ( COMPRESS_3_MASK ) :
                    {
                        if ((PosCtr + 2) <= cchSrc)
                        {
                            ctr = pHashN->pCompHdr->Num3;
                            pComp3 = pHashN->pCompress3;
                            for (; ctr > 0; ctr--, pComp3++)
                            {
                                if ((pComp3->UCP1 == *pPos) &&
                                    (pComp3->UCP2 == *(pPos + 1)) &&
                                    (pComp3->UCP3 == *(pPos + 2)))
                                {
                                    pWeight = &(pComp3->Weights);
                                    *pPosUW = GET_UNICODE_MOD(pWeight, fModify);
                                    *pPosDW = GET_DIACRITIC(pWeight);
                                    *pPosCW = GET_CASE(pWeight) & CaseMask;
                                    pPosUW++;
                                    pPosDW++;
                                    pPosCW++;

                                    //
                                    //  Add only two to source, since one
                                    //  will be added by "for" structure.
                                    //
                                    pPos += 2;
                                    PosCtr += 2;
                                    break;
                                }
                            }
                            if (ctr > 0)
                            {
                                break;
                            }
                        }

                        //
                        //  Fall through if not found.
                        //
                    }

                    case ( COMPRESS_2_MASK ) :
                    {
                        if ((PosCtr + 1) <= cchSrc)
                        {
                            ctr = pHashN->pCompHdr->Num2;
                            pComp2 = pHashN->pCompress2;
                            for (; ctr > 0; ctr--, pComp2++)
                            {
                                if ((pComp2->UCP1 == *pPos) &&
                                    (pComp2->UCP2 == *(pPos + 1)))
                                {
                                    pWeight = &(pComp2->Weights);
                                    *pPosUW = GET_UNICODE_MOD(pWeight, fModify);
                                    *pPosDW = GET_DIACRITIC(pWeight);
                                    *pPosCW = GET_CASE(pWeight) & CaseMask;
                                    pPosUW++;
                                    pPosDW++;
                                    pPosCW++;

                                    //
                                    //  Add only one to source, since one
                                    //  will be added by "for" structure.
                                    //
                                    pPos++;
                                    PosCtr++;
                                    break;
                                }
                            }
                            if (ctr > 0)
                            {
                                break;
                            }
                        }

                        //
                        //  Fall through if not found.
                        //
                    }

                    default :
                    {
                        //
                        //  No possible compression for character, so store
                        //  the various weights for the character.
                        //
                        *pPosUW = GET_UNICODE_SM_MOD(pWeight, SM, fModify);
                        *pPosDW = GET_DIACRITIC(pWeight);
                        *pPosCW = GET_CASE(pWeight) & CaseMask;
                        pPosUW++;
                        pPosDW++;
                        pPosCW++;
                    }
                }
            }
            else
            {
                SPECIAL_CASE_HANDLER( SM,
                                      pWeight,
                                      pHashN->pSortkey,
                                      pTblPtrs->pExpansion,
                                      pPosUW - pUW + 1,
                                      fStringSort,
                                      fIgnoreSymbols,
                                      pPos,
                                      (LPWSTR)pSrc,
                                      fModify );
            }
        }
    }
    else
    {
        //
        //  Go through string, code point by code point.
        //
        //  Compressions DO exist in the given locale, so
        //  check for them.
        //
        //  Double Compressions also exist in the given locale,
        //  so check for them.
        //
        for (; PosCtr <= cchSrc; PosCtr++, pPos++)
        {
            //
            //  Get weights.
            //
            pWeight = &((pHashN->pSortkey)[*pPos]);
            SM = GET_SCRIPT_MEMBER(pWeight);

            if (SM > MAX_SPECIAL_CASE)
            {
                //
                //  No special case on character, but must check for
                //  compression characters and double compression
                //  characters.
                //
                IfDblCompress =
                  (((PosCtr + 1) <= cchSrc) &&
                   ((GET_DWORD_WEIGHT(pHashN, *pPos) & CMP_MASKOFF_CW) ==
                    (GET_DWORD_WEIGHT(pHashN, *(pPos + 1)) & CMP_MASKOFF_CW)))
                   ? 1
                   : 0;

                switch (GET_COMPRESSION(pWeight))
                {
                    case ( COMPRESS_3_MASK ) :
                    {
                        if (IfDblCompress)
                        {
                            if ((PosCtr + 3) <= cchSrc)
                            {
                                ctr = pHashN->pCompHdr->Num3;
                                pComp3 = pHashN->pCompress3;
                                for (; ctr > 0; ctr--, pComp3++)
                                {
                                    if ((pComp3->UCP1 == *(pPos + 1)) &&
                                        (pComp3->UCP2 == *(pPos + 2)) &&
                                        (pComp3->UCP3 == *(pPos + 3)))
                                    {
                                        pWeight = &(pComp3->Weights);
                                        *pPosUW = GET_UNICODE_MOD(pWeight, fModify);
                                        *pPosDW = GET_DIACRITIC(pWeight);
                                        *pPosCW = GET_CASE(pWeight) & CaseMask;
                                        *(pPosUW + 1) = *pPosUW;
                                        *(pPosDW + 1) = *pPosDW;
                                        *(pPosCW + 1) = *pPosCW;
                                        pPosUW += 2;
                                        pPosDW += 2;
                                        pPosCW += 2;

                                        //
                                        //  Add only three to source, since one
                                        //  will be added by "for" structure.
                                        //
                                        pPos += 3;
                                        PosCtr += 3;
                                        break;
                                    }
                                }
                                if (ctr > 0)
                                {
                                    break;
                                }
                            }
                        }

                        //
                        //  Fall through if not found.
                        //
                        if ((PosCtr + 2) <= cchSrc)
                        {
                            ctr = pHashN->pCompHdr->Num3;
                            pComp3 = pHashN->pCompress3;
                            for (; ctr > 0; ctr--, pComp3++)
                            {
                                if ((pComp3->UCP1 == *pPos) &&
                                    (pComp3->UCP2 == *(pPos + 1)) &&
                                    (pComp3->UCP3 == *(pPos + 2)))
                                {
                                    pWeight = &(pComp3->Weights);
                                    *pPosUW = GET_UNICODE_MOD(pWeight, fModify);
                                    *pPosDW = GET_DIACRITIC(pWeight);
                                    *pPosCW = GET_CASE(pWeight) & CaseMask;
                                    pPosUW++;
                                    pPosDW++;
                                    pPosCW++;

                                    //
                                    //  Add only two to source, since one
                                    //  will be added by "for" structure.
                                    //
                                    pPos += 2;
                                    PosCtr += 2;
                                    break;
                                }
                            }
                            if (ctr > 0)
                            {
                                break;
                            }
                        }
                        //
                        //  Fall through if not found.
                        //
                    }

                    case ( COMPRESS_2_MASK ) :
                    {
                        if (IfDblCompress)
                        {
                            if ((PosCtr + 2) <= cchSrc)
                            {
                                ctr = pHashN->pCompHdr->Num2;
                                pComp2 = pHashN->pCompress2;
                                for (; ctr > 0; ctr--, pComp2++)
                                {
                                    if ((pComp2->UCP1 == *(pPos + 1)) &&
                                        (pComp2->UCP2 == *(pPos + 2)))
                                    {
                                        pWeight = &(pComp2->Weights);
                                        *pPosUW = GET_UNICODE_MOD(pWeight, fModify);
                                        *pPosDW = GET_DIACRITIC(pWeight);
                                        *pPosCW = GET_CASE(pWeight) & CaseMask;
                                        *(pPosUW + 1) = *pPosUW;
                                        *(pPosDW + 1) = *pPosDW;
                                        *(pPosCW + 1) = *pPosCW;
                                        pPosUW += 2;
                                        pPosDW += 2;
                                        pPosCW += 2;

                                        //
                                        //  Add only two to source, since one
                                        //  will be added by "for" structure.
                                        //
                                        pPos += 2;
                                        PosCtr += 2;
                                        break;
                                    }
                                }
                                if (ctr > 0)
                                {
                                    break;
                                }
                            }
                        }

                        //
                        //  Fall through if not found.
                        //
                        if ((PosCtr + 1) <= cchSrc)
                        {
                            ctr = pHashN->pCompHdr->Num2;
                            pComp2 = pHashN->pCompress2;
                            for (; ctr > 0; ctr--, pComp2++)
                            {
                                if ((pComp2->UCP1 == *pPos) &&
                                    (pComp2->UCP2 == *(pPos + 1)))
                                {
                                    pWeight = &(pComp2->Weights);
                                    *pPosUW = GET_UNICODE_MOD(pWeight, fModify);
                                    *pPosDW = GET_DIACRITIC(pWeight);
                                    *pPosCW = GET_CASE(pWeight) & CaseMask;
                                    pPosUW++;
                                    pPosDW++;
                                    pPosCW++;

                                    //
                                    //  Add only one to source, since one
                                    //  will be added by "for" structure.
                                    //
                                    pPos++;
                                    PosCtr++;
                                    break;
                                }
                            }
                            if (ctr > 0)
                            {
                                break;
                            }
                        }

                        //
                        //  Fall through if not found.
                        //
                    }

                    default :
                    {
                        //
                        //  No possible compression for character, so store
                        //  the various weights for the character.
                        //
                        *pPosUW = GET_UNICODE_SM_MOD(pWeight, SM, fModify);
                        *pPosDW = GET_DIACRITIC(pWeight);
                        *pPosCW = GET_CASE(pWeight) & CaseMask;
                        pPosUW++;
                        pPosDW++;
                        pPosCW++;
                    }
                }
            }
            else
            {
                SPECIAL_CASE_HANDLER( SM,
                                      pWeight,
                                      pHashN->pSortkey,
                                      pTblPtrs->pExpansion,
                                      pPosUW - pUW + 1,
                                      fStringSort,
                                      fIgnoreSymbols,
                                      pPos,
                                      (LPWSTR)pSrc,
                                      fModify );
            }
        }
    }

    //
    //  Store the final sortkey weights in the destination buffer.
    //
    //  PosCtr will be a BYTE count.
    //
    PosCtr = 0;

    //
    //  If the destination value is zero, then just return the
    //  length of the string that would be returned.  Do NOT touch pDest.
    //
    if (cbDest == 0)
    {
        //
        //  Count the Unicode Weights.
        //
        PosCtr += (int)((LPBYTE)pPosUW - (LPBYTE)pUW);

        //
        //  Count the Separator.
        //
        PosCtr++;

        //
        //  Count the Diacritic Weights.
        //
        //    - Eliminate minimum DW.
        //    - Count the number of diacritic weights.
        //
        if (!(dwFlags & NORM_IGNORENONSPACE))
        {
            pPosDW--;
            if (pHashN->IfReverseDW == TRUE)
            {
                //
                //  Reverse diacritics:
                //    - remove diacritics from left  to right.
                //    - count  diacritics from right to left.
                //
                while ((pDW <= pPosDW) && (*pDW <= MIN_DW))
                {
                    pDW++;
                }
                PosCtr += (int)(pPosDW - pDW + 1);
            }
            else
            {
                //
                //  Regular diacritics:
                //    - remove diacritics from right to left.
                //    - count  diacritics from left  to right.
                //
                while ((pPosDW >= pDW) && (*pPosDW <= MIN_DW))
                {
                    pPosDW--;
                }
                PosCtr += (int)(pPosDW - pDW + 1);
            }
        }

        //
        //  Count the Separator.
        //
        PosCtr++;

        //
        //  Count the Case Weights.
        //
        //    - Eliminate minimum CW.
        //    - Count the number of case weights.
        //
        if ((dwFlags & NORM_DROP_CW) != NORM_DROP_CW)
        {
            pPosCW--;
            while ((pPosCW >= pCW) && (*pPosCW <= MIN_CW))
            {
                pPosCW--;
            }
            PosCtr += (int)(pPosCW - pCW + 1);
        }

        //
        //  Count the Separator.
        //
        PosCtr++;

        //
        //  Count the Extra Weights.
        //
        //    - Eliminate EW.
        //    - Count the number of extra weights and separators.
        //
        if (pXW < pPosXW)
        {
            if (dwFlags & NORM_IGNORENONSPACE)
            {
                //
                //  Ignore 4W and 5W.  Must count separators for
                //  4W and 5W, though.
                //
                PosCtr += 2;
                ctr = 2;
            }
            else
            {
                ctr = 0;
            }
            pPosXW--;
            for (; ctr < NUM_BYTES_XW; ctr++)
            {
                pTmp = pXW + (WeightLen * ctr);
                pPosTmp = pPosXW + (WeightLen * ctr);
                while ((pPosTmp >= pTmp) && (*pPosTmp == pXWDrop[ctr]))
                {
                    pPosTmp--;
                }
                PosCtr += (int)(pPosTmp - pTmp + 1);

                //
                //  Count the Separator.
                //
                PosCtr++;
            }
        }

        //
        //  Count the Separator.
        //
        PosCtr++;

        //
        //  Count the Special Weights.
        //
        if (!fIgnoreSymbols)
        {
            PosCtr += (int)((LPBYTE)pPosSW - (LPBYTE)pSW);
        }

        //
        //  Count the Terminator.
        //
        PosCtr++;
    }
    else
    {
        //
        //  Store the Unicode Weights in the destination buffer.
        //
        //    - Make sure destination buffer is large enough.
        //    - Copy unicode weights to destination buffer.
        //
        //  NOTE:  cbDest is the number of BYTES.
        //         Also, must add one to length for separator.
        //
        if (cbDest < (((LPBYTE)pPosUW - (LPBYTE)pUW) + 1))
        {
            NLS_FREE_TMP_BUFFER(pUW, pBuffer);
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return (0);
        }
        pTmp = (LPBYTE)pUW;
        while (pTmp < (LPBYTE)pPosUW)
        {
            //
            //  Copy Unicode weight to destination buffer.
            //
            //  NOTE:  Unicode Weight is stored in the data file as
            //             Alphanumeric Weight, Script Member
            //         so that the WORD value will be read correctly.
            //
            pDest[PosCtr]     = *(pTmp + 1);
            pDest[PosCtr + 1] = *pTmp;
            PosCtr += 2;
            pTmp += 2;
        }

        //
        //  Copy Separator to destination buffer.
        //
        //  Destination buffer is large enough to hold the separator,
        //  since it was checked with the Unicode weights above.
        //
        pDest[PosCtr] = SORTKEY_SEPARATOR;
        PosCtr++;

        //
        //  Store the Diacritic Weights in the destination buffer.
        //
        //    - Eliminate minimum DW.
        //    - Make sure destination buffer is large enough.
        //    - Copy diacritic weights to destination buffer.
        //
        if (!(dwFlags & NORM_IGNORENONSPACE))
        {
            pPosDW--;
            if (pHashN->IfReverseDW == TRUE)
            {
                //
                //  Reverse diacritics:
                //    - remove diacritics from left  to right.
                //    - store  diacritics from right to left.
                //
                while ((pDW <= pPosDW) && (*pDW <= MIN_DW))
                {
                    pDW++;
                }
                if ((cbDest - PosCtr) <= (pPosDW - pDW + 1))
                {
                    NLS_FREE_TMP_BUFFER(pUW, pBuffer);
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    return (0);
                }
                while (pPosDW >= pDW)
                {
                    pDest[PosCtr] = *pPosDW;
                    PosCtr++;
                    pPosDW--;
                }
            }
            else
            {
                //
                //  Regular diacritics:
                //    - remove diacritics from right to left.
                //    - store  diacritics from left  to right.
                //
                while ((pPosDW >= pDW) && (*pPosDW <= MIN_DW))
                {
                    pPosDW--;
                }
                if ((cbDest - PosCtr) <= (pPosDW - pDW + 1))
                {
                    NLS_FREE_TMP_BUFFER(pUW, pBuffer);
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    return (0);
                }
                while (pDW <= pPosDW)
                {
                    pDest[PosCtr] = *pDW;
                    PosCtr++;
                    pDW++;
                }
            }
        }

        //
        //  Copy Separator to destination buffer if the destination
        //  buffer is large enough.
        //
        if (PosCtr == cbDest)
        {
            NLS_FREE_TMP_BUFFER(pUW, pBuffer);
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return (0);
        }
        pDest[PosCtr] = SORTKEY_SEPARATOR;
        PosCtr++;

        //
        //  Store the Case Weights in the destination buffer.
        //
        //    - Eliminate minimum CW.
        //    - Make sure destination buffer is large enough.
        //    - Copy case weights to destination buffer.
        //
        if ((dwFlags & NORM_DROP_CW) != NORM_DROP_CW)
        {
            pPosCW--;
            while ((pPosCW >= pCW) && (*pPosCW <= MIN_CW))
            {
                pPosCW--;
            }
            if ((cbDest - PosCtr) <= (pPosCW - pCW + 1))
            {
                NLS_FREE_TMP_BUFFER(pUW, pBuffer);
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return (0);
            }
            while (pCW <= pPosCW)
            {
                pDest[PosCtr] = *pCW;
                PosCtr++;
                pCW++;
            }
        }

        //
        //  Copy Separator to destination buffer if the destination
        //  buffer is large enough.
        //
        if (PosCtr == cbDest)
        {
            NLS_FREE_TMP_BUFFER(pUW, pBuffer);
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return (0);
        }
        pDest[PosCtr] = SORTKEY_SEPARATOR;
        PosCtr++;

        //
        //  Store the Extra Weights in the destination buffer.
        //
        //    - Eliminate unnecessary XW.
        //    - Make sure destination buffer is large enough.
        //    - Copy extra weights to destination buffer.
        //
        if (pXW < pPosXW)
        {
            if (dwFlags & NORM_IGNORENONSPACE)
            {
                //
                //  Ignore 4W and 5W.  Must count separators for
                //  4W and 5W, though.
                //
                if ((cbDest - PosCtr) <= 2)
                {
                    NLS_FREE_TMP_BUFFER(pUW, pBuffer);
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    return (0);
                }

                pDest[PosCtr] = pXWSeparator[0];
                pDest[PosCtr + 1] = pXWSeparator[1];
                PosCtr += 2;
                ctr = 2;
            }
            else
            {
                ctr = 0;
            }
            pPosXW--;
            for (; ctr < NUM_BYTES_XW; ctr++)
            {
                pTmp = pXW + (WeightLen * ctr);
                pPosTmp = pPosXW + (WeightLen * ctr);
                while ((pPosTmp >= pTmp) && (*pPosTmp == pXWDrop[ctr]))
                {
                    pPosTmp--;
                }
                if ((cbDest - PosCtr) <= (pPosTmp - pTmp + 1))
                {
                    NLS_FREE_TMP_BUFFER(pUW, pBuffer);
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    return (0);
                }
                while (pTmp <= pPosTmp)
                {
                    pDest[PosCtr] = *pTmp;
                    PosCtr++;
                    pTmp++;
                }

                //
                //  Copy Separator to destination buffer.
                //
                pDest[PosCtr] = pXWSeparator[ctr];
                PosCtr++;
            }
        }

        //
        //  Copy Separator to destination buffer if the destination
        //  buffer is large enough.
        //
        if (PosCtr == cbDest)
        {
            NLS_FREE_TMP_BUFFER(pUW, pBuffer);
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return (0);
        }
        pDest[PosCtr] = SORTKEY_SEPARATOR;
        PosCtr++;

        //
        //  Store the Special Weights in the destination buffer.
        //
        //    - Make sure destination buffer is large enough.
        //    - Copy special weights to destination buffer.
        //
        if (!fIgnoreSymbols)
        {
            if ((cbDest - PosCtr) <= (((LPBYTE)pPosSW - (LPBYTE)pSW)))
            {
                NLS_FREE_TMP_BUFFER(pUW, pBuffer);
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return (0);
            }
            pTmp = (LPBYTE)pSW;
            while (pTmp < (LPBYTE)pPosSW)
            {
                pDest[PosCtr]     = *pTmp;
                pDest[PosCtr + 1] = *(pTmp + 1);

                //
                //  NOTE:  Special Weight is stored in the data file as
                //             Weight, Script
                //         so that the WORD value will be read correctly.
                //
                pDest[PosCtr + 2] = *(pTmp + 3);
                pDest[PosCtr + 3] = *(pTmp + 2);

                PosCtr += 4;
                pTmp += 4;
            }
        }

        //
        //  Copy Terminator to destination buffer if the destination
        //  buffer is large enough.
        //
        if (PosCtr == cbDest)
        {
            NLS_FREE_TMP_BUFFER(pUW, pBuffer);
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return (0);
        }
        pDest[PosCtr] = SORTKEY_TERMINATOR;
        PosCtr++;
    }

    //
    //  Free the buffer used for the weights, if one was allocated.
    //
    NLS_FREE_TMP_BUFFER(pUW, pBuffer);

    //
    //  Return number of BYTES written to destination buffer.
    //
    return (PosCtr);
}


////////////////////////////////////////////////////////////////////////////
//
//  MapNormalization
//
//  Stores the result of the normalization for the given string in the
//  destination buffer, and returns the number of wide characters written
//  to the buffer.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int MapNormalization(
    PLOC_HASH pHashN,
    DWORD dwFlags,
    LPCWSTR pSrc,
    int cchSrc,
    LPWSTR pDest,
    int cchDest)
{
    int ctr;                      // source char counter
    int ctr2 = 0;                 // destination char counter

    //
    //  Make sure the ctype table is available in the system.
    //
    if (GetCTypeFileInfo())
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return (0);
    }

    //
    //  Normalize based on the flags.
    //
    switch (dwFlags)
    {
        case ( NORM_IGNORENONSPACE ) :
        {
            //
            //  If the destination value is zero, then only return
            //  the count of characters.  Do NOT touch pDest.
            //
            if (cchDest == 0)
            {
                //
                //  Count the number of characters that would be written
                //  to the destination buffer.
                //
                for (ctr = 0, ctr2 = 0; ctr < cchSrc; ctr++)
                {
                    if (!IS_NONSPACE(pHashN->pSortkey, pSrc[ctr]))
                    {
                        //
                        //  Not a nonspacing character, so just write the
                        //  character to the destination string.
                        //
                        ctr2++;
                    }
                    else if (!(IS_NONSPACE_ONLY(pHashN->pSortkey, pSrc[ctr])))
                    {
                        //
                        //  PreComposed Form.  Write the base character only.
                        //
                        ctr2++;
                    }
                    //
                    //  Else - nonspace character only, so don't write
                    //         anything.
                    //
                }
            }
            else
            {
                //
                //  Store the normalized string in the destination string.
                //
                for (ctr = 0, ctr2 = 0; (ctr < cchSrc) && (ctr2 < cchDest);
                     ctr++)
                {
                    if (!IS_NONSPACE(pHashN->pSortkey, pSrc[ctr]))
                    {
                        //
                        //  Not a nonspacing character, so just write the
                        //  character to the destination string.
                        //
                        pDest[ctr2] = pSrc[ctr];
                        ctr2++;
                    }
                    else if (!(IS_NONSPACE_ONLY(pHashN->pSortkey, pSrc[ctr])))
                    {
                        //
                        //  PreComposed Form.  Write the base character only.
                        //
                        GET_BASE_CHAR(pSrc[ctr], pDest[ctr2]);
                        if (pDest[ctr2] == 0)
                        {
                            //
                            //  No translation for precomposed character,
                            //  so must write the precomposed character.
                            //
                            pDest[ctr2] = pSrc[ctr];
                        }
                        ctr2++;
                    }
                    //
                    //  Else - nonspace character only, so don't write
                    //         anything.
                    //
                }
            }

            break;
        }

        case ( NORM_IGNORESYMBOLS ) :
        {
            //
            //  If the destination value is zero, then only return
            //  the count of characters.  Do NOT touch pDest.
            //
            if (cchDest == 0)
            {
                //
                //  Count the number of characters that would be written
                //  to the destination buffer.
                //
                for (ctr = 0, ctr2 = 0; ctr < cchSrc; ctr++)
                {
                    if (!IS_SYMBOL(pHashN->pSortkey, pSrc[ctr]))
                    {
                        //
                        //  Not a symbol, so write the character.
                        //
                        ctr2++;
                    }
                }
            }
            else
            {
                //
                //  Store the normalized string in the destination string.
                //
                for (ctr = 0, ctr2 = 0; (ctr < cchSrc) && (ctr2 < cchDest);
                     ctr++)
                {
                    if (!IS_SYMBOL(pHashN->pSortkey, pSrc[ctr]))
                    {
                        //
                        //  Not a symbol, so write the character.
                        //
                        pDest[ctr2] = pSrc[ctr];
                        ctr2++;
                    }
                }
            }

            break;
        }

        case ( NORM_IGNORENONSPACE | NORM_IGNORESYMBOLS ) :
        {
            //
            //  If the destination value is zero, then only return
            //  the count of characters.  Do NOT touch pDest.
            //
            if (cchDest == 0)
            {
                //
                //  Count the number of characters that would be written
                //  to the destination buffer.
                //
                for (ctr = 0, ctr2 = 0; ctr < cchSrc; ctr++)
                {
                    if (!IS_SYMBOL(pHashN->pSortkey, pSrc[ctr]))
                    {
                        //
                        //  Not a symbol, so check for nonspace.
                        //
                        if (!IS_NONSPACE(pHashN->pSortkey, pSrc[ctr]))
                        {
                            //
                            //  Not a nonspacing character, so just write the
                            //  character to the destination string.
                            //
                            ctr2++;
                        }
                        else if (!(IS_NONSPACE_ONLY( pHashN->pSortkey,
                                                     pSrc[ctr] )))
                        {
                            //
                            //  PreComposed Form.  Write the base character
                            //  only.
                            //
                            ctr2++;
                        }
                        //
                        //  Else - nonspace character only, so don't write
                        //         anything.
                        //
                    }
                }
            }
            else
            {
                //
                //  Store the normalized string in the destination string.
                //
                for (ctr = 0, ctr2 = 0; (ctr < cchSrc) && (ctr2 < cchDest);
                     ctr++)
                {
                    //
                    //  Check for symbol and nonspace.
                    //
                    if (!IS_SYMBOL(pHashN->pSortkey, pSrc[ctr]))
                    {
                        //
                        //  Not a symbol, so check for nonspace.
                        //
                        if (!IS_NONSPACE(pHashN->pSortkey, pSrc[ctr]))
                        {
                            //
                            //  Not a nonspacing character, so just write the
                            //  character to the destination string.
                            //
                            pDest[ctr2] = pSrc[ctr];
                            ctr2++;
                        }
                        else if (!(IS_NONSPACE_ONLY( pHashN->pSortkey,
                                                     pSrc[ctr] )))
                        {
                            //
                            //  PreComposed Form.  Write the base character
                            //  only.
                            //
                            GET_BASE_CHAR(pSrc[ctr], pDest[ctr2]);
                            if (pDest[ctr2] == 0)
                            {
                                //
                                //  No translation for precomposed character,
                                //  so must write the precomposed character.
                                //
                                pDest[ctr2] = pSrc[ctr];
                            }
                            ctr2++;
                        }
                        //
                        //  Else - nonspace character only, so don't write
                        //         anything.
                        //
                    }
                }
            }

            break;
        }
    }

    //
    //  Return the number of wide characters written.
    //
    return (ctr2);
}


////////////////////////////////////////////////////////////////////////////
//
//  MapKanaWidth
//
//  Stores the result of the Kana, Width, and/or Casing mappings for the
//  given string in the destination buffer, and returns the number of wide
//  characters written to the buffer.
//
//  07-26-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int MapKanaWidth(
    PLOC_HASH pHashN,
    DWORD dwFlags,
    LPCWSTR pSrc,
    int cchSrc,
    LPWSTR pDest,
    int cchDest)
{
    int ctr;                 // loop counter
    PCASE pCase;             // ptr to case table (if case flag is set)


    //
    //  See if lower or upper case flags are present.
    //
    if (dwFlags & LCMAP_LOWERCASE)
    {
        pCase = (dwFlags & LCMAP_LINGUISTIC_CASING)
                    ? pHashN->pLowerLinguist
                    : pHashN->pLowerCase;
    }
    else if (dwFlags & LCMAP_UPPERCASE)
    {
        pCase = (dwFlags & LCMAP_LINGUISTIC_CASING)
                    ? pHashN->pUpperLinguist
                    : pHashN->pUpperCase;
    }
    else
    {
        pCase = NULL;
    }

    //
    //  Remove lower, upper, and linguistic casing flags.
    //
    dwFlags &= ~(LCMAP_LOWERCASE | LCMAP_UPPERCASE | LCMAP_LINGUISTIC_CASING);

    //
    //  Map the string based on the given flags.
    //
    switch (dwFlags)
    {
        case ( LCMAP_HIRAGANA ) :
        case ( LCMAP_KATAKANA ) :
        {
            //
            //  If the destination value is zero, then just return the
            //  length of the source string.  Do NOT touch pDest.
            //
            if (cchDest == 0)
            {
                return (cchSrc);
            }

            //
            //  If cchSrc is greater than cchDest, then the destination
            //  buffer is too small to hold the string.  Return an error.
            //
            if (cchSrc > cchDest)
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return (0);
            }

            if (dwFlags == LCMAP_HIRAGANA)
            {
                //
                //  Map all Katakana full width to Hiragana full width.
                //  Katakana half width will remain Katakana half width.
                //
                if (pCase)
                {
                    for (ctr = 0; ctr < cchSrc; ctr++)
                    {
                        pDest[ctr] = GET_KANA(pTblPtrs->pHiragana, pSrc[ctr]);

                        pDest[ctr] = GET_LOWER_UPPER_CASE(pCase, pDest[ctr]);
                    }
                }
                else
                {
                    for (ctr = 0; ctr < cchSrc; ctr++)
                    {
                        pDest[ctr] = GET_KANA(pTblPtrs->pHiragana, pSrc[ctr]);
                    }
                }
            }
            else
            {
                //
                //  Map all Hiragana full width to Katakana full width.
                //  Hiragana half width does not exist.
                //
                if (pCase)
                {
                    for (ctr = 0; ctr < cchSrc; ctr++)
                    {
                        pDest[ctr] = GET_KANA(pTblPtrs->pKatakana, pSrc[ctr]);

                        pDest[ctr] = GET_LOWER_UPPER_CASE(pCase, pDest[ctr]);
                    }
                }
                else
                {
                    for (ctr = 0; ctr < cchSrc; ctr++)
                    {
                        pDest[ctr] = GET_KANA(pTblPtrs->pKatakana, pSrc[ctr]);
                    }
                }
            }

            //
            //  Return the number of characters mapped.
            //
            return (cchSrc);

            break;
        }
        case ( LCMAP_HALFWIDTH ) :
        {
            //
            //  Map all chars to half width.
            //
            return (MapHalfKana( pSrc,
                                 cchSrc,
                                 pDest,
                                 cchDest,
                                 NULL,
                                 pCase ));

            break;
        }
        case ( LCMAP_FULLWIDTH ) :
        {
            //
            //  Map all chars to full width.
            //
            return (MapFullKana( pSrc,
                                 cchSrc,
                                 pDest,
                                 cchDest,
                                 NULL,
                                 pCase ));

            break;
        }
        case ( LCMAP_HIRAGANA | LCMAP_HALFWIDTH ) :
        {
            //
            //  This combination of flags is strange, because
            //  Hiragana is only full width.  So, the Hiragana flag
            //  is the most important.  Full width Katakana will be
            //  mapped to full width Hiragana, not half width
            //  Katakana.
            //
            //  Map to Hiragana, then Half Width.
            //
            return (MapHalfKana( pSrc,
                                 cchSrc,
                                 pDest,
                                 cchDest,
                                 pTblPtrs->pHiragana,
                                 pCase ));

            break;
        }
        case ( LCMAP_HIRAGANA | LCMAP_FULLWIDTH ) :
        {
            //
            //  Since Hiragana is only FULL width, the mapping to
            //  width must be done first to convert all half width
            //  Katakana to full width Katakana before trying to
            //  map to Hiragana.
            //
            //  Map to Full Width, then Hiragana.
            //
            return (MapFullKana( pSrc,
                                 cchSrc,
                                 pDest,
                                 cchDest,
                                 pTblPtrs->pHiragana,
                                 pCase ));

            break;
        }
        case ( LCMAP_KATAKANA | LCMAP_HALFWIDTH ) :
        {
            //
            //  Since Hiragana is only FULL width, the mapping to
            //  Katakana must be done first to convert all Hiragana
            //  to Katakana before trying to map to half width.
            //
            //  Map to Katakana, then Half Width.
            //
            return (MapHalfKana( pSrc,
                                 cchSrc,
                                 pDest,
                                 cchDest,
                                 pTblPtrs->pKatakana,
                                 pCase ));

            break;
        }
        case ( LCMAP_KATAKANA | LCMAP_FULLWIDTH ) :
        {
            //
            //  Since Hiragana is only FULL width, it doesn't matter
            //  which way the mapping is done for this combination.
            //
            //  Map to Full Width, then Katakana.
            //
            return (MapFullKana( pSrc,
                                 cchSrc,
                                 pDest,
                                 cchDest,
                                 pTblPtrs->pKatakana,
                                 pCase ));

            break;
        }
        default :
        {
            //
            //  Return error.
            //
            return (0);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  MapHalfKana
//
//  Stores the result of the half width and Kana mapping for the given string
//  in the destination buffer, and returns the number of wide characters
//  written to the buffer.
//
//  This first converts the precomposed characters to their composite forms,
//  and then maps all characters to their half width forms.  This handles the
//  case where the full width precomposed form should map to TWO half width
//  code points (composite form).  The half width precomposed forms do not
//  exist in Unicode.
//
//  11-04-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int MapHalfKana(
    LPCWSTR pSrc,
    int cchSrc,
    LPWSTR pDest,
    int cchDest,
    PKANA pKana,
    PCASE pCase)
{
    int Count;                    // count of characters written
    int ctr = 0;                  // loop counter
    int ct;                       // loop counter
    LPWSTR pBuf;                  // ptr to destination buffer
    LPWSTR pEndBuf;               // ptr to end of destination buffer
    LPWSTR pPosDest;              // ptr to position in destination buffer
    LPWSTR *ppIncr;               // points to ptr to increment
    WCHAR pTmp[MAX_COMPOSITE];    // ptr to temporary buffer
    LPWSTR pEndTmp;               // ptr to end of temporary buffer


    //
    //  Initialize the destination pointers.
    //
    pEndTmp = pTmp + MAX_COMPOSITE;
    if (cchDest == 0)
    {
        //
        //  Do not touch the pDest pointer.  Use the pTmp buffer and
        //  initialize the end pointer.
        //
        pBuf = pTmp;
        pEndBuf = pEndTmp;

        //
        //  This is a bogus pointer and will never be touched.  It just
        //  increments this pointer into oblivion.
        //
        pDest = pBuf;
        ppIncr = &pDest;
    }
    else
    {
        //
        //  Initialize the pointers.  Use the pDest buffer.
        //
        pBuf = pDest;
        pEndBuf = pBuf + cchDest;
        ppIncr = &pBuf;
    }

    //
    //  Search through the source string.  Convert all precomposed
    //  forms to their composite form before converting to half width.
    //
    while ((ctr < cchSrc) && (pBuf < pEndBuf))
    {
        //
        //  Get the character to convert.  If we need to convert to
        //  kana, do it.
        //
        if (pKana)
        {
            *pTmp = GET_KANA(pKana, pSrc[ctr]);
        }
        else
        {
            *pTmp = pSrc[ctr];
        }

        //
        //  Convert to its composite form (if exists).
        //
        //  NOTE: Must use the tmp buffer in case the destination buffer
        //        isn't large enough to hold the composite form.
        //
        Count = InsertCompositeForm(pTmp, pEndTmp);

        //
        //  Convert to half width (if exists) and case (if appropriate).
        //
        pPosDest = pTmp;
        if (pCase)
        {
            for (ct = Count; ct > 0; ct--)
            {
                *pPosDest = GET_HALF_WIDTH(pTblPtrs->pHalfWidth, *pPosDest);

                *pPosDest = GET_LOWER_UPPER_CASE(pCase, *pPosDest);

                pPosDest++;
            }
        }
        else
        {
            for (ct = Count; ct > 0; ct--)
            {
                *pPosDest = GET_HALF_WIDTH(pTblPtrs->pHalfWidth, *pPosDest);
                pPosDest++;
            }
        }

        //
        //  Convert back to its precomposed form (if exists).
        //
        if (Count > 1)
        {
            //
            //  Get the precomposed form.
            //
            //  ct is the number of code points used from the
            //  composite form.
            //
            ct = InsertPreComposedForm(pTmp, pPosDest, pBuf);
            if (ct > 1)
            {
                //
                //  Precomposed form was found.  Need to make sure all
                //  of the composite chars were used.
                //
                if (ct == Count)
                {
                    //
                    //  All composite chars were used.  Increment by 1.
                    //
                    (*ppIncr)++;
                }
                else
                {
                    //
                    //  Not all composite chars were used.  Need to copy
                    //  the rest of the composite chars from the tmp buffer
                    //  to the destination buffer.
                    //
                    (*ppIncr)++;
                    Count -= ct;
                    if (pBuf + Count > pEndBuf)
                    {
                        break;
                    }
                    RtlMoveMemory(pBuf, pTmp + ct, Count * sizeof(WCHAR));
                    (*ppIncr) += Count;
                }
            }
            else
            {
                //
                //  Precomposed form was NOT found.  Need to copy the
                //  composite form from the tmp buffer to the destination
                //  buffer.
                //
                if (pBuf + Count > pEndBuf)
                {
                    break;
                }
                RtlMoveMemory(pBuf, pTmp, Count * sizeof(WCHAR));
                (*ppIncr) += Count;
            }
        }
        else
        {
            //
            //  Only one character (no composite form), so just copy it
            //  from the tmp buffer to the destination buffer.
            //
            *pBuf = *pTmp;
            (*ppIncr)++;
        }

        ctr++;
    }

    //
    //  Return the appropriate number of characters.
    //
    if (cchDest == 0)
    {
        //
        //  Return the number of characters written to the buffer.
        //
        return ((int)((*ppIncr) - pTmp));
    }
    else
    {
        //
        //  Make sure the given buffer was large enough to hold the
        //  mapping.
        //
        if (ctr < cchSrc)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return (0);
        }

        //
        //  Return the number of characters written to the buffer.
        //
        return ((int)((*ppIncr) - pDest));
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  MapFullKana
//
//  Stores the result of the full width and Kana mapping for the given string
//  in the destination buffer, and returns the number of wide characters
//  written to the buffer.
//
//  This first converts the characters to full width, and then maps all
//  composite characters to their precomposed forms.  This handles the case
//  where the half width composite form (TWO code points) should map to a
//  full width precomposed form (ONE full width code point).  The half
//  width precomposed forms do not exist in Unicode and we need the full
//  width precomposed forms to round trip with the TWO half width code
//  points.
//
//  11-04-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int MapFullKana(
    LPCWSTR pSrc,
    int cchSrc,
    LPWSTR pDest,
    int cchDest,
    PKANA pKana,
    PCASE pCase)
{
    int Count;                    // count of characters
    LPWSTR pPosSrc;               // ptr to position in source buffer
    LPWSTR pEndSrc;               // ptr to end of source buffer
    LPWSTR pBuf;                  // ptr to destination buffer
    LPWSTR pEndBuf;               // ptr to end of destination buffer
    LPWSTR *ppIncr;               // points to ptr to increment
    WCHAR pTmp[MAX_COMPOSITE];    // ptr to temporary buffer


    //
    //  Initialize source string pointers.
    //
    pPosSrc = (LPWSTR)pSrc;
    pEndSrc = pPosSrc + cchSrc;

    //
    //  Initialize the destination pointers.
    //
    if (cchDest == 0)
    {
        //
        //  Do not touch the pDest pointer.  Use the pTmp buffer and
        //  initialize the end pointer.
        //
        pBuf = pTmp;
        pEndBuf = pTmp + MAX_COMPOSITE;

        //
        //  This is a bogus pointer and will never be touched.  It just
        //  increments this pointer into oblivion.
        //
        pDest = pBuf;
        ppIncr = &pDest;
    }
    else
    {
        //
        //  Initialize the pointers.  Use the pDest buffer.
        //
        pBuf = pDest;
        pEndBuf = pBuf + cchDest;
        ppIncr = &pBuf;
    }

    //
    //  Search through the source string.  Convert all composite
    //  forms to their precomposed form before converting to full width.
    //
    while ((pPosSrc < pEndSrc) && (pBuf < pEndBuf))
    {
        //
        //  Convert a composite form to its full width precomposed
        //  form (if exists).  Also, convert to case if necessary.
        //
        Count = InsertFullWidthPreComposedForm( pPosSrc,
                                                pEndSrc,
                                                pBuf,
                                                pCase );
        pPosSrc += Count;

        //
        //  Convert to kana if necessary.
        //
        if (pKana)
        {
            *pBuf = GET_KANA(pKana, *pBuf);
        }

        //
        //  Increment the destination pointer.
        //
        (*ppIncr)++;
    }

    //
    //  Return the appropriate number of characters.
    //
    if (cchDest == 0)
    {
        //
        //  Return the number of characters written to the buffer.
        //
        return ((int)((*ppIncr) - pTmp));
    }
    else
    {
        //
        //  Make sure the given buffer was large enough to hold the
        //  mapping.
        //
        if (pPosSrc < pEndSrc)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return (0);
        }

        //
        //  Return the number of characters written to the buffer.
        //
        return ((int)((*ppIncr) - pDest));
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  MapTraditionalSimplified
//
//  Stores the appropriate Traditional or Simplified Chinese values in the
//  destination buffer, and returns the number of wide characters
//  written to the buffer.
//
//  05-07-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int MapTraditionalSimplified(
    PLOC_HASH pHashN,
    DWORD dwFlags,
    LPCWSTR pSrc,
    int cchSrc,
    LPWSTR pDest,
    int cchDest,
    PCHINESE pChinese)
{
    int ctr;                 // loop counter
    PCASE pCase;             // ptr to case table (if case flag is set)


    //
    //  If the destination value is zero, then just return the
    //  length of the source string.  Do NOT touch pDest.
    //
    if (cchDest == 0)
    {
        return (cchSrc);
    }

    //
    //  If cchSrc is greater than cchDest, then the destination buffer
    //  is too small to hold the new string.  Return an error.
    //
    if (cchSrc > cchDest)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return (0);
    }

    //
    //  See if lower or upper case flags are present.
    //
    if (dwFlags & LCMAP_LOWERCASE)
    {
        pCase = (dwFlags & LCMAP_LINGUISTIC_CASING)
                    ? pHashN->pLowerLinguist
                    : pHashN->pLowerCase;
    }
    else if (dwFlags & LCMAP_UPPERCASE)
    {
        pCase = (dwFlags & LCMAP_LINGUISTIC_CASING)
                    ? pHashN->pUpperLinguist
                    : pHashN->pUpperCase;
    }
    else
    {
        pCase = NULL;
    }

    //
    //  Map to Traditional/Simplified and store it in the destination string.
    //  Also map the case, if appropriate.
    //
    if (pCase)
    {
        for (ctr = 0; ctr < cchSrc; ctr++)
        {
            pDest[ctr] = GET_CHINESE(pChinese, pSrc[ctr]);

            pDest[ctr] = GET_LOWER_UPPER_CASE(pCase, pDest[ctr]);
        }
    }
    else
    {
        for (ctr = 0; ctr < cchSrc; ctr++)
        {
            pDest[ctr] = GET_CHINESE(pChinese, pSrc[ctr]);
        }
    }

    //
    //  Return the number of wide characters written.
    //
    return (ctr);
}
