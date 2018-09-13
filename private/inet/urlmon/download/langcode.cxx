/*=========================================================================*
 |     
 |                                                                         |
 |                Copyright 1995 by Microsoft Corporation                  |
 |                         KevinGj - January 1997                          |
 |                                                                         |
 |=========================================================================|
 |                  LangInfo.cpp : Locale/Language information             |
 *=========================================================================*/

#include <cdlpch.h>


//=======================================================================//

CLangInfo::CLangInfo():Active(0)
{

    static LANGPROP const langmatch[] =
    {   
//  Pseudo LocaleName[80](for reference only)       NLS LocaleID    ISO369 AcceptLanguage[5]   AcceptLangAbbr[6]
            "Afrikaans",                                0x0436,             "af",           "af",
            "Albanian",                                 0x041c,             "sq",           "sq",
            "Arabic(Saudi Arabia)",                     0x0401,             "ar-sa",        "arsa",
            "Arabic(Iraq)",                             0x0801,             "ar-iq",        "ariq",
            "Arabic(Egypt)",                            0x0C01,             "ar-eg",        "areg",
            "Arabic(Libya)",                            0x1001,             "ar-ly",        "arly",
            "Arabic(Algeria)",                          0x1401,             "ar-dz",        "ardz",
            "Arabic(Morocco)",                          0x1801,             "ar-ma",        "arma",
            "Arabic(Tunisia)",                          0x1C01,             "ar-tn",        "artn",
            "Arabic(Oman)",                             0x2001,             "ar-om",        "arom",
            "Arabic(Yemen)",                            0x2401,             "ar-ye",        "arye",
            "Arabic(Syria)",                            0x2801,             "ar-sy",        "arsy",
            "Arabic(Jordan)",                           0x2C01,             "ar-jo",        "arjo",
            "Arabic(Lebanon)",                          0x3001,             "ar-lb",        "arlb",
            "Arabic(Kuwait)",                           0x3401,             "ar-kw",        "arkw",
            "Arabic(U.A.E.)",                           0x3801,             "ar-ae",        "arae",
            "Arabic(Bahrain)",                          0x3C01,             "ar-bh",        "arbh",
            "Arabic(Qatar)",                            0x4001,             "ar-qa",        "arqa",
            "Basque",                                   0x042D,             "eu",           "eu",
            "Bulgarian",                                0x0402,             "bg",           "bg",
            "Belarusian",                               0x0423,             "be",           "be",
            "Catalan",                                  0x0403,             "ca",           "ca",
            "Chinese(Taiwan)",                          0x0404,             "zh-tw",        "zhtw",
            "Chinese(PRC)",                             0x0804,             "zh-cn",        "zhcn",
            "Chinese(Hong Kong)",                       0x0C04,             "zh-hk",        "zhhk",
            "Chinese(Singapore)",                       0x1004,             "zh-sg",        "zhsg",
            "Croatian",                                 0x041a,             "hr",           "hr",
            "Czech",                                    0x0405,             "cs",           "cs",
            "Danish",                                   0x0406,             "da",           "da",
            "Dutch(Standard)",                          0x0413,             "nl",           "nl",
            "Dutch(Belgian)",                           0x0813,             "nl-be",        "nlbe",
            "English",                                  0x0009,             "en",           "en",
            "English(United States)",                   0x0409,             "en-us",        "enus",
            "English(British)",                         0x0809,             "en-gb",        "engb",
            "English(Australian)",                      0x0c09,             "en-au",        "enau",
            "English(Canadian)",                        0x1009,             "en-ca",        "enca",
            "English(New Zealand)",                     0x1409,             "en-nz",        "ennz",
            "English(Ireland)",                         0x1809,             "en-ie",        "enie",
            "English(South Africa)",                    0x1c09,             "en-za",        "enza",
            "English(Jamaica)",                         0x2009,             "en-jm",        "enjm",
            "English(Caribbean)",                       0x2409,             "en",           "en",
            "English(Belize)",                          0x2809,             "en-bz",        "enbz",
            "English(Trinidad)",                        0x2c09,             "en-tt",        "entt",
            "Estonian",                                 0x0425,             "et",           "et",
            "Faeroese",                                 0x0438,             "fo",           "fo",
            "Farsi",                                    0x0429,             "fa",           "fa",
            "Finnish",                                  0x040b,             "fi",           "fi",
            "French(Standard)",                         0x040c,             "fr",           "fr",
            "French(Belgian)",                          0x080c,             "fr-be",        "frbe",
            "French(Canadian)",                         0x0c0c,             "fr-ca",        "frca",
            "French(Swiss)",                            0x100c,             "fr-ch",        "frch",
            "French(Luxembourg)",                       0x140c,             "fr-lu",        "frlu",
            "Gaelic(Scots)",                            0x043c,             "gd",           "gd",
            "Gaelic(Irish)",                            0x083c,             "gd-ie",        "gdie",
            "German(Standard)",                         0x0407,             "de",           "de",
            "German(Swiss)",                            0x0807,             "de-ch",        "dech",
            "German(Austrian)",                         0x0c07,             "de-at",        "deat",
            "German(Luxembourg)",                       0x1007,             "de-lu",        "delu",
            "German(Liechtenstein)",                    0x1407,             "de-li",        "deli",
            "Greek",                                    0x0408,             "el",           "el",
            "Hebrew",                                   0x040D,             "he",           "he",
            "Hindi",                                    0x0439,             "hi",           "hi",
            "Hungarian",                                0x040e,             "hu",           "hu",
            "Icelandic",                                0x040F,             "is",           "is",
            "Indonesian",                               0x0421,             "in",           "in",
            "Italian(Standard)",                        0x0410,             "it",           "it",
            "Italian(Swiss)",                           0x0810,             "it-ch",        "itch",
            "Japanese",                                 0x0411,             "ja",           "ja",
            "Korean",                                   0x0412,             "ko",           "ko",
            "Korean(Johab)",                            0x0812,             "ko",           "ko",
            "Latvian",                                  0x0426,             "lv",           "lv",
            "Lithuanian",                               0x0427,             "lt",           "lt",
            "Macedonian",                               0x042f,             "mk",           "mk",
            "Malaysian",                                0x043e,             "ms",           "ms",
            "Maltese",                                  0x043a,             "mt",           "mt",
            "Norwegian(Bokmal)",                        0x0414,             "no",           "no",
            "Norwegian(Nynorsk)",                       0x0814,             "no",           "no",
            "Polish",                                   0x0415,             "pl",           "pl",
            "Portuguese(Brazilian)",                    0x0416,             "pt-br",        "ptbr",
            "Portuguese(Standard)",                     0x0816,             "pt",           "pt",
            "Rhaeto-Romanic",                           0x0417,             "rm",           "rm",
            "Romanian",                                 0x0418,             "ro",           "ro",
            "Romanian(Moldavia)",                       0x0818,             "ro-mo",        "romo",
            "Russian",                                  0x0419,             "ru",           "ru",
            "Russian(Moldavia)",                        0x0819,             "ru-mo",        "rumo",
            "Sami(Lappish)",                            0x043b,             "sz",           "sz",
            "Serbian(Cyrillic)",                        0x0c1a,             "sr",           "sr",
            "Serbian(Latin)",                           0x081a,             "sr",           "sr",
            "Slovak",                                   0x041b,             "sk",           "sk",
            "Slovenian",                                0x0424,             "sl",           "sl",
            "Sorbian",                                  0x042e,             "sb",           "sb",
            "Spanish(Spain - Traditional Sort)",        0x040a,             "es",           "es",
            "Spanish(Mexican)",                         0x080a,             "es-mx",        "esmx",
            "Spanish(Spain - Modern Sort)",             0x0c0a,             "es",           "es",
            "Spanish(Guatemala)",                       0x100a,             "es-gt",        "esgt",
            "Spanish(Costa Rica)",                      0x140a,             "es-cr",        "escr",
            "Spanish(Panama)",                          0x180a,             "es-pa",        "espa",
            "Spanish(Dominican Republic)",              0x1c0a,             "es-do",        "esdo",
            "Spanish(Venezuela)",                       0x200a,             "es-ve",        "esve",
            "Spanish(Colombia)",                        0x240a,             "es-co",        "esco",
            "Spanish(Peru)",                            0x280a,             "es-pe",        "espe",
            "Spanish(Argentina)",                       0x2c0a,             "es-ar",        "esar",
            "Spanish(Ecuador)",                         0x300a,             "es-ec",        "esec",
            "Spanish(Chile)",                           0x340a,             "es-cl",        "escl",
            "Spanish(Uruguay)",                         0x380a,             "es-uy",        "esuy",
            "Spanish(Paraguay)",                        0x3c0a,             "es-py",        "espy",
            "Spanish(Bolivia)",                         0x400a,             "es-bo",        "esbo",
            "Spanish(El Salvador)",                     0x440a,             "es-sv",        "essv",
            "Spanish(Honduras)",                        0x480a,             "es-hn",        "eshn",
            "Spanish(Nicaragua)",                       0x4c0a,             "es-ni",        "esni",
            "Spanish(Puerto Rico)",                     0x500a,             "es-pr",        "espr",
            "Sutu",                                     0x0430,             "sx",           "sx",
            "Swedish",                                  0x041D,             "sv",           "sv",
            "Swedish(Finland)",                         0x081d,             "sv-fi",        "svfi",
            "Thai",                                     0x041E,             "th",           "th",
            "Tsonga",                                   0x0431,             "ts",           "ts",
            "Tswana",                                   0x0432,             "tn",           "tn",
            "Turkish",                                  0x041f,             "tr",           "tr",
            "Ukrainian",                                0x0422,             "uk",           "uk",
            "Urdu",                                     0x0420,             "ur",           "ur",
            "Venda",                                    0x0433,             "ve",           "ve",
            "Vietnamese",                               0x042a,             "vi",           "vi",
            "Xhosa",                                    0x0434,             "xh",           "xh",
            "Yiddish",                                  0x043d,             "ji",           "ji",
            "Zulu",                                     0x0435,             "zu",           "zu",
        };


    m_pLangTable = &langmatch[0];
    SystemLocaleID = GetSystemDefaultLCID();
    UserLocID = GetUserDefaultLCID();


    tablenum = (sizeof(langmatch)) / sizeof(langmatch[0]);


    LocID = SystemLocaleID;

}


//-----------------------------------------------------------------------//
// This routine sets the current language using the specified LCID.

BOOL CLangInfo::SetMyLanguage(LCID lcid)
{
    INT x;

    for(x=0; x<tablenum; x++)
    {   
        if (m_pLangTable[x].LocaleID == lcid)
        {   LocID = lcid;
            Element = x;
            return TRUE;
        }
    }
        
    return FALSE;
}


//-----------------------------------------------------------------------//
// This routine sets the current language using the absolute index value.

BOOL CLangInfo::SetMyIndex(INT ndx)
{
    
    if(ndx < 0 || ndx >= tablenum) return FALSE;
    
    Element = ndx; 
    LocID = m_pLangTable[Element].LocaleID;

    return TRUE;
}

//-----------------------------------------------------------------------//

BOOL CLangInfo::GetMySystemLangString(char *szLangString) const
{
    UINT iRtn =0;
    char szBuff[50];
    iRtn = GetLocaleInfo(   LocID,
                            LOCALE_SENGLANGUAGE,    // type of information 
                            szBuff,
                            (sizeof(szBuff)) / sizeof(szBuff[0])    // size of buffer 
                       );
    if((!iRtn) ||  (strlen(szLangString) < iRtn))
        return(FALSE);
    
    strcpy(szLangString, szBuff);

    return(TRUE);
}


//-----------------------------------------------------------------------//

BOOL CLangInfo::GetMyUserLangString(char *szLangString) const
{
    int iRtn =0;
    char szBuff[50];
    iRtn = GetLocaleInfo(   UserLocID,
                            LOCALE_SENGLANGUAGE,    // type of information 
                            szBuff,
                            (sizeof(szBuff)) / sizeof(szBuff[0])    // size of buffer 
                       );
    if((!iRtn) ||  ((sizeof(szLangString)/sizeof(szLangString[0])) < iRtn))

        return(FALSE);
    
    strcpy(szLangString, szBuff);

    return(TRUE);
}




//-----------------------------------------------------------------------//

LCID CLangInfo::GetMySystemLocale() const
{
    return (m_pLangTable[Element].LocaleID);
}

//=======================================================================//


BOOL CLangInfo::GetAcceptLanguageString(LCID Locale, char *szAcceptLngStr) const
{
                
        for (int i=0; i<tablenum; i++)                          
        {   
            if (Locale == m_pLangTable[i].LocaleID)         //if we find a LOCALE match
            {   
                strcpy(szAcceptLngStr, m_pLangTable[i].AcceptLanguage);
                return(TRUE);
            }                
        }
        
        LCID lcid = (NULL);
        char szLocaleStr[10];
        lcid = GetPrimaryLanguageInfo(Locale,szLocaleStr); 
        if(lcid)
        {
            for (i=0; i<tablenum; i++)                          
            {   
                if (lcid == m_pLangTable[i].LocaleID)           //look for primary language match
                {   
                    strcpy(szAcceptLngStr, m_pLangTable[i].AcceptLanguage);
                    szAcceptLngStr[2] = '\0';
                    return(TRUE);
                }                
            }
        }
        

        return(FALSE);
}

//-----------------------------------------------------------------------//


BOOL CLangInfo::GetAcceptLanguageStringAbbr(LCID Locale, char *szAcceptLngStr) const
{
                
        for (int i=0; i<tablenum; i++)                          
        {   
            if (Locale == m_pLangTable[i].LocaleID)         //if we find a LOCALE match
            {   
                strcpy(szAcceptLngStr, m_pLangTable[i].AcceptLangAbbr);
                return(TRUE);
            }                
        }
    
        LCID lcid = (NULL);
        char szLocaleStr[10];
        lcid = GetPrimaryLanguageInfo(Locale,szLocaleStr); 
        if(lcid)
        {
            for (i=0; i<tablenum; i++)                          
            {   
                if (lcid == m_pLangTable[i].LocaleID)           //look for primary language match
                {   
                    strcpy(szAcceptLngStr, m_pLangTable[i].AcceptLangAbbr);
                    szAcceptLngStr[2] = '\0';
                    return(TRUE);
                }                
            }
        }

        

        return(FALSE);
}


//-----------------------------------------------------------------------//



BOOL CLangInfo::GetLocaleStrings(LPSTR szAcceptLngStr, char *szLocaleStr) const
{
        if(sizeof(szLocaleStr) < 3)
            return(FALSE);

        for (int i=0; i<tablenum; i++)                          
        {   
            if ((2 == CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, 
                szAcceptLngStr,- 1, m_pLangTable[i].AcceptLanguage,- 1)) ||
                                (2 == CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, 
                                szAcceptLngStr,- 1, m_pLangTable[i].AcceptLangAbbr,- 1))  )
            {   
                char szBuff[50];
                int iRtn=0;
                iRtn = GetLocaleInfo(   m_pLangTable[i].LocaleID,
                            LOCALE_SABBREVLANGNAME, 
                            szBuff,
                            (sizeof(szBuff)) / sizeof(szBuff[0])
                       );
                
                if((!iRtn) ||  ((sizeof(szLocaleStr)/sizeof(szLocaleStr[0])) < iRtn))
                        return(FALSE);


                
                strcpy(szLocaleStr, szBuff);
                return(TRUE);
            }                
        }               

        return(FALSE);
}

//-----------------------------------------------------------------------//

BOOL CLangInfo::GetLocaleStrings(LCID Locale, char *szLocaleStr) const

{

    
    int iReturn = 0;
    char szBuff[50];

    
    iReturn = GetLocaleInfo(Locale, LOCALE_SABBREVLANGNAME, szBuff, sizeof(szBuff));
    
        if((!iReturn) ||  ((sizeof(szLocaleStr)/sizeof(szLocaleStr[0])) < iReturn))
                    return(0);


    strcpy(szLocaleStr, szBuff);
    
    return(TRUE);


}


//-----------------------------------------------------------------------//

//BOOL CLangInfo::GetPrimaryLanguageInfo(LCID Locale, char *szAcceptLngStr, char *szLocaleStr) const
LCID CLangInfo::GetPrimaryLanguageInfo(LCID Locale, char *szLocaleStr) const

{

    LCID lcid = NULL;
    int iReturn = 0;
    char szBuff[50];

    lcid = MAKELCID(MAKELANGID(PRIMARYLANGID(LANGIDFROMLCID(Locale)), SUBLANG_DEFAULT), SORT_DEFAULT);
                        iReturn = GetLocaleInfo(lcid, LOCALE_SABBREVLANGNAME, szBuff, sizeof(szBuff));
    if((!iReturn) ||  ((sizeof(szLocaleStr)/sizeof(szLocaleStr[0])) < iReturn))
                return(0);


    strcpy(szLocaleStr, szBuff);
    
    return(lcid);


}
