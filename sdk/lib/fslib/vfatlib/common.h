/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS VFAT filesystem library
 * FILE:        lib\fslib\vfatlib\common.h
 * PURPOSE:     Common code for Fat support
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Eric Kohl
 *              Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _VFATCOMMON_H_
#define _VFATCOMMON_H_

ULONG GetShiftCount(IN ULONG Value);
ULONG CalcVolumeSerialNumber(VOID);

NTSTATUS
FatWipeSectors(
    IN HANDLE FileHandle,
    IN ULONG TotalSectors,
    IN ULONG SectorsPerCluster,
    IN ULONG BytesPerSector,
    IN OUT PFORMAT_CONTEXT Context);

#endif /* _VFATCOMMON_H_ */

/* EOF */
