/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            kbdbios.h
 * PURPOSE:         VDM Keyboard BIOS Support Library
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _KBDBIOS_H_
#define _KBDBIOS_H_

/* DEFINES ********************************************************************/

#define BIOS_KBD_INTERRUPT      0x16

#define BIOS_KBD_BUFFER_SIZE    16

/* FUNCTIONS ******************************************************************/

BOOLEAN KbdBiosInitialize(VOID);
VOID KbdBiosCleanup(VOID);

#endif // _KBDBIOS_H_

/* EOF */
