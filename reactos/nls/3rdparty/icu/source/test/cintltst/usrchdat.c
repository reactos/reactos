/********************************************************************
 * Copyright (c) 2001-2007 International Business Machines 
 * Corporation and others. All Rights Reserved.
 ********************************************************************
 * File USRCHDAT.H
 * Modification History:
 * Name           date            Description            
 * synwee         July 31 2001    creation
 ********************************************************************/


/*
Note: This file is included by other C and C++ files. This file should not be directly compiled.
*/
#ifndef USRCHDAT_C
#define USRCHDAT_C

#include "unicode/ucol.h"

#if !UCONFIG_NO_COLLATION

U_CDECL_BEGIN
struct SearchData {
    const char               *text;
    const char               *pattern;
    const char               *collator;
          UCollationStrength  strength;
    const char               *breaker;
          int8_t              offset[32];
          uint8_t             size[32];
};
U_CDECL_END

typedef struct SearchData SearchData;

static const char *TESTCOLLATORRULE = "& o,O ; p,P";

static const char *EXTRACOLLATIONRULE = " & ae ; \\u00e4 & AE ; \\u00c4 & oe ; \\u00f6 & OE ; \\u00d6 & ue ; \\u00fc & UE ; \\u00dc";

static const SearchData BASIC[] = {
    {"xxxxxxxxxxxxxxxxxxxx", "fisher", NULL, UCOL_TERTIARY, NULL, {-1}, {0}},
    {"silly spring string", "string", NULL, UCOL_TERTIARY, NULL, {13, -1}, 
    {6}},
    {"silly spring string string", "string", NULL, UCOL_TERTIARY, NULL,
    {13, 20, -1}, {6, 6}},
    {"silly string spring string", "string", NULL, UCOL_TERTIARY, NULL,
    {6, 20, -1}, {6, 6}},
    {"string spring string", "string", NULL, UCOL_TERTIARY, NULL, {0, 14, -1}, 
    {6, 6}},
    {"Scott Ganyo", "c", NULL, UCOL_TERTIARY, NULL, {1, -1}, {1}},
    {"Scott Ganyo", " ", NULL, UCOL_TERTIARY, NULL, {5, -1}, {1}},
    {"\\u0300\\u0325", "\\u0300", NULL, UCOL_TERTIARY, NULL, {-1}, {0}},
    {"a\\u0300\\u0325", "\\u0300", NULL, UCOL_TERTIARY, NULL, {-1}, {0}},
    {"a\\u0300\\u0325", "\\u0300\\u0325", NULL, UCOL_TERTIARY, NULL, {1, -1}, 
    {2}},
    {"a\\u0300b", "\\u0300", NULL, UCOL_TERTIARY, NULL, {1, -1}, {1}},
    {"\\u00c9", "e", NULL, UCOL_PRIMARY,  NULL, {0, -1}, {1}},
    {NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

static const SearchData BREAKITERATOREXACT[] = {
    {"foxy fox", "fox", NULL, UCOL_TERTIARY, "characterbreaker", {0, 5, -1}, 
    {3, 3}},
    {"foxy fox", "fox", NULL, UCOL_TERTIARY, "wordbreaker", {5, -1}, {3}},
    {"This is a toe T\\u00F6ne", "toe", "de", UCOL_PRIMARY, 
    "characterbreaker", {10, 14, -1}, {3, 2}},
    {"This is a toe T\\u00F6ne", "toe", "de", UCOL_PRIMARY, "wordbreaker", 
    {10, -1}, {3}},
    {"Channel, another channel, more channels, and one last Channel",
    "Channel", "es", UCOL_TERTIARY, "wordbreaker", {0, 54, -1}, {7, 7}},
    /* jitterbug 1745 */
    {"testing that \\u00e9 does not match e", "e", NULL, UCOL_TERTIARY, 
     "characterbreaker", {1, 17, 30, -1}, {1, 1, 1}},
    {"testing that string ab\\u00e9cd does not match e", "e", NULL, 
     UCOL_TERTIARY, "characterbreaker", {1, 28, 41, -1}, {1, 1, 1}},
    {"\\u00c9", "e", "fr", UCOL_PRIMARY,  "characterbreaker", {0, -1}, {1}},
    {NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

static const SearchData STRENGTH[] = {
    /*012345678901234567890123456789012345678901234567890123456789*/
    {"The quick brown fox jumps over the lazy foxes", "fox", "en", 
        UCOL_PRIMARY, NULL, {16, 40, -1}, {3, 3}},
    {"The quick brown fox jumps over the lazy foxes", "fox", "en", 
    UCOL_PRIMARY, "wordbreaker", {16, -1}, {3}},
    {"blackbirds Pat p\\u00E9ch\\u00E9 p\\u00EAche p\\u00E9cher p\\u00EAcher Tod T\\u00F6ne black Tofu blackbirds Ton PAT toehold blackbird black-bird pat toe big Toe",
    "peche", "fr", UCOL_PRIMARY, NULL, {15, 21, 27, 34, -1}, {5, 5, 5, 5}},
    {"This is a toe T\\u00F6ne", "toe", "de", UCOL_PRIMARY, NULL, 
    {10, 14, -1}, {3, 2}},
    {"A channel, another CHANNEL, more Channels, and one last channel...",
    "channel", "es", UCOL_PRIMARY, NULL, {2, 19, 33, 56, -1},
    {7, 7, 7, 7}},
    {"\\u00c0 should match but not A", "A\\u0300", "en", UCOL_IDENTICAL, 
    NULL, {0, -1}, {1, 0}},
    {NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

static const SearchData VARIABLE[] = {
    /*012345678901234567890123456789012345678901234567890123456789*/
    {"blackbirds black blackbirds blackbird black-bird",
    "blackbird", NULL, UCOL_TERTIARY, NULL, {0, 17, 28, 38, -1}, 
    {9, 9, 9, 10}},
    /* to see that it doesn't go into an infinite loop if the start of text
    is a ignorable character */
    {" on", "go", NULL, UCOL_TERTIARY, NULL, {-1}, {0}},
    {"abcdefghijklmnopqrstuvwxyz", "   ", NULL, UCOL_PRIMARY, NULL, 
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, -1}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    /* testing tightest match */ 
    {" abc  a bc   ab c    a  bc     ab  c", "abc", NULL, UCOL_QUATERNARY, 
    NULL, {1, -1}, {3}},
    /*012345678901234567890123456789012345678901234567890123456789 */
    {" abc  a bc   ab c    a  bc     ab  c", "abc", NULL, UCOL_SECONDARY, 
    NULL, {1, 6, 13, 21, 31, -1}, {3, 4, 4, 5, 5}},
    /* totally ignorable text */
    {"           ---------------", "abc", NULL, UCOL_SECONDARY, 
    NULL, {-1}, {0}},
    {NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

static const SearchData NORMEXACT[] = {
    {"a\\u0300\\u0325", "\\u0325\\u0300", NULL, UCOL_TERTIARY, NULL, {1, -1}, 
    {2}},
    {NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

static const SearchData NONNORMEXACT[] = {
    {"a\\u0300\\u0325", "\\u0325\\u0300", NULL, UCOL_TERTIARY, NULL, {-1}, 
    {0}},
    {NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

static const SearchData OVERLAP[] = {
    {"abababab", "abab", NULL, UCOL_TERTIARY, NULL, {0, 2, 4, -1}, 
    {4, 4, 4}},
    {NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

static const SearchData NONOVERLAP[] = {
    {"abababab", "abab", NULL, UCOL_TERTIARY, NULL, {0, 4, -1}, {4, 4}},
    {NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

static const SearchData COLLATOR[] = {
    /* english */
    {"fox fpx", "fox", NULL, UCOL_TERTIARY, NULL, {0, -1}, {3}}, 
    /* tailored */
    {"fox fpx", "fox", NULL, UCOL_PRIMARY, NULL, {0, 4, -1}, {3, 3}}, 
    {NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

static const SearchData PATTERN[] = {
    {"The quick brown fox jumps over the lazy foxes", "the", NULL, 
        UCOL_PRIMARY, NULL, {0, 31, -1}, {3, 3}},
    {"The quick brown fox jumps over the lazy foxes", "fox", NULL, 
        UCOL_PRIMARY, NULL, {16, 40, -1}, {3, 3}},
    {NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

static const SearchData TEXT[] = {
    {"the foxy brown fox", "fox", NULL, UCOL_TERTIARY, NULL, {4, 15, -1}, 
    {3, 3}},
    {"the quick brown fox", "fox", NULL, UCOL_TERTIARY, NULL, {16, -1},
    {3}},
    {NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

static const SearchData COMPOSITEBOUNDARIES[] = {
    {"\\u00C0", "A", NULL, UCOL_TERTIARY, NULL, {0, -1}, {1}},
    {"A\\u00C0C", "A", NULL, UCOL_TERTIARY, NULL, {0, 1, -1}, {1, 1}},
    {"\\u00C0A", "A", NULL, UCOL_TERTIARY, NULL, {0, 1, -1}, {1, 1}},
    {"B\\u00C0", "A", NULL, UCOL_TERTIARY, NULL, {1, -1}, {1}},
    {"\\u00C0B", "A", NULL, UCOL_TERTIARY, NULL, {0, -1}, {1}},
    {"\\u00C0", "\\u0300", NULL, UCOL_TERTIARY, NULL, {0, -1}, {1}},
    {"\\u0300\\u00C0", "\\u0300", NULL, UCOL_TERTIARY, NULL, {0, 1, -1}, 
    {1, 1}},
    {"\\u00C0\\u0300", "\\u0300", NULL, UCOL_TERTIARY, NULL, {-1}, {0}},
    /* A + 030A + 0301 */
    {"\\u01FA", "\\u01FA", NULL, UCOL_TERTIARY, NULL, {0, -1}, {1}},
    {"\\u01FA", "\\u030A", NULL, UCOL_TERTIARY, NULL, {-1}, {0}},
    {"\\u01FA", "A\\u030A", NULL, UCOL_TERTIARY, NULL, {-1}, {0}},
    {"\\u01FA", "\\u030AA", NULL, UCOL_TERTIARY, NULL, {-1}, {0}},
    {"\\u01FA", "\\u0301", NULL, UCOL_TERTIARY, NULL, {-1}, {0}},
    {"\\u01FA", "A\\u0301", NULL, UCOL_TERTIARY, NULL, {-1}, {0}},
    {"\\u01FA", "\\u0301A", NULL, UCOL_TERTIARY, NULL, {-1}, {0}},
    {"\\u01FA", "\\u030A\\u0301", NULL, UCOL_TERTIARY, NULL, {0, -1}, {1}},
    {"A\\u01FA", "A\\u030A", NULL, UCOL_TERTIARY, NULL, {-1}, {0}},
    {"\\u01FAA", "\\u0301A", NULL, UCOL_TERTIARY, NULL, {-1}, {0}},
    {"\\u0F73", "\\u0F73", NULL, UCOL_TERTIARY, NULL, {0, -1}, {1}},
    {"\\u0F73", "\\u0F71", NULL, UCOL_TERTIARY, NULL, {-1}, {0}},
    {"\\u0F73", "\\u0F72", NULL, UCOL_TERTIARY, NULL, {-1}, {0}},
    {"\\u0F73", "\\u0F71\\u0F72", NULL, UCOL_TERTIARY, NULL, {0, -1}, {1}},
    {"A\\u0F73", "A\\u0F71", NULL, UCOL_TERTIARY, NULL, {-1}, {0}},
    {"\\u0F73A", "\\u0F72A", NULL, UCOL_TERTIARY, NULL, {-1}, {0}},
    {NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

static const SearchData MATCH[] = {
    {"a busy bee is a very busy beeee", "bee", NULL, UCOL_TERTIARY, NULL, 
    {7, 26, -1}, {3, 3}},
    /* 012345678901234567890123456789012345678901234567890 */
    {"a busy bee is a very busy beeee with no bee life", "bee", NULL, 
    UCOL_TERTIARY, NULL, {7, 26, 40, -1}, {3, 3, 3}},
    {NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

static const SearchData SUPPLEMENTARY[] = {
    /* 012345678901234567890123456789012345678901234567890012345678901234567890123456789012345678901234567890012345678901234567890123456789 */
    {"abc \\uD800\\uDC00 \\uD800\\uDC01 \\uD801\\uDC00 \\uD800\\uDC00abc abc\\uD800\\uDC00 \\uD800\\uD800\\uDC00 \\uD800\\uDC00\\uDC00", 
     "\\uD800\\uDC00", NULL, UCOL_TERTIARY, NULL, {4, 13, 22, 26, 29, -1}, 
    {2, 2, 2, 2, 2}},
    {"and\\uD834\\uDDB9this sentence", "\\uD834\\uDDB9", NULL, 
     UCOL_TERTIARY, NULL, {3, -1}, {2}},
    {"and \\uD834\\uDDB9 this sentence", " \\uD834\\uDDB9 ", NULL, 
     UCOL_TERTIARY, NULL, {3, -1}, {4}},
    {"and-\\uD834\\uDDB9-this sentence", "-\\uD834\\uDDB9-", NULL, 
     UCOL_TERTIARY, NULL, {3, -1}, {4}},
    {"and,\\uD834\\uDDB9,this sentence", ",\\uD834\\uDDB9,", NULL, 
     UCOL_TERTIARY, NULL, {3, -1}, {4}},
    {"and?\\uD834\\uDDB9?this sentence", "?\\uD834\\uDDB9?", NULL, 
     UCOL_TERTIARY, NULL, {3, -1}, {4}},
    {NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

static const char *CONTRACTIONRULE = 
    "&z = ab/c < AB < X\\u0300 < ABC < X\\u0300\\u0315";

static const SearchData CONTRACTION[] = {
    /* common discontiguous */
    {"A\\u0300\\u0315", "\\u0300", NULL, UCOL_TERTIARY, NULL, {-1}, {0}},
    {"A\\u0300\\u0315", "\\u0300\\u0315", NULL, UCOL_TERTIARY, NULL, {1, -1}, 
    {2}},
    /* contraction prefix */
    {"AB\\u0315C", "A", NULL, UCOL_TERTIARY, NULL, {-1}, {0}},
    {"AB\\u0315C", "AB", NULL, UCOL_TERTIARY, NULL, {0, -1}, {2}},
    {"AB\\u0315C", "\\u0315", NULL, UCOL_TERTIARY, NULL, {2, -1}, {1}},
    /* discontiguous problem here for backwards iteration. 
    accents not found because discontiguous stores all information */
    {"X\\u0300\\u0319\\u0315", "\\u0319", NULL, UCOL_TERTIARY, NULL, {-1}, 
    {0}},
     /* ends not with a contraction character */
    {"X\\u0315\\u0300D", "\\u0300\\u0315", NULL, UCOL_TERTIARY, NULL, {-1}, 
    {0}},
    {"X\\u0315\\u0300D", "X\\u0300\\u0315", NULL, UCOL_TERTIARY, NULL, 
    {0, -1}, {3}},
    {"X\\u0300\\u031A\\u0315D", "X\\u0300", NULL, UCOL_TERTIARY, NULL, {-1}, 
    {0}},
    /* blocked discontiguous */
    {"X\\u0300\\u031A\\u0315D", "\\u031A\\u0315D", NULL, UCOL_TERTIARY, NULL, 
    {-1}, {0}},
    {"ab", "z", NULL, UCOL_TERTIARY, NULL, {0, -1}, {2}},
    {NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

static const char *IGNORABLERULE = "&a = \\u0300";

static const SearchData IGNORABLE[] = {
    {"\\u0300\\u0315 \\u0300\\u0315 ", "\\u0300", NULL, UCOL_PRIMARY, NULL, 
    {0, 3, -1}, {2, 2}},
    {NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

static const SearchData BASICCANONICAL[] = {
    {"xxxxxxxxxxxxxxxxxxxx", "fisher", NULL, UCOL_TERTIARY, NULL, {-1}, {0}},
    {"silly spring string", "string", NULL, UCOL_TERTIARY, NULL, {13, -1}, 
    {6}},
    {"silly spring string string", "string", NULL, UCOL_TERTIARY, NULL,
    {13, 20, -1}, {6, 6}},
    {"silly string spring string", "string", NULL, UCOL_TERTIARY, NULL,
    {6, 20, -1}, {6, 6}},
    {"string spring string", "string", NULL, UCOL_TERTIARY, NULL, {0, 14, -1}, 
    {6, 6}},
    {"Scott Ganyo", "c", NULL, UCOL_TERTIARY, NULL, {1, -1}, {1}},
    {"Scott Ganyo", " ", NULL, UCOL_TERTIARY, NULL, {5, -1}, {1}},
    {"\\u0300\\u0325", "\\u0300", NULL, UCOL_TERTIARY, NULL, {0, -1}, {2}},
    {"a\\u0300\\u0325", "\\u0300", NULL, UCOL_TERTIARY, NULL, {1, -1}, {2}},
    {"a\\u0300\\u0325", "\\u0300\\u0325", NULL, UCOL_TERTIARY, NULL, {1, -1}, 
    {2}},
    {"a\\u0300b", "\\u0300", NULL, UCOL_TERTIARY, NULL, {1, -1}, {1}},
    {"a\\u0300\\u0325b", "\\u0300b", NULL, UCOL_TERTIARY, NULL, {1, -1}, {3}},
    {"\\u0325\\u0300A\\u0325\\u0300", "\\u0300A\\u0300", NULL, UCOL_TERTIARY, 
    NULL, {0, -1}, {5}},
    {"\\u0325\\u0300A\\u0325\\u0300", "\\u0325A\\u0325", NULL, UCOL_TERTIARY, 
    NULL, {0, -1}, {5}},
    {"a\\u0300\\u0325b\\u0300\\u0325c \\u0325b\\u0300 \\u0300b\\u0325", 
    "\\u0300b\\u0325", NULL, UCOL_TERTIARY, NULL, {1, 12, -1}, {5, 3}},
    {"\\u00c4\\u0323", "A\\u0323\\u0308", NULL, UCOL_TERTIARY, NULL, {0, -1}, {2}},
    {"\\u0308\\u0323", "\\u0323\\u0308", NULL, UCOL_TERTIARY, NULL, {0, -1}, {2}},
    {NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

static const SearchData NORMCANONICAL[] = {
    {"\\u0300\\u0325", "\\u0300", NULL, UCOL_TERTIARY, NULL, {0, -1}, {2}},
    {"\\u0300\\u0325", "\\u0325", NULL, UCOL_TERTIARY, NULL, {0, -1}, {2}},
    {"a\\u0300\\u0325", "\\u0325\\u0300", NULL, UCOL_TERTIARY, NULL, {1, -1}, 
    {2}},
    {"a\\u0300\\u0325", "\\u0300\\u0325", NULL, UCOL_TERTIARY, NULL, {1, -1}, 
    {2}},
    {"a\\u0300\\u0325", "\\u0325", NULL, UCOL_TERTIARY, NULL, {1, -1}, {2}},
    {"a\\u0300\\u0325", "\\u0300", NULL, UCOL_TERTIARY, NULL, {1, -1}, {2}},
    {NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

static const SearchData BREAKITERATORCANONICAL[] = {
    {"foxy fox", "fox", NULL, UCOL_TERTIARY, "characterbreaker", {0, 5, -1}, 
    {3, 3}},
    {"foxy fox", "fox", NULL, UCOL_TERTIARY, "wordbreaker", {5, -1}, {3}},
    {"This is a toe T\\u00F6ne", "toe", "de", UCOL_PRIMARY, 
    "characterbreaker", {10, 14, -1}, {3, 2}},
    {"This is a toe T\\u00F6ne", "toe", "de", UCOL_PRIMARY, "wordbreaker", 
    {10, -1}, {3}},
    {"Channel, another channel, more channels, and one last Channel",
    "Channel", "es", UCOL_TERTIARY, "wordbreaker", {0, 54, -1}, {7, 7}},
    /* jitterbug 1745 */
    {"testing that \\u00e9 does not match e", "e", NULL, UCOL_TERTIARY, 
     "characterbreaker", {1, 17, 30, -1}, {1, 1, 1}},
    {"testing that string ab\\u00e9cd does not match e", "e", NULL, 
     UCOL_TERTIARY, "characterbreaker", {1, 28, 41, -1}, {1, 1, 1}},
    {"\\u00c9", "e", "fr", UCOL_PRIMARY,  "characterbreaker", {0, -1}, {1}},
    {NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

static const SearchData STRENGTHCANONICAL[] = {
    /*012345678901234567890123456789012345678901234567890123456789 */
    {"The quick brown fox jumps over the lazy foxes", "fox", "en", 
        UCOL_PRIMARY, NULL, {16, 40, -1}, {3, 3}},
    {"The quick brown fox jumps over the lazy foxes", "fox", "en", 
    UCOL_PRIMARY, "wordbreaker", {16, -1}, {3}},
    {"blackbirds Pat p\\u00E9ch\\u00E9 p\\u00EAche p\\u00E9cher p\\u00EAcher Tod T\\u00F6ne black Tofu blackbirds Ton PAT toehold blackbird black-bird pat toe big Toe",
    "peche", "fr", UCOL_PRIMARY, NULL, {15, 21, 27, 34, -1}, {5, 5, 5, 5}},
    {"This is a toe T\\u00F6ne", "toe", "de", UCOL_PRIMARY, NULL, 
    {10, 14, -1}, {3, 2}},
    {"A channel, another CHANNEL, more Channels, and one last channel...",
    "channel", "es", UCOL_PRIMARY, NULL, {2, 19, 33, 56, -1},
    {7, 7, 7, 7}},
    {NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

static const SearchData VARIABLECANONICAL[] = {
    /*012345678901234567890123456789012345678901234567890123456789 */
    {"blackbirds black blackbirds blackbird black-bird",
    "blackbird", NULL, UCOL_TERTIARY, NULL, {0, 17, 28, 38, -1}, 
    {9, 9, 9, 10}},
    /* to see that it doesn't go into an infinite loop if the start of text 
    is a ignorable character */
    {" on", "go", NULL, UCOL_TERTIARY, NULL, {-1}, {0}},
    {"abcdefghijklmnopqrstuvwxyz", "   ", NULL, UCOL_PRIMARY, NULL, 
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, -1}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    /* testing tightest match */
    {" abc  a bc   ab c    a  bc     ab  c", "abc", NULL, UCOL_QUATERNARY, 
    NULL, {1, -1}, {3}},
    /*012345678901234567890123456789012345678901234567890123456789 */
    {" abc  a bc   ab c    a  bc     ab  c", "abc", NULL, UCOL_SECONDARY, 
    NULL, {1, 6, 13, 21, 31, -1}, {3, 4, 4, 5, 5}},
    /* totally ignorable text */
    {"           ---------------", "abc", NULL, UCOL_SECONDARY, 
    NULL, {-1}, {0}},
    {NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

static const SearchData OVERLAPCANONICAL[] = {
    {"abababab", "abab", NULL, UCOL_TERTIARY, NULL, {0, 2, 4, -1}, 
    {4, 4, 4}},
    {NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

static const SearchData NONOVERLAPCANONICAL[] = {
    {"abababab", "abab", NULL, UCOL_TERTIARY, NULL, {0, 4, -1}, {4, 4}},
    {NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

static const SearchData COLLATORCANONICAL[] = {
    /* english */
    {"fox fpx", "fox", NULL, UCOL_TERTIARY, NULL, {0, -1}, {3}}, 
    /* tailored */
    {"fox fpx", "fox", NULL, UCOL_PRIMARY, NULL, {0, 4, -1}, {3, 3}}, 
    {NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

static const SearchData PATTERNCANONICAL[] = {
    {"The quick brown fox jumps over the lazy foxes", "the", NULL, 
        UCOL_PRIMARY, NULL, {0, 31, -1}, {3, 3}},
    {"The quick brown fox jumps over the lazy foxes", "fox", NULL, 
        UCOL_PRIMARY, NULL, {16, 40, -1}, {3, 3}},
    {NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

static const SearchData TEXTCANONICAL[] = {
    {"the foxy brown fox", "fox", NULL, UCOL_TERTIARY, NULL, {4, 15, -1}, 
    {3, 3}},
    {"the quick brown fox", "fox", NULL, UCOL_TERTIARY, NULL, {16, -1},
    {3}},
    {NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

static const SearchData COMPOSITEBOUNDARIESCANONICAL[] = {
    {"\\u00C0", "A", NULL, UCOL_TERTIARY, NULL, {0, -1}, {1}},
    {"A\\u00C0C", "A", NULL, UCOL_TERTIARY, NULL, {0, 1, -1}, {1, 1}},
    {"\\u00C0A", "A", NULL, UCOL_TERTIARY, NULL, {0, 1, -1}, {1, 1}},
    {"B\\u00C0", "A", NULL, UCOL_TERTIARY, NULL, {1, -1}, {1}},
    {"\\u00C0B", "A", NULL, UCOL_TERTIARY, NULL, {0, -1}, {1}},
    {"\\u00C0", "\\u0300", NULL, UCOL_TERTIARY, NULL, {0, -1}, {1}},
    {"\\u0300\\u00C0", "\\u0300", NULL, UCOL_TERTIARY, NULL, {0, 1, -1}, 
    {1, 1}},
    /* \\u0300 blocked by \\u0300 */
    {"\\u00C0\\u0300", "\\u0300", NULL, UCOL_TERTIARY, NULL, {0, -1}, {2}},
    /* A + 030A + 0301 */
    {"\\u01FA", "\\u01FA", NULL, UCOL_TERTIARY, NULL, {0, -1}, {1}},
    {"\\u01FA", "\\u030A", NULL, UCOL_TERTIARY, NULL, {0, -1}, {1}},
    {"\\u01FA", "A\\u030A", NULL, UCOL_TERTIARY, NULL, {0, -1}, {1}},
    {"\\u01FA", "\\u030AA", NULL, UCOL_TERTIARY, NULL, {-1}, {0}},
    {"\\u01FA", "\\u0301", NULL, UCOL_TERTIARY, NULL, {0, -1}, {1}},
    /* blocked accent */
    {"\\u01FA", "A\\u0301", NULL, UCOL_TERTIARY, NULL, {-1}, {0}},
    {"\\u01FA", "\\u0301A", NULL, UCOL_TERTIARY, NULL, {-1}, {0}},
    {"\\u01FA", "\\u030A\\u0301", NULL, UCOL_TERTIARY, NULL, {0, -1}, {1}},
    {"A\\u01FA", "A\\u030A", NULL, UCOL_TERTIARY, NULL, {1, -1}, {1}},
    {"\\u01FAA", "\\u0301A", NULL, UCOL_TERTIARY, NULL, {0, -1}, {2}},
    {"\\u0F73", "\\u0F73", NULL, UCOL_TERTIARY, NULL, {0, -1}, {1}},
    {"\\u0F73", "\\u0F71", NULL, UCOL_TERTIARY, NULL, {0, -1}, {1}},
    {"\\u0F73", "\\u0F72", NULL, UCOL_TERTIARY, NULL, {0, -1}, {1}},
    {"\\u0F73", "\\u0F71\\u0F72", NULL, UCOL_TERTIARY, NULL, {0, -1}, {1}},
    {"A\\u0F73", "A\\u0F71", NULL, UCOL_TERTIARY, NULL, {0, -1}, {2}},
    {"\\u0F73A", "\\u0F72A", NULL, UCOL_TERTIARY, NULL, {0, -1}, {2}},
    {"\\u01FA A\\u0301\\u030A A\\u030A\\u0301 A\\u030A \\u01FA", "A\\u030A", 
    NULL, UCOL_TERTIARY, NULL, {0, 6, 10, 13, -1}, {1, 3, 2, 1}},
    {NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

static const SearchData MATCHCANONICAL[] = {
    {"a busy bee is a very busy beeee", "bee", NULL, UCOL_TERTIARY, NULL, 
    {7, 26, -1}, {3, 3}},
    /*012345678901234567890123456789012345678901234567890 */
    {"a busy bee is a very busy beeee with no bee life", "bee", NULL, 
    UCOL_TERTIARY, NULL, {7, 26, 40, -1}, {3, 3, 3}},
    {NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

static const SearchData SUPPLEMENTARYCANONICAL[] = {
    /*012345678901234567890123456789012345678901234567890012345678901234567890123456789012345678901234567890012345678901234567890123456789 */
    {"abc \\uD800\\uDC00 \\uD800\\uDC01 \\uD801\\uDC00 \\uD800\\uDC00abc abc\\uD800\\uDC00 \\uD800\\uD800\\uDC00 \\uD800\\uDC00\\uDC00", 
     "\\uD800\\uDC00", NULL, UCOL_TERTIARY, NULL, {4, 13, 22, 26, 29, -1}, 
    {2, 2, 2, 2, 2}},
    {"and\\uD834\\uDDB9this sentence", "\\uD834\\uDDB9", NULL, 
     UCOL_TERTIARY, NULL, {3, -1}, {2}},
    {"and \\uD834\\uDDB9 this sentence", " \\uD834\\uDDB9 ", NULL, 
     UCOL_TERTIARY, NULL, {3, -1}, {4}},
    {"and-\\uD834\\uDDB9-this sentence", "-\\uD834\\uDDB9-", NULL, 
     UCOL_TERTIARY, NULL, {3, -1}, {4}},
    {"and,\\uD834\\uDDB9,this sentence", ",\\uD834\\uDDB9,", NULL, 
     UCOL_TERTIARY, NULL, {3, -1}, {4}},
    {"and?\\uD834\\uDDB9?this sentence", "?\\uD834\\uDDB9?", NULL, 
     UCOL_TERTIARY, NULL, {3, -1}, {4}},
    {NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

static const SearchData CONTRACTIONCANONICAL[] = {
    /* common discontiguous */
    {"A\\u0300\\u0315", "\\u0300", NULL, UCOL_TERTIARY, NULL, {1, -1}, {2}},
    {"A\\u0300\\u0315", "\\u0300\\u0315", NULL, UCOL_TERTIARY, NULL, {1, -1}, 
    {2}},
    /* contraction prefix */
    {"AB\\u0315C", "A", NULL, UCOL_TERTIARY, NULL, {-1}, {0}},
    {"AB\\u0315C", "AB", NULL, UCOL_TERTIARY, NULL, {0, -1}, {2}},
    {"AB\\u0315C", "\\u0315", NULL, UCOL_TERTIARY, NULL, {2, -1}, {1}},
    /* discontiguous problem here for backwards iteration. 
    forwards gives 0, 4 but backwards give 1, 3 */
    /* {"X\\u0300\\u0319\\u0315", "\\u0319", NULL, UCOL_TERTIARY, NULL, {0, -1}, 
    {4}}, */
    
     /* ends not with a contraction character */
    {"X\\u0315\\u0300D", "\\u0300\\u0315", NULL, UCOL_TERTIARY, NULL, {-1}, 
    {0}},
    {"X\\u0315\\u0300D", "X\\u0300\\u0315", NULL, UCOL_TERTIARY, NULL, 
    {0, -1}, {3}},
    {"X\\u0300\\u031A\\u0315D", "X\\u0300", NULL, UCOL_TERTIARY, NULL, 
    {0, -1}, {4}},
    /* blocked discontiguous */
    {"X\\u0300\\u031A\\u0315D", "\\u031A\\u0315D", NULL, UCOL_TERTIARY, NULL, 
    {1, -1}, {4}},
    {"ab", "z", NULL, UCOL_TERTIARY, NULL, {0, -1}, {2}},
    {NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

static const SearchData DIACTRICMATCH[] = {
		{"\\u03BA\\u03B1\\u03B9\\u0300\\u0020\\u03BA\\u03B1\\u1F76", "\\u03BA\\u03B1\\u03B9", NULL, UCOL_PRIMARY, NULL, {0, 5,-1}, {4, 3}},
		{"\\u0061\\u0061\\u00E1", "\\u0061\\u00E1", NULL, UCOL_SECONDARY, NULL, {1, -1}, {2}},
		{"\\u0020\\u00C2\\u0303\\u0020\\u0041\\u0061\\u1EAA\\u0041\\u0302\\u0303\\u00C2\\u0303\\u1EAB\\u0061\\u0302\\u0303\\u00E2\\u0303\\uD806\\uDC01\\u0300\\u0020",
		 "\\u00C2\\u0303", "LDE_AN_CX_EX_FX_HX_NX_S1", UCOL_PRIMARY, NULL, {1, 4, 5, 6, 7, 10, 12, 13, 16,-1}, {2, 1, 1, 1, 3, 2, 1, 3, 2}},
		{NULL, NULL, NULL, UCOL_TERTIARY, NULL, {-1}, {0}}
};

#endif /* #if !UCONFIG_NO_COLLATION */

#endif
