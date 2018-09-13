

#define tag_cmap  0x70616d63        // 'cmap'
#define tag_gasp  0x70736167        // 'gasp'
#define tag_head  0x64616568        // 'head'
#define tag_hdmx  0x786d6468        // 'hdmx'
#define tag_hhea  0x61656868        // 'hhea'
#define tag_hmtx  0x78746d68        // 'hmtx'
#define tag_kern  0x6e72656b        // 'kern'
#define tag_LTSH  0x4853544c        // 'LTSH'
#define tag_maxp  0x7078616d        // 'maxp'
#define tag_name  0x656d616e        // 'name'
#define tag_OS2   0x322f534f        // 'OS/2'
#define tag_post  0x74736f70        // 'post'
#define tag_VDMX  0x584d4456        // 'VDMX'



typedef signed char    int8;
typedef unsigned char  uint8;
typedef short          int16;
typedef unsigned short uint16;
typedef long           int32;
typedef unsigned long  uint32;

typedef short          FUnit;
typedef unsigned short uFUnit;

typedef long Fixed;
typedef long Fract;



#define FS_2BYTE(p)  ( ((unsigned short)((p)[0]) << 8) |  (p)[1])
#define FS_4BYTE(p)  ( FS_2BYTE((p)+2) | ( (FS_2BYTE(p)+0L) << 16) )

#define SWAPW(a) ((int16) FS_2BYTE( (unsigned char *)(&a) ))
#define SWAPL(a) ((int32) FS_4BYTE( (unsigned char *)(&a) ))



typedef struct {
	uint32 bc;
	uint32 ad;
} BigDate;


typedef uint32 sfnt_TableTag;

typedef struct {
	sfnt_TableTag   tag;
	uint32          checkSum;
	uint32          offset;
	uint32          length;
} sfnt_DirectoryEntry, *sfnt_DirectoryEntryPtr;

typedef struct {
	int32 version;                  // 0x10000 (1.0)
	uint16 numOffsets;              // number of tables
	uint16 searchRange;             // (max2 <= numOffsets)*16
	uint16 entrySelector;           // log2 (max2 <= numOffsets)
	uint16 rangeShift;              // numOffsets*16-searchRange
	sfnt_DirectoryEntry table[1];   // table[numOffsets]
} sfnt_OffsetTable, *sfnt_OffsetTablePtr;


#define SFNT_MAGIC 0x5F0F3CF5

typedef struct {
	Fixed       version;            // for this table, set to 1.0
	Fixed       fontRevision;       // For Font Manufacturer
	uint32      checkSumAdjustment;
	uint32      magicNumber;        // signature, should always be 0x5F0F3CF5  == MAGIC
	uint16      flags;
	uint16      unitsPerEm;         // Specifies how many in Font Units we have per EM

	BigDate     created;
	BigDate     modified;

	FUnit       xMin;
	FUnit       yMin;
	FUnit       xMax;
	FUnit       yMax;

	uint16      macStyle;           // macintosh style word
	uint16      lowestRecPPEM;      // lowest recommended pixels per Em

	int16       fontDirectionHint;

	int16       indexToLocFormat;
	int16       glyphDataFormat;
} sfnt_FontHeader, *sfnt_FontHeaderPtr;

