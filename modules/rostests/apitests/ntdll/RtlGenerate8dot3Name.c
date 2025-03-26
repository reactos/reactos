// This file is converted by code7bit.
// code7bit: https://github.com/katahiromz/code7bit
// To revert conversion, please execute "code7bit -r <file>".
/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for RtlGenerate8dot3Name
 * COPYRIGHT:   Copyright 2015-2016 Pierre Schweitzer <pierre@reactos.org>
 *              Copyright 2021 Jérôme Gardou <jerome.gardou@reactos.org>
 */

#include "precomp.h"

NTSYSAPI
VOID
NTAPI
RtlGenerate8dot3Name(
  _In_ PCUNICODE_STRING Name,
  _In_ BOOLEAN AllowExtendedCharacters,
  _Inout_ PGENERATE_NAME_CONTEXT Context,
  _Inout_ PUNICODE_STRING Name8dot3);

ULONG Locales[][2] =
{
    {1252, 850}, // Most used for latin langs
    {1252, 437}, // Used for English US (not only)
    {1252, 775}, // Used for Estonian
    {1252, 932}, // Western SBCS, Japanese
    {932, 1252}, // Japanese, Western SBCS
    {932, 932},  // Japanese, Japanese
};
#define LOCALES_COUNT _countof(Locales)

PWSTR Names[] = {
    L"Menu D\u00E9marrer",
    L"S\u00E9lecteur de configuration clavier.lnk",
    L"\u00E9\u00E8\u00E0\u00F9\u00E7.txt",
    L"\u00E7\u00F9\u00E0\u00E8\u00E9.txt",
    L"\u00E9\u00E8\u00E0\u00F9.txt",
    L"\u00E7\u00F9\u00E0\u00E8.txt",
    L"\u00E9\u00E8\u00E0\u00F9\u00E7eeauc.txt",
    L"\u00E9e\u00E8\u00E9\u00E7c\u00F9u.txt",
    L"test.\u00E9x\u00E8",
    L"t\u00A3$t\u00A4.txt",
    L"Long file name.txt",
    L"Long file name",
    L"Longfilename.txt",
    L"Longfilename",
    L"\u30c7\u30b9\u30af\u30c8\u30c3\u30d7" /* Desktop in Japanese */
};
#define NAMES_COUNT _countof(Names)

PWSTR ShortNames1[LOCALES_COUNT][NAMES_COUNT] =
{
    { L"MENUDM~1", L"SLECTE~1.LNK", L"5C2D~1.TXT", L"423C~1.TXT", L"925E~1.TXT", L"7E4C~1.TXT", L"EEAUC~1.TXT", L"ECU~1.TXT", L"TEST~1.X", L"T$T~1.TXT", L"LONGFI~1.TXT", L"LONGFI~1", L"LONGFI~1.TXT", L"LONGFI~1", L"9A16~1" },
    { L"MENUDM~1", L"SLECTE~1.LNK", L"5C2D~1.TXT", L"423C~1.TXT", L"925E~1.TXT", L"7E4C~1.TXT", L"EEAUC~1.TXT", L"ECU~1.TXT", L"TEST~1.X", L"T$T~1.TXT", L"LONGFI~1.TXT", L"LONGFI~1", L"LONGFI~1.TXT", L"LONGFI~1", L"9A16~1" },
    { L"MENUDM~1", L"SLECTE~1.LNK", L"5C2D~1.TXT", L"423C~1.TXT", L"925E~1.TXT", L"7E4C~1.TXT", L"EEAUC~1.TXT", L"ECU~1.TXT", L"TEST~1.X", L"T$T~1.TXT", L"LONGFI~1.TXT", L"LONGFI~1", L"LONGFI~1.TXT", L"LONGFI~1", L"9A16~1" },
    { L"MENUDM~1", L"SLECTE~1.LNK", L"5C2D~1.TXT", L"423C~1.TXT", L"925E~1.TXT", L"7E4C~1.TXT", L"EEAUC~1.TXT", L"ECU~1.TXT", L"TEST~1.X", L"T$T~1.TXT", L"LONGFI~1.TXT", L"LONGFI~1", L"LONGFI~1.TXT", L"LONGFI~1", L"9A16~1" },
    { L"MENUDM~1", L"SLECTE~1.LNK", L"5C2D~1.TXT", L"423C~1.TXT", L"925E~1.TXT", L"7E4C~1.TXT", L"EEAUC~1.TXT", L"ECU~1.TXT", L"TEST~1.X", L"T$T~1.TXT", L"LONGFI~1.TXT", L"LONGFI~1", L"LONGFI~1.TXT", L"LONGFI~1", L"9A16~1" },
    { L"MENUDM~1", L"SLECTE~1.LNK", L"5C2D~1.TXT", L"423C~1.TXT", L"925E~1.TXT", L"7E4C~1.TXT", L"EEAUC~1.TXT", L"ECU~1.TXT", L"TEST~1.X", L"T$T~1.TXT", L"LONGFI~1.TXT", L"LONGFI~1", L"LONGFI~1.TXT", L"LONGFI~1", L"9A16~1" },
};
PWSTR ShortNames2[LOCALES_COUNT][NAMES_COUNT] =
{
    { L"MENUDM~2", L"SLECTE~2.LNK", L"5C2D~2.TXT", L"423C~2.TXT", L"925E~2.TXT", L"7E4C~2.TXT", L"EEAUC~2.TXT", L"ECU~2.TXT", L"TEST~2.X", L"T$T~2.TXT", L"LONGFI~2.TXT", L"LONGFI~2", L"LONGFI~2.TXT", L"LONGFI~2", L"9A16~2" },
    { L"MENUDM~2", L"SLECTE~2.LNK", L"5C2D~2.TXT", L"423C~2.TXT", L"925E~2.TXT", L"7E4C~2.TXT", L"EEAUC~2.TXT", L"ECU~2.TXT", L"TEST~2.X", L"T$T~2.TXT", L"LONGFI~2.TXT", L"LONGFI~2", L"LONGFI~2.TXT", L"LONGFI~2", L"9A16~2" },
    { L"MENUDM~2", L"SLECTE~2.LNK", L"5C2D~2.TXT", L"423C~2.TXT", L"925E~2.TXT", L"7E4C~2.TXT", L"EEAUC~2.TXT", L"ECU~2.TXT", L"TEST~2.X", L"T$T~2.TXT", L"LONGFI~2.TXT", L"LONGFI~2", L"LONGFI~2.TXT", L"LONGFI~2", L"9A16~2" },
    { L"MENUDM~2", L"SLECTE~2.LNK", L"5C2D~2.TXT", L"423C~2.TXT", L"925E~2.TXT", L"7E4C~2.TXT", L"EEAUC~2.TXT", L"ECU~2.TXT", L"TEST~2.X", L"T$T~2.TXT", L"LONGFI~2.TXT", L"LONGFI~2", L"LONGFI~2.TXT", L"LONGFI~2", L"9A16~2" },
    { L"MENUDM~2", L"SLECTE~2.LNK", L"5C2D~2.TXT", L"423C~2.TXT", L"925E~2.TXT", L"7E4C~2.TXT", L"EEAUC~2.TXT", L"ECU~2.TXT", L"TEST~2.X", L"T$T~2.TXT", L"LONGFI~2.TXT", L"LONGFI~2", L"LONGFI~2.TXT", L"LONGFI~2", L"9A16~2" },
    { L"MENUDM~2", L"SLECTE~2.LNK", L"5C2D~2.TXT", L"423C~2.TXT", L"925E~2.TXT", L"7E4C~2.TXT", L"EEAUC~2.TXT", L"ECU~2.TXT", L"TEST~2.X", L"T$T~2.TXT", L"LONGFI~2.TXT", L"LONGFI~2", L"LONGFI~2.TXT", L"LONGFI~2", L"9A16~2" },
};
PWSTR ExShortNames1[LOCALES_COUNT][NAMES_COUNT] =
{
    { L"MENUD\u00C9~1", L"S\u00C9LECT~1.LNK", L"\u00C9\u00C8\u00C0\u00D9\u00C7~1.TXT", L"\u00C7\u00D9\u00C0\u00C8\u00C9~1.TXT", L"\u00C9\u00C8\u00C0\u00D9~1.TXT", L"\u00C7\u00D9\u00C0\u00C8~1.TXT", L"\u00C9\u00C8\u00C0\u00D9\u00C7E~1.TXT", L"\u00C9E\u00C8\u00C9\u00C7C~1.TXT", L"TEST~1.\u00C9X\u00C8", L"T\u00A3$T\u00A4~1.TXT", L"LONGFI~1.TXT", L"LONGFI~1", L"LONGFI~1.TXT", L"LONGFI~1", L"9A16~1" },
    { L"MENUD\u00C9~1", L"S\u00C9LECT~1.LNK", L"\u00C9\u00C8\u00C0\u00D9\u00C7~1.TXT", L"\u00C7\u00D9\u00C0\u00C8\u00C9~1.TXT", L"\u00C9\u00C8\u00C0\u00D9~1.TXT", L"\u00C7\u00D9\u00C0\u00C8~1.TXT", L"\u00C9\u00C8\u00C0\u00D9\u00C7E~1.TXT", L"\u00C9E\u00C8\u00C9\u00C7C~1.TXT", L"TEST~1.\u00C9X\u00C8", L"T\u00A3$T_~1.TXT", L"LONGFI~1.TXT", L"LONGFI~1", L"LONGFI~1.TXT", L"LONGFI~1", L"9A16~1" },
    { L"MENUD\u00C9~1", L"S\u00C9LECT~1.LNK", L"\u00C9CAU~1.TXT", L"UAC\u00C9~1.TXT", L"\u00C9CAU~1.TXT", L"UAC~1.TXT", L"\u00C9CAUEE~1.TXT", L"\u00C9EC\u00C9CU~1.TXT", L"TEST~1.\u00C9XC", L"T\u00A3$T\u00A4~1.TXT", L"LONGFI~1.TXT", L"LONGFI~1", L"LONGFI~1.TXT", L"LONGFI~1", L"9A16~1" },
    { L"MENUDE~1", L"SELECT~1.LNK", L"EEAUC~1.TXT", L"CUAEE~1.TXT", L"EEAU~1.TXT", L"CUAE~1.TXT", L"EEAUCE~1.TXT", L"EEEECC~1.TXT", L"TEST~1.EXE", L"T\uFFE1$T~1.TXT", L"LONGFI~1.TXT", L"LONGFI~1", L"LONGFI~1.TXT", L"LONGFI~1", L"\u30c7\u30b9\u30af~1" },
    { L"MENUD\u00C9~1", L"S\u00C9LECT~1.LNK", L"\u00C9\u00C8\u00C0\u00D9\u00C7~1.TXT", L"\u00C7\u00D9\u00C0\u00C8\u00C9~1.TXT", L"\u00C9\u00C8\u00C0\u00D9~1.TXT", L"\u00C7\u00D9\u00C0\u00C8~1.TXT", L"\u00C9\u00C8\u00C0\u00D9\u00C7E~1.TXT", L"\u00C9E\u00C8\u00C9\u00C7C~1.TXT", L"TEST~1.\u00C9X\u00C8", L"T\u00A3$T\u00A4~1.TXT", L"LONGFI~1.TXT", L"LONGFI~1", L"LONGFI~1.TXT", L"LONGFI~1", L"9A16~1" },
    { L"MENUDE~1", L"SELECT~1.LNK", L"EEAUC~1.TXT", L"CUAEE~1.TXT", L"EEAU~1.TXT", L"CUAE~1.TXT", L"EEAUCE~1.TXT", L"EEEECC~1.TXT", L"TEST~1.EXE", L"T\uFFE1$T~1.TXT", L"LONGFI~1.TXT", L"LONGFI~1", L"LONGFI~1.TXT", L"LONGFI~1", L"\u30c7\u30b9\u30af~1" },
};
PWSTR ExShortNames2[LOCALES_COUNT][NAMES_COUNT] =
{
    { L"MENUD\u00C9~2", L"S\u00C9LECT~2.LNK", L"\u00C9\u00C8\u00C0\u00D9\u00C7~2.TXT", L"\u00C7\u00D9\u00C0\u00C8\u00C9~2.TXT", L"\u00C9\u00C8\u00C0\u00D9~2.TXT", L"\u00C7\u00D9\u00C0\u00C8~2.TXT", L"\u00C9\u00C8\u00C0\u00D9\u00C7E~2.TXT", L"\u00C9E\u00C8\u00C9\u00C7C~2.TXT", L"TEST~2.\u00C9X\u00C8", L"T\u00A3$T\u00A4~2.TXT", L"LONGFI~2.TXT", L"LONGFI~2", L"LONGFI~2.TXT", L"LONGFI~2", L"9A16~2" },
    { L"MENUD\u00C9~2", L"S\u00C9LECT~2.LNK", L"\u00C9\u00C8\u00C0\u00D9\u00C7~2.TXT", L"\u00C7\u00D9\u00C0\u00C8\u00C9~2.TXT", L"\u00C9\u00C8\u00C0\u00D9~2.TXT", L"\u00C7\u00D9\u00C0\u00C8~2.TXT", L"\u00C9\u00C8\u00C0\u00D9\u00C7E~2.TXT", L"\u00C9E\u00C8\u00C9\u00C7C~2.TXT", L"TEST~2.\u00C9X\u00C8", L"T\u00A3$T_~2.TXT", L"LONGFI~2.TXT", L"LONGFI~2", L"LONGFI~2.TXT", L"LONGFI~2", L"9A16~2" },
    { L"MENUD\u00C9~2", L"S\u00C9LECT~2.LNK", L"\u00C9CAU~2.TXT", L"UAC\u00C9~2.TXT", L"\u00C9CAU~2.TXT", L"UAC~2.TXT", L"\u00C9CAUEE~2.TXT", L"\u00C9EC\u00C9CU~2.TXT", L"TEST~2.\u00C9XC", L"T\u00A3$T\u00A4~2.TXT", L"LONGFI~2.TXT", L"LONGFI~2", L"LONGFI~2.TXT", L"LONGFI~2", L"9A16~2" },
    { L"MENUDE~2", L"SELECT~2.LNK", L"EEAUC~2.TXT", L"CUAEE~2.TXT", L"EEAU~2.TXT", L"CUAE~2.TXT", L"EEAUCE~2.TXT", L"EEEECC~2.TXT", L"TEST~2.EXE", L"T\uFFE1$T~2.TXT", L"LONGFI~2.TXT", L"LONGFI~2", L"LONGFI~2.TXT", L"LONGFI~2", L"\u30c7\u30b9\u30af~2" },
    { L"MENUD\u00C9~2", L"S\u00C9LECT~2.LNK", L"\u00C9\u00C8\u00C0\u00D9\u00C7~2.TXT", L"\u00C7\u00D9\u00C0\u00C8\u00C9~2.TXT", L"\u00C9\u00C8\u00C0\u00D9~2.TXT", L"\u00C7\u00D9\u00C0\u00C8~2.TXT", L"\u00C9\u00C8\u00C0\u00D9\u00C7E~2.TXT", L"\u00C9E\u00C8\u00C9\u00C7C~2.TXT", L"TEST~2.\u00C9X\u00C8", L"T\u00A3$T\u00A4~2.TXT", L"LONGFI~2.TXT", L"LONGFI~2", L"LONGFI~2.TXT", L"LONGFI~2", L"9A16~2" },
    { L"MENUDE~2", L"SELECT~2.LNK", L"EEAUC~2.TXT", L"CUAEE~2.TXT", L"EEAU~2.TXT", L"CUAE~2.TXT", L"EEAUCE~2.TXT", L"EEEECC~2.TXT", L"TEST~2.EXE", L"T\uFFE1$T~2.TXT", L"LONGFI~2.TXT", L"LONGFI~2", L"LONGFI~2.TXT", L"LONGFI~2", L"\u30c7\u30b9\u30af~2" },
};

#define LONG_NAMES_COUNT 12
PWSTR LongNames[LONG_NAMES_COUNT] =
{
    L"Long File Name 1.txt", L"Long File Name 2.txt", L"Long File Name 3.txt", L"Long File Name 4.txt", L"Long File Name 5.txt", L"Long File Name 6.txt",
    L"Very Long File Name 1.txt", L"Very Long File Name 2.txt", L"Very Long File Name 3.txt", L"Very Long File Name 4.txt", L"Very Long File Name 5.txt", L"Very Long File Name 6.txt",
};

PWSTR LongShortNames[LONG_NAMES_COUNT] =
{
    L"LONGFI~1.TXT", L"LONGFI~2.TXT", L"LONGFI~3.TXT", L"LONGFI~4.TXT", L"LO1796~1.TXT", L"LO1796~2.TXT",
    L"VERYLO~1.TXT", L"VERYLO~2.TXT", L"VERYLO~3.TXT", L"VERYLO~4.TXT", L"VED051~1.TXT", L"VED051~2.TXT",
};

START_TEST(RtlGenerate8dot3Name)
{
    USHORT i, j;

    for (j = 0; j < LOCALES_COUNT; ++j)
    {
        /* Setup locale. */
        SetupLocale(Locales[j][0], Locales[j][1], -1);

        for (i = 0; i < NAMES_COUNT; ++i)
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
            RtlInitUnicodeString(&Expected, ShortNames1[j][i]);
            ok(RtlEqualUnicodeString(&Expected, &ShortName, FALSE), "%u,%u:: Generated: %.*S. Expected: %.*S\n", j, i, ShortName.Length / sizeof(WCHAR), ShortName.Buffer, Expected.Length / sizeof(WCHAR), Expected.Buffer);

            ShortName.Length = 0;
            RtlGenerate8dot3Name(&LongName, FALSE, &Context, &ShortName);
            RtlInitUnicodeString(&Expected, ShortNames2[j][i]);
            ok(RtlEqualUnicodeString(&Expected, &ShortName, FALSE), "%u,%u:: Generated: %.*S. Expected: %.*S\n", j, i, ShortName.Length / sizeof(WCHAR), ShortName.Buffer, Expected.Length / sizeof(WCHAR), Expected.Buffer);

            RtlZeroMemory(&Context, sizeof(GENERATE_NAME_CONTEXT));
            ShortName.Length = 0;
            RtlGenerate8dot3Name(&LongName, TRUE, &Context, &ShortName);
            RtlInitUnicodeString(&Expected, ExShortNames1[j][i]);
            ok(RtlEqualUnicodeString(&Expected, &ShortName, FALSE), "%u,%u:: Generated: %.*S. Expected: %.*S\n", j, i, ShortName.Length / sizeof(WCHAR), ShortName.Buffer, Expected.Length / sizeof(WCHAR), Expected.Buffer);
            if (!RtlEqualUnicodeString(&Expected, &ShortName, FALSE))
            {
                for (int k = 0; k < (ShortName.Length / sizeof(WCHAR)); k++)
                    trace("Got \\u%04x at %d\n", ShortName.Buffer[k], k);
            }

            ShortName.Length = 0;
            RtlGenerate8dot3Name(&LongName, TRUE, &Context, &ShortName);
            RtlInitUnicodeString(&Expected, ExShortNames2[j][i]);
            ok(RtlEqualUnicodeString(&Expected, &ShortName, FALSE), "%u,%u:: Generated: %.*S. Expected: %.*S\n", j, i, ShortName.Length / sizeof(WCHAR), ShortName.Buffer, Expected.Length / sizeof(WCHAR), Expected.Buffer);
            if (!RtlEqualUnicodeString(&Expected, &ShortName, FALSE))
            {
                for (int k = 0; k < (ShortName.Length / sizeof(WCHAR)); k++)
                    trace("Got \\u%04x at %d\n", ShortName.Buffer[k], k);
            }
        }
    }

    {
        WCHAR Buffer[12];
        GENERATE_NAME_CONTEXT Context;
        UNICODE_STRING LongName, ShortName, Expected;

        ShortName.Buffer = Buffer;
        ShortName.MaximumLength = sizeof(Buffer);

        for (i = 0; i < LONG_NAMES_COUNT; ++i)
        {
            if (i % 6 == 0) RtlZeroMemory(&Context, sizeof(GENERATE_NAME_CONTEXT));

            RtlInitUnicodeString(&LongName, LongNames[i]);
            ShortName.Length = 0;

            RtlGenerate8dot3Name(&LongName, FALSE, &Context, &ShortName);
            RtlInitUnicodeString(&Expected, LongShortNames[i]);
            ok(RtlEqualUnicodeString(&Expected, &ShortName, FALSE), "%u:: Generated: %.*S. Expected: %.*S\n", i, ShortName.Length / sizeof(WCHAR), ShortName.Buffer, Expected.Length / sizeof(WCHAR), Expected.Buffer);
        }
    }
}
