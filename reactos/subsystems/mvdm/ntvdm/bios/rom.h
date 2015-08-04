/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            rom.h
 * PURPOSE:         ROM Support Functions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _ROM_H_
#define _ROM_H_

/* DEFINES ********************************************************************/

#define ROM_AREA_START  0xE0000
#define ROM_AREA_END    0xFFFFF

#define OPTION_ROM_SIGNATURE    0xAA55

/* FUNCTIONS ******************************************************************/

BOOLEAN
WriteProtectRom(IN PVOID RomLocation,
                IN ULONG RomSize);

BOOLEAN
WriteUnProtectRom(IN PVOID RomLocation,
                  IN ULONG RomSize);

UCHAR
CalcRomChecksum(IN ULONG RomLocation,
                IN ULONG RomSize);

BOOLEAN
LoadBios(IN  PCSTR  BiosFileName,
         OUT PVOID* BiosLocation OPTIONAL,
         OUT PULONG BiosSize     OPTIONAL);

BOOLEAN
LoadRom(IN  PCSTR  RomFileName,
        IN  PVOID  RomLocation,
        OUT PULONG RomSize OPTIONAL);

VOID
SearchAndInitRoms(IN PCALLBACK16 Context);

#endif // _ROM_H_

/* EOF */
