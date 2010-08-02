
/*
 * (C) Copyright IBM Corp. 1998-2005 - All Rights Reserved 
 *
 * This file is a modification of the ICU file IndicLayoutEngine.cpp
 * by Jens Herden and Javier Sola for Khmer language 
 *
 */


#include "OpenTypeLayoutEngine.h"
#include "KhmerLayoutEngine.h"
#include "LEGlyphStorage.h"
#include "KhmerReordering.h"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(KhmerOpenTypeLayoutEngine)

KhmerOpenTypeLayoutEngine::KhmerOpenTypeLayoutEngine(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode,
                    le_int32 typoFlags, const GlyphSubstitutionTableHeader *gsubTable)
    : OpenTypeLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags, gsubTable)
{
    fFeatureMap   = KhmerReordering::getFeatureMap(fFeatureMapCount);
    fFeatureOrder = TRUE;
}

KhmerOpenTypeLayoutEngine::KhmerOpenTypeLayoutEngine(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode,
						     le_int32 typoFlags)
    : OpenTypeLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags)
{
    fFeatureMap   = KhmerReordering::getFeatureMap(fFeatureMapCount);
    fFeatureOrder = TRUE;
}

KhmerOpenTypeLayoutEngine::~KhmerOpenTypeLayoutEngine()
{
    // nothing to do
}

// Input: characters
// Output: characters, char indices, tags
// Returns: output character count
le_int32 KhmerOpenTypeLayoutEngine::characterProcessing(const LEUnicode chars[], le_int32 offset, le_int32 count, le_int32 max, le_bool rightToLeft,
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
    le_int32 outCharCount = KhmerReordering::reorder(&chars[offset], count, fScriptCode, outChars, glyphStorage);

    glyphStorage.adoptGlyphCount(outCharCount);
    return outCharCount;
}

U_NAMESPACE_END
