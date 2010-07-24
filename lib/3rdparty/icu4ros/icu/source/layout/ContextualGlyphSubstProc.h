/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#ifndef __CONTEXTUALGLYPHSUBSTITUTIONPROCESSOR_H
#define __CONTEXTUALGLYPHSUBSTITUTIONPROCESSOR_H

/**
 * \file
 * \internal
 */

#include "LETypes.h"
#include "MorphTables.h"
#include "SubtableProcessor.h"
#include "StateTableProcessor.h"
#include "ContextualGlyphSubstitution.h"

U_NAMESPACE_BEGIN

class LEGlyphStorage;

class ContextualGlyphSubstitutionProcessor : public StateTableProcessor
{
public:
    virtual void beginStateTable();

    virtual ByteOffset processStateEntry(LEGlyphStorage &glyphStorage, le_int32 &currGlyph, EntryTableIndex index);

    virtual void endStateTable();

    ContextualGlyphSubstitutionProcessor(const MorphSubtableHeader *morphSubtableHeader);
    virtual ~ContextualGlyphSubstitutionProcessor();

    /**
     * ICU "poor man's RTTI", returns a UClassID for the actual class.
     *
     * @stable ICU 2.8
     */
    virtual UClassID getDynamicClassID() const;

    /**
     * ICU "poor man's RTTI", returns a UClassID for this class.
     *
     * @stable ICU 2.8
     */
    static UClassID getStaticClassID();

private:
    ContextualGlyphSubstitutionProcessor();

protected:
    ByteOffset substitutionTableOffset;
    const ContextualGlyphSubstitutionStateEntry *entryTable;

    le_int32 markGlyph;

    const ContextualGlyphSubstitutionHeader *contextualGlyphSubstitutionHeader;

};

U_NAMESPACE_END
#endif
