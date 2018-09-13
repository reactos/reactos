
#include "private.h"

//
//  CLSID for MLANG objects
//
typedef struct tagCLSIDOBJ
{
    LPCTSTR szCLSID;
    LPCTSTR szDesc;
}   CLSIDOBJ, *LPCLSIDOBJ;

const CLSIDOBJ clsidObj[] =
{
    { TEXT("CLSID\\{275C23E2-3747-11D0-9FEA-00AA003F8646}"), TEXT("Multi Language Support") },          // CLSID_MLANG
    { TEXT("CLSID\\{C04D65CF-B70D-11D0-B188-00AA0038C969}"), TEXT("Multi Language String") },           // CLSID_MLANG
    { TEXT("CLSID\\{D66D6F99-CDAA-11D0-B822-00C04FC9B31F}"), TEXT("Multi Language ConvertCharset") },   // CLSID_MLANG
    { NULL, NULL }
};

LPCTSTR szInProcServer = TEXT("InProcServer32");
LPCTSTR szThreadingModel = TEXT("ThreadingModel");
LPCTSTR szThreadingModelValue = TEXT("Both");


HRESULT RegisterServerInfo(void)
{
    HKEY hKey = NULL, hKeySub = NULL;
    DWORD dwAction = 0;
    int i = 0;
    TCHAR szModule[MAX_PATH];
    HRESULT hr = S_OK;

    if (!GetModuleFileName(g_hInst, szModule, ARRAYSIZE(szModule)))
        hr = E_FAIL;

    while (SUCCEEDED(hr) && clsidObj[i].szCLSID)
    {
        if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CLASSES_ROOT, clsidObj[i].szCLSID, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwAction))
        {
            ASSERT(NULL != hKey);
            if (ERROR_SUCCESS != RegSetValueEx(hKey, NULL, 0, REG_SZ, (LPBYTE)clsidObj[i].szDesc, (lstrlen(clsidObj[i].szDesc) + 1) * sizeof(TCHAR)))
                hr = E_FAIL;

            if (ERROR_SUCCESS == RegCreateKeyEx(hKey, szInProcServer, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeySub, &dwAction))
            {
                ASSERT(NULL != hKeySub);
                if (ERROR_SUCCESS != RegSetValueEx(hKeySub, NULL, 0, REG_SZ, (LPBYTE)szModule, (lstrlen(szModule) + 1) * sizeof(TCHAR)))
                    hr = E_FAIL;
                if (ERROR_SUCCESS != RegSetValueEx(hKeySub, szThreadingModel, 0, REG_SZ, (LPBYTE)szThreadingModelValue, (lstrlen(szThreadingModelValue) + 1) * sizeof(TCHAR)))
                    hr = E_FAIL;

                RegCloseKey(hKeySub);
                hKeySub = NULL;
            }
            else
                hr = E_FAIL;

            RegCloseKey(hKey);
            hKey = NULL;
        }
        else
            hr = E_FAIL;
        i++;
    }
    return hr;
}

HRESULT UnregisterServerInfo(void)
{
    HKEY hKey = NULL;
    int i = 0;
    HRESULT hr = S_OK;

    while (clsidObj[i].szCLSID)
    {
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, clsidObj[i].szCLSID, 0, KEY_ALL_ACCESS, &hKey))
        {
            ASSERT(NULL != hKey);
            if (ERROR_SUCCESS != RegDeleteKey(hKey, szInProcServer))
                hr = E_FAIL;

            RegCloseKey(hKey);
            hKey = NULL;

            if (ERROR_SUCCESS != RegDeleteKey(HKEY_CLASSES_ROOT, clsidObj[i].szCLSID))
                hr = E_FAIL;
        }
        else
            hr = S_FALSE;
        i++;
    }
    return hr;
}



// Legacy code for Outlook Express and other clients who depend on MLang created MIME DB in registry
// Those client should switch to MLang interface for MIME data

long PrivRegSetValueEx(HKEY hKey, LPCTSTR lpValueName, DWORD dwType, CONST BYTE *lpData, DWORD cbData, BOOL bOverWrite)
{
    long lRet = ERROR_SUCCESS;

    if (bOverWrite || ERROR_SUCCESS != RegQueryValueEx(hKey, lpValueName, 0, NULL, NULL, NULL))
        lRet = RegSetValueEx(hKey, lpValueName, 0, dwType, lpData, cbData);
    return lRet;
}

//
//  MIME database Key or Value name
//
LPCTSTR szMIMEDatabase = TEXT("MIME\\Database");
LPCTSTR szCharset = TEXT("Charset");
LPCTSTR szRfc1766 = TEXT("Rfc1766");
LPCTSTR szCodepage = TEXT("Codepage");
LPCTSTR szInternetEncoding = TEXT("InternetEncoding");
LPCTSTR szAliasForCharset = TEXT("AliasForCharset");
LPCTSTR szHeaderCharset = TEXT("HeaderCharset");
LPCTSTR szBodyCharset = TEXT("BodyCharset");
LPCTSTR szWebCharset = TEXT("WebCharset");
LPCTSTR szFixedWidthFont = TEXT("FixedWidthFont");
LPCTSTR szProportionalFont = TEXT("ProportionalFont");
LPCTSTR szDescription = TEXT("Description");
LPCTSTR szFamily = TEXT("Family");
LPCTSTR szLevel = TEXT("Level");
LPCTSTR szEncoding = TEXT("Encoding");

//
//  RFC1766 registry data at MIME\Database
//
typedef struct tagREGRFC1766
{
    LPCTSTR szLCID;
    LPCTSTR szAcceptLang;
    UINT    uidLCID;
}   REGRFC1766, *LPREGRFC1766;

const REGRFC1766 regRfc1766[] =
{
    { TEXT("0436"), TEXT("af"),    IDS_RFC1766_LCID0436 },
    { TEXT("041C"), TEXT("sq"),    IDS_RFC1766_LCID041C },
    { TEXT("0001"), TEXT("ar"),    IDS_RFC1766_LCID0001 },
    { TEXT("0401"), TEXT("ar-sa"), IDS_RFC1766_LCID0401 },
    { TEXT("0801"), TEXT("ar-iq"), IDS_RFC1766_LCID0801 },
    { TEXT("0C01"), TEXT("ar-eg"), IDS_RFC1766_LCID0C01 },
    { TEXT("1001"), TEXT("ar-ly"), IDS_RFC1766_LCID1001 },
    { TEXT("1401"), TEXT("ar-dz"), IDS_RFC1766_LCID1401 },
    { TEXT("1801"), TEXT("ar-ma"), IDS_RFC1766_LCID1801 },
    { TEXT("1C01"), TEXT("ar-tn"), IDS_RFC1766_LCID1C01 },
    { TEXT("2001"), TEXT("ar-om"), IDS_RFC1766_LCID2001 },
    { TEXT("2401"), TEXT("ar-ye"), IDS_RFC1766_LCID2401 },
    { TEXT("2801"), TEXT("ar-sy"), IDS_RFC1766_LCID2801 },
    { TEXT("2C01"), TEXT("ar-jo"), IDS_RFC1766_LCID2C01 },
    { TEXT("3001"), TEXT("ar-lb"), IDS_RFC1766_LCID3001 },
    { TEXT("3401"), TEXT("ar-kw"), IDS_RFC1766_LCID3401 },
    { TEXT("3801"), TEXT("ar-ae"), IDS_RFC1766_LCID3801 },
    { TEXT("3C01"), TEXT("ar-bh"), IDS_RFC1766_LCID3C01 },
    { TEXT("4001"), TEXT("ar-qa"), IDS_RFC1766_LCID4001 },
    { TEXT("042D"), TEXT("eu"),    IDS_RFC1766_LCID042D },
    { TEXT("0402"), TEXT("bg"),    IDS_RFC1766_LCID0402 },
    { TEXT("0423"), TEXT("be"),    IDS_RFC1766_LCID0423 },
    { TEXT("0403"), TEXT("ca"),    IDS_RFC1766_LCID0403 },
    { TEXT("0004"), TEXT("zh"),    IDS_RFC1766_LCID0004 },
    { TEXT("0404"), TEXT("zh-tw"), IDS_RFC1766_LCID0404 },
    { TEXT("0804"), TEXT("zh-cn"), IDS_RFC1766_LCID0804 },
    { TEXT("0C04"), TEXT("zh-hk"), IDS_RFC1766_LCID0C04 },
    { TEXT("1004"), TEXT("zh-sg"), IDS_RFC1766_LCID1004 },
    { TEXT("041A"), TEXT("hr"),    IDS_RFC1766_LCID041A },
    { TEXT("0405"), TEXT("cs"),    IDS_RFC1766_LCID0405 },
    { TEXT("0406"), TEXT("da"),    IDS_RFC1766_LCID0406 },
    { TEXT("0413"), TEXT("nl"),    IDS_RFC1766_LCID0413 },
    { TEXT("0813"), TEXT("nl-be"), IDS_RFC1766_LCID0813 },
    { TEXT("0009"), TEXT("en"),    IDS_RFC1766_LCID0009 },
    { TEXT("0409"), TEXT("en-us"), IDS_RFC1766_LCID0409 },
    { TEXT("0809"), TEXT("en-gb"), IDS_RFC1766_LCID0809 },
    { TEXT("0C09"), TEXT("en-au"), IDS_RFC1766_LCID0C09 },
    { TEXT("1009"), TEXT("en-ca"), IDS_RFC1766_LCID1009 },
    { TEXT("1409"), TEXT("en-nz"), IDS_RFC1766_LCID1409 },
    { TEXT("1809"), TEXT("en-ie"), IDS_RFC1766_LCID1809 },
    { TEXT("1C09"), TEXT("en-za"), IDS_RFC1766_LCID1C09 },
    { TEXT("2009"), TEXT("en-jm"), IDS_RFC1766_LCID2009 },
    { TEXT("2809"), TEXT("en-bz"), IDS_RFC1766_LCID2809 },
    { TEXT("2C09"), TEXT("en-tt"), IDS_RFC1766_LCID2C09 },
    { TEXT("0425"), TEXT("et"),    IDS_RFC1766_LCID0425 },
    { TEXT("0438"), TEXT("fo"),    IDS_RFC1766_LCID0438 },
    { TEXT("0429"), TEXT("fa"),    IDS_RFC1766_LCID0429 },
    { TEXT("040B"), TEXT("fi"),    IDS_RFC1766_LCID040B },
    { TEXT("040C"), TEXT("fr"),    IDS_RFC1766_LCID040C },
    { TEXT("080C"), TEXT("fr-be"), IDS_RFC1766_LCID080C },
    { TEXT("0C0C"), TEXT("fr-ca"), IDS_RFC1766_LCID0C0C },
    { TEXT("100C"), TEXT("fr-ch"), IDS_RFC1766_LCID100C },
    { TEXT("140C"), TEXT("fr-lu"), IDS_RFC1766_LCID140C },
    { TEXT("043C"), TEXT("gd"),    IDS_RFC1766_LCID043C },
    { TEXT("0407"), TEXT("de"),    IDS_RFC1766_LCID0407 },
    { TEXT("0807"), TEXT("de-ch"), IDS_RFC1766_LCID0807 },
    { TEXT("0C07"), TEXT("de-at"), IDS_RFC1766_LCID0C07 },
    { TEXT("1007"), TEXT("de-lu"), IDS_RFC1766_LCID1007 },
    { TEXT("1407"), TEXT("de-li"), IDS_RFC1766_LCID1407 },
    { TEXT("0408"), TEXT("el"),    IDS_RFC1766_LCID0408 },
    { TEXT("040D"), TEXT("he"),    IDS_RFC1766_LCID040D },
    { TEXT("0439"), TEXT("hi"),    IDS_RFC1766_LCID0439 },
    { TEXT("040E"), TEXT("hu"),    IDS_RFC1766_LCID040E },
    { TEXT("040F"), TEXT("is"),    IDS_RFC1766_LCID040F },
    { TEXT("0421"), TEXT("in"),    IDS_RFC1766_LCID0421 },
    { TEXT("0410"), TEXT("it"),    IDS_RFC1766_LCID0410 },
    { TEXT("0810"), TEXT("it-ch"), IDS_RFC1766_LCID0810 },
    { TEXT("0411"), TEXT("ja"),    IDS_RFC1766_LCID0411 },
    { TEXT("0412"), TEXT("ko"),    IDS_RFC1766_LCID0412 },
    { TEXT("0426"), TEXT("lv"),    IDS_RFC1766_LCID0426 },
    { TEXT("0427"), TEXT("lt"),    IDS_RFC1766_LCID0427 },
    { TEXT("042F"), TEXT("mk"),    IDS_RFC1766_LCID042F },
    { TEXT("043E"), TEXT("ms"),    IDS_RFC1766_LCID043E },
    { TEXT("043A"), TEXT("mt"),    IDS_RFC1766_LCID043A },
    { TEXT("0414"), TEXT("no"),    IDS_RFC1766_LCID0414 },
    { TEXT("0814"), TEXT("no"),    IDS_RFC1766_LCID0814 },
    { TEXT("0415"), TEXT("pl"),    IDS_RFC1766_LCID0415 },
    { TEXT("0416"), TEXT("pt-br"), IDS_RFC1766_LCID0416 },
    { TEXT("0816"), TEXT("pt"),    IDS_RFC1766_LCID0816 },
    { TEXT("0417"), TEXT("rm"),    IDS_RFC1766_LCID0417 },
    { TEXT("0418"), TEXT("ro"),    IDS_RFC1766_LCID0418 },
    { TEXT("0818"), TEXT("ro-mo"), IDS_RFC1766_LCID0818 },
    { TEXT("0419"), TEXT("ru"),    IDS_RFC1766_LCID0419 },
    { TEXT("0819"), TEXT("ru-mo"), IDS_RFC1766_LCID0819 },
    { TEXT("0C1A"), TEXT("sr"),    IDS_RFC1766_LCID0C1A },
    { TEXT("081A"), TEXT("sr"),    IDS_RFC1766_LCID081A },
    { TEXT("041B"), TEXT("sk"),    IDS_RFC1766_LCID041B },
    { TEXT("0424"), TEXT("sl"),    IDS_RFC1766_LCID0424 },
    { TEXT("042E"), TEXT("sb"),    IDS_RFC1766_LCID042E },
    { TEXT("040A"), TEXT("es"),    IDS_RFC1766_LCID040A },
    { TEXT("080A"), TEXT("es-mx"), IDS_RFC1766_LCID080A },
    { TEXT("0C0A"), TEXT("es"),    IDS_RFC1766_LCID0C0A },
    { TEXT("100A"), TEXT("es-gt"), IDS_RFC1766_LCID100A },
    { TEXT("140A"), TEXT("es-cr"), IDS_RFC1766_LCID140A },
    { TEXT("180A"), TEXT("es-pa"), IDS_RFC1766_LCID180A },
    { TEXT("1C0A"), TEXT("es-do"), IDS_RFC1766_LCID1C0A },
    { TEXT("200A"), TEXT("es-ve"), IDS_RFC1766_LCID200A },
    { TEXT("240A"), TEXT("es-co"), IDS_RFC1766_LCID240A },
    { TEXT("280A"), TEXT("es-pe"), IDS_RFC1766_LCID280A },
    { TEXT("2C0A"), TEXT("es-ar"), IDS_RFC1766_LCID2C0A },
    { TEXT("300A"), TEXT("es-ec"), IDS_RFC1766_LCID300A },
    { TEXT("340A"), TEXT("es-cl"), IDS_RFC1766_LCID340A },
    { TEXT("380A"), TEXT("es-uy"), IDS_RFC1766_LCID380A },
    { TEXT("3C0A"), TEXT("es-py"), IDS_RFC1766_LCID3C0A },
    { TEXT("400A"), TEXT("es-bo"), IDS_RFC1766_LCID400A },
    { TEXT("440A"), TEXT("es-sv"), IDS_RFC1766_LCID440A },
    { TEXT("480A"), TEXT("es-hn"), IDS_RFC1766_LCID480A },
    { TEXT("4C0A"), TEXT("es-ni"), IDS_RFC1766_LCID4C0A },
    { TEXT("500A"), TEXT("es-pr"), IDS_RFC1766_LCID500A },
    { TEXT("0430"), TEXT("sx"),    IDS_RFC1766_LCID0430 },
    { TEXT("041D"), TEXT("sv"),    IDS_RFC1766_LCID041D },
    { TEXT("081D"), TEXT("sv-fi"), IDS_RFC1766_LCID081D },
    { TEXT("041E"), TEXT("th"),    IDS_RFC1766_LCID041E },
    { TEXT("0431"), TEXT("ts"),    IDS_RFC1766_LCID0431 },
    { TEXT("0432"), TEXT("tn"),    IDS_RFC1766_LCID0432 },
    { TEXT("041F"), TEXT("tr"),    IDS_RFC1766_LCID041F },
    { TEXT("0422"), TEXT("uk"),    IDS_RFC1766_LCID0422 },
    { TEXT("0420"), TEXT("ur"),    IDS_RFC1766_LCID0420 },
    { TEXT("042A"), TEXT("vi"),    IDS_RFC1766_LCID042A },
    { TEXT("0434"), TEXT("xh"),    IDS_RFC1766_LCID0434 },
    { TEXT("043D"), TEXT("ji"),    IDS_RFC1766_LCID043D },
    { TEXT("0435"), TEXT("zu"),    IDS_RFC1766_LCID0435 },
    { NULL,         NULL,          0                    }
};

//
//  Charset registry data at MIME\Database
//
typedef struct tagREGCHARSET
{
    LPCTSTR szCharset;
    DWORD dwCodePage;
    DWORD dwInternetEncoding;
    LPCTSTR szAliasForCharset;
    DWORD dwCharsetMask;
}   REGCHARSET, *LPREGCHARSET;

const REGCHARSET regCharset[] =
{
    { TEXT("_autodetect_all"), 50001, 50001, NULL, 0x0 },
    { TEXT("_autodetect"), 50932, 50932, NULL, 0x0 },
    { TEXT("_autodetect_kr"), 50949, 50949, NULL, 0x0 },
    { TEXT("_iso-2022-jp$ESC"), 932, 50221, NULL, 0x0 },
    { TEXT("_iso-2022-jp$SIO"), 932, 50222, NULL, 0x3 },
    { TEXT("Big5"), 950, 950, NULL, 0x0 },
    { TEXT("ks_c_5601-1987"), 949, 949, NULL, 0x3 },
    { TEXT("euc-kr"), 949, 949, NULL, 0x3 },
    { TEXT("GB2312"), 936, 936, NULL, 0x0 },
    { TEXT("hz-gb-2312"), 936, 52936, NULL, 0x0 },
    { TEXT("ibm852"), 852, 852, NULL, 0x3 },
    { TEXT("ibm866"), 866, 866, NULL, 0x3 },
    { TEXT("iso-2022-jp"), 932, 50220, NULL,0x3 },
    { TEXT("iso-2022-kr"), 949, 50225, NULL, 0x3 },
    { TEXT("iso-8859-1"), 1252, 1252, NULL, 0x0 },
    { TEXT("iso-8859-2"), 1250, 28592, NULL, 0x0 },
    { TEXT("iso-8859-3"), 1254, 28593, NULL, 0x0 },
    { TEXT("iso-8859-4"), 1257, 28594, NULL, 0x0 },
    { TEXT("iso-8859-5"), 1251, 28595, NULL, 0x0 },
    { TEXT("iso-8859-6"), 1256, 28596, NULL, 0x0 },
    { TEXT("iso-8859-7"), 1253, 28597, NULL, 0x0 },
    { TEXT("iso-8859-8"), 1255, 28598, NULL, 0x2 },
    { TEXT("iso-8859-8-i"), 1255, 38598, NULL, 0x0 },
    { TEXT("iso-8859-9"), 1254, 1254, NULL, 0x0 },
    { TEXT("iso-8859-11"), 0, 0, TEXT("windows-874"), 0x0 },
    { TEXT("koi8-r"), 1251, 20866, NULL, 0x0 },
    { TEXT("koi8-ru"), 1251, 21866, NULL, 0x2 },
    { TEXT("shift_jis"), 932, 932, NULL, 0x0 },
    { TEXT("unicode-1-1-utf-7"), 0, 0, TEXT("utf-7"), 0x4 },
    { TEXT("unicode-1-1-utf-8"), 0, 0, TEXT("utf-8"), 0x0 },
    { TEXT("x-unicode-2-0-utf-7"), 0, 0, TEXT("utf-7"), 0x4 },
    { TEXT("x-unicode-2-0-utf-8"), 0, 0, TEXT("utf-8"), 0x4 },
    { TEXT("utf-7"), 1200, 65000, NULL, 0x1 },
    { TEXT("utf-8"), 1200, 65001, NULL, 0x1 },
    { TEXT("unicode"), 1200, 1200, NULL, 0x0 },
    { TEXT("unicodeFFFE"), 1200, 1201, NULL, 0x0 },
    { TEXT("windows-1250"), 1250, 1250, NULL, 0x0 },
    { TEXT("windows-1251"), 1251, 1251, NULL, 0x0 },
    { TEXT("windows-1252"), 1252, 1252, NULL, 0x0 },
    { TEXT("windows-1253"), 1253, 1253, NULL, 0x0 },
    { TEXT("windows-1255"), 1255, 1255, NULL, 0x0 },
    { TEXT("windows-1256"), 1256, 1256, NULL, 0x0 },
    { TEXT("windows-1257"), 1257, 1257, NULL, 0x0 },
    { TEXT("windows-1258"), 1258, 1258, NULL, 0x0 },
    { TEXT("windows-874"), 874, 874, NULL, 0x0 },
    { TEXT("x-user-defined"), 50000, 50000, NULL, 0x0 },
    { TEXT("x-ansi"), 0, 0, TEXT("windows-1252"), 0x0 },
    { TEXT("euc-jp"), 932, 51932, NULL, 0x0 },
    { TEXT("x-euc-jp"), 0, 0, TEXT("euc-jp"), 0x0 },
    { TEXT("x-euc"), 0, 0, TEXT("euc-jp"), 0x0 },
    { TEXT("x-ms-cp932"), 0, 0, TEXT("shift_jis"), 0x0 },
    { TEXT("x-sjis"), 0, 0, TEXT("shift_jis"), 0x0 },
    { TEXT("ANSI_X3.4-1968"), 0, 0, TEXT("iso-8859-1"), 0x4 },
    { TEXT("ANSI_X3.4-1986"), 0, 0, TEXT("iso-8859-1"), 0x4 },
    { TEXT("ascii"), 0, 0, TEXT("iso-8859-1"), 0x0 },
    { TEXT("chinese"), 0, 0, TEXT("gb2312"), 0x0 },
    { TEXT("CN-GB"), 0, 0, TEXT("gb2312"), 0x0 },
    { TEXT("cp866"), 0, 0, TEXT("ibm866"), 0x0 },
    { TEXT("cp852"), 0, 0, TEXT("ibm852"), 0x0 },
    { TEXT("cp367"), 0, 0, TEXT("iso-8859-1"), 0x4 },
    { TEXT("cp819"), 0, 0, TEXT("iso-8859-1"), 0x4 },
    { TEXT("csASCII"), 0, 0, TEXT("iso-8859-1"), 0x0 },
    { TEXT("csbig5"), 0, 0, TEXT("big5"), 0x0 },
    { TEXT("csEUCPkdFmtJapanese"), 0, 0, TEXT("euc-jp"), 0x0 },
    { TEXT("csGB2312"), 0, 0, TEXT("gb2312"), 0x0 },
    { TEXT("csISO2022KR"), 0, 0, TEXT("iso-2022-kr"), 0x0 },
    { TEXT("csISO58GB231280"), 0, 0, TEXT("gb2312"), 0x0 },
    { TEXT("csISOLatin2"), 0, 0, TEXT("iso-8859-2"), 0x0 },
    { TEXT("csISOLatin4"), 0, 0, TEXT("iso-8859-4"), 0x0 },
    { TEXT("csISOLatin5"), 0, 0, TEXT("iso-8859-9"), 0x4 },
    { TEXT("csISOLatinCyrillic"), 0, 0, TEXT("iso-8859-5"), 0x0 },
    { TEXT("csISOLatinGreek"), 0, 0, TEXT("iso-8859-7"), 0x0 },
    { TEXT("csISOLatinHebrew"), 0, 0, TEXT("iso-8859-8"), 0x0 },
    { TEXT("csKSC56011987"), 0, 0, TEXT("ks_c_5601-1987"), 0x0 },
    { TEXT("csShiftJIS"), 0, 0, TEXT("shift_jis"), 0x0 },
    { TEXT("csUnicode11UTF7"), 0, 0, TEXT("utf-7"), 0x0 },
    { TEXT("csWindows31J"), 0, 0, TEXT("shift_jis"), 0x0 },
    { TEXT("cyrillic"), 0, 0, TEXT("iso-8859-5"), 0x0 },
    { TEXT("ECMA-118"), 0, 0, TEXT("iso-8859-7"), 0x0 },
    { TEXT("ELOT_928"), 0, 0, TEXT("iso-8859-7"), 0x0 },
    { TEXT("greek"), 0, 0, TEXT("iso-8859-7"), 0x0 },
    { TEXT("greek8"), 0, 0, TEXT("iso-8859-7"), 0x0 },
    { TEXT("hebrew"), 0, 0, TEXT("iso-8859-8"), 0x0 },
    { TEXT("IBM367"), 0, 0, TEXT("iso-8859-1"), 0x0 },
    { TEXT("ibm819"), 0, 0, TEXT("iso-8859-1"), 0x0 },
    { TEXT("ISO_646.irv:1991"), 0, 0, TEXT("iso-8859-1"), 0x4 },
    { TEXT("iso_8859-1"), 0, 0, TEXT("iso-8859-1"), 0x4 },
    { TEXT("iso_8859-1:1987"), 0, 0, TEXT("iso-8859-1"), 0x4 },
    { TEXT("iso_8859-2"), 0, 0, TEXT("iso-8859-2"), 0x0 },
    { TEXT("iso_8859-2:1987"), 0, 0, TEXT("iso-8859-2"), 0x0 },
    { TEXT("ISO_8859-4"), 0, 0, TEXT("iso-8859-4"), 0x0 },
    { TEXT("ISO_8859-5"), 0, 0, TEXT("iso-8859-5"), 0x0 },
    { TEXT("ISO_8859-7"), 0, 0, TEXT("iso-8859-7"), 0x0 },
    { TEXT("ISO_8859-8"), 0, 0, TEXT("iso-8859-8"), 0x0 },
    { TEXT("ISO_8859-9"), 0, 0, TEXT("iso-8859-9"), 0x0 },
    { TEXT("ISO646-US"), 0, 0, TEXT("iso-8859-1"), 0x4 },
    { TEXT("iso8859-1"), 0, 0, TEXT("iso-8859-1"), 0x4 },
    { TEXT("iso-ir-100"), 0, 0, TEXT("iso-8859-1"), 0x4 },
    { TEXT("iso-ir-101"), 0, 0, TEXT("iso-8859-2"), 0x0 },
    { TEXT("iso-ir-110"), 0, 0, TEXT("iso-8859-4"), 0x0 },
    { TEXT("iso-ir-126"), 0, 0, TEXT("iso-8859-7"), 0x0 },
    { TEXT("iso-ir-138"), 0, 0, TEXT("iso-8859-8"), 0x0 },
    { TEXT("iso-ir-144"), 0, 0, TEXT("iso-8859-5"), 0x0 },
    { TEXT("iso-ir-148"), 0, 0, TEXT("iso-8859-9"), 0x0 },
    { TEXT("iso-ir-58"), 0, 0, TEXT("gb2312"), 0x0 },
    { TEXT("iso-ir-6"), 0, 0, TEXT("iso-8859-1"), 0x4 },
    { TEXT("korean"), 0, 0, TEXT("ks_c_5601-1987"), 0x0 },
    { TEXT("ks_c_5601"), 0, 0, TEXT("ks_c_5601-1987"), 0x0 },
    { TEXT("l2"), 0, 0, TEXT("iso-8859-2"), 0x0 },
    { TEXT("l4"), 0, 0, TEXT("iso-8859-4"), 0x0 },
    { TEXT("l5"), 0, 0, TEXT("iso-8859-9"), 0x4 },
    { TEXT("latin1"), 0, 0, TEXT("iso-8859-1"), 0x4 },
    { TEXT("latin2"), 0, 0, TEXT("iso-8859-2"), 0x0 },
    { TEXT("latin4"), 0, 0, TEXT("iso-8859-4"), 0x0 },
    { TEXT("latin5"), 0, 0, TEXT("iso-8859-9"), 0x4 },
    { TEXT("ms_Kanji"), 0, 0, TEXT("shift_jis"), 0x0 },
    { TEXT("shift-jis"), 0, 0, TEXT("shift_jis"), 0x0 },
    { TEXT("unicode-2-0-utf-8"), 0, 0, TEXT("utf-8"), 0x0 },
    { TEXT("us-ascii"), 0, 0, TEXT("iso-8859-1"), 0x0 },
    { TEXT("us"), 0, 0, TEXT("iso-8859-1"), 0x0 },
    { TEXT("x-cp1250"), 0, 0, TEXT("Windows-1250"), 0x0 },
    { TEXT("x-cp1251"), 0, 0, TEXT("Windows-1251"), 0x0 },
    { TEXT("x-x-big5"), 0, 0, TEXT("big5"), 0x0 },
    { TEXT("csISO2022JP"), 0, 0, TEXT("_iso-2022-jp$ESC"), 0x4 },
    { TEXT("csKOI8R"), 0, 0, TEXT("koi8-r"), 0x0 },
    { TEXT("Extended_UNIX_Code_Packed_Format_for_Japanese"), 0, 0, TEXT("euc-jp"), 0x0 },
    { TEXT("GB_2312-80"), 0, 0, TEXT("gb2312"), 0x0 },
    { TEXT("GBK"), 0, 0, TEXT("gb2312"), 0x0 },
    { TEXT("ISO_8859-4:1988"), 0, 0, TEXT("iso-8859-4"), 0x0 },
    { TEXT("ISO_8859-5:1988"), 0, 0, TEXT("iso-8859-5"), 0x0 },
    { TEXT("ISO_8859-7:1987"), 0, 0, TEXT("iso-8859-7"), 0x0 },
    { TEXT("ISO_8859-8:1988"), 0, 0, TEXT("iso-8859-8"), 0x0 },
    { TEXT("ISO_8859-9:1989"), 0, 0, TEXT("iso-8859-9"), 0x0 },
    { TEXT("iso8859-2"), 0, 0, TEXT("iso-8859-2"), 0x0 },
    { TEXT("koi"), 0, 0, TEXT("koi8-r"), 0x0 },
    { TEXT("Windows-1254"), 0, 0, TEXT("iso-8859-9"), 0x0 },
    { TEXT("DOS-720"), 1256, 720, NULL, 0x0 },
    { TEXT("DOS-862"), 1255, 862, NULL, 0x0 },
    { TEXT("DOS-874"), 874, 874, NULL, 0x0 },
    { TEXT("ASMO-708"), 1256, 708, NULL, 0x0 },
    { TEXT("csEUCKR"), 0, 0, TEXT("ks_c_5601-1987"), 0x0 },
    { TEXT("csISOLatin1"), 0, 0, TEXT("windows-1252"), 0x0 },
    { TEXT("iso-ir-111"), 0, 0, TEXT("iso-8859-4"), 0x0 },
    { TEXT("iso-ir-149"), 0, 0, TEXT("ks_c_5601-1987"), 0x0 },
    { TEXT("KSC_5601"), 0, 0, TEXT("ks_c_5601-1987"), 0x0 },
    { TEXT("KSC5601"), 0, 0, TEXT("ks_c_5601-1987"), 0x0 },
    { TEXT("ks_c_5601-1989"), 0, 0, TEXT("ks_c_5601-1987"), 0x0 },
    { TEXT("l1"), 0, 0, TEXT("windows-1252"), 0x0 },
    { TEXT("cp1256"), 0, 0, TEXT("windows-1256"), 0x0 },
    { TEXT("logical"), 0, 0, TEXT("windows-1255"), 0x0 },
    { TEXT("csISOLatinArabic"), 0, 0, TEXT("iso-8859-6"), 0x0 },
    { TEXT("ECMA-114"), 0, 0, TEXT("iso-8859-6"), 0x0 },
    { TEXT("visual"), 0, 0, TEXT("iso-8859-8"), 0x0 },
    { TEXT("ISO-8859-8 Visual"), 0, 0, TEXT("iso-8859-8"), 0x0 },
    { TEXT("ISO_8859-6"), 0, 0, TEXT("iso-8859-6"), 0x0 },
    { TEXT("iso-ir-127"), 0, 0, TEXT("iso-8859-6"), 0x0 },
    { TEXT("ISO_8859-6:1987"), 0, 0, TEXT("iso-8859-6"), 0x0 },
    { TEXT("arabic"), 0, 0, TEXT("iso-8859-6"), 0x0 },
    { NULL, 0, 0, NULL, 0 }    
};

//
//  Codepage registry data at MIME\Database
//
typedef struct tagREGCODEPAGE
{
    LPCTSTR szCodePage;
    LPCTSTR szHeaderCharset;
    LPCTSTR szBodyCharset;
    LPCTSTR szWebCharset;
    UINT uidFixedWidthFont;
    UINT uidProportionalFont;
    UINT uidDescription;
    DWORD dwFamily;
    DWORD dwLevel;
    DWORD dwEncoding;
    DWORD dwCodePageMask;
}   REGCODEPAGE, *LPREGCODEPAGE;

const REGCODEPAGE regCodePage[] =
{
    { TEXT("1200"), NULL, TEXT("unicode"), NULL, IDS_FONT_WESTERN_FIXED, IDS_FONT_WESTERN_PROP, IDS_DESC_1200, 0, 0x00000204, 0x00000101, 0xA0 },
    { TEXT("1201"), NULL, TEXT("unicodeFFFE"), NULL, IDS_FONT_WESTERN_FIXED, IDS_FONT_WESTERN_PROP, IDS_DESC_1201, 1200, 0x00000000, 0x00000101,0xE0 },
    { TEXT("1250"), NULL, TEXT("iso-8859-2"), TEXT("windows-1250"), IDS_FONT_WESTERN_FIXED, IDS_FONT_WESTERN_PROP, IDS_DESC_1250, 0, 0x00000303, 0x00000202, 0xA0 },
    { TEXT("1251"), NULL, TEXT("koi8-r"), TEXT("windows-1251"), IDS_FONT_WESTERN_FIXED, IDS_FONT_WESTERN_PROP, IDS_DESC_1251, 0, 0x00000303, 0x00000202, 0xA0 },
    { TEXT("1252"), NULL, TEXT("iso-8859-1"), TEXT("iso-8859-1"), IDS_FONT_WESTERN_FIXED, IDS_FONT_WESTERN_PROP, IDS_DESC_1252, 0, 0x00000707, 0x00000000, 0xA4 },
    { TEXT("1253"), NULL, TEXT("iso-8859-7"), TEXT("windows-1253"), IDS_FONT_WESTERN_FIXED, IDS_FONT_WESTERN_PROP, IDS_DESC_1253, 0, 0x00000303, 0x00000101, 0x100A6 },
    { TEXT("1254"), NULL, TEXT("iso-8859-9"), TEXT("iso-8859-9"), IDS_FONT_WESTERN_FIXED, IDS_FONT_WESTERN_PROP, IDS_DESC_1254, 0, 0x00000707, 0x00000202, 0xA0 },
    { TEXT("1255"), NULL, TEXT("iso-8859-8-i"), TEXT("windows-1255"), IDS_FONT_WESTERN_FIXED, IDS_FONT_WESTERN_PROP, IDS_DESC_1255, 0, 0x00000303, 0x00000101, 0xA2 },
    { TEXT("1256"), NULL, TEXT("iso-8859-6"), TEXT("windows-1256"), IDS_FONT_WESTERN_FIXED, IDS_FONT_WESTERN_PROP, IDS_DESC_1256, 0, 0x00000303, 0x00000101, 0xA0 },
    { TEXT("1257"), NULL, TEXT("iso-8859-4"), TEXT("windows-1257"), IDS_FONT_WESTERN_FIXED, IDS_FONT_WESTERN_PROP, IDS_DESC_1257, 0, 0x00000707, 0x00000202, 0xA0 },
    { TEXT("1258"), NULL, TEXT("windows-1258"), TEXT("windows-1258"), IDS_FONT_WESTERN_FIXED, IDS_FONT_WESTERN_PROP, IDS_DESC_1258, 0, 0x00000101, 0x00000000, 0xA0 },
    { TEXT("20866"), NULL, TEXT("koi8-r"), NULL, 0, 0, IDS_DESC_20866, 1251, 0x00000707, 0x00000101, 0xA0 },
    { TEXT("21866"), NULL, TEXT("koi8-ru"), NULL, 0, 0, IDS_DESC_21866, 1251, 0x00000707, 0x00000101, 0xA0 },
    { TEXT("28592"), NULL, TEXT("iso-8859-2"), NULL, 0, 0, IDS_DESC_28592, 1250, 0x00000707, 0x00000000, 0xA0 },
    { TEXT("28593"), NULL, TEXT("iso-8859-3"), NULL, 0, 0, IDS_DESC_28593, 1254, 0x00000701, 0x00000000, 0xA0 },
    { TEXT("28594"), NULL, TEXT("iso-8859-4"), NULL, 0, 0, IDS_DESC_28594, 1257, 0x00000301, 0x00000000, 0xA0 },
    { TEXT("28595"), NULL, TEXT("iso-8859-5"), NULL, 0, 0, IDS_DESC_28595, 1251, 0x00000707, 0x00000101, 0xA0 },
    { TEXT("28596"), NULL, TEXT("iso-8859-6"), NULL, 0, 0, IDS_DESC_28596, 1256, 0x00000707, 0x00000000, 0xA0 },
    { TEXT("28597"), NULL, TEXT("iso-8859-7"), NULL, 0, 0, IDS_DESC_28597, 1253, 0x00000707, 0x00000101, 0xA2 },
    { TEXT("50000"), NULL, TEXT("x-user-defined"), NULL, IDS_FONT_WESTERN_FIXED, IDS_FONT_UNICODE_PROP, IDS_DESC_50000, 0, 0x00000303, 0x00000000, 0xA0 },
    { TEXT("50220"), NULL, TEXT("iso-2022-jp"), NULL, 0, 0, IDS_DESC_50220, 932, 0x00000101, 0x00000000, 0xE2 },
    { TEXT("50221"), NULL, TEXT("_iso-2022-jp$ESC"), TEXT("csISO2022JP"), 0, 0, IDS_DESC_50221, 932, 0x00000301, 0x00000000, 0xE6 },
    { TEXT("50222"), NULL, TEXT("_iso-2022-jp$SIO"), TEXT("iso-2022-jp"), 0, 0, IDS_DESC_50222, 932, 0x00000101, 0x00000000, 0xE6 },
    { TEXT("50225"), NULL, TEXT("iso-2022-kr"), TEXT("iso-2022-kr"), 0, 0, IDS_DESC_50225, 949, 0x00000101, 0x00000000, 0xA0 },
    { TEXT("50001"), NULL, TEXT("_autodetect_all"), NULL, 0, 0, IDS_DESC_50001, 0, 0x00000007, 0x00000101, 0xA2 },
    { TEXT("50932"), NULL, TEXT("_autodetect"), NULL, 0, 0, IDS_DESC_50932, 932, 0x00000007, 0x00000101, 0xA2 },
    { TEXT("50949"), NULL, TEXT("_autodetect_kr"), NULL, 0, 0, IDS_DESC_50949, 949, 0x00000001, 0x00000101, 0xA0 },
    { TEXT("51932"), NULL, TEXT("euc-jp"), NULL, 0, 0, IDS_DESC_51932, 932, 0x00000707, 0x00000101, 0xA2 },
    { TEXT("51949"), NULL, TEXT("euc-kr"), NULL, 0, 0, IDS_DESC_51949, 949, 0x00000000, 0x00000101, 0xE2 },
    { TEXT("52936"), NULL, TEXT("hz-gb-2312"), NULL, 0, 0, IDS_DESC_52936, 936, 0x00000303, 0x00000000, 0xA2 },
    { TEXT("65000"), NULL, TEXT("utf-7"), NULL, 0, 0, IDS_DESC_65000, 1200, 0x00000101, 0x00000000, 0xA0 },
    { TEXT("65001"), NULL, TEXT("utf-8"), NULL, 0, 0, IDS_DESC_65001, 1200, 0x00000303, 0x00000000, 0xA0 },
    { TEXT("852"), NULL, TEXT("ibm852"), NULL, 0, 0, IDS_DESC_852, 1250, 0x00000202, 0x00000000, 0xE0 },
    { TEXT("866"), NULL, TEXT("cp866"), NULL, 0, 0, IDS_DESC_866, 1251, 0x00000202, 0x00000000, 0xA0 },
    { TEXT("874"), NULL, TEXT("windows-874"), TEXT("windows-874"), IDS_FONT_THAI_FIXED, IDS_FONT_THAI_PROP, IDS_DESC_874, 0, 0x00000707, 0x00000101, 0x1B8 },
    { TEXT("932"), NULL, TEXT("iso-2022-jp"), TEXT("shift_jis"), IDS_FONT_JAPANESE_FIXED, IDS_FONT_JAPANESE_PROP, IDS_DESC_932, 0, 0x00000707, 0x00000101, 0xA2 },
    { TEXT("936"), NULL, TEXT("gb2312"), NULL, IDS_FONT_CHINESE_FIXED, IDS_FONT_CHINESE_PROP, IDS_DESC_936, 0, 0x00000707, 0x00000000, 0xA0 },
    { TEXT("949"), TEXT("euc-kr"), TEXT("euc-kr"), TEXT("ks_c_5601-1987"), IDS_FONT_KOREAN_FIXED, IDS_FONT_KOREAN_PROP, IDS_DESC_949, 0, 0x00000707, 0x00000101, 0xA7 },
    { TEXT("950"), NULL, TEXT("big5"), NULL, IDS_FONT_TAIWAN_FIXED, IDS_FONT_TAIWAN_PROP, IDS_DESC_950, 0, 0x00000707, 0x00000101, 0xA0 },
    { TEXT("28598"), NULL, TEXT("iso-8859-8"), NULL, 0, 0, IDS_DESC_28598, 1255, 0x00000707, 0x00000101, 0xA0 },
    { TEXT("38598"), NULL, TEXT("iso-8859-8-i"), NULL, IDS_FONT_HEBREW_FIXED, IDS_FONT_HEBREW_PROP, IDS_DESC_38598, 1255, 0x00000707, 0x00000101, 0xB8 },
    { TEXT("708"), NULL, TEXT("ASMO-708"), NULL, 0, 0, IDS_DESC_708, 1256, 0x00000707, 0, 0xA0 },
    { TEXT("720"), NULL, TEXT("DOS-720"), NULL, 0, 0, IDS_DESC_720, 1256, 0x00000707, 0, 0xA0 },
    { TEXT("862"), NULL, TEXT("DOS-862"), NULL, 0, 0, IDS_DESC_862, 1255, 0x00000707, 0, 0xA0 },
    { NULL, NULL, NULL, NULL, 0, 0, 0, 0, 0x00000000, 0x00000000, 0x0  }        
};


BOOL MimeDatabaseInfo(void)
{
    HKEY hKey = NULL, hKeySub = NULL;
    TCHAR szKey[32], szValue[256];
    int i;
    BOOL bNewKey, bOverWrite;
    DWORD dwAction;
    BOOL bRet = TRUE;

    // MIME\Database\CodePage
    wsprintf(szKey, TEXT("%s\\%s"), szMIMEDatabase, szCodepage);
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CLASSES_ROOT, szKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwAction))
    {
        ASSERT(NULL != hKey);
        i = 0;
        bNewKey = (dwAction == REG_CREATED_NEW_KEY);
        while (regCodePage[i].szCodePage)
        {
            if (ERROR_SUCCESS == RegCreateKeyEx(hKey, regCodePage[i].szCodePage, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeySub, &dwAction))
            {
                ASSERT(NULL != hKeySub);
                bOverWrite = (bNewKey || dwAction == REG_CREATED_NEW_KEY);
                if (regCodePage[i].szHeaderCharset)
                {
                    if (ERROR_SUCCESS != PrivRegSetValueEx(hKeySub, szHeaderCharset, REG_SZ, (LPBYTE)regCodePage[i].szHeaderCharset, (lstrlen(regCodePage[i].szHeaderCharset) + 1) * sizeof(TCHAR), bOverWrite || (regCodePage[i].dwCodePageMask & BIT_HEADER_CHARSET)))
                        bRet = FALSE;
                }
                else 
                {
                    if (regCodePage[i].dwCodePageMask & BIT_DEL_HEADER_CHARSET)
                    {
                        RegDeleteValue(hKeySub, szHeaderCharset);
                    }
                }
                if (regCodePage[i].szBodyCharset)
                {
                    if (ERROR_SUCCESS != PrivRegSetValueEx(hKeySub, szBodyCharset, REG_SZ, (LPBYTE)regCodePage[i].szBodyCharset, (lstrlen(regCodePage[i].szBodyCharset) + 1) * sizeof(TCHAR), bOverWrite || (regCodePage[i].dwCodePageMask & BIT_BODY_CHARSET)))
                        bRet = FALSE;
                }
                else 
                {
                    if (regCodePage[i].dwCodePageMask & BIT_DEL_BODY_CHARSET)
                    {
                        RegDeleteValue(hKeySub, szBodyCharset);
                    }
                }
                if (regCodePage[i].szWebCharset)
                {
                    if (ERROR_SUCCESS != PrivRegSetValueEx(hKeySub, szWebCharset, REG_SZ, (LPBYTE)regCodePage[i].szWebCharset, (lstrlen(regCodePage[i].szWebCharset) + 1) * sizeof(TCHAR), bOverWrite || (regCodePage[i].dwCodePageMask & BIT_WEB_CHARSET)))
                        bRet = FALSE;
                }
                else 
                {
                    if (regCodePage[i].dwCodePageMask & BIT_DEL_WEB_CHARSET)
                    {
                        RegDeleteValue(hKeySub, szWebCharset);
                    }
                }
                if (regCodePage[i].uidFixedWidthFont)
                {
                    LoadString(g_hInst, regCodePage[i].uidFixedWidthFont, szValue, sizeof(szValue));
                    if (ERROR_SUCCESS != PrivRegSetValueEx(hKeySub, szFixedWidthFont, REG_SZ, (LPBYTE)szValue, (lstrlen(szValue) + 1) * sizeof(TCHAR), bOverWrite || (regCodePage[i].dwCodePageMask & BIT_WEB_FIXED_WIDTH_FONT)))
                        bRet = FALSE;
                }
                else 
                {
                    if (regCodePage[i].dwCodePageMask & BIT_DEL_WEB_FIXED_WIDTH_FONT)
                    {
                        RegDeleteValue(hKeySub, szFixedWidthFont);
                    }
                }
                if (regCodePage[i].uidProportionalFont)
                {
                    LoadString(g_hInst, regCodePage[i].uidProportionalFont, szValue, sizeof(szValue));
                    if (ERROR_SUCCESS != PrivRegSetValueEx(hKeySub, szProportionalFont, REG_SZ, (LPBYTE)szValue, (lstrlen(szValue) + 1) * sizeof(TCHAR), bOverWrite || (regCodePage[i].dwCodePageMask & BIT_PROPORTIONAL_FONT)))
                        bRet = FALSE;
                }
                else 
                {
                    if (regCodePage[i].dwCodePageMask & BIT_DEL_PROPORTIONAL_FONT)
                    {
                        RegDeleteValue(hKeySub, szProportionalFont);
                    }
                }
                if (regCodePage[i].uidDescription)
                {
                    LoadString(g_hInst, regCodePage[i].uidDescription, szValue, sizeof(szValue));
                    if (ERROR_SUCCESS != PrivRegSetValueEx(hKeySub, szDescription, REG_SZ, (LPBYTE)szValue, (lstrlen(szValue) + 1) * sizeof(TCHAR), bOverWrite || (regCodePage[i].dwCodePageMask & BIT_DESCRIPTION)))
                        bRet = FALSE;
                }
                else 
                {
                    if (regCodePage[i].dwCodePageMask & BIT_DEL_DESCRIPTION)
                    {
                        RegDeleteValue(hKeySub, szDescription);
                    }
                }
                if (regCodePage[i].dwFamily)
                {
                    if (ERROR_SUCCESS != PrivRegSetValueEx(hKeySub, szFamily, REG_DWORD, (LPBYTE)&regCodePage[i].dwFamily, sizeof(DWORD), bOverWrite || (regCodePage[i].dwCodePageMask & BIT_FAMILY)))
                        bRet = FALSE;
                }
                else 
                {
                    if (regCodePage[i].dwCodePageMask & BIT_DEL_FAMILY)
                    {
                        RegDeleteValue(hKeySub, szFamily);
                    }
                }
                if (regCodePage[i].dwLevel)
                {
                    if (ERROR_SUCCESS != PrivRegSetValueEx(hKeySub, szLevel, REG_BINARY, (LPBYTE)&regCodePage[i].dwLevel, sizeof(DWORD), bOverWrite || (regCodePage[i].dwCodePageMask & BIT_LEVEL)))
                        bRet = FALSE;
                }
                else 
                {
                    if (regCodePage[i].dwCodePageMask & BIT_DEL_LEVEL)
                    {
                        RegDeleteValue(hKeySub, szLevel);
                    }
                }
                if (regCodePage[i].dwEncoding)
                {
                    if (ERROR_SUCCESS != PrivRegSetValueEx(hKeySub, szEncoding, REG_BINARY, (LPBYTE)&regCodePage[i].dwEncoding, sizeof(DWORD), bOverWrite || (regCodePage[i].dwCodePageMask & BIT_ENCODING)))
                        bRet = FALSE;
                }
                else 
                {
                    if (regCodePage[i].dwCodePageMask & BIT_DEL_ENCODING)
                    {
                        RegDeleteValue(hKeySub, szEncoding);
                    }
                }
                RegCloseKey(hKeySub);
                hKeySub = NULL;
            }
            else
                bRet = FALSE;
            i++;
        }
        RegCloseKey(hKey);
        hKey = NULL;
    }
    else
        bRet = FALSE;

    // MIME\Database\Charset
    wsprintf(szKey, TEXT("%s\\%s"), szMIMEDatabase, szCharset);
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CLASSES_ROOT, szKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwAction))
    {
        ASSERT(NULL != hKey);
        i = 0;
        bNewKey = (dwAction == REG_CREATED_NEW_KEY);
        while (regCharset[i].szCharset)
        {
            if (ERROR_SUCCESS == RegCreateKeyEx(hKey, regCharset[i].szCharset, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeySub, &dwAction))
            {
                ASSERT(NULL != hKeySub);
                bOverWrite = (bNewKey || dwAction == REG_CREATED_NEW_KEY);
                if (regCharset[i].szAliasForCharset)
                {
                    if (ERROR_SUCCESS != PrivRegSetValueEx(hKeySub, szAliasForCharset, REG_SZ, (LPBYTE)regCharset[i].szAliasForCharset, (lstrlen(regCharset[i].szAliasForCharset) + 1) * sizeof(TCHAR), bOverWrite || (regCharset[i].dwCharsetMask & BIT_ALIAS_FOR_CHARSET)))
                        bRet = FALSE;
                }
                else
                {
                    if (ERROR_SUCCESS != PrivRegSetValueEx(hKeySub, szCodepage, REG_DWORD, (LPBYTE)&regCharset[i].dwCodePage, sizeof(DWORD), bOverWrite || (regCharset[i].dwCharsetMask & BIT_CODEPAGE)))
                        bRet = FALSE;
                    if (ERROR_SUCCESS != PrivRegSetValueEx(hKeySub, szInternetEncoding, REG_DWORD, (LPBYTE)&regCharset[i].dwInternetEncoding, sizeof(DWORD), bOverWrite || (regCharset[i].dwCharsetMask & BIT_INTERNET_ENCODING)))
                        bRet = FALSE;
                }
                RegCloseKey(hKeySub);
                hKeySub = NULL;
            }
            else
                bRet = FALSE;
            i++;
        }
        RegCloseKey(hKey);
        hKey = NULL;
    }
    else
        bRet = FALSE;

    // MIME\Database\Rfc1766
    wsprintf(szKey, TEXT("%s\\%s"), szMIMEDatabase, szRfc1766);
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CLASSES_ROOT, szKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwAction))
    {
        ASSERT(NULL != hKey);
        i = 0;
        while (regRfc1766[i].szLCID)
        {
            TCHAR szBuf[256];

            LoadString(g_hInst, regRfc1766[i].uidLCID, szBuf, sizeof(szBuf));
            wsprintf(szValue, TEXT("%s;%s"), regRfc1766[i].szAcceptLang, szBuf);
            if (ERROR_SUCCESS != PrivRegSetValueEx(hKey, regRfc1766[i].szLCID, REG_SZ, (LPBYTE)szValue, (lstrlen(szValue) + 1) * sizeof(TCHAR), bOverWrite))
                bRet = FALSE;
            i++;
        }
        RegCloseKey(hKey);
        hKey = NULL;
    }
    else
        bRet = FALSE;

    return bRet;
}





























