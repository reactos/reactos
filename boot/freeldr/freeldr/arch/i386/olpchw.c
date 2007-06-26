/* $Id: olpchw.c 21339 2006-03-18 22:09:16Z peterw $
 *
 *  FreeLoader
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

#define NDEBUG
#include <debug.h>

VOID
DetectPciIrqRoutingTable(FRLDRHKEY BusKey);

VOID
OlpcDetectPciBios(FRLDRHKEY SystemKey, ULONG *BusNumber)
{
    PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor;
    WCHAR Buffer[80];
    FRLDRHKEY BiosKey;
    ULONG Size;
    LONG Error;

    /* Report the PCI BIOS */
    if (TRUE/*FindPciBios(&BusData)*/)
    {
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

        /* Set 'Component Information' */
        SetComponentInformation(BiosKey,
            0x0,
            0x0,
            0xFFFFFFFF);

        /* Increment bus number */
        (*BusNumber)++;

        /* Set 'Identifier' value */
        Error = RegSetValue(BiosKey,
            L"Identifier",
            REG_SZ,
            (PCHAR)L"PCI BIOS",
            9 * sizeof(WCHAR));
        if (Error != ERROR_SUCCESS)
        {
            DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
            return;
        }

        /* Set 'Configuration Data' value */
        Size = sizeof(CM_FULL_RESOURCE_DESCRIPTOR);
        FullResourceDescriptor = MmAllocateMemory(Size);
        if (FullResourceDescriptor == NULL)
        {
            DbgPrint((DPRINT_HWDETECT,
                "Failed to allocate resource descriptor\n"));
            return;
        }

        /* Initialize resource descriptor */
        memset(FullResourceDescriptor, 0, Size);
        FullResourceDescriptor->InterfaceType = PCIBus;
        FullResourceDescriptor->BusNumber = 0;
        FullResourceDescriptor->PartialResourceList.Version = 1;
        FullResourceDescriptor->PartialResourceList.Revision = 1;
        FullResourceDescriptor->PartialResourceList.Count = 1;
        FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].Type = CmResourceTypeBusNumber;
        FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].ShareDisposition = CmResourceShareDeviceExclusive;
        FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].u.BusNumber.Start = 0;
        FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].u.BusNumber.Length = 1;

        /* Set 'Configuration Data' value */
        Error = RegSetValue(BiosKey,
            L"Configuration Data",
            REG_FULL_RESOURCE_DESCRIPTOR,
            (PCHAR) FullResourceDescriptor,
            Size);
        MmFreeMemory(FullResourceDescriptor);
        if (Error != ERROR_SUCCESS)
        {
            DbgPrint((DPRINT_HWDETECT,
                "RegSetValue(Configuration Data) failed (Error %u)\n",
                (int)Error));
            return;
        }

        DetectPciIrqRoutingTable(BiosKey);
    }
}



VOID OlpcHwDetect()
{
    FRLDRHKEY SystemKey;
    ULONG BusNumber = 0;
    LONG Error;

    ofwprintf("OlpcHwDetect()\n");

    /* Create the 'System' key */
    Error = RegCreateKey(NULL,
        L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System",
        &SystemKey);
    if (Error != ERROR_SUCCESS)
    {
        DbgPrint((DPRINT_HWDETECT, "RegCreateKey() failed (Error %u)\n", (int)Error));
        return;
    }

    /* Detect buses */
    OlpcDetectPciBios(SystemKey, &BusNumber);
    //DetectApmBios(SystemKey, &BusNumber);
    //DetectPnpBios(SystemKey, &BusNumber);
    //DetectIsaBios(SystemKey, &BusNumber);
    //DetectAcpiBios(SystemKey, &BusNumber);

    ofwprintf("DetectHardware() Done\n");
}
