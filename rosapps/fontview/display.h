#ifndef _DISPLAY_H
#define _DISPLAY_H

/* Messages for the display class */
#define FVM_SETTYPEFACE WM_USER
#define FVM_SETSTRING (WM_USER + 1)

/* Size restrictions */
#define MAX_STRING 100
#define MAX_TYPEFACENAME 32
#define MAX_FORMAT 20

#define MAX_SIZES 7

extern const WCHAR g_szFontDisplayClassName[];

/* Public function */
BOOL Display_InitClass(HINSTANCE hInstance);

#endif // _DISPLAY_H
