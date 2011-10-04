/*
**********************************************************************
*   Copyright (C) 2004-2006, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   file name:  regex.h
*   encoding:   US-ASCII
*   indentation:4
*
*   created on: 2004mar09
*   created by: Andy Heninger
*
*   ICU Regular Expressions, API for C
*/

/**
 * \file
 * \brief C API: Regular Expressions
 *
 * <p>This is a C wrapper around the C++ RegexPattern and RegexMatcher classes.</p>
 */

#ifndef UREGEX_H
#define UREGEX_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_REGULAR_EXPRESSIONS

#include "unicode/parseerr.h"

struct URegularExpression;
/**
  * Structure represeting a compiled regular rexpression, plus the results
  *    of a match operation.
  * @stable ICU 3.0
  */
typedef struct URegularExpression URegularExpression;


/**
 * Constants for Regular Expression Match Modes.
 * @stable ICU 2.4
 */
typedef enum URegexpFlag{

#ifndef U_HIDE_DRAFT_API 
    /** Forces normalization of pattern and strings. 
    Not implemented yet, just a placeholder, hence draft. 
    @draft ICU 2.4 */
    UREGEX_CANON_EQ         = 128,
#endif
    /**  Enable case insensitive matching.  @stable ICU 2.4 */
    UREGEX_CASE_INSENSITIVE = 2,

    /**  Allow white space and comments within patterns  @stable ICU 2.4 */
    UREGEX_COMMENTS         = 4,

    /**  If set, '.' matches line terminators,  otherwise '.' matching stops at line end.
      *  @stable ICU 2.4 */
    UREGEX_DOTALL           = 32,

    /**   Control behavior of "$" and "^"
      *    If set, recognize line terminators within string,
      *    otherwise, match only at start and end of input string.
      *   @stable ICU 2.4 */
    UREGEX_MULTILINE        = 8,

    /**  Unicode word boundaries.
      *     If set, \b uses the Unicode TR 29 definition of word boundaries.
      *     Warning: Unicode word boundaries are quite different from
      *     traditional regular expression word boundaries.  See
      *     http://unicode.org/reports/tr29/#Word_Boundaries
      *     @stable ICU 2.8
      */
    UREGEX_UWORD            = 256
}  URegexpFlag;

/**
  *  Open (compile) an ICU regular expression.  Compiles the regular expression in
  *  string form into an internal representation using the specified match mode flags.
  *  The resulting regular expression handle can then be used to perform various
  *   matching operations.
  *
  * @param pattern        The Regular Expression pattern to be compiled. 
  * @param patternLength  The length of the pattern, or -1 if the pattern is
  *                       NUL termintated.
  * @param flags          Flags that alter the default matching behavior for
  *                       the regular expression, UREGEX_CASE_INSENSITIVE, for
  *                       example.  For default behavior, set this parameter to zero.
  *                       See <code>enum URegexpFlag</code>.  All desired flags
  *                       are bitwise-ORed together.
  * @param pe             Receives the position (line and column nubers) of any syntax
  *                       error within the source regular expression string.  If this
  *                       information is not wanted, pass NULL for this parameter.
  * @param status         Receives error detected by this function.
  * @stable ICU 3.0
  *
  */
U_STABLE URegularExpression * U_EXPORT2
uregex_open( const  UChar          *pattern,
                    int32_t         patternLength,
                    uint32_t        flags,
                    UParseError    *pe,
                    UErrorCode     *status);

/**
  *  Open (compile) an ICU regular expression.  The resulting regular expression
  *   handle can then be used to perform various matching operations.
  *  <p>
  *   This function is the same as uregex_open, except that the pattern
  *   is supplied as an 8 bit char * string in the default code page.
  *
  * @param pattern        The Regular Expression pattern to be compiled, 
  *                       NUL termintated.  
  * @param flags          Flags that alter the default matching behavior for
  *                       the regular expression, UREGEX_CASE_INSENSITIVE, for
  *                       example.  For default behavior, set this parameter to zero.
  *                       See <code>enum URegexpFlag</code>.  All desired flags
  *                       are bitwise-ORed together.
  * @param pe             Receives the position (line and column nubers) of any syntax
  *                       error within the source regular expression string.  If this
  *                       information is not wanted, pass NULL for this parameter.
  * @param status         Receives errors detected by this function.
  * @return               The URegularExpression object representing the compiled
  *                       pattern.
  *
  * @stable ICU 3.0
  */
#if !UCONFIG_NO_CONVERSION
U_STABLE URegularExpression * U_EXPORT2
uregex_openC( const char           *pattern,
                    uint32_t        flags,
                    UParseError    *pe,
                    UErrorCode     *status);
#endif



/**
  *  Close the regular expression, recovering all resources (memory) it
  *   was holding.
  *
  * @param regexp   The regular expression to be closed.
  * @stable ICU 3.0
  */
U_STABLE void U_EXPORT2 
uregex_close(URegularExpression *regexp);

/**
 * Make a copy of a compiled regular expression.  Cloning a regular
 * expression is faster than opening a second instance from the source
 * form of the expression, and requires less memory.
 * <p>
 * Note that the current input string and the position of any matched text
 *  within it are not cloned; only the pattern itself and and the
 *  match mode flags are copied.
 * <p>
 * Cloning can be particularly useful to threaded applications that perform
 * multiple match operations in parallel.  Each concurrent RE
 * operation requires its own instance of a URegularExpression.
 *
 * @param regexp   The compiled regular expression to be cloned.
 * @param status   Receives indication of any errors encountered
 * @return the cloned copy of the compiled regular expression.
 * @stable ICU 3.0
 */
U_STABLE URegularExpression * U_EXPORT2 
uregex_clone(const URegularExpression *regexp, UErrorCode *status);

/**
 *  Return a pointer to the source form of the pattern for this regular expression.
 *
 * @param regexp     The compiled regular expression.
 * @param patLength  This output parameter will be set to the length of the
 *                   pattern string.  A NULL pointer may be used here if the
 *                   pattern length is not needed, as would be the case if
 *                   the pattern is known in advance to be a NUL terminated
 *                   string.
 * @param status     Receives errors detected by this function.
 * @return a pointer to the pattern string.  The storage for the string is
 *                   owned by the regular expression object, and must not be
 *                   altered or deleted by the application.  The returned string
 *                   will remain valid until the regular expression is closed.
 * @stable ICU 3.0
 */
U_STABLE const UChar * U_EXPORT2 
uregex_pattern(const  URegularExpression   *regexp,
                         int32_t           *patLength,
                         UErrorCode        *status);


/**
  * Get the match mode flags that were specified when compiling this regular expression.
  * @param status   Receives errors detected by this function.
  * @param regexp   The compiled regular expression.
  * @return         The match mode flags
  * @see URegexpFlag
  * @stable ICU 3.0
  */
U_STABLE int32_t U_EXPORT2 
uregex_flags(const  URegularExpression   *regexp,
                    UErrorCode           *status);


/**
  *  Set the subject text string upon which the regular expression will look for matches.
  *  This function may be called any number of times, allowing the regular
  *  expression pattern to be applied to different strings.
  *  <p>
  *  Regular expression matching operations work directly on the application's
  *  string data.  No copy is made.  The subject string data must not be
  *  altered after calling this function until after all regular expression
  *  operations involving this string data are completed.  
  *  <p>
  *  Zero length strings are permitted.  In this case, no subsequent match
  *  operation will dereference the text string pointer.
  *
  * @param regexp     The compiled regular expression.
  * @param text       The subject text string.
  * @param textLength The length of the subject text, or -1 if the string
  *                   is NUL terminated.
  * @param status     Receives errors detected by this function.
  * @stable ICU 3.0
  */
U_STABLE void U_EXPORT2 
uregex_setText(URegularExpression *regexp,
               const UChar        *text,
               int32_t             textLength,
               UErrorCode         *status);

/**
  *  Get the subject text that is currently associated with this 
  *   regular expression object.  This simply returns whatever string
  *   pointer was previously supplied via uregex_setText().
  *
  * @param regexp      The compiled regular expression.
  * @param textLength  The length of the string is returned in this output parameter. 
  *                    A NULL pointer may be used here if the
  *                    text length is not needed, as would be the case if
  *                    the text is known in advance to be a NUL terminated
  *                    string.
  * @param status      Receives errors detected by this function.
  * @return            Poiner to the subject text string currently associated with
  *                    this regular expression.
  * @stable ICU 3.0
  */
U_STABLE const UChar * U_EXPORT2 
uregex_getText(URegularExpression *regexp,
               int32_t            *textLength,
               UErrorCode         *status);

/**
  *   Attempts to match the input string, beginning at startIndex, against the pattern.
  *   To succeed, the match must extend to the end of the input string.
  *
  *    @param  regexp      The compiled regular expression.
  *    @param  startIndex  The input string index at which to begin matching.
  *    @param  status      Receives errors detected by this function.
  *    @return             TRUE if there is a match
  *    @stable ICU 3.0
  */
U_STABLE UBool U_EXPORT2 
uregex_matches(URegularExpression *regexp,
                int32_t            startIndex,
                UErrorCode        *status);

/**
  *   Attempts to match the input string, starting from the specified index, against the pattern.
  *   The match may be of any length, and is not required to extend to the end
  *   of the input string.  Contrast with uregex_matches().
  *
  *   <p>If the match succeeds then more information can be obtained via the
  *    <code>uregexp_start()</code>, <code>uregexp_end()</code>,
  *    and <code>uregexp_group()</code> functions.</p>
  *
  *    @param   regexp      The compiled regular expression.
  *    @param   startIndex  The input string index at which to begin matching.
  *    @param   status      A reference to a UErrorCode to receive any errors.
  *    @return  TRUE if there is a match.
  *    @stable ICU 3.0
  */
U_STABLE UBool U_EXPORT2 
uregex_lookingAt(URegularExpression *regexp,
                 int32_t             startIndex,
                 UErrorCode         *status);

/**
  *   Find the first matching substring of the input string that matches the pattern.
  *   The search for a match begins at the specified index.
  *   If a match is found, <code>uregex_start(), uregex_end()</code>, and
  *   <code>uregex_group()</code> will provide more information regarding the match.
  *
  *   @param   regexp      The compiled regular expression.
  *   @param   startIndex  The position in the input string to begin the search
  *   @param   status      A reference to a UErrorCode to receive any errors.
  *   @return              TRUE if a match is found.
  *   @stable ICU 3.0
  */
U_STABLE UBool U_EXPORT2 
uregex_find(URegularExpression *regexp,
            int32_t             startIndex, 
            UErrorCode         *status);

/**
  *  Find the next pattern match in the input string.
  *  Begin searching the input at the location following the end of
  *  the previous match, or at the start of the string if there is no previous match.
  *  If a match is found, <code>uregex_start(), uregex_end()</code>, and
  *  <code>uregex_group()</code> will provide more information regarding the match.
  *
  *  @param   regexp      The compiled regular expression.
  *  @param   status      A reference to a UErrorCode to receive any errors.
  *  @return              TRUE if a match is found.
  *  @see uregex_reset
  *  @stable ICU 3.0
  */
U_STABLE UBool U_EXPORT2 
uregex_findNext(URegularExpression *regexp,
                UErrorCode         *status);

/**
  *   Get the number of capturing groups in this regular expression's pattern.
  *   @param   regexp      The compiled regular expression.
  *   @param   status      A reference to a UErrorCode to receive any errors.
  *   @return the number of capture groups
  *   @stable ICU 3.0
  */
U_STABLE int32_t U_EXPORT2 
uregex_groupCount(URegularExpression *regexp,
                  UErrorCode         *status);

/** Extract the string for the specified matching expression or subexpression.
  * Group #0 is the complete string of matched text.
  * Group #1 is the text matched by the first set of capturing parentheses.
  *
  *   @param   regexp       The compiled regular expression.
  *   @param   groupNum     The capture group to extract.  Group 0 is the complete
  *                         match.  The value of this parameter must be
  *                         less than or equal to the number of capture groups in
  *                         the pattern.
  *   @param   dest         Buffer to receive the matching string data
  *   @param   destCapacity Capacity of the dest buffer.
  *   @param   status       A reference to a UErrorCode to receive any errors.
  *   @return               Length of matching data,
  *                         or -1 if no applicable match.
  *   @stable ICU 3.0
  */
U_STABLE int32_t U_EXPORT2 
uregex_group(URegularExpression *regexp,
             int32_t             groupNum,
             UChar              *dest,
             int32_t             destCapacity,
             UErrorCode          *status);


/**
  *   Returns the index in the input string of the start of the text matched by the
  *   specified capture group during the previous match operation.  Return -1 if
  *   the capture group was not part of the last match.
  *   Group #0 refers to the complete range of matched text.
  *   Group #1 refers to the text matched by the first set of capturing parentheses.
  *
  *    @param   regexp      The compiled regular expression.
  *    @param   groupNum    The capture group number
  *    @param   status      A reference to a UErrorCode to receive any errors.
  *    @return              the starting position in the input of the text matched 
  *                         by the specified group.
  *    @stable ICU 3.0
  */
U_STABLE int32_t U_EXPORT2 
uregex_start(URegularExpression *regexp,
             int32_t             groupNum,
             UErrorCode          *status);

/**
  *   Returns the index in the input string of the position following the end
  *   of the text matched by the specified capture group.
  *   Return -1 if the capture group was not part of the last match.
  *   Group #0 refers to the complete range of matched text.
  *   Group #1 refers to the text matched by the first set of capturing parentheses.
  *
  *    @param   regexp      The compiled regular expression.
  *    @param   groupNum    The capture group number
  *    @param   status      A reference to a UErrorCode to receive any errors.
  *    @return              the index of the position following the last matched character.
  *    @stable ICU 3.0
  */
U_STABLE int32_t U_EXPORT2 
uregex_end(URegularExpression   *regexp,
           int32_t               groupNum,
           UErrorCode           *status);

/**
  *  Reset any saved state from the previous match.  Has the effect of
  *  causing uregex_findNext to begin at the specified index, and causing
  *  uregex_start(), uregex_end() and uregex_group() to return an error 
  *  indicating that there is no match information available.
  *
  *    @param   regexp      The compiled regular expression.
  *    @param   index       The position in the text at which a
  *                         uregex_findNext() should begin searching.
  *    @param   status      A reference to a UErrorCode to receive any errors.
  *    @stable ICU 3.0
  */
U_STABLE void U_EXPORT2 
uregex_reset(URegularExpression    *regexp,
             int32_t               index,
             UErrorCode            *status);

/**
  *    Replaces every substring of the input that matches the pattern
  *    with the given replacement string.  This is a convenience function that
  *    provides a complete find-and-replace-all operation.
  *
  *    This method scans the input string looking for matches of the pattern. 
  *    Input that is not part of any match is copied unchanged to the
  *    destination buffer.  Matched regions are replaced in the output
  *    buffer by the replacement string.   The replacement string may contain
  *    references to capture groups; these take the form of $1, $2, etc.
  *
  *    @param   regexp             The compiled regular expression.
  *    @param   replacementText    A string containing the replacement text.
  *    @param   replacementLength  The length of the replacement string, or
  *                                -1 if it is NUL terminated.
  *    @param   destBuf            A (UChar *) buffer that will receive the result.
  *    @param   destCapacity       The capacity of the desitnation buffer.
  *    @param   status             A reference to a UErrorCode to receive any errors.
  *    @return                     The length of the string resulting from the find
  *                                and replace operation.  In the event that the
  *                                destination capacity is inadequate, the return value
  *                                is still the full length of the untruncated string.
  *    @stable ICU 3.0
  */
U_STABLE int32_t U_EXPORT2 
uregex_replaceAll(URegularExpression    *regexp,
                  const UChar           *replacementText,
                  int32_t                replacementLength,
                  UChar                 *destBuf,
                  int32_t                destCapacity,
                  UErrorCode            *status);


/**
  *    Replaces the first substring of the input that matches the pattern
  *    with the given replacement string.  This is a convenience function that
  *    provides a complete find-and-replace operation.
  *
  *    This method scans the input string looking for a match of the pattern. 
  *    All input that is not part of the match is copied unchanged to the
  *    destination buffer.  The matched region is replaced in the output
  *    buffer by the replacement string.   The replacement string may contain
  *    references to capture groups; these take the form of $1, $2, etc.
  *
  *    @param   regexp             The compiled regular expression.
  *    @param   replacementText    A string containing the replacement text.
  *    @param   replacementLength  The length of the replacement string, or
  *                                -1 if it is NUL terminated.
  *    @param   destBuf            A (UChar *) buffer that will receive the result.
  *    @param   destCapacity       The capacity of the desitnation buffer.
  *    @param   status             a reference to a UErrorCode to receive any errors.
  *    @return                     The length of the string resulting from the find
  *                                and replace operation.  In the event that the
  *                                destination capacity is inadequate, the return value
  *                                is still the full length of the untruncated string.
  *    @stable ICU 3.0
  */
U_STABLE int32_t U_EXPORT2 
uregex_replaceFirst(URegularExpression  *regexp,
                    const UChar         *replacementText,
                    int32_t              replacementLength,
                    UChar               *destBuf,
                    int32_t              destCapacity,
                    UErrorCode          *status);


/**
  *   Implements a replace operation intended to be used as part of an
  *   incremental find-and-replace.
  *
  *   <p>The input string, starting from the end of the previous match and ending at
  *   the start of the current match, is appended to the destination string.  Then the
  *   replacement string is appended to the output string,
  *   including handling any substitutions of captured text.</p>
  *
  *   <p>A note on preflight computation of buffersize and error handling:
  *   Calls to uregex_appendReplacement() and uregex_appendTail() are
  *   designed to be chained, one after another, with the destination
  *   buffer pointer and buffer capacity updated after each in preparation
  *   to for the next.  If the destination buffer is exhausted partway through such a
  *   sequence, a U_BUFFER_OVERFLOW_ERROR status will be returned.  Normal
  *   ICU conventions are for a function to perform no action if it is
  *   called with an error status, but for this one case, uregex_appendRepacement()
  *   will operate normally so that buffer size computations will complete
  *   correctly.
  *
  *   <p>For simple, prepackaged, non-incremental find-and-replace
  *      operations, see replaceFirst() or replaceAll().</p>
  *
  *   @param   regexp      The regular expression object.  
  *   @param   replacementText The string that will replace the matched portion of the
  *                        input string as it is copied to the destination buffer.
  *                        The replacement text may contain references ($1, for
  *                        example) to capture groups from the match.
  *   @param   replacementLength  The length of the replacement text string,
  *                        or -1 if the string is NUL terminated.
  *   @param   destBuf     The buffer into which the results of the
  *                        find-and-replace are placed.  On return, this pointer
  *                        will be updated to refer to the beginning of the
  *                        unused portion of buffer, leaving it in position for
  *                        a subsequent call to this function.
  *   @param   destCapacity The size of the output buffer,  On return, this
  *                        parameter will be updated to reflect the space remaining
  *                        unused in the output buffer.
  *   @param   status      A reference to a UErrorCode to receive any errors. 
  *   @return              The length of the result string.  In the event that
  *                        destCapacity is inadequate, the full length of the
  *                        untruncated output string is returned.
  *
  *   @stable ICU 3.0
  *
  */
U_STABLE int32_t U_EXPORT2 
uregex_appendReplacement(URegularExpression    *regexp,
                  const UChar           *replacementText,
                  int32_t                replacementLength,
                  UChar                **destBuf,
                  int32_t               *destCapacity,
                  UErrorCode            *status);


/**
  * As the final step in a find-and-replace operation, append the remainder
  * of the input string, starting at the position following the last match,
  * to the destination string. <code>uregex_appendTail()</code> is intended 
  *  to be invoked after one or more invocations of the
  *  <code>uregex_appendReplacement()</code> function.
  *
  *   @param   regexp      The regular expression object.  This is needed to 
  *                        obtain the input string and with the position
  *                        of the last match within it.
  *   @param   destBuf     The buffer in which the results of the
  *                        find-and-replace are placed.  On return, the pointer
  *                        will be updated to refer to the beginning of the
  *                        unused portion of buffer.
  *   @param   destCapacity The size of the output buffer,  On return, this
  *                        value will be updated to reflect the space remaining
  *                        unused in the output buffer.
  *   @param   status      A reference to a UErrorCode to receive any errors. 
  *   @return              The length of the result string.  In the event that
  *                        destCapacity is inadequate, the full length of the
  *                        untruncated output string is returned.
  *
  *   @stable ICU 3.0
  */
U_STABLE int32_t U_EXPORT2 
uregex_appendTail(URegularExpression    *regexp,
                  UChar                **destBuf,
                  int32_t               *destCapacity,
                  UErrorCode            *status);




 /**
   * Split a string into fields.  Somewhat like split() from Perl.
   *  The pattern matches identify delimiters that separate the input
   *  into fields.  The input data between the matches becomes the
   *  fields themselves.
   * <p>
   *  Each of the fields is copied from the input string to the destination
   *  buffer, and the NUL terminated.  The position of each field within
   *  the destination buffer is returned in the destFields array.
   *
   *  Note:  another choice for the design of this function would be to not
   *         copy the resulting fields at all, but to return indexes and
   *         lengths within the source text.  
   *           Advantages would be
   *             o  Faster.  No Copying.
   *             o  Nothing extra needed when field data may contain embedded NUL chars.
   *             o  Less memory needed if working on large data.
   *           Disadvantages
   *             o  Less consistent with C++ split, which copies into an
   *                array of UnicodeStrings.
   *             o  No NUL termination, extracted fields would be less convenient
   *                to use in most cases.
   *             o  Possible problems in the future, when support Unicode Normalization
   *                could cause the fields to not correspond exactly to
   *                a range of the source text.
   * 
   *    @param   regexp      The compiled regular expression.
   *    @param   destBuf     A (UChar *) buffer to receive the fields that
   *                         are extracted from the input string. These
   *                         field pointers will refer to positions within the
   *                         destination buffer supplied by the caller.  Any
   *                         extra positions within the destFields array will be
   *                         set to NULL.
   *    @param   destCapacity The capacity of the destBuf.
   *    @param   requiredCapacity  The actual capacity required of the destBuf.
   *                         If destCapacity is too small, requiredCapacity will return 
   *                         the total capacity required to hold all of the output, and
   *                         a U_BUFFER_OVERFLOW_ERROR will be returned.
   *    @param   destFields  An array to be filled with the position of each
   *                         of the extracted fields within destBuf.
   *    @param   destFieldsCapacity  The number of elements in the destFields array.
   *                If the number of fields found is less than destFieldsCapacity,
   *                the extra destFields elements are set to zero.
   *                If destFieldsCapacity is too small, the trailing part of the
   *                input, including any field delimiters, is treated as if it
   *                were the last field - it is copied to the destBuf, and
   *                its position is in the destBuf is stored in the last element
   *                of destFields.  This behavior mimics that of Perl.  It is not
   *                an error condition, and no error status is returned when all destField
   *                positions are used.
   * @param status  A reference to a UErrorCode to receive any errors.
   * @return        The number of fields into which the input string was split.
   * @stable ICU 3.0
   */
U_STABLE int32_t U_EXPORT2 
uregex_split(   URegularExpression      *regexp,
                  UChar                 *destBuf,
                  int32_t                destCapacity,
                  int32_t               *requiredCapacity,
                  UChar                 *destFields[],
                  int32_t                destFieldsCapacity,
                  UErrorCode            *status);



#endif   /*  !UCONFIG_NO_REGULAR_EXPRESSIONS  */
#endif   /*  UREGEX_H  */
