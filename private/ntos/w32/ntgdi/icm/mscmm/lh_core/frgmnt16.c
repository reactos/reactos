/*
	File:		LHFragment16.c

	Contains:	ALUT stuff (16 bit) for Color Sync

	Version:	

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
#define kThisFile kLHFragment16ID
#endif
#endif


/*-----prototypes for local functions-----*/
void	
Fill_inverseGamma_ushort_ALUT	( unsigned short *usALUT, char addrBits,
								  unsigned short gamma_u8_8);

/* ______________________________________________________________________

	CMError
		Fill_inverse_ushort_ALUT_from_CurveTag(	icCurveType		*pCurveTag,
												unsigned short	*usALUT,
												char			addrBits )
	Abstract:
		extracts output luts out of cmSigCurveType tag and converts them
		to desired format: (2 ^ addrBits) values in a range from 0 to 65535
		NOTE: not-monotone CurveTags are manipulated
		NOTE: Memory for the LUT has to be allocated before !

	Params:
		pCurveTag		(in)		extract input LUT from this
		usALUT			(in/out)	result LUT
		addrBits		(in)		2 ^ addrBits values are requested
		
	Return:
		noErr		successful

   _____________________________________________________________________ */
CMError
Fill_inverse_ushort_ALUT_from_CurveTag(	icCurveType		*pCurveTag,
										unsigned short	*usALUT,
										char			addrBits )
{
	unsigned long	i, inCount, outCount, clipIndex, ulFactor;
	unsigned long	intpFirst, intpLast, halfStep, ulAux, target;
	short			monot;
	unsigned short	*inCurve, *usPtr, *stopPtr;
	double			flFactor;
#ifdef DEBUG_OUTPUT
	OSErr err=noErr;
#endif
	LH_START_PROC("Fill_inverse_ushort_ALUT_from_CurveTag")
	
    if( pCurveTag->base.sig != icSigCurveType	/* 'curv' */
	 || addrBits > 15 )
	 {
#ifdef DEBUG_OUTPUT
		if ( DebugCheck(kThisFile, kDebugErrorInfo) )
			DebugPrint("¥ Fill_inverse_ushort_ALUT_from_CurveTag ERROR:   addrBits= %d\n",addrBits);
#endif
		return(cmparamErr);
	 }
	
	outCount  = 1 << addrBits;
	clipIndex = outCount - 1;

		/*---special cases:---*/

	if(pCurveTag->curve.count == 0)		/*---identity---*/
	{
		ulFactor = ((unsigned long)65535 << 16) / clipIndex;		/* use all 32 bits */
		
		for(i=0; i<clipIndex; i++)
			usALUT[i] = (unsigned short)((i * ulFactor + 32767) >> 16);
	
		for(i=clipIndex; i<outCount; i++)
			usALUT[i] = 0xFFFF;
	
		return(noErr);
	}
	else if(pCurveTag->curve.count == 1)	/*---gamma curve---*/
	{
		Fill_inverseGamma_ushort_ALUT(usALUT, addrBits, pCurveTag->curve.data[0]);
		return(noErr);
	}
	
		/*---ordinary case:---*/
	
	inCount = pCurveTag->curve.count;
	inCurve = pCurveTag->curve.data;
		
		 /* exact matching factor needed for special values: */
	flFactor = (double)clipIndex / 65535.;
	
	halfStep = clipIndex >> 1;		/* lessen computation incorrectness */
	
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
				inCurve[i] = inCurve[i-1];	/* correct not-invertible parts */
		
		intpFirst = (unsigned long)(inCurve[0] * flFactor + 0.9999);
		intpLast  = (unsigned long)(inCurve[inCount-1] * flFactor);
		
		for(i=0; i<intpFirst; i++)			/* fill lacking area low */
			usALUT[i] = 0;
		for(i=intpLast+1; i<outCount; i++)	/* fill lacking area high */
			usALUT[i] = 0xFFFF;

			/* interpolate remaining values: */
		usPtr   = inCurve;
		stopPtr = inCurve + inCount - 2; /* stops incrementation */
		
		for(i=intpFirst; i<=intpLast; i++)
		{
			target = (0x0FFFF * i + halfStep)  / clipIndex;
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
			
			usALUT[i] = (unsigned short)ulAux;
		}
	}
	else			/* curve seems to be descending */
	{
		for(i=1; i<inCount; i++)
			if(inCurve[i-1] < inCurve[i])
				inCurve[i] = inCurve[i-1];	/* correct not-invertible parts */
		
		intpFirst = (unsigned long)(inCurve[inCount-1] * flFactor + 0.9999);
		intpLast  = (unsigned long)(inCurve[0] * flFactor);
		
		for(i=0; i<intpFirst; i++)			/* fill lacking area low */
			usALUT[i] = 0xFFFF;
		for(i=intpLast+1; i<outCount; i++)	/* fill lacking area high */
			usALUT[i] = 0;

			/* interpolate remaining values: */
		usPtr   = inCurve + inCount - 1;
		stopPtr = inCurve + 1; 		/* stops decrementation */
		
		for(i=intpFirst; i<=intpLast; i++)
		{
			target = (0x0FFFF * i + halfStep)  / clipIndex;
			while(*(usPtr-1) < target && usPtr > stopPtr)
				usPtr--;					/* find interval */
			
			ulAux = ((unsigned long)(usPtr-1 - inCurve) << 16) / (inCount - 1);
			if(*(usPtr-1) != *usPtr)
			{
				ulAux += (((unsigned long)*(usPtr-1) - target) << 16)
					  / ( (*(usPtr-1) - *usPtr) * (inCount - 1) );
				
				if(ulAux & 0x10000)
					ulAux = 0xFFFF;
			}
			
			usALUT[i] = (unsigned short)ulAux;
		}
	}
	

	LH_END_PROC("Fill_inverse_ushort_ALUT_from_CurveTag")
	return(noErr);
}

/*   _____________________________________________________________________ */

void
Fill_inverseGamma_ushort_ALUT(	unsigned short	*usALUT,
								char			addrBits,
								unsigned short	gamma_u8_8 )
{
	unsigned long	i, j, outCount, step, stopit;
	long			leftVal, Diff, lAux;
	CM_Doub			invGamma, x, xFactor;
	long			clipIndex;
#ifdef DEBUG_OUTPUT
	OSErr err = noErr;
#endif
	LH_START_PROC("Fill_inverseGamma_ushort_ALUT")

	outCount = 0x1 << addrBits;
	
	invGamma  = 256. / (CM_Doub)gamma_u8_8;
	clipIndex = outCount - 1;
	xFactor   = 1. / (CM_Doub)clipIndex;
	
	if(addrBits <= 6)		/* up to 64 - 2 float.computations */
		step = 1;
	else
		step = 0x1 << (addrBits - 6);		/* more would take too long */
	
	usALUT[0]          = 0;			/* these two...	*/
	usALUT[outCount-1] = 0xFFFF;	/* ...are fixed	*/
	
	for(i=step; i<outCount-1; i+=step)
	{
		x = (CM_Doub)i * xFactor;
		if(x > 1.)
			x = 1.;		/* clipping in the end of ALUT */
		
		usALUT[i] = (unsigned short)( pow(x,invGamma) * 65535.0 + 0.5);
	}
	
		/*---fill intervals - except for last, which is odd:---*/
	for(i=0; i<outCount-step; i+=step)
	{
		leftVal = (long)usALUT[i];
		Diff    = (long)usALUT[i + step] - leftVal;
			
		for(j=1; j<step; j++)
		{
			lAux = ( (Diff * j << 8) / step + 128 ) >> 8;

			usALUT[i + j] = (unsigned short)(leftVal + lAux);
		}
	}
	
		/*---fill last interval:---*/
	i       = outCount - step;
	leftVal = (long)usALUT[i];
	Diff    = 0x0FFFF - leftVal;	/* 0xFFFF for 1.0 */
		
	for(j=1; j<step-1; j++)		/* stops here if step <= 2 */
	{
		lAux = ( (Diff * j << 8) / (step - 1) + 128 ) >> 8;

		usALUT[i + j] = (unsigned short)(leftVal + lAux);
	}
	
		/*--overwrite sensitive values depending on Gamma:--*/
	if(addrBits > 6 && invGamma < 1.0)		/* ...if lower part is difficult */
	{
		stopit = 0x1 << (addrBits - 6);
		
		for(i=1; i<stopit; i++)
		{
			x         = (CM_Doub)i * xFactor;
			usALUT[i] = (unsigned short)( pow(x,invGamma) * 65535.0);
		}
	}

	LH_END_PROC("Fill_inverseGamma_ushort_ALUT")
}

/* ______________________________________________________________________

	CMError
	Fill_ushort_ALUTs_from_lut8Tag( CMLutParamPtr	theLutData,
								  	Ptr				profileALuts,
								  	char			addrBits )
	Abstract:
		extracts output luts out of CMLut8Type tag and converts them
		to desired format: (2 ^ addrBits) values in a range from 0 to 65535

	Params:
		theLutData			(in/out)	Ptr to structure that holds all the luts...
		profileALuts		(in)		Ptr to the profile's output luts
		addrBits			(in)		2 ^ addrBits values are requested
		
	Return:
		noErr		successful

   _____________________________________________________________________ */
CMError
Fill_ushort_ALUTs_from_lut8Tag(	CMLutParamPtr	theLutData,
							  	Ptr				profileALuts,
							  	char			addrBits )
{
	long			i, j;
	unsigned char	*curOutLut;
	unsigned char	*profAluts = (unsigned char *)profileALuts;
	unsigned short	*curALUT;
	long			count, clipIndex;
	long			factor, fract, baseInd, lAux, leftVal, rightVal;
	OSErr			err = noErr;
	LUT_DATA_TYPE	localAlut = nil;
	unsigned short	*localAlutPtr;
	long			theAlutSize;
	
	LH_START_PROC("Fill_ushort_ALUTs_from_lut8Tag")
	
	count     = 1 << addrBits;						/* addrBits is always >= 8 */
	clipIndex = count - 1;
	
	theAlutSize = theLutData->colorLutOutDim * count * sizeof(unsigned short);
	localAlut   = ALLOC_DATA(theAlutSize + 2, &err);
	if (err)
		goto CleanupAndExit;
	
	LOCK_DATA(localAlut);
	localAlutPtr = (unsigned short *)DATA_2_PTR(localAlut);
	
	factor = ((255 << 12) + clipIndex/2) / clipIndex;		/* for adjusting the indices */
	
	for(i=0; i<theLutData->colorLutOutDim; i++)
	{
		curOutLut = profAluts + (i << 8);		/* these are unsigned char's */
		curALUT   = localAlutPtr + i * count;	/* these are unsigned short's */
		
		for(j=0; j<=clipIndex-1; j++)
		{
			lAux    = j * factor;
			baseInd = (unsigned long)lAux >> 12;
			fract   = lAux & 0x0FFF;
			
			leftVal = (long)curOutLut[baseInd];
			leftVal = (leftVal << 8) + leftVal;		/* 0xFF -> 0xFFFF */
			
			if(fract)
			{
				rightVal = (long)curOutLut[baseInd + 1];
				rightVal = (rightVal << 8) + rightVal;		/* 0xFF -> 0xFFFF */
				
				lAux = rightVal - leftVal;
				lAux = (lAux * fract + 0x0800) >> 12;
				
				curALUT[j] = (unsigned short)(leftVal + lAux);
			}
			else
				curALUT[j] = (unsigned short)leftVal;
		}
		
		leftVal = (long)curOutLut[255];
		leftVal = (leftVal << 8) + leftVal;		/* 0xFF -> 0xFFFF */
		curALUT[j] = (unsigned short)leftVal;
		
		for(j=clipIndex+1; j<count; j++)		/* unused indices, clip these */
			curALUT[j] = curALUT[clipIndex];
	}
	
	UNLOCK_DATA(localAlut);
	theLutData->outputLut = localAlut;
	localAlut = nil;
CleanupAndExit:
	localAlut = DISPOSE_IF_DATA(localAlut);

	LH_END_PROC("Fill_ushort_ALUTs_from_lut8Tag")
	return err;
}

/* ______________________________________________________________________

	CMError
	Fill_ushort_ALUTs_from_lut16Tag(CMLutParamPtr	theLutData,
									Ptr				profileALuts,
									char			addrBits,
								    long			outputTableEntries )
	Abstract:
		extracts output luts out of CMLut16Type tag and converts them
		to desired format: (2 ^ addrBits) values in a range from 0 to 65535

	Params:
		theLutData			(in/out)	Ptr to structure that holds all the luts...
		profileALuts		(in)		Ptr to the profile's output luts
		addrBits			(in)		2 ^ addrBits values are requested
		outputTableEntries	(in)		number of entries in the output lut (up to 4096)
		
	Return:
		noErr		successful

   _____________________________________________________________________ */
CMError
Fill_ushort_ALUTs_from_lut16Tag(CMLutParamPtr	theLutData,
								Ptr				profileALuts,
								char			addrBits,
							    long			outputTableEntries )
{
	long			i;
	unsigned short	*curOutLut;
	unsigned short	*curALUT;
	unsigned long	ulIndFactor, j;
	long			count, clipIndex, outTabLen;
	long			fract, baseInd, lAux, leftVal, rightVal;
	unsigned short	*profALUTs = (unsigned short *)profileALuts;
	OSErr			err = noErr;
	LUT_DATA_TYPE	localAlut = nil;
	unsigned short	*localAlutPtr;
	long			theAlutSize;
	
	LH_START_PROC("Fill_ushort_ALUTs_from_lut16Tag")
	
	count     = 1 << addrBits;						/* addrBits is always >= 8 */
	clipIndex = count - 1;

	theAlutSize = theLutData->colorLutOutDim * count * sizeof(unsigned short);
	localAlut   = ALLOC_DATA(theAlutSize + 2, &err);
	if (err)
		goto CleanupAndExit;
	
	outTabLen = outputTableEntries;			/* <= 4096 acc. to the spec */
	if(outTabLen > 4096)
	{
		err = cmparamErr;
		goto CleanupAndExit;
	}
	
	ulIndFactor = (((unsigned long)outTabLen - 1) << 20)
				/ (unsigned long)clipIndex;				/* for adjusting the indices */
	
	LOCK_DATA(localAlut);
	localAlutPtr = (unsigned short *)DATA_2_PTR(localAlut);
	
	for(i=0; i<theLutData->colorLutOutDim; i++)
	{
		curOutLut = profALUTs + i * outTabLen;
		curALUT   = localAlutPtr + i * count;
		
		for(j=0; j<=(unsigned long)clipIndex; j++)
		{
			lAux    = (long)( (j * ulIndFactor + 16) >> 5 );		/* n.b: j is unsigned long ! */
			baseInd = (unsigned long)lAux >> 15;
			fract   = lAux & 0x7FFF;	/* 15 bits for interpolation */
			
			if(fract)
			{
				leftVal  = (long)curOutLut[baseInd];
				rightVal = (long)curOutLut[baseInd + 1];
				
				lAux = rightVal - leftVal;
				lAux = (lAux * fract + 16383) >> 15;
				
				curALUT[j] = (unsigned short)(leftVal + lAux);
			}
			else
				curALUT[j] = curOutLut[baseInd];
		}
		
		for(j=clipIndex+1; j<(unsigned long)count; j++)		/* unused indices, clip these */
			curALUT[j] = curALUT[clipIndex];
	}
	
	UNLOCK_DATA(localAlut);
	theLutData->outputLut = localAlut;
	localAlut = nil;
CleanupAndExit:
	localAlut = DISPOSE_IF_DATA(localAlut);

	LH_END_PROC("Fill_ushort_ALUTs_from_lut16Tag")
	return err;
}

/* ______________________________________________________________________

	CMError
		DoAbsoluteShiftForPCS_Cube16(	unsigned short	*theCube,
										long			count,
										CMProfileRef	theProfile,
										Boolean			pcsIsXYZ,
										Boolean			afterInput )
	Abstract:
		Performs color shift necessary for absolute colorimetry. Data of
		the cube points are in linear XYZ (16 bit) or in Lab (16 bit).
		Either conversion just after entering PCS or before leaving PCS (inverse
		operation). NOTE: for devices with colorant matrices this operation is
		done much faster by manipulating the matrix.

	Params:
		theCube			(in/out)	cube grid points
		count			(in)		number of points
		theProfile		(in)		contains media white point
		pcsIsXYZ		(in)		XYZ/Lab, saves one file access to profile
		afterInput		(in)		direct or inverse operation

	Return:
		noErr			successful
   _____________________________________________________________________ */
CMError	DoAbsoluteShiftForPCS_Cube16(	unsigned short	*theCube,
										long			count,
										CMProfileRef	theProfile,
										Boolean			pcsIsXYZ,
										Boolean			afterInput )
{
	unsigned long		i, uLong;
	unsigned short		*usPtr;
   CMError				err = noErr;
	unsigned long 		elementSize;
	icXYZType			curMediaWhite;
	double				xFactor, yFactor, zFactor;
	unsigned long		intFactorX, intFactorY, intFactorZ;
	unsigned long		roundX, roundY, roundZ;
	unsigned long		shiftX, shiftY, shiftZ;
	
	LH_START_PROC("DoAbsoluteShiftForPCS_Cube16")
	
	elementSize = sizeof(icXYZType);
	err = CMGetProfileElement(theProfile, icSigMediaWhitePointTag, &elementSize, &curMediaWhite);
#ifdef IntelMode
   SwapLongOffset( &curMediaWhite.base.sig, 0, 4 );
   SwapLongOffset( &curMediaWhite, (LONG)((char*)&curMediaWhite.data.data[0]-(char*)&curMediaWhite), elementSize );
#endif
	if(err)
	{
		if(err == cmElementTagNotFound)		/* take D50 and do nothing */
			return(noErr);
		else
			return(err);
	}
	
		/*--- preliminary matching factors: ---*/
	xFactor = ((double)curMediaWhite.data.data[0].X) / 65536. / 0.9642;
	if(xFactor > 100.)
		xFactor = 100.;			/* evil profile */
	else if(xFactor < 0.01)
		xFactor = 0.01;

	yFactor = ((double)curMediaWhite.data.data[0].Y) / 65536.;
	if(yFactor > 100.)
		yFactor = 100.;
	else if(yFactor < 0.01)
		yFactor = 0.01;

	zFactor = ((double)curMediaWhite.data.data[0].Z) / 65536. / 0.8249;
	if(zFactor > 100.)
		zFactor = 100.;
	else if(zFactor < 0.01)
		zFactor = 0.01;

	if( ( xFactor < 1.+1.E-3 && xFactor > 1.-1.E-3 ) &&
		( yFactor < 1.+1.E-3 && yFactor > 1.-1.E-3 ) &&
		( zFactor < 1.+1.E-3 && zFactor > 1.-1.E-3 ) )
			return noErr; /* do nothing if MediaWhite is D50 */
	
	if(!afterInput)		/* back to device space (for example with B2A1 table) */
	{
		xFactor = 1. / xFactor;
		yFactor = 1. / yFactor;
		zFactor = 1. / zFactor;
	}
	
		/*--- integer factors for speed: ---*/
	intFactorX = (unsigned long)(xFactor * 65536. * 64.);	/* probably too long...	*/
	intFactorY = (unsigned long)(yFactor * 65536. * 64.);	/* ...adding 22 bits	*/
	intFactorZ = (unsigned long)(zFactor * 65536. * 64.);
	
	roundX = roundY = roundZ = 0x1FFFFF;	/* 2^21 - 1 */
	shiftX = shiftY = shiftZ = 22;
	
	while(intFactorX & 0xFFFF0000)	/* stay within 16 bits to prevent product overflow */
	{
		intFactorX >>= 1;
		roundX     >>= 1;
		shiftX      -= 1;
	}
	
	while(intFactorY & 0xFFFF0000)
	{
		intFactorY >>= 1;
		roundY     >>= 1;
		shiftY      -= 1;
	}
	
	while(intFactorZ & 0xFFFF0000)
	{
		intFactorZ >>= 1;
		roundZ     >>= 1;
		shiftZ      -= 1;
	}
	
		/*--- perform matching: ---*/
	if(!pcsIsXYZ)		/* 16 bit linear Lab  to XYZ before and afterwards */
		Lab2XYZ_forCube16(theCube, count);
	
	usPtr = theCube;

	for(i=0; i<(unsigned long)count; i++)
	{
		uLong = ((unsigned long)(*usPtr) * intFactorX + roundX) >> shiftX;
		if(uLong > 0x0FFFF)
			uLong = 0xFFFF;				/* clip to 2.0 */
		*usPtr++ = (unsigned short)uLong;
		
		uLong = ((unsigned long)(*usPtr) * intFactorY + roundY) >> shiftY;
		if(uLong > 0x0FFFF)
			uLong = 0xFFFF;
		*usPtr++ = (unsigned short)uLong;
		
		uLong = ((unsigned long)(*usPtr) * intFactorZ + roundZ) >> shiftZ;
		if(uLong > 0x0FFFF)
			uLong = 0xFFFF;
		*usPtr++ = (unsigned short)uLong;
	}

	if(!pcsIsXYZ)		/* back to 16 bit Lab */
		XYZ2Lab_forCube16(theCube, count);


	LH_END_PROC("DoAbsoluteShiftForPCS_Cube16")
	return(noErr);
}
