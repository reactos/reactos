//
// Copyright (c) 1997-1999 Microsoft Corporation.
//
#include <windows.h>

#define  NUM_VOWEL   (sizeof(Vowel)/sizeof(char*))
#define  NUM_TRAIL   (sizeof(Trail)/sizeof(char*))

#define  CJK1_BEG   0x4e00
#define  CJK1_END   0x9fa5
#define  ISCJK1(x)  ((x >= CJK1_BEG) && (x <= CJK1_END))

#define  CJK2_BEG   0xf900
#define  CJK2_END   0xfa2d
#define  ISCJK2(x)  ((x >= CJK2_BEG) && (x <= CJK2_END))

#define  EUDC_BEG   0xe000
#define  EUDC_END   0xf8ff
#define  ISEUDC(x)  ((x >= EUDC_BEG) && (x <= EUDC_END))

#define  HANGUL_BEG 0xac00
#define  HANGUL_END 0xd7a3
#define  ISHANGUL(x)  ((x >= HANGUL_BEG) && (x <= HANGUL_END))

#define  UNDEFINED    0
#define  EUDC         1
#define  CJK1         2
#define  CJK2         3
#define  HANGUL       4
#define  OTHERS       5

#define  IDS_UNAME    0

#define  IDS_HANGULSYL     0xe000
#define  IDS_CJKUNDEFIDEO  0xe001
#define  IDS_CJKCOMPIDEO   0xe002
#define  IDS_PRIVATECHAR   0xe003
#define  IDS_UNDEFINED     0xe004

HANDLE hinstDll;

int APIENTRY GetUName(WORD wChar, LPTSTR lpUName);
