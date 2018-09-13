/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    genxx.h

Abstract:

    This file contains macros (some of them destined for the M4 preprocessor)
    to aid in the generation of ks & hal header files.  This is used by
    ke\xxx\genxxx.c, as well as sdktools\genxx.

Author:

    Forrest C. Foltz (forrestf) 23-Jan-1998

Revision History:

--*/



//
// Structure element definitions.  
//

#define MAX_ELEMENT_NAME_LEN 127    // big enough for comments too
typedef struct _STRUC_ELEMENT {

//
// Flags is one or more SEF_xxx, defined below.
//

    UINT64 Flags;

//
// Note that Equate is used to store a pointer in the case of bitfield
// processing.
//

    UINT64 Equate;

//
// Name should be quite long, as it is used to hold comments as well.
//

    CHAR Name[ MAX_ELEMENT_NAME_LEN + 1 ];
} STRUC_ELEMENT, *PSTRUC_ELEMENT;

#define SEF_ENABLE_MASK     0x0000FF00      
#define SEF_HAL             0x00000100
#define SEF_KERNEL          0x00000200

#define SEF_INC_FORMAT_MASK 0x00010000
#define SEF_H_FORMAT        0x00000000
#define SEF_INC_FORMAT      0x00010000

//
// Types.  Note that SETMASK, CLRMASK has no effect on te BITFLD types.  BITFLD
// types have SEF_HAL | SEF_KERNEL set in the type.
//

#define SEF_TYPE_MASK       0x000000FF
#define SEF_EQUATE          0x00000000
#define SEF_EQUATE64        0x00000001
#define SEF_COMMENT         0x00000002      
#define SEF_STRING          0x00000003      // Equate is vararg to printf
#define SEF_BITFLD          0x00000004
#define SEF_BITALIAS        0x00000005
#define SEF_STRUCTURE       0x00000006
#define SEF_SETMASK         0x00000010      // Equate is the mask
#define SEF_CLRMASK         0x00000011      // Equate is the mask
#define SEF_END             0x00000012
#define SEF_START           0x00000013
#define SEF_PATH            0x00000014

//
// Note that BITFLD entries have per-entry hal|kernel flags
//


//
// Define architecture specific generation macros.
//

#define SEF_FLAGS 0
#define HAL SEF_HAL
#define KERNEL SEF_KERNEL

#ifndef ULONG_MAX
#define ULONG_MAX 0xFFFFFFFF
#endif

#ifdef _WIN64_
#define SEF_UINT SEF_EQUATE64
#else
#define SEF_UINT SEF_EQUATE
#endif

//
// genDef(Pc, KPCR, MinorVersion)
//
// -> #define PcMinorVersion 0x0
//

#define genDef(Prefix, Type, Member) \
    { SEF_EQUATE, OFFSET(Type, Member), #Prefix #Member },

//
// genAlt( PbAlignmentFixupCount, KPRCB, KeAlignmentFixupCount )
//
// -> #define PbAlignmentFixupCount 0x2f4
// 

#define genAlt(Name, Type, Member) \
    { SEF_EQUATE, OFFSET(Type, Member), #Name },

//
// genCom("This is a comment")
//
//    //
// -> // This is a comment
//    //
//

#define genCom(Comment) \
    { SEF_COMMENT, 0, Comment },

//
// genNam(PCR_MINOR_VERSION)
//
// -> #define PCR_MINOR_VERSION 0x1
//

#define genNam(Name) \
    { SEF_EQUATE, (ULONG)(Name), #Name },

//
// genNamUint(KSEG0_BASE)
//
// -> #define KSE0_BASE 0xffffffff80000000
//

#define genNamUint(Name) \
    { SEF_UINT, (UINT64)(Name), #Name },

//
// genVal(FirmwareFrameLength, FIRMWARE_FRAME_LENGTH)
//
// -> #define FirmwareFrameLength 0x250
//
// Note: if the value is 64-bit when _WIN64_ is enabled, use genValUint()
//

#define genVal(Name, Value) \
    { SEF_EQUATE, (ULONG)(Value), #Name },

//
// genValUint(KiPcr, KIPCR)
//
// -> #define KiPcr 0xe0000000ffffe000
//

#define genValUint(Name, Value) \
    { SEF_UINT, (UINT64)(Value), #Name },

//
// genSpc()
//
// ->
//

#define genSpc() \
    { SEF_STRING, 0, "\n" },

//
// genStr("    PCR equ ds:[0%lXH]\n", KIP0PCRADDRESS)
//
// ->     PCR equ ds:[0FFDFF000H]
//

#define genStr(String, Value) \
    { SEF_STRING, (ULONG_PTR)(Value), String },

//
// genTxt("ifdef NT_UP\n")
//
// -> ifdef NT_UP
//

#define genTxt(String) \
    { SEF_STRING, 0, String },

#define DisableInc( x ) \
    { SEF_CLRMASK, x, "" },

#define EnableInc( x ) \
    { SEF_SETMASK, x, "" },

#define MARKER_STRING "This is the genxx marker string."

//
// Source file can specify the _NTDRIVE\_NTROOT - relative output path.
// 'f' is the set of enable-flags that should be routed to this file.
// Use '0' if there is only a single output file.
//
// 'f' should also contain one of SEF_H_FORMAT or SEF_INC_FORMAT to
// indicate whether the generated file is in 'header file' or 'include file'
// format.
//

#define setPath( p, f ) \
    { SEF_PATH | f, 0, p },

//
// START_LIST defines the first element in ElementList.  This element contains
// a (possibly truncated) pointer to the ElementList array.  This is used to
// determine the fixup RA bias.
//

#define START_LIST \
    { SEF_START, (ULONG_PTR)ElementList, MARKER_STRING },

#define END_LIST \
    { SEF_END, 0, "" }

//
// Preprocessor assertion.  Do something here to make the compiler generate
// an error if x != y.
//

#define ASSERT_SAME( x, y )

//
// Macro to round Val up to the next Bnd boundary.  Bnd must be an integral
// power of two.
//

#define ROUND_UP( Val, Bnd ) \
    (((Val) + ((Bnd) - 1)) & ~((Bnd) - 1))

#ifndef OFFSET

//
// Define member offset computation macro.
//

#define OFFSET(type, field) ((ULONG_PTR)(&((type *)0)->field))

#endif

//
// Following are some M4 macros to help with bitfields.  
//

#ifndef SKIP_M4

//
// First, define the makezeros(n) macro that will generate a string with
// n pairs of ',0'.  This is a recursively defined macro.
//

define(`makezeros',`ifelse(eval($1),0,,`0,makezeros(eval($1-1))')')

//
// Define a concatenation macro.
//

define(`cat',`$1$2')

//
// The following example bitfield declaration uses HARDWARE_PTE as an
// example, which is declared (for alpha) as follows:
//
// typedef struct _HARDWARE_PTE {
//     ULONG Valid: 1;
//     ULONG Owner: 1;
//     ULONG Dirty: 1;
//     ULONG reserved: 1;
//     ULONG Global: 1;
//     ULONG GranularityHint: 2;
//     ULONG Write: 1;
//     ULONG CopyOnWrite: 1;
//     ULONG PageFrameNumber: 23;
// } HARDWARE_PTE, *PHARDWARE_PTE;
//
//
// // First, startBitStruc() is invoked with the structure name.
//
// startBitStruc( HARDWARE_PTE, SEF_HAL | SEF_KERNEL )
//
// //
// // Now, suppose we wanted to expose seven of the fields in an assembly
// // include file:
// //
//
// genBitField( Valid, PTE_VALID )
// genBitField( Owner, PTE_OWNER )
// genBitField( Dirty, PTE_DIRTY )
// genBitField( reserved )
// genBitField( Global, PTE_GLOBAL )
// genBitField( GranularityHint )
// genBitField( Write, PTE_WRITE )
// genBitField( CopyOnWrite, PTE_COPYONWRITE )
// genBitField( PageFrameNumber, PTE_PFN )
//
// Note that fields that are not used (in this case 'reserved' and
// 'GranularityHint') must still appear in the list.
//
// The above will generate a bunch of static, initialized copies of HARDWARE_PTE
// like so:
//
// HARDWARE_PTE HARDWARE_PTE_Valid = {
//     0xFFFFFFFF };
//
// HARDWARE_PTE HARDWARE_PTE_Owner = {
//     0,   // Valid
//     0xFFFFFFFF };
//
// HARDWARE_PTE HARDWARE_PTE_Dirty = {
//     0,   // Valid
//     0,   // Owner
//     0xFFFFFFFF };
//
// HARDWARE_PTE HARDWARE_PTE_Global = {
//     0,   // Valid
//     0,   // Owner
//     0,   // Dirty
//     0,   // reserved
//     0xFFFFFFFF };
//
// HARDWARE_PTE HARDWARE_PTE_Write = {
//     0,   // Valid
//     0,   // Owner
//     0,   // Dirty
//     0,   // reserved (skipped)
//     0,   // Global
//     0xFFFFFFFF };
//
// HARDWARE_PTE HARDWARE_PTE_CopyOnWrite = {
//     0,   // Valid
//     0,   // Owner
//     0,   // Dirty
//     0,   // reserved (skipped)
//     0,   // Global
//     0,   // GranularityHint (skipped)
//     0xFFFFFFFF };
//
// HARDWARE_PTE HARDWARE_PTE_PageFrameNumber = {
//     0,   // Valid
//     0,   // Owner
//     0,   // Dirty
//     0,   // reserved (skipped)
//     0,   // Global
//     0,   // GranularityHint (skipped)
//     0,   // CopyOnWrite
//     0xFFFFFFFF };
//
// Then, as part of processing the END_LIST macro, these structures are
// generated:
//
// { SEF_BITFLD, &HARDWARE_PTE_Valid,           "PTE_VALID" },
// { SEF_BITFLD, &HARDWARE_PTE_Owner,           "PTE_OWNER" },
// { SEF_BITFLD, &HARDWARE_PTE_Dirty,           "PTE_DIRTY" },
// { SEF_BITFLD, &HARDWARE_PTE_Global,          "PTE_GLOBAL" },
// { SEF_BITFLD, &HARDWARE_PTE_Write,           "PTE_WRITE" },
// { SEF_BITFLD, &HARDWARE_PTE_CopyOnWrite,     "PTE_COPYONWRITE" },
// { SEF_BITFLD, &HARDWARE_PTE_PageFrameNumber, "PTE_PFN" },
// { SEF_END,    0,                             "" }
//
//
// ... and that's what gets compiled by the target compiler into the .obj.
// Now, the final stage: genxx.exe is run against this target .obj, and
// would generate the following:
//
// #define PTE_VALID_MASK 0x1
// #define PTE_VALID 0x0
// #define PTE_OWNER_MASK 0x2
// #define PTE_OWNER 0x1
// #define PTE_DIRTY_MASK 0x4
// #define PTE_DIRTY 0x2
// #define PTE_GLOBAL_MASK 0x10
// #define PTE_GLOBAL 0x4
// #define PTE_WRITE_MASK 0x80
// #define PTE_WRITE 0x7
// #define PTE_COPYONWRITE_MASK 0x100
// #define PTE_COPYONWRITE 0x8
// #define PTE_PFN_MASK 0xfffffe00
// #define PTE_PFN 0x9
//

//
// BITFIELD_STRUCS accumulates array element initializations.  END_LIST will
// dump these into the definition array.
// 

define(`BITFIELD_STRUCS',`')

//
// startBitStruc( <strucname>, <whichfile> )
// sets BIT_STRUC_NAME = <strucname> and resets the ZERO_FIELDS count to 0.
// It also sets the WHICH_FILE macro.
//

define(`startBitStruc', `define(`BIT_STRUC_NAME',`$1')
                         define(`BITFIELD_STRUCS',
                                 BITFIELD_STRUCS
                                 )
                         define(`ZERO_FIELDS',0)
                         define(`SEF_TYPE',$2)
                        ')

//
// genBitField( <fldname>, <generatedname> ) declares a structure of type
// <strucname> and initializes the <fldname> bitfield within it.
//
// Note that I used "cma" instead of an actual comma, this gets changed to
// a comma by END_LIST, below.  If I were more proficient with M4 I would know
// how to get around this.
//

define(`genBitField', `define(`VAR_NAME', cat(cat(BIT_STRUC_NAME,`_'),$1))
                      `#'define `def_'VAR_NAME
                      BIT_STRUC_NAME VAR_NAME = {'
                      `makezeros(ZERO_FIELDS)'
                      `(ULONG_PTR)-1 };'
                      `define(`PAD_VAR_NAME', cat(cat(BIT_STRUC_NAME,`p'),$1))'
                      `ULONG_PTR PAD_VAR_NAME = 0;'
                      `define(`ZERO_FIELDS',incr(ZERO_FIELDS))'
                      `define(`FIELD_NAME', $1)'
                      `define(`FIELD_ASMNAME', $2)'
                      `define(`BITFIELD_STRUCS',
                               BITFIELD_STRUCS
                               `#i'fdef `def_'VAR_NAME
                               `#i'fndef `dec_'VAR_NAME
                               `#de'fine `dec_'VAR_NAME
                               { SEF_BITFLD | SEF_TYPE cma (ULONG_PTR)&VAR_NAME cma "FIELD_ASMNAME" } cma
                               `#e'ndif
                               `#e'ndif
                               )'
                      )

define(`genBitAlias', `define(`BITFIELD_STRUCS',
                               BITFIELD_STRUCS
                               `#i'fdef `def_'VAR_NAME
                               `#i'fndef `deca_'VAR_NAME
                               `#de'fine `deca_'VAR_NAME
                               { SEF_BITALIAS | SEF_TYPE cma 0 cma "$1" } cma
                               `#e'ndif
                               `#e'ndif
                               )'
                    )

//
// END_LIST dumps the array initializers accumulated by BITFIELD_STRUCS, after
// replacing each 'cma' with an actual comma.
//

define(`DUMP_BITFIELDS',`define(`cma',`,') BITFIELD_STRUCS')

#endif  // SKIP_M4

