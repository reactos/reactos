/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/vddsup.h
 * PURPOSE:         Virtual Device Drivers (VDD) Support
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _VDDSUP_H_
#define _VDDSUP_H_

/* FUNCTIONS ******************************************************************/

VOID VDDCreateUserHook(USHORT DosPDB);
VOID VDDTerminateUserHook(USHORT DosPDB);
VOID VDDBlockUserHook(VOID);
VOID VDDResumeUserHook(VOID);

VOID VDDSupInitialize(VOID);

#endif /* _VDDSUP_H_ */
