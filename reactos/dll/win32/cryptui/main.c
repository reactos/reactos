/*
 * Copyright 2008 Juan Lang
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <stdarg.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winuser.h"
#include "softpub.h"
#include "wingdi.h"
#include "richedit.h"
#include "ole2.h"
#include "richole.h"
#include "commdlg.h"
#include "commctrl.h"
#include "cryptuiapi.h"
#include "cryptuires.h"
#include "urlmon.h"
#include "hlink.h"
#include "winreg.h"
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(cryptui);

static HINSTANCE hInstance;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            hInstance = hinstDLL;
            DisableThreadLibraryCalls(hinstDLL);
            break;
        case DLL_PROCESS_DETACH:
            break;
        default:
            break;
    }
    return TRUE;
}

#define MAX_STRING_LEN 512

static void add_cert_columns(HWND hwnd)
{
    HWND lv = GetDlgItem(hwnd, IDC_MGR_CERTS);
    RECT rc;
    WCHAR buf[MAX_STRING_LEN];
    LVCOLUMNW column;

    SendMessageW(lv, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);
    GetWindowRect(lv, &rc);
    LoadStringW(hInstance, IDS_SUBJECT_COLUMN, buf,
     sizeof(buf) / sizeof(buf[0]));
    column.mask = LVCF_WIDTH | LVCF_TEXT;
    column.cx = (rc.right - rc.left) * 29 / 100 - 2;
    column.pszText = buf;
    SendMessageW(lv, LVM_INSERTCOLUMNW, 0, (LPARAM)&column);
    LoadStringW(hInstance, IDS_ISSUER_COLUMN, buf,
     sizeof(buf) / sizeof(buf[0]));
    SendMessageW(lv, LVM_INSERTCOLUMNW, 1, (LPARAM)&column);
    column.cx = (rc.right - rc.left) * 16 / 100 - 2;
    LoadStringW(hInstance, IDS_EXPIRATION_COLUMN, buf,
     sizeof(buf) / sizeof(buf[0]));
    SendMessageW(lv, LVM_INSERTCOLUMNW, 2, (LPARAM)&column);
    column.cx = (rc.right - rc.left) * 23 / 100 - 1;
    LoadStringW(hInstance, IDS_FRIENDLY_NAME_COLUMN, buf,
     sizeof(buf) / sizeof(buf[0]));
    SendMessageW(lv, LVM_INSERTCOLUMNW, 3, (LPARAM)&column);
}

static void add_cert_to_view(HWND lv, PCCERT_CONTEXT cert, DWORD *allocatedLen,
 LPWSTR *str)
{
    DWORD len;
    LVITEMW item;
    WCHAR dateFmt[80]; /* sufficient for LOCALE_SSHORTDATE */
    WCHAR date[80];
    SYSTEMTIME sysTime;

    item.mask = LVIF_IMAGE | LVIF_PARAM | LVIF_TEXT;
    item.iItem = SendMessageW(lv, LVM_GETITEMCOUNT, 0, 0);
    item.iSubItem = 0;
    item.iImage = 0;
    item.lParam = (LPARAM)CertDuplicateCertificateContext(cert);
    len = CertGetNameStringW(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL,
     NULL, 0);
    if (len > *allocatedLen)
    {
        HeapFree(GetProcessHeap(), 0, *str);
        *str = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (*str)
            *allocatedLen = len;
    }
    if (*str)
    {
        CertGetNameStringW(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL,
         *str, len);
        item.pszText = *str;
        SendMessageW(lv, LVM_INSERTITEMW, 0, (LPARAM)&item);
    }

    item.mask = LVIF_TEXT;
    len = CertGetNameStringW(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE,
     CERT_NAME_ISSUER_FLAG, NULL, NULL, 0);
    if (len > *allocatedLen)
    {
        HeapFree(GetProcessHeap(), 0, *str);
        *str = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (*str)
            *allocatedLen = len;
    }
    if (*str)
    {
        CertGetNameStringW(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE,
         CERT_NAME_ISSUER_FLAG, NULL, *str, len);
        item.pszText = *str;
        item.iSubItem = 1;
        SendMessageW(lv, LVM_SETITEMTEXTW, item.iItem, (LPARAM)&item);
    }

    GetLocaleInfoW(LOCALE_SYSTEM_DEFAULT, LOCALE_SSHORTDATE, dateFmt,
     sizeof(dateFmt) / sizeof(dateFmt[0]));
    FileTimeToSystemTime(&cert->pCertInfo->NotAfter, &sysTime);
    GetDateFormatW(LOCALE_SYSTEM_DEFAULT, 0, &sysTime, dateFmt, date,
     sizeof(date) / sizeof(date[0]));
    item.pszText = date;
    item.iSubItem = 2;
    SendMessageW(lv, LVM_SETITEMTEXTW, item.iItem, (LPARAM)&item);

    len = CertGetNameStringW(cert, CERT_NAME_FRIENDLY_DISPLAY_TYPE, 0, NULL,
     NULL, 0);
    if (len > *allocatedLen)
    {
        HeapFree(GetProcessHeap(), 0, *str);
        *str = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (*str)
            *allocatedLen = len;
    }
    if (*str)
    {
        CertGetNameStringW(cert, CERT_NAME_FRIENDLY_DISPLAY_TYPE, 0, NULL,
         *str, len);
        item.pszText = *str;
        item.iSubItem = 3;
        SendMessageW(lv, LVM_SETITEMTEXTW, item.iItem, (LPARAM)&item);
    }
}

static LPSTR get_cert_mgr_usages(void)
{
    static const WCHAR keyName[] = { 'S','o','f','t','w','a','r','e','\\','M',
     'i','c','r','o','s','o','f','t','\\','C','r','y','p','t','o','g','r','a',
     'p','h','y','\\','U','I','\\','C','e','r','t','m','g','r','\\','P','u',
     'r','p','o','s','e',0 };
    LPSTR str = NULL;
    HKEY key;

    if (!RegCreateKeyExW(HKEY_CURRENT_USER, keyName, 0, NULL, 0, KEY_READ,
     NULL, &key, NULL))
    {
        LONG rc;
        DWORD type, size;

        rc = RegQueryValueExA(key, "Purpose", NULL, &type, NULL, &size);
        if ((!rc || rc == ERROR_MORE_DATA) && type == REG_SZ)
        {
            str = HeapAlloc(GetProcessHeap(), 0, size);
            if (str)
            {
                rc = RegQueryValueExA(key, "Purpose", NULL, NULL, (LPBYTE)str,
                 &size);
                if (rc)
                {
                    HeapFree(GetProcessHeap(), 0, str);
                    str = NULL;
                }
            }
        }
        RegCloseKey(key);
    }
    return str;
}

typedef enum {
    PurposeFilterShowAll = 0,
    PurposeFilterShowAdvanced = 1,
    PurposeFilterShowOID = 2
} PurposeFilter;

static void initialize_purpose_selection(HWND hwnd)
{
    HWND cb = GetDlgItem(hwnd, IDC_MGR_PURPOSE_SELECTION);
    WCHAR buf[MAX_STRING_LEN];
    LPSTR usages;
    int index;

    LoadStringW(hInstance, IDS_PURPOSE_ALL, buf,
     sizeof(buf) / sizeof(buf[0]));
    index = SendMessageW(cb, CB_INSERTSTRING, -1, (LPARAM)buf);
    SendMessageW(cb, CB_SETITEMDATA, index, (LPARAM)PurposeFilterShowAll);
    LoadStringW(hInstance, IDS_PURPOSE_ADVANCED, buf,
     sizeof(buf) / sizeof(buf[0]));
    index = SendMessageW(cb, CB_INSERTSTRING, -1, (LPARAM)buf);
    SendMessageW(cb, CB_SETITEMDATA, index, (LPARAM)PurposeFilterShowAdvanced);
    SendMessageW(cb, CB_SETCURSEL, 0, 0);
    if ((usages = get_cert_mgr_usages()))
    {
        LPSTR ptr, comma;

        for (ptr = usages, comma = strchr(ptr, ','); ptr && *ptr;
         ptr = comma ? comma + 1 : NULL,
         comma = ptr ? strchr(ptr, ',') : NULL)
        {
            PCCRYPT_OID_INFO info;

            if (comma)
                *comma = 0;
            if ((info = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY, ptr, 0)))
            {
                index = SendMessageW(cb, CB_INSERTSTRING, 0,
                 (LPARAM)info->pwszName);
                SendMessageW(cb, CB_SETITEMDATA, index, (LPARAM)info);
            }
        }
        HeapFree(GetProcessHeap(), 0, usages);
    }
}

extern BOOL WINAPI WTHelperGetKnownUsages(DWORD action,
 PCCRYPT_OID_INFO **usages);

static CERT_ENHKEY_USAGE *add_oid_to_usage(CERT_ENHKEY_USAGE *usage, LPSTR oid)
{
    if (!usage->cUsageIdentifier)
        usage->rgpszUsageIdentifier = HeapAlloc(GetProcessHeap(), 0,
         sizeof(LPSTR));
    else
        usage->rgpszUsageIdentifier = HeapReAlloc(GetProcessHeap(), 0,
         usage->rgpszUsageIdentifier,
         (usage->cUsageIdentifier + 1) * sizeof(LPSTR));
    if (usage->rgpszUsageIdentifier)
        usage->rgpszUsageIdentifier[usage->cUsageIdentifier++] = oid;
    else
    {
        HeapFree(GetProcessHeap(), 0, usage);
        usage = NULL;
    }
    return usage;
}

static CERT_ENHKEY_USAGE *convert_usages_str_to_usage(LPSTR usageStr)
{
    CERT_ENHKEY_USAGE *usage = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
     sizeof(CERT_ENHKEY_USAGE));

    if (usage)
    {
        LPSTR ptr, comma;

        for (ptr = usageStr, comma = strchr(ptr, ','); usage && ptr && *ptr;
         ptr = comma ? comma + 1 : NULL,
         comma = ptr ? strchr(ptr, ',') : NULL)
        {
            if (comma)
                *comma = 0;
            add_oid_to_usage(usage, ptr);
        }
    }
    return usage;
}

static CERT_ENHKEY_USAGE *create_advanced_filter(void)
{
    CERT_ENHKEY_USAGE *advancedUsage = HeapAlloc(GetProcessHeap(),
     HEAP_ZERO_MEMORY, sizeof(CERT_ENHKEY_USAGE));

    if (advancedUsage)
    {
        PCCRYPT_OID_INFO *usages;

        if (WTHelperGetKnownUsages(1, &usages))
        {
            LPSTR disabledUsagesStr;

            if ((disabledUsagesStr = get_cert_mgr_usages()))
            {
                CERT_ENHKEY_USAGE *disabledUsages =
                 convert_usages_str_to_usage(disabledUsagesStr);

                if (disabledUsages)
                {
                    PCCRYPT_OID_INFO *ptr;

                    for (ptr = usages; *ptr; ptr++)
                    {
                        DWORD i;
                        BOOL disabled = FALSE;

                        for (i = 0; !disabled &&
                         i < disabledUsages->cUsageIdentifier; i++)
                            if (!strcmp(disabledUsages->rgpszUsageIdentifier[i],
                             (*ptr)->pszOID))
                                disabled = TRUE;
                        if (!disabled)
                            add_oid_to_usage(advancedUsage,
                             (LPSTR)(*ptr)->pszOID);
                    }
                    /* The individual strings are pointers to disabledUsagesStr,
                     * so they're freed when it is.
                     */
                    HeapFree(GetProcessHeap(), 0,
                     disabledUsages->rgpszUsageIdentifier);
                    HeapFree(GetProcessHeap(), 0, disabledUsages);
                }
                HeapFree(GetProcessHeap(), 0, disabledUsagesStr);
            }
            WTHelperGetKnownUsages(2, &usages);
        }
    }
    return advancedUsage;
}

static void show_store_certs(HWND hwnd, HCERTSTORE store)
{
    HWND lv = GetDlgItem(hwnd, IDC_MGR_CERTS);
    HWND cb = GetDlgItem(hwnd, IDC_MGR_PURPOSE_SELECTION);
    PCCERT_CONTEXT cert = NULL;
    DWORD allocatedLen = 0;
    LPWSTR str = NULL;
    int index;
    PurposeFilter filter = PurposeFilterShowAll;
    LPCSTR oid = NULL;
    CERT_ENHKEY_USAGE *advanced = NULL;

    index = SendMessageW(cb, CB_GETCURSEL, 0, 0);
    if (index >= 0)
    {
        INT_PTR data = SendMessageW(cb, CB_GETITEMDATA, index, 0);

        if (!HIWORD(data))
            filter = data;
        else
        {
            PCCRYPT_OID_INFO info = (PCCRYPT_OID_INFO)data;

            filter = PurposeFilterShowOID;
            oid = info->pszOID;
        }
    }
    if (filter == PurposeFilterShowAdvanced)
        advanced = create_advanced_filter();
    do {
        cert = CertEnumCertificatesInStore(store, cert);
        if (cert)
        {
            BOOL show = FALSE;

            if (filter == PurposeFilterShowAll)
                show = TRUE;
            else
            {
                int numOIDs;
                DWORD cbOIDs = 0;

                if (CertGetValidUsages(1, &cert, &numOIDs, NULL, &cbOIDs))
                {
                    if (numOIDs == -1)
                    {
                        /* -1 implies all usages are valid */
                        show = TRUE;
                    }
                    else
                    {
                        LPSTR *oids = HeapAlloc(GetProcessHeap(), 0, cbOIDs);

                        if (oids)
                        {
                            if (CertGetValidUsages(1, &cert, &numOIDs, oids,
                             &cbOIDs))
                            {
                                int i;

                                if (filter == PurposeFilterShowOID)
                                {
                                    for (i = 0; !show && i < numOIDs; i++)
                                        if (!strcmp(oids[i], oid))
                                            show = TRUE;
                                }
                                else
                                {
                                    for (i = 0; !show && i < numOIDs; i++)
                                    {
                                        DWORD j;

                                        for (j = 0; !show &&
                                         j < advanced->cUsageIdentifier; j++)
                                            if (!strcmp(oids[i],
                                             advanced->rgpszUsageIdentifier[j]))
                                                show = TRUE;
                                    }
                                }
                            }
                            HeapFree(GetProcessHeap(), 0, oids);
                        }
                    }
                }
            }
            if (show)
                add_cert_to_view(lv, cert, &allocatedLen, &str);
        }
    } while (cert);
    HeapFree(GetProcessHeap(), 0, str);
    if (advanced)
    {
        HeapFree(GetProcessHeap(), 0, advanced->rgpszUsageIdentifier);
        HeapFree(GetProcessHeap(), 0, advanced);
    }
}

static const WCHAR my[] = { 'M','y',0 };
static const WCHAR addressBook[] = {
 'A','d','d','r','e','s','s','B','o','o','k',0 };
static const WCHAR ca[] = { 'C','A',0 };
static const WCHAR root[] = { 'R','o','o','t',0 };
static const WCHAR trustedPublisher[] = {
 'T','r','u','s','t','e','d','P','u','b','l','i','s','h','e','r',0 };
static const WCHAR disallowed[] = { 'D','i','s','a','l','l','o','w','e','d',0 };

struct CertMgrStoreInfo
{
    LPCWSTR name;
    int removeWarning;
    int removePluralWarning;
};

static const struct CertMgrStoreInfo defaultStoreList[] = {
 { my, IDS_WARN_REMOVE_MY, IDS_WARN_REMOVE_PLURAL_MY },
 { addressBook, IDS_WARN_REMOVE_ADDRESSBOOK,
   IDS_WARN_REMOVE_PLURAL_ADDRESSBOOK },
 { ca, IDS_WARN_REMOVE_CA, IDS_WARN_REMOVE_PLURAL_CA },
 { root, IDS_WARN_REMOVE_ROOT, IDS_WARN_REMOVE_PLURAL_ROOT },
 { trustedPublisher, IDS_WARN_REMOVE_TRUSTEDPUBLISHER,
   IDS_WARN_REMOVE_PLURAL_TRUSTEDPUBLISHER },
 { disallowed, IDS_WARN_REMOVE_DEFAULT },
};

static const struct CertMgrStoreInfo publisherStoreList[] = {
 { root, IDS_WARN_REMOVE_ROOT, IDS_WARN_REMOVE_PLURAL_ROOT },
 { trustedPublisher, IDS_WARN_REMOVE_TRUSTEDPUBLISHER,
   IDS_WARN_REMOVE_PLURAL_TRUSTEDPUBLISHER },
 { disallowed, IDS_WARN_REMOVE_PLURAL_DEFAULT },
};

struct CertMgrData
{
    HIMAGELIST imageList;
    LPCWSTR title;
    DWORD nStores;
    const struct CertMgrStoreInfo *stores;
};

static void show_cert_stores(HWND hwnd, DWORD dwFlags, struct CertMgrData *data)
{
    const struct CertMgrStoreInfo *storeList;
    int cStores, i;
    HWND tab = GetDlgItem(hwnd, IDC_MGR_STORES);

    if (dwFlags & CRYPTUI_CERT_MGR_PUBLISHER_TAB)
    {
        storeList = publisherStoreList;
        cStores = sizeof(publisherStoreList) / sizeof(publisherStoreList[0]);
    }
    else
    {
        storeList = defaultStoreList;
        cStores = sizeof(defaultStoreList) / sizeof(defaultStoreList[0]);
    }
    if (dwFlags & CRYPTUI_CERT_MGR_SINGLE_TAB_FLAG)
        cStores = 1;
    data->nStores = cStores;
    data->stores = storeList;
    for (i = 0; i < cStores; i++)
    {
        LPCWSTR name;
        TCITEMW item;
        HCERTSTORE store;

        if (!(name = CryptFindLocalizedName(storeList[i].name)))
            name = storeList[i].name;
        store = CertOpenStore(CERT_STORE_PROV_SYSTEM_W, 0, 0,
         CERT_SYSTEM_STORE_CURRENT_USER, storeList[i].name);
        item.mask = TCIF_TEXT | TCIF_PARAM;
        item.pszText = (LPWSTR)name;
        item.lParam = (LPARAM)store;
        SendMessageW(tab, TCM_INSERTITEMW, i, (LPARAM)&item);
    }
}

static void free_certs(HWND lv)
{
    LVITEMW item;
    int items = SendMessageW(lv, LVM_GETITEMCOUNT, 0, 0), i;

    for (i = 0; i < items; i++)
    {
        item.mask = LVIF_PARAM;
        item.iItem = i;
        item.iSubItem = 0;
        SendMessageW(lv, LVM_GETITEMW, 0, (LPARAM)&item);
        CertFreeCertificateContext((PCCERT_CONTEXT)item.lParam);
    }
}

static HCERTSTORE cert_mgr_index_to_store(HWND tab, int index)
{
    TCITEMW item;

    item.mask = TCIF_PARAM;
    SendMessageW(tab, TCM_GETITEMW, index, (LPARAM)&item);
    return (HCERTSTORE)item.lParam;
}

static HCERTSTORE cert_mgr_current_store(HWND hwnd)
{
    HWND tab = GetDlgItem(hwnd, IDC_MGR_STORES);

    return cert_mgr_index_to_store(tab, SendMessageW(tab, TCM_GETCURSEL, 0, 0));
}

static void close_stores(HWND tab)
{
    int i, tabs = SendMessageW(tab, TCM_GETITEMCOUNT, 0, 0);

    for (i = 0; i < tabs; i++)
        CertCloseStore(cert_mgr_index_to_store(tab, i), 0);
}

static void refresh_store_certs(HWND hwnd)
{
    HWND lv = GetDlgItem(hwnd, IDC_MGR_CERTS);

    free_certs(lv);
    SendMessageW(lv, LVM_DELETEALLITEMS, 0, 0);
    show_store_certs(hwnd, cert_mgr_current_store(hwnd));
}

typedef enum {
    CheckBitmapIndexUnchecked = 1,
    CheckBitmapIndexChecked = 2,
    CheckBitmapIndexDisabledUnchecked = 3,
    CheckBitmapIndexDisabledChecked = 4
} CheckBitmapIndex;

static void add_known_usage(HWND lv, PCCRYPT_OID_INFO info,
 CheckBitmapIndex state)
{
    LVITEMW item;

    item.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
    item.state = INDEXTOSTATEIMAGEMASK(state);
    item.stateMask = LVIS_STATEIMAGEMASK;
    item.iItem = SendMessageW(lv, LVM_GETITEMCOUNT, 0, 0);
    item.iSubItem = 0;
    item.lParam = (LPARAM)info;
    item.pszText = (LPWSTR)info->pwszName;
    SendMessageW(lv, LVM_INSERTITEMW, 0, (LPARAM)&item);
}

static void add_known_usages_to_list(HWND lv, CheckBitmapIndex state)
{
    PCCRYPT_OID_INFO *usages;

    if (WTHelperGetKnownUsages(1, &usages))
    {
        PCCRYPT_OID_INFO *ptr;

        for (ptr = usages; *ptr; ptr++)
            add_known_usage(lv, *ptr, state);
        WTHelperGetKnownUsages(2, &usages);
    }
}

static void toggle_usage(HWND hwnd, int iItem)
{
    LVITEMW item;
    int res;
    HWND lv = GetDlgItem(hwnd, IDC_CERTIFICATE_USAGES);

    item.mask = LVIF_STATE;
    item.iItem = iItem;
    item.iSubItem = 0;
    item.stateMask = LVIS_STATEIMAGEMASK;
    res = SendMessageW(lv, LVM_GETITEMW, 0, (LPARAM)&item);
    if (res)
    {
        int state = item.state >> 12;

        item.state = INDEXTOSTATEIMAGEMASK(
         state == CheckBitmapIndexChecked ? CheckBitmapIndexUnchecked :
         CheckBitmapIndexChecked);
        SendMessageW(lv, LVM_SETITEMSTATE, iItem, (LPARAM)&item);
    }
}

static LONG_PTR find_oid_in_list(HWND lv, LPCSTR oid)
{
    PCCRYPT_OID_INFO oidInfo = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY,
     (void *)oid, CRYPT_ENHKEY_USAGE_OID_GROUP_ID);
    LONG_PTR ret;

    if (oidInfo)
    {
        LVFINDINFOW findInfo;

        findInfo.flags = LVFI_PARAM;
        findInfo.lParam = (LPARAM)oidInfo;
        ret = SendMessageW(lv, LVM_FINDITEMW, -1, (LPARAM)&findInfo);
    }
    else
    {
        LVFINDINFOA findInfo;

        findInfo.flags = LVFI_STRING;
        findInfo.psz = oid;
        ret = SendMessageW(lv, LVM_FINDITEMA, -1, (LPARAM)&findInfo);
    }
    return ret;
}

static void save_cert_mgr_usages(HWND hwnd)
{
    static const WCHAR keyName[] = { 'S','o','f','t','w','a','r','e','\\','M',
     'i','c','r','o','s','o','f','t','\\','C','r','y','p','t','o','g','r','a',
     'p','h','y','\\','U','I','\\','C','e','r','t','m','g','r','\\','P','u',
     'r','p','o','s','e',0 };
    HKEY key;
    HWND lv = GetDlgItem(hwnd, IDC_CERTIFICATE_USAGES);
    int purposes = SendMessageW(lv, LVM_GETITEMCOUNT, 0, 0), i;
    LVITEMW item;
    LPSTR str = NULL;

    item.mask = LVIF_STATE | LVIF_PARAM;
    item.iSubItem = 0;
    item.stateMask = LVIS_STATEIMAGEMASK;
    for (i = 0; i < purposes; i++)
    {
        item.iItem = i;
        if (SendMessageW(lv, LVM_GETITEMW, 0, (LPARAM)&item))
        {
            int state = item.state >> 12;

            if (state == CheckBitmapIndexUnchecked)
            {
                CRYPT_OID_INFO *info = (CRYPT_OID_INFO *)item.lParam;
                BOOL firstString = TRUE;

                if (!str)
                    str = HeapAlloc(GetProcessHeap(), 0,
                     strlen(info->pszOID) + 1);
                else
                {
                    str = HeapReAlloc(GetProcessHeap(), 0, str,
                     strlen(str) + 1 + strlen(info->pszOID) + 1);
                    firstString = FALSE;
                }
                if (str)
                {
                    LPSTR ptr = firstString ? str : str + strlen(str);

                    if (!firstString)
                        *ptr++ = ',';
                    strcpy(ptr, info->pszOID);
                }
            }
        }
    }
    if (!RegCreateKeyExW(HKEY_CURRENT_USER, keyName, 0, NULL, 0, KEY_ALL_ACCESS,
     NULL, &key, NULL))
    {
        if (str)
            RegSetValueExA(key, "Purpose", 0, REG_SZ, (const BYTE *)str,
             strlen(str) + 1);
        else
            RegDeleteValueA(key, "Purpose");
        RegCloseKey(key);
    }
    HeapFree(GetProcessHeap(), 0, str);
}

static LRESULT CALLBACK cert_mgr_advanced_dlg_proc(HWND hwnd, UINT msg,
 WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        RECT rc;
        LVCOLUMNW column;
        HWND lv = GetDlgItem(hwnd, IDC_CERTIFICATE_USAGES);
        HIMAGELIST imageList;
        LPSTR disabledUsages;

        GetWindowRect(lv, &rc);
        column.mask = LVCF_WIDTH;
        column.cx = rc.right - rc.left;
        SendMessageW(lv, LVM_INSERTCOLUMNW, 0, (LPARAM)&column);
        imageList = ImageList_Create(16, 16, ILC_COLOR4 | ILC_MASK, 4, 0);
        if (imageList)
        {
            HBITMAP bmp;
            COLORREF backColor = RGB(255, 0, 255);

            bmp = LoadBitmapW(hInstance, MAKEINTRESOURCEW(IDB_CHECKS));
            ImageList_AddMasked(imageList, bmp, backColor);
            DeleteObject(bmp);
            ImageList_SetBkColor(imageList, CLR_NONE);
            SendMessageW(lv, LVM_SETIMAGELIST, LVSIL_STATE, (LPARAM)imageList);
            SetWindowLongPtrW(hwnd, DWLP_USER, (LPARAM)imageList);
        }
        add_known_usages_to_list(lv, CheckBitmapIndexChecked);
        if ((disabledUsages = get_cert_mgr_usages()))
        {
            LPSTR ptr, comma;

            for (ptr = disabledUsages, comma = strchr(ptr, ','); ptr && *ptr;
             ptr = comma ? comma + 1 : NULL,
             comma = ptr ? strchr(ptr, ',') : NULL)
            {
                LONG_PTR index;

                if (comma)
                    *comma = 0;
                if ((index = find_oid_in_list(lv, ptr)) != -1)
                    toggle_usage(hwnd, index);
            }
            HeapFree(GetProcessHeap(), 0, disabledUsages);
        }
        break;
    }
    case WM_NOTIFY:
    {
        NMHDR *hdr = (NMHDR *)lp;
        NMITEMACTIVATE *nm;

        switch (hdr->code)
        {
        case NM_CLICK:
            nm = (NMITEMACTIVATE *)lp;
            toggle_usage(hwnd, nm->iItem);
            SendMessageW(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0);
            break;
        }
        break;
    }
    case WM_COMMAND:
        switch (wp)
        {
        case IDOK:
            save_cert_mgr_usages(hwnd);
            ImageList_Destroy((HIMAGELIST)GetWindowLongPtrW(hwnd, DWLP_USER));
            EndDialog(hwnd, IDOK);
            break;
        case IDCANCEL:
            ImageList_Destroy((HIMAGELIST)GetWindowLongPtrW(hwnd, DWLP_USER));
            EndDialog(hwnd, IDCANCEL);
            break;
        }
        break;
    }
    return 0;
}

static void cert_mgr_clear_cert_selection(HWND hwnd)
{
    WCHAR empty[] = { 0 };

    EnableWindow(GetDlgItem(hwnd, IDC_MGR_EXPORT), FALSE);
    EnableWindow(GetDlgItem(hwnd, IDC_MGR_REMOVE), FALSE);
    EnableWindow(GetDlgItem(hwnd, IDC_MGR_VIEW), FALSE);
    SendMessageW(GetDlgItem(hwnd, IDC_MGR_PURPOSES), WM_SETTEXT, 0,
     (LPARAM)empty);
    refresh_store_certs(hwnd);
}

static PCCERT_CONTEXT cert_mgr_index_to_cert(HWND hwnd, int index)
{
    PCCERT_CONTEXT cert = NULL;
    LVITEMW item;

    item.mask = LVIF_PARAM;
    item.iItem = index;
    item.iSubItem = 0;
    if (SendMessageW(GetDlgItem(hwnd, IDC_MGR_CERTS), LVM_GETITEMW, 0,
     (LPARAM)&item))
        cert = (PCCERT_CONTEXT)item.lParam;
    return cert;
}

static void show_selected_cert(HWND hwnd, int index)
{
    PCCERT_CONTEXT cert = cert_mgr_index_to_cert(hwnd, index);

    if (cert)
    {
        CRYPTUI_VIEWCERTIFICATE_STRUCTW viewInfo;

        memset(&viewInfo, 0, sizeof(viewInfo));
        viewInfo.dwSize = sizeof(viewInfo);
        viewInfo.hwndParent = hwnd;
        viewInfo.pCertContext = cert;
        /* FIXME: this should be modal */
        CryptUIDlgViewCertificateW(&viewInfo, NULL);
    }
}

static void cert_mgr_show_cert_usages(HWND hwnd, int index)
{
    HWND text = GetDlgItem(hwnd, IDC_MGR_PURPOSES);
    PCCERT_CONTEXT cert = cert_mgr_index_to_cert(hwnd, index);
    PCERT_ENHKEY_USAGE usage;
    DWORD size;

    /* Get enhanced key usage.  Have to check for a property and an extension
     * separately, because CertGetEnhancedKeyUsage will succeed and return an
     * empty usage if neither is set.  Unfortunately an empty usage implies
     * no usage is allowed, so we have to distinguish between the two cases.
     */
    if (CertGetEnhancedKeyUsage(cert, CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG,
     NULL, &size))
    {
        usage = HeapAlloc(GetProcessHeap(), 0, size);
        if (!CertGetEnhancedKeyUsage(cert,
         CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG, usage, &size))
        {
            HeapFree(GetProcessHeap(), 0, usage);
            usage = NULL;
        }
    }
    else if (CertGetEnhancedKeyUsage(cert, CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
     NULL, &size))
    {
        usage = HeapAlloc(GetProcessHeap(), 0, size);
        if (!CertGetEnhancedKeyUsage(cert,
         CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG, usage, &size))
        {
            HeapFree(GetProcessHeap(), 0, usage);
            usage = NULL;
        }
    }
    else
        usage = NULL;
    if (usage)
    {
        if (usage->cUsageIdentifier)
        {
            static const WCHAR commaSpace[] = { ',',' ',0 };
            DWORD i, len = 1;
            LPWSTR str, ptr;

            for (i = 0; i < usage->cUsageIdentifier; i++)
            {
                PCCRYPT_OID_INFO info =
                 CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY,
                 usage->rgpszUsageIdentifier[i],
                 CRYPT_ENHKEY_USAGE_OID_GROUP_ID);

                if (info)
                    len += strlenW(info->pwszName);
                else
                    len += strlen(usage->rgpszUsageIdentifier[i]);
                if (i < usage->cUsageIdentifier - 1)
                    len += strlenW(commaSpace);
            }
            str = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
            if (str)
            {
                for (i = 0, ptr = str; i < usage->cUsageIdentifier; i++)
                {
                    PCCRYPT_OID_INFO info =
                     CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY,
                     usage->rgpszUsageIdentifier[i],
                     CRYPT_ENHKEY_USAGE_OID_GROUP_ID);

                    if (info)
                    {
                        strcpyW(ptr, info->pwszName);
                        ptr += strlenW(info->pwszName);
                    }
                    else
                    {
                        LPCSTR src = usage->rgpszUsageIdentifier[i];

                        for (; *src; ptr++, src++)
                            *ptr = *src;
                        *ptr = 0;
                    }
                    if (i < usage->cUsageIdentifier - 1)
                    {
                        strcpyW(ptr, commaSpace);
                        ptr += strlenW(commaSpace);
                    }
                }
                *ptr = 0;
                SendMessageW(text, WM_SETTEXT, 0, (LPARAM)str);
                HeapFree(GetProcessHeap(), 0, str);
            }
            HeapFree(GetProcessHeap(), 0, usage);
        }
        else
        {
            WCHAR buf[MAX_STRING_LEN];

            LoadStringW(hInstance, IDS_ALLOWED_PURPOSE_NONE, buf,
             sizeof(buf) / sizeof(buf[0]));
            SendMessageW(text, WM_SETTEXT, 0, (LPARAM)buf);
        }
    }
    else
    {
        WCHAR buf[MAX_STRING_LEN];

        LoadStringW(hInstance, IDS_ALLOWED_PURPOSE_ALL, buf,
         sizeof(buf) / sizeof(buf[0]));
        SendMessageW(text, WM_SETTEXT, 0, (LPARAM)buf);
    }
}

static void cert_mgr_do_remove(HWND hwnd)
{
    int tabIndex = SendMessageW(GetDlgItem(hwnd, IDC_MGR_STORES),
     TCM_GETCURSEL, 0, 0);
    struct CertMgrData *data =
     (struct CertMgrData *)GetWindowLongPtrW(hwnd, DWLP_USER);

    if (tabIndex < data->nStores)
    {
        HWND lv = GetDlgItem(hwnd, IDC_MGR_CERTS);
        WCHAR warning[MAX_STRING_LEN], title[MAX_STRING_LEN];
        LPCWSTR pTitle;
        int warningID;

        if (SendMessageW(lv, LVM_GETSELECTEDCOUNT, 0, 0) > 1)
            warningID = data->stores[tabIndex].removePluralWarning;
        else
            warningID = data->stores[tabIndex].removeWarning;
        if (data->title)
            pTitle = data->title;
        else
        {
            LoadStringW(hInstance, IDS_CERT_MGR, title,
             sizeof(title) / sizeof(title[0]));
            pTitle = title;
        }
        LoadStringW(hInstance, warningID, warning,
         sizeof(warning) / sizeof(warning[0]));
        if (MessageBoxW(hwnd, warning, pTitle, MB_YESNO) == IDYES)
        {
            int selection = -1;

            do {
                selection = SendMessageW(lv, LVM_GETNEXTITEM, selection,
                 LVNI_SELECTED);
                if (selection >= 0)
                {
                    PCCERT_CONTEXT cert = cert_mgr_index_to_cert(hwnd,
                     selection);

                    CertDeleteCertificateFromStore(cert);
                }
            } while (selection >= 0);
            cert_mgr_clear_cert_selection(hwnd);
        }
    }
}

static void cert_mgr_do_export(HWND hwnd)
{
    HWND lv = GetDlgItem(hwnd, IDC_MGR_CERTS);
    int selectionCount = SendMessageW(lv, LVM_GETSELECTEDCOUNT, 0, 0);

    if (selectionCount == 1)
    {
        int selection = SendMessageW(lv, LVM_GETNEXTITEM, -1,
         LVNI_SELECTED);

        if (selection >= 0)
        {
            PCCERT_CONTEXT cert = cert_mgr_index_to_cert(hwnd, selection);

            if (cert)
            {
                CRYPTUI_WIZ_EXPORT_INFO info;

                info.dwSize = sizeof(info);
                info.pwszExportFileName = NULL;
                info.dwSubjectChoice = CRYPTUI_WIZ_EXPORT_CERT_CONTEXT;
                info.u.pCertContext = cert;
                info.cStores = 0;
                CryptUIWizExport(0, hwnd, NULL, &info, NULL);
            }
        }
    }
    else if (selectionCount > 1)
    {
        HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
         CERT_STORE_CREATE_NEW_FLAG, NULL);

        if (store)
        {
            CRYPTUI_WIZ_EXPORT_INFO info;
            int selection = -1;

            info.dwSize = sizeof(info);
            info.pwszExportFileName = NULL;
            info.dwSubjectChoice =
             CRYPTUI_WIZ_EXPORT_CERT_STORE_CERTIFICATES_ONLY;
            info.u.hCertStore = store;
            info.cStores = 0;
            do {
                selection = SendMessageW(lv, LVM_GETNEXTITEM, selection,
                 LVNI_SELECTED);
                if (selection >= 0)
                {
                    PCCERT_CONTEXT cert = cert_mgr_index_to_cert(hwnd,
                     selection);

                    CertAddCertificateContextToStore(store, cert,
                     CERT_STORE_ADD_ALWAYS, NULL);
                }
            } while (selection >= 0);
            CryptUIWizExport(0, hwnd, NULL, &info, NULL);
            CertCloseStore(store, 0);
        }
    }
}

static LRESULT CALLBACK cert_mgr_dlg_proc(HWND hwnd, UINT msg, WPARAM wp,
 LPARAM lp)
{
    struct CertMgrData *data;

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        PCCRYPTUI_CERT_MGR_STRUCT pCryptUICertMgr =
         (PCCRYPTUI_CERT_MGR_STRUCT)lp;
        HWND tab = GetDlgItem(hwnd, IDC_MGR_STORES);

        data = HeapAlloc(GetProcessHeap(), 0, sizeof(struct CertMgrData));
        if (!data)
            return 0;
        data->imageList = ImageList_Create(16, 16, ILC_COLOR4 | ILC_MASK, 2, 0);
        if (data->imageList)
        {
            HBITMAP bmp;
            COLORREF backColor = RGB(255, 0, 255);

            bmp = LoadBitmapW(hInstance, MAKEINTRESOURCEW(IDB_SMALL_ICONS));
            ImageList_AddMasked(data->imageList, bmp, backColor);
            DeleteObject(bmp);
            ImageList_SetBkColor(data->imageList, CLR_NONE);
            SendMessageW(GetDlgItem(hwnd, IDC_MGR_CERTS), LVM_SETIMAGELIST,
                         LVSIL_SMALL, (LPARAM)data->imageList);
        }
        SetWindowLongPtrW(hwnd, DWLP_USER, (LPARAM)data);
        data->title = pCryptUICertMgr->pwszTitle;

        initialize_purpose_selection(hwnd);
        add_cert_columns(hwnd);
        if (pCryptUICertMgr->pwszTitle)
            SendMessageW(hwnd, WM_SETTEXT, 0,
             (LPARAM)pCryptUICertMgr->pwszTitle);
        show_cert_stores(hwnd, pCryptUICertMgr->dwFlags, data);
        show_store_certs(hwnd, cert_mgr_index_to_store(tab, 0));
        break;
    }
    case WM_NOTIFY:
    {
        NMHDR *hdr = (NMHDR *)lp;

        switch (hdr->code)
        {
        case TCN_SELCHANGE:
            cert_mgr_clear_cert_selection(hwnd);
            break;
        case LVN_ITEMCHANGED:
        {
            NMITEMACTIVATE *nm;
            HWND lv = GetDlgItem(hwnd, IDC_MGR_CERTS);

            nm = (NMITEMACTIVATE*)lp;
            if (nm->uNewState & LVN_ITEMACTIVATE)
            {
                int numSelected = SendMessageW(lv, LVM_GETSELECTEDCOUNT, 0, 0);

                EnableWindow(GetDlgItem(hwnd, IDC_MGR_EXPORT), numSelected > 0);
                EnableWindow(GetDlgItem(hwnd, IDC_MGR_REMOVE), numSelected > 0);
                EnableWindow(GetDlgItem(hwnd, IDC_MGR_VIEW), numSelected == 1);
                if (numSelected == 1)
                    cert_mgr_show_cert_usages(hwnd, nm->iItem);
            }
            break;
        }
        case NM_DBLCLK:
            show_selected_cert(hwnd, ((NMITEMACTIVATE *)lp)->iItem);
            break;
        case LVN_KEYDOWN:
        {
            NMLVKEYDOWN *lvk = (NMLVKEYDOWN *)lp;

            if (lvk->wVKey == VK_DELETE)
                cert_mgr_do_remove(hwnd);
            break;
        }
        }
        break;
    }
    case WM_COMMAND:
        switch (wp)
        {
        case ((CBN_SELCHANGE << 16) | IDC_MGR_PURPOSE_SELECTION):
            cert_mgr_clear_cert_selection(hwnd);
            break;
        case IDC_MGR_IMPORT:
            if (CryptUIWizImport(0, hwnd, NULL, NULL,
             cert_mgr_current_store(hwnd)))
                refresh_store_certs(hwnd);
            break;
        case IDC_MGR_ADVANCED:
            if (DialogBoxW(hInstance, MAKEINTRESOURCEW(IDD_CERT_MGR_ADVANCED),
             hwnd, cert_mgr_advanced_dlg_proc) == IDOK)
            {
                HWND cb = GetDlgItem(hwnd, IDC_MGR_PURPOSE_SELECTION);
                int index, len;
                LPWSTR curString = NULL;

                index = SendMessageW(cb, CB_GETCURSEL, 0, 0);
                if (index >= 0)
                {
                    len = SendMessageW(cb, CB_GETLBTEXTLEN, index, 0);
                    curString = HeapAlloc(GetProcessHeap(), 0,
                     (len + 1) * sizeof(WCHAR));
                    SendMessageW(cb, CB_GETLBTEXT, index, (LPARAM)curString);
                }
                SendMessageW(cb, CB_RESETCONTENT, 0, 0);
                initialize_purpose_selection(hwnd);
                if (curString)
                {
                    index = SendMessageW(cb, CB_FINDSTRINGEXACT, -1,
                     (LPARAM)curString);
                    if (index >= 0)
                        SendMessageW(cb, CB_SETCURSEL, index, 0);
                    HeapFree(GetProcessHeap(), 0, curString);
                }
                refresh_store_certs(hwnd);
            }
            break;
        case IDC_MGR_VIEW:
        {
            HWND lv = GetDlgItem(hwnd, IDC_MGR_CERTS);
            int selection = SendMessageW(lv, LVM_GETNEXTITEM, -1,
             LVNI_SELECTED);

            if (selection >= 0)
                show_selected_cert(hwnd, selection);
            break;
        }
        case IDC_MGR_EXPORT:
            cert_mgr_do_export(hwnd);
            break;
        case IDC_MGR_REMOVE:
            cert_mgr_do_remove(hwnd);
            break;
        case IDCANCEL:
            free_certs(GetDlgItem(hwnd, IDC_MGR_CERTS));
            close_stores(GetDlgItem(hwnd, IDC_MGR_STORES));
            close_stores(GetDlgItem(hwnd, IDC_MGR_STORES));
            data = (struct CertMgrData *)GetWindowLongPtrW(hwnd, DWLP_USER);
            ImageList_Destroy(data->imageList);
            HeapFree(GetProcessHeap(), 0, data);
            EndDialog(hwnd, IDCANCEL);
            break;
        }
        break;
    }
    return 0;
}

/***********************************************************************
 *		CryptUIDlgCertMgr (CRYPTUI.@)
 */
BOOL WINAPI CryptUIDlgCertMgr(PCCRYPTUI_CERT_MGR_STRUCT pCryptUICertMgr)
{
    TRACE("(%p)\n", pCryptUICertMgr);

    if (pCryptUICertMgr->dwSize != sizeof(CRYPTUI_CERT_MGR_STRUCT))
    {
        WARN("unexpected size %d\n", pCryptUICertMgr->dwSize);
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_CERT_MGR),
     pCryptUICertMgr->hwndParent, cert_mgr_dlg_proc, (LPARAM)pCryptUICertMgr);
    return TRUE;
}

/* FIXME: real names are unknown, functions are undocumented */
typedef struct _CRYPTUI_ENUM_SYSTEM_STORE_ARGS
{
    DWORD dwFlags;
    void *pvSystemStoreLocationPara;
} CRYPTUI_ENUM_SYSTEM_STORE_ARGS, *PCRYPTUI_ENUM_SYSTEM_STORE_ARGS;

typedef struct _CRYPTUI_ENUM_DATA
{
    DWORD                           cStores;
    HCERTSTORE                     *rghStore;
    DWORD                           cEnumArgs;
    PCRYPTUI_ENUM_SYSTEM_STORE_ARGS rgEnumArgs;
} CRYPTUI_ENUM_DATA, *PCRYPTUI_ENUM_DATA;

typedef BOOL (WINAPI *PFN_SELECTED_STORE_CB)(HCERTSTORE store, HWND hwnd,
 void *pvArg);

/* Values for dwFlags */
#define CRYPTUI_ENABLE_SHOW_PHYSICAL_STORE 0x00000001

typedef struct _CRYPTUI_SELECTSTORE_INFO_A
{
    DWORD                 dwSize;
    HWND                  parent;
    DWORD                 dwFlags;
    LPSTR                 pszTitle;
    LPSTR                 pszText;
    CRYPTUI_ENUM_DATA    *pEnumData;
    PFN_SELECTED_STORE_CB pfnSelectedStoreCallback;
    void                 *pvArg;
} CRYPTUI_SELECTSTORE_INFO_A, *PCRYPTUI_SELECTSTORE_INFO_A;

typedef struct _CRYPTUI_SELECTSTORE_INFO_W
{
    DWORD                 dwSize;
    HWND                  parent;
    DWORD                 dwFlags;
    LPWSTR                pwszTitle;
    LPWSTR                pwszText;
    CRYPTUI_ENUM_DATA    *pEnumData;
    PFN_SELECTED_STORE_CB pfnSelectedStoreCallback;
    void                 *pvArg;
} CRYPTUI_SELECTSTORE_INFO_W, *PCRYPTUI_SELECTSTORE_INFO_W;

struct StoreInfo
{
    enum {
        StoreHandle,
        SystemStore
    } type;
    union {
        HCERTSTORE store;
        LPWSTR name;
    } DUMMYUNIONNAME;
};

static BOOL WINAPI enum_store_callback(const void *pvSystemStore,
 DWORD dwFlags, PCERT_SYSTEM_STORE_INFO pStoreInfo, void *pvReserved,
 void *pvArg)
{
    HWND tree = GetDlgItem(pvArg, IDC_STORE_LIST);
    TVINSERTSTRUCTW tvis;
    LPCWSTR localizedName;
    BOOL ret = TRUE;

    tvis.hParent = NULL;
    tvis.hInsertAfter = TVI_LAST;
    tvis.u.item.mask = TVIF_TEXT;
    if ((localizedName = CryptFindLocalizedName(pvSystemStore)))
    {
        struct StoreInfo *storeInfo = HeapAlloc(GetProcessHeap(), 0,
         sizeof(struct StoreInfo));

        if (storeInfo)
        {
            storeInfo->type = SystemStore;
            storeInfo->u.name = HeapAlloc(GetProcessHeap(), 0,
             (strlenW(pvSystemStore) + 1) * sizeof(WCHAR));
            if (storeInfo->u.name)
            {
                tvis.u.item.mask |= TVIF_PARAM;
                tvis.u.item.lParam = (LPARAM)storeInfo;
                strcpyW(storeInfo->u.name, pvSystemStore);
            }
            else
            {
                HeapFree(GetProcessHeap(), 0, storeInfo);
                ret = FALSE;
            }
        }
        else
            ret = FALSE;
        tvis.u.item.pszText = (LPWSTR)localizedName;
    }
    else
        tvis.u.item.pszText = (LPWSTR)pvSystemStore;
    /* FIXME: need a folder icon for the store too */
    if (ret)
        SendMessageW(tree, TVM_INSERTITEMW, 0, (LPARAM)&tvis);
    return ret;
}

static void enumerate_stores(HWND hwnd, CRYPTUI_ENUM_DATA *pEnumData)
{
    DWORD i;
    HWND tree = GetDlgItem(hwnd, IDC_STORE_LIST);

    for (i = 0; i < pEnumData->cEnumArgs; i++)
        CertEnumSystemStore(pEnumData->rgEnumArgs[i].dwFlags,
         pEnumData->rgEnumArgs[i].pvSystemStoreLocationPara,
         hwnd, enum_store_callback);
    for (i = 0; i < pEnumData->cStores; i++)
    {
        DWORD size;

        if (CertGetStoreProperty(pEnumData->rghStore[i],
         CERT_STORE_LOCALIZED_NAME_PROP_ID, NULL, &size))
        {
            LPWSTR name = HeapAlloc(GetProcessHeap(), 0, size);

            if (name)
            {
                if (CertGetStoreProperty(pEnumData->rghStore[i],
                 CERT_STORE_LOCALIZED_NAME_PROP_ID, name, &size))
                {
                    struct StoreInfo *storeInfo = HeapAlloc(GetProcessHeap(),
                     0, sizeof(struct StoreInfo));

                    if (storeInfo)
                    {
                        TVINSERTSTRUCTW tvis;

                        storeInfo->type = StoreHandle;
                        storeInfo->u.store = pEnumData->rghStore[i];
                        tvis.hParent = NULL;
                        tvis.hInsertAfter = TVI_LAST;
                        tvis.u.item.mask = TVIF_TEXT | TVIF_PARAM;
                        tvis.u.item.pszText = name;
                        tvis.u.item.lParam = (LPARAM)storeInfo;
                        SendMessageW(tree, TVM_INSERTITEMW, 0, (LPARAM)&tvis);
                    }
                }
                HeapFree(GetProcessHeap(), 0, name);
            }
        }
    }
}

static void free_store_info(HWND tree)
{
    HTREEITEM next = (HTREEITEM)SendMessageW(tree, TVM_GETNEXTITEM, TVGN_CHILD,
     (LPARAM)NULL);

    while (next)
    {
        TVITEMW item;

        memset(&item, 0, sizeof(item));
        item.mask = TVIF_HANDLE | TVIF_PARAM;
        item.hItem = next;
        SendMessageW(tree, TVM_GETITEMW, 0, (LPARAM)&item);
        if (item.lParam)
        {
            struct StoreInfo *storeInfo = (struct StoreInfo *)item.lParam;

            if (storeInfo->type == SystemStore)
                HeapFree(GetProcessHeap(), 0, storeInfo->u.name);
            HeapFree(GetProcessHeap(), 0, storeInfo);
        }
        next = (HTREEITEM)SendMessageW(tree, TVM_GETNEXTITEM, TVGN_NEXT,
         (LPARAM)next);
    }
}

static HCERTSTORE selected_item_to_store(HWND tree, HTREEITEM hItem)
{
    WCHAR buf[MAX_STRING_LEN];
    TVITEMW item;
    HCERTSTORE store;

    memset(&item, 0, sizeof(item));
    item.mask = TVIF_HANDLE | TVIF_PARAM | TVIF_TEXT;
    item.hItem = hItem;
    item.cchTextMax = sizeof(buf) / sizeof(buf[0]);
    item.pszText = buf;
    SendMessageW(tree, TVM_GETITEMW, 0, (LPARAM)&item);
    if (item.lParam)
    {
        struct StoreInfo *storeInfo = (struct StoreInfo *)item.lParam;

        if (storeInfo->type == StoreHandle)
            store = storeInfo->u.store;
        else
            store = CertOpenSystemStoreW(0, storeInfo->u.name);
    }
    else
    {
        /* It's implicitly a system store */
        store = CertOpenSystemStoreW(0, buf);
    }
    return store;
}

struct SelectStoreInfo
{
    PCRYPTUI_SELECTSTORE_INFO_W info;
    HCERTSTORE                  store;
};

static LRESULT CALLBACK select_store_dlg_proc(HWND hwnd, UINT msg, WPARAM wp,
 LPARAM lp)
{
    struct SelectStoreInfo *selectInfo;
    LRESULT ret = 0;

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        selectInfo = (struct SelectStoreInfo *)lp;
        SetWindowLongPtrW(hwnd, DWLP_USER, lp);
        if (selectInfo->info->pwszTitle)
            SendMessageW(hwnd, WM_SETTEXT, 0,
             (LPARAM)selectInfo->info->pwszTitle);
        if (selectInfo->info->pwszText)
            SendMessageW(GetDlgItem(hwnd, IDC_STORE_TEXT), WM_SETTEXT, 0,
             (LPARAM)selectInfo->info->pwszText);
        if (!(selectInfo->info->dwFlags & CRYPTUI_ENABLE_SHOW_PHYSICAL_STORE))
            ShowWindow(GetDlgItem(hwnd, IDC_SHOW_PHYSICAL_STORES), FALSE);
        enumerate_stores(hwnd, selectInfo->info->pEnumData);
        break;
    }
    case WM_COMMAND:
        switch (wp)
        {
        case IDOK:
        {
            HWND tree = GetDlgItem(hwnd, IDC_STORE_LIST);
            HTREEITEM selection = (HTREEITEM)SendMessageW(tree,
             TVM_GETNEXTITEM, TVGN_CARET, (LPARAM)NULL);

            selectInfo = (struct SelectStoreInfo *)GetWindowLongPtrW(hwnd,
             DWLP_USER);
            if (!selection)
            {
                WCHAR title[MAX_STRING_LEN], error[MAX_STRING_LEN], *pTitle;

                if (selectInfo->info->pwszTitle)
                    pTitle = selectInfo->info->pwszTitle;
                else
                {
                    LoadStringW(hInstance, IDS_SELECT_STORE_TITLE, title,
                     sizeof(title) / sizeof(title[0]));
                    pTitle = title;
                }
                LoadStringW(hInstance, IDS_SELECT_STORE, error,
                 sizeof(error) / sizeof(error[0]));
                MessageBoxW(hwnd, error, pTitle, MB_ICONEXCLAMATION | MB_OK);
            }
            else
            {
                HCERTSTORE store = selected_item_to_store(tree, selection);

                if (!selectInfo->info->pfnSelectedStoreCallback ||
                 selectInfo->info->pfnSelectedStoreCallback(store, hwnd,
                 selectInfo->info->pvArg))
                {
                    selectInfo->store = store;
                    free_store_info(tree);
                    EndDialog(hwnd, IDOK);
                }
                else
                    CertCloseStore(store, 0);
            }
            ret = TRUE;
            break;
        }
        case IDCANCEL:
            free_store_info(GetDlgItem(hwnd, IDC_STORE_LIST));
            EndDialog(hwnd, IDCANCEL);
            ret = TRUE;
            break;
        }
        break;
    }
    return ret;
}

/***********************************************************************
 *		CryptUIDlgSelectStoreW (CRYPTUI.@)
 */
HCERTSTORE WINAPI CryptUIDlgSelectStoreW(PCRYPTUI_SELECTSTORE_INFO_W info)
{
    struct SelectStoreInfo selectInfo = { info, NULL };

    TRACE("(%p)\n", info);

    if (info->dwSize != sizeof(CRYPTUI_SELECTSTORE_INFO_W))
    {
        WARN("unexpected size %d\n", info->dwSize);
        SetLastError(E_INVALIDARG);
        return NULL;
    }
    DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_SELECT_STORE), info->parent,
     select_store_dlg_proc, (LPARAM)&selectInfo);
    return selectInfo.store;
}

/***********************************************************************
 *		CryptUIDlgSelectStoreA (CRYPTUI.@)
 */
HCERTSTORE WINAPI CryptUIDlgSelectStoreA(PCRYPTUI_SELECTSTORE_INFO_A info)
{
    CRYPTUI_SELECTSTORE_INFO_W infoW;
    HCERTSTORE ret;
    int len;

    TRACE("(%p)\n", info);

    if (info->dwSize != sizeof(CRYPTUI_SELECTSTORE_INFO_A))
    {
        WARN("unexpected size %d\n", info->dwSize);
        SetLastError(E_INVALIDARG);
        return NULL;
    }
    memcpy(&infoW, &info, sizeof(info));
    if (info->pszTitle)
    {
        len = MultiByteToWideChar(CP_ACP, 0, info->pszTitle, -1, NULL, 0);
        infoW.pwszTitle = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, info->pszTitle, -1, infoW.pwszTitle,
         len);
    }
    if (info->pszText)
    {
        len = MultiByteToWideChar(CP_ACP, 0, info->pszText, -1, NULL, 0);
        infoW.pwszText = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, info->pszText, -1, infoW.pwszText, len);
    }
    ret = CryptUIDlgSelectStoreW(&infoW);
    HeapFree(GetProcessHeap(), 0, infoW.pwszText);
    HeapFree(GetProcessHeap(), 0, infoW.pwszTitle);
    return ret;
}

/***********************************************************************
 *		CryptUIDlgViewCertificateA (CRYPTUI.@)
 */
BOOL WINAPI CryptUIDlgViewCertificateA(
 PCCRYPTUI_VIEWCERTIFICATE_STRUCTA pCertViewInfo, BOOL *pfPropertiesChanged)
{
    CRYPTUI_VIEWCERTIFICATE_STRUCTW viewInfo;
    LPWSTR title = NULL;
    BOOL ret;

    TRACE("(%p, %p)\n", pCertViewInfo, pfPropertiesChanged);

    memcpy(&viewInfo, pCertViewInfo, sizeof(viewInfo));
    if (pCertViewInfo->szTitle)
    {
        int len = MultiByteToWideChar(CP_ACP, 0, pCertViewInfo->szTitle, -1,
         NULL, 0);

        title = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (title)
        {
            MultiByteToWideChar(CP_ACP, 0, pCertViewInfo->szTitle, -1, title,
             len);
            viewInfo.szTitle = title;
        }
        else
        {
            ret = FALSE;
            goto error;
        }
    }
    if (pCertViewInfo->cPropSheetPages)
    {
        FIXME("ignoring additional prop sheet pages\n");
        viewInfo.cPropSheetPages = 0;
    }
    ret = CryptUIDlgViewCertificateW(&viewInfo, pfPropertiesChanged);
    HeapFree(GetProcessHeap(), 0, title);
error:
    return ret;
}

struct ReadStringStruct
{
    LPCWSTR buf;
    LONG pos;
    LONG len;
};

static DWORD CALLBACK read_text_callback(DWORD_PTR dwCookie, LPBYTE buf,
 LONG cb, LONG *pcb)
{
    struct ReadStringStruct *string = (struct ReadStringStruct *)dwCookie;
    LONG cch = min(cb / sizeof(WCHAR), string->len - string->pos);

    TRACE("(%p, %p, %d, %p)\n", string, buf, cb, pcb);

    memmove(buf, string->buf + string->pos, cch * sizeof(WCHAR));
    string->pos += cch;
    *pcb = cch * sizeof(WCHAR);
    return 0;
}

static void add_unformatted_text_to_control(HWND hwnd, LPCWSTR text, LONG len)
{
    struct ReadStringStruct string;
    EDITSTREAM editstream;

    TRACE("(%p, %s)\n", hwnd, debugstr_wn(text, len));

    string.buf = text;
    string.pos = 0;
    string.len = len;
    editstream.dwCookie = (DWORD_PTR)&string;
    editstream.dwError = 0;
    editstream.pfnCallback = read_text_callback;
    SendMessageW(hwnd, EM_STREAMIN, SF_TEXT | SFF_SELECTION | SF_UNICODE,
     (LPARAM)&editstream);
}

static void add_string_resource_to_control(HWND hwnd, int id)
{
    LPWSTR str;
    LONG len;

    len = LoadStringW(hInstance, id, (LPWSTR)&str, 0);
    add_unformatted_text_to_control(hwnd, str, len);
}

static void add_text_with_paraformat_to_control(HWND hwnd, LPCWSTR text,
 LONG len, const PARAFORMAT2 *fmt)
{
    add_unformatted_text_to_control(hwnd, text, len);
    SendMessageW(hwnd, EM_SETPARAFORMAT, 0, (LPARAM)fmt);
}

static void add_string_resource_with_paraformat_to_control(HWND hwnd, int id,
 const PARAFORMAT2 *fmt)
{
    LPWSTR str;
    LONG len;

    len = LoadStringW(hInstance, id, (LPWSTR)&str, 0);
    add_text_with_paraformat_to_control(hwnd, str, len, fmt);
}

static LPWSTR get_cert_name_string(PCCERT_CONTEXT pCertContext, DWORD dwType,
 DWORD dwFlags)
{
    LPWSTR buf = NULL;
    DWORD len;

    len = CertGetNameStringW(pCertContext, dwType, dwFlags, NULL, NULL, 0);
    if (len)
    {
        buf = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (buf)
            CertGetNameStringW(pCertContext, dwType, dwFlags, NULL, buf, len);
    }
    return buf;
}

static void add_cert_string_to_control(HWND hwnd, PCCERT_CONTEXT pCertContext,
 DWORD dwType, DWORD dwFlags)
{
    LPWSTR name = get_cert_name_string(pCertContext, dwType, dwFlags);

    if (name)
    {
        /* Don't include NULL-terminator in output */
        DWORD len = lstrlenW(name);

        add_unformatted_text_to_control(hwnd, name, len);
        HeapFree(GetProcessHeap(), 0, name);
    }
}

static void add_icon_to_control(HWND hwnd, int id)
{
    HRESULT hr;
    LPRICHEDITOLE richEditOle = NULL;
    LPOLEOBJECT object = NULL;
    CLSID clsid;
    LPOLECACHE oleCache = NULL;
    FORMATETC formatEtc;
    DWORD conn;
    LPDATAOBJECT dataObject = NULL;
    HBITMAP bitmap = NULL;
    RECT rect;
    STGMEDIUM stgm;
    LPOLECLIENTSITE clientSite = NULL;
    REOBJECT reObject;

    TRACE("(%p, %d)\n", hwnd, id);

    SendMessageW(hwnd, EM_GETOLEINTERFACE, 0, (LPARAM)&richEditOle);
    if (!richEditOle)
        goto end;
    hr = OleCreateDefaultHandler(&CLSID_NULL, NULL, &IID_IOleObject,
     (void**)&object);
    if (FAILED(hr))
        goto end;
    hr = IOleObject_GetUserClassID(object, &clsid);
    if (FAILED(hr))
        goto end;
    hr = IOleObject_QueryInterface(object, &IID_IOleCache, (void**)&oleCache);
    if (FAILED(hr))
        goto end;
    formatEtc.cfFormat = CF_BITMAP;
    formatEtc.ptd = NULL;
    formatEtc.dwAspect = DVASPECT_CONTENT;
    formatEtc.lindex = -1;
    formatEtc.tymed = TYMED_GDI;
    hr = IOleCache_Cache(oleCache, &formatEtc, 0, &conn);
    if (FAILED(hr))
        goto end;
    hr = IOleObject_QueryInterface(object, &IID_IDataObject,
     (void**)&dataObject);
    if (FAILED(hr))
        goto end;
    hr = IRichEditOle_GetClientSite(richEditOle, &clientSite);
    if (FAILED(hr))
        goto end;
    bitmap = LoadImageW(hInstance, MAKEINTRESOURCEW(id), IMAGE_BITMAP, 0, 0,
     LR_DEFAULTSIZE | LR_LOADTRANSPARENT);
    if (!bitmap)
        goto end;
    rect.left = rect.top = 0;
    rect.right = GetSystemMetrics(SM_CXICON);
    rect.bottom = GetSystemMetrics(SM_CYICON);
    stgm.tymed = TYMED_GDI;
    stgm.u.hBitmap = bitmap;
    stgm.pUnkForRelease = NULL;
    hr = IDataObject_SetData(dataObject, &formatEtc, &stgm, TRUE);
    if (FAILED(hr))
        goto end;

    reObject.cbStruct = sizeof(reObject);
    reObject.cp = REO_CP_SELECTION;
    reObject.clsid = clsid;
    reObject.poleobj = object;
    reObject.pstg = NULL;
    reObject.polesite = clientSite;
    reObject.sizel.cx = reObject.sizel.cy = 0;
    reObject.dvaspect = DVASPECT_CONTENT;
    reObject.dwFlags = 0;
    reObject.dwUser = 0;

    IRichEditOle_InsertObject(richEditOle, &reObject);

end:
    if (clientSite)
        IOleClientSite_Release(clientSite);
    if (dataObject)
        IDataObject_Release(dataObject);
    if (oleCache)
        IOleCache_Release(oleCache);
    if (object)
        IOleObject_Release(object);
    if (richEditOle)
        IRichEditOle_Release(richEditOle);
}

#define MY_INDENT 200

static void add_oid_text_to_control(HWND hwnd, char *oid)
{
    WCHAR nl = '\n';
    PCCRYPT_OID_INFO oidInfo = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY, oid, 0);
    PARAFORMAT2 parFmt;

    parFmt.cbSize = sizeof(parFmt);
    parFmt.dwMask = PFM_STARTINDENT;
    parFmt.dxStartIndent = MY_INDENT * 3;
    if (oidInfo)
    {
        add_text_with_paraformat_to_control(hwnd, oidInfo->pwszName,
         lstrlenW(oidInfo->pwszName), &parFmt);
        add_unformatted_text_to_control(hwnd, &nl, 1);
    }
}

struct OIDToString
{
    LPCSTR oid;
    int    id;
};

/* The following list MUST be lexicographically sorted by OID */
static struct OIDToString oidMap[] = {
 /* 1.3.6.1.4.1.311.10.3.1 */
 { szOID_KP_CTL_USAGE_SIGNING, IDS_PURPOSE_CTL_USAGE_SIGNING },
 /* 1.3.6.1.4.1.311.10.3.4 */
 { szOID_KP_EFS, IDS_PURPOSE_EFS },
 /* 1.3.6.1.4.1.311.10.3.4.1 */
 { szOID_EFS_RECOVERY, IDS_PURPOSE_EFS_RECOVERY },
 /* 1.3.6.1.4.1.311.10.3.5 */
 { szOID_WHQL_CRYPTO, IDS_PURPOSE_WHQL },
 /* 1.3.6.1.4.1.311.10.3.6 */
 { szOID_NT5_CRYPTO, IDS_PURPOSE_NT5 },
 /* 1.3.6.1.4.1.311.10.3.7 */
 { szOID_OEM_WHQL_CRYPTO, IDS_PURPOSE_OEM_WHQL },
 /* 1.3.6.1.4.1.311.10.3.8 */
 { szOID_EMBEDDED_NT_CRYPTO, IDS_PURPOSE_EMBEDDED_NT },
 /* 1.3.6.1.4.1.311.10.3.9 */
 { szOID_ROOT_LIST_SIGNER, IDS_PURPOSE_ROOT_LIST_SIGNER },
 /* 1.3.6.1.4.1.311.10.3.10 */
 { szOID_KP_QUALIFIED_SUBORDINATION, IDS_PURPOSE_QUALIFIED_SUBORDINATION },
 /* 1.3.6.1.4.1.311.10.3.11 */
 { szOID_KP_KEY_RECOVERY, IDS_PURPOSE_KEY_RECOVERY },
 /* 1.3.6.1.4.1.311.10.3.12 */
 { szOID_KP_DOCUMENT_SIGNING, IDS_PURPOSE_DOCUMENT_SIGNING },
 /* 1.3.6.1.4.1.311.10.3.13 */
 { szOID_KP_LIFETIME_SIGNING, IDS_PURPOSE_LIFETIME_SIGNING },
 /* 1.3.6.1.4.1.311.10.5.1 */
 { szOID_DRM, IDS_PURPOSE_DRM },
 /* 1.3.6.1.4.1.311.10.6.1 */
 { szOID_LICENSES, IDS_PURPOSE_LICENSES },
 /* 1.3.6.1.4.1.311.10.6.2 */
 { szOID_LICENSE_SERVER, IDS_PURPOSE_LICENSE_SERVER },
 /* 1.3.6.1.4.1.311.20.2.1 */
 { szOID_ENROLLMENT_AGENT, IDS_PURPOSE_ENROLLMENT_AGENT },
 /* 1.3.6.1.4.1.311.20.2.2 */
 { szOID_KP_SMARTCARD_LOGON, IDS_PURPOSE_SMARTCARD_LOGON },
 /* 1.3.6.1.4.1.311.21.5 */
 { szOID_KP_CA_EXCHANGE, IDS_PURPOSE_CA_EXCHANGE },
 /* 1.3.6.1.4.1.311.21.6 */
 { szOID_KP_KEY_RECOVERY_AGENT, IDS_PURPOSE_KEY_RECOVERY_AGENT },
 /* 1.3.6.1.4.1.311.21.19 */
 { szOID_DS_EMAIL_REPLICATION, IDS_PURPOSE_DS_EMAIL_REPLICATION },
 /* 1.3.6.1.5.5.7.3.1 */
 { szOID_PKIX_KP_SERVER_AUTH, IDS_PURPOSE_SERVER_AUTH },
 /* 1.3.6.1.5.5.7.3.2 */
 { szOID_PKIX_KP_CLIENT_AUTH, IDS_PURPOSE_CLIENT_AUTH },
 /* 1.3.6.1.5.5.7.3.3 */
 { szOID_PKIX_KP_CODE_SIGNING, IDS_PURPOSE_CODE_SIGNING },
 /* 1.3.6.1.5.5.7.3.4 */
 { szOID_PKIX_KP_EMAIL_PROTECTION, IDS_PURPOSE_EMAIL_PROTECTION },
 /* 1.3.6.1.5.5.7.3.5 */
 { szOID_PKIX_KP_IPSEC_END_SYSTEM, IDS_PURPOSE_IPSEC },
 /* 1.3.6.1.5.5.7.3.6 */
 { szOID_PKIX_KP_IPSEC_TUNNEL, IDS_PURPOSE_IPSEC },
 /* 1.3.6.1.5.5.7.3.7 */
 { szOID_PKIX_KP_IPSEC_USER, IDS_PURPOSE_IPSEC },
 /* 1.3.6.1.5.5.7.3.8 */
 { szOID_PKIX_KP_TIMESTAMP_SIGNING, IDS_PURPOSE_TIMESTAMP_SIGNING },
};

static struct OIDToString *findSupportedOID(LPCSTR oid)
{
    int indexHigh = sizeof(oidMap) / sizeof(oidMap[0]) - 1, indexLow = 0, i;
    struct OIDToString *ret = NULL;

    for (i = (indexLow + indexHigh) / 2; !ret && indexLow <= indexHigh;
     i = (indexLow + indexHigh) / 2)
    {
        int cmp;

        cmp = strcmp(oid, oidMap[i].oid);
        if (!cmp)
            ret = &oidMap[i];
        else if (cmp > 0)
            indexLow = i + 1;
        else
            indexHigh = i - 1;
    }
    return ret;
}

static void add_local_oid_text_to_control(HWND text, LPCSTR oid)
{
    struct OIDToString *entry;
    WCHAR nl = '\n';
    PARAFORMAT2 parFmt;

    parFmt.cbSize = sizeof(parFmt);
    parFmt.dwMask = PFM_STARTINDENT;
    parFmt.dxStartIndent = MY_INDENT * 3;
    if ((entry = findSupportedOID(oid)))
    {
        WCHAR *str, *linebreak, *ptr;
        BOOL multiline = FALSE;
        int len;

        len = LoadStringW(hInstance, entry->id, (LPWSTR)&str, 0);
        ptr = str;
        do {
            if ((linebreak = memchrW(ptr, '\n', len)))
            {
                WCHAR copy[MAX_STRING_LEN];

                multiline = TRUE;
                /* The source string contains a newline, which the richedit
                 * control won't find since it's interpreted as a paragraph
                 * break.  Therefore copy up to the newline.  lstrcpynW always
                 * NULL-terminates, so pass one more than the length of the
                 * source line so the copy includes the entire line and the
                 * NULL-terminator.
                 */
                lstrcpynW(copy, ptr, linebreak - ptr + 1);
                add_text_with_paraformat_to_control(text, copy,
                 linebreak - ptr, &parFmt);
                ptr = linebreak + 1;
                add_unformatted_text_to_control(text, &nl, 1);
            }
            else if (multiline && *ptr)
            {
                /* Add the last line */
                add_text_with_paraformat_to_control(text, ptr,
                 len - (ptr - str), &parFmt);
                add_unformatted_text_to_control(text, &nl, 1);
            }
        } while (linebreak);
        if (!multiline)
        {
            add_text_with_paraformat_to_control(text, str, len, &parFmt);
            add_unformatted_text_to_control(text, &nl, 1);
        }
    }
    else
    {
        WCHAR *oidW = HeapAlloc(GetProcessHeap(), 0,
         (strlen(oid) + 1) * sizeof(WCHAR));

        if (oidW)
        {
            LPCSTR src;
            WCHAR *dst;

            for (src = oid, dst = oidW; *src; src++, dst++)
                *dst = *src;
            *dst = 0;
            add_text_with_paraformat_to_control(text, oidW, lstrlenW(oidW),
             &parFmt);
            add_unformatted_text_to_control(text, &nl, 1);
            HeapFree(GetProcessHeap(), 0, oidW);
        }
    }
}

static void display_app_usages(HWND text, PCCERT_CONTEXT cert,
 BOOL *anyUsageAdded)
{
    static char any_app_policy[] = szOID_ANY_APPLICATION_POLICY;
    WCHAR nl = '\n';
    CHARFORMATW charFmt;
    PCERT_EXTENSION policyExt;
    if (!*anyUsageAdded)
    {
        PARAFORMAT2 parFmt;

        parFmt.cbSize = sizeof(parFmt);
        parFmt.dwMask = PFM_STARTINDENT;
        parFmt.dxStartIndent = MY_INDENT;
        add_string_resource_with_paraformat_to_control(text,
         IDS_CERT_INFO_PURPOSES, &parFmt);
        add_unformatted_text_to_control(text, &nl, 1);
        *anyUsageAdded = TRUE;
    }
    memset(&charFmt, 0, sizeof(charFmt));
    charFmt.cbSize = sizeof(charFmt);
    charFmt.dwMask = CFM_BOLD;
    charFmt.dwEffects = 0;
    SendMessageW(text, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&charFmt);
    if ((policyExt = CertFindExtension(szOID_APPLICATION_CERT_POLICIES,
     cert->pCertInfo->cExtension, cert->pCertInfo->rgExtension)))
    {
        CERT_POLICIES_INFO *policies;
        DWORD size;

        if (CryptDecodeObjectEx(X509_ASN_ENCODING, X509_CERT_POLICIES,
         policyExt->Value.pbData, policyExt->Value.cbData,
         CRYPT_DECODE_ALLOC_FLAG, NULL, &policies, &size))
        {
            DWORD i;

            for (i = 0; i < policies->cPolicyInfo; i++)
            {
                DWORD j;

                for (j = 0; j < policies->rgPolicyInfo[i].cPolicyQualifier; j++)
                    add_local_oid_text_to_control(text,
                     policies->rgPolicyInfo[i].rgPolicyQualifier[j].
                     pszPolicyQualifierId);
            }
            LocalFree(policies);
        }
    }
    else
        add_oid_text_to_control(text, any_app_policy);
}

static BOOL display_cert_usages(HWND text, PCCERT_CONTEXT cert,
 BOOL *anyUsageAdded)
{
    WCHAR nl = '\n';
    DWORD size;
    BOOL badUsages = FALSE;

    if (CertGetEnhancedKeyUsage(cert, 0, NULL, &size))
    {
        CHARFORMATW charFmt;
        static char any_cert_policy[] = szOID_ANY_CERT_POLICY;
        PCERT_ENHKEY_USAGE usage = HeapAlloc(GetProcessHeap(), 0, size);

        if (usage)
        {
            if (CertGetEnhancedKeyUsage(cert, 0, usage, &size))
            {
                DWORD i;

                if (!*anyUsageAdded)
                {
                    PARAFORMAT2 parFmt;

                    parFmt.cbSize = sizeof(parFmt);
                    parFmt.dwMask = PFM_STARTINDENT;
                    parFmt.dxStartIndent = MY_INDENT;
                    add_string_resource_with_paraformat_to_control(text,
                     IDS_CERT_INFO_PURPOSES, &parFmt);
                    add_unformatted_text_to_control(text, &nl, 1);
                    *anyUsageAdded = TRUE;
                }
                memset(&charFmt, 0, sizeof(charFmt));
                charFmt.cbSize = sizeof(charFmt);
                charFmt.dwMask = CFM_BOLD;
                charFmt.dwEffects = 0;
                SendMessageW(text, EM_SETCHARFORMAT, SCF_SELECTION,
                 (LPARAM)&charFmt);
                if (!usage->cUsageIdentifier)
                    add_oid_text_to_control(text, any_cert_policy);
                else
                    for (i = 0; i < usage->cUsageIdentifier; i++)
                        add_local_oid_text_to_control(text,
                         usage->rgpszUsageIdentifier[i]);
            }
            else
                badUsages = TRUE;
            HeapFree(GetProcessHeap(), 0, usage);
        }
        else
            badUsages = TRUE;
    }
    else
        badUsages = TRUE;
    return badUsages;
}

static void set_policy_text(HWND text,
 PCCRYPTUI_VIEWCERTIFICATE_STRUCTW pCertViewInfo)
{
    BOOL includeCertUsages = FALSE, includeAppUsages = FALSE;
    BOOL badUsages = FALSE, anyUsageAdded = FALSE;

    if (pCertViewInfo->cPurposes)
    {
        DWORD i;

        for (i = 0; i < pCertViewInfo->cPurposes; i++)
        {
            if (!strcmp(pCertViewInfo->rgszPurposes[i], szOID_ANY_CERT_POLICY))
                includeCertUsages = TRUE;
            else if (!strcmp(pCertViewInfo->rgszPurposes[i],
             szOID_ANY_APPLICATION_POLICY))
                includeAppUsages = TRUE;
            else
                badUsages = TRUE;
        }
    }
    else
        includeAppUsages = includeCertUsages = TRUE;
    if (includeAppUsages)
        display_app_usages(text, pCertViewInfo->pCertContext, &anyUsageAdded);
    if (includeCertUsages)
        badUsages = display_cert_usages(text, pCertViewInfo->pCertContext,
         &anyUsageAdded);
    if (badUsages)
    {
        PARAFORMAT2 parFmt;

        parFmt.cbSize = sizeof(parFmt);
        parFmt.dwMask = PFM_STARTINDENT;
        parFmt.dxStartIndent = MY_INDENT;
        add_string_resource_with_paraformat_to_control(text,
         IDS_CERT_INFO_BAD_PURPOSES, &parFmt);
    }
}

static CRYPT_OBJID_BLOB *find_policy_qualifier(CERT_POLICIES_INFO *policies,
 LPCSTR policyOid)
{
    CRYPT_OBJID_BLOB *ret = NULL;
    DWORD i;

    for (i = 0; !ret && i < policies->cPolicyInfo; i++)
    {
        DWORD j;

        for (j = 0; !ret && j < policies->rgPolicyInfo[i].cPolicyQualifier; j++)
            if (!strcmp(policies->rgPolicyInfo[i].rgPolicyQualifier[j].
             pszPolicyQualifierId, policyOid))
                ret = &policies->rgPolicyInfo[i].rgPolicyQualifier[j].
                 Qualifier;
    }
    return ret;
}

static WCHAR *get_cps_str_from_qualifier(const CRYPT_OBJID_BLOB *qualifier)
{
    LPWSTR qualifierStr = NULL;
    CERT_NAME_VALUE *qualifierValue;
    DWORD size;

    if (CryptDecodeObjectEx(X509_ASN_ENCODING, X509_NAME_VALUE,
     qualifier->pbData, qualifier->cbData, CRYPT_DECODE_ALLOC_FLAG, NULL,
     &qualifierValue, &size))
    {
        size = CertRDNValueToStrW(qualifierValue->dwValueType,
         &qualifierValue->Value, NULL, 0);
        qualifierStr = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
        if (qualifierStr)
            CertRDNValueToStrW(qualifierValue->dwValueType,
             &qualifierValue->Value, qualifierStr, size);
        LocalFree(qualifierValue);
    }
    return qualifierStr;
}

static WCHAR *get_user_notice_from_qualifier(const CRYPT_OBJID_BLOB *qualifier)
{
    LPWSTR str = NULL;
    CERT_POLICY_QUALIFIER_USER_NOTICE *qualifierValue;
    DWORD size;

    if (CryptDecodeObjectEx(X509_ASN_ENCODING,
     X509_PKIX_POLICY_QUALIFIER_USERNOTICE,
     qualifier->pbData, qualifier->cbData, CRYPT_DECODE_ALLOC_FLAG, NULL,
     &qualifierValue, &size))
    {
        str = HeapAlloc(GetProcessHeap(), 0,
         (strlenW(qualifierValue->pszDisplayText) + 1) * sizeof(WCHAR));
        if (str)
            strcpyW(str, qualifierValue->pszDisplayText);
        LocalFree(qualifierValue);
    }
    return str;
}

struct IssuerStatement
{
    LPWSTR cps;
    LPWSTR userNotice;
};

static void set_issuer_statement(HWND hwnd,
 PCCRYPTUI_VIEWCERTIFICATE_STRUCTW pCertViewInfo)
{
    PCERT_EXTENSION policyExt;

    if (!(pCertViewInfo->dwFlags & CRYPTUI_DISABLE_ISSUERSTATEMENT) &&
     (policyExt = CertFindExtension(szOID_CERT_POLICIES,
     pCertViewInfo->pCertContext->pCertInfo->cExtension,
     pCertViewInfo->pCertContext->pCertInfo->rgExtension)))
    {
        CERT_POLICIES_INFO *policies;
        DWORD size;

        if (CryptDecodeObjectEx(X509_ASN_ENCODING, policyExt->pszObjId,
         policyExt->Value.pbData, policyExt->Value.cbData,
         CRYPT_DECODE_ALLOC_FLAG, NULL, &policies, &size))
        {
            CRYPT_OBJID_BLOB *qualifier;
            LPWSTR cps = NULL, userNotice = NULL;

            if ((qualifier = find_policy_qualifier(policies,
             szOID_PKIX_POLICY_QUALIFIER_CPS)))
                cps = get_cps_str_from_qualifier(qualifier);
            if ((qualifier = find_policy_qualifier(policies,
             szOID_PKIX_POLICY_QUALIFIER_USERNOTICE)))
                userNotice = get_user_notice_from_qualifier(qualifier);
            if (cps || userNotice)
            {
                struct IssuerStatement *issuerStatement =
                 HeapAlloc(GetProcessHeap(), 0, sizeof(struct IssuerStatement));

                if (issuerStatement)
                {
                    issuerStatement->cps = cps;
                    issuerStatement->userNotice = userNotice;
                    EnableWindow(GetDlgItem(hwnd, IDC_ISSUERSTATEMENT), TRUE);
                    SetWindowLongPtrW(hwnd, DWLP_USER,
                     (ULONG_PTR)issuerStatement);
                }
            }
            LocalFree(policies);
        }
    }
}

static void set_cert_info(HWND hwnd,
 PCCRYPTUI_VIEWCERTIFICATE_STRUCTW pCertViewInfo)
{
    CHARFORMATW charFmt;
    PARAFORMAT2 parFmt;
    HWND icon = GetDlgItem(hwnd, IDC_CERTIFICATE_ICON);
    HWND text = GetDlgItem(hwnd, IDC_CERTIFICATE_INFO);
    CRYPT_PROVIDER_SGNR *provSigner = WTHelperGetProvSignerFromChain(
     (CRYPT_PROVIDER_DATA *)pCertViewInfo->u.pCryptProviderData,
     pCertViewInfo->idxSigner, pCertViewInfo->fCounterSigner,
     pCertViewInfo->idxCounterSigner);
    CRYPT_PROVIDER_CERT *root =
     &provSigner->pasCertChain[provSigner->csCertChain - 1];

    if (!provSigner->pChainContext ||
     (provSigner->pChainContext->TrustStatus.dwErrorStatus &
     CERT_TRUST_IS_PARTIAL_CHAIN))
        add_icon_to_control(icon, IDB_CERT_WARNING);
    else if (!root->fTrustedRoot)
        add_icon_to_control(icon, IDB_CERT_ERROR);
    else
        add_icon_to_control(icon, IDB_CERT);

    memset(&charFmt, 0, sizeof(charFmt));
    charFmt.cbSize = sizeof(charFmt);
    charFmt.dwMask = CFM_BOLD;
    charFmt.dwEffects = CFE_BOLD;
    SendMessageW(text, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&charFmt);
    /* FIXME: vertically center text */
    parFmt.cbSize = sizeof(parFmt);
    parFmt.dwMask = PFM_STARTINDENT;
    parFmt.dxStartIndent = MY_INDENT;
    add_string_resource_with_paraformat_to_control(text,
     IDS_CERTIFICATEINFORMATION, &parFmt);

    text = GetDlgItem(hwnd, IDC_CERTIFICATE_STATUS);
    SendMessageW(text, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&charFmt);
    if (provSigner->dwError == TRUST_E_CERT_SIGNATURE)
        add_string_resource_with_paraformat_to_control(text,
         IDS_CERT_INFO_BAD_SIG, &parFmt);
    else if (!provSigner->pChainContext ||
     (provSigner->pChainContext->TrustStatus.dwErrorStatus &
     CERT_TRUST_IS_PARTIAL_CHAIN))
        add_string_resource_with_paraformat_to_control(text,
         IDS_CERT_INFO_PARTIAL_CHAIN, &parFmt);
    else if (!root->fTrustedRoot)
    {
        if (provSigner->csCertChain == 1 && root->fSelfSigned)
            add_string_resource_with_paraformat_to_control(text,
             IDS_CERT_INFO_UNTRUSTED_CA, &parFmt);
        else
            add_string_resource_with_paraformat_to_control(text,
             IDS_CERT_INFO_UNTRUSTED_ROOT, &parFmt);
    }
    else
    {
        set_policy_text(text, pCertViewInfo);
        set_issuer_statement(hwnd, pCertViewInfo);
    }
}

static void set_cert_name_string(HWND hwnd, PCCERT_CONTEXT cert,
 DWORD nameFlags, int heading)
{
    WCHAR nl = '\n';
    HWND text = GetDlgItem(hwnd, IDC_CERTIFICATE_NAMES);
    CHARFORMATW charFmt;
    PARAFORMAT2 parFmt;

    memset(&charFmt, 0, sizeof(charFmt));
    charFmt.cbSize = sizeof(charFmt);
    charFmt.dwMask = CFM_BOLD;
    charFmt.dwEffects = CFE_BOLD;
    SendMessageW(text, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&charFmt);
    parFmt.cbSize = sizeof(parFmt);
    parFmt.dwMask = PFM_STARTINDENT;
    parFmt.dxStartIndent = MY_INDENT * 3;
    add_string_resource_with_paraformat_to_control(text, heading, &parFmt);
    charFmt.dwEffects = 0;
    SendMessageW(text, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&charFmt);
    add_cert_string_to_control(text, cert, CERT_NAME_SIMPLE_DISPLAY_TYPE,
     nameFlags);
    add_unformatted_text_to_control(text, &nl, 1);
    add_unformatted_text_to_control(text, &nl, 1);
    add_unformatted_text_to_control(text, &nl, 1);

}

static void add_date_string_to_control(HWND hwnd, const FILETIME *fileTime)
{
    WCHAR dateFmt[80]; /* sufficient for all versions of LOCALE_SSHORTDATE */
    WCHAR date[80];
    SYSTEMTIME sysTime;

    GetLocaleInfoW(LOCALE_SYSTEM_DEFAULT, LOCALE_SSHORTDATE, dateFmt,
     sizeof(dateFmt) / sizeof(dateFmt[0]));
    FileTimeToSystemTime(fileTime, &sysTime);
    GetDateFormatW(LOCALE_SYSTEM_DEFAULT, 0, &sysTime, dateFmt, date,
     sizeof(date) / sizeof(date[0]));
    add_unformatted_text_to_control(hwnd, date, lstrlenW(date));
}

static void set_cert_validity_period(HWND hwnd, PCCERT_CONTEXT cert)
{
    WCHAR nl = '\n';
    HWND text = GetDlgItem(hwnd, IDC_CERTIFICATE_NAMES);
    CHARFORMATW charFmt;
    PARAFORMAT2 parFmt;

    memset(&charFmt, 0, sizeof(charFmt));
    charFmt.cbSize = sizeof(charFmt);
    charFmt.dwMask = CFM_BOLD;
    charFmt.dwEffects = CFE_BOLD;
    SendMessageW(text, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&charFmt);
    parFmt.cbSize = sizeof(parFmt);
    parFmt.dwMask = PFM_STARTINDENT;
    parFmt.dxStartIndent = MY_INDENT * 3;
    add_string_resource_with_paraformat_to_control(text, IDS_VALID_FROM,
     &parFmt);
    charFmt.dwEffects = 0;
    SendMessageW(text, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&charFmt);
    add_date_string_to_control(text, &cert->pCertInfo->NotBefore);
    charFmt.dwEffects = CFE_BOLD;
    SendMessageW(text, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&charFmt);
    add_string_resource_to_control(text, IDS_VALID_TO);
    charFmt.dwEffects = 0;
    SendMessageW(text, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&charFmt);
    add_date_string_to_control(text, &cert->pCertInfo->NotAfter);
    add_unformatted_text_to_control(text, &nl, 1);
}

static void set_general_info(HWND hwnd,
 PCCRYPTUI_VIEWCERTIFICATE_STRUCTW pCertViewInfo)
{
    set_cert_info(hwnd, pCertViewInfo);
    set_cert_name_string(hwnd, pCertViewInfo->pCertContext, 0,
     IDS_SUBJECT_HEADING);
    set_cert_name_string(hwnd, pCertViewInfo->pCertContext,
     CERT_NAME_ISSUER_FLAG, IDS_ISSUER_HEADING);
    set_cert_validity_period(hwnd, pCertViewInfo->pCertContext);
}

static LRESULT CALLBACK user_notice_dlg_proc(HWND hwnd, UINT msg, WPARAM wp,
 LPARAM lp)
{
    LRESULT ret = 0;
    HWND text;
    struct IssuerStatement *issuerStatement;

    switch (msg)
    {
    case WM_INITDIALOG:
        text = GetDlgItem(hwnd, IDC_USERNOTICE);
        issuerStatement = (struct IssuerStatement *)lp;
        add_unformatted_text_to_control(text, issuerStatement->userNotice,
         strlenW(issuerStatement->userNotice));
        if (issuerStatement->cps)
            SetWindowLongPtrW(hwnd, DWLP_USER, (LPARAM)issuerStatement->cps);
        else
            EnableWindow(GetDlgItem(hwnd, IDC_CPS), FALSE);
        break;
    case WM_COMMAND:
        switch (wp)
        {
        case IDOK:
            EndDialog(hwnd, IDOK);
            ret = TRUE;
            break;
        case IDC_CPS:
        {
            IBindCtx *bctx = NULL;
            LPWSTR cps;

            CreateBindCtx(0, &bctx);
            cps = (LPWSTR)GetWindowLongPtrW(hwnd, DWLP_USER);
            HlinkSimpleNavigateToString(cps, NULL, NULL, NULL, bctx, NULL,
             HLNF_OPENINNEWWINDOW, 0);
            IBindCtx_Release(bctx);
            break;
        }
        }
    }
    return ret;
}

static void show_user_notice(HWND hwnd, struct IssuerStatement *issuerStatement)
{
    DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_USERNOTICE), hwnd,
     user_notice_dlg_proc, (LPARAM)issuerStatement);
}

static LRESULT CALLBACK general_dlg_proc(HWND hwnd, UINT msg, WPARAM wp,
 LPARAM lp)
{
    PROPSHEETPAGEW *page;
    PCCRYPTUI_VIEWCERTIFICATE_STRUCTW pCertViewInfo;

    TRACE("(%p, %08x, %08lx, %08lx)\n", hwnd, msg, wp, lp);

    switch (msg)
    {
    case WM_INITDIALOG:
        page = (PROPSHEETPAGEW *)lp;
        pCertViewInfo = (PCCRYPTUI_VIEWCERTIFICATE_STRUCTW)page->lParam;
        if (pCertViewInfo->dwFlags & CRYPTUI_DISABLE_ADDTOSTORE)
            ShowWindow(GetDlgItem(hwnd, IDC_ADDTOSTORE), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_ISSUERSTATEMENT), FALSE);
        set_general_info(hwnd, pCertViewInfo);
        break;
    case WM_COMMAND:
        switch (wp)
        {
        case IDC_ADDTOSTORE:
            CryptUIWizImport(0, hwnd, NULL, NULL, NULL);
            break;
        case IDC_ISSUERSTATEMENT:
        {
            struct IssuerStatement *issuerStatement =
             (struct IssuerStatement *)GetWindowLongPtrW(hwnd, DWLP_USER);

            if (issuerStatement)
            {
                if (issuerStatement->userNotice)
                    show_user_notice(hwnd, issuerStatement);
                else if (issuerStatement->cps)
                {
                    IBindCtx *bctx = NULL;

                    CreateBindCtx(0, &bctx);
                    HlinkSimpleNavigateToString(issuerStatement->cps, NULL,
                     NULL, NULL, bctx, NULL, HLNF_OPENINNEWWINDOW, 0);
                    IBindCtx_Release(bctx);
                }
            }
            break;
        }
        }
        break;
    }
    return 0;
}

static UINT CALLBACK general_callback_proc(HWND hwnd, UINT msg,
 PROPSHEETPAGEW *page)
{
    struct IssuerStatement *issuerStatement;

    switch (msg)
    {
    case PSPCB_RELEASE:
        issuerStatement =
         (struct IssuerStatement *)GetWindowLongPtrW(hwnd, DWLP_USER);
        if (issuerStatement)
        {
            HeapFree(GetProcessHeap(), 0, issuerStatement->cps);
            HeapFree(GetProcessHeap(), 0, issuerStatement->userNotice);
            HeapFree(GetProcessHeap(), 0, issuerStatement);
        }
        break;
    }
    return 1;
}

static void init_general_page(PCCRYPTUI_VIEWCERTIFICATE_STRUCTW pCertViewInfo,
 PROPSHEETPAGEW *page)
{
    memset(page, 0, sizeof(PROPSHEETPAGEW));
    page->dwSize = sizeof(PROPSHEETPAGEW);
    page->dwFlags = PSP_USECALLBACK;
    page->pfnCallback = general_callback_proc;
    page->hInstance = hInstance;
    page->u.pszTemplate = MAKEINTRESOURCEW(IDD_GENERAL);
    page->pfnDlgProc = general_dlg_proc;
    page->lParam = (LPARAM)pCertViewInfo;
}

typedef WCHAR * (*field_format_func)(PCCERT_CONTEXT cert);

static WCHAR *field_format_version(PCCERT_CONTEXT cert)
{
    static const WCHAR fmt[] = { 'V','%','d',0 };
    WCHAR *buf = HeapAlloc(GetProcessHeap(), 0, 12 * sizeof(WCHAR));

    if (buf)
        sprintfW(buf, fmt, cert->pCertInfo->dwVersion);
    return buf;
}

static WCHAR *format_hex_string(void *pb, DWORD cb)
{
    WCHAR *buf = HeapAlloc(GetProcessHeap(), 0, (cb * 3 + 1) * sizeof(WCHAR));

    if (buf)
    {
        static const WCHAR fmt[] = { '%','0','2','x',' ',0 };
        DWORD i;
        WCHAR *ptr;

        for (i = 0, ptr = buf; i < cb; i++, ptr += 3)
            sprintfW(ptr, fmt, ((BYTE *)pb)[i]);
    }
    return buf;
}

static WCHAR *field_format_serial_number(PCCERT_CONTEXT cert)
{
    return format_hex_string(cert->pCertInfo->SerialNumber.pbData,
     cert->pCertInfo->SerialNumber.cbData);
}

static WCHAR *field_format_issuer(PCCERT_CONTEXT cert)
{
    return get_cert_name_string(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE,
     CERT_NAME_ISSUER_FLAG);
}

static WCHAR *field_format_detailed_cert_name(PCERT_NAME_BLOB name)
{
    WCHAR *str = NULL;
    DWORD len = CertNameToStrW(X509_ASN_ENCODING, name,
     CERT_X500_NAME_STR | CERT_NAME_STR_CRLF_FLAG, NULL, 0);

    if (len)
    {
        str = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (str)
            CertNameToStrW(X509_ASN_ENCODING, name,
             CERT_X500_NAME_STR | CERT_NAME_STR_CRLF_FLAG, str, len);
    }
    return str;
}

static WCHAR *field_format_detailed_issuer(PCCERT_CONTEXT cert, void *param)
{
    return field_format_detailed_cert_name(&cert->pCertInfo->Issuer);
}

static WCHAR *field_format_subject(PCCERT_CONTEXT cert)
{
    return get_cert_name_string(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0);
}

static WCHAR *field_format_detailed_subject(PCCERT_CONTEXT cert, void *param)
{
    return field_format_detailed_cert_name(&cert->pCertInfo->Subject);
}

static WCHAR *format_long_date(const FILETIME *fileTime)
{
    WCHAR dateFmt[80]; /* long enough for LOCALE_SLONGDATE */
    DWORD len;
    WCHAR *buf = NULL;
    SYSTEMTIME sysTime;

    /* FIXME: format isn't quite right, want time too */
    GetLocaleInfoW(LOCALE_SYSTEM_DEFAULT, LOCALE_SLONGDATE, dateFmt,
     sizeof(dateFmt) / sizeof(dateFmt[0]));
    FileTimeToSystemTime(fileTime, &sysTime);
    len = GetDateFormatW(LOCALE_SYSTEM_DEFAULT, 0, &sysTime, dateFmt, NULL, 0);
    if (len)
    {
        buf = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (buf)
            GetDateFormatW(LOCALE_SYSTEM_DEFAULT, 0, &sysTime, dateFmt, buf,
             len);
    }
    return buf;
}

static WCHAR *field_format_from_date(PCCERT_CONTEXT cert)
{
    return format_long_date(&cert->pCertInfo->NotBefore);
}

static WCHAR *field_format_to_date(PCCERT_CONTEXT cert)
{
    return format_long_date(&cert->pCertInfo->NotAfter);
}

static WCHAR *field_format_public_key(PCCERT_CONTEXT cert)
{
    PCCRYPT_OID_INFO oidInfo;
    WCHAR *buf = NULL;

    oidInfo = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY,
     cert->pCertInfo->SubjectPublicKeyInfo.Algorithm.pszObjId, 0);
    if (oidInfo)
    {
        WCHAR fmt[MAX_STRING_LEN];

        if (LoadStringW(hInstance, IDS_FIELD_PUBLIC_KEY_FORMAT, fmt,
         sizeof(fmt) / sizeof(fmt[0])))
        {
            /* Allocate the output buffer.  Use the number of bytes in the
             * public key as a conservative (high) estimate for the number of
             * digits in its output.
             * The output is of the form (in English)
             * "<public key algorithm> (<public key bit length> bits)".
             * Ordinarily having two positional parameters in a string is not a
             * good idea, but as this isn't a sentence fragment, it shouldn't
             * be word-order dependent.
             */
            buf = HeapAlloc(GetProcessHeap(), 0,
             (strlenW(fmt) + strlenW(oidInfo->pwszName) +
             cert->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData * 8)
             * sizeof(WCHAR));
            if (buf)
                sprintfW(buf, fmt, oidInfo->pwszName,
                 CertGetPublicKeyLength(X509_ASN_ENCODING,
                  &cert->pCertInfo->SubjectPublicKeyInfo));
        }
    }
    return buf;
}

static WCHAR *field_format_detailed_public_key(PCCERT_CONTEXT cert, void *param)
{
    return format_hex_string(
     cert->pCertInfo->SubjectPublicKeyInfo.PublicKey.pbData,
     cert->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData);
}

struct field_value_data;
struct detail_data
{
    PCCRYPTUI_VIEWCERTIFICATE_STRUCTW pCertViewInfo;
    BOOL *pfPropertiesChanged;
    int cFields;
    struct field_value_data *fields;
};

typedef void (*add_fields_func)(HWND hwnd, struct detail_data *data);

typedef WCHAR *(*create_detailed_value_func)(PCCERT_CONTEXT cert, void *param);

struct field_value_data
{
    create_detailed_value_func create;
    LPWSTR detailed_value;
    void *param;
};

static void add_field_value_data(struct detail_data *data,
 create_detailed_value_func create, void *param)
{
    if (data->cFields)
        data->fields = HeapReAlloc(GetProcessHeap(), 0, data->fields,
         (data->cFields + 1) * sizeof(struct field_value_data));
    else
        data->fields = HeapAlloc(GetProcessHeap(), 0,
         sizeof(struct field_value_data));
    if (data->fields)
    {
        data->fields[data->cFields].create = create;
        data->fields[data->cFields].detailed_value = NULL;
        data->fields[data->cFields].param = param;
        data->cFields++;
    }
}

static void add_field_and_value_to_list(HWND hwnd, struct detail_data *data,
 LPWSTR field, LPWSTR value, create_detailed_value_func create, void *param)
{
    LVITEMW item;
    int iItem = SendMessageW(hwnd, LVM_GETITEMCOUNT, 0, 0);

    item.mask = LVIF_TEXT | LVIF_PARAM;
    item.iItem = iItem;
    item.iSubItem = 0;
    item.pszText = field;
    item.lParam = (LPARAM)data;
    SendMessageW(hwnd, LVM_INSERTITEMW, 0, (LPARAM)&item);
    if (value)
    {
        item.pszText = value;
        item.iSubItem = 1;
        SendMessageW(hwnd, LVM_SETITEMTEXTW, iItem, (LPARAM)&item);
    }
    add_field_value_data(data, create, param);
}

static void add_string_id_and_value_to_list(HWND hwnd, struct detail_data *data,
 int id, LPWSTR value, create_detailed_value_func create, void *param)
{
    WCHAR buf[MAX_STRING_LEN];

    LoadStringW(hInstance, id, buf, sizeof(buf) / sizeof(buf[0]));
    add_field_and_value_to_list(hwnd, data, buf, value, create, param);
}

struct v1_field
{
    int id;
    field_format_func format;
    create_detailed_value_func create_detailed_value;
};

static void add_v1_field(HWND hwnd, struct detail_data *data,
 const struct v1_field *field)
{
    WCHAR *val = field->format(data->pCertViewInfo->pCertContext);

    if (val)
    {
        add_string_id_and_value_to_list(hwnd, data, field->id, val,
         field->create_detailed_value, NULL);
        HeapFree(GetProcessHeap(), 0, val);
    }
}

static const struct v1_field v1_fields[] = {
 { IDS_FIELD_VERSION, field_format_version, NULL },
 { IDS_FIELD_SERIAL_NUMBER, field_format_serial_number, NULL },
 { IDS_FIELD_ISSUER, field_format_issuer, field_format_detailed_issuer },
 { IDS_FIELD_VALID_FROM, field_format_from_date, NULL },
 { IDS_FIELD_VALID_TO, field_format_to_date, NULL },
 { IDS_FIELD_SUBJECT, field_format_subject, field_format_detailed_subject },
 { IDS_FIELD_PUBLIC_KEY, field_format_public_key,
   field_format_detailed_public_key }
};

static void add_v1_fields(HWND hwnd, struct detail_data *data)
{
    int i;
    PCCERT_CONTEXT cert = data->pCertViewInfo->pCertContext;

    /* The last item in v1_fields is the public key, which is not in the loop
     * because it's a special case.
     */
    for (i = 0; i < sizeof(v1_fields) / sizeof(v1_fields[0]) - 1; i++)
        add_v1_field(hwnd, data, &v1_fields[i]);
    if (cert->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData)
        add_v1_field(hwnd, data, &v1_fields[i]);
}

static WCHAR *crypt_format_extension(const CERT_EXTENSION *ext, DWORD formatStrType)
{
    WCHAR *str = NULL;
    DWORD size;

    if (CryptFormatObject(X509_ASN_ENCODING, 0, formatStrType, NULL,
     ext->pszObjId, ext->Value.pbData, ext->Value.cbData, NULL, &size))
    {
        str = HeapAlloc(GetProcessHeap(), 0, size);
        CryptFormatObject(X509_ASN_ENCODING, 0, formatStrType, NULL,
         ext->pszObjId, ext->Value.pbData, ext->Value.cbData, str, &size);
    }
    return str;
}

static WCHAR *field_format_extension_hex_with_ascii(const CERT_EXTENSION *ext)
{
    WCHAR *str = NULL;

    if (ext->Value.cbData)
    {
        /* The output is formatted as:
         * <hex bytes>  <ascii bytes>\n
         * where <hex bytes> is a string of up to 8 bytes, output as %02x,
         * and <ascii bytes> is the ASCII equivalent of each byte, or '.' if
         * the byte is not printable.
         * So, for example, the extension value consisting of the following
         * bytes:
         *   0x30,0x14,0x31,0x12,0x30,0x10,0x06,0x03,0x55,0x04,0x03,
         *   0x13,0x09,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67
         * is output as:
         *   30 14 31 12 30 10 06 03  0.1.0...
         *   55 04 03 13 09 4a 75 61  U....Jua
         *   6e 20 4c 61 6e 67        n Lang
         * The allocation size therefore requires:
         * - 4 characters per character in an 8-byte line
         *   (2 for the hex format, one for the space, one for the ASCII value)
         * - 3 more characters per 8-byte line (two spaces and a newline)
         * - 1 character for the terminating nul
         * FIXME: should use a fixed-width font for this
         */
        DWORD lines = (ext->Value.cbData + 7) / 8;

        str = HeapAlloc(GetProcessHeap(), 0,
         (lines * 8 * 4 + lines * 3 + 1) * sizeof(WCHAR));
        if (str)
        {
            static const WCHAR fmt[] = { '%','0','2','x',' ',0 };
            DWORD i, j;
            WCHAR *ptr;

            for (i = 0, ptr = str; i < ext->Value.cbData; i += 8)
            {
                /* Output as hex bytes first */
                for (j = i; j < min(i + 8, ext->Value.cbData); j++, ptr += 3)
                    sprintfW(ptr, fmt, ext->Value.pbData[j]);
                /* Pad the hex output with spaces for alignment */
                if (j == ext->Value.cbData && j % 8)
                {
                    static const WCHAR pad[] = { ' ',' ',' ' };

                    for (; j % 8; j++, ptr += sizeof(pad) / sizeof(pad[0]))
                        memcpy(ptr, pad, sizeof(pad));
                }
                /* The last sprintfW included a space, so just insert one
                 * more space between the hex bytes and the ASCII output
                 */
                *ptr++ = ' ';
                /* Output as ASCII bytes */
                for (j = i; j < min(i + 8, ext->Value.cbData); j++, ptr++)
                {
                    if (isprintW(ext->Value.pbData[j]) &&
                     !isspaceW(ext->Value.pbData[j]))
                        *ptr = ext->Value.pbData[j];
                    else
                        *ptr = '.';
                }
                *ptr++ = '\n';
            }
            *ptr++ = '\0';
        }
    }
    return str;
}

static WCHAR *field_format_detailed_extension(PCCERT_CONTEXT cert, void *param)
{
    PCERT_EXTENSION ext = param;
    LPWSTR str = crypt_format_extension(ext,
     CRYPT_FORMAT_STR_MULTI_LINE | CRYPT_FORMAT_STR_NO_HEX);

    if (!str)
        str = field_format_extension_hex_with_ascii(ext);
    return str;
}

static void add_cert_extension_detail(HWND hwnd, struct detail_data *data,
 PCERT_EXTENSION ext)
{
    PCCRYPT_OID_INFO oidInfo = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY,
     ext->pszObjId, 0);
    LPWSTR val = crypt_format_extension(ext, 0);

    if (oidInfo)
        add_field_and_value_to_list(hwnd, data, (LPWSTR)oidInfo->pwszName,
         val, field_format_detailed_extension, ext);
    else
    {
        DWORD len = strlen(ext->pszObjId);
        LPWSTR oidW = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));

        if (oidW)
        {
            DWORD i;

            for (i = 0; i <= len; i++)
                oidW[i] = ext->pszObjId[i];
            add_field_and_value_to_list(hwnd, data, oidW, val,
             field_format_detailed_extension, ext);
            HeapFree(GetProcessHeap(), 0, oidW);
        }
    }
    HeapFree(GetProcessHeap(), 0, val);
}

static void add_all_extensions(HWND hwnd, struct detail_data *data)
{
    DWORD i;
    PCCERT_CONTEXT cert = data->pCertViewInfo->pCertContext;

    for (i = 0; i < cert->pCertInfo->cExtension; i++)
        add_cert_extension_detail(hwnd, data, &cert->pCertInfo->rgExtension[i]);
}

static void add_critical_extensions(HWND hwnd, struct detail_data *data)
{
    DWORD i;
    PCCERT_CONTEXT cert = data->pCertViewInfo->pCertContext;

    for (i = 0; i < cert->pCertInfo->cExtension; i++)
        if (cert->pCertInfo->rgExtension[i].fCritical)
            add_cert_extension_detail(hwnd, data,
             &cert->pCertInfo->rgExtension[i]);
}

typedef WCHAR * (*prop_to_value_func)(void *pb, DWORD cb);

struct prop_id_to_string_id
{
    DWORD prop;
    int id;
    BOOL prop_is_string;
    prop_to_value_func prop_to_value;
};

static WCHAR *format_enhanced_key_usage_value(void *pb, DWORD cb)
{
    CERT_EXTENSION ext;

    ext.pszObjId = (LPSTR)X509_ENHANCED_KEY_USAGE;
    ext.fCritical = FALSE;
    ext.Value.pbData = pb;
    ext.Value.cbData = cb;
    return crypt_format_extension(&ext, 0);
}

/* Logically the access state should also be checked, and IDC_EDITPROPERTIES
 * disabled for read-only certificates, but native doesn't appear to do that.
 */
static const struct prop_id_to_string_id prop_id_map[] = {
 { CERT_HASH_PROP_ID, IDS_PROP_HASH, FALSE, format_hex_string },
 { CERT_FRIENDLY_NAME_PROP_ID, IDS_PROP_FRIENDLY_NAME, TRUE, NULL },
 { CERT_DESCRIPTION_PROP_ID, IDS_PROP_DESCRIPTION, TRUE, NULL },
 { CERT_ENHKEY_USAGE_PROP_ID, IDS_PROP_ENHKEY_USAGE, FALSE,
   format_enhanced_key_usage_value },
};

static void add_properties(HWND hwnd, struct detail_data *data)
{
    DWORD i;
    PCCERT_CONTEXT cert = data->pCertViewInfo->pCertContext;

    for (i = 0; i < sizeof(prop_id_map) / sizeof(prop_id_map[0]); i++)
    {
        DWORD cb;

        if (CertGetCertificateContextProperty(cert, prop_id_map[i].prop, NULL,
         &cb))
        {
            BYTE *pb;
            WCHAR *val = NULL;

            /* FIXME: MS adds a separate value for the signature hash
             * algorithm.
             */
            pb = HeapAlloc(GetProcessHeap(), 0, cb);
            if (pb)
            {
                if (CertGetCertificateContextProperty(cert,
                 prop_id_map[i].prop, pb, &cb))
                {
                    if (prop_id_map[i].prop_is_string)
                    {
                        val = (LPWSTR)pb;
                        /* Don't double-free pb */
                        pb = NULL;
                    }
                    else
                        val = prop_id_map[i].prop_to_value(pb, cb);
                }
                HeapFree(GetProcessHeap(), 0, pb);
            }
            add_string_id_and_value_to_list(hwnd, data, prop_id_map[i].id, val,
             NULL, NULL);
        }
    }
}

static void add_all_fields(HWND hwnd, struct detail_data *data)
{
    add_v1_fields(hwnd, data);
    add_all_extensions(hwnd, data);
    add_properties(hwnd, data);
}

struct selection_list_item
{
    int id;
    add_fields_func add;
};

const struct selection_list_item listItems[] = {
 { IDS_FIELDS_ALL, add_all_fields },
 { IDS_FIELDS_V1, add_v1_fields },
 { IDS_FIELDS_EXTENSIONS, add_all_extensions },
 { IDS_FIELDS_CRITICAL_EXTENSIONS, add_critical_extensions },
 { IDS_FIELDS_PROPERTIES, add_properties },
};

static void create_show_list(HWND hwnd, struct detail_data *data)
{
    HWND cb = GetDlgItem(hwnd, IDC_DETAIL_SELECT);
    WCHAR buf[MAX_STRING_LEN];
    int i;

    for (i = 0; i < sizeof(listItems) / sizeof(listItems[0]); i++)
    {
        int index;

        LoadStringW(hInstance, listItems[i].id, buf,
         sizeof(buf) / sizeof(buf[0]));
        index = SendMessageW(cb, CB_INSERTSTRING, -1, (LPARAM)buf);
        SendMessageW(cb, CB_SETITEMDATA, index, (LPARAM)data);
    }
    SendMessageW(cb, CB_SETCURSEL, 0, 0);
}

static void create_listview_columns(HWND hwnd)
{
    HWND lv = GetDlgItem(hwnd, IDC_DETAIL_LIST);
    RECT rc;
    WCHAR buf[MAX_STRING_LEN];
    LVCOLUMNW column;

    SendMessageW(lv, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);
    GetWindowRect(lv, &rc);
    LoadStringW(hInstance, IDS_FIELD, buf, sizeof(buf) / sizeof(buf[0]));
    column.mask = LVCF_WIDTH | LVCF_TEXT;
    column.cx = (rc.right - rc.left) / 2 - 2;
    column.pszText = buf;
    SendMessageW(lv, LVM_INSERTCOLUMNW, 0, (LPARAM)&column);
    LoadStringW(hInstance, IDS_VALUE, buf, sizeof(buf) / sizeof(buf[0]));
    SendMessageW(lv, LVM_INSERTCOLUMNW, 1, (LPARAM)&column);
}

static void set_fields_selection(HWND hwnd, struct detail_data *data, int sel)
{
    HWND list = GetDlgItem(hwnd, IDC_DETAIL_LIST);

    if (sel >= 0 && sel < sizeof(listItems) / sizeof(listItems[0]))
    {
        SendMessageW(list, LVM_DELETEALLITEMS, 0, 0);
        listItems[sel].add(list, data);
    }
}

static void create_cert_details_list(HWND hwnd, struct detail_data *data)
{
    create_show_list(hwnd, data);
    create_listview_columns(hwnd);
    set_fields_selection(hwnd, data, 0);
}

static void add_purpose(HWND hwnd, LPCSTR oid)
{
    HWND lv = GetDlgItem(hwnd, IDC_CERTIFICATE_USAGES);
    PCRYPT_OID_INFO info = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
     sizeof(CRYPT_OID_INFO));

    if (info)
    {
        char *oidCopy = HeapAlloc(GetProcessHeap(), 0, strlen(oid) + 1);

        if (oidCopy)
        {
            LVITEMA item;

            strcpy(oidCopy, oid);
            info->cbSize = sizeof(CRYPT_OID_INFO);
            info->pszOID = oidCopy;
            item.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
            item.state = INDEXTOSTATEIMAGEMASK(CheckBitmapIndexChecked);
            item.stateMask = LVIS_STATEIMAGEMASK;
            item.iItem = SendMessageW(lv, LVM_GETITEMCOUNT, 0, 0);
            item.iSubItem = 0;
            item.lParam = (LPARAM)info;
            item.pszText = oidCopy;
            SendMessageA(lv, LVM_INSERTITEMA, 0, (LPARAM)&item);
        }
        else
            HeapFree(GetProcessHeap(), 0, info);
    }
}

static BOOL is_valid_oid(LPCSTR oid)
{
    BOOL ret;

    if (oid[0] != '0' && oid[0] != '1' && oid[0] != '2')
        ret = FALSE;
    else if (oid[1] != '.')
        ret = FALSE;
    else if (!oid[2])
        ret = FALSE;
    else
    {
        const char *ptr;
        BOOL expectNum = TRUE;

        for (ptr = oid + 2, ret = TRUE; ret && *ptr; ptr++)
        {
            if (expectNum)
            {
                if (!isdigit(*ptr))
                    ret = FALSE;
                else if (*(ptr + 1) == '.')
                    expectNum = FALSE;
            }
            else
            {
                if (*ptr != '.')
                    ret = FALSE;
                else if (!(*(ptr + 1)))
                    ret = FALSE;
                else
                    expectNum = TRUE;
            }
        }
    }
    return ret;
}

static BOOL is_oid_in_list(HWND hwnd, LPCSTR oid)
{
    return find_oid_in_list(GetDlgItem(hwnd, IDC_CERTIFICATE_USAGES), oid)
     != -1;
}

#define MAX_PURPOSE 255

static LRESULT CALLBACK add_purpose_dlg_proc(HWND hwnd, UINT msg,
 WPARAM wp, LPARAM lp)
{
    LRESULT ret = 0;
    char buf[MAX_PURPOSE + 1];

    switch (msg)
    {
    case WM_INITDIALOG:
        SendMessageW(GetDlgItem(hwnd, IDC_NEW_PURPOSE), EM_SETLIMITTEXT,
         MAX_PURPOSE, 0);
        ShowScrollBar(GetDlgItem(hwnd, IDC_NEW_PURPOSE), SB_VERT, FALSE);
        SetWindowLongPtrW(hwnd, DWLP_USER, lp);
        break;
    case WM_COMMAND:
        switch (HIWORD(wp))
        {
        case EN_CHANGE:
            if (LOWORD(wp) == IDC_NEW_PURPOSE)
            {
                /* Show/hide scroll bar on description depending on how much
                 * text it has.
                 */
                HWND description = GetDlgItem(hwnd, IDC_NEW_PURPOSE);
                int lines = SendMessageW(description, EM_GETLINECOUNT, 0, 0);

                ShowScrollBar(description, SB_VERT, lines > 1);
            }
            break;
        case BN_CLICKED:
            switch (LOWORD(wp))
            {
            case IDOK:
                SendMessageA(GetDlgItem(hwnd, IDC_NEW_PURPOSE), WM_GETTEXT,
                 sizeof(buf) / sizeof(buf[0]), (LPARAM)buf);
                if (!buf[0])
                {
                    /* An empty purpose is the same as cancelling */
                    EndDialog(hwnd, IDCANCEL);
                    ret = TRUE;
                }
                else if (!is_valid_oid(buf))
                {
                    WCHAR title[MAX_STRING_LEN], error[MAX_STRING_LEN];

                    LoadStringW(hInstance, IDS_CERTIFICATE_PURPOSE_ERROR, error,
                     sizeof(error) / sizeof(error[0]));
                    LoadStringW(hInstance, IDS_CERTIFICATE_PROPERTIES, title,
                     sizeof(title) / sizeof(title[0]));
                    MessageBoxW(hwnd, error, title, MB_ICONERROR | MB_OK);
                }
                else if (is_oid_in_list(
                 (HWND)GetWindowLongPtrW(hwnd, DWLP_USER), buf))
                {
                    WCHAR title[MAX_STRING_LEN], error[MAX_STRING_LEN];

                    LoadStringW(hInstance, IDS_CERTIFICATE_PURPOSE_EXISTS,
                     error, sizeof(error) / sizeof(error[0]));
                    LoadStringW(hInstance, IDS_CERTIFICATE_PROPERTIES, title,
                     sizeof(title) / sizeof(title[0]));
                    MessageBoxW(hwnd, error, title, MB_ICONEXCLAMATION | MB_OK);
                }
                else
                {
                    HWND parent = (HWND)GetWindowLongPtrW(hwnd, DWLP_USER);

                    add_purpose(parent, buf);
                    EndDialog(hwnd, wp);
                    ret = TRUE;
                }
                break;
            case IDCANCEL:
                EndDialog(hwnd, wp);
                ret = TRUE;
                break;
            }
            break;
        }
        break;
    }
    return ret;
}

static WCHAR *get_cert_property_as_string(PCCERT_CONTEXT cert, DWORD prop)
{
    WCHAR *name = NULL;
    DWORD cb;

    if (CertGetCertificateContextProperty(cert, prop, NULL, &cb))
    {
        name = HeapAlloc(GetProcessHeap(), 0, cb);
        if (name)
        {
            if (!CertGetCertificateContextProperty(cert, prop, name, &cb))
            {
                HeapFree(GetProcessHeap(), 0, name);
                name = NULL;
            }
        }
    }
    return name;
}

static void redraw_states(HWND list, BOOL enabled)
{
    int items = SendMessageW(list, LVM_GETITEMCOUNT, 0, 0), i;

    for (i = 0; i < items; i++)
    {
        BOOL change = FALSE;
        int state;

        state = SendMessageW(list, LVM_GETITEMSTATE, i, LVIS_STATEIMAGEMASK);
        /* This reverses the INDEXTOSTATEIMAGEMASK shift.  There doesn't appear
         * to be a handy macro for it.
         */
        state >>= 12;
        if (enabled)
        {
            if (state == CheckBitmapIndexDisabledChecked)
            {
                state = CheckBitmapIndexChecked;
                change = TRUE;
            }
            if (state == CheckBitmapIndexDisabledUnchecked)
            {
                state = CheckBitmapIndexUnchecked;
                change = TRUE;
            }
        }
        else
        {
            if (state == CheckBitmapIndexChecked)
            {
                state = CheckBitmapIndexDisabledChecked;
                change = TRUE;
            }
            if (state == CheckBitmapIndexUnchecked)
            {
                state = CheckBitmapIndexDisabledUnchecked;
                change = TRUE;
            }
        }
        if (change)
        {
            LVITEMW item;

            item.state = INDEXTOSTATEIMAGEMASK(state);
            item.stateMask = LVIS_STATEIMAGEMASK;
            SendMessageW(list, LVM_SETITEMSTATE, i, (LPARAM)&item);
        }
    }
}

typedef enum {
    PurposeEnableAll = 0,
    PurposeDisableAll,
    PurposeEnableSelected
} PurposeSelection;

static void select_purposes(HWND hwnd, PurposeSelection selection)
{
    HWND lv = GetDlgItem(hwnd, IDC_CERTIFICATE_USAGES);

    switch (selection)
    {
    case PurposeEnableAll:
    case PurposeDisableAll:
        EnableWindow(lv, FALSE);
        redraw_states(lv, FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_ADD_PURPOSE), FALSE);
        break;
    case PurposeEnableSelected:
        EnableWindow(lv, TRUE);
        redraw_states(lv, TRUE);
        EnableWindow(GetDlgItem(hwnd, IDC_ADD_PURPOSE), TRUE);
    }
}

struct edit_cert_data
{
    PCCERT_CONTEXT cert;
    BOOL *pfPropertiesChanged;
    HIMAGELIST imageList;
};

static void show_cert_usages(HWND hwnd, struct edit_cert_data *data)
{
    PCCERT_CONTEXT cert = data->cert;
    HWND lv = GetDlgItem(hwnd, IDC_CERTIFICATE_USAGES);
    PCERT_ENHKEY_USAGE usage;
    DWORD size;
    RECT rc;
    LVCOLUMNW column;
    PurposeSelection purposeSelection = PurposeEnableAll;

    GetWindowRect(lv, &rc);
    column.mask = LVCF_WIDTH;
    column.cx = rc.right - rc.left;
    SendMessageW(lv, LVM_INSERTCOLUMNW, 0, (LPARAM)&column);
    SendMessageW(lv, LVM_SETIMAGELIST, LVSIL_STATE, (LPARAM)data->imageList);

    /* Get enhanced key usage.  Have to check for a property and an extension
     * separately, because CertGetEnhancedKeyUsage will succeed and return an
     * empty usage if neither is set.  Unfortunately an empty usage implies
     * no usage is allowed, so we have to distinguish between the two cases.
     */
    if (CertGetEnhancedKeyUsage(cert, CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG,
     NULL, &size))
    {
        usage = HeapAlloc(GetProcessHeap(), 0, size);
        if (!CertGetEnhancedKeyUsage(cert,
         CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG, usage, &size))
        {
            HeapFree(GetProcessHeap(), 0, usage);
            usage = NULL;
        }
        else if (usage->cUsageIdentifier)
            purposeSelection = PurposeEnableSelected;
        else
            purposeSelection = PurposeDisableAll;
    }
    else if (CertGetEnhancedKeyUsage(cert, CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
     NULL, &size))
    {
        usage = HeapAlloc(GetProcessHeap(), 0, size);
        if (!CertGetEnhancedKeyUsage(cert,
         CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG, usage, &size))
        {
            HeapFree(GetProcessHeap(), 0, usage);
            usage = NULL;
        }
        else if (usage->cUsageIdentifier)
            purposeSelection = PurposeEnableAll;
        else
            purposeSelection = PurposeDisableAll;
    }
    else
    {
        purposeSelection = PurposeEnableAll;
        usage = NULL;
    }
    if (usage)
    {
        DWORD i;

        for (i = 0; i < usage->cUsageIdentifier; i++)
        {
            PCCRYPT_OID_INFO info = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY,
             usage->rgpszUsageIdentifier[i], CRYPT_ENHKEY_USAGE_OID_GROUP_ID);

            if (info)
                add_known_usage(lv, info, CheckBitmapIndexDisabledChecked);
            else
                add_purpose(hwnd, usage->rgpszUsageIdentifier[i]);
        }
        HeapFree(GetProcessHeap(), 0, usage);
    }
    else
        add_known_usages_to_list(lv, CheckBitmapIndexDisabledChecked);
    select_purposes(hwnd, purposeSelection);
    SendMessageW(GetDlgItem(hwnd, IDC_ENABLE_ALL_PURPOSES + purposeSelection),
     BM_CLICK, 0, 0);
}

static void set_general_cert_properties(HWND hwnd, struct edit_cert_data *data)
{
    PCCERT_CONTEXT cert = data->cert;
    WCHAR *str;

    if ((str = get_cert_property_as_string(cert, CERT_FRIENDLY_NAME_PROP_ID)))
    {
        SendMessageW(GetDlgItem(hwnd, IDC_FRIENDLY_NAME), WM_SETTEXT, 0,
         (LPARAM)str);
        HeapFree(GetProcessHeap(), 0, str);
    }
    if ((str = get_cert_property_as_string(cert, CERT_DESCRIPTION_PROP_ID)))
    {
        SendMessageW(GetDlgItem(hwnd, IDC_DESCRIPTION), WM_SETTEXT, 0,
         (LPARAM)str);
        HeapFree(GetProcessHeap(), 0, str);
    }
    show_cert_usages(hwnd, data);
}

static void set_cert_string_property(PCCERT_CONTEXT cert, DWORD prop,
 LPWSTR str)
{
    if (str && strlenW(str))
    {
        CRYPT_DATA_BLOB blob;

        blob.pbData = (BYTE *)str;
        blob.cbData = (strlenW(str) + 1) * sizeof(WCHAR);
        CertSetCertificateContextProperty(cert, prop, 0, &blob);
    }
    else
        CertSetCertificateContextProperty(cert, prop, 0, NULL);
}

#define WM_REFRESH_VIEW WM_USER + 0

static BOOL CALLBACK refresh_propsheet_pages(HWND hwnd, LPARAM lParam)
{
    if ((GetClassLongW(hwnd, GCW_ATOM) == WC_DIALOG))
        SendMessageW(hwnd, WM_REFRESH_VIEW, 0, 0);
    return TRUE;
}

#define MAX_FRIENDLY_NAME 40
#define MAX_DESCRIPTION 255

static void apply_general_changes(HWND hwnd)
{
    WCHAR buf[MAX_DESCRIPTION + 1];
    struct edit_cert_data *data =
     (struct edit_cert_data *)GetWindowLongPtrW(hwnd, DWLP_USER);

    SendMessageW(GetDlgItem(hwnd, IDC_FRIENDLY_NAME), WM_GETTEXT,
     sizeof(buf) / sizeof(buf[0]), (LPARAM)buf);
    set_cert_string_property(data->cert, CERT_FRIENDLY_NAME_PROP_ID, buf);
    SendMessageW(GetDlgItem(hwnd, IDC_DESCRIPTION), WM_GETTEXT,
     sizeof(buf) / sizeof(buf[0]), (LPARAM)buf);
    set_cert_string_property(data->cert, CERT_DESCRIPTION_PROP_ID, buf);
    if (IsDlgButtonChecked(hwnd, IDC_ENABLE_ALL_PURPOSES))
    {
        /* Setting a NULL usage removes the enhanced key usage property. */
        CertSetEnhancedKeyUsage(data->cert, NULL);
    }
    else if (IsDlgButtonChecked(hwnd, IDC_DISABLE_ALL_PURPOSES))
    {
        CERT_ENHKEY_USAGE usage = { 0, NULL };

        CertSetEnhancedKeyUsage(data->cert, &usage);
    }
    else if (IsDlgButtonChecked(hwnd, IDC_ENABLE_SELECTED_PURPOSES))
    {
        HWND lv = GetDlgItem(hwnd, IDC_CERTIFICATE_USAGES);
        CERT_ENHKEY_USAGE usage = { 0, NULL };
        int purposes = SendMessageW(lv, LVM_GETITEMCOUNT, 0, 0), i;
        LVITEMW item;

        item.mask = LVIF_STATE | LVIF_PARAM;
        item.iSubItem = 0;
        item.stateMask = LVIS_STATEIMAGEMASK;
        for (i = 0; i < purposes; i++)
        {
            item.iItem = i;
            if (SendMessageW(lv, LVM_GETITEMW, 0, (LPARAM)&item))
            {
                int state = item.state >> 12;

                if (state == CheckBitmapIndexChecked)
                {
                    CRYPT_OID_INFO *info = (CRYPT_OID_INFO *)item.lParam;

                    if (usage.cUsageIdentifier)
                        usage.rgpszUsageIdentifier =
                         HeapReAlloc(GetProcessHeap(), 0,
                         usage.rgpszUsageIdentifier,
                         (usage.cUsageIdentifier + 1) * sizeof(LPSTR));
                    else
                        usage.rgpszUsageIdentifier =
                         HeapAlloc(GetProcessHeap(), 0, sizeof(LPSTR));
                    if (usage.rgpszUsageIdentifier)
                        usage.rgpszUsageIdentifier[usage.cUsageIdentifier++] =
                         (LPSTR)info->pszOID;
                }
            }
        }
        CertSetEnhancedKeyUsage(data->cert, &usage);
        HeapFree(GetProcessHeap(), 0, usage.rgpszUsageIdentifier);
    }
    EnumChildWindows(GetParent(GetParent(hwnd)), refresh_propsheet_pages, 0);
    if (data->pfPropertiesChanged)
        *data->pfPropertiesChanged = TRUE;
}

static LRESULT CALLBACK cert_properties_general_dlg_proc(HWND hwnd, UINT msg,
 WPARAM wp, LPARAM lp)
{
    PROPSHEETPAGEW *page;

    TRACE("(%p, %08x, %08lx, %08lx)\n", hwnd, msg, wp, lp);

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        HWND description = GetDlgItem(hwnd, IDC_DESCRIPTION);
        struct detail_data *detailData;
        struct edit_cert_data *editData;

        page = (PROPSHEETPAGEW *)lp;
        detailData = (struct detail_data *)page->lParam;
        SendMessageW(GetDlgItem(hwnd, IDC_FRIENDLY_NAME), EM_SETLIMITTEXT,
         MAX_FRIENDLY_NAME, 0);
        SendMessageW(description, EM_SETLIMITTEXT, MAX_DESCRIPTION, 0);
        ShowScrollBar(description, SB_VERT, FALSE);
        editData = HeapAlloc(GetProcessHeap(), 0,
         sizeof(struct edit_cert_data));
        if (editData)
        {
            editData->imageList = ImageList_Create(16, 16,
             ILC_COLOR4 | ILC_MASK, 4, 0);
            if (editData->imageList)
            {
                HBITMAP bmp;
                COLORREF backColor = RGB(255, 0, 255);

                bmp = LoadBitmapW(hInstance, MAKEINTRESOURCEW(IDB_CHECKS));
                ImageList_AddMasked(editData->imageList, bmp, backColor);
                DeleteObject(bmp);
                ImageList_SetBkColor(editData->imageList, CLR_NONE);
            }
            editData->cert = detailData->pCertViewInfo->pCertContext;
            editData->pfPropertiesChanged = detailData->pfPropertiesChanged;
            SetWindowLongPtrW(hwnd, DWLP_USER, (LPARAM)editData);
            set_general_cert_properties(hwnd, editData);
        }
        break;
    }
    case WM_NOTIFY:
    {
        NMHDR *hdr = (NMHDR *)lp;
        NMITEMACTIVATE *nm;

        switch (hdr->code)
        {
        case NM_CLICK:
            nm = (NMITEMACTIVATE *)lp;
            toggle_usage(hwnd, nm->iItem);
            SendMessageW(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0);
            break;
        case PSN_APPLY:
            apply_general_changes(hwnd);
            break;
        }
        break;
    }
    case WM_COMMAND:
        switch (HIWORD(wp))
        {
        case EN_CHANGE:
            SendMessageW(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0);
            if (LOWORD(wp) == IDC_DESCRIPTION)
            {
                /* Show/hide scroll bar on description depending on how much
                 * text it has.
                 */
                HWND description = GetDlgItem(hwnd, IDC_DESCRIPTION);
                int lines = SendMessageW(description, EM_GETLINECOUNT, 0, 0);

                ShowScrollBar(description, SB_VERT, lines > 1);
            }
            break;
        case BN_CLICKED:
            switch (LOWORD(wp))
            {
            case IDC_ADD_PURPOSE:
                if (DialogBoxParamW(hInstance,
                 MAKEINTRESOURCEW(IDD_ADD_CERT_PURPOSE), hwnd,
                 add_purpose_dlg_proc, (LPARAM)hwnd) == IDOK)
                    SendMessageW(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0);
                break;
            case IDC_ENABLE_ALL_PURPOSES:
            case IDC_DISABLE_ALL_PURPOSES:
            case IDC_ENABLE_SELECTED_PURPOSES:
                SendMessageW(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0);
                select_purposes(hwnd, LOWORD(wp) - IDC_ENABLE_ALL_PURPOSES);
                break;
            }
            break;
        }
        break;
    }
    return 0;
}

static UINT CALLBACK cert_properties_general_callback(HWND hwnd, UINT msg,
 PROPSHEETPAGEW *page)
{
    HWND lv;
    int cItem, i;
    struct edit_cert_data *data;

    switch (msg)
    {
    case PSPCB_RELEASE:
        lv = GetDlgItem(hwnd, IDC_CERTIFICATE_USAGES);
        cItem = SendMessageW(lv, LVM_GETITEMCOUNT, 0, 0);
        for (i = 0; i < cItem; i++)
        {
            LVITEMW item;

            item.mask = LVIF_PARAM;
            item.iItem = i;
            item.iSubItem = 0;
            if (SendMessageW(lv, LVM_GETITEMW, 0, (LPARAM)&item) && item.lParam)
            {
                PCRYPT_OID_INFO info = (PCRYPT_OID_INFO)item.lParam;

                if (info->cbSize == sizeof(CRYPT_OID_INFO) && !info->dwGroupId)
                {
                    HeapFree(GetProcessHeap(), 0, (LPSTR)info->pszOID);
                    HeapFree(GetProcessHeap(), 0, info);
                }
            }
        }
        data = (struct edit_cert_data *)GetWindowLongPtrW(hwnd, DWLP_USER);
        if (data)
        {
            ImageList_Destroy(data->imageList);
            HeapFree(GetProcessHeap(), 0, data);
        }
        break;
    }
    return 1;
}

static void show_edit_cert_properties_dialog(HWND parent,
 struct detail_data *data)
{
    PROPSHEETHEADERW hdr;
    PROPSHEETPAGEW page; /* FIXME: need to add a cross-certificate page */

    TRACE("(%p)\n", data);

    memset(&page, 0, sizeof(PROPSHEETPAGEW));
    page.dwSize = sizeof(page);
    page.dwFlags = PSP_USECALLBACK;
    page.pfnCallback = cert_properties_general_callback;
    page.hInstance = hInstance;
    page.u.pszTemplate = MAKEINTRESOURCEW(IDD_CERT_PROPERTIES_GENERAL);
    page.pfnDlgProc = cert_properties_general_dlg_proc;
    page.lParam = (LPARAM)data;

    memset(&hdr, 0, sizeof(hdr));
    hdr.dwSize = sizeof(hdr);
    hdr.hwndParent = parent;
    hdr.dwFlags = PSH_PROPSHEETPAGE;
    hdr.hInstance = hInstance;
    hdr.pszCaption = MAKEINTRESOURCEW(IDS_CERTIFICATE_PROPERTIES);
    hdr.u3.ppsp = &page;
    hdr.nPages = 1;
    PropertySheetW(&hdr);
}

static void free_detail_fields(struct detail_data *data)
{
    DWORD i;

    for (i = 0; i < data->cFields; i++)
        HeapFree(GetProcessHeap(), 0, data->fields[i].detailed_value);
    HeapFree(GetProcessHeap(), 0, data->fields);
    data->fields = NULL;
    data->cFields = 0;
}

static void refresh_details_view(HWND hwnd)
{
    HWND cb = GetDlgItem(hwnd, IDC_DETAIL_SELECT);
    int curSel;
    struct detail_data *data;

    curSel = SendMessageW(cb, CB_GETCURSEL, 0, 0);
    /* Actually, any index will do, since they all store the same data value */
    data = (struct detail_data *)SendMessageW(cb, CB_GETITEMDATA, curSel, 0);
    free_detail_fields(data);
    set_fields_selection(hwnd, data, curSel);
}

static LRESULT CALLBACK detail_dlg_proc(HWND hwnd, UINT msg, WPARAM wp,
 LPARAM lp)
{
    PROPSHEETPAGEW *page;
    struct detail_data *data;

    TRACE("(%p, %08x, %08lx, %08lx)\n", hwnd, msg, wp, lp);

    switch (msg)
    {
    case WM_INITDIALOG:
        page = (PROPSHEETPAGEW *)lp;
        data = (struct detail_data *)page->lParam;
        create_cert_details_list(hwnd, data);
        if (!(data->pCertViewInfo->dwFlags & CRYPTUI_ENABLE_EDITPROPERTIES))
            EnableWindow(GetDlgItem(hwnd, IDC_EDITPROPERTIES), FALSE);
        if (data->pCertViewInfo->dwFlags & CRYPTUI_DISABLE_EXPORT)
            EnableWindow(GetDlgItem(hwnd, IDC_EXPORT), FALSE);
        break;
    case WM_NOTIFY:
    {
        NMITEMACTIVATE *nm;
        HWND list = GetDlgItem(hwnd, IDC_DETAIL_LIST);

        nm = (NMITEMACTIVATE*)lp;
        if (nm->hdr.hwndFrom == list && nm->uNewState & LVN_ITEMACTIVATE
         && nm->hdr.code == LVN_ITEMCHANGED)
        {
            data = (struct detail_data *)nm->lParam;
            if (nm->iItem >= 0 && data && nm->iItem < data->cFields)
            {
                WCHAR buf[MAX_STRING_LEN], *val = NULL;
                HWND valueCtl = GetDlgItem(hwnd, IDC_DETAIL_VALUE);

                if (data->fields[nm->iItem].create)
                    val = data->fields[nm->iItem].create(
                     data->pCertViewInfo->pCertContext,
                     data->fields[nm->iItem].param);
                else
                {
                    LVITEMW item;
                    int res;

                    item.cchTextMax = sizeof(buf) / sizeof(buf[0]);
                    item.mask = LVIF_TEXT;
                    item.pszText = buf;
                    item.iItem = nm->iItem;
                    item.iSubItem = 1;
                    res = SendMessageW(list, LVM_GETITEMW, 0, (LPARAM)&item);
                    if (res)
                        val = buf;
                }
                /* Select all the text in the control, the next update will
                 * replace it
                 */
                SendMessageW(valueCtl, EM_SETSEL, 0, -1);
                add_unformatted_text_to_control(valueCtl, val,
                 val ? strlenW(val) : 0);
                if (val != buf)
                    HeapFree(GetProcessHeap(), 0, val);
            }
        }
        break;
    }
    case WM_COMMAND:
        switch (wp)
        {
        case IDC_EXPORT:
        {
            HWND cb = GetDlgItem(hwnd, IDC_DETAIL_SELECT);
            CRYPTUI_WIZ_EXPORT_INFO info;

            data = (struct detail_data *)SendMessageW(cb, CB_GETITEMDATA, 0, 0);
            info.dwSize = sizeof(info);
            info.pwszExportFileName = NULL;
            info.dwSubjectChoice = CRYPTUI_WIZ_EXPORT_CERT_CONTEXT;
            info.u.pCertContext = data->pCertViewInfo->pCertContext;
            info.cStores = 0;
            CryptUIWizExport(0, hwnd, NULL, &info, NULL);
            break;
        }
        case IDC_EDITPROPERTIES:
        {
            HWND cb = GetDlgItem(hwnd, IDC_DETAIL_SELECT);
            int curSel;

            curSel = SendMessageW(cb, CB_GETCURSEL, 0, 0);
            /* Actually, any index will do, since they all store the same
             * data value
             */
            data = (struct detail_data *)SendMessageW(cb, CB_GETITEMDATA,
             curSel, 0);
            show_edit_cert_properties_dialog(GetParent(hwnd), data);
            break;
        }
        case ((CBN_SELCHANGE << 16) | IDC_DETAIL_SELECT):
            refresh_details_view(hwnd);
            break;
        }
        break;
    case WM_REFRESH_VIEW:
        refresh_details_view(hwnd);
        break;
    }
    return 0;
}

static UINT CALLBACK detail_callback(HWND hwnd, UINT msg,
 PROPSHEETPAGEW *page)
{
    struct detail_data *data;

    switch (msg)
    {
    case PSPCB_RELEASE:
        data = (struct detail_data *)page->lParam;
        free_detail_fields(data);
        HeapFree(GetProcessHeap(), 0, data);
        break;
    }
    return 0;
}

static BOOL init_detail_page(PCCRYPTUI_VIEWCERTIFICATE_STRUCTW pCertViewInfo,
 BOOL *pfPropertiesChanged, PROPSHEETPAGEW *page)
{
    BOOL ret;
    struct detail_data *data = HeapAlloc(GetProcessHeap(), 0,
     sizeof(struct detail_data));

    if (data)
    {
        data->pCertViewInfo = pCertViewInfo;
        data->pfPropertiesChanged = pfPropertiesChanged;
        data->cFields = 0;
        data->fields = NULL;
        memset(page, 0, sizeof(PROPSHEETPAGEW));
        page->dwSize = sizeof(PROPSHEETPAGEW);
        page->dwFlags = PSP_USECALLBACK;
        page->pfnCallback = detail_callback;
        page->hInstance = hInstance;
        page->u.pszTemplate = MAKEINTRESOURCEW(IDD_DETAIL);
        page->pfnDlgProc = detail_dlg_proc;
        page->lParam = (LPARAM)data;
        ret = TRUE;
    }
    else
        ret = FALSE;
    return ret;
}

struct hierarchy_data
{
    PCCRYPTUI_VIEWCERTIFICATE_STRUCTW pCertViewInfo;
    HIMAGELIST imageList;
    DWORD selectedCert;
};

static LPARAM index_to_lparam(struct hierarchy_data *data, DWORD index)
{
    CRYPT_PROVIDER_SGNR *provSigner = WTHelperGetProvSignerFromChain(
     (CRYPT_PROVIDER_DATA *)data->pCertViewInfo->u.pCryptProviderData,
     data->pCertViewInfo->idxSigner, data->pCertViewInfo->fCounterSigner,
     data->pCertViewInfo->idxCounterSigner);

    /* Takes advantage of the fact that a pointer is 32-bit aligned, and
     * therefore always even.
     */
    if (index == provSigner->csCertChain - 1)
        return (LPARAM)data;
    return index << 1 | 1;
}

static inline DWORD lparam_to_index(struct hierarchy_data *data, LPARAM lp)
{
    CRYPT_PROVIDER_SGNR *provSigner = WTHelperGetProvSignerFromChain(
     (CRYPT_PROVIDER_DATA *)data->pCertViewInfo->u.pCryptProviderData,
     data->pCertViewInfo->idxSigner, data->pCertViewInfo->fCounterSigner,
     data->pCertViewInfo->idxCounterSigner);

    if (!(lp & 1))
        return provSigner->csCertChain - 1;
    return lp >> 1;
}

static struct hierarchy_data *get_hierarchy_data_from_tree_item(HWND tree,
 HTREEITEM hItem)
{
    struct hierarchy_data *data = NULL;
    HTREEITEM root = NULL;

    do {
        HTREEITEM parent = (HTREEITEM)SendMessageW(tree, TVM_GETNEXTITEM,
         TVGN_PARENT, (LPARAM)hItem);

        if (!parent)
            root = hItem;
        hItem = parent;
    } while (hItem);
    if (root)
    {
        TVITEMW item;

        item.mask = TVIF_PARAM;
        item.hItem = root;
        SendMessageW(tree, TVM_GETITEMW, 0, (LPARAM)&item);
        data = (struct hierarchy_data *)item.lParam;
    }
    return data;
}

static WCHAR *get_cert_display_name(PCCERT_CONTEXT cert)
{
    WCHAR *name = get_cert_property_as_string(cert, CERT_FRIENDLY_NAME_PROP_ID);

    if (!name)
        name = get_cert_name_string(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0);
    return name;
}

static void show_cert_chain(HWND hwnd, struct hierarchy_data *data)
{
    HWND tree = GetDlgItem(hwnd, IDC_CERTPATH);
    CRYPT_PROVIDER_SGNR *provSigner = WTHelperGetProvSignerFromChain(
     (CRYPT_PROVIDER_DATA *)data->pCertViewInfo->u.pCryptProviderData,
     data->pCertViewInfo->idxSigner, data->pCertViewInfo->fCounterSigner,
     data->pCertViewInfo->idxCounterSigner);
    DWORD i;
    HTREEITEM parent = NULL;

    SendMessageW(tree, TVM_SETIMAGELIST, TVSIL_NORMAL, (LPARAM)data->imageList);
    for (i = provSigner->csCertChain; i; i--)
    {
        LPWSTR name;

        name = get_cert_display_name(provSigner->pasCertChain[i - 1].pCert);
        if (name)
        {
            TVINSERTSTRUCTW tvis;

            tvis.hParent = parent;
            tvis.hInsertAfter = TVI_LAST;
            tvis.u.item.mask = TVIF_TEXT | TVIF_STATE | TVIF_IMAGE |
             TVIF_SELECTEDIMAGE | TVIF_PARAM;
            tvis.u.item.pszText = name;
            tvis.u.item.state = TVIS_EXPANDED;
            tvis.u.item.stateMask = TVIS_EXPANDED;
            if (i == 1 &&
             (provSigner->pChainContext->TrustStatus.dwErrorStatus &
             CERT_TRUST_IS_PARTIAL_CHAIN))
            {
                /* The root of the chain has a special case:  if the chain is
                 * a partial chain, the icon is a warning icon rather than an
                 * error icon.
                 */
                tvis.u.item.iImage = 2;
            }
            else if (provSigner->pasCertChain[i - 1].pChainElement->TrustStatus.
             dwErrorStatus == 0)
                tvis.u.item.iImage = 0;
            else
                tvis.u.item.iImage = 1;
            tvis.u.item.iSelectedImage = tvis.u.item.iImage;
            tvis.u.item.lParam = index_to_lparam(data, i - 1);
            parent = (HTREEITEM)SendMessageW(tree, TVM_INSERTITEMW, 0,
             (LPARAM)&tvis);
            HeapFree(GetProcessHeap(), 0, name);
        }
    }
}

static void set_certificate_status(HWND hwnd, const CRYPT_PROVIDER_CERT *cert)
{
    /* Select all the text in the control, the next update will replace it */
    SendMessageW(hwnd, EM_SETSEL, 0, -1);
    /* Set the highest priority error messages first. */
    if (!(cert->dwConfidence & CERT_CONFIDENCE_SIG))
        add_string_resource_to_control(hwnd, IDS_CERTIFICATE_BAD_SIGNATURE);
    else if (!(cert->dwConfidence & CERT_CONFIDENCE_TIME))
        add_string_resource_to_control(hwnd, IDS_CERTIFICATE_BAD_TIME);
    else if (!(cert->dwConfidence & CERT_CONFIDENCE_TIMENEST))
        add_string_resource_to_control(hwnd, IDS_CERTIFICATE_BAD_TIMENEST);
    else if (cert->dwRevokedReason)
        add_string_resource_to_control(hwnd, IDS_CERTIFICATE_REVOKED);
    else
        add_string_resource_to_control(hwnd, IDS_CERTIFICATE_VALID);
}

static void set_certificate_status_for_end_cert(HWND hwnd,
 PCCRYPTUI_VIEWCERTIFICATE_STRUCTW pCertViewInfo)
{
    HWND status = GetDlgItem(hwnd, IDC_CERTIFICATESTATUSTEXT);
    CRYPT_PROVIDER_SGNR *provSigner = WTHelperGetProvSignerFromChain(
     (CRYPT_PROVIDER_DATA *)pCertViewInfo->u.pCryptProviderData,
     pCertViewInfo->idxSigner, pCertViewInfo->fCounterSigner,
     pCertViewInfo->idxCounterSigner);
    CRYPT_PROVIDER_CERT *provCert = WTHelperGetProvCertFromChain(provSigner,
     pCertViewInfo->idxCert);

    set_certificate_status(status, provCert);
}

static void show_cert_hierarchy(HWND hwnd, struct hierarchy_data *data)
{
    /* Disable view certificate button until a certificate is selected */
    EnableWindow(GetDlgItem(hwnd, IDC_VIEWCERTIFICATE), FALSE);
    show_cert_chain(hwnd, data);
    set_certificate_status_for_end_cert(hwnd, data->pCertViewInfo);
}

static void show_dialog_for_selected_cert(HWND hwnd)
{
    HWND tree = GetDlgItem(hwnd, IDC_CERTPATH);
    TVITEMW item;
    struct hierarchy_data *data;
    DWORD selection;

    memset(&item, 0, sizeof(item));
    item.mask = TVIF_HANDLE | TVIF_PARAM;
    item.hItem = (HTREEITEM)SendMessageW(tree, TVM_GETNEXTITEM, TVGN_CARET,
     (LPARAM)NULL);
    SendMessageW(tree, TVM_GETITEMW, 0, (LPARAM)&item);
    data = get_hierarchy_data_from_tree_item(tree, item.hItem);
    selection = lparam_to_index(data, item.lParam);
    if (selection != 0)
    {
        CRYPT_PROVIDER_SGNR *provSigner;
        CRYPTUI_VIEWCERTIFICATE_STRUCTW viewInfo;
        BOOL changed = FALSE;

        provSigner = WTHelperGetProvSignerFromChain(
         (CRYPT_PROVIDER_DATA *)data->pCertViewInfo->u.pCryptProviderData,
         data->pCertViewInfo->idxSigner,
         data->pCertViewInfo->fCounterSigner,
         data->pCertViewInfo->idxCounterSigner);
        memset(&viewInfo, 0, sizeof(viewInfo));
        viewInfo.dwSize = sizeof(viewInfo);
        viewInfo.dwFlags = data->pCertViewInfo->dwFlags;
        viewInfo.szTitle = data->pCertViewInfo->szTitle;
        viewInfo.pCertContext = provSigner->pasCertChain[selection].pCert;
        viewInfo.cStores = data->pCertViewInfo->cStores;
        viewInfo.rghStores = data->pCertViewInfo->rghStores;
        viewInfo.cPropSheetPages = data->pCertViewInfo->cPropSheetPages;
        viewInfo.rgPropSheetPages = data->pCertViewInfo->rgPropSheetPages;
        viewInfo.nStartPage = data->pCertViewInfo->nStartPage;
        CryptUIDlgViewCertificateW(&viewInfo, &changed);
        if (changed)
        {
            /* Delete the contents of the tree */
            SendMessageW(tree, TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT);
            /* Reinitialize the tree */
            show_cert_hierarchy(hwnd, data);
        }
    }
}

static LRESULT CALLBACK hierarchy_dlg_proc(HWND hwnd, UINT msg, WPARAM wp,
 LPARAM lp)
{
    PROPSHEETPAGEW *page;
    struct hierarchy_data *data;
    LRESULT ret = 0;
    HWND tree = GetDlgItem(hwnd, IDC_CERTPATH);

    TRACE("(%p, %08x, %08lx, %08lx)\n", hwnd, msg, wp, lp);

    switch (msg)
    {
    case WM_INITDIALOG:
        page = (PROPSHEETPAGEW *)lp;
        data = (struct hierarchy_data *)page->lParam;
        show_cert_hierarchy(hwnd, data);
        break;
    case WM_NOTIFY:
    {
        NMHDR *hdr;

        hdr = (NMHDR *)lp;
        switch (hdr->code)
        {
        case TVN_SELCHANGEDW:
        {
            NMTREEVIEWW *nm = (NMTREEVIEWW*)lp;
            DWORD selection;
            CRYPT_PROVIDER_SGNR *provSigner;

            data = get_hierarchy_data_from_tree_item(tree, nm->itemNew.hItem);
            selection = lparam_to_index(data, nm->itemNew.lParam);
            provSigner = WTHelperGetProvSignerFromChain(
             (CRYPT_PROVIDER_DATA *)data->pCertViewInfo->u.pCryptProviderData,
             data->pCertViewInfo->idxSigner,
             data->pCertViewInfo->fCounterSigner,
             data->pCertViewInfo->idxCounterSigner);
            EnableWindow(GetDlgItem(hwnd, IDC_VIEWCERTIFICATE), selection != 0);
            set_certificate_status(GetDlgItem(hwnd, IDC_CERTIFICATESTATUSTEXT),
             &provSigner->pasCertChain[selection]);
            break;
        }
        case NM_DBLCLK:
            show_dialog_for_selected_cert(hwnd);
            SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, 1);
            ret = 1;
            break;
        }
        break;
    }
    case WM_COMMAND:
        switch (wp)
        {
        case IDC_VIEWCERTIFICATE:
            show_dialog_for_selected_cert(hwnd);
            break;
        }
        break;
    case WM_REFRESH_VIEW:
    {
        TVITEMW item;

        /* Get hierarchy data */
        memset(&item, 0, sizeof(item));
        item.mask = TVIF_HANDLE | TVIF_PARAM;
        item.hItem = (HTREEITEM)SendMessageW(tree, TVM_GETNEXTITEM, TVGN_ROOT,
         (LPARAM)NULL);
        data = get_hierarchy_data_from_tree_item(tree, item.hItem);
        /* Delete the contents of the tree */
        SendMessageW(tree, TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT);
        /* Reinitialize the tree */
        show_cert_hierarchy(hwnd, data);
        break;
    }
    }
    return ret;
}

static UINT CALLBACK hierarchy_callback(HWND hwnd, UINT msg,
 PROPSHEETPAGEW *page)
{
    struct hierarchy_data *data;

    switch (msg)
    {
    case PSPCB_RELEASE:
        data = (struct hierarchy_data *)page->lParam;
        ImageList_Destroy(data->imageList);
        HeapFree(GetProcessHeap(), 0, data);
        break;
    }
    return 0;
}

static BOOL init_hierarchy_page(PCCRYPTUI_VIEWCERTIFICATE_STRUCTW pCertViewInfo,
 PROPSHEETPAGEW *page)
{
    struct hierarchy_data *data = HeapAlloc(GetProcessHeap(), 0,
     sizeof(struct hierarchy_data));
    BOOL ret = FALSE;

    if (data)
    {
        data->imageList = ImageList_Create(16, 16, ILC_COLOR4 | ILC_MASK, 2, 0);
        if (data->imageList)
        {
            HBITMAP bmp;
            COLORREF backColor = RGB(255, 0, 255);

            data->pCertViewInfo = pCertViewInfo;
            data->selectedCert = 0xffffffff;

            bmp = LoadBitmapW(hInstance, MAKEINTRESOURCEW(IDB_SMALL_ICONS));
            ImageList_AddMasked(data->imageList, bmp, backColor);
            DeleteObject(bmp);
            ImageList_SetBkColor(data->imageList, CLR_NONE);

            memset(page, 0, sizeof(PROPSHEETPAGEW));
            page->dwSize = sizeof(PROPSHEETPAGEW);
            page->dwFlags = PSP_USECALLBACK;
            page->hInstance = hInstance;
            page->u.pszTemplate = MAKEINTRESOURCEW(IDD_HIERARCHY);
            page->pfnDlgProc = hierarchy_dlg_proc;
            page->lParam = (LPARAM)data;
            page->pfnCallback = hierarchy_callback;
            ret = TRUE;
        }
        else
            HeapFree(GetProcessHeap(), 0, data);
    }
    return ret;
}

static int CALLBACK cert_prop_sheet_proc(HWND hwnd, UINT msg, LPARAM lp)
{
    RECT rc;
    POINT topLeft;

    TRACE("(%p, %08x, %08lx)\n", hwnd, msg, lp);

    switch (msg)
    {
    case PSCB_INITIALIZED:
        /* Get cancel button's position.. */
        GetWindowRect(GetDlgItem(hwnd, IDCANCEL), &rc);
        topLeft.x = rc.left;
        topLeft.y = rc.top;
        ScreenToClient(hwnd, &topLeft);
        /* hide the cancel button.. */
        ShowWindow(GetDlgItem(hwnd, IDCANCEL), FALSE);
        /* get the OK button's size.. */
        GetWindowRect(GetDlgItem(hwnd, IDOK), &rc);
        /* and move the OK button to the cancel button's original position. */
        MoveWindow(GetDlgItem(hwnd, IDOK), topLeft.x, topLeft.y,
         rc.right - rc.left, rc.bottom - rc.top, FALSE);
        GetWindowRect(GetDlgItem(hwnd, IDOK), &rc);
        break;
    }
    return 0;
}

static BOOL show_cert_dialog(PCCRYPTUI_VIEWCERTIFICATE_STRUCTW pCertViewInfo,
 CRYPT_PROVIDER_CERT *provCert, BOOL *pfPropertiesChanged)
{
    static const WCHAR riched[] = { 'r','i','c','h','e','d','2','0',0 };
    DWORD nPages;
    PROPSHEETPAGEW *pages;
    BOOL ret = FALSE;
    HMODULE lib = LoadLibraryW(riched);

    nPages = pCertViewInfo->cPropSheetPages + 1; /* one for the General tab */
    if (!(pCertViewInfo->dwFlags & CRYPTUI_HIDE_DETAILPAGE))
        nPages++;
    if (!(pCertViewInfo->dwFlags & CRYPTUI_HIDE_HIERARCHYPAGE))
        nPages++;
    pages = HeapAlloc(GetProcessHeap(), 0, nPages * sizeof(PROPSHEETPAGEW));
    if (pages)
    {
        PROPSHEETHEADERW hdr;
        CRYPTUI_INITDIALOG_STRUCT *init = NULL;
        DWORD i;

        memset(&hdr, 0, sizeof(hdr));
        hdr.dwSize = sizeof(hdr);
        hdr.dwFlags = PSH_NOAPPLYNOW | PSH_PROPSHEETPAGE | PSH_USECALLBACK;
        hdr.hInstance = hInstance;
        if (pCertViewInfo->szTitle)
            hdr.pszCaption = pCertViewInfo->szTitle;
        else
            hdr.pszCaption = MAKEINTRESOURCEW(IDS_CERTIFICATE);
        init_general_page(pCertViewInfo, &pages[hdr.nPages++]);
        if (!(pCertViewInfo->dwFlags & CRYPTUI_HIDE_DETAILPAGE))
        {
            if (init_detail_page(pCertViewInfo, pfPropertiesChanged,
             &pages[hdr.nPages]))
                hdr.nPages++;
        }
        if (!(pCertViewInfo->dwFlags & CRYPTUI_HIDE_HIERARCHYPAGE))
        {
            if (init_hierarchy_page(pCertViewInfo, &pages[hdr.nPages]))
                hdr.nPages++;
        }
        /* Copy each additional page, and create the init dialog struct for it
         */
        if (pCertViewInfo->cPropSheetPages)
        {
            init = HeapAlloc(GetProcessHeap(), 0,
             pCertViewInfo->cPropSheetPages *
             sizeof(CRYPTUI_INITDIALOG_STRUCT));
            if (init)
            {
                for (i = 0; i < pCertViewInfo->cPropSheetPages; i++)
                {
                    memcpy(&pages[hdr.nPages + i],
                     &pCertViewInfo->rgPropSheetPages[i],
                     sizeof(PROPSHEETPAGEW));
                    init[i].lParam = pCertViewInfo->rgPropSheetPages[i].lParam;
                    init[i].pCertContext = pCertViewInfo->pCertContext;
                    pages[hdr.nPages + i].lParam = (LPARAM)&init[i];
                }
                if (pCertViewInfo->nStartPage & 0x8000)
                {
                    /* Start page index is relative to the number of default
                     * pages
                     */
                    hdr.u2.nStartPage = pCertViewInfo->nStartPage + hdr.nPages;
                }
                else
                    hdr.u2.nStartPage = pCertViewInfo->nStartPage;
                hdr.nPages = nPages;
                ret = TRUE;
            }
            else
                SetLastError(ERROR_OUTOFMEMORY);
        }
        else
        {
            /* Ignore the relative flag if there aren't any additional pages */
            hdr.u2.nStartPage = pCertViewInfo->nStartPage & 0x7fff;
            ret = TRUE;
        }
        if (ret)
        {
            INT_PTR l;

            hdr.u3.ppsp = pages;
            hdr.pfnCallback = cert_prop_sheet_proc;
            l = PropertySheetW(&hdr);
            if (l == 0)
            {
                SetLastError(ERROR_CANCELLED);
                ret = FALSE;
            }
        }
        HeapFree(GetProcessHeap(), 0, init);
        HeapFree(GetProcessHeap(), 0, pages);
    }
    else
        SetLastError(ERROR_OUTOFMEMORY);
    FreeLibrary(lib);
    return ret;
}

/***********************************************************************
 *		CryptUIDlgViewCertificateW (CRYPTUI.@)
 */
BOOL WINAPI CryptUIDlgViewCertificateW(
 PCCRYPTUI_VIEWCERTIFICATE_STRUCTW pCertViewInfo, BOOL *pfPropertiesChanged)
{
    static GUID generic_cert_verify = WINTRUST_ACTION_GENERIC_CERT_VERIFY;
    CRYPTUI_VIEWCERTIFICATE_STRUCTW viewInfo;
    WINTRUST_DATA wvt;
    WINTRUST_CERT_INFO cert;
    BOOL ret = FALSE;
    CRYPT_PROVIDER_SGNR *signer;
    CRYPT_PROVIDER_CERT *provCert = NULL;

    TRACE("(%p, %p)\n", pCertViewInfo, pfPropertiesChanged);

    if (pCertViewInfo->dwSize != sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCTW))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    /* Make a local copy in case we have to call WinVerifyTrust ourselves */
    memcpy(&viewInfo, pCertViewInfo, sizeof(viewInfo));
    if (!viewInfo.u.hWVTStateData)
    {
        memset(&wvt, 0, sizeof(wvt));
        wvt.cbStruct = sizeof(wvt);
        wvt.dwUIChoice = WTD_UI_NONE;
        if (viewInfo.dwFlags &
         CRYPTUI_ENABLE_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT)
            wvt.fdwRevocationChecks |= WTD_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT;
        if (viewInfo.dwFlags & CRYPTUI_ENABLE_REVOCATION_CHECK_END_CERT)
            wvt.fdwRevocationChecks |= WTD_REVOCATION_CHECK_END_CERT;
        if (viewInfo.dwFlags & CRYPTUI_ENABLE_REVOCATION_CHECK_CHAIN)
            wvt.fdwRevocationChecks |= WTD_REVOCATION_CHECK_CHAIN;
        wvt.dwUnionChoice = WTD_CHOICE_CERT;
        memset(&cert, 0, sizeof(cert));
        cert.cbStruct = sizeof(cert);
        cert.psCertContext = (CERT_CONTEXT *)viewInfo.pCertContext;
        cert.chStores = viewInfo.cStores;
        cert.pahStores = viewInfo.rghStores;
        wvt.u.pCert = &cert;
        wvt.dwStateAction = WTD_STATEACTION_VERIFY;
        WinVerifyTrust(NULL, &generic_cert_verify, &wvt);
        viewInfo.u.pCryptProviderData =
         WTHelperProvDataFromStateData(wvt.hWVTStateData);
        signer = WTHelperGetProvSignerFromChain(
         (CRYPT_PROVIDER_DATA *)viewInfo.u.pCryptProviderData, 0, FALSE, 0);
        provCert = WTHelperGetProvCertFromChain(signer, 0);
        ret = TRUE;
    }
    else
    {
        viewInfo.u.pCryptProviderData =
         WTHelperProvDataFromStateData(viewInfo.u.hWVTStateData);
        signer = WTHelperGetProvSignerFromChain(
         (CRYPT_PROVIDER_DATA *)viewInfo.u.pCryptProviderData,
         viewInfo.idxSigner, viewInfo.fCounterSigner,
         viewInfo.idxCounterSigner);
        provCert = WTHelperGetProvCertFromChain(signer, viewInfo.idxCert);
        ret = TRUE;
    }
    if (ret)
    {
        ret = show_cert_dialog(&viewInfo, provCert, pfPropertiesChanged);
        if (!viewInfo.u.hWVTStateData)
        {
            wvt.dwStateAction = WTD_STATEACTION_CLOSE;
            WinVerifyTrust(NULL, &generic_cert_verify, &wvt);
        }
    }
    return ret;
}

/***********************************************************************
 *		CryptUIDlgViewContext (CRYPTUI.@)
 */
BOOL WINAPI CryptUIDlgViewContext(DWORD dwContextType, LPVOID pvContext,
 HWND hwnd, LPCWSTR pwszTitle, DWORD dwFlags, LPVOID pvReserved)
{
    BOOL ret;

    TRACE("(%d, %p, %p, %s, %08x, %p)\n", dwContextType, pvContext, hwnd,
     debugstr_w(pwszTitle), dwFlags, pvReserved);

    switch (dwContextType)
    {
    case CERT_STORE_CERTIFICATE_CONTEXT:
    {
        CRYPTUI_VIEWCERTIFICATE_STRUCTW viewInfo;

        memset(&viewInfo, 0, sizeof(viewInfo));
        viewInfo.dwSize = sizeof(viewInfo);
        viewInfo.hwndParent = hwnd;
        viewInfo.szTitle = pwszTitle;
        viewInfo.pCertContext = pvContext;
        ret = CryptUIDlgViewCertificateW(&viewInfo, NULL);
        break;
    }
    default:
        FIXME("unimplemented for context type %d\n", dwContextType);
        SetLastError(E_INVALIDARG);
        ret = FALSE;
    }
    return ret;
}

/* Decodes a cert's basic constraints extension (either szOID_BASIC_CONSTRAINTS
 * or szOID_BASIC_CONSTRAINTS2, whichever is present) to determine if it
 * should be a CA.  If neither extension is present, returns
 * defaultIfNotSpecified.
 */
static BOOL is_ca_cert(PCCERT_CONTEXT cert, BOOL defaultIfNotSpecified)
{
    BOOL isCA = defaultIfNotSpecified;
    PCERT_EXTENSION ext = CertFindExtension(szOID_BASIC_CONSTRAINTS,
     cert->pCertInfo->cExtension, cert->pCertInfo->rgExtension);

    if (ext)
    {
        CERT_BASIC_CONSTRAINTS_INFO *info;
        DWORD size = 0;

        if (CryptDecodeObjectEx(X509_ASN_ENCODING, szOID_BASIC_CONSTRAINTS,
         ext->Value.pbData, ext->Value.cbData, CRYPT_DECODE_ALLOC_FLAG,
         NULL, &info, &size))
        {
            if (info->SubjectType.cbData == 1)
                isCA = info->SubjectType.pbData[0] & CERT_CA_SUBJECT_FLAG;
            LocalFree(info);
        }
    }
    else
    {
        ext = CertFindExtension(szOID_BASIC_CONSTRAINTS2,
         cert->pCertInfo->cExtension, cert->pCertInfo->rgExtension);
        if (ext)
        {
            CERT_BASIC_CONSTRAINTS2_INFO info;
            DWORD size = sizeof(CERT_BASIC_CONSTRAINTS2_INFO);

            if (CryptDecodeObjectEx(X509_ASN_ENCODING,
             szOID_BASIC_CONSTRAINTS2, ext->Value.pbData, ext->Value.cbData,
             0, NULL, &info, &size))
                isCA = info.fCA;
        }
    }
    return isCA;
}

static HCERTSTORE choose_store_for_cert(PCCERT_CONTEXT cert)
{
    LPCWSTR storeName;

    if (is_ca_cert(cert, TRUE))
        storeName = ca;
    else
        storeName = addressBook;
    return CertOpenStore(CERT_STORE_PROV_SYSTEM_W, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER, storeName);
}

static BOOL import_cert(PCCERT_CONTEXT cert, HCERTSTORE hDestCertStore)
{
    HCERTSTORE store;
    BOOL ret;

    if (!cert)
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    if (hDestCertStore) store = hDestCertStore;
    else
    {
        if (!(store = choose_store_for_cert(cert)))
        {
            WARN("unable to open certificate store\n");
            return FALSE;
        }
    }
    ret = CertAddCertificateContextToStore(store, cert,
     CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES, NULL);
    if (!hDestCertStore) CertCloseStore(store, 0);
    return ret;
}

static BOOL import_crl(PCCRL_CONTEXT crl, HCERTSTORE hDestCertStore)
{
    HCERTSTORE store;
    BOOL ret;

    if (!crl)
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    if (hDestCertStore) store = hDestCertStore;
    else
    {
        if (!(store = CertOpenStore(CERT_STORE_PROV_SYSTEM_W, 0, 0,
         CERT_SYSTEM_STORE_CURRENT_USER, ca)))
        {
            WARN("unable to open certificate store\n");
            return FALSE;
        }
    }
    ret = CertAddCRLContextToStore(store, crl,
     CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES, NULL);
    if (!hDestCertStore) CertCloseStore(store, 0);
    return ret;
}

static BOOL import_ctl(PCCTL_CONTEXT ctl, HCERTSTORE hDestCertStore)
{
    HCERTSTORE store;
    BOOL ret;

    if (!ctl)
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    if (hDestCertStore) store = hDestCertStore;
    else
    {
        static const WCHAR trust[] = { 'T','r','u','s','t',0 };

        if (!(store = CertOpenStore(CERT_STORE_PROV_SYSTEM_W, 0, 0,
         CERT_SYSTEM_STORE_CURRENT_USER, trust)))
        {
            WARN("unable to open certificate store\n");
            return FALSE;
        }
    }
    ret = CertAddCTLContextToStore(store, ctl,
     CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES, NULL);
    if (!hDestCertStore) CertCloseStore(store, 0);
    return ret;
}

/* Checks type, a type such as CERT_QUERY_CONTENT_CERT returned by
 * CryptQueryObject, against the allowed types.  Returns TRUE if the
 * type is allowed, FALSE otherwise.
 */
static BOOL check_context_type(DWORD dwFlags, DWORD type)
{
    BOOL ret;

    if (dwFlags &
     (CRYPTUI_WIZ_IMPORT_ALLOW_CERT | CRYPTUI_WIZ_IMPORT_ALLOW_CRL |
     CRYPTUI_WIZ_IMPORT_ALLOW_CTL))
    {
        switch (type)
        {
        case CERT_QUERY_CONTENT_CERT:
        case CERT_QUERY_CONTENT_SERIALIZED_CERT:
            ret = dwFlags & CRYPTUI_WIZ_IMPORT_ALLOW_CERT;
            break;
        case CERT_QUERY_CONTENT_CRL:
        case CERT_QUERY_CONTENT_SERIALIZED_CRL:
            ret = dwFlags & CRYPTUI_WIZ_IMPORT_ALLOW_CRL;
            break;
        case CERT_QUERY_CONTENT_CTL:
        case CERT_QUERY_CONTENT_SERIALIZED_CTL:
            ret = dwFlags & CRYPTUI_WIZ_IMPORT_ALLOW_CTL;
            break;
        default:
            /* The remaining types contain more than one type, so allow
             * any combination.
             */
            ret = TRUE;
        }
    }
    else
    {
        /* No allowed types specified, so any type is allowed */
        ret = TRUE;
    }
    if (!ret)
        SetLastError(E_INVALIDARG);
    return ret;
}


static void import_warning(DWORD dwFlags, HWND hwnd, LPCWSTR szTitle,
 int warningID)
{
    if (!(dwFlags & CRYPTUI_WIZ_NO_UI))
    {
        WCHAR title[MAX_STRING_LEN], error[MAX_STRING_LEN];
        LPCWSTR pTitle;

        if (szTitle)
            pTitle = szTitle;
        else
        {
            LoadStringW(hInstance, IDS_IMPORT_WIZARD, title,
             sizeof(title) / sizeof(title[0]));
            pTitle = title;
        }
        LoadStringW(hInstance, warningID, error,
         sizeof(error) / sizeof(error[0]));
        MessageBoxW(hwnd, error, pTitle, MB_ICONERROR | MB_OK);
    }
}

static void import_warn_type_mismatch(DWORD dwFlags, HWND hwnd, LPCWSTR szTitle)
{
    import_warning(dwFlags, hwnd, szTitle, IDS_IMPORT_TYPE_MISMATCH);
}

static BOOL check_store_context_type(DWORD dwFlags, HCERTSTORE store)
{
    BOOL ret;

    if (dwFlags &
     (CRYPTUI_WIZ_IMPORT_ALLOW_CERT | CRYPTUI_WIZ_IMPORT_ALLOW_CRL |
     CRYPTUI_WIZ_IMPORT_ALLOW_CTL))
    {
        PCCERT_CONTEXT cert;
        PCCRL_CONTEXT crl;
        PCCTL_CONTEXT ctl;

        ret = TRUE;
        if ((cert = CertEnumCertificatesInStore(store, NULL)))
        {
            CertFreeCertificateContext(cert);
            if (!(dwFlags & CRYPTUI_WIZ_IMPORT_ALLOW_CERT))
                ret = FALSE;
        }
        if (ret && (crl = CertEnumCRLsInStore(store, NULL)))
        {
            CertFreeCRLContext(crl);
            if (!(dwFlags & CRYPTUI_WIZ_IMPORT_ALLOW_CRL))
                ret = FALSE;
        }
        if (ret && (ctl = CertEnumCTLsInStore(store, NULL)))
        {
            CertFreeCTLContext(ctl);
            if (!(dwFlags & CRYPTUI_WIZ_IMPORT_ALLOW_CTL))
                ret = FALSE;
        }
    }
    else
        ret = TRUE;
    if (!ret)
        SetLastError(E_INVALIDARG);
    return ret;
}

static BOOL import_store(DWORD dwFlags, HWND hwnd, LPCWSTR szTitle,
 HCERTSTORE source, HCERTSTORE dest)
{
    BOOL ret;

    if ((ret = check_store_context_type(dwFlags, source)))
    {
        PCCERT_CONTEXT cert = NULL;
        PCCRL_CONTEXT crl = NULL;
        PCCTL_CONTEXT ctl = NULL;

        do {
            cert = CertEnumCertificatesInStore(source, cert);
            if (cert)
                ret = import_cert(cert, dest);
        } while (ret && cert);
        do {
            crl = CertEnumCRLsInStore(source, crl);
            if (crl)
                ret = import_crl(crl, dest);
        } while (ret && crl);
        do {
            ctl = CertEnumCTLsInStore(source, ctl);
            if (ctl)
                ret = import_ctl(ctl, dest);
        } while (ret && ctl);
    }
    else
        import_warn_type_mismatch(dwFlags, hwnd, szTitle);
    return ret;
}

static HCERTSTORE open_store_from_file(DWORD dwFlags, LPCWSTR fileName,
 DWORD *pContentType)
{
    HCERTSTORE store = NULL;
    DWORD contentType = 0, expectedContentTypeFlags;

    if (dwFlags &
     (CRYPTUI_WIZ_IMPORT_ALLOW_CERT | CRYPTUI_WIZ_IMPORT_ALLOW_CRL |
     CRYPTUI_WIZ_IMPORT_ALLOW_CTL))
    {
        expectedContentTypeFlags =
         CERT_QUERY_CONTENT_FLAG_SERIALIZED_STORE |
         CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED |
         CERT_QUERY_CONTENT_FLAG_PFX;
        if (dwFlags & CRYPTUI_WIZ_IMPORT_ALLOW_CERT)
            expectedContentTypeFlags |=
             CERT_QUERY_CONTENT_FLAG_CERT |
             CERT_QUERY_CONTENT_FLAG_SERIALIZED_CERT;
        if (dwFlags & CRYPTUI_WIZ_IMPORT_ALLOW_CRL)
            expectedContentTypeFlags |=
             CERT_QUERY_CONTENT_FLAG_SERIALIZED_CRL |
             CERT_QUERY_CONTENT_FLAG_CRL;
        if (dwFlags & CRYPTUI_WIZ_IMPORT_ALLOW_CTL)
            expectedContentTypeFlags |=
             CERT_QUERY_CONTENT_FLAG_CTL |
             CERT_QUERY_CONTENT_FLAG_SERIALIZED_CTL;
    }
    else
        expectedContentTypeFlags =
         CERT_QUERY_CONTENT_FLAG_CERT |
         CERT_QUERY_CONTENT_FLAG_CTL |
         CERT_QUERY_CONTENT_FLAG_CRL |
         CERT_QUERY_CONTENT_FLAG_SERIALIZED_STORE |
         CERT_QUERY_CONTENT_FLAG_SERIALIZED_CERT |
         CERT_QUERY_CONTENT_FLAG_SERIALIZED_CTL |
         CERT_QUERY_CONTENT_FLAG_SERIALIZED_CRL |
         CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED |
         CERT_QUERY_CONTENT_FLAG_PFX;

    CryptQueryObject(CERT_QUERY_OBJECT_FILE, fileName,
     expectedContentTypeFlags, CERT_QUERY_FORMAT_FLAG_ALL, 0, NULL,
     &contentType, NULL, &store, NULL, NULL);
    if (pContentType)
        *pContentType = contentType;
    return store;
}

static BOOL import_file(DWORD dwFlags, HWND hwnd, LPCWSTR szTitle,
 LPCWSTR fileName, HCERTSTORE dest)
{
    HCERTSTORE source;
    BOOL ret;

    if ((source = open_store_from_file(dwFlags, fileName, NULL)))
    {
        ret = import_store(dwFlags, hwnd, szTitle, source, dest);
        CertCloseStore(source, 0);
    }
    else
        ret = FALSE;
    return ret;
}

struct ImportWizData
{
    HFONT titleFont;
    DWORD dwFlags;
    LPCWSTR pwszWizardTitle;
    CRYPTUI_WIZ_IMPORT_SRC_INFO importSrc;
    LPWSTR fileName;
    DWORD contentType;
    BOOL freeSource;
    HCERTSTORE hDestCertStore;
    BOOL freeDest;
    BOOL autoDest;
    BOOL success;
};

static LRESULT CALLBACK import_welcome_dlg_proc(HWND hwnd, UINT msg, WPARAM wp,
 LPARAM lp)
{
    LRESULT ret = 0;

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        struct ImportWizData *data;
        PROPSHEETPAGEW *page = (PROPSHEETPAGEW *)lp;
        WCHAR fontFace[MAX_STRING_LEN];
        HDC hDC = GetDC(hwnd);
        int height;

        data = (struct ImportWizData *)page->lParam;
        LoadStringW(hInstance, IDS_WIZARD_TITLE_FONT, fontFace,
         sizeof(fontFace) / sizeof(fontFace[0]));
        height = -MulDiv(12, GetDeviceCaps(hDC, LOGPIXELSY), 72);
        data->titleFont = CreateFontW(height, 0, 0, 0, FW_BOLD, 0, 0, 0,
         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
         DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fontFace);
        SendMessageW(GetDlgItem(hwnd, IDC_IMPORT_TITLE), WM_SETFONT,
         (WPARAM)data->titleFont, TRUE);
        ReleaseDC(hwnd, hDC);
        break;
    }
    case WM_NOTIFY:
    {
        NMHDR *hdr = (NMHDR *)lp;

        switch (hdr->code)
        {
        case PSN_SETACTIVE:
            PostMessageW(GetParent(hwnd), PSM_SETWIZBUTTONS, 0, PSWIZB_NEXT);
            ret = TRUE;
            break;
        }
        break;
    }
    }
    return ret;
}

static const WCHAR filter_cert[] = { '*','.','c','e','r',';','*','.',
 'c','r','t',0 };
static const WCHAR filter_pfx[] = { '*','.','p','f','x',';','*','.',
 'p','1','2',0 };
static const WCHAR filter_crl[] = { '*','.','c','r','l',0 };
static const WCHAR filter_ctl[] = { '*','.','s','t','l',0 };
static const WCHAR filter_serialized_store[] = { '*','.','s','s','t',0 };
static const WCHAR filter_cms[] = { '*','.','s','p','c',';','*','.',
 'p','7','b',0 };
static const WCHAR filter_all[] = { '*','.','*',0 };

struct StringToFilter
{
    int     id;
    DWORD   allowFlags;
    LPCWSTR filter;
} import_filters[] = {
 { IDS_IMPORT_FILTER_CERT, CRYPTUI_WIZ_IMPORT_ALLOW_CERT, filter_cert },
 { IDS_IMPORT_FILTER_PFX, 0, filter_pfx },
 { IDS_IMPORT_FILTER_CRL, CRYPTUI_WIZ_IMPORT_ALLOW_CRL, filter_crl },
 { IDS_IMPORT_FILTER_CTL, CRYPTUI_WIZ_IMPORT_ALLOW_CTL, filter_ctl },
 { IDS_IMPORT_FILTER_SERIALIZED_STORE, 0, filter_serialized_store },
 { IDS_IMPORT_FILTER_CMS, 0, filter_cms },
 { IDS_IMPORT_FILTER_ALL, 0, filter_all },
};

static WCHAR *make_import_file_filter(DWORD dwFlags)
{
    DWORD i;
    int len, totalLen = 2;
    LPWSTR filter = NULL, str;

    for (i = 0; i < sizeof(import_filters) / sizeof(import_filters[0]); i++)
    {
        if (!import_filters[i].allowFlags || !dwFlags ||
         (dwFlags & import_filters[i].allowFlags))
        {
            len = LoadStringW(hInstance, import_filters[i].id, (LPWSTR)&str, 0);
            totalLen += len + strlenW(import_filters[i].filter) + 2;
        }
    }
    filter = HeapAlloc(GetProcessHeap(), 0, totalLen * sizeof(WCHAR));
    if (filter)
    {
        LPWSTR ptr;

        ptr = filter;
        for (i = 0; i < sizeof(import_filters) / sizeof(import_filters[0]); i++)
        {
            if (!import_filters[i].allowFlags || !dwFlags ||
             (dwFlags & import_filters[i].allowFlags))
            {
                len = LoadStringW(hInstance, import_filters[i].id,
                 (LPWSTR)&str, 0);
                memcpy(ptr, str, len * sizeof(WCHAR));
                ptr += len;
                *ptr++ = 0;
                strcpyW(ptr, import_filters[i].filter);
                ptr += strlenW(import_filters[i].filter) + 1;
            }
        }
        *ptr++ = 0;
    }
    return filter;
}

static BOOL import_validate_filename(HWND hwnd, struct ImportWizData *data,
 LPCWSTR fileName)
{
    HANDLE file;
    BOOL ret = FALSE;

    file = CreateFileW(fileName, GENERIC_READ, FILE_SHARE_READ, NULL,
     OPEN_EXISTING, 0, NULL);
    if (file != INVALID_HANDLE_VALUE)
    {
        HCERTSTORE source = open_store_from_file(data->dwFlags, fileName,
         &data->contentType);
        int warningID = 0;

        if (!source)
            warningID = IDS_IMPORT_BAD_FORMAT;
        else if (!check_store_context_type(data->dwFlags, source))
            warningID = IDS_IMPORT_TYPE_MISMATCH;
        else
        {
            data->importSrc.dwSubjectChoice =
             CRYPTUI_WIZ_IMPORT_SUBJECT_CERT_STORE;
            data->importSrc.u.hCertStore = source;
            data->freeSource = TRUE;
            ret = TRUE;
        }
        if (warningID)
        {
            import_warning(data->dwFlags, hwnd, data->pwszWizardTitle,
             warningID);
        }
        CloseHandle(file);
    }
    else
    {
        WCHAR title[MAX_STRING_LEN], error[MAX_STRING_LEN];
        LPCWSTR pTitle;
        LPWSTR msgBuf, fullError;

        if (data->pwszWizardTitle)
            pTitle = data->pwszWizardTitle;
        else
        {
            LoadStringW(hInstance, IDS_IMPORT_WIZARD, title,
             sizeof(title) / sizeof(title[0]));
            pTitle = title;
        }
        LoadStringW(hInstance, IDS_IMPORT_OPEN_FAILED, error,
         sizeof(error) / sizeof(error[0]));
        FormatMessageW(
         FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
         GetLastError(), 0, (LPWSTR) &msgBuf, 0, NULL);
        fullError = HeapAlloc(GetProcessHeap(), 0,
         (strlenW(error) + strlenW(fileName) + strlenW(msgBuf) + 3)
         * sizeof(WCHAR));
        if (fullError)
        {
            LPWSTR ptr = fullError;

            strcpyW(ptr, error);
            ptr += strlenW(error);
            strcpyW(ptr, fileName);
            ptr += strlenW(fileName);
            *ptr++ = ':';
            *ptr++ = '\n';
            strcpyW(ptr, msgBuf);
            MessageBoxW(hwnd, fullError, pTitle, MB_ICONERROR | MB_OK);
            HeapFree(GetProcessHeap(), 0, fullError);
        }
        LocalFree(msgBuf);
    }
    return ret;
}

static LRESULT CALLBACK import_file_dlg_proc(HWND hwnd, UINT msg, WPARAM wp,
 LPARAM lp)
{
    LRESULT ret = 0;
    struct ImportWizData *data;

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        PROPSHEETPAGEW *page = (PROPSHEETPAGEW *)lp;

        data = (struct ImportWizData *)page->lParam;
        SetWindowLongPtrW(hwnd, DWLP_USER, (LPARAM)data);
        if (data->fileName)
        {
            HWND fileNameEdit = GetDlgItem(hwnd, IDC_IMPORT_FILENAME);

            SendMessageW(fileNameEdit, WM_SETTEXT, 0, (LPARAM)data->fileName);
        }
        break;
    }
    case WM_NOTIFY:
    {
        NMHDR *hdr = (NMHDR *)lp;

        switch (hdr->code)
        {
        case PSN_SETACTIVE:
            PostMessageW(GetParent(hwnd), PSM_SETWIZBUTTONS, 0,
             PSWIZB_BACK | PSWIZB_NEXT);
            ret = TRUE;
            break;
        case PSN_WIZNEXT:
        {
            HWND fileNameEdit = GetDlgItem(hwnd, IDC_IMPORT_FILENAME);
            DWORD len = SendMessageW(fileNameEdit, WM_GETTEXTLENGTH, 0, 0);

            data = (struct ImportWizData *)GetWindowLongPtrW(hwnd, DWLP_USER);
            if (!len)
            {
                import_warning(data->dwFlags, hwnd, data->pwszWizardTitle,
                 IDS_IMPORT_EMPTY_FILE);
                SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, 1);
                ret = 1;
            }
            else
            {
                LPWSTR fileName = HeapAlloc(GetProcessHeap(), 0,
                 (len + 1) * sizeof(WCHAR));

                if (fileName)
                {
                    SendMessageW(fileNameEdit, WM_GETTEXT, len + 1,
                     (LPARAM)fileName);
                    if (!import_validate_filename(hwnd, data, fileName))
                    {
                        HeapFree(GetProcessHeap(), 0, fileName);
                        SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, 1);
                        ret = 1;
                    }
                    else
                        data->fileName = fileName;
                }
            }
            break;
        }
        }
        break;
    }
    case WM_COMMAND:
        switch (wp)
        {
        case IDC_IMPORT_BROWSE_FILE:
        {
            OPENFILENAMEW ofn;
            WCHAR fileBuf[MAX_PATH];

            data = (struct ImportWizData *)GetWindowLongPtrW(hwnd, DWLP_USER);
            memset(&ofn, 0, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFilter = make_import_file_filter(data->dwFlags);
            ofn.lpstrFile = fileBuf;
            ofn.nMaxFile = sizeof(fileBuf) / sizeof(fileBuf[0]);
            fileBuf[0] = 0;
            if (GetOpenFileNameW(&ofn))
                SendMessageW(GetDlgItem(hwnd, IDC_IMPORT_FILENAME), WM_SETTEXT,
                 0, (LPARAM)ofn.lpstrFile);
            HeapFree(GetProcessHeap(), 0, (LPWSTR)ofn.lpstrFilter);
            break;
        }
        }
        break;
    }
    return ret;
}

static LRESULT CALLBACK import_store_dlg_proc(HWND hwnd, UINT msg, WPARAM wp,
 LPARAM lp)
{
    LRESULT ret = 0;
    struct ImportWizData *data;

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        PROPSHEETPAGEW *page = (PROPSHEETPAGEW *)lp;

        data = (struct ImportWizData *)page->lParam;
        SetWindowLongPtrW(hwnd, DWLP_USER, (LPARAM)data);
        if (!data->hDestCertStore)
        {
            SendMessageW(GetDlgItem(hwnd, IDC_IMPORT_AUTO_STORE), BM_CLICK,
             0, 0);
            EnableWindow(GetDlgItem(hwnd, IDC_IMPORT_STORE), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_IMPORT_BROWSE_STORE), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_IMPORT_SPECIFY_STORE), FALSE);
        }
        else
        {
            WCHAR storeTitle[MAX_STRING_LEN];

            SendMessageW(GetDlgItem(hwnd, IDC_IMPORT_SPECIFY_STORE), BM_CLICK,
             0, 0);
            EnableWindow(GetDlgItem(hwnd, IDC_IMPORT_STORE), TRUE);
            EnableWindow(GetDlgItem(hwnd, IDC_IMPORT_BROWSE_STORE), TRUE);
            EnableWindow(GetDlgItem(hwnd, IDC_IMPORT_SPECIFY_STORE),
             !(data->dwFlags & CRYPTUI_WIZ_IMPORT_NO_CHANGE_DEST_STORE));
            LoadStringW(hInstance, IDS_IMPORT_DEST_DETERMINED,
             storeTitle, sizeof(storeTitle) / sizeof(storeTitle[0]));
            SendMessageW(GetDlgItem(hwnd, IDC_IMPORT_STORE), WM_SETTEXT,
             0, (LPARAM)storeTitle);
        }
        break;
    }
    case WM_NOTIFY:
    {
        NMHDR *hdr = (NMHDR *)lp;

        switch (hdr->code)
        {
        case PSN_SETACTIVE:
            PostMessageW(GetParent(hwnd), PSM_SETWIZBUTTONS, 0,
             PSWIZB_BACK | PSWIZB_NEXT);
            ret = TRUE;
            break;
        case PSN_WIZNEXT:
        {
            data = (struct ImportWizData *)GetWindowLongPtrW(hwnd, DWLP_USER);
            if (IsDlgButtonChecked(hwnd, IDC_IMPORT_SPECIFY_STORE) &&
             !data->hDestCertStore)
            {
                import_warning(data->dwFlags, hwnd, data->pwszWizardTitle,
                 IDS_IMPORT_SELECT_STORE);
                SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, 1);
                ret = 1;
            }
            break;
        }
        }
        break;
    }
    case WM_COMMAND:
        switch (wp)
        {
        case IDC_IMPORT_AUTO_STORE:
            data = (struct ImportWizData *)GetWindowLongPtrW(hwnd, DWLP_USER);
            data->autoDest = TRUE;
            EnableWindow(GetDlgItem(hwnd, IDC_IMPORT_STORE), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_IMPORT_BROWSE_STORE), FALSE);
            break;
        case IDC_IMPORT_SPECIFY_STORE:
            data = (struct ImportWizData *)GetWindowLongPtrW(hwnd, DWLP_USER);
            data->autoDest = FALSE;
            EnableWindow(GetDlgItem(hwnd, IDC_IMPORT_STORE), TRUE);
            EnableWindow(GetDlgItem(hwnd, IDC_IMPORT_BROWSE_STORE), TRUE);
            break;
        case IDC_IMPORT_BROWSE_STORE:
        {
            CRYPTUI_ENUM_SYSTEM_STORE_ARGS enumArgs = {
             CERT_SYSTEM_STORE_CURRENT_USER, NULL };
            CRYPTUI_ENUM_DATA enumData = { 0, NULL, 1, &enumArgs };
            CRYPTUI_SELECTSTORE_INFO_W selectInfo;
            HCERTSTORE store;

            data = (struct ImportWizData *)GetWindowLongPtrW(hwnd, DWLP_USER);
            selectInfo.dwSize = sizeof(selectInfo);
            selectInfo.parent = hwnd;
            selectInfo.dwFlags = CRYPTUI_ENABLE_SHOW_PHYSICAL_STORE;
            selectInfo.pwszTitle = NULL;
            selectInfo.pEnumData = &enumData;
            selectInfo.pfnSelectedStoreCallback = NULL;
            if ((store = CryptUIDlgSelectStoreW(&selectInfo)))
            {
                WCHAR storeTitle[MAX_STRING_LEN];

                LoadStringW(hInstance, IDS_IMPORT_DEST_DETERMINED,
                 storeTitle, sizeof(storeTitle) / sizeof(storeTitle[0]));
                SendMessageW(GetDlgItem(hwnd, IDC_IMPORT_STORE), WM_SETTEXT,
                 0, (LPARAM)storeTitle);
                data->hDestCertStore = store;
                data->freeDest = TRUE;
            }
            break;
        }
        }
        break;
    }
    return ret;
}

static void show_import_details(HWND lv, struct ImportWizData *data)
{
    WCHAR text[MAX_STRING_LEN];
    LVITEMW item;
    int contentID;

    item.mask = LVIF_TEXT;
    item.iItem = SendMessageW(lv, LVM_GETITEMCOUNT, 0, 0);
    item.iSubItem = 0;
    LoadStringW(hInstance, IDS_IMPORT_STORE_SELECTION, text,
     sizeof(text)/ sizeof(text[0]));
    item.pszText = text;
    SendMessageW(lv, LVM_INSERTITEMW, 0, (LPARAM)&item);
    item.iSubItem = 1;
    if (data->autoDest)
        LoadStringW(hInstance, IDS_IMPORT_DEST_AUTOMATIC, text,
         sizeof(text)/ sizeof(text[0]));
    else
        LoadStringW(hInstance, IDS_IMPORT_DEST_DETERMINED, text,
         sizeof(text)/ sizeof(text[0]));
    SendMessageW(lv, LVM_SETITEMTEXTW, item.iItem, (LPARAM)&item);
    item.iItem = SendMessageW(lv, LVM_GETITEMCOUNT, 0, 0);
    item.iSubItem = 0;
    LoadStringW(hInstance, IDS_IMPORT_CONTENT, text,
     sizeof(text)/ sizeof(text[0]));
    SendMessageW(lv, LVM_INSERTITEMW, 0, (LPARAM)&item);
    switch (data->contentType)
    {
    case CERT_QUERY_CONTENT_CERT:
    case CERT_QUERY_CONTENT_SERIALIZED_CERT:
        contentID = IDS_IMPORT_CONTENT_CERT;
        break;
    case CERT_QUERY_CONTENT_CRL:
    case CERT_QUERY_CONTENT_SERIALIZED_CRL:
        contentID = IDS_IMPORT_CONTENT_CRL;
        break;
    case CERT_QUERY_CONTENT_CTL:
    case CERT_QUERY_CONTENT_SERIALIZED_CTL:
        contentID = IDS_IMPORT_CONTENT_CTL;
        break;
    case CERT_QUERY_CONTENT_PKCS7_SIGNED:
        contentID = IDS_IMPORT_CONTENT_CMS;
        break;
    case CERT_QUERY_CONTENT_FLAG_PFX:
        contentID = IDS_IMPORT_CONTENT_PFX;
        break;
    default:
        contentID = IDS_IMPORT_CONTENT_STORE;
        break;
    }
    LoadStringW(hInstance, contentID, text, sizeof(text)/ sizeof(text[0]));
    item.iSubItem = 1;
    SendMessageW(lv, LVM_SETITEMTEXTW, item.iItem, (LPARAM)&item);
    if (data->fileName)
    {
        item.iItem = SendMessageW(lv, LVM_GETITEMCOUNT, 0, 0);
        item.iSubItem = 0;
        LoadStringW(hInstance, IDS_IMPORT_FILE, text,
         sizeof(text)/ sizeof(text[0]));
        SendMessageW(lv, LVM_INSERTITEMW, 0, (LPARAM)&item);
        item.iSubItem = 1;
        item.pszText = data->fileName;
        SendMessageW(lv, LVM_SETITEMTEXTW, item.iItem, (LPARAM)&item);
    }
}

static BOOL do_import(DWORD dwFlags, HWND hwndParent, LPCWSTR pwszWizardTitle,
 PCCRYPTUI_WIZ_IMPORT_SRC_INFO pImportSrc, HCERTSTORE hDestCertStore)
{
    BOOL ret;

    switch (pImportSrc->dwSubjectChoice)
    {
    case CRYPTUI_WIZ_IMPORT_SUBJECT_FILE:
        ret = import_file(dwFlags, hwndParent, pwszWizardTitle,
         pImportSrc->u.pwszFileName, hDestCertStore);
        break;
    case CRYPTUI_WIZ_IMPORT_SUBJECT_CERT_CONTEXT:
        if ((ret = check_context_type(dwFlags, CERT_QUERY_CONTENT_CERT)))
            ret = import_cert(pImportSrc->u.pCertContext, hDestCertStore);
        else
            import_warn_type_mismatch(dwFlags, hwndParent, pwszWizardTitle);
        break;
    case CRYPTUI_WIZ_IMPORT_SUBJECT_CRL_CONTEXT:
        if ((ret = check_context_type(dwFlags, CERT_QUERY_CONTENT_CRL)))
            ret = import_crl(pImportSrc->u.pCRLContext, hDestCertStore);
        else
            import_warn_type_mismatch(dwFlags, hwndParent, pwszWizardTitle);
        break;
    case CRYPTUI_WIZ_IMPORT_SUBJECT_CTL_CONTEXT:
        if ((ret = check_context_type(dwFlags, CERT_QUERY_CONTENT_CTL)))
            ret = import_ctl(pImportSrc->u.pCTLContext, hDestCertStore);
        else
            import_warn_type_mismatch(dwFlags, hwndParent, pwszWizardTitle);
        break;
    case CRYPTUI_WIZ_IMPORT_SUBJECT_CERT_STORE:
        ret = import_store(dwFlags, hwndParent, pwszWizardTitle,
         pImportSrc->u.hCertStore, hDestCertStore);
        break;
    default:
        WARN("unknown source type: %u\n", pImportSrc->dwSubjectChoice);
        SetLastError(E_INVALIDARG);
        ret = FALSE;
    }
    return ret;
}

static LRESULT CALLBACK import_finish_dlg_proc(HWND hwnd, UINT msg, WPARAM wp,
 LPARAM lp)
{
    LRESULT ret = 0;
    struct ImportWizData *data;

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        PROPSHEETPAGEW *page = (PROPSHEETPAGEW *)lp;
        HWND lv = GetDlgItem(hwnd, IDC_IMPORT_SETTINGS);
        RECT rc;
        LVCOLUMNW column;

        data = (struct ImportWizData *)page->lParam;
        SetWindowLongPtrW(hwnd, DWLP_USER, (LPARAM)data);
        SendMessageW(GetDlgItem(hwnd, IDC_IMPORT_TITLE), WM_SETFONT,
         (WPARAM)data->titleFont, TRUE);
        GetWindowRect(lv, &rc);
        column.mask = LVCF_WIDTH;
        column.cx = (rc.right - rc.left) / 2 - 2;
        SendMessageW(lv, LVM_INSERTCOLUMNW, 0, (LPARAM)&column);
        SendMessageW(lv, LVM_INSERTCOLUMNW, 1, (LPARAM)&column);
        show_import_details(lv, data);
        break;
    }
    case WM_NOTIFY:
    {
        NMHDR *hdr = (NMHDR *)lp;

        switch (hdr->code)
        {
        case PSN_SETACTIVE:
        {
            HWND lv = GetDlgItem(hwnd, IDC_IMPORT_SETTINGS);

            data = (struct ImportWizData *)GetWindowLongPtrW(hwnd, DWLP_USER);
            SendMessageW(lv, LVM_DELETEALLITEMS, 0, 0);
            show_import_details(lv, data);
            PostMessageW(GetParent(hwnd), PSM_SETWIZBUTTONS, 0,
             PSWIZB_BACK | PSWIZB_FINISH);
            ret = TRUE;
            break;
        }
        case PSN_WIZFINISH:
        {
            data = (struct ImportWizData *)GetWindowLongPtrW(hwnd, DWLP_USER);
            if ((data->success = do_import(data->dwFlags, hwnd,
             data->pwszWizardTitle, &data->importSrc, data->hDestCertStore)))
            {
                WCHAR title[MAX_STRING_LEN], message[MAX_STRING_LEN];
                LPCWSTR pTitle;

                if (data->pwszWizardTitle)
                    pTitle = data->pwszWizardTitle;
                else
                {
                    LoadStringW(hInstance, IDS_IMPORT_WIZARD, title,
                     sizeof(title) / sizeof(title[0]));
                    pTitle = title;
                }
                LoadStringW(hInstance, IDS_IMPORT_SUCCEEDED, message,
                 sizeof(message) / sizeof(message[0]));
                MessageBoxW(hwnd, message, pTitle, MB_OK);
            }
            else
                import_warning(data->dwFlags, hwnd, data->pwszWizardTitle,
                 IDS_IMPORT_FAILED);
            break;
        }
        }
        break;
    }
    }
    return ret;
}

static BOOL show_import_ui(DWORD dwFlags, HWND hwndParent,
 LPCWSTR pwszWizardTitle, PCCRYPTUI_WIZ_IMPORT_SRC_INFO pImportSrc,
 HCERTSTORE hDestCertStore)
{
    PROPSHEETHEADERW hdr;
    PROPSHEETPAGEW pages[4];
    struct ImportWizData data;
    int nPages = 0;

    data.dwFlags = dwFlags;
    data.pwszWizardTitle = pwszWizardTitle;
    if (pImportSrc)
    {
        memcpy(&data.importSrc, pImportSrc, sizeof(data.importSrc));
        data.fileName = (LPWSTR)pImportSrc->u.pwszFileName;
    }
    else
    {
        memset(&data.importSrc, 0, sizeof(data.importSrc));
        data.fileName = NULL;
    }
    data.freeSource = FALSE;
    data.hDestCertStore = hDestCertStore;
    data.freeDest = FALSE;
    data.autoDest = TRUE;
    data.success = TRUE;

    memset(&pages, 0, sizeof(pages));

    pages[nPages].dwSize = sizeof(pages[0]);
    pages[nPages].hInstance = hInstance;
    pages[nPages].u.pszTemplate = MAKEINTRESOURCEW(IDD_IMPORT_WELCOME);
    pages[nPages].pfnDlgProc = import_welcome_dlg_proc;
    pages[nPages].dwFlags = PSP_HIDEHEADER;
    pages[nPages].lParam = (LPARAM)&data;
    nPages++;

    if (!pImportSrc ||
     pImportSrc->dwSubjectChoice == CRYPTUI_WIZ_IMPORT_SUBJECT_FILE)
    {
        pages[nPages].dwSize = sizeof(pages[0]);
        pages[nPages].hInstance = hInstance;
        pages[nPages].u.pszTemplate = MAKEINTRESOURCEW(IDD_IMPORT_FILE);
        pages[nPages].pfnDlgProc = import_file_dlg_proc;
        pages[nPages].dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        pages[nPages].pszHeaderTitle = MAKEINTRESOURCEW(IDS_IMPORT_FILE_TITLE);
        pages[nPages].pszHeaderSubTitle =
         MAKEINTRESOURCEW(IDS_IMPORT_FILE_SUBTITLE);
        pages[nPages].lParam = (LPARAM)&data;
        nPages++;
    }
    else
    {
        switch (pImportSrc->dwSubjectChoice)
        {
        case CRYPTUI_WIZ_IMPORT_SUBJECT_CERT_CONTEXT:
            data.contentType = CERT_QUERY_CONTENT_CERT;
            break;
        case CRYPTUI_WIZ_IMPORT_SUBJECT_CRL_CONTEXT:
            data.contentType = CERT_QUERY_CONTENT_CRL;
            break;
        case CRYPTUI_WIZ_IMPORT_SUBJECT_CTL_CONTEXT:
            data.contentType = CERT_QUERY_CONTENT_CTL;
            break;
        case CRYPTUI_WIZ_IMPORT_SUBJECT_CERT_STORE:
            data.contentType = CERT_QUERY_CONTENT_SERIALIZED_STORE;
            break;
        }
    }

    pages[nPages].dwSize = sizeof(pages[0]);
    pages[nPages].hInstance = hInstance;
    pages[nPages].u.pszTemplate = MAKEINTRESOURCEW(IDD_IMPORT_STORE);
    pages[nPages].pfnDlgProc = import_store_dlg_proc;
    pages[nPages].dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    pages[nPages].pszHeaderTitle = MAKEINTRESOURCEW(IDS_IMPORT_STORE_TITLE);
    pages[nPages].pszHeaderSubTitle =
     MAKEINTRESOURCEW(IDS_IMPORT_STORE_SUBTITLE);
    pages[nPages].lParam = (LPARAM)&data;
    nPages++;

    pages[nPages].dwSize = sizeof(pages[0]);
    pages[nPages].hInstance = hInstance;
    pages[nPages].u.pszTemplate = MAKEINTRESOURCEW(IDD_IMPORT_FINISH);
    pages[nPages].pfnDlgProc = import_finish_dlg_proc;
    pages[nPages].dwFlags = PSP_HIDEHEADER;
    pages[nPages].lParam = (LPARAM)&data;
    nPages++;

    memset(&hdr, 0, sizeof(hdr));
    hdr.dwSize = sizeof(hdr);
    hdr.hwndParent = hwndParent;
    hdr.dwFlags = PSH_PROPSHEETPAGE | PSH_WIZARD97_NEW | PSH_HEADER |
     PSH_WATERMARK;
    hdr.hInstance = hInstance;
    if (pwszWizardTitle)
        hdr.pszCaption = pwszWizardTitle;
    else
        hdr.pszCaption = MAKEINTRESOURCEW(IDS_IMPORT_WIZARD);
    hdr.u3.ppsp = pages;
    hdr.nPages = nPages;
    hdr.u4.pszbmWatermark = MAKEINTRESOURCEW(IDB_CERT_WATERMARK);
    hdr.u5.pszbmHeader = MAKEINTRESOURCEW(IDB_CERT_HEADER);
    PropertySheetW(&hdr);
    if (data.fileName != data.importSrc.u.pwszFileName)
        HeapFree(GetProcessHeap(), 0, data.fileName);
    if (data.freeSource &&
     data.importSrc.dwSubjectChoice == CRYPTUI_WIZ_IMPORT_SUBJECT_CERT_STORE)
        CertCloseStore(data.importSrc.u.hCertStore, 0);
    DeleteObject(data.titleFont);
    return data.success;
}

BOOL WINAPI CryptUIWizImport(DWORD dwFlags, HWND hwndParent, LPCWSTR pwszWizardTitle,
                             PCCRYPTUI_WIZ_IMPORT_SRC_INFO pImportSrc, HCERTSTORE hDestCertStore)
{
    BOOL ret;

    TRACE("(0x%08x, %p, %s, %p, %p)\n", dwFlags, hwndParent, debugstr_w(pwszWizardTitle),
          pImportSrc, hDestCertStore);

    if (pImportSrc &&
     pImportSrc->dwSize != sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO))
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }

    if (!(dwFlags & CRYPTUI_WIZ_NO_UI))
        ret = show_import_ui(dwFlags, hwndParent, pwszWizardTitle, pImportSrc,
         hDestCertStore);
    else if (pImportSrc)
        ret = do_import(dwFlags, hwndParent, pwszWizardTitle, pImportSrc,
         hDestCertStore);
    else
    {
        /* Can't have no UI without specifying source */
        SetLastError(E_INVALIDARG);
        ret = FALSE;
    }

    return ret;
}

struct ExportWizData
{
    HFONT titleFont;
    DWORD dwFlags;
    LPCWSTR pwszWizardTitle;
    CRYPTUI_WIZ_EXPORT_INFO exportInfo;
    CRYPTUI_WIZ_EXPORT_CERTCONTEXT_INFO contextInfo;
    BOOL freePassword;
    PCRYPT_KEY_PROV_INFO keyProvInfo;
    BOOL deleteKeys;
    LPWSTR fileName;
    HANDLE file;
    BOOL success;
};

static LRESULT CALLBACK export_welcome_dlg_proc(HWND hwnd, UINT msg, WPARAM wp,
 LPARAM lp)
{
    LRESULT ret = 0;

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        struct ExportWizData *data;
        PROPSHEETPAGEW *page = (PROPSHEETPAGEW *)lp;
        WCHAR fontFace[MAX_STRING_LEN];
        HDC hDC = GetDC(hwnd);
        int height;

        data = (struct ExportWizData *)page->lParam;
        LoadStringW(hInstance, IDS_WIZARD_TITLE_FONT, fontFace,
         sizeof(fontFace) / sizeof(fontFace[0]));
        height = -MulDiv(12, GetDeviceCaps(hDC, LOGPIXELSY), 72);
        data->titleFont = CreateFontW(height, 0, 0, 0, FW_BOLD, 0, 0, 0,
         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
         DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fontFace);
        SendMessageW(GetDlgItem(hwnd, IDC_EXPORT_TITLE), WM_SETFONT,
         (WPARAM)data->titleFont, TRUE);
        ReleaseDC(hwnd, hDC);
        break;
    }
    case WM_NOTIFY:
    {
        NMHDR *hdr = (NMHDR *)lp;

        switch (hdr->code)
        {
        case PSN_SETACTIVE:
            PostMessageW(GetParent(hwnd), PSM_SETWIZBUTTONS, 0, PSWIZB_NEXT);
            ret = TRUE;
            break;
        }
        break;
    }
    }
    return ret;
}

static PCRYPT_KEY_PROV_INFO export_get_private_key_info(PCCERT_CONTEXT cert)
{
    PCRYPT_KEY_PROV_INFO info = NULL;
    DWORD size;

    if (CertGetCertificateContextProperty(cert, CERT_KEY_PROV_INFO_PROP_ID,
     NULL, &size))
    {
        info = HeapAlloc(GetProcessHeap(), 0, size);
        if (info)
        {
            if (!CertGetCertificateContextProperty(cert,
             CERT_KEY_PROV_INFO_PROP_ID, info, &size))
            {
                HeapFree(GetProcessHeap(), 0, info);
                info = NULL;
            }
        }
    }
    return info;
}

static BOOL export_acquire_private_key(const CRYPT_KEY_PROV_INFO *info,
 HCRYPTPROV *phProv)
{
    BOOL ret;

    ret = CryptAcquireContextW(phProv, info->pwszContainerName,
     info->pwszProvName, info->dwProvType, 0);
    if (ret)
    {
        DWORD i;

        for (i = 0; i < info->cProvParam; i++)
            CryptSetProvParam(*phProv, info->rgProvParam[i].dwParam,
             info->rgProvParam[i].pbData, info->rgProvParam[i].dwFlags);
    }
    return ret;
}

static BOOL export_is_key_exportable(HCRYPTPROV hProv, DWORD keySpec)
{
    BOOL ret;
    HCRYPTKEY key;

    if ((ret = CryptGetUserKey(hProv, keySpec, &key)))
    {
        DWORD permissions, size = sizeof(permissions);

        if ((ret = CryptGetKeyParam(key, KP_PERMISSIONS, (BYTE *)&permissions,
         &size, 0)) && !(permissions & CRYPT_EXPORT))
            ret = FALSE;
        CryptDestroyKey(key);
    }
    return ret;
}

static LRESULT CALLBACK export_private_key_dlg_proc(HWND hwnd, UINT msg,
 WPARAM wp, LPARAM lp)
{
    LRESULT ret = 0;
    struct ExportWizData *data;

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        PROPSHEETPAGEW *page = (PROPSHEETPAGEW *)lp;
        PCRYPT_KEY_PROV_INFO info;
        HCRYPTPROV hProv = 0;
        int errorID = 0;

        data = (struct ExportWizData *)page->lParam;
        SetWindowLongPtrW(hwnd, DWLP_USER, (LPARAM)data);
        /* Get enough information about a key to see whether it's exportable.
         */
        if (!(info = export_get_private_key_info(
         data->exportInfo.u.pCertContext)))
            errorID = IDS_EXPORT_PRIVATE_KEY_UNAVAILABLE;
        else if (!export_acquire_private_key(info, &hProv))
            errorID = IDS_EXPORT_PRIVATE_KEY_UNAVAILABLE;
        else if (!export_is_key_exportable(hProv, info->dwKeySpec))
            errorID = IDS_EXPORT_PRIVATE_KEY_NON_EXPORTABLE;

        if (errorID)
        {
            WCHAR error[MAX_STRING_LEN];

            LoadStringW(hInstance, errorID, error,
             sizeof(error) / sizeof(error[0]));
            SendMessageW(GetDlgItem(hwnd, IDC_EXPORT_PRIVATE_KEY_UNAVAILABLE),
             WM_SETTEXT, 0, (LPARAM)error);
            EnableWindow(GetDlgItem(hwnd, IDC_EXPORT_PRIVATE_KEY_YES), FALSE);
        }
        else
            data->keyProvInfo = info;
        if (hProv)
            CryptReleaseContext(hProv, 0);
        SendMessageW(GetDlgItem(hwnd, IDC_EXPORT_PRIVATE_KEY_NO), BM_CLICK,
         0, 0);
        break;
    }
    case WM_NOTIFY:
    {
        NMHDR *hdr = (NMHDR *)lp;

        switch (hdr->code)
        {
        case PSN_SETACTIVE:
            PostMessageW(GetParent(hwnd), PSM_SETWIZBUTTONS, 0,
             PSWIZB_BACK | PSWIZB_NEXT);
            ret = TRUE;
            break;
        case PSN_WIZNEXT:
            data = (struct ExportWizData *)GetWindowLongPtrW(hwnd, DWLP_USER);
            if (IsDlgButtonChecked(hwnd, IDC_EXPORT_PRIVATE_KEY_NO))
            {
                data->contextInfo.dwExportFormat =
                 CRYPTUI_WIZ_EXPORT_FORMAT_DER;
                data->contextInfo.fExportPrivateKeys = FALSE;
            }
            else
            {
                data->contextInfo.dwExportFormat =
                 CRYPTUI_WIZ_EXPORT_FORMAT_PFX;
                data->contextInfo.fExportPrivateKeys = TRUE;
            }
            break;
        }
        break;
    }
    }
    return ret;
}

static BOOL export_info_has_private_key(PCCRYPTUI_WIZ_EXPORT_INFO pExportInfo)
{
    BOOL ret = FALSE;

    if (pExportInfo->dwSubjectChoice == CRYPTUI_WIZ_EXPORT_CERT_CONTEXT)
    {
        DWORD size;

        /* If there's a CRYPT_KEY_PROV_INFO set for this cert, assume the
         * cert has a private key.
         */
        if (CertGetCertificateContextProperty(pExportInfo->u.pCertContext,
         CERT_KEY_PROV_INFO_PROP_ID, NULL, &size))
            ret = TRUE;
    }
    return ret;
}

static void export_format_enable_controls(HWND hwnd, const struct ExportWizData *data)
{
    int defaultFormatID;

    switch (data->contextInfo.dwExportFormat)
    {
    case CRYPTUI_WIZ_EXPORT_FORMAT_BASE64:
        defaultFormatID = IDC_EXPORT_FORMAT_BASE64;
        break;
    case CRYPTUI_WIZ_EXPORT_FORMAT_PKCS7:
        defaultFormatID = IDC_EXPORT_FORMAT_CMS;
        break;
    case CRYPTUI_WIZ_EXPORT_FORMAT_PFX:
        defaultFormatID = IDC_EXPORT_FORMAT_PFX;
        break;
    default:
        defaultFormatID = IDC_EXPORT_FORMAT_DER;
    }
    SendMessageW(GetDlgItem(hwnd, defaultFormatID), BM_CLICK, 0, 0);
    if (defaultFormatID == IDC_EXPORT_FORMAT_PFX)
    {
        EnableWindow(GetDlgItem(hwnd, IDC_EXPORT_FORMAT_DER), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_EXPORT_FORMAT_BASE64), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_EXPORT_FORMAT_CMS), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_EXPORT_FORMAT_PFX), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hwnd, IDC_EXPORT_FORMAT_DER), TRUE);
        EnableWindow(GetDlgItem(hwnd, IDC_EXPORT_FORMAT_BASE64), TRUE);
        EnableWindow(GetDlgItem(hwnd, IDC_EXPORT_FORMAT_CMS), TRUE);
        EnableWindow(GetDlgItem(hwnd, IDC_EXPORT_FORMAT_PFX), FALSE);
    }
}

static LRESULT CALLBACK export_format_dlg_proc(HWND hwnd, UINT msg, WPARAM wp,
 LPARAM lp)
{
    LRESULT ret = 0;
    struct ExportWizData *data;

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        PROPSHEETPAGEW *page = (PROPSHEETPAGEW *)lp;

        data = (struct ExportWizData *)page->lParam;
        SetWindowLongPtrW(hwnd, DWLP_USER, (LPARAM)data);
        export_format_enable_controls(hwnd, data);
        break;
    }
    case WM_NOTIFY:
    {
        NMHDR *hdr = (NMHDR *)lp;

        switch (hdr->code)
        {
        case PSN_SETACTIVE:
            PostMessageW(GetParent(hwnd), PSM_SETWIZBUTTONS, 0,
             PSWIZB_BACK | PSWIZB_NEXT);
            data = (struct ExportWizData *)GetWindowLongPtrW(hwnd, DWLP_USER);
            export_format_enable_controls(hwnd, data);
            ret = TRUE;
            break;
        case PSN_WIZNEXT:
        {
            BOOL skipPasswordPage = TRUE;

            data = (struct ExportWizData *)GetWindowLongPtrW(hwnd, DWLP_USER);
            if (IsDlgButtonChecked(hwnd, IDC_EXPORT_FORMAT_DER))
                data->contextInfo.dwExportFormat =
                 CRYPTUI_WIZ_EXPORT_FORMAT_DER;
            else if (IsDlgButtonChecked(hwnd, IDC_EXPORT_FORMAT_BASE64))
                data->contextInfo.dwExportFormat =
                 CRYPTUI_WIZ_EXPORT_FORMAT_BASE64;
            else if (IsDlgButtonChecked(hwnd, IDC_EXPORT_FORMAT_CMS))
            {
                data->contextInfo.dwExportFormat =
                 CRYPTUI_WIZ_EXPORT_FORMAT_PKCS7;
                if (IsDlgButtonChecked(hwnd, IDC_EXPORT_CMS_INCLUDE_CHAIN))
                    data->contextInfo.fExportChain =
                     CRYPTUI_WIZ_EXPORT_FORMAT_PKCS7;
            }
            else
            {
                data->contextInfo.dwExportFormat =
                 CRYPTUI_WIZ_EXPORT_FORMAT_PFX;
                if (IsDlgButtonChecked(hwnd, IDC_EXPORT_PFX_INCLUDE_CHAIN))
                    data->contextInfo.fExportChain = TRUE;
                if (IsDlgButtonChecked(hwnd, IDC_EXPORT_PFX_STRONG_ENCRYPTION))
                    data->contextInfo.fStrongEncryption = TRUE;
                if (IsDlgButtonChecked(hwnd, IDC_EXPORT_PFX_DELETE_PRIVATE_KEY))
                    data->deleteKeys = TRUE;
                skipPasswordPage = FALSE;
            }
            SetWindowLongPtrW(hwnd, DWLP_MSGRESULT,
             skipPasswordPage ? IDD_EXPORT_FILE : 0);
            ret = 1;
            break;
        }
        }
        break;
    }
    case WM_COMMAND:
        switch (HIWORD(wp))
        {
        case BN_CLICKED:
            switch (LOWORD(wp))
            {
            case IDC_EXPORT_FORMAT_DER:
            case IDC_EXPORT_FORMAT_BASE64:
                EnableWindow(GetDlgItem(hwnd, IDC_EXPORT_CMS_INCLUDE_CHAIN),
                 FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_EXPORT_PFX_INCLUDE_CHAIN),
                 FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_EXPORT_PFX_STRONG_ENCRYPTION),
                 FALSE);
                EnableWindow(GetDlgItem(hwnd,
                 IDC_EXPORT_PFX_DELETE_PRIVATE_KEY), FALSE);
                break;
            case IDC_EXPORT_FORMAT_CMS:
                EnableWindow(GetDlgItem(hwnd, IDC_EXPORT_CMS_INCLUDE_CHAIN),
                 TRUE);
                break;
            case IDC_EXPORT_FORMAT_PFX:
                EnableWindow(GetDlgItem(hwnd, IDC_EXPORT_PFX_INCLUDE_CHAIN),
                 TRUE);
                EnableWindow(GetDlgItem(hwnd, IDC_EXPORT_PFX_STRONG_ENCRYPTION),
                 TRUE);
                EnableWindow(GetDlgItem(hwnd,
                 IDC_EXPORT_PFX_DELETE_PRIVATE_KEY), TRUE);
                break;
            }
            break;
        }
        break;
    }
    return ret;
}

static void export_password_mismatch(HWND hwnd, const struct ExportWizData *data)
{
    WCHAR title[MAX_STRING_LEN], error[MAX_STRING_LEN];
    LPCWSTR pTitle;

    if (data->pwszWizardTitle)
        pTitle = data->pwszWizardTitle;
    else
    {
        LoadStringW(hInstance, IDS_EXPORT_WIZARD, title,
         sizeof(title) / sizeof(title[0]));
        pTitle = title;
    }
    LoadStringW(hInstance, IDS_EXPORT_PASSWORD_MISMATCH, error,
     sizeof(error) / sizeof(error[0]));
    MessageBoxW(hwnd, error, pTitle, MB_ICONERROR | MB_OK);
    SetFocus(GetDlgItem(hwnd, IDC_EXPORT_PASSWORD));
}

static LRESULT CALLBACK export_password_dlg_proc(HWND hwnd, UINT msg,
 WPARAM wp, LPARAM lp)
{
    LRESULT ret = 0;
    struct ExportWizData *data;

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        PROPSHEETPAGEW *page = (PROPSHEETPAGEW *)lp;

        data = (struct ExportWizData *)page->lParam;
        SetWindowLongPtrW(hwnd, DWLP_USER, (LPARAM)data);
        break;
    }
    case WM_NOTIFY:
    {
        NMHDR *hdr = (NMHDR *)lp;

        switch (hdr->code)
        {
        case PSN_SETACTIVE:
            PostMessageW(GetParent(hwnd), PSM_SETWIZBUTTONS, 0,
             PSWIZB_BACK | PSWIZB_NEXT);
            ret = TRUE;
            break;
        case PSN_WIZNEXT:
        {
            HWND passwordEdit = GetDlgItem(hwnd, IDC_EXPORT_PASSWORD);
            HWND passwordConfirmEdit = GetDlgItem(hwnd,
             IDC_EXPORT_PASSWORD_CONFIRM);
            DWORD passwordLen = SendMessageW(passwordEdit, WM_GETTEXTLENGTH,
             0, 0);
            DWORD passwordConfirmLen = SendMessageW(passwordConfirmEdit,
             WM_GETTEXTLENGTH, 0, 0);

            data = (struct ExportWizData *)GetWindowLongPtrW(hwnd, DWLP_USER);
            if (!passwordLen && !passwordConfirmLen)
                data->contextInfo.pwszPassword = NULL;
            else if (passwordLen != passwordConfirmLen)
            {
                export_password_mismatch(hwnd, data);
                SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, 1);
                ret = 1;
            }
            else
            {
                LPWSTR password = HeapAlloc(GetProcessHeap(), 0,
                 (passwordLen + 1) * sizeof(WCHAR));
                LPWSTR passwordConfirm = HeapAlloc(GetProcessHeap(), 0,
                 (passwordConfirmLen + 1) * sizeof(WCHAR));
                BOOL freePassword = TRUE;

                if (password && passwordConfirm)
                {
                    SendMessageW(passwordEdit, WM_GETTEXT, passwordLen + 1,
                     (LPARAM)password);
                    SendMessageW(passwordConfirmEdit, WM_GETTEXT,
                     passwordConfirmLen + 1, (LPARAM)passwordConfirm);
                    if (strcmpW(password, passwordConfirm))
                    {
                        export_password_mismatch(hwnd, data);
                        SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, 1);
                        ret = 1;
                    }
                    else
                    {
                        data->contextInfo.pwszPassword = password;
                        freePassword = FALSE;
                        data->freePassword = TRUE;
                    }
                }
                if (freePassword)
                    HeapFree(GetProcessHeap(), 0, password);
                HeapFree(GetProcessHeap(), 0, passwordConfirm);
            }
            break;
        }
        }
        break;
    }
    }
    return ret;
}

static LPWSTR export_append_extension(const struct ExportWizData *data,
 LPWSTR fileName)
{
    static const WCHAR cer[] = { '.','c','e','r',0 };
    static const WCHAR crl[] = { '.','c','r','l',0 };
    static const WCHAR ctl[] = { '.','c','t','l',0 };
    static const WCHAR p7b[] = { '.','p','7','b',0 };
    static const WCHAR pfx[] = { '.','p','f','x',0 };
    static const WCHAR sst[] = { '.','s','s','t',0 };
    LPCWSTR extension;
    LPWSTR dot;
    BOOL appendExtension;

    switch (data->contextInfo.dwExportFormat)
    {
    case CRYPTUI_WIZ_EXPORT_FORMAT_PKCS7:
        extension = p7b;
        break;
    case CRYPTUI_WIZ_EXPORT_FORMAT_PFX:
        extension = pfx;
        break;
    default:
        switch (data->exportInfo.dwSubjectChoice)
        {
        case CRYPTUI_WIZ_EXPORT_CRL_CONTEXT:
            extension = crl;
            break;
        case CRYPTUI_WIZ_EXPORT_CTL_CONTEXT:
            extension = ctl;
            break;
        case CRYPTUI_WIZ_EXPORT_CERT_STORE:
            extension = sst;
            break;
        default:
            extension = cer;
        }
    }
    dot = strrchrW(fileName, '.');
    if (dot)
        appendExtension = strcmpiW(dot, extension) != 0;
    else
        appendExtension = TRUE;
    if (appendExtension)
    {
        fileName = HeapReAlloc(GetProcessHeap(), 0, fileName,
         (strlenW(fileName) + strlenW(extension) + 1) * sizeof(WCHAR));
        if (fileName)
            strcatW(fileName, extension);
    }
    return fileName;
}

static BOOL export_validate_filename(HWND hwnd, struct ExportWizData *data,
 LPCWSTR fileName)
{
    HANDLE file;
    BOOL tryCreate = TRUE, forceCreate = FALSE, ret = FALSE;

    file = CreateFileW(fileName, GENERIC_WRITE,
     FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (file != INVALID_HANDLE_VALUE)
    {
        WCHAR warning[MAX_STRING_LEN], title[MAX_STRING_LEN];
        LPCWSTR pTitle;

        if (data->pwszWizardTitle)
            pTitle = data->pwszWizardTitle;
        else
        {
            LoadStringW(hInstance, IDS_EXPORT_WIZARD, title,
             sizeof(title) / sizeof(title[0]));
            pTitle = title;
        }
        LoadStringW(hInstance, IDS_EXPORT_FILE_EXISTS, warning,
         sizeof(warning) / sizeof(warning[0]));
        if (MessageBoxW(hwnd, warning, pTitle, MB_YESNO) == IDYES)
            forceCreate = TRUE;
        else
            tryCreate = FALSE;
        CloseHandle(file);
    }
    if (tryCreate)
    {
        file = CreateFileW(fileName, GENERIC_WRITE,
         FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
         forceCreate ? CREATE_ALWAYS : CREATE_NEW,
         0, NULL);
        if (file != INVALID_HANDLE_VALUE)
        {
            data->file = file;
            ret = TRUE;
        }
        else
        {
            WCHAR title[MAX_STRING_LEN], error[MAX_STRING_LEN];
            LPCWSTR pTitle;
            LPWSTR msgBuf, fullError;

            if (data->pwszWizardTitle)
                pTitle = data->pwszWizardTitle;
            else
            {
                LoadStringW(hInstance, IDS_EXPORT_WIZARD, title,
                 sizeof(title) / sizeof(title[0]));
                pTitle = title;
            }
            LoadStringW(hInstance, IDS_IMPORT_OPEN_FAILED, error,
             sizeof(error) / sizeof(error[0]));
            FormatMessageW(
             FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
             GetLastError(), 0, (LPWSTR) &msgBuf, 0, NULL);
            fullError = HeapAlloc(GetProcessHeap(), 0,
             (strlenW(error) + strlenW(fileName) + strlenW(msgBuf) + 3)
             * sizeof(WCHAR));
            if (fullError)
            {
                LPWSTR ptr = fullError;

                strcpyW(ptr, error);
                ptr += strlenW(error);
                strcpyW(ptr, fileName);
                ptr += strlenW(fileName);
                *ptr++ = ':';
                *ptr++ = '\n';
                strcpyW(ptr, msgBuf);
                MessageBoxW(hwnd, fullError, pTitle, MB_ICONERROR | MB_OK);
                HeapFree(GetProcessHeap(), 0, fullError);
            }
            LocalFree(msgBuf);
        }
    }
    return ret;
}

static const WCHAR export_filter_cert[] = { '*','.','c','e','r',0 };
static const WCHAR export_filter_crl[] = { '*','.','c','r','l',0 };
static const WCHAR export_filter_ctl[] = { '*','.','s','t','l',0 };
static const WCHAR export_filter_cms[] = { '*','.','p','7','b',0 };
static const WCHAR export_filter_pfx[] = { '*','.','p','f','x',0 };
static const WCHAR export_filter_sst[] = { '*','.','s','s','t',0 };

static WCHAR *make_export_file_filter(DWORD exportFormat, DWORD subjectChoice)
{
    int baseLen, allLen, totalLen = 2, baseID;
    LPWSTR filter = NULL, baseFilter, all;
    LPCWSTR filterStr;

    switch (exportFormat)
    {
    case CRYPTUI_WIZ_EXPORT_FORMAT_BASE64:
        baseID = IDS_EXPORT_FILTER_BASE64_CERT;
        filterStr = export_filter_cert;
        break;
    case CRYPTUI_WIZ_EXPORT_FORMAT_PFX:
        baseID = IDS_EXPORT_FILTER_PFX;
        filterStr = export_filter_pfx;
        break;
    case CRYPTUI_WIZ_EXPORT_FORMAT_PKCS7:
        baseID = IDS_EXPORT_FILTER_CMS;
        filterStr = export_filter_cms;
        break;
    default:
        switch (subjectChoice)
        {
        case CRYPTUI_WIZ_EXPORT_CRL_CONTEXT:
            baseID = IDS_EXPORT_FILTER_CRL;
            filterStr = export_filter_crl;
            break;
        case CRYPTUI_WIZ_EXPORT_CTL_CONTEXT:
            baseID = IDS_EXPORT_FILTER_CTL;
            filterStr = export_filter_ctl;
            break;
        case CRYPTUI_WIZ_EXPORT_CERT_STORE:
            baseID = IDS_EXPORT_FILTER_SERIALIZED_CERT_STORE;
            filterStr = export_filter_sst;
            break;
        default:
            baseID = IDS_EXPORT_FILTER_CERT;
            filterStr = export_filter_cert;
            break;
        }
    }
    baseLen = LoadStringW(hInstance, baseID, (LPWSTR)&baseFilter, 0);
    totalLen += baseLen + strlenW(filterStr) + 2;
    allLen = LoadStringW(hInstance, IDS_IMPORT_FILTER_ALL, (LPWSTR)&all, 0);
    totalLen += allLen + strlenW(filter_all) + 2;
    filter = HeapAlloc(GetProcessHeap(), 0, totalLen * sizeof(WCHAR));
    if (filter)
    {
        LPWSTR ptr;

        ptr = filter;
        memcpy(ptr, baseFilter, baseLen * sizeof(WCHAR));
        ptr += baseLen;
        *ptr++ = 0;
        strcpyW(ptr, filterStr);
        ptr += strlenW(filterStr) + 1;
        memcpy(ptr, all, allLen * sizeof(WCHAR));
        ptr += allLen;
        *ptr++ = 0;
        strcpyW(ptr, filter_all);
        ptr += strlenW(filter_all) + 1;
        *ptr++ = 0;
    }
    return filter;
}

static LRESULT CALLBACK export_file_dlg_proc(HWND hwnd, UINT msg, WPARAM wp,
 LPARAM lp)
{
    LRESULT ret = 0;
    struct ExportWizData *data;

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        PROPSHEETPAGEW *page = (PROPSHEETPAGEW *)lp;

        data = (struct ExportWizData *)page->lParam;
        SetWindowLongPtrW(hwnd, DWLP_USER, (LPARAM)data);
        if (data->exportInfo.pwszExportFileName)
            SendMessageW(GetDlgItem(hwnd, IDC_EXPORT_FILENAME), WM_SETTEXT, 0,
             (LPARAM)data->exportInfo.pwszExportFileName);
        break;
    }
    case WM_NOTIFY:
    {
        NMHDR *hdr = (NMHDR *)lp;

        switch (hdr->code)
        {
        case PSN_WIZBACK:
            data = (struct ExportWizData *)GetWindowLongPtrW(hwnd, DWLP_USER);
            if (data->contextInfo.dwExportFormat !=
             CRYPTUI_WIZ_EXPORT_FORMAT_PFX)
            {
                SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, IDD_EXPORT_FORMAT);
                ret = 1;
            }
            break;
        case PSN_WIZNEXT:
        {
            HWND fileNameEdit = GetDlgItem(hwnd, IDC_EXPORT_FILENAME);
            DWORD len = SendMessageW(fileNameEdit, WM_GETTEXTLENGTH, 0, 0);

            data = (struct ExportWizData *)GetWindowLongPtrW(hwnd, DWLP_USER);
            if (!len)
            {
                WCHAR title[MAX_STRING_LEN], error[MAX_STRING_LEN];
                LPCWSTR pTitle;

                if (data->pwszWizardTitle)
                    pTitle = data->pwszWizardTitle;
                else
                {
                    LoadStringW(hInstance, IDS_EXPORT_WIZARD, title,
                     sizeof(title) / sizeof(title[0]));
                    pTitle = title;
                }
                LoadStringW(hInstance, IDS_IMPORT_EMPTY_FILE, error,
                 sizeof(error) / sizeof(error[0]));
                MessageBoxW(hwnd, error, pTitle, MB_ICONERROR | MB_OK);
                SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, 1);
                ret = 1;
            }
            else
            {
                LPWSTR fileName = HeapAlloc(GetProcessHeap(), 0,
                 (len + 1) * sizeof(WCHAR));

                if (fileName)
                {
                    SendMessageW(fileNameEdit, WM_GETTEXT, len + 1,
                     (LPARAM)fileName);
                    fileName = export_append_extension(data, fileName);
                    if (!export_validate_filename(hwnd, data, fileName))
                    {
                        HeapFree(GetProcessHeap(), 0, fileName);
                        SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, 1);
                        ret = 1;
                    }
                    else
                        data->fileName = fileName;
                }
            }
            break;
        }
        case PSN_SETACTIVE:
            PostMessageW(GetParent(hwnd), PSM_SETWIZBUTTONS, 0,
             PSWIZB_BACK | PSWIZB_NEXT);
            ret = TRUE;
            break;
        }
        break;
    }
    case WM_COMMAND:
        switch (wp)
        {
        case IDC_EXPORT_BROWSE_FILE:
        {
            OPENFILENAMEW ofn;
            WCHAR fileBuf[MAX_PATH];

            data = (struct ExportWizData *)GetWindowLongPtrW(hwnd, DWLP_USER);
            memset(&ofn, 0, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFilter = make_export_file_filter(
             data->contextInfo.dwExportFormat,
             data->exportInfo.dwSubjectChoice);
            ofn.lpstrFile = fileBuf;
            ofn.nMaxFile = sizeof(fileBuf) / sizeof(fileBuf[0]);
            fileBuf[0] = 0;
            if (GetSaveFileNameW(&ofn))
                SendMessageW(GetDlgItem(hwnd, IDC_EXPORT_FILENAME), WM_SETTEXT,
                 0, (LPARAM)ofn.lpstrFile);
            HeapFree(GetProcessHeap(), 0, (LPWSTR)ofn.lpstrFilter);
            break;
        }
        }
        break;
    }
    return ret;
}

static void show_export_details(HWND lv, const struct ExportWizData *data)
{
    WCHAR text[MAX_STRING_LEN];
    LVITEMW item;
    int contentID;

    item.mask = LVIF_TEXT;
    if (data->fileName)
    {
        item.iItem = SendMessageW(lv, LVM_GETITEMCOUNT, 0, 0);
        item.iSubItem = 0;
        LoadStringW(hInstance, IDS_IMPORT_FILE, text,
         sizeof(text)/ sizeof(text[0]));
        item.pszText = text;
        SendMessageW(lv, LVM_INSERTITEMW, 0, (LPARAM)&item);
        item.iSubItem = 1;
        item.pszText = data->fileName;
        SendMessageW(lv, LVM_SETITEMTEXTW, item.iItem, (LPARAM)&item);
    }

    item.pszText = text;
    switch (data->exportInfo.dwSubjectChoice)
    {
    case CRYPTUI_WIZ_EXPORT_CRL_CONTEXT:
    case CRYPTUI_WIZ_EXPORT_CTL_CONTEXT:
    case CRYPTUI_WIZ_EXPORT_CERT_STORE:
    case CRYPTUI_WIZ_EXPORT_CERT_STORE_CERTIFICATES_ONLY:
        /* do nothing */
        break;
    default:
    {
        item.iItem = SendMessageW(lv, LVM_GETITEMCOUNT, 0, 0);
        item.iSubItem = 0;
        LoadStringW(hInstance, IDS_EXPORT_INCLUDE_CHAIN, text,
         sizeof(text) / sizeof(text[0]));
        SendMessageW(lv, LVM_INSERTITEMW, item.iItem, (LPARAM)&item);
        item.iSubItem = 1;
        LoadStringW(hInstance,
         data->contextInfo.fExportChain ? IDS_YES : IDS_NO, text,
         sizeof(text) / sizeof(text[0]));
        SendMessageW(lv, LVM_SETITEMTEXTW, item.iItem, (LPARAM)&item);

        item.iItem = SendMessageW(lv, LVM_GETITEMCOUNT, 0, 0);
        item.iSubItem = 0;
        LoadStringW(hInstance, IDS_EXPORT_KEYS, text,
         sizeof(text) / sizeof(text[0]));
        SendMessageW(lv, LVM_INSERTITEMW, item.iItem, (LPARAM)&item);
        item.iSubItem = 1;
        LoadStringW(hInstance,
         data->contextInfo.fExportPrivateKeys ? IDS_YES : IDS_NO, text,
         sizeof(text) / sizeof(text[0]));
        SendMessageW(lv, LVM_SETITEMTEXTW, item.iItem, (LPARAM)&item);
    }
    }

    item.iItem = SendMessageW(lv, LVM_GETITEMCOUNT, 0, 0);
    item.iSubItem = 0;
    LoadStringW(hInstance, IDS_EXPORT_FORMAT, text,
     sizeof(text)/ sizeof(text[0]));
    SendMessageW(lv, LVM_INSERTITEMW, 0, (LPARAM)&item);

    item.iSubItem = 1;
    switch (data->exportInfo.dwSubjectChoice)
    {
    case CRYPTUI_WIZ_EXPORT_CRL_CONTEXT:
        contentID = IDS_EXPORT_FILTER_CRL;
        break;
    case CRYPTUI_WIZ_EXPORT_CTL_CONTEXT:
        contentID = IDS_EXPORT_FILTER_CTL;
        break;
    case CRYPTUI_WIZ_EXPORT_CERT_STORE:
        contentID = IDS_EXPORT_FILTER_SERIALIZED_CERT_STORE;
        break;
    default:
        switch (data->contextInfo.dwExportFormat)
        {
        case CRYPTUI_WIZ_EXPORT_FORMAT_BASE64:
            contentID = IDS_EXPORT_FILTER_BASE64_CERT;
            break;
        case CRYPTUI_WIZ_EXPORT_FORMAT_PKCS7:
            contentID = IDS_EXPORT_FILTER_CMS;
            break;
        case CRYPTUI_WIZ_EXPORT_FORMAT_PFX:
            contentID = IDS_EXPORT_FILTER_PFX;
            break;
        default:
            contentID = IDS_EXPORT_FILTER_CERT;
        }
    }
    LoadStringW(hInstance, contentID, text, sizeof(text) / sizeof(text[0]));
    SendMessageW(lv, LVM_SETITEMTEXTW, item.iItem, (LPARAM)&item);
}

static inline BOOL save_der(HANDLE file, const BYTE *pb, DWORD cb)
{
    DWORD bytesWritten;

    return WriteFile(file, pb, cb, &bytesWritten, NULL);
}

static BOOL save_base64(HANDLE file, const BYTE *pb, DWORD cb)
{
    BOOL ret;
    DWORD size = 0;

    if ((ret = CryptBinaryToStringA(pb, cb, CRYPT_STRING_BASE64, NULL, &size)))
    {
        LPSTR buf = HeapAlloc(GetProcessHeap(), 0, size);

        if (buf)
        {
            if ((ret = CryptBinaryToStringA(pb, cb, CRYPT_STRING_BASE64, buf,
             &size)))
                ret = WriteFile(file, buf, size, &size, NULL);
            HeapFree(GetProcessHeap(), 0, buf);
        }
        else
        {
            SetLastError(ERROR_OUTOFMEMORY);
            ret = FALSE;
        }
    }
    return ret;
}

static inline BOOL save_store_as_cms(HANDLE file, HCERTSTORE store)
{
    return CertSaveStore(store, PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
     CERT_STORE_SAVE_AS_PKCS7, CERT_STORE_SAVE_TO_FILE, file, 0);
}

static BOOL save_cert_as_cms(HANDLE file, PCCRYPTUI_WIZ_EXPORT_INFO pExportInfo,
 BOOL includeChain)
{
    BOOL ret;
    HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);

    if (store)
    {
        if (includeChain)
        {
            HCERTSTORE addlStore = CertOpenStore(CERT_STORE_PROV_COLLECTION,
             0, 0, CERT_STORE_CREATE_NEW_FLAG, NULL);

            if (addlStore)
            {
                DWORD i;

                ret = TRUE;
                for (i = 0; ret && i < pExportInfo->cStores; i++)
                    ret = CertAddStoreToCollection(addlStore,
                     pExportInfo->rghStores, 0, 0);
                if (ret)
                {
                    PCCERT_CHAIN_CONTEXT chain;

                    ret = CertGetCertificateChain(NULL,
                     pExportInfo->u.pCertContext, NULL, addlStore, NULL, 0,
                     NULL, &chain);
                    if (ret)
                    {
                        DWORD j;

                        for (i = 0; ret && i < chain->cChain; i++)
                            for (j = 0; ret && j < chain->rgpChain[i]->cElement;
                             j++)
                                ret = CertAddCertificateContextToStore(store,
                                 chain->rgpChain[i]->rgpElement[j]->pCertContext,
                                 CERT_STORE_ADD_ALWAYS, NULL);
                        CertFreeCertificateChain(chain);
                    }
                    else
                    {
                        /* No chain could be created, just add the individual
                         * cert to the message.
                         */
                        ret = CertAddCertificateContextToStore(store,
                         pExportInfo->u.pCertContext, CERT_STORE_ADD_ALWAYS,
                         NULL);
                    }
                }
                CertCloseStore(addlStore, 0);
            }
            else
                ret = FALSE;
        }
        else
            ret = CertAddCertificateContextToStore(store,
             pExportInfo->u.pCertContext, CERT_STORE_ADD_ALWAYS, NULL);
        if (ret)
            ret = save_store_as_cms(file, store);
        CertCloseStore(store, 0);
    }
    else
        ret = FALSE;
    return ret;
}

static BOOL save_serialized_store(HANDLE file, HCERTSTORE store)
{
    return CertSaveStore(store, PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
     CERT_STORE_SAVE_AS_STORE, CERT_STORE_SAVE_TO_FILE, file, 0);
}

static BOOL save_pfx(HANDLE file, PCCRYPTUI_WIZ_EXPORT_INFO pExportInfo,
 PCCRYPTUI_WIZ_EXPORT_CERTCONTEXT_INFO pContextInfo,
 PCRYPT_KEY_PROV_INFO keyProvInfo, BOOL deleteKeys)
{
    HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_MEMORY, X509_ASN_ENCODING,
     0, CERT_STORE_CREATE_NEW_FLAG, NULL);
    BOOL ret = FALSE;

    if (store)
    {
        CRYPT_DATA_BLOB pfxBlob = { 0, NULL };
        PCCERT_CONTEXT cert = NULL;
        BOOL freeKeyProvInfo = FALSE;

        if (pContextInfo->fExportChain)
        {
            HCERTCHAINENGINE engine = NULL;

            if (pExportInfo->cStores)
            {
                CERT_CHAIN_ENGINE_CONFIG config;

                memset(&config, 0, sizeof(config));
                config.cbSize = sizeof(config);
                config.cAdditionalStore = pExportInfo->cStores;
                config.rghAdditionalStore = pExportInfo->rghStores;
                ret = CertCreateCertificateChainEngine(&config, &engine);
            }
            else
                ret = TRUE;
            if (ret)
            {
                CERT_CHAIN_PARA chainPara;
                PCCERT_CHAIN_CONTEXT chain;

                memset(&chainPara, 0, sizeof(chainPara));
                chainPara.cbSize = sizeof(chainPara);
                ret = CertGetCertificateChain(engine,
                 pExportInfo->u.pCertContext, NULL, NULL, &chainPara, 0, NULL,
                 &chain);
                if (ret)
                {
                    DWORD i, j;

                    for (i = 0; ret && i < chain->cChain; i++)
                        for (j = 0; ret && j < chain->rgpChain[i]->cElement;
                         j++)
                        {
                            if (i == 0 && j == 0)
                                ret = CertAddCertificateContextToStore(store,
                                 chain->rgpChain[i]->rgpElement[j]->pCertContext,
                                 CERT_STORE_ADD_ALWAYS, &cert);
                            else
                                ret = CertAddCertificateContextToStore(store,
                                 chain->rgpChain[i]->rgpElement[j]->pCertContext,
                                 CERT_STORE_ADD_ALWAYS, NULL);
                        }
                    CertFreeCertificateChain(chain);
                }
            }
            if (engine)
                CertFreeCertificateChainEngine(engine);
        }
        else
            ret = CertAddCertificateContextToStore(store,
             pExportInfo->u.pCertContext, CERT_STORE_ADD_ALWAYS, &cert);
        /* Copy private key info to newly created cert, so it'll get exported
         * along with the cert.
         */
        if (ret && pContextInfo->fExportPrivateKeys)
        {
            if (keyProvInfo)
                ret = CertSetCertificateContextProperty(cert,
                 CERT_KEY_PROV_INFO_PROP_ID, 0, keyProvInfo);
            else
            {
                if (!(keyProvInfo = export_get_private_key_info(cert)))
                    ret = FALSE;
                else
                {
                    ret = CertSetCertificateContextProperty(cert,
                     CERT_KEY_PROV_INFO_PROP_ID, 0, keyProvInfo);
                    freeKeyProvInfo = TRUE;
                }
            }
        }
        if (ret)
        {
            DWORD exportFlags =
             REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY | EXPORT_PRIVATE_KEYS;

            ret = PFXExportCertStore(store, &pfxBlob,
             pContextInfo->pwszPassword, exportFlags);
            if (ret)
            {
                pfxBlob.pbData = HeapAlloc(GetProcessHeap(), 0, pfxBlob.cbData);
                if (pfxBlob.pbData)
                {
                    ret = PFXExportCertStore(store, &pfxBlob,
                     pContextInfo->pwszPassword, exportFlags);
                    if (ret)
                    {
                        DWORD bytesWritten;

                        ret = WriteFile(file, pfxBlob.pbData, pfxBlob.cbData,
                         &bytesWritten, NULL);
                    }
                }
                else
                {
                    SetLastError(ERROR_OUTOFMEMORY);
                    ret = FALSE;
                }
            }
        }
        if (ret && deleteKeys)
        {
            HCRYPTPROV prov;

            CryptAcquireContextW(&prov, keyProvInfo->pwszContainerName,
             keyProvInfo->pwszProvName, keyProvInfo->dwProvType,
             CRYPT_DELETEKEYSET);
        }
        if (freeKeyProvInfo)
            HeapFree(GetProcessHeap(), 0, keyProvInfo);
        CertFreeCertificateContext(cert);
        CertCloseStore(store, 0);
    }
    return ret;
}

static BOOL do_export(HANDLE file, PCCRYPTUI_WIZ_EXPORT_INFO pExportInfo,
 PCCRYPTUI_WIZ_EXPORT_CERTCONTEXT_INFO pContextInfo,
 PCRYPT_KEY_PROV_INFO keyProvInfo, BOOL deleteKeys)
{
    BOOL ret;

    if (pContextInfo->dwSize != sizeof(CRYPTUI_WIZ_EXPORT_CERTCONTEXT_INFO))
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    switch (pExportInfo->dwSubjectChoice)
    {
    case CRYPTUI_WIZ_EXPORT_CRL_CONTEXT:
        ret = save_der(file,
         pExportInfo->u.pCRLContext->pbCrlEncoded,
         pExportInfo->u.pCRLContext->cbCrlEncoded);
        break;
    case CRYPTUI_WIZ_EXPORT_CTL_CONTEXT:
        ret = save_der(file,
         pExportInfo->u.pCTLContext->pbCtlEncoded,
         pExportInfo->u.pCTLContext->cbCtlEncoded);
        break;
    case CRYPTUI_WIZ_EXPORT_CERT_STORE:
        ret = save_serialized_store(file, pExportInfo->u.hCertStore);
        break;
    case CRYPTUI_WIZ_EXPORT_CERT_STORE_CERTIFICATES_ONLY:
        ret = save_store_as_cms(file, pExportInfo->u.hCertStore);
        break;
    default:
        switch (pContextInfo->dwExportFormat)
        {
        case CRYPTUI_WIZ_EXPORT_FORMAT_DER:
            ret = save_der(file, pExportInfo->u.pCertContext->pbCertEncoded,
             pExportInfo->u.pCertContext->cbCertEncoded);
            break;
        case CRYPTUI_WIZ_EXPORT_FORMAT_BASE64:
            ret = save_base64(file,
             pExportInfo->u.pCertContext->pbCertEncoded,
             pExportInfo->u.pCertContext->cbCertEncoded);
            break;
        case CRYPTUI_WIZ_EXPORT_FORMAT_PKCS7:
            ret = save_cert_as_cms(file, pExportInfo,
             pContextInfo->fExportChain);
            break;
        case CRYPTUI_WIZ_EXPORT_FORMAT_PFX:
            ret = save_pfx(file, pExportInfo, pContextInfo, keyProvInfo,
             deleteKeys);
            break;
        default:
            SetLastError(E_FAIL);
            ret = FALSE;
        }
    }
    return ret;
}

static LRESULT CALLBACK export_finish_dlg_proc(HWND hwnd, UINT msg, WPARAM wp,
 LPARAM lp)
{
    LRESULT ret = 0;
    struct ExportWizData *data;

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        PROPSHEETPAGEW *page = (PROPSHEETPAGEW *)lp;
        HWND lv = GetDlgItem(hwnd, IDC_EXPORT_SETTINGS);
        RECT rc;
        LVCOLUMNW column;

        data = (struct ExportWizData *)page->lParam;
        SetWindowLongPtrW(hwnd, DWLP_USER, (LPARAM)data);
        SendMessageW(GetDlgItem(hwnd, IDC_EXPORT_TITLE), WM_SETFONT,
         (WPARAM)data->titleFont, TRUE);
        GetWindowRect(lv, &rc);
        column.mask = LVCF_WIDTH;
        column.cx = (rc.right - rc.left) / 2 - 2;
        SendMessageW(lv, LVM_INSERTCOLUMNW, 0, (LPARAM)&column);
        SendMessageW(lv, LVM_INSERTCOLUMNW, 1, (LPARAM)&column);
        show_export_details(lv, data);
        break;
    }
    case WM_NOTIFY:
    {
        NMHDR *hdr = (NMHDR *)lp;

        switch (hdr->code)
        {
        case PSN_SETACTIVE:
        {
            HWND lv = GetDlgItem(hwnd, IDC_EXPORT_SETTINGS);

            data = (struct ExportWizData *)GetWindowLongPtrW(hwnd, DWLP_USER);
            SendMessageW(lv, LVM_DELETEALLITEMS, 0, 0);
            show_export_details(lv, data);
            PostMessageW(GetParent(hwnd), PSM_SETWIZBUTTONS, 0,
             PSWIZB_BACK | PSWIZB_FINISH);
            ret = TRUE;
            break;
        }
        case PSN_WIZFINISH:
        {
            int messageID;
            WCHAR title[MAX_STRING_LEN], message[MAX_STRING_LEN];
            LPCWSTR pTitle;
            DWORD mbFlags;

            data = (struct ExportWizData *)GetWindowLongPtrW(hwnd, DWLP_USER);
            if ((data->success = do_export(data->file, &data->exportInfo,
             &data->contextInfo, data->keyProvInfo, data->deleteKeys)))
            {
                messageID = IDS_EXPORT_SUCCEEDED;
                mbFlags = MB_OK;
            }
            else
            {
                messageID = IDS_EXPORT_FAILED;
                mbFlags = MB_OK | MB_ICONERROR;
            }
            if (data->pwszWizardTitle)
                pTitle = data->pwszWizardTitle;
            else
            {
                LoadStringW(hInstance, IDS_EXPORT_WIZARD, title,
                 sizeof(title) / sizeof(title[0]));
                pTitle = title;
            }
            LoadStringW(hInstance, messageID, message,
             sizeof(message) / sizeof(message[0]));
            MessageBoxW(hwnd, message, pTitle, mbFlags);
            break;
        }
        }
        break;
    }
    }
    return ret;
}

static BOOL show_export_ui(DWORD dwFlags, HWND hwndParent,
 LPCWSTR pwszWizardTitle, PCCRYPTUI_WIZ_EXPORT_INFO pExportInfo, const void *pvoid)
{
    PROPSHEETHEADERW hdr;
    PROPSHEETPAGEW pages[6];
    struct ExportWizData data;
    int nPages = 0;
    BOOL hasPrivateKey, showFormatPage = TRUE;
    INT_PTR l;

    data.dwFlags = dwFlags;
    data.pwszWizardTitle = pwszWizardTitle;
    memset(&data.exportInfo, 0, sizeof(data.exportInfo));
    memcpy(&data.exportInfo, pExportInfo,
     min(sizeof(data.exportInfo), pExportInfo->dwSize));
    if (pExportInfo->dwSize > sizeof(data.exportInfo))
        data.exportInfo.dwSize = sizeof(data.exportInfo);
    data.contextInfo.dwSize = sizeof(data.contextInfo);
    data.contextInfo.dwExportFormat = CRYPTUI_WIZ_EXPORT_FORMAT_DER;
    data.contextInfo.fExportChain = FALSE;
    data.contextInfo.fStrongEncryption = FALSE;
    data.contextInfo.fExportPrivateKeys = FALSE;
    data.contextInfo.pwszPassword = NULL;
    data.freePassword = FALSE;
    if (pExportInfo->dwSubjectChoice == CRYPTUI_WIZ_EXPORT_CERT_CONTEXT &&
     pvoid)
        memcpy(&data.contextInfo, pvoid,
         min(((PCCRYPTUI_WIZ_EXPORT_CERTCONTEXT_INFO)pvoid)->dwSize,
         sizeof(data.contextInfo)));
    data.keyProvInfo = NULL;
    data.deleteKeys = FALSE;
    data.fileName = NULL;
    data.file = INVALID_HANDLE_VALUE;
    data.success = FALSE;

    memset(&pages, 0, sizeof(pages));

    pages[nPages].dwSize = sizeof(pages[0]);
    pages[nPages].hInstance = hInstance;
    pages[nPages].u.pszTemplate = MAKEINTRESOURCEW(IDD_EXPORT_WELCOME);
    pages[nPages].pfnDlgProc = export_welcome_dlg_proc;
    pages[nPages].dwFlags = PSP_HIDEHEADER;
    pages[nPages].lParam = (LPARAM)&data;
    nPages++;

    hasPrivateKey = export_info_has_private_key(pExportInfo);
    switch (pExportInfo->dwSubjectChoice)
    {
    case CRYPTUI_WIZ_EXPORT_CRL_CONTEXT:
    case CRYPTUI_WIZ_EXPORT_CTL_CONTEXT:
        showFormatPage = FALSE;
        data.contextInfo.dwExportFormat = CRYPTUI_WIZ_EXPORT_FORMAT_DER;
        break;
    case CRYPTUI_WIZ_EXPORT_CERT_STORE:
        showFormatPage = FALSE;
        data.contextInfo.dwExportFormat =
         CRYPTUI_WIZ_EXPORT_FORMAT_SERIALIZED_CERT_STORE;
        break;
    case CRYPTUI_WIZ_EXPORT_CERT_STORE_CERTIFICATES_ONLY:
        showFormatPage = FALSE;
        data.contextInfo.dwExportFormat = CRYPTUI_WIZ_EXPORT_FORMAT_PKCS7;
        break;
    }

    if (hasPrivateKey && showFormatPage)
    {
        pages[nPages].dwSize = sizeof(pages[0]);
        pages[nPages].hInstance = hInstance;
        pages[nPages].u.pszTemplate = MAKEINTRESOURCEW(IDD_EXPORT_PRIVATE_KEY);
        pages[nPages].pfnDlgProc = export_private_key_dlg_proc;
        pages[nPages].dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        pages[nPages].pszHeaderTitle =
         MAKEINTRESOURCEW(IDS_EXPORT_PRIVATE_KEY_TITLE);
        pages[nPages].pszHeaderSubTitle =
         MAKEINTRESOURCEW(IDS_EXPORT_PRIVATE_KEY_SUBTITLE);
        pages[nPages].lParam = (LPARAM)&data;
        nPages++;
    }
    if (showFormatPage)
    {
        pages[nPages].dwSize = sizeof(pages[0]);
        pages[nPages].hInstance = hInstance;
        pages[nPages].u.pszTemplate = MAKEINTRESOURCEW(IDD_EXPORT_FORMAT);
        pages[nPages].pfnDlgProc = export_format_dlg_proc;
        pages[nPages].dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        pages[nPages].pszHeaderTitle =
         MAKEINTRESOURCEW(IDS_EXPORT_FORMAT_TITLE);
        pages[nPages].pszHeaderSubTitle =
         MAKEINTRESOURCEW(IDS_EXPORT_FORMAT_SUBTITLE);
        pages[nPages].lParam = (LPARAM)&data;
        nPages++;
    }
    if (hasPrivateKey && showFormatPage)
    {
        pages[nPages].dwSize = sizeof(pages[0]);
        pages[nPages].hInstance = hInstance;
        pages[nPages].u.pszTemplate = MAKEINTRESOURCEW(IDD_EXPORT_PASSWORD);
        pages[nPages].pfnDlgProc = export_password_dlg_proc;
        pages[nPages].dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        pages[nPages].pszHeaderTitle =
         MAKEINTRESOURCEW(IDS_EXPORT_PASSWORD_TITLE);
        pages[nPages].pszHeaderSubTitle =
         MAKEINTRESOURCEW(IDS_EXPORT_PASSWORD_SUBTITLE);
        pages[nPages].lParam = (LPARAM)&data;
        nPages++;
    }

    pages[nPages].dwSize = sizeof(pages[0]);
    pages[nPages].hInstance = hInstance;
    pages[nPages].u.pszTemplate = MAKEINTRESOURCEW(IDD_EXPORT_FILE);
    pages[nPages].pfnDlgProc = export_file_dlg_proc;
    pages[nPages].dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    pages[nPages].pszHeaderTitle = MAKEINTRESOURCEW(IDS_EXPORT_FILE_TITLE);
    pages[nPages].pszHeaderSubTitle =
     MAKEINTRESOURCEW(IDS_EXPORT_FILE_SUBTITLE);
    pages[nPages].lParam = (LPARAM)&data;
    nPages++;

    pages[nPages].dwSize = sizeof(pages[0]);
    pages[nPages].hInstance = hInstance;
    pages[nPages].u.pszTemplate = MAKEINTRESOURCEW(IDD_EXPORT_FINISH);
    pages[nPages].pfnDlgProc = export_finish_dlg_proc;
    pages[nPages].dwFlags = PSP_HIDEHEADER;
    pages[nPages].lParam = (LPARAM)&data;
    nPages++;

    memset(&hdr, 0, sizeof(hdr));
    hdr.dwSize = sizeof(hdr);
    hdr.hwndParent = hwndParent;
    hdr.dwFlags = PSH_PROPSHEETPAGE | PSH_WIZARD97_NEW | PSH_HEADER |
     PSH_WATERMARK;
    hdr.hInstance = hInstance;
    if (pwszWizardTitle)
        hdr.pszCaption = pwszWizardTitle;
    else
        hdr.pszCaption = MAKEINTRESOURCEW(IDS_EXPORT_WIZARD);
    hdr.u3.ppsp = pages;
    hdr.nPages = nPages;
    hdr.u4.pszbmWatermark = MAKEINTRESOURCEW(IDB_CERT_WATERMARK);
    hdr.u5.pszbmHeader = MAKEINTRESOURCEW(IDB_CERT_HEADER);
    l = PropertySheetW(&hdr);
    DeleteObject(data.titleFont);
    if (data.freePassword)
        HeapFree(GetProcessHeap(), 0,
         (LPWSTR)data.contextInfo.pwszPassword);
    HeapFree(GetProcessHeap(), 0, data.keyProvInfo);
    CloseHandle(data.file);
    HeapFree(GetProcessHeap(), 0, data.fileName);
    if (l == 0)
    {
        SetLastError(ERROR_CANCELLED);
        return FALSE;
    }
    else
        return data.success;
}

BOOL WINAPI CryptUIWizExport(DWORD dwFlags, HWND hwndParent,
 LPCWSTR pwszWizardTitle, PCCRYPTUI_WIZ_EXPORT_INFO pExportInfo, void *pvoid)
{
    BOOL ret;

    TRACE("(%08x, %p, %s, %p, %p)\n", dwFlags, hwndParent,
     debugstr_w(pwszWizardTitle), pExportInfo, pvoid);

    if (!(dwFlags & CRYPTUI_WIZ_NO_UI))
        ret = show_export_ui(dwFlags, hwndParent, pwszWizardTitle, pExportInfo,
         pvoid);
    else
    {
        HANDLE file = CreateFileW(pExportInfo->pwszExportFileName,
         GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
         CREATE_ALWAYS, 0, NULL);

        if (file != INVALID_HANDLE_VALUE)
        {
            ret = do_export(file, pExportInfo, pvoid, NULL, FALSE);
            CloseHandle(file);
        }
        else
            ret = FALSE;
    }
    return ret;
}
