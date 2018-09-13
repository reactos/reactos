/*
	File:		PI_ColorWorld.c

	Contains:	
				
	Written by:	U. J. Krabbenhoeft

	Version:

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

*/
#ifndef LHGeneralIncs_h
#include "General.h"
#endif

#ifndef PI_ColorWorld_h
#include "PI_Color.h"
#endif

#ifndef realThing
#ifdef DEBUG_OUTPUT
#define kThisFile kLHCMDo3DID
#define __TYPES__
/*#include "DebugSpecial.h"*/
/*#include "LH_Util.h"*/
#endif
#endif

#ifndef PI_CMMInitialization_h
#include "PI_CMM.h"
#endif

#ifndef MemLink_h
#include "MemLink.h"
#endif

/* ______________________________________________________________________

         CMError
               LHColorWorldOpen (Handle* storage)

        Abstract:
                opens ColorWorld and allocates up any necessary memory

        Params:
                storage         (in/out)   pointer to
                                           handle to memory to be used by CMM

        Return:
                noErr                      successful

   _____________________________________________________________________ */

 CMError LHColorWorldOpen (Ptr* storage)
{ 
    Ptr 			    myStorage;
    OSErr               err;
    
    myStorage = (Ptr)SmartNewPtrClear( sizeof( CMMModelData ) , &err);
    if (err) 
        goto CleanupAndExit;
    else
    {
        *storage = myStorage;
    }

CleanupAndExit:
    return err;
}

/* ______________________________________________________________________

          CMError
        LHColorWorldClose(Handle storage);

        Abstract:
                closes ColorWorld and cleans up any remaining memory allocations

        Params:
                storage (in/out)        handle to memory to be used by CMM

        Return:
                noErr           successful

   _____________________________________________________________________ */

 CMError LHColorWorldClose( Ptr storage )
{ 
    CMMModelPtr  modelData = (CMMModelPtr)storage;

    if (storage)
    {
		DISPOSE_IF_DATA((modelData)->lutParam.inputLut);		
		DISPOSE_IF_DATA((modelData)->lutParam.outputLut);		
		DISPOSE_IF_DATA((modelData)->lutParam.colorLut);

		DISPOSE_IF_DATA((modelData)->gamutLutParam.inputLut);		
		DISPOSE_IF_DATA((modelData)->gamutLutParam.outputLut);		
		DISPOSE_IF_DATA((modelData)->gamutLutParam.colorLut);

		DISPOSE_IF_DATA((modelData)->theNamedColorTagData);
#if	__IS_MAC
		DisposeIfHandle((modelData)->Monet);
#endif

		DisposeIfPtr(storage);
    }
    return( noErr );
}
/* ______________________________________________________________________

         CMError
               CWNewColorWorld (CMWorldRef*    storage,
                                 CMProfileRef   srcProfile, 
                                 CMProfileRef   dstProfile)

        Abstract:
                opens ColorWorld and allocates up any necessary memory,
                fill buffer for color transformation


        Params:
                storage         (in/out)   Ptr to memory to be used by CMM
                srcProfile      (in)       pointer to source profile
                                           description
                dstProfile      (in)       pointer to destination profile
                                           description

                storage:
                typedef struct CMPrivateColorWorldRecord *CMWorldRef;

        Return:
                noErr                      successful
		System or result code if an error occurs.

   _____________________________________________________________________ */


 CMError 
       CWNewColorWorld (	CMWorldRef*    storage,
							CMProfileRef   srcProfile,
							CMProfileRef   dstProfile)
{ 
    Ptr 		myStorage;
    CMError    	err;

#ifdef DEBUG_OUTPUT
    /*printf("vor LHColorWorldOpen\n");*/
#endif
    err = LHColorWorldOpen(&myStorage);
#ifdef DEBUG_OUTPUT
    /*printf("nach LHColorWorldOpen: err = %d\n", err);*/
#endif
    if ( !err )
    {
        ((CMMModelPtr)myStorage)->aIntentArr	= 0;
		((CMMModelPtr)myStorage)->dwFlags		= 0xffffffff;
        err =  CMMInitPrivate((CMMModelPtr)myStorage,
                               srcProfile, 
                               dstProfile);
#ifdef DEBUG_OUTPUT
    /*printf("nach NCMMInitPrivate: err = %d\n", err);*/
#endif
    }


    if ( !err )
    {
		*storage = (CMWorldRef)myStorage;
    }
	else{
		*storage = (CMWorldRef)0;
		LHColorWorldClose( myStorage );
	}

    return err;
}

/* ______________________________________________________________________

         CMError
               CWConcatColorWorld (	CMWorldRef*    storage,
									CMConcatProfileSet* profileSet )

        Abstract:
                opens ColorWorld and allocates up any necessary memory,
                fill buffer for color transformation


        Params:
                storage         (in/out)   Ptr to memory to be used by CMM
				profileSet		(in)		CMConcatProfileSet contains an array of
											Profiles which describe the processing
											to be carried out.The profileSet array
											is in processing orderÑ Source through
											Destination. A minimum of one CMProfileRef
											must be specified.

                storage:
                typedef struct CMPrivateColorWorldRecord *CMWorldRef;

        Return:
                noErr                      successful
		System or result code if an error occurs.

   _____________________________________________________________________ */
 CMError 
       CWConcatColorWorld  (	CMWorldRef*         storage,
								CMConcatProfileSet* profileSet)
{ 
    Ptr 		myStorage;
    CMError     err;

    err = LHColorWorldOpen(&myStorage);

    if ( !err )
    {
        ((CMMModelPtr)myStorage)->aIntentArr	= 0;
		((CMMModelPtr)myStorage)->dwFlags		= 0xffffffff;
        err = CMMConcatInitPrivate  ((CMMModelPtr)myStorage, profileSet );
    }


    if ( !err )
    {
		*storage = (CMWorldRef)myStorage;
    }
	else{
		*storage = (CMWorldRef)0;
		LHColorWorldClose( myStorage );
	}

    return err;
}

#if WRITE_PROFILE
 void WriteProf( Ptr name, icProfile *theProf, long currentSize );
#endif
/* ______________________________________________________________________

         CMError
           CWConcatColorWorld4MS (	CMWorldRef			*storage,
									CMConcatProfileSet	*profileSet,
									UINT32				*aIntentArr,
									UINT32				nIntents,
									UINT32				dwFlags
								)

        Abstract:
                opens ColorWorld and allocates up any necessary memory,
                fill buffer for color transformation


        Params:
                storage         (in/out)   Ptr to memory to be used by CMM
				profileSet		(in)		CMConcatProfileSet contains an array of
											Profiles which describe the processing
											to be carried out.The profileSet array
											is in processing orderÑ Source through
											Destination. A minimum of one CMProfileRef
											must be specified.

				padwIntents		(in)		Points to an array of intent structures.
											0 = default behavior ( intents out of profiles )

				nIntents		(in)		Specifies the number of intents in the intent array. 
											Can be 1, or the same value as nProfiles.

				dwFlags			(in)		Specifies flags to control creation of the transform.
											These flags are intended only as hints, and it is up to the CMM
											to determine how best to use these flags. 
											Set the high-order word to ENABLE_GAMUT_CHECKING if the transform will be used 
											for gamut checking. 
											The low-order WORD can have one of the following constant values:
											PROOF_MODE, NORMAL_MODE, BEST_MODE. Moving from PROOF_MODE to BEST_MODE, 
											output quality generally improves and transform speed declines.
        Return:
                noErr                      successful
		System or result code if an error occurs.

   _____________________________________________________________________ */
 CMError	CWConcatColorWorld4MS  (	CMWorldRef			*storage,
										CMConcatProfileSet	*profileSet,
										UINT32				*aIntentArr,
										UINT32				nIntents,
										UINT32				dwFlags
								  )
{ 
    Ptr 		myStorage;
    CMError     err;
#if WRITE_PROFILE
	icProfile *theLinkProfile;
	long l;
#endif

    err = LHColorWorldOpen(&myStorage);

    if ( !err )
    {
		((CMMModelPtr)myStorage)->aIntentArr	= aIntentArr;
        ((CMMModelPtr)myStorage)->nIntents		= nIntents;
        ((CMMModelPtr)myStorage)->dwFlags		= dwFlags;
		err = CMMConcatInitPrivate  ((CMMModelPtr)myStorage, profileSet );
    }


    if ( !err )
    {
		*storage = (CMWorldRef)myStorage;
    }
	else{
		*storage = (CMWorldRef)0;
		LHColorWorldClose( myStorage );
	}

#if WRITE_PROFILE
	err = DeviceLinkFill( (CMMModelPtr)myStorage, profileSet, &theLinkProfile, 0 );
	if( !err ){
		l = *(unsigned long *)theLinkProfile;
		SwapLong(&l);
		WriteProf( "test", theLinkProfile, l );
	}
#endif
    return err;
}

 CMError 
       CWLinkColorWorld (	CMWorldRef*         storage,
                            CMConcatProfileSet* profileSet)
{ 
    Ptr 		myStorage;
    CMError     err;

    err = LHColorWorldOpen(&myStorage);

	if ( !err )
    {
		((CMMModelPtr)myStorage)->currentCall	= kCMMNewLinkProfile;
		((CMMModelPtr)myStorage)->aIntentArr	= 0;
		((CMMModelPtr)myStorage)->dwFlags		= 0xffffffff;
		err = CMMConcatInitPrivate  ((CMMModelPtr)myStorage, profileSet );
    }


    if ( !err )
    {
		*storage = (CMWorldRef)myStorage;
    }
	else{
		*storage = (CMWorldRef)0;
		LHColorWorldClose( myStorage );
	}

    return err;
}

/* ______________________________________________________________________

         CMError
               CWDisposeColorWorld (CMWorldRef storage)

        Abstract:
                closes ColorWorld and cleans up any remaining memory allocations

        Params:
                storage         (in/out)    handle to memory to be used by CMM
        
        Return:
                -
   _____________________________________________________________________ */


 void 
       CWDisposeColorWorld ( CMWorldRef storage )
{ 
    LHColorWorldClose ( (Ptr)storage ); 
}

void CMSetLookupOnlyMode( CMWorldRef	Storage,
						  PI_Boolean		Mode )
{
	CMMModelPtr		aPtr;

	if( Storage == 0 )return;
	LOCK_DATA( Storage );
	aPtr = (CMMModelPtr)(DATA_2_PTR(	Storage ));
	aPtr->lookup = Mode;
	UNLOCK_DATA( Storage );
	return;
}

void CMFullColorRemains( CMWorldRef	Storage,
						 long		ColorMask )
{
	CMMModelPtr		aPtr;
	CMLutParamPtr	lutParamPtr;
	long Address,Size,i,j;

	if( Storage == 0 )return;
	LOCK_DATA( Storage );
	aPtr = (CMMModelPtr)(DATA_2_PTR(	Storage ));
	lutParamPtr = &aPtr->lutParam; 

	Size = lutParamPtr->colorLutWordSize / 8;
	if( !(lutParamPtr->colorLutInDim == 4 && lutParamPtr->colorLutOutDim == 4 ))return;

	for( i=0; i<4; i++ ){
		if( (ColorMask & (1<<i)) == 0 ) continue;
		Address = lutParamPtr->colorLutGridPoints - 1;
		for( j=3-i+1; j<4; j++ )Address *= lutParamPtr->colorLutGridPoints;  
		Address	= Address * lutParamPtr->colorLutOutDim;
		for( j=0; j<4; j++){
			if( i == j ){
				if( Size == 1 ){
					*( ((unsigned char*)lutParamPtr->colorLut)+Address+3-j ) = (unsigned char)255;
				}
				else{
					*( ((unsigned short*)lutParamPtr->colorLut)+Address+3-j ) = (unsigned short)65535;
				}
			}
			else{
				if( Size == 1 ){
					*( ((unsigned char*)lutParamPtr->colorLut)+Address+3-j ) = (unsigned char)0;
				}
				else{
					*( ((unsigned short*)lutParamPtr->colorLut)+Address+3-j ) = (unsigned short)0;
				}
			}
		}
	}
	UNLOCK_DATA( Storage );
	return;
}

CMError	CWCreateLink4MS (				CMWorldRef			storage,
										CMConcatProfileSet	*profileSet,
										UINT32				aIntent,
										icProfile			**theLinkProfile )
{ 
    CMError     err;

	*theLinkProfile = 0;
	
	err = DeviceLinkFill( (CMMModelPtr)storage, profileSet, theLinkProfile, aIntent );

	return err;
}
