/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    macros.h

Abstract:

    Contains all internal macros used in INTERNET.DLL

    Contents:
        ROUND_UP_?K
        ROUND_UP_DWORD
        NEW
        DEL
        NEW_STRING
        DEL_STRING
        NEW_MEMORY
        ZAP
        PRIVATE
        PUBLIC
        GLOBAL
        LOCAL
        DEBUG_FUNCTION
        SKIPWS

Author:

    Richard L Firth (rfirth) 16-Nov-1994

Revision History:

    16-Nov-1994 rfirth
        Created

--*/

//
// macros
//

//
// ROUND_UP_ - return (n) rounded up to the next (k) bytes
//

#define ROUND_UP_NK(n, k)   (((n) + ((_ ## k ## K) - 1)) & -(_ ## k ## K))
#define ROUND_UP_2K(n)      ROUND_UP_NK(n, 2)
#define ROUND_UP_4K(n)      ROUND_UP_NK(n, 4)
#define ROUND_UP_8K(n)      ROUND_UP_NK(n, 8)
#define ROUND_UP_16K(n)     ROUND_UP_NK(n, 16)

//
// ROUND_UP_DWORD - return (n) rounded up to the next 4 bytes
//

#define ROUND_UP_DWORD(value) \
    (((value) + sizeof(DWORD) - 1) & ~(sizeof(DWORD) - 1))

//
// ARRAY_ELEMENTS - returns number of elements in array
//

#define ARRAY_ELEMENTS(array) \
    (sizeof(array)/sizeof(array[0]))

//
// NEW - allocates a new 'object' of type (obj). Memory is allocated with
// LocalAlloc and initialized to zeroes
//

#define NEW(object) \
    (object FAR *)ALLOCATE_ZERO_MEMORY(sizeof(object))

//
// DEL - (should be DELETE, but that symbol is taken) - does opposite of NEW()
//

#define DEL(object) \
    FREE_MEMORY(object)

//
// NEW_STRING - performs NEW for a string
//

#define NEW_STRING(string) \
    NewString(string, 0)

//
// DEL_STRING - performs DEL for a string
//

#define DEL_STRING(string) \
    FREE_MEMORY(string)

//
// NEW_MEMORY - performs NEW for arbitrary sized memory
//

#define NEW_MEMORY(n, type) \
    (type FAR *)ALLOCATE_FIXED_MEMORY(n * sizeof(type))

//
// ZAP - zeroes an object (must be a variable, not a pointer)
//

#define ZAP(thing) \
    ZeroMemory((PVOID)&thing, sizeof(thing))

//
// STRTOUL - character-width independent (compile-time controlled) strtoul
//

#define STRTOUL             strtoul

//
// PRIVATE - make static items visible in debug version *FOR GLOBALS ONLY*. Use
// LOCAL in functions
//

#if INET_DEBUG

#define PRIVATE

#else

//#define PRIVATE static
#define PRIVATE

#endif // INET_DEBUG

//
// PUBLIC - just used as an aide-a-programmer pour le nonce
//

#define PUBLIC

//
// GLOBAL - same as PUBLIC, aide-a-programmer (for now) that tells you this
// thang has global scope
//

#define GLOBAL

//
// LOCAL - always expands to static, so you know that this thing only has
// local scope (within the current block)
//

#define LOCAL   static

//
// DEBUG_FUNCTION - this is a debug-only routine (if it get compiled in retail
// version a compile-time error is generated)
//

#if INET_DEBUG

#define DEBUG_FUNCTION

#else

#define DEBUG_FUNCTION

#endif // INET_DEBUG

//
// SKIPWS - skips blank widespace on the front of a string
//

#define SKIPWS(s) while (*(s)==' ' || *(s)=='\t') (s)++;


//
// MAKE_LOWER - takes an assumed upper character and bit manipulates into a lower.
//              (make sure the character is Upper case alpha char to begin, otherwise it corrupts)
//

#define MAKE_LOWER(c) (c | 0x20)

//
// MAKE_UPPER - takes an assumed lower character and bit manipulates into a upper.
//              (make sure the character is Lower case alpha char to begin, otherwise it corrupts)
//

#define MAKE_UPPER(c) (c & 0xdf)

//
// FASTCALL - used to bypass problems that may arise with UNIX compilers
//

#ifdef FASTCALL
#undef FASTCALL
#endif

#ifdef unix
#define FASTCALL
#else
#define FASTCALL __fastcall
#endif


//
// macro to cast FILETIME to LONGLONG
//
#define FT2LL(x) ( ((LONGLONG)((x).dwLowDateTime)) | (((LONGLONG)((x).dwHighDateTime))<<32) )


//
// Inline function to handle adding LONGLONG to FILETIME.
//
static __inline
void AddLongLongToFT( IN OUT LPFILETIME lpft,
                      IN     LONGLONG   llVal )
{
        LONGLONG llTmp;

        llTmp = FT2LL(*lpft);
        llTmp += llVal;

        lpft->dwLowDateTime =  ((LPDWORD)&llTmp)[0];
        lpft->dwHighDateTime = ((LPDWORD)&llTmp)[1];
}





//
// Macro to compute the number of bytes between two pointers
// The type of this expression is size_t, a signed integral
// type matching the size of a pointer
//
#define PtrDifference(x,y)  ((LPBYTE)(x) - (LPBYTE)(y))

//
// Macro to typecast 64-bit quantity to 32-bits
// Asserts in debug mode if the typecast loses information
//
#ifdef  DBG
#define GuardedCast(x)      ((x)<=0xFFFFFFFFL ? (DWORD)(x) : (InternetAssert(FALSE, __FILE__, __LINE__), 0))
#else
#define GuardedCast(x)      (DWORD)(x)
#endif

// Macro for the most common case
#define PtrDiff32(x,y)      (GuardedCast(PtrDifference(x,y)))
