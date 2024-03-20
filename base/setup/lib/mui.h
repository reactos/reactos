#pragma once

typedef struct
{
    PCWSTR FontName;
    PCWSTR SubFontName;
} MUI_SUBFONT;

typedef USHORT LANGID;
typedef ULONG KLID;

/*
 * See http://archives.miloush.net/michkap/archive/2006/10/14/825404.html
 * and the intl.inf LCID map:
 *
 * ; List of locales.
 * ; <LCID> = <Description>,<OEMCP>,<Language Group>,<langID:HKL pair>,<langID:HKL pair>,...
 *
 * Each MUI_LANGUAGE entry corresponds to one such locale description.
 * Each MUI_LAYOUTS entry corresponds to a <langID:HKL pair>.
 */
typedef struct
{
    LANGID LangID; // Language ID (like 0x0409)
    KLID LayoutID; // Layout ID (like 0x00000409)
} MUI_LAYOUTS;

typedef ULONG GEOID; // See winnls.h

typedef struct
{
    PCWSTR LanguageID;
    UINT ACPage;
    UINT OEMCPage;
    UINT MACCPage;
    PCWSTR LanguageDescriptor;
    GEOID GeoID;
    const MUI_SUBFONT* MuiSubFonts;
    const MUI_LAYOUTS* MuiLayouts;
} MUI_LANGUAGE;


BOOLEAN
IsLanguageAvailable(
    IN PCWSTR LanguageId);

KLID
MUIDefaultKeyboardLayout(
    IN PCWSTR LanguageId);

UINT
MUIGetOEMCodePage(
    IN PCWSTR LanguageId);

GEOID
MUIGetGeoID(
    IN PCWSTR LanguageId);

const MUI_LAYOUTS*
MUIGetLayoutsList(
    IN PCWSTR LanguageId);

BOOLEAN
AddKbLayoutsToRegistry(
    _In_ const MUI_LAYOUTS* MuiLayouts);

BOOLEAN
AddKeyboardLayouts(
    IN PCWSTR LanguageId);

BOOLEAN
AddCodePage(
    IN PCWSTR LanguageId);
