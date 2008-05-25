/*
 * @(#)DeviceTables.cpp 1.5 00/03/15
 *
 * (C) Copyright IBM Corp. 1998 - 2006 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "OpenTypeTables.h"
#include "DeviceTables.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

const le_uint16 DeviceTable::fieldMasks[]    = {0x0003, 0x000F, 0x00FF};
const le_uint16 DeviceTable::fieldSignBits[] = {0x0002, 0x0008, 0x0080};
const le_uint16 DeviceTable::fieldBits[]     = {     2,      4,      8};

#define FORMAT_COUNT LE_ARRAY_SIZE(fieldBits)

le_int16 DeviceTable::getAdjustment(le_uint16 ppem) const
{
    le_uint16 start = SWAPW(startSize);
    le_uint16 format = SWAPW(deltaFormat) - 1;
    le_int16 result = 0;
    
    if (ppem >= start && ppem <= SWAPW(endSize) && format < FORMAT_COUNT) {
        le_uint16 sizeIndex = ppem - start;
        le_uint16 bits = fieldBits[format];
        le_uint16 count = 16 / bits;
        le_uint16 word = SWAPW(deltaValues[sizeIndex / count]);
        le_uint16 fieldIndex = sizeIndex % count;
        le_uint16 shift = 16 - (bits * (fieldIndex + 1));
        le_uint16 field = (word >> shift) & fieldMasks[format];

        result = field;

        if ((field & fieldSignBits[format]) != 0) {
            result |= ~ fieldMasks[format];
        }
    }

    return result;
}

U_NAMESPACE_END
