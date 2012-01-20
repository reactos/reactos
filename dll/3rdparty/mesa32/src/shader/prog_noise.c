/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 2006  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * SimplexNoise1234
 * Copyright (c) 2003-2005, Stefan Gustavson
 *
 * Contact: stegu@itn.liu.se
 */

/**
 * \file
 * \brief C implementation of Perlin Simplex Noise over 1, 2, 3 and 4 dims.
 * \author Stefan Gustavson (stegu@itn.liu.se)
 *
 *
 * This implementation is "Simplex Noise" as presented by
 * Ken Perlin at a relatively obscure and not often cited course
 * session "Real-Time Shading" at Siggraph 2001 (before real
 * time shading actually took on), under the title "hardware noise".
 * The 3D function is numerically equivalent to his Java reference
 * code available in the PDF course notes, although I re-implemented
 * it from scratch to get more readable code. The 1D, 2D and 4D cases
 * were implemented from scratch by me from Ken Perlin's text.
 *
 * This file has no dependencies on any other file, not even its own
 * header file. The header file is made for use by external code only.
 */


#include "main/imports.h"
#include "prog_noise.h"

#define FASTFLOOR(x) ( ((x)>0) ? ((int)x) : (((int)x)-1) )

/*
 * ---------------------------------------------------------------------
 * Static data
 */

/**
 * Permutation table. This is just a random jumble of all numbers 0-255,
 * repeated twice to avoid wrapping the index at 255 for each lookup.
 * This needs to be exactly the same for all instances on all platforms,
 * so it's easiest to just keep it as static explicit data.
 * This also removes the need for any initialisation of this class.
 *
 * Note that making this an int[] instead of a char[] might make the
 * code run faster on platforms with a high penalty for unaligned single
 * byte addressing. Intel x86 is generally single-byte-friendly, but
 * some other CPUs are faster with 4-aligned reads.
 * However, a char[] is smaller, which avoids cache trashing, and that
 * is probably the most important aspect on most architectures.
 * This array is accessed a *lot* by the noise functions.
 * A vector-valued noise over 3D accesses it 96 times, and a
 * float-valued 4D noise 64 times. We want this to fit in the cache!
 */
unsigned char perm[512] = { 151, 160, 137, 91, 90, 15,
   131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8,
      99, 37, 240, 21, 10, 23,
   190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35,
      11, 32, 57, 177, 33,
   88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71,
      134, 139, 48, 27, 166,
   77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41,
      55, 46, 245, 40, 244,
   102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89,
      18, 169, 200, 196,
   135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217,
      226, 250, 124, 123,
   5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58,
      17, 182, 189, 28, 42,
   223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155,
      167, 43, 172, 9,
   129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104,
      218, 246, 97, 228,
   251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235,
      249, 14, 239, 107,
   49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45,
      127, 4, 150, 254,
   138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66,
      215, 61, 156, 180,
   151, 160, 137, 91, 90, 15,
   131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8,
      99, 37, 240, 21, 10, 23,
   190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35,
      11, 32, 57, 177, 33,
   88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71,
      134, 139, 48, 27, 166,
   77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41,
      55, 46, 245, 40, 244,
   102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89,
      18, 169, 200, 196,
   135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217,
      226, 250, 124, 123,
   5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58,
      17, 182, 189, 28, 42,
   223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155,
      167, 43, 172, 9,
   129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104,
      218, 246, 97, 228,
   251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235,
      249, 14, 239, 107,
   49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45,
      127, 4, 150, 254,
   138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66,
      215, 61, 156, 180
};

/*
 * ---------------------------------------------------------------------
 */

/*
 * Helper functions to compute gradients-dot-residualvectors (1D to 4D)
 * Note that these generate gradients of more than unit length. To make
 * a close match with the value range of classic Perlin noise, the final
 * noise values need to be rescaled to fit nicely within [-1,1].
 * (The simplex noise functions as such also have different scaling.)
 * Note also that these noise functions are the most practical and useful
 * signed version of Perlin noise. To return values according to the
 * RenderMan specification from the SL noise() and pnoise() functions,
 * the noise values need to be scaled and offset to [0,1], like this:
 * float SLnoise = (SimplexNoise1234::noise(x,y,z) + 1.0) * 0.5;
 */

static float
grad1(int hash, float x)
{
   int h = hash & 15;
   float grad = 1.0f + (h & 7); /* Gradient value 1.0, 2.0, ..., 8.0 */
   if (h & 8)
      grad = -grad;             /* Set a random sign for the gradient */
   return (grad * x);           /* Multiply the gradient with the distance */
}

static float
grad2(int hash, float x, float y)
{
   int h = hash & 7;            /* Convert low 3 bits of hash code */
   float u = h < 4 ? x : y;     /* into 8 simple gradient directions, */
   float v = h < 4 ? y : x;     /* and compute the dot product with (x,y). */
   return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
}

static float
grad3(int hash, float x, float y, float z)
{
   int h = hash & 15;           /* Convert low 4 bits of hash code into 12 simple */
   float u = h < 8 ? x : y;     /* gradient directions, and compute dot product. */
   float v = h < 4 ? y : h == 12 || h == 14 ? x : z;    /* Fix repeats at h = 12 to 15 */
   return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

static float
grad4(int hash, float x, float y, float z, float t)
{
   int h = hash & 31;           /* Convert low 5 bits of hash code into 32 simple */
   float u = h < 24 ? x : y;    /* gradient directions, and compute dot product. */
   float v = h < 16 ? y : z;
   float w = h < 8 ? z : t;
   return ((h & 1) ? -u : u) + ((h & 2) ? -v : v) + ((h & 4) ? -w : w);
}

/**
 * A lookup table to traverse the simplex around a given point in 4D.
 * Details can be found where this table is used, in the 4D noise method.
 * TODO: This should not be required, backport it from Bill's GLSL code!
 */
static unsigned char simplex[64][4] = {
   {0, 1, 2, 3}, {0, 1, 3, 2}, {0, 0, 0, 0}, {0, 2, 3, 1},
   {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {1, 2, 3, 0},
   {0, 2, 1, 3}, {0, 0, 0, 0}, {0, 3, 1, 2}, {0, 3, 2, 1},
   {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {1, 3, 2, 0},
   {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
   {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
   {1, 2, 0, 3}, {0, 0, 0, 0}, {1, 3, 0, 2}, {0, 0, 0, 0},
   {0, 0, 0, 0}, {0, 0, 0, 0}, {2, 3, 0, 1}, {2, 3, 1, 0},
   {1, 0, 2, 3}, {1, 0, 3, 2}, {0, 0, 0, 0}, {0, 0, 0, 0},
   {0, 0, 0, 0}, {2, 0, 3, 1}, {0, 0, 0, 0}, {2, 1, 3, 0},
   {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
   {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
   {2, 0, 1, 3}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
   {3, 0, 1, 2}, {3, 0, 2, 1}, {0, 0, 0, 0}, {3, 1, 2, 0},
   {2, 1, 0, 3}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
   {3, 1, 0, 2}, {0, 0, 0, 0}, {3, 2, 0, 1}, {3, 2, 1, 0}
};


/** 1D simplex noise */
GLfloat
_mesa_noise1(GLfloat x)
{
   int i0 = FASTFLOOR(x);
   int i1 = i0 + 1;
   float x0 = x - i0;
   float x1 = x0 - 1.0f;
   float t1 = 1.0f - x1 * x1;
   float n0, n1;

   float t0 = 1.0f - x0 * x0;
/*  if(t0 < 0.0f) t0 = 0.0f; // this never happens for the 1D case */
   t0 *= t0;
   n0 = t0 * t0 * grad1(perm[i0 & 0xff], x0);

/*  if(t1 < 0.0f) t1 = 0.0f; // this never happens for the 1D case */
   t1 *= t1;
   n1 = t1 * t1 * grad1(perm[i1 & 0xff], x1);
   /* The maximum value of this noise is 8*(3/4)^4 = 2.53125 */
   /* A factor of 0.395 would scale to fit exactly within [-1,1], but */
   /* we want to match PRMan's 1D noise, so we scale it down some more. */
   return 0.25f * (n0 + n1);
}


/** 2D simplex noise */
GLfloat
_mesa_noise2(GLfloat x, GLfloat y)
{
#define F2 0.366025403f         /* F2 = 0.5*(sqrt(3.0)-1.0) */
#define G2 0.211324865f         /* G2 = (3.0-Math.sqrt(3.0))/6.0 */

   float n0, n1, n2;            /* Noise contributions from the three corners */

   /* Skew the input space to determine which simplex cell we're in */
   float s = (x + y) * F2;      /* Hairy factor for 2D */
   float xs = x + s;
   float ys = y + s;
   int i = FASTFLOOR(xs);
   int j = FASTFLOOR(ys);

   float t = (float) (i + j) * G2;
   float X0 = i - t;            /* Unskew the cell origin back to (x,y) space */
   float Y0 = j - t;
   float x0 = x - X0;           /* The x,y distances from the cell origin */
   float y0 = y - Y0;

   float x1, y1, x2, y2;
   int ii, jj;
   float t0, t1, t2;

   /* For the 2D case, the simplex shape is an equilateral triangle. */
   /* Determine which simplex we are in. */
   int i1, j1;                  /* Offsets for second (middle) corner of simplex in (i,j) coords */
   if (x0 > y0) {
      i1 = 1;
      j1 = 0;
   }                            /* lower triangle, XY order: (0,0)->(1,0)->(1,1) */
   else {
      i1 = 0;
      j1 = 1;
   }                            /* upper triangle, YX order: (0,0)->(0,1)->(1,1) */

   /* A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and */
   /* a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where */
   /* c = (3-sqrt(3))/6 */

   x1 = x0 - i1 + G2;           /* Offsets for middle corner in (x,y) unskewed coords */
   y1 = y0 - j1 + G2;
   x2 = x0 - 1.0f + 2.0f * G2;  /* Offsets for last corner in (x,y) unskewed coords */
   y2 = y0 - 1.0f + 2.0f * G2;

   /* Wrap the integer indices at 256, to avoid indexing perm[] out of bounds */
   ii = i % 256;
   jj = j % 256;

   /* Calculate the contribution from the three corners */
   t0 = 0.5f - x0 * x0 - y0 * y0;
   if (t0 < 0.0f)
      n0 = 0.0f;
   else {
      t0 *= t0;
      n0 = t0 * t0 * grad2(perm[ii + perm[jj]], x0, y0);
   }

   t1 = 0.5f - x1 * x1 - y1 * y1;
   if (t1 < 0.0f)
      n1 = 0.0f;
   else {
      t1 *= t1;
      n1 = t1 * t1 * grad2(perm[ii + i1 + perm[jj + j1]], x1, y1);
   }

   t2 = 0.5f - x2 * x2 - y2 * y2;
   if (t2 < 0.0f)
      n2 = 0.0f;
   else {
      t2 *= t2;
      n2 = t2 * t2 * grad2(perm[ii + 1 + perm[jj + 1]], x2, y2);
   }

   /* Add contributions from each corner to get the final noise value. */
   /* The result is scaled to return values in the interval [-1,1]. */
   return 40.0f * (n0 + n1 + n2);       /* TODO: The scale factor is preliminary! */
}


/** 3D simplex noise */
GLfloat
_mesa_noise3(GLfloat x, GLfloat y, GLfloat z)
{
/* Simple skewing factors for the 3D case */
#define F3 0.333333333f
#define G3 0.166666667f

   float n0, n1, n2, n3;        /* Noise contributions from the four corners */

   /* Skew the input space to determine which simplex cell we're in */
   float s = (x + y + z) * F3;  /* Very nice and simple skew factor for 3D */
   float xs = x + s;
   float ys = y + s;
   float zs = z + s;
   int i = FASTFLOOR(xs);
   int j = FASTFLOOR(ys);
   int k = FASTFLOOR(zs);

   float t = (float) (i + j + k) * G3;
   float X0 = i - t;            /* Unskew the cell origin back to (x,y,z) space */
   float Y0 = j - t;
   float Z0 = k - t;
   float x0 = x - X0;           /* The x,y,z distances from the cell origin */
   float y0 = y - Y0;
   float z0 = z - Z0;

   float x1, y1, z1, x2, y2, z2, x3, y3, z3;
   int ii, jj, kk;
   float t0, t1, t2, t3;

   /* For the 3D case, the simplex shape is a slightly irregular tetrahedron. */
   /* Determine which simplex we are in. */
   int i1, j1, k1;              /* Offsets for second corner of simplex in (i,j,k) coords */
   int i2, j2, k2;              /* Offsets for third corner of simplex in (i,j,k) coords */

/* This code would benefit from a backport from the GLSL version! */
   if (x0 >= y0) {
      if (y0 >= z0) {
         i1 = 1;
         j1 = 0;
         k1 = 0;
         i2 = 1;
         j2 = 1;
         k2 = 0;
      }                         /* X Y Z order */
      else if (x0 >= z0) {
         i1 = 1;
         j1 = 0;
         k1 = 0;
         i2 = 1;
         j2 = 0;
         k2 = 1;
      }                         /* X Z Y order */
      else {
         i1 = 0;
         j1 = 0;
         k1 = 1;
         i2 = 1;
         j2 = 0;
         k2 = 1;
      }                         /* Z X Y order */
   }
   else {                       /* x0<y0 */
      if (y0 < z0) {
         i1 = 0;
         j1 = 0;
         k1 = 1;
         i2 = 0;
         j2 = 1;
         k2 = 1;
      }                         /* Z Y X order */
      else if (x0 < z0) {
         i1 = 0;
         j1 = 1;
         k1 = 0;
         i2 = 0;
         j2 = 1;
         k2 = 1;
      }                         /* Y Z X order */
      else {
         i1 = 0;
         j1 = 1;
         k1 = 0;
         i2 = 1;
         j2 = 1;
         k2 = 0;
      }                         /* Y X Z order */
   }

   /* A step of (1,0,0) in (i,j,k) means a step of (1-c,-c,-c) in
    * (x,y,z), a step of (0,1,0) in (i,j,k) means a step of
    * (-c,1-c,-c) in (x,y,z), and a step of (0,0,1) in (i,j,k) means a
    * step of (-c,-c,1-c) in (x,y,z), where c = 1/6.
    */

   x1 = x0 - i1 + G3;         /* Offsets for second corner in (x,y,z) coords */
   y1 = y0 - j1 + G3;
   z1 = z0 - k1 + G3;
   x2 = x0 - i2 + 2.0f * G3;  /* Offsets for third corner in (x,y,z) coords */
   y2 = y0 - j2 + 2.0f * G3;
   z2 = z0 - k2 + 2.0f * G3;
   x3 = x0 - 1.0f + 3.0f * G3;/* Offsets for last corner in (x,y,z) coords */
   y3 = y0 - 1.0f + 3.0f * G3;
   z3 = z0 - 1.0f + 3.0f * G3;

   /* Wrap the integer indices at 256 to avoid indexing perm[] out of bounds */
   ii = i % 256;
   jj = j % 256;
   kk = k % 256;

   /* Calculate the contribution from the four corners */
   t0 = 0.6f - x0 * x0 - y0 * y0 - z0 * z0;
   if (t0 < 0.0f)
      n0 = 0.0f;
   else {
      t0 *= t0;
      n0 = t0 * t0 * grad3(perm[ii + perm[jj + perm[kk]]], x0, y0, z0);
   }

   t1 = 0.6f - x1 * x1 - y1 * y1 - z1 * z1;
   if (t1 < 0.0f)
      n1 = 0.0f;
   else {
      t1 *= t1;
      n1 =
         t1 * t1 * grad3(perm[ii + i1 + perm[jj + j1 + perm[kk + k1]]], x1,
                         y1, z1);
   }

   t2 = 0.6f - x2 * x2 - y2 * y2 - z2 * z2;
   if (t2 < 0.0f)
      n2 = 0.0f;
   else {
      t2 *= t2;
      n2 =
         t2 * t2 * grad3(perm[ii + i2 + perm[jj + j2 + perm[kk + k2]]], x2,
                         y2, z2);
   }

   t3 = 0.6f - x3 * x3 - y3 * y3 - z3 * z3;
   if (t3 < 0.0f)
      n3 = 0.0f;
   else {
      t3 *= t3;
      n3 =
         t3 * t3 * grad3(perm[ii + 1 + perm[jj + 1 + perm[kk + 1]]], x3, y3,
                         z3);
   }

   /* Add contributions from each corner to get the final noise value.
    * The result is scaled to stay just inside [-1,1]
    */
   return 32.0f * (n0 + n1 + n2 + n3);  /* TODO: The scale factor is preliminary! */
}


/** 4D simplex noise */
GLfloat
_mesa_noise4(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   /* The skewing and unskewing factors are hairy again for the 4D case */
#define F4 0.309016994f         /* F4 = (Math.sqrt(5.0)-1.0)/4.0 */
#define G4 0.138196601f         /* G4 = (5.0-Math.sqrt(5.0))/20.0 */

   float n0, n1, n2, n3, n4;    /* Noise contributions from the five corners */

   /* Skew the (x,y,z,w) space to determine which cell of 24 simplices we're in */
   float s = (x + y + z + w) * F4;      /* Factor for 4D skewing */
   float xs = x + s;
   float ys = y + s;
   float zs = z + s;
   float ws = w + s;
   int i = FASTFLOOR(xs);
   int j = FASTFLOOR(ys);
   int k = FASTFLOOR(zs);
   int l = FASTFLOOR(ws);

   float t = (i + j + k + l) * G4;      /* Factor for 4D unskewing */
   float X0 = i - t;            /* Unskew the cell origin back to (x,y,z,w) space */
   float Y0 = j - t;
   float Z0 = k - t;
   float W0 = l - t;

   float x0 = x - X0;           /* The x,y,z,w distances from the cell origin */
   float y0 = y - Y0;
   float z0 = z - Z0;
   float w0 = w - W0;

   /* For the 4D case, the simplex is a 4D shape I won't even try to describe.
    * To find out which of the 24 possible simplices we're in, we need to
    * determine the magnitude ordering of x0, y0, z0 and w0.
    * The method below is a good way of finding the ordering of x,y,z,w and
    * then find the correct traversal order for the simplex we're in.
    * First, six pair-wise comparisons are performed between each possible pair
    * of the four coordinates, and the results are used to add up binary bits
    * for an integer index.
    */
   int c1 = (x0 > y0) ? 32 : 0;
   int c2 = (x0 > z0) ? 16 : 0;
   int c3 = (y0 > z0) ? 8 : 0;
   int c4 = (x0 > w0) ? 4 : 0;
   int c5 = (y0 > w0) ? 2 : 0;
   int c6 = (z0 > w0) ? 1 : 0;
   int c = c1 + c2 + c3 + c4 + c5 + c6;

   int i1, j1, k1, l1;  /* The integer offsets for the second simplex corner */
   int i2, j2, k2, l2;  /* The integer offsets for the third simplex corner */
   int i3, j3, k3, l3;  /* The integer offsets for the fourth simplex corner */

   float x1, y1, z1, w1, x2, y2, z2, w2, x3, y3, z3, w3, x4, y4, z4, w4;
   int ii, jj, kk, ll;
   float t0, t1, t2, t3, t4;

   /*
    * simplex[c] is a 4-vector with the numbers 0, 1, 2 and 3 in some
    * order.  Many values of c will never occur, since e.g. x>y>z>w
    * makes x<z, y<w and x<w impossible. Only the 24 indices which
    * have non-zero entries make any sense.  We use a thresholding to
    * set the coordinates in turn from the largest magnitude.  The
    * number 3 in the "simplex" array is at the position of the
    * largest coordinate.
    */
   i1 = simplex[c][0] >= 3 ? 1 : 0;
   j1 = simplex[c][1] >= 3 ? 1 : 0;
   k1 = simplex[c][2] >= 3 ? 1 : 0;
   l1 = simplex[c][3] >= 3 ? 1 : 0;
   /* The number 2 in the "simplex" array is at the second largest coordinate. */
   i2 = simplex[c][0] >= 2 ? 1 : 0;
   j2 = simplex[c][1] >= 2 ? 1 : 0;
   k2 = simplex[c][2] >= 2 ? 1 : 0;
   l2 = simplex[c][3] >= 2 ? 1 : 0;
   /* The number 1 in the "simplex" array is at the second smallest coordinate. */
   i3 = simplex[c][0] >= 1 ? 1 : 0;
   j3 = simplex[c][1] >= 1 ? 1 : 0;
   k3 = simplex[c][2] >= 1 ? 1 : 0;
   l3 = simplex[c][3] >= 1 ? 1 : 0;
   /* The fifth corner has all coordinate offsets = 1, so no need to look that up. */

   x1 = x0 - i1 + G4;           /* Offsets for second corner in (x,y,z,w) coords */
   y1 = y0 - j1 + G4;
   z1 = z0 - k1 + G4;
   w1 = w0 - l1 + G4;
   x2 = x0 - i2 + 2.0f * G4;    /* Offsets for third corner in (x,y,z,w) coords */
   y2 = y0 - j2 + 2.0f * G4;
   z2 = z0 - k2 + 2.0f * G4;
   w2 = w0 - l2 + 2.0f * G4;
   x3 = x0 - i3 + 3.0f * G4;    /* Offsets for fourth corner in (x,y,z,w) coords */
   y3 = y0 - j3 + 3.0f * G4;
   z3 = z0 - k3 + 3.0f * G4;
   w3 = w0 - l3 + 3.0f * G4;
   x4 = x0 - 1.0f + 4.0f * G4;  /* Offsets for last corner in (x,y,z,w) coords */
   y4 = y0 - 1.0f + 4.0f * G4;
   z4 = z0 - 1.0f + 4.0f * G4;
   w4 = w0 - 1.0f + 4.0f * G4;

   /* Wrap the integer indices at 256, to avoid indexing perm[] out of bounds */
   ii = i % 256;
   jj = j % 256;
   kk = k % 256;
   ll = l % 256;

   /* Calculate the contribution from the five corners */
   t0 = 0.6f - x0 * x0 - y0 * y0 - z0 * z0 - w0 * w0;
   if (t0 < 0.0f)
      n0 = 0.0f;
   else {
      t0 *= t0;
      n0 =
         t0 * t0 * grad4(perm[ii + perm[jj + perm[kk + perm[ll]]]], x0, y0,
                         z0, w0);
   }

   t1 = 0.6f - x1 * x1 - y1 * y1 - z1 * z1 - w1 * w1;
   if (t1 < 0.0f)
      n1 = 0.0f;
   else {
      t1 *= t1;
      n1 =
         t1 * t1 *
         grad4(perm[ii + i1 + perm[jj + j1 + perm[kk + k1 + perm[ll + l1]]]],
               x1, y1, z1, w1);
   }

   t2 = 0.6f - x2 * x2 - y2 * y2 - z2 * z2 - w2 * w2;
   if (t2 < 0.0f)
      n2 = 0.0f;
   else {
      t2 *= t2;
      n2 =
         t2 * t2 *
         grad4(perm[ii + i2 + perm[jj + j2 + perm[kk + k2 + perm[ll + l2]]]],
               x2, y2, z2, w2);
   }

   t3 = 0.6f - x3 * x3 - y3 * y3 - z3 * z3 - w3 * w3;
   if (t3 < 0.0f)
      n3 = 0.0f;
   else {
      t3 *= t3;
      n3 =
         t3 * t3 *
         grad4(perm[ii + i3 + perm[jj + j3 + perm[kk + k3 + perm[ll + l3]]]],
               x3, y3, z3, w3);
   }

   t4 = 0.6f - x4 * x4 - y4 * y4 - z4 * z4 - w4 * w4;
   if (t4 < 0.0f)
      n4 = 0.0f;
   else {
      t4 *= t4;
      n4 =
         t4 * t4 *
         grad4(perm[ii + 1 + perm[jj + 1 + perm[kk + 1 + perm[ll + 1]]]], x4,
               y4, z4, w4);
   }

   /* Sum up and scale the result to cover the range [-1,1] */
   return 27.0f * (n0 + n1 + n2 + n3 + n4);     /* TODO: The scale factor is preliminary! */
}
