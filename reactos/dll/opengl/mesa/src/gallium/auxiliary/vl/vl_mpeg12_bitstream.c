/**************************************************************************
 *
 * Copyright 2011 Maarten Lankhorst
 * Copyright 2011 Christian König
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

#include "pipe/p_video_decoder.h"
#include "util/u_memory.h"

#include "vl_vlc.h"
#include "vl_mpeg12_bitstream.h"

enum {
   dct_End_of_Block = 0xFF,
   dct_Escape = 0xFE,
   dct_DC = 0xFD,
   dct_AC = 0xFC
};

struct dct_coeff
{
   uint8_t length;
   uint8_t run;
   int16_t level;
};

struct dct_coeff_compressed
{
   uint32_t bitcode;
   struct dct_coeff coeff;
};

/* coding table as found in the spec annex B.5 table B-1 */
static const struct vl_vlc_compressed macroblock_address_increment[] = {
   { 0x8000, { 1, 1 } },
   { 0x6000, { 3, 2 } },
   { 0x4000, { 3, 3 } },
   { 0x3000, { 4, 4 } },
   { 0x2000, { 4, 5 } },
   { 0x1800, { 5, 6 } },
   { 0x1000, { 5, 7 } },
   { 0x0e00, { 7, 8 } },
   { 0x0c00, { 7, 9 } },
   { 0x0b00, { 8, 10 } },
   { 0x0a00, { 8, 11 } },
   { 0x0900, { 8, 12 } },
   { 0x0800, { 8, 13 } },
   { 0x0700, { 8, 14 } },
   { 0x0600, { 8, 15 } },
   { 0x05c0, { 10, 16 } },
   { 0x0580, { 10, 17 } },
   { 0x0540, { 10, 18 } },
   { 0x0500, { 10, 19 } },
   { 0x04c0, { 10, 20 } },
   { 0x0480, { 10, 21 } },
   { 0x0460, { 11, 22 } },
   { 0x0440, { 11, 23 } },
   { 0x0420, { 11, 24 } },
   { 0x0400, { 11, 25 } },
   { 0x03e0, { 11, 26 } },
   { 0x03c0, { 11, 27 } },
   { 0x03a0, { 11, 28 } },
   { 0x0380, { 11, 29 } },
   { 0x0360, { 11, 30 } },
   { 0x0340, { 11, 31 } },
   { 0x0320, { 11, 32 } },
   { 0x0300, { 11, 33 } }
};

#define Q PIPE_MPEG12_MB_TYPE_QUANT
#define F PIPE_MPEG12_MB_TYPE_MOTION_FORWARD
#define B PIPE_MPEG12_MB_TYPE_MOTION_BACKWARD
#define P PIPE_MPEG12_MB_TYPE_PATTERN
#define I PIPE_MPEG12_MB_TYPE_INTRA

/* coding table as found in the spec annex B.5 table B-2 */
static const struct vl_vlc_compressed macroblock_type_i[] = {
   { 0x8000, { 1, I } },
   { 0x4000, { 2, Q|I } }
};

/* coding table as found in the spec annex B.5 table B-3 */
static const struct vl_vlc_compressed macroblock_type_p[] = {
   { 0x8000, { 1, F|P } },
   { 0x4000, { 2, P } },
   { 0x2000, { 3, F } },
   { 0x1800, { 5, I } },
   { 0x1000, { 5, Q|F|P } },
   { 0x0800, { 5, Q|P } },
   { 0x0400, { 6, Q|I } }
};

/* coding table as found in the spec annex B.5 table B-4 */
static const struct vl_vlc_compressed macroblock_type_b[] = {
   { 0x8000, { 2, F|B } },
   { 0xC000, { 2, F|B|P } },
   { 0x4000, { 3, B } },
   { 0x6000, { 3, B|P } },
   { 0x2000, { 4, F } },
   { 0x3000, { 4, F|P } },
   { 0x1800, { 5, I } },
   { 0x1000, { 5, Q|F|B|P } },
   { 0x0C00, { 6, Q|F|P } },
   { 0x0800, { 6, Q|B|P } },
   { 0x0400, { 6, Q|I } }
};

#undef Q
#undef F
#undef B
#undef P
#undef I

/* coding table as found in the spec annex B.5 table B-9 */
static const struct vl_vlc_compressed coded_block_pattern[] = {
   { 0xE000, { 3, 60 } },
   { 0xD000, { 4, 4 } },
   { 0xC000, { 4, 8 } },
   { 0xB000, { 4, 16 } },
   { 0xA000, { 4, 32 } },
   { 0x9800, { 5, 12 } },
   { 0x9000, { 5, 48 } },
   { 0x8800, { 5, 20 } },
   { 0x8000, { 5, 40 } },
   { 0x7800, { 5, 28 } },
   { 0x7000, { 5, 44 } },
   { 0x6800, { 5, 52 } },
   { 0x6000, { 5, 56 } },
   { 0x5800, { 5, 1 } },
   { 0x5000, { 5, 61 } },
   { 0x4800, { 5, 2 } },
   { 0x4000, { 5, 62 } },
   { 0x3C00, { 6, 24 } },
   { 0x3800, { 6, 36 } },
   { 0x3400, { 6, 3 } },
   { 0x3000, { 6, 63 } },
   { 0x2E00, { 7, 5 } },
   { 0x2C00, { 7, 9 } },
   { 0x2A00, { 7, 17 } },
   { 0x2800, { 7, 33 } },
   { 0x2600, { 7, 6 } },
   { 0x2400, { 7, 10 } },
   { 0x2200, { 7, 18 } },
   { 0x2000, { 7, 34 } },
   { 0x1F00, { 8, 7 } },
   { 0x1E00, { 8, 11 } },
   { 0x1D00, { 8, 19 } },
   { 0x1C00, { 8, 35 } },
   { 0x1B00, { 8, 13 } },
   { 0x1A00, { 8, 49 } },
   { 0x1900, { 8, 21 } },
   { 0x1800, { 8, 41 } },
   { 0x1700, { 8, 14 } },
   { 0x1600, { 8, 50 } },
   { 0x1500, { 8, 22 } },
   { 0x1400, { 8, 42 } },
   { 0x1300, { 8, 15 } },
   { 0x1200, { 8, 51 } },
   { 0x1100, { 8, 23 } },
   { 0x1000, { 8, 43 } },
   { 0x0F00, { 8, 25 } },
   { 0x0E00, { 8, 37 } },
   { 0x0D00, { 8, 26 } },
   { 0x0C00, { 8, 38 } },
   { 0x0B00, { 8, 29 } },
   { 0x0A00, { 8, 45 } },
   { 0x0900, { 8, 53 } },
   { 0x0800, { 8, 57 } },
   { 0x0700, { 8, 30 } },
   { 0x0600, { 8, 46 } },
   { 0x0500, { 8, 54 } },
   { 0x0400, { 8, 58 } },
   { 0x0380, { 9, 31 } },
   { 0x0300, { 9, 47 } },
   { 0x0280, { 9, 55 } },
   { 0x0200, { 9, 59 } },
   { 0x0180, { 9, 27 } },
   { 0x0100, { 9, 39 } },
   { 0x0080, { 9, 0 } }
};

/* coding table as found in the spec annex B.5 table B-10 */
static const struct vl_vlc_compressed motion_code[] = {
   { 0x0320, { 11, -16 } },
   { 0x0360, { 11, -15 } },
   { 0x03a0, { 11, -14 } },
   { 0x03e0, { 11, -13 } },
   { 0x0420, { 11, -12 } },
   { 0x0460, { 11, -11 } },
   { 0x04c0, { 10, -10 } },
   { 0x0540, { 10, -9 } },
   { 0x05c0, { 10, -8 } },
   { 0x0700, { 8, -7 } },
   { 0x0900, { 8, -6 } },
   { 0x0b00, { 8, -5 } },
   { 0x0e00, { 7, -4 } },
   { 0x1800, { 5, -3 } },
   { 0x3000, { 4, -2 } },
   { 0x6000, { 3, -1 } },
   { 0x8000, { 1, 0 } },
   { 0x4000, { 3, 1 } },
   { 0x2000, { 4, 2 } },
   { 0x1000, { 5, 3 } },
   { 0x0c00, { 7, 4 } },
   { 0x0a00, { 8, 5 } },
   { 0x0800, { 8, 6 } },
   { 0x0600, { 8, 7 } },
   { 0x0580, { 10, 8 } },
   { 0x0500, { 10, 9 } },
   { 0x0480, { 10, 10 } },
   { 0x0440, { 11, 11 } },
   { 0x0400, { 11, 12 } },
   { 0x03c0, { 11, 13 } },
   { 0x0380, { 11, 14 } },
   { 0x0340, { 11, 15 } },
   { 0x0300, { 11, 16 } }
};

/* coding table as found in the spec annex B.5 table B-11 */
static const struct vl_vlc_compressed dmvector[] = {
   { 0x0000, { 1, 0 } },
   { 0x8000, { 2, 1 } },
   { 0xc000, { 2, -1 } }
};

/* coding table as found in the spec annex B.5 table B-12 */
static const struct vl_vlc_compressed dct_dc_size_luminance[] = {
   { 0x8000, { 3, 0 } },
   { 0x0000, { 2, 1 } },
   { 0x4000, { 2, 2 } },
   { 0xA000, { 3, 3 } },
   { 0xC000, { 3, 4 } },
   { 0xE000, { 4, 5 } },
   { 0xF000, { 5, 6 } },
   { 0xF800, { 6, 7 } },
   { 0xFC00, { 7, 8 } },
   { 0xFE00, { 8, 9 } },
   { 0xFF00, { 9, 10 } },
   { 0xFF80, { 9, 11 } }
};

/* coding table as found in the spec annex B.5 table B-13 */
static const struct vl_vlc_compressed dct_dc_size_chrominance[] = {
   { 0x0000, { 2, 0 } },
   { 0x4000, { 2, 1 } },
   { 0x8000, { 2, 2 } },
   { 0xC000, { 3, 3 } },
   { 0xE000, { 4, 4 } },
   { 0xF000, { 5, 5 } },
   { 0xF800, { 6, 6 } },
   { 0xFC00, { 7, 7 } },
   { 0xFE00, { 8, 8 } },
   { 0xFF00, { 9, 9 } },
   { 0xFF80, { 10, 10 } },
   { 0xFFC0, { 10, 11 } }
};

/* coding table as found in the spec annex B.5 table B-14 */
static const struct dct_coeff_compressed dct_coeff_tbl_zero[] = {
   { 0x8000, { 2, dct_End_of_Block, 0 } },
   { 0x8000, { 1, dct_DC, 1 } },
   { 0xC000, { 2, dct_AC, 1 } },
   { 0x6000, { 3, 1, 1 } },
   { 0x4000, { 4, 0, 2 } },
   { 0x5000, { 4, 2, 1 } },
   { 0x2800, { 5, 0, 3 } },
   { 0x3800, { 5, 3, 1 } },
   { 0x3000, { 5, 4, 1 } },
   { 0x1800, { 6, 1, 2 } },
   { 0x1C00, { 6, 5, 1 } },
   { 0x1400, { 6, 6, 1 } },
   { 0x1000, { 6, 7, 1 } },
   { 0x0C00, { 7, 0, 4 } },
   { 0x0800, { 7, 2, 2 } },
   { 0x0E00, { 7, 8, 1 } },
   { 0x0A00, { 7, 9, 1 } },
   { 0x0400, { 6, dct_Escape, 0 } },
   { 0x2600, { 8, 0, 5 } },
   { 0x2100, { 8, 0, 6 } },
   { 0x2500, { 8, 1, 3 } },
   { 0x2400, { 8, 3, 2 } },
   { 0x2700, { 8, 10, 1 } },
   { 0x2300, { 8, 11, 1 } },
   { 0x2200, { 8, 12, 1 } },
   { 0x2000, { 8, 13, 1 } },
   { 0x0280, { 10, 0, 7 } },
   { 0x0300, { 10, 1, 4 } },
   { 0x02C0, { 10, 2, 3 } },
   { 0x03C0, { 10, 4, 2 } },
   { 0x0240, { 10, 5, 2 } },
   { 0x0380, { 10, 14, 1 } },
   { 0x0340, { 10, 15, 1 } },
   { 0x0200, { 10, 16, 1 } },
   { 0x01D0, { 12, 0, 8 } },
   { 0x0180, { 12, 0, 9 } },
   { 0x0130, { 12, 0, 10 } },
   { 0x0100, { 12, 0, 11 } },
   { 0x01B0, { 12, 1, 5 } },
   { 0x0140, { 12, 2, 4 } },
   { 0x01C0, { 12, 3, 3 } },
   { 0x0120, { 12, 4, 3 } },
   { 0x01E0, { 12, 6, 2 } },
   { 0x0150, { 12, 7, 2 } },
   { 0x0110, { 12, 8, 2 } },
   { 0x01F0, { 12, 17, 1 } },
   { 0x01A0, { 12, 18, 1 } },
   { 0x0190, { 12, 19, 1 } },
   { 0x0170, { 12, 20, 1 } },
   { 0x0160, { 12, 21, 1 } },
   { 0x00D0, { 13, 0, 12 } },
   { 0x00C8, { 13, 0, 13 } },
   { 0x00C0, { 13, 0, 14 } },
   { 0x00B8, { 13, 0, 15 } },
   { 0x00B0, { 13, 1, 6 } },
   { 0x00A8, { 13, 1, 7 } },
   { 0x00A0, { 13, 2, 5 } },
   { 0x0098, { 13, 3, 4 } },
   { 0x0090, { 13, 5, 3 } },
   { 0x0088, { 13, 9, 2 } },
   { 0x0080, { 13, 10, 2 } },
   { 0x00F8, { 13, 22, 1 } },
   { 0x00F0, { 13, 23, 1 } },
   { 0x00E8, { 13, 24, 1 } },
   { 0x00E0, { 13, 25, 1 } },
   { 0x00D8, { 13, 26, 1 } },
   { 0x007C, { 14, 0, 16 } },
   { 0x0078, { 14, 0, 17 } },
   { 0x0074, { 14, 0, 18 } },
   { 0x0070, { 14, 0, 19 } },
   { 0x006C, { 14, 0, 20 } },
   { 0x0068, { 14, 0, 21 } },
   { 0x0064, { 14, 0, 22 } },
   { 0x0060, { 14, 0, 23 } },
   { 0x005C, { 14, 0, 24 } },
   { 0x0058, { 14, 0, 25 } },
   { 0x0054, { 14, 0, 26 } },
   { 0x0050, { 14, 0, 27 } },
   { 0x004C, { 14, 0, 28 } },
   { 0x0048, { 14, 0, 29 } },
   { 0x0044, { 14, 0, 30 } },
   { 0x0040, { 14, 0, 31 } },
   { 0x0030, { 15, 0, 32 } },
   { 0x002E, { 15, 0, 33 } },
   { 0x002C, { 15, 0, 34 } },
   { 0x002A, { 15, 0, 35 } },
   { 0x0028, { 15, 0, 36 } },
   { 0x0026, { 15, 0, 37 } },
   { 0x0024, { 15, 0, 38 } },
   { 0x0022, { 15, 0, 39 } },
   { 0x0020, { 15, 0, 40 } },
   { 0x003E, { 15, 1, 8 } },
   { 0x003C, { 15, 1, 9 } },
   { 0x003A, { 15, 1, 10 } },
   { 0x0038, { 15, 1, 11 } },
   { 0x0036, { 15, 1, 12 } },
   { 0x0034, { 15, 1, 13 } },
   { 0x0032, { 15, 1, 14 } },
   { 0x0013, { 16, 1, 15 } },
   { 0x0012, { 16, 1, 16 } },
   { 0x0011, { 16, 1, 17 } },
   { 0x0010, { 16, 1, 18 } },
   { 0x0014, { 16, 6, 3 } },
   { 0x001A, { 16, 11, 2 } },
   { 0x0019, { 16, 12, 2 } },
   { 0x0018, { 16, 13, 2 } },
   { 0x0017, { 16, 14, 2 } },
   { 0x0016, { 16, 15, 2 } },
   { 0x0015, { 16, 16, 2 } },
   { 0x001F, { 16, 27, 1 } },
   { 0x001E, { 16, 28, 1 } },
   { 0x001D, { 16, 29, 1 } },
   { 0x001C, { 16, 30, 1 } },
   { 0x001B, { 16, 31, 1 } }
};

/* coding table as found in the spec annex B.5 table B-15 */
static const struct dct_coeff_compressed dct_coeff_tbl_one[] = {
   { 0x6000, { 4, dct_End_of_Block, 0 } },
   { 0x8000, { 2, 0, 1 } },
   { 0x4000, { 3, 1, 1 } },
   { 0xC000, { 3, 0, 2 } },
   { 0x2800, { 5, 2, 1 } },
   { 0x7000, { 4, 0, 3 } },
   { 0x3800, { 5, 3, 1 } },
   { 0x1800, { 6, 4, 1 } },
   { 0x3000, { 5, 1, 2 } },
   { 0x1C00, { 6, 5, 1 } },
   { 0x0C00, { 7, 6, 1 } },
   { 0x0800, { 7, 7, 1 } },
   { 0xE000, { 5, 0, 4 } },
   { 0x0E00, { 7, 2, 2 } },
   { 0x0A00, { 7, 8, 1 } },
   { 0xF000, { 7, 9, 1 } },
   { 0x0400, { 6, dct_Escape, 0 } },
   { 0xE800, { 5, 0, 5 } },
   { 0x1400, { 6, 0, 6 } },
   { 0xF200, { 7, 1, 3 } },
   { 0x2600, { 8, 3, 2 } },
   { 0xF400, { 7, 10, 1 } },
   { 0x2100, { 8, 11, 1 } },
   { 0x2500, { 8, 12, 1 } },
   { 0x2400, { 8, 13, 1 } },
   { 0x1000, { 6, 0, 7 } },
   { 0x2700, { 8, 1, 4 } },
   { 0xFC00, { 8, 2, 3 } },
   { 0xFD00, { 8, 4, 2 } },
   { 0x0200, { 9, 5, 2 } },
   { 0x0280, { 9, 14, 1 } },
   { 0x0380, { 9, 15, 1 } },
   { 0x0340, { 10, 16, 1 } },
   { 0xF600, { 7, 0, 8 } },
   { 0xF800, { 7, 0, 9 } },
   { 0x2300, { 8, 0, 10 } },
   { 0x2200, { 8, 0, 11 } },
   { 0x2000, { 8, 1, 5 } },
   { 0x0300, { 10, 2, 4 } },
   { 0x01C0, { 12, 3, 3 } },
   { 0x0120, { 12, 4, 3 } },
   { 0x01E0, { 12, 6, 2 } },
   { 0x0150, { 12, 7, 2 } },
   { 0x0110, { 12, 8, 2 } },
   { 0x01F0, { 12, 17, 1 } },
   { 0x01A0, { 12, 18, 1 } },
   { 0x0190, { 12, 19, 1 } },
   { 0x0170, { 12, 20, 1 } },
   { 0x0160, { 12, 21, 1 } },
   { 0xFA00, { 8, 0, 12 } },
   { 0xFB00, { 8, 0, 13 } },
   { 0xFE00, { 8, 0, 14 } },
   { 0xFF00, { 8, 0, 15 } },
   { 0x00B0, { 13, 1, 6 } },
   { 0x00A8, { 13, 1, 7 } },
   { 0x00A0, { 13, 2, 5 } },
   { 0x0098, { 13, 3, 4 } },
   { 0x0090, { 13, 5, 3 } },
   { 0x0088, { 13, 9, 2 } },
   { 0x0080, { 13, 10, 2 } },
   { 0x00F8, { 13, 22, 1 } },
   { 0x00F0, { 13, 23, 1 } },
   { 0x00E8, { 13, 24, 1 } },
   { 0x00E0, { 13, 25, 1 } },
   { 0x00D8, { 13, 26, 1 } },
   { 0x007C, { 14, 0, 16 } },
   { 0x0078, { 14, 0, 17 } },
   { 0x0074, { 14, 0, 18 } },
   { 0x0070, { 14, 0, 19 } },
   { 0x006C, { 14, 0, 20 } },
   { 0x0068, { 14, 0, 21 } },
   { 0x0064, { 14, 0, 22 } },
   { 0x0060, { 14, 0, 23 } },
   { 0x005C, { 14, 0, 24 } },
   { 0x0058, { 14, 0, 25 } },
   { 0x0054, { 14, 0, 26 } },
   { 0x0050, { 14, 0, 27 } },
   { 0x004C, { 14, 0, 28 } },
   { 0x0048, { 14, 0, 29 } },
   { 0x0044, { 14, 0, 30 } },
   { 0x0040, { 14, 0, 31 } },
   { 0x0030, { 15, 0, 32 } },
   { 0x002E, { 15, 0, 33 } },
   { 0x002C, { 15, 0, 34 } },
   { 0x002A, { 15, 0, 35 } },
   { 0x0028, { 15, 0, 36 } },
   { 0x0026, { 15, 0, 37 } },
   { 0x0024, { 15, 0, 38 } },
   { 0x0022, { 15, 0, 39 } },
   { 0x0020, { 15, 0, 40 } },
   { 0x003E, { 15, 1, 8 } },
   { 0x003C, { 15, 1, 9 } },
   { 0x003A, { 15, 1, 10 } },
   { 0x0038, { 15, 1, 11 } },
   { 0x0036, { 15, 1, 12 } },
   { 0x0034, { 15, 1, 13 } },
   { 0x0032, { 15, 1, 14 } },
   { 0x0013, { 16, 1, 15 } },
   { 0x0012, { 16, 1, 16 } },
   { 0x0011, { 16, 1, 17 } },
   { 0x0010, { 16, 1, 18 } },
   { 0x0014, { 16, 6, 3 } },
   { 0x001A, { 16, 11, 2 } },
   { 0x0019, { 16, 12, 2 } },
   { 0x0018, { 16, 13, 2 } },
   { 0x0017, { 16, 14, 2 } },
   { 0x0016, { 16, 15, 2 } },
   { 0x0015, { 16, 16, 2 } },
   { 0x001F, { 16, 27, 1 } },
   { 0x001E, { 16, 28, 1 } },
   { 0x001D, { 16, 29, 1 } },
   { 0x001C, { 16, 30, 1 } },
   { 0x001B, { 16, 31, 1 } }
};

/* q_scale_type */
static const unsigned quant_scale[2][32] = {
  { 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30,
    32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62 },
  { 0, 1, 2, 3, 4,  5,  6,  7,  8, 10, 12, 14, 16, 18, 20, 22, 24,
    28, 32, 36, 40, 44, 48, 52, 56, 64, 72, 80, 88, 96, 104, 112 }
};

static struct vl_vlc_entry tbl_B1[1 << 11];
static struct vl_vlc_entry tbl_B2[1 << 2];
static struct vl_vlc_entry tbl_B3[1 << 6];
static struct vl_vlc_entry tbl_B4[1 << 6];
static struct vl_vlc_entry tbl_B9[1 << 9];
static struct vl_vlc_entry tbl_B10[1 << 11];
static struct vl_vlc_entry tbl_B11[1 << 2];
static struct vl_vlc_entry tbl_B12[1 << 10];
static struct vl_vlc_entry tbl_B13[1 << 10];
static struct dct_coeff tbl_B14_DC[1 << 17];
static struct dct_coeff tbl_B14_AC[1 << 17];
static struct dct_coeff tbl_B15[1 << 17];

static INLINE void
init_dct_coeff_table(struct dct_coeff *dst, const struct dct_coeff_compressed *src,
                     unsigned size, bool is_DC)
{
   unsigned i;

   for (i=0;i<(1<<17);++i) {
      dst[i].length = 0;
      dst[i].level = 0;
      dst[i].run = dct_End_of_Block;
   }

   for(; size > 0; --size, ++src) {
      struct dct_coeff coeff = src->coeff;
      bool has_sign = true;

      switch (coeff.run) {
      case dct_End_of_Block:
         if (is_DC)
            continue;

         has_sign = false;
         break;

      case dct_Escape:
         has_sign = false;
         break;

      case dct_DC:
         if (!is_DC)
            continue;

         coeff.length += 1;
         coeff.run = 1;
         break;

      case dct_AC:
         if (is_DC)
            continue;

         coeff.length += 1;
         coeff.run = 1;
         break;

      default:
         coeff.length += 1;
         coeff.run += 1;
         break;
      }

      for(i=0; i<(1 << (17 - coeff.length)); ++i)
         dst[src->bitcode << 1 | i] = coeff;

      if (has_sign) {
	 coeff.level = -coeff.level;
         for(; i<(1 << (18 - coeff.length)); ++i)
            dst[src->bitcode << 1 | i] = coeff;
      }
   }
}

static INLINE void
init_tables()
{
   vl_vlc_init_table(tbl_B1, Elements(tbl_B1), macroblock_address_increment, Elements(macroblock_address_increment));
   vl_vlc_init_table(tbl_B2, Elements(tbl_B2), macroblock_type_i, Elements(macroblock_type_i));
   vl_vlc_init_table(tbl_B3, Elements(tbl_B3), macroblock_type_p, Elements(macroblock_type_p));
   vl_vlc_init_table(tbl_B4, Elements(tbl_B4), macroblock_type_b, Elements(macroblock_type_b));
   vl_vlc_init_table(tbl_B9, Elements(tbl_B9), coded_block_pattern, Elements(coded_block_pattern));
   vl_vlc_init_table(tbl_B10, Elements(tbl_B10), motion_code, Elements(motion_code));
   vl_vlc_init_table(tbl_B11, Elements(tbl_B11), dmvector, Elements(dmvector));
   vl_vlc_init_table(tbl_B12, Elements(tbl_B12), dct_dc_size_luminance, Elements(dct_dc_size_luminance));
   vl_vlc_init_table(tbl_B13, Elements(tbl_B13), dct_dc_size_chrominance, Elements(dct_dc_size_chrominance));
   init_dct_coeff_table(tbl_B14_DC, dct_coeff_tbl_zero, Elements(dct_coeff_tbl_zero), true);
   init_dct_coeff_table(tbl_B14_AC, dct_coeff_tbl_zero, Elements(dct_coeff_tbl_zero), false);
   init_dct_coeff_table(tbl_B15, dct_coeff_tbl_one, Elements(dct_coeff_tbl_one), false);
}

static INLINE int
DIV2DOWN(int todiv)
{
   return (todiv&~1)/2;
}

static INLINE int
DIV2UP(int todiv)
{
   return (todiv+1)/2;
}

static INLINE void
motion_vector(struct vl_mpg12_bs *bs, int r, int s, int dmv, short delta[2], short dmvector[2])
{
   int t;
   for (t = 0; t < 2; ++t) {
      int motion_code;
      int r_size = bs->desc.f_code[s][t];

      vl_vlc_fillbits(&bs->vlc);
      motion_code = vl_vlc_get_vlclbf(&bs->vlc, tbl_B10, 11);

      assert(r_size >= 0);
      if (r_size && motion_code) {
         int residual = vl_vlc_get_uimsbf(&bs->vlc, r_size) + 1;
         delta[t] = ((abs(motion_code) - 1) << r_size) + residual;
         if (motion_code < 0)
            delta[t] = -delta[t];
      } else
         delta[t] = motion_code;
      if (dmv)
         dmvector[t] = vl_vlc_get_vlclbf(&bs->vlc, tbl_B11, 2);
   }
}

static INLINE int
wrap(short f, int shift)
{
   if (f < (-16 << shift))
      return f + (32 << shift);
   else if (f >= 16 << shift)
      return f - (32 << shift);
   else
      return f;
}

static INLINE void
motion_vector_frame(struct vl_mpg12_bs *bs, int s, struct pipe_mpeg12_macroblock *mb)
{
   int dmv = mb->macroblock_modes.bits.frame_motion_type == PIPE_MPEG12_MO_TYPE_DUAL_PRIME;
   short dmvector[2], delta[2];

   if (mb->macroblock_modes.bits.frame_motion_type == PIPE_MPEG12_MO_TYPE_FIELD) {
      mb->motion_vertical_field_select |= vl_vlc_get_uimsbf(&bs->vlc, 1) << s;
      motion_vector(bs, 0, s, dmv, delta, dmvector);
      mb->PMV[0][s][0] = wrap(mb->PMV[0][s][0] + delta[0], bs->desc.f_code[s][0]);
      mb->PMV[0][s][1] = wrap(DIV2DOWN(mb->PMV[0][s][1]) + delta[1], bs->desc.f_code[s][1]) * 2;

      mb->motion_vertical_field_select |= vl_vlc_get_uimsbf(&bs->vlc, 1) << (s + 2);
      motion_vector(bs, 1, s, dmv, delta, dmvector);
      mb->PMV[1][s][0] = wrap(mb->PMV[1][s][0] + delta[0], bs->desc.f_code[s][0]);
      mb->PMV[1][s][1] = wrap(DIV2DOWN(mb->PMV[1][s][1]) + delta[1], bs->desc.f_code[s][1]) * 2;

   } else {
      motion_vector(bs, 0, s, dmv, delta, dmvector);
      mb->PMV[0][s][0] = wrap(mb->PMV[0][s][0] + delta[0], bs->desc.f_code[s][0]);
      mb->PMV[0][s][1] = wrap(mb->PMV[0][s][1] + delta[1], bs->desc.f_code[s][1]);
   }
}

static INLINE void
motion_vector_field(struct vl_mpg12_bs *bs, int s, struct pipe_mpeg12_macroblock *mb)
{
   int dmv = mb->macroblock_modes.bits.field_motion_type == PIPE_MPEG12_MO_TYPE_DUAL_PRIME;
   short dmvector[2], delta[2];

   if (mb->macroblock_modes.bits.field_motion_type == PIPE_MPEG12_MO_TYPE_16x8) {
      mb->motion_vertical_field_select |= vl_vlc_get_uimsbf(&bs->vlc, 1) << s;
      motion_vector(bs, 0, s, dmv, delta, dmvector);

      mb->motion_vertical_field_select |= vl_vlc_get_uimsbf(&bs->vlc, 1) << (s + 2);
      motion_vector(bs, 1, s, dmv, delta, dmvector);
   } else {
      if (!dmv)
         mb->motion_vertical_field_select |= vl_vlc_get_uimsbf(&bs->vlc, 1) << s;
      motion_vector(bs, 0, s, dmv, delta, dmvector);
   }
}

static INLINE void
reset_predictor(struct vl_mpg12_bs *bs) {
   bs->pred_dc[0] = bs->pred_dc[1] = bs->pred_dc[2] = 0;
}

static INLINE void
decode_dct(struct vl_mpg12_bs *bs, struct pipe_mpeg12_macroblock *mb, int scale)
{
   static const unsigned blk2cc[] = { 0, 0, 0, 0, 1, 2 };
   static const struct vl_vlc_entry *blk2dcsize[] = {
      tbl_B12, tbl_B12, tbl_B12, tbl_B12, tbl_B13, tbl_B13
   };

   bool intra = mb->macroblock_type & PIPE_MPEG12_MB_TYPE_INTRA;
   const struct dct_coeff *table = intra ? bs->intra_dct_tbl : tbl_B14_AC;
   const struct dct_coeff *entry;
   int i, cbp, blk = 0;
   short *dst = mb->blocks;

   vl_vlc_fillbits(&bs->vlc);
   mb->coded_block_pattern = cbp = intra ? 0x3F : vl_vlc_get_vlclbf(&bs->vlc, tbl_B9, 9);

   goto entry;

   while(1) {
      vl_vlc_eatbits(&bs->vlc, entry->length);
      if (entry->run == dct_End_of_Block) {

         dst += 64;
         cbp <<= 1;
         cbp &= 0x3F;
         blk++;

entry:
         if (!cbp)
            break;

         while(!(cbp & 0x20)) {
            cbp <<= 1;
            blk++;
         }

         vl_vlc_fillbits(&bs->vlc);

         if (intra) {
            unsigned cc = blk2cc[blk];
            unsigned size = vl_vlc_get_vlclbf(&bs->vlc, blk2dcsize[blk], 10);

            if (size) {
               int dct_diff = vl_vlc_get_uimsbf(&bs->vlc, size);
               int half_range = 1 << (size - 1);
               if (dct_diff < half_range)
                  dct_diff = (dct_diff + 1) - (2 * half_range);
               bs->pred_dc[cc] += dct_diff;
            }

            dst[0] = bs->pred_dc[cc];
            i = 0;

         } else {
            entry = tbl_B14_DC + vl_vlc_peekbits(&bs->vlc, 17);
            i = -1;
            continue;
         }

      } else if (entry->run == dct_Escape) {
         i += vl_vlc_get_uimsbf(&bs->vlc, 6) + 1;
         if (i > 64)
            break;

         dst[i] = vl_vlc_get_simsbf(&bs->vlc, 12) * scale;

      } else {
         i += entry->run;
         if (i > 64)
            break;

         dst[i] = entry->level * scale;
      }

      vl_vlc_fillbits(&bs->vlc);
      entry = table + vl_vlc_peekbits(&bs->vlc, 17);
   }
}

static INLINE void
decode_slice(struct vl_mpg12_bs *bs)
{
   struct pipe_mpeg12_macroblock mb;
   short dct_blocks[64*6];
   unsigned dct_scale;
   signed x = -1;

   memset(&mb, 0, sizeof(mb));
   mb.base.codec = PIPE_VIDEO_CODEC_MPEG12;
   mb.y = vl_vlc_get_uimsbf(&bs->vlc, 8) - 1;
   mb.blocks = dct_blocks;

   reset_predictor(bs);
   vl_vlc_fillbits(&bs->vlc);
   dct_scale = quant_scale[bs->desc.q_scale_type][vl_vlc_get_uimsbf(&bs->vlc, 5)];

   if (vl_vlc_get_uimsbf(&bs->vlc, 1))
      while (vl_vlc_get_uimsbf(&bs->vlc, 9) & 1)
         vl_vlc_fillbits(&bs->vlc);

   vl_vlc_fillbits(&bs->vlc);
   assert(vl_vlc_bits_left(&bs->vlc) > 23 && vl_vlc_peekbits(&bs->vlc, 23));
   do {
      int inc = 0;

      if (bs->decoder->profile == PIPE_VIDEO_PROFILE_MPEG1)
         while (vl_vlc_peekbits(&bs->vlc, 11) == 15) {
            vl_vlc_eatbits(&bs->vlc, 11);
            vl_vlc_fillbits(&bs->vlc);
         }

      while (vl_vlc_peekbits(&bs->vlc, 11) == 8) {
         vl_vlc_eatbits(&bs->vlc, 11);
         vl_vlc_fillbits(&bs->vlc);
         inc += 33;
      }
      inc += vl_vlc_get_vlclbf(&bs->vlc, tbl_B1, 11);
      if (x != -1) {
         mb.num_skipped_macroblocks = inc - 1;
         bs->decoder->decode_macroblock(bs->decoder, &mb.base, 1);
      }
      mb.x = x += inc;

      switch (bs->desc.picture_coding_type) {
      case PIPE_MPEG12_PICTURE_CODING_TYPE_I:
         mb.macroblock_type = vl_vlc_get_vlclbf(&bs->vlc, tbl_B2, 2);
         break;

      case PIPE_MPEG12_PICTURE_CODING_TYPE_P:
         mb.macroblock_type = vl_vlc_get_vlclbf(&bs->vlc, tbl_B3, 6);
         break;

      case PIPE_MPEG12_PICTURE_CODING_TYPE_B:
         mb.macroblock_type = vl_vlc_get_vlclbf(&bs->vlc, tbl_B4, 6);
         break;

      default:
         mb.macroblock_type = 0;
         /* dumb gcc */
         assert(0);
      }

      mb.macroblock_modes.value = 0;
      if (mb.macroblock_type & (PIPE_MPEG12_MB_TYPE_MOTION_FORWARD | PIPE_MPEG12_MB_TYPE_MOTION_BACKWARD)) {
         if (bs->desc.picture_structure == PIPE_MPEG12_PICTURE_STRUCTURE_FRAME) {
            if (bs->desc.frame_pred_frame_dct == 0)
               mb.macroblock_modes.bits.frame_motion_type = vl_vlc_get_uimsbf(&bs->vlc, 2);
            else
               mb.macroblock_modes.bits.frame_motion_type = 2;
         } else
            mb.macroblock_modes.bits.field_motion_type = vl_vlc_get_uimsbf(&bs->vlc, 2);

      } else if ((mb.macroblock_type & PIPE_MPEG12_MB_TYPE_INTRA) && bs->desc.concealment_motion_vectors) {
         if (bs->desc.picture_structure == PIPE_MPEG12_PICTURE_STRUCTURE_FRAME)
            mb.macroblock_modes.bits.frame_motion_type = 2;
         else
            mb.macroblock_modes.bits.field_motion_type = 1;
      }

      if (bs->desc.picture_structure == PIPE_MPEG12_PICTURE_STRUCTURE_FRAME &&
          bs->desc.frame_pred_frame_dct == 0 &&
          mb.macroblock_type & (PIPE_MPEG12_MB_TYPE_INTRA | PIPE_MPEG12_MB_TYPE_PATTERN))
         mb.macroblock_modes.bits.dct_type = vl_vlc_get_uimsbf(&bs->vlc, 1);

      if (mb.macroblock_type & PIPE_MPEG12_MB_TYPE_QUANT)
         dct_scale = quant_scale[bs->desc.q_scale_type][vl_vlc_get_uimsbf(&bs->vlc, 5)];

      if (inc > 1 && bs->desc.picture_coding_type == PIPE_MPEG12_PICTURE_CODING_TYPE_P)
         memset(mb.PMV, 0, sizeof(mb.PMV));

      mb.motion_vertical_field_select = 0;
      if ((mb.macroblock_type & PIPE_MPEG12_MB_TYPE_MOTION_FORWARD) ||
          (mb.macroblock_type & PIPE_MPEG12_MB_TYPE_INTRA && bs->desc.concealment_motion_vectors)) {
         if (bs->desc.picture_structure == PIPE_MPEG12_PICTURE_STRUCTURE_FRAME)
            motion_vector_frame(bs, 0, &mb);
         else
            motion_vector_field(bs, 0, &mb);
      }

      if (mb.macroblock_type & PIPE_MPEG12_MB_TYPE_MOTION_BACKWARD) {
         if (bs->desc.picture_structure == PIPE_MPEG12_PICTURE_STRUCTURE_FRAME)
            motion_vector_frame(bs, 1, &mb);
         else
            motion_vector_field(bs, 1, &mb);
      }

      if (mb.macroblock_type & PIPE_MPEG12_MB_TYPE_INTRA && bs->desc.concealment_motion_vectors) {
         unsigned extra = vl_vlc_get_uimsbf(&bs->vlc, 1);
         mb.PMV[1][0][0] = mb.PMV[0][0][0];
         mb.PMV[1][0][1] = mb.PMV[0][0][1];
         assert(extra);
      } else if (mb.macroblock_type & PIPE_MPEG12_MB_TYPE_INTRA ||
                !(mb.macroblock_type & (PIPE_MPEG12_MB_TYPE_MOTION_FORWARD |
                                        PIPE_MPEG12_MB_TYPE_MOTION_BACKWARD))) {
         memset(mb.PMV, 0, sizeof(mb.PMV));
      }

      if ((mb.macroblock_type & PIPE_MPEG12_MB_TYPE_MOTION_FORWARD &&
           mb.macroblock_modes.bits.frame_motion_type == 2) ||
          (mb.macroblock_modes.bits.frame_motion_type == 3)) {
            mb.PMV[1][0][0] = mb.PMV[0][0][0];
            mb.PMV[1][0][1] = mb.PMV[0][0][1];
      }

      if (mb.macroblock_type & PIPE_MPEG12_MB_TYPE_MOTION_BACKWARD &&
          mb.macroblock_modes.bits.frame_motion_type == 2) {
            mb.PMV[1][1][0] = mb.PMV[0][1][0];
            mb.PMV[1][1][1] = mb.PMV[0][1][1];
      }

      if (inc > 1 || !(mb.macroblock_type & PIPE_MPEG12_MB_TYPE_INTRA))
         reset_predictor(bs);

      if (mb.macroblock_type & (PIPE_MPEG12_MB_TYPE_INTRA | PIPE_MPEG12_MB_TYPE_PATTERN)) {
         memset(dct_blocks, 0, sizeof(dct_blocks));
         decode_dct(bs, &mb, dct_scale);
      } else
         mb.coded_block_pattern = 0;

      vl_vlc_fillbits(&bs->vlc);
   } while (vl_vlc_bits_left(&bs->vlc) && vl_vlc_peekbits(&bs->vlc, 23));

   mb.num_skipped_macroblocks = 0;
   bs->decoder->decode_macroblock(bs->decoder, &mb.base, 1);
}

void
vl_mpg12_bs_init(struct vl_mpg12_bs *bs, struct pipe_video_decoder *decoder)
{
   static bool tables_initialized = false;

   assert(bs);

   memset(bs, 0, sizeof(struct vl_mpg12_bs));

   bs->decoder = decoder;

   if (!tables_initialized) {
      init_tables();
      tables_initialized = true;
   }
}

void
vl_mpg12_bs_set_picture_desc(struct vl_mpg12_bs *bs, struct pipe_mpeg12_picture_desc *picture)
{
   bs->desc = *picture;
   bs->intra_dct_tbl = picture->intra_vlc_format ? tbl_B15 : tbl_B14_AC;
}

void
vl_mpg12_bs_decode(struct vl_mpg12_bs *bs, unsigned num_buffers,
                   const void * const *buffers, const unsigned *sizes)
{
   assert(bs);

   vl_vlc_init(&bs->vlc, num_buffers, buffers, sizes);
   while (vl_vlc_bits_left(&bs->vlc) > 32) {
      uint32_t code = vl_vlc_peekbits(&bs->vlc, 32);

      if (code >= 0x101 && code <= 0x1AF) {
         vl_vlc_eatbits(&bs->vlc, 24);
         decode_slice(bs);

         /* align to a byte again */
         vl_vlc_eatbits(&bs->vlc, vl_vlc_valid_bits(&bs->vlc) & 7);

      } else {
         vl_vlc_eatbits(&bs->vlc, 8);
      }

      vl_vlc_fillbits(&bs->vlc);
   }
}
