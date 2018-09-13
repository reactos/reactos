/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vdmredir.h

Abstract:

    Contains common defines, structures, macros, etc. for VdmRedir. This file
    contains macros to read and write the 3 basic data structures from/to VDM
    memory. We *must* use these macros because the MIPS processor does not like
    unaligned data: a DWORD must be read/written on a DWORD boundary (low two
    bits in address = 00), a WORD must be read/written on a WORD boundary (low
    two bits in address = X0) and a BYTE can be read/written to any address (low
    two bits in address = XX). It is illegal to access a WORD at an address
    whose LSB is not 0, and a DWORD at an address whose 2 least significant bits
    are not both 0. Dos programs don't care much about alignment (smart ones do
    because there is a performance penalty for unaligned data on x86, but it
    still works). So we have to assume the worst case for MIPS and break down
    the read/writes of WORDs and DWORDs in VDM memory into BYTE read/writes

    In order to improve efficiency of load/store to potentially unaligned
    addresses, the following data pointer types are made available from this
    include file:

        ULPBYTE     - unaligned byte pointer (same as LPBYTE)
        ULPWORD     - unaligned word pointer
        ULPDWORD    - unaligned dword pointer

    NB. Dependent upon mvdm.h

Author:

    Richard L Firth (rfirth) 16-Sep-1991

Revision History:

    16-Sep-1991 rfirth
        Created

--*/

#ifndef _VDMREDIR_
#define _VDMREDIR_

#include <softpc.h>

//
// PRIVATE - make a routine/data type inaccessible outside current module, but
// only if not DEBUG version
//

#if DBG
#define PRIVATE
#else
#define PRIVATE static
#endif

//
// unaligned data pointer types. These produce exactly the same code as memory
// accesses through 'aligned' pointers on x86, but generate code specific to
// unaligned read/writes on MIPS (& other RISCs)
//

#ifdef UNALIGNED_VDM_POINTERS
typedef BYTE UNALIGNED * ULPBYTE;
typedef WORD UNALIGNED * ULPWORD;
typedef DWORD UNALIGNED * ULPDWORD;
#else
typedef LPBYTE ULPBYTE;
typedef LPWORD ULPWORD;
typedef LPDWORD ULPDWORD;
#endif

//
// misc. defines
//

#define BITS_IN_A_BYTE      8
#define LOCAL_DEVICE_PREFIX "\\\\."

//
//  Define network interrupt to be on Irql 14.
//  If NETWORK_ICA changes to ICA_MASTER then vrnetb.c should only execute 1 eoi
//  If either change then NETWORK_INTERRUPT in int5c.inc must also change.
//

#if defined(NEC_98)
#define NETWORK_ICA     ICA_MASTER
#define NETWORK_LINE    5
#else
#define NETWORK_ICA     ICA_SLAVE
#define NETWORK_LINE    6
#endif

//
// helper macros
//

//
// MAKE_DWORD - converts 2 16-bit words into a 32-bit double word
//
#define MAKE_DWORD(h, l)                ((DWORD)(((DWORD)((WORD)(h)) << 16) | (DWORD)((WORD)(l))))

//
// DWORD_FROM_WORDS - converts two 16-bit words into a 32-bit dword
//
#define DWORD_FROM_WORDS(h, l)          MAKE_DWORD((h), (l))

//
// HANDLE_FROM_WORDS - converts a pair of 16-bit words into a 32-bit handle
//
#define HANDLE_FROM_WORDS(h, l)         ((HANDLE)(MAKE_DWORD((h), (l))))

//
// POINTER_FROM_WORDS - returns a flat 32-bit VOID pointer (in the VDM) OR the
// NULL macro, given the 16-bit real-mode segment & offset. On x86 this will
// return 0 if we pass in 0:0 because all GetVDMAddr does is seg << 4 + off.
// The MIPS version adds this to the start of the virtual DOS memory. The
// problem arises when we have a NULL pointer, and want to keep it NULL - we
// convert it to non-NULL on not x86
//
//#define POINTER_FROM_WORDS(seg, off)    ((LPVOID)GetVDMAddr((seg), (off)))
//#define POINTER_FROM_WORDS(seg, off)    (((((DWORD)(seg)) << 16) | (off)) ? ((LPVOID)GetVDMAddr((seg), (off))) : ((LPVOID)0))

#define POINTER_FROM_WORDS(seg, off)    _inlinePointerFromWords((WORD)(seg), (WORD)(off))

//
// LPSTR_FROM_WORDS - returns a 32-bit pointer to an ASCIZ string given the
// 16-bit real-mode segment & offset
//
#define LPSTR_FROM_WORDS(seg, off)      ((LPSTR)POINTER_FROM_WORDS((seg), (off)))

//
// LPBYTE_FROM_WORDS - returns a 32-bit byte pointer given the 16-bit
// real-mode segment & offset
//
#define LPBYTE_FROM_WORDS(seg, off)     ((LPBYTE)POINTER_FROM_WORDS((seg), (off)))

//
// READ_FAR_POINTER - read the pair of words in VDM memory, currently pointed at
// by a 32-bit flat pointer and convert them to a 32-bit flat pointer
//
#define READ_FAR_POINTER(addr)          ((LPVOID)(POINTER_FROM_WORDS(GET_SELECTOR(addr), GET_OFFSET(addr))))

//
// READ_BYTE - retrieve a single byte from VDM memory. Both x86 and MIPS can
// handle reading a single byte without pain
//
#define READ_BYTE(addr)                 (*((LPBYTE)(addr)))

//
// READ_WORD - read a single 16-bit little-endian word from VDM memory. x86 can
// handle unaligned data, MIPS (&other RISCs) must be broken down into individual
// BYTE reads & the WORD pieced together by shifting & oring. If we are using
// UNALIGNED pointers then the RISC processor can handle non-aligned data
//
#ifdef i386
#define READ_WORD(addr)                 (*((LPWORD)(addr)))
#else
#ifdef UNALIGNED_VDM_POINTERS
#define READ_WORD(addr)                 (*((ULPWORD)(addr)))
#else
#define READ_WORD(addr)                 (((WORD)READ_BYTE(addr)) | (((WORD)READ_BYTE((LPBYTE)(addr)+1)) << 8))
#endif  // UNALIGNED_VDM_POINTERS
#endif  // i386

//
// READ_DWORD - read a 4-byte little-endian double word from VDM memory. x86 can
// handle unaligned data, MIPS (&other RISCs) must be broken down into individual
// BYTE reads & the DWORD pieced together by shifting & oring. If we are using
// UNALIGNED pointers then the RISC processor can handle non-aligned data
//
#ifdef i386
#define READ_DWORD(addr)                (*((LPDWORD)(addr)))
#else
#ifdef UNALIGNED_VDM_POINTERS
#define READ_DWORD(addr)                (*((ULPDWORD)(addr)))
#else
#define READ_DWORD(addr)                (((DWORD)READ_WORD(addr)) | (((DWORD)READ_WORD((LPWORD)(addr)+1)) << 16))
#endif  // UNALIGNED_VDM_POINTERS
#endif  // i386

//
// WRITE_BYTE - write a single byte in VDM memory. Both x86 and MIPS (RISC) can
// write a single byte to a non-aligned address
//
#define WRITE_BYTE(addr, value) (*(LPBYTE)(addr) = (BYTE)(value))

//
// WRITE_WORD - write a 16-bit little-endian value into VDM memory. x86 can write
// WORD data to non-word-aligned address; MIPS (& other RISCs) cannot, so we
// break down the write into 2 byte writes. If we are using UNALIGNED pointers
// then the MIPS (&other RISCs) can generate code to handle this situation
//
#ifdef i386
#define WRITE_WORD(addr, value)         (*((LPWORD)(addr)) = (WORD)(value))
#else
#ifdef UNALIGNED_VDM_POINTERS
#define WRITE_WORD(addr, value)         (*((ULPWORD)(addr)) = (WORD)(value))
#else
#define WRITE_WORD(addr, value) \
            {\
                ((LPBYTE)(addr))[0] = LOBYTE(value); \
                ((LPBYTE)(addr))[1] = HIBYTE(value); \
            }
#endif  // UNALIGNED_VDM_POINTERS
#endif  // i386

//
// WRITE_DWORD - write a 32-bit DWORD value into VDM memory. x86 can write
// DWORD data to non-dword-aligned address; MIPS (& other RISCs) cannot, so we
// break down the write into 4 byte writes. If we are using UNALIGNED pointers
// then the MIPS (&other RISCs) can generate code to handle this situation
//
#ifdef i386
#define WRITE_DWORD(addr, value)        (*((LPDWORD)(addr)) = (DWORD)(value))
#else
#ifdef UNALIGNED_VDM_POINTERS
#define WRITE_DWORD(addr, value)        (*((ULPDWORD)(addr)) = (DWORD)(value))
#else
#define WRITE_DWORD(addr, value) \
            { \
                ((LPBYTE)(addr))[0] = LOBYTE(LOWORD((DWORD)(value))); \
                ((LPBYTE)(addr))[1] = HIBYTE(LOWORD((DWORD)(value))); \
                ((LPBYTE)(addr))[2] = LOBYTE(HIWORD((DWORD)(value))); \
                ((LPBYTE)(addr))[3] = HIBYTE(HIWORD((DWORD)(value))); \
            }
#endif  // UNALIGNED_VDM_POINTERS
#endif  // i386

//
// WRITE_FAR_POINTER - write a 16:16 pointer into VDM memory. This is the same
// as writing a DWORD
//
#define WRITE_FAR_POINTER(addr, ptr)    WRITE_DWORD((addr), (DWORD)(ptr))

//
// GET_SELECTOR - retrieves the selector word from the intel 32-bit far pointer
// (DWORD) pointed at by <pointer> (remember: stored as offset, segment)
//
#define GET_SELECTOR(pointer)           READ_WORD((LPWORD)(pointer)+1)

//
// GET_SEGMENT - same as GET_SELECTOR
//
#define GET_SEGMENT(pointer)            GET_SELECTOR(pointer)

//
// GET_OFFSET - retrieves the offset word from an intel 32-bit far pointer
// (DWORD) pointed at by <pointer> (remember: stored as offset, segment)
//
#define GET_OFFSET(pointer)             READ_WORD((LPWORD)(pointer))

//
// SET_SELECTOR - writes a word into the segment word of a real-mode far pointer
// (DWORD) pointed at by <pointer> (remember: stored as offset, segment)
//
#define SET_SELECTOR(pointer, word)     WRITE_WORD(((LPWORD)(pointer)+1), (word))

//
// SET_SEGMENT - same as SET_SELECTOR
//
#define SET_SEGMENT(pointer, word)      SET_SELECTOR(pointer, word)

//
// SET_OFFSET - writes a word into the offset word of a real-mode far pointer
// (DWORD) pointed at by <pointer> (remember: stored as offset, segment)
//
#define SET_OFFSET(pointer, word)       WRITE_WORD((LPWORD)(pointer), (word))

//
// POINTER_FROM_POINTER - read a segmented pointer in the VDM from an address
// pointed at by a flat 32-bit pointer. Convert the segmented pointer to a
// flat pointer. SAME AS READ_FAR_POINTER
//
#define POINTER_FROM_POINTER(pointer)   POINTER_FROM_WORDS(GET_SELECTOR(pointer), GET_OFFSET(pointer))

//
// LPSTR_FROM_POINTER - perform a POINTER_FROM_POINTER, casting the result to
// a string pointer. SAME AS READ_FAR_POINTER
//
#define LPSTR_FROM_POINTER(pointer)     ((LPSTR)POINTER_FROM_POINTER(pointer))

//
// LPBYTE_FROM_POINTER - perform a POINTER_FROM_POINTER, casting the result to
// a byte pointer. SAME AS READ_FAR_POINTER
//
#define LPBYTE_FROM_POINTER(pointer)    ((LPBYTE)POINTER_FROM_POINTER(pointer))

//
// SET_ERROR - sets the caller's AX register in the VDM context descriptor to
// the value given and sets the caller's VDM carry flag
//
#define SET_ERROR(err)                  {setAX(err); setCF(1);}

//
// SET_SUCCESS - sets the VDM caller's AX register to NERR_Success and clears
// the carry flag
//
#define SET_SUCCESS()                   {setAX(NERR_Success); setCF(0);}

//
// SET_OK - an explicit version of SET_SUCCESS wherein NERR_Success would be
// an inappropriate error, although the right value
//
#define SET_OK(value)                   {setAX(value); setCF(0);}



//
// Miscellaneous macros for working out sizes of things
//

//
// ARRAY_ELEMENTS - gives the number of elements of a particular type in an
// array
//

#define ARRAY_ELEMENTS(a)   (sizeof(a)/sizeof((a)[0]))

//
// LAST_ELEMENT - returns the index of the last element in array
//

#define LAST_ELEMENT(a)     (ARRAY_ELEMENTS(a)-1)

//
// BITSIN - returns the number of bits in a data type or structure. This is
// predicated upon the number of bits in a byte being 8 and all data types
// being composed of a collection of bytes (safe assumption?)
//
#define BITSIN(thing)                   (sizeof(thing) * BITS_IN_A_BYTE)

//
// Miscellaneous other macros
//

//
// IS_ASCII_PATH_SEPARATOR - returns TRUE if ch is / or \. ch is a single
// byte (ASCII) character
//
#define IS_ASCII_PATH_SEPARATOR(ch)     (((ch) == '/') || ((ch) == '\\'))

//
// macros for setting CF and ZF flags for return from hardware interrupt
// callback
//

#define SET_CALLBACK_NOTHING()  {setZF(0); setCF(0);}
#define SET_CALLBACK_NAMEPIPE() {setZF(0); setCF(1);}
#define SET_CALLBACK_DLC()      {setZF(1); setCF(0);}
#define SET_CALLBACK_NETBIOS()  {setZF(1); setCF(1);}

//
// DLC-specific macros etc.
//

extern LPVDM_REDIR_DOS_WINDOW   lpVdmWindow;

//
// setPostRoutine - if dw is not 0 then we write the (DOS segmented) address of
// the post routine into the dwPostRoutine field of the VDM_REDIR_DOS_WINDOW
// structure passed to us at redir DLC initialization. We also set the flags
// to indicate to the redir's hardware interrupt routine there is a DLC post
// routine to run. If dw is 0 then we set the flags to indicate that there is
// no post routine processing
//
#define setPostRoutine( dw )    if (dw) {\
                                    (lpVdmWindow->dwPostRoutine = (DWORD)(dw));\
                                    SET_CALLBACK_DLC();\
                                } else {\
                                    SET_CALLBACK_NOTHING();\
                                }

//
// VR_ASYNC_DISPOSITION - we maintain a serialized list of these structures.
// Used to dispose of VDM redir asynchronous completions in the order in which
// they occurred
//

typedef struct _VR_ASYNC_DISPOSITION {

    //
    // Next - maintains a singly-linked list of dispositions
    //

    struct _VR_ASYNC_DISPOSITION* Next;

    //
    // AsyncDispositionRoutine - pointer to VOID function taking no args which
    // will dispose of the next asynchronous completion - Netbios, named pipe
    // or DLC
    //

    VOID (*AsyncDispositionRoutine)(VOID);
} VR_ASYNC_DISPOSITION, *PVR_ASYNC_DISPOSITION;

//
// _inlinePointerFromWords - the POINTER_FROM_WORDS macro is inefficient if the
// arguments are calls to eg. getES(), getBX() - the calls are made twice if
// the pointer turns out to be non-zero. Use an inline function to achieve the
// same results, but only call function arguments once
//

#ifdef i386

__inline LPVOID _inlinePointerFromWords(WORD seg, WORD off) {

    WORD _seg = seg;
    WORD _off = off;

    return (_seg + _off) ? (LPVOID)GetVDMAddr(_seg, _off) : 0;
}

#else
LPVOID _inlinePointerFromWords(WORD seg, WORD off);
#endif

//
// CONVERT_ADDRESS - convert a segmented (real or protect-mode) address to a
// flat 32-bit address
//

//#define CONVERT_ADDRESS(seg, off, size, mode) !((WORD)(seg) | (WORD)(off)) ? 0 : Sim32GetVDMPointer((((DWORD)seg) << 16) + (DWORD)(off), (size), (mode))
#define CONVERT_ADDRESS(seg, off, size, mode) _inlineConvertAddress((WORD)(seg), (WORD)(off), (WORD)(size), (BOOLEAN)(mode))

#ifdef i386

__inline LPVOID _inlineConvertAddress(WORD Seg, WORD Off, WORD Size, BOOLEAN Pm) {

    WORD _seg = Seg;
    WORD _off = Off;

    return (_seg | _off) ? Sim32GetVDMPointer(((DWORD)_seg << 16) + _off, Size, Pm) : 0;
}

#else
extern LPVOID _inlineConvertAddress(WORD Seg, WORD Off, WORD Size, BOOLEAN Pm);
#endif

#endif  // _VDMREDIR_
