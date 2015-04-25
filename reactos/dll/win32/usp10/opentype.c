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

#include "usp10_internal.h"

#include <winternl.h>

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
    WORD SubstFormat; /* = 1 */
    WORD Coverage;
    WORD ChainSubRuleSetCount;
    WORD ChainSubRuleSet[1];
}GSUB_ChainContextSubstFormat1;

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
            psc->CMAP_Table = HeapAlloc(GetProcessHeap(),0,length);
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

static int compare_group(const void *a, const void* b)
{
    const DWORD *chr = a;
    const CMAP_SegmentedCoverage_group *group = b;

    if (*chr < GET_BE_DWORD(group->startCharCode))
        return -1;
    if (*chr > GET_BE_DWORD(group->endCharCode))
        return 1;
    return 0;
}

DWORD OpenType_CMAP_GetGlyphIndex(HDC hdc, ScriptCache *psc, DWORD utf32c, LPWORD pgi, DWORD flags)
{
    /* BMP: use gdi32 for ease */
    if (utf32c < 0x10000)
    {
        WCHAR ch = utf32c;
        return GetGlyphIndicesW(hdc,&ch, 1, pgi, flags);
    }

    if (!psc->CMAP_format12_Table)
        psc->CMAP_format12_Table = load_CMAP_format12_table(hdc, psc);

    if (flags & GGI_MARK_NONEXISTING_GLYPHS)
        *pgi = 0xffff;
    else
        *pgi = 0;

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
            *pgi = GET_BE_DWORD(group->startGlyphID) + offset;
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

static INT GSUB_is_glyph_covered(LPCVOID table , UINT glyph)
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

static INT GSUB_apply_SingleSubst(const OT_LookupTable *look, WORD *glyphs, INT glyph_index, INT write_dir, INT *glyph_count)
{
    int j;
    TRACE("Single Substitution Subtable\n");

    for (j = 0; j < GET_BE_WORD(look->SubTableCount); j++)
    {
        int offset;
        const GSUB_SingleSubstFormat1 *ssf1;
        offset = GET_BE_WORD(look->SubTable[j]);
        ssf1 = (const GSUB_SingleSubstFormat1*)((const BYTE*)look+offset);
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
        offset = GET_BE_WORD(look->SubTable[j]);
        msf1 = (const GSUB_MultipleSubstFormat1*)((const BYTE*)look+offset);

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

            return glyph_index + (sub_count * write_dir);
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

        offset = GET_BE_WORD(look->SubTable[j]);
        asf1 = (const GSUB_AlternateSubstFormat1*)((const BYTE*)look+offset);
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

        offset = GET_BE_WORD(look->SubTable[j]);
        lsf1 = (const GSUB_LigatureSubstFormat1*)((const BYTE*)look+offset);
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
                        int j;
                        for (j = replaceIdx + 1; j < *glyph_count; j++)
                            glyphs[j] =glyphs[j+CompCount];
                        *glyph_count = *glyph_count - CompCount;
                    }
                    return replaceIdx + write_dir;
                }
            }
        }
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

        offset = GET_BE_WORD(look->SubTable[j]);
        ccsf1 = (const GSUB_ChainContextSubstFormat1*)((const BYTE*)look+offset);
        if (GET_BE_WORD(ccsf1->SubstFormat) == 1)
        {
            static int once;
            if (!once++)
                FIXME("  TODO: subtype 1 (Simple context glyph substitution)\n");
            continue;
        }
        else if (GET_BE_WORD(ccsf1->SubstFormat) == 2)
        {
            static int once;
            if (!once++)
                FIXME("  TODO: subtype 2 (Class-based Chaining Context Glyph Substitution)\n");
            continue;
        }
        else if (GET_BE_WORD(ccsf1->SubstFormat) == 3)
        {
            int k;
            int indexGlyphs;
            const GSUB_ChainContextSubstFormat3_1 *ccsf3_1;
            const GSUB_ChainContextSubstFormat3_2 *ccsf3_2;
            const GSUB_ChainContextSubstFormat3_3 *ccsf3_3;
            const GSUB_ChainContextSubstFormat3_4 *ccsf3_4;
            int newIndex = glyph_index;

            ccsf3_1 = (const GSUB_ChainContextSubstFormat3_1 *)ccsf1;

            TRACE("  subtype 3 (Coverage-based Chaining Context Glyph Substitution)\n");

            for (k = 0; k < GET_BE_WORD(ccsf3_1->BacktrackGlyphCount); k++)
            {
                offset = GET_BE_WORD(ccsf3_1->Coverage[k]);
                if (GSUB_is_glyph_covered((const BYTE*)ccsf3_1+offset, glyphs[glyph_index + (dirBacktrack * (k+1))]) == -1)
                    break;
            }
            if (k != GET_BE_WORD(ccsf3_1->BacktrackGlyphCount))
                continue;
            TRACE("Matched Backtrack\n");

            ccsf3_2 = (const GSUB_ChainContextSubstFormat3_2 *)((BYTE *)ccsf1 +
                    FIELD_OFFSET(GSUB_ChainContextSubstFormat3_1, Coverage[GET_BE_WORD(ccsf3_1->BacktrackGlyphCount)]));

            indexGlyphs = GET_BE_WORD(ccsf3_2->InputGlyphCount);
            for (k = 0; k < indexGlyphs; k++)
            {
                offset = GET_BE_WORD(ccsf3_2->Coverage[k]);
                if (GSUB_is_glyph_covered((const BYTE*)ccsf3_1+offset, glyphs[glyph_index + (write_dir * k)]) == -1)
                    break;
            }
            if (k != indexGlyphs)
                continue;
            TRACE("Matched IndexGlyphs\n");

            ccsf3_3 = (const GSUB_ChainContextSubstFormat3_3 *)((BYTE *)ccsf3_2 +
                    FIELD_OFFSET(GSUB_ChainContextSubstFormat3_2, Coverage[GET_BE_WORD(ccsf3_2->InputGlyphCount)]));

            for (k = 0; k < GET_BE_WORD(ccsf3_3->LookaheadGlyphCount); k++)
            {
                offset = GET_BE_WORD(ccsf3_3->Coverage[k]);
                if (GSUB_is_glyph_covered((const BYTE*)ccsf3_1+offset, glyphs[glyph_index + (dirLookahead * (indexGlyphs + k))]) == -1)
                    break;
            }
            if (k != GET_BE_WORD(ccsf3_3->LookaheadGlyphCount))
                continue;
            TRACE("Matched LookAhead\n");

            ccsf3_4 = (const GSUB_ChainContextSubstFormat3_4 *)((BYTE *)ccsf3_3 +
                    FIELD_OFFSET(GSUB_ChainContextSubstFormat3_3, Coverage[GET_BE_WORD(ccsf3_3->LookaheadGlyphCount)]));

            if (GET_BE_WORD(ccsf3_4->SubstCount))
            {
                for (k = 0; k < GET_BE_WORD(ccsf3_4->SubstCount); k++)
                {
                    int lookupIndex = GET_BE_WORD(ccsf3_4->SubstLookupRecord[k].LookupListIndex);
                    int SequenceIndex = GET_BE_WORD(ccsf3_4->SubstLookupRecord[k].SequenceIndex) * write_dir;

                    TRACE("SUBST: %i -> %i %i\n",k, SequenceIndex, lookupIndex);
                    newIndex = GSUB_apply_lookup(lookup, lookupIndex, glyphs, glyph_index + SequenceIndex, write_dir, glyph_count);
                    if (newIndex == -1)
                    {
                        ERR("Chain failed to generate a glyph\n");
                        continue;
                    }
                }
                return newIndex;
            }
            else return GSUB_E_NOGLYPH;
        }
    }
    return -1;
}

static INT GSUB_apply_lookup(const OT_LookupList* lookup, INT lookup_index, WORD *glyphs, INT glyph_index, INT write_dir, INT *glyph_count)
{
    int offset;
    const OT_LookupTable *look;

    offset = GET_BE_WORD(lookup->Lookup[lookup_index]);
    look = (const OT_LookupTable*)((const BYTE*)lookup + offset);
    TRACE("type %i, flag %x, subtables %i\n",GET_BE_WORD(look->LookupType),GET_BE_WORD(look->LookupFlag),GET_BE_WORD(look->SubTableCount));
    switch(GET_BE_WORD(look->LookupType))
    {
        case 1:
            return GSUB_apply_SingleSubst(look, glyphs, glyph_index, write_dir, glyph_count);
        case 2:
            return GSUB_apply_MultipleSubst(look, glyphs, glyph_index, write_dir, glyph_count);
        case 3:
            return GSUB_apply_AlternateSubst(look, glyphs, glyph_index, write_dir, glyph_count);
        case 4:
            return GSUB_apply_LigatureSubst(look, glyphs, glyph_index, write_dir, glyph_count);
        case 6:
            return GSUB_apply_ChainContextSubst(lookup, look, glyphs, glyph_index, write_dir, glyph_count);
        default:
            FIXME("We do not handle SubType %i\n",GET_BE_WORD(look->LookupType));
    }
    return GSUB_E_NOGLYPH;
}

INT OpenType_apply_GSUB_lookup(LPCVOID table, INT lookup_index, WORD *glyphs, INT glyph_index, INT write_dir, INT *glyph_count)
{
    const GSUB_Header *header = (const GSUB_Header *)table;
    const OT_LookupList *lookup = (const OT_LookupList*)((const BYTE*)header + GET_BE_WORD(header->LookupList));

    return GSUB_apply_lookup(lookup, lookup_index, glyphs, glyph_index, write_dir, glyph_count);
}

/**********
 * GPOS
 **********/
static INT GPOS_apply_lookup(ScriptCache *psc, LPOUTLINETEXTMETRICW lpotm, LPLOGFONTW lplogfont, const SCRIPT_ANALYSIS *analysis, INT* piAdvance,
                             const OT_LookupList* lookup, INT lookup_index, const WORD *glyphs, INT glyph_index, INT glyph_count, GOFFSET *pGoffset);

static INT GPOS_get_device_table_value(const OT_DeviceTable *DeviceTable, WORD ppem)
{
    static const WORD mask[3] = {3,0xf,0xff};
    if (DeviceTable && ppem >= GET_BE_WORD(DeviceTable->StartSize) && ppem  <= GET_BE_WORD(DeviceTable->EndSize))
    {
        int format = GET_BE_WORD(DeviceTable->DeltaFormat);
        int index = ppem - GET_BE_WORD(DeviceTable->StartSize);
        int value;
        TRACE("device table, format %i, index %i\n",format, index);
        index = index << format;
        value = (DeviceTable->DeltaValue[index/sizeof(WORD)] << (index%sizeof(WORD)))&mask[format-1];
        TRACE("offset %i, value %i\n",index, value);
        if (value > mask[format-1]/2)
            value = -1 * ((mask[format-1]+1) - value);
        return value;
    }
    return 0;
}

static VOID GPOS_get_anchor_values(LPCVOID table, LPPOINT pt, WORD ppem)
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

static void GPOS_convert_design_units_to_device(LPOUTLINETEXTMETRICW lpotm, LPLOGFONTW lplogfont, int desX, int desY, double *devX, double *devY)
{
    int emHeight = lpotm->otmTextMetrics.tmAscent + lpotm->otmTextMetrics.tmDescent - lpotm->otmTextMetrics.tmInternalLeading;

    TRACE("emHeight %i lfWidth %i\n",emHeight, lplogfont->lfWidth);
    *devX = (desX * emHeight) / (double)lpotm->otmEMSquare;
    *devY = (desY * emHeight) / (double)lpotm->otmEMSquare;
    if (lplogfont->lfWidth)
        FIXME("Font with lfWidth set no handled properly\n");
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

static VOID GPOS_get_value_record_offsets(const BYTE* head, GPOS_ValueRecord *ValueRecord,  WORD ValueFormat, INT ppem, LPPOINT ptPlacement, LPPOINT ptAdvance)
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

    if (GET_BE_WORD(look->LookupType) == 9)
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

static VOID GPOS_apply_SingleAdjustment(const OT_LookupTable *look, const SCRIPT_ANALYSIS *analysis, const WORD *glyphs, INT glyph_index,
                                        INT glyph_count, INT ppem, LPPOINT ptAdjust, LPPOINT ptAdvance)
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
                GPOS_get_value_record_offsets((const BYTE*)spf1, &ValueRecord,  ValueFormat, ppem, ptAdjust, ptAdvance);
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
                GPOS_get_value_record_offsets((const BYTE*)spf2, &ValueRecord,  ValueFormat, ppem, ptAdjust, ptAdvance);
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
        TRACE( "Glyph 1 resulting cumulative offset is %i,%i design units\n", adjust[0].x, adjust[0].y );
        TRACE( "Glyph 1 resulting cumulative advance is %i,%i design units\n", advance[0].x, advance[0].y );
    }
    if (val_fmt2)
    {
        GPOS_get_value_record_offsets( pos_table, &val_rec2, val_fmt2, ppem, adjust + 1, advance + 1 );
        TRACE( "Glyph 2 resulting cumulative offset is %i,%i design units\n", adjust[1].x, adjust[1].y );
        TRACE( "Glyph 2 resulting cumulative advance is %i,%i design units\n", advance[1].x, advance[1].y );
    }
}

static INT GPOS_apply_PairAdjustment(const OT_LookupTable *look, const SCRIPT_ANALYSIS *analysis, const WORD *glyphs, INT glyph_index,
                                     INT glyph_count, INT ppem, LPPOINT ptAdjust, LPPOINT ptAdvance)
{
    int j;
    int write_dir = (analysis->fRTL && !analysis->fLogicalOrder) ? -1 : 1;

    if (glyph_index + write_dir < 0 || glyph_index + write_dir >= glyph_count) return glyph_index + 1;

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
                        apply_pair_value( ppf1, ValueFormat1, ValueFormat2, pair_val_rec->Value1, ppem, ptAdjust, ptAdvance );
                        if (ValueFormat2) next++;
                        return glyph_index + next;
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

                    apply_pair_value( ppf2, ValueFormat1, ValueFormat2, pair_val, ppem, ptAdjust, ptAdvance );
                    if (ValueFormat2) next++;
                    return glyph_index + next;
                }
            }
        }
        else
            FIXME("Pair Adjustment Positioning: Format %i Unhandled\n",GET_BE_WORD(ppf1->PosFormat));
    }
    return glyph_index+1;
}

static VOID GPOS_apply_CursiveAttachment(const OT_LookupTable *look, const SCRIPT_ANALYSIS *analysis, const WORD *glyphs, INT glyph_index,
                                     INT glyph_count, INT ppem, LPPOINT pt)
{
    int j;
    int write_dir = (analysis->fRTL && !analysis->fLogicalOrder) ? -1 : 1;

    if (glyph_index + write_dir < 0 || glyph_index + write_dir >= glyph_count) return;

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
                    TRACE("Found linkage %x[%i,%i] %x[%i,%i]\n",glyphs[glyph_index], exit_pt.x,exit_pt.y, glyphs[glyph_index+write_dir], entry_pt.x, entry_pt.y);
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

static int GPOS_apply_MarkToBase(ScriptCache *psc, const OT_LookupTable *look, const SCRIPT_ANALYSIS *analysis, const WORD *glyphs, INT glyph_index, INT glyph_count, INT ppem, LPPOINT pt)
{
    int j;
    int write_dir = (analysis->fRTL && !analysis->fLogicalOrder) ? -1 : 1;
    void *glyph_class_table = NULL;
    int rc = -1;

    if (psc->GDEF_Table)
    {
        const GDEF_Header *header = psc->GDEF_Table;
        WORD offset = GET_BE_WORD( header->GlyphClassDef );
        if (offset)
            glyph_class_table = (BYTE *)psc->GDEF_Table + offset;
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
                        ERR("Mark index exeeded mark count\n");
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
                    TRACE("Offset on base is %i,%i design units\n",base_pt.x,base_pt.y);
                    TRACE("Offset on mark is %i,%i design units\n",mark_pt.x, mark_pt.y);
                    pt->x += base_pt.x - mark_pt.x;
                    pt->y += base_pt.y - mark_pt.y;
                    TRACE("Resulting cumulative offset is %i,%i design units\n",pt->x,pt->y);
                    rc = base_glyph;
                }
            }
        }
        else
            FIXME("Unhandled Mark To Base Format %i\n",GET_BE_WORD(mbpf1->PosFormat));
    }
    return rc;
}

static VOID GPOS_apply_MarkToLigature(const OT_LookupTable *look, const SCRIPT_ANALYSIS *analysis, const WORD *glyphs, INT glyph_index,
                                      INT glyph_count, INT ppem, LPPOINT pt)
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
                        ERR("Mark index exeeded mark count\n");
                        return;
                    }
                    mr = &ma->MarkRecord[mark_index];
                    mark_class = GET_BE_WORD(mr->Class);
                    TRACE("Mark Class %i total classes %i\n",mark_class,class_count);
                    offset = GET_BE_WORD(mlpf1->LigatureArray);
                    la = (const GPOS_LigatureArray*)((const BYTE*)mlpf1 + offset);
                    if (ligature_index > GET_BE_WORD(la->LigatureCount))
                    {
                        ERR("Ligature index exeeded ligature count\n");
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
                        ERR("Failed to find avalible ligature connection point\n");
                        return;
                    }

                    GPOS_get_anchor_values((const BYTE*)lt + offset, &ligature_pt, ppem);
                    offset = GET_BE_WORD(mr->MarkAnchor);
                    GPOS_get_anchor_values((const BYTE*)ma + offset, &mark_pt, ppem);
                    TRACE("Offset on ligature is %i,%i design units\n",ligature_pt.x,ligature_pt.y);
                    TRACE("Offset on mark is %i,%i design units\n",mark_pt.x, mark_pt.y);
                    pt->x += ligature_pt.x - mark_pt.x;
                    pt->y += ligature_pt.y - mark_pt.y;
                    TRACE("Resulting cumulative offset is %i,%i design units\n",pt->x,pt->y);
                }
            }
        }
        else
            FIXME("Unhandled Mark To Ligature Format %i\n",GET_BE_WORD(mlpf1->PosFormat));
    }
}

static BOOL GPOS_apply_MarkToMark(const OT_LookupTable *look, const SCRIPT_ANALYSIS *analysis, const WORD *glyphs, INT glyph_index,
                                  INT glyph_count, INT ppem, LPPOINT pt)
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
                    TRACE("Offset on mark2 is %i,%i design units\n",mark2_pt.x,mark2_pt.y);
                    TRACE("Offset on mark is %i,%i design units\n",mark_pt.x, mark_pt.y);
                    pt->x += mark2_pt.x - mark_pt.x;
                    pt->y += mark2_pt.y - mark_pt.y;
                    TRACE("Resulting cumulative offset is %i,%i design units\n",pt->x,pt->y);
                    rc = TRUE;
                }
            }
        }
        else
            FIXME("Unhandled Mark To Mark Format %i\n",GET_BE_WORD(mmpf1->PosFormat));
    }
    return rc;
}

static INT GPOS_apply_ChainContextPos(ScriptCache *psc, LPOUTLINETEXTMETRICW lpotm, LPLOGFONTW lplogfont, const SCRIPT_ANALYSIS *analysis, INT* piAdvance,
                                      const OT_LookupList *lookup, const OT_LookupTable *look, const WORD *glyphs, INT glyph_index,
                                      INT glyph_count, INT ppem, GOFFSET *pGoffset)
{
    int j;
    int write_dir = (analysis->fRTL && !analysis->fLogicalOrder) ? -1 : 1;

    TRACE("Chaining Contextual Positioning Subtable\n");

    for (j = 0; j < GET_BE_WORD(look->SubTableCount); j++)
    {
        int offset;
        const GPOS_ChainContextPosFormat3_1 *ccpf3 = (GPOS_ChainContextPosFormat3_1 *)GPOS_get_subtable(look, j);
        int dirLookahead = write_dir;
        int dirBacktrack = -1 * write_dir;

        if (GET_BE_WORD(ccpf3->PosFormat) == 1)
        {
            static int once;
            if (!once++)
                FIXME("  TODO: subtype 1 (Simple Chaining Context Glyph Positioning)\n");
            continue;
        }
        else if (GET_BE_WORD(ccpf3->PosFormat) == 2)
        {
            static int once;
            if (!once++)
                FIXME("  TODO: subtype 2 (Class-based Chaining Context Glyph Positioning)\n");
            continue;
        }
        else if (GET_BE_WORD(ccpf3->PosFormat) == 3)
        {
            int k;
            int indexGlyphs;
            const GPOS_ChainContextPosFormat3_2 *ccpf3_2;
            const GPOS_ChainContextPosFormat3_3 *ccpf3_3;
            const GPOS_ChainContextPosFormat3_4 *ccpf3_4;

            TRACE("  subtype 3 (Coverage-based Chaining Context Glyph Positioning)\n");

            for (k = 0; k < GET_BE_WORD(ccpf3->BacktrackGlyphCount); k++)
            {
                offset = GET_BE_WORD(ccpf3->Coverage[k]);
                if (GSUB_is_glyph_covered((const BYTE*)ccpf3+offset, glyphs[glyph_index + (dirBacktrack * (k+1))]) == -1)
                    break;
            }
            if (k != GET_BE_WORD(ccpf3->BacktrackGlyphCount))
                continue;
            TRACE("Matched Backtrack\n");

            ccpf3_2 = (const GPOS_ChainContextPosFormat3_2*)((BYTE *)ccpf3 +
                    FIELD_OFFSET(GPOS_ChainContextPosFormat3_1, Coverage[GET_BE_WORD(ccpf3->BacktrackGlyphCount)]));

            indexGlyphs = GET_BE_WORD(ccpf3_2->InputGlyphCount);
            for (k = 0; k < indexGlyphs; k++)
            {
                offset = GET_BE_WORD(ccpf3_2->Coverage[k]);
                if (GSUB_is_glyph_covered((const BYTE*)ccpf3+offset, glyphs[glyph_index + (write_dir * k)]) == -1)
                    break;
            }
            if (k != indexGlyphs)
                continue;
            TRACE("Matched IndexGlyphs\n");

            ccpf3_3 = (const GPOS_ChainContextPosFormat3_3*)((BYTE *)ccpf3_2 +
                    FIELD_OFFSET(GPOS_ChainContextPosFormat3_2, Coverage[GET_BE_WORD(ccpf3_2->InputGlyphCount)]));

            for (k = 0; k < GET_BE_WORD(ccpf3_3->LookaheadGlyphCount); k++)
            {
                offset = GET_BE_WORD(ccpf3_3->Coverage[k]);
                if (GSUB_is_glyph_covered((const BYTE*)ccpf3+offset, glyphs[glyph_index + (dirLookahead * (indexGlyphs + k))]) == -1)
                    break;
            }
            if (k != GET_BE_WORD(ccpf3_3->LookaheadGlyphCount))
                continue;
            TRACE("Matched LookAhead\n");

            ccpf3_4 = (const GPOS_ChainContextPosFormat3_4*)((BYTE *)ccpf3_3 +
                    FIELD_OFFSET(GPOS_ChainContextPosFormat3_3, Coverage[GET_BE_WORD(ccpf3_3->LookaheadGlyphCount)]));

            if (GET_BE_WORD(ccpf3_4->PosCount))
            {
                for (k = 0; k < GET_BE_WORD(ccpf3_4->PosCount); k++)
                {
                    int lookupIndex = GET_BE_WORD(ccpf3_4->PosLookupRecord[k].LookupListIndex);
                    int SequenceIndex = GET_BE_WORD(ccpf3_4->PosLookupRecord[k].SequenceIndex) * write_dir;

                    TRACE("Position: %i -> %i %i\n",k, SequenceIndex, lookupIndex);
                    GPOS_apply_lookup(psc, lpotm, lplogfont, analysis, piAdvance, lookup, lookupIndex, glyphs, glyph_index + SequenceIndex, glyph_count, pGoffset);
                }
                return glyph_index + indexGlyphs + GET_BE_WORD(ccpf3_3->LookaheadGlyphCount);
            }
            else return glyph_index + 1;
        }
        else
            FIXME("Unhandled Chaining Contextual Positioning Format %i\n",GET_BE_WORD(ccpf3->PosFormat));
    }
    return glyph_index + 1;
}

static INT GPOS_apply_lookup(ScriptCache *psc, LPOUTLINETEXTMETRICW lpotm, LPLOGFONTW lplogfont, const SCRIPT_ANALYSIS *analysis, INT* piAdvance, const OT_LookupList* lookup, INT lookup_index, const WORD *glyphs, INT glyph_index, INT glyph_count, GOFFSET *pGoffset)
{
    int offset;
    const OT_LookupTable *look;
    int ppem = lpotm->otmTextMetrics.tmAscent + lpotm->otmTextMetrics.tmDescent - lpotm->otmTextMetrics.tmInternalLeading;
    WORD type;

    offset = GET_BE_WORD(lookup->Lookup[lookup_index]);
    look = (const OT_LookupTable*)((const BYTE*)lookup + offset);
    type = GET_BE_WORD(look->LookupType);
    TRACE("type %i, flag %x, subtables %i\n",type,GET_BE_WORD(look->LookupFlag),GET_BE_WORD(look->SubTableCount));
    if (type == 9)
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
        case 1:
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
        case 2:
        {
            POINT advance[2]= {{0,0},{0,0}};
            POINT adjust[2]= {{0,0},{0,0}};
            double devX, devY;
            int index;
            int write_dir = (analysis->fRTL && !analysis->fLogicalOrder) ? -1 : 1;
            int offset_sign = (analysis->fRTL && analysis->fLogicalOrder) ? -1 : 1;

            index = GPOS_apply_PairAdjustment(look, analysis, glyphs, glyph_index, glyph_count, ppem, adjust, advance);
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
            return index;
        }
        case 3:
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
        case 4:
        {
            double devX, devY;
            POINT desU = {0,0};
            int base_index = GPOS_apply_MarkToBase(psc, look, analysis, glyphs, glyph_index, glyph_count, ppem, &desU);
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
        case 5:
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
        case 6:
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
        case 8:
        {
            return GPOS_apply_ChainContextPos(psc, lpotm, lplogfont, analysis, piAdvance, lookup, look, glyphs, glyph_index, glyph_count, ppem, pGoffset);
        }
        default:
            FIXME("We do not handle SubType %i\n",type);
    }
    return glyph_index+1;
}

INT OpenType_apply_GPOS_lookup(ScriptCache *psc, LPOUTLINETEXTMETRICW lpotm, LPLOGFONTW lplogfont, const SCRIPT_ANALYSIS *analysis, INT* piAdvance, INT lookup_index, const WORD *glyphs, INT glyph_index, INT glyph_count, GOFFSET *pGoffset)
{
    const GPOS_Header *header = (const GPOS_Header *)psc->GPOS_Table;
    const OT_LookupList *lookup = (const OT_LookupList*)((const BYTE*)header + GET_BE_WORD(header->LookupList));

    return GPOS_apply_lookup(psc, lpotm, lplogfont, analysis, piAdvance, lookup, lookup_index, glyphs, glyph_index, glyph_count, pGoffset);
}

static void GSUB_initialize_script_cache(ScriptCache *psc)
{
    int i;

    if (psc->GSUB_Table)
    {
        const OT_ScriptList *script;
        const GSUB_Header* header = (const GSUB_Header*)psc->GSUB_Table;
        script = (const OT_ScriptList*)((const BYTE*)header + GET_BE_WORD(header->ScriptList));
        psc->script_count = GET_BE_WORD(script->ScriptCount);
        TRACE("initializing %i scripts in this font\n",psc->script_count);
        if (psc->script_count)
        {
            psc->scripts = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(LoadedScript) * psc->script_count);
            for (i = 0; i < psc->script_count; i++)
            {
                int offset = GET_BE_WORD(script->ScriptRecord[i].Script);
                psc->scripts[i].tag = MS_MAKE_TAG(script->ScriptRecord[i].ScriptTag[0], script->ScriptRecord[i].ScriptTag[1], script->ScriptRecord[i].ScriptTag[2], script->ScriptRecord[i].ScriptTag[3]);
                psc->scripts[i].gsub_table = ((const BYTE*)script + offset);
            }
        }
    }
}

static void GPOS_expand_script_cache(ScriptCache *psc)
{
    int i, count;
    const OT_ScriptList *script;
    const GPOS_Header* header = (const GPOS_Header*)psc->GPOS_Table;

    if (!header)
        return;

    script = (const OT_ScriptList*)((const BYTE*)header + GET_BE_WORD(header->ScriptList));
    count = GET_BE_WORD(script->ScriptCount);

    if (!count)
        return;

    if (!psc->script_count)
    {
        psc->script_count = count;
        TRACE("initializing %i scripts in this font\n",psc->script_count);
        if (psc->script_count)
        {
            psc->scripts = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(LoadedScript) * psc->script_count);
            for (i = 0; i < psc->script_count; i++)
            {
                int offset = GET_BE_WORD(script->ScriptRecord[i].Script);
                psc->scripts[i].tag = MS_MAKE_TAG(script->ScriptRecord[i].ScriptTag[0], script->ScriptRecord[i].ScriptTag[1], script->ScriptRecord[i].ScriptTag[2], script->ScriptRecord[i].ScriptTag[3]);
                psc->scripts[i].gpos_table = ((const BYTE*)script + offset);
            }
        }
    }
    else
    {
        for (i = 0; i < count; i++)
        {
            int j;
            int offset = GET_BE_WORD(script->ScriptRecord[i].Script);
            OPENTYPE_TAG tag = MS_MAKE_TAG(script->ScriptRecord[i].ScriptTag[0], script->ScriptRecord[i].ScriptTag[1], script->ScriptRecord[i].ScriptTag[2], script->ScriptRecord[i].ScriptTag[3]);
            for (j = 0; j < psc->script_count; j++)
            {
                if (psc->scripts[j].tag == tag)
                {
                    psc->scripts[j].gpos_table = ((const BYTE*)script + offset);
                    break;
                }
            }
            if (j == psc->script_count)
            {
                psc->script_count++;
                psc->scripts = HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,psc->scripts, sizeof(LoadedScript) * psc->script_count);
                psc->scripts[j].tag = tag;
                psc->scripts[j].gpos_table = ((const BYTE*)script + offset);
            }
        }
    }
}

static void _initialize_script_cache(ScriptCache *psc)
{
    if (!psc->scripts_initialized)
    {
        GSUB_initialize_script_cache(psc);
        GPOS_expand_script_cache(psc);
        psc->scripts_initialized = TRUE;
    }
}

HRESULT OpenType_GetFontScriptTags(ScriptCache *psc, OPENTYPE_TAG searchingFor, int cMaxTags, OPENTYPE_TAG *pScriptTags, int *pcTags)
{
    int i;
    HRESULT rc = S_OK;

    _initialize_script_cache(psc);

    *pcTags = psc->script_count;

    if (!searchingFor && cMaxTags < *pcTags)
        rc = E_OUTOFMEMORY;
    else if (searchingFor)
        rc = USP_E_SCRIPT_NOT_IN_FONT;

    for (i = 0; i < psc->script_count; i++)
    {
        if (i < cMaxTags)
            pScriptTags[i] = psc->scripts[i].tag;

        if (searchingFor)
        {
            if (searchingFor == psc->scripts[i].tag)
            {
                pScriptTags[0] = psc->scripts[i].tag;
                *pcTags = 1;
                rc = S_OK;
                break;
            }
        }
    }
    return rc;
}

static void GSUB_initialize_language_cache(LoadedScript *script)
{
    int i;

    if (script->gsub_table)
    {
        DWORD offset;
        const OT_Script* table = script->gsub_table;
        script->language_count = GET_BE_WORD(table->LangSysCount);
        offset = GET_BE_WORD(table->DefaultLangSys);
        if (offset)
        {
            script->default_language.tag = MS_MAKE_TAG('d','f','l','t');
            script->default_language.gsub_table = (const BYTE*)table + offset;
        }

        if (script->language_count)
        {
            TRACE("Deflang %p, LangCount %i\n",script->default_language.gsub_table, script->language_count);

            script->languages = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LoadedLanguage) * script->language_count);

            for (i = 0; i < script->language_count; i++)
            {
                int offset = GET_BE_WORD(table->LangSysRecord[i].LangSys);
                script->languages[i].tag = MS_MAKE_TAG(table->LangSysRecord[i].LangSysTag[0], table->LangSysRecord[i].LangSysTag[1], table->LangSysRecord[i].LangSysTag[2], table->LangSysRecord[i].LangSysTag[3]);
                script->languages[i].gsub_table = ((const BYTE*)table + offset);
            }
        }
    }
}

static void GPOS_expand_language_cache(LoadedScript *script)
{
    int count;
    const OT_Script* table = script->gpos_table;
    DWORD offset;

    if (!table)
        return;

    offset = GET_BE_WORD(table->DefaultLangSys);
    if (offset)
        script->default_language.gpos_table = (const BYTE*)table + offset;

    count = GET_BE_WORD(table->LangSysCount);

    TRACE("Deflang %p, LangCount %i\n",script->default_language.gpos_table, count);

    if (!count)
        return;

    if (!script->language_count)
    {
        int i;
        script->language_count = count;

        script->languages = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LoadedLanguage) * script->language_count);

        for (i = 0; i < script->language_count; i++)
        {
            int offset = GET_BE_WORD(table->LangSysRecord[i].LangSys);
            script->languages[i].tag = MS_MAKE_TAG(table->LangSysRecord[i].LangSysTag[0], table->LangSysRecord[i].LangSysTag[1], table->LangSysRecord[i].LangSysTag[2], table->LangSysRecord[i].LangSysTag[3]);
            script->languages[i].gpos_table = ((const BYTE*)table + offset);
        }
    }
    else if (count)
    {
        int i,j;
        for (i = 0; i < count; i++)
        {
            int offset = GET_BE_WORD(table->LangSysRecord[i].LangSys);
            OPENTYPE_TAG tag = MS_MAKE_TAG(table->LangSysRecord[i].LangSysTag[0], table->LangSysRecord[i].LangSysTag[1], table->LangSysRecord[i].LangSysTag[2], table->LangSysRecord[i].LangSysTag[3]);

            for (j = 0; j < script->language_count; j++)
            {
                if (script->languages[j].tag == tag)
                {
                    script->languages[j].gpos_table = ((const BYTE*)table + offset);
                    break;
                }
            }
            if (j == script->language_count)
            {
                script->language_count++;
                script->languages = HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,script->languages, sizeof(LoadedLanguage) * script->language_count);
                script->languages[j].tag = tag;
                script->languages[j].gpos_table = ((const BYTE*)table + offset);
            }
        }
    }
}

static void _initialize_language_cache(LoadedScript *script)
{
    if (!script->languages_initialized)
    {
        GSUB_initialize_language_cache(script);
        GPOS_expand_language_cache(script);
        script->languages_initialized = TRUE;
    }
}

HRESULT OpenType_GetFontLanguageTags(ScriptCache *psc, OPENTYPE_TAG script_tag, OPENTYPE_TAG searchingFor, int cMaxTags, OPENTYPE_TAG *pLanguageTags, int *pcTags)
{
    int i;
    HRESULT rc = S_OK;
    LoadedScript *script = NULL;

    _initialize_script_cache(psc);

    for (i = 0; i < psc->script_count; i++)
    {
         if (psc->scripts[i].tag == script_tag)
         {
            script = &psc->scripts[i];
            break;
         }
    }

    if (!script)
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

    if (script->default_language.gsub_table)
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


static void GSUB_initialize_feature_cache(LPCVOID table, LoadedLanguage *language)
{
    int i;

    if (language->gsub_table)
    {
        const OT_LangSys *lang = language->gsub_table;
        const GSUB_Header *header = (const GSUB_Header *)table;
        const OT_FeatureList *feature_list;

        language->feature_count = GET_BE_WORD(lang->FeatureCount);
        TRACE("%i features\n",language->feature_count);

        if (language->feature_count)
        {
            language->features = HeapAlloc(GetProcessHeap(),0,sizeof(LoadedFeature)*language->feature_count);

            feature_list = (const OT_FeatureList*)((const BYTE*)header + GET_BE_WORD(header->FeatureList));

            for (i = 0; i < language->feature_count; i++)
            {
                const OT_Feature *feature;
                int j;
                int index = GET_BE_WORD(lang->FeatureIndex[i]);

                language->features[i].tag = MS_MAKE_TAG(feature_list->FeatureRecord[index].FeatureTag[0], feature_list->FeatureRecord[index].FeatureTag[1], feature_list->FeatureRecord[index].FeatureTag[2], feature_list->FeatureRecord[index].FeatureTag[3]);
                language->features[i].feature = ((const BYTE*)feature_list + GET_BE_WORD(feature_list->FeatureRecord[index].Feature));
                feature = (const OT_Feature*)language->features[i].feature;
                language->features[i].lookup_count = GET_BE_WORD(feature->LookupCount);
                language->features[i].lookups = HeapAlloc(GetProcessHeap(),0,sizeof(WORD) * language->features[i].lookup_count);
                for (j = 0; j < language->features[i].lookup_count; j++)
                    language->features[i].lookups[j] = GET_BE_WORD(feature->LookupListIndex[j]);
                language->features[i].tableType = FEATURE_GSUB_TABLE;
            }
        }
    }
}

static void GPOS_expand_feature_cache(LPCVOID table, LoadedLanguage *language)
{
    int i, count;
    const OT_LangSys *lang = language->gpos_table;
    const GPOS_Header *header = (const GPOS_Header *)table;
    const OT_FeatureList *feature_list;

    if (!lang)
        return;

    count = GET_BE_WORD(lang->FeatureCount);
    feature_list = (const OT_FeatureList*)((const BYTE*)header + GET_BE_WORD(header->FeatureList));

    TRACE("%i features\n",count);

    if (!count)
        return;

    if (!language->feature_count)
    {
        language->feature_count = count;

        if (language->feature_count)
        {
            language->features = HeapAlloc(GetProcessHeap(),0,sizeof(LoadedFeature)*language->feature_count);

            for (i = 0; i < language->feature_count; i++)
            {
                const OT_Feature *feature;
                int j;
                int index = GET_BE_WORD(lang->FeatureIndex[i]);

                language->features[i].tag = MS_MAKE_TAG(feature_list->FeatureRecord[index].FeatureTag[0], feature_list->FeatureRecord[index].FeatureTag[1], feature_list->FeatureRecord[index].FeatureTag[2], feature_list->FeatureRecord[index].FeatureTag[3]);
                language->features[i].feature = ((const BYTE*)feature_list + GET_BE_WORD(feature_list->FeatureRecord[index].Feature));
                feature = (const OT_Feature*)language->features[i].feature;
                language->features[i].lookup_count = GET_BE_WORD(feature->LookupCount);
                language->features[i].lookups = HeapAlloc(GetProcessHeap(),0,sizeof(WORD) * language->features[i].lookup_count);
                for (j = 0; j < language->features[i].lookup_count; j++)
                    language->features[i].lookups[j] = GET_BE_WORD(feature->LookupListIndex[j]);
                language->features[i].tableType = FEATURE_GPOS_TABLE;
            }
        }
    }
    else
    {
        language->features = HeapReAlloc(GetProcessHeap(),0,language->features, sizeof(LoadedFeature)*(language->feature_count + count));

        for (i = 0; i < count; i++)
        {
            const OT_Feature *feature;
            int j;
            int index = GET_BE_WORD(lang->FeatureIndex[i]);
            int idx = language->feature_count + i;

            language->features[idx].tag = MS_MAKE_TAG(feature_list->FeatureRecord[index].FeatureTag[0], feature_list->FeatureRecord[index].FeatureTag[1], feature_list->FeatureRecord[index].FeatureTag[2], feature_list->FeatureRecord[index].FeatureTag[3]);
            language->features[idx].feature = ((const BYTE*)feature_list + GET_BE_WORD(feature_list->FeatureRecord[index].Feature));
            feature = (const OT_Feature*)language->features[idx].feature;
            language->features[idx].lookup_count = GET_BE_WORD(feature->LookupCount);
            language->features[idx].lookups = HeapAlloc(GetProcessHeap(),0,sizeof(WORD) * language->features[idx].lookup_count);
            for (j = 0; j < language->features[idx].lookup_count; j++)
                language->features[idx].lookups[j] = GET_BE_WORD(feature->LookupListIndex[j]);
            language->features[idx].tableType = FEATURE_GPOS_TABLE;
        }
        language->feature_count += count;
    }
}

static void _initialize_feature_cache(ScriptCache *psc, LoadedLanguage *language)
{
    if (!language->features_initialized)
    {
        GSUB_initialize_feature_cache(psc->GSUB_Table, language);
        GPOS_expand_feature_cache(psc->GPOS_Table, language);
        language->features_initialized = TRUE;
    }
}

HRESULT OpenType_GetFontFeatureTags(ScriptCache *psc, OPENTYPE_TAG script_tag, OPENTYPE_TAG language_tag, BOOL filtered, OPENTYPE_TAG searchingFor, char tableType, int cMaxTags, OPENTYPE_TAG *pFeatureTags, int *pcTags, LoadedFeature** feature)
{
    int i;
    HRESULT rc = S_OK;
    LoadedScript *script = NULL;
    LoadedLanguage *language = NULL;

    _initialize_script_cache(psc);

    for (i = 0; i < psc->script_count; i++)
    {
        if (psc->scripts[i].tag == script_tag)
        {
            script = &psc->scripts[i];
            break;
        }
    }

    if (!script)
    {
        *pcTags = 0;
        if (!filtered)
            return S_OK;
        else
            return E_INVALIDARG;
    }

    _initialize_language_cache(script);

    if ((script->default_language.gsub_table || script->default_language.gpos_table) && script->default_language.tag == language_tag)
        language = &script->default_language;
    else
    {
        for (i = 0; i < script->language_count; i++)
        {
            if (script->languages[i].tag == language_tag)
            {
                language = &script->languages[i];
                break;
            }
        }
    }

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
