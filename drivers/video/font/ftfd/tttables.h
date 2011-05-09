
/* We need these to access unaligned big endian data */
FORCEINLINE
USHORT GETW(PVOID pv)
{
    PBYTE pj = pv;
    return (((USHORT)pj[0]) << 8) | ((USHORT)pj[1]);
}

FORCEINLINE
ULONG GETD(PVOID pv)
{
    PBYTE pj = pv;
    return (((ULONG)pj[0]) << 24) | (((ULONG)pj[1]) << 16) |
           (((ULONG)pj[2]) << 8) | ((ULONG)pj[3]);
}

#include <pshpack1.h>

typedef struct _TT_COLLECTION
{
    ULONG ulTTCTag;
    ULONG ulVersion;
    ULONG ulNumFonts;
    ULONG aulOffsetTable[1];
} TT_COLLECTION, *PTT_COLLECTION;

typedef struct _TT_TABLE_ENTRY
{
    ULONG ulTag;
    ULONG ulCheckSum;
    ULONG ulOffset;
    ULONG ulLength;
} TT_TABLE_ENTRY, *PTT_TABLE_ENTRY;

typedef struct _TT_FILE_HEADER
{
    ULONG ulIdentifier;
    USHORT usNumTables;
    USHORT usSearchRange;
    USHORT usEntrySelector;
    USHORT usRangeshift;

    TT_TABLE_ENTRY aTableEntries[1];

} TT_FILE_HEADER, *PTT_FILE_HEADER;

typedef struct _TT_OS2_DATA
{
    USHORT 	version; // 	0x0004
    SHORT 	xAvgCharWidth;
    USHORT 	usWeightClass;
    USHORT 	usWidthClass;
    USHORT 	fsType;
    SHORT 	ySubscriptXSize;
    SHORT 	ySubscriptYSize;
    SHORT 	ySubscriptXOffset;
    SHORT 	ySubscriptYOffset;
    SHORT 	ySuperscriptXSize;
    SHORT 	ySuperscriptYSize;
    SHORT 	ySuperscriptXOffset;
    SHORT 	ySuperscriptYOffset;
    SHORT 	yStrikeoutSize;
    SHORT 	yStrikeoutPosition;
    union // 0x30
    {
        struct
        {
            BYTE jClassId;
            BYTE jSubClassId;
        };
        SHORT 	sFamilyClass;
    };
    BYTE 	panose[10];
    ULONG 	ulUnicodeRange1; // 	Bits 0-31
    ULONG 	ulUnicodeRange2; // 	Bits 32-63
    ULONG 	ulUnicodeRange3; // 	Bits 64-95
    ULONG 	ulUnicodeRange4; // 	Bits 96-127
    CHAR 	achVendID[4];
    USHORT 	fsSelection;
    USHORT 	usFirstCharIndex;
    USHORT 	usLastCharIndex;
    SHORT 	sTypoAscender;
    SHORT 	sTypoDescender;
    SHORT 	sTypoLineGap;
    USHORT 	usWinAscent;
    USHORT 	usWinDescent;
    ULONG 	ulCodePageRange1; // 	Bits 0-31
    ULONG 	ulCodePageRange2; // 	Bits 32-63
    SHORT 	sxHeight;
    SHORT 	sCapHeight;
    USHORT 	usDefaultChar;
    USHORT 	usBreakChar;
    USHORT 	usMaxContext;
} TT_OS2_DATA, *PTT_OS2_DATA;

typedef struct _TT_KERNPAIR
{
    USHORT usLeft;
    USHORT usRight;
    FWORD fwdValue;
} TT_KERNPAIR, *PTT_KERNPAIR;

typedef struct _TT_KERNING_FORMAT_0
{
    USHORT nPairs;
    USHORT usSearchRange;
    USHORT usEntrySelector;
    USHORT usRangeShift;
    TT_KERNPAIR akernpair[1];
} TT_KERNING_FORMAT_0, *PTT_KERNING_FORMAT_0;

typedef struct _TT_KERNING_SUBTABLE
{
    USHORT usVersion;
    USHORT usLength;
    USHORT usCoverage;
    ////
    TT_KERNING_FORMAT_0 format0;
} TT_KERNING_SUBTABLE, *PTT_KERNING_SUBTABLE;

typedef struct _TT_KERNING_TABLE
{
    USHORT usVersion;
    USHORT nTables;
    ////
    TT_KERNING_SUBTABLE subtable;
} TT_KERNING_TABLE, *PTT_KERNING_TABLE;

#include <poppack.h>
