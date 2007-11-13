#ifndef INTEL_ROTATE_H
#define INTEL_ROTATE_H 1

struct matrix23
{
   int m00, m01, m02;
   int m10, m11, m12;
};



extern void
matrix23Set(struct matrix23 *m,
            int m00, int m01, int m02, int m10, int m11, int m12);

extern void matrix23TransformCoordi(const struct matrix23 *m, int *x, int *y);

extern void
matrix23TransformCoordf(const struct matrix23 *m, float *x, float *y);

extern void
matrix23TransformDistance(const struct matrix23 *m, int *xDist, int *yDist);

extern void
matrix23TransformRect(const struct matrix23 *m,
                      int *x, int *y, int *w, int *h);

extern void
matrix23Rotate(struct matrix23 *m, int width, int height, int angle);

extern void
matrix23Flip(struct matrix23 *m, int width, int height, int xflip, int yflip);

extern void
matrix23Multiply(struct matrix23 *result,
                 const struct matrix23 *a, const struct matrix23 *b);


#endif /* INTEL_ROTATE_H */
