/* $Id: halxbox.h,v 1.2 2004/12/04 22:52:59 gvg Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Xbox HAL
 * FILE:            hal/halx86/xbox/halxbox.h
 * PURPOSE:         Xbox specific routines
 * PROGRAMMER:      Ge van Geldorp (gvg@reactos.com)
 * UPDATE HISTORY:
 *                  Created 2004/12/02
 */

#ifndef HALXBOX_H_INCLUDED
#define HALXBOX_H_INCLUDED

extern BYTE XboxFont8x16[256 * 16];

void HalpXboxInitPciBus(ULONG BusNumber, PBUS_HANDLER BusHandler);

#endif /* HALXBOX_H_INCLUDED */

/* EOF */
