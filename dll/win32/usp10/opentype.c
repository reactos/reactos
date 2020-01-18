/*
 * Opentype font interfaces for the Uniscribe Script Processor (usp10.dll)
 *
 * Copyright 2012 CodeWeavers, Aric Stewart
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
 *
 */
#include <stdarg.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "usp10.h"
#include "winternl.h"

#include "usp10_internal.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(uniscribe);

#ifdef WORDS_BIGENDIAN
#define GET_BE_WORD(x) (x)
#define GET_BE_DWORD(x) (x)
#else
#define GET_BE_WORD(x) RtlUshortByteSwap(x)
#define GET_BE_DWORD(x) RtlUlongByteSwap(x)
#endif

#define round(x) (((x) < 0) ? (int)((x) - 0.5) : (int)((x) + 0.5))

/* These are all structures needed for the cmap format 12 table */
#define CMAP_TAG MS_MAKE_TAG('c', 'm', 'a', 'p')

enum gpos_lookup_type
{
    GPOS_LOOKUP_ADJUST_SINGLE = 0x1,
    GPOS_LOOKUP_ADJUST_PAIR = 0x2,
    GPOS_LOOKUP_ATTACH_CURSIVE = 0x3,
    GPOS_LOOKUP_ATTACH_MARK_TO_BASE = 0x4,
    GPOS_LOOKUP_ATTACH_MARK_TO_LIGATURE = 0x5,
    GPOS_LOOKUP_ATTACH_MARK_TO_MARK = 0x6,
    GPOS_LOOKUP_POSITION_CONTEXT = 0x7,
    GPOS_LOOKUP_POSITION_CONTEXT_CHAINED = 0x8,
    GPOS_LOOKUP_POSITION_EXTENSION = 0x9,
};

enum gsub_lookup_type
{
    GSUB_LOOKUP_SINGLE = 0x1,
    GSUB_LOOKUP_MULTIPLE = 0x2,
    GSUB_LOOKUP_ALTERNATE = 0x3,
    GSUB_LOOKUP_LIGATURE = 0x4,
    GSUB_LOOKUP_CONTEXT = 0x5,
    GSUB_LOOKUP_CONTEXT_CHAINED = 0x6,
    GSUB_LOOKUP_EXTENSION = 0x7,
    GSUB_LOOKUP_CONTEXT_CHAINED_REVERSE = 0x8,
};

typedef struct {
    WORD platformID;
    WORD encodingID;
    DWORD offset;
} CMAP_EncodingRecord;

typedef struct {
    WORD version;
    WORD numTables;
    CMAP_EncodingRecord tables[1];
} CMAP_Header;

typedef struct {
    DWORD startCharCode;
    DWORD endCharCode;
    DWORD startGlyphID;
} CMAP_SegmentedCoverage_group;

typedef struct {
    WORD format;
    WORD reserved;
    DWORD length;
    DWORD language;
    DWORD nGroups;
    CMAP_SegmentedCoverage_group groups[1];
} CMAP_SegmentedCoverage;

/* These are all structures needed for the GDEF table */
enum {BaseGlyph=1, LigatureGlyph, MarkGlyph, ComponentGlyph};

typedef struct {
    DWORD Version;
    WORD GlyphClassDef;
    WORD AttachList;
    WORD LigCaretList;
    WORD MarkAttachClassDef;
} GDEF_Header;

typedef struct {
    WORD ClassFormat;
    WORD StartGlyph;
    WORD GlyphCount;
    WORD ClassValueArray[1];
} OT_ClassDefFormat1;

typedef struct {
    WORD Start;
    WORD End;
    WORD Class;
} OT_ClassRangeRecord;

typedef struct {
    WORD ClassFormat;
    WORD ClassRangeCount;
    OT_ClassRangeRecord ClassRangeRecord[1];
} OT_ClassDefFormat2;

/* These are all structures needed for the GSUB table */

typedef struct {
    DWORD version;
    WORD ScriptList;
    WORD FeatureList;
    WORD LookupList;
} GSUB_Header;

typedef struct {
    CHAR ScriptTag[4];
    WORD Script;
} OT_ScriptRecord;

typedef struct {
    WORD ScriptCount;
    OT_ScriptRecord ScriptRecord[1];
} OT_ScriptList;

typedef struct {
    CHAR LangSysTag[4];
    WORD LangSys;
} OT_LangSysRecord;

typedef struct {
    WORD DefaultLangSys;
    WORD LangSysCount;
    OT_LangSysRecord LangSysRecord[1];
} OT_Script;

typedef struct {
    WORD LookupOrder; /* Reserved */
    WORD ReqFeatureIndex;
    WORD FeatureCount;
    WORD FeatureIndex[1];
} OT_LangSys;

typedef struct {
    CHAR FeatureTag[4];
    WORD Feature;
} OT_FeatureRecord;

typedef struct {
    WORD FeatureCount;
    OT_FeatureRecord FeatureRecord[1];
} OT_FeatureList;

typedef struct {
    WORD FeatureParams; /* Reserved */
    WORD LookupCount;
    WORD LookupListIndex[1];
} OT_Feature;

typedef struct {
    WORD LookupCount;
    WORD Lookup[1];
} OT_LookupList;

typedef struct {
    WORD LookupType;
    WORD LookupFlag;
    WORD SubTableCount;
    WORD SubTable[1];
} OT_LookupTable;

typedef struct {
    WORD CoverageFormat;
    WORD GlyphCount;
    WORD GlyphArray[1];
} OT_CoverageFormat1;

typedef struct {
    WORD Start;
    WORD End;
    WORD StartCoverageIndex;
} OT_RangeRecord;

typedef struct {
    WORD CoverageFormat;
    WORD RangeCount;
    OT_RangeRecord RangeRecord[1];
} OT_CoverageFormat2;

typedef struct {
    WORD SubstFormat; /* = 1 */
    WORD Coverage;
    WORD DeltaGlyphID;
} GSUB_SingleSubstFormat1;

typedef struct {
    WORD SubstFormat; /* = 2 */
    WORD Coverage;
    WORD GlyphCount;
    WORD Substitute[1];
}GSUB_SingleSubstFormat2;

typedef struct {
    WORD SubstFormat; /* = 1 */
    WORD Coverage;
    WORD SequenceCount;
    WORD Sequence[1];
}GSUB_MultipleSubstFormat1;

typedef struct {
    WORD GlyphCount;
    WORD Substitute[1];
}GSUB_Sequence;

typedef struct {
    WORD SubstFormat; /* = 1 */
    WORD Coverage;
    WORD LigSetCount;
    WORD LigatureSet[1];
}GSUB_LigatureSubstFormat1;

typedef struct {
    WORD LigatureCount;
    WORD Ligature[1];
}GSUB_LigatureSet;

typedef struct{
    WORD LigGlyph;
    WORD CompCount;
    WORD Component[1];
}GSUB_Ligature;

typedef struct{
    WORD SequenceIndex;
    WORD LookupListIndex;

}GSUB_SubstLookupRecord;

typedef struct{
    WORD SubstFormat;
    WORD Coverage;
    WORD SubRuleSetCount;
    WORD SubRuleSet[1];
}GSUB_ContextSubstFormat1;

typedef struct{
    WORD SubRuleCount;
    WORD SubRule[1];
}GSUB_SubRuleSet;

typedef struct {
    WORD GlyphCount;
    WORD SubstCount;
    WORD Input[1];
}GSUB_SubRule_1;

typedef struct {
    GSUB_SubstLookupRecord SubstLookupRecord[1];
}GSUB_SubRule_2;

typedef struct {
    WORD SubstFormat;
    WORD Coverage;
    WORD ClassDef;
    WORD SubClassSetCnt;
    WORD SubClassSet[1];
}GSUB_ContextSubstFormat2;

typedef struct {
    WORD SubClassRuleCnt;
    WORD SubClassRule[1];
}GSUB_SubClassSet;

typedef struct {
    WORD GlyphCount;
    WORD SubstCount;
    WORD Class[1];
}GSUB_SubClassRule_1;

typedef struct {
    GSUB_SubstLookupRecord SubstLookupRecord[1];
}GSUB_SubClassRule_2;

typedef struct{
    WORD SubstFormat; /* = 1 */
    WORD Coverage;
    WORD ChainSubRuleSetCount;
    WORD ChainSubRuleSet[1];
}GSUB_ChainContextSubstFormat1;

typedef struct {
    WORD SubstFormat; /* = 2 */
    WORD Coverage;
    WORD BacktrackClassDef;
    WORD InputClassDef;
    WORD LookaheadClassDef;
    WORD ChainSubClassSetCnt;
    WORD ChainSubClassSet[1];
}GSUB_ChainContextSubstFormat2;

typedef struct {
    WORD ChainSubClassRuleCnt;
    WORD ChainSubClassRule[1];
}GSUB_ChainSubClassSet;

typedef struct {
    WORD BacktrackGlyphCount;
    WORD Backtrack[1];
}GSUB_ChainSubClassRule_1;

typedef struct {
    WORD InputGlyphCount;
    WORD Input[1];
}GSUB_ChainSubClassRule_2;

typedef struct {
    WORD LookaheadGlyphCount;
    WORD LookAhead[1];
}GSUB_ChainSubClassRule_3;

typedef struct {
    WORD SubstCount;
    GSUB_SubstLookupRecord SubstLookupRecord[1];
}GSUB_ChainSubClassRule_4;

typedef struct {
    WORD SubstFormat; /* = 3 */
    WORD BacktrackGlyphCount;
    WORD Coverage[1];
}GSUB_ChainContextSubstFormat3_1;

typedef struct{
    WORD InputGlyphCount;
    WORD Coverage[1];
}GSUB_ChainContextSubstFormat3_2;

typedef struct{
    WORD LookaheadGlyphCount;
    WORD Coverage[1];
}GSUB_ChainContextSubstFormat3_3;

typedef struct{
    WORD SubstCount;
    GSUB_SubstLookupRecord SubstLookupRecord[1];
}GSUB_ChainContextSubstFormat3_4;

typedef struct {
    WORD SubstFormat; /* = 1 */
    WORD Coverage;
    WORD AlternateSetCount;
    WORD AlternateSet[1];
} GSUB_AlternateSubstFormat1;

typedef struct{
    WORD GlyphCount;
    WORD Alternate[1];
} GSUB_AlternateSet;

typedef struct {
    WORD SubstFormat;
    WORD ExtensionLookupType;
    DWORD ExtensionOffset;
} GSUB_ExtensionPosFormat1;

/* These are all structures needed for the GPOS table */

typedef struct {
    DWORD version;
    WORD ScriptList;
    WORD FeatureList;
    WORD LookupList;
} GPOS_Header;

typedef struct {
    WORD StartSize;
    WORD EndSize;
    WORD DeltaFormat;
    WORD DeltaValue[1];
} OT_DeviceTable;

typedef struct {
    WORD AnchorFormat;
    WORD XCoordinate;
    WORD YCoordinate;
} GPOS_AnchorFormat1;

typedef struct {
    WORD AnchorFormat;
    WORD XCoordinate;
    WORD YCoordinate;
    WORD AnchorPoint;
} GPOS_AnchorFormat2;

typedef struct {
    WORD AnchorFormat;
    WORD XCoordinate;
    WORD YCoordinate;
    WORD XDeviceTable;
    WORD YDeviceTable;
} GPOS_AnchorFormat3;

typedef struct {
    WORD XPlacement;
    WORD YPlacement;
    WORD XAdvance;
    WORD YAdvance;
    WORD XPlaDevice;
    WORD YPlaDevice;
    WORD XAdvDevice;
    WORD YAdvDevice;
} GPOS_ValueRecord;

typedef struct {
    WORD PosFormat;
    WORD Coverage;
    WORD ValueFormat;
    WORD Value[1];
} GPOS_SinglePosFormat1;

typedef struct {
    WORD PosFormat;
    WORD Coverage;
    WORD ValueFormat;
    WORD ValueCount;
    WORD Value[1];
} GPOS_SinglePosFormat2;

typedef struct {
    WORD PosFormat;
    WORD Coverage;
    WORD ValueFormat1;
    WORD ValueFormat2;
    WORD PairSetCount;
    WORD PairSetOffset[1];
} GPOS_PairPosFormat1;

typedef struct {
    WORD PosFormat;
    WORD Coverage;
    WORD ValueFormat1;
    WORD ValueFormat2;
    WORD ClassDef1;
    WORD ClassDef2;
    WORD Class1Count;
    WORD Class2Count;
    WORD Class1Record[1];
} GPOS_PairPosFormat2;

typedef struct {
    WORD SecondGlyph;
    WORD Value1[1];
    WORD Value2[1];
} GPOS_PairValueRecord;

typedef struct {
    WORD PairValueCount;
    GPOS_PairValueRecord PairValueRecord[1];
} GPOS_PairSet;

typedef struct {
    WORD EntryAnchor;
    WORD ExitAnchor;
} GPOS_EntryExitRecord;

typedef struct {
    WORD PosFormat;
    WORD Coverage;
    WORD EntryExitCount;
    GPOS_EntryExitRecord EntryExitRecord[1];
} GPOS_CursivePosFormat1;

typedef struct {
    WORD PosFormat;
    WORD MarkCoverage;
    WORD BaseCoverage;
    WORD ClassCount;
    WORD MarkArray;
    WORD BaseArray;
} GPOS_MarkBasePosFormat1;

typedef struct {
    WORD BaseAnchor[1];
} GPOS_BaseRecord;

typedef struct {
    WORD BaseCount;
    GPOS_BaseRecord BaseRecord[1];
} GPOS_BaseArray;

typedef struct {
    WORD Class;
    WORD MarkAnchor;
} GPOS_MarkRecord;

typedef struct {
    WORD MarkCount;
    GPOS_MarkRecord MarkRecord[1];
} GPOS_MarkArray;

typedef struct {
    WORD PosFormat;
    WORD MarkCoverage;
    WORD LigatureCoverage;
    WORD ClassCount;
    WORD MarkArray;
    WORD LigatureArray;
} GPOS_MarkLigPosFormat1;

typedef struct {
    WORD LigatureCount;
    WORD LigatureAttach[1];
} GPOS_LigatureArray;

typedef struct {
    WORD LigatureAnchor[1];
} GPOS_ComponentRecord;

typedef struct {
    WORD ComponentCount;
    GPOS_ComponentRecord ComponentRecord[1];
} GPOS_LigatureAttach;

typedef struct {
    WORD PosFormat;
    WORD Mark1Coverage;
    WORD Mark2Coverage;
    WORD ClassCount;
    WORD Mark1Array;
    WORD Mark2Array;
} GPOS_MarkMarkPosFormat1;

typedef struct {
    WORD Mark2Anchor[1];
} GPOS_Mark2Record;

typedef struct {
    WORD Mark2Count;
    GPOS_Mark2Record Mark2Record[1];
} GPOS_Mark2Array;

typedef struct {
    WORD SequenceIndex;
    WORD LookupListIndex;
} GPOS_PosLookupRecord;

typedef struct {
    WORD PosFormat;
    WORD Coverage;
    WORD ClassDef;
    WORD PosClassSetCnt;
    WORD PosClassSet[1];
} GPOS_ContextPosFormat2;

typedef struct {
    WORD PosClassRuleCnt;
    WORD PosClassRule[1];
} GPOS_PosClassSet;

typedef struct {
    WORD GlyphCount;
    WORD PosCount;
    WORD Class[1];
} GPOS_PosClassRule_1;

typedef struct {
    GPOS_PosLookupRecord PosLookupRecord[1];
} GPOS_PosClassRule_2;

typedef struct {
    WORD PosFormat;
    WORD BacktrackGlyphCount;
    WORD Coverage[1];
} GPOS_ChainContextPosFormat3_1;

typedef struct {
    WORD InputGlyphCount;
    WORD Coverage[1];
} GPOS_ChainContextPosFormat3_2;

typedef struct {
    WORD LookaheadGlyphCount;
    WORD Coverage[1];
} GPOS_ChainContextPosFormat3_3;

typedef struct {
    WORD PosCount;
    GPOS_PosLookupRecord PosLookupRecord[1];
} GPOS_ChainContextPosFormat3_4;

typedef struct {
    WORD PosFormat;
    WORD ExtensionLookupType;
    DWORD ExtensionOffset;
} GPOS_ExtensionPosFormat1;

/**********
 * CMAP
 **********/

static VOID *load_CMAP_format12_table(HDC hdc, ScriptCache *psc)
{
    CMAP_Header *CMAP_Table = NULL;
    int length;
    int i;

    if (!psc->CMAP_Table)
    {
        length = GetFontData(hdc, CMAP_TAG , 0, NULL, 0);
        if (length != GDI_ERROR)
        {
            psc->CMAP_Table = heap_alloc(length);
            GetFontData(hdc, CMAP_TAG , 0, psc->CMAP_Table, length);
            TRACE("Loaded cmap table of %i bytes\n",length);
        }
        else
            return NULL;
    }

    CMAP_Table = psc->CMAP_Table;

    for (i = 0; i < GET_BE_WORD(CMAP_Table->numTables); i++)
    {
        if ( (GET_BE_WORD(CMAP_Table->tables[i].platformID) == 3) &&
             (GET_BE_WORD(CMAP_Table->tables[i].encodingID) == 10) )
        {
            CMAP_SegmentedCoverage *format = (CMAP_SegmentedCoverage*)(((BYTE*)CMAP_Table) + GET_BE_DWORD(CMAP_Table->tables[i].offset));
            if (GET_BE_WORD(format->format) == 12)
                return format;
        }
    }
    return NULL;
}

static int __cdecl compare_group(const void *a, const void* b)
{
    const DWORD *chr = a;
    const CMAP_SegmentedCoverage_group *group = b;

    if (*chr < GET_BE_DWORD(group->startCharCode))
        return -1;
    if (*chr > GET_BE_DWORD(group->endCharCode))
        return 1;
    return 0;
}

DWORD OpenType_CMAP_GetGlyphIndex(HDC hdc, ScriptCache *psc, DWORD utf32c, WORD *glyph_index, DWORD flags)
{
    /* BMP: use gdi32 for ease */
    if (utf32c < 0x10000)
    {
        WCHAR ch = utf32c;
        return GetGlyphIndicesW(hdc, &ch, 1, glyph_index, flags);
    }

    if (!psc->CMAP_format12_Table)
        psc->CMAP_format12_Table = load_CMAP_format12_table(hdc, psc);

    if (flags & GGI_MARK_NONEXISTING_GLYPHS)
        *glyph_index = 0xffffu;
    else
        *glyph_index = 0u;

    if (psc->CMAP_format12_Table)
    {
        CMAP_SegmentedCoverage *format = NULL;
        CMAP_SegmentedCoverage_group *group = NULL;

        format = (CMAP_SegmentedCoverage *)psc->CMAP_format12_Table;

        group = bsearch(&utf32c, format->groups, GET_BE_DWORD(format->nGroups),
                        sizeof(CMAP_SegmentedCoverage_group), compare_group);

        if (group)
        {
            DWORD offset = utf32c - GET_BE_DWORD(group->startCharCode);
            *glyph_index = GET_BE_DWORD(group->startGlyphID) + offset;
            return 0;
        }
    }
    return 0;
}

/**********
 * GDEF
 **********/

static WORD OT_get_glyph_class(const void *table, WORD glyph)
{
    WORD class = 0;
    const OT_ClassDefFormat1 *cf1 = table;

    if (!table) return 0;

    if (GET_BE_WORD(cf1->ClassFormat) == 1)
    {
        if (glyph >= GET_BE_WORD(cf1->StartGlyph))
        {
            int index = glyph - GET_BE_WORD(cf1->StartGlyph);
            if (index < GET_BE_WORD(cf1->GlyphCount))
                class = GET_BE_WORD(cf1->ClassValueArray[index]);
        }
    }
    else if (GET_BE_WORD(cf1->ClassFormat) == 2)
    {
        const OT_ClassDefFormat2 *cf2 = table;
        int i, top;
        top = GET_BE_WORD(cf2->ClassRangeCount);
        for (i = 0; i < top; i++)
        {
            if (glyph >= GET_BE_WORD(cf2->ClassRangeRecord[i].Start) &&
                glyph <= GET_BE_WORD(cf2->ClassRangeRecord[i].End))
            {
                class = GET_BE_WORD(cf2->ClassRangeRecord[i].Class);
                break;
            }
        }
    }
    else
        ERR("Unknown Class Format %i\n",GET_BE_WORD(cf1->ClassFormat));

    return class;
}

void OpenType_GDEF_UpdateGlyphProps(ScriptCache *psc, const WORD *pwGlyphs, const WORD cGlyphs, WORD* pwLogClust, const WORD cChars, SCRIPT_GLYPHPROP *pGlyphProp)
{
    int i;
    void *glyph_class_table = NULL;

    if (psc->GDEF_Table)
    {
        const GDEF_Header *header = psc->GDEF_Table;
        WORD offset = GET_BE_WORD( header->GlyphClassDef );
        if (offset)
            glyph_class_table = (BYTE *)psc->GDEF_Table + offset;
    }

    for (i = 0; i < cGlyphs; i++)
    {
        WORD class;
        int char_count = 0;
        int k;

        k = USP10_FindGlyphInLogClust(pwLogClust, cChars, i);
        if (k >= 0)
        {
            for (; k < cChars && pwLogClust[k] == i; k++)
                char_count++;
        }

        class = OT_get_glyph_class( glyph_class_table, pwGlyphs[i] );

        switch (class)
        {
            case 0:
            case BaseGlyph:
                pGlyphProp[i].sva.fClusterStart = 1;
                pGlyphProp[i].sva.fDiacritic = 0;
                pGlyphProp[i].sva.fZeroWidth = 0;
                break;
            case LigatureGlyph:
                pGlyphProp[i].sva.fClusterStart = 1;
                pGlyphProp[i].sva.fDiacritic = 0;
                pGlyphProp[i].sva.fZeroWidth = 0;
                break;
            case MarkGlyph:
                pGlyphProp[i].sva.fClusterStart = 0;
                pGlyphProp[i].sva.fDiacritic = 1;
                pGlyphProp[i].sva.fZeroWidth = 1;
                break;
            case ComponentGlyph:
                pGlyphProp[i].sva.fClusterStart = 0;
                pGlyphProp[i].sva.fDiacritic = 0;
                pGlyphProp[i].sva.fZeroWidth = 0;
                break;
            default:
                ERR("Unknown glyph class %i\n",class);
                pGlyphProp[i].sva.fClusterStart = 1;
                pGlyphProp[i].sva.fDiacritic = 0;
                pGlyphProp[i].sva.fZeroWidth = 0;
        }

        if (char_count == 0)
            pGlyphProp[i].sva.fClusterStart = 0;
    }
}

/**********
 * GSUB
 **********/
static INT GSUB_apply_lookup(const OT_LookupList* lookup, INT lookup_index, WORD *glyphs, INT glyph_index, INT write_dir, INT *glyph_count);

static int GSUB_is_glyph_covered(const void *table, unsigned int glyph)
{
    const OT_CoverageFormat1* cf1;

    cf1 = table;

    if (GET_BE_WORD(cf1->CoverageFormat) == 1)
    {
        int count = GET_BE_WORD(cf1->GlyphCount);
        int i;
        TRACE("Coverage Format 1, %i glyphs\n",count);
        for (i = 0; i < count; i++)
            if (glyph == GET_BE_WORD(cf1->GlyphArray[i]))
                return i;
        return -1;
    }
    else if (GET_BE_WORD(cf1->CoverageFormat) == 2)
    {
        const OT_CoverageFormat2* cf2;
        int i;
        int count;
        cf2 = (const OT_CoverageFormat2*)cf1;

        count = GET_BE_WORD(cf2->RangeCount);
        TRACE("Coverage Format 2, %i ranges\n",count);
        for (i = 0; i < count; i++)
        {
            if (glyph < GET_BE_WORD(cf2->RangeRecord[i].Start))
                return -1;
            if ((glyph >= GET_BE_WORD(cf2->RangeRecord[i].Start)) &&
                (glyph <= GET_BE_WORD(cf2->RangeRecord[i].End)))
            {
                return (GET_BE_WORD(cf2->RangeRecord[i].StartCoverageIndex) +
                    glyph - GET_BE_WORD(cf2->RangeRecord[i].Start));
            }
        }
        return -1;
    }
    else
        ERR("Unknown CoverageFormat %i\n",GET_BE_WORD(cf1->CoverageFormat));

    return -1;
}

static const BYTE *GSUB_get_subtable(const OT_LookupTable *look, int index)
{
    int offset = GET_BE_WORD(look->SubTable[index]);

    if (GET_BE_WORD(look->LookupType) == GSUB_LOOKUP_EXTENSION)
    {
        const GSUB_ExtensionPosFormat1 *ext = (const GSUB_ExtensionPosFormat1 *)((const BYTE *)look + offset);
        if (GET_BE_WORD(ext->SubstFormat) == 1)
        {
            offset += GET_BE_DWORD(ext->ExtensionOffset);
        }
        else
        {
            FIXME("Unhandled Extension Substitution Format %i\n",GET_BE_WORD(ext->SubstFormat));
        }
    }
    return (const BYTE *)look + offset;
}

static INT GSUB_apply_SingleSubst(const OT_LookupTable *look, WORD *glyphs, INT glyph_index, INT write_dir, INT *glyph_count)
{
    int j;
    TRACE("Single Substitution Subtable\n");

    for (j = 0; j < GET_BE_WORD(look->SubTableCount); j++)
    {
        const GSUB_SingleSubstFormat1 *ssf1 = (const GSUB_SingleSubstFormat1*)GSUB_get_subtable(look, j);
        if (GET_BE_WORD(ssf1->SubstFormat) == 1)
        {
            int offset = GET_BE_WORD(ssf1->Coverage);
            TRACE("  subtype 1, delta %i\n", GET_BE_WORD(ssf1->DeltaGlyphID));
            if (GSUB_is_glyph_covered((const BYTE*)ssf1+offset, glyphs[glyph_index]) != -1)
            {
                TRACE("  Glyph 0x%x ->",glyphs[glyph_index]);
                glyphs[glyph_index] = glyphs[glyph_index] + GET_BE_WORD(ssf1->DeltaGlyphID);
                TRACE(" 0x%x\n",glyphs[glyph_index]);
                return glyph_index + write_dir;
            }
        }
        else
        {
            const GSUB_SingleSubstFormat2 *ssf2;
            INT index;
            INT offset;

            ssf2 = (const GSUB_SingleSubstFormat2 *)ssf1;
            offset = GET_BE_WORD(ssf1->Coverage);
            TRACE("  subtype 2,  glyph count %i\n", GET_BE_WORD(ssf2->GlyphCount));
            index = GSUB_is_glyph_covered((const BYTE*)ssf2+offset, glyphs[glyph_index]);
            TRACE("  Coverage index %i\n",index);
            if (index != -1)
            {
                if (glyphs[glyph_index] == GET_BE_WORD(ssf2->Substitute[index]))
                    return GSUB_E_NOGLYPH;

                TRACE("    Glyph is 0x%x ->",glyphs[glyph_index]);
                glyphs[glyph_index] = GET_BE_WORD(ssf2->Substitute[index]);
                TRACE("0x%x\n",glyphs[glyph_index]);
                return glyph_index + write_dir;
            }
        }
    }
    return GSUB_E_NOGLYPH;
}

static INT GSUB_apply_MultipleSubst(const OT_LookupTable *look, WORD *glyphs, INT glyph_index, INT write_dir, INT *glyph_count)
{
    int j;
    TRACE("Multiple Substitution Subtable\n");

    for (j = 0; j < GET_BE_WORD(look->SubTableCount); j++)
    {
        int offset, index;
        const GSUB_MultipleSubstFormat1 *msf1;
        msf1 = (const GSUB_MultipleSubstFormat1*)GSUB_get_subtable(look, j);

        offset = GET_BE_WORD(msf1->Coverage);
        index = GSUB_is_glyph_covered((const BYTE*)msf1+offset, glyphs[glyph_index]);
        if (index != -1)
        {
            const GSUB_Sequence *seq;
            int sub_count;
            int j;
            offset = GET_BE_WORD(msf1->Sequence[index]);
            seq = (const GSUB_Sequence*)((const BYTE*)msf1+offset);
            sub_count = GET_BE_WORD(seq->GlyphCount);
            TRACE("  Glyph 0x%x (+%i)->",glyphs[glyph_index],(sub_count-1));

            for (j = (*glyph_count)+(sub_count-1); j > glyph_index; j--)
                    glyphs[j] =glyphs[j-(sub_count-1)];

            for (j = 0; j < sub_count; j++)
                    if (write_dir < 0)
                        glyphs[glyph_index + (sub_count-1) - j] = GET_BE_WORD(seq->Substitute[j]);
                    else
                        glyphs[glyph_index + j] = GET_BE_WORD(seq->Substitute[j]);

            *glyph_count = *glyph_count + (sub_count - 1);

            if (TRACE_ON(uniscribe))
            {
                for (j = 0; j < sub_count; j++)
                    TRACE(" 0x%x",glyphs[glyph_index+j]);
                TRACE("\n");
            }

            if (write_dir > 0)
                return glyph_index + sub_count;
            else
                return glyph_index - 1;
        }
    }
    return GSUB_E_NOGLYPH;
}

static INT GSUB_apply_AlternateSubst(const OT_LookupTable *look, WORD *glyphs, INT glyph_index, INT write_dir, INT *glyph_count)
{
    int j;
    TRACE("Alternate Substitution Subtable\n");

    for (j = 0; j < GET_BE_WORD(look->SubTableCount); j++)
    {
        int offset;
        const GSUB_AlternateSubstFormat1 *asf1;
        INT index;

        asf1 = (const GSUB_AlternateSubstFormat1*)GSUB_get_subtable(look, j);
        offset = GET_BE_WORD(asf1->Coverage);

        index = GSUB_is_glyph_covered((const BYTE*)asf1+offset, glyphs[glyph_index]);
        if (index != -1)
        {
            const GSUB_AlternateSet *as;
            offset =  GET_BE_WORD(asf1->AlternateSet[index]);
            as = (const GSUB_AlternateSet*)((const BYTE*)asf1+offset);
            FIXME("%i alternates, picking index 0\n",GET_BE_WORD(as->GlyphCount));
            if (glyphs[glyph_index] == GET_BE_WORD(as->Alternate[0]))
                return GSUB_E_NOGLYPH;

            TRACE("  Glyph 0x%x ->",glyphs[glyph_index]);
            glyphs[glyph_index] = GET_BE_WORD(as->Alternate[0]);
            TRACE(" 0x%x\n",glyphs[glyph_index]);
            return glyph_index + write_dir;
        }
    }
    return GSUB_E_NOGLYPH;
}

static INT GSUB_apply_LigatureSubst(const OT_LookupTable *look, WORD *glyphs, INT glyph_index, INT write_dir, INT *glyph_count)
{
    int j;

    TRACE("Ligature Substitution Subtable\n");
    for (j = 0; j < GET_BE_WORD(look->SubTableCount); j++)
    {
        const GSUB_LigatureSubstFormat1 *lsf1;
        int offset,index;

        lsf1 = (const GSUB_LigatureSubstFormat1*)GSUB_get_subtable(look, j);
        offset = GET_BE_WORD(lsf1->Coverage);
        index = GSUB_is_glyph_covered((const BYTE*)lsf1+offset, glyphs[glyph_index]);
        TRACE("  Coverage index %i\n",index);
        if (index != -1)
        {
            const GSUB_LigatureSet *ls;
            int k, count;

            offset = GET_BE_WORD(lsf1->LigatureSet[index]);
            ls = (const GSUB_LigatureSet*)((const BYTE*)lsf1+offset);
            count = GET_BE_WORD(ls->LigatureCount);
            TRACE("  LigatureSet has %i members\n",count);
            for (k = 0; k < count; k++)
            {
                const GSUB_Ligature *lig;
                int CompCount,l,CompIndex;

                offset = GET_BE_WORD(ls->Ligature[k]);
                lig = (const GSUB_Ligature*)((const BYTE*)ls+offset);
                CompCount = GET_BE_WORD(lig->CompCount) - 1;
                CompIndex = glyph_index+write_dir;
                for (l = 0; l < CompCount && CompIndex >= 0 && CompIndex < *glyph_count; l++)
                {
                    int CompGlyph;
                    CompGlyph = GET_BE_WORD(lig->Component[l]);
                    if (CompGlyph != glyphs[CompIndex])
                        break;
                    CompIndex += write_dir;
                }
                if (l == CompCount)
                {
                    int replaceIdx = glyph_index;
                    if (write_dir < 0)
                        replaceIdx = glyph_index - CompCount;

                    TRACE("    Glyph is 0x%x (+%i) ->",glyphs[glyph_index],CompCount);
                    glyphs[replaceIdx] = GET_BE_WORD(lig->LigGlyph);
                    TRACE("0x%x\n",glyphs[replaceIdx]);
                    if (CompCount > 0)
                    {
                        unsigned int j = replaceIdx + 1;
                        memmove(&glyphs[j], &glyphs[j + CompCount], (*glyph_count - j) * sizeof(*glyphs));
                        *glyph_count = *glyph_count - CompCount;
                    }
                    return replaceIdx + write_dir;
                }
            }
        }
    }
    return GSUB_E_NOGLYPH;
}

static INT GSUB_apply_ContextSubst(const OT_LookupList* lookup, const OT_LookupTable *look, WORD *glyphs, INT glyph_index, INT write_dir, INT *glyph_count)
{
    int j;
    TRACE("Context Substitution Subtable\n");
    for (j = 0; j < GET_BE_WORD(look->SubTableCount); j++)
    {
        const GSUB_ContextSubstFormat1 *csf1;

        csf1 = (const GSUB_ContextSubstFormat1*)GSUB_get_subtable(look, j);
        if (GET_BE_WORD(csf1->SubstFormat) == 1)
        {
            int offset, index;
            TRACE("Context Substitution Subtable: Class 1\n");
            offset = GET_BE_WORD(csf1->Coverage);
            index = GSUB_is_glyph_covered((const BYTE*)csf1+offset, glyphs[glyph_index]);
            TRACE("  Coverage index %i\n",index);
            if (index != -1)
            {
                int k, count;
                const GSUB_SubRuleSet *srs;
                offset = GET_BE_WORD(csf1->SubRuleSet[index]);
                srs = (const GSUB_SubRuleSet*)((const BYTE*)csf1+offset);
                count = GET_BE_WORD(srs->SubRuleCount);
                TRACE("  SubRuleSet has %i members\n",count);
                for (k = 0; k < count; k++)
                {
                    const GSUB_SubRule_1 *sr;
                    const GSUB_SubRule_2 *sr_2;
                    unsigned int g;
                    int g_count, l;
                    int newIndex = glyph_index;

                    offset = GET_BE_WORD(srs->SubRule[k]);
                    sr = (const GSUB_SubRule_1*)((const BYTE*)srs+offset);
                    g_count = GET_BE_WORD(sr->GlyphCount);
                    TRACE("   SubRule has %i glyphs\n",g_count);

                    g = glyph_index + write_dir * (g_count - 1);
                    if (g >= *glyph_count)
                        continue;

                    for (l = 0; l < g_count-1; l++)
                        if (glyphs[glyph_index + (write_dir * (l+1))] != GET_BE_WORD(sr->Input[l])) break;

                    if (l < g_count-1)
                    {
                        TRACE("   Rule does not match\n");
                        continue;
                    }

                    TRACE("   Rule matches\n");
                    sr_2 = (const GSUB_SubRule_2 *)&sr->Input[g_count - 1];

                    for (l = 0; l < GET_BE_WORD(sr->SubstCount); l++)
                    {
                        unsigned int lookup_index = GET_BE_WORD(sr_2->SubstLookupRecord[l].LookupListIndex);
                        unsigned int sequence_index = GET_BE_WORD(sr_2->SubstLookupRecord[l].SequenceIndex);

                        g = glyph_index + write_dir * sequence_index;
                        if (g >= *glyph_count)
                        {
                            WARN("Invalid sequence index %u (glyph index %u, write dir %d).\n",
                                    sequence_index, glyph_index, write_dir);
                            continue;
                        }

                        TRACE("   SUBST: %u -> %u %u.\n", l, sequence_index, lookup_index);
                        newIndex = GSUB_apply_lookup(lookup, lookup_index, glyphs, g, write_dir, glyph_count);
                        if (newIndex == GSUB_E_NOGLYPH)
                        {
                            ERR("   Chain failed to generate a glyph\n");
                            continue;
                        }
                    }
                    return newIndex;
                }
            }
        }
        else if (GET_BE_WORD(csf1->SubstFormat) == 2)
        {
            const GSUB_ContextSubstFormat2 *csf2;
            const void *glyph_class_table;
            int offset, index;

            csf2 = (const GSUB_ContextSubstFormat2*)csf1;
            TRACE("Context Substitution Subtable: Class 2\n");
            offset = GET_BE_WORD(csf2->Coverage);
            index = GSUB_is_glyph_covered((const BYTE*)csf2+offset, glyphs[glyph_index]);
            TRACE("  Coverage index %i\n",index);
            if (index != -1)
            {
                int k, count, class;
                const GSUB_SubClassSet *scs;

                offset = GET_BE_WORD(csf2->ClassDef);
                glyph_class_table = (const BYTE *)csf2 + offset;

                class = OT_get_glyph_class(glyph_class_table,glyphs[glyph_index]);

                offset = GET_BE_WORD(csf2->SubClassSet[class]);
                if (offset == 0)
                {
                    TRACE("  No class rule table for class %i\n",class);
                    continue;
                }
                scs = (const GSUB_SubClassSet*)((const BYTE*)csf2+offset);
                count = GET_BE_WORD(scs->SubClassRuleCnt);
                TRACE("  SubClassSet has %i members\n",count);
                for (k = 0; k < count; k++)
                {
                    const GSUB_SubClassRule_1 *sr;
                    const GSUB_SubClassRule_2 *sr_2;
                    unsigned int g;
                    int g_count, l;
                    int newIndex = glyph_index;

                    offset = GET_BE_WORD(scs->SubClassRule[k]);
                    sr = (const GSUB_SubClassRule_1*)((const BYTE*)scs+offset);
                    g_count = GET_BE_WORD(sr->GlyphCount);
                    TRACE("   SubClassRule has %i glyphs classes\n",g_count);

                    g = glyph_index + write_dir * (g_count - 1);
                    if (g >= *glyph_count)
                        continue;

                    for (l = 0; l < g_count-1; l++)
                    {
                        int g_class = OT_get_glyph_class(glyph_class_table, glyphs[glyph_index + (write_dir * (l+1))]);
                        if (g_class != GET_BE_WORD(sr->Class[l])) break;
                    }

                    if (l < g_count-1)
                    {
                        TRACE("   Rule does not match\n");
                        continue;
                    }

                    TRACE("   Rule matches\n");
                    sr_2 = (const GSUB_SubClassRule_2 *)&sr->Class[g_count - 1];

                    for (l = 0; l < GET_BE_WORD(sr->SubstCount); l++)
                    {
                        unsigned int lookup_index = GET_BE_WORD(sr_2->SubstLookupRecord[l].LookupListIndex);
                        unsigned int sequence_index = GET_BE_WORD(sr_2->SubstLookupRecord[l].SequenceIndex);

                        g = glyph_index + write_dir * sequence_index;
                        if (g >= *glyph_count)
                        {
                            WARN("Invalid sequence index %u (glyph index %u, write dir %d).\n",
                                    sequence_index, glyph_index, write_dir);
                            continue;
                        }

                        TRACE("   SUBST: %u -> %u %u.\n", l, sequence_index, lookup_index);
                        newIndex = GSUB_apply_lookup(lookup, lookup_index, glyphs, g, write_dir, glyph_count);
                        if (newIndex == GSUB_E_NOGLYPH)
                        {
                            ERR("   Chain failed to generate a glyph\n");
                            continue;
                        }
                    }
                    return newIndex;
                }
            }
        }
        else
            FIXME("Unhandled Context Substitution Format %i\n", GET_BE_WORD(csf1->SubstFormat));
    }
    return GSUB_E_NOGLYPH;
}

static INT GSUB_apply_ChainContextSubst(const OT_LookupList* lookup, const OT_LookupTable *look, WORD *glyphs, INT glyph_index, INT write_dir, INT *glyph_count)
{
    int j;

    TRACE("Chaining Contextual Substitution Subtable\n");
    for (j = 0; j < GET_BE_WORD(look->SubTableCount); j++)
    {
        const GSUB_ChainContextSubstFormat1 *ccsf1;
        int offset;
        int dirLookahead = write_dir;
        int dirBacktrack = -1 * write_dir;

        ccsf1 = (const GSUB_ChainContextSubstFormat1*)GSUB_get_subtable(look, j);
        if (GET_BE_WORD(ccsf1->SubstFormat) == 1)
        {
            static int once;
            if (!once++)
                FIXME("  TODO: subtype 1 (Simple context glyph substitution)\n");
            continue;
        }
        else if (GET_BE_WORD(ccsf1->SubstFormat) == 2)
        {
            WORD offset, count;
            const void *backtrack_class_table;
            const void *input_class_table;
            const void *lookahead_class_table;
            int i;
            WORD class;

            const GSUB_ChainContextSubstFormat2 *ccsf2 = (const GSUB_ChainContextSubstFormat2*)ccsf1;
            const GSUB_ChainSubClassSet *csc;

            TRACE("  subtype 2 (Class-based Chaining Context Glyph Substitution)\n");

            offset = GET_BE_WORD(ccsf2->Coverage);

            if (GSUB_is_glyph_covered((const BYTE*)ccsf2+offset, glyphs[glyph_index]) == -1)
            {
                TRACE("Glyph not covered\n");
                continue;
            }
            offset = GET_BE_WORD(ccsf2->BacktrackClassDef);
            backtrack_class_table = (const BYTE*)ccsf2+offset;
            offset = GET_BE_WORD(ccsf2->InputClassDef);
            input_class_table = (const BYTE*)ccsf2+offset;
            offset = GET_BE_WORD(ccsf2->LookaheadClassDef);
            lookahead_class_table = (const BYTE*)ccsf2+offset;
            count =  GET_BE_WORD(ccsf2->ChainSubClassSetCnt);

            class = OT_get_glyph_class(input_class_table, glyphs[glyph_index]);
            offset = GET_BE_WORD(ccsf2->ChainSubClassSet[class]);

            if (offset == 0)
            {
                TRACE("No rules for class\n");
                continue;
            }

            csc = (const GSUB_ChainSubClassSet*)((BYTE*)ccsf2+offset);
            count = GET_BE_WORD(csc->ChainSubClassRuleCnt);

            TRACE("%i rules to check\n",count);

            for (i = 0; i < count; i++)
            {
                WORD backtrack_count, input_count, lookahead_count, substitute_count;
                int k;
                const GSUB_ChainSubClassRule_1 *backtrack;
                const GSUB_ChainSubClassRule_2 *input;
                const GSUB_ChainSubClassRule_3 *lookahead;
                const GSUB_ChainSubClassRule_4 *substitute;
                int new_index = GSUB_E_NOGLYPH;

                offset = GET_BE_WORD(csc->ChainSubClassRule[i]);
                backtrack = (const GSUB_ChainSubClassRule_1 *)((BYTE *)csc + offset);
                backtrack_count = GET_BE_WORD(backtrack->BacktrackGlyphCount);
                k = glyph_index + dirBacktrack * backtrack_count;
                if (k < 0 || k >= *glyph_count)
                    continue;

                input = (const GSUB_ChainSubClassRule_2 *)&backtrack->Backtrack[backtrack_count];
                input_count = GET_BE_WORD(input->InputGlyphCount) - 1;
                k = glyph_index + write_dir * input_count;
                if (k < 0 || k >= *glyph_count)
                    continue;

                lookahead = (const GSUB_ChainSubClassRule_3 *)&input->Input[input_count];
                lookahead_count = GET_BE_WORD(lookahead->LookaheadGlyphCount);
                k = glyph_index + dirLookahead * (input_count + lookahead_count);
                if (k < 0 || k >= *glyph_count)
                    continue;

                substitute = (const GSUB_ChainSubClassRule_4 *)&lookahead->LookAhead[lookahead_count];

                for (k = 0; k < backtrack_count; ++k)
                {
                    WORD target_class = GET_BE_WORD(backtrack->Backtrack[k]);
                    WORD glyph_class = OT_get_glyph_class(backtrack_class_table, glyphs[glyph_index + (dirBacktrack * (k+1))]);
                    if (target_class != glyph_class)
                        break;
                }
                if (k != backtrack_count)
                    continue;
                TRACE("Matched Backtrack\n");

                for (k = 0; k < input_count; ++k)
                {
                    WORD target_class = GET_BE_WORD(input->Input[k]);
                    WORD glyph_class = OT_get_glyph_class(input_class_table, glyphs[glyph_index + (write_dir * (k+1))]);
                    if (target_class != glyph_class)
                        break;
                }
                if (k != input_count)
                    continue;
                TRACE("Matched IndexGlyphs\n");

                for (k = 0; k < lookahead_count; ++k)
                {
                    WORD target_class = GET_BE_WORD(lookahead->LookAhead[k]);
                    WORD glyph_class = OT_get_glyph_class(lookahead_class_table,
                            glyphs[glyph_index + (dirLookahead * (input_count + k + 1))]);
                    if (target_class != glyph_class)
                        break;
                }
                if (k != lookahead_count)
                    continue;
                TRACE("Matched LookAhead\n");

                substitute_count = GET_BE_WORD(substitute->SubstCount);
                for (k = 0; k < substitute_count; ++k)
                {
                    unsigned int lookup_index = GET_BE_WORD(substitute->SubstLookupRecord[k].LookupListIndex);
                    unsigned int sequence_index = GET_BE_WORD(substitute->SubstLookupRecord[k].SequenceIndex);
                    unsigned int g = glyph_index + write_dir * sequence_index;

                    if (g >= *glyph_count)
                    {
                        WARN("Skipping invalid sequence index %u (glyph index %u, write dir %d).\n",
                                sequence_index, glyph_index, write_dir);
                        continue;
                    }

                    TRACE("SUBST: %u -> %u %u.\n", k, sequence_index, lookup_index);
                    new_index = GSUB_apply_lookup(lookup, lookup_index, glyphs, g, write_dir, glyph_count);
                    if (new_index == GSUB_E_NOGLYPH)
                        ERR("Chain failed to generate a glyph.\n");
                }
                return new_index;
            }
        }
        else if (GET_BE_WORD(ccsf1->SubstFormat) == 3)
        {
            WORD backtrack_count, input_count, lookahead_count, substitution_count;
            int k;
            const GSUB_ChainContextSubstFormat3_1 *backtrack;
            const GSUB_ChainContextSubstFormat3_2 *input;
            const GSUB_ChainContextSubstFormat3_3 *lookahead;
            const GSUB_ChainContextSubstFormat3_4 *substitute;
            int new_index = GSUB_E_NOGLYPH;

            TRACE("  subtype 3 (Coverage-based Chaining Context Glyph Substitution)\n");

            backtrack = (const GSUB_ChainContextSubstFormat3_1 *)ccsf1;
            backtrack_count = GET_BE_WORD(backtrack->BacktrackGlyphCount);
            k = glyph_index + dirBacktrack * backtrack_count;
            if (k < 0 || k >= *glyph_count)
                continue;

            input = (const GSUB_ChainContextSubstFormat3_2 *)&backtrack->Coverage[backtrack_count];
            input_count = GET_BE_WORD(input->InputGlyphCount);
            k = glyph_index + write_dir * (input_count - 1);
            if (k < 0 || k >= *glyph_count)
                continue;

            lookahead = (const GSUB_ChainContextSubstFormat3_3 *)&input->Coverage[input_count];
            lookahead_count = GET_BE_WORD(lookahead->LookaheadGlyphCount);
            k = glyph_index + dirLookahead * (input_count + lookahead_count - 1);
            if (k < 0 || k >= *glyph_count)
                continue;

            substitute = (const GSUB_ChainContextSubstFormat3_4 *)&lookahead->Coverage[lookahead_count];

            for (k = 0; k < backtrack_count; ++k)
            {
                offset = GET_BE_WORD(backtrack->Coverage[k]);
                if (GSUB_is_glyph_covered((const BYTE *)ccsf1 + offset,
                        glyphs[glyph_index + (dirBacktrack * (k + 1))]) == -1)
                    break;
            }
            if (k != backtrack_count)
                continue;
            TRACE("Matched Backtrack\n");

            for (k = 0; k < input_count; ++k)
            {
                offset = GET_BE_WORD(input->Coverage[k]);
                if (GSUB_is_glyph_covered((const BYTE *)ccsf1 + offset,
                        glyphs[glyph_index + (write_dir * k)]) == -1)
                    break;
            }
            if (k != input_count)
                continue;
            TRACE("Matched IndexGlyphs\n");

            for (k = 0; k < lookahead_count; ++k)
            {
                offset = GET_BE_WORD(lookahead->Coverage[k]);
                if (GSUB_is_glyph_covered((const BYTE *)ccsf1 + offset,
                        glyphs[glyph_index + (dirLookahead * (input_count + k))]) == -1)
                    break;
            }
            if (k != lookahead_count)
                continue;
            TRACE("Matched LookAhead\n");

            substitution_count = GET_BE_WORD(substitute->SubstCount);
            for (k = 0; k < substitution_count; ++k)
            {
                unsigned int lookup_index = GET_BE_WORD(substitute->SubstLookupRecord[k].LookupListIndex);
                unsigned int sequence_index = GET_BE_WORD(substitute->SubstLookupRecord[k].SequenceIndex);
                unsigned int g = glyph_index + write_dir * sequence_index;

                if (g >= *glyph_count)
                {
                    WARN("Skipping invalid sequence index %u (glyph index %u, write dir %d).\n",
                            sequence_index, glyph_index, write_dir);
                    continue;
                }

                TRACE("SUBST: %u -> %u %u.\n", k, sequence_index, lookup_index);
                new_index = GSUB_apply_lookup(lookup, lookup_index, glyphs, g, write_dir, glyph_count);
                if (new_index == GSUB_E_NOGLYPH)
                    ERR("Chain failed to generate a glyph.\n");
            }
            return new_index;
        }
    }
    return GSUB_E_NOGLYPH;
}

static INT GSUB_apply_lookup(const OT_LookupList* lookup, INT lookup_index, WORD *glyphs, INT glyph_index, INT write_dir, INT *glyph_count)
{
    int offset;
    enum gsub_lookup_type type;
    const OT_LookupTable *look;

    offset = GET_BE_WORD(lookup->Lookup[lookup_index]);
    look = (const OT_LookupTable*)((const BYTE*)lookup + offset);
    type = GET_BE_WORD(look->LookupType);
    TRACE("type %#x, flag %#x, subtables %u.\n", type,
            GET_BE_WORD(look->LookupFlag),GET_BE_WORD(look->SubTableCount));

    if (type == GSUB_LOOKUP_EXTENSION)
    {
        if (GET_BE_WORD(look->SubTableCount))
        {
            const GSUB_ExtensionPosFormat1 *ext = (const GSUB_ExtensionPosFormat1 *)((const BYTE *)look + GET_BE_WORD(look->SubTable[0]));
            if (GET_BE_WORD(ext->SubstFormat) == 1)
            {
                type = GET_BE_WORD(ext->ExtensionLookupType);
                TRACE("extension type %i\n",type);
            }
            else
            {
                FIXME("Unhandled Extension Substitution Format %i\n",GET_BE_WORD(ext->SubstFormat));
            }
        }
        else
        {
            WARN("lookup type is Extension Substitution but no extension subtable exists\n");
        }
    }
    switch(type)
    {
        case GSUB_LOOKUP_SINGLE:
            return GSUB_apply_SingleSubst(look, glyphs, glyph_index, write_dir, glyph_count);
        case GSUB_LOOKUP_MULTIPLE:
            return GSUB_apply_MultipleSubst(look, glyphs, glyph_index, write_dir, glyph_count);
        case GSUB_LOOKUP_ALTERNATE:
            return GSUB_apply_AlternateSubst(look, glyphs, glyph_index, write_dir, glyph_count);
        case GSUB_LOOKUP_LIGATURE:
            return GSUB_apply_LigatureSubst(look, glyphs, glyph_index, write_dir, glyph_count);
        case GSUB_LOOKUP_CONTEXT:
            return GSUB_apply_ContextSubst(lookup, look, glyphs, glyph_index, write_dir, glyph_count);
        case GSUB_LOOKUP_CONTEXT_CHAINED:
            return GSUB_apply_ChainContextSubst(lookup, look, glyphs, glyph_index, write_dir, glyph_count);
        case GSUB_LOOKUP_EXTENSION:
            FIXME("Extension Substitution types not valid here\n");
            break;
        default:
            FIXME("Unhandled GSUB lookup type %#x.\n", type);
    }
    return GSUB_E_NOGLYPH;
}

int OpenType_apply_GSUB_lookup(const void *table, unsigned int lookup_index, WORD *glyphs,
        unsigned int glyph_index, int write_dir, int *glyph_count)
{
    const GSUB_Header *header = (const GSUB_Header *)table;
    const OT_LookupList *lookup = (const OT_LookupList*)((const BYTE*)header + GET_BE_WORD(header->LookupList));

    return GSUB_apply_lookup(lookup, lookup_index, glyphs, glyph_index, write_dir, glyph_count);
}

/**********
 * GPOS
 **********/
static unsigned int GPOS_apply_lookup(const ScriptCache *script_cache, const OUTLINETEXTMETRICW *otm,
        const LOGFONTW *logfont, const SCRIPT_ANALYSIS *analysis, int *advance, const OT_LookupList *lookup,
        unsigned int lookup_index, const WORD *glyphs, unsigned int glyph_index, unsigned int glyph_count,
        GOFFSET *goffset);

static INT GPOS_get_device_table_value(const OT_DeviceTable *DeviceTable, WORD ppem)
{
    static const WORD mask[3] = {3,0xf,0xff};
    if (DeviceTable && ppem >= GET_BE_WORD(DeviceTable->StartSize) && ppem  <= GET_BE_WORD(DeviceTable->EndSize))
    {
        WORD format = GET_BE_WORD(DeviceTable->DeltaFormat);
        int index = ppem - GET_BE_WORD(DeviceTable->StartSize);
        int value;

        TRACE("device table, format %#x, index %i\n", format, index);

        if (format < 1 || format > 3)
        {
            WARN("invalid delta format %#x\n", format);
            return 0;
        }

        index = index << format;
        value = (DeviceTable->DeltaValue[index/sizeof(WORD)] << (index%sizeof(WORD)))&mask[format-1];
        TRACE("offset %i, value %i\n",index, value);
        if (value > mask[format-1]/2)
            value = -1 * ((mask[format-1]+1) - value);
        return value;
    }
    return 0;
}

static void GPOS_get_anchor_values(const void *table, POINT *pt, WORD ppem)
{
    const GPOS_AnchorFormat1* anchor1 = (const GPOS_AnchorFormat1*)table;

    switch (GET_BE_WORD(anchor1->AnchorFormat))
    {
        case 1:
        {
            TRACE("Anchor Format 1\n");
            pt->x = (short)GET_BE_WORD(anchor1->XCoordinate);
            pt->y = (short)GET_BE_WORD(anchor1->YCoordinate);
            break;
        }
        case 2:
        {
            const GPOS_AnchorFormat2* anchor2 = (const GPOS_AnchorFormat2*)table;
            TRACE("Anchor Format 2\n");
            pt->x = (short)GET_BE_WORD(anchor2->XCoordinate);
            pt->y = (short)GET_BE_WORD(anchor2->YCoordinate);
            break;
        }
        case 3:
        {
            int offset;
            const GPOS_AnchorFormat3* anchor3 = (const GPOS_AnchorFormat3*)table;
            TRACE("Anchor Format 3\n");
            pt->x = (short)GET_BE_WORD(anchor3->XCoordinate);
            pt->y = (short)GET_BE_WORD(anchor3->YCoordinate);
            offset = GET_BE_WORD(anchor3->XDeviceTable);
            TRACE("ppem %i\n",ppem);
            if (offset)
            {
                const OT_DeviceTable* DeviceTableX = NULL;
                DeviceTableX = (const OT_DeviceTable*)((const BYTE*)anchor3 + offset);
                pt->x += GPOS_get_device_table_value(DeviceTableX, ppem);
            }
            offset = GET_BE_WORD(anchor3->YDeviceTable);
            if (offset)
            {
                const OT_DeviceTable* DeviceTableY = NULL;
                DeviceTableY = (const OT_DeviceTable*)((const BYTE*)anchor3 + offset);
                pt->y += GPOS_get_device_table_value(DeviceTableY, ppem);
            }
            break;
        }
        default:
            ERR("Unknown Anchor Format %i\n",GET_BE_WORD(anchor1->AnchorFormat));
            pt->x = 0;
            pt->y = 0;
    }
}

static void GPOS_convert_design_units_to_device(const OUTLINETEXTMETRICW *otm, const LOGFONTW *logfont,
        int desX, int desY, double *devX, double *devY)
{
    int emHeight = otm->otmTextMetrics.tmAscent + otm->otmTextMetrics.tmDescent - otm->otmTextMetrics.tmInternalLeading;

    TRACE("emHeight %i lfWidth %i\n",emHeight, logfont->lfWidth);
    *devX = (desX * emHeight) / (double)otm->otmEMSquare;
    *devY = (desY * emHeight) / (double)otm->otmEMSquare;
    if (logfont->lfWidth)
        FIXME("Font with lfWidth set not handled properly.\n");
}

static INT GPOS_get_value_record(WORD ValueFormat, const WORD data[], GPOS_ValueRecord *record)
{
    INT offset = 0;
    if (ValueFormat & 0x0001) { if (data) record->XPlacement = GET_BE_WORD(data[offset]); offset++; }
    if (ValueFormat & 0x0002) { if (data) record->YPlacement = GET_BE_WORD(data[offset]); offset++; }
    if (ValueFormat & 0x0004) { if (data) record->XAdvance   = GET_BE_WORD(data[offset]); offset++; }
    if (ValueFormat & 0x0008) { if (data) record->YAdvance   = GET_BE_WORD(data[offset]); offset++; }
    if (ValueFormat & 0x0010) { if (data) record->XPlaDevice = GET_BE_WORD(data[offset]); offset++; }
    if (ValueFormat & 0x0020) { if (data) record->YPlaDevice = GET_BE_WORD(data[offset]); offset++; }
    if (ValueFormat & 0x0040) { if (data) record->XAdvDevice = GET_BE_WORD(data[offset]); offset++; }
    if (ValueFormat & 0x0080) { if (data) record->YAdvDevice = GET_BE_WORD(data[offset]); offset++; }
    return offset;
}

static void GPOS_get_value_record_offsets(const BYTE *head, GPOS_ValueRecord *ValueRecord,
        WORD ValueFormat, unsigned int ppem, POINT *ptPlacement, POINT *ptAdvance)
{
    if (ValueFormat & 0x0001) ptPlacement->x += (short)ValueRecord->XPlacement;
    if (ValueFormat & 0x0002) ptPlacement->y += (short)ValueRecord->YPlacement;
    if (ValueFormat & 0x0004) ptAdvance->x += (short)ValueRecord->XAdvance;
    if (ValueFormat & 0x0008) ptAdvance->y += (short)ValueRecord->YAdvance;
    if (ValueFormat & 0x0010) ptPlacement->x += GPOS_get_device_table_value((const OT_DeviceTable*)(head + ValueRecord->XPlaDevice), ppem);
    if (ValueFormat & 0x0020) ptPlacement->y += GPOS_get_device_table_value((const OT_DeviceTable*)(head + ValueRecord->YPlaDevice), ppem);
    if (ValueFormat & 0x0040) ptAdvance->x += GPOS_get_device_table_value((const OT_DeviceTable*)(head + ValueRecord->XAdvDevice), ppem);
    if (ValueFormat & 0x0080) ptAdvance->y += GPOS_get_device_table_value((const OT_DeviceTable*)(head + ValueRecord->YAdvDevice), ppem);
    if (ValueFormat & 0xFF00) FIXME("Unhandled Value Format %x\n",ValueFormat&0xFF00);
}

static const BYTE *GPOS_get_subtable(const OT_LookupTable *look, int index)
{
    int offset = GET_BE_WORD(look->SubTable[index]);

    if (GET_BE_WORD(look->LookupType) == GPOS_LOOKUP_POSITION_EXTENSION)
    {
        const GPOS_ExtensionPosFormat1 *ext = (const GPOS_ExtensionPosFormat1 *)((const BYTE *)look + offset);
        if (GET_BE_WORD(ext->PosFormat) == 1)
        {
            offset += GET_BE_DWORD(ext->ExtensionOffset);
        }
        else
        {
            FIXME("Unhandled Extension Positioning Format %i\n",GET_BE_WORD(ext->PosFormat));
        }
    }
    return (const BYTE *)look + offset;
}

static void GPOS_apply_SingleAdjustment(const OT_LookupTable *look, const SCRIPT_ANALYSIS *analysis,
        const WORD *glyphs, unsigned int glyph_index, unsigned int glyph_count, unsigned int ppem,
        POINT *adjust, POINT *advance)
{
    int j;

    TRACE("Single Adjustment Positioning Subtable\n");

    for (j = 0; j < GET_BE_WORD(look->SubTableCount); j++)
    {
        const GPOS_SinglePosFormat1 *spf1 = (const GPOS_SinglePosFormat1*)GPOS_get_subtable(look, j);
        WORD offset;
        if (GET_BE_WORD(spf1->PosFormat) == 1)
        {
            offset = GET_BE_WORD(spf1->Coverage);
            if (GSUB_is_glyph_covered((const BYTE*)spf1+offset, glyphs[glyph_index]) != -1)
            {
                GPOS_ValueRecord ValueRecord = {0,0,0,0,0,0,0,0};
                WORD ValueFormat = GET_BE_WORD(spf1->ValueFormat);
                GPOS_get_value_record(ValueFormat, spf1->Value, &ValueRecord);
                GPOS_get_value_record_offsets((const BYTE *)spf1, &ValueRecord, ValueFormat, ppem, adjust, advance);
                TRACE("Glyph Adjusted by %i,%i\n",ValueRecord.XPlacement,ValueRecord.YPlacement);
            }
        }
        else if (GET_BE_WORD(spf1->PosFormat) == 2)
        {
            int index;
            const GPOS_SinglePosFormat2 *spf2;
            spf2 = (const GPOS_SinglePosFormat2*)spf1;
            offset = GET_BE_WORD(spf2->Coverage);
            index  = GSUB_is_glyph_covered((const BYTE*)spf2+offset, glyphs[glyph_index]);
            if (index != -1)
            {
                int size;
                GPOS_ValueRecord ValueRecord = {0,0,0,0,0,0,0,0};
                WORD ValueFormat = GET_BE_WORD(spf2->ValueFormat);
                size = GPOS_get_value_record(ValueFormat, spf2->Value, &ValueRecord);
                if (index > 0)
                {
                    offset = size * index;
                    GPOS_get_value_record(ValueFormat, &spf2->Value[offset], &ValueRecord);
                }
                GPOS_get_value_record_offsets((const BYTE *)spf2, &ValueRecord, ValueFormat, ppem, adjust, advance);
                TRACE("Glyph Adjusted by %i,%i\n",ValueRecord.XPlacement,ValueRecord.YPlacement);
            }
        }
        else
            FIXME("Single Adjustment Positioning: Format %i Unhandled\n",GET_BE_WORD(spf1->PosFormat));
    }
}

static void apply_pair_value( const void *pos_table, WORD val_fmt1, WORD val_fmt2, const WORD *pair,
                              INT ppem, POINT *adjust, POINT *advance )
{
    GPOS_ValueRecord val_rec1 = {0,0,0,0,0,0,0,0};
    GPOS_ValueRecord val_rec2 = {0,0,0,0,0,0,0,0};
    INT size;

    size = GPOS_get_value_record( val_fmt1, pair, &val_rec1 );
    GPOS_get_value_record( val_fmt2, pair + size, &val_rec2 );

    if (val_fmt1)
    {
        GPOS_get_value_record_offsets( pos_table, &val_rec1, val_fmt1, ppem, adjust, advance );
        TRACE( "Glyph 1 resulting cumulative offset is %s design units\n", wine_dbgstr_point(&adjust[0]) );
        TRACE( "Glyph 1 resulting cumulative advance is %s design units\n", wine_dbgstr_point(&advance[0]) );
    }
    if (val_fmt2)
    {
        GPOS_get_value_record_offsets( pos_table, &val_rec2, val_fmt2, ppem, adjust + 1, advance + 1 );
        TRACE( "Glyph 2 resulting cumulative offset is %s design units\n", wine_dbgstr_point(&adjust[1]) );
        TRACE( "Glyph 2 resulting cumulative advance is %s design units\n", wine_dbgstr_point(&advance[1]) );
    }
}

static int GPOS_apply_PairAdjustment(const OT_LookupTable *look, const SCRIPT_ANALYSIS *analysis,
        const WORD *glyphs, unsigned int glyph_index, unsigned int glyph_count, unsigned int ppem,
        POINT *adjust, POINT *advance)
{
    int j;
    int write_dir = (analysis->fRTL && !analysis->fLogicalOrder) ? -1 : 1;

    if (glyph_index + write_dir >= glyph_count)
        return 1;

    TRACE("Pair Adjustment Positioning Subtable\n");

    for (j = 0; j < GET_BE_WORD(look->SubTableCount); j++)
    {
        const GPOS_PairPosFormat1 *ppf1 = (const GPOS_PairPosFormat1*)GPOS_get_subtable(look, j);
        WORD offset;
        if (GET_BE_WORD(ppf1->PosFormat) == 1)
        {
            int index;
            WORD ValueFormat1 = GET_BE_WORD(ppf1->ValueFormat1);
            WORD ValueFormat2 = GET_BE_WORD(ppf1->ValueFormat2);
            INT val_fmt1_size = GPOS_get_value_record( ValueFormat1, NULL, NULL );
            INT val_fmt2_size = GPOS_get_value_record( ValueFormat2, NULL, NULL );
            offset = GET_BE_WORD(ppf1->Coverage);
            index = GSUB_is_glyph_covered((const BYTE*)ppf1+offset, glyphs[glyph_index]);
            if (index != -1 && index < GET_BE_WORD(ppf1->PairSetCount))
            {
                int k;
                int pair_count;
                const GPOS_PairSet *ps;
                const GPOS_PairValueRecord *pair_val_rec;
                offset = GET_BE_WORD(ppf1->PairSetOffset[index]);
                ps = (const GPOS_PairSet*)((const BYTE*)ppf1+offset);
                pair_count = GET_BE_WORD(ps->PairValueCount);
                pair_val_rec = ps->PairValueRecord;
                for (k = 0; k < pair_count; k++)
                {
                    WORD second_glyph = GET_BE_WORD(pair_val_rec->SecondGlyph);
                    if (glyphs[glyph_index+write_dir] == second_glyph)
                    {
                        int next = 1;
                        TRACE("Format 1: Found Pair %x,%x\n",glyphs[glyph_index],glyphs[glyph_index+write_dir]);
                        apply_pair_value(ppf1, ValueFormat1, ValueFormat2,
                                pair_val_rec->Value1, ppem, adjust, advance);
                        if (ValueFormat2) next++;
                        return next;
                    }
                    pair_val_rec = (const GPOS_PairValueRecord *)(pair_val_rec->Value1 + val_fmt1_size + val_fmt2_size);
                }
            }
        }
        else if (GET_BE_WORD(ppf1->PosFormat) == 2)
        {
            const GPOS_PairPosFormat2 *ppf2 = (const GPOS_PairPosFormat2*)ppf1;
            int index;
            WORD ValueFormat1 = GET_BE_WORD( ppf2->ValueFormat1 );
            WORD ValueFormat2 = GET_BE_WORD( ppf2->ValueFormat2 );
            INT val_fmt1_size = GPOS_get_value_record( ValueFormat1, NULL, NULL );
            INT val_fmt2_size = GPOS_get_value_record( ValueFormat2, NULL, NULL );
            WORD class1_count = GET_BE_WORD( ppf2->Class1Count );
            WORD class2_count = GET_BE_WORD( ppf2->Class2Count );

            offset = GET_BE_WORD( ppf2->Coverage );
            index = GSUB_is_glyph_covered( (const BYTE*)ppf2 + offset, glyphs[glyph_index] );
            if (index != -1)
            {
                WORD class1, class2;
                class1 = OT_get_glyph_class( (const BYTE *)ppf2 + GET_BE_WORD(ppf2->ClassDef1), glyphs[glyph_index] );
                class2 = OT_get_glyph_class( (const BYTE *)ppf2 + GET_BE_WORD(ppf2->ClassDef2), glyphs[glyph_index + write_dir] );
                if (class1 < class1_count && class2 < class2_count)
                {
                    const WORD *pair_val = ppf2->Class1Record + (class1 * class2_count + class2) * (val_fmt1_size + val_fmt2_size);
                    int next = 1;

                    TRACE( "Format 2: Found Pair %x,%x\n", glyphs[glyph_index], glyphs[glyph_index + write_dir] );

                    apply_pair_value(ppf2, ValueFormat1, ValueFormat2, pair_val, ppem, adjust, advance);
                    if (ValueFormat2) next++;
                    return next;
                }
            }
        }
        else
            FIXME("Pair Adjustment Positioning: Format %i Unhandled\n",GET_BE_WORD(ppf1->PosFormat));
    }
    return 1;
}

static void GPOS_apply_CursiveAttachment(const OT_LookupTable *look, const SCRIPT_ANALYSIS *analysis,
        const WORD *glyphs, unsigned int glyph_index, unsigned int glyph_count, unsigned int ppem, POINT *pt)
{
    int j;
    int write_dir = (analysis->fRTL && !analysis->fLogicalOrder) ? -1 : 1;

    if (glyph_index + write_dir >= glyph_count)
        return;

    TRACE("Cursive Attachment Positioning Subtable\n");

    for (j = 0; j < GET_BE_WORD(look->SubTableCount); j++)
    {
        const GPOS_CursivePosFormat1 *cpf1 = (const GPOS_CursivePosFormat1 *)GPOS_get_subtable(look, j);
        if (GET_BE_WORD(cpf1->PosFormat) == 1)
        {
            int index_exit, index_entry;
            WORD offset = GET_BE_WORD( cpf1->Coverage );
            index_exit = GSUB_is_glyph_covered((const BYTE*)cpf1+offset, glyphs[glyph_index]);
            if (index_exit != -1 && cpf1->EntryExitRecord[index_exit].ExitAnchor!= 0)
            {
                index_entry = GSUB_is_glyph_covered((const BYTE*)cpf1+offset, glyphs[glyph_index+write_dir]);
                if (index_entry != -1 && cpf1->EntryExitRecord[index_entry].EntryAnchor != 0)
                {
                    POINT exit_pt, entry_pt;
                    offset = GET_BE_WORD(cpf1->EntryExitRecord[index_exit].ExitAnchor);
                    GPOS_get_anchor_values((const BYTE*)cpf1 + offset, &exit_pt, ppem);
                    offset = GET_BE_WORD(cpf1->EntryExitRecord[index_entry].EntryAnchor);
                    GPOS_get_anchor_values((const BYTE*)cpf1 + offset, &entry_pt, ppem);
                    TRACE("Found linkage %x[%s] %x[%s]\n",glyphs[glyph_index], wine_dbgstr_point(&exit_pt), glyphs[glyph_index+write_dir], wine_dbgstr_point(&entry_pt));
                    pt->x = entry_pt.x - exit_pt.x;
                    pt->y = entry_pt.y - exit_pt.y;
                    return;
                }
            }
        }
        else
            FIXME("Cursive Attachment Positioning: Format %i Unhandled\n",GET_BE_WORD(cpf1->PosFormat));
    }
    return;
}

static int GPOS_apply_MarkToBase(const ScriptCache *script_cache, const OT_LookupTable *look,
        const SCRIPT_ANALYSIS *analysis, const WORD *glyphs, unsigned int glyph_index,
        unsigned int glyph_count, unsigned int ppem, POINT *pt)
{
    int j;
    int write_dir = (analysis->fRTL && !analysis->fLogicalOrder) ? -1 : 1;
    const void *glyph_class_table = NULL;
    int rc = -1;

    if (script_cache->GDEF_Table)
    {
        const GDEF_Header *header = script_cache->GDEF_Table;
        WORD offset = GET_BE_WORD( header->GlyphClassDef );
        if (offset)
            glyph_class_table = (const BYTE *)script_cache->GDEF_Table + offset;
    }

    TRACE("MarkToBase Attachment Positioning Subtable\n");

    for (j = 0; j < GET_BE_WORD(look->SubTableCount); j++)
    {
        const GPOS_MarkBasePosFormat1 *mbpf1 = (const GPOS_MarkBasePosFormat1 *)GPOS_get_subtable(look, j);
        if (GET_BE_WORD(mbpf1->PosFormat) == 1)
        {
            int offset = GET_BE_WORD(mbpf1->MarkCoverage);
            int mark_index;
            mark_index = GSUB_is_glyph_covered((const BYTE*)mbpf1+offset, glyphs[glyph_index]);
            if (mark_index != -1)
            {
                int base_index;
                int base_glyph = glyph_index - write_dir;

                if (glyph_class_table)
                {
                    while (OT_get_glyph_class(glyph_class_table, glyphs[base_glyph]) == MarkGlyph && base_glyph > 0 && base_glyph < glyph_count)
                        base_glyph -= write_dir;
                }

                offset = GET_BE_WORD(mbpf1->BaseCoverage);
                base_index = GSUB_is_glyph_covered((const BYTE*)mbpf1+offset, glyphs[base_glyph]);
                if (base_index != -1)
                {
                    const GPOS_MarkArray *ma;
                    const GPOS_MarkRecord *mr;
                    const GPOS_BaseArray *ba;
                    const GPOS_BaseRecord *br;
                    int mark_class;
                    int class_count = GET_BE_WORD(mbpf1->ClassCount);
                    int baserecord_size;
                    POINT base_pt;
                    POINT mark_pt;
                    TRACE("Mark %x(%i) and base %x(%i)\n",glyphs[glyph_index], mark_index, glyphs[base_glyph], base_index);
                    offset = GET_BE_WORD(mbpf1->MarkArray);
                    ma = (const GPOS_MarkArray*)((const BYTE*)mbpf1 + offset);
                    if (mark_index > GET_BE_WORD(ma->MarkCount))
                    {
                        ERR("Mark index exceeded mark count\n");
                        return -1;
                    }
                    mr = &ma->MarkRecord[mark_index];
                    mark_class = GET_BE_WORD(mr->Class);
                    TRACE("Mark Class %i total classes %i\n",mark_class,class_count);
                    offset = GET_BE_WORD(mbpf1->BaseArray);
                    ba = (const GPOS_BaseArray*)((const BYTE*)mbpf1 + offset);
                    baserecord_size = class_count * sizeof(WORD);
                    br = (const GPOS_BaseRecord*)((const BYTE*)ba + sizeof(WORD) + (baserecord_size * base_index));
                    offset = GET_BE_WORD(br->BaseAnchor[mark_class]);
                    GPOS_get_anchor_values((const BYTE*)ba + offset, &base_pt, ppem);
                    offset = GET_BE_WORD(mr->MarkAnchor);
                    GPOS_get_anchor_values((const BYTE*)ma + offset, &mark_pt, ppem);
                    TRACE("Offset on base is %s design units\n",wine_dbgstr_point(&base_pt));
                    TRACE("Offset on mark is %s design units\n",wine_dbgstr_point(&mark_pt));
                    pt->x += base_pt.x - mark_pt.x;
                    pt->y += base_pt.y - mark_pt.y;
                    TRACE("Resulting cumulative offset is %s design units\n",wine_dbgstr_point(pt));
                    rc = base_glyph;
                }
            }
        }
        else
            FIXME("Unhandled Mark To Base Format %i\n",GET_BE_WORD(mbpf1->PosFormat));
    }
    return rc;
}

static void GPOS_apply_MarkToLigature(const OT_LookupTable *look, const SCRIPT_ANALYSIS *analysis,
        const WORD *glyphs, unsigned int glyph_index, unsigned int glyph_count, unsigned int ppem, POINT *pt)
{
    int j;
    int write_dir = (analysis->fRTL && !analysis->fLogicalOrder) ? -1 : 1;

    TRACE("MarkToLigature Attachment Positioning Subtable\n");

    for (j = 0; j < GET_BE_WORD(look->SubTableCount); j++)
    {
        const GPOS_MarkLigPosFormat1 *mlpf1 = (const GPOS_MarkLigPosFormat1 *)GPOS_get_subtable(look, j);
        if (GET_BE_WORD(mlpf1->PosFormat) == 1)
        {
            int offset = GET_BE_WORD(mlpf1->MarkCoverage);
            int mark_index;
            mark_index = GSUB_is_glyph_covered((const BYTE*)mlpf1+offset, glyphs[glyph_index]);
            if (mark_index != -1)
            {
                int ligature_index;
                offset = GET_BE_WORD(mlpf1->LigatureCoverage);
                ligature_index = GSUB_is_glyph_covered((const BYTE*)mlpf1+offset, glyphs[glyph_index - write_dir]);
                if (ligature_index != -1)
                {
                    const GPOS_MarkArray *ma;
                    const GPOS_MarkRecord *mr;

                    const GPOS_LigatureArray *la;
                    const GPOS_LigatureAttach *lt;
                    int mark_class;
                    int class_count = GET_BE_WORD(mlpf1->ClassCount);
                    int component_count;
                    int component_size;
                    int i;
                    POINT ligature_pt;
                    POINT mark_pt;

                    TRACE("Mark %x(%i) and ligature %x(%i)\n",glyphs[glyph_index], mark_index, glyphs[glyph_index - write_dir], ligature_index);
                    offset = GET_BE_WORD(mlpf1->MarkArray);
                    ma = (const GPOS_MarkArray*)((const BYTE*)mlpf1 + offset);
                    if (mark_index > GET_BE_WORD(ma->MarkCount))
                    {
                        ERR("Mark index exceeded mark count\n");
                        return;
                    }
                    mr = &ma->MarkRecord[mark_index];
                    mark_class = GET_BE_WORD(mr->Class);
                    TRACE("Mark Class %i total classes %i\n",mark_class,class_count);
                    offset = GET_BE_WORD(mlpf1->LigatureArray);
                    la = (const GPOS_LigatureArray*)((const BYTE*)mlpf1 + offset);
                    if (ligature_index > GET_BE_WORD(la->LigatureCount))
                    {
                        ERR("Ligature index exceeded ligature count\n");
                        return;
                    }
                    offset = GET_BE_WORD(la->LigatureAttach[ligature_index]);
                    lt = (const GPOS_LigatureAttach*)((const BYTE*)la + offset);

                    component_count = GET_BE_WORD(lt->ComponentCount);
                    component_size = class_count * sizeof(WORD);
                    offset = 0;
                    for (i = 0; i < component_count && !offset; i++)
                    {
                        int k;
                        const GPOS_ComponentRecord *cr = (const GPOS_ComponentRecord*)((const BYTE*)lt->ComponentRecord + (component_size * i));
                        for (k = 0; k < class_count && !offset; k++)
                            offset = GET_BE_WORD(cr->LigatureAnchor[k]);
                        cr = (const GPOS_ComponentRecord*)((const BYTE*)cr + component_size);
                    }
                    if (!offset)
                    {
                        ERR("Failed to find available ligature connection point\n");
                        return;
                    }

                    GPOS_get_anchor_values((const BYTE*)lt + offset, &ligature_pt, ppem);
                    offset = GET_BE_WORD(mr->MarkAnchor);
                    GPOS_get_anchor_values((const BYTE*)ma + offset, &mark_pt, ppem);
                    TRACE("Offset on ligature is %s design units\n",wine_dbgstr_point(&ligature_pt));
                    TRACE("Offset on mark is %s design units\n",wine_dbgstr_point(&mark_pt));
                    pt->x += ligature_pt.x - mark_pt.x;
                    pt->y += ligature_pt.y - mark_pt.y;
                    TRACE("Resulting cumulative offset is %s design units\n",wine_dbgstr_point(pt));
                }
            }
        }
        else
            FIXME("Unhandled Mark To Ligature Format %i\n",GET_BE_WORD(mlpf1->PosFormat));
    }
}

static BOOL GPOS_apply_MarkToMark(const OT_LookupTable *look, const SCRIPT_ANALYSIS *analysis,
        const WORD *glyphs, unsigned int glyph_index, unsigned int glyph_count, unsigned int ppem, POINT *pt)
{
    int j;
    BOOL rc = FALSE;
    int write_dir = (analysis->fRTL && !analysis->fLogicalOrder) ? -1 : 1;

    TRACE("MarkToMark Attachment Positioning Subtable\n");

    for (j = 0; j < GET_BE_WORD(look->SubTableCount); j++)
    {
        const GPOS_MarkMarkPosFormat1 *mmpf1 = (const GPOS_MarkMarkPosFormat1 *)GPOS_get_subtable(look, j);
        if (GET_BE_WORD(mmpf1->PosFormat) == 1)
        {
            int offset = GET_BE_WORD(mmpf1->Mark1Coverage);
            int mark_index;
            mark_index = GSUB_is_glyph_covered((const BYTE*)mmpf1+offset, glyphs[glyph_index]);
            if (mark_index != -1)
            {
                int mark2_index;
                offset = GET_BE_WORD(mmpf1->Mark2Coverage);
                mark2_index = GSUB_is_glyph_covered((const BYTE*)mmpf1+offset, glyphs[glyph_index - write_dir]);
                if (mark2_index != -1)
                {
                    const GPOS_MarkArray *ma;
                    const GPOS_MarkRecord *mr;
                    const GPOS_Mark2Array *m2a;
                    const GPOS_Mark2Record *m2r;
                    int mark_class;
                    int class_count = GET_BE_WORD(mmpf1->ClassCount);
                    int mark2record_size;
                    POINT mark2_pt;
                    POINT mark_pt;
                    TRACE("Mark %x(%i) and Mark2 %x(%i)\n",glyphs[glyph_index], mark_index, glyphs[glyph_index - write_dir], mark2_index);
                    offset = GET_BE_WORD(mmpf1->Mark1Array);
                    ma = (const GPOS_MarkArray*)((const BYTE*)mmpf1 + offset);
                    if (mark_index > GET_BE_WORD(ma->MarkCount))
                    {
                        ERR("Mark index exceeded mark count\n");
                        return FALSE;
                    }
                    mr = &ma->MarkRecord[mark_index];
                    mark_class = GET_BE_WORD(mr->Class);
                    TRACE("Mark Class %i total classes %i\n",mark_class,class_count);
                    offset = GET_BE_WORD(mmpf1->Mark2Array);
                    m2a = (const GPOS_Mark2Array*)((const BYTE*)mmpf1 + offset);
                    mark2record_size = class_count * sizeof(WORD);
                    m2r = (const GPOS_Mark2Record*)((const BYTE*)m2a + sizeof(WORD) + (mark2record_size * mark2_index));
                    offset = GET_BE_WORD(m2r->Mark2Anchor[mark_class]);
                    GPOS_get_anchor_values((const BYTE*)m2a + offset, &mark2_pt, ppem);
                    offset = GET_BE_WORD(mr->MarkAnchor);
                    GPOS_get_anchor_values((const BYTE*)ma + offset, &mark_pt, ppem);
                    TRACE("Offset on mark2 is %s design units\n",wine_dbgstr_point(&mark2_pt));
                    TRACE("Offset on mark is %s design units\n",wine_dbgstr_point(&mark_pt));
                    pt->x += mark2_pt.x - mark_pt.x;
                    pt->y += mark2_pt.y - mark_pt.y;
                    TRACE("Resulting cumulative offset is %s design units\n",wine_dbgstr_point(pt));
                    rc = TRUE;
                }
            }
        }
        else
            FIXME("Unhandled Mark To Mark Format %i\n",GET_BE_WORD(mmpf1->PosFormat));
    }
    return rc;
}

static unsigned int GPOS_apply_ContextPos(const ScriptCache *script_cache, const OUTLINETEXTMETRICW *otm,
        const LOGFONTW *logfont, const SCRIPT_ANALYSIS *analysis, int *advance, const OT_LookupList *lookup,
        const OT_LookupTable *look, const WORD *glyphs, unsigned int glyph_index, unsigned int glyph_count,
        GOFFSET *goffset)
{
    int j;
    int write_dir = (analysis->fRTL && !analysis->fLogicalOrder) ? -1 : 1;

    TRACE("Contextual Positioning Subtable\n");

    for (j = 0; j < GET_BE_WORD(look->SubTableCount); j++)
    {
        const GPOS_ContextPosFormat2 *cpf2 = (GPOS_ContextPosFormat2*)GPOS_get_subtable(look, j);

        if (GET_BE_WORD(cpf2->PosFormat) == 1)
        {
            static int once;
            if (!once++)
                FIXME("  TODO: subtype 1\n");
            continue;
        }
        else if (GET_BE_WORD(cpf2->PosFormat) == 2)
        {
            WORD offset = GET_BE_WORD(cpf2->Coverage);
            int index;

            TRACE("Contextual Positioning Subtable: Format 2\n");

            index = GSUB_is_glyph_covered((const BYTE*)cpf2+offset, glyphs[glyph_index]);
            TRACE("Coverage index %i\n",index);
            if (index != -1)
            {
                int k, count, class;
                const GPOS_PosClassSet *pcs;
                const void *glyph_class_table = NULL;

                offset = GET_BE_WORD(cpf2->ClassDef);
                glyph_class_table = (const BYTE *)cpf2 + offset;

                class = OT_get_glyph_class(glyph_class_table,glyphs[glyph_index]);

                offset = GET_BE_WORD(cpf2->PosClassSet[class]);
                if (offset == 0)
                {
                    TRACE("No class rule table for class %i\n",class);
                    continue;
                }
                pcs = (const GPOS_PosClassSet*)((const BYTE*)cpf2+offset);
                count = GET_BE_WORD(pcs->PosClassRuleCnt);
                TRACE("PosClassSet has %i members\n",count);
                for (k = 0; k < count; k++)
                {
                    const GPOS_PosClassRule_1 *pr;
                    const GPOS_PosClassRule_2 *pr_2;
                    unsigned int g;
                    int g_count, l;

                    offset = GET_BE_WORD(pcs->PosClassRule[k]);
                    pr = (const GPOS_PosClassRule_1*)((const BYTE*)pcs+offset);
                    g_count = GET_BE_WORD(pr->GlyphCount);
                    TRACE("PosClassRule has %i glyphs classes\n",g_count);

                    g = glyph_index + write_dir * (g_count - 1);
                    if (g >= glyph_count)
                        continue;

                    for (l = 0; l < g_count-1; l++)
                    {
                        int g_class = OT_get_glyph_class(glyph_class_table, glyphs[glyph_index + (write_dir * (l+1))]);
                        if (g_class != GET_BE_WORD(pr->Class[l])) break;
                    }

                    if (l < g_count-1)
                    {
                        TRACE("Rule does not match\n");
                        continue;
                    }

                    TRACE("Rule matches\n");
                    pr_2 = (const GPOS_PosClassRule_2 *)&pr->Class[g_count - 1];

                    for (l = 0; l < GET_BE_WORD(pr->PosCount); l++)
                    {
                        unsigned int lookup_index = GET_BE_WORD(pr_2->PosLookupRecord[l].LookupListIndex);
                        unsigned int sequence_index = GET_BE_WORD(pr_2->PosLookupRecord[l].SequenceIndex);

                        g = glyph_index + write_dir * sequence_index;
                        if (g >= glyph_count)
                        {
                            WARN("Invalid sequence index %u (glyph index %u, write dir %d).\n",
                                    sequence_index, glyph_index, write_dir);
                            continue;
                        }

                        TRACE("Position: %u -> %u %u.\n", l, sequence_index, lookup_index);

                        GPOS_apply_lookup(script_cache, otm, logfont, analysis, advance,
                                lookup, lookup_index, glyphs, g, glyph_count, goffset);
                    }
                    return 1;
                }
            }

            TRACE("Not covered\n");
            continue;
        }
        else if (GET_BE_WORD(cpf2->PosFormat) == 3)
        {
            static int once;
            if (!once++)
                FIXME("  TODO: subtype 3\n");
            continue;
        }
        else
            FIXME("Unhandled Contextual Positioning Format %i\n",GET_BE_WORD(cpf2->PosFormat));
    }
    return 1;
}

static unsigned int GPOS_apply_ChainContextPos(const ScriptCache *script_cache, const OUTLINETEXTMETRICW *otm,
        const LOGFONTW *logfont, const SCRIPT_ANALYSIS *analysis, int *advance, const OT_LookupList *lookup,
        const OT_LookupTable *look, const WORD *glyphs, unsigned int glyph_index, unsigned int glyph_count,
        GOFFSET *goffset)
{
    int j;
    int write_dir = (analysis->fRTL && !analysis->fLogicalOrder) ? -1 : 1;

    TRACE("Chaining Contextual Positioning Subtable\n");

    for (j = 0; j < GET_BE_WORD(look->SubTableCount); j++)
    {
        int offset;
        const GPOS_ChainContextPosFormat3_1 *backtrack = (GPOS_ChainContextPosFormat3_1 *)GPOS_get_subtable(look, j);
        int dirLookahead = write_dir;
        int dirBacktrack = -1 * write_dir;

        if (GET_BE_WORD(backtrack->PosFormat) == 1)
        {
            static int once;
            if (!once++)
                FIXME("  TODO: subtype 1 (Simple Chaining Context Glyph Positioning)\n");
            continue;
        }
        else if (GET_BE_WORD(backtrack->PosFormat) == 2)
        {
            static int once;
            if (!once++)
                FIXME("  TODO: subtype 2 (Class-based Chaining Context Glyph Positioning)\n");
            continue;
        }
        else if (GET_BE_WORD(backtrack->PosFormat) == 3)
        {
            WORD backtrack_count, input_count, lookahead_count, positioning_count;
            int k;
            const GPOS_ChainContextPosFormat3_2 *input;
            const GPOS_ChainContextPosFormat3_3 *lookahead;
            const GPOS_ChainContextPosFormat3_4 *positioning;

            TRACE("  subtype 3 (Coverage-based Chaining Context Glyph Positioning)\n");

            backtrack_count = GET_BE_WORD(backtrack->BacktrackGlyphCount);
            k = glyph_index + dirBacktrack * backtrack_count;
            if (k < 0 || k >= glyph_count)
                continue;

            input = (const GPOS_ChainContextPosFormat3_2 *)&backtrack->Coverage[backtrack_count];
            input_count = GET_BE_WORD(input->InputGlyphCount);
            k = glyph_index + write_dir * (input_count - 1);
            if (k < 0 || k >= glyph_count)
                continue;

            lookahead = (const GPOS_ChainContextPosFormat3_3 *)&input->Coverage[input_count];
            lookahead_count = GET_BE_WORD(lookahead->LookaheadGlyphCount);
            k = glyph_index + dirLookahead * (input_count + lookahead_count - 1);
            if (k < 0 || k >= glyph_count)
                continue;

            positioning = (const GPOS_ChainContextPosFormat3_4 *)&lookahead->Coverage[lookahead_count];

            for (k = 0; k < backtrack_count; ++k)
            {
                offset = GET_BE_WORD(backtrack->Coverage[k]);
                if (GSUB_is_glyph_covered((const BYTE *)backtrack + offset,
                        glyphs[glyph_index + (dirBacktrack * (k + 1))]) == -1)
                    break;
            }
            if (k != backtrack_count)
                continue;
            TRACE("Matched Backtrack\n");

            for (k = 0; k < input_count; ++k)
            {
                offset = GET_BE_WORD(input->Coverage[k]);
                if (GSUB_is_glyph_covered((const BYTE *)backtrack + offset,
                        glyphs[glyph_index + (write_dir * k)]) == -1)
                    break;
            }
            if (k != input_count)
                continue;
            TRACE("Matched IndexGlyphs\n");

            for (k = 0; k < lookahead_count; ++k)
            {
                offset = GET_BE_WORD(lookahead->Coverage[k]);
                if (GSUB_is_glyph_covered((const BYTE *)backtrack + offset,
                        glyphs[glyph_index + (dirLookahead * (input_count + k))]) == -1)
                    break;
            }
            if (k != lookahead_count)
                continue;
            TRACE("Matched LookAhead\n");

            if (!(positioning_count = GET_BE_WORD(positioning->PosCount)))
                return 1;

            for (k = 0; k < positioning_count; ++k)
            {
                unsigned int lookup_index = GET_BE_WORD(positioning->PosLookupRecord[k].LookupListIndex);
                unsigned int sequence_index = GET_BE_WORD(positioning->PosLookupRecord[k].SequenceIndex);
                unsigned int g = glyph_index + write_dir * sequence_index;

                if (g >= glyph_count)
                {
                    WARN("Skipping invalid sequence index %u (glyph index %u, write dir %d).\n",
                            sequence_index, glyph_index, write_dir);
                    continue;
                }

                TRACE("Position: %u -> %u %u.\n", k, sequence_index, lookup_index);
                GPOS_apply_lookup(script_cache, otm, logfont, analysis, advance, lookup, lookup_index,
                        glyphs, g, glyph_count, goffset);
            }
            return input_count + lookahead_count;
        }
        else
            FIXME("Unhandled Chaining Contextual Positioning Format %#x.\n", GET_BE_WORD(backtrack->PosFormat));
    }
    return 1;
}

static unsigned int GPOS_apply_lookup(const ScriptCache *script_cache, const OUTLINETEXTMETRICW *lpotm,
        const LOGFONTW *lplogfont, const SCRIPT_ANALYSIS *analysis, int *piAdvance, const OT_LookupList *lookup,
        unsigned int lookup_index, const WORD *glyphs, unsigned int glyph_index, unsigned int glyph_count,
        GOFFSET *pGoffset)
{
    int offset;
    const OT_LookupTable *look;
    int ppem = lpotm->otmTextMetrics.tmAscent + lpotm->otmTextMetrics.tmDescent - lpotm->otmTextMetrics.tmInternalLeading;
    enum gpos_lookup_type type;

    offset = GET_BE_WORD(lookup->Lookup[lookup_index]);
    look = (const OT_LookupTable*)((const BYTE*)lookup + offset);
    type = GET_BE_WORD(look->LookupType);
    TRACE("type %#x, flag %#x, subtables %u.\n", type,
            GET_BE_WORD(look->LookupFlag), GET_BE_WORD(look->SubTableCount));

    if (type == GPOS_LOOKUP_POSITION_EXTENSION)
    {
        if (GET_BE_WORD(look->SubTableCount))
        {
            const GPOS_ExtensionPosFormat1 *ext = (const GPOS_ExtensionPosFormat1 *)((const BYTE *)look + GET_BE_WORD(look->SubTable[0]));
            if (GET_BE_WORD(ext->PosFormat) == 1)
            {
                type = GET_BE_WORD(ext->ExtensionLookupType);
                TRACE("extension type %i\n",type);
            }
            else
            {
                FIXME("Unhandled Extension Positioning Format %i\n",GET_BE_WORD(ext->PosFormat));
            }
        }
        else
        {
            WARN("lookup type is Extension Positioning but no extension subtable exists\n");
        }
    }
    switch (type)
    {
        case GPOS_LOOKUP_ADJUST_SINGLE:
        {
            double devX, devY;
            POINT adjust = {0,0};
            POINT advance = {0,0};
            GPOS_apply_SingleAdjustment(look, analysis, glyphs, glyph_index, glyph_count, ppem, &adjust, &advance);
            if (adjust.x || adjust.y)
            {
                GPOS_convert_design_units_to_device(lpotm, lplogfont, adjust.x, adjust.y, &devX, &devY);
                pGoffset[glyph_index].du += round(devX);
                pGoffset[glyph_index].dv += round(devY);
            }
            if (advance.x || advance.y)
            {
                GPOS_convert_design_units_to_device(lpotm, lplogfont, advance.x, advance.y, &devX, &devY);
                piAdvance[glyph_index] += round(devX);
                if (advance.y)
                    FIXME("Unhandled adjustment to Y advancement\n");
            }
            break;
        }

        case GPOS_LOOKUP_ADJUST_PAIR:
        {
            POINT advance[2]= {{0,0},{0,0}};
            POINT adjust[2]= {{0,0},{0,0}};
            double devX, devY;
            int index_offset;
            int write_dir = (analysis->fRTL && !analysis->fLogicalOrder) ? -1 : 1;
            int offset_sign = (analysis->fRTL && analysis->fLogicalOrder) ? -1 : 1;

            index_offset = GPOS_apply_PairAdjustment(look, analysis, glyphs,
                    glyph_index, glyph_count, ppem, adjust, advance);
            if (adjust[0].x || adjust[0].y)
            {
                GPOS_convert_design_units_to_device(lpotm, lplogfont, adjust[0].x, adjust[0].y, &devX, &devY);
                pGoffset[glyph_index].du += round(devX) * offset_sign;
                pGoffset[glyph_index].dv += round(devY);
            }
            if (advance[0].x || advance[0].y)
            {
                GPOS_convert_design_units_to_device(lpotm, lplogfont, advance[0].x, advance[0].y, &devX, &devY);
                piAdvance[glyph_index] += round(devX);
            }
            if (adjust[1].x || adjust[1].y)
            {
                GPOS_convert_design_units_to_device(lpotm, lplogfont, adjust[1].x, adjust[1].y, &devX, &devY);
                pGoffset[glyph_index + write_dir].du += round(devX) * offset_sign;
                pGoffset[glyph_index + write_dir].dv += round(devY);
            }
            if (advance[1].x || advance[1].y)
            {
                GPOS_convert_design_units_to_device(lpotm, lplogfont, advance[1].x, advance[1].y, &devX, &devY);
                piAdvance[glyph_index + write_dir] += round(devX);
            }
            return index_offset;
        }

        case GPOS_LOOKUP_ATTACH_CURSIVE:
        {
            POINT desU = {0,0};
            double devX, devY;
            int write_dir = (analysis->fRTL && !analysis->fLogicalOrder) ? -1 : 1;

            GPOS_apply_CursiveAttachment(look, analysis, glyphs, glyph_index, glyph_count, ppem, &desU);
            if (desU.x || desU.y)
            {
                GPOS_convert_design_units_to_device(lpotm, lplogfont, desU.x, desU.y, &devX, &devY);
                /* Windows does not appear to apply X offsets here */
                pGoffset[glyph_index].dv = round(devY) + pGoffset[glyph_index+write_dir].dv;
            }
            break;
        }

        case GPOS_LOOKUP_ATTACH_MARK_TO_BASE:
        {
            double devX, devY;
            POINT desU = {0,0};
            int base_index = GPOS_apply_MarkToBase(script_cache, look, analysis,
                    glyphs, glyph_index, glyph_count, ppem, &desU);
            if (base_index != -1)
            {
                GPOS_convert_design_units_to_device(lpotm, lplogfont, desU.x, desU.y, &devX, &devY);
                if (!analysis->fRTL) pGoffset[glyph_index].du = round(devX) - piAdvance[base_index];
                else
                {
                    if (analysis->fLogicalOrder) devX *= -1;
                    pGoffset[glyph_index].du = round(devX);
                }
                pGoffset[glyph_index].dv = round(devY);
            }
            break;
        }

        case GPOS_LOOKUP_ATTACH_MARK_TO_LIGATURE:
        {
            double devX, devY;
            POINT desU = {0,0};
            GPOS_apply_MarkToLigature(look, analysis, glyphs, glyph_index, glyph_count, ppem, &desU);
            if (desU.x || desU.y)
            {
                GPOS_convert_design_units_to_device(lpotm, lplogfont, desU.x, desU.y, &devX, &devY);
                pGoffset[glyph_index].du = (round(devX) - piAdvance[glyph_index-1]);
                pGoffset[glyph_index].dv = round(devY);
            }
            break;
        }

        case GPOS_LOOKUP_ATTACH_MARK_TO_MARK:
        {
            double devX, devY;
            POINT desU = {0,0};
            int write_dir = (analysis->fRTL && !analysis->fLogicalOrder) ? -1 : 1;
            if (GPOS_apply_MarkToMark(look, analysis, glyphs, glyph_index, glyph_count, ppem, &desU))
            {
                GPOS_convert_design_units_to_device(lpotm, lplogfont, desU.x, desU.y, &devX, &devY);
                if (analysis->fRTL && analysis->fLogicalOrder) devX *= -1;
                pGoffset[glyph_index].du = round(devX) + pGoffset[glyph_index - write_dir].du;
                pGoffset[glyph_index].dv = round(devY) + pGoffset[glyph_index - write_dir].dv;
            }
            break;
        }

        case GPOS_LOOKUP_POSITION_CONTEXT:
            return GPOS_apply_ContextPos(script_cache, lpotm, lplogfont, analysis, piAdvance,
                    lookup, look, glyphs, glyph_index, glyph_count, pGoffset);

        case GPOS_LOOKUP_POSITION_CONTEXT_CHAINED:
            return GPOS_apply_ChainContextPos(script_cache, lpotm, lplogfont, analysis, piAdvance,
                    lookup, look, glyphs, glyph_index, glyph_count, pGoffset);

        default:
            FIXME("Unhandled GPOS lookup type %#x.\n", type);
    }
    return 1;
}

unsigned int OpenType_apply_GPOS_lookup(const ScriptCache *script_cache, const OUTLINETEXTMETRICW *otm,
        const LOGFONTW *logfont, const SCRIPT_ANALYSIS *analysis, int *advance, unsigned int lookup_index,
        const WORD *glyphs, unsigned int glyph_index, unsigned int glyph_count, GOFFSET *goffset)
{
    const GPOS_Header *header = (const GPOS_Header *)script_cache->GPOS_Table;
    const OT_LookupList *lookup = (const OT_LookupList*)((const BYTE*)header + GET_BE_WORD(header->LookupList));

    return GPOS_apply_lookup(script_cache, otm, logfont, analysis, advance, lookup,
            lookup_index, glyphs, glyph_index, glyph_count, goffset);
}

static LoadedScript *usp10_script_cache_add_script(ScriptCache *script_cache, OPENTYPE_TAG tag)
{
    LoadedScript *script;

    if (!usp10_array_reserve((void **)&script_cache->scripts, &script_cache->scripts_size,
            script_cache->script_count + 1, sizeof(*script_cache->scripts)))
    {
        ERR("Failed to grow scripts array.\n");
        return NULL;
    }

    script = &script_cache->scripts[script_cache->script_count++];
    script->tag = tag;

    return script;
}

static LoadedScript *usp10_script_cache_get_script(ScriptCache *script_cache, OPENTYPE_TAG tag)
{
    size_t i;

    for (i = 0; i < script_cache->script_count; ++i)
    {
        if (script_cache->scripts[i].tag == tag)
            return &script_cache->scripts[i];
    }

    return NULL;
}

static void usp10_script_cache_add_script_list(ScriptCache *script_cache,
        enum usp10_script_table table, const OT_ScriptList *list)
{
    SIZE_T initial_count, count, i;
    LoadedScript *script;
    OPENTYPE_TAG tag;

    TRACE("script_cache %p, table %#x, list %p.\n", script_cache, table, list);

    if (!(count = GET_BE_WORD(list->ScriptCount)))
        return;

    TRACE("Adding %lu scripts.\n", count);

    initial_count = script_cache->script_count;
    for (i = 0; i < count; ++i)
    {
        tag = MS_MAKE_TAG(list->ScriptRecord[i].ScriptTag[0],
                list->ScriptRecord[i].ScriptTag[1],
                list->ScriptRecord[i].ScriptTag[2],
                list->ScriptRecord[i].ScriptTag[3]);

        if (!(initial_count && (script = usp10_script_cache_get_script(script_cache, tag)))
                && !(script = usp10_script_cache_add_script(script_cache, tag)))
            return;

        script->table[table] = (const BYTE *)list + GET_BE_WORD(list->ScriptRecord[i].Script);
    }
}

static void _initialize_script_cache(ScriptCache *script_cache)
{
    const GPOS_Header *gpos_header;
    const GSUB_Header *gsub_header;

    if (script_cache->scripts_initialized)
        return;

    if ((gsub_header = script_cache->GSUB_Table))
        usp10_script_cache_add_script_list(script_cache, USP10_SCRIPT_TABLE_GSUB,
                (const OT_ScriptList *)((const BYTE *)gsub_header + GET_BE_WORD(gsub_header->ScriptList)));

    if ((gpos_header = script_cache->GPOS_Table))
        usp10_script_cache_add_script_list(script_cache, USP10_SCRIPT_TABLE_GPOS,
                (const OT_ScriptList *)((const BYTE *)gpos_header + GET_BE_WORD(gpos_header->ScriptList)));

    script_cache->scripts_initialized = TRUE;
}

HRESULT OpenType_GetFontScriptTags(ScriptCache *psc, OPENTYPE_TAG searchingFor, int cMaxTags, OPENTYPE_TAG *pScriptTags, int *pcTags)
{
    int i;
    const LoadedScript *script;
    HRESULT rc = S_OK;

    _initialize_script_cache(psc);

    *pcTags = psc->script_count;

    if (searchingFor)
    {
        if (!(script = usp10_script_cache_get_script(psc, searchingFor)))
            return USP_E_SCRIPT_NOT_IN_FONT;

        *pScriptTags = script->tag;
        *pcTags = 1;
        return S_OK;
    }

    if (cMaxTags < *pcTags)
        rc = E_OUTOFMEMORY;

    cMaxTags = min(cMaxTags, psc->script_count);
    for (i = 0; i < cMaxTags; ++i)
    {
        pScriptTags[i] = psc->scripts[i].tag;
    }
    return rc;
}

static LoadedLanguage *usp10_script_add_language(LoadedScript *script, OPENTYPE_TAG tag)
{
    LoadedLanguage *language;

    if (!usp10_array_reserve((void **)&script->languages, &script->languages_size,
            script->language_count + 1, sizeof(*script->languages)))
    {
        ERR("Failed to grow languages array.\n");
        return NULL;
    }

    language = &script->languages[script->language_count++];
    language->tag = tag;

    return language;
}

static LoadedLanguage *usp10_script_get_language(LoadedScript *script, OPENTYPE_TAG tag)
{
    size_t i;

    for (i = 0; i < script->language_count; ++i)
    {
        if (script->languages[i].tag == tag)
            return &script->languages[i];
    }

    return NULL;
}

static void usp10_script_add_language_list(LoadedScript *script,
        enum usp10_language_table table, const OT_Script *list)
{
    SIZE_T initial_count, count, i;
    LoadedLanguage *language;
    OPENTYPE_TAG tag;
    DWORD offset;

    TRACE("script %p, table %#x, list %p.\n", script, table, list);

    if ((offset = GET_BE_WORD(list->DefaultLangSys)))
    {
        script->default_language.tag = MS_MAKE_TAG('d','f','l','t');
        script->default_language.table[table] = (const BYTE *)list + offset;
        TRACE("Default language %p.\n", script->default_language.table[table]);
    }

    if (!(count = GET_BE_WORD(list->LangSysCount)))
        return;

    TRACE("Adding %lu languages.\n", count);

    initial_count = script->language_count;
    for (i = 0; i < count; ++i)
    {
        tag = MS_MAKE_TAG(list->LangSysRecord[i].LangSysTag[0],
                list->LangSysRecord[i].LangSysTag[1],
                list->LangSysRecord[i].LangSysTag[2],
                list->LangSysRecord[i].LangSysTag[3]);

        if (!(initial_count && (language = usp10_script_get_language(script, tag)))
                && !(language = usp10_script_add_language(script, tag)))
            return;

        language->table[table] = (const BYTE *)list + GET_BE_WORD(list->LangSysRecord[i].LangSys);
    }
}

static void _initialize_language_cache(LoadedScript *script)
{
    const OT_Script *list;

    if (script->languages_initialized)
        return;

    if ((list = script->table[USP10_SCRIPT_TABLE_GSUB]))
        usp10_script_add_language_list(script, USP10_LANGUAGE_TABLE_GSUB, list);
    if ((list = script->table[USP10_SCRIPT_TABLE_GPOS]))
        usp10_script_add_language_list(script, USP10_LANGUAGE_TABLE_GPOS, list);

    script->languages_initialized = TRUE;
}

HRESULT OpenType_GetFontLanguageTags(ScriptCache *psc, OPENTYPE_TAG script_tag, OPENTYPE_TAG searchingFor, int cMaxTags, OPENTYPE_TAG *pLanguageTags, int *pcTags)
{
    int i;
    HRESULT rc = S_OK;
    LoadedScript *script = NULL;

    _initialize_script_cache(psc);
    if (!(script = usp10_script_cache_get_script(psc, script_tag)))
        return E_INVALIDARG;

    _initialize_language_cache(script);

    if (!searchingFor && cMaxTags < script->language_count)
        rc = E_OUTOFMEMORY;
    else if (searchingFor)
        rc = E_INVALIDARG;

    *pcTags = script->language_count;

    for (i = 0; i < script->language_count; i++)
    {
        if (i < cMaxTags)
            pLanguageTags[i] = script->languages[i].tag;

        if (searchingFor)
        {
            if (searchingFor == script->languages[i].tag)
            {
                pLanguageTags[0] = script->languages[i].tag;
                *pcTags = 1;
                rc = S_OK;
                break;
            }
        }
    }

    if (script->default_language.table[USP10_LANGUAGE_TABLE_GSUB])
    {
        if (i < cMaxTags)
            pLanguageTags[i] = script->default_language.tag;

        if (searchingFor  && FAILED(rc))
        {
            pLanguageTags[0] = script->default_language.tag;
        }
        i++;
        *pcTags = (*pcTags) + 1;
    }

    return rc;
}

static void usp10_language_add_feature_list(LoadedLanguage *language, char table_type,
        const OT_LangSys *lang, const OT_FeatureList *feature_list)
{
    unsigned int count = GET_BE_WORD(lang->FeatureCount);
    unsigned int i, j;

    TRACE("table_type %#x, %u features.\n", table_type, count);

    if (!count || !usp10_array_reserve((void **)&language->features, &language->features_size,
            language->feature_count + count, sizeof(*language->features)))
        return;

    for (i = 0; i < count; ++i)
    {
        const OT_FeatureRecord *record;
        LoadedFeature *loaded_feature;
        const OT_Feature *feature;

        record = &feature_list->FeatureRecord[GET_BE_WORD(lang->FeatureIndex[i])];
        feature = (const OT_Feature *)((const BYTE *)feature_list + GET_BE_WORD(record->Feature));

        loaded_feature = &language->features[language->feature_count + i];
        loaded_feature->tag = MS_MAKE_TAG(record->FeatureTag[0], record->FeatureTag[1],
                record->FeatureTag[2], record->FeatureTag[3]);
        loaded_feature->tableType = table_type;
        loaded_feature->feature = feature;
        loaded_feature->lookup_count = GET_BE_WORD(feature->LookupCount);
        loaded_feature->lookups = heap_calloc(loaded_feature->lookup_count, sizeof(*loaded_feature->lookups));
        for (j = 0; j < loaded_feature->lookup_count; ++j)
            loaded_feature->lookups[j] = GET_BE_WORD(feature->LookupListIndex[j]);
    }
    language->feature_count += count;
}

static void _initialize_feature_cache(ScriptCache *psc, LoadedLanguage *language)
{
    const GSUB_Header *gsub_header = psc->GSUB_Table;
    const GPOS_Header *gpos_header = psc->GPOS_Table;
    const OT_FeatureList *feature_list;
    const OT_LangSys *lang;

    if (language->features_initialized)
        return;

    if ((lang = language->table[USP10_LANGUAGE_TABLE_GSUB]))
    {
        feature_list = (const OT_FeatureList *)((const BYTE *)gsub_header + GET_BE_WORD(gsub_header->FeatureList));
        usp10_language_add_feature_list(language, FEATURE_GSUB_TABLE, lang, feature_list);
    }

    if ((lang = language->table[USP10_LANGUAGE_TABLE_GPOS]))
    {
        feature_list = (const OT_FeatureList *)((const BYTE *)gpos_header + GET_BE_WORD(gpos_header->FeatureList));
        usp10_language_add_feature_list(language, FEATURE_GPOS_TABLE, lang, feature_list);
    }

    language->features_initialized = TRUE;
}

HRESULT OpenType_GetFontFeatureTags(ScriptCache *psc, OPENTYPE_TAG script_tag, OPENTYPE_TAG language_tag, BOOL filtered, OPENTYPE_TAG searchingFor, char tableType, int cMaxTags, OPENTYPE_TAG *pFeatureTags, int *pcTags, LoadedFeature** feature)
{
    int i;
    LoadedLanguage *language;
    LoadedScript *script;
    HRESULT rc = S_OK;

    _initialize_script_cache(psc);
    if (!(script = usp10_script_cache_get_script(psc, script_tag)))
    {
        *pcTags = 0;
        if (!filtered)
            return S_OK;
        else
            return E_INVALIDARG;
    }

    _initialize_language_cache(script);

    language = &script->default_language;
    if (language->tag != language_tag || (!language->table[USP10_LANGUAGE_TABLE_GSUB]
            && !language->table[USP10_LANGUAGE_TABLE_GPOS]))
        language = usp10_script_get_language(script, language_tag);

    if (!language)
    {
        *pcTags = 0;
        return S_OK;
    }

    _initialize_feature_cache(psc, language);

    if (tableType)
    {
        *pcTags = 0;
        for (i = 0; i < language->feature_count; i++)
            if (language->features[i].tableType == tableType)
                *pcTags = (*pcTags)+1;
    }
    else
        *pcTags = language->feature_count;

    if (!searchingFor && cMaxTags < *pcTags)
        rc = E_OUTOFMEMORY;
    else if (searchingFor)
        rc = E_INVALIDARG;

    for (i = 0; i < language->feature_count; i++)
    {
        if (i < cMaxTags)
        {
            if (!tableType || language->features[i].tableType == tableType)
                pFeatureTags[i] = language->features[i].tag;
        }

        if (searchingFor)
        {
            if ((searchingFor == language->features[i].tag) &&
                (!tableType || language->features[i].tableType == tableType))
            {
                pFeatureTags[0] = language->features[i].tag;
                *pcTags = 1;
                if (feature)
                    *feature = &language->features[i];
                rc = S_OK;
                break;
            }
        }
    }
    return rc;
}
