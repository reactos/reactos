/* ---------------- classdef.h --------------- */

#ifndef CLASSDEF_H
#define CLASSDEF_H

typedef struct classdefs {
    DFCLASS base;                         /* base window class */
    int (*wndproc)(struct window *,enum messages,PARAM,PARAM);
    int attrib;
} CLASSDEFS;

extern CLASSDEFS classdefs[];

#define SHADOW       0x0001
#define MOVEABLE     0x0002
#define SIZEABLE     0x0004
#define HASMENUBAR   0x0008
#define VSCROLLBAR   0x0010
#define HSCROLLBAR   0x0020
#define VISIBLE      0x0040
#define SAVESELF     0x0080
#define HASTITLEBAR  0x0100
#define CONTROLBOX   0x0200
#define MINMAXBOX    0x0400
#define NOCLIP       0x0800
#define READONLY     0x1000
#define MULTILINE    0x2000
#define HASBORDER    0x4000
#define HASSTATUSBAR 0x8000

#endif
