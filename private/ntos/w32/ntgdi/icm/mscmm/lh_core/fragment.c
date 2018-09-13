/*
	File:		LHFragment.c

	Contains:	Test fragment for Color Sync

	Written by:	H.Siegeritz

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

*/

#ifndef LHGeneralIncs_h
#include "General.h"
#endif

#if GENERATING68K
/*	#include <ConditionalMacros.h> */

	#define CM_Doub	extended
	extern CM_Doub pow(CM_Doub _x,CM_Doub _y);
#else
	#define CM_Doub	double
	#include <math.h>
#endif

#ifndef LHFragment_h
#include "Fragment.h"
#endif

#ifndef LHStdConversionLuts_h
#include "StdConv.h"
#endif

#if ! realThing
#ifdef DEBUG_OUTPUT
#define kThisFile kLHFragmentID
#endif
#endif


/*-----prototypes for local functions-----*/

void	InvLut1dExceptions(unsigned short *inCurve, unsigned long inCount,
                                unsigned short *outCurve, UINT8 AdressBits);
CMError	Fill_ushort_ELUT_Gamma(unsigned short *usELUT, char addrBits, char usedBits,
											long gridPoints, unsigned short gamma_u8_8);
void	Fill_inverseGamma_byte_ALUT(unsigned char *ucALUT, char addrBits,
													unsigned short gamma_u8_8);

/* ______________________________________________________________________

	icCurveType	*
		InvertLut1d(icCurveType	*LookUpTable,
                    UINT8		AdressBits);
	Abstract:
		allocates memory and inverts Lut
		NOTE: not-monotone LookUpTables are manipulated
		areas without values in LookUpTable are set 0 resp. 0xFFFF

	Params:
		LookUpTable	(in)	LUT to be inverted
		AdressBits	(in)	curve with 2 ^ AdressBits values
		
	Return:
		Ptr to icCurveType		successful
		nil						Error

   _____________________________________________________________________ */
icCurveType	*
InvertLut1d ( icCurveType *LookUpTable,
			  UINT8 AdressBits)
{
	unsigned long	i, inCount, outCount;
	unsigned long	intpFirst, intpLast, halfStep, ulAux, target;
	short			monot;
	unsigned short	*inCurve, *outCurve, *usPtr, *stopPtr;
	icCurveType		*outCMcurve = nil;
	double			flFactor;
	OSErr			err = noErr;
	
	LH_START_PROC("InvertLut1d")
	
	if( LookUpTable->base.sig != icSigCurveType	/* 'curv' */ || AdressBits > 15 )
		goto CleanupAndExit;
	
	inCount  = LookUpTable->curve.count;
	inCurve  = LookUpTable->curve.data;
	outCount = 0x1 << AdressBits;
		
	outCMcurve = (icCurveType *)SmartNewPtr( sizeof(OSType)
									  + 2 * sizeof(unsigned long)
									  + outCount * sizeof(unsigned short), &err );
	if(err)
		goto CleanupAndExit;
	
	outCurve = (unsigned short *)outCMcurve->curve.data;
	
   	outCMcurve->base.sig 	= icSigCurveType;	/* 'curv' */
   	outCMcurve->base.reserved[0] = 0x00;
   	outCMcurve->base.reserved[1] = 0x00;
   	outCMcurve->base.reserved[2] = 0x00;
   	outCMcurve->base.reserved[3] = 0x00;
	outCMcurve->curve.count = outCount;
	
	if(inCount < 2)		/* 0 or 1 point in LUT */
	{
		InvLut1dExceptions(inCurve, inCount, outCurve, AdressBits);
		goto CleanupAndExit;
	}
	
		 /* exact matching factor for special values: */
	flFactor = (double)(outCount - 1) / 65535.;
	halfStep = outCount >> 1;		/* lessen computation incorrectness */
	
				/* ascending or descending ? */
	for(monot=0, i=1; i<inCount; i++)
	{
		if(inCurve[i-1] < inCurve[i])
			monot++;
		else if(inCurve[i-1] > inCurve[i])
			monot--;
	}
	
	if(monot >= 0)	/* curve seems to be ascending */
	{
		for(i=1; i<inCount; i++)
			if(inCurve[i-1] > inCurve[i])
				inCurve[i] = inCurve[i-1];
		
		intpFirst = (unsigned long)(inCurve[0] * flFactor + 0.9999);
		intpLast  = (unsigned long)(inCurve[inCount-1] * flFactor);
		
		for(i=0; i<intpFirst; i++)			/* fill lacking area low */
			outCurve[i] = 0;
		for(i=intpLast+1; i<outCount; i++)	/* fill lacking area high */
			outCurve[i] = 0xFFFF;

			/* interpolate remaining values: */
		usPtr   = inCurve;
		stopPtr = inCurve + inCount - 2; /* stops incrementation */
		
		for(i=intpFirst; i<=intpLast; i++)
		{
			target = (0x0FFFF * i + halfStep)  / (outCount - 1);
			while(*(usPtr+1) < target && usPtr < stopPtr)
				usPtr++;					/* find interval */
			
			ulAux = ((unsigned long)(usPtr - inCurve) << 16) / (inCount - 1);
			if(*(usPtr+1) != *usPtr)
			{
				ulAux += ((target - (unsigned long)*usPtr) << 16)
					  / ( (*(usPtr+1) - *usPtr) * (inCount - 1) );
				
				if(ulAux & 0x10000)   /* *(usPtr+1) was required */
					ulAux = 0xFFFF;
			}
			
			outCurve[i] = (unsigned short)ulAux;
		}
	}
	else			/* curve seems to be descending */
	{
		for(i=1; i<inCount; i++)
			if(inCurve[i-1] < inCurve[i])
				inCurve[i] = inCurve[i-1];
		
		intpFirst = (unsigned long)(inCurve[inCount-1] * flFactor + 0.9999);
		intpLast  = (unsigned long)(inCurve[0] * flFactor);
		
		for(i=0; i<intpFirst; i++)			/* fill lacking area low */
			outCurve[i] = 0xFFFF;
		for(i=intpLast+1; i<outCount; i++)	/* fill lacking area high */
			outCurve[i] = 0;

			/* interpolate remaining values: */
		usPtr   = inCurve + inCount - 1;
		stopPtr = inCurve + 1; 		/* stops decrementation */
		
		for(i=intpFirst; i<=intpLast; i++)
		{
			target = (0x0FFFF * i + halfStep)  / (outCount - 1);
			while(*(usPtr-1) < target && usPtr > stopPtr)
				usPtr--;					/* find interval */
			
			ulAux = ((unsigned long)(usPtr-1 - inCurve) << 16) / (inCount - 1);
			if(*(usPtr-1) != *usPtr)
			{
				ulAux += (((unsigned long)*(usPtr-1) - target) << 16)
					  / ( (*(usPtr-1) - *usPtr) * (inCount - 1) );
				
				if(ulAux & 0x10000)
					ulAux = 0x0FFFF;
			}
			
			outCurve[i] = (unsigned short)ulAux;
		}
	}
CleanupAndExit:
	LH_END_PROC("InvertLut1d")
	return(outCMcurve);
}

/* ______________________________________________________________________

	void
		InvLut1dExceptions(	unsigned short	*inCurve,
							unsigned long	inCount,
							unsigned short	*outCurve,
                            UINT8			AdressBits )
	Abstract:
		handles identity and gamma case for LUT inversion

	Params:
		inCurve		(in)	pseudo LUT to be inverted
		inCount		(in)	count of values, 0 (identity) or 1 (gamma)
		outCurve	(out)	inverted LUT
		AdressBits	(in)	2^n values are requested
		
	Return:
		void

   _____________________________________________________________________ */
void
InvLut1dExceptions	( unsigned short *	inCurve,
					  unsigned long 	inCount,
					  unsigned short *	outCurve,
					  UINT8				AdressBits)
{
	unsigned long	i, outCount, step, oldstep, stopit;
	UINT8			shiftBits;
	CM_Doub			invGamma, x, xFactor;
#ifdef DEBUG_OUTPUT
	CMError			err = noErr;
#endif

	LH_START_PROC("InvLut1dExceptions")
	outCount = 0x1 << AdressBits;
	
	if(inCount == 0)	/* identity */
	{
		shiftBits = 16 - AdressBits;
		
		for(i=0; i<outCount; i++)
			outCurve[i] = (unsigned short)( (i << shiftBits)
										  + (i >> AdressBits) );
	}
	else		/* inCount == 1 , gamma */
	{
		invGamma = 256. / (CM_Doub)inCurve[0];
		xFactor  = 1. / (CM_Doub)(outCount - 1);
		
		if(AdressBits <= 6)		/* up to 64 - 2 float.computations */
			step = 1;
		else
			step = 0x1 << (AdressBits - 6);		/* would take too long */
		
		outCurve[0]          = 0;
		outCurve[outCount-1] = 0xFFFF;
		
		for(i=step; i<outCount-1; i+=step)
		{
			x = (CM_Doub)i * xFactor;
			outCurve[i] = (unsigned short)( pow(x,invGamma) * 65535.0);
		}
		
		while(step > 1)		/* fill remaining values successively */
		{
			oldstep = step;
			step  >>= 1;
			
			stopit = outCount - step;	/* last value afterwards */
			
			for(i=step; i<stopit; i+=oldstep)
				outCurve[i] = (unsigned short)( ((long)outCurve[i - step]
										+ (long)outCurve[i + step]) >> 1 );
			
			if(step != 1)
				outCurve[stopit] = (unsigned short)
							( ((long)outCurve[stopit - step] + 0x0FFFF) >> 1 );
		}
		
			/* overwrite sensitive values depending on Gamma */
		if(AdressBits > 6 && invGamma < 1.0)	/* lower part is difficult */
		{
			stopit = 0x1 << (AdressBits - 6);
			
			for(i=1; i<stopit; i++)
			{
				x = (CM_Doub)i * xFactor;
				outCurve[i] = (unsigned short)( pow(x,invGamma) * 65535.0);
			}
		}
	}
	LH_END_PROC("InvLut1dExceptions")
}

/* ______________________________________________________________________

	CMError
		CombiMatrix(icXYZType	*srcColorantData[3],
					icXYZType	*destColorantData[3],
					double		resMatrix[3][3])
	Abstract:
		inverts the 2nd matrix, multiplies it with the 1rst and
		puts the result in resMatrix

	Params:
		srcColorantData		(in)		RGB colorants
		destColorantData	(in)		RGB colorants
		resMatrix			(in/out)
		
	Return:
		noErr		successful

   _____________________________________________________________________ */
CMError
CombiMatrix	( icXYZType srcColorantData[3],
			  icXYZType destColorantData[3],
			  double resMatrix[3][3] )
{
	short		i, j;
	double		straightMat[3][3], invMat[3][3];
	CMError 	err = noErr;

	LH_START_PROC("CombiMatrix")
		/* RGB -> XYZ for first profile: */
	straightMat[0][0] = (double)srcColorantData[0].data.data[0].X;
	straightMat[1][0] = (double)srcColorantData[0].data.data[0].Y;
	straightMat[2][0] = (double)srcColorantData[0].data.data[0].Z;
	
	straightMat[0][1] = (double)srcColorantData[1].data.data[0].X;
	straightMat[1][1] = (double)srcColorantData[1].data.data[0].Y;
	straightMat[2][1] = (double)srcColorantData[1].data.data[0].Z;
	
	straightMat[0][2] = (double)srcColorantData[2].data.data[0].X;
	straightMat[1][2] = (double)srcColorantData[2].data.data[0].Y;
	straightMat[2][2] = (double)srcColorantData[2].data.data[0].Z;
	
		/* RGB -> XYZ for 2nd profile, store in resMatrix prelim.: */
	resMatrix[0][0] = (double)destColorantData[0].data.data[0].X;
	resMatrix[1][0] = (double)destColorantData[0].data.data[0].Y;
	resMatrix[2][0] = (double)destColorantData[0].data.data[0].Z;
	
	resMatrix[0][1] = (double)destColorantData[1].data.data[0].X;
	resMatrix[1][1] = (double)destColorantData[1].data.data[0].Y;
	resMatrix[2][1] = (double)destColorantData[1].data.data[0].Z;
	
	resMatrix[0][2] = (double)destColorantData[2].data.data[0].X;
	resMatrix[1][2] = (double)destColorantData[2].data.data[0].Y;
	resMatrix[2][2] = (double)destColorantData[2].data.data[0].Z;
	
	if( !doubMatrixInvert(resMatrix, invMat) )
	{
#ifdef DEBUG_OUTPUT
        if ( DebugCheck(kThisFile, kDebugErrorInfo) )
            DebugPrint("¥ CombiMatrix-Error: doubMatrixInvert failed \n");
#endif
		err = cmparamErr;
		goto CleanupAndExit;
	}
	
	for(i=0; i<3; i++)
		for(j=0; j<3; j++)
			resMatrix[i][j] = straightMat[i][0] * invMat[0][j]
							+ straightMat[i][1] * invMat[1][j]
							+ straightMat[i][2] * invMat[2][j];
CleanupAndExit:
	LH_END_PROC("CombiMatrix")
	return err;
}

/* ______________________________________________________________________

	Boolean
		doubMatrixInvert(double	MatHin[3][3],
						 double	MatRueck[3][3])
	Abstract:
		inverts MatHin matrix and puts the result in MatRueck

	Params:
		MatHin			(in)			3 * 3 double matrix
		MatRueck		(in/out)		3 * 3 double matrix
		
	Return:
        TRUE		successful

   _____________________________________________________________________ */
Boolean	
doubMatrixInvert(double MatHin[3][3], double MatRueck[3][3])
{
	double	detm, hilf1, hilf2, hilf3, hilf4, hilf5, hilf6;
	double	*a;
	Boolean	success = TRUE;
#ifdef DEBUG_OUTPUT
	CMError err=noErr;
#endif
	LH_START_PROC("doubMatrixInvert")
	a = (double *)MatHin;
	
	hilf1 = a[0] * a[4];
	hilf2 = a[1] * a[5];
	hilf3 = a[2] * a[3];
	hilf4 = a[2] * a[4];
	hilf5 = a[1] * a[3];
	hilf6 = a[0] * a[5];
	
	detm = hilf1 * a[8] + hilf2 * a[6]
		 + hilf3 * a[7] - hilf4 * a[6]
		 - hilf5 * a[8] - hilf6 * a[7];
	
	/*	if(fabs(detm) < 1.E-9) */
	if ( (detm < 1.E-9) && (detm > -1.E-9) )
        success = FALSE;
	else
	{
		detm = 1. / detm;
		
		MatRueck[0][0] = (a[4] * a[8] - a[5] * a[7]) * detm;
		MatRueck[0][1] = (a[7] * a[2] - a[8] * a[1]) * detm;
		MatRueck[0][2] = (hilf2       - hilf4      ) * detm;
	
		MatRueck[1][0] = (a[5] * a[6] - a[3] * a[8]) * detm;
		MatRueck[1][1] = (a[8] * a[0] - a[6] * a[2]) * detm;
		MatRueck[1][2] = (hilf3       - hilf6      ) * detm;
	
		MatRueck[2][0] = (a[3] * a[7] - a[4] * a[6]) * detm;
		MatRueck[2][1] = (a[6] * a[1] - a[7] * a[0]) * detm;
		MatRueck[2][2] = (hilf1       - hilf5      ) * detm;
	}
	
	LH_END_PROC("doubMatrixInvert")
    return(success);
}

/* ______________________________________________________________________
	CMError
		Fill_ushort_ELUT_from_CurveTag(	icCurveType		*pCurveTag,
										unsigned short	*usELUT,
										char			addrBits,
										char			usedBits,
										long 			gridPoints )
	Abstract:
		extracts input luts out of cmSigCurveType tag and converts it
		to desired format: (2 ^ addrBits) values in a range from
		0 to (2 ^ usedBits) - (gridPoints ^ 2)
		NOTE: Memory for the LUTs has to be allocated before !

	Params:
		pCurveTag		(in)		extract input LUT from this
		usELUT			(in/out)	result LUT
		addrBits		(in)		2 ^ addrBits values are requested
		usedBits		(in)		used bits in u.short
		gridPoints		(in)		used for interpolation
		
	Return:
		noErr		successful

   _____________________________________________________________________ */
CMError
Fill_ushort_ELUT_from_CurveTag ( icCurveType	*pCurveTag,
								 unsigned short	*usELUT,
								 char			addrBits,
								 char			usedBits,
								 long 			gridPoints)
{
	long			i, count, indFactor, outFactor, baseInd, maxOut;
	long			fract, lAux, diff, outRound, outShift, interpRound, interpShift;
	unsigned short	*usCurv;
	double			dFactor;
	CMError			err = noErr;
	
	LH_START_PROC("Fill_ushort_ELUT_from_CurveTag")
		/*---special cases:---*/

	if(pCurveTag->curve.count == 0)		/* identity curve */
	{
		err = Fill_ushort_ELUT_identical(usELUT, addrBits, usedBits, gridPoints);
		goto CleanupAndExit;
	}
	
	if(pCurveTag->curve.count == 1)		/* Gamma  curve */
	{
		err = Fill_ushort_ELUT_Gamma(usELUT, addrBits, usedBits, gridPoints, pCurveTag->curve.data[0]);
		goto CleanupAndExit;
	}
		/*---ordinary case:---*/
	
	if(addrBits > 15)
	{
		err = cmparamErr;	/* would lead to overflow */
		goto CleanupAndExit;
	}
	
	count     = 1 << addrBits;
	indFactor = ((pCurveTag->curve.count - 1) << 18) / (count - 1);	/* for adjusting indices */
	
	if(usedBits < 8)
	{
		err = cmparamErr;
		goto CleanupAndExit;
	}
	
	if(gridPoints == 0)
		maxOut = 65535;
	else
		maxOut = ((1L << (usedBits - 8) ) * 256 * (gridPoints - 1)) / gridPoints;
	
		/*-----find factor for the values that fits in 15 bits------*/
		/* (product with 16 bit number must fit in 31 bit uns.long)	*/
		/* n.b: 512 <= maxOut <= 65535  in all possible cases		*/
	
	dFactor  = (double)maxOut / 65535.;		/* 65535 is max. curve value */
	dFactor *= 4194304.;					/* same as << 22, certainly too much */
	
	outFactor = (long)dFactor;
	outRound  = (1L << 21) - 1;
	outShift  = 22;
	while(outFactor & 0xFFF8000)	/* stay within 15 bits to prevent product overflow */
	{
		outFactor >>= 1;
		outRound  >>= 1;
		outShift   -= 1;
	}
	
	interpRound = outRound >> 1;	/* with interpolation we have an additional...	*/
	interpShift = outShift  - 1;	/* ... >> 1 because we must add two nunmbers	*/
	
	usCurv = pCurveTag->curve.data;
	
	for(i=0; i<count; i++)
	{
		lAux    = (i * indFactor+4) >> 3;
		baseInd = (unsigned long)lAux >> 15;
		fract   = lAux & 0x7FFF;		/* 15 bits for interpolation */
		
		if(fract)		/* interpolation necessary ? */
		{
			lAux = (long)usCurv[baseInd] * outFactor >> 1;
			
			diff = (long)usCurv[baseInd+1] - (long)usCurv[baseInd];
			diff = (diff * outFactor >> 15) * fract >> 1;

			usELUT[i] = (unsigned short)( (lAux + diff + interpRound) >> interpShift );
		}
		else
			usELUT[i] = (unsigned short)( ((long)usCurv[baseInd]
												* outFactor + outRound) >> outShift );
	}

CleanupAndExit:	
	LH_END_PROC("Fill_ushort_ELUT_from_CurveTag")
	return(noErr);
}

/* ______________________________________________________________________
   _____________________________________________________________________ */
CMError
Fill_ushort_ELUT_identical	( UINT16 *usELUT,
							  char addrBits,
							  char usedBits,
							  long gridPoints )
{
	long		i, count, factor, maxOut;
	UINT16		*myWPtr;
#ifdef DEBUG_OUTPUT
	CMError err = noErr;
#endif
	LH_START_PROC("Fill_ushort_ELUT_identical")
		
	count  = 1 << addrBits;
	
	if(gridPoints == 0)
		maxOut = 65535;
	else
		maxOut = ((1L << (usedBits - 8) ) * 256 * (gridPoints - 1)) / gridPoints;
	
	factor = (maxOut << 14) / (count - 1);
	
	myWPtr = usELUT;
	for(i=0; i<count; i++)
		*myWPtr++ = (UINT16)((i * factor + 0x2000) >> 14);
	
	LH_END_PROC("Fill_ushort_ELUT_identical")
	return(noErr);
}

/* ______________________________________________________________________
   _____________________________________________________________________ */
CMError
Fill_ushort_ELUT_Gamma	( unsigned short*	usELUT,
						  char				addrBits,
						  char				usedBits,
						  long				gridPoints,
						  unsigned short	gamma_u8_8 )
{
	unsigned long	i, j, outCount, step, stopit;
	CM_Doub			gamma, x, xFactor;
	long			leftVal, Diff, lAux, maxOut;
#ifdef DEBUG_OUTPUT
	CMError			err=noErr;
#endif

	LH_START_PROC("Fill_ushort_ELUT_Gamma")
	outCount = 0x1 << addrBits;
	if(gridPoints == 0)
		maxOut = 65535;
	else
		maxOut = ((1L << (usedBits - 8) ) * 256 * (gridPoints - 1)) / gridPoints;
	
	gamma   = ((CM_Doub)gamma_u8_8 * 3.90625E-3);	/* / 256.0 */
	xFactor = 1. / (CM_Doub)(outCount - 1);
	
	if(addrBits <= 6)		/* up to 64 - 2 float.computations */
		step = 1;
	else
		step = 0x1 << (addrBits - 6);		/* would take too long */
	
	usELUT[0]          = 0;
    usELUT[outCount-1] = (UINT16)maxOut;
	
	for(i=step; i<outCount-1; i+=step)
	{
		x         = (CM_Doub)i * xFactor;
		usELUT[i] = (unsigned short)( pow(x,gamma) * maxOut);
	}
	
		/*---fill intervals - except for last, which is odd:---*/
	for(i=0; i<outCount-step; i+=step)
	{
		leftVal = (long)usELUT[i];
		Diff    = (long)usELUT[i + step] - leftVal;
			
		for(j=1; j<step; j++)
		{
			lAux = ( (Diff * j << 8) / step + 128 ) >> 8;

			usELUT[i + j] = (unsigned short)(leftVal + lAux);
		}
	}
	
		/*---fill last interval:---*/
	i       = outCount - step;
	leftVal = (long)usELUT[i];
	Diff    = maxOut - leftVal;		/* maxOut for 1.0 */
		
	for(j=1; j<step-1; j++)		/* stops here if step <= 2 */
	{
		lAux = ( (Diff * j << 8) / (step - 1) + 128 ) >> 8;

		usELUT[i + j] = (unsigned short)(leftVal + lAux);
	}
	
		/* overwrite sensitive values depending on Gamma */
	if(addrBits > 6 && gamma < 1.0)	/* lower part is difficult */
	{
		stopit = 0x1 << (addrBits - 6);
		
		for(i=1; i<stopit; i++)
		{
			x = (CM_Doub)i * xFactor;
			usELUT[i] = (unsigned short)( pow(x,gamma) * maxOut);
		}
	}

	LH_END_PROC("Fill_ushort_ELUT_Gamma")
	return(noErr);
}


/* ______________________________________________________________________

	CMError
		Fill_inverse_byte_ALUT_from_CurveTag(	icCurveType		*pCurveTag,
												unsigned char	*ucALUT,
												char			addrBits )
	Abstract:
		extracts output luts out of cmSigCurveType tag and converts them
		to desired format: (2 ^ addrBits) values in a range from 0 to 255
		NOTE: not-monotone CurveTags are manipulated
		NOTE: Memory for the LUT has to be allocated before !

	Params:
		pCurveTag		(in)		extract input LUT from this
		ucALUT			(in/out)	result LUT
		addrBits		(in)		2 ^ addrBits values are requested
		
	Return:
		noErr		successful

   _____________________________________________________________________ */
CMError
Fill_inverse_byte_ALUT_from_CurveTag	( icCurveType	*pCurveTag,
										  unsigned char	*ucALUT,
										  char			addrBits )
{
	unsigned long	i, inCount, outCount;
	unsigned long	intpFirst, intpLast, halfStep, ulAux, target;
	short			monot;
	unsigned short	*inCurve, *usPtr, *stopPtr;
	double			flFactor;
	char			baseShift;
	unsigned long	clipIndex;
	CMError			err = noErr;

	LH_START_PROC("Fill_inverse_byte_ALUT_from_CurveTag")

    if( pCurveTag->base.sig != icSigCurveType	/* 'curv' */
     || addrBits > 15 )
    {
#ifdef DEBUG_OUTPUT
        if ( DebugCheck(kThisFile, kDebugErrorInfo) )
            DebugPrint("¥ Fill_inverse_byte_ALUT_from_CurveTag ERROR:  addrBits = %d\n",addrBits);
#endif
		err = cmparamErr;
		goto CleanupAndExit;
	}
	
	outCount = 0x1 << addrBits;

		/*---special cases:---*/

	if(pCurveTag->curve.count == 0)		/*---identity---*/
	{
		baseShift = addrBits - 8;		/* >= 0, we need at least 256 values */
		
		for(i=0; i<outCount; i++)
			ucALUT[i] = (unsigned char)(i >> baseShift);
	
		goto CleanupAndExit;
	}
	else if(pCurveTag->curve.count == 1)	/*---gamma curve---*/
	{
		Fill_inverseGamma_byte_ALUT(ucALUT, addrBits, pCurveTag->curve.data[0]);
		goto CleanupAndExit;
	}
	
		/*---ordinary case:---*/
	
	inCount = pCurveTag->curve.count;
	inCurve = pCurveTag->curve.data;
		
		 /* exact matching factor needed for special values: */
	clipIndex = (1 << addrBits) - (1 << (addrBits - 8)); /* max XLUT output with 10 bit is at 1020, not at1023 */
	flFactor  = (double)clipIndex / 65535.;
	
	halfStep = outCount >> 1;		/* lessen computation incorrectness */
	
				/* ascending or descending ? */
	for(monot=0, i=1; i<inCount; i++)
	{
		if(inCurve[i-1] < inCurve[i])
			monot++;
		else if(inCurve[i-1] > inCurve[i])
			monot--;
	}
	
	if(monot >= 0)	/* curve seems to be ascending */
	{
		for(i=1; i<inCount; i++)
			if(inCurve[i-1] > inCurve[i])
				inCurve[i] = inCurve[i-1];
		
		intpFirst = (unsigned long)(inCurve[0] * flFactor + 0.9999);
		intpLast  = (unsigned long)(inCurve[inCount-1] * flFactor);
		
		for(i=0; i<intpFirst; i++)			/* fill lacking area low */
			ucALUT[i] = 0;
		for(i=intpLast+1; i<outCount; i++)	/* fill lacking area high */
			ucALUT[i] = 0xFF;

			/* interpolate remaining values: */
		usPtr   = inCurve;
		stopPtr = inCurve + inCount - 2; /* stops incrementation */
		
		for(i=intpFirst; i<=intpLast; i++)
		{
			target = (0x0FFFF * i + halfStep)  / (outCount - 1);
			while(*(usPtr+1) < target && usPtr < stopPtr)
				usPtr++;					/* find interval */
			
			ulAux = ((unsigned long)(usPtr - inCurve) << 16) / (inCount - 1);
			if(*(usPtr+1) != *usPtr)
			{
				ulAux += ((target - (unsigned long)*usPtr) << 16)
					  / ( (*(usPtr+1) - *usPtr) * (inCount - 1) );
				
				if(ulAux & 0x10000)   /* *(usPtr+1) was required */
					ulAux = 0x0FFFF;
			}
			
			ucALUT[i] = (unsigned char)(ulAux >> 8);
		}
	}
	else			/* curve seems to be descending */
	{
		for(i=1; i<inCount; i++)
			if(inCurve[i-1] < inCurve[i])
				inCurve[i] = inCurve[i-1];
		
		intpFirst = (unsigned long)(inCurve[inCount-1] * flFactor + 0.9999);
		intpLast  = (unsigned long)(inCurve[0] * flFactor);
		
		for(i=0; i<intpFirst; i++)			/* fill lacking area low */
			ucALUT[i] = 0xFF;
		for(i=intpLast+1; i<outCount; i++)	/* fill lacking area high */
			ucALUT[i] = 0;

			/* interpolate remaining values: */
		usPtr   = inCurve + inCount - 1;
		stopPtr = inCurve + 1; 		/* stops decrementation */
		
		for(i=intpFirst; i<=intpLast; i++)
		{
			target = (0x0FFFF * i + halfStep)  / (outCount - 1);
			while(*(usPtr-1) < target && usPtr > stopPtr)
				usPtr--;					/* find interval */
			
			ulAux = ((unsigned long)(usPtr-1 - inCurve) << 16) / (inCount - 1);
			if(*(usPtr-1) != *usPtr)
			{
				ulAux += (((unsigned long)*(usPtr-1) - target) << 16)
					  / ( (*(usPtr-1) - *usPtr) * (inCount - 1) );
				
				if(ulAux & 0x10000)
					ulAux = 0x0FFFF;
			}
			
            ucALUT[i] = (unsigned char)(ulAux >> 8);
		}
	}

CleanupAndExit:	
	LH_END_PROC("Fill_inverse_byte_ALUT_from_CurveTag")
	return(noErr);
}

/*   _____________________________________________________________________ */
void
Fill_inverseGamma_byte_ALUT ( unsigned char	*	ucALUT,
							  char				addrBits,
							  unsigned short	gamma_u8_8 )
{
	unsigned long	i, j, outCount, step, stopit;
	long			leftVal, Diff, lAux;
	CM_Doub			invGamma, x, xFactor;
    unsigned long	clipIndex;
#ifdef DEBUG_OUTPUT
	CMError			err=noErr;
#endif

	LH_START_PROC("Fill_inverseGamma_byte_ALUT")

	outCount = 0x1 << addrBits;
	
	invGamma  = 256. / (CM_Doub)gamma_u8_8;
	clipIndex = (1 << addrBits) - (1 << (addrBits - 8)); /* max XLUT output with 10 bit is at 1020, not at1023 */
	xFactor   = 1. / (CM_Doub)clipIndex;
	
	if(addrBits <= 6)		/* up to 64 - 2 float.computations */
		step = 1;
	else
		step = 0x1 << (addrBits - 6);		/* more would take too long */
	
	ucALUT[0]          = 0;			/* these two...	*/
	ucALUT[outCount-1] = 0xFF;		/* ...are fixed	*/
	
	for(i=step; i<outCount-1; i+=step)
	{
		x = (CM_Doub)i * xFactor;
		if(x > 1.)
			x = 1.;		/* clipping in the end of ALUT */
		
		ucALUT[i] = (unsigned char)( pow(x,invGamma) * 255.0 + 0.5);
	}
	
		/*---fill intervals - except for last, which is odd:---*/
	for(i=0; i<outCount-step; i+=step)
	{
		leftVal = (long)ucALUT[i];
		Diff    = (long)ucALUT[i + step] - leftVal;
			
		for(j=1; j<step; j++)
		{
			lAux = ( (Diff * j << 8) / step + 128 ) >> 8;

			ucALUT[i + j] = (unsigned char)(leftVal + lAux);
		}
	}
	
		/*---fill last interval:---*/
	i       = outCount - step;
	leftVal = (long)ucALUT[i];
	Diff    = 0x0FF - leftVal;	/* 0xFF for 1.0 */
		
	for(j=1; j<step-1; j++)		/* stops here if step <= 2 */
	{
		lAux = ( (Diff * j << 8) / (step - 1) + 128 ) >> 8;

		ucALUT[i + j] = (unsigned char)(leftVal + lAux);
	}
	
		/*--overwrite sensitive values depending on Gamma:--*/
	if(addrBits > 6 && invGamma < 1.0)		/* ...if lower part is difficult */
	{
		stopit = 0x1 << (addrBits - 6);
		
		for(i=1; i<stopit; i++)
		{
			x         = (CM_Doub)i * xFactor;
			ucALUT[i] = (unsigned char)( pow(x,invGamma) * 255.0);
		}
	}
	LH_END_PROC("Fill_inverseGamma_byte_ALUT")
}

/* ______________________________________________________________________

	CMError
	Fill_ushort_ELUTs_from_lut8Tag ( CMLutParamPtr	theLutData,
									 Ptr			profileELuts,
									 char			addrBits,
									 char			usedBits,
									 long			gridPoints )
	Abstract:
		extracts input luts out of CMlut8Type tag and converts them
		to desired format: (2 ^ addrBits) values in a range from
		0 to (2 ^ usedBits) - (gridPoints ^ 2)

	Params:
		theLutData			(in/out)	Ptr to structure that holds all the luts...
		profileELuts		(in)		Ptr to the profile's input luts
		addrBits			(in)		2 ^ addrBits values are requested
		usedBits			(in)		used bits in u.short
		gridPoints			(in)		used for interpolation

	Return:
		noErr		successful

   _____________________________________________________________________ */
CMError
Fill_ushort_ELUTs_from_lut8Tag ( CMLutParamPtr	theLutData,
								 Ptr			profileELuts,
								 char			addrBits,
								 char			usedBits,
								 long			gridPoints )
{
	long			i, j, maxOut;
	UINT8			*curInLut;
	UINT16			*curELUT;
	long			count, indFactor, outFactor, baseInd, fract, lAux, diff;
	LUT_DATA_TYPE	localElut = nil;
	UINT16			*localElutPtr;
	OSErr			err = noErr;
	long			theElutSize;
	
	LH_START_PROC("Fill_ushort_ELUTs_from_lut8Tag")
	
	count       = 1 << addrBits;
	theElutSize = theLutData->colorLutInDim * count * sizeof (UINT16);
	localElut   = ALLOC_DATA(theElutSize + 2, &err);
	if (err)
		goto CleanupAndExit;
	
	indFactor = (255 << 12) / (count - 1);	/* for adjusting indices */
	
	if(gridPoints == 0)
		maxOut = 65535;
	else
		maxOut = ((1L << (usedBits - 8) ) * 256 * (gridPoints - 1)) / gridPoints;
	outFactor = (maxOut << 12) / 255;			/* 255 is max. value from mft1 output lut */
	
	LOCK_DATA(localElut);
	localElutPtr = (UINT16*)DATA_2_PTR(localElut);
	for(i=0; i<theLutData->colorLutInDim; i++)
	{
		curInLut = (UINT8*)profileELuts + (i << 8);
		curELUT  = localElutPtr + i * count;
		
		for(j=0; j<count; j++)
		{
			lAux    = j * indFactor;
			baseInd = lAux >> 12;
			fract   = (lAux & 0x0FFF) >> 4; /* leaves 8 bits for interpolation as with distortion */

			if(fract && baseInd != 255)		/* interpolation necessary ? */
			{
				lAux = (long)curInLut[baseInd] * outFactor >> 6;
				
				diff = (long)curInLut[baseInd+1] - (long)curInLut[baseInd];
				diff = (diff * outFactor >> 6) * fract >> 8;

				curELUT[j] = (UINT16)( (lAux + diff + 32) >> 6 );
			}
			else
				curELUT[j] = (UINT16)( ((long)curInLut[baseInd]
													* outFactor + 0x0800) >> 12 );
		}
	}
	UNLOCK_DATA(localElut);
	theLutData->inputLut = localElut;
	localElut = nil;
CleanupAndExit:
	localElut = DISPOSE_IF_DATA(localElut);
	LH_END_PROC("Fill_ushort_ELUTs_from_lut8Tag")
	return err;
}

/* ______________________________________________________________________

	CMError
	Fill_byte_ALUTs_from_lut8Tag( CMLutParamPtr	theLutData,
								  Ptr			profileALuts,
								  char 			addrBits )
	Abstract:
		extracts output luts out of CMLut8Type tag and converts them
		to desired format: (2 ^ addrBits) values in a range from 0 to 255

	Params:
		theLutData			(in/out)	Ptr to structure that holds all the luts...
		profileALuts		(in)		Ptr to the profile's output luts
		addrBits			(in)		2 ^ addrBits values are requested
		
	Return:
		noErr		successful

   _____________________________________________________________________ */
CMError
Fill_byte_ALUTs_from_lut8Tag( CMLutParamPtr	theLutData,
							  Ptr			profileALuts,
							  char 			addrBits )
{
	long			i, j;
	UINT8			*curOutLut;
	UINT8			*profAluts = (UINT8*)profileALuts;
	UINT8			*curALUT;
	long			count, clipIndex;
	long			factor, fract, baseInd, lAux;
	OSErr			err = noErr;
	LUT_DATA_TYPE	localAlut = nil;
	UINT8			*localAlutPtr;
	long			theAlutSize;
	
	LH_START_PROC("Fill_byte_ALUTs_from_lut8Tag")

	count     = 1 << addrBits;								/* addrBits is always >= 8 */
	clipIndex = (1 << addrBits) - (1 << (addrBits - 8));	/* max XLUT output with 10 bit is at 1020, not at1023 */
	
	theAlutSize = theLutData->colorLutOutDim * count;
	localAlut   = ALLOC_DATA(theAlutSize + 1, &err);
	if (err)
		goto CleanupAndExit;
	
	LOCK_DATA(localAlut);
	localAlutPtr = (UINT8*)DATA_2_PTR(localAlut);
	
	factor = (255 << 12) / clipIndex;		/* for adjusting the indices */
	
	for(i=0; i<theLutData->colorLutOutDim; i++)
	{
		curOutLut = profAluts + (i << 8);
		curALUT   = localAlutPtr + i * count;
		
		for(j=0; j<=clipIndex; j++)
		{
			lAux    = j * factor;
			baseInd = lAux >> 12;
			fract   = lAux & 0x0FFF;
			
			if(fract)
			{
				lAux = (long)curOutLut[baseInd + 1] - (long)curOutLut[baseInd];
				lAux = (lAux * fract + 0x0800) >> 12;
				
				curALUT[j] = (UINT8)((long)curOutLut[baseInd] + lAux);
			}
			else
				curALUT[j] = curOutLut[baseInd];
		}
		
		for(j=clipIndex+1; j<count; j++)		/* unused indices, clip these */
			curALUT[j] = curALUT[clipIndex];
	}
	
	UNLOCK_DATA(localAlut);
	theLutData->outputLut = localAlut;
	localAlut = nil;
CleanupAndExit:
	localAlut = DISPOSE_IF_DATA(localAlut);
	LH_END_PROC("Fill_byte_ALUTs_from_lut8Tag")
	return err;
}

/* ______________________________________________________________________

	CMError
	Fill_ushort_ELUTs_from_lut16Tag( CMLutParamPtr	theLutData,
									 Ptr			profileELuts,
									 char			addrBits,
									 char			usedBits,
									 long			gridPoints,
									 long			inputTableEntries )

	Abstract:
		extracts input luts out of CMLut16Type tag and converts them
		to desired format: (2 ^ addrBits) values in a range from
		0 to (2 ^ usedBits) - (gridPoints ^ 2)

	Params:
		theLutData			(in/out)	Ptr to structure that holds all the luts...
		profileELuts		(in)		Ptr to the profile's input luts
		addrBits			(in)		2 ^ addrBits values are requested
		usedBits			(in)		used bits in u.short
		gridPoints			(in)		used for interpolation
		inputTableEntries	(in)		number of entries in the input lut (up to 4096)

	Return:
		noErr		successful

   _____________________________________________________________________ */
CMError
Fill_ushort_ELUTs_from_lut16Tag( CMLutParamPtr	theLutData,
								 Ptr			profileELuts,
								 char			addrBits,
								 char			usedBits,
								 long			gridPoints,
								 long			inputTableEntries )
{
	long			i, j, inTabLen, maxOut;
	UINT16			*curInLut;
	UINT16			*curELUT;
	UINT16			*profELUT = (UINT16*)profileELuts;
	long			count, outFactor, fract, lAux, diff;
	long			baseInd, indFactor;
	long			outRound, outShift, interpRound, interpShift;
	double			dFactor;
	long			theElutSize;
	LUT_DATA_TYPE	localElut = nil;
	UINT16			*localElutPtr;
	OSErr			err = noErr;

	LH_START_PROC("Fill_ushort_ELUTs_from_lut16Tag")
	
	count     = 1 << addrBits;
	inTabLen  = inputTableEntries;

	theElutSize = theLutData->colorLutInDim * count * sizeof (UINT16);
	localElut   = ALLOC_DATA(theElutSize + 2, &err);
	if(err)
		goto CleanupAndExit;
	
	indFactor = ((inTabLen - 1) << 18) / (count - 1);	/* for adjusting indices */

	if(gridPoints == 0)
		maxOut = 65535;
	else
		maxOut = ((1L << (usedBits - 8) ) * 256 * (gridPoints - 1)) / gridPoints;

		/*-----find factor for the values that fits in 15 bits------*/
		/* (product with 16 bit number must fit in 31 bit uns.long)	*/
		/* n.b: 512 <= maxOut <= 65535  in all possible cases		*/
	
	dFactor  = (double)maxOut / 65535.;		/* 65535 is max. curve value */
	dFactor *= 4194304.;					/* same as << 22, certainly too much */
	
	outFactor = (long)dFactor;
	outRound  = (1L << 21) - 1;
	outShift  = 22;
	while(outFactor & 0xFFF8000)	/* stay within 15 bits to prevent product overflow */
	{
		outFactor >>= 1;
		outRound  >>= 1;
		outShift   -= 1;
	}
	
	interpRound = outRound >> 1;	/* with interpolation we have an additional...	*/
	interpShift = outShift  - 1;	/* ... >> 1 because we must add two nunmbers	*/
	
	LOCK_DATA(localElut);
	localElutPtr = (UINT16*)DATA_2_PTR(localElut);

	for(i=0; i<theLutData->colorLutInDim; i++)
	{
		curInLut = profELUT + (i * inTabLen);
		curELUT  = localElutPtr + (i * count);

		for(j=0; j<count; j++)
		{
			lAux    = (j * indFactor+4) >> 3;
			baseInd = (unsigned long)lAux >> 15;
			fract   = lAux & 0x7FFF;	/* 15 bits for interpolation */

			if(fract)		/* interpolation necessary ? */
			{
				lAux = (long)curInLut[baseInd] * outFactor >> 1;
				
				diff = (long)curInLut[baseInd+1] - (long)curInLut[baseInd];
				diff = ((diff * outFactor >> 15) * fract) >> 1;

				curELUT[j] = (UINT16)( (lAux + diff + interpRound) >> interpShift );
			}
			else
				curELUT[j] = (UINT16)( ((long)curInLut[baseInd] * outFactor
													+ outRound) >> outShift );
		}
	}
	UNLOCK_DATA(localElut);
	theLutData->inputLut = localElut;
	localElut = nil;
CleanupAndExit:	
	localElut = DISPOSE_IF_DATA(localElut);	
	LH_END_PROC("Fill_ushort_ELUTs_from_lut16Tag")
	return err;
}

/* ______________________________________________________________________

	CMError
	Fill_byte_ALUTs_from_lut16Tag(	CMLutParamPtr	theLutData,
									Ptr				profileALuts,
									char			addrBits,
								    long			outputTableEntries )
	Abstract:
		extracts output luts out of CMLut16Type tag and converts them
		to desired format: (2 ^ addrBits) values in a range from 0 to 255

	Params:
		theLutData			(in/out)	Ptr to structure that holds all the luts...
		profileALuts		(in)		Ptr to the profile's output luts
		addrBits			(in)		2 ^ addrBits values are requested
		outputTableEntries	(in)		number of entries in the output lut (up to 4096)
		
	Return:
		noErr		successful

   _____________________________________________________________________ */
CMError
Fill_byte_ALUTs_from_lut16Tag(	CMLutParamPtr	theLutData,
								Ptr				profileALuts,
								char			addrBits,
							    long			outputTableEntries )
{
	long			i, j;
	UINT16			*curOutLut;
	UINT8			*curALUT;
	long			count, clipIndex, outTabLen;
	long			indFactor, fract, baseInd, lAux;
	UINT16			*profALUTs = (UINT16*)profileALuts;
	OSErr			err = noErr;
	LUT_DATA_TYPE	localAlut = nil;
	UINT8			*localAlutPtr;
	long			theAlutSize;
	
	LH_START_PROC("Fill_byte_ALUTs_from_lut16Tag")
	
	count     = 1 << addrBits;								/* addrBits is always >= 8 */
	clipIndex = (1 << addrBits) - (1 << (addrBits - 8));	/* max XLUT output with 10 bit is at 1020, not at1023 */

	theAlutSize = theLutData->colorLutOutDim * count;
	localAlut   = ALLOC_DATA(theAlutSize + 1, &err);
	if (err)
		goto CleanupAndExit;
	
	outTabLen = outputTableEntries;					/* <= 4096 */
	indFactor = ((outTabLen - 1) << 18) / clipIndex;	/* for adjusting the indices */
	
	LOCK_DATA(localAlut);
	localAlutPtr = (UINT8*)DATA_2_PTR(localAlut);
	
	for(i=0; i<theLutData->colorLutOutDim; i++)
	{
		curOutLut = profALUTs + i * outTabLen;
		curALUT   = localAlutPtr + i * count;
		
		for(j=0; j<=clipIndex; j++)
		{
			lAux    = (j * indFactor+32) >> 6;
			baseInd = lAux >> 12;
			fract   = lAux & 0x0FFF;
			
			if(fract)
			{
				lAux = (long)curOutLut[baseInd + 1] - (long)curOutLut[baseInd];
				lAux = (lAux * fract + 0x0800) >> 12;
				
				curALUT[j] = (UINT8)(((long)curOutLut[baseInd] + lAux) >> 8);
			}
			else
				curALUT[j] = curOutLut[baseInd] >> 8;
		}
		
		for(j=clipIndex+1; j<count; j++)		/* unused indices, clip these */
			curALUT[j] = curALUT[clipIndex];
	}
	
	UNLOCK_DATA(localAlut);
	theLutData->outputLut = localAlut;
	localAlut = nil;
CleanupAndExit:
	localAlut = DISPOSE_IF_DATA(localAlut);
	
	LH_END_PROC("Fill_byte_ALUTs_from_lut16Tag")
	return err;
}

/* ______________________________________________________________________

	CMError
		MakeGamut16or32ForMonitor(	icXYZType		*pRedXYZ,
									icXYZType		*pGreenXYZ,
									icXYZType		*pBlueXYZ,
									unsigned short	**ppELUTs,
                                   UINT8			**ppXLUT,
                                   UINT8			**ppALUT,
									Boolean			cube32Flag )
	Abstract:
		Computes 3 ELUTs, XLUT and ALUT for gamut checking out of the
		3 monitor primaries. Color space is XYZ
		NOTE: Memory for the ELUTs, XLUt and ALUT is allocated here !

	Params:
		pRedXYZ			(in)		-> red primary of monitor
		pGreenXYZ		(in)		-> green primary of monitor
		pBlueXYZ		(in)		-> blue primary of monitor
		ppELUTs			(out)		3 input LUTs
		ppXLUT			(out)		3 dimensional byte Lut (32^3)
		ppALUT			(out)		Boolean output LUT (1024 bytes)
        cube32Flag		(in)		TRUE: 32*32*32 points, FALSE: 16*16*16 points
		
	Return:
		noErr		successful

   _____________________________________________________________________ */

CMError MakeGamut16or32ForMonitor	( 	icXYZType		*pRedXYZ,
							 			icXYZType		*pGreenXYZ,
							  			icXYZType		*pBlueXYZ,
							  			CMLutParamPtr	theLutData,
							  			Boolean			cube32Flag )

{
	double			XYZmatrix[3][3], RGBmatrix[3][3];
	double			sum, dFactor;
	long			i, j, k, gridPoints, planeCount, totalCount, longMat[9];
	long			longX, longY, longZ, longR, longG, longB;
	long			*lPtr, lFactor, maxOut;
	unsigned short	*usPtr;
	unsigned char	*XPtr;
	LUT_DATA_TYPE	tempXLutHdl	= nil;
	LUT_DATA_TYPE	tempELutHdl	= nil;
	LUT_DATA_TYPE	tempALutHdl	= nil;
	unsigned char   *tempXLut 	= nil;
	unsigned char   *tempALut 	= nil;
	unsigned short  *tempELut 	= nil;
	unsigned short	Levels[32];
	OSErr			err = noErr;
	
	LH_START_PROC("MakeGamut16or32ForMonitor")
	
	if(theLutData->inputLut != nil || theLutData->colorLut != nil || theLutData->outputLut != nil)
	{
		err = cmparamErr;
		goto CleanupAndExit;
	}
	

	/*----------------------------------------------------------------------------------------- E */
	tempELutHdl = ALLOC_DATA(adr_bereich_elut * 3 * sizeof(unsigned short) + 2, &err);  /* +2 extra space for Interpolation */
	if(err)
		goto CleanupAndExit;
	LOCK_DATA(tempELutHdl);
	tempELut = (unsigned short *)DATA_2_PTR(tempELutHdl);
	

  /*----------------------------------------------------------------------------------------- X */
	if(cube32Flag)
		gridPoints = 32;			/* for cube grid */
	else
		gridPoints = 16;
	totalCount = gridPoints * gridPoints * gridPoints;
  totalCount += 1 + gridPoints + gridPoints * gridPoints; /* extra space for Interpolation */

#ifdef ALLOW_MMX
			totalCount+=3;	/* +1 for MMX 4 Byte access */
#endif

	tempXLutHdl = ALLOC_DATA(totalCount, &err);
	if(err)
		goto CleanupAndExit;
	LOCK_DATA(tempXLutHdl);
	tempXLut = (unsigned char *)DATA_2_PTR(tempXLutHdl);
	
	
  /*----------------------------------------------------------------------------------------- A */
	tempALutHdl = ALLOC_DATA(adr_bereich_alut + 1, &err);   /* +1 extra space for Interpolation */
	if(err)
		goto CleanupAndExit;
	LOCK_DATA(tempALutHdl);
	tempALut = (unsigned char *)DATA_2_PTR(tempALutHdl);
	
		/*---------fill 3 ELUTs for X, Y, Z (256 u.shorts each):--------------------*/
		/* linear curve with clipping, slope makes white value  become				*/
		/* max * 30.5/31 or max * 14.5/15, that is half of the last XLUT interval	*/
	if(cube32Flag)
		dFactor = 30.5 / 31.;
	else
		dFactor = 14.5 / 15.;
	
	maxOut = ((1L << (16 /*usedBits*/ - 8) ) * 256 * (gridPoints - 1)) / gridPoints;
	
	for(i=0; i<3; i++)		/* X, Y, Z ELUTs */
	{
		if(i == 0)
			lFactor = (long)( dFactor * 2. * maxOut * 256. / 255. / 0.9642 );/* X, adjust D50 */
		else if(i == 1)
			lFactor = (long)( dFactor * 2. * maxOut * 256. / 255.);			/* Y */
		else
			lFactor = (long)( dFactor * 2. * maxOut * 256. / 255. / 0.8249);/* Z, adjust D50 */
		
		usPtr = tempELut + 256 * i;
		for(j=0; j<256; j++)
		{
			k = (j * lFactor + 127) >> 8;
			if(k > maxOut)
				k = maxOut;		/* max. ELUT value */
			
			*usPtr++ = (unsigned short)k;
		}
	}

		/*------ RGB to XYZ matrix in the range 0.0 to 1.0 -----*/
		/* floating point for accurate inversion				*/
	XYZmatrix[0][0] = (double)pRedXYZ->data.data[0].X;
	XYZmatrix[1][0] = (double)pRedXYZ->data.data[0].Y;
	XYZmatrix[2][0] = (double)pRedXYZ->data.data[0].Z;

	XYZmatrix[0][1] = (double)pGreenXYZ->data.data[0].X;
	XYZmatrix[1][1] = (double)pGreenXYZ->data.data[0].Y;
	XYZmatrix[2][1] = (double)pGreenXYZ->data.data[0].Z;

	XYZmatrix[0][2] = (double)pBlueXYZ->data.data[0].X;
	XYZmatrix[1][2] = (double)pBlueXYZ->data.data[0].Y;
	XYZmatrix[2][2] = (double)pBlueXYZ->data.data[0].Z;
	
		/*--- grey with R = G = B (D50 adjustment is done by the ELUTs) ----*/
	for(i=0; i<3; i++)
	{
		sum = XYZmatrix[i][0] + XYZmatrix[i][1] + XYZmatrix[i][2];
		if(sum < 0.1)
			sum = 0.1;	/* prevent from div. by 0 (bad profiles) */
		
		for(j=0; j<3; j++)
			XYZmatrix[i][j] /= sum;
	}
	
		/*---XYZ to RGB matrix:---*/
	if(!doubMatrixInvert(XYZmatrix, RGBmatrix))
	{
		err = cmparamErr;
		goto CleanupAndExit;
	}
	
	for(i=0; i<3; i++)				/* create integer format for speed, */
		for(j=0; j<3; j++)			/* 1.0 becomes 2^13, works for coeff. up to 8. */
			longMat[3*i + j] = (long)(RGBmatrix[i][j] * 8192.);
	
		/*-----grid levels for cube grid in XYZ (16 bit) so ----*/
		/* that white is at half of last interval, so the last	*/
		/* value is white * 15/14.5 or white * 31/30.5			*/
	if(cube32Flag)
		dFactor = 32768. / 30.5 * 31. / (gridPoints - 1);
	else
		dFactor = 32768. / 14.5 * 15. / (gridPoints - 1);

	for(i=0; i<gridPoints; i++)			/* n.b: 32 is max. possible gridPoints */
		Levels[i] = (unsigned short)(i * dFactor + 0.5);
	
		/*----special treatment of first and last plane for speed:----*/
	planeCount = gridPoints * gridPoints;
	XPtr       = tempXLut;
	for(i=0; i<planeCount; i++)
		*XPtr++ = 255;		/* out of gamut */
	
	XPtr = tempXLut + (gridPoints - 1) * planeCount;
	for(i=0; i<planeCount; i++)
		*XPtr++ = 255;		/* out of gamut */
	
	*tempXLut = 0;	/* set black (white is between last 2 planes) */

		/*----second to second last plane must be computed:-----*/
		/*  transform points to RGB and judge in/out			*/
	XPtr = tempXLut + planeCount;
		
	for(i=1; i<gridPoints-1; i++)
		for(j=0; j<gridPoints; j++)
			for(k=0; k<gridPoints; k++)
			{
				longX = (long)Levels[i];		/* X */
				longY = (long)Levels[j];		/* Y */
				longZ = (long)Levels[k];		/* Z */
				
					/* matrix coeff: 2^13 is 1.0 , XYZ values: 1.0 or 100. is 2^15 ;	*/
					/* -> mask for products < 0 and >= 2^28 is used for in/out checking	*/
				
				longR = longX * longMat[0] + longY * longMat[1] + longZ * longMat[2];
				if(longR & 0xF0000000)
					*XPtr++ = 255;		/* out of gamut */
				else
				{
					longG = longX * longMat[3] + longY * longMat[4] + longZ * longMat[5];
					if(longG & 0xF0000000)
						*XPtr++ = 255;		/* out of gamut */
					else
					{
						longB = longX * longMat[6] + longY * longMat[7] + longZ * longMat[8];
						if(longB & 0xF0000000)
							*XPtr++ = 255;		/* out of gamut */
						else
							*XPtr++ = 0;		/* in gamut */
					}
				}
			}
	
		/*---fill Boolean output LUT, adr_bereich_alut Bytes, 4 at one time with long:---*/
	lPtr = (long *)(tempALut);
	j = adr_bereich_alut/4/2 + 8;	/* slightly more than 50 % */
	for(i=0; i<j; i++)				/* slightly more than 50 % */
		*(lPtr + i) = 0;			/* in gamut */
	k = adr_bereich_alut/4;
	for(i=j; i<k; i++)
		*(lPtr + i) = 0xFFFFFFFF;	/* out of gamut */
	
	UNLOCK_DATA(tempELutHdl);
	UNLOCK_DATA(tempXLutHdl);
	UNLOCK_DATA(tempALutHdl);
	theLutData->colorLut	= tempXLutHdl;	tempXLutHdl = nil;
	theLutData->inputLut	= tempELutHdl;	tempELutHdl = nil;
	theLutData->outputLut	= tempALutHdl;	tempALutHdl = nil;
	
CleanupAndExit:
	tempELutHdl = DISPOSE_IF_DATA(tempELutHdl);
	tempXLutHdl = DISPOSE_IF_DATA(tempXLutHdl);
	tempALutHdl = DISPOSE_IF_DATA(tempALutHdl);
	LH_END_PROC("MakeGamut16or32ForMonitor")
	return (err);
}

