/***********************************************************************************/
/** MIT License **/
/** ----------- **/
/** **/  
/** Copyright (c) 2002-2019 Advanced Micro Devices, Inc. **/
/** **/
/** Permission is hereby granted, free of charge, to any person obtaining a copy **/
/** of this Software and associated documentaon files (the "Software"), to deal **/
/** in the Software without restriction, including without limitation the rights **/
/** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell **/
/** copies of the Software, and to permit persons to whom the Software is **/
/** furnished to do so, subject to the following conditions: **/
/** **/ 
/** The above copyright notice and this permission notice shall be included in **/
/** all copies or substantial portions of the Software. **/
/** **/
/** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR **/
/** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, **/
/** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE **/
/** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER **/
/** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, **/
/** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN **/
/** THE SOFTWARE. **/
/***********************************************************************************/

/*
******************************************************************************
 * Source File  : simd.h
 * Archive File : $Archive: $
 * Date		    : 6/04/01
 * Description  : The include file for  the AMD SIMD exception filter routine
 *                for  Microsoft Structured Exception Handling
 *		 
 *
$Revision:$
$Name:$
$Date:$
$Author:$
$History: simd.h $
 * 
 */

#include <emmintrin.h>

// simd.h
// This file contains structure definitions to provide
// convenient access to SIMD and MMX data as unsigned
// integer data.

// change the following define to a 1 to print terse output
#define DO_PRINT 0

// can't use the 3DNOW SDK as written with 64 bit tools
#if !defined (_AMD64_)
#define USE_3DNOW_SDK 1
#define SUPPORTS_FTZ 1 
#endif


/*****************************************************************/

// Basic type definitions

typedef UINT_PTR AWORD; //  x86-64 safe

typedef union
{
	float f;
    unsigned long long l;
} LFLOAT;

//typedef struct
//{
//	DWORD dw[2];
//} 
typedef unsigned _int64 QWORD;

typedef union
{
	double f;
    unsigned long long l[2];
} LDOUBLE;

typedef __declspec(align(16)) struct
{
	LFLOAT f0,f1,f2,f3;
} SSESINGLE;

typedef __declspec(align(16)) struct
{
	LDOUBLE d0,d1;
} SSEDOUBLE;


// this is the key data structure type used by the filter
// and the test program.  It will be aligned, since
// the __m128 types are all aligned.  It allows the
// use of one variable to carry all the needed data
// types.
typedef union
{
	__m128  m;
	__m128d md;
	__m128i mi;
	__m64   m64[2];
	DWORD	l[4];
	int		i[4];
	LFLOAT	f[4];
	QWORD	q[2];
	LDOUBLE d[2];
} ML128;

// this defined to provide a MMX type for the FXSTOR structure.
typedef union
{
	unsigned short mmx[4];	// mmx regs are 64 bits
	unsigned short fp[5];	// floating point regs are 80 bits
} MMX80;

/*****************************************************************/

// define  constants used by SIMD

// define MXCSR rounding control bits.
#define SDIMCW_RC   0x6000
#define SDIRC_NEAR  0x0000
#define SDIRC_DOWN  0x2000
#define SDIRC_UP    0x4000
#define SDIRC_CHOP  0x6000

// define other MXCSR control bits
#define SDDAZ   0x0040
#define SDFTZ   0x8000

#define opADD    0x58
#define opAND    0x54
#define opANDN   0x55
#define opCMP    0xC2
#define opCOMISS  0x2F
#define opCVTPI2PS    0x2A
#define opCVTTPS2PI   0x2C
#define opCVTPS2PI    0x2D
#define opCVTPS2PD    0x5A
#define opCVTDQ2PS    0x5B
#define opCVTTPD2DQ    0xE6
#define opDIV    0x5E
#define opMAX    0x5F
#define opMIN    0x5D
#define opMUL    0x59
#define opSQRT   0x51
#define opSUB    0x5C
#define opUCOMISS 0x2E

// define EFlags bits
#define ZF  (1 << 6)
#define PF  (1 << 2)
#define CF  (1 << 0)

// define the REX prefix bits
#define REX_PREFIX 0x40
#define REX_W      0x8
#define REX_R      0x4
#define REX_X      0x2
#define REX_B      0x1


// define the exception information record

// constants for the status bits
#define IEM_INEXACT    0x20
#define IEM_UNDERFLOW  0x10
#define IEM_OVERFLOW   0x08
#define IEM_ZERODIVIDE 0x04
#define IEM_DENORMAL   0x02
#define IEM_INVALID    0x01
#define IEM_MASK       0x3F

#define IMM_INEXACT    0x1000
#define IMM_UNDERFLOW  0x0800
#define IMM_OVERFLOW   0x0400
#define IMM_ZERODIVIDE 0x0200
#define IMM_DENORMAL   0x0100
#define IMM_INVALID    0x0080
#define IMM_MASK       0x1F80

/*****************************************************************/

// Instruction forms

// Type enumerations
//

typedef enum
{
    fGdWsd,
    fGdWss,
    fQqWpd,
    fQqWps,
    fVpdQq,
    fVpdWpd,
    fVpdWpdIb,
    fVpdWpdi,
    fVpdWps,
    fVpdiWpd,
    fVpdiWps,
    fVpsQq,
    fVpsWpd,
    fVpsWpdi,
    fVpsWps,
    fVpsWpsIb,
    fVsdEd,
    fVsdWsd,
    fVsdWsdIb,
    fVsdWss,
    fVssEd,
    fVssWsd,
    fVssWss,
    fVssWssIb
} InstType;

// operand types
typedef enum
{
    oEd,    //General register dword mod R/M
    oGd,    //General register dword
    oQq,    // MMX quadword mod R/M
    oVpd,   // XMM register
    oVpdi,
    oVps,
    oVsd,
    oVss,
    oWpd,   // XMM mod R/M
    oWpdi,
    oWps,
    oWsd,
    oWss
} OpType;

// operand class
typedef enum
{
    oXMMreg,
    oXMMmrm,
    oMMXreg,
    oMMXmrm,
    oGENreg,
    oGENmrm,
} OpClass;

// data types
typedef enum
{
    dDW,        // integer DWORD
    dPD,        // packed double precision
    dPDI,       // packed integer DWORD
    dPS,        // packed single precision
    dQ,         // integer quadword
    dSD,        // scalar double precision
    dSS         // scalar single precision
} DataType;

/*****************************************************************/

// Structure definitions
//


// define the format of the data used by
// the FXSAVE and FXRSTOR commands
typedef struct
{
    MMX80 mmx;              // the mmx/fp register
    unsigned short reserved[3]; // floating point regs are 80 bits
} FPMMX;

#if defined (_AMD64_)
// x86-64 version
typedef struct _FXMM_SAVE_AREA {
    WORD    ControlWord;
    WORD    StatusWord;
    WORD    TagWord;
    WORD    OpCode;
    QWORD   ErrorOffset;
    QWORD   DataOffset;
    DWORD   Mxcsr;
    DWORD   reserved3;
    FPMMX   FMMXreg[8];
    ML128   XMMreg[16];
} FXMM_SAVE_AREA;
#else
// 32 bit x86 version
typedef struct _FXMM_SAVE_AREA {
    WORD    ControlWord;
    WORD    StatusWord;
    WORD    TagWord;
    WORD    OpCode;
    DWORD   ErrorOffset;
    WORD    ErrorSelector;
    WORD    reserved1;
    DWORD   DataOffset;
    WORD    DataSelector;
    WORD    reserved2;
    DWORD   Mxcsr;
    DWORD   reserved3;
    FPMMX   FMMXreg[8];
    ML128   XMMreg[8];
} FXMM_SAVE_AREA;
#endif
typedef FXMM_SAVE_AREA *PFXMM_SAVE_AREA;

/* This structure is used to access the excepting opcode */
typedef struct {
    unsigned char opcode;
    unsigned char rmbyte;
    union {
        unsigned long long offset; // this will need work for x86-64
        unsigned char imm8;
    } data;
    
} SIMD_OP, *PSIMD_OP;

// Define a SIMD exception flag type.
// This is just like the _FPIEEE_EXCEPTION_FLAGS
// except that it adds the denormal field.
typedef struct {
    unsigned int Inexact : 1;
    unsigned int Underflow : 1;
    unsigned int Overflow : 1;
    unsigned int ZeroDivide : 1;
    unsigned int InvalidOperation : 1;
    unsigned int Denormal : 1;
} _SIMD_EXCEPTION_FLAGS;


/* define the local simd record structures */
typedef struct {
    unsigned int RoundingMode;
    _SIMD_EXCEPTION_FLAGS Cause;
    _SIMD_EXCEPTION_FLAGS Enable;
    _SIMD_EXCEPTION_FLAGS Status;
    PSIMD_OP opaddress;         // points to 0F xx opcode
    int curAddr;                // used when parsing mod R/M byte
    unsigned char prefix;
    unsigned char opcode;
    unsigned char rmbyte;
    unsigned char immediate8;
    // add a rex field for x86-64
    unsigned char rex;
    int eopcode;                // encoded opcode (index for tables)
    int op_form;
    int op1_class;              // XMM, MMX, or gen register
    int op1_type;               // data format 
    int op2_class;
    int op2_type;
    int is_commiss;
    int commiss_val;
    unsigned int mxcsr;         // value of mscsr from context record.
    ML128 op1_value;
    ML128 op2_value;
    ML128 *op2_ptr;

} _SIMD_RECORD, *_PSIMD_RECORD;

/* define a record for the operand form table */
typedef struct {
    int op1;   // form of operand 1
    int op2;   // form of operand 2
} _OPERAND_RECORD;

