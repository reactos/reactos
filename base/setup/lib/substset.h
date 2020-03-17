#pragma once

typedef struct _FONTSUBSTSETTINGS
{
    BOOL bFoundFontMINGLIU;
    BOOL bFoundFontSIMSUN;
    BOOL bFoundFontMSSONG;
    BOOL bFoundFontMSGOTHIC;
    BOOL bFoundFontMSMINCHO;
    BOOL bFoundFontGULIM;
    BOOL bFoundFontBATANG;
    INT nTahomaCount;
    INT nTimesCount;
    INT nCourierCount;
} FONTSUBSTSETTINGS, *PFONTSUBSTSETTINGS;

BOOL
DoRegistryFontFixup(PFONTSUBSTSETTINGS pSettings, LANGID LangID);
