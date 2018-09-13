/*
	File:		LHCalcNDim.h

	Contains:	

	Written by:	U. J. Krabbenhoeft

	Version:

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

*/

#ifndef LHCalcNDim_h
#define LHCalcNDim_h
/*--------------------------------------------------------------------------------------------------------------
	DoNDim prototypes
  --------------------------------------------------------------------------------------------------------------*/
CMError	CalcNDim_Data8To8_Lut8		( CMCalcParamPtr calcParam,
									  CMLutParamPtr  lutParam );

CMError	CalcNDim_Data8To16_Lut8		( CMCalcParamPtr calcParam,
									  CMLutParamPtr  lutParam );

CMError	CalcNDim_Data16To8_Lut8		( CMCalcParamPtr calcParam,
									  CMLutParamPtr  lutParam );
									  
CMError	CalcNDim_Data16To16_Lut8	( CMCalcParamPtr calcParam,
									  CMLutParamPtr  lutParam );
									  
CMError	CalcNDim_Data8To8_Lut16		( CMCalcParamPtr calcParam,
									  CMLutParamPtr  lutParam );

CMError	CalcNDim_Data8To16_Lut16	( CMCalcParamPtr calcParam,
									  CMLutParamPtr  lutParam );

CMError	CalcNDim_Data16To8_Lut16	( CMCalcParamPtr calcParam,
									  CMLutParamPtr  lutParam );

CMError	CalcNDim_Data16To16_Lut16	( CMCalcParamPtr calcParam,
									  CMLutParamPtr  lutParam );

#endif
