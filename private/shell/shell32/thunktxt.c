//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:       thunktxt.c
//
//  Contents:   Support routines to thunk API parameters ANSI <-> UNICODE
//
//  Functions:  ConvertStrings()
//
//  History:    2-03-95   davepl   Created
//
//--------------------------------------------------------------------------

#include <shellprv.h>
#pragma  hdrstop

//+-------------------------------------------------------------------------
//
//  Function:   ConvertStrings
//
//  Synopsis:   Converts a series of XCHAR strings into TCHAR strings,
//              packed as a series of pointers followed by a contiguous
//              block of memory where the output strings are stored.
//
//              Eg: ConvertStrings(4, "Hello", "", NULL, "World");
//
//              Returns a pointer to a block of memory as follows:
//
//              4  bytes         <address of L"Hello">
//              4  bytes         <address of L"">
//              4  bytes         NULL
//              4  bytes         <address of L"World">
//              12 bytes         L"Hello\0"
//              2  bytes         L"\0"
//              12 bytes         L"World\0"
//              ---------------------------------------------------
//              42 bytes
//
//              The strings may then be referenced as ThunkText.m_pStr[0],
//              [1], [2], and [3], where [2] is a NULL pointer.
//
//              When the caller is finished with the strings, the entire
//              block should be freed via LocalAlloc().
//
//  Arguments:  [cCount]            -- Number of strings passed, incl NULs
//              [pszOriginalString] -- The strings to convert
//              (... etc ...)
//
//  Returns:    Pointer to a ThunkText structure
//
//  History:    2-03-95   davepl   Created
//
//  Notes:      In UNICODE builds, converts ANSI to UNICODE.  In ANSI
//              builds, converts to UNICODE (if present).
//
//--------------------------------------------------------------------------

#ifdef UNICODE

ThunkText * ConvertStrings(UINT cCount, ...)
{
    ThunkText *  pThunkText   = NULL;
    UINT         cTmp;
    LPXSTR       pXChar;

    va_list     vaListMarker;

    //
    // Byte count is size of fixed members plus cCount pointers.  cbOffset
    // is the offset at which we will begin dumping strings into the struct
    //

    UINT cbStructSize =  SIZEOF(ThunkText) + (cCount - 1) * SIZEOF(LPTSTR);
    UINT cbOffset     =  cbStructSize;

    //
    // Scan the list of input strings, and add their lengths (in bytes, once
    // converted to TCHARs, incl NUL) to the output structure size
    //

    cTmp = 0;
    va_start(vaListMarker, cCount);
    do
    {
        pXChar = va_arg(vaListMarker, LPXSTR);
        if (pXChar)
        {
            cbStructSize += (lstrlenX(pXChar) + 1) * SIZEOF(TCHAR);
        }
        cTmp++;
    }
    while (cTmp < cCount);

    //
    // Allocate the output structure.
    //

    pThunkText = (ThunkText *) LocalAlloc(LMEM_FIXED, cbStructSize);
    if (NULL == pThunkText)
    {
        SetLastError((DWORD)E_OUTOFMEMORY);     // BUGBUG - Need better error value (win32 error value, not ole)
        return NULL;
    }

    //
    // Convert each of the input strings into the allocated output
    // buffer.
    //

    cTmp = 0;
    va_start(vaListMarker, cCount);
    do
    {
        INT cchResult;
        
        pXChar = va_arg(vaListMarker, LPXSTR);      // grab next src XSTR

        if (NULL == pXChar)
        {
            pThunkText->m_pStr[cTmp] = NULL;
        }
        else
        {
            pThunkText->m_pStr[cTmp] = (LPTSTR)(((LPBYTE)pThunkText) + cbOffset);

        

            #ifdef UNICODE

            cchResult = MultiByteToWideChar(CP_ACP,      // code page
                                            0,           // flags
                                            pXChar,      // source XCHAR
                                            -1,          // assume NUL term
                                            pThunkText->m_pStr[cTmp],  //outbuf
                                            (cbStructSize - cbOffset) / sizeof(WCHAR) ); //buflen
            #else

            cchResult = WideCharToMultiByte(CP_ACP,      // code page
                                            0,           // flags
                                            pXChar,      // source XCHAR
                                            -1,          // assume NUL term
                                            pThunkText->m_pStr[cTmp], //outbuf
                                            (cbStructSize - cbOffset) / sizeof(CHAR),  //buflen
                                            NULL,        // default char
                                            NULL);       // &fDefUsed
            #endif

            //
            // Even a NUL string returns a 1 character conversion, so 0 means
            // the conversion failed.  Cleanup and bail.
            //

            if (0 == cchResult)
            {
                LocalFree(pThunkText);
                SetLastError((DWORD)E_FAIL);        // BUGBUG - Need better error value
                return NULL;
            }

            cbOffset += cchResult * SIZEOF(TCHAR);
         }
         cTmp++;
    } while (cTmp < cCount);

    return pThunkText;
}

#endif // UNICODE
