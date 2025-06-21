/*
 *    Methods for dealing with opentype font tables
 *
 * Copyright 2014 Aric Stewart for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define COBJMACROS

#include <stdint.h>
#include "dwrite_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dwrite);

#define MS_HEAD_TAG DWRITE_MAKE_OPENTYPE_TAG('h','e','a','d')
#define MS_HHEA_TAG DWRITE_MAKE_OPENTYPE_TAG('h','h','e','a')
#define MS_OTTO_TAG DWRITE_MAKE_OPENTYPE_TAG('O','T','T','O')
#define MS_OS2_TAG  DWRITE_MAKE_OPENTYPE_TAG('O','S','/','2')
#define MS_POST_TAG DWRITE_MAKE_OPENTYPE_TAG('p','o','s','t')
#define MS_TTCF_TAG DWRITE_MAKE_OPENTYPE_TAG('t','t','c','f')
#define MS_GDEF_TAG DWRITE_MAKE_OPENTYPE_TAG('G','D','E','F')
#define MS_NAME_TAG DWRITE_MAKE_OPENTYPE_TAG('n','a','m','e')
#define MS_GLYF_TAG DWRITE_MAKE_OPENTYPE_TAG('g','l','y','f')
#define MS_CFF__TAG DWRITE_MAKE_OPENTYPE_TAG('C','F','F',' ')
#define MS_CFF2_TAG DWRITE_MAKE_OPENTYPE_TAG('C','F','F','2')
#define MS_CPAL_TAG DWRITE_MAKE_OPENTYPE_TAG('C','P','A','L')
#define MS_COLR_TAG DWRITE_MAKE_OPENTYPE_TAG('C','O','L','R')
#define MS_SVG__TAG DWRITE_MAKE_OPENTYPE_TAG('S','V','G',' ')
#define MS_SBIX_TAG DWRITE_MAKE_OPENTYPE_TAG('s','b','i','x')
#define MS_MAXP_TAG DWRITE_MAKE_OPENTYPE_TAG('m','a','x','p')
#define MS_CBLC_TAG DWRITE_MAKE_OPENTYPE_TAG('C','B','L','C')
#define MS_CMAP_TAG DWRITE_MAKE_OPENTYPE_TAG('c','m','a','p')
#define MS_META_TAG DWRITE_MAKE_OPENTYPE_TAG('m','e','t','a')
#define MS_KERN_TAG DWRITE_MAKE_OPENTYPE_TAG('k','e','r','n')
#define MS_FVAR_TAG DWRITE_MAKE_OPENTYPE_TAG('f','v','a','r')

/* 'sbix' formats */
#define MS_PNG__TAG DWRITE_MAKE_OPENTYPE_TAG('p','n','g',' ')
#define MS_JPG__TAG DWRITE_MAKE_OPENTYPE_TAG('j','p','g',' ')
#define MS_TIFF_TAG DWRITE_MAKE_OPENTYPE_TAG('t','i','f','f')

#define MS_WOFF_TAG DWRITE_MAKE_OPENTYPE_TAG('w','O','F','F')
#define MS_WOF2_TAG DWRITE_MAKE_OPENTYPE_TAG('w','O','F','2')

/* 'meta' tags */
#define MS_DLNG_TAG DWRITE_MAKE_OPENTYPE_TAG('d','l','n','g')
#define MS_SLNG_TAG DWRITE_MAKE_OPENTYPE_TAG('s','l','n','g')

#ifdef WORDS_BIGENDIAN
#define GET_BE_WORD(x) (x)
#define GET_BE_DWORD(x) (x)
#define GET_BE_FIXED(x) (x / 65536.0f)
#else
#define GET_BE_WORD(x)  RtlUshortByteSwap(x)
#define GET_BE_DWORD(x) RtlUlongByteSwap(x)
#define GET_BE_FIXED(x) ((int32_t)GET_BE_DWORD(x) / 65536.0f)
#endif

#define GLYPH_CONTEXT_MAX_LENGTH 64
#define SHAPE_MAX_NESTING_LEVEL 6

struct ttc_header
{
    uint32_t tag;
    uint16_t major_version;
    uint16_t minor_version;
    uint32_t num_fonts;
    uint32_t offsets[1];
};

struct ot_table_dir
{
    uint32_t version;
    uint16_t numTables;
    uint16_t searchRange;
    uint16_t entrySelector;
    uint16_t rangeShift;
};

struct ot_table_record
{
    uint32_t tag;
    uint32_t checksum;
    uint32_t offset;
    uint32_t length;
};

struct cmap_encoding_record
{
    uint16_t platformID;
    uint16_t encodingID;
    uint32_t offset;
};

struct cmap_header
{
    uint16_t version;
    uint16_t num_tables;
    struct cmap_encoding_record tables[1];
};

enum OPENTYPE_CMAP_TABLE_FORMAT
{
    OPENTYPE_CMAP_TABLE_SEGMENT_MAPPING = 4,
    OPENTYPE_CMAP_TABLE_SEGMENTED_COVERAGE = 12
};

enum opentype_cmap_table_platform
{
    OPENTYPE_CMAP_TABLE_PLATFORM_WIN = 3,
};

enum opentype_cmap_table_encoding
{
    OPENTYPE_CMAP_TABLE_ENCODING_SYMBOL = 0,
    OPENTYPE_CMAP_TABLE_ENCODING_UNICODE_BMP = 1,
    OPENTYPE_CMAP_TABLE_ENCODING_UNICODE_FULL = 10,
};

/* PANOSE is 10 bytes in size, need to pack the structure properly */
#include "pshpack2.h"
struct tt_head
{
    USHORT majorVersion;
    USHORT minorVersion;
    ULONG revision;
    ULONG checksumadj;
    ULONG magic;
    USHORT flags;
    USHORT unitsPerEm;
    ULONGLONG created;
    ULONGLONG modified;
    SHORT xMin;
    SHORT yMin;
    SHORT xMax;
    SHORT yMax;
    USHORT macStyle;
    USHORT lowestRecPPEM;
    SHORT direction_hint;
    SHORT index_format;
    SHORT glyphdata_format;
};

enum tt_head_macstyle
{
    TT_HEAD_MACSTYLE_BOLD      = 1 << 0,
    TT_HEAD_MACSTYLE_ITALIC    = 1 << 1,
    TT_HEAD_MACSTYLE_UNDERLINE = 1 << 2,
    TT_HEAD_MACSTYLE_OUTLINE   = 1 << 3,
    TT_HEAD_MACSTYLE_SHADOW    = 1 << 4,
    TT_HEAD_MACSTYLE_CONDENSED = 1 << 5,
    TT_HEAD_MACSTYLE_EXTENDED  = 1 << 6,
};

struct tt_post
{
    uint32_t Version;
    int32_t italicAngle;
    int16_t underlinePosition;
    int16_t underlineThickness;
    uint32_t fixed_pitch;
    uint32_t minmemType42;
    uint32_t maxmemType42;
    uint32_t minmemType1;
    uint32_t maxmemType1;
};

struct tt_os2
{
    USHORT version;
    SHORT xAvgCharWidth;
    USHORT usWeightClass;
    USHORT usWidthClass;
    SHORT fsType;
    SHORT ySubscriptXSize;
    SHORT ySubscriptYSize;
    SHORT ySubscriptXOffset;
    SHORT ySubscriptYOffset;
    SHORT ySuperscriptXSize;
    SHORT ySuperscriptYSize;
    SHORT ySuperscriptXOffset;
    SHORT ySuperscriptYOffset;
    SHORT yStrikeoutSize;
    SHORT yStrikeoutPosition;
    SHORT sFamilyClass;
    PANOSE panose;
    ULONG ulUnicodeRange1;
    ULONG ulUnicodeRange2;
    ULONG ulUnicodeRange3;
    ULONG ulUnicodeRange4;
    CHAR achVendID[4];
    USHORT fsSelection;
    USHORT usFirstCharIndex;
    USHORT usLastCharIndex;
    /* According to the Apple spec, original version didn't have the below fields,
     * version numbers were taken from the OpenType spec.
     */
    /* version 0 (TrueType 1.5) */
    USHORT sTypoAscender;
    USHORT sTypoDescender;
    USHORT sTypoLineGap;
    USHORT usWinAscent;
    USHORT usWinDescent;
    /* version 1 (TrueType 1.66) */
    ULONG ulCodePageRange1;
    ULONG ulCodePageRange2;
    /* version 2 (OpenType 1.2) */
    SHORT sxHeight;
    SHORT sCapHeight;
    USHORT usDefaultChar;
    USHORT usBreakChar;
    USHORT usMaxContext;
};

struct tt_hhea
{
    uint16_t majorVersion;
    uint16_t minorVersion;
    int16_t ascender;
    int16_t descender;
    int16_t linegap;
    uint16_t advanceWidthMax;
    int16_t minLeftSideBearing;
    int16_t minRightSideBearing;
    int16_t xMaxExtent;
    int16_t caretSlopeRise;
    int16_t caretSlopeRun;
    int16_t caretOffset;
    int16_t reserved[4];
    int16_t metricDataFormat;
    uint16_t numberOfHMetrics;
};

struct sbix_header
{
    uint16_t version;
    uint16_t flags;
    uint32_t num_strikes;
    uint32_t strike_offset[1];
};

struct sbix_strike
{
    uint16_t ppem;
    uint16_t ppi;
    uint32_t glyphdata_offsets[1];
};

struct sbix_glyph_data
{
    int16_t originOffsetX;
    int16_t originOffsetY;
    uint32_t graphic_type;
    uint8_t data[1];
};

struct maxp
{
    DWORD version;
    WORD num_glyphs;
};

struct cblc_header
{
    uint16_t major_version;
    uint16_t minor_version;
    uint32_t num_sizes;
};

struct sbit_line_metrics
{
    int8_t ascender;
    int8_t descender;
    uint8_t widthMax;
    int8_t caretSlopeNumerator;
    int8_t caretSlopeDenominator;
    int8_t caretOffset;
    int8_t minOriginSB;
    int8_t minAdvanceSB;
    int8_t maxBeforeBL;
    int8_t minAfterBL;
    int8_t pad1;
    int8_t pad2;
};

struct cblc_bitmapsize_table
{
    uint32_t indexSubTableArrayOffset;
    uint32_t indexTablesSize;
    uint32_t numberofIndexSubTables;
    uint32_t colorRef;
    struct sbit_line_metrics hori;
    struct sbit_line_metrics vert;
    uint16_t startGlyphIndex;
    uint16_t endGlyphIndex;
    uint8_t ppemX;
    uint8_t ppemY;
    uint8_t bit_depth;
    int8_t flags;
};

struct gasp_range
{
    WORD max_ppem;
    WORD flags;
};

struct gasp_header
{
    WORD version;
    WORD num_ranges;
    struct gasp_range ranges[1];
};

enum OS2_FSSELECTION {
    OS2_FSSELECTION_ITALIC           = 1 << 0,
    OS2_FSSELECTION_UNDERSCORE       = 1 << 1,
    OS2_FSSELECTION_NEGATIVE         = 1 << 2,
    OS2_FSSELECTION_OUTLINED         = 1 << 3,
    OS2_FSSELECTION_STRIKEOUT        = 1 << 4,
    OS2_FSSELECTION_BOLD             = 1 << 5,
    OS2_FSSELECTION_REGULAR          = 1 << 6,
    OS2_FSSELECTION_USE_TYPO_METRICS = 1 << 7,
    OS2_FSSELECTION_WWS              = 1 << 8,
    OS2_FSSELECTION_OBLIQUE          = 1 << 9
};

struct name_record
{
    uint16_t platformID;
    uint16_t encodingID;
    uint16_t languageID;
    uint16_t nameID;
    uint16_t length;
    uint16_t offset;
};

struct name_header
{
    uint16_t format;
    uint16_t count;
    uint16_t stringOffset;
    struct name_record records[1];
};

struct vdmx_header
{
    uint16_t version;
    uint16_t num_recs;
    uint16_t num_ratios;
};

struct vdmx_ratio
{
    uint8_t bCharSet;
    uint8_t xRatio;
    uint8_t yStartRatio;
    uint8_t yEndRatio;
};

struct vdmx_vtable
{
    uint16_t yPelHeight;
    int16_t yMax;
    int16_t yMin;
};

struct vdmx_group
{
    uint16_t recs;
    uint8_t startsz;
    uint8_t endsz;
    struct vdmx_vtable entries[1];
};

struct ot_feature_record
{
    uint32_t tag;
    uint16_t offset;
};

struct ot_feature_list
{
    uint16_t feature_count;
    struct ot_feature_record features[1];
};

struct ot_langsys
{
    uint16_t lookup_order; /* Reserved */
    uint16_t required_feature_index;
    uint16_t feature_count;
    uint16_t feature_index[1];
};

struct ot_langsys_record
{
    uint32_t tag;
    uint16_t langsys;
};

struct ot_script
{
    uint16_t default_langsys;
    uint16_t langsys_count;
    struct ot_langsys_record langsys[1];
};

struct ot_script_record
{
    uint32_t tag;
    uint16_t script;
};

struct ot_script_list
{
    uint16_t script_count;
    struct ot_script_record scripts[1];
};

enum ot_gdef_class
{
    GDEF_CLASS_UNCLASSIFIED = 0,
    GDEF_CLASS_BASE = 1,
    GDEF_CLASS_LIGATURE = 2,
    GDEF_CLASS_MARK = 3,
    GDEF_CLASS_COMPONENT = 4,
    GDEF_CLASS_MAX = GDEF_CLASS_COMPONENT,
};

struct gdef_header
{
    uint16_t major_version;
    uint16_t minor_version;
    uint16_t classdef;
    uint16_t attach_list;
    uint16_t ligcaret_list;
    uint16_t markattach_classdef;
    uint16_t markglyphsetdef;
};

struct ot_gdef_classdef_format1
{
    uint16_t format;
    uint16_t start_glyph;
    uint16_t glyph_count;
    uint16_t classes[1];
};

struct ot_gdef_class_range
{
    uint16_t start_glyph;
    uint16_t end_glyph;
    uint16_t glyph_class;
};

struct ot_gdef_classdef_format2
{
    uint16_t format;
    uint16_t range_count;
    struct ot_gdef_class_range ranges[1];
};

struct ot_gdef_markglyphsets
{
    uint16_t format;
    uint16_t count;
    uint32_t offsets[1];
};

struct gpos_gsub_header
{
    uint16_t major_version;
    uint16_t minor_version;
    uint16_t script_list;
    uint16_t feature_list;
    uint16_t lookup_list;
};

enum gsub_gpos_lookup_flags
{
    LOOKUP_FLAG_RTL = 0x1, /* Only used for GPOS cursive attachments. */

    LOOKUP_FLAG_IGNORE_BASE = 0x2,
    LOOKUP_FLAG_IGNORE_LIGATURES = 0x4,
    LOOKUP_FLAG_IGNORE_MARKS = 0x8,
    LOOKUP_FLAG_IGNORE_MASK = 0xe, /* Combined LOOKUP_FLAG_IGNORE_* flags. */

    LOOKUP_FLAG_USE_MARK_FILTERING_SET = 0x10,
    LOOKUP_FLAG_MARK_ATTACHMENT_TYPE = 0xff00,
};

enum attach_type
{
    GLYPH_ATTACH_NONE = 0,
    GLYPH_ATTACH_MARK,
    GLYPH_ATTACH_CURSIVE,
};

enum glyph_prop_flags
{
    GLYPH_PROP_BASE = LOOKUP_FLAG_IGNORE_BASE,
    GLYPH_PROP_LIGATURE = LOOKUP_FLAG_IGNORE_LIGATURES,
    GLYPH_PROP_MARK = LOOKUP_FLAG_IGNORE_MARKS,

    GLYPH_PROP_ZWNJ = 0x10,
    GLYPH_PROP_ZWJ = 0x20,
    GLYPH_PROP_IGNORABLE = 0x40,
    GLYPH_PROP_HIDDEN = 0x80,

    GLYPH_PROP_MARK_ATTACH_CLASS_MASK = 0xff00, /* Used with LOOKUP_FLAG_MARK_ATTACHMENT_TYPE. */
    GLYPH_PROP_ATTACH_TYPE_MASK = 0xff0000,
};

enum gpos_lookup_type
{
    GPOS_LOOKUP_SINGLE_ADJUSTMENT = 1,
    GPOS_LOOKUP_PAIR_ADJUSTMENT = 2,
    GPOS_LOOKUP_CURSIVE_ATTACHMENT = 3,
    GPOS_LOOKUP_MARK_TO_BASE_ATTACHMENT = 4,
    GPOS_LOOKUP_MARK_TO_LIGATURE_ATTACHMENT = 5,
    GPOS_LOOKUP_MARK_TO_MARK_ATTACHMENT = 6,
    GPOS_LOOKUP_CONTEXTUAL_POSITION = 7,
    GPOS_LOOKUP_CONTEXTUAL_CHAINING_POSITION = 8,
    GPOS_LOOKUP_EXTENSION_POSITION = 9,
};

enum gsub_lookup_type
{
    GSUB_LOOKUP_SINGLE_SUBST = 1,
    GSUB_LOOKUP_MULTIPLE_SUBST = 2,
    GSUB_LOOKUP_ALTERNATE_SUBST = 3,
    GSUB_LOOKUP_LIGATURE_SUBST = 4,
    GSUB_LOOKUP_CONTEXTUAL_SUBST = 5,
    GSUB_LOOKUP_CHAINING_CONTEXTUAL_SUBST = 6,
    GSUB_LOOKUP_EXTENSION_SUBST = 7,
    GSUB_LOOKUP_REVERSE_CHAINING_CONTEXTUAL_SUBST = 8,
};

enum gpos_value_format
{
    GPOS_VALUE_X_PLACEMENT = 0x1,
    GPOS_VALUE_Y_PLACEMENT = 0x2,
    GPOS_VALUE_X_ADVANCE = 0x4,
    GPOS_VALUE_Y_ADVANCE = 0x8,
    GPOS_VALUE_X_PLACEMENT_DEVICE = 0x10,
    GPOS_VALUE_Y_PLACEMENT_DEVICE = 0x20,
    GPOS_VALUE_X_ADVANCE_DEVICE = 0x40,
    GPOS_VALUE_Y_ADVANCE_DEVICE = 0x80,
};

enum OPENTYPE_PLATFORM_ID
{
    OPENTYPE_PLATFORM_UNICODE = 0,
    OPENTYPE_PLATFORM_MAC,
    OPENTYPE_PLATFORM_ISO,
    OPENTYPE_PLATFORM_WIN,
    OPENTYPE_PLATFORM_CUSTOM
};

struct ot_gsubgpos_extension_format1
{
    uint16_t format;
    uint16_t lookup_type;
    uint32_t extension_offset;
};

struct ot_gsub_singlesubst_format1
{
    uint16_t format;
    uint16_t coverage;
    int16_t delta;
};

struct ot_gsub_singlesubst_format2
{
    uint16_t format;
    uint16_t coverage;
    uint16_t count;
    uint16_t substitutes[1];
};

struct ot_gsub_multsubst_format1
{
    uint16_t format;
    uint16_t coverage;
    uint16_t seq_count;
    uint16_t seq[1];
};

struct ot_gsub_altsubst_format1
{
    uint16_t format;
    uint16_t coverage;
    uint16_t count;
    uint16_t sets[1];
};

struct ot_gsub_ligsubst_format1
{
    uint16_t format;
    uint16_t coverage;
    uint16_t lig_set_count;
    uint16_t lig_sets[1];
};

struct ot_gsub_ligset
{
    uint16_t count;
    uint16_t offsets[1];
};

struct ot_gsub_lig
{
    uint16_t lig_glyph;
    uint16_t comp_count;
    uint16_t components[1];
};

struct ot_gsubgpos_context_format1
{
    uint16_t format;
    uint16_t coverage;
    uint16_t ruleset_count;
    uint16_t rulesets[1];
};

struct ot_gsubgpos_ruleset
{
    uint16_t count;
    uint16_t offsets[1];
};

struct ot_feature
{
    uint16_t feature_params;
    uint16_t lookup_count;
    uint16_t lookuplist_index[1];
};

struct ot_lookup_list
{
    uint16_t lookup_count;
    uint16_t lookup[1];
};

struct ot_lookup_table
{
    uint16_t lookup_type;
    uint16_t flags;
    uint16_t subtable_count;
    uint16_t subtable[1];
};

#define GLYPH_NOT_COVERED (~0u)

struct ot_coverage_format1
{
    uint16_t format;
    uint16_t glyph_count;
    uint16_t glyphs[1];
};

struct ot_coverage_range
{
    uint16_t start_glyph;
    uint16_t end_glyph;
    uint16_t startcoverage_index;
};

struct ot_coverage_format2
{
    uint16_t format;
    uint16_t range_count;
    struct ot_coverage_range ranges[1];
};

struct ot_gpos_device_table
{
    uint16_t start_size;
    uint16_t end_size;
    uint16_t format;
    uint16_t values[1];
};

struct ot_gpos_singlepos_format1
{
    uint16_t format;
    uint16_t coverage;
    uint16_t value_format;
    uint16_t value[1];
};

struct ot_gpos_singlepos_format2
{
    uint16_t format;
    uint16_t coverage;
    uint16_t value_format;
    uint16_t value_count;
    uint16_t values[1];
};

struct ot_gpos_pairvalue
{
    uint16_t second_glyph;
    uint8_t data[1];
};

struct ot_gpos_pairset
{
    uint16_t pairvalue_count;
    struct ot_gpos_pairvalue pairvalues[1];
};

struct ot_gpos_pairpos_format1
{
    uint16_t format;
    uint16_t coverage;
    uint16_t value_format1;
    uint16_t value_format2;
    uint16_t pairset_count;
    uint16_t pairsets[1];
};

struct ot_gpos_pairpos_format2
{
    uint16_t format;
    uint16_t coverage;
    uint16_t value_format1;
    uint16_t value_format2;
    uint16_t class_def1;
    uint16_t class_def2;
    uint16_t class1_count;
    uint16_t class2_count;
    uint16_t values[1];
};

struct ot_gpos_anchor_format1
{
    uint16_t format;
    int16_t x_coord;
    int16_t y_coord;
};

struct ot_gpos_anchor_format2
{
    uint16_t format;
    int16_t x_coord;
    int16_t y_coord;
    uint16_t anchor_point;
};

struct ot_gpos_anchor_format3
{
    uint16_t format;
    int16_t x_coord;
    int16_t y_coord;
    uint16_t x_dev_offset;
    uint16_t y_dev_offset;
};

struct ot_gpos_cursive_format1
{
    uint16_t format;
    uint16_t coverage;
    uint16_t count;
    uint16_t anchors[1];
};

struct ot_gpos_mark_record
{
    uint16_t mark_class;
    uint16_t mark_anchor;
};

struct ot_gpos_mark_array
{
    uint16_t count;
    struct ot_gpos_mark_record records[1];
};

struct ot_gpos_base_array
{
    uint16_t count;
    uint16_t offsets[1];
};

struct ot_gpos_mark_to_base_format1
{
    uint16_t format;
    uint16_t mark_coverage;
    uint16_t base_coverage;
    uint16_t mark_class_count;
    uint16_t mark_array;
    uint16_t base_array;
};

struct ot_gpos_mark_to_lig_format1
{
    uint16_t format;
    uint16_t mark_coverage;
    uint16_t lig_coverage;
    uint16_t mark_class_count;
    uint16_t mark_array;
    uint16_t lig_array;
};

struct ot_gpos_mark_to_mark_format1
{
    uint16_t format;
    uint16_t mark1_coverage;
    uint16_t mark2_coverage;
    uint16_t mark_class_count;
    uint16_t mark1_array;
    uint16_t mark2_array;
};

struct kern_header
{
    uint16_t version;
    uint16_t table_count;
};

struct kern_subtable_header
{
    uint16_t version;
    uint16_t length;
    uint16_t coverage;
};

#include "poppack.h"

enum TT_NAME_WINDOWS_ENCODING_ID
{
    TT_NAME_WINDOWS_ENCODING_SYMBOL = 0,
    TT_NAME_WINDOWS_ENCODING_UNICODE_BMP,
    TT_NAME_WINDOWS_ENCODING_SJIS,
    TT_NAME_WINDOWS_ENCODING_PRC,
    TT_NAME_WINDOWS_ENCODING_BIG5,
    TT_NAME_WINDOWS_ENCODING_WANSUNG,
    TT_NAME_WINDOWS_ENCODING_JOHAB,
    TT_NAME_WINDOWS_ENCODING_RESERVED1,
    TT_NAME_WINDOWS_ENCODING_RESERVED2,
    TT_NAME_WINDOWS_ENCODING_RESERVED3,
    TT_NAME_WINDOWS_ENCODING_UNICODE_FULL
};

enum TT_NAME_MAC_ENCODING_ID
{
    TT_NAME_MAC_ENCODING_ROMAN = 0,
    TT_NAME_MAC_ENCODING_JAPANESE,
    TT_NAME_MAC_ENCODING_TRAD_CHINESE,
    TT_NAME_MAC_ENCODING_KOREAN,
    TT_NAME_MAC_ENCODING_ARABIC,
    TT_NAME_MAC_ENCODING_HEBREW,
    TT_NAME_MAC_ENCODING_GREEK,
    TT_NAME_MAC_ENCODING_RUSSIAN,
    TT_NAME_MAC_ENCODING_RSYMBOL,
    TT_NAME_MAC_ENCODING_DEVANAGARI,
    TT_NAME_MAC_ENCODING_GURMUKHI,
    TT_NAME_MAC_ENCODING_GUJARATI,
    TT_NAME_MAC_ENCODING_ORIYA,
    TT_NAME_MAC_ENCODING_BENGALI,
    TT_NAME_MAC_ENCODING_TAMIL,
    TT_NAME_MAC_ENCODING_TELUGU,
    TT_NAME_MAC_ENCODING_KANNADA,
    TT_NAME_MAC_ENCODING_MALAYALAM,
    TT_NAME_MAC_ENCODING_SINHALESE,
    TT_NAME_MAC_ENCODING_BURMESE,
    TT_NAME_MAC_ENCODING_KHMER,
    TT_NAME_MAC_ENCODING_THAI,
    TT_NAME_MAC_ENCODING_LAOTIAN,
    TT_NAME_MAC_ENCODING_GEORGIAN,
    TT_NAME_MAC_ENCODING_ARMENIAN,
    TT_NAME_MAC_ENCODING_SIMPL_CHINESE,
    TT_NAME_MAC_ENCODING_TIBETAN,
    TT_NAME_MAC_ENCODING_MONGOLIAN,
    TT_NAME_MAC_ENCODING_GEEZ,
    TT_NAME_MAC_ENCODING_SLAVIC,
    TT_NAME_MAC_ENCODING_VIETNAMESE,
    TT_NAME_MAC_ENCODING_SINDHI,
    TT_NAME_MAC_ENCODING_UNINTERPRETED
};

enum TT_NAME_MAC_LANGUAGE_ID
{
    TT_NAME_MAC_LANGID_ENGLISH = 0,
    TT_NAME_MAC_LANGID_FRENCH,
    TT_NAME_MAC_LANGID_GERMAN,
    TT_NAME_MAC_LANGID_ITALIAN,
    TT_NAME_MAC_LANGID_DUTCH,
    TT_NAME_MAC_LANGID_SWEDISH,
    TT_NAME_MAC_LANGID_SPANISH,
    TT_NAME_MAC_LANGID_DANISH,
    TT_NAME_MAC_LANGID_PORTUGUESE,
    TT_NAME_MAC_LANGID_NORWEGIAN,
    TT_NAME_MAC_LANGID_HEBREW,
    TT_NAME_MAC_LANGID_JAPANESE,
    TT_NAME_MAC_LANGID_ARABIC,
    TT_NAME_MAC_LANGID_FINNISH,
    TT_NAME_MAC_LANGID_GREEK,
    TT_NAME_MAC_LANGID_ICELANDIC,
    TT_NAME_MAC_LANGID_MALTESE,
    TT_NAME_MAC_LANGID_TURKISH,
    TT_NAME_MAC_LANGID_CROATIAN,
    TT_NAME_MAC_LANGID_TRAD_CHINESE,
    TT_NAME_MAC_LANGID_URDU,
    TT_NAME_MAC_LANGID_HINDI,
    TT_NAME_MAC_LANGID_THAI,
    TT_NAME_MAC_LANGID_KOREAN,
    TT_NAME_MAC_LANGID_LITHUANIAN,
    TT_NAME_MAC_LANGID_POLISH,
    TT_NAME_MAC_LANGID_HUNGARIAN,
    TT_NAME_MAC_LANGID_ESTONIAN,
    TT_NAME_MAC_LANGID_LATVIAN,
    TT_NAME_MAC_LANGID_SAMI,
    TT_NAME_MAC_LANGID_FAROESE,
    TT_NAME_MAC_LANGID_FARSI,
    TT_NAME_MAC_LANGID_RUSSIAN,
    TT_NAME_MAC_LANGID_SIMPL_CHINESE,
    TT_NAME_MAC_LANGID_FLEMISH,
    TT_NAME_MAC_LANGID_GAELIC,
    TT_NAME_MAC_LANGID_ALBANIAN,
    TT_NAME_MAC_LANGID_ROMANIAN,
    TT_NAME_MAC_LANGID_CZECH,
    TT_NAME_MAC_LANGID_SLOVAK,
    TT_NAME_MAC_LANGID_SLOVENIAN,
    TT_NAME_MAC_LANGID_YIDDISH,
    TT_NAME_MAC_LANGID_SERBIAN,
    TT_NAME_MAC_LANGID_MACEDONIAN,
    TT_NAME_MAC_LANGID_BULGARIAN,
    TT_NAME_MAC_LANGID_UKRAINIAN,
    TT_NAME_MAC_LANGID_BYELORUSSIAN,
    TT_NAME_MAC_LANGID_UZBEK,
    TT_NAME_MAC_LANGID_KAZAKH,
    TT_NAME_MAC_LANGID_AZERB_CYR,
    TT_NAME_MAC_LANGID_AZERB_ARABIC,
    TT_NAME_MAC_LANGID_ARMENIAN,
    TT_NAME_MAC_LANGID_GEORGIAN,
    TT_NAME_MAC_LANGID_MOLDAVIAN,
    TT_NAME_MAC_LANGID_KIRGHIZ,
    TT_NAME_MAC_LANGID_TAJIKI,
    TT_NAME_MAC_LANGID_TURKMEN,
    TT_NAME_MAC_LANGID_MONGOLIAN,
    TT_NAME_MAC_LANGID_MONGOLIAN_CYR,
    TT_NAME_MAC_LANGID_PASHTO,
    TT_NAME_MAC_LANGID_KURDISH,
    TT_NAME_MAC_LANGID_KASHMIRI,
    TT_NAME_MAC_LANGID_SINDHI,
    TT_NAME_MAC_LANGID_TIBETAN,
    TT_NAME_MAC_LANGID_NEPALI,
    TT_NAME_MAC_LANGID_SANSKRIT,
    TT_NAME_MAC_LANGID_MARATHI,
    TT_NAME_MAC_LANGID_BENGALI,
    TT_NAME_MAC_LANGID_ASSAMESE,
    TT_NAME_MAC_LANGID_GUJARATI,
    TT_NAME_MAC_LANGID_PUNJABI,
    TT_NAME_MAC_LANGID_ORIYA,
    TT_NAME_MAC_LANGID_MALAYALAM,
    TT_NAME_MAC_LANGID_KANNADA,
    TT_NAME_MAC_LANGID_TAMIL,
    TT_NAME_MAC_LANGID_TELUGU,
    TT_NAME_MAC_LANGID_SINHALESE,
    TT_NAME_MAC_LANGID_BURMESE,
    TT_NAME_MAC_LANGID_KHMER,
    TT_NAME_MAC_LANGID_LAO,
    TT_NAME_MAC_LANGID_VIETNAMESE,
    TT_NAME_MAC_LANGID_INDONESIAN,
    TT_NAME_MAC_LANGID_TAGALOG,
    TT_NAME_MAC_LANGID_MALAY_ROMAN,
    TT_NAME_MAC_LANGID_MALAY_ARABIC,
    TT_NAME_MAC_LANGID_AMHARIC,
    TT_NAME_MAC_LANGID_TIGRINYA,
    TT_NAME_MAC_LANGID_GALLA,
    TT_NAME_MAC_LANGID_SOMALI,
    TT_NAME_MAC_LANGID_SWAHILI,
    TT_NAME_MAC_LANGID_KINYARWANDA,
    TT_NAME_MAC_LANGID_RUNDI,
    TT_NAME_MAC_LANGID_NYANJA,
    TT_NAME_MAC_LANGID_MALAGASY,
    TT_NAME_MAC_LANGID_ESPERANTO,
    TT_NAME_MAC_LANGID_WELSH = 128,
    TT_NAME_MAC_LANGID_BASQUE,
    TT_NAME_MAC_LANGID_CATALAN,
    TT_NAME_MAC_LANGID_LATIN,
    TT_NAME_MAC_LANGID_QUECHUA,
    TT_NAME_MAC_LANGID_GUARANI,
    TT_NAME_MAC_LANGID_AYMARA,
    TT_NAME_MAC_LANGID_TATAR,
    TT_NAME_MAC_LANGID_UIGHUR,
    TT_NAME_MAC_LANGID_DZONGKHA,
    TT_NAME_MAC_LANGID_JAVANESE,
    TT_NAME_MAC_LANGID_SUNDANESE,
    TT_NAME_MAC_LANGID_GALICIAN,
    TT_NAME_MAC_LANGID_AFRIKAANS,
    TT_NAME_MAC_LANGID_BRETON,
    TT_NAME_MAC_LANGID_INUKTITUT,
    TT_NAME_MAC_LANGID_SCOTTISH_GAELIC,
    TT_NAME_MAC_LANGID_MANX_GAELIC,
    TT_NAME_MAC_LANGID_IRISH_GAELIC,
    TT_NAME_MAC_LANGID_TONGAN,
    TT_NAME_MAC_LANGID_GREEK_POLYTONIC,
    TT_NAME_MAC_LANGID_GREENLANDIC,
    TT_NAME_MAC_LANGID_AZER_ROMAN
};

/* Names are indexed with TT_NAME_MAC_LANGUAGE_ID values */
static const char name_mac_langid_to_locale[][10] = {
    "en-US",
    "fr-FR",
    "de-DE",
    "it-IT",
    "nl-NL",
    "sv-SE",
    "es-ES",
    "da-DA",
    "pt-PT",
    "no-NO",
    "he-IL",
    "ja-JP",
    "ar-AR",
    "fi-FI",
    "el-GR",
    "is-IS",
    "mt-MT",
    "tr-TR",
    "hr-HR",
    "zh-HK",
    "ur-PK",
    "hi-IN",
    "th-TH",
    "ko-KR",
    "lt-LT",
    "pl-PL",
    "hu-HU",
    "et-EE",
    "lv-LV",
    "se-NO",
    "fo-FO",
    "fa-IR",
    "ru-RU",
    "zh-CN",
    "nl-BE",
    "gd-GB",
    "sq-AL",
    "ro-RO",
    "cs-CZ",
    "sk-SK",
    "sl-SI",
    "",
    "sr-Latn",
    "mk-MK",
    "bg-BG",
    "uk-UA",
    "be-BY",
    "uz-Latn",
    "kk-KZ",
    "az-Cyrl-AZ",
    "az-AZ",
    "hy-AM",
    "ka-GE",
    "",
    "",
    "tg-TJ",
    "tk-TM",
    "mn-Mong",
    "mn-MN",
    "ps-AF",
    "ku-Arab",
    "",
    "sd-Arab",
    "bo-CN",
    "ne-NP",
    "sa-IN",
    "mr-IN",
    "bn-IN",
    "as-IN",
    "gu-IN",
    "pa-Arab",
    "or-IN",
    "ml-IN",
    "kn-IN",
    "ta-LK",
    "te-IN",
    "si-LK",
    "",
    "km-KH",
    "lo-LA",
    "vi-VN",
    "id-ID",
    "",
    "ms-MY",
    "ms-Arab",
    "am-ET",
    "ti-ET",
    "",
    "",
    "sw-KE",
    "rw-RW",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "cy-GB",
    "eu-ES",
    "ca-ES",
    "",
    "",
    "",
    "",
    "tt-RU",
    "ug-CN",
    "",
    "",
    "",
    "gl-ES",
    "af-ZA",
    "br-FR",
    "iu-Latn-CA",
    "gd-GB",
    "",
    "ga-IE",
    "",
    "",
    "kl-GL",
    "az-Latn"
};

enum OPENTYPE_STRING_ID
{
    OPENTYPE_STRING_COPYRIGHT_NOTICE = 0,
    OPENTYPE_STRING_FAMILY_NAME,
    OPENTYPE_STRING_SUBFAMILY_NAME,
    OPENTYPE_STRING_UNIQUE_IDENTIFIER,
    OPENTYPE_STRING_FULL_FONTNAME,
    OPENTYPE_STRING_VERSION_STRING,
    OPENTYPE_STRING_POSTSCRIPT_FONTNAME,
    OPENTYPE_STRING_TRADEMARK,
    OPENTYPE_STRING_MANUFACTURER,
    OPENTYPE_STRING_DESIGNER,
    OPENTYPE_STRING_DESCRIPTION,
    OPENTYPE_STRING_VENDOR_URL,
    OPENTYPE_STRING_DESIGNER_URL,
    OPENTYPE_STRING_LICENSE_DESCRIPTION,
    OPENTYPE_STRING_LICENSE_INFO_URL,
    OPENTYPE_STRING_RESERVED_ID15,
    OPENTYPE_STRING_TYPOGRAPHIC_FAMILY_NAME,
    OPENTYPE_STRING_TYPOGRAPHIC_SUBFAMILY_NAME,
    OPENTYPE_STRING_COMPATIBLE_FULLNAME,
    OPENTYPE_STRING_SAMPLE_TEXT,
    OPENTYPE_STRING_POSTSCRIPT_CID_NAME,
    OPENTYPE_STRING_WWS_FAMILY_NAME,
    OPENTYPE_STRING_WWS_SUBFAMILY_NAME
};

static const UINT16 dwriteid_to_opentypeid[DWRITE_INFORMATIONAL_STRING_WEIGHT_STRETCH_STYLE_FAMILY_NAME + 1] =
{
    (UINT16)-1, /* DWRITE_INFORMATIONAL_STRING_NONE is not used */
    OPENTYPE_STRING_COPYRIGHT_NOTICE,
    OPENTYPE_STRING_VERSION_STRING,
    OPENTYPE_STRING_TRADEMARK,
    OPENTYPE_STRING_MANUFACTURER,
    OPENTYPE_STRING_DESIGNER,
    OPENTYPE_STRING_DESIGNER_URL,
    OPENTYPE_STRING_DESCRIPTION,
    OPENTYPE_STRING_VENDOR_URL,
    OPENTYPE_STRING_LICENSE_DESCRIPTION,
    OPENTYPE_STRING_LICENSE_INFO_URL,
    OPENTYPE_STRING_FAMILY_NAME,
    OPENTYPE_STRING_SUBFAMILY_NAME,
    OPENTYPE_STRING_TYPOGRAPHIC_FAMILY_NAME,
    OPENTYPE_STRING_TYPOGRAPHIC_SUBFAMILY_NAME,
    OPENTYPE_STRING_SAMPLE_TEXT,
    OPENTYPE_STRING_FULL_FONTNAME,
    OPENTYPE_STRING_POSTSCRIPT_FONTNAME,
    OPENTYPE_STRING_POSTSCRIPT_CID_NAME,
    OPENTYPE_STRING_WWS_FAMILY_NAME,
};

/* CPAL table */
struct cpal_header_0
{
    uint16_t version;
    uint16_t num_palette_entries;
    uint16_t num_palettes;
    uint16_t num_color_records;
    uint32_t offset_first_color_record;
    uint16_t color_record_indices[1];
};

struct cpal_color_record
{
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t alpha;
};

/* COLR table */
struct colr_header
{
    uint16_t version;
    uint16_t num_baseglyph_records;
    uint32_t offset_baseglyph_records;
    uint32_t offset_layer_records;
    uint16_t num_layer_records;
};

struct colr_baseglyph_record
{
    uint16_t glyph;
    uint16_t first_layer_index;
    uint16_t num_layers;
};

struct colr_layer_record
{
    uint16_t glyph;
    uint16_t palette_index;
};

struct meta_data_map
{
    uint32_t tag;
    uint32_t offset;
    uint32_t length;
};

struct meta_header
{
    uint32_t version;
    uint32_t flags;
    uint32_t reserved;
    uint32_t data_maps_count;
    struct meta_data_map maps[1];
};

struct fvar_header
{
    uint16_t major_version;
    uint16_t minor_version;
    uint16_t axes_array_offset;
    uint16_t reserved;
    uint16_t axis_count;
    uint16_t axis_size;
    uint16_t instance_count;
    uint16_t instance_size;
};

struct var_axis_record
{
    uint32_t tag;
    int32_t min_value;
    int32_t default_value;
    int32_t max_value;
    uint16_t flags;
    uint16_t nameid;
};

static const void *table_read_ensure(const struct dwrite_fonttable *table, unsigned int offset, unsigned int size)
{
    if (size > table->size || offset > table->size - size)
        return NULL;

    return table->data + offset;
}

static WORD table_read_be_word(const struct dwrite_fonttable *table, unsigned int offset)
{
    const WORD *ptr = table_read_ensure(table, offset, sizeof(*ptr));
    return ptr ? GET_BE_WORD(*ptr) : 0;
}

static DWORD table_read_be_dword(const struct dwrite_fonttable *table, unsigned int offset)
{
    const DWORD *ptr = table_read_ensure(table, offset, sizeof(*ptr));
    return ptr ? GET_BE_DWORD(*ptr) : 0;
}

static float table_read_be_fixed(const struct dwrite_fonttable *table, unsigned int offset)
{
    return (int32_t)table_read_be_dword(table, offset) / 65536.0;
}

static DWORD table_read_dword(const struct dwrite_fonttable *table, unsigned int offset)
{
    const DWORD *ptr = table_read_ensure(table, offset, sizeof(*ptr));
    return ptr ? *ptr : 0;
}

BOOL is_face_type_supported(DWRITE_FONT_FACE_TYPE type)
{
    return (type == DWRITE_FONT_FACE_TYPE_CFF) ||
           (type == DWRITE_FONT_FACE_TYPE_TRUETYPE) ||
           (type == DWRITE_FONT_FACE_TYPE_OPENTYPE_COLLECTION) ||
           (type == DWRITE_FONT_FACE_TYPE_RAW_CFF);
}

typedef HRESULT (*dwrite_fontfile_analyzer)(IDWriteFontFileStream *stream, UINT32 *font_count, DWRITE_FONT_FILE_TYPE *file_type,
    DWRITE_FONT_FACE_TYPE *face_type);

static HRESULT opentype_ttc_analyzer(IDWriteFontFileStream *stream, UINT32 *font_count, DWRITE_FONT_FILE_TYPE *file_type,
    DWRITE_FONT_FACE_TYPE *face_type)
{
    const struct ttc_header *header;
    void *context;
    HRESULT hr;

    hr = IDWriteFontFileStream_ReadFileFragment(stream, (const void**)&header, 0, sizeof(header), &context);
    if (FAILED(hr))
        return hr;

    if (header->tag == MS_TTCF_TAG)
    {
        *font_count = GET_BE_DWORD(header->num_fonts);
        *file_type = DWRITE_FONT_FILE_TYPE_OPENTYPE_COLLECTION;
        *face_type = DWRITE_FONT_FACE_TYPE_OPENTYPE_COLLECTION;
    }

    IDWriteFontFileStream_ReleaseFileFragment(stream, context);

    return *file_type != DWRITE_FONT_FILE_TYPE_UNKNOWN ? S_OK : S_FALSE;
}

static HRESULT opentype_ttf_analyzer(IDWriteFontFileStream *stream, UINT32 *font_count, DWRITE_FONT_FILE_TYPE *file_type,
    DWRITE_FONT_FACE_TYPE *face_type)
{
    const DWORD *header;
    void *context;
    HRESULT hr;

    hr = IDWriteFontFileStream_ReadFileFragment(stream, (const void**)&header, 0, sizeof(*header), &context);
    if (FAILED(hr))
        return hr;

    if (GET_BE_DWORD(*header) == 0x10000) {
        *font_count = 1;
        *file_type = DWRITE_FONT_FILE_TYPE_TRUETYPE;
        *face_type = DWRITE_FONT_FACE_TYPE_TRUETYPE;
    }

    IDWriteFontFileStream_ReleaseFileFragment(stream, context);

    return *file_type != DWRITE_FONT_FILE_TYPE_UNKNOWN ? S_OK : S_FALSE;
}

static HRESULT opentype_otf_analyzer(IDWriteFontFileStream *stream, UINT32 *font_count, DWRITE_FONT_FILE_TYPE *file_type,
    DWRITE_FONT_FACE_TYPE *face_type)
{
    const DWORD *header;
    void *context;
    HRESULT hr;

    hr = IDWriteFontFileStream_ReadFileFragment(stream, (const void**)&header, 0, sizeof(*header), &context);
    if (FAILED(hr))
        return hr;

    if (GET_BE_DWORD(*header) == MS_OTTO_TAG) {
        *font_count = 1;
        *file_type = DWRITE_FONT_FILE_TYPE_CFF;
        *face_type = DWRITE_FONT_FACE_TYPE_CFF;
    }

    IDWriteFontFileStream_ReleaseFileFragment(stream, context);

    return *file_type != DWRITE_FONT_FILE_TYPE_UNKNOWN ? S_OK : S_FALSE;
}

static HRESULT opentype_type1_analyzer(IDWriteFontFileStream *stream, UINT32 *font_count, DWRITE_FONT_FILE_TYPE *file_type,
    DWRITE_FONT_FACE_TYPE *face_type)
{
#include "pshpack1.h"
    /* Specified in Adobe TechNote #5178 */
    struct pfm_header {
        WORD  dfVersion;
        DWORD dfSize;
        char  data0[95];
        DWORD dfDevice;
        char  data1[12];
    };
#include "poppack.h"
    struct type1_header {
        WORD tag;
        char data[14];
    };
    const struct type1_header *header;
    void *context;
    HRESULT hr;

    hr = IDWriteFontFileStream_ReadFileFragment(stream, (const void**)&header, 0, sizeof(*header), &context);
    if (FAILED(hr))
        return hr;

    /* tag is followed by plain text section */
    if (header->tag == 0x8001 &&
        (!memcmp(header->data, "%!PS-AdobeFont", 14) ||
         !memcmp(header->data, "%!FontType", 10))) {
        *font_count = 1;
        *file_type = DWRITE_FONT_FILE_TYPE_TYPE1_PFB;
        *face_type = DWRITE_FONT_FACE_TYPE_TYPE1;
    }

    IDWriteFontFileStream_ReleaseFileFragment(stream, context);

    /* let's see if it's a .pfm metrics file */
    if (*file_type == DWRITE_FONT_FILE_TYPE_UNKNOWN) {
        const struct pfm_header *pfm_header;
        UINT64 filesize;
        DWORD offset;
        BOOL header_checked;

        hr = IDWriteFontFileStream_GetFileSize(stream, &filesize);
        if (FAILED(hr))
            return hr;

        hr = IDWriteFontFileStream_ReadFileFragment(stream, (const void**)&pfm_header, 0, sizeof(*pfm_header), &context);
        if (FAILED(hr))
            return hr;

        offset = pfm_header->dfDevice;
        header_checked = pfm_header->dfVersion == 0x100 && pfm_header->dfSize == filesize;
        IDWriteFontFileStream_ReleaseFileFragment(stream, context);

        /* as a last test check static string in PostScript information section */
        if (header_checked) {
            static const char postscript[] = "PostScript";
            char *devtype_name;

            hr = IDWriteFontFileStream_ReadFileFragment(stream, (const void**)&devtype_name, offset, sizeof(postscript), &context);
            if (FAILED(hr))
                return hr;

            if (!memcmp(devtype_name, postscript, sizeof(postscript))) {
                *font_count = 1;
                *file_type = DWRITE_FONT_FILE_TYPE_TYPE1_PFM;
                *face_type = DWRITE_FONT_FACE_TYPE_TYPE1;
            }

            IDWriteFontFileStream_ReleaseFileFragment(stream, context);
        }
    }

    return *file_type != DWRITE_FONT_FILE_TYPE_UNKNOWN ? S_OK : S_FALSE;
}

HRESULT opentype_analyze_font(IDWriteFontFileStream *stream, BOOL *supported, DWRITE_FONT_FILE_TYPE *file_type,
        DWRITE_FONT_FACE_TYPE *face_type, UINT32 *face_count)
{
    static dwrite_fontfile_analyzer fontfile_analyzers[] = {
        opentype_ttf_analyzer,
        opentype_otf_analyzer,
        opentype_ttc_analyzer,
        opentype_type1_analyzer,
        NULL
    };
    dwrite_fontfile_analyzer *analyzer = fontfile_analyzers;
    DWRITE_FONT_FACE_TYPE face;
    HRESULT hr;

    if (!face_type)
        face_type = &face;

    *file_type = DWRITE_FONT_FILE_TYPE_UNKNOWN;
    *face_type = DWRITE_FONT_FACE_TYPE_UNKNOWN;
    *face_count = 0;

    while (*analyzer) {
        hr = (*analyzer)(stream, face_count, file_type, face_type);
        if (FAILED(hr))
            return hr;

        if (hr == S_OK)
            break;

        analyzer++;
    }

    *supported = is_face_type_supported(*face_type);
    return S_OK;
}

HRESULT opentype_try_get_font_table(const struct file_stream_desc *stream_desc, UINT32 tag, const void **table_data,
    void **table_context, UINT32 *table_size, BOOL *found)
{
    void *table_directory_context, *sfnt_context;
    const struct ot_table_record *table_record = NULL;
    const struct ot_table_dir *table_dir = NULL;
    UINT32 table_offset = 0;
    UINT16 table_count;
    HRESULT hr;

    if (found) *found = FALSE;
    if (table_size) *table_size = 0;

    *table_data = NULL;
    *table_context = NULL;

    if (stream_desc->face_type == DWRITE_FONT_FACE_TYPE_OPENTYPE_COLLECTION)
    {
        const struct ttc_header *ttc_header;
        void * ttc_context;

        hr = IDWriteFontFileStream_ReadFileFragment(stream_desc->stream, (const void **)&ttc_header, 0,
                sizeof(*ttc_header), &ttc_context);
        if (SUCCEEDED(hr))
        {
            if (stream_desc->face_index >= GET_BE_DWORD(ttc_header->num_fonts))
                hr = E_INVALIDARG;
            else
            {
                table_offset = GET_BE_DWORD(ttc_header->offsets[stream_desc->face_index]);
                hr = IDWriteFontFileStream_ReadFileFragment(stream_desc->stream, (const void **)&table_dir, table_offset,
                        sizeof(*table_dir), &sfnt_context);
            }
            IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, ttc_context);
        }
    }
    else
        hr = IDWriteFontFileStream_ReadFileFragment(stream_desc->stream, (const void **)&table_dir, 0,
                sizeof(*table_dir), &sfnt_context);

    if (FAILED(hr))
        return hr;

    table_count = GET_BE_WORD(table_dir->numTables);
    table_offset += sizeof(*table_dir);

    IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, sfnt_context);

    hr = IDWriteFontFileStream_ReadFileFragment(stream_desc->stream, (const void **)&table_record, table_offset,
            table_count * sizeof(*table_record), &table_directory_context);
    if (hr == S_OK)
    {
        UINT16 i;

        for (i = 0; i < table_count; ++i)
        {
            if (table_record->tag == tag)
            {
                UINT32 offset = GET_BE_DWORD(table_record->offset);
                UINT32 length = GET_BE_DWORD(table_record->length);

                if (found)
                    *found = TRUE;
                if (table_size)
                    *table_size = length;
                hr = IDWriteFontFileStream_ReadFileFragment(stream_desc->stream, table_data, offset,
                        length, table_context);
                break;
            }
            table_record++;
        }

        IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, table_directory_context);
    }

    return hr;
}

static HRESULT opentype_get_font_table(const struct file_stream_desc *stream_desc, UINT32 tag,
        struct dwrite_fonttable *table)
{
    return opentype_try_get_font_table(stream_desc, tag, (const void **)&table->data, &table->context, &table->size, &table->exists);
}

/**********
 * CMAP
 **********/

static UINT16 opentype_cmap_format0_get_glyph(const struct dwrite_cmap *cmap, unsigned int ch)
{
    const UINT8 *glyphs = cmap->data;
    return (ch < 0xff) ? glyphs[ch] : 0;
}

static unsigned int opentype_cmap_format0_get_ranges(const struct dwrite_cmap *cmap, unsigned int count,
        DWRITE_UNICODE_RANGE *ranges)
{
    if (count > 0)
    {
        ranges->first = 0;
        ranges->last = 255;
    }

    return 1;
}

struct cmap_format4_compare_context
{
    const struct dwrite_cmap *cmap;
    unsigned int ch;
};

static int __cdecl cmap_format4_compare_range(const void *a, const void *b)
{
    const struct cmap_format4_compare_context *key = a;
    const UINT16 *end = b;
    unsigned int idx;

    if (key->ch > GET_BE_WORD(*end))
        return 1;

    idx = end - key->cmap->u.format4.ends;
    if (key->ch < GET_BE_WORD(key->cmap->u.format4.starts[idx]))
        return -1;

    return 0;
}

static UINT16 opentype_cmap_format4_get_glyph(const struct dwrite_cmap *cmap, unsigned int ch)
{
    struct cmap_format4_compare_context key = { .cmap = cmap, .ch = ch };
    unsigned int glyph, idx, range_offset;
    const UINT16 *end_found;

    /* Look up range. */
    end_found = bsearch(&key, cmap->u.format4.ends, cmap->u.format4.seg_count, sizeof(*cmap->u.format4.ends),
            cmap_format4_compare_range);
    if (!end_found)
        return 0;

    idx = end_found - cmap->u.format4.ends;

    range_offset = GET_BE_WORD(cmap->u.format4.id_range_offset[idx]);

    if (!range_offset)
    {
        glyph = ch + GET_BE_WORD(cmap->u.format4.id_delta[idx]);
    }
    else
    {
        unsigned int index = range_offset / 2 + (ch - GET_BE_WORD(cmap->u.format4.starts[idx])) + idx - cmap->u.format4.seg_count;
        if (index >= cmap->u.format4.glyph_id_array_len)
            return 0;
        glyph = GET_BE_WORD(cmap->u.format4.glyph_id_array[index]);
        if (!glyph)
            return 0;
        glyph += GET_BE_WORD(cmap->u.format4.id_delta[idx]);
    }

    return glyph & 0xffff;
}

static unsigned int opentype_cmap_format4_get_ranges(const struct dwrite_cmap *cmap, unsigned int count,
        DWRITE_UNICODE_RANGE *ranges)
{
    unsigned int i;

    count = min(count, cmap->u.format4.seg_count);

    for (i = 0; i < count; ++i)
    {
        ranges[i].first = GET_BE_WORD(cmap->u.format4.starts[i]);
        ranges[i].last = GET_BE_WORD(cmap->u.format4.ends[i]);
    }

    return cmap->u.format4.seg_count;
}

static UINT16 opentype_cmap_format6_10_get_glyph(const struct dwrite_cmap *cmap, unsigned int ch)
{
    const UINT16 *glyphs = cmap->data;
    if (ch < cmap->u.format6_10.first || ch > cmap->u.format6_10.last) return 0;
    return glyphs[ch - cmap->u.format6_10.first];
}

static unsigned int opentype_cmap_format6_10_get_ranges(const struct dwrite_cmap *cmap, unsigned int count,
        DWRITE_UNICODE_RANGE *ranges)
{
    if (count > 0)
    {
        ranges->first = cmap->u.format6_10.first;
        ranges->last = cmap->u.format6_10.last;
    }

    return 1;
}

static int __cdecl cmap_format12_13_compare_group(const void *a, const void *b)
{
    const unsigned int *ch = a;
    const UINT32 *group = b;

    if (*ch > GET_BE_DWORD(group[1]))
        return 1;

    if (*ch < GET_BE_DWORD(group[0]))
        return -1;

    return 0;
}

static UINT16 opentype_cmap_format12_get_glyph(const struct dwrite_cmap *cmap, unsigned int ch)
{
    const UINT32 *groups = cmap->data;
    const UINT32 *group_found;

    if (!(group_found = bsearch(&ch, groups, cmap->u.format12_13.group_count, 3 * sizeof(*groups),
            cmap_format12_13_compare_group)))
        return 0;

    return GET_BE_DWORD(group_found[0]) <= GET_BE_DWORD(group_found[1]) ?
            GET_BE_DWORD(group_found[2]) + (ch - GET_BE_DWORD(group_found[0])) : 0;
}

static unsigned int opentype_cmap_format12_13_get_ranges(const struct dwrite_cmap *cmap, unsigned int count,
        DWRITE_UNICODE_RANGE *ranges)
{
    unsigned int i, group_count = cmap->u.format12_13.group_count;
    const UINT32 *groups = cmap->data;

    count = min(count, group_count);

    for (i = 0; i < count; ++i)
    {
        ranges[i].first = GET_BE_DWORD(groups[3 * i]);
        ranges[i].last = GET_BE_DWORD(groups[3 * i + 1]);
    }

    return group_count;
}

static UINT16 opentype_cmap_format13_get_glyph(const struct dwrite_cmap *cmap, unsigned int ch)
{
    const UINT32 *groups = cmap->data;
    const UINT32 *group_found;

    if (!(group_found = bsearch(&ch, groups, cmap->u.format12_13.group_count, 3 * sizeof(*groups),
            cmap_format12_13_compare_group)))
        return 0;

    return GET_BE_DWORD(group_found[2]);
}

static UINT16 opentype_cmap_dummy_get_glyph(const struct dwrite_cmap *cmap, unsigned int ch)
{
    return 0;
}

static unsigned int opentype_cmap_dummy_get_ranges(const struct dwrite_cmap *cmap, unsigned int count,
        DWRITE_UNICODE_RANGE *ranges)
{
    return 0;
}

UINT16 opentype_cmap_get_glyph(const struct dwrite_cmap *cmap, unsigned int ch)
{
    UINT16 glyph;

    if (!cmap->get_glyph) return 0;
    glyph = cmap->get_glyph(cmap, ch);
    if (!glyph && cmap->symbol && ch <= 0xff)
        glyph = cmap->get_glyph(cmap, ch + 0xf000);
    return glyph;
}

static int __cdecl cmap_header_compare(const void *a, const void *b)
{
    const UINT16 *key = a;
    const UINT16 *record = b;

    /* Platform. */
    if (key[0] < GET_BE_WORD(record[0])) return -1;
    if (key[0] > GET_BE_WORD(record[0])) return 1;
    /* Encoding. */
    if (key[1] < GET_BE_WORD(record[1])) return -1;
    if (key[1] > GET_BE_WORD(record[1])) return 1;

    return 0;
}

void dwrite_cmap_init(struct dwrite_cmap *cmap, IDWriteFontFile *file, unsigned int face_index,
        DWRITE_FONT_FACE_TYPE face_type)
{
    static const UINT16 encodings[][2] =
    {
        { 3, 0 }, /* MS Symbol encoding is preferred. */
        { 3, 10 },
        { 0, 6 },
        { 0, 4 },
        { 3, 1 },
        { 0, 3 },
        { 0, 2 },
        { 0, 1 },
        { 0, 0 },
    };
    const struct cmap_encoding_record *records, *found_record = NULL;
    unsigned int length, offset, format, count, f, i, num_records;
    struct file_stream_desc stream_desc;
    struct dwrite_fonttable table;
    const UINT16 *pair = NULL;
    HRESULT hr;

    if (cmap->data) return;

    /* For fontface stream is already available and preset. */
    if (!cmap->stream && FAILED(hr = get_filestream_from_file(file, &cmap->stream)))
    {
        WARN("Failed to get file stream, hr %#lx.\n", hr);
        goto failed;
    }

    stream_desc.stream = cmap->stream;
    stream_desc.face_type = face_type;
    stream_desc.face_index = face_index;

    opentype_get_font_table(&stream_desc, MS_CMAP_TAG, &table);
    if (!table.exists)
        goto failed;
    cmap->table_context = table.context;

    num_records = table_read_be_word(&table, 2);
    records = table_read_ensure(&table, 4, sizeof(*records) * num_records);

    for (i = 0; i < ARRAY_SIZE(encodings); ++i)
    {
        pair = encodings[i];
        if ((found_record = bsearch(pair, records, num_records, sizeof(*records), cmap_header_compare)))
            break;
    }

    if (!found_record)
    {
        WARN("No suitable cmap table were found.\n");
        goto failed;
    }

    /* Symbol encoding. */
    cmap->symbol = pair[0] == 3 && pair[1] == 0;
    offset = GET_BE_DWORD(found_record->offset);

    format = table_read_be_word(&table, offset);

    switch (format)
    {
        case 0:
            cmap->data = table_read_ensure(&table, offset + 6, 256);
            cmap->get_glyph = opentype_cmap_format0_get_glyph;
            cmap->get_ranges = opentype_cmap_format0_get_ranges;
            break;
        case 4:
            length = table_read_be_word(&table, offset + 2);
            cmap->u.format4.seg_count = count = table_read_be_word(&table, offset + 6) / 2;
            cmap->u.format4.ends = table_read_ensure(&table, offset + 14, count * 2);
            cmap->u.format4.starts = cmap->u.format4.ends + count + 1;
            cmap->u.format4.id_delta = cmap->u.format4.starts + count;
            cmap->u.format4.id_range_offset = cmap->u.format4.id_delta + count;
            cmap->u.format4.glyph_id_array = cmap->data = cmap->u.format4.id_range_offset + count;
            cmap->u.format4.glyph_id_array_len = (length - 16 - 8 * count) / 2;
            cmap->get_glyph = opentype_cmap_format4_get_glyph;
            cmap->get_ranges = opentype_cmap_format4_get_ranges;
            break;
        case 6:
        case 10:
            /* Format 10 uses 4 byte fields. */
            f = format == 6 ? 1 : 2;
            cmap->u.format6_10.first = table_read_be_word(&table, offset + f * 6);
            count = table_read_be_word(&table, offset + f * 8);
            cmap->u.format6_10.last = cmap->u.format6_10.first + count;
            cmap->data = table_read_ensure(&table, offset + f * 10, count * 2);
            cmap->get_glyph = opentype_cmap_format6_10_get_glyph;
            cmap->get_ranges = opentype_cmap_format6_10_get_ranges;
            break;
        case 12:
        case 13:
            cmap->u.format12_13.group_count = count = table_read_be_dword(&table, offset + 12);
            cmap->data = table_read_ensure(&table, offset + 16, count * 3 * 4);
            cmap->get_glyph = format == 12 ? opentype_cmap_format12_get_glyph : opentype_cmap_format13_get_glyph;
            cmap->get_ranges = opentype_cmap_format12_13_get_ranges;
            break;
        default:
            WARN("Unhandled subtable format %u.\n", format);
    }

failed:

    if (!cmap->data)
    {
        /* Dummy implementation, returns 0 unconditionally. */
        cmap->data = cmap;
        cmap->get_glyph = opentype_cmap_dummy_get_glyph;
        cmap->get_ranges = opentype_cmap_dummy_get_ranges;
    }
}

void dwrite_cmap_release(struct dwrite_cmap *cmap)
{
    if (cmap->stream)
    {
        IDWriteFontFileStream_ReleaseFileFragment(cmap->stream, cmap->table_context);
        IDWriteFontFileStream_Release(cmap->stream);
    }
    cmap->data = NULL;
    cmap->stream = NULL;
}

HRESULT opentype_cmap_get_unicode_ranges(const struct dwrite_cmap *cmap, unsigned int max_count, DWRITE_UNICODE_RANGE *ranges,
        unsigned int *count)
{
    if (!cmap->data)
        return E_FAIL;

    *count = cmap->get_ranges(cmap, max_count, ranges);

    return *count > max_count ? E_NOT_SUFFICIENT_BUFFER : S_OK;
}

void opentype_get_font_typo_metrics(struct file_stream_desc *stream_desc, unsigned int *ascent, unsigned int *descent)
{
    struct dwrite_fonttable os2;

    opentype_get_font_table(stream_desc, MS_OS2_TAG, &os2);

    *ascent = *descent = 0;

    if (os2.size >= FIELD_OFFSET(struct tt_os2, sTypoLineGap))
    {
        SHORT value = table_read_be_word(&os2, FIELD_OFFSET(struct tt_os2, sTypoDescender));
        *ascent = table_read_be_word(&os2, FIELD_OFFSET(struct tt_os2, sTypoAscender));
        *descent = value < 0 ? -value : 0;
    }

    if (os2.data)
        IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, os2.context);
}

void opentype_get_font_metrics(struct file_stream_desc *stream_desc, DWRITE_FONT_METRICS1 *metrics, DWRITE_CARET_METRICS *caret)
{
    struct dwrite_fonttable os2, head, post, hhea;

    memset(metrics, 0, sizeof(*metrics));

    opentype_get_font_table(stream_desc, MS_OS2_TAG,  &os2);
    opentype_get_font_table(stream_desc, MS_HEAD_TAG, &head);
    opentype_get_font_table(stream_desc, MS_POST_TAG, &post);
    opentype_get_font_table(stream_desc, MS_HHEA_TAG, &hhea);

    if (head.data)
    {
        metrics->designUnitsPerEm = table_read_be_word(&head, FIELD_OFFSET(struct tt_head, unitsPerEm));
        metrics->glyphBoxLeft = table_read_be_word(&head, FIELD_OFFSET(struct tt_head, xMin));
        metrics->glyphBoxTop = table_read_be_word(&head, FIELD_OFFSET(struct tt_head, yMax));
        metrics->glyphBoxRight = table_read_be_word(&head, FIELD_OFFSET(struct tt_head, xMax));
        metrics->glyphBoxBottom = table_read_be_word(&head, FIELD_OFFSET(struct tt_head, yMin));
    }

    if (caret)
    {
        if (hhea.data)
        {
            caret->slopeRise = table_read_be_word(&hhea, FIELD_OFFSET(struct tt_hhea, caretSlopeRise));
            caret->slopeRun = table_read_be_word(&hhea, FIELD_OFFSET(struct tt_hhea, caretSlopeRun));
            caret->offset = table_read_be_word(&hhea, FIELD_OFFSET(struct tt_hhea, caretOffset));
        }
        else
            memset(caret, 0, sizeof(*caret));
    }

    if (os2.data)
    {
        USHORT version = table_read_be_word(&os2, FIELD_OFFSET(struct tt_os2, version));

        metrics->ascent = table_read_be_word(&os2, FIELD_OFFSET(struct tt_os2, usWinAscent));
        /* Some fonts have usWinDescent value stored as signed short, which could be wrongly
           interpreted as large unsigned value. */
        metrics->descent = abs((SHORT)table_read_be_word(&os2, FIELD_OFFSET(struct tt_os2, usWinDescent)));

        /* Line gap is estimated using two sets of ascender/descender values and 'hhea' line gap. */
        if (hhea.data)
        {
            SHORT descender = (SHORT)table_read_be_word(&hhea, FIELD_OFFSET(struct tt_hhea, descender));
            INT32 linegap;

            linegap = table_read_be_word(&hhea, FIELD_OFFSET(struct tt_hhea, ascender)) + abs(descender) +
                    table_read_be_word(&hhea, FIELD_OFFSET(struct tt_hhea, linegap)) - metrics->ascent - metrics->descent;
            metrics->lineGap = linegap > 0 ? linegap : 0;
        }

        metrics->strikethroughPosition  = table_read_be_word(&os2, FIELD_OFFSET(struct tt_os2, yStrikeoutPosition));
        metrics->strikethroughThickness = table_read_be_word(&os2, FIELD_OFFSET(struct tt_os2, yStrikeoutSize));
        metrics->subscriptPositionX = table_read_be_word(&os2, FIELD_OFFSET(struct tt_os2, ySubscriptXOffset));
        /* Y offset is stored as positive offset below baseline */
        metrics->subscriptPositionY = -table_read_be_word(&os2, FIELD_OFFSET(struct tt_os2, ySubscriptYOffset));
        metrics->subscriptSizeX = table_read_be_word(&os2, FIELD_OFFSET(struct tt_os2, ySubscriptXSize));
        metrics->subscriptSizeY = table_read_be_word(&os2, FIELD_OFFSET(struct tt_os2, ySubscriptYSize));
        metrics->superscriptPositionX = table_read_be_word(&os2, FIELD_OFFSET(struct tt_os2, ySuperscriptXOffset));
        metrics->superscriptPositionY = table_read_be_word(&os2, FIELD_OFFSET(struct tt_os2, ySuperscriptYOffset));
        metrics->superscriptSizeX = table_read_be_word(&os2, FIELD_OFFSET(struct tt_os2, ySuperscriptXSize));
        metrics->superscriptSizeY = table_read_be_word(&os2, FIELD_OFFSET(struct tt_os2, ySuperscriptYSize));

        /* version 2 fields */
        if (version >= 2)
        {
            metrics->capHeight = table_read_be_word(&os2, FIELD_OFFSET(struct tt_os2, sCapHeight));
            metrics->xHeight   = table_read_be_word(&os2, FIELD_OFFSET(struct tt_os2, sxHeight));
        }

        if (table_read_be_word(&os2, FIELD_OFFSET(struct tt_os2, fsSelection)) & OS2_FSSELECTION_USE_TYPO_METRICS)
        {
            SHORT descent = table_read_be_word(&os2, FIELD_OFFSET(struct tt_os2, sTypoDescender));
            metrics->ascent = table_read_be_word(&os2, FIELD_OFFSET(struct tt_os2, sTypoAscender));
            metrics->descent = descent < 0 ? -descent : 0;
            metrics->lineGap = table_read_be_word(&os2, FIELD_OFFSET(struct tt_os2, sTypoLineGap));
            metrics->hasTypographicMetrics = TRUE;
        }
    }
    else
    {
        metrics->strikethroughPosition = metrics->designUnitsPerEm / 3;
        if (hhea.data)
        {
            metrics->ascent = table_read_be_word(&hhea, FIELD_OFFSET(struct tt_hhea, ascender));
            metrics->descent = abs((SHORT)table_read_be_word(&hhea, FIELD_OFFSET(struct tt_hhea, descender)));
        }
    }

    if (post.data)
    {
        metrics->underlinePosition = table_read_be_word(&post, FIELD_OFFSET(struct tt_post, underlinePosition));
        metrics->underlineThickness = table_read_be_word(&post, FIELD_OFFSET(struct tt_post, underlineThickness));
    }

    if (metrics->underlineThickness == 0)
        metrics->underlineThickness = metrics->designUnitsPerEm / 14;
    if (metrics->strikethroughThickness == 0)
        metrics->strikethroughThickness = metrics->underlineThickness;

    /* estimate missing metrics */
    if (metrics->xHeight == 0)
        metrics->xHeight = metrics->designUnitsPerEm / 2;
    if (metrics->capHeight == 0)
        metrics->capHeight = metrics->designUnitsPerEm * 7 / 10;

    if (os2.data)
        IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, os2.context);
    if (head.data)
        IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, head.context);
    if (post.data)
        IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, post.context);
    if (hhea.data)
        IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, hhea.context);
}

void opentype_get_font_properties(const struct file_stream_desc *stream_desc, struct dwrite_font_props *props)
{
    struct dwrite_fonttable os2, head, post, colr, cpal;
    BOOL is_symbol, is_monospaced;

    opentype_get_font_table(stream_desc, MS_OS2_TAG, &os2);
    opentype_get_font_table(stream_desc, MS_HEAD_TAG, &head);

    memset(props, 0, sizeof(*props));

    /* Default stretch, weight and style to normal */
    props->stretch = DWRITE_FONT_STRETCH_NORMAL;
    props->weight = DWRITE_FONT_WEIGHT_NORMAL;
    props->style = DWRITE_FONT_STYLE_NORMAL;

    /* DWRITE_FONT_STRETCH enumeration values directly match font data values */
    if (os2.data)
    {
        USHORT version = table_read_be_word(&os2, FIELD_OFFSET(struct tt_os2, version));
        USHORT fsSelection = table_read_be_word(&os2, FIELD_OFFSET(struct tt_os2, fsSelection));
        USHORT usWeightClass = table_read_be_word(&os2, FIELD_OFFSET(struct tt_os2, usWeightClass));
        USHORT usWidthClass = table_read_be_word(&os2, FIELD_OFFSET(struct tt_os2, usWidthClass));
        const void *panose;

        if (usWidthClass > DWRITE_FONT_STRETCH_UNDEFINED && usWidthClass <= DWRITE_FONT_STRETCH_ULTRA_EXPANDED)
            props->stretch = usWidthClass;

        if (usWeightClass >= 1 && usWeightClass <= 9)
            usWeightClass *= 100;

        if (usWeightClass > DWRITE_FONT_WEIGHT_ULTRA_BLACK)
            props->weight = DWRITE_FONT_WEIGHT_ULTRA_BLACK;
        else if (usWeightClass > 0)
            props->weight = usWeightClass;

        if (version >= 4 && (fsSelection & OS2_FSSELECTION_OBLIQUE))
            props->style = DWRITE_FONT_STYLE_OBLIQUE;
        else if (fsSelection & OS2_FSSELECTION_ITALIC)
            props->style = DWRITE_FONT_STYLE_ITALIC;
        props->lf.lfItalic = !!(fsSelection & OS2_FSSELECTION_ITALIC);

        if ((panose = table_read_ensure(&os2, FIELD_OFFSET(struct tt_os2, panose), sizeof(props->panose))))
            memcpy(&props->panose, panose, sizeof(props->panose));

        /* FONTSIGNATURE */
        props->fontsig.fsUsb[0] = table_read_be_dword(&os2, FIELD_OFFSET(struct tt_os2, ulUnicodeRange1));
        props->fontsig.fsUsb[1] = table_read_be_dword(&os2, FIELD_OFFSET(struct tt_os2, ulUnicodeRange2));
        props->fontsig.fsUsb[2] = table_read_be_dword(&os2, FIELD_OFFSET(struct tt_os2, ulUnicodeRange3));
        props->fontsig.fsUsb[3] = table_read_be_dword(&os2, FIELD_OFFSET(struct tt_os2, ulUnicodeRange4));

        if (version)
        {
            props->fontsig.fsCsb[0] = table_read_be_dword(&os2, FIELD_OFFSET(struct tt_os2, ulCodePageRange1));
            props->fontsig.fsCsb[1] = table_read_be_dword(&os2, FIELD_OFFSET(struct tt_os2, ulCodePageRange2));
        }
    }
    else if (head.data)
    {
        USHORT macStyle = table_read_be_word(&head, FIELD_OFFSET(struct tt_head, macStyle));

        if (macStyle & TT_HEAD_MACSTYLE_CONDENSED)
            props->stretch = DWRITE_FONT_STRETCH_CONDENSED;
        else if (macStyle & TT_HEAD_MACSTYLE_EXTENDED)
            props->stretch = DWRITE_FONT_STRETCH_EXPANDED;

        if (macStyle & TT_HEAD_MACSTYLE_BOLD)
            props->weight = DWRITE_FONT_WEIGHT_BOLD;

        if (macStyle & TT_HEAD_MACSTYLE_ITALIC) {
            props->style = DWRITE_FONT_STYLE_ITALIC;
            props->lf.lfItalic = 1;
        }
    }

    props->lf.lfWeight = props->weight;

    /* FONT_IS_SYMBOL */
    if (!(is_symbol = props->panose.familyKind == DWRITE_PANOSE_FAMILY_SYMBOL))
    {
        struct dwrite_fonttable cmap;
        int i, offset, num_tables;

        opentype_get_font_table(stream_desc, MS_CMAP_TAG, &cmap);

        if (cmap.data)
        {
            num_tables = table_read_be_word(&cmap, FIELD_OFFSET(struct cmap_header, num_tables));
            offset = FIELD_OFFSET(struct cmap_header, tables);

            for (i = 0; !is_symbol && i < num_tables; ++i)
            {
                WORD platform, encoding;

                platform = table_read_be_word(&cmap, offset + i * sizeof(struct cmap_encoding_record) +
                        FIELD_OFFSET(struct cmap_encoding_record, platformID));
                encoding = table_read_be_word(&cmap, offset + i * sizeof(struct cmap_encoding_record) +
                        FIELD_OFFSET(struct cmap_encoding_record, encodingID));

                is_symbol = platform == OPENTYPE_CMAP_TABLE_PLATFORM_WIN &&
                        encoding == OPENTYPE_CMAP_TABLE_ENCODING_SYMBOL;
            }

            IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, cmap.context);
        }
    }
    if (is_symbol)
        props->flags |= FONT_IS_SYMBOL;

    /* FONT_IS_MONOSPACED, slant angle */
    opentype_get_font_table(stream_desc, MS_POST_TAG, &post);
    is_monospaced = props->panose.text.proportion == DWRITE_PANOSE_PROPORTION_MONOSPACED;
    if (post.data)
    {
        if (!is_monospaced)
            is_monospaced = !!table_read_dword(&post, FIELD_OFFSET(struct tt_post, fixed_pitch));
        props->slant_angle = table_read_be_fixed(&post, FIELD_OFFSET(struct tt_post, italicAngle));
    }
    if (post.context)
        IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, post.context);

    if (is_monospaced)
        props->flags |= FONT_IS_MONOSPACED;

    /* FONT_IS_COLORED */
    opentype_get_font_table(stream_desc, MS_COLR_TAG, &colr);
    if (colr.data)
    {
        opentype_get_font_table(stream_desc, MS_CPAL_TAG, &cpal);
        if (cpal.data)
        {
            props->flags |= FONT_IS_COLORED;
            IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, cpal.context);
        }

        IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, colr.context);
    }

    TRACE("stretch %d, weight %d, style %d\n", props->stretch, props->weight, props->style);

    if (os2.data)
        IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, os2.context);
    if (head.data)
        IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, head.context);
}

static UINT get_name_record_codepage(enum OPENTYPE_PLATFORM_ID platform, USHORT encoding)
{
    UINT codepage = 0;

    switch (platform) {
    case OPENTYPE_PLATFORM_UNICODE:
        break;
    case OPENTYPE_PLATFORM_MAC:
        switch (encoding)
        {
            case TT_NAME_MAC_ENCODING_ROMAN:
                codepage = 10000;
                break;
            case TT_NAME_MAC_ENCODING_JAPANESE:
                codepage = 10001;
                break;
            case TT_NAME_MAC_ENCODING_TRAD_CHINESE:
                codepage = 10002;
                break;
            case TT_NAME_MAC_ENCODING_KOREAN:
                codepage = 10003;
                break;
            case TT_NAME_MAC_ENCODING_ARABIC:
                codepage = 10004;
                break;
            case TT_NAME_MAC_ENCODING_HEBREW:
                codepage = 10005;
                break;
            case TT_NAME_MAC_ENCODING_GREEK:
                codepage = 10006;
                break;
            case TT_NAME_MAC_ENCODING_RUSSIAN:
                codepage = 10007;
                break;
            case TT_NAME_MAC_ENCODING_SIMPL_CHINESE:
                codepage = 10008;
                break;
            case TT_NAME_MAC_ENCODING_THAI:
                codepage = 10021;
                break;
            default:
                FIXME("encoding %u not handled, platform %d.\n", encoding, platform);
                break;
        }
        break;
    case OPENTYPE_PLATFORM_WIN:
        switch (encoding)
        {
            case TT_NAME_WINDOWS_ENCODING_SYMBOL:
            case TT_NAME_WINDOWS_ENCODING_UNICODE_BMP:
            case TT_NAME_WINDOWS_ENCODING_UNICODE_FULL:
                break;
            case TT_NAME_WINDOWS_ENCODING_SJIS:
                codepage = 932;
                break;
            case TT_NAME_WINDOWS_ENCODING_PRC:
                codepage = 936;
                break;
            case TT_NAME_WINDOWS_ENCODING_BIG5:
                codepage = 950;
                break;
            case TT_NAME_WINDOWS_ENCODING_WANSUNG:
                codepage = 20949;
                break;
            case TT_NAME_WINDOWS_ENCODING_JOHAB:
                codepage = 1361;
                break;
            default:
                FIXME("encoding %u not handled, platform %d.\n", encoding, platform);
                break;
        }
        break;
    default:
        FIXME("unknown platform %d\n", platform);
    }

    return codepage;
}

static void get_name_record_locale(enum OPENTYPE_PLATFORM_ID platform, USHORT lang_id, WCHAR *locale, USHORT locale_len)
{
    switch (platform)
    {
    case OPENTYPE_PLATFORM_MAC:
    {
        const char *locale_name = NULL;

        if (lang_id > TT_NAME_MAC_LANGID_AZER_ROMAN)
            WARN("invalid mac lang id %d\n", lang_id);
        else if (!name_mac_langid_to_locale[lang_id][0])
            FIXME("failed to map mac lang id %d to locale name\n", lang_id);
        else
            locale_name = name_mac_langid_to_locale[lang_id];

        if (locale_name)
            MultiByteToWideChar(CP_ACP, 0, name_mac_langid_to_locale[lang_id], -1, locale, locale_len);
        else
            wcscpy(locale, L"en-US");
        break;
    }
    case OPENTYPE_PLATFORM_WIN:
        if (!LCIDToLocaleName(MAKELCID(lang_id, SORT_DEFAULT), locale, locale_len, 0))
        {
            FIXME("failed to get locale name for lcid=0x%08lx\n", MAKELCID(lang_id, SORT_DEFAULT));
            wcscpy(locale, L"en-US");
        }
        break;
    case OPENTYPE_PLATFORM_UNICODE:
        wcscpy(locale, L"en-US");
        break;
    default:
        FIXME("unknown platform %d\n", platform);
    }
}

static BOOL opentype_is_english_namerecord(const struct dwrite_fonttable *table, unsigned int idx)
{
    const struct name_header *header = (const struct name_header *)table->data;
    const struct name_record *record;

    record = &header->records[idx];

    return GET_BE_WORD(record->platformID) == OPENTYPE_PLATFORM_MAC &&
            GET_BE_WORD(record->languageID) == TT_NAME_MAC_LANGID_ENGLISH;
}

static BOOL opentype_decode_namerecord(const struct dwrite_fonttable *table, unsigned int idx,
        IDWriteLocalizedStrings *strings)
{
    USHORT lang_id, length, offset, encoding, platform;
    const struct name_header *header = (const struct name_header *)table->data;
    const struct name_record *record;
    unsigned int i, string_offset;
    BOOL ret = FALSE;
    const void *name;

    string_offset = table_read_be_word(table, FIELD_OFFSET(struct name_header, stringOffset));

    record = &header->records[idx];

    platform = GET_BE_WORD(record->platformID);
    lang_id = GET_BE_WORD(record->languageID);
    length = GET_BE_WORD(record->length);
    offset = GET_BE_WORD(record->offset);
    encoding = GET_BE_WORD(record->encodingID);

    if (!(name = table_read_ensure(table, string_offset + offset, length)))
        return FALSE;

    if (lang_id < 0x8000)
    {
        WCHAR locale[LOCALE_NAME_MAX_LENGTH];
        WCHAR *name_string;
        UINT codepage;

        codepage = get_name_record_codepage(platform, encoding);
        get_name_record_locale(platform, lang_id, locale, ARRAY_SIZE(locale));

        if (codepage)
        {
            DWORD len = MultiByteToWideChar(codepage, 0, name, length, NULL, 0);
            name_string = malloc(sizeof(WCHAR) * (len+1));
            MultiByteToWideChar(codepage, 0, name, length, name_string, len);
            name_string[len] = 0;
        }
        else
        {
            length /= sizeof(WCHAR);
            name_string = heap_strdupnW(name, length);
            for (i = 0; i < length; i++)
                name_string[i] = GET_BE_WORD(name_string[i]);
        }

        TRACE("string %s for locale %s found\n", debugstr_w(name_string), debugstr_w(locale));
        add_localizedstring(strings, locale, name_string);
        free(name_string);

        ret = !wcscmp(locale, L"en-US");
    }
    else
        FIXME("handle NAME format 1\n");

    return ret;
}

static HRESULT opentype_get_font_strings_from_id(const struct dwrite_fonttable *table, enum OPENTYPE_STRING_ID id,
        IDWriteLocalizedStrings **strings)
{
    int i, count, candidate_mac, candidate_mac_en, candidate_unicode;
    const struct name_record *records;
    BOOL has_english;
    WORD format;
    HRESULT hr;

    if (!table->data)
        return E_FAIL;

    if (FAILED(hr = create_localizedstrings(strings)))
        return hr;

    format = table_read_be_word(table, FIELD_OFFSET(struct name_header, format));

    if (format != 0 && format != 1)
        FIXME("unsupported NAME format %d\n", format);

    count = table_read_be_word(table, FIELD_OFFSET(struct name_header, count));

    if (!(records = table_read_ensure(table, FIELD_OFFSET(struct name_header, records),
                count * sizeof(struct name_record))))
    {
        count = 0;
    }

    has_english = FALSE;
    candidate_unicode = candidate_mac = candidate_mac_en = -1;

    for (i = 0; i < count; i++)
    {
        unsigned short platform;

        if (GET_BE_WORD(records[i].nameID) != id)
            continue;

        platform = GET_BE_WORD(records[i].platformID);
        switch (platform)
        {
            /* Skip Unicode or Mac entries for now, fonts tend to duplicate those
               strings as WIN platform entries. If font does not have WIN entry for
               this id, we will use Mac or Unicode platform entry while assuming
               en-US locale. */
            case OPENTYPE_PLATFORM_UNICODE:
                if (candidate_unicode == -1)
                    candidate_unicode = i;
                break;
            case OPENTYPE_PLATFORM_MAC:
                if (candidate_mac == -1)
                    candidate_mac = i;
                if (candidate_mac_en == -1 && opentype_is_english_namerecord(table, i))
                    candidate_mac_en = i;
                break;
            case OPENTYPE_PLATFORM_WIN:
                has_english |= opentype_decode_namerecord(table, i, *strings);
                break;
            default:
                FIXME("platform %i not supported\n", platform);
                break;
        }
    }

    if (!get_localizedstrings_count(*strings) && candidate_mac != -1)
        has_english |= opentype_decode_namerecord(table, candidate_mac, *strings);
    if (!get_localizedstrings_count(*strings) && candidate_unicode != -1)
        has_english |= opentype_decode_namerecord(table, candidate_unicode, *strings);
    if (!has_english && candidate_mac_en != -1)
        opentype_decode_namerecord(table, candidate_mac_en, *strings);

    if (!get_localizedstrings_count(*strings))
    {
        IDWriteLocalizedStrings_Release(*strings);
        *strings = NULL;
    }

    if (*strings)
        sort_localizedstrings(*strings);

    return *strings ? S_OK : E_FAIL;
}

static WCHAR *meta_get_lng_name(WCHAR *str, WCHAR **ctx)
{
    WCHAR *ret;

    if (!str) str = *ctx;
    while (*str && wcschr(L", ", *str)) str++;
    if (!*str) return NULL;
    ret = str++;
    while (*str && !wcschr(L", ", *str)) str++;
    if (*str) *str++ = 0;
    *ctx = str;

    return ret;
}

static HRESULT opentype_get_font_strings_from_meta(const struct file_stream_desc *stream_desc,
        DWRITE_INFORMATIONAL_STRING_ID id, IDWriteLocalizedStrings **ret)
{
    const struct meta_data_map *maps;
    IDWriteLocalizedStrings *strings;
    struct dwrite_fonttable meta;
    DWORD version, i, count, tag;
    HRESULT hr;

    *ret = NULL;

    switch (id)
    {
        case DWRITE_INFORMATIONAL_STRING_DESIGN_SCRIPT_LANGUAGE_TAG:
            tag = MS_DLNG_TAG;
            break;
        case DWRITE_INFORMATIONAL_STRING_SUPPORTED_SCRIPT_LANGUAGE_TAG:
            tag = MS_SLNG_TAG;
            break;
        default:
            WARN("Unexpected id %d.\n", id);
            return S_OK;
    }

    if (FAILED(hr = create_localizedstrings(&strings)))
        return hr;

    opentype_get_font_table(stream_desc, MS_META_TAG, &meta);

    if (meta.data)
    {
        version = table_read_be_dword(&meta, 0);
        if (version != 1)
        {
            WARN("Unexpected meta table version %ld.\n", version);
            goto end;
        }

        count = table_read_be_dword(&meta, FIELD_OFFSET(struct meta_header, data_maps_count));
        if (!(maps = table_read_ensure(&meta, FIELD_OFFSET(struct meta_header, maps),
                count * sizeof(struct meta_data_map))))
            goto end;

        for (i = 0; i < count; ++i)
        {
            const char *data;

            if (maps[i].tag == tag && maps[i].length)
            {
                DWORD length = GET_BE_DWORD(maps[i].length), j;

                if ((data = table_read_ensure(&meta, GET_BE_DWORD(maps[i].offset), length)))
                {
                    WCHAR *ptrW, *ctx, *token;

                    if (!(ptrW = malloc((length + 1) * sizeof(WCHAR))))
                    {
                        hr = E_OUTOFMEMORY;
                        goto end;
                    }

                    /* Data is stored in comma separated list, ASCII range only. */
                    for (j = 0; j < length; ++j)
                        ptrW[j] = data[j];
                    ptrW[length] = 0;

                    token = meta_get_lng_name(ptrW, &ctx);

                    while (token)
                    {
                        add_localizedstring(strings, L"", token);
                        token = meta_get_lng_name(NULL, &ctx);
                    }

                    free(ptrW);
                }
            }
        }
end:
        IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, meta.context);
    }

    if (IDWriteLocalizedStrings_GetCount(strings))
        *ret = strings;
    else
        IDWriteLocalizedStrings_Release(strings);

    return hr;
}

HRESULT opentype_get_font_info_strings(const struct file_stream_desc *stream_desc, DWRITE_INFORMATIONAL_STRING_ID id,
        IDWriteLocalizedStrings **strings)
{
    struct dwrite_fonttable name;

    switch (id)
    {
        case DWRITE_INFORMATIONAL_STRING_DESIGN_SCRIPT_LANGUAGE_TAG:
        case DWRITE_INFORMATIONAL_STRING_SUPPORTED_SCRIPT_LANGUAGE_TAG:
            opentype_get_font_strings_from_meta(stream_desc, id, strings);
            break;
        default:
            opentype_get_font_table(stream_desc, MS_NAME_TAG, &name);
            opentype_get_font_strings_from_id(&name, dwriteid_to_opentypeid[id], strings);
            if (name.context)
                IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, name.context);
    }

    return S_OK;
}

HRESULT opentype_get_font_familyname(const struct file_stream_desc *stream_desc, DWRITE_FONT_FAMILY_MODEL family_model,
        IDWriteLocalizedStrings **names)
{
    static const unsigned int wws_candidates[] =
    {
        OPENTYPE_STRING_WWS_FAMILY_NAME,
        OPENTYPE_STRING_TYPOGRAPHIC_FAMILY_NAME,
        OPENTYPE_STRING_FAMILY_NAME,
        ~0u,
    };
    static const unsigned int typographic_candidates[] =
    {
        OPENTYPE_STRING_TYPOGRAPHIC_FAMILY_NAME,
        OPENTYPE_STRING_WWS_FAMILY_NAME,
        OPENTYPE_STRING_FAMILY_NAME,
        ~0u,
    };
    struct dwrite_fonttable os2, name;
    const unsigned int *id;
    BOOL try_wws_name;
    HRESULT hr;

    opentype_get_font_table(stream_desc, MS_OS2_TAG, &os2);
    opentype_get_font_table(stream_desc, MS_NAME_TAG, &name);

    *names = NULL;

    if (family_model == DWRITE_FONT_FAMILY_MODEL_TYPOGRAPHIC)
    {
        id = typographic_candidates;
    }
    else
    {
        /* FamilyName locating order is WWS Family Name -> Preferred Family Name -> Family Name. If font claims to
           have 'Preferred Family Name' in WWS format, then WWS name is not used. */

        opentype_get_font_table(stream_desc, MS_OS2_TAG, &os2);
        /* If Preferred Family doesn't conform to WWS model try WWS name. */
        try_wws_name = os2.data && !(table_read_be_word(&os2, FIELD_OFFSET(struct tt_os2, fsSelection)) & OS2_FSSELECTION_WWS);
        if (os2.context)
            IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, os2.context);

        id = wws_candidates;
        if (!try_wws_name) id++;
    }

    while (*id != ~0u)
    {
        if (SUCCEEDED(hr = opentype_get_font_strings_from_id(&name, *id, names)))
            break;
        id++;
    }

    if (name.context)
        IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, name.context);

    return hr;
}

/* FaceName locating order is WWS Face Name -> Preferred Face Name -> Face Name. If font claims to
   have 'Preferred Face Name' in WWS format, then WWS name is not used. */
HRESULT opentype_get_font_facename(struct file_stream_desc *stream_desc, WCHAR *lfname, IDWriteLocalizedStrings **names)
{
    struct dwrite_fonttable os2, name;
    IDWriteLocalizedStrings *lfnames;
    UINT16 fsselection;
    HRESULT hr;

    opentype_get_font_table(stream_desc, MS_OS2_TAG, &os2);
    opentype_get_font_table(stream_desc, MS_NAME_TAG, &name);

    *names = NULL;

    /* if Preferred Family doesn't conform to WWS model try WWS name */
    fsselection = os2.data ? table_read_be_word(&os2, FIELD_OFFSET(struct tt_os2, fsSelection)) : 0;
    if (os2.data && !(fsselection & OS2_FSSELECTION_WWS))
        hr = opentype_get_font_strings_from_id(&name, OPENTYPE_STRING_WWS_SUBFAMILY_NAME, names);
    else
        hr = E_FAIL;

    if (FAILED(hr))
        hr = opentype_get_font_strings_from_id(&name, OPENTYPE_STRING_TYPOGRAPHIC_SUBFAMILY_NAME, names);
    if (FAILED(hr))
        hr = opentype_get_font_strings_from_id(&name, OPENTYPE_STRING_SUBFAMILY_NAME, names);

    /* User locale is preferred, with fallback to en-us. */
    *lfname = 0;
    if (SUCCEEDED(opentype_get_font_strings_from_id(&name, OPENTYPE_STRING_FAMILY_NAME, &lfnames)))
    {
        WCHAR localeW[LOCALE_NAME_MAX_LENGTH];
        UINT32 index;
        BOOL exists;

        exists = FALSE;
        if (GetSystemDefaultLocaleName(localeW, ARRAY_SIZE(localeW)))
            IDWriteLocalizedStrings_FindLocaleName(lfnames, localeW, &index, &exists);

        if (!exists)
            IDWriteLocalizedStrings_FindLocaleName(lfnames, L"en-us", &index, &exists);

        if (exists) {
            UINT32 length = 0;
            WCHAR *nameW;

            IDWriteLocalizedStrings_GetStringLength(lfnames, index, &length);
            nameW = malloc((length + 1) * sizeof(WCHAR));
            if (nameW)
            {
                *nameW = 0;
                IDWriteLocalizedStrings_GetString(lfnames, index, nameW, length + 1);
                lstrcpynW(lfname, nameW, LF_FACESIZE);
                free(nameW);
            }
        }

        IDWriteLocalizedStrings_Release(lfnames);
    }

    if (os2.context)
        IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, os2.context);
    if (name.context)
        IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, name.context);

    return hr;
}

static const struct ot_langsys *opentype_get_langsys(const struct ot_gsubgpos_table *table, unsigned int script_index,
        unsigned int language_index, unsigned int *feature_count)
{
    unsigned int table_offset, langsys_offset;
    const struct ot_langsys *langsys = NULL;

    *feature_count = 0;

    if (!table->table.data || script_index == ~0u)
        return NULL;

    /* ScriptTable offset. */
    table_offset = table_read_be_word(&table->table, table->script_list + FIELD_OFFSET(struct ot_script_list, scripts) +
            script_index * sizeof(struct ot_script_record) + FIELD_OFFSET(struct ot_script_record, script));
    if (!table_offset)
        return NULL;

    if (language_index == ~0u)
        langsys_offset = table_read_be_word(&table->table, table->script_list + table_offset);
    else
        langsys_offset = table_read_be_word(&table->table, table->script_list + table_offset +
                FIELD_OFFSET(struct ot_script, langsys) + language_index * sizeof(struct ot_langsys_record) +
                FIELD_OFFSET(struct ot_langsys_record, langsys));
    langsys_offset += table->script_list + table_offset;

    *feature_count = table_read_be_word(&table->table, langsys_offset + FIELD_OFFSET(struct ot_langsys, feature_count));
    if (*feature_count)
        langsys = table_read_ensure(&table->table, langsys_offset, FIELD_OFFSET(struct ot_langsys, feature_index[*feature_count]));
    if (!langsys)
        *feature_count = 0;

    return langsys;
}

void opentype_get_typographic_features(struct ot_gsubgpos_table *table, unsigned int script_index,
        unsigned int language_index, struct tag_array *t)
{
    unsigned int i, total_feature_count, script_feature_count;
    const struct ot_feature_list *feature_list;
    const struct ot_langsys *langsys = NULL;

    langsys = opentype_get_langsys(table, script_index, language_index, &script_feature_count);

    total_feature_count = table_read_be_word(&table->table, table->feature_list);
    if (!total_feature_count)
        return;

    feature_list = table_read_ensure(&table->table, table->feature_list,
            FIELD_OFFSET(struct ot_feature_list, features[total_feature_count]));
    if (!feature_list)
        return;

    for (i = 0; i < script_feature_count; ++i)
    {
        unsigned int feature_index = GET_BE_WORD(langsys->feature_index[i]);
        if (feature_index >= total_feature_count)
            continue;

        if (!dwrite_array_reserve((void **)&t->tags, &t->capacity, t->count + 1, sizeof(*t->tags)))
            return;

        t->tags[t->count++] = feature_list->features[feature_index].tag;
    }
}

static unsigned int find_vdmx_group(const struct vdmx_header *hdr)
{
    WORD num_ratios, i;
    const struct vdmx_ratio *ratios = (struct vdmx_ratio *)(hdr + 1);
    BYTE dev_x_ratio = 1, dev_y_ratio = 1;
    unsigned int group_offset = 0;

    num_ratios = GET_BE_WORD(hdr->num_ratios);

    for (i = 0; i < num_ratios; i++) {

        if (!ratios[i].bCharSet) continue;

        if ((ratios[i].xRatio == 0 && ratios[i].yStartRatio == 0 &&
             ratios[i].yEndRatio == 0) ||
	   (ratios[i].xRatio == dev_x_ratio && ratios[i].yStartRatio <= dev_y_ratio &&
            ratios[i].yEndRatio >= dev_y_ratio))
        {
            group_offset = GET_BE_WORD(*((WORD *)(ratios + num_ratios) + i));
            break;
        }
    }

    return group_offset;
}

BOOL opentype_get_vdmx_size(const struct dwrite_fonttable *vdmx, INT emsize, UINT16 *ascent, UINT16 *descent)
{
    unsigned int num_ratios, num_recs, group_offset, i;
    const struct vdmx_header *header;
    const struct vdmx_group *group;

    if (!vdmx->exists)
        return FALSE;

    num_ratios = table_read_be_word(vdmx, FIELD_OFFSET(struct vdmx_header, num_ratios));
    num_recs = table_read_be_word(vdmx, FIELD_OFFSET(struct vdmx_header, num_recs));

    header = table_read_ensure(vdmx, 0, sizeof(*header) + num_ratios * sizeof(struct vdmx_ratio) +
            num_recs * sizeof(*group));

    if (!header)
        return FALSE;

    group_offset = find_vdmx_group(header);
    if (!group_offset)
        return FALSE;

    num_recs = table_read_be_word(vdmx, group_offset);
    group = table_read_ensure(vdmx, group_offset, FIELD_OFFSET(struct vdmx_group, entries[num_recs]));

    if (!group)
        return FALSE;

    if (emsize < group->startsz || emsize >= group->endsz)
        return FALSE;

    for (i = 0; i < num_recs; ++i)
    {
        WORD ppem = GET_BE_WORD(group->entries[i].yPelHeight);
        if (ppem > emsize) {
            FIXME("interpolate %d\n", emsize);
            return FALSE;
        }

        if (ppem == emsize) {
            *ascent = (SHORT)GET_BE_WORD(group->entries[i].yMax);
            *descent = -(SHORT)GET_BE_WORD(group->entries[i].yMin);
            return TRUE;
        }
    }

    return FALSE;
}

unsigned int opentype_get_gasp_flags(const struct dwrite_fonttable *gasp, float emsize)
{
    unsigned int version, num_ranges, i;
    const struct gasp_header *table;
    WORD flags = 0;

    if (!gasp->exists)
        return 0;

    num_ranges = table_read_be_word(gasp, FIELD_OFFSET(struct gasp_header, num_ranges));

    table = table_read_ensure(gasp, 0, FIELD_OFFSET(struct gasp_header, ranges[num_ranges]));
    if (!table)
        return 0;

    version = GET_BE_WORD(table->version);
    if (version > 1)
    {
        ERR("Unsupported gasp table format version %u.\n", version);
        goto done;
    }

    for (i = 0; i < num_ranges; ++i)
    {
        flags = GET_BE_WORD(table->ranges[i].flags);
        if (emsize <= GET_BE_WORD(table->ranges[i].max_ppem)) break;
    }

done:
    return flags;
}

unsigned int opentype_get_cpal_palettecount(const struct dwrite_fonttable *cpal)
{
    return table_read_be_word(cpal, FIELD_OFFSET(struct cpal_header_0, num_palettes));
}

unsigned int opentype_get_cpal_paletteentrycount(const struct dwrite_fonttable *cpal)
{
    return table_read_be_word(cpal, FIELD_OFFSET(struct cpal_header_0, num_palette_entries));
}

HRESULT opentype_get_cpal_entries(const struct dwrite_fonttable *cpal, unsigned int palette,
        unsigned int first_entry_index, unsigned int entry_count, DWRITE_COLOR_F *entries)
{
    unsigned int num_palettes, num_palette_entries, i;
    const struct cpal_color_record *records;
    const struct cpal_header_0 *header;
    struct d3d_color
    {
        float r;
        float g;
        float b;
        float a;
    } *colors = (void *)entries;

    header = table_read_ensure(cpal, 0, sizeof(*header));

    if (!cpal->exists || !header)
        return DWRITE_E_NOCOLOR;

    num_palettes = GET_BE_WORD(header->num_palettes);
    if (palette >= num_palettes)
        return DWRITE_E_NOCOLOR;

    header = table_read_ensure(cpal, 0, FIELD_OFFSET(struct cpal_header_0, color_record_indices[palette]));
    if (!header)
        return DWRITE_E_NOCOLOR;

    num_palette_entries = GET_BE_WORD(header->num_palette_entries);
    if (first_entry_index + entry_count > num_palette_entries)
        return E_INVALIDARG;

    records = table_read_ensure(cpal, GET_BE_DWORD(header->offset_first_color_record),
            sizeof(*records) * GET_BE_WORD(header->num_color_records));
    if (!records)
        return DWRITE_E_NOCOLOR;

    first_entry_index += GET_BE_WORD(header->color_record_indices[palette]);

    for (i = 0; i < entry_count; ++i)
    {
        colors[i].r = records[first_entry_index + i].red   / 255.0f;
        colors[i].g = records[first_entry_index + i].green / 255.0f;
        colors[i].b = records[first_entry_index + i].blue  / 255.0f;
        colors[i].a = records[first_entry_index + i].alpha / 255.0f;
    }

    return S_OK;
}

static int __cdecl colr_compare_gid(const void *g, const void *r)
{
    const struct colr_baseglyph_record *record = r;
    UINT16 glyph = *(UINT16*)g, GID = GET_BE_WORD(record->glyph);
    int ret = 0;

    if (glyph > GID)
        ret = 1;
    else if (glyph < GID)
        ret = -1;

    return ret;
}

HRESULT opentype_get_colr_glyph(const struct dwrite_fonttable *colr, UINT16 glyph, struct dwrite_colorglyph *ret)
{
    unsigned int num_baseglyph_records, offset_baseglyph_records;
    const struct colr_baseglyph_record *record;
    const struct colr_layer_record *layer;
    const struct colr_header *header;

    memset(ret, 0, sizeof(*ret));
    ret->glyph = glyph;
    ret->palette_index = 0xffff;

    header = table_read_ensure(colr, 0, sizeof(*header));
    if (!header)
        return S_FALSE;

    num_baseglyph_records = GET_BE_WORD(header->num_baseglyph_records);
    offset_baseglyph_records = GET_BE_DWORD(header->offset_baseglyph_records);
    if (!table_read_ensure(colr, offset_baseglyph_records, num_baseglyph_records * sizeof(*record)))
    {
        return S_FALSE;
    }

    record = bsearch(&glyph, colr->data + offset_baseglyph_records, num_baseglyph_records,
            sizeof(*record), colr_compare_gid);
    if (!record)
        return S_FALSE;

    ret->first_layer = GET_BE_WORD(record->first_layer_index);
    ret->num_layers = GET_BE_WORD(record->num_layers);

    if ((layer = table_read_ensure(colr, GET_BE_DWORD(header->offset_layer_records),
            (ret->first_layer + ret->layer) * sizeof(*layer))))
    {
        layer += ret->first_layer + ret->layer;
        ret->glyph = GET_BE_WORD(layer->glyph);
        ret->palette_index = GET_BE_WORD(layer->palette_index);
    }

    return S_OK;
}

void opentype_colr_next_glyph(const struct dwrite_fonttable *colr, struct dwrite_colorglyph *glyph)
{
    const struct colr_layer_record *layer;
    const struct colr_header *header;

    /* iterated all the way through */
    if (glyph->layer == glyph->num_layers)
        return;

    if (!(header = table_read_ensure(colr, 0, sizeof(*header))))
        return;

    glyph->layer++;

    if ((layer = table_read_ensure(colr, GET_BE_DWORD(header->offset_layer_records),
            (glyph->first_layer + glyph->layer) * sizeof(*layer))))
    {
        layer += glyph->first_layer + glyph->layer;
        glyph->glyph = GET_BE_WORD(layer->glyph);
        glyph->palette_index = GET_BE_WORD(layer->palette_index);
    }
}

static BOOL opentype_has_font_table(IDWriteFontFace5 *fontface, UINT32 tag)
{
    BOOL exists = FALSE;
    const void *data;
    void *context;
    UINT32 size;
    HRESULT hr;

    hr = IDWriteFontFace5_TryGetFontTable(fontface, tag, &data, &size, &context, &exists);
    if (FAILED(hr))
        return FALSE;

    if (exists)
        IDWriteFontFace5_ReleaseFontTable(fontface, context);

    return exists;
}

static unsigned int opentype_get_sbix_formats(IDWriteFontFace5 *fontface)
{
    unsigned int num_strikes, num_glyphs, i, j, ret = 0;
    const struct sbix_header *sbix_header;
    struct dwrite_fonttable table;

    memset(&table, 0, sizeof(table));
    table.exists = TRUE;

    if (!get_fontface_table(fontface, MS_MAXP_TAG, &table))
        return 0;

    num_glyphs = table_read_be_word(&table, FIELD_OFFSET(struct maxp, num_glyphs));

    IDWriteFontFace5_ReleaseFontTable(fontface, table.context);

    memset(&table, 0, sizeof(table));
    table.exists = TRUE;

    if (!get_fontface_table(fontface, MS_SBIX_TAG, &table))
        return 0;

    num_strikes = table_read_be_dword(&table, FIELD_OFFSET(struct sbix_header, num_strikes));
    sbix_header = table_read_ensure(&table, 0, FIELD_OFFSET(struct sbix_header, strike_offset[num_strikes]));

    if (sbix_header)
    {
        for (i = 0; i < num_strikes; ++i)
        {
            unsigned int strike_offset = GET_BE_DWORD(sbix_header->strike_offset[i]);
            const struct sbix_strike *strike = table_read_ensure(&table, strike_offset,
                    FIELD_OFFSET(struct sbix_strike, glyphdata_offsets[num_glyphs + 1]));

            if (!strike)
                continue;

            for (j = 0; j < num_glyphs; j++)
            {
                unsigned int offset = GET_BE_DWORD(strike->glyphdata_offsets[j]);
                unsigned int next_offset = GET_BE_DWORD(strike->glyphdata_offsets[j + 1]);
                const struct sbix_glyph_data *glyph_data;

                if (offset == next_offset)
                    continue;

                glyph_data = table_read_ensure(&table, strike_offset + offset, sizeof(*glyph_data));
                if (!glyph_data)
                    continue;

                switch (glyph_data->graphic_type)
                {
                    case MS_PNG__TAG:
                        ret |= DWRITE_GLYPH_IMAGE_FORMATS_PNG;
                        break;
                    case MS_JPG__TAG:
                        ret |= DWRITE_GLYPH_IMAGE_FORMATS_JPEG;
                        break;
                    case MS_TIFF_TAG:
                        ret |= DWRITE_GLYPH_IMAGE_FORMATS_TIFF;
                        break;
                    default:
                        FIXME("unexpected bitmap format %s\n", debugstr_fourcc(GET_BE_DWORD(glyph_data->graphic_type)));
                }
            }
        }
    }

    IDWriteFontFace5_ReleaseFontTable(fontface, table.context);

    return ret;
}

static unsigned int opentype_get_cblc_formats(IDWriteFontFace5 *fontface)
{
    const unsigned int format_mask = DWRITE_GLYPH_IMAGE_FORMATS_PNG |
            DWRITE_GLYPH_IMAGE_FORMATS_PREMULTIPLIED_B8G8R8A8;
    const struct cblc_bitmapsize_table *sizes;
    struct dwrite_fonttable cblc = { 0 };
    unsigned int num_sizes, i, ret = 0;
    const struct cblc_header *header;

    cblc.exists = TRUE;
    if (!get_fontface_table(fontface, MS_CBLC_TAG, &cblc))
        return 0;

    num_sizes = table_read_be_dword(&cblc, FIELD_OFFSET(struct cblc_header, num_sizes));
    sizes = table_read_ensure(&cblc, sizeof(*header), num_sizes * sizeof(*sizes));

    if (sizes)
    {
        for (i = 0; i < num_sizes; ++i)
        {
            BYTE bpp = sizes[i].bit_depth;

            if ((ret & format_mask) == format_mask)
                break;

            if (bpp == 1 || bpp == 2 || bpp == 4 || bpp == 8)
                ret |= DWRITE_GLYPH_IMAGE_FORMATS_PNG;
            else if (bpp == 32)
                ret |= DWRITE_GLYPH_IMAGE_FORMATS_PREMULTIPLIED_B8G8R8A8;
        }
    }

    IDWriteFontFace5_ReleaseFontTable(fontface, cblc.context);

    return ret;
}

UINT32 opentype_get_glyph_image_formats(IDWriteFontFace5 *fontface)
{
    UINT32 ret = DWRITE_GLYPH_IMAGE_FORMATS_NONE;

    if (opentype_has_font_table(fontface, MS_GLYF_TAG))
        ret |= DWRITE_GLYPH_IMAGE_FORMATS_TRUETYPE;

    if (opentype_has_font_table(fontface, MS_CFF__TAG) ||
            opentype_has_font_table(fontface, MS_CFF2_TAG))
        ret |= DWRITE_GLYPH_IMAGE_FORMATS_CFF;

    if (opentype_has_font_table(fontface, MS_COLR_TAG))
        ret |= DWRITE_GLYPH_IMAGE_FORMATS_COLR;

    if (opentype_has_font_table(fontface, MS_SVG__TAG))
        ret |= DWRITE_GLYPH_IMAGE_FORMATS_SVG;

    if (opentype_has_font_table(fontface, MS_SBIX_TAG))
        ret |= opentype_get_sbix_formats(fontface);

    if (opentype_has_font_table(fontface, MS_CBLC_TAG))
        ret |= opentype_get_cblc_formats(fontface);

    return ret;
}

DWRITE_CONTAINER_TYPE opentype_analyze_container_type(void const *data, UINT32 data_size)
{
    DWORD signature;

    if (data_size < sizeof(DWORD))
        return DWRITE_CONTAINER_TYPE_UNKNOWN;

    /* Both WOFF and WOFF2 start with 4 bytes signature. */
    signature = *(DWORD *)data;

    switch (signature)
    {
    case MS_WOFF_TAG:
        return DWRITE_CONTAINER_TYPE_WOFF;
    case MS_WOF2_TAG:
        return DWRITE_CONTAINER_TYPE_WOFF2;
    default:
        return DWRITE_CONTAINER_TYPE_UNKNOWN;
    }
}

void opentype_layout_scriptshaping_cache_init(struct scriptshaping_cache *cache)
{
    cache->font->grab_font_table(cache->context, MS_GSUB_TAG, &cache->gsub.table.data, &cache->gsub.table.size,
            &cache->gsub.table.context);

    if (cache->gsub.table.data)
    {
        cache->gsub.script_list = table_read_be_word(&cache->gsub.table, FIELD_OFFSET(struct gpos_gsub_header, script_list));
        cache->gsub.feature_list = table_read_be_word(&cache->gsub.table, FIELD_OFFSET(struct gpos_gsub_header, feature_list));
        cache->gsub.lookup_list = table_read_be_word(&cache->gsub.table, FIELD_OFFSET(struct gpos_gsub_header, lookup_list));
    }

    cache->font->grab_font_table(cache->context, MS_GPOS_TAG, &cache->gpos.table.data, &cache->gpos.table.size,
            &cache->gpos.table.context);

    if (cache->gpos.table.data)
    {
        cache->gpos.script_list = table_read_be_word(&cache->gpos.table,
                FIELD_OFFSET(struct gpos_gsub_header, script_list));
        cache->gpos.feature_list = table_read_be_word(&cache->gpos.table,
                FIELD_OFFSET(struct gpos_gsub_header, feature_list));
        cache->gpos.lookup_list = table_read_be_word(&cache->gpos.table,
                FIELD_OFFSET(struct gpos_gsub_header, lookup_list));
    }

    cache->font->grab_font_table(cache->context, MS_GDEF_TAG, &cache->gdef.table.data, &cache->gdef.table.size,
            &cache->gdef.table.context);

    if (cache->gdef.table.data)
    {
        unsigned int version = table_read_be_dword(&cache->gdef.table, 0);

        cache->gdef.classdef = table_read_be_word(&cache->gdef.table, FIELD_OFFSET(struct gdef_header, classdef));
        cache->gdef.markattachclassdef = table_read_be_word(&cache->gdef.table,
                FIELD_OFFSET(struct gdef_header, markattach_classdef));
        if (version >= 0x00010002)
            cache->gdef.markglyphsetdef = table_read_be_word(&cache->gdef.table,
                    FIELD_OFFSET(struct gdef_header, markglyphsetdef));
    }
}

unsigned int opentype_layout_find_script(const struct scriptshaping_cache *cache, unsigned int kind, DWORD script,
        unsigned int *script_index)
{
    const struct ot_gsubgpos_table *table = kind == MS_GSUB_TAG ? &cache->gsub : &cache->gpos;
    UINT16 script_count;
    unsigned int i;

    *script_index = ~0u;

    script_count = table_read_be_word(&table->table, table->script_list);
    if (!script_count)
        return 0;

    for (i = 0; i < script_count; i++)
    {
        unsigned int tag = table_read_dword(&table->table, table->script_list + FIELD_OFFSET(struct ot_script_list, scripts) +
                i * sizeof(struct ot_script_record));
        if (!tag)
            continue;

        if (tag == script)
        {
            *script_index = i;
            return script;
        }
    }

    return 0;
}

unsigned int opentype_layout_find_language(const struct scriptshaping_cache *cache, unsigned int kind, DWORD language,
        unsigned int script_index, unsigned int *language_index)
{
    const struct ot_gsubgpos_table *table = kind == MS_GSUB_TAG ? &cache->gsub : &cache->gpos;
    UINT16 table_offset, lang_count;
    unsigned int i;

    *language_index = ~0u;

    table_offset = table_read_be_word(&table->table, table->script_list + FIELD_OFFSET(struct ot_script_list, scripts) +
            script_index * sizeof(struct ot_script_record) + FIELD_OFFSET(struct ot_script_record, script));
    if (!table_offset)
        return 0;

    lang_count = table_read_be_word(&table->table, table->script_list + table_offset +
            FIELD_OFFSET(struct ot_script, langsys_count));
    for (i = 0; i < lang_count; i++)
    {
        unsigned int tag = table_read_dword(&table->table, table->script_list + table_offset +
                FIELD_OFFSET(struct ot_script, langsys) + i * sizeof(struct ot_langsys_record));

        if (tag == language)
        {
            *language_index = i;
            return language;
        }
    }

    /* Try 'defaultLangSys' if it's set. */
    if (table_read_be_word(&table->table, table->script_list + table_offset))
        return ~0u;

    return 0;
}

static int __cdecl gdef_class_compare_format2(const void *g, const void *r)
{
    const struct ot_gdef_class_range *range = r;
    UINT16 glyph = *(UINT16 *)g;

    if (glyph < GET_BE_WORD(range->start_glyph))
        return -1;
    else if (glyph > GET_BE_WORD(range->end_glyph))
        return 1;
    else
        return 0;
}

static unsigned int opentype_layout_get_glyph_class(const struct dwrite_fonttable *table,
        unsigned int offset, UINT16 glyph)
{
    WORD format = table_read_be_word(table, offset), count;
    unsigned int glyph_class = GDEF_CLASS_UNCLASSIFIED;

    if (format == 1)
    {
        const struct ot_gdef_classdef_format1 *format1;

        count = table_read_be_word(table, offset + FIELD_OFFSET(struct ot_gdef_classdef_format1, glyph_count));
        format1 = table_read_ensure(table, offset, FIELD_OFFSET(struct ot_gdef_classdef_format1, classes[count]));
        if (format1)
        {
            WORD start_glyph = GET_BE_WORD(format1->start_glyph);
            if (glyph >= start_glyph && (glyph - start_glyph) < count)
            {
                glyph_class = GET_BE_WORD(format1->classes[glyph - start_glyph]);
                if (glyph_class > GDEF_CLASS_MAX)
                     glyph_class = GDEF_CLASS_UNCLASSIFIED;
            }
        }
    }
    else if (format == 2)
    {
        const struct ot_gdef_classdef_format2 *format2;

        count = table_read_be_word(table, offset + FIELD_OFFSET(struct ot_gdef_classdef_format2, range_count));
        format2 = table_read_ensure(table, offset, FIELD_OFFSET(struct ot_gdef_classdef_format2, ranges[count]));
        if (format2)
        {
            const struct ot_gdef_class_range *range = bsearch(&glyph, format2->ranges, count,
                    sizeof(struct ot_gdef_class_range), gdef_class_compare_format2);
            glyph_class = range && glyph <= GET_BE_WORD(range->end_glyph) ?
                    GET_BE_WORD(range->glyph_class) : GDEF_CLASS_UNCLASSIFIED;
            if (glyph_class > GDEF_CLASS_MAX)
                 glyph_class = GDEF_CLASS_UNCLASSIFIED;
        }
    }
    else
        WARN("Unknown GDEF format %u.\n", format);

    return glyph_class;
}

static unsigned int opentype_set_glyph_props(struct scriptshaping_context *context, unsigned int idx)
{
    struct scriptshaping_cache *cache = context->cache;
    unsigned int glyph_class = 0, props;

    if (cache->gdef.classdef)
    {
        glyph_class = opentype_layout_get_glyph_class(&cache->gdef.table, cache->gdef.classdef,
                context->u.buffer.glyphs[idx]);
    }

    switch (glyph_class)
    {
        case GDEF_CLASS_BASE:
            props = GLYPH_PROP_BASE;
            break;
        case GDEF_CLASS_LIGATURE:
            props = GLYPH_PROP_LIGATURE;
            break;
        case GDEF_CLASS_MARK:
            props = GLYPH_PROP_MARK;
            if (cache->gdef.markattachclassdef)
            {
                glyph_class = opentype_layout_get_glyph_class(&cache->gdef.table, cache->gdef.markattachclassdef,
                        context->u.buffer.glyphs[idx]);
                props |= glyph_class << 8;
            }
            break;
        default:
            props = 0;
    }

    context->glyph_infos[idx].props = props;

    return props;
}

static void opentype_set_subst_glyph_props(struct scriptshaping_context *context, unsigned int idx)
{
    unsigned int glyph_props = opentype_set_glyph_props(context, idx) & LOOKUP_FLAG_IGNORE_MASK;
    context->u.subst.glyph_props[idx].isDiacritic = !!(glyph_props == GLYPH_PROP_MARK);
    context->u.subst.glyph_props[idx].isZeroWidthSpace = !!(glyph_props == GLYPH_PROP_MARK);
}

struct coverage_compare_format1_context
{
    UINT16 glyph;
    const UINT16 *table_base;
    unsigned int *coverage_index;
};

static int __cdecl coverage_compare_format1(const void *left, const void *right)
{
    const struct coverage_compare_format1_context *context = left;
    UINT16 glyph = GET_BE_WORD(*(UINT16 *)right);
    int ret;

    ret = context->glyph - glyph;
    if (!ret)
        *context->coverage_index = (UINT16 *)right - context->table_base;

    return ret;
}

static int __cdecl coverage_compare_format2(const void *g, const void *r)
{
    const struct ot_coverage_range *range = r;
    UINT16 glyph = *(UINT16 *)g;

    if (glyph < GET_BE_WORD(range->start_glyph))
        return -1;
    else if (glyph > GET_BE_WORD(range->end_glyph))
        return 1;
    else
        return 0;
}

static unsigned int opentype_layout_is_glyph_covered(const struct dwrite_fonttable *table, unsigned int coverage,
        UINT16 glyph)
{
    WORD format = table_read_be_word(table, coverage), count;

    count = table_read_be_word(table, coverage + 2);

    if (format == 1)
    {
        const struct ot_coverage_format1 *format1 = table_read_ensure(table, coverage,
                FIELD_OFFSET(struct ot_coverage_format1, glyphs[count]));
        struct coverage_compare_format1_context context;
        unsigned int coverage_index = GLYPH_NOT_COVERED;

        if (format1)
        {
            context.glyph = glyph;
            context.table_base = format1->glyphs;
            context.coverage_index = &coverage_index;

            bsearch(&context, format1->glyphs, count, sizeof(glyph), coverage_compare_format1);
        }

        return coverage_index;
    }
    else if (format == 2)
    {
        const struct ot_coverage_format2 *format2 = table_read_ensure(table, coverage,
                FIELD_OFFSET(struct ot_coverage_format2, ranges[count]));
        if (format2)
        {
            const struct ot_coverage_range *range = bsearch(&glyph, format2->ranges, count,
                    sizeof(struct ot_coverage_range), coverage_compare_format2);
            return range && glyph <= GET_BE_WORD(range->end_glyph) ?
                    GET_BE_WORD(range->startcoverage_index) + glyph - GET_BE_WORD(range->start_glyph) :
                    GLYPH_NOT_COVERED;
        }
    }
    else
        WARN("Unknown coverage format %u.\n", format);

    return -1;
}

static inline unsigned int dwrite_popcount(unsigned int x)
{
#if defined(__MINGW32__)
    return __builtin_popcount(x);
#else
    x -= x >> 1 & 0x55555555;
    x = (x & 0x33333333) + (x >> 2 & 0x33333333);
    return ((x + (x >> 4)) & 0x0f0f0f0f) * 0x01010101 >> 24;
#endif
}

static float opentype_scale_gpos_be_value(WORD value, float emsize, UINT16 upem)
{
    return (short)GET_BE_WORD(value) * emsize / upem;
}

static int opentype_layout_gpos_get_dev_value(const struct scriptshaping_context *context, unsigned int offset)
{
    const struct dwrite_fonttable *table = &context->table->table;
    unsigned int start_size, end_size, format, value_word;
    unsigned int index, ppem, mask;
    int value;

    if (!offset)
        return 0;

    start_size = table_read_be_word(table, offset);
    end_size = table_read_be_word(table, offset + FIELD_OFFSET(struct ot_gpos_device_table, end_size));

    ppem = context->emsize;
    if (ppem < start_size || ppem > end_size)
        return 0;

    format = table_read_be_word(table, offset + FIELD_OFFSET(struct ot_gpos_device_table, format));

    if (format < 1 || format > 3)
        return 0;

    index = ppem - start_size;

    value_word = table_read_be_word(table, offset + FIELD_OFFSET(struct ot_gpos_device_table, values[index >> (4 - format)]));
    mask = 0xffff >> (16 - (1 << format));

    value = (value_word >> ((index % (4 - format)) * (1 << format))) & mask;

    if ((unsigned int)value >= ((mask + 1) >> 1))
        value -= mask + 1;

    return value;
}

static void opentype_layout_apply_gpos_value(struct scriptshaping_context *context, unsigned int table_offset,
        WORD value_format, const WORD *values, unsigned int glyph)
{
    const struct scriptshaping_cache *cache = context->cache;
    DWRITE_GLYPH_OFFSET *offset = &context->offsets[glyph];
    float *advance = &context->advances[glyph];

    if (!value_format)
        return;

    if (value_format & GPOS_VALUE_X_PLACEMENT)
    {
        offset->advanceOffset += opentype_scale_gpos_be_value(*values, context->emsize, cache->upem);
        values++;
    }
    if (value_format & GPOS_VALUE_Y_PLACEMENT)
    {
        offset->ascenderOffset += opentype_scale_gpos_be_value(*values, context->emsize, cache->upem);
        values++;
    }
    if (value_format & GPOS_VALUE_X_ADVANCE)
    {
        *advance += opentype_scale_gpos_be_value(*values, context->emsize, cache->upem);
        values++;
    }
    if (value_format & GPOS_VALUE_Y_ADVANCE)
    {
        values++;
    }
    if (value_format & GPOS_VALUE_X_PLACEMENT_DEVICE)
    {
        offset->advanceOffset += opentype_layout_gpos_get_dev_value(context, table_offset + GET_BE_WORD(*values));
        values++;
    }
    if (value_format & GPOS_VALUE_Y_PLACEMENT_DEVICE)
    {
        offset->ascenderOffset += opentype_layout_gpos_get_dev_value(context, table_offset + GET_BE_WORD(*values));
        values++;
    }
    if (value_format & GPOS_VALUE_X_ADVANCE_DEVICE)
    {
        *advance += opentype_layout_gpos_get_dev_value(context, table_offset + GET_BE_WORD(*values));
        values++;
    }
    if (value_format & GPOS_VALUE_Y_ADVANCE_DEVICE)
    {
        values++;
    }
}

struct lookup
{
    unsigned short index;
    unsigned short type;
    unsigned short subtable_count;

    unsigned int mask;
    unsigned int flags;
    unsigned int offset;
    unsigned int auto_zwnj : 1;
    unsigned int auto_zwj : 1;
};

static unsigned int opentype_layout_is_subst_context(const struct scriptshaping_context *context)
{
    return context->table == &context->cache->gsub;
}

static unsigned int opentype_layout_is_pos_context(const struct scriptshaping_context *context)
{
    return context->table == &context->cache->gpos;
}

static unsigned int opentype_layout_get_gsubgpos_subtable(const struct scriptshaping_context *context,
        const struct lookup *lookup, unsigned int subtable, unsigned int *lookup_type)
{
    unsigned int subtable_offset = table_read_be_word(&context->table->table, lookup->offset +
            FIELD_OFFSET(struct ot_lookup_table, subtable[subtable]));
    const struct ot_gsubgpos_extension_format1 *format1;

    subtable_offset += lookup->offset;

    if ((opentype_layout_is_subst_context(context) && lookup->type != GSUB_LOOKUP_EXTENSION_SUBST) ||
            (opentype_layout_is_pos_context(context) && lookup->type != GPOS_LOOKUP_EXTENSION_POSITION))
    {
        *lookup_type = lookup->type;
        return subtable_offset;
    }

    *lookup_type = 0;

    if (!(format1 = table_read_ensure(&context->table->table, subtable_offset, sizeof(*format1))))
        return 0;

    if (GET_BE_WORD(format1->format) != 1)
    {
        WARN("Unexpected extension table format %#x.\n", format1->format);
        return 0;
    }

    *lookup_type = GET_BE_WORD(format1->lookup_type);
    return subtable_offset + GET_BE_DWORD(format1->extension_offset);
}

struct ot_lookup
{
    unsigned int offset;
    unsigned int subtable_count;
    unsigned int flags;
};

enum iterator_match
{
    /* First two to fit matching callback result. */
    ITER_NO = 0,
    ITER_YES = 1,
    ITER_MAYBE,
};

struct match_context;
struct match_data
{
    const struct match_context *mc;
    unsigned int subtable_offset;
};

typedef BOOL (*p_match_func)(UINT16 glyph, UINT16 glyph_data, const struct match_data *match_data);

struct match_context
{
    struct scriptshaping_context *context;
    unsigned int backtrack_offset;
    unsigned int input_offset;
    unsigned int lookahead_offset;
    p_match_func match_func;
    const struct lookup *lookup;
};

struct glyph_iterator
{
    struct scriptshaping_context *context;
    unsigned int flags;
    unsigned int pos;
    unsigned int len;
    unsigned int mask;
    p_match_func match_func;
    const UINT16 *glyph_data;
    const struct match_data *match_data;
    unsigned int ignore_zwnj;
    unsigned int ignore_zwj;
};

static void glyph_iterator_init(struct scriptshaping_context *context, unsigned int flags, unsigned int pos,
        unsigned int len, struct glyph_iterator *iter)
{
    iter->context = context;
    iter->flags = flags;
    iter->pos = pos;
    iter->len = len;
    iter->mask = ~0u;
    iter->match_func = NULL;
    iter->match_data = NULL;
    iter->glyph_data = NULL;
    /* Context matching iterators will get these fixed up. */
    iter->ignore_zwnj = !!opentype_layout_is_pos_context(context);
    iter->ignore_zwj = context->auto_zwj;
}

struct ot_gdef_mark_glyph_sets
{
    UINT16 format;
    UINT16 count;
    DWORD offsets[1];
};

static BOOL opentype_match_glyph_func(UINT16 glyph, UINT16 glyph_data, const struct match_data *data)
{
    return glyph == glyph_data;
}

static BOOL opentype_match_class_func(UINT16 glyph, UINT16 glyph_data, const struct match_data *data)
{
    const struct match_context *mc = data->mc;
    UINT16 glyph_class = opentype_layout_get_glyph_class(&mc->context->table->table, data->subtable_offset, glyph);
    return glyph_class == glyph_data;
}

static BOOL opentype_match_coverage_func(UINT16 glyph, UINT16 glyph_data, const struct match_data *data)
{
    const struct match_context *mc = data->mc;
    return opentype_layout_is_glyph_covered(&mc->context->table->table, data->subtable_offset + glyph_data, glyph)
            != GLYPH_NOT_COVERED;
}

static BOOL opentype_layout_mark_set_covers(const struct scriptshaping_cache *cache, unsigned int set_index,
        UINT16 glyph)
{
    unsigned int format, offset = cache->gdef.markglyphsetdef, coverage_offset, count;

    if (!offset)
        return FALSE;

    format = table_read_be_word(&cache->gdef.table, offset);

    if (format == 1)
    {
        count = table_read_be_word(&cache->gdef.table, offset + FIELD_OFFSET(struct ot_gdef_markglyphsets, count));
        if (!count || set_index >= count)
            return FALSE;

        coverage_offset = table_read_be_dword(&cache->gdef.table, offset +
                FIELD_OFFSET(struct ot_gdef_markglyphsets, offsets[set_index]));
        return opentype_layout_is_glyph_covered(&cache->gdef.table, offset + coverage_offset, glyph) != GLYPH_NOT_COVERED;
    }
    else
        WARN("Unexpected MarkGlyphSets format %#x.\n", format);

    return FALSE;
}

static BOOL lookup_is_glyph_match(const struct scriptshaping_context *context, unsigned int idx, unsigned int match_props)
{
    unsigned int glyph_props = context->glyph_infos[idx].props;
    UINT16 glyph = context->u.buffer.glyphs[idx];

    if (glyph_props & match_props & LOOKUP_FLAG_IGNORE_MASK)
        return FALSE;

    if (!(glyph_props & GLYPH_PROP_MARK))
        return TRUE;

    if (match_props & LOOKUP_FLAG_USE_MARK_FILTERING_SET)
        return opentype_layout_mark_set_covers(context->cache, match_props >> 16, glyph);

    if (match_props & LOOKUP_FLAG_MARK_ATTACHMENT_TYPE)
        return (match_props & LOOKUP_FLAG_MARK_ATTACHMENT_TYPE) == (glyph_props & LOOKUP_FLAG_MARK_ATTACHMENT_TYPE);

    return TRUE;
}

static enum iterator_match glyph_iterator_may_skip(const struct glyph_iterator *iter)
{
    unsigned int glyph_props = iter->context->glyph_infos[iter->pos].props & (GLYPH_PROP_IGNORABLE | GLYPH_PROP_HIDDEN);

    if (!lookup_is_glyph_match(iter->context, iter->pos, iter->flags))
        return ITER_YES;

    if (glyph_props == GLYPH_PROP_IGNORABLE && !iter->context->u.buffer.glyph_props[iter->pos].components &&
            (iter->ignore_zwnj || !(iter->context->glyph_infos[iter->pos].props & GLYPH_PROP_ZWNJ)) &&
            (iter->ignore_zwj || !(iter->context->glyph_infos[iter->pos].props & GLYPH_PROP_ZWJ)))
    {
        return ITER_MAYBE;
    }

    return ITER_NO;
}

static enum iterator_match glyph_iterator_may_match(const struct glyph_iterator *iter)
{
    if (!(iter->mask & iter->context->glyph_infos[iter->pos].mask))
        return ITER_NO;

    /* Glyph data is used for input, backtrack, and lookahead arrays, swap it here instead of doing that
       in all matching functions. */
    if (iter->match_func)
        return !!iter->match_func(iter->context->u.buffer.glyphs[iter->pos], GET_BE_WORD(*iter->glyph_data), iter->match_data);

    return ITER_MAYBE;
}

static BOOL glyph_iterator_next(struct glyph_iterator *iter)
{
    enum iterator_match skip, match;

    while (iter->pos + iter->len < iter->context->glyph_count)
    {
        ++iter->pos;

        skip = glyph_iterator_may_skip(iter);
        if (skip == ITER_YES)
            continue;

        match = glyph_iterator_may_match(iter);
        if (match == ITER_YES || (match == ITER_MAYBE && skip == ITER_NO))
        {
            --iter->len;
            if (iter->glyph_data)
                ++iter->glyph_data;
            return TRUE;
        }

        if (skip == ITER_NO)
            return FALSE;
    }

    return FALSE;
}

static BOOL glyph_iterator_prev(struct glyph_iterator *iter)
{
    enum iterator_match skip, match;

    while (iter->pos > iter->len - 1)
    {
        --iter->pos;

        skip = glyph_iterator_may_skip(iter);
        if (skip == ITER_YES)
            continue;

        match = glyph_iterator_may_match(iter);
        if (match == ITER_YES || (match == ITER_MAYBE && skip == ITER_NO))
        {
            --iter->len;
            if (iter->glyph_data)
                ++iter->glyph_data;
            return TRUE;
        }

        if (skip == ITER_NO)
            return FALSE;
    }

    return FALSE;
}

static BOOL opentype_layout_apply_gpos_single_adjustment(struct scriptshaping_context *context,
        const struct lookup *lookup, unsigned int subtable_offset)
{
    const struct dwrite_fonttable *table = &context->table->table;
    UINT16 format, value_format, value_len, coverage, glyph;

    unsigned int coverage_index;

    format = table_read_be_word(table, subtable_offset);

    coverage = table_read_be_word(table, subtable_offset + FIELD_OFFSET(struct ot_gpos_singlepos_format1, coverage));
    value_format = table_read_be_word(table, subtable_offset + FIELD_OFFSET(struct ot_gpos_singlepos_format1, value_format));
    value_len = dwrite_popcount(value_format);

    glyph = context->u.pos.glyphs[context->cur];

    if (format == 1)
    {
        const struct ot_gpos_singlepos_format1 *format1 = table_read_ensure(table, subtable_offset,
                FIELD_OFFSET(struct ot_gpos_singlepos_format1, value[value_len]));

        coverage_index = opentype_layout_is_glyph_covered(table, subtable_offset + coverage, glyph);
        if (coverage_index == GLYPH_NOT_COVERED)
            return FALSE;

        opentype_layout_apply_gpos_value(context, subtable_offset, value_format, format1->value, context->cur);
    }
    else if (format == 2)
    {
        WORD value_count = table_read_be_word(table, subtable_offset +
                FIELD_OFFSET(struct ot_gpos_singlepos_format2, value_count));
        const struct ot_gpos_singlepos_format2 *format2 = table_read_ensure(table, subtable_offset,
                FIELD_OFFSET(struct ot_gpos_singlepos_format2, values) + value_count * value_len * sizeof(WORD));

        coverage_index = opentype_layout_is_glyph_covered(table, subtable_offset + coverage, glyph);
        if (coverage_index == GLYPH_NOT_COVERED || coverage_index >= value_count)
            return FALSE;

        opentype_layout_apply_gpos_value(context, subtable_offset, value_format, &format2->values[coverage_index * value_len],
                context->cur);
    }
    else
    {
        WARN("Unknown single adjustment format %u.\n", format);
        return FALSE;
    }

    context->cur++;

    return TRUE;
}

static int __cdecl gpos_pair_adjustment_compare_format1(const void *g, const void *r)
{
    const struct ot_gpos_pairvalue *pairvalue = r;
    UINT16 second_glyph = GET_BE_WORD(pairvalue->second_glyph);
    return *(UINT16 *)g - second_glyph;
}

static BOOL opentype_layout_apply_gpos_pair_adjustment(struct scriptshaping_context *context,
        const struct lookup *lookup, unsigned int subtable_offset)
{
    const struct dwrite_fonttable *table = &context->table->table;
    unsigned int first_glyph, second_glyph;
    struct glyph_iterator iter_pair;
    WORD format, coverage;

    WORD value_format1, value_format2, value_len1, value_len2;
    unsigned int coverage_index;

    glyph_iterator_init(context, lookup->flags, context->cur, 1, &iter_pair);
    if (!glyph_iterator_next(&iter_pair))
        return FALSE;

    if (context->is_rtl)
    {
        first_glyph = iter_pair.pos;
        second_glyph = context->cur;
    }
    else
    {
        first_glyph = context->cur;
        second_glyph = iter_pair.pos;
    }

    format = table_read_be_word(table, subtable_offset);

    coverage = table_read_be_word(table, subtable_offset + FIELD_OFFSET(struct ot_gpos_pairpos_format1, coverage));
    if (!coverage)
        return FALSE;

    coverage_index = opentype_layout_is_glyph_covered(table, subtable_offset + coverage, context->u.pos.glyphs[first_glyph]);
    if (coverage_index == GLYPH_NOT_COVERED)
        return FALSE;

    if (format == 1)
    {
        const struct ot_gpos_pairpos_format1 *format1;
        WORD pairset_count = table_read_be_word(table, subtable_offset + FIELD_OFFSET(struct ot_gpos_pairpos_format1,
                pairset_count));
        unsigned int pairvalue_len, pairset_offset;
        const struct ot_gpos_pairset *pairset;
        const WORD *pairvalue;
        WORD pairvalue_count;

        if (!pairset_count || coverage_index >= pairset_count)
            return FALSE;

        format1 = table_read_ensure(table, subtable_offset, FIELD_OFFSET(struct ot_gpos_pairpos_format1, pairsets[pairset_count]));
        if (!format1)
            return FALSE;

        /* Ordered paired values. */
        pairvalue_count = table_read_be_word(table, subtable_offset + GET_BE_WORD(format1->pairsets[coverage_index]));
        if (!pairvalue_count)
            return FALSE;

        /* Structure length is variable, but does not change across the subtable. */
        value_format1 = GET_BE_WORD(format1->value_format1) & 0xff;
        value_format2 = GET_BE_WORD(format1->value_format2) & 0xff;

        value_len1 = dwrite_popcount(value_format1);
        value_len2 = dwrite_popcount(value_format2);
        pairvalue_len = FIELD_OFFSET(struct ot_gpos_pairvalue, data) + value_len1 * sizeof(WORD) +
                value_len2 * sizeof(WORD);

        pairset_offset = subtable_offset + GET_BE_WORD(format1->pairsets[coverage_index]);
        pairset = table_read_ensure(table, pairset_offset, pairvalue_len * pairvalue_count);
        if (!pairset)
            return FALSE;

        pairvalue = bsearch(&context->u.pos.glyphs[second_glyph], pairset->pairvalues, pairvalue_count,
                pairvalue_len, gpos_pair_adjustment_compare_format1);
        if (!pairvalue)
            return FALSE;

        pairvalue += 1; /* Skip SecondGlyph. */
        opentype_layout_apply_gpos_value(context, pairset_offset, value_format1, pairvalue, first_glyph);
        opentype_layout_apply_gpos_value(context, pairset_offset, value_format2, pairvalue + value_len1,
                second_glyph);

        context->cur = iter_pair.pos;
        if (value_len2)
            context->cur++;
    }
    else if (format == 2)
    {
        const struct ot_gpos_pairpos_format2 *format2;
        WORD class1_count, class2_count;
        unsigned int class1, class2;
        const WCHAR *values;

        value_format1 = table_read_be_word(table, subtable_offset +
                FIELD_OFFSET(struct ot_gpos_pairpos_format2, value_format1)) & 0xff;
        value_format2 = table_read_be_word(table, subtable_offset +
                FIELD_OFFSET(struct ot_gpos_pairpos_format2, value_format2)) & 0xff;

        class1_count = table_read_be_word(table, subtable_offset + FIELD_OFFSET(struct ot_gpos_pairpos_format2, class1_count));
        class2_count = table_read_be_word(table, subtable_offset + FIELD_OFFSET(struct ot_gpos_pairpos_format2, class2_count));

        value_len1 = dwrite_popcount(value_format1);
        value_len2 = dwrite_popcount(value_format2);

        format2 = table_read_ensure(table, subtable_offset, FIELD_OFFSET(struct ot_gpos_pairpos_format2,
                values[class1_count * class2_count * (value_len1 + value_len2)]));
        if (!format2)
            return FALSE;

        class1 = opentype_layout_get_glyph_class(table, subtable_offset + GET_BE_WORD(format2->class_def1),
                context->u.pos.glyphs[first_glyph]);
        class2 = opentype_layout_get_glyph_class(table, subtable_offset + GET_BE_WORD(format2->class_def2),
                context->u.pos.glyphs[second_glyph]);

        if (!(class1 < class1_count && class2 < class2_count))
            return FALSE;

        values = &format2->values[(class1 * class2_count + class2) * (value_len1 + value_len2)];
        opentype_layout_apply_gpos_value(context, subtable_offset, value_format1, values, first_glyph);
        opentype_layout_apply_gpos_value(context, subtable_offset, value_format2, values + value_len1,
                second_glyph);

        context->cur = iter_pair.pos;
        if (value_len2)
            context->cur++;
    }
    else
    {
        WARN("Unknown pair adjustment format %u.\n", format);
        return FALSE;
    }

    return TRUE;
}

static void opentype_layout_gpos_get_anchor(const struct scriptshaping_context *context, unsigned int anchor_offset,
        unsigned int glyph_index, float *x, float *y)
{
    const struct scriptshaping_cache *cache = context->cache;
    const struct dwrite_fonttable *table = &context->table->table;

    WORD format = table_read_be_word(table, anchor_offset);

    *x = *y = 0.0f;

    if (format == 1)
    {
        const struct ot_gpos_anchor_format1 *format1 = table_read_ensure(table, anchor_offset, sizeof(*format1));

        if (format1)
        {
            *x = opentype_scale_gpos_be_value(format1->x_coord, context->emsize, cache->upem);
            *y = opentype_scale_gpos_be_value(format1->y_coord, context->emsize, cache->upem);
        }
    }
    else if (format == 2)
    {
        const struct ot_gpos_anchor_format2 *format2 = table_read_ensure(table, anchor_offset, sizeof(*format2));

        if (format2)
        {
            if (context->measuring_mode != DWRITE_MEASURING_MODE_NATURAL)
                FIXME("Use outline anchor point for glyph %u.\n", context->u.pos.glyphs[glyph_index]);

            *x = opentype_scale_gpos_be_value(format2->x_coord, context->emsize, cache->upem);
            *y = opentype_scale_gpos_be_value(format2->y_coord, context->emsize, cache->upem);
        }
    }
    else if (format == 3)
    {
        const struct ot_gpos_anchor_format3 *format3 = table_read_ensure(table, anchor_offset, sizeof(*format3));

        if (format3)
        {
            *x = opentype_scale_gpos_be_value(format3->x_coord, context->emsize, cache->upem);
            *y = opentype_scale_gpos_be_value(format3->y_coord, context->emsize, cache->upem);

            if (context->measuring_mode != DWRITE_MEASURING_MODE_NATURAL)
            {
                if (format3->x_dev_offset)
                    *x += opentype_layout_gpos_get_dev_value(context, anchor_offset + GET_BE_WORD(format3->x_dev_offset));
                if (format3->y_dev_offset)
                    *y += opentype_layout_gpos_get_dev_value(context, anchor_offset + GET_BE_WORD(format3->y_dev_offset));
            }
        }
    }
    else
        WARN("Unknown anchor format %u.\n", format);
}

static void opentype_set_glyph_attach_type(struct scriptshaping_context *context, unsigned int idx,
        enum attach_type attach_type)
{
    context->glyph_infos[idx].props &= ~GLYPH_PROP_ATTACH_TYPE_MASK;
    context->glyph_infos[idx].props |= attach_type << 16;
}

static enum attach_type opentype_get_glyph_attach_type(const struct scriptshaping_context *context, unsigned int idx)
{
    return (context->glyph_infos[idx].props >> 16) & 0xff;
}

static void opentype_reverse_cursive_offset(struct scriptshaping_context *context, unsigned int i,
        unsigned int new_parent)
{
    enum attach_type type = opentype_get_glyph_attach_type(context, i);
    int chain = context->glyph_infos[i].attach_chain;
    unsigned int j;

    if (!chain || type != GLYPH_ATTACH_CURSIVE)
        return;

    context->glyph_infos[i].attach_chain = 0;

    j = (int)i + chain;
    if (j == new_parent)
        return;

    opentype_reverse_cursive_offset(context, j, new_parent);

    /* FIXME: handle vertical flow direction */
    context->offsets[j].ascenderOffset = -context->offsets[i].ascenderOffset;

    context->glyph_infos[j].attach_chain = -chain;
    opentype_set_glyph_attach_type(context, j, type);
}

static BOOL opentype_layout_apply_gpos_cursive_attachment(struct scriptshaping_context *context,
        const struct lookup *lookup, unsigned int subtable_offset)
{
    const struct dwrite_fonttable *table = &context->table->table;
    UINT16 format, glyph;

    format = table_read_be_word(table, subtable_offset);
    glyph = context->u.pos.glyphs[context->cur];

    if (format == 1)
    {
        WORD coverage_offset = table_read_be_word(table, subtable_offset +
                FIELD_OFFSET(struct ot_gpos_cursive_format1, coverage));
        unsigned int glyph_index, entry_count, entry_anchor, exit_anchor, child, parent;
        float entry_x, entry_y, exit_x, exit_y, delta;
        struct glyph_iterator prev_iter;
        float y_offset;

        if (!coverage_offset)
            return FALSE;

        entry_count = table_read_be_word(table, subtable_offset + FIELD_OFFSET(struct ot_gpos_cursive_format1, count));

        glyph_index = opentype_layout_is_glyph_covered(table, subtable_offset + coverage_offset, glyph);
        if (glyph_index == GLYPH_NOT_COVERED || glyph_index >= entry_count)
            return FALSE;

        entry_anchor = table_read_be_word(table, subtable_offset +
                FIELD_OFFSET(struct ot_gpos_cursive_format1, anchors[glyph_index * 2]));
        if (!entry_anchor)
            return FALSE;

        glyph_iterator_init(context, lookup->flags, context->cur, 1, &prev_iter);
        if (!glyph_iterator_prev(&prev_iter))
            return FALSE;

        glyph_index = opentype_layout_is_glyph_covered(table, subtable_offset + coverage_offset,
                context->u.pos.glyphs[prev_iter.pos]);
        if (glyph_index == GLYPH_NOT_COVERED || glyph_index >= entry_count)
            return FALSE;

        exit_anchor = table_read_be_word(table, subtable_offset +
                FIELD_OFFSET(struct ot_gpos_cursive_format1, anchors[glyph_index * 2 + 1]));
        if (!exit_anchor)
            return FALSE;

        opentype_layout_gpos_get_anchor(context, subtable_offset + exit_anchor, prev_iter.pos, &exit_x, &exit_y);
        opentype_layout_gpos_get_anchor(context, subtable_offset + entry_anchor, context->cur, &entry_x, &entry_y);

        if (context->is_rtl)
        {
            delta = exit_x + context->offsets[prev_iter.pos].advanceOffset;
            context->advances[prev_iter.pos] -= delta;
            context->advances[context->cur] = entry_x + context->offsets[context->cur].advanceOffset;
            context->offsets[prev_iter.pos].advanceOffset -= delta;
        }
        else
        {
            delta = entry_x + context->offsets[context->cur].advanceOffset;
            context->advances[prev_iter.pos] = exit_x + context->offsets[prev_iter.pos].advanceOffset;
            context->advances[context->cur] -= delta;
            context->offsets[context->cur].advanceOffset -= delta;
        }

        if (lookup->flags & LOOKUP_FLAG_RTL)
        {
            y_offset = entry_y - exit_y;
            child = prev_iter.pos;
            parent = context->cur;
        }
        else
        {
            y_offset = exit_y - entry_y;
            child = context->cur;
            parent = prev_iter.pos;
        }

        opentype_reverse_cursive_offset(context, child, parent);

        context->offsets[child].ascenderOffset = y_offset;

        opentype_set_glyph_attach_type(context, child, GLYPH_ATTACH_CURSIVE);
        context->glyph_infos[child].attach_chain = (int)parent - (int)child;
        context->has_gpos_attachment = 1;

        if (context->glyph_infos[parent].attach_chain == -context->glyph_infos[child].attach_chain)
            context->glyph_infos[parent].attach_chain = 0;

        context->cur++;
    }
    else
    {
        WARN("Unknown cursive attachment format %u.\n", format);
        return FALSE;
    }

    return TRUE;
}

static BOOL opentype_layout_apply_mark_array(struct scriptshaping_context *context, unsigned int subtable_offset,
        unsigned int mark_array, unsigned int mark_index, unsigned int glyph_index, unsigned int anchors_matrix,
        unsigned int class_count, unsigned int glyph_pos)
{
    const struct dwrite_fonttable *table = &context->table->table;
    unsigned int mark_class, mark_count, glyph_count;
    const struct ot_gpos_mark_record *record;
    float mark_x, mark_y, base_x, base_y;
    const UINT16 *anchors;

    mark_count = table_read_be_word(table, subtable_offset + mark_array);
    if (mark_index >= mark_count) return FALSE;

    if (!(record = table_read_ensure(table, subtable_offset + mark_array +
            FIELD_OFFSET(struct ot_gpos_mark_array, records[mark_index]), sizeof(*record))))
    {
        return FALSE;
    }

    mark_class = GET_BE_WORD(record->mark_class);
    if (mark_class >= class_count) return FALSE;

    glyph_count = table_read_be_word(table, subtable_offset + anchors_matrix);
    if (glyph_index >= glyph_count) return FALSE;

    /* Anchors data is stored as two dimensional array [glyph_count][class_count], starting with row count field. */
    anchors = table_read_ensure(table, subtable_offset + anchors_matrix + 2, glyph_count * class_count * sizeof(*anchors));
    if (!anchors) return FALSE;

    opentype_layout_gpos_get_anchor(context, subtable_offset + mark_array + GET_BE_WORD(record->mark_anchor),
            context->cur, &mark_x, &mark_y);
    opentype_layout_gpos_get_anchor(context, subtable_offset + anchors_matrix +
            GET_BE_WORD(anchors[glyph_index * class_count + mark_class]), glyph_pos, &base_x, &base_y);

    if (context->is_rtl)
        context->offsets[context->cur].advanceOffset = mark_x - base_x;
    else
        context->offsets[context->cur].advanceOffset = base_x - mark_x;
    context->offsets[context->cur].ascenderOffset = base_y - mark_y;
    opentype_set_glyph_attach_type(context, context->cur, GLYPH_ATTACH_MARK);
    context->glyph_infos[context->cur].attach_chain = (int)glyph_pos - (int)context->cur;
    context->has_gpos_attachment = 1;

    context->cur++;

    return TRUE;
}

static BOOL opentype_layout_apply_gpos_mark_to_base_attachment(struct scriptshaping_context *context,
        const struct lookup *lookup, unsigned int subtable_offset)
{
    const struct dwrite_fonttable *table = &context->table->table;
    WORD format;

    format = table_read_be_word(table, subtable_offset);

    if (format == 1)
    {
        const struct ot_gpos_mark_to_base_format1 *format1;
        unsigned int base_index, mark_index;
        struct glyph_iterator base_iter;

        if (!(format1 = table_read_ensure(table, subtable_offset, sizeof(*format1)))) return FALSE;

        mark_index = opentype_layout_is_glyph_covered(table, subtable_offset + GET_BE_WORD(format1->mark_coverage),
                context->u.pos.glyphs[context->cur]);
        if (mark_index == GLYPH_NOT_COVERED) return FALSE;

        /* Look back for first base glyph. */
        glyph_iterator_init(context, LOOKUP_FLAG_IGNORE_MARKS, context->cur, 1, &base_iter);
        if (!glyph_iterator_prev(&base_iter))
            return FALSE;

        base_index = opentype_layout_is_glyph_covered(table, subtable_offset + GET_BE_WORD(format1->base_coverage),
                context->u.pos.glyphs[base_iter.pos]);
        if (base_index == GLYPH_NOT_COVERED) return FALSE;

        return opentype_layout_apply_mark_array(context, subtable_offset, GET_BE_WORD(format1->mark_array), mark_index,
                base_index, GET_BE_WORD(format1->base_array), GET_BE_WORD(format1->mark_class_count), base_iter.pos);
    }
    else
    {
        WARN("Unknown mark-to-base format %u.\n", format);
        return FALSE;
    }

    return TRUE;
}

static const UINT16 * table_read_array_be_word(const struct dwrite_fonttable *table, unsigned int offset,
        unsigned int index, UINT16 *data)
{
    unsigned int count = table_read_be_word(table, offset);
    const UINT16 *array;

    if (index != ~0u && index >= count) return NULL;
    if (!(array = table_read_ensure(table, offset + 2, count * sizeof(*array)))) return FALSE;
    *data = index == ~0u ? count : GET_BE_WORD(array[index]);
    return array;
}

static BOOL opentype_layout_apply_gpos_mark_to_lig_attachment(struct scriptshaping_context *context,
        const struct lookup *lookup, unsigned int subtable_offset)
{
    const struct dwrite_fonttable *table = &context->table->table;
    WORD format;

    format = table_read_be_word(table, subtable_offset);

    if (format == 1)
    {
        unsigned int mark_index, lig_index, comp_index, class_count, comp_count;
        const struct ot_gpos_mark_to_lig_format1 *format1;
        struct glyph_iterator lig_iter;
        unsigned int lig_array;
        UINT16 lig_attach;

        if (!(format1 = table_read_ensure(table, subtable_offset, sizeof(*format1)))) return FALSE;

        mark_index = opentype_layout_is_glyph_covered(table, subtable_offset + GET_BE_WORD(format1->mark_coverage),
                context->u.pos.glyphs[context->cur]);
        if (mark_index == GLYPH_NOT_COVERED) return FALSE;

        glyph_iterator_init(context, LOOKUP_FLAG_IGNORE_MARKS, context->cur, 1, &lig_iter);
        if (!glyph_iterator_prev(&lig_iter))
            return FALSE;

        lig_index = opentype_layout_is_glyph_covered(table, subtable_offset + GET_BE_WORD(format1->lig_coverage),
                context->u.pos.glyphs[lig_iter.pos]);
        if (lig_index == GLYPH_NOT_COVERED) return FALSE;

        class_count = GET_BE_WORD(format1->mark_class_count);

        lig_array = GET_BE_WORD(format1->lig_array);

        if (!table_read_array_be_word(table, subtable_offset + lig_array, lig_index, &lig_attach)) return FALSE;

        comp_count = table_read_be_word(table, subtable_offset + lig_array + lig_attach);
        if (!comp_count) return FALSE;

        comp_index = context->u.buffer.glyph_props[lig_iter.pos].components -
                context->u.buffer.glyph_props[context->cur].lig_component - 1;
        if (comp_index >= comp_count) return FALSE;

        return opentype_layout_apply_mark_array(context, subtable_offset, GET_BE_WORD(format1->mark_array), mark_index,
                comp_index, lig_array + lig_attach, class_count, lig_iter.pos);
    }
    else
        WARN("Unknown mark-to-ligature format %u.\n", format);

    return FALSE;
}

static BOOL opentype_layout_apply_gpos_mark_to_mark_attachment(struct scriptshaping_context *context,
        const struct lookup *lookup, unsigned int subtable_offset)
{
    const struct dwrite_fonttable *table = &context->table->table;
    WORD format;

    format = table_read_be_word(table, subtable_offset);

    if (format == 1)
    {
        const struct ot_gpos_mark_to_mark_format1 *format1;
        unsigned int mark1_index, mark2_index;
        struct glyph_iterator mark_iter;

        if (!(format1 = table_read_ensure(table, subtable_offset, sizeof(*format1)))) return FALSE;

        mark1_index = opentype_layout_is_glyph_covered(table, subtable_offset + GET_BE_WORD(format1->mark1_coverage),
                context->u.pos.glyphs[context->cur]);
        if (mark1_index == GLYPH_NOT_COVERED) return FALSE;

        glyph_iterator_init(context, lookup->flags & ~LOOKUP_FLAG_IGNORE_MASK, context->cur, 1, &mark_iter);
        if (!glyph_iterator_prev(&mark_iter))
            return FALSE;

        if (!context->u.pos.glyph_props[mark_iter.pos].isDiacritic)
            return FALSE;

        mark2_index = opentype_layout_is_glyph_covered(table, subtable_offset + GET_BE_WORD(format1->mark2_coverage),
                context->u.pos.glyphs[mark_iter.pos]);
        if (mark2_index == GLYPH_NOT_COVERED) return FALSE;

        return opentype_layout_apply_mark_array(context, subtable_offset, GET_BE_WORD(format1->mark1_array), mark1_index,
                mark2_index, GET_BE_WORD(format1->mark2_array), GET_BE_WORD(format1->mark_class_count), mark_iter.pos);
    }
    else
    {
        WARN("Unknown mark-to-mark format %u.\n", format);
        return FALSE;
    }

    return TRUE;
}

static BOOL opentype_layout_apply_context(struct scriptshaping_context *context, const struct lookup *lookup,
        unsigned int subtable_offset);
static BOOL opentype_layout_apply_chain_context(struct scriptshaping_context *context, const struct lookup *lookup,
        unsigned int subtable_offset);

static BOOL opentype_layout_apply_gpos_lookup(struct scriptshaping_context *context, const struct lookup *lookup)
{
    unsigned int i, lookup_type;
    BOOL ret = FALSE;

    for (i = 0; i < lookup->subtable_count; ++i)
    {
        unsigned int subtable_offset = opentype_layout_get_gsubgpos_subtable(context, lookup, i, &lookup_type);

        switch (lookup_type)
        {
            case GPOS_LOOKUP_SINGLE_ADJUSTMENT:
                ret = opentype_layout_apply_gpos_single_adjustment(context, lookup, subtable_offset);
                break;
            case GPOS_LOOKUP_PAIR_ADJUSTMENT:
                ret = opentype_layout_apply_gpos_pair_adjustment(context, lookup, subtable_offset);
                break;
            case GPOS_LOOKUP_CURSIVE_ATTACHMENT:
                ret = opentype_layout_apply_gpos_cursive_attachment(context, lookup, subtable_offset);
                break;
            case GPOS_LOOKUP_MARK_TO_BASE_ATTACHMENT:
                ret = opentype_layout_apply_gpos_mark_to_base_attachment(context, lookup, subtable_offset);
                break;
            case GPOS_LOOKUP_MARK_TO_LIGATURE_ATTACHMENT:
                ret = opentype_layout_apply_gpos_mark_to_lig_attachment(context, lookup, subtable_offset);
                break;
            case GPOS_LOOKUP_MARK_TO_MARK_ATTACHMENT:
                ret = opentype_layout_apply_gpos_mark_to_mark_attachment(context, lookup, subtable_offset);
                break;
            case GPOS_LOOKUP_CONTEXTUAL_POSITION:
                ret = opentype_layout_apply_context(context, lookup, subtable_offset);
                break;
            case GPOS_LOOKUP_CONTEXTUAL_CHAINING_POSITION:
                ret = opentype_layout_apply_chain_context(context, lookup, subtable_offset);
                break;
            case GPOS_LOOKUP_EXTENSION_POSITION:
                WARN("Recursive extension lookup.\n");
                break;
            default:
                WARN("Unknown lookup type %u.\n", lookup_type);
        }

        if (ret)
            break;
    }

    return ret;
}

struct lookups
{
    struct lookup *lookups;
    size_t capacity;
    size_t count;
};

static int __cdecl lookups_sorting_compare(const void *a, const void *b)
{
    const struct lookup *left = (const struct lookup *)a;
    const struct lookup *right = (const struct lookup *)b;
    return left->index < right->index ? -1 : left->index > right->index ? 1 : 0;
};

static BOOL opentype_layout_init_lookup(const struct ot_gsubgpos_table *table, unsigned short lookup_index,
        const struct shaping_feature *feature, struct lookup *lookup)
{
    unsigned short subtable_count, lookup_type, mark_filtering_set;
    const struct ot_lookup_table *lookup_table;
    unsigned int offset, flags;

    if (!(offset = table_read_be_word(&table->table, table->lookup_list +
            FIELD_OFFSET(struct ot_lookup_list, lookup[lookup_index]))))
    {
        return FALSE;
    }

    offset += table->lookup_list;

    if (!(lookup_table = table_read_ensure(&table->table, offset, sizeof(*lookup_table))))
        return FALSE;

    if (!(subtable_count = GET_BE_WORD(lookup_table->subtable_count)))
        return FALSE;

    lookup_type = GET_BE_WORD(lookup_table->lookup_type);
    flags = GET_BE_WORD(lookup_table->flags);

    if (flags & LOOKUP_FLAG_USE_MARK_FILTERING_SET)
    {
        mark_filtering_set = table_read_be_word(&table->table, offset +
                FIELD_OFFSET(struct ot_lookup_table, subtable[subtable_count]));
        flags |= mark_filtering_set << 16;
    }

    lookup->index = lookup_index;
    lookup->type = lookup_type;
    lookup->flags = flags;
    lookup->subtable_count = subtable_count;
    lookup->offset = offset;
    if (feature)
    {
        lookup->mask = feature->mask;
        lookup->auto_zwnj = !(feature->flags & FEATURE_MANUAL_ZWNJ);
        lookup->auto_zwj = !(feature->flags & FEATURE_MANUAL_ZWJ);
    }

    return TRUE;
}

static void opentype_layout_add_lookups(const struct ot_feature_list *feature_list, UINT16 total_lookup_count,
        const struct ot_gsubgpos_table *table, struct shaping_feature *feature, struct lookups *lookups)
{
    UINT16 feature_offset, lookup_count;
    unsigned int i;

    /* Feature wasn't found */
    if (feature->index == 0xffff)
        return;

    feature_offset = GET_BE_WORD(feature_list->features[feature->index].offset);

    lookup_count = table_read_be_word(&table->table, table->feature_list + feature_offset +
            FIELD_OFFSET(struct ot_feature, lookup_count));
    if (!lookup_count)
        return;

    if (!dwrite_array_reserve((void **)&lookups->lookups, &lookups->capacity, lookups->count + lookup_count,
            sizeof(*lookups->lookups)))
    {
        return;
    }

    for (i = 0; i < lookup_count; ++i)
    {
        UINT16 lookup_index = table_read_be_word(&table->table, table->feature_list + feature_offset +
                FIELD_OFFSET(struct ot_feature, lookuplist_index[i]));

        if (lookup_index >= total_lookup_count)
            continue;

        if (opentype_layout_init_lookup(table, lookup_index, feature, &lookups->lookups[lookups->count]))
            lookups->count++;
    }
}

static void opentype_layout_collect_lookups(struct scriptshaping_context *context, unsigned int script_index,
        unsigned int language_index, struct shaping_features *features, const struct ot_gsubgpos_table *table,
        struct lookups *lookups)
{
    unsigned int last_num_lookups = 0, stage, script_feature_count = 0;
    UINT16 total_feature_count, total_lookup_count;
    struct shaping_feature required_feature = { 0 };
    const struct ot_feature_list *feature_list;
    const struct ot_langsys *langsys = NULL;
    struct shaping_feature *feature;
    unsigned int i, j, next_bit;
    unsigned int global_bit_shift = 1;
    unsigned int global_bit_mask = 2;
    UINT16 feature_index;

    if (!table->table.data)
        return;

    if (script_index != ~0u)
    {
        unsigned int table_offset, langsys_offset;

        /* ScriptTable offset. */
        table_offset = table_read_be_word(&table->table, table->script_list + FIELD_OFFSET(struct ot_script_list, scripts) +
                script_index * sizeof(struct ot_script_record) + FIELD_OFFSET(struct ot_script_record, script));
        if (!table_offset)
            return;

        if (language_index == ~0u)
            langsys_offset = table_read_be_word(&table->table, table->script_list + table_offset);
        else
            langsys_offset = table_read_be_word(&table->table, table->script_list + table_offset +
                    FIELD_OFFSET(struct ot_script, langsys) + language_index * sizeof(struct ot_langsys_record) +
                    FIELD_OFFSET(struct ot_langsys_record, langsys));
        langsys_offset += table->script_list + table_offset;

        script_feature_count = table_read_be_word(&table->table, langsys_offset + FIELD_OFFSET(struct ot_langsys, feature_count));
        if (script_feature_count)
            langsys = table_read_ensure(&table->table, langsys_offset,
                    FIELD_OFFSET(struct ot_langsys, feature_index[script_feature_count]));
        if (!langsys)
            script_feature_count = 0;
    }

    total_feature_count = table_read_be_word(&table->table, table->feature_list);
    if (!total_feature_count)
        return;

    total_lookup_count = table_read_be_word(&table->table, table->lookup_list);
    if (!total_lookup_count)
        return;

    feature_list = table_read_ensure(&table->table, table->feature_list,
            FIELD_OFFSET(struct ot_feature_list, features[total_feature_count]));
    if (!feature_list)
        return;

    /* Required feature. */
    required_feature.index = langsys ? GET_BE_WORD(langsys->required_feature_index) : 0xffff;
    if (required_feature.index < total_feature_count)
        required_feature.tag = feature_list->features[required_feature.index].tag;
    required_feature.mask = global_bit_mask;

    context->global_mask = global_bit_mask;
    next_bit = global_bit_shift + 1;
    for (i = 0; i < features->count; ++i)
    {
        BOOL found = FALSE;
        DWORD bits_needed;

        feature = &features->features[i];

        feature->index = 0xffff;

        if ((feature->flags & FEATURE_GLOBAL) && feature->max_value == 1)
            bits_needed = 0;
        else
        {
            BitScanReverse(&bits_needed, min(feature->max_value, 256));
            bits_needed++;
        }

        if (!feature->max_value || next_bit + bits_needed > 8 * sizeof (feature->mask))
            continue;

        if (required_feature.tag == feature->tag)
            required_feature.stage = feature->stage;

        for (j = 0; j < script_feature_count; ++j)
        {
            feature_index = GET_BE_WORD(langsys->feature_index[j]);
            if (feature_index >= total_feature_count)
                continue;
            if ((found = feature_list->features[feature_index].tag == feature->tag))
            {
                feature->index = feature_index;
                break;
            }
        }

        if (!found && (features->features[i].flags & FEATURE_GLOBAL_SEARCH))
        {
            for (j = 0; j < total_feature_count; ++j)
            {
                if ((found = (feature_list->features[j].tag == feature->tag)))
                {
                    feature->index = j;
                    break;
                }
            }
        }

        if (!found && !(features->features[i].flags & FEATURE_HAS_FALLBACK))
            continue;

        if (feature->flags & FEATURE_GLOBAL && feature->max_value == 1)
        {
            feature->shift = global_bit_shift;
            feature->mask = global_bit_mask;
        }
        else
        {
            feature->shift = next_bit;
            feature->mask = (1 << (next_bit + bits_needed)) - (1 << next_bit);
            next_bit += bits_needed;
            context->global_mask |= (feature->default_value << feature->shift) & feature->mask;
        }
        if (!found)
            feature->flags |= FEATURE_NEEDS_FALLBACK;
    }

    for (stage = 0; stage <= features->stage; ++stage)
    {
        if (required_feature.index != 0xffff && required_feature.stage == stage)
            opentype_layout_add_lookups(feature_list, total_lookup_count, table, &required_feature, lookups);

        for (i = 0; i < features->count; ++i)
        {
            if (features->features[i].stage == stage)
                opentype_layout_add_lookups(feature_list, total_lookup_count, table, &features->features[i], lookups);
        }

        /* Sort and merge lookups for current stage. */
        if (last_num_lookups < lookups->count)
        {
            qsort(lookups->lookups + last_num_lookups, lookups->count - last_num_lookups, sizeof(*lookups->lookups),
                    lookups_sorting_compare);

            j = last_num_lookups;
            for (i = j + 1; i < lookups->count; ++i)
            {
                if (lookups->lookups[i].index != lookups->lookups[j].index)
                {
                    lookups->lookups[++j] = lookups->lookups[i];
                }
                else
                {
                    lookups->lookups[j].mask |= lookups->lookups[i].mask;
                    lookups->lookups[j].auto_zwnj &= lookups->lookups[i].auto_zwnj;
                    lookups->lookups[j].auto_zwj &= lookups->lookups[i].auto_zwj;
                }
            }
            lookups->count = j + 1;
        }

        last_num_lookups = lookups->count;
        features->stages[stage].last_lookup = last_num_lookups;
    }
}

static int __cdecl feature_search_compare(const void *a, const void* b)
{
    unsigned int tag = *(unsigned int *)a;
    const struct shaping_feature *feature = b;

    return tag < feature->tag ? -1 : tag > feature->tag ? 1 : 0;
}

static unsigned int shaping_features_get_mask(const struct shaping_features *features, unsigned int tag, unsigned int *shift)
{
    struct shaping_feature *feature;

    feature = bsearch(&tag, features->features, features->count, sizeof(*features->features), feature_search_compare);

    if (!feature || feature->index == 0xffff)
        return 0;

    if (shift) *shift = feature->shift;
    return feature->mask;
}

unsigned int shape_get_feature_1_mask(const struct shaping_features *features, unsigned int tag)
{
    unsigned int shift, mask = shaping_features_get_mask(features, tag, &shift);
    return (1 << shift) & mask;
}

static void opentype_layout_get_glyph_range_for_text(struct scriptshaping_context *context, unsigned int start_char,
        unsigned int end_char, unsigned int *start_glyph, unsigned int *end_glyph)
{
    *start_glyph = context->u.buffer.clustermap[start_char];
    if (end_char >= context->length - 1)
        *end_glyph = context->glyph_count - 1;
    else
        *end_glyph = context->u.buffer.clustermap[end_char] - 1;
}

static void opentype_layout_set_glyph_masks(struct scriptshaping_context *context, const struct shaping_features *features)
{
   const DWRITE_TYPOGRAPHIC_FEATURES **user_features = context->user_features.features;
   unsigned int f, r, g, start_char, mask, shift, value;

   for (g = 0; g < context->glyph_count; ++g)
       context->glyph_infos[g].mask = context->global_mask;

   if (opentype_layout_is_subst_context(context) && context->shaper->setup_masks)
       context->shaper->setup_masks(context, features);

   for (r = 0, start_char = 0; r < context->user_features.range_count; ++r)
   {
       unsigned int start_glyph, end_glyph;

       if (start_char >= context->length)
           break;

       if (!context->user_features.range_lengths[r])
           continue;

       opentype_layout_get_glyph_range_for_text(context, start_char, start_char + context->user_features.range_lengths[r],
               &start_glyph, &end_glyph);
       start_char += context->user_features.range_lengths[r];

       if (start_glyph > end_glyph || end_glyph >= context->glyph_count)
           continue;

       for (f = 0; f < user_features[r]->featureCount; ++f)
       {
           mask = shaping_features_get_mask(features, user_features[r]->features[f].nameTag, &shift);
           if (!mask)
               continue;

           value = (user_features[r]->features[f].parameter << shift) & mask;

           for (g = start_glyph; g <= end_glyph; ++g)
               context->glyph_infos[g].mask = (context->glyph_infos[g].mask & ~mask) | value;
       }
   }
}

static void opentype_layout_apply_gpos_context_lookup(struct scriptshaping_context *context, unsigned int lookup_index)
{
    struct lookup lookup = { 0 };
    if (opentype_layout_init_lookup(context->table, lookup_index, NULL, &lookup))
        opentype_layout_apply_gpos_lookup(context, &lookup);
}

static void opentype_propagate_attachment_offsets(struct scriptshaping_context *context, unsigned int i)
{
    enum attach_type type = opentype_get_glyph_attach_type(context, i);
    int chain = context->glyph_infos[i].attach_chain;
    unsigned int j, k;

    if (!chain)
        return;

    context->glyph_infos[i].attach_chain = 0;

    j = (int)i + chain;
    if (j >= context->glyph_count)
        return;

    opentype_propagate_attachment_offsets(context, j);

    if (type == GLYPH_ATTACH_CURSIVE)
    {
        /* FIXME: handle vertical direction. */
        context->offsets[i].ascenderOffset += context->offsets[j].ascenderOffset;
    }
    else if (type == GLYPH_ATTACH_MARK)
    {
        context->offsets[i].advanceOffset += context->offsets[j].advanceOffset;
        context->offsets[i].ascenderOffset += context->offsets[j].ascenderOffset;

        /* FIXME: handle vertical adjustment. */
        if (context->is_rtl)
        {
            for (k = j + 1; k < i + 1; ++k)
            {
                context->offsets[i].advanceOffset += context->advances[k];
            }
        }
        else
        {
            for (k = j; k < i; k++)
            {
                context->offsets[i].advanceOffset -= context->advances[k];
            }
        }
    }
}

void opentype_layout_apply_gpos_features(struct scriptshaping_context *context, unsigned int script_index,
        unsigned int language_index, struct shaping_features *features)
{
    struct lookups lookups = { 0 };
    unsigned int i;
    BOOL ret;

    context->nesting_level_left = SHAPE_MAX_NESTING_LEVEL;
    context->u.buffer.apply_context_lookup = opentype_layout_apply_gpos_context_lookup;
    opentype_layout_collect_lookups(context, script_index, language_index, features, &context->cache->gpos, &lookups);

    for (i = 0; i < context->glyph_count; ++i)
        opentype_set_glyph_props(context, i);
    opentype_layout_set_glyph_masks(context, features);

    for (i = 0; i < lookups.count; ++i)
    {
        const struct lookup *lookup = &lookups.lookups[i];

        context->cur = 0;
        context->lookup_mask = lookup->mask;
        context->auto_zwnj = lookup->auto_zwnj;
        context->auto_zwj = lookup->auto_zwj;

        while (context->cur < context->glyph_count)
        {
            ret = FALSE;

            if ((context->glyph_infos[context->cur].mask & lookup->mask) &&
                    lookup_is_glyph_match(context, context->cur, lookup->flags))
            {
                ret = opentype_layout_apply_gpos_lookup(context, lookup);
            }

            if (!ret)
                context->cur++;
        }
    }

    free(lookups.lookups);

    if (context->has_gpos_attachment)
    {
        for (i = 0; i < context->glyph_count; ++i)
            opentype_propagate_attachment_offsets(context, i);
    }
}

static void opentype_layout_replace_glyph(struct scriptshaping_context *context, UINT16 glyph)
{
    UINT16 orig_glyph = context->u.subst.glyphs[context->cur];
    if (glyph != orig_glyph)
    {
        context->u.subst.glyphs[context->cur] = glyph;
        opentype_set_subst_glyph_props(context, context->cur);
    }
}

static BOOL opentype_layout_apply_gsub_single_substitution(struct scriptshaping_context *context, const struct lookup *lookup,
        unsigned int subtable_offset)
{
    const struct dwrite_fonttable *table = &context->table->table;
    UINT16 format, coverage, orig_glyph, glyph;
    unsigned int coverage_index;

    orig_glyph = glyph = context->u.subst.glyphs[context->cur];

    format = table_read_be_word(table, subtable_offset);

    coverage = table_read_be_word(table, subtable_offset + FIELD_OFFSET(struct ot_gsub_singlesubst_format1, coverage));

    if (format == 1)
    {
        coverage_index = opentype_layout_is_glyph_covered(table, subtable_offset + coverage, glyph);
        if (coverage_index == GLYPH_NOT_COVERED) return FALSE;

        glyph = orig_glyph + table_read_be_word(table, subtable_offset + FIELD_OFFSET(struct ot_gsub_singlesubst_format1, delta));
    }
    else if (format == 2)
    {
        coverage_index = opentype_layout_is_glyph_covered(table, subtable_offset + coverage, glyph);
        if (coverage_index == GLYPH_NOT_COVERED) return FALSE;

        if (!table_read_array_be_word(table, subtable_offset + FIELD_OFFSET(struct ot_gsub_singlesubst_format2, count),
                coverage_index, &glyph))
        {
            return FALSE;
        }
    }
    else
    {
        WARN("Unknown single substitution format %u.\n", format);
        return FALSE;
    }

    opentype_layout_replace_glyph(context, glyph);
    context->cur++;

    return TRUE;
}

static BOOL opentype_layout_gsub_ensure_buffer(struct scriptshaping_context *context, unsigned int count)
{
    DWRITE_SHAPING_GLYPH_PROPERTIES *glyph_props;
    struct shaping_glyph_info *glyph_infos;
    unsigned int new_capacity;
    UINT16 *glyphs;
    BOOL ret;

    if (context->u.subst.capacity >= count)
        return TRUE;

    new_capacity = context->u.subst.capacity * 2;

    if ((glyphs = realloc(context->u.subst.glyphs, new_capacity * sizeof(*glyphs))))
        context->u.subst.glyphs = glyphs;
    if ((glyph_props = realloc(context->u.subst.glyph_props, new_capacity * sizeof(*glyph_props))))
        context->u.subst.glyph_props = glyph_props;
    if ((glyph_infos = realloc(context->glyph_infos, new_capacity * sizeof(*glyph_infos))))
        context->glyph_infos = glyph_infos;

    if ((ret = (glyphs && glyph_props && glyph_infos)))
        context->u.subst.capacity = new_capacity;

    return ret;
}

static BOOL opentype_layout_apply_gsub_mult_substitution(struct scriptshaping_context *context, const struct lookup *lookup,
        unsigned int subtable_offset)
{
    const struct dwrite_fonttable *table = &context->table->table;
    UINT16 format, coverage, glyph, glyph_count;
    unsigned int i, idx, coverage_index;
    const UINT16 *glyphs;

    idx = context->cur;
    glyph = context->u.subst.glyphs[idx];

    format = table_read_be_word(table, subtable_offset);

    coverage = table_read_be_word(table, subtable_offset + FIELD_OFFSET(struct ot_gsub_multsubst_format1, coverage));

    if (format == 1)
    {
        UINT16 seq_offset;

        coverage_index = opentype_layout_is_glyph_covered(table, subtable_offset + coverage, glyph);
        if (coverage_index == GLYPH_NOT_COVERED) return FALSE;

        if (!table_read_array_be_word(table, subtable_offset + FIELD_OFFSET(struct ot_gsub_multsubst_format1, seq_count),
                coverage_index, &seq_offset))
        {
            return FALSE;
        }

        if (!(glyphs = table_read_array_be_word(table, subtable_offset + seq_offset, ~0u, &glyph_count))) return FALSE;

        if (glyph_count == 1)
        {
            /* Equivalent of single substitution. */
            opentype_layout_replace_glyph(context, GET_BE_WORD(glyphs[0]));
            context->cur++;
        }
        else if (glyph_count == 0)
        {
            context->cur++;
        }
        else
        {
            unsigned int shift_len, src_idx, dest_idx, mask;

            /* Current glyph is also replaced. */
            glyph_count--;

            if (!(opentype_layout_gsub_ensure_buffer(context, context->glyph_count + glyph_count)))
                return FALSE;

            shift_len = context->cur + 1 < context->glyph_count ? context->glyph_count - context->cur - 1 : 0;

            if (shift_len)
            {
                src_idx = context->cur + 1;
                dest_idx = src_idx + glyph_count;

                memmove(&context->u.subst.glyphs[dest_idx], &context->u.subst.glyphs[src_idx],
                        shift_len * sizeof(*context->u.subst.glyphs));
                memmove(&context->u.subst.glyph_props[dest_idx], &context->u.subst.glyph_props[src_idx],
                        shift_len * sizeof(*context->u.subst.glyph_props));
                memmove(&context->glyph_infos[dest_idx], &context->glyph_infos[src_idx],
                        shift_len * sizeof(*context->glyph_infos));
            }

            mask = context->glyph_infos[context->cur].mask;
            for (i = 0, idx = context->cur; i <= glyph_count; ++i)
            {
                glyph = GET_BE_WORD(glyphs[i]);
                context->u.subst.glyphs[idx + i] = glyph;
                if (i)
                {
                    context->u.subst.glyph_props[idx + i].isClusterStart = 0;
                    context->u.buffer.glyph_props[idx + i].components = 0;
                    context->glyph_infos[idx + i].start_text_idx = 0;
                }
                opentype_set_subst_glyph_props(context, idx + i);
                /* Inherit feature mask from original matched glyph. */
                context->glyph_infos[idx + i].mask = mask;
            }

            context->cur += glyph_count + 1;
            context->glyph_count += glyph_count;
        }
    }
    else
    {
        WARN("Unknown multiple substitution format %u.\n", format);
        return FALSE;
    }

    return TRUE;
}

static BOOL opentype_layout_apply_gsub_alt_substitution(struct scriptshaping_context *context, const struct lookup *lookup,
        unsigned int subtable_offset)
{
    const struct dwrite_fonttable *table = &context->table->table;
    unsigned int idx, coverage_index;
    UINT16 format, coverage, glyph;

    idx = context->cur;
    glyph = context->u.subst.glyphs[idx];

    format = table_read_be_word(table, subtable_offset);

    if (format == 1)
    {
        const struct ot_gsub_altsubst_format1 *format1 = table_read_ensure(table, subtable_offset, sizeof(*format1));
        DWORD shift;
        unsigned int alt_index;
        UINT16 set_offset;

        coverage = table_read_be_word(table, subtable_offset + FIELD_OFFSET(struct ot_gsub_altsubst_format1, coverage));

        coverage_index = opentype_layout_is_glyph_covered(table, subtable_offset + coverage, glyph);
        if (coverage_index == GLYPH_NOT_COVERED) return FALSE;

        if (!table_read_array_be_word(table, subtable_offset + FIELD_OFFSET(struct ot_gsub_altsubst_format1, count),
                coverage_index, &set_offset))
            return FALSE;

        /* Argument is 1-based. */
        BitScanForward(&shift, context->lookup_mask);
        alt_index = (context->lookup_mask & context->glyph_infos[idx].mask) >> shift;
        if (!alt_index) return FALSE;

        if (!table_read_array_be_word(table, subtable_offset + set_offset, alt_index - 1, &glyph)) return FALSE;
    }
    else
    {
        WARN("Unexpected alternate substitution format %d.\n", format);
        return FALSE;
    }

    opentype_layout_replace_glyph(context, glyph);
    context->cur++;

    return TRUE;
}

static BOOL opentype_layout_context_match_input(const struct match_context *mc, unsigned int count, const UINT16 *input,
        unsigned int *end_offset, unsigned int *match_positions)
{
    struct match_data match_data = { .mc = mc, .subtable_offset = mc->input_offset };
    struct scriptshaping_context *context = mc->context;
    struct glyph_iterator iter;
    unsigned int i;

    if (count > GLYPH_CONTEXT_MAX_LENGTH)
        return FALSE;

    match_positions[0] = context->cur;

    glyph_iterator_init(context, mc->lookup->flags, context->cur, count - 1, &iter);
    iter.mask = context->lookup_mask;
    iter.match_func = mc->match_func;
    iter.match_data = &match_data;
    iter.glyph_data = input;

    for (i = 1; i < count; ++i)
    {
        if (!glyph_iterator_next(&iter))
            return FALSE;

        match_positions[i] = iter.pos;
    }

    *end_offset = iter.pos - context->cur + 1;

    return TRUE;
}

/* Marks text segment as unsafe to break between [start, end) glyphs. */
void opentype_layout_unsafe_to_break(struct scriptshaping_context *context, unsigned int start,
        unsigned int end)
{
    unsigned int i;

    while (start && !context->u.buffer.glyph_props[start].isClusterStart)
        --start;

    while (--end && !context->u.buffer.glyph_props[end].isClusterStart)
        ;

    if (start == end)
    {
        context->u.buffer.text_props[context->glyph_infos[start].start_text_idx].canBreakShapingAfter = 0;
        return;
    }

    for (i = context->glyph_infos[start].start_text_idx; i < context->glyph_infos[end].start_text_idx; ++i)
    {
        context->u.buffer.text_props[i].canBreakShapingAfter = 0;
    }
}

static void opentype_layout_delete_glyph(struct scriptshaping_context *context, unsigned int idx)
{
    unsigned int shift_len;

    shift_len = context->glyph_count - context->cur - 1;

    if (shift_len)
    {
        memmove(&context->u.buffer.glyphs[idx], &context->u.buffer.glyphs[idx + 1],
                shift_len * sizeof(*context->u.buffer.glyphs));
        memmove(&context->u.buffer.glyph_props[idx], &context->u.buffer.glyph_props[idx + 1],
                shift_len * sizeof(*context->u.buffer.glyph_props));
        memmove(&context->glyph_infos[idx], &context->glyph_infos[idx + 1], shift_len * sizeof(*context->glyph_infos));
    }

    context->glyph_count--;
}

static BOOL opentype_layout_apply_ligature(struct scriptshaping_context *context, unsigned int offset,
        const struct lookup *lookup)
{
    struct match_context mc = { .context = context, .lookup = lookup, .match_func = opentype_match_glyph_func };
    const struct dwrite_fonttable *gsub = &context->table->table;
    unsigned int match_positions[GLYPH_CONTEXT_MAX_LENGTH];
    unsigned int i, j, comp_count, match_length = 0;
    const struct ot_gsub_lig *lig;
    UINT16 lig_glyph;

    comp_count = table_read_be_word(gsub, offset + FIELD_OFFSET(struct ot_gsub_lig, comp_count));

    if (!comp_count)
        return FALSE;

    lig = table_read_ensure(gsub, offset, FIELD_OFFSET(struct ot_gsub_lig, components[comp_count-1]));
    if (!lig)
        return FALSE;

    lig_glyph = GET_BE_WORD(lig->lig_glyph);

    if (comp_count == 1)
    {
        opentype_layout_replace_glyph(context, lig_glyph);
        context->cur++;
        return TRUE;
    }

    if (!opentype_layout_context_match_input(&mc, comp_count, lig->components, &match_length, match_positions))
        return FALSE;

    opentype_layout_replace_glyph(context, lig_glyph);
    context->u.buffer.glyph_props[context->cur].components = comp_count;

    /* Positioning against a ligature implies keeping track of ligature component
       glyph should be attached to. Update per-glyph property for interleaving glyphs,
       0 means attaching to last component, n - attaching to n-th glyph before last. */
    for (i = 1; i < comp_count; ++i)
    {
        j = match_positions[i - 1] + 1;
        while (j < match_positions[i])
        {
            context->u.buffer.glyph_props[j++].lig_component = comp_count - i;
        }
    }
    opentype_layout_unsafe_to_break(context, match_positions[0], match_positions[comp_count - 1] + 1);

    /* Delete ligated glyphs, backwards to preserve index. */
    for (i = 1; i < comp_count; ++i)
    {
        opentype_layout_delete_glyph(context, match_positions[comp_count - i]);
    }

    /* Skip whole matched sequence, accounting for deleted glyphs. */
    context->cur += match_length - (comp_count - 1);

    return TRUE;
}

static BOOL opentype_layout_apply_gsub_lig_substitution(struct scriptshaping_context *context, const struct lookup *lookup,
        unsigned int subtable_offset)
{
    const struct dwrite_fonttable *table = &context->table->table;
    UINT16 format, coverage, glyph, lig_set_offset;
    unsigned int coverage_index;

    glyph = context->u.subst.glyphs[context->cur];

    format = table_read_be_word(table, subtable_offset);

    if (format == 1)
    {
        const struct ot_gsub_ligsubst_format1 *format1 = table_read_ensure(table, subtable_offset, sizeof(*format1));
        unsigned int i;
        const UINT16 *offsets;
        UINT16 lig_count;

        coverage = table_read_be_word(table, subtable_offset + FIELD_OFFSET(struct ot_gsub_ligsubst_format1, coverage));

        coverage_index = opentype_layout_is_glyph_covered(table, subtable_offset + coverage, glyph);
        if (coverage_index == GLYPH_NOT_COVERED) return FALSE;

        if (!table_read_array_be_word(table, subtable_offset + FIELD_OFFSET(struct ot_gsub_ligsubst_format1, lig_set_count),
                coverage_index, &lig_set_offset))
            return FALSE;

        if (!(offsets = table_read_array_be_word(table, subtable_offset + lig_set_offset, ~0u, &lig_count)))
            return FALSE;

        /* First applicable ligature is used. */
        for (i = 0; i < lig_count; ++i)
        {
            if (opentype_layout_apply_ligature(context, subtable_offset + lig_set_offset + GET_BE_WORD(offsets[i]), lookup))
                return TRUE;
        }
    }
    else
        WARN("Unexpected ligature substitution format %d.\n", format);

    return FALSE;
}

static BOOL opentype_layout_context_match_backtrack(const struct match_context *mc, unsigned int count,
        const UINT16 *backtrack, unsigned int *match_start)
{
    struct match_data match_data = { .mc = mc, .subtable_offset = mc->backtrack_offset };
    struct scriptshaping_context *context = mc->context;
    struct glyph_iterator iter;
    unsigned int i;

    glyph_iterator_init(context, mc->lookup->flags, context->cur, count, &iter);
    iter.match_func = mc->match_func;
    iter.match_data = &match_data;
    iter.glyph_data = backtrack;
    iter.ignore_zwnj |= context->auto_zwnj;
    iter.ignore_zwj = 1;

    for (i = 0; i < count; ++i)
    {
        if (!glyph_iterator_prev(&iter))
            return FALSE;
    }

    *match_start = iter.pos;

    return TRUE;
}

static BOOL opentype_layout_context_match_lookahead(const struct match_context *mc, unsigned int count,
        const UINT16 *lookahead, unsigned int offset, unsigned int *end_index)
{
    struct match_data match_data = { .mc = mc, .subtable_offset = mc->lookahead_offset };
    struct scriptshaping_context *context = mc->context;
    struct glyph_iterator iter;
    unsigned int i;

    glyph_iterator_init(context, mc->lookup->flags, context->cur + offset - 1, count, &iter);
    iter.match_func = mc->match_func;
    iter.match_data = &match_data;
    iter.glyph_data = lookahead;
    iter.ignore_zwnj |= context->auto_zwnj;
    iter.ignore_zwj = 1;

    for (i = 0; i < count; ++i)
    {
        if (!glyph_iterator_next(&iter))
            return FALSE;
    }

    *end_index = iter.pos;

    return TRUE;
}

static BOOL opentype_layout_context_apply_lookup(struct scriptshaping_context *context, unsigned int count,
        unsigned int *match_positions, unsigned int lookup_count, const UINT16 *lookup_records, unsigned int match_length)
{
    unsigned int i, j;
    int end, delta;

    if (!context->nesting_level_left)
        return TRUE;

    end = context->cur + match_length;

    for (i = 0; i < lookup_count; ++i)
    {
        unsigned int idx = GET_BE_WORD(lookup_records[i]);
        unsigned int orig_len, lookup_index, next;

        if (idx >= count)
            continue;

        context->cur = match_positions[idx];

        orig_len = context->glyph_count;

        lookup_index = GET_BE_WORD(lookup_records[i+1]);

        --context->nesting_level_left;
        context->u.buffer.apply_context_lookup(context, lookup_index);
        ++context->nesting_level_left;

        delta = context->glyph_count - orig_len;
        if (!delta)
            continue;

        end += delta;
        if (end <= (int)match_positions[idx])
        {
            end = match_positions[idx];
            break;
        }

        next = idx + 1;

        if (delta > 0)
        {
            if (delta + count > GLYPH_CONTEXT_MAX_LENGTH)
                break;
        }
        else
        {
            delta = max(delta, (int)next - (int)count);
            next -= delta;
        }

        memmove(match_positions + next + delta, match_positions + next,
                (count - next) * sizeof (*match_positions));
        next += delta;
        count += delta;

        for (j = idx + 1; j < next; j++)
            match_positions[j] = match_positions[j - 1] + 1;

        for (; next < count; next++)
            match_positions[next] += delta;
    }

    context->cur = end;

    return TRUE;
}

static BOOL opentype_layout_apply_chain_context_match(unsigned int backtrack_count, const UINT16 *backtrack,
        unsigned int input_count, const UINT16 *input, unsigned int lookahead_count, const UINT16 *lookahead,
        unsigned int lookup_count, const UINT16 *lookup_records, const struct match_context *mc)
{
    unsigned int start_index = 0, match_length = 0, end_index = 0;
    unsigned int match_positions[GLYPH_CONTEXT_MAX_LENGTH];

    return opentype_layout_context_match_input(mc, input_count, input, &match_length, match_positions) &&
            opentype_layout_context_match_backtrack(mc, backtrack_count, backtrack, &start_index) &&
            opentype_layout_context_match_lookahead(mc, lookahead_count, lookahead, input_count, &end_index) &&
            opentype_layout_context_apply_lookup(mc->context, input_count, match_positions, lookup_count, lookup_records, match_length);
}

static BOOL opentype_layout_apply_chain_rule_set(const struct match_context *mc, unsigned int offset)
{
    unsigned int backtrack_count, input_count, lookahead_count, lookup_count;
    const struct dwrite_fonttable *table = &mc->context->table->table;
    const UINT16 *backtrack, *lookahead, *input, *lookup_records;
    const struct ot_gsubgpos_ruleset *ruleset;
    unsigned int i, count;

    count = table_read_be_word(table, offset);
    ruleset = table_read_ensure(table, offset, count * sizeof(ruleset->offsets));

    for (i = 0; i < count; ++i)
    {
        unsigned int rule_offset = offset + GET_BE_WORD(ruleset->offsets[i]);

        backtrack_count = table_read_be_word(table, rule_offset);
        rule_offset += 2;
        backtrack = table_read_ensure(table, rule_offset, backtrack_count * sizeof(*backtrack));
        rule_offset += backtrack_count * sizeof(*backtrack);

        if (!(input_count = table_read_be_word(table, rule_offset)))
            continue;

        rule_offset += 2;
        input = table_read_ensure(table, rule_offset, (input_count - 1) * sizeof(*input));
        rule_offset += (input_count - 1) * sizeof(*input);

        lookahead_count = table_read_be_word(table, rule_offset);
        rule_offset += 2;
        lookahead = table_read_ensure(table, rule_offset, lookahead_count * sizeof(*lookahead));
        rule_offset += lookahead_count * sizeof(*lookahead);

        lookup_count = table_read_be_word(table, rule_offset);
        rule_offset += 2;
        lookup_records = table_read_ensure(table, rule_offset, lookup_count * 2 * sizeof(*lookup_records));

        /* First applicable rule is used. */
        if (opentype_layout_apply_chain_context_match(backtrack_count, backtrack, input_count, input, lookahead_count,
                lookahead, lookup_count, lookup_records, mc))
        {
            return TRUE;
        }
    }

    return FALSE;
}

static BOOL opentype_layout_apply_context_match(unsigned int input_count, const UINT16 *input, unsigned int lookup_count,
        const UINT16 *lookup_records, const struct match_context *mc)
{
    unsigned int match_positions[GLYPH_CONTEXT_MAX_LENGTH];
    unsigned int match_length = 0;

    return opentype_layout_context_match_input(mc, input_count, input, &match_length, match_positions) &&
            opentype_layout_context_apply_lookup(mc->context, input_count, match_positions, lookup_count,
                    lookup_records, match_length);
}

static BOOL opentype_layout_apply_rule_set(const struct match_context *mc, unsigned int offset)
{
    unsigned int input_count, lookup_count;
    const struct dwrite_fonttable *table = &mc->context->table->table;
    const UINT16 *input, *lookup_records;
    const struct ot_gsubgpos_ruleset *ruleset;
    unsigned int i, count;

    count = table_read_be_word(table, offset);
    ruleset = table_read_ensure(table, offset, count * sizeof(ruleset->offsets));

    for (i = 0; i < count; ++i)
    {
        unsigned int rule_offset = offset + GET_BE_WORD(ruleset->offsets[i]);

        if (!(input_count = table_read_be_word(table, rule_offset)))
            continue;
        rule_offset += 2;

        if (!(lookup_count = table_read_be_word(table, rule_offset)))
            continue;
        rule_offset += 2;

        if (!(input = table_read_ensure(table, rule_offset, (input_count - 1) * sizeof(*input))))
            continue;
        rule_offset += (input_count - 1) * sizeof(*input);

        if (!(lookup_records = table_read_ensure(table, rule_offset, lookup_count * 2 * sizeof(*lookup_records))))
            continue;

        /* First applicable rule is used. */
        if (opentype_layout_apply_context_match(input_count, input, lookup_count, lookup_records, mc))
            return TRUE;
    }

    return FALSE;
}

static BOOL opentype_layout_apply_context(struct scriptshaping_context *context, const struct lookup *lookup,
        unsigned int subtable_offset)
{
    struct match_context mc = { .context = context, .lookup = lookup };
    const struct dwrite_fonttable *table = &context->table->table;
    unsigned int coverage_index = GLYPH_NOT_COVERED, count, offset;
    UINT16 glyph, format, coverage;
    BOOL ret = FALSE;

    glyph = context->u.subst.glyphs[context->cur];

    format = table_read_be_word(table, subtable_offset);

    if (format == 1)
    {
        coverage = table_read_be_word(table, subtable_offset + FIELD_OFFSET(struct ot_gsubgpos_context_format1, coverage));

        coverage_index = opentype_layout_is_glyph_covered(table, subtable_offset + coverage, glyph);
        if (coverage_index == GLYPH_NOT_COVERED)
            return FALSE;

        count = table_read_be_word(table, subtable_offset + FIELD_OFFSET(struct ot_gsubgpos_context_format1, ruleset_count));
        if (coverage_index >= count)
            return FALSE;

        offset = table_read_be_word(table, subtable_offset + FIELD_OFFSET(struct ot_gsubgpos_context_format1,
                rulesets[coverage_index]));
        offset += subtable_offset;

        mc.match_func = opentype_match_glyph_func;

        ret = opentype_layout_apply_rule_set(&mc, offset);
    }
    else if (format == 2)
    {
        unsigned int input_classdef, rule_set_idx;

        offset = subtable_offset + 2 /* format */;

        coverage = table_read_be_word(table, offset);
        offset += 2;

        coverage_index = opentype_layout_is_glyph_covered(table, subtable_offset + coverage, glyph);
        if (coverage_index == GLYPH_NOT_COVERED)
            return FALSE;

        input_classdef = table_read_be_word(table, offset) + subtable_offset;
        offset += 2;

        count = table_read_be_word(table, offset);
        offset+= 2;

        rule_set_idx = opentype_layout_get_glyph_class(table, input_classdef, glyph);
        if (rule_set_idx >= count)
            return FALSE;

        offset = table_read_be_word(table, offset + rule_set_idx * 2);
        offset += subtable_offset;

        mc.input_offset = input_classdef;
        mc.match_func = opentype_match_class_func;

        ret = opentype_layout_apply_rule_set(&mc, offset);
    }
    else if (format == 3)
    {
        unsigned int input_count, lookup_count;
        const UINT16 *input, *lookup_records;

        offset = subtable_offset + 2 /* format */;

        input_count = table_read_be_word(table, offset);
        offset += 2;

        if (!input_count)
            return FALSE;

        lookup_count = table_read_be_word(table, offset);
        offset += 2;

        if (!(input = table_read_ensure(table, offset, sizeof(*input) * input_count)))
            return FALSE;
        offset += sizeof(*input) * input_count;

        coverage_index = opentype_layout_is_glyph_covered(table, subtable_offset + GET_BE_WORD(input[0]), glyph);
        if (coverage_index == GLYPH_NOT_COVERED)
            return FALSE;

        lookup_records = table_read_ensure(table, offset, lookup_count * 2 * sizeof(*lookup_records));

        mc.input_offset = subtable_offset;
        mc.match_func = opentype_match_coverage_func;

        ret = opentype_layout_apply_context_match(input_count, input + 1, lookup_count, lookup_records, &mc);
    }
    else
        WARN("Unknown contextual substitution format %u.\n", format);

    return ret;
}

static BOOL opentype_layout_apply_chain_context(struct scriptshaping_context *context, const struct lookup *lookup,
        unsigned int subtable_offset)
{
    struct match_context mc = { .context = context, .lookup = lookup };
    const struct dwrite_fonttable *table = &context->table->table;
    unsigned int coverage_index = GLYPH_NOT_COVERED, count, offset;
    UINT16 glyph, format, coverage;
    BOOL ret = FALSE;

    glyph = context->u.subst.glyphs[context->cur];

    format = table_read_be_word(table, subtable_offset);

    if (format == 1)
    {
        coverage = table_read_be_word(table, subtable_offset + FIELD_OFFSET(struct ot_gsubgpos_context_format1, coverage));

        coverage_index = opentype_layout_is_glyph_covered(table, subtable_offset + coverage, glyph);
        if (coverage_index == GLYPH_NOT_COVERED)
            return FALSE;

        count = table_read_be_word(table, subtable_offset + FIELD_OFFSET(struct ot_gsubgpos_context_format1, ruleset_count));
        if (coverage_index >= count)
            return FALSE;

        offset = table_read_be_word(table, subtable_offset + FIELD_OFFSET(struct ot_gsubgpos_context_format1,
                rulesets[coverage_index]));
        offset += subtable_offset;

        mc.match_func = opentype_match_glyph_func;

        ret = opentype_layout_apply_chain_rule_set(&mc, offset);
    }
    else if (format == 2)
    {
        unsigned int backtrack_classdef, input_classdef, lookahead_classdef, rule_set_idx;

        offset = subtable_offset + 2 /* format */;

        coverage = table_read_be_word(table, offset);
        offset += 2;

        coverage_index = opentype_layout_is_glyph_covered(table, subtable_offset + coverage, glyph);
        if (coverage_index == GLYPH_NOT_COVERED)
            return FALSE;

        backtrack_classdef = table_read_be_word(table, offset) + subtable_offset;
        offset += 2;

        input_classdef = table_read_be_word(table, offset) + subtable_offset;
        offset += 2;

        lookahead_classdef = table_read_be_word(table, offset) + subtable_offset;
        offset += 2;

        count = table_read_be_word(table, offset);
        offset+= 2;

        rule_set_idx = opentype_layout_get_glyph_class(table, input_classdef, glyph);
        if (rule_set_idx >= count)
            return FALSE;

        offset = table_read_be_word(table, offset + rule_set_idx * 2);
        offset += subtable_offset;

        mc.backtrack_offset = backtrack_classdef;
        mc.input_offset = input_classdef;
        mc.lookahead_offset = lookahead_classdef;
        mc.match_func = opentype_match_class_func;

        ret = opentype_layout_apply_chain_rule_set(&mc, offset);
    }
    else if (format == 3)
    {
        unsigned int backtrack_count, input_count, lookahead_count, lookup_count;
        const UINT16 *backtrack, *lookahead, *input, *lookup_records;

        offset = subtable_offset + 2 /* format */;

        backtrack_count = table_read_be_word(table, offset);
        offset += 2;
        backtrack = table_read_ensure(table, offset, backtrack_count * sizeof(*backtrack));
        offset += backtrack_count * sizeof(*backtrack);

        input_count = table_read_be_word(table, offset);
        offset += 2;
        input = table_read_ensure(table, offset, input_count * sizeof(*input));
        offset += input_count * sizeof(*input);

        lookahead_count = table_read_be_word(table, offset);
        offset += 2;
        lookahead = table_read_ensure(table, offset, lookahead_count * sizeof(*lookahead));
        offset += lookahead_count * sizeof(*lookahead);

        lookup_count = table_read_be_word(table, offset);
        offset += 2;
        lookup_records = table_read_ensure(table, offset, lookup_count * 2 * sizeof(*lookup_records));

        if (input)
            coverage_index = opentype_layout_is_glyph_covered(table, subtable_offset + GET_BE_WORD(input[0]), glyph);

        if (coverage_index == GLYPH_NOT_COVERED)
            return FALSE;

        mc.backtrack_offset = subtable_offset;
        mc.input_offset = subtable_offset;
        mc.lookahead_offset = subtable_offset;
        mc.match_func = opentype_match_coverage_func;

        ret = opentype_layout_apply_chain_context_match(backtrack_count, backtrack, input_count, input + 1, lookahead_count,
                lookahead, lookup_count, lookup_records, &mc);
    }
    else
        WARN("Unknown chaining contextual substitution format %u.\n", format);

    return ret;
}

static BOOL opentype_layout_apply_gsub_reverse_chain_context_substitution(struct scriptshaping_context *context,
        const struct lookup *lookup, unsigned int subtable_offset)
{
    const struct dwrite_fonttable *table = &context->table->table;
    unsigned int offset = subtable_offset;
    UINT16 glyph, format;

    if (context->nesting_level_left != SHAPE_MAX_NESTING_LEVEL)
        return FALSE;

    glyph = context->u.subst.glyphs[context->cur];

    format = table_read_be_word(table, offset);
    offset += 2;

    if (format == 1)
    {
        struct match_context mc = { .context = context, .lookup = lookup };
        unsigned int start_index = 0, end_index = 0, backtrack_count, lookahead_count;
        unsigned int coverage, coverage_index;
        const UINT16 *backtrack, *lookahead;

        coverage = table_read_be_word(table, offset);
        offset += 2;

        coverage_index = opentype_layout_is_glyph_covered(table, subtable_offset + coverage, glyph);
        if (coverage_index == GLYPH_NOT_COVERED)
            return FALSE;

        backtrack_count = table_read_be_word(table, offset);
        offset += 2;

        backtrack = table_read_ensure(table, offset, sizeof(*backtrack) * backtrack_count);
        offset += sizeof(*backtrack) * backtrack_count;

        lookahead_count = table_read_be_word(table, offset);
        offset += 2;

        lookahead = table_read_ensure(table, offset, sizeof(*lookahead) * lookahead_count);
        offset += sizeof(*lookahead) * lookahead_count;

        mc.match_func = opentype_match_coverage_func;
        mc.backtrack_offset = subtable_offset;
        mc.lookahead_offset = subtable_offset;

        if (opentype_layout_context_match_backtrack(&mc, backtrack_count, backtrack, &start_index) &&
                opentype_layout_context_match_lookahead(&mc, lookahead_count, lookahead, 1, &end_index))
        {
            unsigned int glyph_count = table_read_be_word(table, offset);
            if (coverage_index >= glyph_count)
                return FALSE;
            offset += 2;

            glyph = table_read_be_word(table, offset + coverage_index * sizeof(glyph));
            opentype_layout_replace_glyph(context, glyph);

            return TRUE;
        }
    }
    else
        WARN("Unknown reverse chaining contextual substitution format %u.\n", format);

    return FALSE;
}

static BOOL opentype_layout_apply_gsub_lookup(struct scriptshaping_context *context, const struct lookup *lookup)
{
    unsigned int i, lookup_type;
    BOOL ret = FALSE;

    for (i = 0; i < lookup->subtable_count; ++i)
    {
        unsigned int subtable_offset = opentype_layout_get_gsubgpos_subtable(context, lookup, i, &lookup_type);

        switch (lookup_type)
        {
            case GSUB_LOOKUP_SINGLE_SUBST:
                ret = opentype_layout_apply_gsub_single_substitution(context, lookup, subtable_offset);
                break;
            case GSUB_LOOKUP_MULTIPLE_SUBST:
                ret = opentype_layout_apply_gsub_mult_substitution(context, lookup, subtable_offset);
                break;
            case GSUB_LOOKUP_ALTERNATE_SUBST:
                ret = opentype_layout_apply_gsub_alt_substitution(context, lookup, subtable_offset);
                break;
            case GSUB_LOOKUP_LIGATURE_SUBST:
                ret = opentype_layout_apply_gsub_lig_substitution(context, lookup, subtable_offset);
                break;
            case GSUB_LOOKUP_CONTEXTUAL_SUBST:
                ret = opentype_layout_apply_context(context, lookup, subtable_offset);
                break;
            case GSUB_LOOKUP_CHAINING_CONTEXTUAL_SUBST:
                ret = opentype_layout_apply_chain_context(context, lookup, subtable_offset);
                break;
            case GSUB_LOOKUP_REVERSE_CHAINING_CONTEXTUAL_SUBST:
                ret = opentype_layout_apply_gsub_reverse_chain_context_substitution(context, lookup, subtable_offset);
                break;
            case GSUB_LOOKUP_EXTENSION_SUBST:
                WARN("Invalid lookup type for extension substitution %#x.\n", lookup_type);
                break;
            default:
                WARN("Unknown lookup type %u.\n", lookup_type);
        }

        if (ret)
            break;
    }

    return ret;
}

static unsigned int unicode_get_mirrored_char(unsigned int codepoint)
{
    extern const WCHAR wine_mirror_map[];
    WCHAR mirror;
    /* TODO: check if mirroring for higher planes makes sense at all */
    if (codepoint > 0xffff) return codepoint;
    mirror = get_table_entry_16(wine_mirror_map, codepoint);
    return mirror ? mirror : codepoint;
}

/*
     * 034F          # Mn       COMBINING GRAPHEME JOINER
     * 061C          # Cf       ARABIC LETTER MARK
     * 180B..180D    # Mn   [3] MONGOLIAN FREE VARIATION SELECTOR ONE..MONGOLIAN FREE VARIATION SELECTOR THREE
     * 180E          # Cf       MONGOLIAN VOWEL SEPARATOR
     * 200B..200F    # Cf   [5] ZERO WIDTH SPACE..RIGHT-TO-LEFT MARK
     * FEFF          # Cf       ZERO WIDTH NO-BREAK SPACE
*/
static unsigned int opentype_is_zero_width(unsigned int codepoint)
{
    return codepoint == 0x34f || codepoint == 0x61c || codepoint == 0xfeff ||
            (codepoint >= 0x180b && codepoint <= 0x180e) || (codepoint >= 0x200b && codepoint <= 0x200f);
}

/*
    * 00AD          # Cf       SOFT HYPHEN
    * 034F          # Mn       COMBINING GRAPHEME JOINER
    * 061C          # Cf       ARABIC LETTER MARK
    * 115F..1160    # Lo   [2] HANGUL CHOSEONG FILLER..HANGUL JUNGSEONG FILLER
    * 17B4..17B5    # Mn   [2] KHMER VOWEL INHERENT AQ..KHMER VOWEL INHERENT AA
    * 180B..180D    # Mn   [3] MONGOLIAN FREE VARIATION SELECTOR ONE..MONGOLIAN FREE VARIATION SELECTOR THREE
    * 180E          # Cf       MONGOLIAN VOWEL SEPARATOR
    * 200B..200F    # Cf   [5] ZERO WIDTH SPACE..RIGHT-TO-LEFT MARK
    * 202A..202E    # Cf   [5] LEFT-TO-RIGHT EMBEDDING..RIGHT-TO-LEFT OVERRIDE
    * 2060..2064    # Cf   [5] WORD JOINER..INVISIBLE PLUS
    * 2065          # Cn       <reserved-2065>
    * 2066..206F    # Cf  [10] LEFT-TO-RIGHT ISOLATE..NOMINAL DIGIT SHAPES
    * 3164          # Lo       HANGUL FILLER
    * FE00..FE0F    # Mn  [16] VARIATION SELECTOR-1..VARIATION SELECTOR-16
    * FEFF          # Cf       ZERO WIDTH NO-BREAK SPACE
    * FFA0          # Lo       HALFWIDTH HANGUL FILLER
    * FFF0..FFF8    # Cn   [9] <reserved-FFF0>..<reserved-FFF8>
    * 1BCA0..1BCA3  # Cf   [4] SHORTHAND FORMAT LETTER OVERLAP..SHORTHAND FORMAT UP STEP
    * 1D173..1D17A  # Cf   [8] MUSICAL SYMBOL BEGIN BEAM..MUSICAL SYMBOL END PHRASE
    * E0000         # Cn       <reserved-E0000>
    * E0001         # Cf       LANGUAGE TAG
    * E0002..E001F  # Cn  [30] <reserved-E0002>..<reserved-E001F>
    * E0020..E007F  # Cf  [96] TAG SPACE..CANCEL TAG
    * E0080..E00FF  # Cn [128] <reserved-E0080>..<reserved-E00FF>
    * E0100..E01EF  # Mn [240] VARIATION SELECTOR-17..VARIATION SELECTOR-256
    * E01F0..E0FFF  # Cn [3600] <reserved-E01F0>..<reserved-E0FFF>
*/
static unsigned int opentype_is_default_ignorable(unsigned int codepoint)
{
    if (codepoint < 0x80) return 0;
    return codepoint == 0xad ||
            codepoint == 0x34f ||
            codepoint == 0x61c ||
            (codepoint >= 0x17b4 && codepoint <= 0x17b5) ||
            (codepoint >= 0x180b && codepoint <= 0x180e) ||
            (codepoint >= 0x200b && codepoint <= 0x200f) ||
            (codepoint >= 0x202a && codepoint <= 0x202e) ||
            (codepoint >= 0x2060 && codepoint <= 0x206f) ||
            (codepoint >= 0xfe00 && codepoint <= 0xfe0f) ||
            codepoint == 0xfeff ||
            (codepoint >= 0xfff0 && codepoint <= 0xfff8) ||
            (codepoint >= 0x1d173 && codepoint <= 0x1d17a) ||
            (codepoint >= 0xe0000 && codepoint <= 0xe0fff);
}

static unsigned int opentype_is_diacritic(unsigned int codepoint)
{
    WCHAR ch = codepoint;
    WORD type = 0;
    /* Ignore higher planes for now. */
    if (codepoint > 0xffff) return 0;
    GetStringTypeW(CT_CTYPE3, &ch, 1, &type);
    return !!(type & C3_DIACRITIC);
}

static void opentype_get_nominal_glyphs(struct scriptshaping_context *context, const struct shaping_features *features)
{
    unsigned int rtlm_mask = shaping_features_get_mask(features, DWRITE_MAKE_OPENTYPE_TAG('r','t','l','m'), NULL);
    const struct shaping_font_ops *font = context->cache->font;
    unsigned int i, g, c, codepoint, cluster_start_idx = 0;
    UINT16 *clustermap = context->u.subst.clustermap;
    const WCHAR *text = context->text;
    BOOL bmp;

    memset(context->u.subst.glyph_props, 0, context->u.subst.max_glyph_count * sizeof(*context->u.subst.glyph_props));
    memset(context->u.buffer.text_props, 0, context->length * sizeof(*context->u.buffer.text_props));

    for (i = 0; i < context->length; ++i)
    {
        g = context->glyph_count;

        if ((bmp = !(IS_HIGH_SURROGATE(text[i]) && (i < context->length - 1) && IS_LOW_SURROGATE(text[i + 1]))))
        {
            codepoint = text[i];
        }
        else
        {
            codepoint = 0x10000 + ((text[i] - 0xd800) << 10) + (text[i + 1] - 0xdc00);
        }

        if (context->is_rtl)
        {
            c = unicode_get_mirrored_char(codepoint);
            if (c != codepoint && font->has_glyph(context->cache->context, c))
                codepoint = c;
            else
                context->glyph_infos[i].mask |= rtlm_mask;
        }

        /* Glyph availability is not tested for a replacement digit. */
        if (*context->u.subst.digits && codepoint >= '0' && codepoint <= '9')
            codepoint = context->u.subst.digits[codepoint - '0'];

        context->glyph_infos[g].codepoint = codepoint;
        context->u.buffer.glyphs[g] = font->get_glyph(context->cache->context, codepoint);
        context->u.buffer.glyph_props[g].justification = SCRIPT_JUSTIFY_CHARACTER;
        opentype_set_subst_glyph_props(context, g);
        if (opentype_is_default_ignorable(codepoint))
        {
            context->glyph_infos[g].props |= GLYPH_PROP_IGNORABLE;
            if (codepoint == 0x200d)
                context->glyph_infos[g].props |= GLYPH_PROP_ZWJ;
            else if (codepoint == 0x200c)
                context->glyph_infos[g].props |= GLYPH_PROP_ZWNJ;
            /* Mongolian FVSs, TAGs, COMBINING GRAPHEME JOINER */
            else if ((codepoint >= 0x180b && codepoint <= 0x180d) ||
                    (codepoint >= 0xe0020 && codepoint <= 0xe007f) ||
                    codepoint == 0x34f)
            {
                context->glyph_infos[g].props |= GLYPH_PROP_HIDDEN;
            }
        }

        /* Group diacritics with preceding base. Glyph class is ignored here. */
        if (!g || !opentype_is_diacritic(codepoint))
        {
            context->u.buffer.glyph_props[g].isClusterStart = 1;
            context->glyph_infos[g].start_text_idx = i;
            cluster_start_idx = g;
        }
        if (opentype_is_zero_width(codepoint))
            context->u.buffer.glyph_props[g].isZeroWidthSpace = 1;

        context->u.buffer.glyph_props[g].components = 1;
        context->glyph_count++;

        /* Set initial cluster map here, it's used for setting user features masks. */
        clustermap[i] = cluster_start_idx;
        if (bmp)
            context->u.buffer.text_props[i].canBreakShapingAfter = 1;
        else
        {
            clustermap[i + 1] = cluster_start_idx;
            context->u.buffer.text_props[i + 1].canBreakShapingAfter = 1;
            ++i;
        }
    }
}

static BOOL opentype_is_gsub_lookup_reversed(const struct scriptshaping_context *context, const struct lookup *lookup)
{
    unsigned int lookup_type;

    opentype_layout_get_gsubgpos_subtable(context, lookup, 0, &lookup_type);
    return lookup_type == GSUB_LOOKUP_REVERSE_CHAINING_CONTEXTUAL_SUBST;
}

static void opentype_layout_apply_gsub_context_lookup(struct scriptshaping_context *context, unsigned int lookup_index)
{
    struct lookup lookup = { 0 };
    if (opentype_layout_init_lookup(context->table, lookup_index, NULL, &lookup))
        opentype_layout_apply_gsub_lookup(context, &lookup);
}

void opentype_layout_apply_gsub_features(struct scriptshaping_context *context, unsigned int script_index,
        unsigned int language_index, struct shaping_features *features)
{
    struct lookups lookups = { 0 };
    unsigned int i = 0, j, start_idx;
    BOOL ret;

    context->nesting_level_left = SHAPE_MAX_NESTING_LEVEL;
    context->u.buffer.apply_context_lookup = opentype_layout_apply_gsub_context_lookup;
    opentype_layout_collect_lookups(context, script_index, language_index, features, context->table, &lookups);

    opentype_get_nominal_glyphs(context, features);
    opentype_layout_set_glyph_masks(context, features);

    for (j = 0; j <= features->stage; ++j)
    {
        for (; i < features->stages[j].last_lookup; ++i)
        {
            const struct lookup *lookup = &lookups.lookups[i];

            context->lookup_mask = lookup->mask;
            context->auto_zwnj = lookup->auto_zwnj;
            context->auto_zwj = lookup->auto_zwj;

            if (!opentype_is_gsub_lookup_reversed(context, lookup))
            {
                context->cur = 0;
                while (context->cur < context->glyph_count)
                {
                    ret = FALSE;

                    if ((context->glyph_infos[context->cur].mask & lookup->mask) &&
                            lookup_is_glyph_match(context, context->cur, lookup->flags))
                    {
                        ret = opentype_layout_apply_gsub_lookup(context, lookup);
                    }

                    if (!ret)
                        context->cur++;
                }
            }
            else
            {
                context->cur = context->glyph_count - 1;

                for (;;)
                {
                    if ((context->glyph_infos[context->cur].mask & lookup->mask) &&
                            lookup_is_glyph_match(context, context->cur, lookup->flags))
                    {
                        opentype_layout_apply_gsub_lookup(context, lookup);
                    }

                    if (context->cur == 0) break;
                    --context->cur;
                }
            }
        }

        if (features->stages[j].func)
            features->stages[j].func(context, features);
    }

    /* For every glyph range of [<last>.isClusterStart, <next>.isClusterStart) set corresponding
       text span to start_idx. */
    start_idx = 0;
    for (i = 1; i < context->glyph_count; ++i)
    {
        if (context->u.buffer.glyph_props[i].isClusterStart)
        {
            unsigned int start_text, end_text;

            start_text = context->glyph_infos[start_idx].start_text_idx;
            end_text = context->glyph_infos[i].start_text_idx;

            for (j = start_text; j < end_text; ++j)
                context->u.buffer.clustermap[j] = start_idx;

            start_idx = i;
        }
    }

    /* Fill the tail. */
    for (j = context->glyph_infos[start_idx].start_text_idx; j < context->length; ++j)
        context->u.buffer.clustermap[j] = start_idx;

    free(lookups.lookups);
}

static BOOL opentype_layout_contextual_lookup_is_glyph_covered(struct scriptshaping_context *context, UINT16 glyph,
        unsigned int subtable_offset, unsigned int coverage, unsigned int format)
{
    const struct dwrite_fonttable *table = &context->table->table;
    const UINT16 *offsets;
    unsigned int count;

    if (format == 1 || format == 2)
    {
        if (opentype_layout_is_glyph_covered(table, subtable_offset + coverage, glyph) != GLYPH_NOT_COVERED)
            return TRUE;
    }
    else if (format == 3)
    {
        count = table_read_be_word(table, subtable_offset + 2);
        if (!count || !(offsets = table_read_ensure(table, subtable_offset + 6, count * sizeof(*offsets))))
            return FALSE;

        if (opentype_layout_is_glyph_covered(table, subtable_offset + GET_BE_WORD(offsets[0]), glyph) != GLYPH_NOT_COVERED)
            return TRUE;
    }

    return FALSE;
}

static BOOL opentype_layout_chain_contextual_lookup_is_glyph_covered(struct scriptshaping_context *context, UINT16 glyph,
        unsigned int subtable_offset, unsigned int coverage, unsigned int format)
{
    const struct dwrite_fonttable *table = &context->table->table;
    unsigned int count, backtrack_count;
    const UINT16 *offsets;

    if (format == 1 || format == 2)
    {
        if (opentype_layout_is_glyph_covered(table, subtable_offset + coverage, glyph) != GLYPH_NOT_COVERED)
            return TRUE;
    }
    else if (format == 3)
    {
        backtrack_count = table_read_be_word(table, subtable_offset + 2);

        count = table_read_be_word(table, subtable_offset + 4 + backtrack_count * sizeof(*offsets));

        if (!count || !(offsets = table_read_ensure(table, subtable_offset + 6 + backtrack_count * sizeof(*offsets),
                count * sizeof(*offsets))))
            return FALSE;

        if (opentype_layout_is_glyph_covered(table, subtable_offset + GET_BE_WORD(offsets[0]), glyph) != GLYPH_NOT_COVERED)
            return TRUE;
    }

    return FALSE;
}

static BOOL opentype_layout_gsub_lookup_is_glyph_covered(struct scriptshaping_context *context, UINT16 glyph,
        const struct lookup *lookup)
{
    const struct dwrite_fonttable *gsub = &context->table->table;
    static const unsigned short gsub_formats[] =
    {
        0, /* Unused  */
        1, /* SingleSubst */
        1, /* MultipleSubst */
        1, /* AlternateSubst */
        1, /* LigatureSubst */
        3, /* ContextSubst */
        3, /* ChainContextSubst */
        0, /* Extension, unused */
        1, /* ReverseChainSubst */
    };
    unsigned int i, coverage, lookup_type, format;

    for (i = 0; i < lookup->subtable_count; ++i)
    {
        unsigned int subtable_offset = opentype_layout_get_gsubgpos_subtable(context, lookup, i, &lookup_type);

        format = table_read_be_word(gsub, subtable_offset);

        if (!format || format > ARRAY_SIZE(gsub_formats) || format > gsub_formats[lookup_type])
            break;

        coverage = table_read_be_word(gsub, subtable_offset + 2);

        switch (lookup_type)
        {
            case GSUB_LOOKUP_SINGLE_SUBST:
            case GSUB_LOOKUP_MULTIPLE_SUBST:
            case GSUB_LOOKUP_ALTERNATE_SUBST:
            case GSUB_LOOKUP_LIGATURE_SUBST:
            case GSUB_LOOKUP_REVERSE_CHAINING_CONTEXTUAL_SUBST:

                if (opentype_layout_is_glyph_covered(gsub, subtable_offset + coverage, glyph) != GLYPH_NOT_COVERED)
                    return TRUE;

                break;

            case GSUB_LOOKUP_CONTEXTUAL_SUBST:

                if (opentype_layout_contextual_lookup_is_glyph_covered(context, glyph, subtable_offset, coverage, format))
                    return TRUE;

                break;

            case GSUB_LOOKUP_CHAINING_CONTEXTUAL_SUBST:

                if (opentype_layout_chain_contextual_lookup_is_glyph_covered(context, glyph, subtable_offset, coverage, format))
                    return TRUE;

                break;

            default:
                WARN("Unknown lookup type %u.\n", lookup_type);
        }
    }

    return FALSE;
}

static BOOL opentype_layout_gpos_lookup_is_glyph_covered(struct scriptshaping_context *context, UINT16 glyph,
        const struct lookup *lookup)
{
    const struct dwrite_fonttable *gpos = &context->table->table;
    static const unsigned short gpos_formats[] =
    {
        0, /* Unused  */
        2, /* SinglePos */
        2, /* PairPos */
        1, /* CursivePos */
        1, /* MarkBasePos */
        1, /* MarkLigPos */
        1, /* MarkMarkPos */
        3, /* ContextPos */
        3, /* ChainContextPos */
        0, /* Extension, unused */
    };
    unsigned int i, coverage, lookup_type, format;

    for (i = 0; i < lookup->subtable_count; ++i)
    {
        unsigned int subtable_offset = opentype_layout_get_gsubgpos_subtable(context, lookup, i, &lookup_type);

        format = table_read_be_word(gpos, subtable_offset);

        if (!format || format > ARRAY_SIZE(gpos_formats) || format > gpos_formats[lookup_type])
            break;

        coverage = table_read_be_word(gpos, subtable_offset + 2);

        switch (lookup_type)
        {
            case GPOS_LOOKUP_SINGLE_ADJUSTMENT:
            case GPOS_LOOKUP_PAIR_ADJUSTMENT:
            case GPOS_LOOKUP_CURSIVE_ATTACHMENT:
            case GPOS_LOOKUP_MARK_TO_BASE_ATTACHMENT:
            case GPOS_LOOKUP_MARK_TO_LIGATURE_ATTACHMENT:
            case GPOS_LOOKUP_MARK_TO_MARK_ATTACHMENT:

                if (opentype_layout_is_glyph_covered(gpos, subtable_offset + coverage, glyph) != GLYPH_NOT_COVERED)
                    return TRUE;

                break;

            case GPOS_LOOKUP_CONTEXTUAL_POSITION:

                if (opentype_layout_contextual_lookup_is_glyph_covered(context, glyph, subtable_offset, coverage, format))
                    return TRUE;

                break;

            case GPOS_LOOKUP_CONTEXTUAL_CHAINING_POSITION:

                if (opentype_layout_chain_contextual_lookup_is_glyph_covered(context, glyph, subtable_offset, coverage, format))
                    return TRUE;

                break;

            default:
                WARN("Unknown lookup type %u.\n", lookup_type);
        }
    }

    return FALSE;
}

typedef BOOL (*p_lookup_is_glyph_covered_func)(struct scriptshaping_context *context, UINT16 glyph, const struct lookup *lookup);

BOOL opentype_layout_check_feature(struct scriptshaping_context *context, unsigned int script_index,
        unsigned int language_index, struct shaping_feature *feature, unsigned int glyph_count,
        const UINT16 *glyphs, UINT8 *feature_applies)
{
    p_lookup_is_glyph_covered_func func_is_covered;
    struct shaping_features features = { 0 };
    struct lookups lookups = { 0 };
    BOOL ret = FALSE, is_covered;
    unsigned int i, j, applies;

    features.features = feature;
    features.count = 1;

    for (i = 0; i < context->glyph_count; ++i)
        opentype_set_glyph_props(context, i);

    opentype_layout_collect_lookups(context, script_index, language_index, &features, context->table, &lookups);

    func_is_covered = opentype_layout_is_subst_context(context) ? opentype_layout_gsub_lookup_is_glyph_covered :
            opentype_layout_gpos_lookup_is_glyph_covered;

    for (i = 0; i < lookups.count; ++i)
    {
        struct lookup *lookup = &lookups.lookups[i];

        applies = 0;
        for (j = 0; j < context->glyph_count; ++j)
        {
            if (lookup_is_glyph_match(context, j, lookup->flags))
            {
                if ((is_covered = func_is_covered(context, glyphs[i], lookup)))
                    ++applies;
                feature_applies[j] |= is_covered;
            }
        }

        if ((ret = (applies == context->glyph_count)))
            break;
    }

    free(lookups.lookups);

    return ret;
}

BOOL opentype_has_vertical_variants(struct dwrite_fontface *fontface)
{
    unsigned int i, j, count = 0, lookup_type, subtable_offset;
    struct shaping_features features = { 0 };
    struct shaping_feature vert_feature = { 0 };
    struct scriptshaping_context context = { 0 };
    struct lookups lookups = { 0 };
    UINT16 format;

    if (fontface->flags & (FONTFACE_VERTICAL_VARIANTS | FONTFACE_NO_VERTICAL_VARIANTS))
        return !!(fontface->flags & FONTFACE_VERTICAL_VARIANTS);

    context.cache = fontface_get_shaping_cache(fontface);
    context.table = &context.cache->gsub;

    vert_feature.tag = DWRITE_MAKE_OPENTYPE_TAG('v','e','r','t');
    vert_feature.flags = FEATURE_GLOBAL | FEATURE_GLOBAL_SEARCH;
    vert_feature.max_value = 1;
    vert_feature.default_value = 1;

    features.features = &vert_feature;
    features.count = features.capacity = 1;

    opentype_layout_collect_lookups(&context, ~0u, ~0u, &features, context.table, &lookups);

    for (i = 0; i < lookups.count && !count; ++i)
    {
        const struct dwrite_fonttable *table = &context.table->table;
        const struct lookup *lookup = &lookups.lookups[i];

        for (j = 0; j < lookup->subtable_count && !count; ++j)
        {
            subtable_offset = opentype_layout_get_gsubgpos_subtable(&context, lookup, j, &lookup_type);

            if (lookup_type != GSUB_LOOKUP_SINGLE_SUBST)
                continue;

            format = table_read_be_word(table, subtable_offset);

            if (format == 1)
            {
                count = 1;
            }
            else if (format == 2)
            {
                count = table_read_be_word(table, subtable_offset + FIELD_OFFSET(struct ot_gsub_singlesubst_format2, count));
            }
            else
                WARN("Unrecognized single substitution format %u.\n", format);
        }
    }

    free(lookups.lookups);

    if (count)
        fontface->flags |= FONTFACE_VERTICAL_VARIANTS;
    else
        fontface->flags |= FONTFACE_NO_VERTICAL_VARIANTS;

    return !!(fontface->flags & FONTFACE_VERTICAL_VARIANTS);
}

HRESULT opentype_get_vertical_glyph_variants(struct dwrite_fontface *fontface, unsigned int glyph_count,
        const UINT16 *nominal_glyphs, UINT16 *glyphs)
{
    struct shaping_features features = { 0 };
    struct shaping_feature vert_feature = { 0 };
    struct scriptshaping_context context = { 0 };
    struct lookups lookups = { 0 };
    unsigned int i;

    memcpy(glyphs, nominal_glyphs, glyph_count * sizeof(*glyphs));

    if (!opentype_has_vertical_variants(fontface))
        return S_OK;

    context.cache = fontface_get_shaping_cache(fontface);
    context.u.subst.glyphs = glyphs;
    context.u.subst.glyph_props = calloc(glyph_count, sizeof(*context.u.subst.glyph_props));
    context.u.subst.max_glyph_count = glyph_count;
    context.u.subst.capacity = glyph_count;
    context.glyph_infos = calloc(glyph_count, sizeof(*context.glyph_infos));
    context.table = &context.cache->gsub;

    vert_feature.tag = DWRITE_MAKE_OPENTYPE_TAG('v','e','r','t');
    vert_feature.flags = FEATURE_GLOBAL | FEATURE_GLOBAL_SEARCH;
    vert_feature.max_value = 1;
    vert_feature.default_value = 1;

    features.features = &vert_feature;
    features.count = features.capacity = 1;

    opentype_layout_collect_lookups(&context, ~0u, ~0u, &features, context.table, &lookups);
    opentype_layout_set_glyph_masks(&context, &features);

    for (i = 0; i < lookups.count; ++i)
    {
        const struct lookup *lookup = &lookups.lookups[i];

        context.cur = 0;
        while (context.cur < context.glyph_count)
        {
            BOOL ret = FALSE;

            if (lookup_is_glyph_match(&context, context.cur, lookup->flags))
                ret = opentype_layout_apply_gsub_lookup(&context, lookup);

            if (!ret)
                context.cur++;
        }
    }

    free(context.u.subst.glyph_props);
    free(context.glyph_infos);
    free(lookups.lookups);

    return S_OK;
}

BOOL opentype_has_kerning_pairs(struct dwrite_fontface *fontface)
{
    const struct kern_subtable_header *subtable;
    struct file_stream_desc stream_desc;
    const struct kern_header *header;
    unsigned int offset, count, i;

    if (fontface->flags & (FONTFACE_KERNING_PAIRS | FONTFACE_NO_KERNING_PAIRS))
        return !!(fontface->flags & FONTFACE_KERNING_PAIRS);

    fontface->flags |= FONTFACE_NO_KERNING_PAIRS;

    stream_desc.stream = fontface->stream;
    stream_desc.face_type = fontface->type;
    stream_desc.face_index = fontface->index;

    opentype_get_font_table(&stream_desc, MS_KERN_TAG, &fontface->kern);
    if (fontface->kern.exists)
    {
        if ((header = table_read_ensure(&fontface->kern, 0, sizeof(*header))))
        {
            count = GET_BE_WORD(header->table_count);
            offset = sizeof(*header);

            /* FreeType limits table count this way. */
            count = min(count, 32);

            /* Check for presence of format 0 subtable with horizontal coverage. */
            for (i = 0; i < count; ++i)
            {
                if (!(subtable = table_read_ensure(&fontface->kern, offset, sizeof(*subtable))))
                    break;

                if (subtable->version == 0 && GET_BE_WORD(subtable->coverage) & 1)
                {
                    fontface->flags &= ~FONTFACE_NO_KERNING_PAIRS;
                    fontface->flags |= FONTFACE_KERNING_PAIRS;
                    break;
                }

                offset += GET_BE_WORD(subtable->length);
            }
        }
    }

    if (fontface->flags & FONTFACE_NO_KERNING_PAIRS && fontface->kern.data)
        IDWriteFontFileStream_ReleaseFileFragment(stream_desc.stream, fontface->kern.context);

    return !!(fontface->flags & FONTFACE_KERNING_PAIRS);
}

struct kern_format0_compare_key
{
    UINT16 left;
    UINT16 right;
};

static int __cdecl kern_format0_compare(const void *a, const void *b)
{
    const struct kern_format0_compare_key *key = a;
    const WORD *data = b;
    UINT16 left = GET_BE_WORD(data[0]), right = GET_BE_WORD(data[1]);
    int ret;

    if ((ret = (int)key->left - (int)left)) return ret;
    if ((ret = (int)key->right - (int)right)) return ret;
    return 0;
}

HRESULT opentype_get_kerning_pairs(struct dwrite_fontface *fontface, unsigned int count,
        const UINT16 *glyphs, INT32 *values)
{
    const struct kern_subtable_header *subtable;
    unsigned int i, s, offset, pair_count, subtable_count;
    struct kern_format0_compare_key key;
    const struct kern_header *header;
    const WORD *data;

    if (!opentype_has_kerning_pairs(fontface))
    {
        memset(values, 0, count * sizeof(*values));
        return S_OK;
    }

    subtable_count = table_read_be_word(&fontface->kern, 2);
    subtable_count = min(subtable_count, 32);

    for (i = 0; i < count - 1; ++i)
    {
        offset = sizeof(*header);

        key.left = glyphs[i];
        key.right = glyphs[i + 1];
        values[i] = 0;

        for (s = 0; s < subtable_count; ++s)
        {
            if (!(subtable = table_read_ensure(&fontface->kern, offset, sizeof(*subtable))))
                break;

            if (subtable->version == 0 && GET_BE_WORD(subtable->coverage) & 1)
            {
                if ((data = table_read_ensure(&fontface->kern, offset, GET_BE_WORD(subtable->length))))
                {
                    /* Skip subtable header */
                    data += 3;
                    pair_count = GET_BE_WORD(*data);
                    data += 4;
                    /* Move to pair data */
                    if ((data = table_read_ensure(&fontface->kern, offset + 7 * sizeof(*data),
                            pair_count * 3 * sizeof(*data))))
                    {
                        if ((data = bsearch(&key, data, pair_count, 3 * sizeof(*data), kern_format0_compare)))
                        {
                            values[i] = (short)GET_BE_WORD(data[2]);
                            break;
                        }
                    }
                }
            }

            offset += GET_BE_WORD(subtable->length);
        }
    }
    values[count - 1] = 0;

    return S_OK;
}

static void opentype_font_var_add_static_axis(struct dwrite_var_axis **axis, unsigned int *axis_count,
        unsigned int tag, float value)
{
    struct dwrite_var_axis *entry = &(*axis)[(*axis_count)++];
    entry->tag = tag;
    entry->min_value = entry->max_value = entry->default_value = value;
    entry->attributes = 0;
}

HRESULT opentype_get_font_var_axis(const struct file_stream_desc *stream_desc, struct dwrite_var_axis **axis,
        unsigned int *axis_count)
{
    static const float width_axis_values[] =
    {
        0.0f, /* DWRITE_FONT_STRETCH_UNDEFINED */
        50.0f, /* DWRITE_FONT_STRETCH_ULTRA_CONDENSED */
        62.5f, /* DWRITE_FONT_STRETCH_EXTRA_CONDENSED */
        75.0f, /* DWRITE_FONT_STRETCH_CONDENSED */
        87.5f, /* DWRITE_FONT_STRETCH_SEMI_CONDENSED */
        100.0f, /* DWRITE_FONT_STRETCH_NORMAL */
        112.5f, /* DWRITE_FONT_STRETCH_SEMI_EXPANDED */
        125.0f, /* DWRITE_FONT_STRETCH_EXPANDED */
        150.0f, /* DWRITE_FONT_STRETCH_EXTRA_EXPANDED */
        200.0f, /* DWRITE_FONT_STRETCH_ULTRA_EXPANDED */
    };
    BOOL has_wght = FALSE, has_wdth = FALSE, has_slnt = FALSE, has_ital = FALSE;
    const struct var_axis_record *records;
    const struct fvar_header *header;
    unsigned int i, count, tag, size;
    struct dwrite_font_props props;
    struct dwrite_fonttable fvar;
    HRESULT hr = S_OK;

    *axis = NULL;
    *axis_count = 0;

    opentype_get_font_table(stream_desc, MS_FVAR_TAG, &fvar);

    if (!(header = table_read_ensure(&fvar, 0, sizeof(*header)))) goto done;
    if (!(GET_BE_WORD(header->major_version) == 1 && GET_BE_WORD(header->minor_version) == 0))
    {
        WARN("Unexpected fvar version.\n");
        goto done;
    }

    count = GET_BE_WORD(header->axis_count);
    size = GET_BE_WORD(header->axis_size);

    if (!count || size != sizeof(*records)) goto done;
    if (!(records = table_read_ensure(&fvar, GET_BE_WORD(header->axes_array_offset), size * count))) goto done;

    if (!(*axis = calloc(count + 4, sizeof(**axis))))
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    for (i = 0; i < count; ++i)
    {
        (*axis)[i].tag = tag = records[i].tag;
        (*axis)[i].default_value = GET_BE_FIXED(records[i].default_value);
        (*axis)[i].min_value = GET_BE_FIXED(records[i].min_value);
        (*axis)[i].max_value = GET_BE_FIXED(records[i].max_value);
        if (GET_BE_WORD(records[i].flags & 0x1))
            (*axis)[i].attributes |= DWRITE_FONT_AXIS_ATTRIBUTES_HIDDEN;
        /* FIXME: set DWRITE_FONT_AXIS_ATTRIBUTES_VARIABLE */

        if (tag == DWRITE_FONT_AXIS_TAG_WEIGHT) has_wght = TRUE;
        if (tag == DWRITE_FONT_AXIS_TAG_WIDTH) has_wdth = TRUE;
        if (tag == DWRITE_FONT_AXIS_TAG_SLANT) has_slnt = TRUE;
        if (tag == DWRITE_FONT_AXIS_TAG_ITALIC) has_ital = TRUE;
    }

    if (!has_wght || !has_wdth || !has_slnt || !has_ital)
    {
        opentype_get_font_properties(stream_desc, &props);
        if (!has_wght) opentype_font_var_add_static_axis(axis, &count, DWRITE_FONT_AXIS_TAG_WEIGHT, props.weight);
        if (!has_ital) opentype_font_var_add_static_axis(axis, &count, DWRITE_FONT_AXIS_TAG_ITALIC,
                props.style == DWRITE_FONT_STYLE_ITALIC ? 1.0f : 0.0f);
        if (!has_wdth) opentype_font_var_add_static_axis(axis, &count, DWRITE_FONT_AXIS_TAG_WIDTH,
                width_axis_values[props.stretch]);
        if (!has_slnt) opentype_font_var_add_static_axis(axis, &count, DWRITE_FONT_AXIS_TAG_SLANT, props.slant_angle);
    }

    *axis_count = count;

done:
    if (fvar.context)
        IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, fvar.context);

    return hr;
}
