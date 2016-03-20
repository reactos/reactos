/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for RtlGenerate8dot3Name
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <ndk/rtlfuncs.h>
#include <stdio.h>

NTSYSAPI
VOID
NTAPI
RtlGenerate8dot3Name(
  _In_ PCUNICODE_STRING Name,
  _In_ BOOLEAN AllowExtendedCharacters,
  _Inout_ PGENERATE_NAME_CONTEXT Context,
  _Inout_ PUNICODE_STRING Name8dot3);

PWSTR Names[] = { L"Menu Démarrer", L"Sélecteur de configuration clavier.lnk", L"éèàùç.txt", L"çùàèé.txt", L"éèàù.txt", L"çùàè.txt", L"éèàùçeeauc.txt", L"éeèéçcùu.txt", L"test.éxè", L"t£$t¤.txt", L"Long file name.txt", L"Long file name", L"Longfilename.txt", L"Longfilename" };
PWSTR ShortNames1[] = { L"MENUDM~1", L"SLECTE~1.LNK", L"5C2D~1.TXT", L"423C~1.TXT", L"925E~1.TXT", L"7E4C~1.TXT", L"EEAUC~1.TXT", L"ECU~1.TXT", L"TEST~1.X", L"T$T~1.TXT", L"LONGFI~1.TXT", L"LONGFI~1", L"LONGFI~1.TXT", L"LONGFI~1" };
PWSTR ShortNames2[] = { L"MENUDM~2", L"SLECTE~2.LNK", L"5C2D~2.TXT", L"423C~2.TXT", L"925E~2.TXT", L"7E4C~2.TXT", L"EEAUC~2.TXT", L"ECU~2.TXT", L"TEST~2.X", L"T$T~2.TXT", L"LONGFI~2.TXT", L"LONGFI~2", L"LONGFI~2.TXT", L"LONGFI~2" };
PWSTR ExShortNames1[] = { L"MENUDÉ~1", L"SÉLECT~1.LNK", L"ÉÈÀÙÇ~1.TXT", L"ÇÙÀÈÉ~1.TXT", L"ÉÈÀÙ~1.TXT", L"ÇÙÀÈ~1.TXT", L"ÉÈÀÙÇE~1.TXT", L"ÉEÈÉÇC~1.TXT", L"TEST~1.ÉXÈ", L"T£$T¤~1.TXT", L"LONGFI~1.TXT", L"LONGFI~1", L"LONGFI~1.TXT", L"LONGFI~1" };
PWSTR ExShortNames2[] = { L"MENUDÉ~2", L"SÉLECT~2.LNK", L"ÉÈÀÙÇ~2.TXT", L"ÇÙÀÈÉ~2.TXT", L"ÉÈÀÙ~2.TXT", L"ÇÙÀÈ~2.TXT", L"ÉÈÀÙÇE~2.TXT", L"ÉEÈÉÇC~2.TXT", L"TEST~2.ÉXÈ", L"T£$T¤~2.TXT", L"LONGFI~2.TXT", L"LONGFI~2", L"LONGFI~2.TXT", L"LONGFI~2" };
 
PVOID LoadCodePageData(ULONG Code)
{
    char filename[MAX_PATH], sysdir[MAX_PATH];
    HANDLE hFile;
    PVOID Data = NULL;
    GetSystemDirectoryA(sysdir, MAX_PATH);

    if (Code != -1)
       sprintf(filename, "%s\\c_%lu.nls", sysdir, Code);
    else
        sprintf(filename, "%s\\l_intl.nls", sysdir);

    hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwRead;
        DWORD dwFileSize = GetFileSize(hFile, NULL);
        Data = malloc(dwFileSize);
        ReadFile(hFile, Data, dwFileSize, &dwRead, NULL);
        CloseHandle(hFile);
    }
    return Data;
}

/* https://www.microsoft.com/resources/msdn/goglobal/default.mspx */
void SetupLocale(ULONG AnsiCode, ULONG OemCode, ULONG Unicode)
{
    NLSTABLEINFO NlsTable;
    PVOID AnsiCodePageData;
    PVOID OemCodePageData;
    PVOID UnicodeCaseTableData;

    AnsiCodePageData = LoadCodePageData(AnsiCode);
    OemCodePageData = LoadCodePageData(OemCode);
    UnicodeCaseTableData = LoadCodePageData(Unicode);

    RtlInitNlsTables(AnsiCodePageData, OemCodePageData, UnicodeCaseTableData, &NlsTable);
    RtlResetRtlTranslations(&NlsTable);
    /* Do NOT free the buffers here, they are directly used!
        Yes, we leak the old buffers, but this is a test anyway... */

}

START_TEST(RtlGenerate8dot3Name)
{
    USHORT i;

    /* Set a french locale. */
    SetupLocale(1252, 850, -1);

    for (i = 0; i < 14; ++i)
    {
        WCHAR Buffer[12];
        GENERATE_NAME_CONTEXT Context;
        UNICODE_STRING LongName, ShortName, Expected;

        RtlZeroMemory(&Context, sizeof(GENERATE_NAME_CONTEXT));
        RtlInitUnicodeString(&LongName, Names[i]);
        ShortName.Buffer = Buffer;
        ShortName.Length = 0;
        ShortName.MaximumLength = sizeof(Buffer);

        RtlGenerate8dot3Name(&LongName, FALSE, &Context, &ShortName);
        RtlInitUnicodeString(&Expected, ShortNames1[i]);
        ok(RtlEqualUnicodeString(&Expected, &ShortName, FALSE), "Generated: %.*S. Expected: %.*S\n", ShortName.Length / sizeof(WCHAR), ShortName.Buffer, Expected.Length / sizeof(WCHAR), Expected.Buffer);

        ShortName.Length = 0;
        RtlGenerate8dot3Name(&LongName, FALSE, &Context, &ShortName);
        RtlInitUnicodeString(&Expected, ShortNames2[i]);
        ok(RtlEqualUnicodeString(&Expected, &ShortName, FALSE), "Generated: %.*S. Expected: %.*S\n", ShortName.Length / sizeof(WCHAR), ShortName.Buffer, Expected.Length / sizeof(WCHAR), Expected.Buffer);

        RtlZeroMemory(&Context, sizeof(GENERATE_NAME_CONTEXT));
        ShortName.Length = 0;
        RtlGenerate8dot3Name(&LongName, TRUE, &Context, &ShortName);
        RtlInitUnicodeString(&Expected, ExShortNames1[i]);
        ok(RtlEqualUnicodeString(&Expected, &ShortName, FALSE), "Generated: %.*S. Expected: %.*S\n", ShortName.Length / sizeof(WCHAR), ShortName.Buffer, Expected.Length / sizeof(WCHAR), Expected.Buffer);

        ShortName.Length = 0;
        RtlGenerate8dot3Name(&LongName, TRUE, &Context, &ShortName);
        RtlInitUnicodeString(&Expected, ExShortNames2[i]);
        ok(RtlEqualUnicodeString(&Expected, &ShortName, FALSE), "Generated: %.*S. Expected: %.*S\n", ShortName.Length / sizeof(WCHAR), ShortName.Buffer, Expected.Length / sizeof(WCHAR), Expected.Buffer);
    }
}
