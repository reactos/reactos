/*
	File:		LHGenLuts.c

	Contains:	

	Version:	

	Written by:	S. Bleker & W. Neubrand & U.Krabbenhoeft

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

*/

#ifndef LHGeneralIncs_h
#include "General.h"
#endif

#ifndef LHTheRoutines_h
#include "Routines.h"
#endif

#ifndef LHFragment_h
#include "Fragment.h"
#endif

#ifndef LHCalcNDim_h
#include "CalcNDim.h"
#endif

#ifndef LHDoNDim_h
#include "DoNDim.h"
#endif


#if ! realThing
#ifdef DEBUG_OUTPUT
#define kThisFile kLHGenLutsID
#define DoDisplayLutNew DoDisplayLut
#endif
#endif

#ifndef LHStdConversionLuts_h
#include "StdConv.h"
#endif

#define ALLOW_DEVICE_LINK   /* allows link as the last profile in a chain, change in PI_CMM.c too */
/*••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••
	debugging only:
      - define  WRITE_LUTS to write out all luts that will be generated....
  ••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••*/
/*#define WRITE_LUTS */
#ifdef WRITE_LUTS
void
WriteLut2File  ( Str255			theTitle,
				 LUT_DATA_TYPE	theLut,
				 OSType  		theType );
/*———————————————————————————————————————————————————————————————————————————————————————————
	write a lut to a file...
  ———————————————————————————————————————————————————————————————————————————————————————————*/
void
WriteLut2File  ( Str255			theTitle,
				 LUT_DATA_TYPE	theLut,
				 OSType  		theType )
{
	FSSpec  theFSSpec;
	SINT16	refNum;
	SINT32	theCount;
	
	if (theLut)
	{
		FSMakeFSSpec(0, 0, theTitle, &theFSSpec);
		FSpDelete(&theFSSpec);
		FSpCreate(&theFSSpec, 'Fill', theType, 0L);
		FSpOpenDF(&theFSSpec,fsWrPerm,&refNum);
		theCount = GETDATASIZE(theLut);
		FSWrite(refNum, &theCount, DATA_2_PTR(theLut));
		FSClose(refNum);
	}
}
#endif


/*••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••
	debugging only
  ••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••*/
/* #define WRITE_STRING */
#ifdef WRITE_STRING
#include "stdio.h"
void
WriteString2File  ( Str255	theFile,
					Str255	theString );
/*———————————————————————————————————————————————————————————————————————————————————————————
	write a string to a file...
  ———————————————————————————————————————————————————————————————————————————————————————————*/
void
WriteString2File  ( Str255	theFile,
					Str255	theString )
{
	FSSpec  theFSSpec;
	SINT16	refNum;
	SINT32	theCount;
	SINT32	curEOF;
	OSErr	err;
	
	FSMakeFSSpec(-LMGetSFSaveDisk(), LMGetCurDirStore(), theFile, &theFSSpec);
	err = FSpOpenDF(&theFSSpec,fsWrPerm,&refNum);
	if (err == fnfErr)
	{
		FSpCreate(&theFSSpec, 'MPS ', 'TEXT', 0L);
		FSpOpenDF(&theFSSpec,fsWrPerm,&refNum);
	}
	GetEOF(refNum,&curEOF);
	SetFPos(refNum,fsFromStart,curEOF);
	theCount = theString[0];
	FSWrite(refNum, &theCount, &theString[1]);
	FSClose(refNum);
}

#endif

/* ———————————————————————————————————————————————————————————————————————————————————————————

1.		InvertLut1d( icCurveType *Lut )
	Params:
		Lut	(in/out)		Reference to Lut.
	Abstract:
		Given a Ptr to a Lut of type icCurveType this function calculates the invers Lut
		and overwrites the Entry-Lut.

			
2.		CombiMatrix( icXYZType *srcColorantData[3],
					 icXYZType *destColorantData[3] )
	Abstract:
		Given colorant data for source and destination matrix this function calculates the
		invers matrix of the destination and after it the multiplicated matrix of both.
	Params:
		*srcColorantData[3]	(in)			Reference to source matrix
		*destColorantData[3](in/out)		Reference to destination matrix
		
	Return:
		noErr					successful
   ——————————————————————————————————————————————————————————————————————————————————————————— */
	
#define CLIPPByte(x,a,b) ((x)<(a)?(UINT8)(a):((x)>(b)?(UINT8)(b):(UINT8)(x+.5)))
#define CLIPPWord(x,a,b) ((x)<(a)?(UINT16)(a):((x)>(b)?(UINT16)(b):(UINT16)(x+.5)))

#define VAL_USED_BITS 16
#define VAL_MAX (1<<VAL_USED_BITS)
#define VAL_MAXM1 (VAL_MAX-1)
/* ———————————————————————————————————————————————————————————————————————————————————————————
void
CreateLinearElut16 (	Ptr		theElut,
				   		SINT32	theSize,
				   		SINT32	gridPoints,
				   		SINT32	gridPointsCube)
   ——————————————————————————————————————————————————————————————————————————————————————————— */
void
CreateLinearElut16 (	Ptr		theElut,
				   		SINT32	theSize,
				   		SINT32	gridPoints,
				   		SINT32	gridPointsCube)
{
	UINT16*			wordPtr;
	register UINT32 aVal;
	SINT16			loop;
	register UINT32 aFac;
	register UINT32 aDiv;
	register UINT32 aRound;
#ifdef DEBUG_OUTPUT
	CMError err = noErr;
#endif
	
	LH_START_PROC("CreateLinearElut16")
	if ( gridPointsCube )
	{
		if( gridPoints ){
			aFac = (UINT32)((gridPoints-1) * (256*128. * gridPointsCube) + (gridPointsCube-1) / 2) ;
			aFac = aFac / (gridPointsCube-1);
			aDiv = (UINT32)((gridPoints * (theSize-1) + .9999999 )/ 2.);
		}
		else{
			aFac = (UINT32)( (256*128. * gridPointsCube) + (gridPointsCube-1) / 2) ;
			aFac = aFac / (gridPointsCube-1);
			aDiv = (UINT32)(( (theSize-1) + .9999999 )/ 2.);
		}
	}
	else
	{
		if( gridPoints ){
			aFac = (gridPoints-1) * 65536;
			aDiv = gridPoints * (theSize-1);
		}
		else{
			aFac = 65535 ;
			aDiv = (theSize-1) ;
		}
	}
	aRound = aDiv/2-1;
	wordPtr = (UINT16*)theElut;
	for (loop =0; loop< theSize; loop++)
	{
		aVal = (loop * aFac + aRound ) / aDiv;
		if ( aVal > VAL_MAXM1 )
			aVal = VAL_MAXM1;
		wordPtr[loop] = (UINT16)aVal;
	}
	LH_END_PROC("CreateLinearElut16")
}

/* ———————————————————————————————————————————————————————————————————————————————————————————
	void
	CreateLinearElut ( Ptr		theElut,
					   SINT32		theSize,
					   SINT32		gridPoints,
					   SINT32		gridPointsCube)
   ——————————————————————————————————————————————————————————————————————————————————————————— */
void
CreateLinearElut ( Ptr		theElut,
				   SINT32		theSize,
				   SINT32		gridPoints,
				   SINT32		gridPointsCube)
{
	UINT16*	wordPtr;
	register UINT32 aVal;
	SINT32	loop;
	register UINT32 aFac;
	register UINT32 aDiv;
	register UINT32 aRound;
	register UINT32 aMax;
#ifdef DEBUG_OUTPUT
	CMError err = noErr;
#endif

	LH_START_PROC("CreateLinearElut")
	if ( gridPointsCube )
	{
		if( gridPoints ){
			aMax = 1024 * (gridPoints-1) / gridPoints;
			aFac = (UINT32)((gridPoints-1) * (1024 * gridPointsCube) + (gridPointsCube-1) / 2) ;
			aFac = aFac / (gridPointsCube-1);
			aDiv = (UINT32)((gridPoints * (theSize-1) + .9999999 ));
		}
		else{
			aFac = (UINT32)( (1024 * gridPointsCube) + (gridPointsCube-1) / 2) ;
			aFac = aFac / (gridPointsCube-1);
			aDiv = (UINT32)(( (theSize-1) + .9999999 ));
			aMax = 1023;
		}
	}
	else
	{
		if( gridPoints ){
			aMax = 1024 * (gridPoints-1) / gridPoints;
			aFac = (gridPoints-1) * 1024/2;
			aDiv = gridPoints * (theSize-1)/2;
		}
		else{
			aMax = 1023;
			aFac = aMax;
			aDiv = (theSize-1) ;
		}
	}
	aRound = aDiv/2-1;
	wordPtr = (UINT16*)theElut;
	for (loop =0; loop< theSize; loop++)
	{
		aVal = (loop * aFac + aRound ) / aDiv;
		if ( aVal > aMax )
			aVal = aMax;
		wordPtr[loop] = (UINT16)aVal;
	}
	LH_END_PROC("CreateLinearElut")
}

/* ———————————————————————————————————————————————————————————————————————————————————————————
	void
	CreateLinearAlut16 ( 	UINT16*	theAlut,
				    		SINT32 	aCount )
   ——————————————————————————————————————————————————————————————————————————————————————————— */
void
CreateLinearAlut16 ( 	UINT16*	theAlut,
				    	SINT32 	aCount )
{
	SINT32 	count = aCount;
	SINT32 	loop;
	SINT32 	aFac = 4096*VAL_MAXM1/(count-1);
	SINT32 	aRound = 2047;
	SINT32	aVal;
#ifdef DEBUG_OUTPUT
	CMError err = noErr;
#endif
		
	LH_START_PROC("CreateLinearAlut16")
	for ( loop = 0; loop < count; loop++)
	{
		aVal = ( loop  * aFac + aRound ) >> 12;
		if ( aVal > VAL_MAXM1 )
			aVal = VAL_MAXM1;
		theAlut[loop] = (UINT16)aVal;
	}
	LH_END_PROC("CreateLinearAlut16")
}

/* ———————————————————————————————————————————————————————————————————————————————————————————
	void
	CreateLinearAlut ( UINT8*	theAlut,
					   SINT32	count )
   ——————————————————————————————————————————————————————————————————————————————————————————— */
void
CreateLinearAlut ( UINT8*	theAlut,
				   SINT32	count )
{
	SINT32 	adr_Bits;
	SINT32 	loop,i;
	SINT32	shift;
	SINT32	aRound;
	SINT32	aVal;
#ifdef DEBUG_OUTPUT
	CMError err = noErr;
#endif
	
	LH_START_PROC("CreateLinearAlut")
	for( i=1; i<100; ++i)
		if ( (1<<i) == count )
			break;  /* calculate gridpoints */
	if ( i<= 0 || i >= 100 )
		return;
	adr_Bits = i;
	shift = adr_Bits - 8;
	if ( shift > 0 )
		aRound = (1<<(shift-1));
	else
		aRound = 0;
	
	for ( loop = 0; loop < count; loop++)
	{
		aVal = ( loop + aRound ) >> shift;
		if ( aVal > 255 )
			aVal = 255;
		theAlut[loop] = (UINT8)aVal;
	}
	LH_END_PROC("CreateLinearAlut")
}

/* ———————————————————————————————————————————————————————————————————————————————————————————
void
SetMem16  ( void *wordPtr,
			UINT32 numBytes,
			UINT16 wordValue)
   ——————————————————————————————————————————————————————————————————————————————————————————— */
void
SetMem16  ( void *wordPtr,
			UINT32 numBytes,
			UINT16 wordValue);
void
SetMem16  ( void *wordPtr,
			UINT32 numBytes,
			UINT16 wordValue)
{
	register SINT32 i;
	register SINT32 count = numBytes;
	register UINT16 *ptr = (UINT16 *)wordPtr;
	register UINT16 value = wordValue;
#ifdef DEBUG_OUTPUT
	CMError err = noErr;
#endif

	LH_START_PROC("SetMem16")
	for (i = 0; i < count; ++i)
		*ptr++ = value;
	LH_END_PROC("SetMem16")
}

/* ———————————————————————————————————————————————————————————————————————————————————————————
CMError
DoMakeGamutForMonitor	( CMLutParamPtr		theLutData,
						  LHCombiDataPtr	theCombiData)
   ——————————————————————————————————————————————————————————————————————————————————————————— */
CMError
DoMakeGamutForMonitor	( CMLutParamPtr		theLutData,
						  LHCombiDataPtr	theCombiData)
{
	OSType			colorantTags[3];
	icXYZType 		colorantData[3];
	SINT16			loop;
	CMError			err;
	UINT32	elementSize;
	
	LH_START_PROC("DoMakeGamutForMonitor")
	
	colorantTags[0] = icSigRedColorantTag;
	colorantTags[1] = icSigGreenColorantTag;
	colorantTags[2] = icSigBlueColorantTag;
	
	/* --------------------------------------------------------------------------------- */
	for (loop = 0; loop < 3; loop++)
	{
		err = CMGetProfileElement(theCombiData->theProfile, colorantTags[loop], &elementSize, nil);
		if (err != noErr)
			goto CleanupAndExit;
		err = CMGetProfileElement(theCombiData->theProfile, colorantTags[loop], &elementSize, &colorantData[loop]);
#ifdef IntelMode
        SwapLongOffset( &colorantData[loop].base.sig, 0, 4 );
        SwapLongOffset( &colorantData[loop], (ULONG)((char*)&colorantData[0].data.data[0]-(char*)&colorantData[0]), elementSize );
#endif
		if (err != noErr)
			goto CleanupAndExit;
	}
	
	if ( theCombiData->precision == cmBestMode )
	{
		err = MakeGamut16or32ForMonitor( &colorantData[0], &colorantData[1], &colorantData[2], theLutData, TRUE );
		theLutData->colorLutGridPoints  = 32;
	}
	else
	{
		err = MakeGamut16or32ForMonitor( &colorantData[0], &colorantData[1], &colorantData[2], theLutData, FALSE);
		theLutData->colorLutGridPoints  = 16;
	}
	theLutData->colorLutInDim	= 3;
	theLutData->colorLutOutDim	= 1;
	theLutData->inputLutEntryCount = (1<<adr_breite_elut);
	theLutData->inputLutWordSize = VAL_USED_BITS;
	theLutData->outputLutEntryCount = adr_bereich_alut;
	theLutData->outputLutWordSize = bit_breite_alut;
	theLutData->colorLutWordSize = 8;

#ifdef DEBUG_OUTPUT
	if ( DebugLutCheck( kDisplayGamut ) )
	{
		LOCK_DATA(theLutData->colorLut);
		if (theCombiData->precision == cmBestMode)
			Show32by32by32GamutXLUT(DATA_2_PTR(theLutData->colorLut));
		else
			Show16by16by16GamutXLUT(DATA_2_PTR(theLutData->colorLut));
		UNLOCK_DATA(theLutData->colorLut);
	}
#endif
	
CleanupAndExit:

	LH_END_PROC("DoMakeGamutForMonitor")
	return err;
}

/* ———————————————————————————————————————————————————————————————————————————————————————————
	CMError
	GetMatrixFromProfile	( CMProfileRef 	theProfile,
							  Ptr*			theMatrix,
							  OSType		theTag,
							  double		factor )
   ——————————————————————————————————————————————————————————————————————————————————————————— */
CMError
GetMatrixFromProfile	( CMLutParamPtr		theLutData,
						  LHCombiDataPtr	theCombiData,
						  OSType			theTag,
						  double			factor )
{
 	CMError		err = noErr;
  	OSErr		aOSerr = noErr;
	SINT32		i;
	SINT32		j;
	Matrix2D	localMatrix;
	long		matrix[3][3];
	UINT32 		byteCount;
	SINT32		offset;
	
	LH_START_PROC("GetMatrixFromProfile")
	
	/* -------------------------------------------------------- get tag data from profile */
	offset	  = 12;	/* matrix starts at byte 12 */
	byteCount = 9 * sizeof(Fixed);
	err = CMGetPartialProfileElement(theCombiData->theProfile, theTag, offset, &byteCount, &matrix);
#ifdef IntelMode
        SwapLongOffset( &matrix, 0, byteCount );
#endif
	if (err)
		goto CleanupAndExit;
	
	for (i = 0; i < 3; i++)
		for (j = 0; j < 3; j++)
			localMatrix[i][j] =  (double)( matrix[i][j] / 65536.0 * factor);
	
	if ( 0  || localMatrix[0][0] + localMatrix[1][1] + localMatrix[2][2] != 3.0 )		/* ignore ident */
	{
		theLutData->matrixMFT = SmartNewPtr(sizeof(Matrix2D), &aOSerr);
		err = aOSerr;
		if (err)
			goto CleanupAndExit;
		BlockMoveData(localMatrix, theLutData->matrixMFT, sizeof(Matrix2D));
	}
	else
		theLutData->matrixMFT = nil;
	/* ---------------------------------------------------------------------------------
	    clean up
	   ---------------------------------------------------------------------------------*/
CleanupAndExit:

	LH_END_PROC("GetMatrixFromProfile")
	return err;
}

/* ———————————————————————————————————————————————————————————————————————————————————————————
	CMError
	Extract_MFT_Elut	  ( CMLutParamPtr	theLutData,
							LHCombiDataPtr	theCombiData,
							Ptr				profileLutPtr,
							OSType			theTag,
							SINT32			theCubeSize )
   ——————————————————————————————————————————————————————————————————————————————————————————— */
CMError
Extract_MFT_Elut	  ( CMLutParamPtr	theLutData,
						LHCombiDataPtr	theCombiData,
						Ptr				profileLutPtr,
						OSType			theTag )
{
 	CMError		err = noErr;
  	OSErr		aOSerr = noErr;
	UINT32		byteCount;
	SINT32		offset;
	SINT32		i;
	SINT32		inputTableEntries;
	Ptr			theInputLuts	= nil;		/*•••12/1/95*/
	Boolean		readLutFromFile = TRUE;
	UINT16*		shortPtr;
	
	LH_START_PROC("Extract_MFT_Elut")
	
	/* ============================================================================================================================
																 mft1
	   ============================================================================================================================ */
	if ( *((OSType*)profileLutPtr) == icSigLut8Type )
	{
		/* --------------------------------------------------------------------------------- get inputLuts out of the profile */
		inputTableEntries	= 256;
		offset    			= 48;	/* input luts for mft1 start at byte 48 */
		byteCount			= theLutData->colorLutInDim * inputTableEntries;
		theInputLuts = SmartNewPtr(byteCount, &aOSerr);
		err = aOSerr;
		if (err)
			goto CleanupAndExit;
		err = CMGetPartialProfileElement(theCombiData->theProfile, theTag, offset, &byteCount, theInputLuts);
		if (err)
			goto CleanupAndExit;
		if ( theCombiData->doCreate_16bit_ELut )	/* this is NOT the first Elut -or- doCreateLinkProfile */
		{
																		/* if we create a LinkProfile the Elut should not be scaled... */
			if ((theCombiData->doCreateLinkProfile) && (theCombiData->profLoop == 0))
				err = Fill_ushort_ELUTs_from_lut8Tag( theLutData, theInputLuts, adr_breite_elut, bit_breite_elut, 0 );
			else 														/* else scale Elut to gridpoints in profile... */
				err = Fill_ushort_ELUTs_from_lut8Tag( theLutData, theInputLuts, adr_breite_elut, VAL_USED_BITS, theLutData->colorLutGridPoints );
			theLutData->inputLutEntryCount = (1<<adr_breite_elut);
			theLutData->inputLutWordSize = VAL_USED_BITS;
		}
		else															/* this is the first Elut */
		{
       	    if ( theCombiData->doCreate_16bit_Combi )	/* UWE 9.2.96 */
       	   	{
	            if (theCombiData->maxProfileCount == 0)					/* if we have only one profile, the scale Elut to gridpoints in profile...*/
	                err = Fill_ushort_ELUTs_from_lut8Tag( theLutData, theInputLuts, adr_breite_elut, VAL_USED_BITS, theLutData->colorLutGridPoints );
	            else													/* else scale Elut to the gridpoints in the cube...*/
              		err = Fill_ushort_ELUTs_from_lut8Tag( theLutData, theInputLuts, adr_breite_elut, VAL_USED_BITS, theCombiData->gridPointsCube );
           		theLutData->inputLutWordSize = VAL_USED_BITS;
      	    }
      	    else
      	    {
	            if (theCombiData->maxProfileCount == 0)					/* if we have only one profile, the scale Elut to gridpoints in profile...*/
	                err = Fill_ushort_ELUTs_from_lut8Tag( theLutData, theInputLuts, adr_breite_elut, bit_breite_elut, theLutData->colorLutGridPoints );
	            else													/* else scale Elut to the gridpoints in the cube...*/
	                err = Fill_ushort_ELUTs_from_lut8Tag( theLutData, theInputLuts, adr_breite_elut, bit_breite_elut, theCombiData->gridPointsCube );
	            theLutData->inputLutWordSize = bit_breite_elut;

            }
            theLutData->inputLutEntryCount = (1<<adr_breite_elut);
		}
	}
	else
	/* ============================================================================================================================
																 mft2
	   ============================================================================================================================ */
	{
		/* --------------------------------------------------------------------------------- get inputLuts out of the profile */
		inputTableEntries	= ((icLut16Type *)profileLutPtr)->lut.inputEnt;
		if (inputTableEntries <2)
		{
#if ! realThing
			readLutFromFile = FALSE;
			inputTableEntries = 2;
#else
			err = cmProfileError;
			goto CleanupAndExit;
#endif
		}
		if (readLutFromFile)
		{
			offset    			= 52;	/* input luts for mft2 start at byte 52 */
			byteCount			= theLutData->colorLutInDim * inputTableEntries * sizeof(UINT16);
			theInputLuts = SmartNewPtr(byteCount, &aOSerr);
			err = aOSerr;
			if (err)
				goto CleanupAndExit;
			err = CMGetPartialProfileElement(theCombiData->theProfile, theTag, offset, &byteCount, theInputLuts);
#ifdef IntelMode
            SwapShortOffset( theInputLuts, 0, byteCount );
#endif
			if (err)
				goto CleanupAndExit;
		} else
		{
			theInputLuts = SmartNewPtr(inputTableEntries * sizeof(SINT16) * theLutData->colorLutInDim, &aOSerr);
			err = aOSerr;
			if (err)
				goto CleanupAndExit;	/* •••12/1/95 */
			shortPtr = (UINT16*)theInputLuts;
			for (i = 0; i< (theLutData->colorLutInDim * inputTableEntries); i+=inputTableEntries)
			{
				shortPtr[i] 	= 0;
				shortPtr[i+1]	= 0xFFFF;
			}
		}
		if ( theCombiData->doCreate_16bit_ELut )						/* this is NOT the first Elut -or- doCreateLinkProfile */
		{
																		/* if we create a LinkProfile the Elut should not be scaled... */
			if ((theCombiData->doCreateLinkProfile) && (theCombiData->profLoop == 0))
				err = Fill_ushort_ELUTs_from_lut16Tag( theLutData, theInputLuts, adr_breite_elut, VAL_USED_BITS, 0, inputTableEntries );
			else 														/* else scale Elut to gridpoints in profile... */
				err = Fill_ushort_ELUTs_from_lut16Tag( theLutData, theInputLuts, adr_breite_elut, VAL_USED_BITS, theLutData->colorLutGridPoints, inputTableEntries );
			theLutData->inputLutEntryCount = (1<<adr_breite_elut);
			theLutData->inputLutWordSize = VAL_USED_BITS;
		}
		else															/* this is the first Elut */
		{
         	if ( theCombiData->doCreate_16bit_Combi ) 	/* UWE 9.2.96 */
         	{
	            if (theCombiData->maxProfileCount == 0)					/* if we have only one profile, the scale Elut to gridpoints in profile...*/
	                err = Fill_ushort_ELUTs_from_lut16Tag( theLutData, theInputLuts, adr_breite_elut, VAL_USED_BITS, theLutData->colorLutGridPoints, inputTableEntries );
	            else													/* else scale Elut to the gridpoints in the cube...*/
              		err = Fill_ushort_ELUTs_from_lut16Tag( theLutData, theInputLuts, adr_breite_elut, VAL_USED_BITS, theCombiData->gridPointsCube, inputTableEntries );
            	theLutData->inputLutWordSize = VAL_USED_BITS;
      	    }
            else
            {
	            if (theCombiData->maxProfileCount == 0)					/* if we have only one profile, the scale Elut to gridpoints in profile...*/
	                err = Fill_ushort_ELUTs_from_lut16Tag( theLutData, theInputLuts, adr_breite_elut, bit_breite_elut, theLutData->colorLutGridPoints, inputTableEntries );
	            else													/* else scale Elut to the gridpoints in the cube...*/
	                err = Fill_ushort_ELUTs_from_lut16Tag( theLutData, theInputLuts, adr_breite_elut, bit_breite_elut, theCombiData->gridPointsCube, inputTableEntries );
            	theLutData->inputLutWordSize = bit_breite_elut;
           }
            theLutData->inputLutEntryCount = (1<<adr_breite_elut);
		}
	}
	if (err)
		goto CleanupAndExit;

	/* ---------------------------------------------------------------------------------
	    clean up & exit
	   ---------------------------------------------------------------------------------*/
CleanupAndExit:
	theInputLuts = DisposeIfPtr(theInputLuts);

	LH_END_PROC("Extract_MFT_Elut")
	return err;
}

/* ———————————————————————————————————————————————————————————————————————————————————————————
	CMError
	Extract_MFT_Xlut	  ( CMLutParamPtr	theLutData,
							LHCombiDataPtr	theCombiData,
							Ptr				profileLutPtr,
							OSType			theTag )
   ——————————————————————————————————————————————————————————————————————————————————————————— */
CMError
Extract_MFT_Xlut	  ( CMLutParamPtr	theLutData,
						LHCombiDataPtr	theCombiData,
						Ptr				profileLutPtr,
						OSType			theTag )
{
 	CMError			err = noErr;
  	OSErr			aOSerr = noErr;
	SINT32			i;
	SINT32			clutSize;
	LUT_DATA_TYPE	localXlut = nil;
	Ptr				tempXlut   = nil;
	Ptr				bytePtr   = nil;
	Ptr				xlutPtr   = nil;
	SINT32			offset;
	SINT32			inputTableEntries;
	UINT32			byteCount;
	UINT32			theSize;
	UINT32			aExtraSize;
	
	LH_START_PROC("Extract_MFT_Xlut")
	
	clutSize = theLutData->colorLutOutDim;
	for(i=0; i<theLutData->colorLutInDim; i++)
		clutSize *= theLutData->colorLutGridPoints;

	/* ============================================================================================================================
																 mft1
	   ============================================================================================================================ */
	if ( *((OSType*)profileLutPtr) == icSigLut8Type )
	{
		if ( theCombiData->maxProfileCount == 0 ){			/* Link Profile UK13.8.96*/
			theSize = 1;
			aExtraSize = 1;
			for( i=0; i<(theLutData->colorLutInDim-1); ++i){	/* Extra Size for Interpolation */
				theSize *= theLutData->colorLutGridPoints;
				aExtraSize += theSize;
			}
#ifdef ALLOW_MMX
			aExtraSize++;	/* +1 for MMX 4 Byte access */
#endif
		}	
		else{
			aExtraSize = 0;
		}
		aExtraSize *= theLutData->colorLutOutDim;
		localXlut = ALLOC_DATA(clutSize+aExtraSize, &aOSerr);	
       	err = aOSerr;	
		if (err)
			goto CleanupAndExit;
		LOCK_DATA(localXlut);
		inputTableEntries	= 256;
		offset 				= 48 + (inputTableEntries * theLutData->colorLutInDim);
		byteCount			= clutSize;
		err = CMGetPartialProfileElement(theCombiData->theProfile, theTag, offset, &byteCount, DATA_2_PTR(localXlut));
		theLutData->colorLutWordSize = 8;
		if (err)
			goto CleanupAndExit;
	} else
	/* ============================================================================================================================
																 mft2
	   ============================================================================================================================ */
	{
		if (( theCombiData->maxProfileCount > 0 ) || (theCombiData->doCreateLinkProfile) || (theCombiData->doCreate_16bit_XLut))/* UWE 9.2.96*/
			clutSize *= 2;
		if ( theCombiData->maxProfileCount == 0 ){			/* Link Profile  UK13.8.96*/
			theSize = 1;
			aExtraSize = 1;
			for( i=0; i<(theLutData->colorLutInDim-1); ++i){	/* Extra Size for Interpolation */
				theSize *= theLutData->colorLutGridPoints;
				aExtraSize += theSize;
			}
			if (( theCombiData->doCreateLinkProfile) || (theCombiData->doCreate_16bit_XLut)){
				aExtraSize *= 2;
			}
#ifdef ALLOW_MMX
			aExtraSize++;	/* +1 for MMX 4 Byte access */
#endif
		}	
		else{
			aExtraSize = 0;
		}
		aExtraSize *= theLutData->colorLutOutDim;
		localXlut = ALLOC_DATA(clutSize+aExtraSize, &aOSerr);	
       	err = aOSerr;
		if (err)
			goto CleanupAndExit;
		LOCK_DATA(localXlut);
		inputTableEntries	= ((icLut16Type *)profileLutPtr)->lut.inputEnt;
		if (inputTableEntries <2)
		{
			err = cmProfileError;
			goto CleanupAndExit;
		}
		offset = 52 + ( inputTableEntries * theLutData->colorLutInDim) * sizeof(UINT16);
		
		if (( theCombiData->maxProfileCount > 0 ) || (theCombiData->doCreateLinkProfile) || (theCombiData->doCreate_16bit_XLut))/* UWE 9.2.96 */
		{
			byteCount = clutSize;
			tempXlut  = 0;
			err = CMGetPartialProfileElement(theCombiData->theProfile, theTag, offset, &byteCount, DATA_2_PTR(localXlut));
#ifdef IntelMode
           SwapShortOffset( localXlut, 0, byteCount );
#endif
			theLutData->colorLutWordSize = VAL_USED_BITS;
			if (err)
				goto CleanupAndExit;
		}
		else
		{									/* should only happen with 1 Link profile */
#ifdef DEBUG_OUTPUT
			if ( DebugCheck(kThisFile, kDebugMiscInfo) )
				DebugPrint("• Extract_MFT_Xlut: 1 Link profile mode\n",err);
#endif
			byteCount = clutSize * sizeof(UINT16);
			tempXlut  = SmartNewPtr(byteCount, &aOSerr);
	        err = aOSerr;
			if (err)
				goto CleanupAndExit;
			err = CMGetPartialProfileElement(theCombiData->theProfile, theTag, offset, &byteCount, tempXlut);
#ifdef IntelMode
/*           SwapShortOffset( tempXlut, 0, byteCount ); !! do not swap, take first byte */
#endif
			if (err)
				goto CleanupAndExit;
			bytePtr = tempXlut;
			xlutPtr = (Ptr)DATA_2_PTR(localXlut);
			for (i = 0; i < clutSize; i++)
			{
				*xlutPtr = *bytePtr;
				bytePtr+=2;
				xlutPtr++;
			}
			theLutData->colorLutWordSize = 8;
		}
	}
	UNLOCK_DATA(localXlut);
	theLutData->colorLut = localXlut;
	localXlut = nil;
	
	/* ---------------------------------------------------------------------------------
	    clean up & exit
	   ---------------------------------------------------------------------------------*/
CleanupAndExit:
	localXlut = DISPOSE_IF_DATA(localXlut);
	tempXlut  = DisposeIfPtr(tempXlut);

	LH_END_PROC("Extract_MFT_Xlut")
	return err;
}

/* ———————————————————————————————————————————————————————————————————————————————————————————
	CMError
	Extract_MFT_Alut	  ( CMLutParamPtr	theLutData,
							LHCombiDataPtr	theCombiData,
							Ptr				profileLutPtr )
   ——————————————————————————————————————————————————————————————————————————————————————————— */
CMError
Extract_MFT_Alut	  ( CMLutParamPtr	theLutData,
						LHCombiDataPtr	theCombiData,
						Ptr				profileLutPtr,
						OSType			theTag )
{
 	CMError		err = noErr;
  	OSErr		aOSerr = noErr;
	UINT32		byteCount;
	SINT32		offset;
	SINT32		clutSize;
	SINT32		i;
	SINT32		inputTableEntries;
	SINT32		outputTableEntries;
	Ptr			theOutputLuts	= nil;	/* •••12/1/95;*/
	Boolean		readLutFromFile = TRUE;
	UINT16*		shortPtr;
	
	LH_START_PROC("Extract_MFT_Alut")
	
	clutSize = theLutData->colorLutOutDim;
	for(i=0; i<theLutData->colorLutInDim; i++)
		clutSize *= theLutData->colorLutGridPoints;
	/* ============================================================================================================================
																 mft1
	   ============================================================================================================================ */
	if ( *((OSType*)profileLutPtr) == icSigLut8Type )
	{
		/* --------------------------------------------------------------------------------- get outputLuts out of the profile */
		inputTableEntries  = 256;
		outputTableEntries = 256;
		offset 			   = 48 + (inputTableEntries * theLutData->colorLutInDim) + clutSize;
		byteCount = theLutData->colorLutOutDim * outputTableEntries;
       	theOutputLuts = SmartNewPtr(byteCount, &aOSerr);
		err = aOSerr;
		if (err)
			goto CleanupAndExit;
		err = CMGetPartialProfileElement(theCombiData->theProfile, theTag, offset, &byteCount, theOutputLuts);
		if (err)
			goto CleanupAndExit;
		if ( theCombiData->doCreate_16bit_ALut || theCombiData->doCreate_16bit_Combi )	/* this is NOT the last Alut*/
		{
			err = Fill_ushort_ALUTs_from_lut8Tag( theLutData, theOutputLuts, adr_breite_alut);
			if (err)
				goto CleanupAndExit;
			theLutData->outputLutEntryCount = adr_bereich_alut;
			theLutData->outputLutWordSize = VAL_USED_BITS;
		}
		else																			/* this is the last Alut */
		{
			err = Fill_byte_ALUTs_from_lut8Tag( theLutData, theOutputLuts, adr_breite_alut);
			if (err)
				goto CleanupAndExit;
			theLutData->outputLutEntryCount = adr_bereich_alut;
			theLutData->outputLutWordSize = bit_breite_alut;
		}
	}
	/* ============================================================================================================================
																 mft2
	   ============================================================================================================================ */
	else
	{
		/* --------------------------------------------------------------------------------- get outputLuts out of the profile */
		inputTableEntries	= ((icLut16Type *)profileLutPtr)->lut.inputEnt;
		if (inputTableEntries <2)
		{
#if realThing
			err = cmProfileError;
			goto CleanupAndExit;
#endif
		}
		outputTableEntries	= ((icLut16Type *)profileLutPtr)->lut.outputEnt;
		if (outputTableEntries <2)
		{
#if ! realThing
			readLutFromFile = FALSE;
			outputTableEntries = 2;
#else
			err = cmProfileError;
			goto CleanupAndExit;
#endif
		}
		if (readLutFromFile)
		{
			offset		= 52 + (( inputTableEntries * theLutData->colorLutInDim) + clutSize) * sizeof(UINT16);
			byteCount	= theLutData->colorLutOutDim * outputTableEntries * sizeof(UINT16);
           	theOutputLuts = SmartNewPtr(byteCount, &aOSerr);
			err = aOSerr;
			if (err)
				goto CleanupAndExit;
			err = CMGetPartialProfileElement(theCombiData->theProfile, theTag, offset, &byteCount, theOutputLuts);
			if (err)
				goto CleanupAndExit;
#ifdef IntelMode
           SwapShortOffset( theOutputLuts, 0, byteCount );
#endif
		} else
		{
			theOutputLuts = SmartNewPtr(2 * sizeof(SINT16) * theLutData->colorLutOutDim, &aOSerr);
			err = aOSerr;
			if (err)
				goto CleanupAndExit;
			shortPtr = (UINT16*)theOutputLuts;
			for (i = 0; i< ( theLutData->colorLutOutDim * outputTableEntries ); i+=outputTableEntries)
			{
				shortPtr[i] 	= 0;
				shortPtr[i+1]	= (UINT16)0xFFFF;
			}
		}
		if ( theCombiData->doCreate_16bit_ALut || theCombiData->doCreate_16bit_Combi )/*  UWE 9.2.96	this is NOT the last Alut */
		{
			err = Fill_ushort_ALUTs_from_lut16Tag( theLutData, theOutputLuts, adr_breite_alut, outputTableEntries);
			if (err)
				goto CleanupAndExit;
			theLutData->outputLutEntryCount = adr_bereich_alut;
			theLutData->outputLutWordSize = VAL_USED_BITS;
		}
		else														/*  this is the last Alut */
		{
			err = Fill_byte_ALUTs_from_lut16Tag( theLutData, theOutputLuts, adr_breite_alut, outputTableEntries);
			if (err)
				goto CleanupAndExit;
			theLutData->outputLutEntryCount = adr_bereich_alut;
			theLutData->outputLutWordSize = bit_breite_alut;
		}
	}
	
	/*---------------------------------------------------------------------------------
	   clean up & exit
	  ---------------------------------------------------------------------------------*/
CleanupAndExit:
	theOutputLuts = DisposeIfPtr(theOutputLuts);

	LH_END_PROC("Extract_MFT_Alut")
	return err;
}

/* ———————————————————————————————————————————————————————————————————————————————————————————
	CMError
	ExtractAll_MFT_Luts  (  CMLutParamPtr	theLutData,
							LHCombiDataPtr	theCombiData,
							OSType			theTag )
   ——————————————————————————————————————————————————————————————————————————————————————————— */
CMError
ExtractAll_MFT_Luts  (  CMLutParamPtr	theLutData,
						LHCombiDataPtr	theCombiData,
						OSType			theTag )
{
  	CMError		err = noErr;
  	OSErr		aOSerr = noErr;
	Ptr			profileLutPtr = nil;
	UINT32		elementSize;
	double		factor;
	UINT32 		byteCount;

	LH_START_PROC("ExtractAll_MFT_Luts")

	/* -------------------------------------------------------- get partial tag data from profile */
	err = CMGetProfileElement(theCombiData->theProfile, theTag, &elementSize, nil);
	if (err)
		goto CleanupAndExit;
	
	byteCount = 52;											/* get the first 52 bytes out of the profile */
  	profileLutPtr = SmartNewPtr(byteCount, &aOSerr);
	err = aOSerr;
	if (err)
		goto CleanupAndExit;
	
    err = CMGetProfileElement(theCombiData->theProfile, theTag, &byteCount, profileLutPtr);
#ifdef IntelMode
    SwapLongOffset( &((icLut16Type*)profileLutPtr)->base.sig, 0, 4 );
    SwapShortOffset( &((icLut16Type*)profileLutPtr)->lut.inputEnt, 0, 2 );
    SwapShortOffset( &((icLut16Type*)profileLutPtr)->lut.outputEnt, 0, 2 );
#endif
	if (err)
		goto CleanupAndExit;

	theLutData->colorLutInDim 		= ((icLut8Type*)profileLutPtr)->lut.inputChan;
	theLutData->colorLutOutDim 		= ((icLut8Type*)profileLutPtr)->lut.outputChan;
	theLutData->colorLutGridPoints 	= ((icLut8Type*)profileLutPtr)->lut.clutPoints;
	
	/* ---------------------------------------------------------------------- handle matrix
	 	matrix is only used if:
	 	• number of input channels is 3 and
	 	• input is XYZ
	 	matrix is identity for output when pcs == Lab
	*/
	if ( ( theLutData->colorLutInDim == 3) &&
	       ( (  theCombiData->amIPCS && (theCombiData->profileConnectionSpace == icSigXYZData) ) ||
	       	 ( !theCombiData->amIPCS && (theCombiData->dataColorSpace 		  == icSigXYZData) ) ) )
	{
		factor = 1.;
		err = GetMatrixFromProfile(theLutData, theCombiData, theTag, factor);
	}
	
	/* ---------------------------------------------------------------------- process A Lut */
	err = Extract_MFT_Alut( theLutData, theCombiData, profileLutPtr, theTag );
	if (err)
		goto CleanupAndExit;
	
	/* ---------------------------------------------------------------------- process X Lut */
	err = Extract_MFT_Xlut ( theLutData, theCombiData, profileLutPtr, theTag );
	if (err)
		goto CleanupAndExit;
	
	/* ---------------------------------------------------------------------- process E Lut */
	err = Extract_MFT_Elut( theLutData, theCombiData, profileLutPtr, theTag );
	if (err)
		goto CleanupAndExit;

	/* ---------------------------------------------------------------------------------
		 clean up
	   ---------------------------------------------------------------------------------*/
CleanupAndExit:
	profileLutPtr = DisposeIfPtr(profileLutPtr);

	LH_END_PROC("ExtractAll_MFT_Luts")
	return err;
}



/* ———————————————————————————————————————————————————————————————————————————————————————————
	CMError
	Extract_TRC_Alut	  ( CMLutParamPtr	theLutData,
							LHCombiDataPtr	theCombiData )
   ——————————————————————————————————————————————————————————————————————————————————————————— */
CMError
Extract_TRC_Alut	  ( CMLutParamPtr	theLutData,
						LHCombiDataPtr	theCombiData )
{
	OSType			trcSig[kNumOfRGBchannels];
	icCurveType*	pCurveTag	   = nil;
	icCurveType*	invertCurveTag = nil;
 	CMError			err = noErr;
  	OSErr			aOSerr = noErr;
	UINT32			elementSize;
	SINT32			loop;
	SINT32			theSize;
	SINT32			theAlutSize;
	UINT8			addrBits;
	LUT_DATA_TYPE	localAlut = nil;
	
	LH_START_PROC("Extract_TRC_Alut")
	
	/* ---------------------------------------------------- initialization */
	addrBits = adr_breite_alut;
	if ( theCombiData->doCreate_16bit_ALut || theCombiData->doCreate_16bit_Combi )/* UWE 9.2.96 */
		theSize = sizeof(UINT16);
	else
		theSize = sizeof(UINT8);
	theAlutSize = (1<<addrBits);
	localAlut = ALLOC_DATA(theLutData->colorLutOutDim * theAlutSize * theSize + theSize, &aOSerr);
	err = aOSerr;
	if (err)
	{
#ifdef DEBUG_OUTPUT
		if ( err && DebugCheck(kThisFile, kDebugErrorInfo) )
			DebugPrint("• Extract_TRC_Alut ALLOC_DATA(%d * %d) error\n",theLutData->colorLutOutDim , theAlutSize);
#endif
		goto CleanupAndExit;
	}
	LOCK_DATA(localAlut);
	if (!theCombiData->amIPCS)
	{			
		/* ---------------------------------------------------------------------------------
				if NOT PCS -> create linear Alut ...
		   ---------------------------------------------------------------------------------*/
		if ( theCombiData->doCreate_16bit_ALut || theCombiData->doCreate_16bit_Combi )/* UWE 9.2.96		this is NOT the last Alut */
		{
			CreateLinearAlut16 ( (UINT16 *)DATA_2_PTR(localAlut),theAlutSize);
			for (loop = 0; loop < theLutData->colorLutOutDim; loop++)
				BlockMoveData(DATA_2_PTR(localAlut), (Ptr)DATA_2_PTR(localAlut) + loop * theAlutSize * sizeof(UINT16), theAlutSize * sizeof(UINT16));
			theLutData->outputLutEntryCount = (SINT16)theAlutSize;
			theLutData->outputLutWordSize = VAL_USED_BITS;
		}
		else													/* this is NOT the last Alut */
		{
			CreateLinearAlut ( (UINT8 *)DATA_2_PTR(localAlut),theAlutSize);
			for (loop = 0; loop < theLutData->colorLutOutDim; loop++)
				BlockMoveData(DATA_2_PTR(localAlut), (Ptr)DATA_2_PTR(localAlut) + loop * theAlutSize, theAlutSize);
			theLutData->outputLutEntryCount = (SINT16)theAlutSize;
			theLutData->outputLutWordSize = 8;
		}
	}
	else
	{
		/* ---------------------------------------------------------------------------------
		  	... else take TRCs for Aluts
		   ---------------------------------------------------------------------------------*/
		trcSig[0] = icSigRedTRCTag;
		trcSig[1] = icSigGreenTRCTag;
		trcSig[2] = icSigBlueTRCTag;
				
		for (loop = 0; loop < kNumOfRGBchannels; loop++)
		{
			err = CMGetProfileElement(theCombiData->theProfile, trcSig[loop], &elementSize, nil);
			if (err)
				goto CleanupAndExit;
            pCurveTag = (icCurveType *)SmartNewPtr(elementSize, &aOSerr);
			err = aOSerr;
			if (err)
				goto CleanupAndExit;

			err = CMGetProfileElement(theCombiData->theProfile, trcSig[loop], &elementSize, pCurveTag);
			if (err){
				goto CleanupAndExit;
			}
#ifdef IntelMode
            SwapLongOffset( &pCurveTag->base.sig, 0, 4 );
            SwapLong( &pCurveTag->curve.count );
            SwapShortOffset( pCurveTag, (ULONG)((SINT8*)&pCurveTag->curve.data[0]-(SINT8*)pCurveTag), elementSize );
#endif
			if (pCurveTag)
			{
				if ( theCombiData->doCreate_16bit_ALut || theCombiData->doCreate_16bit_Combi )	/* UWE 9.2.96		 this is NOT the last Alut*/
				{
					err = Fill_inverse_ushort_ALUT_from_CurveTag( pCurveTag, (UINT16*)DATA_2_PTR(localAlut) + (theAlutSize * loop), addrBits);
					if (err)
						goto CleanupAndExit;
					theLutData->outputLutEntryCount = (SINT16)theAlutSize;
					theLutData->outputLutWordSize = VAL_USED_BITS;
				}
				else												/* this is the last Alut */
				{
					err = Fill_inverse_byte_ALUT_from_CurveTag( pCurveTag, (UINT8*)DATA_2_PTR(localAlut) + (theAlutSize * loop), addrBits);
					if (err)
						goto CleanupAndExit;
					theLutData->outputLutEntryCount = (SINT16)theAlutSize;
					theLutData->outputLutWordSize = bit_breite_alut;
				}
				pCurveTag = (icCurveType*)DisposeIfPtr((Ptr)pCurveTag);
			} else
			{
				err = unimpErr;
				goto CleanupAndExit;
			}
		}
	}
	UNLOCK_DATA(localAlut);
	/* ----------------------------------------------- no error occured -> save results */
	theLutData->outputLut   = localAlut;	
	localAlut = nil;
	/* ---------------------------------------------------------------------------------
	    clean up & exit
	   ---------------------------------------------------------------------------------*/
CleanupAndExit:
	localAlut 		= DISPOSE_IF_DATA(localAlut);
	pCurveTag 		= (icCurveType*)DisposeIfPtr((Ptr)pCurveTag);

	LH_END_PROC("Extract_TRC_Alut")
	return err;
}


/* ———————————————————————————————————————————————————————————————————————————————————————————
	CMError
	Extract_TRC_Elut	  ( CMLutParamPtr	theLutData,
							LHCombiDataPtr	theCombiData )
   ——————————————————————————————————————————————————————————————————————————————————————————— */
CMError
Extract_TRC_Elut	  ( CMLutParamPtr	theLutData,
						LHCombiDataPtr	theCombiData )
{
	OSType			trcSig[kNumOfRGBchannels];
	icCurveType*	pCurveTag = nil;
	SINT32			loop;
	SINT32			theElutSize;
 	CMError			err = noErr;
  	OSErr			aOSerr = noErr;
	UINT32			elementSize;
	LUT_DATA_TYPE	localElut = nil;
	Ptr				singleElut = nil;

	LH_START_PROC("Extract_TRC_Elut")

	theElutSize =  (1<<adr_breite_elut) * sizeof (UINT16);
	localElut = ALLOC_DATA(theLutData->colorLutInDim * theElutSize + sizeof (UINT16), &aOSerr);
	err = aOSerr;
	if (err)
		goto CleanupAndExit;

	/* --------------------------------------------------------------------------------- set gridPoints */
	theLutData->colorLutGridPoints = theCombiData->gridPointsCube;
	
	LOCK_DATA(localElut);
	if (theCombiData->amIPCS)
	{
		/* ---------------------------------------------------------------------------------
		  	if PCS -> create linear Elut...
		   --------------------------------------------------------------------------------- */
		if ( theCombiData->doCreate_16bit_ELut )					/* this is NOT the first Elut */
		{
			CreateLinearElut16 ( (Ptr)DATA_2_PTR(localElut), theElutSize / sizeof(UINT16), theLutData->colorLutGridPoints, 0);
			theLutData->inputLutEntryCount = theElutSize / sizeof(UINT16);
			theLutData->inputLutWordSize = VAL_USED_BITS;
		}
		else														/* this is the first Elut */
		{
          	if ( theCombiData->doCreate_16bit_Combi )				/* UWE 9.2.96 */
          	{
            	CreateLinearElut16 ( (Ptr)DATA_2_PTR(localElut), theElutSize / sizeof(UINT16), theCombiData->gridPointsCube, 0);
            	theLutData->inputLutWordSize = VAL_USED_BITS;
            }
            else
            {
            	CreateLinearElut ( (Ptr)DATA_2_PTR(localElut), theElutSize / sizeof(UINT16), theCombiData->gridPointsCube, 0);
            	theLutData->inputLutWordSize = bit_breite_elut;
            }
            theLutData->inputLutEntryCount = theElutSize / sizeof(UINT16);
		}
		for (loop = 1; loop < theLutData->colorLutInDim; loop++)
			BlockMoveData(DATA_2_PTR(localElut), (Ptr)DATA_2_PTR(localElut) + loop * theElutSize, theElutSize);
	}
	else
	{
		/* ---------------------------------------------------------------------------------
		  	... else take TRCs for Eluts
		   --------------------------------------------------------------------------------- */
		trcSig[0] = icSigRedTRCTag;
		trcSig[1] = icSigGreenTRCTag;
		trcSig[2] = icSigBlueTRCTag;
						
		singleElut = SmartNewPtr(theElutSize, &aOSerr);
		err = aOSerr;
		if (err)
			goto CleanupAndExit;
		for (loop = 0; loop < kNumOfRGBchannels; loop++)
		{
			err = CMGetProfileElement(theCombiData->theProfile, trcSig[loop], &elementSize, nil);
			if (err)
				goto CleanupAndExit;
			pCurveTag = (icCurveType *)SmartNewPtr(elementSize, &aOSerr);
			err = aOSerr;
			if (err)
				goto CleanupAndExit;

			err = CMGetProfileElement(theCombiData->theProfile, trcSig[loop], &elementSize, pCurveTag);
#ifdef IntelMode
            SwapLongOffset( &pCurveTag->base.sig, 0, 4 );
            SwapLong( &pCurveTag->curve.count );
            SwapShortOffset( pCurveTag, (ULONG)((char*)&pCurveTag->curve.data[0]-(char*)pCurveTag), elementSize );
#endif
			if (err)
				goto CleanupAndExit;
			if ( theCombiData->doCreate_16bit_ELut  )		/* this is either NOT the first Elut -or- we have a NewLinkProfile call */
			{
				if ((theCombiData->doCreateLinkProfile) && (theCombiData->profLoop == 0))
					err = Fill_ushort_ELUT_from_CurveTag(pCurveTag, (UINT16*)singleElut, adr_breite_elut, VAL_USED_BITS, 0);
				else
					err = Fill_ushort_ELUT_from_CurveTag(pCurveTag, (UINT16*)singleElut, adr_breite_elut, VAL_USED_BITS, theLutData->colorLutGridPoints);
				theLutData->inputLutEntryCount = (1<<adr_breite_elut);
				theLutData->inputLutWordSize = VAL_USED_BITS;
			}
			else														/* this is the first Elut */
			{
         	  	if ( theCombiData->doCreate_16bit_Combi ) 	/* UWE 9.2.96 */
         	  	{
                	err = Fill_ushort_ELUT_from_CurveTag(pCurveTag, (UINT16*)singleElut, adr_breite_elut, VAL_USED_BITS, theCombiData->gridPointsCube);
                    theLutData->inputLutWordSize = VAL_USED_BITS;
                }
                else
                {
                	err = Fill_ushort_ELUT_from_CurveTag(pCurveTag, (UINT16*)singleElut, adr_breite_elut, bit_breite_elut, theCombiData->gridPointsCube);
                    theLutData->inputLutWordSize = bit_breite_elut;
                }
                theLutData->inputLutEntryCount = (1<<adr_breite_elut);
			}
			if (err)
				goto CleanupAndExit;
			BlockMoveData(singleElut, (Ptr)DATA_2_PTR(localElut) + loop * theElutSize, theElutSize);
			pCurveTag  = (icCurveType *)DisposeIfPtr((Ptr)pCurveTag);
		}
	}
	/* ----------------------------------------------- no error occured -> save results */
	UNLOCK_DATA(localElut);
	theLutData->inputLut = localElut;
	localElut = nil;
	/* ---------------------------------------------------------------------------------
		clean up & exit
	   --------------------------------------------------------------------------------- */
CleanupAndExit:
	pCurveTag  = (icCurveType *)DisposeIfPtr((Ptr)pCurveTag);
	localElut  = DISPOSE_IF_DATA(localElut);
	singleElut = DisposeIfPtr(singleElut);

	LH_END_PROC("Extract_TRC_Elut")
	return err;
}
/* ———————————————————————————————————————————————————————————————————————————————————————————
	CMError
	Extract_TRC_Matrix	  ( CMLutParamPtr	theLutData,
							LHCombiDataPtr	theCombiData )
   ——————————————————————————————————————————————————————————————————————————————————————————— */
CMError
Extract_TRC_Matrix	  ( CMLutParamPtr	theLutData,
						LHCombiDataPtr	theCombiData )
{
	icXYZType 		colorantData[kNumOfRGBchannels];
	icXYZType		curMediaWhite;
	Matrix2D		localMatrix;
	Matrix2D		invertMatrix;
	OSType			colorantTags[kNumOfRGBchannels];
 	CMError			err = noErr;
  	OSErr			aOSerr = noErr;
	SINT32			loop;
	SINT32			i;
	SINT32			j;
	UINT32 			elementSize;
	double			factor;

	LH_START_PROC("Extract_TRC_Matrix")

	/* -----------------------------------------------------------------initialize */
	colorantTags[0] = icSigRedColorantTag;
	colorantTags[1] = icSigGreenColorantTag;
	colorantTags[2] = icSigBlueColorantTag;
		
	/* ---------------------------------------------------------------------------------
	  	take Matrix	from profile
	   ---------------------------------------------------------------------------------*/
	for (loop = 0; loop < kNumOfRGBchannels; loop++)
	{
		err = CMGetProfileElement(theCombiData->theProfile, colorantTags[loop], &elementSize, nil);
		if (err != noErr)
			goto CleanupAndExit;
		err = CMGetProfileElement(theCombiData->theProfile, colorantTags[loop], &elementSize, &colorantData[loop]);
#ifdef IntelMode
        SwapLongOffset( &colorantData[loop].base.sig, 0, 4 );
        SwapLongOffset( &colorantData[loop], (ULONG)((SINT8*)&colorantData[0].data.data[0]-(SINT8*)&colorantData[0]), elementSize );
#endif
		if (err != noErr)
			goto CleanupAndExit;
		localMatrix[0][loop] = ((double)colorantData[loop].data.data[0].X) / 65536.    / 2.;
		localMatrix[1][loop] = ((double)colorantData[loop].data.data[0].Y) / 65536.    / 2.;
		localMatrix[2][loop] = ((double)colorantData[loop].data.data[0].Z) / 65536.    / 2.;
	}
	
	if (theCombiData->renderingIntent == icAbsoluteColorimetric)
	{
		elementSize = sizeof(icXYZType);
		err = CMGetProfileElement(theCombiData->theProfile, icSigMediaWhitePointTag, &elementSize, &curMediaWhite);
#ifdef IntelMode
        SwapLongOffset( &curMediaWhite.base.sig, 0, 4 );
        SwapLongOffset( &curMediaWhite, (ULONG)((SINT8*)&curMediaWhite.data.data[0]-(SINT8*)&curMediaWhite), elementSize );
#endif
		if (err != cmElementTagNotFound)		/* otherwise take D50 -> do nothing */
		{
			if (err != noErr)
				goto CleanupAndExit;
		
			for (i=0; i<3; i++)		/* adjust for media white point */
			{
				if (i == 0)									/* divide X by D50 white X */
					factor = ((double)curMediaWhite.data.data[0].X) / 65536. / 0.9642;
				else if (i == 1)
					factor = ((double)curMediaWhite.data.data[0].Y) / 65536.;
				else										/* divide Z by D50 white Z */
					factor = ((double)curMediaWhite.data.data[0].Z) / 65536. / 0.8249;
				
				for (j=0; j<3; j++)
					localMatrix[i][j] *= factor;
			}
		}
	}
	
	if (theCombiData->amIPCS)
	{
		doubMatrixInvert(localMatrix, invertMatrix);
		BlockMoveData(invertMatrix, localMatrix, 9 * sizeof(double));
	}
	
	theLutData->matrixTRC = SmartNewPtr(sizeof(Matrix2D), &aOSerr);
	err = aOSerr;
	if (err)
		goto CleanupAndExit;
	BlockMoveData(localMatrix, theLutData->matrixTRC, sizeof(Matrix2D));
	/* ---------------------------------------------------------------------------------
	    clean up & exit
	   ---------------------------------------------------------------------------------*/
CleanupAndExit:

	LH_END_PROC("Extract_TRC_Matrix")
	return err;
}

/* ———————————————————————————————————————————————————————————————————————————————————————————
	CMError
	ExtractAll_TRC_Luts  (  CMLutParamPtr	theLutData,
							LHCombiDataPtr	theCombiData )
   ——————————————————————————————————————————————————————————————————————————————————————————— */
CMError
ExtractAll_TRC_Luts  (  CMLutParamPtr	theLutData,
						LHCombiDataPtr	theCombiData )
{
	CMError	err = noErr;
 	
	LH_START_PROC("ExtractAll_TRC_Luts")
 	
	theLutData->colorLutInDim = kNumOfRGBchannels;
	theLutData->colorLutOutDim = kNumOfRGBchannels;
			
	/* ------------------------------------------------------------------------- process A lut */
	err = Extract_TRC_Alut( theLutData, theCombiData);
	if (err != noErr)
		goto CleanupAndExit;
		
	/* ------------------------------------------------------------------------- process matrix */
	err = Extract_TRC_Matrix ( theLutData, theCombiData);
	if (err != noErr)
		goto CleanupAndExit;
		
	/* ------------------------------------------------------------------------- process E lut */
	err = Extract_TRC_Elut( theLutData, theCombiData);
	if (err != noErr)
		goto CleanupAndExit;
		
	/* ---------------------------------------------------------------------------------
	    clean up
	   ---------------------------------------------------------------------------------*/
CleanupAndExit:

	LH_END_PROC("ExtractAll_TRC_Luts")
	return err;
}

/* ———————————————————————————————————————————————————————————————————————————————————————————
	CMError
	Extract_Gray_Luts	 (  CMLutParamPtr	theLutData,
							LHCombiDataPtr	theCombiData )
   ——————————————————————————————————————————————————————————————————————————————————————————— */
CMError
Extract_Gray_Luts	 (  CMLutParamPtr	theLutData,
						LHCombiDataPtr	theCombiData )
{
 	CMError			err = noErr;
  	OSErr			aOSerr = noErr;
	LUT_DATA_TYPE	theElut = nil;
	LUT_DATA_TYPE	theXlut = nil;
	LUT_DATA_TYPE	theAlut = nil;
	SINT32			theElutSize;
	SINT32			theAlutSize;
	SINT32			theXlutSize;
	SINT32			loop;
	UINT32			elementSize;
	UINT8*			bytePtr 	   = nil;
	UINT16*			wordPtr 	   = nil;
	icCurveType*	pCurveTag 	   = nil;
	SINT32			theSize;
	
	LH_START_PROC("Extract_Gray_Luts")
	
	theLutData->colorLutGridPoints = 2;
	
	if (theCombiData->amIPCS)
	{
		theLutData->colorLutInDim = 3;
		theLutData->colorLutOutDim = 1;
	} else
	{
		theLutData->colorLutInDim = 1;
		theLutData->colorLutOutDim = 3;
	}
	
	if (err)
		goto CleanupAndExit;

	/* --------------------------------------------------------------------------------- get tag data */
	err = CMGetProfileElement(theCombiData->theProfile, icSigGrayTRCTag, &elementSize, nil);
	if (err)
		goto CleanupAndExit;
	pCurveTag = (icCurveType *)SmartNewPtr(elementSize, &aOSerr);
	err = aOSerr;
	if (err)
		goto CleanupAndExit;
	err = CMGetProfileElement(theCombiData->theProfile, icSigGrayTRCTag, &elementSize, pCurveTag);
#ifdef IntelMode
    SwapLongOffset( &pCurveTag->base.sig, 0, 4 );
    SwapLong( &pCurveTag->curve.count );
    SwapShortOffset( pCurveTag, (ULONG)((SINT8*)&pCurveTag->curve.data[0]-(SINT8*)pCurveTag), elementSize );
#endif
	if (err)
		goto CleanupAndExit;

	/* €€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€
	  												X l u t
	   €€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€*/
	if ( theCombiData->profileConnectionSpace == icSigLabData )
	{
		if (theCombiData->amIPCS)			/*------------------------------------------------ Lab -> Gray */
		{
			theXlutSize = ((1 << theLutData->colorLutInDim)+1 ) * (theLutData->colorLutOutDim) * sizeof(UINT16); /*+1=Extra Size for Interpolatio UK13.8.96*/
			theXlut = ALLOC_DATA(theXlutSize + sizeof (UINT16), &aOSerr);
			err = aOSerr;
			if (err)
				goto CleanupAndExit;
			LOCK_DATA(theXlut);
			wordPtr = (UINT16*)DATA_2_PTR(theXlut);
			*(wordPtr    )  = 0x0000;
			*(wordPtr + 1)  = 0x0000;
			*(wordPtr + 2)  = 0x0000;
			*(wordPtr + 3)  = 0x0000;
			*(wordPtr + 4)  = 0xffff;
			*(wordPtr + 5)  = 0xffff;
			*(wordPtr + 6)  = 0xffff;
			*(wordPtr + 7)  = 0xffff;
			UNLOCK_DATA(theXlut);
			theLutData->colorLutWordSize = 16;
		} else								/*------------------------------------------------ Gray -> Lab */
		{
			theXlutSize = (1 << theLutData->colorLutInDim ) * (theLutData->colorLutOutDim) * sizeof(UINT16);
			theXlut = ALLOC_DATA(theXlutSize + sizeof (UINT16), &aOSerr);
			err = aOSerr;
			if (err)
				goto CleanupAndExit;
			LOCK_DATA(theXlut);
			wordPtr = (UINT16*)DATA_2_PTR(theXlut);
			*(wordPtr    )  = 0x0000;
			*(wordPtr + 1)  = 0x0000;
			*(wordPtr + 2)  = 0x0000;
			*(wordPtr + 3)  = 0xffff;
			*(wordPtr + 4)  = 0xffff;
			*(wordPtr + 5)  = 0xffff;
			UNLOCK_DATA(theXlut);
			theLutData->colorLutWordSize = 16;
		}
	} else
	{
		if (theCombiData->amIPCS)			/*------------------------------------------------ XYZ -> Gray */
		{
			theLutData->colorLutGridPoints = 3;
			theXlutSize = (9 * theLutData->colorLutInDim ) * (theLutData->colorLutOutDim) * sizeof(UINT16);
			theXlut = ALLOC_DATA(theXlutSize + 7*sizeof (UINT16), &aOSerr);
			err = aOSerr;
			if (err)
				goto CleanupAndExit;
			LOCK_DATA(theXlut);
			wordPtr = (UINT16*)DATA_2_PTR(theXlut);
			for (loop = 0; loop < theLutData->colorLutGridPoints; loop ++)
			{
				*wordPtr++  = 0x0000;
				*wordPtr++  = 0x0000;
				*wordPtr++  = 0x0000;
				*wordPtr++  = 0xffff;
				*wordPtr++  = 0xffff;
				*wordPtr++  = 0xffff;
				*wordPtr++  = 0xffff;
				*wordPtr++  = 0xffff;
				*wordPtr++  = 0xffff;
			}
			UNLOCK_DATA(theXlut);
			theLutData->colorLutWordSize = 16;
		} else								/*------------------------------------------------ Gray -> XYZ */
		{
			theXlutSize = (1 << theLutData->colorLutInDim ) * (theLutData->colorLutOutDim) * sizeof(UINT16);
			theXlut = ALLOC_DATA(theXlutSize + sizeof (UINT16), &aOSerr);
			err = aOSerr;
			if (err)
				goto CleanupAndExit;
			LOCK_DATA(theXlut);
			wordPtr = (UINT16*)DATA_2_PTR(theXlut);
			*(wordPtr    )  = 0x0000;
			*(wordPtr + 1)  = 0x0000;
			*(wordPtr + 2)  = 0x0000;
			*(wordPtr + 3)  = 0x0f6d5 / 2;
			*(wordPtr + 4)  = 0x0ffff / 2;
			*(wordPtr + 5)  = 0x0d3c2 / 2;
			UNLOCK_DATA(theXlut);
			theLutData->colorLutWordSize = 16;
		}
	}

	/* €€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€
	  												E l u t
	   €€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€*/
	theElutSize = (1<<adr_breite_elut) * sizeof (UINT16);
	theElut = ALLOC_DATA(theElutSize * theLutData->colorLutInDim + sizeof (UINT16), &aOSerr);
	err = aOSerr;
	if (err)
		goto CleanupAndExit;
	LOCK_DATA(theElut);
		
	if (theCombiData->amIPCS)
	{
		/* -----------------------------------------------------------------------------
		  	if PCS -> create Elut
		   -----------------------------------------------------------------------------*/
		if ( theCombiData->doCreate_16bit_ELut )					/* this is NOT the first Elut */
		{
			CreateLinearElut16 ( (Ptr)DATA_2_PTR(theElut), theElutSize / sizeof(UINT16), theLutData->colorLutGridPoints, 0);
			theLutData->inputLutEntryCount = theElutSize / sizeof(UINT16);
			theLutData->inputLutWordSize = VAL_USED_BITS;
		}
        else														/* this is the first Elut*/
        {
          	if ( theCombiData->doCreate_16bit_Combi )	/* UWE 9.2.96 */
          	{
	            CreateLinearElut16 ( (Ptr)DATA_2_PTR(theElut), theElutSize / sizeof(UINT16), theCombiData->gridPointsCube, 0);
	            theLutData->inputLutWordSize = VAL_USED_BITS;
	        }
	        else
			{
				CreateLinearElut ( (Ptr)DATA_2_PTR(theElut), theElutSize / sizeof(UINT16), theCombiData->gridPointsCube, 0);
				theLutData->inputLutWordSize = bit_breite_elut;
			}
           	theLutData->inputLutEntryCount = theElutSize / sizeof(UINT16);
		}
	}
	else
	{
		/* -----------------------------------------------------------------------------
		  	... else get Elut from TRC tag
		   -----------------------------------------------------------------------------*/
		if ( theCombiData->doCreate_16bit_ELut  )					/* this is NOT the first Elut */
		{
			err = Fill_ushort_ELUT_from_CurveTag(pCurveTag, (UINT16*)DATA_2_PTR(theElut), adr_breite_elut, VAL_USED_BITS, theLutData->colorLutGridPoints);
			theLutData->inputLutEntryCount = (1<<adr_breite_elut);
			theLutData->inputLutWordSize = VAL_USED_BITS;
		}
		else														/* this is the first Elut */
		{
          	if ( theCombiData->doCreate_16bit_Combi )		/* UWE 9.2.96 */
          	{
	            err = Fill_ushort_ELUT_from_CurveTag(pCurveTag, (UINT16*)DATA_2_PTR(theElut), adr_breite_elut, VAL_USED_BITS, theCombiData->gridPointsCube);
	            theLutData->inputLutWordSize = VAL_USED_BITS;
			}
	     	else
	     	{
	            err = Fill_ushort_ELUT_from_CurveTag(pCurveTag, (UINT16*)DATA_2_PTR(theElut), adr_breite_elut, bit_breite_elut, theCombiData->gridPointsCube);
	            theLutData->inputLutWordSize = bit_breite_elut;
			}
            theLutData->inputLutEntryCount = (1<<adr_breite_elut);
		}

	}
	for (loop = 1; loop < (theLutData->colorLutInDim); loop++)
		BlockMoveData(DATA_2_PTR(theElut), (Ptr)DATA_2_PTR(theElut) + loop * theElutSize, theElutSize);
	UNLOCK_DATA(theElut);

	/* €€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€
	  												A l u t
	   €€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€€*/
	if ( theCombiData->doCreate_16bit_ALut || theCombiData->doCreate_16bit_Combi ) /* UWE 9.2.96*/
		theSize = sizeof(UINT16);
	else
		theSize = sizeof(UINT8);
	
	theAlutSize = (1<<adr_breite_alut);
	theAlut = ALLOC_DATA(theLutData->colorLutOutDim * theAlutSize * theSize + theSize, &aOSerr);
	err = aOSerr;
	if (err)
		goto CleanupAndExit;

	LOCK_DATA(theAlut);
	if (theCombiData->amIPCS)
	{
		/* -----------------------------------------------------------------------------
		  	if PCS -> get Alut from TRC tag
		   -----------------------------------------------------------------------------*/
		if ( theCombiData->doCreate_16bit_ALut || theCombiData->doCreate_16bit_Combi )	/*  UWE 9.2.96			this is NOT the last Alut*/
		{
			err = Fill_inverse_ushort_ALUT_from_CurveTag( pCurveTag, (UINT16*)DATA_2_PTR(theAlut), adr_breite_alut);
			if (err)
				goto CleanupAndExit;
			theLutData->outputLutEntryCount = (SINT16)theAlutSize;
			theLutData->outputLutWordSize = VAL_USED_BITS;
		}
		else															/* this is the last Alut */
		{
			err = Fill_inverse_byte_ALUT_from_CurveTag( pCurveTag, (UINT8*)DATA_2_PTR(theAlut), adr_breite_alut);
			if (err)
				goto CleanupAndExit;
			theLutData->outputLutEntryCount = (SINT16)theAlutSize;
			theLutData->outputLutWordSize = bit_breite_alut;
		}
	} else
	{
		/* -----------------------------------------------------------------------------
		  	... else create linear Alut
		   -----------------------------------------------------------------------------*/
		if ( theCombiData->doCreate_16bit_ALut || theCombiData->doCreate_16bit_Combi )	/* UWE 9.2.96			this is NOT the last Alut */
		{
			CreateLinearAlut16 ( (UINT16 *)DATA_2_PTR(theAlut), theAlutSize);
			theLutData->outputLutEntryCount = (SINT16)theAlutSize;
			theLutData->outputLutWordSize = VAL_USED_BITS;
			if ( theCombiData->profileConnectionSpace == icSigLabData )
				SetMem16((Ptr)DATA_2_PTR(theAlut) + theAlutSize * theSize, (theLutData->colorLutOutDim -1 ) * theAlutSize, 0x08000);
			else
			{
				for (loop = 1; loop < theLutData->colorLutOutDim; loop++)
					BlockMoveData(DATA_2_PTR(theAlut), (Ptr)DATA_2_PTR(theAlut) + loop * theAlutSize * sizeof(UINT16), theAlutSize * sizeof(UINT16));
			}
		}
		else														/* this is the last Alut */
		{
			CreateLinearAlut ( (UINT8 *)DATA_2_PTR(theAlut), theAlutSize);
			theLutData->outputLutEntryCount = theAlutSize;
			theLutData->outputLutWordSize = VAL_USED_BITS;
			if ( theCombiData->profileConnectionSpace == icSigLabData )
				SetMem((Ptr)DATA_2_PTR(theAlut) + theAlutSize * theSize, (theLutData->colorLutOutDim -1 ) * theAlutSize * theSize, 0x080);
			else
			{
				for (loop = 1; loop < theLutData->colorLutOutDim; loop++)
					BlockMoveData(DATA_2_PTR(theAlut), (Ptr)DATA_2_PTR(theAlut) + loop * theAlutSize, theAlutSize);
			}
		}
	}
	UNLOCK_DATA(theAlut);

	/* --------------------------------------------------------------------------------- */
	theLutData->inputLut = theElut; theElut = nil;
	theLutData->outputLut = theAlut; theAlut = nil;
	theLutData->colorLut = theXlut; theXlut = nil;

	/* ---------------------------------------------------------------------------------
	    clean up
	   --------------------------------------------------------------------------------- */
CleanupAndExit:
	pCurveTag	= (icCurveType*)DisposeIfPtr((Ptr)pCurveTag);
	theElut 	=  DISPOSE_IF_DATA(theElut);
	theAlut 	=  DISPOSE_IF_DATA(theAlut);
	theXlut 	=  DISPOSE_IF_DATA(theXlut);

	LH_END_PROC("Extract_Gray_Luts")
	return err;
}

/* ———————————————————————————————————————————————————————————————————————————————————————————
	CMError
	ExtractAllLuts    ( CMLutParamPtr	theLutData,
						LHCombiDataPtr	theCombiData )
   ——————————————————————————————————————————————————————————————————————————————————————————— */
CMError
ExtractAllLuts    ( CMLutParamPtr	theLutData,
					LHCombiDataPtr	theCombiData )
{
	CMError	 	err = noErr;
	OSType		theTag;

	LH_START_PROC("ExtractAllLuts")

	/* --------------------------------------------------------------------------------- find out what tag to use... */
	if ( theCombiData->doCreateGamutLut )
		theTag = icSigGamutTag;
	else
	{
		switch (theCombiData->renderingIntent)
		{
			case icPerceptual:				/* Photographic images */
				if (theCombiData->usePreviewTag)
					theTag = icSigPreview0Tag;
				else
				{
					if (theCombiData->amIPCS)
						theTag = icSigBToA0Tag;
					else
						theTag = icSigAToB0Tag;
				}
				break;
			case icRelativeColorimetric:	/* Logo Colors */
				if (theCombiData->usePreviewTag)
					theTag = icSigPreview1Tag;
				else
				{
					if (theCombiData->amIPCS)
						theTag = icSigBToA1Tag;
					else
						theTag = icSigAToB1Tag;
				}
				break;
			case icSaturation:				/* Business graphics */
				if (theCombiData->usePreviewTag)
					theTag = icSigPreview2Tag;
				else
				{
					if (theCombiData->amIPCS)
						theTag = icSigBToA2Tag;
					else
						theTag = icSigAToB2Tag;
				}
				break;
			case icAbsoluteColorimetric:	/* Logo Colors */
				if (theCombiData->usePreviewTag)
					theTag = icSigPreview1Tag;
				else
				{
					if (theCombiData->amIPCS)
						theTag = icSigBToA1Tag;
					else
						theTag = icSigAToB1Tag;
				}
				break;
			default:
				err = cmProfileError;
				break;
		}
	}
#ifdef DEBUG_OUTPUT
	if ( DebugCheck(kThisFile, kDebugMiscInfo) )
		DebugPrint("  ExtractAllLuts: theTag = '%4.4s'=0x%08X (renderingIntent = %d)\n",  &theTag, theTag, theCombiData->renderingIntent );
#endif

	switch (theCombiData->profileClass)
	{
		/* ---------------------------------------------------------------------------------
		  	input profile
		   ---------------------------------------------------------------------------------*/
		case icSigInputClass:
			switch (theCombiData->dataColorSpace)
			{
				case icSigGrayData:																		/*  Gray Input Profile */
					err = Extract_Gray_Luts( theLutData, theCombiData);
					break;
				case icSigRgbData:																			/*  RGB Input Profile */
					err = ExtractAll_MFT_Luts( theLutData, theCombiData, theTag );
					if ( err != noErr && theCombiData->renderingIntent != icPerceptual )
					{
						if (theCombiData->amIPCS)
							theTag = icSigBToA0Tag;
						else
							theTag = icSigAToB0Tag;
#ifdef DEBUG_OUTPUT
						if ( DebugCheck(kThisFile, kDebugErrorInfo) )
							DebugPrint("∆ 'scnr': ExtractAll_MFT_Luts failed - continuing with '%4.4s'...\n", &theTag);
#endif
						err = ExtractAll_MFT_Luts( theLutData, theCombiData, theTag );
					}
					if (( err != noErr ) && (theCombiData->profileConnectionSpace == icSigXYZData ))
					{
						/* XYZ and no mft -> try matrix/TRC: */
#ifdef DEBUG_OUTPUT
						if ( DebugCheck(kThisFile, kDebugErrorInfo) )
							DebugPrint("∆ 'scnr': ExtractAll_MFT_Luts failed - continuing with TRC...\n");
#endif
						err = ExtractAll_TRC_Luts( theLutData, theCombiData );
					}
					break;
				case icSigCmykData:																		/* CMYK */
					err = ExtractAll_MFT_Luts( theLutData, theCombiData, theTag );
					if ( err != noErr && theCombiData->renderingIntent != icPerceptual )
					{
						if (theCombiData->amIPCS)
							theTag = icSigBToA0Tag;
						else
							theTag = icSigAToB0Tag;
#ifdef DEBUG_OUTPUT
						if ( DebugCheck(kThisFile, kDebugErrorInfo) )
							DebugPrint("∆ 'scnr': ExtractAll_MFT_Luts failed - continuing with '%4.4s'...\n", &theTag);
#endif
						err = ExtractAll_MFT_Luts( theLutData, theCombiData, theTag );
					}
					break;
				default:																				/* this covers also: cmHSVData, cmHLSData, cmCMYData */
					/* CMYK Input Profile must have an A2B0Tag (see page 17 InterColor Profile Format) */
					if (theCombiData->amIPCS)
						theTag = icSigBToA0Tag;
					else
						theTag = icSigAToB0Tag;
					err = ExtractAll_MFT_Luts( theLutData, theCombiData, theTag );
					break;
			}
			break;
		/* ---------------------------------------------------------------------------------
		  	display profile
		   ---------------------------------------------------------------------------------*/
		case icSigDisplayClass:
			switch (theCombiData->dataColorSpace)
			{
				case icSigGrayData:																		/*  Gray display Profile */
					err = Extract_Gray_Luts( theLutData, theCombiData);
					break;
				case icSigRgbData:																			/*  RGB display Profile */
					if ( theCombiData->doCreateGamutLut)
					{
						err = DoMakeGamutForMonitor( theLutData, theCombiData );
					} else
					{
						err = ExtractAll_MFT_Luts(theLutData, theCombiData, theTag);
						if (( err != noErr ) && (theCombiData->profileConnectionSpace == icSigXYZData ))
						{
							/* XYZ and no mft -> try matrix/TRC: */
#ifdef DEBUG_OUTPUT
							if ( DebugCheck(kThisFile, kDebugErrorInfo) )
								DebugPrint("∆ 'mntr': ExtractAll_MFT_Luts failed - continuing with TRC...\n");
#endif
							err = ExtractAll_TRC_Luts( theLutData, theCombiData );
						}
					}
					break;
				case icSigCmykData:
				default:																				/* this covers: cmCMYK, cmHSVData, cmHLSData, cmCMYData */
					err = ExtractAll_MFT_Luts(theLutData, theCombiData, theTag);
					break;
			}
			break;
		/* ---------------------------------------------------------------------------------
		  	output profile
		   ---------------------------------------------------------------------------------*/
		case icSigOutputClass:
			switch (theCombiData->dataColorSpace)
			{
				case icSigGrayData:																		/*  Gray output Profile */
					err = Extract_Gray_Luts( theLutData, theCombiData );
					break;
				case icSigRgbData:																			/*  RGB output Profile */
				case icSigCmykData:																		/*  CMYK output Profile */
				default:																				/*  this covers also: cmHSVData, cmHLSData, cmCMYData*/
					err = ExtractAll_MFT_Luts( theLutData, theCombiData, theTag );
					break;
			}
			break;
		/* ---------------------------------------------------------------------------------
		  	DeviceLink profile
		   ---------------------------------------------------------------------------------*/
		case icSigLinkClass:
			err = ExtractAll_MFT_Luts( theLutData, theCombiData, icSigAToB0Tag );
			break;
		/* ---------------------------------------------------------------------------------
		  	ColorSpaceConversion profile
		   ---------------------------------------------------------------------------------*/
        case icSigColorSpaceClass:		/* 'spac' */
			if (theCombiData->amIPCS)
				err = ExtractAll_MFT_Luts( theLutData, theCombiData, icSigBToA0Tag );
			else
				err = ExtractAll_MFT_Luts( theLutData, theCombiData, icSigAToB0Tag );
			break;
		/* ---------------------------------------------------------------------------------
		  	Abstract profile
		   ---------------------------------------------------------------------------------*/
        case icSigAbstractClass:		/* 'abst' */
			err = ExtractAll_MFT_Luts( theLutData, theCombiData, icSigAToB0Tag );
			break;
		default:
			err = cmProfileError;
			break;
	}

	LH_END_PROC("ExtractAllLuts")
	return err;
}
#ifdef RenderInt
long GetActualRenderingIntent( CMMModelPtr CMSession, UINT32 i )
{
	long Render;
	Render = icAbsoluteColorimetric;
	if( CMSession->dwFlags & kUseRelColorimetric ) Render = icRelativeColorimetric;
	if( i == 0xffffffff ) return Render;
	 /* First Intent of array is for first transform */
	if( CMSession-> aIntentArr && CMSession-> nIntents >= 1 ){
		if( i < CMSession-> nIntents ){
			Render = CMSession-> aIntentArr[i];
		}
	}
	return Render;
}
#endif
CMError InitNamedColorProfileData( 	CMMModelPtr 		storage,
						  			CMProfileRef		aProf,
									long				pcs,
									long				*theDeviceCoords);
/*CMError QuantizeNamedValues( CMMModelPtr 		storage,
							 Ptr				imgIn,
							 long				size );*/
/* ———————————————————————————————————————————————————————————————————————————————————————————
	CMError
	Create_LH_ProfileSet	( CMMModelPtr    		CMSession,
							  CMConcatProfileSet* 	profileSet,
							  LHConcatProfileSet**	newProfileSet )
   ——————————————————————————————————————————————————————————————————————————————————————————— */
CMError
Create_LH_ProfileSet	( CMMModelPtr    		CMSession,
						  CMConcatProfileSet* 	profileSet,
						  LHConcatProfileSet**	newProfileSet )
{
	icHeader				profHeader;
	LHConcatProfileSet* 	theProfileSet;
	CMProfileRef			theProfile;
   	OSType					previousDataColorSpace  = 0x20202020;
   	OSType					previousConnectionSpace = 0x20202020;
   	UINT32					previousRenderingIntent;
  	CMError					err = noErr;
  	OSErr					aOSerr = noErr;
	UINT32 					elementSize = 0;
	SINT16					max;
	SINT16					theSize;
	SINT16					index = 0;
	UINT16					profLoop;
	Boolean					amIPCS = FALSE;
	OSType					theTag;
	UINT32					intentIndex = 0;
	long					nDeviceCoords;

	LH_START_PROC("Create_LH_ProfileSet")
	
	nDeviceCoords=0;
	max = profileSet->count * 3;
	theSize = sizeof (LHConcatProfileSet) + sizeof(LHProfile) * max;
   	theProfileSet = (LHConcatProfileSet *)SmartNewPtrClear(theSize, &aOSerr);
	err = aOSerr;
	if (err)
		goto CleanupAndExit;
		
	theProfileSet->keyIndex = profileSet->keyIndex;
	
	CMSession->hasNamedColorProf = NoNamedColorProfile;
	for ( profLoop = 0; profLoop < profileSet->count; profLoop++ )
	{
		err = CMGetProfileHeader(profileSet->profileSet[profLoop], (CMCoreProfileHeader *)&profHeader);
		if (err)
			goto CleanupAndExit;
		if (profLoop == 0){
#ifdef RenderInt
			if( CMSession-> dwFlags != 0xffffffff ){
				profHeader.renderingIntent = GetActualRenderingIntent( CMSession, profLoop );
			}
#endif
			theProfileSet->prof[index].renderingIntent = profHeader.renderingIntent;
		}
		else{
#ifdef RenderInt
			if( CMSession-> dwFlags != 0xffffffff ){
				profHeader.renderingIntent = GetActualRenderingIntent( CMSession, profLoop );
				previousRenderingIntent = profHeader.renderingIntent;
			}
#endif
			theProfileSet->prof[index].renderingIntent = previousRenderingIntent;
		}
		if ( (profLoop > 0) && (profLoop < profileSet->count - 1) )
		{
			/*----------------------------------------------------------------------------- color space conv. inbetween? -> ignore the profile*/
			if( profHeader.deviceClass == icSigColorSpaceClass )
				continue;
			if( profHeader.deviceClass == icSigNamedColorClass ){
				err = cmCantConcatenateError;
				goto CleanupAndExit;
			}
		}
		/*-------------------------------------------------------------------------------------------------------
		   if we have non matching pcs color spaces, we have to add a conversion between Lab and XYZ
		  -------------------------------------------------------------------------------------------------------*/
		if( profHeader.deviceClass == icSigNamedColorClass ){
			err = InitNamedColorProfileData( CMSession, profileSet->profileSet[profLoop], profHeader.pcs, &nDeviceCoords );
			if (err)
				goto CleanupAndExit;
			profHeader.pcs = icSigLabData;
			if( profileSet->count == 1 ){
				CMSession->hasNamedColorProf = NamedColorProfileOnly;
				//CMSession->dataColorSpace = profHeader.colorSpace;
			}
			else if( profLoop == 0 ){
				CMSession->hasNamedColorProf = NamedColorProfileAtBegin;
				profHeader.colorSpace = icSigNamedData;
				CMSession->firstColorSpace = icSigNamedData;
				theProfileSet->prof[index].pcsConversionMode = kNoInfo;	
				theProfileSet->prof[index].profileSet = 0;
				index++;
				theProfileSet->prof[index].renderingIntent = previousRenderingIntent;
				previousDataColorSpace = (OSType)profHeader.colorSpace;
			}
			else if( profLoop == profileSet->count-1 ){
				CMSession->hasNamedColorProf = NamedColorProfileAtEnd;
				CMSession->lastColorSpace = icSigNamedData;
				profHeader.colorSpace = icSigNamedData;
				theProfileSet->prof[index].pcsConversionMode = kNoInfo;	
				theProfileSet->prof[index].profileSet = 0;
				index++;
				theProfileSet->prof[index].renderingIntent = previousRenderingIntent;
			}
		}
		if (amIPCS)
		{
			if (previousConnectionSpace != (OSType)profHeader.pcs)
			{
				/* insert a XYZ <--> Lab conversion lut */
				if (previousConnectionSpace == icSigLabData)
					theProfileSet->prof[index].pcsConversionMode = kDoLab2XYZ;		/* create Lab->XYZ */
				else
					theProfileSet->prof[index].pcsConversionMode = kDoXYZ2Lab;		/* create XYZ->Lab */
				index++;
				theProfileSet->prof[index].renderingIntent = previousRenderingIntent;
			}
		} else if (index > 0)
		{
			if ( previousDataColorSpace != (OSType)profHeader.colorSpace)
			{
				/* copy old profile for back transform to pcs */
				theProfileSet->prof[index].profileSet = theProfileSet->prof[index-1].profileSet;
				index++;
				theProfileSet->count = index;
				
				if (previousConnectionSpace != (OSType)profHeader.pcs)
				{
					/* insert a XYZ <--> Lab conversion lut */
					if (previousConnectionSpace == icSigLabData)
						theProfileSet->prof[index].pcsConversionMode = kDoLab2XYZ;		/* create Lab->XYZ */
					else
						theProfileSet->prof[index].pcsConversionMode = kDoXYZ2Lab;		/* create XYZ->Lab */
					theProfileSet->prof[index].renderingIntent = previousRenderingIntent;
					index++;
				}
				
				previousDataColorSpace  = profHeader.colorSpace;
				previousConnectionSpace = profHeader.pcs;
				amIPCS = TRUE;
			}
		}

		/*-------------------------------------------------------------------------------------------------------
		   copy the profile from the original profileSet to the LinoProfileSet
		  -------------------------------------------------------------------------------------------------------*/
		theProfileSet->prof[index].profileSet = profileSet->profileSet[profLoop];
		if( profHeader.deviceClass == icSigNamedColorClass ){
			theProfileSet->prof[index].profileSet = 0;
		}
		index++;
		if( intentIndex < CMSession-> nIntents -1 ) intentIndex++;
		/*-------------------------------------------------------------------------------------------------------
		   more than 2 profiles -> we have to double the profile in our own LHConcatProfileSet
		   if we have the profiles  RGB  RGB  RGB  we will now correctly generate: RGB  RGB  rgb  RGB
		                            XYZ  XYZ  XYZ                                  XYZ  XYZ  xyz  XYZ
		  -------------------------------------------------------------------------------------------------------*/
		if( amIPCS && (profLoop > 0) && (profLoop < profileSet->count - 1) )
		{
			/*----------------------------------------------------------------------------- color space conv. inbetween? -> ignore the profile*/
			if (profHeader.deviceClass == icSigColorSpaceClass)
				index--;										/* not used. behavior changed. see above */
			else
			{
				/*------------------------------------------------------------------------- if the inserted profile contains a preview tag, then use it...*/
				theProfile = theProfileSet->prof[index-1].profileSet;

				/* Do we check the right preview tag for the intent */
				switch (previousRenderingIntent)
				{
					case icPerceptual:				/* Photographic images */
						theTag = icSigPreview0Tag;
						break;
						
					case icRelativeColorimetric:	/* Logo Colors */
						theTag = icSigPreview1Tag;
						break;
						
					case icSaturation:				/* Business graphics */
						theTag = icSigPreview2Tag;
						break;
						
					case icAbsoluteColorimetric:	/* Logo Colors */
						theTag = icSigPreview1Tag;
						break;
						
					default:
						err = cmProfileError;
						goto CleanupAndExit;
				}
				err = CMGetProfileElement(theProfile, theTag, &elementSize, nil);
				
				if (err == noErr)
				{
					theProfileSet->prof[index-1].usePreviewTag = TRUE;
					theProfileSet->prof[index-1].renderingIntent = profHeader.renderingIntent;	/* for concate absolute */
				} else
				{
				/*------------------------------------------------------------------------- ... else insert the same profile once more */
					if ( (profHeader.deviceClass != icSigAbstractClass) && (profHeader.deviceClass != icSigLinkClass) && (profHeader.deviceClass != icSigNamedColorClass) )	
					{
#ifdef RenderInt
						if( CMSession-> dwFlags != 0xffffffff ){
							profHeader.renderingIntent = GetActualRenderingIntent( CMSession, 0xffffffff );
						}
#endif
						theProfileSet->prof[index].profileSet = profileSet->profileSet[profLoop];
						theProfileSet->prof[index].renderingIntent = profHeader.renderingIntent;/* for concate absolute */
						index++;
					}
				}
				amIPCS = FALSE;		/* will force that we stay in PCS... */
			}
		}
		theProfileSet->count = index;
		previousDataColorSpace  = profHeader.colorSpace;
		previousConnectionSpace = profHeader.pcs;
		previousRenderingIntent = profHeader.renderingIntent;

		if( profHeader.deviceClass == icSigLinkClass ){
			previousDataColorSpace = profHeader.pcs;
			amIPCS = TRUE;
		}
		if (amIPCS)
			amIPCS = (previousDataColorSpace == icSigLabData) || (previousDataColorSpace == icSigXYZData);
		else
			amIPCS = TRUE;
	}

#ifdef ALLOW_DEVICE_LINK
	if( (CMSession)->appendDeviceLink ){
		if( previousDataColorSpace == (CMSession)->lastColorSpace ){
			theProfileSet->prof[index].profileSet = profileSet->profileSet[profLoop];
			theProfileSet->prof[index].renderingIntent = icPerceptual;
			index++;
			theProfileSet->count = index;
			err = CMGetProfileHeader(profileSet->profileSet[profLoop], (CMCoreProfileHeader *)&profHeader);
			if (err)
				goto CleanupAndExit;
			(CMSession)->lastColorSpace = profHeader.pcs;
		}
		else{
			err = cmCantConcatenateError;
			goto CleanupAndExit;
		}
	}
	if( CMSession->hasNamedColorProf == NamedColorProfileAtEnd || CMSession->hasNamedColorProf == NamedColorProfileOnly ){
			theProfileSet->count--;
	}
	if( CMSession->hasNamedColorProf == NamedColorProfileAtBegin ){
		theProfileSet->count--;					/* remove first profile */
		for( profLoop=0; profLoop<theProfileSet->count; profLoop++){
			theProfileSet->prof[profLoop] = theProfileSet->prof[profLoop+1];
		}
	}
#endif
	*newProfileSet = theProfileSet;
	theProfileSet = nil;
	/* ---------------------------------------------------------------------------------
	    clean up
	   --------------------------------------------------------------------------------- */
CleanupAndExit:
	theProfileSet = (LHConcatProfileSet *)DisposeIfPtr((Ptr)theProfileSet);

	LH_END_PROC("Create_LH_ProfileSet")
	return err;
}

#define DoLabSpecial
#ifdef DoLabSpecial
OSErr MakeSpecialCube16(	long 			inputDim,
		  					long 			*theCubeSize,
		  					CUBE_DATA_TYPE	*theCube,
							long 			*theBits,
							long 			*theExtraSize );
OSErr MakeSpecialCube16(	long 			inputDim,
		  					long 			*theCubeSize,
		  					CUBE_DATA_TYPE	*theCube,
							long 			*theBits,
							long 			*theExtraSize )
{
	long 			needBits,theSize,aExtraSize;
	long 			i,gridPoints;
	OSErr			err = noErr;
	UINT16 			*cube = nil;
	CUBE_DATA_TYPE	tempCube;
	
	LH_START_PROC("MakeSpecialCube16")
	err = CalcGridPoints4Cube(*theCubeSize, inputDim, &gridPoints, &needBits);
	if (err)
		goto CleanupAndExit;
		
	theSize = 1;
	aExtraSize = 1;
	for( i=0; i<(inputDim-1); ++i){	/* Extra Size for Interpolation */
		theSize *= gridPoints;
		aExtraSize += theSize;
	}
	
#ifdef ALLOW_MMX
	aExtraSize++;	/* +1 for MMX 4 Byte access */
#endif
    theSize *= gridPoints;
    	
	*theExtraSize = aExtraSize;
	*theCubeSize = theSize * inputDim;
	tempCube = ALLOC_DATA( (theSize+aExtraSize) * inputDim*2, &err);
	if (err != noErr)
		goto CleanupAndExit;
	LOCK_DATA(tempCube);
	cube = (UINT16*)DATA_2_PTR(tempCube);

	*theBits = needBits;
	if( inputDim == 3)
	{
		register unsigned long  aShift;
		register long  j,k;
		register UINT16 aI, aJ, aK;
		aShift = (16 - needBits);

		gridPoints /= 2;
		needBits--;
		for( i=0; i<=gridPoints; ++i){
			aI = (UINT16)(i<< aShift);
			for( j=0; j<=gridPoints; ++j){
				aJ = (UINT16)(j<< aShift);
				for( k=0; k<=gridPoints; ++k){
					*cube++ = aI;
					*cube++ = aJ;
					aK = (UINT16)(k<< aShift);
					*cube++ = aK;
				}
				for( k=1; k<gridPoints; ++k){
					*cube++ = aI;
					*cube++ = aJ;
					aK = (UINT16)(k<< aShift);
					aK |= aK >> needBits;
					aK |= aK >> (2*needBits);
					aK |= aK >> (4*needBits);
					*cube++ = aK+0x08000;
				}
			}
			for( j=1; j<gridPoints; ++j){
				aJ = (UINT16)(j<< aShift);
				aJ |= aJ >> needBits;
				aJ |= aJ >> (2*needBits);
				aJ |= aJ >> (4*needBits);
				aJ += 0x08000;
				for( k=0; k<=gridPoints; ++k){
					*cube++ = aI;
					*cube++ = aJ;
					aK = (UINT16)(k<< aShift);
					*cube++ = aK;
				}
				for( k=1; k<gridPoints; ++k){
					*cube++ = aI;
					*cube++ = aJ;
					aK = (UINT16)(k<< aShift);
					aK |= aK >> needBits;
					aK |= aK >> (2*needBits);
					aK |= aK >> (4*needBits);
					*cube++ = aK+0x08000;
				}
			}
		}
		for( i=1; i<gridPoints; ++i){
			aI = (UINT16)(i<< aShift);
			aI |= aI >> needBits;
			aI |= aI >> (2*needBits);
			aI |= aI >> (4*needBits);
			aI += 0x08000;
			for( j=0; j<=gridPoints; ++j){
				aJ = (UINT16)(j<< aShift);
				for( k=0; k<=gridPoints; ++k){
					*cube++ = aI;
					*cube++ = aJ;
					aK = (UINT16)(k<< aShift);
					*cube++ = aK;
				}
				for( k=1; k<gridPoints; ++k){
					*cube++ = aI;
					*cube++ = aJ;
					aK = (UINT16)(k<< aShift);
					aK |= aK >> needBits;
					aK |= aK >> (2*needBits);
					aK |= aK >> (4*needBits);
					*cube++ = aK+0x08000;
				}
			}
			for( j=1; j<gridPoints; ++j){
				aJ = (UINT16)(j<< aShift);
				aJ |= aJ >> needBits;
				aJ |= aJ >> (2*needBits);
				aJ |= aJ >> (4*needBits);
				aJ += 0x08000;
				for( k=0; k<=gridPoints; ++k){
					*cube++ = aI;
					*cube++ = aJ;
					aK = (UINT16)(k<< aShift);
					*cube++ = aK;
				}
				for( k=1; k<gridPoints; ++k){
					*cube++ = aI;
					*cube++ = aJ;
					aK = (UINT16)(k<< aShift);
					aK |= aK >> needBits;
					aK |= aK >> (2*needBits);
					aK |= aK >> (4*needBits);
					*cube++ = aK+0x08000;
				}
			}
		}
	}
	UNLOCK_DATA(tempCube);
	*theCube = tempCube;
CleanupAndExit:
	LH_END_PROC("MakeSpecialCube16")
	return err;
}
#endif

/* ———————————————————————————————————————————————————————————————————————————————————————————
	CMError
	CreateCombi	( CMMModelHandle    	modelingData,
				  CMConcatProfileSet* 	profileSet,
				  LHConcatProfileSet*	newProfileSet,
				  CMLutParam*			finalLutData,	
				  Boolean				createGamutLut )
   ——————————————————————————————————————————————————————————————————————————————————————————— */
CMError
CreateCombi	( CMMModelPtr	    	modelingData,
			  CMConcatProfileSet* 	profileSet,
			  LHConcatProfileSet*	newProfileSet,
			  CMLutParamPtr			finalLutData,	
			  Boolean				createGamutLut )
{
	icHeader				profHeader;
	CMLutParam				aDoNDimTableData;
	DoMatrixForCubeStruct	aDoMatrixForCubeStruct;
	CMCalcParam 			calcParam;
			
   	CMError				err = noErr;
   	OSErr				aOSerr = noErr;
	CUBE_DATA_TYPE		inputBuffer	 = nil;
	CUBE_DATA_TYPE		outputBuffer = nil;

	SINT32				loop;
	SINT32				theSize;
	SINT32				theElutSize;
	SINT32				theAlutSize;
	SINT32				theCubeSize;
	SINT32				theExtraSize=1;
	SINT32				theCubePixelCount;
	UINT16				profLoop;
	SINT32				savedGridPoints;
	SINT32				gridBits;
	SINT32				gridPointsCube;
	Boolean 			SavedoCreate_16bit_XLut;
	Boolean 			SavedoCreate_16bit_ALut;
	void					*SaveoutputLut;
	long				SaveoutputLutEntryCount;
	long				SaveoutputLutWordSize;
	/*long				SavegridPointsCube;*/
	UINT8	 			bIsLabConnection = 0;
	
	SINT32				numOfElutsCube;

	Boolean				doSaveElut		= TRUE;
	Boolean 			skipCombi		= FALSE;
	Boolean 			pcsConversion	= FALSE;
	Boolean 			useOutputBuffer;
	LHCombiData			theCombiData;
	CMLutParam			theLutData;	
	SINT32				theBufferByteCount;
	UINT16				aUINT16;
	UINT16				*wordPtr   = nil;
	UINT8				*xlutPtr   = nil;
#ifdef DoLabSpecial
	UINT32				aFac;
	UINT32				i;
#endif

#ifdef WRITE_LUTS
	Str255				fileString;
#endif
#ifdef DEBUG_OUTPUT
	Str255				lutString;
#endif
	LH_START_PROC("CreateCombi")

	theBufferByteCount = 2;
	SetMem(&theCombiData,	sizeof(LHCombiData), 0);
	SetMem(&theLutData, 	sizeof(CMLutParam),	 0);
	
	theCombiData.amIPCS = FALSE;			/* amIPCS has to be FALSE at the beginning of the chain */
	if ( modelingData->hasNamedColorProf == NamedColorProfileAtBegin ){
		theCombiData.amIPCS = TRUE;
	}
	if (newProfileSet->count == 1)
		skipCombi = TRUE;
	
	err = CMGetProfileHeader(profileSet->profileSet[0], (CMCoreProfileHeader *)&profHeader);
	if (err)
		goto CleanupAndExit;
	modelingData->precision = (SINT16)((profHeader.flags & kQualityMask)>>16);
	
#ifdef RenderInt
	if( modelingData-> dwFlags != 0xffffffff ){
		modelingData->precision = (short)( modelingData->dwFlags & 0x0ffff);
	}
#endif
	if ( modelingData->precision >= cmBestMode )		/* first fix - best mode creates 16-bit luts */
	{
		theCombiData.doCreate_16bit_Combi = TRUE;
		modelingData->precision = cmBestMode;
	}
	else
	{
      theCombiData.doCreate_16bit_Combi = FALSE;
  	}
#ifdef DEBUG_OUTPUT
	if ( DebugCheck(kThisFile, kDebugMiscInfo) )
		DebugPrint("  CMMModelHandle->precision = %d doCreate_16bit_Combi=%d\n",modelingData->precision,theCombiData.doCreate_16bit_Combi);
#endif
	
	switch (modelingData->precision)
	{
		case cmNormalMode:
			if (createGamutLut)
				theCubePixelCount = 87382;
			else
				theCubePixelCount = 87382;
				if( modelingData->lookup )theCubePixelCount = 87382;
				break;
		case cmDraftMode:
			if (createGamutLut)
				theCubePixelCount = 5462;
			else
				theCubePixelCount = 5462;		/* 5462 * 3 ≈ 4 * 8 * 8 * 8 * 8	 	->  allow 8^4 */
			break;
		case cmBestMode:
			if (createGamutLut)
				theCubePixelCount = 87382;
			else
				theCubePixelCount = 87382;   /* 87382 * 3 ≈ 4 * 16 * 16 * 16 * 16;	->  allow 16^4  !!! */
			break;
	}
	theCubeSize = theCubePixelCount * 3;
	theCombiData.precision 			 = modelingData->precision;
	theCombiData.maxProfileCount 	 = newProfileSet->count-1;
	theCombiData.doCreateLinkProfile = (modelingData->currentCall == kCMMNewLinkProfile);
	switch ( modelingData->firstColorSpace )
	{
		case icSigXYZData:
		case icSigLabData:
		case icSigLuvData:
		case icSigYxyData:
		case icSigRgbData:
		case icSigHsvData:
		case icSigHlsData:
		case icSigCmyData:
#ifdef PI_Application_h
		case icSigYCbCrData:
		case icSigMCH3Data:
		case icSigNamedData:
#endif
			err = CalcGridPoints4Cube ( theCubeSize, 3, &theCombiData.gridPointsCube, &gridBits );		/* 3 input channels */
			break;
		case icSigGrayData:
			theCubeSize = 255 ;
			err = CalcGridPoints4Cube ( theCubeSize, 1, &theCombiData.gridPointsCube, &gridBits );		/* 1 input channel  */
			break;
		case icSigCmykData:
		case icSigMCH4Data:
			err = CalcGridPoints4Cube ( theCubeSize, 4, &theCombiData.gridPointsCube, &gridBits );		/* 4 input channels */
			break;
		case icSigMCH2Data:
			err = CalcGridPoints4Cube ( theCubeSize, 2, &theCombiData.gridPointsCube, &gridBits );		/* 2 input channels */
			break;
		case icSigMCH5Data:
			err = CalcGridPoints4Cube ( theCubeSize, 5, &theCombiData.gridPointsCube, &gridBits );		/* 5 input channels */
			break;
		case icSigMCH6Data:
			err = CalcGridPoints4Cube ( theCubeSize, 6, &theCombiData.gridPointsCube, &gridBits );		/* 6 input channels */
			break;
		case icSigMCH7Data:
			err = CalcGridPoints4Cube ( theCubeSize, 7, &theCombiData.gridPointsCube, &gridBits );		/* 7 input channels */
			break;
		case icSigMCH8Data:
			err = CalcGridPoints4Cube ( theCubeSize, 8, &theCombiData.gridPointsCube, &gridBits );		/* 8 input channels */
			break;
	}

	bIsLabConnection = 0;
	if( profHeader.pcs == icSigXYZData ){
		for ( profLoop = 0; profLoop < newProfileSet->count; profLoop++ ){
			if( newProfileSet->prof[profLoop].profileSet == 0 ){
				bIsLabConnection |= 1;
			}
		}
	}
	err = CMGetProfileHeader(profileSet->profileSet[profileSet->count-1], (CMCoreProfileHeader *)&profHeader);
	if (err)
		goto CleanupAndExit;
	if( profHeader.pcs == icSigXYZData ){
		for ( profLoop = 0; profLoop < newProfileSet->count; profLoop++ ){
			if( newProfileSet->prof[profLoop].profileSet == 0 && !createGamutLut ){
				bIsLabConnection |=2;
			}
		}
	}
	/*bIsLabConnection = 0;*/
	if( modelingData->hasNamedColorProf == NamedColorProfileAtBegin ){
		bIsLabConnection |=1;
		theCombiData.doCreate_16bit_Combi = TRUE;
	}
		
	if( modelingData->hasNamedColorProf == NamedColorProfileAtEnd ){
		bIsLabConnection |=2;
		theCombiData.doCreate_16bit_Combi = TRUE;
	}
		
	/*--------------------------------------------------------------------------------------------------
	  --                                  loop over all profiles
	  --------------------------------------------------------------------------------------------------*/
	for ( profLoop = 0; profLoop < newProfileSet->count; profLoop++ )
	{
#ifdef DEBUG_OUTPUT
		if ( DebugCheck(kThisFile, kDebugMiscInfo) )
			DebugPrint("  <————————————————————— Processing profile #%d —————————————————————>\n",profLoop);
#endif
		theCombiData.profLoop = (long)profLoop;
		if ( theCombiData.doCreateLinkProfile )
		{
			theCombiData.doCreate_16bit_ELut = TRUE;
            theCombiData.doCreate_16bit_XLut = TRUE; /* UWE 9.2.96 */
			theCombiData.doCreate_16bit_ALut = TRUE;
		} else
		{
			theCombiData.doCreate_16bit_ELut = ( profLoop != 0 );
            theCombiData.doCreate_16bit_XLut = theCombiData.doCreate_16bit_Combi;	/* UWE 9.2.96 */
			theCombiData.doCreate_16bit_ALut = ( profLoop != newProfileSet->count-1 );
		}
		if (newProfileSet->prof[profLoop].profileSet)
		{
			theCombiData.theProfile = newProfileSet->prof[profLoop].profileSet;
			err = CMGetProfileHeader(theCombiData.theProfile, (CMCoreProfileHeader *)&profHeader);
			if (err)
				goto CleanupAndExit;
			theCombiData.profileClass			= profHeader.deviceClass;
			theCombiData.dataColorSpace 		= profHeader.colorSpace;
			theCombiData.profileConnectionSpace	= profHeader.pcs;
			theCombiData.renderingIntent 		= newProfileSet->prof[profLoop].renderingIntent;
			pcsConversion = FALSE;
		}
		else{
			pcsConversion = TRUE;
		}
		if ( (err == noErr) || pcsConversion ){
			/*------------------------ free pointers... */
			theLutData.inputLut 	= DISPOSE_IF_DATA(theLutData.inputLut);
			theLutData.outputLut 	= DISPOSE_IF_DATA(theLutData.outputLut);
			theLutData.colorLut 	= DISPOSE_IF_DATA(theLutData.colorLut);
			theLutData.matrixMFT 	= DisposeIfPtr(theLutData.matrixMFT);
			theLutData.matrixTRC 	= DisposeIfPtr(theLutData.matrixTRC);
			
			if (pcsConversion)
			{
				/*——————————————————————————————————————————————————————————————————————————————————————
				  	pcsConversion: we have to convert:     XYZ <-->  Lab
				  ——————————————————————————————————————————————————————————————————————————————————————*/
				if (inputBuffer == nil)
				{
					theLutData.colorLutInDim = 3;
#ifdef DoLabSpecial
					if( modelingData->firstColorSpace == icSigLabData || modelingData->firstColorSpace == icSigLuvData ){
						err = MakeSpecialCube16(theLutData.colorLutInDim, &theCubeSize, &theLutData.colorLut, &gridBits, &theExtraSize );
					}
					else
#endif
					err = MakeCube16(theLutData.colorLutInDim, &theCubeSize, &theLutData.colorLut, &gridBits, &theExtraSize );
#ifdef DEBUG_OUTPUT
					if ( DebugCheck(kThisFile, kDebugMiscInfo) )
						DebugPrint("  MakeCube16 -> '%d' bits    cubeSize = %d\n",gridBits, theCubeSize);
#endif
					if (err)
						goto CleanupAndExit;
					theCubePixelCount = theCubeSize / theLutData.colorLutInDim;
					numOfElutsCube = theLutData.colorLutInDim;
					gridPointsCube = 1<<gridBits;
					savedGridPoints = gridPointsCube;
					if( profLoop == 0 ){
						theLutData.inputLutEntryCount = 1<<adr_breite_elut;
						if( theCombiData.doCreate_16bit_ELut ) theLutData.inputLutWordSize = VAL_USED_BITS;
						else  theLutData.inputLutWordSize = bit_breite_elut;
						theLutData.colorLutGridPoints = gridPointsCube;	
					}

				}
				else{
					theLutData.colorLut = inputBuffer;
					inputBuffer = nil;
				}
				
				LOCK_DATA(theLutData.colorLut);
#ifdef DEBUG_OUTPUT
				ShowCube16( profLoop, "Lab<->XYZ", createGamutLut, (UINT16 *)DATA_2_PTR(theLutData.colorLut), gridPointsCube, theLutData.colorLutInDim, 3,VAL_USED_BITS );
#endif
				/*if ( modelingData->hasNamedColorProf == NamedColorProfileAtBegin ){
					inputBuffer = theLutData.colorLut;
					theLutData.colorLut = 0;
					QuantizeNamedValues( modelingData, inputBuffer, theCubeSize/theLutData.colorLutInDim );
					theCombiData.amIPCS = TRUE;
#ifdef DEBUG_OUTPUT
					ShowCube16( profLoop, "after Lab<->XYZ", createGamutLut, (UINT16 *)DATA_2_PTR(theLutData.colorLut), gridPointsCube, theLutData.colorLutInDim, 3, VAL_USED_BITS );
#endif
					UNLOCK_DATA(theLutData.colorLut);
					skipCombi = FALSE;
					theLutData.colorLutInDim = kNumOfLab_XYZchannels;
					theLutData.colorLutOutDim = kNumOfLab_XYZchannels;
					continue;
				}
				else */
				if ( newProfileSet->prof[profLoop].pcsConversionMode == kDoLab2XYZ )
				{
#ifdef DEBUG_OUTPUT
					if ( DebugCheck(kThisFile, kDebugMiscInfo) )
						DebugPrint("  doing a PCS conversion:  Lab -> XYZ\n");
#endif
					Lab2XYZ_forCube16((UINT16*)DATA_2_PTR(theLutData.colorLut), theCubePixelCount);
				}
				else if ( newProfileSet->prof[profLoop].pcsConversionMode == kDoXYZ2Lab )
				{
#ifdef DEBUG_OUTPUT
					if ( DebugCheck(kThisFile, kDebugMiscInfo) )
						DebugPrint("  doing a PCS conversion:  XYZ -> Lab\n");
#endif
					XYZ2Lab_forCube16((UINT16*)DATA_2_PTR(theLutData.colorLut), theCubePixelCount);
				}
#ifdef DEBUG_OUTPUT
				ShowCube16( profLoop, "after Lab<->XYZ", createGamutLut, (UINT16 *)DATA_2_PTR(theLutData.colorLut), gridPointsCube, theLutData.colorLutInDim, 3, VAL_USED_BITS );
#endif
				UNLOCK_DATA(theLutData.colorLut);
				skipCombi = TRUE;
				theLutData.colorLutInDim = kNumOfLab_XYZchannels;
				theLutData.colorLutOutDim = kNumOfLab_XYZchannels;
			} else
			{
				/*——————————————————————————————————————————————————————————————————————————————————————
				  	NO pcsConversion: we are dealing with a profile
				  ——————————————————————————————————————————————————————————————————————————————————————*/
				theCombiData.doCreateGamutLut = createGamutLut && (profLoop == newProfileSet->count - 1);
				theCombiData.usePreviewTag 	  = (UINT8)newProfileSet->prof[profLoop].usePreviewTag;				
				
				/*----------------------------------------------------------------------------------------------
				   get data out of the profile
				  ----------------------------------------------------------------------------------------------*/

				if( bIsLabConnection & 1 ){
					if ( theCombiData.maxProfileCount > 0 ){
						theCombiData.maxProfileCount = 0;
						SavedoCreate_16bit_XLut = theCombiData.doCreate_16bit_XLut;
						theCombiData.doCreate_16bit_XLut = TRUE;
					}
				}
				if( bIsLabConnection & 2 ){
					SavedoCreate_16bit_ALut = theCombiData.doCreate_16bit_ALut;
					theCombiData.doCreate_16bit_ALut = TRUE;
				}
				err = ExtractAllLuts (&theLutData, &theCombiData);
				if( newProfileSet->count == 1 ){
					gridPointsCube = theLutData.colorLutGridPoints;
				}

				if( bIsLabConnection & 1 ){
					theCombiData.maxProfileCount = newProfileSet->count-1;
					if ( theCombiData.maxProfileCount > 0 ){
						theCombiData.doCreate_16bit_XLut = SavedoCreate_16bit_XLut;
					}
				}
				if( bIsLabConnection & 2 ){
					theCombiData.doCreate_16bit_ALut = SavedoCreate_16bit_ALut;
				}
				
				if (err)
					goto CleanupAndExit;
				if ( (theLutData.colorLutInDim == 0) || (theLutData.colorLutOutDim == 0))
				{
					err = cmProfileError;
					goto CleanupAndExit;
				}
			}
#ifdef DEBUG_OUTPUT
			if ( DebugCheck(kThisFile, kDebugEXASizeInfo) )
			{
				if (theLutData.inputLut)
					DebugPrint("  Elut = %6d\n", (theLutData.inputLutEntryCount*theLutData.colorLutInDim*(theLutData.inputLutWordSize>8?2:1)));
				if (theLutData.colorLut)
					DebugPrint("  Xlut colorLutGridPoints = %6d\n       colorLutInDim      = %6d\n       colorLutOutDim     = %6d\n       colorLutWordSize   = %6d\n",
								theLutData.colorLutGridPoints,theLutData.colorLutInDim,theLutData.colorLutOutDim,theLutData.colorLutWordSize);
				if (theLutData.outputLut)
					DebugPrint("  Alut = %6d ", (theLutData.outputLutEntryCount*theLutData.colorLutOutDim*(theLutData.outputLutWordSize>8?2:1)));
				DebugPrint("\n");
			}
#endif
			if (theLutData.matrixMFT)
				skipCombi = FALSE;
			/*----------------------------------------------------------------------------------------------
			   save first Elut...
			  ----------------------------------------------------------------------------------------------*/
			if (doSaveElut)	
			{
				if ( skipCombi )
					savedGridPoints = theLutData.colorLutGridPoints;
				else
				{
					if (theLutData.colorLutInDim == 1)
					{
						theCubeSize = 255 ;
					}
					/*------------------------create and initialize cube*/
#ifdef DoLabSpecial
					if( modelingData->firstColorSpace == icSigLabData || modelingData->firstColorSpace == icSigLuvData ){
						err = MakeSpecialCube16(theLutData.colorLutInDim, &theCubeSize, &inputBuffer, &gridBits, &theExtraSize );
					}
					else
#endif
					err = MakeCube16(theLutData.colorLutInDim, &theCubeSize, &inputBuffer, &gridBits, &theExtraSize);
#ifdef DEBUG_OUTPUT
					if ( DebugCheck(kThisFile, kDebugMiscInfo) )
						DebugPrint("  MakeCube16 -> '%d' bits    cubeSize = %d\n",gridBits, theCubeSize);
#endif
					if (err)
						goto CleanupAndExit;
					theCubePixelCount = theCubeSize / theLutData.colorLutInDim;
					numOfElutsCube = theLutData.colorLutInDim;
					gridPointsCube = 1<<gridBits;
					savedGridPoints = gridPointsCube;
				}
				
				/*theSize = GETDATASIZE(theLutData.inputLut);	*/			/* save final ELUT */
				theSize = theLutData.inputLutEntryCount * theLutData.colorLutInDim;				/* save final ELUT */
				if ( theLutData.inputLutWordSize > 8 )
					theSize *= 2;
				
				if( bIsLabConnection & 1 ){
					
					finalLutData->inputLut = ALLOC_DATA(theSize+2, &aOSerr);
					err = aOSerr;
					if (err)
						goto CleanupAndExit;
					
					/*------------------------------------------------------------------------------------------ create linear elut*/
					theElutSize = theSize / theLutData.colorLutInDim;
					LOCK_DATA(finalLutData->inputLut);
					if( theCombiData.doCreate_16bit_ELut || theCombiData.doCreate_16bit_Combi){
						CreateLinearElut16 ( (Ptr)DATA_2_PTR(finalLutData->inputLut), theElutSize/ sizeof (UINT16), gridPointsCube, 0 );
						finalLutData->inputLutWordSize = VAL_USED_BITS;
					}
					else{
						CreateLinearElut ( (Ptr)DATA_2_PTR(finalLutData->inputLut), theElutSize/ sizeof (UINT16), gridPointsCube, 0 );
						finalLutData->inputLutWordSize = bit_breite_elut;
					}
					for (loop = 0; loop < theLutData.colorLutInDim; loop++)
						BlockMoveData(DATA_2_PTR(finalLutData->inputLut), (Ptr)DATA_2_PTR(finalLutData->inputLut) + loop * theElutSize, theElutSize);
					finalLutData->inputLutEntryCount = theElutSize/sizeof(UINT16);
					UNLOCK_DATA(finalLutData->inputLut);
					finalLutData->colorLutInDim		 = theLutData.colorLutInDim;
					bIsLabConnection &= ~1;
				}
				else{
					finalLutData->inputLutWordSize	 = theLutData.inputLutWordSize;
					finalLutData->inputLut 			 = theLutData.inputLut;
					finalLutData->inputLutEntryCount = theLutData.inputLutEntryCount;
					finalLutData->colorLutInDim		 = theLutData.colorLutInDim;
					
					theLutData.inputLut = ALLOC_DATA(theSize+2, &aOSerr);
					err = aOSerr;
					if (err)
						goto CleanupAndExit;
					
					/*------------------------------------------------------------------------------------------ create linear elut*/
					theElutSize = theSize / theLutData.colorLutInDim;
					LOCK_DATA(theLutData.inputLut);
					CreateLinearElut16 ( (Ptr)DATA_2_PTR(theLutData.inputLut), theElutSize/ sizeof (UINT16), theLutData.colorLutGridPoints, 0);
					for (loop = 0; loop < theLutData.colorLutInDim; loop++)
						BlockMoveData(DATA_2_PTR(theLutData.inputLut), (Ptr)DATA_2_PTR(theLutData.inputLut) + loop * theElutSize, theElutSize);
					theLutData.inputLutEntryCount = theElutSize/sizeof(UINT16);
					theLutData.inputLutWordSize = VAL_USED_BITS;
					UNLOCK_DATA(theLutData.inputLut);
				}
				doSaveElut = FALSE;
#ifdef DoLabSpecial
				if( (modelingData->firstColorSpace == icSigLabData || modelingData->firstColorSpace == icSigLuvData) ){
					LOCK_DATA(finalLutData->inputLut);
					wordPtr = (UINT16 *)DATA_2_PTR( finalLutData->inputLut );
					aFac = (((1<<15)*((UINT32)finalLutData->inputLutEntryCount-1)*(UINT32)gridPointsCube + finalLutData->inputLutEntryCount/2)/
							(UINT32)finalLutData->inputLutEntryCount + gridPointsCube/2)/((UINT32)gridPointsCube-1);
					i = finalLutData->inputLutEntryCount/2 - 1;
					for( aUINT16 = 0; aUINT16 < (UINT16)finalLutData->colorLutInDim; aUINT16++){
						for( loop = 0; loop <= (SINT32)(i+1); loop++){
							*wordPtr++ = (UINT16)((*wordPtr * aFac + (1<<(15-1)) )>>15);
						}

						for( loop = 1; loop <= (SINT32)i; loop++){
							*wordPtr++ = (UINT16)((*wordPtr * ((aFac *( i - loop ) + (1<<15) * loop + i/2)/i) + (1<<(15-1)) )>>15);
						}
					}
					UNLOCK_DATA(finalLutData->inputLut);
				}	
#endif			
			}
			
			/*----------------------------------------------------------------------------------------------
			   save last Alut...
			  ----------------------------------------------------------------------------------------------*/
			if (profLoop == newProfileSet->count - 1)
			{
				/*theSize = GETDATASIZE(theLutData.outputLut);*/
				theSize = theLutData.outputLutEntryCount * theLutData.colorLutOutDim;				/* save final ALUT */
				if ( theLutData.outputLutWordSize > 8 )
					theSize *= 2;
				if( bIsLabConnection & 2 ){
					SaveoutputLut = theLutData.outputLut;
					SaveoutputLutEntryCount = theLutData.outputLutEntryCount;
					SaveoutputLutWordSize = theLutData.outputLutWordSize;
				}
				else{
					finalLutData->outputLut = theLutData.outputLut;
					finalLutData->outputLutEntryCount = theLutData.outputLutEntryCount;
					finalLutData->outputLutWordSize = theLutData.outputLutWordSize;
				}
				finalLutData->colorLutOutDim = theLutData.colorLutOutDim;

				theAlutSize = (1<<adr_breite_alut);
                if (theCombiData.doCreate_16bit_ALut || theCombiData.doCreate_16bit_Combi)/* UWE 9.2.96*/
				{
					theSize = sizeof(UINT16);;
					theLutData.outputLut = ALLOC_DATA(theLutData.colorLutOutDim * theAlutSize * theSize+theSize, &aOSerr);
					err = aOSerr;
					if (err)
						goto CleanupAndExit;
					LOCK_DATA(theLutData.outputLut);
					CreateLinearAlut16 ( (UINT16 *)DATA_2_PTR(theLutData.outputLut), theAlutSize);
					for (loop = 1; loop < theLutData.colorLutOutDim; loop++)
						BlockMoveData(DATA_2_PTR(theLutData.outputLut), (Ptr)DATA_2_PTR(theLutData.outputLut) + loop * theAlutSize * sizeof(UINT16), theAlutSize * sizeof(UINT16));
					theLutData.outputLutEntryCount = (SINT16)theAlutSize;
					theLutData.outputLutWordSize = VAL_USED_BITS;
					UNLOCK_DATA(theLutData.outputLut);
				}
				else
				{
					theSize = sizeof(UINT8);;
					theLutData.outputLut = ALLOC_DATA(theLutData.colorLutOutDim * theAlutSize * theSize+theSize, &aOSerr);
					err = aOSerr;
					if (err)
						goto CleanupAndExit;
					LOCK_DATA(theLutData.outputLut);

					CreateLinearAlut( (UINT8*)DATA_2_PTR(theLutData.outputLut), theAlutSize );
					for (loop = 1; loop < theLutData.colorLutOutDim; loop++)
						BlockMoveData(DATA_2_PTR(theLutData.outputLut), (Ptr)DATA_2_PTR(theLutData.outputLut) + loop * theAlutSize, theAlutSize);
					theLutData.outputLutEntryCount = theAlutSize;
					theLutData.outputLutWordSize = bit_breite_alut;
					UNLOCK_DATA(theLutData.outputLut);
					theBufferByteCount = 1;			/* last should be Byte ALUT*/
				}
				if( bIsLabConnection & 2 ){
					bIsLabConnection &= ~2;
					finalLutData->outputLut = theLutData.outputLut;
					finalLutData->outputLutEntryCount = theLutData.outputLutEntryCount;
					finalLutData->outputLutWordSize = theLutData.outputLutWordSize;
					theLutData.outputLut = SaveoutputLut;
					theLutData.outputLutEntryCount = SaveoutputLutEntryCount;
					theLutData.outputLutWordSize = SaveoutputLutWordSize;
				}
			}
			
			if (skipCombi)
			{
				/*=============================================================================================
				  	we have either only one profile -or- we did do a pcsConversion
				  =============================================================================================*/
				if (newProfileSet->count == 1){			/* one profile */
					theSize = 1;
					theExtraSize = 1;
					for( loop=0; loop<(theLutData.colorLutInDim-1); ++loop){	/* Extra Size for Interpolation */
						theSize *=theLutData.colorLutGridPoints ;
						theExtraSize += theSize;
					}
					theSize = 1;
					for( loop=0; loop<theLutData.colorLutInDim; ++loop){
						theSize *= theLutData.colorLutGridPoints;
					}
					theSize *= theLutData.colorLutOutDim;
#ifdef ALLOW_MMX
					theExtraSize++;	/* +1 for MMX 4 Byte access */
#endif
					theExtraSize *= theLutData.colorLutOutDim;
					inputBuffer = DISPOSE_IF_DATA(inputBuffer);
					if( theCombiData.doCreate_16bit_Combi ){
						inputBuffer = ALLOC_DATA( (theSize+theExtraSize) * 2, &aOSerr );
						err = aOSerr;
						if (err)
							goto CleanupAndExit;

						LOCK_DATA(theLutData.colorLut);
						LOCK_DATA(inputBuffer);
						wordPtr = (UINT16 *)DATA_2_PTR( inputBuffer );
						xlutPtr = (UINT8 *)DATA_2_PTR( theLutData.colorLut );
						if( theLutData.colorLutWordSize == 8 ){
							for( loop=0; loop<theSize; ++loop){
								aUINT16 = (UINT16)xlutPtr[loop];
								wordPtr[loop] = (aUINT16<<8) | aUINT16;
							}
						}
						else{
							BlockMoveData( xlutPtr, wordPtr, theSize * 2 );
						}
						UNLOCK_DATA(theLutData.colorLut);
						UNLOCK_DATA(inputBuffer);
						theLutData.colorLut = inputBuffer;
						theLutData.colorLutWordSize = 16;
						inputBuffer = 0;
					}
					else{
						inputBuffer = ALLOC_DATA( (theSize+theExtraSize), &aOSerr );
						err = aOSerr;
						if (err)
							goto CleanupAndExit;

						LOCK_DATA(theLutData.colorLut);
						LOCK_DATA(inputBuffer);
						wordPtr = (UINT16 *)DATA_2_PTR( inputBuffer );
						xlutPtr = (UINT8 *)DATA_2_PTR( theLutData.colorLut );
						BlockMoveData( xlutPtr, wordPtr, theSize );
						UNLOCK_DATA(theLutData.colorLut);
						UNLOCK_DATA(inputBuffer);
						DISPOSE_IF_DATA( theLutData.colorLut );
						theLutData.colorLut = inputBuffer;
						theLutData.colorLutWordSize = 8;
						inputBuffer = 0;
					}
/*					if( theCombiData.doCreate_16bit_Combi ){
						if( theLutData.colorLutWordSize == 8 ){
							theSize = 1;
							for( loop=0; loop<theLutData.colorLutInDim; ++loop){
								theSize *= theLutData.colorLutGridPoints;
							}
                    		theSize = theSize *  theLutData.colorLutOutDim;
							inputBuffer = DISPOSE_IF_DATA(inputBuffer);
							inputBuffer = ALLOC_DATA( theSize * 2+2, &aOSerr );
							err = aOSerr;
							if (err)
								goto CleanupAndExit;
							
							LOCK_DATA(theLutData.colorLut);
							LOCK_DATA(inputBuffer);
							wordPtr = DATA_2_PTR( inputBuffer );
							xlutPtr = DATA_2_PTR( theLutData.colorLut );
							for( loop=0; loop<theSize; ++loop){
								aUINT16 = (UINT16)xlutPtr[loop];
								wordPtr[loop] = (aUINT16<<8) | aUINT16;
							}
							UNLOCK_DATA(theLutData.colorLut);
							UNLOCK_DATA(inputBuffer);
							theLutData.colorLut = inputBuffer;
							theLutData.colorLutWordSize = 16;
							inputBuffer = 0;
						}
					}*/
				}
				inputBuffer = DISPOSE_IF_DATA(inputBuffer);
				inputBuffer = theLutData.colorLut;
				if (inputBuffer){
					theCubeSize = theCubePixelCount *  theLutData.colorLutInDim;
                    theCubeSize *= theLutData.colorLutOutDim;
				}
				theLutData.colorLut = 0;
				if ( (profLoop == newProfileSet->count - 1) && ( ! theCombiData.doCreate_16bit_ALut  && !theCombiData.doCreate_16bit_Combi) )/* UWE 9.2.96*/
					theBufferByteCount = 1;			/* last should be Byte ALUT*/

			} else
			{
				/*=============================================================================================
				  	create combi
				  =============================================================================================*/
				if (theLutData.matrixTRC)
				{
					/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
					   the profile contained a matrix/TRC:
					  +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
					LOCK_DATA(theLutData.inputLut);
					LOCK_DATA(theLutData.outputLut);
					LOCK_DATA(inputBuffer);
#ifdef DEBUG_OUTPUT
					if ( DebugCheck(kThisFile, kDebugMiscInfo) )
					{
						DebugPrint("• matrixTRC: calling DoMatrixForCube16  (gridPointsCube = %d   inputBuffer = %d)\n", gridPointsCube, theCubeSize);
						DebugPrint("  theLutData.colorLutGridPoints = %d\n", theLutData.colorLutGridPoints);
						DebugPrint("     %f    %f    %f\n     %f    %f    %f\n     %f    %f    %f\n",
								(*(Matrix2D *)theLutData.matrixTRC)[0][0],(*(Matrix2D *)theLutData.matrixTRC)[0][1],(*(Matrix2D *)theLutData.matrixTRC)[0][2],
								(*(Matrix2D *)theLutData.matrixTRC)[1][0],(*(Matrix2D *)theLutData.matrixTRC)[1][1],(*(Matrix2D *)theLutData.matrixTRC)[1][2],
								(*(Matrix2D *)theLutData.matrixTRC)[2][0],(*(Matrix2D *)theLutData.matrixTRC)[2][1],(*(Matrix2D *)theLutData.matrixTRC)[2][2]);
					}
					if ( createGamutLut )
					{
						if (DebugLutCheck(kDisplayEXAGamut) )
						{
							lutString[0] = sprintf((SINT8*)&lutString[1], "Gamut-E Lut #%d @ matrixTRC", profLoop);
							DoDisplayLutNew(lutString,&theLutData, 0);
							lutString[0] = sprintf((SINT8*)&lutString[1], "Gamut-A Lut #%d @ matrixTRC", profLoop);
							DoDisplayLutNew(lutString,&theLutData, 2);
						}
					} else if (DebugLutCheck(kDebugEXAReal))
					{
						
						lutString[0] = sprintf((SINT8*)&lutString[1], "E Lut #%d @ matrixTRC", profLoop);
						DoDisplayLutNew(lutString,&theLutData, 0);
						lutString[0] = sprintf((SINT8*)&lutString[1], "A Lut #%d @ matrixTRC", profLoop);
						DoDisplayLutNew(lutString,&theLutData, 2);
					}
#endif
#ifdef WRITE_LUTS
					if ( !createGamutLut )
					{
						fileString[0] = sprintf((SINT8*)&fileString[1], "E Lut #%d @ matrixTRC", profLoop);
						WriteLut2File( fileString,theLutData.inputLut, 'ELUT');
						fileString[0] = sprintf((SINT8*)&fileString[1], "A Lut #%d @ matrixTRC", profLoop);
						WriteLut2File( fileString,theLutData.outputLut, 'ALUT');
					}
#endif
					aDoMatrixForCubeStruct.aElutAdrSize 	= theLutData.inputLutEntryCount;
					aDoMatrixForCubeStruct.aElutAdrShift	= 0;
					aDoMatrixForCubeStruct.aElutWordSize 	= theLutData.inputLutWordSize;
					aDoMatrixForCubeStruct.separateEluts	= TRUE;
					aDoMatrixForCubeStruct.ein_lut	 		= (UINT16 *)DATA_2_PTR(theLutData.inputLut);
					aDoMatrixForCubeStruct.aAlutAdrSize 	= theLutData.outputLutEntryCount;
					aDoMatrixForCubeStruct.aAlutAdrShift 	= 0;
					aDoMatrixForCubeStruct.aAlutWordSize 	= theLutData.outputLutWordSize;
					aDoMatrixForCubeStruct.separateAluts	= TRUE;
					aDoMatrixForCubeStruct.aus_lut	 		= (UINT8 *)DATA_2_PTR(theLutData.outputLut);
					aDoMatrixForCubeStruct.theMatrix		= (Matrix2D	*)theLutData.matrixTRC;
					aDoMatrixForCubeStruct.aPointCount 		= theCubePixelCount;
					aDoMatrixForCubeStruct.gridPoints 		= gridPointsCube;
					aDoMatrixForCubeStruct.aBufferByteCount = theBufferByteCount;
					aDoMatrixForCubeStruct.theArr		 	= (UINT8 *)DATA_2_PTR(inputBuffer);
#ifdef DEBUG_OUTPUT
					ShowCube16( profLoop, "DoMatrixForCube16", createGamutLut, (UINT16 *)DATA_2_PTR(inputBuffer), gridPointsCube, numOfElutsCube, 3, VAL_USED_BITS );
#endif
#ifdef WRITE_LUTS
					if ( !createGamutLut )
					{
						fileString[0] = sprintf((SINT8*)&fileString[1], "DoMat4Cube #%d (TRC)", profLoop);
						WriteLut2File( fileString,inputBuffer, 'XLUT');
					}
#endif
					DoMatrixForCube16( &aDoMatrixForCubeStruct );
#ifdef DEBUG_OUTPUT
					ShowCube16( profLoop, "after DoMatrixForCube16", createGamutLut, (UINT16 *)DATA_2_PTR(inputBuffer), gridPointsCube, numOfElutsCube, 3, 8*theBufferByteCount );
#endif
#ifdef WRITE_LUTS
					if ( !createGamutLut )
					{
						fileString[0] = sprintf((SINT8*)&fileString[1], "after DoMat4Cube #%d (TRC)", profLoop);
						WriteLut2File( fileString,inputBuffer, 'XLUT');
					}
#endif
					
					UNLOCK_DATA(inputBuffer);
					UNLOCK_DATA(theLutData.inputLut);
					UNLOCK_DATA(theLutData.outputLut);
					/*SETDATASIZE(inputBuffer, theCubePixelCount * theLutData.colorLutOutDim * theBufferByteCount);*/
				} else
				{
					/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
					   the profile contained a mft
					  +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
					
					/*----------------------------------------------------------------------------------------- absolute colorimetry*/
					if ( theCombiData.amIPCS && newProfileSet->prof[profLoop].renderingIntent == icAbsoluteColorimetric )
					{
						/*for concate absolute, make sure this is not done when preview tag exists)*/
						if (!newProfileSet->prof[profLoop].usePreviewTag) {
							LOCK_DATA(inputBuffer);
							err = DoAbsoluteShiftForPCS_Cube16( (UINT16*)DATA_2_PTR(inputBuffer), theCubePixelCount,
														   theCombiData.theProfile, (Boolean)(theCombiData.profileConnectionSpace != icSigLabData), kAbsShiftBeforeDoNDim );
							UNLOCK_DATA(inputBuffer);
							if (err)
								goto CleanupAndExit;
						}
					}
					if (theLutData.matrixMFT)
					{
						/*-----------------------------------------------------------------------------------------
						   the profile contained a mft AND a matrix:
						  -----------------------------------------------------------------------------------------*/
						/*------------------------------------ pcs is Lab -> handle direct (without Elut and Alut) */
						LOCK_DATA(inputBuffer);
#ifdef DEBUG_OUTPUT
						if ( DebugCheck(kThisFile, kDebugMiscInfo) )
						{
							DebugPrint("∆ matrixMFT: calling DoOnlyMatrixForCube16\n");
							DebugPrint("     %f    %f    %f\n     %f    %f    %f\n     %f    %f    %f\n",
								(*(Matrix2D *)theLutData.matrixMFT)[0][0],(*(Matrix2D *)theLutData.matrixMFT)[0][1],(*(Matrix2D *)theLutData.matrixMFT)[0][2],
								(*(Matrix2D *)theLutData.matrixMFT)[1][0],(*(Matrix2D *)theLutData.matrixMFT)[1][1],(*(Matrix2D *)theLutData.matrixMFT)[1][2],
								(*(Matrix2D *)theLutData.matrixMFT)[2][0],(*(Matrix2D *)theLutData.matrixMFT)[2][1],(*(Matrix2D *)theLutData.matrixMFT)[2][2]);
						}
#endif
#ifdef DEBUG_OUTPUT
						ShowCube16( profLoop, "DoOnlyMatrixForCube16 (mft)", createGamutLut, (UINT16 *)DATA_2_PTR(inputBuffer), gridPointsCube, numOfElutsCube, kNumOfLab_XYZchannels, VAL_USED_BITS );
#endif
#ifdef WRITE_LUTS
						if ( !createGamutLut )
						{
							fileString[0] = sprintf((SINT8*)&fileString[1], "DoOnlyMat4Cube #%d (mft)", profLoop);
							WriteLut2File( fileString,inputBuffer, 'XLUT');
						}
#endif
						DoOnlyMatrixForCube16( (Matrix2D	*)theLutData.matrixMFT, (Ptr)DATA_2_PTR(inputBuffer), theCubePixelCount, gridPointsCube );
#ifdef DEBUG_OUTPUT
						ShowCube16( profLoop, "after DoOnlyMatrixForCube16 (mft)", createGamutLut, (UINT16 *)DATA_2_PTR(inputBuffer), gridPointsCube, numOfElutsCube, kNumOfLab_XYZchannels, VAL_USED_BITS );
#endif
#ifdef WRITE_LUTS
						if ( !createGamutLut )
						{
							fileString[0] = sprintf((SINT8*)&fileString[1], "DoOnlyMat4Cube #%d (mft)", profLoop);
							WriteLut2File( fileString,inputBuffer, 'XLUT');
						}
#endif
						
						UNLOCK_DATA(inputBuffer);
					}
					if (theLutData.colorLutInDim >= theLutData.colorLutOutDim && theBufferByteCount == 2 )
					{
						outputBuffer = inputBuffer;
						useOutputBuffer = FALSE;
					} else
					{
						outputBuffer = ALLOC_DATA( (theCubePixelCount + theExtraSize ) * theLutData.colorLutOutDim * theBufferByteCount, &aOSerr);
						err = aOSerr;
						if (err)
							goto CleanupAndExit;
						useOutputBuffer = TRUE;
					}
					LOCK_DATA(inputBuffer);
					LOCK_DATA(outputBuffer);
					LOCK_DATA(theLutData.inputLut);
					LOCK_DATA(theLutData.colorLut);
					LOCK_DATA(theLutData.outputLut);
#ifdef DEBUG_OUTPUT
					if ( createGamutLut )
					{
						if ( DebugLutCheck(kDisplayEXAGamut) )
						{
							lutString[0] = sprintf((SINT8*)&lutString[1], "Gamut-E Lut #%d @ DoNDim", profLoop);
							DoDisplayLutNew(lutString,&theLutData, 0);
							lutString[0] = sprintf((SINT8*)&lutString[1], "Gamut-A Lut #%d @ DoNDim", profLoop);
							DoDisplayLutNew(lutString,&theLutData, 2);
							lutString[0] = sprintf((SINT8*)&lutString[1], "Gamut-X Lut #%d @ DoNDim", profLoop);
							DoDisplayLutNew(lutString, &theLutData,1);
						}
					} else if (DebugLutCheck(kDebugEXAReal) )
					{
							lutString[0] = sprintf((SINT8*)&lutString[1], "E Lut #%d @ DoNDim", profLoop);
							DoDisplayLutNew(lutString,&theLutData, 0);
							lutString[0] = sprintf((SINT8*)&lutString[1], "A Lut #%d @ DoNDim", profLoop);
							DoDisplayLutNew(lutString,&theLutData, 2);
							lutString[0] = sprintf((SINT8*)&lutString[1], "X Lut #%d @ DoNDim", profLoop);
							DoDisplayLutNew(lutString, &theLutData,1);
					}
#endif
					
#ifdef WRITE_LUTS
					if ( !createGamutLut )
					{
						fileString[0] = sprintf((SINT8*)&fileString[1], "E Lut #%d @ DoNDim", profLoop);
						WriteLut2File( fileString,theLutData.inputLut, 'ELUT');
						fileString[0] = sprintf((SINT8*)&fileString[1], "A Lut #%d @ DoNDim", profLoop);
						WriteLut2File( fileString,theLutData.outputLut, 'ALUT');
						fileString[0] = sprintf((SINT8*)&fileString[1], "X Lut #%d @ DoNDim", profLoop);
						WriteLut2File( fileString,theLutData.colorLut, 'XLUT');
					}
#endif

					calcParam.cmInputColorSpace		= cm16PerChannelPacking;
					calcParam.cmOutputColorSpace	= theBufferByteCount == 1 ? cm8PerChannelPacking : cm16PerChannelPacking;
					calcParam.cmPixelPerLine		= theCubePixelCount;
					calcParam.cmLineCount			= 1;
					calcParam.cmInputPixelOffset	= sizeof(SINT16)*theLutData.colorLutInDim;
					calcParam.cmOutputPixelOffset	= theBufferByteCount*theLutData.colorLutOutDim;
					calcParam.cmInputBytesPerLine	= theCubePixelCount*calcParam.cmInputPixelOffset;
					calcParam.cmOutputBytesPerLine	= theCubePixelCount*calcParam.cmOutputPixelOffset;
/*					calcParam.cmInputBytesPerPixel	= calcParam.cmInputPixelOffset;	*/
/*					calcParam.cmOutputBytesPerPixel	= calcParam.cmOutputPixelOffset;*/
					for (loop = 0; loop<theLutData.colorLutInDim; loop++)
						calcParam.inputData[loop]	= (Ptr)(DATA_2_PTR(inputBuffer)) + loop * sizeof(SINT16);
					for (loop = 0; loop<theLutData.colorLutOutDim; loop++)
						calcParam.outputData[loop]	= (Ptr)(DATA_2_PTR(outputBuffer)) + loop * theBufferByteCount;
					calcParam.clearMask				= FALSE;
					calcParam.copyAlpha				= FALSE;

#ifdef DEBUG_OUTPUT
					ShowCube16( profLoop, "DoNDim", createGamutLut, (UINT16 *)DATA_2_PTR(inputBuffer), gridPointsCube, numOfElutsCube, theLutData.colorLutInDim, VAL_USED_BITS );
#endif
#ifdef WRITE_LUTS
					if ( !createGamutLut )
					{
						fileString[0] = sprintf((SINT8*)&fileString[1], "DoNDim #%d", profLoop);
						WriteLut2File( fileString,inputBuffer, 'XLUT');
					}
#endif

					aDoNDimTableData = theLutData;
#if ! LUTS_ARE_PTR_BASED
					aDoNDimTableData.inputLut	= DATA_2_PTR(theLutData.inputLut);
					aDoNDimTableData.colorLut	= DATA_2_PTR(theLutData.colorLut);
					aDoNDimTableData.outputLut	= DATA_2_PTR(theLutData.outputLut);
#endif
					if (theLutData.colorLutWordSize == 18)		/* •••••••• */
					{
						if ( theBufferByteCount == 1 )
							err = CalcNDim_Data16To8_Lut8(&calcParam, &aDoNDimTableData);
						else
							err = CalcNDim_Data16To16_Lut8(&calcParam, &aDoNDimTableData);
					} else
					{
						if ( theBufferByteCount == 1 )
							err = CalcNDim_Data16To8_Lut16(&calcParam, &aDoNDimTableData);
						else
							err = CalcNDim_Data16To16_Lut16(&calcParam, &aDoNDimTableData);
					}
					if (err)
						goto CleanupAndExit;
#ifdef DEBUG_OUTPUT
					ShowCube16( profLoop, "after DoNDim", createGamutLut, (UINT16 *)DATA_2_PTR(outputBuffer), gridPointsCube, numOfElutsCube, theLutData.colorLutOutDim, 8*theBufferByteCount );
#endif
#ifdef WRITE_LUTS
					if ( !createGamutLut )
					{
						fileString[0] = sprintf((SINT8*)&fileString[1], "after DoNDim #%d", profLoop);
						WriteLut2File( fileString,outputBuffer, 'XLUT');
					}
#endif
					UNLOCK_DATA(theLutData.inputLut);
					UNLOCK_DATA(theLutData.colorLut);
					UNLOCK_DATA(theLutData.outputLut);
					UNLOCK_DATA(inputBuffer);
					UNLOCK_DATA(outputBuffer);
					
					/*----------------------------------------------------------------------------------------- absolute colorimetry*/
					/*for concate absolute, make sure this IS done when preview tag exists)*/
					if (newProfileSet->prof[profLoop].usePreviewTag && newProfileSet->prof[profLoop].renderingIntent == icAbsoluteColorimetric) {
						LOCK_DATA(outputBuffer);
						err = DoAbsoluteShiftForPCS_Cube16( (UINT16*)DATA_2_PTR(outputBuffer), theCubePixelCount,
														   theCombiData.theProfile, (Boolean)(theCombiData.profileConnectionSpace != icSigLabData), kAbsShiftAfterDoNDim );
						UNLOCK_DATA(outputBuffer);
						if (err)
							goto CleanupAndExit;
					}
					if ( !theCombiData.amIPCS && newProfileSet->prof[profLoop].renderingIntent == icAbsoluteColorimetric )
					{
						LOCK_DATA(outputBuffer);
							err = DoAbsoluteShiftForPCS_Cube16( (UINT16*)DATA_2_PTR(outputBuffer), theCubePixelCount,
														   theCombiData.theProfile, (Boolean)(theCombiData.profileConnectionSpace != icSigLabData), kAbsShiftAfterDoNDim );
						UNLOCK_DATA(outputBuffer);
						if (err)
							goto CleanupAndExit;
					}

					if (useOutputBuffer)
					{
						inputBuffer = DISPOSE_IF_DATA(inputBuffer);
						inputBuffer = outputBuffer;
					} else
					{
						/*SETDATASIZE(inputBuffer, theCubePixelCount * theLutData.colorLutOutDim * theBufferByteCount);*/
						/*err = MemError();*/
                       	/*inputBuffer = ALLOC_DATA( theCubePixelCount * theLutData.colorLutOutDim * theBufferByteCount, &aOSerr );
						err = aOSerr;
						if (err)
							goto CleanupAndExit;*/
					}
					/*theCubeSize = GETDATASIZE(inputBuffer)/theBufferByteCount;*/
                   	theCubeSize = theCubePixelCount * theLutData.colorLutOutDim * theBufferByteCount;
					outputBuffer = nil;
				}
			}
		}
		skipCombi = FALSE;
		if( profHeader.deviceClass == icSigLinkClass ){
			theCombiData.dataColorSpace = profHeader.pcs;
			theCombiData.amIPCS = TRUE;
		}

		if (theCombiData.amIPCS && !pcsConversion)
			theCombiData.amIPCS = (theCombiData.usePreviewTag == TRUE) || (theCombiData.dataColorSpace == icSigLabData) || (theCombiData.dataColorSpace == icSigXYZData);
		else
			theCombiData.amIPCS = TRUE;
	}
#ifdef DEBUG_OUTPUT
	if ( DebugCheck(kThisFile, kDebugMiscInfo) )
		DebugPrint("  <——————————————————————————————————————————————————————————————————>\n");
#endif
	theLutData.colorLut = DISPOSE_IF_DATA(theLutData.colorLut);
	theLutData.colorLut = inputBuffer;
	inputBuffer = nil;

	finalLutData->colorLut = theLutData.colorLut;	theLutData.colorLut = nil;
	finalLutData->colorLutGridPoints = savedGridPoints;
	if ( theBufferByteCount == 1 )
		finalLutData->colorLutWordSize = 8;
	else
		finalLutData->colorLutWordSize = 16;
	/* ---------------------------------------------------------------------------------
	    clean up
	   ---------------------------------------------------------------------------------*/
CleanupAndExit:
	inputBuffer			 = DISPOSE_IF_DATA(inputBuffer);
	outputBuffer		 = DISPOSE_IF_DATA(outputBuffer);
	theLutData.inputLut	 = DISPOSE_IF_DATA(theLutData.inputLut);
 	theLutData.outputLut = DISPOSE_IF_DATA(theLutData.outputLut);
	theLutData.colorLut	 = DISPOSE_IF_DATA(theLutData.colorLut);
 	theLutData.matrixMFT = DisposeIfPtr(theLutData.matrixMFT);
	theLutData.matrixTRC = DisposeIfPtr(theLutData.matrixTRC);

	LH_END_PROC("CreateCombi")
	return err;
}

/* ———————————————————————————————————————————————————————————————————————————————————————————
	CMError
	PrepareCombiLUTs	( CMMModelHandle    	CMSession,
						  CMConcatProfileSet*	profileSet )
   ——————————————————————————————————————————————————————————————————————————————————————————— */
/*CMError
PrepareCombiLUTsNew	( CMMModelPtr    		CMSession,
					  CMConcatProfileSet*	profileSet );*/
CMError
PrepareCombiLUTs	( CMMModelPtr    		CMSession,
					  CMConcatProfileSet*	profileSet )
{
	icHeader			 firstHeader;
	icHeader			 lastHeader;
	LHConcatProfileSet*	 newProfileSet = nil;
	CMError				 err = noErr;
	Boolean 			 needGamutCalc	= FALSE;
	CMLutParam			 theLutData;
 	UINT16		 		 count;
 	
	LH_START_PROC("PrepareCombiLUTs")
	/*err = PrepareCombiLUTsNew	( CMSession, profileSet );
	LH_END_PROC("PrepareCombiLUTs")
	return err;*/
	SetMem(&theLutData, sizeof(CMLutParam), 0);
	count = profileSet->count;

	/* ------------------------------------------------------------------------------------------------*/
	err = CMGetProfileHeader(profileSet->profileSet[0], (CMCoreProfileHeader *)&firstHeader);
	if (err)
		goto CleanupAndExit;
	(CMSession)->firstColorSpace = firstHeader.colorSpace;
	
	err = CMGetProfileHeader(profileSet->profileSet[count-1], (CMCoreProfileHeader *)&lastHeader);
	if (err)
		goto CleanupAndExit;
	(CMSession)->lastColorSpace = lastHeader.colorSpace;
	if( lastHeader.deviceClass == icSigLinkClass )
		(CMSession)->lastColorSpace = lastHeader.pcs;
	else
		(CMSession)->lastColorSpace = lastHeader.colorSpace;
#ifdef ALLOW_DEVICE_LINK
	if( count > 1 && lastHeader.deviceClass == icSigLinkClass ){
		(CMSession)->appendDeviceLink = TRUE;
		profileSet->count--;
	}
	else{
		(CMSession)->appendDeviceLink = FALSE;
	}
#endif
	/* ------------------------------------------------------------------------------------------------
		right now we create the gamut-luts always if the last profile is a display -or- an output profile
		(NO gamut-luts are created for a CMMNewLinkProfile-call).
	   ------------------------------------------------------------------------------------------------*/
	if  ( (lastHeader.deviceClass == icSigOutputClass) ||  (lastHeader.deviceClass == icSigDisplayClass) )
		needGamutCalc = TRUE;

	if ( (CMSession)->currentCall == kCMMNewLinkProfile )
		needGamutCalc = FALSE;
		
	/* ------------------------------------------------------------------------------------------------
		we could speed up the init-phase if we would use a flag in the first profile to indicate whether
		or not the gamut-luts should be created:
	   ------------------------------------------------------------------------------------------------*/
	if ( (firstHeader.flags & kCreateGamutLutMask) == kCreateGamutLutMask)
		needGamutCalc = FALSE;

#ifdef RenderInt
	if( CMSession-> dwFlags != 0xffffffff ){
		if( CMSession->dwFlags & kCreateGamutLutMask ){
			needGamutCalc = FALSE;
		}
		else if( lastHeader.deviceClass == icSigOutputClass ||
				 lastHeader.deviceClass == icSigDisplayClass )
			needGamutCalc = TRUE;
	}
#endif
	/* ------------------------------------------------------------------------------------------------*/
	err = Create_LH_ProfileSet( CMSession, profileSet, &newProfileSet);
	if (err)
		goto CleanupAndExit;
	
#ifdef DEBUG_OUTPUT
	if ( ! needGamutCalc && DebugCheck(kThisFile, kDebugMiscInfo) )
    	DebugPrint("\n  <=================== Gamut Combi is not created =================>\n\n");
#endif
    /*=============================================================================================*/
    /* Create Gamut Combi*/
    /*=============================================================================================*/
	if( CMSession->hasNamedColorProf == NamedColorProfileOnly ||
		CMSession->hasNamedColorProf == NamedColorProfileAtEnd )needGamutCalc = FALSE;
    if (needGamutCalc)
    {
#ifdef DEBUG_OUTPUT
        if ( DebugCheck(kThisFile, kDebugMiscInfo) )
            DebugPrint("  <======================= Create Gamut Combi =====================>\n");
#endif
		if( lastHeader.pcs != icSigXYZData &&			/* since there's only a makeGamutForMonitor for xyz */
			lastHeader.deviceClass == icSigDisplayClass &&
			lastHeader.colorSpace == icSigRgbData ){	/* insert a Lab->XYZ conversion */
											
			newProfileSet->prof[newProfileSet->count] = newProfileSet->prof[newProfileSet->count-1];
			newProfileSet->prof[newProfileSet->count-1].pcsConversionMode = kDoLab2XYZ;		/* create Lab->XYZ */
			newProfileSet->prof[newProfileSet->count-1].profileSet = 0;
			newProfileSet->prof[newProfileSet->count-1].renderingIntent = 0;
			newProfileSet->count++;
		}
		err = CreateCombi(CMSession, profileSet, newProfileSet, &theLutData, kDoGamutLut );
		if( lastHeader.pcs != icSigXYZData &&			/* since there's only a makeGamutForMonitor for xyz */
			lastHeader.deviceClass == icSigDisplayClass &&
			lastHeader.colorSpace == icSigRgbData ){			/* remove a Lab->XYZ conversion */	

			newProfileSet->count--;
			newProfileSet->prof[newProfileSet->count-1] = newProfileSet->prof[newProfileSet->count];
		}
		/* continue even if err != noErr !!!!*/
#ifdef DEBUG_OUTPUT
		if ( err && DebugCheck(kThisFile, kDebugErrorInfo) )
			DebugPrint("• Error: PrepareCombiLUTs - kDoGamutLut: %d\n",err);
#endif
		
		if (err == noErr)
		{
#ifdef DEBUG_OUTPUT
			if (DebugLutCheck(kDisplayEXAGamut) )
			{
				LOCK_DATA(theLutData.inputLut);
				LOCK_DATA(theLutData.colorLut);
				LOCK_DATA(theLutData.outputLut);
				DoDisplayLutNew("\pFINAL GAMUT E Lut", &theLutData, 0);
				DoDisplayLutNew("\pFINAL GAMUT A Lut", &theLutData, 2);
				DoDisplayLutNew("\pFINAL GAMUT X Lut", &theLutData, 1);
				UNLOCK_DATA(theLutData.inputLut);
				UNLOCK_DATA(theLutData.colorLut);
				UNLOCK_DATA(theLutData.outputLut);
			}
			if ( DebugLutCheck( kDisplayGamut ) )
			{
				LOCK_DATA(theLutData.colorLut);
				if ((CMSession)->precision == cmBestMode)
					Show32by32by32GamutXLUT(DATA_2_PTR(theLutData.colorLut));
				else
					Show16by16by16GamutXLUT(DATA_2_PTR(theLutData.colorLut));
				UNLOCK_DATA(theLutData.colorLut);
			}
#endif
			if (theLutData.inputLut == nil)
			{
#ifdef DEBUG_OUTPUT
				if ( DebugCheck(kThisFile, kDebugErrorInfo) )
					DebugPrint("• ERROR: final Gamut Elut == nil\n");
#endif
			} else
			{
				(CMSession)->gamutLutParam.inputLut = theLutData.inputLut;												/* E lut */
				theLutData.inputLut = nil;	
			}
			
			if (theLutData.outputLut == nil)
			{
#ifdef DEBUG_OUTPUT
				if ( DebugCheck(kThisFile, kDebugErrorInfo) )
					DebugPrint("• ERROR: final Gamut Alut == nil\n");
#endif
			} else
			{
				(CMSession)->gamutLutParam.outputLut = theLutData.outputLut;											/* A lut */
				theLutData.outputLut = nil;	
			}
			
			if (theLutData.colorLut == nil)
			{
#ifdef DEBUG_OUTPUT
				if ( DebugCheck(kThisFile, kDebugErrorInfo) )
					DebugPrint("• ERROR: final Gamut Xlut == nil\n");
#endif
			} else
			{
				(CMSession)->gamutLutParam.colorLut = theLutData.colorLut;												/* X lut */
				theLutData.colorLut = nil;	
			}
			(CMSession)->gamutLutParam.colorLutInDim		= theLutData.colorLutInDim;
			(CMSession)->gamutLutParam.colorLutOutDim		= theLutData.colorLutOutDim;
			(CMSession)->gamutLutParam.colorLutGridPoints	= theLutData.colorLutGridPoints;
			(CMSession)->gamutLutParam.colorLutWordSize		= theLutData.colorLutWordSize;
			(CMSession)->gamutLutParam.inputLutWordSize		= theLutData.inputLutWordSize;
			(CMSession)->gamutLutParam.outputLutWordSize	= theLutData.outputLutWordSize;
			(CMSession)->gamutLutParam.inputLutEntryCount	= theLutData.inputLutEntryCount;
			(CMSession)->gamutLutParam.outputLutEntryCount	= theLutData.outputLutEntryCount;
		}
	}
	/*=============================================================================================
	   Create 'real' Combi
	  =============================================================================================*/
#ifdef DEBUG_OUTPUT
	if ( DebugCheck(kThisFile, kDebugMiscInfo) )
		DebugPrint("  <======================= Create real Combi ======================>\n");
#endif

	err = CreateCombi(CMSession, profileSet, newProfileSet, &theLutData, kDoDefaultLut );
	if (err)
		goto CleanupAndExit;
	
#ifdef DEBUG_OUTPUT
	if (DebugLutCheck(kDebugEXAReal) )
	{
		LOCK_DATA(theLutData.inputLut);
		LOCK_DATA(theLutData.colorLut);
		LOCK_DATA(theLutData.outputLut);
		DoDisplayLutNew("\pFINAL E Lut", &theLutData, 0);
		DoDisplayLutNew("\pFINAL A Lut", &theLutData, 2);
		DoDisplayLutNew("\pFINAL X Lut", &theLutData, 1);
		UNLOCK_DATA(theLutData.inputLut);
		UNLOCK_DATA(theLutData.colorLut);
		UNLOCK_DATA(theLutData.outputLut);
	}
#endif
#ifdef WRITE_LUTS
	LOCK_DATA(theLutData.inputLut);
	LOCK_DATA(theLutData.colorLut);
	LOCK_DATA(theLutData.outputLut);
	WriteLut2File( "\pFINAL E Lut",theLutData.inputLut, 'ELUT');
	WriteLut2File( "\pFINAL A Lut",theLutData.outputLut, 'ALUT');
	WriteLut2File( "\pFINAL X Lut",theLutData.colorLut, 'XLUT');
	UNLOCK_DATA(theLutData.inputLut);
	UNLOCK_DATA(theLutData.colorLut);
	UNLOCK_DATA(theLutData.outputLut);
#endif
	/*------------------------ save all results...*/
	if (theLutData.inputLut == nil)
	{
#ifdef DEBUG_OUTPUT
		if ( DebugCheck(kThisFile, kDebugErrorInfo) )
			DebugPrint("• ERROR: theLutData.inputLut == nil\n");
#endif
	} else
	{
		(CMSession)->lutParam.inputLut = theLutData.inputLut;												/* E lut */
		theLutData.inputLut = nil;	
	}
	
	if (theLutData.outputLut == nil)
	{
#ifdef DEBUG_OUTPUT
		if ( DebugCheck(kThisFile, kDebugErrorInfo) )
			DebugPrint("• ERROR: theLutData.outputLut == nil\n");
#endif
	} else
	{
		(CMSession)->lutParam.outputLut = theLutData.outputLut;											/* A lut */
		theLutData.outputLut = nil;	
	}
	
	if (theLutData.colorLut == nil)
	{
#ifdef DEBUG_OUTPUT
		if ( DebugCheck(kThisFile, kDebugErrorInfo) )
			DebugPrint("• ERROR: theLutData.colorLut == nil\n");
#endif
	} else
	{
		(CMSession)->lutParam.colorLut = theLutData.colorLut;												/* X lut */
		theLutData.colorLut = nil;	
	}
	
	(CMSession)->lutParam.colorLutInDim			= theLutData.colorLutInDim;
	(CMSession)->lutParam.colorLutOutDim		= theLutData.colorLutOutDim;
	(CMSession)->lutParam.colorLutGridPoints	= theLutData.colorLutGridPoints;
	(CMSession)->lutParam.colorLutWordSize		= theLutData.colorLutWordSize;
	(CMSession)->lutParam.inputLutWordSize		= theLutData.inputLutWordSize;
	(CMSession)->lutParam.outputLutWordSize		= theLutData.outputLutWordSize;
	(CMSession)->lutParam.inputLutEntryCount	= theLutData.inputLutEntryCount;
	(CMSession)->lutParam.outputLutEntryCount	= theLutData.outputLutEntryCount;
	
	/* ---------------------------------------------------------------------------------
	    clean up & exit
	   ---------------------------------------------------------------------------------*/
CleanupAndExit:
	newProfileSet = (LHConcatProfileSet *)DisposeIfPtr((Ptr)newProfileSet);
	theLutData.inputLut		= DISPOSE_IF_DATA(theLutData.inputLut);
	theLutData.outputLut	= DISPOSE_IF_DATA(theLutData.outputLut);
	theLutData.colorLut		= DISPOSE_IF_DATA(theLutData.colorLut);

	LH_END_PROC("PrepareCombiLUTs")
	return err;
}

CMError InitNamedColorProfileData( 	CMMModelPtr 		storage,
						  			CMProfileRef		aProf,
									long				pcs,
									long				*theDeviceCoords )
{
	CMError				err = noErr;
	UINT32				elementSize,count,nDeviceCoords,i;
	LUT_DATA_TYPE		profileLutPtr = 0;
	Ptr					cPtr;
	OSErr				aOSerr;

	*theDeviceCoords = 0;
	if( storage->hasNamedColorProf != NoNamedColorProfile ){ /* allow only 1 named profile */
		err = cmCantConcatenateError;
		goto CleanupAndExit;
	}
	err = CMGetProfileElement(aProf, icSigNamedColor2Tag, &elementSize, nil);
	if (err)
		goto CleanupAndExit;
	
  	profileLutPtr = ALLOC_DATA(elementSize, &aOSerr );
	err = aOSerr;
	if (err)
		goto CleanupAndExit;
	
	LOCK_DATA(profileLutPtr);
	err = CMGetProfileElement( aProf, icSigNamedColor2Tag, &elementSize, DATA_2_PTR(profileLutPtr) );
	if (err)
		goto CleanupAndExit;
#ifdef IntelMode
	SwapLongOffset( &((icNamedColor2Type*)profileLutPtr)->ncolor.count, 0, 4 );
	SwapLongOffset( &((icNamedColor2Type*)profileLutPtr)->ncolor.nDeviceCoords, 0, 4 );
#endif
	count = ((icNamedColor2Type*)profileLutPtr)->ncolor.count;
	nDeviceCoords = ((icNamedColor2Type*)profileLutPtr)->ncolor.nDeviceCoords;
	cPtr = &((icNamedColor2Type*)profileLutPtr)->ncolor.data[0];
	if( pcs == icSigXYZData ){
		for( i=0; i<count; i++){
			cPtr += 32;
#ifdef IntelMode
			SwapShortOffset( cPtr, 0, 3*2 );
			XYZ2Lab_forCube16((unsigned short *)cPtr, 1);
			cPtr += 3*2;
			SwapShortOffset( cPtr, 0, nDeviceCoords*2 );
			cPtr += nDeviceCoords*2;
#else
			XYZ2Lab_forCube16((unsigned short *)cPtr, 1);
			cPtr += 3*2 + nDeviceCoords*2;
#endif
		}
	}
#ifdef IntelMode
	else{
		for( i=0; i<count; i++){
			cPtr += 32;
			SwapShortOffset( cPtr, 0, 3*2 );
			cPtr += 3*2;
			SwapShortOffset( cPtr, 0, nDeviceCoords*2 );
			cPtr += nDeviceCoords*2;
		}
	}
#endif
	storage->theNamedColorTagData = profileLutPtr;
	UNLOCK_DATA(profileLutPtr);
	profileLutPtr = 0;
	*theDeviceCoords = nDeviceCoords;

CleanupAndExit:
	DISPOSE_IF_DATA(profileLutPtr);
	return err;
}
