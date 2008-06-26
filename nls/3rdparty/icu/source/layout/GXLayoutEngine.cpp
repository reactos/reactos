
/*
 *
 * (C) Copyright IBM Corp. 1998-2007 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "LayoutEngine.h"
#include "GXLayoutEngine.h"
#include "LEGlyphStorage.h"

#include "MorphTables.h"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(GXLayoutEngine)

GXLayoutEngine::GXLayoutEngine(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode, const MorphTableHeader *morphTable) 
    : LayoutEngine(fontInstance, scriptCode, languageCode, 0), fMorphTable(morphTable)
{
    // nothing else to do?
}

GXLayoutEngine::~GXLayoutEngine()
{
    reset();
}

// apply 'mort' table
le_int32 GXLayoutEngine::computeGlyphs(const LEUnicode chars[], le_int32 offset, le_int32 count, le_int32 max, le_bool rightToLeft, LEGlyphStorage &glyphStorage, LEErrorCode &success)
{
    if (LE_FAILURE(success)) {
        return 0;
    }

    if (chars == NULL || offset < 0 || count < 0 || max < 0 || offset >= max || offset + count > max) {
        success = LE_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    mapCharsToGlyphs(chars, offset, count, FALSE, rightToLeft, glyphStorage, success);

    if (LE_FAILURE(success)) {
        return 0;
    }

    fMorphTable->process(glyphStorage);

    return count;
}

// apply positional tables
void GXLayoutEngine::adjustGlyphPositions(const LEUnicode chars[], le_int32 offset, le_int32 count, le_bool /*reverse*/,
                                          LEGlyphStorage &/*glyphStorage*/, LEErrorCode &success)
{
    if (LE_FAILURE(success)) {
        return;
    }

    if (chars == NULL || offset < 0 || count < 0) {
        success = LE_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    // FIXME: no positional processing yet...
}

U_NAMESPACE_END
