/*
 *
 * (C) Copyright IBM Corp. 1998-2006 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "OpenTypeTables.h"
#include "OpenTypeUtilities.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

//
// Finds the high bit by binary searching
// through the bits in n.
//
le_int8 OpenTypeUtilities::highBit(le_int32 value)
{
    if (value <= 0) {
        return -32;
    }

    le_uint8 bit = 0;

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

Offset OpenTypeUtilities::getTagOffset(LETag tag, const TagAndOffsetRecord *records, le_int32 recordCount)
{
    le_uint8 bit = highBit(recordCount);
    le_int32 power = 1 << bit;
    le_int32 extra = recordCount - power;
    le_int32 probe = power;
    le_int32 index = 0;

    if (SWAPT(records[extra].tag) <= tag) {
        index = extra;
    }

    while (probe > (1 << 0)) {
        probe >>= 1;

        if (SWAPT(records[index + probe].tag) <= tag) {
            index += probe;
        }
    }

    if (SWAPT(records[index].tag) == tag) {
        return SWAPW(records[index].offset);
    }

    return 0;
}

le_int32 OpenTypeUtilities::getGlyphRangeIndex(TTGlyphID glyphID, const GlyphRangeRecord *records, le_int32 recordCount)
{
    le_uint8 bit = highBit(recordCount);
    le_int32 power = 1 << bit;
    le_int32 extra = recordCount - power;
    le_int32 probe = power;
    le_int32 range = 0;

	if (recordCount == 0) {
		return -1;
	}

    if (SWAPW(records[extra].firstGlyph) <= glyphID) {
        range = extra;
    }

    while (probe > (1 << 0)) {
        probe >>= 1;

        if (SWAPW(records[range + probe].firstGlyph) <= glyphID) {
            range += probe;
        }
    }

    if (SWAPW(records[range].firstGlyph) <= glyphID && SWAPW(records[range].lastGlyph) >= glyphID) {
        return range;
    }

    return -1;
}

le_int32 OpenTypeUtilities::search(le_uint32 value, const le_uint32 array[], le_int32 count)
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

le_int32 OpenTypeUtilities::search(le_uint16 value, const le_uint16 array[], le_int32 count)
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

//
// Straight insertion sort from Knuth vol. III, pg. 81
//
void OpenTypeUtilities::sort(le_uint16 *array, le_int32 count)
{
    for (le_int32 j = 1; j < count; j += 1) {
        le_int32 i;
        le_uint16 v = array[j];

        for (i = j - 1; i >= 0; i -= 1) {
            if (v >= array[i]) {
                break;
            }

            array[i + 1] = array[i];
        }

        array[i + 1] = v;
    }
}

 

U_NAMESPACE_END
