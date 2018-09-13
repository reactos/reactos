
/* pngmem.c - stub functions for memory allocation

   libpng 1.0 beta 2 - version 0.88
   For conditions of distribution and use, see copyright notice in png.h
   Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.
   January 25, 1996

   This file provides a location for all memory allocation.  Users which
   need special memory handling are expected to modify the code in this file
   to meet their needs.  See the instructions at each function. */

#ifdef NEVER
#define PNG_INTERNAL
#include "png.h"
#endif

#include "headers.h"

/* Borland DOS special memory handler */
#if defined(__TURBOC__) && !defined(_Windows) && !defined(__FLAT__)
/* if you change this, be sure to change the one in png.h also */

/* Allocate memory.  For reasonable files, size should never exceed
   64K.  However, zlib may allocate more then 64K if you don't tell
   it not to.  See zconf.h and png.h for more information. zlib does
   need to allocate exactly 64K, so whatever you call here must
   have the ability to do that. */

/* Borland seems to have a problem in DOS mode for exactly 64K.
   It gives you a segment with an offset of 8 (perhaps to store it's
   memory stuff).  zlib doesn't like this at all, so we have to
   detect and deal with it.  This code should not be needed in
   Windows or OS/2 modes, and only in 16 bit mode.
*/

png_voidp
png_large_malloc(png_structp png_ptr, png_uint_32 size)
{
   png_voidp ret;
   if (!png_ptr || !size)
      return ((voidp)0);

#ifdef PNG_MAX_MALLOC_64K
   if (size > (png_uint_32)65536L)
      png_error(png_ptr, "Cannot Allocate > 64K");
#endif

   if (size == (png_uint_32)(65536L))
   {
      if (!png_ptr->offset_table)
      {
         /* try to see if we need to do any of this fancy stuff */
         ret = farmalloc(size);
         if (!ret || ((long)ret & 0xffff))
         {
            int num_blocks;
            png_uint_32 total_size;
            png_bytep table;
            int i;
            png_byte huge * hptr;

            if (ret)
               farfree(ret);
            ret = 0;

            num_blocks = (int)(1 << (png_ptr->zlib_window_bits - 14));
            if (num_blocks < 1)
               num_blocks = 1;
            if (png_ptr->zlib_mem_level >= 7)
               num_blocks += (int)(1 << (png_ptr->zlib_mem_level - 7));
            else
               num_blocks++;

            total_size = ((png_uint_32)65536L) * (png_uint_32)num_blocks;

            table = farmalloc(total_size);

            if (!table)
            {
               png_error(png_ptr, "Out of Memory");
            }

            if ((long)table & 0xffff)
            {
               farfree(table);
               total_size += (png_uint_32)65536L;
            }

            table = farmalloc(total_size);

            if (!table)
            {
               png_error(png_ptr, "Out of Memory");
            }

            png_ptr->offset_table = table;
            png_ptr->offset_table_ptr = farmalloc(
               num_blocks * sizeof (png_bytep));
            hptr = (png_byte huge *)table;
            if ((long)hptr & 0xffff)
            {
               hptr = (png_byte huge *)((long)(hptr) & 0xffff0000L);
               hptr += 65536L;
            }
            for (i = 0; i < num_blocks; i++)
            {
               png_ptr->offset_table_ptr[i] = (png_bytep)hptr;
               hptr += 65536L;
            }

            png_ptr->offset_table_number = num_blocks;
            png_ptr->offset_table_count = 0;
            png_ptr->offset_table_count_free = 0;
         }

         if (png_ptr->offset_table_count >= png_ptr->offset_table_number)
            png_error(png_ptr, "Out of Memory");

         ret = png_ptr->offset_table_ptr[png_ptr->offset_table_count++];
      }
   }
   else
      ret = farmalloc(size);

   if (ret == NULL)
   {
      png_error(png_ptr, "Out of Memory");
   }

   return ret;
}

/* free a pointer allocated by png_large_malloc().  In the default
  configuration, png_ptr is not used, but is passed in case it
  is needed.  If ptr is NULL, return without taking any action. */
void
png_large_free(png_structp png_ptr, png_voidp ptr)
{
   if (!png_ptr)
      return;

   if (ptr != NULL)
   {
      if (png_ptr->offset_table)
      {
         int i;

         for (i = 0; i < png_ptr->offset_table_count; i++)
         {
            if (ptr == png_ptr->offset_table_ptr[i])
            {
               ptr = 0;
               png_ptr->offset_table_count_free++;
               break;
            }
         }
         if (png_ptr->offset_table_count_free == png_ptr->offset_table_count)
         {
            farfree(png_ptr->offset_table);
            farfree(png_ptr->offset_table_ptr);
            png_ptr->offset_table = 0;
            png_ptr->offset_table_ptr = 0;
         }
      }

      if (ptr)
         farfree(ptr);
   }
}

#else /* Not the Borland DOS special memory handler */

/* Allocate memory.  For reasonable files, size should never exceed
   64K.  However, zlib may allocate more then 64K if you don't tell
   it not to.  See zconf.h and png.h for more information. zlib does
   need to allocate exactly 64K, so whatever you call here must
   have the ability to do that. */


png_voidp
png_large_malloc(png_structp png_ptr, png_uint_32 size)
{
   png_voidp ret;
   if (!png_ptr || !size)
      return ((voidp)0);

#ifdef PNG_MAX_MALLOC_64K
   if (size > (png_uint_32)65536L)
      png_error(png_ptr, "Cannot Allocate > 64K");
#endif

#if defined(__TURBOC__) && !defined(__FLAT__)
   ret = farmalloc(size);
#else
# if defined(_MSC_VER) && defined(MAXSEG_64K)
   ret = halloc(size, 1);
# else
    ret = GlobalAlloc(GMEM_FIXED, size);
# endif
#endif

   if (ret == NULL)
   {
      png_error(png_ptr, "Out of Memory");
   }

   return ret;
}

/* free a pointer allocated by png_large_malloc().  In the default
  configuration, png_ptr is not used, but is passed in case it
  is needed.  If ptr is NULL, return without taking any action. */
void
png_large_free(png_structp png_ptr, png_voidp ptr)
{
   if (!png_ptr)
      return;

   if (ptr != NULL)
   {
#if defined(__TURBOC__) && !defined(__FLAT__)
      farfree(ptr);
#else
# if defined(_MSC_VER) && defined(MAXSEG_64K)
      hfree(ptr);
# else
        GlobalFree(ptr);
# endif
#endif
   }
}

#endif /* Not Borland DOS special memory handler */

/* Allocate memory.  This is called for smallish blocks only  It
   should not get anywhere near 64K.  On segmented machines, this
   must come from the local heap (for zlib).  Currently, zlib is
   the only one that uses this, so you should only get one call
   to this, and that a small block. */
void *
png_malloc(png_structp png_ptr, png_uint_32 size)
{
   void *ret;

   if (!png_ptr || !size)
   {
      return ((void *)0);
   }

#ifdef PNG_MAX_MALLOC_64K
   if (size > (png_uint_32)65536L)
      png_error(png_ptr, "Cannot Allocate > 64K");
#endif


   ret = GlobalAlloc(GMEM_FIXED, (png_size_t)size); // malloc((png_size_t)size);

   if (!ret)
   {
      png_error(png_ptr, "Out of Memory");
   }

   return ret;
}

/* Reallocate memory.  This will not get near 64K on a
   even marginally reasonable file.  This is not used in
   the current version of the library. */
void *
png_realloc(png_structp png_ptr, void * ptr, png_uint_32 size,
   png_uint_32 old_size)
{
   void *ret;

   if (!png_ptr || !old_size || !ptr || !size)
      return ((void *)0);

#ifdef PNG_MAX_MALLOC_64K
   if (size > (png_uint_32)65536L)
      png_error(png_ptr, "Cannot Allocate > 64K");
#endif

    ret = GlobalReAlloc(ptr, (png_size_t)size, GMEM_FIXED);

   if (!ret)
   {
      png_error(png_ptr, "Out of Memory 7");
   }

   return ret;
}

/* free a pointer allocated by png_malloc().  In the default
  configuration, png_ptr is not used, but is passed incase it
  is needed.  If ptr is NULL, return without taking any action. */
void
png_free(png_structp png_ptr, void * ptr)
{
   if (!png_ptr)
      return;

   if (ptr != (void *)0)
        GlobalFree(ptr);
}


