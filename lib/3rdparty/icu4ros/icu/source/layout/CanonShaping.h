/*
 *
 * (C) Copyright IBM Corp. 1998-2006 - All Rights Reserved
 *
 */

#ifndef __CANONSHAPING_H
#define __CANONSHAPING_H

#include "LETypes.h"

U_NAMESPACE_BEGIN

class LEGlyphStorage;

class U_LAYOUT_API CanonShaping /* not : public UObject because all members are static */
{
public:
    static const le_uint8 glyphSubstitutionTable[];
    static const le_uint8 glyphDefinitionTable[];

    static void reorderMarks(const LEUnicode *inChars, le_int32 charCount, le_bool rightToLeft,
                                   LEUnicode *outChars, LEGlyphStorage &glyphStorage);

private:
    static void sortMarks(le_int32 *indices, const le_int32 *combiningClasses, le_int32 index, le_int32 limit);
};

U_NAMESPACE_END
#endif
