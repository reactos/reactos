/*
 * Window classes definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef __WINE_CLASS_H
#define __WINE_CLASS_H

#include <user32/winproc.h>

struct tagDCE;

#define MAX_CLASSNAME	255


#define CLASS_MAGIC   ('C' | ('L' << 8) | ('A' << 16) | ('S' << 24))


typedef struct tagCLASS
{
    struct tagCLASS *next;          /* Next class */
    UINT           magic;         /* Magic number */
    UINT           cWindows;      /* Count of existing windows */
    UINT           style;         /* Class style */
    WNDPROC        winproc;       /* Window procedure */ 
    INT            cbClsExtra;    /* Class extra bytes */
    INT            cbWndExtra;    /* Window extra bytes */
    char           menuNameA[MAX_CLASSNAME]; 
    WCHAR          menuNameW[MAX_CLASSNAME];     /* Default menu name (Unicode) */
    struct tagDCE   *dce;           /* Class DCE (if CS_CLASSDC) */
    HINSTANCE      hInstance;     /* Module that created the task */
    HICON          hIcon;         /* Default icon */
    HICON          hIconSm;       /* Default small icon */
    HCURSOR        hCursor;       /* Default cursor */
    HBRUSH         hbrBackground; /* Default background */
    ATOM             atomName;      /* Name of the class */
    char            classNameA[MAX_CLASSNAME];    /* Class name (ASCII string) */
    WCHAR           classNameW[MAX_CLASSNAME];    /* Class name (Unicode) */
    LONG             wExtra[1];     /* Class extra bytes */
} CLASS;

void CLASS_DumpClass( CLASS *class );
void CLASS_WalkClasses(void);
void CLASS_FreeModuleClasses( HMODULE hModule );
CLASS *CLASS_FindClassByAtom( ATOM atom, HINSTANCE hinstance );

#endif  /* __WINE_CLASS_H */
