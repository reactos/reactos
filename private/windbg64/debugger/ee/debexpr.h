/***    debexpr.h - include file for expression evaluator
 *
 *      Constants, structures and function prototypes required by
 *      expression evaluator.
 *
 */

#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>

#include "types.h"
#include "cvinfo.h"
#include "cvtypes.h"
#include "shapi.h"
#include "odtypes.h"
#include "eeapi.h"
#include "debdef.h"
#include "shfunc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef REGREL32 *LPREGREL32;
typedef __int64             QUAD;
typedef unsigned __int64    UQUAD;

// define DASSERT macro
#if defined (DEBUGVER)
#include "cxassert.h"
#define DASSERT(ex)      assert(ex)
#define NOTTESTED(ex)
#else
#define DASSERT(ex)
#define NOTTESTED(ex)
#endif

// define the default string buffers for the API formatting routines

#define TYPESTRMAX     768      // EEGetTypeFromTM maximum string length
#define NAMESTRMAX     256      // EEGetNameFromTM maximum string length
#define FMTSTRMAX      256      // EEGetValueFromTM maximum string length
#define ERRSTRMAX      256      // EEGetError maximum string length
#define FCNSTRMAX      256      // maximum formatted function prototype
#define TMLISTCNT       20      // number of entries in TM list

#define MAXRETURN       1000    // maximum structure return from fcn call
#define ESTACK_DEFAULT  10      // default evaluation stack size
#define HSYML_SIZE      0x1000  // size of HSYM list buffer

#define BIND_fForceBind     0x01    // TRUE if bind is forced
#define BIND_fEnableProlog  0x02    // TRUE if prolog search enabled
#define BIND_fSupOvlOps     0x04    // TRUE if overloaded operators suppressed
#define BIND_fSupBase       0x08    // TRUE if base class searching suppressed

#define HSYM_MARKER     0x01    // in expr, indicates that an HSYM follows
#define HSYM_CODE_LEN   (2*sizeof(UINT_PTR))    // length of encoded HSYM

// Number of operators supported

enum {
#define OPCNT(name, val) name = val
#define OPCDAT(opc)
#define OPDAT(op, opfprec, opgprec, opclass, opbind, opeval)
#include "debops.h"
#undef OPDAT
#undef OPCDAT
#undef OPCNT
};

// Error message ordinals

#include "resource.h"
typedef int ERRNUM;

// Operator type.

typedef enum {
#define OPCNT(name, val)
#define OPCDAT(opc)
#define OPDAT(op, opfprec, opgprec, opclass, opbind, opeval) op,
#include "debops.h"
#undef OPDAT
#undef OPCDAT
#undef OPCNT

    OP_badtok  = 255
} op_t;


// Operator class.


typedef enum
{
#define OPCNT(name, val)
#define OPDAT(op, opfprec, opgprec, opclass, opbind, opeval)
#define OPCDAT(opc) opc,
#include "debops.h"
#undef OPCDAT
#undef OPDAT
#undef OPCNT
} opc_t;

// kinds of syntesized expressions
typedef enum {
    SE_totallynew,
    SE_array    ,
    SE_ptr      ,
    SE_member   ,
    SE_bclass   ,
    SE_method   ,
    SE_downcast ,
    SE_deref    ,
    SE_derefmember,
    SE_downcastmember
} SE_t;

// handy floating point conversion for float10's

#if (_MSC_VER >= 800) && defined(_M_IX86) && (_M_IX86 >= 300)

#pragma message("WARNING:floating point code not portable to non-x86 CPU's")

FLOAT10 __inline    Float10FromDouble ( double d ) {
    FLOAT10 f10Ret;
    __asm {
        fld     qword ptr d
        fstp    tbyte ptr f10Ret
        fwait
        }
    return f10Ret;
    }

#pragma warning(disable:4035)

double __inline     DoubleFromFloat10 ( FLOAT10 flt10 ) {
    double  d;
    __asm   fld     tbyte ptr flt10
    __asm   fstp    qword ptr d
    return d;
    }

float  __inline     FloatFromFloat10 ( FLOAT10 flt10 ) {
    float   f;
    __asm   fld     tbyte ptr flt10
    __asm   fstp    dword ptr f
    return f;
    }

int __inline        Float10LessThanEqual ( FLOAT10 f1, FLOAT10 f2 ) {
    __asm {
        fld     tbyte ptr f1
        fld     tbyte ptr f2
        fcompp
        fnstsw  ax
        fwait
        sahf
        mov     eax, 0      // can't use xor or sub eax,eax because of flags!!!
        setbe   al
        }
    // note: leaves result in eax!
    }

#pragma warning(default:4035)

long __inline       LongFromFloat10 ( FLOAT10 flt10 ) {
    long    lRet;
    __asm {
        fld     tbyte ptr flt10
        fistp   lRet
        fwait
        }
    return lRet;
    }

#else

// FLOAT10 is really a long double
FLOAT10 __inline    Float10FromDouble ( double d ) {
    FLOAT10 f10 = {0};
    DASSERT(FALSE);
    return f10;
    }

double __inline     DoubleFromFloat10 ( FLOAT10 flt10 ) {
    DASSERT(FALSE);
    return 0.0;
    }

float  __inline     FloatFromFloat10 ( FLOAT10 flt10 ) {
    DASSERT(FALSE);
    return 0.0f;
    }

int __inline        Float10LessThanEqual ( FLOAT10 f1, FLOAT10 f2 ) {
    DASSERT(FALSE);
    return 0;
    //return f1 <= f2;
    }

long __inline       LongFromFloat10 ( FLOAT10 flt10 ) {
    DASSERT(FALSE);
    return 0;
    //return (long) flt10;
    }

#endif

// missing ctype macros from tchar.h

#define _istcsymf(_c)   (_istalpha(_c) || ((_c) == '_'))
#define _istcsym(_c)    (_istalnum(_c) || ((_c) == '_'))


//  return enumeration from MatchType

typedef enum MTYP_t {
    MTYP_none,
    MTYP_exact,
    MTYP_inexact
} MTYP_t;

// Macros for determination of operator types.  Relies on
// implicit ordering of ops (see debops.h).


#define OP_IS_IDENT(op)     ((op) < OP_lparen)
#define OP_IS_GROUP(op)     (((op) == OP_lparen) || ((op) == OP_rparen))
#define OP_IS_UNARY(op)     (((op) >= OP_bang) && ((op) <= OP_context))
#define OP_IS_BINARY(op)    ((op) >= OP_function)


// Token Structure Definition
// This structure is built by the lexer and is used to hold information in
// the shift/reduce stack until the data can be transerred to the parse
// tree in debtree.c

// M00WARN: This structure MUST be kept in sync with that in DEBLEXER.ASM
//  value structure for constant nodes in parse tree or for all elements
//  in evaluation stack at evaluation time.


typedef union val_t {
    char        vchar;
    uchar       vuchar;
    short       vshort;
    ushort      vushort;
    long        vlong;
    ulong       vulong;
    QUAD        vquad;
    UQUAD       vuquad;
    float       vfloat;
    double      vdouble;
    FLOAT10     vldouble;
    ADDR        vptr;
} val_t;
typedef val_t  *pval_t;

typedef struct token_t {
    op_t        opTok;          // Token type: OP_ident, OP_dot, etc.
    char *pbTok;                // Pointer to start of actual token
    char *pbEnd;                // pointer to last character + 1 of token
    ulong       iTokStart;      // index of token start calculated from pbTok
    ulong        cbTok;         // Size of token (in bytes)
    CV_typ_t    typ;            // Type of constant token
    val_t   val;                // Value of constant token
} token_t;
typedef token_t *ptoken_t;

//  Macros to access values of a parse token

#define VAL_VAL(pv)     ((pv)->val)
#define VAL_CHAR(pv)    ((pv)->val.vchar)
#define VAL_UCHAR(pv)   ((pv)->val.vuchar)
#define VAL_SHORT(pv)   ((pv)->val.vshort)
#define VAL_USHORT(pv)  ((pv)->val.vushort)
#define VAL_LONG(pv)    ((pv)->val.vlong)
#define VAL_ULONG(pv)   ((pv)->val.vulong)
#define VAL_QUAD(pv)    ((pv)->val.vquad)
#define VAL_UQUAD(pv)   ((pv)->val.vuquad)
#define VAL_FLOAT(pv)   ((pv)->val.vfloat)
#define VAL_DOUBLE(pv)  ((pv)->val.vdouble)
#define VAL_LDOUBLE(pv) ((pv)->val.vldouble)



//------------------- Node Structure Definition -------------------

enum fcn_call {
    FCN_C = 1,
    FCN_PASCAL,
    FCN_FAST,
    FCN_PCODE,
    FCN_STD,
    FCN_THISCALL,
    FCN_MIPS,
    FCN_ALPHA,
    FCN_PPC,
    FCN_IA64
};

enum eval_op {
    EV_type = 1,        // node represents a type - no address or value
    EV_hsym,            // node represents a handle to symbol
    EV_constant,        // node represents a constant - no address
    EV_lvalue,          // node represents a value - has value
    EV_rvalue           // node represents an address
};




// The following bit field describes the contents of an evaluation node.
// This information is contained in a parse tree node after the bind phase
// or in elements on the evaluation stack during bind or evaluation
// If a bitfield is set, the the corresponding vdata_t union element will
// contain the additional information describing the data in the node.  For
// example, if isptr is set, then vbits.ptr.pIndex contains the type index of
// the object pointed to.

typedef union vbits_t {
    struct  {
        unsigned    ptrtype     :5; // type of pointer
        unsigned    isptr       :1; // true if node is a pointer
        unsigned    ispmember   :1; // true if node is a pointer to member
        unsigned    ispmethod   :1; // true if node is a pointer to method
        unsigned    isref       :1; // true if node contains a reference
        unsigned    isdptr      :1; // true if node is a data pointer
        unsigned    isaddr      :1; // true if node is an address
        unsigned    isdata      :1; // true if node references data
        unsigned    isflag32    :1; // true if 0:32 model
        unsigned    isreg       :1; // true if node references a register
        unsigned    isclass     :1; // true if node is a class
        unsigned    isenum      :1; // true if node is an enumeration
        unsigned    isbitf      :1; // true if node is a bitfield
        unsigned    isfcn       :1; // true if node is a function
        unsigned    isambiguous :1; // true if function is ambiguous
        unsigned    isarray     :1; // true if node is an arrary
        unsigned    isbprel     :1; // true if node is bp relative symbol
        unsigned    isregrel    :1; // true if register relative symbol
        unsigned    istlsrel    :1; // true if thread local storage symbol
        unsigned    ismember    :1; // true if node is member
        unsigned    isstmember  :1; // true if node is static member
        unsigned    isvptr      :1; // true if node is a vtable pointer
        unsigned    ismethod    :1; // true if node is a method
        unsigned    isstmethod  :1; // true if node is a static method
        unsigned    isvtshape   :1; // true if node is a virtual fcn shape table
        unsigned    isconst     :1; // true if constant
        unsigned    isvolatile  :1; // true if volatile
        unsigned    islabel     :1; // true if code label
        unsigned    access      :2; // access control if class member
        unsigned    iscurpc     :1;
        unsigned	isregia64   :1; // true if node references an IA64 register
        unsigned	isregrelia64:1; // true if node is IA64 register relative symbol
    } bits;
    unsigned long clr[2];
} vbits_t;

//  value type flags in identifier node in parse tree or all elements
//  in the evaluation stack at evaluation time

typedef union vdata_t {
    struct {
        CV_typ_t    utype;      // underlying type of pointer
        ulong       bseg;       // base segment
        ulong       symtype;    // symbol type of base
        ADDR        addr;       // address of base
        CV_typ_t    btype;      // type index of base on type
        CV_typ_t    stype;      // type index of based symbol
        CV_typ_t    pmclass;    // containing class for pointer to member
        ulong       pmenum;     // enumeration specifying pm format
        long        arraylen;   // length of array in bytes
        long        thisadjust; // adjustor for this pointer
        struct {
            ulong       ireg;       // System register values
            bool_t      hibyte;     // TRUE if register is high byte
        } reg;
    } ptr;

    struct {
        struct {
            ulong   fGlobType :1; // TRUE if bscope left member is global type
            ulong    fFollowsBnOp :1;  // TRUE if bscope left member is
                                // at the right of a bnOp (".", "->", "::")
            ulong    fNameSpace:1;   // this :: node was bound via namespace lookup
        } flags;
        ulong       cblen;      // number of bytes in class
        long        count;      // number of elements in class
        CV_typ_t    fList;      // type index of field list
        CV_typ_t    dList;      // type index of derivation list
        CV_typ_t    utype;      // underlying type if enum
        CV_typ_t    vshape;     // type index of vshape table for this class
        CV_prop_t   property;   // class property mask
    } classd;

    struct {
        struct {
            ulong    fGlobType :1; // TRUE if bscope left member is global type
        } flags;
        long        count;      // number of elements in class
        CV_typ_t    fList;      // type index of field list
        CV_typ_t    utype;      // underlying type if enum
        CV_prop_t   property;   // class property mask
    } enumd;

    struct {
        ulong       len;        // length of bitfield
        ulong       pos;        // position of bitfield
        CV_typ_t    type;       // type of bitfield
    } bit;

    struct {
        enum fcn_call call; // calling sequence indices
        struct {            // calling sequence flags
            ulong    farcall     :1;     // far call if true
            ulong    callerpop   :1;     // caller pop if true
            ulong    varargs     :1;     // variable arguments if true
            ulong    defargs     :1;     // default arguments if true
            ulong    notpresent  :1;     // not present if true
        } flags;
        CV_fldattr_t attr;      // attribute if method
        CV_off32_t  thisadjust; // logical this adjustor
        CV_typ_t    rvtype;     // type index of function return value
        long        cparam;     // number of parameters
        CV_typ_t    parmtype;   // type index of parameter list
        CV_typ_t    classtype;  // class type index if member function
        CV_typ_t    thistype;   // this type index if member function
        UOFFSET     vtabind;    // vtable index if virtual method
        CV_typ_t    vfptype;    // type index of vfuncptr if virtual method
    } fcn;

    struct {
        struct {
            ulong   fbase     :1; // true if direct base
            ulong   fvbase    :1; // true if virtual base
            ulong   fivbase   :1; // true if indirect virtual base
        } flags;
        CV_typ_t    type;       // type index of member
        CV_fldattr_t access;    // field attribute
        UOFFSET     offset;     // offset from containing class address point
        CV_typ_t    vbptr;      // virtual base pointer type
        UOFFSET     vbpoff;     // offset from this pointer to virtual base pointer
        ulong       vbind;      // virtual base index in vb pointer table
        CV_typ_t    thistype;   // type index of this pointer
        ulong       thisexpr;   // node to start this calculation if non-zero
    } member;

    struct {
        UOFFSET  offset;   // offset of member in class
    } vptr;

    struct {
        ulong   count;      // number of entries in shape table
    } vtshape;

    struct {
        long        count;      // number of overloads for function name
        CV_typ_t    type;       // type index of method list
    } method;
} vdata_t;

typedef struct vreg_t{
    ulong       ireg;       // System register values
    ulong       ireghi;     // High order register, in case
                            // the value spans over two registers
    bool_t      hibyte;     // TRUE if register is high byte
} vreg_t;



//  Evaluation element
//  This contains all of the information known about a constant or identifier
//  in the bound parse tree or all elements in the evaluation stack at bind or
//  evaluation time


typedef struct eval_t {
    ulong       state;          // evaluation state EV_lvalue, EV_rvalue, EV_constant
    CV_typ_t    type;           // type index
    ulong       iTokStart;      // offset of token in command string
    ulong        cbTok;          // length of token
    ADDR        addr;           // address of symbol
    HMOD        mod;            // handle to module
    CXT         CXTT;           // context where ID was found
    HSYM        hSym;           // handle to symbol structure
    HTYPE       typdef;         // handle to typdef structure
    vbits_t     flags;
    vdata_t     data;
    vreg_t      reg;
    ulong       regrel;         // REGREL index off of register
    ulong       vallen;         // length of value in bytes
    val_t       val;            // value of node
} eval_t;
typedef eval_t *peval_t;    // far pointer to evaluation element
typedef eval_t     *neval_t;    // near pointer to evaluation element

// The following macros are used to test and set values in an evaluation
// element.  pv is a far pointer to an evaluation element either in the
// syntax tree or on the evaluation stack.

#define CLEAR_EVAL(pv) memset(pv, 0, sizeof (eval_t))

#define EVAL_STATE(pv)          ((pv)->state)
#define EVAL_ITOK(pv)           ((pv)->iTokStart)
#define EVAL_CBTOK(pv)          ((pv)->cbTok)
#define EVAL_TYPDEF(pv)         ((pv)->typdef)
#define EVAL_SYM(pv)            ((pv)->addr)
#define EVAL_SYM_ADDR(pv)       ((pv)->addr.addr)
#define EVAL_SYM_SEG(pv)        ((pv)->addr.addr.seg)
#define EVAL_SYM_OFF(pv)        ((pv)->addr.addr.off)
#define EVAL_SYM_EMI(pv)        ((pv)->addr.emi)
#define EVAL_SYM_MODE(pv)       ((pv)->addr.mode)

#define EVAL_SYM_IS32(pv)       (ADDR_IS_OFF32(EVAL_SYM(pv)))
#define EVAL_SYM_ISFLAT(pv)     (ADDR_IS_FLAT(EVAL_SYM(pv)))
#define EVAL_HSYM(pv)           ((pv)->hSym)
#define EVAL_MOD(pv)            ((pv)->mod)
#define EVAL_CXTT(pv)           ((pv)->CXTT)
#define EVAL_VALLEN(pv)         ((pv)->vallen)


#define EVAL_TYP(pv)            ((pv)->type)
#define EVAL_FLAGS(pv)          ((pv)->flags)
#define EVAL_DATA(pv)           ((pv)->data)
#define EVAL_REGREL(pv)         ((pv)->regrel)
#define EVAL_VAL(pv)            ((pv)->val)
#define EVAL_PVAL(pv)           (&(EVAL_VAL(pv)))
#define EVAL_CHAR(pv)           ((pv)->val.vchar)
#define EVAL_UCHAR(pv)          ((pv)->val.vuchar)
#define EVAL_SHORT(pv)          ((pv)->val.vshort)
#define EVAL_USHORT(pv)         ((pv)->val.vushort)
#define EVAL_LONG(pv)           ((pv)->val.vlong)
#define EVAL_ULONG(pv)          ((pv)->val.vulong)
#define EVAL_QUAD(pv)           ((pv)->val.vquad)
#define EVAL_UQUAD(pv)          ((pv)->val.vuquad)
#define EVAL_FLOAT(pv)          ((pv)->val.vfloat)
#define EVAL_DOUBLE(pv)         ((pv)->val.vdouble)
#define EVAL_LDOUBLE(pv)        ((pv)->val.vldouble)
#define EVAL_PTR(pv)            ((pv)->val.vptr)
#define EVAL_PTR_ADDR(pv)       ((pv)->val.vptr.addr)
#define EVAL_PTR_OFF(pv)        ((pv)->val.vptr.addr.off)
#define EVAL_PTR_SEG(pv)        ((pv)->val.vptr.addr.seg)
#define EVAL_PTR_EMI(pv)        ((pv)->val.vptr.emi)
#define EVAL_PTR_MODE(pv)       ((pv)->val.vptr.mode)

//  Macros for accessing flag bits in node structure
//  This data is almost always from the type and typedef record of the node

#define CLEAR_EVAL_FLAGS(pv)    ((pv)->flags.clr[0] = (pv)->flags.clr[1] = 0L)
#define EVAL_IS_REG(pv)         ((pv)->flags.bits.isreg)
#define EVAL_IS_DATA(pv)        ((pv)->flags.bits.isdata)
#define EVAL_IS_CLASS(pv)       ((pv)->flags.bits.isclass)
#define EVAL_IS_ENUM(pv)        ((pv)->flags.bits.isenum)
#define EVAL_IS_ARRAY(pv)       ((pv)->flags.bits.isarray)
#define EVAL_IS_REF(pv)         ((pv)->flags.bits.isref)
#define EVAL_IS_BITF(pv)        ((pv)->flags.bits.isbitf)
#define EVAL_IS_ADDR(pv)        ((pv)->flags.bits.isaddr)
#define EVAL_IS_FCN(pv)         ((pv)->flags.bits.isfcn)
#define EVAL_IS_BPREL(pv)       ((pv)->flags.bits.isbprel)
#define EVAL_IS_REGREL(pv)      ((pv)->flags.bits.isregrel)
#define EVAL_IS_TLSREL(pv)      ((pv)->flags.bits.istlsrel)
#define EVAL_IS_STMEMBER(pv)    ((pv)->flags.bits.isstmember)
#define EVAL_IS_MEMBER(pv)      ((pv)->flags.bits.ismember)
#define EVAL_IS_METHOD(pv)      ((pv)->flags.bits.ismethod)
#define EVAL_IS_STMETHOD(pv)    ((pv)->flags.bits.isstmethod)
#define EVAL_IS_VPTR(pv)        ((pv)->flags.bits.isvptr)
#define EVAL_IS_AMBIGUOUS(pv)   ((pv)->flags.bits.isambiguous)
#define EVAL_IS_CONST(pv)       ((pv)->flags.bits.isconst)
#define EVAL_IS_VOLATILE(pv)    ((pv)->flags.bits.isvolatile)
#define EVAL_IS_LABEL(pv)       ((pv)->flags.bits.islabel)
#define EVAL_IS_VTSHAPE(pv)     ((pv)->flags.bits.isvtshape)

#define EVAL_IS_PTR(pv)         ((pv)->flags.bits.isptr)
#define EVAL_IS_PMEMBER(pv)     ((pv)->flags.bits.ispmember)
#define EVAL_IS_PMETHOD(pv)     ((pv)->flags.bits.ispmethod)
#define EVAL_IS_DPTR(pv)        ((pv)->flags.bits.isdptr)
#define EVAL_IS_BASED(pv)       (((pv)->flags.bits.ptrtype >= CV_PTR_BASE_SEG) && ((pv)->flags.bits.ptrtype <= CV_PTR_BASE_SELF))
#define EVAL_PTRTYPE(pv)        ((pv)->flags.bits.ptrtype)
#define EVAL_ACCESS(pv)         ((pv)->flags.bits.access)
#define EVAL_IS_NPTR(pv)        (((pv)->flags.bits.ptrtype==CV_PTR_NEAR)\
                                &&((pv)->flags.bits.isarray==FALSE)&&((pv)->flags.bits.isfcn==FALSE))
#define EVAL_IS_FPTR(pv)        (((pv)->flags.bits.ptrtype==CV_PTR_FAR) \
                                &&((pv)->flags.bits.isarray==FALSE)&&((pv)->flags.bits.isfcn==FALSE))
#define EVAL_IS_NPTR32(pv)      (((pv)->flags.bits.ptrtype==CV_PTR_NEAR32)\
                                &&((pv)->flags.bits.isarray==FALSE)&&((pv)->flags.bits.isfcn==FALSE))
#define EVAL_IS_FPTR32(pv)      (((pv)->flags.bits.ptrtype==CV_PTR_FAR32) \
                                &&((pv)->flags.bits.isarray==FALSE)&&((pv)->flags.bits.isfcn==FALSE))
#define EVAL_IS_PTR64(pv)      (((pv)->flags.bits.ptrtype==CV_PTR_64) \
                                &&((pv)->flags.bits.isarray==FALSE)&&((pv)->flags.bits.isfcn==FALSE))
#define EVAL_IS_HPTR(pv)        (((pv)->flags.bits.ptrtype==CV_PTR_HUGE)&&((pv)->flags.bits.isarray==FALSE)&&((pv)->flags.bits.isfcn==FALSE))
#define EVAL_IS_BSEG(pv)        ((pv)->flags.bits.ptrtype==CV_PTR_BASE_SEG)
#define EVAL_IS_BVAL(pv)        ((pv)->flags.bits.ptrtype==CV_PTR_BASE_VAL)
#define EVAL_IS_BSEGVAL(pv)     ((pv)->flags.bits.ptrtype==CV_PTR_BASE_SEGVAL)
#define EVAL_IS_BADDR(pv)       ((pv)->flags.bits.ptrtype==CV_PTR_BASE_ADDR)
#define EVAL_IS_BSEGADDR(pv)    ((pv)->flags.bits.ptrtype==CV_PTR_BASE_SEGADDR)
#define EVAL_IS_BTYPE(pv)       ((pv)->flags.bits.ptrtype==CV_PTR_BASE_TYPE)
#define EVAL_IS_BSELF(pv)       ((pv)->flags.bits.ptrtype==CV_PTR_BASE_SELF)
#define EVAL_IS_CURPC(pv)       ((pv)->flags.bits.iscurpc)
#define EVAL_IS_REGIA64(pv)     ((pv)->flags.bits.isregia64)
#define EVAL_IS_REGRELIA64(pv)  ((pv)->flags.bits.isregrelia64)

//  Macros for accessing flag specified data in node structure
//  This data is almost always from the type and typedef record of the node

#define EVAL_REG(pv)            ((pv)->reg.ireg)
#define EVAL_REGHI(pv)          ((pv)->reg.ireghi)
#define EVAL_IS_HIBYTE(pv)      ((pv)->reg.hibyte)

#define EVAL_IS_NFCN(pv)        (!((pv)->data.fcn.flags.farcall))
#define EVAL_IS_FFCN(pv)        (((pv)->data.fcn.flags.farcall))


#define PTR_UTYPE(pv)           (((pv)->data.ptr.utype))
#define PTR_BSEG(pv)            (((pv)->data.ptr.bseg))
#define PTR_BSYM(pv)            (((pv)->data.ptr.bSym))
#define PTR_BTYPE(pv)           (((pv)->data.ptr.btype))
#define PTR_STYPE(pv)           (((pv)->data.ptr.stype))
#define PTR_BSYMTYPE(pv)        (((pv)->data.ptr.symtype))
#define PTR_ADDR(pv)            (((pv)->data.ptr.addr))
#define PTR_THISADJUST(pv)      (((pv)->data.ptr.thisadjust))
#define PTR_PMCLASS(pv)         (((pv)->data.ptr.pmclass))
#define PTR_PMENUM(pv)          (((pv)->data.ptr.pmenum))
#define PTR_ARRAYLEN(pv)        (((pv)->data.ptr.arraylen))
#define PTR_REG_IREG(pv)        (((pv)->data.ptr.reg.ireg))
#define PTR_REG_HIBYTE(pv)    (((pv)->data.ptr.reg.hibyte))

#define CLASS_LEN(pv)           (((pv)->data.classd.cblen))
#define CLASS_COUNT(pv)         (((pv)->data.classd.count))
#define CLASS_FIELD(pv)         (((pv)->data.classd.fList))
#define CLASS_DERIVE(pv)        (((pv)->data.classd.dList))
#define CLASS_PROP(pv)          (((pv)->data.classd.property))
#define CLASS_UTYPE(pv)         (((pv)->data.classd.utype))
#define CLASS_VTSHAPE(pv)       (((pv)->data.classd.vshape))
#define CLASS_GLOBALTYPE(pv)    (((pv)->data.classd.flags.fGlobType))
#define CLASS_FOLLOWSBNOP(pv)   (((pv)->data.classd.flags.fFollowsBnOp))
#define CLASS_NAMESPACE(pv)     (((pv)->data.classd.flags.fNameSpace))

#define ENUM_COUNT(pv)          (((pv)->data.enumd.count))
#define ENUM_FIELD(pv)          (((pv)->data.enumd.fList))
#define ENUM_PROP(pv)           (((pv)->data.enumd.property))
#define ENUM_UTYPE(pv)          (((pv)->data.enumd.utype))
#define ENUM_GLOBALTYPE(pv)     (((pv)->data.enumd.flags.fGlobType))

#define BITF_LEN(pv)            (((pv)->data.bit.len))
#define BITF_POS(pv)            (((pv)->data.bit.pos))
#define BITF_UTYPE(pv)          (((pv)->data.bit.type))

#define FCN_CALL(pv)            (((pv)->data.fcn.call))
#define FCN_FARCALL(pv)         (((pv)->data.fcn.flags.farcall))
#define FCN_CALLERPOP(pv)       (((pv)->data.fcn.flags.callerpop))
#define FCN_VARARGS(pv)         (((pv)->data.fcn.flags.varargs))
#define FCN_DEFARGS(pv)         (((pv)->data.fcn.flags.defargs))
#define FCN_NOTPRESENT(pv)      (((pv)->data.fcn.flags.notpresent))
#define FCN_RETURN(pv)          (((pv)->data.fcn.rvtype))
#define FCN_PCOUNT(pv)          (((pv)->data.fcn.cparam))
#define FCN_PINDEX(pv)          (((pv)->data.fcn.parmtype))
#define FCN_CLASS(pv)           (((pv)->data.fcn.classtype))
#define FCN_THIS(pv)            (((pv)->data.fcn.thistype))
#define FCN_ATTR(pv)            (((pv)->data.fcn.attr))
#define FCN_ACCESS(pv)          (((pv)->data.fcn.attr.access))
#define FCN_PROPERTY(pv)        (((pv)->data.fcn.attr.mprop))
#define FCN_VTABIND(pv)         (((pv)->data.fcn.vtabind))
#define FCN_VFPTYPE(pv)         (((pv)->data.fcn.vfptype))
#define FCN_THISADJUST(pv)      (((pv)->data.fcn.thisadjust))

#define MEMBER_OFFSET(pv)       (((pv)->data.member.offset))
#define MEMBER_THISTYPE(pv)     (((pv)->data.member.thistype))
#define MEMBER_THISEXPR(pv)     (((pv)->data.member.thisexpr))
#define MEMBER_TYPE(pv)         (((pv)->data.member.type))
#define MEMBER_ACCESS(pv)       (((pv)->data.member.access))
#define MEMBER_BASE(pv)         (((pv)->data.member.flags.fbase))
#define MEMBER_VBASE(pv)        (((pv)->data.member.flags.fvbase))
#define MEMBER_IVBASE(pv)       (((pv)->data.member.flags.fivbase))
#define MEMBER_VBPTR(pv)        (((pv)->data.member.vbptr))
#define MEMBER_VBPOFF(pv)       (((pv)->data.member.vbpoff))
#define MEMBER_VBIND(pv)        (((pv)->data.member.vbind))

#define VTSHAPE_COUNT(pv)       (((pv)->data.vtshape.count))

//  Near versions of the above macros.  These macros can be used when it is
//  known that the evaluation node is in DS

#define N_CLEAR_EVAL(nv) _nmemset(nv, 0, sizeof (eval_t))

#define N_EVAL_STATE(nv)          ((nv)->state)
#define N_EVAL_ITOK(nv)           ((nv)->iTokStart)
#define N_EVAL_CBTOK(nv)          ((nv)->cbTok)
#define N_EVAL_TYPDEF(nv)         ((nv)->typdef)
#define N_EVAL_SYM(nv)            ((nv)->addr)
#define N_EVAL_SYM_ADDR(nv)       ((nv)->addr.addr)
#define N_EVAL_SYM_SEG(nv)        ((nv)->addr.addr.seg)
#define N_EVAL_SYM_OFF(nv)        ((nv)->addr.addr.off)
#define N_EVAL_SYM_EMI(nv)        ((nv)->addr.emi)
#define N_EVAL_SYM_MODE(nv)       ((nv)->addr.mode)
#define N_EVAL_HSYM(nv)           ((nv)->hSym)
#define N_EVAL_MOD(nv)            ((nv)->mod)
#define N_EVAL_CXTT(nv)           ((nv)->CXTT)
#define N_EVAL_VALLEN(nv)         ((nv)->vallen)


#define N_EVAL_TYP(nv)            ((nv)->type)
#define N_EVAL_FLAGS(nv)          ((nv)->flags)
#define N_EVAL_DATA(nv)           ((nv)->data)
#define N_EVAL_VAL(nv)            ((nv)->val)
#define N_EVAL_PVAL(nv)           (&(N_EVAL_VAL(nv)))
#define N_EVAL_CHAR(nv)           ((nv)->val.vchar)
#define N_EVAL_UCHAR(nv)          ((nv)->val.vuchar)
#define N_EVAL_SHORT(nv)          ((nv)->val.vshort)
#define N_EVAL_USHORT(nv)         ((nv)->val.vushort)
#define N_EVAL_LONG(nv)           ((nv)->val.vlong)
#define N_EVAL_ULONG(nv)          ((nv)->val.vulong)
#define N_EVAL_QUAD(nv)           ((nv)->val.vquad)
#define N_EVAL_UQUAD(nv)          ((nv)->val.vuquad)
#define N_EVAL_FLOAT(nv)          ((nv)->val.vfloat)
#define N_EVAL_DOUBLE(nv)         ((nv)->val.vdouble)
#define N_EVAL_LDOUBLE(nv)        ((nv)->val.vldouble)
#define N_EVAL_PTR(nv)            ((nv)->val.vptr)
#define N_EVAL_PTR_ADDR(nv)       ((nv)->val.vptr.addr)
#define N_EVAL_PTR_OFF(nv)        ((nv)->val.vptr.addr.off)
#define N_EVAL_PTR_SEG(nv)        ((nv)->val.vptr.addr.seg)
#define N_EVAL_PTR_EMI(nv)        ((nv)->val.vptr.emi)
#define N_EVAL_PTR_MODE(nv)       ((nv)->val.vptr.mode)

//  Macros for accessing flag bits in node structure
//  This data is almost always from the type and typedef record of the node

#define N_CLEAR_EVAL_FLAGS(nv)    ((nv)->flags.clr[0] = (nv)->flags.clr[1] = 0L)
#define N_EVAL_IS_REG(nv)         ((nv)->flags.bits.isreg)
#define N_EVAL_IS_DATA(nv)        ((nv)->flags.bits.isdata)
#define N_EVAL_IS_ENUM(nv)        ((nv)->flags.bits.isenum)
#define N_EVAL_IS_CLASS(nv)       ((nv)->flags.bits.isclass)
#define N_EVAL_IS_ARRAY(nv)       ((nv)->flags.bits.isarray)
#define N_EVAL_IS_REF(nv)         ((nv)->flags.bits.isref)
#define N_EVAL_IS_BITF(nv)        ((nv)->flags.bits.isbitf)
#define N_EVAL_IS_ADDR(nv)        ((nv)->flags.bits.isaddr)
#define N_EVAL_IS_FCN(nv)         ((nv)->flags.bits.isfcn)
#define N_EVAL_IS_BPREL(nv)       ((nv)->flags.bits.isbprel)
#define N_EVAL_IS_REGREL(nv)      ((nv)->flags.bits.isregrel)
#define N_EVAL_IS_TLSREL(nv)      ((nv)->flags.bits.istlsrel)
#define N_EVAL_IS_STMEMBER(nv)    ((nv)->flags.bits.isstmember)
#define N_EVAL_IS_MEMBER(nv)      ((nv)->flags.bits.ismember)
#define N_EVAL_IS_METHOD(nv)      ((nv)->flags.bits.ismethod)
#define N_EVAL_IS_STMETHOD(nv)    ((nv)->flags.bits.isstmethod)
#define N_EVAL_IS_VPTR(nv)        ((nv)->flags.bits.isvptr)
#define N_EVAL_IS_AMBIGUOUS(nv)   ((nv)->flags.bits.isambiguous)
#define N_EVAL_IS_CONST(nv)       ((nv)->flags.bits.isconst)
#define N_EVAL_IS_VOLATILE(nv)    ((nv)->flags.bits.isvolatile)
#define N_EVAL_IS_LABEL(nv)       ((nv)->flags.bits.islabel)
#define N_EVAL_IS_VTSHAPE(nv)     ((nv)->flags.bits.isvtshape)

#define N_EVAL_IS_PTR(nv)         ((nv)->flags.bits.isptr)
#define N_EVAL_IS_PMEMBER(nv)     ((nv)->flags.bits.ispmember)
#define N_EVAL_IS_PMETHOD(nv)     ((nv)->flags.bits.ispmethod)
#define N_EVAL_IS_DPTR(nv)        ((nv)->flags.bits.isdptr)
#define N_EVAL_IS_BASED(nv)       (((nv)->flags.bits.ptrtype >= CV_PTR_BASE_SEG) && ((nv)->flags.bits.ptrtype <= CV_PTR_BASE_SELF))
#define N_EVAL_PTRTYPE(nv)        ((nv)->flags.bits.ptrtype)
#define N_EVAL_ACCESS(nv)         ((nv)->flags.bits.access)
#define N_EVAL_IS_NPTR(nv)        (((nv)->flags.bits.ptrtype==CV_PTR_NEAR)\
                                  &&((nv)->flags.bits.isarray==FALSE)&&((nv)->flags.bits.isfcn==FALSE))
#define N_EVAL_IS_FPTR(nv)        (((nv)->flags.bits.ptrtype==CV_PTR_FAR) \
                                  &&((nv)->flags.bits.isarray==FALSE)&&((nv)->flags.bits.isfcn==FALSE))
#define N_EVAL_IS_NPTR32(nv)      (((nv)->flags.bits.ptrtype==CV_PTR_NEAR32)\
                                  &&((nv)->flags.bits.isarray==FALSE)&&((nv)->flags.bits.isfcn==FALSE))
#define N_EVAL_IS_FPTR32(nv)      (((nv)->flags.bits.ptrtype==CV_PTR_FAR32) \
                                  &&((nv)->flags.bits.isarray==FALSE)&&((nv)->flags.bits.isfcn==FALSE))
#define N_EVAL_IS_HPTR(nv)        (((nv)->flags.bits.ptrtype==CV_PTR_HUGE)&&((nv)->flags.bits.isarray==FALSE)&&((nv)->flags.bits.isfcn==FALSE))
#define N_EVAL_IS_BSEG(nv)        ((nv)->flags.bits.ptrtype==CV_PTR_BASE_SEG)
#define N_EVAL_IS_BVAL(nv)        ((nv)->flags.bits.ptrtype==CV_PTR_BASE_VAL)
#define N_EVAL_IS_BSEGVAL(nv)     ((nv)->flags.bits.ptrtype==CV_PTR_BASE_SEGVAL)
#define N_EVAL_IS_BADDR(nv)       ((nv)->flags.bits.ptrtype==CV_PTR_BASE_ADDR)
#define N_EVAL_IS_BSEGADDR(nv)    ((nv)->flags.bits.ptrtype==CV_PTR_BASE_SEGADDR)
#define N_EVAL_IS_BTYPE(nv)       ((nv)->flags.bits.ptrtype==CV_PTR_BASE_TYPE)
#define N_EVAL_IS_BSELF(nv)       ((nv)->flags.bits.ptrtype==CV_PTR_BASE_SELF)

//  Macros for accessing flag specified data in node structure
//  This data is almost always from the type and typedef record of the node

#define N_EVAL_REG(nv)            ((nv)->data.reg.ireg)
#define N_EVAL_IS_HIBYTE(nv)      ((nv)->data.reg.hibyte)

#define N_EVAL_IS_NFCN(nv)        (!((nv)->data.fcn.flags.farcall))
#define N_EVAL_IS_FFCN(nv)        (((nv)->data.fcn.flags.farcall))


#define N_PTR_UTYPE(nv)           (((nv)->data.ptr.utype))
#define N_PTR_BSEG(nv)            (((nv)->data.ptr.bseg))
#define N_PTR_BSYM(nv)            (((nv)->data.ptr.bSym))
#define N_PTR_BTYPE(nv)           (((nv)->data.ptr.btype))
#define N_PTR_STYPE(nv)           (((nv)->data.ptr.stype))
#define N_PTR_BSYMTYPE(nv)        (((nv)->data.ptr.symtype))
#define N_PTR_ADDR(nv)            (((nv)->data.ptr.addr))
#define N_PTR_THISADJUST(nv)      (((nv)->data.ptr.thisadjust))
#define N_PTR_PMCLASS(nv)         (((nv)->data.ptr.pmclass))
#define N_PTR_PMENUM(nv)          (((nv)->data.ptr.pmenum))
#define N_PTR_ARRAYLEN(nv)        (((nv)->data.ptr.arraylen))
#define N_PTR_REG_IREG(nv)        (((nv)->data.ptr.reg.ireg))
#define N_PTR_REG_HIBYTE(nv)      (((nv)->data.ptr.reg.hibyte))

#define N_CLASS_LEN(nv)           (((nv)->data.classd.cblen))
#define N_CLASS_COUNT(nv)         (((nv)->data.classd.count))
#define N_CLASS_FIELD(nv)         (((nv)->data.classd.fList))
#define N_CLASS_DERIVE(nv)        (((nv)->data.classd.dList))
#define N_CLASS_PROP(nv)          (((nv)->data.classd.property))
#define N_CLASS_UTYPE(nv)         (((nv)->data.classd.utype))
#define N_CLASS_VTSHAPE(nv)       (((nv)->data.classd.vshape))
#define N_CLASS_GLOBALTYPE(nv)    (((nv)->data.classd.flags.fGlobType))
#define N_CLASS_FOLLOWSBNOP(pv)   (((nv)->data.classd.flags.fFollowsBnOp))

#define N_ENUM_COUNT(nv)          (((nv)->data.enumd.count))
#define N_ENUM_FIELD(nv)          (((nv)->data.enumd.fList))
#define N_ENUM_PROP(nv)           (((nv)->data.enumd.property))
#define N_ENUM_UTYPE(nv)          (((nv)->data.enumd.utype))
#define N_ENUM_GLOBALTYPE(nv)     (((nv)->data.enumd.flags.fGlobType))


#define N_BITF_LEN(nv)            (((nv)->data.bit.len))
#define N_BITF_POS(nv)            (((nv)->data.bit.pos))
#define N_BITF_UTYPE(nv)          (((nv)->data.bit.type))

#define N_FCN_CALL(nv)            (((nv)->data.fcn.call))
#define N_FCN_FARCALL(nv)         (((nv)->data.fcn.flags.farcall))
#define N_FCN_CALLERPOP(nv)       (((nv)->data.fcn.flags.callerpop))
#define N_FCN_VARARGS(nv)         (((nv)->data.fcn.flags.varargs))
#define N_FCN_DEFARGS(nv)         (((nv)->data.fcn.flags.defargs))
#define N_FCN_NOTPRESENT(nv)      (((nv)->data.fcn.flags.notpresent))
#define N_FCN_RETURN(nv)          (((nv)->data.fcn.rvtype))
#define N_FCN_PCOUNT(nv)          (((nv)->data.fcn.cparam))
#define N_FCN_PINDEX(nv)          (((nv)->data.fcn.parmtype))
#define N_FCN_CLASS(nv)           (((nv)->data.fcn.classtype))
#define N_FCN_THIS(nv)            (((nv)->data.fcn.thistype))
#define N_FCN_ATTR(nv)            (((nv)->data.fcn.attr))
#define N_FCN_ACCESS(nv)          (((nv)->data.fcn.attr.access))
#define N_FCN_PROPERTY(nv)        (((nv)->data.fcn.attr.mprop))
#define N_FCN_VTABIND(nv)         (((nv)->data.fcn.attr.vtabind))
#define N_FCN_VFPTYPE(nv)         (((nv)->data.fcn.vfptype))
#define N_FCN_THISADJUST(nv)      (((nv)->data.fcn.thisadjust))

#define N_MEMBER_OFFSET(nv)       (((nv)->data.member.offset))
#define N_MEMBER_THISTYPE(nv)     (((nv)->data.member.thistype))
#define N_MEMBER_THISEXPR(nv)     (((nv)->data.member.thisexpr))
#define N_MEMBER_TYPE(nv)         (((nv)->data.member.type))
#define N_MEMBER_ACCESS(nv)       (((nv)->data.member.access))
#define N_MEMBER_BASE(nv)         (((nv)->data.member.flags.fbase))
#define N_MEMBER_VBASE(nv)        (((nv)->data.member.flags.fvbase))
#define N_MEMBER_IVBASE(nv)       (((nv)->data.member.flags.fivbase))
#define N_MEMBER_VBPTR(nv)        (((nv)->data.member.vbptr))
#define N_MEMBER_VBPOFF(nv)       (((nv)->data.member.vbpoff))
#define N_MEMBER_VBIND(nv)        (((nv)->data.member.vbind))

#define N_VTSHAPE_COUNT(pv)       (((nv)->data.vtshape.count))


//  pointers to evaluation stack

extern HDEP     hEStack;        // handle of evaluation stack if not zero
extern struct elem_t *pEStack;  // pointer to evaluation stack
typedef struct elem_t _based(pEStack) *belem_t; // based pointer to stack element
typedef struct elem_t *pelem_t;    // far pointer to evaluation stack element

extern belem_t  StackLen;       // length is evaluation stack buffer
extern belem_t  StackMax;       // maximum length reached by evaluation stack
extern belem_t  StackOffset;    // offset into evaluation stack for next element
extern belem_t  StackCkPoint;   // checkpointed evaluation stack offset
extern bool_t   Evaluating;     // TRUE if evaluating rather than binding
extern char    *pExStr;         // pointer to current expression string
extern bool_t   BindingBP;      // true if binding breakpoint expression
extern CV_typ_t ClassExp;       // current explicit class
extern CV_typ_t ClassImp;       // implicit class (set if current context is method)
extern long     ClassThisAdjust;// implicit class this adjustor
extern char    *vfuncptr;
extern char    *vbaseptr;
extern HTM     *pTMList;        // breakpoint return list
extern PTML     pTMLbp;         // global pointer to TML for breakpoint
extern HDEP     hBPatch;        // handle to back patch data during BP bind
extern ulong    iBPatch;        // index into pTMLbp for backpatch data
extern bool_t   GettingChild;   // true if in EEGetChildTM
extern BOOL     fAutoClassCast; // true if auto class cast is enabled

typedef struct elem_t {
    belem_t     pe;             // based pointer to previous evalation element
    eval_t      se;             // evaluation element
} elem_t;


extern peval_t  ST;             // pointer to evaluation stack top
extern peval_t  STP;            // pointer to evaluation stack previous
extern PCXT     pCxt;           // pointer to current cxt for bind
extern PCXT     pBindBPCxt;     // pointer to Bp Binding context


//  stree_t - abstract syntax tree
//  The abstract syntax tree is a variable sized buffer allocated as
//          struct stree_t
//          variable space for tree nodes (initially NODE_DEFAULT bytes)
//          NSTACK_MAX indices to open terms in the parse tree
//  unused node space and the NSTACK_MAX indices are deallocated at the
//  end of the parse to maintain minimum memory usage.


#define NODE_DEFAULT    (10 *sizeof (node_t) + 5 * sizeof (eval_t))
#define NSTACK_MAX      30

typedef struct stree_t {
    ulong       size;           // allocated size of syntax tree
    ulong       stack_base;     // offset into buffer for parse stack
    ulong       stack_next;     // index of next parse stack entry
    ulong       node_next;      // offset of space for next node
    ulong       start_node;     // offset of node to start evaluation
    size_t      StackSize;      // size required for evaluation stack
    char        nodebase[];
} stree_t;

typedef stree_t *pstree_t;
extern pstree_t pTree;      // locked address of abstract or evaluation tree

typedef struct node_t _based (pTree) *bnode_t;
extern bnode_t  bArgList;       // based pointer to argument list
extern bnode_t  bnCxt;          // based pointer to node containing {...} context

typedef struct node_t {
    op_t        op;             // Operator, OP_...
    CV_typ_t    stype;          // type to set stack eval to (OP_addrof, ...)
    bnode_t     pnLeft;         // offset of left child
    bnode_t     pnRight;        // offset of right child or 0 if unary op
    eval_t      v[];            // extra data if ident or constant
} node_t;
typedef struct node_t *pnode_t;


typedef struct adjust_t {
    CV_typ_t    btype;          // type index of base
    CV_typ_t    vbptr;          // type index of virtual base table pointer
    UOFFSET     vbpoff;         // displacement from address point to vbptr
    SOFFSET     disp;           // displacement from *vbptr to base displacement
                                // use OFFSET instead of UOFFSET as this
                                // value can be negated by CastBaseToDeriv
} adjust_t;
typedef struct adjust_t *padjust_t;

typedef enum {
    OM_none,                    // initialization state
    OM_exact,                   // exact match - no conversion required
    OM_implicit,                // implicit conversion required
    OM_vararg                   // matches on vararg
} ometric;


//     Structure for argument data including overload calculations
//     There is one of these structures allocated for each OP_arg node

typedef struct argd_t {
    CV_typ_t    type;
    CV_typ_t    actual;         // actual type at argument bind time
    UOFFSET     SPoff;
    struct      {
        unsigned    isreg   :1;     // true if register parameter
        unsigned    ref     :1;     // true if reference parameter
        unsigned    utclass :1;     // true if referenced type is a class
        unsigned    istype  :1;     // true if arg node is EV_type
        unsigned    isconst :1;     // true if arg type is const
        unsigned    isvolatile :1;  // true if arg type is volatile
    } flags;
    unsigned    reg;            // register handles
    CV_typ_t    utype;          // underlying type of reference
    unsigned    vallen;
    ometric     best;           // best overload metric for this argument
    ometric     current;        // current overload metric for this argument
} argd_t;
typedef struct argd_t *pargd_t;



// Macros for accessing Node structure.

#define CLEAR_NODE(pn)      memset(pn, 0, sizeof (node_t))

#define NODE_OP(pn)         ((pn)->op)
#define NODE_STYPE(pn)      ((pn)->stype)
#define NODE_LCHILD(pn)     ((pn)->pnLeft)
#define NODE_RCHILD(pn)     ((pn)->pnRight)


typedef struct GCC_state_t {
    // static state information to optimize search in GetClassiChild
    // by continuing the search for child i+1 from the point where
    // the search for child i ended.
    // Search identification
    HTM         hTMIn;
    long        ordinal;
    // Last State to be restored
    HTYPE       hField;
    uint        fSkip;
} GCC_state_t;


//  expression evaluator state structure definition
//  This is the structure that totally describes an expression and is the
//  TM referred to in the API documentation.

typedef struct exstate_t {
    struct {
        ulong   childtm     :1; // TM represents a child
        ulong   noname      :1; // TM does not have a name
        ulong   parse_ok    :1; // true if expression parsed without error
        ulong   bind_ok     :1; // true if expression is bound without error
        ulong   eval_ok     :1; // true if expression evaluated without error
        ulong   cChildren_ok:1; // true if cChildren count is valid
        ulong   bprel       :1; // true if expression contains BP relative terms
        ulong   regrel      :1; // true if expression contains register relative term
                        // bprel implies regrel
        ulong   nullcontext :1; // true if expression contains {} context
        ulong   fCase       :1; // true if case insensitive search
        ulong   fEProlog    :1; // true if function symbols during prolog
        ulong   fFunction   :1; // true if bind tree contains function call
        ulong   fLData      :1; // true if bind tree references local data
        ulong   fGData      :1; // true if bind tree references global data
        ulong   fSupOvlOps  :1; // true if overloaded operator search suppressed
        ulong   fSupBase    :1; // true if base class searches suppressed
        ulong   f32bit      :1; // true if ints and pointer offsets are 32-bit
        ulong   tlsrel      :1; // true if expression is TLS
        ulong   fNotPresent :1; // true if expr contains a missing data item
        ulong   autoexpand  :1; // TM represents an auto-expand child
        ulong   fAErebind   :1; // TMLAutoExpand list needs rebinding
        ulong   fAEreeval   :1; // TMLAutoExpand list needs reevaluating
} state;
    uint        ExLen;      // expression length
    uint        ExLenSav;   // saved expression length
    uint        style;      // display formatting style
    ulong       err_num;    // error number
    ulong       strIndex;   // index into string at end of parse
    ulong       strIndexSav;// index into saved expression at the end of parse
    uint        radix;      // radix for expression
    HDEP        hCName;     // handle to child name if derived TM
    HDEP        hDClassName;// handle to derived class name if TM points
                            //  to an enclosed base class object
    HDEP        hExStr;     // handle to zero terminated expression string
    HDEP        hExStrSav;  // handle to saved string for preserving template names
    HDEP        hErrStr;    // handle to optional str to insert into err msg
    HDEP        hSTree;     // handle of abstract syntax tree
    HDEP        hETree;     // handle of tree in evaluation
// members used for evaluation caching
    SE_t        seTemplate; // synthesized expression template
    uint        nRefCount;  // reference counter
    HTM         hParentTM;  // handle to parent TM
    ULONG       timeStamp;  // evaluation timestamp
    eval_t      evalThisSav;// saved "this" node
// end of members used for eval caching
    TML         TMLAutoExpand; // Auto-expanded ChildTM list
    HDEP        hAutoExpandRule; // Rule used for auto expansion
    long        cChildren;  // childTM count
    GCC_state_t searchState; // GetClassiChild last saved search state
    bnode_t     ambiguous;  // based pointer to breakpoint ambiguous node
    CXT         cxt;        // bind context
    HFRAME      hframe;      // evaluation frame
    eval_t      result;     // evaluation result
} exstate_t;

typedef exstate_t *pexstate_t;
extern pexstate_t pExState;    // global pointer to current evaluation state structure


// global handle to the CXT list buffer

extern HCXTL   hCxtl;       // handle for context list buffer during GetCXTL
extern PCXTL   pCxtl;       // pointer to context list buffer during GetCXTL
extern ulong   mCxtl;       // maximum number of elements in context list

//  Structure describing the current state of a function match

typedef struct {
    CV_typ_t    match;          // type index of a matching function
    CV_fldattr_t attr;          // attributes of matched method
    UOFFSET     vtabind;        // vftable index if virtual method
    ulong       exact;          // number of exact matches on argument type
    ulong       implicit;       // number of implicit matches on argument type
    ulong       varargs;        // ordinal of the first argument matching as vararg
    HSYM        hSym;           // handle of best matching function
    CXT         CXTT;           // CXT for best matching function
} argcounters;


// structure describing base classes traversed in feature search

typedef struct  symbase_t {
    eval_t      Base;           // value node for base
    CV_fldattr_t attrBC;        // attribute of base class
    CV_typ_t    tTypeDef;       // typedef type if found (member/method overrides)
    bool_t      fVirtual;       // true of virtual base
    SOFFSET      thisadjust;    // this pointer adjustment from address point
                                // or offset of displacement from vbase table
    ulong       clsmask;        // used to flag introvirtual search
    CV_typ_t    vbptr;          // type of virtual base pointer
    UOFFSET     vbpoff;         // offset of virtual base pointer from address point
    ulong       vbind;          // index into virtal base table
} symbase_t;

typedef symbase_t *psymbase_t;

// structure for accumulating dominated and searched virtual bases

#define DOMBASE_DEFAULT    30

typedef struct dombase_t {
    ulong       CurIndex;
    ulong       MaxIndex;
    CV_typ_t    dom[];
} dombase_t;

typedef struct dombase_t *pdombase_t;


//  symclass_t - structure to describe symbol searching in a class
//  The symclass_t is a variable sized buffer allocated as
//          struct symclass_t
//          variable array of symbase_t (initially SYMBASE_DEFAULT entries)


#define SYMBASE_DEFAULT    10

typedef struct symclass_t {
    HDEP        hNext;          // handle of next found class element
    long        CurIndex;       // index of current symbase_t element
    long        MaxIndex;       // maximum symbase_t element
    CXT         CXTT;           // context of found symbol
    struct  {
        bool_t      viable;     // path is viable (set false if dominated)
        bool_t      isbase;     // found feature is base if true
        bool_t      isvbase;    // found feature is virtual base if true
        bool_t      isivbase;   // found feature is indirect virtual base if true
        bool_t      isdupvbase; // found feature is duplicate virtual base if true
    };
    eval_t      evalP;          // value node for feature found on this path
    UOFFSET     offset;         // offset of member from final base
    CV_fldattr_t access;        // access flags of member
    address_t   addr;
    CV_typ_t    vfpType;        // type index of vfuncptr if method
    ulong       possibles;      // count of possible features
    CV_typ_t    mlist;          // type index of method list if method
    CV_typ_t    vbptr;          // type of virtual base pointer
    UOFFSET     vbpoff;         // offset of virtual base pointer from address point
    ulong       vbind;          // index into virtal base table
    symbase_t   symbase[];
} symclass_t;

typedef symclass_t *psymclass_t;



//  Internal state of SearchSym

typedef enum {
    SYM_init,
    SYM_bclass,
    SYM_lexical,
    SYM_function,
    SYM_class,
    SYM_module,
    SYM_global,
    SYM_exe,
    SYM_public
} symstate_t;



//  Structure to describe search name to fnCmp

typedef struct  search_t {

    // input to search routine
    SSTR        sstr;
    ulong       initializer;    // enum of routine that initialized search
    ulong       lastsym;        // type of symbol last checked by pfnCmp
    int         flags;          // compare flags
    symstate_t  state;          // internal state of symbol search
    ulong       scope;          // mask describing scopes to be searched
    ulong       clsmask;        // mask describing grouping to search in class
    CV_typ_t    typeIn;         // input type if type search (i.e. casting)
    CXT         CXTT;           // initial context (modified during search)
    bnode_t     bnOp;           // based pointer to operator node if right search
    bnode_t     bn;             // based pointer to node (used for tree rewrite)
    peval_t     pv;             // pointer to value to receive symbol info
    PFNCMP      pfnCmp;         // pointer to symbol compare routine
    HSYM        hSym;           // handle to symbol if found in symbol table
    ulong       cFound;         // count of handles to found features
    HDEP        hFound;         // handle of path to first found feature
    HDEP        hAmbCl;         // handle of ambiguous found feature list
    HEXE        hExe;           // handle to exe of starting context
    HMOD        hMod;           // handle to module of starting context
    HMOD        hModCur;        // handle to module of current context
    CV_typ_t    ExpClass;       // initial class if explicit class reference
    CV_typ_t    CurClass;       // class currently being searched
    ulong       possibles;      // number of possible matches
    CV_typ_t    FcnBP;          // function index of another function with
                                // same name but different index if searching
                                // for a procedure
    // output from search routine

    CV_typ_t    typeOut;        // type index of found element
    UOFFSET     offset;         // symbol address point offset if member
    UOFFSET     thisoff;        // this pointer offset from original address point
#ifdef NEVER
    address_t   address;        // address of symbol
#endif
    ADDR        addr;           // address of symbol
    argcounters best;
    ulong       overloadCount;  // overload counter used by arg matching functions --gdp 9/20/92
    CV_fldattr_t access;        // access flags if class member
} search_t;

typedef search_t * psearch_t;



// search flags describing implicit and explicit class searching

#define CLS_vfunc       0x0001          // search for vfunc pointer
#define CLS_vbase       0x0002          // search for vbase pointer
#define CLS_member      0x0004          // search for members
#define CLS_method      0x0008          // search for methods
#define CLS_frmethod    0x0010          // search for friend method
#define CLS_bclass      0x0020          // search for base classes
#define CLS_fclass      0x0040          // search for friend classes
#define CLS_enumerate   0x0080          // search for enumerates
#define CLS_ntype       0x0100          // search for nested types
#define CLS_virtintro   0x0200          // search for introducing virtual

#define CLS_data (CLS_member | CLS_vfunc | CLS_vbase | CLS_bclass)
#define CLS_defn (CLS_member | CLS_vfunc | CLS_vbase | CLS_bclass | CLS_method | CLS_enumerate | CLS_ntype)




//  Enumeration of return values from GetArgType.  This value specifies what
//  type of argument matching is permitted for a function

typedef enum {
    FARG_error,             // error in argument
    FARG_none,              // no arguments allowed
    FARG_vararg,            // variable argument list
    FARG_exact,             // exact argument list is required
} farg_t;


//  Bit values describing the permitted scope for symbol searching

#define SCP_lexical     0x0001  // lexical scope (formals, automatics & local statics)
#define SCP_class       0x0002  // class scope if member function
#define SCP_module      0x0004  // module scope
#define SCP_global      0x0008  // global scope
#define SCP_nomatchfn   0x0010  // don't call matchfunction (avoid recursion)

#define SCP_all (SCP_lexical | SCP_module | SCP_class | SCP_global)


//  Enumeration of return values from SearchSym


typedef enum {
    HR_error,               // error in search - do not continue
    HR_notfound,            // symbol not found
    HR_found,               // symbol found
    HR_rewrite,             // symbol found but tree rewrite required
    HR_ambiguous,           // symbol found but ambiguous
    HR_end                  // end of search
} HR_t;

//  Enumeration of return values from AEParseRule

typedef enum {
    AE_error,
    AE_childTM,
    AE_downcasttype
} AE_t;



//  Global functions

//  debapi.c

EESTATUS ParseBind (EEHSTR, HTM, PHTM, ulong  *, uint, SHFLAG);

//  deblex.c

ulong  GetDBToken (uchar *, ptoken_t, uint, op_t);
ulong  GetEscapedChar (char * *, ulong  *, ushort *);
ulong  ParseConst (uchar *, ptoken_t, uint);

//  debparse.c

EESTATUS DoParse (PHTM, uint, bool_t, bool_t, ulong  *);
EESTATUS Parse (const char *, uint, SHFLAG, SHFLAG, PHTM, ulong  *);
AE_t AEParseRule(LSZ);

//  debbind.c

EESTATUS DoBind (PHTM, PCXT, uint);
void InitSearchRight (bnode_t, bnode_t, psearch_t, ulong );
void InitSearchSym (bnode_t, peval_t, psearch_t, CV_typ_t, ulong , ulong );

//  debeval.c

bool_t DoEval (PHTM, HFRAME, EEDSP);


//  debsup.c

bool_t AreTypesEqual (HTM, HTM);
ulong  cChildrenTM (PHTM, long *, PSHFLAG);
ulong  cSynthChildTM(PHTM, long *);
ulong  cParamTM (HTM, ulong  *, PSHFLAG);
EESTATUS DereferenceTM (HTM, PEEHSTR, PEEHSTR);
ulong  DupTM (PHTM, PHTM);
EESTATUS GetChildTM (HTM, long, PHTM, ulong  *, EERADIX, SHFLAG);
bool_t GetDerivClassName(peval_t, BOOL, char *, uint);
bool_t GetVirtFuncName(PHTM, PCXF, char *, uint);
EESTATUS GetDerivedTM (HTM, LSZ, HTM *);
EESTATUS GetParmTM (HTM, ulong, PEEHSTR, PEEHSTR);
EESTATUS GetSymName (PHTM, PEEHSTR);
BOOL GetNameFromHSYM(char *, HSYM);
EESTATUS InfoFromTM (PHTM, PRI, PHTI);
ulong  IsExpandablePtr (peval_t);
bool_t LoadVal (peval_t);
bool_t ResolveAddr(peval_t);
bnode_t GetParentSubtree (bnode_t, SE_t);
void LinkWithParentTM (HTM, HTM);
EESTATUS GetMemberIA(PHTM, PHBCIA, ulong );

#define REG_DS  CV_REG_DS
#define REG_SS  CV_REG_SS
#define REG_CS  CV_REG_CS
SEGMENT GetSegmentRegister(HFRAME, int);

//  debsym.c

void InitSearchAErule (psearch_t, peval_t, CV_typ_t, HMOD);
void InitSearchBase (bnode_t, CV_typ_t, CV_typ_t, psearch_t, peval_t);
#ifdef NEVER
void InitSearchtDef (psearch_t, peval_t, ulong );
#endif
bool_t fValidInProlog (HSYM, bool_t);
farg_t GetArgType (peval_t, int, CV_typ_t *);
bool_t getDefnFromDecl(CV_typ_t, peval_t, CV_typ_t*);
CV_ptrtype_e SetAmbiant (bool_t);
EESTATUS GetHSYMList (HDEP *, PCXT, ulong , uchar *, SHFLAG);
HSYM SearchCFlag (void);
HR_t SearchBasePtrBase (psearch_t);
HR_t SearchSym (psearch_t);
bool_t pvThisFromST (bnode_t);

//  debtree.c

ulong  PushToken (ptoken_t);
bool_t      FASTCALL    GrowTree (uint);
bool_t      FASTCALL    GrowETree (uint);

//  debsrch.c

bool_t FindPtrToPtr (peval_t, CV_typ_t *);
bool_t LoadSymVal (peval_t pn);
bool_t LoadLValue (peval_t);
MTYP_t MatchType (peval_t, bool_t);
void ProtoPtr (peval_t, peval_t, bool_t, CV_modifier_t);

//  debtyper.c

bool_t CastNode (peval_t, CV_typ_t, CV_typ_t);
bool_t DeNormalizePtr (peval_t, peval_t);
bool_t NormalizeBase (peval_t);
void TypeNodes (op_t, peval_t, peval_t);
bool_t ValidateNodes (op_t, peval_t, peval_t);

// debfmt.c

ulong  FormatCXT (PCXT, PEEHSTR, DWORD);
void FormatFormal (HSYM, CXF *, ulong , char **, long  *);
EESTATUS FormatNode (PHTM phTM, uint Radix, PEEFORMAT pFormat, PEEHSTR phszValue, SHFLAG fAutoExp);
void FormatType (peval_t, char **, uint *, char **, ulong, PHDR_TYPE);

// deberr.c

ulong  GetErrorText (PHTM, EESTATUS, PEEHSTR);
void ErrUnknownSymbol (LPSSTR);


// debutil.c

bool_t  csCmp (psearch_t, SYMPTR, char const *, uint);
bool_t  fnCmp (psearch_t, SYMPTR, char const *, uint);
#ifdef NEVER
bool_t  tdCmp (psearch_t, SYMPTR, char const *, uint);
#endif
bool_t  PopStack (void);
bool_t  PushStack (peval_t);
void    CkPointStack (void);
bool_t  fCanSubtractPtrs (peval_t, peval_t);
char *  GetHSYMCodeFromHSYM(HSYM);
HSYM    GetHSYMFromHSYMCode(char *);
ULONG   GetTickCounter (void);
LSZ             GetTypeName (CV_typ_t, HMOD);
void    IncrTickCounter (void);
bool_t  ResetStack (void);
void    ResetTickCounter (void);
void    RemoveIndir (peval_t);
UQUAD   RNumLeaf (void *, uint *);
bool_t  SetNodeType (peval_t, CV_typ_t);
bool_t  TMLAddHTM (PTML, HTM);
long    TypeDefSize (peval_t);
long    TypeSize (peval_t);
int             TypeSizePrim (CV_typ_t);
bool_t  UpdateMem (peval_t);
UINT    GetDebuggeeBytes (ADDR, UINT, void *, CV_typ_t);
UINT    PutDebuggeeBytes (ADDR, UINT, void *, CV_typ_t);
CV_typ_t GetUdtDefnTindex (CV_typ_t TypeIn, neval_t nv, char *lpStr);


// debwalk.c

ulong   DoGetCXTL (PHTM, PHCXTL);
EESTATUS GetExpr (EERADIX, PEEHSTR, ulong  *);

// dllmain.c
int     LoadEEMsg (uint, char *, int);
bool_t  LoadAERule (LSZ, LSZ, uint);

INLINE unsigned char
SkipPad(
    unsigned char *pb
    )
{
    if (*pb >= LF_PAD0) {
        // there is a pad field
        return(*pb & 0x0f);
    }

    return(0);
}

// helper routines for 64 bit ints
INLINE bool_t
fIsQuadType (
    CV_typ_t typ
    )
{
    return (typ == T_QUAD || typ == T_UQUAD ||
            typ == T_INT8 || typ == T_INT8);
}

INLINE ulong
DWordHi(
    UQUAD UQuad
    )
{
    return (ulong) (UQuad >> 32);
}

INLINE ulong
DWordLo(
    UQUAD UQuad
    )
{
    return (ulong) UQuad;
}

// helper routines for method property flags
INLINE BOOL
fIntroducingVirtual(
    unsigned mprop
    )
{
    return mprop == CV_MTintro || mprop == CV_MTpureintro;
}

INLINE BOOL
fNonIntroVirtual(
    unsigned mprop
    )
{
    return mprop == CV_MTvirtual ||mprop == CV_MTpurevirt;
}

INLINE BOOL
fAnyVirtual(
    unsigned mprop
    )
{
    return fIntroducingVirtual(mprop) || fNonIntroVirtual(mprop);
}

/*
 * Temporary workaround until this function is provided
 * by the kernel.
 */

enum _ftargetosenums {
    fTargetOS2 = 0x0001,
    fTargetWin3,
    fTargetDos16,
    fTargetDos32,
    fTargetCruiser,
    fTargetMac,
    fTargetMacPPC,
    fTargetWin32s
};

typedef struct _DTI {
    DWORD dwTargetOS;
} DTI;

typedef DTI *LPDTI;

extern uint uSkipLfOneMethod(plfOneMethod plf);
extern char *pbNameInLfOneMethod(plfOneMethod plf);
extern MPT TargetMachine;

#ifdef __cplusplus
} // extern "C" {
#endif
