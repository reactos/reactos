/*
	File:		LHCalcEngine.h

	Contains:	

	Written by:	U. J. Krabbenhoeft

	Version:

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

*/

#ifndef LHCalcEngine_h
#define LHCalcEngine_h

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
CMError	LHCalc3to3_Di8_Do8_Lut8_G32 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to3_Di16_Do8_Lut8_G32		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to3_Di8_Do16_Lut8_G32		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to3_Di16_Do16_Lut8_G32		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
								 
CMError	LHCalc3to3_Di8_Do8_Lut8_G16 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to3_Di16_Do8_Lut8_G16 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc3to3_Di8_Do16_Lut8_G16 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc3to3_Di16_Do16_Lut8_G16		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 


CMError	LHCalc3to3_Di8_Do8_Lut16_G32 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to3_Di16_Do8_Lut16_G32		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to3_Di8_Do16_Lut16_G32		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to3_Di16_Do16_Lut16_G32		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
								 
CMError	LHCalc3to3_Di8_Do8_Lut16_G16 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to3_Di16_Do8_Lut16_G16 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc3to3_Di8_Do16_Lut16_G16 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc3to3_Di16_Do16_Lut16_G16		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 


/*--------------------------------------------------------------------------------------------------------------
	calc 3 to 4
  --------------------------------------------------------------------------------------------------------------*/
CMError	LHCalc3to4_Di8_Do8_Lut8_G32 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to4_Di16_Do8_Lut8_G32		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to4_Di8_Do16_Lut8_G32		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to4_Di16_Do16_Lut8_G32		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
								 
CMError	LHCalc3to4_Di8_Do8_Lut8_G16 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to4_Di16_Do8_Lut8_G16 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc3to4_Di8_Do16_Lut8_G16 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc3to4_Di16_Do16_Lut8_G16 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 


CMError	LHCalc3to4_Di8_Do8_Lut16_G32 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to4_Di16_Do8_Lut16_G32		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to4_Di8_Do16_Lut16_G32		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to4_Di16_Do16_Lut16_G32		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
								 
CMError	LHCalc3to4_Di8_Do8_Lut16_G16 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to4_Di16_Do8_Lut16_G16 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc3to4_Di8_Do16_Lut16_G16 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc3to4_Di16_Do16_Lut16_G16 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 

/*--------------------------------------------------------------------------------------------------------------
	calc 4 to 3
  --------------------------------------------------------------------------------------------------------------*/
CMError	LHCalc4to3_Di8_Do8_Lut8_G8 			( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc4to3_Di16_Do8_Lut8_G8			( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc4to3_Di8_Do16_Lut8_G8			( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc4to3_Di16_Do16_Lut8_G8		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
								 
CMError	LHCalc4to3_Di8_Do8_Lut8_G16 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc4to3_Di16_Do8_Lut8_G16 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc4to3_Di8_Do16_Lut8_G16 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc4to3_Di16_Do16_Lut8_G16 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 


CMError	LHCalc4to3_Di8_Do8_Lut16_G8 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc4to3_Di16_Do8_Lut16_G8		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc4to3_Di8_Do16_Lut16_G8		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc4to3_Di16_Do16_Lut16_G8		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
								 
CMError	LHCalc4to3_Di8_Do8_Lut16_G16 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc4to3_Di16_Do8_Lut16_G16 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc4to3_Di8_Do16_Lut16_G16 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc4to3_Di16_Do16_Lut16_G16 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 


/*--------------------------------------------------------------------------------------------------------------
	calc 4 to 4
  --------------------------------------------------------------------------------------------------------------*/
CMError	LHCalc4to4_Di8_Do8_Lut8_G8 			( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc4to4_Di16_Do8_Lut8_G8 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc4to4_Di8_Do16_Lut8_G8 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc4to4_Di16_Do16_Lut8_G8 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 

CMError	LHCalc4to4_Di8_Do8_Lut8_G16 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc4to4_Di16_Do8_Lut8_G16 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc4to4_Di8_Do16_Lut8_G16 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc4to4_Di16_Do16_Lut8_G16 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
								 
								 
CMError	LHCalc4to4_Di8_Do8_Lut16_G8 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc4to4_Di16_Do8_Lut16_G8 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc4to4_Di8_Do16_Lut16_G8 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc4to4_Di16_Do16_Lut16_G8 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 

CMError	LHCalc4to4_Di8_Do8_Lut16_G16 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc4to4_Di16_Do8_Lut16_G16 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc4to4_Di8_Do16_Lut16_G16 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc4to4_Di16_Do16_Lut16_G16 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 

/*--------------------------------------------------------------------------------------------------------------
	calc 3 to 3
  --------------------------------------------------------------------------------------------------------------*/
CMError	LHCalc3to3_Di8_Do8_Lut8_G32_LO 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to3_Di16_Do8_Lut8_G32_LO		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to3_Di8_Do16_Lut8_G32_LO		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to3_Di16_Do16_Lut8_G32_LO	( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
								 
CMError	LHCalc3to3_Di8_Do8_Lut8_G16_LO 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to3_Di16_Do8_Lut8_G16_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc3to3_Di8_Do16_Lut8_G16_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc3to3_Di16_Do16_Lut8_G16_LO	( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 


CMError	LHCalc3to3_Di8_Do8_Lut16_G32_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to3_Di16_Do8_Lut16_G32_LO	( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to3_Di8_Do16_Lut16_G32_LO	( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to3_Di16_Do16_Lut16_G32_LO	( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
								 
CMError	LHCalc3to3_Di8_Do8_Lut16_G16_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to3_Di16_Do8_Lut16_G16_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc3to3_Di8_Do16_Lut16_G16_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc3to3_Di16_Do16_Lut16_G16_LO	( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 


/*--------------------------------------------------------------------------------------------------------------
	calc 3 to 4
  --------------------------------------------------------------------------------------------------------------*/
CMError	LHCalc3to4_Di8_Do8_Lut8_G32_LO 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to4_Di16_Do8_Lut8_G32_LO		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to4_Di8_Do16_Lut8_G32_LO		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to4_Di16_Do16_Lut8_G32_LO	( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
								 
CMError	LHCalc3to4_Di8_Do8_Lut8_G16_LO 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to4_Di16_Do8_Lut8_G16_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc3to4_Di8_Do16_Lut8_G16_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc3to4_Di16_Do16_Lut8_G16_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 


CMError	LHCalc3to4_Di8_Do8_Lut16_G32_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to4_Di16_Do8_Lut16_G32_LO	( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to4_Di8_Do16_Lut16_G32_LO	( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to4_Di16_Do16_Lut16_G32_LO	( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
								 
CMError	LHCalc3to4_Di8_Do8_Lut16_G16_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to4_Di16_Do8_Lut16_G16_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc3to4_Di8_Do16_Lut16_G16_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc3to4_Di16_Do16_Lut16_G16_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 

/*--------------------------------------------------------------------------------------------------------------
	calc 4 to 3
  --------------------------------------------------------------------------------------------------------------*/
CMError	LHCalc4to3_Di8_Do8_Lut8_G8_LO 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc4to3_Di16_Do8_Lut8_G8_LO		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc4to3_Di8_Do16_Lut8_G8_LO		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc4to3_Di16_Do16_Lut8_G8_LO		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
								 
CMError	LHCalc4to3_Di8_Do8_Lut8_G16_LO 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc4to3_Di16_Do8_Lut8_G16_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc4to3_Di8_Do16_Lut8_G16_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc4to3_Di16_Do16_Lut8_G16_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 


CMError	LHCalc4to3_Di8_Do8_Lut16_G8_LO 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc4to3_Di16_Do8_Lut16_G8_LO		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc4to3_Di8_Do16_Lut16_G8_LO		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc4to3_Di16_Do16_Lut16_G8_LO	( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
								 
CMError	LHCalc4to3_Di8_Do8_Lut16_G16_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc4to3_Di16_Do8_Lut16_G16_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc4to3_Di8_Do16_Lut16_G16_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc4to3_Di16_Do16_Lut16_G16_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 


/*--------------------------------------------------------------------------------------------------------------
	calc 4 to 4
  --------------------------------------------------------------------------------------------------------------*/
CMError	LHCalc4to4_Di8_Do8_Lut8_G8_LO 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc4to4_Di16_Do8_Lut8_G8_LO 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc4to4_Di8_Do16_Lut8_G8_LO 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc4to4_Di16_Do16_Lut8_G8_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 

CMError	LHCalc4to4_Di8_Do8_Lut8_G16_LO 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc4to4_Di16_Do8_Lut8_G16_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc4to4_Di8_Do16_Lut8_G16_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc4to4_Di16_Do16_Lut8_G16_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
								 
								 
CMError	LHCalc4to4_Di8_Do8_Lut16_G8_LO 		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc4to4_Di16_Do8_Lut16_G8_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc4to4_Di8_Do16_Lut16_G8_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc4to4_Di16_Do16_Lut16_G8_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 

CMError	LHCalc4to4_Di8_Do8_Lut16_G16_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc4to4_Di16_Do8_Lut16_G16_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc4to4_Di8_Do16_Lut16_G16_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 
CMError	LHCalc4to4_Di16_Do16_Lut16_G16_LO 	( CMCalcParamPtr calcParam,
											  CMLutParamPtr lutParam );		 

/*--------------------------------------------------------------------------------------------------------------
	calc routines for non in place matching
  --------------------------------------------------------------------------------------------------------------*/
#if LH_CALC_USE_ADDITIONAL_OLD_CODE
CMError	LHCalc3to3_Di8_Do8_Lut8_G16_Old		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to3_Di8_Do8_Lut8_G32_Old		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to4_Di8_Do8_Lut8_G16_Old		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc3to4_Di8_Do8_Lut8_G32_Old		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
#endif							 
#if LH_CALC_USE_ADDITIONAL_OLD_CODE_4DIM
CMError	LHCalc4to3_Di8_Do8_Lut8_G8_Old		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc4to3_Di8_Do8_Lut8_G16_Old		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc4to4_Di8_Do8_Lut8_G8_Old		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
CMError	LHCalc4to4_Di8_Do8_Lut8_G16_Old		( CMCalcParamPtr calcParam,
											  CMLutParamPtr  lutParam );
#endif							 
#endif							 
