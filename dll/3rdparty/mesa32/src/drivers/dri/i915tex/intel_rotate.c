
/**
 * Routines for simple 2D->2D transformations for rotated, flipped screens.
 *
 * XXX This code is not intel-specific.  Move it into a common/utility
 * someday.
 */

#include "intel_rotate.h"

#define MIN2(A, B)   ( ((A) < (B)) ? (A) : (B) )

#define ABS(A)  ( ((A) < 0) ? -(A) : (A) )


void
matrix23Set(struct matrix23 *m,
            int m00, int m01, int m02, int m10, int m11, int m12)
{
   m->m00 = m00;
   m->m01 = m01;
   m->m02 = m02;
   m->m10 = m10;
   m->m11 = m11;
   m->m12 = m12;
}


/*
 * Transform (x,y) coordinate by the given matrix.
 */
void
matrix23TransformCoordf(const struct matrix23 *m, float *x, float *y)
{
   const float x0 = *x;
   const float y0 = *y;

   *x = m->m00 * x0 + m->m01 * y0 + m->m02;
   *y = m->m10 * x0 + m->m11 * y0 + m->m12;
}


void
matrix23TransformCoordi(const struct matrix23 *m, int *x, int *y)
{
   const int x0 = *x;
   const int y0 = *y;

   *x = m->m00 * x0 + m->m01 * y0 + m->m02;
   *y = m->m10 * x0 + m->m11 * y0 + m->m12;
}


/*
 * Transform a width and height by the given matrix.
 * XXX this could be optimized quite a bit.
 */
void
matrix23TransformDistance(const struct matrix23 *m, int *xDist, int *yDist)
{
   int x0 = 0, y0 = 0;
   int x1 = *xDist, y1 = 0;
   int x2 = 0, y2 = *yDist;
   matrix23TransformCoordi(m, &x0, &y0);
   matrix23TransformCoordi(m, &x1, &y1);
   matrix23TransformCoordi(m, &x2, &y2);

   *xDist = (x1 - x0) + (x2 - x0);
   *yDist = (y1 - y0) + (y2 - y0);

   if (*xDist < 0)
      *xDist = -*xDist;
   if (*yDist < 0)
      *yDist = -*yDist;
}


/**
 * Transform the rect defined by (x, y, w, h) by m.
 */
void
matrix23TransformRect(const struct matrix23 *m, int *x, int *y, int *w,
                      int *h)
{
   int x0 = *x, y0 = *y;
   int x1 = *x + *w, y1 = *y;
   int x2 = *x + *w, y2 = *y + *h;
   int x3 = *x, y3 = *y + *h;
   matrix23TransformCoordi(m, &x0, &y0);
   matrix23TransformCoordi(m, &x1, &y1);
   matrix23TransformCoordi(m, &x2, &y2);
   matrix23TransformCoordi(m, &x3, &y3);
   *w = ABS(x1 - x0) + ABS(x2 - x1);
   /**w = ABS(*w);*/
   *h = ABS(y1 - y0) + ABS(y2 - y1);
   /**h = ABS(*h);*/
   *x = MIN2(x0, x1);
   *x = MIN2(*x, x2);
   *y = MIN2(y0, y1);
   *y = MIN2(*y, y2);
}


/*
 * Make rotation matrix for width X height screen.
 */
void
matrix23Rotate(struct matrix23 *m, int width, int height, int angle)
{
   switch (angle) {
   case 0:
      matrix23Set(m, 1, 0, 0, 0, 1, 0);
      break;
   case 90:
      matrix23Set(m, 0, 1, 0, -1, 0, width);
      break;
   case 180:
      matrix23Set(m, -1, 0, width, 0, -1, height);
      break;
   case 270:
      matrix23Set(m, 0, -1, height, 1, 0, 0);
      break;
   default:
      /*abort() */ ;
   }
}


/*
 * Make flip/reflection matrix for width X height screen.
 */
void
matrix23Flip(struct matrix23 *m, int width, int height, int xflip, int yflip)
{
   if (xflip) {
      m->m00 = -1;
      m->m01 = 0;
      m->m02 = width - 1;
   }
   else {
      m->m00 = 1;
      m->m01 = 0;
      m->m02 = 0;
   }
   if (yflip) {
      m->m10 = 0;
      m->m11 = -1;
      m->m12 = height - 1;
   }
   else {
      m->m10 = 0;
      m->m11 = 1;
      m->m12 = 0;
   }
}


/*
 * result = a * b
 */
void
matrix23Multiply(struct matrix23 *result,
                 const struct matrix23 *a, const struct matrix23 *b)
{
   result->m00 = a->m00 * b->m00 + a->m01 * b->m10;
   result->m01 = a->m00 * b->m01 + a->m01 * b->m11;
   result->m02 = a->m00 * b->m02 + a->m01 * b->m12 + a->m02;

   result->m10 = a->m10 * b->m00 + a->m11 * b->m10;
   result->m11 = a->m10 * b->m01 + a->m11 * b->m11;
   result->m12 = a->m10 * b->m02 + a->m11 * b->m12 + a->m12;
}


#if 000

#include <stdio.h>

int
main(int argc, char *argv[])
{
   int width = 500, height = 400;
   int rot;
   int fx = 0, fy = 0;          /* flip x and/or y ? */
   int coords[4][2];

   /* four corner coords to test with */
   coords[0][0] = 0;
   coords[0][1] = 0;
   coords[1][0] = width - 1;
   coords[1][1] = 0;
   coords[2][0] = width - 1;
   coords[2][1] = height - 1;
   coords[3][0] = 0;
   coords[3][1] = height - 1;


   for (rot = 0; rot < 360; rot += 90) {
      struct matrix23 rotate, flip, m;
      int i;

      printf("Rot %d, xFlip %d, yFlip %d:\n", rot, fx, fy);

      /* make transformation matrix 'm' */
      matrix23Rotate(&rotate, width, height, rot);
      matrix23Flip(&flip, width, height, fx, fy);
      matrix23Multiply(&m, &rotate, &flip);

      /* xform four coords */
      for (i = 0; i < 4; i++) {
         int x = coords[i][0];
         int y = coords[i][1];
         matrix23TransformCoordi(&m, &x, &y);
         printf("  %d, %d  -> %d %d\n", coords[i][0], coords[i][1], x, y);
      }

      /* xform width, height */
      {
         int x = width;
         int y = height;
         matrix23TransformDistance(&m, &x, &y);
         printf("  %d x %d -> %d x %d\n", width, height, x, y);
      }

      /* xform rect */
      {
         int x = 50, y = 10, w = 200, h = 100;
         matrix23TransformRect(&m, &x, &y, &w, &h);
         printf("  %d,%d %d x %d -> %d, %d %d x %d\n", 50, 10, 200, 100,
                x, y, w, h);
      }

   }

   return 0;
}
#endif
