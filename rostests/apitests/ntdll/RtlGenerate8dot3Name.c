/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for RtlGenerate8dot3Name
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <ndk/rtlfuncs.h>

NTSYSAPI
VOID
NTAPI
RtlGenerate8dot3Name(
  _In_ PCUNICODE_STRING Name,
  _In_ BOOLEAN AllowExtendedCharacters,
  _Inout_ PGENERATE_NAME_CONTEXT Context,
  _Inout_ PUNICODE_STRING Name8dot3);

PWSTR Names[] = { L"Menu Démarrer", L"Sélecteur de configuration clavier.lnk", L"éèàùç.txt", L"éèàùçeeauc.txt", L"Long file name.txt", L"Long file name", L"Longfilename.txt", L"Longfilename" };
PWSTR ShortNames1[] = { L"MENUDM~1", L"SLECTE~1.LNK", L"5C2D~1.TXT", L"EEAUC~1.TXT", L"LONGFI~1.TXT", L"LONGFI~1", L"LONGFI~1.TXT", L"LONGFI~1" };
PWSTR ShortNames2[] = { L"MENUDM~2", L"SLECTE~2.LNK", L"5C2D~2.TXT", L"EEAUC~2.TXT", L"LONGFI~2.TXT", L"LONGFI~2", L"LONGFI~2.TXT", L"LONGFI~2" };
PWSTR ExShortNames1[] = { L"MENUDÉ~1", L"SÉLECT~1.LNK", L"ÉÈÀÙÇ~1.TXT", L"ÉÈÀÙÇE~1.TXT", L"LONGFI~1.TXT", L"LONGFI~1", L"LONGFI~1.TXT", L"LONGFI~1" };
PWSTR ExShortNames2[] = { L"MENUDÉ~2", L"SÉLECT~2.LNK", L"ÉÈÀÙÇ~2.TXT", L"ÉÈÀÙÇE~2.TXT", L"LONGFI~2.TXT", L"LONGFI~2", L"LONGFI~2.TXT", L"LONGFI~2" };

START_TEST(RtlGenerate8dot3Name)
{
    USHORT i;

    for (i = 0; i < 8; ++i)
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
        ok(RtlEqualUnicodeString(&Expected, &ShortName, FALSE), "Generated: %.*S. Expected: %.*S\n", ShortName.Length, ShortName.Buffer, Expected.Length, Expected.Buffer);

        ShortName.Length = 0;
        RtlGenerate8dot3Name(&LongName, FALSE, &Context, &ShortName);
        RtlInitUnicodeString(&Expected, ShortNames2[i]);
        ok(RtlEqualUnicodeString(&Expected, &ShortName, FALSE), "Generated: %.*S. Expected: %.*S\n", ShortName.Length, ShortName.Buffer, Expected.Length, Expected.Buffer);

        RtlZeroMemory(&Context, sizeof(GENERATE_NAME_CONTEXT));
        ShortName.Length = 0;
        RtlGenerate8dot3Name(&LongName, TRUE, &Context, &ShortName);
        RtlInitUnicodeString(&Expected, ExShortNames1[i]);
        ok(RtlEqualUnicodeString(&Expected, &ShortName, FALSE), "Generated: %.*S. Expected: %.*S\n", ShortName.Length, ShortName.Buffer, Expected.Length, Expected.Buffer);

        ShortName.Length = 0;
        RtlGenerate8dot3Name(&LongName, TRUE, &Context, &ShortName);
        RtlInitUnicodeString(&Expected, ExShortNames2[i]);
        ok(RtlEqualUnicodeString(&Expected, &ShortName, FALSE), "Generated: %.*S. Expected: %.*S\n", ShortName.Length, ShortName.Buffer, Expected.Length, Expected.Buffer);
    }
}
