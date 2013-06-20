
/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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
 * eval.c was written by
 * Bernd Barsuhn (bdbarsuh@cip.informatik.uni-erlangen.de) and
 * Volker Weiss (vrweiss@cip.informatik.uni-erlangen.de).
 *
 * My original implementation of evaluators was simplistic and didn't
 * compute surface normal vectors properly.  Bernd and Volker applied
 * used more sophisticated methods to get better results.
 *
 * Thanks guys!
 */


#include "main/glheader.h"
#include "main/config.h"
#include "m_eval.h"

static GLfloat inv_tab[MAX_EVAL_ORDER];



/*
 * Horner scheme for Bezier curves
 *
 * Bezier curves can be computed via a Horner scheme.
 * Horner is numerically less stable than the de Casteljau
 * algorithm, but it is faster. For curves of degree n
 * the complexity of Horner is O(n) and de Casteljau is O(n^2).
 * Since stability is not important for displaying curve
 * points I decided to use the Horner scheme.
 *
 * A cubic Bezier curve with control points b0, b1, b2, b3 can be
 * written as
 *
 *        (([3]        [3]     )     [3]       )     [3]
 * c(t) = (([0]*s*b0 + [1]*t*b1)*s + [2]*t^2*b2)*s + [3]*t^2*b3
 *
 *                                           [n]
 * where s=1-t and the binomial coefficients [i]. These can
 * be computed iteratively using the identity:
 *
 * [n]               [n  ]             [n]
 * [i] = (n-i+1)/i * [i-1]     and     [0] = 1
 */


void
_math_horner_bezier_curve(const GLfloat * cp, GLfloat * out, GLfloat t,
			  GLuint dim, GLuint order)
{
   GLfloat s, powert, bincoeff;
   GLuint i, k;

   if (order >= 2) {
      bincoeff = (GLfloat) (order - 1);
      s = 1.0F - t;

      for (k = 0; k < dim; k++)
	 out[k] = s * cp[k] + bincoeff * t * cp[dim + k];

      for (i = 2, cp += 2 * dim, powert = t * t; i < order;
	   i++, powert *= t, cp += dim) {
	 bincoeff *= (GLfloat) (order - i);
	 bincoeff *= inv_tab[i];

	 for (k = 0; k < dim; k++)
	    out[k] = s * out[k] + bincoeff * powert * cp[k];
      }
   }
   else {			/* order=1 -> constant curve */

      for (k = 0; k < dim; k++)
	 out[k] = cp[k];
   }
}

/*
 * Tensor product Bezier surfaces
 *
 * Again the Horner scheme is used to compute a point on a
 * TP Bezier surface. First a control polygon for a curve
 * on the surface in one parameter direction is computed,
 * then the point on the curve for the other parameter
 * direction is evaluated.
 *
 * To store the curve control polygon additional storage
 * for max(uorder,vorder) points is needed in the
 * control net cn.
 */

void
_math_horner_bezier_surf(GLfloat * cn, GLfloat * out, GLfloat u, GLfloat v,
			 GLuint dim, GLuint uorder, GLuint vorder)
{
   GLfloat *cp = cn + uorder * vorder * dim;
   GLuint i, uinc = vorder * dim;

   if (vorder > uorder) {
      if (uorder >= 2) {
	 GLfloat s, poweru, bincoeff;
	 GLuint j, k;

	 /* Compute the control polygon for the surface-curve in u-direction */
	 for (j = 0; j < vorder; j++) {
	    GLfloat *ucp = &cn[j * dim];

	    /* Each control point is the point for parameter u on a */
	    /* curve defined by the control polygons in u-direction */
	    bincoeff = (GLfloat) (uorder - 1);
	    s = 1.0F - u;

	    for (k = 0; k < dim; k++)
	       cp[j * dim + k] = s * ucp[k] + bincoeff * u * ucp[uinc + k];

	    for (i = 2, ucp += 2 * uinc, poweru = u * u; i < uorder;
		 i++, poweru *= u, ucp += uinc) {
	       bincoeff *= (GLfloat) (uorder - i);
	       bincoeff *= inv_tab[i];

	       for (k = 0; k < dim; k++)
		  cp[j * dim + k] =
		     s * cp[j * dim + k] + bincoeff * poweru * ucp[k];
	    }
	 }

	 /* Evaluate curve point in v */
	 _math_horner_bezier_curve(cp, out, v, dim, vorder);
      }
      else			/* uorder=1 -> cn defines a curve in v */
	 _math_horner_bezier_curve(cn, out, v, dim, vorder);
   }
   else {			/* vorder <= uorder */

      if (vorder > 1) {
	 GLuint i;

	 /* Compute the control polygon for the surface-curve in u-direction */
	 for (i = 0; i < uorder; i++, cn += uinc) {
	    /* For constant i all cn[i][j] (j=0..vorder) are located */
	    /* on consecutive memory locations, so we can use        */
	    /* horner_bezier_curve to compute the control points     */

	    _math_horner_bezier_curve(cn, &cp[i * dim], v, dim, vorder);
	 }

	 /* Evaluate curve point in u */
	 _math_horner_bezier_curve(cp, out, u, dim, uorder);
      }
      else			/* vorder=1 -> cn defines a curve in u */
	 _math_horner_bezier_curve(cn, out, u, dim, uorder);
   }
}

/*
 * The direct de Casteljau algorithm is used when a point on the
 * surface and the tangent directions spanning the tangent plane
 * should be computed (this is needed to compute normals to the
 * surface). In this case the de Casteljau algorithm approach is
 * nicer because a point and the partial derivatives can be computed
 * at the same time. To get the correct tangent length du and dv
 * must be multiplied with the (u2-u1)/uorder-1 and (v2-v1)/vorder-1.
 * Since only the directions are needed, this scaling step is omitted.
 *
 * De Casteljau needs additional storage for uorder*vorder
 * values in the control net cn.
 */

void
_math_de_casteljau_surf(GLfloat * cn, GLfloat * out, GLfloat * du,
			GLfloat * dv, GLfloat u, GLfloat v, GLuint dim,
			GLuint uorder, GLuint vorder)
{
   GLfloat *dcn = cn + uorder * vorder * dim;
   GLfloat us = 1.0F - u, vs = 1.0F - v;
   GLuint h, i, j, k;
   GLuint minorder = uorder < vorder ? uorder : vorder;
   GLuint uinc = vorder * dim;
   GLuint dcuinc = vorder;

   /* Each component is evaluated separately to save buffer space  */
   /* This does not drasticaly decrease the performance of the     */
   /* algorithm. If additional storage for (uorder-1)*(vorder-1)   */
   /* points would be available, the components could be accessed  */
   /* in the innermost loop which could lead to less cache misses. */

#define CN(I,J,K) cn[(I)*uinc+(J)*dim+(K)]
#define DCN(I, J) dcn[(I)*dcuinc+(J)]
   if (minorder < 3) {
      if (uorder == vorder) {
	 for (k = 0; k < dim; k++) {
	    /* Derivative direction in u */
	    du[k] = vs * (CN(1, 0, k) - CN(0, 0, k)) +
	       v * (CN(1, 1, k) - CN(0, 1, k));

	    /* Derivative direction in v */
	    dv[k] = us * (CN(0, 1, k) - CN(0, 0, k)) +
	       u * (CN(1, 1, k) - CN(1, 0, k));

	    /* bilinear de Casteljau step */
	    out[k] = us * (vs * CN(0, 0, k) + v * CN(0, 1, k)) +
	       u * (vs * CN(1, 0, k) + v * CN(1, 1, k));
	 }
      }
      else if (minorder == uorder) {
	 for (k = 0; k < dim; k++) {
	    /* bilinear de Casteljau step */
	    DCN(1, 0) = CN(1, 0, k) - CN(0, 0, k);
	    DCN(0, 0) = us * CN(0, 0, k) + u * CN(1, 0, k);

	    for (j = 0; j < vorder - 1; j++) {
	       /* for the derivative in u */
	       DCN(1, j + 1) = CN(1, j + 1, k) - CN(0, j + 1, k);
	       DCN(1, j) = vs * DCN(1, j) + v * DCN(1, j + 1);

	       /* for the `point' */
	       DCN(0, j + 1) = us * CN(0, j + 1, k) + u * CN(1, j + 1, k);
	       DCN(0, j) = vs * DCN(0, j) + v * DCN(0, j + 1);
	    }

	    /* remaining linear de Casteljau steps until the second last step */
	    for (h = minorder; h < vorder - 1; h++)
	       for (j = 0; j < vorder - h; j++) {
		  /* for the derivative in u */
		  DCN(1, j) = vs * DCN(1, j) + v * DCN(1, j + 1);

		  /* for the `point' */
		  DCN(0, j) = vs * DCN(0, j) + v * DCN(0, j + 1);
	       }

	    /* derivative direction in v */
	    dv[k] = DCN(0, 1) - DCN(0, 0);

	    /* derivative direction in u */
	    du[k] = vs * DCN(1, 0) + v * DCN(1, 1);

	    /* last linear de Casteljau step */
	    out[k] = vs * DCN(0, 0) + v * DCN(0, 1);
	 }
      }
      else {			/* minorder == vorder */

	 for (k = 0; k < dim; k++) {
	    /* bilinear de Casteljau step */
	    DCN(0, 1) = CN(0, 1, k) - CN(0, 0, k);
	    DCN(0, 0) = vs * CN(0, 0, k) + v * CN(0, 1, k);
	    for (i = 0; i < uorder - 1; i++) {
	       /* for the derivative in v */
	       DCN(i + 1, 1) = CN(i + 1, 1, k) - CN(i + 1, 0, k);
	       DCN(i, 1) = us * DCN(i, 1) + u * DCN(i + 1, 1);

	       /* for the `point' */
	       DCN(i + 1, 0) = vs * CN(i + 1, 0, k) + v * CN(i + 1, 1, k);
	       DCN(i, 0) = us * DCN(i, 0) + u * DCN(i + 1, 0);
	    }

	    /* remaining linear de Casteljau steps until the second last step */
	    for (h = minorder; h < uorder - 1; h++)
	       for (i = 0; i < uorder - h; i++) {
		  /* for the derivative in v */
		  DCN(i, 1) = us * DCN(i, 1) + u * DCN(i + 1, 1);

		  /* for the `point' */
		  DCN(i, 0) = us * DCN(i, 0) + u * DCN(i + 1, 0);
	       }

	    /* derivative direction in u */
	    du[k] = DCN(1, 0) - DCN(0, 0);

	    /* derivative direction in v */
	    dv[k] = us * DCN(0, 1) + u * DCN(1, 1);

	    /* last linear de Casteljau step */
	    out[k] = us * DCN(0, 0) + u * DCN(1, 0);
	 }
      }
   }
   else if (uorder == vorder) {
      for (k = 0; k < dim; k++) {
	 /* first bilinear de Casteljau step */
	 for (i = 0; i < uorder - 1; i++) {
	    DCN(i, 0) = us * CN(i, 0, k) + u * CN(i + 1, 0, k);
	    for (j = 0; j < vorder - 1; j++) {
	       DCN(i, j + 1) = us * CN(i, j + 1, k) + u * CN(i + 1, j + 1, k);
	       DCN(i, j) = vs * DCN(i, j) + v * DCN(i, j + 1);
	    }
	 }

	 /* remaining bilinear de Casteljau steps until the second last step */
	 for (h = 2; h < minorder - 1; h++)
	    for (i = 0; i < uorder - h; i++) {
	       DCN(i, 0) = us * DCN(i, 0) + u * DCN(i + 1, 0);
	       for (j = 0; j < vorder - h; j++) {
		  DCN(i, j + 1) = us * DCN(i, j + 1) + u * DCN(i + 1, j + 1);
		  DCN(i, j) = vs * DCN(i, j) + v * DCN(i, j + 1);
	       }
	    }

	 /* derivative direction in u */
	 du[k] = vs * (DCN(1, 0) - DCN(0, 0)) + v * (DCN(1, 1) - DCN(0, 1));

	 /* derivative direction in v */
	 dv[k] = us * (DCN(0, 1) - DCN(0, 0)) + u * (DCN(1, 1) - DCN(1, 0));

	 /* last bilinear de Casteljau step */
	 out[k] = us * (vs * DCN(0, 0) + v * DCN(0, 1)) +
	    u * (vs * DCN(1, 0) + v * DCN(1, 1));
      }
   }
   else if (minorder == uorder) {
      for (k = 0; k < dim; k++) {
	 /* first bilinear de Casteljau step */
	 for (i = 0; i < uorder - 1; i++) {
	    DCN(i, 0) = us * CN(i, 0, k) + u * CN(i + 1, 0, k);
	    for (j = 0; j < vorder - 1; j++) {
	       DCN(i, j + 1) = us * CN(i, j + 1, k) + u * CN(i + 1, j + 1, k);
	       DCN(i, j) = vs * DCN(i, j) + v * DCN(i, j + 1);
	    }
	 }

	 /* remaining bilinear de Casteljau steps until the second last step */
	 for (h = 2; h < minorder - 1; h++)
	    for (i = 0; i < uorder - h; i++) {
	       DCN(i, 0) = us * DCN(i, 0) + u * DCN(i + 1, 0);
	       for (j = 0; j < vorder - h; j++) {
		  DCN(i, j + 1) = us * DCN(i, j + 1) + u * DCN(i + 1, j + 1);
		  DCN(i, j) = vs * DCN(i, j) + v * DCN(i, j + 1);
	       }
	    }

	 /* last bilinear de Casteljau step */
	 DCN(2, 0) = DCN(1, 0) - DCN(0, 0);
	 DCN(0, 0) = us * DCN(0, 0) + u * DCN(1, 0);
	 for (j = 0; j < vorder - 1; j++) {
	    /* for the derivative in u */
	    DCN(2, j + 1) = DCN(1, j + 1) - DCN(0, j + 1);
	    DCN(2, j) = vs * DCN(2, j) + v * DCN(2, j + 1);

	    /* for the `point' */
	    DCN(0, j + 1) = us * DCN(0, j + 1) + u * DCN(1, j + 1);
	    DCN(0, j) = vs * DCN(0, j) + v * DCN(0, j + 1);
	 }

	 /* remaining linear de Casteljau steps until the second last step */
	 for (h = minorder; h < vorder - 1; h++)
	    for (j = 0; j < vorder - h; j++) {
	       /* for the derivative in u */
	       DCN(2, j) = vs * DCN(2, j) + v * DCN(2, j + 1);

	       /* for the `point' */
	       DCN(0, j) = vs * DCN(0, j) + v * DCN(0, j + 1);
	    }

	 /* derivative direction in v */
	 dv[k] = DCN(0, 1) - DCN(0, 0);

	 /* derivative direction in u */
	 du[k] = vs * DCN(2, 0) + v * DCN(2, 1);

	 /* last linear de Casteljau step */
	 out[k] = vs * DCN(0, 0) + v * DCN(0, 1);
      }
   }
   else {			/* minorder == vorder */

      for (k = 0; k < dim; k++) {
	 /* first bilinear de Casteljau step */
	 for (i = 0; i < uorder - 1; i++) {
	    DCN(i, 0) = us * CN(i, 0, k) + u * CN(i + 1, 0, k);
	    for (j = 0; j < vorder - 1; j++) {
	       DCN(i, j + 1) = us * CN(i, j + 1, k) + u * CN(i + 1, j + 1, k);
	       DCN(i, j) = vs * DCN(i, j) + v * DCN(i, j + 1);
	    }
	 }

	 /* remaining bilinear de Casteljau steps until the second last step */
	 for (h = 2; h < minorder - 1; h++)
	    for (i = 0; i < uorder - h; i++) {
	       DCN(i, 0) = us * DCN(i, 0) + u * DCN(i + 1, 0);
	       for (j = 0; j < vorder - h; j++) {
		  DCN(i, j + 1) = us * DCN(i, j + 1) + u * DCN(i + 1, j + 1);
		  DCN(i, j) = vs * DCN(i, j) + v * DCN(i, j + 1);
	       }
	    }

	 /* last bilinear de Casteljau step */
	 DCN(0, 2) = DCN(0, 1) - DCN(0, 0);
	 DCN(0, 0) = vs * DCN(0, 0) + v * DCN(0, 1);
	 for (i = 0; i < uorder - 1; i++) {
	    /* for the derivative in v */
	    DCN(i + 1, 2) = DCN(i + 1, 1) - DCN(i + 1, 0);
	    DCN(i, 2) = us * DCN(i, 2) + u * DCN(i + 1, 2);

	    /* for the `point' */
	    DCN(i + 1, 0) = vs * DCN(i + 1, 0) + v * DCN(i + 1, 1);
	    DCN(i, 0) = us * DCN(i, 0) + u * DCN(i + 1, 0);
	 }

	 /* remaining linear de Casteljau steps until the second last step */
	 for (h = minorder; h < uorder - 1; h++)
	    for (i = 0; i < uorder - h; i++) {
	       /* for the derivative in v */
	       DCN(i, 2) = us * DCN(i, 2) + u * DCN(i + 1, 2);

	       /* for the `point' */
	       DCN(i, 0) = us * DCN(i, 0) + u * DCN(i + 1, 0);
	    }

	 /* derivative direction in u */
	 du[k] = DCN(1, 0) - DCN(0, 0);

	 /* derivative direction in v */
	 dv[k] = us * DCN(0, 2) + u * DCN(1, 2);

	 /* last linear de Casteljau step */
	 out[k] = us * DCN(0, 0) + u * DCN(1, 0);
      }
   }
#undef DCN
#undef CN
}


/*
 * Do one-time initialization for evaluators.
 */
void
_math_init_eval(void)
{
   GLuint i;

   /* KW: precompute 1/x for useful x.
    */
   for (i = 1; i < MAX_EVAL_ORDER; i++)
      inv_tab[i] = 1.0F / i;
}
