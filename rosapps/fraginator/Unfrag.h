/*****************************************************************************

  Unfrag

*****************************************************************************/


#ifndef UNFRAG_H
#define UNFRAG_H


// Blah blah blah your template name is too long ... SO WHAT
#pragma warning (disable: 4786)


// I forget what this disables
#ifdef __ICL
#pragma warning (disable: 268)
#endif


// Hello Mr. Platform SDK, please let us use Windows 2000 only features
#ifndef WINVER
#define WINVER 0x0500
#define _WIN32_WINNT 0x0500
#endif


#include <windows.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>


#define APPNAME_CLI   L"Unfrag"
#define APPNAME_GUI   L"Fraginator"
#define APPVER_STR    L"1.03"
#define APPVER_NUM     1.03f
#define APPAUTHOR     L"Rick Brewster"
#define APPCOPYRIGHT  L"Copyright 2000-2002 Rick Brewster"


#include <vector>
#include <string>
using namespace std;


typedef unsigned __int8  uint8;
typedef signed __int8    sint8;
typedef unsigned __int16 uint16;
typedef signed __int16   sint16;
typedef unsigned __int32 uint32;
typedef signed __int32   sint32;
typedef unsigned __int64 uint64;
typedef signed __int64   sint64;
typedef unsigned char    uchar;


extern bool QuietMode;
extern bool VerboseMode;


typedef enum
{
    DefragInvalid,
    DefragFast,
    DefragExtensive,
    DefragAnalyze
} DefragType;


extern bool CheckWinVer (void);


class Defragment;
extern Defragment *StartDefragThread (wstring Drive, DefragType Method, HANDLE &Handle);


extern wchar_t *AddCommas (wchar_t *Result, uint64 Number);


#endif // UNFRAG_H

