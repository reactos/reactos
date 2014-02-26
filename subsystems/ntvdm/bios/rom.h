/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            rom.h
 * PURPOSE:         ROM Support Functions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _ROM_H_
#define _ROM_H_

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

/* DEFINES ********************************************************************/

#define ROM_AREA_START  0xE0000
#define ROM_AREA_END    0xFFFFF

#define OPTION_ROM_SIGNATURE    0xAA55

/* FUNCTIONS ******************************************************************/

BOOLEAN LoadBios(IN  LPCWSTR BiosFileName,
                 OUT PVOID*  BiosLocation OPTIONAL,
                 OUT PDWORD  BiosSize     OPTIONAL);

BOOLEAN LoadRom(IN  LPCWSTR RomFileName,
                IN  PVOID   RomLocation,
                OUT PDWORD  RomSize OPTIONAL);

VOID SearchAndInitRoms(IN PCALLBACK16 Context);

#endif // _ROM_H_

/* EOF */
