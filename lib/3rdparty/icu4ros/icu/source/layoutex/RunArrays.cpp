/*
 **********************************************************************
 *   Copyright (C) 2003, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 **********************************************************************
 */

#include "layout/LETypes.h"
#include "layout/LEFontInstance.h"

#include "unicode/locid.h"

#include "layout/RunArrays.h"

U_NAMESPACE_BEGIN

const char RunArray::fgClassID = 0;

RunArray::RunArray(le_int32 initialCapacity)
    : fClientArrays(FALSE), fLimits(NULL), fCount(0), fCapacity(initialCapacity)
{
    if (initialCapacity > 0) {
        fLimits = LE_NEW_ARRAY(le_int32, fCapacity);
    }
}

RunArray::~RunArray()
{
    if (! fClientArrays) {
        LE_DELETE_ARRAY(fLimits);
        fLimits = NULL;
    }
}

le_int32 RunArray::ensureCapacity()
{
    if (fCount >= fCapacity) {
        if (fCapacity == 0) {
            fCapacity = INITIAL_CAPACITY;
            init(fCapacity);
        } else {
            fCapacity += (fCapacity < CAPACITY_GROW_LIMIT ? fCapacity : CAPACITY_GROW_LIMIT);
            grow(fCapacity);
        }
    }

    return fCount++;
}

void RunArray::init(le_int32 capacity)
{
    fLimits = LE_NEW_ARRAY(le_int32, capacity);
}

void RunArray::grow(le_int32 newCapacity)
{
    fLimits = (le_int32 *) LE_GROW_ARRAY(fLimits, newCapacity);
}

le_int32 RunArray::add(le_int32 limit)
{
    if (fClientArrays) {
        return -1;
    }

    le_int32  index  = ensureCapacity();
    le_int32 *limits = (le_int32 *) fLimits;

    limits[index] = limit;

    return index;
}

const char FontRuns::fgClassID = 0;

FontRuns::FontRuns(le_int32 initialCapacity)
    : RunArray(initialCapacity), fFonts(NULL)
{
    if (initialCapacity > 0) {
        fFonts = LE_NEW_ARRAY(const LEFontInstance *, initialCapacity);
    }
}

FontRuns::~FontRuns()
{
    if (! fClientArrays) {
        LE_DELETE_ARRAY(fFonts);
        fFonts = NULL;
    }
}

void FontRuns::init(le_int32 capacity)
{
    RunArray::init(capacity);
    fFonts = LE_NEW_ARRAY(const LEFontInstance *, capacity);
}

void FontRuns::grow(le_int32 capacity)
{
    RunArray::grow(capacity);
    fFonts = (const LEFontInstance **) LE_GROW_ARRAY(fFonts, capacity);
}

le_int32 FontRuns::add(const LEFontInstance *font, le_int32 limit)
{
    le_int32 index = RunArray::add(limit);

    if (index >= 0) {
        LEFontInstance **fonts = (LEFontInstance **) fFonts;

        fonts[index] = (LEFontInstance *) font;
    }

    return index;
}

const LEFontInstance *FontRuns::getFont(le_int32 run) const
{
    if (run < 0 || run >= getCount()) {
        return NULL;
    }

    return fFonts[run];
}

const char LocaleRuns::fgClassID = 0;

LocaleRuns::LocaleRuns(le_int32 initialCapacity)
    : RunArray(initialCapacity), fLocales(NULL)
{
    if (initialCapacity > 0) {
        fLocales = LE_NEW_ARRAY(const Locale *, initialCapacity);
    }
}

LocaleRuns::~LocaleRuns()
{
    if (! fClientArrays) {
        LE_DELETE_ARRAY(fLocales);
        fLocales = NULL;
    }
}

void LocaleRuns::init(le_int32 capacity)
{
    RunArray::init(capacity);
    fLocales = LE_NEW_ARRAY(const Locale *, capacity);
}

void LocaleRuns::grow(le_int32 capacity)
{
    RunArray::grow(capacity);
    fLocales = (const Locale **) LE_GROW_ARRAY(fLocales, capacity);
}

le_int32 LocaleRuns::add(const Locale *locale, le_int32 limit)
{
    le_int32 index = RunArray::add(limit);

    if (index >= 0) {
        Locale **locales = (Locale **) fLocales;

        locales[index] = (Locale *) locale;
    }

    return index;
}

const Locale *LocaleRuns::getLocale(le_int32 run) const
{
    if (run < 0 || run >= getCount()) {
        return NULL;
    }

    return fLocales[run];
}

const char ValueRuns::fgClassID = 0;

ValueRuns::ValueRuns(le_int32 initialCapacity)
    : RunArray(initialCapacity), fValues(NULL)
{
    if (initialCapacity > 0) {
        fValues = LE_NEW_ARRAY(le_int32, initialCapacity);
    }
}

ValueRuns::~ValueRuns()
{
    if (! fClientArrays) {
        LE_DELETE_ARRAY(fValues);
        fValues = NULL;
    }
}

void ValueRuns::init(le_int32 capacity)
{
    RunArray::init(capacity);
    fValues = LE_NEW_ARRAY(le_int32, capacity);
}

void ValueRuns::grow(le_int32 capacity)
{
    RunArray::grow(capacity);
    fValues = (const le_int32 *) LE_GROW_ARRAY(fValues, capacity);
}

le_int32 ValueRuns::add(le_int32 value, le_int32 limit)
{
    le_int32 index = RunArray::add(limit);

    if (index >= 0) {
        le_int32 *values = (le_int32 *) fValues;

        values[index] = value;
    }

    return index;
}

le_int32 ValueRuns::getValue(le_int32 run) const
{
    if (run < 0 || run >= getCount()) {
        return -1;
    }

    return fValues[run];
}

U_NAMESPACE_END
