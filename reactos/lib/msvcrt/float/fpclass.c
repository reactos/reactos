#include <msvcrti.h>


#define _FPCLASS_SNAN	0x0001	/* signaling NaN */
#define _FPCLASS_QNAN	0x0002	/* quiet NaN */
#define _FPCLASS_NINF	0x0004	/* negative infinity */
#define _FPCLASS_NN	0x0008	/* negative normal */
#define _FPCLASS_ND	0x0010	/* negative denormal */
#define _FPCLASS_NZ	0x0020	/* -0 */
#define _FPCLASS_PZ	0x0040	/* +0 */
#define _FPCLASS_PD	0x0080	/* positive denormal */
#define _FPCLASS_PN	0x0100	/* positive normal */
#define _FPCLASS_PINF	0x0200	/* positive infinity */

#define FP_SNAN       0x0001  //    signaling NaN
#define	FP_QNAN       0x0002  //    quiet NaN
#define	FP_NINF       0x0004  //    negative infinity
#define FP_PINF       0x0200  //    positive infinity
#define FP_NDENORM    0x0008  //    negative denormalized non-zero
#define FP_PDENORM    0x0010  //    positive denormalized non-zero
#define FP_NZERO      0x0020  //    negative zero
#define FP_PZERO      0x0040  //    positive zero
#define FP_NNORM      0x0080  //    negative normalized non-zero
#define FP_PNORM      0x0100  //    positive normalized non-zero

typedef int fpclass_t;

fpclass_t _fpclass(double __d)
{
	double_t *d = (double_t *)&__d;

	if ( d->exponent == 0 ) {
		if ( d->mantissah == 0 &&  d->mantissal == 0 ) {
			if ( d->sign ==0 )
				return FP_NZERO;
			else
				return FP_PZERO;
		} else {
			if ( d->sign ==0 )
				return FP_NDENORM;
			else
				return FP_PDENORM;
		}
	}
	if (d->exponent == 0x7ff ) {
		if ( d->mantissah == 0 &&  d->mantissal == 0 ) {
			if ( d->sign ==0 )
				return FP_NINF;
			else
				return FP_PINF;
		} 
		else if ( d->mantissah == 0 &&  d->mantissal != 0 ) {
			return FP_QNAN;
		}
		else if ( d->mantissah == 0 &&  d->mantissal != 0 ) {
			return FP_SNAN;
		}
	
	}

	return 0;
}


