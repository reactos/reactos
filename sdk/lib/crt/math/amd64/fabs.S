/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Implementation of fabs
 * FILE:              lib/sdk/crt/math/amd64/fabs.S
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>

/* CODE **********************************************************************/
.code64

#ifdef _USE_ML
/* fabs is now allowed as label name, so create _fabs instead and alias fabs to it */
PUBLIC _fabs
_fabs:
#else
PUBLIC fabs
fabs:
#endif
    UNIMPLEMENTED fabs
    ret

#ifdef _USE_ML
ALIAS <fabs> = <_fabs>
#endif

END
