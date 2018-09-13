//=--------------------------------------------------------------------------=
// Debug.H
//=--------------------------------------------------------------------------=
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// contains the various macros and the like which are only useful in DEBUG
// builds
//
#ifndef _DEBUG_H_

//=---------------------------------------------------------------------------=
// all the things required to handle our ASSERT mechanism
//=---------------------------------------------------------------------------=
//
#if DEBUG

// Function Prototypes
//
VOID DisplayAssert(LPSTR pszMsg, LPSTR pszAssert, LPSTR pszFile, UINT line);

// Macros
//
// *** Include this macro at the top of any source file using *ASSERT*() macros ***
//
#define SZTHISFILE	static char _szThisFile[] = __FILE__;


// our versions of the ASSERT and FAIL macros.
//
#define ASSERT(fTest, szMsg)                                \
    if (!(fTest))  {                                        \
        static char szMsgCode[] = szMsg;                    \
        static char szAssert[] = #fTest;                    \
        DisplayAssert(szMsgCode, szAssert, _szThisFile, __LINE__); \
    }

#define FAIL(szMsg)                                         \
        { static char szMsgCode[] = szMsg;                    \
        DisplayAssert(szMsgCode, "FAIL", _szThisFile, __LINE__); }



// macro that checks a pointer for validity on input
//
#define CHECK_POINTER(val) if (!(val) || IsBadWritePtr((void *)(val), sizeof(void *))) return E_POINTER

#else  // DEBUG

#define SZTHISFILE
#define ASSERT(fTest, err)
#define FAIL(err)

#define CHECK_POINTER(val)
#endif	// DEBUG




#define _DEBUG_H_
#endif // _DEBUG_H_

