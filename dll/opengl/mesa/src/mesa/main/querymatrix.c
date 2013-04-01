/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 **************************************************************************/


/**
 * Code to implement GL_OES_query_matrix.  See the spec at:
 * http://www.khronos.org/registry/gles/extensions/OES/OES_query_matrix.txt
 */


#include <stdlib.h>
#include <math.h>
#include "GLES/gl.h"
#include "GLES/glext.h"


/**
 * This is from the GL_OES_query_matrix extension specification:
 *
 *  GLbitfield glQueryMatrixxOES( GLfixed mantissa[16],
 *                                GLint   exponent[16] )
 *  mantissa[16] contains the contents of the current matrix in GLfixed
 *  format.  exponent[16] contains the unbiased exponents applied to the
 *  matrix components, so that the internal representation of component i
 *  is close to mantissa[i] * 2^exponent[i].  The function returns a status
 *  word which is zero if all the components are valid. If
 *  status & (1<<i) != 0, the component i is invalid (e.g., NaN, Inf).
 *  The implementations are not required to keep track of overflows.  In
 *  that case, the invalid bits are never set.
 */

#define INT_TO_FIXED(x) ((GLfixed) ((x) << 16))
#define FLOAT_TO_FIXED(x) ((GLfixed) ((x) * 65536.0))

#if defined(_MSC_VER)
/* Oddly, the fpclassify() function doesn't exist in such a form
 * on MSVC.  This is an implementation using slightly different
 * lower-level Windows functions.
 */
#include <float.h>

enum {FP_NAN, FP_INFINITE, FP_ZERO, FP_SUBNORMAL, FP_NORMAL}
fpclassify(double x)
{
    switch(_fpclass(x)) {
        case _FPCLASS_SNAN: /* signaling NaN */
        case _FPCLASS_QNAN: /* quiet NaN */
            return FP_NAN;
        case _FPCLASS_NINF: /* negative infinity */
        case _FPCLASS_PINF: /* positive infinity */
            return FP_INFINITE;
        case _FPCLASS_NN:   /* negative normal */
        case _FPCLASS_PN:   /* positive normal */
            return FP_NORMAL;
        case _FPCLASS_ND:   /* negative denormalized */
        case _FPCLASS_PD:   /* positive denormalized */
            return FP_SUBNORMAL;
        case _FPCLASS_NZ:   /* negative zero */
        case _FPCLASS_PZ:   /* positive zero */
            return FP_ZERO;
        default:
            /* Should never get here; but if we do, this will guarantee
             * that the pattern is not treated like a number.
             */
            return FP_NAN;
    }
}

#elif defined(__APPLE__) || defined(__CYGWIN__) || defined(__FreeBSD__) || \
     defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__) || \
     (defined(__sun) && defined(__C99FEATURES__)) || defined(__MINGW32__) || \
     (defined(__sun) && defined(__GNUC__)) || defined(ANDROID) || defined(__HAIKU__)

/* fpclassify is available. */

#elif !defined(_XOPEN_SOURCE) || _XOPEN_SOURCE < 600

enum {FP_NAN, FP_INFINITE, FP_ZERO, FP_SUBNORMAL, FP_NORMAL}
fpclassify(double x)
{
   /* XXX do something better someday */
   return FP_NORMAL;
}

#endif

extern GLbitfield GL_APIENTRY _es_QueryMatrixxOES(GLfixed mantissa[16], GLint exponent[16]);

/* The Mesa functions we'll need */
extern void GL_APIENTRY _mesa_GetIntegerv(GLenum pname, GLint *params);
extern void GL_APIENTRY _mesa_GetFloatv(GLenum pname, GLfloat *params);

GLbitfield GL_APIENTRY _es_QueryMatrixxOES(GLfixed mantissa[16], GLint exponent[16])
{
    GLfloat matrix[16];
    GLint tmp;
    GLenum currentMode = GL_FALSE;
    GLenum desiredMatrix = GL_FALSE;
    /* The bitfield returns 1 for each component that is invalid (i.e.
     * NaN or Inf).  In case of error, everything is invalid.
     */
    GLbitfield rv;
    register unsigned int i;
    unsigned int bit;

    /* This data structure defines the mapping between the current matrix
     * mode and the desired matrix identifier.
     */
    static struct {
        GLenum currentMode;
        GLenum desiredMatrix;
    } modes[] = {
        {GL_MODELVIEW, GL_MODELVIEW_MATRIX},
        {GL_PROJECTION, GL_PROJECTION_MATRIX},
        {GL_TEXTURE, GL_TEXTURE_MATRIX},
    };

    /* Call Mesa to get the current matrix in floating-point form.  First,
     * we have to figure out what the current matrix mode is.
     */
    _mesa_GetIntegerv(GL_MATRIX_MODE, &tmp);
    currentMode = (GLenum) tmp;

    /* The mode is either GL_FALSE, if for some reason we failed to query
     * the mode, or a given mode from the above table.  Search for the
     * returned mode to get the desired matrix; if we don't find it,
     * we can return immediately, as _mesa_GetInteger() will have
     * logged the necessary error already.
     */
    for (i = 0; i < sizeof(modes)/sizeof(modes[0]); i++) {
        if (modes[i].currentMode == currentMode) {
            desiredMatrix = modes[i].desiredMatrix;
            break;
        }
    }
    if (desiredMatrix == GL_FALSE) {
        /* Early error means all values are invalid. */
        return 0xffff;
    }

    /* Now pull the matrix itself. */
    _mesa_GetFloatv(desiredMatrix, matrix);

    rv = 0;
    for (i = 0, bit = 1; i < 16; i++, bit<<=1) {
        float normalizedFraction;
        int exp;

        switch (fpclassify(matrix[i])) {
            /* A "subnormal" or denormalized number is too small to be
             * represented in normal format; but despite that it's a
             * valid floating point number.  FP_ZERO and FP_NORMAL
             * are both valid as well.  We should be fine treating
             * these three cases as legitimate floating-point numbers.
             */
            case FP_SUBNORMAL:
            case FP_NORMAL:
            case FP_ZERO:
                normalizedFraction = (GLfloat)frexp(matrix[i], &exp);
                mantissa[i] = FLOAT_TO_FIXED(normalizedFraction);
                exponent[i] = (GLint) exp;
                break;

            /* If the entry is not-a-number or an infinity, then the
             * matrix component is invalid.  The invalid flag for
             * the component is already set; might as well set the
             * other return values to known values.  We'll set
             * distinct values so that a savvy end user could determine
             * whether the matrix component was a NaN or an infinity,
             * but this is more useful for debugging than anything else
             * since the standard doesn't specify any such magic
             * values to return.
             */
            case FP_NAN:
                mantissa[i] = INT_TO_FIXED(0);
                exponent[i] = (GLint) 0;
                rv |= bit;
                break;

            case FP_INFINITE:
                /* Return +/- 1 based on whether it's a positive or
                 * negative infinity.
                 */
                if (matrix[i] > 0) {
                    mantissa[i] = INT_TO_FIXED(1);
                }
                else {
                    mantissa[i] = -INT_TO_FIXED(1);
                }
                exponent[i] = (GLint) 0;
                rv |= bit;
                break;

            /* We should never get here; but here's a catching case
             * in case fpclassify() is returnings something unexpected.
             */
            default:
                mantissa[i] = INT_TO_FIXED(2);
                exponent[i] = (GLint) 0;
                rv |= bit;
                break;
        }

    } /* for each component */

    /* All done */
    return rv;
}
