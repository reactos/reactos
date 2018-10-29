#pragma once

typedef struct
{
    PCWSTR FontName;
    PCWSTR SubFontName;
} MUI_SUBFONT;

typedef struct
{
    PCWSTR LangID; // Language ID (like "0409")
    PCWSTR LayoutID; // Layout ID (like "00000409")
} MUI_LAYOUTS;

typedef struct
{
    PCWSTR LanguageID;
    PCWSTR ACPage;
    PCWSTR OEMCPage;
    PCWSTR MACCPage;
    PCWSTR LanguageDescriptor;
    PCWSTR GeoID;
    const MUI_SUBFONT * MuiSubFonts;
    const MUI_LAYOUTS * MuiLayouts;
} MUI_LANGUAGE;


BOOLEAN
IsLanguageAvailable(
    IN PCWSTR LanguageId);

PCWSTR
MUIDefaultKeyboardLayout(
    IN PCWSTR LanguageId);

PCWSTR
MUIGetOEMCodePage(
    IN PCWSTR LanguageId);

PCWSTR
MUIGetGeoID(
    IN PCWSTR LanguageId);

const MUI_LAYOUTS*
MUIGetLayoutsList(
    IN PCWSTR LanguageId);

BOOLEAN
AddKbLayoutsToRegistry(
    IN const MUI_LAYOUTS *MuiLayouts);

BOOLEAN
AddKeyboardLayouts(
    IN PCWSTR LanguageId);

BOOLEAN
AddCodePage(
    IN PCWSTR LanguageId);
