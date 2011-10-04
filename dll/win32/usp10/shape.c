/*
 * Implementation of Shaping for the Uniscribe Script Processor (usp10.dll)
 *
 * Copyright 2010 CodeWeavers, Aric Stewart
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

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "usp10.h"
#include "winternl.h"

#include "usp10_internal.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uniscribe);

#define FIRST_ARABIC_CHAR 0x0600
#define LAST_ARABIC_CHAR  0x06ff

extern const unsigned short wine_shaping_table[];
extern const unsigned short wine_shaping_forms[LAST_ARABIC_CHAR - FIRST_ARABIC_CHAR + 1][4];

enum joining_types {
    jtU,
    jtT,
    jtR,
    jtL,
    jtD,
    jtC
};

enum joined_forms {
    Xn=0,
    Xr,
    Xl,
    Xm
};

#ifdef WORDS_BIGENDIAN
#define GET_BE_WORD(x) (x)
#else
#define GET_BE_WORD(x) RtlUshortByteSwap(x)
#endif

/* These are all structures needed for the GSUB table */
#define MS_MAKE_TAG( _x1, _x2, _x3, _x4 ) \
          ( ( (ULONG)_x4 << 24 ) |     \
            ( (ULONG)_x3 << 16 ) |     \
            ( (ULONG)_x2 <<  8 ) |     \
              (ULONG)_x1         )

#define GSUB_TAG MS_MAKE_TAG('G', 'S', 'U', 'B')
#define GSUB_E_NOFEATURE -2
#define GSUB_E_NOGLYPH -1

typedef struct {
    DWORD version;
    WORD ScriptList;
    WORD FeatureList;
    WORD LookupList;
} GSUB_Header;

typedef struct {
    CHAR ScriptTag[4];
    WORD Script;
} GSUB_ScriptRecord;

typedef struct {
    WORD ScriptCount;
    GSUB_ScriptRecord ScriptRecord[1];
} GSUB_ScriptList;

typedef struct {
    CHAR LangSysTag[4];
    WORD LangSys;
} GSUB_LangSysRecord;

typedef struct {
    WORD DefaultLangSys;
    WORD LangSysCount;
    GSUB_LangSysRecord LangSysRecord[1];
} GSUB_Script;

typedef struct {
    WORD LookupOrder; /* Reserved */
    WORD ReqFeatureIndex;
    WORD FeatureCount;
    WORD FeatureIndex[1];
} GSUB_LangSys;

typedef struct {
    CHAR FeatureTag[4];
    WORD Feature;
} GSUB_FeatureRecord;

typedef struct {
    WORD FeatureCount;
    GSUB_FeatureRecord FeatureRecord[1];
} GSUB_FeatureList;

typedef struct {
    WORD FeatureParams; /* Reserved */
    WORD LookupCount;
    WORD LookupListIndex[1];
} GSUB_Feature;

typedef struct {
    WORD LookupCount;
    WORD Lookup[1];
} GSUB_LookupList;

typedef struct {
    WORD LookupType;
    WORD LookupFlag;
    WORD SubTableCount;
    WORD SubTable[1];
} GSUB_LookupTable;

typedef struct {
    WORD CoverageFormat;
    WORD GlyphCount;
    WORD GlyphArray[1];
} GSUB_CoverageFormat1;

typedef struct {
    WORD Start;
    WORD End;
    WORD StartCoverageIndex;
} GSUB_RangeRecord;

typedef struct {
    WORD CoverageFormat;
    WORD RangeCount;
    GSUB_RangeRecord RangeRecord[1];
} GSUB_CoverageFormat2;

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

static INT GSUB_apply_lookup(const GSUB_LookupList* lookup, INT lookup_index, WORD *glyphs, INT glyph_index, INT write_dir, INT *glyph_count);

/* the orders of joined_forms and contextual_features need to line up */
static const char* contextual_features[] =
{
    "isol",
    "fina",
    "init",
    "medi"
};

static const char* arabic_GSUB_features[] =
{
    "rlig",
    "calt",
    "liga",
    "dlig",
    "cswh",
    "mset",
    NULL
};

static INT GSUB_is_glyph_covered(LPCVOID table , UINT glyph)
{
    const GSUB_CoverageFormat1* cf1;

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
        const GSUB_CoverageFormat2* cf2;
        int i;
        int count;
        cf2 = (const GSUB_CoverageFormat2*)cf1;

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

static const GSUB_Script* GSUB_get_script_table( const GSUB_Header* header, const char* tag)
{
    const GSUB_ScriptList *script;
    const GSUB_Script *deflt = NULL;
    int i;
    script = (const GSUB_ScriptList*)((const BYTE*)header + GET_BE_WORD(header->ScriptList));

    TRACE("%i scripts in this font\n",GET_BE_WORD(script->ScriptCount));
    for (i = 0; i < GET_BE_WORD(script->ScriptCount); i++)
    {
        const GSUB_Script *scr;
        int offset;

        offset = GET_BE_WORD(script->ScriptRecord[i].Script);
        scr = (const GSUB_Script*)((const BYTE*)script + offset);

        if (strncmp(script->ScriptRecord[i].ScriptTag, tag,4)==0)
            return scr;
        if (strncmp(script->ScriptRecord[i].ScriptTag, "dflt",4)==0)
            deflt = scr;
    }
    return deflt;
}

static const GSUB_LangSys* GSUB_get_lang_table( const GSUB_Script* script, const char* tag)
{
    int i;
    int offset;
    const GSUB_LangSys *Lang;

    TRACE("Deflang %x, LangCount %i\n",GET_BE_WORD(script->DefaultLangSys), GET_BE_WORD(script->LangSysCount));

    for (i = 0; i < GET_BE_WORD(script->LangSysCount) ; i++)
    {
        offset = GET_BE_WORD(script->LangSysRecord[i].LangSys);
        Lang = (const GSUB_LangSys*)((const BYTE*)script + offset);

        if ( strncmp(script->LangSysRecord[i].LangSysTag,tag,4)==0)
            return Lang;
    }
    offset = GET_BE_WORD(script->DefaultLangSys);
    if (offset)
    {
        Lang = (const GSUB_LangSys*)((const BYTE*)script + offset);
        return Lang;
    }
    return NULL;
}

static const GSUB_Feature * GSUB_get_feature(const GSUB_Header *header, const GSUB_LangSys *lang, const char* tag)
{
    int i;
    const GSUB_FeatureList *feature;
    feature = (const GSUB_FeatureList*)((const BYTE*)header + GET_BE_WORD(header->FeatureList));

    TRACE("%i features\n",GET_BE_WORD(lang->FeatureCount));
    for (i = 0; i < GET_BE_WORD(lang->FeatureCount); i++)
    {
        int index = GET_BE_WORD(lang->FeatureIndex[i]);
        if (strncmp(feature->FeatureRecord[index].FeatureTag,tag,4)==0)
        {
            const GSUB_Feature *feat;
            feat = (const GSUB_Feature*)((const BYTE*)feature + GET_BE_WORD(feature->FeatureRecord[index].Feature));
            return feat;
        }
    }
    return NULL;
}

static INT GSUB_apply_SingleSubst(const GSUB_LookupTable *look, WORD *glyphs, INT glyph_index, INT write_dir, INT *glyph_count)
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
                return glyph_index + 1;
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
                TRACE("    Glyph is 0x%x ->",glyphs[glyph_index]);
                glyphs[glyph_index] = GET_BE_WORD(ssf2->Substitute[index]);
                TRACE("0x%x\n",glyphs[glyph_index]);
                return glyph_index + 1;
            }
        }
    }
    return GSUB_E_NOGLYPH;
}

static INT GSUB_apply_LigatureSubst(const GSUB_LookupTable *look, WORD *glyphs, INT glyph_index, INT write_dir, INT *glyph_count)
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
                    return replaceIdx + 1;
                }
            }
        }
    }
    return GSUB_E_NOGLYPH;
}

static INT GSUB_apply_ChainContextSubst(const GSUB_LookupList* lookup, const GSUB_LookupTable *look, WORD *glyphs, INT glyph_index, INT write_dir, INT *glyph_count)
{
    int j;
    BOOL done = FALSE;

    TRACE("Chaining Contextual Substitution Subtable\n");
    for (j = 0; j < GET_BE_WORD(look->SubTableCount) && !done; j++)
    {
        const GSUB_ChainContextSubstFormat1 *ccsf1;
        int offset;
        int dirLookahead = write_dir;
        int dirBacktrack = -1 * write_dir;

        offset = GET_BE_WORD(look->SubTable[j]);
        ccsf1 = (const GSUB_ChainContextSubstFormat1*)((const BYTE*)look+offset);
        if (GET_BE_WORD(ccsf1->SubstFormat) == 1)
        {
            FIXME("  TODO: subtype 1 (Simple context glyph substitution)\n");
            return -1;
        }
        else if (GET_BE_WORD(ccsf1->SubstFormat) == 2)
        {
            FIXME("  TODO: subtype 2 (Class-based Chaining Context Glyph Substitution)\n");
            return -1;
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
                return -1;
            TRACE("Matched Backtrack\n");

            ccsf3_2 = (const GSUB_ChainContextSubstFormat3_2 *)(((LPBYTE)ccsf1)+sizeof(GSUB_ChainContextSubstFormat3_1) + (sizeof(WORD) * (GET_BE_WORD(ccsf3_1->BacktrackGlyphCount)-1)));

            indexGlyphs = GET_BE_WORD(ccsf3_2->InputGlyphCount);
            for (k = 0; k < indexGlyphs; k++)
            {
                offset = GET_BE_WORD(ccsf3_2->Coverage[k]);
                if (GSUB_is_glyph_covered((const BYTE*)ccsf3_1+offset, glyphs[glyph_index + (write_dir * k)]) == -1)
                    break;
            }
            if (k != indexGlyphs)
                return -1;
            TRACE("Matched IndexGlyphs\n");

            ccsf3_3 = (const GSUB_ChainContextSubstFormat3_3 *)(((LPBYTE)ccsf3_2)+sizeof(GSUB_ChainContextSubstFormat3_2) + (sizeof(WORD) * (GET_BE_WORD(ccsf3_2->InputGlyphCount)-1)));

            for (k = 0; k < GET_BE_WORD(ccsf3_3->LookaheadGlyphCount); k++)
            {
                offset = GET_BE_WORD(ccsf3_3->Coverage[k]);
                if (GSUB_is_glyph_covered((const BYTE*)ccsf3_1+offset, glyphs[glyph_index + (dirLookahead * (indexGlyphs + k+1))]) == -1)
                    break;
            }
            if (k != GET_BE_WORD(ccsf3_3->LookaheadGlyphCount))
                return -1;
            TRACE("Matched LookAhead\n");

            ccsf3_4 = (const GSUB_ChainContextSubstFormat3_4 *)(((LPBYTE)ccsf3_3)+sizeof(GSUB_ChainContextSubstFormat3_3) + (sizeof(WORD) * (GET_BE_WORD(ccsf3_3->LookaheadGlyphCount)-1)));

            for (k = 0; k < GET_BE_WORD(ccsf3_4->SubstCount); k++)
            {
                int lookupIndex = GET_BE_WORD(ccsf3_4->SubstLookupRecord[k].LookupListIndex);
                int SequenceIndex = GET_BE_WORD(ccsf3_4->SubstLookupRecord[k].SequenceIndex) * write_dir;

                TRACE("SUBST: %i -> %i %i\n",k, SequenceIndex, lookupIndex);
                newIndex = GSUB_apply_lookup(lookup, lookupIndex, glyphs, glyph_index + SequenceIndex, write_dir, glyph_count);
                if (newIndex == -1)
                {
                    ERR("Chain failed to generate a glyph\n");
                    return -1;
                }
            }
            return newIndex + 1;
        }
    }
    return -1;
}

static INT GSUB_apply_lookup(const GSUB_LookupList* lookup, INT lookup_index, WORD *glyphs, INT glyph_index, INT write_dir, INT *glyph_count)
{
    int offset;
    const GSUB_LookupTable *look;

    offset = GET_BE_WORD(lookup->Lookup[lookup_index]);
    look = (const GSUB_LookupTable*)((const BYTE*)lookup + offset);
    TRACE("type %i, flag %x, subtables %i\n",GET_BE_WORD(look->LookupType),GET_BE_WORD(look->LookupFlag),GET_BE_WORD(look->SubTableCount));
    switch(GET_BE_WORD(look->LookupType))
    {
        case 1:
            return GSUB_apply_SingleSubst(look, glyphs, glyph_index, write_dir, glyph_count);
        case 4:
            return GSUB_apply_LigatureSubst(look, glyphs, glyph_index, write_dir, glyph_count);
        case 6:
            return GSUB_apply_ChainContextSubst(lookup, look, glyphs, glyph_index, write_dir, glyph_count);
        default:
            FIXME("We do not handle SubType %i\n",GET_BE_WORD(look->LookupType));
    }
    return GSUB_E_NOGLYPH;
}

static INT GSUB_apply_feature(const GSUB_Header * header, const GSUB_Feature* feature, WORD *glyphs, INT glyph_index, INT write_dir, INT *glyph_count)
{
    int i;
    int out_index = GSUB_E_NOGLYPH;
    const GSUB_LookupList *lookup;

    lookup = (const GSUB_LookupList*)((const BYTE*)header + GET_BE_WORD(header->LookupList));

    TRACE("%i lookups\n", GET_BE_WORD(feature->LookupCount));
    for (i = 0; i < GET_BE_WORD(feature->LookupCount); i++)
    {
        out_index = GSUB_apply_lookup(lookup, GET_BE_WORD(feature->LookupListIndex[i]), glyphs, glyph_index, write_dir, glyph_count);
        if (out_index != GSUB_E_NOGLYPH)
            break;
    }
    if (out_index == GSUB_E_NOGLYPH)
        TRACE("lookups found no glyphs\n");
    return out_index;
}

static const char* get_opentype_script(HDC hdc, SCRIPT_ANALYSIS *psa)
{
    UINT charset;

    switch (psa->eScript)
    {
        case Script_Arabic:
            return "arab";
        case Script_Syriac:
            return "syrc";
        case Script_Hebrew:
            return "hebr";
        case Script_Latin:
        case Script_Numeric:
        case Script_CR:
        case Script_LF:
            return "latn";
    }

    /*
     * fall back to the font charset
     */
    charset = GetTextCharsetInfo(hdc, NULL, 0x0);
    switch (charset)
    {
        case ANSI_CHARSET: return "latn";
        case BALTIC_CHARSET: return "latn"; /* ?? */
        case CHINESEBIG5_CHARSET: return "hani";
        case EASTEUROPE_CHARSET: return "latn"; /* ?? */
        case GB2312_CHARSET: return "hani";
        case GREEK_CHARSET: return "grek";
        case HANGUL_CHARSET: return "hang";
        case RUSSIAN_CHARSET: return "cyrl";
        case SHIFTJIS_CHARSET: return "kana";
        case TURKISH_CHARSET: return "latn"; /* ?? */
        case VIETNAMESE_CHARSET: return "latn";
        case JOHAB_CHARSET: return "latn"; /* ?? */
        case ARABIC_CHARSET: return "arab";
        case HEBREW_CHARSET: return "hebr";
        case THAI_CHARSET: return "thai";
        default: return "latn";
    }
}

static INT apply_GSUB_feature_to_glyph(HDC hdc, SCRIPT_ANALYSIS *psa, void* GSUB_Table, WORD *glyphs, INT index, INT write_dir, INT* glyph_count, const char* feat)
{
    const GSUB_Header *header;
    const GSUB_Script *script;
    const GSUB_LangSys *language;
    const GSUB_Feature *feature;

    if (!GSUB_Table)
        return GSUB_E_NOFEATURE;

    header = GSUB_Table;

    script = GSUB_get_script_table(header, get_opentype_script(hdc,psa));
    if (!script)
    {
        TRACE("Script not found\n");
        return GSUB_E_NOFEATURE;
    }
    language = GSUB_get_lang_table(script, "xxxx"); /* Need to get Lang tag */
    if (!language)
    {
        TRACE("Language not found\n");
        return GSUB_E_NOFEATURE;
    }
    feature  =  GSUB_get_feature(header, language, feat);
    if (!feature)
    {
        TRACE("%s feature not found\n",feat);
        return GSUB_E_NOFEATURE;
    }
    TRACE("applying feature %s\n",feat);
    return GSUB_apply_feature(header, feature, glyphs, index, write_dir, glyph_count);
}

static VOID *load_gsub_table(HDC hdc)
{
    VOID* GSUB_Table = NULL;
    int length = GetFontData(hdc, GSUB_TAG , 0, NULL, 0);
    if (length != GDI_ERROR)
    {
        GSUB_Table = HeapAlloc(GetProcessHeap(),0,length);
        GetFontData(hdc, GSUB_TAG , 0, GSUB_Table, length);
        TRACE("Loaded GSUB table of %i bytes\n",length);
    }
    return GSUB_Table;
}

static int apply_GSUB_feature(HDC hdc, SCRIPT_ANALYSIS *psa, void* GSUB_Table, WORD *pwOutGlyphs, int write_dir, INT* pcGlyphs, const char* feat)
{
    int i;

    if (GSUB_Table)
    {
        const GSUB_Header *header;
        const GSUB_Script *script;
        const GSUB_LangSys *language;
        const GSUB_Feature *feature;

        if (!GSUB_Table)
            return GSUB_E_NOFEATURE;

        header = GSUB_Table;

        script = GSUB_get_script_table(header, get_opentype_script(hdc,psa));
        if (!script)
        {
            TRACE("Script not found\n");
            return GSUB_E_NOFEATURE;
        }
        language = GSUB_get_lang_table(script, "xxxx");
        if (!language)
        {
            TRACE("Language not found\n");
            return GSUB_E_NOFEATURE;
        }
        feature  =  GSUB_get_feature(header, language, feat);
        if (!feature)
        {
            TRACE("%s feature not found\n",feat);
            return GSUB_E_NOFEATURE;
        }

        i = 0;
        TRACE("applying feature %s\n",feat);
        while(i < *pcGlyphs)
        {
                INT nextIndex;
                nextIndex = GSUB_apply_feature(header, feature, pwOutGlyphs, i, write_dir, pcGlyphs);
                if (nextIndex > GSUB_E_NOGLYPH)
                    i = nextIndex;
                else
                    i++;
        }
        return *pcGlyphs;
    }
    return GSUB_E_NOFEATURE;
}

static CHAR neighbour_joining_type(int i, int delta, const CHAR* context_type, INT cchLen, SCRIPT_ANALYSIS *psa)
{
    if (i + delta < 0)
    {
        if (psa->fLinkBefore)
            return jtR;
        else
            return jtU;
    }
    if ( i+ delta >= cchLen)
    {
        if (psa->fLinkAfter)
            return jtL;
        else
            return jtU;
    }

    i += delta;

    if (context_type[i] == jtT)
        return neighbour_joining_type(i,delta,context_type,cchLen,psa);
    else
        return context_type[i];
}

static inline BOOL right_join_causing(CHAR joining_type)
{
    return (joining_type == jtL || joining_type == jtD || joining_type == jtC);
}

static inline BOOL left_join_causing(CHAR joining_type)
{
    return (joining_type == jtR || joining_type == jtD || joining_type == jtC);
}

/* SHAPE_ShapeArabicGlyphs
 */
void SHAPE_ShapeArabicGlyphs(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs)
{
    CHAR *context_type;
    INT *context_shape;
    INT dirR, dirL;
    int i;

    if (psa->eScript != Script_Arabic)
        return;

    if (*pcGlyphs != cChars)
    {
        ERR("Number of Glyphs and Chars need to match at the beginning\n");
        return;
    }


    if (!psa->fLogicalOrder && psa->fRTL)
    {
        dirR = 1;
        dirL = -1;
    }
    else
    {
        dirR = -1;
        dirL = 1;
    }

    if (!psc->GSUB_Table)
        psc->GSUB_Table = load_gsub_table(hdc);

    context_type = HeapAlloc(GetProcessHeap(),0,cChars);
    context_shape = HeapAlloc(GetProcessHeap(),0,sizeof(INT) * cChars);

    for (i = 0; i < cChars; i++)
        context_type[i] = wine_shaping_table[wine_shaping_table[pwcChars[i] >> 8] + (pwcChars[i] & 0xff)];

    for (i = 0; i < cChars; i++)
    {
        if (context_type[i] == jtR && right_join_causing(neighbour_joining_type(i,dirR,context_type,cChars,psa)))
            context_shape[i] = Xr;
        else if (context_type[i] == jtL && left_join_causing(neighbour_joining_type(i,dirL,context_type,cChars,psa)))
            context_shape[i] = Xl;
        else if (context_type[i] == jtD && left_join_causing(neighbour_joining_type(i,dirL,context_type,cChars,psa)) && right_join_causing(neighbour_joining_type(i,dirR,context_type,cChars,psa)))
            context_shape[i] = Xm;
        else if (context_type[i] == jtD && right_join_causing(neighbour_joining_type(i,dirR,context_type,cChars,psa)))
            context_shape[i] = Xr;
        else if (context_type[i] == jtD && left_join_causing(neighbour_joining_type(i,dirL,context_type,cChars,psa)))
            context_shape[i] = Xl;
        else
            context_shape[i] = Xn;
    }

    /* Contextual Shaping */
    i = 0;
    while(i < *pcGlyphs)
    {
        BOOL shaped = FALSE;

        if (psc->GSUB_Table)
        {
            INT nextIndex;
            nextIndex = apply_GSUB_feature_to_glyph(hdc, psa, psc->GSUB_Table, pwOutGlyphs, i, dirL, pcGlyphs, contextual_features[context_shape[i]]);
            if (nextIndex > GSUB_E_NOGLYPH)
                i = nextIndex;
            shaped = (nextIndex > GSUB_E_NOGLYPH);
        }

        if (!shaped)
        {
            WORD newGlyph = pwOutGlyphs[i];
            if (pwcChars[i] >= FIRST_ARABIC_CHAR && pwcChars[i] <= LAST_ARABIC_CHAR)
            {
                /* fall back to presentation form B */
                WCHAR context_char = wine_shaping_forms[pwcChars[i] - FIRST_ARABIC_CHAR][context_shape[i]];
                if (context_char != pwcChars[i] && GetGlyphIndicesW(hdc, &context_char, 1, &newGlyph, 0) != GDI_ERROR && newGlyph != 0x0000)
                    pwOutGlyphs[i] = newGlyph;
            }
            i++;
        }
    }

    i = 0;
    while (arabic_GSUB_features[i] != NULL)
    {
        apply_GSUB_feature(hdc, psa, psc->GSUB_Table, pwOutGlyphs, dirL, pcGlyphs, arabic_GSUB_features[i]);
        i++;
    }

    HeapFree(GetProcessHeap(),0,context_shape);
    HeapFree(GetProcessHeap(),0,context_type);
}
