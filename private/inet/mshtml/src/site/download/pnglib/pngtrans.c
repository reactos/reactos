
/* pngtrans.c - transforms the data in a row
   routines used by both readers and writers

   libpng 1.0 beta 2 - version 0.88
   For conditions of distribution and use, see copyright notice in png.h
   Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.
   January 25, 1996
   */

#ifdef NEVER
#define PNG_INTERNAL
#include "png.h"
#endif

#include "headers.h"

#if defined(PNG_READ_BGR_SUPPORTED) || defined(PNG_WRITE_BGR_SUPPORTED)
/* turn on bgr to rgb mapping */
void
png_set_bgr(png_structp png_ptr)
{
   png_ptr->transformations |= PNG_BGR;
}
#endif

#if defined(PNG_READ_SWAP_SUPPORTED) || defined(PNG_WRITE_SWAP_SUPPORTED)
/* turn on 16 bit byte swapping */
void
png_set_swap(png_structp png_ptr)
{
   if (png_ptr->bit_depth == 16)
      png_ptr->transformations |= PNG_SWAP_BYTES;
}
#endif

#if defined(PNG_READ_PACK_SUPPORTED) || defined(PNG_WRITE_PACK_SUPPORTED)
/* turn on pixel packing */
void
png_set_packing(png_structp png_ptr)
{
   if (png_ptr->bit_depth < 8)
   {
      png_ptr->transformations |= PNG_PACK;
      png_ptr->usr_bit_depth = 8;
   }
}
#endif

#if defined(PNG_READ_SHIFT_SUPPORTED) || defined(PNG_WRITE_SHIFT_SUPPORTED)
void
png_set_shift(png_structp png_ptr, png_color_8p true_bits)
{
   png_ptr->transformations |= PNG_SHIFT;
   png_ptr->shift = *true_bits;
}
#endif

#if defined(PNG_READ_INTERLACING_SUPPORTED) || defined(PNG_WRITE_INTERLACING_SUPPORTED)
int
png_set_interlace_handling(png_structp png_ptr)
{
   if (png_ptr->interlaced)
   {
      png_ptr->transformations |= PNG_INTERLACE;
      return 7;
   }

   return 1;
}
#endif

#if defined(PNG_READ_FILLER_SUPPORTED) || defined(PNG_WRITE_FILLER_SUPPORTED)
void
png_set_filler(png_structp png_ptr, int filler, int filler_loc)
{
   png_ptr->transformations |= PNG_FILLER;
   png_ptr->filler = (png_byte)filler;
   png_ptr->filler_loc = (png_byte)filler_loc;
   if (png_ptr->color_type == PNG_COLOR_TYPE_RGB &&
      png_ptr->bit_depth == 8)
      png_ptr->usr_channels = 4;
}

/* old functions kept around for compatability purposes */
void
png_set_rgbx(png_structp png_ptr)
{
   png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
}

void
png_set_xrgb(png_structp png_ptr)
{
   png_set_filler(png_ptr, 0xff, PNG_FILLER_BEFORE);
}
#endif

#if defined(PNG_READ_INVERT_SUPPORTED) || defined(PNG_WRITE_INVERT_SUPPORTED)
void
png_set_invert_mono(png_structp png_ptr)
{
   png_ptr->transformations |= PNG_INVERT_MONO;
}

/* invert monocrome grayscale data */
void
png_do_invert(png_row_infop row_info, png_bytep row)
{
   if (row && row_info && row_info->bit_depth == 1 &&
      row_info->color_type == PNG_COLOR_TYPE_GRAY)
   {
      png_bytep rp;
      png_uint_32 i;

      for (i = 0, rp = row;
         i < row_info->rowbytes;
         i++, rp++)
      {
         *rp = (png_byte)(~(*rp));
      }
   }
}
#endif

#if defined(PNG_READ_SWAP_SUPPORTED) || defined(PNG_WRITE_SWAP_SUPPORTED)
/* swaps byte order on 16 bit depth images */
void
png_do_swap(png_row_infop row_info, png_bytep row)
{
   if (row && row_info && row_info->bit_depth == 16)
   {
      png_bytep rp;
      png_byte t;
      png_uint_32 i;

      for (i = 0, rp = row;
         i < row_info->width * row_info->channels;
         i++, rp += 2)
      {
         t = *rp;
         *rp = *(rp + 1);
         *(rp + 1) = t;
      }
   }
}
#endif

#if defined(PNG_READ_BGR_SUPPORTED) || defined(PNG_WRITE_BGR_SUPPORTED)
/* swaps red and blue */
void
png_do_bgr(png_row_infop row_info, png_bytep row)
{
   if (row && row_info && (row_info->color_type & 2))
   {
      if (row_info->color_type == 2 && row_info->bit_depth == 8)
      {
         png_bytep rp;
         png_byte t;
         png_uint_32 i;

         for (i = 0, rp = row;
            i < row_info->width;
            i++, rp += 3)
         {
            t = *rp;
            *rp = *(rp + 2);
            *(rp + 2) = t;
         }
      }
      else if (row_info->color_type == 6 && row_info->bit_depth == 8)
      {
         png_bytep rp;
         png_byte t;
         png_uint_32 i;

         for (i = 0, rp = row;
            i < row_info->width;
            i++, rp += 4)
         {
            t = *rp;
            *rp = *(rp + 2);
            *(rp + 2) = t;
         }
      }
      else if (row_info->color_type == 2 && row_info->bit_depth == 16)
      {
         png_bytep rp;
         png_byte t[2];
         png_uint_32 i;

         for (i = 0, rp = row;
            i < row_info->width;
            i++, rp += 6)
         {
            t[0] = *rp;
            t[1] = *(rp + 1);
            *rp = *(rp + 4);
            *(rp + 1) = *(rp + 5);
            *(rp + 4) = t[0];
            *(rp + 5) = t[1];
         }
      }
      else if (row_info->color_type == 6 && row_info->bit_depth == 16)
      {
         png_bytep rp;
         png_byte t[2];
         png_uint_32 i;

         for (i = 0, rp = row;
            i < row_info->width;
            i++, rp += 8)
         {
            t[0] = *rp;
            t[1] = *(rp + 1);
            *rp = *(rp + 4);
            *(rp + 1) = *(rp + 5);
            *(rp + 4) = t[0];
            *(rp + 5) = t[1];
         }
      }
   }
}
#endif

