/**************************************************************************
 *
 * Copyright 2011 Christian König.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/*
 * Functions for fast bitwise access to multiple probably unaligned input buffers
 */

#ifndef vl_vlc_h
#define vl_vlc_h

#include "pipe/p_compiler.h"

#include "util/u_math.h"
#include "util/u_pointer.h"
#include "util/u_debug.h"

struct vl_vlc
{
   uint64_t buffer;
   signed invalid_bits;
   const uint8_t *data;
   const uint8_t *end;

   unsigned          num_inputs;
   const void *const *inputs;
   const unsigned    *sizes;
   unsigned          bytes_left;
};

struct vl_vlc_entry
{
   int8_t length;
   int8_t value;
};

struct vl_vlc_compressed
{
   uint16_t bitcode;
   struct vl_vlc_entry entry;
};

/**
 * initalize and decompress a lookup table
 */
static INLINE void
vl_vlc_init_table(struct vl_vlc_entry *dst, unsigned dst_size, const struct vl_vlc_compressed *src, unsigned src_size)
{
   unsigned i, bits = util_logbase2(dst_size);

   assert(dst && dst_size);
   assert(src && src_size);

   for (i=0;i<dst_size;++i) {
      dst[i].length = 0;
      dst[i].value = 0;
   }

   for(; src_size > 0; --src_size, ++src) {
      for(i=0; i<(1 << (bits - src->entry.length)); ++i)
         dst[src->bitcode >> (16 - bits) | i] = src->entry;
   }
}

/**
 * switch over to next input buffer
 */
static INLINE void
vl_vlc_next_input(struct vl_vlc *vlc)
{
   const uint8_t* data = vlc->inputs[0];
   unsigned len = vlc->sizes[0];

   assert(vlc);
   assert(vlc->num_inputs);

   vlc->bytes_left -= len;

   /* align the data pointer */
   while (len && pointer_to_uintptr(data) & 3) {
      vlc->buffer |= (uint64_t)*data << (24 + vlc->invalid_bits);
      ++data;
      --len;
      vlc->invalid_bits -= 8;
   }
   vlc->data = data;
   vlc->end = data + len;

   --vlc->num_inputs;
   ++vlc->inputs;
   ++vlc->sizes;
}

/**
 * fill the bit buffer, so that at least 32 bits are valid
 */
static INLINE void
vl_vlc_fillbits(struct vl_vlc *vlc)
{
   assert(vlc);

   /* as long as the buffer needs to be filled */
   while (vlc->invalid_bits > 0) {
      unsigned bytes_left = vlc->end - vlc->data;

      /* if this input is depleted */
      if (bytes_left == 0) {

         if (vlc->num_inputs)
            /* go on to next input */
            vl_vlc_next_input(vlc);
         else
            /* or give up since we don't have anymore inputs */
            return;

      } else if (bytes_left >= 4) {

         /* enough bytes in buffer, read in a whole dword */
         uint64_t value = *(const uint32_t*)vlc->data;

#ifndef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif

         vlc->buffer |= value << vlc->invalid_bits;
         vlc->data += 4;
         vlc->invalid_bits -= 32;

         /* buffer is now definitely filled up avoid the loop test */
         break;

      } else while (vlc->data < vlc->end) {

         /* not enough bytes left in buffer, read single bytes */
         vlc->buffer |= (uint64_t)*vlc->data << (24 + vlc->invalid_bits);
         ++vlc->data;
         vlc->invalid_bits -= 8;
      }
   }
}

/**
 * initialize vlc structure and start reading from first input buffer
 */
static INLINE void
vl_vlc_init(struct vl_vlc *vlc, unsigned num_inputs,
            const void *const *inputs, const unsigned *sizes)
{
   unsigned i;

   assert(vlc);
   assert(num_inputs);

   vlc->buffer = 0;
   vlc->invalid_bits = 32;
   vlc->num_inputs = num_inputs;
   vlc->inputs = inputs;
   vlc->sizes = sizes;
   vlc->bytes_left = 0;

   for (i = 0; i < num_inputs; ++i)
      vlc->bytes_left += sizes[i];

   vl_vlc_next_input(vlc);
   vl_vlc_fillbits(vlc);
   vl_vlc_fillbits(vlc);
}

/**
 * number of bits still valid in bit buffer
 */
static INLINE unsigned
vl_vlc_valid_bits(struct vl_vlc *vlc)
{
   return 32 - vlc->invalid_bits;
}

/**
 * number of bits left over all inbut buffers
 */
static INLINE unsigned
vl_vlc_bits_left(struct vl_vlc *vlc)
{
   signed bytes_left = vlc->end - vlc->data;
   bytes_left += vlc->bytes_left;
   return bytes_left * 8 + vl_vlc_valid_bits(vlc);
}

/**
 * get num_bits from bit buffer without removing them
 */
static INLINE unsigned
vl_vlc_peekbits(struct vl_vlc *vlc, unsigned num_bits)
{
   assert(vl_vlc_valid_bits(vlc) >= num_bits || vlc->data >= vlc->end);
   return vlc->buffer >> (64 - num_bits);
}

/**
 * remove num_bits from bit buffer
 */
static INLINE void
vl_vlc_eatbits(struct vl_vlc *vlc, unsigned num_bits)
{
   assert(vl_vlc_valid_bits(vlc) >= num_bits);

   vlc->buffer <<= num_bits;
   vlc->invalid_bits += num_bits;
}

/**
 * get num_bits from bit buffer with removing them
 */
static INLINE unsigned
vl_vlc_get_uimsbf(struct vl_vlc *vlc, unsigned num_bits)
{
   unsigned value;

   assert(vl_vlc_valid_bits(vlc) >= num_bits);

   value = vlc->buffer >> (64 - num_bits);
   vl_vlc_eatbits(vlc, num_bits);

   return value;
}

/**
 * treat num_bits as signed value and remove them from bit buffer
 */
static INLINE signed
vl_vlc_get_simsbf(struct vl_vlc *vlc, unsigned num_bits)
{
   signed value;

   assert(vl_vlc_valid_bits(vlc) >= num_bits);

   value = ((int64_t)vlc->buffer) >> (64 - num_bits);
   vl_vlc_eatbits(vlc, num_bits);

   return value;
}

/**
 * lookup a value and length in a decompressed table
 */
static INLINE int8_t
vl_vlc_get_vlclbf(struct vl_vlc *vlc, const struct vl_vlc_entry *tbl, unsigned num_bits)
{
   tbl += vl_vlc_peekbits(vlc, num_bits);
   vl_vlc_eatbits(vlc, tbl->length);
   assert(tbl->length);
   return tbl->value;
}

#endif /* vl_vlc_h */
