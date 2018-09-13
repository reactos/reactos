#include	"types.h"
#include	"cvtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned short _segment;

typedef uint bool_t;

#if !defined(HIBYTE)

#define HIBYTE(a)	(*(((unsigned char FAR *) &(a)) + 1))
#define LOBYTE(a)	(*((unsigned char FAR *) &(a)))

#endif




// these typedefs define the pointers to the cvinfo.h structures

typedef TYPTYPE 		FAR *TYPPTR;
typedef lfEasy			FAR *plfEasy;
typedef lfModifier		FAR *plfModifier;
typedef lfPointer		FAR *plfPointer;
typedef lfArray 		FAR *plfArray;
typedef lfClass 		FAR *plfClass;
typedef lfStructure 	FAR *plfStructure;
typedef lfUnion 		FAR *plfUnion;
typedef lfEnum			FAR *plfEnum;
typedef lfProc			FAR *plfProc;
typedef lfMFunc 		FAR *plfMFunc;
typedef lfVTShape		FAR *plfVTShape;
typedef lfCobol0		FAR *plfCobol0;
typedef lfCobol1		FAR *plfCobol1;
typedef lfBArray		FAR *plfBArray;
typedef lfDimArray		FAR *plfDimArray;
typedef lfVFTPath		FAR *plfVFTPath;
typedef lfLabel 		FAR *plfLabel;
typedef lfSkip			FAR *plfSkip;
typedef lfArgList		FAR *plfArgList;
typedef lfDerived		FAR *plfDerived;
typedef lfDefArg		FAR *plfDefArg;
typedef lfList			FAR *plfList;
typedef lfFieldList 	FAR *plfFieldList;
typedef mlMethod		FAR *pmlMethod;
typedef lfMethodList	FAR *plfMethodList;
typedef lfBitfield		FAR *plfBitfield;
typedef lfDimCon		FAR *plfDimCon;
typedef lfDimVar		FAR *plfDimVar;
typedef lfRefSym		FAR *plfRefSym;
typedef lfChar			FAR *plfChar;
typedef lfShort 		FAR *plfShort;
typedef lfUShort		FAR *plfUShort;
typedef lfLong			FAR *plfLong;
typedef lfULong 		FAR *plfULong;
typedef lfReal32		FAR *plfReal32;
typedef lfReal48		FAR *plfReal48;
typedef lfReal64		FAR *plfReal64;
typedef lfReal80		FAR *plfReal80;
typedef lfReal128		FAR *plfReal128;
typedef lfIndex 		FAR *plfIndex;
typedef lfIndex 		FAR *nplfIndex;
typedef lfBClass		FAR *plfBClass;
typedef lfVBClass		FAR *plfVBClass;
typedef lfFriendCls 	FAR *plfFriendCls;
typedef lfFriendFcn 	FAR *plfFriendFcn;
typedef lfMember		FAR *plfMember;
typedef lfSTMember		FAR *plfSTMember;
typedef lfVFuncTab		FAR *plfVFuncTab;
typedef lfMethod		FAR *plfMethod;
typedef lfOneMethod		FAR *plfOneMethod;
typedef lfEnumerate 	FAR *plfEnumerate;
typedef lfNestType		FAR *plfNestType;

typedef SYMTYPE 		FAR *SYMPTR;
typedef CFLAGSYM		FAR *CFLAGPTR;
typedef CONSTSYM		FAR *CONSTPTR;
typedef REGSYM			FAR *REGPTR;
typedef UDTSYM			FAR *UDTPTR;
typedef SEARCHSYM		FAR *SEARCHPTR;
typedef BLOCKSYM16		FAR *BLOCKPTR16;
typedef DATASYM16		FAR *DATAPTR16;
typedef PUBSYM16		FAR *PUBPTR16;
typedef LABELSYM16		FAR *LABELPTR16;
typedef BPRELSYM16		FAR *BPRELPTR16;
typedef PROCSYM16		FAR *PROCPTR16;
typedef THUNKSYM16		FAR *THUNKPTR16;
typedef CEXMSYM16		FAR *CEXMPTR16;
typedef VPATHSYM16		FAR *VPATHPTR16;
typedef WITHSYM16		FAR *WITHPTR16;

typedef BLOCKSYM32		FAR *BLOCKPTR32;
typedef DATASYM32		FAR *DATAPTR32;
typedef PUBSYM32		FAR *PUBPTR32;
typedef LABELSYM32		FAR *LABELPTR32;
typedef BPRELSYM32		FAR *BPRELPTR32;
typedef PROCSYM32		FAR *PROCPTR32;
typedef PROCSYMMIPS		FAR *PROCPTRMIPS;
typedef THUNKSYM32		FAR *THUNKPTR32;
typedef WITHSYM32		FAR *WITHPTR32;
typedef VPATHSYM32		FAR *VPATHPTR32;

typedef BLOCKSYM		FAR *BLOCKPTR;
typedef PROCSYM 		FAR *PROCPTR;
typedef THUNKSYM		FAR *THUNKPTR;
typedef WITHSYM 		FAR *WITHPTR;

typedef REGSYMIA64      FAR *REGPTRIA64;
typedef REGRELIA64      FAR *LPREGRELIA64;
typedef PROCSYMIA64     FAR *PROCPTRIA64;

#ifdef __cplusplus
} // extern "C" {
#endif
