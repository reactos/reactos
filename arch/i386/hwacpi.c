/*
 *  FreeLoader
 *
 *  Copyright (C) 2004  Eric Kohl
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <freeldr.h>
#include <debug.h>

BOOLEAN AcpiPresent = FALSE;

static BOOLEAN
FindAcpiBios(VOID)
{
    PUCHAR Ptr;

    /* Find the 'Root System Descriptor Table Pointer' */
    Ptr = (PUCHAR)0xE0000;
    while ((ULONG)Ptr < 0x100000)
    {
        if (!memcmp(Ptr, "RSD PTR ", 8))
        {
            DbgPrint((DPRINT_HWDETECT, "ACPI supported\n"));

            return TRUE;
        }

        Ptr = (PUCHAR)((ULONG)Ptr + 0x10);
    }

    DbgPrint((DPRINT_HWDETECT, "ACPI not supported\n"));

    return FALSE;
}


VOID
DetectAcpiBios(FRLDRHKEY SystemKey, ULONG *BusNumber)
{
    WCHAR Buffer[80];
    FRLDRHKEY BiosKey;
    LONG Error;

    if (FindAcpiBios())
    {
        AcpiPresent = TRUE;
        /* Create new bus key */
        swprintf(Buffer,
                 L"MultifunctionAdapter\\%u", *BusNumber);
        Error = RegCreateKey(SystemKey,
                             Buffer,
                             &BiosKey);
        if (Error != ERROR_SUCCESS)
        {
            DbgPrint((DPRINT_HWDETECT, "RegCreateKey() failed (Error %u)\n", (int)Error));
            return;
        }

#if 0
      /* Set 'Component Information' */
      SetComponentInformation(BiosKey,
                              0x0,
                              0x0,
                              0xFFFFFFFF);
#endif

        /* Increment bus number */
        (*BusNumber)++;

        /* Set 'Identifier' value */
        Error = RegSetValue(BiosKey,
                            L"Identifier",
                            REG_SZ,
                            (PCHAR)L"ACPI BIOS",
                            10 * sizeof(WCHAR));
        if (Error != ERROR_SUCCESS)
        {
            DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
            return;
        }

    }
}

/* EOF */
