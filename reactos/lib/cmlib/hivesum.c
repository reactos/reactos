/*
 * PROJECT:   registry manipulation library
 * LICENSE:   GPL - See COPYING in the top level directory
 * COPYRIGHT: Copyright 2005 Filip Navara <navaraf@reactos.org>
 *            Copyright 2001 - 2005 Eric Kohl
 */

#include "cmlib.h"

/**
 * @name HvpHiveHeaderChecksum
 *
 * Compute checksum of hive header and return it.
 */

ULONG CMAPI
HvpHiveHeaderChecksum(
   PHBASE_BLOCK HiveHeader)
{
   PULONG Buffer = (PULONG)HiveHeader;
   ULONG Sum = 0;
   ULONG i;

   for (i = 0; i < 127; i++)
      Sum ^= Buffer[i];
   if (Sum == (ULONG)-1)
      Sum = (ULONG)-2;
   if (Sum == 0)
      Sum = 1;

   return Sum;
}
