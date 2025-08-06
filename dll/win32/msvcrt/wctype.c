/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT
 * PURPOSE:     Implementation of wctype
 * COPYRIGHT:   Copyright 2024 ReactOS Project
 */

#include <precomp.h>
#include <wctype.h>
#include <string.h>

/*
 * @implemented
 * wctype - Get character classification category
 */
wctype_t __cdecl wctype(const char *property)
{
    if (!property)
        return 0;
    
    if (!strcmp(property, "alnum"))
        return _ALPHA | _DIGIT;
    else if (!strcmp(property, "alpha"))
        return _ALPHA;
    else if (!strcmp(property, "blank"))
        return _BLANK;
    else if (!strcmp(property, "cntrl"))
        return _CONTROL;
    else if (!strcmp(property, "digit"))
        return _DIGIT;
    else if (!strcmp(property, "graph"))
        return _ALPHA | _DIGIT | _PUNCT;
    else if (!strcmp(property, "lower"))
        return _LOWER;
    else if (!strcmp(property, "print"))
        return _ALPHA | _DIGIT | _PUNCT | _BLANK;
    else if (!strcmp(property, "punct"))
        return _PUNCT;
    else if (!strcmp(property, "space"))
        return _SPACE;
    else if (!strcmp(property, "upper"))
        return _UPPER;
    else if (!strcmp(property, "xdigit"))
        return _HEX;
    
    return 0;
}