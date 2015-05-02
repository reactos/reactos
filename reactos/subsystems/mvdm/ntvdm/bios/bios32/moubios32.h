/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            moubios32.h
 * PURPOSE:         VDM Mouse 32-bit BIOS
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _MOUBIOS32_H_
#define _MOUBIOS32_H_

/* DEFINES ********************************************************************/

/* FUNCTIONS ******************************************************************/

VOID BiosMousePs2Interface(LPWORD Stack);

BOOLEAN MouseBios32Initialize(VOID);
VOID MouseBios32Cleanup(VOID);

#endif /* _MOUBIOS32_H_ */

/* EOF */
