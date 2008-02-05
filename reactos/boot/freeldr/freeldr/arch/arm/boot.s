/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            boot/freeldr/arch/arm/boot.s
 * PURPOSE:         Implements the entry point for ARM machines
 * PROGRAMMERS:     alex@winsiderss.com
 */

/* INCLUDES *******************************************************************/

//#include <kxarm.h>

/* GLOBALS ********************************************************************/

.global _start
.section startup
    
/* BOOT CODE ******************************************************************/

_start:
	b .
