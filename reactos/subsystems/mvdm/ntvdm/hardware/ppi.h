/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            ppi.h
 * PURPOSE:         Programmable Peripheral Interface emulation -
 *                  i8255A-5 compatible
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _PPI_H_
#define _PPI_H_

/* DEFINES ********************************************************************/

#define PPI_PORT_61H    0x61
#define PPI_PORT_62H    0x62

extern BYTE Port61hState;

/* FUNCTIONS ******************************************************************/

VOID PpiInitialize(VOID);

#endif // _PPI_H_

/* EOF */
