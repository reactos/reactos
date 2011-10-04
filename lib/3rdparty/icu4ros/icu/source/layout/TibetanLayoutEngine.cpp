/*
 *
 * (C) Copyright IBM Corp. 1998-2005 - All Rights Reserved 
 *
 * Developed at DIT - Government of Bhutan
 *
 * Contact person: Pema Geyleg - <pema_geyleg@druknet.bt> 
 *
 * This file is a modification of the ICU file KhmerReordering.cpp
 * by Jens Herden and Javier Sola who have given all their possible rights to IBM and the Governement of Bhutan
 * A first module for Dzongkha was developed by Karunakar under Panlocalisation funding.
 * Assistance for this module has been received from Namgay Thinley, Christopher Fynn and Javier Sola
 *
 */


#include "OpenTypeLayoutEngine.h"
#include "TibetanLayoutEngine.h"
#include "LEGlyphStorage.h"
#include "TibetanReordering.h"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(TibetanOpenTypeLayoutEngine)

TibetanOpenTypeLayoutEngine::TibetanOpenTypeLayoutEngine(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode,
                    le_int32 typoFlags, const GlyphSubstitutionTableHeader *gsubTable)
    : OpenTypeLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags, gsubTable)
{
    fFeatureMap   = TibetanReordering::getFeatureMap(fFeatureMapCount);
    fFeatureOrder = TRUE;
}

TibetanOpenTypeLayoutEngine::TibetanOpenTypeLayoutEngine(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode,
						     le_int32 typoFlags)
    : OpenTypeLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags)
{
    fFeatureMap   = TibetanReordering::getFeatureMap(fFeatureMapCount);
    fFeatureOrder = TRUE;
}

TibetanOpenTypeLayoutEngine::~TibetanOpenTypeLayoutEngine()
{
    // nothing to do
}

// Input: characters
// Output: characters, char indices, tags
// Returns: output character count
le_int32 TibetanOpenTypeLayoutEngine::characterProcessing(const LEUnicode chars[], le_int32 offset, le_int32 count, le_int32 max, le_bool rightToLeft,
        LEUnicode *&outChars, LEGlyphStorage &glyphStorage, LEErrorCode &success)
{
    if (LE_FAILURE(success)) {
        return 0;
    }

    if (chars == NULL || offset < 0 || count < 0 || max < 0 || offset >= max || offset + count > max) {
        success = LE_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    le_int32 worstCase = count * 3;  // worst case is 3 for Khmer  TODO check if 2 is enough

    outChars = LE_NEW_ARRAY(LEUnicode, worstCase);

    if (outChars == NULL) {
        success = LE_MEMORY_ALLOCATION_ERROR;
        return 0;
    }

    glyphStorage.allocateGlyphArray(worstCase, rightToLeft, success);
    glyphStorage.allocateAuxData(success);

    if (LE_FAILURE(success)) {
        LE_DELETE_ARRAY(outChars);
        return 0;
    }

    // NOTE: assumes this allocates featureTags...
    // (probably better than doing the worst case stuff here...)
    le_int32 outCharCount = TibetanReordering::reorder(&chars[offset], count, fScriptCode, outChars, glyphStorage);

    glyphStorage.adoptGlyphCount(outCharCount);
    return outCharCount;
}

U_NAMESPACE_END
