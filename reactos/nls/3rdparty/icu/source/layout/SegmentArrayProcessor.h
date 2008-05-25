/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#ifndef __SEGMENTARRAYPROCESSOR_H
#define __SEGMENTARRAYPROCESSOR_H

/**
 * \file
 * \internal
 */

#include "LETypes.h"
#include "MorphTables.h"
#include "SubtableProcessor.h"
#include "NonContextualGlyphSubst.h"
#include "NonContextualGlyphSubstProc.h"

U_NAMESPACE_BEGIN

class LEGlyphStorage;

class SegmentArrayProcessor : public NonContextualGlyphSubstitutionProcessor
{
public:
    virtual void process(LEGlyphStorage &glyphStorage);

    SegmentArrayProcessor(const MorphSubtableHeader *morphSubtableHeader);

    virtual ~SegmentArrayProcessor();

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
    SegmentArrayProcessor();

protected:
    const SegmentArrayLookupTable *segmentArrayLookupTable;

};

U_NAMESPACE_END
#endif

