#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef HANDLE HTHEMEFILE;

typedef struct tagPARSE_ERROR_INFO
{
    DWORD cbSize;
    UINT nID;
    WCHAR szDescription[2 * MAX_PATH];
    WCHAR szFile[MAX_PATH];
    WCHAR szLine[MAX_PATH];
    INT nLineNo;
} PARSE_ERROR_INFO, *PPARSE_ERROR_INFO;

HRESULT WINAPI GetThemeParseErrorInfo(_Inout_ PPARSE_ERROR_INFO pInfo);

/**********************************************************************
 *              ENUMTHEMEPROC
 *
 * Callback function for EnumThemes.
 *
 * RETURNS
 *     TRUE to continue enumeration, FALSE to stop
 *
 * PARAMS
 *     lpReserved          Always 0
 *     pszThemeFileName    Full path to theme msstyles file
 *     pszThemeName        Display name for theme
 *     pszToolTip          Tooltip name for theme
 *     lpReserved2         Always 0
 *     lpData              Value passed through lpData from EnumThemes
 */
typedef BOOL (CALLBACK *ENUMTHEMEPROC)(LPVOID lpReserved, LPCWSTR pszThemeFileName,
                                       LPCWSTR pszThemeName, LPCWSTR pszToolTip, LPVOID lpReserved2,
                                       LPVOID lpData);

/**********************************************************************
 *              PARSETHEMEINIFILEPROC
 *
 * Callback function for ParseThemeIniFile.
 *
 * RETURNS
 *     TRUE to continue enumeration, FALSE to stop
 *
 * PARAMS
 *     dwType              Entry type
 *     pszParam1           Use defined by entry type
 *     pszParam2           Use defined by entry type
 *     pszParam3           Use defined by entry type
 *     dwParam             Use defined by entry type
 *     lpData              Value passed through lpData from ParseThemeIniFile
 *
 * NOTES
 * I don't know what the valid entry types are
 */
typedef BOOL (CALLBACK* PARSETHEMEINIFILEPROC)(DWORD dwType, LPWSTR pszParam1,
                                               LPWSTR pszParam2, LPWSTR pszParam3,
                                               DWORD dwParam, LPVOID lpData);

/* Structure filled in by EnumThemeColors() and EnumeThemeSizes() with the
 * various strings for a theme color or size. */
typedef struct tagTHEMENAMES
{
    WCHAR szName[MAX_PATH+1];
    WCHAR szDisplayName[MAX_PATH+1];
    WCHAR szTooltip[MAX_PATH+1];
} THEMENAMES, *PTHEMENAMES;

/* Declarations for undocumented functions for use internally */
DWORD WINAPI QueryThemeServices(void);

HRESULT WINAPI OpenThemeFile(LPCWSTR pszThemeFileName,
                             LPCWSTR pszColorName,
                             LPCWSTR pszSizeName,
                             HTHEMEFILE *hThemeFile,
                             DWORD unknown);

HRESULT WINAPI CloseThemeFile(HTHEMEFILE hThemeFile);

HRESULT WINAPI ApplyTheme(HTHEMEFILE hThemeFile,
                          char *unknown,
                          HWND hWnd);

HRESULT WINAPI GetThemeDefaults(LPCWSTR pszThemeFileName,
                                LPWSTR pszColorName,
                                DWORD dwColorNameLen,
                                LPWSTR pszSizeName,
                                DWORD dwSizeNameLen);

HRESULT WINAPI EnumThemes(LPCWSTR pszThemePath,
                          ENUMTHEMEPROC callback,
                          LPVOID lpData);

HRESULT WINAPI EnumThemeColors(LPWSTR pszThemeFileName,
                               LPWSTR pszSizeName,
                               DWORD dwColorNum,
                               PTHEMENAMES pszColorNames);

HRESULT WINAPI EnumThemeSizes(LPWSTR pszThemeFileName,
                              LPWSTR pszColorName,
                              DWORD dwSizeNum,
                              PTHEMENAMES pszColorNames);

HRESULT WINAPI ParseThemeIniFile(LPCWSTR pszIniFileName,
                                 LPWSTR pszUnknown,
                                 PARSETHEMEINIFILEPROC callback,
                                 LPVOID lpData);

HTHEME WINAPI OpenThemeDataFromFile(HTHEMEFILE hThemeFile,
                                    HWND hwnd,
                                    LPCWSTR pszClassList,
                                    DWORD flags);

/* The DNCP_* flags let the caller decide what should be painted */
#define DNCP_ACTIVEWINDOW   0x1
#define DNCP_INACTIVEWINDOW 0x2
#define DNCP_DIALOGWINDOW   0x4
#define DNCP_DRAW_ALL       DNCP_ACTIVEWINDOW | DNCP_INACTIVEWINDOW | DNCP_DIALOGWINDOW

HRESULT WINAPI DrawNCPreview(HDC hDC,
                             DWORD DNCP_Flag,
                             LPRECT prcPreview,
                             LPCWSTR pszThemeFileName,
                             LPCWSTR pszColorName,
                             LPCWSTR pszSizeName,
                             PNONCLIENTMETRICSW pncMetrics,
                             COLORREF* lpaRgbValues);

BOOL WINAPI ThemeHooksInstall(VOID);

BOOL WINAPI ThemeHooksRemove(VOID);

#ifdef __cplusplus
} // extern "C"
#endif
