// Copyright (c) 1997-1999 Microsoft Corporation
// 
// KBFUNC.C    // Function library to KBMAIN.C
// File modified to paint bitmaps instead of icons : a-anilk :02-16-99
// Last updated Maria Jose and Anil Kumar
// 
#define STRICT

#include <windows.h>
#include <commdlg.h>

#include "kbmain.h"
#include "kbus.h"
#include "kbfunc.h"
#include "resource.h"
#include "dgsett.h"
// #include "wingdi.h"
#include <malloc.h>
#include <stdlib.h>

#define RGBBLACK     RGB(0,0,0)
#define RGBWHITE     RGB(255,255,255)
#define RGBBACK     RGB(107,107,107)
#define DSPDxax   0x00E20746L


//This is not define until NT5 (#define WINVER 0x0500)
//The header file I am using won't let me build a release build if I set WINVER 0x0500
//Currently it is WINVER 0x0400
//When we get the new header file, we can remove the line below
#define IDC_HAND            MAKEINTRESOURCE(32649)


#define REDRAW			1
#define NREDRAW			2

extern BOOL g_fDrawShift;
extern BOOL g_fDrawCapital;


extern KBkeyRec	KBkey[] =
	{
	//0
    {TEXT(""),TEXT(""),	TEXT(""),TEXT(""),
     NO_NAME,0,0,0,0,TRUE,KNORMAL_TYPE,BOTH},  //DUMMY

//1
	{TEXT("esc"),TEXT("esc"),TEXT("{esc}"),TEXT("{esc}"),
     NO_NAME, 1,1,8,8, TRUE,  KNORMAL_TYPE, BOTH, NREDRAW, 2,
     {0x01,0x00,0x00,0x00}},

//2
    {TEXT("F1"), TEXT("F1"), TEXT("{f1}"), TEXT("{f1}"),
     NO_NAME, 1,19, 8,8, FALSE, KNORMAL_TYPE, BOTH, NREDRAW, 2,
     {0x3B,0x00,0x00,0x00}},

//3
    {TEXT("F2"), TEXT("F2"), TEXT("{f2}"), TEXT("{f2}"),
     NO_NAME, 1,28, 8,8, FALSE, KNORMAL_TYPE, BOTH, NREDRAW, 2,
     {0x3C,0x00,0x00,0x00}},

//4
    {TEXT("F3"), TEXT("F3"), TEXT("{f3}"), TEXT("{f3}"),
     NO_NAME, 1,37, 8,8, FALSE, KNORMAL_TYPE, BOTH, NREDRAW, 2,
     {0x3D,0x00,0x00,0x00}},

//5
    {TEXT("F4"), TEXT("F4"), TEXT("{f4}"), TEXT("{f4}"),
     NO_NAME, 1,46, 8,8, FALSE, KNORMAL_TYPE, BOTH, NREDRAW, 2,
     {0x3E,0x00,0x00,0x00}},

//6
    {TEXT("F5"), TEXT("F5"), TEXT("{f5}"), TEXT("{f5}"),
     NO_NAME, 1,60, 8,8, FALSE, KNORMAL_TYPE, BOTH, NREDRAW, 2,
     {0x3F,0x00,0x00,0x00}},

//7
    {TEXT("F6"), TEXT("F6"), TEXT("{f6}"), TEXT("{f6}"),
     NO_NAME, 1,69, 8,8, FALSE, KNORMAL_TYPE, BOTH, NREDRAW, 2,
     {0x40,0x00,0x00,0x00}},

//8
    {TEXT("F7"), TEXT("F7"), TEXT("{f7}"), TEXT("{f7}"),
     NO_NAME, 1,78, 8,8, FALSE, KNORMAL_TYPE, BOTH, NREDRAW, 2,
     {0x41,0x00,0x00,0x00}},

//9
    {TEXT("F8"), TEXT("F8"), TEXT("{f8}"), TEXT("{f8}"),
     NO_NAME, 1,87, 8,8, FALSE, KNORMAL_TYPE, BOTH, NREDRAW, 2,
     {0x42,0x00,0x00,0x00}},

//10
    {TEXT("F9"), TEXT("F9"), TEXT("{f9}"), TEXT("{f9}"),
     NO_NAME, 1,101, 8,8, FALSE, KNORMAL_TYPE, BOTH, NREDRAW, 2,
     {0x43,0x00,0x00,0x00}},

//11
    {TEXT("F10"),TEXT("F10"), TEXT("{f10}"),TEXT("{f10}"),
     NO_NAME,  1,110, 8,8, FALSE, KNORMAL_TYPE, BOTH, NREDRAW, 2,
     {0x44,0x00,0x00,0x00}},

//12
    {TEXT("F11"),TEXT("F11"), TEXT("{f11}"),TEXT("{f11}"),
     NO_NAME,  1,119, 8,8, FALSE, KNORMAL_TYPE, BOTH, NREDRAW, 2,
     {0x57,0x00,0x00,0x00}},

//13
    {TEXT("F12"),TEXT("F12"), TEXT("{f12}"),TEXT("{f12}"),
     NO_NAME,1,128,8,8, FALSE, KNORMAL_TYPE, BOTH, NREDRAW, 2,
     {0x58,0x00,0x00,0x00}},

//14
    {TEXT("psc"), TEXT("psc"),TEXT("{PRTSC}"),TEXT("{PRTSC}"),
     KB_PSC, 1,138,8,8,  TRUE, KNORMAL_TYPE, LARGE, NREDRAW, 2,
     {0xE0,0x2A,0xE0,0x37}},

//15
    {TEXT("slk"), TEXT("slk"),TEXT("{SCROLLOCK}"),TEXT("{SCROLLOCK}"),
     KB_SCROLL,1,147,8, 8, TRUE, SCROLLOCK_TYPE, LARGE, NREDRAW, 2,
     {0x46,0x00,0x00,0x00}},

//16
	{TEXT("brk"), TEXT("pau"), TEXT("{BREAK}"), TEXT("{^s}"),
     NO_NAME,1,156,8,8, TRUE, KNORMAL_TYPE, LARGE, REDRAW, 2,
     {0xE1,0x1D,0x45,0x00}},

//17
    {TEXT("`"), TEXT("~"), TEXT("`"), TEXT("{~}"),
     NO_NAME, 12,1,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
     {0x29,0x00,0x00,0x00}},

//18
    {TEXT("1"), TEXT("!"), TEXT("1"), TEXT("!"),
     NO_NAME, 12,10,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
     {0x02,0x00,0x00,0x00}},

//19
	{TEXT("2"),	TEXT("@"), TEXT("2"), TEXT("@"),
     NO_NAME, 12,19,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
     {0x03,0x00,0x00,0x00}},

//20
    {TEXT("3"), TEXT("#"), TEXT("3"), TEXT("#"),
     NO_NAME,12,28,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
     {0x04,0x00,0x00,0x00}},

//21
	{TEXT("4"),		TEXT("$"),		TEXT("4"),		TEXT("$"),		NO_NAME,	 12,	  37,   8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x05,0x00,0x00,0x00}},
	
//22	
	{TEXT("5"), 	TEXT("%"), 		TEXT("5"),		TEXT("{%}"),	NO_NAME,	 12,	  46,   8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x06,0x00,0x00,0x00}},
	
//23	
	{TEXT("6"),		TEXT("^"),		TEXT("6"),		TEXT("{^}"),	NO_NAME,	 12,	  55,   8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x07,0x00,0x00,0x00}},
	
//24
	{TEXT("7"),		TEXT("&"),		TEXT("7"),		TEXT("&"),		NO_NAME,	 12,	  64,   8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x08,0x00,0x00,0x00}},
	
//25
	{TEXT("8"), 	TEXT("*"), 		TEXT("8"),		TEXT("*"),		NO_NAME,	 12,	  73,   8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x09,0x00,0x00,0x00}},
	
//26
	{TEXT("9"),		TEXT("("),		TEXT("9"),		TEXT("("),		NO_NAME,	 12,	  82,   8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x0A,0x00,0x00,0x00}},
	
//27
	{TEXT("0"),		TEXT(")"),		TEXT("0"),		TEXT(")"),		NO_NAME,	 12,	  91,   8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x0B,0x00,0x00,0x00}},
	
//28
	{TEXT("-"), 	TEXT("_"), 		TEXT("-"),		TEXT("_"),		NO_NAME,	 12,	 100,   8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x0C,0x00,0x00,0x00}},
	
//29
	{TEXT("="),		TEXT("+"),		TEXT("="),		TEXT("{+}"),	NO_NAME,	 12,	 109,   8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x0D,0x00,0x00,0x00}},

//30
//Japanese KB extra key
	{TEXT(""),TEXT(""),	TEXT(""),TEXT(""), NO_NAME,0,0,0,0,TRUE,KNORMAL_TYPE,NOTSHOW},  //DUMMY

//31
	{TEXT("bksp"),TEXT("bksp"),TEXT("{BS}"),TEXT("{BS}"),
     NO_NAME,12, 118,8,18,  TRUE, KNORMAL_TYPE, BOTH, NREDRAW, 2,
     {0x0E,0x00,0x00,0x00}},

//32
	{TEXT("ins"),TEXT("ins"),TEXT("{INSERT}"),TEXT("{INSERT}"), NO_NAME, 12,138, 8,8, TRUE, KNORMAL_TYPE, LARGE, NREDRAW, 2, {0xE0,0x52,0x00,0x00}},
	
//33	
	{TEXT("hm"), TEXT("hm"), TEXT("{HOME}"), TEXT("{HOME}"), 	NO_NAME, 12,147, 8,8, TRUE, KNORMAL_TYPE, LARGE, NREDRAW, 2, {0xE0,0x47,0x00,0x00}},

//34
	{TEXT("pup"),TEXT("pup"),TEXT("{PGUP}"),TEXT("{PGUP}"),		NO_NAME, 12,156, 8,8, TRUE, KNORMAL_TYPE, LARGE, NREDRAW, 2, {0xE0,0x49,0x00,0x00}},

//35
	{TEXT("nlk"),TEXT("nlk"),	TEXT("{NUMLOCK}"),TEXT("{NUMLOCK}"),KB_NUMLOCK,12,166,  8,	 8, FALSE, NUMLOCK_TYPE, LARGE, NREDRAW, 2, {0x45,0x00,0x00,0x00}},
	
//36
	{TEXT("/"),	TEXT("/"),	TEXT("/"),	TEXT("/"),	NO_NAME, 12, 175,  8, 8, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 2, {0xE0,0x35,0x00,0x00}},
	
//37
	{TEXT("*"),	TEXT("*"),	TEXT("*"),	TEXT("*"),	NO_NAME, 12, 184,  8, 8, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 2, {0xE0,0x37,0x00,0x00}},
	
//38	
	{TEXT("-"),	TEXT("-"),	TEXT("-"),	TEXT("-"),	NO_NAME, 12, 193,  8, 8, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 1, {0x4A,0x00,0x00,0x00}},

//39
	{TEXT("tab"),	TEXT("tab"),	TEXT("{TAB}"),TEXT("{TAB}"),NO_NAME, 21,   1,  8,	13, FALSE, KNORMAL_TYPE, BOTH, NREDRAW, 2, {0x0F,0x00,0x00,0x00}},

//40
	{TEXT("q"),	TEXT("Q"),	TEXT("q"),	TEXT("+q"),	NO_NAME, 21,  15,  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x10,0x00,0x00,0x00}},
	
//41
	{TEXT("w"),	TEXT("W"),	TEXT("w"),	TEXT("+w"),	NO_NAME, 21,  24,  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x11,0x00,0x00,0x00}},
	
//42
	{TEXT("e"),	TEXT("E"),	TEXT("e"),	TEXT("+e"),	NO_NAME, 21,  33,  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x12,0x00,0x00,0x00}},
	
//43
	{TEXT("r"),	TEXT("R"),	TEXT("r"),	TEXT("+r"),	NO_NAME, 21,  42,  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x13,0x00,0x00,0x00}},

//44
    {TEXT("t"),	TEXT("T"),	TEXT("t"),	TEXT("+t"),	
     NO_NAME, 21,51,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
     {0x14,0x00,0x00,0x00}},

//45
	{TEXT("y"),	TEXT("Y"),	TEXT("y"),	TEXT("+y"),	NO_NAME, 21,  60,  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x15,0x00,0x00,0x00}},
	
//46	
	{TEXT("u"),	TEXT("U"),	TEXT("u"),	TEXT("+u"),	NO_NAME, 21,  69,  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x16,0x00,0x00,0x00}},
	
//47	
	{TEXT("i"),	TEXT("I"),	TEXT("i"),	TEXT("+i"),	NO_NAME, 21,  78,  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x17,0x00,0x00,0x00}},
	
//48
	{TEXT("o"),	TEXT("O"),	TEXT("o"),	TEXT("+o"),	NO_NAME, 21,  87,  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x18,0x00,0x00,0x00}},
	
//49	
	{TEXT("p"),	TEXT("P"),	TEXT("p"),	TEXT("+p"),	NO_NAME, 21,  96,  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x19,0x00,0x00,0x00}},
	
//50
	{TEXT("["),	TEXT("{"),	TEXT("["),	TEXT("{{}"),	NO_NAME, 21, 105,  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x1A,0x00,0x00,0x00}},
	
//51	
	{TEXT("]"),	TEXT("}"),	TEXT("]"),	TEXT("{}}"),	NO_NAME, 21, 114,  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x1B,0x00,0x00,0x00}},
	
//52	
	{TEXT("\\"),	TEXT("|"),	TEXT("\\"),	TEXT("|"),	NO_NAME, 21, 123,  8,	13, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x2B,0x00,0x00,0x00}},

//53
	{TEXT("del"), TEXT("del"), 	TEXT("{DEL}"),TEXT("{DEL}"),NO_NAME, 21,   138,  8, 8, TRUE, KNORMAL_TYPE, LARGE, NREDRAW, 2, {0xE0,0x53,0x00,0x00}},

//54	
	{TEXT("end"),	TEXT("end"), 	TEXT("{END}"),TEXT("{END}"),NO_NAME, 21,   147,  8, 8, TRUE, KNORMAL_TYPE, LARGE, NREDRAW, 2, {0xE0,0x4F,0x00,0x00}},

//55	
	{TEXT("pdn"), TEXT("pdn"), 	TEXT("{PGDN}"),TEXT("{PGDN}"),NO_NAME, 21, 156,  8, 8, TRUE, KNORMAL_TYPE, LARGE, NREDRAW, 2, {0xE0,0x51,0x00,0x00}},

//56
	{TEXT("7"),		TEXT("7"),		TEXT("hm"),		TEXT("7"),		NO_NAME,	 21,	 166,	  8,	 8, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 2, {0x47,0x00,0x00,0x00}},

//57	
	{TEXT("8"),		TEXT("8"),		TEXT("8"),		TEXT("8"),		NO_NAME,	 21,	 175,	  8,	 8, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 2, {0x48,0x00,0x00,0x00}},

//58	
	{TEXT("9"),		TEXT("9"),		TEXT("pup"),		TEXT("9"),		NO_NAME,	 21,	 184,	  8,	 8, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 2, {0x49,0x00,0x00,0x00}},
	
//59
	{TEXT("+"),		TEXT("+"),		TEXT("{+}"),  	TEXT("{+}"),	NO_NAME,	 21,	 193,	 17,	 8, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 2, {0x4E,0x00,0x00,0x00}},


//60
    {TEXT("lock"),TEXT("lock"),TEXT("{caplock}"),TEXT("{caplock}"),
     KB_CAPLOCK, 30,1,8,17, TRUE, KMODIFIER_TYPE, BOTH, REDRAW, 2,
     {0x3A,0x00,0x00,0x00}},

//61
	{TEXT("a"),	TEXT("A"), TEXT("a"), TEXT("+a"),
     NO_NAME, 30,19,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
     {0x1E,0x00,0x00,0x00}},

//62
	{TEXT("s"),		TEXT("S"),		TEXT("s"),		TEXT("+s"),		NO_NAME,	  30,	  28,	  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x1F,0x00,0x00,0x00}},
	
//63
	{TEXT("d"),		TEXT("D"),		TEXT("d"),		TEXT("+d"),		NO_NAME,	  30,	  37,	  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x20,0x00,0x00,0x00}},
	
//64
	{TEXT("f"),		TEXT("F"),		TEXT("f"),		TEXT("+f"),		NO_NAME,	  30,	  46,	  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x21,0x00,0x00,0x00}},
	
//65
	{TEXT("g"),		TEXT("G"),		TEXT("g"),		TEXT("+g"),		NO_NAME,	  30,	  55,	  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x22,0x00,0x00,0x00}},
	
//66
	{TEXT("h"),		TEXT("H"),		TEXT("h"),		TEXT("+h"),		NO_NAME,	  30,	  64,	  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x23,0x00,0x00,0x00}},

//67
	{TEXT("j"),	TEXT("J"), TEXT("j"), TEXT("+j"),
     NO_NAME, 30,73,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x24,0x00,0x00,0x00}},

//68
	{TEXT("k"),		TEXT("K"),		TEXT("k"),		TEXT("+k"),		NO_NAME,	  30,	  82,	  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x25,0x00,0x00,0x00}},
	
//69
	{TEXT("l"),		TEXT("L"),		TEXT("l"),		TEXT("+l"),		NO_NAME,	  30,	  91,	  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x26,0x00,0x00,0x00}},
	
//70	
	{TEXT(";"), TEXT(":"), TEXT(";"), TEXT("+;"),
     NO_NAME, 30,100,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x27,0x00,0x00,0x00}},

//71
	{TEXT("'"),		TEXT("''"),		TEXT("'"),		TEXT("''"),		NO_NAME,	  30,	 109,	  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x28,0x00,0x00,0x00}},
	
//72
//Japanese KB extra key
	{TEXT("\\"),	TEXT("|"),	TEXT("\\"),	TEXT("|"),	NO_NAME, 21, 118,  8,	8, FALSE, KNORMAL_TYPE, NOTSHOW, REDRAW, 1, {0x2B,0x00,0x00,0x00}},
	
//73	
	{TEXT("ent"),TEXT("ent"),TEXT("{enter}"),TEXT("{enter}"),	NO_NAME,  30,	 118,	  8,  18, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 2, {0x1C,0x00,0x00,0x00}},


//74
    {TEXT("4"), TEXT("4"), TEXT("4"), TEXT("4"),
     NO_NAME, 30,166,8,8, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 2,
     {0x4B,0x00,0x00,0x00}},

//75
    {TEXT("5"),	TEXT("5"), TEXT("5"), TEXT("5"),
     NO_NAME, 30,175,8,8, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 2,
     {0x4C,0x00,0x00,0x00}},

//76
    {TEXT("6"),	TEXT("6"), TEXT("6"), TEXT("6"),
     NO_NAME, 30,184,8,8, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 2,
     {0x4D,0x00,0x00,0x00}},


//77
	{TEXT("shft"),TEXT("shft"),	TEXT(""), TEXT(""),
     KB_LSHIFT, 39,1,8,21, TRUE, KMODIFIER_TYPE, BOTH, REDRAW, 2,
     {0x2A,0x00,0x00,0x00}},

//78
    {TEXT("z"), TEXT("Z"),  TEXT("z"),  TEXT("+z"),
     NO_NAME,39,23,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
     {0x2C,0x00,0x00,0x00}},

//79
    {TEXT("x"),	TEXT("X"), TEXT("x"), TEXT("+x"),
     NO_NAME, 39,32,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
     {0x2D,0x00,0x00,0x00}},

//80
    {TEXT("c"), TEXT("C"), TEXT("c"), TEXT("+c"),
     NO_NAME, 39,41,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
     {0x2E,0x00,0x00,0x00}},

//81
    {TEXT("v"), TEXT("V"), TEXT("v"), TEXT("+v"),
     NO_NAME, 39,50,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
     {0x2F,0x00,0x00,0x00}},

//82
    {TEXT("b"),TEXT("B"),TEXT("b"),TEXT("+b"),
     NO_NAME,39,59,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
     {0x30,0x00,0x00,0x00}},

//83
    {TEXT("n"),	TEXT("N"), TEXT("n"), TEXT("+n"),
     NO_NAME,39,68,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
     {0x31,0x00,0x00,0x00}},

//84
    {TEXT("m"), TEXT("M"), TEXT("m"), TEXT("+m"),
     NO_NAME, 39,77,8,8,FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
     {0x32,0x00,0x00,0x00}},

//85
    {TEXT(","),	TEXT("<"), TEXT(","), TEXT("+<"),
     NO_NAME, 39,86,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
     {0x33,0x00,0x00,0x00}},

//86
    {TEXT("."), TEXT(">"), TEXT("."), TEXT("+>"),
     NO_NAME, 39,95,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
    {0x34,0x00,0x00,0x00}},

//87
    {TEXT("/"),	TEXT("?"), TEXT("/"), TEXT("+/"),
     NO_NAME, 39,104,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
     {0x35,0x00,0x00,0x00}},


//88
//Japanese KB extra key
	{TEXT(""),TEXT(""),	TEXT(""),TEXT(""), NO_NAME,0,0,0,0,TRUE,KNORMAL_TYPE,NOTSHOW},  //DUMMY
	
//89
	{TEXT("shft"),TEXT("shft"),TEXT(""),TEXT(""),
     KB_RSHIFT,39,113,8,23,TRUE, KMODIFIER_TYPE, BOTH, REDRAW, 2,
     {0x36,0x00,0x00,0x00}},


//90
    {TEXT("IDB_UPUPARW"),TEXT("IDB_UPDNARW"),TEXT("IDB_UP"),TEXT("{UP}"),
     BITMAP,39,147,8,8, FALSE, KMODIFIER_TYPE, LARGE, NREDRAW, 1,
     {0xE0,0x48,0x00,0x00}},

//91
	{TEXT("1"), TEXT("1"),TEXT("end"),TEXT("1"),
     NO_NAME,39,166,8,8, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 2,
     {0x4F,0x00,0x00,0x00}},

//92
	{TEXT("2"), TEXT("2"),TEXT("2"),TEXT("2"),
     NO_NAME,39,175,8,8, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 2,
     {0x50,0x00,0x00,0x00}},

//93
	{TEXT("3"),TEXT("3"),TEXT("pdn"),TEXT("3"),
     NO_NAME,39,184,8,8, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 2,
     {0x51,0x00,0x00,0x00}},

//94
	{TEXT("ent"),TEXT("ent"),TEXT("ent"),TEXT("ent"),
     NO_NAME, 39,193,17,8,  TRUE, KNORMAL_TYPE, LARGE, NREDRAW, 2,
     {0xE0,0x1C,0x00,0x00}},


//95
	{TEXT("ctrl"), TEXT("ctrl"),TEXT(""),TEXT(""),
     KB_LCTR,48,1,8,13,  TRUE, KMODIFIER_TYPE, BOTH, REDRAW, 2,
     {0x1D,0x00,0x00,0x00}},

//96
    {TEXT("winlogoUp"), TEXT("winlogoDn"),TEXT("I_winlogo"),TEXT("lwin"),
     ICON, 48, 15 ,8,8,TRUE, KMODIFIER_TYPE,BOTH, REDRAW},

//97
    {TEXT("alt"),TEXT("alt"),TEXT(""),TEXT(""),
	 KB_LALT,48,24,8,13,TRUE, KMODIFIER_TYPE, BOTH, REDRAW, 2,
     {0x38,0x00,0x00,0x00}},

//98
//Japanese KB extra key
	{TEXT(""),TEXT(""),	TEXT(""),TEXT(""), NO_NAME,0,0,0,0,TRUE,KNORMAL_TYPE,NOTSHOW},  //DUMMY

//99
    {TEXT(""),TEXT(""),TEXT(" "),TEXT(" "),
     KB_SPACE,48,38,8,52, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 1,
     {0x39,0x00,0x00,0x00}},

//100
//Japanese KB extra key
	{TEXT(""),TEXT(""),	TEXT(""),TEXT(""), NO_NAME,0,0,0,0,TRUE,KNORMAL_TYPE,NOTSHOW},  //DUMMY

//101
//Japanese KB extra key
	{TEXT(""),TEXT(""),	TEXT(""),TEXT(""), NO_NAME,0,0,0,0,TRUE,KNORMAL_TYPE,NOTSHOW},  //DUMMY


//102
    {TEXT("alt"),TEXT("alt"),TEXT(""),TEXT(""),
     KB_RALT,48,91,8,13, TRUE, KMODIFIER_TYPE, LARGE, REDRAW, 2,
     {0xE0,0x38,0x00,0x00}},

//103
	{TEXT("winlogoUp"), TEXT("winlogoDn"), TEXT("I_winlogo"),TEXT("rwin"),
     ICON, 48,105,8,8,TRUE, KMODIFIER_TYPE,LARGE, REDRAW},

//104
	{TEXT("MenuKeyUp"), TEXT("MenuKeyDn"), TEXT("I_MenuKey"),TEXT("App"),
     ICON, 48,114,8,8, TRUE, KMODIFIER_TYPE,LARGE, REDRAW},

//105
    {TEXT("ctrl"),TEXT("ctrl"),TEXT(""),TEXT(""),
     KB_RCTR,48,123,8,13,TRUE, KMODIFIER_TYPE, LARGE, REDRAW, 2,
     {0xE0,0x10,0x00,0x00}},


//106
	{TEXT("IDB_LFUPARW"),TEXT("IDB_LFDNARW"),TEXT("IDB_LEFT"),TEXT("{LEFT}"),
     BITMAP, 48,138,8,8, FALSE, KMODIFIER_TYPE, LARGE, NREDRAW, 1,
     {0xE0,0x4B,0x00,0x00}},

//107
	{TEXT("IDB_DNUPARW"),TEXT("IDB_DNDNARW"),TEXT("IDB_DOWN"),TEXT("{DOWN}"),
     BITMAP, 48,147,8,8, FALSE, KMODIFIER_TYPE, LARGE, NREDRAW, 1,
     {0xE0,0x50,0x00,0x00}},

//108
	{TEXT("IDB_RHUPARW"),TEXT("IDB_RHDNARW"),TEXT("IDB_RIGHT"),TEXT("{RIGHT}"),
     BITMAP, 48,156,8,8, FALSE, KMODIFIER_TYPE, LARGE, NREDRAW, 1,
     {0xE0,0x4D,0x00,0x00}},


//109
    {TEXT("0"),	TEXT("0"),	TEXT("ins"),	TEXT("0"),
     NO_NAME, 48,166,8,17, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 2,
     {0x52,0x00,0x00,0x00}},

//110
    {TEXT("."),	TEXT("."),	TEXT("del"),	TEXT("."),
     NO_NAME, 48,184,8,8, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 2,
     {0x53,0x00,0x00,0x00}},

//End of large KB

//111
	{TEXT(""), TEXT(""), TEXT(" "), TEXT(" "),
     KB_SPACE,  48,38,8,38, FALSE, KNORMAL_TYPE, SMALL, NREDRAW, 1,
     {0x39,0x00,0x00,0x00}},


//112
	{TEXT("alt"), TEXT("alt"), TEXT(""), TEXT(""),
     KB_RALT,  48,77,8,13, TRUE, KMODIFIER_TYPE, SMALL, REDRAW, 2,
     {0xE0,0x38,0x00,0x00}},

//113
	{TEXT("MenuKeyUp"), TEXT("MenuKeyDn"), TEXT("I_MenuKey"), TEXT("App"),
     ICON, 48,91,8,8, TRUE, KMODIFIER_TYPE, SMALL, REDRAW},


//114
	{TEXT("IDB_UPUPARW"),TEXT("IDB_UPUPARW"),TEXT("IDB_UP"),TEXT("{UP}"),
     BITMAP, 48,100,8,8, FALSE, KMODIFIER_TYPE, SMALL, NREDRAW, 1,
     {0xE0,0x48,0x00,0x00}},

//115
	{TEXT("IDB_DNUPARW"),TEXT("IDB_DNDNARW"),TEXT("IDB_DOWN"),TEXT("{DOWN}"),
     BITMAP, 48,109,8,8, FALSE, KMODIFIER_TYPE, SMALL, NREDRAW, 1,
     {0xE0,0x50,0x00,0x00}},

//116
	{TEXT("IDB_LFUPARW"),TEXT("IDB_LFDNARW"),TEXT("IDB_LEFT"),TEXT("{LEFT}"),
     BITMAP, 48,118,8,8, FALSE, KMODIFIER_TYPE, SMALL, NREDRAW, 1,
     {0xE0,0x4B,0x00,0x00}},

//117
    {TEXT("IDB_RHUPARW"),TEXT("IDB_RHDNARW"),TEXT("IDB_RIGHT"),TEXT("{RIGHT}"),
     BITMAP,48,127, 8,9, FALSE, KMODIFIER_TYPE, SMALL, NREDRAW, 1,
     {0xE0,0x4D,0x00,0x00}},

	};

/**************************************************************************/
// FUNCTIONS in Other FILEs
/**************************************************************************/
LRESULT WINAPI kbMainWndProc (HWND hwnd, UINT message,
											WPARAM wParam, LPARAM lParam);
LRESULT WINAPI kbKeyWndProc (HWND Childhwnd, UINT message, WPARAM wParam,
											 LPARAM lParam);
LRESULT WINAPI kbNumBaseWndProc (HWND NumBasehwnd, UINT message, WPARAM wParam,
											 LPARAM lParam);
LRESULT WINAPI kbNumWndProc (HWND Numhwnd, UINT message, WPARAM wParam,
											 LPARAM lParam);
void SendErrorMessage(UINT id_string);
//COLORREF RChooseColor(COLORREF color);
void MakeClick(int what);
BOOL WINAPI KillMouse(void);
BOOL WINAPI CapLetterON(void);    // in mousehook.c

BOOL AltKeyPressed(void);    // in mousehook.c
BOOL ControlKeyPressed(void);    // in mousehook.c


DWORD WhatPlatform(void);

/****************************************************************************/
/* Global Vars */
/****************************************************************************/
TCHAR szKbMainClass[] 	= TEXT("MainKb") ;
extern BOOL settingChanged;
extern HWND ShiftHwnd;
/****************************************************************************/
/* BOOL InitProc(void) */
/****************************************************************************/
BOOL InitProc(void)
{	
	// this has to be done after the creation of the windows si hace falta
	 if(GetSystemMetrics(SM_MOUSEPRESENT) == 0)  // No mouse install
	 {
		SendErrorMessage(IDS_NO_MOUSE);
		return FALSE;
	 }

	// How many keys we have.
	lenKBkey = sizeof(KBkey)/sizeof(KBkey[0]);
    return TRUE;
} // InitiateProcess

/****************************************************************************/
/* BOOL RegisterWndClass(void) */
/****************************************************************************/
BOOL RegisterWndClass(HINSTANCE hInst)
{
	WNDCLASS    wndclass;
	TCHAR		Wclass[10]=TEXT("");
	register 	int i;
	COLORREF    color;
	HCURSOR		hCursor=NULL;

	// Keyboard frame class
	wndclass.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wndclass.lpfnWndProc   = kbMainWndProc ;
	wndclass.cbClsExtra    = 0 ;
	wndclass.cbWndExtra    = 0 ;
	wndclass.hInstance     = hInst;
	wndclass.hIcon         = LoadIcon (hInst, TEXT("APP_OSK"));
	

	//Load the system hand cursor
	hCursor = LoadCursor (NULL, IDC_HAND);

	//If no cursor available (since it is only in NT5), use our own
	if(hCursor == NULL)
		wndclass.hCursor   = LoadCursor (hInst, MAKEINTRESOURCE(IDC_CURHAND1));
	
	else  //there is one, use the system one
		wndclass.hCursor   = hCursor;


	wndclass.hbrBackground = (HBRUSH)COLOR_INACTIVECAPTION;
	wndclass.lpszMenuName  = TEXT("IDR_MENU");
	wndclass.lpszClassName = szKbMainClass ;

	RegisterClass(&wndclass);

   /*
    //Number pad keys class
#if 0
	wndclass.lpfnWndProc  = kbNumBaseWndProc;
	wndclass.lpszMenuName = NULL;
	wndclass.hbrBackground = GetStockObject (LTGRAY_BRUSH);
	wndclass.lpszClassName = szKbNumBaseClass;

	RegisterClass(&wndclass);
#endif
    */

	//All keys class
	for (i = 1; i < lenKBkey; i++)
	{
		switch (KBkey[i].ktype)
			{
			case KNORMAL_TYPE:
				wsprintf(Wclass, TEXT("N%d"), i);
				color = COLOR_WINDOW;
			break;
			case KMODIFIER_TYPE:
				wsprintf(Wclass, TEXT("M%d"), i);
				color = COLOR_MENU;
			break;
			case KDEAD_TYPE:
				wsprintf(Wclass, TEXT("D%d"), i);
				color = COLOR_MENU;
			break;
			case NUMLOCK_TYPE:
				wsprintf(Wclass, TEXT("NL%d"), i);
				color = COLOR_WINDOW;
			break;

			case SCROLLOCK_TYPE:
				wsprintf(Wclass, TEXT("SL%d"), i);
				color = COLOR_WINDOW;
			break;
			case LED_NUMLOCK_TYPE:
				wsprintf(Wclass, TEXT("LN%d"), i);
				color = COLOR_GRAYTEXT;
			break;
			case LED_CAPSLOCK_TYPE:
				wsprintf(Wclass, TEXT("LC%d"), i);
				color = COLOR_GRAYTEXT;
			break;
			case LED_SCROLLLOCK_TYPE:
				wsprintf(Wclass, TEXT("LS%d"), i);
				color = COLOR_GRAYTEXT;
			break;
			}
		wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
		wndclass.lpfnWndProc   = kbKeyWndProc ;
		wndclass.cbWndExtra    = sizeof (long);
        wndclass.hIcon         = NULL;
		wndclass.hbrBackground = (HBRUSH)color;
		wndclass.lpszClassName = Wclass ;
//		wndclass.hIconSm = NULL;
		RegisterClass (&wndclass);
	}


	return TRUE;
} // RegisterWndClass
/****************************************************************************/



extern BOOL  Setting_ReadSuccess;      //read the setting file success ?
extern DWORD platform;


/****************************************************************************/
/*  HWND CreateMainWindow(void) */
/****************************************************************************/
HWND CreateMainWindow(BOOL re_size)
{
	int x, y, cx, cy, temp;
	TCHAR  str[256]=TEXT("");
	int KB_SMALLRMARGIN= 137;

	// SmallMargin for Actual / Block layout
	if(kbPref->Actual)
		KB_SMALLRMARGIN = 137;  //Actual
	else
		KB_SMALLRMARGIN = 152;  //Block


	if(!Setting_ReadSuccess)       //if can't read the setting file
	{	
        g_margin = scrCX / KB_LARGERMARGIN;

		if(g_margin < 4)
		{
			g_margin = 4;
			smallKb = TRUE;
			cx = KB_SMALLRMARGIN * g_margin;
		}
		else
			cx = KB_LARGERMARGIN * g_margin;

		temp = scrCY - 5;          // 5 units from the bottom
		y = temp - (g_margin * KB_CHARBMARGIN) - captionCY; //- menuCY;
		x = 5;                     // 5 units from the left
		cy = temp - y;
	}


	if (re_size)
    {	
        MoveWindow(kbmainhwnd, x, y, cx, cy, TRUE);
        return NULL;
    }


    LoadString(hInst, IDS_TITLE1, &str[0], 256);
	
	
    //*********************************
    //Create the main window (Keyboard)
    //*********************************

	//if can't read the setting file
    if (!Setting_ReadSuccess)
    {


        return (CreateWindow(szKbMainClass, str,
                             WS_OVERLAPPED| //  WS_THICKFRAME |
                             WS_MINIMIZEBOX | WS_SYSMENU,
                             x, y, cx, cy,
                             NULL, NULL, hInst, NULL));
    }

	//Can read the setting file (Mostly go here)
    else
    {
        return (CreateWindow(szKbMainClass, str,
                             WS_OVERLAPPED| WS_SYSMENU| // WS_THICKFRAME |
						     WS_MINIMIZEBOX,
                             kbPref->KB_Rect.left, kbPref->KB_Rect.right,
                             kbPref->KB_Rect.right - kbPref->KB_Rect.left,
                             kbPref->KB_Rect.bottom - kbPref->KB_Rect.top,
                             NULL, NULL, hInst, NULL));
    }

} // CreateMainWindow



/*****************************************************************************
* void mlGetSystemParam( void)
*
* GET SYSTEM PARAMETERS
*****************************************************************************/
void mlGetSystemParam(void)
{

	scrCX 		= GetSystemMetrics(SM_CXSCREEN);       // Screen Width
	scrCY 		= GetSystemMetrics(SM_CYSCREEN);       // Screen Height
	captionCY 	= GetSystemMetrics(SM_CYCAPTION);		// Caption Bar Height

} // mlGetSystemParam

/****************************************************************************/
/* BOOL rSetWindowPos(void)                            */
/*                                                     */
/* Place the main window always on top / non top most  */
/*                                                     */
/****************************************************************************/
BOOL rSetWindowPos(void)
{
	HWND where;

	if(PrefAlwaysontop == TRUE)
		where = HWND_TOPMOST;
	else
		where = HWND_NOTOPMOST;


	SetWindowPos(kbmainhwnd, where, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
	return TRUE;
} // rSetWindowPos


/****************************************************************************/

#define SND_UP 	1
#define SND_DOWN 	2
static BOOL lastDown = FALSE;
/********************************************************************
* udfDraw3D(HDC bhdc, RECT brect)
*
* 3d effect in the right and bottom side
********************************************************************/
void udfDraw3D(HDC bhdc, RECT brect)
{
	static LOGPEN lpBlack = { PS_SOLID, 3, 3, RGB (127, 127, 127) },
				  lpWhite = { PS_SOLID, 3, 3, RGB (255, 255, 255) } ;

	POINT bPoint[3];
	HPEN oldhpen;
	HPEN hPenBlack, hPenWhite ;

	hPenWhite = CreatePenIndirect(&lpWhite);
	hPenBlack = CreatePenIndirect(&lpBlack);
	oldhpen = SelectObject(bhdc, hPenBlack);

	bPoint[0].x = brect.right - 2 ;
	bPoint[0].y =  2;
	bPoint[1].x = brect.right - 2;
	bPoint[1].y = brect.bottom - 2;
	bPoint[2].x =  0;
	bPoint[2].y = brect.bottom - 2;
	Polyline(bhdc, bPoint,3);

	SelectObject(bhdc, hPenWhite);
	bPoint[0].x =  2 ;
	bPoint[0].y =  brect.bottom -3;
	bPoint[1].x = 2;
	bPoint[1].y = 2;
	bPoint[2].x =  brect.right - 3;
	bPoint[2].y = 2;
	Polyline(bhdc, bPoint,3);

	SelectObject(bhdc, oldhpen);
	DeleteObject(hPenWhite);
	DeleteObject(hPenBlack);

} //udfDraw3D


/********************************************************************
* udfDraw3Dpush(HDC hdc, RECT rect, BOOL wcolor)
*
* 3d effect when pushing buttons
********************************************************************/

void udfDraw3Dpush(HDC bhdc, RECT brect, BOOL wcolor)      // button down
{
	POINT bPoint[3];
	HPEN oldhpen;
	HPEN hPenBlack;

	LOGPEN lpBlack = { PS_SOLID, 3, 3, RGB (0, 0, 0) };

	if(wcolor)   //the red outline pen
	{
		lpBlack.lopnColor = RGB(255,0,0);
		lpBlack.lopnWidth.x = 2;
		lpBlack.lopnWidth.y = 2;
	}

	hPenBlack = CreatePenIndirect(&lpBlack);
	oldhpen = SelectObject(bhdc, hPenBlack);

	bPoint[0].x = brect.right - 1 ;
	bPoint[0].y =  +2;
	bPoint[1].x = brect.right - 1;
	bPoint[1].y = brect.bottom - 1;
	bPoint[2].x =  0;
	bPoint[2].y = brect.bottom - 1;
	Polyline(bhdc, bPoint,3);

	bPoint[0].x =  1 ;
	bPoint[0].y =  brect.bottom;
	bPoint[1].x = 0;
	bPoint[1].y = 0;
	bPoint[2].x =  brect.right;
	bPoint[2].y = 1;
	Polyline(bhdc, bPoint,3);

	SelectObject(bhdc, oldhpen);
	DeleteObject(hPenBlack);

} //udfDraw3Dpush

/*******************************************************************/
//Change to dead key color or re-set to normal key color
//result == -1 or 2 means it is dead key
/*******************************************************************/
BOOL Change_Reset_DeadKeyColor(HWND Childhwnd, int index, int result)
{	static int lastindex=-1;
	BOOL bRet=FALSE;

	//change dead key color
	if(((result == -1) || (result == 2))&& (index != lastindex))
	{	KBkey[index].ktype = KDEAD_TYPE;   //dead key
		
		//When it is hilite, change the color to black
		if(GetWindowLong(Childhwnd, 0) == 4)
        {
            DeleteObject((HGDIOBJ)SetClassLongPtr(Childhwnd, GCLP_HBRBACKGROUND,
                         (LONG_PTR)CreateSolidBrush(RGB(0,0,0))));
        }
		else
			SetClassLongPtr(Childhwnd, GCLP_HBRBACKGROUND, COLOR_MENU);

		InvalidateRect(Childhwnd, NULL, TRUE);
		lastindex=index;
		bRet = TRUE;
	}
    //previous got set to dead key color, Re-set it
    else if(KBkey[index].ktype == KDEAD_TYPE)
    {
        KBkey[index].ktype = KNORMAL_TYPE;    //reset to normal
        SetClassLongPtr(Childhwnd, GCLP_HBRBACKGROUND, COLOR_WINDOW);
        lastindex=-1;
        bRet = FALSE;
    }

    return bRet;
}


/*******************************************************************/
extern BOOL RALT;
extern HWND DeadHwnd;
extern BOOL save_the_shift;
extern BOOL kbfCapLock;
extern HKL	hkl;
//extern DWORD g_dwConversion;
/********************************************************************
* vPrintCenter(HDC bhdc, RECT brect, char *outstr, int bpush)
*
* print out str in the center of brect
********************************************************************/
void vPrintCenter(HWND Childhwnd, HDC bhdc, RECT brect, int index, int bpush)
{	
    TEXTMETRIC tm;
	int    outwidth, outheight, outsize;
	int    px, py, oldBkMode;
	BYTE   kbuf[256]="";          //array to record the keyboard state
	TCHAR *outstr;
	UINT   vk;
	TCHAR  cbuf[30]=TEXT("");
	UINT   spcscan;
	HFONT  hFont=NULL;        // Handle of the selected font
	int    result;
	HKL	   hkl;
	DWORD  dwProcessId;
	DWORD  dwlayout;
	int i;

/*
HIMC hImc=0;
DWORD dwConversion=0, dwSentence=0;
char hi[]="hi";
*/

	//Get the keys status
	GetKeyboardState(kbuf);



	if(GetKeyState(VK_KANA) & 0x01)
		kbuf[VK_KANA]= 0x80;
	else
		kbuf[VK_KANA]= 0;


/*
	hImc = ImmGetContext(GetActiveWindow());
	ImmGetConversionStatus(hImc, &dwConversion, &dwSentence);

if(hImc==0)
OutputDebugString(&hi);

	if(dwConversion & IME_CMODE_ROMAN)
		kbuf[VK_KANA]= 0;
	else
		kbuf[VK_KANA]= 0x80;
*/



	//Clean up the dead key previously store in keyboard buffer
	spcscan = MapVirtualKey(VK_SPACE,0);


	//Get the active window's thread
	dwlayout=GetWindowThreadProcessId(GetActiveWindow(), &dwProcessId);
	//Get the active window's keyboard layout
	hkl=GetKeyboardLayout(dwlayout);


#ifdef UNICODE
	ToUnicodeEx(VK_SPACE, spcscan, kbuf, cbuf, 30, 0, hkl);
#else
	ToAsciiEx(VK_SPACE, spcscan, kbuf, (LPWORD)cbuf, 0, hkl);
#endif




	// Get the Virtual Key from Scan Code
	vk = MapVirtualKey(KBkey[index].scancode[0],1);



	//Print Cap letters
	if(CapLetterON())
	{
		//I have to manually set it; otherwise it may not refresh properly
		//Let test see it breaks any thing

		if (g_fDrawCapital)
		{
			// v-mjgran: CapLock is pressed
			kbuf[VK_CAPITAL]= 0x01;  // set the low bit (toggled)		
			
			if (g_fDrawShift)
				kbuf[VK_SHIFT]=0x80;	// v-mjgran: Shift is also pressed
			else
				kbuf[VK_SHIFT]=0x0;		// v-mjgran: Shift is not pressed

			kbuf[VK_LSHIFT]= 0;
			kbuf[VK_RSHIFT]= 0;
			if (!AltKeyPressed())
				kbuf[VK_CONTROL]= 0;			//v-mjgran: Control off to avoid weird symbols
		}
		else                         //CapLock OFF, let's set the Shift ON
		{
			kbuf[VK_SHIFT]= 0x80;
			kbuf[VK_CAPITAL]= 0;
			if (!AltKeyPressed())
				kbuf[VK_CONTROL]= 0;			//v-mjgran: Control off to avoid weird symbols
		}

#ifdef UNICODE
		result=ToUnicodeEx(vk,KBkey[index].scancode[0],kbuf,cbuf,30,0, hkl);
#else
		result=ToAsciiEx(vk,KBkey[index].scancode[0],kbuf,(LPWORD)cbuf,0, hkl);
#endif

		//Just want the first char in the buffer. Clean up the rest.
		if(result < 0 || result == 2)
			for(i=1; i<30; i++)
				cbuf[i] = 0;

    }


	//for drawing the special key when RAlt key is down
    else if (RALT || (AltKeyPressed() && ControlKeyPressed()))		//v-mjgran: RALT and Ctrl+LALT must have the same management
    {		
		kbuf[VK_MENU]=0x80;       //Alt down
		kbuf[VK_CONTROL]=0x80;    //Ctrl down
		kbuf[VK_SHIFT]=0;



#ifdef UNICODE
		result=ToUnicodeEx(vk,KBkey[index].scancode[0],kbuf,cbuf,30,0, hkl);

		//Handle the dead key
		//Currently some bug in ToUnicode, we need to call ToUnicode twice to reset on
		//the dead key in the keyboard buffer.
		if(result < 0 || result == 2)
		{	ToUnicodeEx(vk,KBkey[index].scancode[0],kbuf,cbuf,30,0, hkl);
			
			//There are more than one char get copy into the buffer if it is a dead key
			//We need to clean up the buffer
			for(i=1; i<30; i++)
				cbuf[i] = 0;
		}

#else
		result=ToAsciiEx(vk,KBkey[index].scancode[0],kbuf,(LPWORD)cbuf,0, hkl);

		//Handle the dead key
		//Currently some bug in ToUnicode, we need to call ToUnicode twice to reset on
		//the dead key in the keyboard buffer.
		if(result < 0 || result == 2)
		{	ToAsciiEx(vk,KBkey[index].scancode[0],kbuf,(LPWORD)cbuf,0, hkl);

			//There are more than one char get copy into the buffer if it is a dead key
			//We need to clean up the buffer
			for(i=1; i<30; i++)
				cbuf[i] = 0;
		}

#endif

	}





	//Print normal letter
	else
	{
		//**Below I manually set some keys state for the special cases**
		
		//For CapLock On and Shift down, the number keys should be symbols(e.g. ~!@#)
		if(ShiftHwnd != NULL)     //CapLock down and Shift down
		{
			kbuf[VK_SHIFT]=0x80;
			if (!AltKeyPressed())
				kbuf[VK_CONTROL]= 0;			//v-mjgran: Control off to avoid weird symbols
		}
		else	                  //Shift up
		{	kbuf[VK_SHIFT]=0;
			kbuf[VK_LSHIFT]=0;
			kbuf[VK_RSHIFT]=0;
			kbuf[VK_CAPITAL]=0;
			if (!AltKeyPressed())
				kbuf[VK_CONTROL]= 0;			//v-mjgran: Control off to avoid weird symbols
		}
		
		//For shift dead key
		if(Childhwnd==DeadHwnd && save_the_shift)
			kbuf[VK_SHIFT]=0x80;



		//Get the text we need to print on the key
		result=GetKeyText(vk, KBkey[index].scancode[0], kbuf, cbuf, hkl);
		
	}



	//Change the key color if it is a dead key
	Change_Reset_DeadKeyColor(Childhwnd, index, result);
	outstr=cbuf;




    //Use the header file pre-define text
    if(KBkey[index].print==2)
    {
		if(CapLetterON())
			outstr=KBkey[index].textC;  //Print Cap
		else
			outstr=KBkey[index].textL;  //Print lower
    }


/*
// **Doing testing on redraw num pad***


	//We handle the numpad keys
	if(KBkey[index].print==3)
	{
		//Num Lock Toggled (ON)
		if(LOBYTE(GetKeyState(VK_NUMLOCK)) &0x01)
			outstr=KBkey[index].textL;       //print the text for Num Lock On
		else
			outstr=KBkey[index].skLow;      //print the text for Num Lock Off
	}
*/

	outsize = lstrlen(outstr);
	hFont=ReSizeFont(index, plf, outsize);

    if (NULL != hFont)
    	oldFontHdle = SelectObject(bhdc, hFont);

	oldBkMode = SetBkMode(bhdc, TRANSPARENT);

	//Hilite color
	if(bpush == 4)
	{
		SetTextColor(bhdc, RGB(255,255,255));
		bpush = 0;
	}

	//Modifier key color
	else if(KBkey[index].ktype == KMODIFIER_TYPE)
	{
		BOOL clr = (BOOL)GetWindowLongPtr(Childhwnd, GWLP_USERDATA);
		SetTextColor(bhdc, clr? GetSysColor(COLOR_INACTIVECAPTION) : GetSysColor(COLOR_INACTIVECAPTIONTEXT));
	}


	//Dead key color
    //in NT result < 0; in Win95 result == 2
    else if (result < 0 || result == 2)
        SetTextColor(bhdc, GetSysColor(COLOR_INACTIVECAPTIONTEXT));


	//All other keys color
	else
		SetTextColor(bhdc, GetSysColor(COLOR_BTNTEXT));  //COLOR_BTNTEXT

	GetTextMetrics(bhdc, &tm);
	outheight = tm.tmHeight + tm.tmExternalLeading;
	outwidth = tm.tmAveCharWidth * outsize;

	px =(int) (((float)((brect.right -brect.left) - outwidth + 0) / 2.0) +
               ((float)(tm.tmAveCharWidth * bpush)/3.0));

	py =(int) (((float)((brect.bottom -brect.top) - outheight) / 1.5));


    //Special case, these letters are fatter
    switch (*outstr)
        {
    case 'W':
        px -= 2;
    break;

    case 'M':
    case 'm':
        px -= 1;
    break;

    case '%':
        px -= 3;
    break;
        }


	TextOut(bhdc, px, py, outstr, outsize);
	SetBkMode(bhdc, oldBkMode);


	if((Prefusesound == TRUE) && (bpush != 4))
    {
		if(bpush !=0)
			lastDown = TRUE;
		else if((bpush == 0) && (lastDown == TRUE))
			lastDown = FALSE;
    }


	SelectObject(bhdc, oldFontHdle);
	DeleteObject(hFont);



} // vPrintCenter

/****************************************************************************/
//Use ToUnicodeEx to get the key text.
//Check the return value is within the range of alpha
//If not, set off the CONTROL and ALT in the keyboard state  kbuf[CONTROL],
//kbuf[MENU]
/****************************************************************************/
int GetKeyText(UINT vk, UINT sc, BYTE *kbuf, TCHAR *cbuf, HKL hkl)
{	int result;
	int i;

#ifdef UNICODE
	result=ToUnicodeEx(vk, sc, kbuf, cbuf, 30, 0, hkl);
#else
	result=ToAsciiEx(vk, sc, kbuf, (LPWORD)cbuf, 0, hkl);
#endif

	//Handle the dead key
	//Currently some bug in ToUnicode, we need to call ToUnicode twice to reset on
	//the dead key in the keyboard buffer.
	if(result < 0 || result == 2)
	{	

#ifdef UNICODE
		ToUnicodeEx(vk, sc, kbuf, cbuf, 30, 0, hkl);
#else
		ToAsciiEx(vk, sc, kbuf, (LPWORD)cbuf, 0, hkl);
#endif
		
		//There are more than one char get copy into the buffer if it is a dead key
		//We need to clean up the buffer
		for(i=1; i<30; i++)
			cbuf[i] = 0;
		return result;
	}

	if(iswalpha(cbuf[0]) == 0)  //Not in the range of alpha
	{
		kbuf[VK_CONTROL] = 0;   //Set the Control off
		
#ifdef UNICODE
		result = ToUnicodeEx(vk, sc, kbuf, cbuf, 30, 0, hkl);
#else
		result = ToAsciiEx(vk, sc, kbuf, (LPWORD)cbuf, 0, hkl);
#endif
		
		return result;
	}

	else
		return result;

}
/****************************************************************************/
//Redraw the num lock key.
//Toggole it stay hilite or off
/****************************************************************************/
BOOL RedrawNumLock(void)
{	int i;
	int bRet=0;
	

	for(i=1; i<lenKBkey; i++)
	{	
		if(KBkey[i].ktype == NUMLOCK_TYPE)
		{
			if(LOBYTE(GetKeyState(VK_NUMLOCK)) &0x01)   //Toggled (ON)
			{
				SetWindowLong(lpkeyhwnd[i], 0, 4);	
				DeleteObject((HGDIOBJ)SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, (LONG_PTR)CreateSolidBrush(COLOR_WINDOWTEXT)));
				
				bRet = 1;
			}
			else
			{	SetWindowLong(lpkeyhwnd[i], 0, 0);
                SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, COLOR_WINDOW);	

				bRet = 0;
			}
			
			InvalidateRect(lpkeyhwnd[i], NULL, TRUE);
			
			break;
		}
	}
	return bRet;
}
/****************************************************************************/
//Redraw the scroll lock key.
//Toggole it stay hilite or off
/****************************************************************************/
BOOL RedrawScrollLock(void)
{	int i;
	int bRet=0;
	

	for(i=1; i<lenKBkey; i++)
	{	if(KBkey[i].ktype == SCROLLOCK_TYPE)
		{
			if(LOBYTE(GetKeyState(VK_SCROLL)) &0x01)   //Toggled (ON)
			{
				SetWindowLong(lpkeyhwnd[i], 0, 4);	
				DeleteObject((HGDIOBJ)SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, (LONG_PTR)CreateSolidBrush(RGB(0,0,0))));
				
				bRet = 1;
			}
			else
			{	SetWindowLong(lpkeyhwnd[i], 0, 0);
                SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, COLOR_WINDOW);	

				bRet = 0;
			}
			
			InvalidateRect(lpkeyhwnd[i], NULL, TRUE);
			
			break;
		}
	}
	return bRet;
}
/****************************************************************************/

HFONT	ReSizeFont(int index, LOGFONT *plf, int outsize)
{
	static int    firsttime=1;
	static int    FontHeight=-12;
	static float  Scale=1.0;
	static float  UpRatio=0.0, DnRatio=0.0;

    HFONT    hFont=NULL;        // Handle of the selected font
	LOGFONT  smallLF;
	float    Scale1=0.0;
	int      delta=0;
	RECT     rect;



	//use smaller font
	if(outsize >= 2 && KBkey[index].ktype != 2 && index !=30 && index != 38 &&
       index !=71 )
	{
		GetClientRect(kbmainhwnd, &rect);
		Scale1= (float)(rect.right - rect.left);

		if(Scale1/Scale >= UpRatio)
			delta= -2;
		else if(Scale1/Scale <= DnRatio)
			delta= +2;

		smallLF.lfHeight= FontHeight +2;       // + delta;
		smallLF.lfWidth= 0;
		smallLF.lfEscapement= 0;
		smallLF.lfOrientation= 0;
		smallLF.lfWeight= 700;
		smallLF.lfItalic= '\0';
		smallLF.lfUnderline= '\0';
		smallLF.lfStrikeOut= '\0';
		smallLF.lfCharSet= plf->lfCharSet;  // '\0'
		smallLF.lfOutPrecision= '\x01';
		smallLF.lfClipPrecision= '\x02';
		smallLF.lfQuality= '\x01';
		smallLF.lfPitchAndFamily= DEFAULT_PITCH || FF_DONTCARE;  //'"';

        lstrcpy(smallLF.lfFaceName, plf->lfFaceName);

		hFont = CreateFontIndirect(&smallLF);

		return hFont;
	}
	else if(newFont == TRUE)
	{	
        hFont = CreateFontIndirect(plf);
		return hFont;
	}
    return hFont;
}


/**********************************************************************/
/*  BOOL ChooseNewFont( HWND hWnd )*/
/**********************************************************************/
BOOL ChooseNewFont(HWND hWnd)
{
	CHOOSEFONT   chf;

	chf.hDC = NULL;
	chf.lStructSize = sizeof(CHOOSEFONT);
	chf.hwndOwner = NULL;    
	chf.lpLogFont = plf;
	chf.Flags = CF_SCREENFONTS | CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT;
	chf.rgbColors = PrefTextKeyColor; //RGB(0, 0, 0);
	chf.lCustData = 0;
	chf.hInstance = (HANDLE)hInst;
	chf.lpszStyle = (LPTSTR)NULL;
	chf.nFontType = SCREEN_FONTTYPE;
	chf.nSizeMin = 0;
	chf.nSizeMax = 14;
	chf.lpfnHook = (LPCFHOOKPROC)(FARPROC)NULL;
	chf.lpTemplateName = (LPTSTR)NULL;


	if( ChooseFont(&chf) == FALSE )
    {
		return FALSE;
    }

	newFont = TRUE;
	/*if(PrefTextKeyColor != chf.rgbColors)
		PrefTextKeyColor = chf.rgbColors;*/

    kbPref->lf.lfHeight      = plf->lfHeight;
    kbPref->lf.lfWidth       = plf->lfWidth;
    kbPref->lf.lfEscapement  = plf->lfEscapement;
    kbPref->lf.lfOrientation = plf->lfOrientation;
    kbPref->lf.lfWeight      = plf->lfWeight;
    kbPref->lf.lfItalic      = plf->lfItalic;
    kbPref->lf.lfUnderline   = plf->lfUnderline;
    kbPref->lf.lfStrikeOut   = plf->lfStrikeOut;
    kbPref->lf.lfCharSet     = plf->lfCharSet;
    kbPref->lf.lfOutPrecision  = plf->lfOutPrecision;
    kbPref->lf.lfClipPrecision = plf->lfClipPrecision;
    kbPref->lf.lfQuality       = plf->lfQuality;
    kbPref->lf.lfPitchAndFamily= plf->lfPitchAndFamily;

#ifdef UNICODE
    wsprintfA(kbPref->lf.lfFaceName, "%ls", plf->lfFaceName);
#else
    wsprintfA(kbPref->lf.lfFaceName, "%hs", plf->lfFaceName);
#endif

	return (TRUE);
} //ChooseNewFont


/**********************************************************************/
/*  BOOL RDrawIcon(HDC hDC, char *pIconName)*/
/**********************************************************************/
BOOL RDrawIcon(HDC hDC, TCHAR *pIconName, int bpush, RECT rect)
{
	HICON hIcon;
	BOOL iret;
    int rx, ry, Ox, Oy;

    rx = rect.right - rect.left;
    ry = rect.bottom - rect.top;


    hIcon = LoadImage(hInst, pIconName, IMAGE_ICON, 0, 0,
                     LR_DEFAULTSIZE|LR_SHARED);

	if(hIcon == NULL)
	{
		SendErrorMessage(IDS_CANNOT_LOAD_ICON);
		return FALSE;
	}

	SetMapMode(hDC,MM_TEXT);

    //Find out where is the top left corner to place the icon

    Ox = (int)(rx/2) - 16;
    Oy = (int)(ry/2) - 16;

    //Draw the icon (Draw in center)
    iret = DrawIconEx(hDC, Ox, Oy, hIcon, 0,0,0, NULL, DI_NORMAL);

	return iret;

} //RDrawIcon

// a-anilk added
BOOL RDrawBitMap(HDC hDC, TCHAR *pIconName, int bpush, RECT rect, BOOL transform)
{
	HBITMAP hBitMap;
	BOOL iret;
    SIZE sz;
	HDC hDC1;
    int rx, ry, Ox, Oy;
	DWORD err;
	COLORREF clrIn, clrTx;

    rx = rect.right - rect.left - 2;
    ry = rect.bottom - rect.top;
	
	SetMapMode(hDC,MM_TEXT);

	hDC1 = CreateCompatibleDC(hDC);

	clrIn = GetSysColor(COLOR_INACTIVECAPTION);
	clrTx = GetSysColor(COLOR_INACTIVECAPTIONTEXT);

	hBitMap = LoadImage(hInst, pIconName, IMAGE_BITMAP, 0, 0,
		                 LR_DEFAULTSIZE | LR_SHARED );

	if ( transform )
	{
		// convert the background and text to match inactive title
		// and inactive text color

		// Take care not to overwrite each other and also skip if you donot need any transformation.

		if ( clrIn == RGBWHITE )
		{
			// Then reverse the process
			ChangeBitmapColor (hBitMap, RGBWHITE, clrTx, NULL);
			ChangeBitmapColor (hBitMap, RGBBACK, clrIn, NULL);
		}
		else
		{
			if ( RGBBACK != clrIn)
				ChangeBitmapColor (hBitMap, RGBBACK, clrIn, NULL);

			if ( RGBWHITE != clrTx)
				ChangeBitmapColor (hBitMap, RGBWHITE, clrTx, NULL);
		}
	}

	
	SelectObject(hDC1, hBitMap);

	Ox = (int)(rx/2) - 16;
    Oy = (int)(ry/2) - 16;

    //Leave 1 pixels for drawing border
    iret = StretchBlt(hDC, 1, 1, rx, ry, hDC1, 0, 0, rx, ry, SRCCOPY);

	DeleteDC(hDC1);

	return iret;
	
} //RDrawBitMap


/**************************************************************************/
/* void DeleteChildBackground(void)                                           */
/**************************************************************************/
void DeleteChildBackground(void)
{
	register int i;


	for (i = 1; i < lenKBkey; i++)
		{
		switch (KBkey[i].ktype)
			{
			case KNORMAL_TYPE:
				SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, COLOR_WINDOW);
								//(LONG_PTR)CreateSolidBrush(PrefCharKeyColor)));
			break;
			
			case KMODIFIER_TYPE:
				SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, COLOR_MENU);
								//(LONG_PTR)CreateSolidBrush(PrefModifierKeyColor)));
			break;
			
			case KDEAD_TYPE:
				SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, COLOR_MENU);
			break;
			
			case LED_NUMLOCK_TYPE:
				SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, COLOR_GRAYTEXT);
			break;
			
			case LED_CAPSLOCK_TYPE:
				SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, COLOR_GRAYTEXT);
			break;

			case LED_SCROLLLOCK_TYPE:
				SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, COLOR_GRAYTEXT);
			break;

			}
		}
}// DeleteBackground

/****************************************************************************/
//Draw the keys LED light
/****************************************************************************/
/*
void DrawIcon_KeyLight(HDC hDC, int which, RECT rect)
{
    HICON hIcon;
    TCHAR sIconName[]=TEXT("LED_LIGHT");


    switch (which)
        {
    case KB_CAPLOCK:

        if(LOBYTE(GetKeyState(VK_CAPITAL))== FALSE)
            return;

    break;

    case KB_NUMLOCK:

        if(LOBYTE(GetKeyState(VK_NUMLOCK))== FALSE)
            return;

    break;

    case KB_SCROLL:

        if(LOBYTE(GetKeyState(VK_SCROLL))== FALSE)
            return;

    break;

        }

    hIcon = LoadImage(hInst,&sIconName[0], IMAGE_ICON, rect.right, rect.bottom,
                      LR_SHARED);

    if(hIcon == NULL)
    {
        SendErrorMessage(IDS_CANNOT_LOAD_ICON);
        return;
    }

    SetMapMode(hDC,MM_TEXT);

    DrawIconEx(hDC, 2, 2, hIcon, rect.right, rect.bottom, 0, NULL, DI_NORMAL);

}
*/
/*****************************************************************************/
//Redraw the keys when Shift/Cap being pressed or released
/*****************************************************************************/
void RedrawKeys(void)
{
    int i;

    for(i = 1; i <= lenKBkey; i++)
    {
        if((KBkey[i].Caps_Redraw == REDRAW))
           InvalidateRect(lpkeyhwnd[i], NULL, TRUE);
    }

}

/*****************************************************************************/
//Redraw the Num Pad Keys
/*****************************************************************************/
void RedrawNumPadKeys(void)
{	register int i;

	for (i = 1; i <= lenKBkey; i++)
		if(KBkey[i].print==3)
			InvalidateRect(lpkeyhwnd[i], NULL, TRUE);

}

/*****************************************************************************/
//Round the coner of each key
/*****************************************************************************/
void SetKeyRegion(HWND hwnd, int w, int h)
{	
	HRGN hRgn;   //region handle


	hRgn = CreateRoundRectRgn(1, 1, w, h, 5, 2);
	SetWindowRgn(hwnd, hRgn, TRUE);


}
/*
typedef struct MyBITMAP {
   LONG   bmType;
   LONG   bmWidth;
   LONG   bmHeight;
   LONG   bmWidthBytes;
   WORD   bmPlanes;
   WORD   bmBitsPixel;
   LPVOID bmBits;
} BITMAP;
*/
// a-anilk, Bitmap transformation
// BUG: BUG: Donot hardcode sizeof BITMAP == 24
void ChangeBitmapColor(HBITMAP hbmSrc, COLORREF rgbOld, COLORREF rgbNew, HPALETTE hPal)
{
	HDC hDC;
	HDC hdcMem;
	PBITMAP bmBits;
	DWORD err = sizeof(BITMAP);
	bmBits = (LPBITMAP)LocalAlloc(LMEM_FIXED, 24);

   if (hDC = GetDC(NULL))
   {
      if (hdcMem = CreateCompatibleDC(hDC))
      {
      //
      // Get the bitmap struct needed by ChangeBitmapColorDC()
      //
      GetObject(hbmSrc, 24, (LPBITMAP)bmBits);

	  err = GetLastError();
         //
         // Select our bitmap into the memory DC
         //
         hbmSrc = (HBITMAP) SelectObject(hdcMem, hbmSrc);

         // Select in our palette so our RGB references will come
         // out correctly

         if (hPal) {
            SelectPalette(hdcMem, hPal, FALSE);
            RealizePalette(hdcMem);
            }

         ChangeBitmapColorDC(hdcMem, bmBits, rgbOld, rgbNew);

         //
         // Unselect our bitmap before deleting the DC
         //
         hbmSrc = (HBITMAP) SelectObject(hdcMem, hbmSrc);

         DeleteDC(hdcMem);
      }

      ReleaseDC(NULL, hDC);
   }
   LocalFree(bmBits);
}                  /* ChangeBitmapColor()  */

void ChangeBitmapColorDC (HDC hdcBM, LPBITMAP lpBM,
                                     COLORREF rgbOld, COLORREF rgbNew)
{
   HDC hdcMask;
   HBITMAP hbmMask, hbmOld;
   HBRUSH hbrOld;

   if (!lpBM)
      return;

   //
   // if the bitmap is mono we have nothing to do
   //

   if (lpBM->bmPlanes == 1 && lpBM->bmBitsPixel == 1)
      return;

   //
   // To perform the color switching, we need to create a monochrome
   // "mask" which is the same size as our color bitmap, but has all
   // pixels which match the old color (rgbOld) in the bitmap set to 1.
   //
   // We then use the ROP code "DSPDxax" to Blt our monochrome
   // bitmap to the color bitmap.  "D" is the Destination color
   // bitmap, "S" is the source monochrome bitmap, and "P" is the
   // selected brush (which is set to the replacement color (rgbNew)).
   // "x" and "a" represent the XOR and AND operators, respectively.
   //
   // The DSPDxax ROP code can be explained as having the following
   // effect:
   //
   // "Every place the Source bitmap is 1, we want to replace the
   // same location in our color bitmap with the new color.  All
   // other colors we leave as is."
   //
   // The truth table for DSPDxax is as follows:
   //
   //       D S P Result
   //       - - - ------
   //       0 0 0   0
   //       0 0 1   0
   //       0 1 0   0
   //       0 1 1   1
   //       1 0 0   1
   //       1 0 1   1
   //       1 1 0   0
   //       1 1 1   1
   //
   // (Even though the table is assuming monochrome D (Destination color),
   // S (Source color), & P's (Pattern color), the results apply to color
   // bitmaps also).
   //
   // By examining the table, every place that the Source is 1
   // (source bitmap contains a 1), the result is equal to the
   // Pattern at that location.  Where S is zero, the result equals
   // the Destination.
   //
   // See Section 11.2 (page 11-4) of the "Reference -- Volume 2" for more
   // information on the Termary Raster Operation codes.
   //

   if (hbmMask = CreateBitmap(lpBM->bmWidth, lpBM->bmHeight, 1, 1, NULL))
   {
      if (hdcMask = CreateCompatibleDC(hdcBM))
      {
      //
      // Select th mask bitmap into the mono DC
      //
         hbmOld = (HBITMAP) SelectObject(hdcMask, hbmMask);

         //
         // Create the brush and select it into the source color DC --
         // this is our "Pattern" or "P" color in our DSPDxax ROP.
         //

         hbrOld = (HBRUSH) SelectObject(hdcBM, CreateSolidBrush(rgbNew));

         //
         // To create the mask, we will use a feature of BitBlt -- when
         // converting from Color to Mono bitmaps, all Pixels of the
         // background colors are set to WHITE (1), and all other pixels
         // are set to BLACK (0).  So all pixels in our bitmap that are
         // rgbOld color, we set to 1.
         //

         SetBkColor(hdcBM, rgbOld);
         BitBlt(hdcMask, 0, 0, lpBM->bmWidth, lpBM->bmHeight,
                hdcBM, 0, 0, SRCCOPY);

         //
         // Where the mask is 1, lay down the brush, where it is 0, leave
         // the destination.
         //

         SetBkColor(hdcBM, RGBWHITE);
         SetTextColor(hdcBM, RGBBLACK);

         BitBlt(hdcBM, 0, 0, lpBM->bmWidth, lpBM->bmHeight,
                hdcMask, 0, 0, DSPDxax);

         SelectObject(hdcMask, hbmOld);

         hbrOld = (HBRUSH) SelectObject(hdcBM, hbrOld);
         DeleteObject(hbrOld);

         DeleteDC(hdcMask);
      }
      else
         return;

      DeleteObject(hbmMask);
   }
   else
      return;
}                  /* ChangeBitmapColorDC()  */

