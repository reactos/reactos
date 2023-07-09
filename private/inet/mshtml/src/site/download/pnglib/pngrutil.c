
/* pngrutil.c - utilities to read a png file

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

/* grab an uint 32 from a buffer */
png_uint_32
png_get_uint_32(png_bytep buf)
{
   png_uint_32 i;

   i = ((png_uint_32)(*buf) << 24) +
      ((png_uint_32)(*(buf + 1)) << 16) +
      ((png_uint_32)(*(buf + 2)) << 8) +
      (png_uint_32)(*(buf + 3));

   return i;
}

/* grab an uint 16 from a buffer */
png_uint_16
png_get_uint_16(png_bytep buf)
{
   png_uint_16 i;

   i = (png_uint_16)(((png_uint_16)(*buf) << 8) +
      (png_uint_16)(*(buf + 1)));

   return i;
}

/* read data, and run it through the crc */
void
png_crc_read(png_structp png_ptr, png_bytep buf, png_uint_32 length)
{
   png_read_data(png_ptr, buf, length);
   png_calculate_crc(png_ptr, buf, length);
}

/* skip data, but calcuate the crc anyway */
void
png_crc_skip(png_structp png_ptr, png_uint_32 length)
{
   png_uint_32 i;

   for (i = length; i > png_ptr->zbuf_size; i -= png_ptr->zbuf_size)
   {
      png_read_data(png_ptr, png_ptr->zbuf, png_ptr->zbuf_size);
      png_calculate_crc(png_ptr, png_ptr->zbuf, png_ptr->zbuf_size);
   }
   if (i)
   {
      png_read_data(png_ptr, png_ptr->zbuf, i);
      png_calculate_crc(png_ptr, png_ptr->zbuf, i);
   }
}

/* read and check the IDHR chunk */
void
png_handle_IHDR(png_structp png_ptr, png_infop info, png_uint_32 length)
{
   png_byte buf[13];
   png_uint_32 width, height;
   int bit_depth, color_type, compression_type, filter_type;
   int interlace_type;

   /* check the length */
   if (length != 13)
      png_error(png_ptr, "Invalid IHDR chunk");

   png_crc_read(png_ptr, buf, 13);

   width = png_get_uint_32(buf);
   height = png_get_uint_32(buf + 4);
   bit_depth = buf[8];
   color_type = buf[9];
   compression_type = buf[10];
   filter_type = buf[11];
   interlace_type = buf[12];

   /* check for width and height valid values */
   if (width == 0 || height == 0)
      png_error(png_ptr, "Invalid Width or Height Found");

   /* check other values */
   if (bit_depth != 1 && bit_depth != 2 &&
      bit_depth != 4 && bit_depth != 8 &&
      bit_depth != 16)
      png_error(png_ptr, "Invalid Bit Depth Found");

   if (color_type < 0 || color_type == 1 ||
      color_type == 5 || color_type > 6)
      png_error(png_ptr, "Invalid Color Type Found");

   if (color_type == PNG_COLOR_TYPE_PALETTE &&
      bit_depth == 16)
      png_error(png_ptr, "Found Invalid Color Type and Bit Depth Combination");

   if ((color_type == PNG_COLOR_TYPE_RGB ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA ||
      color_type == PNG_COLOR_TYPE_RGB_ALPHA) &&
      bit_depth < 8)
      png_error(png_ptr, "Found Invalid Color Type and Bit Depth Combination");

   if (interlace_type > 1)
      png_error(png_ptr, "Found Invalid Interlace Value");

   if (compression_type > 0)
      png_error(png_ptr, "Found Invalid Compression Value");

   if (filter_type > 0)
      png_error(png_ptr, "Found Invalid Filter Value");

   /* set internal variables */
   png_ptr->width = width;
   png_ptr->height = height;
   png_ptr->bit_depth = (png_byte)bit_depth;
   png_ptr->interlaced = (png_byte)interlace_type;
   png_ptr->color_type = (png_byte)color_type;

   /* find number of channels */
   switch (png_ptr->color_type)
   {
      case 0:
      case 3:
         png_ptr->channels = 1;
         break;
      case 2:
         png_ptr->channels = 3;
         break;
      case 4:
         png_ptr->channels = 2;
         break;
      case 6:
         png_ptr->channels = 4;
         break;
   }
   /* set up other useful info */
   png_ptr->pixel_depth = (png_byte)(png_ptr->bit_depth *
      png_ptr->channels);
   png_ptr->rowbytes = ((png_ptr->width *
      (png_uint_32)png_ptr->pixel_depth + 7) >> 3);
   /* call the IHDR callback (which should just set up info) */
   png_read_IHDR(png_ptr, info, width, height, bit_depth,
      color_type, compression_type, filter_type, interlace_type);
}

/* read and check the palette */
void
png_handle_PLTE(png_structp png_ptr, png_infop info, png_uint_32 length)
{
   int num, i;
   png_colorp palette;

   if (length % 3)
      png_error(png_ptr, "Invalid Palette Chunk");

   num = (int)length / 3;
   palette = (png_colorp)png_large_malloc(png_ptr, num * sizeof (png_color));
   png_ptr->do_free |= PNG_FREE_PALETTE;
   for (i = 0; i < num; i++)
   {
      png_byte buf[3];

      png_crc_read(png_ptr, buf, 3);
      /* don't depend upon png_color being any order */
      palette[i].red = buf[0];
      palette[i].green = buf[1];
      palette[i].blue = buf[2];
   }
   png_ptr->palette = palette;
   png_ptr->num_palette = (png_uint_16)num;
   png_read_PLTE(png_ptr, info, palette, num);
}

#if defined(PNG_READ_gAMA_SUPPORTED)
void
png_handle_gAMA(png_structp png_ptr, png_infop info, png_uint_32 length)
{
   png_uint_32 igamma;
   float gamma;
   png_byte buf[4];

   if (length != 4)
   {
      png_warning(png_ptr, "Incorrect gAMA chunk length");
      png_crc_skip(png_ptr, length);
      return;
   }

   png_crc_read(png_ptr, buf, 4);
   igamma = png_get_uint_32(buf);
   /* check for zero gamma */
   if (!igamma)
      return;

   gamma = (float)igamma / (float)100000.0;
   png_read_gAMA(png_ptr, info, gamma);
   png_ptr->gamma = gamma;
}
#endif

#if defined(PNG_READ_sBIT_SUPPORTED)
void
png_handle_sBIT(png_structp png_ptr, png_infop info, png_uint_32 length)
{
   int slen;
   png_byte buf[4];

   buf[0] = buf[1] = buf[2] = buf[3] = 0;

   if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      slen = 3;
   else
      slen = png_ptr->channels;

   if (length != (png_uint_32)slen)
   {
      png_warning(png_ptr, "Incorrect sBIT chunk length");
      png_crc_skip(png_ptr, length);
      return;
   }

   png_crc_read(png_ptr, buf, length);
   if (png_ptr->color_type & PNG_COLOR_MASK_COLOR)
   {
      png_ptr->sig_bit.red = buf[0];
      png_ptr->sig_bit.green = buf[1];
      png_ptr->sig_bit.blue = buf[2];
      png_ptr->sig_bit.alpha = buf[3];
   }
   else
   {
      png_ptr->sig_bit.gray = buf[0];
      png_ptr->sig_bit.alpha = buf[1];
   }
   png_read_sBIT(png_ptr, info, &(png_ptr->sig_bit));
}
#endif

#if defined(PNG_READ_cHRM_SUPPORTED)
void
png_handle_cHRM(png_structp png_ptr, png_infop info, png_uint_32 length)
{
   png_byte buf[4];
   png_uint_32 v;
   float white_x, white_y, red_x, red_y, green_x, green_y, blue_x, blue_y;

   if (length != 32)
   {
      png_warning(png_ptr, "Incorrect cHRM chunk length");
      png_crc_skip(png_ptr, length);
      return;
   }

   png_crc_read(png_ptr, buf, 4);
   v = png_get_uint_32(buf);
   white_x = (float)v / (float)100000.0;

   png_crc_read(png_ptr, buf, 4);
   v = png_get_uint_32(buf);
   white_y = (float)v / (float)100000.0;

   png_crc_read(png_ptr, buf, 4);
   v = png_get_uint_32(buf);
   red_x = (float)v / (float)100000.0;

   png_crc_read(png_ptr, buf, 4);
   v = png_get_uint_32(buf);
   red_y = (float)v / (float)100000.0;

   png_crc_read(png_ptr, buf, 4);
   v = png_get_uint_32(buf);
   green_x = (float)v / (float)100000.0;

   png_crc_read(png_ptr, buf, 4);
   v = png_get_uint_32(buf);
   green_y = (float)v / (float)100000.0;

   png_crc_read(png_ptr, buf, 4);
   v = png_get_uint_32(buf);
   blue_x = (float)v / (float)100000.0;

   png_crc_read(png_ptr, buf, 4);
   v = png_get_uint_32(buf);
   blue_y = (float)v / (float)100000.0;

   png_read_cHRM(png_ptr, info,
      white_x, white_y, red_x, red_y, green_x, green_y, blue_x, blue_y);
}
#endif

#if defined(PNG_READ_tRNS_SUPPORTED)
void
png_handle_tRNS(png_structp png_ptr, png_infop info, png_uint_32 length)
{
   if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
   {
      if (length > png_ptr->num_palette)
      {
         png_warning(png_ptr, "Incorrect tRNS chunk length");
         png_crc_skip(png_ptr, length);
         return;
      }

      png_ptr->trans = (png_bytep)png_large_malloc(png_ptr, length);
      png_ptr->do_free |= PNG_FREE_TRANS;
      png_crc_read(png_ptr, png_ptr->trans, length);
      png_ptr->num_trans = (png_uint_16)length;
   }
   else if (png_ptr->color_type == PNG_COLOR_TYPE_RGB)
   {
      png_byte buf[6];

      if (length != 6)
      {
         png_warning(png_ptr, "Incorrect tRNS chunk length");
         png_crc_skip(png_ptr, length);
         return;
      }

      png_crc_read(png_ptr, buf, length);
      png_ptr->num_trans = 3;
      png_ptr->trans_values.red = png_get_uint_16(buf);
      png_ptr->trans_values.green = png_get_uint_16(buf + 2);
      png_ptr->trans_values.blue = png_get_uint_16(buf + 4);
   }
   else if (png_ptr->color_type == PNG_COLOR_TYPE_GRAY)
   {
      png_byte buf[6];

      if (length != 2)
      {
         png_warning(png_ptr, "Incorrect tRNS chunk length");
         png_crc_skip(png_ptr, length);
         return;
      }

      png_crc_read(png_ptr, buf, 2);
      png_ptr->num_trans = 1;
      png_ptr->trans_values.gray = png_get_uint_16(buf);
   }
   else
      png_warning(png_ptr, "Invalid tRNS chunk");

   png_read_tRNS(png_ptr, info, png_ptr->trans, png_ptr->num_trans,
      &(png_ptr->trans_values));
}
#endif

#if defined(PNG_READ_bKGD_SUPPORTED)
void
png_handle_bKGD(png_structp png_ptr, png_infop info, png_uint_32 length)
{
   int truelen;
   png_byte buf[6];

   if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      truelen = 1;
   else if (png_ptr->color_type & PNG_COLOR_MASK_COLOR)
      truelen = 6;
   else
      truelen = 2;

   if (length != (png_uint_32)truelen)
   {
      png_warning(png_ptr, "Incorrect bKGD chunk length");
      png_crc_skip(png_ptr, length);
      return;
   }

   png_crc_read(png_ptr, buf, length);
   if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      png_ptr->background.index = buf[0];
   else if (!(png_ptr->color_type & PNG_COLOR_MASK_COLOR))
      png_ptr->background.gray = png_get_uint_16(buf);
   else
   {
      png_ptr->background.red = png_get_uint_16(buf);
      png_ptr->background.green = png_get_uint_16(buf + 2);
      png_ptr->background.blue = png_get_uint_16(buf + 4);
   }

   png_read_bKGD(png_ptr, info, &(png_ptr->background));
}
#endif

#if defined(PNG_READ_hIST_SUPPORTED)
void
png_handle_hIST(png_structp png_ptr, png_infop info, png_uint_32 length)
{
   int num, i;

   if (length != 2 * png_ptr->num_palette)
   {
      png_warning(png_ptr, "Incorrect hIST chunk length");
      png_crc_skip(png_ptr, length);
      return;
   }

   num = (int)length / 2;
   png_ptr->hist = (png_uint_16p)png_large_malloc(png_ptr,
      num * sizeof (png_uint_16));
   png_ptr->do_free |= PNG_FREE_HIST;
   for (i = 0; i < num; i++)
   {
      png_byte buf[2];

      png_crc_read(png_ptr, buf, 2);
      png_ptr->hist[i] = png_get_uint_16(buf);
   }
   png_read_hIST(png_ptr, info, png_ptr->hist);
}
#endif

#if defined(PNG_READ_pHYs_SUPPORTED)
void
png_handle_pHYs(png_structp png_ptr, png_infop info, png_uint_32 length)
{
   png_byte buf[9];
   png_uint_32 res_x, res_y;
   int unit_type;

   if (length != 9)
   {
      png_warning(png_ptr, "Incorrect pHYs chunk length");
      png_crc_skip(png_ptr, length);
      return;
   }

   png_crc_read(png_ptr, buf, 9);

   res_x = png_get_uint_32(buf);
   res_y = png_get_uint_32(buf + 4);
   unit_type = buf[8];
   png_read_pHYs(png_ptr, info, res_x, res_y, unit_type);
}
#endif

#if defined(PNG_READ_oFFs_SUPPORTED)
void
png_handle_oFFs(png_structp png_ptr, png_infop info, png_uint_32 length)
{
   png_byte buf[9];
   png_uint_32 offset_x, offset_y;
   int unit_type;

   if (length != 9)
   {
      png_warning(png_ptr, "Incorrect oFFs chunk length");
      png_crc_skip(png_ptr, length);
      return;
   }

   png_crc_read(png_ptr, buf, 9);

   offset_x = png_get_uint_32(buf);
   offset_y = png_get_uint_32(buf + 4);
   unit_type = buf[8];
   png_read_oFFs(png_ptr, info, offset_x, offset_y, unit_type);
}
#endif

#if defined(PNG_READ_tIME_SUPPORTED)
void
png_handle_tIME(png_structp png_ptr, png_infop info, png_uint_32 length)
{
   png_byte buf[7];
   png_time mod_time;

   if (length != 7)
   {
      png_warning(png_ptr, "Incorrect tIME chunk length");
      png_crc_skip(png_ptr, length);
      return;
   }

   png_crc_read(png_ptr, buf, 7);

   mod_time.second = buf[6];
   mod_time.minute = buf[5];
   mod_time.hour = buf[4];
   mod_time.day = buf[3];
   mod_time.month = buf[2];
   mod_time.year = png_get_uint_16(buf);

   png_read_tIME(png_ptr, info, &mod_time);
}
#endif

#if defined(PNG_READ_tEXt_SUPPORTED)
/* note: this does not correctly handle chunks that are > 64K */
void
png_handle_tEXt(png_structp png_ptr, png_infop info, png_uint_32 length)
{
   png_charp key;
   png_charp text;

   key = (png_charp )png_large_malloc(png_ptr, length + 1);
   png_crc_read(png_ptr, (png_bytep )key, length);
   key[(png_size_t)length] = '\0';

   for (text = key; *text; text++)
      /* empty loop */ ;

   if (text != key + (png_size_t)length)
      text++;

   png_read_tEXt(png_ptr, info, key, text, length - (text - key));
}
#endif

#if defined(PNG_READ_zTXt_SUPPORTED)
/* note: this does not correctly handle chunks that are > 64K compressed */
void
png_handle_zTXt(png_structp png_ptr, png_infop info, png_uint_32 length)
{
   png_charp key;
   png_charp text;
   int ret;
   png_uint_32 text_size, key_size;

   key = png_large_malloc(png_ptr, length + 1);
   png_crc_read(png_ptr, (png_bytep )key, length);
   key[(png_size_t)length] = '\0';

   for (text = key; *text; text++)
      /* empty loop */ ;

   /* zTXt can't have zero text */
   if (text == key + (png_size_t)length)
   {
      png_warning(png_ptr, "Zero length zTXt chunk");
      png_large_free(png_ptr, key);
      return;
   }

   text++;

   if (*text) /* check compression byte */
   {
      png_large_free(png_ptr, key);
      return;
   }

   text++;

   png_ptr->zstream->next_in = (png_bytep )text;
   png_ptr->zstream->avail_in = (uInt)(length - (text - key));
   png_ptr->zstream->next_out = png_ptr->zbuf;
   png_ptr->zstream->avail_out = (png_size_t)png_ptr->zbuf_size;

   key_size = text - key;
   text_size = 0;
   text = NULL;
   ret = Z_STREAM_END;

   while (png_ptr->zstream->avail_in)
   {
      ret = inflate(png_ptr->zstream, Z_PARTIAL_FLUSH);
      if (ret != Z_OK && ret != Z_STREAM_END)
      {
         if (png_ptr->zstream->msg)
            png_warning(png_ptr, png_ptr->zstream->msg);
         else
            png_warning(png_ptr, "zTXt decompression error");
         inflateReset(png_ptr->zstream);
         png_ptr->zstream->avail_in = 0;
         png_large_free(png_ptr, key);
         png_large_free(png_ptr, text);
         return;
      }
      if (!png_ptr->zstream->avail_out || ret == Z_STREAM_END)
      {
         if (!text)
         {
            text = (png_charp)png_large_malloc(png_ptr,
               png_ptr->zbuf_size - png_ptr->zstream->avail_out +
                  key_size + 1);
            png_memcpy(text + (png_size_t)key_size, png_ptr->zbuf,
               (png_size_t)(png_ptr->zbuf_size - png_ptr->zstream->avail_out));
            png_memcpy(text, key, (png_size_t)key_size);
            text_size = key_size + (png_size_t)png_ptr->zbuf_size -
               png_ptr->zstream->avail_out;
            *(text + (png_size_t)text_size) = '\0';
         }
         else
         {
            png_charp tmp;

            tmp = text;
            text = png_large_malloc(png_ptr, text_size +
               png_ptr->zbuf_size - png_ptr->zstream->avail_out + 1);
            png_memcpy(text, tmp, (png_size_t)text_size);
            png_large_free(png_ptr, tmp);
            png_memcpy(text + (png_size_t)text_size, png_ptr->zbuf,
               (png_size_t)(png_ptr->zbuf_size - png_ptr->zstream->avail_out));
            text_size += png_ptr->zbuf_size - png_ptr->zstream->avail_out;
            *(text + (png_size_t)text_size) = '\0';
         }
         if (ret != Z_STREAM_END)
         {
            png_ptr->zstream->next_out = png_ptr->zbuf;
            png_ptr->zstream->avail_out = (uInt)png_ptr->zbuf_size;
         }
      }
      else
      {
         break;
      }

      if (ret == Z_STREAM_END)
         break;
   }

   inflateReset(png_ptr->zstream);
   png_ptr->zstream->avail_in = 0;

   if (ret != Z_STREAM_END)
   {
      png_large_free(png_ptr, key);
      png_large_free(png_ptr, text);
      return;
   }

   png_large_free(png_ptr, key);
   key = text;
   text += (png_size_t)key_size;
   text_size -= key_size;

   png_read_zTXt(png_ptr, info, key, text, text_size, 0);
}
#endif

/* Combines the row recently read in with the previous row.
   This routine takes care of alpha and transparency if requested.
   This routine also handles the two methods of progressive display
   of interlaced images, depending on the mask value.
   The mask value describes which pixels are to be combined with
   the row.  The pattern always repeats every 8 pixels, so just 8
   bits are needed.  A one indicates the pixels is to be combined,
   a zero indicates the pixel is to be skipped.  This is in addition
   to any alpha or transparency value associated with the pixel.  If
   you want all pixels to be combined, pass 0xff (255) in mask.
*/
void
png_combine_row(png_structp png_ptr, png_bytep row,
   int mask)
{
   if (mask == 0xff)
   {
      png_memcpy(row, png_ptr->row_buf + 1,
         (png_size_t)((png_ptr->width *
         png_ptr->row_info.pixel_depth + 7) >> 3));
   }
   else
   {
      switch (png_ptr->row_info.pixel_depth)
      {
         case 1:
         {
            png_bytep sp;
            png_bytep dp;
            int m;
            int shift;
            png_uint_32 i;
            int value;

            sp = png_ptr->row_buf + 1;
            dp = row;
            shift = 7;
            m = 0x80;
            for (i = 0; i < png_ptr->width; i++)
            {
               if (m & mask)
               {
                  value = (*sp >> shift) & 0x1;
                  *dp &= (png_byte)((0x7f7f >> (7 - shift)) & 0xff);
                  *dp |= (png_byte)(value << shift);
               }

               if (shift == 0)
               {
                  shift = 7;
                  sp++;
                  dp++;
               }
               else
                  shift--;

               if (m == 1)
                  m = 0x80;
               else
                  m >>= 1;
            }
            break;
         }
         case 2:
         {
            png_bytep sp;
            png_bytep dp;
            int m;
            int shift;
            png_uint_32 i;
            int value;

            sp = png_ptr->row_buf + 1;
            dp = row;
            shift = 6;
            m = 0x80;
            for (i = 0; i < png_ptr->width; i++)
            {
               if (m & mask)
               {
                  value = (*sp >> shift) & 0x3;
                  *dp &= (png_byte)((0x3f3f >> (6 - shift)) & 0xff);
                  *dp |= (png_byte)(value << shift);
               }

               if (shift == 0)
               {
                  shift = 6;
                  sp++;
                  dp++;
               }
               else
                  shift -= 2;
               if (m == 1)
                  m = 0x80;
               else
                  m >>= 1;
            }
            break;
         }
         case 4:
         {
            png_bytep sp;
            png_bytep dp;
            int m;
            int shift;
            png_uint_32 i;
            int value;

            sp = png_ptr->row_buf + 1;
            dp = row;
            shift = 4;
            m = 0x80;
            for (i = 0; i < png_ptr->width; i++)
            {
               if (m & mask)
               {
                  value = (*sp >> shift) & 0xf;
                  *dp &= (png_byte)((0xf0f >> (4 - shift)) & 0xff);
                  *dp |= (png_byte)(value << shift);
               }

               if (shift == 0)
               {
                  shift = 4;
                  sp++;
                  dp++;
               }
               else
                  shift -= 4;
               if (m == 1)
                  m = 0x80;
               else
                  m >>= 1;
            }
            break;
         }
         default:
         {
            png_bytep sp;
            png_bytep dp;
            png_uint_32 i;
            int pixel_bytes, m;

            pixel_bytes = (png_ptr->row_info.pixel_depth >> 3);

            sp = png_ptr->row_buf + 1;
            dp = row;
            m = 0x80;
            for (i = 0; i < png_ptr->width; i++)
            {
               if (m & mask)
               {
                  png_memcpy(dp, sp, pixel_bytes);
               }

               sp += pixel_bytes;
               dp += pixel_bytes;

               if (m == 1)
                  m = 0x80;
               else
                  m >>= 1;
            }
            break;
         }
      }
   }
}

#if defined(PNG_READ_INTERLACING_SUPPORTED)
void
png_do_read_interlace(png_row_infop row_info, png_bytep row, int pass)
{
   if (row && row_info)
   {
      png_uint_32 final_width;

      final_width = row_info->width * png_pass_inc[pass];

      switch (row_info->pixel_depth)
      {
         case 1:
         {
            png_bytep sp, dp;
            int sshift, dshift;
            png_byte v;
            png_uint_32 i;
            int j;

            sp = row + (png_size_t)((row_info->width - 1) >> 3);
            sshift = 7 - (int)((row_info->width + 7) & 7);
            dp = row + (png_size_t)((final_width - 1) >> 3);
            dshift = 7 - (int)((final_width + 7) & 7);
            for (i = row_info->width; i; i--)
            {
               v = (png_byte)((*sp >> sshift) & 0x1);
               for (j = 0; j < png_pass_inc[pass]; j++)
               {
                  *dp &= (png_byte)((0x7f7f >> (7 - dshift)) & 0xff);
                  *dp |= (png_byte)(v << dshift);
                  if (dshift == 7)
                  {
                     dshift = 0;
                     dp--;
                  }
                  else
                     dshift++;
               }
               if (sshift == 7)
               {
                  sshift = 0;
                  sp--;
               }
               else
                  sshift++;
            }
            break;
         }
         case 2:
         {
            png_bytep sp, dp;
            int sshift, dshift;
            png_byte v;
            png_uint_32 i, j;

            sp = row + (png_size_t)((row_info->width - 1) >> 2);
            sshift = (png_size_t)((3 - ((row_info->width + 3) & 3)) << 1);
            dp = row + (png_size_t)((final_width - 1) >> 2);
            dshift = (png_size_t)((3 - ((final_width + 3) & 3)) << 1);
            for (i = row_info->width; i; i--)
            {
               v = (png_byte)((*sp >> sshift) & 0x3);
               for (j = 0; j < png_pass_inc[pass]; j++)
               {
                  *dp &= (png_byte)((0x3f3f >> (6 - dshift)) & 0xff);
                  *dp |= (png_byte)(v << dshift);
                  if (dshift == 6)
                  {
                     dshift = 0;
                     dp--;
                  }
                  else
                     dshift += 2;
               }
               if (sshift == 6)
               {
                  sshift = 0;
                  sp--;
               }
               else
                  sshift += 2;
            }
            break;
         }
         case 4:
         {
            png_bytep sp, dp;
            int sshift, dshift;
            png_byte v;
            png_uint_32 i;
            int j;

            sp = row + (png_size_t)((row_info->width - 1) >> 1);
            sshift = (png_size_t)((1 - ((row_info->width + 1) & 1)) << 2);
            dp = row + (png_size_t)((final_width - 1) >> 1);
            dshift = (png_size_t)((1 - ((final_width + 1) & 1)) << 2);
            for (i = row_info->width; i; i--)
            {
               v = (png_byte)((*sp >> sshift) & 0xf);
               for (j = 0; j < png_pass_inc[pass]; j++)
               {
                  *dp &= (png_byte)((0xf0f >> (4 - dshift)) & 0xff);
                  *dp |= (png_byte)(v << dshift);
                  if (dshift == 4)
                  {
                     dshift = 0;
                     dp--;
                  }
                  else
                     dshift = 4;
               }
               if (sshift == 4)
               {
                  sshift = 0;
                  sp--;
               }
               else
                  sshift = 4;
            }
            break;
         }
         default:
         {
            png_bytep sp, dp;
            png_byte v[8];
            png_uint_32 i;
            int j;
            int pixel_bytes;

            pixel_bytes = (row_info->pixel_depth >> 3);

            sp = row + (png_size_t)((row_info->width - 1) * pixel_bytes);
            dp = row + (png_size_t)((final_width - 1) * pixel_bytes);
            for (i = row_info->width; i; i--)
            {
               png_memcpy(v, sp, pixel_bytes);
               for (j = 0; j < png_pass_inc[pass]; j++)
               {
                  png_memcpy(dp, v, pixel_bytes);
                  dp -= pixel_bytes;
               }
               sp -= pixel_bytes;
            }
            break;
         }
      }
      row_info->width = final_width;
      row_info->rowbytes = ((final_width *
         (png_uint_32)row_info->pixel_depth + 7) >> 3);
   }
}
#endif

void
png_read_filter_row(png_row_infop row_info, png_bytep row,
   png_bytep prev_row, int filter)
{
   switch (filter)
   {
      case 0:
         break;
      case 1:
      {
         png_uint_32 i;
         int bpp;
         png_bytep rp;
         png_bytep lp;

         bpp = (row_info->pixel_depth + 7) / 8;
         for (i = (png_uint_32)bpp, rp = row + bpp, lp = row;
            i < row_info->rowbytes; i++, rp++, lp++)
         {
            *rp = (png_byte)(((int)(*rp) + (int)(*lp)) & 0xff);
         }
         break;
      }
      case 2:
      {
         png_uint_32 i;
         png_bytep rp;
         png_bytep pp;

         for (i = 0, rp = row, pp = prev_row;
            i < row_info->rowbytes; i++, rp++, pp++)
         {
            *rp = (png_byte)(((int)(*rp) + (int)(*pp)) & 0xff);
         }
         break;
      }
      case 3:
      {
         png_uint_32 i;
         int bpp;
         png_bytep rp;
         png_bytep pp;
         png_bytep lp;

         bpp = (row_info->pixel_depth + 7) / 8;
         for (i = 0, rp = row, pp = prev_row;
            i < (png_uint_32)bpp; i++, rp++, pp++)
         {
            *rp = (png_byte)(((int)(*rp) +
               ((int)(*pp) / 2)) & 0xff);
         }
         for (lp = row; i < row_info->rowbytes; i++, rp++, lp++, pp++)
         {
            *rp = (png_byte)(((int)(*rp) +
               (int)(*pp + *lp) / 2) & 0xff);
         }
         break;
      }
      case 4:
      {
         int bpp;
         png_uint_32 i;
         png_bytep rp;
         png_bytep pp;
         png_bytep lp;
         png_bytep cp;

         bpp = (row_info->pixel_depth + 7) / 8;
         for (i = 0, rp = row, pp = prev_row,
            lp = row - bpp, cp = prev_row - bpp;
            i < row_info->rowbytes; i++, rp++, pp++, lp++, cp++)
         {
            int a, b, c, pa, pb, pc, p;

            b = *pp;
            if (i >= (png_uint_32)bpp)
            {
               c = *cp;
               a = *lp;
            }
            else
            {
               a = c = 0;
            }
            p = a + b - c;
            pa = abs(p - a);
            pb = abs(p - b);
            pc = abs(p - c);

            if (pa <= pb && pa <= pc)
               p = a;
            else if (pb <= pc)
               p = b;
            else
               p = c;

            *rp = (png_byte)(((int)(*rp) + p) & 0xff);
         }
         break;
      }
      default:
         break;
   }
}

void
png_read_finish_row(png_structp png_ptr)
{
   png_ptr->row_number++;
   if (png_ptr->row_number < png_ptr->num_rows)
      return;

   if (png_ptr->interlaced)
   {
      png_ptr->row_number = 0;
      png_memset(png_ptr->prev_row, 0, (png_size_t)png_ptr->rowbytes + 1);
      do
      {
         png_ptr->pass++;
         if (png_ptr->pass >= 7)
            break;
         png_ptr->iwidth = (png_ptr->width +
            png_pass_inc[png_ptr->pass] - 1 -
            png_pass_start[png_ptr->pass]) /
            png_pass_inc[png_ptr->pass];
         png_ptr->irowbytes = ((png_ptr->iwidth *
            png_ptr->pixel_depth + 7) >> 3) + 1;
         if (!(png_ptr->transformations & PNG_INTERLACE))
         {
            png_ptr->num_rows = (png_ptr->height +
               png_pass_yinc[png_ptr->pass] - 1 -
               png_pass_ystart[png_ptr->pass]) /
               png_pass_yinc[png_ptr->pass];
            if (!(png_ptr->num_rows))
               continue;
         }
         if (png_ptr->transformations & PNG_INTERLACE)
            break;
      } while (png_ptr->iwidth == 0);

      if (png_ptr->pass < 7)
         return;
   }

   if (!png_ptr->zlib_finished)
   {
      char extra;
      int ret;

      png_ptr->zstream->next_out = (Byte *)&extra;
      png_ptr->zstream->avail_out = (uInt)1;
      do
      {
         if (!(png_ptr->zstream->avail_in))
         {
            while (!png_ptr->idat_size)
            {
               png_byte buf[4];
               png_uint_32 crc;

               png_read_data(png_ptr, buf, 4);
               crc = png_get_uint_32(buf);
               if (((crc ^ 0xffffffffL) & 0xffffffffL) !=
                  (png_ptr->crc & 0xffffffffL))
                  png_error(png_ptr, "Bad CRC value");

               png_read_data(png_ptr, buf, 4);
               png_ptr->idat_size = png_get_uint_32(buf);
               png_reset_crc(png_ptr);

               png_crc_read(png_ptr, buf, 4);
               if (png_memcmp(buf, png_IDAT, 4))
                  png_error(png_ptr, "Not enough image data");

            }
            png_ptr->zstream->avail_in = (uInt)png_ptr->zbuf_size;
            png_ptr->zstream->next_in = png_ptr->zbuf;
            if (png_ptr->zbuf_size > png_ptr->idat_size)
               png_ptr->zstream->avail_in = (uInt)png_ptr->idat_size;
            png_crc_read(png_ptr, png_ptr->zbuf, png_ptr->zstream->avail_in);
            png_ptr->idat_size -= png_ptr->zstream->avail_in;
         }
         ret = inflate(png_ptr->zstream, Z_PARTIAL_FLUSH);
         if (ret == Z_STREAM_END)
         {
            if (!(png_ptr->zstream->avail_out) || png_ptr->zstream->avail_in ||
               png_ptr->idat_size)
               png_error(png_ptr, "Extra compressed data");
            png_ptr->mode = PNG_AT_LAST_IDAT;
            break;
         }
         if (ret != Z_OK)
            png_error(png_ptr, "Compression Error");

         if (!(png_ptr->zstream->avail_out))
            png_error(png_ptr, "Extra compressed data");

      } while (1);
      png_ptr->zstream->avail_out = 0;
   }

   if (png_ptr->idat_size || png_ptr->zstream->avail_in)
      png_error(png_ptr, "Extra compression data");

   inflateReset(png_ptr->zstream);

   png_ptr->mode = PNG_AT_LAST_IDAT;
}

void
png_read_start_row(png_structp png_ptr)
{
   int max_pixel_depth;
   png_uint_32 rowbytes;

   png_ptr->zstream->avail_in = 0;
   png_init_read_transformations(png_ptr);
   if (png_ptr->interlaced)
   {
      if (!(png_ptr->transformations & PNG_INTERLACE))
         png_ptr->num_rows = (png_ptr->height + png_pass_yinc[0] - 1 -
            png_pass_ystart[0]) / png_pass_yinc[0];
      else
         png_ptr->num_rows = png_ptr->height;

      png_ptr->iwidth = (png_ptr->width +
         png_pass_inc[png_ptr->pass] - 1 -
         png_pass_start[png_ptr->pass]) /
         png_pass_inc[png_ptr->pass];
      png_ptr->irowbytes = ((png_ptr->iwidth *
         png_ptr->pixel_depth + 7) >> 3) + 1;
   }
   else
   {
      png_ptr->num_rows = png_ptr->height;
      png_ptr->iwidth = png_ptr->width;
      png_ptr->irowbytes = png_ptr->rowbytes + 1;
   }

   max_pixel_depth = png_ptr->pixel_depth;

#if defined(PNG_READ_PACK_SUPPORTED)
   if ((png_ptr->transformations & PNG_PACK) && png_ptr->bit_depth < 8)
   {
      max_pixel_depth = 8;
   }
#endif

#if defined(PNG_READ_EXPAND_SUPPORTED) || defined(PNG_READ_PACK_SUPPORTED)
   if (png_ptr->transformations & (PNG_EXPAND | PNG_PACK))
   {
      if (png_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      {
         if (png_ptr->num_trans)
            max_pixel_depth = 32;
         else
            max_pixel_depth = 24;
      }
      else if (png_ptr->color_type == PNG_COLOR_TYPE_GRAY)
      {
         if (max_pixel_depth < 8)
            max_pixel_depth = 8;
         if (png_ptr->num_trans)
            max_pixel_depth *= 2;
      }
      else if (png_ptr->color_type == PNG_COLOR_TYPE_RGB)
      {
         if (png_ptr->num_trans)
         {
            max_pixel_depth *= 4;
            max_pixel_depth /= 3;
         }
      }
   }
#endif

#if defined(PNG_READ_FILLER_SUPPORTED)
   if (png_ptr->transformations & (PNG_FILLER))
   {
      if (max_pixel_depth < 32)
         max_pixel_depth = 32;
   }
#endif

#if defined(PNG_READ_GRAY_TO_RGB_SUPPORTED)
   if (png_ptr->transformations & PNG_GRAY_TO_RGB)
   {
      if ((png_ptr->num_trans && (png_ptr->transformations & PNG_EXPAND)) ||
         png_ptr->color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
      {
         if (max_pixel_depth <= 16)
            max_pixel_depth = 32;
         else if (max_pixel_depth <= 32)
            max_pixel_depth = 64;
      }
      else
      {
         if (max_pixel_depth <= 8)
            max_pixel_depth = 24;
         else if (max_pixel_depth <= 16)
            max_pixel_depth = 48;
      }
   }
#endif

   /* align the width on the next larger 8 pixels.  Mainly used
      for interlacing */
   rowbytes = ((png_ptr->width + 7) & ~((png_uint_32)7));
   /* calculate the maximum bytes needed, adding a byte and a pixel
      for safety sake */
   rowbytes = ((rowbytes * (png_uint_32)max_pixel_depth + 7) >> 3) +
      1 + ((max_pixel_depth + 7) >> 3);
#ifdef PNG_MAX_MALLOC_64K
   if (rowbytes > 65536L)
      png_error(png_ptr, "This image requires a row greater then 64KB");
#endif
   png_ptr->row_buf = (png_bytep )png_large_malloc(png_ptr, rowbytes);

#ifdef PNG_MAX_MALLOC_64K
   if (png_ptr->rowbytes + 1 > 65536L)
      png_error(png_ptr, "This image requires a row greater then 64KB");
#endif
   png_ptr->prev_row = png_large_malloc(png_ptr,
      png_ptr->rowbytes + 1);

   png_memset(png_ptr->prev_row, 0, (png_size_t)png_ptr->rowbytes + 1);

   png_ptr->row_init = 1;
}

