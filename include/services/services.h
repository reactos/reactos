/* $Id$
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS kernel
 * FILE:        include/services/services.h
 * PURPOSE:     Private interface between SERVICES.EXE and ADVAPI32.DLL
 * PROGRAMMER:  Eric Kohl
 */

#ifndef __SERVICES_SERVICES_H__
#define __SERVICES_SERVICES_H__

#define SCM_START_COMMAND 1

typedef struct _SCM_START_PACKET
{
  ULONG Command;
  ULONG Size;
  WCHAR Arguments[1];
} SCM_START_PACKET, *PSCM_START_PACKET;

#endif /* __SERVICES_SERVICES_H__ */

/* EOF */
