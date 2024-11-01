#pragma once

/*
 * See the intl.inf file map:
 *
 * ; List of locales.
 * ; <LCID> = <Font>,<Font Substitute>
 */
typedef struct
{
    PCWSTR FontName;
    PCWSTR SubFontName;
} MUI_SUBFONT;

typedef USHORT LANGID;
// #define MAXUSHORT USHRT_MAX
// typedef DWORD LCID;
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
    LCID LanguageID; // LocaleID;
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
    _In_ LANGID LanguageId);

KLID
MUIDefaultKeyboardLayout(
    _In_ LANGID LanguageId);

UINT
MUIGetOEMCodePage(
    _In_ LANGID LanguageId);

GEOID
MUIGetGeoID(
    _In_ LANGID LanguageId);

const MUI_LAYOUTS*
MUIGetLayoutsList(
    _In_ LANGID LanguageId);

BOOLEAN
AddKbLayoutsToRegistry(
    _In_ const MUI_LAYOUTS* MuiLayouts);

BOOLEAN
AddKeyboardLayouts(
    _In_ LANGID LanguageId);

BOOLEAN
AddCodePage(
    _In_ LANGID LanguageId);
