/*
 **********************************************************************
 *   Copyright (C) 2002-2003, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 **********************************************************************
 */

#include "layout/LETypes.h"
#include "LXUtilities.h"

U_NAMESPACE_BEGIN

//
// Finds the high bit by binary searching
// through the bits in n.
//
le_int8 LXUtilities::highBit(le_int32 value)
{
    if (value <= 0) {
        return -32;
    }

    le_int8 bit = 0;

    if (value >= 1 << 16) {
        value >>= 16;
        bit += 16;
    }

    if (value >= 1 << 8) {
        value >>= 8;
        bit += 8;
    }

    if (value >= 1 << 4) {
        value >>= 4;
        bit += 4;
    }

    if (value >= 1 << 2) {
        value >>= 2;
        bit += 2;
    }

    if (value >= 1 << 1) {
        value >>= 1;
        bit += 1;
    }

    return bit;
}

le_int32 LXUtilities::search(le_int32 value, const le_int32 array[], le_int32 count)
{
    le_int32 power = 1 << highBit(count);
    le_int32 extra = count - power;
    le_int32 probe = power;
    le_int32 index = 0;

    if (value >= array[extra]) {
        index = extra;
    }

    while (probe > (1 << 0)) {
        probe >>= 1;

        if (value >= array[index + probe]) {
            index += probe;
        }
    }

    return index;
}

void LXUtilities::reverse(le_int32 array[], le_int32 length)
{
    le_int32 front, back;

    for (front = 0, back = length - 1; front < back; front += 1, back -= 1) {
        le_int32 swap = array[front];

        array[front] = array[back];
        array[back]  = swap;
    }
}

void LXUtilities::reverse(float array[], le_int32 length)
{
    le_int32 front, back;

    for (front = 0, back = length - 1; front < back; front += 1, back -= 1) {
        float swap = array[front];

        array[front] = array[back];
        array[back]  = swap;
    }
}

U_NAMESPACE_END
