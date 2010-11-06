/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#ifndef __STATETABLES_H
#define __STATETABLES_H

/**
 * \file
 * \internal
 */

#include "LETypes.h"
#include "LayoutTables.h"

U_NAMESPACE_BEGIN

struct StateTableHeader
{
    le_int16 stateSize;
    ByteOffset classTableOffset;
    ByteOffset stateArrayOffset;
    ByteOffset entryTableOffset;
};

enum ClassCodes
{
    classCodeEOT = 0,
    classCodeOOB = 1,
    classCodeDEL = 2,
    classCodeEOL = 3,
    classCodeFirstFree = 4,
    classCodeMAX = 0xFF
};

typedef le_uint8 ClassCode;

struct ClassTable
{
    TTGlyphID firstGlyph;
    le_uint16 nGlyphs;
    ClassCode classArray[ANY_NUMBER];
};

enum StateNumber
{
    stateSOT        = 0,
    stateSOL        = 1,
    stateFirstFree  = 2,
    stateMAX        = 0xFF
};

typedef le_uint8 EntryTableIndex;

struct StateEntry
{
    ByteOffset  newStateOffset;
    le_int16    flags;
};

U_NAMESPACE_END
#endif

