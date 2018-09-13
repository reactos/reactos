/***    cvinfo.h - Generic CodeView information definitions
 *
 *      Structures, constants, etc. for accessing and interpreting
 *      CodeView information.
 *
 */


/***    The master copy of this file resides in the langapi project.
 *      All Microsoft projects are required to use the master copy without
 *      modification.  Modification of the master version or a copy
 *      without consultation with all parties concerned is extremely
 *      risky.
 *
 *      When this file is modified, the corresponding documentation file
 *      omfdeb.doc in the langapi project must be updated.
 */

#ifndef _VC_VER_INC
#include "..\include\vcver.h"
#endif

#ifndef _CV_INFO_INCLUDED
#define _CV_INFO_INCLUDED
#pragma message("--- Using 32-bit types --- (" __FILE__ ")" )

#ifdef  __cplusplus
#pragma warning ( disable: 4200 )
#endif

#ifndef __INLINE
#ifdef  __cplusplus
#define __INLINE inline
#else
#define __INLINE __inline
#endif
#endif

#pragma pack ( push, 1 )
typedef unsigned long   CV_uoff32_t;
typedef          long   CV_off32_t;
typedef unsigned short  CV_uoff16_t;
typedef          short  CV_off16_t;
typedef unsigned short  CV_typ16_t;
typedef unsigned long   CV_typ_t;
typedef unsigned short  _2BYTEPAD;

#if !defined (CV_ZEROLEN)
#define CV_ZEROLEN
#endif

#if !defined (FLOAT10)
#if defined(_M_I86)                    // 16 bit x86 supporting long double
typedef long double FLOAT10;
#else                                  // 32 bit w/o long double support
typedef struct FLOAT10
{
    char b[10];
} FLOAT10;
#endif
#endif


#define CV_SIGNATURE_C6         0L  // Actual signature is >64K
#define CV_SIGNATURE_C7         1L  // First explicit signature
#define CV_SIGNATURE_C11        2L  // C11 (vc5.x) 32-bit types
#if CC_LAZYSYMS
#define CV_SIGNATURE_LAZY       3L  // used only in PDB: see obj file for actual symbols
#define CV_SIGNATURE_RESERVED   4L  // All signatures from 4 to 64K are reserved
#else
#define CV_SIGNATURE_RESERVED   3L  // All signatures from 3 to 64K are reserved
#endif

#define CV_MAXOFFSET   0xffffffff

/**     CodeView Symbol and Type OMF type information is broken up into two
 *      ranges.  Type indices less than 0x1000 describe type information
 *      that is frequently used.  Type indices above 0x1000 are used to
 *      describe more complex features such as functions, arrays and
 *      structures.
 */




/**     Primitive types have predefined meaning that is encoded in the
 *      values of the various bit fields in the value.
 *
 *      A CodeView primitive type is defined as:
 *
 *      1 1
 *      1 089  7654  3  210
 *      r mode type  r  sub
 *
 *      Where
 *          mode is the pointer mode
 *          type is a type indicator
 *          sub  is a subtype enumeration
 *          r    is a reserved field
 *
 *      See Microsoft Symbol and Type OMF (Version 4.0) for more
 *      information.
 */


#define CV_MMASK        0x700       // mode mask
#define CV_TMASK        0x0f0       // type mask

// can we use the reserved bit ??
#define CV_SMASK        0x00f       // subtype mask

#define CV_MSHIFT       8           // primitive mode right shift count
#define CV_TSHIFT       4           // primitive type right shift count
#define CV_SSHIFT       0           // primitive subtype right shift count

// macros to extract primitive mode, type and size

#define CV_MODE(typ)    (((typ) & CV_MMASK) >> CV_MSHIFT)
#define CV_TYPE(typ)    (((typ) & CV_TMASK) >> CV_TSHIFT)
#define CV_SUBT(typ)    (((typ) & CV_SMASK) >> CV_SSHIFT)

// macros to insert new primitive mode, type and size

#define CV_NEWMODE(typ, nm)     ((CV_typ_t)(((typ) & ~CV_MMASK) | ((nm) << CV_MSHIFT)))
#define CV_NEWTYPE(typ, nt)     (((typ) & ~CV_TMASK) | ((nt) << CV_TSHIFT))
#define CV_NEWSUBT(typ, ns)     (((typ) & ~CV_SMASK) | ((ns) << CV_SSHIFT))



//     pointer mode enumeration values

typedef enum CV_prmode_e {
    CV_TM_DIRECT = 0,       // mode is not a pointer
    CV_TM_NPTR   = 1,       // mode is a near pointer
    CV_TM_FPTR   = 2,       // mode is a far pointer
    CV_TM_HPTR   = 3,       // mode is a huge pointer
    CV_TM_NPTR32 = 4,       // mode is a 32 bit near pointer
    CV_TM_FPTR32 = 5,       // mode is a 32 bit far pointer
    CV_TM_NPTR64 = 6,       // mode is a 64 bit near pointer
    CV_TM_NPTR128 = 7       // mode is a 128 bit near pointer
} CV_prmode_e;




//      type enumeration values


typedef enum CV_type_e {
    CV_SPECIAL      = 0x00,         // special type size values
    CV_SIGNED       = 0x01,         // signed integral size values
    CV_UNSIGNED     = 0x02,         // unsigned integral size values
    CV_BOOLEAN      = 0x03,         // Boolean size values
    CV_REAL         = 0x04,         // real number size values
    CV_COMPLEX      = 0x05,         // complex number size values
    CV_SPECIAL2     = 0x06,         // second set of special types
    CV_INT          = 0x07,         // integral (int) values
    CV_CVRESERVED   = 0x0f
} CV_type_e;




//      subtype enumeration values for CV_SPECIAL


typedef enum CV_special_e {
    CV_SP_NOTYPE    = 0x00,
    CV_SP_ABS       = 0x01,
    CV_SP_SEGMENT   = 0x02,
    CV_SP_VOID      = 0x03,
    CV_SP_CURRENCY  = 0x04,
    CV_SP_NBASICSTR = 0x05,
    CV_SP_FBASICSTR = 0x06,
    CV_SP_NOTTRANS  = 0x07
} CV_special_e;




//      subtype enumeration values for CV_SPECIAL2


typedef enum CV_special2_e {
    CV_S2_BIT       = 0x00,
    CV_S2_PASCHAR   = 0x01          // Pascal CHAR
} CV_special2_e;





//      subtype enumeration values for CV_SIGNED, CV_UNSIGNED and CV_BOOLEAN


typedef enum CV_integral_e {
    CV_IN_1BYTE     = 0x00,
    CV_IN_2BYTE     = 0x01,
    CV_IN_4BYTE     = 0x02,
    CV_IN_8BYTE     = 0x03,
    CV_IN_16BYTE    = 0x04
} CV_integral_e;





//      subtype enumeration values for CV_REAL and CV_COMPLEX


typedef enum CV_real_e {
    CV_RC_REAL32    = 0x00,
    CV_RC_REAL64    = 0x01,
    CV_RC_REAL80    = 0x02,
    CV_RC_REAL128   = 0x03,
    CV_RC_REAL48    = 0x04
} CV_real_e;




//      subtype enumeration values for CV_INT (really int)


typedef enum CV_int_e {
    CV_RI_CHAR      = 0x00,
    CV_RI_INT1      = 0x00,
    CV_RI_WCHAR     = 0x01,
    CV_RI_UINT1     = 0x01,
    CV_RI_INT2      = 0x02,
    CV_RI_UINT2     = 0x03,
    CV_RI_INT4      = 0x04,
    CV_RI_UINT4     = 0x05,
    CV_RI_INT8      = 0x06,
    CV_RI_UINT8     = 0x07,
    CV_RI_INT16     = 0x08,
    CV_RI_UINT16    = 0x09
} CV_int_e;




// macros to check the type of a primitive

#define CV_TYP_IS_DIRECT(typ)   (CV_MODE(typ) == CV_TM_DIRECT)
#define CV_TYP_IS_PTR(typ)      (CV_MODE(typ) != CV_TM_DIRECT)
#define CV_TYP_IS_NPTR(typ)     (CV_MODE(typ) == CV_TM_NPTR)
#define CV_TYP_IS_FPTR(typ)     (CV_MODE(typ) == CV_TM_FPTR)
#define CV_TYP_IS_HPTR(typ)     (CV_MODE(typ) == CV_TM_HPTR)
#define CV_TYP_IS_NPTR32(typ)   (CV_MODE(typ) == CV_TM_NPTR32)
#define CV_TYP_IS_FPTR32(typ)   (CV_MODE(typ) == CV_TM_FPTR32)

#define CV_TYP_IS_SIGNED(typ)   (((CV_TYPE(typ) == CV_SIGNED) && CV_TYP_IS_DIRECT(typ)) || \
                                 (typ == T_INT1)  || \
                                 (typ == T_INT2)  || \
                                 (typ == T_INT4)  || \
                                 (typ == T_INT8)  || \
                                 (typ == T_INT16) || \
                                 (typ == T_RCHAR))

#define CV_TYP_IS_UNSIGNED(typ) (((CV_TYPE(typ) == CV_UNSIGNED) && CV_TYP_IS_DIRECT(typ)) || \
                                 (typ == T_UINT1) || \
                                 (typ == T_UINT2) || \
                                 (typ == T_UINT4) || \
                                 (typ == T_UINT8) || \
                                 (typ == T_UINT16))

#define CV_TYP_IS_REAL(typ)     ((CV_TYPE(typ) == CV_REAL)  && CV_TYP_IS_DIRECT(typ))

#define CV_FIRST_NONPRIM 0x1000
#define CV_IS_PRIMITIVE(typ)    ((typ) < CV_FIRST_NONPRIM)
#define CV_TYP_IS_COMPLEX(typ)  ((CV_TYPE(typ) == CV_COMPLEX)   && CV_TYP_IS_DIRECT(typ))




// selected values for type_index - for a more complete definition, see
// Microsoft Symbol and Type OMF document




//      Special Types


#define T_NOTYPE        0x0000      // uncharacterized type (no type)
#define T_ABS           0x0001      // absolute symbol
#define T_SEGMENT       0x0002      // segment type
#define T_VOID          0x0003      // void
#define T_PVOID         0x0103      // near pointer to void
#define T_PFVOID        0x0203      // far pointer to void
#define T_PHVOID        0x0303      // huge pointer to void
#define T_32PVOID       0x0403      // 16:32 near pointer to void
#define T_32PFVOID      0x0503      // 16:32 far pointer to void
#define T_64PVOID       0x0603      // 64 bit pointer to void
#define T_CURRENCY      0x0004      // BASIC 8 byte currency value
#define T_NBASICSTR     0x0005      // Near BASIC string
#define T_FBASICSTR     0x0006      // Far BASIC string
#define T_NOTTRANS      0x0007      // type not translated by cvpack
#define T_BIT           0x0060      // bit
#define T_PASCHAR       0x0061      // Pascal CHAR



//      Character types


#define T_CHAR          0x0010      // 8 bit signed
#define T_UCHAR         0x0020      // 8 bit unsigned
#define T_PCHAR         0x0110      // near pointer to 8 bit signed
#define T_PUCHAR        0x0120      // near pointer to 8 bit unsigned
#define T_PFCHAR        0x0210      // far pointer to 8 bit signed
#define T_PFUCHAR       0x0220      // far pointer to 8 bit unsigned
#define T_PHCHAR        0x0310      // huge pointer to 8 bit signed
#define T_PHUCHAR       0x0320      // huge pointer to 8 bit unsigned
#define T_32PCHAR       0x0410      // 16:32 near pointer to 8 bit signed
#define T_32PUCHAR      0x0420      // 16:32 near pointer to 8 bit unsigned
#define T_32PFCHAR      0x0510      // 16:32 far pointer to 8 bit signed
#define T_32PFUCHAR     0x0520      // 16:32 far pointer to 8 bit unsigned
#define T_64PCHAR       0X0610      // 64 bit pointer to 8 bit signed
#define T_64PUCHAR      0X0620      // 64 bit pointer to 8 bit unsigned




//      really a character types

#define T_RCHAR         0x0070      // really a char
#define T_PRCHAR        0x0170      // 16:16 near pointer to a real char
#define T_PFRCHAR       0x0270      // 16:16 far pointer to a real char
#define T_PHRCHAR       0x0370      // 16:16 huge pointer to a real char
#define T_32PRCHAR      0x0470      // 16:32 near pointer to a real char
#define T_32PFRCHAR     0x0570      // 16:32 far pointer to a real char
#define T_64PRCHAR      0x0670      // 64 bit pointer to a real char



//      really a wide character types

#define T_WCHAR         0x0071      // wide char
#define T_PWCHAR        0x0171      // 16:16 near pointer to a wide char
#define T_PFWCHAR       0x0271      // 16:16 far pointer to a wide char
#define T_PHWCHAR       0x0371      // 16:16 huge pointer to a wide char
#define T_32PWCHAR      0x0471      // 16:32 near pointer to a wide char
#define T_32PFWCHAR     0x0571      // 16:32 far pointer to a wide char
#define T_64PWCHAR      0x0671      // 64 bit pointer to a wide char


//      8 bit int types


#define T_INT1          0x0068      // 8 bit signed int
#define T_UINT1         0x0069      // 8 bit unsigned int
#define T_PINT1         0x0168      // near pointer to 8 bit signed int
#define T_PUINT1        0x0169      // near pointer to 8 bit unsigned int
#define T_PFINT1        0x0268      // far pointer to 8 bit signed int
#define T_PFUINT1       0x0269      // far pointer to 8 bit unsigned int
#define T_PHINT1        0x0368      // huge pointer to 8 bit signed int
#define T_PHUINT1       0x0369      // huge pointer to 8 bit unsigned int

#define T_32PINT1       0x0468      // 16:32 near pointer to 8 bit signed int
#define T_32PUINT1      0x0469      // 16:32 near pointer to 8 bit unsigned int
#define T_32PFINT1      0x0568      // 16:32 far pointer to 8 bit signed int
#define T_32PFUINT1     0x0569      // 16:32 far pointer to 8 bit unsigned int
#define T_64PINT1       0x0668      // 64 bit pointer to 8 bit signed int
#define T_64PUINT1      0x0669      // 64 bit pointer to 8 bit unsigned int


//      16 bit short types


#define T_SHORT         0x0011      // 16 bit signed
#define T_USHORT        0x0021      // 16 bit unsigned
#define T_PSHORT        0x0111      // near pointer to 16 bit signed
#define T_PUSHORT       0x0121      // near pointer to 16 bit unsigned
#define T_PFSHORT       0x0211      // far pointer to 16 bit signed
#define T_PFUSHORT      0x0221      // far pointer to 16 bit unsigned
#define T_PHSHORT       0x0311      // huge pointer to 16 bit signed
#define T_PHUSHORT      0x0321      // huge pointer to 16 bit unsigned

#define T_32PSHORT      0x0411      // 16:32 near pointer to 16 bit signed
#define T_32PUSHORT     0x0421      // 16:32 near pointer to 16 bit unsigned
#define T_32PFSHORT     0x0511      // 16:32 far pointer to 16 bit signed
#define T_32PFUSHORT    0x0521      // 16:32 far pointer to 16 bit unsigned
#define T_64PSHORT      0x0611      // 64 bit pointer to 16 bit signed
#define T_64PUSHORT     0x0621      // 64 bit pointer to 16 bit unsigned




//      16 bit int types


#define T_INT2          0x0072      // 16 bit signed int
#define T_UINT2         0x0073      // 16 bit unsigned int
#define T_PINT2         0x0172      // near pointer to 16 bit signed int
#define T_PUINT2        0x0173      // near pointer to 16 bit unsigned int
#define T_PFINT2        0x0272      // far pointer to 16 bit signed int
#define T_PFUINT2       0x0273      // far pointer to 16 bit unsigned int
#define T_PHINT2        0x0372      // huge pointer to 16 bit signed int
#define T_PHUINT2       0x0373      // huge pointer to 16 bit unsigned int

#define T_32PINT2       0x0472      // 16:32 near pointer to 16 bit signed int
#define T_32PUINT2      0x0473      // 16:32 near pointer to 16 bit unsigned int
#define T_32PFINT2      0x0572      // 16:32 far pointer to 16 bit signed int
#define T_32PFUINT2     0x0573      // 16:32 far pointer to 16 bit unsigned int
#define T_64PINT2       0x0672      // 64 bit pointer to 16 bit signed int
#define T_64PUINT2      0x0673      // 64 bit pointer to 16 bit unsigned int




//      32 bit long types


#define T_LONG          0x0012      // 32 bit signed
#define T_ULONG         0x0022      // 32 bit unsigned
#define T_PLONG         0x0112      // near pointer to 32 bit signed
#define T_PULONG        0x0122      // near pointer to 32 bit unsigned
#define T_PFLONG        0x0212      // far pointer to 32 bit signed
#define T_PFULONG       0x0222      // far pointer to 32 bit unsigned
#define T_PHLONG        0x0312      // huge pointer to 32 bit signed
#define T_PHULONG       0x0322      // huge pointer to 32 bit unsigned

#define T_32PLONG       0x0412      // 16:32 near pointer to 32 bit signed
#define T_32PULONG      0x0422      // 16:32 near pointer to 32 bit unsigned
#define T_32PFLONG      0x0512      // 16:32 far pointer to 32 bit signed
#define T_32PFULONG     0x0522      // 16:32 far pointer to 32 bit unsigned
#define T_64PLONG       0x0612      // 64 bit pointer to 32 bit signed
#define T_64PULONG      0x0622      // 64 bit pointer to 32 bit unsigned




//      32 bit int types


#define T_INT4          0x0074      // 32 bit signed int
#define T_UINT4         0x0075      // 32 bit unsigned int
#define T_PINT4         0x0174      // near pointer to 32 bit signed int
#define T_PUINT4        0x0175      // near pointer to 32 bit unsigned int
#define T_PFINT4        0x0274      // far pointer to 32 bit signed int
#define T_PFUINT4       0x0275      // far pointer to 32 bit unsigned int
#define T_PHINT4        0x0374      // huge pointer to 32 bit signed int
#define T_PHUINT4       0x0375      // huge pointer to 32 bit unsigned int

#define T_32PINT4       0x0474      // 16:32 near pointer to 32 bit signed int
#define T_32PUINT4      0x0475      // 16:32 near pointer to 32 bit unsigned int
#define T_32PFINT4      0x0574      // 16:32 far pointer to 32 bit signed int
#define T_32PFUINT4     0x0575      // 16:32 far pointer to 32 bit unsigned int
#define T_64PINT4       0x0674      // 64 bit pointer to 32 bit signed int
#define T_64PUINT4      0x0675      // 64 bit pointer to 32 bit unsigned int




//      64 bit quad types


#define T_QUAD          0x0013      // 64 bit signed
#define T_UQUAD         0x0023      // 64 bit unsigned
#define T_PQUAD         0x0113      // near pointer to 64 bit signed
#define T_PUQUAD        0x0123      // near pointer to 64 bit unsigned
#define T_PFQUAD        0x0213      // far pointer to 64 bit signed
#define T_PFUQUAD       0x0223      // far pointer to 64 bit unsigned
#define T_PHQUAD        0x0313      // huge pointer to 64 bit signed
#define T_PHUQUAD       0x0323      // huge pointer to 64 bit unsigned
#define T_32PQUAD       0x0413      // 16:32 near pointer to 64 bit signed
#define T_32PUQUAD      0x0423      // 16:32 near pointer to 64 bit unsigned
#define T_32PFQUAD      0x0513      // 16:32 far pointer to 64 bit signed
#define T_32PFUQUAD     0x0523      // 16:32 far pointer to 64 bit unsigned
#define T_64PQUAD       0x0613      // 64 bit pointer to 64 bit signed
#define T_64PUQUAD      0x0623      // 64 bit pointer to 64 bit unsigned



//      64 bit int types


#define T_INT8          0x0076      // 64 bit signed int
#define T_UINT8         0x0077      // 64 bit unsigned int
#define T_PINT8         0x0176      // near pointer to 64 bit signed int
#define T_PUINT8        0x0177      // near pointer to 64 bit unsigned int
#define T_PFINT8        0x0276      // far pointer to 64 bit signed int
#define T_PFUINT8       0x0277      // far pointer to 64 bit unsigned int
#define T_PHINT8        0x0376      // huge pointer to 64 bit signed int
#define T_PHUINT8       0x0377      // huge pointer to 64 bit unsigned int

#define T_32PINT8       0x0476      // 16:32 near pointer to 64 bit signed int
#define T_32PUINT8      0x0477      // 16:32 near pointer to 64 bit unsigned int
#define T_32PFINT8      0x0576      // 16:32 far pointer to 64 bit signed int
#define T_32PFUINT8     0x0577      // 16:32 far pointer to 64 bit unsigned int
#define T_64PINT8       0x0676      // 64 bit pointer to 64 bit signed int
#define T_64PUINT8      0x0677      // 64 bit pointer to 64 bit unsigned int


//      128 bit octet types


#define T_OCT           0x0014      // 128 bit signed
#define T_UOCT          0x0024      // 128 bit unsigned
#define T_POCT          0x0114      // near pointer to 128 bit signed
#define T_PUOCT         0x0124      // near pointer to 128 bit unsigned
#define T_PFOCT         0x0214      // far pointer to 128 bit signed
#define T_PFUOCT        0x0224      // far pointer to 128 bit unsigned
#define T_PHOCT         0x0314      // huge pointer to 128 bit signed
#define T_PHUOCT        0x0324      // huge pointer to 128 bit unsigned

#define T_32POCT        0x0414      // 16:32 near pointer to 128 bit signed
#define T_32PUOCT       0x0424      // 16:32 near pointer to 128 bit unsigned
#define T_32PFOCT       0x0514      // 16:32 far pointer to 128 bit signed
#define T_32PFUOCT      0x0524      // 16:32 far pointer to 128 bit unsigned
#define T_64POCT        0x0614      // 64 bit pointer to 128 bit signed
#define T_64PUOCT       0x0624      // 64 bit pointer to 128 bit unsigned

//      128 bit int types


#define T_INT16         0x0078      // 128 bit signed int
#define T_UINT16        0x0079      // 128 bit unsigned int
#define T_PINT16        0x0178      // near pointer to 128 bit signed int
#define T_PUINT16       0x0179      // near pointer to 128 bit unsigned int
#define T_PFINT16       0x0278      // far pointer to 128 bit signed int
#define T_PFUINT16      0x0279      // far pointer to 128 bit unsigned int
#define T_PHINT16       0x0378      // huge pointer to 128 bit signed int
#define T_PHUINT16      0x0379      // huge pointer to 128 bit unsigned int

#define T_32PINT16      0x0478      // 16:32 near pointer to 128 bit signed int
#define T_32PUINT16     0x0479      // 16:32 near pointer to 128 bit unsigned int
#define T_32PFINT16     0x0578      // 16:32 far pointer to 128 bit signed int
#define T_32PFUINT16    0x0579      // 16:32 far pointer to 128 bit unsigned int
#define T_64PINT16      0x0678      // 64 bit pointer to 128 bit signed int
#define T_64PUINT16     0x0679      // 64 bit pointer to 128 bit unsigned int





//      32 bit real types


#define T_REAL32        0x0040      // 32 bit real
#define T_PREAL32       0x0140      // near pointer to 32 bit real
#define T_PFREAL32      0x0240      // far pointer to 32 bit real
#define T_PHREAL32      0x0340      // huge pointer to 32 bit real
#define T_32PREAL32     0x0440      // 16:32 near pointer to 32 bit real
#define T_32PFREAL32    0x0540      // 16:32 far pointer to 32 bit real
#define T_64PREAL32     0x0640      // 64 bit pointer to 32 bit real



//      48 bit real types


#define T_REAL48        0x0044      // 48 bit real
#define T_PREAL48       0x0144      // near pointer to 48 bit real
#define T_PFREAL48      0x0244      // far pointer to 48 bit real
#define T_PHREAL48      0x0344      // huge pointer to 48 bit real
#define T_32PREAL48     0x0444      // 16:32 near pointer to 48 bit real
#define T_32PFREAL48    0x0544      // 16:32 far pointer to 48 bit real
#define T_64PREAL48     0x0644      // 64 bit pointer to 48 bit real




//      64 bit real types


#define T_REAL64        0x0041      // 64 bit real
#define T_PREAL64       0x0141      // near pointer to 64 bit real
#define T_PFREAL64      0x0241      // far pointer to 64 bit real
#define T_PHREAL64      0x0341      // huge pointer to 64 bit real
#define T_32PREAL64     0x0441      // 16:32 near pointer to 64 bit real
#define T_32PFREAL64    0x0541      // 16:32 far pointer to 64 bit real
#define T_64PREAL64     0x0641      // 64 bit pointer to 64 bit real




//      80 bit real types


#define T_REAL80        0x0042      // 80 bit real
#define T_PREAL80       0x0142      // near pointer to 80 bit real
#define T_PFREAL80      0x0242      // far pointer to 80 bit real
#define T_PHREAL80      0x0342      // huge pointer to 80 bit real
#define T_32PREAL80     0x0442      // 16:32 near pointer to 80 bit real
#define T_32PFREAL80    0x0542      // 16:32 far pointer to 80 bit real
#define T_64PREAL80     0x0642      // 64 bit pointer to 80 bit real




//      128 bit real types


#define T_REAL128       0x0043      // 128 bit real
#define T_PREAL128      0x0143      // near pointer to 128 bit real
#define T_PFREAL128     0x0243      // far pointer to 128 bit real
#define T_PHREAL128     0x0343      // huge pointer to 128 bit real
#define T_32PREAL128    0x0443      // 16:32 near pointer to 128 bit real
#define T_32PFREAL128   0x0543      // 16:32 far pointer to 128 bit real
#define T_64PREAL128    0x0643      // 64 bit pointer to 128 bit real




//      32 bit complex types


#define T_CPLX32        0x0050      // 32 bit complex
#define T_PCPLX32       0x0150      // near pointer to 32 bit complex
#define T_PFCPLX32      0x0250      // far pointer to 32 bit complex
#define T_PHCPLX32      0x0350      // huge pointer to 32 bit complex
#define T_32PCPLX32     0x0450      // 16:32 near pointer to 32 bit complex
#define T_32PFCPLX32    0x0550      // 16:32 far pointer to 32 bit complex
#define T_64PCPLX32     0x0650      // 64 bit pointer to 32 bit complex




//      64 bit complex types


#define T_CPLX64        0x0051      // 64 bit complex
#define T_PCPLX64       0x0151      // near pointer to 64 bit complex
#define T_PFCPLX64      0x0251      // far pointer to 64 bit complex
#define T_PHCPLX64      0x0351      // huge pointer to 64 bit complex
#define T_32PCPLX64     0x0451      // 16:32 near pointer to 64 bit complex
#define T_32PFCPLX64    0x0551      // 16:32 far pointer to 64 bit complex
#define T_64PCPLX64     0x0651      // 64 bit pointer to 64 bit complex




//      80 bit complex types


#define T_CPLX80        0x0052      // 80 bit complex
#define T_PCPLX80       0x0152      // near pointer to 80 bit complex
#define T_PFCPLX80      0x0252      // far pointer to 80 bit complex
#define T_PHCPLX80      0x0352      // huge pointer to 80 bit complex
#define T_32PCPLX80     0x0452      // 16:32 near pointer to 80 bit complex
#define T_32PFCPLX80    0x0552      // 16:32 far pointer to 80 bit complex
#define T_64PCPLX80     0x0652      // 64 bit pointer to 80 bit complex




//      128 bit complex types


#define T_CPLX128       0x0053      // 128 bit complex
#define T_PCPLX128      0x0153      // near pointer to 128 bit complex
#define T_PFCPLX128     0x0253      // far pointer to 128 bit complex
#define T_PHCPLX128     0x0353      // huge pointer to 128 bit real
#define T_32PCPLX128    0x0453      // 16:32 near pointer to 128 bit complex
#define T_32PFCPLX128   0x0553      // 16:32 far pointer to 128 bit complex
#define T_64PCPLX128    0x0653      // 64 bit pointer to 128 bit complex




//      boolean types


#define T_BOOL08        0x0030      // 8 bit boolean
#define T_BOOL16        0x0031      // 16 bit boolean
#define T_BOOL32        0x0032      // 32 bit boolean
#define T_BOOL64        0x0033      // 64 bit boolean
#define T_PBOOL08       0x0130      // near pointer to  8 bit boolean
#define T_PBOOL16       0x0131      // near pointer to 16 bit boolean
#define T_PBOOL32       0x0132      // near pointer to 32 bit boolean
#define T_PBOOL64       0x0133      // near pointer to 64 bit boolean
#define T_PFBOOL08      0x0230      // far pointer to  8 bit boolean
#define T_PFBOOL16      0x0231      // far pointer to 16 bit boolean
#define T_PFBOOL32      0x0232      // far pointer to 32 bit boolean
#define T_PFBOOL64      0x0233      // far pointer to 64 bit boolean
#define T_PHBOOL08      0x0330      // huge pointer to  8 bit boolean
#define T_PHBOOL16      0x0331      // huge pointer to 16 bit boolean
#define T_PHBOOL32      0x0332      // huge pointer to 32 bit boolean
#define T_PHBOOL64      0x0333      // huge pointer to 64 bit boolean

#define T_32PBOOL08     0x0430      // 16:32 near pointer to 8 bit boolean
#define T_32PFBOOL08    0x0530      // 16:32 far pointer to 8 bit boolean
#define T_32PBOOL16     0x0431      // 16:32 near pointer to 18 bit boolean
#define T_32PFBOOL16    0x0531      // 16:32 far pointer to 16 bit boolean
#define T_32PBOOL32     0x0432      // 16:32 near pointer to 32 bit boolean
#define T_32PFBOOL32    0x0532      // 16:32 far pointer to 32 bit boolean
#define T_32PBOOL64     0x0433      // 16:32 near pointer to 64 bit boolean
#define T_32PFBOOL64    0x0533      // 16:32 far pointer to 64 bit boolean

#define T_64PBOOL08     0x0630      // 64 bit pointer to 8 bit boolean
#define T_64PBOOL16     0x0631      // 64 bit pointer to 18 bit boolean
#define T_64PBOOL32     0x0632      // 64 bit pointer to 32 bit boolean
#define T_64PBOOL64     0x0633      // 64 bit pointer to 64 bit boolean


#define T_NCVPTR        0x01f0      // CV Internal type for created near pointers
#define T_FCVPTR        0x02f0      // CV Internal type for created far pointers
#define T_HCVPTR        0x03f0      // CV Internal type for created huge pointers
#define T_32NCVPTR      0x04f0      // CV Internal type for created near 32-bit pointers
#define T_32FCVPTR      0x05f0      // CV Internal type for created far 32-bit pointers
#define T_64NCVPTR      0x06f0      // CV Internal type for created near 64-bit pointers

#define CV_IS_INTERNAL_PTR(typ) (CV_IS_PRIMITIVE(typ) && \
                                 CV_TYPE(typ) == CV_CVRESERVED && \
                                 CV_TYP_IS_PTR(typ))


/**     No leaf index can have a value of 0x0000.  The leaf indices are
 *      separated into ranges depending upon the use of the type record.
 *      The second range is for the type records that are directly referenced
 *      in symbols. The first range is for type records that are not
 *      referenced by symbols but instead are referenced by other type
 *      records.  All type records must have a starting leaf index in these
 *      first two ranges.  The third range of leaf indices are used to build
 *      up complex lists such as the field list of a class type record.  No
 *      type record can begin with one of the leaf indices. The fourth ranges
 *      of type indices are used to represent numeric data in a symbol or
 *      type record. These leaf indices are greater than 0x8000.  At the
 *      point that type or symbol processor is expecting a numeric field, the
 *      next two bytes in the type record are examined.  If the value is less
 *      than 0x8000, then the two bytes contain the numeric value.  If the
 *      value is greater than 0x8000, then the data follows the leaf index in
 *      a format specified by the leaf index. The final range of leaf indices
 *      are used to force alignment of subfields within a complex type record..
 */



    // leaf indices starting records but referenced from symbol records

#define LF_MODIFIER_16t     0x0001
#define LF_POINTER_16t      0x0002
#define LF_ARRAY_16t        0x0003
#define LF_CLASS_16t        0x0004
#define LF_STRUCTURE_16t    0x0005
#define LF_UNION_16t        0x0006
#define LF_ENUM_16t         0x0007
#define LF_PROCEDURE_16t    0x0008
#define LF_MFUNCTION_16t    0x0009
#define LF_VTSHAPE          0x000a
#define LF_COBOL0_16t       0x000b
#define LF_COBOL1           0x000c
#define LF_BARRAY_16t       0x000d
#define LF_LABEL            0x000e
#define LF_NULL             0x000f
#define LF_NOTTRAN          0x0010
#define LF_DIMARRAY_16t     0x0011
#define LF_VFTPATH_16t      0x0012
#define LF_PRECOMP_16t      0x0013      // not referenced from symbol
#define LF_ENDPRECOMP       0x0014      // not referenced from symbol
#define LF_OEM_16t          0x0015      // oem definable type string
#define LF_TYPESERVER       0x0016      // not referenced from symbol

    // leaf indices starting records but referenced only from type records

#define LF_SKIP_16t         0x0200
#define LF_ARGLIST_16t      0x0201
#define LF_DEFARG_16t       0x0202
#define LF_LIST             0x0203
#define LF_FIELDLIST_16t    0x0204
#define LF_DERIVED_16t      0x0205
#define LF_BITFIELD_16t     0x0206
#define LF_METHODLIST_16t   0x0207
#define LF_DIMCONU_16t      0x0208
#define LF_DIMCONLU_16t     0x0209
#define LF_DIMVARU_16t      0x020a
#define LF_DIMVARLU_16t     0x020b
#define LF_REFSYM           0x020c
                            
#define LF_BCLASS_16t       0x0400
#define LF_VBCLASS_16t      0x0401
#define LF_IVBCLASS_16t     0x0402
#define LF_ENUMERATE        0x0403
#define LF_FRIENDFCN_16t    0x0404
#define LF_INDEX_16t        0x0405
#define LF_MEMBER_16t       0x0406
#define LF_STMEMBER_16t     0x0407
#define LF_METHOD_16t       0x0408
#define LF_NESTTYPE_16t     0x0409
#define LF_VFUNCTAB_16t     0x040a
#define LF_FRIENDCLS_16t    0x040b
#define LF_ONEMETHOD_16t    0x040c
#define LF_VFUNCOFF_16t     0x040d

// 32-bit type index versions of leaves, all have the 0x1000 bit set
//
#define LF_TI16_MAX         0x1000

#define LF_MODIFIER         0x1001
#define LF_POINTER          0x1002
#define LF_ARRAY            0x1003
#define LF_CLASS            0x1004
#define LF_STRUCTURE        0x1005
#define LF_UNION            0x1006
#define LF_ENUM             0x1007
#define LF_PROCEDURE        0x1008
#define LF_MFUNCTION        0x1009
#define LF_COBOL0           0x100a
#define LF_BARRAY           0x100b
#define LF_DIMARRAY         0x100c
#define LF_VFTPATH          0x100d
#define LF_PRECOMP          0x100e      // not referenced from symbol
#define LF_OEM              0x100f      // oem definable type string
                            
    // leaf indices starting records but referenced only from type records
                            
#define LF_SKIP             0x1200
#define LF_ARGLIST          0x1201
#define LF_DEFARG           0x1202
#define LF_FIELDLIST        0x1203
#define LF_DERIVED          0x1204
#define LF_BITFIELD         0x1205
#define LF_METHODLIST       0x1206
#define LF_DIMCONU          0x1207
#define LF_DIMCONLU         0x1208
#define LF_DIMVARU          0x1209
#define LF_DIMVARLU         0x120a
                            
#define LF_BCLASS           0x1400
#define LF_VBCLASS          0x1401
#define LF_IVBCLASS         0x1402
#define LF_FRIENDFCN        0x1403
#define LF_INDEX            0x1404
#define LF_MEMBER           0x1405
#define LF_STMEMBER         0x1406
#define LF_METHOD           0x1407
#define LF_NESTTYPE         0x1408
#define LF_VFUNCTAB         0x1409
#define LF_FRIENDCLS        0x140a
#define LF_ONEMETHOD        0x140b
#define LF_VFUNCOFF         0x140c
#define LF_NESTTYPEEX       0x140d
#define LF_MEMBERMODIFY     0x140e




#define LF_NUMERIC          0x8000
#define LF_CHAR             0x8000
#define LF_SHORT            0x8001
#define LF_USHORT           0x8002
#define LF_LONG             0x8003
#define LF_ULONG            0x8004
#define LF_REAL32           0x8005
#define LF_REAL64           0x8006
#define LF_REAL80           0x8007
#define LF_REAL128          0x8008
#define LF_QUADWORD         0x8009
#define LF_UQUADWORD        0x800a
#define LF_REAL48           0x800b
#define LF_COMPLEX32        0x800c
#define LF_COMPLEX64        0x800d
#define LF_COMPLEX80        0x800e
#define LF_COMPLEX128       0x800f
#define LF_VARSTRING        0x8010
                            
#define LF_OCTWORD          0x8017
#define LF_UOCTWORD         0x8018
                            
#define LF_PAD0             0xf0
#define LF_PAD1             0xf1
#define LF_PAD2             0xf2
#define LF_PAD3             0xf3
#define LF_PAD4             0xf4
#define LF_PAD5             0xf5
#define LF_PAD6             0xf6
#define LF_PAD7             0xf7
#define LF_PAD8             0xf8
#define LF_PAD9             0xf9
#define LF_PAD10            0xfa
#define LF_PAD11            0xfb
#define LF_PAD12            0xfc
#define LF_PAD13            0xfd
#define LF_PAD14            0xfe
#define LF_PAD15            0xff

// end of leaf indices




//      Type enum for pointer records
//      Pointers can be one of the following types


typedef enum CV_ptrtype_e {
    CV_PTR_NEAR         = 0x00, // near pointer
    CV_PTR_FAR          = 0x01, // far pointer
    CV_PTR_HUGE         = 0x02, // huge pointer
    CV_PTR_BASE_SEG     = 0x03, // based on segment
    CV_PTR_BASE_VAL     = 0x04, // based on value of base
    CV_PTR_BASE_SEGVAL  = 0x05, // based on segment value of base
    CV_PTR_BASE_ADDR    = 0x06, // based on address of base
    CV_PTR_BASE_SEGADDR = 0x07, // based on segment address of base
    CV_PTR_BASE_TYPE    = 0x08, // based on type
    CV_PTR_BASE_SELF    = 0x09, // based on self
    CV_PTR_NEAR32       = 0x0a, // 16:32 near pointer
    CV_PTR_FAR32        = 0x0b, // 16:32 far pointer
    CV_PTR_64           = 0x0c, // 64 bit pointer
    CV_PTR_UNUSEDPTR    = 0x0d  // first unused pointer type
} CV_ptrtype_e;





//      Mode enum for pointers
//      Pointers can have one of the following modes


typedef enum CV_ptrmode_e {
    CV_PTR_MODE_PTR     = 0x00, // "normal" pointer
    CV_PTR_MODE_REF     = 0x01, // reference
    CV_PTR_MODE_PMEM    = 0x02, // pointer to data member
    CV_PTR_MODE_PMFUNC  = 0x03, // pointer to member function
    CV_PTR_MODE_RESERVED= 0x04  // first unused pointer mode
} CV_ptrmode_e;




//      Enumeration for function call type


typedef enum CV_call_e {
    CV_CALL_NEAR_C      = 0x00, // near right to left push, caller pops stack
    CV_CALL_FAR_C       = 0x01, // far right to left push, caller pops stack
    CV_CALL_NEAR_PASCAL = 0x02, // near left to right push, callee pops stack
    CV_CALL_FAR_PASCAL  = 0x03, // far left to right push, callee pops stack
    CV_CALL_NEAR_FAST   = 0x04, // near left to right push with regs, callee pops stack
    CV_CALL_FAR_FAST    = 0x05, // far left to right push with regs, callee pops stack
    CV_CALL_SKIPPED     = 0x06, // skipped (unused) call index
    CV_CALL_NEAR_STD    = 0x07, // near standard call
    CV_CALL_FAR_STD     = 0x08, // far standard call
    CV_CALL_NEAR_SYS    = 0x09, // near sys call
    CV_CALL_FAR_SYS     = 0x0a, // far sys call
    CV_CALL_THISCALL    = 0x0b, // this call (this passed in register)
    CV_CALL_MIPSCALL    = 0x0c, // Mips call
    CV_CALL_GENERIC     = 0x0d, // Generic call sequence
    CV_CALL_ALPHACALL   = 0x0e, // Alpha call
    CV_CALL_PPCCALL     = 0x0f, // PPC call
    CV_CALL_RESERVED    = 0x10  // first unused call enumeration
} CV_call_e;




//      Values for the access protection of class attributes


typedef enum CV_access_e {
    CV_private   = 1,
    CV_protected = 2,
    CV_public    = 3
} CV_access_e;



//      enumeration for method properties

typedef enum CV_methodprop_e {
    CV_MTvanilla        = 0x00,
    CV_MTvirtual        = 0x01,
    CV_MTstatic         = 0x02,
    CV_MTfriend         = 0x03,
    CV_MTintro          = 0x04,
    CV_MTpurevirt       = 0x05,
    CV_MTpureintro      = 0x06
} CV_methodprop_e;




//      enumeration for virtual shape table entries

typedef enum CV_VTS_desc_e {
    CV_VTS_near         = 0x00,
    CV_VTS_far          = 0x01,
    CV_VTS_thin         = 0x02,
    CV_VTS_outer        = 0x03,
    CV_VTS_meta         = 0x04,
    CV_VTS_near32       = 0x05,
    CV_VTS_far32        = 0x06,
    CV_VTS_unused       = 0x07
} CV_VTS_desc_e;




//      enumeration for LF_LABEL address modes

typedef enum CV_LABEL_TYPE_e {
    CV_LABEL_NEAR = 0,       // near return
    CV_LABEL_FAR  = 4        // far return
} CV_LABEL_TYPE_e;



//      enumeration for LF_MODIFIER values


typedef struct CV_modifier_t {
    unsigned short  MOD_const       :1;
    unsigned short  MOD_volatile    :1;
    unsigned short  MOD_unaligned   :1;
    unsigned short  MOD_unused      :13;
} CV_modifier_t;


//  bit field structure describing class/struct/union/enum properties

typedef struct CV_prop_t {
    unsigned short  packed      :1;     // true if structure is packed
    unsigned short  ctor        :1;     // true if constructors or destructors present
    unsigned short  ovlops      :1;     // true if overloaded operators present
    unsigned short  isnested    :1;     // true if this is a nested class
    unsigned short  cnested     :1;     // true if this class contains nested types
    unsigned short  opassign    :1;     // true if overloaded assignment (=)
    unsigned short  opcast      :1;     // true if casting methods
    unsigned short  fwdref      :1;     // true if forward reference (incomplete defn)
    unsigned short  scoped      :1;     // scoped definition
    unsigned short  reserved    :7;
} CV_prop_t;




//  class field attribute

typedef struct CV_fldattr_t {
    unsigned short  access      :2;     // access protection CV_access_t
    unsigned short  mprop       :3;     // method properties CV_methodprop_t
    unsigned short  pseudo      :1;     // compiler generated fcn and does not exist
    unsigned short  noinherit   :1;     // true if class cannot be inherited
    unsigned short  noconstruct :1;     // true if class cannot be constructed
    unsigned short  compgenx    :1;     // compiler generated fcn and does exist
    unsigned short  unused      :7;     // unused
} CV_fldattr_t;



//  Structures to access to the type records


typedef struct TYPTYPE {
    unsigned short  len;
    unsigned short  leaf;
    unsigned char   data[CV_ZEROLEN];
} TYPTYPE;          // general types record


__INLINE char *NextType (char * pType) {
    return (pType + ((TYPTYPE *)pType)->len + sizeof(unsigned short));
}

typedef enum CV_PMEMBER {
    CV_PDM16_NONVIRT    = 0x00, // 16:16 data no virtual fcn or base
    CV_PDM16_VFCN       = 0x01, // 16:16 data with virtual functions
    CV_PDM16_VBASE      = 0x02, // 16:16 data with virtual bases
    CV_PDM32_NVVFCN     = 0x03, // 16:32 data w/wo virtual functions
    CV_PDM32_VBASE      = 0x04, // 16:32 data with virtual bases

    CV_PMF16_NEARNVSA   = 0x05, // 16:16 near method nonvirtual single address point
    CV_PMF16_NEARNVMA   = 0x06, // 16:16 near method nonvirtual multiple address points
    CV_PMF16_NEARVBASE  = 0x07, // 16:16 near method virtual bases
    CV_PMF16_FARNVSA    = 0x08, // 16:16 far method nonvirtual single address point
    CV_PMF16_FARNVMA    = 0x09, // 16:16 far method nonvirtual multiple address points
    CV_PMF16_FARVBASE   = 0x0a, // 16:16 far method virtual bases

    CV_PMF32_NVSA       = 0x0b, // 16:32 method nonvirtual single address point
    CV_PMF32_NVMA       = 0x0c, // 16:32 method nonvirtual multiple address point
    CV_PMF32_VBASE      = 0x0d  // 16:32 method virtual bases
} CV_PMEMBER;



//  memory representation of pointer to member.  These representations are
//  indexed by the enumeration above in the LF_POINTER record




//  representation of a 16:16 pointer to data for a class with no
//  virtual functions or virtual bases


struct CV_PDMR16_NONVIRT {
    CV_off16_t      mdisp;      // displacement to data (NULL = -1)
};




//  representation of a 16:16 pointer to data for a class with virtual
//  functions


struct CV_PMDR16_VFCN {
    CV_off16_t      mdisp;      // displacement to data ( NULL = 0)
};




//  representation of a 16:16 pointer to data for a class with
//  virtual bases


struct CV_PDMR16_VBASE {
    CV_off16_t      mdisp;      // displacement to data
    CV_off16_t      pdisp;      // this pointer displacement to vbptr
    CV_off16_t      vdisp;      // displacement within vbase table
                                // NULL = (,,0xffff)
};




//  representation of a 16:32 near pointer to data for a class with
//  or without virtual functions and no virtual bases


struct CV_PDMR32_NVVFCN {
    CV_off32_t      mdisp;      // displacement to data (NULL = 0x80000000)
};




//  representation of a 16:32 near pointer to data for a class
//  with virtual bases


struct CV_PDMR32_VBASE {
    CV_off32_t      mdisp;      // displacement to data
    CV_off32_t      pdisp;      // this pointer displacement
    CV_off32_t      vdisp;      // vbase table displacement
                                // NULL = (,,0xffffffff)
};




//  representation of a 16:16 pointer to near member function for a
//  class with no virtual functions or bases and a single address point


struct CV_PMFR16_NEARNVSA {
    CV_uoff16_t     off;        // near address of function (NULL = 0)
};



//  representation of a 16:16 near pointer to member functions of a
//  class with no virtual bases and multiple address points


struct CV_PMFR16_NEARNVMA {
    CV_uoff16_t     off;        // offset of function (NULL = 0,x)
    signed short    disp;
};




//  representation of a 16:16 near pointer to member function of a
//  class with virtual bases


struct CV_PMFR16_NEARVBASE {
    CV_uoff16_t     off;        // offset of function (NULL = 0,x,x,x)
    CV_off16_t      mdisp;      // displacement to data
    CV_off16_t      pdisp;      // this pointer displacement
    CV_off16_t      vdisp;      // vbase table displacement
};




//  representation of a 16:16 pointer to far member function for a
//  class with no virtual bases and a single address point


struct CV_PMFR16_FARNVSA {
    CV_uoff16_t     off;        // offset of function (NULL = 0:0)
    unsigned short  seg;        // segment of function
};




//  representation of a 16:16 far pointer to member functions of a
//  class with no virtual bases and multiple address points


struct CV_PMFR16_FARNVMA {
    CV_uoff16_t     off;        // offset of function (NULL = 0:0,x)
    unsigned short  seg;
    signed short    disp;
};




//  representation of a 16:16 far pointer to member function of a
//  class with virtual bases


struct CV_PMFR16_FARVBASE {
    CV_uoff16_t     off;        // offset of function (NULL = 0:0,x,x,x)
    unsigned short  seg;
    CV_off16_t      mdisp;      // displacement to data
    CV_off16_t      pdisp;      // this pointer displacement
    CV_off16_t      vdisp;      // vbase table displacement

};




//  representation of a 16:32 near pointer to member function for a
//  class with no virtual bases and a single address point


struct CV_PMFR32_NVSA {
    CV_uoff32_t      off;        // near address of function (NULL = 0L)
};




//  representation of a 16:32 near pointer to member function for a
//  class with no virtual bases and multiple address points


struct CV_PMFR32_NVMA {
    CV_uoff32_t     off;        // near address of function (NULL = 0L,x)
    CV_off32_t      disp;
};




//  representation of a 16:32 near pointer to member function for a
//  class with virtual bases


struct CV_PMFR32_VBASE {
    CV_uoff32_t     off;        // near address of function (NULL = 0L,x,x,x)
    CV_off32_t      mdisp;      // displacement to data
    CV_off32_t      pdisp;      // this pointer displacement
    CV_off32_t      vdisp;      // vbase table displacement
};





//  Easy leaf - used for generic casting to reference leaf field
//  of a subfield of a complex list

typedef struct lfEasy {
    unsigned short  leaf;           // LF_...
} lfEasy;


/**     The following type records are basically variant records of the
 *      above structure.  The "unsigned short leaf" of the above structure and
 *      the "unsigned short leaf" of the following type definitions are the same
 *      symbol.  When the OMF record is locked via the MHOMFLock API
 *      call, the address of the "unsigned short leaf" is returned
 */

/**     Notes on alignment
 *      Alignment of the fields in most of the type records is done on the 
 *      basis of the TYPTYPE record base.  That is why in most of the lf*
 *      records that the CV_typ_t (32-bit types) is located on what appears to
 *      be a offset mod 4 == 2 boundary.  The exception to this rule are those
 *      records that are in a list (lfFieldList, lfMethodList), which are
 *      aligned to their own bases since they don't have the length field
 */

/**** Change log for 16-bit to 32-bit type and symbol records

    Record type         Change (f == field arrangement, p = padding added)
    ----------------------------------------------------------------------
    lfModifer           f
    lfPointer           fp
    lfClass             f
    lfStructure         f
    lfUnion             f
    lfEnum              f
    lfVFTPath           p
    lfPreComp           p
    lfOEM               p
    lfArgList           p
    lfDerived           p
    mlMethod            p   (method list member)
    lfBitField          f
    lfDimCon            f
    lfDimVar            p
    lfIndex             p   (field list member)
    lfBClass            f   (field list member)
    lfVBClass           f   (field list member)
    lfFriendCls         p   (field list member)
    lfFriendFcn         p   (field list member)
    lfMember            f   (field list member)
    lfSTMember          f   (field list member)
    lfVFuncTab          p   (field list member)
    lfVFuncOff          p   (field list member)
    lfNestType          p   (field list member)

    DATASYM32           f
    PROCSYM32           f
    VPATHSYM32          f
    REGREL32            f
    THREADSYM32         f
    PROCSYMMIPS         f

    
*/

//      Type record for LF_MODIFIER

typedef struct lfModifier_16t {
    unsigned short  leaf;           // LF_MODIFIER_16t
    CV_modifier_t   attr;           // modifier attribute modifier_t
    CV_typ16_t      type;           // modified type
} lfModifier_16t;

typedef struct lfModifier {
    unsigned short  leaf;           // LF_MODIFIER
    CV_typ_t        type;           // modified type
    CV_modifier_t   attr;           // modifier attribute modifier_t
} lfModifier;




//      type record for LF_POINTER

#ifndef __cplusplus
typedef struct lfPointer_16t {
#endif
    struct lfPointerBody_16t {
        unsigned short      leaf;           // LF_POINTER_16t
        struct lfPointerAttr_16t {
            unsigned char   ptrtype     :5; // ordinal specifying pointer type (CV_ptrtype_e)
            unsigned char   ptrmode     :3; // ordinal specifying pointer mode (CV_ptrmode_e)
            unsigned char   isflat32    :1; // true if 0:32 pointer
            unsigned char   isvolatile  :1; // TRUE if volatile pointer
            unsigned char   isconst     :1; // TRUE if const pointer
            unsigned char   isunaligned :1; // TRUE if unaligned pointer
            unsigned char   unused      :4;
        } attr;
        CV_typ16_t  utype;          // type index of the underlying type
#if (defined(__cplusplus) || defined(_MSC_VER)) // for C++ and MS compilers that support unnamed unions
    };
#else
    } u;
#endif
#ifdef  __cplusplus
typedef struct lfPointer_16t : public lfPointerBody_16t {
#endif
    union  {
        struct {
            CV_typ16_t      pmclass;    // index of containing class for pointer to member
            unsigned short  pmenum;     // enumeration specifying pm format
        } pm;
        unsigned short      bseg;       // base segment if PTR_BASE_SEG
        unsigned char       Sym[1];     // copy of base symbol record (including length)
        struct  {
            CV_typ16_t      index;      // type index if CV_PTR_BASE_TYPE
            unsigned char   name[1];    // name of base type
        } btype;
    } pbase;
} lfPointer_16t;

#ifndef __cplusplus
typedef struct lfPointer {
#endif
    struct lfPointerBody {
        unsigned short      leaf;           // LF_POINTER
        CV_typ_t            utype;          // type index of the underlying type
        struct lfPointerAttr {
            unsigned long   ptrtype     :5; // ordinal specifying pointer type (CV_ptrtype_e)
            unsigned long   ptrmode     :3; // ordinal specifying pointer mode (CV_ptrmode_e)
            unsigned long   isflat32    :1; // true if 0:32 pointer
            unsigned long   isvolatile  :1; // TRUE if volatile pointer
            unsigned long   isconst     :1; // TRUE if const pointer
            unsigned long   isunaligned :1; // TRUE if unaligned pointer
            unsigned long   isrestrict  :1; // TRUE if restricted pointer (allow agressive opts)
            unsigned long   unused      :19;// pad out to 32-bits for following cv_typ_t's
        } attr;
#if (defined(__cplusplus) || defined(_MSC_VER)) // for C++ and MS compilers that support unnamed unions
    };
#else
    } u;
#endif
#ifdef  __cplusplus
typedef struct lfPointer : public lfPointerBody {
#endif
    union  {
        struct {
            CV_typ_t        pmclass;    // index of containing class for pointer to member
            unsigned short  pmenum;     // enumeration specifying pm format
        } pm;
        unsigned short      bseg;       // base segment if PTR_BASE_SEG
        unsigned char       Sym[1];     // copy of base symbol record (including length)
        struct  {
            CV_typ_t        index;      // type index if CV_PTR_BASE_TYPE
            unsigned char   name[1];    // name of base type
        } btype;
    } pbase;
} lfPointer;




//      type record for LF_ARRAY


typedef struct lfArray_16t {
    unsigned short  leaf;           // LF_ARRAY_16t
    CV_typ16_t      elemtype;       // type index of element type
    CV_typ16_t      idxtype;        // type index of indexing type
    unsigned char   data[CV_ZEROLEN];         // variable length data specifying
                                    // size in bytes and name
} lfArray_16t;

typedef struct lfArray {
    unsigned short  leaf;           // LF_ARRAY
    CV_typ_t        elemtype;       // type index of element type
    CV_typ_t        idxtype;        // type index of indexing type
    unsigned char   data[CV_ZEROLEN];         // variable length data specifying
                                    // size in bytes and name
} lfArray;




//      type record for LF_CLASS, LF_STRUCTURE


typedef struct lfClass_16t {
    unsigned short  leaf;           // LF_CLASS_16t, LF_STRUCT_16t
    unsigned short  count;          // count of number of elements in class
    CV_typ16_t      field;          // type index of LF_FIELD descriptor list
    CV_prop_t       property;       // property attribute field (prop_t)
    CV_typ16_t      derived;        // type index of derived from list if not zero
    CV_typ16_t      vshape;         // type index of vshape table for this class
    unsigned char   data[CV_ZEROLEN];         // data describing length of structure in
                                    // bytes and name
} lfClass_16t;
typedef lfClass_16t lfStructure_16t;


typedef struct lfClass {
    unsigned short  leaf;           // LF_CLASS, LF_STRUCT
    unsigned short  count;          // count of number of elements in class
    CV_prop_t       property;       // property attribute field (prop_t)
    CV_typ_t        field;          // type index of LF_FIELD descriptor list
    CV_typ_t        derived;        // type index of derived from list if not zero
    CV_typ_t        vshape;         // type index of vshape table for this class
    unsigned char   data[CV_ZEROLEN];         // data describing length of structure in
                                    // bytes and name
} lfClass;
typedef lfClass lfStructure;




//      type record for LF_UNION


typedef struct lfUnion_16t {
    unsigned short  leaf;           // LF_UNION_16t
    unsigned short  count;          // count of number of elements in class
    CV_typ16_t      field;          // type index of LF_FIELD descriptor list
    CV_prop_t       property;       // property attribute field
    unsigned char   data[CV_ZEROLEN];         // variable length data describing length of
                                    // structure and name
} lfUnion_16t;


typedef struct lfUnion {
    unsigned short  leaf;           // LF_UNION
    unsigned short  count;          // count of number of elements in class
    CV_prop_t       property;       // property attribute field
    CV_typ_t        field;          // type index of LF_FIELD descriptor list
    unsigned char   data[CV_ZEROLEN];         // variable length data describing length of
                                    // structure and name
} lfUnion;




//      type record for LF_ENUM


typedef struct lfEnum_16t {
    unsigned short  leaf;           // LF_ENUM_16t
    unsigned short  count;          // count of number of elements in class
    CV_typ16_t      utype;          // underlying type of the enum
    CV_typ16_t      field;          // type index of LF_FIELD descriptor list
    CV_prop_t       property;       // property attribute field
    unsigned char   Name[1];        // length prefixed name of enum
} lfEnum_16t;

typedef struct lfEnum {
    unsigned short  leaf;           // LF_ENUM
    unsigned short  count;          // count of number of elements in class
    CV_prop_t       property;       // property attribute field
    CV_typ_t        utype;          // underlying type of the enum
    CV_typ_t        field;          // type index of LF_FIELD descriptor list
    unsigned char   Name[1];        // length prefixed name of enum
} lfEnum;




//      Type record for LF_PROCEDURE


typedef struct lfProc_16t {
    unsigned short  leaf;           // LF_PROCEDURE_16t
    CV_typ16_t      rvtype;         // type index of return value
    unsigned char   calltype;       // calling convention (CV_call_t)
    unsigned char   reserved;       // reserved for future use
    unsigned short  parmcount;      // number of parameters
    CV_typ16_t      arglist;        // type index of argument list
} lfProc_16t;

typedef struct lfProc {
    unsigned short  leaf;           // LF_PROCEDURE
    CV_typ_t        rvtype;         // type index of return value
    unsigned char   calltype;       // calling convention (CV_call_t)
    unsigned char   reserved;       // reserved for future use
    unsigned short  parmcount;      // number of parameters
    CV_typ_t        arglist;        // type index of argument list
} lfProc;



//      Type record for member function


typedef struct lfMFunc_16t {
    unsigned short  leaf;           // LF_MFUNCTION_16t
    CV_typ16_t      rvtype;         // type index of return value
    CV_typ16_t      classtype;      // type index of containing class
    CV_typ16_t      thistype;       // type index of this pointer (model specific)
    unsigned char   calltype;       // calling convention (call_t)
    unsigned char   reserved;       // reserved for future use
    unsigned short  parmcount;      // number of parameters
    CV_typ16_t      arglist;        // type index of argument list
    long            thisadjust;     // this adjuster (long because pad required anyway)
} lfMFunc_16t;

typedef struct lfMFunc {
    unsigned short  leaf;           // LF_MFUNCTION
    CV_typ_t        rvtype;         // type index of return value
    CV_typ_t        classtype;      // type index of containing class
    CV_typ_t        thistype;       // type index of this pointer (model specific)
    unsigned char   calltype;       // calling convention (call_t)
    unsigned char   reserved;       // reserved for future use
    unsigned short  parmcount;      // number of parameters
    CV_typ_t        arglist;        // type index of argument list
    long            thisadjust;     // this adjuster (long because pad required anyway)
} lfMFunc;




//     type record for virtual function table shape


typedef struct lfVTShape {
    unsigned short  leaf;       // LF_VTSHAPE
    unsigned short  count;      // number of entries in vfunctable
    unsigned char   desc[CV_ZEROLEN];     // 4 bit (CV_VTS_desc) descriptors
} lfVTShape;




//      type record for cobol0


typedef struct lfCobol0_16t {
    unsigned short  leaf;       // LF_COBOL0_16t
    CV_typ16_t      type;       // parent type record index
    unsigned char   data[CV_ZEROLEN];
} lfCobol0_16t;

typedef struct lfCobol0 {
    unsigned short  leaf;       // LF_COBOL0
    CV_typ_t        type;       // parent type record index
    unsigned char   data[CV_ZEROLEN];
} lfCobol0;




//      type record for cobol1


typedef struct lfCobol1 {
    unsigned short  leaf;       // LF_COBOL1
    unsigned char   data[CV_ZEROLEN];
} lfCobol1;




//      type record for basic array


typedef struct lfBArray_16t {
    unsigned short  leaf;       // LF_BARRAY_16t
    CV_typ16_t      utype;      // type index of underlying type
} lfBArray_16t;

typedef struct lfBArray {
    unsigned short  leaf;       // LF_BARRAY
    CV_typ_t        utype;      // type index of underlying type
} lfBArray;

//      type record for assembler labels


typedef struct lfLabel {
    unsigned short  leaf;       // LF_LABEL
    unsigned short  mode;       // addressing mode of label
} lfLabel;



//      type record for dimensioned arrays


typedef struct lfDimArray_16t {
    unsigned short  leaf;       // LF_DIMARRAY_16t
    CV_typ16_t      utype;      // underlying type of the array
    CV_typ16_t      diminfo;    // dimension information
    unsigned char   name[1];    // length prefixed name
} lfDimArray_16t;

typedef struct lfDimArray {
    unsigned short  leaf;       // LF_DIMARRAY
    CV_typ_t        utype;      // underlying type of the array
    CV_typ_t        diminfo;    // dimension information
    unsigned char   name[1];    // length prefixed name
} lfDimArray;



//      type record describing path to virtual function table


typedef struct lfVFTPath_16t {
    unsigned short  leaf;       // LF_VFTPATH_16t
    unsigned short  count;      // count of number of bases in path
    CV_typ16_t      base[1];    // bases from root to leaf
} lfVFTPath_16t;

typedef struct lfVFTPath {
    unsigned short  leaf;       // LF_VFTPATH
    unsigned long   count;      // count of number of bases in path
    CV_typ_t        base[1];    // bases from root to leaf
} lfVFTPath;


//      type record describing inclusion of precompiled types


typedef struct lfPreComp_16t {
    unsigned short  leaf;       // LF_PRECOMP_16t
    unsigned short  start;      // starting type index included
    unsigned short  count;      // number of types in inclusion
    unsigned long   signature;  // signature
    unsigned char   name[CV_ZEROLEN];     // length prefixed name of included type file
} lfPreComp_16t;

typedef struct lfPreComp {
    unsigned short  leaf;       // LF_PRECOMP
    unsigned long   start;      // starting type index included
    unsigned long   count;      // number of types in inclusion
    unsigned long   signature;  // signature
    unsigned char   name[CV_ZEROLEN];     // length prefixed name of included type file
} lfPreComp;



//      type record describing end of precompiled types that can be
//      included by another file


typedef struct lfEndPreComp {
    unsigned short  leaf;       // LF_ENDPRECOMP
    unsigned long   signature;  // signature
} lfEndPreComp;





//      type record for OEM definable type strings


typedef struct lfOEM_16t {
    unsigned short  leaf;       // LF_OEM_16t
    unsigned short  cvOEM;      // MS assigned OEM identified
    unsigned short  recOEM;     // OEM assigned type identifier
    unsigned short  count;      // count of type indices to follow
    CV_typ16_t      index[CV_ZEROLEN];  // array of type indices followed
                                // by OEM defined data
} lfOEM_16t;

typedef struct lfOEM {
    unsigned short  leaf;       // LF_OEM
    unsigned short  cvOEM;      // MS assigned OEM identified
    unsigned short  recOEM;     // OEM assigned type identifier
    unsigned long   count;      // count of type indices to follow
    CV_typ_t        index[CV_ZEROLEN];  // array of type indices followed
                                // by OEM defined data
} lfOEM;

#define OEM_MS_FORTRAN90        0xF090
#define OEM_ODI                 0x0010
#define OEM_THOMSON_SOFTWARE    0x5453
#define OEM_ODI_REC_BASELIST    0x0000


//      type record describing using of a type server

typedef struct lfTypeServer {
    unsigned short  leaf;       // LF_TYPESERVER
    unsigned long   signature;  // signature
    unsigned long   age;        // age of database used by this module
    unsigned char   name[CV_ZEROLEN];     // length prefixed name of PDB
} lfTypeServer;

//      description of type records that can be referenced from
//      type records referenced by symbols



//      type record for skip record


typedef struct lfSkip_16t {
    unsigned short  leaf;       // LF_SKIP_16t
    CV_typ16_t      type;       // next valid index
    unsigned char   data[CV_ZEROLEN];     // pad data
} lfSkip_16t;

typedef struct lfSkip {
    unsigned short  leaf;       // LF_SKIP
    CV_typ_t        type;       // next valid index
    unsigned char   data[CV_ZEROLEN];     // pad data
} lfSkip;



//      argument list leaf


typedef struct lfArgList_16t {
    unsigned short  leaf;           // LF_ARGLIST_16t
    unsigned short  count;          // number of arguments
    CV_typ16_t      arg[CV_ZEROLEN];      // number of arguments
} lfArgList_16t;

typedef struct lfArgList {
    unsigned short  leaf;           // LF_ARGLIST
    unsigned long   count;          // number of arguments
    CV_typ_t        arg[CV_ZEROLEN];      // number of arguments
} lfArgList;




//      derived class list leaf


typedef struct lfDerived_16t {
    unsigned short  leaf;           // LF_DERIVED_16t
    unsigned short  count;          // number of arguments
    CV_typ16_t      drvdcls[CV_ZEROLEN];      // type indices of derived classes
} lfDerived_16t;

typedef struct lfDerived {
    unsigned short  leaf;           // LF_DERIVED
    unsigned long   count;          // number of arguments
    CV_typ_t        drvdcls[CV_ZEROLEN];      // type indices of derived classes
} lfDerived;




//      leaf for default arguments


typedef struct lfDefArg_16t {
    unsigned short  leaf;               // LF_DEFARG_16t
    CV_typ16_t      type;               // type of resulting expression
    unsigned char   expr[CV_ZEROLEN];   // length prefixed expression string
} lfDefArg_16t;

typedef struct lfDefArg {
    unsigned short  leaf;               // LF_DEFARG
    CV_typ_t        type;               // type of resulting expression
    unsigned char   expr[CV_ZEROLEN];   // length prefixed expression string
} lfDefArg;



//      list leaf
//          This list should no longer be used because the utilities cannot
//          verify the contents of the list without knowing what type of list
//          it is.  New specific leaf indices should be used instead.


typedef struct lfList {
    unsigned short  leaf;           // LF_LIST
    char            data[CV_ZEROLEN];         // data format specified by indexing type
} lfList;




//      field list leaf
//      This is the header leaf for a complex list of class and structure
//      subfields.


typedef struct lfFieldList_16t {
    unsigned short  leaf;           // LF_FIELDLIST_16t
    char            data[CV_ZEROLEN];         // field list sub lists
} lfFieldList_16t;


typedef struct lfFieldList {
    unsigned short  leaf;           // LF_FIELDLIST
    char            data[CV_ZEROLEN];         // field list sub lists
} lfFieldList;







//  type record for non-static methods and friends in overloaded method list

typedef struct mlMethod_16t {
    CV_fldattr_t   attr;           // method attribute
    CV_typ16_t     index;          // index to type record for procedure
    unsigned long  vbaseoff[CV_ZEROLEN];    // offset in vfunctable if intro virtual
} mlMethod_16t;

typedef struct mlMethod {
    CV_fldattr_t    attr;           // method attribute
    _2BYTEPAD       pad0;           // internal padding, must be 0
    CV_typ_t        index;          // index to type record for procedure
    unsigned long   vbaseoff[CV_ZEROLEN];    // offset in vfunctable if intro virtual
} mlMethod;


typedef struct lfMethodList_16t {
    unsigned short leaf;
    unsigned char  mList[CV_ZEROLEN];         // really a mlMethod_16t type
} lfMethodList_16t;

typedef struct lfMethodList {
    unsigned short leaf;
    unsigned char  mList[CV_ZEROLEN];         // really a mlMethod type
} lfMethodList;





//      type record for LF_BITFIELD


typedef struct lfBitfield_16t {
    unsigned short  leaf;           // LF_BITFIELD_16t
    unsigned char   length;
    unsigned char   position;
    CV_typ16_t      type;           // type of bitfield

} lfBitfield_16t;

typedef struct lfBitfield {
    unsigned short  leaf;           // LF_BITFIELD
    CV_typ_t        type;           // type of bitfield
    unsigned char   length;
    unsigned char   position;

} lfBitfield;




//      type record for dimensioned array with constant bounds


typedef struct lfDimCon_16t {
    unsigned short  leaf;           // LF_DIMCONU_16t or LF_DIMCONLU_16t
    unsigned short  rank;           // number of dimensions
    CV_typ16_t      typ;            // type of index
    unsigned char   dim[CV_ZEROLEN];          // array of dimension information with
                                    // either upper bounds or lower/upper bound
} lfDimCon_16t;

typedef struct lfDimCon {
    unsigned short  leaf;           // LF_DIMCONU or LF_DIMCONLU
    CV_typ_t        typ;            // type of index
    unsigned short  rank;           // number of dimensions
    unsigned char   dim[CV_ZEROLEN];          // array of dimension information with
                                    // either upper bounds or lower/upper bound
} lfDimCon;




//      type record for dimensioned array with variable bounds


typedef struct lfDimVar_16t {
    unsigned short  leaf;           // LF_DIMVARU_16t or LF_DIMVARLU_16t
    unsigned short  rank;           // number of dimensions
    CV_typ16_t      typ;            // type of index
    unsigned char   dim[CV_ZEROLEN];          // array of type indices for either
                                    // variable upper bound or variable
                                    // lower/upper bound.  The referenced
                                    // types must be LF_REFSYM or T_VOID
} lfDimVar_16t;

typedef struct lfDimVar {
    unsigned short  leaf;           // LF_DIMVARU or LF_DIMVARLU
    unsigned long   rank;           // number of dimensions
    CV_typ_t        typ;            // type of index
    CV_typ_t        dim[CV_ZEROLEN];          // array of type indices for either
                                    // variable upper bound or variable
                                    // lower/upper bound.  The count of type
                                    // indices is rank or rank*2 depending on
                                    // whether it is LFDIMVARU or LF_DIMVARLU.
                                    // The referenced types must be
                                    // LF_REFSYM or T_VOID
} lfDimVar;




//      type record for referenced symbol


typedef struct lfRefSym {
    unsigned short  leaf;           // LF_REFSYM
    unsigned char   Sym[1];         // copy of referenced symbol record
                                    // (including length)
} lfRefSym;





/**     the following are numeric leaves.  They are used to indicate the
 *      size of the following variable length data.  When the numeric
 *      data is a single byte less than 0x8000, then the data is output
 *      directly.  If the data is more the 0x8000 or is a negative value,
 *      then the data is preceeded by the proper index.
 */



//      signed character leaf

typedef struct lfChar {
    unsigned short  leaf;           // LF_CHAR
    signed char     val;            // signed 8-bit value
} lfChar;




//      signed short leaf

typedef struct lfShort {
    unsigned short  leaf;           // LF_SHORT
    short           val;            // signed 16-bit value
} lfShort;




//      unsigned short leaf

typedef struct lfUShort {
    unsigned short  leaf;           // LF_unsigned short
    unsigned short  val;            // unsigned 16-bit value
} lfUShort;




//      signed long leaf

typedef struct lfLong {
    unsigned short  leaf;           // LF_LONG
    long            val;            // signed 32-bit value
} lfLong;




//      unsigned long leaf

typedef struct lfULong {
    unsigned short  leaf;           // LF_ULONG
    unsigned long   val;            // unsigned 32-bit value
} lfULong;




//      signed quad leaf

typedef struct lfQuad {
    unsigned short  leaf;           // LF_QUAD
    unsigned char   val[8];         // signed 64-bit value
} lfQuad;




//      unsigned quad leaf

typedef struct lfUQuad {
    unsigned short  leaf;           // LF_UQUAD
    unsigned char   val[8];         // unsigned 64-bit value
} lfUQuad;


//      signed int128 leaf

typedef struct lfOct {
    unsigned short  leaf;           // LF_OCT
    unsigned char   val[16];        // signed 128-bit value
} lfOct;

//      unsigned int128 leaf

typedef struct lfUOct {
    unsigned short  leaf;           // LF_UOCT
    unsigned char   val[16];        // unsigned 128-bit value
} lfUOct;




//      real 32-bit leaf

typedef struct lfReal32 {
    unsigned short  leaf;           // LF_REAL32
    float           val;            // 32-bit real value
} lfReal32;




//      real 48-bit leaf

typedef struct lfReal48 {
    unsigned short  leaf;           // LF_REAL48
    unsigned char   val[6];         // 48-bit real value
} lfReal48;




//      real 64-bit leaf

typedef struct lfReal64 {
    unsigned short  leaf;           // LF_REAL64
    double          val;            // 64-bit real value
} lfReal64;




//      real 80-bit leaf

typedef struct lfReal80 {
    unsigned short  leaf;           // LF_REAL80
    FLOAT10         val;            // real 80-bit value
} lfReal80;




//      real 128-bit leaf

typedef struct lfReal128 {
    unsigned short  leaf;           // LF_REAL128
    char            val[16];        // real 128-bit value
} lfReal128;




//      complex 32-bit leaf

typedef struct lfCmplx32 {
    unsigned short  leaf;           // LF_COMPLEX32
    float           val_real;       // real component
    float           val_imag;       // imaginary component
} lfCmplx32;




//      complex 64-bit leaf

typedef struct lfCmplx64 {
    unsigned short  leaf;           // LF_COMPLEX64
    double          val_real;       // real component
    double          val_imag;       // imaginary component
} flCmplx64;




//      complex 80-bit leaf

typedef struct lfCmplx80 {
    unsigned short  leaf;           // LF_COMPLEX80
    FLOAT10         val_real;       // real component
    FLOAT10         val_imag;       // imaginary component
} lfCmplx80;




//      complex 128-bit leaf

typedef struct lfCmplx128 {
    unsigned short  leaf;           // LF_COMPLEX128
    char            val_real[16];   // real component
    char            val_imag[16];   // imaginary component
} lfCmplx128;



//  variable length numeric field

typedef struct lfVarString {
    unsigned short  leaf;       // LF_VARSTRING
    unsigned short  len;        // length of value in bytes
    unsigned char   value[CV_ZEROLEN];  // value
} lfVarString;

//***********************************************************************


//      index leaf - contains type index of another leaf
//      a major use of this leaf is to allow the compilers to emit a
//      long complex list (LF_FIELD) in smaller pieces.

typedef struct lfIndex_16t {
    unsigned short  leaf;           // LF_INDEX_16t
    CV_typ16_t      index;          // type index of referenced leaf
} lfIndex_16t;

typedef struct lfIndex {
    unsigned short  leaf;           // LF_INDEX
    _2BYTEPAD       pad0;           // internal padding, must be 0
    CV_typ_t        index;          // type index of referenced leaf
} lfIndex;


//      subfield record for base class field

typedef struct lfBClass_16t {
    unsigned short  leaf;           // LF_BCLASS_16t
    CV_typ16_t      index;          // type index of base class
    CV_fldattr_t    attr;           // attribute
    unsigned char   offset[CV_ZEROLEN];       // variable length offset of base within class
} lfBClass_16t;

typedef struct lfBClass {
    unsigned short  leaf;           // LF_BCLASS
    CV_fldattr_t    attr;           // attribute
    CV_typ_t        index;          // type index of base class
    unsigned char   offset[CV_ZEROLEN];       // variable length offset of base within class
} lfBClass;





//      subfield record for direct and indirect virtual base class field

typedef struct lfVBClass_16t {
    unsigned short  leaf;           // LF_VBCLASS_16t | LV_IVBCLASS_16t
    CV_typ16_t      index;          // type index of direct virtual base class
    CV_typ16_t      vbptr;          // type index of virtual base pointer
    CV_fldattr_t    attr;           // attribute
    unsigned char   vbpoff[CV_ZEROLEN];       // virtual base pointer offset from address point
                                    // followed by virtual base offset from vbtable
} lfVBClass_16t;

typedef struct lfVBClass {
    unsigned short  leaf;           // LF_VBCLASS | LV_IVBCLASS
    CV_fldattr_t    attr;           // attribute
    CV_typ_t        index;          // type index of direct virtual base class
    CV_typ_t        vbptr;          // type index of virtual base pointer
    unsigned char   vbpoff[CV_ZEROLEN];       // virtual base pointer offset from address point
                                    // followed by virtual base offset from vbtable
} lfVBClass;





//      subfield record for friend class


typedef struct lfFriendCls_16t {
    unsigned short  leaf;           // LF_FRIENDCLS_16t
    CV_typ16_t      index;          // index to type record of friend class
} lfFriendCls_16t;

typedef struct lfFriendCls {
    unsigned short  leaf;           // LF_FRIENDCLS
    _2BYTEPAD       pad0;           // internal padding, must be 0
    CV_typ_t        index;          // index to type record of friend class
} lfFriendCls;





//      subfield record for friend function


typedef struct lfFriendFcn_16t {
    unsigned short  leaf;           // LF_FRIENDFCN_16t
    CV_typ16_t      index;          // index to type record of friend function
    unsigned char   Name[1];        // name of friend function
} lfFriendFcn_16t;

typedef struct lfFriendFcn {
    unsigned short  leaf;           // LF_FRIENDFCN
    _2BYTEPAD       pad0;           // internal padding, must be 0
    CV_typ_t        index;          // index to type record of friend function
    unsigned char   Name[1];        // name of friend function
} lfFriendFcn;



//      subfield record for non-static data members

typedef struct lfMember_16t {
    unsigned short  leaf;           // LF_MEMBER_16t
    CV_typ16_t      index;          // index of type record for field
    CV_fldattr_t    attr;           // attribute mask
    unsigned char   offset[CV_ZEROLEN];       // variable length offset of field followed
                                    // by length prefixed name of field
} lfMember_16t;

typedef struct lfMember {
    unsigned short  leaf;           // LF_MEMBER
    CV_fldattr_t    attr;           // attribute mask
    CV_typ_t        index;          // index of type record for field
    unsigned char   offset[CV_ZEROLEN];       // variable length offset of field followed
                                    // by length prefixed name of field
} lfMember;



//  type record for static data members

typedef struct lfSTMember_16t {
    unsigned short  leaf;           // LF_STMEMBER_16t
    CV_typ16_t      index;          // index of type record for field
    CV_fldattr_t    attr;           // attribute mask
    unsigned char   Name[1];        // length prefixed name of field
} lfSTMember_16t;

typedef struct lfSTMember {
    unsigned short  leaf;           // LF_STMEMBER
    CV_fldattr_t    attr;           // attribute mask
    CV_typ_t        index;          // index of type record for field
    unsigned char   Name[1];        // length prefixed name of field
} lfSTMember;



//      subfield record for virtual function table pointer

typedef struct lfVFuncTab_16t {
    unsigned short  leaf;           // LF_VFUNCTAB_16t
    CV_typ16_t      type;           // type index of pointer
} lfVFuncTab_16t;

typedef struct lfVFuncTab {
    unsigned short  leaf;           // LF_VFUNCTAB
    _2BYTEPAD       pad0;           // internal padding, must be 0
    CV_typ_t        type;           // type index of pointer
} lfVFuncTab;



//      subfield record for virtual function table pointer with offset

typedef struct lfVFuncOff_16t {
    unsigned short  leaf;           // LF_VFUNCOFF_16t
    CV_typ16_t      type;           // type index of pointer
    CV_off32_t      offset;         // offset of virtual function table pointer
} lfVFuncOff_16t;

typedef struct lfVFuncOff {
    unsigned short  leaf;           // LF_VFUNCOFF
    _2BYTEPAD       pad0;           // internal padding, must be 0.
    CV_typ_t        type;           // type index of pointer
    CV_off32_t      offset;         // offset of virtual function table pointer
} lfVFuncOff;



//      subfield record for overloaded method list


typedef struct lfMethod_16t {
    unsigned short  leaf;           // LF_METHOD_16t
    unsigned short  count;          // number of occurrences of function
    CV_typ16_t      mList;          // index to LF_METHODLIST record
    unsigned char   Name[1];        // length prefixed name of method
} lfMethod_16t;

typedef struct lfMethod {
    unsigned short  leaf;           // LF_METHOD
    unsigned short  count;          // number of occurrences of function
    CV_typ_t        mList;          // index to LF_METHODLIST record
    unsigned char   Name[1];        // length prefixed name of method
} lfMethod;



//      subfield record for nonoverloaded method


typedef struct lfOneMethod_16t {
    unsigned short leaf;            // LF_ONEMETHOD_16t
    CV_fldattr_t   attr;            // method attribute
    CV_typ16_t     index;           // index to type record for procedure
    unsigned long  vbaseoff[CV_ZEROLEN];    // offset in vfunctable if
                                    // intro virtual followed by
                                    // length prefixed name of method
} lfOneMethod_16t;

typedef struct lfOneMethod {
    unsigned short leaf;            // LF_ONEMETHOD
    CV_fldattr_t   attr;            // method attribute
    CV_typ_t       index;           // index to type record for procedure
    unsigned long  vbaseoff[CV_ZEROLEN];    // offset in vfunctable if
                                    // intro virtual followed by
                                    // length prefixed name of method
} lfOneMethod;


//      subfield record for enumerate

typedef struct lfEnumerate {
    unsigned short  leaf;       // LF_ENUMERATE
    CV_fldattr_t    attr;       // access
    unsigned char   value[CV_ZEROLEN];    // variable length value field followed
                                // by length prefixed name
} lfEnumerate;


//  type record for nested (scoped) type definition

typedef struct lfNestType_16t {
    unsigned short  leaf;       // LF_NESTTYPE_16t
    CV_typ16_t      index;      // index of nested type definition
    unsigned char   Name[1];    // length prefixed type name
} lfNestType_16t;

typedef struct lfNestType {
    unsigned short  leaf;       // LF_NESTTYPE
    _2BYTEPAD       pad0;       // internal padding, must be 0
    CV_typ_t        index;      // index of nested type definition
    unsigned char   Name[1];    // length prefixed type name
} lfNestType;

//  type record for nested (scoped) type definition, with attributes
//  new records for vC v5.0, no need to have 16-bit ti versions.

typedef struct lfNestTypeEx {
    unsigned short  leaf;       // LF_NESTTYPEEX
    CV_fldattr_t    attr;       // member access
    CV_typ_t        index;      // index of nested type definition
    unsigned char   Name[1];    // length prefixed type name
} lfNestTypeEx;

//  type record for modifications to members

typedef struct lfMemberModify {
    unsigned short  leaf;       // LF_MEMBERMODIFY
    CV_fldattr_t    attr;       // the new attributes
    CV_typ_t        index;      // index of base class type definition
    unsigned char   Name[1];    // length prefixed member name
} lfMemberModify;

//  type record for pad leaf

typedef struct lfPad {
    unsigned char   leaf;
} SYM_PAD;



//  Symbol definitions

typedef enum SYM_ENUM_e {
    S_COMPILE       =  0x0001, // Compile flags symbol
    S_REGISTER_16t  =  0x0002, // Register variable
    S_CONSTANT_16t  =  0x0003, // constant symbol
    S_UDT_16t       =  0x0004, // User defined type
    S_SSEARCH       =  0x0005, // Start Search
    S_END           =  0x0006, // Block, procedure, "with" or thunk end
    S_SKIP          =  0x0007, // Reserve symbol space in $$Symbols table
    S_CVRESERVE     =  0x0008, // Reserved symbol for CV internal use
    S_OBJNAME       =  0x0009, // path to object file name
    S_ENDARG        =  0x000a, // end of argument/return list
    S_COBOLUDT_16t  =  0x000b, // special UDT for cobol that does not symbol pack
    S_MANYREG_16t   =  0x000c, // multiple register variable
    S_RETURN        =  0x000d, // return description symbol
    S_ENTRYTHIS     =  0x000e, // description of this pointer on entry
                       
    S_BPREL16       =  0x0100, // BP-relative
    S_LDATA16       =  0x0101, // Module-local symbol
    S_GDATA16       =  0x0102, // Global data symbol
    S_PUB16         =  0x0103, // a public symbol
    S_LPROC16       =  0x0104, // Local procedure start
    S_GPROC16       =  0x0105, // Global procedure start
    S_THUNK16       =  0x0106, // Thunk Start
    S_BLOCK16       =  0x0107, // block start
    S_WITH16        =  0x0108, // with start
    S_LABEL16       =  0x0109, // code label
    S_CEXMODEL16    =  0x010a, // change execution model
    S_VFTABLE16     =  0x010b, // address of virtual function table
    S_REGREL16      =  0x010c, // register relative address
                       
    S_BPREL32_16t   =  0x0200, // BP-relative
    S_LDATA32_16t   =  0x0201, // Module-local symbol
    S_GDATA32_16t   =  0x0202, // Global data symbol
    S_PUB32_16t     =  0x0203, // a public symbol (CV internal reserved)
    S_LPROC32_16t   =  0x0204, // Local procedure start
    S_GPROC32_16t   =  0x0205, // Global procedure start
    S_THUNK32       =  0x0206, // Thunk Start
    S_BLOCK32       =  0x0207, // block start
    S_WITH32        =  0x0208, // with start
    S_LABEL32       =  0x0209, // code label
    S_CEXMODEL32    =  0x020a, // change execution model
    S_VFTABLE32_16t =  0x020b, // address of virtual function table
    S_REGREL32_16t  =  0x020c, // register relative address
    S_LTHREAD32_16t =  0x020d, // local thread storage
    S_GTHREAD32_16t =  0x020e, // global thread storage
    S_SLINK32       =  0x020f, // static link for MIPS EH implementation
                       
    S_LPROCMIPS_16t =  0x0300, // Local procedure start
    S_GPROCMIPS_16t =  0x0301, // Global procedure start
                       
    S_PROCREF       =  0x0400, // Reference to a procedure
    S_DATAREF       =  0x0401, // Reference to data
    S_ALIGN         =  0x0402, // Used for page alignment of symbols
    S_LPROCREF      =  0x0403, // Local Reference to a procedure

    // sym records with 32-bit types embedded instead of 16-bit
    // all have 0x1000 bit set for easy identification
    // only do the 32-bit target versions since we don't really
    // care about 16-bit ones anymore.
S_TI16_MAX          =  0x1000,
    S_REGISTER      =  0x1001, // Register variable
    S_CONSTANT      =  0x1002, // constant symbol
    S_UDT           =  0x1003, // User defined type
    S_COBOLUDT      =  0x1004, // special UDT for cobol that does not symbol pack
    S_MANYREG       =  0x1005, // multiple register variable
    S_BPREL32       =  0x1006, // BP-relative
    S_LDATA32       =  0x1007, // Module-local symbol
    S_GDATA32       =  0x1008, // Global data symbol
    S_PUB32         =  0x1009, // a public symbol (CV internal reserved)
    S_LPROC32       =  0x100a, // Local procedure start
    S_GPROC32       =  0x100b, // Global procedure start
    S_VFTABLE32     =  0x100c, // address of virtual function table
    S_REGREL32      =  0x100d, // register relative address
    S_LTHREAD32     =  0x100e, // local thread storage
    S_GTHREAD32     =  0x100f, // global thread storage

    S_LPROCMIPS     =  0x1010, // Local procedure start
    S_GPROCMIPS     =  0x1011, // Global procedure start
                       
} SYM_ENUM_e;




//  enum describing the compile flag source language


typedef enum CV_CFL_LANG {
    CV_CFL_C        = 0x00,
    CV_CFL_CXX      = 0x01,
    CV_CFL_FORTRAN  = 0x02,
    CV_CFL_MASM     = 0x03,
    CV_CFL_PASCAL   = 0x04,
    CV_CFL_BASIC    = 0x05,
    CV_CFL_COBOL    = 0x06,
    CV_CFL_LINK     = 0x07,
    CV_CFL_CVTRES   = 0x08,
} CV_CFL_LANG;



//  enum describing target processor


typedef enum CV_CPU_TYPE_e {
    CV_CFL_8080         = 0x00,
    CV_CFL_8086         = 0x01,
    CV_CFL_80286        = 0x02,
    CV_CFL_80386        = 0x03,
    CV_CFL_80486        = 0x04,
    CV_CFL_PENTIUM      = 0x05,
    CV_CFL_PENTIUMPRO   = 0x06,
    CV_CFL_MIPSR4000    = 0x10,
    CV_CFL_M68000       = 0x20,
    CV_CFL_M68010       = 0x21,
    CV_CFL_M68020       = 0x22,
    CV_CFL_M68030       = 0x23,
    CV_CFL_M68040       = 0x24,
    CV_CFL_ALPHA        = 0x30,
    CV_CFL_PPC601       = 0x40,
    CV_CFL_PPC603       = 0x41,
    CV_CFL_PPC604       = 0x42,
    CV_CFL_PPC620       = 0x43

} CV_CPU_TYPE_e;




//  enum describing compile flag ambient data model


typedef enum CV_CFL_DATA {
    CV_CFL_DNEAR    = 0x00,
    CV_CFL_DFAR     = 0x01,
    CV_CFL_DHUGE    = 0x02
} CV_CFL_DATA;




//  enum describing compile flag ambiant code model


typedef enum CV_CFL_CODE_e {
    CV_CFL_CNEAR    = 0x00,
    CV_CFL_CFAR     = 0x01,
    CV_CFL_CHUGE    = 0x02
} CV_CFL_CODE_e;




//  enum describing compile flag target floating point package

typedef enum CV_CFL_FPKG_e {
    CV_CFL_NDP      = 0x00,
    CV_CFL_EMU      = 0x01,
    CV_CFL_ALT      = 0x02
} CV_CFL_FPKG_e;


// enum describing function return method


typedef struct CV_PROCFLAGS {
    union {
        unsigned char   bAll;
        struct {
            unsigned char CV_PFLAG_NOFPO:1; // frame pointer present
            unsigned char CV_PFLAG_INT  :1; // interrupt return
            unsigned char CV_PFLAG_FAR  :1; // far return
            unsigned char CV_PFLAG_NEVER:1; // function does not return
            unsigned char unused        :4; //
        };
    };
} CV_PROCFLAGS;


// enum describing function data return method

typedef enum CV_GENERIC_STYLE_e {
    CV_GENERIC_VOID   = 0x00,       // void return type
    CV_GENERIC_REG    = 0x01,       // return data is in registers
    CV_GENERIC_ICAN   = 0x02,       // indirect caller allocated near
    CV_GENERIC_ICAF   = 0x03,       // indirect caller allocated far
    CV_GENERIC_IRAN   = 0x04,       // indirect returnee allocated near
    CV_GENERIC_IRAF   = 0x05,       // indirect returnee allocated far
    CV_GENERIC_UNUSED = 0x06        // first unused
} CV_GENERIC_STYLE_e;


typedef struct CV_GENERIC_FLAG {
    unsigned short  cstyle  :1;     // true push varargs right to left
    unsigned short  rsclean :1;     // true if returnee stack cleanup
    unsigned short  unused  :14;    // unused
} CV_GENERIC_FLAG;





typedef struct SYMTYPE {
    unsigned short      reclen;     // Record length
    unsigned short      rectyp;     // Record type
    char                data[CV_ZEROLEN];
} SYMTYPE;

__INLINE SYMTYPE *NextSym (SYMTYPE * pSym) {
    return (SYMTYPE *) ((char *)pSym + pSym->reclen + sizeof(unsigned short));
}

//      non-model specific symbol types



typedef struct REGSYM_16t {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_REGISTER_16t
    CV_typ16_t      typind;     // Type index
    unsigned short  reg;        // register enumerate
    unsigned char   name[1];    // Length-prefixed name
} REGSYM_16t;

typedef struct REGSYM {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_REGISTER
    CV_typ_t        typind;     // Type index
    unsigned short  reg;        // register enumerate
    unsigned char   name[1];    // Length-prefixed name
} REGSYM;



typedef struct MANYREGSYM_16t {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_MANYREG_16t
    CV_typ16_t      typind;     // Type index
    unsigned char   count;      // count of number of registers
    unsigned char   reg[1];     // count register enumerates followed by
                                // length-prefixed name.  Registers are
                                // most significant first.
} MANYREGSYM_16t;

typedef struct MANYREGSYM {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_MANYREG
    CV_typ_t        typind;     // Type index
    unsigned char   count;      // count of number of registers
    unsigned char   reg[1];     // count register enumerates followed by
                                // length-prefixed name.  Registers are
                                // most significant first.
} MANYREGSYM;



typedef struct CONSTSYM_16t {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_CONSTANT_16t
    CV_typ16_t      typind;     // Type index (containing enum if enumerate)
    unsigned short  value;      // numeric leaf containing value
    unsigned char   name[CV_ZEROLEN];     // Length-prefixed name
} CONSTSYM_16t;

typedef struct CONSTSYM {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_CONSTANT
    CV_typ_t        typind;     // Type index (containing enum if enumerate)
    unsigned short  value;      // numeric leaf containing value
    unsigned char   name[CV_ZEROLEN];     // Length-prefixed name
} CONSTSYM;


typedef struct UDTSYM_16t {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_UDT_16t | S_COBOLUDT_16t
    CV_typ16_t      typind;     // Type index
    unsigned char   name[1];    // Length-prefixed name
} UDTSYM_16t;

typedef struct UDTSYM {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_UDT | S_COBOLUDT
    CV_typ_t        typind;     // Type index
    unsigned char   name[1];    // Length-prefixed name
} UDTSYM;

typedef struct SEARCHSYM {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_SSEARCH
    unsigned long   startsym;   // offset of the procedure
    unsigned short  seg;        // segment of symbol
} SEARCHSYM;

typedef struct CFLAGSYM {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_COMPILE
    unsigned char   machine;    // target processor
    struct  {
        unsigned char   language    :8; // language index
        unsigned char   pcode       :1; // true if pcode present
        unsigned char   floatprec   :2; // floating precision
        unsigned char   floatpkg    :2; // float package
        unsigned char   ambdata     :3; // ambient data model
        unsigned char   ambcode     :3; // ambient code model
        unsigned char   mode32      :1; // true if compiled 32 bit mode
        unsigned char   pad         :4; // reserved
    } flags;
    unsigned char       ver[1];     // Length-prefixed compiler version string
} CFLAGSYM;





typedef struct OBJNAMESYM {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_OBJNAME
    unsigned long   signature;  // signature
    unsigned char   name[1];    // Length-prefixed name
} OBJNAMESYM;




typedef struct ENDARGSYM {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_ENDARG
} ENDARGSYM;


typedef struct RETURNSYM {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_RETURN
    CV_GENERIC_FLAG flags;      // flags
    CV_GENERIC_STYLE_e style;   // return style
                                // followed by return method data
} RETURNSYM;


typedef struct ENTRYTHISSYM {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_ENTRYTHIS
    unsigned char   thissym;    // symbol describing this pointer on entry
} ENTRYTHISSYM;


//      symbol types for 16:16 memory model


typedef struct BPRELSYM16 {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_BPREL16
    CV_off16_t      off;        // BP-relative offset
    CV_typ16_t      typind;     // Type index
    unsigned char   name[1];    // Length-prefixed name
} BPRELSYM16;



typedef struct DATASYM16 {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_LDATA or S_GDATA
    CV_uoff16_t     off;        // offset of symbol
    unsigned short  seg;        // segment of symbol
    CV_typ16_t      typind;     // Type index
    unsigned char   name[1];    // Length-prefixed name
} DATASYM16;
typedef DATASYM16 PUBSYM16;


typedef struct PROCSYM16 {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_GPROC16 or S_LPROC16
    unsigned long   pParent;    // pointer to the parent
    unsigned long   pEnd;       // pointer to this blocks end
    unsigned long   pNext;      // pointer to next symbol
    unsigned short  len;        // Proc length
    unsigned short  DbgStart;   // Debug start offset
    unsigned short  DbgEnd;     // Debug end offset
    CV_uoff16_t     off;        // offset of symbol
    unsigned short  seg;        // segment of symbol
    CV_typ16_t      typind;     // Type index
    CV_PROCFLAGS    flags;      // Proc flags
    unsigned char   name[1];    // Length-prefixed name
} PROCSYM16;




typedef struct THUNKSYM16 {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_THUNK
    unsigned long   pParent;    // pointer to the parent
    unsigned long   pEnd;       // pointer to this blocks end
    unsigned long   pNext;      // pointer to next symbol
    CV_uoff16_t     off;        // offset of symbol
    unsigned short  seg;        // segment of symbol
    unsigned short  len;        // length of thunk
    unsigned char   ord;        // ordinal specifying type of thunk
    unsigned char   name[1];    // name of thunk
    unsigned char   variant[CV_ZEROLEN]; // variant portion of thunk
} THUNKSYM16;

typedef enum {
    THUNK_ORDINAL_NOTYPE,
    THUNK_ORDINAL_ADJUSTOR,
    THUNK_ORDINAL_VCALL,
    THUNK_ORDINAL_PCODE
} THUNK_ORDINAL;

typedef struct LABELSYM16 {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_LABEL16
    CV_uoff16_t     off;        // offset of symbol
    unsigned short  seg;        // segment of symbol
    CV_PROCFLAGS    flags;      // flags
    unsigned char   name[1];    // Length-prefixed name
} LABELSYM16;

typedef struct BLOCKSYM16 {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_BLOCK16
    unsigned long   pParent;    // pointer to the parent
    unsigned long   pEnd;       // pointer to this blocks end
    unsigned short  len;        // Block length
    CV_uoff16_t     off;        // offset of symbol
    unsigned short  seg;        // segment of symbol
    unsigned char   name[1];    // Length-prefixed name
} BLOCKSYM16;

typedef struct WITHSYM16 {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_WITH16
    unsigned long   pParent;    // pointer to the parent
    unsigned long   pEnd;       // pointer to this blocks end
    unsigned short  len;        // Block length
    CV_uoff16_t     off;        // offset of symbol
    unsigned short  seg;        // segment of symbol
    unsigned char   expr[1];    // Length-prefixed expression
} WITHSYM16;




typedef enum CEXM_MODEL_e {
    CEXM_MDL_table          = 0x00, // not executable
    CEXM_MDL_jumptable      = 0x01, // Compiler generated jump table
    CEXM_MDL_datapad        = 0x02, // Data padding for alignment
    CEXM_MDL_native         = 0x20, // native (actually not-pcode)
    CEXM_MDL_cobol          = 0x21, // cobol
    CEXM_MDL_codepad        = 0x22, // Code padding for alignment
    CEXM_MDL_code           = 0x23, // code
    CEXM_MDL_sql            = 0x30, // sql
    CEXM_MDL_pcode          = 0x40, // pcode
    CEXM_MDL_pcode32Mac     = 0x41, // macintosh 32 bit pcode
    CEXM_MDL_pcode32MacNep  = 0x42, // macintosh 32 bit pcode native entry point
    CEXM_MDL_javaInt        = 0x50,
    CEXM_MDL_unknown        = 0xff
} CEXM_MODEL_e;

// use the correct enumerate name
#define CEXM_MDL_SQL CEXM_MDL_sql

typedef enum CV_COBOL_e {
    CV_COBOL_dontstop,
    CV_COBOL_pfm,
    CV_COBOL_false,
    CV_COBOL_extcall
} CV_COBOL_e;

typedef struct CEXMSYM16 {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_CEXMODEL16
    CV_uoff16_t     off;        // offset of symbol
    unsigned short  seg;        // segment of symbol
    unsigned short  model;      // execution model
    union var16 {
        struct  {
            CV_uoff16_t pcdtable;   // offset to pcode function table
            CV_uoff16_t pcdspi;     // offset to segment pcode information
        } pcode;
        struct {
            unsigned short  subtype;   // see CV_COBOL_e above
            unsigned short  flag;
        } cobol;
    };
} CEXMSYM16;




typedef struct VPATHSYM16 {
    unsigned short  reclen;     // record length
    unsigned short  rectyp;     // S_VFTPATH16
    CV_uoff16_t     off;        // offset of virtual function table
    unsigned short  seg;        // segment of virtual function table
    CV_typ16_t      root;       // type index of the root of path
    CV_typ16_t      path;       // type index of the path record
} VPATHSYM16;




typedef struct REGREL16 {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_REGREL16
    CV_uoff16_t     off;        // offset of symbol
    unsigned short  reg;        // register index
    CV_typ16_t      typind;     // Type index
    unsigned char   name[1];    // Length-prefixed name
} REGREL16;





typedef struct BPRELSYM32_16t {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_BPREL32_16t
    CV_off32_t      off;        // BP-relative offset
    CV_typ16_t      typind;     // Type index
    unsigned char   name[1];    // Length-prefixed name
} BPRELSYM32_16t;

typedef struct BPRELSYM32 {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_BPREL32
    CV_off32_t      off;        // BP-relative offset
    CV_typ_t        typind;     // Type index
    unsigned char   name[1];    // Length-prefixed name
} BPRELSYM32;

typedef struct DATASYM32_16t {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_LDATA32_16t, S_GDATA32_16t or S_PUB32_16t
    CV_uoff32_t     off;
    unsigned short  seg;
    CV_typ16_t      typind;     // Type index
    unsigned char   name[1];    // Length-prefixed name
} DATASYM32_16t;
typedef DATASYM32_16t PUBSYM32_16t;

typedef struct DATASYM32 {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_LDATA32, S_GDATA32 or S_PUB32
    CV_typ_t        typind;     // Type index
    CV_uoff32_t     off;
    unsigned short  seg;
    unsigned char   name[1];    // Length-prefixed name
} DATASYM32;
typedef DATASYM32 PUBSYM32;



typedef struct PROCSYM32_16t {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_GPROC32_16t or S_LPROC32_16t
    unsigned long   pParent;    // pointer to the parent
    unsigned long   pEnd;       // pointer to this blocks end
    unsigned long   pNext;      // pointer to next symbol
    unsigned long   len;        // Proc length
    unsigned long   DbgStart;   // Debug start offset
    unsigned long   DbgEnd;     // Debug end offset
    CV_uoff32_t     off;
    unsigned short  seg;
    CV_typ16_t      typind;     // Type index
    CV_PROCFLAGS    flags;      // Proc flags
    unsigned char   name[1];    // Length-prefixed name
} PROCSYM32_16t;

typedef struct PROCSYM32 {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_GPROC32 or S_LPROC32
    unsigned long   pParent;    // pointer to the parent
    unsigned long   pEnd;       // pointer to this blocks end
    unsigned long   pNext;      // pointer to next symbol
    unsigned long   len;        // Proc length
    unsigned long   DbgStart;   // Debug start offset
    unsigned long   DbgEnd;     // Debug end offset
    CV_typ_t        typind;     // Type index
    CV_uoff32_t     off;
    unsigned short  seg;
    CV_PROCFLAGS    flags;      // Proc flags
    unsigned char   name[1];    // Length-prefixed name
} PROCSYM32;




typedef struct THUNKSYM32 {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_THUNK32
    unsigned long   pParent;    // pointer to the parent
    unsigned long   pEnd;       // pointer to this blocks end
    unsigned long   pNext;      // pointer to next symbol
    CV_uoff32_t     off;
    unsigned short  seg;
    unsigned short  len;        // length of thunk
    unsigned char   ord;        // ordinal specifying type of thunk
    unsigned char   name[1];    // Length-prefixed name
    unsigned char   variant[CV_ZEROLEN]; // variant portion of thunk
} THUNKSYM32;




typedef struct LABELSYM32 {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_LABEL32
    CV_uoff32_t     off;
    unsigned short  seg;
    CV_PROCFLAGS    flags;      // flags
    unsigned char   name[1];    // Length-prefixed name
} LABELSYM32;


typedef struct BLOCKSYM32 {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_BLOCK32
    unsigned long   pParent;    // pointer to the parent
    unsigned long   pEnd;       // pointer to this blocks end
    unsigned long   len;        // Block length
    CV_uoff32_t     off;        // Offset in code segment
    unsigned short  seg;        // segment of label
    unsigned char   name[1];    // Length-prefixed name
} BLOCKSYM32;


typedef struct WITHSYM32 {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_WITH32
    unsigned long   pParent;    // pointer to the parent
    unsigned long   pEnd;       // pointer to this blocks end
    unsigned long   len;        // Block length
    CV_uoff32_t     off;        // Offset in code segment
    unsigned short  seg;        // segment of label
    unsigned char   expr[1];    // Length-prefixed expression string
} WITHSYM32;



typedef struct CEXMSYM32 {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_CEXMODEL32
    CV_uoff32_t     off;        // offset of symbol
    unsigned short  seg;        // segment of symbol
    unsigned short  model;      // execution model
    union var32 {
        struct  {
            CV_uoff32_t pcdtable;   // offset to pcode function table
            CV_uoff32_t pcdspi;     // offset to segment pcode information
        } pcode;
        struct {
            unsigned short  subtype;   // see CV_COBOL_e above
            unsigned short  flag;
        } cobol;
        struct {
            CV_uoff32_t calltableOff; // offset to function table
            unsigned short calltableSeg; // segment of function table
        } pcode32Mac;
    };
} CEXMSYM32;



typedef struct VPATHSYM32_16t {
    unsigned short  reclen;     // record length
    unsigned short  rectyp;     // S_VFTABLE32_16t
    CV_uoff32_t     off;        // offset of virtual function table
    unsigned short  seg;        // segment of virtual function table
    CV_typ16_t      root;       // type index of the root of path
    CV_typ16_t      path;       // type index of the path record
} VPATHSYM32_16t;

typedef struct VPATHSYM32 {
    unsigned short  reclen;     // record length
    unsigned short  rectyp;     // S_VFTABLE32
    CV_typ_t        root;       // type index of the root of path
    CV_typ_t        path;       // type index of the path record
    CV_uoff32_t     off;        // offset of virtual function table
    unsigned short  seg;        // segment of virtual function table
} VPATHSYM32;





typedef struct REGREL32_16t {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_REGREL32_16t
    CV_uoff32_t     off;        // offset of symbol
    unsigned short  reg;        // register index for symbol
    CV_typ16_t      typind;     // Type index
    unsigned char   name[1];    // Length-prefixed name
} REGREL32_16t;

typedef struct REGREL32 {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_REGREL32
    CV_uoff32_t     off;        // offset of symbol
    CV_typ_t        typind;     // Type index
    unsigned short  reg;        // register index for symbol
    unsigned char   name[1];    // Length-prefixed name
} REGREL32;



typedef struct THREADSYM32_16t {
    unsigned short  reclen;     // record length
    unsigned short  rectyp;     // S_LTHREAD32_16t | S_GTHREAD32_16t
    CV_uoff32_t     off;        // offset into thread storage
    unsigned short  seg;        // segment of thread storage
    CV_typ16_t      typind;     // type index
    unsigned char   name[1];    // length prefixed name
} THREADSYM32_16t;

typedef struct THREADSYM32 {
    unsigned short  reclen;     // record length
    unsigned short  rectyp;     // S_LTHREAD32 | S_GTHREAD32
    CV_typ_t        typind;     // type index
    CV_uoff32_t     off;        // offset into thread storage
    unsigned short  seg;        // segment of thread storage
    unsigned char   name[1];    // length prefixed name
} THREADSYM32;

typedef struct SLINK32 {
    unsigned short  reclen;     // record length
    unsigned short  rectyp;     // S_SLINK32
    unsigned long   framesize;  // frame size of parent procedure
    CV_off32_t      off;        // signed offset where the static link was saved relative to the value of reg
    unsigned short  reg;
} SLINK32;

typedef struct PROCSYMMIPS_16t {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_GPROCMIPS_16t or S_LPROCMIPS_16t
    unsigned long   pParent;    // pointer to the parent
    unsigned long   pEnd;       // pointer to this blocks end
    unsigned long   pNext;      // pointer to next symbol
    unsigned long   len;        // Proc length
    unsigned long   DbgStart;   // Debug start offset
    unsigned long   DbgEnd;     // Debug end offset
    unsigned long   regSave;    // int register save mask
    unsigned long   fpSave;     // fp register save mask
    CV_uoff32_t     intOff;     // int register save offset
    CV_uoff32_t     fpOff;      // fp register save offset
    CV_uoff32_t     off;        // Symbol offset
    unsigned short  seg;        // Symbol segment
    CV_typ16_t      typind;     // Type index
    unsigned char   retReg;     // Register return value is in
    unsigned char   frameReg;   // Frame pointer register
    unsigned char   name[1];    // Length-prefixed name
} PROCSYMMIPS_16t;

typedef struct PROCSYMMIPS {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_GPROCMIPS or S_LPROCMIPS
    unsigned long   pParent;    // pointer to the parent
    unsigned long   pEnd;       // pointer to this blocks end
    unsigned long   pNext;      // pointer to next symbol
    unsigned long   len;        // Proc length
    unsigned long   DbgStart;   // Debug start offset
    unsigned long   DbgEnd;     // Debug end offset
    unsigned long   regSave;    // int register save mask
    unsigned long   fpSave;     // fp register save mask
    CV_uoff32_t     intOff;     // int register save offset
    CV_uoff32_t     fpOff;      // fp register save offset
    CV_typ_t        typind;     // Type index
    CV_uoff32_t     off;        // Symbol offset
    unsigned short  seg;        // Symbol segment
    unsigned char   retReg;     // Register return value is in
    unsigned char   frameReg;   // Frame pointer register
    unsigned char   name[1];    // Length-prefixed name
} PROCSYMMIPS;


typedef struct REFSYM {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_PROCREF or S_DATAREF
    unsigned long   sumName;    // SUC of the name
    unsigned long   ibSym;      // Offset of actual symbol in $$Symbols
    unsigned short  imod;       // Module containing the actual symbol
    unsigned short  usFill;     // align this record
} REFSYM;

typedef struct ALIGNSYM {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_PROCREF or S_DATAREF
} ALIGNSYM;

//  generic block definition symbols
//  these are similar to the equivalent 16:16 or 16:32 symbols but
//  only define the length, type and linkage fields

typedef struct PROCSYM {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_GPROC16 or S_LPROC16
    unsigned long   pParent;    // pointer to the parent
    unsigned long   pEnd;       // pointer to this blocks end
    unsigned long   pNext;      // pointer to next symbol
} PROCSYM;


typedef struct THUNKSYM {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_THUNK
    unsigned long   pParent;    // pointer to the parent
    unsigned long   pEnd;       // pointer to this blocks end
    unsigned long   pNext;      // pointer to next symbol
} THUNKSYM;

typedef struct BLOCKSYM {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_BLOCK16
    unsigned long   pParent;    // pointer to the parent
    unsigned long   pEnd;       // pointer to this blocks end
} BLOCKSYM;


typedef struct WITHSYM {
    unsigned short  reclen;     // Record length
    unsigned short  rectyp;     // S_WITH16
    unsigned long   pParent;    // pointer to the parent
    unsigned long   pEnd;       // pointer to this blocks end
} WITHSYM;


typedef enum CV_HREG_e {
    //  Register set for the Intel 80x86 and ix86 processor series
    //  (plus PCODE registers)

    CV_REG_NONE     =   0,
    CV_REG_AL       =   1,
    CV_REG_CL       =   2,
    CV_REG_DL       =   3,
    CV_REG_BL       =   4,
    CV_REG_AH       =   5,
    CV_REG_CH       =   6,
    CV_REG_DH       =   7,
    CV_REG_BH       =   8,
    CV_REG_AX       =   9,
    CV_REG_CX       =  10,
    CV_REG_DX       =  11,
    CV_REG_BX       =  12,
    CV_REG_SP       =  13,
    CV_REG_BP       =  14,
    CV_REG_SI       =  15,
    CV_REG_DI       =  16,
    CV_REG_EAX      =  17,
    CV_REG_ECX      =  18,
    CV_REG_EDX      =  19,
    CV_REG_EBX      =  20,
    CV_REG_ESP      =  21,
    CV_REG_EBP      =  22,
    CV_REG_ESI      =  23,
    CV_REG_EDI      =  24,
    CV_REG_ES       =  25,
    CV_REG_CS       =  26,
    CV_REG_SS       =  27,
    CV_REG_DS       =  28,
    CV_REG_FS       =  29,
    CV_REG_GS       =  30,
    CV_REG_IP       =  31,
    CV_REG_FLAGS    =  32,
    CV_REG_EIP      =  33,
    CV_REG_EFLAGS   =  34,
    CV_REG_TEMP     =  40,          // PCODE Temp
    CV_REG_TEMPH    =  41,          // PCODE TempH
    CV_REG_QUOTE    =  42,          // PCODE Quote
    CV_REG_PCDR3    =  43,          // PCODE reserved
    CV_REG_PCDR4    =  44,          // PCODE reserved
    CV_REG_PCDR5    =  45,          // PCODE reserved
    CV_REG_PCDR6    =  46,          // PCODE reserved
    CV_REG_PCDR7    =  47,          // PCODE reserved
    CV_REG_CR0      =  80,          // CR0 -- control registers
    CV_REG_CR1      =  81,
    CV_REG_CR2      =  82,
    CV_REG_CR3      =  83,
    CV_REG_CR4      =  84,          // Pentium
    CV_REG_DR0      =  90,          // Debug register
    CV_REG_DR1      =  91,
    CV_REG_DR2      =  92,
    CV_REG_DR3      =  93,
    CV_REG_DR4      =  94,
    CV_REG_DR5      =  95,
    CV_REG_DR6      =  96,
    CV_REG_DR7      =  97,
    CV_REG_GDTR     =  110,
    CV_REG_GDTL     =  111,
    CV_REG_IDTR     =  112,
    CV_REG_IDTL     =  113,
    CV_REG_LDTR     =  114,
    CV_REG_TR       =  115,

    CV_REG_PSEUDO1  =  116,
    CV_REG_PSEUDO2  =  117,
    CV_REG_PSEUDO3  =  118,
    CV_REG_PSEUDO4  =  119,
    CV_REG_PSEUDO5  =  120,
    CV_REG_PSEUDO6  =  121,
    CV_REG_PSEUDO7  =  122,
    CV_REG_PSEUDO8  =  123,
    CV_REG_PSEUDO9  =  124,

    CV_REG_ST0      =  128,
    CV_REG_ST1      =  129,
    CV_REG_ST2      =  130,
    CV_REG_ST3      =  131,
    CV_REG_ST4      =  132,
    CV_REG_ST5      =  133,
    CV_REG_ST6      =  134,
    CV_REG_ST7      =  135,
    CV_REG_CTRL     =  136,
    CV_REG_STAT     =  137,
    CV_REG_TAG      =  138,
    CV_REG_FPIP     =  139,
    CV_REG_FPCS     =  140,
    CV_REG_FPDO     =  141,
    CV_REG_FPDS     =  142,
    CV_REG_ISEM     =  143,
    CV_REG_FPEIP    =  144,
    CV_REG_FPEDO    =  145,

    // registers for the 68K processors

    CV_R68_D0       =    0,
    CV_R68_D1       =    1,
    CV_R68_D2       =    2,
    CV_R68_D3       =    3,
    CV_R68_D4       =    4,
    CV_R68_D5       =    5,
    CV_R68_D6       =    6,
    CV_R68_D7       =    7,
    CV_R68_A0       =    8,
    CV_R68_A1       =    9,
    CV_R68_A2       =   10,
    CV_R68_A3       =   11,
    CV_R68_A4       =   12,
    CV_R68_A5       =   13,
    CV_R68_A6       =   14,
    CV_R68_A7       =   15,
    CV_R68_CCR      =   16,
    CV_R68_SR       =   17,
    CV_R68_USP      =   18,
    CV_R68_MSP      =   19,
    CV_R68_SFC      =   20,
    CV_R68_DFC      =   21,
    CV_R68_CACR     =   22,
    CV_R68_VBR      =   23,
    CV_R68_CAAR     =   24,
    CV_R68_ISP      =   25,
    CV_R68_PC       =   26,
    //reserved  27
    CV_R68_FPCR     =   28,
    CV_R68_FPSR     =   29,
    CV_R68_FPIAR    =   30,
    //reserved  31
    CV_R68_FP0      =   32,
    CV_R68_FP1      =   33,
    CV_R68_FP2      =   34,
    CV_R68_FP3      =   35,
    CV_R68_FP4      =   36,
    CV_R68_FP5      =   37,
    CV_R68_FP6      =   38,
    CV_R68_FP7      =   39,
    //reserved  40
    CV_R68_MMUSR030 =   41,
    CV_R68_MMUSR    =   42,
    CV_R68_URP      =   43,
    CV_R68_DTT0     =   44,
    CV_R68_DTT1     =   45,
    CV_R68_ITT0     =   46,
    CV_R68_ITT1     =   47,
    //reserved  50
    CV_R68_PSR      =   51,
    CV_R68_PCSR     =   52,
    CV_R68_VAL      =   53,
    CV_R68_CRP      =   54,
    CV_R68_SRP      =   55,
    CV_R68_DRP      =   56,
    CV_R68_TC       =   57,
    CV_R68_AC       =   58,
    CV_R68_SCC      =   59,
    CV_R68_CAL      =   60,
    CV_R68_TT0      =   61,
    CV_R68_TT1      =   62,
    //reserved  63
    CV_R68_BAD0     =   64,
    CV_R68_BAD1     =   65,
    CV_R68_BAD2     =   66,
    CV_R68_BAD3     =   67,
    CV_R68_BAD4     =   68,
    CV_R68_BAD5     =   69,
    CV_R68_BAD6     =   70,
    CV_R68_BAD7     =   71,
    CV_R68_BAC0     =   72,
    CV_R68_BAC1     =   73,
    CV_R68_BAC2     =   74,
    CV_R68_BAC3     =   75,
    CV_R68_BAC4     =   76,
    CV_R68_BAC5     =   77,
    CV_R68_BAC6     =   78,
    CV_R68_BAC7     =   79,

     // Register set for the MIPS 4000

    CV_M4_NOREG     =   CV_REG_NONE,

    CV_M4_IntZERO   =   10,      /* CPU REGISTER */
    CV_M4_IntAT     =   11,
    CV_M4_IntV0     =   12,
    CV_M4_IntV1     =   13,
    CV_M4_IntA0     =   14,
    CV_M4_IntA1     =   15,
    CV_M4_IntA2     =   16,
    CV_M4_IntA3     =   17,
    CV_M4_IntT0     =   18,
    CV_M4_IntT1     =   19,
    CV_M4_IntT2     =   20,
    CV_M4_IntT3     =   21,
    CV_M4_IntT4     =   22,
    CV_M4_IntT5     =   23,
    CV_M4_IntT6     =   24,
    CV_M4_IntT7     =   25,
    CV_M4_IntS0     =   26,
    CV_M4_IntS1     =   27,
    CV_M4_IntS2     =   28,
    CV_M4_IntS3     =   29,
    CV_M4_IntS4     =   30,
    CV_M4_IntS5     =   31,
    CV_M4_IntS6     =   32,
    CV_M4_IntS7     =   33,
    CV_M4_IntT8     =   34,
    CV_M4_IntT9     =   35,
    CV_M4_IntKT0    =   36,
    CV_M4_IntKT1    =   37,
    CV_M4_IntGP     =   38,
    CV_M4_IntSP     =   39,
    CV_M4_IntS8     =   40,
    CV_M4_IntRA     =   41,
    CV_M4_IntLO     =   42,
    CV_M4_IntHI     =   43,

    CV_M4_Fir       =   50,
    CV_M4_Psr       =   51,

    CV_M4_FltF0     =   60,      /* Floating point registers */
    CV_M4_FltF1     =   61,
    CV_M4_FltF2     =   62,
    CV_M4_FltF3     =   63,
    CV_M4_FltF4     =   64,
    CV_M4_FltF5     =   65,
    CV_M4_FltF6     =   66,
    CV_M4_FltF7     =   67,
    CV_M4_FltF8     =   68,
    CV_M4_FltF9     =   69,
    CV_M4_FltF10    =   70,
    CV_M4_FltF11    =   71,
    CV_M4_FltF12    =   72,
    CV_M4_FltF13    =   73,
    CV_M4_FltF14    =   74,
    CV_M4_FltF15    =   75,
    CV_M4_FltF16    =   76,
    CV_M4_FltF17    =   77,
    CV_M4_FltF18    =   78,
    CV_M4_FltF19    =   79,
    CV_M4_FltF20    =   80,
    CV_M4_FltF21    =   81,
    CV_M4_FltF22    =   82,
    CV_M4_FltF23    =   83,
    CV_M4_FltF24    =   84,
    CV_M4_FltF25    =   85,
    CV_M4_FltF26    =   86,
    CV_M4_FltF27    =   87,
    CV_M4_FltF28    =   88,
    CV_M4_FltF29    =   89,
    CV_M4_FltF30    =   90,
    CV_M4_FltF31    =   91,
    CV_M4_FltFsr    =   92,


    // Register set for the ALPHA AXP

    CV_ALPHA_NOREG  = CV_REG_NONE,

    CV_ALPHA_FltF0  =   10,   // Floating point registers
    CV_ALPHA_FltF1  =   11,
    CV_ALPHA_FltF2  =   12,
    CV_ALPHA_FltF3  =   13,
    CV_ALPHA_FltF4  =   14,
    CV_ALPHA_FltF5  =   15,
    CV_ALPHA_FltF6  =   16,
    CV_ALPHA_FltF7  =   17,
    CV_ALPHA_FltF8  =   18,
    CV_ALPHA_FltF9  =   19,
    CV_ALPHA_FltF10 =   20,
    CV_ALPHA_FltF11 =   21,
    CV_ALPHA_FltF12 =   22,
    CV_ALPHA_FltF13 =   23,
    CV_ALPHA_FltF14 =   24,
    CV_ALPHA_FltF15 =   25,
    CV_ALPHA_FltF16 =   26,
    CV_ALPHA_FltF17 =   27,
    CV_ALPHA_FltF18 =   28,
    CV_ALPHA_FltF19 =   29,
    CV_ALPHA_FltF20 =   30,
    CV_ALPHA_FltF21 =   31,
    CV_ALPHA_FltF22 =   32,
    CV_ALPHA_FltF23 =   33,
    CV_ALPHA_FltF24 =   34,
    CV_ALPHA_FltF25 =   35,
    CV_ALPHA_FltF26 =   36,
    CV_ALPHA_FltF27 =   37,
    CV_ALPHA_FltF28 =   38,
    CV_ALPHA_FltF29 =   39,
    CV_ALPHA_FltF30 =   40,
    CV_ALPHA_FltF31 =   41,

    CV_ALPHA_IntV0  =   42,   // Integer registers
    CV_ALPHA_IntT0  =   43,
    CV_ALPHA_IntT1  =   44,
    CV_ALPHA_IntT2  =   45,
    CV_ALPHA_IntT3  =   46,
    CV_ALPHA_IntT4  =   47,
    CV_ALPHA_IntT5  =   48,
    CV_ALPHA_IntT6  =   49,
    CV_ALPHA_IntT7  =   50,
    CV_ALPHA_IntS0  =   51,
    CV_ALPHA_IntS1  =   52,
    CV_ALPHA_IntS2  =   53,
    CV_ALPHA_IntS3  =   54,
    CV_ALPHA_IntS4  =   55,
    CV_ALPHA_IntS5  =   56,
    CV_ALPHA_IntFP  =   57,
    CV_ALPHA_IntA0  =   58,
    CV_ALPHA_IntA1  =   59,
    CV_ALPHA_IntA2  =   60,
    CV_ALPHA_IntA3  =   61,
    CV_ALPHA_IntA4  =   62,
    CV_ALPHA_IntA5  =   63,
    CV_ALPHA_IntT8  =   64,
    CV_ALPHA_IntT9  =   65,
    CV_ALPHA_IntT10 =   66,
    CV_ALPHA_IntT11 =   67,
    CV_ALPHA_IntRA  =   68,
    CV_ALPHA_IntT12 =   69,
    CV_ALPHA_IntAT  =   70,
    CV_ALPHA_IntGP  =   71,
    CV_ALPHA_IntSP  =   72,
    CV_ALPHA_IntZERO =  73,


    CV_ALPHA_Fpcr   =   74,   // Control registers
    CV_ALPHA_Fir    =   75,
    CV_ALPHA_Psr    =   76,
    CV_ALPHA_FltFsr =   77,
    CV_ALPHA_SoftFpcr =   78,

    // Register Set for Motorola/IBM PowerPC

    /*
    ** PowerPC General Registers ( User Level )
    */
    CV_PPC_GPR0     =  1,
    CV_PPC_GPR1     =  2,
    CV_PPC_GPR2     =  3,
    CV_PPC_GPR3     =  4,
    CV_PPC_GPR4     =  5,
    CV_PPC_GPR5     =  6,
    CV_PPC_GPR6     =  7,
    CV_PPC_GPR7     =  8,
    CV_PPC_GPR8     =  9,
    CV_PPC_GPR9     = 10,
    CV_PPC_GPR10    = 11,
    CV_PPC_GPR11    = 12,
    CV_PPC_GPR12    = 13,
    CV_PPC_GPR13    = 14,
    CV_PPC_GPR14    = 15,
    CV_PPC_GPR15    = 16,
    CV_PPC_GPR16    = 17,
    CV_PPC_GPR17    = 18,
    CV_PPC_GPR18    = 19,
    CV_PPC_GPR19    = 20,
    CV_PPC_GPR20    = 21,
    CV_PPC_GPR21    = 22,
    CV_PPC_GPR22    = 23,
    CV_PPC_GPR23    = 24,
    CV_PPC_GPR24    = 25,
    CV_PPC_GPR25    = 26,
    CV_PPC_GPR26    = 27,
    CV_PPC_GPR27    = 28,
    CV_PPC_GPR28    = 29,
    CV_PPC_GPR29    = 30,
    CV_PPC_GPR30    = 31,
    CV_PPC_GPR31    = 32,

    /*
    ** PowerPC Condition Register ( User Level )
    */
    CV_PPC_CR       = 33,
    CV_PPC_CR0      = 34,
    CV_PPC_CR1      = 35,
    CV_PPC_CR2      = 36,
    CV_PPC_CR3      = 37,
    CV_PPC_CR4      = 38,
    CV_PPC_CR5      = 39,
    CV_PPC_CR6      = 40,
    CV_PPC_CR7      = 41,

    /*
    ** PowerPC Floating Point Registers ( User Level )
    */
    CV_PPC_FPR0     = 42,
    CV_PPC_FPR1     = 43,
    CV_PPC_FPR2     = 44,
    CV_PPC_FPR3     = 45,
    CV_PPC_FPR4     = 46,
    CV_PPC_FPR5     = 47,
    CV_PPC_FPR6     = 48,
    CV_PPC_FPR7     = 49,
    CV_PPC_FPR8     = 50,
    CV_PPC_FPR9     = 51,
    CV_PPC_FPR10    = 52,
    CV_PPC_FPR11    = 53,
    CV_PPC_FPR12    = 54,
    CV_PPC_FPR13    = 55,
    CV_PPC_FPR14    = 56,
    CV_PPC_FPR15    = 57,
    CV_PPC_FPR16    = 58,
    CV_PPC_FPR17    = 59,
    CV_PPC_FPR18    = 60,
    CV_PPC_FPR19    = 61,
    CV_PPC_FPR20    = 62,
    CV_PPC_FPR21    = 63,
    CV_PPC_FPR22    = 64,
    CV_PPC_FPR23    = 65,
    CV_PPC_FPR24    = 66,
    CV_PPC_FPR25    = 67,
    CV_PPC_FPR26    = 68,
    CV_PPC_FPR27    = 69,
    CV_PPC_FPR28    = 70,
    CV_PPC_FPR29    = 71,
    CV_PPC_FPR30    = 72,
    CV_PPC_FPR31    = 73,

    /*
    ** PowerPC Floating Point Status and Control Register ( User Level )
    */
    CV_PPC_FPSCR    = 74,

    /*
    ** PowerPC Machine State Register ( Supervisor Level )
    */
    CV_PPC_MSR      = 75,

    /*
    ** PowerPC Segment Registers ( Supervisor Level )
    */
    CV_PPC_SR0      = 76,
    CV_PPC_SR1      = 77,
    CV_PPC_SR2      = 78,
    CV_PPC_SR3      = 79,
    CV_PPC_SR4      = 80,
    CV_PPC_SR5      = 81,
    CV_PPC_SR6      = 82,
    CV_PPC_SR7      = 83,
    CV_PPC_SR8      = 84,
    CV_PPC_SR9      = 85,
    CV_PPC_SR10     = 86,
    CV_PPC_SR11     = 87,
    CV_PPC_SR12     = 88,
    CV_PPC_SR13     = 89,
    CV_PPC_SR14     = 90,
    CV_PPC_SR15     = 91,

    /*
    ** For all of the special purpose registers add 100 to the SPR# that the
    ** Motorola/IBM documentation gives with the exception of any imaginary
    ** registers.
    */

    /*
    ** PowerPC Special Purpose Registers ( User Level )
    */
    CV_PPC_PC       = 99,     // PC (imaginary register)

    CV_PPC_MQ       = 100,    // MPC601
    CV_PPC_XER      = 101,
    CV_PPC_RTCU     = 104,    // MPC601
    CV_PPC_RTCL     = 105,    // MPC601
    CV_PPC_LR       = 108,
    CV_PPC_CTR      = 109,

    CV_PPC_COMPARE  = 110,    // part of XER (internal to the debugger only)
    CV_PPC_COUNT    = 111,    // part of XER (internal to the debugger only)

    /*
    ** PowerPC Special Purpose Registers ( Supervisor Level )
    */
    CV_PPC_DSISR    = 118,
    CV_PPC_DAR      = 119,
    CV_PPC_DEC      = 122,
    CV_PPC_SDR1     = 125,
    CV_PPC_SRR0     = 126,
    CV_PPC_SRR1     = 127,
    CV_PPC_SPRG0    = 372,
    CV_PPC_SPRG1    = 373,
    CV_PPC_SPRG2    = 374,
    CV_PPC_SPRG3    = 375,
    CV_PPC_ASR      = 280,    // 64-bit implementations only
    CV_PPC_EAR      = 382,
    CV_PPC_PVR      = 287,
    CV_PPC_BAT0U    = 628,
    CV_PPC_BAT0L    = 629,
    CV_PPC_BAT1U    = 630,
    CV_PPC_BAT1L    = 631,
    CV_PPC_BAT2U    = 632,
    CV_PPC_BAT2L    = 633,
    CV_PPC_BAT3U    = 634,
    CV_PPC_BAT3L    = 635,
    CV_PPC_DBAT0U   = 636,
    CV_PPC_DBAT0L   = 637,
    CV_PPC_DBAT1U   = 638,
    CV_PPC_DBAT1L   = 639,
    CV_PPC_DBAT2U   = 640,
    CV_PPC_DBAT2L   = 641,
    CV_PPC_DBAT3U   = 642,
    CV_PPC_DBAT3L   = 643,

    /*
    ** PowerPC Special Purpose Registers Implementation Dependent ( Supervisor Level )
    */

    /*
    ** Doesn't appear that IBM/Motorola has finished defining these.
    */

    CV_PPC_PMR0     = 1044,   // MPC620,
    CV_PPC_PMR1     = 1045,   // MPC620,
    CV_PPC_PMR2     = 1046,   // MPC620,
    CV_PPC_PMR3     = 1047,   // MPC620,
    CV_PPC_PMR4     = 1048,   // MPC620,
    CV_PPC_PMR5     = 1049,   // MPC620,
    CV_PPC_PMR6     = 1050,   // MPC620,
    CV_PPC_PMR7     = 1051,   // MPC620,
    CV_PPC_PMR8     = 1052,   // MPC620,
    CV_PPC_PMR9     = 1053,   // MPC620,
    CV_PPC_PMR10    = 1054,   // MPC620,
    CV_PPC_PMR11    = 1055,   // MPC620,
    CV_PPC_PMR12    = 1056,   // MPC620,
    CV_PPC_PMR13    = 1057,   // MPC620,
    CV_PPC_PMR14    = 1058,   // MPC620,
    CV_PPC_PMR15    = 1059,   // MPC620,

    CV_PPC_DMISS    = 1076,   // MPC603
    CV_PPC_DCMP     = 1077,   // MPC603
    CV_PPC_HASH1    = 1078,   // MPC603
    CV_PPC_HASH2    = 1079,   // MPC603
    CV_PPC_IMISS    = 1080,   // MPC603
    CV_PPC_ICMP     = 1081,   // MPC603
    CV_PPC_RPA      = 1082,   // MPC603

    CV_PPC_HID0     = 1108,   // MPC601, MPC603, MPC620
    CV_PPC_HID1     = 1109,   // MPC601
    CV_PPC_HID2     = 1110,   // MPC601, MPC603, MPC620 ( IABR )
    CV_PPC_HID3     = 1111,   // Not Defined
    CV_PPC_HID4     = 1112,   // Not Defined
    CV_PPC_HID5     = 1113,   // MPC601, MPC604, MPC620 ( DABR )
    CV_PPC_HID6     = 1114,   // Not Defined
    CV_PPC_HID7     = 1115,   // Not Defined
    CV_PPC_HID8     = 1116,   // MPC620 ( BUSCSR )
    CV_PPC_HID9     = 1117,   // MPC620 ( L2CSR )
    CV_PPC_HID10    = 1118,   // Not Defined
    CV_PPC_HID11    = 1119,   // Not Defined
    CV_PPC_HID12    = 1120,   // Not Defined
    CV_PPC_HID13    = 1121,   // MPC604 ( HCR )
    CV_PPC_HID14    = 1122,   // Not Defined
    CV_PPC_HID15    = 1123,   // MPC601, MPC604, MPC620 ( PIR )

    //
    // JAVA VM registers
    //

    CV_JAVA_PC      = 1,


} CV_HREG_e;

#pragma pack ( pop )

#endif /* CV_INFO_INCLUDED */
