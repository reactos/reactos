/*
 * PROJECT:   registry manipulation library
 * LICENSE:   GPL - See COPYING in the top level directory
 * COPYRIGHT: Copyright 2005 Filip Navara <navaraf@reactos.org>
 *            Copyright 2001 - 2005 Eric Kohl
 */

#include "cmlib.h"
#define NDEBUG
#include <debug.h>

BOOLEAN
NTAPI
CmCompareHash(
	IN PCUNICODE_STRING KeyName,
	IN PCHAR HashString,
	IN BOOLEAN CaseInsensitive)
{
	CHAR Buffer[4];

	Buffer[0] = (KeyName->Length >= 2) ? (CHAR)KeyName->Buffer[0] : 0;
	Buffer[1] = (KeyName->Length >= 4) ? (CHAR)KeyName->Buffer[1] : 0;
	Buffer[2] = (KeyName->Length >= 6) ? (CHAR)KeyName->Buffer[2] : 0;
	Buffer[3] = (KeyName->Length >= 8) ? (CHAR)KeyName->Buffer[3] : 0;

    if (CaseInsensitive)
    {
        return (strncasecmp(Buffer, HashString, 4) == 0);
    }
    else
    {
        return (strncmp(Buffer, HashString, 4) == 0);
    }
}

BOOLEAN
NTAPI
CmComparePackedNames(
    IN PCUNICODE_STRING CompareName,
    IN PVOID Name,
    IN USHORT NameLength,
    IN BOOLEAN NamePacked,
    IN BOOLEAN CaseInsensitive)
{
    ULONG i;

    if (NamePacked)
    {
        PUCHAR PackedName = (PUCHAR)Name;

        if (CompareName->Length != NameLength * sizeof(WCHAR))
        {
            //DPRINT1("Length doesn'T match %lu / %lu\n", CompareName->Length, NameLength);
            return FALSE;
        }

        if (CaseInsensitive)
        {
            for (i = 0; i < CompareName->Length / sizeof(WCHAR); i++)
            {
                //DbgPrint("%c/%c,",
                //         RtlUpcaseUnicodeChar(CompareName->Buffer[i]),
                //         RtlUpcaseUnicodeChar(PackedName[i]));
                if (RtlUpcaseUnicodeChar(CompareName->Buffer[i]) !=
                    RtlUpcaseUnicodeChar(PackedName[i]))
                {
                    //DbgPrint("\nFailed!\n");
                    return FALSE;
                }
            }
            //DbgPrint("\nSuccess!\n");
        }
        else
        {
            for (i = 0; i < CompareName->Length / sizeof(WCHAR); i++)
            {
                if (CompareName->Buffer[i] != PackedName[i])
                    return FALSE;
            }
        }

    }
    else
    {
        PWCHAR UnicodeName = (PWCHAR)Name;

        if (CompareName->Length != NameLength)
            return FALSE;

        if (CaseInsensitive)
        {
            for (i = 0; i < CompareName->Length / sizeof(WCHAR); i++)
            {
                if (RtlUpcaseUnicodeChar(CompareName->Buffer[i]) !=
                    RtlUpcaseUnicodeChar(UnicodeName[i]))
                    return FALSE;
            }
        }
        else
        {
            for (i = 0; i < CompareName->Length / sizeof(WCHAR); i++)
            {
                if (CompareName->Buffer[i] != UnicodeName[i])
                    return FALSE;
            }
        }
    }

    return TRUE;
}


BOOLEAN
NTAPI
CmCompareKeyName(
    IN PCM_KEY_NODE KeyNode,
    IN PCUNICODE_STRING KeyName,
    IN BOOLEAN CaseInsensitive)
{
    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);
    return CmComparePackedNames(KeyName,
                                KeyNode->Name,
                                KeyNode->NameLength,
                                (KeyNode->Flags & KEY_COMP_NAME) ? TRUE : FALSE,
                                CaseInsensitive);
}

BOOLEAN
NTAPI
CmCompareKeyValueName(
    IN PCM_KEY_VALUE ValueCell,
    IN PCUNICODE_STRING ValueName,
    IN BOOLEAN CaseInsensitive)
{
    ASSERT(ValueCell->Signature == CM_KEY_VALUE_SIGNATURE);
    return CmComparePackedNames(ValueName,
                                ValueCell->Name,
                                ValueCell->NameLength,
                                (ValueCell->Flags & VALUE_COMP_NAME) ? TRUE : FALSE,
                                CaseInsensitive);
}

ULONG
NTAPI
CmCopyPackedName(
    _Out_ PWCHAR Buffer,
    _In_ ULONG BufferLength,
    _In_ PVOID Name,
    _In_ USHORT NameLength,
    _In_ BOOLEAN NamePacked)
{
    ULONG CharCount, i;
    ASSERT(Name != 0);
    ASSERT(NameLength != 0);

    if (NamePacked)
    {
        NameLength *= sizeof(WCHAR);
        CharCount = min(BufferLength, NameLength) / sizeof(WCHAR);

        if (Buffer != NULL)
        {
            PUCHAR PackedName = (PUCHAR)Name;

            for (i = 0; i < CharCount; i++)
            {
                Buffer[i] = PackedName[i];
            }
        }
    }
    else
    {
        CharCount = min(BufferLength, NameLength) / sizeof(WCHAR);

        if (Buffer != NULL)
        {
            PWCHAR UnicodeName = (PWCHAR)Name;

            for (i = 0; i < CharCount; i++)
            {
                Buffer[i] = UnicodeName[i];
            }
        }
    }

    if (BufferLength >= NameLength + sizeof(WCHAR))
    {
        Buffer[NameLength / sizeof(WCHAR)] = '\0';
    }

    return NameLength + sizeof(WCHAR);
}

ULONG
NTAPI
CmCopyKeyName(
    _In_ PCM_KEY_NODE KeyNode,
    _Out_ PWCHAR KeyNameBuffer,
    _Inout_ ULONG BufferLength)
{
    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);
    return CmCopyPackedName(KeyNameBuffer,
                            BufferLength,
                            KeyNode->Name,
                            KeyNode->NameLength,
                            (KeyNode->Flags & KEY_COMP_NAME) ? TRUE : FALSE);
}

ULONG
NTAPI
CmCopyKeyValueName(
    _In_ PCM_KEY_VALUE ValueCell,
    _Out_ PWCHAR ValueNameBuffer,
    _Inout_ ULONG BufferLength)
{
    ASSERT(ValueCell->Signature == CM_KEY_VALUE_SIGNATURE);
    return CmCopyPackedName(ValueNameBuffer,
                            BufferLength,
                            ValueCell->Name,
                            ValueCell->NameLength,
                            (ValueCell->Flags & VALUE_COMP_NAME) ? TRUE : FALSE);
}
