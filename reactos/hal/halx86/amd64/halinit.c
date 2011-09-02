/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/amd64/halinit.c
 * PURPOSE:         HAL Entrypoint and Initialization
 * PROGRAMMERS:
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

BOOLEAN HalpPciLockSettings;

/* PRIVATE FUNCTIONS *********************************************************/

/* FUNCTIONS *****************************************************************/

VOID
HalpInitPhase0(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{

}

VOID
HalpInitPhase1(VOID)
{

}

VOID
INIT_FUNCTION
HalpInitializeClock(VOID)
{
}

VOID
HalpCalibrateStallExecution()
{
}

VOID
HalpInitializePICs(IN BOOLEAN EnableInterrupts)
{
}
