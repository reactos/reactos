/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            dskbios32.h
 * PURPOSE:         VDM 32-bit Disk BIOS
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _DSKBIOS32_H_
#define _DSKBIOS32_H_

/* DEFINES ********************************************************************/

#define BIOS_DISK_INTERRUPT     0x13

/* FUNCTIONS ******************************************************************/

VOID DiskBios32Post(VOID);

BOOLEAN DiskBios32Initialize(VOID);
VOID DiskBios32Cleanup(VOID);

#endif /* _DSKBIOS32_H_ */

/* EOF */
