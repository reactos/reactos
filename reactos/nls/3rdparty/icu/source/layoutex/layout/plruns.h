/*
 *
 * (C) Copyright IBM Corp. 1998-2007 - All Rights Reserved
 *
 */

#ifndef __PLRUNS_H
#define __PLRUNS_H

#include "unicode/utypes.h"
#include "unicode/ubidi.h"
#include "layout/LETypes.h"

#include "layout/loengine.h"

typedef void pl_fontRuns;
typedef void pl_valueRuns;
typedef void pl_localeRuns;

/**
 * \file 
 * \brief C API for run arrays.
 *
 * This is a technology preview. The API may
 * change significantly.
 *
 */

/**
 * Construct a <code>pl_fontRuns</code> object from pre-existing arrays of fonts
 * and limit indices.
 *
 * @param fonts is the address of an array of pointers to <code>le_font</code> objects. This
 *              array, and the <code>le_font</code> objects to which it points must remain
 *              valid until the <code>pl_fontRuns</code> object is closed.
 *
 * @param limits is the address of an array of limit indices. This array must remain valid until
 *               the <code>pl_fontRuns</code> object is closed.
 *
 * @param count is the number of entries in the two arrays.
 *
 * @internal
 */
U_INTERNAL pl_fontRuns * U_EXPORT2
pl_openFontRuns(const le_font **fonts,
                const le_int32 *limits,
                le_int32 count);

/**
 * Construct an empty <code>pl_fontRuns</code> object. Clients can add font and limit
 * indices arrays using the <code>pl_addFontRun</code> routine.
 *
 * @param initialCapacity is the initial size of the font and limit indices arrays. If
 *                        this value is zero, no arrays will be allocated.
 *
 * @see pl_addFontRun
 *
 * @internal
 */
U_INTERNAL pl_fontRuns * U_EXPORT2
pl_openEmptyFontRuns(le_int32 initialCapacity);

/**
 * Close the given <code>pl_fontRuns</code> object. Once this
 * call returns, the object can no longer be referenced.
 *
 * @param fontRuns is the <code>pl_fontRuns</code> object.
 *
 * @internal
 */
U_INTERNAL void U_EXPORT2
pl_closeFontRuns(pl_fontRuns *fontRuns);

/**
 * Get the number of font runs.
 *
 * @param fontRuns is the <code>pl_fontRuns</code> object.
 *
 * @return the number of entries in the limit indices array.
 *
 * @internal
 */
U_INTERNAL le_int32 U_EXPORT2
pl_getFontRunCount(const pl_fontRuns *fontRuns);

/**
 * Reset the number of font runs to zero.
 *
 * @param fontRuns is the <code>pl_fontRuns</code> object.
 *
 * @internal
 */
U_INTERNAL void U_EXPORT2
pl_resetFontRuns(pl_fontRuns *fontRuns);

/**
 * Get the limit index for the last font run. This is the
 * number of characters in the text.
 *
 * @param fontRuns is the <code>pl_fontRuns</code> object.
 *
 * @return the last limit index.
 *
 * @internal
 */
U_INTERNAL le_int32 U_EXPORT2
pl_getFontRunLastLimit(const pl_fontRuns *fontRuns);

/**
 * Get the limit index for a particular font run.
 *
 * @param fontRuns is the <code>pl_fontRuns</code> object.
 * @param run is the run. This is an index into the limit index array.
 *
 * @return the limit index for the run, or -1 if <code>run</code> is out of bounds.
 *
 * @internal
 */
U_INTERNAL le_int32 U_EXPORT2
pl_getFontRunLimit(const pl_fontRuns *fontRuns,
                   le_int32 run);

/**
 * Get the <code>le_font</code> object assoicated with the given run
 * of text. Use <code>pl_getFontRunLimit(run)</code> to get the corresponding
 * limit index.
 *
 * @param fontRuns is the <code>pl_fontRuns</code> object.
 * @param run is the index into the font and limit indices arrays.
 *
 * @return the <code>le_font</code> associated with the given text run.
 *
 * @internal
 */
U_INTERNAL const le_font * U_EXPORT2
pl_getFontRunFont(const pl_fontRuns *fontRuns,
                  le_int32 run);


/**
 * Add a new font run to the given <code>pl_fontRuns</code> object.
 *
 * If the <code>pl_fontRuns</code> object was not created by calling
 * <code>pl_openEmptyFontRuns</code>, this method will return a run index of -1.
 *
 * @param fontRuns is the <code>pl_fontRuns</code> object.
 *
 * @param font is the address of the <code>le_font</code> to add. This object must
 *             remain valid until the <code>pl_fontRuns</code> object is closed.
 *
 * @param limit is the limit index to add
 *
 * @return the run index where the font and limit index were stored, or -1 if 
 *         the run cannot be added.
 *
 * @internal
 */
U_INTERNAL le_int32 U_EXPORT2
pl_addFontRun(pl_fontRuns *fontRuns,
              const le_font *font,
              le_int32 limit);

/**
 * Construct a <code>pl_valueRuns</code> object from pre-existing arrays of values
 * and limit indices.
 *
 * @param values is the address of an array of values. This array must remain valid until
                 the <code>pl_valueRuns</code> object is closed.
 *
 * @param limits is the address of an array of limit indices. This array must remain valid until
 *               the <code>pl_valueRuns</code> object is closed.
 *
 * @param count is the number of entries in the two arrays.
 *
 * @internal
 */
U_INTERNAL pl_valueRuns * U_EXPORT2
pl_openValueRuns(const le_int32 *values,
                 const le_int32 *limits,
                 le_int32 count);

/**
 * Construct an empty <code>pl_valueRuns</code> object. Clients can add values and limits
 * using the <code>pl_addValueRun</code> routine.
 *
 * @param initialCapacity is the initial size of the value and limit indices arrays. If
 *                        this value is zero, no arrays will be allocated.
 *
 * @see pl_addValueRun
 *
 * @internal
 */
U_INTERNAL pl_valueRuns * U_EXPORT2
pl_openEmptyValueRuns(le_int32 initialCapacity);

/**
 * Close the given <code>pl_valueRuns</code> object. Once this
 * call returns, the object can no longer be referenced.
 *
 * @param valueRuns is the <code>pl_valueRuns</code> object.
 *
 * @internal
 */
U_INTERNAL void U_EXPORT2
pl_closeValueRuns(pl_valueRuns *valueRuns);

/**
 * Get the number of value runs.
 *
 * @param valueRuns is the <code>pl_valueRuns</code> object.
 *
 * @return the number of value runs.
 *
 * @internal
 */
U_INTERNAL le_int32 U_EXPORT2
pl_getValueRunCount(const pl_valueRuns *valueRuns);

/**
 * Reset the number of value runs to zero.
 *
 * @param valueRuns is the <code>pl_valueRuns</code> object.
 *
 * @internal
 */
U_INTERNAL void U_EXPORT2
pl_resetValueRuns(pl_valueRuns *valueRuns);

/**
 * Get the limit index for the last value run. This is the
 * number of characters in the text.
 *
 * @param valueRuns is the <code>pl_valueRuns</code> object.
 *
 * @return the last limit index.
 *
 * @internal
 */
U_INTERNAL le_int32 U_EXPORT2
pl_getValueRunLastLimit(const pl_valueRuns *valueRuns);

/**
 * Get the limit index for a particular value run.
 *
 * @param valueRuns is the <code>pl_valueRuns</code> object.
 * @param run is the run index.
 *
 * @return the limit index for the run, or -1 if <code>run</code> is out of bounds.
 *
 * @internal
 */
U_INTERNAL le_int32 U_EXPORT2
pl_getValueRunLimit(const pl_valueRuns *valueRuns,
                     le_int32 run);

/**
 * Get the value assoicated with the given run * of text. Use
 * <code>pl_getValueRunLimit(run)</code> to get the corresponding
 * limit index.
 *
 * @param valueRuns is the <code>pl_valueRuns</code> object.
 * @param run is the run index.
 *
 * @return the value associated with the given text run.
 *
 * @internal
 */
U_INTERNAL le_int32 U_EXPORT2
pl_getValueRunValue(const pl_valueRuns *valueRuns,
                    le_int32 run);


/**
 * Add a new font run to the given <code>pl_valueRuns</code> object.
 *
 * If the <code>pl_valueRuns</code> object was not created by calling
 * <code>pl_openEmptyFontRuns</code>, this method will return a run index of -1.
 *
 * @param valueRuns is the <code>pl_valueRuns</code> object.
 *
 * @param value is the value to add.
 *
 * @param limit is the limit index to add
 *
 * @return the run index where the font and limit index were stored, or -1 if 
 *         the run cannot be added.
 *
 * @internal
 */
U_INTERNAL le_int32 U_EXPORT2
pl_addValueRun(pl_valueRuns *valueRuns,
               le_int32 value,
               le_int32 limit);

/**
 * Construct a <code>pl_localeRuns</code> object from pre-existing arrays of fonts
 * and limit indices.
 *
 * @param locales is the address of an array of pointers to locale name strings. This
 *                array must remain valid until the <code>pl_localeRuns</code> object is destroyed.
 *
 * @param limits is the address of an array of limit indices. This array must remain valid until
 *               the <code>pl_valueRuns</code> object is destroyed.
 *
 * @param count is the number of entries in the two arrays.
 *
 * @internal
 */
U_INTERNAL pl_localeRuns * U_EXPORT2
pl_openLocaleRuns(const char **locales,
                  const le_int32 *limits,
                  le_int32 count);

/**
 * Construct an empty <code>pl_localeRuns</code> object. Clients can add font and limit
 * indices arrays using the <code>pl_addFontRun</code> routine.
 *
 * @param initialCapacity is the initial size of the font and limit indices arrays. If
 *                        this value is zero, no arrays will be allocated.
 *
 * @see pl_addLocaleRun
 *
 * @internal
 */
U_INTERNAL pl_localeRuns * U_EXPORT2
pl_openEmptyLocaleRuns(le_int32 initialCapacity);

/**
 * Close the given <code>pl_localeRuns</code> object. Once this
 * call returns, the object can no longer be referenced.
 *
 * @param localeRuns is the <code>pl_localeRuns</code> object.
 *
 * @internal
 */
U_INTERNAL void U_EXPORT2
pl_closeLocaleRuns(pl_localeRuns *localeRuns);

/**
 * Get the number of font runs.
 *
 * @param localeRuns is the <code>pl_localeRuns</code> object.
 *
 * @return the number of entries in the limit indices array.
 *
 * @internal
 */
U_INTERNAL le_int32 U_EXPORT2
pl_getLocaleRunCount(const pl_localeRuns *localeRuns);

/**
 * Reset the number of locale runs to zero.
 *
 * @param localeRuns is the <code>pl_localeRuns</code> object.
 *
 * @internal
 */
U_INTERNAL void U_EXPORT2
pl_resetLocaleRuns(pl_localeRuns *localeRuns);

/**
 * Get the limit index for the last font run. This is the
 * number of characters in the text.
 *
 * @param localeRuns is the <code>pl_localeRuns</code> object.
 *
 * @return the last limit index.
 *
 * @internal
 */
U_INTERNAL le_int32 U_EXPORT2
pl_getLocaleRunLastLimit(const pl_localeRuns *localeRuns);

/**
 * Get the limit index for a particular font run.
 *
 * @param localeRuns is the <code>pl_localeRuns</code> object.
 * @param run is the run. This is an index into the limit index array.
 *
 * @return the limit index for the run, or -1 if <code>run</code> is out of bounds.
 *
 * @internal
 */
U_INTERNAL le_int32 U_EXPORT2
pl_getLocaleRunLimit(const pl_localeRuns *localeRuns,
                     le_int32 run);

/**
 * Get the <code>le_font</code> object assoicated with the given run
 * of text. Use <code>pl_getLocaleRunLimit(run)</code> to get the corresponding
 * limit index.
 *
 * @param localeRuns is the <code>pl_localeRuns</code> object.
 * @param run is the index into the font and limit indices arrays.
 *
 * @return the <code>le_font</code> associated with the given text run.
 *
 * @internal
 */
U_INTERNAL const char * U_EXPORT2
pl_getLocaleRunLocale(const pl_localeRuns *localeRuns,
                      le_int32 run);


/**
 * Add a new run to the given <code>pl_localeRuns</code> object.
 *
 * If the <code>pl_localeRuns</code> object was not created by calling
 * <code>pl_openEmptyLocaleRuns</code>, this method will return a run index of -1.
 *
 * @param localeRuns is the <code>pl_localeRuns</code> object.
 *
 * @param locale is the name of the locale to add. This name must
 *               remain valid until the <code>pl_localeRuns</code> object is closed.
 *
 * @param limit is the limit index to add
 *
 * @return the run index where the font and limit index were stored, or -1 if 
 *         the run cannot be added.
 *
 * @internal
 */
U_INTERNAL le_int32 U_EXPORT2
pl_addLocaleRun(pl_localeRuns *localeRuns,
                const char *locale,
                le_int32 limit);

#endif
