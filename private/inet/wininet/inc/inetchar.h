#ifndef INETCHAR_H

#define INETCHAR_H

/* Copyright (c) 1998  Microsoft Corporation

Module Name:

    inetchar.h

Abstract:

    macros for converting between Unicode and MultiByte characters.

    Contents:
        REASSIGN_ALLOC
        REASSIGN_SIZE
        ALLOC_MB
        UNICODE_TO_ANSI
        MAYBE_COPY_ANSI
        
Author:

    Ahsan S. Kabir  

Revision History:

    18Nov97 akabir
        Created

*/

//

// ---- Macros to simplify recovering values from memory packets -------------

#define REASSIGN_ALLOC(mp,ps,dw) \
    ps = mp.psStr; \
    dw = mp.dwAlloc;
    
#define REASSIGN_SIZE(mp,ps,dw) \
    ps = mp.psStr; \
    dw = mp.dwSize;


// -- (MAYBE_)ALLOC_MB ------------
// Macros to allocate enough memory for an ansi-equivalent string

#define ALLOC_MB(URLW,DWW,MPMP) { \
    MPMP.dwAlloc = ((DWW ? DWW : lstrlenW(URLW))+ 1)*sizeof(WCHAR); \
    MPMP.psStr = (LPSTR)ALLOC_BYTES(MPMP.dwAlloc*sizeof(CHAR)); }


// -- UNICODE_TO_ANSI -----
// Base case macro to convert from unicode to ansi
// We're subtracting 1 because we're converting the nullchar in dwAlloc.

#define UNICODE_TO_ANSI(pszW, mpA) \
    mpA.dwSize = \
        WideCharToMultiByte(CP_ACP,0,pszW,mpA.dwAlloc/sizeof(*pszW),mpA.psStr,mpA.dwAlloc,NULL,NULL) - 1;

#define UNICODE_TO_ANSI_CHECKED(pszW, mpA, pfNotSafe) \
    mpA.dwSize = \
        WideCharToMultiByte(CP_ACP,0,pszW,mpA.dwAlloc/sizeof(*pszW),mpA.psStr,mpA.dwAlloc,NULL,pfNotSafe) - 1;


// -- (MAYBE_)COPY_ANSI ----
//  Given ansi source and widechar destination pointers, convert from the former to latter

#define COPY_ANSI(mp,pszW,dwW) { dwW = MultiByteToWideChar(CP_ACP, 0, mp.psStr, mp.dwSize+1, pszW, dwW) - 1; }


#define MAYBE_COPY_ANSI(mp,pszW,dwW) { if (pszW && mp.psStr) \
            { dwW = MultiByteToWideChar(CP_ACP, 0, mp.psStr, mp.dwSize+1, pszW, dwW) - 1; } }


#endif

