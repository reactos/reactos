/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmcontrl.c
 * PURPOSE:         Configuration Manager - Control Set Management
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#include "cm.h"
#define NDEBUG
#include "debug.h"

/* GLOBALS *******************************************************************/

/* FUNCTIONS *****************************************************************/

LANGID
NTAPI
CmpConvertLangId(IN LPWSTR Name,
                 IN ULONG NameLength)
{
    ULONG i;
    WCHAR p;
    LANGID LangId = 0;
    ULONG IdCode;

    /* Convert the length in chars and loop */
    NameLength = NameLength / sizeof(WCHAR);
    for (i = 0; i < NameLength; i++)
    {
        /* Get the character */
        p = Name[i];

        /* Handle each case */
        if ((p >= L'0') && (p <= L'9'))
        {
            /* Handle digits*/
            IdCode = p - L'0';
        }
        else if ((p >= L'A') && (p <= L'F'))
        {
            /* Handle upper-case letters */
            IdCode = p - L'A' + 10;
        }
        else if ((p >= L'a') && (p <= L'f'))
        {
            /* Handle lower-case letters */
            IdCode = p - L'a' + 10;
        }
        else
        {
            /* Unhandled case, return what we have till now */
            break;
        }

        /* If the ID Code is >= 16, then we're done */
        if (IdCode >= 16) break;

        /* Build the Language ID */
        LangId = (LangId << 4) | (LANGID)IdCode;
    }

    /* Return the Language ID */
    return LangId;
}

HCELL_INDEX
NTAPI
CmpWalkPath(IN PHHIVE SystemHive,
            IN HCELL_INDEX ParentCell,
            IN LPWSTR Path)
{
    UNICODE_STRING UnicodePath, NextName;
    BOOLEAN LastName;
    HCELL_INDEX CurrentCell = ParentCell;
    PCM_KEY_NODE Node;

    /* We shouldn't have a release routine at this point */
    ASSERT(SystemHive->ReleaseCellRoutine == NULL);

    /* Initialize the Unicode path and start looping */
    RtlInitUnicodeString(&UnicodePath, Path);
    while (TRUE)
    {
        /* Get the next name */
        CmpGetNextName(&UnicodePath, &NextName, &LastName);
        if (!NextName.Length) return CurrentCell;

        /* Get the subkey */
        Node = (PCM_KEY_NODE)HvGetCell(SystemHive, CurrentCell);
        if (!Node) return HCELL_NIL;
        CurrentCell = CmpFindSubKeyByName(SystemHive, Node, &NextName);
        if (CurrentCell == HCELL_NIL) return CurrentCell;
    }
}

VOID
NTAPI
CmGetSystemControlValues(IN PVOID SystemHiveData,
                         IN PCM_SYSTEM_CONTROL_VECTOR ControlVector)
{
    PHHIVE SystemHive = (PHHIVE)&CmControlHive;
    NTSTATUS Status;
    HCELL_INDEX RootCell, BaseCell, KeyCell, ValueCell;
    ULONG Length, DataSize;
    PCM_KEY_NODE Node;
    PCM_KEY_VALUE ValueData;
    UNICODE_STRING KeyName;
    BOOLEAN Auto, IsSmallKey;
    PVOID Buffer;

    /* LUDDDIIIICRROOOUUSSSS KI^H^H HACKKKK */
    if (!SystemHiveData) return;

    /* Initialize the Hive View List and the security cache */
    RtlZeroMemory(SystemHive, sizeof(SystemHive));
    CmpInitHiveViewList((PCMHIVE)SystemHive);
    CmpInitSecurityCache((PCMHIVE)SystemHive);

    /* Initialize the Hive */
    Status = HvInitialize(SystemHive,
                          HINIT_FLAT,
                          HIVE_VOLATILE,
                          HFILE_TYPE_PRIMARY,
                          SystemHiveData,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          1,
                          NULL);
    if (!NT_SUCCESS(Status)) KeBugCheckEx(BAD_SYSTEM_CONFIG_INFO, 1, 1, 0, 0);

    /* Sanity check, flat hives don't have release routines */
    ASSERT(SystemHive->ReleaseCellRoutine == NULL);

    /* FIXME: Prepare it */
    CmPrepareHive(SystemHive);

    /* Set the Root Cell */
    RootCell = ((PHBASE_BLOCK)SystemHiveData)->RootCell;

    /* Find the current control set */
    RtlInitUnicodeString(&KeyName, L"current");
    BaseCell = CmpFindControlSet(SystemHive, RootCell, &KeyName, &Auto);
    if (BaseCell == HCELL_NIL) KeBugCheckEx(BAD_SYSTEM_CONFIG_INFO, 1, 2, 0, 0);

    /*  Find the control subkey */
    RtlInitUnicodeString(&KeyName, L"control");
    Node = (PCM_KEY_NODE)HvGetCell(SystemHive, BaseCell);
    BaseCell = CmpFindSubKeyByName(SystemHive, Node, &KeyName);
    if (BaseCell == HCELL_NIL) KeBugCheckEx(BAD_SYSTEM_CONFIG_INFO,1 , 3, 0, 0);

    /* Loop each key */
    while (ControlVector->KeyPath)
    {
        /*  Assume failure */
        Length = -1;

        /* Get the cell for this key */
        KeyCell = CmpWalkPath(SystemHive, BaseCell, ControlVector->KeyPath);
        if (KeyCell != HCELL_NIL)
        {
            /* Now get the cell for the value */
            RtlInitUnicodeString(&KeyName, ControlVector->ValueName);
            Node = (PCM_KEY_NODE)HvGetCell(SystemHive, KeyCell);
            ValueCell = CmpFindValueByName(SystemHive, Node, &KeyName);
            if (ValueCell != HCELL_NIL)
            {
                /* Check if there's any data */
                if (!ControlVector->BufferLength)
                {
                    /* No, the buffer will only be large enough for a ULONG */
                    DataSize = sizeof(ULONG);
                }
                else
                {
                    /* Yes, save the data size */
                    DataSize = *ControlVector->BufferLength;
                }

                /* Get the actual data */
                ValueData = (PCM_KEY_VALUE)HvGetCell(SystemHive, ValueCell);

                /* Check if this is a small key */
                IsSmallKey = CmpIsKeyValueSmall(&Length, ValueData->DataLength);

                /* If the length is bigger then our buffer, normalize it */
                if (DataSize < Length) Length = DataSize;

                /* Make sure we have some data */
                if (Length > 0)
                {
                    /* Check if this was a small key */
                    if (IsSmallKey)
                    {
                        /* The buffer is directly safe to read */
                        Buffer = (PVOID)(&(ValueData->Data));
                    }
                    else
                    {
                        /* Use the longer path */
                        Buffer = (PVOID)HvGetCell(SystemHive, ValueData->Data);
                    }

                    /* Sanity check if this is a small key */
                    ASSERT((IsSmallKey ?
                           (Length <= CM_KEY_VALUE_SMALL) : TRUE));

                    /* Copy the data in the buffer */
                    RtlCopyMemory(ControlVector->Buffer, Buffer, Length);
                }

                /* Check if we should return the data type */
                if (ControlVector->Type)
                {
                    /* Return the type that we read */
                    *ControlVector->Type = ValueData->Type;
                }
            }
        }

        /* Return the size that we read */
        if (ControlVector->BufferLength) *ControlVector->BufferLength = Length;

        /* Go to the next entry */
        ControlVector++;
    }

    /* Check if the ID is in the registry */
    if (CmDefaultLanguageIdType == REG_SZ)
    {
        /* Read it */
        PsDefaultSystemLocaleId =
            (LCID)CmpConvertLangId(CmDefaultLanguageId,
                                   CmDefaultLanguageIdLength);
    }
    else
    {
        /* Use EN_US by default */
        PsDefaultSystemLocaleId = 0x409;
    }

    /* Check if the ID Is in the registry */
    if (CmInstallUILanguageIdType == REG_SZ)
    {
        /* Read it */
        PsInstallUILanguageId =  CmpConvertLangId(CmInstallUILanguageId,
                                                  CmInstallUILanguageIdLength);
    }
    else
    {
        /* Otherwise, use the default */
        PsInstallUILanguageId = LANGIDFROMLCID(PsDefaultSystemLocaleId);
    }

    /* Set the defaults for the Thread UI */
    PsDefaultThreadLocaleId = PsDefaultSystemLocaleId;
    PsDefaultUILanguageId = PsInstallUILanguageId;
}
