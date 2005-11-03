/* ---------------- classdef.h --------------- */

#ifndef CLASSDEF_H
#define CLASSDEF_H

typedef struct DfClassDefs {
    DFCLASS base;                         /* base window class */
    int (*wndproc)(struct DfWindow *,enum DfMessages,DF_PARAM,DF_PARAM);
    int attrib;
} DFCLASSDEFS;

extern DFCLASSDEFS DfClassDefs[];

#define DF_SHADOW       0x0001
#define DF_MOVEABLE     0x0002
#define DF_SIZEABLE     0x0004
#define DF_HASMENUBAR   0x0008
#define DF_VSCROLLBAR   0x0010
#define DF_HSCROLLBAR   0x0020
#define DF_VISIBLE      0x0040
#define DF_SAVESELF     0x0080
#define DF_HASTITLEBAR  0x0100
#define DF_CONTROLBOX   0x0200
#define DF_MINMAXBOX    0x0400
#define DF_NOCLIP       0x0800
#define DF_READONLY     0x1000
#define DF_MULTILINE    0x2000
#define DF_HASBORDER    0x4000
#define DF_HASSTATUSBAR 0x8000

#endif
