#ifndef __WIN32K_PALETTE_H
#define __WIN32K_PALETTE_H

#define PALETTE_FIXED    0x0001 /* read-only colormap - have to use XAllocColor (if not virtual) */
#define PALETTE_VIRTUAL  0x0002 /* no mapping needed - pixel == pixel color */

#define PALETTE_PRIVATE  0x1000 /* private colormap, identity mapping */
#define PALETTE_WHITESET 0x2000

typedef struct {
    int shift;
    int scale;
    int max;
} ColorShifts;

static ColorShifts PALETTE_PRed   = {0,0,0};
static ColorShifts PALETTE_LRed   = {0,0,0};
static ColorShifts PALETTE_PGreen = {0,0,0};
static ColorShifts PALETTE_LGreen = {0,0,0};
static ColorShifts PALETTE_PBlue  = {0,0,0};
static ColorShifts PALETTE_LBlue  = {0,0,0};
static int PALETTE_Graymax        = 0;
static int palette_size;

#endif /* __WIN32K_PALETTE_H */
