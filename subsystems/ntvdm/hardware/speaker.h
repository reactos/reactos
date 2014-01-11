/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            speaker.h
 * PURPOSE:         PC Speaker emulation
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _SPEAKER_H_
#define _SPEAKER_H_

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

/* DEFINES ********************************************************************/

#define SPEAKER_CONTROL_PORT 0x61

/* FUNCTIONS ******************************************************************/

VOID SpeakerInitialize(VOID);
VOID SpeakerCleanup(VOID);
BYTE SpeakerReadStatus(VOID);
VOID SpeakerWriteCommand(BYTE Value);

#endif // _SPEAKER_H_

/* EOF */
