/*
 *	TrueType File Format Structure Definitions
 *
 * Copyright (c) 1997-1999 Microsoft Corporation.
 */
/*  Whole :
	+-----------------------+
	| Header		|
	+-----------------------+
	| Tables		|
	|			|
		    |
	|			|
	+-----------------------+
    Header :
	----------------------------------------------------------
	  Fixed		sfnt version	0x00010000
	  ushort	numTables	Number of tables
	  ushort	searchRange	(n^2 <= numtables)*16
	  ushort	entrySelector	Log2(n^2<=numTables)
	  ushort	rangeShift	NumTables*16-searchRange
	     Table Directory Entries
	----------------------------------------------------------
    Table Directory:
	----------------------------------------------------------
	  ulong		tag		4-byte idetifier
	  ulong		checkSum	CheckSum for this table
	  ulong		offset		offset from beginning of
					TrueType font file
	  ulong		length		Length of this table
	----------------------------------------------------------

    Tables:
	----------------------------------------------------------
	  cmap		character to glyph mapping
	  glyf		glyph data
	  head		font header
	  hhea		horizontal header
	  hmtx		hosizontal metrics
	  loca		index to location
	  maxp		maximum profile
	  name		naming table
	  post		PostScript information
	  OS/2		OS/2 and Windows specific metrics
		--------------------------
	  cvt		Control Value Table
	  fpgm		font program
	  hdmx		hosizontal device metrics
	  kern		kerning
	  LTSH		Linear threshold table
	  prep		CVT Program
	  WIN 		(reserved)
	  VDMX		Vertical Device Metrics table
	  FOCA		Reserved for IBM Font Object Content Architecture data
	  PCLT		PCL 5 table
	  mort		glyph metamorphosis table
		--------------------------
	  vhea		Vertical Header table
	  vmtx		Vertical metrics table
	----------------------------------------------------------
*/
#define		TAGSIZ	4
struct TTFHeader {
	char	sfnt_version[4];
	short	numTables;
	short	searchRange;
	short	entrySelector;
	short	rangeShift;
	};
struct TableEntry {
	char	tagName[TAGSIZ];
	unsigned long	checkSum;
	long	ofs;
	long	siz;
	};


/***
 *	cmap
 ***/
struct CmapHead {
	short	version;
	short	nTbls;
	};

struct CmapEncodingTbl {
	short	PlatformID;
	short	PlatformSpecEncID;
	long	ofs;
	};

struct CmapSubtable {
	short	format;
	short	length;
	short	version;
	short	segCnt2;
	short	searchRange;
	short	rangeShift;
	short	endCnt[2];
	short	reservedPad;
	short	startCnt[2];
	short	idDelta[2];
	short	idRangeOfs[2];
	};
/***
 *	maxp
 ***/
struct MaxpTbl {
	char	version[4];
	short	numGlyph;
	short	maxPoints;
	short	maxContours;
	short	maxCompositePoints;
	short	maxCompositeContours;
	short	maxZones;
	short	maxTwilightPoints;
	short	maxStorage;
	short	maxFunctionDefs;
	short	maxInstructiondefs;
	short	maxStackElements;
	short	maxSizeOfInstructions;
	short	maxComponentElements;
	short	maxComponentDepth;
	};
/***
 *	name
 ***/
struct NamingTable {
	short	FormSel;
	short	NRecs;
	short	OfsToStr;
	/* Following NameRecords */
	};
struct NameRecord {
	short	PlatformID;
	short	PlatformSpecEncID;
	short	LanguageID;
	short	NameID;
	short	StringLength;
	short	StringOfs;
	};
/***
 *	head
 ***/
struct HeadTable {
	char	version[4];		/* 0x00010000 */
	char	revision[4];
	unsigned long	chkSum;
	unsigned long	magicNumber;	/* 0x5F0F3CF5 */
	short	flags;
	short	unitsPerEm;
	char	createdDate[8];
	char	updatedDate[8];
	short	xMin;
	short	yMin;
	short	xMax;
	short	yMax;
	short	macStyle;
	short	lowestRecPPEM;
	short	fontDirectionHint;
	short	indexToLocFormat;
	short	glyphDataFormat;	/* 0*/
	};
/***
 *	hhea
 ***/
struct HheaTable	{
	char	version[4];
	short	Ascender;
	short	Descender;
	short	LineGap;
	short	advanceWidthMax;
	short	minLeftSideBearing;
	short	minRightSideBearing;
	short	xMaxExtent;
	short	caretSlopeRise;
	short	caretSlopeRun;
	short	reserved[5];
	short	metricDataFormat;
	short	numberOfHMetrics;
	};
/***
 *	hmtx
 ***/
 struct HMetrics {
	short	advanceWidth;
	short	leftSideBearing;
	};
/***
 *	vhea
 ***/
struct VheaTable	{
	char	version[4];
	short	Ascender;
	short	Descender;
	short	LineGap;
	short	advanceHeightMax;
	short	minTopSideBearing;
	short	minBottomSideBearing;
	short	yMaxExtent;
	short	caretSlopeRise;
	short	caretSlopeRun;
	short	caretOffset;
	short	reserved[4];
	short	metricDataFormat;
	short	numOfLongVerMetrics;
	};
/***
 *	vmtx
 ***/
 struct VMetrics {
	short	advanceHeight;
	short	topSideBearing;
	};
/***
 *	post
 ***/
struct postTable {
	char	FormatType[4];	/* 00030000 */
	long	italicAngle;
	short	underlinePosition;
	short	underlineThickness;
	unsigned long	isFixedPitch;
	unsigned long	minMemType42;
	unsigned long	maxMemType42;
	unsigned long	minMemType1;
	unsigned long	maxMemType1;
	};
/***
 *	OS/2
 ***/
/*
typedef struct {
	char	bFamily;
	char	bSerifStyle;
	char	bWeight;
	char	bProportion;
	char	bContrast;
	char	bStrokeVariation;
	char	bArmStyle;
	char	bLetterform;
	char	bMidline;
	char	bXHeight;
	} PANOSE;
*/
struct OS2Table {
	unsigned short	version;
	short	xAvgCharWidth;
	unsigned short	usWeightClass;
	unsigned short	usWidthClass;
	short	fsType;
	short	ySubscriptXSize;
	short	ySubscriptYSize;
	short	ySubscriptXOffset;
	short	ySubscriptYOffset;
	short	ySuperscriptXSize;
	short	ySuperscriptYSize;
	short	ySuperscriptXOffset;
	short	ySuperscriptYOffset;
	short	yStrikeoutSize;
	short	yStrikeoutPosition;
	short	sFamilyClass;
	PANOSE	panose;
	unsigned long	ulCharRange[4];
	char	achVendID[4];
	unsigned short	fsSelection;
	unsigned short	usFirstCharIndex;
	unsigned short	usLastCharIndex;
	short	sTypoAscender;
	short	sTypoDescender;
	short	sTypoLineGap;
	unsigned short	usWinAscent;
	unsigned short	usWinDescent;
	};
/***
 *	glyf
 ***/
struct glyfHead {
	short	numberOfContour;
	short	xMin, yMin;
	short	xMax, yMax;
	};
/** glyf data flag definition **/
	
#define	GLYF_ON_CURVE	0x01
#define	GLYF_X_SHORT	0x02	/* x is short */
#define	GLYF_Y_SHORT	0x04	/* y is short */
#define	GLYF_X_SHORT_N	0x02	/* x is short & negative */
#define	GLYF_Y_SHORT_N	0x04	/* y is short & negative */
#define	GLYF_X_SHORT_P	0x12	/* x is short & positive */
#define	GLYF_Y_SHORT_P	0x24	/* y is short & positive */
#define	GLYF_REPEAT	0x08
#define	GLYF_X_SAME	0x10
#define	GLYF_Y_SAME	0x20

/***
 *	Bounding Box ( Not for file structure )
 ***/
struct BBX	{
	int	xMin, yMin;
	int	xMax, yMax;
	};

/* EOF */
