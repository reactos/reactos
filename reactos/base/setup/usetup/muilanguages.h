#ifndef LANG_MUI_LANGUAGES_H__
#define LANG_MUI_LANGUAGES_H__

#include "lang/af-ZA.h"
#include "lang/ar-AE.h"
#include "lang/ar-BH.h"
#include "lang/ar-DZ.h"
#include "lang/ar-EG.h"
#include "lang/ar-IQ.h"
#include "lang/ar-JO.h"
#include "lang/ar-KW.h"
#include "lang/ar-LB.h"
#include "lang/ar-LY.h"
#include "lang/ar-MA.h"
#include "lang/ar-OM.h"
#include "lang/ar-QA.h"
#include "lang/ar-SA.h"
#include "lang/ar-SY.h"
#include "lang/ar-TN.h"
#include "lang/ar-YE.h"
#include "lang/az-AZ.h"
#include "lang/be-BY.h"
#include "lang/bg-BG.h"
#include "lang/ca-ES.h"
#include "lang/cs-CZ.h"
#include "lang/da-DK.h"
#include "lang/de-AT.h"
#include "lang/de-CH.h"
#include "lang/de-DE.h"
#include "lang/de-LI.h"
#include "lang/de-LU.h"
#include "lang/dv-MV.h"
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
#include "lang/eu-ES.h"
#include "lang/fa-IR.h"
#include "lang/fi-FI.h"
#include "lang/fo-FO.h"
#include "lang/fr-BE.h"
#include "lang/fr-CA.h"
#include "lang/fr-CH.h"
#include "lang/fr-FR.h"
#include "lang/fr-LU.h"
#include "lang/fr-MC.h"
#include "lang/gl-ES.h"
#include "lang/gu-IN.h"
#include "lang/he-IL.h"
#include "lang/hi-IN.h"
#include "lang/hr-HR.h"
#include "lang/hu-HU.h"
#include "lang/hy-AM.h"
#include "lang/id-ID.h"
#include "lang/is-IS.h"
#include "lang/it-CH.h"
#include "lang/it-IT.h"
#include "lang/ja-JP.h"
#include "lang/ka-GE.h"
#include "lang/kk-KZ.h"
#include "lang/kn-IN.h"
#include "lang/kok-IN.h"
#include "lang/ko-KR.h"
#include "lang/ky-KG.h"
#include "lang/lt-LT.h"
#include "lang/lv-LV.h"
#include "lang/mk-MK.h"
#include "lang/mn-MN.h"
#include "lang/mr-IN.h"
#include "lang/ms-BN.h"
#include "lang/ms-MY.h"
#include "lang/nl-BE.h"
#include "lang/nl-NL.h"
#include "lang/nn-NO.h"
#include "lang/pa-IN.h"
#include "lang/pl-PL.h"
#include "lang/pt-BR.h"
#include "lang/pt-PT.h"
#include "lang/ro-RO.h"
#include "lang/ru-RU.h"
#include "lang/sa-IN.h"
#include "lang/sk-SK.h"
#include "lang/sl-SI.h"
#include "lang/sq-AL.h"
#include "lang/sr-SP.h"
#include "lang/sv-FI.h"
#include "lang/sv-SE.h"
#include "lang/sw-KE.h"
#include "lang/syr-SY.h"
#include "lang/ta-IN.h"
#include "lang/te-IN.h"
#include "lang/th-TH.h"
#include "lang/tr-TR.h"
#include "lang/tt-TA.h"
#include "lang/uk-UA.h"
#include "lang/ur-PK.h"
#include "lang/uz-UZ.h"
#include "lang/vi-VN.h"
#include "lang/zh-CN.h"
#include "lang/zh-HK.h"
#include "lang/zh-MO.h"
#include "lang/zh-SG.h"
#include "lang/zh-TW.h"

const MUI_LANGUAGE LanguageList[] =
{
  /* Lang ID,   ANSI CP, OEM CP, MAC CP,   Language Name,                   page strgs,error strings,    other strings, fonts,       kb layouts */
  {L"00000436", L"1252", L"850", L"10000", L"Afrikaans",                    enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    afZALayouts },
  {L"0000041C", L"1250", L"852", L"10029", L"Albanian",                     enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    sqALLayouts },
  {L"00000401", L"1256", L"720", L"10004", L"Arabic (Saudi Arabia)",        enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  arSALayouts },
  {L"00000801", L"1256", L"720", L"10004", L"Arabic (Iraq)",                enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  arIQLayouts },
  {L"00000C01", L"1256", L"720", L"10004", L"Arabic (Egypt)",               enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  arEGLayouts },
  {L"00001001", L"1256", L"720", L"10004", L"Arabic (Libya)",               enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  arLYLayouts },
  {L"00001401", L"1256", L"720", L"10004", L"Arabic (Algeria)",             enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  arDZLayouts },
  {L"00001801", L"1256", L"720", L"10004", L"Arabic (Morocco)",             enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  arMALayouts },
  {L"00001C01", L"1256", L"720", L"10004", L"Arabic (Tunisia)",             enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  arTNLayouts },
  {L"00002001", L"1256", L"720", L"10004", L"Arabic (Oman)",                enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  arOMLayouts },
  {L"00002401", L"1256", L"720", L"10004", L"Arabic (Yemen)",               enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  arYELayouts },
  {L"00002801", L"1256", L"720", L"10004", L"Arabic (Syria)",               enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  arSYLayouts },
  {L"00002C01", L"1256", L"720", L"10004", L"Arabic (Jordan)",              enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  arJOLayouts },
  {L"00003001", L"1256", L"720", L"10004", L"Arabic (Lebanon)",             enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  arLBLayouts },
  {L"00003401", L"1256", L"720", L"10004", L"Arabic (Kuwait)",              enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  arKWLayouts },
  {L"00003801", L"1256", L"720", L"10004", L"Arabic (U.A.E.)",              enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  arAELayouts },
  {L"00003C01", L"1256", L"720", L"10004", L"Arabic (Bahrain)",             enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  arBHLayouts },
  {L"00004001", L"1256", L"720", L"10004", L"Arabic (Qatar)",               enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  arQALayouts },
  {L"0000042B", L"0",    L"1",   L"2",     L"Armenian",                     enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  hyAMLayouts },
  {L"0000082C", L"1251", L"866", L"10007", L"Azeri (Cyrillic)",             enUSPages, enUSErrorEntries, enUSStrings, CyrillicFonts, azAZLayouts },
  {L"0000042C", L"1254", L"857", L"10081", L"Azeri (Latin)",                enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    azAZLayouts },
  {L"0000042D", L"1252", L"850", L"10000", L"Basque",                       enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    euESLayouts },
  {L"00000423", L"1251", L"866", L"10007", L"Belarusian",                   enUSPages, enUSErrorEntries, enUSStrings, CyrillicFonts, beBYLayouts },
  {L"00000402", L"1251", L"866", L"10007", L"Bulgarian",                    bgBGPages, bgBGErrorEntries, bgBGStrings, CyrillicFonts, bgBGLayouts },
  {L"00000455", L"0",    L"1",   L"2",     L"Burmese",                      enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  enUSLayouts },
  {L"00000403", L"1252", L"850", L"10000", L"Catalan",                      enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    caESLayouts },
  {L"00000404", L"950",  L"950", L"10008", L"Chinese (Taiwan)",             enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  zhTWLayouts },
  {L"00000804", L"936",  L"936", L"10008", L"Chinese (PRC)",                enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  zhCNLayouts },
  {L"00000C04", L"950",  L"950", L"10008", L"Chinese (Hong Kong S.A.R.)",   enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  zhHKLayouts },
  {L"00001004", L"936",  L"936", L"10008", L"Chinese (Singapore)",          enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  zhSGLayouts },
  {L"00001404", L"950",  L"950", L"10002", L"Chinese (Macau S.A.R.)",       enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  zhMOLayouts },
  {L"0000041A", L"1250", L"852", L"10029", L"Croatian",                     enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    hrHRLayouts },
  {L"00000405", L"1250", L"852", L"10029", L"Czech",                        csCZPages, csCZErrorEntries, csCZStrings, LatinFonts,    csCZLayouts },
  {L"00000406", L"1252", L"850", L"10000", L"Danish",                       enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    daDKLayouts },
  {L"00000465", L"0",    L"1",   L"2",     L"Divehi (Maldives)",            enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  dvMVLayouts },
  {L"00000413", L"1252", L"850", L"10000", L"Dutch (Netherlands)",          enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    nlNLLayouts },
  {L"00000813", L"1252", L"850", L"10000", L"Dutch (Belgium)",              enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    nlBELayouts },
  {L"00000409", L"1252", L"437", L"10000", L"English (United States)",      enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    enUSLayouts },
  {L"00000809", L"1252", L"850", L"10000", L"English (United Kingdom)",     enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    enGBLayouts },
  {L"00000C09", L"1252", L"850", L"10000", L"English (Australia)",          enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    enAULayouts },
  {L"00001009", L"1252", L"850", L"10000", L"English (Canada)",             enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    enCALayouts },
  {L"00001409", L"1252", L"850", L"10000", L"English (New Zealand)",        enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    enNZLayouts },
  {L"00001809", L"1252", L"850", L"10000", L"English (Ireland)",            enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    enIELayouts },
  {L"00001C09", L"1252", L"437", L"10000", L"English (South Africa)",       enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    enZALayouts },
  {L"00002009", L"1252", L"850", L"10000", L"English (Jamaica)",            enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    enJMLayouts },
  {L"00002409", L"1252", L"850", L"10000", L"English (Caribbean)",          enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    enCBLayouts },
  {L"00002809", L"1252", L"850", L"10000", L"English (Belize)",             enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    enBZLayouts },
  {L"00002C09", L"1252", L"850", L"10000", L"English (Trinidad)",           enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    enTTLayouts },
  {L"00003009", L"1252", L"437", L"10000", L"English (Zimbabwe)",           enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    enZWLayouts },
  {L"00003409", L"1252", L"437", L"10000", L"English (Philippines)",        enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    enPHLayouts },
  {L"00000425", L"1257", L"775", L"10029", L"Estonian",                     enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    etEELayouts },
  {L"00000438", L"1252", L"850", L"10079", L"Faeroese",                     enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    foFOLayouts },
  {L"00000429", L"1256", L"720", L"10004", L"Farsi",                        enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  faIRLayouts },
  {L"0000040B", L"1252", L"850", L"10000", L"Finnish",                      enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    fiFILayouts },
  {L"00000C0C", L"1252", L"850", L"10000", L"French (Canada)",              frFRPages, frFRErrorEntries, frFRStrings, LatinFonts,    frCALayouts },
  {L"0000040C", L"1252", L"850", L"10000", L"French (France)",              frFRPages, frFRErrorEntries, frFRStrings, LatinFonts,    frFRLayouts },
  {L"0000080C", L"1252", L"850", L"10000", L"French (Belgium)",             frFRPages, frFRErrorEntries, frFRStrings, LatinFonts,    frBELayouts },
  {L"0000100C", L"1252", L"850", L"10000", L"French (Switzerland)",         frFRPages, frFRErrorEntries, frFRStrings, LatinFonts,    frCHLayouts },
  {L"0000140C", L"1252", L"850", L"10000", L"French (Luxembourg)",          frFRPages, frFRErrorEntries, frFRStrings, LatinFonts,    frLULayouts },
  {L"0000180C", L"1252", L"850", L"10000", L"French (Monaco)",              frFRPages, frFRErrorEntries, frFRStrings, LatinFonts,    frMCLayouts },
  {L"00000456", L"1252", L"850", L"10000", L"Galician (Spain)",             enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    glESLayouts },
  {L"00000437", L"0",    L"1",   L"2",     L"Georgian",                     enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  kaGELayouts },
  {L"00000407", L"1252", L"850", L"10000", L"German (Germany)",             deDEPages, deDEErrorEntries, deDEStrings, LatinFonts,    deDELayouts },
  {L"00000807", L"1252", L"850", L"10000", L"German (Switzerland)",         deDEPages, deDEErrorEntries, deDEStrings, LatinFonts,    deCHLayouts },
  {L"00000C07", L"1252", L"850", L"10000", L"German (Austria)",             deDEPages, deDEErrorEntries, deDEStrings, LatinFonts,    deATLayouts },
  {L"00001007", L"1252", L"850", L"10000", L"German (Luxembourg)",          deDEPages, deDEErrorEntries, deDEStrings, LatinFonts,    deLULayouts },
  {L"00001407", L"1252", L"850", L"10000", L"German (Liechtenstein)",       deDEPages, deDEErrorEntries, deDEStrings, LatinFonts,    deLILayouts },
  {L"00000408", L"1253", L"737", L"10006", L"Greek",                        elGRPages, elGRErrorEntries, elGRStrings, GreekFonts,    elGRLayouts },
  {L"00000447", L"0",    L"1",   L"2",     L"Gujarati (India)",             enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  guINLayouts },
  {L"0000040D", L"1255", L"862", L"10005", L"Hebrew",                       enUSPages, enUSErrorEntries, enUSStrings, HebrewFonts,   heILLayouts },
  {L"00000439", L"1252", L"437", L"10000", L"Hindi",                        enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  hiINLayouts },
  {L"0000040E", L"1250", L"852", L"10029", L"Hungarian",                    enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    huHULayouts },
  {L"0000040F", L"1252", L"850", L"10079", L"Icelandic",                    enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    isISLayouts },
  {L"00000421", L"1252", L"850", L"10079", L"Indonesian",                   enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    idIDLayouts },
  {L"00000410", L"1252", L"850", L"10000", L"Italian (Italy)",              itITPages, itITErrorEntries, itITStrings, LatinFonts,    itITLayouts },
  {L"00000810", L"1252", L"850", L"10000", L"Italian (Switzerland)",        itITPages, itITErrorEntries, itITStrings, LatinFonts,    itCHLayouts },
  {L"00000411", L"932",  L"932", L"10001", L"Japanese",                     enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  jaJPLayouts },
  {L"0000044B", L"1252", L"437", L"10079", L"Kannada (India)",              enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    knINLayouts },
  {L"0000043F", L"1251", L"866", L"10007", L"Kazakh",                       enUSPages, enUSErrorEntries, enUSStrings, CyrillicFonts, kkKZLayouts },
  {L"00000457", L"0",    L"437", L"2",     L"Konkani",                      enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  kokINLayouts},
  {L"00000412", L"949",  L"949", L"10003", L"Korean",                       enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  koKRLayouts },
  {L"00000440", L"1251", L"866", L"10007", L"Kyrgyz (Kyrgyzstan)",          enUSPages, enUSErrorEntries, enUSStrings, CyrillicFonts, kyKGLayouts },
  {L"00000426", L"1257", L"775", L"10029", L"Latvian",                      enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    lvLVLayouts },
  {L"00000427", L"1257", L"775", L"10029", L"Lithuanian",                   ltLTPages, ltLTErrorEntries, ltLTStrings, LatinFonts,    ltLTLayouts },
  {L"0000042F", L"1251", L"866", L"10007", L"FYRO Macedonian",              enUSPages, enUSErrorEntries, enUSStrings, CyrillicFonts, mkMKLayouts },
  {L"0000083E", L"1252", L"850", L"10000", L"Malay (Brunei Darussalam)",    enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    msBNLayouts },
  {L"0000043E", L"1252", L"850", L"10000", L"Malay (Malaysia)",             enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    msMYLayouts },
  {L"0000044E", L"0",    L"1",   L"2",     L"Marathi",                      enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  mrINLayouts },
  {L"00000450", L"1251", L"866", L"10007", L"Mongolian (Mongolia)",         enUSPages, enUSErrorEntries, enUSStrings, CyrillicFonts, mnMNLayouts },
  {L"00000414", L"1252", L"850", L"10000", L"Norwegian (Bokmal)",           enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    nnNOLayouts },
  {L"00000814", L"1252", L"850", L"10000", L"Norwegian (Nynorsk)",          enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    nnNOLayouts },
  {L"00000415", L"1250", L"852", L"10029", L"Polish",                       plPLPages, plPLErrorEntries, plPLStrings, LatinFonts,    plPLLayouts },
  {L"00000816", L"1252", L"850", L"10000", L"Portuguese (Portugal)",        enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    ptPTLayouts },
  {L"00000416", L"1252", L"850", L"10000", L"Portuguese (Brazil)",          enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    ptBRLayouts },
  {L"00000446", L"0",    L"1",   L"2",     L"Punjabi (India)",              enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  paINLayouts },
  {L"00000418", L"1250", L"852", L"10029", L"Romanian",                     enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    roROLayouts },
  {L"00000419", L"1251", L"866", L"10007", L"Russian",                      ruRUPages, ruRUErrorEntries, ruRUStrings, CyrillicFonts, ruRULayouts },
  {L"0000044F", L"0",    L"1",   L"2",     L"Sanskrit",                     enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  saINLayouts },
  {L"00000C1A", L"1251", L"855", L"10007", L"Serbian (Cyrillic)",           enUSPages, enUSErrorEntries, enUSStrings, CyrillicFonts, srSPLayouts },
  {L"0000081A", L"1250", L"852", L"10029", L"Serbian (Latin)",              enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    srSPLayouts },
  {L"0000041B", L"1250", L"852", L"10029", L"Slovak",                       skSKPages, skSKErrorEntries, skSKStrings, LatinFonts,    skSKLayouts },
  {L"00000424", L"1250", L"852", L"10029", L"Slovenian",                    enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    slSILayouts },
  {L"0000040A", L"1252", L"850", L"10000", L"Spanish (Traditional Sort)",   esESPages, esESErrorEntries, esESStrings, LatinFonts,    esESLayouts },
  {L"0000080A", L"1252", L"850", L"10000", L"Spanish (Mexico)",             esESPages, esESErrorEntries, esESStrings, LatinFonts,    esMXLayouts },
  {L"00000C0A", L"1252", L"850", L"10000", L"Spanish (International Sort)", esESPages, esESErrorEntries, esESStrings, LatinFonts,    esESLayouts },
  {L"0000100A", L"1252", L"850", L"10000", L"Spanish (Guatemala)",          esESPages, esESErrorEntries, esESStrings, LatinFonts,    esGTLayouts },
  {L"0000140A", L"1252", L"850", L"10000", L"Spanish (Costa Rica)",         esESPages, esESErrorEntries, esESStrings, LatinFonts,    esCRLayouts },
  {L"0000180A", L"1252", L"850", L"10000", L"Spanish (Panama)",             esESPages, esESErrorEntries, esESStrings, LatinFonts,    esPALayouts },
  {L"00001C0A", L"1252", L"850", L"10000", L"Spanish (Dominican Republic)", esESPages, esESErrorEntries, esESStrings, LatinFonts,    esDOLayouts },
  {L"0000200A", L"1252", L"850", L"10000", L"Spanish (Venezuela)",          esESPages, esESErrorEntries, esESStrings, LatinFonts,    esVELayouts },
  {L"0000240A", L"1252", L"850", L"10000", L"Spanish (Colombia)",           esESPages, esESErrorEntries, esESStrings, LatinFonts,    esCOLayouts },
  {L"0000280A", L"1252", L"850", L"10000", L"Spanish (Peru)",               esESPages, esESErrorEntries, esESStrings, LatinFonts,    esPELayouts },
  {L"00002C0A", L"1252", L"850", L"10000", L"Spanish (Argentina)",          esESPages, esESErrorEntries, esESStrings, LatinFonts,    esARLayouts },
  {L"0000300A", L"1252", L"850", L"10000", L"Spanish (Ecuador)",            esESPages, esESErrorEntries, esESStrings, LatinFonts,    esECLayouts },
  {L"0000340A", L"1252", L"850", L"10000", L"Spanish (Chile)",              esESPages, esESErrorEntries, esESStrings, LatinFonts,    esCLLayouts },
  {L"0000380A", L"1252", L"850", L"10000", L"Spanish (Uruguay)",            esESPages, esESErrorEntries, esESStrings, LatinFonts,    esUYLayouts },
  {L"00003C0A", L"1252", L"850", L"10000", L"Spanish (Paraguay)",           esESPages, esESErrorEntries, esESStrings, LatinFonts,    esPYLayouts },
  {L"0000400A", L"1252", L"850", L"10000", L"Spanish (Bolivia)",            esESPages, esESErrorEntries, esESStrings, LatinFonts,    esBOLayouts },
  {L"0000440A", L"1252", L"850", L"10000", L"Spanish (El Salvador)",        esESPages, esESErrorEntries, esESStrings, LatinFonts,    esSVLayouts },
  {L"0000480A", L"1252", L"850", L"10000", L"Spanish (Honduras)",           esESPages, esESErrorEntries, esESStrings, LatinFonts,    esHNLayouts },
  {L"00004C0A", L"1252", L"850", L"10000", L"Spanish (Nicaragua)",          esESPages, esESErrorEntries, esESStrings, LatinFonts,    esNILayouts },
  {L"0000500A", L"1252", L"850", L"10000", L"Spanish (Puerto Rico)",        esESPages, esESErrorEntries, esESStrings, LatinFonts,    esPRLayouts },
  {L"00000441", L"1252", L"437", L"10000", L"Swahili",                      enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    swKELayouts },
  {L"0000041D", L"1252", L"850", L"10000", L"Swedish",                      svSEPages, svSEErrorEntries, svSEStrings, LatinFonts,    svSELayouts },
  {L"0000081D", L"1252", L"850", L"10000", L"Swedish (Finland)",            svSEPages, svSEErrorEntries, svSEStrings, LatinFonts,    svFILayouts },
  {L"0000045A", L"0",    L"1",   L"2",     L"Syriac (Syria)",               enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  syrSYLayouts},
  {L"00000449", L"0",    L"1",   L"2",     L"Tamil",                        enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  taINLayouts },
  {L"00000444", L"1251", L"866", L"10007", L"Tatar",                        enUSPages, enUSErrorEntries, enUSStrings, CyrillicFonts, ttTALayouts },
  {L"0000044A", L"0",    L"1",   L"2",     L"Telugu (India)",               enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  teINLayouts },
  {L"0000041E", L"874",  L"874", L"10021", L"Thai",                         enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  thTHLayouts },
  {L"0000041F", L"1254", L"857", L"10081", L"Turkish",                      enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    trTRLayouts },
  {L"00000422", L"1251", L"866", L"10017", L"Ukrainian",                    ukUAPages, ukUAErrorEntries, ukUAStrings, CyrillicFonts, ukUALayouts },
  {L"00000420", L"1256", L"720", L"10004", L"Urdu",                         enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  urPKLayouts },
  {L"00000443", L"1254", L"857", L"10029", L"Uzbek (Latin)",                enUSPages, enUSErrorEntries, enUSStrings, CyrillicFonts, uzUZLayouts },
  {L"00000843", L"1251", L"866", L"10007", L"Uzbek (Cyrillic)",             enUSPages, enUSErrorEntries, enUSStrings, CyrillicFonts, uzUZLayouts },
  {L"0000042A", L"1258", L"1258",L"10000", L"Vietnamese",                   enUSPages, enUSErrorEntries, enUSStrings, UnicodeFonts,  viVNLayouts },
  {L"00000435", L"1252", L"850", L"10000", L"Zulu",                         enUSPages, enUSErrorEntries, enUSStrings, LatinFonts,    enUSLayouts },
  {NULL, NULL, NULL, NULL, NULL, NULL}
};

#endif
