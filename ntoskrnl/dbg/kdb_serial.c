/* $Id$
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/dbg/kdb_serial.c
 * PURPOSE:         Serial driver
 * 
 * PROGRAMMERS:     Victor Kirhenshtein (sauros@iname.com)
 *                  Jason Filby (jasonfilby@yahoo.com)
 *                  arty
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

extern KD_PORT_INFORMATION LogPortInfo;


CHAR
KdbpTryGetCharSerial(UINT Retry)
{
  CHAR Result = -1;

  if (Retry == 0)
     while (!KdPortGetByteEx(&LogPortInfo, (PUCHAR)&Result));
  else
     while (!KdPortGetByteEx(&LogPortInfo, (PUCHAR)&Result) && Retry-- > 0);

  return Result;
}
