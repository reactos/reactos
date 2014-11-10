/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            clock.h
 * PURPOSE:         Clock for VDM
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _CLOCK_H_
#define _CLOCK_H_

/* FUNCTIONS ******************************************************************/

VOID ClockUpdate(VOID);
BOOLEAN ClockInitialize(VOID);

#endif // _CLOCK_H_

/* EOF */
