/*
 * PROJECT:     ReactOS Timedate Control Panel
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/cpl/timedate/timedate.c
 * PURPOSE:     ReactOS Timedate Control Panel
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

typedef struct _SERVERS
{
    CHAR *Address;
    WCHAR *Name;
} SERVERS;


#define NUM_APPLETS    (1)

LONG APIENTRY
Applet(HWND hwnd, UINT uMsg, LONG wParam, LONG lParam);


HINSTANCE hApplet = 0;
BOOL bSynced = FALSE;

PTIMEZONE_ENTRY TimeZoneListHead = NULL;
PTIMEZONE_ENTRY TimeZoneListTail = NULL;

static HBITMAP hBitmap = NULL;
static int cxSource, cySource;
static WNDPROC pOldWndProc = NULL;

/* Applets */
APPLET Applets[NUM_APPLETS] =
{
  {IDC_CPLICON, IDS_CPLNAME, IDS_CPLDESCRIPTION, Applet}
};

VOID GetError(VOID)
{
    LPVOID lpMsgBuf;

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  GetLastError(),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR) &lpMsgBuf,
                  0,
                  NULL );

    MessageBox(NULL, lpMsgBuf, _T("Error!"), MB_OK | MB_ICONERROR);

    LocalFree(lpMsgBuf);
}

static BOOL
SystemSetLocalTime(LPSYSTEMTIME lpSystemTime)
{
    HANDLE hToken;
    DWORD PrevSize;
    TOKEN_PRIVILEGES priv, previouspriv;
    BOOL Ret = FALSE;

    /*
     * Enable the SeSystemtimePrivilege privilege
     */

    if (OpenProcessToken(GetCurrentProcess(),
                         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                         &hToken))
    {
        priv.PrivilegeCount = 1;
        priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (LookupPrivilegeValue(NULL,
                                 SE_SYSTEMTIME_NAME,
                                 &priv.Privileges[0].Luid))
        {
            if (AdjustTokenPrivileges(hToken,
                                      FALSE,
                                      &priv,
                                      sizeof(previouspriv),
                                      &previouspriv,
                                      &PrevSize) &&
                GetLastError() == ERROR_SUCCESS)
            {
                /*
                 * We successfully enabled it, we're permitted to change the system time
                 * Call SetLocalTime twice to ensure correct results
                 */
                Ret = SetLocalTime(lpSystemTime) &&
                      SetLocalTime(lpSystemTime);

                /*
                 * For the sake of security, restore the previous status again
                 */
                if (previouspriv.PrivilegeCount > 0)
                {
                    AdjustTokenPrivileges(hToken,
                                          FALSE,
                                          &previouspriv,
                                          0,
                                          NULL,
                                          0);
                }
            }
        }
        CloseHandle(hToken);
    }

    return Ret;
}


static VOID
SetLocalSystemTime(HWND hwnd)
{
    SYSTEMTIME Time;

    if (DateTime_GetSystemtime(GetDlgItem(hwnd,
                                          IDC_TIMEPICKER),
                               &Time) == GDT_VALID &&
        SendMessage(GetDlgItem(hwnd,
                               IDC_MONTHCALENDAR),
                    MCCM_GETDATE,
                    (WPARAM)&Time,
                    0))
    {
        SystemSetLocalTime(&Time);

        SetWindowLong(hwnd,
                      DWL_MSGRESULT,
                      PSNRET_NOERROR);

        SendMessage(GetDlgItem(hwnd,
                               IDC_MONTHCALENDAR),
                    MCCM_RESET,
                    (WPARAM)&Time,
                    0);

        /* broadcast the time change message */
        SendMessage(HWND_BROADCAST,
                    WM_TIMECHANGE,
                    0,
                    0);
    }
}


static VOID
SetTimeZoneName(HWND hwnd)
{
  TIME_ZONE_INFORMATION TimeZoneInfo;
  WCHAR TimeZoneString[128];
  WCHAR TimeZoneText[128];
  WCHAR TimeZoneName[128];
  DWORD TimeZoneId;

  TimeZoneId = GetTimeZoneInformation(&TimeZoneInfo);

  LoadStringW(hApplet, IDS_TIMEZONETEXT, TimeZoneText, 128);

  switch (TimeZoneId)
  {
    case TIME_ZONE_ID_STANDARD:
      wcscpy(TimeZoneName, TimeZoneInfo.StandardName);
      break;

    case TIME_ZONE_ID_DAYLIGHT:
      wcscpy(TimeZoneName, TimeZoneInfo.DaylightName);
      break;

    case TIME_ZONE_ID_UNKNOWN:
      LoadStringW(hApplet, IDS_TIMEZONEUNKNOWN, TimeZoneName, 128);
      break;

    case TIME_ZONE_ID_INVALID:
    default:
      LoadStringW(hApplet, IDS_TIMEZONEINVALID, TimeZoneName, 128);
      break;
  }

  wsprintfW(TimeZoneString, TimeZoneText, TimeZoneName);
  SendDlgItemMessageW(hwnd, IDC_TIMEZONE, WM_SETTEXT, 0, (LPARAM)TimeZoneString);
}


static VOID
FillMonthsComboBox(HWND hCombo)
{
    SYSTEMTIME LocalDate = {0};
    WCHAR szBuf[64];
    INT i;
    UINT Month;

    GetLocalTime(&LocalDate);

    SendMessage(hCombo,
                CB_RESETCONTENT,
                0,
                0);

    for (Month = 1;
         Month <= 13;
         Month++)
    {
        i = GetLocaleInfoW(LOCALE_USER_DEFAULT,
                           ((Month < 13) ? LOCALE_SMONTHNAME1 + Month - 1 : LOCALE_SMONTHNAME13),
                           szBuf,
                           sizeof(szBuf) / sizeof(szBuf[0]));
        if (i > 1)
        {
            i = (INT)SendMessage(hCombo,
                                 CB_ADDSTRING,
                                 0,
                                 (LPARAM)szBuf);
            if (i != CB_ERR)
            {
                SendMessage(hCombo,
                            CB_SETITEMDATA,
                            (WPARAM)i,
                            Month);

                if (Month == (UINT)LocalDate.wMonth)
                {
                    SendMessage(hCombo,
                                CB_SETCURSEL,
                                (WPARAM)i,
                                0);
                }
            }
        }
    }
}


static WORD
GetCBSelectedMonth(HWND hCombo)
{
    INT i;
    WORD Ret = (WORD)-1;

    i = (INT)SendMessage(hCombo,
                         CB_GETCURSEL,
                         0,
                         0);
    if (i != CB_ERR)
    {
        i = (INT)SendMessage(hCombo,
                             CB_GETITEMDATA,
                             (WPARAM)i,
                             0);

        if (i >= 1 && i <= 13)
            Ret = (WORD)i;
    }

    return Ret;
}


static VOID
ChangeMonthCalDate(HWND hMonthCal,
                   WORD Day,
                   WORD Month,
                   WORD Year)
{
    SendMessage(hMonthCal,
                MCCM_SETDATE,
                MAKEWPARAM(Day,
                           Month),
                MAKELPARAM(Year,
                           0));
}

static VOID
AutoUpdateMonthCal(HWND hwndDlg,
                   PNMMCCAUTOUPDATE lpAutoUpdate)
{
    UNREFERENCED_PARAMETER(lpAutoUpdate);

    /* update the controls */
    FillMonthsComboBox(GetDlgItem(hwndDlg,
                                  IDC_MONTHCB));
}


INT_PTR CALLBACK
DTPProc(HWND hwnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_KEYDOWN:
      /* stop the timer when the user is about to change the time */
      if ((wParam != VK_LEFT) & (wParam != VK_RIGHT))
        KillTimer(GetParent(hwnd), ID_TIMER);
      break;

  }

  return CallWindowProc(pOldWndProc, hwnd, uMsg, wParam, lParam);
}

/* Property page dialog callback */
INT_PTR CALLBACK
DateTimePageProc(HWND hwndDlg,
         UINT uMsg,
         WPARAM wParam,
         LPARAM lParam)
{
  SYSTEMTIME st;
  GetLocalTime(&st);

  switch (uMsg)
  {
    case WM_INITDIALOG:
        FillMonthsComboBox(GetDlgItem(hwndDlg,
                                      IDC_MONTHCB));

        SetTimer(hwndDlg, ID_TIMER, 1000, NULL);

        /* set range and current year */
        SendMessage(GetDlgItem(hwndDlg, IDC_YEAR), UDM_SETRANGE, 0, MAKELONG ((short) 9999, (short) 1900));
        SendMessage(GetDlgItem(hwndDlg, IDC_YEAR), UDM_SETPOS, 0, MAKELONG( (short) st.wYear, 0));

		pOldWndProc = (WNDPROC) SetWindowLong(GetDlgItem(hwndDlg, IDC_TIMEPICKER), GWL_WNDPROC, (INT_PTR) DTPProc);

    break;

    case WM_TIMER:
    {
        SendMessage(GetDlgItem(hwndDlg, IDC_TIMEPICKER), DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM) &st);
        break;
    }
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
            case IDC_MONTHCB:
            {
                if (HIWORD(wParam) == CBN_SELCHANGE)
                {
                    ChangeMonthCalDate(GetDlgItem(hwndDlg,
                                                  IDC_MONTHCALENDAR),
                                       -1,
                                       GetCBSelectedMonth((HWND)lParam),
                                       -1);
                }
                break;
            }
        }
        break;
    }
    case WM_CTLCOLORSTATIC:
    {
        if ((HWND) lParam == GetDlgItem(hwndDlg, IDC_YEARTEXT))
            return (int) GetSysColorBrush(COLOR_WINDOW);
        break;
    }
    case WM_NOTIFY:
      {
          LPNMHDR lpnm = (LPNMHDR)lParam;

          switch (lpnm->idFrom)
          {
              case IDC_YEAR:
                  switch (lpnm->code)
                  {
				      case UDN_DELTAPOS:
                      {
                         short wYear;
                         LPNMUPDOWN updown = (LPNMUPDOWN) lpnm;
                         wYear = SendMessage(GetDlgItem(hwndDlg, IDC_YEAR), UDM_GETPOS, 0, 0);
                          /* Enable the 'Apply' button */
                         PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                         ChangeMonthCalDate(GetDlgItem(hwndDlg,
                                       IDC_MONTHCALENDAR),
                                       -1,
                                       -1,
                                       wYear + updown->iDelta);
                          break;
                      }
                  }
                  break;
              case IDC_TIMEPICKER:
                  switch (lpnm->code)
                  {
                      case DTN_DATETIMECHANGE:
                          /* Enable the 'Apply' button */
                          KillTimer(hwndDlg, ID_TIMER);
                          PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                          break;
                  }
                  break;

              case IDC_MONTHCALENDAR:
                  switch (lpnm->code)
                  {
                      case MCCN_SELCHANGE:
                          /* Enable the 'Apply' button */
                          PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                          break;

                      case MCCN_AUTOUPDATE:
                          AutoUpdateMonthCal(hwndDlg,
                                             (PNMMCCAUTOUPDATE)lpnm);
                          break;
                  }
                  break;

              default:
                  switch (lpnm->code)
                  {
                      case PSN_SETACTIVE:
                          SetTimeZoneName(hwndDlg);
                          break;

                      case PSN_APPLY:
                          SetLocalSystemTime(hwndDlg);
                          SetTimer(hwndDlg, ID_TIMER, 1000, NULL);
                          return TRUE;
                  }
          }
      }
      break;

    case WM_TIMECHANGE:
    {
        /* FIXME - we don't get this message as we're not a top-level window... */
        SendMessage(GetDlgItem(hwndDlg,
                               IDC_MONTHCALENDAR),
                    MCCM_RESET,
                    0,
                    0);
        break;
    }
    case WM_DESTROY:
    {
        KillTimer(hwndDlg, ID_TIMER);
        break;
    }
  }

  return FALSE;
}


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
      dwNameSize = 256;
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

      if (RegOpenKeyExW(hZonesKey,
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

  dwIndex = SendMessage(hwnd,
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

  if (RegQueryValueExW(hKey,
               L"DisableAutoDaylightTimeSet",
               NULL,
               NULL,
               NULL,
               NULL))
    {
      SendMessage(hwnd, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
    }
  else
    {
      SendMessage(hwnd, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
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

  if (SendMessage(hwnd, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
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
      hBitmap = LoadImage(hApplet, MAKEINTRESOURCE(IDC_WORLD), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
      if (hBitmap != NULL)
      {
         GetObject(hBitmap, sizeof(BITMAP), &bitmap);

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
            PAINTSTRUCT ps;
            HDC hdc, hdcMem;
            hdc = BeginPaint(hwndDlg, &ps);
            hdcMem = CreateCompatibleDC(hdc);
            SelectObject(hdcMem, hBitmap);
            StretchBlt(lpDrawItem->hDC, lpDrawItem->rcItem.left, lpDrawItem->rcItem.top,
                       lpDrawItem->rcItem.right - lpDrawItem->rcItem.left,
                       lpDrawItem->rcItem.bottom - lpDrawItem->rcItem.top,
                       hdcMem, 0, 0, cxSource, cySource, SRCCOPY);
            DeleteDC(hdcMem);
            EndPaint(hwndDlg, &ps);
		}
        break;
    }
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
                SetAutoDaylightInfo(GetDlgItem(hwndDlg, IDC_AUTODAYLIGHT));
                SetLocalTimeZone(GetDlgItem(hwndDlg, IDC_TIMEZONELIST));
                SetWindowLong(hwndDlg, DWL_MSGRESULT, PSNRET_NOERROR);
                return TRUE;

              default:
                break;
            }

      }
      break;
  }

  return FALSE;
}


#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

static VOID
CreateNTPServerList(HWND hwnd)
{
    HWND hList;
    WCHAR ValName[MAX_VALUE_NAME];
    WCHAR Data[256];
    DWORD Index = 0;
    DWORD ValSize;
    DWORD dwNameSize;
    DWORD Default = 1;
    LONG Ret;
    HKEY hKey;

    hList = GetDlgItem(hwnd,
                       IDC_SERVERLIST);

    Ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DateTime\\Servers",
                        0,
                        KEY_QUERY_VALUE,
                        &hKey);
    if (Ret != ERROR_SUCCESS)
        return;

    while (TRUE)
    {
            ValSize = MAX_VALUE_NAME;
            ValName[0] = '\0';
            Ret = RegEnumValueW(hKey,
                                Index,
                                ValName,
                                &ValSize,
                                NULL,
                                NULL,
                                (LPBYTE)Data,
                                &dwNameSize);

            if (Ret == ERROR_SUCCESS)
            {
                if (wcscmp(ValName, L"") == 0)
                {
                    Default = _wtoi(Data);
                    Index++;
                }
                else
                {
                    SendMessageW(hList,
                                 CB_ADDSTRING,
                                 0,
                                 (LPARAM)Data);
                    Index++;
                }
            }
            else if (Ret != ERROR_MORE_DATA)
                break;
    }

    if (Default < 1 || Default > Index)
        Default = 1;

    SendMessage(hList,
                CB_SETCURSEL,
                --Default, /* reg entries count from 1 */
                0);

    RegCloseKey(hKey);

}

/* Set the selected server in the registry */
static VOID
SetNTPServer(HWND hwnd)
{
    HKEY hKey;
    HWND hList;
    UINT Sel;
    WCHAR szSel[4];
    LONG Ret;

    hList = GetDlgItem(hwnd,
                       IDC_SERVERLIST);

    Sel = (UINT)SendMessage(hList,
                            CB_GETCURSEL,
                            0,
                            0);

    /* reg entries count from 1 */
    _itow(++Sel, szSel, 10);

    Ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DateTime\\Servers",
                        0,
                        KEY_SET_VALUE,
                        &hKey);
    if (Ret != ERROR_SUCCESS)
    {
        GetError();
        return;
    }

    Ret = RegSetValueExW(hKey,
                         L"",
                         0,
                         REG_SZ,
                         (LPBYTE)szSel,
                         sizeof(szSel));
    if (Ret != ERROR_SUCCESS)
        GetError();

    RegCloseKey(hKey);

}


/* get the dotted decimal address from the registry */
static BOOL
GetNTPServerAddress(CHAR *szIpAddr)
{
    HKEY hKey;
    WCHAR szSel[4];
    WCHAR buf[32];
    DWORD dwSize;
    LONG Ret;

    Ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DateTime\\Servers",
                        0,
                        KEY_QUERY_VALUE,
                        &hKey);
    if (Ret != ERROR_SUCCESS)
    {
        GetError();
        return FALSE;
    }

    dwSize = sizeof(szSel);
    Ret = RegQueryValueExW(hKey,
                           L"",
                           NULL,
                           NULL,
                           (LPBYTE)szSel,
                           &dwSize);
    if (Ret != ERROR_SUCCESS)
    {
        GetError();
        return FALSE;
    }

    dwSize = sizeof(szSel);
    Ret = RegQueryValueExW(hKey,
                           szSel,
                           NULL,
                           NULL,
                           (LPBYTE)buf,
                           &dwSize);
    if (Ret != ERROR_SUCCESS)
    {
        GetError();
        return FALSE;
    }

    if (WideCharToMultiByte(CP_ACP,
                            0,
                            buf,
                            sizeof(buf),
                            szIpAddr,
                            sizeof(szIpAddr),
                            NULL,
                            NULL) == 0)
    {
        GetError();
        return FALSE;
    }

    /* safety check */
    if (inet_addr(szIpAddr) == INADDR_NONE)
        return FALSE;

    return TRUE;
}


/* request the time from the current NTP server */
static ULONG
GetTimeFromServer(VOID)
{
    CHAR szIpAddr[32];
    ULONG ulTime = 0;

    if (! GetNTPServerAddress(szIpAddr))
        return 0;

    if (InitialiseConnection(szIpAddr))
    {
        if (SendData())
        {
            RecieveData(ulTime);
        }
    }

    DestroyConnection();

    return ulTime;
}

/*
 * NTP servers state the number of seconds passed since
 * 1st Jan, 1900. The time returned from the server
 * needs adding to that date to get the current Gregorian time
 */
static VOID
UpdateSystemTime(ULONG ulTime)
{
    FILETIME ftNew;
    LARGE_INTEGER li;
    SYSTEMTIME stNew;

    /* time at 1st Jan 1900 */
    stNew.wYear = 1900;
    stNew.wMonth = 1;
    stNew.wDay = 1;
    stNew.wHour = 0;
    stNew.wMinute = 0;
    stNew.wSecond = 0;
    stNew.wMilliseconds = 0;

    /* convert to a file time */
    SystemTimeToFileTime(&stNew, &ftNew);

    /* add on the time passed since 1st Jan 1900 */
    li = *(LARGE_INTEGER *)&ftNew;
    li.QuadPart += (LONGLONG)10000000 * ulTime;
    ftNew = * (FILETIME *)&li;

    /* convert back to a system time */
    FileTimeToSystemTime(&ftNew, &stNew);

    if (! SetSystemTime(&stNew))
         GetError();

}


/* Property page dialog callback */
INT_PTR CALLBACK
InetTimePageProc(HWND hwndDlg,
                 UINT uMsg,
                 WPARAM wParam,
                 LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            CreateNTPServerList(hwndDlg);

        break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_UPDATEBUTTON:
                {
                    ULONG ulTime;

                    SetNTPServer(hwndDlg);

                    ulTime = GetTimeFromServer();
                    if (ulTime != 0)
                        UpdateSystemTime(ulTime);

                }
                break;

                case IDC_SERVERLIST:
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                        /* Enable the 'Apply' button */
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                }
                break;

                case IDC_AUTOSYNC:
                {
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        BOOL bChecked;
                        HWND hCheck = GetDlgItem(hwndDlg, IDC_AUTOSYNC);
                        UINT Check = (UINT)SendMessageW(hCheck, BM_GETCHECK, 0, 0);

                        bChecked = (Check == BST_CHECKED) ? TRUE : FALSE;

                        EnableWindow(GetDlgItem(hwndDlg, IDC_SERVERTEXT), bChecked);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_SERVERLIST), bChecked);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_UPDATEBUTTON), bChecked);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_SUCSYNC), bChecked);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_NEXTSYNC), bChecked);

                        /* Enable the 'Apply' button */
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                }
                break;
            }
            break;

        case WM_DESTROY:
        break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_APPLY:

                    SetNTPServer(hwndDlg);

                    return TRUE;

                default:
                break;
            }
        }
        break;
    }

    return FALSE;
}


static VOID
InitPropSheetPage(PROPSHEETPAGE *psp, WORD idDlg, DLGPROC DlgProc)
{
  ZeroMemory(psp, sizeof(PROPSHEETPAGE));
  psp->dwSize = sizeof(PROPSHEETPAGE);
  psp->dwFlags = PSP_DEFAULT;
  psp->hInstance = hApplet;
  psp->pszTemplate = MAKEINTRESOURCE(idDlg);
  psp->pfnDlgProc = DlgProc;
}


LONG APIENTRY
Applet(HWND hwnd, UINT uMsg, LONG wParam, LONG lParam)
{
  PROPSHEETHEADER psh;
  PROPSHEETPAGE psp[3];
  TCHAR Caption[256];
  LONG Ret = 0;

  if (RegisterMonthCalControl(hApplet) &&
      RegisterClockControl())
  {
    LoadString(hApplet, IDS_CPLNAME, Caption, sizeof(Caption) / sizeof(TCHAR));

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_PROPTITLE;
    psh.hwndParent = NULL;
    psh.hInstance = hApplet;
    psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDC_CPLICON));
    psh.pszCaption = Caption;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = psp;

    InitPropSheetPage(&psp[0], IDD_DATETIMEPAGE, DateTimePageProc);
    InitPropSheetPage(&psp[1], IDD_TIMEZONEPAGE, TimeZonePageProc);
    InitPropSheetPage(&psp[2], IDD_INETTIMEPAGE, InetTimePageProc);

    Ret = (LONG)(PropertySheet(&psh) != -1);

    UnregisterMonthCalControl(hApplet);
    UnregisterClockControl();
  }

  return Ret;
}


/* Control Panel Callback */
LONG CALLBACK
CPlApplet(HWND hwndCpl,
      UINT uMsg,
      LPARAM lParam1,
      LPARAM lParam2)
{
  int i = (int)lParam1;

  switch (uMsg)
  {
    case CPL_INIT:
      return TRUE;

    case CPL_GETCOUNT:
      return NUM_APPLETS;

    case CPL_INQUIRE:
    {
      CPLINFO *CPlInfo = (CPLINFO*)lParam2;
      CPlInfo->lData = 0;
      CPlInfo->idIcon = Applets[i].idIcon;
      CPlInfo->idName = Applets[i].idName;
      CPlInfo->idInfo = Applets[i].idDescription;
      break;
    }

    case CPL_DBLCLK:
    {
      Applets[i].AppletProc(hwndCpl, uMsg, lParam1, lParam2);
      break;
    }
  }
  return FALSE;
}


BOOL STDCALL
DllMain(HINSTANCE hinstDLL,
    DWORD dwReason,
    LPVOID lpReserved)
{
  switch (dwReason)
  {
    case DLL_PROCESS_ATTACH:
    {
      INITCOMMONCONTROLSEX InitControls;

      InitControls.dwSize = sizeof(INITCOMMONCONTROLSEX);
      InitControls.dwICC = ICC_DATE_CLASSES | ICC_PROGRESS_CLASS | ICC_UPDOWN_CLASS;
      InitCommonControlsEx(&InitControls);

      hApplet = hinstDLL;
    }
    break;
  }

  return TRUE;
}

/* EOF */
