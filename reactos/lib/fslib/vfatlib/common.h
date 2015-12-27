/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS VFAT filesystem library
 * FILE:        lib\fslib\vfatlib\common.h
 * PURPOSE:     Common code for Fat support
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Eric Kohl
 */

#ifndef _VFATCOMMON_H_
#define _VFATCOMMON_H_

ULONG GetShiftCount(IN ULONG Value);
ULONG CalcVolumeSerialNumber(VOID);

NTSTATUS
Fat1216WipeSectors(
    IN HANDLE FileHandle,
    IN PFAT16_BOOT_SECTOR BootSector,
    IN OUT PFORMAT_CONTEXT Context);

#endif /* _VFATCOMMON_H_ */

/* EOF */
