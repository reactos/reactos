/*
 * Internal msstyles related defines & declarations
 *
 * Copyright (C) 2003 Kevin Koltzau
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

#ifndef __WINE_MSSTYLES_H
#define __WINE_MSSTYLES_H

#define TMT_ENUM 200

#define MAX_THEME_APP_NAME 60
#define MAX_THEME_CLASS_NAME 60
#define MAX_THEME_VALUE_NAME 60

typedef struct _THEME_PROPERTY {
    int iPrimitiveType;
    int iPropertyId;
    PROPERTYORIGIN origin;

    LPCWSTR lpValue;
    DWORD dwValueLen;

    struct _THEME_PROPERTY *next;
} THEME_PROPERTY, *PTHEME_PROPERTY;

typedef struct _THEME_PARTSTATE {
    int iPartId;
    int iStateId;
    PTHEME_PROPERTY properties;

    struct _THEME_PARTSTATE *next;
} THEME_PARTSTATE, *PTHEME_PARTSTATE;

struct _THEME_FILE;

typedef struct _THEME_CLASS {
    HMODULE hTheme;
    struct _THEME_FILE* tf;
    WCHAR szAppName[MAX_THEME_APP_NAME];
    WCHAR szClassName[MAX_THEME_CLASS_NAME];
    PTHEME_PARTSTATE partstate;
    struct _THEME_CLASS *overrides;

    struct _THEME_CLASS *next;
} THEME_CLASS, *PTHEME_CLASS;

typedef struct _THEME_IMAGE {
    WCHAR name[MAX_PATH];
    HBITMAP image;
    BOOL hasAlpha;
    
    struct _THEME_IMAGE *next;
} THEME_IMAGE, *PTHEME_IMAGE;

typedef struct _THEME_FILE {
    DWORD dwRefCount;
    HMODULE hTheme;
    WCHAR szThemeFile[MAX_PATH];
    LPWSTR pszAvailColors;
    LPWSTR pszAvailSizes;

    LPWSTR pszSelectedColor;
    LPWSTR pszSelectedSize;

    PTHEME_CLASS classes;
    PTHEME_PROPERTY metrics;
    PTHEME_IMAGE images;
} THEME_FILE, *PTHEME_FILE;

typedef struct _UXINI_FILE *PUXINI_FILE;

HRESULT MSSTYLES_OpenThemeFile(LPCWSTR lpThemeFile, LPCWSTR pszColorName, LPCWSTR pszSizeName, PTHEME_FILE *tf);
void MSSTYLES_CloseThemeFile(PTHEME_FILE tf);
HRESULT MSSTYLES_SetActiveTheme(PTHEME_FILE tf, BOOL setMetrics);
PTHEME_CLASS MSSTYLES_OpenThemeClass(LPCWSTR pszAppName, LPCWSTR pszClassList);
HRESULT MSSTYLES_CloseThemeClass(PTHEME_CLASS tc);
BOOL MSSTYLES_LookupProperty(LPCWSTR pszPropertyName, int *dwPrimitive, int *dwId);
BOOL MSSTYLES_LookupEnum(LPCWSTR pszValueName, int dwEnum, int *dwValue);
BOOL MSSTYLES_LookupPartState(LPCWSTR pszClass, LPCWSTR pszPart, LPCWSTR pszState, int *iPartId, int *iStateId);
PUXINI_FILE MSSTYLES_GetThemeIni(PTHEME_FILE tf);
PTHEME_PARTSTATE MSSTYLES_FindPartState(PTHEME_CLASS tc, int iPartId, int iStateId, PTHEME_CLASS *tcNext);
PTHEME_PROPERTY MSSTYLES_FindProperty(PTHEME_CLASS tc, int iPartId, int iStateId, int iPropertyPrimitive, int iPropertyId);
PTHEME_PROPERTY MSSTYLES_FindMetric(int iPropertyPrimitive, int iPropertyId);
HBITMAP MSSTYLES_LoadBitmap(PTHEME_CLASS tc, LPCWSTR lpFilename, BOOL* hasAlpha);

HRESULT MSSTYLES_GetPropertyBool(PTHEME_PROPERTY tp, BOOL *pfVal);
HRESULT MSSTYLES_GetPropertyColor(PTHEME_PROPERTY tp, COLORREF *pColor);
HRESULT MSSTYLES_GetPropertyFont(PTHEME_PROPERTY tp, HDC hdc, LOGFONTW *pFont);
HRESULT MSSTYLES_GetPropertyInt(PTHEME_PROPERTY tp, int *piVal);
HRESULT MSSTYLES_GetPropertyIntList(PTHEME_PROPERTY tp, INTLIST *pIntList);
HRESULT MSSTYLES_GetPropertyPosition(PTHEME_PROPERTY tp, POINT *pPoint);
HRESULT MSSTYLES_GetPropertyString(PTHEME_PROPERTY tp, LPWSTR pszBuff, int cchMaxBuffChars);
HRESULT MSSTYLES_GetPropertyRect(PTHEME_PROPERTY tp, RECT *pRect);
HRESULT MSSTYLES_GetPropertyMargins(PTHEME_PROPERTY tp, RECT *prc, MARGINS *pMargins);

PUXINI_FILE UXINI_LoadINI(HMODULE hTheme, LPCWSTR lpName);
void UXINI_CloseINI(PUXINI_FILE uf);
LPCWSTR UXINI_GetNextSection(PUXINI_FILE uf, DWORD *dwLen);
BOOL UXINI_FindSection(PUXINI_FILE uf, LPCWSTR lpName);
LPCWSTR UXINI_GetNextValue(PUXINI_FILE uf, DWORD *dwNameLen, LPCWSTR *lpValue, DWORD *dwValueLen);
BOOL UXINI_FindValue(PUXINI_FILE uf, LPCWSTR lpName, LPCWSTR *lpValue, DWORD *dwValueLen);

#endif
