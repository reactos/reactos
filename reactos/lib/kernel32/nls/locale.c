/*
 * COPYRIGHT:       See COPYING in the top level directory
		    Addition copyrights might be specified in LGPL.c
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/nls/locale.c
 * PURPOSE:         National language support functions
 * PROGRAMMER:      Boudewijn ( ariadne@xs4all.nl)
 * UPDATE HISTORY:  Modified from Onno Hovers wfc. ( 08/02/99 )
		    Modified from wine. ( 08/02/99 )
 *                  
 */

/*
 * nls/locale.c
 *
 */
/*
 *	OLE2NLS library
 *
 *	Copyright 1995	Martin von Loewis
 *      Copyright 1998  David Lee Lambert
 */
#undef WIN32_LEAN_AND_MEAN
#include<stdlib.h>
#include<string.h>
#include<windows.h>
#include <wchar.h>

#include <kernel32/thread.h>
#include <kernel32/nls.h>

#undef tolower
#undef toupper
#undef isupper
#undef islower
#undef isalnum
#undef isalpha
#undef isblank
#undef isdigit

#undef towlower
#undef towupper
#undef iswupper
#undef iswlower
#undef iswalnum
#undef iswalpha

#define tolower(c) 	((c >= 'A' && c <= 'Z')   ? c - ( 'A' - 'a' ) : c)
#define toupper(c) 	((c >= 'a' && c <= 'z')   ? c + 'A' - 'a' : c);
#define isupper(c)	(c >= 'A' && c <= 'Z' )
#define islower(c)	(c >= 'a' && c <= 'z')
#define isalnum(c)	((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')  || (c >= '0' && c <= '9'))
#define isalpha(c)	(c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
#define isblank(c) 	( c == ' ' || c == '\t' )
#define isdigit(c)	((c >= '0' && c <= '9'))
#define isspace(c)	((c == ' '))
#define ispunct(c)	((c == '.'))
#define isxdigit(c)	((c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')  || (c >= '0' && c <= '9'))
#define iscntrl(c)	((c >=0x00 && c <= 0x1f) || c == 0x7f)


#define towlower(c) 	((c >= L'A' && c <= L'Z')   ? c - ( L'A' - L'a' ) : c)
#define towupper(c) 	((c >= L'a' && c <= L'z')   ? c + L'A' - L'a' : c);
#define iswupper(c)	(c >= L'A' && c <= L'Z' )
#define iswlower(c)	(c >= L'a' && c <= L'z')
#define iswalnum(c)	((c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z')  || (c >= L'0' && c <= L'9'))
#define iswalpha(c)	(c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z')

/*

int OLE_GetFormatA(LCID locale,
			    DWORD flags,
			    DWORD tflags,
			    LPSYSTEMTIME xtime,
			    LPCSTR _format, 	
			    LPSTR date,		
			    int datelen);
*/

#undef LCID

#define SYSTEM_DEFAULT_LANGUAGE		LANG_ENGLISH
#define SYSTEM_DEFAULT_SUBLANGUAGE	SUBLANG_ENGLISH_US
#define SYSTEM_DEFAULT_SORTORDER	SORT_DEFAULT


PLOCALE __UserLocale;
PLOCALE __TebLocale;
LOCALE  __Locale[LOCALE_ARRAY];

WINBOOL __LocaleInit(void)
{
   PSTR		locstr;   
   LCID		lcid;
   PLOCALE	plocale=NULL;
   
   locstr=getenv("WF_LOCALE");
   if(locstr)
   {  
      plocale=__Locale;
      while((plocale->Id)&&(strcasecmp(locstr,plocale->AbbrName)))
         plocale++;
   }
   /* if we do not have a locale, default */
   if(!plocale)
   {
      lcid=MAKELCID(MAKELANGID(SYSTEM_DEFAULT_LANGUAGE,
                               SYSTEM_DEFAULT_SUBLANGUAGE),
                      SYSTEM_DEFAULT_SORTORDER);
      plocale=__Locale;
      while((plocale->Id)&&(lcid!=plocale->Id))
         plocale++;                                                   
   }
   /* if this does not work, use our disaster plan */
   if(!plocale)
      plocale=__Locale;

   __UserLocale=plocale;
   __TebLocale=plocale;
   return TRUE;
}

LANGID STDCALL GetUserDefaultLangID(void)
{
   return LANGIDFROMLCID(__UserLocale->Id);
}

LCID
STDCALL
GetUserDefaultLCID(void)
{
   return __UserLocale->Id;
}

LANGID STDCALL GetSystemDefaultLangID(void)
{
   return MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US);
}

LCID STDCALL GetSystemDefaultLCID(void)
{
   return MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US), 
                   SORT_DEFAULT);
}

LCID STDCALL GetThreadLocale(void)
{
   return __TebLocale;
}

WINBOOL STDCALL SetThreadLocale(LCID  Locale)
{
   PLOCALE plocale;

   plocale=__Locale;   
   /* find locale */
   while((plocale->Id)&&(Locale!=plocale->Id))
      plocale++;      
   if(!plocale) {
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
   }
   __TebLocale=plocale;
   return TRUE;   
}

WINBOOL
STDCALL
IsValidLocale(
    LCID   Locale,
    DWORD  dwFlags)
{
   PLOCALE plocale;

   plocale=__Locale;   
   
   /* find locale */
   while((plocale->Id)&&(Locale!=plocale->Id))
      plocale++;     
   /* is it valid ?? */    
   if(!plocale)
      return FALSE;
   else
      return TRUE;   
}

LPSTR static __xtoa(LPSTR str, DWORD val)
{
   LPSTR retstr=str;
   DWORD hex;
   
   do
   {
      hex=val%16;
      if(hex<10)
         *str=val + '0';
      else
         *str=val + 'A';
      val=val/16;
      str++;
   }
   while(val);
   return retstr;
}

LPWSTR static __xtow(LPWSTR str, DWORD val)
{
   LPWSTR retstr=str;
   DWORD hex;
   
   do
   {
      hex=val%16;
      if(hex<10)
         *str=val + '0';
      else
         *str=val + 'A';
      val=val/16;
      str++;
   }
   while(val);
   return retstr;
}


WINBOOL
STDCALL
EnumSystemLocalesA(
    LOCALE_ENUMPROC lpLocaleEnumProc,
    DWORD            dwFlags)
{
   CHAR		locstr[10];
   BOOL		retval;
   PLOCALE	plocale;   
   
   if(!lpLocaleEnumProc) {
        SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
   }
      
   plocale=__Locale;
   retval=TRUE;
   while((plocale->Id)&&(retval))
   {
      __xtoa(locstr,plocale->Id); 
      retval=lpLocaleEnumProc((void *)locstr);
   }
   return TRUE;
}
WINBOOL
STDCALL
EnumSystemLocalesW(
    LOCALE_ENUMPROC lpLocaleEnumProc,
    DWORD            dwFlags)
{
   WCHAR	locstr[10];
   WINBOOL	retval=TRUE;
   PLOCALE	plocale=__Locale;  
   
   
   if(!lpLocaleEnumProc) {
        SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
   }
      
   while((plocale->Id)&&(retval))
   {
      __xtow(locstr,plocale->Id);
      retval=lpLocaleEnumProc((void *)locstr);
   }
   return TRUE;
}

int
STDCALL
GetLocaleInfoW(
    LCID     Locale,
    LCTYPE   LCType,
    LPWSTR  lpLCData,
    int      cchData)
{
   INT		retcnt=0;
   PLOCALE	plocale=__Locale;  
   LPWSTR	infostr; 
   
   while(plocale->Id!=Locale)
      plocale++;
   
   if(LCType<89)
      infostr=plocale->Info0[LCType];
   else if((LCType>0x1000)&&(LCType<0x1011))
      infostr=plocale->Info1[LCType-0x1000];
   else
      { SetLastError(ERROR_INVALID_PARAMETER); return 0; }
      
   if(cchData)
   {   
      /* I really need a wide string copy, here */
      do      
      {
         *lpLCData=*infostr;
         infostr++;
         lpLCData++;
         cchData--;
         retcnt++;
      }
      while((*infostr)&&(cchData));
   }
   else
   {
      /* I really need a wide string length, here */
      do
      {
         infostr++;
         retcnt++;
      }
      while(*infostr);
   }
   return retcnt;
}
int
STDCALL
GetLocaleInfoA(
    LCID     Locale,
    LCTYPE   LCType,
    LPSTR  lpLCData,
    int      cchData)
{
   INT		retcnt=0;
   PLOCALE	plocale=__Locale;  
   LPWSTR	infostr; 
   
   while(plocale->Id!=Locale)
      plocale++;
   
   if(LCType<89)
      infostr=plocale->Info0[LCType];
   else if((LCType>0x1000)&&(LCType<0x1011))
      infostr=plocale->Info1[LCType-0x1000];
   else
      { SetLastError(ERROR_INVALID_PARAMETER); return 0; }
      
   if(cchData)
   {   
      /* I really need a wide string copy, here */
      do      
      {
         *lpLCData=*infostr;
         infostr++;
         lpLCData++;
         cchData--;
         retcnt++;
      }
      while((*infostr)&&(cchData));
   }
   else
   {
      /* I really need a wide string length, here */
      do
      {
         infostr++;
         retcnt++;
      }
      while(*infostr);
   }
   return retcnt;
}

const struct map_lcid2str {
	LCID		langid;
	const char	*langname;
} languages[]={
	{0x0401,"Arabic (Saudi Arabia)"},
	{0x0801,"Arabic (Iraq)"},
	{0x0c01,"Arabic (Egypt)"},
	{0x1001,"Arabic (Libya)"},
	{0x1401,"Arabic (Algeria)"},
	{0x1801,"Arabic (Morocco)"},
	{0x1c01,"Arabic (Tunisia)"},
	{0x2001,"Arabic (Oman)"},
	{0x2401,"Arabic (Yemen)"},
	{0x2801,"Arabic (Syria)"},
	{0x2c01,"Arabic (Jordan)"},
	{0x3001,"Arabic (Lebanon)"},
	{0x3401,"Arabic (Kuwait)"},
	{0x3801,"Arabic (United Arab Emirates)"},
	{0x3c01,"Arabic (Bahrain)"},
	{0x4001,"Arabic (Qatar)"},
	{0x0402,"Bulgarian"},
	{0x0403,"Catalan"},
	{0x0404,"Chinese (Taiwan)"},
	{0x0804,"Chinese (People's Republic of China)"},
	{0x0c04,"Chinese (Hong Kong)"},
	{0x1004,"Chinese (Singapore)"},
	{0x1404,"Chinese (Macau)"},
	{0x0405,"Czech"},
	{0x0406,"Danish"},
	{0x0407,"German (Germany)"},
	{0x0807,"German (Switzerland)"},
	{0x0c07,"German (Austria)"},
	{0x1007,"German (Luxembourg)"},
	{0x1407,"German (Liechtenstein)"},
	{0x0408,"Greek"},
	{0x0409,"English (United States)"},
	{0x0809,"English (United Kingdom)"},
	{0x0c09,"English (Australia)"},
	{0x1009,"English (Canada)"},
	{0x1409,"English (New Zealand)"},
	{0x1809,"English (Ireland)"},
	{0x1c09,"English (South Africa)"},
	{0x2009,"English (Jamaica)"},
	{0x2409,"English (Caribbean)"},
	{0x2809,"English (Belize)"},
	{0x2c09,"English (Trinidad)"},
	{0x3009,"English (Zimbabwe)"},
	{0x3409,"English (Philippines)"},
	{0x040a,"Spanish (Spain, traditional sorting)"},
	{0x080a,"Spanish (Mexico)"},
	{0x0c0a,"Spanish (Spain, international sorting)"},
	{0x100a,"Spanish (Guatemala)"},
	{0x140a,"Spanish (Costa Rica)"},
	{0x180a,"Spanish (Panama)"},
	{0x1c0a,"Spanish (Dominican Republic)"},
	{0x200a,"Spanish (Venezuela)"},
	{0x240a,"Spanish (Colombia)"},
	{0x280a,"Spanish (Peru)"},
	{0x2c0a,"Spanish (Argentina)"},
	{0x300a,"Spanish (Ecuador)"},
	{0x340a,"Spanish (Chile)"},
	{0x380a,"Spanish (Uruguay)"},
	{0x3c0a,"Spanish (Paraguay)"},
	{0x400a,"Spanish (Bolivia)"},
	{0x440a,"Spanish (El Salvador)"},
	{0x480a,"Spanish (Honduras)"},
	{0x4c0a,"Spanish (Nicaragua)"},
	{0x500a,"Spanish (Puerto Rico)"},
	{0x040b,"Finnish"},
	{0x040c,"French (France)"},
	{0x080c,"French (Belgium)"},
	{0x0c0c,"French (Canada)"},
	{0x100c,"French (Switzerland)"},
	{0x140c,"French (Luxembourg)"},
	{0x180c,"French (Monaco)"},
	{0x040d,"Hebrew"},
	{0x040e,"Hungarian"},
	{0x040f,"Icelandic"},
	{0x0410,"Italian (Italy)"},
	{0x0810,"Italian (Switzerland)"},
	{0x0411,"Japanese"},
	{0x0412,"Korean (Wansung)"},
	{0x0812,"Korean (Johab)"},
	{0x0413,"Dutch (Netherlands)"},
	{0x0813,"Dutch (Belgium)"},
	{0x0414,"Norwegian (Bokmal)"},
	{0x0814,"Norwegian (Nynorsk)"},
	{0x0415,"Polish"},
	{0x0416,"Portuguese (Brazil)"},
	{0x0816,"Portuguese (Portugal)"},
	{0x0417,"Rhaeto Romanic"},
	{0x0418,"Romanian"},
	{0x0818,"Moldavian"},
	{0x0419,"Russian (Russia)"},
	{0x0819,"Russian (Moldavia)"},
	{0x041a,"Croatian"},
	{0x081a,"Serbian (latin)"},
	{0x0c1a,"Serbian (cyrillic)"},
	{0x041b,"Slovak"},
	{0x041c,"Albanian"},
	{0x041d,"Swedish (Sweden)"},
	{0x081d,"Swedish (Finland)"},
	{0x041e,"Thai"},
	{0x041f,"Turkish"},
	{0x0420,"Urdu"},
	{0x0421,"Indonesian"},
	{0x0422,"Ukrainian"},
	{0x0423,"Belarusian"},
	{0x0424,"Slovene"},
	{0x0425,"Estonian"},
	{0x0426,"Latvian"},
	{0x0427,"Lithuanian (modern)"},
	{0x0827,"Lithuanian (classic)"},
	{0x0428,"Maori"},
	{0x0429,"Farsi"},
	{0x042a,"Vietnamese"},
	{0x042b,"Armenian"},
	{0x042c,"Azeri (latin)"},
	{0x082c,"Azeri (cyrillic)"},
	{0x042d,"Basque"},
	{0x042e,"Sorbian"},
	{0x042f,"Macedonian"},
	{0x0430,"Sutu"},
	{0x0431,"Tsonga"},
	{0x0432,"Tswana"},
	{0x0433,"Venda"},
	{0x0434,"Xhosa"},
	{0x0435,"Zulu"},
	{0x0436,"Afrikaans"},
	{0x0437,"Georgian"},
	{0x0438,"Faeroese"},
	{0x0439,"Hindi"},
	{0x043a,"Maltese"},
	{0x043b,"Saami"},
	{0x043c,"Irish gaelic"},
	{0x083c,"Scottish gaelic"},
	{0x043e,"Malay (Malaysia)"},
	{0x083e,"Malay (Brunei Darussalam)"},
	{0x043f,"Kazak"},
	{0x0441,"Swahili"},
	{0x0443,"Uzbek (latin)"},
	{0x0843,"Uzbek (cyrillic)"},
	{0x0444,"Tatar"},
	{0x0445,"Bengali"},
	{0x0446,"Punjabi"},
	{0x0447,"Gujarati"},
	{0x0448,"Oriya"},
	{0x0449,"Tamil"},
	{0x044a,"Telugu"},
	{0x044b,"Kannada"},
	{0x044c,"Malayalam"},
	{0x044d,"Assamese"},
	{0x044e,"Marathi"},
	{0x044f,"Sanskrit"},
	{0x0457,"Konkani"},
	{0x048f,"Esperanto"}, /* Non official */
	{0x0000,"Unknown"}
    
};


static const unsigned char CT_CType2_LUT[] = {
  C2_NOTAPPLICABLE, /*   -   0 */
  C2_NOTAPPLICABLE, /*   -   1 */
  C2_NOTAPPLICABLE, /*   -   2 */
  C2_NOTAPPLICABLE, /*   -   3 */
  C2_NOTAPPLICABLE, /*   -   4 */
  C2_NOTAPPLICABLE, /*   -   5 */
  C2_NOTAPPLICABLE, /*   -   6 */
  C2_NOTAPPLICABLE, /*   -   7 */
  C2_NOTAPPLICABLE, /*   -   8 */
  C2_SEGMENTSEPARATOR, /*   -   9 */
  C2_NOTAPPLICABLE, /*   -  10 */
  C2_NOTAPPLICABLE, /*   -  11 */
  C2_NOTAPPLICABLE, /*   -  12 */
  C2_NOTAPPLICABLE, /*   -  13 */
  C2_NOTAPPLICABLE, /*   -  14 */
  C2_NOTAPPLICABLE, /*   -  15 */
  C2_NOTAPPLICABLE, /*   -  16 */
  C2_NOTAPPLICABLE, /*   -  17 */
  C2_NOTAPPLICABLE, /*   -  18 */
  C2_NOTAPPLICABLE, /*   -  19 */
  C2_NOTAPPLICABLE, /*   -  20 */
  C2_NOTAPPLICABLE, /*   -  21 */
  C2_NOTAPPLICABLE, /*   -  22 */
  C2_NOTAPPLICABLE, /*   -  23 */
  C2_NOTAPPLICABLE, /*   -  24 */
  C2_NOTAPPLICABLE, /*   -  25 */
  C2_NOTAPPLICABLE, /*   -  26 */
  C2_NOTAPPLICABLE, /*   -  27 */
  C2_NOTAPPLICABLE, /*   -  28 */
  C2_NOTAPPLICABLE, /*   -  29 */
  C2_NOTAPPLICABLE, /*   -  30 */
  C2_NOTAPPLICABLE, /*   -  31 */
  C2_WHITESPACE, /*   -  32 */
  C2_OTHERNEUTRAL, /* ! -  33 */
  C2_OTHERNEUTRAL, /* " -  34 */ /* " */
  C2_EUROPETERMINATOR, /* # -  35 */
  C2_EUROPETERMINATOR, /* $ -  36 */
  C2_EUROPETERMINATOR, /* % -  37 */
  C2_LEFTTORIGHT, /* & -  38 */
  C2_OTHERNEUTRAL, /* ' -  39 */
  C2_OTHERNEUTRAL, /* ( -  40 */
  C2_OTHERNEUTRAL, /* ) -  41 */
  C2_OTHERNEUTRAL, /* * -  42 */
  C2_EUROPETERMINATOR, /* + -  43 */
  C2_COMMONSEPARATOR, /* , -  44 */
  C2_EUROPETERMINATOR, /* - -  45 */
  C2_EUROPESEPARATOR, /* . -  46 */
  C2_EUROPESEPARATOR, /* / -  47 */
  C2_EUROPENUMBER, /* 0 -  48 */
  C2_EUROPENUMBER, /* 1 -  49 */
  C2_EUROPENUMBER, /* 2 -  50 */
  C2_EUROPENUMBER, /* 3 -  51 */
  C2_EUROPENUMBER, /* 4 -  52 */
  C2_EUROPENUMBER, /* 5 -  53 */
  C2_EUROPENUMBER, /* 6 -  54 */
  C2_EUROPENUMBER, /* 7 -  55 */
  C2_EUROPENUMBER, /* 8 -  56 */
  C2_EUROPENUMBER, /* 9 -  57 */
  C2_COMMONSEPARATOR, /* : -  58 */
  C2_OTHERNEUTRAL, /* ; -  59 */
  C2_OTHERNEUTRAL, /* < -  60 */
  C2_OTHERNEUTRAL, /* = -  61 */
  C2_OTHERNEUTRAL, /* > -  62 */
  C2_OTHERNEUTRAL, /* ? -  63 */
  C2_LEFTTORIGHT, /* @ -  64 */
  C2_LEFTTORIGHT, /* A -  65 */
  C2_LEFTTORIGHT, /* B -  66 */
  C2_LEFTTORIGHT, /* C -  67 */
  C2_LEFTTORIGHT, /* D -  68 */
  C2_LEFTTORIGHT, /* E -  69 */
  C2_LEFTTORIGHT, /* F -  70 */
  C2_LEFTTORIGHT, /* G -  71 */
  C2_LEFTTORIGHT, /* H -  72 */
  C2_LEFTTORIGHT, /* I -  73 */
  C2_LEFTTORIGHT, /* J -  74 */
  C2_LEFTTORIGHT, /* K -  75 */
  C2_LEFTTORIGHT, /* L -  76 */
  C2_LEFTTORIGHT, /* M -  77 */
  C2_LEFTTORIGHT, /* N -  78 */
  C2_LEFTTORIGHT, /* O -  79 */
  C2_LEFTTORIGHT, /* P -  80 */
  C2_LEFTTORIGHT, /* Q -  81 */
  C2_LEFTTORIGHT, /* R -  82 */
  C2_LEFTTORIGHT, /* S -  83 */
  C2_LEFTTORIGHT, /* T -  84 */
  C2_LEFTTORIGHT, /* U -  85 */
  C2_LEFTTORIGHT, /* V -  86 */
  C2_LEFTTORIGHT, /* W -  87 */
  C2_LEFTTORIGHT, /* X -  88 */
  C2_LEFTTORIGHT, /* Y -  89 */
  C2_LEFTTORIGHT, /* Z -  90 */
  C2_OTHERNEUTRAL, /* [ -  91 */
  C2_OTHERNEUTRAL, /* \ -  92 */
  C2_OTHERNEUTRAL, /* ] -  93 */
  C2_OTHERNEUTRAL, /* ^ -  94 */
  C2_OTHERNEUTRAL, /* _ -  95 */
  C2_OTHERNEUTRAL, /* ` -  96 */
  C2_LEFTTORIGHT, /* a -  97 */
  C2_LEFTTORIGHT, /* b -  98 */
  C2_LEFTTORIGHT, /* c -  99 */
  C2_LEFTTORIGHT, /* d - 100 */
  C2_LEFTTORIGHT, /* e - 101 */
  C2_LEFTTORIGHT, /* f - 102 */
  C2_LEFTTORIGHT, /* g - 103 */
  C2_LEFTTORIGHT, /* h - 104 */
  C2_LEFTTORIGHT, /* i - 105 */
  C2_LEFTTORIGHT, /* j - 106 */
  C2_LEFTTORIGHT, /* k - 107 */
  C2_LEFTTORIGHT, /* l - 108 */
  C2_LEFTTORIGHT, /* m - 109 */
  C2_LEFTTORIGHT, /* n - 110 */
  C2_LEFTTORIGHT, /* o - 111 */
  C2_LEFTTORIGHT, /* p - 112 */
  C2_LEFTTORIGHT, /* q - 113 */
  C2_LEFTTORIGHT, /* r - 114 */
  C2_LEFTTORIGHT, /* s - 115 */
  C2_LEFTTORIGHT, /* t - 116 */
  C2_LEFTTORIGHT, /* u - 117 */
  C2_LEFTTORIGHT, /* v - 118 */
  C2_LEFTTORIGHT, /* w - 119 */
  C2_LEFTTORIGHT, /* x - 120 */
  C2_LEFTTORIGHT, /* y - 121 */
  C2_LEFTTORIGHT, /* z - 122 */
  C2_OTHERNEUTRAL, /* { - 123 */
  C2_OTHERNEUTRAL, /* | - 124 */
  C2_OTHERNEUTRAL, /* } - 125 */
  C2_OTHERNEUTRAL, /* ~ - 126 */
  C2_NOTAPPLICABLE, /*  - 127 */
  C2_NOTAPPLICABLE, /* Ä - 128 */
  C2_NOTAPPLICABLE, /* Å - 129 */
  C2_OTHERNEUTRAL, /* Ç - 130 */
  C2_LEFTTORIGHT, /* É - 131 */
  C2_OTHERNEUTRAL, /* Ñ - 132 */
  C2_OTHERNEUTRAL, /* Ö - 133 */
  C2_OTHERNEUTRAL, /* Ü - 134 */
  C2_OTHERNEUTRAL, /* á - 135 */
  C2_LEFTTORIGHT, /* à - 136 */
  C2_EUROPETERMINATOR, /* â - 137 */
  C2_LEFTTORIGHT, /* ä - 138 */
  C2_OTHERNEUTRAL, /* ã - 139 */
  C2_LEFTTORIGHT, /* å - 140 */
  C2_NOTAPPLICABLE, /* ç - 141 */
  C2_NOTAPPLICABLE, /* é - 142 */
  C2_NOTAPPLICABLE, /* è - 143 */
  C2_NOTAPPLICABLE, /* ê - 144 */
  C2_OTHERNEUTRAL, /* ë - 145 */
  C2_OTHERNEUTRAL, /* í - 146 */
  C2_OTHERNEUTRAL, /* ì - 147 */
  C2_OTHERNEUTRAL, /* î - 148 */
  C2_OTHERNEUTRAL, /* ï - 149 */
  C2_OTHERNEUTRAL, /* ñ - 150 */
  C2_OTHERNEUTRAL, /* ó - 151 */
  C2_LEFTTORIGHT, /* ò - 152 */
  C2_OTHERNEUTRAL, /* ô - 153 */
  C2_LEFTTORIGHT, /* ö - 154 */
  C2_OTHERNEUTRAL, /* õ - 155 */
  C2_LEFTTORIGHT, /* ú - 156 */
  C2_NOTAPPLICABLE, /* ù - 157 */
  C2_NOTAPPLICABLE, /* û - 158 */
  C2_LEFTTORIGHT, /* ü - 159 */
  C2_WHITESPACE, /* † - 160 */
  C2_OTHERNEUTRAL, /* ° - 161 */
  C2_EUROPETERMINATOR, /* ¢ - 162 */
  C2_EUROPETERMINATOR, /* £ - 163 */
  C2_EUROPETERMINATOR, /* § - 164 */
  C2_EUROPETERMINATOR, /* • - 165 */
  C2_OTHERNEUTRAL, /* ¶ - 166 */
  C2_OTHERNEUTRAL, /* ß - 167 */
  C2_OTHERNEUTRAL, /* ® - 168 */
  C2_OTHERNEUTRAL, /* © - 169 */
  C2_OTHERNEUTRAL, /* ™ - 170 */
  C2_OTHERNEUTRAL, /* ´ - 171 */
  C2_OTHERNEUTRAL, /* ¨ - 172 */
  C2_OTHERNEUTRAL, /* ≠ - 173 */
  C2_OTHERNEUTRAL, /* Æ - 174 */
  C2_OTHERNEUTRAL, /* Ø - 175 */
  C2_EUROPETERMINATOR, /* ∞ - 176 */
  C2_EUROPETERMINATOR, /* ± - 177 */
  C2_EUROPENUMBER, /* ≤ - 178 */
  C2_EUROPENUMBER, /* ≥ - 179 */
  C2_OTHERNEUTRAL, /* ¥ - 180 */
  C2_OTHERNEUTRAL, /* µ - 181 */
  C2_OTHERNEUTRAL, /* ∂ - 182 */
  C2_OTHERNEUTRAL, /* ∑ - 183 */
  C2_OTHERNEUTRAL, /* ∏ - 184 */
  C2_EUROPENUMBER, /* π - 185 */
  C2_OTHERNEUTRAL, /* ∫ - 186 */
  C2_OTHERNEUTRAL, /* ª - 187 */
  C2_OTHERNEUTRAL, /* º - 188 */
  C2_OTHERNEUTRAL, /* Ω - 189 */
  C2_OTHERNEUTRAL, /* æ - 190 */
  C2_OTHERNEUTRAL, /* ø - 191 */
  C2_LEFTTORIGHT, /* ¿ - 192 */
  C2_LEFTTORIGHT, /* ¡ - 193 */
  C2_LEFTTORIGHT, /* ¬ - 194 */
  C2_LEFTTORIGHT, /* √ - 195 */
  C2_LEFTTORIGHT, /* ƒ - 196 */
  C2_LEFTTORIGHT, /* ≈ - 197 */
  C2_LEFTTORIGHT, /* ∆ - 198 */
  C2_LEFTTORIGHT, /* « - 199 */
  C2_LEFTTORIGHT, /* » - 200 */
  C2_LEFTTORIGHT, /* … - 201 */
  C2_LEFTTORIGHT, /*   - 202 */
  C2_LEFTTORIGHT, /* À - 203 */
  C2_LEFTTORIGHT, /* Ã - 204 */
  C2_LEFTTORIGHT, /* Õ - 205 */
  C2_LEFTTORIGHT, /* Œ - 206 */
  C2_LEFTTORIGHT, /* œ - 207 */
  C2_LEFTTORIGHT, /* – - 208 */
  C2_LEFTTORIGHT, /* — - 209 */
  C2_LEFTTORIGHT, /* “ - 210 */
  C2_LEFTTORIGHT, /* ” - 211 */
  C2_LEFTTORIGHT, /* ‘ - 212 */
  C2_LEFTTORIGHT, /* ’ - 213 */
  C2_LEFTTORIGHT, /* ÷ - 214 */
  C2_OTHERNEUTRAL, /* ◊ - 215 */
  C2_LEFTTORIGHT, /* ÿ - 216 */
  C2_LEFTTORIGHT, /* Ÿ - 217 */
  C2_LEFTTORIGHT, /* ⁄ - 218 */
  C2_LEFTTORIGHT, /* € - 219 */
  C2_LEFTTORIGHT, /* ‹ - 220 */
  C2_LEFTTORIGHT, /* › - 221 */
  C2_LEFTTORIGHT, /* ﬁ - 222 */
  C2_LEFTTORIGHT, /* ﬂ - 223 */
  C2_LEFTTORIGHT, /* ‡ - 224 */
  C2_LEFTTORIGHT, /* · - 225 */
  C2_LEFTTORIGHT, /* ‚ - 226 */
  C2_LEFTTORIGHT, /* „ - 227 */
  C2_LEFTTORIGHT, /* ‰ - 228 */
  C2_LEFTTORIGHT, /* Â - 229 */
  C2_LEFTTORIGHT, /* Ê - 230 */
  C2_LEFTTORIGHT, /* Á - 231 */
  C2_LEFTTORIGHT, /* Ë - 232 */
  C2_LEFTTORIGHT, /* È - 233 */
  C2_LEFTTORIGHT, /* Í - 234 */
  C2_LEFTTORIGHT, /* Î - 235 */
  C2_LEFTTORIGHT, /* Ï - 236 */
  C2_LEFTTORIGHT, /* Ì - 237 */
  C2_LEFTTORIGHT, /* Ó - 238 */
  C2_LEFTTORIGHT, /* Ô - 239 */
  C2_LEFTTORIGHT, /*  - 240 */
  C2_LEFTTORIGHT, /* Ò - 241 */
  C2_LEFTTORIGHT, /* Ú - 242 */
  C2_LEFTTORIGHT, /* Û - 243 */
  C2_LEFTTORIGHT, /* Ù - 244 */
  C2_LEFTTORIGHT, /* ı - 245 */
  C2_LEFTTORIGHT, /* ˆ - 246 */
  C2_OTHERNEUTRAL, /* ˜ - 247 */
  C2_LEFTTORIGHT, /* ¯ - 248 */
  C2_LEFTTORIGHT, /* ˘ - 249 */
  C2_LEFTTORIGHT, /* ˙ - 250 */
  C2_LEFTTORIGHT, /* ˚ - 251 */
  C2_LEFTTORIGHT, /* ¸ - 252 */
  C2_LEFTTORIGHT, /* ˝ - 253 */
  C2_LEFTTORIGHT, /* ˛ - 254 */
  C2_LEFTTORIGHT /* ˇ - 255 */
};

static const WORD CT_CType3_LUT[] = { 
  0x0000, /*   -   0 */
  0x0000, /*   -   1 */
  0x0000, /*   -   2 */
  0x0000, /*   -   3 */
  0x0000, /*   -   4 */
  0x0000, /*   -   5 */
  0x0000, /*   -   6 */
  0x0000, /*   -   7 */
  0x0000, /*   -   8 */
  0x0008, /*   -   9 */
  0x0008, /*   -  10 */
  0x0008, /*   -  11 */
  0x0008, /*   -  12 */
  0x0008, /*   -  13 */
  0x0000, /*   -  14 */
  0x0000, /*   -  15 */
  0x0000, /*   -  16 */
  0x0000, /*   -  17 */
  0x0000, /*   -  18 */
  0x0000, /*   -  19 */
  0x0000, /*   -  20 */
  0x0000, /*   -  21 */
  0x0000, /*   -  22 */
  0x0000, /*   -  23 */
  0x0000, /*   -  24 */
  0x0000, /*   -  25 */
  0x0000, /*   -  26 */
  0x0000, /*   -  27 */
  0x0000, /*   -  28 */
  0x0000, /*   -  29 */
  0x0000, /*   -  30 */
  0x0000, /*   -  31 */
  0x0048, /*   -  32 */
  0x0048, /* ! -  33 */
  0x0448, /* " -  34 */ /* " */
  0x0048, /* # -  35 */
  0x0448, /* $ -  36 */
  0x0048, /* % -  37 */
  0x0048, /* & -  38 */
  0x0440, /* ' -  39 */
  0x0048, /* ( -  40 */
  0x0048, /* ) -  41 */
  0x0048, /* * -  42 */
  0x0048, /* + -  43 */
  0x0048, /* , -  44 */
  0x0440, /* - -  45 */
  0x0048, /* . -  46 */
  0x0448, /* / -  47 */
  0x0040, /* 0 -  48 */
  0x0040, /* 1 -  49 */
  0x0040, /* 2 -  50 */
  0x0040, /* 3 -  51 */
  0x0040, /* 4 -  52 */
  0x0040, /* 5 -  53 */
  0x0040, /* 6 -  54 */
  0x0040, /* 7 -  55 */
  0x0040, /* 8 -  56 */
  0x0040, /* 9 -  57 */
  0x0048, /* : -  58 */
  0x0048, /* ; -  59 */
  0x0048, /* < -  60 */
  0x0448, /* = -  61 */
  0x0048, /* > -  62 */
  0x0048, /* ? -  63 */
  0x0448, /* @ -  64 */
  0x8040, /* A -  65 */
  0x8040, /* B -  66 */
  0x8040, /* C -  67 */
  0x8040, /* D -  68 */
  0x8040, /* E -  69 */
  0x8040, /* F -  70 */
  0x8040, /* G -  71 */
  0x8040, /* H -  72 */
  0x8040, /* I -  73 */
  0x8040, /* J -  74 */
  0x8040, /* K -  75 */
  0x8040, /* L -  76 */
  0x8040, /* M -  77 */
  0x8040, /* N -  78 */
  0x8040, /* O -  79 */
  0x8040, /* P -  80 */
  0x8040, /* Q -  81 */
  0x8040, /* R -  82 */
  0x8040, /* S -  83 */
  0x8040, /* T -  84 */
  0x8040, /* U -  85 */
  0x8040, /* V -  86 */
  0x8040, /* W -  87 */
  0x8040, /* X -  88 */
  0x8040, /* Y -  89 */
  0x8040, /* Z -  90 */
  0x0048, /* [ -  91 */
  0x0448, /* \ -  92 */
  0x0048, /* ] -  93 */
  0x0448, /* ^ -  94 */
  0x0448, /* _ -  95 */
  0x0448, /* ` -  96 */
  0x8040, /* a -  97 */
  0x8040, /* b -  98 */
  0x8040, /* c -  99 */
  0x8040, /* d - 100 */
  0x8040, /* e - 101 */
  0x8040, /* f - 102 */
  0x8040, /* g - 103 */
  0x8040, /* h - 104 */
  0x8040, /* i - 105 */
  0x8040, /* j - 106 */
  0x8040, /* k - 107 */
  0x8040, /* l - 108 */
  0x8040, /* m - 109 */
  0x8040, /* n - 110 */
  0x8040, /* o - 111 */
  0x8040, /* p - 112 */
  0x8040, /* q - 113 */
  0x8040, /* r - 114 */
  0x8040, /* s - 115 */
  0x8040, /* t - 116 */
  0x8040, /* u - 117 */
  0x8040, /* v - 118 */
  0x8040, /* w - 119 */
  0x8040, /* x - 120 */
  0x8040, /* y - 121 */
  0x8040, /* z - 122 */
  0x0048, /* { - 123 */
  0x0048, /* | - 124 */
  0x0048, /* } - 125 */
  0x0448, /* ~ - 126 */
  0x0000, /*  - 127 */
  0x0000, /* Ä - 128 */
  0x0000, /* Å - 129 */
  0x0008, /* Ç - 130 */
  0x8000, /* É - 131 */
  0x0008, /* Ñ - 132 */
  0x0008, /* Ö - 133 */
  0x0008, /* Ü - 134 */
  0x0008, /* á - 135 */
  0x0001, /* à - 136 */
  0x0008, /* â - 137 */
  0x8003, /* ä - 138 */
  0x0008, /* ã - 139 */
  0x8000, /* å - 140 */
  0x0000, /* ç - 141 */
  0x0000, /* é - 142 */
  0x0000, /* è - 143 */
  0x0000, /* ê - 144 */
  0x0088, /* ë - 145 */
  0x0088, /* í - 146 */
  0x0088, /* ì - 147 */
  0x0088, /* î - 148 */
  0x0008, /* ï - 149 */
  0x0400, /* ñ - 150 */
  0x0400, /* ó - 151 */
  0x0408, /* ò - 152 */
  0x0000, /* ô - 153 */
  0x8003, /* ö - 154 */
  0x0008, /* õ - 155 */
  0x8000, /* ú - 156 */
  0x0000, /* ù - 157 */
  0x0000, /* û - 158 */
  0x8003, /* ü - 159 */
  0x0008, /* † - 160 */
  0x0008, /* ° - 161 */
  0x0048, /* ¢ - 162 */
  0x0048, /* £ - 163 */
  0x0008, /* § - 164 */
  0x0048, /* • - 165 */
  0x0048, /* ¶ - 166 */
  0x0008, /* ß - 167 */
  0x0408, /* ® - 168 */
  0x0008, /* © - 169 */
  0x0400, /* ™ - 170 */
  0x0008, /* ´ - 171 */
  0x0048, /* ¨ - 172 */
  0x0408, /* ≠ - 173 */
  0x0008, /* Æ - 174 */
  0x0448, /* Ø - 175 */
  0x0008, /* ∞ - 176 */
  0x0008, /* ± - 177 */
  0x0000, /* ≤ - 178 */
  0x0000, /* ≥ - 179 */
  0x0408, /* ¥ - 180 */
  0x0008, /* µ - 181 */
  0x0008, /* ∂ - 182 */
  0x0008, /* ∑ - 183 */
  0x0408, /* ∏ - 184 */
  0x0000, /* π - 185 */
  0x0400, /* ∫ - 186 */
  0x0008, /* ª - 187 */
  0x0000, /* º - 188 */
  0x0000, /* Ω - 189 */
  0x0000, /* æ - 190 */
  0x0008, /* ø - 191 */
  0x8003, /* ¿ - 192 */
  0x8003, /* ¡ - 193 */
  0x8003, /* ¬ - 194 */
  0x8003, /* √ - 195 */
  0x8003, /* ƒ - 196 */
  0x8003, /* ≈ - 197 */
  0x8000, /* ∆ - 198 */
  0x8003, /* « - 199 */
  0x8003, /* » - 200 */
  0x8003, /* … - 201 */
  0x8003, /*   - 202 */
  0x8003, /* À - 203 */
  0x8003, /* Ã - 204 */
  0x8003, /* Õ - 205 */
  0x8003, /* Œ - 206 */
  0x8003, /* œ - 207 */
  0x8000, /* – - 208 */
  0x8003, /* — - 209 */
  0x8003, /* “ - 210 */
  0x8003, /* ” - 211 */
  0x8003, /* ‘ - 212 */
  0x8003, /* ’ - 213 */
  0x8003, /* ÷ - 214 */
  0x0008, /* ◊ - 215 */
  0x8003, /* ÿ - 216 */
  0x8003, /* Ÿ - 217 */
  0x8003, /* ⁄ - 218 */
  0x8003, /* € - 219 */
  0x8003, /* ‹ - 220 */
  0x8003, /* › - 221 */
  0x8000, /* ﬁ - 222 */
  0x8000, /* ﬂ - 223 */
  0x8003, /* ‡ - 224 */
  0x8003, /* · - 225 */
  0x8003, /* ‚ - 226 */
  0x8003, /* „ - 227 */
  0x8003, /* ‰ - 228 */
  0x8003, /* Â - 229 */
  0x8000, /* Ê - 230 */
  0x8003, /* Á - 231 */
  0x8003, /* Ë - 232 */
  0x8003, /* È - 233 */
  0x8003, /* Í - 234 */
  0x8003, /* Î - 235 */
  0x8003, /* Ï - 236 */
  0x8003, /* Ì - 237 */
  0x8003, /* Ó - 238 */
  0x8003, /* Ô - 239 */
  0x8000, /*  - 240 */
  0x8003, /* Ò - 241 */
  0x8003, /* Ú - 242 */
  0x8003, /* Û - 243 */
  0x8003, /* Ù - 244 */
  0x8003, /* ı - 245 */
  0x8003, /* ˆ - 246 */
  0x0008, /* ˜ - 247 */
  0x8003, /* ¯ - 248 */
  0x8003, /* ˘ - 249 */
  0x8003, /* ˙ - 250 */
  0x8003, /* ˚ - 251 */
  0x8003, /* ¸ - 252 */
  0x8003, /* ˝ - 253 */
  0x8000, /* ˛ - 254 */
  0x8003  /* ˇ - 255 */
};



WINBOOL
STDCALL
GetStringTypeExA(
    LCID     Locale,
    DWORD    dwInfoType,
    LPCSTR lpSrcStr,
    int      cchSrc,
    LPWORD   lpCharType)
{
	int	i;
	
	if ((lpSrcStr==NULL) || (lpCharType==NULL) || (lpSrcStr==(LPCSTR)lpCharType))
	{
	  SetLastError(ERROR_INVALID_PARAMETER);
	  return FALSE;
	}

	if (cchSrc==-1)
	  cchSrc=lstrlenA(lpSrcStr)+1;
	  
	switch (dwInfoType) {
	case CT_CTYPE1:
	  for (i=0;i<cchSrc;i++) 
	  {
	    lpCharType[i] = 0;
	    if (isdigit(lpSrcStr[i])) lpCharType[i]|=C1_DIGIT;
	    if (isalpha(lpSrcStr[i])) lpCharType[i]|=C1_ALPHA;
	    if (islower(lpSrcStr[i])) lpCharType[i]|=C1_LOWER;
	    if (isupper(lpSrcStr[i])) lpCharType[i]|=C1_UPPER;
	    if (isspace(lpSrcStr[i])) lpCharType[i]|=C1_SPACE;
	    if (ispunct(lpSrcStr[i])) lpCharType[i]|=C1_PUNCT;
	    if (iscntrl(lpSrcStr[i])) lpCharType[i]|=C1_CNTRL;
	    if (isblank(lpSrcStr[i])) lpCharType[i]|=C1_BLANK; 
	    if (isxdigit(lpSrcStr[i])) lpCharType[i]|=C1_XDIGIT; 
	}
	return TRUE;

	case CT_CTYPE2:
	  for (i=0;i<cchSrc;i++) 
	  {
	    lpCharType[i]=(WORD)CT_CType2_LUT[i];
	  }
	  return TRUE;

	case CT_CTYPE3:
	  for (i=0;i<cchSrc;i++) 
	  {
	    lpCharType[i]=CT_CType3_LUT[i];
	  }
	  return TRUE;

	default:
	  return FALSE;
	}
}

WINBOOL
STDCALL
GetStringTypeA(
    LCID     Locale,
    DWORD    dwInfoType,
    LPCSTR   lpSrcStr,
    int      cchSrc,
    LPWORD   lpCharType)
{
	return GetStringTypeExA(Locale, dwInfoType, lpSrcStr, cchSrc, lpCharType);
}

WINBOOL
STDCALL
GetStringTypeExW(
    LCID     Locale,
    DWORD    dwInfoType,
    LPCWSTR lpSrcStr,
    int      cchSrc,
    LPWORD   lpCharType)
{
	int	i;
	
	if ((lpSrcStr==NULL) || (lpCharType==NULL) || (lpSrcStr==(LPCWSTR)lpCharType))
	{
	  SetLastError(ERROR_INVALID_PARAMETER);
	  return FALSE;
	}

	if (cchSrc==-1)
	  cchSrc=lstrlenW(lpSrcStr)+1;
	  
	switch (dwInfoType) {
	case CT_CTYPE1:
	  for (i=0;i<cchSrc;i++) 
	  {
	    lpCharType[i] = 0;
	    if (isdigit(lpSrcStr[i])) lpCharType[i]|=C1_DIGIT;
	    if (iswalpha(lpSrcStr[i])) lpCharType[i]|=C1_ALPHA;
	    if (iswlower(lpSrcStr[i])) lpCharType[i]|=C1_LOWER;
	    if (iswupper(lpSrcStr[i])) lpCharType[i]|=C1_UPPER;
	    if (isspace(lpSrcStr[i])) lpCharType[i]|=C1_SPACE;
	    if (ispunct(lpSrcStr[i])) lpCharType[i]|=C1_PUNCT;
	    if (iscntrl(lpSrcStr[i])) lpCharType[i]|=C1_CNTRL;
	    if (isblank(lpSrcStr[i])) lpCharType[i]|=C1_BLANK; 
	    if (isxdigit(lpSrcStr[i])) lpCharType[i]|=C1_XDIGIT; 
	}
	return TRUE;

	case CT_CTYPE2:
	  for (i=0;i<cchSrc;i++) 
	  {
	    lpCharType[i]=(WORD)CT_CType2_LUT[i];
	  }
	  return TRUE;

	case CT_CTYPE3:
	  for (i=0;i<cchSrc;i++) 
	  {
	    lpCharType[i]=CT_CType3_LUT[i];
	  }
	  return TRUE;

	default:
	  return FALSE;
	}
}

WINBOOL
STDCALL
GetStringTypeW(
    DWORD    dwInfoType,
    LPCWSTR  lpSrcStr,
    int      cchSrc,
    LPWORD   lpCharType)
{
	LCID     Locale = GetThreadLocale();
	return GetStringTypeExW(Locale, dwInfoType, lpSrcStr, cchSrc, lpCharType);
}

DWORD
STDCALL
VerLanguageNameA(
        DWORD wLang,
        LPSTR szLang,
        DWORD nSize
        )
{
	int	i;
	int len;
	for (i=0;languages[i].langid!=0;i++)
		if (wLang==languages[i].langid)
			break;

	len = min(strlen(languages[i].langname),nSize);
	strncpy(szLang,languages[i].langname,len);
	return len;
}

DWORD
STDCALL
VerLanguageNameW(
        DWORD wLang,
        LPWSTR szLang,
        DWORD nSize
        )
{
	int	i,j;
	
	for (i=0;languages[i].langid!=0;i++)
		if (wLang==languages[i].langid)
			break;
	for(j=0;j<nSize && languages[i].langname[j] != 0 ;i++)
		szLang[j] = languages[i].langname[j];
	szLang[j] = 0;
	return strlen(languages[i].langname);
}




int
STDCALL
GetDateFormatW(
    LCID     Locale,
    DWORD    dwFlags,
    CONST SYSTEMTIME *lpDate,
    LPCWSTR lpFormat,
    LPWSTR  lpDateStr,
    int      cchDate)
{
	return 0;
}

/******************************************************************************
 *		GetDateFormatA	[KERNEL32.310]
 * Makes an ASCII string of the date
 *
 * This function uses format to format the date,  or,  if format
 * is NULL, uses the default for the locale.  format is a string
 * of literal fields and characters as follows:
 *
 * - d    single-digit (no leading zero) day (of month)
 * - dd   two-digit day (of month)
 * - ddd  short day-of-week name
 * - dddd long day-of-week name
 * - M    single-digit month
 * - MM   two-digit month
 * - MMM  short month name
 * - MMMM full month name
 * - y    two-digit year, no leading 0
 * - yy   two-digit year
 * - yyyy four-digit year
 * - gg   era string
 *
 */
int STDCALL GetDateFormatA(LCID locale,DWORD flags,
			      CONST SYSTEMTIME *xtime,
			      LPCSTR format, LPSTR date,int datelen) 
{
   
  char format_buf[40];
  LPCSTR thisformat;
  SYSTEMTIME t;
  LPSYSTEMTIME thistime;
  LCID thislocale;
  INT ret;


  
  thislocale = OLE2NLS_CheckLocale ( locale );

  if (xtime == NULL) {
     GetSystemTime(&t);
     thistime = &t;
  } else {
     thistime = (LPSYSTEMTIME)xtime;
  };

  if (format == NULL) {
     GetLocaleInfoA(thislocale, ((flags&DATE_LONGDATE) 
				   ? LOCALE_SLONGDATE
				   : LOCALE_SSHORTDATE),
		      format_buf, sizeof(format_buf));
     thisformat = format_buf;
  } else {
     thisformat = format;
  };

  
  ret = OLE_GetFormatA(thislocale, flags, 0, thistime, thisformat, date, datelen);
  

  
  return ret;
}


int
STDCALL
GetTimeFormatW(
    LCID     Locale,
    DWORD    dwFlags,
    CONST SYSTEMTIME *lpTime,
    LPCWSTR lpFormat,
    LPWSTR  lpTimeStr,
    int      cchTime)
{
	return 0;
}


/******************************************************************************
 *		GetTimeFormat32A	[KERNEL32.422]
 * Makes an ASCII string of the time
 *
 * Formats date according to format,  or locale default if format is
 * NULL. The format consists of literal characters and fields as follows:
 *
 * h  hours with no leading zero (12-hour)
 * hh hours with full two digits
 * H  hours with no leading zero (24-hour)
 * HH hours with full two digits
 * m  minutes with no leading zero
 * mm minutes with full two digits
 * s  seconds with no leading zero
 * ss seconds with full two digits
 * t  time marker (A or P)
 * tt time marker (AM, PM)
 *
 */
int
STDCALL
GetTimeFormatA(
    LCID     Locale,
    DWORD    dwFlags,
    CONST SYSTEMTIME *lpTime,
    LPCSTR lpFormat,
    LPSTR  lpTimeStr,
    int      cchTime)
{ 
  char format_buf[40];
  LPCSTR thisformat;
  SYSTEMTIME t;
  LPSYSTEMTIME thistime;
  LCID thislocale=0;
  DWORD thisflags=LOCALE_STIMEFORMAT; /* standart timeformat */
  INT ret;

  thislocale = OLE2NLS_CheckLocale ( Locale );

  if ( dwFlags & (TIME_NOTIMEMARKER | TIME_FORCE24HOURFORMAT ))
  { 
	//FIXME(ole,"TIME_NOTIMEMARKER or TIME_FORCE24HOURFORMAT not implemented\n");
  }
  
  dwFlags &= (TIME_NOSECONDS | TIME_NOMINUTESORSECONDS); /* mask for OLE_GetFormatA*/

  if (lpFormat == NULL) 
  { if (dwFlags & LOCALE_NOUSEROVERRIDE)  /*use system default*/
    { thislocale = GetSystemDefaultLCID();
    }
    GetLocaleInfoA(thislocale, thisflags, format_buf, sizeof(format_buf));
    thisformat = format_buf;
  }
  else 
  { thisformat = lpFormat;
  }
  
  if (lpTime == NULL) /* NULL means use the current local time*/
  { GetSystemTime(&t);
    thistime = &t;
  } 
  else
  { thistime = lpTime;
  }
  ret = OLE_GetFormatA(thislocale, thisflags, dwFlags, thistime, thisformat,
  			 lpTimeStr, cchTime);
  return ret;
}

LCID OLE2NLS_CheckLocale (LCID locale)
{
	if (!locale) 
	{ locale = LOCALE_SYSTEM_DEFAULT;
	}
  
	if (locale == LOCALE_SYSTEM_DEFAULT) 
  	{ return GetSystemDefaultLCID();
	} 
	else if (locale == LOCALE_USER_DEFAULT) 
	{ return GetUserDefaultLCID();
	}
	else
	{ return locale;
	}
}

/******************************************************************************
 *		OLE_GetFormatA	[Internal]
 *
 * FIXME
 *    If datelen == 0, it should return the reguired string length.
 *
 This function implements stuff for GetDateFormat() and 
 GetTimeFormat().

  d    single-digit (no leading zero) day (of month)
  dd   two-digit day (of month)
  ddd  short day-of-week name
  dddd long day-of-week name
  M    single-digit month
  MM   two-digit month
  MMM  short month name
  MMMM full month name
  y    two-digit year, no leading 0
  yy   two-digit year
  yyyy four-digit year
  gg   era string
  h    hours with no leading zero (12-hour)
  hh   hours with full two digits
  H    hours with no leading zero (24-hour)
  HH   hours with full two digits
  m    minutes with no leading zero
  mm   minutes with full two digits
  s    seconds with no leading zero
  ss   seconds with full two digits
  t    time marker (A or P)
  tt   time marker (AM, PM)
  ''   used to quote literal characters
  ''   (within a quoted string) indicates a literal '

 These functions REQUIRE valid locale, date,  and format. 
 */
int OLE_GetFormatA(LCID locale,
			    DWORD flags,
			    DWORD tflags,
			    LPSYSTEMTIME xtime,
			    LPCSTR _format, 	/*in*/
			    LPSTR date,		/*out*/
			    int datelen)
{
   INT inpos, outpos;
   int count, type, inquote, Overflow;
   char buf[40];
   char format[40];
   char * pos;
   int buflen;

   const char * _dgfmt[] = { "%d", "%02d" };
   const char ** dgfmt = _dgfmt - 1; 

 
   if(datelen == 0) {
     return 255;
   }

   /* initalize state variables and output buffer */
   inpos = outpos = 0;
   count = 0; inquote = 0; Overflow = 0;
   type = '\0';
   date[0] = buf[0] = '\0';
      
   strcpy(format,_format);

   /* alter the formatstring, while it works for all languages now in wine
   its possible that it fails when the time looks like ss:mm:hh as example*/   
   if (tflags & (TIME_NOMINUTESORSECONDS))
   { if ((pos = strstr ( format, ":mm")))
     { memcpy ( pos, pos+3, strlen(format)-(pos-format)-2 );
     }
   }
   if (tflags & (TIME_NOSECONDS))
   { if ((pos = strstr ( format, ":ss")))
     { memcpy ( pos, pos+3, strlen(format)-(pos-format)-2 );
     }
   }
   
   for (inpos = 0;; inpos++) {
      /* TRACE(ole, "STATE inpos=%2d outpos=%2d count=%d inquote=%d type=%c buf,date = %c,%c\n", inpos, outpos, count, inquote, type, buf[inpos], date[outpos]); */
      if (inquote) {
	 if (format[inpos] == '\'') {
	    if (format[inpos+1] == '\'') {
	       inpos += 1;
	       date[outpos++] = '\'';
	    } else {
	       inquote = 0;
	       continue; /* we did nothing to the output */
	    }
	 } else if (format[inpos] == '\0') {
	    date[outpos++] = '\0';
	    if (outpos > datelen) Overflow = 1;
	    break;
	 } else {
	    date[outpos++] = format[inpos];
	    if (outpos > datelen) {
	       Overflow = 1;
	       date[outpos-1] = '\0'; /* this is the last place where
					 it's safe to write */
	       break;
	    }
	 }
      } else if (  (count && (format[inpos] != type))
		   || count == 4
		   || (count == 2 && strchr("ghHmst", type)) )
       {
	    if         (type == 'd') {
	       if        (count == 4) {
		  GetLocaleInfoA(locale,
				   LOCALE_SDAYNAME1
				   + xtime->wDayOfWeek - 1,
				   buf, sizeof(buf));
	       } else if (count == 3) {
			   GetLocaleInfoA(locale, 
					    LOCALE_SABBREVDAYNAME1 
					    + xtime->wDayOfWeek - 1,
					    buf, sizeof(buf));
		      } else {
		  wsprintfA(buf, dgfmt[count], xtime->wDay);
	       }
	    } else if (type == 'M') {
	       if (count == 3) {
		  GetLocaleInfoA(locale, 
				   LOCALE_SABBREVMONTHNAME1
				   + xtime->wMonth - 1,
				   buf, sizeof(buf));
	       } else if (count == 4) {
		  GetLocaleInfoA(locale,
				   LOCALE_SMONTHNAME1
				   + xtime->wMonth - 1,
				   buf, sizeof(buf));
		 } else {
		  wsprintfA(buf, dgfmt[count], xtime->wMonth);
	       }
	    } else if (type == 'y') {
	       if (count == 4) {
		      wsprintfA(buf, "%d", xtime->wYear);
	       } else if (count == 3) {
		  lstrcpyA(buf, "yyy");
//		  WARN(ole, "unknown format, c=%c, n=%d\n",  type, count);
		 } else {
		  wsprintfA(buf, dgfmt[count], xtime->wYear % 100);
	       }
	    } else if (type == 'g') {
	       if        (count == 2) {
//		  FIXME(ole, "LOCALE_ICALENDARTYPE unimp.\n");
		  lstrcpyA(buf, "AD");
	    } else {
		  lstrcpyA(buf, "g");
//		  WARN(ole, "unknown format, c=%c, n=%d\n", type, count);
	       }
	    } else if (type == 'h') {
	       /* gives us hours 1:00 -- 12:00 */
	       wsprintfA(buf, dgfmt[count], (xtime->wHour-1)%12 +1);
	    } else if (type == 'H') {
	       /* 24-hour time */
	       wsprintfA(buf, dgfmt[count], xtime->wHour);
	    } else if ( type == 'm') {
	       wsprintfA(buf, dgfmt[count], xtime->wMinute);
	    } else if ( type == 's') {
	       wsprintfA(buf, dgfmt[count], xtime->wSecond);
	    } else if (type == 't') {
	       if (count == 1) {
		  wsprintfA(buf, "%c", (xtime->wHour < 12) ? 'A' : 'P');
	       } else if (count == 2) {
		  /* sprintf(buf, "%s", (xtime->wHour < 12) ? "AM" : "PM"); */
		  GetLocaleInfoA(locale,
				   (xtime->wHour<12) 
				   ? LOCALE_S1159 : LOCALE_S2359,
				   buf, sizeof(buf));
	       }
	    };

	    /* we need to check the next char in the format string 
	       again, no matter what happened */
	    inpos--;
	    
	    /* add the contents of buf to the output */
	    buflen = lstrlenA(buf);
	    if (outpos + buflen < datelen) {
	       date[outpos] = '\0'; /* for strcat to hook onto */
	       lstrcatA(date, buf);
	       outpos += buflen;
	    } else {
	         date[outpos] = '\0';
	         strncat(date, buf, datelen - outpos);
		 date[datelen - 1] = '\0';
		 SetLastError(ERROR_INSUFFICIENT_BUFFER);
		 return 0;
	    }

	    /* reset the variables we used to keep track of this item */
	    count = 0;
	    type = '\0';
	 } else if (format[inpos] == '\0') {
	    /* we can't check for this at the loop-head, because
	       that breaks the printing of the last format-item */
	    date[outpos] = '\0';
	    break;
         } else if (count) {
	    /* continuing a code for an item */
	    count +=1;
	    continue;
	 } else if (strchr("hHmstyMdg", format[inpos])) {
	    type = format[inpos];
	    count = 1;
	    continue;
	 } else if (format[inpos] == '\'') {
	    inquote = 1;
	    continue;
       } else {
	    date[outpos++] = format[inpos];
	 }
      /* now deal with a possible buffer overflow */
      if (outpos >= datelen) {
       date[datelen - 1] = '\0';
       SetLastError(ERROR_INSUFFICIENT_BUFFER);
       return 0;
      }
   }
   
   if (Overflow) {
      SetLastError(ERROR_INSUFFICIENT_BUFFER);
   };

   /* finish it off with a string terminator */
   outpos++;
   /* sanity check */
   if (outpos > datelen-1) outpos = datelen-1;
   date[outpos] = '\0';
   

   return outpos;
}



