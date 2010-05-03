/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmboot.c
 * PURPOSE:         Configuration Manager - Boot Initialization
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#define NDEBUG
#include "debug.h"

/* GLOBALS *******************************************************************/

/* FUNCTIONS *****************************************************************/

HCELL_INDEX
NTAPI
CmpFindControlSet(IN PHHIVE SystemHive,
                  IN HCELL_INDEX RootCell,
                  IN PUNICODE_STRING SelectKeyName,
                  OUT PBOOLEAN AutoSelect)
{
    UNICODE_STRING KeyName;
    PCM_KEY_NODE Node;
    HCELL_INDEX SelectCell, AutoSelectCell, SelectValueCell, ControlSetCell;
    HCELL_INDEX CurrentValueCell;
    PCM_KEY_VALUE KeyValue;
    ULONG Length;
    PULONG ControlSetId;
    ANSI_STRING ControlSetAnsiName;
    CHAR Buffer[128];
    WCHAR WideBuffer[128];
    NTSTATUS Status;
    PULONG CurrentData;

    /* Sanity check */
    ASSERT(SystemHive->ReleaseCellRoutine == NULL);

    /* Get the Select subkey */
    RtlInitUnicodeString(&KeyName, L"select");
    Node = (PCM_KEY_NODE)HvGetCell(SystemHive, RootCell);
    if (!Node) return HCELL_NIL;
    SelectCell = CmpFindSubKeyByName(SystemHive, Node, &KeyName);
    if (SelectCell == HCELL_NIL) return SelectCell;

    /* Get AutoSelect value */
    RtlInitUnicodeString(&KeyName, L"AutoSelect");
    Node = (PCM_KEY_NODE)HvGetCell(SystemHive, SelectCell);
    if (!Node) return HCELL_NIL;
    AutoSelectCell = CmpFindValueByName(SystemHive, Node, &KeyName);
    if (AutoSelectCell == HCELL_NIL)
    {
        /* Assume TRUE if the value is missing. */
        *AutoSelect = TRUE;
    }
    else
    {
        /* Read the value */
        KeyValue = (PCM_KEY_VALUE)HvGetCell(SystemHive, AutoSelectCell);
        if (KeyValue == NULL) return HCELL_NIL;

        /* Convert it to a boolean */
        *AutoSelect = *(PBOOLEAN)CmpValueToData(SystemHive, KeyValue, &Length);
    }

    /* Now find the control set being looked up */
    Node = (PCM_KEY_NODE)HvGetCell(SystemHive, SelectCell);
    if (!Node) return HCELL_NIL;
    SelectValueCell = CmpFindValueByName(SystemHive, Node, SelectKeyName);
    if (SelectValueCell == HCELL_NIL) return SelectValueCell;

    /* Read the value (corresponding to the CCS ID) */
    KeyValue = (PCM_KEY_VALUE)HvGetCell(SystemHive, SelectValueCell);
    if (!KeyValue) return HCELL_NIL;
    if (KeyValue->Type != REG_DWORD) return HCELL_NIL;
    ControlSetId = (PULONG)CmpValueToData(SystemHive, KeyValue, &Length);

    /* Now build an Ansi String for the CCS's Name */
    sprintf(Buffer, "ControlSet%03lu", *ControlSetId);
    ControlSetAnsiName.Length = (USHORT)strlen(Buffer);
    ControlSetAnsiName.MaximumLength = (USHORT)strlen(Buffer);
    ControlSetAnsiName.Buffer = Buffer;

    /* And convert it to Unicode... */
    KeyName.MaximumLength = 256;
    KeyName.Buffer = WideBuffer;
    Status = RtlAnsiStringToUnicodeString(&KeyName,
                                          &ControlSetAnsiName,
                                          FALSE);
    if (!NT_SUCCESS(Status)) return HCELL_NIL;

    /* Now open it */
    Node = (PCM_KEY_NODE)HvGetCell(SystemHive, RootCell);
    if (!Node) return HCELL_NIL;
    ControlSetCell = CmpFindSubKeyByName(SystemHive, Node, &KeyName);
    if (ControlSetCell == HCELL_NIL) return ControlSetCell;

    /* Get the value of the "Current" CCS */
    RtlInitUnicodeString(&KeyName, L"Current");
    Node =  (PCM_KEY_NODE)HvGetCell(SystemHive, SelectCell);
    if (!Node) return HCELL_NIL;
    CurrentValueCell = CmpFindValueByName(SystemHive, Node, &KeyName);

    /* Make sure it exists */
    if (CurrentValueCell != HCELL_NIL)
    {
        /* Get the current value and make sure its a ULONG */
        KeyValue = (PCM_KEY_VALUE)HvGetCell(SystemHive, CurrentValueCell);
        if (!KeyValue) return HCELL_NIL;
        if (KeyValue->Type == REG_DWORD)
        {
            /* Get the data and update it */
            CurrentData = (PULONG)CmpValueToData(SystemHive,
                                                 KeyValue,
                                                 &Length);
            if (!CurrentData) return HCELL_NIL;
            *CurrentData = *ControlSetId;
        }
    }

    /* Return the CCS Cell */
    return ControlSetCell;
}
