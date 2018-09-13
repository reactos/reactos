/*
	File:		PI_CMMInitialization.c

	Contains:	
				
	Written by:	U. J. Krabbenhoeft

	Version:

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

*/

#ifndef LHGeneralIncs_h
#include "General.h"
#endif

#ifndef LHGenLuts_h
#include "GenLuts.h"
#endif

#ifndef PI_CMMInitialization_h
#include "PI_CMM.h"
#endif

#ifndef LHStdConversionLuts_h
#include "StdConv.h"
#endif

#if ! realThing
#ifdef DEBUG_OUTPUT
#define kThisFile kCMMInitializationID
#define __TYPES__
/*#include "DebugSpecial.h"*/
/*#include "LH_Util.h"*/
#endif
#endif

#define ALLOW_DEVICE_LINK   /* allows link as the last profile in a chain, change in genluts.c too */
/* ______________________________________________________________________

CMError CMMInitPrivate( 	CMMModelPtr 		storage, 
						 	CMProfileRef 		srcProfile, 
						 	CMProfileRef 		dstProfile );

	Abstract:
		ColorWorld function called to initialize a matching session.

	Params:
		Storage		(in)		Reference to CMMModel.
		srcProfile	(in)		Reference to source CMProfileRef.
		dstProfile	(in)		Reference to destination CMProfileRef.
		
	Return:
		noErr		successful
		System or result code if an error occurs.

   _____________________________________________________________________ */

CMError CMMInitPrivate( 	CMMModelPtr 		storage, 
						 	CMProfileRef 		srcProfile, 
						 	CMProfileRef 		dstProfile )
{
    CMError					err = noErr;
    OSErr					aOSerr = noErr;
    CMConcatProfileSet 		*profileSet = nil;
    CMCoreProfileHeader		sourceHeader;
    CMCoreProfileHeader		destHeader;
    Boolean					valid;
    short					mode = 0;
#ifdef DEBUG_OUTPUT
    long					timer = TickCount();
#endif

#ifdef DEBUG_OUTPUT
    if ( DebugCheck(kThisFile, kDebugMiscInfo) )
        DebugPrint("¥ ->CMMInitPrivate\n");
#endif

    /*	--------------------------------------------------------------------------------------- valid profiles ???*/
    err = CMValidateProfile( srcProfile, &valid );
    if (err)
        goto CleanupAndExit;
    if (!valid)
    {
#ifdef DEBUG_OUTPUT
        if ( DebugCheck(kThisFile, kDebugErrorInfo) )
            DebugPrint("¥ CMMInitPrivate ERROR:  srcProfile is NOT valid!\n");
#endif
        
#ifdef realThing
        err = cmProfileError;
        goto CleanupAndExit;
#endif
    }

    err = CMValidateProfile(dstProfile, &valid );
    if (err)
        goto CleanupAndExit;
    if (!valid)
    {
#ifdef DEBUG_OUTPUT
        if ( DebugCheck(kThisFile, kDebugErrorInfo) )
            DebugPrint("¥ CMMInitPrivate ERROR:  dstProfile is NOT valid!\n");
#endif
#ifdef realThing
        err = cmProfileError;
        goto CleanupAndExit;
#endif
    }
    
    /*	--------------------------------------------------------------------------------------- initialization*/
	(storage)->lutParam.inputLut  = DISPOSE_IF_DATA((storage)->lutParam.inputLut);
	(storage)->lutParam.colorLut  = DISPOSE_IF_DATA((storage)->lutParam.colorLut);
	(storage)->lutParam.outputLut = DISPOSE_IF_DATA((storage)->lutParam.outputLut);
	
	(storage)->gamutLutParam.inputLut  = DISPOSE_IF_DATA((storage)->gamutLutParam.inputLut);
	(storage)->gamutLutParam.colorLut  = DISPOSE_IF_DATA((storage)->gamutLutParam.colorLut);
	(storage)->gamutLutParam.outputLut = DISPOSE_IF_DATA((storage)->gamutLutParam.outputLut);

 	(storage)->theNamedColorTagData = DISPOSE_IF_DATA((storage)->theNamedColorTagData);
	
	(storage)->srcProfileVersion = icVersionNumber;
    (storage)->dstProfileVersion = icVersionNumber;

    /*	--------------------------------------------------------------------------------------- check version of source profile*/
    err = CMGetProfileHeader( srcProfile, &sourceHeader);
    if (err)
        goto CleanupAndExit;

    if ( !((sourceHeader.version & 0xff000000) >= icVersionNumber) ){
        err = cmProfileError;
        goto CleanupAndExit;
    }
	if (sourceHeader.deviceClass == icSigLinkClass)
	{
		err = cmCantConcatenateError;
		goto CleanupAndExit;
	}

    /*	--------------------------------------------------------------------------------------- check version of destination profile*/
    err = CMGetProfileHeader( dstProfile, &destHeader);
    if (err)
        goto CleanupAndExit;

    if ( !((destHeader.version & 0xff000000) >= icVersionNumber) ){
        err = cmProfileError;
        goto CleanupAndExit;
    }
  	if (destHeader.deviceClass == icSigLinkClass)
	{
		err = cmCantConcatenateError;
		goto CleanupAndExit;
	}
  	storage->lookup =  (Boolean)((sourceHeader.flags & kLookupOnlyMask)>>16);		/* lookup vs. interpolation */
  	
#ifdef RenderInt
	if( storage-> dwFlags != 0xffffffff ){
		storage->lookup = (Boolean)((storage-> dwFlags & kLookupOnlyMask)>>16);
	}
#endif
    /*	--------------------------------------------------------------------------------------- 'normal' cases*/
                profileSet = (CMConcatProfileSet *)SmartNewPtr(sizeof (CMConcatProfileSet) + sizeof(CMProfileRef), &aOSerr);
                if (aOSerr)
                    goto CleanupAndExit;
                    
                profileSet->count    = 2;
                profileSet->keyIndex = 1;
                /* profileSet->flags	 = sourceHeader.flags; */
                profileSet->profileSet[0] = srcProfile;
                /* profileSet->profileSet[0]->renderingIntent = sourceHeader.renderingIntent; */
                profileSet->profileSet[1] = dstProfile;
                /* profileSet->profileSet[1]->renderingIntent = destHeader.renderingIntent; */
                     
                err =  PrepareCombiLUTs( storage, profileSet );	
    if (err)
        goto CleanupAndExit;

CleanupAndExit:
    profileSet = (CMConcatProfileSet*)DisposeIfPtr( (Ptr)profileSet );

#ifdef DEBUG_OUTPUT
    if ( err && DebugCheck(kThisFile, kDebugErrorInfo) )
        DebugPrint("¥ CMMInitPrivate: err = %d\n", err);
    if ( DebugCheck(kThisFile, kDebugTimingInfo) )
        DebugPrint("  time in CMMInitPrivate: %f second(s)\n",(TickCount()-timer)/60.0);
    if ( DebugCheck(kThisFile, kDebugMiscInfo) )
        DebugPrint("¥ <-CMMInitPrivate\n");
#endif
	return err;
} 

CMError MakeSessionFromLink(		CMMModelPtr 		storage, 
						  			CMConcatProfileSet 	*profileSet	);

#ifndef HD_NEW_CONCATE_INIT
CMError CMMConcatInitPrivate	( 	CMMModelPtr 		storage, 
						  			CMConcatProfileSet 	*profileSet)
{
	CMCoreProfileHeader	firstHeader;
	CMCoreProfileHeader	lastHeader;
	CMCoreProfileHeader	tempHeader;
	CMError					err = noErr;
	unsigned short			count;
	unsigned short			loop;
	Boolean					valid;
	CMProfileRef			theProfile;
	#ifdef DEBUG_OUTPUT
	long					timer = TickCount();
	#endif
	

	#ifdef DEBUG_OUTPUT
	if ( DebugCheck(kThisFile, kDebugMiscInfo) )
	{
		DebugPrint("¥ ->CMMConcatInitPrivate\n");
		DebugPrint("  got %d profiles      keyindex is %d \n", profileSet->count, profileSet->keyIndex);
	}
	#endif
	
	count = profileSet->count;
	if (count == 0)
	{
		err = cmparamErr;
		goto CleanupAndExit;
	}
	/* ------------------------------------------------------------------------------------------ get first header	*/
	err = CMGetProfileHeader( profileSet->profileSet[0], &firstHeader);
	if (err)
		goto CleanupAndExit;
	/* ------------------------------------------------------------------------------------------ only one profile? -> has to be a link profile	*/
  	storage->lookup =  (Boolean)((firstHeader.flags & kLookupOnlyMask)>>16);		/* lookup vs. interpolation */
  	
#ifdef RenderInt
	if( storage-> dwFlags != 0xffffffff ){
		storage->lookup = (Boolean)((storage-> dwFlags & kLookupOnlyMask)>>16);
	}
#endif
	if (count == 1 && firstHeader.deviceClass != icSigNamedColorClass )
	{
		if (firstHeader.deviceClass != icSigLinkClass && firstHeader.deviceClass != icSigAbstractClass )
		{
			err =cmCantConcatenateError;
			goto CleanupAndExit;
		}
		else{
			err = MakeSessionFromLink( storage, profileSet	);
			if( err == 0 )return noErr;
		}
		lastHeader = firstHeader;
	} else
	{
		/* -------------------------------------------------------------------------------------- get last header	*/
		err = CMGetProfileHeader( profileSet->profileSet[count-1], &lastHeader);
		if (err)
			goto CleanupAndExit;
#ifndef ALLOW_DEVICE_LINK
		if (lastHeader.deviceClass == icSigLinkClass)
		{
			err = cmCantConcatenateError;
			goto CleanupAndExit;
		}
#endif
	}
	
	/* ------------------------------------------------------------------------------------------ valid profiles ???	*/
	for ( loop = 0; loop < count; loop++)
	{
		theProfile = profileSet->profileSet[loop];
		err = CMValidateProfile( theProfile, &valid );
		if (err)
			goto CleanupAndExit;
		if (!valid)
		{
		 	#ifdef DEBUG_OUTPUT
			if ( DebugCheck(kThisFile, kDebugErrorInfo) )
				DebugPrint("¥ CMMConcatInitPrivate ERROR: profile #%d is NOT valid!\n", loop);
			#endif
			#ifdef realThing
			err = cmProfileError;
			goto CleanupAndExit;
			#endif
		}
		/* -------------------------------------------------------------------------------------- link profiles may not be used inbetween	*/
		if ( (loop > 0) && (loop < count-1))
		{
			err = CMGetProfileHeader( profileSet->profileSet[loop], &tempHeader);
			if (err)
				goto CleanupAndExit;
			if (tempHeader.deviceClass == icSigLinkClass)
			{
				err = cmCantConcatenateError;
				goto CleanupAndExit;
			}
		}
	}
	
	/* ------------------------------------------------------------------------------------------ no abstract profile as first or last	*/
	if ( (count >1) && ( (firstHeader.deviceClass == icSigAbstractClass) || (lastHeader.deviceClass == icSigAbstractClass) ) )
	{
		err = cmCantConcatenateError;
		goto CleanupAndExit;
	}
	
	/* ------------------------------------------------------------------------------------------ initialization	*/
	(storage)->lutParam.inputLut  = DISPOSE_IF_DATA((storage)->lutParam.inputLut);
	(storage)->lutParam.colorLut  = DISPOSE_IF_DATA((storage)->lutParam.colorLut);
	(storage)->lutParam.outputLut = DISPOSE_IF_DATA((storage)->lutParam.outputLut);
	
	(storage)->gamutLutParam.inputLut  = DISPOSE_IF_DATA((storage)->gamutLutParam.inputLut);
	(storage)->gamutLutParam.colorLut  = DISPOSE_IF_DATA((storage)->gamutLutParam.colorLut);
	(storage)->gamutLutParam.outputLut = DISPOSE_IF_DATA((storage)->gamutLutParam.outputLut);

	(storage)->theNamedColorTagData = DISPOSE_IF_DATA((storage)->theNamedColorTagData);
	/* ------------------------------------------------------------------------------------------ check 'special' cases	*/
	{
		err =  PrepareCombiLUTs( storage, profileSet );	
	}

CleanupAndExit:

	#ifdef DEBUG_OUTPUT
	if ( err && DebugCheck(kThisFile, kDebugErrorInfo) )
		DebugPrint("  CMMConcatInitPrivate: err = %d\n", err);
	DebugPrint("  time in CMMConcatInitPrivate: %f second(s)\n",(TickCount()-timer)/60.0);
	DebugPrint("¥ <-CMMConcatInitPrivate\n");
	#endif
	return( err );
} 

Boolean IsPowerOf2( unsigned long l );
Boolean IsPowerOf2( unsigned long l )
{
	unsigned long i;
	for( i=1; i<32; i++){
		if( (1U<<i) == l ) return 1;
	}
	return 0;
}

#endif
CMError MakeSessionFromLink(		CMMModelPtr 		storage, 
						  			CMConcatProfileSet 	*profileSet	)
{
	CMLutParam	theLut={0};	
	CMLutParam	*theLutData;	
	LHCombiData	theCombi={0};
	LHCombiData	*theCombiData;
	double		*aDoublePtr;
	double		aDouble;
	OSType		theTag = icSigAToB0Tag;

  	CMError		err = noErr;
  	OSErr		aOSerr = noErr;
	Ptr			profileLutPtr = nil;
	UINT32		elementSize;
	double		factor;
	UINT32 		byteCount;
    CMCoreProfileHeader		aHeader;

	LH_START_PROC("MakeSessionFromLink")


	theLutData = &theLut;
	theCombiData = &theCombi;
 	theCombiData->theProfile = profileSet->profileSet[0];

	/* -------------------------------------------------------- get partial tag data from profile */
	err = CMGetProfileElement(theCombiData->theProfile, theTag, &elementSize, nil);
	if (err)
		goto CleanupAndExit;
	
	byteCount = 52;											/* get the first 52 bytes out of the profile */
  	profileLutPtr = SmartNewPtr(byteCount, &aOSerr);
	err = aOSerr;
	if (err)
		goto CleanupAndExit;
	
    err = CMGetProfileElement( theCombiData->theProfile, theTag, &byteCount, profileLutPtr );
	if (err)
		goto CleanupAndExit;
#ifdef IntelMode
    SwapLongOffset( &((icLut16Type*)profileLutPtr)->base.sig, 0, 4 );
    SwapShortOffset( &((icLut16Type*)profileLutPtr)->lut.inputEnt, 0, 2 );
    SwapShortOffset( &((icLut16Type*)profileLutPtr)->lut.outputEnt, 0, 2 );
#endif

	theLutData->colorLutInDim 		= ((icLut8Type*)profileLutPtr)->lut.inputChan;
	theLutData->colorLutOutDim 		= ((icLut8Type*)profileLutPtr)->lut.outputChan;
	theLutData->colorLutGridPoints 	= ((icLut8Type*)profileLutPtr)->lut.clutPoints;

	switch( theLutData->colorLutInDim ){
	  case 3:
		if( theLutData->colorLutGridPoints != 16 && theLutData->colorLutGridPoints != 32 ){
			err = 1;
			goto CleanupAndExit;
		}
		break;
	  case 4:
		if( theLutData->colorLutGridPoints != 8 && theLutData->colorLutGridPoints != 16 ){
			err = 1;
			goto CleanupAndExit;
		}
		break;
	}

	err = CMGetProfileHeader( profileSet->profileSet[0], &aHeader);
	if (err)
		goto CleanupAndExit;
	storage->firstColorSpace = aHeader.colorSpace;
	storage->lastColorSpace = aHeader.pcs;
    storage->srcProfileVersion = icVersionNumber;
    storage->dstProfileVersion = icVersionNumber;

	if (	( theLutData->colorLutInDim == 3) &&
			( aHeader.pcs == icSigXYZData ) )
	{
		factor = 1.;
		err = GetMatrixFromProfile(theLutData, theCombiData, theTag, factor);
		if( err ) goto CleanupAndExit;
		aDoublePtr = (double *)theLutData->matrixMFT;
		if( aDoublePtr != 0 ){
			aDouble = aDoublePtr[0] + aDoublePtr[4] + aDoublePtr[8];
			if( aDouble > 3.0 + 1E-6 || aDouble < 3.0 - 1E-6 ){
				err = 1;
				goto CleanupAndExit;
			}
		}
	}
	theCombiData->maxProfileCount = 0;
	if( ((icLut16Type*)profileLutPtr)->base.sig == icSigLut16Type ){
		theCombiData->doCreate_16bit_Combi = 1;
		theCombiData->doCreate_16bit_ELut = 0;
		theCombiData->doCreate_16bit_XLut = 1;
		theCombiData->doCreate_16bit_ALut = 0;
	}
	else{
		theCombiData->doCreate_16bit_Combi = 0;
		theCombiData->doCreate_16bit_ELut = 0;
		theCombiData->doCreate_16bit_XLut = 0;
		theCombiData->doCreate_16bit_ALut = 0;
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

	storage->lutParam = *theLutData;

	/* ---------------------------------------------------------------------------------
		 clean up
	   ---------------------------------------------------------------------------------*/
CleanupAndExit:
	profileLutPtr = DisposeIfPtr(profileLutPtr);
	LH_END_PROC("MakeSessionFromLink")
	return err;
}
#if 0
#define	POS(x)	((x) > (0) ? (x) : -(x))
CMError QuantizeNamedValues( CMMModelPtr 		storage,
							 Ptr				imgIn,
							 long				size )
{
	long	j,k;
	UINT16	*imgInPtr;
	UINT16	*tagTbl = NULL,*colorPtr = NULL;
	Handle	tagH = NULL;
	CMError	err = noErr;
	long	elemSz,deviceChannelCount,count;
	UINT16	LL,aa,bb;
	UINT32	dE,dEnow,index;

	LH_START_PROC("QuantizeNamedValues")
	tagH = storage->theNamedColorTagData;
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

	imgInPtr = (UINT16 *)  imgIn;
 	for (j = 0; j < size; j++) 
	{
		LL = (*imgInPtr+0);
		aa = (*imgInPtr+1);
		bb = (*imgInPtr+2);
		/* go through the whole table to find the closest one*/
		dEnow = 0x40000;	/* just arbitrarily high = 256*256*4 */
		index =(UINT32)-1;
		colorPtr = tagTbl;
		for (k = 0; k < count; k++) 
		{
			dE = 	  POS(LL - *(colorPtr+0));
			dE = dE + POS(aa - *(colorPtr+1));
			dE = dE + POS(bb - *(colorPtr+2));
			if (dE < dEnow) 
			{
				index = k;
				dEnow = dE;
			}
			colorPtr += elemSz;
		}
		colorPtr = tagTbl + index * elemSz;
		*(imgInPtr+0)= *(colorPtr+0);
		*(imgInPtr+1)= *(colorPtr+1);
		*(imgInPtr+2)= *(colorPtr+2);
		imgInPtr += 3;
	}
		UNLOCK_DATA(tagH);
CleanUp:
	LH_END_PROC("QuantizeNamedValues")
	return err;
}
#endif
