/*
	File:		LHGenLuts.h

	Contains:	

	Written by:	

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

*/
#ifndef LHGenLuts_h
#define LHGenLuts_h

#define kAbsoluteCalcNothing	0
#define kAbsoluteCalcBefore		1
#define kAbsoluteCalcAfter		2

#define kAbsShiftBeforeDoNDim	FALSE
#define kAbsShiftAfterDoNDim	TRUE


CMError 
FillLuts  ( CMMModelHandle	CMSession,
			CMProfileRef 		srcProfile,
			CMProfileRef			dstProfile );

void
CreateLinearElut16 (	Ptr		theElut,
				   		long	theSize,
				   		long	gridPoints,
				   		long	gridPointsCube);
void
CreateLinearElut ( Ptr		theElut,
				   long		theSize,
				   long		gridPoints,
				   long		gridPointsCube);
void
CreateLinearAlut ( UINT8*	theAlut,
				   long		count );
void
CreateLinearAlut16 ( 	UINT16*	theAlut,
				    	long 	aCount );
CMError
DoMakeGamutForMonitor	( CMLutParamPtr		theLutData,
						  LHCombiDataPtr	theCombiData );
CMError
Extract_MFT_Xlut	  ( CMLutParamPtr	theLutData,  
						LHCombiDataPtr	theCombiData,
						Ptr				profileLutPtr,
						OSType			theTag );
CMError
Extract_MFT_Elut	  ( CMLutParamPtr	theLutData,  
						LHCombiDataPtr	theCombiData,
						Ptr				profileLutPtr,
						OSType			theTag );
CMError
Extract_MFT_Alut	  ( CMLutParamPtr	theLutData,  
						LHCombiDataPtr	theCombiData,
						Ptr				profileLutPtr,
						OSType			theTag  );
						
CMError
ExtractAll_MFT_Luts  (  CMLutParamPtr	theLutData,
						LHCombiDataPtr	theCombiData,
						OSType			theTag );
CMError
Extract_TRC_Alut	  ( CMLutParamPtr	theLutData,
						LHCombiDataPtr	theCombiData );
CMError
Extract_TRC_Elut	  ( CMLutParamPtr	theLutData,
						LHCombiDataPtr	theCombiData );
CMError
Extract_TRC_Matrix	  ( CMLutParamPtr	theLutData,
						LHCombiDataPtr	theCombiData );
CMError
ExtractAll_TRC_Luts  (  CMLutParamPtr	theLutData,
						LHCombiDataPtr	theCombiData );
CMError
Extract_Gray_Luts	 (  CMLutParamPtr	theLutData,
						LHCombiDataPtr	theCombiData );
CMError
GetMatrixFromProfile	( CMLutParamPtr		theLutData,
						  LHCombiDataPtr	theCombiData,
						  OSType			theTag,
						  double			factor );
CMError
ExtractAllLuts    ( CMLutParamPtr	theLutData,
					LHCombiDataPtr	theCombiData );
					
CMError
Create_LH_ProfileSet	( CMMModelPtr    		CMSession,
						  CMConcatProfileSet* 	profileSet,
						  LHConcatProfileSet**	newProfileSet );
CMError
CreateCombi	( CMMModelPtr	    	modelingData,
			  CMConcatProfileSet* 	profileSet,
			  LHConcatProfileSet*	newProfileSet,
			  CMLutParamPtr			finalLutData,	
			  Boolean				createGamutLut );
CMError 
PrepareCombiLUTs	( CMMModelPtr    CMSession,
					  CMConcatProfileSet* profileSet );

#endif
