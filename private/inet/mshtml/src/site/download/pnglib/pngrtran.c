
/* pngrtran.c - transforms the data in a row for png readers

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

#if defined(PNG_READ_BACKGROUND_SUPPORTED)
/* handle alpha and tRNS via a background color */
void
png_set_background(png_structp png_ptr,
   png_color_16p background_color, int background_gamma_code,
   int need_expand, double background_gamma)
{
   png_ptr->transformations |= PNG_BACKGROUND;
   png_memcpy(&(png_ptr->background), background_color,
      sizeof(png_color_16));
   png_ptr->background_gamma = (float)background_gamma;
   png_ptr->background_gamma_type = (png_byte)(background_gamma_code);
   png_ptr->background_expand = (png_byte)need_expand;
}
#endif

#if defined(PNG_READ_16_TO_8_SUPPORTED)
/* strip 16 bit depth files to 8 bit depth */
void
png_set_strip_16(png_structp png_ptr)
{
   png_ptr->transformations |= PNG_16_TO_8;
}
#endif

#if defined(PNG_READ_DITHER_SUPPORTED)
/* dither file to 8 bit.  Supply a palette, the current number
   of elements in the palette, the maximum number of elements
   allowed, and a histogram, if possible.  If the current number
   is greater then the maximum number, the palette will be
   modified to fit in the maximum number */


typedef struct png_dsort_struct
{
   struct png_dsort_struct FAR * next;
   png_byte left;
   png_byte right;
} png_dsort;
typedef png_dsort FAR *       png_dsortp;
typedef png_dsort FAR * FAR * png_dsortpp;

void
png_set_dither(png_structp png_ptr, png_colorp palette,
   int num_palette, int maximum_colors, png_uint_16p histogram,
   int full_dither)
{
   png_ptr->transformations |= PNG_DITHER;

   if (!full_dither)
   {
      int i;

      png_ptr->dither_index = (png_bytep)png_large_malloc(png_ptr,
         num_palette * sizeof (png_byte));
      for (i = 0; i < num_palette; i++)
         png_ptr->dither_index[i] = (png_byte)i;
   }

   if (num_palette > maximum_colors)
   {
      if (histogram)
      {
         /* this is easy enough, just throw out the least used colors.
            perhaps not the best solution, but good enough */

         int i;
         png_bytep sort;

         /* initialize an array to sort colors */
         sort = (png_bytep)png_large_malloc(png_ptr, num_palette * sizeof (png_byte));

         /* initialize the sort array */
         for (i = 0; i < num_palette; i++)
            sort[i] = (png_byte)i;

         /* find the least used palette entries by starting a
            bubble sort, and running it until we have sorted
            out enough colors.  Note that we don't care about
            sorting all the colors, just finding which are
            least used. */

         for (i = num_palette - 1; i >= maximum_colors; i--)
         {
            int done; /* to stop early if the list is pre-sorted */
            int j;

            done = 1;
            for (j = 0; j < i; j++)
            {
               if (histogram[sort[j]] < histogram[sort[j + 1]])
               {
                  png_byte t;

                  t = sort[j];
                  sort[j] = sort[j + 1];
                  sort[j + 1] = t;
                  done = 0;
               }
            }
            if (done)
               break;
         }

         /* swap the palette around, and set up a table, if necessary */
         if (full_dither)
         {
            int j;

            /* put all the useful colors within the max, but don't
               move the others */
            j = num_palette;
            for (i = 0; i < maximum_colors; i++)
            {
               if (sort[i] >= maximum_colors)
               {
                  do
                     j--;
                  while (sort[j] >= maximum_colors);
                  palette[i] = palette[j];
               }
            }
         }
         else
         {
            int j;

            /* move all the used colors inside the max limit, and
               develop a translation table */
            j = num_palette;
            for (i = 0; i < maximum_colors; i++)
            {
               /* only move the colors we need to */
               if (sort[i] >= maximum_colors)
               {
                  png_color tmp_color;

                  do
                     j--;
                  while (sort[j] >= maximum_colors);

                  tmp_color = palette[j];
                  palette[j] = palette[i];
                  palette[i] = tmp_color;
                  /* indicate where the color went */
                  png_ptr->dither_index[j] = (png_byte)i;
                  png_ptr->dither_index[i] = (png_byte)j;
               }
            }
            /* find closest color for those colors we are not
               using */
            for (i = 0; i < num_palette; i++)
            {
               if (png_ptr->dither_index[i] >= maximum_colors)
               {
                  int min_d, j, min_j, index;

                  /* find the closest color to one we threw out */
                  index = png_ptr->dither_index[i];
                  min_d = PNG_COLOR_DIST(palette[index],
                        palette[0]);
                  min_j = 0;
                  for (j = 1; j < maximum_colors; j++)
                  {
                     int d;

                     d = PNG_COLOR_DIST(palette[index],
                        palette[j]);

                     if (d < min_d)
                     {
                        min_d = d;
                        min_j = j;
                     }
                  }
                  /* point to closest color */
                  png_ptr->dither_index[i] = (png_byte)min_j;
               }
            }
         }
         png_large_free(png_ptr, sort);
      }
      else
      {
         /* this is much harder to do simply (and quickly).  Perhaps
            we need to go through a median cut routine, but those
            don't always behave themselves with only a few colors
            as input.  So we will just find the closest two colors,
            and throw out one of them (chosen somewhat randomly).
            */
         int i;
         int max_d;
         int num_new_palette;
         png_dsortpp hash;
         png_bytep index_to_palette;
            /* where the original index currently is in the palette */
         png_bytep palette_to_index;
            /* which original index points to this palette color */

         /* initialize palette index arrays */
         index_to_palette = (png_bytep)png_large_malloc(png_ptr,
            num_palette * sizeof (png_byte));
         palette_to_index = (png_bytep)png_large_malloc(png_ptr,
            num_palette * sizeof (png_byte));

         /* initialize the sort array */
         for (i = 0; i < num_palette; i++)
         {
            index_to_palette[i] = (png_byte)i;
            palette_to_index[i] = (png_byte)i;
         }

         hash = (png_dsortpp)png_large_malloc(png_ptr, 769 * sizeof (png_dsortp));
         for (i = 0; i < 769; i++)
            hash[i] = (png_dsortp)0;
/*         png_memset(hash, 0, 769 * sizeof (png_dsortp)); */

         num_new_palette = num_palette;

         /* initial wild guess at how far apart the farthest pixel
            pair we will be eliminating will be.  Larger
            numbers mean more areas will be allocated, Smaller
            numbers run the risk of not saving enough data, and
            having to do this all over again.

            I have not done extensive checking on this number.
            */
         max_d = 96;

         while (num_new_palette > maximum_colors)
         {
            for (i = 0; i < num_new_palette - 1; i++)
            {
               int j;

               for (j = i + 1; j < num_new_palette; j++)
               {
                  int d;

                  d = PNG_COLOR_DIST(palette[i], palette[j]);

                  if (d <= max_d)
                  {
                     png_dsortp t;

                     t = (struct png_dsort_struct *)png_large_malloc(png_ptr, sizeof (png_dsort));
                     t->next = hash[d];
                     t->left = (png_byte)i;
                     t->right = (png_byte)j;
                     hash[d] = t;
                  }
               }
            }

            for (i = 0; i <= max_d; i++)
            {
               if (hash[i])
               {
                  png_dsortp p;

                  for (p = hash[i]; p; p = p->next)
                  {
                     if (index_to_palette[p->left] < num_new_palette &&
                        index_to_palette[p->right] < num_new_palette)
                     {
                        int j, next_j;

                        if (num_new_palette & 1)
                        {
                           j = p->left;
                           next_j = p->right;
                        }
                        else
                        {
                           j = p->right;
                           next_j = p->left;
                        }

                        num_new_palette--;
                        palette[index_to_palette[j]] =
                           palette[num_new_palette];
                        if (!full_dither)
                        {
                           int k;

                           for (k = 0; k < num_palette; k++)
                           {
                              if (png_ptr->dither_index[k] ==
                                 index_to_palette[j])
                                 png_ptr->dither_index[k] =
                                    index_to_palette[next_j];
                              if (png_ptr->dither_index[k] ==
                                 num_new_palette)
                                 png_ptr->dither_index[k] =
                                    index_to_palette[j];
                           }
                        }

                        index_to_palette[palette_to_index[num_new_palette]] =
                           index_to_palette[j];
                        palette_to_index[index_to_palette[j]] =
                           palette_to_index[num_new_palette];

                        index_to_palette[j] = (png_byte)num_new_palette;
                        palette_to_index[num_new_palette] = (png_byte)j;
                     }
                     if (num_new_palette <= maximum_colors)
                        break;
                  }
                  if (num_new_palette <= maximum_colors)
                     break;
               }
            }

            for (i = 0; i < 769; i++)
            {
               if (hash[i])
               {
                  png_dsortp p;

                  p = hash[i];
                  while (p)
                  {
                     png_dsortp t;

                     t = p->next;
                     png_large_free(png_ptr, p);
                     p = t;
                  }
               }
               hash[i] = 0;
            }
            max_d += 96;
         }
         png_large_free(png_ptr, hash);
         png_large_free(png_ptr, palette_to_index);
         png_large_free(png_ptr, index_to_palette);
      }
      num_palette = maximum_colors;
   }
   if (!(png_ptr->palette))
   {
      png_ptr->palette = palette;
      png_ptr->user_palette = 1;
   }
   png_ptr->num_palette = (png_uint_16)num_palette;

   if (full_dither)
   {
      int i;
      int total_bits, num_red, num_green, num_blue;
      png_uint_32 num_entries;
      png_bytep distance;

      total_bits = PNG_DITHER_RED_BITS + PNG_DITHER_GREEN_BITS +
         PNG_DITHER_BLUE_BITS;

      num_red = (1 << PNG_DITHER_RED_BITS);
      num_green = (1 << PNG_DITHER_GREEN_BITS);
      num_blue = (1 << PNG_DITHER_BLUE_BITS);
      num_entries = ((png_uint_32)1 << total_bits);

      png_ptr->palette_lookup = (png_bytep )png_large_malloc(png_ptr,
         (png_size_t)num_entries * sizeof (png_byte));

      png_memset(png_ptr->palette_lookup, 0, (png_size_t)num_entries * sizeof (png_byte));

      distance = (png_bytep )png_large_malloc(png_ptr,
         (png_size_t)num_entries * sizeof (png_byte));

      png_memset(distance, 0xff, (png_size_t)num_entries * sizeof (png_byte));

      for (i = 0; i < num_palette; i++)
      {
         int r, g, b, ir, ig, ib;

         r = (palette[i].red >> (8 - PNG_DITHER_RED_BITS));
         g = (palette[i].green >> (8 - PNG_DITHER_GREEN_BITS));
         b = (palette[i].blue >> (8 - PNG_DITHER_BLUE_BITS));

         for (ir = 0; ir < num_red; ir++)
         {
            int dr, index_r;

            dr = abs(ir - r);
            index_r = (ir << (PNG_DITHER_BLUE_BITS + PNG_DITHER_GREEN_BITS));
            for (ig = 0; ig < num_green; ig++)
            {
               int dg, dt, dm, index_g;

               dg = abs(ig - g);
               dt = dr + dg;
               dm = ((dr > dg) ? dr : dg);
               index_g = index_r | (ig << PNG_DITHER_BLUE_BITS);
               for (ib = 0; ib < num_blue; ib++)
               {
                  int index, db, dmax, d;

                  index = index_g | ib;
                  db = abs(ib - b);
                  dmax = ((dm > db) ? dm : db);
                  d = dmax + dt + db;

                  if (d < distance[index])
                  {
                     distance[index] = (png_byte)d;
                     png_ptr->palette_lookup[index] = (png_byte)i;
                  }
               }
            }
         }
      }

      png_large_free(png_ptr, distance);
   }
}
#endif

#if defined(PNG_READ_GAMMA_SUPPORTED)
/* transform the image from the file_gamma to the screen_gamma */
void
png_set_gamma(png_structp png_ptr, double screen_gamma,
   double file_gamma)
{
   png_ptr->transformations |= PNG_GAMMA;
   png_ptr->gamma = (float)file_gamma;
   png_ptr->display_gamma = (float)screen_gamma;
}
#endif

#if defined(PNG_READ_EXPAND_SUPPORTED)
/* expand paletted images to rgb, expand grayscale images of
   less then 8 bit depth to 8 bit depth, and expand tRNS chunks
   to alpha channels */
void
png_set_expand(png_structp png_ptr)
{
   png_ptr->transformations |= PNG_EXPAND;
}
#endif

#if defined(PNG_READ_GRAY_TO_RGB_SUPPORTED)
void
png_set_gray_to_rgb(png_structp png_ptr)
{
   png_ptr->transformations |= PNG_GRAY_TO_RGB;
}
#endif

/* initialize everything needed for the read.  This includes modifying
   the palette */
void
png_init_read_transformations(png_structp png_ptr)
{
   int color_type;

   color_type = png_ptr->color_type;

#if defined(PNG_READ_EXPAND_SUPPORTED) && \
    defined(PNG_READ_BACKGROUND_SUPPORTED)
   if (png_ptr->transformations & PNG_EXPAND)
   {
      if (color_type == PNG_COLOR_TYPE_GRAY &&
         png_ptr->bit_depth < 8 &&
         (png_ptr->transformations & PNG_BACKGROUND) &&
         png_ptr->background_expand)
/*         (!(png_ptr->transformations & PNG_BACKGROUND) ||
         png_ptr->background_expand)) */
      {
         /* expand background chunk.  While this may not be
            the fastest way to do this, it only happens once
            per file. */
         switch (png_ptr->bit_depth)
         {
            case 1:
               png_ptr->background.gray *= (png_byte)0xff;
               break;
            case 2:
               png_ptr->background.gray *= (png_byte)0x55;
               break;
            case 4:
               png_ptr->background.gray *= (png_byte)0x11;
               break;
         }
      }
      if (color_type == PNG_COLOR_TYPE_PALETTE &&
         (png_ptr->transformations & PNG_BACKGROUND) &&
         png_ptr->background_expand)
      {
         /* expand background chunk */
         png_ptr->background.red =
            png_ptr->palette[png_ptr->background.index].red;
         png_ptr->background.green =
            png_ptr->palette[png_ptr->background.index].green;
         png_ptr->background.blue =
            png_ptr->palette[png_ptr->background.index].blue;
         color_type = PNG_COLOR_TYPE_RGB;
      }
   }
#endif

#if defined(PNG_READ_BACKGROUND_SUPPORTED)
   png_ptr->background_1 = png_ptr->background;
#endif
#if defined(PNG_READ_GAMMA_SUPPORTED)
   if (png_ptr->transformations & PNG_GAMMA)
   {
      png_build_gamma_table(png_ptr);
#if defined(PNG_READ_BACKGROUND_SUPPORTED)
      if ((png_ptr->transformations & PNG_BACKGROUND) &&
         (color_type != PNG_COLOR_TYPE_PALETTE))
      {
         if (png_ptr->background_gamma_type != PNG_BACKGROUND_GAMMA_UNKNOWN)
         {
            double g, gs, m;

            m = (double)(((png_uint_32)1 << png_ptr->bit_depth) - 1);
            g = 1.0;
            gs = 1.0;

            switch (png_ptr->background_gamma_type)
            {
               case PNG_BACKGROUND_GAMMA_SCREEN:
                  g = (png_ptr->display_gamma);
                  gs = 1.0;
                  break;
               case PNG_BACKGROUND_GAMMA_FILE:
                  g = 1.0 / (png_ptr->gamma);
                  gs = 1.0 / (png_ptr->gamma * png_ptr->display_gamma);
                  break;
               case PNG_BACKGROUND_GAMMA_UNIQUE:
                  g = 1.0 / (png_ptr->background_gamma);
                  gs = 1.0 / (png_ptr->background_gamma *
                     png_ptr->display_gamma);
                  break;
            }

            if (color_type & PNG_COLOR_MASK_COLOR)
            {
               png_ptr->background_1.red = (png_uint_16)(pow(
                  (double)png_ptr->background.red / m, g) * m + .5);
               png_ptr->background_1.green = (png_uint_16)(pow(
                  (double)png_ptr->background.green / m, g) * m + .5);
               png_ptr->background_1.blue = (png_uint_16)(pow(
                  (double)png_ptr->background.blue / m, g) * m + .5);
               png_ptr->background.red = (png_uint_16)(pow(
                  (double)png_ptr->background.red / m, gs) * m + .5);
               png_ptr->background.green = (png_uint_16)(pow(
                  (double)png_ptr->background.green / m, gs) * m + .5);
               png_ptr->background.blue = (png_uint_16)(pow(
                  (double)png_ptr->background.blue / m, gs) * m + .5);
            }
            else
            {
               png_ptr->background_1.gray = (png_uint_16)(pow(
                  (double)png_ptr->background.gray / m, g) * m + .5);
               png_ptr->background.gray = (png_uint_16)(pow(
                  (double)png_ptr->background.gray / m, gs) * m + .5);
            }
         }
      }
#endif
   }
#endif

#if defined(PNG_READ_SHIFT_SUPPORTED)
   if ((png_ptr->transformations & PNG_SHIFT) &&
      color_type == PNG_COLOR_TYPE_PALETTE)
   {
      png_uint_16 i;
      int sr, sg, sb;

      sr = 8 - png_ptr->sig_bit.red;
      if (sr < 0 || sr > 8)
         sr = 0;
      sg = 8 - png_ptr->sig_bit.green;
      if (sg < 0 || sg > 8)
         sg = 0;
      sb = 8 - png_ptr->sig_bit.blue;
      if (sb < 0 || sb > 8)
         sb = 0;
      for (i = 0; i < png_ptr->num_palette; i++)
      {
         png_ptr->palette[i].red >>= sr;
         png_ptr->palette[i].green >>= sg;
         png_ptr->palette[i].blue >>= sb;
      }
   }
#endif
}

/* modify the info structure to reflect the transformations.  The
   info should be updated so a png file could be written with it,
   assuming the transformations result in valid png data */
void
png_read_transform_info(png_structp png_ptr, png_infop info_ptr)
{
#if defined(PNG_READ_EXPAND_SUPPORTED)
   if ((png_ptr->transformations & PNG_EXPAND) &&
      info_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
   {
      if (png_ptr->num_trans)
         info_ptr->color_type = PNG_COLOR_TYPE_RGB_ALPHA;
      else
         info_ptr->color_type = PNG_COLOR_TYPE_RGB;
      info_ptr->bit_depth = 8;
      info_ptr->num_trans = 0;
   }
   else if (png_ptr->transformations & PNG_EXPAND)
   {
      if (png_ptr->num_trans)
         info_ptr->color_type |= PNG_COLOR_MASK_ALPHA;
      if (info_ptr->bit_depth < 8)
         info_ptr->bit_depth = 8;
      info_ptr->num_trans = 0;
   }
#endif

#if defined(PNG_READ_BACKGROUND_SUPPORTED)
   if (png_ptr->transformations & PNG_BACKGROUND)
   {
      info_ptr->color_type &= ~PNG_COLOR_MASK_ALPHA;
      info_ptr->num_trans = 0;
      info_ptr->background = png_ptr->background;
   }
#endif

#if defined(PNG_READ_16_TO_8_SUPPORTED)
   if ((png_ptr->transformations & PNG_16_TO_8) && info_ptr->bit_depth == 16)
      info_ptr->bit_depth = 8;
#endif

#if defined(PNG_READ_DITHER_SUPPORTED)
   if (png_ptr->transformations & PNG_DITHER)
   {
      if (((info_ptr->color_type == PNG_COLOR_TYPE_RGB) ||
         (info_ptr->color_type == PNG_COLOR_TYPE_RGB_ALPHA)) &&
         png_ptr->palette_lookup && info_ptr->bit_depth == 8)
      {
         info_ptr->color_type = PNG_COLOR_TYPE_PALETTE;
      }
   }
#endif

#if defined(PNG_READ_PACK_SUPPORTED)
   if ((png_ptr->transformations & PNG_PACK) && info_ptr->bit_depth < 8)
      info_ptr->bit_depth = 8;
#endif

#if defined(PNG_READ_GRAY_TO_RGB_SUPPORTED)
   if ((png_ptr->transformations & PNG_GRAY_TO_RGB) &&
      !(info_ptr->color_type & PNG_COLOR_MASK_COLOR))
      info_ptr->color_type |= PNG_COLOR_MASK_COLOR;
#endif
   if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      info_ptr->channels = 1;
   else if (info_ptr->color_type & PNG_COLOR_MASK_COLOR)
      info_ptr->channels = 3;
   else
      info_ptr->channels = 1;
   if (info_ptr->color_type & PNG_COLOR_MASK_ALPHA)
      info_ptr->channels++;
   info_ptr->pixel_depth = (png_byte)(info_ptr->channels *
      info_ptr->bit_depth);
   info_ptr->rowbytes = ((info_ptr->width * info_ptr->pixel_depth + 7) >> 3);
}

/* transform the row.  The order of transformations is significant,
   and is very touchy.  If you add a transformation, take care to
   decide how it fits in with the other transformations here */
void
png_do_read_transformations(png_structp png_ptr)
{
#if defined(PNG_READ_EXPAND_SUPPORTED)
   if ((png_ptr->transformations & PNG_EXPAND) &&
      png_ptr->row_info.color_type == PNG_COLOR_TYPE_PALETTE)
   {
      png_do_expand_palette(&(png_ptr->row_info), png_ptr->row_buf + 1,
         png_ptr->palette, png_ptr->trans, png_ptr->num_trans);
   }
   else if (png_ptr->transformations & PNG_EXPAND)
   {
      if (png_ptr->num_trans)
         png_do_expand(&(png_ptr->row_info), png_ptr->row_buf + 1,
            &(png_ptr->trans_values));
      else
         png_do_expand(&(png_ptr->row_info), png_ptr->row_buf + 1,
            NULL);
   }
#endif

#if defined(PNG_READ_BACKGROUND_SUPPORTED)
   if (png_ptr->transformations & PNG_BACKGROUND)
      png_do_background(&(png_ptr->row_info), png_ptr->row_buf + 1,
         &(png_ptr->trans_values), &(png_ptr->background),
         &(png_ptr->background_1),
         png_ptr->gamma_table, png_ptr->gamma_from_1,
         png_ptr->gamma_to_1, png_ptr->gamma_16_table,
         png_ptr->gamma_16_from_1, png_ptr->gamma_16_to_1,
         png_ptr->gamma_shift);
#endif

#if defined(PNG_READ_GAMMA_SUPPORTED)
   if ((png_ptr->transformations & PNG_GAMMA) &&
      !(png_ptr->transformations & PNG_BACKGROUND))
      png_do_gamma(&(png_ptr->row_info), png_ptr->row_buf + 1,
         png_ptr->gamma_table, png_ptr->gamma_16_table,
         png_ptr->gamma_shift);
#endif

#if defined(PNG_READ_16_TO_8_SUPPORTED)
   if (png_ptr->transformations & PNG_16_TO_8)
      png_do_chop(&(png_ptr->row_info), png_ptr->row_buf + 1);
#endif

#if defined(PNG_READ_DITHER_SUPPORTED)
   if (png_ptr->transformations & PNG_DITHER)
   {
      png_do_dither((png_row_infop)&(png_ptr->row_info),
         png_ptr->row_buf + 1,
         png_ptr->palette_lookup,
         png_ptr->dither_index);
   }
#endif

#if defined(PNG_READ_INVERT_SUPPORTED)
   if (png_ptr->transformations & PNG_INVERT_MONO)
      png_do_invert(&(png_ptr->row_info), png_ptr->row_buf + 1);
#endif

#if defined(PNG_READ_SHIFT_SUPPORTED)
   if (png_ptr->transformations & PNG_SHIFT)
      png_do_unshift(&(png_ptr->row_info), png_ptr->row_buf + 1,
         &(png_ptr->shift));
#endif

#if defined(PNG_READ_PACK_SUPPORTED)
   if (png_ptr->transformations & PNG_PACK)
      png_do_unpack(&(png_ptr->row_info), png_ptr->row_buf + 1);
#endif

#if defined(PNG_READ_BGR_SUPPORTED)
   if (png_ptr->transformations & PNG_BGR)
      png_do_bgr(&(png_ptr->row_info), png_ptr->row_buf + 1);
#endif

#if defined(PNG_READ_GRAY_TO_RGB_SUPPORTED)
   if (png_ptr->transformations & PNG_GRAY_TO_RGB)
      png_do_gray_to_rgb(&(png_ptr->row_info), png_ptr->row_buf + 1);
#endif

#if defined(PNG_READ_SWAP_SUPPORTED)
   if (png_ptr->transformations & PNG_SWAP_BYTES)
      png_do_swap(&(png_ptr->row_info), png_ptr->row_buf + 1);
#endif

#if defined(PNG_READ_FILLER_SUPPORTED)
   if (png_ptr->transformations & PNG_FILLER)
      png_do_read_filler(&(png_ptr->row_info), png_ptr->row_buf + 1,
         png_ptr->filler, png_ptr->filler_loc);
#endif
}

#if defined(PNG_READ_PACK_SUPPORTED)
/* unpack pixels of 1, 2, or 4 bits per pixel into 1 byte per pixel,
   without changing the actual values.  Thus, if you had a row with
   a bit depth of 1, you would end up with bytes that only contained
   the numbers 0 or 1.  If you would rather they contain 0 and 255, use
   png_do_shift() after this. */
void
png_do_unpack(png_row_infop row_info, png_bytep row)
{
   int shift;
   png_bytep sp, dp;
   png_uint_32 i;

   if (row && row_info && row_info->bit_depth < 8)
   {
      switch (row_info->bit_depth)
      {
         case 1:
         {
            sp = row + (png_size_t)((row_info->width - 1) >> 3);
            dp = row + (png_size_t)row_info->width - 1;
            shift = 7 - (int)((row_info->width + 7) & 7);
            for (i = 0; i < row_info->width; i++)
            {
               *dp = (png_byte)((*sp >> shift) & 0x1);
               if (shift == 7)
               {
                  shift = 0;
                  sp--;
               }
               else
                  shift++;

               dp--;
            }
            break;
         }
         case 2:
         {

            sp = row + (png_size_t)((row_info->width - 1) >> 2);
            dp = row + (png_size_t)row_info->width - 1;
            shift = (int)((3 - ((row_info->width + 3) & 3)) << 1);
            for (i = 0; i < row_info->width; i++)
            {
               *dp = (png_byte)((*sp >> shift) & 0x3);
               if (shift == 6)
               {
                  shift = 0;
                  sp--;
               }
               else
                  shift += 2;

               dp--;
            }
            break;
         }
         case 4:
         {
            sp = row + (png_size_t)((row_info->width - 1) >> 1);
            dp = row + (png_size_t)row_info->width - 1;
            shift = (int)((1 - ((row_info->width + 1) & 1)) << 2);
            for (i = 0; i < row_info->width; i++)
            {
               *dp = (png_byte)((*sp >> shift) & 0xf);
               if (shift == 4)
               {
                  shift = 0;
                  sp--;
               }
               else
                  shift = 4;

               dp--;
            }
            break;
         }
      }
      row_info->bit_depth = 8;
      row_info->pixel_depth = (png_byte)(8 * row_info->channels);
      row_info->rowbytes = row_info->width * row_info->channels;
   }
}
#endif

#if defined(PNG_READ_SHIFT_SUPPORTED)
/* reverse the effects of png_do_shift.  This routine merely shifts the
   pixels back to their significant bits values.  Thus, if you have
   a row of bit depth 8, but only 5 are significant, this will shift
   the values back to 0 through 31 */
void
png_do_unshift(png_row_infop row_info, png_bytep row,
   png_color_8p sig_bits)
{
   png_bytep bp;
   png_uint_16 value;
   png_uint_32 i;
   if (row && row_info && sig_bits &&
      row_info->color_type != PNG_COLOR_TYPE_PALETTE)
   {
      int shift[4];
      int channels;

      channels = 0;
      if (row_info->color_type & PNG_COLOR_MASK_COLOR)
      {
         shift[channels++] = row_info->bit_depth - sig_bits->red;
         shift[channels++] = row_info->bit_depth - sig_bits->green;
         shift[channels++] = row_info->bit_depth - sig_bits->blue;
      }
      else
      {
         shift[channels++] = row_info->bit_depth - sig_bits->gray;
      }
      if (row_info->color_type & PNG_COLOR_MASK_ALPHA)
      {
         shift[channels++] = row_info->bit_depth - sig_bits->alpha;
      }

      value = 0;

      for (i = 0; i < channels; i++)
      {
         if (shift[(png_size_t)i] <= 0)
            shift[(png_size_t)i] = 0;
         else
            value = 1;
      }

      if (!value)
         return;

      switch (row_info->bit_depth)
      {
         case 2:
         {
            for (bp = row, i = 0;
               i < row_info->rowbytes;
               i++, bp++)
            {
               *bp >>= 1;
               *bp &= 0x55;
            }
            break;
         }
         case 4:
         {
            png_byte  mask;
            mask = (png_byte)(((int)0xf0 >> shift[0]) & (int)0xf0) |
               (png_byte)((int)0xf >> shift[0]);
            for (bp = row, i = 0;
               i < row_info->rowbytes;
               i++, bp++)
            {
               *bp >>= shift[0];
               *bp &= mask;
            }
            break;
         }
         case 8:
         {
            for (bp = row, i = 0;
               i < row_info->width; i++)
            {
               int c;

               for (c = 0; c < row_info->channels; c++, bp++)
               {
                  *bp >>= shift[c];
               }
            }
            break;
         }
         case 16:
         {
            for (bp = row, i = 0;
               i < row_info->width; i++)
            {
               int c;

               for (c = 0; c < row_info->channels; c++, bp += 2)
               {
                  value = (png_uint_16)((*bp << 8) + *(bp + 1));
                  value >>= shift[c];
                  *bp = (png_byte)(value >> 8);
                  *(bp + 1) = (png_byte)(value & 0xff);
               }
            }
            break;
         }
      }
   }
}
#endif

#if defined(PNG_READ_16_TO_8_SUPPORTED)
/* chop rows of bit depth 16 down to 8 */
void
png_do_chop(png_row_infop row_info, png_bytep row)
{
   png_bytep sp, dp;
   png_uint_32 i;
   if (row && row_info && row_info->bit_depth == 16)
   {
      sp = row;
      dp = row;
      for (i = 0; i < row_info->width * row_info->channels; i++)
      {
         *dp = *sp;
/* not yet, as I'm afraid of overflow here
         *dp = ((((((png_uint_16)(*sp) << 8)) |
            (png_uint_16)((*(sp + 1) - *sp) & 0xff) +
            0x7f) >> 8) & 0xff);
*/
         sp += 2;
         dp++;
      }
      row_info->bit_depth = 8;
      row_info->pixel_depth = (png_byte)(8 * row_info->channels);
      row_info->rowbytes = row_info->width * row_info->channels;
   }
}
#endif

#if defined(PNG_READ_FILLER_SUPPORTED)
/* add filler byte */
void
png_do_read_filler(png_row_infop row_info, png_bytep row,
   png_byte filler, png_byte filler_loc)
{
   png_bytep sp, dp;
   png_uint_32 i;
   if (row && row_info && row_info->color_type == 2 &&
      row_info->bit_depth == 8)
   {
      if (filler_loc == PNG_FILLER_AFTER)
      {
         for (i = 1, sp = row + (png_size_t)row_info->width * 3,
            dp = row + (png_size_t)row_info->width * 4;
            i < row_info->width;
            i++)
         {
            *(--dp) = filler;
            *(--dp) = *(--sp);
            *(--dp) = *(--sp);
            *(--dp) = *(--sp);
         }
         *(--dp) = filler;
         row_info->channels = 4;
         row_info->pixel_depth = 32;
         row_info->rowbytes = row_info->width * 4;
      }
      else
      {
         for (i = 0, sp = row + (png_size_t)row_info->width * 3,
            dp = row + (png_size_t)row_info->width * 4;
            i < row_info->width;
            i++)
         {
            *(--dp) = *(--sp);
            *(--dp) = *(--sp);
            *(--dp) = *(--sp);
            *(--dp) = filler;
         }
         row_info->channels = 4;
         row_info->pixel_depth = 32;
         row_info->rowbytes = row_info->width * 4;
      }
   }
}
#endif

#if defined(PNG_READ_GRAY_TO_RGB_SUPPORTED)
/* expand grayscale files to rgb, with or without alpha */
void
png_do_gray_to_rgb(png_row_infop row_info, png_bytep row)
{
   png_bytep sp, dp;
   png_uint_32 i;

   if (row && row_info && row_info->bit_depth >= 8 &&
      !(row_info->color_type & PNG_COLOR_MASK_COLOR))
   {
      if (row_info->color_type == PNG_COLOR_TYPE_GRAY)
      {
         if (row_info->bit_depth == 8)
         {
            for (i = 0, sp = row + (png_size_t)row_info->width - 1,
               dp = row + (png_size_t)row_info->width * 3 - 1;
               i < row_info->width;
               i++)
            {
               *(dp--) = *sp;
               *(dp--) = *sp;
               *(dp--) = *sp;
               sp--;
            }
         }
         else
         {
            for (i = 0, sp = row + (png_size_t)row_info->width * 2 - 1,
               dp = row + (png_size_t)row_info->width * 6 - 1;
               i < row_info->width;
               i++)
            {
               *(dp--) = *sp;
               *(dp--) = *(sp - 1);
               *(dp--) = *sp;
               *(dp--) = *(sp - 1);
               *(dp--) = *sp;
               *(dp--) = *(sp - 1);
               sp--;
               sp--;
            }
         }
      }
      else if (row_info->color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
      {
         if (row_info->bit_depth == 8)
         {
            for (i = 0, sp = row + (png_size_t)row_info->width * 2 - 1,
               dp = row + (png_size_t)row_info->width * 4 - 1;
               i < row_info->width;
               i++)
            {
               *(dp--) = *(sp--);
               *(dp--) = *sp;
               *(dp--) = *sp;
               *(dp--) = *sp;
               sp--;
            }
         }
         else
         {
            for (i = 0, sp = row + (png_size_t)row_info->width * 4 - 1,
               dp = row + (png_size_t)row_info->width * 8 - 1;
               i < row_info->width;
               i++)
            {
               *(dp--) = *(sp--);
               *(dp--) = *(sp--);
               *(dp--) = *sp;
               *(dp--) = *(sp - 1);
               *(dp--) = *sp;
               *(dp--) = *(sp - 1);
               *(dp--) = *sp;
               *(dp--) = *(sp - 1);
               sp--;
               sp--;
            }
         }
      }
      row_info->channels += (png_byte)2;
      row_info->color_type |= PNG_COLOR_MASK_COLOR;
      row_info->pixel_depth = (png_byte)(row_info->channels *
         row_info->bit_depth);
      row_info->rowbytes = ((row_info->width *
         row_info->pixel_depth + 7) >> 3);
   }
}
#endif

/* build a grayscale palette.  Palette is assumed to be 1 << bit_depth
   large of png_color.  This lets grayscale images be treated as
   paletted.  Most useful for gamma correction and simplification
   of code. */
void
png_build_grayscale_palette(int bit_depth, png_colorp palette)
{
   int num_palette;
   int color_inc;
   int i;
   int v;

   if (!palette)
      return;

   switch (bit_depth)
   {
      case 1:
         num_palette = 2;
         color_inc = 0xff;
         break;
      case 2:
         num_palette = 4;
         color_inc = 0x55;
         break;
      case 4:
         num_palette = 16;
         color_inc = 0x11;
         break;
      case 8:
         num_palette = 256;
         color_inc = 1;
         break;
      default:
         num_palette = 0;
         color_inc = 0;
         break;
   }

   for (i = 0, v = 0; i < num_palette; i++, v += color_inc)
   {
      palette[i].red = (png_byte)v;
      palette[i].green = (png_byte)v;
      palette[i].blue = (png_byte)v;
   }
}

#if defined(PNG_READ_DITHER_SUPPORTED)
void
png_correct_palette(png_structp png_ptr, png_colorp palette,
   int num_palette)
{
   if ((png_ptr->transformations & (PNG_GAMMA)) &&
      (png_ptr->transformations & (PNG_BACKGROUND)))
   {
      if (png_ptr->color_type == 3)
      {
         int i;
         png_color back, back_1;

         back.red = png_ptr->gamma_table[png_ptr->palette[
            png_ptr->background.index].red];
         back.green = png_ptr->gamma_table[png_ptr->palette[
            png_ptr->background.index].green];
         back.blue = png_ptr->gamma_table[png_ptr->palette[
            png_ptr->background.index].blue];

         back_1.red = png_ptr->gamma_to_1[png_ptr->palette[
            png_ptr->background.index].red];
         back_1.green = png_ptr->gamma_to_1[png_ptr->palette[
            png_ptr->background.index].green];
         back_1.blue = png_ptr->gamma_to_1[png_ptr->palette[
            png_ptr->background.index].blue];

         for (i = 0; i < num_palette; i++)
         {
            if (i < (int)png_ptr->num_trans &&
               png_ptr->trans[i] == 0)
            {
               palette[i] = back;
            }
            else if (i < (int)png_ptr->num_trans &&
               png_ptr->trans[i] != 0xff)
            {
               int v;

               v = png_ptr->gamma_to_1[png_ptr->palette[i].red];
               v = (int)(((png_uint_32)(v) *
                  (png_uint_32)(png_ptr->trans[i]) +
                  (png_uint_32)(back_1.red) *
                  (png_uint_32)(255 - png_ptr->trans[i]) +
                  127) / 255);
               palette[i].red = png_ptr->gamma_from_1[v];

               v = png_ptr->gamma_to_1[png_ptr->palette[i].green];
               v = (int)(((png_uint_32)(v) *
                  (png_uint_32)(png_ptr->trans[i]) +
                  (png_uint_32)(back_1.green) *
                  (png_uint_32)(255 - png_ptr->trans[i]) +
                  127) / 255);
               palette[i].green = png_ptr->gamma_from_1[v];

               v = png_ptr->gamma_to_1[png_ptr->palette[i].blue];
               v = (int)(((png_uint_32)(v) *
                  (png_uint_32)(png_ptr->trans[i]) +
                  (png_uint_32)(back_1.blue) *
                  (png_uint_32)(255 - png_ptr->trans[i]) +
                  127) / 255);
               palette[i].blue = png_ptr->gamma_from_1[v];
            }
            else
            {
               palette[i].red = png_ptr->gamma_table[palette[i].red];
               palette[i].green = png_ptr->gamma_table[palette[i].green];
               palette[i].blue = png_ptr->gamma_table[palette[i].blue];
            }
         }
      }
      else
      {
         int i, back;

         back = png_ptr->gamma_table[png_ptr->background.gray];

         for (i = 0; i < num_palette; i++)
         {
            if (palette[i].red == png_ptr->trans_values.gray)
            {
               palette[i].red = (png_byte)back;
               palette[i].green = (png_byte)back;
               palette[i].blue = (png_byte)back;
            }
            else
            {
               palette[i].red = png_ptr->gamma_table[palette[i].red];
               palette[i].green = png_ptr->gamma_table[palette[i].green];
               palette[i].blue = png_ptr->gamma_table[palette[i].blue];
            }
         }
      }
   }
   else if (png_ptr->transformations & (PNG_GAMMA))
   {
      int i;

      for (i = 0; i < num_palette; i++)
      {
         palette[i].red = png_ptr->gamma_table[palette[i].red];
         palette[i].green = png_ptr->gamma_table[palette[i].green];
         palette[i].blue = png_ptr->gamma_table[palette[i].blue];
      }
   }
   else if (png_ptr->transformations & (PNG_BACKGROUND))
   {
      if (png_ptr->color_type == 3)
      {
         int i;
         png_byte br, bg, bb;

         br = palette[png_ptr->background.index].red;
         bg = palette[png_ptr->background.index].green;
         bb = palette[png_ptr->background.index].blue;

         for (i = 0; i < num_palette; i++)
         {
            if (i >= (int)png_ptr->num_trans ||
               png_ptr->trans[i] == 0)
            {
               palette[i].red = br;
               palette[i].green = bg;
               palette[i].blue = bb;
            }
            else if (i < (int)png_ptr->num_trans ||
               png_ptr->trans[i] != 0xff)
            {
               palette[i].red = (png_byte)((
                  (png_uint_32)(png_ptr->palette[i].red) *
                  (png_uint_32)(png_ptr->trans[i]) +
                  (png_uint_32)(br) *
                  (png_uint_32)(255 - png_ptr->trans[i]) +
                  127) / 255);
               palette[i].green = (png_byte)((
                  (png_uint_32)(png_ptr->palette[i].green) *
                  (png_uint_32)(png_ptr->trans[i]) +
                  (png_uint_32)(bg) *
                  (png_uint_32)(255 - png_ptr->trans[i]) +
                  127) / 255);
               palette[i].blue = (png_byte)((
                  (png_uint_32)(png_ptr->palette[i].blue) *
                  (png_uint_32)(png_ptr->trans[i]) +
                  (png_uint_32)(bb) *
                  (png_uint_32)(255 - png_ptr->trans[i]) +
                  127) / 255);
            }
         }
      }
      else /* assume grayscale palette (what else could it be?) */
      {
         int i;

         for (i = 0; i < num_palette; i++)
         {
            if (i == (int)png_ptr->trans_values.gray)
            {
               palette[i].red = (png_byte)png_ptr->background.gray;
               palette[i].green = (png_byte)png_ptr->background.gray;
               palette[i].blue = (png_byte)png_ptr->background.gray;
            }
         }
      }
   }
}
#endif

#if defined(PNG_READ_BACKGROUND_SUPPORTED)
/* replace any alpha or transparency with the supplied background color.
   background is the color (in rgb or grey or palette index, as
   appropriate).  note that paletted files are taken care of elsewhere */
void
png_do_background(png_row_infop row_info, png_bytep row,
   png_color_16p trans_values, png_color_16p background,
   png_color_16p background_1,
   png_bytep gamma_table, png_bytep gamma_from_1, png_bytep gamma_to_1,
   png_uint_16pp gamma_16, png_uint_16pp gamma_16_from_1,
   png_uint_16pp gamma_16_to_1, int gamma_shift)
{
   png_bytep sp, dp;
   png_uint_32 i;

   int shift;
   if (row && row_info && background &&
      (!(row_info->color_type & PNG_COLOR_MASK_ALPHA) ||
      (row_info->color_type != PNG_COLOR_TYPE_PALETTE &&
      trans_values)))
   {
      switch (row_info->color_type)
      {
         case PNG_COLOR_TYPE_GRAY:
         {
            switch (row_info->bit_depth)
            {
               case 1:
               {
                  sp = row;
                  shift = 7;
                  for (i = 0; i < row_info->width; i++)
                  {
                     if (((*sp >> shift) & 0x1) ==
                        trans_values->gray)
                     {
                        *sp &= (png_byte)((0x7f7f >> (7 - shift)) & 0xff);
                        *sp |= (png_byte)(background->gray << shift);
                     }
                     if (!shift)
                     {
                        shift = 7;
                        sp++;
                     }
                     else
                        shift--;
                  }
                  break;
               }
               case 2:
               {
                  sp = row;
                  shift = 6;
                  for (i = 0; i < row_info->width; i++)
                  {
                     if (((*sp >> shift) & 0x3) ==
                        trans_values->gray)
                     {
                        *sp &= (png_byte)((0x3f3f >> (6 - shift)) & 0xff);
                        *sp |= (png_byte)(background->gray << shift);
                     }
                     if (!shift)
                     {
                        shift = 6;
                        sp++;
                     }
                     else
                        shift -= 2;
                  }
                  break;
               }
               case 4:
               {
                  sp = row + 1;
                  shift = 4;
                  for (i = 0; i < row_info->width; i++)
                  {
                     if (((*sp >> shift) & 0xf) ==
                        trans_values->gray)
                     {
                        *sp &= (png_byte)((0xf0f >> (4 - shift)) & 0xff);
                        *sp |= (png_byte)(background->gray << shift);
                     }
                     if (!shift)
                     {
                        shift = 4;
                        sp++;
                     }
                     else
                        shift -= 4;
                  }
                  break;
               }
               case 8:
               {
#if defined(PNG_READ_GAMMA_SUPPORTED)
                  if (gamma_table)
                  {

                     for (i = 0, sp = row;
                        i < row_info->width; i++, sp++)
                     {
                        if (*sp == trans_values->gray)
                        {
                           *sp = background->gray;
                        }
                        else
                        {
                           *sp = gamma_table[*sp];
                        }
                     }
                  }
                  else
#endif
                  {
                     for (i = 0, sp = row;
                        i < row_info->width; i++, sp++)
                     {
                        if (*sp == trans_values->gray)
                        {
                           *sp = background->gray;
                        }
                     }
                  }
                  break;
               }
               case 16:
               {
#if defined(PNG_READ_GAMMA_SUPPORTED)
                  if (gamma_16)
                  {
                     for (i = 0, sp = row;
                        i < row_info->width; i++, sp += 2)
                     {
                        png_uint_16 v;

                        v = (png_uint_16)(((png_uint_16)(*sp) << 8) +
                           (png_uint_16)(*(sp + 1)));
                        if (v == trans_values->gray)
                        {
                           *sp = (png_byte)((background->gray >> 8) & 0xff);
                           *(sp + 1) = (png_byte)(background->gray & 0xff);
                        }
                        else
                        {
                           v = gamma_16[
                              *(sp + 1) >> gamma_shift][*sp];
                           *sp = (png_byte)((v >> 8) & 0xff);
                           *(sp + 1) = (png_byte)(v & 0xff);
                        }
                     }
                  }
                  else
#endif
                  {
                     for (i = 0, sp = row;
                        i < row_info->width; i++, sp += 2)
                     {
                        png_uint_16 v;

                        v = (png_uint_16)(((png_uint_16)(*sp) << 8) +
                           (png_uint_16)(*(sp + 1)));
                        if (v == trans_values->gray)
                        {
                           *sp = (png_byte)((background->gray >> 8) & 0xff);
                           *(sp + 1) = (png_byte)(background->gray & 0xff);
                        }
                     }
                  }
                  break;
               }
            }
            break;
         }
         case PNG_COLOR_TYPE_RGB:
         {
            if (row_info->bit_depth == 8)
            {
#if defined(PNG_READ_GAMMA_SUPPORTED)
               if (gamma_table)
               {
                  for (i = 0, sp = row;
                     i < row_info->width; i++, sp += 3)
                  {
                     if (*sp == trans_values->red &&
                        *(sp + 1) == trans_values->green &&
                        *(sp + 2) == trans_values->blue)
                     {
                        *sp = background->red;
                        *(sp + 1) = background->green;
                        *(sp + 2) = background->blue;
                     }
                     else
                     {
                        *sp = gamma_table[*sp];
                        *(sp + 1) = gamma_table[*(sp + 1)];
                        *(sp + 2) = gamma_table[*(sp + 2)];
                     }
                  }
               }
               else
#endif
               {
                  for (i = 0, sp = row;
                     i < row_info->width; i++, sp += 3)
                  {
                     if (*sp == trans_values->red &&
                        *(sp + 1) == trans_values->green &&
                        *(sp + 2) == trans_values->blue)
                     {
                        *sp = background->red;
                        *(sp + 1) = background->green;
                        *(sp + 2) = background->blue;
                     }
                  }
               }
            }
            else if (row_info->bit_depth == 16)
            {
#if defined(PNG_READ_GAMMA_SUPPORTED)
               if (gamma_16)
               {
                  for (i = 0, sp = row;
                     i < row_info->width; i++, sp += 6)
                  {
                     png_uint_16 r, g, b;

                     r = (png_uint_16)(((png_uint_16)(*sp) << 8) +
                        (png_uint_16)(*(sp + 1)));
                     g = (png_uint_16)(((png_uint_16)(*(sp + 2)) << 8) +
                        (png_uint_16)(*(sp + 3)));
                     b = (png_uint_16)(((png_uint_16)(*(sp + 4)) << 8) +
                        (png_uint_16)(*(sp + 5)));
                     if (r == trans_values->red &&
                        g == trans_values->green &&
                        b == trans_values->blue)
                     {
                        *sp = (png_byte)((background->red >> 8) & 0xff);
                        *(sp + 1) = (png_byte)(background->red & 0xff);
                        *(sp + 2) = (png_byte)((background->green >> 8) & 0xff);
                        *(sp + 3) = (png_byte)(background->green & 0xff);
                        *(sp + 4) = (png_byte)((background->blue >> 8) & 0xff);
                        *(sp + 5) = (png_byte)(background->blue & 0xff);
                     }
                     else
                     {
                        png_uint_16 v;
                        v = gamma_16[
                           *(sp + 1) >> gamma_shift][*sp];
                        *sp = (png_byte)((v >> 8) & 0xff);
                        *(sp + 1) = (png_byte)(v & 0xff);
                        v = gamma_16[
                           *(sp + 3) >> gamma_shift][*(sp + 2)];
                        *(sp + 2) = (png_byte)((v >> 8) & 0xff);
                        *(sp + 3) = (png_byte)(v & 0xff);
                        v = gamma_16[
                           *(sp + 5) >> gamma_shift][*(sp + 4)];
                        *(sp + 4) = (png_byte)((v >> 8) & 0xff);
                        *(sp + 5) = (png_byte)(v & 0xff);
                     }
                  }
               }
               else
#endif
               {
                  for (i = 0, sp = row;
                     i < row_info->width; i++, sp += 6)
                  {
                     png_uint_16 r, g, b;

                     r = (png_uint_16)(((png_uint_16)(*sp) << 8) +
                        (png_uint_16)(*(sp + 1)));
                     g = (png_uint_16)(((png_uint_16)(*(sp + 2)) << 8) +
                        (png_uint_16)(*(sp + 3)));
                     b = (png_uint_16)(((png_uint_16)(*(sp + 4)) << 8) +
                        (png_uint_16)(*(sp + 5)));
                     if (r == trans_values->red &&
                        g == trans_values->green &&
                        b == trans_values->blue)
                     {
                        *sp = (png_byte)((background->red >> 8) & 0xff);
                        *(sp + 1) = (png_byte)(background->red & 0xff);
                        *(sp + 2) = (png_byte)((background->green >> 8) & 0xff);
                        *(sp + 3) = (png_byte)(background->green & 0xff);
                        *(sp + 4) = (png_byte)((background->blue >> 8) & 0xff);
                        *(sp + 5) = (png_byte)(background->blue & 0xff);
                     }
                  }
               }
            }
            break;
         }
         case PNG_COLOR_TYPE_GRAY_ALPHA:
         {
            switch (row_info->bit_depth)
            {
               case 8:
               {
#if defined(PNG_READ_GAMMA_SUPPORTED)
                  if (gamma_to_1 && gamma_from_1 && gamma_table)
                  {
                     for (i = 0, sp = row,
                        dp = row;
                        i < row_info->width; i++, sp += 2, dp++)
                     {
                        png_uint_16 a;

                        a = *(sp + 1);
                        if (a == 0xff)
                        {
                           *dp = gamma_table[*sp];
                        }
                        else if (a == 0)
                        {
                           *dp = background->gray;
                        }
                        else
                        {
                           png_uint_16 v;

                           v = gamma_to_1[*sp];
                           v = (png_uint_16)(((png_uint_16)(v) * a +
                              (png_uint_16)background_1->gray *
                              (255 - a) + 127) / 255);
                           *dp = gamma_from_1[v];
                        }
                     }
                  }
                  else
#endif
                  {
                     for (i = 0, sp = row,
                        dp = row;
                        i < row_info->width; i++, sp += 2, dp++)
                     {
                        png_uint_16 a;

                        a = *(sp + 1);
                        if (a == 0xff)
                        {
                           *dp = *sp;
                        }
                        else if (a == 0)
                        {
                           *dp = background->gray;
                        }
                        else
                        {
                           *dp = (png_byte)(((png_uint_16)(*sp) * a +
                              (png_uint_16)background_1->gray *
                              (255 - a) + 127) / 255);
                        }
                     }
                  }
                  break;
               }
               case 16:
               {
#if defined(PNG_READ_GAMMA_SUPPORTED)
                  if (gamma_16 && gamma_16_from_1 && gamma_16_to_1)
                  {
                     for (i = 0, sp = row,
                        dp = row;
                        i < row_info->width; i++, sp += 4, dp += 2)
                     {
                        png_uint_16 a;

                        a = (png_uint_16)(((png_uint_16)(*(sp + 2)) << 8) +
                           (png_uint_16)(*(sp + 3)));
                        if (a == (png_uint_16)0xffff)
                        {
                           png_uint_32 v;

                           v = gamma_16[
                              *(sp + 1) >> gamma_shift][*sp];
                           *dp = (png_byte)((v >> 8) & 0xff);
                           *(dp + 1) = (png_byte)(v & 0xff);
                        }
                        else if (a == 0)
                        {
                           *dp = (png_byte)((background->gray >> 8) & 0xff);
                           *(dp + 1) = (png_byte)(background->gray & 0xff);
                        }
                        else
                        {
                           png_uint_32 g, v;

                           g = gamma_16_to_1[
                              *(sp + 1) >> gamma_shift][*sp];
                           v = (g * (png_uint_32)a +
                              (png_uint_32)background_1->gray *
                              (png_uint_32)((png_uint_16)65535L - a) +
                              (png_uint_16)32767) / (png_uint_16)65535L;
                           v = gamma_16_from_1[(size_t)(
                              (v & 0xff) >> gamma_shift)][(size_t)(v >> 8)];
                           *dp = (png_byte)((v >> 8) & 0xff);
                           *(dp + 1) = (png_byte)(v & 0xff);
                        }
                     }
                  }
                  else
#endif
                  {
                     for (i = 0, sp = row,
                        dp = row;
                        i < row_info->width; i++, sp += 4, dp += 2)
                     {
                        png_uint_16 a;

                        a = (png_uint_16)(((png_uint_16)(*(sp + 2)) << 8) +
                           (png_uint_16)(*(sp + 3)));
                        if (a == (png_uint_16)0xffff)
                        {
                           png_memcpy(dp, sp, 2);
                        }
                        else if (a == 0)
                        {
                           *dp = (png_byte)((background->gray >> 8) & 0xff);
                           *(dp + 1) = (png_byte)(background->gray & 0xff);
                        }
                        else
                        {
                           png_uint_32 g, v;

                           g = ((png_uint_32)(*sp) << 8) +
                              (png_uint_32)(*(sp + 1));
                           v = (g * (png_uint_32)a +
                              (png_uint_32)background_1->gray *
                              (png_uint_32)((png_uint_16)65535L - a) +
                              (png_uint_16)32767) / (png_uint_16)65535L;
                           *dp = (png_byte)((v >> 8) & 0xff);
                           *(dp + 1) = (png_byte)(v & 0xff);
                        }
                     }
                  }
                  break;
               }
            }
            break;
         }
         case PNG_COLOR_TYPE_RGB_ALPHA:
         {
            if (row_info->bit_depth == 8)
            {
#if defined(PNG_READ_GAMMA_SUPPORTED)
               if (gamma_to_1 && gamma_from_1 && gamma_table)
               {
                  for (i = 0, sp = row,
                     dp = row;
                     i < row_info->width; i++, sp += 4, dp += 3)
                  {
                     png_uint_16 a;

                     a = *(sp + 3);
                     if (a == 0xff)
                     {
                        *dp = gamma_table[*sp];
                        *(dp + 1) = gamma_table[*(sp + 1)];
                        *(dp + 2) = gamma_table[*(sp + 2)];
                     }
                     else if (a == 0)
                     {
                        *dp = background->red;
                        *(dp + 1) = background->green;
                        *(dp + 2) = background->blue;
                     }
                     else
                     {
                        png_uint_16 v;

                        v = gamma_to_1[*sp];
                        v = (png_uint_16)(((png_uint_16)(v) * a +
                           (png_uint_16)background_1->red *
                           (255 - a) + 127) / 255);
                        *dp = gamma_from_1[v];
                        v = gamma_to_1[*(sp + 1)];
                        v = (png_uint_16)(((png_uint_16)(v) * a +
                           (png_uint_16)background_1->green *
                           (255 - a) + 127) / 255);
                        *(dp + 1) = gamma_from_1[v];
                        v = gamma_to_1[*(sp + 2)];
                        v = (png_uint_16)(((png_uint_16)(v) * a +
                           (png_uint_16)background_1->blue *
                           (255 - a) + 127) / 255);
                        *(dp + 2) = gamma_from_1[v];
                     }
                  }
               }
               else
#endif
               {
                  for (i = 0, sp = row,
                     dp = row;
                     i < row_info->width; i++, sp += 4, dp += 3)
                  {
                     png_uint_16 a;

                     a = *(sp + 3);
                     if (a == 0xff)
                     {
                        *dp = *sp;
                        *(dp + 1) = *(sp + 1);
                        *(dp + 2) = *(sp + 2);
                     }
                     else if (a == 0)
                     {
                        *dp = background->red;
                        *(dp + 1) = background->green;
                        *(dp + 2) = background->blue;
                     }
                     else
                     {
                        *dp = (png_byte)(((png_uint_16)(*sp) * a +
                           (png_uint_16)background->red *
                           (255 - a) + 127) / 255);
                        *(dp + 1) = (png_byte)(((png_uint_16)(*(sp + 1)) * a +
                           (png_uint_16)background->green *
                           (255 - a) + 127) / 255);
                        *(dp + 2) = (png_byte)(((png_uint_16)(*(sp + 2)) * a +
                           (png_uint_16)background->blue *
                           (255 - a) + 127) / 255);
                     }
                  }
               }
            }
            else if (row_info->bit_depth == 16)
            {
#if defined(PNG_READ_GAMMA_SUPPORTED)
               if (gamma_16 && gamma_16_from_1 && gamma_16_to_1)
               {
                  for (i = 0, sp = row,
                     dp = row;
                     i < row_info->width; i++, sp += 8, dp += 6)
                  {
                     png_uint_16 a;

                     a = (png_uint_16)(((png_uint_16)(*(sp + 6)) << 8) +
                        (png_uint_16)(*(sp + 7)));
                     if (a == (png_uint_16)0xffff)
                     {
                        png_uint_16 v;

                        v = gamma_16[
                           *(sp + 1) >> gamma_shift][*sp];
                        *dp = (png_byte)((v >> 8) & 0xff);
                        *(dp + 1) = (png_byte)(v & 0xff);
                        v = gamma_16[
                           *(sp + 3) >> gamma_shift][*(sp + 2)];
                        *(dp + 2) = (png_byte)((v >> 8) & 0xff);
                        *(dp + 3) = (png_byte)(v & 0xff);
                        v = gamma_16[
                           *(sp + 5) >> gamma_shift][*(sp + 4)];
                        *(dp + 4) = (png_byte)((v >> 8) & 0xff);
                        *(dp + 5) = (png_byte)(v & 0xff);
                     }
                     else if (a == 0)
                     {
                        *dp = (png_byte)((background->red >> 8) & 0xff);
                        *(dp + 1) = (png_byte)(background->red & 0xff);
                        *(dp + 2) = (png_byte)((background->green >> 8) & 0xff);
                        *(dp + 3) = (png_byte)(background->green & 0xff);
                        *(dp + 4) = (png_byte)((background->blue >> 8) & 0xff);
                        *(dp + 5) = (png_byte)(background->blue & 0xff);
                     }
                     else
                     {
                        png_uint_32 v;

                        v = gamma_16_to_1[
                           *(sp + 1) >> gamma_shift][*sp];
                        v = (v * (png_uint_32)a +
                           (png_uint_32)background->red *
                           (png_uint_32)((png_uint_16)65535L - a) +
                           (png_uint_16)32767) / (png_uint_16)65535L;
                        v = gamma_16_from_1[(size_t)(
                           (v & 0xff) >> gamma_shift)][(size_t)(v >> 8)];
                        *dp = (png_byte)((v >> 8) & 0xff);
                        *(dp + 1) = (png_byte)(v & 0xff);
                        v = gamma_16_to_1[
                           *(sp + 3) >> gamma_shift][*(sp + 2)];
                        v = (v * (png_uint_32)a +
                           (png_uint_32)background->green *
                           (png_uint_32)((png_uint_16)65535L - a) +
                           (png_uint_16)32767) / (png_uint_16)65535L;
                        v = gamma_16_from_1[(size_t)(
                           (v & 0xff) >> gamma_shift)][(size_t)(v >> 8)];
                        *(dp + 2) = (png_byte)((v >> 8) & 0xff);
                        *(dp + 3) = (png_byte)(v & 0xff);
                        v = gamma_16_to_1[
                           *(sp + 5) >> gamma_shift][*(sp + 4)];
                        v = (v * (png_uint_32)a +
                           (png_uint_32)background->blue *
                           (png_uint_32)((png_uint_16)65535L - a) +
                           (png_uint_16)32767) / (png_uint_16)65535L;
                        v = gamma_16_from_1[(size_t)(
                           (v & 0xff) >> gamma_shift)][(size_t)(v >> 8)];
                        *(dp + 4) = (png_byte)((v >> 8) & 0xff);
                        *(dp + 5) = (png_byte)(v & 0xff);
                     }
                  }
               }
               else
#endif
               {
                  for (i = 0, sp = row,
                     dp = row;
                     i < row_info->width; i++, sp += 8, dp += 6)
                  {
                     png_uint_16 a;

                     a = (png_uint_16)(((png_uint_16)(*(sp + 6)) << 8) +
                        (png_uint_16)(*(sp + 7)));
                     if (a == (png_uint_16)0xffff)
                     {
                        png_memcpy(dp, sp, 6);
                     }
                     else if (a == 0)
                     {
                        *dp = (png_byte)((background->red >> 8) & 0xff);
                        *(dp + 1) = (png_byte)(background->red & 0xff);
                        *(dp + 2) = (png_byte)((background->green >> 8) & 0xff);
                        *(dp + 3) = (png_byte)(background->green & 0xff);
                        *(dp + 4) = (png_byte)((background->blue >> 8) & 0xff);
                        *(dp + 5) = (png_byte)(background->blue & 0xff);
                     }
                     else
                     {
                        png_uint_32 r, g, b, v;

                        r = ((png_uint_32)(*sp) << 8) +
                           (png_uint_32)(*(sp + 1));
                        g = ((png_uint_32)(*(sp + 2)) << 8) +
                           (png_uint_32)(*(sp + 3));
                        b = ((png_uint_32)(*(sp + 4)) << 8) +
                           (png_uint_32)(*(sp + 5));
                        v = (r * (png_uint_32)a +
                           (png_uint_32)background->red *
                           (png_uint_32)((png_uint_32)65535L - a) +
                           (png_uint_32)32767) / (png_uint_32)65535L;
                        *dp = (png_byte)((v >> 8) & 0xff);
                        *(dp + 1) = (png_byte)(v & 0xff);
                        v = (g * (png_uint_32)a +
                           (png_uint_32)background->green *
                           (png_uint_32)((png_uint_32)65535L - a) +
                           (png_uint_32)32767) / (png_uint_32)65535L;
                        *(dp + 2) = (png_byte)((v >> 8) & 0xff);
                        *(dp + 3) = (png_byte)(v & 0xff);
                        v = (b * (png_uint_32)a +
                           (png_uint_32)background->blue *
                           (png_uint_32)((png_uint_32)65535L - a) +
                           (png_uint_32)32767) / (png_uint_32)65535L;
                        *(dp + 4) = (png_byte)((v >> 8) & 0xff);
                        *(dp + 5) = (png_byte)(v & 0xff);
                     }
                  }
               }
            }
            break;
         }
      }
      if (row_info->color_type & PNG_COLOR_MASK_ALPHA)
      {
         row_info->color_type &= ~PNG_COLOR_MASK_ALPHA;
         row_info->channels -= (png_byte)1;
         row_info->pixel_depth = (png_byte)(row_info->channels *
            row_info->bit_depth);
         row_info->rowbytes = ((row_info->width *
            row_info->pixel_depth + 7) >> 3);
      }
   }
}
#endif

#if defined(PNG_READ_GAMMA_SUPPORTED)
/* gamma correct the image, avoiding the alpha channel.  Make sure
   you do this after you deal with the trasparency issue on grayscale
   or rgb images. If your bit depth is 8, use gamma_table, if it is 16,
   use gamma_16_table and gamma_shift.  Build these with
   build_gamma_table().  If your bit depth <= 8, gamma correct a
   palette, not the data.  */
void
png_do_gamma(png_row_infop row_info, png_bytep row,
   png_bytep gamma_table, png_uint_16pp gamma_16_table,
   int gamma_shift)
{
   png_bytep sp;
   png_uint_32 i;

   if (row && row_info && ((row_info->bit_depth <= 8 && gamma_table) ||
      (row_info->bit_depth == 16 && gamma_16_table)))
   {
      switch (row_info->color_type)
      {
         case PNG_COLOR_TYPE_RGB:
         {
            if (row_info->bit_depth == 8)
            {
               for (i = 0, sp = row;
                  i < row_info->width; i++)
               {
                  *sp = gamma_table[*sp];
                  sp++;
                  *sp = gamma_table[*sp];
                  sp++;
                  *sp = gamma_table[*sp];
                  sp++;
               }
            }
            else if (row_info->bit_depth == 16)
            {
               for (i = 0, sp = row;
                  i < row_info->width; i++)
               {
                  png_uint_16 v;

                  v = gamma_16_table[*(sp + 1) >>
                     gamma_shift][*sp];
                  *sp = (png_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (png_byte)(v & 0xff);
                  sp += 2;
                  v = gamma_16_table[*(sp + 1) >>
                     gamma_shift][*sp];
                  *sp = (png_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (png_byte)(v & 0xff);
                  sp += 2;
                  v = gamma_16_table[*(sp + 1) >>
                     gamma_shift][*sp];
                  *sp = (png_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (png_byte)(v & 0xff);
                  sp += 2;
               }
            }
            break;
         }
         case PNG_COLOR_TYPE_RGB_ALPHA:
         {
            if (row_info->bit_depth == 8)
            {
               for (i = 0, sp = row;
                  i < row_info->width; i++)
               {
                  *sp = gamma_table[*sp];
                  sp++;
                  *sp = gamma_table[*sp];
                  sp++;
                  *sp = gamma_table[*sp];
                  sp++;
                  sp++;
               }
            }
            else if (row_info->bit_depth == 16)
            {
               for (i = 0, sp = row;
                  i < row_info->width; i++)
               {
                  png_uint_16 v;

                  v = gamma_16_table[*(sp + 1) >>
                     gamma_shift][*sp];
                  *sp = (png_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (png_byte)(v & 0xff);
                  sp += 2;
                  v = gamma_16_table[*(sp + 1) >>
                     gamma_shift][*sp];
                  *sp = (png_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (png_byte)(v & 0xff);
                  sp += 2;
                  v = gamma_16_table[*(sp + 1) >>
                     gamma_shift][*sp];
                  *sp = (png_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (png_byte)(v & 0xff);
                  sp += 4;
               }
            }
            break;
         }
         case PNG_COLOR_TYPE_GRAY_ALPHA:
         {
            if (row_info->bit_depth == 8)
            {
               for (i = 0, sp = row;
                  i < row_info->width; i++)
               {
                  *sp = gamma_table[*sp];
                  sp++;
                  sp++;
               }
            }
            else if (row_info->bit_depth == 16)
            {
               for (i = 0, sp = row;
                  i < row_info->width; i++)
               {
                  png_uint_16 v;

                  v = gamma_16_table[*(sp + 1) >>
                     gamma_shift][*sp];
                  *sp = (png_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (png_byte)(v & 0xff);
                  sp += 4;
               }
            }
            break;
         }
         case PNG_COLOR_TYPE_GRAY:
         {
            if (row_info->bit_depth == 8)
            {
               for (i = 0, sp = row;
                  i < row_info->width; i++)
               {
                  *sp = gamma_table[*sp];
                  sp++;
               }
            }
            else if (row_info->bit_depth == 16)
            {
               for (i = 0, sp = row;
                  i < row_info->width; i++)
               {
                  png_uint_16 v;

                  v = gamma_16_table[*(sp + 1) >>
                     gamma_shift][*sp];
                  *sp = (png_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (png_byte)(v & 0xff);
                  sp += 2;
               }
            }
            break;
         }
      }
   }
}
#endif

#if defined(PNG_READ_EXPAND_SUPPORTED)
/* expands a palette row to an rgb or rgba row depending
   upon whether you supply trans and num_trans */
void
png_do_expand_palette(png_row_infop row_info, png_bytep row,
   png_colorp palette,
   png_bytep trans, int num_trans)
{
   int shift, value;
   png_bytep sp, dp;
   png_uint_32 i;

   if (row && row_info && row_info->color_type == PNG_COLOR_TYPE_PALETTE)
   {
      if (row_info->bit_depth < 8)
      {
         switch (row_info->bit_depth)
         {
            case 1:
            {
               sp = row + (png_size_t)((row_info->width - 1) >> 3);
               dp = row + (png_size_t)row_info->width - 1;
               shift = 7 - (int)((row_info->width + 7) & 7);
               for (i = 0; i < row_info->width; i++)
               {
                  if ((*sp >> shift) & 0x1)
                     *dp = 1;
                  else
                     *dp = 0;
                  if (shift == 7)
                  {
                     shift = 0;
                     sp--;
                  }
                  else
                     shift++;

                  dp--;
               }
               break;
            }
            case 2:
            {
               sp = row + (png_size_t)((row_info->width - 1) >> 2);
               dp = row + (png_size_t)row_info->width - 1;
               shift = (int)((3 - ((row_info->width + 3) & 3)) << 1);
               for (i = 0; i < row_info->width; i++)
               {
                  value = (*sp >> shift) & 0x3;
                  *dp = (png_byte)value;
                  if (shift == 6)
                  {
                     shift = 0;
                     sp--;
                  }
                  else
                     shift += 2;

                  dp--;
               }
               break;
            }
            case 4:
            {
               sp = row + (png_size_t)((row_info->width - 1) >> 1);
               dp = row + (png_size_t)row_info->width - 1;
               shift = (int)((row_info->width & 1) << 2);
               for (i = 0; i < row_info->width; i++)
               {
                  value = (*sp >> shift) & 0xf;
                  *dp = (png_byte)value;
                  if (shift == 4)
                  {
                     shift = 0;
                     sp--;
                  }
                  else
                     shift += 4;

                  dp--;
               }
               break;
            }
         }
         row_info->bit_depth = 8;
         row_info->pixel_depth = 8;
         row_info->rowbytes = row_info->width;
      }
      switch (row_info->bit_depth)
      {
         case 8:
         {
            if (trans)
            {
               sp = row + (png_size_t)row_info->width - 1;
               dp = row + (png_size_t)(row_info->width << 2) - 1;

               for (i = 0; i < row_info->width; i++)
               {
                  if (*sp >= (png_byte)num_trans)
                     *dp-- = 0xff;
                  else
                     *dp-- = trans[*sp];
                  *dp-- = palette[*sp].blue;
                  *dp-- = palette[*sp].green;
                  *dp-- = palette[*sp].red;
                  sp--;
               }
               row_info->bit_depth = 8;
               row_info->pixel_depth = 32;
               row_info->rowbytes = row_info->width * 4;
               row_info->color_type = 6;
               row_info->channels = 4;
            }
            else
            {
               sp = row + (png_size_t)row_info->width - 1;
               dp = row + (png_size_t)(row_info->width * 3) - 1;

               for (i = 0; i < row_info->width; i++)
               {
                  *dp-- = palette[*sp].blue;
                  *dp-- = palette[*sp].green;
                  *dp-- = palette[*sp].red;
                  sp--;
               }
               row_info->bit_depth = 8;
               row_info->pixel_depth = 24;
               row_info->rowbytes = row_info->width * 3;
               row_info->color_type = 2;
               row_info->channels = 3;
            }
            break;
         }
      }
   }
}

/* if the bit depth < 8, it is expanded to 8.  Also, if the
   transparency value is supplied, an alpha channel is built. */
void
png_do_expand(png_row_infop row_info, png_bytep row,
   png_color_16p trans_value)
{
   int shift, value;
   png_bytep sp, dp;
   png_uint_32 i;

   if (row && row_info)
   {
      if (row_info->color_type == PNG_COLOR_TYPE_GRAY &&
         row_info->bit_depth < 8)
      {
         switch (row_info->bit_depth)
         {
            case 1:
            {
               sp = row + (png_size_t)((row_info->width - 1) >> 3);
               dp = row + (png_size_t)row_info->width - 1;
               shift = 7 - (int)((row_info->width + 7) & 7);
               for (i = 0; i < row_info->width; i++)
               {
                  if ((*sp >> shift) & 0x1)
                     *dp = 0xff;
                  else
                     *dp = 0;
                  if (shift == 7)
                  {
                     shift = 0;
                     sp--;
                  }
                  else
                     shift++;

                  dp--;
               }
               break;
            }
            case 2:
            {
               sp = row + (png_size_t)((row_info->width - 1) >> 2);
               dp = row + (png_size_t)row_info->width - 1;
               shift = (int)((3 - ((row_info->width + 3) & 3)) << 1);
               for (i = 0; i < row_info->width; i++)
               {
                  value = (*sp >> shift) & 0x3;
                  *dp = (png_byte)(value | (value << 2) | (value << 4) |
                     (value << 6));
                  if (shift == 6)
                  {
                     shift = 0;
                     sp--;
                  }
                  else
                     shift += 2;

                  dp--;
               }
               break;
            }
            case 4:
            {
               sp = row + (png_size_t)((row_info->width - 1) >> 1);
               dp = row + (png_size_t)row_info->width - 1;
               shift = (int)((1 - ((row_info->width + 1) & 1)) << 2);
               for (i = 0; i < row_info->width; i++)
               {
                  value = (*sp >> shift) & 0xf;
                  *dp = (png_byte)(value | (value << 4));
                  if (shift == 4)
                  {
                     shift = 0;
                     sp--;
                  }
                  else
                     shift = 4;

                  dp--;
               }
               break;
            }
         }
         row_info->bit_depth = 8;
         row_info->pixel_depth = 8;
         row_info->rowbytes = row_info->width;
      }
      if (row_info->color_type == PNG_COLOR_TYPE_GRAY && trans_value)
      {
         if (row_info->bit_depth == 8)
         {
            sp = row + (png_size_t)row_info->width - 1;
            dp = row + (png_size_t)(row_info->width << 1) - 1;
            for (i = 0; i < row_info->width; i++)
            {
               if (*sp == trans_value->gray)
                  *dp-- = 0;
               else
                  *dp-- = 0xff;
               *dp-- = *sp--;
            }
         }
         else if (row_info->bit_depth == 16)
         {
            sp = row + (png_size_t)row_info->rowbytes - 1;
            dp = row + (png_size_t)(row_info->rowbytes << 1) - 1;
            for (i = 0; i < row_info->width; i++)
            {
               if (((png_uint_16)*(sp) |
                  ((png_uint_16)*(sp - 1) << 8)) == trans_value->gray)
               {
                  *dp-- = 0;
                  *dp-- = 0;
               }
               else
               {
                  *dp-- = 0xff;
                  *dp-- = 0xff;
               }
               *dp-- = *sp--;
               *dp-- = *sp--;
            }
         }
         row_info->color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
         row_info->channels = 2;
         row_info->pixel_depth = (png_byte)(row_info->bit_depth << 1);
         row_info->rowbytes =
            ((row_info->width * row_info->pixel_depth) >> 3);
      }
      else if (row_info->color_type == PNG_COLOR_TYPE_RGB && trans_value)
      {
         if (row_info->bit_depth == 8)
         {
            sp = row + (png_size_t)row_info->rowbytes - 1;
            dp = row + (png_size_t)(row_info->width << 2) - 1;
            for (i = 0; i < row_info->width; i++)
            {
               if (*(sp - 2) == trans_value->red &&
                  *(sp - 1) == trans_value->green &&
                  *(sp - 0) == trans_value->blue)
                  *dp-- = 0;
               else
                  *dp-- = 0xff;
               *dp-- = *sp--;
               *dp-- = *sp--;
               *dp-- = *sp--;
            }
         }
         else if (row_info->bit_depth == 16)
         {
            sp = row + (png_size_t)row_info->rowbytes - 1;
            dp = row + (png_size_t)(row_info->width << 3) - 1;
            for (i = 0; i < row_info->width; i++)
            {
               if ((((png_uint_16)*(sp - 4) |
                  ((png_uint_16)*(sp - 5) << 8)) == trans_value->red) &&
                  (((png_uint_16)*(sp - 2) |
                  ((png_uint_16)*(sp - 3) << 8)) == trans_value->green) &&
                  (((png_uint_16)*(sp - 0) |
                  ((png_uint_16)*(sp - 1) << 8)) == trans_value->blue))
               {
                  *dp-- = 0;
                  *dp-- = 0;
               }
               else
               {
                  *dp-- = 0xff;
                  *dp-- = 0xff;
               }
               *dp-- = *sp--;
               *dp-- = *sp--;
               *dp-- = *sp--;
               *dp-- = *sp--;
               *dp-- = *sp--;
               *dp-- = *sp--;
            }
         }
         row_info->color_type = PNG_COLOR_TYPE_RGB_ALPHA;
         row_info->channels = 4;
         row_info->pixel_depth = (png_byte)(row_info->bit_depth << 2);
         row_info->rowbytes =
            ((row_info->width * row_info->pixel_depth) >> 3);
      }
   }
}
#endif

#if defined(PNG_READ_DITHER_SUPPORTED)
void
png_do_dither(png_row_infop row_info, png_bytep row,
    png_bytep palette_lookup, png_bytep dither_lookup)
{
   png_bytep sp, dp;
   png_uint_32 i;

   if (row && row_info)
   {
      if (row_info->color_type == PNG_COLOR_TYPE_RGB &&
         palette_lookup && row_info->bit_depth == 8)
      {
         int r, g, b, p;
         sp = row;
         dp = row;
         for (i = 0; i < row_info->width; i++)
         {
            r = *sp++;
            g = *sp++;
            b = *sp++;

            /* this looks real messy, but the compiler will reduce
               it down to a reasonable formula.  For example, with
               5 bits per color, we get:
               p = (((r >> 3) & 0x1f) << 10) |
                  (((g >> 3) & 0x1f) << 5) |
                  ((b >> 3) & 0x1f);
               */
            p = (((r >> (8 - PNG_DITHER_RED_BITS)) &
               ((1 << PNG_DITHER_RED_BITS) - 1)) <<
               (PNG_DITHER_GREEN_BITS + PNG_DITHER_BLUE_BITS)) |
               (((g >> (8 - PNG_DITHER_GREEN_BITS)) &
               ((1 << PNG_DITHER_GREEN_BITS) - 1)) <<
               (PNG_DITHER_BLUE_BITS)) |
               ((b >> (8 - PNG_DITHER_BLUE_BITS)) &
               ((1 << PNG_DITHER_BLUE_BITS) - 1));

            *dp++ = palette_lookup[p];
         }
         row_info->color_type = PNG_COLOR_TYPE_PALETTE;
         row_info->channels = 1;
         row_info->pixel_depth = row_info->bit_depth;
         row_info->rowbytes =
            ((row_info->width * row_info->pixel_depth + 7) >> 3);
      }
      else if (row_info->color_type == PNG_COLOR_TYPE_RGB_ALPHA &&
         palette_lookup && row_info->bit_depth == 8)
      {
         int r, g, b, p;
         sp = row;
         dp = row;
         for (i = 0; i < row_info->width; i++)
         {
            r = *sp++;
            g = *sp++;
            b = *sp++;
            sp++;

            p = (((r >> (8 - PNG_DITHER_RED_BITS)) &
               ((1 << PNG_DITHER_RED_BITS) - 1)) <<
               (PNG_DITHER_GREEN_BITS + PNG_DITHER_BLUE_BITS)) |
               (((g >> (8 - PNG_DITHER_GREEN_BITS)) &
               ((1 << PNG_DITHER_GREEN_BITS) - 1)) <<
               (PNG_DITHER_BLUE_BITS)) |
               ((b >> (8 - PNG_DITHER_BLUE_BITS)) &
               ((1 << PNG_DITHER_BLUE_BITS) - 1));

            *dp++ = palette_lookup[p];
         }
         row_info->color_type = PNG_COLOR_TYPE_PALETTE;
         row_info->channels = 1;
         row_info->pixel_depth = row_info->bit_depth;
         row_info->rowbytes =
            ((row_info->width * row_info->pixel_depth + 7) >> 3);
      }
      else if (row_info->color_type == PNG_COLOR_TYPE_PALETTE &&
         dither_lookup && row_info->bit_depth == 8)
      {
         sp = row;
         for (i = 0; i < row_info->width; i++, sp++)
         {
            *sp = dither_lookup[*sp];
         }
      }
   }
}
#endif

#if defined(PNG_READ_GAMMA_SUPPORTED)
static int png_gamma_shift[] =
   {0x10, 0x21, 0x42, 0x84, 0x110, 0x248, 0x550, 0xff0};

void
png_build_gamma_table(png_structp png_ptr)
{
   if (png_ptr->bit_depth <= 8)
   {
      int i;
      double g;

      g = 1.0 / (png_ptr->gamma * png_ptr->display_gamma);

      png_ptr->gamma_table = (png_bytep)png_large_malloc(png_ptr,
         (png_uint_32)256);

      for (i = 0; i < 256; i++)
      {
         png_ptr->gamma_table[i] = (png_byte)(pow((double)i / 255.0,
            g) * 255.0 + .5);
      }

      if (png_ptr->transformations & PNG_BACKGROUND)
      {
         g = 1.0 / (png_ptr->gamma);

         png_ptr->gamma_to_1 = (png_bytep)png_large_malloc(png_ptr,
            (png_uint_32)256);

         for (i = 0; i < 256; i++)
         {
            png_ptr->gamma_to_1[i] = (png_byte)(pow((double)i / 255.0,
               g) * 255.0 + .5);
         }

         g = 1.0 / (png_ptr->display_gamma);

         png_ptr->gamma_from_1 = (png_bytep)png_large_malloc(png_ptr,
            (png_uint_32)256);

         for (i = 0; i < 256; i++)
         {
            png_ptr->gamma_from_1[i] = (png_byte)(pow((double)i / 255.0,
               g) * 255.0 + .5);
         }
      }
   }
   else
   {
      double g;
      int i, j, shift, num;
      int sig_bit;
      png_uint_32 ig;

      if (png_ptr->color_type & PNG_COLOR_MASK_COLOR)
      {
         sig_bit = (int)png_ptr->sig_bit.red;
         if ((int)png_ptr->sig_bit.green > sig_bit)
            sig_bit = png_ptr->sig_bit.green;
         if ((int)png_ptr->sig_bit.blue > sig_bit)
            sig_bit = png_ptr->sig_bit.blue;
      }
      else
      {
         sig_bit = (int)png_ptr->sig_bit.gray;
      }

      if (sig_bit > 0)
         shift = 16 - sig_bit;
      else
         shift = 0;

      if (png_ptr->transformations & PNG_16_TO_8)
      {
         if (shift < (16 - PNG_MAX_GAMMA_8))
            shift = (16 - PNG_MAX_GAMMA_8);
      }

      if (shift > 8)
         shift = 8;
      if (shift < 0)
         shift = 0;

      png_ptr->gamma_shift = (png_byte)shift;

      num = (1 << (8 - shift));

      g = 1.0 / (png_ptr->gamma * png_ptr->display_gamma);

      png_ptr->gamma_16_table = (png_uint_16pp)png_large_malloc(png_ptr,
         num * sizeof (png_uint_16p ));

      if ((png_ptr->transformations & PNG_16_TO_8) &&
         !(png_ptr->transformations & PNG_BACKGROUND))
      {
         double fin, fout;
         png_uint_32 last, max;

         for (i = 0; i < num; i++)
         {
            png_ptr->gamma_16_table[i] = (png_uint_16p)png_large_malloc(png_ptr,
               256 * sizeof (png_uint_16));
         }

         g = 1.0 / g;
         last = 0;
         for (i = 0; i < 256; i++)
         {
            fout = ((double)i + 0.5) / 256.0;
            fin = pow(fout, g);
            max = (png_uint_32)(fin * (double)((png_uint_32)num << 8));
            while (last <= max)
            {
               png_ptr->gamma_16_table[(int)(last & (0xff >> shift))]
                  [(int)(last >> (8 - shift))] = (png_uint_16)(
                  (png_uint_16)i | ((png_uint_16)i << 8));
               last++;
            }
         }
         while (last < ((png_uint_32)num << 8))
         {
            png_ptr->gamma_16_table[(int)(last & (0xff >> shift))]
               [(int)(last >> (8 - shift))] =
               (png_uint_16)65535L;
            last++;
         }
      }
      else
      {
         for (i = 0; i < num; i++)
         {
            png_ptr->gamma_16_table[i] = (png_uint_16p)png_large_malloc(png_ptr,
               256 * sizeof (png_uint_16));

            ig = (((png_uint_32)i *
               (png_uint_32)png_gamma_shift[shift]) >> 4);
            for (j = 0; j < 256; j++)
            {
               png_ptr->gamma_16_table[i][j] =
                  (png_uint_16)(pow((double)(ig + ((png_uint_32)j << 8)) /
                     65535.0, g) * 65535.0 + .5);
            }
         }
      }

      if (png_ptr->transformations & PNG_BACKGROUND)
      {
         g = 1.0 / (png_ptr->gamma);

         png_ptr->gamma_16_to_1 = (png_uint_16pp)png_large_malloc(png_ptr,
            num * sizeof (png_uint_16p ));

         for (i = 0; i < num; i++)
         {
            png_ptr->gamma_16_to_1[i] = (png_uint_16p)png_large_malloc(png_ptr,
               256 * sizeof (png_uint_16));

            ig = (((png_uint_32)i *
               (png_uint_32)png_gamma_shift[shift]) >> 4);
            for (j = 0; j < 256; j++)
            {
               png_ptr->gamma_16_to_1[i][j] =
                  (png_uint_16)(pow((double)(ig + ((png_uint_32)j << 8)) /
                     65535.0, g) * 65535.0 + .5);
            }
         }
         g = 1.0 / (png_ptr->display_gamma);

         png_ptr->gamma_16_from_1 = (png_uint_16pp)png_large_malloc(png_ptr,
            num * sizeof (png_uint_16p));

         for (i = 0; i < num; i++)
         {
            png_ptr->gamma_16_from_1[i] = (png_uint_16p)png_large_malloc(png_ptr,
               256 * sizeof (png_uint_16));

            ig = (((png_uint_32)i *
               (png_uint_32)png_gamma_shift[shift]) >> 4);
            for (j = 0; j < 256; j++)
            {
               png_ptr->gamma_16_from_1[i][j] =
                  (png_uint_16)(pow((double)(ig + ((png_uint_32)j << 8)) /
                     65535.0, g) * 65535.0 + .5);
            }
         }
      }
   }
}
#endif

