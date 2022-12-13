/*
 * PROJECT:     ReactOS HID Parser Library
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     HID Parser usages helpers
 * COPYRIGHT:   Copyright  Michael Martin <michael.martin@reactos.org>
 *              Copyright  Johannes Anderwald <johannes.anderwald@reactos.org>
 */

#include "hidparser.h"
#include "hidpmem.h"

#define NDEBUG
#include <debug.h>

static ULONG KeyboardScanCodes[256] =
{ /*    0       1       2       3       4       5       6       7       8       9       A       B       C       D       E       F */
/* 0 */ 0x0000, 0x0000, 0x0000, 0x0000, 0x001e, 0x0030, 0x002e, 0x0020, 0x0012, 0x0021, 0x0022, 0x0023, 0x0017, 0x0024, 0x0025, 0x0026,
/* 1 */ 0x0032, 0x0031, 0x0018, 0x0019, 0x0010, 0x0013, 0x001f, 0x0014, 0x0016, 0x002f, 0x0011, 0x002d, 0x0015, 0x002c, 0x0002, 0x0003,
/* 2 */ 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000a, 0x000b, 0x001c, 0x0001, 0x000e, 0x000f, 0x0039, 0x000c, 0x000d, 0x001a,
/* 3 */ 0x001b, 0x002b, 0x002b, 0x0027, 0x0028, 0x0029, 0x0033, 0x0034, 0x0035, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f, 0x0040,
/* 4 */ 0x0041, 0x0042, 0x0043, 0x0044, 0x0057, 0x0058, 0xE037, 0x0046, 0x0045, 0xE052, 0xE047, 0xE049, 0xE053, 0xE04F, 0xE051, 0xE04D,
/* 5 */ 0xE04B, 0xE050, 0xE048, 0x0045, 0xE035, 0x0037, 0x004a, 0x004e, 0xE01C, 0x004f, 0x0050, 0x0051, 0x004b, 0x004c, 0x004d, 0x0047,
/* 6 */ 0x0048, 0x0049, 0x0052, 0x0053, 0x0056, 0xE05D, 0xE05E, 0x0075, 0x00b7, 0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be,
/* 7 */ 0x00bf, 0x00c0, 0x00c1, 0x00c2, 0x0086, 0x008a, 0x0082, 0x0084, 0x0080, 0x0081, 0x0083, 0x0089, 0x0085, 0x0087, 0x0088, 0x0071,
/* 8 */ 0x0073, 0x0072, 0x0000, 0x0000, 0x0000, 0x0079, 0x0000, 0x0059, 0x005d, 0x007c, 0x005c, 0x005e, 0x005f, 0x0000, 0x0000, 0x0000,
/* 9 */ 0x007a, 0x007b, 0x005a, 0x005b, 0x0055, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/* A */ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/* B */ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/* C */ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/* D */ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
/* E */ 0x001D, 0x002A, 0x0038, 0xE05B, 0xE01D, 0x0036, 0xE038, 0xE05C, 0x00a4, 0x00a6, 0x00a5, 0x00a3, 0x00a1, 0x0073, 0x0072, 0x0071,
/* F */ 0x0096, 0x009e, 0x009f, 0x0080, 0x0088, 0x00b1, 0x00b2, 0x00b0, 0x008e, 0x0098, 0x00ad, 0x008c, 0x0000, 0x0000, 0x0000, 0x0000,
};

static struct
{
    USAGE Usage;
    ULONG ScanCode;
} CustomerScanCodes[] =
{
    { 0x00B5, 0xE019 },
    { 0x00B6, 0xE010 },
    { 0x00B7, 0xE024 },
    { 0x00CD, 0xE022 },
    { 0x00E2, 0xE020 },
    { 0x00E9, 0xE030 },
    { 0x00EA, 0xE02E },
    { 0x0183, 0xE06D },
    { 0x018A, 0xE06C },
    { 0x0192, 0xE021 },
    { 0x0194, 0xE06B },
    { 0x0221, 0xE065 },
    { 0x0223, 0xE032 },
    { 0x0224, 0xE06A },
    { 0x0225, 0xE069 },
    { 0x0226, 0xE068 },
    { 0x0227, 0xE067 },
    { 0x022A, 0xE066 },
};

#define NTOHS(n) (((((unsigned short)(n) & 0xFF)) << 8) | (((unsigned short)(n) & 0xFF00) >> 8))

ULONG
HidParser_GetScanCodeFromKbdUsage(
    IN USAGE Usage)
{
    if (Usage < sizeof(KeyboardScanCodes) / sizeof(KeyboardScanCodes[0]))
    {
        // valid usage
        return KeyboardScanCodes[Usage];
    }

    // invalid usage
    return 0;
}

ULONG
HidParser_GetScanCodeFromCustUsage(
    IN USAGE Usage)
{
    ULONG i;

    // find usage in array
    for (i = 0; i < sizeof(CustomerScanCodes) / sizeof(CustomerScanCodes[0]); ++i)
    {
        if (CustomerScanCodes[i].Usage == Usage)
        {
            // valid usage
            return CustomerScanCodes[i].ScanCode;
        }
    }

    // invalid usage
    return 0;
}

VOID
HidParser_DispatchKey(
    IN PCHAR ScanCodes,
    IN HIDP_KEYBOARD_DIRECTION KeyAction,
    IN PHIDP_INSERT_SCANCODES InsertCodesProcedure,
    IN PVOID InsertCodesContext)
{
    ULONG index;
    ULONG length = 0;

    // count code length
    for(index = 0; index < sizeof(ULONG); index++)
    {
        if (ScanCodes[index] == 0)
        {
            // last scan code
            break;
        }

        // is this a key break
        if (KeyAction == HidP_Keyboard_Break)
        {
            // add break - see USB HID to PS/2 Scan Code Translation Table
            ScanCodes[index] |= 0x80;
        }

        // more scan counts
        length++;
    }

    if (length > 0)
    {
         // dispatch scan codes
         InsertCodesProcedure(InsertCodesContext, ScanCodes, length);
    }
}

NTSTATUS
HidParser_TranslateKbdUsage(
    IN USAGE Usage,
    IN HIDP_KEYBOARD_DIRECTION  KeyAction,
    IN OUT PHIDP_KEYBOARD_MODIFIER_STATE  ModifierState,
    IN PHIDP_INSERT_SCANCODES  InsertCodesProcedure,
    IN PVOID  InsertCodesContext)
{
    ULONG scanCode;
    CHAR fakeShift[] = {0xE0, 0x2A, 0x00};
    CHAR fakeCtrl[] = {0xE1, 0x1D, 0x00};

    // get scan code
    scanCode = HidParser_GetScanCodeFromKbdUsage(Usage);
    if (!scanCode)
    {
        // invalid lookup or no scan code available
        DPRINT("No Scan code for Usage %x\n", Usage);
        return HIDP_STATUS_I8042_TRANS_UNKNOWN;
    }

    if (scanCode & 0xFF00)
    {
        // swap scan code
        scanCode = NTOHS(scanCode);
    }

    if (Usage == 0x46 && KeyAction == HidP_Keyboard_Make)
    {
        // Print Screen generates additional FakeShift
        HidParser_DispatchKey(fakeShift, KeyAction, InsertCodesProcedure, InsertCodesContext);
    }

    if (Usage == 0x48)
    {
        // Pause/Break generates additional FakeCtrl. Note: it's always before key press/release.
        HidParser_DispatchKey(fakeCtrl, KeyAction, InsertCodesProcedure, InsertCodesContext);
    }

    // FIXME: translate modifier states
    HidParser_DispatchKey((PCHAR)&scanCode, KeyAction, InsertCodesProcedure, InsertCodesContext);

    if (Usage == 0x46 && KeyAction == HidP_Keyboard_Break)
    {
        // Print Screen generates additional FakeShift
        HidParser_DispatchKey(fakeShift, KeyAction, InsertCodesProcedure, InsertCodesContext);
    }

    return HIDP_STATUS_SUCCESS;
}

NTSTATUS
HidParser_TranslateCustUsage(
    IN USAGE Usage,
    IN HIDP_KEYBOARD_DIRECTION  KeyAction,
    IN OUT PHIDP_KEYBOARD_MODIFIER_STATE  ModifierState,
    IN PHIDP_INSERT_SCANCODES  InsertCodesProcedure,
    IN PVOID  InsertCodesContext)
{
    ULONG scanCode;

    // get scan code
    scanCode = HidParser_GetScanCodeFromCustUsage(Usage);
    if (!scanCode)
    {
        // invalid lookup or no scan code available
        DPRINT("No Scan code for Usage %x\n", Usage);
        return HIDP_STATUS_I8042_TRANS_UNKNOWN;
    }

    if (scanCode & 0xFF00)
    {
        // swap scan code
        scanCode = NTOHS(scanCode);
    }

    // FIXME: translate modifier states
    HidParser_DispatchKey((PCHAR)&scanCode, KeyAction, InsertCodesProcedure, InsertCodesContext);

    return HIDP_STATUS_SUCCESS;
}

HIDAPI
NTSTATUS
NTAPI
HidParser_UsageListDifference(
    IN PUSAGE PreviousUsageList,
    IN PUSAGE CurrentUsageList,
    OUT PUSAGE BreakUsageList,
    OUT PUSAGE MakeUsageList,
    IN ULONG UsageListLength)
{
    ULONG index;
    ULONG subIndex;
    ULONG bFound;
    ULONG breakUsageIndex = 0;
    ULONG makeUsageIndex = 0;
    USAGE currentUsage, usage;

    if (UsageListLength)
    {
        index = 0;
        do
        {
            /* get current usage */
            currentUsage = PreviousUsageList[index];

            /* is the end of list reached? */
            if (!currentUsage)
                break;

            /* start searching in current usage list */
            subIndex = 0;
            bFound = FALSE;
            do
            {
                /* get usage of current list */
                usage = CurrentUsageList[subIndex];

                /* end of list reached? */
                if (!usage)
                    break;

                /* check if it matches the current one */
                if (currentUsage == usage)
                {
                    /* it does */
                    bFound = TRUE;
                    break;
                }

                /* move to next usage */
                subIndex++;
            }
            while (subIndex < UsageListLength);

            /* was the usage found ?*/
            if (!bFound)
            {
                /* store it in the break usage list */
                BreakUsageList[breakUsageIndex] = currentUsage;
                breakUsageIndex++;
            }

            /* move to next usage */
            index++;

        }
        while (index < UsageListLength);

        /* now process the new items */
        index = 0;
        do
        {
            /* get current usage */
            currentUsage = CurrentUsageList[index];

            /* is the end of list reached? */
            if (!currentUsage)
                break;

            /* start searching in current usage list */
            subIndex = 0;
            bFound = FALSE;
            do
            {
                /* get usage of previous list */
                usage = PreviousUsageList[subIndex];

                /* end of list reached? */
                if (!usage)
                    break;

                /* check if it matches the current one */
                if (currentUsage == usage)
                {
                    /* it does */
                    bFound = TRUE;
                    break;
                }

                /* move to next usage */
                subIndex++;
            }
            while (subIndex < UsageListLength);

            /* was the usage found ?*/
            if (!bFound)
            {
                /* store it in the make usage list */
                MakeUsageList[makeUsageIndex] = currentUsage;
                makeUsageIndex++;
            }

            /* move to next usage */
            index++;

        }
        while (index < UsageListLength);
    }

    /* does the break list contain empty entries */
    if (breakUsageIndex < UsageListLength)
    {
        /* zeroize entries */
        ZeroFunction(&BreakUsageList[breakUsageIndex], sizeof(USAGE) * (UsageListLength - breakUsageIndex));
    }

    /* does the make usage list contain empty entries */
    if (makeUsageIndex < UsageListLength)
    {
        /* zeroize entries */
        ZeroFunction(&MakeUsageList[makeUsageIndex], sizeof(USAGE) * (UsageListLength - makeUsageIndex));
    }

    /* done */
    return HIDP_STATUS_SUCCESS;
}

HIDAPI
NTSTATUS
NTAPI
HidParser_TranslateUsagesToI8042ScanCodes(
    IN PUSAGE  ChangedUsageList,
    IN ULONG  UsageListLength,
    IN HIDP_KEYBOARD_DIRECTION  KeyAction,
    IN OUT PHIDP_KEYBOARD_MODIFIER_STATE  ModifierState,
    IN PHIDP_INSERT_SCANCODES  InsertCodesProcedure,
    IN PVOID  InsertCodesContext)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}
