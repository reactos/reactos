/*
 *  ReactOS Task Manager
 *
 *  dbgchnl.c
 *
 *  Copyright (C) 2003 - 2004 Eric Pouech
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <precomp.h>

/* TODO:
 *      - the dialog box could be non modal
 *      - in that case,
 *              + could refresh channels from time to time
 *      - get a better UI (replace the 'x' by real tick boxes in list view)
 *      - implement a real solution around the get_symbol hack
 *      - enhance visual feedback: the list is large, and it's hard to get the
 *        right line when clicking on rightmost column (trace for example)
 *      - get rid of printfs (error reporting) and use real message boxes
 *      - include the column width settings in the full column management scheme
 */

BOOL DebugChannelsAreSupported(void)
{
#ifdef WINE
    return TRUE;
#endif
    return FALSE;
}

static DWORD    get_selected_pid(void)
{
    LVITEM  lvitem;
    ULONG   Index;
    DWORD   dwProcessId;

    for (Index=0; Index<(ULONG)ListView_GetItemCount(hProcessPageListCtrl); Index++)
    {
        memset(&lvitem, 0, sizeof(LVITEM));

        lvitem.mask = LVIF_STATE;
        lvitem.stateMask = LVIS_SELECTED;
        lvitem.iItem = Index;

        (void)ListView_GetItem(hProcessPageListCtrl, &lvitem);

        if (lvitem.state & LVIS_SELECTED)
            break;
    }

    dwProcessId = PerfDataGetProcessId(Index);

    if ((ListView_GetSelectedCount(hProcessPageListCtrl) != 1) || (dwProcessId == 0))
        return 0;
    return dwProcessId;
}

static int     list_channel_CB(HANDLE hProcess, void* addr, WCHAR* buffer, void* user)
{
    int     j;
    WCHAR   val[2];
    LVITEM  lvi;
    int     index;
    HWND    hChannelLV = (HWND)user;

    memset(&lvi, 0, sizeof(lvi));

    lvi.mask = LVIF_TEXT;
    lvi.pszText = buffer + 1;

    index = ListView_InsertItem(hChannelLV, &lvi);
    if (index == -1) return 0;

    val[1] = L'\0';
    for (j = 0; j < 4; j++)
    {
        val[0] = (buffer[0] & (1 << j)) ? L'x' : L' ';
        ListView_SetItemText(hChannelLV, index, j + 1, val);
    }
    return 1;
}

struct cce_user
{
    LPCTSTR   name;           /* channel to look for */
    unsigned  value, mask;    /* how to change channel */
    unsigned  done;           /* number of successful changes */
    unsigned  notdone;        /* number of unsuccessful changes */
};

/******************************************************************
 *		change_channel_CB
 *
 * Callback used for changing a given channel attributes
 */
static int change_channel_CB(HANDLE hProcess, void* addr, WCHAR* buffer, void* pmt)
{
    struct cce_user*  user = (struct cce_user*)pmt;

    if (!user->name || !wcscmp(buffer + 1, user->name))
    {
        buffer[0] = (buffer[0] & ~user->mask) | (user->value & user->mask);
        if (WriteProcessMemory(hProcess, addr, buffer, 1, NULL))
            user->done++;
        else
            user->notdone++;
    }
    return 1;
}

#ifdef WINE
/******************************************************************
 *		get_symbol
 *
 * Here it gets ugly :-(
 * This is quick hack to get the address of first_dll in a running process
 * We make the following assumptions:
 *      - libwine (lib) is loaded in all processes at the same address (or
 *        at least at the same address at this process)
 *      - we load the same libwine.so version in this process and in the
 *        examined process
 * Final address is gotten by: 1/ querying the address of a known exported
 * symbol out of libwine.so with dlsym, 2/ then querying nm on libwine.so to
 * get the offset from the data segment of this known symbol and of first_dll,
 * 3/ computing the actual address of first_dll by adding the result of 1/ and
 * the delta of 2/.
 * Ugly, yes, but it somehow works. We should replace that with debughlp
 * library, that'd be way better. Exporting first_dll from libwine.so would make
 * this code simpler, but still ugly.
 */
/* FIXME: we only need those includes for the next function */
#include <dlfcn.h> /* for RTLD_LAZY */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include "wine/library.h"

void* get_symbol(HANDLE hProcess, const char* name, const char* lib)
{
    char                buffer[1024];
    void*               h;
    DWORD               addr = 0, tmp = 0;
    FILE*               f;
    char*               env;

    if (!(h = wine_dlopen(lib, RTLD_LAZY, buffer, sizeof(buffer))))
    {
        printf("Couldn't load %s (%s)\n", lib, buffer);
    return NULL;
    }

    env = getenv("LD_LIBRARY_PATH");
    if (env)
    {
        char            *next, *ptr;
        struct stat     s;

        for (ptr = env = strdup(env); ptr; ptr = next)
        {
            next = strchr(ptr, ':');
            if (next) *next++ = '\0';
            sprintf(buffer, "nm %s", ptr);
            if (buffer[strlen(buffer) - 1] != '/') strcat(buffer, "/");
            strcat(buffer, lib);
            if (stat(buffer + 3, &s) == 0) break;
        }
        free(env);
        if (!ptr)
        {
        printf("Couldn't find %s in LD_LIBRARY_PATH\n", lib);
        return NULL;
    }
    }
    if (!(f = popen(buffer, "r")))
    {
        printf("Cannot execute '%s'\n", buffer);
    return NULL;
    }

    while (fgets(buffer, sizeof(buffer), f))
    {
        char *p = buffer + strlen(buffer) - 1;
        if (p < buffer) continue;
        if (*p == '\n') *p-- = 0;
        if (p - buffer < 11) continue;
        buffer[8] = '\0';
        if (!strcmp(&buffer[11], name)) addr += strtol(buffer, NULL, 16);
        if (buffer[9] == 'D' && !tmp && (tmp = (DWORD)wine_dlsym(h, &buffer[11], NULL, 0)) != 0)
            addr += tmp - strtol(buffer, NULL, 16);
    }
    pclose(f);
    return (char*)addr;
}
#else
void* get_symbol(HANDLE hProcess, const char* name, const char* lib)
{
    printf("get_symbol: not implemented on this platform\n");
    return NULL;
}
#endif

struct dll_option_layout
{
    void*         next;
    void*         prev;
    char* const*  channels;
    unsigned int  nb_channels;
};

typedef int (*EnumChannelCB)(HANDLE, void*, WCHAR*, void*);

/******************************************************************
 *		enum_channel
 *
 * Enumerates all known channels on process hProcess through callback
 * ce.
 */
static int enum_channel(HANDLE hProcess, EnumChannelCB ce, void* user, unsigned unique)
{
    struct dll_option_layout  dol;
    int                       ret = 1;
    void*                     buf_addr;
    WCHAR                     buffer[32];
    void*                     addr;
    const WCHAR**             cache = NULL;
    unsigned                  i, j, num_cache, used_cache;

    addr = get_symbol(hProcess, "first_dll", "libwine.so");
    if (!addr) return -1;
    if (unique)
        cache = HeapAlloc(GetProcessHeap(), 0, (num_cache = 32) * sizeof(WCHAR*));
    else
        num_cache = 0;
    used_cache = 0;

    for (;
         ret && addr && ReadProcessMemory(hProcess, addr, &dol, sizeof(dol), NULL);
         addr = dol.next)
    {
        for (i = 0; i < dol.nb_channels; i++)
        {
            if (ReadProcessMemory(hProcess, (void*)(dol.channels + i), &buf_addr, sizeof(buf_addr), NULL) &&
                ReadProcessMemory(hProcess, buf_addr, buffer, sizeof(buffer), NULL))
            {
                if (unique)
                {
                    /* since some channels are defined in multiple compilation units,
                     * they will appear several times...
                     * so cache the channel's names we already reported and don't report
                     * them again
                     */
                    for (j = 0; j < used_cache; j++)
                        if (!wcscmp(cache[j], buffer + 1)) break;
                    if (j != used_cache) continue;
                    if (used_cache == num_cache)
                        cache = HeapReAlloc(GetProcessHeap(), 0, cache, (num_cache *= 2) * sizeof(WCHAR*));
                    cache[used_cache++] = wcscpy(HeapAlloc(GetProcessHeap(), 0, (wcslen(buffer + 1) + 1) * sizeof(WCHAR)),
                                                  buffer + 1);
                }
                ret = ce(hProcess, buf_addr, buffer, user);
            }
        }
    }
    if (unique)
    {
        for (j = 0; j < used_cache; j++) HeapFree(GetProcessHeap(), 0, (WCHAR*)cache[j]);
        HeapFree(GetProcessHeap(), 0, cache);
    }
    return 0;
}

static void DebugChannels_FillList(HWND hChannelLV)
{
    HANDLE  hProcess;

    (void)ListView_DeleteAllItems(hChannelLV);

    hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ, FALSE, get_selected_pid());
    if (!hProcess) return; /* FIXME messagebox */
    SendMessage(hChannelLV, WM_SETREDRAW, FALSE, 0);
    enum_channel(hProcess, list_channel_CB, (void*)hChannelLV, TRUE);
    SendMessage(hChannelLV, WM_SETREDRAW, TRUE, 0);
    CloseHandle(hProcess);
}

static void DebugChannels_OnCreate(HWND hwndDlg)
{
    HWND      hLV = GetDlgItem(hwndDlg, IDC_DEBUG_CHANNELS_LIST);
    LVCOLUMN  lvc;

    lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    lvc.pszText = L"Debug Channel";
    lvc.cx = 100;
    (void)ListView_InsertColumn(hLV, 0, &lvc);

    lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_CENTER;
    lvc.pszText = L"Fixme";
    lvc.cx = 55;
    (void)ListView_InsertColumn(hLV, 1, &lvc);

    lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_CENTER;
    lvc.pszText = L"Err";
    lvc.cx = 55;
    (void)ListView_InsertColumn(hLV, 2, &lvc);

    lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_CENTER;
    lvc.pszText = L"Warn";
    lvc.cx = 55;
    (void)ListView_InsertColumn(hLV, 3, &lvc);

    lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_CENTER;
    lvc.pszText = L"Trace";
    lvc.cx = 55;
    (void)ListView_InsertColumn(hLV, 4, &lvc);

    DebugChannels_FillList(hLV);
}

static void DebugChannels_OnNotify(HWND hDlg, LPARAM lParam)
{
    NMHDR*  nmh = (NMHDR*)lParam;

    switch (nmh->code)
    {
    case NM_CLICK:
        if (nmh->idFrom == IDC_DEBUG_CHANNELS_LIST)
        {
            LVHITTESTINFO    lhti;
            HWND             hChannelLV;
            HANDLE           hProcess;
            NMITEMACTIVATE*  nmia = (NMITEMACTIVATE*)lParam;

            hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE, get_selected_pid());
            if (!hProcess) return; /* FIXME message box */
            lhti.pt = nmia->ptAction;
            hChannelLV = GetDlgItem(hDlg, IDC_DEBUG_CHANNELS_LIST);
            SendMessage(hChannelLV, LVM_SUBITEMHITTEST, 0, (LPARAM)&lhti);
            if (nmia->iSubItem >= 1 && nmia->iSubItem <= 4)
            {
                WCHAR            val[2];
                WCHAR            name[32];
                unsigned         bitmask = 1 << (lhti.iSubItem - 1);
                struct cce_user  user;

                ListView_GetItemText(hChannelLV, lhti.iItem, 0, name, sizeof(name) / sizeof(name[0]));
                ListView_GetItemText(hChannelLV, lhti.iItem, lhti.iSubItem, val, sizeof(val) / sizeof(val[0]));
                user.name = name;
                user.value = (val[0] == L'x') ? 0 : bitmask;
                user.mask = bitmask;
                user.done = user.notdone = 0;
                enum_channel(hProcess, change_channel_CB, &user, FALSE);
                if (user.done)
                {
                    val[0] ^= (L'x' ^ L' ');
                    ListView_SetItemText(hChannelLV, lhti.iItem, lhti.iSubItem, val);
                }
                if (user.notdone)
                    printf("Some channel instance weren't correctly set\n");
            }
            CloseHandle(hProcess);
        }
        break;
    }
}

static INT_PTR CALLBACK DebugChannelsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        DebugChannels_OnCreate(hDlg);
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        break;
    case WM_NOTIFY:
        DebugChannels_OnNotify(hDlg, lParam);
        break;
    }
    return FALSE;
}

void ProcessPage_OnDebugChannels(void)
{
    DialogBox(hInst, (LPCTSTR)IDD_DEBUG_CHANNELS_DIALOG, hMainWnd, DebugChannelsDlgProc);
}
