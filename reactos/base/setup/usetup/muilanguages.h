#ifndef LANG_MUI_LANGUAGES_H__
#define LANG_MUI_LANGUAGES_H__

#include "lang/ar-DZ.h"
#include "lang/ar-EG.h"
#include "lang/ar-IQ.h"
#include "lang/ar-LY.h"
#include "lang/ar-SA.h"
#include "lang/az-AZ.h"
#include "lang/be-BY.h"
#include "lang/bg-BG.h"
#include "lang/ca-ES.h"
#include "lang/cs-CZ.h"
#include "lang/da-DK.h"
#include "lang/de-CH.h"
#include "lang/de-DE.h"
#include "lang/el-GR.h"
#include "lang/en-AU.h"
#include "lang/en-BZ.h"
#include "lang/en-CA.h"
#include "lang/en-CB.h"
#include "lang/en-GB.h"
#include "lang/en-IE.h"
#include "lang/en-JM.h"
#include "lang/en-NZ.h"
#include "lang/en-PH.h"
#include "lang/en-TT.h"
#include "lang/en-US.h"
#include "lang/en-ZA.h"
#include "lang/en-ZW.h"
#include "lang/es-AR.h"
#include "lang/es-BO.h"
#include "lang/es-CL.h"
#include "lang/es-CO.h"
#include "lang/es-CR.h"
#include "lang/es-DO.h"
#include "lang/es-EC.h"
#include "lang/es-ES.h"
#include "lang/es-GT.h"
#include "lang/es-HN.h"
#include "lang/es-MX.h"
#include "lang/es-NI.h"
#include "lang/es-PA.h"
#include "lang/es-PE.h"
#include "lang/es-PR.h"
#include "lang/es-PY.h"
#include "lang/es-SV.h"
#include "lang/es-UY.h"
#include "lang/es-VE.h"
#include "lang/et-EE.h"
#include "lang/fi-FI.h"
#include "lang/fr-BE.h"
#include "lang/fr-CA.h"
#include "lang/fr-FR.h"
#include "lang/he-IL.h"
#include "lang/hu-HU.h"
#include "lang/hy-AM.h"
#include "lang/is-IS.h"
#include "lang/it-IT.h"
#include "lang/ja-JP.h"
#include "lang/ka-GE.h"
#include "lang/kk-KZ.h"
#include "lang/ko-KR.h"
#include "lang/lt-LT.h"
#include "lang/lv-LV.h"
#include "lang/mk-MK.h"
#include "lang/nl-BE.h"
#include "lang/nl-NL.h"
#include "lang/nn-NO.h"
#include "lang/pl-PL.h"
#include "lang/pt-BR.h"
#include "lang/pt-PT.h"
#include "lang/ro-RO.h"
#include "lang/ru-RU.h"
#include "lang/sk-SK.h"
#include "lang/sq-AL.h"
#include "lang/sr-SP.h"
#include "lang/sv-SE.h"
#include "lang/th-TH.h"
#include "lang/tr-TR.h"
#include "lang/tt-TA.h"
#include "lang/uk-UA.h"
#include "lang/uz-UZ.h"
#include "lang/vi-VN.h"
#include "lang/zh-CN.h"
#include "lang/zh-HK.h"
#include "lang/zh-SG.h"
#include "lang/zh-TW.h"

const MUI_LANGUAGE LanguageList[] =
{
  /* Lang ID,   ANSI CP, OEM CP, MAC CP,   Language Name,                   page strgs,error strings,    other strings, fonts,     kb layouts */
  {L"0000041C", L"1250", L"852", L"10029", L"Albanian",                     enUSPages, enUSErrorEntries, enUSStrings, CP1250Fonts, sqALLayouts },
  {L"00000401", L"1256", L"720", L"10004", L"Arabic (Saudi Arabia)",        enUSPages, enUSErrorEntries, enUSStrings, CP1256Fonts, arSALayouts },
  {L"00000801", L"1256", L"720", L"10004", L"Arabic (Iraq)",                enUSPages, enUSErrorEntries, enUSStrings, CP1256Fonts, arIQLayouts },
  {L"00000C01", L"1256", L"720", L"10004", L"Arabic (Egypt)",               enUSPages, enUSErrorEntries, enUSStrings, CP1256Fonts, arEGLayouts },
  {L"00001001", L"1256", L"720", L"10004", L"Arabic (Libya)",               enUSPages, enUSErrorEntries, enUSStrings, CP1256Fonts, arLYLayouts },
  {L"00001401", L"1256", L"720", L"10004", L"Arabic (Algeria)",             enUSPages, enUSErrorEntries, enUSStrings, CP1256Fonts, arDZLayouts },
  {L"0000042B", L"0",    L"1",   L"2",     L"Armenian",                     enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,hyAMLayouts },
  {L"0000082C", L"1251", L"866", L"10007", L"Azeri (Cyrillic)",             enUSPages, enUSErrorEntries, enUSStrings, CP1251Fonts, azAZLayouts },
  {L"0000042C", L"1254", L"857", L"10081", L"Azeri (Latin)",                enUSPages, enUSErrorEntries, enUSStrings, CP1254Fonts, azAZLayouts },
  {L"00000423", L"1251", L"866", L"10007", L"Belarusian",                   enUSPages, enUSErrorEntries, enUSStrings, CP1251Fonts, beBYLayouts },
  {L"00000402", L"1251", L"866", L"10007", L"Bulgarian",                    bgBGPages, bgBGErrorEntries, bgBGStrings, CP1251Fonts, bgBGLayouts },
  {L"00000455", L"0",    L"1",   L"2",     L"Burmese",                      enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,enUSLayouts },
  {L"00000403", L"1252", L"850", L"10000", L"Catalan",                      enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts, caESLayouts },
  {L"00000404", L"950",  L"950", L"10008", L"Chinese (Taiwan)",             enUSPages, enUSErrorEntries, enUSStrings, CP950Fonts,  zhTWLayouts },
  {L"00000804", L"936",  L"936", L"10008", L"Chinese (PRC)",                enUSPages, enUSErrorEntries, enUSStrings, CP936Fonts,  zhCNLayouts },
  {L"00000C04", L"950",  L"950", L"10008", L"Chinese (Hong Kong S.A.R.)",   enUSPages, enUSErrorEntries, enUSStrings, CP950Fonts,  zhHKLayouts },
  {L"00001004", L"936",  L"936", L"10008", L"Chinese (Singapore)",          enUSPages, enUSErrorEntries, enUSStrings, CP936Fonts,  zhSGLayouts },
  {L"00000405", L"1250", L"852", L"10029", L"Czech",                        csCZPages, csCZErrorEntries, csCZStrings, CP1250Fonts, csCZLayouts },
  {L"00000406", L"1252", L"850", L"10000", L"Danish",                       enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts, daDKLayouts },
  {L"00000413", L"1252", L"850", L"10000", L"Dutch (Netherlands)",          enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts, nlNLLayouts },
  {L"00000813", L"1252", L"850", L"10000", L"Dutch (Belgium)",              enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts, nlBELayouts },
  {L"00000409", L"1252", L"437", L"10000", L"English (United States)",      enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts, enUSLayouts },
  {L"00000809", L"1252", L"850", L"10000", L"English (United Kingdom)",     enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts, enGBLayouts },
  {L"00000C09", L"1252", L"850", L"10000", L"English (Australia)",          enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts, enAULayouts },
  {L"00001009", L"1252", L"850", L"10000", L"English (Canada)",             enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts, enCALayouts },
  {L"00001409", L"1252", L"850", L"10000", L"English (New Zealand)",        enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts, enNZLayouts },
  {L"00001809", L"1252", L"850", L"10000", L"English (Ireland)",            enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts, enIELayouts },
  {L"00001C09", L"1252", L"437", L"10000", L"English (South Africa)",       enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts, enZALayouts },
  {L"00002009", L"1252", L"850", L"10000", L"English (Jamaica)",            enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts, enJMLayouts },
  {L"00002409", L"1252", L"850", L"10000", L"English (Caribbean)",          enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts, enCBLayouts },
  {L"00002809", L"1252", L"850", L"10000", L"English (Belize)",             enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts, enBZLayouts },
  {L"00002C09", L"1252", L"850", L"10000", L"English (Trinidad)",           enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts, enTTLayouts },
  {L"00003009", L"1252", L"437", L"10000", L"English (Zimbabwe)",           enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts, enZWLayouts },
  {L"00003409", L"1252", L"437", L"10000", L"English (Philippines)",        enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts, enPHLayouts },
  {L"00000425", L"1257", L"775", L"10029", L"Estonian",                     enUSPages, enUSErrorEntries, enUSStrings, CP1257Fonts, etEELayouts },
  {L"0000040B", L"1252", L"850", L"10000", L"Finnish",                      enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts, fiFILayouts },
  {L"00000C0C", L"1252", L"850", L"10000", L"French (Canada)",              frFRPages, frFRErrorEntries, frFRStrings, CP1252Fonts, frCALayouts },
  {L"0000040C", L"1252", L"850", L"10000", L"French (France)",              frFRPages, frFRErrorEntries, frFRStrings, CP1252Fonts, frFRLayouts },
  {L"0000080C", L"1252", L"850", L"10000", L"French (Belgium)",             frFRPages, frFRErrorEntries, frFRStrings, CP1252Fonts, frBELayouts },
  {L"00000437", L"0",    L"1",   L"2",     L"Georgian",                     enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,kaGELayouts },
  {L"00000407", L"1252", L"850", L"10000", L"German (Germany)",             deDEPages, deDEErrorEntries, deDEStrings, CP1252Fonts, deDELayouts },
  {L"00000807", L"1252", L"850", L"10000", L"German (Switzerland)",         deDEPages, deDEErrorEntries, deDEStrings, CP1252Fonts, deCHLayouts },
  {L"00000408", L"1253", L"737", L"10006", L"Greek",                        elGRPages, elGRErrorEntries, elGRStrings, CP1253Fonts, elGRLayouts },
  {L"0000040D", L"1255", L"862", L"10005", L"Hebrew",                       enUSPages, enUSErrorEntries, enUSStrings, CP1255Fonts, heILLayouts },
  {L"0000040E", L"1250", L"852", L"10029", L"Hungarian",                    enUSPages, enUSErrorEntries, enUSStrings, CP1250Fonts, huHULayouts },
  {L"0000040F", L"1252", L"850", L"10079", L"Icelandic",                    enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts, isISLayouts },
  {L"00000410", L"1252", L"850", L"10000", L"Italian (Italy)",              itITPages, itITErrorEntries, itITStrings, CP1252Fonts, itITLayouts },
  {L"00000411", L"932",  L"932", L"10001", L"Japanese",                     enUSPages, enUSErrorEntries, enUSStrings, CP932Fonts,  jaJPLayouts },
  {L"0000043F", L"1251", L"866", L"10007", L"Kazakh",                       enUSPages, enUSErrorEntries, enUSStrings, CP1251Fonts, kkKZLayouts },
  {L"00000412", L"949",  L"949", L"10003", L"Korean",                       enUSPages, enUSErrorEntries, enUSStrings, CP949Fonts,  koKRLayouts },
  {L"00000426", L"1257", L"775", L"10029", L"Latvian",                      enUSPages, enUSErrorEntries, enUSStrings, CP1257Fonts, lvLVLayouts },
  {L"00000427", L"1257", L"775", L"10029", L"Lithuanian",                   ltLTPages, ltLTErrorEntries, ltLTStrings, CP1257Fonts, ltLTLayouts },
  {L"0000042F", L"1251", L"866", L"10007", L"FYRO Macedonian",              enUSPages, enUSErrorEntries, enUSStrings, CP1251Fonts, mkMKLayouts },
  {L"00000414", L"1252", L"850", L"10000", L"Norwegian (Bokmal)",           enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts, nnNOLayouts },
  {L"00000415", L"1250", L"852", L"10029", L"Polish",                       plPLPages, plPLErrorEntries, plPLStrings, CP1250Fonts, plPLLayouts },
  {L"00000816", L"1252", L"850", L"10000", L"Portuguese (Portugal)",        enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts, ptPTLayouts },
  {L"00000416", L"1252", L"850", L"10000", L"Portuguese (Brazil)",          enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts, ptBRLayouts },
  {L"00000418", L"1250", L"852", L"10029", L"Romanian",                     enUSPages, enUSErrorEntries, enUSStrings, CP1250Fonts, roROLayouts },
  {L"00000419", L"1251", L"866", L"10007", L"Russian",                      ruRUPages, ruRUErrorEntries, ruRUStrings, CP1251Fonts, ruRULayouts },
  {L"00000C1A", L"1251", L"855", L"10007", L"Serbian (Cyrillic)",           enUSPages, enUSErrorEntries, enUSStrings, CP1251Fonts, srSPLayouts },
  {L"0000081A", L"1250", L"852", L"10029", L"Serbian (Latin)",              enUSPages, enUSErrorEntries, enUSStrings, CP1250Fonts, srSPLayouts },
  {L"0000041B", L"1250", L"852", L"10029", L"Slovak",                       skSKPages, skSKErrorEntries, skSKStrings, CP1250Fonts, skSKLayouts },
  {L"0000040A", L"1252", L"850", L"10000", L"Spanish (Traditional Sort)",   esESPages, esESErrorEntries, esESStrings, CP1252Fonts, esESLayouts },
  {L"0000080A", L"1252", L"850", L"10000", L"Spanish (Mexico)",             esESPages, esESErrorEntries, esESStrings, CP1252Fonts, esMXLayouts },
  {L"00000C0A", L"1252", L"850", L"10000", L"Spanish (International Sort)", esESPages, esESErrorEntries, esESStrings, CP1252Fonts, esESLayouts },
  {L"0000100A", L"1252", L"850", L"10000", L"Spanish (Guatemala)",          esESPages, esESErrorEntries, esESStrings, CP1252Fonts, esGTLayouts },
  {L"0000140A", L"1252", L"850", L"10000", L"Spanish (Costa Rica)",         esESPages, esESErrorEntries, esESStrings, CP1252Fonts, esCRLayouts },
  {L"0000180A", L"1252", L"850", L"10000", L"Spanish (Panama)",             esESPages, esESErrorEntries, esESStrings, CP1252Fonts, esPALayouts },
  {L"00001C0A", L"1252", L"850", L"10000", L"Spanish (Dominican Republic)", esESPages, esESErrorEntries, esESStrings, CP1252Fonts, esDOLayouts },
  {L"0000200A", L"1252", L"850", L"10000", L"Spanish (Venezuela)",          esESPages, esESErrorEntries, esESStrings, CP1252Fonts, esVELayouts },
  {L"0000240A", L"1252", L"850", L"10000", L"Spanish (Colombia)",           esESPages, esESErrorEntries, esESStrings, CP1252Fonts, esCOLayouts },
  {L"0000280A", L"1252", L"850", L"10000", L"Spanish (Peru)",               esESPages, esESErrorEntries, esESStrings, CP1252Fonts, esPELayouts },
  {L"00002C0A", L"1252", L"850", L"10000", L"Spanish (Argentina)",          esESPages, esESErrorEntries, esESStrings, CP1252Fonts, esARLayouts },
  {L"0000300A", L"1252", L"850", L"10000", L"Spanish (Ecuador)",            esESPages, esESErrorEntries, esESStrings, CP1252Fonts, esECLayouts },
  {L"0000340A", L"1252", L"850", L"10000", L"Spanish (Chile)",              esESPages, esESErrorEntries, esESStrings, CP1252Fonts, esCLLayouts },
  {L"0000380A", L"1252", L"850", L"10000", L"Spanish (Uruguay)",            esESPages, esESErrorEntries, esESStrings, CP1252Fonts, esUYLayouts },
  {L"00003C0A", L"1252", L"850", L"10000", L"Spanish (Paraguay)",           esESPages, esESErrorEntries, esESStrings, CP1252Fonts, esPYLayouts },
  {L"0000400A", L"1252", L"850", L"10000", L"Spanish (Bolivia)",            esESPages, esESErrorEntries, esESStrings, CP1252Fonts, esBOLayouts },
  {L"0000440A", L"1252", L"850", L"10000", L"Spanish (El Salvador)",        esESPages, esESErrorEntries, esESStrings, CP1252Fonts, esSVLayouts },
  {L"0000480A", L"1252", L"850", L"10000", L"Spanish (Honduras)",           esESPages, esESErrorEntries, esESStrings, CP1252Fonts, esHNLayouts },
  {L"00004C0A", L"1252", L"850", L"10000", L"Spanish (Nicaragua)",          esESPages, esESErrorEntries, esESStrings, CP1252Fonts, esNILayouts },
  {L"0000500A", L"1252", L"850", L"10000", L"Spanish (Puerto Rico)",        esESPages, esESErrorEntries, esESStrings, CP1252Fonts, esPRLayouts },
  {L"0000041D", L"1252", L"850", L"10000", L"Swedish",                      svSEPages, svSEErrorEntries, svSEStrings, CP1252Fonts, svSELayouts },
  {L"00000444", L"1251", L"866", L"10007", L"Tatar",                        enUSPages, enUSErrorEntries, enUSStrings, CP1251Fonts, ttTALayouts },
  {L"0000041E", L"874",  L"874", L"10021", L"Thai",                         enUSPages, enUSErrorEntries, enUSStrings, CP874Fonts,  thTHLayouts },
  {L"0000041F", L"1254", L"857", L"10081", L"Turkish",                      enUSPages, enUSErrorEntries, enUSStrings, CP1254Fonts, trTRLayouts },
  {L"00000422", L"1251", L"866", L"10017", L"Ukrainian",                    ukUAPages, ukUAErrorEntries, ukUAStrings, CP1251Fonts, ukUALayouts },
  {L"00000843", L"1251", L"866", L"10007", L"Uzbek (Cyrillic)",             enUSPages, enUSErrorEntries, enUSStrings, CP1251Fonts, uzUZLayouts },
  {L"0000042A", L"1258", L"1258",L"10000", L"Vietnamese",                   enUSPages, enUSErrorEntries, enUSStrings, CP1258Fonts, viVNLayouts },
  {L"00000435", L"1252", L"850", L"10000", L"Zulu",                         enUSPages, enUSErrorEntries, enUSStrings, CP1252Fonts, enUSLayouts },
  {NULL, NULL, NULL, NULL, NULL, NULL}
};

#endif
