/*
 *
 * (C) Copyright IBM Corp. 1998 - 2004 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "OpenTypeTables.h"
#include "GlyphDefinitionTables.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

const GlyphClassDefinitionTable *GlyphDefinitionTableHeader::getGlyphClassDefinitionTable() const
{
    return (const GlyphClassDefinitionTable *) ((char *) this + SWAPW(glyphClassDefOffset));
}

const AttachmentListTable *GlyphDefinitionTableHeader::getAttachmentListTable() const
{
    return (const AttachmentListTable *) ((char *) this + SWAPW(attachListOffset));
}

const LigatureCaretListTable *GlyphDefinitionTableHeader::getLigatureCaretListTable() const
{
    return (const LigatureCaretListTable *) ((char *) this + SWAPW(ligCaretListOffset));
}

const MarkAttachClassDefinitionTable *GlyphDefinitionTableHeader::getMarkAttachClassDefinitionTable() const
{
    return (const MarkAttachClassDefinitionTable *) ((char *) this + SWAPW(MarkAttachClassDefOffset));
}

U_NAMESPACE_END
