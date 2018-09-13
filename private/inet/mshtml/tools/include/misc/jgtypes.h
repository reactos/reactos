/*----------------------------------------------------------------------------
;
; Start of jgtypes.h - Definitions for common types and macros
;
; Copyright (c) 1994-1996 Johnson-Grace Company, all rights reserved
;
;---------------------------------------------------------------------------*/

#ifndef JGTYPES_INCLUDED
#define JGTYPES_INCLUDED

#ifdef __cplusplus
  extern "C" {                  // indicate C declarations if C++
#endif

// Determine if 32-bit architecture (vs. 16-bit segmented).  If segmented,
// determine if using a small data model (tiny model is not supported).

#if defined(__FLAT__) || defined(_WIN32) || defined(unix) || defined(_MAC)
  #define JGMACH32
#elif defined(__SMALL__) || defined(__MEDIUM__) // Borland
  #define JGSMALLDATA
#elif defined(_M_I86SM) || defined(_M_I86MM)    // Microsoft
  #define JGSMALLDATA
#endif

#define JGCONST const

// These defines declare the native int.  These are the fastest ints
// available (of at least 16 bits) for a given environment.

#ifdef JGINTW32
  typedef long INTW;
  typedef unsigned long UINTW;
#else  
  typedef int INTW;
  typedef unsigned int UINTW;
#endif

// These ints are for known lengths.
// Note that they work for both 16 and 32 machines...
// (at least for WATCOM, BORLAND and Microsoft compilers)

typedef unsigned char  UINT8; 
typedef unsigned short UINT16;
typedef unsigned long  UINT32;   
typedef signed char    INT8;
typedef signed short   INT16;
typedef signed long    INT32;

// Pointers, calling conventions.

#ifdef JGMACH32
  #define JGHUGE
  #define JGFAR
  #define JGNEAR
  #ifndef JGCCONV
	#ifdef unix
		#define JGCCONV
	#elif defined(_MAC)
		#define JGCCONV
	#else
	#define JGCCONV __cdecl 
	#endif
  #endif
  #ifndef JGEXPORT
    #define JGEXPORT __declspec(dllexport)
  #endif
#else
  #define JGHUGE _huge
  #define JGFAR  _far
  #define JGNEAR _near
  #ifndef JGCCONV
    #define JGCCONV __pascal
  #endif
  #ifndef JGEXPORT
    #define JGEXPORT __export
  #endif
#endif

#define JGPTR JGFAR *
#define JGHPTR JGHUGE *
#ifndef JGNFUNC
  #define JGNFUNC JGNEAR JGCCONV
#endif
#ifndef JGFFUNC
  #define JGFFUNC JGFAR JGCCONV
#endif
#ifndef JGFUNC
  #define JGFUNC JGCCONV
#endif

// Other simple types.

typedef int JGBOOL;
typedef UINTW JGERR;

// Type-checked handle creation macro
#define JG_DECLARE_HANDLE(id) \
  struct id##_DUMMY { UINTW dummy[16]; }; \
  typedef const struct id##_DUMMY JGPTR id

// Generic JG handle
JG_DECLARE_HANDLE(JGHANDLE);

/******************************************
MEMORY MANIPULATION
******************************************/

typedef void (JGFUNC JGPTR JG_HMEMCPY_FN)(void JGHPTR, void JGHPTR, UINT32);

 #if defined(JGMACH32)
  #if defined( _MAC)
    #define JG_HMEMCPY(Dst, Src, n) memmove(Dst, Src, (UINTW) n)
    #define JG_FMEMCPY(Dst, Src, n)  memmove(Dst, Src, (UINTW) n)
    #define JG_FMEMMOVE(Dst, Src, n)  memmove(Dst, Src, (UINTW) n)
  #else
    #define JG_HMEMCPY(Dst, Src, n) memcpy(Dst, Src, (UINTW)(n))
    #define JG_FMEMCPY memcpy
    #define JG_FMEMMOVE memmove
  #endif

  #define JG_FMEMCMP memcmp
  #define JG_FMEMSET memset
  #define JG_FSTRNCPY strncpy
  #define JG_FSTRCPY strcpy
  #define JG_FSTRCAT strcat
  #define JG_FSTRLEN strlen
  #define JG_FSTRCHR strchr
  #define JG_FSTRRCHR strrchr
#else
  #define JG_HMEMCPY Jghmemcpy          // Defined by user.
  void JGFUNC Jghmemcpy(void JGHPTR, void JGHPTR, UINT32);
  #define JG_FMEMCPY Jgfmemcpy
  void JGFUNC Jgfmemcpy(void JGPTR, void JGPTR, UINTW);
  #define JG_FMEMCMP _fmemcmp
  #define JG_FMEMMOVE _fmemmove
  #define JG_FMEMSET _fmemset
  #define JG_FSTRNCPY _fstrncpy
  #define JG_FSTRCPY _fstrcpy
  #define JG_FSTRCAT _fstrcat
  #define JG_FSTRLEN _fstrlen
  #define JG_FSTRCHR _fstrchr
  #define JG_FSTRRCHR _fstrrchr
#endif

/**************************************
*MEMORY ALLOCATION MACROS             *
**************************************/

typedef void JGPTR (JGFUNC JGPTR JG_FMALLOC_FN)(UINT32 n);
typedef void (JGFUNC JGPTR JG_FFREE_FN)(void JGPTR p);

#ifndef JGNOMEMDEF
 #ifdef JGMACH32
  #ifdef JGMEMCALL
    #define JG_MALLOC   Jgmalloc
    void * JGFUNC       Jgmalloc(UINT32 n);
    #define JG_FREE     Jgfree
    UINTW JGFUNC        Jgfree(void *p);

    #define JG_FMALLOC(n) Jgmalloc((UINTW)(n))
    #define JG_FFREE    Jgfree
	#define JG_MAKEEXEPTR JgMakeExePtr
	void JGPTR JGFUNC       JgMakeExePtr(void JGPTR p);
	#define JG_FREEEXEPTR JgFreeExePtr
	void JGFUNC JgFreeExePtr(void JGPTR p);
  #else
    #define JG_MALLOC(n)  malloc((UINTW)(n))
    #define JG_FMALLOC(n) malloc((UINTW)(n))
    #define JG_FREE     free
    #define JG_FFREE    free
	#define JG_MAKEEXEPTR(a) (a)
	#define JG_FREEEXEPTR(a)  
  #endif
 #else
  #ifndef JGMEMCALL
    #define JGMEMCALL
  #endif
    #define JG_MALLOC   Jgmalloc
    void * JGFUNC       Jgmalloc(UINT32 n);
    #define JG_FREE     Jgfree
    UINTW JGFUNC        Jgfree(void *p);

    #define JG_FMALLOC  Jgfmalloc       // note: must return 0-offset pointers
    void JGPTR JGFUNC   Jgfmalloc(UINT32 n);
    #define JG_FFREE    Jgffree
    void JGFUNC         Jgffree(void JGPTR p);
	#define JG_MAKEEXEPTR JgMakeExePtr
	void JGPTR JGFUNC       JgMakeExePtr(void JGPTR p);
	#define JG_FREEEXEPTR JgFreeExePtr
	void JGFUNC JgFreeExePtr(void JGPTR p);

 #endif
#endif

/**************************************
* ERROR CLASSIFICATIONS               *
**************************************/

#define JG_ERR_SHIFT (12)
#define JG_ERR_MASK  (0x000F)

#define JG_ERR_UNKNOWN (0<<JG_ERR_SHIFT) // (Place holder, don't use this)
#define JG_ERR_STATUS  (1<<JG_ERR_SHIFT) // Exceptions that may not be errors
#define JG_ERR_MEMORY  (2<<JG_ERR_SHIFT) // Memory allocation errors
#define JG_ERR_FILEIO  (3<<JG_ERR_SHIFT) // File IO Errors
#define JG_ERR_ARG     (4<<JG_ERR_SHIFT) // Errors due to passing bad args
#define JG_ERR_VERSION (5<<JG_ERR_SHIFT) // Errors due to version mismatch
#define JG_ERR_DATA    (6<<JG_ERR_SHIFT) // Errors due to corrupted data
#define JG_ERR_CHECK   (7<<JG_ERR_SHIFT) // Internal consistency checks
#define JG_ERR_STATE   (8<<JG_ERR_SHIFT) // State invalid to perform operation

/**************************************
* COMPLEX STRUCTURES                  *
**************************************/

// More complex structures/types.

typedef UINT32 JGFOURCHAR;		// Four character code
#define JG_MAKEFOURCHAR(a,b,c,d) \
	(((UINT32)(UINT8)(a) << 24) | ((UINT32)(UINT8)(b) << 16) | \
	((UINT32)(UINT8)(c) << 8) | (UINT32)(UINT8)(d))
#define JG4C_AUTO JG_MAKEFOURCHAR('a','u','t','o')	// image formats
#define JG4C_ART  JG_MAKEFOURCHAR('A','R','T','f')
#define JG4C_GIF  JG_MAKEFOURCHAR('G','I','F','f')
#define JG4C_BMP  JG_MAKEFOURCHAR('B','M','P',' ')
#define JG4C_JPEG JG_MAKEFOURCHAR('J','P','E','G')
#define JG4C_ART_GT8	JG_MAKEFOURCHAR('G','T','8',' ') // .ART sub-formats
#define JG4C_ART_GT24	JG_MAKEFOURCHAR('G','T','2','4')
#define JG4C_ART_CT	JG_MAKEFOURCHAR('C','T',' ',' ')
#define JG4C_ART_WAVE	JG_MAKEFOURCHAR('W','A','V','E')

typedef struct {		// component decoder stream type descriptor
    UINTW nSize;		// sizeof() this structure, in bytes
    JGFOURCHAR	ImageFormat;	// main format (JPEG, GIF, etc)
    JGFOURCHAR	SubFormat;	// sub-format (e.g. ART: GT, WAVE, etc), or 0
    char JGPTR	Extension;	// common three character file extension
    char JGPTR	Description;	// short format description string
} JG_READER_DESC;

typedef struct {                // bit-stream pointer structure
    UINT8 JGPTR BitPtr;         // pointer to next byte to access
    UINTW  BitCnt;              // next *BitPtr bit, 7(hi bit) to 0(low bit)
} BIT_STREAM;

typedef struct {                // bit-stream pointer structure
    UINT8 JGPTR BitPtr;         // pointer to next byte to access
    UINTW  BitCnt;              // next *BitPtr bit, 7(hi bit) to 0(low bit)
} JG_BIT_PTR;

typedef struct {                // "new" bit-block bit-stream pointer
    UINT8 JGPTR BitPtr;         // pointer to next byte to access in bit-block
    UINTW BitCnt;               // next *BitPtr bit, 7(hi bit) to 0(low bit)
    UINTW ByteCnt;              // bytes remaining in blk (including *BitPtr)
} JG_BIT_STREAM;

typedef struct {                // Vector description structure
    INT16 JGPTR Codebook;       // 4x4 block code book table
    UINT8 JGPTR Data;           // ptr to vector of CodeBook indexes
} VQ_DATA;

typedef struct {                // edge information structure
    UINT16 FirstBlock;          // absolute 1st-panel-block index
    UINT16 BlocksPerPanel;      // blocks per panel
    UINT16 NEdges;              // ??? undocumented ???
    UINT16 BlocksPerRow;        // number of blocks per panel row
    UINT16 NextEdgeBlock;       // next edge block's absolute position
    UINT16 JGPTR Offsets;       // ptr to offsets of following blocks
} EDGE_INFO;

// Portable color representations - when viewed as 32-bit values:
//
//   JG0RGB:   [31..24] = 0, [23..16] = Red, [15..8] = Green, [7..0] = Blue.
//   JG0YUV:   [31..24] = 0, [23..16] = Y,   [15..8] = U,     [7..0] = V.
//   JG0RGB16: [15..15] = 0, [14..10] = Red, [ 9..5] = Green, [4..0] = Blue.

typedef UINT32 JG0RGB;
typedef UINT32 JG0YUV;
typedef UINT16 JG0RGB16;

// Structures for palettes, color data, ...
// If you are going to used these in arrays, (for palettes or images),
// make sure the 3 byte structures are packed tightly.

typedef struct {        // Used to define an RGB color in 3 bytes.
    UINT8 red;           
    UINT8 green;
    UINT8 blue;
} JG_RGB;

typedef struct {        // Used to define an RGB color in 4 bytes.
    UINT8 red;
    UINT8 green;
    UINT8 blue;
    UINT8 flags;        // Don't care field. Named "flags" after Windows.
} JG_RGBX;

typedef struct {        // Used to define a color in 3 bytes. (RGB backwards)
    UINT8 blue;
    UINT8 green;
    UINT8 red;
} JG_BGR;

typedef struct {        // Used to define a color in 4 bytes. (RGB backwards)
    UINT8 blue;
    UINT8 green;
    UINT8 red;          
    UINT8 flags;        // Don't care field.  Named "flags" after windows.
} JG_BGRX;

typedef struct {
    UINT8 y;            // Luminance Component.
    UINT8 u;            // U. Color difference. Usually offset by +128.
    UINT8 v;            // V. Color difference. Usually offset by +128.
} JG_YUV;

// JG_CCCXPAL is used to define 4 byte palette entry where the first three
// components may be any colorspace/order and the fourth is ignored.
typedef struct {
	UINT8   c0;
	UINT8   c1;
	UINT8   c2;
	UINT8   flags;
} JG_CCCXPAL;
     
// JG_CCCPAL is used to define 3 byte palette entry where the components 
// may be any colorspace/order.
typedef struct {
	UINT8   c0;
	UINT8   c1;
	UINT8   c2;
} JG_CCCPAL;
     
#ifdef __cplusplus
  }
#endif

#endif

/*----------------------------------------------------------------------------
;
; End of jgtypes.h
;
;---------------------------------------------------------------------------*/
