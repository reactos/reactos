/*
 * nls/cptable.h
 */

#ifndef __NLS_CPTABLE_H
#define __NLS_CPTABLE_H

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <kernel32/lctable.h>


#define CODEPAGE_ANSI	1
#define CODEPAGE_OEM	2
#define CODEPAGE_MAC	3
#define CODEPAGE_EBCDIC 4
#define CODEPAGE_DBCS	0x10



typedef struct __CURRENCYFMTA
{
   UINT		NumDigits;
   UINT		LeadingZero;
   UINT		Grouping;
   LPSTR	lpDecimalSep;
   LPSTR	lpThousandSep;
   UINT		NegativeOrder;
   UINT		PositiveOrder;
   LPSTR	lpCurrencySymbol;
} CURRENCYFMTA, *PCURRENCYFMTA, *LPCURRENCYFMTA;

typedef struct __CURRENCYFMTW
{
   UINT		NumDigits;
   UINT		LeadingZero;
   UINT		Grouping;
   LPWSTR	lpDecimalSep;
   LPWSTR	lpThousandSep;
   UINT		NegativeOrder;
   UINT		PositiveOrder;
   LPWSTR	lpCurrencySymbol;
} CURRENCYFMTW, *PCURRENCYFMTW, *LPCURRENCYFMTW;

typedef struct __NUMBERFMTA
{ 
   UINT		NumDigits; 
   UINT		LeadingZero; 
   UINT		Grouping; 
   LPSTR	lpDecimalSep; 
   LPSTR	lpThousandSep; 
   UINT		NegativeOrder; 
} NUMBERFMTA, *PNUMBERFMTA, *LPNUMBERFMTA; 

typedef struct __NUMBERFMTW
{ 
   UINT		NumDigits; 
   UINT		LeadingZero; 
   UINT		Grouping; 
   LPWSTR	lpDecimalSep; 
   LPWSTR	lpThousandSep; 
   UINT		NegativeOrder; 
} NUMBERFMTW, *PNUMBERFMTW, *LPNUMBERFMTW; 
 
typedef struct __CODEPAGE
{
   struct __CODEPAGE *Next;
   INT		Id;
   DWORD	Flags; 
   WCHAR	**ToUnicode;
   WCHAR	**ToUnicodeGlyph;
   CHAR		***FromUnicode;  
   LPCPINFO	Info;
} CODEPAGE, *PCODEPAGE;

extern PCODEPAGE __CPFirst;

extern CODEPAGE __CP37;
extern CODEPAGE __CP437;
extern CODEPAGE __CP500;
extern CODEPAGE __CP737;
extern CODEPAGE __CP775;
extern CODEPAGE __CP850;
extern CODEPAGE __CP852;
extern CODEPAGE __CP855;
extern CODEPAGE __CP857;
extern CODEPAGE __CP860;
extern CODEPAGE __CP861;
extern CODEPAGE __CP863;
extern CODEPAGE __CP865;
extern CODEPAGE __CP866;
extern CODEPAGE __CP869;
extern CODEPAGE __CP875;
extern CODEPAGE __CP1026;
extern CODEPAGE __CP1250;
extern CODEPAGE __CP1251;
extern CODEPAGE __CP1252;
extern CODEPAGE __CP1253;
extern CODEPAGE __CP1254;
extern CODEPAGE __CP1255;
extern CODEPAGE __CP1256;
extern CODEPAGE __CP1257;
extern CODEPAGE __CP1258;
extern CODEPAGE __CP10000;
extern CODEPAGE __CP10006;
extern CODEPAGE __CP10007;
extern CODEPAGE __CP10029;
extern CODEPAGE __CP10079;
extern CODEPAGE __CP10081;

extern CPINFO __CPGenInfo;

extern WCHAR __ASCII_00[32];
extern WCHAR __ASCII_20[32];
extern WCHAR __ASCII_40[32];
extern WCHAR __ASCII_60[32];

extern CHAR  __ASCII_0000[32];
extern CHAR  __ASCII_0020[32];
extern CHAR  __ASCII_0040[32];
extern CHAR  __ASCII_0060[32];

extern WCHAR __NULL_00[32];
extern CHAR  __NULL_0000[32];
extern CHAR  *__NULL_00XX[32];

#endif