/*** 
*variant.h
*
*  Copyright (C) 1992-1993, Microsoft Corporation.  All Rights Reserved.
*
*Purpose:
*  This file declares VARIANT, and related data types.
*
*Implementation Notes:
*  This file requires ole2.h
*
*****************************************************************************/

#ifndef _VARIANT_H_
#define _VARIANT_H_


#ifndef HUGEP
# ifdef _MAC
#  define HUGEP FAR
# elif  WIN32
#  define HUGEP
# else
#  define HUGEP _huge
# endif
#endif


/* Forward Declarations */

#ifdef __cplusplus
interface IDispatch;
#else
typedef interface IDispatch IDispatch;
#endif


typedef char FAR* BSTR;
typedef BSTR FAR* LPBSTR;


typedef struct FARSTRUCT tagSAFEARRAYBOUND {
    ULONG cElements;
    LONG lLbound;
} SAFEARRAYBOUND, FAR* LPSAFEARRAYBOUND;

typedef struct FARSTRUCT tagSAFEARRAY {
    USHORT cDims;
    USHORT fFeatures;
    USHORT cbElements;
    USHORT cLocks;
    ULONG handle;
    void HUGEP* pvData;
    SAFEARRAYBOUND rgsabound[1];
} SAFEARRAY, FAR* LPSAFEARRAY;

#define FADF_AUTO       0x0001	/* array is allocated on the stack         */
#define FADF_STATIC     0x0002	/* array is staticly allocated             */
#define FADF_EMBEDDED   0x0004	/* array is embedded in a structure        */
#define FADF_FIXEDSIZE  0x0010	/* array may not be resized or reallocated */
#define FADF_BSTR       0x0100	/* an array of BSTRs                       */
#define FADF_UNKNOWN    0x0200	/* an array of IUnknown*                   */
#define FADF_DISPATCH   0x0400	/* an array of IDispatch*                  */
#define FADF_VARIANT    0x0800	/* an array of VARIANTs                    */
#define FADF_RESERVED   0xF0E8  /* bits reserved for future use            */


/* 0 == FALSE, -1 == TRUE */
typedef short VARIANT_BOOL;


typedef double DATE;


/* This is a helper struct for use in handling currency. */
typedef struct FARSTRUCT tagCY {
#ifdef _MAC
    long	  Hi;
    unsigned long Lo;
#else
    unsigned long Lo;
    long	  Hi;
#endif
} CY;


/*
 * VARENUM usage key,
 *
 *   [V] - may appear in a VARIANT
 *   [T] - may appear in a TYPEDESC
 *   [P] - may appear in an OLE property set
 *
 */
enum VARENUM
{
    VT_EMPTY           = 0,   /* [V]   [P]  nothing                     */
    VT_NULL            = 1,   /* [V]        SQL style Null              */
    VT_I2              = 2,   /* [V][T][P]  2 byte signed int           */
    VT_I4              = 3,   /* [V][T][P]  4 byte signed int           */
    VT_R4              = 4,   /* [V][T][P]  4 byte real                 */
    VT_R8              = 5,   /* [V][T][P]  8 byte real                 */
    VT_CY              = 6,   /* [V][T][P]  currency                    */
    VT_DATE            = 7,   /* [V][T][P]  date                        */
    VT_BSTR            = 8,   /* [V][T][P]  binary string               */
    VT_DISPATCH        = 9,   /* [V][T]     IDispatch FAR*              */
    VT_ERROR           = 10,  /* [V][T]     SCODE                       */
    VT_BOOL            = 11,  /* [V][T][P]  True=-1, False=0            */
    VT_VARIANT         = 12,  /* [V][T][P]  VARIANT FAR*                */
    VT_UNKNOWN         = 13,  /* [V][T]     IUnknown FAR*               */
    VT_WBSTR           = 14,  /* [V][T]     wide binary string          */

    VT_I1              = 16,  /*    [T]     signed char                 */
    VT_UI1             = 17,  /*    [T]     unsigned char               */
    VT_UI2             = 18,  /*    [T]     unsigned short              */
    VT_UI4             = 19,  /*    [T]     unsigned short              */
    VT_I8              = 20,  /*    [T][P]  signed 64-bit int           */
    VT_UI8             = 21,  /*    [T]     unsigned 64-bit int         */
    VT_INT             = 22,  /*    [T]     signed machine int          */
    VT_UINT            = 23,  /*    [T]     unsigned machine int        */
    VT_VOID            = 24,  /*    [T]     C style void                */
    VT_HRESULT         = 25,  /*    [T]                                 */
    VT_PTR             = 26,  /*    [T]     pointer type                */
    VT_SAFEARRAY       = 27,  /*    [T]     (use VT_ARRAY in VARIANT)   */
    VT_CARRAY          = 28,  /*    [T]     C style array               */
    VT_USERDEFINED     = 29,  /*    [T]     user defined type	        */
    VT_LPSTR           = 30,  /*    [T][P]  null terminated string      */
    VT_LPWSTR          = 31,  /*    [T][P]  wide null terminated string */

    VT_FILETIME        = 64,  /*       [P]  FILETIME                    */
    VT_BLOB            = 65,  /*       [P]  Length prefixed bytes       */
    VT_STREAM          = 66,  /*       [P]  Name of the stream follows  */
    VT_STORAGE         = 67,  /*       [P]  Name of the storage follows */
    VT_STREAMED_OBJECT = 68,  /*       [P]  Stream contains an object   */
    VT_STORED_OBJECT   = 69,  /*       [P]  Storage contains an object  */
    VT_BLOB_OBJECT     = 70,  /*       [P]  Blob contains an object     */
    VT_CF              = 71,  /*       [P]  Clipboard format            */
    VT_CLSID           = 72   /*       [P]  A Class ID                  */
};

#define VT_VECTOR      0x1000 /*       [P]  simple counted array        */
#define VT_ARRAY       0x2000 /* [V]        SAFEARRAY*                  */
#define VT_BYREF       0x4000 /* [V]                                    */
#define VT_RESERVED    0x8000


typedef unsigned short VARTYPE;

typedef struct FARSTRUCT tagVARIANT VARIANT;
typedef struct FARSTRUCT tagVARIANT FAR* LPVARIANT;
typedef struct FARSTRUCT tagVARIANT VARIANTARG;
typedef struct FARSTRUCT tagVARIANT FAR* LPVARIANTARG;

struct FARSTRUCT tagVARIANT{
    VARTYPE vt;
    WORD wReserved1;
    WORD wReserved2;
    WORD wReserved3;
    union {
      short	   iVal;             /* VT_I2                */
      long	   lVal;             /* VT_I4                */
      float	   fltVal;           /* VT_R4                */
      double	   dblVal;           /* VT_R8                */
      VARIANT_BOOL bool;             /* VT_BOOL              */
      SCODE	   scode;            /* VT_ERROR             */
      CY	   cyVal;            /* VT_CY                */
      DATE	   date;             /* VT_DATE              */
      BSTR	   bstrVal;          /* VT_BSTR              */
      IUnknown	   FAR* punkVal;     /* VT_UNKNOWN           */
      IDispatch	   FAR* pdispVal;    /* VT_DISPATCH          */
      SAFEARRAY	   FAR* parray;	     /* VT_ARRAY|*           */

      short	   FAR* piVal;       /* VT_BYREF|VT_I2	     */
      long	   FAR* plVal;       /* VT_BYREF|VT_I4	     */
      float	   FAR* pfltVal;     /* VT_BYREF|VT_R4       */
      double	   FAR* pdblVal;     /* VT_BYREF|VT_R8       */
      VARIANT_BOOL FAR* pbool;       /* VT_BYREF|VT_BOOL     */
      SCODE	   FAR* pscode;      /* VT_BYREF|VT_ERROR    */
      CY	   FAR* pcyVal;      /* VT_BYREF|VT_CY       */
      DATE	   FAR* pdate;       /* VT_BYREF|VT_DATE     */
      BSTR	   FAR* pbstrVal;    /* VT_BYREF|VT_BSTR     */
      IUnknown FAR* FAR* ppunkVal;   /* VT_BYREF|VT_UNKNOWN  */
      IDispatch FAR* FAR* ppdispVal; /* VT_BYREF|VT_DISPATCH */
      SAFEARRAY FAR* FAR* pparray;   /* VT_BYREF|VT_ARRAY|*  */
      VARIANT	   FAR* pvarVal;     /* VT_BYREF|VT_VARIANT  */

      void	   FAR* byref;	     /* Generic ByRef        */
    }
#ifdef NONAMELESSUNION
    u
#endif
     ;
};

#ifdef NONAMELESSUNION
# define V_UNION(X, Y) ((X)->u.Y)
#else
# define V_UNION(X, Y) ((X)->Y)
#endif

/* Variant access macros */
#define V_VT(X)          ((X)->vt)
#define V_ISBYREF(X)     (V_VT(X)&VT_BYREF)
#define V_ISARRAY(X)     (V_VT(X)&VT_ARRAY)
#define V_ISVECTOR(X)    (V_VT(X)&VT_VECTOR)

#define V_NONE(X)        V_I2(X)
#define V_I2(X)	         V_UNION(X, iVal)
#define V_I2REF(X)       V_UNION(X, piVal)
#define V_I4(X)          V_UNION(X, lVal)
#define V_I4REF(X)       V_UNION(X, plVal)
#define V_R4(X)	         V_UNION(X, fltVal)
#define V_R4REF(X)       V_UNION(X, pfltVal)
#define V_R8(X)	         V_UNION(X, dblVal)
#define V_R8REF(X)       V_UNION(X, pdblVal)
#define V_BOOL(X)        V_UNION(X, bool)
#define V_BOOLREF(X)     V_UNION(X, pbool)
#define V_ERROR(X)       V_UNION(X, scode)
#define V_ERRORREF(X)    V_UNION(X, pscode)
#define V_CY(X)	         V_UNION(X, cyVal)
#define V_CYREF(X)       V_UNION(X, pcyVal)
#define V_DATE(X)        V_UNION(X, date)
#define V_DATEREF(X)     V_UNION(X, pdate)
#define V_BSTR(X)        V_UNION(X, bstrVal)
#define V_BSTRREF(X)     V_UNION(X, pbstrVal)
#define V_UNKNOWN(X)     V_UNION(X, punkVal)
#define V_UNKNOWNREF(X)  V_UNION(X, ppunkVal)
#define V_DISPATCH(X)    V_UNION(X, pdispVal)
#define V_DISPATCHREF(X) V_UNION(X, ppdispVal)
#define V_VARIANTREF(X)  V_UNION(X, pvarVal)
#define V_ARRAY(X)       V_UNION(X, parray)
#define V_ARRAYREF(X)    V_UNION(X, pparray)
#define V_BYREF(X)       V_UNION(X, byref)

#endif /* _VARIANT_H_ */
