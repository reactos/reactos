/*
 * jmemmac.c
 *
 * Copyright (C) 1992-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * jmemmac.c provides an Apple Macintosh implementation of the system-
 * dependent portion of the JPEG memory manager.
 *
 * jmemmac.c uses the Macintosh toolbox routines NewPtr and DisposePtr
 * instead of malloc and free.  It accurately determines the amount of
 * memory available by using CompactMem.  Notice that if left to its
 * own devices, this code can chew up all available space in the
 * application's zone, with the exception of the rather small "slop"
 * factor computed in jpeg_mem_available().  The application can ensure
 * that more space is left over by reducing max_memory_to_use.
 *
 * Large images are swapped to disk using temporary files created with
 * tmpfile(); that part of the module is the same as in jmemansi.c.
 * Metrowerks CodeWarrior's implementation of tmpfile() isn't quite what
 * we want: it puts the files in the local directory and makes them
 * user-visible -- and only deletes them when the application quits,
 * which means they stick around in the event of a crash.
 * It would be better to create the temp files in the system's temporary
 * items folder.  Perhaps someday we'll get around to doing that.
 *
 * Contributed by Sam Bushell (jsam@iagu.on.net).
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jmemsys.h"		/* import the system-dependent declarations */

#include <Memory.h>		/* we use the MacOS memory manager */

#ifndef SEEK_SET		/* pre-ANSI systems may not define this; */
#define SEEK_SET  0		/* if not, assume 0 is correct */
#endif


/*
 * Memory allocation and freeing are controlled by the MacOS library
 * routines NewPtr() and DisposePtr(), which allocate fixed-address
 * storage.  Unfortunately, the IJG library isn't smart enough to cope
 * with relocatable storage.
 */

GLOBAL(void *)
jpeg_get_small (j_common_ptr cinfo, size_t sizeofobject)
{
  return (void *) NewPtr(sizeofobject);
}

GLOBAL(void)
jpeg_free_small (j_common_ptr cinfo, void * object, size_t sizeofobject)
{
  DisposePtr((Ptr) object);
}


/*
 * "Large" objects are treated the same as "small" ones.
 * NB: we include FAR keywords in the routine declarations simply for
 * consistency with the rest of the IJG code; FAR should expand to empty
 * on rational architectures like the Mac.
 */

GLOBAL(void FAR *)
jpeg_get_large (j_common_ptr cinfo, size_t sizeofobject)
{
  return (void FAR *) NewPtr(sizeofobject);
}

GLOBAL(void)
jpeg_free_large (j_common_ptr cinfo, void FAR * object, size_t sizeofobject)
{
  DisposePtr((Ptr) object);
}


/*
 * This routine computes the total memory space available for allocation.
 */

GLOBAL(long)
jpeg_mem_available (j_common_ptr cinfo, long min_bytes_needed,
		    long max_bytes_needed, long already_allocated)
{
  long limit = cinfo->mem->max_memory_to_use - already_allocated;
  long slop, mem;

  /* Don't ask for more than what application has told us we may use */
  if (max_bytes_needed > limit && limit > 0)
    max_bytes_needed = limit;
  /* Find whether there's a big enough free block in the heap.
   * CompactMem tries to create a contiguous block of the requested size,
   * and then returns the size of the largest free block (which could be
   * much more or much less than we asked for).
   * We add some slop to ensure we don't use up all available memory.
   */
  slop = max_bytes_needed / 16 + 32768L;
  mem = CompactMem(max_bytes_needed + slop) - slop;
  if (mem < 0)
    mem = 0;			/* sigh, couldn't even get the slop */
  /* Don't take more than the application says we can have */
  if (mem > limit && limit > 0)
    mem = limit;
  return mem;
}


/*
 * Backing store (temporary file) management.
 * Backing store objects are only used when the value returned by
 * jpeg_mem_available is less than the total space needed.  You can dispense
 * with these routines if you have plenty of virtual memory; see jmemnobs.c.
 */


METHODDEF(void)
read_backing_store (j_common_ptr cinfo, backing_store_ptr info,
		    void FAR * buffer_address,
		    long file_offset, long byte_count)
{
  if (fseek(info->temp_file, file_offset, SEEK_SET))
    ERREXIT(cinfo, JERR_TFILE_SEEK);
  if (JFREAD(info->temp_file, buffer_address, byte_count)
      != (size_t) byte_count)
    ERREXIT(cinfo, JERR_TFILE_READ);
}


METHODDEF(void)
write_backing_store (j_common_ptr cinfo, backing_store_ptr info,
		     void FAR * buffer_address,
		     long file_offset, long byte_count)
{
  if (fseek(info->temp_file, file_offset, SEEK_SET))
    ERREXIT(cinfo, JERR_TFILE_SEEK);
  if (JFWRITE(info->temp_file, buffer_address, byte_count)
      != (size_t) byte_count)
    ERREXIT(cinfo, JERR_TFILE_WRITE);
}


METHODDEF(void)
close_backing_store (j_common_ptr cinfo, backing_store_ptr info)
{
  fclose(info->temp_file);
  /* Since this implementation uses tmpfile() to create the file,
   * no explicit file deletion is needed.
   */
}


/*
 * Initial opening of a backing-store object.
 *
 * This version uses tmpfile(), which constructs a suitable file name
 * behind the scenes.  We don't have to use info->temp_name[] at all;
 * indeed, we can't even find out the actual name of the temp file.
 */

GLOBAL(void)
jpeg_open_backing_store (j_common_ptr cinfo, backing_store_ptr info,
			 long total_bytes_needed)
{
  if ((info->temp_file = tmpfile()) == NULL)
    ERREXITS(cinfo, JERR_TFILE_CREATE, "");
  info->read_backing_store = read_backing_store;
  info->write_backing_store = write_backing_store;
  info->close_backing_store = close_backing_store;
}


/*
 * These routines take care of any system-dependent initialization and
 * cleanup required.
 */

GLOBAL(long)
jpeg_mem_init (j_common_ptr cinfo)
{
  /* max_memory_to_use will be initialized to FreeMem()'s result;
   * the calling application might later reduce it, for example
   * to leave room to invoke multiple JPEG objects.
   * Note that FreeMem returns the total number of free bytes;
   * it may not be possible to allocate a single block of this size.
   */
  return FreeMem();
}

GLOBAL(void)
jpeg_mem_term (j_common_ptr cinfo)
{
  /* no work */
}
