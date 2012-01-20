/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/vfat/string.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 *
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "vfat.h"

/* FUNCTIONS ****************************************************************/

const WCHAR *long_illegals = L"\"*\\<>/?:|";

BOOLEAN
vfatIsLongIllegal(WCHAR c)
{
  return wcschr(long_illegals, c) ? TRUE : FALSE;
}
