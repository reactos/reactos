/*
	File:		LHTypeDefs.h

	Contains:	

	Written by:	U. J. Krabbenhoeft

	Version:

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

*/

#ifndef LHTypeDefs_h
#define LHTypeDefs_h

/* flags to keep track if there is a named profile in the profile sequenc */
enum 
{
	NoNamedColorProfile 		= 0x0000,
	NamedColorProfileOnly		= 0x0001,
	NamedColorProfileAtBegin	= 0x0002,
	NamedColorProfileAtEnd		= 0x0003
};



/* ------------------------------------------------------------------------------------------------------------
	CMLutParam - lut struct - set in the Initphase and used by the Match/Check routines 
   ------------------------------------------------------------------------------------------------------------ */
typedef struct CMLutParam
{
	long 				inputLutEntryCount;		/* count of entries for input lut for one dimension */
	long 				inputLutWordSize;		/* count of bits of each entry ( e.g. 16 for UINT16 ) */
	LUT_DATA_TYPE		inputLut;				/* pointer/handle to input lut */
	long 				outputLutEntryCount;	/* count of entries for output lut for one dimension	 */
	long 				outputLutWordSize;		/* count of bits of each entry ( e.g. 8 for UINT8 ) */
	LUT_DATA_TYPE		outputLut;				/* pointer/handle to output lut */
	long 				colorLutInDim;			/* input dimension  ( e.g. 3 for LAB ; 4 for CMYK ) */
	long 				colorLutOutDim;			/* output dimension ( e.g. 3 for LAB ; 4 for CMYK ) */
	long 				colorLutGridPoints;		/* count of gridpoints for color lut ( for one Dimension ) */
	long 				colorLutWordSize;		/* count of bits of each entry ( e.g. 8 for UINT8 ) */
	LUT_DATA_TYPE		colorLut;				/* pointer/handle to color lut */

	/* --------- used in Init-Phase by CreateCombi ------------ */
	Ptr 				matrixTRC;
	Ptr 				matrixMFT;

	/* ---------- used for DoNDim - DoNDimTableData ------------*/
	unsigned long		cmInputColorSpace;		/* packing info for input		*/
	unsigned long		cmOutputColorSpace;		/* packing info for output		*/
	void*				userData;
} CMLutParam, *CMLutParamPtr, **CMLutParamHdl;


/* ------------------------------------------------------------------------------------------------------------
	CMCalcParam - calc struct for the Match/Check routines 
   ------------------------------------------------------------------------------------------------------------ */

typedef struct CMCalcParam
{
	CMBitmapColorSpace	cmInputColorSpace;		/* input color space */
	CMBitmapColorSpace	cmOutputColorSpace;		/* output color space */
	long				cmPixelPerLine;			/* pixel per line */
	long				cmLineCount;			/* number of lines */
	long				cmInputBytesPerLine;	/* bytes per line */
	long				cmOutputBytesPerLine;	/* bytes per line */
	long				cmInputPixelOffset;		/* offset to next input pixel */
	long				cmOutputPixelOffset;	/* offset to next output pixel */
	void*				inputData[8];			/* 8 pointers to input data */
	void*				outputData[8];			/* 8 pointers to output data */
	Boolean				copyAlpha;				/* true -> copy alpha */
	Boolean				clearMask;				/* true -> set to zero  false-> copy alpha (if any) */
} CMCalcParam, *CMCalcParamPtr, **CMCalcParamHdl;



/* ------------------------------------------------------------------------------------------------------------
	DoNDimCalcData - calc data for DoNDim 
   ------------------------------------------------------------------------------------------------------------ */
struct DoNDimCalcData
{ 	
	long 	pixelCount;			/* count of input pixels	*/
	Ptr		inputData;			/* input array				*/
	Ptr		outputData;			/* output array				*/
};
typedef struct DoNDimCalcData DoNDimCalcData, *DoNDimCalcDataPtr, **DoNDimCalcDataHdl;

/* ------------------------------------------------------------------------------------------------------------
	LHCombiData - struct used in initphase while creating the combi-luts 

   ------------------------------------------------------------------------------------------------------------ */
typedef struct LHCombiData
{
	CMProfileRef	theProfile;
	OSType			profileClass;
	OSType			dataColorSpace;
	OSType			profileConnectionSpace;
	long			gridPointsCube;
	long			renderingIntent;
	long			precision;
	long			maxProfileCount;
	long			profLoop;
	Boolean			doCreate_16bit_ELut;
	Boolean			doCreate_16bit_XLut;
	Boolean			doCreate_16bit_ALut;
	Boolean			doCreateLinkProfile;
	Boolean			doCreate_16bit_Combi;
	Boolean			doCreateGamutLut;
	Boolean			amIPCS;
	Boolean			usePreviewTag;
} LHCombiData, *LHCombiDataPtr, **LHCombiDataHdl;
#if powerc
#pragma options align=reset
#endif

#if powerc
#pragma options align=mac68k
#endif
/* ------------------------------------------------------------------------------------------------------------
	LHProfile - internal information for one profile 
   ------------------------------------------------------------------------------------------------------------ */
typedef struct LHProfile
{
	CMProfileRef	profileSet;
	short			pcsConversionMode;
	short			usePreviewTag;
	unsigned long	renderingIntent;
} LHProfile;

/* ------------------------------------------------------------------------------------------------------------
	LHConcatProfileSet - internal information for a set of profiles 
   ------------------------------------------------------------------------------------------------------------ */
typedef struct LHConcatProfileSet 
{
	unsigned short			keyIndex;				/* Zero-based							*/
	unsigned short			count;					/* Min 1								*/
	LHProfile				prof[1];				/* Variable. Ordered from Source -> Dest*/
} LHConcatProfileSet;
#if powerc
#pragma options align=reset
#endif


/* ------------------------------------------------------------------------------------------------------------
	CMMModelData - global CMM data 
   ------------------------------------------------------------------------------------------------------------ */
/*
#if powerc
#pragma options align=mac68k
#endif
*/
struct CMMModelData 
{
	CMLutParam				lutParam;
	CMLutParam				gamutLutParam;

	short					precision;
	Boolean					lookup;					/* false -> interpolation,  true -> lookup only */
		
	OSType					firstColorSpace;
	OSType					lastColorSpace;
	
	long					currentCall;
	long					lastCall;

	long					srcProfileVersion;
	long					dstProfileVersion;
	Handle					Monet;
	
	/* for NamedColor matching */
	long				hasNamedColorProf;
	Handle				thePCSProfHandle;
	LUT_DATA_TYPE		theNamedColorTagData;

	/*OSType				dataColorSpace;*/
	/*OSType				profileConnectionSpace;*/

	UINT32				*aIntentArr;
	UINT32				nIntents;
	UINT32				dwFlags;
	Boolean				appendDeviceLink;					/* if count > 1 && last profile is deviceLink */
	CMWorldRef			pBackwardTransform;
#if	__IS_MAC
	ComponentInstance	accelerationComponent;
#endif
};
typedef struct CMMModelData CMMModelData, *CMMModelPtr, **CMMModelHandle;

/*
#if powerc
#pragma options align=reset
#endif
*/


typedef double Matrix2D[3][3];

#endif
