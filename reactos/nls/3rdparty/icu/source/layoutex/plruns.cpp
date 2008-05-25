/*
 *
 * (C) Copyright IBM Corp. 1998-2007 - All Rights Reserved
 *
 */

#include "layout/LETypes.h"
#include "layout/loengine.h"
#include "layout/plruns.h"

#include "unicode/locid.h"

#include "layout/LayoutEngine.h"
#include "layout/RunArrays.h"

U_NAMESPACE_USE

U_CAPI pl_fontRuns * U_EXPORT2
pl_openFontRuns(const le_font **fonts,
                const le_int32 *limits,
                le_int32 count)
{
    return (pl_fontRuns *) new FontRuns((const LEFontInstance **) fonts, limits, count);
}

U_CAPI pl_fontRuns * U_EXPORT2
pl_openEmptyFontRuns(le_int32 initialCapacity)
{
    return (pl_fontRuns *) new FontRuns(initialCapacity);
}

U_CAPI void U_EXPORT2
pl_closeFontRuns(pl_fontRuns *fontRuns)
{
    FontRuns *fr = (FontRuns *) fontRuns;

    delete fr;
}

U_CAPI le_int32 U_EXPORT2
pl_getFontRunCount(const pl_fontRuns *fontRuns)
{
    const FontRuns *fr = (const FontRuns *) fontRuns;

    if (fr == NULL) {
        return -1;
    }

    return fr->getCount();
}

U_CAPI void U_EXPORT2
pl_resetFontRuns(pl_fontRuns *fontRuns)
{
    FontRuns *fr = (FontRuns *) fontRuns;

    if (fr != NULL) {
        fr->reset();
    }
}

U_CAPI le_int32 U_EXPORT2
pl_getFontRunLastLimit(const pl_fontRuns *fontRuns)
{
    const FontRuns *fr = (const FontRuns *) fontRuns;

    if (fr == NULL) {
        return -1;
    }

    return fr->getLimit();
}

U_CAPI le_int32 U_EXPORT2
pl_getFontRunLimit(const pl_fontRuns *fontRuns,
                   le_int32 run)
{
    const FontRuns *fr = (const FontRuns *) fontRuns;

    if (fr == NULL) {
        return -1;
    }

    return fr->getLimit(run);
}

U_CAPI const le_font * U_EXPORT2
pl_getFontRunFont(const pl_fontRuns *fontRuns,
                    le_int32 run)
{
    const FontRuns *fr = (const FontRuns *) fontRuns;

    if (fr == NULL) {
        return NULL;
    }

    return (const le_font *) fr->getFont(run);
}

U_CAPI le_int32 U_EXPORT2
pl_addFontRun(pl_fontRuns *fontRuns,
              const le_font *font,
              le_int32 limit)
{
    FontRuns *fr = (FontRuns *) fontRuns;

    if (fr == NULL) {
        return -1;
    }

    return fr->add((const LEFontInstance *) font, limit);
}

U_CAPI pl_valueRuns * U_EXPORT2
pl_openValueRuns(const le_int32 *values,
                 const le_int32 *limits,
                 le_int32 count)
{
    return (pl_valueRuns *) new ValueRuns(values, limits, count);
}

U_CAPI pl_valueRuns * U_EXPORT2
pl_openEmptyValueRuns(le_int32 initialCapacity)
{
    return (pl_valueRuns *) new ValueRuns(initialCapacity);
}

U_CAPI void U_EXPORT2
pl_closeValueRuns(pl_valueRuns *valueRuns)
{
    ValueRuns *vr = (ValueRuns *) valueRuns;

    delete vr;
}

U_CAPI le_int32 U_EXPORT2
pl_getValueRunCount(const pl_valueRuns *valueRuns)
{
    const ValueRuns *vr = (const ValueRuns *) valueRuns;

    if (vr == NULL) {
        return -1;
    }

    return vr->getCount();
}

U_CAPI void U_EXPORT2
pl_resetValueRuns(pl_valueRuns *valueRuns)
{
    ValueRuns *vr = (ValueRuns *) valueRuns;

    if (vr != NULL) {
        vr->reset();
    }
}

U_CAPI le_int32 U_EXPORT2
pl_getValueRunLastLimit(const pl_valueRuns *valueRuns)
{
    const ValueRuns *vr = (const ValueRuns *) valueRuns;

    if (vr == NULL) {
        return -1;
    }

    return vr->getLimit();
}

U_CAPI le_int32 U_EXPORT2
pl_getValueRunLimit(const pl_valueRuns *valueRuns,
                    le_int32 run)
{
    const ValueRuns *vr = (const ValueRuns *) valueRuns;

    if (vr == NULL) {
        return -1;
    }

    return vr->getLimit(run);
}

U_CAPI le_int32 U_EXPORT2
pl_getValueRunValue(const pl_valueRuns *valueRuns,
                     le_int32 run)
{
    const ValueRuns *vr = (const ValueRuns *) valueRuns;

    if (vr == NULL) {
        return -1;
    }

    return vr->getValue(run);
}

U_CAPI le_int32 U_EXPORT2
pl_addValueRun(pl_valueRuns *valueRuns,
               le_int32 value,
               le_int32 limit)
{
    ValueRuns *vr = (ValueRuns *) valueRuns;

    if (vr == NULL) {
        return -1;
    }

    return vr->add(value, limit);
}

U_NAMESPACE_BEGIN
class ULocRuns : public LocaleRuns
{
public:
    /**
     * Construct a <code>LocaleRuns</code> object from pre-existing arrays of locales
     * and limit indices.
     *
     * @param locales is the address of an array of locale name strings. This array,
     *                and the <code>Locale</code> objects to which it points, must remain valid until
     *                the <code>LocaleRuns</code> object is destroyed.
     *
     * @param limits is the address of an array of limit indices. This array must remain valid until the
     *               <code>LocaleRuns</code> object is destroyed.
     *
     * @param count is the number of entries in the two arrays.
     *
     * @draft ICU 3.8
     */
    ULocRuns(const char **locales, const le_int32 *limits, le_int32 count);

    /**
     * Construct an empty <code>LoIDRuns</code> object. Clients can add locale and limit
     * indices arrays using the <code>add</code> method.
     *
     * @param initialCapacity is the initial size of the locale and limit indices arrays. If
     *        this value is zero, no arrays will be allocated.
     *
     * @see add
     *
     * @draft ICU 3.8
     */
    ULocRuns(le_int32 initialCapacity);

    /**
     * The destructor; virtual so that subclass destructors are invoked as well.
     *
     * @draft ICU 3.8
     */
    virtual ~ULocRuns();

    /**
     * Get the name of the locale assoicated with the given run
     * of text. Use <code>RunArray::getLimit(run)</code> to get the corresponding
     * limit index.
     *
     * @param run is the index into the font and limit indices arrays.
     *
     * @return the locale name associated with the given text run.
     *
     * @see RunArray::getLimit
     *
     * @draft ICU 3.8
     */
    const char *getLocaleName(le_int32 run) const;

    /**
     * Add a <code>Locale</code> and limit index pair to the data arrays and return
     * the run index where the data was stored. This  method calls
     * <code>RunArray::add(limit)</code> which will create or grow the arrays as needed.
     *
     * If the <code>ULocRuns</code> object was created with a client-supplied
     * locale and limit indices arrays, this method will return a run index of -1.
     *
     * Subclasses should not override this method. Rather they should provide a new <code>add</code>
     * method which takes a locale name and a limit index along with whatever other data they implement.
     * The new <code>add</code> method should first call this method to grow the font and limit indices
     * arrays, and use the returned run index to store data their own arrays.
     *
     * @param locale is the name of the locale to add. This object must remain valid
     *               until the <code>ULocRuns</code> object is destroyed.
     *
     * @param limit is the limit index to add
     *
     * @return the run index where the locale and limit index were stored, or -1 if the data cannot be stored.
     *
     * @draft ICU 3.8
     */
    le_int32 add(const char *locale, le_int32 limit);

    /**
     * ICU "poor man's RTTI", returns a UClassID for this class.
     *
     * @draft ICU 3.8
     */
    static inline UClassID getStaticClassID();

    /**
     * ICU "poor man's RTTI", returns a UClassID for the actual class.
     *
     * @draft ICU 3.8
     */
    virtual inline UClassID getDynamicClassID() const;

protected:
    virtual void init(le_int32 capacity);
    virtual void grow(le_int32 capacity);

private:

    inline ULocRuns();
    inline ULocRuns(const ULocRuns &other);
    inline ULocRuns &operator=(const ULocRuns & /*other*/) { return *this; };
    const char **fLocaleNames;
    Locale **fLocalesCopy;
};

inline ULocRuns::ULocRuns()
    : LocaleRuns(0), fLocaleNames(NULL)
{
    // nothing else to do...
}

inline ULocRuns::ULocRuns(const ULocRuns & /*other*/)
    : LocaleRuns(0), fLocaleNames(NULL)
{
    // nothing else to do...
}

static const Locale **getLocales(const char **localeNames, le_int32 count)
{
    Locale **locales = LE_NEW_ARRAY(Locale *, count);

    for (int i = 0; i < count; i += 1) {
        locales[i] = new Locale(Locale::createFromName(localeNames[i]));
    }

    return (const Locale **) locales;
}

ULocRuns::ULocRuns(const char **locales, const le_int32 *limits, le_int32 count)
    : LocaleRuns(getLocales(locales, count), limits, count), fLocaleNames(locales)
{
    // nothing else to do...
}

ULocRuns::ULocRuns(le_int32 initialCapacity)
    : LocaleRuns(initialCapacity), fLocaleNames(NULL)
{
    if(initialCapacity > 0) {
        fLocaleNames = LE_NEW_ARRAY(const char *, initialCapacity);
    }
}

ULocRuns::~ULocRuns()
{
    le_int32 count = getCount();

    for(int i = 0; i < count; i += 1) {
        delete fLocales[i];
    }

    if (fClientArrays) {
        LE_DELETE_ARRAY(fLocales);
        fLocales = NULL;
    } else {
        LE_DELETE_ARRAY(fLocaleNames);
        fLocaleNames = NULL;
    }
}

void ULocRuns::init(le_int32 capacity)
{
    LocaleRuns::init(capacity);
    fLocaleNames = LE_NEW_ARRAY(const char *, capacity);
}

void ULocRuns::grow(le_int32 capacity)
{
    RunArray::grow(capacity);
    fLocaleNames = (const char **) LE_GROW_ARRAY(fLocaleNames, capacity);
}

le_int32 ULocRuns::add(const char *locale, le_int32 limit)
{
    Locale *loc = new Locale(Locale::createFromName(locale));
    le_int32 index = LocaleRuns::add(loc, limit);

    if (index >= 0) {
        char **localeNames = (char **) fLocaleNames;

        localeNames[index] = (char *) locale;
    }

    return index;
}

const char *ULocRuns::getLocaleName(le_int32 run) const
{
    if (run < 0 || run >= getCount()) {
        return NULL;
    }

    return fLocaleNames[run];
}
UOBJECT_DEFINE_RTTI_IMPLEMENTATION(ULocRuns)
U_NAMESPACE_END

U_CAPI pl_localeRuns * U_EXPORT2
pl_openLocaleRuns(const char **locales,
                  const le_int32 *limits,
                  le_int32 count)
{
    return (pl_localeRuns *) new ULocRuns(locales, limits, count);
}

U_CAPI pl_localeRuns * U_EXPORT2
pl_openEmptyLocaleRuns(le_int32 initialCapacity)
{
    return (pl_localeRuns *) new ULocRuns(initialCapacity);
}

U_CAPI void U_EXPORT2
pl_closeLocaleRuns(pl_localeRuns *localeRuns)
{
    ULocRuns *lr = (ULocRuns *) localeRuns;

    delete lr;
}

U_CAPI le_int32 U_EXPORT2
pl_getLocaleRunCount(const pl_localeRuns *localeRuns)
{
    const ULocRuns *lr = (const ULocRuns *) localeRuns;

    if (lr == NULL) {
        return -1;
    }

    return lr->getCount();
}

U_CAPI void U_EXPORT2
pl_resetLocaleRuns(pl_localeRuns *localeRuns)
{
    ULocRuns *lr = (ULocRuns *) localeRuns;

    if (lr != NULL) {
        lr->reset();
    }
}

U_CAPI le_int32 U_EXPORT2
pl_getLocaleRunLastLimit(const pl_localeRuns *localeRuns)
{
    const ULocRuns *lr = (const ULocRuns *) localeRuns;

    if (lr == NULL) {
        return -1;
    }

    return lr->getLimit();
}

U_CAPI le_int32 U_EXPORT2
pl_getLocaleRunLimit(const pl_localeRuns *localeRuns,
                     le_int32 run)
{
    const ULocRuns *lr = (const ULocRuns *) localeRuns;

    if (lr == NULL) {
        return -1;
    }

    return lr->getLimit(run);
}

U_CAPI const char * U_EXPORT2
pl_getLocaleRunLocale(const pl_localeRuns *localeRuns,
                      le_int32 run)
{
    const ULocRuns *lr = (const ULocRuns *) localeRuns;

    if (lr == NULL) {
        return NULL;
    }

    return lr->getLocaleName(run);
}

U_CAPI le_int32 U_EXPORT2
pl_addLocaleRun(pl_localeRuns *localeRuns,
                const char *locale,
                le_int32 limit)
{
    ULocRuns *lr = (ULocRuns *) localeRuns;

    if (lr == NULL) {
        return -1;
    }

    return lr->add(locale, limit);
}
