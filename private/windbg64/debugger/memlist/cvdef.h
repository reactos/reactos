/***    cvdef.h - header for cv global symbol definitions.
 *
 *      Copyright <C> 1989, Microsoft Corporation
 *
 *      Purpose:
 *
 */

#include "version.h"
#include "nothunk.h"

#define LOCAL static
#define CC
#define CV3
#define HELP_BUTTON

#ifdef OS2
#ifndef DOS5
#define DOS5
#endif

#ifdef DOS5
#define INCL_NOPM
#define _MAX_CVPATH   259     /* max. length of full pathname */
#define _MAX_CVDRIVE    3     /* max. length of drive component */
#define _MAX_CVDIR    257     /* max. length of path component */
#define _MAX_CVFNAME  257     /* max. length of file name component */
#define _MAX_CVEXT    257     /* max. length of extension component */

#else

#define _MAX_CVPATH  144      /* max. length of full pathname */
#define _MAX_CVDRIVE   3      /* max. length of drive component */
#define _MAX_CVDIR   130      /* max. length of path component */
#define _MAX_CVFNAME   9      /* max. length of file name component */
#define _MAX_CVEXT     5      /* max. length of extension component */

#endif


#define Unreferenced( x )   ((void)x)   // for unreferenced local variables


typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned long   ulong;

typedef unsigned long   CV_uoff32_t;
typedef          long   CV_off32_t;
typedef unsigned short  CV_uoff16_t;
typedef          short  CV_off16_t;

#if defined (ADDR_16)
// we are operating as a 16:16 evaluator only
// the address packet will be defined as an offset and a 16 bit filler
typedef     CV_off16_t     OFFSET;
typedef     CV_uoff16_t    UOFFSET;
#else
typedef     CV_off32_t     OFFSET;
typedef     CV_uoff32_t    UOFFSET;
#endif


#ifdef NEVER

// these typedefs define the pointers to the cvinfo.h structures

typedef TYPTYPE         far *TYPPTR;
typedef lfEasy          far *plfEasy;
typedef lfModifier      far *plfModifier;
typedef lfPointer       far *plfPointer;
typedef lfArray         far *plfArray;
typedef lfClass         far *plfClass;
typedef lfStructure     far *plfStructure;
typedef lfUnion         far *plfUnion;
typedef lfEnum          far *plfEnum;
typedef lfProc          far *plfProc;
typedef lfMFunc         far *plfMFunc;
typedef lfVTShape       far *plfVTShape;
typedef lfCobol0        far *plfCobol0;
typedef lfCobol1        far *plfCobol1;
typedef lfBArray        far *plfBArray;
typedef lfDimArray      far *plfDimArray;
typedef lfVFTPath       far *plfVFTPath;
typedef lfLabel         far *plfLabel;
typedef lfSkip          far *plfSkip;
typedef lfArgList       far *plfArgList;
typedef lfDerived       far *plfDerived;
typedef lfDefArg        far *plfDefArg;
typedef lfList          far *plfList;
typedef lfFieldList     far *plfFieldList;
typedef mlMethod        far *pmlMethod;
typedef lfMethodList    far *plfMethodList;
typedef lfBitfield      far *plfBitfield;
typedef lfDimCon        far *plfDimCon;
typedef lfDimVar        far *plfDimVar;
typedef lfRefSym        far *plfRefSym;
typedef lfChar          far *plfChar;
typedef lfShort         far *plfShort;
typedef lfUShort        far *plfUShort;
typedef lfLong          far *plfLong;
typedef lfULong         far *plfULong;
typedef lfReal32        far *plfReal32;
typedef lfReal48        far *plfReal48;
typedef lfReal64        far *plfReal64;
typedef lfReal80        far *plfReal80;
typedef lfReal128       far *plfReal128;
typedef lfIndex         far *plfIndex;
typedef lfIndex         far *nplfIndex;
typedef lfBClass        far *plfBClass;
typedef lfVBClass       far *plfVBClass;
typedef lfIVBClass      far *plfIVBClass;
typedef lfFriend        far *plfFriend;
typedef lfMember        far *plfMember;
typedef lfSTMember      far *plfSTMember;
typedef lfVFuncTab      far *plfVFuncTab;
typedef lfMethod        far *plfMethod;
typedef lfEnumerate     far *plfEnumerate;
typedef lfNestType      far *plfNestType;

typedef SYMTYPE         far *SYMPTR;
typedef CFLAGSYM        far *CFLAGPTR;
typedef CONSTSYM        far *CONSTPTR;
typedef REGSYM          far *REGPTR;
typedef UDTSYM          far *UDTPTR;
typedef SEARCHSYM       far *SEARCHPTR;
typedef BLOCKSYM16      far *BLOCKPTR16;
typedef DATASYM16       far *DATAPTR16;
typedef PUBSYM16        far *PUBPTR16;
typedef LABELSYM16      far *LABELPTR16;
typedef BPRELSYM16      far *BPRELPTR16;
typedef PROCSYM16       far *PROCPTR16;
typedef THUNKSYM16      far *THUNKPTR16;
typedef CEXMSYM16       far *CEXMPTR16;
typedef VPATHSYM16      far *VPATHPTR16;
typedef WITHSYM16       far *WITHPTR16;

typedef BLOCKSYM32      far *BLOCKPTR32;
typedef DATASYM32       far *DATAPTR32;
typedef PUBSYM32        far *PUBPTR32;
typedef LABELSYM32      far *LABELPTR32;
typedef BPRELSYM32      far *BPRELPTR32;
typedef PROCSYM32       far *PROCPTR32;
typedef THUNKSYM32      far *THUNKPTR32;
typedef WITHSYM32       far *WITHPTR32;
typedef VPATHSYM32      far *VPATHPTR32;

typedef BLOCKSYM        far *BLOCKPTR;
typedef PROCSYM         far *PROCPTR;
typedef THUNKSYM        far *THUNKPTR;
typedef WITHSYM         far *WITHPTR;


#endif
