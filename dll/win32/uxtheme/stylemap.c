/*
 * msstyle data maps
 *
 * Copyright (C) 2004 Kevin Koltzau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "tmschema.h"

#define TMT_ENUM 200

typedef struct _MSSTYLES_PROPERTY_MAP {
    WORD dwPrimitiveType;
    WORD dwPropertyID;
    WCHAR szPropertyName[24];
} MSSTYLES_PROPERTY_MAP, *PMSSTYLES_PROPERTY_MAP;

typedef struct _MSSTYLES_ENUM_MAP {
    WORD dwEnum;
    WORD dwValue;
    WCHAR szValueName[18];
} MSSTYLES_ENUM_MAP, *PMSSTYLES_ENUM_MAP;

typedef struct _MSSTYLES_CLASS_MAP {
    WORD dwPartID;
    WORD dwStateID;
    WCHAR szName[31];
} MSSTYLES_CLASS_MAP, *PMSSTYLES_CLASS_MAP;

typedef struct _MSSTYLES_CLASS_NAME {
    const MSSTYLES_CLASS_MAP *lpMap;
    WCHAR pszClass[12];
} MSSTYLES_CLASS_NAME, *PMSSTYLES_CLASS_NAME;

/***********************************************************************
 * Map property names to IDs & primitive types
 * PrimitiveType,PropertyID,PropertyName
 */
static const MSSTYLES_PROPERTY_MAP mapProperty[] = {
    {TMT_STRING,TMT_STRING,{'S','T','R','I','N','G','\0'}},
    {TMT_INT,TMT_INT,{'I','N','T','\0'}},
    {TMT_BOOL,TMT_BOOL,{'B','O','O','L','\0'}},
    {TMT_COLOR,TMT_COLOR,{'C','O','L','O','R','\0'}},
    {TMT_MARGINS,TMT_MARGINS,{'M','A','R','G','I','N','S','\0'}},
    {TMT_FILENAME,TMT_FILENAME,{'F','I','L','E','N','A','M','E','\0'}},
    {TMT_SIZE,TMT_SIZE,{'S','I','Z','E','\0'}},
    {TMT_POSITION,TMT_POSITION,{'P','O','S','I','T','I','O','N','\0'}},
    {TMT_RECT,TMT_RECT,{'R','E','C','T','\0'}},
    {TMT_FONT,TMT_FONT,{'F','O','N','T','\0'}},
    {TMT_INTLIST,TMT_INTLIST,{'I','N','T','L','I','S','T','\0'}},
    {TMT_STRING,TMT_COLORSCHEMES,{'C','O','L','O','R','S','C','H','E','M','E','S','\0'}},
    {TMT_STRING,TMT_SIZES,{'S','I','Z','E','S','\0'}},
    {TMT_INT,TMT_CHARSET,{'C','H','A','R','S','E','T','\0'}},
    {TMT_STRING,TMT_DISPLAYNAME,{'D','I','S','P','L','A','Y','N','A','M','E','\0'}},
    {TMT_STRING,TMT_TOOLTIP,{'T','O','O','L','T','I','P','\0'}},
    {TMT_STRING,TMT_COMPANY,{'C','O','M','P','A','N','Y','\0'}},
    {TMT_STRING,TMT_AUTHOR,{'A','U','T','H','O','R','\0'}},
    {TMT_STRING,TMT_COPYRIGHT,{'C','O','P','Y','R','I','G','H','T','\0'}},
    {TMT_STRING,TMT_URL,{'U','R','L','\0'}},
    {TMT_STRING,TMT_VERSION,{'V','E','R','S','I','O','N','\0'}},
    {TMT_STRING,TMT_DESCRIPTION,{'D','E','S','C','R','I','P','T','I','O','N','\0'}},
    {TMT_FONT,TMT_CAPTIONFONT,{'C','A','P','T','I','O','N','F','O','N','T','\0'}},
    {TMT_FONT,TMT_SMALLCAPTIONFONT,{'S','M','A','L','L','C','A','P','T','I','O','N','F','O','N','T','\0'}},
    {TMT_FONT,TMT_MENUFONT,{'M','E','N','U','F','O','N','T','\0'}},
    {TMT_FONT,TMT_STATUSFONT,{'S','T','A','T','U','S','F','O','N','T','\0'}},
    {TMT_FONT,TMT_MSGBOXFONT,{'M','S','G','B','O','X','F','O','N','T','\0'}},
    {TMT_FONT,TMT_ICONTITLEFONT,{'I','C','O','N','T','I','T','L','E','F','O','N','T','\0'}},
    {TMT_BOOL,TMT_FLATMENUS,{'F','L','A','T','M','E','N','U','S','\0'}},
    {TMT_SIZE,TMT_SIZINGBORDERWIDTH,{'S','I','Z','I','N','G','B','O','R','D','E','R','W','I','D','T','H','\0'}},
    {TMT_SIZE,TMT_SCROLLBARWIDTH,{'S','C','R','O','L','L','B','A','R','W','I','D','T','H','\0'}},
    {TMT_SIZE,TMT_SCROLLBARHEIGHT,{'S','C','R','O','L','L','B','A','R','H','E','I','G','H','T','\0'}},
    {TMT_SIZE,TMT_CAPTIONBARWIDTH,{'C','A','P','T','I','O','N','B','A','R','W','I','D','T','H','\0'}},
    {TMT_SIZE,TMT_CAPTIONBARHEIGHT,{'C','A','P','T','I','O','N','B','A','R','H','E','I','G','H','T','\0'}},
    {TMT_SIZE,TMT_SMCAPTIONBARWIDTH,{'S','M','C','A','P','T','I','O','N','B','A','R','W','I','D','T','H','\0'}},
    {TMT_SIZE,TMT_SMCAPTIONBARHEIGHT,{'S','M','C','A','P','T','I','O','N','B','A','R','H','E','I','G','H','T','\0'}},
    {TMT_SIZE,TMT_MENUBARWIDTH,{'M','E','N','U','B','A','R','W','I','D','T','H','\0'}},
    {TMT_SIZE,TMT_MENUBARHEIGHT,{'M','E','N','U','B','A','R','H','E','I','G','H','T','\0'}},
    {TMT_INT,TMT_MINCOLORDEPTH,{'M','I','N','C','O','L','O','R','D','E','P','T','H','\0'}},
    {TMT_STRING,TMT_CSSNAME,{'C','S','S','N','A','M','E','\0'}},
    {TMT_STRING,TMT_XMLNAME,{'X','M','L','N','A','M','E','\0'}},
    {TMT_COLOR,TMT_SCROLLBAR,{'S','C','R','O','L','L','B','A','R','\0'}},
    {TMT_COLOR,TMT_BACKGROUND,{'B','A','C','K','G','R','O','U','N','D','\0'}},
    {TMT_COLOR,TMT_ACTIVECAPTION,{'A','C','T','I','V','E','C','A','P','T','I','O','N','\0'}},
    {TMT_COLOR,TMT_INACTIVECAPTION,{'I','N','A','C','T','I','V','E','C','A','P','T','I','O','N','\0'}},
    {TMT_COLOR,TMT_MENU,{'M','E','N','U','\0'}},
    {TMT_COLOR,TMT_WINDOW,{'W','I','N','D','O','W','\0'}},
    {TMT_COLOR,TMT_WINDOWFRAME,{'W','I','N','D','O','W','F','R','A','M','E','\0'}},
    {TMT_COLOR,TMT_MENUTEXT,{'M','E','N','U','T','E','X','T','\0'}},
    {TMT_COLOR,TMT_WINDOWTEXT,{'W','I','N','D','O','W','T','E','X','T','\0'}},
    {TMT_COLOR,TMT_CAPTIONTEXT,{'C','A','P','T','I','O','N','T','E','X','T','\0'}},
    {TMT_COLOR,TMT_ACTIVEBORDER,{'A','C','T','I','V','E','B','O','R','D','E','R','\0'}},
    {TMT_COLOR,TMT_INACTIVEBORDER,{'I','N','A','C','T','I','V','E','B','O','R','D','E','R','\0'}},
    {TMT_COLOR,TMT_APPWORKSPACE,{'A','P','P','W','O','R','K','S','P','A','C','E','\0'}},
    {TMT_COLOR,TMT_HIGHLIGHT,{'H','I','G','H','L','I','G','H','T','\0'}},
    {TMT_COLOR,TMT_HIGHLIGHTTEXT,{'H','I','G','H','L','I','G','H','T','T','E','X','T','\0'}},
    {TMT_COLOR,TMT_BTNFACE,{'B','T','N','F','A','C','E','\0'}},
    {TMT_COLOR,TMT_BTNSHADOW,{'B','T','N','S','H','A','D','O','W','\0'}},
    {TMT_COLOR,TMT_GRAYTEXT,{'G','R','A','Y','T','E','X','T','\0'}},
    {TMT_COLOR,TMT_BTNTEXT,{'B','T','N','T','E','X','T','\0'}},
    {TMT_COLOR,TMT_INACTIVECAPTIONTEXT,{'I','N','A','C','T','I','V','E','C','A','P','T','I','O','N','T','E','X','T','\0'}},
    {TMT_COLOR,TMT_BTNHIGHLIGHT,{'B','T','N','H','I','G','H','L','I','G','H','T','\0'}},
    {TMT_COLOR,TMT_DKSHADOW3D,{'D','K','S','H','A','D','O','W','3','D','\0'}},
    {TMT_COLOR,TMT_LIGHT3D,{'L','I','G','H','T','3','D','\0'}},
    {TMT_COLOR,TMT_INFOTEXT,{'I','N','F','O','T','E','X','T','\0'}},
    {TMT_COLOR,TMT_INFOBK,{'I','N','F','O','B','K','\0'}},
    {TMT_COLOR,TMT_BUTTONALTERNATEFACE,{'B','U','T','T','O','N','A','L','T','E','R','N','A','T','E','F','A','C','E','\0'}},
    {TMT_COLOR,TMT_HOTTRACKING,{'H','O','T','T','R','A','C','K','I','N','G','\0'}},
    {TMT_COLOR,TMT_GRADIENTACTIVECAPTION,{'G','R','A','D','I','E','N','T','A','C','T','I','V','E','C','A','P','T','I','O','N','\0'}},
    {TMT_COLOR,TMT_GRADIENTINACTIVECAPTION,{'G','R','A','D','I','E','N','T','I','N','A','C','T','I','V','E','C','A','P','T','I','O','N','\0'}},
    {TMT_COLOR,TMT_MENUHILIGHT,{'M','E','N','U','H','I','L','I','G','H','T','\0'}},
    {TMT_COLOR,TMT_MENUBAR,{'M','E','N','U','B','A','R','\0'}},
    {TMT_INT,TMT_FROMHUE1,{'F','R','O','M','H','U','E','1','\0'}},
    {TMT_INT,TMT_FROMHUE2,{'F','R','O','M','H','U','E','2','\0'}},
    {TMT_INT,TMT_FROMHUE3,{'F','R','O','M','H','U','E','3','\0'}},
    {TMT_INT,TMT_FROMHUE4,{'F','R','O','M','H','U','E','4','\0'}},
    {TMT_INT,TMT_FROMHUE5,{'F','R','O','M','H','U','E','5','\0'}},
    {TMT_INT,TMT_TOHUE1,{'T','O','H','U','E','1','\0'}},
    {TMT_INT,TMT_TOHUE2,{'T','O','H','U','E','2','\0'}},
    {TMT_INT,TMT_TOHUE3,{'T','O','H','U','E','3','\0'}},
    {TMT_INT,TMT_TOHUE4,{'T','O','H','U','E','4','\0'}},
    {TMT_INT,TMT_TOHUE5,{'T','O','H','U','E','5','\0'}},
    {TMT_COLOR,TMT_FROMCOLOR1,{'F','R','O','M','C','O','L','O','R','1','\0'}},
    {TMT_COLOR,TMT_FROMCOLOR2,{'F','R','O','M','C','O','L','O','R','2','\0'}},
    {TMT_COLOR,TMT_FROMCOLOR3,{'F','R','O','M','C','O','L','O','R','3','\0'}},
    {TMT_COLOR,TMT_FROMCOLOR4,{'F','R','O','M','C','O','L','O','R','4','\0'}},
    {TMT_COLOR,TMT_FROMCOLOR5,{'F','R','O','M','C','O','L','O','R','5','\0'}},
    {TMT_COLOR,TMT_TOCOLOR1,{'T','O','C','O','L','O','R','1','\0'}},
    {TMT_COLOR,TMT_TOCOLOR2,{'T','O','C','O','L','O','R','2','\0'}},
    {TMT_COLOR,TMT_TOCOLOR3,{'T','O','C','O','L','O','R','3','\0'}},
    {TMT_COLOR,TMT_TOCOLOR4,{'T','O','C','O','L','O','R','4','\0'}},
    {TMT_COLOR,TMT_TOCOLOR5,{'T','O','C','O','L','O','R','5','\0'}},
    {TMT_BOOL,TMT_TRANSPARENT,{'T','R','A','N','S','P','A','R','E','N','T','\0'}},
    {TMT_BOOL,TMT_AUTOSIZE,{'A','U','T','O','S','I','Z','E','\0'}},
    {TMT_BOOL,TMT_BORDERONLY,{'B','O','R','D','E','R','O','N','L','Y','\0'}},
    {TMT_BOOL,TMT_COMPOSITED,{'C','O','M','P','O','S','I','T','E','D','\0'}},
    {TMT_BOOL,TMT_BGFILL,{'B','G','F','I','L','L','\0'}},
    {TMT_BOOL,TMT_GLYPHTRANSPARENT,{'G','L','Y','P','H','T','R','A','N','S','P','A','R','E','N','T','\0'}},
    {TMT_BOOL,TMT_GLYPHONLY,{'G','L','Y','P','H','O','N','L','Y','\0'}},
    {TMT_BOOL,TMT_ALWAYSSHOWSIZINGBAR,{'A','L','W','A','Y','S','S','H','O','W','S','I','Z','I','N','G','B','A','R','\0'}},
    {TMT_BOOL,TMT_MIRRORIMAGE,{'M','I','R','R','O','R','I','M','A','G','E','\0'}},
    {TMT_BOOL,TMT_UNIFORMSIZING,{'U','N','I','F','O','R','M','S','I','Z','I','N','G','\0'}},
    {TMT_BOOL,TMT_INTEGRALSIZING,{'I','N','T','E','G','R','A','L','S','I','Z','I','N','G','\0'}},
    {TMT_BOOL,TMT_SOURCEGROW,{'S','O','U','R','C','E','G','R','O','W','\0'}},
    {TMT_BOOL,TMT_SOURCESHRINK,{'S','O','U','R','C','E','S','H','R','I','N','K','\0'}},
    {TMT_INT,TMT_IMAGECOUNT,{'I','M','A','G','E','C','O','U','N','T','\0'}},
    {TMT_INT,TMT_ALPHALEVEL,{'A','L','P','H','A','L','E','V','E','L','\0'}},
    {TMT_INT,TMT_BORDERSIZE,{'B','O','R','D','E','R','S','I','Z','E','\0'}},
    {TMT_INT,TMT_ROUNDCORNERWIDTH,{'R','O','U','N','D','C','O','R','N','E','R','W','I','D','T','H','\0'}},
    {TMT_INT,TMT_ROUNDCORNERHEIGHT,{'R','O','U','N','D','C','O','R','N','E','R','H','E','I','G','H','T','\0'}},
    {TMT_INT,TMT_GRADIENTRATIO1,{'G','R','A','D','I','E','N','T','R','A','T','I','O','1','\0'}},
    {TMT_INT,TMT_GRADIENTRATIO2,{'G','R','A','D','I','E','N','T','R','A','T','I','O','2','\0'}},
    {TMT_INT,TMT_GRADIENTRATIO3,{'G','R','A','D','I','E','N','T','R','A','T','I','O','3','\0'}},
    {TMT_INT,TMT_GRADIENTRATIO4,{'G','R','A','D','I','E','N','T','R','A','T','I','O','4','\0'}},
    {TMT_INT,TMT_GRADIENTRATIO5,{'G','R','A','D','I','E','N','T','R','A','T','I','O','5','\0'}},
    {TMT_INT,TMT_PROGRESSCHUNKSIZE,{'P','R','O','G','R','E','S','S','C','H','U','N','K','S','I','Z','E','\0'}},
    {TMT_INT,TMT_PROGRESSSPACESIZE,{'P','R','O','G','R','E','S','S','S','P','A','C','E','S','I','Z','E','\0'}},
    {TMT_INT,TMT_SATURATION,{'S','A','T','U','R','A','T','I','O','N','\0'}},
    {TMT_INT,TMT_TEXTBORDERSIZE,{'T','E','X','T','B','O','R','D','E','R','S','I','Z','E','\0'}},
    {TMT_INT,TMT_ALPHATHRESHOLD,{'A','L','P','H','A','T','H','R','E','S','H','O','L','D','\0'}},
    {TMT_SIZE,TMT_WIDTH,{'W','I','D','T','H','\0'}},
    {TMT_SIZE,TMT_HEIGHT,{'H','E','I','G','H','T','\0'}},
    {TMT_INT,TMT_GLYPHINDEX,{'G','L','Y','P','H','I','N','D','E','X','\0'}},
    {TMT_INT,TMT_TRUESIZESTRETCHMARK,{'T','R','U','E','S','I','Z','E','S','T','R','E','T','C','H','M','A','R','K','\0'}},
    {TMT_INT,TMT_MINDPI1,{'M','I','N','D','P','I','1','\0'}},
    {TMT_INT,TMT_MINDPI2,{'M','I','N','D','P','I','2','\0'}},
    {TMT_INT,TMT_MINDPI3,{'M','I','N','D','P','I','3','\0'}},
    {TMT_INT,TMT_MINDPI4,{'M','I','N','D','P','I','4','\0'}},
    {TMT_INT,TMT_MINDPI5,{'M','I','N','D','P','I','5','\0'}},
    {TMT_FONT,TMT_GLYPHFONT,{'G','L','Y','P','H','F','O','N','T','\0'}},
    {TMT_FILENAME,TMT_IMAGEFILE,{'I','M','A','G','E','F','I','L','E','\0'}},
    {TMT_FILENAME,TMT_IMAGEFILE1,{'I','M','A','G','E','F','I','L','E','1','\0'}},
    {TMT_FILENAME,TMT_IMAGEFILE2,{'I','M','A','G','E','F','I','L','E','2','\0'}},
    {TMT_FILENAME,TMT_IMAGEFILE3,{'I','M','A','G','E','F','I','L','E','3','\0'}},
    {TMT_FILENAME,TMT_IMAGEFILE4,{'I','M','A','G','E','F','I','L','E','4','\0'}},
    {TMT_FILENAME,TMT_IMAGEFILE5,{'I','M','A','G','E','F','I','L','E','5','\0'}},
    {TMT_FILENAME,TMT_STOCKIMAGEFILE,{'S','T','O','C','K','I','M','A','G','E','F','I','L','E','\0'}},
    {TMT_FILENAME,TMT_GLYPHIMAGEFILE,{'G','L','Y','P','H','I','M','A','G','E','F','I','L','E','\0'}},
    {TMT_STRING,TMT_TEXT,{'T','E','X','T','\0'}},
    {TMT_POSITION,TMT_OFFSET,{'O','F','F','S','E','T','\0'}},
    {TMT_POSITION,TMT_TEXTSHADOWOFFSET,{'T','E','X','T','S','H','A','D','O','W','O','F','F','S','E','T','\0'}},
    {TMT_POSITION,TMT_MINSIZE,{'M','I','N','S','I','Z','E','\0'}},
    {TMT_POSITION,TMT_MINSIZE1,{'M','I','N','S','I','Z','E','1','\0'}},
    {TMT_POSITION,TMT_MINSIZE2,{'M','I','N','S','I','Z','E','2','\0'}},
    {TMT_POSITION,TMT_MINSIZE3,{'M','I','N','S','I','Z','E','3','\0'}},
    {TMT_POSITION,TMT_MINSIZE4,{'M','I','N','S','I','Z','E','4','\0'}},
    {TMT_POSITION,TMT_MINSIZE5,{'M','I','N','S','I','Z','E','5','\0'}},
    {TMT_POSITION,TMT_NORMALSIZE,{'N','O','R','M','A','L','S','I','Z','E','\0'}},
    {TMT_MARGINS,TMT_SIZINGMARGINS,{'S','I','Z','I','N','G','M','A','R','G','I','N','S','\0'}},
    {TMT_MARGINS,TMT_CONTENTMARGINS,{'C','O','N','T','E','N','T','M','A','R','G','I','N','S','\0'}},
    {TMT_MARGINS,TMT_CAPTIONMARGINS,{'C','A','P','T','I','O','N','M','A','R','G','I','N','S','\0'}},
    {TMT_COLOR,TMT_BORDERCOLOR,{'B','O','R','D','E','R','C','O','L','O','R','\0'}},
    {TMT_COLOR,TMT_FILLCOLOR,{'F','I','L','L','C','O','L','O','R','\0'}},
    {TMT_COLOR,TMT_TEXTCOLOR,{'T','E','X','T','C','O','L','O','R','\0'}},
    {TMT_COLOR,TMT_EDGELIGHTCOLOR,{'E','D','G','E','L','I','G','H','T','C','O','L','O','R','\0'}},
    {TMT_COLOR,TMT_EDGEHIGHLIGHTCOLOR,{'E','D','G','E','H','I','G','H','L','I','G','H','T','C','O','L','O','R','\0'}},
    {TMT_COLOR,TMT_EDGESHADOWCOLOR,{'E','D','G','E','S','H','A','D','O','W','C','O','L','O','R','\0'}},
    {TMT_COLOR,TMT_EDGEDKSHADOWCOLOR,{'E','D','G','E','D','K','S','H','A','D','O','W','C','O','L','O','R','\0'}},
    {TMT_COLOR,TMT_EDGEFILLCOLOR,{'E','D','G','E','F','I','L','L','C','O','L','O','R','\0'}},
    {TMT_COLOR,TMT_TRANSPARENTCOLOR,{'T','R','A','N','S','P','A','R','E','N','T','C','O','L','O','R','\0'}},
    {TMT_COLOR,TMT_GRADIENTCOLOR1,{'G','R','A','D','I','E','N','T','C','O','L','O','R','1','\0'}},
    {TMT_COLOR,TMT_GRADIENTCOLOR2,{'G','R','A','D','I','E','N','T','C','O','L','O','R','2','\0'}},
    {TMT_COLOR,TMT_GRADIENTCOLOR3,{'G','R','A','D','I','E','N','T','C','O','L','O','R','3','\0'}},
    {TMT_COLOR,TMT_GRADIENTCOLOR4,{'G','R','A','D','I','E','N','T','C','O','L','O','R','4','\0'}},
    {TMT_COLOR,TMT_GRADIENTCOLOR5,{'G','R','A','D','I','E','N','T','C','O','L','O','R','5','\0'}},
    {TMT_COLOR,TMT_SHADOWCOLOR,{'S','H','A','D','O','W','C','O','L','O','R','\0'}},
    {TMT_COLOR,TMT_GLOWCOLOR,{'G','L','O','W','C','O','L','O','R','\0'}},
    {TMT_COLOR,TMT_TEXTBORDERCOLOR,{'T','E','X','T','B','O','R','D','E','R','C','O','L','O','R','\0'}},
    {TMT_COLOR,TMT_TEXTSHADOWCOLOR,{'T','E','X','T','S','H','A','D','O','W','C','O','L','O','R','\0'}},
    {TMT_COLOR,TMT_GLYPHTEXTCOLOR,{'G','L','Y','P','H','T','E','X','T','C','O','L','O','R','\0'}},
    {TMT_COLOR,TMT_GLYPHTRANSPARENTCOLOR,{'G','L','Y','P','H','T','R','A','N','S','P','A','R','E','N','T','C','O','L','O','R','\0'}},
    {TMT_COLOR,TMT_FILLCOLORHINT,{'F','I','L','L','C','O','L','O','R','H','I','N','T','\0'}},
    {TMT_COLOR,TMT_BORDERCOLORHINT,{'B','O','R','D','E','R','C','O','L','O','R','H','I','N','T','\0'}},
    {TMT_COLOR,TMT_ACCENTCOLORHINT,{'A','C','C','E','N','T','C','O','L','O','R','H','I','N','T','\0'}},
    {TMT_ENUM,TMT_BGTYPE,{'B','G','T','Y','P','E','\0'}},
    {TMT_ENUM,TMT_BORDERTYPE,{'B','O','R','D','E','R','T','Y','P','E','\0'}},
    {TMT_ENUM,TMT_FILLTYPE,{'F','I','L','L','T','Y','P','E','\0'}},
    {TMT_ENUM,TMT_SIZINGTYPE,{'S','I','Z','I','N','G','T','Y','P','E','\0'}},
    {TMT_ENUM,TMT_HALIGN,{'H','A','L','I','G','N','\0'}},
    {TMT_ENUM,TMT_CONTENTALIGNMENT,{'C','O','N','T','E','N','T','A','L','I','G','N','M','E','N','T','\0'}},
    {TMT_ENUM,TMT_VALIGN,{'V','A','L','I','G','N','\0'}},
    {TMT_ENUM,TMT_OFFSETTYPE,{'O','F','F','S','E','T','T','Y','P','E','\0'}},
    {TMT_ENUM,TMT_ICONEFFECT,{'I','C','O','N','E','F','F','E','C','T','\0'}},
    {TMT_ENUM,TMT_TEXTSHADOWTYPE,{'T','E','X','T','S','H','A','D','O','W','T','Y','P','E','\0'}},
    {TMT_ENUM,TMT_IMAGELAYOUT,{'I','M','A','G','E','L','A','Y','O','U','T','\0'}},
    {TMT_ENUM,TMT_GLYPHTYPE,{'G','L','Y','P','H','T','Y','P','E','\0'}},
    {TMT_ENUM,TMT_IMAGESELECTTYPE,{'I','M','A','G','E','S','E','L','E','C','T','T','Y','P','E','\0'}},
    {TMT_ENUM,TMT_GLYPHFONTSIZINGTYPE,{'G','L','Y','P','H','F','O','N','T','S','I','Z','I','N','G','T','Y','P','E','\0'}},
    {TMT_ENUM,TMT_TRUESIZESCALINGTYPE,{'T','R','U','E','S','I','Z','E','S','C','A','L','I','N','G','T','Y','P','E','\0'}},
    {TMT_BOOL,TMT_USERPICTURE,{'U','S','E','R','P','I','C','T','U','R','E','\0'}},
    {TMT_RECT,TMT_DEFAULTPANESIZE,{'D','E','F','A','U','L','T','P','A','N','E','S','I','Z','E','\0'}},
    {TMT_COLOR,TMT_BLENDCOLOR,{'B','L','E','N','D','C','O','L','O','R','\0'}},
    {0,0,{'\0'}}
};

/***********************************************************************
 * Map strings to enumeration values
 * Enum,Value,ValueName
 */
static const MSSTYLES_ENUM_MAP mapEnum[] = {
    {TMT_BGTYPE,BT_IMAGEFILE,{'I','M','A','G','E','F','I','L','E','\0'}},
    {TMT_BGTYPE,BT_BORDERFILL,{'B','O','R','D','E','R','F','I','L','L','\0'}},
    {TMT_BGTYPE,BT_NONE,{'N','O','N','E','\0'}},
    {TMT_IMAGELAYOUT,IL_VERTICAL,{'V','E','R','T','I','C','A','L','\0'}},
    {TMT_IMAGELAYOUT,IL_HORIZONTAL,{'H','O','R','I','Z','O','N','T','A','L','\0'}},
    {TMT_BORDERTYPE,BT_RECT,{'R','E','C','T','\0'}},
    {TMT_BORDERTYPE,BT_ROUNDRECT,{'R','O','U','N','D','R','E','C','T','\0'}},
    {TMT_BORDERTYPE,BT_ELLIPSE,{'E','L','L','I','P','S','E','\0'}},
    {TMT_FILLTYPE,FT_SOLID,{'S','O','L','I','D','\0'}},
    {TMT_FILLTYPE,FT_VERTGRADIENT,{'V','E','R','T','G','R','A','D','I','E','N','T','\0'}},
    {TMT_FILLTYPE,FT_HORZGRADIENT,{'H','O','R','Z','G','R','A','D','I','E','N','T','\0'}},
    {TMT_FILLTYPE,FT_RADIALGRADIENT,{'R','A','D','I','A','L','G','R','A','D','I','E','N','T','\0'}},
    {TMT_FILLTYPE,FT_TILEIMAGE,{'T','I','L','E','I','M','A','G','E','\0'}},
    {TMT_SIZINGTYPE,ST_TRUESIZE,{'T','R','U','E','S','I','Z','E','\0'}},
    {TMT_SIZINGTYPE,ST_STRETCH,{'S','T','R','E','T','C','H','\0'}},
    {TMT_SIZINGTYPE,ST_TILE,{'T','I','L','E','\0'}},
    {TMT_HALIGN,HA_LEFT,{'L','E','F','T','\0'}},
    {TMT_HALIGN,HA_CENTER,{'C','E','N','T','E','R','\0'}},
    {TMT_HALIGN,HA_RIGHT,{'R','I','G','H','T','\0'}},
    {TMT_CONTENTALIGNMENT,CA_LEFT,{'L','E','F','T','\0'}},
    {TMT_CONTENTALIGNMENT,CA_CENTER,{'C','E','N','T','E','R','\0'}},
    {TMT_CONTENTALIGNMENT,CA_RIGHT,{'R','I','G','H','T','\0'}},
    {TMT_VALIGN,VA_TOP,{'T','O','P','\0'}},
    {TMT_VALIGN,VA_CENTER,{'C','E','N','T','E','R','\0'}},
    {TMT_VALIGN,VA_BOTTOM,{'B','O','T','T','O','M','\0'}},
    {TMT_OFFSETTYPE,OT_TOPLEFT,{'T','O','P','L','E','F','T','\0'}},
    {TMT_OFFSETTYPE,OT_TOPRIGHT,{'T','O','P','R','I','G','H','T','\0'}},
    {TMT_OFFSETTYPE,OT_TOPMIDDLE,{'T','O','P','M','I','D','D','L','E','\0'}},
    {TMT_OFFSETTYPE,OT_BOTTOMLEFT,{'B','O','T','T','O','M','L','E','F','T','\0'}},
    {TMT_OFFSETTYPE,OT_BOTTOMRIGHT,{'B','O','T','T','O','M','R','I','G','H','T','\0'}},
    {TMT_OFFSETTYPE,OT_BOTTOMMIDDLE,{'B','O','T','T','O','M','M','I','D','D','L','E','\0'}},
    {TMT_OFFSETTYPE,OT_MIDDLELEFT,{'M','I','D','D','L','E','L','E','F','T','\0'}},
    {TMT_OFFSETTYPE,OT_MIDDLERIGHT,{'M','I','D','D','L','E','R','I','G','H','T','\0'}},
    {TMT_OFFSETTYPE,OT_LEFTOFCAPTION,{'L','E','F','T','O','F','C','A','P','T','I','O','N','\0'}},
    {TMT_OFFSETTYPE,OT_RIGHTOFCAPTION,{'R','I','G','H','T','O','F','C','A','P','T','I','O','N','\0'}},
    {TMT_OFFSETTYPE,OT_LEFTOFLASTBUTTON,{'L','E','F','T','O','F','L','A','S','T','B','U','T','T','O','N','\0'}},
    {TMT_OFFSETTYPE,OT_RIGHTOFLASTBUTTON,{'R','I','G','H','T','O','F','L','A','S','T','B','U','T','T','O','N','\0'}},
    {TMT_OFFSETTYPE,OT_ABOVELASTBUTTON,{'A','B','O','V','E','L','A','S','T','B','U','T','T','O','N','\0'}},
    {TMT_OFFSETTYPE,OT_BELOWLASTBUTTON,{'B','E','L','O','W','L','A','S','T','B','U','T','T','O','N','\0'}},
    {TMT_ICONEFFECT,ICE_NONE,{'N','O','N','E','\0'}},
    {TMT_ICONEFFECT,ICE_GLOW,{'G','L','O','W','\0'}},
    {TMT_ICONEFFECT,ICE_SHADOW,{'S','H','A','D','O','W','\0'}},
    {TMT_ICONEFFECT,ICE_PULSE,{'P','U','L','S','E','\0'}},
    {TMT_ICONEFFECT,ICE_ALPHA,{'A','L','P','H','A','\0'}},
    {TMT_TEXTSHADOWTYPE,TST_NONE,{'N','O','N','E','\0'}},
    {TMT_TEXTSHADOWTYPE,TST_SINGLE,{'S','I','N','G','L','E','\0'}},
    {TMT_TEXTSHADOWTYPE,TST_CONTINUOUS,{'C','O','N','T','I','N','U','O','U','S','\0'}},
    {TMT_GLYPHTYPE,GT_NONE,{'N','O','N','E','\0'}},
    {TMT_GLYPHTYPE,GT_IMAGEGLYPH,{'I','M','A','G','E','G','L','Y','P','H','\0'}},
    {TMT_GLYPHTYPE,GT_FONTGLYPH,{'F','O','N','T','G','L','Y','P','H','\0'}},
    {TMT_IMAGESELECTTYPE,IST_NONE,{'N','O','N','E','\0'}},
    {TMT_IMAGESELECTTYPE,IST_SIZE,{'S','I','Z','E','\0'}},
    {TMT_IMAGESELECTTYPE,IST_DPI,{'D','P','I','\0'}},
    {TMT_TRUESIZESCALINGTYPE,TSST_NONE,{'N','O','N','E','\0'}},
    {TMT_TRUESIZESCALINGTYPE,TSST_SIZE,{'S','I','Z','E','\0'}},
    {TMT_TRUESIZESCALINGTYPE,TSST_DPI,{'D','P','I','\0'}},
    {TMT_GLYPHFONTSIZINGTYPE,GFST_NONE,{'N','O','N','E','\0'}},
    {TMT_GLYPHFONTSIZINGTYPE,GFST_SIZE,{'S','I','Z','E','\0'}},
    {TMT_GLYPHFONTSIZINGTYPE,GFST_DPI,{'D','P','I','\0'}},
    {0,0,{'\0'}}
};


/***********************************************************************
 * Classes defined below
 * Defined as PartID,StateID,TextName
 * If StateID == 0 then its a part being defined
 */

/* These are globals to all classes, but its treated as a separate class */
static const MSSTYLES_CLASS_MAP classGlobals[] = {
    {GP_BORDER,0,{'B','O','R','D','E','R','\0'}},
    {GP_BORDER,BSS_FLAT,{'F','L','A','T','\0'}},
    {GP_BORDER,BSS_RAISED,{'R','A','I','S','E','D','\0'}},
    {GP_BORDER,BSS_SUNKEN,{'S','U','N','K','E','N','\0'}},
    {GP_LINEHORZ,0,{'L','I','N','E','H','O','R','Z','\0'}},
    {GP_LINEHORZ,LHS_FLAT,{'F','L','A','T','\0'}},
    {GP_LINEHORZ,LHS_RAISED,{'R','A','I','S','E','D','\0'}},
    {GP_LINEHORZ,LHS_SUNKEN,{'S','U','N','K','E','N','\0'}},
    {GP_LINEVERT,0,{'L','I','N','E','V','E','R','T','\0'}},
    {GP_LINEVERT,LVS_FLAT,{'F','L','A','T','\0'}},
    {GP_LINEVERT,LVS_RAISED,{'R','A','I','S','E','D','\0'}},
    {GP_LINEVERT,LVS_SUNKEN,{'S','U','N','K','E','N','\0'}},
    {0,0,{'\0'}}
};

static const MSSTYLES_CLASS_MAP classButton[] = {
    {BP_PUSHBUTTON,0,{'P','U','S','H','B','U','T','T','O','N','\0'}},
    {BP_PUSHBUTTON,PBS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {BP_PUSHBUTTON,PBS_HOT,{'H','O','T','\0'}},
    {BP_PUSHBUTTON,PBS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {BP_PUSHBUTTON,PBS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {BP_PUSHBUTTON,PBS_DEFAULTED,{'D','E','F','A','U','L','T','E','D','\0'}},
    {BP_RADIOBUTTON,0,{'R','A','D','I','O','B','U','T','T','O','N','\0'}},
    {BP_RADIOBUTTON,RBS_UNCHECKEDNORMAL,{'U','N','C','H','E','C','K','E','D','N','O','R','M','A','L','\0'}},
    {BP_RADIOBUTTON,RBS_UNCHECKEDHOT,{'U','N','C','H','E','C','K','E','D','H','O','T','\0'}},
    {BP_RADIOBUTTON,RBS_UNCHECKEDPRESSED,{'U','N','C','H','E','C','K','E','D','P','R','E','S','S','E','D','\0'}},
    {BP_RADIOBUTTON,RBS_UNCHECKEDDISABLED,{'U','N','C','H','E','C','K','E','D','D','I','S','A','B','L','E','D','\0'}},
    {BP_RADIOBUTTON,RBS_CHECKEDNORMAL,{'C','H','E','C','K','E','D','N','O','R','M','A','L','\0'}},
    {BP_RADIOBUTTON,RBS_CHECKEDHOT,{'C','H','E','C','K','E','D','H','O','T','\0'}},
    {BP_RADIOBUTTON,RBS_CHECKEDPRESSED,{'C','H','E','C','K','E','D','P','R','E','S','S','E','D','\0'}},
    {BP_RADIOBUTTON,RBS_CHECKEDDISABLED,{'C','H','E','C','K','E','D','D','I','S','A','B','L','E','D','\0'}},
    {BP_CHECKBOX,0,{'C','H','E','C','K','B','O','X','\0'}},
    {BP_RADIOBUTTON,CBS_UNCHECKEDNORMAL,{'U','N','C','H','E','C','K','E','D','N','O','R','M','A','L','\0'}},
    {BP_RADIOBUTTON,CBS_UNCHECKEDHOT,{'U','N','C','H','E','C','K','E','D','H','O','T','\0'}},
    {BP_RADIOBUTTON,CBS_UNCHECKEDPRESSED,{'U','N','C','H','E','C','K','E','D','P','R','E','S','S','E','D','\0'}},
    {BP_RADIOBUTTON,CBS_UNCHECKEDDISABLED,{'U','N','C','H','E','C','K','E','D','D','I','S','A','B','L','E','D','\0'}},
    {BP_RADIOBUTTON,CBS_CHECKEDNORMAL,{'C','H','E','C','K','E','D','N','O','R','M','A','L','\0'}},
    {BP_RADIOBUTTON,CBS_CHECKEDHOT,{'C','H','E','C','K','E','D','H','O','T','\0'}},
    {BP_RADIOBUTTON,CBS_CHECKEDPRESSED,{'C','H','E','C','K','E','D','P','R','E','S','S','E','D','\0'}},
    {BP_RADIOBUTTON,CBS_CHECKEDDISABLED,{'C','H','E','C','K','E','D','D','I','S','A','B','L','E','D','\0'}},
    {BP_RADIOBUTTON,CBS_MIXEDNORMAL,{'M','I','X','E','D','N','O','R','M','A','L','\0'}},
    {BP_RADIOBUTTON,CBS_MIXEDHOT,{'M','I','X','E','D','H','O','T','\0'}},
    {BP_RADIOBUTTON,CBS_MIXEDPRESSED,{'M','I','X','E','D','P','R','E','S','S','E','D','\0'}},
    {BP_RADIOBUTTON,CBS_MIXEDDISABLED,{'M','I','X','E','D','D','I','S','A','B','L','E','D','\0'}},
    {BP_GROUPBOX,0,{'G','R','O','U','P','B','O','X','\0'}},
    {BP_RADIOBUTTON,GBS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {BP_RADIOBUTTON,GBS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {BP_USERBUTTON,0,{'U','S','E','R','B','U','T','T','O','N','\0'}},
    {0,0,{'\0'}}
};

static const MSSTYLES_CLASS_MAP classClock[] = {
    {CLP_TIME,0,{'T','I','M','E','\0'}},
    {CLP_TIME,CLS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {0,0,{'\0'}}
};

static const MSSTYLES_CLASS_MAP classCombobox[] = {
    {CP_DROPDOWNBUTTON,0,{'D','R','O','P','D','O','W','N','B','U','T','T','O','N','\0'}},
    {CP_DROPDOWNBUTTON,CBXS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {CP_DROPDOWNBUTTON,CBXS_HOT,{'H','O','T','\0'}},
    {CP_DROPDOWNBUTTON,CBXS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {CP_DROPDOWNBUTTON,CBXS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {0,0,{'\0'}}
};

static const MSSTYLES_CLASS_MAP classEdit[] = {
    {EP_EDITTEXT,0,{'E','D','I','T','T','E','X','T','\0'}},
    {EP_EDITTEXT,ETS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {EP_EDITTEXT,ETS_HOT,{'H','O','T','\0'}},
    {EP_EDITTEXT,ETS_SELECTED,{'S','E','L','E','C','T','E','D','\0'}},
    {EP_EDITTEXT,ETS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {EP_EDITTEXT,ETS_FOCUSED,{'F','O','C','U','S','E','D','\0'}},
    {EP_EDITTEXT,ETS_READONLY,{'R','E','A','D','O','N','L','Y','\0'}},
    {EP_EDITTEXT,ETS_ASSIST,{'A','S','S','I','S','T','\0'}},
    {EP_CARET,0,{'C','A','R','E','T','\0'}},
    {0,0,{'\0'}}
};

static const MSSTYLES_CLASS_MAP classExplorerbar[] = {
    {EBP_HEADERBACKGROUND,0,{'H','E','A','D','E','R','B','A','C','K','G','R','O','U','N','D','\0'}},
    {EBP_HEADERCLOSE,0,{'H','E','A','D','E','R','C','L','O','S','E','\0'}},
    {EBP_HEADERCLOSE,EBHC_NORMAL,{'N','O','R','M','A','L','\0'}},
    {EBP_HEADERCLOSE,EBHC_HOT,{'H','O','T','\0'}},
    {EBP_HEADERCLOSE,EBHC_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {EBP_HEADERPIN,0,{'H','E','A','D','E','R','P','I','N','\0'}},
    {EBP_HEADERPIN,EBHP_NORMAL,{'N','O','R','M','A','L','\0'}},
    {EBP_HEADERPIN,EBHP_HOT,{'H','O','T','\0'}},
    {EBP_HEADERPIN,EBHP_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {EBP_HEADERPIN,EBHP_SELECTEDNORMAL,{'S','E','L','E','C','T','E','D','N','O','R','M','A','L','\0'}},
    {EBP_HEADERPIN,EBHP_SELECTEDHOT,{'S','E','L','E','C','T','E','D','H','O','T','\0'}},
    {EBP_HEADERPIN,EBHP_SELECTEDPRESSED,{'S','E','L','E','C','T','E','D','P','R','E','S','S','E','D','\0'}},
    {EBP_IEBARMENU,0,{'I','E','B','A','R','M','E','N','U','\0'}},
    {EBP_IEBARMENU,EBM_NORMAL,{'N','O','R','M','A','L','\0'}},
    {EBP_IEBARMENU,EBM_HOT,{'H','O','T','\0'}},
    {EBP_IEBARMENU,EBM_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {EBP_NORMALGROUPBACKGROUND,0,{'N','O','R','M','A','L','G','R','O','U','P','B','A','C','K','G','R','O','U','N','D','\0'}},
    {EBP_NORMALGROUPCOLLAPSE,0,{'N','O','R','M','A','L','G','R','O','U','P','C','O','L','L','A','P','S','E','\0'}},
    {EBP_NORMALGROUPCOLLAPSE,EBNGC_NORMAL,{'N','O','R','M','A','L','\0'}},
    {EBP_NORMALGROUPCOLLAPSE,EBNGC_HOT,{'H','O','T','\0'}},
    {EBP_NORMALGROUPCOLLAPSE,EBNGC_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {EBP_NORMALGROUPEXPAND,0,{'N','O','R','M','A','L','G','R','O','U','P','E','X','P','A','N','D','\0'}},
    {EBP_NORMALGROUPEXPAND,EBNGE_NORMAL,{'N','O','R','M','A','L','\0'}},
    {EBP_NORMALGROUPEXPAND,EBNGE_HOT,{'H','O','T','\0'}},
    {EBP_NORMALGROUPEXPAND,EBNGE_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {EBP_NORMALGROUPHEAD,0,{'N','O','R','M','A','L','G','R','O','U','P','H','E','A','D','\0'}},
    {EBP_SPECIALGROUPBACKGROUND,0,{'S','P','E','C','I','A','L','G','R','O','U','P','B','A','C','K','G','R','O','U','N','D','\0'}},
    {EBP_SPECIALGROUPCOLLAPSE,0,{'S','P','E','C','I','A','L','G','R','O','U','P','C','O','L','L','A','P','S','E','\0'}},
    {EBP_SPECIALGROUPCOLLAPSE,EBSGC_NORMAL,{'N','O','R','M','A','L','\0'}},
    {EBP_SPECIALGROUPCOLLAPSE,EBSGC_HOT,{'H','O','T','\0'}},
    {EBP_SPECIALGROUPCOLLAPSE,EBSGC_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {EBP_SPECIALGROUPEXPAND,0,{'S','P','E','C','I','A','L','G','R','O','U','P','E','X','P','A','N','D','\0'}},
    {EBP_SPECIALGROUPEXPAND,EBSGE_NORMAL,{'N','O','R','M','A','L','\0'}},
    {EBP_SPECIALGROUPEXPAND,EBSGE_HOT,{'H','O','T','\0'}},
    {EBP_SPECIALGROUPEXPAND,EBSGE_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {EBP_SPECIALGROUPHEAD,0,{'S','P','E','C','I','A','L','G','R','O','U','P','H','E','A','D','\0'}},
    {0,0,{'\0'}}
};

static const MSSTYLES_CLASS_MAP classHeader[] = {
    {HP_HEADERITEM,0,{'H','E','A','D','E','R','I','T','E','M','\0'}},
    {HP_HEADERITEM,HIS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {HP_HEADERITEM,HIS_HOT,{'H','O','T','\0'}},
    {HP_HEADERITEM,HIS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {HP_HEADERITEMLEFT,0,{'H','E','A','D','E','R','I','T','E','M','L','E','F','T','\0'}},
    {HP_HEADERITEMLEFT,HILS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {HP_HEADERITEMLEFT,HILS_HOT,{'H','O','T','\0'}},
    {HP_HEADERITEMLEFT,HILS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {HP_HEADERITEMRIGHT,0,{'H','E','A','D','E','R','I','T','E','M','R','I','G','H','T','\0'}},
    {HP_HEADERITEMRIGHT,HIRS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {HP_HEADERITEMRIGHT,HIRS_HOT,{'H','O','T','\0'}},
    {HP_HEADERITEMRIGHT,HIRS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {HP_HEADERSORTARROW,0,{'H','E','A','D','E','R','S','O','R','T','A','R','R','O','W','\0'}},
    {HP_HEADERSORTARROW,HSAS_SORTEDUP,{'S','O','R','T','E','D','U','P','\0'}},
    {HP_HEADERSORTARROW,HSAS_SORTEDDOWN,{'S','O','R','T','E','D','D','O','W','N','\0'}},
    {0,0,{'\0'}}
};

static const MSSTYLES_CLASS_MAP classListview[] = {
    {LVP_LISTITEM,0,{'L','I','S','T','I','T','E','M','\0'}},
    {LVP_LISTITEM,LIS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {LVP_LISTITEM,LIS_HOT,{'H','O','T','\0'}},
    {LVP_LISTITEM,LIS_SELECTED,{'S','E','L','E','C','T','E','D','\0'}},
    {LVP_LISTITEM,LIS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {LVP_LISTITEM,LIS_SELECTEDNOTFOCUS,{'S','E','L','E','C','T','E','D','N','O','T','F','O','C','U','S','\0'}},
    {LVP_LISTGROUP,0,{'L','I','S','T','G','R','O','U','P','\0'}},
    {LVP_LISTDETAIL,0,{'L','I','S','T','D','E','T','A','I','L','\0'}},
    {LVP_LISTSORTEDDETAIL,0,{'L','I','S','T','S','O','R','T','E','D','D','E','T','A','I','L','\0'}},
    {LVP_EMPTYTEXT,0,{'E','M','P','T','Y','T','E','X','T','\0'}},
    {0,0,{'\0'}}
};

static const MSSTYLES_CLASS_MAP classMenu[] = {
    {MP_MENUITEM,0,{'M','E','N','U','I','T','E','M','\0'}},
    {MP_MENUITEM,MS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {MP_MENUITEM,MS_SELECTED,{'S','E','L','E','C','T','E','D','\0'}},
    {MP_MENUITEM,MS_DEMOTED,{'D','E','M','O','T','E','D','\0'}},
    {MP_MENUDROPDOWN,0,{'M','E','N','U','D','R','O','P','D','O','W','N','\0'}},
    {MP_MENUDROPDOWN,MS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {MP_MENUDROPDOWN,MS_SELECTED,{'S','E','L','E','C','T','E','D','\0'}},
    {MP_MENUDROPDOWN,MS_DEMOTED,{'D','E','M','O','T','E','D','\0'}},
    {MP_MENUBARITEM,0,{'M','E','N','U','B','A','R','I','T','E','M','\0'}},
    {MP_MENUBARITEM,MS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {MP_MENUBARITEM,MS_SELECTED,{'S','E','L','E','C','T','E','D','\0'}},
    {MP_MENUBARITEM,MS_DEMOTED,{'D','E','M','O','T','E','D','\0'}},
    {MP_MENUBARDROPDOWN,0,{'M','E','N','U','B','A','R','D','R','O','P','D','O','W','N','\0'}},
    {MP_MENUBARDROPDOWN,MS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {MP_MENUBARDROPDOWN,MS_SELECTED,{'S','E','L','E','C','T','E','D','\0'}},
    {MP_MENUBARDROPDOWN,MS_DEMOTED,{'D','E','M','O','T','E','D','\0'}},
    {MP_CHEVRON,0,{'C','H','E','V','R','O','N','\0'}},
    {MP_CHEVRON,MS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {MP_CHEVRON,MS_SELECTED,{'S','E','L','E','C','T','E','D','\0'}},
    {MP_CHEVRON,MS_DEMOTED,{'D','E','M','O','T','E','D','\0'}},
    {MP_SEPARATOR,0,{'S','E','P','A','R','A','T','O','R','\0'}},
    {MP_SEPARATOR,MS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {MP_SEPARATOR,MS_SELECTED,{'S','E','L','E','C','T','E','D','\0'}},
    {MP_SEPARATOR,MS_DEMOTED,{'D','E','M','O','T','E','D','\0'}},
    {0,0,{'\0'}}
};

static const MSSTYLES_CLASS_MAP classMenuband[] = {
    {MDP_NEWAPPBUTTON,0,{'N','E','W','A','P','P','B','U','T','T','O','N','\0'}},
    {MDP_NEWAPPBUTTON,MDS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {MDP_NEWAPPBUTTON,MDS_HOT,{'H','O','T','\0'}},
    {MDP_NEWAPPBUTTON,MDS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {MDP_NEWAPPBUTTON,MDS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {MDP_NEWAPPBUTTON,MDS_CHECKED,{'C','H','E','C','K','E','D','\0'}},
    {MDP_NEWAPPBUTTON,MDS_HOTCHECKED,{'H','O','T','C','H','E','C','K','E','D','\0'}},
    {MDP_SEPERATOR,0,{'S','E','P','E','R','A','T','O','R','\0'}},
    {0,0,{'\0'}}
};

static const MSSTYLES_CLASS_MAP classPage[] = {
    {PGRP_UP,0,{'U','P','\0'}},
    {PGRP_UP,UPS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {PGRP_UP,UPS_HOT,{'H','O','T','\0'}},
    {PGRP_UP,UPS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {PGRP_UP,UPS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {PGRP_DOWN,0,{'D','O','W','N','\0'}},
    {PGRP_DOWN,DNS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {PGRP_DOWN,DNS_HOT,{'H','O','T','\0'}},
    {PGRP_DOWN,DNS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {PGRP_DOWN,DNS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {PGRP_UPHORZ,0,{'U','P','H','O','R','Z','\0'}},
    {PGRP_UPHORZ,UPHZS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {PGRP_UPHORZ,UPHZS_HOT,{'H','O','T','\0'}},
    {PGRP_UPHORZ,UPHZS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {PGRP_UPHORZ,UPHZS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {PGRP_DOWNHORZ,0,{'D','O','W','N','H','O','R','Z','\0'}},
    {PGRP_DOWNHORZ,DNHZS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {PGRP_DOWNHORZ,DNHZS_HOT,{'H','O','T','\0'}},
    {PGRP_DOWNHORZ,DNHZS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {PGRP_DOWNHORZ,DNHZS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {0,0,{'\0'}}
};

static const MSSTYLES_CLASS_MAP classProgress[] = {
    {PP_BAR,0,{'B','A','R','\0'}},
    {PP_BARVERT,0,{'B','A','R','V','E','R','T','\0'}},
    {PP_CHUNK,0,{'C','H','U','N','K','\0'}},
    {PP_CHUNKVERT,0,{'C','H','U','N','K','V','E','R','T','\0'}},
    {0,0,{'\0'}}
};

static const MSSTYLES_CLASS_MAP classRebar[] = {
    {RP_GRIPPER,0,{'G','R','I','P','P','E','R','\0'}},
    {RP_GRIPPERVERT,0,{'G','R','I','P','P','E','R','V','E','R','T','\0'}},
    {RP_BAND,0,{'B','A','N','D','\0'}},
    {RP_CHEVRON,0,{'C','H','E','V','R','O','N','\0'}},
    {RP_CHEVRON,CHEVS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {RP_CHEVRON,CHEVS_HOT,{'H','O','T','\0'}},
    {RP_CHEVRON,CHEVS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {RP_CHEVRONVERT,0,{'C','H','E','V','R','O','N','V','E','R','T','\0'}},
    {0,0,{'\0'}}
};

static const MSSTYLES_CLASS_MAP classScrollbar[] = {
    {SBP_ARROWBTN,0,{'A','R','R','O','W','B','T','N','\0'}},
    {SBP_ARROWBTN,ABS_UPNORMAL,{'U','P','N','O','R','M','A','L','\0'}},
    {SBP_ARROWBTN,ABS_UPHOT,{'U','P','H','O','T','\0'}},
    {SBP_ARROWBTN,ABS_UPPRESSED,{'U','P','P','R','E','S','S','E','D','\0'}},
    {SBP_ARROWBTN,ABS_UPDISABLED,{'U','P','D','I','S','A','B','L','E','D','\0'}},
    {SBP_ARROWBTN,ABS_DOWNNORMAL,{'D','O','W','N','N','O','R','M','A','L','\0'}},
    {SBP_ARROWBTN,ABS_DOWNHOT,{'D','O','W','N','H','O','T','\0'}},
    {SBP_ARROWBTN,ABS_DOWNPRESSED,{'D','O','W','N','P','R','E','S','S','E','D','\0'}},
    {SBP_ARROWBTN,ABS_DOWNDISABLED,{'D','O','W','N','D','I','S','A','B','L','E','D','\0'}},
    {SBP_ARROWBTN,ABS_LEFTNORMAL,{'L','E','F','T','N','O','R','M','A','L','\0'}},
    {SBP_ARROWBTN,ABS_LEFTHOT,{'L','E','F','T','H','O','T','\0'}},
    {SBP_ARROWBTN,ABS_LEFTPRESSED,{'L','E','F','T','P','R','E','S','S','E','D','\0'}},
    {SBP_ARROWBTN,ABS_LEFTDISABLED,{'L','E','F','T','D','I','S','A','B','L','E','D','\0'}},
    {SBP_ARROWBTN,ABS_RIGHTNORMAL,{'R','I','G','H','T','N','O','R','M','A','L','\0'}},
    {SBP_ARROWBTN,ABS_RIGHTHOT,{'R','I','G','H','T','H','O','T','\0'}},
    {SBP_ARROWBTN,ABS_RIGHTPRESSED,{'R','I','G','H','T','P','R','E','S','S','E','D','\0'}},
    {SBP_ARROWBTN,ABS_RIGHTDISABLED,{'R','I','G','H','T','D','I','S','A','B','L','E','D','\0'}},
    {SBP_THUMBBTNHORZ,0,{'T','H','U','M','B','B','T','N','H','O','R','Z','\0'}},
    {SBP_THUMBBTNHORZ,SCRBS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {SBP_THUMBBTNHORZ,SCRBS_HOT,{'H','O','T','\0'}},
    {SBP_THUMBBTNHORZ,SCRBS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {SBP_THUMBBTNHORZ,SCRBS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {SBP_THUMBBTNVERT,0,{'T','H','U','M','B','B','T','N','V','E','R','T','\0'}},
    {SBP_THUMBBTNVERT,SCRBS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {SBP_THUMBBTNVERT,SCRBS_HOT,{'H','O','T','\0'}},
    {SBP_THUMBBTNVERT,SCRBS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {SBP_THUMBBTNVERT,SCRBS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {SBP_LOWERTRACKHORZ,0,{'L','O','W','E','R','T','R','A','C','K','H','O','R','Z','\0'}},
    {SBP_LOWERTRACKHORZ,SCRBS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {SBP_LOWERTRACKHORZ,SCRBS_HOT,{'H','O','T','\0'}},
    {SBP_LOWERTRACKHORZ,SCRBS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {SBP_LOWERTRACKHORZ,SCRBS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {SBP_UPPERTRACKHORZ,0,{'U','P','P','E','R','T','R','A','C','K','H','O','R','Z','\0'}},
    {SBP_UPPERTRACKHORZ,SCRBS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {SBP_UPPERTRACKHORZ,SCRBS_HOT,{'H','O','T','\0'}},
    {SBP_UPPERTRACKHORZ,SCRBS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {SBP_UPPERTRACKHORZ,SCRBS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {SBP_LOWERTRACKVERT,0,{'L','O','W','E','R','T','R','A','C','K','V','E','R','T','\0'}},
    {SBP_LOWERTRACKVERT,SCRBS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {SBP_LOWERTRACKVERT,SCRBS_HOT,{'H','O','T','\0'}},
    {SBP_LOWERTRACKVERT,SCRBS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {SBP_LOWERTRACKVERT,SCRBS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {SBP_UPPERTRACKVERT,0,{'U','P','P','E','R','T','R','A','C','K','V','E','R','T','\0'}},
    {SBP_UPPERTRACKVERT,SCRBS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {SBP_UPPERTRACKVERT,SCRBS_HOT,{'H','O','T','\0'}},
    {SBP_UPPERTRACKVERT,SCRBS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {SBP_UPPERTRACKVERT,SCRBS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {SBP_GRIPPERHORZ,0,{'G','R','I','P','P','E','R','H','O','R','Z','\0'}},
    {SBP_GRIPPERVERT,0,{'G','R','I','P','P','E','R','V','E','R','T','\0'}},
    {SBP_SIZEBOX,0,{'S','I','Z','E','B','O','X','\0'}},
    {SBP_SIZEBOX,SZB_RIGHTALIGN,{'R','I','G','H','T','A','L','I','G','N','\0'}},
    {SBP_SIZEBOX,SZB_LEFTALIGN,{'L','E','F','T','A','L','I','G','N','\0'}},
    {0,0,{'\0'}}
};

static const MSSTYLES_CLASS_MAP classSpin[] = {
    {SPNP_UP,0,{'U','P','\0'}},
    {SPNP_UP,UPS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {SPNP_UP,UPS_HOT,{'H','O','T','\0'}},
    {SPNP_UP,UPS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {SPNP_UP,UPS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {SPNP_DOWN,0,{'D','O','W','N','\0'}},
    {SPNP_DOWN,DNS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {SPNP_DOWN,DNS_HOT,{'H','O','T','\0'}},
    {SPNP_DOWN,DNS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {SPNP_DOWN,DNS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {SPNP_UPHORZ,0,{'U','P','H','O','R','Z','\0'}},
    {SPNP_UPHORZ,UPHZS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {SPNP_UPHORZ,UPHZS_HOT,{'H','O','T','\0'}},
    {SPNP_UPHORZ,UPHZS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {SPNP_UPHORZ,UPHZS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {SPNP_DOWNHORZ,0,{'D','O','W','N','H','O','R','Z','\0'}},
    {SPNP_DOWNHORZ,DNHZS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {SPNP_DOWNHORZ,DNHZS_HOT,{'H','O','T','\0'}},
    {SPNP_DOWNHORZ,DNHZS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {SPNP_DOWNHORZ,DNHZS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {0,0,{'\0'}}
};

static const MSSTYLES_CLASS_MAP classStartpanel[] = {
    {SPP_USERPANE,0,{'U','S','E','R','P','A','N','E','\0'}},
    {SPP_MOREPROGRAMS,0,{'M','O','R','E','P','R','O','G','R','A','M','S','\0'}},
    {SPP_MOREPROGRAMSARROW,0,{'M','O','R','E','P','R','O','G','R','A','M','S','A','R','R','O','W','\0'}},
    {SPP_MOREPROGRAMSARROW,SPS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {SPP_MOREPROGRAMSARROW,SPS_HOT,{'H','O','T','\0'}},
    {SPP_MOREPROGRAMSARROW,SPS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {SPP_PROGLIST,0,{'P','R','O','G','L','I','S','T','\0'}},
    {SPP_PROGLISTSEPARATOR,0,{'P','R','O','G','L','I','S','T','S','E','P','A','R','A','T','O','R','\0'}},
    {SPP_PLACESLIST,0,{'P','L','A','C','E','S','L','I','S','T','\0'}},
    {SPP_PLACESLISTSEPARATOR,0,{'P','L','A','C','E','S','L','I','S','T','S','E','P','A','R','A','T','O','R','\0'}},
    {SPP_LOGOFF,0,{'L','O','G','O','F','F','\0'}},
    {SPP_LOGOFFBUTTONS,0,{'L','O','G','O','F','F','B','U','T','T','O','N','S','\0'}},
    {SPP_LOGOFFBUTTONS,SPLS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {SPP_LOGOFFBUTTONS,SPLS_HOT,{'H','O','T','\0'}},
    {SPP_LOGOFFBUTTONS,SPLS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {SPP_USERPICTURE,0,{'U','S','E','R','P','I','C','T','U','R','E','\0'}},
    {SPP_PREVIEW,0,{'P','R','E','V','I','E','W','\0'}},
    {0,0,{'\0'}}
};

static const MSSTYLES_CLASS_MAP classStatus[] = {
    {SP_PANE,0,{'P','A','N','E','\0'}},
    {SP_GRIPPERPANE,0,{'G','R','I','P','P','E','R','P','A','N','E','\0'}},
    {SP_GRIPPER,0,{'G','R','I','P','P','E','R','\0'}},
    {0,0,{'\0'}}
};

static const MSSTYLES_CLASS_MAP classTab[] = {
    {TABP_TABITEM,0,{'T','A','B','I','T','E','M','\0'}},
    {TABP_TABITEM,TIS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {TABP_TABITEM,TIS_HOT,{'H','O','T','\0'}},
    {TABP_TABITEM,TIS_SELECTED,{'S','E','L','E','C','T','E','D','\0'}},
    {TABP_TABITEM,TIS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {TABP_TABITEM,TIS_FOCUSED,{'F','O','C','U','S','E','D','\0'}},
    {TABP_TABITEMLEFTEDGE,0,{'T','A','B','I','T','E','M','L','E','F','T','E','D','G','E','\0'}},
    {TABP_TABITEMLEFTEDGE,TILES_NORMAL,{'N','O','R','M','A','L','\0'}},
    {TABP_TABITEMLEFTEDGE,TILES_HOT,{'H','O','T','\0'}},
    {TABP_TABITEMLEFTEDGE,TILES_SELECTED,{'S','E','L','E','C','T','E','D','\0'}},
    {TABP_TABITEMLEFTEDGE,TILES_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {TABP_TABITEMLEFTEDGE,TILES_FOCUSED,{'F','O','C','U','S','E','D','\0'}},
    {TABP_TABITEMRIGHTEDGE,0,{'T','A','B','I','T','E','M','R','I','G','H','T','E','D','G','E','\0'}},
    {TABP_TABITEMRIGHTEDGE,TIRES_NORMAL,{'N','O','R','M','A','L','\0'}},
    {TABP_TABITEMRIGHTEDGE,TIRES_HOT,{'H','O','T','\0'}},
    {TABP_TABITEMRIGHTEDGE,TIRES_SELECTED,{'S','E','L','E','C','T','E','D','\0'}},
    {TABP_TABITEMRIGHTEDGE,TIRES_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {TABP_TABITEMRIGHTEDGE,TIRES_FOCUSED,{'F','O','C','U','S','E','D','\0'}},
    {TABP_TABITEMBOTHEDGE,0,{'T','A','B','I','T','E','M','B','O','T','H','E','D','G','E','\0'}},
    {TABP_TABITEMBOTHEDGE,TIBES_NORMAL,{'N','O','R','M','A','L','\0'}},
    {TABP_TABITEMBOTHEDGE,TIBES_HOT,{'H','O','T','\0'}},
    {TABP_TABITEMBOTHEDGE,TIBES_SELECTED,{'S','E','L','E','C','T','E','D','\0'}},
    {TABP_TABITEMBOTHEDGE,TIBES_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {TABP_TABITEMBOTHEDGE,TIBES_FOCUSED,{'F','O','C','U','S','E','D','\0'}},
    {TABP_TOPTABITEM,0,{'T','O','P','T','A','B','I','T','E','M','\0'}},
    {TABP_TOPTABITEM,TTIS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {TABP_TOPTABITEM,TTIS_HOT,{'H','O','T','\0'}},
    {TABP_TOPTABITEM,TTIS_SELECTED,{'S','E','L','E','C','T','E','D','\0'}},
    {TABP_TOPTABITEM,TTIS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {TABP_TOPTABITEM,TTIS_FOCUSED,{'F','O','C','U','S','E','D','\0'}},
    {TABP_TOPTABITEMLEFTEDGE,0,{'T','O','P','T','A','B','I','T','E','M','L','E','F','T','E','D','G','E','\0'}},
    {TABP_TOPTABITEMLEFTEDGE,TTILES_NORMAL,{'N','O','R','M','A','L','\0'}},
    {TABP_TOPTABITEMLEFTEDGE,TTILES_HOT,{'H','O','T','\0'}},
    {TABP_TOPTABITEMLEFTEDGE,TTILES_SELECTED,{'S','E','L','E','C','T','E','D','\0'}},
    {TABP_TOPTABITEMLEFTEDGE,TTILES_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {TABP_TOPTABITEMLEFTEDGE,TTILES_FOCUSED,{'F','O','C','U','S','E','D','\0'}},
    {TABP_TOPTABITEMRIGHTEDGE,0,{'T','O','P','T','A','B','I','T','E','M','R','I','G','H','T','E','D','G','E','\0'}},
    {TABP_TOPTABITEMRIGHTEDGE,TTIRES_NORMAL,{'N','O','R','M','A','L','\0'}},
    {TABP_TOPTABITEMRIGHTEDGE,TTIRES_HOT,{'H','O','T','\0'}},
    {TABP_TOPTABITEMRIGHTEDGE,TTIRES_SELECTED,{'S','E','L','E','C','T','E','D','\0'}},
    {TABP_TOPTABITEMRIGHTEDGE,TTIRES_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {TABP_TOPTABITEMRIGHTEDGE,TTIRES_FOCUSED,{'F','O','C','U','S','E','D','\0'}},
    {TABP_TOPTABITEMBOTHEDGE,0,{'T','O','P','T','A','B','I','T','E','M','B','O','T','H','E','D','G','E','\0'}},
    {TABP_TOPTABITEMBOTHEDGE,TTIBES_NORMAL,{'N','O','R','M','A','L','\0'}},
    {TABP_TOPTABITEMBOTHEDGE,TTIBES_HOT,{'H','O','T','\0'}},
    {TABP_TOPTABITEMBOTHEDGE,TTIBES_SELECTED,{'S','E','L','E','C','T','E','D','\0'}},
    {TABP_TOPTABITEMBOTHEDGE,TTIBES_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {TABP_TOPTABITEMBOTHEDGE,TTIBES_FOCUSED,{'F','O','C','U','S','E','D','\0'}},
    {TABP_PANE,0,{'P','A','N','E','\0'}},
    {TABP_BODY,0,{'B','O','D','Y','\0'}},
    {0,0,{'\0'}}
};

static const MSSTYLES_CLASS_MAP classTaskband[] = {
    {TDP_GROUPCOUNT,0,{'G','R','O','U','P','C','O','U','N','T','\0'}},
    {TDP_FLASHBUTTON,0,{'F','L','A','S','H','B','U','T','T','O','N','\0'}},
    {TDP_FLASHBUTTONGROUPMENU,0,{'F','L','A','S','H','B','U','T','T','O','N','G','R','O','U','P','M','E','N','U','\0'}},
    {0,0,{'\0'}}
};

static const MSSTYLES_CLASS_MAP classTaskbar[] = {
    {TBP_BACKGROUNDBOTTOM,0,{'B','A','C','K','G','R','O','U','N','D','B','O','T','T','O','M','\0'}},
    {TBP_BACKGROUNDRIGHT,0,{'B','A','C','K','G','R','O','U','N','D','R','I','G','H','T','\0'}},
    {TBP_BACKGROUNDTOP,0,{'B','A','C','K','G','R','O','U','N','D','T','O','P','\0'}},
    {TBP_BACKGROUNDLEFT,0,{'B','A','C','K','G','R','O','U','N','D','L','E','F','T','\0'}},
    {TBP_SIZINGBARBOTTOM,0,{'S','I','Z','I','N','G','B','A','R','B','O','T','T','O','M','\0'}},
    {TBP_SIZINGBARRIGHT,0,{'S','I','Z','I','N','G','B','A','R','R','I','G','H','T','\0'}},
    {TBP_SIZINGBARTOP,0,{'S','I','Z','I','N','G','B','A','R','T','O','P','\0'}},
    {TBP_SIZINGBARLEFT,0,{'S','I','Z','I','N','G','B','A','R','L','E','F','T','\0'}},
    {0,0,{'\0'}}
};

static const MSSTYLES_CLASS_MAP classToolbar[] = {
    {TP_BUTTON,0,{'B','U','T','T','O','N','\0'}},
    {TP_BUTTON,TS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {TP_BUTTON,TS_HOT,{'H','O','T','\0'}},
    {TP_BUTTON,TS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {TP_BUTTON,TS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {TP_BUTTON,TS_CHECKED,{'C','H','E','C','K','E','D','\0'}},
    {TP_BUTTON,TS_HOTCHECKED,{'H','O','T','C','H','E','C','K','E','D','\0'}},
    {TP_DROPDOWNBUTTON,0,{'D','R','O','P','D','O','W','N','B','U','T','T','O','N','\0'}},
    {TP_DROPDOWNBUTTON,TS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {TP_DROPDOWNBUTTON,TS_HOT,{'H','O','T','\0'}},
    {TP_DROPDOWNBUTTON,TS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {TP_DROPDOWNBUTTON,TS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {TP_DROPDOWNBUTTON,TS_CHECKED,{'C','H','E','C','K','E','D','\0'}},
    {TP_DROPDOWNBUTTON,TS_HOTCHECKED,{'H','O','T','C','H','E','C','K','E','D','\0'}},
    {TP_SPLITBUTTON,0,{'S','P','L','I','T','B','U','T','T','O','N','\0'}},
    {TP_SPLITBUTTON,TS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {TP_SPLITBUTTON,TS_HOT,{'H','O','T','\0'}},
    {TP_SPLITBUTTON,TS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {TP_SPLITBUTTON,TS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {TP_SPLITBUTTON,TS_CHECKED,{'C','H','E','C','K','E','D','\0'}},
    {TP_SPLITBUTTON,TS_HOTCHECKED,{'H','O','T','C','H','E','C','K','E','D','\0'}},
    {TP_SPLITBUTTONDROPDOWN,0,{'S','P','L','I','T','B','U','T','T','O','N','D','R','O','P','D','O','W','N','\0'}},
    {TP_SPLITBUTTONDROPDOWN,TS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {TP_SPLITBUTTONDROPDOWN,TS_HOT,{'H','O','T','\0'}},
    {TP_SPLITBUTTONDROPDOWN,TS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {TP_SPLITBUTTONDROPDOWN,TS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {TP_SPLITBUTTONDROPDOWN,TS_CHECKED,{'C','H','E','C','K','E','D','\0'}},
    {TP_SPLITBUTTONDROPDOWN,TS_HOTCHECKED,{'H','O','T','C','H','E','C','K','E','D','\0'}},
    {TP_SEPARATOR,0,{'S','E','P','A','R','A','T','O','R','\0'}},
    {TP_SEPARATOR,TS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {TP_SEPARATOR,TS_HOT,{'H','O','T','\0'}},
    {TP_SEPARATOR,TS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {TP_SEPARATOR,TS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {TP_SEPARATOR,TS_CHECKED,{'C','H','E','C','K','E','D','\0'}},
    {TP_SEPARATOR,TS_HOTCHECKED,{'H','O','T','C','H','E','C','K','E','D','\0'}},
    {TP_SEPARATORVERT,0,{'S','E','P','A','R','A','T','O','R','V','E','R','T','\0'}},
    {TP_SEPARATORVERT,TS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {TP_SEPARATORVERT,TS_HOT,{'H','O','T','\0'}},
    {TP_SEPARATORVERT,TS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {TP_SEPARATORVERT,TS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {TP_SEPARATORVERT,TS_CHECKED,{'C','H','E','C','K','E','D','\0'}},
    {TP_SEPARATORVERT,TS_HOTCHECKED,{'H','O','T','C','H','E','C','K','E','D','\0'}},
    {0,0,{'\0'}}
};

static const MSSTYLES_CLASS_MAP classTooltip[] = {
    {TTP_STANDARD,0,{'S','T','A','N','D','A','R','D','\0'}},
    {TTP_STANDARD,TTSS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {TTP_STANDARD,TTSS_LINK,{'L','I','N','K','\0'}},
    {TTP_STANDARDTITLE,0,{'S','T','A','N','D','A','R','D','T','I','T','L','E','\0'}},
    {TTP_STANDARDTITLE,TTSS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {TTP_STANDARDTITLE,TTSS_LINK,{'L','I','N','K','\0'}},
    {TTP_BALLOON,0,{'B','A','L','L','O','O','N','\0'}},
    {TTP_BALLOON,TTBS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {TTP_BALLOON,TTBS_LINK,{'L','I','N','K','\0'}},
    {TTP_BALLOONTITLE,0,{'B','A','L','L','O','O','N','T','I','T','L','E','\0'}},
    {TTP_BALLOONTITLE,TTBS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {TTP_BALLOONTITLE,TTBS_LINK,{'L','I','N','K','\0'}},
    {TTP_CLOSE,0,{'C','L','O','S','E','\0'}},
    {TTP_CLOSE,TTCS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {TTP_CLOSE,TTCS_HOT,{'H','O','T','\0'}},
    {TTP_CLOSE,TTCS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {0,0,{'\0'}}
};

static const MSSTYLES_CLASS_MAP classTrackbar[] = {
    {TKP_TRACK,0,{'T','R','A','C','K','\0'}},
    {TKP_TRACK,TRS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {TKP_TRACKVERT,0,{'T','R','A','C','K','V','E','R','T','\0'}},
    {TKP_TRACKVERT,TRVS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {TKP_THUMB,0,{'T','H','U','M','B','\0'}},
    {TKP_THUMB,TUS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {TKP_THUMB,TUS_HOT,{'H','O','T','\0'}},
    {TKP_THUMB,TUS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {TKP_THUMB,TUS_FOCUSED,{'F','O','C','U','S','E','D','\0'}},
    {TKP_THUMB,TUS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {TKP_THUMBBOTTOM,0,{'T','H','U','M','B','B','O','T','T','O','M','\0'}},
    {TKP_THUMBBOTTOM,TUBS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {TKP_THUMBBOTTOM,TUBS_HOT,{'H','O','T','\0'}},
    {TKP_THUMBBOTTOM,TUBS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {TKP_THUMBBOTTOM,TUBS_FOCUSED,{'F','O','C','U','S','E','D','\0'}},
    {TKP_THUMBBOTTOM,TUBS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {TKP_THUMBTOP,0,{'T','H','U','M','B','T','O','P','\0'}},
    {TKP_THUMBTOP,TUTS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {TKP_THUMBTOP,TUTS_HOT,{'H','O','T','\0'}},
    {TKP_THUMBTOP,TUTS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {TKP_THUMBTOP,TUTS_FOCUSED,{'F','O','C','U','S','E','D','\0'}},
    {TKP_THUMBTOP,TUTS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {TKP_THUMBVERT,0,{'T','H','U','M','B','V','E','R','T','\0'}},
    {TKP_THUMBVERT,TUVS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {TKP_THUMBVERT,TUVS_HOT,{'H','O','T','\0'}},
    {TKP_THUMBVERT,TUVS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {TKP_THUMBVERT,TUVS_FOCUSED,{'F','O','C','U','S','E','D','\0'}},
    {TKP_THUMBVERT,TUVS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {TKP_THUMBLEFT,0,{'T','H','U','M','B','L','E','F','T','\0'}},
    {TKP_THUMBLEFT,TUVLS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {TKP_THUMBLEFT,TUVLS_HOT,{'H','O','T','\0'}},
    {TKP_THUMBLEFT,TUVLS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {TKP_THUMBLEFT,TUVLS_FOCUSED,{'F','O','C','U','S','E','D','\0'}},
    {TKP_THUMBLEFT,TUVLS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {TKP_THUMBRIGHT,0,{'T','H','U','M','B','R','I','G','H','T','\0'}},
    {TKP_THUMBRIGHT,TUVRS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {TKP_THUMBRIGHT,TUVRS_HOT,{'H','O','T','\0'}},
    {TKP_THUMBRIGHT,TUVRS_PRESSED,{'P','R','E','S','S','E','D','\0'}},
    {TKP_THUMBRIGHT,TUVRS_FOCUSED,{'F','O','C','U','S','E','D','\0'}},
    {TKP_THUMBRIGHT,TUVRS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {TKP_TICS,0,{'T','I','C','S','\0'}},
    {TKP_TICS,TSS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {TKP_TICSVERT,0,{'T','I','C','S','V','E','R','T','\0'}},
    {TKP_TICSVERT,TSVS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {0,0,{'\0'}}
};

static const MSSTYLES_CLASS_MAP classTraynotify[] = {
    {TNP_BACKGROUND,0,{'B','A','C','K','G','R','O','U','N','D','\0'}},
    {TNP_ANIMBACKGROUND,0,{'A','N','I','M','B','A','C','K','G','R','O','U','N','D','\0'}},
    {0,0,{'\0'}}
};

static const MSSTYLES_CLASS_MAP classTreeview[] = {
    {TVP_TREEITEM,0,{'T','R','E','E','I','T','E','M','\0'}},
    {TVP_TREEITEM,TREIS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {TVP_TREEITEM,TREIS_HOT,{'H','O','T','\0'}},
    {TVP_TREEITEM,TREIS_SELECTED,{'S','E','L','E','C','T','E','D','\0'}},
    {TVP_TREEITEM,TREIS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {TVP_TREEITEM,TREIS_SELECTEDNOTFOCUS,{'S','E','L','E','C','T','E','D','N','O','T','F','O','C','U','S','\0'}},
    {TVP_GLYPH,0,{'G','L','Y','P','H','\0'}},
    {TVP_GLYPH,GLPS_CLOSED,{'C','L','O','S','E','D','\0'}},
    {TVP_GLYPH,GLPS_OPENED,{'O','P','E','N','E','D','\0'}},
    {TVP_BRANCH,0,{'B','R','A','N','C','H','\0'}},
    {0,0,{'\0'}}
};

static const MSSTYLES_CLASS_MAP classWindow[] = {
    {WP_CAPTION,0,{'C','A','P','T','I','O','N','\0'}},
    {WP_CAPTION,CS_ACTIVE,{'A','C','T','I','V','E','\0'}},
    {WP_CAPTION,CS_INACTIVE,{'I','N','A','C','T','I','V','E','\0'}},
    {WP_CAPTION,CS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {WP_SMALLCAPTION,0,{'S','M','A','L','L','C','A','P','T','I','O','N','\0'}},
    {WP_SMALLCAPTION,CS_ACTIVE,{'A','C','T','I','V','E','\0'}},
    {WP_SMALLCAPTION,CS_INACTIVE,{'I','N','A','C','T','I','V','E','\0'}},
    {WP_SMALLCAPTION,CS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {WP_MINCAPTION,0,{'M','I','N','C','A','P','T','I','O','N','\0'}},
    {WP_MINCAPTION,MNCS_ACTIVE,{'A','C','T','I','V','E','\0'}},
    {WP_MINCAPTION,MNCS_INACTIVE,{'I','N','A','C','T','I','V','E','\0'}},
    {WP_MINCAPTION,MNCS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {WP_SMALLMINCAPTION,0,{'S','M','A','L','L','M','I','N','C','A','P','T','I','O','N','\0'}},
    {WP_SMALLMINCAPTION,MNCS_ACTIVE,{'A','C','T','I','V','E','\0'}},
    {WP_SMALLMINCAPTION,MNCS_INACTIVE,{'I','N','A','C','T','I','V','E','\0'}},
    {WP_SMALLMINCAPTION,MNCS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {WP_MAXCAPTION,0,{'M','A','X','C','A','P','T','I','O','N','\0'}},
    {WP_MAXCAPTION,MXCS_ACTIVE,{'A','C','T','I','V','E','\0'}},
    {WP_MAXCAPTION,MXCS_INACTIVE,{'I','N','A','C','T','I','V','E','\0'}},
    {WP_MAXCAPTION,MXCS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {WP_SMALLMAXCAPTION,0,{'S','M','A','L','L','M','A','X','C','A','P','T','I','O','N','\0'}},
    {WP_SMALLMAXCAPTION,MXCS_ACTIVE,{'A','C','T','I','V','E','\0'}},
    {WP_SMALLMAXCAPTION,MXCS_INACTIVE,{'I','N','A','C','T','I','V','E','\0'}},
    {WP_SMALLMAXCAPTION,MXCS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {WP_FRAMELEFT,0,{'F','R','A','M','E','L','E','F','T','\0'}},
    {WP_FRAMELEFT,FS_ACTIVE,{'A','C','T','I','V','E','\0'}},
    {WP_FRAMELEFT,FS_INACTIVE,{'I','N','A','C','T','I','V','E','\0'}},
    {WP_FRAMERIGHT,0,{'F','R','A','M','E','R','I','G','H','T','\0'}},
    {WP_FRAMERIGHT,FS_ACTIVE,{'A','C','T','I','V','E','\0'}},
    {WP_FRAMERIGHT,FS_INACTIVE,{'I','N','A','C','T','I','V','E','\0'}},
    {WP_FRAMEBOTTOM,0,{'F','R','A','M','E','B','O','T','T','O','M','\0'}},
    {WP_FRAMEBOTTOM,FS_ACTIVE,{'A','C','T','I','V','E','\0'}},
    {WP_FRAMEBOTTOM,FS_INACTIVE,{'I','N','A','C','T','I','V','E','\0'}},
    {WP_SMALLFRAMELEFT,0,{'S','M','A','L','L','F','R','A','M','E','L','E','F','T','\0'}},
    {WP_SMALLFRAMELEFT,FS_ACTIVE,{'A','C','T','I','V','E','\0'}},
    {WP_SMALLFRAMELEFT,FS_INACTIVE,{'I','N','A','C','T','I','V','E','\0'}},
    {WP_SMALLFRAMERIGHT,0,{'S','M','A','L','L','F','R','A','M','E','R','I','G','H','T','\0'}},
    {WP_SMALLFRAMERIGHT,FS_ACTIVE,{'A','C','T','I','V','E','\0'}},
    {WP_SMALLFRAMERIGHT,FS_INACTIVE,{'I','N','A','C','T','I','V','E','\0'}},
    {WP_SMALLFRAMEBOTTOM,0,{'S','M','A','L','L','F','R','A','M','E','B','O','T','T','O','M','\0'}},
    {WP_SMALLFRAMEBOTTOM,FS_ACTIVE,{'A','C','T','I','V','E','\0'}},
    {WP_SMALLFRAMEBOTTOM,FS_INACTIVE,{'I','N','A','C','T','I','V','E','\0'}},
    {WP_SYSBUTTON,0,{'S','Y','S','B','U','T','T','O','N','\0'}},
    {WP_SYSBUTTON,SBS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {WP_SYSBUTTON,SBS_HOT,{'H','O','T','\0'}},
    {WP_SYSBUTTON,SBS_PUSHED,{'P','U','S','H','E','D','\0'}},
    {WP_SYSBUTTON,SBS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {WP_MDISYSBUTTON,0,{'M','D','I','S','Y','S','B','U','T','T','O','N','\0'}},
    {WP_MDISYSBUTTON,SBS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {WP_MDISYSBUTTON,SBS_HOT,{'H','O','T','\0'}},
    {WP_MDISYSBUTTON,SBS_PUSHED,{'P','U','S','H','E','D','\0'}},
    {WP_MDISYSBUTTON,SBS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {WP_MINBUTTON,0,{'M','I','N','B','U','T','T','O','N','\0'}},
    {WP_MINBUTTON,MINBS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {WP_MINBUTTON,MINBS_HOT,{'H','O','T','\0'}},
    {WP_MINBUTTON,MINBS_PUSHED,{'P','U','S','H','E','D','\0'}},
    {WP_MINBUTTON,MINBS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {WP_MDIMINBUTTON,0,{'M','D','I','M','I','N','B','U','T','T','O','N','\0'}},
    {WP_MDIMINBUTTON,MINBS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {WP_MDIMINBUTTON,MINBS_HOT,{'H','O','T','\0'}},
    {WP_MDIMINBUTTON,MINBS_PUSHED,{'P','U','S','H','E','D','\0'}},
    {WP_MDIMINBUTTON,MINBS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {WP_MAXBUTTON,0,{'M','A','X','B','U','T','T','O','N','\0'}},
    {WP_MAXBUTTON,MAXBS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {WP_MAXBUTTON,MAXBS_HOT,{'H','O','T','\0'}},
    {WP_MAXBUTTON,MAXBS_PUSHED,{'P','U','S','H','E','D','\0'}},
    {WP_MAXBUTTON,MAXBS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {WP_CLOSEBUTTON,0,{'C','L','O','S','E','B','U','T','T','O','N','\0'}},
    {WP_CLOSEBUTTON,CBS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {WP_CLOSEBUTTON,CBS_HOT,{'H','O','T','\0'}},
    {WP_CLOSEBUTTON,CBS_PUSHED,{'P','U','S','H','E','D','\0'}},
    {WP_CLOSEBUTTON,CBS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {WP_SMALLCLOSEBUTTON,0,{'S','M','A','L','L','C','L','O','S','E','B','U','T','T','O','N','\0'}},
    {WP_SMALLCLOSEBUTTON,CBS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {WP_SMALLCLOSEBUTTON,CBS_HOT,{'H','O','T','\0'}},
    {WP_SMALLCLOSEBUTTON,CBS_PUSHED,{'P','U','S','H','E','D','\0'}},
    {WP_SMALLCLOSEBUTTON,CBS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {WP_MDICLOSEBUTTON,0,{'M','D','I','C','L','O','S','E','B','U','T','T','O','N','\0'}},
    {WP_MDICLOSEBUTTON,CBS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {WP_MDICLOSEBUTTON,CBS_HOT,{'H','O','T','\0'}},
    {WP_MDICLOSEBUTTON,CBS_PUSHED,{'P','U','S','H','E','D','\0'}},
    {WP_MDICLOSEBUTTON,CBS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {WP_RESTOREBUTTON,0,{'R','E','S','T','O','R','E','B','U','T','T','O','N','\0'}},
    {WP_RESTOREBUTTON,RBS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {WP_RESTOREBUTTON,RBS_HOT,{'H','O','T','\0'}},
    {WP_RESTOREBUTTON,RBS_PUSHED,{'P','U','S','H','E','D','\0'}},
    {WP_RESTOREBUTTON,RBS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {WP_MDIRESTOREBUTTON,0,{'M','D','I','R','E','S','T','O','R','E','B','U','T','T','O','N','\0'}},
    {WP_MDIRESTOREBUTTON,RBS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {WP_MDIRESTOREBUTTON,RBS_HOT,{'H','O','T','\0'}},
    {WP_MDIRESTOREBUTTON,RBS_PUSHED,{'P','U','S','H','E','D','\0'}},
    {WP_MDIRESTOREBUTTON,RBS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {WP_HELPBUTTON,0,{'H','E','L','P','B','U','T','T','O','N','\0'}},
    {WP_HELPBUTTON,HBS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {WP_HELPBUTTON,HBS_HOT,{'H','O','T','\0'}},
    {WP_HELPBUTTON,HBS_PUSHED,{'P','U','S','H','E','D','\0'}},
    {WP_HELPBUTTON,HBS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {WP_MDIHELPBUTTON,0,{'M','D','I','H','E','L','P','B','U','T','T','O','N','\0'}},
    {WP_MDIHELPBUTTON,HBS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {WP_MDIHELPBUTTON,HBS_HOT,{'H','O','T','\0'}},
    {WP_MDIHELPBUTTON,HBS_PUSHED,{'P','U','S','H','E','D','\0'}},
    {WP_MDIHELPBUTTON,HBS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {WP_HORZSCROLL,0,{'H','O','R','Z','S','C','R','O','L','L','\0'}},
    {WP_HORZSCROLL,HSS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {WP_HORZSCROLL,HSS_HOT,{'H','O','T','\0'}},
    {WP_HORZSCROLL,HSS_PUSHED,{'P','U','S','H','E','D','\0'}},
    {WP_HORZSCROLL,HSS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {WP_HORZTHUMB,0,{'H','O','R','Z','T','H','U','M','B','\0'}},
    {WP_HORZTHUMB,HTS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {WP_HORZTHUMB,HTS_HOT,{'H','O','T','\0'}},
    {WP_HORZTHUMB,HTS_PUSHED,{'P','U','S','H','E','D','\0'}},
    {WP_HORZTHUMB,HTS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {WP_VERTSCROLL,0,{'V','E','R','T','S','C','R','O','L','L','\0'}},
    {WP_VERTSCROLL,VSS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {WP_VERTSCROLL,VSS_HOT,{'H','O','T','\0'}},
    {WP_VERTSCROLL,VSS_PUSHED,{'P','U','S','H','E','D','\0'}},
    {WP_VERTSCROLL,VSS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {WP_VERTTHUMB,0,{'V','E','R','T','T','H','U','M','B','\0'}},
    {WP_VERTTHUMB,VTS_NORMAL,{'N','O','R','M','A','L','\0'}},
    {WP_VERTTHUMB,VTS_HOT,{'H','O','T','\0'}},
    {WP_VERTTHUMB,VTS_PUSHED,{'P','U','S','H','E','D','\0'}},
    {WP_VERTTHUMB,VTS_DISABLED,{'D','I','S','A','B','L','E','D','\0'}},
    {WP_DIALOG,0,{'D','I','A','L','O','G','\0'}},
    {WP_CAPTIONSIZINGTEMPLATE,0,{'C','A','P','T','I','O','N','S','I','Z','I','N','G','T','E','M','P','L','A','T','E','\0'}},
    {WP_SMALLCAPTIONSIZINGTEMPLATE,0,{'S','M','A','L','L','C','A','P','T','I','O','N','S','I','Z','I','N','G','T','E','M','P','L','A','T','E','\0'}},
    {WP_FRAMELEFTSIZINGTEMPLATE,0,{'F','R','A','M','E','L','E','F','T','S','I','Z','I','N','G','T','E','M','P','L','A','T','E','\0'}},
    {WP_SMALLFRAMELEFTSIZINGTEMPLATE,0,{'S','M','A','L','L','F','R','A','M','E','L','E','F','T','S','I','Z','I','N','G','T','E','M','P','L','A','T','E','\0'}},
    {WP_FRAMERIGHTSIZINGTEMPLATE,0,{'F','R','A','M','E','R','I','G','H','T','S','I','Z','I','N','G','T','E','M','P','L','A','T','E','\0'}},
    {WP_SMALLFRAMERIGHTSIZINGTEMPLATE,0,{'S','M','A','L','L','F','R','A','M','E','R','I','G','H','T','S','I','Z','I','N','G','T','E','M','P','L','A','T','E','\0'}},
    {WP_FRAMEBOTTOMSIZINGTEMPLATE,0,{'F','R','A','M','E','B','O','T','T','O','M','S','I','Z','I','N','G','T','E','M','P','L','A','T','E','\0'}},
    {WP_SMALLFRAMEBOTTOMSIZINGTEMPLATE,0,{'S','M','A','L','L','F','R','A','M','E','B','O','T','T','O','M','S','I','Z','I','N','G','T','E','M','P','L','A','T','E','\0'}},
    {0,0,{'\0'}}
};

/* Map class names to part/state maps */
static const MSSTYLES_CLASS_NAME mapClass[] = {
    {classGlobals, {'G','L','O','B','A','L','S','\0'}},
    {classButton, {'B','U','T','T','O','N','\0'}},
    {classClock, {'C','L','O','C','K','\0'}},
    {classCombobox, {'C','O','M','B','O','B','O','X','\0'}},
    {classEdit, {'E','D','I','T','\0'}},
    {classExplorerbar, {'E','X','P','L','O','R','E','R','B','A','R','\0'}},
    {classHeader, {'H','E','A','D','E','R','\0'}},
    {classListview, {'L','I','S','T','V','I','E','W','\0'}},
    {classMenu, {'M','E','N','U','\0'}},
    {classMenuband, {'M','E','N','U','B','A','N','D','\0'}},
    {classPage, {'P','A','G','E','\0'}},
    {classProgress, {'P','R','O','G','R','E','S','S','\0'}},
    {classRebar, {'R','E','B','A','R','\0'}},
    {classScrollbar, {'S','C','R','O','L','L','B','A','R','\0'}},
    {classSpin, {'S','P','I','N','\0'}},
    {classStartpanel, {'S','T','A','R','T','P','A','N','E','L','\0'}},
    {classStatus, {'S','T','A','T','U','S','\0'}},
    {classTab, {'T','A','B','\0'}},
    {classTaskband, {'T','A','S','K','B','A','N','D','\0'}},
    {classTaskbar, {'T','A','S','K','B','A','R','\0'}},
    {classToolbar, {'T','O','O','L','B','A','R','\0'}},
    {classTooltip, {'T','O','O','L','T','I','P','\0'}},
    {classTrackbar, {'T','R','A','C','K','B','A','R','\0'}},
    {classTraynotify, {'T','R','A','Y','N','O','T','I','F','Y','\0'}},
    {classTreeview, {'T','R','E','E','V','I','E','W','\0'}},
    {classWindow, {'W','I','N','D','O','W','\0'}}
};

BOOL MSSTYLES_LookupPartState(LPCWSTR pszClass, LPCWSTR pszPart, LPCWSTR pszState, int *iPartId, int *iStateId)
{
    unsigned int i;
    const MSSTYLES_CLASS_MAP *map;

    *iPartId = 0;
    *iStateId = 0;
    for(i=0; i<sizeof(mapClass)/sizeof(mapClass[0]); i++) {
        if(!lstrcmpiW(mapClass[i].pszClass, pszClass)) {
            map = mapClass[i].lpMap;
            if(pszPart) {
                do {
                    if(map->dwStateID == 0 && !lstrcmpiW(map->szName, pszPart)) {
                        *iPartId = map->dwPartID;
                        break;
                    }
                } while(*((++map)->szName));
            }
            if(pszState) {
                if(pszPart && *iPartId == 0) {
                    break;
                }
                do {
                    if(pszPart) {
                        if(map->dwPartID == *iPartId && !lstrcmpiW(map->szName, pszState)) {
                            *iStateId = map->dwStateID;
                            break;
                        }
                    }
                    else {
                        if(!lstrcmpiW(map->szName, pszState)) {
                            *iStateId = map->dwStateID;
                            break;
                        }
                    }
                } while(*((++map)->szName));
            }
            break;
        }
    }
    if(pszPart && *iPartId == 0) {
        return FALSE;
    }
    if(pszState && *iStateId == 0) {
        return FALSE;
    }
    return TRUE;
}

/**********************************************************************
 *      MSSTYLES_LookupProperty
 *
 * Find a property ID from name
 *
 * PARAMS
 *     pszPropertyName     Name of property to lookup
 *     dwPrimitive         Location to store primitive type of property
 *     dwId                Location to store ID of property
 *
 * RETURNS
 *     FALSE if value is not found, TRUE otherwise
 */
BOOL MSSTYLES_LookupProperty(LPCWSTR pszPropertyName, int *dwPrimitive, int *dwId)
{
    DWORD item = 0;
    do {
        if(!lstrcmpiW(mapProperty[item].szPropertyName, pszPropertyName)) {
            if(dwPrimitive) *dwPrimitive = mapProperty[item].dwPrimitiveType;
            if(dwId) *dwId = mapProperty[item].dwPropertyID;
            return TRUE;
        }
    } while(*mapProperty[++item].szPropertyName);
    return FALSE;
}

/**********************************************************************
 *      MSSTYLES_LookupEnum
 *
 * Lookup the value for an enumeration
 *
 * PARAMS
 *     pszValueName        Value name to lookup
 *     dwEnum              Enumeration property ID to search
 *     dwValue             Location to store value
 *
 * RETURNS
 *     FALSE if value is not found, TRUE otherwise
 */
BOOL MSSTYLES_LookupEnum(LPCWSTR pszValueName, int dwEnum, int *dwValue)
{
    DWORD item = 0;
    /* Locate the enum block */
    while(*mapEnum[item].szValueName && mapEnum[item].dwEnum != dwEnum) item++;
    /* Now find the value in that block */
    while(*mapEnum[item].szValueName && mapEnum[item].dwEnum == dwEnum) {
        if(!lstrcmpiW(mapEnum[item].szValueName, pszValueName)) {
            if(dwValue) *dwValue = mapEnum[item].dwValue;
            return TRUE;
        }
        item++;
    }
    return FALSE;
}
