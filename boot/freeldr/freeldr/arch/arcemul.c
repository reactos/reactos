/*
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <freeldr.h>

MACHVTBL MachVtbl;


/* ARC FUNCTIONS **************************************************************/

TIMEINFO*
ArcGetTime(VOID)
{
    return MachVtbl.GetTime();
}

ULONG
ArcGetRelativeTime(VOID)
{
    TIMEINFO* TimeInfo;
    ULONG ret;

    TimeInfo = ArcGetTime();
    ret = ((TimeInfo->Hour * 24) + TimeInfo->Minute) * 60 + TimeInfo->Second;
    return ret;
}

PCONFIGURATION_COMPONENT_DATA
MachHwDetect(_In_opt_ PCSTR Options)
{
    return MachVtbl.HwDetect(Options);
}

VOID MachPrepareForReactOS(VOID)
{
    MachVtbl.PrepareForReactOS();
}

VOID MachGetExtendedBIOSData(PULONG ExtendedBIOSDataArea, PULONG ExtendedBIOSDataSize)
{
    MachVtbl.GetExtendedBIOSData(ExtendedBIOSDataArea, ExtendedBIOSDataSize);
}

VOID MachVideoGetFontsFromFirmware(PULONG RomFontPointers)
{
    MachVtbl.VideoGetFontsFromFirmware(RomFontPointers);
}

ULONG
MachGetBootSectorLoadAddress(IN UCHAR DriveNumber)
{
#if defined(SARCH_PC98)
    return Pc98GetBootSectorLoadAddress(DriveNumber);
#else
    return 0x7C00;
#endif
}

/* EOF */
