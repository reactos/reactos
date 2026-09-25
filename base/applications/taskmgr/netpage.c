/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Networking Page
 * COPYRIGHT:   Copyright 2026 ReactOS Team
 */

#include "precomp.h"

#include <shlwapi.h>
#include <iphlpapi.h>

/* Utilization is stored in 1/1000 of a percent, so 100% == 100000 */
#define NET_UTIL_UNIT       1000
#define NET_UTIL_MAX        (100 * NET_UTIL_UNIT)

#define NET_HISTORY_SIZE    2048    /* Samples kept per adapter */
#define NET_GRID_CELL       12      /* Grid cell size, in pixels */
#define NET_LABEL_PADDING   4       /* Space between scale labels and plot */

#define NET_CLR_BACK        RGB(0, 0, 0)
#define NET_CLR_GRID        RGB(0, 128, 64)
#define NET_CLR_TOTAL       RGB(0, 255, 0)
#define NET_CLR_SENT        RGB(255, 0, 0)
#define NET_CLR_RECEIVED    RGB(255, 255, 0)
#define NET_CLR_TEXT        RGB(0, 255, 0)

typedef enum _NET_SERIES
{
    NET_SERIES_SENT,
    NET_SERIES_RECEIVED,
    NET_SERIES_TOTAL
} NET_SERIES;

typedef enum _NET_COLUMN
{
    NET_COLUMN_NAME,
    NET_COLUMN_UTILIZATION,
    NET_COLUMN_LINKSPEED,
    NET_COLUMN_STATE,
    NET_COLUMN_SENDRATE,
    NET_COLUMN_RECVRATE,
    NET_COLUMN_BYTESSENT,
    NET_COLUMN_BYTESRECV,
    NET_COLUMN_COUNT
} NET_COLUMN;

typedef struct _NET_COLUMN_INFO
{
    UINT idString;
    INT  cxWidth;
    INT  Format;
} NET_COLUMN_INFO;

static const NET_COLUMN_INFO NetColumns[NET_COLUMN_COUNT] =
{
    { IDS_NET_COL_ADAPTER,     150, LVCFMT_LEFT  },
    { IDS_NET_COL_UTILIZATION,  80, LVCFMT_RIGHT },
    { IDS_NET_COL_LINKSPEED,    70, LVCFMT_RIGHT },
    { IDS_NET_COL_STATE,        80, LVCFMT_LEFT  },
    { IDS_NET_COL_SENDRATE,     80, LVCFMT_RIGHT },
    { IDS_NET_COL_RECVRATE,     80, LVCFMT_RIGHT },
    { IDS_NET_COL_BYTESSENT,    80, LVCFMT_RIGHT },
    { IDS_NET_COL_BYTESRECV,    80, LVCFMT_RIGHT },
};

/* Available graph scales: 0.1%, 0.5%, 1%, 5%, 10%, 25%, 50%, 100% */
static const ULONG NetScales[] =
{
    100, 500, 1000, 5000, 10000, 25000, 50000, 100000
};

typedef struct _NET_ADAPTER
{
    DWORD     IfIndex;
    BOOL      Seen;
    BOOL      HasSample;

    DWORD     LastInOctets;
    DWORD     LastOutOctets;
    DWORD     LastTick;

    ULONGLONG TotalReceived;        /* 64-bit totals survive 32-bit counter wrap */
    ULONGLONG TotalSent;
    ULONGLONG ReceiveRate;          /* Bytes per second */
    ULONGLONG SendRate;

    DWORD     Speed;                /* Bits per second */
    DWORD     OperStatus;
    DWORD     AdminStatus;
    ULONG     Utilization;          /* 1/1000 % */

    WCHAR     szName[MAX_INTERFACE_NAME_LEN];

    UINT      HistoryHead;          /* Index of the newest sample */
    UINT      HistoryCount;
    ULONG     SentHistory[NET_HISTORY_SIZE];
    ULONG     RecvHistory[NET_HISTORY_SIZE];
} NET_ADAPTER, *PNET_ADAPTER;

HWND hNetworkPage;                  /* Networking Property Page */
HWND hNetworkPageListCtrl;          /* Network adapters list */
static HWND hNetworkHistoryFrame;   /* Graph group box */
static HWND hNetworkGraph;          /* Owner-drawn graph */

static PNET_ADAPTER *NetAdapters;
static UINT NetAdapterCount;
static UINT NetAdapterCapacity;
static DWORD NetSelectedIfIndex;
static BOOL NetHasSelection;
static BOOL NetRebuildingList;

static PMIB_IFTABLE NetIfTable;
static ULONG NetIfTableSize;

static UINT NetGridShift;

static HPEN   hNetPenGrid;
static HPEN   hNetPenTotal;
static HPEN   hNetPenSent;
static HPEN   hNetPenReceived;
static HBRUSH hNetBrushBack;

/* Layout captured from the dialog template, in pixels */
static INT  nNetMargin;
static INT  nNetListHeight;
static RECT rcNetGraphInset;

static WCHAR szNetDecimal[4] = L".";
static WCHAR szNetHistoryTitle[128];
static WCHAR szNetNoAdapters[128];
static WCHAR szNetRateFormat[32];
static WCHAR szNetStateConnected[64];
static WCHAR szNetStateDisconnected[64];
static WCHAR szNetStateConnecting[64];
static WCHAR szNetStateDisabled[64];

/* ------------------------------------------------------------------------ */
/* Formatting helpers                                                        */
/* ------------------------------------------------------------------------ */

/* Formats a value in 1/1000 of a percent, e.g. 1250 -> "1.25 %" */
static void
NetPage_FormatPercent(ULONG Value, UINT Decimals, BOOL TrimZeros, LPWSTR pszOut, SIZE_T cchOut)
{
    ULONG Whole = Value / NET_UTIL_UNIT;
    ULONG Frac = Value % NET_UTIL_UNIT;
    WCHAR szFrac[4];
    UINT i;

    if (Decimals > 3)
        Decimals = 3;

    /* Keep the requested number of decimal digits */
    for (i = 3; i > Decimals; i--)
        Frac /= 10;

    for (i = Decimals; i > 0; i--)
    {
        szFrac[i - 1] = L'0' + (WCHAR)(Frac % 10);
        Frac /= 10;
    }
    szFrac[Decimals] = UNICODE_NULL;

    if (TrimZeros)
    {
        for (i = Decimals; i > 0 && szFrac[i - 1] == L'0'; i--)
            szFrac[i - 1] = UNICODE_NULL;
    }

    if (szFrac[0] != UNICODE_NULL)
        StringCchPrintfW(pszOut, cchOut, L"%lu%s%s %%", Whole, szNetDecimal, szFrac);
    else
        StringCchPrintfW(pszOut, cchOut, L"%lu %%", Whole);
}

static void
NetPage_FormatLinkSpeed(DWORD Speed, LPWSTR pszOut, SIZE_T cchOut)
{
    static const LPCWSTR Units[] = { L"bps", L"Kbps", L"Mbps", L"Gbps" };
    ULONGLONG Divisor = 1;
    ULONGLONG Whole, Tenths;
    UINT Unit = 0;

    while (Unit < _countof(Units) - 1 && Speed >= Divisor * 1000)
    {
        Divisor *= 1000;
        Unit++;
    }

    Whole = Speed / Divisor;
    Tenths = ((Speed % Divisor) * 10) / Divisor;

    if (Tenths)
        StringCchPrintfW(pszOut, cchOut, L"%I64u%s%I64u %s", Whole, szNetDecimal, Tenths, Units[Unit]);
    else
        StringCchPrintfW(pszOut, cchOut, L"%I64u %s", Whole, Units[Unit]);
}

static void
NetPage_FormatBytes(ULONGLONG Bytes, LPWSTR pszOut, UINT cchOut)
{
    if (!StrFormatByteSizeW((LONGLONG)Bytes, pszOut, cchOut))
        StringCchPrintfW(pszOut, cchOut, L"%I64u", Bytes);
}

static void
NetPage_FormatRate(ULONGLONG BytesPerSec, LPWSTR pszOut, UINT cchOut)
{
    WCHAR szSize[64];

    NetPage_FormatBytes(BytesPerSec, szSize, _countof(szSize));
    StringCchPrintfW(pszOut, cchOut, szNetRateFormat, szSize);
}

static LPCWSTR
NetPage_GetStateText(PNET_ADAPTER Adapter)
{
    if (Adapter->AdminStatus == MIB_IF_ADMIN_STATUS_DOWN)
        return szNetStateDisabled;

    switch (Adapter->OperStatus)
    {
        case MIB_IF_OPER_STATUS_CONNECTED:
        case MIB_IF_OPER_STATUS_OPERATIONAL:
            return szNetStateConnected;

        case MIB_IF_OPER_STATUS_CONNECTING:
            return szNetStateConnecting;

        default:
            return szNetStateDisconnected;
    }
}

/* ------------------------------------------------------------------------ */
/* Adapter data                                                              */
/* ------------------------------------------------------------------------ */

static PNET_ADAPTER
NetPage_FindAdapter(DWORD IfIndex)
{
    UINT i;

    for (i = 0; i < NetAdapterCount; i++)
    {
        if (NetAdapters[i]->IfIndex == IfIndex)
            return NetAdapters[i];
    }
    return NULL;
}

static PNET_ADAPTER
NetPage_AddAdapter(DWORD IfIndex)
{
    PNET_ADAPTER Adapter;

    if (NetAdapterCount == NetAdapterCapacity)
    {
        UINT NewCapacity = NetAdapterCapacity ? NetAdapterCapacity * 2 : 8;
        PNET_ADAPTER *NewArray;

        if (NetAdapters)
            NewArray = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, NetAdapters, NewCapacity * sizeof(PNET_ADAPTER));
        else
            NewArray = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, NewCapacity * sizeof(PNET_ADAPTER));

        if (!NewArray)
            return NULL;

        NetAdapters = NewArray;
        NetAdapterCapacity = NewCapacity;
    }

    Adapter = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(NET_ADAPTER));
    if (!Adapter)
        return NULL;

    Adapter->IfIndex = IfIndex;
    NetAdapters[NetAdapterCount++] = Adapter;
    return Adapter;
}

static void
NetPage_FreeAdapters(void)
{
    UINT i;

    for (i = 0; i < NetAdapterCount; i++)
        HeapFree(GetProcessHeap(), 0, NetAdapters[i]);

    if (NetAdapters)
        HeapFree(GetProcessHeap(), 0, NetAdapters);

    NetAdapters = NULL;
    NetAdapterCount = NetAdapterCapacity = 0;
}

/* Converts bytes transferred during an interval to a share of the link capacity */
static ULONG
NetPage_CalcUtilization(ULONGLONG Bytes, DWORD IntervalMs, DWORD Speed)
{
    ULONGLONG Capacity, Value;

    if (!Speed || !IntervalMs)
        return 0;

    Capacity = (ULONGLONG)Speed * IntervalMs / 1000; /* bits per interval */
    if (!Capacity)
        return 0;

    Value = Bytes * 8 * NET_UTIL_MAX / Capacity;
    return (ULONG)min(Value, NET_UTIL_MAX);
}

/* Returns the number of bytes counted since the previous sample */
static DWORD
NetPage_CounterDelta(DWORD Current, DWORD Previous, DWORD IntervalMs, DWORD Speed)
{
    DWORD Delta = Current - Previous; /* Unsigned math handles 32-bit wrap */

    /*
     * If the counter went backwards and the wrapped difference is more than
     * the link could possibly carry, the counter was reset (for example,
     * the adapter was re-enabled), so count from zero instead.
     */
    if (Current < Previous && Speed)
    {
        ULONGLONG MaxBytes = ((ULONGLONG)Speed / 8) * (IntervalMs + 1000) / 1000 * 2;
        if (Delta > MaxBytes)
            Delta = Current;
    }
    return Delta;
}

static void
NetPage_SampleAdapter(PNET_ADAPTER Adapter, const MIB_IFROW *Row, DWORD Tick)
{
    ULONG Sent = 0, Received = 0;
    WCHAR szName[MAX_INTERFACE_NAME_LEN];
    int cchDescr, cchName = 0;

    /* Adapter name: the description is ANSI and may not be NUL-terminated */
    cchDescr = (int)min(Row->dwDescrLen, (DWORD)MAXLEN_IFDESCR);
    while (cchDescr > 0 && Row->bDescr[cchDescr - 1] == '\0')
        cchDescr--;

    if (cchDescr > 0)
        cchName = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)Row->bDescr, cchDescr,
                                      szName, _countof(szName) - 1);

    if (cchName > 0)
        szName[cchName] = UNICODE_NULL;
    else
        StringCchPrintfW(szName, _countof(szName), L"#%lu", Row->dwIndex);
    StringCchCopyW(Adapter->szName, _countof(Adapter->szName), szName);

    Adapter->Speed = Row->dwSpeed;
    Adapter->OperStatus = Row->dwOperStatus;
    Adapter->AdminStatus = Row->dwAdminStatus;

    if (!Adapter->HasSample)
    {
        Adapter->TotalReceived = Row->dwInOctets;
        Adapter->TotalSent = Row->dwOutOctets;
        Adapter->HasSample = TRUE;
    }
    else
    {
        DWORD Interval = Tick - Adapter->LastTick;
        DWORD InDelta = NetPage_CounterDelta(Row->dwInOctets, Adapter->LastInOctets, Interval, Adapter->Speed);
        DWORD OutDelta = NetPage_CounterDelta(Row->dwOutOctets, Adapter->LastOutOctets, Interval, Adapter->Speed);

        Adapter->TotalReceived += InDelta;
        Adapter->TotalSent += OutDelta;

        if (Interval)
        {
            Adapter->ReceiveRate = (ULONGLONG)InDelta * 1000 / Interval;
            Adapter->SendRate = (ULONGLONG)OutDelta * 1000 / Interval;
            Received = NetPage_CalcUtilization(InDelta, Interval, Adapter->Speed);
            Sent = NetPage_CalcUtilization(OutDelta, Interval, Adapter->Speed);
        }
    }

    Adapter->LastInOctets = Row->dwInOctets;
    Adapter->LastOutOctets = Row->dwOutOctets;
    Adapter->LastTick = Tick;
    Adapter->Utilization = min(Sent + Received, NET_UTIL_MAX);

    /* Push the sample into the history ring buffer */
    Adapter->HistoryHead = (Adapter->HistoryHead + 1) % NET_HISTORY_SIZE;
    Adapter->SentHistory[Adapter->HistoryHead] = Sent;
    Adapter->RecvHistory[Adapter->HistoryHead] = Received;
    if (Adapter->HistoryCount < NET_HISTORY_SIZE)
        Adapter->HistoryCount++;
}

static BOOL
NetPage_QueryIfTable(void)
{
    DWORD dwError;
    UINT Tries;

    for (Tries = 0; Tries < 3; Tries++)
    {
        ULONG Size = NetIfTableSize;

        dwError = GetIfTable(NetIfTable, &Size, TRUE);
        if (dwError == NO_ERROR)
            return TRUE;

        if (dwError != ERROR_INSUFFICIENT_BUFFER)
            return FALSE;

        /* The adapter list grew, allocate a bigger buffer */
        Size = max(Size, (ULONG)sizeof(MIB_IFTABLE));
        if (NetIfTable)
            HeapFree(GetProcessHeap(), 0, NetIfTable);

        NetIfTable = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Size);
        if (!NetIfTable)
        {
            NetIfTableSize = 0;
            return FALSE;
        }
        NetIfTableSize = Size;
    }
    return FALSE;
}

/* Updates the adapters list; returns TRUE if adapters were added or removed */
static BOOL
NetPage_UpdateAdapters(void)
{
    DWORD Tick = GetTickCount();
    BOOL bChanged = FALSE;
    UINT i;

    for (i = 0; i < NetAdapterCount; i++)
        NetAdapters[i]->Seen = FALSE;

    if (NetPage_QueryIfTable())
    {
        for (i = 0; i < NetIfTable->dwNumEntries; i++)
        {
            const MIB_IFROW *Row = &NetIfTable->table[i];
            PNET_ADAPTER Adapter;

            if (Row->dwType == MIB_IF_TYPE_LOOPBACK)
                continue;

            Adapter = NetPage_FindAdapter(Row->dwIndex);
            if (!Adapter)
            {
                Adapter = NetPage_AddAdapter(Row->dwIndex);
                if (!Adapter)
                    continue;
                bChanged = TRUE;
            }

            NetPage_SampleAdapter(Adapter, Row, Tick);
            Adapter->Seen = TRUE;
        }
    }

    /* Drop the adapters that went away */
    for (i = 0; i < NetAdapterCount; )
    {
        if (!NetAdapters[i]->Seen)
        {
            HeapFree(GetProcessHeap(), 0, NetAdapters[i]);
            NetAdapters[i] = NetAdapters[--NetAdapterCount];
            bChanged = TRUE;
        }
        else
        {
            i++;
        }
    }

    return bChanged;
}

static PNET_ADAPTER
NetPage_GetSelectedAdapter(void)
{
    if (!NetHasSelection)
        return NULL;
    return NetPage_FindAdapter(NetSelectedIfIndex);
}

/* ------------------------------------------------------------------------ */
/* Adapter list view                                                         */
/* ------------------------------------------------------------------------ */

static void
NetPage_SetItemText(int iItem, int iSubItem, LPCWSTR pszText)
{
    WCHAR szOld[260];

    /* Only touch the item if the text changed, to avoid flicker */
    szOld[0] = UNICODE_NULL;
    ListView_GetItemText(hNetworkPageListCtrl, iItem, iSubItem, szOld, _countof(szOld));
    if (wcscmp(szOld, pszText) != 0)
        ListView_SetItemText(hNetworkPageListCtrl, iItem, iSubItem, (LPWSTR)pszText);
}

static void
NetPage_UpdateListItem(int iItem, PNET_ADAPTER Adapter)
{
    WCHAR szText[128];

    NetPage_SetItemText(iItem, NET_COLUMN_NAME, Adapter->szName);

    NetPage_FormatPercent(Adapter->Utilization, 2, FALSE, szText, _countof(szText));
    NetPage_SetItemText(iItem, NET_COLUMN_UTILIZATION, szText);

    NetPage_FormatLinkSpeed(Adapter->Speed, szText, _countof(szText));
    NetPage_SetItemText(iItem, NET_COLUMN_LINKSPEED, szText);

    NetPage_SetItemText(iItem, NET_COLUMN_STATE, NetPage_GetStateText(Adapter));

    NetPage_FormatRate(Adapter->SendRate, szText, _countof(szText));
    NetPage_SetItemText(iItem, NET_COLUMN_SENDRATE, szText);

    NetPage_FormatRate(Adapter->ReceiveRate, szText, _countof(szText));
    NetPage_SetItemText(iItem, NET_COLUMN_RECVRATE, szText);

    NetPage_FormatBytes(Adapter->TotalSent, szText, _countof(szText));
    NetPage_SetItemText(iItem, NET_COLUMN_BYTESSENT, szText);

    NetPage_FormatBytes(Adapter->TotalReceived, szText, _countof(szText));
    NetPage_SetItemText(iItem, NET_COLUMN_BYTESRECV, szText);
}

static void
NetPage_UpdateFrameTitle(void)
{
    PNET_ADAPTER Adapter = NetPage_GetSelectedAdapter();

    SetWindowTextW(hNetworkHistoryFrame, Adapter ? Adapter->szName : szNetHistoryTitle);
}

static void
NetPage_RebuildList(void)
{
    LVITEMW item;
    int iSelect = -1;
    UINT i;

    NetRebuildingList = TRUE;
    SendMessageW(hNetworkPageListCtrl, WM_SETREDRAW, FALSE, 0);

    (void)ListView_DeleteAllItems(hNetworkPageListCtrl);

    for (i = 0; i < NetAdapterCount; i++)
    {
        ZeroMemory(&item, sizeof(item));
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = i;
        item.pszText = NetAdapters[i]->szName;
        item.lParam = (LPARAM)NetAdapters[i]->IfIndex;
        (void)ListView_InsertItem(hNetworkPageListCtrl, &item);

        NetPage_UpdateListItem(i, NetAdapters[i]);

        if (NetHasSelection && NetAdapters[i]->IfIndex == NetSelectedIfIndex)
            iSelect = i;
    }

    /* Keep the previous selection, or fall back to the first adapter */
    if (iSelect < 0 && NetAdapterCount > 0)
        iSelect = 0;

    if (iSelect >= 0)
    {
        NetSelectedIfIndex = NetAdapters[iSelect]->IfIndex;
        NetHasSelection = TRUE;
        ListView_SetItemState(hNetworkPageListCtrl, iSelect,
                              LVIS_SELECTED | LVIS_FOCUSED,
                              LVIS_SELECTED | LVIS_FOCUSED);
    }
    else
    {
        NetHasSelection = FALSE;
    }

    SendMessageW(hNetworkPageListCtrl, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hNetworkPageListCtrl, NULL, TRUE);
    NetRebuildingList = FALSE;

    NetPage_UpdateFrameTitle();
}

static void
NetPage_OnListItemChanged(LPNMLISTVIEW pnmv)
{
    if (NetRebuildingList)
        return;

    if ((pnmv->uChanged & LVIF_STATE) &&
        (pnmv->uNewState & LVIS_SELECTED) &&
        !(pnmv->uOldState & LVIS_SELECTED))
    {
        NetSelectedIfIndex = (DWORD)pnmv->lParam;
        NetHasSelection = TRUE;
        NetPage_UpdateFrameTitle();
        InvalidateRect(hNetworkGraph, NULL, FALSE);
    }
}

static void
NetPage_SetupColumns(void)
{
    LVCOLUMNW column;
    WCHAR szText[128];
    UINT i;

    ListView_SetExtendedListViewStyle(hNetworkPageListCtrl,
                                      LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);

    for (i = 0; i < NET_COLUMN_COUNT; i++)
    {
        LoadStringW(hInst, NetColumns[i].idString, szText, _countof(szText));

        ZeroMemory(&column, sizeof(column));
        column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT | LVCF_SUBITEM;
        column.fmt = (i == 0) ? LVCFMT_LEFT : NetColumns[i].Format;
        column.cx = NetColumns[i].cxWidth;
        column.pszText = szText;
        column.iSubItem = i;
        (void)ListView_InsertColumn(hNetworkPageListCtrl, i, &column);
    }
}

/* ------------------------------------------------------------------------ */
/* History graph                                                             */
/* ------------------------------------------------------------------------ */

static ULONG
NetPage_GetSample(PNET_ADAPTER Adapter, UINT Index, NET_SERIES Series)
{
    switch (Series)
    {
        case NET_SERIES_SENT:
            return Adapter->SentHistory[Index];
        case NET_SERIES_RECEIVED:
            return Adapter->RecvHistory[Index];
        default:
            return min(Adapter->SentHistory[Index] + Adapter->RecvHistory[Index], NET_UTIL_MAX);
    }
}

static UINT
NetPage_PrevIndex(UINT Index)
{
    return Index ? Index - 1 : NET_HISTORY_SIZE - 1;
}

static BOOL
NetPage_IsSeriesVisible(NET_SERIES Series)
{
    switch (Series)
    {
        case NET_SERIES_SENT:
            return TaskManagerSettings.NetShowBytesSent;
        case NET_SERIES_RECEIVED:
            return TaskManagerSettings.NetShowBytesReceived;
        default:
            return TaskManagerSettings.NetShowBytesTotal;
    }
}

/* Picks the smallest scale that fits every visible sample */
static ULONG
NetPage_ChooseScale(PNET_ADAPTER Adapter, UINT nPoints)
{
    ULONG MaxValue = 0;
    UINT Index = Adapter->HistoryHead;
    UINT i, s;

    nPoints = min(nPoints, Adapter->HistoryCount);

    for (i = 0; i < nPoints; i++)
    {
        for (s = NET_SERIES_SENT; s <= NET_SERIES_TOTAL; s++)
        {
            if (NetPage_IsSeriesVisible((NET_SERIES)s))
                MaxValue = max(MaxValue, NetPage_GetSample(Adapter, Index, (NET_SERIES)s));
        }
        Index = NetPage_PrevIndex(Index);
    }

    for (i = 0; i < _countof(NetScales); i++)
    {
        if (MaxValue <= NetScales[i])
            return NetScales[i];
    }
    return NET_UTIL_MAX;
}

static int
NetPage_ValueToY(ULONG Value, ULONG Scale, const RECT *prc)
{
    int Height = prc->bottom - prc->top - 1;

    if (Value > Scale)
        Value = Scale;
    return prc->bottom - 1 - (int)(((ULONGLONG)Value * Height) / Scale);
}

static void
NetPage_PlotSeries(HDC hdc, const RECT *prc, PNET_ADAPTER Adapter,
                   NET_SERIES Series, ULONG Scale, UINT nPoints, HPEN hPen)
{
    UINT Index = Adapter->HistoryHead;
    int x = prc->right - 1;
    UINT i;

    nPoints = min(nPoints, Adapter->HistoryCount);
    if (!nPoints)
        return;

    SelectObject(hdc, hPen);
    MoveToEx(hdc, x, NetPage_ValueToY(NetPage_GetSample(Adapter, Index, Series), Scale, prc), NULL);

    for (i = 1; i < nPoints; i++)
    {
        Index = NetPage_PrevIndex(Index);
        x -= PLOT_SHIFT;
        LineTo(hdc, x, NetPage_ValueToY(NetPage_GetSample(Adapter, Index, Series), Scale, prc));
    }
}

static void
NetPage_DrawGrid(HDC hdc, const RECT *prc)
{
    int p;

    SelectObject(hdc, hNetPenGrid);

    for (p = prc->bottom - 1; p >= prc->top; p -= NET_GRID_CELL)
    {
        MoveToEx(hdc, prc->left, p, NULL);
        LineTo(hdc, prc->right, p);
    }

    for (p = prc->right - 1 - (int)NetGridShift; p >= prc->left; p -= NET_GRID_CELL)
    {
        MoveToEx(hdc, p, prc->top, NULL);
        LineTo(hdc, p, prc->bottom);
    }
}

/* Returns the width needed by the widest possible scale label */
static int
NetPage_GetScaleLabelWidth(HDC hdc)
{
    WCHAR szText[32];
    SIZE size;
    int cxLabels = 0;
    UINT i;

    /* Constant width, so the plot doesn't jump when the scale changes */
    for (i = 0; i < _countof(NetScales); i++)
    {
        NetPage_FormatPercent(NetScales[i], 3, TRUE, szText, _countof(szText));
        if (GetTextExtentPoint32W(hdc, szText, (int)wcslen(szText), &size))
            cxLabels = max(cxLabels, size.cx);

        NetPage_FormatPercent(NetScales[i] / 2, 3, TRUE, szText, _countof(szText));
        if (GetTextExtentPoint32W(hdc, szText, (int)wcslen(szText), &size))
            cxLabels = max(cxLabels, size.cx);
    }
    return cxLabels;
}

/* Draws the scale labels (100%, 50% and 0% of the current scale) */
static void
NetPage_DrawScale(HDC hdc, const RECT *prc, int cxLabels, ULONG Scale)
{
    WCHAR szText[32];
    RECT rcText;

    SetTextColor(hdc, NET_CLR_TEXT);
    SetBkMode(hdc, TRANSPARENT);

    SetRect(&rcText, prc->left, prc->top, prc->left + cxLabels, prc->bottom);

    NetPage_FormatPercent(Scale, 3, TRUE, szText, _countof(szText));
    DrawTextW(hdc, szText, -1, &rcText, DT_RIGHT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);

    NetPage_FormatPercent(Scale / 2, 3, TRUE, szText, _countof(szText));
    DrawTextW(hdc, szText, -1, &rcText, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    NetPage_FormatPercent(0, 0, TRUE, szText, _countof(szText));
    DrawTextW(hdc, szText, -1, &rcText, DT_RIGHT | DT_BOTTOM | DT_SINGLELINE | DT_NOPREFIX);
}

static void
NetPage_DrawGraph(LPDRAWITEMSTRUCT pdis)
{
    PNET_ADAPTER Adapter;
    HDC hdcMem;
    HBITMAP hbmMem, hbmOld;
    HFONT hFont, hFontOld = NULL;
    RECT rc, rcPlot;
    int cx = pdis->rcItem.right - pdis->rcItem.left;
    int cy = pdis->rcItem.bottom - pdis->rcItem.top;

    if (cx <= 0 || cy <= 0)
        return;

    /* Double buffering to avoid flicker */
    hdcMem = CreateCompatibleDC(pdis->hDC);
    hbmMem = CreateCompatibleBitmap(pdis->hDC, cx, cy);
    if (!hdcMem || !hbmMem)
    {
        if (hbmMem) DeleteObject(hbmMem);
        if (hdcMem) DeleteDC(hdcMem);
        return;
    }
    hbmOld = SelectObject(hdcMem, hbmMem);

    hFont = (HFONT)SendMessageW(hNetworkPage, WM_GETFONT, 0, 0);
    if (hFont)
        hFontOld = SelectObject(hdcMem, hFont);

    SetRect(&rc, 0, 0, cx, cy);
    FillRect(hdcMem, &rc, hNetBrushBack);

    Adapter = NetPage_GetSelectedAdapter();
    if (!Adapter)
    {
        NetPage_DrawGrid(hdcMem, &rc);
        SetTextColor(hdcMem, NET_CLR_TEXT);
        SetBkMode(hdcMem, TRANSPARENT);
        DrawTextW(hdcMem, szNetNoAdapters, -1, &rc,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }
    else
    {
        RECT rcLabels;
        UINT nPoints;
        ULONG Scale;
        int cxLabels;

        rcPlot = rc;
        InflateRect(&rcPlot, -2, -2);

        rcLabels = rcPlot;
        cxLabels = 0;
        if (TaskManagerSettings.NetShowScale)
        {
            cxLabels = NetPage_GetScaleLabelWidth(hdcMem);
            rcPlot.left = min(rcPlot.left + cxLabels + NET_LABEL_PADDING, rcPlot.right - 1);
        }

        nPoints = (UINT)max(rcPlot.right - 1 - rcPlot.left, 0) / PLOT_SHIFT + 1;
        Scale = NetPage_ChooseScale(Adapter, nPoints);

        if (TaskManagerSettings.NetShowScale)
            NetPage_DrawScale(hdcMem, &rcLabels, cxLabels, Scale);

        NetPage_DrawGrid(hdcMem, &rcPlot);

        if (TaskManagerSettings.NetShowBytesTotal)
            NetPage_PlotSeries(hdcMem, &rcPlot, Adapter, NET_SERIES_TOTAL, Scale, nPoints, hNetPenTotal);
        if (TaskManagerSettings.NetShowBytesReceived)
            NetPage_PlotSeries(hdcMem, &rcPlot, Adapter, NET_SERIES_RECEIVED, Scale, nPoints, hNetPenReceived);
        if (TaskManagerSettings.NetShowBytesSent)
            NetPage_PlotSeries(hdcMem, &rcPlot, Adapter, NET_SERIES_SENT, Scale, nPoints, hNetPenSent);
    }

    BitBlt(pdis->hDC, pdis->rcItem.left, pdis->rcItem.top, cx, cy, hdcMem, 0, 0, SRCCOPY);

    if (hFontOld)
        SelectObject(hdcMem, hFontOld);
    SelectObject(hdcMem, hbmOld);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
}

/* ------------------------------------------------------------------------ */
/* Page layout                                                               */
/* ------------------------------------------------------------------------ */

static void
NetPage_GetChildRect(HWND hDlg, HWND hCtrl, LPRECT prc)
{
    GetWindowRect(hCtrl, prc);
    MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)prc, sizeof(RECT) / sizeof(POINT));
}

static void
NetPage_CaptureLayout(HWND hDlg)
{
    RECT rcFrame, rcGraph, rcList;

    NetPage_GetChildRect(hDlg, hNetworkHistoryFrame, &rcFrame);
    NetPage_GetChildRect(hDlg, hNetworkGraph, &rcGraph);
    NetPage_GetChildRect(hDlg, hNetworkPageListCtrl, &rcList);

    nNetMargin = rcFrame.left;
    nNetListHeight = rcList.bottom - rcList.top;

    rcNetGraphInset.left = rcGraph.left - rcFrame.left;
    rcNetGraphInset.top = rcGraph.top - rcFrame.top;
    rcNetGraphInset.right = rcFrame.right - rcGraph.right;
    rcNetGraphInset.bottom = rcFrame.bottom - rcGraph.bottom;
}

static void
NetPage_Layout(int cx, int cy)
{
    HDWP hdwp;
    int cxInner = max(cx - 2 * nNetMargin, 1);
    int cyFrame = max(cy - nNetListHeight - 3 * nNetMargin,
                      rcNetGraphInset.top + rcNetGraphInset.bottom + 1);
    int yList = nNetMargin * 2 + cyFrame;

    hdwp = BeginDeferWindowPos(3);
    if (!hdwp)
        return;

    hdwp = DeferWindowPos(hdwp, hNetworkHistoryFrame, NULL,
                          nNetMargin, nNetMargin, cxInner, cyFrame,
                          SWP_NOZORDER | SWP_NOACTIVATE);
    if (hdwp)
        hdwp = DeferWindowPos(hdwp, hNetworkGraph, NULL,
                              nNetMargin + rcNetGraphInset.left,
                              nNetMargin + rcNetGraphInset.top,
                              max(cxInner - rcNetGraphInset.left - rcNetGraphInset.right, 1),
                              max(cyFrame - rcNetGraphInset.top - rcNetGraphInset.bottom, 1),
                              SWP_NOZORDER | SWP_NOACTIVATE);
    if (hdwp)
        hdwp = DeferWindowPos(hdwp, hNetworkPageListCtrl, NULL,
                              nNetMargin, yList, cxInner, nNetListHeight,
                              SWP_NOZORDER | SWP_NOACTIVATE);
    if (hdwp)
        EndDeferWindowPos(hdwp);

    InvalidateRect(hNetworkHistoryFrame, NULL, TRUE);
    InvalidateRect(hNetworkGraph, NULL, FALSE);
}

/* ------------------------------------------------------------------------ */
/* Page procedure and public interface                                       */
/* ------------------------------------------------------------------------ */

static void
NetPage_LoadStrings(void)
{
    if (!GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, szNetDecimal, _countof(szNetDecimal)))
        StringCchCopyW(szNetDecimal, _countof(szNetDecimal), L".");

    LoadStringW(hInst, IDS_NET_HISTORY, szNetHistoryTitle, _countof(szNetHistoryTitle));
    LoadStringW(hInst, IDS_NET_NOADAPTERS, szNetNoAdapters, _countof(szNetNoAdapters));
    LoadStringW(hInst, IDS_NET_STATE_CONNECTED, szNetStateConnected, _countof(szNetStateConnected));
    LoadStringW(hInst, IDS_NET_STATE_DISCONNECTED, szNetStateDisconnected, _countof(szNetStateDisconnected));
    LoadStringW(hInst, IDS_NET_STATE_CONNECTING, szNetStateConnecting, _countof(szNetStateConnecting));
    LoadStringW(hInst, IDS_NET_STATE_DISABLED, szNetStateDisabled, _countof(szNetStateDisabled));

    if (!LoadStringW(hInst, IDS_NET_RATE_FORMAT, szNetRateFormat, _countof(szNetRateFormat)))
        StringCchCopyW(szNetRateFormat, _countof(szNetRateFormat), L"%s/s");
}

static BOOL
NetPage_CreateGdiObjects(void)
{
    hNetPenGrid = CreatePen(PS_SOLID, 0, NET_CLR_GRID);
    hNetPenTotal = CreatePen(PS_SOLID, 0, NET_CLR_TOTAL);
    hNetPenSent = CreatePen(PS_SOLID, 0, NET_CLR_SENT);
    hNetPenReceived = CreatePen(PS_SOLID, 0, NET_CLR_RECEIVED);
    hNetBrushBack = CreateSolidBrush(NET_CLR_BACK);

    return hNetPenGrid && hNetPenTotal && hNetPenSent && hNetPenReceived && hNetBrushBack;
}

static void
NetPage_DeleteGdiObjects(void)
{
    if (hNetPenGrid) DeleteObject(hNetPenGrid);
    if (hNetPenTotal) DeleteObject(hNetPenTotal);
    if (hNetPenSent) DeleteObject(hNetPenSent);
    if (hNetPenReceived) DeleteObject(hNetPenReceived);
    if (hNetBrushBack) DeleteObject(hNetBrushBack);

    hNetPenGrid = hNetPenTotal = hNetPenSent = hNetPenReceived = NULL;
    hNetBrushBack = NULL;
}

INT_PTR CALLBACK
NetworkPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
        {
            hNetworkPage = hDlg;

            /* Update window position */
            SetWindowPos(hDlg, NULL, 15, 30, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);

            hNetworkHistoryFrame = GetDlgItem(hDlg, IDC_NETWORK_HISTORY_FRAME);
            hNetworkGraph = GetDlgItem(hDlg, IDC_NETWORK_GRAPH);
            hNetworkPageListCtrl = GetDlgItem(hDlg, IDC_NETWORK_ADAPTERS);

            NetPage_LoadStrings();
            if (!NetPage_CreateGdiObjects())
            {
                NetPage_DeleteGdiObjects();
                return FALSE;
            }

            NetPage_CaptureLayout(hDlg);
            NetPage_SetupColumns();

            /* Take the first sample so the next refresh has something to compare with */
            NetPage_UpdateAdapters();
            NetPage_RebuildList();
            return TRUE;
        }

        case WM_DESTROY:
            NetPage_FreeAdapters();
            NetPage_DeleteGdiObjects();
            if (NetIfTable)
            {
                HeapFree(GetProcessHeap(), 0, NetIfTable);
                NetIfTable = NULL;
                NetIfTableSize = 0;
            }
            break;

        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED)
                NetPage_Layout(LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;

            if (pdis->CtlID == IDC_NETWORK_GRAPH)
            {
                NetPage_DrawGraph(pdis);
                SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, TRUE);
                return TRUE;
            }
            break;
        }

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            if (pnmh->hwndFrom == hNetworkPageListCtrl && pnmh->code == LVN_ITEMCHANGED)
                NetPage_OnListItemChanged((LPNMLISTVIEW)lParam);
            break;
        }
    }

    return 0;
}

void RefreshNetworkPage(void)
{
    UINT i;

    if (!hNetworkPage)
        return;

    /* Keep collecting data even while hidden, so the history is complete */
    if (NetPage_UpdateAdapters())
    {
        NetPage_RebuildList();
    }
    else
    {
        for (i = 0; i < NetAdapterCount; i++)
            NetPage_UpdateListItem(i, NetAdapters[i]);
        NetPage_UpdateFrameTitle();
    }

    NetGridShift = (NetGridShift + PLOT_SHIFT) % NET_GRID_CELL;

    if (IsWindowVisible(hNetworkPage))
        InvalidateRect(hNetworkGraph, NULL, FALSE);
}

static void
NetPage_UpdateViewMenuChecks(HMENU hViewMenu)
{
    CheckMenuItem(hViewMenu, ID_VIEW_NETHISTORY_SENT, MF_BYCOMMAND |
                  (TaskManagerSettings.NetShowBytesSent ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hViewMenu, ID_VIEW_NETHISTORY_RECEIVED, MF_BYCOMMAND |
                  (TaskManagerSettings.NetShowBytesReceived ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hViewMenu, ID_VIEW_NETHISTORY_TOTAL, MF_BYCOMMAND |
                  (TaskManagerSettings.NetShowBytesTotal ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hViewMenu, ID_VIEW_NET_SHOWSCALE, MF_BYCOMMAND |
                  (TaskManagerSettings.NetShowScale ? MF_CHECKED : MF_UNCHECKED));
}

void NetworkPage_AppendViewMenu(HMENU hViewMenu)
{
    HMENU hSubMenu;
    WCHAR szTemp[256];

    hSubMenu = CreatePopupMenu();
    if (hSubMenu)
    {
        LoadStringW(hInst, IDS_MENU_BYTESSENT, szTemp, _countof(szTemp));
        AppendMenuW(hSubMenu, MF_STRING, ID_VIEW_NETHISTORY_SENT, szTemp);

        LoadStringW(hInst, IDS_MENU_BYTESRECEIVED, szTemp, _countof(szTemp));
        AppendMenuW(hSubMenu, MF_STRING, ID_VIEW_NETHISTORY_RECEIVED, szTemp);

        LoadStringW(hInst, IDS_MENU_BYTESTOTAL, szTemp, _countof(szTemp));
        AppendMenuW(hSubMenu, MF_STRING, ID_VIEW_NETHISTORY_TOTAL, szTemp);

        LoadStringW(hInst, IDS_MENU_NETHISTORY, szTemp, _countof(szTemp));
        AppendMenuW(hViewMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, szTemp);
    }

    LoadStringW(hInst, IDS_MENU_SHOWSCALE, szTemp, _countof(szTemp));
    AppendMenuW(hViewMenu, MF_STRING, ID_VIEW_NET_SHOWSCALE, szTemp);

    NetPage_UpdateViewMenuChecks(hViewMenu);
}

void NetworkPage_OnViewHistoryOption(UINT idCmd)
{
    HMENU hViewMenu = GetSubMenu(GetMenu(hMainWnd), 2);

    switch (idCmd)
    {
        case ID_VIEW_NETHISTORY_SENT:
            TaskManagerSettings.NetShowBytesSent = !TaskManagerSettings.NetShowBytesSent;
            break;
        case ID_VIEW_NETHISTORY_RECEIVED:
            TaskManagerSettings.NetShowBytesReceived = !TaskManagerSettings.NetShowBytesReceived;
            break;
        case ID_VIEW_NETHISTORY_TOTAL:
            TaskManagerSettings.NetShowBytesTotal = !TaskManagerSettings.NetShowBytesTotal;
            break;
        case ID_VIEW_NET_SHOWSCALE:
            TaskManagerSettings.NetShowScale = !TaskManagerSettings.NetShowScale;
            break;
        default:
            return;
    }

    if (hViewMenu)
        NetPage_UpdateViewMenuChecks(hViewMenu);

    InvalidateRect(hNetworkGraph, NULL, FALSE);
}
