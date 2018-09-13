
/* pngpread.c - read a png file in push mode

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

#ifdef PNG_PROGRESSIVE_READ_SUPPORTED

void
png_process_data(png_structp png_ptr, png_infop info,
   png_bytep buffer, png_uint_32 buffer_size)
{
   png_push_restore_buffer(png_ptr, buffer, buffer_size);

   while (png_ptr->buffer_size)
   {
      png_process_some_data(png_ptr, info);
   }
}

void
png_process_some_data(png_structp png_ptr, png_infop info)
{
   switch (png_ptr->process_mode)
   {
      case PNG_READ_SIG_MODE:
      {
         png_push_read_sig(png_ptr);
         break;
      }
      case PNG_READ_CHUNK_MODE:
      {
         png_push_read_chunk(png_ptr, info);
         break;
      }
      case PNG_READ_IDAT_MODE:
      {
         png_push_read_idat(png_ptr);
         break;
      }
      case PNG_READ_PLTE_MODE:
      {
         png_push_read_plte(png_ptr, info);
         break;
      }
#if defined(PNG_READ_tEXt_SUPPORTED)
      case PNG_READ_tEXt_MODE:
      {
         png_push_read_text(png_ptr, info);
         break;
      }
#endif
#if defined(PNG_READ_zTXt_SUPPORTED)
      case PNG_READ_zTXt_MODE:
      {
         png_push_read_ztxt(png_ptr, info);
         break;
      }
#endif
      case PNG_READ_END_MODE:
      {
         png_push_read_end(png_ptr, info);
         break;
      }
      case PNG_SKIP_MODE:
      {
         png_push_skip(png_ptr);
         break;
      }
      default:
      {
         png_ptr->buffer_size = 0;
         break;
      }
   }
}

void
png_push_read_sig(png_structp png_ptr)
{
   png_byte sig[8];

   if (png_ptr->buffer_size < 8)
   {
      png_push_save_buffer(png_ptr);
      return;
   }

   png_push_fill_buffer(png_ptr, sig, 8);

   if (png_check_sig(sig, 8))
   {
      png_ptr->process_mode = PNG_READ_CHUNK_MODE;
   }
   else
   {
      png_error(png_ptr, "Not a PNG file");
   }
}

void
png_push_read_chunk(png_structp png_ptr, png_infop info)
{
   if (!png_ptr->have_chunk_header)
   {
      png_byte chunk_start[8];

      if (png_ptr->buffer_size < 8)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      png_push_fill_buffer(png_ptr, chunk_start, 8);
      png_ptr->push_length = png_get_uint_32(chunk_start);
      png_memcpy(png_ptr->push_chunk_name, (png_voidp)(chunk_start + 4), 4);
      png_ptr->have_chunk_header = 1;
      png_reset_crc(png_ptr);
      png_calculate_crc(png_ptr, chunk_start + 4, 4);
   }

   if (!png_memcmp(png_ptr->push_chunk_name, png_IHDR, 4))
   {
      if (png_ptr->mode != PNG_BEFORE_IHDR)
         png_error(png_ptr, "Out of Place IHDR");

      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      png_handle_IHDR(png_ptr, info, png_ptr->push_length);
      png_push_check_crc(png_ptr);
      png_ptr->mode = PNG_HAVE_IHDR;
   }
   else if (!png_memcmp(png_ptr->push_chunk_name, png_PLTE, 4))
   {
      if (png_ptr->mode != PNG_HAVE_IHDR)
         png_error(png_ptr, "Missing IHDR");

#if !defined(PNG_READ_OPT_PLTE_SUPPORTED)
      if (png_ptr->color_type != PNG_COLOR_TYPE_PALETTE)
         png_push_crc_skip(png_ptr, length);
      else
#else
      {
         png_push_handle_PLTE(png_ptr, png_ptr->push_length);
      }
#endif
      png_ptr->mode = PNG_HAVE_PLTE;
   }
   else if (!png_memcmp(png_ptr->push_chunk_name, png_IDAT, 4))
   {
      png_ptr->idat_size = png_ptr->push_length;
      png_ptr->mode = PNG_HAVE_IDAT;
      png_ptr->process_mode = PNG_READ_IDAT_MODE;
      png_push_have_info(png_ptr, info);
      png_ptr->zstream->avail_out = (uInt)png_ptr->irowbytes;
      png_ptr->zstream->next_out = png_ptr->row_buf;
      return;
   }
   else if (!png_memcmp(png_ptr->push_chunk_name, png_IEND, 4))
   {
      png_error(png_ptr, "No Image in File");
   }
#if defined(PNG_READ_gAMA_SUPPORTED)
   else if (!png_memcmp(png_ptr->push_chunk_name, png_gAMA, 4))
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      if (png_ptr->mode != PNG_HAVE_IHDR)
         png_error(png_ptr, "Out of Place gAMA");

      png_handle_gAMA(png_ptr, info, png_ptr->push_length);
      png_push_check_crc(png_ptr);
   }
#endif
#if defined(PNG_READ_sBIT_SUPPORTED)
   else if (!png_memcmp(png_ptr->push_chunk_name, png_sBIT, 4))
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      if (png_ptr->mode != PNG_HAVE_IHDR)
         png_error(png_ptr, "Out of Place sBIT");

      png_handle_sBIT(png_ptr, info, png_ptr->push_length);
      png_push_check_crc(png_ptr);
   }
#endif
#if defined(PNG_READ_cHRM_SUPPORTED)
   else if (!png_memcmp(png_ptr->push_chunk_name, png_cHRM, 4))
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      if (png_ptr->mode != PNG_HAVE_IHDR)
         png_error(png_ptr, "Out of Place cHRM");

      png_handle_cHRM(png_ptr, info, png_ptr->push_length);
      png_push_check_crc(png_ptr);
   }
#endif
#if defined(PNG_READ_tRNS_SUPPORTED)
   else if (!png_memcmp(png_ptr->push_chunk_name, png_tRNS, 4))
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }
      if (png_ptr->mode != PNG_HAVE_IHDR &&
         png_ptr->mode != PNG_HAVE_PLTE)
         png_error(png_ptr, "Out of Place tRNS");

      png_handle_tRNS(png_ptr, info, png_ptr->push_length);
      png_push_check_crc(png_ptr);
   }
#endif
#if defined(PNG_READ_bKGD_SUPPORTED)
   else if (!png_memcmp(png_ptr->push_chunk_name, png_bKGD, 4))
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      if (png_ptr->mode != PNG_HAVE_IHDR &&
         png_ptr->mode != PNG_HAVE_PLTE)
         png_error(png_ptr, "Out of Place bKGD");

      png_handle_bKGD(png_ptr, info, png_ptr->push_length);
      png_push_check_crc(png_ptr);
   }
#endif
#if defined(PNG_READ_hIST_SUPPORTED)
   else if (!png_memcmp(png_ptr->push_chunk_name, png_hIST, 4))
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      if (png_ptr->mode != PNG_HAVE_PLTE)
         png_error(png_ptr, "Out of Place hIST");

      png_handle_hIST(png_ptr, info, png_ptr->push_length);
      png_push_check_crc(png_ptr);
   }
#endif
#if defined(PNG_READ_pHYs_SUPPORTED)
   else if (!png_memcmp(png_ptr->push_chunk_name, png_pHYs, 4))
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      if (png_ptr->mode != PNG_HAVE_IHDR &&
         png_ptr->mode != PNG_HAVE_PLTE)
         png_error(png_ptr, "Out of Place pHYs");

      png_handle_pHYs(png_ptr, info, png_ptr->push_length);
      png_push_check_crc(png_ptr);
   }
#endif
#if defined(PNG_READ_oFFs_SUPPORTED)
   else if (!png_memcmp(png_ptr->push_chunk_name, png_oFFs, 4))
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      if (png_ptr->mode != PNG_HAVE_IHDR &&
         png_ptr->mode != PNG_HAVE_PLTE)
         png_error(png_ptr, "Out of Place oFFs");

      png_handle_oFFs(png_ptr, info, png_ptr->push_length);
      png_push_check_crc(png_ptr);
   }
#endif
#if defined(PNG_READ_tIME_SUPPORTED)
   else if (!png_memcmp(png_ptr->push_chunk_name, png_tIME, 4))
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      if (png_ptr->mode == PNG_BEFORE_IHDR ||
         png_ptr->mode == PNG_AFTER_IEND)
         png_error(png_ptr, "Out of Place tIME");

      png_handle_tIME(png_ptr, info, png_ptr->push_length);
      png_push_check_crc(png_ptr);
   }
#endif
#if defined(PNG_READ_tEXt_SUPPORTED)
   else if (!png_memcmp(png_ptr->push_chunk_name, png_tEXt, 4))
   {
      if (png_ptr->mode == PNG_BEFORE_IHDR ||
         png_ptr->mode == PNG_AFTER_IEND)
         png_error(png_ptr, "Out of Place tEXt");

      png_push_handle_tEXt(png_ptr, png_ptr->push_length);
   }
#endif
#if defined(PNG_READ_zTXt_SUPPORTED)
   else if (!png_memcmp(png_ptr->push_chunk_name, png_zTXt, 4))
   {
      if (png_ptr->mode == PNG_BEFORE_IHDR ||
         png_ptr->mode == PNG_AFTER_IEND)
         png_error(png_ptr, "Out of Place zTXt");

      png_push_handle_zTXt(png_ptr, png_ptr->push_length);
   }
#endif
   else
   {
      if ((png_ptr->push_chunk_name[0] & 0x20) == 0)
         png_error(png_ptr, "Unknown Critical Chunk");

      png_push_crc_skip(png_ptr, png_ptr->push_length);
   }
   png_ptr->have_chunk_header = 0;
}

void
png_push_check_crc(png_structp png_ptr)
{
   png_byte crc_buf[4];
   png_uint_32 crc;

   png_push_fill_buffer(png_ptr, crc_buf, 4);
   crc = png_get_uint_32(crc_buf);
   if (((crc ^ 0xffffffffL) & 0xffffffffL) !=
      (png_ptr->crc & 0xffffffffL))
      png_error(png_ptr, "Bad CRC value");
}

void
png_push_crc_skip(png_structp png_ptr, png_uint_32 length)
{
   png_ptr->process_mode = PNG_SKIP_MODE;
   png_ptr->skip_length = length;
}

void
png_push_skip(png_structp png_ptr)
{
   if (png_ptr->skip_length && png_ptr->save_buffer_size)
   {
      png_uint_32 save_size;

      if (png_ptr->skip_length < png_ptr->save_buffer_size)
         save_size = png_ptr->skip_length;
      else
         save_size = png_ptr->save_buffer_size;

      png_calculate_crc(png_ptr, png_ptr->save_buffer_ptr, save_size);

      png_ptr->skip_length -= save_size;
      png_ptr->buffer_size -= save_size;
      png_ptr->save_buffer_size -= save_size;
      png_ptr->save_buffer_ptr += (png_size_t)save_size;
   }
   if (png_ptr->skip_length && png_ptr->current_buffer_size)
   {
      png_uint_32 save_size;

      if (png_ptr->skip_length < png_ptr->current_buffer_size)
         save_size = png_ptr->skip_length;
      else
         save_size = png_ptr->current_buffer_size;

      png_calculate_crc(png_ptr, png_ptr->current_buffer_ptr, save_size);

      png_ptr->skip_length -= save_size;
      png_ptr->buffer_size -= save_size;
      png_ptr->current_buffer_size -= save_size;
      png_ptr->current_buffer_ptr += (png_size_t)save_size;
   }
   if (!png_ptr->skip_length)
   {
      if (png_ptr->buffer_size < 4)
      {
         png_push_save_buffer(png_ptr);
         return;
      }
      png_push_check_crc(png_ptr);
      png_ptr->process_mode = PNG_READ_CHUNK_MODE;
   }
}

void
png_push_fill_buffer(png_structp png_ptr, png_bytep buffer,
   png_uint_32 length)
{
   png_bytep ptr;

   ptr = buffer;
   if (png_ptr->save_buffer_size)
   {
      png_uint_32 save_size;

      if (length < png_ptr->save_buffer_size)
         save_size = length;
      else
         save_size = png_ptr->save_buffer_size;

      png_memcpy(ptr, png_ptr->save_buffer_ptr, (png_size_t)save_size);
      length -= save_size;
      ptr += (png_size_t)save_size;
      png_ptr->buffer_size -= save_size;
      png_ptr->save_buffer_size -= save_size;
      png_ptr->save_buffer_ptr += (png_size_t)save_size;
   }
   if (length && png_ptr->current_buffer_size)
   {
      png_uint_32 save_size;

      if (length < png_ptr->current_buffer_size)
         save_size = length;
      else
         save_size = png_ptr->current_buffer_size;

      png_memcpy(ptr, png_ptr->current_buffer_ptr, (png_size_t)save_size);
      png_ptr->buffer_size -= save_size;
      png_ptr->current_buffer_size -= save_size;
      png_ptr->current_buffer_ptr += (png_size_t)save_size;
   }
}

void
png_push_save_buffer(png_structp png_ptr)
{
   if (png_ptr->save_buffer_size)
   {
      if (png_ptr->save_buffer_ptr != png_ptr->save_buffer)
      {
         int i;
         png_bytep sp;
         png_bytep dp;

         for (i = 0, sp = png_ptr->save_buffer_ptr, dp = png_ptr->save_buffer;
            i < png_ptr->save_buffer_size;
            i++, sp++, dp++)
         {
            *dp = *sp;
         }
      }
   }
   if (png_ptr->save_buffer_size + png_ptr->current_buffer_size >
      png_ptr->save_buffer_max)
   {
      int new_max;
      png_bytep old_buffer;

      new_max = (int)(png_ptr->save_buffer_size +
         png_ptr->current_buffer_size + 256);
      old_buffer = png_ptr->save_buffer;
      png_ptr->save_buffer = (png_bytep)
         png_large_malloc(png_ptr, new_max);
      png_memcpy(png_ptr->save_buffer, old_buffer,
         (png_size_t)png_ptr->save_buffer_size);
      png_large_free(png_ptr, old_buffer);
        png_ptr->save_buffer_max = new_max;
   }
   if (png_ptr->current_buffer_size)
   {
      png_memcpy(png_ptr->save_buffer +
         (png_size_t)png_ptr->save_buffer_size,
         png_ptr->current_buffer_ptr,
         (png_size_t)png_ptr->current_buffer_size);
      png_ptr->save_buffer_size += png_ptr->current_buffer_size;
      png_ptr->current_buffer_size = 0;
   }
   png_ptr->save_buffer_ptr = png_ptr->save_buffer;
   png_ptr->buffer_size = 0;
}

void
png_push_restore_buffer(png_structp png_ptr, png_bytep buffer,
   png_uint_32 buffer_length)
{
   png_ptr->current_buffer = buffer;
   png_ptr->current_buffer_size = buffer_length;
   png_ptr->buffer_size = buffer_length + png_ptr->save_buffer_size;
   png_ptr->current_buffer_ptr = png_ptr->current_buffer;
}

void
png_push_read_idat(png_structp png_ptr)
{
   if (!png_ptr->have_chunk_header)
   {
      png_byte chunk_start[8];

      if (png_ptr->buffer_size < 8)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      png_push_fill_buffer(png_ptr, chunk_start, 8);
      png_ptr->push_length = png_get_uint_32(chunk_start);
      png_memcpy(png_ptr->push_chunk_name,
         (png_voidp)(chunk_start + 4), 4);
      png_ptr->have_chunk_header = 1;
      png_reset_crc(png_ptr);
      png_calculate_crc(png_ptr, chunk_start + 4, 4);
      if (png_memcmp(png_ptr->push_chunk_name, png_IDAT, 4))
      {
         png_ptr->process_mode = PNG_READ_END_MODE;
         if (!png_ptr->zlib_finished)
            png_error(png_ptr, "Not enough compressed data");
         return;
      }

      png_ptr->idat_size = png_ptr->push_length;
   }
   if (png_ptr->idat_size && png_ptr->save_buffer_size)
   {
      png_uint_32 save_size;

      if (png_ptr->idat_size < png_ptr->save_buffer_size)
         save_size = png_ptr->idat_size;
      else
         save_size = png_ptr->save_buffer_size;

      png_calculate_crc(png_ptr, png_ptr->save_buffer_ptr, save_size);
      png_process_IDAT_data(png_ptr, png_ptr->save_buffer_ptr, save_size);

      png_ptr->idat_size -= save_size;
      png_ptr->buffer_size -= save_size;
      png_ptr->save_buffer_size -= save_size;
      png_ptr->save_buffer_ptr += (png_size_t)save_size;
   }
   if (png_ptr->idat_size && png_ptr->current_buffer_size)
   {
      png_uint_32 save_size;

      if (png_ptr->idat_size < png_ptr->current_buffer_size)
         save_size = png_ptr->idat_size;
      else
         save_size = png_ptr->current_buffer_size;

      png_calculate_crc(png_ptr, png_ptr->current_buffer_ptr, save_size);
      png_process_IDAT_data(png_ptr, png_ptr->current_buffer_ptr, save_size);

      png_ptr->idat_size -= save_size;
      png_ptr->buffer_size -= save_size;
      png_ptr->current_buffer_size -= save_size;
      png_ptr->current_buffer_ptr += (png_size_t)save_size;
   }
   if (!png_ptr->idat_size)
   {
      if (png_ptr->buffer_size < 4)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      png_push_check_crc(png_ptr);
      png_ptr->have_chunk_header = 0;
   }
}

void
png_process_IDAT_data(png_structp png_ptr, png_bytep buffer,
   png_uint_32 buffer_length)
{
   int ret;

   if (png_ptr->zlib_finished && buffer_length)
      png_error(png_ptr, "Extra compression data");

   png_ptr->zstream->next_in = buffer;
   png_ptr->zstream->avail_in = (uInt)buffer_length;
   do
   {
      ret = inflate(png_ptr->zstream, Z_PARTIAL_FLUSH);
      if (ret == Z_STREAM_END)
      {
         if (png_ptr->zstream->avail_in)
            png_error(png_ptr, "Extra compressed data");
         if (!png_ptr->zstream->avail_out)
         {
            png_push_process_row(png_ptr);
         }
         png_ptr->mode = PNG_AT_LAST_IDAT;
         png_ptr->zlib_finished = 1;
         break;
      }
      if (ret != Z_OK)
         png_error(png_ptr, "Compression Error");
      if (!(png_ptr->zstream->avail_out))
      {
         png_push_process_row(png_ptr);
         png_ptr->zstream->avail_out = (uInt)png_ptr->irowbytes;
         png_ptr->zstream->next_out = png_ptr->row_buf;
      }

   } while (png_ptr->zstream->avail_in);
}

void
png_push_process_row(png_structp png_ptr)
{
   png_ptr->row_info.color_type = png_ptr->color_type;
   png_ptr->row_info.width = png_ptr->iwidth;
   png_ptr->row_info.channels = png_ptr->channels;
   png_ptr->row_info.bit_depth = png_ptr->bit_depth;
   png_ptr->row_info.pixel_depth = png_ptr->pixel_depth;
   png_ptr->row_info.rowbytes = ((png_ptr->row_info.width *
      (png_uint_32)png_ptr->row_info.pixel_depth + 7) >> 3);

   if (png_ptr->row_buf[0])
      png_read_filter_row(&(png_ptr->row_info),
         png_ptr->row_buf + 1, png_ptr->prev_row + 1,
         (int)(png_ptr->row_buf[0]));

   png_memcpy(png_ptr->prev_row, png_ptr->row_buf, (png_size_t)png_ptr->rowbytes + 1);

   if (png_ptr->transformations)
      png_do_read_transformations(png_ptr);

#if defined(PNG_READ_INTERLACING_SUPPORTED)
   /* blow up interlaced rows to full size */
   if (png_ptr->interlaced &&
      (png_ptr->transformations & PNG_INTERLACE))
   {
      if (png_ptr->pass < 6)
         png_do_read_interlace(&(png_ptr->row_info),
            png_ptr->row_buf + 1, png_ptr->pass);

      switch (png_ptr->pass)
      {
         case 0:
         {
            int i;
            for (i = 0; i < 8 && png_ptr->pass == 0; i++)
            {
               png_push_have_row(png_ptr, png_ptr->row_buf + 1);
               png_read_push_finish_row(png_ptr);
            }
            break;
         }
         case 1:
         {
            int i;
            for (i = 0; i < 8 && png_ptr->pass == 1; i++)
            {
               png_push_have_row(png_ptr, png_ptr->row_buf + 1);
               png_read_push_finish_row(png_ptr);
            }
            if (png_ptr->pass == 2)
            {
               for (i = 0; i < 4 && png_ptr->pass == 2; i++)
               {
                  png_push_have_row(png_ptr, NULL);
                  png_read_push_finish_row(png_ptr);
               }
            }
            break;
         }
         case 2:
         {
            int i;
            for (i = 0; i < 4 && png_ptr->pass == 2; i++)
            {
               png_push_have_row(png_ptr, png_ptr->row_buf + 1);
               png_read_push_finish_row(png_ptr);
            }
            for (i = 0; i < 4 && png_ptr->pass == 2; i++)
            {
               png_push_have_row(png_ptr, NULL);
               png_read_push_finish_row(png_ptr);
            }
            break;
         }
         case 3:
         {
            int i;
            for (i = 0; i < 4 && png_ptr->pass == 3; i++)
            {
               png_push_have_row(png_ptr, png_ptr->row_buf + 1);
               png_read_push_finish_row(png_ptr);
            }
            if (png_ptr->pass == 4)
            {
               for (i = 0; i < 2 && png_ptr->pass == 4; i++)
               {
                  png_push_have_row(png_ptr, NULL);
                  png_read_push_finish_row(png_ptr);
               }
            }
            break;
         }
         case 4:
         {
            int i;
            for (i = 0; i < 2 && png_ptr->pass == 4; i++)
            {
               png_push_have_row(png_ptr, png_ptr->row_buf + 1);
               png_read_push_finish_row(png_ptr);
            }
            for (i = 0; i < 2 && png_ptr->pass == 4; i++)
            {
               png_push_have_row(png_ptr, NULL);
               png_read_push_finish_row(png_ptr);
            }
            break;
         }
         case 5:
         {
            int i;
            for (i = 0; i < 2 && png_ptr->pass == 5; i++)
            {
               png_push_have_row(png_ptr, png_ptr->row_buf + 1);
               png_read_push_finish_row(png_ptr);
            }
            if (png_ptr->pass == 6)
            {
               png_push_have_row(png_ptr, NULL);
               png_read_push_finish_row(png_ptr);
            }
            break;
         }
         case 6:
         {
            png_push_have_row(png_ptr, png_ptr->row_buf + 1);
            png_read_push_finish_row(png_ptr);
            if (png_ptr->pass != 6)
               break;
            png_push_have_row(png_ptr, NULL);
            png_read_push_finish_row(png_ptr);
         }
      }
   }
   else
#endif
   {
      png_push_have_row(png_ptr, png_ptr->row_buf + 1);
      png_read_push_finish_row(png_ptr);
   }
}

void
png_read_push_finish_row(png_structp png_ptr)
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
   }
}


void
png_push_handle_PLTE(png_structp png_ptr, png_uint_32 length)
{
   if (length % 3)
      png_error(png_ptr, "Invalid Palette Chunk");

   png_ptr->num_palette = (png_uint_16)(length / 3);
   png_ptr->cur_palette = 0;
   png_ptr->palette = (png_colorp)png_large_malloc(png_ptr,
      png_ptr->num_palette * sizeof (png_color));
   png_ptr->process_mode = PNG_READ_PLTE_MODE;
}

void
png_push_read_plte(png_structp png_ptr, png_infop info)
{
   while (png_ptr->cur_palette < png_ptr->num_palette &&
      png_ptr->buffer_size >= 3)
   {
      png_byte buf[3];
      png_push_fill_buffer(png_ptr, buf, 3);
      png_calculate_crc(png_ptr, buf, 3);

      /* don't depend upon png_color being any order */
      png_ptr->palette[png_ptr->cur_palette].red = buf[0];
      png_ptr->palette[png_ptr->cur_palette].green = buf[1];
      png_ptr->palette[png_ptr->cur_palette].blue = buf[2];
      png_ptr->cur_palette++;
   }
   if (png_ptr->cur_palette == png_ptr->num_palette &&
      png_ptr->buffer_size >= 4)
   {
      png_push_check_crc(png_ptr);
      png_read_PLTE(png_ptr, info, png_ptr->palette, png_ptr->num_palette);
      png_ptr->process_mode = PNG_READ_CHUNK_MODE;
   }
   else
   {
      png_push_save_buffer(png_ptr);
   }
}

#if defined(PNG_READ_tEXt_SUPPORTED)
void
png_push_handle_tEXt(png_structp png_ptr, png_uint_32 length)
{
   png_ptr->current_text = (png_charp)png_large_malloc(png_ptr, length + 1);
   png_ptr->current_text[(png_size_t)length] = '\0';
   png_ptr->current_text_ptr = png_ptr->current_text;
   png_ptr->current_text_size = length;
   png_ptr->current_text_left = length;
   png_ptr->process_mode = PNG_READ_tEXt_MODE;
}

void
png_push_read_text(png_structp png_ptr, png_infop info)
{
   if (png_ptr->buffer_size && png_ptr->current_text_left)
   {
      png_uint_32 text_size;

      if (png_ptr->buffer_size < png_ptr->current_text_left)
         text_size = png_ptr->buffer_size;
      else
         text_size = png_ptr->current_text_left;
      png_push_fill_buffer(png_ptr, (png_bytep)png_ptr->current_text_ptr,
         text_size);
      png_calculate_crc(png_ptr, (png_bytep)png_ptr->current_text_ptr,
         text_size);
      png_ptr->current_text_left -= text_size;
      png_ptr->current_text_ptr += (png_size_t)text_size;
   }
   if (!(png_ptr->current_text_left))
   {
      png_charp text;
      png_charp key;

      if (png_ptr->buffer_size < 4)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      png_push_check_crc(png_ptr);

      key = png_ptr->current_text;
      png_ptr->current_text = 0;

      for (text = key; *text; text++)
         /* empty loop */ ;

      if (text != key + (png_size_t)png_ptr->current_text_size)
         text++;

      png_read_tEXt(png_ptr, info, key, text,
         png_ptr->current_text_size - (text - key));

      if (png_ptr->mode == PNG_AT_LAST_IDAT ||
         png_ptr->mode == PNG_AFTER_IDAT)
      {
         png_ptr->process_mode = PNG_READ_END_MODE;
      }
      else
      {
         png_ptr->process_mode = PNG_READ_CHUNK_MODE;
      }
   }
}
#endif

#if defined(PNG_READ_zTXt_SUPPORTED)
void
png_push_handle_zTXt(png_structp png_ptr,
   png_uint_32 length)
{
   png_ptr->current_text = (png_charp)png_large_malloc(png_ptr, length + 1);
   png_ptr->current_text[(png_size_t)length] = '\0';
   png_ptr->current_text_ptr = png_ptr->current_text;
   png_ptr->current_text_size = length;
   png_ptr->current_text_left = length;
   png_ptr->process_mode = PNG_READ_zTXt_MODE;
}

void
png_push_read_ztxt(png_structp png_ptr, png_infop info)
{
   if (png_ptr->buffer_size && png_ptr->current_text_left)
   {
      png_uint_32 text_size;

      if (png_ptr->buffer_size < png_ptr->current_text_left)
         text_size = png_ptr->buffer_size;
      else
         text_size = png_ptr->current_text_left;
      png_push_fill_buffer(png_ptr, (png_bytep)png_ptr->current_text_ptr,
         text_size);
      png_calculate_crc(png_ptr, (png_bytep)png_ptr->current_text_ptr,
         text_size);
      png_ptr->current_text_left -= text_size;
      png_ptr->current_text_ptr += (png_size_t)text_size;
   }
   if (!(png_ptr->current_text_left))
   {
      png_charp text;
      png_charp key;
      int ret;
      png_uint_32 text_size, key_size;

      if (png_ptr->buffer_size < 4)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      png_push_check_crc(png_ptr);

      key = png_ptr->current_text;
      png_ptr->current_text = 0;

      for (text = key; *text; text++)
         /* empty loop */ ;

      /* zTXt can't have zero text */
      if (text == key + (png_size_t)png_ptr->current_text_size)
      {
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
      png_ptr->zstream->avail_in = (uInt)(png_ptr->current_text_size -
         (text - key));
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
               text = (char *)png_large_malloc(png_ptr, text_size +
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
      if (png_ptr->mode == PNG_AT_LAST_IDAT ||
         png_ptr->mode == PNG_AFTER_IDAT)
      {
         png_ptr->process_mode = PNG_READ_END_MODE;
      }
      else
      {
         png_ptr->process_mode = PNG_READ_CHUNK_MODE;
      }
   }
}
#endif

void
png_push_have_info(png_structp png_ptr, png_infop info)
{
   if (png_ptr->info_fn)
      (*(png_ptr->info_fn))(png_ptr, info);
}

void
png_push_have_end(png_structp png_ptr, png_infop info)
{
   if (png_ptr->end_fn)
      (*(png_ptr->end_fn))(png_ptr, info);
}

void
png_push_have_row(png_structp png_ptr, png_bytep row)
{
   if (png_ptr->row_fn)
      (*(png_ptr->row_fn))(png_ptr, row, png_ptr->row_number,
         (int)png_ptr->pass);
}

png_voidp
png_get_progressive_ptr(png_structp png_ptr)
{
   return png_ptr->push_ptr;
}

void
png_push_read_end(png_structp png_ptr, png_infop info)
{
   if (!png_ptr->have_chunk_header)
   {
      png_byte chunk_start[8];

      if (png_ptr->buffer_size < 8)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      png_push_fill_buffer(png_ptr, chunk_start, 8);
      png_ptr->push_length = png_get_uint_32(chunk_start);
      png_memcpy(png_ptr->push_chunk_name, (png_voidp)(chunk_start + 4), 4);
      png_ptr->have_chunk_header = 1;
      png_reset_crc(png_ptr);
      png_calculate_crc(png_ptr, chunk_start + 4, 4);
   }

   if (!png_memcmp(png_ptr->push_chunk_name, png_IHDR, 4))
   {
      png_error(png_ptr, "invalid chunk after IDAT");
   }
   else if (!png_memcmp(png_ptr->push_chunk_name, png_PLTE, 4))
   {
      png_error(png_ptr, "invalid chunk after IDAT");
   }
   else if (!png_memcmp(png_ptr->push_chunk_name, png_IDAT, 4))
   {
      if (png_ptr->push_length > 0 || png_ptr->mode != PNG_AT_LAST_IDAT)
         png_error(png_ptr, "too many IDAT's found");
   }
   else if (!png_memcmp(png_ptr->push_chunk_name, png_IEND, 4))
   {
      if (png_ptr->push_length)
         png_error(png_ptr, "Invalid IEND chunk");
      if (png_ptr->buffer_size < 4)
      {
         png_push_save_buffer(png_ptr);
         return;
      }
      png_push_check_crc(png_ptr);
      png_ptr->mode = PNG_AFTER_IEND;
      png_ptr->process_mode = PNG_READ_DONE_MODE;
      png_push_have_end(png_ptr, info);
   }
#if defined(PNG_READ_gAMA_SUPPORTED)
   else if (!png_memcmp(png_ptr->push_chunk_name, png_gAMA, 4))
   {
      png_error(png_ptr, "invalid chunk after IDAT");
   }
#endif
#if defined(PNG_READ_sBIT_SUPPORTED)
   else if (!png_memcmp(png_ptr->push_chunk_name, png_sBIT, 4))
   {
      png_error(png_ptr, "invalid chunk after IDAT");
   }
#endif
#if defined(PNG_READ_cHRM_SUPPORTED)
   else if (!png_memcmp(png_ptr->push_chunk_name, png_cHRM, 4))
   {
      png_error(png_ptr, "invalid chunk after IDAT");
   }
#endif
#if defined(PNG_READ_tRNS_SUPPORTED)
   else if (!png_memcmp(png_ptr->push_chunk_name, png_tRNS, 4))
   {
      png_error(png_ptr, "invalid chunk after IDAT");
   }
#endif
#if defined(PNG_READ_bKGD_SUPPORTED)
   else if (!png_memcmp(png_ptr->push_chunk_name, png_bKGD, 4))
   {
      png_error(png_ptr, "invalid chunk after IDAT");
   }
#endif
#if defined(PNG_READ_hIST_SUPPORTED)
   else if (!png_memcmp(png_ptr->push_chunk_name, png_hIST, 4))
   {
      png_error(png_ptr, "invalid chunk after IDAT");
   }
#endif
#if defined(PNG_READ_pHYs_SUPPORTED)
   else if (!png_memcmp(png_ptr->push_chunk_name, png_pHYs, 4))
   {
      png_error(png_ptr, "invalid chunk after IDAT");
   }
#endif
#if defined(PNG_READ_oFFs_SUPPORTED)
   else if (!png_memcmp(png_ptr->push_chunk_name, png_oFFs, 4))
   {
      png_error(png_ptr, "invalid chunk after IDAT");
   }
#endif
#if defined(PNG_READ_tIME_SUPPORTED)
   else if (!png_memcmp(png_ptr->push_chunk_name, png_tIME, 4))
   {
      if (png_ptr->push_length + 4 > png_ptr->buffer_size)
      {
         png_push_save_buffer(png_ptr);
         return;
      }

      if (png_ptr->mode == PNG_BEFORE_IHDR ||
         png_ptr->mode == PNG_AFTER_IEND)
         png_error(png_ptr, "Out of Place tIME");

      png_handle_tIME(png_ptr, info, png_ptr->push_length);
      png_push_check_crc(png_ptr);
   }
#endif
#if defined(PNG_READ_tEXt_SUPPORTED)
   else if (!png_memcmp(png_ptr->push_chunk_name, png_tEXt, 4))
   {
      if (png_ptr->mode == PNG_BEFORE_IHDR ||
         png_ptr->mode == PNG_AFTER_IEND)
         png_error(png_ptr, "Out of Place tEXt");

      png_push_handle_tEXt(png_ptr, png_ptr->push_length);
   }
#endif
#if defined(PNG_READ_zTXt_SUPPORTED)
   else if (!png_memcmp(png_ptr->push_chunk_name, png_zTXt, 4))
   {
      if (png_ptr->mode == PNG_BEFORE_IHDR ||
         png_ptr->mode == PNG_AFTER_IEND)
         png_error(png_ptr, "Out of Place zTXt");

      png_push_handle_zTXt(png_ptr, png_ptr->push_length);
   }
#endif
   else
   {
      if ((png_ptr->push_chunk_name[0] & 0x20) == 0)
         png_error(png_ptr, "Unknown Critical Chunk");

      png_push_crc_skip(png_ptr, png_ptr->push_length);
   }
   if (png_ptr->mode == PNG_AT_LAST_IDAT)
      png_ptr->mode = PNG_AFTER_IDAT;
   png_ptr->have_chunk_header = 0;
}

void
png_set_progressive_read_fn(png_structp png_ptr, png_voidp progressive_ptr,
   png_progressive_info_ptr info_fn, png_progressive_row_ptr row_fn,
   png_progressive_end_ptr end_fn)
{
   png_ptr->info_fn = info_fn;
   png_ptr->row_fn = row_fn;
   png_ptr->push_ptr = progressive_ptr;
   png_ptr->end_fn = end_fn;
   png_ptr->read_mode = PNG_READ_PUSH_MODE;
}

void
png_progressive_combine_row (png_structp png_ptr,
   png_bytep old_row, png_bytep new_row)
{
   if (new_row)
      png_combine_row(png_ptr, old_row, png_pass_dsp_mask[png_ptr->pass]);
}

#endif /* PNG_PROGRESSIVE_READ_SUPPORTED */

