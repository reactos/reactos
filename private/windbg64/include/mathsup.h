/***
*cv.h - definitions for floating point conversion
*
*	Copyright (c) 1991-1991, Microsoft Corporation.	All rights reserved.
*
*Purpose:
*   define types, macros, and constants used in floating point
*   conversion routines
*
*Revision History:
*   7-17-91	GDP	initial version
*   9-21-91	GDP	restructured 'ifdef' directives
*  10-29-91	GDP	MIPS port: new defs for ALIGN and UDOUBLE
*   3-03-92	GDP	removed os2 16-bit stuff
*   4-30-92	GDP	support intrncvt.c --cleanup and reorganize
*
*******************************************************************************/
#ifndef _INC_CV

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _WINDOWS_       // NOTENOTE Davegi This needs to be cleaned up!
typedef float FLOAT;
#endif // _WINDOWS_

// #include <cruntime.h>

#ifndef _MIPS_
#if (_MSC_VER <= 600)
#define __cdecl _cdecl
#endif
#endif


//
// For MIPS, define UDOUBLE as 'double' before including fltintrn.h,
// so that the definition in fltintrn.h is not used.
// This is done because floating point arguments are passed in the
// fp register.
//

#ifdef MIPS
#define UDOUBLE double
#endif

//
// definitions from crt32\h\fltintrn.h -- keep in sync
//


/*
 * structs used to fool the compiler into not generating floating point
 * instructions when copying and pushing [long] double values
 */

#ifndef UDOUBLE

typedef struct {
	double x;
} UDOUBLE;

#endif


/*
 * typedef for _fltout
 */

typedef struct _strflt
{
	int sign;	      /* zero if positive otherwise negative */
	int decpt;	      /* exponent of floating point number */
	int flag;	      /* zero if okay otherwise IEEE overflow */
	char *mantissa;       /* pointer to mantissa in string form */
}
	*STRFLT;


/*
 * typedef for _fltin
 */

typedef struct _flt
{
	int flags;
	int nbytes;	     /* number of characters read */
	long lval;
	double dval;	     /* the returned floating point number */
}
	*FLT;


char *_cftoe(double *, char *, int, int);
char *_cftof(double *, char *, int);
void _fptostr(char *, int, STRFLT);

#ifdef	MTHREAD

STRFLT	_fltout2( double, STRFLT, char * );
FLT	 _fltin2( FLT , const char *, int, int, int );

#else

STRFLT	_fltout( UDOUBLE );
FLT	 _fltin( const char *, int, int, int );

#endif


//
// end of definitions from crt32\h\fltintrn.h
//



/* define little endian or big endian memory */

#ifdef i386
#define L_END
#endif

#ifdef MIPS
#define L_END
#endif

#ifdef IA64
#define L_END
#endif

#ifdef ALPHA
#define L_END
#endif

#ifdef PPC
#define L_END
#endif

typedef unsigned char	u_char;   /* should have 1 byte	*/
typedef char		s_char;   /* should have 1 byte	*/
typedef unsigned short	u_short;  /* should have 2 bytes */
typedef signed short	s_short;  /* should have 2 bytes */
typedef unsigned int	u_long;	  /* sholuld have 4 bytes */
typedef int		s_long;	  /* sholuld have 4 bytes */


//
// defining _LDSUPPORT enables using long double computations
// for string conversion. We do not do this even for i386,
// since we want to avoid using floating point code that
// may generate IEEE exceptions.
//
// Currently our string conversion routines do not conform
// to the special requirements of the IEEE standard for
// floating point conversions
//


#ifndef _LDSUPPORT

#pragma pack(4)
typedef struct {
    u_char ld[10];
} _ULDOUBLE;
#pragma pack()

#define PTR_LD(x) ((u_char  *)(&(x)->ld))

#else

typedef long double _ULDOUBLE;

#define PTR_LD(x) ((u_char  *)(x))

#endif


#pragma pack(4)
typedef struct {
    u_char ld12[12];
} _ULDBL12;
#pragma pack()

#if 0
typedef struct {
    float f;
} FLOAT;
#endif



//
// return values for internal conversion routines
// (12-byte to long double, double, or float)
//

typedef enum {
    INTRNCVT_OK,
    INTRNCVT_OVERFLOW,
    INTRNCVT_UNDERFLOW
} INTRNCVT_STATUS;


//
// return values for strgtold12 routine
//

#define SLD_UNDERFLOW 1
#define SLD_OVERFLOW 2
#define SLD_NODIGITS 4

#define MAX_MAN_DIGITS 21


// specifies '%f' format

#define SO_FFORMAT 1

typedef  struct _FloatOutStruct {
		    short   exp;
		    char    sign;
		    char    ManLen;
		    char    man[MAX_MAN_DIGITS+1];
		    } FOS;



#define PTR_12(x) ((u_char  *)(&(x)->ld12))

#define MAX_USHORT  ((u_short)0xffff)
#define MSB_USHORT  ((u_short)0x8000)
#define MAX_ULONG   ((u_long)0xffffffff)
#define MSB_ULONG   ((u_long)0x80000000)

#define TMAX10 5200	  /* maximum temporary decimal exponent */
#define TMIN10 -5200	  /* minimum temporary decimal exponent */
#define LD_MAX_EXP_LEN 4  /* maximum number of decimal exponent digits */
#define LD_MAX_MAN_LEN 24  /* maximum length of mantissa (decimal)*/
#define LD_MAX_MAN_LEN1 25 /* MAX_MAN_LEN+1 */

#define LD_BIAS	0x3fff	  /* exponent bias for long double */
#define LD_BIASM1 0x3ffe  /* LD_BIAS - 1 */
#define LD_MAXEXP 0x7fff  /* maximum biased exponent */

#define D_BIAS	0x3ff	 /* exponent bias for double */
#define D_BIASM1 0x3fe	/* D_BIAS - 1 */
#define D_MAXEXP 0x7ff	/* maximum biased exponent */



/* Recognizing special patterns in the mantissa field */
#define _EXP_SP  0x7fff
#define NAN_BIT (1<<30)

#define _IS_MAN_INF(signbit, manhi, manlo) \
	( (manhi)==MSB_ULONG && (manlo)==0x0 )


#ifdef i386
#define _IS_MAN_IND(signbit, manhi, manlo) \
	((signbit) && (manhi)==0xc0000000 && (manlo)==0)

#define _IS_MAN_QNAN(signbit, manhi, manlo) \
	( (manhi)&NAN_BIT )

#define _IS_MAN_SNAN(signbit, manhi, manlo) \
	(!( _IS_MAN_INF(signbit, manhi, manlo) || \
	   _IS_MAN_QNAN(signbit, manhi, manlo) ))


#else
#ifdef IA64
#define _IS_MAN_IND(signbit, manhi, manlo) \
	((signbit) && (manhi)==0xc0000000 && (manlo)==0)

#define _IS_MAN_QNAN(signbit, manhi, manlo) \
	( (manhi)&NAN_BIT )

#define _IS_MAN_SNAN(signbit, manhi, manlo) \
	(!( _IS_MAN_INF(signbit, manhi, manlo) || \
		_IS_MAN_QNAN(signbit, manhi, manlo) ))

#else
#ifdef MIPS
#define _IS_MAN_IND(signbit, manhi, manlo) \
	(!(signbit) && (manhi)==0xbfffffff && (manlo)==0xfffff800)

#define _IS_MAN_SNAN(signbit, manhi, manlo) \
	( (manhi)&NAN_BIT )

#define _IS_MAN_QNAN(signbit, manhi, manlo) \
	(!( _IS_MAN_INF(signbit, manhi, manlo) || \
	   _IS_MAN_SNAN(signbit, manhi, manlo) ))
#else
#ifdef PPC 
// copied alpha crap below bugbug fmbutt 
#define _IS_MAN_IND(signbit, manhi, manlo) 1
#define _IS_MAN_SNAN(signbit, manhi, manlo) 1
#define _IS_MAN_QNAN(signbit, manhi, manlo) 1 
#else // ALPHA
// MBH - bugbug - just put anything in here because its
// only used to determine string size!
//
#define _IS_MAN_IND(signbit, manhi, manlo) 1
#define _IS_MAN_SNAN(signbit, manhi, manlo) 1
#define _IS_MAN_QNAN(signbit, manhi, manlo) 1
#endif
#endif
#endif
#endif

//
// MBH -bugbug
//   We probaby don't look just like 386 in this regard.
//   Figure out ALPHA stuff for FP here.
//

#if defined (L_END) && !defined (MIPS)
/* "little endian" memory */
/* Note: MIPS has alignment requirements and has different macros */
/*
 * Manipulation of a 12-byte long double number (an ordinary
 * 10-byte long double plus two extra bytes of mantissa).
 */

/* a pointer to the exponent/sign portion */
#define U_EXP_12(p) ((u_short  *)(PTR_12(p)+10))

/* a pointer to the 4 hi-order bytes of the mantissa */
#define UL_MANHI_12(p) ((u_long  *)(PTR_12(p)+6))

/* a pointer to the 4 lo-order bytes of the ordinary (8-byte) mantissa */
#define UL_MANLO_12(p) ((u_long  *)(PTR_12(p)+2))

/* a pointer to the 2 extra bytes of the mantissa */
#define U_XT_12(p) ((u_short  *)PTR_12(p))

/* a pointer to the 4 lo-order bytes of the extended (10-byte) mantissa */
#define UL_LO_12(p) ((u_long  *)PTR_12(p))

/* a pointer to the 4 mid-order bytes of the extended (10-byte) mantissa */
#define UL_MED_12(p) ((u_long  *)(PTR_12(p)+4))

/* a pointer to the 4 hi-order bytes of the extended long double */
#define UL_HI_12(p) ((u_long  *)(PTR_12(p)+8))

/* a pointer to the byte of order i (LSB=0, MSB=9)*/
#define UCHAR_12(p,i) ((u_char	*)PTR_12(p)+(i))

/* a pointer to a u_short with offset i */
#define USHORT_12(p,i) ((u_short  *)((u_char  *)PTR_12(p)+(i)))

/* a pointer to a u_long with offset i */
#define ULONG_12(p,i) ((u_long	*)((u_char  *)PTR_12(p)+(i)))

/* a pointer to the 10 MSBytes of a 12-byte long double */
#define TEN_BYTE_PART(p) ((u_char  *)PTR_12(p)+2)

/*
 * Manipulation of a 10-byte long double number
 */
#define U_EXP_LD(p) ((u_short  *)(PTR_LD(p)+8))
#define UL_MANHI_LD(p) ((u_long  *)(PTR_LD(p)+4))
#define UL_MANLO_LD(p) ((u_long  *)PTR_LD(p))

/*
 * Manipulation of a 64bit IEEE double
 */
#define U_SHORT4_D(p) ((u_short  *)(p) + 3)
#define UL_HI_D(p) ((u_long  *)(p) + 1)
#define UL_LO_D(p) ((u_long  *)(p))

#endif

#ifdef B_END   /* big endian */

#define U_EXP_12(p) ((u_short  *)PTR_12(p))
#define UL_MANHI_12(p) ((u_long  *)(PTR_12(p)+2))
#define UL_MANLO_12(p) ((u_long  *)(PTR_12(p)+6))
#define U_XT_12(p) ((u_short  *)(PTR_12(p)+10))

#define UL_LO_12(p) ((u_long  *)PTR_12(p))
#define UL_MED_12(p) ((u_long  *)(PTR_12(p)+4))
#define UL_HI_12(p) ((u_long  *)(PTR_12(p)+8))

#define UCHAR_12(p,i) ((u_char	*)PTR_12(p)+(11-(i)))
#define USHORT_12(p,i)	((u_short  *)((u_char  *)PTR_12(p)+10-(i)))
#define ULONG_12(p,i) ((u_long	*)((u_char  *)PTR_12(p)+8-(i)))
#define TEN_BYTE_PART(p) (u_char  *)PTR_12(p)

#define U_EXP_LD(p) ((u_short  *)PTR_LD(p))
#define UL_MANHI_LD(p) ((u_long  *)(PTR_LD(p)+4))
#define UL_MANLO_LD(p) ((u_long  *)(PTR_LD(p)+8))

/*
 * Manipulation of a 64bit IEEE double
 */
#define U_SHORT4_D(p) ((u_short  *)(p) + 3)
#define UL_HI_D(p) ((u_long  *)(p) + 1)
#define UL_LO_D(p) ((u_long  *)(p))

#endif

#ifdef MIPS

#define MIPSALIGN(x)  ( (unsigned long  __unaligned *) (x))

#define U_EXP_12(p) ((u_short  *)(PTR_12(p)+10))

#define UL_MANHI_12(p) ((u_long  __unaligned *) (PTR_12(p)+6) )
#define UL_MANLO_12(p) ((u_long  __unaligned *) (PTR_12(p)+2) )


#define U_XT_12(p) ((u_short  *)PTR_12(p))
#define UL_LO_12(p) ((u_long  *)PTR_12(p))
#define UL_MED_12(p) ((u_long  *)(PTR_12(p)+4))
#define UL_HI_12(p) ((u_long  *)(PTR_12(p)+8))

/* the following 3 macros do not take care of proper alignment */
#define UCHAR_12(p,i) ((u_char	*)PTR_12(p)+(i))
#define USHORT_12(p,i) ((u_short  *)((u_char  *)PTR_12(p)+(i)))
#define ULONG_12(p,i) ((u_long	*) ((u_char  *)PTR_12(p)+(i) ))

#define TEN_BYTE_PART(p) ((u_char  *)PTR_12(p)+2)

/*
 * Manipulation of a 10-byte long double number
 */
#define U_EXP_LD(p) ((u_short  *)(PTR_LD(p)+8))

#define UL_MANHI_LD(p) ((u_long  *) (PTR_LD(p)+4) )
#define UL_MANLO_LD(p) ((u_long  *) PTR_LD(p) )

/*
 * Manipulation of a 64bit IEEE double
 */
#define U_SHORT4_D(p) ((u_short  *)(p) + 3)
#define UL_HI_D(p) ((u_long  *)(p) + 1)
#define UL_LO_D(p) ((u_long  *)(p))

#endif


#define PUT_INF_12(p,sign) \
		  *UL_HI_12(p) = (sign)?0xffff8000:0x7fff8000; \
		  *UL_MED_12(p) = 0; \
		  *UL_LO_12(p) = 0;

#define PUT_ZERO_12(p) *UL_HI_12(p) = 0; \
		  *UL_MED_12(p) = 0; \
		  *UL_LO_12(p) = 0;

#define ISZERO_12(p) ((*UL_HI_12(p)&0x7fffffff) == 0 && \
		      *UL_MED_12(p) == 0 && \
		      *UL_LO_12(p) == 0 )

#define PUT_INF_LD(p,sign) \
		  *U_EXP_LD(p) = (sign)?0xffff:0x7fff; \
		  *UL_MANHI_LD(p) = 0x8000; \
		  *UL_MANLO_LD(p) = 0;

#define PUT_ZERO_LD(p) *U_EXP_LD(p) = 0; \
		  *UL_MANHI_LD(p) = 0; \
		  *UL_MANLO_LD(p) = 0;

#define ISZERO_LD(p) ((*U_EXP_LD(p)&0x7fff) == 0 && \
		      *UL_MANHI_LD(p) == 0 && \
		      *UL_MANLO_LD(p) == 0 )


/*********************************************************
 *
 *   Function Prototypes
 *
 *********************************************************/

/* from mantold.c */
void  __mtold12(char	*manptr, unsigned manlen,_ULDBL12 *ld12);
int  __addl(u_long x, u_long y, u_long  *sum);
void __shl_12(_ULDBL12  *ld12);
void __shr_12(_ULDBL12  *ld12);
void __add_12(_ULDBL12  *x, _ULDBL12  *y);

/* from tenpow.c */
void __multtenpow12(_ULDBL12	*pld12,int pow, unsigned mult12);
void __ld12mul(_ULDBL12  *px, _ULDBL12  *py);

/* from strgtold.c */
unsigned int __strgtold12(_ULDBL12 *pld12,
	    char  *  *p_end_ptr,
	    char  *str,
	    int mult12);


/* from x10fout.c */

char * _uldtoa (_ULDOUBLE *px, int maxchars, char *ldtext);



/* this is defined as void in convert.h
 * After porting the asm files to c, we need a return value for
 * i10_output, that used to reside in reg. ax
 */
int 	$I10_OUTPUT(_ULDOUBLE ld, int ndigits,
		    unsigned output_flags, FOS	*fos);


/* for cvt.c and fltused.c */
void _cfltcvt(double *arg, char *buffer,
			 int format, int precision,
			 int caps);
void _cropzeros(char *buf);
void _fassign(int flag, char  *argument, char *number);
void _forcdecpt(char *buf);
int  _positive(double *arg);

/* from intrncvt.c */
void _atodbl(UDOUBLE *d, char *str);
void _atoldbl(_ULDOUBLE *ld, char *str);
void _atoflt(FLOAT *f, char *str);
INTRNCVT_STATUS _ld12tod(_ULDBL12 *ifp, UDOUBLE *d);
INTRNCVT_STATUS _ld12tof(_ULDBL12 *ifp, FLOAT *f);
void _ld12told(_ULDBL12 *ifp, _ULDOUBLE *ld);



#ifdef __cplusplus
}
#endif

#define _INC_CV
#endif	/* _INC_CV */
