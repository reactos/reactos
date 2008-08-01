/*
 **********************************************************************
 *   Copyright (C) 2003-2007, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 **********************************************************************
 */

#ifndef __RUNARRAYS_H

#define __RUNARRAYS_H

#include "layout/LETypes.h"
#include "layout/LEFontInstance.h"

#include "unicode/utypes.h"
#include "unicode/locid.h"

/**
 * \file 
 * \brief C++ API: base class for building classes which represent data that is associated with runs of text.
 */
 
U_NAMESPACE_BEGIN

/**
 * The initial size of an array if it is unspecified.
 *
 * @stable ICU 3.2
 */
#define INITIAL_CAPACITY 16

/**
 * When an array needs to grow, it will double in size until
 * it becomes this large, then it will grow by this amount.
 *
 * @stable ICU 3.2
 */
#define CAPACITY_GROW_LIMIT 128

/**
 * The <code>RunArray</code> class is a base class for building classes
 * which represent data that is associated with runs of text. This class
 * maintains an array of limit indices into the text, subclasses
 * provide one or more arrays of data.
 *
 * @stable ICU 3.2
 */
class U_LAYOUTEX_API RunArray : public UObject
{
public:
    /**
     * Construct a <code>RunArray</code> object from a pre-existing
     * array of limit indices.
     *
     * @param limits is an array of limit indices. This array must remain
     *               valid until the <code>RunArray</code> object is destroyed.
     *
     * @param count is the number of entries in the limit array.
     *
     * @stable ICU 3.2
     */
    inline RunArray(const le_int32 *limits, le_int32 count);

    /**
     * Construct an empty <code>RunArray</code> object. Clients can add limit
     * indices array using the <code>add</code> method.
     *
     * @param initialCapacity is the initial size of the limit indices array. If
     *        this value is zero, no array will be allocated.
     *
     * @see add
     *
     * @stable ICU 3.2
     */
    RunArray(le_int32 initialCapacity);

    /**
     * The destructor; virtual so that subclass destructors are invoked as well.
     *
     * @stable ICU 3.2
     */
    virtual ~RunArray();

    /**
     * Get the number of entries in the limit indices array.
     *
     * @return the number of entries in the limit indices array.
     *
     * @stable ICU 3.2
     */
    inline le_int32 getCount() const;

    /**
     * Reset the limit indices array. This method sets the number of entries in the
     * limit indices array to zero. It does not delete the array.
     *
     * Note: Subclass arrays will also be reset and not deleted.
     *
     * @stable ICU 3.6
     */
    inline void reset();

    /**
     * Get the last limit index. This is the number of characters in
     * the text.
     *
     * @return the last limit index.
     *
     * @stable ICU 3.2
     */
    inline le_int32 getLimit() const;

    /**
     * Get the limit index for a particular run of text.
     *
     * @param run is the run. This is an index into the limit index array.
     *
     * @return the limit index for the run, or -1 if <code>run</code> is out of bounds.
     *
     * @stable ICU 3.2
     */
    inline le_int32 getLimit(le_int32 run) const;

    /**
     * Add a limit index to the limit indices array and return the run index
     * where it was stored. If the array does not exist, it will be created by
     * calling the <code>init</code> method. If it is full, it will be grown by
     * calling the <code>grow</code> method.
     *
     * If the <code>RunArray</code> object was created with a client-supplied
     * limit indices array, this method will return a run index of -1.
     *
     * Subclasses should not override this method. Rather they should provide
     * a new <code>add</code> method which takes a limit index along with whatever
     * other data they implement. The new <code>add</code> method should
     * first call this method to grow the data arrays, and use the return value
     * to store the data in their own arrays.
     *
     * @param limit is the limit index to add to the array.
     *
     * @return the run index where the limit index was stored, or -1 if the limit index cannt be stored.
     *
     * @see init
     * @see grow
     *
     * @stable ICU 3.2
     */
    le_int32 add(le_int32 limit);

    /**
     * ICU "poor man's RTTI", returns a UClassID for this class.
     *
     * @stable ICU 3.2
     */
    static inline UClassID getStaticClassID() { return (UClassID)&fgClassID; }

    /**
     * ICU "poor man's RTTI", returns a UClassID for the actual class.
     *
     * @stable ICU 3.2
     */
    virtual inline UClassID getDynamicClassID() const { return getStaticClassID(); }

protected:
    /**
     * Create a data array with the given initial size. This method will be
     * called by the <code>add</code> method if there is no limit indices
     * array. Subclasses which override this method must also call it from
     * the overriding method to create the limit indices array.
     *
     * @param capacity is the initial size of the data array.
     *
     * @see add
     *
     * @stable ICU 3.2
     */
    virtual void init(le_int32 capacity);

    /**
     * Grow a data array to the given initial size. This method will be
     * called by the <code>add</code> method if the limit indices
     * array is full. Subclasses which override this method must also call it from
     * the overriding method to grow the limit indices array.
     *
     * @param capacity is the initial size of the data array.
     *
     * @see add
     *
     * @stable ICU 3.2
     */
    virtual void grow(le_int32 capacity);

    /**
     * Set by the constructors to indicate whether
     * or not the client supplied the data arrays.
     * If they were supplied by the client, the 
     * <code>add</code> method won't change the arrays
     * and the destructor won't delete them.
     *
     * @stable ICU 3.2
     */
    le_bool fClientArrays;

private:
    /**
     * The address of this static class variable serves as this class's ID
     * for ICU "poor man's RTTI".
     */
    static const char fgClassID;

    le_int32 ensureCapacity();

    inline RunArray();
    inline RunArray(const RunArray & /*other*/);
    inline RunArray &operator=(const RunArray & /*other*/) { return *this; };

    const le_int32 *fLimits;
          le_int32  fCount;
          le_int32  fCapacity;
};

inline RunArray::RunArray()
    : UObject(), fClientArrays(FALSE), fLimits(NULL), fCount(0), fCapacity(0)
{
    // nothing else to do...
}

inline RunArray::RunArray(const RunArray & /*other*/)
    : UObject(), fClientArrays(FALSE), fLimits(NULL), fCount(0), fCapacity(0)
{
    // nothing else to do...
}

inline RunArray::RunArray(const le_int32 *limits, le_int32 count)
    : UObject(), fClientArrays(TRUE), fLimits(limits), fCount(count), fCapacity(count)
{
    // nothing else to do...
}

inline le_int32 RunArray::getCount() const
{
    return fCount;
}

inline void RunArray::reset()
{
    fCount = 0;
}

inline le_int32 RunArray::getLimit(le_int32 run) const
{
    if (run < 0 || run >= fCount) {
        return -1;
    }

    return fLimits[run];
}

inline le_int32 RunArray::getLimit() const
{
    return getLimit(fCount - 1);
}

/**
 * The <code>FontRuns</code> class associates pointers to <code>LEFontInstance</code>
 * objects with runs of text.
 *
 * @stable ICU 3.2
 */
class U_LAYOUTEX_API FontRuns : public RunArray
{
public:
    /**
     * Construct a <code>FontRuns</code> object from pre-existing arrays of fonts
     * and limit indices.
     *
     * @param fonts is the address of an array of pointers to <code>LEFontInstance</code> objects. This
     *              array, and the <code>LEFontInstance</code> objects to which it points must remain
     *              valid until the <code>FontRuns</code> object is destroyed.
     *
     * @param limits is the address of an array of limit indices. This array must remain valid until
     *               the <code>FontRuns</code> object is destroyed.
     *
     * @param count is the number of entries in the two arrays.
     *
     * @stable ICU 3.2
     */
    inline FontRuns(const LEFontInstance **fonts, const le_int32 *limits, le_int32 count);

    /**
     * Construct an empty <code>FontRuns</code> object. Clients can add font and limit
     * indices arrays using the <code>add</code> method.
     *
     * @param initialCapacity is the initial size of the font and limit indices arrays. If
     *        this value is zero, no arrays will be allocated.
     *
     * @see add
     *
     * @stable ICU 3.2
     */
    FontRuns(le_int32 initialCapacity);

    /**
     * The destructor; virtual so that subclass destructors are invoked as well.
     *
     * @stable ICU 3.2
     */
    virtual ~FontRuns();

    /**
     * Get the <code>LEFontInstance</code> object assoicated with the given run
     * of text. Use <code>RunArray::getLimit(run)</code> to get the corresponding
     * limit index.
     *
     * @param run is the index into the font and limit indices arrays.
     *
     * @return the <code>LEFontInstance</code> associated with the given text run.
     *
     * @see RunArray::getLimit
     *
     * @stable ICU 3.2
     */
    const LEFontInstance *getFont(le_int32 run) const;


    /**
     * Add an <code>LEFontInstance</code> and limit index pair to the data arrays and return
     * the run index where the data was stored. This  method calls
     * <code>RunArray::add(limit)</code> which will create or grow the arrays as needed.
     *
     * If the <code>FontRuns</code> object was created with a client-supplied
     * font and limit indices arrays, this method will return a run index of -1.
     *
     * Subclasses should not override this method. Rather they should provide a new <code>add</code>
     * method which takes a font and a limit index along with whatever other data they implement.
     * The new <code>add</code> method should first call this method to grow the font and limit indices
     * arrays, and use the returned run index to store data their own arrays.
     *
     * @param font is the address of the <code>LEFontInstance</code> to add. This object must
     *             remain valid until the <code>FontRuns</code> object is destroyed.
     *
     * @param limit is the limit index to add
     *
     * @return the run index where the font and limit index were stored, or -1 if the data cannot be stored.
     *
     * @stable ICU 3.2
     */
    le_int32 add(const LEFontInstance *font, le_int32 limit);

    /**
     * ICU "poor man's RTTI", returns a UClassID for this class.
     *
     * @stable ICU 3.2
     */
    static inline UClassID getStaticClassID() { return (UClassID)&fgClassID; }

    /**
     * ICU "poor man's RTTI", returns a UClassID for the actual class.
     *
     * @stable ICU 3.2
     */
    virtual inline UClassID getDynamicClassID() const { return getStaticClassID(); }

protected:
    virtual void init(le_int32 capacity);
    virtual void grow(le_int32 capacity);

private:

    inline FontRuns();
    inline FontRuns(const FontRuns &other);
    inline FontRuns &operator=(const FontRuns & /*other*/) { return *this; };

    /**
     * The address of this static class variable serves as this class's ID
     * for ICU "poor man's RTTI".
     */
    static const char fgClassID;

    const LEFontInstance **fFonts;
};

inline FontRuns::FontRuns()
    : RunArray(0), fFonts(NULL)
{
    // nothing else to do...
}

inline FontRuns::FontRuns(const FontRuns & /*other*/)
    : RunArray(0), fFonts(NULL)
{
    // nothing else to do...
}

inline FontRuns::FontRuns(const LEFontInstance **fonts, const le_int32 *limits, le_int32 count)
    : RunArray(limits, count), fFonts(fonts)
{
    // nothing else to do...
}

/**
 * The <code>LocaleRuns</code> class associates pointers to <code>Locale</code>
 * objects with runs of text.
 *
 * @stable ICU 3.2
 */
class U_LAYOUTEX_API LocaleRuns : public RunArray
{
public:
    /**
     * Construct a <code>LocaleRuns</code> object from pre-existing arrays of locales
     * and limit indices.
     *
     * @param locales is the address of an array of pointers to <code>Locale</code> objects. This array,
     *                and the <code>Locale</code> objects to which it points, must remain valid until
     *                the <code>LocaleRuns</code> object is destroyed.
     *
     * @param limits is the address of an array of limit indices. This array must remain valid until the
     *               <code>LocaleRuns</code> object is destroyed.
     *
     * @param count is the number of entries in the two arrays.
     *
     * @stable ICU 3.2
     */
    inline LocaleRuns(const Locale **locales, const le_int32 *limits, le_int32 count);

    /**
     * Construct an empty <code>LocaleRuns</code> object. Clients can add locale and limit
     * indices arrays using the <code>add</code> method.
     *
     * @param initialCapacity is the initial size of the locale and limit indices arrays. If
     *        this value is zero, no arrays will be allocated.
     *
     * @see add
     *
     * @stable ICU 3.2
     */
    LocaleRuns(le_int32 initialCapacity);

    /**
     * The destructor; virtual so that subclass destructors are invoked as well.
     *
     * @stable ICU 3.2
     */
    virtual ~LocaleRuns();

    /**
     * Get the <code>Locale</code> object assoicated with the given run
     * of text. Use <code>RunArray::getLimit(run)</code> to get the corresponding
     * limit index.
     *
     * @param run is the index into the font and limit indices arrays.
     *
     * @return the <code>Locale</code> associated with the given text run.
     *
     * @see RunArray::getLimit
     *
     * @stable ICU 3.2
     */
    const Locale *getLocale(le_int32 run) const;


    /**
     * Add a <code>Locale</code> and limit index pair to the data arrays and return
     * the run index where the data was stored. This  method calls
     * <code>RunArray::add(limit)</code> which will create or grow the arrays as needed.
     *
     * If the <code>LocaleRuns</code> object was created with a client-supplied
     * locale and limit indices arrays, this method will return a run index of -1.
     *
     * Subclasses should not override this method. Rather they should provide a new <code>add</code>
     * method which takes a locale and a limit index along with whatever other data they implement.
     * The new <code>add</code> method should first call this method to grow the font and limit indices
     * arrays, and use the returned run index to store data their own arrays.
     *
     * @param locale is the address of the <code>Locale</code> to add. This object must remain valid
     *               until the <code>LocaleRuns</code> object is destroyed.
     *
     * @param limit is the limit index to add
     *
     * @return the run index where the locale and limit index were stored, or -1 if the data cannot be stored.
     *
     * @stable ICU 3.2
     */
    le_int32 add(const Locale *locale, le_int32 limit);

    /**
     * ICU "poor man's RTTI", returns a UClassID for this class.
     *
     * @stable ICU 3.2
     */
    static inline UClassID getStaticClassID() { return (UClassID)&fgClassID; }

    /**
     * ICU "poor man's RTTI", returns a UClassID for the actual class.
     *
     * @stable ICU 3.2
     */
    virtual inline UClassID getDynamicClassID() const { return getStaticClassID(); }

protected:
    virtual void init(le_int32 capacity);
    virtual void grow(le_int32 capacity);

    const Locale **fLocales;

private:

    inline LocaleRuns();
    inline LocaleRuns(const LocaleRuns &other);
    inline LocaleRuns &operator=(const LocaleRuns & /*other*/) { return *this; };

    /**
     * The address of this static class variable serves as this class's ID
     * for ICU "poor man's RTTI".
     */
    static const char fgClassID;
};

inline LocaleRuns::LocaleRuns()
    : RunArray(0), fLocales(NULL)
{
    // nothing else to do...
}

inline LocaleRuns::LocaleRuns(const LocaleRuns & /*other*/)
    : RunArray(0), fLocales(NULL)
{
    // nothing else to do...
}

inline LocaleRuns::LocaleRuns(const Locale **locales, const le_int32 *limits, le_int32 count)
    : RunArray(limits, count), fLocales(locales)
{
    // nothing else to do...
}

/**
 * The <code>ValueRuns</code> class associates integer values with runs of text.
 *
 * @stable ICU 3.2
 */
class U_LAYOUTEX_API ValueRuns : public RunArray
{
public:
    /**
     * Construct a <code>ValueRuns</code> object from pre-existing arrays of values
     * and limit indices.
     *
     * @param values is the address of an array of integer. This array must remain valid until
     *               the <code>ValueRuns</code> object is destroyed.
     *
     * @param limits is the address of an array of limit indices. This array must remain valid until
     *               the <code>ValueRuns</code> object is destroyed.
     *
     * @param count is the number of entries in the two arrays.
     *
     * @stable ICU 3.2
     */
    inline ValueRuns(const le_int32 *values, const le_int32 *limits, le_int32 count);

    /**
     * Construct an empty <code>ValueRuns</code> object. Clients can add value and limit
     * indices arrays using the <code>add</code> method.
     *
     * @param initialCapacity is the initial size of the value and limit indices arrays. If
     *        this value is zero, no arrays will be allocated.
     *
     * @see add
     *
     * @stable ICU 3.2
     */
    ValueRuns(le_int32 initialCapacity);

    /**
     * The destructor; virtual so that subclass destructors are invoked as well.
     *
     * @stable ICU 3.2
     */
    virtual ~ValueRuns();

    /**
     * Get the integer value assoicated with the given run
     * of text. Use <code>RunArray::getLimit(run)</code> to get the corresponding
     * limit index.
     *
     * @param run is the index into the font and limit indices arrays.
     *
     * @return the integer value associated with the given text run.
     *
     * @see RunArray::getLimit
     *
     * @stable ICU 3.2
     */
    le_int32 getValue(le_int32 run) const;


    /**
     * Add an integer value and limit index pair to the data arrays and return
     * the run index where the data was stored. This  method calls
     * <code>RunArray::add(limit)</code> which will create or grow the arrays as needed.
     *
     * If the <code>ValueRuns</code> object was created with a client-supplied
     * font and limit indices arrays, this method will return a run index of -1.
     *
     * Subclasses should not override this method. Rather they should provide a new <code>add</code>
     * method which takes an integer value and a limit index along with whatever other data they implement.
     * The new <code>add</code> method should first call this method to grow the font and limit indices
     * arrays, and use the returned run index to store data their own arrays.
     *
     * @param value is the integer value to add
     *
     * @param limit is the limit index to add
     *
     * @return the run index where the value and limit index were stored, or -1 if the data cannot be stored.
     *
     * @stable ICU 3.2
     */
    le_int32 add(le_int32 value, le_int32 limit);

    /**
     * ICU "poor man's RTTI", returns a UClassID for this class.
     *
     * @stable ICU 3.2
     */
    static inline UClassID getStaticClassID() { return (UClassID)&fgClassID; }

    /**
     * ICU "poor man's RTTI", returns a UClassID for the actual class.
     *
     * @stable ICU 3.2
     */
    virtual inline UClassID getDynamicClassID() const { return getStaticClassID(); }

protected:
    virtual void init(le_int32 capacity);
    virtual void grow(le_int32 capacity);

private:

    inline ValueRuns();
    inline ValueRuns(const ValueRuns &other);
    inline ValueRuns &operator=(const ValueRuns & /*other*/) { return *this; };

    /**
     * The address of this static class variable serves as this class's ID
     * for ICU "poor man's RTTI".
     */
    static const char fgClassID;

    const le_int32 *fValues;
};

inline ValueRuns::ValueRuns()
    : RunArray(0), fValues(NULL)
{
    // nothing else to do...
}

inline ValueRuns::ValueRuns(const ValueRuns & /*other*/)
    : RunArray(0), fValues(NULL)
{
    // nothing else to do...
}

inline ValueRuns::ValueRuns(const le_int32 *values, const le_int32 *limits, le_int32 count)
    : RunArray(limits, count), fValues(values)
{
    // nothing else to do...
}

U_NAMESPACE_END
#endif
