//
// comndata.h
//
// Common to inflate and deflate
//
#ifndef _COMNDATA_H
#define _COMNDATA_H

#ifdef DECLARE_DATA

const BYTE g_CodeOrder[] = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
const BYTE g_ExtraLengthBits[] = {0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};
const BYTE g_ExtraDistanceBits[] = {0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13,0,0};
const ULONG g_LengthBase[] = {3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258};
const ULONG g_DistanceBasePosition[] = {1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,
257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577,0,0};
const ULONG g_BitMask[] = {0,1,3,7,15,31,63,127,255,511,1023,2047,4095,8191,16383,32767};

BYTE    g_LengthLookup[256];
BYTE    g_DistLookup[512];

BYTE    g_StaticLiteralTreeLength[MAX_LITERAL_TREE_ELEMENTS];
USHORT  g_StaticLiteralTreeCode[MAX_LITERAL_TREE_ELEMENTS];

// note: g_StaticDistanceTreeLength == 5 for all distances, which is why we don't have a
// table for that
USHORT  g_StaticDistanceTreeCode[MAX_DIST_TREE_ELEMENTS];

BOOL    g_InitialisedStaticBlock = FALSE;

#else /* !DECLARE_DATA */

extern const BYTE g_CodeOrder[19];
extern const BYTE g_ExtraLengthBits[];
extern const BYTE g_ExtraDistanceBits[];
extern const ULONG g_LengthBase[];
extern const ULONG g_DistanceBasePosition[];
extern const ULONG g_BitMask[];

extern BYTE     g_LengthLookup[256];
extern BYTE     g_DistLookup[512];

extern BYTE     g_StaticLiteralTreeLength[MAX_LITERAL_TREE_ELEMENTS];
extern USHORT   g_StaticLiteralTreeCode[MAX_LITERAL_TREE_ELEMENTS];
extern USHORT   g_StaticDistanceTreeCode[MAX_DIST_TREE_ELEMENTS];

extern BOOL     g_InitialisedStaticBlock;

#endif /* !DECLARE_DATA */

#endif /* _COMNDATA_H */

