/*
 *
 * (C) Copyright IBM Corp. 1998-2005 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "LEGlyphStorage.h"
#include "CanonShaping.h"
#include "GlyphDefinitionTables.h"
#include "ClassDefinitionTables.h"

U_NAMESPACE_BEGIN

void CanonShaping::sortMarks(le_int32 *indices, const le_int32 *combiningClasses, le_int32 index, le_int32 limit)
{
    for (le_int32 j = index + 1; j < limit; j += 1) {
        le_int32 i;
        le_int32 v = indices[j];
        le_int32 c = combiningClasses[v];

        for (i = j - 1; i >= index; i -= 1) {
            if (c >= combiningClasses[indices[i]]) {
                break;
            }

            indices[i + 1] = indices[i];
        }

        indices[i + 1] = v;
    }
}

void CanonShaping::reorderMarks(const LEUnicode *inChars, le_int32 charCount, le_bool rightToLeft,
                                LEUnicode *outChars, LEGlyphStorage &glyphStorage)
{
    const GlyphDefinitionTableHeader *gdefTable = (const GlyphDefinitionTableHeader *) glyphDefinitionTable;
    const ClassDefinitionTable *classTable = gdefTable->getMarkAttachClassDefinitionTable();
    le_int32 *combiningClasses = LE_NEW_ARRAY(le_int32, charCount);
    le_int32 *indices = LE_NEW_ARRAY(le_int32, charCount);
    LEErrorCode status = LE_NO_ERROR;
    le_int32 i;

    for (i = 0; i < charCount; i += 1) {
        combiningClasses[i] = classTable->getGlyphClass((LEGlyphID) inChars[i]);
        indices[i] = i;
    }

    for (i = 0; i < charCount; i += 1) {
        if (combiningClasses[i] != 0) {
            le_int32 mark;

            for (mark = i; mark < charCount; mark += 1) {
                if (combiningClasses[mark] == 0) {
                    break;
                }
            }

            sortMarks(indices, combiningClasses, i, mark);
        }
    }

    le_int32 out = 0, dir = 1;

    if (rightToLeft) {
        out = charCount - 1;
        dir = -1;
    }

    for (i = 0; i < charCount; i += 1, out += dir) {
        le_int32 index = indices[i];

        outChars[i] = inChars[index];
        glyphStorage.setCharIndex(out, index, status);
    }

    LE_DELETE_ARRAY(indices);
    LE_DELETE_ARRAY(combiningClasses);
}

U_NAMESPACE_END
