#include "vgavideo.h"

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

void vgaSetWriteMode(char mode)
{
  VideoPortWritePortUchar((PUCHAR)0x03ce, 0x03);
  VideoPortWritePortUchar((PUCHAR)0x03cf, mode);
}

void vgaSetColor(int cindex, int red, int green, int blue)
{
  VideoPortWritePortUchar((PUCHAR)0x03c8, cindex);
  VideoPortWritePortUchar((PUCHAR)0x03c9, red);
  VideoPortWritePortUchar((PUCHAR)0x03c9, green);
  VideoPortWritePortUchar((PUCHAR)0x03c9, blue);
}
