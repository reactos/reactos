#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <stdio.h>
#include "uimain.h"
#include "rdesktop.h"
#include "bsops.h"
#include "orders.h"
#include "resource.h"

//#include <stdio.h>

#ifndef __TODO_MSTSC_H
#define __TODO_MSTSC_H


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

BOOL OpenRDPConnectDialog(HINSTANCE hInstance, PRDPSETTINGS pRdpSettings);
PRDPSETTINGS LoadRdpSettingsFromFile(LPWSTR lpFile);
BOOL SaveRdpSettingsToFile(LPWSTR lpFile, PRDPSETTINGS pRdpSettings);
INT GetIntegerFromSettings(PRDPSETTINGS pSettings, LPWSTR lpValue);
LPWSTR GetStringFromSettings(PRDPSETTINGS pSettings, LPWSTR lpValue);
BOOL SetIntegerToSettings(PRDPSETTINGS pRdpSettings, LPWSTR lpKey, INT Value);
BOOL SetStringToSettings(PRDPSETTINGS pRdpSettings, LPWSTR lpKey, LPWSTR lpValue);


#endif /* __TODO_MSTSC_H */
