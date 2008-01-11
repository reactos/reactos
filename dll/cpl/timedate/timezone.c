/*
 * PROJECT:     ReactOS Timedate Control Panel
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/cpl/timedate/timezone.c
 * PURPOSE:     Time Zone property page
 * COPYRIGHT:   Copyright 2004-2005 Eric Kohl
 *              Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *              Copyright 2006 Christoph v. Wittich <Christoph@ActiveVB.de>
 *
 */

#include <timedate.h>

typedef struct _TZ_INFO
{
    LONG Bias;
    LONG StandardBias;
    LONG DaylightBias;
    SYSTEMTIME StandardDate;
    SYSTEMTIME DaylightDate;
} TZ_INFO, *PTZ_INFO;

typedef struct _TIMEZONE_ENTRY
{
    struct _TIMEZONE_ENTRY *Prev;
    struct _TIMEZONE_ENTRY *Next;
    WCHAR Description[64];   /* 'Display' */
    WCHAR StandardName[33];  /* 'Std' */
    WCHAR DaylightName[33];  /* 'Dlt' */
    TZ_INFO TimezoneInfo;    /* 'TZI' */
    ULONG Index;             /* 'Index ' */
} TIMEZONE_ENTRY, *PTIMEZONE_ENTRY;


static HBITMAP hBitmap = NULL;
static int cxSource, cySource;

PTIMEZONE_ENTRY TimeZoneListHead = NULL;
PTIMEZONE_ENTRY TimeZoneListTail = NULL;

static PTIMEZONE_ENTRY
GetLargerTimeZoneEntry(DWORD Index)
{
    PTIMEZONE_ENTRY Entry;

    Entry = TimeZoneListHead;
    while (Entry != NULL)
    {
        if (Entry->Index >= Index)
            return Entry;

        Entry = Entry->Next;
    }

    return NULL;
}


static VOID
CreateTimeZoneList(VOID)
{
    WCHAR szKeyName[256];
    DWORD dwIndex;
    DWORD dwNameSize;
    DWORD dwValueSize;
    LONG lError;
    HKEY hZonesKey;
    HKEY hZoneKey;

    PTIMEZONE_ENTRY Entry;
    PTIMEZONE_ENTRY Current;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones",
                      0,
                      KEY_ENUMERATE_SUB_KEYS,
                      &hZonesKey))
        return;

    dwIndex = 0;
    while (TRUE)
    {
        dwNameSize = 256 * sizeof(WCHAR);
        lError = RegEnumKeyExW(hZonesKey,
                               dwIndex,
                               szKeyName,
                               &dwNameSize,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
        if (lError != ERROR_SUCCESS && lError != ERROR_MORE_DATA)
            break;

        if (RegOpenKeyEx (hZonesKey,
                          szKeyName,
                          0,
                          KEY_QUERY_VALUE,
                          &hZoneKey))
            break;

        Entry = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TIMEZONE_ENTRY));
        if (Entry == NULL)
        {
            RegCloseKey(hZoneKey);
            break;
        }

        dwValueSize = 64 * sizeof(WCHAR);
        if (RegQueryValueExW(hZoneKey,
                             L"Display",
                             NULL,
                             NULL,
                             (LPBYTE)&Entry->Description,
                             &dwValueSize))
        {
            RegCloseKey(hZoneKey);
            break;
        }

        dwValueSize = 33 * sizeof(WCHAR);
        if (RegQueryValueExW(hZoneKey,
                             L"Std",
                             NULL,
                             NULL,
                             (LPBYTE)&Entry->StandardName,
                             &dwValueSize))
        {
            RegCloseKey(hZoneKey);
            break;
        }

        dwValueSize = 33 * sizeof(WCHAR);
        if (RegQueryValueExW(hZoneKey,
                             L"Dlt",
                             NULL,
                             NULL,
                             (LPBYTE)&Entry->DaylightName,
                             &dwValueSize))
        {
            RegCloseKey(hZoneKey);
            break;
        }

        dwValueSize = sizeof(DWORD);
        if (RegQueryValueExW(hZoneKey,
                             L"Index",
                             NULL,
                             NULL,
                             (LPBYTE)&Entry->Index,
                             &dwValueSize))
        {
            RegCloseKey(hZoneKey);
            break;
        }

        dwValueSize = sizeof(TZ_INFO);
        if (RegQueryValueExW(hZoneKey,
                             L"TZI",
                             NULL,
                             NULL,
                             (LPBYTE)&Entry->TimezoneInfo,
                             &dwValueSize))
        {
            RegCloseKey(hZoneKey);
            break;
        }

        RegCloseKey(hZoneKey);

        if (TimeZoneListHead == NULL &&
            TimeZoneListTail == NULL)
        {
            Entry->Prev = NULL;
            Entry->Next = NULL;
            TimeZoneListHead = Entry;
            TimeZoneListTail = Entry;
        }
        else
        {
            Current = GetLargerTimeZoneEntry(Entry->Index);
            if (Current != NULL)
            {
                if (Current == TimeZoneListHead)
                {
                    /* Prepend to head */
                    Entry->Prev = NULL;
                    Entry->Next = TimeZoneListHead;
                    TimeZoneListHead->Prev = Entry;
                    TimeZoneListHead = Entry;
                }
                else
                {
                    /* Insert before current */
                    Entry->Prev = Current->Prev;
                    Entry->Next = Current;
                    Current->Prev->Next = Entry;
                    Current->Prev = Entry;
                }
            }
            else
            {
                /* Append to tail */
                Entry->Prev = TimeZoneListTail;
                Entry->Next = NULL;
                TimeZoneListTail->Next = Entry;
                TimeZoneListTail = Entry;
            }
        }

        dwIndex++;
    }

    RegCloseKey(hZonesKey);
}


static VOID
DestroyTimeZoneList(VOID)
{
    PTIMEZONE_ENTRY Entry;

    while (TimeZoneListHead != NULL)
    {
        Entry = TimeZoneListHead;

        TimeZoneListHead = Entry->Next;
        if (TimeZoneListHead != NULL)
        {
            TimeZoneListHead->Prev = NULL;
        }

        HeapFree(GetProcessHeap(), 0, Entry);
    }

    TimeZoneListTail = NULL;
}


static VOID
ShowTimeZoneList(HWND hwnd)
{
    TIME_ZONE_INFORMATION TimeZoneInfo;
    PTIMEZONE_ENTRY Entry;
    DWORD dwIndex;
    DWORD i;

    GetTimeZoneInformation(&TimeZoneInfo);

    dwIndex = 0;
    i = 0;
    Entry = TimeZoneListHead;
    while (Entry != NULL)
    {
        SendMessageW(hwnd,
                     CB_ADDSTRING,
                     0,
                     (LPARAM)Entry->Description);

        if (!wcscmp(Entry->StandardName, TimeZoneInfo.StandardName))
            dwIndex = i;

        i++;
        Entry = Entry->Next;
    }

    SendMessageW(hwnd,
                 CB_SETCURSEL,
                 (WPARAM)dwIndex,
                 0);
}


static VOID
SetLocalTimeZone(HWND hwnd)
{
    TIME_ZONE_INFORMATION TimeZoneInformation;
    PTIMEZONE_ENTRY Entry;
    DWORD dwIndex;
    DWORD i;

    dwIndex = (DWORD)SendMessageW(hwnd,
                                  CB_GETCURSEL,
                                  0,
                                  0);

    i = 0;
    Entry = TimeZoneListHead;
    while (i < dwIndex)
    {
        if (Entry == NULL)
            return;

        i++;
        Entry = Entry->Next;
    }

    wcscpy(TimeZoneInformation.StandardName,
            Entry->StandardName);
    wcscpy(TimeZoneInformation.DaylightName,
            Entry->DaylightName);

    TimeZoneInformation.Bias = Entry->TimezoneInfo.Bias;
    TimeZoneInformation.StandardBias = Entry->TimezoneInfo.StandardBias;
    TimeZoneInformation.DaylightBias = Entry->TimezoneInfo.DaylightBias;

    memcpy(&TimeZoneInformation.StandardDate,
           &Entry->TimezoneInfo.StandardDate,
           sizeof(SYSTEMTIME));
    memcpy(&TimeZoneInformation.DaylightDate,
           &Entry->TimezoneInfo.DaylightDate,
           sizeof(SYSTEMTIME));

    /* Set time zone information */
    SetTimeZoneInformation(&TimeZoneInformation);
}


static VOID
GetAutoDaylightInfo(HWND hwnd)
{
    HKEY hKey;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation",
                      0,
                      KEY_QUERY_VALUE,
                      &hKey))
        return;

    /* if the call fails (non zero), the reg value isn't available,
     * which means it shouldn't be disabled, so we should check the button.
     */
    if (RegQueryValueExW(hKey,
                         L"DisableAutoDaylightTimeSet",
                         NULL,
                         NULL,
                         NULL,
                         NULL))
    {
        SendMessageW(hwnd, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
    }
    else
    {
        SendMessageW(hwnd, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
    }

    RegCloseKey(hKey);
}


static VOID
SetAutoDaylightInfo(HWND hwnd)
{
    HKEY hKey;
    DWORD dwValue = 1;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation",
                      0,
                      KEY_SET_VALUE,
                      &hKey))
        return;

    if (SendMessageW(hwnd, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
    {
        RegSetValueExW(hKey,
                       L"DisableAutoDaylightTimeSet",
                       0,
                       REG_DWORD,
                       (LPBYTE)&dwValue,
                       sizeof(DWORD));
    }
    else
    {
        RegDeleteValueW(hKey,
                       L"DisableAutoDaylightTimeSet");
    }

    RegCloseKey(hKey);
}


/* Property page dialog callback */
INT_PTR CALLBACK
TimeZonePageProc(HWND hwndDlg,
                 UINT uMsg,
                 WPARAM wParam,
                 LPARAM lParam)
{
    BITMAP bitmap;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            CreateTimeZoneList();
            ShowTimeZoneList(GetDlgItem(hwndDlg, IDC_TIMEZONELIST));
            GetAutoDaylightInfo(GetDlgItem(hwndDlg, IDC_AUTODAYLIGHT));
            hBitmap = LoadImageW(hApplet, MAKEINTRESOURCEW(IDC_WORLD), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
            if (hBitmap != NULL)
            {
                GetObjectW(hBitmap, sizeof(BITMAP), &bitmap);

                cxSource = bitmap.bmWidth;
                cySource = bitmap.bmHeight;
            }
            break;

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT lpDrawItem;
            lpDrawItem = (LPDRAWITEMSTRUCT) lParam;
            if(lpDrawItem->CtlID == IDC_WORLD_BACKGROUND)
            {
                HDC hdcMem;
                hdcMem = CreateCompatibleDC(lpDrawItem->hDC);
                if (hdcMem != NULL)
                {
                    SelectObject(hdcMem, hBitmap);
                    StretchBlt(lpDrawItem->hDC, lpDrawItem->rcItem.left, lpDrawItem->rcItem.top,
                               lpDrawItem->rcItem.right - lpDrawItem->rcItem.left,
                               lpDrawItem->rcItem.bottom - lpDrawItem->rcItem.top,
                               hdcMem, 0, 0, cxSource, cySource, SRCCOPY);
                    DeleteDC(hdcMem);
                }
            }
        }
        break;

        case WM_COMMAND:
            if ((LOWORD(wParam) == IDC_TIMEZONELIST && HIWORD(wParam) == CBN_SELCHANGE) ||
                (LOWORD(wParam) == IDC_AUTODAYLIGHT && HIWORD(wParam) == BN_CLICKED))
            {
                /* Enable the 'Apply' button */
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
            }
            break;

        case WM_DESTROY:
            DestroyTimeZoneList();
            DeleteObject(hBitmap);
            break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_APPLY:
                {
                    SetAutoDaylightInfo(GetDlgItem(hwndDlg, IDC_AUTODAYLIGHT));
                    SetLocalTimeZone(GetDlgItem(hwndDlg, IDC_TIMEZONELIST));
                    SetWindowLong(hwndDlg, DWL_MSGRESULT, PSNRET_NOERROR);
                    return TRUE;
                }

                default:
                    break;
            }
        }
        break;
    }

  return FALSE;
}
