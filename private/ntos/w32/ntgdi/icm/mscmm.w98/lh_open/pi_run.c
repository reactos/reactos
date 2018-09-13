/*
	File:		PI_CWRuntime.c

	Contains:	
				
	Written by:	U. J. Krabbenhoeft

	Version:

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

*/

#ifndef LHGeneralIncs_h
#include "General.h"
#endif


#ifndef LHCMRuntime_h
#include "Runtime.h"
#endif
/* ______________________________________________________________________

CMError CWMatchBitmap   ( 
 							CMWorldRef          Storage,
							CMBitmap*           bitMap, 
							CMBitmapCallBackUPP progressProc, 
							void*               refCon, 
							CMBitmap*           matchedBitMap)
        Abstract:
                Match pixel data of bitMap according to the CMProfileRef parameters
                supplied to a previous call to CMMInitPrivate(É), CWNewColorWorld(...),
                CMMConcatInitPrivate(É) or CWConcatColorWorld(...).

        Params:
                storage                 (in)            Reference to ColorWorld storage.
                bitMap                  (in/out)        Describes source BitMap data.
                progressProc            (in)            Client function which is called once
                                                        per row of the BitMap. If the
                                                        function result is TRUE then the
                                                        operation is aborted. May be NULL.
                refCon                  (in)            Client data which is passed as a
                                                        parameter to calls to progressProc.
                matchedBitMap           (in/out)        Result matched BitMap. The caller
                                                        is responsible for allocating the
                                                        pixel buffer pointed to by baseAddr.
                                                        If NULL then the source BitMap is
                                                        matched in place.
        
        Return:
               noErr                                    successful
               System or result code if an error occurs.

   _____________________________________________________________________ */

CMError CWMatchBitmap   ( 
 							CMWorldRef          Storage,
							CMBitmap*           bitMap, 
							CMBitmapCallBackUPP progressProc, 
							void*               refCon, 
							CMBitmap*           matchedBitMap)

{
	CMError			err = noErr;
	CMMModelPtr aPtr;

	if( Storage == 0 )return cmparamErr;
	LOCK_DATA( Storage );
	aPtr = (CMMModelPtr)(DATA_2_PTR(	Storage ));
	err = LHMatchBitMapPrivate(	aPtr, 
								(const CMBitmap*)bitMap, 
								progressProc,
								refCon, 
								matchedBitMap );
	UNLOCK_DATA( Storage );
	return err;
}

/* ______________________________________________________________________

CMError CWCheckBitmap (
							CMWorldRef			Storage, 
							const CMBitmap		*bitMap,
							CMBitmapCallBackUPP	progressProc,
							void				*refCon,
							CMBitmap 			*resultBitMap )
						
	Abstract:
		Gamut test pixel data of bitMap  according to the CMProfileRef
		parameters supplied to a previous call to CMMInitPrivate(É), CWNewColorWorld(...),
		CMMConcatInitPrivate(É) or CWConcatColorWorld(...).

	Params:
		storage			(in)		Reference to ColorWorld storage.
		bitMap			(in/out)	Describes BitMap data.
		progressProc	(in)		Client function which is called
									once per row of the BitMap. If
									the function result is true then
									the operation is aborted.May be NULL.
		refCon			(in)		Client data which is passed as a
									parameter to calls to progressProc.
		resultBitMap	(in/out)	Result BitMap. Must be one bit depth
									and equal bounds of bitMap parameter.
									Pixels are set to 1 if corresponding
									pixel of bitMap is out of gamut.
		
	Return:
		noErr		successful
		System or result code if an error occurs.

   _____________________________________________________________________ */

CMError CWCheckBitmap (
							CMWorldRef			Storage, 
							const CMBitmap		*bitMap,
							CMBitmapCallBackUPP	progressProc,
							void				*refCon,
							CMBitmap 			*resultBitMap )
{
	CMError		err = noErr;
	CMMModelPtr aPtr;

	if( Storage == 0 )return cmparamErr;
	LOCK_DATA( Storage );
	aPtr = (CMMModelPtr)(DATA_2_PTR(	Storage ));
    err = LHCheckBitMapPrivate(  aPtr, 
                                  (const CMBitmap*)bitMap, 
                                  progressProc,
                                  refCon, 
                                  resultBitMap );
	UNLOCK_DATA( Storage );
	return err;
}
/* ______________________________________________________________________

CMError CWMatchBitmapPlane(	CMWorldRef          Storage,
                         	LH_CMBitmapPlane*   bitMap, 
                         	CMBitmapCallBackUPP progressProc, 
                         	void*               refCon, 
                         	LH_CMBitmapPlane*   matchedBitMap)
                        
        Abstract:
                Match pixel data of LH_CMBitmapPlane according to the CMProfileRef parameters
                supplied to a previous call to CMMInitPrivate(É), CWNewColorWorld(...),
                CMMConcatInitPrivate(É) or CWConcatColorWorld(...).

        Params:
                storage                 (in)            Reference to ColorWorld storage.
                bitMap                  (in/out)        Describes source BitMap data.
                progressProc            (in)            Client function which is called once
                                                        per row of the BitMap. If the
                                                        function result is TRUE then the
                                                        operation is aborted. May be NULL.
                refCon                  (in)            Client data which is passed as a
                                                        parameter to calls to progressProc.
                matchedBitMap           (in/out)        Result matched BitMap. The caller
                                                        is responsible for allocating the
                                                        pixel buffer pointed to by baseAddr.
                                                        If NULL then the source BitMap is
                                                        matched in place.
        
        Return:
               noErr                                    successful
               System or result code if an error occurs.

   _____________________________________________________________________ */

CMError CWMatchBitmapPlane(	CMWorldRef          Storage,
                         	LH_CMBitmapPlane*   bitMap, 
                         	CMBitmapCallBackUPP progressProc, 
                         	void*               refCon, 
                         	LH_CMBitmapPlane*   matchedBitMap)

{
	CMError			err = noErr;
	CMMModelPtr aPtr;

	if( Storage == 0 )return cmparamErr;
	LOCK_DATA( Storage );
	aPtr = (CMMModelPtr)(DATA_2_PTR(	Storage ));
	err = LHMatchBitMapPlanePrivate(	aPtr, 
                                  		(const LH_CMBitmapPlane*)bitMap, 
                                 	 	progressProc,
                                  		refCon, 
                                  		matchedBitMap );
	UNLOCK_DATA( Storage );
	return err;
}

/* ______________________________________________________________________

 CMError CWCheckBitmapPlane(	CMWorldRef          Storage,
								LH_CMBitmapPlane*   bitMap, 
								CMBitmapCallBackUPP progressProc, 
								void*               refCon, 
								LH_CMBitmapPlane*   CheckedBitMap)
                        
        Abstract:
                Check pixel data of LH_CMBitmapPlane according to the CMProfileRef parameters
                supplied to a previous call to CMMInitPrivate(É), CWNewColorWorld(...),
                CMMConcatInitPrivate(É) or CWConcatColorWorld(...).

        Params:
                storage                 (in)            Reference to ColorWorld storage.
                bitMap                  (in/out)        Describes source BitMap data.
                progressProc            (in)            Client function which is called once
                                                        per row of the BitMap. If the
                                                        function result is TRUE then the
                                                        operation is aborted. May be NULL.
                refCon                  (in)            Client data which is passed as a
                                                        parameter to calls to progressProc.
                CheckedBitMap           (in/out)        Result Checked BitMap. The caller
                                                        is responsible for allocating the
                                                        pixel buffer pointed to by baseAddr.
                                                        If NULL then the source BitMap is
                                                        Checked in place.
        
        Return:
               noErr                                    successful
               System or result code if an error occurs.

   _____________________________________________________________________ */

 CMError CWCheckBitmapPlane(	CMWorldRef          Storage,
								LH_CMBitmapPlane*   bitMap, 
								CMBitmapCallBackUPP progressProc, 
								void*               refCon, 
								LH_CMBitmapPlane*   CheckedBitMap)

{
	Storage=Storage;
	bitMap=bitMap;
	progressProc=progressProc;
	refCon=refCon;
	CheckedBitMap=CheckedBitMap;
	return cmparamErr;
	/*CMError			err = noErr;
	CMMModelPtr aPtr;

	if( Storage == 0 )return cmparamErr;
	LOCK_DATA( Storage );
	aPtr = (CMMModelPtr)(DATA_2_PTR(	Storage ));
	err = LHMatchBitMapPlanePrivate( aPtr, 
                                  	(const LH_CMBitmapPlane*)bitMap, 
                                 	progressProc,
                                  	refCon, 
                                  	CheckedBitMap );
	UNLOCK_DATA( Storage );
		return err;*/
}

/*---------------------------------------------------------------------------------
 CMError CWMatchColors(	CMWorldRef		Storage,
						CMColor			*myColors, 
						unsigned long	count )
						
	Abstract:
		Color-matching on a list of CMColor. The source and destination
		data types are specified by the CMProfileRef parameters to the
		previous call to the CMMInitPrivate(É), CWNewColorWorld(...),
		CMMConcatInitPrivate(É) or CWConcatColorWorld(...) function.

	Params:
		storage		(in)		Reference to ColorWorld storage.
		myColors	(in/out)	Array of CMColor.
		count		(in)		One-based count of elements in array.
		
	Return:
		noErr		successful
		System or result code if an error occurs.

   _____________________________________________________________________ */
 CMError CWMatchColors(	CMWorldRef		Storage,
						CMColor			*myColors, 
						unsigned long	count )

{
	CMError			err = noErr;
	CMMModelPtr aPtr;

	if( Storage == 0 )return cmparamErr;
	LOCK_DATA( Storage );
	aPtr = (CMMModelPtr)(DATA_2_PTR(	Storage ));
	err = LHMatchColorsPrivate( aPtr, myColors, count );
	UNLOCK_DATA( Storage );
	return err;
}

/* ______________________________________________________________________

CMError CWCheckColorsMS ( 
						CMWorldRef 		Storage, 
						CMColor 		*myColors, 
						unsigned long	count, 
						long 			*result )
						
	Abstract:
		Gamut test a list of CMColor. The source and destination are
		specified by the CMProfileRef parameters to the previous call to
		the CMMInitPrivate(É), CWNewColorWorld(...),
		CMMConcatInitPrivate(É) or CWConcatColorWorld(...) function.

	Params:
		storage		(in)		Reference to ColorWorld storage.
		myColors	(in)		Array of CMColor.
		count		(in)		One-based count of elements in array.
		result		(in/out)	Bits in array are set to 1 if the corresponding color is out of gamut.
		
	Return:
		noErr		successful
		System or result code if an error occurs.

   _____________________________________________________________________ */


CMError CWCheckColorsMS ( 
						CMWorldRef 		Storage, 
						CMColor 		*myColors, 
						unsigned long	count, 
						UINT8 			*result )
{
	CMError			err = noErr;
	CMMModelPtr aPtr;

	if( Storage == 0 )return cmparamErr;
	LOCK_DATA( Storage );
	aPtr = (CMMModelPtr)(DATA_2_PTR(	Storage ));
	err = LHCheckColorsPrivateMS( aPtr, myColors, count, result );
	UNLOCK_DATA( Storage );
	return err;
}
/* ______________________________________________________________________

CMError CWCheckColors ( 
						CMWorldRef 		Storage, 
						CMColor 		*myColors, 
						unsigned long	count, 
						unsigned char	*result )
						
	Abstract:
		Gamut test a list of CMColor. The source and destination are
		specified by the CMProfileRef parameters to the previous call to
		the CMMInitPrivate(É), CWNewColorWorld(...),
		CMMConcatInitPrivate(É) or CWConcatColorWorld(...) function.

	Params:
		storage		(in)		Reference to ColorWorld storage.
		myColors	(in)		Array of CMColor.
		count		(in)		One-based count of elements in array.
		result		(in/out)	Bits in array are set to 1 if the corresponding color is out of gamut.
		
	Return:
		noErr		successful
		System or result code if an error occurs.

   _____________________________________________________________________ */


CMError CWCheckColors ( 
						CMWorldRef 		Storage, 
						CMColor 		*myColors, 
						unsigned long	count, 
						unsigned char	*result )
{
	CMError			err = noErr;
	CMMModelPtr aPtr;

	if( Storage == 0 )return cmparamErr;
	LOCK_DATA( Storage );
	aPtr = (CMMModelPtr)(DATA_2_PTR(	Storage ));
	err = LHCheckColorsPrivate( aPtr, myColors, count, result );
	UNLOCK_DATA( Storage );
	return err;
}
/* ______________________________________________________________________

 CMError CWGetColorSpaces(	CMWorldRef cw, 
							CMBitmapColorSpace *In, 
							CMBitmapColorSpace *Out );
                        
        Abstract:
                Get input and output colorspace out of CMWorldRef.

        Params:
                cw       (in)            Reference to ColorWorld storage.
                In       (Out)           Reference to input CMBitmapColorSpace.
                In       (Out)           Reference to output CMBitmapColorSpace.
        
        Return:
               noErr                                    successful
               System or result code if an error occurs.

   _____________________________________________________________________ */

CMError CWGetColorSpaces(	CMWorldRef cw, 
							CMBitmapColorSpace *In, 
							CMBitmapColorSpace *Out )

{
	CMError			err = noErr;
	CMMModelPtr aPtr;

	if( cw == 0 )return cmparamErr;
	LOCK_DATA( cw );
	aPtr = (CMMModelPtr)(DATA_2_PTR( cw ));
	*In = aPtr->firstColorSpace;
	*Out = aPtr->lastColorSpace;
	UNLOCK_DATA( cw );
	return err;
}

