
/*++

Copyright (c) 1993  Digital Equipment Corporation

Module Name:

    optable.h

Abstract:

    Definitions for -
    Table of operations, their names and charactersitics
    Used by ntsd, windbg and acc's dissassembler

Author:

    Miche Baker-Harvey (mbh) 10-Jan-1993

Revision History:

--*/

#ifndef _OPTABLE_
#define _OPTABLE_

//
// Each entry in the opTable is either for a
// +  function - one option on a particular opcode
// +  terminal opcode - an opcode without a function field
//      the above two can both appear directly in disassembly
//
// +  non terminal opcode - an opcode with a function field:
//      these entries do not represent values which can be
//      executed directly:  they require a function entry.
//
// + invalid opcode - this is an opcode reserved to digital
//

typedef enum ENTRY_TYPE {

   INVALID_ETYPE,
   NON_TERMINAL_ETYPE,
   TERMINAL_ETYPE,
   FUNCTION_ETYPE,
   NOT_AN_ETYPE

} ENTRY_TYPE;


#define NO_FUNC   (ULONG)-1

typedef ULONG (* PFOPPARSE)();

typedef struct _OPTBLENTRY {

    union {

        struct {

            PUCHAR     _pszName;                // Name of the instruction
            PFOPPARSE  _parsFunc;               // Function to parse operands

        } s0;                  // functions and terminal opcodes

        struct {

            struct _OPTBLENTRY * _funcTable;    // Describes funcs for opcode
            ULONG            _funcTableSize;    // Number of possible funcs

        } s1;                  // non-terminal opcodes

    } u;

    //
    // These fields describe the instruction
    //

    ULONG opCode;       // Top 6 bits of a 32-bit alpha instr
    ULONG funcCode;     // Function; meaning is opcode dependent
    ULONG iType;        // type of the instr: branch, fp, mem...
                        // values are ALPHA_* in alphaops.h

    ENTRY_TYPE eType;   // type of this entry in the opTable

} OPTBLENTRY, * POPTBLENTRY;

//
// MBH - hack workaround:
// I tried to do this with nameless functions and structures;
// it works just fine on ALPHA, but dies on 386, so use this
// ugly hack instead.
// The name "pszAlphaName" is used instead of the more obvious
// "pszName" because other structures contain pszName.
//
#define pszAlphaName  u.s0._pszName
#define parsFunc      u.s0._parsFunc
#define funcTable     u.s1._funcTable
#define funcTableSize u.s1._funcTableSize

POPTBLENTRY findNonTerminalEntry(ULONG);
POPTBLENTRY findStringEntry(PUCHAR);
POPTBLENTRY findOpCodeEntry(ULONG);
char * findFuncName(POPTBLENTRY, ULONG);
char * findFlagName(ULONG, ULONG);
void printTable();
void opTableInit();

//
// This structure is used for the floating point flag names.
//

#define FPFLAGS_NOT_AN_ENTRY 0xffffffff

typedef struct _FPFLAGS {

    ULONG flags;        // the flags on the opcode
    PUCHAR flagname;    // the string mnemonic for the flags

} FPFLAGS, * PFPFLAGS;


#endif   // _OPTABLE_
