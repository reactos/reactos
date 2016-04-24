/*
 * FILE:            hal/halx86/amd64/systimer.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         System Timer Interrupt and Management
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
#include <ksamd64.inc>

/* GLOBALS *******************************************************************/

.data

PUBLIC MsgUnimplemented
MsgUnimplemented:
.asciz "WARNING:  %s at %s:%d is UNIMPLEMENTED!\n"


/* FUNCTIONS *****************************************************************/

.code64



END
