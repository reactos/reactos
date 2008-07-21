/*
 * (C) Copyright IBM Corp. 1998-2007 - All Rights Reserved
 *
 */

#ifndef __DEFAULTCHARMAPPER_H
#define __DEFAULTCHARMAPPER_H

/**
 * \file
 * \internal
 */

#include "LETypes.h"
#include "LEFontInstance.h"

U_NAMESPACE_BEGIN

/**
 * This class is an instance of LECharMapper which
 * implements control character filtering and bidi
 * mirroring.
 *
 * @see LECharMapper
 */
class DefaultCharMapper : public UMemory, public LECharMapper
{
private:
    le_bool fFilterControls;
    le_bool fMirror;

    static const LEUnicode32 controlChars[];

    static const le_int32 controlCharsCount;

    static const LEUnicode32 mirroredChars[];
    static const LEUnicode32 srahCderorrim[];

    static const le_int32 mirroredCharsCount;

public:
    DefaultCharMapper(le_bool filterControls, le_bool mirror)
        : fFilterControls(filterControls), fMirror(mirror)
    {
        // nothing
    };

    ~DefaultCharMapper()
    {
        // nada
    };

    LEUnicode32 mapChar(LEUnicode32 ch) const;
};

U_NAMESPACE_END
#endif
