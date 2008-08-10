/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#ifndef __SUBTABLEPROCESSOR_H
#define __SUBTABLEPROCESSOR_H

/**
 * \file
 * \internal
 */

#include "LETypes.h"
#include "MorphTables.h"

U_NAMESPACE_BEGIN

class LEGlyphStorage;

class SubtableProcessor : public UMemory {
public:
    virtual void process(LEGlyphStorage &glyphStorage) = 0;
    virtual ~SubtableProcessor();

protected:
    SubtableProcessor(const MorphSubtableHeader *morphSubtableHeader);

    SubtableProcessor();

    le_int16 length;
    SubtableCoverage coverage;
    FeatureFlags subtableFeatures;

    const MorphSubtableHeader *subtableHeader;

private:

    SubtableProcessor(const SubtableProcessor &other); // forbid copying of this class
    SubtableProcessor &operator=(const SubtableProcessor &other); // forbid copying of this class
};

U_NAMESPACE_END
#endif

