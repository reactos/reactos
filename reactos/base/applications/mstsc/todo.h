#define MAXKEY 256
#define MAXVALUE 256

typedef struct _SETTINGS
{
    WCHAR Key[MAXKEY];
    WCHAR Type; // holds 'i' or 's'
    union {
        INT i;
        WCHAR s[MAXVALUE];
    } Value;
} SETTINGS, *PSETTINGS;

typedef struct _RDPSETTINGS
{
    PSETTINGS pSettings;
    INT NumSettings;
} RDPSETTINGS, *PRDPSETTINGS;


PRDPSETTINGS LoadRdpSettingsFromFile(LPWSTR lpFile);
INT GetIntegerFromSettings(PRDPSETTINGS pSettings, LPWSTR lpValue);
LPWSTR GetStringFromSettings(PRDPSETTINGS pSettings, LPWSTR lpValue);
