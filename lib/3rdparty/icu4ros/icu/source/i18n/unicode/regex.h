/*
**********************************************************************
*   Copyright (C) 2002-2007, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   file name:  regex.h
*   encoding:   US-ASCII
*   indentation:4
*
*   created on: 2002oct22
*   created by: Andy Heninger
*
*   ICU Regular Expressions, API for C++
*/

#ifndef REGEX_H
#define REGEX_H

//#define REGEX_DEBUG

/**
 * \file
 * \brief  C++ API:  Regular Expressions
 *
 * <h2>Regular Expression API</h2>
 *
 * <p>The ICU API for processing regular expressions consists of two classes,
 *  <code>RegexPattern</code> and <code>RegexMatcher</code>.
 *  <code>RegexPattern</code> objects represent a pre-processed, or compiled
 *  regular expression.  They are created from a regular expression pattern string,
 *  and can be used to create <code>RegexMatcher</code> objects for the pattern.</p>
 *
 * <p>Class <code>RegexMatcher</code> bundles together a regular expression
 *  pattern and a target string to which the search pattern will be applied.
 *  <code>RegexMatcher</code> includes API for doing plain find or search
 *  operations, for search and replace operations, and for obtaining detailed
 *  information about bounds of a match. </p>
 *
 * <p>Note that by constructing <code>RegexMatcher</code> objects directly from regular 
 * expression pattern strings application code can be simplified and the explicit
 * need for <code>RegexPattern</code> objects can usually be eliminated.
 * </p>
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_REGULAR_EXPRESSIONS

#include "unicode/uobject.h"
#include "unicode/unistr.h"
#include "unicode/parseerr.h"

#include "unicode/uregex.h"

U_NAMESPACE_BEGIN


// Forward Declarations...

class RegexMatcher;
class RegexPattern;
class UVector;
class UVector32;
class UnicodeSet;
struct REStackFrame;
struct Regex8BitSet;
class  RuleBasedBreakIterator;
class  RegexCImpl;




/**
 *   RBBIPatternDump   Debug function, displays the compiled form of a pattern.
 *   @internal
 */
#ifdef REGEX_DEBUG
U_INTERNAL void U_EXPORT2
    RegexPatternDump(const RegexPattern *pat);
#else
    #define RegexPatternDump(pat)
#endif



/**
  * Class <code>RegexPattern</code> represents a compiled regular expression.  It includes
  * factory methods for creating a RegexPattern object from the source (string) form
  * of a regular expression, methods for creating RegexMatchers that allow the pattern
  * to be applied to input text, and a few convenience methods for simple common
  * uses of regular expressions.
  *
  * <p>Class RegexPattern is not intended to be subclassed.</p>
  *
  * @stable ICU 2.4
  */
class U_I18N_API RegexPattern: public UObject {
public:

    /**
     * default constructor.  Create a RegexPattern object that refers to no actual
     *   pattern.  Not normally needed; RegexPattern objects are usually
     *   created using the factory method <code>compile()</code>.
     *
     * @stable ICU 2.4
     */
    RegexPattern();

    /**
     * Copy Constructor.  Create a new RegexPattern object that is equivalent
     *                    to the source object.
     * @param source the pattern object to be copied.
     * @stable ICU 2.4
     */
    RegexPattern(const RegexPattern &source);

    /**
     * Destructor.  Note that a RegexPattern object must persist so long as any
     *  RegexMatcher objects that were created from the RegexPattern are active.
     * @stable ICU 2.4
     */
    virtual ~RegexPattern();

    /**
     * Comparison operator.  Two RegexPattern objects are considered equal if they
     * were constructed from identical source patterns using the same match flag
     * settings.
     * @param that a RegexPattern object to compare with "this".
     * @return TRUE if the objects are equivalent.
     * @stable ICU 2.4
     */
    UBool           operator==(const RegexPattern& that) const;

    /**
     * Comparison operator.  Two RegexPattern objects are considered equal if they
     * were constructed from identical source patterns using the same match flag
     * settings.
     * @param that a RegexPattern object to compare with "this".
     * @return TRUE if the objects are different.
     * @stable ICU 2.4
     */
    inline UBool    operator!=(const RegexPattern& that) const {return ! operator ==(that);};

    /**
     * Assignment operator.  After assignment, this RegexPattern will behave identically
     *     to the source object.
     * @stable ICU 2.4
     */
    RegexPattern  &operator =(const RegexPattern &source);

    /**
     * Create an exact copy of this RegexPattern object.  Since RegexPattern is not
     * intended to be subclasses, <code>clone()</code> and the copy construction are
     * equivalent operations.
     * @return the copy of this RegexPattern
     * @stable ICU 2.4
     */
    virtual RegexPattern  *clone() const;


   /**
    * Compiles the regular expression in string form into a RegexPattern
    * object.  These compile methods, rather than the constructors, are the usual
    * way that RegexPattern objects are created.
    *
    * <p>Note that RegexPattern objects must not be deleted while RegexMatcher
    * objects created from the pattern are active.  RegexMatchers keep a pointer
    * back to their pattern, so premature deletion of the pattern is a
    * catastrophic error.</p>
    *
    * <p>All pattern match mode flags are set to their default values.</p>
    *
    * <p>Note that it is often more convenient to construct a RegexMatcher directly
    *    from a pattern string rather than separately compiling the pattern and
    *    then creating a RegexMatcher object from the pattern.</p>
    *
    * @param regex The regular expression to be compiled.
    * @param pe    Receives the position (line and column nubers) of any error
    *              within the regular expression.)
    * @param status A reference to a UErrorCode to receive any errors.
    * @return      A regexPattern object for the compiled pattern.
    *
    * @stable ICU 2.4
    */
    static RegexPattern * U_EXPORT2 compile( const UnicodeString &regex,
        UParseError          &pe,
        UErrorCode           &status);

   /**
    * Compiles the regular expression in string form into a RegexPattern
    * object using the specified match mode flags.  These compile methods,
    * rather than the constructors, are the usual way that RegexPattern objects
    * are created.
    *
    * <p>Note that RegexPattern objects must not be deleted while RegexMatcher
    * objects created from the pattern are active.  RegexMatchers keep a pointer
    * back to their pattern, so premature deletion of the pattern is a
    * catastrophic error.</p>
    *
    * <p>Note that it is often more convenient to construct a RegexMatcher directly
    *    from a pattern string instead of than separately compiling the pattern and
    *    then creating a RegexMatcher object from the pattern.</p>
    *
    * @param regex The regular expression to be compiled.
    * @param flags The match mode flags to be used.
    * @param pe    Receives the position (line and column nubers) of any error
    *              within the regular expression.)
    * @param status   A reference to a UErrorCode to receive any errors.
    * @return      A regexPattern object for the compiled pattern.
    *
    * @stable ICU 2.4
    */
    static RegexPattern * U_EXPORT2 compile( const UnicodeString &regex,
        uint32_t             flags,
        UParseError          &pe,
        UErrorCode           &status);


   /**
    * Compiles the regular expression in string form into a RegexPattern
    * object using the specified match mode flags.  These compile methods,
    * rather than the constructors, are the usual way that RegexPattern objects
    * are created.
    *
    * <p>Note that RegexPattern objects must not be deleted while RegexMatcher
    * objects created from the pattern are active.  RegexMatchers keep a pointer
    * back to their pattern, so premature deletion of the pattern is a
    * catastrophic error.</p>
    *
    * <p>Note that it is often more convenient to construct a RegexMatcher directly
    *    from a pattern string instead of than separately compiling the pattern and
    *    then creating a RegexMatcher object from the pattern.</p>
    *
    * @param regex The regular expression to be compiled.
    * @param flags The match mode flags to be used.
    * @param status   A reference to a UErrorCode to receive any errors.
    * @return      A regexPattern object for the compiled pattern.
    *
    * @stable ICU 2.6
    */
    static RegexPattern * U_EXPORT2 compile( const UnicodeString &regex,
        uint32_t             flags,
        UErrorCode           &status);


   /**
    * Get the match mode flags that were used when compiling this pattern.
    * @return  the match mode flags
    * @stable ICU 2.4
    */
    virtual uint32_t flags() const;

   /**
    * Creates a RegexMatcher that will match the given input against this pattern.  The
    * RegexMatcher can then be used to perform match, find or replace operations
    * on the input.  Note that a RegexPattern object must not be deleted while
    * RegexMatchers created from it still exist and might possibly be used again.
    * <p>
    * The matcher will retain a reference to the supplied input string, and all regexp
    * pattern matching operations happen directly on this original string.  It is
    * critical that the string not be altered or deleted before use by the regular
    * expression operations is complete.
    *
    * @param input    The input string to which the regular expression will be applied.
    * @param status   A reference to a UErrorCode to receive any errors.
    * @return         A RegexMatcher object for this pattern and input.
    *
    * @stable ICU 2.4
    */
    virtual RegexMatcher *matcher(const UnicodeString &input,
        UErrorCode          &status) const;

private:
    /**
     * Cause a compilation error if an application accidently attempts to
     *   create a matcher with a (UChar *) string as input rather than
     *   a UnicodeString.  Avoids a dangling reference to a temporary string.
     * <p>
     * To efficiently work with UChar *strings, wrap the data in a UnicodeString
     * using one of the aliasing constructors, such as
     * <code>UnicodeString(UBool isTerminated, const UChar *text, int32_t textLength);</code>
     *
     * @internal
     */
    RegexMatcher *matcher(const UChar *input,
        UErrorCode          &status) const;
public:


   /**
    * Creates a RegexMatcher that will match against this pattern.  The
    * RegexMatcher can be used to perform match, find or replace operations.
    * Note that a RegexPattern object must not be deleted while
    * RegexMatchers created from it still exist and might possibly be used again.
    *
    * @param status   A reference to a UErrorCode to receive any errors.
    * @return      A RegexMatcher object for this pattern and input.
    *
    * @stable ICU 2.6
    */
    virtual RegexMatcher *matcher(UErrorCode  &status) const;


   /**
    * Test whether a string matches a regular expression.  This convenience function
    * both compiles the reguluar expression and applies it in a single operation.
    * Note that if the same pattern needs to be applied repeatedly, this method will be
    * less efficient than creating and reusing a RegexMatcher object.
    *
    * @param regex The regular expression
    * @param input The string data to be matched
    * @param pe Receives the position of any syntax errors within the regular expression
    * @param status A reference to a UErrorCode to receive any errors.
    * @return True if the regular expression exactly matches the full input string.
    *
    * @stable ICU 2.4
    */
    static UBool U_EXPORT2 matches(const UnicodeString   &regex,
        const UnicodeString   &input,
        UParseError     &pe,
        UErrorCode      &status);


   /**
    *    Returns the regular expression from which this pattern was compiled.
    *    @stable ICU 2.4
    */
    virtual UnicodeString pattern() const;


    /**
     * Split a string into fields.  Somewhat like split() from Perl.
     * The pattern matches identify delimiters that separate the input
     *  into fields.  The input data between the matches becomes the
     *  fields themselves.
     * <p>
     *  For the best performance on split() operations,
     *  <code>RegexMatcher::split</code> is perferable to this function
     *
     * @param input   The string to be split into fields.  The field delimiters
     *                match the pattern (in the "this" object)
     * @param dest    An array of UnicodeStrings to receive the results of the split.
     *                This is an array of actual UnicodeString objects, not an
     *                array of pointers to strings.  Local (stack based) arrays can
     *                work well here.
     * @param destCapacity  The number of elements in the destination array.
     *                If the number of fields found is less than destCapacity, the
     *                extra strings in the destination array are not altered.
     *                If the number of destination strings is less than the number
     *                of fields, the trailing part of the input string, including any
     *                field delimiters, is placed in the last destination string.
     * @param status  A reference to a UErrorCode to receive any errors.
     * @return        The number of fields into which the input string was split.
     * @stable ICU 2.4
     */
    virtual int32_t  split(const UnicodeString &input,
        UnicodeString    dest[],
        int32_t          destCapacity,
        UErrorCode       &status) const;


    /**
     * ICU "poor man's RTTI", returns a UClassID for the actual class.
     *
     * @stable ICU 2.4
     */
    virtual UClassID getDynamicClassID() const;

    /**
     * ICU "poor man's RTTI", returns a UClassID for this class.
     *
     * @stable ICU 2.4
     */
    static UClassID U_EXPORT2 getStaticClassID();

private:
    //
    //  Implementation Data
    //
    UnicodeString   fPattern;      // The original pattern string.
    uint32_t        fFlags;        // The flags used when compiling the pattern.
                                   //
    UVector32       *fCompiledPat; // The compiled pattern p-code.
    UnicodeString   fLiteralText;  // Any literal string data from the pattern,
                                   //   after un-escaping, for use during the match.

    UVector         *fSets;        // Any UnicodeSets referenced from the pattern.
    Regex8BitSet    *fSets8;       //      (and fast sets for latin-1 range.)


    UErrorCode      fDeferredStatus; // status if some prior error has left this
                                   //  RegexPattern in an unusable state.

    int32_t         fMinMatchLen;  // Minimum Match Length.  All matches will have length
                                   //   >= this value.  For some patterns, this calculated
                                   //   value may be less than the true shortest
                                   //   possible match.

    int32_t         fFrameSize;    // Size of a state stack frame in the
                                   //   execution engine.

    int32_t         fDataSize;     // The size of the data needed by the pattern that
                                   //   does not go on the state stack, but has just
                                   //   a single copy per matcher.

    UVector32       *fGroupMap;    // Map from capture group number to position of
                                   //   the group's variables in the matcher stack frame.

    int32_t         fMaxCaptureDigits;

    UnicodeSet     **fStaticSets;  // Ptr to static (shared) sets for predefined
                                   //   regex character classes, e.g. Word.

    Regex8BitSet   *fStaticSets8;  // Ptr to the static (shared) latin-1 only
                                   //  sets for predefined regex classes.

    int32_t         fStartType;    // Info on how a match must start.
    int32_t         fInitialStringIdx;     //
    int32_t         fInitialStringLen;
    UnicodeSet     *fInitialChars;
    UChar32         fInitialChar;
    Regex8BitSet   *fInitialChars8;

    friend class RegexCompile;
    friend class RegexMatcher;
    friend class RegexCImpl;

    //
    //  Implementation Methods
    //
    void        init();            // Common initialization, for use by constructors.
    void        zap();             // Common cleanup
#ifdef REGEX_DEBUG
    void        dumpOp(int32_t index) const;
    friend     void U_EXPORT2 RegexPatternDump(const RegexPattern *);
#endif

};



/**
 *  class RegexMatcher bundles together a reular expression pattern and
 *  input text to which the expression can be applied.  It includes methods
 *  for testing for matches, and for find and replace operations.
 *
 * <p>Class RegexMatcher is not intended to be subclassed.</p>
 *
 * @stable ICU 2.4
 */
class U_I18N_API RegexMatcher: public UObject {
public:

    /**
      * Construct a RegexMatcher for a regular expression.
      * This is a convenience method that avoids the need to explicitly create
      * a RegexPattern object.  Note that if several RegexMatchers need to be
      * created for the same expression, it will be more efficient to
      * separately create and cache a RegexPattern object, and use
      * its matcher() method to create the RegexMatcher objects.
      *
      *  @param regexp The Regular Expression to be compiled.
      *  @param flags  Regular expression options, such as case insensitive matching.
      *                @see UREGEX_CASE_INSENSITIVE
      *  @param status Any errors are reported by setting this UErrorCode variable.
      *  @stable ICU 2.6
      */
    RegexMatcher(const UnicodeString &regexp, uint32_t flags, UErrorCode &status);

    /**
      * Construct a RegexMatcher for a regular expression.
      * This is a convenience method that avoids the need to explicitly create
      * a RegexPattern object.  Note that if several RegexMatchers need to be
      * created for the same expression, it will be more efficient to
      * separately create and cache a RegexPattern object, and use
      * its matcher() method to create the RegexMatcher objects.
      * <p>
      * The matcher will retain a reference to the supplied input string, and all regexp
      * pattern matching operations happen directly on the original string.  It is
      * critical that the string not be altered or deleted before use by the regular
      * expression operations is complete.
      *
      *  @param regexp The Regular Expression to be compiled.  
      *  @param input  The string to match.  The matcher retains a reference to the
      *                caller's string; mo copy is made.
      *  @param flags  Regular expression options, such as case insensitive matching.
      *                @see UREGEX_CASE_INSENSITIVE
      *  @param status Any errors are reported by setting this UErrorCode variable.
      *  @stable ICU 2.6
      */
    RegexMatcher(const UnicodeString &regexp, const UnicodeString &input,
        uint32_t flags, UErrorCode &status);

private:
    /**
     * Cause a compilation error if an application accidently attempts to
     *   create a matcher with a (UChar *) string as input rather than
     *   a UnicodeString.    Avoids a dangling reference to a temporary string.
     * <p>
     * To efficiently work with UChar *strings, wrap the data in a UnicodeString
     * using one of the aliasing constructors, such as
     * <code>UnicodeString(UBool isTerminated, const UChar *text, int32_t textLength);</code>
     *
     * @internal
     */
    RegexMatcher(const UnicodeString &regexp, const UChar *input,
        uint32_t flags, UErrorCode &status);
public:


   /**
    *   Destructor.
    *
    *  @stable ICU 2.4
    */
    virtual ~RegexMatcher();


   /**
    *   Attempts to match the entire input string against the pattern.
    *    @param   status     A reference to a UErrorCode to receive any errors.
    *    @return TRUE if there is a match
    *    @stable ICU 2.4
    */
    virtual UBool matches(UErrorCode &status);

   /**
    *   Attempts to match the input string, beginning at startIndex, against the pattern.
    *   The match must extend to the end of the input string.
    *    @param   startIndex The input string index at which to begin matching.
    *    @param   status     A reference to a UErrorCode to receive any errors.
    *    @return TRUE if there is a match
    *    @stable ICU 2.8
    */
    virtual UBool matches(int32_t startIndex, UErrorCode &status);




   /**
    *   Attempts to match the input string, starting from the beginning, against the pattern.
    *   Like the matches() method, this function always starts at the beginning of the input string;
    *   unlike that function, it does not require that the entire input string be matched.
    *
    *   <p>If the match succeeds then more information can be obtained via the <code>start()</code>,
    *     <code>end()</code>, and <code>group()</code> functions.</p>
    *
    *    @param   status     A reference to a UErrorCode to receive any errors.
    *    @return  TRUE if there is a match at the start of the input string.
    *    @stable ICU 2.4
    */
    virtual UBool lookingAt(UErrorCode &status);


  /**
    *   Attempts to match the input string, starting from the specified index, against the pattern.
    *   The match may be of any length, and is not required to extend to the end
    *   of the input string.  Contrast with match().
    *
    *   <p>If the match succeeds then more information can be obtained via the <code>start()</code>,
    *     <code>end()</code>, and <code>group()</code> functions.</p>
    *
    *    @param   startIndex The input string index at which to begin matching.
    *    @param   status     A reference to a UErrorCode to receive any errors.
    *    @return  TRUE if there is a match.
    *    @stable ICU 2.8
    */
    virtual UBool lookingAt(int32_t startIndex, UErrorCode &status);

   /**
    *  Find the next pattern match in the input string.
    *  The find begins searching the input at the location following the end of
    *  the previous match, or at the start of the string if there is no previous match.
    *  If a match is found, <code>start(), end()</code> and <code>group()</code>
    *  will provide more information regarding the match.
    *  <p>Note that if the input string is changed by the application,
    *     use find(startPos, status) instead of find(), because the saved starting
    *     position may not be valid with the altered input string.</p>
    *  @return  TRUE if a match is found.
    *  @stable ICU 2.4
    */
    virtual UBool find();


   /**
    *   Resets this RegexMatcher and then attempts to find the next substring of the
    *   input string that matches the pattern, starting at the specified index.
    *
    *   @param   start     the position in the input string to begin the search
    *   @param   status    A reference to a UErrorCode to receive any errors.
    *   @return  TRUE if a match is found.
    *   @stable ICU 2.4
    */
    virtual UBool find(int32_t start, UErrorCode &status);


   /**
    *   Returns a string containing the text matched by the previous match.
    *   If the pattern can match an empty string, an empty string may be returned.
    *   @param   status      A reference to a UErrorCode to receive any errors.
    *                        Possible errors are  U_REGEX_INVALID_STATE if no match
    *                        has been attempted or the last match failed.
    *   @return  a string containing the matched input text.
    *   @stable ICU 2.4
    */
    virtual UnicodeString group(UErrorCode &status) const;


   /**
    *    Returns a string containing the text captured by the given group
    *    during the previous match operation.  Group(0) is the entire match.
    *
    *    @param groupNum the capture group number
    *    @param   status     A reference to a UErrorCode to receive any errors.
    *                        Possible errors are  U_REGEX_INVALID_STATE if no match
    *                        has been attempted or the last match failed and
    *                        U_INDEX_OUTOFBOUNDS_ERROR for a bad capture group number.
    *    @return the captured text
    *    @stable ICU 2.4
    */
    virtual UnicodeString group(int32_t groupNum, UErrorCode &status) const;


   /**
    *   Returns the number of capturing groups in this matcher's pattern.
    *   @return the number of capture groups
    *   @stable ICU 2.4
    */
    virtual int32_t groupCount() const;


   /**
    *   Returns the index in the input string of the start of the text matched
    *   during the previous match operation.
    *    @param   status      a reference to a UErrorCode to receive any errors.
    *    @return              The position in the input string of the start of the last match.
    *    @stable ICU 2.4
    */
    virtual int32_t start(UErrorCode &status) const;


   /**
    *   Returns the index in the input string of the start of the text matched by the
    *    specified capture group during the previous match operation.  Return -1 if
    *    the capture group exists in the pattern, but was not part of the last match.
    *
    *    @param  group       the capture group number
    *    @param  status      A reference to a UErrorCode to receive any errors.  Possible
    *                        errors are  U_REGEX_INVALID_STATE if no match has been
    *                        attempted or the last match failed, and
    *                        U_INDEX_OUTOFBOUNDS_ERROR for a bad capture group number
    *    @return the start position of substring matched by the specified group.
    *    @stable ICU 2.4
    */
    virtual int32_t start(int32_t group, UErrorCode &status) const;


   /**
    *    Returns the index in the input string of the first character following the
    *    text matched during the previous match operation.
    *   @param   status      A reference to a UErrorCode to receive any errors.  Possible
    *                        errors are  U_REGEX_INVALID_STATE if no match has been
    *                        attempted or the last match failed.
    *    @return the index of the last character matched, plus one.
    *   @stable ICU 2.4
    */
    virtual int32_t end(UErrorCode &status) const;


   /**
    *    Returns the index in the input string of the character following the
    *    text matched by the specified capture group during the previous match operation.
    *    @param group  the capture group number
    *    @param   status      A reference to a UErrorCode to receive any errors.  Possible
    *                        errors are  U_REGEX_INVALID_STATE if no match has been
    *                        attempted or the last match failed and
    *                        U_INDEX_OUTOFBOUNDS_ERROR for a bad capture group number
    *    @return  the index of the first character following the text
    *              captured by the specifed group during the previous match operation.
    *              Return -1 if the capture group exists in the pattern but was not part of the match.
    *    @stable ICU 2.4
    */
    virtual int32_t end(int32_t group, UErrorCode &status) const;


   /**
    *   Resets this matcher.  The effect is to remove any memory of previous matches,
    *       and to cause subsequent find() operations to begin at the beginning of
    *       the input string.
    *
    *   @return this RegexMatcher.
    *   @stable ICU 2.4
    */
    virtual RegexMatcher &reset();


   /**
    *   Resets this matcher, and set the current input position.
    *   The effect is to remove any memory of previous matches,
    *       and to cause subsequent find() operations to begin at
    *       the specified position in the input string.
    *
    *   @return this RegexMatcher.
    *   @stable ICU 2.8
    */
    virtual RegexMatcher &reset(int32_t index, UErrorCode &status);


   /**
    *   Resets this matcher with a new input string.  This allows instances of RegexMatcher
    *     to be reused, which is more efficient than creating a new RegexMatcher for
    *     each input string to be processed.  
    *   @param input The new string on which subsequent pattern matches will operate.
    *                The matcher retains a reference to the callers string, and operates
    *                directly on that.  Ownership of the string remains with the caller.
    *                Because no copy of the string is made, it is essential that the
    *                caller not delete the string until after regexp operations on it
    *                are done.  
    *   @return this RegexMatcher.
    *   @stable ICU 2.4
    */
    virtual RegexMatcher &reset(const UnicodeString &input);

private:
    /**
     * Cause a compilation error if an application accidently attempts to
     *   reset a matcher with a (UChar *) string as input rather than
     *   a UnicodeString.    Avoids a dangling reference to a temporary string.
     * <p>
     * To efficiently work with UChar *strings, wrap the data in a UnicodeString
     * using one of the aliasing constructors, such as
     * <code>UnicodeString(UBool isTerminated, const UChar *text, int32_t textLength);</code>
     *
     * @internal
     */
    RegexMatcher &reset(const UChar *input);
public:

   /**
    *   Returns the input string being matched.  The returned string is not a copy,
    *   but the live input string.  It should not be altered or deleted.
    *   @return the input string
    *   @stable ICU 2.4
    */
    virtual const UnicodeString &input() const;


   /**
    *    Returns the pattern that is interpreted by this matcher.
    *    @return  the RegexPattern for this RegexMatcher
    *    @stable ICU 2.4
    */
    virtual const RegexPattern &pattern() const;


   /**
    *    Replaces every substring of the input that matches the pattern
    *    with the given replacement string.  This is a convenience function that
    *    provides a complete find-and-replace-all operation.
    *
    *    This method first resets this matcher. It then scans the input string
    *    looking for matches of the pattern. Input that is not part of any
    *    match is left unchanged; each match is replaced in the result by the
    *    replacement string. The replacement string may contain references to
    *    capture groups.
    *
    *    @param   replacement a string containing the replacement text.
    *    @param   status      a reference to a UErrorCode to receive any errors.
    *    @return              a string containing the results of the find and replace.
    *    @stable ICU 2.4
    */
    virtual UnicodeString replaceAll(const UnicodeString &replacement, UErrorCode &status);


   /**
    * Replaces the first substring of the input that matches
    * the pattern with the replacement string.   This is a convenience
    * function that provides a complete find-and-replace operation.
    *
    * <p>This function first resets this RegexMatcher. It then scans the input string
    * looking for a match of the pattern. Input that is not part
    * of the match is appended directly to the result string; the match is replaced
    * in the result by the replacement string. The replacement string may contain
    * references to captured groups.</p>
    *
    * <p>The state of the matcher (the position at which a subsequent find()
    *    would begin) after completing a replaceFirst() is not specified.  The
    *    RegexMatcher should be reset before doing additional find() operations.</p>
    *
    *    @param   replacement a string containing the replacement text.
    *    @param   status      a reference to a UErrorCode to receive any errors.
    *    @return              a string containing the results of the find and replace.
    *    @stable ICU 2.4
    */
    virtual UnicodeString replaceFirst(const UnicodeString &replacement, UErrorCode &status);

   /**
    *   Implements a replace operation intended to be used as part of an
    *   incremental find-and-replace.
    *
    *   <p>The input string, starting from the end of the previous replacement and ending at
    *   the start of the current match, is appended to the destination string.  Then the
    *   replacement string is appended to the output string,
    *   including handling any substitutions of captured text.</p>
    *
    *   <p>For simple, prepackaged, non-incremental find-and-replace
    *   operations, see replaceFirst() or replaceAll().</p>
    *
    *   @param   dest        A UnicodeString to which the results of the find-and-replace are appended.
    *   @param   replacement A UnicodeString that provides the text to be substituted for
    *                        the input text that matched the regexp pattern.  The replacement
    *                        text may contain references to captured text from the
    *                        input.
    *   @param   status      A reference to a UErrorCode to receive any errors.  Possible
    *                        errors are  U_REGEX_INVALID_STATE if no match has been
    *                        attempted or the last match failed, and U_INDEX_OUTOFBOUNDS_ERROR
    *                        if the replacement text specifies a capture group that
    *                        does not exist in the pattern.
    *
    *   @return  this  RegexMatcher
    *   @stable ICU 2.4
    *
    */
    virtual RegexMatcher &appendReplacement(UnicodeString &dest,
        const UnicodeString &replacement, UErrorCode &status);


   /**
    * As the final step in a find-and-replace operation, append the remainder
    * of the input string, starting at the position following the last appendReplacement(),
    * to the destination string. <code>appendTail()</code> is intended to be invoked after one
    * or more invocations of the <code>RegexMatcher::appendReplacement()</code>.
    *
    *  @param dest A UnicodeString to which the results of the find-and-replace are appended.
    *  @return  the destination string.
    *  @stable ICU 2.4
    */
    virtual UnicodeString &appendTail(UnicodeString &dest);



    /**
     * Split a string into fields.  Somewhat like split() from Perl.
     * The pattern matches identify delimiters that separate the input
     *  into fields.  The input data between the matches becomes the
     *  fields themselves.
     * <p>
     *
     * @param input   The string to be split into fields.  The field delimiters
     *                match the pattern (in the "this" object).  This matcher
     *                will be reset to this input string.
     * @param dest    An array of UnicodeStrings to receive the results of the split.
     *                This is an array of actual UnicodeString objects, not an
     *                array of pointers to strings.  Local (stack based) arrays can
     *                work well here.
     * @param destCapacity  The number of elements in the destination array.
     *                If the number of fields found is less than destCapacity, the
     *                extra strings in the destination array are not altered.
     *                If the number of destination strings is less than the number
     *                of fields, the trailing part of the input string, including any
     *                field delimiters, is placed in the last destination string.
     * @param status  A reference to a UErrorCode to receive any errors.
     * @return        The number of fields into which the input string was split.
     * @stable ICU 2.6
     */
    virtual int32_t  split(const UnicodeString &input,
        UnicodeString    dest[],
        int32_t          destCapacity,
        UErrorCode       &status);



   /**
     *   setTrace   Debug function, enable/disable tracing of the matching engine.
     *              For internal ICU development use only.  DO NO USE!!!!
     *   @internal
     */
    void setTrace(UBool state);


    /**
    * ICU "poor man's RTTI", returns a UClassID for this class.
    *
    * @stable ICU 2.2
    */
    static UClassID U_EXPORT2 getStaticClassID();

    /**
     * ICU "poor man's RTTI", returns a UClassID for the actual class.
     *
     * @stable ICU 2.2
     */
    virtual UClassID getDynamicClassID() const;

private:
    // Constructors and other object boilerplate are private.
    // Instances of RegexMatcher can not be assigned, copied, cloned, etc.
    RegexMatcher(); // default constructor not implemented
    RegexMatcher(const RegexPattern *pat);
    RegexMatcher(const RegexMatcher &other);
    RegexMatcher &operator =(const RegexMatcher &rhs);
    friend class RegexPattern;
    friend class RegexCImpl;

    //
    //  MatchAt   This is the internal interface to the match engine itself.
    //            Match status comes back in matcher member variables.
    //
    void                 MatchAt(int32_t startIdx, UErrorCode &status);
    inline void          backTrack(int32_t &inputIdx, int32_t &patIdx);
    UBool                isWordBoundary(int32_t pos);         // perform Perl-like  \b test
    UBool                isUWordBoundary(int32_t pos);        // perform RBBI based \b test
    REStackFrame        *resetStack();
    inline REStackFrame *StateSave(REStackFrame *fp, int32_t savePatIdx,
                                   int32_t frameSize, UErrorCode &status);


    const RegexPattern  *fPattern;
    RegexPattern        *fPatternOwned;    // Non-NULL if this matcher owns the pattern, and
                                           //   should delete it when through.
    const UnicodeString *fInput;

    UBool                fMatch;           // True if the last match was successful.
    int32_t              fMatchStart;      // Position of the start of the most recent match
    int32_t              fMatchEnd;        // First position after the end of the most recent match
    int32_t              fLastMatchEnd;    // First position after the end of the previous match,
                                           //   or -1 if there was no previous match.
    int32_t              fLastReplaceEnd;  // First position after the end of the previous appendReplacement();

    UVector32           *fStack;
    REStackFrame        *fFrame;           // After finding a match, the last active stack
                                           //   frame, which will contain the capture group results.
                                           //   NOT valid while match engine is running.

    int32_t             *fData;            // Data area for use by the compiled pattern.
    int32_t             fSmallData[8];     //   Use this for data if it's enough.

    UBool               fTraceDebug;       // Set true for debug tracing of match engine.

    UErrorCode          fDeferredStatus;   // Save error state if that cannot be immediately
                                           //   reported, or that permanently disables this matcher.

    RuleBasedBreakIterator  *fWordBreakItr;


};

U_NAMESPACE_END
#endif  // UCONFIG_NO_REGULAR_EXPRESSIONS
#endif
