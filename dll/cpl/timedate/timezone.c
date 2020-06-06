/*
 * PROJECT:     ReactOS Timedate Control Panel
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/timedate/timezone.c
 * PURPOSE:     Time Zone property page
 * COPYRIGHT:   Copyright 2004-2005 Eric Kohl
 *              Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *              Copyright 2006 Christoph v. Wittich <Christoph@ActiveVB.de>
 *
 */

#include "timedate.h"
#include <tzlib.h>

typedef struct _TIMEZONE_ENTRY
{
    struct _TIMEZONE_ENTRY *Prev;
    struct _TIMEZONE_ENTRY *Next;
    WCHAR Description[128]; /* 'Display' */
    WCHAR StandardName[33]; /* 'Std' */
    WCHAR DaylightName[33]; /* 'Dlt' */
    REG_TZI_FORMAT TimezoneInfo; /* 'TZI' */
} TIMEZONE_ENTRY, *PTIMEZONE_ENTRY;


static HBITMAP hBitmap = NULL;
static int cxSource, cySource;

PTIMEZONE_ENTRY TimeZoneListHead = NULL;
PTIMEZONE_ENTRY TimeZoneListTail = NULL;

static
PTIMEZONE_ENTRY
GetLargerTimeZoneEntry(
    LONG Bias,
    LPWSTR lpDescription)
{
    PTIMEZONE_ENTRY Entry;

    Entry = TimeZoneListHead;
    while (Entry != NULL)
    {
        if (Entry->TimezoneInfo.Bias < Bias)
            return Entry;

        if (Entry->TimezoneInfo.Bias == Bias)
        {
            if (_wcsicmp(Entry->Description, lpDescription) > 0)
                return Entry;
        }

        Entry = Entry->Next;
    }

    return NULL;
}

static LONG
RetrieveTimeZone(
    IN HKEY hZoneKey,
    IN PVOID Context)
{
    LONG lError;
    PTIMEZONE_ENTRY Entry;
    PTIMEZONE_ENTRY Current;
    ULONG DescriptionSize;
    ULONG StandardNameSize;
    ULONG DaylightNameSize;

    Entry = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TIMEZONE_ENTRY));
    if (Entry == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    DescriptionSize  = sizeof(Entry->Description);
    StandardNameSize = sizeof(Entry->StandardName);
    DaylightNameSize = sizeof(Entry->DaylightName);

    lError = QueryTimeZoneData(hZoneKey,
                               NULL,
                               &Entry->TimezoneInfo,
                               Entry->Description,
                               &DescriptionSize,
                               Entry->StandardName,
                               &StandardNameSize,
                               Entry->DaylightName,
                               &DaylightNameSize);
    if (lError != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, Entry);
        return lError;
    }

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
        Current = GetLargerTimeZoneEntry(Entry->TimezoneInfo.Bias, Entry->Description);
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

    return ERROR_SUCCESS;
}

static VOID
CreateTimeZoneList(VOID)
{
    EnumerateTimeZoneList(RetrieveTimeZone, NULL);
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
    BOOL bDoAdvancedTest;
    DWORD dwIndex;
    DWORD i;

    GetTimeZoneInformation(&TimeZoneInfo);
    bDoAdvancedTest = (!*TimeZoneInfo.StandardName);

    dwIndex = 0;
    i = 0;
    Entry = TimeZoneListHead;
    while (Entry != NULL)
    {
        SendMessageW(hwnd,
                     CB_ADDSTRING,
                     0,
                     (LPARAM)Entry->Description);

        if ( (!bDoAdvancedTest && *Entry->StandardName &&
                wcscmp(Entry->StandardName, TimeZoneInfo.StandardName) == 0) ||
             ( (Entry->TimezoneInfo.Bias == TimeZoneInfo.Bias) &&
               (Entry->TimezoneInfo.StandardBias == TimeZoneInfo.StandardBias) &&
               (Entry->TimezoneInfo.DaylightBias == TimeZoneInfo.DaylightBias) &&
               (memcmp(&Entry->TimezoneInfo.StandardDate, &TimeZoneInfo.StandardDate, sizeof(SYSTEMTIME)) == 0) &&
               (memcmp(&Entry->TimezoneInfo.DaylightDate, &TimeZoneInfo.DaylightDate, sizeof(SYSTEMTIME)) == 0) ) )
        {
            dwIndex = i;
        }

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
        {
            CreateTimeZoneList();
            ShowTimeZoneList(GetDlgItem(hwndDlg, IDC_TIMEZONELIST));

            SendDlgItemMessage(hwndDlg, IDC_AUTODAYLIGHT, BM_SETCHECK,
                               (WPARAM)(GetAutoDaylight() ? BST_CHECKED : BST_UNCHECKED), 0);

            hBitmap = LoadImageW(hApplet, MAKEINTRESOURCEW(IDC_WORLD), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
            if (hBitmap != NULL)
            {
                GetObjectW(hBitmap, sizeof(bitmap), &bitmap);

                cxSource = bitmap.bmWidth;
                cySource = bitmap.bmHeight;
            }
            break;
        }

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
                    SetAutoDaylight(SendDlgItemMessage(hwndDlg, IDC_AUTODAYLIGHT,
                                                       BM_GETCHECK, 0, 0) != BST_UNCHECKED);
                    SetLocalTimeZone(GetDlgItem(hwndDlg, IDC_TIMEZONELIST));
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
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
