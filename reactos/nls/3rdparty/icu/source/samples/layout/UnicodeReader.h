/*
 ******************************************************************************
 * Copyright (C) 1998-2001, International Business Machines Corporation and   *
 * others. All Rights Reserved.                                               *
 ******************************************************************************
 */

#ifndef __UNICODEREADER_H
#define __UNICODEREADER_H

#include "unicode/utypes.h"

#include "GUISupport.h"

class UnicodeReader
{
public:
    UnicodeReader()
    {
        // nothing...
    }

    ~UnicodeReader()
    {
        // nothing, too
    }

    static const UChar *readFile(const char *fileName, GUISupport *guiSupport, int32_t &charCount);
};

#endif

