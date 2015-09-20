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

PWSTR Names[] = { L"Menu Démarrer", L"Sélecteur de configuration clavier.lnk", L"éèàùç.txt", L"éèàùçeeauc.txt" };
PWSTR ShortNames1[] = { L"MENUDM~1", L"SLECTE~1.LNK", L"5C2D~1.TXT", L"EEAUC~1.TXT" };
PWSTR ShortNames2[] = { L"MENUDM~2", L"SLECTE~2.LNK", L"5C2D~2.TXT", L"EEAUC~2.TXT" };

START_TEST(RtlGenerate8dot3Name)
{
    USHORT i;

    for (i = 0; i < 4; ++i)
    {
        WCHAR Buffer[12];
        GENERATE_NAME_CONTEXT Context;
        UNICODE_STRING LongName, ShortName, Expected;

        RtlZeroMemory(&Context, sizeof(GENERATE_NAME_CONTEXT));
        RtlInitUnicodeString(&LongName, Names[i]);
        ShortName.Buffer = Buffer;
        ShortName.Length = sizeof(Buffer);
        ShortName.MaximumLength = sizeof(Buffer);

        RtlGenerate8dot3Name(&LongName, FALSE, &Context, &ShortName);
        RtlInitUnicodeString(&Expected, ShortNames1[i]);
        ok(RtlEqualUnicodeString(&Expected, &ShortName, FALSE), "Generated: %.*S. Expected: %.*S\n", ShortName.Length, ShortName.Buffer, Expected.Length, Expected.Buffer);

        RtlGenerate8dot3Name(&LongName, FALSE, &Context, &ShortName);
        RtlInitUnicodeString(&Expected, ShortNames2[i]);
        ok(RtlEqualUnicodeString(&Expected, &ShortName, FALSE), "Generated: %.*S. Expected: %.*S\n", ShortName.Length, ShortName.Buffer, Expected.Length, Expected.Buffer);
    }
}
