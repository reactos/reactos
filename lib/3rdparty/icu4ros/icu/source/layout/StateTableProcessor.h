/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#ifndef __STATETABLEPROCESSOR_H
#define __STATETABLEPROCESSOR_H

/**
 * \file
 * \internal
 */

#include "LETypes.h"
#include "MorphTables.h"
#include "MorphStateTables.h"
#include "SubtableProcessor.h"

U_NAMESPACE_BEGIN

class LEGlyphStorage;

class StateTableProcessor : public SubtableProcessor
{
public:
    void process(LEGlyphStorage &glyphStorage);

    virtual void beginStateTable() = 0;

    virtual ByteOffset processStateEntry(LEGlyphStorage &glyphStorage, le_int32 &currGlyph, EntryTableIndex index) = 0;

    virtual void endStateTable() = 0;

protected:
    StateTableProcessor(const MorphSubtableHeader *morphSubtableHeader);
    virtual ~StateTableProcessor();

    StateTableProcessor();

    le_int16 stateSize;
    ByteOffset classTableOffset;
    ByteOffset stateArrayOffset;
    ByteOffset entryTableOffset;

    const ClassTable *classTable;
    TTGlyphID firstGlyph;
    TTGlyphID lastGlyph;

    const MorphStateTableHeader *stateTableHeader;

private:
    StateTableProcessor(const StateTableProcessor &other); // forbid copying of this class
    StateTableProcessor &operator=(const StateTableProcessor &other); // forbid copying of this class
};

U_NAMESPACE_END
#endif
