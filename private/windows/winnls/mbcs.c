/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    mbcs.c

Abstract:

    This file contains functions that convert multibyte character strings
    to wide character strings, convert wide character strings to multibyte
    character strings, convert a multibyte character string from one code
    page to a multibyte character string of another code page, and get the
    DBCS leadbyte ranges for a given code page.

    APIs found in this file:
      IsValidCodePage
      GetACP
      GetOEMCP
      GetCPInfo
      GetCPInfoExW
      IsDBCSLeadByte
      IsDBCSLeadByteEx
      MultiByteToWideChar
      WideCharToMultiByte

Revision History:

    05-31-91    JulieB    Created.

--*/



//
//  Include Files.
//

#include "nls.h"




//
//  Forward Declarations.
//

BOOL
GetLocalizedCodePageName(
    UINT CodePage,
    LPWSTR pCPName);

int
GetWCCompSB(
    PMB_TABLE pMBTbl,
    LPBYTE pMBStr,
    LPWSTR pWCStr,
    LPWSTR pEndWCStr);

int
GetWCCompMB(
    PCP_HASH pHashN,
    PMB_TABLE pMBTbl,
    LPBYTE pMBStr,
    LPBYTE pEndMBStr,
    LPWSTR pWCStr,
    LPWSTR pEndWCStr,
    int *pmbIncr);

int
GetWCCompSBErr(
    PCP_HASH pHashN,
    PMB_TABLE pMBTbl,
    LPBYTE pMBStr,
    LPWSTR pWCStr,
    LPWSTR pEndWCStr);

int
GetWCCompMBErr(
    PCP_HASH pHashN,
    PMB_TABLE pMBTbl,
    LPBYTE pMBStr,
    LPBYTE pEndMBStr,
    LPWSTR pWCStr,
    LPWSTR pEndWCStr,
    int *pmbIncr);

int
GetMBNoDefault(
    PCP_HASH pHashN,
    LPWSTR pWCStr,
    LPWSTR pEndWCStr,
    LPBYTE pMBStr,
    int cbMultiByte,
    DWORD dwFlags);

int
GetMBDefault(
    PCP_HASH pHashN,
    LPWSTR pWCStr,
    LPWSTR pEndWCStr,
    LPBYTE pMBStr,
    int cbMultiByte,
    WORD wDefault,
    LPBOOL pUsedDef,
    DWORD dwFlags);

int
GetMBDefaultComp(
    PCP_HASH pHashN,
    LPWSTR pWCStr,
    LPWSTR pEndWCStr,
    LPBYTE pMBStr,
    int cbMultiByte,
    WORD wDefault,
    LPBOOL pUsedDef,
    DWORD dwFlags);

int
GetMBCompSB(
    PCP_HASH pHashN,
    DWORD dwFlags,
    LPWSTR pWCStr,
    LPBYTE pMBStr,
    int mbCount,
    WORD wDefault,
    LPBOOL pUsedDef);

int
GetMBCompMB(
    PCP_HASH pHashN,
    DWORD dwFlags,
    LPWSTR pWCStr,
    LPBYTE pMBStr,
    int mbCount,
    WORD wDefault,
    LPBOOL pUsedDef,
    BOOL *fError,
    BOOL fOnlyOne);

UINT
GetMacCodePage(void);





//-------------------------------------------------------------------------//
//                           INTERNAL MACROS                               //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  CHECK_DBCS_LEAD_BYTE
//
//  Returns the offset to the DBCS table for the given leadbyte character.
//  If the given character is not a leadbyte, then it returns zero (table
//  value).
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define CHECK_DBCS_LEAD_BYTE(pDBCSOff, Ch)                                 \
    (pDBCSOff ? ((WORD)(pDBCSOff[Ch])) : ((WORD)0))


////////////////////////////////////////////////////////////////////////////
//
//  CHECK_ERROR_WC_SINGLE
//
//  Checks to see if the default character was used due to an invalid
//  character.  Sets last error and returns 0 characters written if an
//  invalid character was used.
//
//  NOTE: This macro may return if an error is encountered.
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define CHECK_ERROR_WC_SINGLE( pHashN,                                     \
                               wch,                                        \
                               Ch )                                        \
{                                                                          \
    if ( ( (wch == pHashN->pCPInfo->wUniDefaultChar) &&                    \
           (Ch != pHashN->pCPInfo->wTransUniDefaultChar) ) ||              \
         ( (wch >= PRIVATE_USE_BEGIN) && (wch <= PRIVATE_USE_END) ) )      \
    {                                                                      \
        SetLastError(ERROR_NO_UNICODE_TRANSLATION);                        \
        return (0);                                                        \
    }                                                                      \
}


////////////////////////////////////////////////////////////////////////////
//
//  CHECK_ERROR_WC_MULTI
//
//  Checks to see if the default character was used due to an invalid
//  character.  Sets last error and returns 0 characters written if an
//  invalid character was used.
//
//  NOTE: This macro may return if an error is encountered.
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define CHECK_ERROR_WC_MULTI( pHashN,                                      \
                              wch,                                         \
                              lead,                                        \
                              trail )                                      \
{                                                                          \
    if ((wch == pHashN->pCPInfo->wUniDefaultChar) &&                       \
        (MAKEWORD(trail, lead) != pHashN->pCPInfo->wTransUniDefaultChar))  \
    {                                                                      \
        SetLastError(ERROR_NO_UNICODE_TRANSLATION);                        \
        return (0);                                                        \
    }                                                                      \
}


////////////////////////////////////////////////////////////////////////////
//
//  CHECK_ERROR_WC_MULTI_SPECIAL
//
//  Checks to see if the default character was used due to an invalid
//  character.  Sets it to 0xffff if invalid.
//
//  DEFINED AS A MACRO.
//
//  08-21-95    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define CHECK_ERROR_WC_MULTI_SPECIAL( pHashN,                              \
                                      pWCStr,                              \
                                      lead,                                \
                                      trail )                              \
{                                                                          \
    if ((*pWCStr == pHashN->pCPInfo->wUniDefaultChar) &&                   \
        (MAKEWORD(trail, lead) != pHashN->pCPInfo->wTransUniDefaultChar))  \
    {                                                                      \
        *pWCStr = 0xffff;                                                  \
    }                                                                      \
}


////////////////////////////////////////////////////////////////////////////
//
//  GET_WC_SINGLE
//
//  Fills in pWCStr with the wide character(s) for the corresponding single
//  byte character from the appropriate translation table.
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define GET_WC_SINGLE( pMBTbl,                                             \
                       pMBStr,                                             \
                       pWCStr )                                            \
{                                                                          \
    *pWCStr = pMBTbl[*pMBStr];                                             \
}


////////////////////////////////////////////////////////////////////////////
//
//  GET_WC_SINGLE_SPECIAL
//
//  Fills in pWCStr with the wide character(s) for the corresponding single
//  byte character from the appropriate translation table.  Also checks for
//  invalid characters - if invalid, it fills in 0xffff instead.
//
//  DEFINED AS A MACRO.
//
//  08-21-95    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define GET_WC_SINGLE_SPECIAL( pHashN,                                     \
                               pMBTbl,                                     \
                               pMBStr,                                     \
                               pWCStr )                                    \
{                                                                          \
    *pWCStr = pMBTbl[*pMBStr];                                             \
                                                                           \
    if ( ( (*pWCStr == pHashN->pCPInfo->wUniDefaultChar) &&                \
           (*pMBStr != pHashN->pCPInfo->wTransUniDefaultChar) ) ||         \
         ( (*pWCStr >= PRIVATE_USE_BEGIN) &&                               \
           (*pWCStr <= PRIVATE_USE_END) ) )                                \
    {                                                                      \
        *pWCStr = 0xffff;                                                  \
    }                                                                      \
}


////////////////////////////////////////////////////////////////////////////
//
//  GET_WC_MULTI
//
//  Fills in pWCStr with the wide character(s) for the corresponding multibyte
//  character from the appropriate translation table.  The number of bytes
//  used from the pMBStr buffer (single byte or double byte) is stored in
//  the mbIncr parameter.
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define GET_WC_MULTI( pHashN,                                              \
                      pMBTbl,                                              \
                      pMBStr,                                              \
                      pEndMBStr,                                           \
                      pWCStr,                                              \
                      pEndWCStr,                                           \
                      mbIncr )                                             \
{                                                                          \
    WORD Offset;                  /* offset to DBCS table for range */     \
                                                                           \
                                                                           \
    if (Offset = CHECK_DBCS_LEAD_BYTE(pHashN->pDBCSOffsets, *pMBStr))      \
    {                                                                      \
        /*                                                                 \
         *  DBCS Lead Byte.  Make sure there is a trail byte with the      \
         *  lead byte.                                                     \
         */                                                                \
        if (pMBStr + 1 == pEndMBStr)                                       \
        {                                                                  \
            /*                                                             \
             *  There is no trail byte with the lead byte.  The lead byte  \
             *  is the LAST character in the string.  Translate to NULL.   \
             */                                                            \
            *pWCStr = (WCHAR)0;                                            \
            mbIncr = 1;                                                    \
        }                                                                  \
        else if (*(pMBStr + 1) == 0)                                       \
        {                                                                  \
            /*                                                             \
             *  There is no trail byte with the lead byte.  The lead byte  \
             *  is followed by a NULL.  Translate to NULL.                 \
             *                                                             \
             *  Increment by 2 so that the null is not counted twice.      \
             */                                                            \
            *pWCStr = (WCHAR)0;                                            \
            mbIncr = 2;                                                    \
        }                                                                  \
        else                                                               \
        {                                                                  \
            /*                                                             \
             *  Fill in the wide character translation from the double     \
             *  byte character table.                                      \
             */                                                            \
            *pWCStr = (pHashN->pDBCSOffsets + Offset)[*(pMBStr + 1)];      \
            mbIncr = 2;                                                    \
        }                                                                  \
    }                                                                      \
    else                                                                   \
    {                                                                      \
        /*                                                                 \
         *  Not DBCS Lead Byte.  Fill in the wide character translation    \
         *  from the single byte character table.                          \
         */                                                                \
        *pWCStr = pMBTbl[*pMBStr];                                         \
        mbIncr = 1;                                                        \
    }                                                                      \
}


////////////////////////////////////////////////////////////////////////////
//
//  GET_WC_MULTI_ERR
//
//  Fills in pWCStr with the wide character(s) for the corresponding multibyte
//  character from the appropriate translation table.  The number of bytes
//  used from the pMBStr buffer (single byte or double byte) is stored in
//  the mbIncr parameter.
//
//  Once the character has been translated, it checks to be sure the
//  character was valid.  If not, it sets last error and return 0 characters
//  written.
//
//  NOTE: This macro may return if an error is encountered.
//
//  DEFINED AS A MACRO.
//
//  09-01-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define GET_WC_MULTI_ERR( pHashN,                                          \
                          pMBTbl,                                          \
                          pMBStr,                                          \
                          pEndMBStr,                                       \
                          pWCStr,                                          \
                          pEndWCStr,                                       \
                          mbIncr )                                         \
{                                                                          \
    WORD Offset;                  /* offset to DBCS table for range */     \
                                                                           \
                                                                           \
    if (Offset = CHECK_DBCS_LEAD_BYTE(pHashN->pDBCSOffsets, *pMBStr))      \
    {                                                                      \
        /*                                                                 \
         *  DBCS Lead Byte.  Make sure there is a trail byte with the      \
         *  lead byte.                                                     \
         */                                                                \
        if ((pMBStr + 1 == pEndMBStr) || (*(pMBStr + 1) == 0))             \
        {                                                                  \
            /*                                                             \
             *  There is no trail byte with the lead byte.  Return error.  \
             */                                                            \
            SetLastError(ERROR_NO_UNICODE_TRANSLATION);                    \
            return (0);                                                    \
        }                                                                  \
                                                                           \
        /*                                                                 \
         *  Fill in the wide character translation from the double         \
         *  byte character table.                                          \
         */                                                                \
        *pWCStr = (pHashN->pDBCSOffsets + Offset)[*(pMBStr + 1)];          \
        mbIncr = 2;                                                        \
                                                                           \
        /*                                                                 \
         *  Make sure an invalid character was not translated to           \
         *  the default char.  Return an error if invalid.                 \
         */                                                                \
        CHECK_ERROR_WC_MULTI( pHashN,                                      \
                              *pWCStr,                                     \
                              *pMBStr,                                     \
                              *(pMBStr + 1) );                             \
    }                                                                      \
    else                                                                   \
    {                                                                      \
        /*                                                                 \
         *  Not DBCS Lead Byte.  Fill in the wide character translation    \
         *  from the single byte character table.                          \
         */                                                                \
        *pWCStr = pMBTbl[*pMBStr];                                         \
        mbIncr = 1;                                                        \
                                                                           \
        /*                                                                 \
         *  Make sure an invalid character was not translated to           \
         *  the default char.  Return an error if invalid.                 \
         */                                                                \
        CHECK_ERROR_WC_SINGLE( pHashN,                                     \
                               *pWCStr,                                    \
                               *pMBStr );                                  \
    }                                                                      \
}


////////////////////////////////////////////////////////////////////////////
//
//  GET_WC_MULTI_ERR_SPECIAL
//
//  Fills in pWCStr with the wide character(s) for the corresponding multibyte
//  character from the appropriate translation table.  The number of bytes
//  used from the pMBStr buffer (single byte or double byte) is stored in
//  the mbIncr parameter.
//
//  Once the character has been translated, it checks to be sure the
//  character was valid.  If not, it fills in 0xffff.
//
//  DEFINED AS A MACRO.
//
//  08-21-95    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define GET_WC_MULTI_ERR_SPECIAL( pHashN,                                  \
                                  pMBTbl,                                  \
                                  pMBStr,                                  \
                                  pEndMBStr,                               \
                                  pWCStr,                                  \
                                  pEndWCStr,                               \
                                  mbIncr )                                 \
{                                                                          \
    WORD Offset;                  /* offset to DBCS table for range */     \
                                                                           \
                                                                           \
    if (Offset = CHECK_DBCS_LEAD_BYTE(pHashN->pDBCSOffsets, *pMBStr))      \
    {                                                                      \
        /*                                                                 \
         *  DBCS Lead Byte.  Make sure there is a trail byte with the      \
         *  lead byte.                                                     \
         */                                                                \
        if ((pMBStr + 1 == pEndMBStr) || (*(pMBStr + 1) == 0))             \
        {                                                                  \
            /*                                                             \
             *  There is no trail byte with the lead byte.  The lead byte  \
             *  is the LAST character in the string.  Translate to 0xffff. \
             */                                                            \
            *pWCStr = (WCHAR)0xffff;                                       \
            mbIncr = 1;                                                    \
        }                                                                  \
        else                                                               \
        {                                                                  \
            /*                                                             \
             *  Fill in the wide character translation from the double     \
             *  byte character table.                                      \
             */                                                            \
            *pWCStr = (pHashN->pDBCSOffsets + Offset)[*(pMBStr + 1)];      \
            mbIncr = 2;                                                    \
                                                                           \
            /*                                                             \
             *  Make sure an invalid character was not translated to       \
             *  the default char.  Translate to 0xffff if invalid.         \
             */                                                            \
            CHECK_ERROR_WC_MULTI_SPECIAL( pHashN,                          \
                                          pWCStr,                          \
                                          *pMBStr,                         \
                                          *(pMBStr + 1) );                 \
        }                                                                  \
    }                                                                      \
    else                                                                   \
    {                                                                      \
        /*                                                                 \
         *  Not DBCS Lead Byte.  Fill in the wide character translation    \
         *  from the single byte character table.                          \
         *  Make sure an invalid character was not translated to           \
         *  the default char.  Return an error if invalid.                 \
         */                                                                \
        GET_WC_SINGLE_SPECIAL( pHashN,                                     \
                               pMBTbl,                                     \
                               pMBStr,                                     \
                               pWCStr );                                   \
        mbIncr = 1;                                                        \
    }                                                                      \
}


////////////////////////////////////////////////////////////////////////////
//
//  COPY_MB_CHAR
//
//  Copies a multibyte character to the given string buffer.  If the
//  high byte of the multibyte word is zero, then it is a single byte
//  character and the number of characters written (returned) is 1.
//  Otherwise, it is a double byte character and the number of characters
//  written (returned) is 2.
//
//  NumByte will be 0 if the buffer is too small for the translation.
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define COPY_MB_CHAR( mbChar,                                              \
                      pMBStr,                                              \
                      NumByte,                                             \
                      fOnlyOne )                                           \
{                                                                          \
    if (HIBYTE(mbChar))                                                    \
    {                                                                      \
        /*                                                                 \
         *  Make sure there is enough room in the buffer for both bytes.   \
         */                                                                \
        if (fOnlyOne)                                                      \
        {                                                                  \
            NumByte = 0;                                                   \
        }                                                                  \
        else                                                               \
        {                                                                  \
            /*                                                             \
             *  High Byte is NOT zero, so it's a DOUBLE byte char.         \
             *  Return 2 characters written.                               \
             */                                                            \
            *pMBStr = HIBYTE(mbChar);                                      \
            *(pMBStr + 1) = LOBYTE(mbChar);                                \
            NumByte = 2;                                                   \
        }                                                                  \
    }                                                                      \
    else                                                                   \
    {                                                                      \
        /*                                                                 \
         *  High Byte IS zero, so it's a SINGLE byte char.                 \
         *  Return 1 character written.                                    \
         */                                                                \
        *pMBStr = LOBYTE(mbChar);                                          \
        NumByte = 1;                                                       \
    }                                                                      \
}


////////////////////////////////////////////////////////////////////////////
//
//  GET_SB
//
//  Fills in pMBStr with the single byte character for the corresponding
//  wide character from the appropriate translation table.
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define GET_SB( pWC,                                                       \
                wChar,                                                     \
                pMBStr )                                                   \
{                                                                          \
    *pMBStr = ((BYTE *)(pWC))[wChar];                                      \
}


////////////////////////////////////////////////////////////////////////////
//
//  GET_MB
//
//  Fills in pMBStr with the multi byte character for the corresponding
//  wide character from the appropriate translation table.
//
//  mbCount will be 0 if the buffer is too small for the translation.
//
//    Broken Down Version:
//    --------------------
//        mbChar = ((WORD *)(pHashN->pWC))[wChar];
//        COPY_MB_CHAR(mbChar, pMBStr, mbCount);
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define GET_MB( pWC,                                                       \
                wChar,                                                     \
                pMBStr,                                                    \
                mbCount,                                                   \
                fOnlyOne )                                                 \
{                                                                          \
    COPY_MB_CHAR( ((WORD *)(pWC))[wChar],                                  \
                  pMBStr,                                                  \
                  mbCount,                                                 \
                  fOnlyOne );                                              \
}


////////////////////////////////////////////////////////////////////////////
//
//  ELIMINATE_BEST_FIT_SB
//
//  Checks to see if a single byte Best Fit character was used.  If so,
//  it replaces it with a single byte default character.
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define ELIMINATE_BEST_FIT_SB( pHashN,                                     \
                               wChar,                                      \
                               pMBStr )                                    \
{                                                                          \
    if ((pHashN->pMBTbl)[*pMBStr] != wChar)                                \
    {                                                                      \
        *pMBStr = LOBYTE(pHashN->pCPInfo->wDefaultChar);                   \
    }                                                                      \
}


////////////////////////////////////////////////////////////////////////////
//
//  ELIMINATE_BEST_FIT_MB
//
//  Checks to see if a multi byte Best Fit character was used.  If so,
//  it replaces it with a multi byte default character.
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define ELIMINATE_BEST_FIT_MB( pHashN,                                     \
                               wChar,                                      \
                               pMBStr,                                     \
                               mbCount,                                    \
                               fOnlyOne )                                  \
{                                                                          \
    WORD Offset;                                                           \
    WORD wDefault;                                                         \
                                                                           \
    if (((mbCount == 1) && ((pHashN->pMBTbl)[*pMBStr] != wChar)) ||        \
        ((mbCount == 2) &&                                                 \
         (Offset = CHECK_DBCS_LEAD_BYTE(pHashN->pDBCSOffsets, *pMBStr)) && \
         (((pHashN->pDBCSOffsets + Offset)[*(pMBStr + 1)]) != wChar)))     \
    {                                                                      \
        wDefault = pHashN->pCPInfo->wDefaultChar;                          \
        if (HIBYTE(wDefault))                                              \
        {                                                                  \
            if (fOnlyOne)                                                  \
            {                                                              \
                mbCount = 0;                                               \
            }                                                              \
            else                                                           \
            {                                                              \
                *pMBStr = HIBYTE(wDefault);                                \
                *(pMBStr + 1) = LOBYTE(wDefault);                          \
                mbCount = 2;                                               \
            }                                                              \
        }                                                                  \
        else                                                               \
        {                                                                  \
            *pMBStr = LOBYTE(wDefault);                                    \
            mbCount = 1;                                                   \
        }                                                                  \
    }                                                                      \
}


////////////////////////////////////////////////////////////////////////////
//
//  GET_DEFAULT_WORD
//
//  Takes a pointer to a character string (either one or two characters),
//  and converts it to a WORD value.  If the character is not DBCS, then it
//  zero extends the high byte.
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define GET_DEFAULT_WORD(pOff, pDefault)                                   \
    (CHECK_DBCS_LEAD_BYTE(pOff, *pDefault)                                 \
         ? MAKEWORD(*(pDefault + 1), *pDefault)                            \
         : MAKEWORD(*pDefault, 0))


////////////////////////////////////////////////////////////////////////////
//
//  DEFAULT_CHAR_CHECK_SB
//
//  Checks to see if the default character is used.  If it is, it sets
//  pUsedDef to TRUE (if non-null).  If the user specified a default, then
//  the user's default character is used.
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define DEFAULT_CHAR_CHECK_SB( pHashN,                                     \
                               wch,                                        \
                               pMBStr,                                     \
                               wDefChar,                                   \
                               pUsedDef )                                  \
{                                                                          \
    WORD wSysDefChar = pHashN->pCPInfo->wDefaultChar;                      \
                                                                           \
                                                                           \
    /*                                                                     \
     *  Check for default character being used.                            \
     */                                                                    \
    if ((*pMBStr == (BYTE)wSysDefChar) &&                                  \
        (wch != pHashN->pCPInfo->wTransDefaultChar))                       \
    {                                                                      \
        /*                                                                 \
         *  Default was used.  Set the pUsedDef parameter to TRUE.         \
         */                                                                \
        *pUsedDef = TRUE;                                                  \
                                                                           \
        /*                                                                 \
         *  If the user specified a different default character than       \
         *  the system default, use that character instead.                \
         */                                                                \
        if (wSysDefChar != wDefChar)                                       \
        {                                                                  \
            *pMBStr = LOBYTE(wDefChar);                                    \
        }                                                                  \
    }                                                                      \
}


////////////////////////////////////////////////////////////////////////////
//
//  DEFAULT_CHAR_CHECK_MB
//
//  Checks to see if the default character is used.  If it is, it sets
//  pUsedDef to TRUE (if non-null).  If the user specified a default, then
//  the user's default character is used.  The number of bytes written to
//  the buffer is returned.
//
//  NumByte will be -1 if the buffer is too small for the translation.
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define DEFAULT_CHAR_CHECK_MB( pHashN,                                     \
                               wch,                                        \
                               pMBStr,                                     \
                               wDefChar,                                   \
                               pUsedDef,                                   \
                               NumByte,                                    \
                               fOnlyOne )                                  \
{                                                                          \
    WORD wSysDefChar = pHashN->pCPInfo->wDefaultChar;                      \
                                                                           \
                                                                           \
    /*                                                                     \
     *  Set NumByte to zero for return (zero bytes written).               \
     */                                                                    \
    NumByte = 0;                                                           \
                                                                           \
    /*                                                                     \
     *  Check for default character being used.                            \
     */                                                                    \
    if ((*pMBStr == (BYTE)wSysDefChar) &&                                  \
        (wch != pHashN->pCPInfo->wTransDefaultChar))                       \
    {                                                                      \
        /*                                                                 \
         *  Default was used.  Set the pUsedDef parameter to TRUE.         \
         */                                                                \
        *pUsedDef = TRUE;                                                  \
                                                                           \
        /*                                                                 \
         *  If the user specified a different default character than       \
         *  the system default, use that character instead.                \
         */                                                                \
        if (wSysDefChar != wDefChar)                                       \
        {                                                                  \
            COPY_MB_CHAR( wDefChar,                                        \
                          pMBStr,                                          \
                          NumByte,                                         \
                          fOnlyOne );                                      \
            if (NumByte == 0)                                              \
            {                                                              \
                NumByte = -1;                                              \
            }                                                              \
        }                                                                  \
    }                                                                      \
}


////////////////////////////////////////////////////////////////////////////
//
//  GET_WC_TRANSLATION_SB
//
//  Gets the 1:1 translation of a given wide character.  It fills in the
//  string pointer with the single byte character.
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define GET_WC_TRANSLATION_SB( pHashN,                                     \
                               wch,                                        \
                               pMBStr,                                     \
                               wDefault,                                   \
                               pUsedDef,                                   \
                               dwFlags )                                   \
{                                                                          \
    GET_SB( pHashN->pWC,                                                   \
            wch,                                                           \
            pMBStr );                                                      \
    if (dwFlags & WC_NO_BEST_FIT_CHARS)                                    \
    {                                                                      \
        ELIMINATE_BEST_FIT_SB( pHashN,                                     \
                               wch,                                        \
                               pMBStr );                                   \
    }                                                                      \
    DEFAULT_CHAR_CHECK_SB( pHashN,                                         \
                           wch,                                            \
                           pMBStr,                                         \
                           wDefault,                                       \
                           pUsedDef );                                     \
}


////////////////////////////////////////////////////////////////////////////
//
//  GET_WC_TRANSLATION_MB
//
//  Gets the 1:1 translation of a given wide character.  It fills in the
//  appropriate number of characters for the multibyte character and then
//  returns the number of characters written to the multibyte string.
//
//  mbCnt will be 0 if the buffer is too small for the translation.
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define GET_WC_TRANSLATION_MB( pHashN,                                     \
                               wch,                                        \
                               pMBStr,                                     \
                               wDefault,                                   \
                               pUsedDef,                                   \
                               mbCnt,                                      \
                               fOnlyOne,                                   \
                               dwFlags )                                   \
{                                                                          \
    int mbCnt2;              /* number of characters written */            \
                                                                           \
                                                                           \
    GET_MB( pHashN->pWC,                                                   \
            wch,                                                           \
            pMBStr,                                                        \
            mbCnt,                                                         \
            fOnlyOne );                                                    \
    if (dwFlags & WC_NO_BEST_FIT_CHARS)                                    \
    {                                                                      \
        ELIMINATE_BEST_FIT_MB( pHashN,                                     \
                               wch,                                        \
                               pMBStr,                                     \
                               mbCnt,                                      \
                               fOnlyOne );                                 \
    }                                                                      \
    if (mbCnt)                                                             \
    {                                                                      \
        DEFAULT_CHAR_CHECK_MB( pHashN,                                     \
                               wch,                                        \
                               pMBStr,                                     \
                               wDefault,                                   \
                               pUsedDef,                                   \
                               mbCnt2,                                     \
                               fOnlyOne );                                 \
        if (mbCnt2 == -1)                                                  \
        {                                                                  \
            mbCnt = 0;                                                     \
        }                                                                  \
        else if (mbCnt2)                                                   \
        {                                                                  \
            mbCnt = mbCnt2;                                                \
        }                                                                  \
    }                                                                      \
}


////////////////////////////////////////////////////////////////////////////
//
//  GET_CP_HASH_NODE
//
//  Sets the code page value (if a special value is passed in) and the
//  hash node pointer.  If the code page value is invalid, the pointer
//  to the hash node will be set to NULL.
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define GET_CP_HASH_NODE( CodePage,                                        \
                          pHashN )                                         \
{                                                                          \
    PLOC_HASH pHashLoc;                                                    \
                                                                           \
                                                                           \
    /*                                                                     \
     *  Check for the ACP, OEMCP, or MACCP.  Fill in the appropriate       \
     *  value for the code page if one of these values is given.           \
     *  Otherwise, just get the hash node for the given code page.         \
     */                                                                    \
    if (CodePage == gAnsiCodePage)                                         \
    {                                                                      \
        pHashN = gpACPHashN;                                               \
    }                                                                      \
    else if (CodePage == gOemCodePage)                                     \
    {                                                                      \
        pHashN = gpOEMCPHashN;                                             \
    }                                                                      \
    else if (CodePage == CP_ACP)                                           \
    {                                                                      \
        CodePage = gAnsiCodePage;                                          \
        pHashN = gpACPHashN;                                               \
    }                                                                      \
    else if (CodePage == CP_OEMCP)                                         \
    {                                                                      \
        CodePage = gOemCodePage;                                           \
        pHashN = gpOEMCPHashN;                                             \
    }                                                                      \
    else if (CodePage == CP_MACCP)                                         \
    {                                                                      \
        CodePage = GetMacCodePage();                                       \
        pHashN = gpMACCPHashN;                                             \
    }                                                                      \
    else if (CodePage == CP_THREAD_ACP)                                    \
    {                                                                      \
        VALIDATE_LOCALE(NtCurrentTeb()->CurrentLocale, pHashLoc, FALSE);   \
        if (pHashLoc != NULL)                                              \
        {                                                                  \
            CodePage = pHashLoc->pLocaleFixed->DefaultACP;                 \
        }                                                                  \
        if (CodePage == CP_ACP)                                            \
        {                                                                  \
            CodePage = gAnsiCodePage;                                      \
            pHashN = gpACPHashN;                                           \
        }                                                                  \
        else if (CodePage == CP_OEMCP)                                     \
        {                                                                  \
            CodePage = gOemCodePage;                                       \
            pHashN = gpOEMCPHashN;                                         \
        }                                                                  \
        else if (CodePage == CP_MACCP)                                     \
        {                                                                  \
            CodePage = GetMacCodePage();                                   \
            pHashN = gpMACCPHashN;                                         \
        }                                                                  \
        else                                                               \
        {                                                                  \
            pHashN = GetCPHashNode(CodePage);                              \
        }                                                                  \
    }                                                                      \
    else                                                                   \
    {                                                                      \
        pHashN = GetCPHashNode(CodePage);                                  \
    }                                                                      \
}




//-------------------------------------------------------------------------//
//                             API ROUTINES                                //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  IsValidCodePage
//
//  Checks that the given code page is a valid one.  It does so by querying
//  the registry.  If the code page is found, then TRUE is returned.
//  Otherwise, FALSE is returned.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI IsValidCodePage(
    UINT CodePage)
{
    //
    //  Do not allow special code page values to be valid here.
    //     (CP_ACP, CP_OEMCP, CP_MACCP, CP_THREAD_ACP, CP_SYMBOL are invalid)
    //

    //
    //  Do the quick check for the code page value equal to either
    //  the Ansi code page value or the OEM code page value.
    //
    if ((CodePage == gAnsiCodePage) || (CodePage == gOemCodePage) ||
        (CodePage == CP_UTF7) || (CodePage == CP_UTF8))
    {
        //
        //  Return success.
        //
        return (TRUE);
    }

    //
    //  Check for other code page values.
    //
    if (GetCPHashNode(CodePage) == NULL)
    {
        //
        //  Return failure.
        //
        return (FALSE);
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetACP
//
//  Returns the ANSI code page for the system.  If the registry value is
//  not readable, then the chosen default ACP is used (NLS_DEFAULT_ACP).
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

UINT WINAPI GetACP()
{
    //
    //  Return the ACP stored in the cache.
    //
    return (gAnsiCodePage);
}


////////////////////////////////////////////////////////////////////////////
//
//  SetCPGlobal
//
//  Sets the code page global, used by Setup to force the code page into
//  the correct value during GUI mode.
//
//  02-15-99    JimSchm   Created.
////////////////////////////////////////////////////////////////////////////

UINT
WINAPI
SetCPGlobal (
    IN      UINT NewAcp
    )
{
    UINT oldVal;


    oldVal = gAnsiCodePage;

    //
    //  Sets the ACP global.  This is a private exported routine, not an API.
    //
    gAnsiCodePage = NewAcp;
    return oldVal;
}


////////////////////////////////////////////////////////////////////////////
//
//  GetOEMCP
//
//  Returns the OEM code page for the system.  If the registry value is
//  not readable, then the chosen default ACP is used (NLS_DEFAULT_OEMCP).
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

UINT WINAPI GetOEMCP()
{
    //
    //  Return the OEMCP stored in the cache.
    //
    return (gOemCodePage);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetCPInfo
//
//  Returns information about a given code page.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI GetCPInfo(
    UINT CodePage,
    LPCPINFO lpCPInfo)
{
    PCP_HASH pHashN;              // ptr to CP hash node
    PCP_TABLE pInfo;              // ptr to CP information in file
    WORD wDefChar;                // default character
    BYTE *pLeadBytes;             // ptr to lead byte ranges
    UINT Ctr;                     // loop counter


    //
    //  See if it's a special code page value for UTF translations.
    //
    if (CodePage >= NLS_CP_ALGORITHM_RANGE)
    {
        return (UTFCPInfo(CodePage, lpCPInfo, FALSE));
    }

    //
    //  Get the code page value and the appropriate hash node.
    //
    GET_CP_HASH_NODE(CodePage, pHashN);

    //
    //  Invalid Parameter Check:
    //     - validate code page - get hash node containing translation tables
    //     - lpCPInfo is NULL
    //
    if ( (pHashN == NULL) ||
         ((pHashN->pCPInfo == NULL) && (pHashN->pfnCPProc == NULL)) ||
         (lpCPInfo == NULL) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    //
    //  See if the given code page is in the DLL range.
    //
    if (pHashN->pfnCPProc)
    {
        //
        //  Call the DLL to get the code page information.
        //
        return ( (*(pHashN->pfnCPProc))( CodePage,
                                         NLS_CP_CPINFO,
                                         NULL,
                                         0,
                                         NULL,
                                         0,
                                         lpCPInfo ) );
    }

    //
    //  Fill in the CPINFO structure with the appropriate information.
    //
    pInfo = pHashN->pCPInfo;

    //
    //  Get the max char size.
    //
    lpCPInfo->MaxCharSize = (UINT)((WORD)pInfo->MaxCharSize);

    //
    //  Get the default character.
    //
    wDefChar = pInfo->wDefaultChar;
    if (HIBYTE(wDefChar))
    {
        (lpCPInfo->DefaultChar)[0] = HIBYTE(wDefChar);
        (lpCPInfo->DefaultChar)[1] = LOBYTE(wDefChar);
    }
    else
    {
        (lpCPInfo->DefaultChar)[0] = LOBYTE(wDefChar);
        (lpCPInfo->DefaultChar)[1] = (BYTE)0;
    }

    //
    //  Get the leadbytes.
    //
    pLeadBytes = pInfo->LeadByte;
    for (Ctr = 0; Ctr < MAX_LEADBYTES; Ctr++)
    {
        (lpCPInfo->LeadByte)[Ctr] = pLeadBytes[Ctr];
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetCPInfoExW
//
//  Returns information about a given code page.
//
//  11-15-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI GetCPInfoExW(
    UINT CodePage,
    DWORD dwFlags,
    LPCPINFOEXW lpCPInfoEx)
{
    PCP_HASH pHashN;              // ptr to CP hash node
    PCP_TABLE pInfo;              // ptr to CP information in file
    WORD wDefChar;                // default character
    BYTE *pLeadBytes;             // ptr to lead byte ranges
    UINT Ctr;                     // loop counter


    //
    //  See if it's a special code page value for UTF translations.
    //
    if (CodePage >= NLS_CP_ALGORITHM_RANGE)
    {
        if (UTFCPInfo(CodePage, (LPCPINFO)lpCPInfoEx, TRUE))
        {
            return (GetLocalizedCodePageName(CodePage, lpCPInfoEx->CodePageName));
        }
        return (FALSE);
    }

    //
    //  Get the code page value and the appropriate hash node.
    //
    GET_CP_HASH_NODE(CodePage, pHashN);

    //
    //  Invalid Parameter Check:
    //     - validate code page - get hash node containing translation tables
    //     - lpCPInfoEx is NULL
    //
    if ( (pHashN == NULL) ||
         ((pHashN->pCPInfo == NULL) && (pHashN->pfnCPProc == NULL)) ||
         (lpCPInfoEx == NULL) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    //
    //  Invalid Flags Check:
    //     - flags not 0
    //
    if (dwFlags != 0)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return (FALSE);
    }

    //
    //  See if the given code page is in the DLL range.
    //
    if (pHashN->pfnCPProc)
    {
        //
        //  Fill in the Ex version info in case the DLL doesn't.
        //
        lpCPInfoEx->UnicodeDefaultChar = L'?';
        lpCPInfoEx->CodePage = CodePage;
        GetLocalizedCodePageName(CodePage, lpCPInfoEx->CodePageName);

        //
        //  Call the DLL to get the code page information.
        //
        return ( (*(pHashN->pfnCPProc))( CodePage,
                                         NLS_CP_CPINFOEX,
                                         NULL,
                                         0,
                                         NULL,
                                         0,
                                         (LPCPINFO)lpCPInfoEx ) );
    }

    //
    //  Fill in the CPINFO structure with the appropriate information.
    //
    pInfo = pHashN->pCPInfo;

    //
    //  Get the max char size.
    //
    lpCPInfoEx->MaxCharSize = (UINT)((WORD)pInfo->MaxCharSize);

    //
    //  Get the default character.
    //
    wDefChar = pInfo->wDefaultChar;
    if (HIBYTE(wDefChar))
    {
        (lpCPInfoEx->DefaultChar)[0] = HIBYTE(wDefChar);
        (lpCPInfoEx->DefaultChar)[1] = LOBYTE(wDefChar);
    }
    else
    {
        (lpCPInfoEx->DefaultChar)[0] = LOBYTE(wDefChar);
        (lpCPInfoEx->DefaultChar)[1] = (BYTE)0;
    }

    //
    //  Get the leadbytes.
    //
    pLeadBytes = pInfo->LeadByte;
    for (Ctr = 0; Ctr < MAX_LEADBYTES; Ctr++)
    {
        (lpCPInfoEx->LeadByte)[Ctr] = pLeadBytes[Ctr];
    }

    //
    //  Get the Unicode default character.
    //
    lpCPInfoEx->UnicodeDefaultChar = pInfo->wUniDefaultChar;

    //
    //  Get the code page id.
    //
    lpCPInfoEx->CodePage = CodePage;

    //
    //  Get the code page name.
    //
    if (!GetLocalizedCodePageName(CodePage, lpCPInfoEx->CodePageName))
    {
        return (FALSE);
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  IsDBCSLeadByte
//
//  Checks to see if a given character is a DBCS lead byte in the ACP.
//  Returns TRUE if it is, FALSE if it is not.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI IsDBCSLeadByte(
    BYTE TestChar)
{
    //
    //  Get the hash node for the ACP.
    //
    if (gpACPHashN == NULL)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return (FALSE);
    }

    //
    //  See if the given character is a DBCS lead byte.
    //
    if (CHECK_DBCS_LEAD_BYTE(gpACPHashN->pDBCSOffsets, TestChar))
    {
        //
        //  Return success - IS a DBCS lead byte.
        //
        return (TRUE);
    }

    //
    //  Return failure - is NOT a DBCS lead byte.
    //
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  IsDBCSLeadByteEx
//
//  Checks to see if a given character is a DBCS lead byte in the given
//  code page.  Returns TRUE if it is, FALSE if it is not.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI IsDBCSLeadByteEx(
    UINT CodePage,
    BYTE TestChar)
{
    PCP_HASH pHashN;              // ptr to CP hash node


    //
    //  Get the code page value and the appropriate hash node.
    //
    GET_CP_HASH_NODE(CodePage, pHashN);

    //
    //  Invalid Parameter Check:
    //     - validate code page
    //
    if (pHashN == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    //
    //  See if the given character is a DBCS lead byte.
    //
    if (CHECK_DBCS_LEAD_BYTE(pHashN->pDBCSOffsets, TestChar))
    {
        //
        //  Return success - IS a DBCS lead byte.
        //
        return (TRUE);
    }

    //
    //  Return failure - is NOT a DBCS lead byte.
    //
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  MultiByteToWideChar
//
//  Maps a multibyte character string to its wide character string
//  counterpart.
//
//  05-31-91    JulieB    Created.
//  09-01-93    JulieB    Add support for MB_ERR_INVALID_CHARS flag.
////////////////////////////////////////////////////////////////////////////

int WINAPI MultiByteToWideChar(
    UINT CodePage,
    DWORD dwFlags,
    LPCSTR lpMultiByteStr,
    int cbMultiByte,
    LPWSTR lpWideCharStr,
    int cchWideChar)
{
    PCP_HASH pHashN;              // ptr to CP hash node
    register LPBYTE pMBStr;       // ptr to search through MB string
    register LPWSTR pWCStr;       // ptr to search through WC string
    LPBYTE pEndMBStr;             // ptr to end of MB search string
    LPWSTR pEndWCStr;             // ptr to end of WC string buffer
    int wcIncr;                   // amount to increment pWCStr
    int mbIncr;                   // amount to increment pMBStr
    int wcCount = 0;              // count of wide chars written
    int CompSet;                  // if MB_COMPOSITE flag is set
    PMB_TABLE pMBTbl;             // ptr to correct MB table (MB or GLYPH)
    int ctr;                      // loop counter


    //
    //  See if it's a special code page value for UTF translations.
    //
    if (CodePage >= NLS_CP_ALGORITHM_RANGE)
    {
        return (UTFToUnicode( CodePage,
                              dwFlags,
                              lpMultiByteStr,
                              cbMultiByte,
                              lpWideCharStr,
                              cchWideChar ));
    }

    //
    //  Get the code page value and the appropriate hash node.
    //
    GET_CP_HASH_NODE(CodePage, pHashN);

    //
    //  Invalid Parameter Check:
    //     - length of MB string is 0
    //     - wide char buffer size is negative
    //     - MB string is NULL
    //     - length of WC string is NOT zero AND
    //         (WC string is NULL OR src and dest pointers equal)
    //
    if ( (cbMultiByte == 0) || (cchWideChar < 0) ||
         (lpMultiByteStr == NULL) ||
         ((cchWideChar != 0) &&
          ((lpWideCharStr == NULL) ||
           (lpMultiByteStr == (LPSTR)lpWideCharStr))) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (0);
    }

    //
    //  If cbMultiByte is -1, then the string is null terminated and we
    //  need to get the length of the string.  Add one to the length to
    //  include the null termination.  (This will always be at least 1.)
    //
    if (cbMultiByte <= -1)
    {
        cbMultiByte = strlen(lpMultiByteStr) + 1;
    }

    //
    //  Check for valid code page.
    //
    if (pHashN == NULL)
    {
        //
        //  Special case the CP_SYMBOL code page.
        //
        if ((CodePage == CP_SYMBOL) && (dwFlags == 0))
        {
            //
            //  If the caller just wants the size of the buffer needed
            //  to do this translation, return the size of the MB string.
            //
            if (cchWideChar == 0)
            {
                return (cbMultiByte);
            }

            //
            //  Make sure the buffer is large enough.
            //
            if (cchWideChar < cbMultiByte)
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return (0);
            }

            //
            //  Translate SB char xx to Unicode f0xx.
            //    0x00->0x1f map to 0x0000->0x001f
            //    0x20->0xff map to 0xf020->0xf0ff
            //
            for (ctr = 0; ctr < cbMultiByte; ctr++)
            {
                lpWideCharStr[ctr] = ((BYTE)(lpMultiByteStr[ctr]) < 0x20)
                                       ? (WCHAR)lpMultiByteStr[ctr]
                                       : MAKEWORD(lpMultiByteStr[ctr], 0xf0);
            }
            return (cbMultiByte);
        }
        else
        {
            SetLastError(((CodePage == CP_SYMBOL) && (dwFlags != 0))
                           ? ERROR_INVALID_FLAGS
                           : ERROR_INVALID_PARAMETER);
            return (0);
        }
    }

    //
    //  See if the given code page is in the DLL range.
    //
    if (pHashN->pfnCPProc)
    {
        //
        //  Invalid Flags Check:
        //     - flags not 0
        //
        if (dwFlags != 0)
        {
            SetLastError(ERROR_INVALID_FLAGS);
            return (0);
        }

        //
        //  Call the DLL to do the translation.
        //
        return ( (*(pHashN->pfnCPProc))( CodePage,
                                         NLS_CP_MBTOWC,
                                         (LPSTR)lpMultiByteStr,
                                         cbMultiByte,
                                         (LPWSTR)lpWideCharStr,
                                         cchWideChar,
                                         NULL ) );
    }

    //
    //  Invalid Flags Check:
    //     - flags other than valid ones
    //     - composite and precomposed both set
    //
    if ( (dwFlags & MB_INVALID_FLAG) ||
         ((dwFlags & MB_PRECOMPOSED) && (dwFlags & MB_COMPOSITE)) )
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return (0);
    }

    //
    //  Initialize multibyte character loop pointers.
    //
    pMBStr = (LPBYTE)lpMultiByteStr;
    pEndMBStr = pMBStr + cbMultiByte;
    CompSet = dwFlags & MB_COMPOSITE;

    //
    //  Get the correct MB table (MB or GLYPH).
    //
    if ((dwFlags & MB_USEGLYPHCHARS) && (pHashN->pGlyphTbl != NULL))
    {
        pMBTbl = pHashN->pGlyphTbl;
    }
    else
    {
        pMBTbl = pHashN->pMBTbl;
    }

    //
    //  If cchWideChar is 0, then we can't use lpWideCharStr.  In this
    //  case, we simply want to count the number of characters that would
    //  be written to the buffer.
    //
    if (cchWideChar == 0)
    {
        WCHAR pTempStr[MAX_COMPOSITE];   // tmp buffer - max for composite

        //
        //  For each multibyte char, translate it to its corresponding
        //  wide char and increment the wide character count.
        //
        pEndWCStr = pTempStr + MAX_COMPOSITE;
        if (IS_SBCS_CP(pHashN))
        {
            //
            //  Single Byte Character Code Page.
            //
            if (CompSet)
            {
                //
                //  Composite flag is set.
                //
                if (dwFlags & MB_ERR_INVALID_CHARS)
                {
                    //
                    //  Error check flag is set.
                    //
                    while (pMBStr < pEndMBStr)
                    {
                        if (!(wcIncr = GetWCCompSBErr( pHashN,
                                                       pMBTbl,
                                                       pMBStr,
                                                       pTempStr,
                                                       pEndWCStr )))
                        {
                            return (0);
                        }
                        pMBStr++;
                        wcCount += wcIncr;
                    }
                }
                else
                {
                    //
                    //  Error check flag is NOT set.
                    //
                    while (pMBStr < pEndMBStr)
                    {
                        wcCount += GetWCCompSB( pMBTbl,
                                                pMBStr,
                                                pTempStr,
                                                pEndWCStr );
                        pMBStr++;
                    }
                }
            }
            else
            {
                //
                //  Composite flag is NOT set.
                //
                if (dwFlags & MB_ERR_INVALID_CHARS)
                {
                    //
                    //  Error check flag is set.
                    //
                    wcCount = (int)(pEndMBStr - pMBStr);
                    while (pMBStr < pEndMBStr)
                    {
                        GET_WC_SINGLE( pMBTbl,
                                       pMBStr,
                                       pTempStr );
                        CHECK_ERROR_WC_SINGLE( pHashN,
                                               *pTempStr,
                                               *pMBStr );
                        pMBStr++;
                    }
                }
                else
                {
                    //
                    //  Error check flag is NOT set.
                    //
                    //  Just return the size of the MB string, since
                    //  it's a 1:1 translation.
                    //
                    wcCount = (int)(pEndMBStr - pMBStr);
                }
            }
        }
        else
        {
            //
            //  Multi Byte Character Code Page.
            //
            if (CompSet)
            {
                //
                //  Composite flag is set.
                //
                if (dwFlags & MB_ERR_INVALID_CHARS)
                {
                    //
                    //  Error check flag is set.
                    //
                    while (pMBStr < pEndMBStr)
                    {
                        if (!(wcIncr = GetWCCompMBErr( pHashN,
                                                       pMBTbl,
                                                       pMBStr,
                                                       pEndMBStr,
                                                       pTempStr,
                                                       pEndWCStr,
                                                       &mbIncr )))
                        {
                            return (0);
                        }
                        pMBStr += mbIncr;
                        wcCount += wcIncr;
                    }
                }
                else
                {
                    //
                    //  Error check flag is NOT set.
                    //
                    while (pMBStr < pEndMBStr)
                    {
                        wcCount += GetWCCompMB( pHashN,
                                                pMBTbl,
                                                pMBStr,
                                                pEndMBStr,
                                                pTempStr,
                                                pEndWCStr,
                                                &mbIncr );
                        pMBStr += mbIncr;
                    }
                }
            }
            else
            {
                //
                //  Composite flag is NOT set.
                //
                if (dwFlags & MB_ERR_INVALID_CHARS)
                {
                    //
                    //  Error check flag is set.
                    //
                    while (pMBStr < pEndMBStr)
                    {
                        GET_WC_MULTI_ERR( pHashN,
                                          pMBTbl,
                                          pMBStr,
                                          pEndMBStr,
                                          pTempStr,
                                          pEndWCStr,
                                          mbIncr );
                        pMBStr += mbIncr;
                        wcCount++;
                    }
                }
                else
                {
                    //
                    //  Error check flag is NOT set.
                    //
                    while (pMBStr < pEndMBStr)
                    {
                        GET_WC_MULTI( pHashN,
                                      pMBTbl,
                                      pMBStr,
                                      pEndMBStr,
                                      pTempStr,
                                      pEndWCStr,
                                      mbIncr );
                        pMBStr += mbIncr;
                        wcCount++;
                    }
                }
            }
        }
    }
    else
    {
        //
        //  Initialize wide character loop pointers.
        //
        pWCStr = lpWideCharStr;
        pEndWCStr = pWCStr + cchWideChar;

        //
        //  For each multibyte char, translate it to its corresponding
        //  wide char, store it in lpWideCharStr, and increment the wide
        //  character count.
        //
        if (IS_SBCS_CP(pHashN))
        {
            //
            //  Single Byte Character Code Page.
            //
            if (CompSet)
            {
                //
                //  Composite flag is set.
                //
                if (dwFlags & MB_ERR_INVALID_CHARS)
                {
                    //
                    //  Error check flag is set.
                    //
                    while ((pMBStr < pEndMBStr) && (pWCStr < pEndWCStr))
                    {
                        if (!(wcIncr = GetWCCompSBErr( pHashN,
                                                       pMBTbl,
                                                       pMBStr,
                                                       pWCStr,
                                                       pEndWCStr )))
                        {
                            return (0);
                        }
                        pMBStr++;
                        pWCStr += wcIncr;
                    }
                    wcCount = (int)(pWCStr - lpWideCharStr);
                }
                else
                {
                    //
                    //  Error check flag is NOT set.
                    //
                    while ((pMBStr < pEndMBStr) && (pWCStr < pEndWCStr))
                    {
                        pWCStr += GetWCCompSB( pMBTbl,
                                               pMBStr,
                                               pWCStr,
                                               pEndWCStr );
                        pMBStr++;
                    }
                    wcCount = (int)(pWCStr - lpWideCharStr);
                }
            }
            else
            {
                //
                //  Composite flag is NOT set.
                //
                if (dwFlags & MB_ERR_INVALID_CHARS)
                {
                    //
                    //  Error check flag is set.
                    //
                    wcCount = (int)(pEndMBStr - pMBStr);
                    if ((pEndWCStr - pWCStr) < wcCount)
                    {
                        wcCount = (int)(pEndWCStr - pWCStr);
                    }
                    for (ctr = wcCount; ctr > 0; ctr--)
                    {
                        GET_WC_SINGLE( pMBTbl,
                                       pMBStr,
                                       pWCStr );
                        CHECK_ERROR_WC_SINGLE( pHashN,
                                               *pWCStr,
                                               *pMBStr );
                        pMBStr++;
                        pWCStr++;
                    }
                }
                else
                {
                    //
                    //  Error check flag is NOT set.
                    //
                    wcCount = (int)(pEndMBStr - pMBStr);
                    if ((pEndWCStr - pWCStr) < wcCount)
                    {
                        wcCount = (int)(pEndWCStr - pWCStr);
                    }
                    for (ctr = wcCount; ctr > 0; ctr--)
                    {
                        GET_WC_SINGLE( pMBTbl,
                                       pMBStr,
                                       pWCStr );
                        pMBStr++;
                        pWCStr++;
                    }
                }
            }
        }
        else
        {
            //
            //  Multi Byte Character Code Page.
            //
            if (CompSet)
            {
                //
                //  Composite flag is set.
                //
                if (dwFlags & MB_ERR_INVALID_CHARS)
                {
                    //
                    //  Error check flag is set.
                    //
                    while ((pMBStr < pEndMBStr) && (pWCStr < pEndWCStr))
                    {
                        if (!(wcIncr = GetWCCompMBErr( pHashN,
                                                       pMBTbl,
                                                       pMBStr,
                                                       pEndMBStr,
                                                       pWCStr,
                                                       pEndWCStr,
                                                       &mbIncr )))
                        {
                            return (0);
                        }
                        pMBStr += mbIncr;
                        pWCStr += wcIncr;
                    }
                    wcCount = (int)(pWCStr - lpWideCharStr);
                }
                else
                {
                    //
                    //  Error check flag is NOT set.
                    //
                    while ((pMBStr < pEndMBStr) && (pWCStr < pEndWCStr))
                    {
                        pWCStr += GetWCCompMB( pHashN,
                                               pMBTbl,
                                               pMBStr,
                                               pEndMBStr,
                                               pWCStr,
                                               pEndWCStr,
                                               &mbIncr );
                        pMBStr += mbIncr;
                    }
                    wcCount = (int)(pWCStr - lpWideCharStr);
                }
            }
            else
            {
                //
                //  Composite flag is NOT set.
                //
                if (dwFlags & MB_ERR_INVALID_CHARS)
                {
                    //
                    //  Error check flag is set.
                    //
                    while ((pMBStr < pEndMBStr) && (pWCStr < pEndWCStr))
                    {
                        GET_WC_MULTI_ERR( pHashN,
                                          pMBTbl,
                                          pMBStr,
                                          pEndMBStr,
                                          pWCStr,
                                          pEndWCStr,
                                          mbIncr );
                        pMBStr += mbIncr;
                        pWCStr++;
                    }
                    wcCount = (int)(pWCStr - lpWideCharStr);
                }
                else
                {
                    //
                    //  Error check flag is NOT set.
                    //
                    while ((pMBStr < pEndMBStr) && (pWCStr < pEndWCStr))
                    {
                        GET_WC_MULTI( pHashN,
                                      pMBTbl,
                                      pMBStr,
                                      pEndMBStr,
                                      pWCStr,
                                      pEndWCStr,
                                      mbIncr );
                        pMBStr += mbIncr;
                        pWCStr++;
                    }
                    wcCount = (int)(pWCStr - lpWideCharStr);
                }
            }
        }

        //
        //  Make sure wide character buffer was large enough.
        //
        if (pMBStr < pEndMBStr)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return (0);
        }
    }

    //
    //  Return the number of characters written (or that would have
    //  been written) to the buffer.
    //
    return (wcCount);
}


////////////////////////////////////////////////////////////////////////////
//
//  WideCharToMultiByte
//
//  Maps a wide character string to its multibyte character string
//  counterpart.
//
//  NOTE:  Most significant bit of dwFlags parameter is used by this routine
//         to indicate that the caller only wants the count of the number of
//         characters written, not the string (ie. do not back up in buffer).
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WINAPI WideCharToMultiByte(
    UINT CodePage,
    DWORD dwFlags,
    LPCWSTR lpWideCharStr,
    int cchWideChar,
    LPSTR lpMultiByteStr,
    int cbMultiByte,
    LPCSTR lpDefaultChar,
    LPBOOL lpUsedDefaultChar)
{
    PCP_HASH pHashN;              // ptr to CP hash node
    LPWSTR pWCStr;                // ptr to search through WC string
    LPWSTR pEndWCStr;             // ptr to end of WC string buffer
    WORD wDefault = 0;            // default character as a word
    int IfNoDefault;              // if default check is to be made
    int IfCompositeChk;           // if check for composite
    BOOL TmpUsed;                 // temp storage for default used
    int ctr;                      // loop counter


    //
    //  See if it's a special code page value for UTF translations.
    //
    if (CodePage >= NLS_CP_ALGORITHM_RANGE)
    {
        return (UnicodeToUTF( CodePage,
                              dwFlags,
                              lpWideCharStr,
                              cchWideChar,
                              lpMultiByteStr,
                              cbMultiByte,
                              lpDefaultChar,
                              lpUsedDefaultChar ));
    }

    //
    //  Get the code page value and the appropriate hash node.
    //
    GET_CP_HASH_NODE(CodePage, pHashN);

    //
    //  Invalid Parameter Check:
    //     - length of WC string is 0
    //     - multibyte buffer size is negative
    //     - WC string is NULL
    //     - length of WC string is NOT zero AND
    //         (MB string is NULL OR src and dest pointers equal)
    //
    if ( (cchWideChar == 0) || (cbMultiByte < 0) ||
         (lpWideCharStr == NULL) ||
         ((cbMultiByte != 0) &&
          ((lpMultiByteStr == NULL) ||
           (lpWideCharStr == (LPWSTR)lpMultiByteStr))) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (0);
    }

    //
    //  If cchWideChar is -1, then the string is null terminated and we
    //  need to get the length of the string.  Add one to the length to
    //  include the null termination.  (This will always be at least 1.)
    //
    if (cchWideChar <= -1)
    {
        cchWideChar = NlsStrLenW(lpWideCharStr) + 1;
    }

    //
    //  Check for valid code page.
    //
    if (pHashN == NULL)
    {
        //
        //  Special case the CP_SYMBOL code page.
        //
        if ((CodePage == CP_SYMBOL) && (dwFlags == 0) &&
            (lpDefaultChar == NULL) && (lpUsedDefaultChar == NULL))
        {
            //
            //  If the caller just wants the size of the buffer needed
            //  to do this translation, return the size of the MB string.
            //
            if (cbMultiByte == 0)
            {
                return (cchWideChar);
            }

            //
            //  Make sure the buffer is large enough.
            //
            if (cbMultiByte < cchWideChar)
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return (0);
            }

            //
            //  Translate Unicode char f0xx to SB xx.
            //    0x0000->0x001f map to 0x00->0x1f
            //    0xf020->0xf0ff map to 0x20->0xff
            //
            for (ctr = 0; ctr < cchWideChar; ctr++)
            {
                if ((lpWideCharStr[ctr] >= 0x0020) &&
                    ((lpWideCharStr[ctr] < 0xf020) ||
                     (lpWideCharStr[ctr] > 0xf0ff)))
                {
                    SetLastError(ERROR_NO_UNICODE_TRANSLATION);                        \
                    return (0);
                }
                lpMultiByteStr[ctr] = (BYTE)lpWideCharStr[ctr];
            }
            return (cchWideChar);
        }
        else
        {
            SetLastError(((CodePage == CP_SYMBOL) && (dwFlags != 0))
                           ? ERROR_INVALID_FLAGS
                           : ERROR_INVALID_PARAMETER);
            return (0);
        }
    }

    //
    //  See if the given code page is in the DLL range.
    //
    if (pHashN->pfnCPProc)
    {
        //
        //  Invalid Parameter Check:
        //     - lpDefaultChar not NULL
        //     - lpUsedDefaultChar not NULL
        //
        if ((lpDefaultChar != NULL) || (lpUsedDefaultChar != NULL))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return (0);
        }

        //
        //  Invalid Flags Check:
        //     - flags not 0
        //
        if (dwFlags != 0)
        {
            SetLastError(ERROR_INVALID_FLAGS);
            return (0);
        }

        //
        //  Call the DLL to do the translation.
        //
        return ( (*(pHashN->pfnCPProc))( CodePage,
                                         NLS_CP_WCTOMB,
                                         (LPSTR)lpMultiByteStr,
                                         cbMultiByte,
                                         (LPWSTR)lpWideCharStr,
                                         cchWideChar,
                                         NULL ) );
    }

    //
    //  Invalid Flags Check:
    //     - compositechk flag is not set AND any of comp flags are set
    //     - flags other than valid ones
    //
    if ( ((!(IfCompositeChk = (dwFlags & WC_COMPOSITECHECK))) &&
          (dwFlags & WC_COMPCHK_FLAGS)) ||
         (dwFlags & WC_INVALID_FLAG) )
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return (0);
    }

    //
    //  Initialize wide character loop pointers.
    //
    pWCStr = (LPWSTR)lpWideCharStr;
    pEndWCStr = pWCStr + cchWideChar;

    //
    //  Set the IfNoDefault parameter to TRUE if both lpDefaultChar and
    //  lpUsedDefaultChar are NULL.
    //
    IfNoDefault = ((lpDefaultChar == NULL) && (lpUsedDefaultChar == NULL));

    //
    //  If the composite check flag is NOT set AND both of the default
    //  parameters (lpDefaultChar and lpUsedDefaultChar) are null, then
    //  do the quick translation.
    //
    if (IfNoDefault && !IfCompositeChk)
    {
        //
        //  Translate WC string to MB string, ignoring default chars.
        //
        return (GetMBNoDefault( pHashN,
                                pWCStr,
                                pEndWCStr,
                                (LPBYTE)lpMultiByteStr,
                                cbMultiByte,
                                dwFlags ));
    }

    //
    //  Set the system default character.
    //
    wDefault = pHashN->pCPInfo->wDefaultChar;

    //
    //  See if the default check is needed.
    //
    if (!IfNoDefault)
    {
        //
        //  If lpDefaultChar is NULL, then use the system default.
        //  Form a word out of the default character.  Single byte
        //  characters are zero extended, DBCS characters are as is.
        //
        if (lpDefaultChar != NULL)
        {
            wDefault = GET_DEFAULT_WORD( pHashN->pDBCSOffsets,
                                         (LPBYTE)lpDefaultChar );
        }

        //
        //  If lpUsedDefaultChar is NULL, then it won't be used later
        //  on if a default character is detected.  Otherwise, we need
        //  to initialize it.
        //
        if (lpUsedDefaultChar == NULL)
        {
            lpUsedDefaultChar = &TmpUsed;
        }
        *lpUsedDefaultChar = FALSE;

        //
        //  Check for "composite check" flag.
        //
        if (!IfCompositeChk)
        {
            //
            //  Translate WC string to MB string, checking for the use of the
            //  default character.
            //
            return (GetMBDefault( pHashN,
                                  pWCStr,
                                  pEndWCStr,
                                  (LPBYTE)lpMultiByteStr,
                                  cbMultiByte,
                                  wDefault,
                                  lpUsedDefaultChar,
                                  dwFlags ));
        }
        else
        {
            //
            //  Translate WC string to MB string, checking for the use of the
            //  default character.
            //
            return (GetMBDefaultComp( pHashN,
                                      pWCStr,
                                      pEndWCStr,
                                      (LPBYTE)lpMultiByteStr,
                                      cbMultiByte,
                                      wDefault,
                                      lpUsedDefaultChar,
                                      dwFlags ));
        }
    }
    else
    {
        //
        //  The only case left here is that the Composite check
        //  flag IS set and the default check flag is NOT set.
        //
        //  Translate WC string to MB string, checking for the use of the
        //  default character.
        //
        return (GetMBDefaultComp( pHashN,
                                  pWCStr,
                                  pEndWCStr,
                                  (LPBYTE)lpMultiByteStr,
                                  cbMultiByte,
                                  wDefault,
                                  &TmpUsed,
                                  dwFlags ));
    }
}




//-------------------------------------------------------------------------//
//                          INTERNAL ROUTINES                              //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  GetLocalizedCodePageName
//
//  Returns the localized version of the code page name for the given
//  code page id.  It gets the information from the resource file in the
//  language that the current user is using.
//
//  11-15-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL GetLocalizedCodePageName(
    UINT CodePage,
    LPWSTR pCPName)
{
    HANDLE hFindRes;                   // handle from find resource
    HANDLE hLoadRes;                   // handle from load resource
    LANGID LangId;                     // language id
    LPWSTR pName;                      // ptr to code page name


    //
    //  Find the resource.
    //
    LangId = LANGIDFROMLCID(GetUserDefaultLCID());
    if ((!(hFindRes = FindResourceExW( hModule,
                                       L"CODEPAGE",
                                       MAKEINTRESOURCEW(CodePage),
                                       (WORD)LangId ))))
    {
        //
        //  Could not find resource.  Try NEUTRAL language id.
        //
        if ((!(hFindRes = FindResourceExW( hModule,
                                           L"CODEPAGE",
                                           MAKEINTRESOURCEW(CodePage),
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
        //  code page name buffer.
        //
        if (pName = (LPWSTR)LockResource(hLoadRes))
        {
            NlsStrCpyW(pCPName, pName);
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
//  GetWCCompSB
//
//  Fills in pWCStr with the wide character(s) for the corresponding single
//  byte character from the appropriate translation table and returns the
//  number of wide characters written.  This routine should only be called
//  when the precomposed forms need to be translated to composite.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetWCCompSB(
    PMB_TABLE pMBTbl,
    LPBYTE pMBStr,
    LPWSTR pWCStr,
    LPWSTR pEndWCStr)
{
    //
    //  Get the single byte to wide character translation.
    //
    GET_WC_SINGLE(pMBTbl, pMBStr, pWCStr);

    //
    //  Fill in the composite form of the character (if one exists)
    //  and return the number of wide characters written.
    //
    return (InsertCompositeForm(pWCStr, pEndWCStr));
}


////////////////////////////////////////////////////////////////////////////
//
//  GetWCCompMB
//
//  Fills in pWCStr with the wide character(s) for the corresponding multibyte
//  character from the appropriate translation table and returns the number
//  of wide characters written.  The number of bytes used from the pMBStr
//  buffer (single byte or double byte) is returned in the mbIncr parameter.
//  This routine should only be called when the precomposed forms need to be
//  translated to composite.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetWCCompMB(
    PCP_HASH pHashN,
    PMB_TABLE pMBTbl,
    LPBYTE pMBStr,
    LPBYTE pEndMBStr,
    LPWSTR pWCStr,
    LPWSTR pEndWCStr,
    int *pmbIncr)
{
    //
    //  Get the multibyte to wide char translation.
    //
    GET_WC_MULTI( pHashN,
                  pMBTbl,
                  pMBStr,
                  pEndMBStr,
                  pWCStr,
                  pEndWCStr,
                  *pmbIncr );

    //
    //  Fill in the composite form of the character (if one exists)
    //  and return the number of wide characters written.
    //
    return (InsertCompositeForm(pWCStr, pEndWCStr));
}


////////////////////////////////////////////////////////////////////////////
//
//  GetWCCompSBErr
//
//  Fills in pWCStr with the wide character(s) for the corresponding single
//  byte character from the appropriate translation table and returns the
//  number of wide characters written.  This routine should only be called
//  when the precomposed forms need to be translated to composite.
//
//  Checks to be sure an invalid character is not translated to the default
//  character.  If so, it sets last error and returns 0 characters written.
//
//  09-01-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetWCCompSBErr(
    PCP_HASH pHashN,
    PMB_TABLE pMBTbl,
    LPBYTE pMBStr,
    LPWSTR pWCStr,
    LPWSTR pEndWCStr)
{
    //
    //  Get the single byte to wide character translation.
    //
    GET_WC_SINGLE(pMBTbl, pMBStr, pWCStr);

    //
    //  Make sure an invalid character was not translated to the
    //  default char.  If it was, set last error and return 0
    //  characters written.
    //
    CHECK_ERROR_WC_SINGLE(pHashN, *pWCStr, *pMBStr);

    //
    //  Fill in the composite form of the character (if one exists)
    //  and return the number of wide characters written.
    //
    return (InsertCompositeForm(pWCStr, pEndWCStr));
}


////////////////////////////////////////////////////////////////////////////
//
//  GetWCCompMBErr
//
//  Fills in pWCStr with the wide character(s) for the corresponding multibyte
//  character from the appropriate translation table and returns the number
//  of wide characters written.  The number of bytes used from the pMBStr
//  buffer (single byte or double byte) is returned in the mbIncr parameter.
//  This routine should only be called when the precomposed forms need to be
//  translated to composite.
//
//  Checks to be sure an invalid character is not translated to the default
//  character.  If so, it sets last error and returns 0 characters written.
//
//  09-01-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetWCCompMBErr(
    PCP_HASH pHashN,
    PMB_TABLE pMBTbl,
    LPBYTE pMBStr,
    LPBYTE pEndMBStr,
    LPWSTR pWCStr,
    LPWSTR pEndWCStr,
    int *pmbIncr)
{
    //
    //  Get the multibyte to wide char translation.
    //
    //  Make sure an invalid character was not translated to the
    //  default char.  If it was, set last error and return 0
    //  characters written.
    //
    GET_WC_MULTI_ERR( pHashN,
                      pMBTbl,
                      pMBStr,
                      pEndMBStr,
                      pWCStr,
                      pEndWCStr,
                      *pmbIncr );

    //
    //  Fill in the composite form of the character (if one exists)
    //  and return the number of wide characters written.
    //
    return (InsertCompositeForm(pWCStr, pEndWCStr));
}


////////////////////////////////////////////////////////////////////////////
//
//  GetMBNoDefault
//
//  Translates the wide character string to a multibyte string and returns
//  the number of bytes written.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetMBNoDefault(
    PCP_HASH pHashN,
    LPWSTR pWCStr,
    LPWSTR pEndWCStr,
    LPBYTE pMBStr,
    int cbMultiByte,
    DWORD dwFlags)
{
    int mbIncr;                   // amount to increment pMBStr
    int mbCount = 0;              // count of multibyte chars written
    LPBYTE pEndMBStr;             // ptr to end of MB string buffer
    PWC_TABLE pWC = pHashN->pWC;  // ptr to WC table
    int ctr;                      // loop counter


    //
    //  If cbMultiByte is 0, then we can't use pMBStr.  In this
    //  case, we simply want to count the number of characters that
    //  would be written to the buffer.
    //
    if (cbMultiByte == 0)
    {
        BYTE pTempStr[2];             // tmp buffer - 2 bytes for DBCS

        //
        //  For each wide char, translate it to its corresponding multibyte
        //  char and increment the multibyte character count.
        //
        if (IS_SBCS_CP(pHashN))
        {
            //
            //  Single Byte Character Code Page.
            //
            //  Just return the count of characters - it will be the
            //  same number of characters as the source string.
            //
            mbCount = (int)(pEndWCStr - pWCStr);
        }
        else
        {
            //
            //  Multi Byte Character Code Page.
            //
            if (dwFlags & WC_NO_BEST_FIT_CHARS)
            {
                while (pWCStr < pEndWCStr)
                {
                    GET_MB( pWC,
                            *pWCStr,
                            pTempStr,
                            mbIncr,
                            FALSE );
                    ELIMINATE_BEST_FIT_MB( pHashN,
                                           *pWCStr,
                                           pTempStr,
                                           mbIncr,
                                           FALSE );
                    pWCStr++;
                    mbCount += mbIncr;
                }
            }
            else
            {
                while (pWCStr < pEndWCStr)
                {
                    GET_MB( pWC,
                            *pWCStr,
                            pTempStr,
                            mbIncr,
                            FALSE );
                    pWCStr++;
                    mbCount += mbIncr;
                }
            }
        }
    }
    else
    {
        //
        //  Initialize multibyte loop pointers.
        //
        pEndMBStr = pMBStr + cbMultiByte;

        //
        //  For each wide char, translate it to its corresponding
        //  multibyte char, store it in pMBStr, and increment the
        //  multibyte character count.
        //
        if (IS_SBCS_CP(pHashN))
        {
            //
            //  Single Byte Character Code Page.
            //
            mbCount = (int)(pEndWCStr - pWCStr);
            if ((pEndMBStr - pMBStr) < mbCount)
            {
                mbCount = (int)(pEndMBStr - pMBStr);
            }
            if (dwFlags & WC_NO_BEST_FIT_CHARS)
            {
                for (ctr = mbCount; ctr > 0; ctr--)
                {
                    GET_SB( pWC,
                            *pWCStr,
                            pMBStr );
                    ELIMINATE_BEST_FIT_SB( pHashN,
                                           *pWCStr,
                                           pMBStr );
                    pWCStr++;
                    pMBStr++;
                }
            }
            else
            {
                for (ctr = mbCount; ctr > 0; ctr--)
                {
                    GET_SB( pWC,
                            *pWCStr,
                            pMBStr );
                    pWCStr++;
                    pMBStr++;
                }
            }
        }
        else
        {
            //
            //  Multi Byte Character Code Page.
            //
            if (dwFlags & WC_NO_BEST_FIT_CHARS)
            {
                while ((pWCStr < pEndWCStr) && (pMBStr < pEndMBStr))
                {
                    GET_MB( pWC,
                            *pWCStr,
                            pMBStr,
                            mbIncr,
                            ((pMBStr + 1) < pEndMBStr) ? FALSE : TRUE );
                    ELIMINATE_BEST_FIT_MB( pHashN,
                                           *pWCStr,
                                           pMBStr,
                                           mbIncr,
                                           ((pMBStr + 1) < pEndMBStr) ? FALSE : TRUE );
                    if (mbIncr == 0)
                    {
                        //
                        //  Not enough space in buffer.
                        //
                        break;
                    }

                    pWCStr++;
                    mbCount += mbIncr;
                    pMBStr += mbIncr;
                }
            }
            else
            {
                while ((pWCStr < pEndWCStr) && (pMBStr < pEndMBStr))
                {
                    GET_MB( pWC,
                            *pWCStr,
                            pMBStr,
                            mbIncr,
                            ((pMBStr + 1) < pEndMBStr) ? FALSE : TRUE );
                    if (mbIncr == 0)
                    {
                        //
                        //  Not enough space in buffer.
                        //
                        break;
                    }

                    pWCStr++;
                    mbCount += mbIncr;
                    pMBStr += mbIncr;
                }
            }
        }

        //
        //  Make sure multibyte character buffer was large enough.
        //
        if (pWCStr < pEndWCStr)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return (0);
        }
    }

    //
    //  Return the number of characters written (or that would have
    //  been written) to the buffer.
    //
    return (mbCount);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetMBDefault
//
//  Translates the wide character string to a multibyte string and returns
//  the number of bytes written.  This also checks for the use of the default
//  character, so the translation is slower.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetMBDefault(
    PCP_HASH pHashN,
    LPWSTR pWCStr,
    LPWSTR pEndWCStr,
    LPBYTE pMBStr,
    int cbMultiByte,
    WORD wDefault,
    LPBOOL pUsedDef,
    DWORD dwFlags)
{
    int mbIncr;                   // amount to increment pMBStr
    int mbIncr2;                  // amount to increment pMBStr
    int mbCount = 0;              // count of multibyte chars written
    LPBYTE pEndMBStr;             // ptr to end of MB string buffer
    PWC_TABLE pWC = pHashN->pWC;  // ptr to WC table
    int ctr;                      // loop counter


    //
    //  If cbMultiByte is 0, then we can't use pMBStr.  In this
    //  case, we simply want to count the number of characters that
    //  would be written to the buffer.
    //
    if (cbMultiByte == 0)
    {
        BYTE pTempStr[2];             // tmp buffer - 2 bytes for DBCS

        //
        //  For each wide char, translate it to its corresponding multibyte
        //  char and increment the multibyte character count.
        //
        if (IS_SBCS_CP(pHashN))
        {
            //
            //  Single Byte Character Code Page.
            //
            mbCount = (int)(pEndWCStr - pWCStr);
            if (dwFlags & WC_NO_BEST_FIT_CHARS)
            {
                while (pWCStr < pEndWCStr)
                {
                    GET_SB( pWC,
                            *pWCStr,
                            pTempStr );
                    ELIMINATE_BEST_FIT_SB( pHashN,
                                           *pWCStr,
                                           pTempStr );
                    DEFAULT_CHAR_CHECK_SB( pHashN,
                                           *pWCStr,
                                           pTempStr,
                                           wDefault,
                                           pUsedDef );
                    pWCStr++;
                }
            }
            else
            {
                while (pWCStr < pEndWCStr)
                {
                    GET_SB( pWC,
                            *pWCStr,
                            pTempStr );
                    DEFAULT_CHAR_CHECK_SB( pHashN,
                                           *pWCStr,
                                           pTempStr,
                                           wDefault,
                                           pUsedDef );
                    pWCStr++;
                }
            }
        }
        else
        {
            //
            //  Multi Byte Character Code Page.
            //
            if (dwFlags & WC_NO_BEST_FIT_CHARS)
            {
                while (pWCStr < pEndWCStr)
                {
                    GET_MB( pWC,
                            *pWCStr,
                            pTempStr,
                            mbIncr,
                            FALSE );
                    ELIMINATE_BEST_FIT_MB( pHashN,
                                           *pWCStr,
                                           pTempStr,
                                           mbIncr,
                                           FALSE );
                    DEFAULT_CHAR_CHECK_MB( pHashN,
                                           *pWCStr,
                                           pTempStr,
                                           wDefault,
                                           pUsedDef,
                                           mbIncr2,
                                           FALSE );
                    mbCount += (mbIncr2) ? (mbIncr2) : (mbIncr);
                    pWCStr++;
                }
            }
            else
            {
                while (pWCStr < pEndWCStr)
                {
                    GET_MB( pWC,
                            *pWCStr,
                            pTempStr,
                            mbIncr,
                            FALSE );
                    DEFAULT_CHAR_CHECK_MB( pHashN,
                                           *pWCStr,
                                           pTempStr,
                                           wDefault,
                                           pUsedDef,
                                           mbIncr2,
                                           FALSE );
                    mbCount += (mbIncr2) ? (mbIncr2) : (mbIncr);
                    pWCStr++;
                }
            }
        }
    }
    else
    {
        //
        //  Initialize multibyte loop pointers.
        //
        pEndMBStr = pMBStr + cbMultiByte;

        //
        //  For each wide char, translate it to its corresponding
        //  multibyte char, store it in pMBStr, and increment the
        //  multibyte character count.
        //
        if (IS_SBCS_CP(pHashN))
        {
            //
            //  Single Byte Character Code Page.
            //
            mbCount = (int)(pEndWCStr - pWCStr);
            if ((pEndMBStr - pMBStr) < mbCount)
            {
                mbCount = (int)(pEndMBStr - pMBStr);
            }
            if (dwFlags & WC_NO_BEST_FIT_CHARS)
            {
                for (ctr = mbCount; ctr > 0; ctr--)
                {
                    GET_SB( pWC,
                            *pWCStr,
                            pMBStr );
                    ELIMINATE_BEST_FIT_SB( pHashN,
                                           *pWCStr,
                                           pMBStr );
                    DEFAULT_CHAR_CHECK_SB( pHashN,
                                           *pWCStr,
                                           pMBStr,
                                           wDefault,
                                           pUsedDef );
                    pWCStr++;
                    pMBStr++;
                }
            }
            else
            {
                for (ctr = mbCount; ctr > 0; ctr--)
                {
                    GET_SB( pWC,
                            *pWCStr,
                            pMBStr );
                    DEFAULT_CHAR_CHECK_SB( pHashN,
                                           *pWCStr,
                                           pMBStr,
                                           wDefault,
                                           pUsedDef );
                    pWCStr++;
                    pMBStr++;
                }
            }
        }
        else
        {
            //
            //  Multi Byte Character Code Page.
            //
            if (dwFlags & WC_NO_BEST_FIT_CHARS)
            {
                while ((pWCStr < pEndWCStr) && (pMBStr < pEndMBStr))
                {
                    GET_MB( pWC,
                            *pWCStr,
                            pMBStr,
                            mbIncr,
                            ((pMBStr + 1) < pEndMBStr) ? FALSE : TRUE );
                    ELIMINATE_BEST_FIT_MB( pHashN,
                                           *pWCStr,
                                           pMBStr,
                                           mbIncr,
                                           ((pMBStr + 1) < pEndMBStr) ? FALSE : TRUE );
                    DEFAULT_CHAR_CHECK_MB( pHashN,
                                           *pWCStr,
                                           pMBStr,
                                           wDefault,
                                           pUsedDef,
                                           mbIncr2,
                                           ((pMBStr + 1) < pEndMBStr) ? FALSE : TRUE );
                    if ((mbIncr == 0) || (mbIncr2 == -1))
                    {
                        //
                        //  Not enough room in buffer.
                        //
                        break;
                    }

                    mbCount += (mbIncr2) ? (mbIncr2) : (mbIncr);
                    pWCStr++;
                    pMBStr += mbIncr;
                }
            }
            else
            {
                while ((pWCStr < pEndWCStr) && (pMBStr < pEndMBStr))
                {
                    GET_MB( pWC,
                            *pWCStr,
                            pMBStr,
                            mbIncr,
                            ((pMBStr + 1) < pEndMBStr) ? FALSE : TRUE );
                    DEFAULT_CHAR_CHECK_MB( pHashN,
                                           *pWCStr,
                                           pMBStr,
                                           wDefault,
                                           pUsedDef,
                                           mbIncr2,
                                           ((pMBStr + 1) < pEndMBStr) ? FALSE : TRUE );
                    if ((mbIncr == 0) || (mbIncr2 == -1))
                    {
                        //
                        //  Not enough room in buffer.
                        //
                        break;
                    }

                    mbCount += (mbIncr2) ? (mbIncr2) : (mbIncr);
                    pWCStr++;
                    pMBStr += mbIncr;
                }
            }
        }

        //
        //  Make sure multibyte character buffer was large enough.
        //
        if (pWCStr < pEndWCStr)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return (0);
        }
    }

    //
    //  Return the number of characters written (or that would have
    //  been written) to the buffer.
    //
    return (mbCount);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetMBDefaultComp
//
//  Translates the wide character string to a multibyte string and returns
//  the number of bytes written.  This also checks for the use of the default
//  character and tries to convert composite forms to precomposed forms, so
//  the translation is a lot slower.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetMBDefaultComp(
    PCP_HASH pHashN,
    LPWSTR pWCStr,
    LPWSTR pEndWCStr,
    LPBYTE pMBStr,
    int cbMultiByte,
    WORD wDefault,
    LPBOOL pUsedDef,
    DWORD dwFlags)
{
    int mbIncr;                   // amount to increment pMBStr
    int mbCount = 0;              // count of multibyte chars written
    LPBYTE pEndMBStr;             // ptr to end of MB string buffer
    BOOL fError;                  // if error during MB conversion


    //
    //  If cbMultiByte is 0, then we can't use pMBStr.  In this
    //  case, we simply want to count the number of characters that
    //  would be written to the buffer.
    //
    if (cbMultiByte == 0)
    {
        BYTE pTempStr[2];             // tmp buffer - 2 bytes for DBCS

        //
        //  Set most significant bit of flags to indicate to the
        //  GetMBComp routine that it's using a temporary storage
        //  area, so don't back up in the buffer.
        //
        SET_MSB(dwFlags);

        //
        //  For each wide char, translate it to its corresponding multibyte
        //  char and increment the multibyte character count.
        //
        if (IS_SBCS_CP(pHashN))
        {
            //
            //  Single Byte Character Code Page.
            //
            while (pWCStr < pEndWCStr)
            {
                //
                //  Get the translation.
                //
                mbCount += GetMBCompSB( pHashN,
                                        dwFlags,
                                        pWCStr,
                                        pTempStr,
                                        mbCount,
                                        wDefault,
                                        pUsedDef );
                pWCStr++;
            }
        }
        else
        {
            //
            //  Multi Byte Character Code Page.
            //
            while (pWCStr < pEndWCStr)
            {
                //
                //  Get the translation.
                //
                mbCount += GetMBCompMB( pHashN,
                                        dwFlags,
                                        pWCStr,
                                        pTempStr,
                                        mbCount,
                                        wDefault,
                                        pUsedDef,
                                        &fError,
                                        FALSE );
                pWCStr++;
            }
        }
    }
    else
    {
        //
        //  Initialize multibyte loop pointers.
        //
        pEndMBStr = pMBStr + cbMultiByte;

        //
        //  For each wide char, translate it to its corresponding
        //  multibyte char, store it in pMBStr, and increment the
        //  multibyte character count.
        //
        if (IS_SBCS_CP(pHashN))
        {
            //
            //  Single Byte Character Code Page.
            //
            while ((pWCStr < pEndWCStr) && (pMBStr < pEndMBStr))
            {
                //
                //  Get the translation.
                //
                mbIncr = GetMBCompSB( pHashN,
                                      dwFlags,
                                      pWCStr,
                                      pMBStr,
                                      mbCount,
                                      wDefault,
                                      pUsedDef );
                pWCStr++;
                mbCount += mbIncr;
                pMBStr += mbIncr;
            }
        }
        else
        {
            //
            //  Multi Byte Character Code Page.
            //
            while ((pWCStr < pEndWCStr) && (pMBStr < pEndMBStr))
            {
                //
                //  Get the translation.
                //
                mbIncr = GetMBCompMB( pHashN,
                                      dwFlags,
                                      pWCStr,
                                      pMBStr,
                                      mbCount,
                                      wDefault,
                                      pUsedDef,
                                      &fError,
                                      ((pMBStr + 1) < pEndMBStr) ? FALSE : TRUE );
                if (fError)
                {
                    //
                    //  Not enough room in the buffer.
                    //
                    break;
                }

                pWCStr++;
                mbCount += mbIncr;
                pMBStr += mbIncr;
            }
        }

        //
        //  Make sure multibyte character buffer was large enough.
        //
        if (pWCStr < pEndWCStr)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return (0);
        }
    }

    //
    //  Return the number of characters written (or that would have
    //  been written) to the buffer.
    //
    return (mbCount);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetMBCompSB
//
//  Fills in pMBStr with the byte character(s) for the corresponding wide
//  character from the appropriate translation table and returns the number
//  of byte characters written to pMBStr.  This routine is only called if
//  the defaultcheck and compositecheck flags were both set.
//
//  NOTE:  Most significant bit of dwFlags parameter is used by this routine
//         to indicate that the caller only wants the count of the number of
//         characters written, not the string (ie. do not back up in buffer).
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetMBCompSB(
    PCP_HASH pHashN,
    DWORD dwFlags,
    LPWSTR pWCStr,
    LPBYTE pMBStr,
    int mbCount,
    WORD wDefault,
    LPBOOL pUsedDef)
{
    WCHAR PreComp;                // precomposed wide character


    if (!IS_NONSPACE_ONLY(pTblPtrs->pDefaultSortkey, *pWCStr))
    {
        //
        //  Get the 1:1 translation from wide char to single byte.
        //
        GET_WC_TRANSLATION_SB( pHashN,
                               *pWCStr,
                               pMBStr,
                               wDefault,
                               pUsedDef,
                               dwFlags );
        return (1);
    }
    else
    {
        if (mbCount < 1)
        {
            //
            //  Need to handle the nonspace character by itself, since
            //  it is the first character in the string.
            //
            if (dwFlags & WC_DISCARDNS)
            {
                //
                //  Discard the non-spacing char, so just return with
                //  zero chars written.
                //
                return (0);
            }
            else if (dwFlags & WC_DEFAULTCHAR)
            {
                //
                //  Need to replace the nonspace character with the default
                //  character and return the number of characters written
                //  to the multibyte string.
                //
                *pUsedDef = TRUE;
                *pMBStr = LOBYTE(wDefault);
                return (1);
            }
            else                  // WC_SEPCHARS - default
            {
                //
                //  Get the 1:1 translation from wide char to multibyte
                //  of the non-spacing char and return the number of
                //  characters written to the multibyte string.
                //
                GET_WC_TRANSLATION_SB( pHashN,
                                       *pWCStr,
                                       pMBStr,
                                       wDefault,
                                       pUsedDef,
                                       dwFlags );
                return (1);
            }
        }
        else if (PreComp = GetPreComposedChar(*pWCStr, *(pWCStr - 1)))
        {
            //
            //  Back up in the single byte string and write the
            //  precomposed char.
            //
            if (!IS_MSB(dwFlags))
            {
                pMBStr--;
            }

            GET_WC_TRANSLATION_SB( pHashN,
                                   PreComp,
                                   pMBStr,
                                   wDefault,
                                   pUsedDef,
                                   dwFlags );
            return (0);
        }
        else
        {
            if (dwFlags & WC_DISCARDNS)
            {
                //
                //  Discard the non-spacing char, so just return with
                //  zero chars written.
                //
                return (0);
            }
            else if (dwFlags & WC_DEFAULTCHAR)
            {
                //
                //  Need to replace the base character with the default
                //  character.  Since we've already written the base
                //  translation char in the single byte string, we need to
                //  back up in the single byte string and write the default
                //  char.
                //
                if (!IS_MSB(dwFlags))
                {
                    pMBStr--;
                }

                *pUsedDef = TRUE;
                *pMBStr = LOBYTE(wDefault);
                return (0);
            }
            else                  // WC_SEPCHARS - default
            {
                //
                //  Get the 1:1 translation from wide char to multibyte
                //  of the non-spacing char and return the number of
                //  characters written to the multibyte string.
                //
                GET_WC_TRANSLATION_SB( pHashN,
                                       *pWCStr,
                                       pMBStr,
                                       wDefault,
                                       pUsedDef,
                                       dwFlags );
                return (1);
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  GetMBCompMB
//
//  Fills in pMBStr with the byte character(s) for the corresponding wide
//  character from the appropriate translation table and returns the number
//  of byte characters written to pMBStr.  This routine is only called if
//  the defaultcheck and compositecheck flags were both set.
//
//  If the buffer was too small, the fError flag will be set to TRUE.
//
//  NOTE:  Most significant bit of dwFlags parameter is used by this routine
//         to indicate that the caller only wants the count of the number of
//         characters written, not the string (ie. do not back up in buffer).
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetMBCompMB(
    PCP_HASH pHashN,
    DWORD dwFlags,
    LPWSTR pWCStr,
    LPBYTE pMBStr,
    int mbCount,
    WORD wDefault,
    LPBOOL pUsedDef,
    BOOL *fError,
    BOOL fOnlyOne)
{
    WCHAR PreComp;                // precomposed wide character
    BYTE pTmpSp[2];               // temp space - 2 bytes for DBCS
    int nCnt;                     // number of characters written


    *fError = FALSE;
    if (!IS_NONSPACE_ONLY(pTblPtrs->pDefaultSortkey, *pWCStr))
    {
        //
        //  Get the 1:1 translation from wide char to multibyte.
        //  This also handles DBCS and returns the number of characters
        //  written to the multibyte string.
        //
        GET_WC_TRANSLATION_MB( pHashN,
                               *pWCStr,
                               pMBStr,
                               wDefault,
                               pUsedDef,
                               nCnt,
                               fOnlyOne,
                               dwFlags );
        if (nCnt == 0)
        {
            *fError = TRUE;
        }
        return (nCnt);
    }
    else
    {
        if (mbCount < 1)
        {
            //
            //  Need to handle the nonspace character by itself, since
            //  it is the first character in the string.
            //
            if (dwFlags & WC_DISCARDNS)
            {
                //
                //  Discard the non-spacing char, so just return with
                //  zero chars written.
                //
                return (0);
            }
            else if (dwFlags & WC_DEFAULTCHAR)
            {
                //
                //  Need to replace the nonspace character with the default
                //  character and return the number of characters written
                //  to the multibyte string.
                //
                *pUsedDef = TRUE;
                COPY_MB_CHAR( wDefault,
                              pMBStr,
                              nCnt,
                              fOnlyOne );
                if (nCnt == 0)
                {
                    *fError = TRUE;
                }
                return (nCnt);
            }
            else                  // WC_SEPCHARS - default
            {
                //
                //  Get the 1:1 translation from wide char to multibyte
                //  of the non-spacing char and return the number of
                //  characters written to the multibyte string.
                //
                GET_WC_TRANSLATION_MB( pHashN,
                                       *pWCStr,
                                       pMBStr,
                                       wDefault,
                                       pUsedDef,
                                       nCnt,
                                       fOnlyOne,
                                       dwFlags );
                if (nCnt == 0)
                {
                    *fError = TRUE;
                }
                return (nCnt);
            }

        }
        else if (PreComp = GetPreComposedChar(*pWCStr, *(pWCStr - 1)))
        {
            //
            //  Get the 1:1 translation from wide char to multibyte
            //  of the precomposed char, back up in the multibyte string,
            //  write the precomposed char, and return the DIFFERENCE of
            //  the number of characters written to the the multibyte
            //  string.
            //
            GET_WC_TRANSLATION_MB( pHashN,
                                   *(pWCStr - 1),
                                   pTmpSp,
                                   wDefault,
                                   pUsedDef,
                                   nCnt,
                                   fOnlyOne,
                                   dwFlags );
            if (nCnt == 0)
            {
                *fError = TRUE;
                return (nCnt);
            }

            if (!IS_MSB(dwFlags))
            {
                pMBStr -= nCnt;
            }

            GET_WC_TRANSLATION_MB( pHashN,
                                   PreComp,
                                   pMBStr,
                                   wDefault,
                                   pUsedDef,
                                   mbCount,
                                   fOnlyOne,
                                   dwFlags );
            if (mbCount == 0)
            {
                *fError = TRUE;
            }
            return (mbCount - nCnt);
        }
        else
        {
            if (dwFlags & WC_DISCARDNS)
            {
                //
                //  Discard the non-spacing char, so just return with
                //  zero chars written.
                //
                return (0);
            }
            else if (dwFlags & WC_DEFAULTCHAR)
            {
                //
                //  Need to replace the base character with the default
                //  character.  Since we've already written the base
                //  translation char in the multibyte string, we need to
                //  back up in the multibyte string and return the
                //  DIFFERENCE of the number of characters written
                //  (could be negative).
                //

                //
                //  If the previous character written is the default
                //  character, then the base character for this nonspace
                //  character has already been replaced.  Simply throw
                //  this character away and return zero chars written.
                //
                if (!IS_MSB(dwFlags))
                {
                    //
                    //  Not using a temporary buffer, so find out if the
                    //  previous character translated was the default char.
                    //
                    if ((MAKEWORD(*(pMBStr - 1), 0) == wDefault) ||
                        ((mbCount > 1) &&
                         (MAKEWORD(*(pMBStr - 1), *(pMBStr - 2)) == wDefault)))
                    {
                        return (0);
                    }
                }
                else
                {
                    //
                    //  Using a temporary buffer.  The temp buffer is 2 bytes
                    //  in length and contains the previous character written.
                    //
                    if ((MAKEWORD(*pMBStr, 0) == wDefault) ||
                        ((mbCount > 1) &&
                         (MAKEWORD(*pMBStr, *(pMBStr + 1)) == wDefault)))
                    {
                        return (0);
                    }
                }

                //
                //  Get the 1:1 translation from wide char to multibyte
                //  of the base char, back up in the multibyte string,
                //  write the default char, and return the DIFFERENCE of
                //  the number of characters written to the the multibyte
                //  string.
                //
                GET_WC_TRANSLATION_MB( pHashN,
                                       *(pWCStr - 1),
                                       pTmpSp,
                                       wDefault,
                                       pUsedDef,
                                       nCnt,
                                       fOnlyOne,
                                       dwFlags );
                if (nCnt == 0)
                {
                    *fError = TRUE;
                    return (nCnt);
                }

                if (!IS_MSB(dwFlags))
                {
                    pMBStr -= nCnt;
                }

                *pUsedDef = TRUE;
                COPY_MB_CHAR( wDefault,
                              pMBStr,
                              mbCount,
                              fOnlyOne );
                if (mbCount == 0)
                {
                    *fError = TRUE;
                }
                return (mbCount - nCnt);
            }
            else                  // WC_SEPCHARS - default
            {
                //
                //  Get the 1:1 translation from wide char to multibyte
                //  of the non-spacing char and return the number of
                //  characters written to the multibyte string.
                //
                GET_WC_TRANSLATION_MB( pHashN,
                                       *pWCStr,
                                       pMBStr,
                                       wDefault,
                                       pUsedDef,
                                       nCnt,
                                       fOnlyOne,
                                       dwFlags );
                if (nCnt == 0)
                {
                    *fError = TRUE;
                }
                return (nCnt);
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  GetMacCodePage
//
//  Returns the system default Mac code page.
//
//  09-22-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

UINT GetMacCodePage()
{
    PKEY_VALUE_FULL_INFORMATION pKeyValueFull;   // ptr to query information
    BYTE pStatic[MAX_KEY_VALUE_FULLINFO];        // ptr to static buffer
    UNICODE_STRING ObUnicodeStr;                 // unicode string
    UINT CodePage;                               // code page value
    PCP_HASH pHashN;                             // ptr to hash node


    //
    //  See if the Mac code page globals have been initialized yet.
    //  If they have, return the mac code page value.
    //
    if (gMacCodePage != 0)
    {
        return (gMacCodePage);
    }

    //
    //  Make sure code page key is open.
    //
    OPEN_CODEPAGE_KEY(NLS_DEFAULT_MACCP);

    //
    //  Query the registry for the Mac CP value.
    //
    CodePage = 0;
    pKeyValueFull = (PKEY_VALUE_FULL_INFORMATION)pStatic;
    if ((QueryRegValue( hCodePageKey,
                        NLS_VALUE_MACCP,
                        &pKeyValueFull,
                        MAX_KEY_VALUE_FULLINFO,
                        NULL )) == NO_ERROR)
    {
        //
        //  Convert the value to an integer.
        //
        RtlInitUnicodeString(&ObUnicodeStr, GET_VALUE_DATA_PTR(pKeyValueFull));
        if (RtlUnicodeStringToInteger(&ObUnicodeStr, 10, (PULONG)&CodePage))
        {
            CodePage = 0;
        }
    }

    //
    //  Make sure the CodePage value was set.
    //
    if (CodePage == 0)
    {
        //
        //  Registry value is corrupt, so use default Mac code page.
        //
        CodePage = NLS_DEFAULT_MACCP;
    }

    //
    //  Get the hash node for the Mac code page.
    //
    pHashN = GetCPHashNode(CodePage);

    //
    //  Make sure the Mac hash node is valid.
    //
    if (pHashN == NULL)
    {
        //
        //  Invalid hash node, which means either the registry is
        //  corrupt, or setup failed to install a file.  Use the
        //  Ansi code page values.
        //
        CodePage = gAnsiCodePage;
        pHashN = gpACPHashN;
    }

    //
    //  Set the final MAC CP values.
    //
    RtlEnterCriticalSection(&gcsTblPtrs);

    if (gMacCodePage == 0)
    {
        gpMACCPHashN = pHashN;
        gMacCodePage = CodePage;
    }

    RtlLeaveCriticalSection(&gcsTblPtrs);

    //
    //  Return the Mac code page value.
    //
    return (gMacCodePage);
}


////////////////////////////////////////////////////////////////////////////
//
//  SpecialMBToWC
//
//  Maps a multibyte character string to its wide character string
//  counterpart.
//
//  08-21-95    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int SpecialMBToWC(
    PCP_HASH pHashN,
    DWORD dwFlags,
    LPCSTR lpMultiByteStr,
    int cbMultiByte,
    LPWSTR lpWideCharStr,
    int cchWideChar)
{
    register LPBYTE pMBStr;       // ptr to search through MB string
    register LPWSTR pWCStr;       // ptr to search through WC string
    LPBYTE pEndMBStr;             // ptr to end of MB search string
    LPWSTR pEndWCStr;             // ptr to end of WC string buffer
    int mbIncr;                   // amount to increment pMBStr
    int wcCount = 0;              // count of wide chars written
    PMB_TABLE pMBTbl;             // ptr to MB table
    int ctr;                      // loop counter


    //
    //  Initialize multibyte character loop pointers.
    //
    pMBStr = (LPBYTE)lpMultiByteStr;
    pEndMBStr = pMBStr + cbMultiByte;

    //
    //  Get the MB table.
    //
    pMBTbl = pHashN->pMBTbl;

    //
    //  If cchWideChar is 0, then we can't use lpWideCharStr.  In this
    //  case, we simply want to count the number of characters that would
    //  be written to the buffer.
    //
    if (cchWideChar == 0)
    {
        //
        //  For each multibyte char, translate it to its corresponding
        //  wide char and increment the wide character count.
        //
        if (IS_SBCS_CP(pHashN))
        {
            //
            //  Single Byte Character Code Page.
            //
            wcCount = (int)(pEndMBStr - pMBStr);
        }
        else
        {
            //
            //  Multi Byte Character Code Page.
            //
            WCHAR pTempStr[MAX_COMPOSITE];   // tmp buffer

            pEndWCStr = pTempStr + MAX_COMPOSITE;
            while (pMBStr < pEndMBStr)
            {
                GET_WC_MULTI( pHashN,
                              pMBTbl,
                              pMBStr,
                              pEndMBStr,
                              pTempStr,
                              pEndWCStr,
                              mbIncr );
                pMBStr += mbIncr;
                wcCount++;
            }
        }
    }
    else
    {
        //
        //  Initialize wide character loop pointers.
        //
        pWCStr = lpWideCharStr;
        pEndWCStr = pWCStr + cchWideChar;

        //
        //  For each multibyte char, translate it to its corresponding
        //  wide char, store it in lpWideCharStr, and increment the wide
        //  character count.
        //
        if (IS_SBCS_CP(pHashN))
        {
            //
            //  Single Byte Character Code Page.
            //
            wcCount = (int)(pEndMBStr - pMBStr);
            if ((pEndWCStr - pWCStr) < wcCount)
            {
                wcCount = (int)(pEndWCStr - pWCStr);
            }

            if (dwFlags & MB_INVALID_CHAR_CHECK)
            {
                //
                //  Error check flag is set.
                //
                for (ctr = wcCount; ctr > 0; ctr--)
                {
                    GET_WC_SINGLE_SPECIAL( pHashN,
                                           pMBTbl,
                                           pMBStr,
                                           pWCStr );
                    pMBStr++;
                    pWCStr++;
                }
            }
            else
            {
                //
                //  Error check flag is NOT set.
                //
                for (ctr = wcCount; ctr > 0; ctr--)
                {
                    GET_WC_SINGLE( pMBTbl,
                                   pMBStr,
                                   pWCStr );
                    pMBStr++;
                    pWCStr++;
                }
            }
        }
        else
        {
            //
            //  Multi Byte Character Code Page.
            //
            if (dwFlags & MB_INVALID_CHAR_CHECK)
            {
                //
                //  Error check flag is set.
                //
                while ((pMBStr < pEndMBStr) && (pWCStr < pEndWCStr))
                {
                    GET_WC_MULTI_ERR_SPECIAL( pHashN,
                                              pMBTbl,
                                              pMBStr,
                                              pEndMBStr,
                                              pWCStr,
                                              pEndWCStr,
                                              mbIncr );
                    pMBStr += mbIncr;
                    pWCStr++;
                }
                wcCount = (int)(pWCStr - lpWideCharStr);
            }
            else
            {
                //
                //  Error check flag is NOT set.
                //
                while ((pMBStr < pEndMBStr) && (pWCStr < pEndWCStr))
                {
                    GET_WC_MULTI( pHashN,
                                  pMBTbl,
                                  pMBStr,
                                  pEndMBStr,
                                  pWCStr,
                                  pEndWCStr,
                                  mbIncr );
                    pMBStr += mbIncr;
                    pWCStr++;
                }
                wcCount = (int)(pWCStr - lpWideCharStr);
            }
        }

        //
        //  Make sure wide character buffer was large enough.
        //
        if (pMBStr < pEndMBStr)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return (0);
        }
    }

    //
    //  Return the number of characters written (or that would have
    //  been written) to the buffer.
    //
    return (wcCount);
}
