/*
  PROJECT:    ReactOS
  LICENSE:    GPL v2 or any later version
  FILE:       include/host/nls.h
  PURPOSE:    NLS definitions for host tools
  COPYRIGHT:  Copyright 2007 Colin Finck <mail@colinfinck.de>
*/

#ifndef _HOST_NLS_H
#define _HOST_NLS_H

#include <host/typedefs.h>

typedef DWORD LCID;

#define MAKELANGID(p,s)      ((((WORD)(s))<<10)|(WORD)(p))
#define PRIMARYLANGID(l)     ((WORD)(l)&0x3ff)
#define SUBLANGID(l)         ((WORD)(l)>>10)
#define LANGIDFROMLCID(l)    ((WORD)(l))

#define LANG_AFRIKAANS       0x36
#define LANG_ALBANIAN        0x1c
#define LANG_ARABIC          0x01
#define LANG_ARMENIAN        0x2b
#define LANG_ASSAMESE        0x4d
#define LANG_AZERI           0x2c
#define LANG_BASQUE          0x2d
#define LANG_BELARUSIAN      0x23
#define LANG_BENGALI         0x45
#define LANG_BULGARIAN       0x02
#define LANG_CATALAN         0x03
#define LANG_CHINESE         0x04
#define LANG_CROATIAN        0x1a
#define LANG_CZECH           0x05
#define LANG_DANISH          0x06
#define LANG_DIVEHI          0x65
#define LANG_DUTCH           0x13
#define LANG_ENGLISH         0x09
#define LANG_ESTONIAN        0x25
#define LANG_FAEROESE        0x38
#define LANG_FARSI           0x29
#define LANG_FINNISH         0x0b
#define LANG_FRENCH          0x0c
#define LANG_GALICIAN        0x56
#define LANG_GEORGIAN        0x37
#define LANG_GERMAN          0x07
#define LANG_GREEK           0x08
#define LANG_GUJARATI        0x47
#define LANG_HEBREW          0x0d
#define LANG_HINDI           0x39
#define LANG_HUNGARIAN       0x0e
#define LANG_ICELANDIC       0x0f
#define LANG_INDONESIAN      0x21
#define LANG_IRISH           0x3c
#define LANG_ITALIAN         0x10
#define LANG_JAPANESE        0x11
#define LANG_KANNADA         0x4b
#define LANG_KASHMIRI        0x60
#define LANG_KAZAK           0x3f
#define LANG_KONKANI         0x57
#define LANG_KOREAN          0x12
#define LANG_KYRGYZ          0x40
#define LANG_LATVIAN         0x26
#define LANG_LITHUANIAN      0x27
#define LANG_MACEDONIAN      0x2f
#define LANG_MANIPURI        0x58
#define LANG_MALAY           0x3e
#define LANG_MALAYALAM       0x4c
#define LANG_MARATHI         0x4e
#define LANG_MONGOLIAN       0x50
#define LANG_NEPALI          0x61
#define LANG_NEUTRAL         0x00
#define LANG_NORWEGIAN       0x14
#define LANG_ORIYA           0x48
#define LANG_POLISH          0x15
#define LANG_PORTUGUESE      0x16
#define LANG_PUNJABI         0x46
#define LANG_ROMANIAN        0x18
#define LANG_ROMANSH         0x17
#define LANG_RUSSIAN         0x19
#define LANG_SAMI            0x3b
#define LANG_SANSKRIT        0x4f
#define LANG_SERBIAN         0x1a
#define LANG_SINDHI          0x59
#define LANG_SLOVAK          0x1b
#define LANG_SLOVENIAN       0x24
#define LANG_SPANISH         0x0a
#define LANG_SWAHILI         0x41
#define LANG_SWEDISH         0x1d
#define LANG_SYRIAC          0x5a
#define LANG_TAJIK           0x28
#define LANG_TAMIL           0x49
#define LANG_TATAR           0x44
#define LANG_TELUGU          0x4a
#define LANG_THAI            0x1e
#define LANG_TURKISH         0x1f
#define LANG_UKRAINIAN       0x22
#define LANG_URDU            0x20
#define LANG_UZBEK           0x43
#define LANG_VIETNAMESE      0x2a

/* non standard; keep the number high enough (but < 0xff) */
#define LANG_ESPERANTO       0x8f
#define LANG_WALON           0x90
#define LANG_CORNISH         0x91
#define LANG_WELSH           0x92
#define LANG_BRETON          0x93

/* FIXME: these are not in the Windows header */
#define LANG_GAELIC          0x94
#define LANG_MALTESE         0x3a
#define LANG_RHAETO_ROMANCE  0x17
#define LANG_SAAMI           0x3b
#define LANG_SORBIAN         0x2e
#define LANG_LOWER_SORBIAN   0x2e
#define LANG_UPPER_SORBIAN   0x2e
#define LANG_SUTU            0x30
#define LANG_TSONGA          0x31
#define LANG_TSWANA          0x32
#define LANG_VENDA           0x33
#define LANG_XHOSA           0x34
#define LANG_ZULU            0x35

/* Sublanguages */
#define SUBLANG_AZERI_CYRILLIC      0x02
#define SUBLANG_CHINESE_SINGAPORE   0x04
#define SUBLANG_CHINESE_SIMPLIFIED  0x02
#define SUBLANG_DEFAULT             0x01
#define SUBLANG_NEUTRAL             0x00
#define SUBLANG_NORWEGIAN_NYNORSK   0x02
#define SUBLANG_SERBIAN_CYRILLIC    0x03
#define SUBLANG_UZBEK_CYRILLIC      0x02

#endif
