/*
	File:		LHCMRuntime.c

	Contains:	

	Written by:	U. J. Krabbenhoeft

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

	Version:	
*/

#ifndef LHGeneralIncs_h
#include "General.h"
#endif

#ifndef LHCalcEngine_h
#include "CalcEng.h"
#endif

#ifndef LHCalcEngine1Dim_h
#include "CalcEng1.h"
#endif

#ifdef ALLOW_MMX
#ifndef LHCalcEngineFas_h
#include "CalcEnF.h"
#endif
#endif

#ifndef LHCalcNDim_h
#include "CalcNDim.h"
#endif

#ifndef LHCMRuntime_h
#include "Runtime.h"
#endif

#if ! realThing
#ifdef DEBUG_OUTPUT
#define kThisFile kCMMRuntimeID
extern Boolean gUSE_NDIM_FOR_BITMAP;
#endif
#endif

typedef struct ColorSpaceInfo
{
	SINT32	origSizeIn;
	SINT32	origSizeOut;
	SINT32	usedSizeIn;
	SINT32	usedSizeOut;
	void*	tempInBuffer;
	void*	tempOutBuffer;
	SINT32	processedLinesIn;
	SINT32	processedLinesOut;
	SINT32	inputPixelSize;
	SINT32	outputPixelSize;
	Boolean inPlace;
} ColorSpaceInfo;

/* -------------------------------------------------------------------------------------------------------------- */
#define kProgressTicks 	30			/* .5secs -> min. time between calls of progressproc */
#define kMaxTempBlock	300 * 1024	/* allow max. 300 kByte temp buffer */

/* -------------------------------------------------------------------------------------------------------------- */
typedef CMError  (*CalcProcPtr)(CMCalcParamPtr calcParamPtr,
							    CMLutParamPtr  lutParamPtr );

typedef CalcProcPtr 	 CalcProcUPP;
typedef CMError  (*CalcProc1DimPtr)(CMCalcParamPtr calcParamPtr,
									CMLutParamPtr  lutParamPtr, char OutDim );

typedef CalcProc1DimPtr 	 CalcProc1DimUPP;
#define NewCalcProc(userRoutine)		\
		((CalcProcUPP) (userRoutine))
#define NewCalcProc1Dim(userRoutine)		\
		((CalcProc1DimUPP) (userRoutine))
#define CallCalcProc(userRoutine, calcParamPtr, lutParamPtr)		\
		(*(userRoutine))( (calcParamPtr), (lutParamPtr) )

/*--------------------------------------------------------------------------------------------------------------
	local prototypes
  --------------------------------------------------------------------------------------------------------------*/

OSErr	Convert5To8				( Ptr		dataPtr5,
								  Ptr		dataPtr8,
								  SINT32	startLine,
								  SINT32	height,
								  SINT32	width,
								  SINT32	rowBytes5 );
OSErr	Convert8To5				( Ptr		dataPtr8,
								  Ptr		dataPtr5,
								  SINT32	startLine,
								  SINT32	height,
								  SINT32	width,
								  SINT32	rowBytes5 );
OSErr	Convert8To1				( Ptr		dataPtr8,
								  Ptr		dataPtr5,
								  SINT32	startLine,
								  SINT32	height,
								  SINT32	width,
								  SINT32	rowBytes5 );
OSErr	Convert16To10			( Ptr 		dataPtr16, 
								  Ptr		dataPtr10,
								  SINT32	startLine,
								  SINT32	height,
								  SINT32	width,
								  SINT32	rowBytes10 );	
OSErr	Convert10To16			( Ptr 		dataPtr10, 
								  Ptr		dataPtr16,
								  SINT32	startLine,
								  SINT32	height,
								  SINT32	width,
								  SINT32	rowBytes10 );		

CMError	FillLutParam 			( CMLutParamPtr		lutParam,
 								  CMMModelPtr		modelingData );
CMError	FillLutParamChk			( CMLutParamPtr		lutParam,
 								  CMMModelPtr		modelingData );

CMError	FillCalcParam			( CMCalcParamPtr	calcParam,
				 				  const CMBitmap *	bitMap, 
							 	  const CMBitmap *	matchedBitMap );

CMError	FillCalcParamCM			( CMCalcParamPtr	calcParam,
								  CMLutParamPtr		lutParam,
								  CMColor*			myColors,
								  SINT32				count );
CMError	CheckInputColorSpace 	( const CMBitmap*	bitMap,
								  CMCalcParamPtr	calcParam,
								  ColorSpaceInfo*	info,
								  OSType			inColorSpace,
					 			  long 				colorLutInDim );
CMError	CheckOutputColorSpace	( const CMBitmap*	bitMap,
								  CMCalcParamPtr	calcParam,
								  ColorSpaceInfo*	info,
								  OSType			outColorSpace,
								  long				colorLutOutDim );
CMError	SetOutputColorSpaceInplace	( CMCalcParamPtr	calcParam,
									  ColorSpaceInfo*	info,
									  OSType 			outColorSpace );
CMError	CheckOutputColorSpaceChk( const CMBitmap*	bitMap,
								  CMCalcParamPtr	calcParam,
								  ColorSpaceInfo*	info );

CMError	AllocBufferCheckCM		( CMCalcParamPtr	calcParam,
								  ColorSpaceInfo*	info );
								  
CalcProcPtr	FindLookupRoutine 	( const CMLutParam*		lutParam,
								  const ColorSpaceInfo*	info );

CalcProcPtr	FindCalcRoutine 	( const CMCalcParam*	calcParam,
								  const CMLutParam*		lutParam,
								  const ColorSpaceInfo*	info,
								  const Boolean			lookupOnly );


#ifdef __MWERKS__
#pragma mark ================  packing/unpacking  ================
#endif

/*--------------------------------------------------------------------------------------------------------------
	Convert5To8								convert cmRGB16Space to cmRGB24Space
  --------------------------------------------------------------------------------------------------------------*/
OSErr
Convert5To8	( Ptr		dataPtr5,
			  Ptr		dataPtr8,
			  SINT32	startLine,
			  SINT32	height,
			  SINT32	width,
			  SINT32	rowBytes5 )
{
	UINT16*		srcPtr16;
	UINT8*		destPtr8   = (UINT8*)dataPtr8;
	UINT8		data8;
	UINT16		three5Bits;
	SINT32		lineLoop;
	SINT32		pixelLoop;
	OSErr		err = noErr;
	
	LH_START_PROC("Convert5To8")
	for (lineLoop = startLine;  lineLoop < startLine + height; lineLoop++)
	{
		srcPtr16 = (UINT16*)(dataPtr5 + (lineLoop*rowBytes5));
		for (pixelLoop = 0; pixelLoop < width; pixelLoop++)
		{
			three5Bits = *srcPtr16++;
			
			data8 = (three5Bits >> 10) & 0x001F;	
			*destPtr8++ = (data8 << 3) | (data8 >>2);
			
			data8 = (three5Bits >> 5) & 0x001F;
			*destPtr8++ = (data8 << 3) | (data8 >>2);
	
			data8 = three5Bits & 0x001F;	
			*destPtr8++ = (data8 << 3) | (data8 >>2);
		}
	}
	LH_END_PROC("Convert5To8")
	return err;
}

/*--------------------------------------------------------------------------------------------------------------
	Convert8To5								convert cmRGB24Space to cmRGB16Space
  --------------------------------------------------------------------------------------------------------------*/
OSErr
Convert8To5	( Ptr		dataPtr8,
			  Ptr		dataPtr5,
			  SINT32	startLine,
			  SINT32	height,
			  SINT32	width,
			  SINT32	rowBytes5 )
{
	UINT16*	destPtr16;
	UINT8*	srcPtr8		= (UINT8*)dataPtr8;
	UINT16	three5Bits;
	SINT32	lineLoop;
	SINT32	pixelLoop;
	OSErr	err = noErr;
	
	LH_START_PROC("Convert8To5")
	for (lineLoop = startLine;  lineLoop < startLine + height; lineLoop++)
	{
		destPtr16 = (UINT16*)(dataPtr5 + (lineLoop*rowBytes5));
		for (pixelLoop = 0; pixelLoop < width; pixelLoop++)
		{
			three5Bits  = (((UINT16)*srcPtr8 & 0x00F8) << 7);	
			srcPtr8++;
			three5Bits |= (((UINT16)*srcPtr8 & 0x00F8) << 2);	
			srcPtr8++;
			three5Bits |= (((UINT16)*srcPtr8 & 0x00F8) >> 3);	
			srcPtr8++;

			*destPtr16++ = three5Bits;
		}
	}
	LH_END_PROC("Convert8To5")
	return err;
}
#ifdef PI_Application_h
OSErr	Convert565To8			( Ptr		dataPtr5,
								  Ptr		dataPtr8,
								  SINT32	startLine,
								  SINT32	height,
								  SINT32	width,
								  SINT32	rowBytes5 );
OSErr	Convert8To565			( Ptr		dataPtr8,
								  Ptr		dataPtr5,
								  SINT32	startLine,
								  SINT32	height,
								  SINT32	width,
								  SINT32	rowBytes5 );
/*--------------------------------------------------------------------------------------------------------------
	Convert565To8								convert cmRGB16_565Space to cmRGB24Space
  --------------------------------------------------------------------------------------------------------------*/
OSErr
Convert565To8( Ptr		dataPtr5,
			  Ptr		dataPtr8,
			  SINT32	startLine,
			  SINT32	height,
			  SINT32	width,
			  SINT32	rowBytes5 )
{
	UINT16*		srcPtr16;
	UINT8*		destPtr8   = (UINT8*)dataPtr8;
	UINT8		data8;
	UINT16		three5Bits;
	SINT32		lineLoop;
	SINT32		pixelLoop;
	OSErr		err = noErr;
	
	LH_START_PROC("Convert565To8")
	for (lineLoop = startLine;  lineLoop < startLine + height; lineLoop++)
	{
		srcPtr16 = (UINT16*)(dataPtr5 + (lineLoop*rowBytes5));
		for (pixelLoop = 0; pixelLoop < width; pixelLoop++)
		{
			three5Bits = *srcPtr16++;
			
			data8 = (three5Bits >> 11) & 0x001F;	
			*destPtr8++ = (data8 << 3) | (data8 >>2);
			
			data8 = (three5Bits >> 5) & 0x003F;
			*destPtr8++ = (data8 << 2) | (data8 >>4);
	
			data8 = three5Bits & 0x001F;	
			*destPtr8++ = (data8 << 3) | (data8 >>2);
		}
	}
	LH_END_PROC("Convert565To8")
	return err;
}

/*--------------------------------------------------------------------------------------------------------------
	Convert8To565								convert cmRGB24Space to cmRGB16_565Space
  --------------------------------------------------------------------------------------------------------------*/
OSErr
Convert8To565( Ptr		dataPtr8,
			  Ptr		dataPtr5,
			  SINT32	startLine,
			  SINT32	height,
			  SINT32	width,
			  SINT32	rowBytes5 )
{
	UINT16*	destPtr16;
	UINT8*	srcPtr8		= (UINT8*)dataPtr8;
	UINT16	three5Bits;
	SINT32	lineLoop;
	SINT32	pixelLoop;
	OSErr	err = noErr;
	
	LH_START_PROC("Convert8To565")
	for (lineLoop = startLine;  lineLoop < startLine + height; lineLoop++)
	{
		destPtr16 = (UINT16*)(dataPtr5 + (lineLoop*rowBytes5));
		for (pixelLoop = 0; pixelLoop < width; pixelLoop++)
		{
			three5Bits  = (((UINT16)*srcPtr8 & 0x00F8) << 8);	
			srcPtr8++;
			three5Bits |= (((UINT16)*srcPtr8 & 0x00FC) << 3);	
			srcPtr8++;
			three5Bits |= (((UINT16)*srcPtr8 & 0x00F8) >> 3);	
			srcPtr8++;

			*destPtr16++ = three5Bits;
		}
	}
	LH_END_PROC("Convert8To565")
	return err;
}
#endif
/*--------------------------------------------------------------------------------------------------------------
	Convert8To1								convert 8-bit to 1-bit (gamut result)
  --------------------------------------------------------------------------------------------------------------*/
OSErr
Convert8To1	( Ptr		dataPtr8,
			  Ptr		dataPtr1,
			  SINT32	startLine,
			  SINT32	height,
			  SINT32	width,
			  SINT32	rowBytes1 )
{
	UINT8*	destPtr8;
	UINT8*	srcPtr8  = (UINT8*)dataPtr8;
	UINT8	theWord  = 0;
	SINT32	lineLoop;
	SINT32	pixelLoop;
	OSErr	err = noErr;

	LH_START_PROC("Convert8To1")
	for (lineLoop = startLine;  lineLoop < startLine + height; lineLoop++)
	{
		destPtr8 = (UINT8*)(dataPtr1 + (lineLoop*rowBytes1));
		for ( pixelLoop = 0; pixelLoop < width; pixelLoop++ )
		{
			if (*srcPtr8)
				theWord |= (1<< (7- ( pixelLoop & 7)));
			if ( (pixelLoop & 7) == 7)
			{
				*destPtr8++ = theWord;
				theWord = 0;
			}
			srcPtr8 ++;
		}
		if (width % 8)
		{
			*destPtr8 = theWord | ( ((unsigned char)255 >> (width % 8)));
			theWord = 0;
		}
	}
	LH_END_PROC("Convert8To1")
	return err;
}

/* -----------------------------------------------------------------------
	Convert16To10
   ----------------------------------------------------------------------- */
OSErr
Convert16To10	( Ptr 		dataPtr16, 
				  Ptr		dataPtr10,
				  SINT32	startLine,
				  SINT32	height,
				  SINT32	width,
				  SINT32	rowBytes10 )
{
	UINT32		data16_1;
	UINT32		data16_2;
	UINT32		data16_3;
	UINT16*		srcPtr16 = (UINT16*)dataPtr16;
	UINT32*		destPtr32;
	SINT32		lineLoop;
	SINT32		pixelLoop;
	OSErr	err = noErr;

	LH_START_PROC("Convert16To10")
	for (lineLoop = startLine;  lineLoop < startLine + height; lineLoop++)
	{
		destPtr32 = (UINT32*)( dataPtr10 + (lineLoop * rowBytes10));
		for (pixelLoop = 0; pixelLoop < width; pixelLoop++)
		{
			data16_1 = ((UINT32)*srcPtr16++  & 0x0000FFC0) << 14;
			data16_2 = ((UINT32)*srcPtr16++  & 0x0000FFC0) <<  4;
			data16_3 = ((UINT32)*srcPtr16++  & 0x0000FFC0) >>  6;
			*destPtr32++ = data16_1 | data16_2 | data16_3;
		}
	}
	LH_END_PROC("Convert16To10")
	return err;
}

/* -----------------------------------------------------------------------
	Convert10To16
   ----------------------------------------------------------------------- */
OSErr
Convert10To16	( Ptr 		dataPtr10, 
				  Ptr		dataPtr16,
				  SINT32	startLine,
				  SINT32	height,
				  SINT32	width,
				  SINT32	rowBytes10 )		
{
	UINT16		data16;
	UINT32		data32;
	UINT32*		srcPtr32;
	UINT16*		destPtr16 = (UINT16*)dataPtr16;
	SINT32		lineLoop;
	SINT32		pixelLoop;
	OSErr	err = noErr;

	LH_START_PROC("Convert10To16")
	for (lineLoop = startLine;  lineLoop < startLine + height; lineLoop++)
	{
		srcPtr32 = (UINT32*)( dataPtr10 + (lineLoop * rowBytes10));
		for (pixelLoop = 0; pixelLoop < width; pixelLoop++)
		{
			data32 = *srcPtr32++;
			data16 = (UINT16)(data32>>14) & 0x0FFC0;
			*destPtr16++  = data16 | data16>>10;
			data16 = (UINT16)(data32>>4) & 0x0FFC0;
			*destPtr16++  = data16 | data16>>10;
			data16 = (UINT16)(data32<<6);
			*destPtr16++  = data16 | data16>>10;
		}
	}
	LH_END_PROC("Convert10To16")
	return err;
}

#ifdef __MWERKS__
#pragma mark ================  setup & checking  ================
#endif

/*--------------------------------------------------------------------------------------------------------------
	FillLutParam
  --------------------------------------------------------------------------------------------------------------*/
CMError
FillLutParam 	( CMLutParamPtr		lutParam,
 				  CMMModelPtr		modelingData )
{
	CMError err = noErr;

	LH_START_PROC("FillLutParam")
	*lutParam = (modelingData)->lutParam;
#if ! LUTS_ARE_PTR_BASED
	lutParam->inputLut		 	  = DATA_2_PTR(modelingData)->lutParam.inputLut;
	lutParam->outputLut			  = DATA_2_PTR(modelingData)->lutParam.outputLut;
	lutParam->colorLut		 	  = DATA_2_PTR(modelingData)->lutParam.colorLut;
#endif
	LH_END_PROC("FillLutParam")
	return err;
}

/*--------------------------------------------------------------------------------------------------------------
	FillLutParamChk
  --------------------------------------------------------------------------------------------------------------*/
CMError
FillLutParamChk 	( CMLutParamPtr		lutParam,
 					  CMMModelPtr		modelingData )
{
	CMError err = noErr;

	LH_START_PROC("FillLutParamChk")
	*lutParam = (modelingData)->gamutLutParam;
#if ! LUTS_ARE_PTR_BASED
	lutParam->inputLut		= DATA_2_PTR(modelingData)->gamutLutParam.inputLut;
	lutParam->outputLut		= DATA_2_PTR(modelingData)->gamutLutParam.outputLut;
	lutParam->colorLut		= DATA_2_PTR(modelingData)->gamutLutParam.colorLut;
#endif
	LH_END_PROC("FillLutParamChk")
	return err;
}

/*--------------------------------------------------------------------------------------------------------------
	FillCalcParam
  --------------------------------------------------------------------------------------------------------------*/
CMError
FillCalcParam	( CMCalcParamPtr	calcParam,
 				  const CMBitmap *	bitMap, 
			 	  const CMBitmap *	matchedBitMap )
{
	CMError err = noErr;

	LH_START_PROC("FillCalcParam")

	calcParam->cmInputColorSpace	= bitMap->space;
	calcParam->cmOutputColorSpace	= matchedBitMap->space;
	calcParam->cmPixelPerLine		= bitMap->width;
	calcParam->cmInputBytesPerLine  = bitMap->rowBytes;
	calcParam->cmOutputBytesPerLine = matchedBitMap->rowBytes;
	calcParam->cmLineCount			= bitMap->height;

	LH_END_PROC("FillCalcParam")
	return err;
}

/*--------------------------------------------------------------------------------------------------------------
	FillCalcParamCM
  --------------------------------------------------------------------------------------------------------------*/
CMError
FillCalcParamCM	( CMCalcParamPtr	calcParam,
				  CMLutParamPtr		lutParam,
				  CMColor*			myColors,
				  SINT32			count )
{
	SINT32	loop;
	CMError err = noErr;

	LH_START_PROC("FillCalcParamCM")
	
	calcParam->cmInputPixelOffset  = sizeof(CMColor);
	calcParam->cmOutputPixelOffset = sizeof(CMColor);

	/* ---------------------------------------------------------- handle input */
	switch (lutParam->colorLutInDim)
	{
		case 1:
			calcParam->cmInputColorSpace = cmGraySpace;	/* cmGraySpace is 16 bit */
			calcParam->inputData[0]	= (Ptr)myColors;
			break;
		case 3:
			calcParam->cmInputColorSpace = cmRGBSpace|cm16PerChannelPacking;
			calcParam->inputData[0]	= ((Ptr)myColors);
			calcParam->inputData[1]	= ((Ptr)myColors) + 2;
			calcParam->inputData[2]	= ((Ptr)myColors) + 4;
			break;
		case 4:
			calcParam->cmInputColorSpace = cmCMYKSpace|cm16PerChannelPacking;
			calcParam->inputData[0]	= ((Ptr)myColors);
			calcParam->inputData[1]	= ((Ptr)myColors) + 2;
			calcParam->inputData[2]	= ((Ptr)myColors) + 4;
			calcParam->inputData[3]	= ((Ptr)myColors) + 6;
			break;
		case 5:
		case 6:
		case 7:
		case 8:
#if ( CM_MAX_COLOR_CHANNELS == 15 )
		case 2:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
#endif	
			calcParam->cmInputColorSpace = cmMCFiveSpace + lutParam->colorLutInDim-5;
			calcParam->cmInputColorSpace |= cm8PerChannelPacking;
			for (loop = 0; loop<lutParam->colorLutInDim; loop++)
				calcParam->inputData[loop]	= ((Ptr)myColors) + loop;
			break;
	}
	
	/* ---------------------------------------------------------- handle output */
	switch (lutParam->colorLutOutDim)
	{
		case 1:
			calcParam->cmOutputColorSpace = cmGraySpace;	/* cmGraySpace is 16 bit */
			calcParam->outputData[0]	= (Ptr)myColors;
			break;
		case 3:
			calcParam->cmOutputColorSpace = cmRGBSpace|cm16PerChannelPacking;
			calcParam->outputData[0]	= ((Ptr)myColors);
			calcParam->outputData[1]	= ((Ptr)myColors) + 2;
			calcParam->outputData[2]	= ((Ptr)myColors) + 4;
			break;
		case 4:
			calcParam->cmOutputColorSpace = cmCMYKSpace|cm16PerChannelPacking;
			calcParam->outputData[0]	= ((Ptr)myColors);
			calcParam->outputData[1]	= ((Ptr)myColors) + 2;
			calcParam->outputData[2]	= ((Ptr)myColors) + 4;
			calcParam->outputData[3]	= ((Ptr)myColors) + 6;
			break;
		case 5:
		case 6:
		case 7:
		case 8:
#if ( CM_MAX_COLOR_CHANNELS == 15 )
		case 2:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
#endif
			calcParam->cmOutputColorSpace = cmMCFiveSpace + lutParam->colorLutOutDim - 5;
			calcParam->cmOutputColorSpace |= cm8PerChannelPacking;
			for (loop = 0; loop<lutParam->colorLutOutDim; loop++)
				calcParam->outputData[loop]	= ((Ptr)myColors) + loop;
			break;
	}
	calcParam->cmPixelPerLine		= count;
	calcParam->cmInputBytesPerLine  = count*sizeof(CMColor);
	calcParam->cmOutputBytesPerLine = count*sizeof(CMColor);
	calcParam->cmLineCount			= 1;
	LH_END_PROC("FillCalcParamCM")
	return err;
}

#ifdef PI_Application_h
#define ALLOW_16BIT_DATA
#else
#define Byte_Factor 1
#endif
	
CMError Do8To555Setup( CMCalcParamPtr calcParam, ColorSpaceInfo *info, SINT32 *theLinesAtOnce, long reverseOrder );
CMError Do8To555Setup( CMCalcParamPtr calcParam, ColorSpaceInfo *info, SINT32 *theLinesAtOnce, long reverseOrder )
{
	SINT32	newRowBytes,linesAtOnce,bufferSize;
	SINT16	iErr = noErr;

	*theLinesAtOnce = 0;
	newRowBytes = calcParam->cmPixelPerLine * 3;	/* TempBuffer -> cmRGB24Space */
	linesAtOnce = (kMaxTempBlock) / newRowBytes;
	if (linesAtOnce == 0)
		linesAtOnce = 1;
	else if (linesAtOnce > calcParam->cmLineCount)
		linesAtOnce = calcParam->cmLineCount;
		
	bufferSize = newRowBytes * linesAtOnce;
	info->processedLinesOut = 0;
	info->origSizeOut 		= 5;
	info->usedSizeOut 		= 8;
	info->tempOutBuffer 	= (void*)SmartNewPtr(bufferSize, &iErr);
	info->outputPixelSize = 16;
	if (iErr != noErr)
	{
		return iErr;
	}

	calcParam->cmLineCount			= linesAtOnce;
	calcParam->cmOutputColorSpace   = cmRGB24Space;
	calcParam->cmOutputBytesPerLine = newRowBytes;
	if( reverseOrder ){
		calcParam->outputData[2]		=  (Ptr)info->tempOutBuffer;
		calcParam->outputData[1]		= ((Ptr)info->tempOutBuffer)+1;
		calcParam->outputData[0]		= ((Ptr)info->tempOutBuffer)+2;
	}
	else{
		calcParam->outputData[0]		=  (Ptr)info->tempOutBuffer;
		calcParam->outputData[1]		= ((Ptr)info->tempOutBuffer)+1;
		calcParam->outputData[2]		= ((Ptr)info->tempOutBuffer)+2;
	}
	calcParam->cmOutputPixelOffset	= 3;
	*theLinesAtOnce = linesAtOnce;
	return 0;
}

CMError Do555To8Setup( CMCalcParamPtr calcParam, ColorSpaceInfo *info, SINT32 *theLinesAtOnce, long reverseOrder );
CMError Do555To8Setup( CMCalcParamPtr calcParam, ColorSpaceInfo *info, SINT32 *theLinesAtOnce, long reverseOrder )
{
	SINT32	newRowBytes,linesAtOnce,bufferSize;
	SINT16	iErr = noErr;

	*theLinesAtOnce = 0;
	newRowBytes = calcParam->cmPixelPerLine * 3;	/* TempBuffer -> cmRGB24Space */
	linesAtOnce = (kMaxTempBlock) / newRowBytes;
	if (linesAtOnce == 0)
		linesAtOnce = 1;
	else if (linesAtOnce > calcParam->cmLineCount)
		linesAtOnce = calcParam->cmLineCount;
		
	bufferSize = newRowBytes * linesAtOnce;
	info->processedLinesIn = 0;
	info->origSizeIn = 5;
	info->usedSizeIn = 8;
	info->tempInBuffer = (void*)SmartNewPtr(bufferSize, &iErr);
	info->inputPixelSize = 16;
	if (iErr != noErr)
	{
		return iErr;
	}
	calcParam->cmLineCount			= linesAtOnce;
	calcParam->cmInputColorSpace	= cmRGB24Space;
	calcParam->cmInputBytesPerLine	= newRowBytes;
	if( reverseOrder ){
		calcParam->inputData[2]	=  (Ptr)info->tempInBuffer;
		calcParam->inputData[1]	= ((Ptr)info->tempInBuffer)+1;
		calcParam->inputData[0]	= ((Ptr)info->tempInBuffer)+2;
	}
	else{
		calcParam->inputData[0]	=  (Ptr)info->tempInBuffer;
		calcParam->inputData[1]	= ((Ptr)info->tempInBuffer)+1;
		calcParam->inputData[2]	= ((Ptr)info->tempInBuffer)+2;
	}
	calcParam->cmInputPixelOffset = 3;
	*theLinesAtOnce = linesAtOnce;
	return noErr;
}
/*--------------------------------------------------------------------------------------------------------------
	CheckInputColorSpace
  --------------------------------------------------------------------------------------------------------------*/
CMError
CheckInputColorSpace (const CMBitmap*	bitMap,
					  CMCalcParamPtr	calcParam,
					  ColorSpaceInfo*	info,
					  OSType			inColorSpace,
					  long 				colorLutInDim )
{
	CMError err = noErr;
	SINT32	newRowBytes;
	SINT32	bufferSize;
	SINT32	linesAtOnce;
	SINT32	loop;
	SINT16	iErr = noErr;
	CMBitmapColorSpace	inputBitMapColorSpace = calcParam->cmInputColorSpace;
#ifdef ALLOW_16BIT_DATA
	UINT8 Byte_Factor=1;
#endif

	LH_START_PROC("CheckInputColorSpace")
	colorLutInDim=colorLutInDim;
	
#ifdef ALLOW_16BIT_DATA
	if( inputBitMapColorSpace & cm16PerChannelPacking && (inputBitMapColorSpace & 31) != cmGraySpace){
		Byte_Factor = 2;
		inputBitMapColorSpace &= ~cm16PerChannelPacking;
		inputBitMapColorSpace |= cm8PerChannelPacking;
	}
#endif
	info->origSizeIn = Byte_Factor*8;
	info->usedSizeIn = Byte_Factor*8;
	switch ( inputBitMapColorSpace )
	{
		case cmNoSpace:
		case cmRGBSpace:			/* "... bitmap never uses this constant alone..." */
		case cmHSVSpace:			/* "... bitmap never uses this constant alone..." */
		case cmHLSSpace:			/* "... bitmap never uses this constant alone..." */
		case cmYXYSpace:			/* "... bitmap never uses this constant alone..." */
		case cmXYZSpace:			/* "... bitmap never uses this constant alone..." */
		case cmLUVSpace:			/* "... bitmap never uses this constant alone..." */
		case cmLABSpace:			/* "... bitmap never uses this constant alone..." */
		case cmMCFiveSpace:			/* "... bitmap never uses this constant alone..." */
		case cmMCSixSpace:			/* "... bitmap never uses this constant alone..." */
		case cmMCSevenSpace:		/* "... bitmap never uses this constant alone..." */
		case cmMCEightSpace:		/* "... bitmap never uses this constant alone..." */
		case cmGamutResultSpace:	/* "... bitmap never uses this constant alone..." */
		case cmGamutResult1Space:	/* not as colorspace for CMMatchBitmap */
#ifdef PI_Application_h
		case cmYCCSpace:			/* "... bitmap never uses this constant alone..." */
		case cmBGRSpace:			/* "... bitmap never uses this constant alone..." */
#endif
			err = cmInvalidSrcMap;
			break;
		case cmCMYKSpace:			/* "... bitmap never uses this constant alone..." */
		case cmKYMCSpace:			/* "... bitmap never uses this constant alone..." */
			err = cmInvalidSrcMap;
#if ! realThing
			if ( (inColorSpace != icSigCmykData) && (inColorSpace != icSigCmyData) )
				err = cmInvalidColorSpace;
			else
			{
				err = noErr;
				calcParam->inputData[0]	= &bitMap->image[0];
				calcParam->inputData[1]	= &bitMap->image[2];
				calcParam->inputData[2]	= &bitMap->image[4];
				calcParam->inputData[3]	= &bitMap->image[6];
				calcParam->cmInputPixelOffset = 8;
				info->origSizeIn = 16;
				info->usedSizeIn = 16;
			}
#endif
			break;
		case cmGraySpace:
			if (inColorSpace != icSigGrayData)
				err = cmInvalidColorSpace;
			else 
			{
				calcParam->inputData[0]	= &bitMap->image[0];
				calcParam->cmInputPixelOffset	= 2;
				info->origSizeIn = 16;
				info->usedSizeIn = 16;
				info->inputPixelSize = 16;
			}
			break;
		case cmGrayASpace:
			if (inColorSpace != icSigGrayData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->inputData[0]	= &bitMap->image[0];
				calcParam->inputData[1]	= &bitMap->image[2];
				calcParam->cmInputPixelOffset	= 4;
				info->origSizeIn = 16;
				info->usedSizeIn = 16;
				info->inputPixelSize = 32;
			}
			break;
		case cmLAB24Space:
			if (inColorSpace != icSigLabData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->inputData[0]	= &bitMap->image[Byte_Factor*0];
				calcParam->inputData[1]	= &bitMap->image[Byte_Factor*1];
				calcParam->inputData[2]	= &bitMap->image[Byte_Factor*2];
				calcParam->cmInputPixelOffset   = Byte_Factor*3;
					info->inputPixelSize = Byte_Factor*24;
			}
			break;

		/* separated cmRGB24Space and cmRGB32Space to reflect the bitmap format definition changes. */
		case cmRGB24Space:
			if (inColorSpace != icSigRgbData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->inputData[0]	= &bitMap->image[Byte_Factor*0];
				calcParam->inputData[1]	= &bitMap->image[Byte_Factor*1];
				calcParam->inputData[2]	= &bitMap->image[Byte_Factor*2];
				calcParam->cmInputPixelOffset	= Byte_Factor*3;
				info->inputPixelSize = Byte_Factor*24;
			}
			break;
		case cmRGB32Space:
			if (inColorSpace != icSigRgbData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->inputData[0]	= &bitMap->image[Byte_Factor*1];
				calcParam->inputData[1]	= &bitMap->image[Byte_Factor*2];
				calcParam->inputData[2]	= &bitMap->image[Byte_Factor*3];
				calcParam->inputData[3]	= &bitMap->image[Byte_Factor*0];
				calcParam->cmInputPixelOffset = Byte_Factor*4;
					info->inputPixelSize = Byte_Factor*32;
			}
			break;

		case cmRGBASpace:
		case cmRGBA32Space:
			if (inColorSpace != icSigRgbData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->inputData[0]	= &bitMap->image[Byte_Factor*0];
				calcParam->inputData[1]	= &bitMap->image[Byte_Factor*1];
				calcParam->inputData[2]	= &bitMap->image[Byte_Factor*2];
				calcParam->inputData[3]	= &bitMap->image[Byte_Factor*3];
				calcParam->cmInputPixelOffset = Byte_Factor*4;
				info->inputPixelSize = Byte_Factor*32;
			}
			break;
			
#ifdef PI_Application_h

		case cmGraySpace|cm8PerChannelPacking:
			if (inColorSpace != icSigGrayData)
				err = cmInvalidColorSpace;
			else 
			{
				calcParam->inputData[0]	= &bitMap->image[0];
				calcParam->cmInputPixelOffset	= 1;
				info->origSizeIn = 8;
				info->usedSizeIn = 8;
				info->inputPixelSize = 8;
			}
			break;
		case cmGrayASpace|cm8PerChannelPacking:
			if (inColorSpace != icSigGrayData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->inputData[0]	= &bitMap->image[0];
				calcParam->inputData[1]	= &bitMap->image[1];
				calcParam->cmInputPixelOffset	= 2;
				info->origSizeIn = 8;
				info->usedSizeIn = 8;
				info->inputPixelSize = 16;
			}
			break;


		case cmBGR24Space:
			if (inColorSpace != icSigRgbData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->inputData[0]	= &bitMap->image[Byte_Factor*2];
				calcParam->inputData[1]	= &bitMap->image[Byte_Factor*1];
				calcParam->inputData[2]	= &bitMap->image[Byte_Factor*0];
				calcParam->cmInputPixelOffset	= Byte_Factor*3;
				info->inputPixelSize = Byte_Factor*24;
			}
			break;
		case cmBGR32Space:
			if (inColorSpace != icSigRgbData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->inputData[0]	= &bitMap->image[Byte_Factor*2];
				calcParam->inputData[1]	= &bitMap->image[Byte_Factor*1];
				calcParam->inputData[2]	= &bitMap->image[Byte_Factor*0];
				calcParam->inputData[3]	= &bitMap->image[Byte_Factor*3];
				calcParam->cmInputPixelOffset = Byte_Factor*4;
				info->inputPixelSize = Byte_Factor*32;
			}
			break;

		case cmYCC32Space:
			if (inColorSpace != icSigYCbCrData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->inputData[0]		= &bitMap->image[Byte_Factor*1];
				calcParam->inputData[1]		= &bitMap->image[Byte_Factor*2];
				calcParam->inputData[2]		= &bitMap->image[Byte_Factor*3];
				calcParam->inputData[3]		= &bitMap->image[Byte_Factor*0];
				calcParam->cmInputPixelOffset	= Byte_Factor*4;
				info->inputPixelSize = Byte_Factor*32;
			}
			break;
		case cmYCC24Space:
			if (inColorSpace != icSigYCbCrData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->inputData[0]	= &bitMap->image[Byte_Factor*0];
				calcParam->inputData[1]	= &bitMap->image[Byte_Factor*1];
				calcParam->inputData[2]	= &bitMap->image[Byte_Factor*2];
				calcParam->cmInputPixelOffset = Byte_Factor*3;
				info->inputPixelSize = Byte_Factor*24;
			}
			break;

		case cmYCCASpace:
		case cmYCCA32Space:
			if (inColorSpace != icSigYCbCrData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->inputData[0]	= &bitMap->image[Byte_Factor*0];
				calcParam->inputData[1]	= &bitMap->image[Byte_Factor*1];
				calcParam->inputData[2]	= &bitMap->image[Byte_Factor*2];
				calcParam->inputData[3]	= &bitMap->image[Byte_Factor*3];
				calcParam->cmInputPixelOffset = Byte_Factor*4;
				info->inputPixelSize = Byte_Factor*32;
			}
			break;
		case cmAYCC32Space:
			if (inColorSpace != icSigYCbCrData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->inputData[0]	= &bitMap->image[Byte_Factor*1];
				calcParam->inputData[1]	= &bitMap->image[Byte_Factor*2];
				calcParam->inputData[2]	= &bitMap->image[Byte_Factor*3];
				calcParam->inputData[3]	= &bitMap->image[Byte_Factor*0];
				calcParam->cmInputPixelOffset = Byte_Factor*4;
				info->inputPixelSize = Byte_Factor*32;
			}
			break;
		case cmLong8ColorPacking + cmLABSpace:
			if (inColorSpace != icSigLabData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->inputData[0]	= &bitMap->image[Byte_Factor*0];
				calcParam->inputData[1]	= &bitMap->image[Byte_Factor*1];
				calcParam->inputData[2]	= &bitMap->image[Byte_Factor*2];
				calcParam->inputData[3]	= &bitMap->image[Byte_Factor*3];
				calcParam->cmInputPixelOffset = Byte_Factor*4;
				info->inputPixelSize = Byte_Factor*32;
			}
			break;
		case cmLong8ColorPacking + cmXYZSpace:
			if (inColorSpace != icSigXYZData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->inputData[0]	= &bitMap->image[Byte_Factor*0];
				calcParam->inputData[1]	= &bitMap->image[Byte_Factor*1];
				calcParam->inputData[2]	= &bitMap->image[Byte_Factor*2];
				calcParam->inputData[3]	= &bitMap->image[Byte_Factor*3];
				calcParam->cmInputPixelOffset = Byte_Factor*4;
				info->inputPixelSize = Byte_Factor*32;
			}
			break;
		case cmLong8ColorPacking + cmYXYSpace:
			if (inColorSpace != icSigYxyData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->inputData[0]	= &bitMap->image[Byte_Factor*0];
				calcParam->inputData[1]	= &bitMap->image[Byte_Factor*1];
				calcParam->inputData[2]	= &bitMap->image[Byte_Factor*2];
				calcParam->inputData[3]	= &bitMap->image[Byte_Factor*3];
				calcParam->cmInputPixelOffset = Byte_Factor*4;
				info->inputPixelSize = Byte_Factor*32;
			}
			break;
		case cmGenericSpace + cm8PerChannelPacking:
			{
				for (loop = 0; loop< colorLutInDim; loop++)
					calcParam->inputData[loop]	= &bitMap->image[Byte_Factor*loop];
				calcParam->cmInputPixelOffset = Byte_Factor*colorLutInDim;
				info->inputPixelSize = Byte_Factor*colorLutInDim*8;
			}
			break;
		case cmGenericSpace + cmLong8ColorPacking:
			{
				for (loop = 0; loop< (colorLutInDim+1); loop++)
					calcParam->inputData[loop]	= &bitMap->image[Byte_Factor*loop];
				calcParam->cmInputPixelOffset = Byte_Factor*(colorLutInDim+1);
				info->inputPixelSize = Byte_Factor*(colorLutInDim+1)*8;
			}
			break;
		case cmCMY24Space:
			if ( inColorSpace != icSigCmyData )
				err = cmInvalidColorSpace;
			else
			{
				calcParam->inputData[0]	= &bitMap->image[Byte_Factor*0];
				calcParam->inputData[1]	= &bitMap->image[Byte_Factor*1];
				calcParam->inputData[2]	= &bitMap->image[Byte_Factor*2];
				calcParam->cmInputPixelOffset = Byte_Factor*3;
				info->inputPixelSize = Byte_Factor*24;
			}
			break;
#endif

		case cmCMYK32Space:
			if ( (inColorSpace != icSigCmykData) && (inColorSpace != icSigCmyData) )
				err = cmInvalidColorSpace;
			else
			{
				calcParam->inputData[0]	= &bitMap->image[Byte_Factor*0];
				calcParam->inputData[1]	= &bitMap->image[Byte_Factor*1];
				calcParam->inputData[2]	= &bitMap->image[Byte_Factor*2];
				calcParam->inputData[3]	= &bitMap->image[Byte_Factor*3];
				calcParam->cmInputPixelOffset = Byte_Factor*4;
				info->inputPixelSize = Byte_Factor*32;
			}
			break;
		case cmKYMC32Space:
			if ( (inColorSpace != icSigCmykData) && (inColorSpace != icSigCmyData) )
				err = cmInvalidColorSpace;
			else
			{
				calcParam->inputData[3]	= &bitMap->image[Byte_Factor*0];
				calcParam->inputData[2]	= &bitMap->image[Byte_Factor*1];
				calcParam->inputData[1]	= &bitMap->image[Byte_Factor*2];
				calcParam->inputData[0]	= &bitMap->image[Byte_Factor*3];
				calcParam->cmInputPixelOffset = Byte_Factor*4;
				info->inputPixelSize = Byte_Factor*32;
			}
			break;
		case cmARGB32Space:
			if (inColorSpace != icSigRgbData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->inputData[0]	= &bitMap->image[Byte_Factor*1];
				calcParam->inputData[1]	= &bitMap->image[Byte_Factor*2];
				calcParam->inputData[2]	= &bitMap->image[Byte_Factor*3];
				calcParam->inputData[3]	= &bitMap->image[Byte_Factor*0];
				calcParam->cmInputPixelOffset = Byte_Factor*4;
				info->inputPixelSize = Byte_Factor*32;
			}
			break;
		case cmMCFive8Space:
			if (inColorSpace != icSigMCH5Data)
				err = cmInvalidColorSpace;
			else
			{
				for (loop = 0; loop< 5; loop++)
					calcParam->inputData[loop]	= &bitMap->image[Byte_Factor*loop];
				calcParam->cmInputPixelOffset = Byte_Factor*5;
				info->inputPixelSize = Byte_Factor*40;
			}
			break;
		case cmMCSix8Space:
			if (inColorSpace != icSigMCH6Data)
				err = cmInvalidColorSpace;
			else
			{
				for (loop = 0; loop< 6; loop++)
					calcParam->inputData[loop]	= &bitMap->image[Byte_Factor*loop];
				calcParam->cmInputPixelOffset = Byte_Factor*6;
				info->inputPixelSize = Byte_Factor*48;
			}
			break;
		case cmMCSeven8Space:
			if (inColorSpace != icSigMCH7Data)
				err = cmInvalidColorSpace;
			else
			{
				for (loop = 0; loop< 7; loop++)
					calcParam->inputData[loop]	= &bitMap->image[Byte_Factor*loop];
				calcParam->cmInputPixelOffset = Byte_Factor*7;
				info->inputPixelSize = Byte_Factor*56;
			}
			break;
		case cmMCEight8Space:
			if (inColorSpace != icSigMCH8Data)
				err = cmInvalidColorSpace;
			else
			{
				for (loop = 0; loop< 8; loop++)
					calcParam->inputData[loop]	= &bitMap->image[Byte_Factor*loop];
				calcParam->cmInputPixelOffset = Byte_Factor*8;
				info->inputPixelSize = Byte_Factor*64;
			}
			break;
#if ( CM_MAX_COLOR_CHANNELS == 15 )
		case cmMC98Space:
		case cmMCa8Space:
		case cmMCb8Space:
		case cmMCc8Space:
		case cmMCd8Space:
		case cmMCe8Space:
		case cmMCf8Space:
		case cmMC28Space:
			if (inColorSpace != icSigMCH8Data)
				err = cmInvalidColorSpace;
			else
			{
				for (loop = 0; loop< 8; loop++)
					calcParam->inputData[loop]	= &bitMap->image[Byte_Factor*loop];
				calcParam->cmInputPixelOffset = Byte_Factor*8;
				info->inputPixelSize = Byte_Factor*64;
			}
			break;
#endif
#ifdef PI_Application_h
		case cmWord5ColorPacking + cmLABSpace:
			if (inColorSpace != icSigLabData)
				err = cmInvalidColorSpace;
			else{
				newRowBytes = calcParam->cmInputBytesPerLine;
				err = Do555To8Setup( calcParam, info, &linesAtOnce, 0 );
				if( err ) goto CleanupAndExit;
			}
			break;
		case cmWord5ColorPacking + cmXYZSpace:
			if (inColorSpace != icSigXYZData)
				err = cmInvalidColorSpace;
			else{
				newRowBytes = calcParam->cmInputBytesPerLine;
				err = Do555To8Setup( calcParam, info, &linesAtOnce, 0 );
				if( err ) goto CleanupAndExit;
			}
			break;
		case cmWord5ColorPacking + cmYXYSpace:
			if (inColorSpace != icSigYxyData)
				err = cmInvalidColorSpace;
			else{
				newRowBytes = calcParam->cmInputBytesPerLine;
				err = Do555To8Setup( calcParam, info, &linesAtOnce, 0 );
				if( err ) goto CleanupAndExit;
			}
			break;
		case cmWord5ColorPacking + cmGenericSpace:
			{
				newRowBytes = calcParam->cmInputBytesPerLine;
				err = Do555To8Setup( calcParam, info, &linesAtOnce, 0 );
				if( err ) goto CleanupAndExit;
			}
			break;
		case cmWord565ColorPacking + cmGenericSpace:
			{
				newRowBytes = calcParam->cmInputBytesPerLine;
				err = Do555To8Setup( calcParam, info, &linesAtOnce, 0 );
				if( err ) goto CleanupAndExit;
				info->origSizeIn = 6;
			}
			break;
		case cmWord5ColorPacking + cmBGRSpace:
			if (inColorSpace != icSigRgbData)
				err = cmInvalidColorSpace;
			else{
				newRowBytes = calcParam->cmInputBytesPerLine;
				err = Do555To8Setup( calcParam, info, &linesAtOnce, 1 );
				if( err ) goto CleanupAndExit;
			}
			break;
		case cmWord565ColorPacking + cmBGRSpace:
			if (inColorSpace != icSigRgbData)
				err = cmInvalidColorSpace;
			else{
				newRowBytes = calcParam->cmInputBytesPerLine;
				err = Do555To8Setup( calcParam, info, &linesAtOnce, 1 );
				if( err ) goto CleanupAndExit;
				info->origSizeIn = 6;
			}
			break;
		case cmRGB16_565Space:
#endif
		case cmRGB16Space:
			if (inColorSpace != icSigRgbData)
				err = cmInvalidColorSpace;
			else{
				newRowBytes = calcParam->cmInputBytesPerLine;
				err = Do555To8Setup( calcParam, info, &linesAtOnce, 0 );
				if( err ) goto CleanupAndExit;
#ifdef PI_Application_h
				if( inputBitMapColorSpace == cmRGB16_565Space ){
					info->origSizeIn = 6;
				}
#endif
			}
			break;
		case cmNamedIndexed32Space:
			if( (inputBitMapColorSpace == cmNamedIndexed32Space) && (inColorSpace != icSigNamedData)){
				err = cmInvalidColorSpace;
			}
			else{
				newRowBytes = calcParam->cmPixelPerLine * 3;	/* TempBuffer -> 32 BIT */
				linesAtOnce = (kMaxTempBlock) / newRowBytes;
				if (linesAtOnce == 0)
					linesAtOnce = 1;
				else if (linesAtOnce > calcParam->cmLineCount)
					linesAtOnce = calcParam->cmLineCount;
					
				bufferSize = newRowBytes * linesAtOnce;
				info->processedLinesIn = 0;
				info->origSizeIn = 8;
				info->usedSizeIn = 8;
				info->tempInBuffer = (void*)SmartNewPtr(bufferSize, &iErr);
				info->inputPixelSize = 32;
				if (iErr != noErr)
				{
					err = iErr;
					goto CleanupAndExit;
				}
	
				calcParam->cmLineCount			= linesAtOnce;
				calcParam->cmInputColorSpace = cmLAB24Space;
				calcParam->inputData[0]	= ((Ptr)info->tempInBuffer)+0;
				calcParam->inputData[1]	= ((Ptr)info->tempInBuffer)+1;
				calcParam->inputData[2]	= ((Ptr)info->tempInBuffer)+2;

				calcParam->cmInputBytesPerLine	= newRowBytes;
				calcParam->cmInputPixelOffset = 3;
			}
			break;
		case cmHSV32Space:
		case cmHLS32Space:
		case cmYXY32Space:
		case cmXYZ32Space:
		case cmLUV32Space:
		case cmLAB32Space:
#ifdef PI_Application_h
		case cmBGRSpace + cmLong10ColorPacking:
		case cmRGBSpace + cmLong10ColorPacking:
		case cmGenericSpace + cmLong10ColorPacking:
#endif
			if(  (	((inputBitMapColorSpace == cmHSV32Space) && (inColorSpace != icSigHsvData)) ||
					((inputBitMapColorSpace == cmHLS32Space) && (inColorSpace != icSigHlsData)) ||
					((inputBitMapColorSpace == cmYXY32Space) && (inColorSpace != icSigYxyData)) ||
					((inputBitMapColorSpace == cmXYZ32Space) && (inColorSpace != icSigXYZData)) ||
					((inputBitMapColorSpace == cmLUV32Space) && (inColorSpace != icSigLuvData)) ||
					((inputBitMapColorSpace == cmLAB32Space) && (inColorSpace != icSigLabData))  
#ifdef PI_Application_h
					|| ((inputBitMapColorSpace == cmBGRSpace + cmLong10ColorPacking) && (inColorSpace != icSigRgbData)) 
					|| ((inputBitMapColorSpace == cmRGBSpace + cmLong10ColorPacking) && (inColorSpace != icSigRgbData)) 
					|| ((inputBitMapColorSpace == cmGenericSpace + cmLong10ColorPacking) && (inColorSpace != icSigMCH3Data)) 
#endif
					)
#if ! realThing
					&& FALSE  
#endif
				)
				err = cmInvalidColorSpace;
			else{
				newRowBytes = calcParam->cmPixelPerLine * 3 * sizeof(SINT16);	/* TempBuffer -> cm16PerChannelPacking */
				linesAtOnce = (kMaxTempBlock) / newRowBytes;
				if (linesAtOnce == 0)
					linesAtOnce = 1;
				else if (linesAtOnce > calcParam->cmLineCount)
					linesAtOnce = calcParam->cmLineCount;
					
				bufferSize = newRowBytes * linesAtOnce;
				info->processedLinesIn = 0;
				info->origSizeIn = 10;
				info->usedSizeIn = 16;
				info->tempInBuffer = (void*)SmartNewPtr(bufferSize, &iErr);
				info->inputPixelSize = 32;
				if (iErr != noErr)
				{
					err = iErr;
					goto CleanupAndExit;
				}
	
				calcParam->cmLineCount			= linesAtOnce;
				if( inputBitMapColorSpace == cmBGRSpace + cmLong10ColorPacking ){
					calcParam->cmInputColorSpace = cmBGRSpace;
					calcParam->inputData[0]	= ((Ptr)info->tempInBuffer)+4;
					calcParam->inputData[1]	= ((Ptr)info->tempInBuffer)+2;
					calcParam->inputData[2]	= ((Ptr)info->tempInBuffer)+0;
				}
				else{
					calcParam->cmInputColorSpace = cmRGBSpace;
					calcParam->inputData[0]	= ((Ptr)info->tempInBuffer)+0;
					calcParam->inputData[1]	= ((Ptr)info->tempInBuffer)+2;
					calcParam->inputData[2]	= ((Ptr)info->tempInBuffer)+4;
				}
				calcParam->cmInputBytesPerLine	= newRowBytes;
				calcParam->cmInputPixelOffset = 6;
			}
			break;
		default:
			err = cmInvalidSrcMap;
			break;
	}
	
CleanupAndExit:
	LH_END_PROC("CheckInputColorSpace")
	return err;
}

/*--------------------------------------------------------------------------------------------------------------
	CheckOutputColorSpace
  --------------------------------------------------------------------------------------------------------------*/
CMError
CheckOutputColorSpace (const CMBitmap*	bitMap,
					   CMCalcParamPtr	calcParam,
					   ColorSpaceInfo*	info,
					   OSType			outColorSpace,
					   long				colorLutOutDim )
{
	CMError err = noErr;
	SINT32	newRowBytes;
	SINT32	bufferSize;
	SINT32	linesAtOnce;
	SINT32	loop;
	SINT16	iErr = noErr;
	CMBitmapColorSpace	outputBitMapColorSpace = calcParam->cmOutputColorSpace;
#ifdef ALLOW_16BIT_DATA
	UINT8 Byte_Factor=1;
#endif

	LH_START_PROC("CheckOutputColorSpace")
	colorLutOutDim = colorLutOutDim;
	
#ifdef ALLOW_16BIT_DATA
	if( outputBitMapColorSpace & cm16PerChannelPacking && (outputBitMapColorSpace & 31) != cmGraySpace){
		Byte_Factor = 2;
		outputBitMapColorSpace &= ~cm16PerChannelPacking;
		outputBitMapColorSpace |= cm8PerChannelPacking;
	}
#endif
	info->origSizeOut = Byte_Factor*8;
	info->usedSizeOut = Byte_Factor*8;
	switch ( outputBitMapColorSpace )
	{
		case cmNoSpace:
		case cmRGBSpace:			/* "... bitmap never uses this constant alone..." */
		case cmHSVSpace:			/* "... bitmap never uses this constant alone..." */
		case cmHLSSpace:			/* "... bitmap never uses this constant alone..." */
		case cmYXYSpace:			/* "... bitmap never uses this constant alone..." */
		case cmXYZSpace:			/* "... bitmap never uses this constant alone..." */
		case cmLUVSpace:			/* "... bitmap never uses this constant alone..." */
		case cmLABSpace:			/* "... bitmap never uses this constant alone..." */
		case cmMCFiveSpace:			/* "... bitmap never uses this constant alone..." */
		case cmMCSixSpace:			/* "... bitmap never uses this constant alone..." */
		case cmMCSevenSpace:		/* "... bitmap never uses this constant alone..." */
		case cmMCEightSpace:		/* "... bitmap never uses this constant alone..." */
		case cmGamutResultSpace:	/* "... bitmap never uses this constant alone..." */
		case cmGamutResult1Space:	/* not as colorspace for CMMatchBitmap */
#ifdef PI_Application_h
		case cmYCCSpace:			/* "... bitmap never uses this constant alone..." */
		case cmBGRSpace:			/* "... bitmap never uses this constant alone..." */
#endif
			err = cmInvalidDstMap;
			break;
		case cmCMYKSpace:			/* "... bitmap never uses this constant alone..." */
		case cmKYMCSpace:			/* "... bitmap never uses this constant alone..." */
			err = cmInvalidDstMap;
#if ! realThing
			if ( (outColorSpace != icSigCmykData) && (outColorSpace != icSigCmyData) )
				err = cmInvalidColorSpace;
			else
			{
				calcParam->outputData[0]		= &bitMap->image[0];
				calcParam->outputData[1]		= &bitMap->image[2];
				calcParam->outputData[2]		= &bitMap->image[4];
				calcParam->outputData[3]		= &bitMap->image[6];
				calcParam->cmOutputPixelOffset	= 8;
				info->usedSizeOut = 16;
			}
#endif
			break;
		case cmGraySpace:
			if (outColorSpace != icSigGrayData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->outputData[0]		= &bitMap->image[0];
				calcParam->cmOutputPixelOffset	= 2;
				info->usedSizeOut = 16;
				info->outputPixelSize = 16;
			}
			break;
		case cmGrayASpace:
			if (outColorSpace != icSigGrayData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->outputData[0]		= &bitMap->image[0];
				calcParam->outputData[1]		= &bitMap->image[2];
				calcParam->cmOutputPixelOffset	= 4;
				info->usedSizeOut = 16;
				info->outputPixelSize = 32;
			}
			break;
		case cmLAB24Space:
			if (outColorSpace != icSigLabData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->outputData[0]	= &bitMap->image[Byte_Factor*0];
				calcParam->outputData[1]	= &bitMap->image[Byte_Factor*1];
				calcParam->outputData[2]	= &bitMap->image[Byte_Factor*2];
				calcParam->cmOutputPixelOffset = Byte_Factor*3;
				info->outputPixelSize = Byte_Factor*24;
			}
			break;

		/* separated cmRGB24Space and cmRGB32Space to reflect the bitmap format definition changes. */
		case cmRGB24Space:
			if (outColorSpace != icSigRgbData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->outputData[0]	= &bitMap->image[Byte_Factor*0];
				calcParam->outputData[1]	= &bitMap->image[Byte_Factor*1];
				calcParam->outputData[2]	= &bitMap->image[Byte_Factor*2];
				calcParam->cmOutputPixelOffset = Byte_Factor*3;
				info->outputPixelSize = Byte_Factor*24;
			}
			break;
		case cmRGB32Space:
			if (outColorSpace != icSigRgbData)
				err = cmInvalidColorSpace;
			else
				{
				calcParam->outputData[0]		= &bitMap->image[Byte_Factor*1];
				calcParam->outputData[1]		= &bitMap->image[Byte_Factor*2];
				calcParam->outputData[2]		= &bitMap->image[Byte_Factor*3];
				calcParam->outputData[3]		= &bitMap->image[Byte_Factor*0];
				calcParam->cmOutputPixelOffset	= Byte_Factor*4;
				calcParam->clearMask			= TRUE;
				info->outputPixelSize = Byte_Factor*32;
			}
			break;

		case cmRGBASpace:
		case cmRGBA32Space:
			if (outColorSpace != icSigRgbData)
				err = cmInvalidColorSpace;
				else
				{
				calcParam->outputData[0]	= &bitMap->image[Byte_Factor*0];
				calcParam->outputData[1]	= &bitMap->image[Byte_Factor*1];
				calcParam->outputData[2]	= &bitMap->image[Byte_Factor*2];
				calcParam->outputData[3]	= &bitMap->image[Byte_Factor*3];
				calcParam->cmOutputPixelOffset = Byte_Factor*4;
				info->outputPixelSize = Byte_Factor*32;
			}
			break;

#ifdef PI_Application_h
		case cmGraySpace|cm8PerChannelPacking:
			if (outColorSpace != icSigGrayData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->outputData[0]		= &bitMap->image[0];
				calcParam->cmOutputPixelOffset	= 1;
				info->usedSizeOut = 8;
				info->outputPixelSize = 8;
			}
			break;
		case cmGrayASpace|cm8PerChannelPacking:
			if (outColorSpace != icSigGrayData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->outputData[0]		= &bitMap->image[0];
				calcParam->outputData[1]		= &bitMap->image[1];
				calcParam->cmOutputPixelOffset	= 2;
				info->usedSizeOut = 8;
				info->outputPixelSize = 16;
			}
			break;

		case cmBGR24Space:
			if (outColorSpace != icSigRgbData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->outputData[0]	= &bitMap->image[Byte_Factor*2];
				calcParam->outputData[1]	= &bitMap->image[Byte_Factor*1];
				calcParam->outputData[2]	= &bitMap->image[Byte_Factor*0];
				calcParam->cmOutputPixelOffset = Byte_Factor*3;
				info->outputPixelSize = Byte_Factor*24;
			}
			break;
		case cmBGR32Space:
			if (outColorSpace != icSigRgbData)
				err = cmInvalidColorSpace;
			else
				{
				calcParam->outputData[0]		= &bitMap->image[Byte_Factor*2];
				calcParam->outputData[1]		= &bitMap->image[Byte_Factor*1];
				calcParam->outputData[2]		= &bitMap->image[Byte_Factor*0];
				calcParam->outputData[3]		= &bitMap->image[Byte_Factor*3];
				calcParam->cmOutputPixelOffset	= Byte_Factor*4;
				calcParam->clearMask			= FALSE;
				calcParam->copyAlpha			= TRUE;
				info->outputPixelSize = Byte_Factor*32;
			}
			break;

		case cmYCC24Space:		/* cmYCC24Space and cmYCC32Space have the same value !!!	*/
			if (outColorSpace != icSigYCbCrData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->outputData[0]	= &bitMap->image[Byte_Factor*0];
				calcParam->outputData[1]	= &bitMap->image[Byte_Factor*1];
				calcParam->outputData[2]	= &bitMap->image[Byte_Factor*2];
				calcParam->cmOutputPixelOffset = Byte_Factor*3;
				info->outputPixelSize = Byte_Factor*24;
			}
			break;
		case cmYCC32Space:
			if (outColorSpace != icSigYCbCrData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->outputData[0]		= &bitMap->image[Byte_Factor*1];
				calcParam->outputData[1]		= &bitMap->image[Byte_Factor*2];
				calcParam->outputData[2]		= &bitMap->image[Byte_Factor*3];
				calcParam->outputData[3]		= &bitMap->image[Byte_Factor*0];
				calcParam->cmOutputPixelOffset	= Byte_Factor*4;
				calcParam->clearMask			= TRUE;
				info->outputPixelSize = Byte_Factor*32;
			}
			break;

		case cmYCCASpace:
		case cmYCCA32Space:
			if (outColorSpace != icSigYCbCrData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->outputData[0]	= &bitMap->image[Byte_Factor*0];
				calcParam->outputData[1]	= &bitMap->image[Byte_Factor*1];
				calcParam->outputData[2]	= &bitMap->image[Byte_Factor*2];
				calcParam->outputData[3]	= &bitMap->image[Byte_Factor*3];
				calcParam->cmOutputPixelOffset = Byte_Factor*4;
				info->outputPixelSize = Byte_Factor*32;
			}
			break;
		case cmAYCC32Space:
			if (outColorSpace != icSigYCbCrData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->outputData[0]	= &bitMap->image[Byte_Factor*1];
				calcParam->outputData[1]	= &bitMap->image[Byte_Factor*2];
				calcParam->outputData[2]	= &bitMap->image[Byte_Factor*3];
				calcParam->outputData[3]	= &bitMap->image[Byte_Factor*0];
				calcParam->cmOutputPixelOffset = Byte_Factor*4;
				info->outputPixelSize = Byte_Factor*32;
			}
			break;
		case cmLong8ColorPacking + cmLABSpace:
			if (outColorSpace != icSigLabData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->outputData[0]	= &bitMap->image[Byte_Factor*0];
				calcParam->outputData[1]	= &bitMap->image[Byte_Factor*1];
				calcParam->outputData[2]	= &bitMap->image[Byte_Factor*2];
				calcParam->outputData[3]	= &bitMap->image[Byte_Factor*3];
				calcParam->cmOutputPixelOffset = Byte_Factor*4;
				info->outputPixelSize = Byte_Factor*32;
			}
			break;
		case cmLong8ColorPacking + cmXYZSpace:
			if (outColorSpace != icSigXYZData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->outputData[0]	= &bitMap->image[Byte_Factor*0];
				calcParam->outputData[1]	= &bitMap->image[Byte_Factor*1];
				calcParam->outputData[2]	= &bitMap->image[Byte_Factor*2];
				calcParam->outputData[3]	= &bitMap->image[Byte_Factor*3];
				calcParam->cmOutputPixelOffset = Byte_Factor*4;
				info->outputPixelSize = Byte_Factor*32;
			}
			break;
		case cmLong8ColorPacking + cmYXYSpace:
			if (outColorSpace != icSigYxyData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->outputData[0]	= &bitMap->image[Byte_Factor*0];
				calcParam->outputData[1]	= &bitMap->image[Byte_Factor*1];
				calcParam->outputData[2]	= &bitMap->image[Byte_Factor*2];
				calcParam->outputData[3]	= &bitMap->image[Byte_Factor*3];
				calcParam->cmOutputPixelOffset = Byte_Factor*4;
				info->outputPixelSize = Byte_Factor*32;
			}
			break;
		case cmGenericSpace + cm8PerChannelPacking:
			{
				for (loop = 0; loop< colorLutOutDim; loop++)
					calcParam->outputData[loop]	= &bitMap->image[Byte_Factor*loop];
				calcParam->cmOutputPixelOffset = Byte_Factor*colorLutOutDim;
				info->outputPixelSize = Byte_Factor*colorLutOutDim*8;
			}
			break;
		case cmGenericSpace + cmLong8ColorPacking:
			{
				for (loop = 0; loop< (colorLutOutDim+1); loop++)
					calcParam->outputData[loop]	= &bitMap->image[Byte_Factor*loop];
				calcParam->cmOutputPixelOffset = Byte_Factor*(colorLutOutDim+1);
				info->outputPixelSize = Byte_Factor*(colorLutOutDim+1)*8;
			}
			break;
		case cmCMY24Space:
			if ( outColorSpace != icSigCmyData )
				err = cmInvalidColorSpace;
			else
			{
				calcParam->outputData[0]	= &bitMap->image[Byte_Factor*0];
				calcParam->outputData[1]	= &bitMap->image[Byte_Factor*1];
				calcParam->outputData[2]	= &bitMap->image[Byte_Factor*2];
				calcParam->cmOutputPixelOffset = Byte_Factor*3;
				info->outputPixelSize = Byte_Factor*24;
			}
			break;
#endif

		case cmCMYK32Space:
			if ( (outColorSpace != icSigCmykData) && (outColorSpace != icSigCmyData) )
				err = cmInvalidColorSpace;
			else
			{
				calcParam->outputData[0]	= &bitMap->image[Byte_Factor*0];
				calcParam->outputData[1]	= &bitMap->image[Byte_Factor*1];
				calcParam->outputData[2]	= &bitMap->image[Byte_Factor*2];
				calcParam->outputData[3]	= &bitMap->image[Byte_Factor*3];
				calcParam->cmOutputPixelOffset = Byte_Factor*4;
				if (outColorSpace == icSigCmyData)					/* if we have CMY-Data - clear the k */
					calcParam->clearMask	= TRUE;
				info->outputPixelSize = Byte_Factor*32;
			}
			break;
		case cmKYMC32Space:
			if ( (outColorSpace != icSigCmykData) && (outColorSpace != icSigCmyData) )
				err = cmInvalidColorSpace;
			else
			{
				calcParam->outputData[3]	= &bitMap->image[Byte_Factor*0];
				calcParam->outputData[2]	= &bitMap->image[Byte_Factor*1];
				calcParam->outputData[1]	= &bitMap->image[Byte_Factor*2];
				calcParam->outputData[0]	= &bitMap->image[Byte_Factor*3];
				calcParam->cmOutputPixelOffset = Byte_Factor*4;
				if (outColorSpace == icSigCmyData)					/* if we have CMY-Data - clear the k */
					calcParam->clearMask	= TRUE;
				info->outputPixelSize = Byte_Factor*32;
			}
			break;
		case cmARGB32Space:
			if (outColorSpace != icSigRgbData)
				err = cmInvalidColorSpace;
			else
			{
				calcParam->outputData[0]	= &bitMap->image[Byte_Factor*1];
				calcParam->outputData[1]	= &bitMap->image[Byte_Factor*2];
				calcParam->outputData[2]	= &bitMap->image[Byte_Factor*3];
				calcParam->outputData[3]	= &bitMap->image[Byte_Factor*0];
				calcParam->cmOutputPixelOffset =Byte_Factor*4;
				info->outputPixelSize = Byte_Factor*32;
			}
			break;
		case cmMCFive8Space:
			if (outColorSpace != icSigMCH5Data)
				err = cmInvalidColorSpace;
			else
			{
				for (loop = 0; loop< 5; loop++)
					calcParam->outputData[loop]	= &bitMap->image[Byte_Factor*loop];
				calcParam->cmOutputPixelOffset = Byte_Factor*5;
				info->outputPixelSize = Byte_Factor*40;
			}
			break;
		case cmMCSix8Space:
			if (outColorSpace != icSigMCH6Data)
				err = cmInvalidColorSpace;
			else
			{
				for (loop = 0; loop< 6; loop++)
					calcParam->outputData[loop]	= &bitMap->image[Byte_Factor*loop];
				calcParam->cmOutputPixelOffset = Byte_Factor*6;
				info->outputPixelSize = Byte_Factor*48;
			}
			break;
		case cmMCSeven8Space:
			if (outColorSpace != icSigMCH7Data)
				err = cmInvalidColorSpace;
			else
			{
				for (loop = 0; loop< 7; loop++)
					calcParam->outputData[loop]	= &bitMap->image[Byte_Factor*loop];
				calcParam->cmOutputPixelOffset = Byte_Factor*7;
				info->outputPixelSize = Byte_Factor*56;
			}
			break;
		case cmMCEight8Space:
			if (outColorSpace != icSigMCH8Data)
				err = cmInvalidColorSpace;
			else
			{
				for (loop = 0; loop< 8; loop++)
					calcParam->outputData[loop]	= &bitMap->image[Byte_Factor*loop];
				calcParam->cmOutputPixelOffset = Byte_Factor*8;
				info->outputPixelSize = Byte_Factor*64;
			}
			break;
#ifdef PI_Application_h
		case cmWord5ColorPacking + cmLABSpace:
			if (outColorSpace != icSigLabData)
				err = cmInvalidColorSpace;
			else{
				err = Do8To555Setup( calcParam, info, &linesAtOnce, 0 );
				if( err ) goto CleanupAndExit;
			}
			break;
		case cmWord5ColorPacking + cmXYZSpace:
			if (outColorSpace != icSigXYZData)
				err = cmInvalidColorSpace;
			else{
				err = Do8To555Setup( calcParam, info, &linesAtOnce, 0 );
				if( err ) goto CleanupAndExit;
			}
			break;
		case cmWord5ColorPacking + cmYXYSpace:
			if (outColorSpace != icSigYxyData)
				err = cmInvalidColorSpace;
			else{
				err = Do8To555Setup( calcParam, info, &linesAtOnce, 0 );
				if( err ) goto CleanupAndExit;
			}
			break;
		case cmWord5ColorPacking + cmGenericSpace:
			{
				err = Do8To555Setup( calcParam, info, &linesAtOnce, 0 );
				if( err ) goto CleanupAndExit;
			}
			break;
		case cmWord565ColorPacking + cmGenericSpace:
			{
				err = Do8To555Setup( calcParam, info, &linesAtOnce, 0 );
				if( err ) goto CleanupAndExit;
				info->origSizeOut = 6;
			}
			break;
		case cmWord5ColorPacking + cmBGRSpace:
			if (outColorSpace != icSigRgbData)
				err = cmInvalidColorSpace;
			else{
				err = Do8To555Setup( calcParam, info, &linesAtOnce, 1 );
				if( err ) goto CleanupAndExit;
			}
			break;
		case cmWord565ColorPacking + cmBGRSpace:
			if (outColorSpace != icSigRgbData)
				err = cmInvalidColorSpace;
			else{
				err = Do8To555Setup( calcParam, info, &linesAtOnce, 1 );
				if( err ) goto CleanupAndExit;
				info->origSizeOut = 6;
			}
			break;
		case cmRGB16_565Space:
#endif
		case cmRGB16Space:
			if (outColorSpace != icSigRgbData)
				err = cmInvalidColorSpace;
			else{
				err = Do8To555Setup( calcParam, info, &linesAtOnce, 0 );
				if( err ) goto CleanupAndExit;
#ifdef PI_Application_h
				if( outputBitMapColorSpace == cmRGB16_565Space ){
					info->origSizeOut = 6;
				}
#endif
			}
			break;
		case cmNamedIndexed32Space:
			if( (outColorSpace == cmNamedIndexed32Space) && (outColorSpace != icSigNamedData)){
				err = cmInvalidColorSpace;
			}
			else{
				calcParam->outputData[0]	= &bitMap->image[0];
				calcParam->outputData[1]	= &bitMap->image[1];
				calcParam->outputData[2]	= &bitMap->image[2];
				calcParam->outputData[3]	= &bitMap->image[3];
				calcParam->cmOutputPixelOffset = 4;
				info->outputPixelSize = 32;
			}
			break;
		case cmHSV32Space:
		case cmHLS32Space:
		case cmYXY32Space:
		case cmXYZ32Space:
		case cmLUV32Space:
		case cmLAB32Space:
#ifdef PI_Application_h
		case cmBGRSpace + cmLong10ColorPacking:
		case cmRGBSpace + cmLong10ColorPacking:
		case cmGenericSpace + cmLong10ColorPacking:
#endif
			if ( (  ((outputBitMapColorSpace == cmHSV32Space) && (outColorSpace != icSigHsvData)) ||
					((outputBitMapColorSpace == cmHLS32Space) && (outColorSpace != icSigHlsData)) ||
					((outputBitMapColorSpace == cmYXY32Space) && (outColorSpace != icSigYxyData)) ||
					((outputBitMapColorSpace == cmXYZ32Space) && (outColorSpace != icSigXYZData)) ||
					((outputBitMapColorSpace == cmLUV32Space) && (outColorSpace != icSigLuvData)) ||
					((outputBitMapColorSpace == cmLAB32Space) && (outColorSpace != icSigLabData)) 
#ifdef PI_Application_h
					|| ((outputBitMapColorSpace == cmBGRSpace + cmLong10ColorPacking) && (outColorSpace != icSigRgbData)) 
					|| ((outputBitMapColorSpace == cmRGBSpace + cmLong10ColorPacking) && (outColorSpace != icSigRgbData)) 
					|| ((outputBitMapColorSpace == cmGenericSpace + cmLong10ColorPacking) && (outColorSpace != icSigMCH3Data)) 
#endif
					)
#if ! realThing
					&& FALSE
#endif
				)
				err = cmInvalidColorSpace;
			else{
				newRowBytes = calcParam->cmPixelPerLine * 3 * sizeof(SINT16);	/* TempBuffer -> cm16PerChannelPacking */
				linesAtOnce = (kMaxTempBlock) / newRowBytes;
				if (linesAtOnce == 0)
					linesAtOnce = 1;
				else if (linesAtOnce > calcParam->cmLineCount)
					linesAtOnce = calcParam->cmLineCount;
					
				bufferSize = newRowBytes * linesAtOnce;
				info->processedLinesOut = 0;
				info->origSizeOut 		= 10;
				info->usedSizeOut 		= 16;
				info->tempOutBuffer 	= (void*)SmartNewPtr(bufferSize, &iErr);
				info->outputPixelSize = 32;
				if (iErr != noErr)
				{
					err = iErr;
					goto CleanupAndExit;
				}
				
				calcParam->cmLineCount			= linesAtOnce;
				calcParam->cmOutputBytesPerLine	= newRowBytes;
				if( outputBitMapColorSpace == cmBGRSpace + cmLong10ColorPacking ){
					calcParam->cmOutputColorSpace = cmBGRSpace;
					calcParam->outputData[0]		= ((Ptr)info->tempOutBuffer)+4;
					calcParam->outputData[1]		= ((Ptr)info->tempOutBuffer)+2;
					calcParam->outputData[2]		= ((Ptr)info->tempOutBuffer)+0;
				}
				else{
					calcParam->cmOutputColorSpace	= cmRGBSpace;
					calcParam->outputData[0]		= ((Ptr)info->tempOutBuffer)+0;
					calcParam->outputData[1]		= ((Ptr)info->tempOutBuffer)+2;
					calcParam->outputData[2]		= ((Ptr)info->tempOutBuffer)+4;
				}
				calcParam->cmOutputPixelOffset = 6;
			}
			break;
		default:
			err = cmInvalidDstMap;
			break;
	}

CleanupAndExit:
	LH_END_PROC("CheckOutputColorSpace")
	return err;
}


/*--------------------------------------------------------------------------------------------------------------
	SetOutputColorSpaceInplace
  --------------------------------------------------------------------------------------------------------------*/
CMError
SetOutputColorSpaceInplace	( CMCalcParamPtr	calcParam,
							  ColorSpaceInfo*	info,
							  OSType 			outColorSpace )
{
	CMError err = noErr;

	LH_START_PROC("SetOutputColorSpaceInplace")

	switch ( outColorSpace )
	{
		case icSigXYZData:
			calcParam->cmOutputColorSpace = cmXYZ32Space;
			break;
		case icSigLuvData:
			calcParam->cmOutputColorSpace = cmLUV32Space;
			break;		
		case icSigYxyData:
			calcParam->cmOutputColorSpace = cmYXY32Space;
			break;
		case icSigHsvData:
			calcParam->cmOutputColorSpace = cmHSV32Space;		
			break;
		case icSigHlsData:
			calcParam->cmOutputColorSpace = cmHLS32Space;
			break;
		case icSigGrayData:
			if (calcParam->cmInputPixelOffset == 1)
				calcParam->cmOutputColorSpace = cmGraySpace|cm8PerChannelPacking;
			else if (calcParam->cmInputPixelOffset == 2)
				calcParam->cmOutputColorSpace = cmGraySpace;
			else 
				calcParam->cmOutputColorSpace = cmGrayASpace;
			break;

		case icSigRgbData:
			/* based on the size of the input bitmap, select the right rgb packing format */	
#ifdef PI_Application_h
			if( (calcParam->cmInputColorSpace & 0x1f) == cmBGRSpace ){
				if (info->inputPixelSize < 24){
					if (info->origSizeIn == 6){
						calcParam->cmOutputColorSpace = cmBGRSpace|cmWord565ColorPacking;
					}
					else{																		/* output is 5 bit */
						calcParam->cmOutputColorSpace = cmBGRSpace|cmWord5ColorPacking;
					}
				}
				if (info->inputPixelSize < 32)
					calcParam->cmOutputColorSpace = cmBGR24Space;
				else if (info->inputPixelSize == 32)
					calcParam->cmOutputColorSpace = cmBGR32Space;
				else if (info->inputPixelSize == 48)
					calcParam->cmOutputColorSpace = cmBGRSpace | cm16PerChannelPacking;
				else if (info->inputPixelSize == 64)
					calcParam->cmOutputColorSpace = cmBGR32Space | cm16PerChannelPacking;
				break;
			}
#endif
			if (info->inputPixelSize < 24)
#ifdef PI_Application_h
				if (info->origSizeIn == 6){
					calcParam->cmOutputColorSpace = cmRGB16_565Space;
				}
				else																		/* output is 5 bit */
#endif
				calcParam->cmOutputColorSpace = cmRGB16Space;
			else if (info->inputPixelSize < 32)
				calcParam->cmOutputColorSpace = cmRGB24Space;
			else if (info->inputPixelSize == 32)
				calcParam->cmOutputColorSpace = cmRGB32Space;
			else if (info->inputPixelSize == 48)
				calcParam->cmOutputColorSpace = cmRGBSpace | cm16PerChannelPacking;
			else if (info->inputPixelSize == 64)
				calcParam->cmOutputColorSpace = cmRGB32Space | cm16PerChannelPacking;
			break;
			
		case icSigLabData:
			/* based on the size of the input bitmap, select the right lab packing format */	
			if (info->inputPixelSize < 32)
				calcParam->cmOutputColorSpace = cmLAB24Space;
			else
				calcParam->cmOutputColorSpace = cmLAB32Space;
			break;
			
		case icSigCmyData:
			calcParam->cmOutputColorSpace = cmCMY24Space;
			break;
		case icSigCmykData:
#ifdef PI_Application_h
			if( (calcParam->cmInputColorSpace & 0x1f) == cmKYMCSpace ){
				calcParam->cmOutputColorSpace = cmKYMC32Space;
				break;
			}
#endif
			calcParam->cmOutputColorSpace = cmCMYK32Space;
			break;
			
		case icSigMCH5Data:
			calcParam->cmOutputColorSpace = cmMCFive8Space;
			break;
		case icSigMCH6Data:
			calcParam->cmOutputColorSpace = cmMCSix8Space;
			break;
		case icSigMCH7Data:
			calcParam->cmOutputColorSpace = cmMCSeven8Space;
			break;
		case icSigMCH8Data:
			calcParam->cmOutputColorSpace = cmMCEight8Space;
			break;
		default:
			break;
	}
 
	LH_END_PROC("SetOutputColorSpaceInplace")
	return err;
}

/*--------------------------------------------------------------------------------------------------------------
	CheckOutputColorSpaceChk
  --------------------------------------------------------------------------------------------------------------*/
CMError
CheckOutputColorSpaceChk  ( const CMBitmap*	bitMap,
							CMCalcParamPtr	calcParam,
							ColorSpaceInfo*	info )
{
	CMError err = noErr;
	SINT32	newRowBytes;
	SINT32	bufferSize;
	SINT32	linesAtOnce;
	SINT16	iErr = noErr;

	LH_START_PROC("CheckOutputColorSpaceChk")
	switch ( bitMap->space)
	{
		case cmGamutResult1Space:
			newRowBytes = calcParam->cmPixelPerLine;		/* TempBuffer -> 1 byte per pixel */
			linesAtOnce = (kMaxTempBlock) / newRowBytes;
			if (linesAtOnce == 0)
				linesAtOnce = 1;
			else if (linesAtOnce > calcParam->cmLineCount)
				linesAtOnce = calcParam->cmLineCount;
				
			bufferSize = newRowBytes * linesAtOnce;
			info->processedLinesOut = 0;
			info->origSizeOut = 1;
			info->usedSizeOut = 8;				/* ACHTUNG !!!!! bufferSize*info->usedSizeOut */
			info->tempOutBuffer 	= (void*)SmartNewPtr(bufferSize*info->usedSizeOut, &iErr);
			if (iErr != noErr)
			{
				err = iErr;
				goto CleanupAndExit;
			}
	
			calcParam->cmLineCount			 = linesAtOnce;
			calcParam->cmOutputColorSpace    = cmGraySpace|cm8PerChannelPacking;
			calcParam->cmOutputBytesPerLine  = newRowBytes;
			calcParam->outputData[0]		 =  (Ptr)info->tempOutBuffer;
			calcParam->cmOutputPixelOffset	 = 1;
			break;
		case cmGamutResultSpace:
			newRowBytes = calcParam->cmPixelPerLine;		/* TempBuffer -> 1 byte per pixel */
			linesAtOnce = (kMaxTempBlock) / newRowBytes;
			if (linesAtOnce == 0)
				linesAtOnce = 1;
			else if (linesAtOnce > calcParam->cmLineCount)
				linesAtOnce = calcParam->cmLineCount;
				
			bufferSize = newRowBytes * linesAtOnce;
			info->processedLinesOut = 0;
			info->origSizeOut = 8;
			info->usedSizeOut = 8;
			info->tempOutBuffer 	= 0;
	
			calcParam->cmLineCount			 = linesAtOnce;
			calcParam->cmOutputColorSpace    = cmGraySpace|cm8PerChannelPacking;
			calcParam->cmOutputBytesPerLine  = newRowBytes;
			calcParam->cmOutputPixelOffset	 = 1;
			calcParam->outputData[0]		= &bitMap->image[0];
			break;
		default:
			err = cmInvalidDstMap;
			break;
	}
CleanupAndExit:
	LH_END_PROC("CheckOutputColorSpaceChk")
	return err;
}

/*--------------------------------------------------------------------------------------------------------------
	AllocBufferCheckCM
  --------------------------------------------------------------------------------------------------------------*/
CMError
AllocBufferCheckCM	( CMCalcParamPtr	calcParam,
					  ColorSpaceInfo*	info )
{
	CMError err = noErr;
	SINT32	newRowBytes;
	SINT32	bufferSize;
	SINT32	linesAtOnce;
	SINT16	iErr = noErr;
	
	LH_START_PROC("AllocBufferCheckCM")
	newRowBytes = calcParam->cmPixelPerLine;		/* TempBuffer -> 1 byte per pixel */
	linesAtOnce = (kMaxTempBlock) / newRowBytes;
	if (linesAtOnce == 0)
		linesAtOnce = 1;
	else if (linesAtOnce > calcParam->cmLineCount)
		linesAtOnce = calcParam->cmLineCount;
		
	bufferSize = newRowBytes * linesAtOnce;
	info->processedLinesOut = 0;
	info->origSizeOut = 1;
	info->usedSizeOut = 8;
	info->tempOutBuffer 	= (void*)SmartNewPtr(bufferSize, &iErr);
	if (iErr != noErr)
	{
		err = iErr;
		goto CleanupAndExit;
	}

	calcParam->cmLineCount			 = linesAtOnce;
	calcParam->cmOutputColorSpace    = cmGraySpace|cm8PerChannelPacking;
	calcParam->cmOutputBytesPerLine  = newRowBytes;
	calcParam->outputData[0]		 =  (Ptr)info->tempOutBuffer;
	calcParam->cmOutputPixelOffset	 = 1;

CleanupAndExit:
	LH_END_PROC("AllocBufferCheckCM")
	return err;
}

/*--------------------------------------------------------------------------------------------------------------
	FindLookupRoutine
  --------------------------------------------------------------------------------------------------------------*/
CalcProcPtr
FindLookupRoutine   ( const CMLutParam*		lutParam,
					  const ColorSpaceInfo*	info )
{
	CalcProcPtr		proc2call	= nil;
#ifdef DEBUG_OUTPUT
	CMError err = noErr;
#endif
	LH_START_PROC("FindLookupRoutine")

	switch ( lutParam->colorLutInDim )
	{
		case 3:																						/* 3 ->															*/
			switch ( lutParam->colorLutOutDim )
			{
				case 3:																				/* 3 -> 3														*/
					switch (info->usedSizeIn)
					{	
						case 8:																		/* 3 -> 3		Di 8                     						*/
							switch (info->usedSizeOut)
							{
								case 8:																/* 3 -> 3		Di 8		Do 8								*/
									switch (lutParam->colorLutWordSize)
									{
										case 8:														/* 3 -> 3		Di 8		Do 8		Lut 8					*/
											switch (lutParam->colorLutGridPoints)
											{
#if LH_CALC_ENGINE_ALL_FORMATS_LO
												case 16:											/* 3 -> 3		Di 8		Do 8		Lut 8		Grid 16		*/
													proc2call = NewCalcProc(LHCalc3to3_Di8_Do8_Lut8_G16_LO);
													break;
#endif
												case 32:											/* 3 -> 3		Di 8		Do 8		Lut 8		Grid 32		*/
													proc2call = NewCalcProc(LHCalc3to3_Di8_Do8_Lut8_G32_LO);
													break;
											}
											break;
#if LH_CALC_ENGINE_ALL_FORMATS_LO
										case 16:													/* 3 -> 3		Di 8		Do 8		Lut 16					*/			
											switch (lutParam->colorLutGridPoints)
											{
												case 16:											/* 3 -> 3		Di 8		Do 8		Lut 16		Grid 16		*/
													proc2call = NewCalcProc(LHCalc3to3_Di8_Do8_Lut16_G16_LO);
													break;
												case 32:											/* 3 -> 3		Di 8		Do 8		Lut 16		Grid 32		*/
													proc2call = NewCalcProc(LHCalc3to3_Di8_Do8_Lut16_G32_LO);
													break;
											}
											break;
#endif
									}
									break;
#if LH_CALC_ENGINE_ALL_FORMATS_LO
								case 16:															/* 3 -> 3		Di 8		Do 16								*/
#if LH_CALC_ENGINE_MIXED_DATAFORMAT
									switch (lutParam->colorLutWordSize)
									{
										case 8:														/* 3 -> 3		Di 8		Do 16		Lut 8					*/
											switch (lutParam->colorLutGridPoints)
											{
												case 16:											/* 3 -> 3		Di 8		Do 16		Lut 8		Grid 16		*/
													proc2call = NewCalcProc(LHCalc3to3_Di8_Do16_Lut8_G16_LO);
													break;
												case 32:											/* 3 -> 3		Di 8		Do 16		Lut 8		Grid 32		*/
													proc2call = NewCalcProc(LHCalc3to3_Di8_Do16_Lut8_G32_LO);
													break;
											}
											break;
										case 16:													/* 3 -> 3		Di 8		Do 16		Lut 16					*/
											switch (lutParam->colorLutGridPoints)
											{
												case 16:											/* 3 -> 3		Di 8		Do 16		Lut 16		Grid 16		*/
													proc2call = NewCalcProc(LHCalc3to3_Di8_Do16_Lut16_G16_LO);
													break;
												case 32:											/* 3 -> 3		Di 8		Do 16		Lut 16		Grid 32		*/
													proc2call = NewCalcProc(LHCalc3to3_Di8_Do16_Lut16_G32_LO);
													break;
											}
											break;
									}
#endif
									break;
#endif
							}

							break;
#if LH_CALC_ENGINE_ALL_FORMATS_LO
						case 16:																	/* 3 -> 3		Di 16                    						*/
							switch (info->usedSizeOut)
							{
								case 8:																/* 3 -> 3		Di 16		Do 8								*/
#if LH_CALC_ENGINE_MIXED_DATAFORMAT
									switch (lutParam->colorLutWordSize)
									{
										case 8:														/* 3 -> 3		Di 16		Do 8		Lut 8					*/
											switch (lutParam->colorLutGridPoints)
											{
												case 16:											/* 3 -> 3		Di 16		Do 8		Lut 8		Grid 16		*/
													proc2call = NewCalcProc(LHCalc3to3_Di16_Do8_Lut8_G16_LO);
													break;
												case 32:											/* 3 -> 3		Di 16		Do 8		Lut 8		Grid 32		*/
													proc2call = NewCalcProc(LHCalc3to3_Di16_Do8_Lut8_G32_LO);
													break;
											}
											break;
										case 16:													/* 3 -> 3		Di 16		Do 8		Lut 16					*/
											switch (lutParam->colorLutGridPoints)
											{
												case 16:											/* 3 -> 3		Di 16		Do 8		Lut 16		Grid 16		*/
													proc2call = NewCalcProc(LHCalc3to3_Di16_Do8_Lut16_G16_LO);
													break;
												case 32:											/* 3 -> 3		Di 16		Do 8		Lut 16		Grid 32		*/
													proc2call = NewCalcProc(LHCalc3to3_Di16_Do8_Lut16_G32_LO);
													break;
											}
											break;
									}
#endif
									break;
								case 16:															/* 3 -> 3		Di 16		Do 16								*/
#if LH_CALC_ENGINE_16_BIT_LO
									switch (lutParam->colorLutWordSize)
									{
										case 8:														/* 3 -> 3		Di 16		Do 16		Lut 8					*/
											switch (lutParam->colorLutGridPoints)
											{
												case 16:											/* 3 -> 3		Di 16		Do 16		Lut 8		Grid 16		*/
													proc2call = NewCalcProc(LHCalc3to3_Di16_Do16_Lut8_G16_LO);
													break;
												case 32:											/* 3 -> 3		Di 16		Do 16		Lut 8		Grid 32		*/
													proc2call = NewCalcProc(LHCalc3to3_Di16_Do16_Lut8_G32_LO);
													break;
											}
											break;
										case 16:													/* 3 -> 3		Di 16		Do 16		Lut 16					*/
											switch (lutParam->colorLutGridPoints)
											{
												case 16:											/* 3 -> 3		Di 16		Do 16		Lut 16		Grid 16		*/
													proc2call = NewCalcProc(LHCalc3to3_Di16_Do16_Lut16_G16_LO);
													break;
												case 32:											/* 3 -> 3		Di 16		Do 16		Lut 16		Grid 32		*/
													proc2call = NewCalcProc(LHCalc3to3_Di16_Do16_Lut16_G32_LO);
													break;
											}
											break;
									}
#endif
									break;
							}
							break;
#endif
					}
					break;
#if LH_CALC_ENGINE_ALL_FORMATS_LO
				case 4:																				/* 3 -> 4														*/
					switch (info->usedSizeIn)
					{
						case 8:																		/* 3 -> 4		Di 8											*/
							switch (info->usedSizeOut)
							{
								case 8:																/* 3 -> 4		Di 8		Do 8								*/
									switch (lutParam->colorLutWordSize)
									{
										case 8:														/* 3 -> 4		Di 8		Do 8		Lut 8					*/
											switch (lutParam->colorLutGridPoints)
											{
												case 16:											/* 3 -> 4		Di 8		Do 8		Lut 8		Grid 16		*/
													proc2call = NewCalcProc(LHCalc3to4_Di8_Do8_Lut8_G16_LO);
													break;
												case 32:											/* 3 -> 4		Di 8		Do 8		Lut 8		Grid 32		*/
													proc2call = NewCalcProc(LHCalc3to4_Di8_Do8_Lut8_G32_LO);
													break;
											}
											break;
										case 16:													/* 3 -> 4		Di 8		Do 8		Lut 16					*/
											switch (lutParam->colorLutGridPoints)
											{
												case 16:											/* 3 -> 4		Di 8		Do 8		Lut 16		Grid 16		*/
													proc2call = NewCalcProc(LHCalc3to4_Di8_Do8_Lut16_G16_LO);
													break;
												case 32:											/* 3 -> 4		Di 8		Do 8		Lut 16		Grid 32		*/
													proc2call = NewCalcProc(LHCalc3to4_Di8_Do8_Lut16_G32_LO);
													break;
											}
											break;
									}
									break;
								case 16:															/* 3 -> 4		Di 8		Do 16								*/
#if LH_CALC_ENGINE_MIXED_DATAFORMAT
									switch (lutParam->colorLutWordSize)
									{
										case 8:														/* 3 -> 4		Di 8		Do 16		Lut 8					*/
											switch (lutParam->colorLutGridPoints)
											{
												case 16:											/* 3 -> 4		Di 8		Do 16		Lut 8		Grid 16		*/
													proc2call = NewCalcProc(LHCalc3to4_Di8_Do16_Lut8_G16_LO);
													break;
												case 32:											/* 3 -> 4		Di 8		Do 16		Lut 8		Grid 32		*/
													proc2call = NewCalcProc(LHCalc3to4_Di8_Do16_Lut8_G32_LO);
													break;
											}
											break;
										case 16:													/* 3 -> 4		Di 8		Do 16		Lut 16					*/
											switch (lutParam->colorLutGridPoints)
											{
												case 16:											/* 3 -> 4		Di 8		Do 16		Lut 16		Grid 16		*/
													proc2call = NewCalcProc(LHCalc3to4_Di8_Do16_Lut16_G16_LO);
													break;
												case 32:											/* 3 -> 4		Di 8		Do 16		Lut 16		Grid 32		*/
													proc2call = NewCalcProc(LHCalc3to4_Di8_Do16_Lut16_G32_LO);
													break;
											}
											break;
									}
#endif
									break;
							}
							break;
						case 16:																	/* 3 -> 4		Di 16											*/
							switch (info->usedSizeOut)
							{
								case 8:																/* 3 -> 4		Di 16		Do 8								*/
#if LH_CALC_ENGINE_MIXED_DATAFORMAT
									switch (lutParam->colorLutWordSize)
									{
										case 8:														/* 3 -> 4		Di 16		Do 8		Lut 8					*/
											switch (lutParam->colorLutGridPoints)
											{
												case 16:											/* 3 -> 4		Di 16		Do 8		Lut 8		Grid 16		*/
													proc2call = NewCalcProc(LHCalc3to4_Di16_Do8_Lut8_G16_LO);
													break;
												case 32:											/* 3 -> 4		Di 16		Do 8		Lut 8		Grid 32		*/
													proc2call = NewCalcProc(LHCalc3to4_Di16_Do8_Lut8_G32_LO);
													break;
											}
											break;
										case 16:													/* 3 -> 4		Di 16		Do 8		Lut 16					*/
											switch (lutParam->colorLutGridPoints)
											{
												case 16:											/* 3 -> 4		Di 16		Do 8		Lut 16		Grid 16		*/
													proc2call = NewCalcProc(LHCalc3to4_Di16_Do8_Lut16_G16_LO);
													break;
												case 32:											/* 3 -> 4		Di 16		Do 8		Lut 16		Grid 32		*/
													proc2call = NewCalcProc(LHCalc3to4_Di16_Do8_Lut16_G32_LO);
													break;
											}
											break;
									}
#endif
									break;
								case 16:															/* 3 -> 4		Di 16		Do 16								*/
#if LH_CALC_ENGINE_16_BIT_LO
									switch (lutParam->colorLutWordSize)
									{
										case 8:														/* 3 -> 4		Di 16		Do 16		Lut 8					*/
											switch (lutParam->colorLutGridPoints)
											{
												case 16:											/* 3 -> 4		Di 16		Do 16		Lut 8		Grid 16		*/
													proc2call = NewCalcProc(LHCalc3to4_Di16_Do16_Lut8_G16_LO);
													break;
												case 32:											/* 3 -> 4		Di 16		Do 16		Lut 8		Grid 32		*/
													proc2call = NewCalcProc(LHCalc3to4_Di16_Do16_Lut8_G32_LO);
													break;
											}
											break;
										case 16:													/* 3 -> 4		Di 16		Do 16		Lut 16					*/
											switch (lutParam->colorLutGridPoints)
											{
												case 16:											/* 3 -> 4		Di 16		Do 16		Lut 16		Grid 16		*/
													proc2call = NewCalcProc(LHCalc3to4_Di16_Do16_Lut16_G16_LO);
													break;
												case 32:											/* 3 -> 4		Di 16		Do 16		Lut 16		Grid 32		*/
													proc2call = NewCalcProc(LHCalc3to4_Di16_Do16_Lut16_G32_LO);
													break;
											}
											break;
									}
#endif
									break;
							}
							break;
					}
					break;
#endif
			}
			break;
#if LH_CALC_ENGINE_ALL_FORMATS_LO
		case 4:																						/* 4 ->															*/
			switch ( lutParam->colorLutOutDim )
			{
				case 3:																				/* 4 ->	3														*/
					switch (info->usedSizeIn)
					{
						case 8:																		/* 4 -> 3		Di 8											*/
							switch (info->usedSizeOut)
							{
								case 8:																/* 4 -> 3		Di 8		Do 8								*/
									switch (lutParam->colorLutWordSize)
									{
										case 8:														/* 4 -> 3		Di 8		Do 8		Lut 8					*/
											switch (lutParam->colorLutGridPoints)
											{
												case 8:												/* 4 -> 3		Di 8		Do 8		Lut 8		Grid 8		*/
													proc2call = NewCalcProc(LHCalc4to3_Di8_Do8_Lut8_G8_LO);
													break;
												case 16:											/* 4 -> 3		Di 8		Do 8		Lut 8		Grid 16		*/
													proc2call = NewCalcProc(LHCalc4to3_Di8_Do8_Lut8_G16_LO);
													break;
											}
											break;
										case 16:													/* 4 -> 3		Di 8		Do 8		Lut 16					*/
											switch (lutParam->colorLutGridPoints)
											{
												case 8:												/* 4 -> 3		Di 8		Do 8		Lut 16		Grid 8		*/
													proc2call = NewCalcProc(LHCalc4to3_Di8_Do8_Lut16_G8_LO);
													break;
												case 16:											/* 4 -> 3		Di 8		Do 8		Lut 16		Grid 16		*/
													proc2call = NewCalcProc(LHCalc4to3_Di8_Do8_Lut16_G16_LO);
													break;
											}
											break;
									}
									break;
								case 16:															/* 4 -> 3		Di 8		Do 16								*/
#if LH_CALC_ENGINE_MIXED_DATAFORMAT
									switch (lutParam->colorLutWordSize)
									{
										case 8:														/* 4 -> 3		Di 8		Do 16		Lut 8					*/
											switch (lutParam->colorLutGridPoints)
											{
												case 8:												/* 4 -> 3		Di 8		Do 16		Lut 8		Grid 8		*/
													proc2call = NewCalcProc(LHCalc4to3_Di8_Do16_Lut8_G8_LO);
													break;
												case 16:											/* 4 -> 3		Di 8		Do 16		Lut 8		Grid 16		*/
													proc2call = NewCalcProc(LHCalc4to3_Di8_Do16_Lut8_G16_LO);
													break;
											}
											break;
										case 16:													/* 4 -> 3		Di 8		Do 16		Lut 16					*/
											switch (lutParam->colorLutGridPoints)
											{
												case 8:												/* 4 -> 3		Di 8		Do 16		Lut 16		Grid 8		*/
													proc2call = NewCalcProc(LHCalc4to3_Di8_Do16_Lut16_G8_LO);
													break;
												case 16:											/* 4 -> 3		Di 8		Do 16		Lut 16		Grid 16		*/
													proc2call = NewCalcProc(LHCalc4to3_Di8_Do16_Lut16_G16_LO);
													break;
											}
											break;
									}
#endif
									break;
							}
							break;
						case 16:																	/* 4 -> 3		Di 16											*/
							switch (info->usedSizeOut)
							{
								case 8:																/* 4 -> 3		Di 16		Do 8								*/
#if LH_CALC_ENGINE_MIXED_DATAFORMAT
									switch (lutParam->colorLutWordSize)
									{
										case 8:														/* 4 -> 3		Di 16		Do 8		Lut 8					*/
											switch (lutParam->colorLutGridPoints)
											{
												case 8:												/* 4 -> 3		Di 16		Do 8		Lut 8		Grid 8		*/
													proc2call = NewCalcProc(LHCalc4to3_Di16_Do8_Lut8_G8_LO);
													break;
												case 16:											/* 4 -> 3		Di 16		Do 8		Lut 8		Grid 16		*/
													proc2call = NewCalcProc(LHCalc4to3_Di16_Do8_Lut8_G16_LO);
													break;
											}
											break;
										case 16:													/* 4 -> 3		Di 16		Do 8		Lut 16					*/
											switch (lutParam->colorLutGridPoints)
											{
												case 8:												/* 4 -> 3		Di 16		Do 8		Lut 16		Grid 8		*/
													proc2call = NewCalcProc(LHCalc4to3_Di16_Do8_Lut16_G8_LO);
													break;
												case 16:											/* 4 -> 3		Di 16		Do 8		Lut 16		Grid 16		*/
													proc2call = NewCalcProc(LHCalc4to3_Di16_Do8_Lut16_G16_LO);
													break;
											}
											break;
									}
#endif
									break;
								case 16:															/* 4 -> 3		Di 16		Do 16								*/
#if LH_CALC_ENGINE_16_BIT_LO
									switch (lutParam->colorLutWordSize)
									{
										case 8:														/* 4 -> 3		Di 16		Do 16		Lut 8					*/
											switch (lutParam->colorLutGridPoints)
											{
												case 8:												/* 4 -> 3		Di 16		Do 16		Lut 8		Grid 8		*/
													proc2call = NewCalcProc(LHCalc4to3_Di16_Do16_Lut8_G8_LO);
													break;
												case 16:											/* 4 -> 3		Di 16		Do 16		Lut 8		Grid 16		*/
													proc2call = NewCalcProc(LHCalc4to3_Di16_Do16_Lut8_G16_LO);
													break;
											}
											break;
										case 16:													/* 4 -> 3		Di 16		Do 16		Lut 16					*/
											switch (lutParam->colorLutGridPoints)
											{
												case 8:												/* 4 -> 3		Di 16		Do 16		Lut 16		Grid 8		*/
													proc2call = NewCalcProc(LHCalc4to3_Di16_Do16_Lut16_G8_LO);
													break;
												case 16:											/* 4 -> 3		Di 16		Do 16		Lut 16		Grid 16		*/
													proc2call = NewCalcProc(LHCalc4to3_Di16_Do16_Lut16_G16_LO);
													break;
											}
											break;
									}
#endif
									break;
							}
							break;
					}
					break;
				case 4:																				/* 4 -> 4														*/
					switch (info->usedSizeIn)
					{
						case 8:																		/* 4 -> 4		Di 8											*/
							switch (info->usedSizeOut)
							{
								case 8:																/* 4 -> 4		Di 8		Do 8								*/
									switch (lutParam->colorLutWordSize)
									{
										case 8:														/* 4 -> 4		Di 8		Do 8		Lut 8					*/
											switch (lutParam->colorLutGridPoints)
											{
												case 8:												/* 4 -> 4		Di 8		Do 8		Lut 8		Grid 8		*/
													proc2call = NewCalcProc(LHCalc4to4_Di8_Do8_Lut8_G8_LO);
													break;
												case 16:											/* 4 -> 4		Di 8		Do 8		Lut 8		Grid 16		*/
													proc2call = NewCalcProc(LHCalc4to4_Di8_Do8_Lut8_G16_LO);
													break;
											}
											break;
										case 16:													/* 4 -> 4		Di 8		Do 8		Lut 16					*/
											switch (lutParam->colorLutGridPoints)
											{
												case 8:												/* 4 -> 4		Di 8		Do 8		Lut 16		Grid 8		*/
													proc2call = NewCalcProc(LHCalc4to4_Di8_Do8_Lut16_G8_LO);
													break;
												case 16:											/* 4 -> 4		Di 8		Do 8		Lut 16		Grid 16		*/
													proc2call = NewCalcProc(LHCalc4to4_Di8_Do8_Lut16_G16_LO);
													break;
											}
											break;
									}
									break;
								case 16:															/* 4 -> 4		Di 8		Do 16								*/
#if LH_CALC_ENGINE_MIXED_DATAFORMAT
									switch (lutParam->colorLutWordSize)
									{
										case 8:														/* 4 -> 4		Di 8		Do 16		Lut 8					*/
											switch (lutParam->colorLutGridPoints)
											{
												case 8:												/* 4 -> 4		Di 8		Do 16		Lut 8		Grid 8		*/
													proc2call = NewCalcProc(LHCalc4to4_Di8_Do16_Lut8_G8_LO);
													break;
												case 16:											/* 4 -> 4		Di 8		Do 16		Lut 8		Grid 16		*/
													proc2call = NewCalcProc(LHCalc4to4_Di8_Do16_Lut8_G16_LO);
													break;
											}
											break;
										case 16:													/* 4 -> 4		Di 8		Do 16		Lut 16					*/
											switch (lutParam->colorLutGridPoints)
											{
												case 8:												/* 4 -> 4		Di 8		Do 16		Lut 16		Grid 8		*/
													proc2call = NewCalcProc(LHCalc4to4_Di8_Do16_Lut16_G8_LO);
													break;
												case 16:											/* 4 -> 4		Di 8		Do 16		Lut 16		Grid 16		*/
													proc2call = NewCalcProc(LHCalc4to4_Di8_Do16_Lut16_G16_LO);
													break;
											}
											break;
									}
#endif
									break;
							}
							break;
						case 16:																	/* 4 -> 4		Di 16											*/
							switch (info->usedSizeOut)
							{
								case 8:																/* 4 -> 4		Di 16		Do 8								*/
#if LH_CALC_ENGINE_MIXED_DATAFORMAT
									switch (lutParam->colorLutWordSize)
									{
										case 8:														/* 4 -> 4		Di 16		Do 8		Lut 8					*/
											switch (lutParam->colorLutGridPoints)
											{
												case 8:												/* 4 -> 4		Di 16		Do 8		Lut 8		Grid 8		*/
													proc2call = NewCalcProc(LHCalc4to4_Di16_Do8_Lut8_G8_LO);
													break;
												case 16:											/* 4 -> 4		Di 16		Do 8		Lut 8		Grid 16		*/
													proc2call = NewCalcProc(LHCalc4to4_Di16_Do8_Lut8_G16_LO);
													break;
											}
											break;
										case 16:													/* 4 -> 4		Di 16		Do 8		Lut 16					*/
											switch (lutParam->colorLutGridPoints)
											{
												case 8:												/* 4 -> 4		Di 16		Do 8		Lut 16		Grid 8		*/
													proc2call = NewCalcProc(LHCalc4to4_Di16_Do8_Lut16_G8_LO);
													break;
												case 16:											/* 4 -> 4		Di 16		Do 8		Lut 16		Grid 16		*/
													proc2call = NewCalcProc(LHCalc4to4_Di16_Do8_Lut16_G16_LO);
													break;
											}
											break;
									}
#endif
									break;
								case 16:															/* 4 -> 4		Di 16		Do 16								*/
#if LH_CALC_ENGINE_16_BIT_LO
									switch (lutParam->colorLutWordSize)
									{
										case 8:														/* 4 -> 4		Di 16		Do 16		Lut 8					*/
											switch (lutParam->colorLutGridPoints)
											{
												case 8:												/* 4 -> 4		Di 16		Do 16		Lut 8		Grid 8		*/
													proc2call = NewCalcProc(LHCalc4to4_Di16_Do16_Lut8_G8_LO);
													break;
												case 16:											/* 4 -> 4		Di 16		Do 16		Lut 8		Grid 16		*/
													proc2call = NewCalcProc(LHCalc4to4_Di16_Do16_Lut8_G16_LO);
													break;
											}
											break;
										case 16:													/* 4 -> 4		Di 16		Do 16		Lut 16					*/
											switch (lutParam->colorLutGridPoints)
											{
												case 8:												/* 4 -> 4		Di 16		Do 16		Lut 16		Grid 8		*/
													proc2call = NewCalcProc(LHCalc4to4_Di16_Do16_Lut16_G8_LO);
													break;
												case 16:											/* 4 -> 4		Di 16		Do 16		Lut 16		Grid 16		*/
													proc2call = NewCalcProc(LHCalc4to4_Di16_Do16_Lut16_G16_LO);
													break;
											}
											break;
									}
#endif
									break;
							}
							break;
					}
					break;
			}
			break;
#endif
	}

	LH_END_PROC("FindLookupRoutine")
	return proc2call;
}

#ifdef ALLOW_MMX
#define cpuid   __asm _emit 0x0F __asm _emit 0xA2

#define CPUID_FLAG		0x00200000
#define MMX_FLAG		0x00800000
#define FAMILY_5_FLAG	0x00000500 /* Pentium, not Pentium II */
#define FAMILY_MASK		0x00000F00

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
*                                                                    *
*                                                                    *
*   DetectMMX                                                        *
*                                                                    *
*   Inputs   : none                                                  *
*                                                                    *
*   Outputs  : TRUE - CPU has MMX                                    *
*                                                                    *
*   Abstract : This function detects existance of MMX-Technology.    *
*                                                                    *
*                                                                    *
*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

static LH_UINT32 DetectMMX (void)
{
    LH_UINT32    fMMX = FALSE;

    __asm {

            push    edx             ; 
            push    ebx             ; 
			push    eax             ; 
            pushfd                  ; Push flags on stack
            pop     eax             ; and get flags into eax
            xor     eax, CPUID_FLAG ; Toggle CPUID flag
            push    eax             ; Move flags back into
            popfd                   ; flags reg
            pushfd                  ; Get flags into ebx
            pop     ebx
            and     ebx, CPUID_FLAG ; Mask CPUID flag
            and     eax, CPUID_FLAG
            cmp     eax, ebx        ; Test toggled bit
            jnz     Done            ; CPU does not support CPUID

            mov     eax, 1          ; Set CPUID mode
            cpuid
            test    edx, MMX_FLAG   ; Check MMX-Technology bit
            jz      Done            ; CPU has no MMX-Technology
			and		eax, FAMILY_MASK	; mask out family bits
            cmp     eax, FAMILY_5_FLAG	; Check Family 5 bits
            jnz     Done            ; CPU has no MMX-Technology

            mov     fMMX, TRUE      ; CPU has MMX-Technology
Done:
            ;pushfd                        ; dummy
			pop     eax             ;
            pop     ebx             ; 
            pop     edx             ; 
   }

    return (fMMX);
}

#endif
/*--------------------------------------------------------------------------------------------------------------
	FindCalcRoutine
  --------------------------------------------------------------------------------------------------------------*/
CalcProcPtr
FindCalcRoutine(	const CMCalcParam*		calcParam,
					const CMLutParam*		lutParam,
					const ColorSpaceInfo*	info,
					const Boolean			lookupOnly )
{
 	SINT32			index 		= 0;
	CalcProcPtr		proc2call	= nil;
#ifdef DEBUG_OUTPUT
	CMError err = noErr;
#endif
	
	LH_START_PROC("FindCalcRoutine")

	calcParam  = calcParam;
/* to reduce the code size change the next line to 0 - this will call DoNDim instead of the optimized functions */
#if 1
	if (lookupOnly)
		proc2call = FindLookupRoutine(lutParam, info);
	if (proc2call == nil)
	{
		switch ( lutParam->colorLutInDim )
		{
			case 1:																						/* 1 ->															*/
				switch ( lutParam->colorLutOutDim )
				{
					case 1:																				/* 1 -> 1,3,4														*/
					case 3:																				/* 1 -> 1,3,4													*/
					case 4:																				/* 1 -> 1,3,4													*/
						switch (info->usedSizeIn)
						{	
							case 8:																		/* 1 -> 1,3,4	Di 8                     						*/
								switch (info->usedSizeOut)
								{
									case 8:																/* 1 -> 1,3,4	Di 8		Do 8								*/
										switch (lutParam->colorLutWordSize)
										{
											case 8:														/* 1 -> 1,3,4	Di 8		Do 8		Lut 8					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc1toX_Di8_Do8_Lut8_G128 != LH_CALC_USE_DO_N_DIM
													case 128:											/* 1 -> 1,3,4	Di 8		Do 8		Lut 8		Grid 16		*/
														proc2call = NewCalcProc(LHCalc1toX_Di8_Do8_Lut8_G128);
														break;
#endif
												}
												break;
											case 16:													/* 1 -> 1,3,4	Di 8		Do 8		Lut 16					*/			
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc1toX_Di8_Do8_Lut16_G128 != LH_CALC_USE_DO_N_DIM
													case 128:											/* 1 -> 1,3,4	Di 8		Do 8		Lut 16		Grid 16		*/
														proc2call = NewCalcProc(LHCalc1toX_Di8_Do8_Lut16_G128);
														break;
#endif
												}
												break;
										}
										break;
									case 16:															/* 1 -> 1,3,4	Di 8		Do 16								*/
										switch (lutParam->colorLutWordSize)
										{
											case 8:														/* 1 -> 1,3,4	Di 8		Do 8		Lut 8					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc1toX_Di8_Do16_Lut8_G128 != LH_CALC_USE_DO_N_DIM
													case 128:											/* 1 -> 1,3,4	Di 8		Do 8		Lut 8		Grid 16		*/
														proc2call = NewCalcProc(LHCalc1toX_Di8_Do16_Lut8_G128);
														break;
#endif
												}
												break;
											case 16:													/* 1 -> 1,3,4	Di 8		Do 8		Lut 16					*/			
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc1toX_Di8_Do16_Lut16_G128 != LH_CALC_USE_DO_N_DIM
													case 128:											/* 1 -> 1,3,4	Di 8		Do 8		Lut 16		Grid 16		*/
														proc2call = NewCalcProc(LHCalc1toX_Di8_Do16_Lut16_G128);
														break;
#endif
												}
												break;
										}
										break;
								}
								break;
							case 16:																	/* 1 -> 1,3,4	Di 16                    						*/
								switch (info->usedSizeOut)
								{
									case 8:																/* 1 -> 1,3,4	Di 8		Do 8								*/
										switch (lutParam->colorLutWordSize)
										{
											case 8:														/* 1 -> 1,3,4	Di 8		Do 8		Lut 8					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc1toX_Di16_Do8_Lut8_G128 != LH_CALC_USE_DO_N_DIM
													case 128:											/* 1 -> 1,3,4	Di 8		Do 8		Lut 8		Grid 16		*/
														proc2call = NewCalcProc(LHCalc1toX_Di16_Do8_Lut8_G128);
														break;
#endif
												}
												break;
											case 16:													/* 1 -> 1,3,4	Di 8		Do 8		Lut 16					*/			
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc1toX_Di16_Do8_Lut16_G128 != LH_CALC_USE_DO_N_DIM
													case 128:											/* 1 -> 1,3,4	Di 8		Do 8		Lut 16		Grid 16		*/
														proc2call = NewCalcProc(LHCalc1toX_Di16_Do8_Lut16_G128);
														break;
#endif
												}
												break;
										}
										break;
									case 16:															/* 1 -> 1,3,4	Di 8		Do 16								*/
										switch (lutParam->colorLutWordSize)
										{
											case 8:														/* 1 -> 1,3,4	Di 8		Do 8		Lut 8					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc1toX_Di16_Do16_Lut8_G128 != LH_CALC_USE_DO_N_DIM
													case 128:											/* 1 -> 1,3,4	Di 8		Do 8		Lut 8		Grid 16		*/
														proc2call = NewCalcProc(LHCalc1toX_Di16_Do16_Lut8_G128);
														break;
#endif
												}
												break;
											case 16:													/* 1 -> 1,3,4	Di 8		Do 8		Lut 16					*/			
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc1toX_Di16_Do16_Lut16_G128 != LH_CALC_USE_DO_N_DIM
													case 128:											/* 1 -> 1,3,4	Di 8		Do 8		Lut 16		Grid 16		*/
														proc2call = NewCalcProc(LHCalc1toX_Di16_Do16_Lut16_G128);
														break;
#endif
												}
												break;
										}
										break;
								}
								break;
						}
						break;
					}
					break;
			case 3:																						/* 3 ->															*/
				switch ( lutParam->colorLutOutDim )
				{
					case 3:																				/* 3 -> 3														*/
						switch (info->usedSizeIn)
						{	
							case 8:																		/* 3 -> 3		Di 8                     						*/
								switch (info->usedSizeOut)
								{
									case 8:																/* 3 -> 3		Di 8		Do 8								*/
										switch (lutParam->colorLutWordSize)
										{
											case 8:														/* 3 -> 3		Di 8		Do 8		Lut 8					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc3to3_Di8_Do8_Lut8_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 3 -> 3		Di 8		Do 8		Lut 8		Grid 16		*/
#ifdef ALLOW_MMX
                                                        if (DetectMMX()) {
														    proc2call = NewCalcProc(LHCalc3to3_Di8_Do8_Lut8_G16_F);
                                                        }
                                                        else
#endif
#if LH_CALC_USE_ADDITIONAL_OLD_CODE
														if( !info->inPlace ){
														    proc2call = NewCalcProc(LHCalc3to3_Di8_Do8_Lut8_G16_Old);
														}
														else
#endif
    														proc2call = NewCalcProc(LHCalc3to3_Di8_Do8_Lut8_G16);
														break;
#endif
#if LH_Calc3to3_Di8_Do8_Lut8_G32 != LH_CALC_USE_DO_N_DIM
													case 32:											/* 3 -> 3		Di 8		Do 8		Lut 8		Grid 32		*/
#ifdef ALLOW_MMX
                                                        if (DetectMMX()) {
														    proc2call = NewCalcProc(LHCalc3to3_Di8_Do8_Lut8_G32_F);
                                                        }
                                                        else
#endif
#if LH_CALC_USE_ADDITIONAL_OLD_CODE
														if( !info->inPlace ){
														    proc2call = NewCalcProc(LHCalc3to3_Di8_Do8_Lut8_G32_Old);
														}
														else
#endif
    														proc2call = NewCalcProc(LHCalc3to3_Di8_Do8_Lut8_G32);
														break;
#endif
												}
												break;
											case 16:													/* 3 -> 3		Di 8		Do 8		Lut 16					*/			
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc3to3_Di8_Do8_Lut16_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 3 -> 3		Di 8		Do 8		Lut 16		Grid 16		*/
														proc2call = NewCalcProc(LHCalc3to3_Di8_Do8_Lut16_G16);
														break;
#endif
#if LH_Calc3to3_Di8_Do8_Lut16_G32 != LH_CALC_USE_DO_N_DIM
													case 32:											/* 3 -> 3		Di 8		Do 8		Lut 16		Grid 32		*/
														proc2call = NewCalcProc(LHCalc3to3_Di8_Do8_Lut16_G32);
														break;
#endif
												}
												break;
										}
										break;
									case 16:															/* 3 -> 3		Di 8		Do 16								*/
										switch (lutParam->colorLutWordSize)
										{
											case 8:														/* 3 -> 3		Di 8		Do 16		Lut 8					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc3to3_Di8_Do16_Lut8_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 3 -> 3		Di 8		Do 16		Lut 8		Grid 16		*/
														proc2call = NewCalcProc(LHCalc3to3_Di8_Do16_Lut8_G16);
														break;
#endif
#if LH_Calc3to3_Di8_Do16_Lut8_G32 != LH_CALC_USE_DO_N_DIM
													case 32:											/* 3 -> 3		Di 8		Do 16		Lut 8		Grid 32		*/
														proc2call = NewCalcProc(LHCalc3to3_Di8_Do16_Lut8_G32);
														break;
#endif
												}
												break;
											case 16:													/* 3 -> 3		Di 8		Do 16		Lut 16					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc3to3_Di8_Do16_Lut16_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 3 -> 3		Di 8		Do 16		Lut 16		Grid 16		*/
														proc2call = NewCalcProc(LHCalc3to3_Di8_Do16_Lut16_G16);
														break;
#endif
#if LH_Calc3to3_Di8_Do16_Lut16_G32 != LH_CALC_USE_DO_N_DIM
													case 32:											/* 3 -> 3		Di 8		Do 16		Lut 16		Grid 32		*/
														proc2call = NewCalcProc(LHCalc3to3_Di8_Do16_Lut16_G32);
														break;
#endif
												}
												break;
										}
										break;
								}
								break;
							case 16:																	/* 3 -> 3		Di 16                    						*/
								switch (info->usedSizeOut)
								{
									case 8:																/* 3 -> 3		Di 16		Do 8								*/
										switch (lutParam->colorLutWordSize)
										{
											case 8:														/* 3 -> 3		Di 16		Do 8		Lut 8					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc3to3_Di16_Do8_Lut8_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 3 -> 3		Di 16		Do 8		Lut 8		Grid 16		*/
														proc2call = NewCalcProc(LHCalc3to3_Di16_Do8_Lut8_G16);
														break;
#endif
#if LH_Calc3to3_Di16_Do8_Lut8_G32 != LH_CALC_USE_DO_N_DIM
													case 32:											/* 3 -> 3		Di 16		Do 8		Lut 8		Grid 32		*/
														proc2call = NewCalcProc(LHCalc3to3_Di16_Do8_Lut8_G32);
														break;
#endif
												}
												break;
											case 16:													/* 3 -> 3		Di 16		Do 8		Lut 16					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc3to3_Di16_Do8_Lut16_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 3 -> 3		Di 16		Do 8		Lut 16		Grid 16		*/
														proc2call = NewCalcProc(LHCalc3to3_Di16_Do8_Lut16_G16);
														break;
#endif
#if LH_Calc3to3_Di16_Do8_Lut16_G32 != LH_CALC_USE_DO_N_DIM
													case 32:											/* 3 -> 3		Di 16		Do 8		Lut 16		Grid 32		*/
														proc2call = NewCalcProc(LHCalc3to3_Di16_Do8_Lut16_G32);
														break;
#endif
												}
												break;
										}
										break;
									case 16:															/* 3 -> 3		Di 16		Do 16								*/
										switch (lutParam->colorLutWordSize)
										{
											case 8:														/* 3 -> 3		Di 16		Do 16		Lut 8					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc3to3_Di16_Do16_Lut8_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 3 -> 3		Di 16		Do 16		Lut 8		Grid 16		*/
														proc2call = NewCalcProc(LHCalc3to3_Di16_Do16_Lut8_G16);
														break;
#endif
#if LH_Calc3to3_Di16_Do16_Lut8_G32 != LH_CALC_USE_DO_N_DIM
													case 32:											/* 3 -> 3		Di 16		Do 16		Lut 8		Grid 32		*/
														proc2call = NewCalcProc(LHCalc3to3_Di16_Do16_Lut8_G32);
														break;
#endif
												}
												break;
											case 16:													/* 3 -> 3		Di 16		Do 16		Lut 16					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc3to3_Di16_Do16_Lut16_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 3 -> 3		Di 16		Do 16		Lut 16		Grid 16		*/
														proc2call = NewCalcProc(LHCalc3to3_Di16_Do16_Lut16_G16);
														break;
#endif
#if LH_Calc3to3_Di16_Do16_Lut16_G32 != LH_CALC_USE_DO_N_DIM
													case 32:											/* 3 -> 3		Di 16		Do 16		Lut 16		Grid 32		*/
														proc2call = NewCalcProc(LHCalc3to3_Di16_Do16_Lut16_G32);
														break;
#endif
												}
												break;
										}
										break;
								}
								break;
						}
						break;
					case 4:																				/* 3 -> 4														*/
						switch (info->usedSizeIn)
						{
							case 8:																		/* 3 -> 4		Di 8											*/
								switch (info->usedSizeOut)
								{
									case 8:																/* 3 -> 4		Di 8		Do 8								*/
										switch (lutParam->colorLutWordSize)
										{
											case 8:														/* 3 -> 4		Di 8		Do 8		Lut 8					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc3to4_Di8_Do8_Lut8_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 3 -> 4		Di 8		Do 8		Lut 8		Grid 16		*/
#ifdef ALLOW_MMX
                                                        if (DetectMMX()) {
														    proc2call = NewCalcProc(LHCalc3to4_Di8_Do8_Lut8_G16_F);
                                                        }
                                                        else
#endif
#if LH_CALC_USE_ADDITIONAL_OLD_CODE
														if( !info->inPlace ){
														    proc2call = NewCalcProc(LHCalc3to4_Di8_Do8_Lut8_G16_Old);
														}
														else
#endif
															proc2call = NewCalcProc(LHCalc3to4_Di8_Do8_Lut8_G16);
														break;
#endif
#if LH_Calc3to4_Di8_Do8_Lut8_G32 != LH_CALC_USE_DO_N_DIM
													case 32:											/* 3 -> 4		Di 8		Do 8		Lut 8		Grid 32		*/
#ifdef ALLOW_MMX
                                                        if (DetectMMX()) {
														    proc2call = NewCalcProc(LHCalc3to4_Di8_Do8_Lut8_G32_F);
                                                        }
                                                        else
#endif
#if LH_CALC_USE_ADDITIONAL_OLD_CODE
														if( !info->inPlace ){
														    proc2call = NewCalcProc(LHCalc3to4_Di8_Do8_Lut8_G32_Old);
														}
														else
#endif
															proc2call = NewCalcProc(LHCalc3to4_Di8_Do8_Lut8_G32);
														break;
#endif
												}
												break;
											case 16:													/* 3 -> 4		Di 8		Do 8		Lut 16					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc3to4_Di8_Do8_Lut16_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 3 -> 4		Di 8		Do 8		Lut 16		Grid 16		*/
														proc2call = NewCalcProc(LHCalc3to4_Di8_Do8_Lut16_G16);
														break;
#endif
#if LH_Calc3to4_Di8_Do8_Lut16_G32 != LH_CALC_USE_DO_N_DIM
													case 32:											/* 3 -> 4		Di 8		Do 8		Lut 16		Grid 32		*/
														proc2call = NewCalcProc(LHCalc3to4_Di8_Do8_Lut16_G32);
														break;
#endif
												}
												break;
										}
										break;
									case 16:															/* 3 -> 4		Di 8		Do 16								*/
										switch (lutParam->colorLutWordSize)
										{
											case 8:														/* 3 -> 4		Di 8		Do 16		Lut 8					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc3to4_Di8_Do16_Lut8_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 3 -> 4		Di 8		Do 16		Lut 8		Grid 16		*/
														proc2call = NewCalcProc(LHCalc3to4_Di8_Do16_Lut8_G16);
														break;
#endif
#if LH_Calc3to4_Di8_Do16_Lut8_G32 != LH_CALC_USE_DO_N_DIM
													case 32:											/* 3 -> 4		Di 8		Do 16		Lut 8		Grid 32		*/
														proc2call = NewCalcProc(LHCalc3to4_Di8_Do16_Lut8_G32);
														break;
#endif
												}
												break;
											case 16:													/* 3 -> 4		Di 8		Do 16		Lut 16					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc3to4_Di8_Do16_Lut16_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 3 -> 4		Di 8		Do 16		Lut 16		Grid 16		*/
														proc2call = NewCalcProc(LHCalc3to4_Di8_Do16_Lut16_G16);
														break;
#endif
#if LH_Calc3to4_Di8_Do16_Lut16_G32 != LH_CALC_USE_DO_N_DIM
													case 32:											/* 3 -> 4		Di 8		Do 16		Lut 16		Grid 32		*/
														proc2call = NewCalcProc(LHCalc3to4_Di8_Do16_Lut16_G32);
														break;
#endif
												}
												break;
										}
										break;
								}
								break;
							case 16:																	/* 3 -> 4		Di 16											*/
								switch (info->usedSizeOut)
								{
									case 8:																/* 3 -> 4		Di 16		Do 8								*/
										switch (lutParam->colorLutWordSize)
										{
											case 8:														/* 3 -> 4		Di 16		Do 8		Lut 8					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc3to4_Di16_Do8_Lut8_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 3 -> 4		Di 16		Do 8		Lut 8		Grid 16		*/
														proc2call = NewCalcProc(LHCalc3to4_Di16_Do8_Lut8_G16);
														break;
#endif
#if LH_Calc3to4_Di16_Do8_Lut8_G32 != LH_CALC_USE_DO_N_DIM
													case 32:											/* 3 -> 4		Di 16		Do 8		Lut 8		Grid 32		*/
														proc2call = NewCalcProc(LHCalc3to4_Di16_Do8_Lut8_G32);
														break;
#endif
												}
												break;
											case 16:													/* 3 -> 4		Di 16		Do 8		Lut 16					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc3to4_Di16_Do8_Lut16_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 3 -> 4		Di 16		Do 8		Lut 16		Grid 16		*/
														proc2call = NewCalcProc(LHCalc3to4_Di16_Do8_Lut16_G16);
														break;
#endif
#if LH_Calc3to4_Di16_Do8_Lut16_G32 != LH_CALC_USE_DO_N_DIM
													case 32:											/* 3 -> 4		Di 16		Do 8		Lut 16		Grid 32		*/
														proc2call = NewCalcProc(LHCalc3to4_Di16_Do8_Lut16_G32);
														break;
#endif
												}
												break;
										}
										break;
									case 16:															/* 3 -> 4		Di 16		Do 16								*/
										switch (lutParam->colorLutWordSize)
										{
											case 8:														/* 3 -> 4		Di 16		Do 16		Lut 8					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc3to4_Di16_Do16_Lut8_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 3 -> 4		Di 16		Do 16		Lut 8		Grid 16		*/
														proc2call = NewCalcProc(LHCalc3to4_Di16_Do16_Lut8_G16);
														break;
#endif
#if LH_Calc3to4_Di16_Do16_Lut8_G32 != LH_CALC_USE_DO_N_DIM
													case 32:											/* 3 -> 4		Di 16		Do 16		Lut 8		Grid 32		*/
														proc2call = NewCalcProc(LHCalc3to4_Di16_Do16_Lut8_G32);
														break;
#endif
												}
												break;
											case 16:													/* 3 -> 4		Di 16		Do 16		Lut 16					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc3to4_Di16_Do16_Lut16_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 3 -> 4		Di 16		Do 16		Lut 16		Grid 16		*/
														proc2call = NewCalcProc(LHCalc3to4_Di16_Do16_Lut16_G16);
														break;
#endif
#if LH_Calc3to4_Di16_Do16_Lut16_G32 != LH_CALC_USE_DO_N_DIM
													case 32:											/* 3 -> 4		Di 16		Do 16		Lut 16		Grid 32		*/
														proc2call = NewCalcProc(LHCalc3to4_Di16_Do16_Lut16_G32);
														break;
#endif
												}
												break;
										}
										break;
								}
								break;
						}
						break;
				}
				break;
			case 4:																						/* 4 ->															*/
				switch ( lutParam->colorLutOutDim )
				{
					case 3:																				/* 4 ->	3														*/
						switch (info->usedSizeIn)
						{
							case 8:																		/* 4 -> 3		Di 8											*/
								switch (info->usedSizeOut)
								{
									case 8:																/* 4 -> 3		Di 8		Do 8								*/
										switch (lutParam->colorLutWordSize)
										{
											case 8:														/* 4 -> 3		Di 8		Do 8		Lut 8					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc4to3_Di8_Do8_Lut8_G8 != LH_CALC_USE_DO_N_DIM
													case 8:												/* 4 -> 3		Di 8		Do 8		Lut 8		Grid 8		*/
#ifdef ALLOW_MMX
                                                        if (DetectMMX()) {
														    proc2call = NewCalcProc(LHCalc4to3_Di8_Do8_Lut8_G8_F);
                                                        }
                                                        else
#endif
#if LH_CALC_USE_ADDITIONAL_OLD_CODE_4DIM
														if( !info->inPlace ){
														    proc2call = NewCalcProc(LHCalc4to3_Di8_Do8_Lut8_G8_Old);
														}
														else
#endif
															proc2call = NewCalcProc(LHCalc4to3_Di8_Do8_Lut8_G8);
														break;
#endif
#if LH_Calc4to3_Di8_Do8_Lut8_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 4 -> 3		Di 8		Do 8		Lut 8		Grid 16		*/
#ifdef ALLOW_MMX
                                                        if (DetectMMX()) {
														    proc2call = NewCalcProc(LHCalc4to3_Di8_Do8_Lut8_G16_F);
                                                        }
                                                        else
#endif
#if LH_CALC_USE_ADDITIONAL_OLD_CODE_4DIM
														if( !info->inPlace ){
														    proc2call = NewCalcProc(LHCalc4to3_Di8_Do8_Lut8_G16_Old);
														}
														else
#endif
															proc2call = NewCalcProc(LHCalc4to3_Di8_Do8_Lut8_G16);
														break;
#endif
												}
												break;
											case 16:													/* 4 -> 3		Di 8		Do 8		Lut 16					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc4to3_Di8_Do8_Lut16_G8 != LH_CALC_USE_DO_N_DIM
													case 8:												/* 4 -> 3		Di 8		Do 8		Lut 16		Grid 8		*/
														proc2call = NewCalcProc(LHCalc4to3_Di8_Do8_Lut16_G8);
														break;
#endif
#if LH_Calc4to3_Di8_Do8_Lut16_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 4 -> 3		Di 8		Do 8		Lut 16		Grid 16		*/
														proc2call = NewCalcProc(LHCalc4to3_Di8_Do8_Lut16_G16);
														break;
#endif
												}
												break;
										}
										break;
									case 16:															/* 4 -> 3		Di 8		Do 16								*/
										switch (lutParam->colorLutWordSize)
										{
											case 8:														/* 4 -> 3		Di 8		Do 16		Lut 8					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc4to3_Di8_Do16_Lut8_G8 != LH_CALC_USE_DO_N_DIM
													case 8:												/* 4 -> 3		Di 8		Do 16		Lut 8		Grid 8		*/
														proc2call = NewCalcProc(LHCalc4to3_Di8_Do16_Lut8_G8);
														break;
#endif
#if LH_Calc4to3_Di8_Do16_Lut8_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 4 -> 3		Di 8		Do 16		Lut 8		Grid 16		*/
														proc2call = NewCalcProc(LHCalc4to3_Di8_Do16_Lut8_G16);
														break;
#endif
												}
												break;
											case 16:													/* 4 -> 3		Di 8		Do 16		Lut 16					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc4to3_Di8_Do16_Lut16_G8 != LH_CALC_USE_DO_N_DIM
													case 8:												/* 4 -> 3		Di 8		Do 16		Lut 16		Grid 8		*/
														proc2call = NewCalcProc(LHCalc4to3_Di8_Do16_Lut16_G8);
														break;
#endif
#if LH_Calc4to3_Di8_Do16_Lut16_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 4 -> 3		Di 8		Do 16		Lut 16		Grid 16		*/
														proc2call = NewCalcProc(LHCalc4to3_Di8_Do16_Lut16_G16);
														break;
#endif
												}
												break;
										}
										break;
								}
								break;
							case 16:																	/* 4 -> 3		Di 16											*/
								switch (info->usedSizeOut)
								{
									case 8:																/* 4 -> 3		Di 16		Do 8								*/
										switch (lutParam->colorLutWordSize)
										{
											case 8:														/* 4 -> 3		Di 16		Do 8		Lut 8					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc4to3_Di16_Do8_Lut8_G8 != LH_CALC_USE_DO_N_DIM
													case 8:												/* 4 -> 3		Di 16		Do 8		Lut 8		Grid 8		*/
														proc2call = NewCalcProc(LHCalc4to3_Di16_Do8_Lut8_G8);
														break;
#endif
#if LH_Calc4to3_Di16_Do8_Lut8_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 4 -> 3		Di 16		Do 8		Lut 8		Grid 16		*/
														proc2call = NewCalcProc(LHCalc4to3_Di16_Do8_Lut8_G16);
														break;
#endif
												}
												break;
											case 16:													/* 4 -> 3		Di 16		Do 8		Lut 16					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc4to3_Di16_Do8_Lut16_G8 != LH_CALC_USE_DO_N_DIM
													case 8:												/* 4 -> 3		Di 16		Do 8		Lut 16		Grid 8		*/
														proc2call = NewCalcProc(LHCalc4to3_Di16_Do8_Lut16_G8);
														break;
#endif
#if LH_Calc4to3_Di16_Do8_Lut16_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 4 -> 3		Di 16		Do 8		Lut 16		Grid 16		*/
														proc2call = NewCalcProc(LHCalc4to3_Di16_Do8_Lut16_G16);
														break;
#endif
												}
												break;
										}
										break;
									case 16:															/* 4 -> 3		Di 16		Do 16								*/
										switch (lutParam->colorLutWordSize)
										{
											case 8:														/* 4 -> 3		Di 16		Do 16		Lut 8					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc4to3_Di16_Do16_Lut8_G8 != LH_CALC_USE_DO_N_DIM
													case 8:												/* 4 -> 3		Di 16		Do 16		Lut 8		Grid 8		*/
														proc2call = NewCalcProc(LHCalc4to3_Di16_Do16_Lut8_G8);
														break;
#endif
#if LH_Calc4to3_Di16_Do16_Lut8_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 4 -> 3		Di 16		Do 16		Lut 8		Grid 16		*/
														proc2call = NewCalcProc(LHCalc4to3_Di16_Do16_Lut8_G16);
														break;
#endif
												}
												break;
											case 16:													/* 4 -> 3		Di 16		Do 16		Lut 16					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc4to3_Di16_Do16_Lut16_G8 != LH_CALC_USE_DO_N_DIM
													case 8:												/* 4 -> 3		Di 16		Do 16		Lut 16		Grid 8		*/
														proc2call = NewCalcProc(LHCalc4to3_Di16_Do16_Lut16_G8);
														break;
#endif
#if LH_Calc4to3_Di16_Do16_Lut16_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 4 -> 3		Di 16		Do 16		Lut 16		Grid 16		*/
														proc2call = NewCalcProc(LHCalc4to3_Di16_Do16_Lut16_G16);
														break;
#endif
												}
												break;
										}
										break;
								}
								break;
						}
						break;
					case 4:																				/* 4 -> 4														*/
						switch (info->usedSizeIn)
						{
							case 8:																		/* 4 -> 4		Di 8											*/
								switch (info->usedSizeOut)
								{
									case 8:																/* 4 -> 4		Di 8		Do 8								*/
										switch (lutParam->colorLutWordSize)
										{
											case 8:														/* 4 -> 4		Di 8		Do 8		Lut 8					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc4to4_Di8_Do8_Lut8_G8 != LH_CALC_USE_DO_N_DIM
													case 8:												/* 4 -> 4		Di 8		Do 8		Lut 8		Grid 8		*/
#ifdef ALLOW_MMX
                                                        if (DetectMMX()) {
														    proc2call = NewCalcProc(LHCalc4to4_Di8_Do8_Lut8_G8_F);
                                                        }
                                                        else
#endif
#if LH_CALC_USE_ADDITIONAL_OLD_CODE_4DIM
														if( !info->inPlace ){
														    proc2call = NewCalcProc(LHCalc4to4_Di8_Do8_Lut8_G8_Old);
														}
														else
#endif
															proc2call = NewCalcProc(LHCalc4to4_Di8_Do8_Lut8_G8);
														break;
#endif
#if LH_Calc4to4_Di8_Do8_Lut8_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 4 -> 4		Di 8		Do 8		Lut 8		Grid 16		*/
#ifdef ALLOW_MMX
                                                        if (DetectMMX()) {
														    proc2call = NewCalcProc(LHCalc4to4_Di8_Do8_Lut8_G16_F);
                                                        }
                                                        else
#endif
#if LH_CALC_USE_ADDITIONAL_OLD_CODE_4DIM
														if( !info->inPlace ){
														    proc2call = NewCalcProc(LHCalc4to4_Di8_Do8_Lut8_G16_Old);
														}
														else
#endif
															proc2call = NewCalcProc(LHCalc4to4_Di8_Do8_Lut8_G16);
														break;
#endif
												}
												break;
											case 16:													/* 4 -> 4		Di 8		Do 8		Lut 16					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc4to4_Di8_Do8_Lut16_G8 != LH_CALC_USE_DO_N_DIM
													case 8:												/* 4 -> 4		Di 8		Do 8		Lut 16		Grid 8		*/
														proc2call = NewCalcProc(LHCalc4to4_Di8_Do8_Lut16_G8);
														break;
#endif
#if LH_Calc4to4_Di8_Do8_Lut16_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 4 -> 4		Di 8		Do 8		Lut 16		Grid 16		*/
														proc2call = NewCalcProc(LHCalc4to4_Di8_Do8_Lut16_G16);
														break;
#endif
												}
												break;
										}
										break;
									case 16:															/* 4 -> 4		Di 8		Do 16								*/
										switch (lutParam->colorLutWordSize)
										{
											case 8:														/* 4 -> 4		Di 8		Do 16		Lut 8					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc4to4_Di8_Do16_Lut8_G8 != LH_CALC_USE_DO_N_DIM
													case 8:												/* 4 -> 4		Di 8		Do 16		Lut 8		Grid 8		*/
														proc2call = NewCalcProc(LHCalc4to4_Di8_Do16_Lut8_G8);
														break;
#endif
#if LH_Calc4to4_Di8_Do16_Lut8_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 4 -> 4		Di 8		Do 16		Lut 8		Grid 16		*/
														proc2call = NewCalcProc(LHCalc4to4_Di8_Do16_Lut8_G16);
														break;
#endif
												}
												break;
											case 16:													/* 4 -> 4		Di 8		Do 16		Lut 16					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc4to4_Di8_Do16_Lut16_G8 != LH_CALC_USE_DO_N_DIM
													case 8:												/* 4 -> 4		Di 8		Do 16		Lut 16		Grid 8		*/
														proc2call = NewCalcProc(LHCalc4to4_Di8_Do16_Lut16_G8);
														break;
#endif
#if LH_Calc4to4_Di8_Do16_Lut16_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 4 -> 4		Di 8		Do 16		Lut 16		Grid 16		*/
														proc2call = NewCalcProc(LHCalc4to4_Di8_Do16_Lut16_G16);
														break;
#endif
												}
												break;
										}
										break;
								}
								break;
							case 16:																	/* 4 -> 4		Di 16											*/
								switch (info->usedSizeOut)
								{
									case 8:																/* 4 -> 4		Di 16		Do 8								*/
										switch (lutParam->colorLutWordSize)
										{
											case 8:														/* 4 -> 4		Di 16		Do 8		Lut 8					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc4to4_Di16_Do8_Lut8_G8 != LH_CALC_USE_DO_N_DIM
													case 8:												/* 4 -> 4		Di 16		Do 8		Lut 8		Grid 8		*/
														proc2call = NewCalcProc(LHCalc4to4_Di16_Do8_Lut8_G8);
														break;
#endif
#if LH_Calc4to4_Di16_Do8_Lut8_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 4 -> 4		Di 16		Do 8		Lut 8		Grid 16		*/
														proc2call = NewCalcProc(LHCalc4to4_Di16_Do8_Lut8_G16);
														break;
#endif
												}
												break;
											case 16:													/* 4 -> 4		Di 16		Do 8		Lut 16					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc4to4_Di16_Do8_Lut16_G8 != LH_CALC_USE_DO_N_DIM
													case 8:												/* 4 -> 4		Di 16		Do 8		Lut 16		Grid 8		*/
														proc2call = NewCalcProc(LHCalc4to4_Di16_Do8_Lut16_G8);
														break;
#endif
#if LH_Calc4to4_Di16_Do8_Lut16_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 4 -> 4		Di 16		Do 8		Lut 16		Grid 16		*/
														proc2call = NewCalcProc(LHCalc4to4_Di16_Do8_Lut16_G16);
														break;
#endif
												}
												break;
										}
										break;
									case 16:															/* 4 -> 4		Di 16		Do 16								*/
										switch (lutParam->colorLutWordSize)
										{
											case 8:														/* 4 -> 4		Di 16		Do 16		Lut 8					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc4to4_Di16_Do16_Lut8_G8 != LH_CALC_USE_DO_N_DIM
													case 8:												/* 4 -> 4		Di 16		Do 16		Lut 8		Grid 8		*/
														proc2call = NewCalcProc(LHCalc4to4_Di16_Do16_Lut8_G8);
														break;
#endif
#if LH_Calc4to4_Di16_Do16_Lut8_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 4 -> 4		Di 16		Do 16		Lut 8		Grid 16		*/
														proc2call = NewCalcProc(LHCalc4to4_Di16_Do16_Lut8_G16);
														break;
#endif
												}
												break;
											case 16:													/* 4 -> 4		Di 16		Do 16		Lut 16					*/
												switch (lutParam->colorLutGridPoints)
												{
#if LH_Calc4to4_Di16_Do16_Lut16_G8 != LH_CALC_USE_DO_N_DIM
													case 8:												/* 4 -> 4		Di 16		Do 16		Lut 16		Grid 8		*/
														proc2call = NewCalcProc(LHCalc4to4_Di16_Do16_Lut16_G8);
														break;
#endif
#if LH_Calc4to4_Di16_Do16_Lut16_G16 != LH_CALC_USE_DO_N_DIM
													case 16:											/* 4 -> 4		Di 16		Do 16		Lut 16		Grid 16		*/
														proc2call = NewCalcProc(LHCalc4to4_Di16_Do16_Lut16_G16);
														break;
#endif
												}
												break;
										}
										break;
								}
								break;
						}
						break;
				}
				break;
		}
  }
#endif
  
	if ( ( proc2call == nil )
#ifdef DEBUG_OUTPUT
		 || ( gUSE_NDIM_FOR_BITMAP )
#endif
		)						
	{
		switch (info->usedSizeIn)
		{
			case 8:																		/*	Di 8							*/
				switch (info->usedSizeOut)
				{
					case 8:																/*	Di 8		Do 8				*/
						switch (lutParam->colorLutWordSize)
						{
							case 8:														/*	Di 8		Do 8		Lut 8	*/
								proc2call = NewCalcProc(CalcNDim_Data8To8_Lut8);
								break;
							case 16:													/*	Di 8		Do 8		Lut 16	*/
								proc2call = NewCalcProc(CalcNDim_Data8To8_Lut16);
								break;
						}
						break;
					case 16:															/*	Di 8		Do 16				*/
						switch (lutParam->colorLutWordSize)
						{
							case 8:														/*	Di 8		Do 16		Lut 8	*/
								proc2call = NewCalcProc(CalcNDim_Data8To16_Lut8);
								break;
							case 16:													/*	Di 8		Do 16		Lut 16	*/
								proc2call = NewCalcProc(CalcNDim_Data8To16_Lut16);
								break;
						}
						break;
				}
				break;
			case 16:																	/*	Di 16							*/
				switch (info->usedSizeOut)
				{
					case 8:																/*	Di 16		Do 8				*/
						switch (lutParam->colorLutWordSize)
						{
							case 8:														/*	Di 16		Do 8		Lut 8	*/
								proc2call = NewCalcProc(CalcNDim_Data16To8_Lut8);
								break;
							case 16:													/*	Di 16		Do 8		Lut 16	*/
								proc2call = NewCalcProc(CalcNDim_Data16To8_Lut16);
								break;
						}
						break;
					case 16:															/*	Di 16		Do 16				*/
						switch (lutParam->colorLutWordSize)
						{
							case 8:														/*	Di 16		Do 16		Lut 8	*/
								proc2call = NewCalcProc(CalcNDim_Data16To16_Lut8);
								break;
							case 16:													/*	Di 16		Do 16		Lut 16	*/
								proc2call = NewCalcProc(CalcNDim_Data16To16_Lut16);
								break;
						}
						break;
				}
				break;
		}
	}

#ifdef DEBUG_OUTPUT
	if ( DebugCheck(kThisFile, kDebugMiscInfo) )
	{
		if ( gUSE_NDIM_FOR_BITMAP )
			DebugPrint("DoNDIM:     %1d->%1d   Di %2d   Do %2d   Grid %2d  Lut %2d\n", 	lutParam->colorLutInDim, lutParam->colorLutOutDim,
																						info->usedSizeIn, info->usedSizeOut,
																						lutParam->colorLutGridPoints, lutParam->colorLutWordSize);
		else
			DebugPrint("OPTIMIZED:  %1d->%1d   Di %2d   Do %2d   Grid %2d  Lut %2d\n", 	lutParam->colorLutInDim, lutParam->colorLutOutDim,
																						info->usedSizeIn, info->usedSizeOut,
																						lutParam->colorLutGridPoints, lutParam->colorLutWordSize);
	}
	if (proc2call == nil)
		DebugPrint("¥ ERROR: FindCalcRoutine is nil !!!\n");
#endif
	LH_END_PROC("FindCalcRoutine")
	return proc2call;
}

#ifdef __MWERKS__
#pragma mark ================  Match/Check CMColors  ================
#endif

/*--------------------------------------------------------------------------------------------------------------
	LHMatchColorsPrivate
  --------------------------------------------------------------------------------------------------------------*/
CMError
LHMatchColorsPrivate	 (CMMModelPtr		modelingData, 
						  CMColor*			myColors, 
						  SINT32			count)
{
	CMCalcParam 	calcParam;
	CMLutParam		lutParam;
	CMError			err = -1;
	ColorSpaceInfo	info;
	CalcProcPtr		calcRoutine   = nil;

	LH_START_PROC("LHMatchColorsPrivate")
	
	LOCK_DATA((modelingData)->lutParam.inputLut);
	LOCK_DATA((modelingData)->lutParam.colorLut);
	LOCK_DATA((modelingData)->lutParam.outputLut);

 	/* preprocess for NamedColor stuff */
	if (modelingData->hasNamedColorProf == NamedColorProfileOnly){
		err = ConvertNamedIndexToColors(modelingData,myColors,count); 
		goto CleanupAndExit;
	} 
	else if (modelingData->hasNamedColorProf==NamedColorProfileAtBegin){
		err = ConvertNamedIndexToPCS(modelingData,myColors,count); 
		if (err) goto CleanupAndExit;
	}

	SetMem(&info,		sizeof(ColorSpaceInfo), 0);
	SetMem(&calcParam,	sizeof(CMCalcParam), 	0);
 
 	info.origSizeIn  = 16;
	info.origSizeOut = 16;
	info.usedSizeIn	 = 16;
	info.usedSizeOut = 16;

	FillLutParam(&lutParam, modelingData);
	FillCalcParamCM(&calcParam, &lutParam, myColors, count );
	
	info.inPlace = TRUE;
	calcRoutine = FindCalcRoutine( &calcParam, &lutParam, &info, modelingData->lookup );
	if (calcRoutine == nil)
	{
		err = cmMethodError;
		goto CleanupAndExit;
	}
		
	err = CallCalcProc(calcRoutine,&calcParam, &lutParam);
	if (err)
		goto CleanupAndExit;
	
	/* postprocess for NamedColor stuff */
	err = ConvertPCSToNamedIndex(modelingData,myColors,count); 

CleanupAndExit:
	UNLOCK_DATA((modelingData)->lutParam.inputLut);
	UNLOCK_DATA((modelingData)->lutParam.colorLut);
	UNLOCK_DATA((modelingData)->lutParam.outputLut);

	LH_END_PROC("LHMatchColorsPrivate")
	return err;
}

/*--------------------------------------------------------------------------------------------------------------
	LHCheckColorsPrivateMS
  --------------------------------------------------------------------------------------------------------------*/
CMError LHCheckColorsPrivateMS(	CMMModelPtr		modelingData, 
								CMColor*		myColors, 
								UINT32			count, 
								UINT8 			*result )
{
	CMCalcParam 	calcParam;
	CMLutParam		lutParam;
	CMError			err = -1;
	ColorSpaceInfo	info;
	CalcProcPtr		calcRoutine   = nil;

	LH_START_PROC("LHCheckColorsPrivateMS")
	
	if( (modelingData)->gamutLutParam.colorLut == 0 )return cmMethodError;
	LOCK_DATA((modelingData)->gamutLutParam.inputLut);
	LOCK_DATA((modelingData)->gamutLutParam.colorLut);
	LOCK_DATA((modelingData)->gamutLutParam.outputLut);

 	/* preprocess for NamedColor stuff */
	if (modelingData->hasNamedColorProf == NamedColorProfileOnly){
 		err = unimpErr;
 		goto CleanupAndExit;
	} 
	else if (modelingData->hasNamedColorProf==NamedColorProfileAtBegin){
		err = ConvertNamedIndexToPCS(modelingData,myColors,count); 
		if (err) goto CleanupAndExit;
	}

	SetMem(&info,		sizeof(ColorSpaceInfo), 0);
	SetMem(&calcParam,	sizeof(CMCalcParam), 	0);
 
 	info.origSizeIn  = 16;
	info.origSizeOut = 8;
	info.usedSizeIn	 = 16;
	info.usedSizeOut = 8;

	FillLutParamChk(&lutParam, modelingData);
	FillCalcParamCM(&calcParam, &lutParam, myColors, count );
	calcParam.outputData[0]	= (Ptr)result;
	calcParam.cmOutputBytesPerLine = count*sizeof(UINT8);
	calcParam.cmOutputPixelOffset = 1;
	calcParam.cmOutputColorSpace = cmGraySpace8Bit;	/* cmGraySpace is 16 bit */

	
	info.inPlace = (UINT8 *)myColors == result;
	calcRoutine = FindCalcRoutine( &calcParam, &lutParam, &info, modelingData->lookup );
	if (calcRoutine == nil)
	{
		err = cmMethodError;
		goto CleanupAndExit;
	}
		
	err = CallCalcProc(calcRoutine,&calcParam, &lutParam);
	if (err)
		goto CleanupAndExit;
	
	/* postprocess for NamedColor stuff */
	if( modelingData->hasNamedColorProf == NamedColorProfileAtEnd ){
 		err = unimpErr;
 		goto CleanupAndExit;
	}
CleanupAndExit:
	UNLOCK_DATA((modelingData)->gamutLutParam.inputLut);
	UNLOCK_DATA((modelingData)->gamutLutParam.colorLut);
	UNLOCK_DATA((modelingData)->gamutLutParam.outputLut);

	LH_END_PROC("LHCheckColorsPrivateMS")
	return err;
}

/*--------------------------------------------------------------------------------------------------------------
	LHCheckColorsPrivate
  --------------------------------------------------------------------------------------------------------------*/
CMError 
LHCheckColorsPrivate	( CMMModelPtr		modelingData, 
						  CMColor*			myColors, 
						  UINT32 			count, 
						  UINT8 			*result )
{
	CMCalcParam 	calcParam;
	CMLutParam		lutParam;
	CMError			err = -1;
	ColorSpaceInfo	info;
	CalcProcPtr		calcRoutine   = nil;
	
	LH_START_PROC("LHCheckColorsPrivate")
	
	SetMem(&info,		sizeof(ColorSpaceInfo), 0);
	SetMem(&calcParam,	sizeof(CMCalcParam), 	0);
 
 	/* disabling check colors with Named Color Profile */
 	if ( modelingData->hasNamedColorProf == NamedColorProfileOnly ||
		 modelingData->hasNamedColorProf == NamedColorProfileAtEnd ){
 		err = unimpErr;
 		goto CleanupAndExit;
 	}
	
 	info.origSizeIn  = 16;
	info.origSizeOut = 16;
	info.usedSizeIn	 = 16;
	info.usedSizeOut = 16;

	if( (modelingData)->gamutLutParam.colorLut == 0 )return cmMethodError;
	LOCK_DATA((modelingData)->gamutLutParam.inputLut);
	LOCK_DATA((modelingData)->gamutLutParam.colorLut);
	LOCK_DATA((modelingData)->gamutLutParam.outputLut);

	FillLutParamChk(&lutParam, modelingData);
	FillCalcParamCM(&calcParam, &lutParam, myColors, count );

	AllocBufferCheckCM(&calcParam, &info);
	
	info.inPlace = info.tempOutBuffer == nil;
	calcRoutine = FindCalcRoutine( &calcParam, &lutParam, &info, modelingData->lookup  );
	if (calcRoutine == nil)
	{
		err = cmMethodError;
		goto CleanupAndExit;
	}
		
	err = CallCalcProc(calcRoutine, &calcParam, &lutParam);
	
	Convert8To1( (Ptr)info.tempOutBuffer, (Ptr)result, info.processedLinesIn, calcParam.cmLineCount, count, count*sizeof(CMColor));			

	DisposeIfPtr((Ptr)info.tempOutBuffer);
	
CleanupAndExit:
	UNLOCK_DATA((modelingData)->gamutLutParam.inputLut);
	UNLOCK_DATA((modelingData)->gamutLutParam.colorLut);
	UNLOCK_DATA((modelingData)->gamutLutParam.outputLut);
	
	LH_END_PROC("LHCheckColorsPrivate")
	return err;
}

void CopyIndexData( CMBitmap *bitMapIn, CMBitmap *bitMapOut, ColorSpaceInfo *info );
void CopyIndexData( CMBitmap *bitMapIn, CMBitmap *bitMapOut, ColorSpaceInfo *info )
{
	Ptr			imgIn=NULL;
	Ptr			imgOut=NULL;
	SINT32		i,j;
#ifdef DEBUG_OUTPUT
	CMError			err = -1;
#endif	
	LH_START_PROC("CopyIndexData")

	for( i=0; i<bitMapIn->height; i++ ){
		imgIn = bitMapIn->image + i * bitMapIn->rowBytes;
		imgOut = bitMapOut->image + i * bitMapOut->rowBytes;
		if( info->inputPixelSize == 24 ){
			if( info->outputPixelSize == 24 ){
				for (j=0;j<bitMapIn->width;j++) {
					*imgOut++  = *imgIn++;
					*imgOut++  = *imgIn++;
					*imgOut++  = *imgIn++;
				}
			}
			else if( info->outputPixelSize == 32 ){
				for (j=0;j<bitMapIn->width;j++) {
					*imgOut++  = *imgIn++;
					*imgOut++  = *imgIn++;
					*imgOut++  = *imgIn++;
					imgOut++;
				}
			}
		}
		else if( info->inputPixelSize == 32 ){
			if( info->outputPixelSize == 24 ){
				for (j=0;j<bitMapIn->width;j++) {
					*imgOut++  = *imgIn++;
					*imgOut++  = *imgIn++;
					*imgOut++  = *imgIn++;
					imgIn++;
				}
			}
			else if( info->outputPixelSize == 32 ){
				for (j=0;j<bitMapIn->width;j++) {
					*imgOut++  = *imgIn++;
					*imgOut++  = *imgIn++;
					*imgOut++  = *imgIn++;
					imgOut++;
					imgIn++;
				}
			}
		}
	}
	LH_END_PROC("CopyIndexData")
	return;
}

#ifdef __MWERKS__
#pragma mark ================  Match/Check CMBitmaps  ================
#endif

/*--------------------------------------------------------------------------------------------------------------
	LHMatchBitMapPrivate
  --------------------------------------------------------------------------------------------------------------*/
CMError
LHMatchBitMapPrivate	 ( CMMModelPtr			modelingData, 
						   const CMBitmap *		inBitMap, 
						   CMBitmapCallBackUPP	progressProc, 
						   void *				refCon, 
						   CMBitmap *			outBitMap )
{
	CMCalcParam 	calcParam;
	CMLutParam		lutParam;
	CMError			err = -1;
	ColorSpaceInfo	info;
	CalcProcPtr		calcRoutine 	= nil;
	CMBitmap 		bitMapOut;
	CMBitmap 		bitMapIn 			= *inBitMap;
	OSType			inColorSpace 	= modelingData->firstColorSpace;
	OSType			outColorSpace 	= modelingData->lastColorSpace;
	Boolean			progressProcWasCalled = FALSE;
	SINT32			offset;
	SINT32			progressTimer;
	SINT32			dimLoop;
	Boolean			matchInPlace = FALSE;
	long			progressProcCount = 0;
	SINT32			inLineCount;
	Ptr				aBuffer;
	
	LH_START_PROC("LHMatchBitMapPrivate")

	LOCK_DATA((modelingData)->lutParam.inputLut);
	LOCK_DATA((modelingData)->lutParam.colorLut);
	LOCK_DATA((modelingData)->lutParam.outputLut);

	if( outBitMap == nil ){
		bitMapOut = *inBitMap;
		matchInPlace = TRUE;
	}
	else{
		bitMapOut 	= *outBitMap;
	}

	SetMem(&info, 		sizeof(ColorSpaceInfo), 0);
 	SetMem(&calcParam,	sizeof(CMCalcParam), 	0);

	FillLutParam(&lutParam, modelingData);
	FillCalcParam(&calcParam, &bitMapIn, &bitMapOut);
	
	err = CheckInputColorSpace( &bitMapIn, &calcParam, &info, inColorSpace, lutParam.colorLutInDim );
	inLineCount = calcParam.cmLineCount;
	if (err)
		goto CleanupAndExit;
		
	if (matchInPlace){
		err = SetOutputColorSpaceInplace( &calcParam, &info, outColorSpace);
		if (err)
			goto CleanupAndExit;
	}
	err = CheckOutputColorSpace( &bitMapOut, &calcParam, &info, outColorSpace, lutParam.colorLutOutDim );
	if (err)
		goto CleanupAndExit;
	
	if (matchInPlace)		/* matching in place - check if pixel sizes are ok */
	{
		if (info.inputPixelSize < info.outputPixelSize)
		{
			err = cmInvalidDstMap;
			goto CleanupAndExit;
		}		
		/* set the color space field to the output color space */
		bitMapIn.space = calcParam.cmOutputColorSpace;
	} else
	{
		calcParam.copyAlpha = (calcParam.cmInputColorSpace & cmAlphaSpace) && (calcParam.cmOutputColorSpace & cmAlphaSpace);
	}

	info.inPlace = bitMapOut.image == bitMapIn.image && info.tempInBuffer == nil && info.tempOutBuffer == nil;
	calcRoutine = FindCalcRoutine( &calcParam, &lutParam, &info, modelingData->lookup );
	if (calcRoutine == nil)
	{
		err = cmMethodError;
		goto CleanupAndExit;
	}
	
	if( info.inPlace && calcParam.cmInputPixelOffset < calcParam.cmOutputPixelOffset ){
			for( dimLoop=0; dimLoop<8; dimLoop++){					/* now work backwards				*/
				calcParam.inputData[dimLoop] = (Ptr)calcParam.inputData[dimLoop] + (calcParam.cmLineCount-1) * calcParam.cmInputBytesPerLine + (calcParam.cmPixelPerLine-1) * calcParam.cmInputPixelOffset;
				calcParam.outputData[dimLoop] = (Ptr)calcParam.outputData[dimLoop] + (calcParam.cmLineCount-1) * calcParam.cmOutputBytesPerLine + (calcParam.cmPixelPerLine-1) * calcParam.cmOutputPixelOffset;
			}
			calcParam.cmInputPixelOffset = -calcParam.cmInputPixelOffset;	
			calcParam.cmOutputPixelOffset = -calcParam.cmOutputPixelOffset;
			calcParam.cmInputBytesPerLine = -calcParam.cmInputBytesPerLine;
			calcParam.cmOutputBytesPerLine = -calcParam.cmOutputBytesPerLine;
	}
	/* ------------------------------------------------- 8 or 16 bit data --------------------------------------------------------- */
	if ((info.tempInBuffer == nil) && (info.tempOutBuffer == nil)&& modelingData->hasNamedColorProf == NoNamedColorProfile)
	{
		if (progressProc == nil)
			err = CallCalcProc(calcRoutine,&calcParam, &lutParam);		/* no callback proc - do it all in one step */
		else
		{
			calcParam.cmLineCount = kMaxTempBlock / calcParam.cmInputBytesPerLine;
			if (calcParam.cmLineCount < 1)
				calcParam.cmLineCount = 1;
			else if (calcParam.cmLineCount > bitMapIn.height)
				calcParam.cmLineCount = bitMapIn.height;
				
			progressTimer = TickCount();
			while (	info.processedLinesIn <  bitMapIn.height )
			{
				err = CallCalcProc(calcRoutine,&calcParam, &lutParam);
			
				for (dimLoop = 0; dimLoop< lutParam.colorLutInDim; dimLoop++)
					calcParam.inputData[dimLoop] = (Ptr)calcParam.inputData[dimLoop] + calcParam.cmLineCount * calcParam.cmInputBytesPerLine;
					
				for (dimLoop = 0; dimLoop< lutParam.colorLutOutDim; dimLoop++)
					calcParam.outputData[dimLoop] =  (Ptr)calcParam.outputData[dimLoop]  + calcParam.cmLineCount * calcParam.cmOutputBytesPerLine;
	
				info.processedLinesIn += calcParam.cmLineCount;
				if ( info.processedLinesIn + calcParam.cmLineCount > bitMapIn.height )
					calcParam.cmLineCount = bitMapIn.height-info.processedLinesIn;
						
				/* - - - - - handle CMBitmapCallBackProc - - - - - */
				if ( progressProc && ( progressTimer + kProgressTicks < (SINT32)TickCount()) )	
				{
					progressProcWasCalled = TRUE;
					if (CallCMBitmapCallBackProc ( progressProc, bitMapIn.height, info.processedLinesIn, (void *)refCon ))
					{
						info.processedLinesIn =  bitMapIn.height; 
						err = userCanceledErr;
					} else
						progressTimer = TickCount();
						progressProcCount = info.processedLinesIn;
				}
			}
		}
	}
	else if (modelingData->hasNamedColorProf != NamedColorProfileOnly ){
		info.processedLinesIn = 0;
		progressTimer = TickCount();
		if( info.tempInBuffer && info.tempOutBuffer ){
			if( inLineCount > calcParam.cmLineCount )inLineCount = calcParam.cmLineCount;
		}
		else if( info.tempInBuffer ){
			if( progressProc && modelingData->hasNamedColorProf == NamedColorProfileAtEnd ){
				inLineCount /= 32;
				if( inLineCount < 1 ) inLineCount = 1;
			}
		}
		else if( info.tempOutBuffer ){
			inLineCount = calcParam.cmLineCount;
		}
		else if( progressProc ){
			inLineCount = kMaxTempBlock / calcParam.cmInputBytesPerLine;
			if (inLineCount < 1)
				inLineCount = 1;
			else if (inLineCount > bitMapIn.height)
				inLineCount = bitMapIn.height;
			if( modelingData->hasNamedColorProf == NamedColorProfileAtEnd ){
				inLineCount /= 32;
				if( inLineCount < 1 ) inLineCount = 1;
			}
		}
		while (	info.processedLinesIn <  bitMapIn.height )
		{
			if ( info.processedLinesIn + inLineCount > bitMapIn.height )
				inLineCount = bitMapIn.height-info.processedLinesIn;

			if (info.origSizeIn == 5 )																			/* input is 5 bit */
				Convert5To8  ( bitMapIn.image, (Ptr)info.tempInBuffer, info.processedLinesIn, inLineCount, bitMapIn.width, bitMapIn.rowBytes );
#ifdef PI_Application_h
			else if (info.origSizeIn == 6 )																			/* output is 5 bit */
				Convert565To8 ( bitMapIn.image, (Ptr)info.tempInBuffer, info.processedLinesIn, inLineCount, bitMapIn.width, bitMapIn.rowBytes );
#endif
			else if (info.origSizeIn == 10 ) 
					Convert10To16  ( bitMapIn.image, (Ptr)info.tempInBuffer, info.processedLinesIn, inLineCount, bitMapIn.width, bitMapIn.rowBytes );
			
			if (modelingData->hasNamedColorProf == NamedColorProfileAtBegin ){
				err = ConvertIndexToLabBitmap(	modelingData, bitMapIn.image, 
												(Ptr)info.tempInBuffer, info.processedLinesIn, 
												inLineCount, bitMapIn.width, 
												bitMapIn.rowBytes, 
												calcParam.cmInputBytesPerLine, 
												calcParam.cmInputPixelOffset*8 );
				if (err) 
					goto CleanupAndExit;
			}
			calcParam.cmLineCount = inLineCount;
			err = CallCalcProc( calcRoutine, &calcParam, &lutParam );
			if (err) 
				goto CleanupAndExit;

			if (modelingData->hasNamedColorProf == NamedColorProfileAtEnd ){
				if( info.tempOutBuffer )aBuffer = (Ptr)info.tempOutBuffer;
				else aBuffer = bitMapOut.image;
				err = ConvertLabToIndexBitmap(	modelingData, aBuffer, 
												info.processedLinesIn, 
												calcParam.cmLineCount,
												bitMapOut.width, bitMapOut.rowBytes );
				if (err) 
					goto CleanupAndExit;
			}

			if( info.origSizeOut == 5 )																			/* output is 5 bit */
				Convert8To5 ( (Ptr)info.tempOutBuffer, bitMapOut.image, info.processedLinesIn, calcParam.cmLineCount, bitMapOut.width, bitMapOut.rowBytes);
#ifdef PI_Application_h
			else if( info.origSizeOut == 6 )																			/* output is 5 bit */
				Convert8To565 ( (Ptr)info.tempOutBuffer, bitMapOut.image, info.processedLinesIn, calcParam.cmLineCount, bitMapOut.width, bitMapOut.rowBytes);
#endif
			else if( info.origSizeOut == 10 )																								/* output is 10 bit */
				Convert16To10 ( (Ptr)info.tempOutBuffer, bitMapOut.image, info.processedLinesIn, calcParam.cmLineCount, bitMapOut.width, bitMapOut.rowBytes);

			info.processedLinesIn += inLineCount;
			/* - - - - - handle CMBitmapCallBackProc - - - - - */
			if ( progressProc && ( progressTimer + kProgressTicks < (SINT32)TickCount()) )	
			{
				progressProcWasCalled = TRUE;
				if (CallCMBitmapCallBackProc ( progressProc, bitMapIn.height, info.processedLinesIn, (void *)refCon ))
				{
					info.processedLinesIn =  bitMapIn.height; 
					err = userCanceledErr;
				} else
					progressTimer = TickCount();
					progressProcCount = info.processedLinesIn;
			}
			if( !info.tempInBuffer ){
				offset = calcParam.cmLineCount * calcParam.cmInputBytesPerLine;
				for (dimLoop = 0; dimLoop< lutParam.colorLutInDim; dimLoop++)
					calcParam.inputData[dimLoop] = (Ptr)calcParam.inputData[dimLoop] + offset;
			}
			if( !info.tempOutBuffer ){
				offset = calcParam.cmLineCount * calcParam.cmOutputBytesPerLine;
				for (dimLoop = 0; dimLoop< lutParam.colorLutOutDim; dimLoop++)
					calcParam.outputData[dimLoop] =  (Ptr)calcParam.outputData[dimLoop]  + offset;
			}
		}
	}
	else{
		CopyIndexData( &bitMapIn, &bitMapOut, &info );
	}
	if (err) 
		goto CleanupAndExit;
	
	if( progressProcWasCalled && progressProcCount != bitMapIn.height )
		CallCMBitmapCallBackProc ( progressProc, bitMapIn.height, bitMapIn.height, (void *)refCon );
CleanupAndExit:

  /* Clean up temporary storage */
	DisposeIfPtr((Ptr)info.tempInBuffer);
	DisposeIfPtr((Ptr)info.tempOutBuffer);
	
  UNLOCK_DATA((modelingData)->lutParam.inputLut);
	UNLOCK_DATA((modelingData)->lutParam.colorLut);
	UNLOCK_DATA((modelingData)->lutParam.outputLut);

	LH_END_PROC("LHMatchBitMapPrivate")
	
	return err;
}

/*--------------------------------------------------------------------------------------------------------------
	LHCheckBitMapPrivate
  --------------------------------------------------------------------------------------------------------------*/
CMError 
LHCheckBitMapPrivate	( CMMModelPtr			modelingData, 
						  const CMBitmap		*inBitMap,
						  CMBitmapCallBackUPP	progressProc,
						  void *				refCon, 
						  CMBitmap 				*outBitMap )
{
	CMCalcParam 	calcParam;
	CMLutParam		lutParam;
	CMError			err = -1;
	ColorSpaceInfo	info;
	CalcProcPtr		calcRoutine 	= nil;
	CMBitmap 		bitMapIn 			= *inBitMap;
	CMBitmap 		bitMapOut;
	OSType			inColorSpace 	= modelingData->firstColorSpace;
	Boolean			progressProcWasCalled = FALSE;
	SINT32			offset;
	SINT32			progressTimer;
	SINT32			dimLoop;
	Boolean			matchInPlace = FALSE;
	long			progressProcCount = 0;
	SINT32			inLineCount;
	
	LH_START_PROC("LHCheckBitMapPrivate")

	if( (modelingData)->gamutLutParam.colorLut == 0 )return cmMethodError;
	LOCK_DATA((modelingData)->gamutLutParam.inputLut);
	LOCK_DATA((modelingData)->gamutLutParam.colorLut);
	LOCK_DATA((modelingData)->gamutLutParam.outputLut);

	SetMem(&info, 		sizeof(ColorSpaceInfo), 0);
 	SetMem(&calcParam,	sizeof(CMCalcParam), 	0);

	if ( (outBitMap == nil ) || (modelingData->gamutLutParam.colorLut == nil) )
	{
		err = cmInvalidDstMap;
		goto CleanupAndExit;
	}
	bitMapOut = *outBitMap;
	FillLutParamChk(&lutParam, modelingData);
	FillCalcParam(&calcParam, &bitMapIn, &bitMapOut);
	
	err = CheckInputColorSpace( &bitMapIn, &calcParam, &info, inColorSpace, lutParam.colorLutInDim );
	inLineCount = calcParam.cmLineCount;
	if (err)
		goto CleanupAndExit;
		
	err = CheckOutputColorSpaceChk( &bitMapOut, &calcParam, &info );
	if (err)
		goto CleanupAndExit;
	
	info.inPlace = bitMapOut.image == bitMapIn.image && info.tempInBuffer == nil && info.tempOutBuffer == nil;
	calcRoutine = FindCalcRoutine( &calcParam, &lutParam, &info, modelingData->lookup );
	if (calcRoutine == nil)
	{
		err = cmMethodError;
		goto CleanupAndExit;
	}
	
	/* ------------------------------------------------- 8 or 16 bit data --------------------------------------------------------- */
	if ((info.tempInBuffer == nil) && (info.tempOutBuffer == nil) && modelingData->hasNamedColorProf == NoNamedColorProfile)
	{
		if (progressProc == nil)
			err = CallCalcProc(calcRoutine,&calcParam, &lutParam);		/* no callback proc - do it all in one step */
		else
		{
			calcParam.cmLineCount = kMaxTempBlock / calcParam.cmInputBytesPerLine;
			if (calcParam.cmLineCount < 1)
				calcParam.cmLineCount = 1;
			else if (calcParam.cmLineCount > bitMapIn.height)
				calcParam.cmLineCount = bitMapIn.height;
				
			progressTimer = TickCount();
			while (	info.processedLinesIn <  bitMapIn.height )
			{
				err = CallCalcProc(calcRoutine,&calcParam, &lutParam);
			
				for (dimLoop = 0; dimLoop< lutParam.colorLutInDim; dimLoop++)
					calcParam.inputData[dimLoop] = (Ptr)calcParam.inputData[dimLoop] + calcParam.cmLineCount * calcParam.cmInputBytesPerLine;
					
				for (dimLoop = 0; dimLoop< lutParam.colorLutOutDim; dimLoop++)
					calcParam.outputData[dimLoop] =  (Ptr)calcParam.outputData[dimLoop]  + calcParam.cmLineCount * calcParam.cmOutputBytesPerLine;
	
				info.processedLinesIn += calcParam.cmLineCount;
				if ( info.processedLinesIn + calcParam.cmLineCount > bitMapIn.height )
					calcParam.cmLineCount = bitMapIn.height-info.processedLinesIn;
						
				/* - - - - - handle CMBitmapCallBackProc - - - - - */
				if ( progressProc && ( progressTimer + kProgressTicks < (SINT32)TickCount()) )	
				{
					progressProcWasCalled = TRUE;
					if (CallCMBitmapCallBackProc ( progressProc, bitMapIn.height, info.processedLinesIn, (void *)refCon ))
					{
						info.processedLinesIn =  bitMapIn.height; 
						err = userCanceledErr;
					} else
						progressTimer = TickCount();
						progressProcCount = info.processedLinesIn;
				}
			}
		}
	}
	else if (modelingData->hasNamedColorProf != NamedColorProfileOnly ){
		info.processedLinesIn = 0;
		progressTimer = TickCount();
		if( info.tempInBuffer && info.tempOutBuffer ){
			if( inLineCount > calcParam.cmLineCount )inLineCount = calcParam.cmLineCount;
		}
		else if( info.tempInBuffer ){
		}
		else if( info.tempOutBuffer ){
			inLineCount = calcParam.cmLineCount;
		}
		else if( progressProc ){
			inLineCount = kMaxTempBlock / calcParam.cmInputBytesPerLine;
			if (inLineCount < 1)
				inLineCount = 1;
			else if (inLineCount > bitMapIn.height)
				inLineCount = bitMapIn.height;
		}
		while (	info.processedLinesIn <  bitMapIn.height )
		{
			if ( info.processedLinesIn + inLineCount > bitMapIn.height )
				inLineCount = bitMapIn.height-info.processedLinesIn;

			if (info.origSizeIn == 5 )																			/* input is 5 bit */
				Convert5To8  ( bitMapIn.image, (Ptr)info.tempInBuffer, info.processedLinesIn, inLineCount, bitMapIn.width, bitMapIn.rowBytes );
#ifdef PI_Application_h
			else if (info.origSizeIn == 6 )																			/* output is 5 bit */
				Convert565To8 ( bitMapIn.image, (Ptr)info.tempInBuffer, info.processedLinesIn, inLineCount, bitMapIn.width, bitMapIn.rowBytes );
#endif
			else if (info.origSizeIn == 10 ) 
					Convert10To16  ( bitMapIn.image, (Ptr)info.tempInBuffer, info.processedLinesIn, inLineCount, bitMapIn.width, bitMapIn.rowBytes );
			
			if (modelingData->hasNamedColorProf == NamedColorProfileAtBegin ){
				err = ConvertIndexToLabBitmap(	modelingData, bitMapIn.image, 
												(Ptr)info.tempInBuffer, info.processedLinesIn, 
												inLineCount, bitMapIn.width, 
												bitMapIn.rowBytes, 
												calcParam.cmInputBytesPerLine, 
												calcParam.cmInputPixelOffset*8 );
				if (err) 
					goto CleanupAndExit;
			}
			calcParam.cmLineCount = inLineCount;
			err = CallCalcProc( calcRoutine, &calcParam, &lutParam );
			if (err) 
				goto CleanupAndExit;

			if( info.origSizeOut == 1 )																			/* output is 5 bit */
				Convert8To1 ( (Ptr)info.tempOutBuffer, bitMapOut.image, info.processedLinesIn, calcParam.cmLineCount, bitMapOut.width, bitMapOut.rowBytes);

			info.processedLinesIn += inLineCount;
			/* - - - - - handle CMBitmapCallBackProc - - - - - */
			if ( progressProc && ( progressTimer + kProgressTicks < (SINT32)TickCount()) )	
			{
				progressProcWasCalled = TRUE;
				if (CallCMBitmapCallBackProc ( progressProc, bitMapIn.height, info.processedLinesIn, (void *)refCon ))
				{
					info.processedLinesIn =  bitMapIn.height; 
					err = userCanceledErr;
				} else
					progressTimer = TickCount();
					progressProcCount = info.processedLinesIn;
			}
			if( !info.tempInBuffer ){
				offset = calcParam.cmLineCount * calcParam.cmInputBytesPerLine;
				for (dimLoop = 0; dimLoop< lutParam.colorLutInDim; dimLoop++)
					calcParam.inputData[dimLoop] = (Ptr)calcParam.inputData[dimLoop] + offset;
			}
			if( !info.tempOutBuffer ){
				offset = calcParam.cmLineCount * calcParam.cmOutputBytesPerLine;
				for (dimLoop = 0; dimLoop< lutParam.colorLutOutDim; dimLoop++)
					calcParam.outputData[dimLoop] =  (Ptr)calcParam.outputData[dimLoop]  + offset;
			}
		}
		DisposeIfPtr((Ptr)info.tempInBuffer);
		DisposeIfPtr((Ptr)info.tempOutBuffer);
	}
	else{
		DisposeIfPtr((Ptr)info.tempInBuffer);
		err = cmMethodError;
		goto CleanupAndExit;
	}
	if (err) 
		goto CleanupAndExit;
	
	if( progressProcWasCalled && progressProcCount != bitMapIn.height )
		CallCMBitmapCallBackProc ( progressProc, bitMapIn.height, bitMapIn.height, (void *)refCon );
CleanupAndExit:
	UNLOCK_DATA((modelingData)->gamutLutParam.inputLut);
	UNLOCK_DATA((modelingData)->gamutLutParam.colorLut);
	UNLOCK_DATA((modelingData)->gamutLutParam.outputLut);

	LH_END_PROC("LHCheckBitMapPrivate")
	
	return err;
}

#ifdef __MWERKS__
#pragma mark ================  Match/Check CMBitmaps Plane ================
#endif

/*--------------------------------------------------------------------------------------------------------------
	CMError
	LHMatchBitMapPlanePrivate	 ( CMMModelPtr				modelingData, 
								   const LH_CMBitmapPlane *	bitMapLH, 
								   CMBitmapCallBackUPP		progressProc, 
							  	   void *					refCon, 
								   LH_CMBitmapPlane *		matchedBitMapLH )
  --------------------------------------------------------------------------------------------------------------*/
CMError
LHMatchBitMapPlanePrivate	 ( CMMModelPtr				modelingData, 
							   const LH_CMBitmapPlane *	bitMapLH, 
							   CMBitmapCallBackUPP		progressProc, 
						  	   void *					refCon, 
							   LH_CMBitmapPlane *		matchedBitMapLH )
{
	CMCalcParam 	calcParam;
	CMLutParam		lutParam;
	CMError			err = -1;
	ColorSpaceInfo	info;
	CalcProcPtr		calcRoutine 		= nil;
	LH_CMBitmapPlane *	secondBitmapLH 	= matchedBitMapLH;
	OSType			inColorSpace 		= (modelingData)->firstColorSpace;
	OSType			outColorSpace 		= (modelingData)->lastColorSpace;
	Boolean			progressProcWasCalled = FALSE;
	Boolean			matchInPlace = FALSE;
	SINT32			dimLoop;
	
	LH_START_PROC("LHMatchBitMapPlanePrivate")
	
	SetMem(&info, 		sizeof(ColorSpaceInfo), 0);
 	SetMem(&calcParam,	sizeof(CMCalcParam), 	0);
 	
	if (secondBitmapLH == nil)
	{
		secondBitmapLH = (LH_CMBitmapPlane *)bitMapLH;
		matchInPlace = TRUE;
	}
	
	LOCK_DATA((modelingData)->lutParam.inputLut);
	LOCK_DATA((modelingData)->lutParam.colorLut);
	LOCK_DATA((modelingData)->lutParam.outputLut);
	
	/* create CMBitmap based on the LH_CMBitmapPlane - so we can use the 'standard' setup functions */
	{
		CMBitmap		bitMap;
		CMBitmap		secondBitmap;
	
		bitMap.image			= bitMapLH->image[0];
		bitMap.width			= bitMapLH->width;
		bitMap.height			= bitMapLH->height;
		bitMap.rowBytes			= bitMapLH->rowBytes;
		bitMap.pixelSize		= bitMapLH->pixelSize;
		bitMap.space			= bitMapLH->space;
	
		secondBitmap.image		= secondBitmapLH->image[0];
		secondBitmap.width		= secondBitmapLH->width;
		secondBitmap.height		= secondBitmapLH->height;
		secondBitmap.rowBytes	= secondBitmapLH->rowBytes;
		secondBitmap.pixelSize	= secondBitmapLH->pixelSize;
		secondBitmap.space		= secondBitmapLH->space;
	
		FillLutParam(&lutParam, modelingData);
		FillCalcParam(&calcParam, &bitMap, &secondBitmap);

		err = CheckInputColorSpace(&bitMap,&calcParam, &info, inColorSpace, lutParam.colorLutInDim );
		if (err)
			goto CleanupAndExit;
		err = CheckOutputColorSpace(&secondBitmap,&calcParam, &info, outColorSpace, lutParam.colorLutOutDim );
		if (err)
			goto CleanupAndExit;
	}
	
	/* now update the input/output data pointers */
	for (dimLoop = 0; dimLoop < 8; dimLoop++)
	{
		calcParam.inputData[dimLoop]	= bitMapLH->image[dimLoop];
		calcParam.outputData[dimLoop]	= secondBitmapLH->image[dimLoop];
	}
	
	calcParam.cmInputPixelOffset = bitMapLH->elementOffset;
	calcParam.cmOutputPixelOffset = secondBitmapLH->elementOffset;
	if (bitMapLH == secondBitmapLH)		/* matching in place - check if pixeloffsets are ok */
	{
		if (calcParam.cmInputPixelOffset < calcParam.cmOutputPixelOffset)
		{
			err = cmInvalidDstMap;
			goto CleanupAndExit;
		}
		if (info.origSizeIn * lutParam.colorLutInDim < info.origSizeOut * lutParam.colorLutOutDim)
		{
			err = cmInvalidDstMap;
			goto CleanupAndExit;
		}
	} else
	{
		calcParam.copyAlpha = (calcParam.cmInputColorSpace & cmAlphaSpace) && (calcParam.cmOutputColorSpace & cmAlphaSpace);
	}

	info.inPlace = info.tempInBuffer == nil && info.tempOutBuffer == nil;
	calcRoutine = FindCalcRoutine( &calcParam, &lutParam, &info, modelingData->lookup );
	if (calcRoutine == nil)
	{
		err = cmMethodError;
		goto CleanupAndExit;
	}
	
	/* ------------------------------------------------- 8 or 16 bit data --------------------------------------------------------- */
	if ((info.tempInBuffer == nil) && (info.tempOutBuffer == nil))
	{
		err = CallCalcProc(calcRoutine,&calcParam, &lutParam);
	}
	/* ----------------------------------------------- input is 5 or 10 bit ------------------------------------------------------- */
	else if ((info.tempInBuffer != nil) && (info.tempOutBuffer == nil))
	{
		DisposeIfPtr((Ptr)info.tempInBuffer);
	}
	/* ----------------------------------------------- output is 5 or 10 bit ------------------------------------------------------- */
	else if ((info.tempInBuffer == nil) && (info.tempOutBuffer != nil))
	{
		DisposeIfPtr((Ptr)info.tempOutBuffer);
	}
	/* ------------------------------------------ input and output are 5 or 10 bit ------------------------------------------------- */
	else
	{
		DisposeIfPtr((Ptr)info.tempInBuffer);
		DisposeIfPtr((Ptr)info.tempOutBuffer);
	}
	
	if (progressProcWasCalled)
		CallCMBitmapCallBackProc ( progressProc, 0, 0, (void *)refCon );
CleanupAndExit:
	UNLOCK_DATA((modelingData)->lutParam.inputLut);
	UNLOCK_DATA((modelingData)->lutParam.colorLut);
	UNLOCK_DATA((modelingData)->lutParam.outputLut);
		
	LH_END_PROC("LHMatchBitMapPlanePrivate")
	return err;
}

#ifdef __MWERKS__
#pragma mark ================  Utilities for NamedColor ================
#endif

/*--------------------------------------------------------------------------------------------------------------
OSErr ConvertNamedIndexToColors(	CMMModelPtr	modelingData,
									CMColor 	*theData, 
									SINT32 		pixCnt )
  --------------------------------------------------------------------------------------------------------------*/
OSErr ConvertNamedIndexToColors(	CMMModelPtr	modelingData,
									CMColor 	*theData, 
									SINT32 		pixCnt )
{
	UINT16			*tagTbl = NULL;
	LUT_DATA_TYPE	tagH = NULL;
	OSErr			err = noErr;
	UINT32			i,index;
	UINT16			*colorPtr=NULL;
	int				elemSz,deviceChannelCount,count;

	LH_START_PROC("ConvertNamedIndexToColors")
	
	if ( modelingData->hasNamedColorProf!=NoNamedColorProfile) 
	{
		/* prepare the tag table */
		tagH = modelingData->theNamedColorTagData;
		if (tagH==NULL) 
		{
			err = cmparamErr;
			goto CleanUp;
		}
		LOCK_DATA(tagH);
		
		/* tagTbl should now point to beginning of first device data */
		/* = CMNamedColor2Type_header(84) + firstName(32) + PCSSize(3*2) */
		tagTbl = (UINT16 *)DATA_2_PTR(tagH) + 61;
		
		/* find out how many bytes to skip per element. div'ed 2 for indexing purpose */
		count = ((icNamedColor2Type *)DATA_2_PTR(tagH))->ncolor.count;
		deviceChannelCount = ((icNamedColor2Type *)DATA_2_PTR(tagH))->ncolor.nDeviceCoords;
		if (deviceChannelCount==3) 
		{
			elemSz = 32+(3+3)*sizeof(SINT16);
		} else if (deviceChannelCount == 4) 
		{
			elemSz = 32+(3+4)*sizeof(SINT16);
		} else if (deviceChannelCount == 0) 
		{
			elemSz = 32+(3+0)*sizeof(SINT16);
		} else 
		{
			err = cmparamErr;
			goto CleanUp;
		}
		elemSz/=2;
		
		/* doing the actual */
		for (i=0; i<(UINT32)pixCnt; i++) 
		{
			/* go to the index'th element*/
			index = theData->namedColor.namedColorIndex;
			if (index > (UINT32)count || index == 0 ) 
			{
				err = cmNamedColorNotFound;
				goto CleanUp;
			}
			colorPtr = tagTbl + (index-1)*elemSz;
			/* put in all the info */
			if (deviceChannelCount==3) 
			{
				theData->rgb.red = *colorPtr++;
				theData->rgb.green = *colorPtr++;
				theData->rgb.blue = *colorPtr++;
			} else if (deviceChannelCount==4) 
			{
				theData->cmyk.cyan = *colorPtr++;
				theData->cmyk.magenta = *colorPtr++;
				theData->cmyk.yellow = *colorPtr++;
				theData->cmyk.black = *colorPtr++;
			}
			theData++;
		}
		UNLOCK_DATA(tagH);
	}
CleanUp:
	LH_END_PROC("ConvertNamedIndexToColors")
	return err;
}

/*--------------------------------------------------------------------------------------------------------------
OSErr ConvertNamedIndexToPCS(	CMMModelPtr		cw,
								CMColor 		*theData, 
								SINT32 			pixCnt )
  --------------------------------------------------------------------------------------------------------------*/
OSErr ConvertNamedIndexToPCS(	CMMModelPtr		cw,
								CMColor 		*theData, 
								SINT32 			pixCnt )
{
	UINT16			*tagTbl = NULL;
	LUT_DATA_TYPE	tagH = NULL;
	OSErr			err = noErr;
	UINT32			i,index;
	UINT16			*colorPtr=NULL;
	int				elemSz,deviceChannelCount,count;
	CMMModelPtr		modelingData = (CMMModelPtr)cw;

	
	LH_START_PROC("ConvertNamedIndexToPCS")
	if( cw == 0 )return cmparamErr;
	{
		/* prepare the tag table */
			tagH = modelingData->theNamedColorTagData;
		if (tagH == NULL) 
		{
			err = cmparamErr;
			goto CleanUp;
		}
		LOCK_DATA(tagH);
		
		/* tagTbl should now point to beginning of first PCS data */
		/* = CMNamedColor2Type_header(84) + firstName(32) */
		tagTbl = (UINT16 *)DATA_2_PTR(tagH) + 58;
		
		/* find out how many bytes to skip per element. div'ed 2 for indexing purpose */
		count = ((icNamedColor2Type *)DATA_2_PTR(tagH))->ncolor.count;
		deviceChannelCount = ((icNamedColor2Type *)DATA_2_PTR(tagH))->ncolor.nDeviceCoords;
		if (deviceChannelCount == 3) 
		{
			elemSz = 32+(3+3)*sizeof(SINT16);
		} else if (deviceChannelCount == 4) 
		{
			elemSz = 32+(3+4)*sizeof(SINT16);
		} else if (deviceChannelCount == 0) 
		{
			elemSz = 32+(3+0)*sizeof(SINT16);
		} else 
		{
			err = cmparamErr;
			goto CleanUp;
		}
		elemSz /= 2;
		
		/* doing the actual */
		for (i=0; i<(UINT32)pixCnt; i++) 
		{
			/* go to the index'th element*/
			index = theData->namedColor.namedColorIndex;
			if (index > (UINT32)count || index == 0 ) 
			{
				err = cmNamedColorNotFound;
				goto CleanUp;
			}
			colorPtr = tagTbl + (index-1)*elemSz;
			/* put in all the info */
			theData->Lab.L = *colorPtr++;
			theData->Lab.a = *colorPtr++;
			theData->Lab.b = *colorPtr++;
			theData++;
		}
		UNLOCK_DATA(tagH);
	}
CleanUp:
	LH_END_PROC("ConvertNamedIndexToPCS")
	return err;
}



#define	POS(x)	((x) > (0) ? (UINT32)(x) : (UINT32)(-(x)))
/*--------------------------------------------------------------------------------------------------------------
OSErr ConvertPCSToNamedIndex(	CMMModelPtr	modelingData,
								CMColor 	*theData, 
								SINT32 		pixCnt )
  --------------------------------------------------------------------------------------------------------------*/
OSErr ConvertPCSToNamedIndex(	CMMModelPtr	modelingData,
								CMColor 	*theData, 
								SINT32 		pixCnt )
{
	UINT16			*tagTbl = NULL;
	LUT_DATA_TYPE	tagH = NULL;
	OSErr			err = noErr;
	UINT32			i,j,index,dE,dEnow;
	UINT16			*colorPtr=NULL;
	int				elemSz,deviceChannelCount,count;
	
	LH_START_PROC("ConvertPCSToNamedIndex")
	if (modelingData->hasNamedColorProf == NamedColorProfileAtEnd) 
	{
		/* prepare the tag table */
		tagH = modelingData->theNamedColorTagData;
		if (tagH==NULL) 
		{
			err = cmparamErr;
			goto CleanUp;
		}
		LOCK_DATA(tagH);
		
		/* tagTbl should now point to beginning of first PCS data */
		/* = CMNamedColor2Type_header(84) + firstName(32) */
		tagTbl = (UINT16 *)DATA_2_PTR(tagH) + 58;
		
		/* find out how many bytes to skip per element. div'ed 2 for indexing purpose */
		count = ((icNamedColor2Type *)DATA_2_PTR(tagH))->ncolor.count;
		deviceChannelCount = ((icNamedColor2Type *)DATA_2_PTR(tagH))->ncolor.nDeviceCoords;
		if (deviceChannelCount == 3) 
		{
			elemSz = 32+(3+3)*sizeof(SINT16);
		} else if (deviceChannelCount == 4) 
		{
			elemSz = 32+(3+4)*sizeof(SINT16);
		} else if (deviceChannelCount == 0) 
		{
			elemSz = 32+(3+0)*sizeof(SINT16);
		} else 
		{
			err = cmparamErr;
			goto CleanUp;
		}
		elemSz/=2;
		
		/* doing the actual */
		for (i=0; i < (UINT32)pixCnt; i++) 
		{
			/* go through the whole table to find the closest one*/
			dEnow = 0x40000;	/* just arbitrarily high = 256*256*4 */
			index = (UINT32)-1;
			colorPtr = tagTbl;
			for (j=0; j < (UINT32)count; j++) 
			{
				dE =      POS((long)theData->Lab.a - *(colorPtr+1));
				dE = dE + POS((long)theData->Lab.b - *(colorPtr+2));
				dE = 2*dE + (dE>>1) + POS((long)theData->Lab.L - *(colorPtr));
				if (dE < dEnow) 
				{
					index = j;
					dEnow = dE;
				}
				colorPtr += elemSz;
			}
			theData->namedColor.namedColorIndex = index+1;
			theData++;
		}
		UNLOCK_DATA(tagH);
	}
CleanUp:
	LH_END_PROC("ConvertPCSToNamedIndex")
	return err;
}

/*--------------------------------------------------------------------------------------------------------------
CMError CMConvertNamedIndexToColors(	CMWorldRef		cw,
										CMColor 		*theData, 
										unsigned long	pixCnt )
  --------------------------------------------------------------------------------------------------------------*/
CMError CMConvertNamedIndexToColors( CMWorldRef cw, CMColor *theData, unsigned long pixCnt );
CMError CMConvertNamedIndexToColors( CMWorldRef cw, CMColor *theData, unsigned long pixCnt )
{
	return ConvertNamedIndexToColors( (CMMModelPtr)cw, theData, (SINT32)pixCnt );
}
/*--------------------------------------------------------------------------------------------------------------
CMError CMConvertNamedIndexToPCS(	CMWorldRef		cw,
									CMColor 		*theData, 
									unsigned long	pixCnt )
  --------------------------------------------------------------------------------------------------------------*/
CMError CMConvertNamedIndexToPCS( CMWorldRef cw, CMColor *theData, unsigned long pixCnt );
CMError CMConvertNamedIndexToPCS( CMWorldRef cw, CMColor *theData, unsigned long pixCnt )
{
	return ConvertNamedIndexToPCS( (CMMModelPtr)cw, theData, (SINT32)pixCnt );
}
CMError ConvertLabToIndexBitmap(	CMMModelPtr	modelingData,
									Ptr 		InBuffer,
									UINT32		processedLinesIn,
									UINT32		inLineCount,
									UINT32		width,
									UINT32		rowBytes )
{
	OSErr			err = noErr;
	LUT_DATA_TYPE	tagH=NULL;
	UINT16			*tagTbl = NULL,*colorPtr = NULL;
	int				deviceChannelCount,elemSz,count;
	UINT32			i,j,k,index,dE,dEnow;
	UINT16			LL,aa,bb;
	UINT8			*imgIn=NULL,*imgOut=NULL;
	UINT32			*imgInPtr=NULL;
	UINT8			*imgInPtr8=NULL;
	UINT8			*imgOutPtr8=NULL;
	
	LH_START_PROC("ConvertLabToIndexBitmap")
	/* prepare the tag table  */
	tagH = modelingData->theNamedColorTagData;
	if (tagH==NULL){
		err = cmparamErr;
		goto CleanUp;
	}
	LOCK_DATA(tagH);
	tagTbl = (UINT16 *)DATA_2_PTR(tagH) + 58;
	count = ((icNamedColor2Type *)DATA_2_PTR(tagH))->ncolor.count;
	deviceChannelCount = ((icNamedColor2Type *)DATA_2_PTR(tagH))->ncolor.nDeviceCoords;
	if (deviceChannelCount == 3){
		elemSz = 32+(3+3)*sizeof(SINT16);
	}
	else if (deviceChannelCount == 4){
		elemSz = 32+(3+4)*sizeof(SINT16);
	}
	else if (deviceChannelCount == 0){
		elemSz = 32+(3+0)*sizeof(SINT16);
	}
	else{
		err = cmparamErr;
		goto CleanUp;
	}
	elemSz /= 2;
	
	/* search for index */
	imgIn = (UINT8*)InBuffer + processedLinesIn * rowBytes;
	imgInPtr8=((UINT8 *)imgIn);
	LL = *(imgInPtr8+0)+1;	/* do not use cache for first pixel */
	for (i = 0; i < inLineCount; i++){
		for (j = 0; j < width; j++){
			imgInPtr8=((UINT8 *)imgIn+j*4);
			if( LL == *(imgInPtr8+0) ){
				if( aa == *(imgInPtr8+1) && bb == *(imgInPtr8+2) ){
					*((UINT32 *)imgIn+j) = index+1;
					continue;
				}
			}
			LL = *(imgInPtr8+0);
			aa = *(imgInPtr8+1);
			bb = *(imgInPtr8+2);
			/* go through the whole table to find the closest one*/
			dEnow = 0x40000;	/* just arbitrarily high = 256*256*4 */
			index =(UINT32)-1;
			colorPtr = tagTbl;
			for (k = 0; k < (UINT32)count; k++){
				dE =      POS((long)aa - (*(colorPtr+1)>>8));
				dE = dE + POS((long)bb - (*(colorPtr+2)>>8));
				dE = 2*dE + (dE>>1) + POS((long)LL - (*(colorPtr)>>8));			/* Quantization L = 2.55 * (a|b) */
				if (dE < dEnow){
					index = k;
					dEnow = dE;
				}
				colorPtr += elemSz;
			}
			*((UINT32 *)imgIn+j) = index+1;
		}
		imgIn+=rowBytes;
	}
CleanUp:
	UNLOCK_DATA(tagH);
	LH_END_PROC("ConvertLabToIndexBitmap")
	return err;
}
CMError ConvertIndexToLabBitmap(	CMMModelPtr	modelingData,
									Ptr		 	InBuffer,
									Ptr		 	OutBuffer,
									UINT32		processedLinesIn,
									UINT32		lineCount,
									UINT32		inWidth,
									UINT32		inRowBytes,
									UINT32		outRowBytes,
									UINT32		outputSize )
{
	OSErr			err = noErr;
	LUT_DATA_TYPE	tagH=NULL;
	UINT16			*tagTbl = NULL,*ColorPtr = NULL;
	UINT32			*imgIn=NULL;
	UINT8			*imgOut=NULL;
	int				deviceChannelCount,elemSz;
	UINT32			i,j,index,countNamesInProfile;
	
	LH_START_PROC("ConvertIndexToLabBitmap")
	/* set up the table for indexing */
	tagH = modelingData->theNamedColorTagData;
	if (tagH==NULL) {
		err = cmparamErr;
		goto CleanUp;
	}
	
	LOCK_DATA(tagH);
	tagTbl = (UINT16 *)DATA_2_PTR(tagH) + 58;		/* points to PCS */
	deviceChannelCount = ((icNamedColor2Type *)DATA_2_PTR(tagH))->ncolor.nDeviceCoords;
	countNamesInProfile = ((icNamedColor2Type *)DATA_2_PTR(tagH))->ncolor.count;
	if (deviceChannelCount==3) {
		elemSz = 32+(3+3)*sizeof(UINT16);
	} else if (deviceChannelCount==4) {
		elemSz = 32+(3+4)*sizeof(UINT16);
	} else if (deviceChannelCount==0) {
		elemSz = 32+(3+0)*sizeof(UINT16);
	} else {
		err = cmparamErr;
		goto CleanUp;
	}
	elemSz/=2;
	
	/* ...and convert the data from index to PCS values */
	for( i=processedLinesIn; i<processedLinesIn+lineCount; i++ ){
		imgIn = (UINT32 *)(InBuffer + i * inRowBytes);
		imgOut = (UINT8*)OutBuffer + (i-processedLinesIn) * outRowBytes;
		if( outputSize == 24 ){
			for (j=0;j<inWidth;j++) {
				index = *imgIn++;
				if( index == 0 || index > countNamesInProfile ){
					err = cmNamedColorNotFound;
					goto CleanUp;
				}

				ColorPtr = tagTbl + (index-1)*elemSz;
				*imgOut++  = (*ColorPtr++)>>8;
				*imgOut++  = (*ColorPtr++)>>8;
				*imgOut++  = (*ColorPtr++)>>8;
			}
		}
		else if( outputSize == 32 ){
			for (j=0;j<inWidth;j++) {
				index = *imgIn++;
				if( index == 0 || index > countNamesInProfile ){
					err = cmNamedColorNotFound;
					goto CleanUp;
				}

				ColorPtr = tagTbl + (index-1)*elemSz;
				*imgOut++  = (*ColorPtr++)>>8;
				*imgOut++  = (*ColorPtr++)>>8;
				*imgOut    = (*ColorPtr++)>>8;
				imgOut += 2;
			}
		}
	}
CleanUp:
	UNLOCK_DATA(tagH);
	LH_END_PROC("ConvertIndexToLabBitmap")
	return err;
}

#include <string.h>
CMError CMConvNameToIndex( icNamedColor2Type *Data, pcCMColorName Ptr2Name, unsigned long *Arr2Indices, unsigned long count );
CMError CMConvNameToIndex( icNamedColor2Type *Data, pcCMColorName Ptr2Name, unsigned long *Arr2Indices, unsigned long count )
{	
	OSErr		err = noErr;
	UINT32		i,index,l,len;
	char		*colorPtr=NULL;
	long 		elemSz,deviceChannelCount,countNamesInProfile;
	const char	*aPtr;
	Ptr			tagTbl;

	LH_START_PROC("CMConvNameToIndex")
	
	if( Data == 0 || count == 0 )return cmparamErr;
		
	tagTbl = &Data->ncolor.data[0];
	/* find out how many bytes to skip per element. div'ed 2 for indexing purpose */
	countNamesInProfile = Data->ncolor.count;
	deviceChannelCount = Data->ncolor.nDeviceCoords;
	if (deviceChannelCount==3){
		elemSz = 32+(3+3)*sizeof(SINT16);
	}
	else if (deviceChannelCount == 4){
		elemSz = 32+(3+4)*sizeof(SINT16);
	}
	else if (deviceChannelCount == 0){
		elemSz = 32+(3+0)*sizeof(SINT16);
	}
	else{
		err = cmparamErr;
		goto CleanUp;
	}
	
	for (i=0; i<(UINT32)count; i++){
		colorPtr = tagTbl;
		len = strlen( Ptr2Name[i] );
		aPtr = Ptr2Name[i];
		for( index = 0; index < (UINT32)countNamesInProfile; index++ ){
			for( l = 0; l<len; l++ ){
				if( colorPtr[l] != aPtr[l] )break;
				if( colorPtr[l] == 0 )break;
			}
			if( l >= len )break;
			colorPtr += elemSz;
		}
		if( l < len )index = 0;	/* not found */
		else index++;			/* Indices are 1 based */

		/* put in all the info */
		if( index == 0 ){
			err = cmNamedColorNotFound;
			goto CleanUp;
		}
		Arr2Indices[i] = index;
	}
CleanUp:
	LH_END_PROC("CMConvNameToIndex")
	return err;
}

CMError CMConvNameToIndexCW( CMWorldRef *Storage, pcCMColorName Ptr2Name, unsigned long *Arr2Indices, unsigned long count);
CMError CMConvNameToIndexCW( CMWorldRef *Storage, pcCMColorName Ptr2Name, unsigned long *Arr2Indices, unsigned long count)
{	
	Ptr					tagTbl = NULL;
	LUT_DATA_TYPE		tagH = NULL;
	CMError				err = noErr;
	char				*colorPtr=NULL;
	CMMModelPtr 		modelingData;
	icNamedColor2Type 	*namedData;

	LH_START_PROC("CMConvNameToIndexCW")
	
	if( Storage == 0 )return cmparamErr;
	LOCK_DATA( Storage );
	modelingData = (CMMModelPtr)(DATA_2_PTR(	Storage ));
	if ( modelingData->hasNamedColorProf!=NoNamedColorProfile) 
	{
		/* prepare the tag table */
		tagH = modelingData->theNamedColorTagData;
		if (tagH==NULL){
			err = cmparamErr;
			goto CleanUp;
		}
		LOCK_DATA(tagH);
		
		namedData = (icNamedColor2Type *)DATA_2_PTR(tagH);
		err = CMConvNameToIndex( namedData, Ptr2Name, Arr2Indices, count );
		UNLOCK_DATA(tagH);
	}
CleanUp:
	UNLOCK_DATA( Storage );
	LH_END_PROC("CMConvNameToIndexCW")
	return err;
}
CMError CMConvNameToIndexProfile( CMProfileRef aProf, pcCMColorName Ptr2Name, unsigned long *Arr2Indices, unsigned long count);
CMError CMConvNameToIndexProfile( CMProfileRef aProf, pcCMColorName Ptr2Name, unsigned long *Arr2Indices, unsigned long count)
{
	CMError				err;
	short				aOSerr;
	icNamedColor2Type	*aName = 0;
	unsigned long		byteCount;

	LH_START_PROC("CMConvNameToIndexProfile")
	
	if( aProf == 0 )return cmparamErr;
	err = CMGetPartialProfileElement(	aProf,
										icSigNamedColor2Tag,
										0,
										&byteCount,
										0 );
	if (err)
		goto CleanupAndExit;
    aName = (icNamedColor2Type *)SmartNewPtr( byteCount, &aOSerr );
	err = aOSerr;
	if (err)
		goto CleanupAndExit;
	err = CMGetPartialProfileElement(	aProf,
										icSigNamedColor2Tag,
										0,
										&byteCount,
										(Ptr)aName );
	if (err){
		goto CleanupAndExit;
	}

#ifdef IntelMode
	SwapLongOffset( &aName->ncolor.count, 0, 4 );
	SwapLongOffset( &aName->ncolor.nDeviceCoords, 0, 4 );
#endif
	err = CMConvNameToIndex( aName, Ptr2Name, Arr2Indices, count );

CleanupAndExit:
	DisposeIfPtr( (Ptr)aName );
	LH_END_PROC("CMConvNameToIndexProfile")
	return err;
}

CMError CMConvIndexToName( icNamedColor2Type *Data, unsigned long *Arr2Indices, pCMColorName Ptr2Name, unsigned long count);
CMError CMConvIndexToName( icNamedColor2Type *Data, unsigned long *Arr2Indices, pCMColorName Ptr2Name, unsigned long count)
{	
	Ptr				tagTbl = NULL;
	OSErr			err = noErr;
	UINT32			i,index;
	char			*colorPtr=NULL;
	long 			elemSz,deviceChannelCount,countNamesInProfile;

	LH_START_PROC("CMConvIndexToName")
	
	if( Data == 0 || count == 0 )return cmparamErr;
	tagTbl = &Data->ncolor.data[0];
		
	/* find out how many bytes to skip per element. div'ed 2 for indexing purpose */
	countNamesInProfile = Data->ncolor.count;
	deviceChannelCount = Data->ncolor.nDeviceCoords;
	if (deviceChannelCount==3){
		elemSz = 32+(3+3)*sizeof(SINT16);
	}
	else if (deviceChannelCount == 4){
		elemSz = 32+(3+4)*sizeof(SINT16);
	} 
	else if (deviceChannelCount == 0){
		elemSz = 32+(3+0)*sizeof(SINT16);
	}
	else{
		err = cmparamErr;
		goto CleanUp;
	}
	
	/* doing the actual */
	for (i=0; i<(UINT32)count; i++){
		/* go to the index'th element*/
		index = Arr2Indices[i];
		if (index > (UINT32)countNamesInProfile || index == 0 ) 
		{
			err = cmNamedColorNotFound;
			goto CleanUp;
		}
		colorPtr = tagTbl + (index-1)*elemSz;
		/* put in all the info */
		strcpy( Ptr2Name[i], colorPtr );
	}

CleanUp:
	LH_END_PROC("CMConvIndexToName")
	return err;
}

CMError CMConvIndexToNameCW( CMWorldRef *Storage, unsigned long *Arr2Indices, pCMColorName Ptr2Name, unsigned long count);
CMError CMConvIndexToNameCW( CMWorldRef *Storage, unsigned long *Arr2Indices, pCMColorName Ptr2Name, unsigned long count)
{	
	Ptr					tagTbl = NULL;
	LUT_DATA_TYPE		tagH = NULL;
	CMError				err = noErr;
	char				*colorPtr=NULL;
	CMMModelPtr 		modelingData;
	icNamedColor2Type 	*namedData;

	LH_START_PROC("CMConvIndexToNameCW")
	
	if( Storage == 0 )return cmparamErr;
	LOCK_DATA( Storage );
	modelingData = (CMMModelPtr)(DATA_2_PTR(	Storage ));
	if ( modelingData->hasNamedColorProf!=NoNamedColorProfile) 
	{
		/* prepare the tag table */
		tagH = modelingData->theNamedColorTagData ;
		if (tagH==NULL){
			err = cmparamErr;
			goto CleanUp;
		}
		LOCK_DATA(tagH);
		
		namedData = (icNamedColor2Type *)DATA_2_PTR(tagH);
		err = CMConvIndexToName( namedData, Arr2Indices, Ptr2Name, count );
		UNLOCK_DATA(tagH);
	}
CleanUp:
	UNLOCK_DATA( Storage );
	LH_END_PROC("CMConvIndexToNameCW")
	return err;
}
CMError CMConvIndexToNameProfile( CMProfileRef aProf, unsigned long *Arr2Indices, pCMColorName Ptr2Name, unsigned long count);
CMError CMConvIndexToNameProfile( CMProfileRef aProf, unsigned long *Arr2Indices, pCMColorName Ptr2Name, unsigned long count)
{
	CMError				err;
	short				aOSerr;
	icNamedColor2Type	*aName = 0;
	unsigned long		byteCount;

	LH_START_PROC("CMConvIndexToNameProfile")
	
	if( aProf == 0 )return cmparamErr;
	err = CMGetPartialProfileElement(	aProf,
										icSigNamedColor2Tag,
										0,
										&byteCount,
										0 );
	if (err)
		goto CleanupAndExit;
    aName = (icNamedColor2Type *)SmartNewPtr( byteCount, &aOSerr );
	err = aOSerr;
	if (err)
		goto CleanupAndExit;
	err = CMGetPartialProfileElement(	aProf,
										icSigNamedColor2Tag,
										0,
										&byteCount,
										(Ptr)aName );
	if (err){
		goto CleanupAndExit;
	}

#ifdef IntelMode
	SwapLongOffset( &aName->ncolor.count, 0, 4 );
	SwapLongOffset( &aName->ncolor.nDeviceCoords, 0, 4 );
#endif
	err = CMConvIndexToName( aName, Arr2Indices, Ptr2Name, count );

CleanupAndExit:
	DisposeIfPtr( (Ptr)aName );
	LH_END_PROC("CMConvIndexToNameProfile")
	return err;
}

CMError CMGetNamedProfileInfoProfile( CMProfileRef aProf, pCMNamedProfileInfo Info );
CMError CMGetNamedProfileInfoProfile( CMProfileRef aProf, pCMNamedProfileInfo Info )
{	
	CMError				err = noErr;
	Ptr					tagTbl;
	short				aOSerr;
	icNamedColor2Type	*aName = 0;
	unsigned long		byteCount;

	LH_START_PROC("CMGetNamedProfileInfoProfile")
	
	if( aProf == 0 )return cmparamErr;
	err = CMGetPartialProfileElement(	aProf,
										icSigNamedColor2Tag,
										0,
										&byteCount,
										0 );
	if (err)
		goto CleanupAndExit;
    aName = (icNamedColor2Type *)SmartNewPtr( byteCount, &aOSerr );
	err = aOSerr;
	if (err)
		goto CleanupAndExit;
	err = CMGetPartialProfileElement(	aProf,
										icSigNamedColor2Tag,
										0,
										&byteCount,
										(Ptr)aName );
	if (err){
		goto CleanupAndExit;
	}

	
	/* = CMNamedColor2Type_header(84) + firstName(32) + PCSSize(3*2) */
	tagTbl = (Ptr)aName + 8;
	
#ifdef IntelMode
	SwapLongOffset( &aName->ncolor.count, 0, 4 );
	SwapLongOffset( &aName->ncolor.nDeviceCoords, 0, 4 );
	SwapLongOffset( &aName->ncolor.vendorFlag, 0, 4 );
#endif
	memcpy( Info, tagTbl, sizeof( CMNamedProfileInfo ));
CleanupAndExit:

  /* Clean up allocated storage */
	DisposeIfPtr((Ptr) aName);
  LH_END_PROC("CMGetNamedProfileInfoProfile")
	return err;
}
CMError CMGetNamedProfileInfoCW( CMWorldRef *Storage, pCMNamedProfileInfo Info );
CMError CMGetNamedProfileInfoCW( CMWorldRef *Storage, pCMNamedProfileInfo Info )
{	
	LUT_DATA_TYPE	tagH = NULL;
	OSErr			err = noErr;
	CMMModelPtr 	modelingData;
	Ptr				tagTbl;

	LH_START_PROC("CMGetNamedProfileInfoCW")
	
	if( Storage == 0 )return cmparamErr;
	LOCK_DATA( Storage );
	modelingData = (CMMModelPtr)(DATA_2_PTR(	Storage ));
	if( modelingData->hasNamedColorProf != NoNamedColorProfile ){
		/* prepare the tag table */
		tagH = modelingData->theNamedColorTagData;
		if (tagH==NULL){
			err = cmparamErr;
			goto CleanUp;
		}
		LOCK_DATA(tagH);
		
		/* tagTbl should now point to beginning of first device data */
		/* = CMNamedColor2Type_header(84) + firstName(32) + PCSSize(3*2) */
		tagTbl = (Ptr)DATA_2_PTR(tagH) + 8;
		
		memcpy( Info, tagTbl, sizeof( CMNamedProfileInfo ));
		UNLOCK_DATA(tagH);
	}
CleanUp:
	UNLOCK_DATA( Storage );
	LH_END_PROC("CMGetNamedProfileInfoCW")
	return err;
}
