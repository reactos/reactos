/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#ifndef __GLYPHDEFINITIONTABLES_H
#define __GLYPHDEFINITIONTABLES_H

/**
 * \file
 * \internal
 */

#include "LETypes.h"
#include "OpenTypeTables.h"
#include "ClassDefinitionTables.h"

U_NAMESPACE_BEGIN

typedef ClassDefinitionTable GlyphClassDefinitionTable;

enum GlyphClassDefinitions
{
    gcdNoGlyphClass     = 0,
    gcdSimpleGlyph      = 1,
    gcdLigatureGlyph    = 2,
    gcdMarkGlyph        = 3,
    gcdComponentGlyph   = 4
};

struct AttachmentListTable
{
    Offset  coverageTableOffset;
    le_uint16  glyphCount;
    Offset  attachPointTableOffsetArray[ANY_NUMBER];
};

struct AttachPointTable
{
    le_uint16  pointCount;
    le_uint16  pointIndexArray[ANY_NUMBER];
};

struct LigatureCaretListTable
{
    Offset  coverageTableOffset;
    le_uint16  ligGlyphCount;
    Offset  ligGlyphTableOffsetArray[ANY_NUMBER];
};

struct LigatureGlyphTable
{
    le_uint16  caretCount;
    Offset  caretValueTableOffsetArray[ANY_NUMBER];
};

struct CaretValueTable
{
    le_uint16  caretValueFormat;
};

struct CaretValueFormat1Table : CaretValueTable
{
    le_int16   coordinate;
};

struct CaretValueFormat2Table : CaretValueTable
{
    le_uint16  caretValuePoint;
};

struct CaretValueFormat3Table : CaretValueTable
{
    le_int16   coordinate;
    Offset  deviceTableOffset;
};

typedef ClassDefinitionTable MarkAttachClassDefinitionTable;

struct GlyphDefinitionTableHeader
{
    fixed32 version;
    Offset  glyphClassDefOffset;
    Offset  attachListOffset;
    Offset  ligCaretListOffset;
    Offset  MarkAttachClassDefOffset;

    const GlyphClassDefinitionTable *getGlyphClassDefinitionTable() const;
    const AttachmentListTable *getAttachmentListTable()const ;
    const LigatureCaretListTable *getLigatureCaretListTable() const;
    const MarkAttachClassDefinitionTable *getMarkAttachClassDefinitionTable() const;
};

U_NAMESPACE_END
#endif
