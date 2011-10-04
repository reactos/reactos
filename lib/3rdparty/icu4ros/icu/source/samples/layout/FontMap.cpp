/*
 ******************************************************************************
 * Copyright (C) 1998-2006, International Business Machines Corporation and   *
 * others. All Rights Reserved.                                               *
 ******************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "unicode/utypes.h"
#include "unicode/uscript.h"

#include "layout/LETypes.h"
#include "layout/LEScripts.h"
#include "layout/LEFontInstance.h"

#include "GUISupport.h"
#include "FontMap.h"

FontMap::FontMap(const char *fileName, le_int16 pointSize, GUISupport *guiSupport, LEErrorCode &status)
    : fPointSize(pointSize), fFontCount(0), fAscent(0), fDescent(0), fLeading(0), fGUISupport(guiSupport)
{
    le_int32 defaultFont = -1, i, script;
    le_bool haveFonts = FALSE;

/**/
    for (i = 0; i < scriptCodeCount; i += 1) {
        fFontIndices[i] = -1;
        fFontNames[i] = NULL;
        fFontInstances[i] = NULL;
    }
 /**/

    if (LE_FAILURE(status)) {
        return;
    }

    char *c, *scriptName, *fontName, *line, buffer[BUFFER_SIZE];
    FILE *file;

    file = fopen(fileName, "r");

    if (file == NULL) {
        sprintf(errorMessage, "Could not open the font map file: %s.", fileName);
        fGUISupport->postErrorMessage(errorMessage, "Font Map Error");
        status = LE_FONT_FILE_NOT_FOUND_ERROR;
        return;
    }

    while (fgets(buffer, BUFFER_SIZE, file) != NULL) {
        UScriptCode scriptCode;
        UErrorCode scriptStatus = U_ZERO_ERROR;

        line = strip(buffer);
        if (line[0] == '#' || line[0] == 0) {
            continue;
        }

        c = strchr(line, ':');
        c[0] = 0;

        fontName   = strip(&c[1]);
        scriptName = strip(line);

        if (strcmp(scriptName, "DEFAULT") == 0) {
            defaultFont = getFontIndex(fontName);
            haveFonts = TRUE;
            continue;
        }

        le_int32 fillCount = uscript_getCode(scriptName, &scriptCode, 1, &scriptStatus);

        if (U_FAILURE(scriptStatus) || fillCount <= 0 ||
            scriptStatus == U_USING_FALLBACK_WARNING || scriptStatus == U_USING_DEFAULT_WARNING) {
            sprintf(errorMessage, "The script name %s is invalid.", line);
            fGUISupport->postErrorMessage(errorMessage, "Font Map Error");
            continue;
        }

        script = (le_int32) scriptCode;

        if (fFontIndices[script] >= 0) {
            // FIXME: complain that this is a duplicate entry and bail (?)
            fFontIndices[script] = -1;
        }

        fFontIndices[script] = getFontIndex(fontName);
        haveFonts = TRUE;
    }

    if (defaultFont >= 0) {
        for (script = 0; script < scriptCodeCount; script += 1) {
            if (fFontIndices[script] < 0) {
                fFontIndices[script] = defaultFont;
            }
        }
    }

    if (! haveFonts) {
        sprintf(errorMessage, "The font map file %s does not contain any valid scripts.", fileName);
        fGUISupport->postErrorMessage(errorMessage, "Font Map Error");
        status = LE_ILLEGAL_ARGUMENT_ERROR;
    }

    fclose(file);
}

FontMap::~FontMap()
{
    le_int32 font;

    for (font = 0; font < fFontCount; font += 1) {
        if (fFontNames[font] != NULL) {
            delete[] (char *) fFontNames[font];
        }
    }

    for (font = 0; font < fFontCount; font += 1) {
        if (fFontInstances[font] != NULL) {
            delete fFontInstances[font];
        }
    }
}

le_int32 FontMap::getFontIndex(const char *fontName)
{
    le_int32 index;

    for (index = 0; index < fFontCount; index += 1) {
        if (strcmp(fontName, fFontNames[index]) == 0) {
            return index;
        }
    }

    if (fFontCount < (le_int32) scriptCodeCount) {
        index = fFontCount++;
    } else {
        // The font name table is full. Since there can
        // only be scriptCodeCount fonts in use at once,
        // there should be at least one that's not being
        // referenced; find it and resue it's index.

        for (index = 0; index < fFontCount; index += 1) {
            le_int32 script;

            for (script = 0; script < scriptCodeCount; script += 1) {
                if (fFontIndices[script] == index) {
                    break;
                }
            }

            if (script >= scriptCodeCount) {
                break;
            }
        }
    }

    if (index >= scriptCodeCount) {
        return -1;
    }

    le_int32 len = strlen(fontName);
    char *s = new char[len + 1];

    fFontNames[index] = strcpy(s, fontName);
    return index;
}

char *FontMap::strip(char *s)
{
    le_int32 start, end, len;

    start = 0;
    len = strlen(s);

    while (start < len && isspace(s[start])) {
        start += 1;
    }

    end = len - 1;

    while (end > start && isspace(s[end])) {
        end -= 1;
    }

    if (end < len) {
        s[end + 1] = '\0';
    }

    return &s[start];
}

const LEFontInstance *FontMap::getScriptFont(le_int32 scriptCode, LEErrorCode &status)
{
    if (LE_FAILURE(status)) {
        return NULL;
    }

    if (scriptCode <= -1 || scriptCode >= scriptCodeCount) {
        status = LE_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }


    le_int32 fontIndex = fFontIndices[scriptCode];

    if (fontIndex < 0) {
        sprintf(errorMessage, "No font was set for script %s", uscript_getName((UScriptCode) scriptCode));
        fGUISupport->postErrorMessage(errorMessage, "Font Map Error");
        status = LE_FONT_FILE_NOT_FOUND_ERROR;
        return NULL;
    }

    if (fFontInstances[fontIndex] == NULL) {
        fFontInstances[fontIndex] = openFont(fFontNames[fontIndex], fPointSize, status);

        if (LE_FAILURE(status)) {
            sprintf(errorMessage, "Could not open font file %s", fFontNames[fontIndex]);
            fGUISupport->postErrorMessage(errorMessage, "Font Map Error");
            return NULL;
        }
    }

    return fFontInstances[fontIndex];
}

le_int32 FontMap::getAscent() const
{
    if (fAscent <= 0) {
        ((FontMap *) this)->getMaxMetrics();
    }

    return fAscent;
}

le_int32 FontMap::getDescent() const
{
    if (fDescent <= 0) {
        ((FontMap *) this)->getMaxMetrics();
    }

    return fDescent;
}

le_int32 FontMap::getLeading() const
{
    if (fLeading <= 0) {
        ((FontMap *) this)->getMaxMetrics();
    }

    return fLeading;
}

void FontMap::getMaxMetrics()
{
    for (le_int32 i = 0; i < fFontCount; i += 1) {
        LEErrorCode status = LE_NO_ERROR;
        le_int32 ascent, descent, leading;

        if (fFontInstances[i] == NULL) {
            fFontInstances[i] = openFont(fFontNames[i], fPointSize, status);

            if (LE_FAILURE(status)) {
                continue;
            }
        }

        ascent  = fFontInstances[i]->getAscent();
        descent = fFontInstances[i]->getDescent();
        leading = fFontInstances[i]->getLeading();

        if (ascent > fAscent) {
            fAscent = ascent;
        }

        if (descent > fDescent) {
            fDescent = descent;
        }

        if (leading > fLeading) {
            fLeading = leading;
        }
    }
}

