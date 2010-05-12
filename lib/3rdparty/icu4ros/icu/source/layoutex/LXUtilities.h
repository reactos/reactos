/*
 **********************************************************************
 *   Copyright (C) 2003, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 **********************************************************************
 */

#ifndef __LXUTILITIES_H

#define __LXUTILITIES_H

#include "layout/LETypes.h"

U_NAMESPACE_BEGIN

class LXUtilities
{
public:
    static le_int8 highBit(le_int32 value);
    static le_int32 search(le_int32 value, const le_int32 array[], le_int32 count);
    static void reverse(le_int32 array[], le_int32 count);
    static void reverse(float array[], le_int32 count);
};

U_NAMESPACE_END
#endif
