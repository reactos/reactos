/*=========================================================================*
 |                LRC32 - Localized Resource Test Utility                  |
 |                                                                         |
 |                Copyright 1996 by Microsoft Corporation                  |
 |                         KevinGj - January 1996                          |
 |                                                                         |
 |=========================================================================|
 |              LangInfo.h : Header for the CLangInfo class                |
 *=========================================================================*/

#ifndef LANGCODE_H
#define LANGCODE_H


#include "windows.h"

#define LOCALEIDMASK 0x7FF   //00000000000000000011111111111

typedef struct
{
    CHAR    LocaleName[80];     // Pseudo MS NLS Locale Name for reference (should use GetLocaleInfo() for real name)
    LCID    LocaleID;           // DWORD Locale ID
    CHAR    AcceptLanguage[6];  // ISO 369 abbreviated (no hyphen) language string
    CHAR    AcceptLangAbbr[5];  // ISO 369 standard accept language string
} LANGPROP, *PLANGPROP;


class CLangInfo
{
public:
    //Construction
    CLangInfo();

    // Control
    BOOL SetMyLanguage(LCID lcid);
    BOOL SetMyIndex(INT ndx);

    //Queries
    BOOL GetAcceptLanguageString(LCID Locale, char *szAcceptLngStr) const;
    BOOL GetAcceptLanguageStringAbbr(LCID Locale, char *szAcceptLngStr) const;


    BOOL GetLocaleStrings(LPSTR szAcceptLngStr, char *szLocaleStr) const;
    BOOL GetLocaleStrings(LCID Locale, char *szLocaleStr) const;
    
    LCID GetPrimaryLanguageInfo(LCID Locale, char *szLocaleStr) const;



    LCID GetMySystemLocale() const; 
    BOOL GetMySystemLangString(char *szLangString) const;
    BOOL GetMyUserLangString(char *szLangString) const;


    LCID LocID;
    LCID SystemLocaleID;

private:
    unsigned int Element;
    unsigned int Active;
    INT tablenum;
    LCID SystemLocID;
    LCID UserLocID;
    LANGPROP const *m_pLangTable;
    HKL MyHKL;

};

#endif // LANGINFO_H

//=======================================================================//
//                                          - EOF -                                 //
//=======================================================================//

