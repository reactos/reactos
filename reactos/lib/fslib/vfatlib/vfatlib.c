/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS VFAT filesystem library
 * FILE:        vfatlib.c
 * PURPOSE:     Main API
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 05/04-2003 Created
 */
#define NDEBUG
#include <debug.h>
#define NTOS_MODE_USER
#include <ntos.h>
#include <fslib/vfatlib.h>


NTSTATUS
VfatInitialize()
{
  DPRINT("VfatInitialize()\n");

  return STATUS_SUCCESS;
}


NTSTATUS
VfatCleanup()
{
  DPRINT("VfatCleanup()\n");

  return STATUS_SUCCESS;
}
