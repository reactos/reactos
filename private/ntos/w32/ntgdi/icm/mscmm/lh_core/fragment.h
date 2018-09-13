/*
	File:		LHFragment.h

	Contains:	prototypes for special lut extraction

	Written by:	H.Siegeritz

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.


*/

#ifndef LHFragment_h
#define	LHFragment_h

#ifndef LHDefines_h
#include "Defines.h"
#endif

#ifndef LHGenLuts_h
#include "GenLuts.h"
#endif


icCurveType	*	InvertLut1d	( icCurveType *LookUpTable,
							  UINT8 AdressBits);
							  
CMError			CombiMatrix	( icXYZType srcColorantData[3],
							  icXYZType destColorantData[3], 
							  double resMatrix[3][3] );
							  
Boolean		doubMatrixInvert( double MatHin[3][3], 
							  double MatRueck[3][3] );

CMError Fill_ushort_ELUT_identical(	UINT16 *usELUT, 
									char addrBits, 
									char usedBits, 
									long gridPoints);

CMError Fill_ushort_ELUT_from_CurveTag( icCurveType *pCurveTag,
										UINT16		*usELUT, 
										char		addrBits, 
										char		usedBits, 
										long		gridPoints);

CMError Fill_inverse_byte_ALUT_from_CurveTag( icCurveType	*pCurveTag,
											  UINT8			*ucALUT, 
											  char			addrBits);

CMError Fill_inverse_ushort_ALUT_from_CurveTag(	icCurveType		*pCurveTag,
												unsigned short	*usALUT,
												char			addrBits );


CMError	Fill_ushort_ELUTs_from_lut8Tag ( CMLutParamPtr	theLutData,
										 Ptr			profileELuts,
										 char			addrBits,
										 char			usedBits, 
										 long			gridPoints );

CMError Fill_byte_ALUTs_from_lut8Tag( CMLutParamPtr	theLutData,
									  Ptr			profileALuts, 
									  char			addrBits );

CMError Fill_ushort_ALUTs_from_lut8Tag(	CMLutParamPtr	theLutData,
							  			Ptr				profileALuts, 
							  			char			addrBits );


CMError	Fill_ushort_ELUTs_from_lut16Tag ( CMLutParamPtr	theLutData,
										  Ptr			profileELuts,
										  char			addrBits,
										  char			usedBits, 
										  long			gridPoints,
										  long			inputTableEntries );

CMError Fill_byte_ALUTs_from_lut16Tag( CMLutParamPtr	theLutData,
									   Ptr				profileALuts, 
									   char				addrBits,
									   long				outputTableEntries );

CMError Fill_ushort_ALUTs_from_lut16Tag(CMLutParamPtr	theLutData,
										Ptr				profileALuts,
										char			addrBits,
							    		long			outputTableEntries );


CMError  MakeGamut16or32ForMonitor(	icXYZType		*pRedXYZ,
									icXYZType		*pGreenXYZ,
									icXYZType		*pBlueXYZ,
									CMLutParamPtr	theLutData,
									Boolean			cube32Flag );

CMError	DoAbsoluteShiftForPCS_Cube16(	unsigned short	*theCube,
										long			count,
										CMProfileRef	theProfile,
										Boolean			pcsIsXYZ,
										Boolean			afterInput );
#endif 

