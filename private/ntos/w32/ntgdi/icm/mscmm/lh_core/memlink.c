/*
	File:		MemLink.c

	Contains:	
		creation of mem based profiles

	Written by:	U. J. Krabbenhoeft

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

	Version:	
*/

#ifndef LHGeneralIncs_h
#include "General.h"
#endif

#if __IS_MSDOS
#include <wtypes.h>
#include <winbase.h>
#include <windowsX.h>
#ifdef RenderInt
#include "Windef.h"
#endif
#endif

#ifndef MemLink_h
#include "MemLink.h"
#endif

#if !__IS_MSDOS && !__IS_MAC
#include <time.h>
#endif

#ifdef DEBUG_OUTPUT
#define kThisFile kLHCMRuntimeID
#endif


#ifndef IntelMode
#define CMHelperICC2int16Const(a, b) 																\
		  (*((UINT16 *)(a))) = (UINT16)(b);
#define CMHelperICC2int32Const(a, b) 																\
		  (*((UINT32 *)(a))) = (UINT32)(b);
#else
#define CMHelperICC2int16Const(a, b) 																\
		  (*((UINT16 *)(a))) = (((UINT16)(b)>>8)         | ((UINT16)((UINT8 )(b)) << 8));
#define CMHelperICC2int32Const(a, b) 																\
		  (*((UINT32 *)(a))) = (((UINT32)(b)>>24)         | ((UINT32)(0x0ff0000&(UINT32)(b)) >> 8) | \
		        ((UINT32)(0x0ff00&(UINT32)(b)) << 8) | ((UINT32)(0x0ff&(UINT32)(b)) << 24));
#endif

CMError MyAdd_NL_Header( UINT32 theSize, icHeader	*linkHeader,
						 UINT32 aIntent, UINT32 aClass, UINT32 aColorSpace, UINT32 aConnectionSpace )
{
	OSErr					err = noErr;
#if ! __IS_MSDOS
	unsigned long			secs;
#endif
	DateTimeRec				datetimeRec;
	/*icHeader	linkHeader; */

#if __IS_MSDOS
	SYSTEMTIME aSystemTime;
	GetLocalTime( (SYSTEMTIME *)&aSystemTime);
	datetimeRec.year	 = aSystemTime.wYear; 	datetimeRec.month	 = aSystemTime.wMonth;
	datetimeRec.day		 = aSystemTime.wDay; 	datetimeRec.hour	 = aSystemTime.wHour;
	datetimeRec.minute	 = aSystemTime.wMinute; datetimeRec.second	 = aSystemTime.wSecond;
#else
#if __IS_MAC
	GetDateTime(&secs);
	SecondsToDate(secs, &datetimeRec);/* Secs2Date  (link.*)=(.*); CMHelperICC2int32(\&(\1),\2); */
#else
    struct tm* loctime;
    time_t long_time;
    time( &long_time );                /* Get time as long integer. */
	loctime = localtime(&long_time);
	datetimeRec.year	 = loctime->tm_year;	datetimeRec.month	= loctime->tm_mon+1;
	datetimeRec.day		 = loctime->tm_mday;	datetimeRec.hour	= loctime->tm_hour;
	datetimeRec.minute	 = loctime->tm_min;		datetimeRec.second	= loctime->tm_sec;
#endif
#endif
	
	CMHelperICC2int32Const(&(linkHeader->size				), theSize);		/* This is the total size of the CMProfileRef */
	CMHelperICC2int32Const(&(linkHeader->cmmId 				), 'Win ');							/* CMM signature,  Registered with ICC consortium  */
	CMHelperICC2int32Const(&(linkHeader->version 			), icVersionNumber);				/* Version of CMProfile format */
	CMHelperICC2int32Const(&(linkHeader->deviceClass 		), aClass);					/* input, display, output, devicelink, abstract, or color conversion profile type */
	
	CMHelperICC2int32Const(&(linkHeader->colorSpace			), aColorSpace);	/* color space of data = dataColorSpace of first profile*/

	CMHelperICC2int32Const(&(linkHeader->pcs				), aConnectionSpace);	/* profile connection color space = dataColorSpace of last profile*/
	
	CMHelperICC2int16(&(linkHeader->date.year				), &datetimeRec.year);					/* date and time of profile creation */
	CMHelperICC2int16(&(linkHeader->date.month				), &datetimeRec.month);	
	CMHelperICC2int16(&(linkHeader->date.day				), &datetimeRec.day);	
	CMHelperICC2int16(&(linkHeader->date.hours				), &datetimeRec.hour);	
	CMHelperICC2int16(&(linkHeader->date.minutes			), &datetimeRec.minute);	
	CMHelperICC2int16(&(linkHeader->date.seconds			), &datetimeRec.second);	
	CMHelperICC2int32Const(&(linkHeader->magic				), icMagicNumber);					/* 'acsp' constant ICC file ID */
	CMHelperICC2int32Const(&(linkHeader->platform 			), icSigMicrosoft);					/* primary profile platform, Registered with ICC consortium */
	CMHelperICC2int32Const(&(linkHeader->flags				), 0);								/* profile flags */
	CMHelperICC2int32Const(&(linkHeader->manufacturer		), icSigMicrosoft);							/* Registered with ICC consortium */
	CMHelperICC2int32Const(&(linkHeader->model				), 0);								/* Registered with ICC consortium */
	CMHelperICC2int32Const(&(linkHeader->attributes[0]		), 0);								/* Attributes like paper type */
	CMHelperICC2int32Const(&(linkHeader->attributes[1]		), 0);
	CMHelperICC2int32Const(&(linkHeader->renderingIntent	), aIntent );								/* preferred rendering intent of tagged object */
	CMHelperICC2int32Const(&(linkHeader->illuminant.X		), 0.9642 * 65536);					/* profile illuminant */
	CMHelperICC2int32Const(&(linkHeader->illuminant.Y		), 1.0000 * 65536);
	CMHelperICC2int32Const(&(linkHeader->illuminant.Z		), 0.8249 * 65536);
	CMHelperICC2int32Const(&(linkHeader->creator			), 'UJK ');
		
	return err;
}

CMError MyAdd_NL_HeaderMS	( UINT32 theSize, icHeader	*linkHeader, unsigned long aIntent, icColorSpaceSignature sCS, icColorSpaceSignature dCS );
CMError MyAdd_NL_HeaderMS	( UINT32 theSize, icHeader	*linkHeader, unsigned long aIntent, icColorSpaceSignature sCS, icColorSpaceSignature dCS )
{
	OSErr					err = noErr;
#if ! __IS_MSDOS
	unsigned long			secs;
#endif
	DateTimeRec				datetimeRec;
	/*icHeader	linkHeader; */

#if __IS_MSDOS
	SYSTEMTIME aSystemTime;
	GetLocalTime( (SYSTEMTIME *)&aSystemTime);
	datetimeRec.year	 = aSystemTime.wYear; 	datetimeRec.month	 = aSystemTime.wMonth;
	datetimeRec.day		 = aSystemTime.wDay; 	datetimeRec.hour	 = aSystemTime.wHour;
	datetimeRec.minute	 = aSystemTime.wMinute; datetimeRec.second	 = aSystemTime.wSecond;
#else
#if __IS_MAC
	GetDateTime(&secs);
	SecondsToDate(secs, &datetimeRec);/* Secs2Date  (link.*)=(.*); CMHelperICC2int32(\&(\1),\2); */
#else
    struct tm* loctime;
    time_t long_time;
    time( &long_time );                /* Get time as long integer. */
	loctime = localtime(&long_time);
	datetimeRec.year	 = loctime->tm_year;	datetimeRec.month	= loctime->tm_mon+1;
	datetimeRec.day		 = loctime->tm_mday;	datetimeRec.hour	= loctime->tm_hour;
	datetimeRec.minute	 = loctime->tm_min;		datetimeRec.second	= loctime->tm_sec;
#endif
#endif
	
	
	linkHeader->size				= theSize;		/* This is the total size of the CMProfileRef */
	linkHeader->cmmId 				= 'Win ';							/* CMM signature,  Registered with ICC consortium  */
	linkHeader->version 			= icVersionNumber;				/* Version of CMProfile format */
	linkHeader->deviceClass 		= icSigLinkClass;					/* input, display, output, devicelink, abstract, or color conversion profile type */
	
	linkHeader->colorSpace			= sCS;	/* color space of data = dataColorSpace of first profile*/

	linkHeader->pcs					= dCS;	/* profile connection color space = dataColorSpace of last profile*/
	
	linkHeader->date.year			= datetimeRec.year;					/* date and time of profile creation */
	linkHeader->date.month			= datetimeRec.month;	
	linkHeader->date.day			= datetimeRec.day;	
	linkHeader->date.hours			= datetimeRec.hour;	
	linkHeader->date.minutes		= datetimeRec.minute;	
	linkHeader->date.seconds		= datetimeRec.second;	
	linkHeader->magic				= icMagicNumber;					/* 'acsp' constant ICC file ID */
	linkHeader->platform 			= icSigMicrosoft;					/* primary profile platform, Registered with ICC consortium */
	linkHeader->flags				= 0;								/* profile flags */
	linkHeader->manufacturer		= icSigMicrosoft;							/* Registered with ICC consortium */
	linkHeader->model				= 0;								/* Registered with ICC consortium */
	linkHeader->attributes[0]		= 0;								/* Attributes like paper type */
	linkHeader->attributes[1]		= 0;
	linkHeader->renderingIntent		= aIntent ;								/* preferred rendering intent of tagged object */
	linkHeader->illuminant.X		= (long)(0.9642 * 65536);					/* profile illuminant */
	linkHeader->illuminant.Y		= (long)(1.0000 * 65536);
	linkHeader->illuminant.Z		= (long)(0.8249 * 65536);
	linkHeader->creator				= 'UJK ';

	return err;
}

CMError MyAdd_NL_DescriptionTag	( LHTextDescriptionType *descPtr, unsigned char *theText )
{
	Ptr						thePtr;
	OSErr					err = noErr;
	
	theText[theText[0]] = 0x00;
	
	/*descPtr = (LHTextDescriptionType*)NewPtrClear( theSize );	 */
	if (descPtr == 0)
		return -1;
	
	/*------------------------------------------------------------------------ ASCII */
	CMHelperICC2int32Const(&(descPtr->typeDescriptor ), icSigTextDescriptionType);
	CMHelperICC2int32Const(&(descPtr->reserved ), 0);
	CMHelperICC2int32Const(&(descPtr->ASCIICount	 ), (long)theText[0]);
	thePtr = (Ptr) &descPtr->ASCIIName[0];
	BlockMove(&theText[1], thePtr, theText[0]);
	thePtr += theText[0];
	
	/*------------------------------------------------------------------------ Unicode */
	/**((unsigned long*)thePtr) = 0;	does not work on some machines ( adress not long word aligned )	/*Unicode code */
	/*thePtr+=sizeof(unsigned long); */
	*thePtr++ = 0;
	*thePtr++ = 0;
	*thePtr++ = 0;
	*thePtr++ = 0;
	/**((unsigned long*)thePtr) = 0;						/*Unicode character count */
	/*thePtr+=sizeof(unsigned long); */
	*thePtr++ = 0;
	*thePtr++ = 0;
	*thePtr++ = 0;
	*thePtr++ = 0;
	/*BlockMove(&theText[1], thePtr, theText[0]);		//Unicode string  */
	/*thePtr += theText[0]; */
	
	/*------------------------------------------------------------------------ Macintosh */
	/**((short*)thePtr) = 0;
	thePtr+=sizeof(short); */
	*thePtr++ = 0;
	*thePtr++ = 0;
	BlockMove(&theText[0], thePtr, theText[0]+1);	
	return err;
}

CMError MyAdd_NL_ColorantTag	( icXYZType *descPtr, MyXYZNumber *aColor )
{
	OSErr					err = noErr;
	
	CMHelperICC2int32Const(&(descPtr->base ), icSigXYZType);
	CMHelperICC2int32Const((OSType*)&(descPtr->base )+1, 0);
	CMHelperICC2int32(&(descPtr->data.data[0].X	 ), &aColor->X);
	CMHelperICC2int32(&(descPtr->data.data[0].Y	 ), &aColor->Y);
	CMHelperICC2int32(&(descPtr->data.data[0].Z	 ), &aColor->Z);
	return err;
}

CMError MyAdd_NL_CurveTag	( icCurveType *descPtr, unsigned short Gamma )
{
	OSErr					err = noErr;
	
	CMHelperICC2int32Const(&(descPtr->base ), icSigCurveType);
	CMHelperICC2int32Const((OSType*)&(descPtr->base )+1, 0);
	CMHelperICC2int32Const(&(descPtr->curve.count ), 1);
	CMHelperICC2int16(&(descPtr->curve.data[0] ), &Gamma );
	return err;
}

CMError MyAdd_NL_CopyrightTag		( unsigned char *copyrightText, LHTextType *aLHTextType )
{
    OSErr			err=noErr;

    copyrightText[copyrightText[0]] = 0x00;
    CMHelperICC2int32Const(&(aLHTextType->base ), icSigTextType);
	CMHelperICC2int32Const((OSType*)&(aLHTextType->base )+1, 0);
    BlockMove(&copyrightText[1], &aLHTextType->text[0], copyrightText[0]);

#ifdef DEBUG_OUTPUT
    if ( err && DebugCheck(kThisFile, kDebugErrorInfo) )
        DebugPrint("¥ MyAdd_NL_CopyrightTag-Error: result = %d\n",err);
#endif
    return err;
}

/* ______________________________________________________________________
    CMError
    Fill_mft1_InputTable	( Ptr				theElut,
                              icLut8*		lutPtr,
                              CMMModelPtr	modelData)

    Abstract:
        Fill mft1 inputTable with the data from the given E-Lut.

    Params:
        theElut		(in)		Reference to E-Lut.
        tempLutPtr	(in/out)	Reference to icLut8.

    Return:
        noErr		successful
        System or result code if an error occurs.
   _____________________________________________________________________ */
CMError Fill_mft1_InputTable(	Ptr			theElut,
                          		icLut8*		lutPtr,
                          		CMMModelPtr	modelData);
CMError Fill_mft1_InputTable(	Ptr			theElut,
                          		icLut8*		lutPtr,
                          		CMMModelPtr	modelData)
{
    OSErr			err = noErr;
    long			i;
    long			j;
    unsigned long	factor;
    unsigned long	round;
    unsigned long	shift;
    unsigned long	ulAux;
    unsigned short	*wordElut = (unsigned short*)theElut;
    unsigned long	newScale = 255;
    unsigned long	oldScale = ((1U<< modelData->lutParam.inputLutWordSize) * (lutPtr->clutPoints-1)) / lutPtr->clutPoints;

    if( modelData->currentCall == kCMMNewLinkProfile ){
		oldScale = 1<< modelData->lutParam.inputLutWordSize;
	}
	factor = (newScale << 24) / oldScale;
    round  = (1<<(24-1))-1;
    shift  = 24;

    ulAux = ~( (1<< (32-modelData->lutParam.inputLutWordSize))-1);
	while (factor & ulAux )	/* stay within 16 bits to prevent product overflow */
    {
        factor >>= 1;
        round  >>= 1;
        shift   -= 1;
    }

    for (i=0; i<(long)lutPtr->inputChan; i++)
    {
        for (j=0; j< 256; j++)
        {
            ulAux    = ((unsigned long)*wordElut * factor + round) >> shift;
            lutPtr->data[i * 256 + j] = (UINT8)(ulAux );
            wordElut++;
        }
    }
    return err;
}

/* ______________________________________________________________________
    CMError
    Fill_mft1_OutputTable	( Ptr			theAlut,
                              icLut8*	lutPtr,
                              long			offset,
                              CMMModelPtr	modelData)

    Abstract:
        Fill mft1 outputTable with the data from the given A-Lut.

    Params:
        theAlut	(in)		Reference to A-Lut.
        lutPtr	(in/out)	Reference to icLut8.
        offset	(in)		starting position for outputTable within icLut8

    Return:
        noErr		successful
        System or  result code if an error occurs.
   _____________________________________________________________________ */
CMError Fill_mft1_OutputTable(	Ptr			theAlut,
                      			icLut8*		lutPtr,
                          		long		offset,
                          		CMMModelPtr	modelData);
CMError Fill_mft1_OutputTable(	Ptr			theAlut,
                      			icLut8*		lutPtr,
                          		long		offset,
                          		CMMModelPtr	modelData)
{
    OSErr	err = noErr;
    long	i;
    long	j;
    UINT8*	byteAlut = (UINT8*)theAlut;
    long	numOfAlutEntries = modelData->lutParam.outputLutEntryCount;

    for (i=0; i<(long)lutPtr->outputChan; i++)
    {
        for (j=0; j<256; j++)
        {
            byteAlut = (UINT8*)theAlut + (j * (numOfAlutEntries-1)) / 255 ;
            lutPtr->data[offset + i * 256 + j] = *byteAlut;
        }
		theAlut +=  numOfAlutEntries;
    }
    return err;
}

/* ______________________________________________________________________
    CMError
    Fill_mft2_InputTable	( Ptr				theElut,
                              icLut16*		lutPtr,
                              CMMModelPtr	modelData)

    Abstract:
        Fill mft2 inputTable with the data from the given E-Lut.

    Params:
        theElut		(in)		Reference to E-Lut.
        tempLutPtr	(in/out)	Reference to icLut8.

    Return:
        noErr		successful
        System or  result code if an error occurs.
   _____________________________________________________________________ */
CMError Fill_mft2_InputTable(	Ptr				theElut,
                          		icLut16*		lutPtr,
                          		CMMModelPtr	modelData);
CMError Fill_mft2_InputTable(	Ptr				theElut,
                          		icLut16*		lutPtr,
                          		CMMModelPtr	modelData)
{
    OSErr			err = noErr;
    unsigned long	factor;
    unsigned long	round;
    unsigned long	shift;
    unsigned long	ulAux;
    long			i;
    long			j;
    UINT16*			wordElut = (UINT16*)theElut;
    UINT16*			wordData = (UINT16*)lutPtr->data;
    unsigned long	newScale = 65535;

    unsigned long	oldScale = ( (1<<modelData->lutParam.inputLutWordSize) * (modelData->lutParam.colorLutGridPoints-1)) / modelData->lutParam.colorLutGridPoints;

    if( modelData->currentCall == kCMMNewLinkProfile ){
		oldScale = 1<< modelData->lutParam.inputLutWordSize;
	}
	factor = (newScale << 15) / oldScale;
    round  = (1<<(15-1))-1;
    shift  = 15;

    while (factor & 0xFFFF0000)	/* stay within 16 bits to prevent product overflow */
    {
        factor >>= 1;
        round  >>= 1;
        shift   -= 1;
    }

     for (i=0; i<(long)lutPtr->inputChan; i++)
    {
        for (j=0; j< (long)lutPtr->inputEnt; j++)
        {
            ulAux    = ((unsigned long)*wordElut * factor + round) >> shift;
			CMHelperICC2int16Const( wordData, ulAux );
			wordData++;
            wordElut++;
        }
    }
    return err;
}
/* ______________________________________________________________________
    CMError
    Fill_mft2_OutputTable	( Ptr				theAlut,
                              icLut16*		lutPtr,
                              long				offset,
                              CMMModelPtr	modelData )

    Abstract:
        Fill mft2 outputTable with the data from the given A-Lut.

    Params:
        theAlut	(in)		Reference to A-Lut.
        lutPtr	(in/out)	Reference to icLut8.
        offset	(in)		starting position for outputTable within CMLut8Type

    Return:
        noErr		successful
        System or  result code if an error occurs.
   _____________________________________________________________________ */
CMError Fill_mft2_OutputTable(	Ptr			theAlut,
                          		icLut16*	lutPtr,
                          		long		offset,
                          		CMMModelPtr	modelData );
CMError Fill_mft2_OutputTable(	Ptr			theAlut,
                          		icLut16*	lutPtr,
                          		long		offset,
                          		CMMModelPtr	modelData )
{
    OSErr	err = noErr;
    long	i;
    long	j;
    UINT16*	wordAlut = (UINT16*)theAlut;
    UINT16*	wordData = (UINT16*)lutPtr->data + offset;

    modelData = modelData;
    for (i=0; i<(long)lutPtr->outputChan; i++)
    {
        for (j=0; j<(long)lutPtr->outputEnt; j++)
        {
            CMHelperICC2int16Const( wordData, *wordAlut );
			wordData++;
            wordAlut++;
        }
    }
    return err;
}

/* ______________________________________________________________________
    CMError
    Fill_mft2_ColorTable	( Ptr			theAlut,
                              icLut16*		lutPtr,
                              long			offset,
                              long 			count )

    Abstract:
        Fill mft2 outputTable with the data from the given A-Lut.

    Params:
        theAlut	(in)		Reference to A-Lut.
        lutPtr	(in/out)	Reference to icLut8.
        offset	(in)		starting position for outputTable within CMLut8Type

    Return:
        noErr		successful
        System or  result code if an error occurs.
   _____________________________________________________________________ */
CMError Fill_mft2_ColorTable(	Ptr			theClut,
                          		icLut16*	lutPtr,
                          		long		offset,
                          		long 		count );
CMError Fill_mft2_ColorTable(	Ptr			theClut,
                          		icLut16*	lutPtr,
                          		long		offset,
                          		long 		count )
{
    OSErr	err = noErr;
    long	i;
    UINT16*	wordClut = (UINT16*)theClut;
    UINT16*	wordData = (UINT16*)lutPtr->data + offset;

    for ( i=0; i<count; i++)
    {
        CMHelperICC2int16Const( wordData, *wordClut );
		wordData++;
        wordClut++;
    }
    return err;
}

CMError MyAdd_NL_AToB0Tag_mft1( CMMModelPtr cw, icLut8Type *lutPtr, long colorLutSize )
{
	CMError			err;
	icLut8			*tempLutPtr;
	long			offset;
	UINT16			inputChannels;
	UINT16			outputChannels;
	UINT16			gridPoints;
	
	LH_START_PROC("Add_NL_AToB0Tag_mft1")

	LOCK_DATA(cw->lutParam.inputLut);
	LOCK_DATA(cw->lutParam.colorLut);
	LOCK_DATA(cw->lutParam.outputLut);
	
	inputChannels  = (UINT16)cw->lutParam.colorLutInDim;
	outputChannels = (UINT16)cw->lutParam.colorLutOutDim;
	gridPoints	   = (UINT16)cw->lutParam.colorLutGridPoints;
	
	tempLutPtr	= &lutPtr->lut;
	CMHelperICC2int32Const(&(lutPtr->base ), icSigLut8Type);
	CMHelperICC2int32Const((OSType*)&(lutPtr->base )+1, 0);
	tempLutPtr->inputChan	= (UINT8)inputChannels;
	tempLutPtr->outputChan	= (UINT8)outputChannels;
	tempLutPtr->clutPoints 	= (UINT8)gridPoints;
	tempLutPtr->e00 = tempLutPtr->e01 = tempLutPtr->e02 = 0;
	tempLutPtr->e10 = tempLutPtr->e11 = tempLutPtr->e12 = 0;
	tempLutPtr->e20 = tempLutPtr->e21 = tempLutPtr->e22 = 0;
	CMHelperICC2int32Const(&(tempLutPtr->e00	), 0x10000);
	CMHelperICC2int32Const(&(tempLutPtr->e11	), 0x10000);
	CMHelperICC2int32Const(&(tempLutPtr->e22	), 0x10000);

	err = Fill_mft1_InputTable((Ptr)DATA_2_PTR(cw->lutParam.inputLut), tempLutPtr, cw);
	if (err)
		goto CleanupAndExit;

	offset = 256 * inputChannels;
	BlockMoveData( DATA_2_PTR(cw->lutParam.colorLut), & tempLutPtr->data[0] + offset, colorLutSize);

	offset = offset + colorLutSize;
	err = Fill_mft1_OutputTable((Ptr)DATA_2_PTR(cw->lutParam.outputLut),  tempLutPtr, offset, cw);
	if (err)
		goto CleanupAndExit;

CleanupAndExit:

	#ifdef DEBUG_OUTPUT
	if ( err  )
		DebugPrint("¥ MyAdd_NL_AToB0Tag_mft1-Error: result = %d\n",err);
	#endif
	UNLOCK_DATA(cw->lutParam.inputLut);
	UNLOCK_DATA(cw->lutParam.colorLut);
	UNLOCK_DATA(cw->lutParam.outputLut);
	LH_END_PROC("Add_NL_AToB0Tag_mft1")
	return err;
}

UINT32 GetSizes( CMMModelPtr cw, UINT32 *clutSize );
UINT32 GetSizes( CMMModelPtr cw, UINT32 *clutSize )
{
	UINT16			inputChannels;
	UINT16			outputChannels;
	UINT16			gridPoints;
	SINT32			colorLutSize;
	UINT32			i,theSize;

	LOCK_DATA(cw->lutParam.inputLut);
	LOCK_DATA(cw->lutParam.colorLut);
	LOCK_DATA(cw->lutParam.outputLut);
	
	inputChannels  = (UINT16)cw->lutParam.colorLutInDim;
	outputChannels = (UINT16)cw->lutParam.colorLutOutDim;
	gridPoints	   = (UINT16)cw->lutParam.colorLutGridPoints;
	
	colorLutSize = outputChannels;
	for(i = 0; i < inputChannels; i++)
		colorLutSize *= gridPoints;

	if ( cw->lutParam.colorLutWordSize != 8){
		theSize = sizeof(OSType) + sizeof(UINT32) + (4 * sizeof(UINT8)) + (9 * sizeof(Fixed))			/* typeDescriptor...matrix */
					+ 2 * sizeof(icUInt16Number) 														/* inputLutEntryCount outputLutEntryCount*/
					+ (inputChannels * cw->lutParam.inputLutEntryCount * sizeof(UINT16)) 				/* inputTable */
					+ (outputChannels * cw->lutParam.outputLutEntryCount * sizeof(UINT16)) 				/* CLUT */
					+ colorLutSize * sizeof(UINT16);																		/* outputTable */
	}
	else{
		theSize = sizeof(OSType) + sizeof(UINT32) + (4 * sizeof(UINT8)) + (9 * sizeof(Fixed))			/* typeDescriptor...matrix */
					+ (inputChannels * 256 * sizeof(UINT8)) 											/* inputTable */
					+ (outputChannels * 256 * sizeof(UINT8)) 											/* CLUT */
					+ colorLutSize;																		/* outputTable */
	}
	*clutSize = colorLutSize;
	UNLOCK_DATA(cw->lutParam.inputLut);
	UNLOCK_DATA(cw->lutParam.colorLut);
	UNLOCK_DATA(cw->lutParam.outputLut);
	return theSize;
}

CMError MyAdd_NL_AToB0Tag_mft2( CMMModelPtr cw, icLut16Type *lutPtr, long colorLutSize )
{
	CMError			err;
	icLut16			*tempLutPtr;
	long			offset;
	UINT16			inputChannels;
	UINT16			outputChannels;
	UINT16			gridPoints;
	
	LH_START_PROC("Add_NL_AToB0Tag_mft2")

	LOCK_DATA(cw->lutParam.inputLut);
	LOCK_DATA(cw->lutParam.colorLut);
	LOCK_DATA(cw->lutParam.outputLut);
	
	inputChannels  = (UINT16)cw->lutParam.colorLutInDim;
	outputChannels = (UINT16)cw->lutParam.colorLutOutDim;
	gridPoints	   = (UINT16)cw->lutParam.colorLutGridPoints;
	
	tempLutPtr	= &lutPtr->lut;
	CMHelperICC2int32Const(&(lutPtr->base ), icSigLut16Type);
	CMHelperICC2int32Const((OSType*)&(lutPtr->base )+1, 0);
	tempLutPtr->inputChan	= (UINT8)inputChannels;
	tempLutPtr->outputChan	= (UINT8)outputChannels;
	tempLutPtr->clutPoints 	= (UINT8)((gridPoints>255)?255:gridPoints);
	tempLutPtr->e00 = tempLutPtr->e01 = tempLutPtr->e02 = 0;
	tempLutPtr->e10 = tempLutPtr->e11 = tempLutPtr->e12 = 0;
	tempLutPtr->e20 = tempLutPtr->e21 = tempLutPtr->e22 = 0;
	CMHelperICC2int32Const(&(tempLutPtr->e00	), 0x10000);
	CMHelperICC2int32Const(&(tempLutPtr->e11	), 0x10000);
	CMHelperICC2int32Const(&(tempLutPtr->e22	), 0x10000);

	tempLutPtr->inputEnt	= (UINT16)cw->lutParam.inputLutEntryCount;
	tempLutPtr->outputEnt	= (UINT16)cw->lutParam.outputLutEntryCount;
	err = Fill_mft2_InputTable((Ptr)DATA_2_PTR(cw->lutParam.inputLut), tempLutPtr, cw);
	if (err)
		goto CleanupAndExit;

	offset = cw->lutParam.inputLutEntryCount * inputChannels;
	Fill_mft2_ColorTable( (Ptr)DATA_2_PTR(cw->lutParam.colorLut), tempLutPtr, offset, colorLutSize);

	offset = offset + colorLutSize;
	err = Fill_mft2_OutputTable((Ptr)DATA_2_PTR(cw->lutParam.outputLut),  tempLutPtr, offset, cw);
	if (err)
		goto CleanupAndExit;

	CMHelperICC2int16Const(&(tempLutPtr->inputEnt	), (UINT16)cw->lutParam.inputLutEntryCount);
	CMHelperICC2int16Const(&(tempLutPtr->outputEnt	), (UINT16)cw->lutParam.outputLutEntryCount);

CleanupAndExit:

	#ifdef DEBUG_OUTPUT
	if ( err  )
		DebugPrint("¥ MyAdd_NL_AToB0Tag_mft2-Error: result = %d\n",err);
	#endif
	UNLOCK_DATA(cw->lutParam.inputLut);
	UNLOCK_DATA(cw->lutParam.colorLut);
	UNLOCK_DATA(cw->lutParam.outputLut);
	LH_END_PROC("Add_NL_AToB0Tag_mft2")
	return err;
}

CMError MyGetColorSpaces(	CMConcatProfileSet	*profileSet,
							UINT32				*sCS,
							UINT32				*dCS );
CMError MyGetColorSpaces(	CMConcatProfileSet	*profileSet,
							UINT32				*sCS,
							UINT32				*dCS )
{
	CMError						err;
	icHeader					profHeader;

	LH_START_PROC("MyGetColorSpaces")
	
	err = CMGetProfileHeader(profileSet->profileSet[0], (CMCoreProfileHeader *)&profHeader);
	if (err)
		goto CleanupAndExit;
	*sCS = profHeader.colorSpace;

	err = CMGetProfileHeader(profileSet->profileSet[profileSet->count-1], (CMCoreProfileHeader *)&profHeader);
	if (err)
		goto CleanupAndExit;
	*dCS = profHeader.colorSpace;

CleanupAndExit:
	LH_END_PROC("MyAdd_NL_SequenceDescTag")
	return err;
}
/* ______________________________________________________________________
	CMError
	MyAdd_NL_SequenceDescTag	( CMProfileRef 		 linkProfile,
							  CMConcatProfileSet *profileSet )

	Abstract:
		Create the ProfileSequenceDescTag for the NewDeviceLink CMProfileRef.
		Copy the data from the profiles in profileSet

	Params:
		linkProfile	(in/out)	Reference to new profile.
		profileSet	(in)		Reference to CMConcatProfileSet.
		
	Return:
		noErr		successful
		System or  result code if an error occurs.
   _____________________________________________________________________ */
CMError MyAdd_NL_SequenceDescTag(	CMConcatProfileSet			*profileSet,
						  			icProfileSequenceDescType	*pSeqPtr,
						  			long						*aSize )
{
	CMError						err;
	OSErr						aOSerr;
	/*icProfileSequenceDescType*	pSeqPtr 	= nil;*/
	Ptr							thePtr 		= nil;
	icHeader					profHeader;
	SINT32						loop;
	OSType						technology;
	UINT32 						elementSize;
	icSignatureType				theSignature;
	icTextDescriptionType*		descPtr 	= nil;
	char						descPtrNull[90]={0};

	LH_START_PROC("MyAdd_NL_SequenceDescTag")
	
	CMHelperICC2int32Const(&(((icTextDescriptionType*)descPtrNull)->base ), icSigTextDescriptionType );
	CMHelperICC2int32Const(((OSType*)&((icTextDescriptionType*)descPtrNull)->base )+1, 0);
	CMHelperICC2int32Const(&(((icTextDescriptionType*)descPtrNull)->desc.count ), 1 );
	/*pSeqPtr =  (icProfileSequenceDescType*)SmartNewPtrClear(5000, &aOSerr);
	err = aOSerr;
	if (err)
		goto CleanupAndExit;*/
	CMHelperICC2int32Const(&(pSeqPtr->base ), icSigProfileSequenceDescType);
	CMHelperICC2int32Const((OSType*)&(pSeqPtr->base )+1, 0);
	CMHelperICC2int32Const(&(pSeqPtr->desc.count ), profileSet->count );
	thePtr = (Ptr)( (SIZE_T)&(pSeqPtr->desc.count) + sizeof(SINT32) ) ;		
	
	for (loop = 0; loop < (SINT32)profileSet->count; loop++)
	{
		err = CMGetProfileHeader(profileSet->profileSet[loop], (CMCoreProfileHeader *)&profHeader);
		if (err)
			goto CleanupAndExit;
		CMHelperICC2int32Const(thePtr, profHeader.manufacturer);
		thePtr += sizeof(OSType);
		CMHelperICC2int32Const(thePtr, profHeader.model);
		thePtr += sizeof(OSType);
		CMHelperICC2int32Const(thePtr, profHeader.attributes[0]);
		thePtr += sizeof(UINT32);
		CMHelperICC2int32Const(thePtr, profHeader.attributes[1]);
		thePtr += sizeof(UINT32);
		
		/* ----------------------------------------------------------------- icSigTechnologyTag */
		technology = 0;
		if ( CMGetProfileElement(profileSet->profileSet[loop], icSigTechnologyTag, &elementSize, nil) == noErr)
		{
			if (elementSize == sizeof(icSigTechnologyTag))
			{
				if ( CMGetProfileElement(profileSet->profileSet[loop], icSigTechnologyTag, &elementSize, &theSignature) == noErr)
					technology = theSignature.signature;
			}
		}
		*((OSType*)thePtr) = technology;									/* signature */
		thePtr += sizeof(OSType);
		
		/* ----------------------------------------------------------------- icSigDeviceMfgDescTag */
		if ( CMGetProfileElement(profileSet->profileSet[loop], icSigDeviceMfgDescTag, &elementSize, nil) == noErr)
		{
			descPtr = (icTextDescriptionType*)SmartNewPtr(elementSize,&aOSerr);
			err = aOSerr;
			if (err)
				goto CleanupAndExit;
			if ( CMGetProfileElement(profileSet->profileSet[loop], icSigDeviceMfgDescTag, &elementSize, descPtr) == noErr)
			{
				BlockMoveData( descPtr, thePtr, elementSize);
				thePtr += elementSize;
			}
			descPtr = (icTextDescriptionType*)DisposeIfPtr((Ptr)descPtr);
		} else
		{
			BlockMoveData( descPtrNull, thePtr, sizeof(descPtrNull));
			thePtr += sizeof(descPtrNull);
		}
		/* ----------------------------------------------------------------- icSigDeviceModelDescTag */
		if ( CMGetProfileElement(profileSet->profileSet[loop], icSigDeviceModelDescTag, &elementSize, nil) == noErr)
		{
			descPtr = (icTextDescriptionType*)SmartNewPtr(elementSize,&aOSerr);
			err = aOSerr;
			if (err)
				goto CleanupAndExit;
			if ( CMGetProfileElement(profileSet->profileSet[loop], icSigDeviceModelDescTag, &elementSize, descPtr) == noErr)
			{
				BlockMoveData( descPtr, thePtr, elementSize);
				thePtr += elementSize;
			}
			descPtr =  (icTextDescriptionType*)DisposeIfPtr((Ptr)descPtr);
		} else
		{
			BlockMoveData( descPtrNull, thePtr, sizeof(descPtrNull));
			thePtr += sizeof(descPtrNull);
		}
		/* ----------------------------------------------------------------- */
	}

   //Sundown safe truncation
	*aSize = (LONG)((ULONG_PTR)thePtr - (ULONG_PTR)pSeqPtr);
	
CleanupAndExit:
	descPtr = (icTextDescriptionType*)DisposeIfPtr((Ptr)descPtr);
	LH_END_PROC("MyAdd_NL_SequenceDescTag")
	return err;
}


MyXYZNumber MakeXYZNumber( MyXYZNumber *x );
MyXYZNumber MakeXYZNumber( MyXYZNumber *x )
{
	MyXYZNumber ret;
	ret.X = x->X>>14;
	ret.Y = x->Y>>14;
	ret.Z = x->Z>>14;
	return ret;
}

void  MakeMyDoubleXYZ( MyXYZNumber *x, MyDoubleXYZ *ret );
void  MakeMyDoubleXYZ( MyXYZNumber *x, MyDoubleXYZ *ret )
{
	ret->X = x->X/(double)(1<<30);
	ret->Y = x->Y/(double)(1<<30);
	ret->Z = x->Z/(double)(1<<30);
}
Boolean  doubMatrixInvert(	double	inpMat[3][3],
						double	outMat[3][3] );
void  TransposeMatrix(	double	inpMat[3][3] );
void  TransposeMatrix(	double	inpMat[3][3] )
{
	long i,j;
	double sav;
	for( i=0; i< 3; i++ ){
		for( j=i+1; j< 3; j++ ){
			sav			 =  inpMat[i][j];
			inpMat[i][j] =  inpMat[j][i];
			inpMat[j][i] =  sav;
		}
	}

}
#define Round(a) (((a)>0.)?((a)+.5):((a)-.5))

double GetMatrixedVal( double m[3][3], long color, MyDoubleXYZ *aXYZ );
double GetMatrixedVal( double m[3][3], long color, MyDoubleXYZ *aXYZ )
{
	double tmp;
	tmp = m[color][0] * aXYZ->X;
	tmp+= m[color][1] * aXYZ->Y;
	tmp+= m[color][2] * aXYZ->Z;
	return tmp;
}

void NormalizeColor( MyXYZNumber *r, MyXYZNumber *g, MyXYZNumber *b, MyDoubleXYZ *Illuminant, MyXYZNumber *resR, MyXYZNumber *resG, MyXYZNumber *resB );
void NormalizeColor( MyXYZNumber *r, MyXYZNumber *g, MyXYZNumber *b, MyDoubleXYZ *Illuminant, MyXYZNumber *resR, MyXYZNumber *resG, MyXYZNumber *resB )
{
	double 	factorX,factorY,factorZ;
	double inMat[3][3];
	
	MakeMyDoubleXYZ( r, (MyDoubleXYZ*)&inMat[0][0] );
	MakeMyDoubleXYZ( g, (MyDoubleXYZ*)&inMat[1][0] );
	MakeMyDoubleXYZ( b, (MyDoubleXYZ*)&inMat[2][0] );

	factorX = inMat[0][0] + inMat[1][0] + inMat[2][0];
	factorY = inMat[0][1] + inMat[1][1] + inMat[2][1];
	factorZ = inMat[0][2] + inMat[1][2] + inMat[2][2];

	factorX = Illuminant->X / factorX * 65536;
	factorY = Illuminant->Y / factorY * 65536;
	factorZ = Illuminant->Z / factorZ * 65536;

	resR->X = (long)Round(inMat[0][0] * factorX);			/* red primary */
	resR->Y = (long)Round(inMat[0][1] * factorY);
	resR->Z = (long)Round(inMat[0][2] * factorZ);
	resG->X = (long)Round(inMat[1][0] * factorX);			/* green primary */
	resG->Y = (long)Round(inMat[1][1] * factorY);
	resG->Z = (long)Round(inMat[1][2] * factorZ);
	resB->X = (long)Round(inMat[2][0] * factorX);			/* blue primary */
	resB->Y = (long)Round(inMat[2][1] * factorY);
	resB->Z = (long)Round(inMat[2][2] * factorZ);
}

void NormalizeWithWhiteAdaption( MyXYZNumber *r, MyXYZNumber *g, MyXYZNumber *b, MyDoubleXYZ *Illuminant, MyXYZNumber *resR, MyXYZNumber *resG, MyXYZNumber *resB );
void NormalizeWithWhiteAdaption( MyXYZNumber *r, MyXYZNumber *g, MyXYZNumber *b, MyDoubleXYZ *Illuminant, MyXYZNumber *resR, MyXYZNumber *resG, MyXYZNumber *resB )
{
	double 	factorR,factorG,factorB;
	double inMat[3][3];
	double outMat[3][3];

	MakeMyDoubleXYZ( r, (MyDoubleXYZ*)&inMat[0][0] );
	MakeMyDoubleXYZ( g, (MyDoubleXYZ*)&inMat[1][0] );
	MakeMyDoubleXYZ( b, (MyDoubleXYZ*)&inMat[2][0] );

	if( !doubMatrixInvert(inMat, outMat) )
	{
		return ;
	}

	TransposeMatrix( outMat );

	factorR =  GetMatrixedVal( outMat, 0, Illuminant )*65536;
	factorG =  GetMatrixedVal( outMat, 1, Illuminant )*65536;
	factorB =  GetMatrixedVal( outMat, 2, Illuminant )*65536;

	resR->X = (long)Round(inMat[0][0] * factorR);			/* red primary */
	resR->Y = (long)Round(inMat[0][1] * factorR);
	resR->Z = (long)Round(inMat[0][2] * factorR);
	resG->X = (long)Round(inMat[1][0] * factorG);			/* green primary */
	resG->Y = (long)Round(inMat[1][1] * factorG);
	resG->Z = (long)Round(inMat[1][2] * factorG);
	resB->X = (long)Round(inMat[2][0] * factorB);			/* blue primary */
	resB->Y = (long)Round(inMat[2][1] * factorB);
	resB->Z = (long)Round(inMat[2][2] * factorB);
}

#ifdef WRITE_PROFILE
void WriteProf( Ptr name, icProfile *theProf, long currentSize );
#endif

#define MyTagCount 9

#if __IS_MSDOS && defined(RenderInt)
CMError MyNewAbstract( LPLOGCOLORSPACEA	lpColorSpace, icProfile **theProf )
{
	CMError			err = unimpErr;
	OSErr			aOSerr = unimpErr;
#ifdef __MWERKS__
	unsigned char	theText[] = "\pLogColorSpProfile   ";
	char			copyrightText[] = "\p©1996 by Heidelberger Druckmaschinen AG  U.J.K.";
#elif __IS_MSDOS
	char			theText[] = "\030LogColorSpProfile      ";
	char			copyrightText[] = "\060©1996 by Heidelberger Druckmaschinen AG  U.J.K.";
#else
	char			theText[] = "\030LogColorSpProfile      ";
	char			copyrightText[] = "\060©1996 by Heidelberger Druckmaschinen AG  U.J.K.";
#endif
	icProfile 		*aProf=0;
	long 			theTagTabSize;
	long 			theHeaderSize;
	long 			theDescSize;
	long 			theMediaSize;
	long 			theEndPointSize;
	long 			theTRCSize;
	long 			theCopyRightSize;
	long 			currentSize=0;
	long 			theTotalSize=0;
	icTag			aTag;
	unsigned long	aIntent;

	MyXYZNumber	rXYZ,gXYZ,bXYZ;
	MyDoubleXYZ D50XYZ = { 0.9642, 1.0000, 0.8249 };
	MyXYZNumber D50 = { (unsigned long)(D50XYZ.X * 65536), (unsigned long)(D50XYZ.Y * 65536), (unsigned long)(D50XYZ.Z * 65536)};
	theHeaderSize = sizeof(icHeader);
	theDescSize   = 									  sizeof(OSType) 			/* type descriptor */
														+ sizeof(unsigned long) 	/* reserved */
														+ sizeof(unsigned long) 	/* ASCII length */
														+ theText[0]				/* ASCII profile description */
														+ sizeof(unsigned long)		/* Unicode code */
														+ sizeof(unsigned long)		/* Unicode character count */
														+ sizeof(unsigned short)	/* Macintosh script code */
														+ sizeof(unsigned char)		/* Macintosh string length */
														+ 67						/* Macintosh string */
														;
	theCopyRightSize   = 								  sizeof(OSType) 			/* type descriptor */
														+ sizeof(unsigned long) 	/* reserved */
														+ copyrightText[0]			/* ASCII profile description */
														;
	theMediaSize = sizeof( icXYZType );;
	theEndPointSize = sizeof( icXYZType );;
	theTRCSize = 4*((sizeof( icCurveType ) +3)/4);

	theTagTabSize = MyTagCount * sizeof( icTag ) + sizeof( unsigned long );
	
	theTotalSize = theHeaderSize + theTagTabSize + theDescSize + theCopyRightSize
		+ theMediaSize + 3*theEndPointSize + 3*theTRCSize;
	aProf = (icProfile *)GlobalAllocPtr( GHND, theTotalSize );
	if( aProf == 0 ){
		err = GetLastError();
		goto CleanupAndExit;
	}

	switch( lpColorSpace->lcsIntent ){
	  case LCS_GM_BUSINESS:
		aIntent = icSaturation;
		break;
	  case LCS_GM_GRAPHICS:
		aIntent = icRelativeColorimetric;
		break;
	  case LCS_GM_ABS_COLORIMETRIC:
		aIntent = icAbsoluteColorimetric;
		break;
	  default:
		aIntent = icPerceptual;
		break;
	}
	err = MyAdd_NL_Header(theTotalSize, (icHeader*)((Ptr)aProf+currentSize), aIntent, icSigInputClass, icSigRgbData, icSigXYZData );
	if (err)
		goto CleanupAndExit;
		
	/*----------------------------------------------------------------------------------------- cmProfileDescriptionTag */
	currentSize = theHeaderSize + theTagTabSize;
	CMHelperICC2int32Const(&(aProf->tagList.count ), MyTagCount);

	CMHelperICC2int32Const(&(aTag.sig ), icSigProfileDescriptionTag);
	CMHelperICC2int32Const(&(aTag.offset ), currentSize);
	CMHelperICC2int32Const(&(aTag.size ), theDescSize);
	aProf->tagList.tags[0] = aTag;
	err =MyAdd_NL_DescriptionTag	( (LHTextDescriptionType *)((Ptr)aProf+currentSize), (unsigned char *)theText );
	if (err)
		goto CleanupAndExit;
	currentSize += theDescSize;

	CMHelperICC2int32Const(&(aTag.sig ), icSigMediaWhitePointTag);
	CMHelperICC2int32Const(&(aTag.offset ), currentSize );
	CMHelperICC2int32Const(&(aTag.size ), theMediaSize);
	aProf->tagList.tags[1] = aTag;
	MyAdd_NL_ColorantTag((icXYZType *)((Ptr)aProf+currentSize), &D50);
	currentSize += theMediaSize;

	NormalizeColor(	(icXYZNumber*)&lpColorSpace->lcsEndpoints.ciexyzRed,
					(icXYZNumber*)&lpColorSpace->lcsEndpoints.ciexyzGreen,
					(icXYZNumber*)&lpColorSpace->lcsEndpoints.ciexyzBlue,
					&D50XYZ,
					&rXYZ,
					&gXYZ,
					&bXYZ );
	CMHelperICC2int32Const(&(aTag.sig ), icSigRedColorantTag);
	CMHelperICC2int32Const(&(aTag.offset ), currentSize );
	CMHelperICC2int32Const(&(aTag.size ), theEndPointSize);
	aProf->tagList.tags[2] = aTag;
	MyAdd_NL_ColorantTag((icXYZType *)((Ptr)aProf+currentSize), &rXYZ );
	currentSize += theEndPointSize;

	CMHelperICC2int32Const(&(aTag.sig ), icSigGreenColorantTag);
	CMHelperICC2int32Const(&(aTag.offset ), currentSize );
	CMHelperICC2int32Const(&(aTag.size ), theEndPointSize);
	aProf->tagList.tags[3] = aTag;
	MyAdd_NL_ColorantTag((icXYZType *)((Ptr)aProf+currentSize), &gXYZ );
	currentSize += theEndPointSize;

	CMHelperICC2int32Const(&(aTag.sig ), icSigBlueColorantTag);
	CMHelperICC2int32Const(&(aTag.offset ), currentSize );
	CMHelperICC2int32Const(&(aTag.size ), theEndPointSize);
	aProf->tagList.tags[4] = aTag;
	MyAdd_NL_ColorantTag((icXYZType *)((Ptr)aProf+currentSize), &bXYZ );
	currentSize += theEndPointSize;

	CMHelperICC2int32Const(&(aTag.sig ), icSigRedTRCTag);
	CMHelperICC2int32Const(&(aTag.offset ), currentSize );
	CMHelperICC2int32Const(&(aTag.size ), sizeof( icCurveType ));
	aProf->tagList.tags[5] = aTag;
	MyAdd_NL_CurveTag((icCurveType  *)((Ptr)aProf+currentSize), (unsigned short)(lpColorSpace->lcsGammaRed>>8) );
	currentSize += theTRCSize;

	CMHelperICC2int32Const(&(aTag.sig ), icSigGreenTRCTag);
	CMHelperICC2int32Const(&(aTag.offset ), currentSize );
	CMHelperICC2int32Const(&(aTag.size ), sizeof( icCurveType ));
	aProf->tagList.tags[6] = aTag;
	MyAdd_NL_CurveTag((icCurveType  *)((Ptr)aProf+currentSize), (unsigned short)(lpColorSpace->lcsGammaGreen>>8) );
	currentSize += theTRCSize;

	CMHelperICC2int32Const(&(aTag.sig ), icSigBlueTRCTag);
	CMHelperICC2int32Const(&(aTag.offset ), currentSize );
	CMHelperICC2int32Const(&(aTag.size ), sizeof( icCurveType ));
	aProf->tagList.tags[7] = aTag;
	MyAdd_NL_CurveTag((icCurveType  *)((Ptr)aProf+currentSize), (unsigned short)(lpColorSpace->lcsGammaBlue>>8) );
	currentSize += theTRCSize;

	CMHelperICC2int32Const(&(aTag.sig ), icSigCopyrightTag);
	CMHelperICC2int32Const(&(aTag.offset ), currentSize );
	CMHelperICC2int32Const(&(aTag.size ), theCopyRightSize);
	aProf->tagList.tags[8] = aTag;
	err = MyAdd_NL_CopyrightTag( (unsigned char *)copyrightText, (LHTextType *)((Ptr)aProf+currentSize));
	if (err)
		goto CleanupAndExit;
	currentSize += theCopyRightSize;
		
	*theProf = aProf;
#ifdef WRITE_PROFILE
	WriteProf( "MyNewAbstract.pf", aProf, currentSize );
#endif
	return noErr;
	
CleanupAndExit:
	if( aProf )GlobalFreePtr( aProf );
	return err;
}


CMError MyNewAbstractW( LPLOGCOLORSPACEW	lpColorSpace, icProfile **theProf )
{
	CMError			err = unimpErr;
	OSErr			aOSerr = unimpErr;
#ifdef __MWERKS__
	unsigned char	theText[] = "\pLogColorSpProfile   ";
	char			copyrightText[] = "\p©1996 by Heidelberger Druckmaschinen AG  U.J.K.";
#elif __IS_MSDOS
	char			theText[] = "\030LogColorSpProfile      ";
	char			copyrightText[] = "\060©1996 by Heidelberger Druckmaschinen AG  U.J.K.";
#else
	char			theText[] = "\030LogColorSpProfile      ";
	char			copyrightText[] = "\060©1996 by Heidelberger Druckmaschinen AG  U.J.K.";
#endif
	icProfile 		*aProf=0;
	long 			theTagTabSize;
	long 			theHeaderSize;
	long 			theDescSize;
	long 			theMediaSize;
	long 			theEndPointSize;
	long 			theTRCSize;
	long 			theCopyRightSize;
	long 			currentSize=0;
	long 			theTotalSize=0;
	icTag			aTag;
	unsigned long	aIntent;

	MyXYZNumber	rXYZ,gXYZ,bXYZ;
	MyDoubleXYZ D50XYZ = { 0.9642, 1.0000, 0.8249 };
	MyXYZNumber D50 = { (unsigned long)(D50XYZ.X * 65536), (unsigned long)(D50XYZ.Y * 65536), (unsigned long)(D50XYZ.Z * 65536)};
	theHeaderSize = sizeof(icHeader);
	theDescSize   = 									  sizeof(OSType) 			/* type descriptor */
														+ sizeof(unsigned long) 	/* reserved */
														+ sizeof(unsigned long) 	/* ASCII length */
														+ theText[0]				/* ASCII profile description */
														+ sizeof(unsigned long)		/* Unicode code */
														+ sizeof(unsigned long)		/* Unicode character count */
														+ sizeof(unsigned short)	/* Macintosh script code */
														+ sizeof(unsigned char)		/* Macintosh string length */
														+ 67						/* Macintosh string */
														;
	theCopyRightSize   = 								  sizeof(OSType) 			/* type descriptor */
														+ sizeof(unsigned long) 	/* reserved */
														+ copyrightText[0]			/* ASCII profile description */
														;
	theMediaSize = sizeof( icXYZType );;
	theEndPointSize = sizeof( icXYZType );;
	theTRCSize = 4*((sizeof( icCurveType ) +3)/4);

	theTagTabSize = MyTagCount * sizeof( icTag ) + sizeof( unsigned long );
	
	theTotalSize = theHeaderSize + theTagTabSize + theDescSize + theCopyRightSize
		+ theMediaSize + 3*theEndPointSize + 3*theTRCSize;
	aProf = (icProfile *)GlobalAllocPtr( GHND, theTotalSize );
	if( aProf == 0 ){
		err = GetLastError();
		goto CleanupAndExit;
	}

	switch( lpColorSpace->lcsIntent ){
	  case LCS_GM_BUSINESS:
		aIntent = icSaturation;
		break;
	  case LCS_GM_GRAPHICS:
		aIntent = icRelativeColorimetric;
		break;
	  case LCS_GM_GRAPHICS+1:
		aIntent = icAbsoluteColorimetric;
		break;
	  default:
		aIntent = icPerceptual;
		break;
	}
	err = MyAdd_NL_Header(theTotalSize, (icHeader*)((Ptr)aProf+currentSize), aIntent, icSigInputClass, icSigRgbData, icSigXYZData );
	if (err)
		goto CleanupAndExit;
		
	/*----------------------------------------------------------------------------------------- cmProfileDescriptionTag */
	currentSize = theHeaderSize + theTagTabSize;
	CMHelperICC2int32Const(&(aProf->tagList.count ), MyTagCount);

	CMHelperICC2int32Const(&(aTag.sig ), icSigProfileDescriptionTag);
	CMHelperICC2int32Const(&(aTag.offset ), currentSize);
	CMHelperICC2int32Const(&(aTag.size ), theDescSize);
	aProf->tagList.tags[0] = aTag;
	err =MyAdd_NL_DescriptionTag	( (LHTextDescriptionType *)((Ptr)aProf+currentSize), (unsigned char *)theText );
	if (err)
		goto CleanupAndExit;
	currentSize += theDescSize;

	CMHelperICC2int32Const(&(aTag.sig ), icSigMediaWhitePointTag);
	CMHelperICC2int32Const(&(aTag.offset ), currentSize );
	CMHelperICC2int32Const(&(aTag.size ), theMediaSize);
	aProf->tagList.tags[1] = aTag;
	MyAdd_NL_ColorantTag((icXYZType *)((Ptr)aProf+currentSize), &D50);
	currentSize += theMediaSize;

	NormalizeColor(	(icXYZNumber*)&lpColorSpace->lcsEndpoints.ciexyzRed,
					(icXYZNumber*)&lpColorSpace->lcsEndpoints.ciexyzGreen,
					(icXYZNumber*)&lpColorSpace->lcsEndpoints.ciexyzBlue,
					&D50XYZ,
					&rXYZ,
					&gXYZ,
					&bXYZ );
	CMHelperICC2int32Const(&(aTag.sig ), icSigRedColorantTag);
	CMHelperICC2int32Const(&(aTag.offset ), currentSize );
	CMHelperICC2int32Const(&(aTag.size ), theEndPointSize);
	aProf->tagList.tags[2] = aTag;
	MyAdd_NL_ColorantTag((icXYZType *)((Ptr)aProf+currentSize), &rXYZ );
	currentSize += theEndPointSize;

	CMHelperICC2int32Const(&(aTag.sig ), icSigGreenColorantTag);
	CMHelperICC2int32Const(&(aTag.offset ), currentSize );
	CMHelperICC2int32Const(&(aTag.size ), theEndPointSize);
	aProf->tagList.tags[3] = aTag;
	MyAdd_NL_ColorantTag((icXYZType *)((Ptr)aProf+currentSize), &gXYZ );
	currentSize += theEndPointSize;

	CMHelperICC2int32Const(&(aTag.sig ), icSigBlueColorantTag);
	CMHelperICC2int32Const(&(aTag.offset ), currentSize );
	CMHelperICC2int32Const(&(aTag.size ), theEndPointSize);
	aProf->tagList.tags[4] = aTag;
	MyAdd_NL_ColorantTag((icXYZType *)((Ptr)aProf+currentSize), &bXYZ );
	currentSize += theEndPointSize;

	CMHelperICC2int32Const(&(aTag.sig ), icSigRedTRCTag);
	CMHelperICC2int32Const(&(aTag.offset ), currentSize );
	CMHelperICC2int32Const(&(aTag.size ), sizeof( icCurveType ));
	aProf->tagList.tags[5] = aTag;
	MyAdd_NL_CurveTag((icCurveType  *)((Ptr)aProf+currentSize), (unsigned short)(lpColorSpace->lcsGammaRed>>8) );
	currentSize += theTRCSize;

	CMHelperICC2int32Const(&(aTag.sig ), icSigGreenTRCTag);
	CMHelperICC2int32Const(&(aTag.offset ), currentSize );
	CMHelperICC2int32Const(&(aTag.size ), sizeof( icCurveType ));
	aProf->tagList.tags[6] = aTag;
	MyAdd_NL_CurveTag((icCurveType  *)((Ptr)aProf+currentSize), (unsigned short)(lpColorSpace->lcsGammaGreen>>8) );
	currentSize += theTRCSize;

	CMHelperICC2int32Const(&(aTag.sig ), icSigBlueTRCTag);
	CMHelperICC2int32Const(&(aTag.offset ), currentSize );
	CMHelperICC2int32Const(&(aTag.size ), sizeof( icCurveType ));
	aProf->tagList.tags[7] = aTag;
	MyAdd_NL_CurveTag((icCurveType  *)((Ptr)aProf+currentSize), (unsigned short)(lpColorSpace->lcsGammaBlue>>8) );
	currentSize += theTRCSize;

	CMHelperICC2int32Const(&(aTag.sig ), icSigCopyrightTag);
	CMHelperICC2int32Const(&(aTag.offset ), currentSize );
	CMHelperICC2int32Const(&(aTag.size ), theCopyRightSize);
	aProf->tagList.tags[8] = aTag;
	err = MyAdd_NL_CopyrightTag( (unsigned char *)copyrightText, (LHTextType *)((Ptr)aProf+currentSize));
	if (err)
		goto CleanupAndExit;
	currentSize += theCopyRightSize;
		
	*theProf = aProf;
#ifdef WRITE_PROFILE
	WriteProf( "MyNewAbstractW.pf", aProf, currentSize );
#endif
	return noErr;
	
CleanupAndExit:
	if( aProf )GlobalFreePtr( aProf );
	return err;
}
#endif

#define MyTagCountLink 5
#define LINK_BUFFER_MAX 3000

CMError DeviceLinkFill(	CMMModelPtr cw,
						CMConcatProfileSet *profileSet,
						icProfile **theProf,
						unsigned long aIntent )
{
	CMError			err = unimpErr;
	OSErr			aOSerr = unimpErr;
#ifdef __MWERKS__
	unsigned char	theText[] = "\pDeviceLink profile  ";
	char			copyrightText[] = "\p©1996 by Heidelberger Druckmaschinen AG  U.J.K.";
#elif __IS_MSDOS
	char			theText[] = "\030DeviceLink profile     ";
	char			copyrightText[] = "\060©1996 by Heidelberger Druckmaschinen AG  U.J.K.";
#else
	char			theText[] = "\030DeviceLink profile     ";
	char			copyrightText[] = "\060©1996 by Heidelberger Druckmaschinen AG  U.J.K.";
#endif
	icProfile 		*aProf=0;
	long 			theTagTabSize;
	long 			theHeaderSize;
	long 			theDescSize;
	long 			theMediaSize;
	long 			theSequenceDescSize;
	long 			theA2B0Size;
	long 			theCopyRightSize;
	long 			currentSize=0;
	long 			theTotalSize=0;
	icTag			aTag;
	UINT32			sCS,dCS,clutSize;
	Ptr				aPtr=0;

	MyDoubleXYZ D50XYZ = { 0.9642, 1.0000, 0.8249 };
	MyXYZNumber D50 = { (unsigned long)(0.9642 * 65536), (unsigned long)(1.0000 * 65536), (unsigned long)(0.8249 * 65536)};
	theHeaderSize = sizeof(icHeader);
	theDescSize   = 									  sizeof(OSType) 			/* type descriptor */
														+ sizeof(unsigned long) 	/* reserved */
														+ sizeof(unsigned long) 	/* ASCII length */
														+ theText[0]				/* ASCII profile description */
														+ sizeof(unsigned long)		/* Unicode code */
														+ sizeof(unsigned long)		/* Unicode character count */
														+ sizeof(unsigned short)	/* Macintosh script code */
														+ sizeof(unsigned char)		/* Macintosh string length */
														+ 67						/* Macintosh string */
														;
	theCopyRightSize   = 								  sizeof(OSType) 			/* type descriptor */
														+ sizeof(unsigned long) 	/* reserved */
														+ copyrightText[0]			/* ASCII profile description */
														;
	theMediaSize = sizeof( icXYZType );;

	theTagTabSize = MyTagCountLink * sizeof( icTag ) + sizeof( unsigned long );
	
	*theProf = 0;
	
	if( cw->hasNamedColorProf != NoNamedColorProfile ){
		err = cmProfileError;
		goto CleanupAndExit;
	}
	err = MyGetColorSpaces( profileSet, &sCS, &dCS );
	if (err)
		goto CleanupAndExit;
		
	aProf = (icProfile *)SmartNewPtrClear( LINK_BUFFER_MAX, &aOSerr );
	err = aOSerr;
	if (err)
		goto CleanupAndExit;

	err = MyAdd_NL_Header(theTotalSize, (icHeader*)((Ptr)aProf+currentSize), aIntent, icSigLinkClass, sCS, dCS );
	if (err)
		goto CleanupAndExit;
		
	/*----------------------------------------------------------------------------------------- cmProfileDescriptionTag */
	currentSize = theHeaderSize + theTagTabSize;
	CMHelperICC2int32Const(&(aProf->tagList.count ), MyTagCountLink);

	CMHelperICC2int32Const(&(aTag.sig ), icSigProfileDescriptionTag);
	CMHelperICC2int32Const(&(aTag.offset ), currentSize);
	CMHelperICC2int32Const(&(aTag.size ), theDescSize);
	aProf->tagList.tags[0] = aTag;
	err =MyAdd_NL_DescriptionTag	( (LHTextDescriptionType *)((Ptr)aProf+currentSize), (unsigned char *)theText );
	if (err)
		goto CleanupAndExit;
	currentSize += theDescSize;
	currentSize = ( currentSize + 3 ) & ~ 3;

	CMHelperICC2int32Const(&(aTag.sig ), icSigMediaWhitePointTag);
	CMHelperICC2int32Const(&(aTag.offset ), currentSize );
	CMHelperICC2int32Const(&(aTag.size ), theMediaSize);
	aProf->tagList.tags[1] = aTag;
	MyAdd_NL_ColorantTag((icXYZType *)((Ptr)aProf+currentSize), &D50);
	currentSize += theMediaSize;
	currentSize = ( currentSize + 3 ) & ~ 3;

	CMHelperICC2int32Const(&(aTag.sig ), icSigCopyrightTag);
	CMHelperICC2int32Const(&(aTag.offset ), currentSize );
	CMHelperICC2int32Const(&(aTag.size ), theCopyRightSize);
	aProf->tagList.tags[2] = aTag;
	err = MyAdd_NL_CopyrightTag( (unsigned char *)copyrightText, (LHTextType *)((Ptr)aProf+currentSize));
	if (err)
		goto CleanupAndExit;
	currentSize += theCopyRightSize;
	currentSize = ( currentSize + 3 ) & ~ 3;
		
	err = MyAdd_NL_SequenceDescTag( profileSet, (icProfileSequenceDescType *)((Ptr)aProf+currentSize), &theSequenceDescSize );
	CMHelperICC2int32Const(&(aTag.sig ), icSigProfileSequenceDescTag);
	CMHelperICC2int32Const(&(aTag.offset ), currentSize );
	CMHelperICC2int32Const(&(aTag.size ), theSequenceDescSize);
	aProf->tagList.tags[3] = aTag;
	currentSize += theSequenceDescSize;
	currentSize = ( currentSize + 3 ) & ~ 3;

    theA2B0Size = GetSizes( (CMMModelPtr)cw, &clutSize );

	CMHelperICC2int32Const(&(aTag.sig ), icSigAToB0Tag);
	CMHelperICC2int32Const(&(aTag.offset ), currentSize );
	CMHelperICC2int32Const(&(aTag.size ), theA2B0Size);
	aProf->tagList.tags[4] = aTag;

#if __IS_MSDOS
	aPtr = GlobalAllocPtr( GHND, theA2B0Size+currentSize );
	if( aPtr == 0 ){
		err = GetLastError();
		goto CleanupAndExit;
	}
#else	
	aPtr =  SmartNewPtr( theA2B0Size+currentSize, &aOSerr );
	err = aOSerr;
	if( err ){
		goto CleanupAndExit;
	}
#endif


	if ( cw->lutParam.colorLutWordSize == 8 )
        err = MyAdd_NL_AToB0Tag_mft1( (CMMModelPtr)cw, (icLut8Type *)(aPtr+currentSize), clutSize );
    else
        err = MyAdd_NL_AToB0Tag_mft2( (CMMModelPtr)cw, (icLut16Type *)(aPtr+currentSize), clutSize );

	if (err)
		goto CleanupAndExit;

	BlockMove( (Ptr)aProf, aPtr, currentSize );
	aProf = (icProfile *)DisposeIfPtr( (Ptr)aProf );

	CMHelperICC2int32Const( aPtr, theA2B0Size+currentSize );

	*theProf = (icProfile *)aPtr;
#ifdef WRITE_PROFILE
	WriteProf( "DeviceLinkFill.pf", (icProfile *)aPtr, theA2B0Size+currentSize );
#endif
	return noErr;
	
CleanupAndExit:
	*theProf = (icProfile *)DisposeIfPtr( (Ptr)aProf );
#if __IS_MSDOS
	if( aPtr )GlobalFreePtr( aPtr );
#else
	aPtr = DisposeIfPtr( (Ptr)aPtr );
#endif
	return err;
}


