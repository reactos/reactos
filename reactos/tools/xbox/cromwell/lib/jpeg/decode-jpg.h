/*
 *    linux/drivers/video/bootsplash/decode-jpg.h - a tiny jpeg decoder.
 *
 *      (w) August 2001 by Michael Schroeder, <mls@suse.de>
 */

#ifndef __DECODE_JPG_H
#define __DECODE_JPG_H

#define ERR_NO_SOI 1
#define ERR_NOT_8BIT 2
#define ERR_HEIGHT_MISMATCH 3
#define ERR_WIDTH_MISMATCH 4
#define ERR_BAD_WIDTH_OR_HEIGHT 5
#define ERR_TOO_MANY_COMPPS 6
#define ERR_ILLEGAL_HV 7
#define ERR_QUANT_TABLE_SELECTOR 8
#define ERR_NOT_YCBCR_221111 9
#define ERR_UNKNOWN_CID_IN_SCAN 10
#define ERR_NOT_SEQUENTIAL_DCT 11
#define ERR_WRONG_MARKER 12
#define ERR_NO_EOI 13
#define ERR_BAD_TABLES 14
#define ERR_DEPTH_MISMATCH 15

struct jpeg_decdata {
	int dcts[6 * 64 + 16];
	int out[64 * 6];
	int dquant[3][64];
};

extern int jpeg_decode(unsigned char *, unsigned char *, int, int, int, struct jpeg_decdata *);
extern int jpeg_get_size(unsigned char *, int *, int *, int *);

#endif
