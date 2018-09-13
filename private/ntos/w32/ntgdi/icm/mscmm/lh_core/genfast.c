/*
	File:		LHCalcGenerator.c

	Contains:	

	Written by:	U. J. Krabbenhoeft

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

	Version:	

*/

/* #define LH_CALC_ENGINE_SMALL see LHGeneralIncs.h */

#ifndef LHGeneralIncs_h
#include "General.h"
#endif

#ifdef ALLOW_MMX
#ifndef LHCalcEngine_h
#include "CalcEng.h"
#endif

#ifdef DEBUG_OUTPUT
#define kThisFile kLHCalcGeneratorID
#endif

#define LH_ADR_BREIT_EIN_LUT   	adr_breite_elut
#define LH_ADR_BREIT_AUS_LUT   	adr_breite_alut

#undef LH_DATA_IN_COUNT_4
#undef LH_DATA_IN_SIZE_16
#undef LH_DATA_OUT_SIZE_16
#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		4


#undef LH_DATA_IN_COUNT_4
#undef LH_DATA_OUT_COUNT_4
#undef LH_DATA_IN_SIZE_16
#undef LH_DATA_OUT_SIZE_16
#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		4


#if LH_Calc3to3_Di8_Do8_Lut8_G16 == LH_CALC_USE_BIG_ENGINE
CMError LHCalc3to3_Di8_Do8_Lut8_G16_F( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to3_Di8_Do8_Lut8_G16_F"
#include "EngineF.c"
#endif

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		5


#if LH_Calc3to3_Di8_Do8_Lut8_G32 == LH_CALC_USE_BIG_ENGINE
CMError LHCalc3to3_Di8_Do8_Lut8_G32_F( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to3_Di8_Do8_Lut8_G32_F"
#include "EngineF.c"
#endif

#undef LH_DATA_OUT_COUNT_4
#undef LH_DATA_IN_SIZE_16
#undef LH_DATA_OUT_SIZE_16
#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		4


#if LH_Calc3to4_Di8_Do8_Lut8_G16 == LH_CALC_USE_BIG_ENGINE
CMError LHCalc3to4_Di8_Do8_Lut8_G16_F( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to4_Di8_Do8_Lut8_G16_F"
#include "EngineF.c"
#endif


#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		5


#if LH_Calc3to4_Di8_Do8_Lut8_G32 == LH_CALC_USE_BIG_ENGINE
CMError LHCalc3to4_Di8_Do8_Lut8_G32_F( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to4_Di8_Do8_Lut8_G32_F"
#include "EngineF.c"
#endif

#undef LH_DATA_IN_COUNT_4
#undef LH_DATA_OUT_COUNT_4
#undef LH_DATA_IN_SIZE_16
#undef LH_DATA_OUT_SIZE_16
#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		3


#if LH_Calc4to3_Di8_Do8_Lut8_G8 == LH_CALC_USE_BIG_ENGINE
CMError LHCalc4to3_Di8_Do8_Lut8_G8_F( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to3_Di8_Do8_Lut8_G8_F"
#include "EngineF.c"
#endif


#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		4


#if LH_Calc4to3_Di8_Do8_Lut8_G16 == LH_CALC_USE_BIG_ENGINE
CMError LHCalc4to3_Di8_Do8_Lut8_G16_F( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to3_Di8_Do8_Lut8_G16_F"
#include "EngineF.c"
#endif


#undef LH_DATA_OUT_COUNT_4
#undef LH_DATA_IN_SIZE_16
#undef LH_DATA_OUT_SIZE_16
#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		3


#if LH_Calc4to4_Di8_Do8_Lut8_G8 == LH_CALC_USE_BIG_ENGINE
CMError LHCalc4to4_Di8_Do8_Lut8_G8_F( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to4_Di8_Do8_Lut8_G8_F"
#include "EngineF.c"
#endif


#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		4


#if LH_Calc4to4_Di8_Do8_Lut8_G16 == LH_CALC_USE_BIG_ENGINE
CMError LHCalc4to4_Di8_Do8_Lut8_G16_F( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to4_Di8_Do8_Lut8_G16_F"
#include "EngineF.c"
#endif

#endif
