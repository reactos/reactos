#pragma once

typedef struct
{
    PWCHAR FontName;
    PWCHAR SubFontName;
} MUI_SUBFONT;

typedef struct
{
    PWCHAR LangID; // Language ID (like "0409")
    PWCHAR LayoutID; // Layout ID (like "00000409")
} MUI_LAYOUTS;

typedef struct
{
    PWCHAR LanguageID;
    PWCHAR ACPage;
    PWCHAR OEMCPage;
    PWCHAR MACCPage;
    PWCHAR LanguageDescriptor;
    PWCHAR GeoID;
    const MUI_SUBFONT * MuiSubFonts;
    const MUI_LAYOUTS * MuiLayouts;
} MUI_LANGUAGE;

BOOLEAN
IsLanguageAvailable(
    PWCHAR LanguageId);

PCWSTR
MUIDefaultKeyboardLayout(VOID);

PWCHAR
MUIGetGeoID(VOID);

const MUI_LAYOUTS *
MUIGetLayoutsList(VOID);

BOOLEAN
AddKbLayoutsToRegistry(
    IN const MUI_LAYOUTS *MuiLayouts);

BOOLEAN
AddCodePage(VOID);

BOOLEAN
AddKeyboardLayouts(VOID);
