/* $Id:$
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
KdbTryGetCharSerial()
{
  UCHAR Result;

  while( !KdPortGetByteEx (&LogPortInfo, &Result) );

  return Result;
}
