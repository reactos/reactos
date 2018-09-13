/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    string.c

Abstract:

    This file contains functions that deal with characters and strings.

    APIs found in this file:
      CompareStringW
      GetStringTypeExW
      GetStringTypeW

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
//  State Table.
//
#define STATE_DW                  1    // normal diacritic weight state
#define STATE_REVERSE_DW          2    // reverse diacritic weight state
#define STATE_CW                  4    // case weight state


//
//  Invalid weight value.
//
#define CMP_INVALID_WEIGHT        0xffffffff
#define CMP_INVALID_FAREAST       0xffff0000
#define CMP_INVALID_UW            0xffff




//
//  Forward Declarations.
//
int
LongCompareStringW(
    PLOC_HASH pHashN,
    DWORD dwCmpFlags,
    LPCWSTR lpString1,
    int cchCount1,
    LPCWSTR lpString2,
    int cchCount2,
    BOOL fModify);





//-------------------------------------------------------------------------//
//                           INTERNAL MACROS                               //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  NOT_END_STRING
//
//  Checks to see if the search has reached the end of the string.
//  It returns TRUE if the counter is not at zero (counting backwards) and
//  the null termination has not been reached (if -1 was passed in the count
//  parameter.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define NOT_END_STRING(ct, ptr, cchIn)                                     \
    ((ct != 0) && (!((*(ptr) == 0) && (cchIn == -2))))


////////////////////////////////////////////////////////////////////////////
//
//  AT_STRING_END
//
//  Checks to see if the pointer is at the end of the string.
//  It returns TRUE if the counter is zero or if the null termination
//  has been reached (if -2 was passed in the count parameter).
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define AT_STRING_END(ct, ptr, cchIn)                                      \
    ((ct == 0) || ((*(ptr) == 0) && (cchIn == -2)))


////////////////////////////////////////////////////////////////////////////
//
//  REMOVE_STATE
//
//  Removes the current state from the state table.  This should only be
//  called when the current state should not be entered for the remainder
//  of the comparison.  It decrements the counter going through the state
//  table and decrements the number of states in the table.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define REMOVE_STATE(value)            (State &= ~value)


////////////////////////////////////////////////////////////////////////////
//
//  POINTER_FIXUP
//
//  Fixup the string pointers if expansion characters were found.
//  Then, advance the string pointers and decrement the string counters.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define POINTER_FIXUP()                                                    \
{                                                                          \
    /*                                                                     \
     *  Fixup the pointers (if necessary).                                 \
     */                                                                    \
    if (pSave1 && (--cExpChar1 == 0))                                      \
    {                                                                      \
        /*                                                                 \
         *  Done using expansion temporary buffer.                         \
         */                                                                \
        pString1 = pSave1;                                                 \
        pSave1 = NULL;                                                     \
    }                                                                      \
                                                                           \
    if (pSave2 && (--cExpChar2 == 0))                                      \
    {                                                                      \
        /*                                                                 \
         *  Done using expansion temporary buffer.                         \
         */                                                                \
        pString2 = pSave2;                                                 \
        pSave2 = NULL;                                                     \
    }                                                                      \
                                                                           \
    /*                                                                     \
     *  Advance the string pointers.                                       \
     */                                                                    \
    pString1++;                                                            \
    pString2++;                                                            \
}


////////////////////////////////////////////////////////////////////////////
//
//  SCAN_LONGER_STRING
//
//  Scans the longer string for diacritic, case, and special weights.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define SCAN_LONGER_STRING( ct,                                            \
                            ptr,                                           \
                            cchIn,                                         \
                            ret )                                          \
{                                                                          \
    /*                                                                     \
     *  Search through the rest of the longer string to make sure          \
     *  all characters are not to be ignored.  If find a character that    \
     *  should not be ignored, return the given return value immediately.  \
     *                                                                     \
     *  The only exception to this is when a nonspace mark is found.  If   \
     *  another DW difference has been found earlier, then use that.       \
     */                                                                    \
    while (NOT_END_STRING(ct, ptr, cchIn))                                 \
    {                                                                      \
        Weight1 = GET_DWORD_WEIGHT(pHashN, *ptr);                          \
        switch (GET_SCRIPT_MEMBER(&Weight1))                               \
        {                                                                  \
            case ( UNSORTABLE ):                                           \
            {                                                              \
                break;                                                     \
            }                                                              \
            case ( NONSPACE_MARK ):                                        \
            {                                                              \
                if ((!fIgnoreDiacritic) && (!WhichDiacritic))              \
                {                                                          \
                    return (ret);                                          \
                }                                                          \
                break;                                                     \
            }                                                              \
            case ( PUNCTUATION ) :                                         \
            case ( SYMBOL_1 ) :                                            \
            case ( SYMBOL_2 ) :                                            \
            case ( SYMBOL_3 ) :                                            \
            case ( SYMBOL_4 ) :                                            \
            case ( SYMBOL_5 ) :                                            \
            {                                                              \
                if (!fIgnoreSymbol)                                        \
                {                                                          \
                    return (ret);                                          \
                }                                                          \
                break;                                                     \
            }                                                              \
            case ( EXPANSION ) :                                           \
            case ( FAREAST_SPECIAL ) :                                     \
            default :                                                      \
            {                                                              \
                return (ret);                                              \
            }                                                              \
            case ( RESERVED_2 ) :                                          \
            case ( RESERVED_3 ) :                                          \
            {                                                              \
                /*                                                         \
                 *  Fill out the case statement so the compiler            \
                 *  will use a jump table.                                 \
                 */                                                        \
                break;                                                     \
            }                                                              \
        }                                                                  \
                                                                           \
        /*                                                                 \
         *  Advance pointer and decrement counter.                         \
         */                                                                \
        ptr++;                                                             \
        ct--;                                                              \
    }                                                                      \
                                                                           \
    /*                                                                     \
     *  Need to check diacritic, case, extra, and special weights for      \
     *  final return value.  Still could be equal if the longer part of    \
     *  the string contained only characters to be ignored.                \
     *                                                                     \
     *  NOTE:  The following checks MUST REMAIN IN THIS ORDER:             \
     *            Diacritic, Case, Extra, Punctuation.                     \
     */                                                                    \
    if (WhichDiacritic)                                                    \
    {                                                                      \
        return (WhichDiacritic);                                           \
    }                                                                      \
    if (WhichCase)                                                         \
    {                                                                      \
        return (WhichCase);                                                \
    }                                                                      \
    if (WhichExtra)                                                        \
    {                                                                      \
        if (!fIgnoreDiacritic)                                             \
        {                                                                  \
            if (GET_WT_FOUR(&WhichExtra))                                  \
            {                                                              \
                return (GET_WT_FOUR(&WhichExtra));                         \
            }                                                              \
            if (GET_WT_FIVE(&WhichExtra))                                  \
            {                                                              \
                return (GET_WT_FIVE(&WhichExtra));                         \
            }                                                              \
        }                                                                  \
        if (GET_WT_SIX(&WhichExtra))                                       \
        {                                                                  \
            return (GET_WT_SIX(&WhichExtra));                              \
        }                                                                  \
        if (GET_WT_SEVEN(&WhichExtra))                                     \
        {                                                                  \
            return (GET_WT_SEVEN(&WhichExtra));                            \
        }                                                                  \
    }                                                                      \
    if (WhichPunct1)                                                       \
    {                                                                      \
        return (WhichPunct1);                                              \
    }                                                                      \
    if (WhichPunct2)                                                       \
    {                                                                      \
        return (WhichPunct2);                                              \
    }                                                                      \
                                                                           \
    return (CSTR_EQUAL);                                                   \
}


////////////////////////////////////////////////////////////////////////////
//
//  QUICK_SCAN_LONGER_STRING
//
//  Scans the longer string for diacritic, case, and special weights.
//  Assumes that both strings are null-terminated.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define QUICK_SCAN_LONGER_STRING( ptr,                                     \
                                  ret )                                    \
{                                                                          \
    /*                                                                     \
     *  Search through the rest of the longer string to make sure          \
     *  all characters are not to be ignored.  If find a character that    \
     *  should not be ignored, return the given return value immediately.  \
     *                                                                     \
     *  The only exception to this is when a nonspace mark is found.  If   \
     *  another DW difference has been found earlier, then use that.       \
     */                                                                    \
    while (*ptr != 0)                                                      \
    {                                                                      \
        switch (GET_SCRIPT_MEMBER(&(pHashN->pSortkey[*ptr])))              \
        {                                                                  \
            case ( UNSORTABLE ):                                           \
            {                                                              \
                break;                                                     \
            }                                                              \
            case ( NONSPACE_MARK ):                                        \
            {                                                              \
                if (!WhichDiacritic)                                       \
                {                                                          \
                    return (ret);                                          \
                }                                                          \
                break;                                                     \
            }                                                              \
            default :                                                      \
            {                                                              \
                return (ret);                                              \
            }                                                              \
        }                                                                  \
                                                                           \
        /*                                                                 \
         *  Advance pointer.                                               \
         */                                                                \
        ptr++;                                                             \
    }                                                                      \
                                                                           \
    /*                                                                     \
     *  Need to check diacritic, case, extra, and special weights for      \
     *  final return value.  Still could be equal if the longer part of    \
     *  the string contained only unsortable characters.                   \
     *                                                                     \
     *  NOTE:  The following checks MUST REMAIN IN THIS ORDER:             \
     *            Diacritic, Case, Extra, Punctuation.                     \
     */                                                                    \
    if (WhichDiacritic)                                                    \
    {                                                                      \
        return (WhichDiacritic);                                           \
    }                                                                      \
    if (WhichCase)                                                         \
    {                                                                      \
        return (WhichCase);                                                \
    }                                                                      \
    if (WhichExtra)                                                        \
    {                                                                      \
        if (GET_WT_FOUR(&WhichExtra))                                      \
        {                                                                  \
            return (GET_WT_FOUR(&WhichExtra));                             \
        }                                                                  \
        if (GET_WT_FIVE(&WhichExtra))                                      \
        {                                                                  \
            return (GET_WT_FIVE(&WhichExtra));                             \
        }                                                                  \
        if (GET_WT_SIX(&WhichExtra))                                       \
        {                                                                  \
            return (GET_WT_SIX(&WhichExtra));                              \
        }                                                                  \
        if (GET_WT_SEVEN(&WhichExtra))                                     \
        {                                                                  \
            return (GET_WT_SEVEN(&WhichExtra));                            \
        }                                                                  \
    }                                                                      \
    if (WhichPunct1)                                                       \
    {                                                                      \
        return (WhichPunct1);                                              \
    }                                                                      \
    if (WhichPunct2)                                                       \
    {                                                                      \
        return (WhichPunct2);                                              \
    }                                                                      \
                                                                           \
    return (CSTR_EQUAL);                                                   \
}


////////////////////////////////////////////////////////////////////////////
//
//  GET_FAREAST_WEIGHT
//
//  Returns the weight for the far east special case in "wt".  This currently
//  includes the Cho-on, the Repeat, and the Kana characters.
//
//  08-19-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define GET_FAREAST_WEIGHT( wt,                                            \
                            uw,                                            \
                            mask,                                          \
                            pBegin,                                        \
                            pCur,                                          \
                            ExtraWt,                                       \
                            fModify )                                      \
{                                                                          \
    int ct;                       /* loop counter */                       \
    BYTE PrevSM;                  /* previous script member value */       \
    BYTE PrevAW;                  /* previous alphanumeric value */        \
    BYTE PrevCW;                  /* previous case value */                \
    BYTE AW;                      /* alphanumeric value */                 \
    BYTE CW;                      /* case value */                         \
    DWORD PrevWt;                 /* previous weight */                    \
                                                                           \
                                                                           \
    /*                                                                     \
     *  Get the alphanumeric weight and the case weight of the             \
     *  current code point.                                                \
     */                                                                    \
    AW = GET_ALPHA_NUMERIC(&wt);                                           \
    CW = GET_CASE(&wt);                                                    \
    ExtraWt = (DWORD)0;                                                    \
                                                                           \
    /*                                                                     \
     *  Special case Repeat and Cho-On.                                    \
     *    AW = 0  =>  Repeat                                               \
     *    AW = 1  =>  Cho-On                                               \
     *    AW = 2+ =>  Kana                                                 \
     */                                                                    \
    if (AW <= MAX_SPECIAL_AW)                                              \
    {                                                                      \
        /*                                                                 \
         *  If the script member of the previous character is              \
         *  invalid, then give the special character an                    \
         *  invalid weight (highest possible weight) so that it            \
         *  will sort AFTER everything else.                               \
         */                                                                \
        ct = 1;                                                            \
        PrevWt = CMP_INVALID_FAREAST;                                      \
        while ((pCur - ct) >= pBegin)                                      \
        {                                                                  \
            PrevWt = GET_DWORD_WEIGHT(pHashN, *(pCur - ct));               \
            PrevWt &= mask;                                                \
            PrevSM = GET_SCRIPT_MEMBER(&PrevWt);                           \
            if (PrevSM < FAREAST_SPECIAL)                                  \
            {                                                              \
                if (PrevSM == EXPANSION)                                   \
                {                                                          \
                    PrevWt = CMP_INVALID_FAREAST;                          \
                }                                                          \
                else                                                       \
                {                                                          \
                    /*                                                     \
                     *  UNSORTABLE or NONSPACE_MARK.                       \
                     *                                                     \
                     *  Just ignore these, since we only care about the    \
                     *  previous UW value.                                 \
                     */                                                    \
                    PrevWt = CMP_INVALID_FAREAST;                          \
                    ct++;                                                  \
                    continue;                                              \
                }                                                          \
            }                                                              \
            else if (PrevSM == FAREAST_SPECIAL)                            \
            {                                                              \
                PrevAW = GET_ALPHA_NUMERIC(&PrevWt);                       \
                if (PrevAW <= MAX_SPECIAL_AW)                              \
                {                                                          \
                    /*                                                     \
                     *  Handle case where two special chars follow         \
                     *  each other.  Keep going back in the string.        \
                     */                                                    \
                    PrevWt = CMP_INVALID_FAREAST;                          \
                    ct++;                                                  \
                    continue;                                              \
                }                                                          \
                                                                           \
                UNICODE_WT(&PrevWt) =                                      \
                    MAKE_UNICODE_WT(KANA, PrevAW, fModify);                \
                                                                           \
                /*                                                         \
                 *  Only build weights 4, 5, 6, and 7 if the               \
                 *  previous character is KANA.                            \
                 *                                                         \
                 *  Always:                                                \
                 *    4W = previous CW  &  ISOLATE_SMALL                   \
                 *    6W = previous CW  &  ISOLATE_KANA                    \
                 *                                                         \
                 */                                                        \
                PrevCW = GET_CASE(&PrevWt);                                \
                GET_WT_FOUR(&ExtraWt) = PrevCW & ISOLATE_SMALL;            \
                GET_WT_SIX(&ExtraWt)  = PrevCW & ISOLATE_KANA;             \
                                                                           \
                if (AW == AW_REPEAT)                                       \
                {                                                          \
                    /*                                                     \
                     *  Repeat:                                            \
                     *    UW = previous UW                                 \
                     *    5W = WT_FIVE_REPEAT                              \
                     *    7W = previous CW  &  ISOLATE_WIDTH               \
                     */                                                    \
                    uw = UNICODE_WT(&PrevWt);                              \
                    GET_WT_FIVE(&ExtraWt)  = WT_FIVE_REPEAT;               \
                    GET_WT_SEVEN(&ExtraWt) = PrevCW & ISOLATE_WIDTH;       \
                }                                                          \
                else                                                       \
                {                                                          \
                    /*                                                     \
                     *  Cho-On:                                            \
                     *    UW = previous UW  &  CHO_ON_UW_MASK              \
                     *    5W = WT_FIVE_CHO_ON                              \
                     *    7W = current  CW  &  ISOLATE_WIDTH               \
                     */                                                    \
                    uw = UNICODE_WT(&PrevWt) & CHO_ON_UW_MASK;             \
                    GET_WT_FIVE(&ExtraWt)  = WT_FIVE_CHO_ON;               \
                    GET_WT_SEVEN(&ExtraWt) = CW & ISOLATE_WIDTH;           \
                }                                                          \
            }                                                              \
            else                                                           \
            {                                                              \
                uw = GET_UNICODE_MOD(&PrevWt, fModify);                    \
            }                                                              \
                                                                           \
            break;                                                         \
        }                                                                  \
    }                                                                      \
    else                                                                   \
    {                                                                      \
        /*                                                                 \
         *  Kana:                                                          \
         *    SM = KANA                                                    \
         *    AW = current AW                                              \
         *    4W = current CW  &  ISOLATE_SMALL                            \
         *    5W = WT_FIVE_KANA                                            \
         *    6W = current CW  &  ISOLATE_KANA                             \
         *    7W = current CW  &  ISOLATE_WIDTH                            \
         */                                                                \
        uw = MAKE_UNICODE_WT(KANA, AW, fModify);                           \
        GET_WT_FOUR(&ExtraWt)  = CW & ISOLATE_SMALL;                       \
        GET_WT_FIVE(&ExtraWt)  = WT_FIVE_KANA;                             \
        GET_WT_SIX(&ExtraWt)   = CW & ISOLATE_KANA;                        \
        GET_WT_SEVEN(&ExtraWt) = CW & ISOLATE_WIDTH;                       \
    }                                                                      \
                                                                           \
    /*                                                                     \
     *  Get the weight for the far east special case and store it in wt.   \
     */                                                                    \
    if ((AW > MAX_SPECIAL_AW) || (PrevWt != CMP_INVALID_FAREAST))          \
    {                                                                      \
        /*                                                                 \
         *  Always:                                                        \
         *    DW = current DW                                              \
         *    CW = minimum CW                                              \
         */                                                                \
        UNICODE_WT(&wt) = uw;                                              \
        CASE_WT(&wt) = MIN_CW;                                             \
    }                                                                      \
    else                                                                   \
    {                                                                      \
        uw = CMP_INVALID_UW;                                               \
        wt = CMP_INVALID_FAREAST;                                          \
        ExtraWt = 0;                                                       \
    }                                                                      \
}




//-------------------------------------------------------------------------//
//                             API ROUTINES                                //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  CompareStringW
//
//  Compares two wide character strings of the same locale according to the
//  supplied locale handle.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WINAPI CompareStringW(
    LCID Locale,
    DWORD dwCmpFlags,
    LPCWSTR lpString1,
    int cchCount1,
    LPCWSTR lpString2,
    int cchCount2)
{
    register LPWSTR pString1;     // ptr to go thru string 1
    register LPWSTR pString2;     // ptr to go thru string 2
    PLOC_HASH pHashN;             // ptr to LOC hash node
    BOOL fIgnorePunct;            // flag to ignore punctuation (not symbol)
    BOOL fModify;                 // flag to use modified script member weights
    DWORD State;                  // state table
    DWORD Mask;                   // mask for weights
    DWORD Weight1;                // full weight of char - string 1
    DWORD Weight2;                // full weight of char - string 2
    int WhichDiacritic;           // DW => 1 = str1 smaller, 3 = str2 smaller
    int WhichCase;                // CW => 1 = str1 smaller, 3 = str2 smaller
    int WhichPunct1;              // SW => 1 = str1 smaller, 3 = str2 smaller
    int WhichPunct2;              // SW => 1 = str1 smaller, 3 = str2 smaller
    LPWSTR pSave1;                // ptr to saved pString1
    LPWSTR pSave2;                // ptr to saved pString2
    int cExpChar1, cExpChar2;     // ct of expansions in tmp

    DWORD ExtraWt1, ExtraWt2;     // extra weight values (for far east)
    DWORD WhichExtra;             // XW => wts 4, 5, 6, 7 (for far east)


    //
    //  Call longer compare string if any of the following is true:
    //     - locale is invalid
    //     - compression locale
    //     - either count is not -1
    //     - dwCmpFlags is not 0 or ignore case   (see NOTE below)
    //     - locale is Korean - script member weight adjustment needed
    //
    //  NOTE:  If the value of NORM_IGNORECASE ever changes, this
    //         code should check for:
    //            ( (dwCmpFlags != 0)  &&  (dwCmpFlags != NORM_IGNORECASE) )
    //         Since NORM_IGNORECASE is equal to 1, we can optimize this
    //         by checking for > 1.
    //
    dwCmpFlags &= (~LOCALE_USE_CP_ACP);
    VALIDATE_LANGUAGE(Locale, pHashN, 0, TRUE);
    fModify = IS_KOREAN(Locale);
    if ( (pHashN == NULL) ||
         (pHashN->IfCompression) ||
         (cchCount1 > -1) || (cchCount2 > -1) ||
         (dwCmpFlags > NORM_IGNORECASE) ||
         (fModify == TRUE) )
    {
        return (LongCompareStringW( pHashN,
                                    dwCmpFlags,
                                    lpString1,
                                    ((cchCount1 <= -1) ? -2 : cchCount1),
                                    lpString2,
                                    ((cchCount2 <= -1) ? -2 : cchCount2),
                                    fModify ));
    }

    //
    //  Initialize string pointers.
    //
    pString1 = (LPWSTR)lpString1;
    pString2 = (LPWSTR)lpString2;

    //
    //  Invalid Parameter Check:
    //    - NULL string pointers
    //
    if ((pString1 == NULL) || (pString2 == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (0);
    }

    //
    //  Do a wchar by wchar compare.
    //
    while (TRUE)
    {
        //
        //  See if characters are equal.
        //  If characters are equal, increment pointers and continue
        //  string compare.
        //
        //  NOTE: Loop is unrolled 8 times for performance.
        //
        if ((*pString1 != *pString2) || (*pString1 == 0))
        {
            break;
        }
        pString1++;
        pString2++;

        if ((*pString1 != *pString2) || (*pString1 == 0))
        {
            break;
        }
        pString1++;
        pString2++;

        if ((*pString1 != *pString2) || (*pString1 == 0))
        {
            break;
        }
        pString1++;
        pString2++;

        if ((*pString1 != *pString2) || (*pString1 == 0))
        {
            break;
        }
        pString1++;
        pString2++;

        if ((*pString1 != *pString2) || (*pString1 == 0))
        {
            break;
        }
        pString1++;
        pString2++;

        if ((*pString1 != *pString2) || (*pString1 == 0))
        {
            break;
        }
        pString1++;
        pString2++;

        if ((*pString1 != *pString2) || (*pString1 == 0))
        {
            break;
        }
        pString1++;
        pString2++;

        if ((*pString1 != *pString2) || (*pString1 == 0))
        {
            break;
        }
        pString1++;
        pString2++;
    }

    //
    //  If strings are both at null terminators, return equal.
    //
    if (*pString1 == *pString2)
    {
        return (CSTR_EQUAL);
    }

    //
    //  Initialize flags, pointers, and counters.
    //
    fIgnorePunct = FALSE;
    WhichDiacritic = 0;
    WhichCase = 0;
    WhichPunct1 = 0;
    WhichPunct2 = 0;
    pSave1 = NULL;
    pSave2 = NULL;
    ExtraWt1 = (DWORD)0;
    WhichExtra = (DWORD)0;

    //
    //  Switch on the different flag options.  This will speed up
    //  the comparisons of two strings that are different.
    //
    //  The only two possibilities in this optimized section are
    //  no flags and the ignore case flag.
    //
    if (dwCmpFlags == 0)
    {
        Mask = CMP_MASKOFF_NONE;
    }
    else
    {
        Mask = CMP_MASKOFF_CW;
    }
    State = (pHashN->IfReverseDW) ? STATE_REVERSE_DW : STATE_DW;
    State |= STATE_CW;

    //
    //  Compare each character's sortkey weight in the two strings.
    //
    while ((*pString1 != 0) && (*pString2 != 0))
    {
        Weight1 = GET_DWORD_WEIGHT(pHashN, *pString1);
        Weight2 = GET_DWORD_WEIGHT(pHashN, *pString2);
        Weight1 &= Mask;
        Weight2 &= Mask;

        if (Weight1 != Weight2)
        {
            BYTE sm1 = GET_SCRIPT_MEMBER(&Weight1);     // script member 1
            BYTE sm2 = GET_SCRIPT_MEMBER(&Weight2);     // script member 2
            WORD uw1 = GET_UNICODE_SM(&Weight1, sm1);   // unicode weight 1
            WORD uw2 = GET_UNICODE_SM(&Weight2, sm2);   // unicode weight 2
            BYTE dw1;                                   // diacritic weight 1
            BYTE dw2;                                   // diacritic weight 2
            BOOL fContinue;                             // flag to continue loop
            DWORD Wt;                                   // temp weight holder
            WCHAR pTmpBuf1[MAX_TBL_EXPANSION];          // temp buffer for exp 1
            WCHAR pTmpBuf2[MAX_TBL_EXPANSION];          // temp buffer for exp 2


            //
            //  If Unicode Weights are different and no special cases,
            //  then we're done.  Otherwise, we need to do extra checking.
            //
            //  Must check ENTIRE string for any possibility of Unicode Weight
            //  differences.  As soon as a Unicode Weight difference is found,
            //  then we're done.  If no UW difference is found, then the
            //  first Diacritic Weight difference is used.  If no DW difference
            //  is found, then use the first Case Difference.  If no CW
            //  difference is found, then use the first Extra Weight
            //  difference.  If no EW difference is found, then use the first
            //  Special Weight difference.
            //  difference.
            //
            if ((uw1 != uw2) || (sm1 == FAREAST_SPECIAL))
            {
                //
                //  Initialize the continue flag.
                //
                fContinue = FALSE;

                //
                //  Check for Unsortable characters and skip them.
                //  This needs to be outside the switch statement.  If EITHER
                //  character is unsortable, must skip it and start over.
                //
                if (sm1 == UNSORTABLE)
                {
                    pString1++;
                    fContinue = TRUE;
                }
                if (sm2 == UNSORTABLE)
                {
                    pString2++;
                    fContinue = TRUE;
                }
                if (fContinue)
                {
                    continue;
                }

                //
                //  Switch on the script member of string 1 and take care
                //  of any special cases.
                //
                switch (sm1)
                {
                    case ( NONSPACE_MARK ) :
                    {
                        //
                        //  Nonspace only - look at diacritic weight only.
                        //
                        if ((WhichDiacritic == 0) ||
                            (State & STATE_REVERSE_DW))
                        {
                            WhichDiacritic = CSTR_GREATER_THAN;

                            //
                            //  Remove state from state machine.
                            //
                            REMOVE_STATE(STATE_DW);
                        }

                        //
                        //  Adjust pointer and set flags.
                        //
                        pString1++;
                        fContinue = TRUE;

                        break;
                    }
                    case ( PUNCTUATION ) :
                    {
                        //
                        //  If the ignore punctuation flag is set, then skip
                        //  over the punctuation.
                        //
                        if (fIgnorePunct)
                        {
                            pString1++;
                            fContinue = TRUE;
                        }
                        else if (sm2 != PUNCTUATION)
                        {
                            //
                            //  The character in the second string is
                            //  NOT punctuation.
                            //
                            if (WhichPunct2)
                            {
                                //
                                //  Set WP 2 to show that string 2 is smaller,
                                //  since a punctuation char had already been
                                //  found at an earlier position in string 2.
                                //
                                //  Set the Ignore Punctuation flag so we just
                                //  skip over any other punctuation chars in
                                //  the string.
                                //
                                WhichPunct2 = CSTR_GREATER_THAN;
                                fIgnorePunct = TRUE;
                            }
                            else
                            {
                                //
                                //  Set WP 1 to show that string 2 is smaller,
                                //  and that string 1 has had a punctuation
                                //  char - since no punctuation chars have
                                //  been found in string 2.
                                //
                                WhichPunct1 = CSTR_GREATER_THAN;
                            }

                            //
                            //  Advance pointer 1, and set flag to true.
                            //
                            pString1++;
                            fContinue = TRUE;
                        }

                        //
                        //  Do NOT want to advance the pointer in string 1 if
                        //  string 2 is also a punctuation char.  This will
                        //  be done later.
                        //

                        break;
                    }
                    case ( EXPANSION ) :
                    {
                        //
                        //  Save pointer in pString1 so that it can be
                        //  restored.
                        //
                        if (pSave1 == NULL)
                        {
                            pSave1 = pString1;
                        }
                        pString1 = pTmpBuf1;

                        //
                        //  Expand character into temporary buffer.
                        //
                        pTmpBuf1[0] = GET_EXPANSION_1(&Weight1);
                        pTmpBuf1[1] = GET_EXPANSION_2(&Weight1);

                        //
                        //  Set cExpChar1 to the number of expansion characters
                        //  stored.
                        //
                        cExpChar1 = MAX_TBL_EXPANSION;

                        fContinue = TRUE;
                        break;
                    }
                    case ( FAREAST_SPECIAL ) :
                    {
                        //
                        //  Get the weight for the far east special case
                        //  and store it in Weight1.
                        //
                        GET_FAREAST_WEIGHT( Weight1,
                                            uw1,
                                            Mask,
                                            lpString1,
                                            pString1,
                                            ExtraWt1,
                                            FALSE );

                        if (sm2 != FAREAST_SPECIAL)
                        {
                            //
                            //  The character in the second string is
                            //  NOT a fareast special char.
                            //
                            //  Set each of weights 4, 5, 6, and 7 to show
                            //  that string 2 is smaller (if not already set).
                            //
                            if ((GET_WT_FOUR(&WhichExtra) == 0) &&
                                (GET_WT_FOUR(&ExtraWt1) != 0))
                            {
                                GET_WT_FOUR(&WhichExtra) = CSTR_GREATER_THAN;
                            }
                            if ((GET_WT_FIVE(&WhichExtra) == 0) &&
                                (GET_WT_FIVE(&ExtraWt1) != 0))
                            {
                                GET_WT_FIVE(&WhichExtra) = CSTR_GREATER_THAN;
                            }
                            if ((GET_WT_SIX(&WhichExtra) == 0) &&
                                (GET_WT_SIX(&ExtraWt1) != 0))
                            {
                                GET_WT_SIX(&WhichExtra) = CSTR_GREATER_THAN;
                            }
                            if ((GET_WT_SEVEN(&WhichExtra) == 0) &&
                                (GET_WT_SEVEN(&ExtraWt1) != 0))
                            {
                                GET_WT_SEVEN(&WhichExtra) = CSTR_GREATER_THAN;
                            }
                        }

                        break;
                    }
                    case ( RESERVED_2 ) :
                    case ( RESERVED_3 ) :
                    case ( UNSORTABLE ) :
                    {
                        //
                        //  Fill out the case statement so the compiler
                        //  will use a jump table.
                        //
                        break;
                    }
                }

                //
                //  Switch on the script member of string 2 and take care
                //  of any special cases.
                //
                switch (sm2)
                {
                    case ( NONSPACE_MARK ) :
                    {
                        //
                        //  Nonspace only - look at diacritic weight only.
                        //
                        if ((WhichDiacritic == 0) ||
                            (State & STATE_REVERSE_DW))
                        {
                            WhichDiacritic = CSTR_LESS_THAN;

                            //
                            //  Remove state from state machine.
                            //
                            REMOVE_STATE(STATE_DW);
                        }

                        //
                        //  Adjust pointer and set flags.
                        //
                        pString2++;
                        fContinue = TRUE;

                        break;
                    }
                    case ( PUNCTUATION ) :
                    {
                        //
                        //  If the ignore punctuation flag is set, then skip
                        //  over the punctuation.
                        //
                        if (fIgnorePunct)
                        {
                            //
                            //  Pointer 2 will be advanced after if-else
                            //  statement.
                            //
                            ;
                        }
                        else if (sm1 != PUNCTUATION)
                        {
                            //
                            //  The character in the first string is
                            //  NOT punctuation.
                            //
                            if (WhichPunct1)
                            {
                                //
                                //  Set WP 1 to show that string 1 is smaller,
                                //  since a punctuation char had already
                                //  been found at an earlier position in
                                //  string 1.
                                //
                                //  Set the Ignore Punctuation flag so we just
                                //  skip over any other punctuation in the
                                //  string.
                                //
                                WhichPunct1 = CSTR_LESS_THAN;
                                fIgnorePunct = TRUE;
                            }
                            else
                            {
                                //
                                //  Set WP 2 to show that string 1 is smaller,
                                //  and that string 2 has had a punctuation
                                //  char - since no punctuation chars have
                                //  been found in string 1.
                                //
                                WhichPunct2 = CSTR_LESS_THAN;
                            }

                            //
                            //  Pointer 2 will be advanced after if-else
                            //  statement.
                            //
                        }
                        else
                        {
                            //
                            //  Both code points are punctuation.
                            //
                            //  See if either of the strings has encountered
                            //  punctuation chars previous to this.
                            //
                            if (WhichPunct1)
                            {
                                //
                                //  String 1 has had a punctuation char, so
                                //  it should be the smaller string (since
                                //  both have punctuation chars).
                                //
                                WhichPunct1 = CSTR_LESS_THAN;
                            }
                            else if (WhichPunct2)
                            {
                                //
                                //  String 2 has had a punctuation char, so
                                //  it should be the smaller string (since
                                //  both have punctuation chars).
                                //
                                WhichPunct2 = CSTR_GREATER_THAN;
                            }
                            else
                            {
                                //
                                //  Position is the same, so compare the
                                //  special weights.  Set WhichPunct1 to
                                //  the smaller special weight.
                                //
                                WhichPunct1 = (((GET_ALPHA_NUMERIC(&Weight1) <
                                                 GET_ALPHA_NUMERIC(&Weight2)))
                                                 ? CSTR_LESS_THAN
                                                 : CSTR_GREATER_THAN);
                            }

                            //
                            //  Set the Ignore Punctuation flag so we just
                            //  skip over any other punctuation in the string.
                            //
                            fIgnorePunct = TRUE;

                            //
                            //  Advance pointer 1.  Pointer 2 will be
                            //  advanced after if-else statement.
                            //
                            pString1++;
                        }

                        //
                        //  Advance pointer 2 and set flag to true.
                        //
                        pString2++;
                        fContinue = TRUE;

                        break;
                    }
                    case ( EXPANSION ) :
                    {
                        //
                        //  Save pointer in pString1 so that it can be
                        //  restored.
                        //
                        if (pSave2 == NULL)
                        {
                            pSave2 = pString2;
                        }
                        pString2 = pTmpBuf2;

                        //
                        //  Expand character into temporary buffer.
                        //
                        pTmpBuf2[0] = GET_EXPANSION_1(&Weight2);
                        pTmpBuf2[1] = GET_EXPANSION_2(&Weight2);

                        //
                        //  Set cExpChar2 to the number of expansion characters
                        //  stored.
                        //
                        cExpChar2 = MAX_TBL_EXPANSION;

                        fContinue = TRUE;
                        break;
                    }
                    case ( FAREAST_SPECIAL ) :
                    {
                        //
                        //  Get the weight for the far east special case
                        //  and store it in Weight2.
                        //
                        GET_FAREAST_WEIGHT( Weight2,
                                            uw2,
                                            Mask,
                                            lpString2,
                                            pString2,
                                            ExtraWt2,
                                            FALSE );

                        if (sm1 != FAREAST_SPECIAL)
                        {
                            //
                            //  The character in the first string is
                            //  NOT a fareast special char.
                            //
                            //  Set each of weights 4, 5, 6, and 7 to show
                            //  that string 1 is smaller (if not already set).
                            //
                            if ((GET_WT_FOUR(&WhichExtra) == 0) &&
                                (GET_WT_FOUR(&ExtraWt2) != 0))
                            {
                                GET_WT_FOUR(&WhichExtra) = CSTR_LESS_THAN;
                            }
                            if ((GET_WT_FIVE(&WhichExtra) == 0) &&
                                (GET_WT_FIVE(&ExtraWt2) != 0))
                            {
                                GET_WT_FIVE(&WhichExtra) = CSTR_LESS_THAN;
                            }
                            if ((GET_WT_SIX(&WhichExtra) == 0) &&
                                (GET_WT_SIX(&ExtraWt2) != 0))
                            {
                                GET_WT_SIX(&WhichExtra) = CSTR_LESS_THAN;
                            }
                            if ((GET_WT_SEVEN(&WhichExtra) == 0) &&
                                (GET_WT_SEVEN(&ExtraWt2) != 0))
                            {
                                GET_WT_SEVEN(&WhichExtra) = CSTR_LESS_THAN;
                            }
                        }
                        else
                        {
                            //
                            //  Characters in both strings are fareast
                            //  special chars.
                            //
                            //  Set each of weights 4, 5, 6, and 7
                            //  appropriately (if not already set).
                            //
                            if ( (GET_WT_FOUR(&WhichExtra) == 0) &&
                                 ( GET_WT_FOUR(&ExtraWt1) !=
                                   GET_WT_FOUR(&ExtraWt2) ) )
                            {
                                GET_WT_FOUR(&WhichExtra) =
                                  ( GET_WT_FOUR(&ExtraWt1) <
                                    GET_WT_FOUR(&ExtraWt2) )
                                  ? CSTR_LESS_THAN
                                  : CSTR_GREATER_THAN;
                            }
                            if ( (GET_WT_FIVE(&WhichExtra) == 0) &&
                                 ( GET_WT_FIVE(&ExtraWt1) !=
                                   GET_WT_FIVE(&ExtraWt2) ) )
                            {
                                GET_WT_FIVE(&WhichExtra) =
                                  ( GET_WT_FIVE(&ExtraWt1) <
                                    GET_WT_FIVE(&ExtraWt2) )
                                  ? CSTR_LESS_THAN
                                  : CSTR_GREATER_THAN;
                            }
                            if ( (GET_WT_SIX(&WhichExtra) == 0) &&
                                 ( GET_WT_SIX(&ExtraWt1) !=
                                   GET_WT_SIX(&ExtraWt2) ) )
                            {
                                GET_WT_SIX(&WhichExtra) =
                                  ( GET_WT_SIX(&ExtraWt1) <
                                    GET_WT_SIX(&ExtraWt2) )
                                  ? CSTR_LESS_THAN
                                  : CSTR_GREATER_THAN;
                            }
                            if ( (GET_WT_SEVEN(&WhichExtra) == 0) &&
                                 ( GET_WT_SEVEN(&ExtraWt1) !=
                                   GET_WT_SEVEN(&ExtraWt2) ) )
                            {
                                GET_WT_SEVEN(&WhichExtra) =
                                  ( GET_WT_SEVEN(&ExtraWt1) <
                                    GET_WT_SEVEN(&ExtraWt2) )
                                  ? CSTR_LESS_THAN
                                  : CSTR_GREATER_THAN;
                            }
                        }

                        break;
                    }
                    case ( RESERVED_2 ) :
                    case ( RESERVED_3 ) :
                    case ( UNSORTABLE ) :
                    {
                        //
                        //  Fill out the case statement so the compiler
                        //  will use a jump table.
                        //
                        break;
                    }
                }

                //
                //  See if the comparison should start again.
                //
                if (fContinue)
                {
                    continue;
                }

                //
                //  We're not supposed to drop down into the state table if
                //  unicode weights are different, so stop comparison and
                //  return result of unicode weight comparison.
                //
                if (uw1 != uw2)
                {
                    return ((uw1 < uw2) ? CSTR_LESS_THAN : CSTR_GREATER_THAN);
                }
            }

            //
            //  For each state in the state table, do the appropriate
            //  comparisons.     (UW1 == UW2)
            //
            if (State & (STATE_DW | STATE_REVERSE_DW))
            {
                //
                //  Get the diacritic weights.
                //
                dw1 = GET_DIACRITIC(&Weight1);
                dw2 = GET_DIACRITIC(&Weight2);

                if (dw1 != dw2)
                {
                    //
                    //  Look ahead to see if diacritic follows a
                    //  minimum diacritic weight.  If so, get the
                    //  diacritic weight of the nonspace mark.
                    //
                    while (*(pString1 + 1) != 0)
                    {
                        Wt = GET_DWORD_WEIGHT(pHashN, *(pString1 + 1));
                        if (GET_SCRIPT_MEMBER(&Wt) == NONSPACE_MARK)
                        {
                            dw1 += GET_DIACRITIC(&Wt);
                            pString1++;
                        }
                        else
                        {
                            break;
                        }
                    }

                    while (*(pString2 + 1) != 0)
                    {
                        Wt = GET_DWORD_WEIGHT(pHashN, *(pString2 + 1));
                        if (GET_SCRIPT_MEMBER(&Wt) == NONSPACE_MARK)
                        {
                            dw2 += GET_DIACRITIC(&Wt);
                            pString2++;
                        }
                        else
                        {
                            break;
                        }
                    }

                    //
                    //  Save which string has the smaller diacritic
                    //  weight if the diacritic weights are still
                    //  different.
                    //
                    if (dw1 != dw2)
                    {
                        WhichDiacritic = (dw1 < dw2)
                                           ? CSTR_LESS_THAN
                                           : CSTR_GREATER_THAN;

                        //
                        //  Remove state from state machine.
                        //
                        REMOVE_STATE(STATE_DW);
                    }
                }
            }
            if (State & STATE_CW)
            {
                //
                //  Get the case weights.
                //
                if (GET_CASE(&Weight1) != GET_CASE(&Weight2))
                {
                    //
                    //  Save which string has the smaller case weight.
                    //
                    WhichCase = (GET_CASE(&Weight1) < GET_CASE(&Weight2))
                                  ? CSTR_LESS_THAN
                                  : CSTR_GREATER_THAN;

                    //
                    //  Remove state from state machine.
                    //
                    REMOVE_STATE(STATE_CW);
                }
            }
        }

        //
        //  Fixup the pointers.
        //
        POINTER_FIXUP();
    }

    //
    //  If the end of BOTH strings has been reached, then the unicode
    //  weights match exactly.  Check the diacritic, case and special
    //  weights.  If all are zero, then return success.  Otherwise,
    //  return the result of the weight difference.
    //
    //  NOTE:  The following checks MUST REMAIN IN THIS ORDER:
    //            Diacritic, Case, Punctuation.
    //
    if (*pString1 == 0)
    {
        if (*pString2 == 0)
        {
            if (WhichDiacritic)
            {
                return (WhichDiacritic);
            }
            if (WhichCase)
            {
                return (WhichCase);
            }
            if (WhichExtra)
            {
                if (GET_WT_FOUR(&WhichExtra))
                {
                    return (GET_WT_FOUR(&WhichExtra));
                }
                if (GET_WT_FIVE(&WhichExtra))
                {
                    return (GET_WT_FIVE(&WhichExtra));
                }
                if (GET_WT_SIX(&WhichExtra))
                {
                    return (GET_WT_SIX(&WhichExtra));
                }
                if (GET_WT_SEVEN(&WhichExtra))
                {
                    return (GET_WT_SEVEN(&WhichExtra));
                }
            }
            if (WhichPunct1)
            {
                return (WhichPunct1);
            }
            if (WhichPunct2)
            {
                return (WhichPunct2);
            }

            return (CSTR_EQUAL);
        }
        else
        {
            //
            //  String 2 is longer.
            //
            pString1 = pString2;
        }
    }

    //
    //  Scan to the end of the longer string.
    //
    QUICK_SCAN_LONGER_STRING( pString1,
                              ((*pString2 == 0)
                                ? CSTR_GREATER_THAN
                                : CSTR_LESS_THAN) );
}


////////////////////////////////////////////////////////////////////////////
//
//  GetStringTypeExW
//
//  Returns character type information about a particular Unicode string.
//
//  01-18-94    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI GetStringTypeExW(
    LCID Locale,
    DWORD dwInfoType,
    LPCWSTR lpSrcStr,
    int cchSrc,
    LPWORD lpCharType)
{
    PLOC_HASH pHashN;             // ptr to LOC hash node


    //
    //  Invalid Parameter Check:
    //    - Validate LCID
    //
    VALIDATE_LOCALE(Locale, pHashN, FALSE);
    if (pHashN == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (0);
    }

    //
    //  Return the result of GetStringTypeW.
    //
    return (GetStringTypeW( dwInfoType,
                            lpSrcStr,
                            cchSrc,
                            lpCharType ));
}


////////////////////////////////////////////////////////////////////////////
//
//  GetStringTypeW
//
//  Returns character type information about a particular Unicode string.
//
//  NOTE:  The number of parameters is different from GetStringTypeA.
//         The 16-bit OLE product shipped GetStringTypeA with the wrong
//         parameters (ported from Chicago) and now we must support it.
//
//         Use GetStringTypeEx to get the same set of parameters between
//         the A and W version.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI GetStringTypeW(
    DWORD dwInfoType,
    LPCWSTR lpSrcStr,
    int cchSrc,
    LPWORD lpCharType)
{
    int Ctr;                      // loop counter


    //
    //  Invalid Parameter Check:
    //    - lpSrcStr NULL
    //    - cchSrc is 0
    //    - lpCharType NULL
    //    - same buffer - src and destination
    //    - (flags will be checked in switch statement below)
    //
    if ( (lpSrcStr == NULL) || (cchSrc == 0) ||
         (lpCharType == NULL) || (lpSrcStr == lpCharType) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
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
    //  Make sure the ctype table is mapped in.
    //
    if (GetCTypeFileInfo())
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return (FALSE);
    }

    //
    //  Return the appropriate information in the lpCharType parameter
    //  based on the dwInfoType parameter.
    //
    switch (dwInfoType)
    {
        case ( CT_CTYPE1 ) :
        {
            //
            //  Return the ctype 1 information for the string.
            //
            for (Ctr = 0; Ctr < cchSrc; Ctr++)
            {
                lpCharType[Ctr] = GET_CTYPE(lpSrcStr[Ctr], CType1);
            }
            break;
        }
        case ( CT_CTYPE2 ) :
        {
            //
            //  Return the ctype 2 information.
            //
            for (Ctr = 0; Ctr < cchSrc; Ctr++)
            {
                lpCharType[Ctr] = GET_CTYPE(lpSrcStr[Ctr], CType2);
            }
            break;
        }
        case ( CT_CTYPE3 ) :
        {
            //
            //  Return the ctype 3 information.
            //
            for (Ctr = 0; Ctr < cchSrc; Ctr++)
            {
                lpCharType[Ctr] = GET_CTYPE(lpSrcStr[Ctr], CType3);
            }
            break;
        }
        default :
        {
            //
            //  Invalid flag parameter, so return failure.
            //
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
//  LongCompareStringW
//
//  Compares two wide character strings of the same locale according to the
//  supplied locale handle.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int LongCompareStringW(
    PLOC_HASH pHashN,
    DWORD dwCmpFlags,
    LPCWSTR lpString1,
    int cchCount1,
    LPCWSTR lpString2,
    int cchCount2,
    BOOL fModify)
{
    int ctr1 = cchCount1;         // loop counter for string 1
    int ctr2 = cchCount2;         // loop counter for string 2
    register LPWSTR pString1;     // ptr to go thru string 1
    register LPWSTR pString2;     // ptr to go thru string 2
    BOOL IfCompress;              // if compression in locale
    BOOL IfDblCompress1;          // if double compression in string 1
    BOOL IfDblCompress2;          // if double compression in string 2
    BOOL fEnd1;                   // if at end of string 1
    BOOL fIgnorePunct;            // flag to ignore punctuation (not symbol)
    BOOL fIgnoreDiacritic;        // flag to ignore diacritics
    BOOL fIgnoreSymbol;           // flag to ignore symbols
    BOOL fStringSort;             // flag to use string sort
    DWORD State;                  // state table
    DWORD Mask;                   // mask for weights
    DWORD Weight1;                // full weight of char - string 1
    DWORD Weight2;                // full weight of char - string 2
    int WhichDiacritic;           // DW => 1 = str1 smaller, 3 = str2 smaller
    int WhichCase;                // CW => 1 = str1 smaller, 3 = str2 smaller
    int WhichPunct1;              // SW => 1 = str1 smaller, 3 = str2 smaller
    int WhichPunct2;              // SW => 1 = str1 smaller, 3 = str2 smaller
    LPWSTR pSave1;                // ptr to saved pString1
    LPWSTR pSave2;                // ptr to saved pString2
    int cExpChar1, cExpChar2;     // ct of expansions in tmp

    DWORD ExtraWt1, ExtraWt2;     // extra weight values (for far east)
    DWORD WhichExtra;             // XW => wts 4, 5, 6, 7 (for far east)


    //
    //  Initialize string pointers.
    //
    pString1 = (LPWSTR)lpString1;
    pString2 = (LPWSTR)lpString2;

    //
    //  Invalid Parameter Check:
    //    - invalid locale (hash node)
    //    - either string is null
    //
    if ( (pHashN == NULL) ||
         (pString1 == NULL) || (pString2 == NULL) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (0);
    }

    //
    //  Invalid Flags Check:
    //    - invalid flags
    //
    if (dwCmpFlags & CS_INVALID_FLAG)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return (0);
    }

    //
    //  See if we should stop on the null terminator regardless of the
    //  count values.  The original count values are stored in ctr1 and ctr2
    //  above, so it's ok to set these here.
    //
    if (dwCmpFlags & NORM_STOP_ON_NULL)
    {
        cchCount1 = cchCount2 = -2;
    }

    //
    //  Check if compression in the given locale.  If not, then
    //  try a wchar by wchar compare.  If strings are equal, this
    //  will be quick.
    //
    if ((IfCompress = pHashN->IfCompression) == FALSE)
    {
        //
        //  Compare each wide character in the two strings.
        //
        while ( NOT_END_STRING(ctr1, pString1, cchCount1) &&
                NOT_END_STRING(ctr2, pString2, cchCount2) )
        {
            //
            //  See if characters are equal.
            //
            if (*pString1 == *pString2)
            {
                //
                //  Characters are equal, so increment pointers,
                //  decrement counters, and continue string compare.
                //
                pString1++;
                pString2++;
                ctr1--;
                ctr2--;
            }
            else
            {
                //
                //  Difference was found.  Fall into the sortkey
                //  check below.
                //
                break;
            }
        }

        //
        //  If the end of BOTH strings has been reached, then the strings
        //  match exactly.  Return success.
        //
        if ( AT_STRING_END(ctr1, pString1, cchCount1) &&
             AT_STRING_END(ctr2, pString2, cchCount2) )
        {
            return (CSTR_EQUAL);
        }
    }

    //
    //  Initialize flags, pointers, and counters.
    //
    fIgnorePunct = dwCmpFlags & NORM_IGNORESYMBOLS;
    fIgnoreDiacritic = dwCmpFlags & NORM_IGNORENONSPACE;
    fIgnoreSymbol = fIgnorePunct;
    fStringSort = dwCmpFlags & SORT_STRINGSORT;
    WhichDiacritic = 0;
    WhichCase = 0;
    WhichPunct1 = 0;
    WhichPunct2 = 0;
    pSave1 = NULL;
    pSave2 = NULL;
    ExtraWt1 = (DWORD)0;
    WhichExtra = (DWORD)0;

    //
    //  Set the weights to be invalid.  This flags whether or not to
    //  recompute the weights next time through the loop.  It also flags
    //  whether or not to start over (continue) in the loop.
    //
    Weight1 = CMP_INVALID_WEIGHT;
    Weight2 = CMP_INVALID_WEIGHT;

    //
    //  Switch on the different flag options.  This will speed up
    //  the comparisons of two strings that are different.
    //
    State = STATE_CW;
    switch (dwCmpFlags & (NORM_IGNORECASE | NORM_IGNORENONSPACE))
    {
        case ( 0 ) :
        {
            Mask = CMP_MASKOFF_NONE;
            State |= (pHashN->IfReverseDW) ? STATE_REVERSE_DW : STATE_DW;

            break;
        }

        case ( NORM_IGNORECASE ) :
        {
            Mask = CMP_MASKOFF_CW;
            State |= (pHashN->IfReverseDW) ? STATE_REVERSE_DW : STATE_DW;

            break;
        }

        case ( NORM_IGNORENONSPACE ) :
        {
            Mask = CMP_MASKOFF_DW;

            break;
        }

        case ( NORM_IGNORECASE | NORM_IGNORENONSPACE ) :
        {
            Mask = CMP_MASKOFF_DW_CW;

            break;
        }
    }

    switch (dwCmpFlags & (NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH))
    {
        case ( 0 ) :
        {
            break;
        }

        case ( NORM_IGNOREKANATYPE ) :
        {
            Mask &= CMP_MASKOFF_KANA;

            break;
        }

        case ( NORM_IGNOREWIDTH ) :
        {
            Mask &= CMP_MASKOFF_WIDTH;

            if (dwCmpFlags & NORM_IGNORECASE)
            {
                REMOVE_STATE(STATE_CW);
            }

            break;
        }

        case ( NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH ) :
        {
            Mask &= CMP_MASKOFF_KANA_WIDTH;

            if (dwCmpFlags & NORM_IGNORECASE)
            {
                REMOVE_STATE(STATE_CW);
            }

            break;
        }
    }

    //
    //  Compare each character's sortkey weight in the two strings.
    //
    while ( NOT_END_STRING(ctr1, pString1, cchCount1) &&
            NOT_END_STRING(ctr2, pString2, cchCount2) )
    {
        if (Weight1 == CMP_INVALID_WEIGHT)
        {
            Weight1 = GET_DWORD_WEIGHT(pHashN, *pString1);
            Weight1 &= Mask;
        }
        if (Weight2 == CMP_INVALID_WEIGHT)
        {
            Weight2 = GET_DWORD_WEIGHT(pHashN, *pString2);
            Weight2 &= Mask;
        }

        //
        //  If compression locale, then need to check for compression
        //  characters even if the weights are equal.  If it's not a
        //  compression locale, then we don't need to check anything
        //  if the weights are equal.
        //
        if ( (IfCompress) &&
             (GET_COMPRESSION(&Weight1) || GET_COMPRESSION(&Weight2)) )
        {
            int ctr;                   // loop counter
            PCOMPRESS_3 pComp3;        // ptr to compress 3 table
            PCOMPRESS_2 pComp2;        // ptr to compress 2 table
            int If1;                   // if compression found in string 1
            int If2;                   // if compression found in string 2
            int CompVal;               // compression value
            int IfEnd1;                // if exists 1 more char in string 1
            int IfEnd2;                // if exists 1 more char in string 2


            //
            //  Check for compression in the weights.
            //
            If1 = GET_COMPRESSION(&Weight1);
            If2 = GET_COMPRESSION(&Weight2);
            CompVal = ((If1 > If2) ? If1 : If2);

            IfEnd1 = AT_STRING_END(ctr1 - 1, pString1 + 1, cchCount1);
            IfEnd2 = AT_STRING_END(ctr2 - 1, pString2 + 1, cchCount2);

            if (pHashN->IfDblCompression == FALSE)
            {
                //
                //  NO double compression, so don't check for it.
                //
                switch (CompVal)
                {
                    //
                    //  Check for 3 characters compressing to 1.
                    //
                    case ( COMPRESS_3_MASK ) :
                    {
                        //
                        //  Check character in string 1 and string 2.
                        //
                        if ( ((If1) && (!IfEnd1) &&
                              !AT_STRING_END(ctr1 - 2, pString1 + 2, cchCount1)) ||
                             ((If2) && (!IfEnd2) &&
                              !AT_STRING_END(ctr2 - 2, pString2 + 2, cchCount2)) )
                        {
                            ctr = pHashN->pCompHdr->Num3;
                            pComp3 = pHashN->pCompress3;
                            for (; ctr > 0; ctr--, pComp3++)
                            {
                                //
                                //  Check character in string 1.
                                //
                                if ( (If1) && (!IfEnd1) &&
                                     !AT_STRING_END(ctr1 - 2, pString1 + 2, cchCount1) &&
                                     (pComp3->UCP1 == *pString1) &&
                                     (pComp3->UCP2 == *(pString1 + 1)) &&
                                     (pComp3->UCP3 == *(pString1 + 2)) )
                                {
                                    //
                                    //  Found compression for string 1.
                                    //  Get new weight and mask it.
                                    //  Increment pointer and decrement counter.
                                    //
                                    Weight1 = MAKE_SORTKEY_DWORD(pComp3->Weights);
                                    Weight1 &= Mask;
                                    pString1 += 2;
                                    ctr1 -= 2;

                                    //
                                    //  Set boolean for string 1 - search is
                                    //  complete.
                                    //
                                    If1 = 0;

                                    //
                                    //  Break out of loop if both searches are
                                    //  done.
                                    //
                                    if (If2 == 0)
                                        break;
                                }

                                //
                                //  Check character in string 2.
                                //
                                if ( (If2) && (!IfEnd2) &&
                                     !AT_STRING_END(ctr2 - 2, pString2 + 2, cchCount2) &&
                                     (pComp3->UCP1 == *pString2) &&
                                     (pComp3->UCP2 == *(pString2 + 1)) &&
                                     (pComp3->UCP3 == *(pString2 + 2)) )
                                {
                                    //
                                    //  Found compression for string 2.
                                    //  Get new weight and mask it.
                                    //  Increment pointer and decrement counter.
                                    //
                                    Weight2 = MAKE_SORTKEY_DWORD(pComp3->Weights);
                                    Weight2 &= Mask;
                                    pString2 += 2;
                                    ctr2 -= 2;

                                    //
                                    //  Set boolean for string 2 - search is
                                    //  complete.
                                    //
                                    If2 = 0;

                                    //
                                    //  Break out of loop if both searches are
                                    //  done.
                                    //
                                    if (If1 == 0)
                                    {
                                        break;
                                    }
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

                    //
                    //  Check for 2 characters compressing to 1.
                    //
                    case ( COMPRESS_2_MASK ) :
                    {
                        //
                        //  Check character in string 1 and string 2.
                        //
                        if ( ((If1) && (!IfEnd1)) ||
                             ((If2) && (!IfEnd2)) )
                        {
                            ctr = pHashN->pCompHdr->Num2;
                            pComp2 = pHashN->pCompress2;
                            for (; ((ctr > 0) && (If1 || If2)); ctr--, pComp2++)
                            {
                                //
                                //  Check character in string 1.
                                //
                                if ( (If1) &&
                                     (!IfEnd1) &&
                                     (pComp2->UCP1 == *pString1) &&
                                     (pComp2->UCP2 == *(pString1 + 1)) )
                                {
                                    //
                                    //  Found compression for string 1.
                                    //  Get new weight and mask it.
                                    //  Increment pointer and decrement counter.
                                    //
                                    Weight1 = MAKE_SORTKEY_DWORD(pComp2->Weights);
                                    Weight1 &= Mask;
                                    pString1++;
                                    ctr1--;

                                    //
                                    //  Set boolean for string 1 - search is
                                    //  complete.
                                    //
                                    If1 = 0;

                                    //
                                    //  Break out of loop if both searches are
                                    //  done.
                                    //
                                    if (If2 == 0)
                                        break;
                                }

                                //
                                //  Check character in string 2.
                                //
                                if ( (If2) &&
                                     (!IfEnd2) &&
                                     (pComp2->UCP1 == *pString2) &&
                                     (pComp2->UCP2 == *(pString2 + 1)) )
                                {
                                    //
                                    //  Found compression for string 2.
                                    //  Get new weight and mask it.
                                    //  Increment pointer and decrement counter.
                                    //
                                    Weight2 = MAKE_SORTKEY_DWORD(pComp2->Weights);
                                    Weight2 &= Mask;
                                    pString2++;
                                    ctr2--;

                                    //
                                    //  Set boolean for string 2 - search is
                                    //  complete.
                                    //
                                    If2 = 0;

                                    //
                                    //  Break out of loop if both searches are
                                    //  done.
                                    //
                                    if (If1 == 0)
                                    {
                                        break;
                                    }
                                }
                            }
                            if (ctr > 0)
                            {
                                break;
                            }
                        }
                    }
                }
            }
            else if (!IfEnd1 && !IfEnd2)
            {
                //
                //  Double Compression exists, so must check for it.
                //
                if (IfDblCompress1 =
                       ((GET_DWORD_WEIGHT(pHashN, *pString1) & CMP_MASKOFF_CW) ==
                        (GET_DWORD_WEIGHT(pHashN, *(pString1 + 1)) & CMP_MASKOFF_CW)))
                {
                    //
                    //  Advance past the first code point to get to the
                    //  compression character.
                    //
                    pString1++;
                    ctr1--;
                    IfEnd1 = AT_STRING_END(ctr1 - 1, pString1 + 1, cchCount1);
                }

                if (IfDblCompress2 =
                       ((GET_DWORD_WEIGHT(pHashN, *pString2) & CMP_MASKOFF_CW) ==
                        (GET_DWORD_WEIGHT(pHashN, *(pString2 + 1)) & CMP_MASKOFF_CW)))
                {
                    //
                    //  Advance past the first code point to get to the
                    //  compression character.
                    //
                    pString2++;
                    ctr2--;
                    IfEnd2 = AT_STRING_END(ctr2 - 1, pString2 + 1, cchCount2);
                }

                switch (CompVal)
                {
                    //
                    //  Check for 3 characters compressing to 1.
                    //
                    case ( COMPRESS_3_MASK ) :
                    {
                        //
                        //  Check character in string 1.
                        //
                        if ( (If1) && (!IfEnd1) &&
                             !AT_STRING_END(ctr1 - 2, pString1 + 2, cchCount1) )
                        {
                            ctr = pHashN->pCompHdr->Num3;
                            pComp3 = pHashN->pCompress3;
                            for (; ctr > 0; ctr--, pComp3++)
                            {
                                //
                                //  Check character in string 1.
                                //
                                if ( (pComp3->UCP1 == *pString1) &&
                                     (pComp3->UCP2 == *(pString1 + 1)) &&
                                     (pComp3->UCP3 == *(pString1 + 2)) )
                                {
                                    //
                                    //  Found compression for string 1.
                                    //  Get new weight and mask it.
                                    //  Increment pointer and decrement counter.
                                    //
                                    Weight1 = MAKE_SORTKEY_DWORD(pComp3->Weights);
                                    Weight1 &= Mask;
                                    if (!IfDblCompress1)
                                    {
                                        pString1 += 2;
                                        ctr1 -= 2;
                                    }

                                    //
                                    //  Set boolean for string 1 - search is
                                    //  complete.
                                    //
                                    If1 = 0;
                                    break;
                                }
                            }
                        }

                        //
                        //  Check character in string 2.
                        //
                        if ( (If2) && (!IfEnd2) &&
                             !AT_STRING_END(ctr2 - 2, pString2 + 2, cchCount2) )
                        {
                            ctr = pHashN->pCompHdr->Num3;
                            pComp3 = pHashN->pCompress3;
                            for (; ctr > 0; ctr--, pComp3++)
                            {
                                //
                                //  Check character in string 2.
                                //
                                if ( (pComp3->UCP1 == *pString2) &&
                                     (pComp3->UCP2 == *(pString2 + 1)) &&
                                     (pComp3->UCP3 == *(pString2 + 2)) )
                                {
                                    //
                                    //  Found compression for string 2.
                                    //  Get new weight and mask it.
                                    //  Increment pointer and decrement counter.
                                    //
                                    Weight2 = MAKE_SORTKEY_DWORD(pComp3->Weights);
                                    Weight2 &= Mask;
                                    if (!IfDblCompress2)
                                    {
                                        pString2 += 2;
                                        ctr2 -= 2;
                                    }

                                    //
                                    //  Set boolean for string 2 - search is
                                    //  complete.
                                    //
                                    If2 = 0;
                                    break;
                                }
                            }
                        }

                        //
                        //  Fall through if not found.
                        //
                        if ((If1 == 0) && (If2 == 0))
                        {
                            break;
                        }
                    }

                    //
                    //  Check for 2 characters compressing to 1.
                    //
                    case ( COMPRESS_2_MASK ) :
                    {
                        //
                        //  Check character in string 1.
                        //
                        if ((If1) && (!IfEnd1))
                        {
                            ctr = pHashN->pCompHdr->Num2;
                            pComp2 = pHashN->pCompress2;
                            for (; ctr > 0; ctr--, pComp2++)
                            {
                                //
                                //  Check character in string 1.
                                //
                                if ((pComp2->UCP1 == *pString1) &&
                                    (pComp2->UCP2 == *(pString1 + 1)))
                                {
                                    //
                                    //  Found compression for string 1.
                                    //  Get new weight and mask it.
                                    //  Increment pointer and decrement counter.
                                    //
                                    Weight1 = MAKE_SORTKEY_DWORD(pComp2->Weights);
                                    Weight1 &= Mask;
                                    if (!IfDblCompress1)
                                    {
                                        pString1++;
                                        ctr1--;
                                    }

                                    //
                                    //  Set boolean for string 1 - search is
                                    //  complete.
                                    //
                                    If1 = 0;
                                    break;
                                }
                            }
                        }

                        //
                        //  Check character in string 2.
                        //
                        if ((If2) && (!IfEnd2))
                        {
                            ctr = pHashN->pCompHdr->Num2;
                            pComp2 = pHashN->pCompress2;
                            for (; ctr > 0; ctr--, pComp2++)
                            {
                                //
                                //  Check character in string 2.
                                //
                                if ((pComp2->UCP1 == *pString2) &&
                                    (pComp2->UCP2 == *(pString2 + 1)))
                                {
                                    //
                                    //  Found compression for string 2.
                                    //  Get new weight and mask it.
                                    //  Increment pointer and decrement counter.
                                    //
                                    Weight2 = MAKE_SORTKEY_DWORD(pComp2->Weights);
                                    Weight2 &= Mask;
                                    if (!IfDblCompress2)
                                    {
                                        pString2++;
                                        ctr2--;
                                    }

                                    //
                                    //  Set boolean for string 2 - search is
                                    //  complete.
                                    //
                                    If2 = 0;
                                    break;
                                }
                            }
                        }
                    }
                }

                //
                //  Reset the pointer back to the beginning of the double
                //  compression.  Pointer fixup at the end will advance
                //  them correctly.
                //
                //  If double compression, we advanced the pointer at
                //  the beginning of the switch statement.  If double
                //  compression character was actually found, the pointer
                //  was NOT advanced.  We now want to decrement the pointer
                //  to put it back to where it was.
                //
                //  The next time through, the pointer will be pointing to
                //  the regular compression part of the string.
                //
                if (IfDblCompress1)
                {
                    pString1--;
                    ctr1++;
                }
                if (IfDblCompress2)
                {
                    pString2--;
                    ctr2++;
                }
            }
        }

        //
        //  Check the weights again.
        //
        if (Weight1 != Weight2)
        {
            //
            //  Weights are still not equal, even after compression
            //  check, so compare the different weights.
            //
            BYTE sm1 = GET_SCRIPT_MEMBER(&Weight1);                // script member 1
            BYTE sm2 = GET_SCRIPT_MEMBER(&Weight2);                // script member 2
            WORD uw1 = GET_UNICODE_SM_MOD(&Weight1, sm1, fModify); // unicode weight 1
            WORD uw2 = GET_UNICODE_SM_MOD(&Weight2, sm2, fModify); // unicode weight 2
            BYTE dw1;                                              // diacritic weight 1
            BYTE dw2;                                              // diacritic weight 2
            DWORD Wt;                                              // temp weight holder
            WCHAR pTmpBuf1[MAX_TBL_EXPANSION];                     // temp buffer for exp 1
            WCHAR pTmpBuf2[MAX_TBL_EXPANSION];                     // temp buffer for exp 2


            //
            //  If Unicode Weights are different and no special cases,
            //  then we're done.  Otherwise, we need to do extra checking.
            //
            //  Must check ENTIRE string for any possibility of Unicode Weight
            //  differences.  As soon as a Unicode Weight difference is found,
            //  then we're done.  If no UW difference is found, then the
            //  first Diacritic Weight difference is used.  If no DW difference
            //  is found, then use the first Case Difference.  If no CW
            //  difference is found, then use the first Extra Weight
            //  difference.  If no EW difference is found, then use the first
            //  Special Weight difference.
            //
            if ((uw1 != uw2) ||
                ((sm1 <= SYMBOL_5) && (sm1 >= FAREAST_SPECIAL)))
            {
                //
                //  Check for Unsortable characters and skip them.
                //  This needs to be outside the switch statement.  If EITHER
                //  character is unsortable, must skip it and start over.
                //
                if (sm1 == UNSORTABLE)
                {
                    pString1++;
                    ctr1--;
                    Weight1 = CMP_INVALID_WEIGHT;
                }
                if (sm2 == UNSORTABLE)
                {
                    pString2++;
                    ctr2--;
                    Weight2 = CMP_INVALID_WEIGHT;
                }

                //
                //  Check for Ignore Nonspace and Ignore Symbol.  If
                //  Ignore Nonspace is set and either character is a
                //  nonspace mark only, then we need to advance the
                //  pointer to skip over the character and continue.
                //  If Ignore Symbol is set and either character is a
                //  punctuation char, then we need to advance the
                //  pointer to skip over the character and continue.
                //
                //  This step is necessary so that a string with a
                //  nonspace mark and a punctuation char following one
                //  another are properly ignored when one or both of
                //  the ignore flags is set.
                //
                if (fIgnoreDiacritic)
                {
                    if (sm1 == NONSPACE_MARK)
                    {
                        pString1++;
                        ctr1--;
                        Weight1 = CMP_INVALID_WEIGHT;
                    }
                    if (sm2 == NONSPACE_MARK)
                    {
                        pString2++;
                        ctr2--;
                        Weight2 = CMP_INVALID_WEIGHT;
                    }
                }
                if (fIgnoreSymbol)
                {
                    if (sm1 == PUNCTUATION)
                    {
                        pString1++;
                        ctr1--;
                        Weight1 = CMP_INVALID_WEIGHT;
                    }
                    if (sm2 == PUNCTUATION)
                    {
                        pString2++;
                        ctr2--;
                        Weight2 = CMP_INVALID_WEIGHT;
                    }
                }
                if ((Weight1 == CMP_INVALID_WEIGHT) || (Weight2 == CMP_INVALID_WEIGHT))
                {
                    continue;
                }

                //
                //  Switch on the script member of string 1 and take care
                //  of any special cases.
                //
                switch (sm1)
                {
                    case ( NONSPACE_MARK ) :
                    {
                        //
                        //  Nonspace only - look at diacritic weight only.
                        //
                        if (!fIgnoreDiacritic)
                        {
                            if ((WhichDiacritic == 0) ||
                                (State & STATE_REVERSE_DW))
                            {
                                WhichDiacritic = CSTR_GREATER_THAN;

                                //
                                //  Remove state from state machine.
                                //
                                REMOVE_STATE(STATE_DW);
                            }
                        }

                        //
                        //  Adjust pointer and counter and set flags.
                        //
                        pString1++;
                        ctr1--;
                        Weight1 = CMP_INVALID_WEIGHT;

                        break;
                    }
                    case ( SYMBOL_1 ) :
                    case ( SYMBOL_2 ) :
                    case ( SYMBOL_3 ) :
                    case ( SYMBOL_4 ) :
                    case ( SYMBOL_5 ) :
                    {
                        //
                        //  If the ignore symbol flag is set, then skip over
                        //  the symbol.
                        //
                        if (fIgnoreSymbol)
                        {
                            pString1++;
                            ctr1--;
                            Weight1 = CMP_INVALID_WEIGHT;
                        }

                        break;
                    }
                    case ( PUNCTUATION ) :
                    {
                        //
                        //  If the ignore punctuation flag is set, then skip
                        //  over the punctuation char.
                        //
                        if (fIgnorePunct)
                        {
                            pString1++;
                            ctr1--;
                            Weight1 = CMP_INVALID_WEIGHT;
                        }
                        else if (!fStringSort)
                        {
                            //
                            //  Use WORD sort method.
                            //
                            if (sm2 != PUNCTUATION)
                            {
                                //
                                //  The character in the second string is
                                //  NOT punctuation.
                                //
                                if (WhichPunct2)
                                {
                                    //
                                    //  Set WP 2 to show that string 2 is
                                    //  smaller, since a punctuation char had
                                    //  already been found at an earlier
                                    //  position in string 2.
                                    //
                                    //  Set the Ignore Punctuation flag so we
                                    //  just skip over any other punctuation
                                    //  chars in the string.
                                    //
                                    WhichPunct2 = CSTR_GREATER_THAN;
                                    fIgnorePunct = TRUE;
                                }
                                else
                                {
                                    //
                                    //  Set WP 1 to show that string 2 is
                                    //  smaller, and that string 1 has had
                                    //  a punctuation char - since no
                                    //  punctuation chars have been found
                                    //  in string 2.
                                    //
                                    WhichPunct1 = CSTR_GREATER_THAN;
                                }

                                //
                                //  Advance pointer 1 and decrement counter 1.
                                //
                                pString1++;
                                ctr1--;
                                Weight1 = CMP_INVALID_WEIGHT;
                            }

                            //
                            //  Do NOT want to advance the pointer in string 1
                            //  if string 2 is also a punctuation char.  This
                            //  will be done later.
                            //
                        }

                        break;
                    }
                    case ( EXPANSION ) :
                    {
                        //
                        //  Save pointer in pString1 so that it can be
                        //  restored.
                        //
                        if (pSave1 == NULL)
                        {
                            pSave1 = pString1;
                        }
                        pString1 = pTmpBuf1;

                        //
                        //  Add one to counter so that subtraction doesn't end
                        //  comparison prematurely.
                        //
                        ctr1++;

                        //
                        //  Expand character into temporary buffer.
                        //
                        pTmpBuf1[0] = GET_EXPANSION_1(&Weight1);
                        pTmpBuf1[1] = GET_EXPANSION_2(&Weight1);

                        //
                        //  Set cExpChar1 to the number of expansion characters
                        //  stored.
                        //
                        cExpChar1 = MAX_TBL_EXPANSION;

                        Weight1 = CMP_INVALID_WEIGHT;
                        break;
                    }
                    case ( FAREAST_SPECIAL ) :
                    {
                        //
                        //  Get the weight for the far east special case
                        //  and store it in Weight1.
                        //
                        GET_FAREAST_WEIGHT( Weight1,
                                            uw1,
                                            Mask,
                                            lpString1,
                                            pString1,
                                            ExtraWt1,
                                            fModify );

                        if (sm2 != FAREAST_SPECIAL)
                        {
                            //
                            //  The character in the second string is
                            //  NOT a fareast special char.
                            //
                            //  Set each of weights 4, 5, 6, and 7 to show
                            //  that string 2 is smaller (if not already set).
                            //
                            if ((GET_WT_FOUR(&WhichExtra) == 0) &&
                                (GET_WT_FOUR(&ExtraWt1) != 0))
                            {
                                GET_WT_FOUR(&WhichExtra) = CSTR_GREATER_THAN;
                            }
                            if ((GET_WT_FIVE(&WhichExtra) == 0) &&
                                (GET_WT_FIVE(&ExtraWt1) != 0))
                            {
                                GET_WT_FIVE(&WhichExtra) = CSTR_GREATER_THAN;
                            }
                            if ((GET_WT_SIX(&WhichExtra) == 0) &&
                                (GET_WT_SIX(&ExtraWt1) != 0))
                            {
                                GET_WT_SIX(&WhichExtra) = CSTR_GREATER_THAN;
                            }
                            if ((GET_WT_SEVEN(&WhichExtra) == 0) &&
                                (GET_WT_SEVEN(&ExtraWt1) != 0))
                            {
                                GET_WT_SEVEN(&WhichExtra) = CSTR_GREATER_THAN;
                            }
                        }

                        break;
                    }
                    case ( RESERVED_2 ) :
                    case ( RESERVED_3 ) :
                    case ( UNSORTABLE ) :
                    {
                        //
                        //  Fill out the case statement so the compiler
                        //  will use a jump table.
                        //
                        break;
                    }
                }

                //
                //  Switch on the script member of string 2 and take care
                //  of any special cases.
                //
                switch (sm2)
                {
                    case ( NONSPACE_MARK ) :
                    {
                        //
                        //  Nonspace only - look at diacritic weight only.
                        //
                        if (!fIgnoreDiacritic)
                        {
                            if ((WhichDiacritic == 0) ||
                                (State & STATE_REVERSE_DW))

                            {
                                WhichDiacritic = CSTR_LESS_THAN;

                                //
                                //  Remove state from state machine.
                                //
                                REMOVE_STATE(STATE_DW);
                            }
                        }

                        //
                        //  Adjust pointer and counter and set flags.
                        //
                        pString2++;
                        ctr2--;
                        Weight2 = CMP_INVALID_WEIGHT;

                        break;
                    }
                    case ( SYMBOL_1 ) :
                    case ( SYMBOL_2 ) :
                    case ( SYMBOL_3 ) :
                    case ( SYMBOL_4 ) :
                    case ( SYMBOL_5 ) :
                    {
                        //
                        //  If the ignore symbol flag is set, then skip over
                        //  the symbol.
                        //
                        if (fIgnoreSymbol)
                        {
                            pString2++;
                            ctr2--;
                            Weight2 = CMP_INVALID_WEIGHT;
                        }

                        break;
                    }
                    case ( PUNCTUATION ) :
                    {
                        //
                        //  If the ignore punctuation flag is set, then
                        //  skip over the punctuation char.
                        //
                        if (fIgnorePunct)
                        {
                            //
                            //  Advance pointer 2 and decrement counter 2.
                            //
                            pString2++;
                            ctr2--;
                            Weight2 = CMP_INVALID_WEIGHT;
                        }
                        else if (!fStringSort)
                        {
                            //
                            //  Use WORD sort method.
                            //
                            if (sm1 != PUNCTUATION)
                            {
                                //
                                //  The character in the first string is
                                //  NOT punctuation.
                                //
                                if (WhichPunct1)
                                {
                                    //
                                    //  Set WP 1 to show that string 1 is
                                    //  smaller, since a punctuation char had
                                    //  already been found at an earlier
                                    //  position in string 1.
                                    //
                                    //  Set the Ignore Punctuation flag so we
                                    //  just skip over any other punctuation
                                    //  chars in the string.
                                    //
                                    WhichPunct1 = CSTR_LESS_THAN;
                                    fIgnorePunct = TRUE;
                                }
                                else
                                {
                                    //
                                    //  Set WP 2 to show that string 1 is
                                    //  smaller, and that string 2 has had
                                    //  a punctuation char - since no
                                    //  punctuation chars have been found
                                    //  in string 1.
                                    //
                                    WhichPunct2 = CSTR_LESS_THAN;
                                }

                                //
                                //  Pointer 2 and counter 2 will be updated
                                //  after if-else statement.
                                //
                            }
                            else
                            {
                                //
                                //  Both code points are punctuation chars.
                                //
                                //  See if either of the strings has encountered
                                //  punctuation chars previous to this.
                                //
                                if (WhichPunct1)
                                {
                                    //
                                    //  String 1 has had a punctuation char, so
                                    //  it should be the smaller string (since
                                    //  both have punctuation chars).
                                    //
                                    WhichPunct1 = CSTR_LESS_THAN;
                                }
                                else if (WhichPunct2)
                                {
                                    //
                                    //  String 2 has had a punctuation char, so
                                    //  it should be the smaller string (since
                                    //  both have punctuation chars).
                                    //
                                    WhichPunct2 = CSTR_GREATER_THAN;
                                }
                                else
                                {
                                    //
                                    //  Position is the same, so compare the
                                    //  special weights.   Set WhichPunct1 to
                                    //  the smaller special weight.
                                    //
                                    WhichPunct1 = (((GET_ALPHA_NUMERIC(&Weight1) <
                                                     GET_ALPHA_NUMERIC(&Weight2)))
                                                    ? CSTR_LESS_THAN
                                                    : CSTR_GREATER_THAN);
                                }

                                //
                                //  Set the Ignore Punctuation flag.
                                //
                                fIgnorePunct = TRUE;

                                //
                                //  Advance pointer 1 and decrement counter 1.
                                //  Pointer 2 and counter 2 will be updated
                                //  after if-else statement.
                                //
                                pString1++;
                                ctr1--;
                                Weight1 = CMP_INVALID_WEIGHT;
                            }

                            //
                            //  Advance pointer 2 and decrement counter 2.
                            //
                            pString2++;
                            ctr2--;
                            Weight2 = CMP_INVALID_WEIGHT;
                        }

                        break;
                    }
                    case ( EXPANSION ) :
                    {
                        //
                        //  Save pointer in pString1 so that it can be restored.
                        //
                        if (pSave2 == NULL)
                        {
                            pSave2 = pString2;
                        }
                        pString2 = pTmpBuf2;

                        //
                        //  Add one to counter so that subtraction doesn't end
                        //  comparison prematurely.
                        //
                        ctr2++;

                        //
                        //  Expand character into temporary buffer.
                        //
                        pTmpBuf2[0] = GET_EXPANSION_1(&Weight2);
                        pTmpBuf2[1] = GET_EXPANSION_2(&Weight2);

                        //
                        //  Set cExpChar2 to the number of expansion characters
                        //  stored.
                        //
                        cExpChar2 = MAX_TBL_EXPANSION;

                        Weight2 = CMP_INVALID_WEIGHT;
                        break;
                    }
                    case ( FAREAST_SPECIAL ) :
                    {
                        //
                        //  Get the weight for the far east special case
                        //  and store it in Weight2.
                        //
                        GET_FAREAST_WEIGHT( Weight2,
                                            uw2,
                                            Mask,
                                            lpString2,
                                            pString2,
                                            ExtraWt2,
                                            fModify );

                        if (sm1 != FAREAST_SPECIAL)
                        {
                            //
                            //  The character in the first string is
                            //  NOT a fareast special char.
                            //
                            //  Set each of weights 4, 5, 6, and 7 to show
                            //  that string 1 is smaller (if not already set).
                            //
                            if ((GET_WT_FOUR(&WhichExtra) == 0) &&
                                (GET_WT_FOUR(&ExtraWt2) != 0))
                            {
                                GET_WT_FOUR(&WhichExtra) = CSTR_LESS_THAN;
                            }
                            if ((GET_WT_FIVE(&WhichExtra) == 0) &&
                                (GET_WT_FIVE(&ExtraWt2) != 0))
                            {
                                GET_WT_FIVE(&WhichExtra) = CSTR_LESS_THAN;
                            }
                            if ((GET_WT_SIX(&WhichExtra) == 0) &&
                                (GET_WT_SIX(&ExtraWt2) != 0))
                            {
                                GET_WT_SIX(&WhichExtra) = CSTR_LESS_THAN;
                            }
                            if ((GET_WT_SEVEN(&WhichExtra) == 0) &&
                                (GET_WT_SEVEN(&ExtraWt2) != 0))
                            {
                                GET_WT_SEVEN(&WhichExtra) = CSTR_LESS_THAN;
                            }
                        }
                        else
                        {
                            //
                            //  Characters in both strings are fareast
                            //  special chars.
                            //
                            //  Set each of weights 4, 5, 6, and 7
                            //  appropriately (if not already set).
                            //
                            if ( (GET_WT_FOUR(&WhichExtra) == 0) &&
                                 ( GET_WT_FOUR(&ExtraWt1) !=
                                   GET_WT_FOUR(&ExtraWt2) ) )
                            {
                                GET_WT_FOUR(&WhichExtra) =
                                  ( GET_WT_FOUR(&ExtraWt1) <
                                    GET_WT_FOUR(&ExtraWt2) )
                                  ? CSTR_LESS_THAN
                                  : CSTR_GREATER_THAN;
                            }
                            if ( (GET_WT_FIVE(&WhichExtra) == 0) &&
                                 ( GET_WT_FIVE(&ExtraWt1) !=
                                   GET_WT_FIVE(&ExtraWt2) ) )
                            {
                                GET_WT_FIVE(&WhichExtra) =
                                  ( GET_WT_FIVE(&ExtraWt1) <
                                    GET_WT_FIVE(&ExtraWt2) )
                                  ? CSTR_LESS_THAN
                                  : CSTR_GREATER_THAN;
                            }
                            if ( (GET_WT_SIX(&WhichExtra) == 0) &&
                                 ( GET_WT_SIX(&ExtraWt1) !=
                                   GET_WT_SIX(&ExtraWt2) ) )
                            {
                                GET_WT_SIX(&WhichExtra) =
                                  ( GET_WT_SIX(&ExtraWt1) <
                                    GET_WT_SIX(&ExtraWt2) )
                                  ? CSTR_LESS_THAN
                                  : CSTR_GREATER_THAN;
                            }
                            if ( (GET_WT_SEVEN(&WhichExtra) == 0) &&
                                 ( GET_WT_SEVEN(&ExtraWt1) !=
                                   GET_WT_SEVEN(&ExtraWt2) ) )
                            {
                                GET_WT_SEVEN(&WhichExtra) =
                                  ( GET_WT_SEVEN(&ExtraWt1) <
                                    GET_WT_SEVEN(&ExtraWt2) )
                                  ? CSTR_LESS_THAN
                                  : CSTR_GREATER_THAN;
                            }
                        }

                        break;
                    }
                    case ( RESERVED_2 ) :
                    case ( RESERVED_3 ) :
                    case ( UNSORTABLE ) :
                    {
                        //
                        //  Fill out the case statement so the compiler
                        //  will use a jump table.
                        //
                        break;
                    }
                }

                //
                //  See if the comparison should start again.
                //
                if ((Weight1 == CMP_INVALID_WEIGHT) || (Weight2 == CMP_INVALID_WEIGHT))
                {
                    //
                    //  Check to see if we're modifying the script value.
                    //  If so, then we need to reset the fareast weight
                    //  (if applicable) so that it doesn't get modified
                    //  again.
                    //
                    if (fModify == TRUE)
                    {
                        if (sm1 == FAREAST_SPECIAL)
                        {
                            Weight1 = CMP_INVALID_WEIGHT;
                        }
                        else if (sm2 == FAREAST_SPECIAL)
                        {
                            Weight2 = CMP_INVALID_WEIGHT;
                        }
                    }
                    continue;
                }

                //
                //  We're not supposed to drop down into the state table if
                //  the unicode weights are different, so stop comparison
                //  and return result of unicode weight comparison.
                //
                if (uw1 != uw2)
                {
                    return ((uw1 < uw2) ? CSTR_LESS_THAN : CSTR_GREATER_THAN);
                }
            }

            //
            //  For each state in the state table, do the appropriate
            //  comparisons.
            //
            if (State & (STATE_DW | STATE_REVERSE_DW))
            {
                //
                //  Get the diacritic weights.
                //
                dw1 = GET_DIACRITIC(&Weight1);
                dw2 = GET_DIACRITIC(&Weight2);

                if (dw1 != dw2)
                {
                    //
                    //  Look ahead to see if diacritic follows a
                    //  minimum diacritic weight.  If so, get the
                    //  diacritic weight of the nonspace mark.
                    //
                    while (!AT_STRING_END(ctr1 - 1, pString1 + 1, cchCount1))
                    {
                        Wt = GET_DWORD_WEIGHT(pHashN, *(pString1 + 1));
                        if (GET_SCRIPT_MEMBER(&Wt) == NONSPACE_MARK)
                        {
                            dw1 += GET_DIACRITIC(&Wt);
                            pString1++;
                            ctr1--;
                        }
                        else
                        {
                            break;
                        }
                    }

                    while (!AT_STRING_END(ctr2 - 1, pString2 + 1, cchCount2))
                    {
                        Wt = GET_DWORD_WEIGHT(pHashN, *(pString2 + 1));
                        if (GET_SCRIPT_MEMBER(&Wt) == NONSPACE_MARK)
                        {
                            dw2 += GET_DIACRITIC(&Wt);
                            pString2++;
                            ctr2--;
                        }
                        else
                        {
                            break;
                        }
                    }

                    //
                    //  Save which string has the smaller diacritic
                    //  weight if the diacritic weights are still
                    //  different.
                    //
                    if (dw1 != dw2)
                    {
                        WhichDiacritic = (dw1 < dw2)
                                           ? CSTR_LESS_THAN
                                           : CSTR_GREATER_THAN;

                        //
                        //  Remove state from state machine.
                        //
                        REMOVE_STATE(STATE_DW);
                    }
                }
            }
            if (State & STATE_CW)
            {
                //
                //  Get the case weights.
                //
                if (GET_CASE(&Weight1) != GET_CASE(&Weight2))
                {
                    //
                    //  Save which string has the smaller case weight.
                    //
                    WhichCase = (GET_CASE(&Weight1) < GET_CASE(&Weight2))
                                  ? CSTR_LESS_THAN
                                  : CSTR_GREATER_THAN;

                    //
                    //  Remove state from state machine.
                    //
                    REMOVE_STATE(STATE_CW);
                }
            }
        }

        //
        //  Fixup the pointers and counters.
        //
        POINTER_FIXUP();
        ctr1--;
        ctr2--;

        //
        //  Reset the weights to be invalid.
        //
        Weight1 = CMP_INVALID_WEIGHT;
        Weight2 = CMP_INVALID_WEIGHT;
    }

    //
    //  If the end of BOTH strings has been reached, then the unicode
    //  weights match exactly.  Check the diacritic, case and special
    //  weights.  If all are zero, then return success.  Otherwise,
    //  return the result of the weight difference.
    //
    //  NOTE:  The following checks MUST REMAIN IN THIS ORDER:
    //            Diacritic, Case, Punctuation.
    //
    if (AT_STRING_END(ctr1, pString1, cchCount1))
    {
        if (AT_STRING_END(ctr2, pString2, cchCount2))
        {
            if (WhichDiacritic)
            {
                return (WhichDiacritic);
            }
            if (WhichCase)
            {
                return (WhichCase);
            }
            if (WhichExtra)
            {
                if (!fIgnoreDiacritic)
                {
                    if (GET_WT_FOUR(&WhichExtra))
                    {
                        return (GET_WT_FOUR(&WhichExtra));
                    }
                    if (GET_WT_FIVE(&WhichExtra))
                    {
                        return (GET_WT_FIVE(&WhichExtra));
                    }
                }
                if (GET_WT_SIX(&WhichExtra))
                {
                    return (GET_WT_SIX(&WhichExtra));
                }
                if (GET_WT_SEVEN(&WhichExtra))
                {
                    return (GET_WT_SEVEN(&WhichExtra));
                }
            }
            if (WhichPunct1)
            {
                return (WhichPunct1);
            }
            if (WhichPunct2)
            {
                return (WhichPunct2);
            }

            return (CSTR_EQUAL);
        }
        else
        {
            //
            //  String 2 is longer.
            //
            pString1 = pString2;
            ctr1 = ctr2;
            cchCount1 = cchCount2;
            fEnd1 = CSTR_LESS_THAN;
        }
    }
    else
    {
        fEnd1 = CSTR_GREATER_THAN;
    }

    //
    //  Scan to the end of the longer string.
    //
    SCAN_LONGER_STRING( ctr1,
                        pString1,
                        cchCount1,
                        fEnd1 );
}
