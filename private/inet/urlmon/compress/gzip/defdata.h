//
// defdata.h
//
// Data specific to deflate
//
// BUGBUG Some of these USHORTs could be made into ULONGs for speed-of-access.  The disadvantage would
//        be greater memory/cache usage.  g_StaticDistanceTreeCode[] could be made into a BYTE array,
//        since the codes are 5 bits.  Changes of this nature will require various code changes elsewhere.
//
#ifdef DECLARE_DATA

// lookup tables for finding the slot number of a match length or distance
BYTE    g_LengthLookup[256];
BYTE    g_DistLookup[512];

// literal codes for static blocks
BYTE    g_StaticLiteralTreeLength[MAX_LITERAL_TREE_ELEMENTS];
USHORT  g_StaticLiteralTreeCode[MAX_LITERAL_TREE_ELEMENTS];

// distance codes for static blocks
// note: g_StaticDistanceTreeLength == 5 for all distances, which is why we don't have a table for that
USHORT  g_StaticDistanceTreeCode[MAX_DIST_TREE_ELEMENTS];

// cached tree structure output for fast encoder
BYTE    g_FastEncoderTreeStructureData[MAX_TREE_DATA_SIZE];
int     g_FastEncoderTreeLength; // # bytes in g_FastEncoderTreeStructureData
ULONG   g_FastEncoderPostTreeBitbuf; // final value of bitbuf
int     g_FastEncoderPostTreeBitcount; // final value of bitcount

#else /* !DECLARE_DATA */

extern BYTE     g_LengthLookup[256];
extern BYTE     g_DistLookup[512];

extern BYTE     g_StaticLiteralTreeLength[MAX_LITERAL_TREE_ELEMENTS];
extern USHORT   g_StaticLiteralTreeCode[MAX_LITERAL_TREE_ELEMENTS];
extern USHORT   g_StaticDistanceTreeCode[MAX_DIST_TREE_ELEMENTS];

extern BYTE     g_FastEncoderTreeStructureData[MAX_TREE_DATA_SIZE];
extern int      g_FastEncoderTreeLength;
extern ULONG    g_FastEncoderPostTreeBitbuf;
extern int      g_FastEncoderPostTreeBitcount;

#endif /* !DECLARE_DATA */
