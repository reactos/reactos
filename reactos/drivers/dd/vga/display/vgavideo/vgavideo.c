#include <ddk/ntddk.h>
#include <ddk/ntddvid.h>

#include "vgavideo.h"

#include <internal/i386/io.h>

div_t div(int num, int denom)
{
   div_t r;
   if (num > 0 && denom < 0) {
      num = -num;
      denom = -denom;
   }
   r.quot = num / denom;
   r.rem = num % denom;
   if (num < 0 && denom > 0)
   {
      if (r.rem > 0)
      {
         r.quot++;
         r.rem -= denom;
      }
   }
   return r;
}

int mod(int num, int denom)
{
   div_t dvt = div(num, denom);
   return dvt.rem;
}

VOID vgaPreCalc()
{
   ULONG j;

   startmasks[1] = 127;
   startmasks[2] = 63;
   startmasks[3] = 31;
   startmasks[4] = 15;
   startmasks[5] = 7;
   startmasks[6] = 3;
   startmasks[7] = 1;
   startmasks[8] = 255;

   endmasks[0] = 128;
   endmasks[1] = 192;
   endmasks[2] = 224;
   endmasks[3] = 240;
   endmasks[4] = 248;
   endmasks[5] = 252;
   endmasks[6] = 254;
   endmasks[7] = 255;
   endmasks[8] = 255;

   for(j=0; j<80; j++)
   {
     maskbit[j*8]   = 128;
     maskbit[j*8+1] = 64;
     maskbit[j*8+2] = 32;
     maskbit[j*8+3] = 16;
     maskbit[j*8+4] = 8;
     maskbit[j*8+5] = 4;
     maskbit[j*8+6] = 2;
     maskbit[j*8+7] = 1;

     bit8[j*8]   = 7;
     bit8[j*8+1] = 6;
     bit8[j*8+2] = 5;
     bit8[j*8+3] = 4;
     bit8[j*8+4] = 3;
     bit8[j*8+5] = 2;
     bit8[j*8+6] = 1;
     bit8[j*8+7] = 0;
   }
   for(j=0; j<480; j++)
   {
     y80[j]  = j*80;
   }
   for(j=0; j<640; j++)
   {
     xconv[j] = j >> 3;
   }
}

void vgaPutPixel(int x, int y, unsigned char c)
{
  unsigned offset;
  unsigned char a;

  offset = xconv[x]+y80[y];

  WRITE_PORT_UCHAR((PUCHAR)0x3ce,0x08);
  WRITE_PORT_UCHAR((PUCHAR)0x3cf,maskbit[x]);

  a = READ_REGISTER_UCHAR(vidmem + offset);
  WRITE_REGISTER_UCHAR(vidmem + offset, c);
}

void vgaPutByte(int x, int y, unsigned char c)
{
  unsigned offset;

  offset = xconv[x]+y80[y];

  // Set the write mode
  WRITE_PORT_UCHAR((PUCHAR)0x3ce,0x08);
  WRITE_PORT_UCHAR((PUCHAR)0x3cf,0xff);

  WRITE_REGISTER_UCHAR(vidmem + offset, c);
}

void vgaGetByte(unsigned offset,
                unsigned char *b, unsigned char *g,
                unsigned char *r, unsigned char *i)
{
// fixme: use READ_REGISTER_UCHAR

  WRITE_PORT_USHORT((PUSHORT)0x03ce, 0x0304);
  *i = vidmem[offset];
  WRITE_PORT_USHORT((PUSHORT)0x03ce, 0x0204);
  *r = vidmem[offset];
  WRITE_PORT_USHORT((PUSHORT)0x03ce, 0x0104);
  *g = vidmem[offset];
  WRITE_PORT_USHORT((PUSHORT)0x03ce, 0x0004);
  *b = vidmem[offset];
}

int vgaGetPixel(int x, int y)
{
  unsigned char mask, b, g, r, i;
  unsigned offset;

  offset = xconv[x]+y80[y];
  mask=maskbit[x];
  vgaGetByte(offset, &b, &g, &r, &i);
  b=b&mask;
  g=g&mask;
  r=r&mask;
  i=i&mask;

  mask=bit8[x];
  g=g>>mask;
  b=b>>mask;
  r=r>>mask;
  i=i>>mask;

  return(b+2*g+4*r+8*i);
}

BOOL vgaHLine(int x, int y, int len, unsigned char c)
{
  unsigned char a;
  unsigned int pre1, i;
  unsigned int orgpre1, orgx, midpre1;
  unsigned long leftpixs, midpixs, rightpixs, temp;

  orgx=x;

  if(len<8)
  {
    for (i=x; i<x+len; i++)
      vgaPutPixel(i, y, c);
  } else {

    leftpixs=x;
    while(leftpixs>8) leftpixs-=8;
    temp = len;
    midpixs = 0;

    while(temp>7)
    {
      temp-=8;
      midpixs++;
    }
    if((temp>=0) && (midpixs>0)) midpixs--;

    pre1=xconv[x]+y80[y];
    orgpre1=pre1;

    // Left
    if(leftpixs==8) {
      // Left edge should be an entire middle bar
      x=orgx;
      leftpixs=0;
    }
    else if(leftpixs>0)
    {
      // Write left pixels
      WRITE_PORT_UCHAR((PUCHAR)0x3ce,0x08);     // set the mask
      WRITE_PORT_UCHAR((PUCHAR)0x3cf,startmasks[leftpixs]);

      a = READ_REGISTER_UCHAR(vidmem + pre1);
      WRITE_REGISTER_UCHAR(vidmem + pre1, c);

      // Middle
      x=orgx+(8-leftpixs)+leftpixs;

    } else {
      // leftpixs == 0
      midpixs+=1;
    }

    if(midpixs>0)
    {
      midpre1=xconv[x]+y80[y];

      // Set mask to all pixels in byte
      WRITE_PORT_UCHAR((PUCHAR)0x3ce, 0x08);
      WRITE_PORT_UCHAR((PUCHAR)0x3cf, 0xff);
      memset(vidmem+midpre1, c, midpixs); // write middle pixels, no need to read in latch because of the width
    }

    rightpixs = len - ((midpixs*8) + leftpixs);

    if((rightpixs>0))
    {
      x=(orgx+len)-rightpixs;

      // Go backwards till we reach the 8-byte boundary
      while(mod(x, 8)!=0) { x--; rightpixs++; }

      while(rightpixs>7)
      {
        // This is a BAD case as this should have been a midpixs

        vgaPutByte(x, y, c);
        rightpixs-=8;
        x+=8;
      }

      pre1=xconv[x]+y80[y];

      // Write right pixels
      WRITE_PORT_UCHAR((PUCHAR)0x3ce,0x08);     // set the mask bits
      WRITE_PORT_UCHAR((PUCHAR)0x3cf,endmasks[rightpixs]);

      a = READ_REGISTER_UCHAR(vidmem + pre1);
      WRITE_REGISTER_UCHAR(vidmem + pre1, c);
    }
  }

  return TRUE;
}

BOOL vgaVLine(int x, int y, int len, unsigned char c)
{
  unsigned offset, i;
  unsigned char a;

  offset = xconv[x]+y80[y];

  WRITE_PORT_UCHAR((PUCHAR)0x3ce,0x08);       // set the mask
  WRITE_PORT_UCHAR((PUCHAR)0x3cf,maskbit[x]);

  len++;

  for(i=y; i<y+len; i++)
  {
    a = READ_REGISTER_UCHAR(vidmem + offset);
    WRITE_REGISTER_UCHAR(vidmem + offset, c);
    offset+=80;
  }

  return TRUE;
}
