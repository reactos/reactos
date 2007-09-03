/* $Id: resource.c 23907 2006-09-04 05:52:23Z arty $
 *
 * COPYRIGHT:             See COPYING in the top level directory
 * PROJECT:               ReactOS kernel
 * FILE:                  hal/halx86/generic/resource.c
 * PURPOSE:               Miscellaneous resource functions
 * PROGRAMMER:            Eric Kohl (ekohl@rz-online.de)
 */

/* INCLUDES *****************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>


/* FUNCTIONS ****************************************************************/

VOID STDCALL
HalReportResourceUsage(VOID)
{
  /*
   * FIXME: Report all resources used by hal.
   *        Calls IoReportHalResourceUsage()
   */

  /* Initialize PCI bus. */
  HalpInitPciBus ();
}

/* EOF */
