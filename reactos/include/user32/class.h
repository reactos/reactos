/*
 * Window classes definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef __WINE_CLASS_H
#define __WINE_CLASS_H

#include <windows.h>
#include <user32/winproc.h>
#include <user32/dce.h>

#define MAX_CLASSNAME	255


#define CLASS_MAGIC   ('C' | ('L' << 8) | ('A' << 16) | ('S' << 24))




typedef struct tagCLASS
{
    struct tagCLASS *next;        /* Next class */
    UINT           magic;         /* Magic number */
    UINT           cWindows;      /* Count of existing windows */
    UINT           style;         /* Class style */
    WNDPROC        winproc;       /* Window procedure */ 
    INT            cbClsExtra;    /* Class extra bytes */
    INT            cbWndExtra;    /* Window extra bytes */
    void *         menuName;     /* Menu name */
    struct tagDCE  *dce;           /* Class DCE (if CS_CLASSDC) */
    HINSTANCE      hInstance;     /* Module that created the task */
    HICON          hIcon;         /* Default icon */
    HICON          hIconSm;       /* Default small icon */
    HCURSOR        hCursor;       /* Default cursor */
    HBRUSH         hbrBackground; /* Default background */
    ATOM           atomName;      /* Name of the class */
    void *         className;   /* Class name (Unicode) */
    WINBOOL        bUnicode;	  /* Unicode or Ascii window */
    LONG           wExtra[1];     /* Class extra bytes */
} CLASS;

CLASS *CLASS_FindClassByAtom( ATOM classAtom, HINSTANCE hInstance );
WINBOOL CLASS_FreeClass(CLASS *classPtr);

#endif  /* __WINE_CLASS_H */
