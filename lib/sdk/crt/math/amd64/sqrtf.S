/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Implementation of tan
 * FILE:              lib/sdk/crt/math/amd64/sqrtf.S
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <reactos/asm.h>
#include <ndk/amd64/asm.h>

/* DATA *********************************************************************/

PUBLIC sqrtf
sqrtf:
    sqrtss	xmm0, xmm0
    ret
