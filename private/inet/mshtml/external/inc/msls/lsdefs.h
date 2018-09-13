/* Common definitions for line services
 */

#ifndef LSDEFS_DEFINED
#define LSDEFS_DEFINED

#ifdef UNIX
#include <wchar.h>
#endif

#ifndef WINVER	/* defined in <windows.h> */

/* <windows.h> must be included FIRST, if at all. */
/* We define basic types if <windows.h> is not included. */

#ifndef NULL
#define NULL    ((void *)0)
#endif /* NULL */

#define WINAPI __stdcall
#define FALSE	0
#define TRUE	1

typedef int BOOL;
typedef long LONG;
typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef unsigned int UINT;
typedef unsigned char BYTE;
typedef int INT;
#ifdef UNIX
typedef wchar_t WCHAR;
#else
typedef WORD WCHAR;
#endif
typedef const WCHAR* LPCWSTR;
typedef WCHAR* LPWSTR;

typedef struct tagRECT
{
    LONG    left;
    LONG    top;
    LONG    right;
    LONG    bottom;
} RECT;

typedef struct tagPOINT
{
    LONG  x;
    LONG  y;
} POINT;

#endif /* WINVER */


/* Line services definitions */
struct lscontext;					/* Opaque to clients */
typedef struct lscontext* PLSC;
typedef const struct lscontext* PCLSC;

struct ols;								/* Owner of LineServices */
typedef struct ols* POLS;				/*  (Opaque to LineService) */

typedef long LSCP;
typedef DWORD LSDCP;

typedef WORD GINDEX;
typedef GINDEX* PGINDEX;
typedef const GINDEX* PCGINDEX;

typedef struct tagPOINTUV
{
    LONG  u;
    LONG  v;
} POINTUV;

typedef POINTUV* PPOINTUV;
typedef const POINTUV* PCPOINTUV;

typedef struct tagGOFFSET
{
    LONG  du;
    LONG  dv;
} GOFFSET;

typedef GOFFSET* PGOFFSET;
typedef const GOFFSET* PCGOFFSET;

/* Line services error codes */
typedef long LSERR;
#define lserrNone						( 0L)
#ifdef LSERRSTOP				/* stop immediately, don't return error */
#define lserrInvalidParameter   		AssertErr("lserrInvalidParameter")  	
#define lserrOutOfMemory    			AssertErr("lserrOutOfMemory")
#define lserrNullOutputParameter  		AssertErr("lserrNullOutputParameter") 
#define lserrInvalidContext    			AssertErr("lserrInvalidContext") 
#define lserrInvalidLine    			AssertErr("lserrInvalidLine")
#define lserrInvalidDnode    			AssertErr("lserrInvalidDnode")    
#define lserrInvalidDeviceResolution 	AssertErr("lserrInvalidDeviceResolution")
#define lserrInvalidRun  			   	AssertErr("lserrInvalidRun")
#define lserrMismatchLineContext  		AssertErr("lserrMismatchLineContext")
#define lserrContextInUse    			AssertErr("lserrContextInUse")
#define lserrDuplicateSpecialCharacter 	AssertErr("lserrDuplicateSpecialCharacter")
#define lserrInvalidAutonumRun  		AssertErr("lserrInvalidAutonumRun")
#define lserrFormattingFunctionDisabled AssertErr("lserrFormattingFunctionDisabled")
#define lserrUnfinishedDnode   			AssertErr("lserrUnfinishedDnode")
#define lserrInvalidDnodeType   		AssertErr("lserrInvalidDnodeType")
#define lserrInvalidPenDnode   			AssertErr("lserrInvalidPenDnode")   
#define lserrInvalidNonPenDnode   		AssertErr("lserrInvalidNonPenDnode")
#define lserrInvalidBaselinePenDnode 	AssertErr("lserrInvalidBaselinePenDnode")
#define lserrInvalidFormatterResult  	AssertErr("lserrInvalidFormatterResult")
#define lserrInvalidObjectIdFetched  	AssertErr("lserrInvalidObjectIdFetched")
#define lserrInvalidDcpFetched   		AssertErr("lserrInvalidDcpFetched")
#define lserrInvalidCpContentFetched 	AssertErr("lserrInvalidCpContentFetched")
#define lserrInvalidBookmarkType  		AssertErr("lserrInvalidBookmarkType")
#define lserrSetDocDisabled    			AssertErr("lserrSetDocDisabled")
#define lserrFiniFunctionDisabled  		AssertErr("lserrFiniFunctionDisabled")
#define lserrCurrentDnodeIsNotTab  		AssertErr("lserrCurrentDnodeIsNotTab")
#define lserrPendingTabIsNotResolved	AssertErr("lserrPendingTabIsNotResolved")
#define lserrWrongFiniFunction 			AssertErr("lserrWrongFiniFunction")
#define lserrInvalidBreakingClass		AssertErr("lserrInvalidBreakingClass")
#define lserrBreakingTableNotSet		AssertErr("lserrBreakingTableNotSet")
#define lserrInvalidModWidthClass		AssertErr("lserrInvalidModWidthClass")
#define lserrModWidthPairsNotSet		AssertErr("lserrModWidthPairsNotSet")
#define lserrWrongTruncationPoint 		AssertErr("lserrWrongTruncationPoint")
#define lserrWrongBreak 				AssertErr("lserrWrongBreak")
#define lserrDupInvalid 				AssertErr("lserrDupInvalid")
#define lserrRubyInvalidVersion			AssertErr("lserrRubyVersionInvalid")
#define lserrTatenakayokoInvalidVersion	AssertErr("lserrTatenakayokoInvalidVersion")
#define lserrWarichuInvalidVersion		AssertErr("lserrWarichuInvalidVersion")
#define lserrWarichuInvalidData			AssertErr("lserrWarichuInvalidData")
#define lserrCreateSublineDisabled		AssertErr("lserrCreateSublineDisabled")
#define lserrCurrentSublineDoesNotExist	AssertErr("lserrCurrentSublineDoesNotExist")
#define lserrCpOutsideSubline			AssertErr("lserrCpOutsideSubline")
#define lserrHihInvalidVersion			AssertErr("lserrHihInvalidVersion")
#define lserrInsufficientQueryDepth		AssertErr("lserrInsufficientQueryDepth")
#define lserrInsufficientBreakRecBuffer	AssertErr("lserrInsufficientBreakRecBuffer")
#define lserrInvalidBreakRecord			AssertErr("lserrInvalidBreakRecord")
#define lserrInvalidPap					AssertErr("lserrInvalidPap")
#define lserrContradictoryQueryInput	AssertErr("lserrContradictoryQueryInput")
#define lserrLineIsNotActive			AssertErr("lserrLineIsNotActive")
#define lserrTooLongParagraph			AssertErr("lserrTooLongParagraph")
#else
#define lserrInvalidParameter			(-1L)
#define lserrOutOfMemory				(-2L)
#define lserrNullOutputParameter		(-3L)
#define lserrInvalidContext				(-4L)
#define lserrInvalidLine				(-5L)
#define lserrInvalidDnode				(-6L)
#define lserrInvalidDeviceResolution	(-7L)
#define lserrInvalidRun					(-8L)
#define lserrMismatchLineContext		(-9L)
#define lserrContextInUse				(-10L)
#define lserrDuplicateSpecialCharacter	(-11L)
#define lserrInvalidAutonumRun			(-12L)
#define lserrFormattingFunctionDisabled	(-13L)
#define lserrUnfinishedDnode			(-14L)
#define lserrInvalidDnodeType			(-15L)
#define lserrInvalidPenDnode			(-16L)
#define lserrInvalidNonPenDnode			(-17L)
#define lserrInvalidBaselinePenDnode	(-18L)
#define lserrInvalidFormatterResult		(-19L)
#define lserrInvalidObjectIdFetched		(-20L)
#define lserrInvalidDcpFetched			(-21L)
#define lserrInvalidCpContentFetched	(-22L)
#define lserrInvalidBookmarkType		(-23L)
#define lserrSetDocDisabled				(-24L)
#define lserrFiniFunctionDisabled		(-25L)
#define lserrCurrentDnodeIsNotTab		(-26L)
#define lserrPendingTabIsNotResolved    (-27L)
#define lserrWrongFiniFunction 			(-28L)
#define lserrInvalidBreakingClass		(-29L)
#define lserrBreakingTableNotSet		(-30L)
#define lserrInvalidModWidthClass		(-31L)
#define lserrModWidthPairsNotSet		(-32L)
#define lserrWrongTruncationPoint 		(-33L)
#define lserrWrongBreak 				(-34L)
#define	lserrDupInvalid					(-35L)
#define lserrRubyInvalidVersion			(-36L)
#define lserrTatenakayokoInvalidVersion	(-37L)
#define lserrWarichuInvalidVersion		(-38L)
#define lserrWarichuInvalidData			(-39L)
#define lserrCreateSublineDisabled		(-40L)
#define lserrCurrentSublineDoesNotExist	(-41L)
#define lserrCpOutsideSubline			(-42L)
#define lserrHihInvalidVersion			(-43L)
#define lserrInsufficientQueryDepth		(-44L)
#define lserrInsufficientBreakRecBuffer	(-45L)
#define lserrInvalidBreakRecord			(-46L)
#define lserrInvalidPap					(-47L)
#define lserrContradictoryQueryInput	(-48L)
#define lserrLineIsNotActive			(-49L)
#define lserrTooLongParagraph			(-50L)
#endif	/* LSERRORSTOP */


#ifndef fTrue
#define fTrue	1
#define fFalse	0
#endif

#define uLsInfiniteRM	0x3FFFFFFF

#define czaUnitInch					(1440L) /* 1440 absolute units per inch */

#endif /* LSDEFS_DEFINED */
