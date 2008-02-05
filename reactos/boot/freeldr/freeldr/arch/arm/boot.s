/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            boot/freeldr/arch/arm/boot.s
 * PURPOSE:         Implements the entry point for ARM machines
 * PROGRAMMERS:     alex@winsiderss.com
 */

/* INCLUDES *******************************************************************/

//#include <ksarm.h>
//#include <kxarm.h>

/* GLOBALS ********************************************************************/

.globl _start
.globl _bss
    
/* BOOT CODE ******************************************************************/

.extern ArmInit
_start:
	b .
