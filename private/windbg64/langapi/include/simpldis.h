/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    simpldis.h

Abstract:

    Generic C language interface to PUMA disassembler.

    This is designed to expose the functionality of the PUMA disassembler
    via a simple C language API.  It is not intended to be as flexible or
    as efficient as using the DIS class interface.  It is intended to provide
    "least common denominator" access to the disassembler for C code which
    does not have stringent performance demands.  A minimum set of types is
    exposed, and they are named so as to minimize the chance of collisions.

Author:

    KentF 16-Jan-1996

--*/


#ifndef _SIMPLDIS_H
#define _SIMPLDIS_H

#ifdef __cplusplus
extern "C" {
#endif

enum _SIMPLE_ARCHITECTURE {
    Simple_Arch_X86,
    Simple_Arch_Mips,
    Simple_Arch_AlphaAxp,
    Simple_Arch_PowerPc,
    Simple_Arch_IA64
};

//
// enumeration for x86 registers.
// all of the other platforms currently supported
// use an intuitively obvious register numbering
// scheme - e.g. r0..r31 represented as 0..31
//

enum _SIMPLE_REGX86
{
   SimpleRegEax	= 0,
   SimpleRegEcx	= 1,
   SimpleRegEdx	= 2,
   SimpleRegEbx	= 3,
   SimpleRegEsp	= 4,
   SimpleRegEbp	= 5,
   SimpleRegEsi	= 6,
   SimpleRegEdi	= 7,
};

#define SD_STRINGMAX 100

//
// disassembly return structure.  this contains
// all of the data returned from the disassembler.
// it contains no input data, and need not be
// initialized by the caller.
//

typedef struct _SIMPLEDIS {

    DWORD   dwEA0;              // First effective address
    size_t  cbEA0;              // data size
    DWORD   dwEA1;              // Second effective address
    size_t  cbEA1;              // data size
    DWORD   dwEA2;              // Third effective address
    size_t  cbEA2;              // data size
    DWORD   cbMemref;           // size of data at EA0
    DWORD   dwBranchTarget;     // branch target, if IsBranch or IsCall
    DWORD   dwJumpTable;        // jump table for indirect jumps
    DWORD   cbJumpEntry;        // size of each jump table entry
    BOOL    IsCall;
    BOOL    IsBranch;
    BOOL    IsTrap;

    char    szAddress[SD_STRINGMAX];
    char    szRaw[SD_STRINGMAX];
    char    szOpcode[SD_STRINGMAX];
    char    szOperands[SD_STRINGMAX];
    char    szComment[SD_STRINGMAX];
    char    szEA0[SD_STRINGMAX];
    char    szEA1[SD_STRINGMAX];
    char    szEA2[SD_STRINGMAX];

} SIMPLEDIS, *PSIMPLEDIS;



   // Pfncchaddr() is the callback function for symbol lookup.
   // If the address is non-zero, the callback function is called during
   // CchFormatInstr to query the symbol for the supplied address.  If there
   // is no symbol at this address, the callback should return 0.

typedef
size_t (WINAPI *PFNCCHADDR) (
    PVOID pv,
    DWORD addr,
    PCHAR symbol,
    size_t symsize,
    DWORD *displacement
    );


   // Pfncchfixup() is the callback function for symbol lookup.
   // If the address is non-zero, the callback function is called during
   // CchFormatInstr to query the symbol and displacement referenced by
   // operands of the current instruction.  The callback should examine the
   // contents of the memory identified by the supplied address and size and
   // return the name of any symbol targeted by a fixup on this memory and the
   // displacement from that symbol.  If there is no fixup on the specified
   // memory, the callback should return 0.

typedef
size_t (WINAPI *PFNCCHFIXUP) (
    PVOID pv,
    DWORD ipaddr,
    DWORD addr,
    size_t opsize,
    PCHAR symbol,
    size_t symsize,
    DWORD *displacement
    );


typedef
size_t (WINAPI *PFNCCHREGREL) (
    PVOID pv,
    DWORD ipaddr,
    int reg,
    DWORD offset,
    PCHAR symbol,
    size_t symsize,
    DWORD *displacement
    );


typedef
DWORDLONG (WINAPI *PFNQWGETREG) (
    PVOID pv,
    int reg
    );


int
WINAPI
SimplyDisassemble(
    PBYTE           pb,
    const size_t    cbMax,
    const DWORD     Address,
    const int       Architecture,
    PSIMPLEDIS      Sdis,
    PFNCCHADDR      pfnCchAddr,
    PFNCCHFIXUP     pfnCchFixup,
    PFNCCHREGREL    pfnCchRegrel,
    PFNQWGETREG     pfnQwGetreg,
    const PVOID     pv
    );

// Call this when you are done using the disassembler. 
// This frees up any memory allocated by the disassembler.

void 
WINAPI
CleanupDisassembler(
	VOID
	);

#ifdef __cplusplus
}
#endif

#endif // _SIMPLDIS_H
