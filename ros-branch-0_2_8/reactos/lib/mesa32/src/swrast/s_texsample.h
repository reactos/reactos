



/*
 * These values are used in the fixed-point arithmetic used
 * for linear filtering.
 */
#define WEIGHT_SCALE 65536.0F
#define WEIGHT_SHIFT 16


/*
 * Compute the remainder of a divided by b, but be careful with
 * negative values so that GL_REPEAT mode works right.
 */
static INLINE GLint
repeat_remainder(GLint a, GLint b)
{
   if (a >= 0)
      return a % b;
   else
      return (a + 1) % b + b - 1;
}


/*
 * Used to compute texel locations for linear sampling.
 * Input:
 *    wrapMode = GL_REPEAT, GL_CLAMP, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
 *    S = texcoord in [0,1]
 *    SIZE = width (or height or depth) of texture
 * Output:
 *    U = texcoord in [0, width]
 *    I0, I1 = two nearest texel indexes
 */
#define COMPUTE_LINEAR_TEXEL_LOCATIONS(wrapMode, S, U, SIZE, I0, I1)	\
{									\
   if (wrapMode == GL_REPEAT) {						\
      U = S * SIZE - 0.5F;						\
      if (tObj->_IsPowerOfTwo) {					\
         I0 = IFLOOR(U) & (SIZE - 1);					\
         I1 = (I0 + 1) & (SIZE - 1);					\
      }									\
      else {								\
         I0 = repeat_remainder(IFLOOR(U), SIZE);			\
         I1 = repeat_remainder(I0 + 1, SIZE);				\
      }									\
   }									\
   else if (wrapMode == GL_CLAMP_TO_EDGE) {				\
      if (S <= 0.0F)							\
         U = 0.0F;							\
      else if (S >= 1.0F)						\
         U = (GLfloat) SIZE;						\
      else								\
         U = S * SIZE;							\
      U -= 0.5F;							\
      I0 = IFLOOR(U);							\
      I1 = I0 + 1;							\
      if (I0 < 0)							\
         I0 = 0;							\
      if (I1 >= (GLint) SIZE)						\
         I1 = SIZE - 1;							\
   }									\
   else if (wrapMode == GL_CLAMP_TO_BORDER) {				\
      const GLfloat min = -1.0F / (2.0F * SIZE);			\
      const GLfloat max = 1.0F - min;					\
      if (S <= min)							\
         U = min * SIZE;						\
      else if (S >= max)						\
         U = max * SIZE;						\
      else								\
         U = S * SIZE;							\
      U -= 0.5F;							\
      I0 = IFLOOR(U);							\
      I1 = I0 + 1;							\
   }									\
   else if (wrapMode == GL_MIRRORED_REPEAT) {				\
      const GLint flr = IFLOOR(S);					\
      if (flr & 1)							\
         U = 1.0F - (S - (GLfloat) flr);	/* flr is odd */	\
      else								\
         U = S - (GLfloat) flr;		/* flr is even */		\
      U = (U * SIZE) - 0.5F;						\
      I0 = IFLOOR(U);							\
      I1 = I0 + 1;							\
      if (I0 < 0)							\
         I0 = 0;							\
      if (I1 >= (GLint) SIZE)						\
         I1 = SIZE - 1;							\
   }									\
   else if (wrapMode == GL_MIRROR_CLAMP_EXT) {				\
      U = (GLfloat) fabs(S);						\
      if (U >= 1.0F)							\
         U = (GLfloat) SIZE;						\
      else								\
         U *= SIZE;							\
      U -= 0.5F;							\
      I0 = IFLOOR(U);							\
      I1 = I0 + 1;							\
   }									\
   else if (wrapMode == GL_MIRROR_CLAMP_TO_EDGE_EXT) {			\
      U = (GLfloat) fabs(S);						\
      if (U >= 1.0F)							\
         U = (GLfloat) SIZE;						\
      else								\
         U *= SIZE;							\
      U -= 0.5F;							\
      I0 = IFLOOR(U);							\
      I1 = I0 + 1;							\
      if (I0 < 0)							\
         I0 = 0;							\
      if (I1 >= (GLint) SIZE)						\
         I1 = SIZE - 1;							\
   }									\
   else if (wrapMode == GL_MIRROR_CLAMP_TO_BORDER_EXT) {		\
      const GLfloat min = -1.0F / (2.0F * SIZE);			\
      const GLfloat max = 1.0F - min;					\
      U = (GLfloat) fabs(S);						\
      if (U <= min)							\
         U = min * SIZE;						\
      else if (U >= max)						\
         U = max * SIZE;						\
      else								\
         U *= SIZE;							\
      U -= 0.5F;							\
      I0 = IFLOOR(U);							\
      I1 = I0 + 1;							\
   }									\
   else {								\
      ASSERT(wrapMode == GL_CLAMP);					\
      if (S <= 0.0F)							\
         U = 0.0F;							\
      else if (S >= 1.0F)						\
         U = (GLfloat) SIZE;						\
      else								\
         U = S * SIZE;							\
      U -= 0.5F;							\
      I0 = IFLOOR(U);							\
      I1 = I0 + 1;							\
   }									\
}


/*
 * Used to compute texel location for nearest sampling.
 */
#define COMPUTE_NEAREST_TEXEL_LOCATION(wrapMode, S, SIZE, I)		\
{									\
   if (wrapMode == GL_REPEAT) {						\
      /* s limited to [0,1) */						\
      /* i limited to [0,size-1] */					\
      I = IFLOOR(S * SIZE);						\
      if (tObj->_IsPowerOfTwo)						\
         I &= (SIZE - 1);						\
      else								\
         I = repeat_remainder(I, SIZE);					\
   }									\
   else if (wrapMode == GL_CLAMP_TO_EDGE) {				\
      /* s limited to [min,max] */					\
      /* i limited to [0, size-1] */					\
      const GLfloat min = 1.0F / (2.0F * SIZE);				\
      const GLfloat max = 1.0F - min;					\
      if (S < min)							\
         I = 0;								\
      else if (S > max)							\
         I = SIZE - 1;							\
      else								\
         I = IFLOOR(S * SIZE);						\
   }									\
   else if (wrapMode == GL_CLAMP_TO_BORDER) {				\
      /* s limited to [min,max] */					\
      /* i limited to [-1, size] */					\
      const GLfloat min = -1.0F / (2.0F * SIZE);			\
      const GLfloat max = 1.0F - min;					\
      if (S <= min)							\
         I = -1;							\
      else if (S >= max)						\
         I = SIZE;							\
      else								\
         I = IFLOOR(S * SIZE);						\
   }									\
   else if (wrapMode == GL_MIRRORED_REPEAT) {				\
      const GLfloat min = 1.0F / (2.0F * SIZE);				\
      const GLfloat max = 1.0F - min;					\
      const GLint flr = IFLOOR(S);					\
      GLfloat u;							\
      if (flr & 1)							\
         u = 1.0F - (S - (GLfloat) flr);	/* flr is odd */	\
      else								\
         u = S - (GLfloat) flr;		/* flr is even */		\
      if (u < min)							\
         I = 0;								\
      else if (u > max)							\
         I = SIZE - 1;							\
      else								\
         I = IFLOOR(u * SIZE);						\
   }									\
   else if (wrapMode == GL_MIRROR_CLAMP_EXT) {				\
      /* s limited to [0,1] */						\
      /* i limited to [0,size-1] */					\
      const GLfloat u = (GLfloat) fabs(S);				\
      if (u <= 0.0F)							\
         I = 0;								\
      else if (u >= 1.0F)						\
         I = SIZE - 1;							\
      else								\
         I = IFLOOR(u * SIZE);						\
   }									\
   else if (wrapMode == GL_MIRROR_CLAMP_TO_EDGE_EXT) {			\
      /* s limited to [min,max] */					\
      /* i limited to [0, size-1] */					\
      const GLfloat min = 1.0F / (2.0F * SIZE);				\
      const GLfloat max = 1.0F - min;					\
      const GLfloat u = (GLfloat) fabs(S);				\
      if (u < min)							\
         I = 0;								\
      else if (u > max)							\
         I = SIZE - 1;							\
      else								\
         I = IFLOOR(u * SIZE);						\
   }									\
   else if (wrapMode == GL_MIRROR_CLAMP_TO_BORDER_EXT) {		\
      /* s limited to [min,max] */					\
      /* i limited to [0, size-1] */					\
      const GLfloat min = -1.0F / (2.0F * SIZE);			\
      const GLfloat max = 1.0F - min;					\
      const GLfloat u = (GLfloat) fabs(S);				\
      if (u < min)							\
         I = -1;							\
      else if (u > max)							\
         I = SIZE;							\
      else								\
         I = IFLOOR(u * SIZE);						\
   }									\
   else {								\
      ASSERT(wrapMode == GL_CLAMP);					\
      /* s limited to [0,1] */						\
      /* i limited to [0,size-1] */					\
      if (S <= 0.0F)							\
         I = 0;								\
      else if (S >= 1.0F)						\
         I = SIZE - 1;							\
      else								\
         I = IFLOOR(S * SIZE);						\
   }									\
}


/* Power of two image sizes only */
#define COMPUTE_LINEAR_REPEAT_TEXEL_LOCATION(S, U, SIZE, I0, I1)	\
{									\
   U = S * SIZE - 0.5F;							\
   I0 = IFLOOR(U) & (SIZE - 1);						\
   I1 = (I0 + 1) & (SIZE - 1);						\
}


/*
 * Compute linear mipmap levels for given lambda.
 */
#define COMPUTE_LINEAR_MIPMAP_LEVEL(tObj, lambda, level)	\
{								\
   if (lambda < 0.0F)						\
      level = tObj->BaseLevel;					\
   else if (lambda > tObj->_MaxLambda)				\
      level = (GLint) (tObj->BaseLevel + tObj->_MaxLambda);	\
   else								\
      level = (GLint) (tObj->BaseLevel + lambda);		\
}


/*
 * Compute nearest mipmap level for given lambda.
 */
#define COMPUTE_NEAREST_MIPMAP_LEVEL(tObj, lambda, level)	\
{								\
   GLfloat l;							\
   if (lambda <= 0.5F)						\
      l = 0.0F;							\
   else if (lambda > tObj->_MaxLambda + 0.4999F)		\
      l = tObj->_MaxLambda + 0.4999F;				\
   else								\
      l = lambda;						\
   level = (GLint) (tObj->BaseLevel + l + 0.5F);		\
   if (level > tObj->_MaxLevel)					\
      level = tObj->_MaxLevel;					\
}



/*
 * Note, the FRAC macro has to work perfectly.  Otherwise you'll sometimes
 * see 1-pixel bands of improperly weighted linear-sampled texels.  The
 * tests/texwrap.c demo is a good test.
 * Also note, FRAC(x) doesn't truly return the fractional part of x for x < 0.
 * Instead, if x < 0 then FRAC(x) = 1 - true_frac(x).
 */
#define FRAC(f)  ((f) - IFLOOR(f))



/*
 * Bitflags for texture border color sampling.
 */
#define I0BIT   1
#define I1BIT   2
#define J0BIT   4
#define J1BIT   8
#define K0BIT  16
#define K1BIT  32



#if 000
/*
 * Get texture palette entry.
 */
static void
palette_sample(const GLcontext *ctx,
               const struct gl_texture_object *tObj,
               GLint index, GLchan rgba[4] )
{
   const GLchan *palette;
   GLenum format;

   if (ctx->Texture.SharedPalette) {
      ASSERT(ctx->Texture.Palette.Type != GL_FLOAT);
      palette = (const GLchan *) ctx->Texture.Palette.Table;
      format = ctx->Texture.Palette.Format;
   }
   else {
      ASSERT(tObj->Palette.Type != GL_FLOAT);
      palette = (const GLchan *) tObj->Palette.Table;
      format = tObj->Palette.Format;
   }

   switch (format) {
      case GL_ALPHA:
         rgba[ACOMP] = palette[index];
         return;
      case GL_LUMINANCE:
      case GL_INTENSITY:
         rgba[RCOMP] = palette[index];
         return;
      case GL_LUMINANCE_ALPHA:
         rgba[RCOMP] = palette[(index << 1) + 0];
         rgba[ACOMP] = palette[(index << 1) + 1];
         return;
      case GL_RGB:
         rgba[RCOMP] = palette[index * 3 + 0];
         rgba[GCOMP] = palette[index * 3 + 1];
         rgba[BCOMP] = palette[index * 3 + 2];
         return;
      case GL_RGBA:
         rgba[RCOMP] = palette[(index << 2) + 0];
         rgba[GCOMP] = palette[(index << 2) + 1];
         rgba[BCOMP] = palette[(index << 2) + 2];
         rgba[ACOMP] = palette[(index << 2) + 3];
         return;
      default:
         _mesa_problem(ctx, "Bad palette format in palette_sample");
   }
}
#endif



/**********************************************************************/
/*                    1-D Texture Sampling Functions                  */
/**********************************************************************/

/*
 * Return the texture sample for coordinate (s) using GL_NEAREST filter.
 */
static void
sample_1d_nearest(GLcontext *ctx,
                  const struct gl_texture_object *tObj,
                  const struct gl_texture_image *img,
                  const GLfloat texcoord[4], TYPE rgba[4])
{
   const GLint width = img->Width2;  /* without border */
   GLint i;

   COMPUTE_NEAREST_TEXEL_LOCATION(tObj->WrapS, texcoord[0], width, i);

   /* skip over the border, if any */
   i += img->Border;

   if (i < 0 || i >= (GLint) img->Width) {
      /* Need this test for GL_CLAMP_TO_BORDER mode */
#if TYPE_ENUM == GL_FLOAT
      COPY_4V(rgba, tObj->BorderColor);
#else
      COPY_CHAN4(rgba, tObj->_BorderChan);
#endif      
   }
   else {
#if TYPE_ENUM == GL_FLOAT
      img->FetchTexelf(img, i, 0, 0, rgba);
#else
      img->FetchTexelc(img, i, 0, 0, rgba);
#endif
      if (img->Format == GL_COLOR_INDEX) {
         palette_sample(ctx, tObj, rgba[0], rgba);
      }
   }
}



/*
 * Return the texture sample for coordinate (s) using GL_LINEAR filter.
 */
static void
sample_1d_linear(GLcontext *ctx,
                 const struct gl_texture_object *tObj,
                 const struct gl_texture_image *img,
                 const GLfloat texcoord[4], GLchan rgba[4])
{
   const GLint width = img->Width2;
   GLint i0, i1;
   GLfloat u;
   GLuint useBorderColor;

   COMPUTE_LINEAR_TEXEL_LOCATIONS(tObj->WrapS, texcoord[0], u, width, i0, i1);

   useBorderColor = 0;
   if (img->Border) {
      i0 += img->Border;
      i1 += img->Border;
   }
   else {
      if (i0 < 0 || i0 >= width)   useBorderColor |= I0BIT;
      if (i1 < 0 || i1 >= width)   useBorderColor |= I1BIT;
   }

   {
      const GLfloat a = FRAC(u);

#if CHAN_TYPE == GL_FLOAT || CHAN_TYPE == GL_UNSIGNED_SHORT
      const GLfloat w0 = (1.0F-a);
      const GLfloat w1 =       a ;
#else /* CHAN_BITS == 8 */
      /* compute sample weights in fixed point in [0,WEIGHT_SCALE] */
      const GLint w0 = IROUND_POS((1.0F - a) * WEIGHT_SCALE);
      const GLint w1 = IROUND_POS(        a  * WEIGHT_SCALE);
#endif
      GLchan t0[4], t1[4];  /* texels */

      if (useBorderColor & I0BIT) {
         COPY_CHAN4(t0, tObj->_BorderChan);
      }
      else {
         img->FetchTexelc(img, i0, 0, 0, t0);
         if (img->Format == GL_COLOR_INDEX) {
            palette_sample(ctx, tObj, t0[0], t0);
         }
      }
      if (useBorderColor & I1BIT) {
         COPY_CHAN4(t1, tObj->_BorderChan);
      }
      else {
         img->FetchTexelc(img, i1, 0, 0, t1);
         if (img->Format == GL_COLOR_INDEX) {
            palette_sample(ctx, tObj, t1[0], t1);
         }
      }

#if CHAN_TYPE == GL_FLOAT
      rgba[0] = w0 * t0[0] + w1 * t1[0];
      rgba[1] = w0 * t0[1] + w1 * t1[1];
      rgba[2] = w0 * t0[2] + w1 * t1[2];
      rgba[3] = w0 * t0[3] + w1 * t1[3];
#elif CHAN_TYPE == GL_UNSIGNED_SHORT
      rgba[0] = (GLchan) (w0 * t0[0] + w1 * t1[0] + 0.5);
      rgba[1] = (GLchan) (w0 * t0[1] + w1 * t1[1] + 0.5);
      rgba[2] = (GLchan) (w0 * t0[2] + w1 * t1[2] + 0.5);
      rgba[3] = (GLchan) (w0 * t0[3] + w1 * t1[3] + 0.5);
#else /* CHAN_BITS == 8 */
      rgba[0] = (GLchan) ((w0 * t0[0] + w1 * t1[0]) >> WEIGHT_SHIFT);
      rgba[1] = (GLchan) ((w0 * t0[1] + w1 * t1[1]) >> WEIGHT_SHIFT);
      rgba[2] = (GLchan) ((w0 * t0[2] + w1 * t1[2]) >> WEIGHT_SHIFT);
      rgba[3] = (GLchan) ((w0 * t0[3] + w1 * t1[3]) >> WEIGHT_SHIFT);
#endif

   }
}


static void
sample_1d_nearest_mipmap_nearest(GLcontext *ctx,
                                 const struct gl_texture_object *tObj,
                                 GLuint n, const GLfloat texcoord[][4],
                                 const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   ASSERT(lambda != NULL);
   for (i = 0; i < n; i++) {
      GLint level;
      COMPUTE_NEAREST_MIPMAP_LEVEL(tObj, lambda[i], level);
      sample_1d_nearest(ctx, tObj, tObj->Image[0][level], texcoord[i], rgba[i]);
   }
}


static void
sample_1d_linear_mipmap_nearest(GLcontext *ctx,
                                const struct gl_texture_object *tObj,
                                GLuint n, const GLfloat texcoord[][4],
                                const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   ASSERT(lambda != NULL);
   for (i = 0; i < n; i++) {
      GLint level;
      COMPUTE_NEAREST_MIPMAP_LEVEL(tObj, lambda[i], level);
      sample_1d_linear(ctx, tObj, tObj->Image[0][level], texcoord[i], rgba[i]);
   }
}



/*
 * This is really just needed in order to prevent warnings with some compilers.
 */
#if CHAN_TYPE == GL_FLOAT
#define CHAN_CAST
#else
#define CHAN_CAST (GLchan) (GLint)
#endif


static void
sample_1d_nearest_mipmap_linear(GLcontext *ctx,
                                const struct gl_texture_object *tObj,
                                GLuint n, const GLfloat texcoord[][4],
                                const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   ASSERT(lambda != NULL);
   for (i = 0; i < n; i++) {
      GLint level;
      COMPUTE_LINEAR_MIPMAP_LEVEL(tObj, lambda[i], level);
      if (level >= tObj->_MaxLevel) {
         sample_1d_nearest(ctx, tObj, tObj->Image[0][tObj->_MaxLevel],
                           texcoord[i], rgba[i]);
      }
      else {
         GLchan t0[4], t1[4];
         const GLfloat f = FRAC(lambda[i]);
         sample_1d_nearest(ctx, tObj, tObj->Image[0][level  ], texcoord[i], t0);
         sample_1d_nearest(ctx, tObj, tObj->Image[0][level+1], texcoord[i], t1);
         rgba[i][RCOMP] = CHAN_CAST ((1.0F-f) * t0[RCOMP] + f * t1[RCOMP]);
         rgba[i][GCOMP] = CHAN_CAST ((1.0F-f) * t0[GCOMP] + f * t1[GCOMP]);
         rgba[i][BCOMP] = CHAN_CAST ((1.0F-f) * t0[BCOMP] + f * t1[BCOMP]);
         rgba[i][ACOMP] = CHAN_CAST ((1.0F-f) * t0[ACOMP] + f * t1[ACOMP]);
      }
   }
}



static void
sample_1d_linear_mipmap_linear(GLcontext *ctx,
                               const struct gl_texture_object *tObj,
                               GLuint n, const GLfloat texcoord[][4],
                               const GLfloat lambda[], GLchan rgba[][4])
{
   GLuint i;
   ASSERT(lambda != NULL);
   for (i = 0; i < n; i++) {
      GLint level;
      COMPUTE_LINEAR_MIPMAP_LEVEL(tObj, lambda[i], level);
      if (level >= tObj->_MaxLevel) {
         sample_1d_linear(ctx, tObj, tObj->Image[0][tObj->_MaxLevel],
                          texcoord[i], rgba[i]);
      }
      else {
         GLchan t0[4], t1[4];
         const GLfloat f = FRAC(lambda[i]);
         sample_1d_linear(ctx, tObj, tObj->Image[0][level  ], texcoord[i], t0);
         sample_1d_linear(ctx, tObj, tObj->Image[0][level+1], texcoord[i], t1);
         rgba[i][RCOMP] = CHAN_CAST ((1.0F-f) * t0[RCOMP] + f * t1[RCOMP]);
         rgba[i][GCOMP] = CHAN_CAST ((1.0F-f) * t0[GCOMP] + f * t1[GCOMP]);
         rgba[i][BCOMP] = CHAN_CAST ((1.0F-f) * t0[BCOMP] + f * t1[BCOMP]);
         rgba[i][ACOMP] = CHAN_CAST ((1.0F-f) * t0[ACOMP] + f * t1[ACOMP]);
      }
   }
}



static void
sample_nearest_1d( GLcontext *ctx, GLuint texUnit,
                   const struct gl_texture_object *tObj, GLuint n,
                   const GLfloat texcoords[][4], const GLfloat lambda[],
                   GLchan rgba[][4] )
{
   GLuint i;
   struct gl_texture_image *image = tObj->Image[0][tObj->BaseLevel];
   (void) lambda;
   for (i=0;i<n;i++) {
      sample_1d_nearest(ctx, tObj, image, texcoords[i], rgba[i]);
   }
}



static void
sample_linear_1d( GLcontext *ctx, GLuint texUnit,
                  const struct gl_texture_object *tObj, GLuint n,
                  const GLfloat texcoords[][4], const GLfloat lambda[],
                  GLchan rgba[][4] )
{
   GLuint i;
   struct gl_texture_image *image = tObj->Image[0][tObj->BaseLevel];
   (void) lambda;
   for (i=0;i<n;i++) {
      sample_1d_linear(ctx, tObj, image, texcoords[i], rgba[i]);
   }
}


/*
 * Given an (s) texture coordinate and lambda (level of detail) value,
 * return a texture sample.
 *
 */
static void
sample_lambda_1d( GLcontext *ctx, GLuint texUnit,
                  const struct gl_texture_object *tObj, GLuint n,
                  const GLfloat texcoords[][4],
                  const GLfloat lambda[], GLchan rgba[][4] )
{
   GLuint minStart, minEnd;  /* texels with minification */
   GLuint magStart, magEnd;  /* texels with magnification */
   GLuint i;

   ASSERT(lambda != NULL);
   compute_min_mag_ranges(SWRAST_CONTEXT(ctx)->_MinMagThresh[texUnit],
                          n, lambda, &minStart, &minEnd, &magStart, &magEnd);

   if (minStart < minEnd) {
      /* do the minified texels */
      const GLuint m = minEnd - minStart;
      switch (tObj->MinFilter) {
      case GL_NEAREST:
         for (i = minStart; i < minEnd; i++)
            sample_1d_nearest(ctx, tObj, tObj->Image[0][tObj->BaseLevel],
                              texcoords[i], rgba[i]);
         break;
      case GL_LINEAR:
         for (i = minStart; i < minEnd; i++)
            sample_1d_linear(ctx, tObj, tObj->Image[0][tObj->BaseLevel],
                             texcoords[i], rgba[i]);
         break;
      case GL_NEAREST_MIPMAP_NEAREST:
         sample_1d_nearest_mipmap_nearest(ctx, tObj, m, texcoords + minStart,
                                          lambda + minStart, rgba + minStart);
         break;
      case GL_LINEAR_MIPMAP_NEAREST:
         sample_1d_linear_mipmap_nearest(ctx, tObj, m, texcoords + minStart,
                                         lambda + minStart, rgba + minStart);
         break;
      case GL_NEAREST_MIPMAP_LINEAR:
         sample_1d_nearest_mipmap_linear(ctx, tObj, m, texcoords + minStart,
                                         lambda + minStart, rgba + minStart);
         break;
      case GL_LINEAR_MIPMAP_LINEAR:
         sample_1d_linear_mipmap_linear(ctx, tObj, m, texcoords + minStart,
                                        lambda + minStart, rgba + minStart);
         break;
      default:
         _mesa_problem(ctx, "Bad min filter in sample_1d_texture");
         return;
      }
   }

   if (magStart < magEnd) {
      /* do the magnified texels */
      switch (tObj->MagFilter) {
      case GL_NEAREST:
         for (i = magStart; i < magEnd; i++)
            sample_1d_nearest(ctx, tObj, tObj->Image[0][tObj->BaseLevel],
                              texcoords[i], rgba[i]);
         break;
      case GL_LINEAR:
         for (i = magStart; i < magEnd; i++)
            sample_1d_linear(ctx, tObj, tObj->Image[0][tObj->BaseLevel],
                             texcoords[i], rgba[i]);
         break;
      default:
         _mesa_problem(ctx, "Bad mag filter in sample_1d_texture");
         return;
      }
   }
}


