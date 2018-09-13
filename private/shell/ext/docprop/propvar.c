
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
//
//  File:       PropVar.c
//
//  Purpose:    This file provides Office-aware routines which 
//              operate on PropVariants.  They are Office-aware in
//              that they only operate on the subset of
//              VarTypes which are used by Office.
//
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////


#include "priv.h"
#pragma hdrstop


/////////////////////////////////////////////////////////////////////////////////
//
//  Function:   FPropVarLoad
//
//  Purpse:     Load data into a PropVariant.  If the target PropVariant
//              already contains data, it will be freed.
//
//              Note that new memory is allocated, if necessary, to hold
//              the data in the PropVariant.  Also note that the
//              resulting PropVariant should be freed by the caller using
//              PropVariantClear.
//
//  Inputs:     LPPROPVARIANT - to be loaded.  This should be a valid
//                              (i.e. intialized) PropVariant.
//              VARTYPE       - of the new PropVariant (must be a member of
//                              the limited set used by Office).
//              LPVOID        - Either the data to be loaded, or a pointer
//                              to such data, depending on the type.
//
//  Output:     TRUE if and only if successful.
//
/////////////////////////////////////////////////////////////////////////////////


BOOL
FPropVarLoad
    ( LPPROPVARIANT     lppropvar,
      VARTYPE           vt,
      LPVOID const      lpv )
{
    //  ------
    //  Locals
    //  ------

    BOOL    fSuccess = FALSE;   // Return code
    ULONG   cch, cb;

    //  ----------
    //  Initialize
    //  ----------

    Assert (lppropvar != NULL);
    Assert (lpv != NULL);

    // Free any data currently in the PropVariant.

    PropVariantClear (lppropvar);

    //  ---------------------------------------------------
    //  Set the value of the PropVariant based on the type.
    //  ---------------------------------------------------

    switch (vt)
    {
        // Strings

        case VT_LPTSTR:

            // Determine the character and byte count.

            cch = CchTszLen(lpv);    // Doesn't include the NULL.
            cb = CBTSTR(lpv);       // *Does* include the NULL.

            // Allocate memory in the PropVariant.

            lppropvar->pszVal = CoTaskMemAlloc (cb);
            if (lppropvar->pszVal == NULL)
            {
                MESSAGE(TEXT("Couldn't allocate new VT_LPTSTR"));
                goto Exit;
            }

            // Copy the string to the PropVariant and terminate it.

            PbSzNCopy (lppropvar->pszVal, lpv, cch);
            ((LPTSTR)lppropvar->pszVal)[cch] = TEXT('\0');

            break;

        // DWORDs

        case VT_I4:

            lppropvar->lVal = *(DWORD*) lpv;
            break;

        // FileTime

        case VT_FILETIME:

            PbMemCopy (&lppropvar->filetime, lpv, sizeof(FILETIME));
            break;

        // Double

        case VT_R8:
            PbMemCopy (&lppropvar->dblVal, lpv, sizeof(double));
            break;

        // Bool

        case VT_BOOL:
            lppropvar->boolVal = *(VARIANT_BOOL*) lpv ? VARIANT_TRUE : VARIANT_FALSE;
            break;

        // Invalid type.

        default:
            MESSAGE(TEXT("Invalid VarType"));
            goto Exit;
    }

    // Set the VT of the PropVariant, and we're done.

    lppropvar->vt = vt;

    //  ----
    //  Exit
    //  ----

    fSuccess = TRUE;

Exit:
    return (fSuccess);

}   // FPropVarLoad



////////////////////////////////////////////////////////////////////////////////
//
//  Function:   FCoStrToWStr
//
//  Purpose:    Convert a COM string (ANSI) to a COM wide-string.
//              ("COM" because the string is allocated using
//              the COM heap).
//
//  Inputs:     LPWSTR* - The converted string.
//              LPSTR   - The original string.
//              UINT    - The ANSI code page.
//
//  Output:     TRUE if and only if successful.
//
////////////////////////////////////////////////////////////////////////////////

BOOL
FCoStrToWStr( LPWSTR            *lplpwstr,
              const LPSTR       lpstr,
              UINT              uCodePage)
{
    //  ------
    //  Locals
    //  ------

    BOOL fSuccess = FALSE;  // Return value.
    ULONG cchBuffer = 0;    // Size of converted string (includes NULL).

    Assert (lpstr != NULL && lplpwstr != NULL);

    //  ------------------
    //  Convert the string
    //  ------------------

    // Make two passes.  The first will calculate the
    // size of the target buffer, the second will actually
    // make the conversion.

    *lplpwstr = NULL;

    while (TRUE)
    {
        cchBuffer = MultiByteToWideChar(
                        uCodePage,          // Source code page
                        0,                  // Default flags
                        lpstr,              // Source string
                        -1,                 // Default length
                        *lplpwstr,          // Destination string
                        cchBuffer );        // Max dest string characters.

        // Is this the second pass (when the conversion should
        // have taken place)?

        if (*lplpwstr != NULL)
        {
            // If we got a good result, then we're done.
            if (cchBuffer != 0)
            {
                break;
            }

            // 0 was returned.  There was an error.
            else
            {
                AssertSz (0, TEXT("Couldn't convert MBCS to Wide"));
                goto Exit;
            }
        }

        // Otherwise, this is the first pass.  We need to
        // allocate a buffer.

        else
        {
            // We should have gotten a positive buffer size.

            if (cchBuffer == 0)
            {
                AssertSz(0, TEXT("MultiByteToWideChar returned invalid target buffer size"));
                goto Exit;
            }

            // Allocate memory for the converted string.
            else
            {
                *lplpwstr = (LPWSTR) CoTaskMemAlloc( cchBuffer * 2 );
                if ( *lplpwstr == NULL)
                {
                    AssertSz (0, TEXT("Could not allocate memory for wide string"));
                    goto Exit;
                }
            }
        }   // if( *lplpwstr != NULL ... else
    }   // while (TRUE)



    //  ----
    //  Exit
    //  ----

    fSuccess = TRUE;

Exit:

    // If there was a problem, free the Unicode string.

    if (!fSuccess)
    {
        CoTaskMemFree (*lplpwstr);
        *lplpwstr = NULL;
    }

    return (fSuccess);

}   // FCoStrToWStr


////////////////////////////////////////////////////////////////////////////////
//
//  Function:   FCoWStrToStr
//
//  Purpose:    Convert a COM wide-string to an ANSI string.
//              ("COM" because the string is allocated using
//              the COM heap).
//
//  Inputs:     LPSTR*  - The converted string.
//              LPWSTR  - The source string.
//              UINT    - The ANSI code page.
//
//  Output:     TRUE if and only if successful.
//
////////////////////////////////////////////////////////////////////////////////

BOOL
FCoWStrToStr( LPSTR             *lplpstr,
              const LPWSTR      lpwstr,
              UINT              uCodePage)
{
    //  ------
    //  Locals
    //  ------

    BOOL fSuccess = FALSE;  // Return result
    ULONG cch;              // Charcters in original string (w/o NULL).
    ULONG cbBuffer = 0;     // Size of target buffer (including NULL)

    Assert (lpwstr != NULL && lplpstr != NULL);

    //  ------------------
    //  Convert the String
    //  ------------------

    // We'll make two calls to WideCharToMultiByte.
    // In the first, we'll determine the size required
    // for the multi-byte string.  We'll use this
    // to allocate memory.  In the second pass, we'll actually
    // make the conversion.

    cch = CchWszLen( lpwstr );   // How big is the source string?
    *lplpstr = NULL;            // Initialize the target buffer.

    while (TRUE)
    {
        cbBuffer = WideCharToMultiByte(
                    uCodePage,         // Source code page
                    0,                  // Default flags
                    lpwstr,             // Source string
                    cch + 1,            // # chars in wide string (including NULL)
                    *lplpstr,           // Destination string
                    cbBuffer,           // Size of destination buffer
                    NULL, NULL );       // No default character


        // Is this the second pass (when the conversion should
        // have taken place)?

        if (*lplpstr != NULL)
        {
            // If we got a good result, then we're done.
            if (cbBuffer != 0)
            {
                break;
            }

            // 0 was returned.  There was an error.
            else
            {
                AssertSz (0, TEXT("Couldn't convert Wide to MBCS"));
                goto Exit;
            }
        }

        // Otherwise, this is the first pass.  We need to
        // allocate a buffer.

        else
        {
            // We should have gotten a positive buffer size.

            if (cbBuffer == 0)
            {
                AssertSz(0, TEXT("WideCharMultiByte returned invalid target buffer size"));
                goto Exit;
            }

            // Allocate memory for the converted string.
            else
            {
                *lplpstr = (LPSTR) CoTaskMemAlloc( cbBuffer );
                if ( *lplpstr == NULL)
                {
                    AssertSz (0, TEXT("Could not allocate memory for wide string"));
                    goto Exit;
                }
            }
        }   // if( lpstr != NULL ... else
    }   //  while (TRUE)
        

    //  ----
    //  Exit
    //  ----

    fSuccess = TRUE;

Exit:

    // If there was a problem, free the new string.

    if (!fSuccess)
    {
        CoTaskMemFree (*lplpstr);
        *lplpstr = NULL;
    }

    return (fSuccess);

}   // FCoWStrToStr




//////////////////////////////////////////////////////////////////////////////
//
//  Function:   FPropVarConvertString
//
//  Purpose:    Convert a PropVariant from VT_LPSTR to VT_LPWSTR,
//              or vice-versa.  The correct direction is inferred
//              from the input.  The source PropVariant is not 
//              modified.
//
//              If the PropVariant is a Vector, all elements are
//              converted.
//
//  Inputs:     LPPROPVARIANT   - The buffer in which to put the
//                                converted PropVariant.
//              LPPROPVARIANT   - The source of the conversion.
//              UINT            - The code page of VT_LPSTRs.
//
//  Output:     TRUE if successful.  If unsuccessful, the original 
//              PropVariant will be returned unmodified.
//
//  Pre-Conditions:
//              The input must be either a VT_LPSTR or a VT_LPWSTR
//              (with or without the VT_VECTOR bit set).
//              &&
//              The destination PropVariant is VT_EMPTY.
//              &&
//              The code page must not be CP_WINUNICODE (Unicode
//              LPSTRs are not legal in the SumInfo property sets).
//
//////////////////////////////////////////////////////////////////////////////

BOOL FPropVarConvertString( LPPROPVARIANT       lppropvarDest,
                            const LPPROPVARIANT lppropvarSource,
                            UINT                uCodePage)
{

    //  ------
    //  Locals
    //  ------

    BOOL    fSuccess = FALSE;   // Return code.

    BOOL    fConvertToAnsi;     // Indicates the direction of the conversion.
    LPSTR   *lplpstrDest;       // Pointer to pointer to a converted string.
    LPSTR   lpstrSource;        // Pointer to a string to be converted.
    ULONG   cElems;             // The number of strings still requiring conversion
    ULONG   ulIndex = 0;        // Index into vector (if this VT is a vector)

    //  ----------
    //  Initialize
    //  ----------

    Assert(lppropvarDest != NULL && lppropvarSource != NULL);
    Assert(lppropvarDest->vt == VT_EMPTY);
    Assert(uCodePage != CP_WINUNICODE);

    // Determine the direction of the conversion.

    fConvertToAnsi = (lppropvarSource->vt & ~VT_VECTOR) == VT_LPSTR
                     ? FALSE
                     : TRUE;

    //  -----------------------------------
    //  Initialize based on the Vector bit.
    //  -----------------------------------

    if (lppropvarSource->vt & VT_VECTOR)
    {
        // We're a vector.

        cElems = lppropvarSource->calpstr.cElems;

        // Allocate an array of string pointers, putting it in
        // lppropvarDest.

        lppropvarDest->calpstr.pElems = CoTaskMemAlloc( cElems
                                                        * sizeof(*lppropvarDest->calpstr.pElems) );
        if (lppropvarDest->calpstr.pElems == NULL)
        {
            AssertSz(0,TEXT("Couldn't allocate memory for pElemsNew"));
            goto Exit;
        }

        // Fill this new buffer so we don't get confused in the error path.

        FillBuf (lppropvarDest->calpstr.pElems, 0,
                 cElems * sizeof(*lppropvarDest->calpstr.pElems));

        lppropvarDest->calpstr.cElems = cElems;

        // Initialize the pointers for the first string to convert.

        lplpstrDest = &lppropvarDest->calpstr.pElems[ 0 ];
        lpstrSource = lppropvarSource->calpstr.pElems[ 0 ];

    }   // if (lppropvar->vt & VT_VECTOR)
    else
    {
        // We're not a vector, initialize to the only string.

        cElems = 1;

        lplpstrDest = &lppropvarDest->pszVal;
        lpstrSource = lppropvarSource->pszVal;
    }
        

    //  ---------------------
    //  Convert the String(s)
    //  ---------------------

    while (cElems)
    {

        if (fConvertToAnsi)
        {
            if (!FCoWStrToStr ((LPSTR*)lplpstrDest, (LPWSTR) lpstrSource,
                               uCodePage))
            {
                goto Exit;
            }
        }
        else
        {
            if (!FCoStrToWStr ((LPWSTR*) lplpstrDest, (LPSTR) lpstrSource,
                               uCodePage))
            {
                goto Exit;
            }
        }

        // Move on to the next entry, if there is one.

        if (--cElems)
        {
            ulIndex++;
            lplpstrDest = &lppropvarDest->calpstr.pElems[ ulIndex ];
            lpstrSource = lppropvarSource->calpstr.pElems[ ulIndex ];

        }   // if (--cElems)
    }   // while (cElems)


    // Switch the destination VT to VT_LPSTR (for ansi strings) or VT_LPWSTR
    // (for Unicode strings), preserving all other bits.

    lppropvarDest->vt = (lppropvarSource->vt & ~(VT_LPSTR|VT_LPWSTR))
                        |
                        (fConvertToAnsi ? VT_LPSTR : VT_LPWSTR);
                    

    //  ----
    //  Exit
    //  ----

    fSuccess = TRUE;

Exit:

    if (!fSuccess)
        PropVariantClear (lppropvarDest);

    return( fSuccess );

}   // FPropVarConvertString



////////////////////////////////////////////////////////////////////////////////
//
//  Function:   FPropVarCopyToBuf
//
//  Purpose:    Copy the value from a PropVariant to a buffer.
//              (The caller indicates how large the buffer is).
//              The VarType of the PropVar must be from the 
//              valid Office sub-set.
//
//  Inputs:     LPPROPVARIANT   - The source.
//              DWORD           - The size of the target buffer.
//              LPVOID          - The target buffer.
//
//  Output:     TRUE if successful.
//
////////////////////////////////////////////////////////////////////////////////

BOOL
FPropVarCopyToBuf
  (LPPROPVARIANT const lppropvar,
   DWORD cbMax,
   LPVOID lpvBuf)
{
    //  ------
    //  Locals
    //  ------

    BOOL    fSuccess = FALSE;
    DWORD   cb;

    //  ----------------
    //  Check the inputs
    //  ----------------

    Assert (lppropvar != NULL);
    Assert (lpvBuf != NULL);

    //  ---------------------
    //  Copy, based on the VT
    //  ---------------------

    switch (lppropvar->vt)
    {
        // File Time

        case VT_FILETIME :
            if (cbMax < sizeof(FILETIME))
                return(FALSE);
            PbMemCopy (lpvBuf, &lppropvar->filetime, sizeof(FILETIME));
            break;

        // Double

        case VT_R8:
            if (cbMax < sizeof(double))
                return(FALSE);
            PbMemCopy (lpvBuf, &lppropvar->dblVal, sizeof(double));

        // Boolean

        case VT_BOOL:
            if (cbMax < sizeof(WORD))
                return(FALSE);

            *(WORD *)lpvBuf = (WORD)lppropvar->boolVal;
            break;

        // DWORD

        case VT_I4:
            if (cbMax < sizeof(DWORD))
                return(FALSE);

            *(DWORD *)lpvBuf = (DWORD)lppropvar->ulVal;
            break;

        // String

        case VT_LPTSTR:
            if (lppropvar->pszVal == NULL)
                return(FALSE);

            // Calculate the number of characters to copy (not
            // including the NULL).  But don't let it exceed
            // cbMax - sizeof('\0').

            cb = min ( CchTszLen ((LPTSTR)lppropvar->pszVal) * sizeof(TCHAR),
                       cbMax - sizeof(TCHAR));

            PbMemCopy(lpvBuf, lppropvar->pszVal, cb);
            ((LPTSTR) lpvBuf)[ cb/sizeof(TCHAR) ] = TEXT('\0');
            break;

        // Invalid type.

        default :
            AssertSz (0, TEXT("Invalid type in FPropVarCopyToBuf"));
            goto Exit;

    }   // switch (lppropvar->vt)

    fSuccess = TRUE;

    //  ----
    //  Exit
    //  ----

Exit:

    return (fSuccess);

} // FPropVarCopyToBuf



