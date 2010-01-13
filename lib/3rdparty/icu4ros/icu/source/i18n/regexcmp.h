//
//  regexcmp.h
//
//  Copyright (C) 2002-2007, International Business Machines Corporation and others.
//  All Rights Reserved.
//
//  This file contains declarations for the class RegexCompile
//
//  This class is internal to the regular expression implementation.
//  For the public Regular Expression API, see the file "unicode/regex.h"
//


#ifndef RBBISCAN_H
#define RBBISCAN_H

#include "unicode/utypes.h"
#if !UCONFIG_NO_REGULAR_EXPRESSIONS

#include "unicode/uobject.h"
#include "unicode/uniset.h"
#include "unicode/parseerr.h"
#include "uhash.h"
#include "uvector.h"



U_NAMESPACE_BEGIN


//--------------------------------------------------------------------------------
//
//  class RegexCompile    Contains the regular expression compiler.
//
//--------------------------------------------------------------------------------
static const int    kStackSize = 100;               // The size of the state stack for
                                                    //   pattern parsing.  Corresponds roughly
                                                    //   to the depth of parentheses nesting
                                                    //   that is allowed in the rules.

struct  RegexTableEl;
class   RegexPattern;


class RegexCompile : public UMemory {
public:

    struct RegexPatternChar {
        UChar32             fChar;
        UBool               fQuoted;
    };

    RegexCompile(RegexPattern *rp, UErrorCode &e);
    
    void       compile(const UnicodeString &pat, UParseError &pp, UErrorCode &e);


    virtual    ~RegexCompile();

    void        nextChar(RegexPatternChar &c);      // Get the next char from the input stream.

    static void cleanup();                       // Memory cleanup



    // Categories of parentheses in pattern.
    //   The category is saved in the compile-time parentheses stack frame, and
    //   determines the code to be generated when the matching close ) is encountered.
    enum EParenClass {
        plain        = -1,               // No special handling
        capturing    = -2, 
        atomic       = -3,
        lookAhead    = -4,
        negLookAhead = -5,
        flags        = -6,
        lookBehind   = -7,
        lookBehindN  = -8
    };

private:


    UBool       doParseActions(int32_t a);
    void        error(UErrorCode e);                   // error reporting convenience function.

    UChar32     nextCharLL();
    UChar32     peekCharLL();
    UnicodeSet  *scanSet();
    UnicodeSet  *scanProp();
    void        handleCloseParen();
    int32_t     blockTopLoc(UBool reserve);          // Locate a position in the compiled pattern
                                                     //  at the top of the just completed block
                                                     //  or operation, and optionally ensure that
                                                     //  there is space to add an opcode there.
    void        compileSet(UnicodeSet *theSet);      // Generate the compiled pattern for
                                                     //   a reference to a UnicodeSet.
    void        compileInterval(int32_t InitOp,      // Generate the code for a {min,max} quantifier.
                               int32_t LoopOp);
    UBool       compileInlineInterval();             // Generate inline code for a {min,max} quantifier
    void        literalChar(UChar32 c);              // Compile a literal char
    void        fixLiterals(UBool split=FALSE);      // Fix literal strings.
    void        insertOp(int32_t where);             // Open up a slot for a new op in the
                                                     //   generated code at the specified location.
    void        emitONE_CHAR(UChar32 c);             // EMit a ONE_CHAR op into the compiled code,
                                                     //   taking case mode into account.
    int32_t     minMatchLength(int32_t start,
                               int32_t end);
    int32_t     maxMatchLength(int32_t start,
                               int32_t end);
    void        matchStartType();
    void        stripNOPs();
    void        OptDotStar();


    UErrorCode                    *fStatus;
    RegexPattern                  *fRXPat;
    UParseError                   *fParseErr;

    //
    //  Data associated with low level character scanning
    //
    int32_t                       fScanIndex;        // Index of current character being processed
                                                     //   in the rule input string.
    int32_t                       fNextIndex;        // Index of the next character, which
                                                     //   is the first character not yet scanned.
    UBool                         fQuoteMode;        // Scan is in a \Q...\E quoted region
    UBool                         fInBackslashQuote; // Scan is between a '\' and the following char.
    UBool                         fEOLComments;      // When scan is just after '(?',  inhibit #... to 
                                                     //   end of line comments, in favor of (?#...) comments.
    int32_t                       fLineNum;          // Line number in input file.
    int32_t                       fCharNum;          // Char position within the line.
    UChar32                       fLastChar;         // Previous char, needed to count CR-LF
                                                     //   as a single line, not two.
    UChar32                       fPeekChar;         // Saved char, if we've scanned ahead.


    RegexPatternChar              fC;                // Current char for parse state machine
                                                     //   processing.

    //
    //   Data for the state machine that parses the regular expression.
    //
    RegexTableEl                  **fStateTable;     // State Transition Table for regex Rule
                                                     //   parsing.  index by p[state][char-class]

    uint16_t                      fStack[kStackSize];  // State stack, holds state pushes
    int32_t                       fStackPtr;           //  and pops as specified in the state
                                                       //  transition rules.

    //
    //  Data associated with the generation of the pcode for the match engine
    //
    int32_t                       fModeFlags;        // Match Flags.  (Case Insensitive, etc.)
                                                     //   Always has high bit (31) set so that flag values
                                                     //   on the paren stack are distinguished from relocatable
                                                     //   pcode addresses.
    int32_t                       fNewModeFlags;     // New flags, while compiling (?i, holds state
                                                     //   until last flag is scanned.
    UBool                         fSetModeFlag;      // true for (?ismx, false for (?-ismx


    int32_t                       fStringOpStart;    // While a literal string is being scanned
                                                     //   holds the start index within RegexPattern.
                                                     //   fLiteralText where the string is being stored.

    int32_t                       fPatternLength;    // Length of the input pattern string.

    UVector32                     fParenStack;       // parentheses stack.  Each frame consists of
                                                     //   the positions of compiled pattern operations
                                                     //   needing fixup, followed by negative value.  The  
                                                     //   first entry in each frame is the position of the
                                                     //   spot reserved for use when a quantifier
                                                     //   needs to add a SAVE at the start of a (block)
                                                     //   The negative value (-1, -2,...) indicates
                                                     //   the kind of paren that opened the frame.  Some
                                                     //   need special handling on close.


    int32_t                       fMatchOpenParen;   // The position in the compiled pattern
                                                     //   of the slot reserved for a state save
                                                     //   at the start of the most recently processed
                                                     //   parenthesized block.
    int32_t                       fMatchCloseParen;  // The position in the pattern of the first
                                                     //   location after the most recently processed
                                                     //   parenthesized block.

    int32_t                       fIntervalLow;      // {lower, upper} interval quantifier values.
    int32_t                       fIntervalUpper;    // Placed here temporarily, when pattern is
                                                     //   initially scanned.  Each new interval
                                                     //   encountered overwrites these values.
                                                     //   -1 for the upper interval value means none
                                                     //   was specified (unlimited occurences.)

    int32_t                       fNameStartPos;     // Starting position of a \N{NAME} name in a
                                                     //   pattern, valid while remainder of name is
                                                     //   scanned.
};

U_NAMESPACE_END
#endif   // !UCONFIG_NO_REGULAR_EXPRESSIONS
#endif   // RBBISCAN_H
