#ifndef LANG_MUI_LANGUAGES_H__
#define LANG_MUI_LANGUAGES_H__

#include "lang/bg-BG.h"
#include "lang/cs-CZ.h"
#include "lang/en-US.h"
#include "lang/de-DE.h"
#include "lang/el-GR.h"
#include "lang/es-ES.h"
#include "lang/fr-FR.h"
#include "lang/it-IT.h"
#include "lang/pl-PL.h"
#include "lang/ru-RU.h"
#include "lang/sk-SK.h"
#include "lang/sv-SE.h"
#include "lang/uk-UA.h"
#include "lang/lt-LT.h"

const MUI_LANGUAGE LanguageList[] =
{
  /* Lang ID,   DefKbdLayout, SecKbLayout, ANSI CP, OEM CP, MAC CP,   Language Name,        page strgs,error strings,    other strings */
  {L"00000409", L"00000409",  NULL,        L"1252", L"437", L"10000", L"English",           enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts },
  {L"0000041C", L"0000041C",  L"00000409", L"1250", L"852", L"10029", L"Albanian",          enUSPages, enUSErrorEntries, enUSStrings, CP1250Fonts },
  {L"00000401", L"00000401",  L"00000409", L"1256", L"720", L"10004", L"Arabic",            enUSPages, enUSErrorEntries, enUSStrings, CP1256Fonts },
  {L"0000042B", L"0000042B",  L"00000409", L"0",    L"1",   L"2",     L"Armenian Eastern",  enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts},
  {L"0000082C", L"0000082C",  L"00000409", L"1251", L"866", L"10007", L"Azeri Cyrillic",    enUSPages, enUSErrorEntries, enUSStrings, CP1251Fonts },
  {L"0000042C", L"0000042C",  L"00000409", L"1254", L"857", L"10081", L"Azeri Latin",       enUSPages, enUSErrorEntries, enUSStrings, CP1254Fonts },
  {L"00000423", L"00000423",  L"00000409", L"1251", L"866", L"10007", L"Belarusian",        enUSPages, enUSErrorEntries, enUSStrings, CP1251Fonts },
  {L"00000813", L"00000813",  L"00000409", L"1252", L"850", L"10000", L"Belgian (Dutch)",   enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts },
  {L"0000080C", L"0000080C",  L"00000409", L"1252", L"850", L"10000", L"Belgian (French)",  enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts },
  {L"00000416", L"00010416",  L"00000409", L"1252", L"850", L"10000", L"Brazilian",         enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts },
  {L"00000402", L"00000402",  L"00000409", L"1251", L"866", L"10007", L"Bulgarian",         bgBGPages, bgBGErrorEntries, bgBGStrings, CP1251Fonts },
  {L"00000455", L"00000455",  L"00000409", L"0",    L"1",   L"2",     L"Burmese",           enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts},
  {L"00000C0C", L"00000C0C",  L"00000409", L"1252", L"850", L"10000", L"Canadian (French)", enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts },
  {L"00000403", L"0000040A",  L"00000409", L"1252", L"850", L"10000", L"Catalan",           enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts },
  {L"00000804", L"00000804",  L"00000409", L"936",  L"936", L"10008", L"Chinese (PRC)",     enUSPages, enUSErrorEntries, enUSStrings, CP936Fonts  },
  {L"00000405", L"00000405",  L"00000409", L"1250", L"852", L"10029", L"Czech",             csCZPages, csCZErrorEntries, csCZStrings, CP1250Fonts },
  {L"00000406", L"00000406",  L"00000409", L"1252", L"850", L"10000", L"Danish",            enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts },
  {L"00000407", L"00000407",  L"00000409", L"1252", L"850", L"10000", L"Deutsch",           deDEPages, deDEErrorEntries, deDEStrings, CP1252Fonts },
  {L"00000413", L"00000813",  L"00000409", L"1252", L"850", L"10000", L"Dutch",             enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts },
  {L"00000425", L"00000425",  L"00000409", L"1257", L"775", L"10029", L"Estonian",          enUSPages, enUSErrorEntries, enUSStrings, CP1257Fonts },
  {L"0000040B", L"0000040B",  L"00000409", L"1252", L"850", L"10000", L"Finnish",           enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts },
  {L"0000040C", L"0000040C",  L"00000409", L"1252", L"850", L"10000", L"French",            frFRPages, frFRErrorEntries, frFRStrings, CP1252Fonts },
  {L"00000437", L"00000437",  L"00000409", L"0",    L"1",   L"2",     L"Georgian",          enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts},
  {L"00000408", L"00000408",  L"00000409", L"1253", L"737", L"10006", L"Greek",             elGRPages, elGRErrorEntries, elGRStrings, CP1253Fonts },
  {L"0000040D", L"0000040D",  L"00000409", L"1255", L"862", L"10005", L"Hebrew",            enUSPages, enUSErrorEntries, enUSStrings, CP1255Fonts },
  {L"0000040E", L"0000040E",  L"00000409", L"1250", L"852", L"10029", L"Hungarian",         enUSPages, enUSErrorEntries, enUSStrings, CP1250Fonts },
  {L"0000040F", L"0000040F",  L"00000409", L"1252", L"850", L"10079", L"Icelandic",         enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts },
  {L"00000410", L"00000410",  L"00000409", L"1252", L"850", L"10000", L"Italian",           itITPages, itITErrorEntries, itITStrings, CP1252Fonts },
  {L"00000411", L"00000411",  L"00000409", L"932",  L"932", L"10001", L"Japanese",          enUSPages, enUSErrorEntries, enUSStrings, CP932Fonts  },
  {L"0000043F", L"0000043F",  L"00000409", L"1251", L"866", L"10007", L"Kazakh",            enUSPages, enUSErrorEntries, enUSStrings, CP1251Fonts },
  {L"00000412", L"00000412",  L"00000409", L"949",  L"949", L"10003", L"Korean",            enUSPages, enUSErrorEntries, enUSStrings, CP949Fonts  },
  {L"00000426", L"00000426",  L"00000409", L"1257", L"775", L"10029", L"Latvian",           enUSPages, enUSErrorEntries, enUSStrings, CP1257Fonts },
  {L"00000427", L"00000427",  L"00000409", L"1257", L"775", L"10029", L"Lithuanian",        ltLTPages, ltLTErrorEntries, ltLTStrings, CP1257Fonts },
  {L"0000042F", L"0000042F",  L"00000409", L"1251", L"866", L"10007", L"Macedonian",        enUSPages, enUSErrorEntries, enUSStrings, CP1251Fonts },
  {L"00000414", L"00000414",  L"00000409", L"1252", L"850", L"10000", L"Norwegian",         enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts },
  {L"00000418", L"00000418",  L"00000409", L"1250", L"852", L"10029", L"Romanian",          enUSPages, enUSErrorEntries, enUSStrings, CP1250Fonts },
  {L"00000419", L"00000419",  L"00000409", L"1251", L"866", L"10007", L"Russkij",           ruRUPages, ruRUErrorEntries, ruRUStrings, CP1251Fonts },
  {L"00000415", L"00000415",  L"00000409", L"1250", L"852", L"10029", L"Polski",            plPLPages, plPLErrorEntries, plPLStrings, CP1250Fonts },
  {L"00000816", L"00000816",  L"00000409", L"1252", L"850", L"10000", L"Portuguese",        enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts },
  {L"00000C1A", L"00000C1A",  L"00000409", L"1251", L"855", L"10007", L"Serbian (Cyrillic)",enUSPages, enUSErrorEntries, enUSStrings, CP1251Fonts },
  {L"0000081A", L"0000081A",  L"00000409", L"1250", L"852", L"10029", L"Serbian (Latin)",   enUSPages, enUSErrorEntries, enUSStrings, CP1250Fonts },
  {L"0000041B", L"0000041B",  L"00000409", L"1250", L"852", L"10029", L"Slovak",            skSKPages, skSKErrorEntries, skSKStrings, CP1250Fonts },
  {L"0000040A", L"0000040A",  L"00000409", L"1252", L"850", L"10000", L"Spanish",           esESPages, esESErrorEntries, esESStrings, CP1252Fonts },
  {L"00000807", L"00000807",  L"00000409", L"1252", L"850", L"10000", L"Swiss (German)",    enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts },
  {L"0000041D", L"0000041D",  L"00000409", L"1252", L"850", L"10000", L"Swedish",           svSEPages, svSEErrorEntries, svSEStrings, CP1252Fonts },
  {L"00000444", L"00000444",  L"00000409", L"1251", L"866", L"10007", L"Tatar",             enUSPages, enUSErrorEntries, enUSStrings, CP1251Fonts },
  {L"0000041E", L"0000041E",  L"00000409", L"874",  L"874", L"10021", L"Thai",              enUSPages, enUSErrorEntries, enUSStrings, CP874Fonts  },
  {L"0000041F", L"0000041F",  L"00000409", L"1254", L"857", L"10081", L"Turkish",           enUSPages, enUSErrorEntries, enUSStrings, CP1254Fonts },
  {L"00000422", L"00000422",  L"00000409", L"1251", L"866", L"10017", L"Ukrainian",         ukUAPages, ukUAErrorEntries, ukUAStrings, CP1251Fonts },
  {L"00000809", L"00000809",  L"00000409", L"1252", L"850", L"10000", L"United Kingdom",    enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts },
  {L"00000843", L"00000843",  L"00000409", L"1251", L"866", L"10007", L"Uzbek",             enUSPages, enUSErrorEntries, enUSStrings, CP1251Fonts },
  {L"0000042A", L"0000042A",  L"00000409", L"1258", L"1258",L"10000", L"Vietnamese",        enUSPages, enUSErrorEntries, enUSStrings, CP1258Fonts },
  {NULL, NULL, NULL, NULL, NULL, NULL}
};

#endif
