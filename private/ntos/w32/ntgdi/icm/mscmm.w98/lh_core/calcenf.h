/*
	File:		LHCalcEngine.h

	Contains:	

	Written by:	U. J. Krabbenhoeft

	Version:

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

*/

#ifndef LHCalcEngineF_h
#define LHCalcEngineF_h

#ifndef LHTypeDefs_h
#include "TypeDefs.h"
#endif

typedef unsigned char  LH_UINT8;
typedef unsigned short LH_UINT16;
typedef unsigned long  LH_UINT32;
#undef  LH_CALC_ENGINE_UNDEF_MODE
#undef  LH_CALC_ENGINE_P_TO_P
#undef  LH_CALC_ENGINE_P_TO_U			
#undef  LH_CALC_ENGINE_U_TO_P
#undef  LH_CALC_ENGINE_U_TO_U		
#define LH_CALC_ENGINE_UNDEF_MODE           0
#define LH_CALC_ENGINE_P_TO_P               1
#define LH_CALC_ENGINE_P_TO_U		        2				
#define LH_CALC_ENGINE_U_TO_P			    3
#define LH_CALC_ENGINE_U_TO_U			    4				

/*--------------------------------------------------------------------------------------------------------------
	calc 3 to 3
  --------------------------------------------------------------------------------------------------------------*/
CMError	LHCalc3to3_Di8_Do8_Lut8_G32_F 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to3_Di8_Do8_Lut8_G16_F 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
								

/*--------------------------------------------------------------------------------------------------------------
	calc 3 to 4
  --------------------------------------------------------------------------------------------------------------*/
CMError	LHCalc3to4_Di8_Do8_Lut8_G32_F 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to4_Di8_Do8_Lut8_G16_F 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );

/*--------------------------------------------------------------------------------------------------------------
	calc 4 to 3
  --------------------------------------------------------------------------------------------------------------*/
CMError	LHCalc4to3_Di8_Do8_Lut8_G8_F 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc4to3_Di8_Do8_Lut8_G16_F 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
/*--------------------------------------------------------------------------------------------------------------
	calc 4 to 4
  --------------------------------------------------------------------------------------------------------------*/
CMError	LHCalc4to4_Di8_Do8_Lut8_G8_F 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc4to4_Di8_Do8_Lut8_G16_F 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );

#endif							 
