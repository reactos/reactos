/*
 * PROJECT:     ReactOS DllHost
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Japanese keyboard type setup program
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */
/*
 * This application is launched via the BootExecute value of smss.exe.
 * This application does not use Win32 API.
 *   Hankaku/Zenkaku (scancode 0x29) : 106 Japanese keyboard
 *   Space           (scancode 0x39) : 101 English keyboard
 *   S               (scancode 0x1F) : Other keyboard
 *
 * It times out after 30 seconds.
 * If no key is pressed for 30 seconds, this app is skipped
 * and will not ask again (HKLM\SYSTEM\CurrentControlSet\Control\JKBDSEL\Done=1).
 * Once done, "jkbdsel" is automatically removed from the BootExecute value of
 * HKLM\SYSTEM\CurrentControlSet\Control\Session Manager.
 *
 * boot/bootdata/hivesys.inf:
 * HKLM,"SYSTEM\CurrentControlSet\Control\Session Manager","BootExecute",0x00010000,"autocheck autochk","jkbdsel"
 */
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winnt.h>
#include <winreg.h>
#include <wchar.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#define KEY_BREAK 0x01
#define KEY_E0    0x02

typedef struct tagKBD_INPUT_DATA
{
    USHORT UnitId;
    USHORT MakeCode;
    USHORT Flags;
    USHORT Reserved;
    ULONG  ExtraInformation;
} KBD_INPUT_DATA;

#define PARAMS_KEY L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services\\i8042prt\\Parameters"
#define DONE_KEY   L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\JKBDSEL"

typedef enum { C_106, C_101, C_OTHER, C_TIMEOUT, C_ERROR } CHOICE;

static void Print(PCWSTR s)
{
    UNICODE_STRING us;
    RtlInitUnicodeString(&us, s);
    NtDisplayString(&us);
}

static void PrintHex(PCWSTR label, ULONG v)
{
    WCHAR buf[8 + 1];
    int i;
    Print(label);
    for (i = 7; i >= 0; --i)
    {
        ULONG d = v & 0xF;
        buf[i] = (WCHAR)(d < 10 ? L'0' + d : L'A' + d - 10);
        v >>= 4;
    }
    buf[8] = 0;
    Print(buf);
    Print(L"\n");
}

static void Delay(ULONG ms)
{
    LARGE_INTEGER t;
    t.QuadPart = -(LONGLONG)ms * 10000LL; /* negative value = relative */
    NtDelayExecution(FALSE, &t);
}

/* ---------- Registry ---------- */
static NTSTATUS OpenKey(PCWSTR path, BOOL create, HANDLE *h)
{
    UNICODE_STRING name;
    OBJECT_ATTRIBUTES oa;
    ULONG disp;

    RtlInitUnicodeString(&name, path);
    InitializeObjectAttributes(&oa, &name, OBJ_CASE_INSENSITIVE, NULL, NULL);
    if (create)
        return NtCreateKey(h, KEY_ALL_ACCESS, &oa, 0, NULL,
                           REG_OPTION_NON_VOLATILE, &disp);
    return NtOpenKey(h, KEY_QUERY_VALUE, &oa);
}

static void SetDword(HANDLE h, PCWSTR name, ULONG v)
{
    UNICODE_STRING n;
    RtlInitUnicodeString(&n, name);
    NtSetValueKey(h, &n, 0, REG_DWORD, &v, sizeof(v));
}

static void SetSz(HANDLE h, PCWSTR name, PCWSTR v)
{
    UNICODE_STRING n;
    RtlInitUnicodeString(&n, name);
    NtSetValueKey(h, &n, 0, REG_SZ, (PVOID)v, (wcslen(v) + 1) * sizeof(WCHAR));
}

static void DelValue(HANDLE h, PCWSTR name)
{
    UNICODE_STRING n;
    RtlInitUnicodeString(&n, name);
    NtDeleteValueKey(h, &n);
}

static BOOL FileExists(PCWSTR path)
{
    UNICODE_STRING name;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK iosb;
    HANDLE h;
    NTSTATUS Status;

    RtlInitUnicodeString(&name, path);
    InitializeObjectAttributes(&oa, &name, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtCreateFile(&h, FILE_READ_ATTRIBUTES | SYNCHRONIZE, &oa, &iosb,
                      NULL, FILE_ATTRIBUTE_NORMAL,
                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                      FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    if (NT_SUCCESS(Status))
        NtClose(h);
    return NT_SUCCESS(Status);
}

static BOOL IsUnattended(void)
{
    return FileExists(L"\\SystemRoot\\unattend.inf");
}

#define NLS_LANG_KEY \
    L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\Nls\\Language"

static USHORT ReadLangId(HANDLE h, PCWSTR valueName)
{
    UNICODE_STRING n, s;
    ULONG buf[16], len, val = 0;
    PKEY_VALUE_PARTIAL_INFORMATION info = (PVOID)buf;

    RtlInitUnicodeString(&n, valueName);
    if (!NT_SUCCESS(NtQueryValueKey(h, &n, KeyValuePartialInformation,
                                    info, sizeof(buf), &len)))
        return 0;
    if (info->Type != REG_SZ || info->DataLength < sizeof(WCHAR))
        return 0;

    s.Buffer = (PWSTR)info->Data;
    s.Length = (USHORT)(info->DataLength - sizeof(WCHAR));   /* Excluding NUL */
    s.MaximumLength = (USHORT)info->DataLength;
    if (!NT_SUCCESS(RtlUnicodeStringToInteger(&s, 16, &val)))
        return 0;
    return (USHORT)val;
}

static BOOL IsJapaneseSystem(void)
{
    HANDLE h;
    USHORT lang;
    BOOL ja = FALSE;

    if (!NT_SUCCESS(OpenKey(NLS_LANG_KEY, FALSE, &h)))
        return FALSE;

    lang = ReadLangId(h, L"InstallLanguage");
    if (lang == 0)
        lang = ReadLangId(h, L"Default");

    if (lang != 0 && PRIMARYLANGID(lang) == LANG_JAPANESE)
        ja = TRUE;

    NtClose(h);
    return ja;
}

static BOOL IsDone(void)
{
    HANDLE h;
    UNICODE_STRING n;
    ULONG buf[8], len;
    PKEY_VALUE_PARTIAL_INFORMATION info = (PVOID)buf;
    BOOL done = FALSE;

    if (!NT_SUCCESS(OpenKey(DONE_KEY, FALSE, &h)))
        return FALSE;
    RtlInitUnicodeString(&n, L"Done");
    if (NT_SUCCESS(NtQueryValueKey(h, &n, KeyValuePartialInformation,
                                   info, sizeof(buf), &len))
        && info->Type == REG_DWORD && info->DataLength == sizeof(ULONG)
        && *(PULONG)info->Data != 0)
        done = TRUE;
    NtClose(h);
    return done;
}

#define SESSION_MANAGER_KEY \
    L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\Session Manager"

/*
 * Remove "jkbdsel" from the REG_MULTI_SZ value BootExecute of
 * HKLM\SYSTEM\CurrentControlSet\Control\Session Manager.
 */
static BOOL RemoveFromBootExecute(void)
{
    UNICODE_STRING keyName, valName, target, item;
    OBJECT_ATTRIBUTES oa;
    HANDLE h;
    NTSTATUS Status;
    ULONG len = 0;
    PKEY_VALUE_PARTIAL_INFORMATION info = NULL;
    PWCHAR out = NULL, src, end, dst;
    PVOID heap = RtlGetProcessHeap();
    BOOL removed = FALSE, ok = FALSE;

    RtlInitUnicodeString(&keyName, SESSION_MANAGER_KEY);
    InitializeObjectAttributes(&oa, &keyName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    if (!NT_SUCCESS(NtOpenKey(&h, KEY_QUERY_VALUE | KEY_SET_VALUE, &oa)))
        return FALSE;

    RtlInitUnicodeString(&valName, L"BootExecute");
    RtlInitUnicodeString(&target, L"jkbdsel");

    /* Get the required size */
    Status = NtQueryValueKey(h, &valName, KeyValuePartialInformation, NULL, 0, &len);
    if ((Status != STATUS_BUFFER_TOO_SMALL && Status != STATUS_BUFFER_OVERFLOW) || !len)
        goto cleanup;

    info = RtlAllocateHeap(heap, 0, len);
    out = RtlAllocateHeap(heap, 0, len + 2 * sizeof(WCHAR));
    if (!info || !out)
        goto cleanup;

    Status = NtQueryValueKey(h, &valName, KeyValuePartialInformation, info, len, &len);
    if (!NT_SUCCESS(Status) || info->Type != REG_MULTI_SZ)
        goto cleanup;

    src = (PWCHAR)info->Data;
    end = src + info->DataLength / sizeof(WCHAR);
    dst = out;

    /* Copy every entry except "jkbdsel" */
    while (src < end && *src)
    {
        ULONG cch;
        PWCHAR pch;

        for (pch = src; pch < end && *pch; ++pch)
            ;

        cch = (ULONG)(pch - src);

        item.Buffer = src;
        item.Length = item.MaximumLength = (USHORT)(cch * sizeof(WCHAR));

        if (RtlEqualUnicodeString(&item, &target, TRUE))
        {
            removed = TRUE;
        }
        else
        {
            RtlCopyMemory(dst, src, cch * sizeof(WCHAR));
            dst += cch;
            *dst++ = UNICODE_NULL;
        }

        src = pch + 1;
    }

    if (!removed)
    {
        ok = TRUE; /* Nothing to do */
        goto cleanup;
    }

    /* Final terminator (an empty list becomes two NULs) */
    *dst++ = UNICODE_NULL;
    if (dst == out + 1)
        *dst++ = UNICODE_NULL;

    Status = NtSetValueKey(h, &valName, 0, REG_MULTI_SZ, out, (ULONG)((dst - out) * sizeof(WCHAR)));
    if (NT_SUCCESS(Status))
    {
        NtFlushKey(h);
        ok = TRUE;
    }

cleanup:
    if (info)
        RtlFreeHeap(heap, 0, info);
    if (out)
        RtlFreeHeap(heap, 0, out);
    NtClose(h);
    return ok;
}

static BOOL WriteDone(void)
{
    HANDLE h;
    NTSTATUS Status = OpenKey(DONE_KEY, TRUE, &h);
    if (!NT_SUCCESS(Status))
    {
        PrintHex(L"JKBDSEL: create DONE_KEY failed: 0x", (ULONG)Status);
        Delay(5000);
        return FALSE;
    }
    SetDword(h, L"Done", 1);
    NtFlushKey(h);
    NtClose(h);

    /* Setup is finished: don't run at the next boot */
    RemoveFromBootExecute();
    return TRUE;
}

static BOOL  Apply(CHOICE c)
{
    HANDLE h;

    if (!NT_SUCCESS(OpenKey(PARAMS_KEY, TRUE, &h)))
        return FALSE;

    if (c == C_OTHER)
    {
        DelValue(h, L"OverrideKeyboardType");
        DelValue(h, L"OverrideKeyboardSubtype");
        DelValue(h, L"LayerDriver JPN");
    }
    else
    {
        SetDword(h, L"OverrideKeyboardType", 7);
        SetDword(h, L"OverrideKeyboardSubtype", (c == C_106) ? 2 : 0);
        SetSz(h, L"LayerDriver JPN", (c == C_106) ? L"kbd106.dll" : L"kbd101.dll");
    }

    NtFlushKey(h);
    NtClose(h);

    return WriteDone();
}

/* ---------- Key Input ---------- */

#define WAIT_SECONDS 30

static CHOICE WaitForChoice(void)
{
    UNICODE_STRING name;
    OBJECT_ATTRIBUTES oa;
    static IO_STATUS_BLOCK iosb;
    static KBD_INPUT_DATA kd;
    HANDLE hKbd = NULL, hEvent = NULL;
    LARGE_INTEGER deadline;
    NTSTATUS Status;
    CHOICE ret = C_ERROR;
    ULONG startTick = NtGetTickCount();

    RtlInitUnicodeString(&name, L"\\Device\\KeyboardClass0");
    InitializeObjectAttributes(&oa, &name, OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = NtCreateFile(&hKbd, GENERIC_READ | SYNCHRONIZE, &oa, &iosb, NULL,
                      FILE_ATTRIBUTE_NORMAL,
                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                      FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    if (!NT_SUCCESS(Status))
    {
        PrintHex(L"JKBDSEL: open failed: 0x", (ULONG)Status);
        Delay(5000);
        return C_ERROR;
    }

    Status = NtCreateEvent(&hEvent, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE);
    if (!NT_SUCCESS(Status))
    {
        PrintHex(L"JKBDSEL: event failed: 0x", (ULONG)Status);
        Delay(5000);
        NtClose(hKbd);
        return C_ERROR;
    }

    NtQuerySystemTime(&deadline);
    deadline.QuadPart += (LONGLONG)WAIT_SECONDS * 10000000LL;

    for (;;)
    {
        LONGLONG remain = (LONGLONG)WAIT_SECONDS * 1000 -
                          (LONGLONG)(NtGetTickCount() - startTick);
        LARGE_INTEGER to, offset;

        if (remain <= 0)
        {
            ret = C_TIMEOUT;
            break;
        }
        to.QuadPart = -remain * 10000LL; /* negative value == relative */

        NtResetEvent(hEvent, NULL);
        offset.QuadPart = 0;
        Status = NtReadFile(hKbd, hEvent, NULL, NULL, &iosb,
                        &kd, sizeof(kd), &offset, NULL);
        //PrintHex(L"JKBDSEL: read Status: 0x", (ULONG)Status);

        if (Status == STATUS_PENDING)
        {
            Status = NtWaitForSingleObject(hEvent, FALSE, &to);
            //PrintHex(L"JKBDSEL: wait Status: 0x", (ULONG)Status);
            if (Status == STATUS_TIMEOUT)
            {
                NtCancelIoFile(hKbd, &iosb);
                ret = C_TIMEOUT;
                break;
            }
            Status = iosb.Status;
            //PrintHex(L"JKBDSEL: iosb Status: 0x", (ULONG)Status);
        }

        if (!NT_SUCCESS(Status))
            break;

        if ((kd.Flags & KEY_BREAK) || (kd.Flags & KEY_E0))
            continue;

        switch (kd.MakeCode)
        {
            case 0x29: ret = C_106;   goto done; /* Hankaku/Zenkaku */
            case 0x39: ret = C_101;   goto done; /* Space */
            case 0x1F: ret = C_OTHER; goto done; /* S */
        }
    }

done:
    NtClose(hEvent);
    NtClose(hKbd);
    return ret;
}

/* ---------- Entry ---------- */
VOID NTAPI NtProcessStartup(PPEB Peb)
{
    CHOICE c;
    BOOL bSelected = FALSE;

    UNREFERENCED_PARAMETER(Peb);

    if (!IsJapaneseSystem())
        goto quit;

    if (IsDone())
    {
        RemoveFromBootExecute();
        goto quit;
    }

    if (IsUnattended())
    {
        Print(L"JKBDSEL: Detected unattended setup\n");
        WriteDone();
        goto quit;
    }

    /* NOTE: The following text should be easy English. */
    Print(
        L"\n\n"
        L"  ReactOS - Japanese Keyboard Setup\n"
        L"  ---------------------------------\n\n"
        L"  Please press key on keyboard within 30 seconds:\n\n"
        L"    [Hankaku/Zenkaku] key : for 106 Japanese keyboard\n"
        L"    [Space] key           : for 101 English keyboard\n"
        L"    [S] key               : for other keyboard (use default)\n\n");

    c = WaitForChoice();
    //PrintHex(L"JKBDSEL: choice = 0x", (ULONG)c); /* 0:106 1:101 2:OTHER 3:SKIP 4:TIMEOUT 5:ERROR */

    switch (c)
    {
        case C_106:
            Print(L"JKBDSEL: Selected 106.\n");
            bSelected = TRUE;
            break;
        case C_101:
            Print(L"JKBDSEL: Selected 101.\n");
            bSelected = TRUE;
            break;
        case C_OTHER:
            Print(L"JKBDSEL: Selected other.\n");
            bSelected = TRUE;
            break;
        case C_TIMEOUT:
            Print(L"JKBDSEL: Skipped.\n");
            WriteDone();
            Delay(3000);
            goto quit;
        case C_ERROR:
            Print(L"JKBDSEL: Error.\n");
            goto quit;
        default:
            Print(L"JKBDSEL: Logical error.\n");
            goto quit;
    }

    if (bSelected)
    {
        if (Apply(c))
            Print(L"JKBDSEL: Keyboard setting saved.\n");
        else
            Print(L"JKBDSEL: Failed to write the registry.\n");

        Delay(3000);
    }

quit:
    NtTerminateProcess(NtCurrentProcess(), 0);
}
