/* $Id: fdtable.c,v 1.3 2002/03/11 20:48:08 hyperion Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/misc/fdtable.c
 * PURPOSE:     File descriptors table functions
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              12/02/2002: Created
 */

#include <string.h>
#include <errno.h>
#include <psx/fdtable.h>
#include <psx/stdlib.h>
#include <psx/debug.h>
#include <psx/safeobj.h>

int __fdtable_init(__fdtable_t * fdtable)
{
 if(fdtable == 0)
 {
  errno = EINVAL;
  return (-1);
 }

 memset(fdtable, 0, sizeof(*fdtable));

 fdtable->Signature = __FDTABLE_MAGIC;

 return (0);
}

int __fdtable_free(__fdtable_t * fdtable)
{
 if(fdtable == 0)
 {
  errno = EINVAL;
  return (-1);
 }

 __free(&fdtable->Descriptors);

 memset(fdtable, 0, sizeof(*fdtable));

 fdtable->Signature = MAGIC('B', 'A', 'A', 'D');

 return (0);
}

int __fdtable_entry_isavail(__fdtable_t * fdtable, int fileno)
{
 return ((fdtable->DescriptorsBitmap[fileno / 32] >> (fileno % 32)) % 2);
}

int __fdtable_entry_nextavail(__fdtable_t * fdtable, int fileno)
{
 int      nCurMapIndex;
 int      nUnusedIndex;
 uint32_t nCurMapCell;

 nUnusedIndex = fileno;

 /* The file descriptors bitmap is an array of 32 bit unsigned integers (32 bit
    integers were chosen for proper data alignment without padding). The array is
    big enough to hold at least OPEN_MAX bits, that is it has OPEN_MAX / 32 cells
    (see also the __fdtable_t definition in psx/fdtable.h). Bits correspond to
    file numbers: if a bit is 1, the corresponding file number is in use, else
    it's unused. Bit numbering is right-to-left wise, that is the rightmost (least
    significative) bit of cell 0 corresponds to file number 0, the leftmost (most
    significative) bit of cell 0 to file number 7, the leftmost bit of cell 1 to
    file number 8, and so on
  */
 /* NOTE: I'm sure the algorytm can be greatly optimized, but I prefer to privilege
    readability - it allows for more maintenable code. Please don't pretend to
    outsmart the compiler: such optimizations as performing divisions as bit shifts
    are useless */

 /* index of the bitmap cell containing nUnusedIndex */
 nCurMapIndex = nUnusedIndex / 32;

 /* get a copy of the bitmap cell containg nUnusedIndex, and shift it to the right
    so that the rightmost (least significative) bit is the one referencing nUnusedIndex */
 nCurMapCell = fdtable->DescriptorsBitmap[nCurMapIndex] >> (nUnusedIndex % 32);

 while(1)
 {
  /* if the least significative bit of the current cell is 0, we've found an unused
     fileno, and we return it */
  if((nCurMapCell % 2) == 0)
   return (nUnusedIndex);

  /* on to next fileno */
  nUnusedIndex ++;

  /* this is NOT a failure. -1 with undefined errno means that no unused file
     number exists */
  if(nUnusedIndex >= OPEN_MAX)
   return (-1);

  /* this fileno is referenced in the next cell */
  if((nUnusedIndex % 32) == 0)
  {
   nCurMapIndex ++;
   nCurMapCell = fdtable->DescriptorsBitmap[nCurMapIndex];
  }
  /* on to next fileno (bit) in the current cell */
  else
   nCurMapCell >>= 1;
 }

 return (-1);
}

int __fdtable_entry_add(__fdtable_t * fdtable, int fileno, __fildes_t * fildes, __fildes_t ** newfd)
{
 int nFileNo;

 /* descriptors count reached OPEN_MAX */
 if(fdtable->UsedDescriptors >= OPEN_MAX)
 {
  ERR("file descriptor table full");
  errno = EMFILE;
  return (-1);
 }

 /* base fileno less than zero: use the lowest unused fileno */
 if(fileno < 0)
  nFileNo = fdtable->LowestUnusedFileNo;
 /* base fileno greater than or equal to zero: use the next available fileno */
 else
  nFileNo = __fdtable_entry_nextavail(fdtable, fileno);

 INFO("lowest unused file number is %d", nFileNo);

 /* descriptors count reached OPEN_MAX */
 if(nFileNo < 0)
 {
  ERR("nFileNo is less than zero");
  errno = EMFILE;
  return (-1);
 }

 /* if the table doesn't have enough space for the next entry ... */
 if(nFileNo >= fdtable->AllocatedDescriptors)
 {
  void * pTemp;

  INFO
  (
   "growing the array from %lu to %lu bytes",
   fdtable->AllocatedDescriptors * sizeof(*fdtable->Descriptors),
   (nFileNo + 1) * sizeof(*fdtable->Descriptors)
  );

  /* ... try to increase the size of the table */
  pTemp = __realloc
  (
   fdtable->Descriptors,
   (nFileNo + 1) * sizeof(*fdtable->Descriptors)
  );

  /* reallocation failed */
  if(pTemp == 0)
  {
   ERR("__realloc() failed");
   errno = ENOMEM;
   return (-1);
  }

  /* update the table */
  fdtable->AllocatedDescriptors = nFileNo + 1;
  fdtable->Descriptors = pTemp;
 }

 /* initialize descriptor */
 if(fildes == 0)
  memset(&fdtable->Descriptors[nFileNo], 0, sizeof(__fildes_t));
 else
  memcpy(&fdtable->Descriptors[nFileNo], fildes, sizeof(__fildes_t));

 if(newfd != 0)
  *newfd = &fdtable->Descriptors[nFileNo];

 INFO
 (
  "file number %d: handle 0x%08X, open flags 0x%08X, flags 0x%08X, extra data size %u, extra data at 0x%08X",
  nFileNo,
  fdtable->Descriptors[nFileNo].FileHandle,
  fdtable->Descriptors[nFileNo].OpenFlags,
  fdtable->Descriptors[nFileNo].FdFlags,
  fdtable->Descriptors[nFileNo].ExtraDataSize,
  fdtable->Descriptors[nFileNo].ExtraData
 );

 INFO
 (
  "incrementing used descriptors count from %u to %u",
  fdtable->UsedDescriptors,
  fdtable->UsedDescriptors + 1
 );
 fdtable->UsedDescriptors ++;

 INFO
 (
  "setting bit %u of cell %u of the bitmap to 1",
  nFileNo % 32,
  nFileNo / 32
 );
 fdtable->DescriptorsBitmap[nFileNo / 32] |= (1 << (nFileNo % 32));

 fdtable->LowestUnusedFileNo = __fdtable_entry_nextavail(fdtable, nFileNo);
 INFO("setting the lowest unused file number to %d", fdtable->LowestUnusedFileNo);

 return (nFileNo);
}

int __fdtable_entry_remove(__fdtable_t * fdtable, int fileno)
{
 return (-1);
}

__fildes_t *__fdtable_entry_get(__fdtable_t * fdtable, int fileno)
{
 /* this fileno hasn't been allocated */
 if(fileno >= fdtable->AllocatedDescriptors)
  return (0);

 /* TODO: check the fileno against the bitmap */
 return (&fdtable->Descriptors[fileno]);
}

/* EOF */

