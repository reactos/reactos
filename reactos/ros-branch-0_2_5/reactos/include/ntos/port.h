/* $Id: port.h,v 1.2 2002/11/14 18:21:03 chorns Exp $
 *
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         include/ntos/port.h
 * PURPOSE:      Port declarations used by all the parts of the 
 *               system
 * PROGRAMMER:   Eric Kohl <ekokl@rz-online.de>
 * UPDATE HISTORY: 
 *               02/02/2001: Created
 */

#ifndef __INCLUDE_PORT_H
#define __INCLUDE_PORT_H

#ifndef __USE_W32API

/* Port Object Access */

#define PORT_ALL_ACCESS               (0x1)

#else /* __USE_W32API */

#include <ddk/ntifs.h>

#endif /* __USE_W32API */

#endif /* __INCLUDE_PORT_H */

/* EOF */
