/*
 ******************************************************************************
 * Copyright (C) 1998-2003, International Business Machines Corporation and   *
 * others. All Rights Reserved.                                               *
 ******************************************************************************
 */

#include <windows.h>

#include "layout/LEFontInstance.h"

#include "GDIFontInstance.h"

#include "GUISupport.h"
#include "FontMap.h"
#include "GDIFontMap.h"

GDIFontMap::GDIFontMap(GDISurface *surface, const char *fileName, le_int16 pointSize, GUISupport *guiSupport, LEErrorCode &status)
    : FontMap(fileName, pointSize, guiSupport, status), fSurface(surface)
{
    // nothing to do?
}

GDIFontMap::~GDIFontMap()
{
    // anything?
}

const LEFontInstance *GDIFontMap::openFont(const char *fontName, le_int16 pointSize, LEErrorCode &status)
{
	LEFontInstance *result = new GDIFontInstance(fSurface, fontName, pointSize, status);

	if (LE_FAILURE(status)) {
		delete result;
		result = NULL;
	}

    return result;
}
