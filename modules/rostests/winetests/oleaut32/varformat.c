/*
 * VARFORMAT test program
 *
 * Copyright 1998 Jean-Claude Cote
 * Copyright 2006 Google (Benjamin Arai)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

#include "windef.h"
#include "winbase.h"
#include "winsock2.h"
#include "wine/test.h"
#include "winuser.h"
#include "wingdi.h"
#include "winnls.h"
#include "winerror.h"
#include "winnt.h"

#include "wtypes.h"
#include "oleauto.h"

#define FMT_NUMBER(vt,val) \
  VariantInit(&v); V_VT(&v) = vt; val(&v) = 1; \
  hres = VarFormatNumber(&v,2,0,0,0,0,&str); \
  ok(hres == S_OK, "VarFormatNumber (vt %d): returned %8lx\n", vt, hres); \
  if (hres == S_OK) { \
    ok(str && wcscmp(str,szResult1) == 0, \
       "VarFormatNumber (vt %d): string different\n", vt); \
    SysFreeString(str); \
  }

static void test_VarFormatNumber(void)
{
  static const WCHAR szResult1[] = L"1.00";
  char buff[8];
  HRESULT hres;
  VARIANT v;
  BSTR str = NULL;

  GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, buff, ARRAY_SIZE(buff));
  if (buff[0] != '.' || buff[1])
  {
    skip("Skipping VarFormatNumber tests as decimal separator is '%s'\n", buff);
    return;
  }

  FMT_NUMBER(VT_I1, V_I1);
  FMT_NUMBER(VT_UI1, V_UI1);
  FMT_NUMBER(VT_I2, V_I2);
  FMT_NUMBER(VT_UI2, V_UI2);
  FMT_NUMBER(VT_I4, V_I4);
  FMT_NUMBER(VT_UI4, V_UI4);
  FMT_NUMBER(VT_I8, V_I8);
  FMT_NUMBER(VT_UI8, V_UI8);
  FMT_NUMBER(VT_R4, V_R4);
  FMT_NUMBER(VT_R8, V_R8);
  FMT_NUMBER(VT_BOOL, V_BOOL);

  V_VT(&v) = VT_BSTR;
  V_BSTR(&v) = SysAllocString(L"1");

  hres = VarFormatNumber(&v,2,0,0,0,0,&str);
  ok(hres == S_OK, "VarFormatNumber (bstr): returned %8lx\n", hres);
  if (hres == S_OK)
    ok(str && wcscmp(str, szResult1) == 0, "VarFormatNumber (bstr): string different\n");
  SysFreeString(V_BSTR(&v));
  SysFreeString(str);

  V_BSTR(&v) = SysAllocString(L"-1");
  hres = VarFormatNumber(&v,2,0,-1,0,0,&str);
  ok(hres == S_OK, "VarFormatNumber (bstr): returned %8lx\n", hres);
  if (hres == S_OK)
    ok(str && wcscmp(str, L"(1.00)") == 0, "VarFormatNumber (-bstr): string different\n");
  SysFreeString(V_BSTR(&v));
  SysFreeString(str);
}

#define SIGNED_VTBITS (VTBIT_I1|VTBIT_I2|VTBIT_I4|VTBIT_I8|VTBIT_R4|VTBIT_R8)

static const char *szVarFmtFail = "VT %d|0x%04x Format %s: expected 0x%08x, '%s', got 0x%08x, '%s'\n";
#define VARFMT(vt,v,val,fmt,ret,str) do { \
  out = NULL; \
  V_VT(&in) = (vt); v(&in) = val; \
  if (fmt) MultiByteToWideChar(CP_ACP, 0, fmt, -1, buffW, ARRAY_SIZE(buffW)); \
  hres = VarFormat(&in,fmt ? buffW : NULL,fd,fw,flags,&out); \
  if (SUCCEEDED(hres)) WideCharToMultiByte(CP_ACP, 0, out, -1, buff, sizeof(buff),0,0); \
  else buff[0] = '\0'; \
  ok(hres == ret && (FAILED(ret) || !strcmp(buff, str)), \
     szVarFmtFail, \
     (vt)&VT_TYPEMASK,(vt)&~VT_TYPEMASK,fmt?fmt:"<null>",ret,str,hres,buff); \
  SysFreeString(out); \
  } while(0)

typedef struct tagFMTRES
{
  LPCSTR fmt;
  LPCSTR one_res;
  LPCSTR zero_res;
} FMTRES;

static const FMTRES VarFormat_results[] =
{
  { NULL, "1", "0" },
  { "", "1", "0" },
  { "General Number", "1", "0" },
  { "Percent", "100.00%", "0.00%" },
  { "Standard", "1.00", "0.00" },
  { "Scientific","1.00E+00", "0.00E+00" },
  { "True/False", "True", "False" },
  { "On/Off", "On", "Off" },
  { "Yes/No", "Yes", "No" },
  { "#", "1", "" },
  { "##", "1", "" },
  { "#.#", "1.", "." },
  { "0", "1", "0" },
  { "00", "01", "00" },
  { "0.0", "1.0", "0.0" },
  { "00\\c\\o\\p\\y", "01copy","00copy" },
  { "\"pos\";\"neg\"", "pos", "pos" },
  { "\"pos\";\"neg\";\"zero\"","pos", "zero" }
};

typedef struct tagFMTDATERES
{
  DATE   val;
  LPCSTR fmt;
  LPCSTR res;
} FMTDATERES;

static const FMTDATERES VarFormat_date_results[] =
{
  { 0.0, "w", "7" },
  { 0.0, "w", "6" },
  { 0.0, "w", "5" },
  { 0.0, "w", "4" },
  { 0.0, "w", "3" },
  { 0.0, "w", "2" },
  { 0.0, "w", "1" }, /* First 7 entries must remain in this order! */
  { 2.525, "am/pm", "pm" },
  { 2.525, "AM/PM", "PM" },
  { 2.525, "A/P", "P" },
  { 2.525, "a/p", "p" },
  { 2.525, "q", "1" },
  { 2.525, "d", "1" },
  { 2.525, "dd", "01" },
  { 2.525, "ddd", "Mon" },
  { 2.525, "dddd", "Monday" },
  { 2.525, "mmm", "Jan" },
  { 2.525, "mmmm", "January" },
  { 2.525, "y", "1" },
  { 2.525, "yy", "00" },
  { 2.525, "yyy", "001" },
  { 2.525, "yyyy", "1900" },
  { 2.525, "dd mm yyyy hh:mm:ss", "01 01 1900 12:36:00" },
  { 2.525, "dd mm yyyy mm", "01 01 1900 01" },
  { 2.525, "dd mm yyyy :mm", "01 01 1900 :01" },
  { 2.525, "dd mm yyyy hh:mm", "01 01 1900 12:36" },
  { 2.525, "mm mm", "01 01" },
  { 2.525, "mm :mm:ss", "01 :01:00" },
  { 2.525, "mm :ss:mm", "01 :00:01" },
  { 2.525, "hh:mm :ss:mm", "12:36 :00:01" },
  { 2.525, "hh:dd :mm:mm", "12:01 :01:01" },
  { 2.525, "dd:hh :mm:mm", "01:12 :36:01" },
  { 2.525, "hh :mm:mm", "12 :36:01" },
  { 2.525, "dd :mm:mm", "01 :01:01" },
  { 2.525, "dd :mm:nn", "01 :01:36" },
  { 2.725, "hh:nn:ss A/P", "05:24:00 P" },
  { 40531.0, "dddd", "Sunday" },
  { 40531.0, "ddd", "Sun" }
};

/* The following tests require that the time separator is a colon (:) */
static const FMTDATERES VarFormat_namedtime_results[] =
{
  { 2.525, "short time", "12:36" },
  { 2.525, "medium time", "12:36 PM" },
  { 2.525, "long time", "12:36:00 PM" }
};

#define VNUMFMT(vt,v) \
  for (i = 0; i < ARRAY_SIZE(VarFormat_results); i++) \
  { \
    VARFMT(vt,v,1,VarFormat_results[i].fmt,S_OK,VarFormat_results[i].one_res); \
    VARFMT(vt,v,0,VarFormat_results[i].fmt,S_OK,VarFormat_results[i].zero_res); \
  } \
  if ((1 << vt) & SIGNED_VTBITS) \
  { \
    VARFMT(vt,v,-1,"\"pos\";\"neg\"",S_OK,"neg"); \
    VARFMT(vt,v,-1,"\"pos\";\"neg\";\"zero\"",S_OK,"neg"); \
  }

static void test_VarFormat(void)
{
  size_t i;
  WCHAR buffW[256];
  char buff[256];
  VARIANT in;
  VARIANT_BOOL bTrue = VARIANT_TRUE, bFalse = VARIANT_FALSE;
  int fd = 0, fw = 0;
  ULONG flags = 0;
  BSTR bstrin, out = NULL;
  HRESULT hres;

  if (PRIMARYLANGID(LANGIDFROMLCID(GetUserDefaultLCID())) != LANG_ENGLISH)
  {
    skip("Skipping VarFormat tests for non English language\n");
    return;
  }
  GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, buff, ARRAY_SIZE(buff));
  if (buff[0] != '.' || buff[1])
  {
    skip("Skipping VarFormat tests as decimal separator is '%s'\n", buff);
    return;
  }
  GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_IDIGITS, buff, ARRAY_SIZE(buff));
  if (buff[0] != '2' || buff[1])
  {
    skip("Skipping VarFormat tests as decimal places is '%s'\n", buff);
    return;
  }

  VARFMT(VT_BOOL,V_BOOL,VARIANT_TRUE,"True/False",S_OK,"True");
  VARFMT(VT_BOOL,V_BOOL,VARIANT_FALSE,"True/False",S_OK,"False");

  VNUMFMT(VT_I1,V_I1);
  VNUMFMT(VT_I2,V_I2);
  VNUMFMT(VT_I4,V_I4);
  VNUMFMT(VT_I8,V_I8);
  VNUMFMT(VT_INT,V_INT);
  VNUMFMT(VT_UI1,V_UI1);
  VNUMFMT(VT_UI2,V_UI2);
  VNUMFMT(VT_UI4,V_UI4);
  VNUMFMT(VT_UI8,V_UI8);
  VNUMFMT(VT_UINT,V_UINT);
  VNUMFMT(VT_R4,V_R4);
  VNUMFMT(VT_R8,V_R8);

  /* Reference types are dereferenced */
  VARFMT(VT_BOOL|VT_BYREF,V_BOOLREF,&bTrue,"True/False",S_OK,"True");
  VARFMT(VT_BOOL|VT_BYREF,V_BOOLREF,&bFalse,"True/False",S_OK,"False");

  /* Dates */
  for (i = 0; i < ARRAY_SIZE(VarFormat_date_results); i++)
  {
    if (i < 7)
      fd = i + 1; /* Test first day */
    else
      fd = 0;
    VARFMT(VT_DATE,V_DATE,VarFormat_date_results[i].val,
           VarFormat_date_results[i].fmt,S_OK,
           VarFormat_date_results[i].res);
  }

  /* Named time formats */
  GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_STIMEFORMAT, buff, ARRAY_SIZE(buff));
  if (strcmp(buff, "h:mm:ss tt"))
  {
    skip("Skipping named time tests as time format is '%s'\n", buff);
  }
  else
  {
    for (i = 0; i < ARRAY_SIZE(VarFormat_namedtime_results); i++)
    {
      fd = 0;
      VARFMT(VT_DATE,V_DATE,VarFormat_namedtime_results[i].val,
             VarFormat_namedtime_results[i].fmt,S_OK,
             VarFormat_namedtime_results[i].res);
    }
  }

  /* Strings */
  bstrin = SysAllocString(L"testing");
  VARFMT(VT_BSTR,V_BSTR,bstrin,"",S_OK,"testing");
  VARFMT(VT_BSTR,V_BSTR,bstrin,"@",S_OK,"testing");
  VARFMT(VT_BSTR,V_BSTR,bstrin,"&",S_OK,"testing");
  VARFMT(VT_BSTR,V_BSTR,bstrin,"\\x@\\x@",S_OK,"xtxesting");
  VARFMT(VT_BSTR,V_BSTR,bstrin,"\\x&\\x&",S_OK,"xtxesting");
  VARFMT(VT_BSTR,V_BSTR,bstrin,"@\\x",S_OK,"txesting");
  VARFMT(VT_BSTR,V_BSTR,bstrin,"@@@@@@@@",S_OK," testing");
  VARFMT(VT_BSTR,V_BSTR,bstrin,"@\\x@@@@@@@",S_OK," xtesting");
  VARFMT(VT_BSTR,V_BSTR,bstrin,"&&&&&&&&",S_OK,"testing");
  VARFMT(VT_BSTR,V_BSTR,bstrin,"!&&&&&&&",S_OK,"testing");
  VARFMT(VT_BSTR,V_BSTR,bstrin,"&&&&&&&!",S_OK,"testing");
  VARFMT(VT_BSTR,V_BSTR,bstrin,">&&",S_OK,"TESTING");
  VARFMT(VT_BSTR,V_BSTR,bstrin,"<&&",S_OK,"testing");
  VARFMT(VT_BSTR,V_BSTR,bstrin,"<&>&",S_OK,"testing");
  SysFreeString(bstrin);
  bstrin = SysAllocString(L"39697.11");
  VARFMT(VT_BSTR,V_BSTR,bstrin,"hh:mm",S_OK,"02:38");
  VARFMT(VT_BSTR,V_BSTR,bstrin,"mm-dd-yy",S_OK,"09-06-08");
  SysFreeString(bstrin);
  /* Numeric values are converted to strings then output */
  VARFMT(VT_I1,V_I1,1,"<&>&",S_OK,"1");

  /* Number formats */
  VARFMT(VT_I4,V_I4,1,"#00000000",S_OK,"00000001");
  VARFMT(VT_I4,V_I4,1,"000###",S_OK,"000001");
  VARFMT(VT_I4,V_I4,1,"#00##00#0",S_OK,"00000001");
  VARFMT(VT_I4,V_I4,1,"1#####0000",S_OK,"10001");
  VARFMT(VT_I4,V_I4,1,"##abcdefghijklmnopqrstuvwxyz",S_OK,"1abcdefghijklmnopqrstuvwxyz");
  VARFMT(VT_I4,V_I4,100000,"#,###,###,###",S_OK,"100,000");
  VARFMT(VT_I4,V_I4,1,"0,000,000,000",S_OK,"0,000,000,001");
  VARFMT(VT_I4,V_I4,123456789,"#,#.#",S_OK,"123,456,789.");
  VARFMT(VT_I4,V_I4,123456789,"###, ###, ###",S_OK,"123, 456, 789");
  VARFMT(VT_I4,V_I4,1,"#;-#",S_OK,"1");
  VARFMT(VT_I4,V_I4,-1,"#;-#",S_OK,"-1");
  VARFMT(VT_R8,V_R8,1.23456789,"0#.0#0#0#0#0",S_OK,"01.234567890");
  VARFMT(VT_R8,V_R8,1.2,"0#.0#0#0#0#0",S_OK,"01.200000000");
  VARFMT(VT_R8,V_R8,9.87654321,"#0.#0#0#0#0#",S_OK,"9.87654321");
  VARFMT(VT_R8,V_R8,9.8,"#0.#0#0#0#0#",S_OK,"9.80000000");
  VARFMT(VT_R8,V_R8,0.00000008,"#0.#0#0#0#0#0",S_OK,"0.0000000800");
  VARFMT(VT_R8,V_R8,0.00010705,"#0.##########",S_OK,"0.00010705");
  VARFMT(VT_I4,V_I4,17,"#0",S_OK,"17");
  VARFMT(VT_I4,V_I4,4711,"#0",S_OK,"4711");
  VARFMT(VT_I4,V_I4,17,"#00",S_OK,"17");
  VARFMT(VT_I4,V_I4,100,"0##",S_OK,"100");
  VARFMT(VT_I4,V_I4,17,"#000",S_OK,"017");
  VARFMT(VT_I4,V_I4,17,"#0.00",S_OK,"17.00");
  VARFMT(VT_I4,V_I4,17,"#0000.00",S_OK,"0017.00");
  VARFMT(VT_I4,V_I4,17,"#.00",S_OK,"17.00");
  VARFMT(VT_R8,V_R8,1.7,"#.00",S_OK,"1.70");
  VARFMT(VT_R8,V_R8,.17,"#.00",S_OK,".17");
  VARFMT(VT_I4,V_I4,17,"#3",S_OK,"173");
  VARFMT(VT_I4,V_I4,17,"#33",S_OK,"1733");
  VARFMT(VT_I4,V_I4,17,"#3.33",S_OK,"173.33");
  VARFMT(VT_I4,V_I4,17,"#3333.33",S_OK,"173333.33");
  VARFMT(VT_I4,V_I4,17,"#.33",S_OK,"17.33");
  VARFMT(VT_R8,V_R8,.17,"#.33",S_OK,".33");
  VARFMT(VT_R8,V_R8,1.7,"0.0000E-000",S_OK,"1.7000E000");
  VARFMT(VT_R8,V_R8,1.7,"0.0000e-1",S_OK,"1.7000e01");
  VARFMT(VT_R8,V_R8,86.936849,"#0.000000000000e-000",S_OK,"86.936849000000e000");
  VARFMT(VT_R8,V_R8,1.7,"#0",S_OK,"2");
  VARFMT(VT_R8,V_R8,1.7,"#.33",S_OK,"2.33");
  VARFMT(VT_R8,V_R8,1.7,"#3",S_OK,"23");
  VARFMT(VT_R8,V_R8,1.73245,"0.0000E+000",S_OK,"1.7325E+000");
  VARFMT(VT_R8,V_R8,9.9999999,"#0.000000",S_OK,"10.000000");
  VARFMT(VT_R8,V_R8,1.7,"0.0000e+0#",S_OK,"1.7000e+0");
  VARFMT(VT_R8,V_R8,100.0001e+0,"0.0000E+0",S_OK,"1.0000E+2");
  VARFMT(VT_R8,V_R8,1000001,"0.0000e+1",S_OK,"1.0000e+61");
  VARFMT(VT_R8,V_R8,100.0001e+25,"0.0000e+0",S_OK,"1.0000e+27");
  VARFMT(VT_R8,V_R8,450.0001e+43,"#000.0000e+0",S_OK,"4500.0010e+42");
  VARFMT(VT_R8,V_R8,0.0001e-11,"##00.0000e-0",S_OK,"1000.0000e-18");
  VARFMT(VT_R8,V_R8,0.0317e-11,"0000.0000e-0",S_OK,"3170.0000e-16");
  VARFMT(VT_R8,V_R8,0.0021e-11,"00##.0000e-0",S_OK,"2100.0000e-17");
  VARFMT(VT_R8,V_R8,1.0001e-27,"##00.0000e-0",S_OK,"1000.1000e-30");
  VARFMT(VT_R8,V_R8,47.11,".0000E+0",S_OK,".4711E+2");
  VARFMT(VT_R8,V_R8,3.0401e-13,"#####.####e-0%",S_OK,"30401.e-15%");
  VARFMT(VT_R8,V_R8,1.57,"0.00",S_OK,"1.57");
  VARFMT(VT_R8,V_R8,-1.57,"0.00",S_OK,"-1.57");
  VARFMT(VT_R8,V_R8,-1.57,"#.##",S_OK,"-1.57");
  VARFMT(VT_R8,V_R8,-0.1,".#",S_OK,"-.1");
  VARFMT(VT_R8,V_R8,0.099,"#.#",S_OK,".1");
  VARFMT(VT_R8,V_R8,0.0999,"#.##",S_OK,".1");
  VARFMT(VT_R8,V_R8,0.099,"#.##",S_OK,".1");
  VARFMT(VT_R8,V_R8,0.0099,"#.##",S_OK,".01");
  VARFMT(VT_R8,V_R8,0.0049,"#.##",S_OK,".");
  VARFMT(VT_R8,V_R8,0.0094,"#.##",S_OK,".01");
  VARFMT(VT_R8,V_R8,0.00099,"#.##",S_OK,".");
  VARFMT(VT_R8,V_R8,0.0995,"#.##",S_OK,".1");
  VARFMT(VT_R8,V_R8,8.0995,"#.##",S_OK,"8.1");
  VARFMT(VT_R8,V_R8,0.0994,"#.##",S_OK,".1");
  VARFMT(VT_R8,V_R8,1.00,"#,##0.00",S_OK,"1.00");
  VARFMT(VT_R8,V_R8,0.0995,"#.###",S_OK,".1");


  /* 'out' is not cleared */
  out = (BSTR)0x1;
  hres = VarFormat(&in,NULL,fd,fw,flags,&out); /* Would crash if out is cleared */
  ok(hres == S_OK, "got %08lx\n", hres);
  SysFreeString(out);
  out = NULL;

  /* VT_NULL */
  V_VT(&in) = VT_NULL;
  hres = VarFormat(&in,NULL,fd,fw,0,&out);
  ok(hres == S_OK, "VarFormat failed with 0x%08lx\n", hres);
  ok(out == NULL, "expected NULL formatted string\n");

  /* Invalid args */
  hres = VarFormat(&in,NULL,fd,fw,flags,NULL);
  ok(hres == E_INVALIDARG, "Null out: expected E_INVALIDARG, got 0x%08lx\n", hres);
  hres = VarFormat(NULL,NULL,fd,fw,flags,&out);
  ok(hres == E_INVALIDARG, "Null in: expected E_INVALIDARG, got 0x%08lx\n", hres);
  fd = -1;
  VARFMT(VT_BOOL,V_BOOL,VARIANT_TRUE,"",E_INVALIDARG,"");
  fd = 8;
  VARFMT(VT_BOOL,V_BOOL,VARIANT_TRUE,"",E_INVALIDARG,"");
  fd = 0; fw = -1;
  VARFMT(VT_BOOL,V_BOOL,VARIANT_TRUE,"",E_INVALIDARG,"");
  fw = 4;
  VARFMT(VT_BOOL,V_BOOL,VARIANT_TRUE,"",E_INVALIDARG,"");
}

static const char *szVarWdnFail =
    "VarWeekdayName (%d, %d, %d, %d, %x): returned %8x, expected %8x\n";
#define VARWDN(iWeekday, fAbbrev, iFirstDay, dwFlags, ret, buff, out, freeOut) \
do { \
  hres = VarWeekdayName(iWeekday, fAbbrev, iFirstDay, dwFlags, &out); \
  if (SUCCEEDED(hres)) { \
    WideCharToMultiByte(CP_ACP, 0, out, -1, buff, sizeof(buff), 0, 0); \
    if (freeOut) SysFreeString(out); \
  } else { \
    buff[0] = '\0'; \
  } \
  ok(hres == ret, \
     szVarWdnFail, \
     iWeekday, fAbbrev, iFirstDay, dwFlags, &out, hres, ret \
     ); \
} while(0)

#define VARWDN_F(iWeekday, fAbbrev, iFirstDay, dwFlags, ret) \
  VARWDN(iWeekday, fAbbrev, iFirstDay, dwFlags, ret, buff, out, 1)

#define VARWDN_O(iWeekday, fAbbrev, iFirstDay, dwFlags) \
  VARWDN(iWeekday, fAbbrev, iFirstDay, dwFlags, S_OK, buff, out, 0)

static void test_VarWeekdayName(void)
{
  char buff[256];
  BSTR out = NULL;
  HRESULT hres;
  int iWeekday, fAbbrev, iFirstDay;
  BSTR dayNames[7][2]; /* Monday-Sunday, full/abbr */
  DWORD defaultFirstDay;
  int firstDay;
  int day;
  int size;
  DWORD localeValue;

  SetLastError(0xdeadbeef);
  GetLocaleInfoW(LOCALE_USER_DEFAULT, 0, NULL, 0);
  if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
  {
    win_skip("GetLocaleInfoW is not implemented\n");
    return;
  }

  /* Initialize days' names */
  for (day = 0; day <= 6; ++day)
  {
    for (fAbbrev = 0; fAbbrev <= 1; ++fAbbrev)
    {
      localeValue = fAbbrev ? LOCALE_SABBREVDAYNAME1 : LOCALE_SDAYNAME1;
      localeValue += day;
      size = GetLocaleInfoW(LOCALE_USER_DEFAULT, localeValue, NULL, 0);
      dayNames[day][fAbbrev] = SysAllocStringLen(NULL, size - 1);
      GetLocaleInfoW(LOCALE_USER_DEFAULT, localeValue,
                     dayNames[day][fAbbrev], size);
    }
  }

  /* Get the user's first day of week. 0=Monday, .. */
  GetLocaleInfoW(
      LOCALE_USER_DEFAULT, LOCALE_IFIRSTDAYOFWEEK | LOCALE_RETURN_NUMBER,
      (LPWSTR)&defaultFirstDay, sizeof(defaultFirstDay) / sizeof(WCHAR));

  /* Check invalid arguments */
  VARWDN_F(0, 0, 4, 0, E_INVALIDARG);
  VARWDN_F(8, 0, 4, 0, E_INVALIDARG);
  VARWDN_F(4, 0, -1, 0, E_INVALIDARG);
  VARWDN_F(4, 0, 8, 0, E_INVALIDARG);

  hres = VarWeekdayName(1, 0, 0, 0, NULL);
  ok(E_INVALIDARG == hres,
     "Null pointer: expected E_INVALIDARG, got 0x%08lx\n", hres);

  /* Check all combinations */
  for (iWeekday = 1; iWeekday <= 7; ++iWeekday)
  {
    for (fAbbrev = 0; fAbbrev <= 1; ++fAbbrev)
    {
      /* 0 = Default, 1 = Sunday, 2 = Monday, .. */
      for (iFirstDay = 0; iFirstDay <= 7; ++iFirstDay)
      {
        VARWDN_O(iWeekday, fAbbrev, iFirstDay, 0);
        if (iFirstDay == 0)
          firstDay = defaultFirstDay;
        else
          /* Translate from 0=Sunday to 0=Monday in the modulo 7 space */
          firstDay = iFirstDay - 2;
        day = (7 + iWeekday - 1 + firstDay) % 7;
        ok(VARCMP_EQ == VarBstrCmp(out, dayNames[day][fAbbrev], LOCALE_USER_DEFAULT, 0),
           "VarWeekdayName(%d,%d,%d): got wrong dayname: '%s'\n",
           iWeekday, fAbbrev, iFirstDay, buff);
        SysFreeString(out);
      }
    }
  }

  /* Cleanup */
  for (day = 0; day <= 6; ++day)
  {
    for (fAbbrev = 0; fAbbrev <= 1; ++fAbbrev)
    {
      SysFreeString(dayNames[day][fAbbrev]);
    }
  }
}

static void test_VarFormatFromTokens(void)
{
    static WCHAR number_fmt[] = L"###,##0.00";
    static WCHAR date_fmt[] = L"dd-mm";
    static WCHAR string_fmt[] = L"@";

    BYTE buff[256];
    LCID lcid;
    VARIANT var;
    BSTR bstr;
    HRESULT hres;

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(L"6,90");

    lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
    hres = VarTokenizeFormatString(number_fmt, buff, sizeof(buff), 1, 1, lcid, NULL);
    ok(hres == S_OK, "VarTokenizeFormatString failed: %lx\n", hres);
    hres = VarFormatFromTokens(&var, number_fmt, buff, 0, &bstr, lcid);
    ok(hres == S_OK, "VarFormatFromTokens failed: %lx\n", hres);
    ok(!wcscmp(bstr, L"690.00"), "incorrectly formatted number: %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    lcid = MAKELCID(MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN), SORT_DEFAULT);
    hres = VarTokenizeFormatString(number_fmt, buff, sizeof(buff), 1, 1, lcid, NULL);
    ok(hres == S_OK, "VarTokenizeFormatString failed: %lx\n", hres);
    hres = VarFormatFromTokens(&var, number_fmt, buff, 0, &bstr, lcid);
    ok(hres == S_OK, "VarFormatFromTokens failed: %lx\n", hres);
    ok(!wcscmp(bstr, L"6,90"), "incorrectly formatted number: %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    VariantClear(&var);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(L"12-11");

    lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
    hres = VarTokenizeFormatString(date_fmt, buff, sizeof(buff), 1, 1, lcid, NULL);
    ok(hres == S_OK, "VarTokenizeFormatString failed: %lx\n", hres);
    hres = VarFormatFromTokens(&var, date_fmt, buff, 0, &bstr, lcid);
    ok(hres == S_OK, "VarFormatFromTokens failed: %lx\n", hres);
    ok(!wcscmp(bstr, L"11-12"), "incorrectly formatted date: %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    lcid = MAKELCID(MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN), SORT_DEFAULT);
    hres = VarTokenizeFormatString(date_fmt, buff, sizeof(buff), 1, 1, lcid, NULL);
    ok(hres == S_OK, "VarTokenizeFormatString failed: %lx\n", hres);
    hres = VarFormatFromTokens(&var, date_fmt, buff, 0, &bstr, lcid);
    ok(hres == S_OK, "VarFormatFromTokens failed: %lx\n", hres);
    ok(!wcscmp(bstr, L"12-11"), "incorrectly formatted date: %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    VariantClear(&var);

    V_VT(&var) = VT_R4;
    V_R4(&var) = 1.5;

    lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
    hres = VarTokenizeFormatString(string_fmt, buff, sizeof(buff), 1, 1, lcid, NULL);
    ok(hres == S_OK, "VarTokenizeFormatString failed: %lx\n", hres);
    hres = VarFormatFromTokens(&var, string_fmt, buff, 0, &bstr, lcid);
    ok(hres == S_OK, "VarFormatFromTokens failed: %lx\n", hres);
    ok(!wcscmp(bstr, L"1.5"), "incorrectly formatted string: %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    lcid = MAKELCID(MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN), SORT_DEFAULT);
    hres = VarTokenizeFormatString(string_fmt, buff, sizeof(buff), 1, 1, lcid, NULL);
    ok(hres == S_OK, "VarTokenizeFormatString failed: %lx\n", hres);
    hres = VarFormatFromTokens(&var, string_fmt, buff, 0, &bstr, lcid);
    ok(hres == S_OK, "VarFormatFromTokens failed: %lx\n", hres);
    ok(!wcscmp(bstr, L"1,5"), "incorrectly formatted string: %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
}

static void test_GetAltMonthNames(void)
{
    LPOLESTR *str, *str2;
    HRESULT hr;

    str = (void *)0xdeadbeef;
    hr = GetAltMonthNames(0, &str);
    ok(hr == S_OK, "Unexpected return value %08lx\n", hr);
    ok(str == NULL, "Got %p\n", str);

    str = (void *)0xdeadbeef;
    hr = GetAltMonthNames(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), &str);
    ok(hr == S_OK, "Unexpected return value %08lx\n", hr);
    ok(str == NULL, "Got %p\n", str);

    str = NULL;
    hr = GetAltMonthNames(MAKELCID(MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_EGYPT), SORT_DEFAULT), &str);
    ok(hr == S_OK, "Unexpected return value %08lx\n", hr);
    ok(str != NULL, "Got %p\n", str);

    str2 = NULL;
    hr = GetAltMonthNames(MAKELCID(MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_EGYPT), SORT_DEFAULT), &str2);
    ok(hr == S_OK, "Unexpected return value %08lx\n", hr);
    ok(str2 == str, "Got %p\n", str2);

    str = NULL;
    hr = GetAltMonthNames(MAKELCID(MAKELANGID(LANG_RUSSIAN, SUBLANG_DEFAULT), SORT_DEFAULT), &str);
    ok(hr == S_OK, "Unexpected return value %08lx\n", hr);
    ok(str != NULL, "Got %p\n", str);

    str = NULL;
    hr = GetAltMonthNames(MAKELCID(MAKELANGID(LANG_POLISH, SUBLANG_DEFAULT), SORT_DEFAULT), &str);
    ok(hr == S_OK, "Unexpected return value %08lx\n", hr);
    ok(str != NULL, "Got %p\n", str);
}

static void test_VarFormatCurrency(void)
{
    HRESULT hr;
    VARIANT in;
    BSTR str, str2;

    V_CY(&in).int64 = 0;
    V_VT(&in) = VT_CY;
    hr = VarFormatCurrency(&in, 3, -2, -2, -2, 0, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&in) = VT_BSTR;
    V_BSTR(&in) = str;
    hr = VarFormatCurrency(&in, 1, -2, -2, -2, 0, &str2);
    ok(hr == S_OK, "Unexpected hr %#lx for %s\n", hr, wine_dbgstr_w(str));
    ok(lstrcmpW(str, str2), "Expected different string.\n");
    SysFreeString(str2);

    V_VT(&in) = VT_BSTR | VT_BYREF;
    V_BSTRREF(&in) = &str;
    hr = VarFormatCurrency(&in, 1, -2, -2, -2, 0, &str2);
    ok(hr == S_OK, "Unexpected hr %#lx for %s\n", hr, wine_dbgstr_w(str));
    ok(lstrcmpW(str, str2), "Expected different string.\n");

    SysFreeString(str);
    SysFreeString(str2);

    V_VT(&in) = VT_BSTR;
    V_BSTR(&in) = SysAllocString(L"test");
    hr = VarFormatCurrency(&in, 1, -2, -2, -2, 0, &str2);
    ok(hr == DISP_E_TYPEMISMATCH, "Unexpected hr %#lx.\n", hr);
    VariantClear(&in);
}

static void test_VarFormatDateTime(void)
{
    VARIANT in;
    HRESULT hr;
    BSTR str;

    V_VT(&in) = VT_NULL;
    str = (void *)0xdeadbeef;
    hr = VarFormatDateTime(&in, 0, 0, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!str, "Unexpected out string %p.\n", str);
}

START_TEST(varformat)
{
    test_VarFormatNumber();
    test_VarFormat();
    test_VarWeekdayName();
    test_VarFormatFromTokens();
    test_GetAltMonthNames();
    test_VarFormatCurrency();
    test_VarFormatDateTime();
}
