/*
* Copyright (C) 1997-2006, International Business Machines Corporation and others. All Rights Reserved.
********************************************************************************
*
* File MSGFMT.H
*
* Modification History:
*
*   Date        Name        Description
*   02/19/97    aliu        Converted from java.
*   03/20/97    helena      Finished first cut of implementation.
*   07/22/98    stephen     Removed operator!= (defined in Format)
*   08/19/2002  srl         Removing Javaisms
********************************************************************************
*/

#ifndef MSGFMT_H
#define MSGFMT_H

#include "unicode/utypes.h"

/**
 * \file 
 * \brief C++ API: Formats messages in a language-neutral way.
 */
 
#if !UCONFIG_NO_FORMATTING

#include "unicode/format.h"
#include "unicode/locid.h"
#include "unicode/parseerr.h"

U_NAMESPACE_BEGIN

class NumberFormat;
class DateFormat;

/**
 *
 * A MessageFormat produces concatenated messages in a
 * language-neutral way.  It should be used for all string
 * concatenations that are visible to end users.
 * <P>
 * A MessageFormat contains an array of <EM>subformats</EM> arranged
 * within a <EM>template string</EM>.  Together, the subformats and
 * template string determine how the MessageFormat will operate during
 * formatting and parsing.
 * <P>
 * Typically, both the subformats and the template string are
 * specified at once in a <EM>pattern</EM>.  By using different
 * patterns for different locales, messages may be localized.
 * <P>
 * During formatting, the MessageFormat takes an array of arguments
 * and produces a user-readable string.  Each argument is a
 * Formattable object; they may be passed in in an array, or as a
 * single Formattable object which itself contains an array.  Each
 * argument is matched up with its corresponding subformat, which then
 * formats it into a string.  The resultant strings are then assembled
 * within the string template of the MessageFormat to produce the
 * final output string.
 * <P>
 * During parsing, an input string is matched against the string
 * template of the MessageFormat to produce an array of Formattable
 * objects.  Plain text of the template string is matched directly
 * against intput text.  At each position in the template string where
 * a subformat is located, the subformat is called to parse the
 * corresponding segment of input text to produce an output argument.
 * In this way, an array of arguments is created which together
 * constitute the parse result.
 * <P>
 * Parsing may fail or produce unexpected results in a number of
 * circumstances.
 * <UL>
 * <LI>If one of the arguments does not occur in the pattern, it
 * will be returned as a default Formattable.
 * <LI>If the format of an argument is loses information, such as with
 * a choice format where a large number formats to "many", then the
 * parse may not correspond to the originally formatted argument.
 * <LI>MessageFormat does not handle ChoiceFormat recursion during
 * parsing; such parses will fail.
 * <LI>Parsing will not always find a match (or the correct match) if
 * some part of the parse is ambiguous.  For example, if the pattern
 * "{1},{2}" is used with the string arguments {"a,b", "c"}, it will
 * format as "a,b,c".  When the result is parsed, it will return {"a",
 * "b,c"}.
 * <LI>If a single argument is formatted more than once in the string,
 * then the rightmost subformat in the pattern string will produce the
 * parse result; prior subformats with the same argument index will
 * have no effect.
 * </UL>
 * Here are some examples of usage:
 * <P>
 * Example 1:
 * <pre>
 * \code
 *     UErrorCode success = U_ZERO_ERROR;
 *     GregorianCalendar cal(success);
 *     Formattable arguments[] = {
 *         7L,
 *         Formattable( (Date) cal.getTime(success), Formattable::kIsDate),
 *         "a disturbance in the Force"
 *     };
 *
 *     UnicodeString result;
 *     MessageFormat::format(
 *          "At {1,time} on {1,date}, there was {2} on planet {0,number}.",
 *          arguments, 3, result, success );
 *
 *     cout << "result: " << result << endl;
 *     //<output>: At 4:34:20 PM on 23-Mar-98, there was a disturbance
 *     //             in the Force on planet 7.
 * \endcode
 * </pre>
 * Typically, the message format will come from resources, and the
 * arguments will be dynamically set at runtime.
 * <P>
 * Example 2:
 * <pre>
 *  \code
 *     success = U_ZERO_ERROR;
 *     Formattable testArgs[] = {3L, "MyDisk"};
 *
 *     MessageFormat form(
 *         "The disk \"{1}\" contains {0} file(s).", success );
 *
 *     UnicodeString string;
 *     FieldPosition fpos = 0;
 *     cout << "format: " << form.format(testArgs, 2, string, fpos, success ) << endl;
 *
 *     // output, with different testArgs:
 *     // output: The disk "MyDisk" contains 0 file(s).
 *     // output: The disk "MyDisk" contains 1 file(s).
 *     // output: The disk "MyDisk" contains 1,273 file(s).
 *  \endcode
 *  </pre>
 *
 *  The pattern is of the following form.  Legend:
 *  <pre>
 * \code
 *       {optional item}
 *       (group that may be repeated)*
 * \endcode
 *  </pre>
 *  Do not confuse optional items with items inside quotes braces, such
 *  as this: "{".  Quoted braces are literals.
 *  <pre>
 *  \code
 *       messageFormatPattern := string ( "{" messageFormatElement "}" string )*
 *
 *       messageFormatElement := argumentIndex { "," elementFormat }
 *
 *       elementFormat := "time" { "," datetimeStyle }
 *                      | "date" { "," datetimeStyle }
 *                      | "number" { "," numberStyle }
 *                      | "choice" "," choiceStyle
 *
 *       datetimeStyle := "short"
 *                      | "medium"
 *                      | "long"
 *                      | "full"
 *                      | dateFormatPattern
 *
 *       numberStyle :=   "currency"
 *                      | "percent"
 *                      | "integer"
 *                      | numberFormatPattern
 *
 *       choiceStyle :=   choiceFormatPattern
 * \endcode
 * </pre>
 * If there is no elementFormat, then the argument must be a string,
 * which is substituted. If there is no dateTimeStyle or numberStyle,
 * then the default format is used (e.g.  NumberFormat::createInstance(),
 * DateFormat::createTimeInstance(DateFormat::kDefault, ...) or DateFormat::createDateInstance(DateFormat::kDefault, ...). For
 * a ChoiceFormat, the pattern must always be specified, since there
 * is no default.
 * <P>
 * In strings, single quotes can be used to quote syntax characters.
 * A literal single quote is represented by '', both within and outside
 * of single-quoted segments.  Inside a
 * messageFormatElement, quotes are <EM>not</EM> removed. For example,
 * {1,number,$'#',##} will produce a number format with the pound-sign
 * quoted, with a result such as: "$#31,45".
 * <P>
 * If a pattern is used, then unquoted braces in the pattern, if any,
 * must match: that is, "ab {0} de" and "ab '}' de" are ok, but "ab
 * {0'}' de" and "ab } de" are not.
 * <p>
 * <dl><dt><b>Warning:</b><dd>The rules for using quotes within message
 * format patterns unfortunately have shown to be somewhat confusing.
 * In particular, it isn't always obvious to localizers whether single
 * quotes need to be doubled or not. Make sure to inform localizers about
 * the rules, and tell them (for example, by using comments in resource
 * bundle source files) which strings will be processed by MessageFormat.
 * Note that localizers may need to use single quotes in translated
 * strings where the original version doesn't have them.
 * <br>Note also that the simplest way to avoid the problem is to
 * use the real apostrophe (single quote) character U+2019 (') for
 * human-readable text, and to use the ASCII apostrophe (U+0027 ' )
 * only in program syntax, like quoting in MessageFormat.
 * See the annotations for U+0027 Apostrophe in The Unicode Standard.</p>
 * </dl>
 * <P>
 * The argumentIndex is a non-negative integer, which corresponds to the
 * index of the arguments presented in an array to be formatted.  The
 * first argument has argumentIndex 0.
 * <P>
 * It is acceptable to have unused arguments in the array.  With missing
 * arguments or arguments that are not of the right class for the
 * specified format, a failing UErrorCode result is set.
 * <P>
 * For more sophisticated patterns, you can use a ChoiceFormat to get
 * output:
 * <pre>
 * \code
 *     UErrorCode success = U_ZERO_ERROR;
 *     MessageFormat* form("The disk \"{1}\" contains {0}.", success);
 *     double filelimits[] = {0,1,2};
 *     UnicodeString filepart[] = {"no files","one file","{0,number} files"};
 *     ChoiceFormat* fileform = new ChoiceFormat(filelimits, filepart, 3);
 *     form.setFormat(1, *fileform); // NOT zero, see below
 *
 *     Formattable testArgs[] = {1273L, "MyDisk"};
 *
 *     UnicodeString string;
 *     FieldPosition fpos = 0;
 *     cout << form.format(testArgs, 2, string, fpos, success) << endl;
 *
 *     // output, with different testArgs
 *     // output: The disk "MyDisk" contains no files.
 *     // output: The disk "MyDisk" contains one file.
 *     // output: The disk "MyDisk" contains 1,273 files.
 * \endcode
 * </pre>
 * You can either do this programmatically, as in the above example,
 * or by using a pattern (see ChoiceFormat for more information) as in:
 * <pre>
 * \code
 *    form.applyPattern(
 *      "There {0,choice,0#are no files|1#is one file|1<are {0,number,integer} files}.");
 * \endcode
 * </pre>
 * <P>
 * <EM>Note:</EM> As we see above, the string produced by a ChoiceFormat in
 * MessageFormat is treated specially; occurences of '{' are used to
 * indicated subformats, and cause recursion.  If you create both a
 * MessageFormat and ChoiceFormat programmatically (instead of using
 * the string patterns), then be careful not to produce a format that
 * recurses on itself, which will cause an infinite loop.
 * <P>
 * <EM>Note:</EM> Subformats are numbered by their order in the pattern.
 * This is <EM>not</EM> the same as the argumentIndex.
 * <pre>
 * \code
 *    For example: with "abc{2}def{3}ghi{0}...",
 *
 *    format0 affects the first variable {2}
 *    format1 affects the second variable {3}
 *    format2 affects the second variable {0}
 * \endcode
 * </pre>
 *
 * <p><em>User subclasses are not supported.</em> While clients may write
 * subclasses, such code will not necessarily work and will not be
 * guaranteed to work stably from release to release.
 */
class U_I18N_API MessageFormat : public Format {
public:
    /**
     * Enum type for kMaxFormat.
     * @obsolete ICU 3.0.  The 10-argument limit was removed as of ICU 2.6,
     * rendering this enum type obsolete.
     */
    enum EFormatNumber {
        /**
         * The maximum number of arguments.
         * @obsolete ICU 3.0.  The 10-argument limit was removed as of ICU 2.6,
         * rendering this constant obsolete.
         */
        kMaxFormat = 10
    };

    /**
     * Constructs a new MessageFormat using the given pattern and the
     * default locale.
     *
     * @param pattern   Pattern used to construct object.
     * @param status    Input/output error code.  If the
     *                  pattern cannot be parsed, set to failure code.
     * @stable ICU 2.0
     */
    MessageFormat(const UnicodeString& pattern,
                  UErrorCode &status);

    /**
     * Constructs a new MessageFormat using the given pattern and locale.
     * @param pattern   Pattern used to construct object.
     * @param newLocale The locale to use for formatting dates and numbers.
     * @param status    Input/output error code.  If the
     *                  pattern cannot be parsed, set to failure code.
     * @stable ICU 2.0
     */
    MessageFormat(const UnicodeString& pattern,
                  const Locale& newLocale,
                        UErrorCode& status);
    /**
     * Constructs a new MessageFormat using the given pattern and locale.
     * @param pattern   Pattern used to construct object.
     * @param newLocale The locale to use for formatting dates and numbers.
     * @param parseError Struct to recieve information on position 
     *                   of error within the pattern.
     * @param status    Input/output error code.  If the
     *                  pattern cannot be parsed, set to failure code.
     * @stable ICU 2.0
     */
    MessageFormat(const UnicodeString& pattern,
                  const Locale& newLocale,
                  UParseError& parseError,
                  UErrorCode& status);
    /**
     * Constructs a new MessageFormat from an existing one.
     * @stable ICU 2.0
     */
    MessageFormat(const MessageFormat&);

    /**
     * Assignment operator.
     * @stable ICU 2.0
     */
    const MessageFormat& operator=(const MessageFormat&);

    /**
     * Destructor.
     * @stable ICU 2.0
     */
    virtual ~MessageFormat();

    /**
     * Clones this Format object polymorphically.  The caller owns the
     * result and should delete it when done.
     * @stable ICU 2.0
     */
    virtual Format* clone(void) const;

    /**
     * Returns true if the given Format objects are semantically equal.
     * Objects of different subclasses are considered unequal.
     * @param other  the object to be compared with.
     * @return       true if the given Format objects are semantically equal.
     * @stable ICU 2.0
     */
    virtual UBool operator==(const Format& other) const;

    /**
     * Sets the locale. This locale is used for fetching default number or date
     * format information.
     * @param theLocale    the new locale value to be set.
     * @stable ICU 2.0
     */
    virtual void setLocale(const Locale& theLocale);

    /**
     * Gets the locale. This locale is used for fetching default number or date
     * format information.
     * @return    the locale of the object.
     * @stable ICU 2.0
     */
    virtual const Locale& getLocale(void) const;

    /**
     * Applies the given pattern string to this message format.
     *
     * @param pattern   The pattern to be applied.
     * @param status    Input/output error code.  If the
     *                  pattern cannot be parsed, set to failure code.
     * @stable ICU 2.0
     */
    virtual void applyPattern(const UnicodeString& pattern,
                              UErrorCode& status);
    /**
     * Applies the given pattern string to this message format.
     *
     * @param pattern    The pattern to be applied.
     * @param parseError Struct to recieve information on position 
     *                   of error within pattern.
     * @param status    Input/output error code.  If the
     *                  pattern cannot be parsed, set to failure code.
     * @stable ICU 2.0
     */
    virtual void applyPattern(const UnicodeString& pattern,
                             UParseError& parseError,
                             UErrorCode& status);

    /**
     * Returns a pattern that can be used to recreate this object.
     *
     * @param appendTo  Output parameter to receive the pattern.
     *                  Result is appended to existing contents.
     * @return          Reference to 'appendTo' parameter.
     * @stable ICU 2.0
     */
    virtual UnicodeString& toPattern(UnicodeString& appendTo) const;

    /**
     * Sets subformats.
     * See the class description about format numbering.
     * The caller should not delete the Format objects after this call.
     * <EM>The array formatsToAdopt is not itself adopted.</EM> Its
     * ownership is retained by the caller. If the call fails because
     * memory cannot be allocated, then the formats will be deleted
     * by this method, and this object will remain unchanged.
     * 
     * @stable ICU 2.0
     * @param formatsToAdopt    the format to be adopted.
     * @param count             the size of the array.
     */
    virtual void adoptFormats(Format** formatsToAdopt, int32_t count);

    /**
     * Sets subformats.
     * See the class description about format numbering.
     * Each item in the array is cloned into the internal array.
     * If the call fails because memory cannot be allocated, then this
     * object will remain unchanged.
     * 
     * @stable ICU 2.0
     * @param newFormats the new format to be set.
     * @param cnt        the size of the array.
     */
    virtual void setFormats(const Format** newFormats,int32_t cnt);


    /**
     * Sets one subformat.
     * See the class description about format numbering.
     * The caller should not delete the Format object after this call.
     * If the number is over the number of formats already set,
     * the item will be deleted and ignored.
     * @stable ICU 2.0
     * @param formatNumber     index of the subformat.
     * @param formatToAdopt    the format to be adopted.
     */
    virtual void adoptFormat(int32_t formatNumber, Format* formatToAdopt);

    /**
     * Sets one subformat.
     * See the class description about format numbering.
     * If the number is over the number of formats already set,
     * the item will be ignored.
     * @param formatNumber     index of the subformat.
     * @param format    the format to be set.
     * @stable ICU 2.0
     */
    virtual void setFormat(int32_t formatNumber, const Format& format);

    /**
     * Gets an array of subformats of this object.  The returned array
     * should not be deleted by the caller, nor should the pointers
     * within the array.  The array and its contents remain valid only
     * until the next call to any method of this class is made with
     * this object.  See the class description about format numbering.
     * @param count output parameter to receive the size of the array
     * @return an array of count Format* objects, or NULL if out of
     * memory.  Any or all of the array elements may be NULL.
     * @stable ICU 2.0
     */
    virtual const Format** getFormats(int32_t& count) const;

    /**
     * Formats the given array of arguments into a user-readable string.
     * Does not take ownership of the Formattable* array or its contents.
     *
     * @param source    An array of objects to be formatted.
     * @param count     The number of elements of 'source'.
     * @param appendTo  Output parameter to receive result.
     *                  Result is appended to existing contents.
     * @param ignore    Not used; inherited from base class API.
     * @param status    Input/output error code.  If the
     *                  pattern cannot be parsed, set to failure code.
     * @return          Reference to 'appendTo' parameter.
     * @stable ICU 2.0
     */
    UnicodeString& format(  const Formattable* source,
                            int32_t count,
                            UnicodeString& appendTo,
                            FieldPosition& ignore,
                            UErrorCode& status) const;

    /**
     * Formats the given array of arguments into a user-readable string
     * using the given pattern.
     *
     * @param pattern   The pattern.
     * @param arguments An array of objects to be formatted.
     * @param count     The number of elements of 'source'.
     * @param appendTo  Output parameter to receive result.
     *                  Result is appended to existing contents.
     * @param status    Input/output error code.  If the
     *                  pattern cannot be parsed, set to failure code.
     * @return          Reference to 'appendTo' parameter.
     * @stable ICU 2.0
     */
    static UnicodeString& format(   const UnicodeString& pattern,
                                    const Formattable* arguments,
                                    int32_t count,
                                    UnicodeString& appendTo,
                                    UErrorCode& status);

    /**
     * Formats the given array of arguments into a user-readable
     * string.  The array must be stored within a single Formattable
     * object of type kArray. If the Formattable object type is not of
     * type kArray, then returns a failing UErrorCode.
     *
     * @param obj       A Formattable of type kArray containing
     *                  arguments to be formatted.
     * @param appendTo  Output parameter to receive result.
     *                  Result is appended to existing contents.
     * @param pos       On input: an alignment field, if desired.
     *                  On output: the offsets of the alignment field.
     * @param status    Input/output error code.  If the
     *                  pattern cannot be parsed, set to failure code.
     * @return          Reference to 'appendTo' parameter.
     * @stable ICU 2.0
     */
    virtual UnicodeString& format(const Formattable& obj,
                                  UnicodeString& appendTo,
                                  FieldPosition& pos,
                                  UErrorCode& status) const;

    /**
     * Formats the given array of arguments into a user-readable
     * string.  The array must be stored within a single Formattable
     * object of type kArray. If the Formattable object type is not of
     * type kArray, then returns a failing UErrorCode.
     *
     * @param obj       The object to format
     * @param appendTo  Output parameter to receive result.
     *                  Result is appended to existing contents.
     * @param status    Input/output error code.  If the
     *                  pattern cannot be parsed, set to failure code.
     * @return          Reference to 'appendTo' parameter.
     * @stable ICU 2.0
     */
    UnicodeString& format(const Formattable& obj,
                          UnicodeString& appendTo,
                          UErrorCode& status) const;

    /**
     * Parses the given string into an array of output arguments.
     *
     * @param source    String to be parsed.
     * @param pos       On input, starting position for parse. On output,
     *                  final position after parse.  Unchanged if parse
     *                  fails.
     * @param count     Output parameter to receive the number of arguments
     *                  parsed.
     * @return an array of parsed arguments.  The caller owns both
     * the array and its contents.
     * @stable ICU 2.0
     */
    virtual Formattable* parse( const UnicodeString& source,
                                ParsePosition& pos,
                                int32_t& count) const;

    /**
     * Parses the given string into an array of output arguments.
     *
     * @param source    String to be parsed.
     * @param count     Output param to receive size of returned array.
     * @param status    Input/output error code.  If the
     *                  pattern cannot be parsed, set to failure code.
     * @return an array of parsed arguments.  The caller owns both
     * the array and its contents.
     * @stable ICU 2.0
     */
    virtual Formattable* parse( const UnicodeString& source,
                                int32_t& count,
                                UErrorCode& status) const;

    /**
     * Parses the given string into an array of output arguments
     * stored within a single Formattable of type kArray.
     *
     * @param source    The string to be parsed into an object.
     * @param result    Formattable to be set to the parse result.
     *                  If parse fails, return contents are undefined.
     * @param pos       On input, starting position for parse. On output,
     *                  final position after parse.  Unchanged if parse
     *                  fails.
     * @stable ICU 2.0
     */
    virtual void parseObject(const UnicodeString& source,
                             Formattable& result,
                             ParsePosition& pos) const;

    /**
     * Convert an 'apostrophe-friendly' pattern into a standard
     * pattern.  Standard patterns treat all apostrophes as
     * quotes, which is problematic in some languages, e.g. 
     * French, where apostrophe is commonly used.  This utility
     * assumes that only an unpaired apostrophe immediately before
     * a brace is a true quote.  Other unpaired apostrophes are paired,
     * and the resulting standard pattern string is returned.
     *
     * <p><b>Note</b> it is not guaranteed that the returned pattern
     * is indeed a valid pattern.  The only effect is to convert
     * between patterns having different quoting semantics.
     *
     * @param pattern the 'apostrophe-friendly' patttern to convert
     * @param status    Input/output error code.  If the pattern
     *                  cannot be parsed, the failure code is set.
     * @return the standard equivalent of the original pattern
     * @stable ICU 3.4
     */
    static UnicodeString autoQuoteApostrophe(const UnicodeString& pattern, 
        UErrorCode& status);

    /**
     * Returns a unique class ID POLYMORPHICALLY.  Pure virtual override.
     * This method is to implement a simple version of RTTI, since not all
     * C++ compilers support genuine RTTI.  Polymorphic operator==() and
     * clone() methods call this method.
     *
     * @return          The class ID for this object. All objects of a
     *                  given class have the same class ID.  Objects of
     *                  other classes have different class IDs.
     * @stable ICU 2.0
     */
    virtual UClassID getDynamicClassID(void) const;

    /**
     * Return the class ID for this class.  This is useful only for
     * comparing to a return value from getDynamicClassID().  For example:
     * <pre>
     * .   Base* polymorphic_pointer = createPolymorphicObject();
     * .   if (polymorphic_pointer->getDynamicClassID() ==
     * .      Derived::getStaticClassID()) ...
     * </pre>
     * @return          The class ID for all objects of this class.
     * @stable ICU 2.0
     */
    static UClassID U_EXPORT2 getStaticClassID(void);
    
private:

    Locale              fLocale;
    UnicodeString       fPattern;
    Format**            formatAliases; // see getFormats
    int32_t             formatAliasesCapacity;

    MessageFormat(); // default constructor not implemented

    /*
     * A structure representing one subformat of this MessageFormat.
     * Each subformat has a Format object, an offset into the plain
     * pattern text fPattern, and an argument number.  The argument
     * number corresponds to the array of arguments to be formatted.
     * @internal
     */
    class Subformat {
    public:
        /**
         * @internal 
         */
        Format* format; // formatter
        /**
         * @internal 
         */
        int32_t offset; // offset into fPattern
        /**
         * @internal 
         */
        int32_t arg;    // 0-based argument number

        /**
         * Clone that.format and assign it to this.format
         * Do NOT delete this.format
         * @internal
         */
        Subformat& operator=(const Subformat& that) {
            format = that.format ? that.format->clone() : NULL;
            offset = that.offset;
            arg = that.arg;
            return *this;
        }

        /**
         * @internal 
         */
        UBool operator==(const Subformat& that) const {
            // Do cheap comparisons first
            return offset == that.offset &&
                   arg == that.arg &&
                   ((format == that.format) || // handles NULL
                    (*format == *that.format));
        }

        /**
         * @internal
         */
        UBool operator!=(const Subformat& that) const {
            return !operator==(that);
        }
    };

    /**
     * A MessageFormat contains an array of subformats.  This array
     * needs to grow dynamically if the MessageFormat is modified.
     */
    Subformat* subformats;
    int32_t    subformatCount;
    int32_t    subformatCapacity;

    /**
     * A MessageFormat formats an array of arguments.  Each argument
     * has an expected type, based on the pattern.  For example, if
     * the pattern contains the subformat "{3,number,integer}", then
     * we expect argument 3 to have type Formattable::kLong.  This
     * array needs to grow dynamically if the MessageFormat is
     * modified.
     */
    Formattable::Type* argTypes;
    int32_t            argTypeCount;
    int32_t            argTypeCapacity;

    // Variable-size array management
    UBool allocateSubformats(int32_t capacity);
    UBool allocateArgTypes(int32_t capacity);

    /**
     * Default Format objects used when no format is specified and a
     * numeric or date argument is formatted.  These are volatile
     * cache objects maintained only for performance.  They do not
     * participate in operator=(), copy constructor(), nor
     * operator==().
     */
    NumberFormat* defaultNumberFormat;
    DateFormat*   defaultDateFormat;

    /**
     * Method to retrieve default formats (or NULL on failure).
     * These are semantically const, but may modify *this.
     */
    const NumberFormat* getDefaultNumberFormat(UErrorCode&) const;
    const DateFormat*   getDefaultDateFormat(UErrorCode&) const;

    /**
     * Finds the word s, in the keyword list and returns the located index.
     * @param s the keyword to be searched for.
     * @param list the list of keywords to be searched with.
     * @return the index of the list which matches the keyword s.
     */
    static int32_t findKeyword( const UnicodeString& s,
                                const UChar * const *list);

    /**
     * Formats the array of arguments and copies the result into the
     * result buffer, updates the field position.
     *
     * @param arguments The formattable objects array.
     * @param cnt       The array count.
     * @param appendTo  Output parameter to receive result.
     *                  Result is appended to existing contents.
     * @param status    Field position status.
     * @param recursionProtection
     *                  Initially zero. Bits 0..9 are used to indicate
     *                  that a parameter has already been seen, to
     *                  avoid recursion.  Currently unused.
     * @param success   The error code status.
     * @return          Reference to 'appendTo' parameter.
     */
    UnicodeString&  format( const Formattable* arguments,
                            int32_t cnt,
                            UnicodeString& appendTo,
                            FieldPosition& status,
                            int32_t recursionProtection,
                            UErrorCode& success) const;

    void             makeFormat(int32_t offsetNumber,
                                UnicodeString* segments,
                                UParseError& parseError,
                                UErrorCode& success);

    /**
     * Convenience method that ought to be in NumberFormat
     */
    NumberFormat* createIntegerFormat(const Locale& locale, UErrorCode& status) const;

    /**
     * Checks the range of the source text to quote the special
     * characters, { and ' and copy to target buffer.
     * @param source
     * @param start the text offset to start the process of in the source string
     * @param end the text offset to end the process of in the source string
     * @param appendTo  Output parameter to receive result.
     *                  Result is appended to existing contents.
     */
    static void copyAndFixQuotes(const UnicodeString& appendTo, int32_t start, int32_t end, UnicodeString& target);

    /**
     * Returns array of argument types in the parsed pattern 
     * for use in C API.  Only for the use of umsg_vformat().  Not
     * for public consumption.
     * @param listCount  Output parameter to receive the size of array
     * @return           The array of formattable types in the pattern
     * @internal
     */
    const Formattable::Type* getArgTypeList(int32_t& listCount) const {
        listCount = argTypeCount;
        return argTypes; 
    }

    friend class MessageFormatAdapter; // getFormatTypeList() access
};

inline UnicodeString&
MessageFormat::format(const Formattable& obj,
                      UnicodeString& appendTo,
                      UErrorCode& status) const {
    return Format::format(obj, appendTo, status);
}
U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif // _MSGFMT
//eof

