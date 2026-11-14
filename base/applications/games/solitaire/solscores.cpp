/* Per-user high scores and the persistent Vegas balance. */
#include "solitaire.h"
#include "resource.h"
#include <commctrl.h>
#include <winreg.h>
#include <winnls.h>

static const WCHAR scoreKey[] = L"Software\\ReactOS\\Solitaire";
static const int boardCount = 8, entryCount = 10;
static const LONG scoreBound = 100000000;
struct ScoreEntry
{
    LONG score;
    DWORD seconds, won;
    FILETIME round, finished;
    WCHAR name[32];
};
struct ScoreBoards
{
    DWORD version, count[boardCount];
    ScoreEntry entries[boardCount][entryCount];
};
static DWORD roundOptions;
static LONG roundBase;
static FILETIME roundId;
static bool roundReady, roundRecorded, namePending;
static FILETIME nameRound;
static HWND nameEdit;

bool IsScoreNameInput(void)
{
    return nameEdit != NULL && GetFocus() == nameEdit;
}

static int ScoreMode(DWORD options)
{
    DWORD mode = options & (OPTION_SCORE_STD | OPTION_SCORE_VEGAS);
    return mode == OPTION_SCORE_STD ? SCORE_STD : mode == OPTION_SCORE_VEGAS ? SCORE_VEGAS : SCORE_NONE;
}

static int BoardIndex(DWORD options)
{
    return (ScoreMode(options) == SCORE_VEGAS ? 4 : 0) +
           ((options & OPTION_THREE_CARDS) ? 2 : 0) + ((options & OPTION_SHOW_TIME) ? 1 : 0);
}

static bool SameRound(const FILETIME &a, const FILETIME &b)
{
    return a.dwLowDateTime == b.dwLowDateTime && a.dwHighDateTime == b.dwHighDateTime;
}

static bool Better(const ScoreEntry &a, const ScoreEntry &b, bool timed)
{
    return a.score > b.score || (a.score == b.score && timed && a.seconds < b.seconds);
}

static void ReadBoards(ScoreBoards &boards)
{
    ZeroMemory(&boards, sizeof(boards));
    boards.version = 2;
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, scoreKey, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) return;
    struct OldEntry { LONG score; DWORD seconds, won; FILETIME round, finished; };
    struct OldBoards { DWORD version, count[boardCount]; OldEntry entries[boardCount][entryCount]; };
    ScoreBoards stored = {};
    DWORD type, size = sizeof(stored);
    LONG result = RegQueryValueExW(key, L"HighScores", NULL, &type, (BYTE *)&stored, &size);
    if (result == ERROR_SUCCESS && type == REG_BINARY &&
        stored.version == 1 && size == sizeof(OldBoards))
    {
        OldBoards old = {};
        size = sizeof(old);
        result = RegQueryValueExW(key, L"HighScores", NULL, &type, (BYTE *)&old, &size);
        ZeroMemory(&stored, sizeof(stored));
        stored.version = 2;
        for (int b = 0; b < boardCount; ++b)
        {
            stored.count[b] = old.count[b];
            for (int i = 0; i < entryCount; ++i)
                CopyMemory(&stored.entries[b][i], &old.entries[b][i], sizeof(OldEntry));
        }
        size = sizeof(stored);
    }
    RegCloseKey(key);
    if (result != ERROR_SUCCESS || type != REG_BINARY || size != sizeof(stored) || stored.version != 2) return;
    for (int board = 0; board < boardCount; ++board)
    {
        if (stored.count[board] > entryCount) return;
        for (DWORD i = 0; i < stored.count[board]; ++i)
        {
            const ScoreEntry &entry = stored.entries[board][i];
            SYSTEMTIME date;
            if (entry.name[ARRAYSIZE(entry.name) - 1] != 0 ||
                entry.score < (board < 4 ? 0 : -52) || entry.score > (board < 4 ? scoreBound : 208) ||
                entry.seconds > 31536000 || entry.won > 1 || (board < 4 && !entry.won) ||
                (!entry.round.dwLowDateTime && !entry.round.dwHighDateTime) ||
                !FileTimeToSystemTime(&entry.finished, &date)) return;
            if (i && Better(entry, stored.entries[board][i - 1], (board & 1) != 0)) return;
        }
    }
    boards = stored;
}

static void ReportScoreError(void)
{
    WCHAR text[256];
    LoadStringW(GetModuleHandle(NULL), IDS_HOF_SAVE_ERROR, text, ARRAYSIZE(text));
    MessageBoxW(hwndMain, text, szAppName, MB_OK | MB_ICONERROR);
}

static bool WriteBalance(DWORD options, LONG balance)
{
    if (ScoreMode(options) != SCORE_VEGAS || !(options & OPTION_KEEP_SCORE)) return true;
    HKEY key;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, scoreKey, 0, NULL, 0, KEY_SET_VALUE,
                        NULL, &key, NULL) != ERROR_SUCCESS) return false;
    LONG result = RegSetValueExW(key, (options & OPTION_THREE_CARDS) ? L"VegasBalance3" : L"VegasBalance1",
                                 0, REG_DWORD, (const BYTE *)&balance, sizeof(balance));
    RegCloseKey(key);
    return result == ERROR_SUCCESS;
}

bool ReadVegasBalance(DWORD options, LONG *balance)
{
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, scoreKey, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) return false;
    LONG value;
    DWORD type, size = sizeof(value);
    LONG result = RegQueryValueExW(key, (options & OPTION_THREE_CARDS) ? L"VegasBalance3" : L"VegasBalance1",
                                   NULL, &type, (BYTE *)&value, &size);
    RegCloseKey(key);
    if (result != ERROR_SUCCESS || type != REG_DWORD || size != sizeof(value) ||
        value < -scoreBound || value > scoreBound) return false;
    *balance = value;
    return true;
}

void SaveScoreBalance(void)
{
    WriteBalance(dwOptions, lScore);
}

void StartScoreRound(LONG base)
{
    namePending = false;
    roundOptions = dwOptions;
    roundBase = base;
    FILETIME now;
    GetSystemTimeAsFileTime(&now);
    if (CompareFileTime(&now, &roundId) <= 0)
    {
        now = roundId;
        if (++now.dwLowDateTime == 0) ++now.dwHighDateTime;
    }
    roundId = now;
    roundReady = true;
    roundRecorded = false;
}

DWORD ScoreRoundOptions(void)
{
    return roundReady ? roundOptions : 0;
}

void GetScoreRound(LONG *base, FILETIME *id)
{
    *base = roundBase;
    *id = roundId;
}

void RestoreScoreRound(LONG base, const FILETIME &id)
{
    roundOptions = dwOptions;
    roundBase = base;
    roundId = id;
    roundReady = true;
    roundRecorded = false;
}

void FinishScoreRound(bool won)
{
    if (!roundReady || roundRecorded) return;
    roundRecorded = true;
    bool balanceSaved = WriteBalance(roundOptions, lScore);
    int mode = ScoreMode(roundOptions);
    if (mode == SCORE_NONE || (!won && (mode == SCORE_STD || !fGameStarted)))
    {
        if (!balanceSaved) ReportScoreError();
        return;
    }
    ScoreEntry entry = {};
    // A cumulative Vegas bankroll must never compete with a single-round score.
    LONGLONG score = mode == SCORE_VEGAS ? (LONGLONG)lScore - roundBase : lScore;
    if (score < (mode == SCORE_VEGAS ? -52 : 0) || score > (mode == SCORE_VEGAS ? 208 : scoreBound)) return;
    entry.score = (LONG)score;
    entry.seconds = (roundOptions & OPTION_SHOW_TIME) ? dwTime : 0;
    entry.won = won;
    entry.round = roundId;
    GetSystemTimeAsFileTime(&entry.finished);
    ScoreBoards boards;
    ReadBoards(boards);
    for (int b = 0; b < boardCount; ++b)
        for (DWORD i = 0; i < boards.count[b]; ++i)
            if (SameRound(boards.entries[b][i].round, roundId)) return;
    int board = BoardIndex(roundOptions);
    DWORD position = 0;
    while (position < boards.count[board] && !Better(entry, boards.entries[board][position], (board & 1) != 0)) ++position;
    if (position == entryCount) return;
    DWORD count = min(boards.count[board] + 1, entryCount);
    for (DWORD i = count - 1; i > position; --i) boards.entries[board][i] = boards.entries[board][i - 1];
    boards.entries[board][position] = entry;
    boards.count[board] = count;
    HKEY key;
    LONG result = RegCreateKeyExW(HKEY_CURRENT_USER, scoreKey, 0, NULL, 0, KEY_SET_VALUE, NULL, &key, NULL);
    if (result == ERROR_SUCCESS)
    {
        result = RegSetValueExW(key, L"HighScores", 0, REG_BINARY, (const BYTE *)&boards, sizeof(boards));
        RegCloseKey(key);
    }
    if (result == ERROR_SUCCESS)
    {
        namePending = true;
        nameRound = entry.round;
    }
    if (result != ERROR_SUCCESS || !balanceSaved) ReportScoreError();
}

struct NameDialog
{
    WCHAR name[32];
    LONG score;
    DWORD rank;
};

static INT_PTR CALLBACK NameDlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
    NameDialog *data = (NameDialog *)GetWindowLongPtr(dlg, DWLP_USER);
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            data = (NameDialog *)lp;
            SetWindowLongPtr(dlg, DWLP_USER, lp);
            WCHAR format[128], text[256];
            LoadStringW(GetModuleHandle(NULL), IDS_HOF_PLACEMENT, format, ARRAYSIZE(format));
            wsprintfW(text, format, data->rank, data->score);
            SetDlgItemTextW(dlg, IDC_HOF_PLACEMENT, text);
            nameEdit = GetDlgItem(dlg, IDC_HOF_NAME);
            SendMessage(nameEdit, EM_SETLIMITTEXT, ARRAYSIZE(data->name) - 1, 0);
            SetWindowTextW(nameEdit, data->name);
            SetFocus(nameEdit);
            SendMessage(nameEdit, EM_SETSEL, 0, -1);
            return FALSE;
        }
        case WM_CLOSE:
            SendMessage(dlg, WM_COMMAND, IDCANCEL, 0);
            return TRUE;
        case WM_COMMAND:
            if (LOWORD(wp) == IDOK || LOWORD(wp) == IDCANCEL)
            {
                GetDlgItemTextW(dlg, IDC_HOF_NAME, data->name, ARRAYSIZE(data->name));
                EndDialog(dlg, LOWORD(wp));
                return TRUE;
            }
            break;
        case WM_DESTROY:
            nameEdit = NULL;
            break;
    }
    return FALSE;
}

// IDIGNORE means no qualifying result; otherwise the dialog replaces the deal prompt.
int PromptScoreName(void)
{
    if (!namePending) return IDIGNORE;
    namePending = false;
    ScoreBoards boards;
    ReadBoards(boards);
    ScoreEntry *entry = NULL;
    NameDialog data = {};
    for (int b = 0; b < boardCount; ++b)
        for (DWORD i = 0; i < boards.count[b]; ++i)
            if (SameRound(boards.entries[b][i].round, nameRound))
            {
                entry = &boards.entries[b][i];
                data.rank = i + 1;
                data.score = entry->score;
            }
    if (!entry) return IDIGNORE;
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, scoreKey, 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
    {
        DWORD type, size = sizeof(data.name);
        if (RegQueryValueExW(key, L"LastPlayerName", NULL, &type, (BYTE *)data.name, &size) != ERROR_SUCCESS ||
            type != REG_SZ || size < sizeof(WCHAR) || size % sizeof(WCHAR)) data.name[0] = 0;
        data.name[ARRAYSIZE(data.name) - 1] = 0;
        RegCloseKey(key);
    }
    fGameStarted = false;
    INT_PTR choice = DialogBoxParamW(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDD_SCORENAME),
                                    hwndMain, NameDlgProc, (LPARAM)&data);
    if (!IsWindow(hwndMain)) return IDCANCEL;
    if (choice != IDOK && choice != IDCANCEL) return IDIGNORE;
    lstrcpynW(entry->name, data.name, ARRAYSIZE(entry->name));
    LONG result = RegCreateKeyExW(HKEY_CURRENT_USER, scoreKey, 0, NULL, 0, KEY_SET_VALUE, NULL, &key, NULL);
    if (result == ERROR_SUCCESS)
    {
        result = RegSetValueExW(key, L"HighScores", 0, REG_BINARY, (BYTE *)&boards, sizeof(boards));
        if (result == ERROR_SUCCESS)
            result = RegSetValueExW(key, L"LastPlayerName", 0, REG_SZ, (BYTE *)data.name,
                                   (lstrlenW(data.name) + 1) * sizeof(WCHAR));
        // This round is finished, even when its Vegas board is not cleared yet.
        if (choice == IDCANCEL) RegDeleteValueW(key, L"SavedGame");
        RegCloseKey(key);
    }
    if (result != ERROR_SUCCESS) ReportScoreError();
    if (choice == IDCANCEL) DestroyWindow(hwndMain);
    return (int)choice;
}

static void FillScores(HWND dlg)
{
    DWORD options = SendDlgItemMessage(dlg, IDC_HOF_MODE, CB_GETCURSEL, 0, 0) == 1 ? OPTION_SCORE_VEGAS : OPTION_SCORE_STD;
    if (SendDlgItemMessage(dlg, IDC_HOF_DRAW, CB_GETCURSEL, 0, 0) == 1) options |= OPTION_THREE_CARDS;
    if (IsDlgButtonChecked(dlg, IDC_HOF_TIMED) == BST_CHECKED) options |= OPTION_SHOW_TIME;
    ScoreBoards boards;
    ReadBoards(boards);
    int board = BoardIndex(options);
    HWND list = GetDlgItem(dlg, IDC_HOF_LIST);
    ListView_DeleteAllItems(list);
    for (DWORD i = 0; i < boards.count[board]; ++i)
    {
        const ScoreEntry &entry = boards.entries[board][i];
        WCHAR text[128];
        wsprintfW(text, L"%lu", i + 1);
        LVITEMW item = {};
        item.mask = LVIF_TEXT; item.iItem = i; item.pszText = text;
        ListView_InsertItem(list, &item);
        wsprintfW(text, L"%ld", entry.score);
        ListView_SetItemText(list, i, 1, text);
        if (options & OPTION_SHOW_TIME) wsprintfW(text, L"%lu:%02lu", entry.seconds / 60, entry.seconds % 60);
        else lstrcpyW(text, L"-");
        ListView_SetItemText(list, i, 2, text);
        FILETIME local;
        SYSTEMTIME date;
        text[0] = 0;
        if (FileTimeToLocalFileTime(&entry.finished, &local) && FileTimeToSystemTime(&local, &date))
            GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &date, NULL, text, ARRAYSIZE(text));
        ListView_SetItemText(list, i, 3, text);
        LoadStringW(GetModuleHandle(NULL), entry.won ? IDS_HOF_WON : IDS_HOF_ENDED, text, ARRAYSIZE(text));
        ListView_SetItemText(list, i, 4, text);
        ListView_SetItemText(list, i, 5, const_cast<LPWSTR>(entry.name[0] ? entry.name : L"—"));
    }
}

static INT_PTR CALLBACK ScoresDlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            const int ids[] = {IDS_HOF_STANDARD, IDS_HOF_VEGAS, IDS_HOF_ONE, IDS_HOF_THREE};
            WCHAR text[128];
            for (int i = 0; i < 4; ++i)
            {
                LoadStringW(GetModuleHandle(NULL), ids[i], text, ARRAYSIZE(text));
                SendDlgItemMessageW(dlg, i < 2 ? IDC_HOF_MODE : IDC_HOF_DRAW, CB_ADDSTRING, 0, (LPARAM)text);
            }
            SendDlgItemMessage(dlg, IDC_HOF_MODE, CB_SETCURSEL, GetScoreMode() == SCORE_VEGAS ? 1 : 0, 0);
            SendDlgItemMessage(dlg, IDC_HOF_DRAW, CB_SETCURSEL, (dwOptions & OPTION_THREE_CARDS) ? 1 : 0, 0);
            CheckDlgButton(dlg, IDC_HOF_TIMED, (dwOptions & OPTION_SHOW_TIME) ? BST_CHECKED : BST_UNCHECKED);
            const int headers[] = {IDS_HOF_RANK, IDS_HOF_SCORE, IDS_HOF_TIME, IDS_HOF_DATE, IDS_HOF_RESULT, IDS_HOF_NAME};
            const int widths[] = {25, 50, 45, 65, 65, 140};
            HWND list = GetDlgItem(dlg, IDC_HOF_LIST);
            ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
            for (int i = 0; i < 6; ++i)
            {
                LoadStringW(GetModuleHandle(NULL), headers[i], text, ARRAYSIZE(text));
                RECT units = {0, 0, widths[i], 0};
                MapDialogRect(dlg, &units);
                LVCOLUMNW column = {};
                column.mask = LVCF_TEXT | LVCF_WIDTH;
                column.pszText = text; column.cx = units.right;
                ListView_InsertColumn(list, i, &column);
            }
            FillScores(dlg);
            return TRUE;
        }
        case WM_COMMAND:
            switch (LOWORD(wp))
            {
                case IDC_HOF_MODE:
                case IDC_HOF_DRAW:
                    if (HIWORD(wp) == CBN_SELCHANGE) FillScores(dlg);
                    return TRUE;
                case IDC_HOF_TIMED:
                    FillScores(dlg);
                    return TRUE;
                case IDOK:
                case IDCANCEL:
                    EndDialog(dlg, 0);
                    return TRUE;
            }
    }
    return FALSE;
}

void ShowHighScores(HWND owner)
{
    DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_HIGHSCORES), owner, ScoresDlgProc);
}
