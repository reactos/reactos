/*
	File:		LHCalcGeneratorOnlyLookup.c

	Contains:	

	Written by:	U. J. Krabbenhoeft

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

	Version:	
*/

/* #define LH_CALC_ENGINE_SMALL */

#ifndef LHGeneralIncs_h
#include "General.h"
#endif

#ifndef LHCalcEngine_h
#include "CalcEng.h"
#endif

#ifdef DEBUG_OUTPUT
#define kThisFile kLHCalcGeneratorLookupID
#endif

#define LH_ADR_BREIT_EIN_LUT   	adr_breite_elut
#define LH_ADR_BREIT_AUS_LUT   	adr_breite_alut


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


#if LH_CALC_ENGINE_ALL_FORMATS_LO
CMError LHCalc3to3_Di8_Do8_Lut8_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to3_Di8_Do8_Lut8_G16_LO"
#include "Engineup.c"
#endif

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		5


CMError LHCalc3to3_Di8_Do8_Lut8_G32_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to3_Di8_Do8_Lut8_G32_LO"
#include "Engineup.c"

#if LH_CALC_ENGINE_ALL_FORMATS_LO
#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		4


CMError LHCalc3to3_Di8_Do8_Lut16_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to3_Di8_Do8_Lut16_G16_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		5


CMError LHCalc3to3_Di8_Do8_Lut16_G32_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to3_Di8_Do8_Lut16_G32_LO"
#include "Engineup.c"

#if LH_CALC_ENGINE_MIXED_DATAFORMAT
#undef LH_DATA_OUT_SIZE_16
#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		4


CMError LHCalc3to3_Di8_Do16_Lut8_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to3_Di8_Do16_Lut8_G16_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		5


CMError LHCalc3to3_Di8_Do16_Lut8_G32_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to3_Di8_Do16_Lut8_G32_LO"
#include "Engineup.c"

#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		4


CMError LHCalc3to3_Di8_Do16_Lut16_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to3_Di8_Do16_Lut16_G16_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		5


CMError LHCalc3to3_Di8_Do16_Lut16_G32_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to3_Di8_Do16_Lut16_G32_LO"
#include "Engineup.c"

#undef LH_DATA_IN_SIZE_16
#undef LH_DATA_OUT_SIZE_16
#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		4


CMError LHCalc3to3_Di16_Do8_Lut8_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to3_Di16_Do8_Lut8_G16_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		5


CMError LHCalc3to3_Di16_Do8_Lut8_G32_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to3_Di16_Do8_Lut8_G32_LO"
#include "Engineup.c"

#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		4


CMError LHCalc3to3_Di16_Do8_Lut16_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to3_Di16_Do8_Lut16_G16_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		5


CMError LHCalc3to3_Di16_Do8_Lut16_G32_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to3_Di16_Do8_Lut16_G32_LO"
#include "Engineup.c"
#else
#undef LH_DATA_IN_SIZE_16
#endif

#if LH_CALC_ENGINE_16_BIT_LO
#undef LH_DATA_OUT_SIZE_16
#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		4


CMError LHCalc3to3_Di16_Do16_Lut8_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to3_Di16_Do16_Lut8_G16_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		5


CMError LHCalc3to3_Di16_Do16_Lut8_G32_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to3_Di16_Do16_Lut8_G32_LO"
#include "Engineup.c"

#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		4


CMError LHCalc3to3_Di16_Do16_Lut16_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to3_Di16_Do16_Lut16_G16_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		5


CMError LHCalc3to3_Di16_Do16_Lut16_G32_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to3_Di16_Do16_Lut16_G32_LO"
#include "Engineup.c"

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


CMError LHCalc3to4_Di8_Do8_Lut8_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to4_Di8_Do8_Lut8_G16_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		5


CMError LHCalc3to4_Di8_Do8_Lut8_G32_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to4_Di8_Do8_Lut8_G32_LO"
#include "Engineup.c"

#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		4


CMError LHCalc3to4_Di8_Do8_Lut16_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to4_Di8_Do8_Lut16_G16_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		5


CMError LHCalc3to4_Di8_Do8_Lut16_G32_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to4_Di8_Do8_Lut16_G32_LO"
#include "Engineup.c"

#if LH_CALC_ENGINE_MIXED_DATAFORMAT
#undef LH_DATA_OUT_SIZE_16
#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		4


CMError LHCalc3to4_Di8_Do16_Lut8_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to4_Di8_Do16_Lut8_G16_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		5


CMError LHCalc3to4_Di8_Do16_Lut8_G32_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to4_Di8_Do16_Lut8_G32_LO"
#include "Engineup.c"

#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		4


CMError LHCalc3to4_Di8_Do16_Lut16_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to4_Di8_Do16_Lut16_G16_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		5


CMError LHCalc3to4_Di8_Do16_Lut16_G32_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to4_Di8_Do16_Lut16_G32_LO"
#include "Engineup.c"

#undef LH_DATA_IN_SIZE_16
#undef LH_DATA_OUT_SIZE_16
#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		4


CMError LHCalc3to4_Di16_Do8_Lut8_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to4_Di16_Do8_Lut8_G16_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		5


CMError LHCalc3to4_Di16_Do8_Lut8_G32_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to4_Di16_Do8_Lut8_G32_LO"
#include "Engineup.c"

#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		4


CMError LHCalc3to4_Di16_Do8_Lut16_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to4_Di16_Do8_Lut16_G16_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		5


CMError LHCalc3to4_Di16_Do8_Lut16_G32_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to4_Di16_Do8_Lut16_G32_LO"
#include "Engineup.c"
#else
#undef LH_DATA_IN_SIZE_16
#endif

#if LH_CALC_ENGINE_16_BIT_LO
#undef LH_DATA_OUT_SIZE_16
#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		4


CMError LHCalc3to4_Di16_Do16_Lut8_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to4_Di16_Do16_Lut8_G16_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		5


CMError LHCalc3to4_Di16_Do16_Lut8_G32_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to4_Di16_Do16_Lut8_G32_LO"
#include "Engineup.c"

#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		4


CMError LHCalc3to4_Di16_Do16_Lut16_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to4_Di16_Do16_Lut16_G16_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		0
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		5


CMError LHCalc3to4_Di16_Do16_Lut16_G32_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc3to4_Di16_Do16_Lut16_G32_LO"
#include "Engineup.c"

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


CMError LHCalc4to3_Di8_Do8_Lut8_G8_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to3_Di8_Do8_Lut8_G8_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		4


CMError LHCalc4to3_Di8_Do8_Lut8_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to3_Di8_Do8_Lut8_G16_LO"
#include "Engineup.c"

#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		3


CMError LHCalc4to3_Di8_Do8_Lut16_G8_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to3_Di8_Do8_Lut16_G8_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		4


CMError LHCalc4to3_Di8_Do8_Lut16_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to3_Di8_Do8_Lut16_G16_LO"
#include "Engineup.c"

#if LH_CALC_ENGINE_MIXED_DATAFORMAT
#undef LH_DATA_OUT_SIZE_16
#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		3


CMError LHCalc4to3_Di8_Do16_Lut8_G8_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to3_Di8_Do16_Lut8_G8_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		4


CMError LHCalc4to3_Di8_Do16_Lut8_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to3_Di8_Do16_Lut8_G16_LO"
#include "Engineup.c"

#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		3


CMError LHCalc4to3_Di8_Do16_Lut16_G8_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to3_Di8_Do16_Lut8_G16_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		4


CMError LHCalc4to3_Di8_Do16_Lut16_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to3_Di8_Do16_Lut16_G16_LO"
#include "Engineup.c"

#undef LH_DATA_IN_SIZE_16
#undef LH_DATA_OUT_SIZE_16
#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		3


CMError LHCalc4to3_Di16_Do8_Lut8_G8_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to3_Di16_Do8_Lut8_G8_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		4


CMError LHCalc4to3_Di16_Do8_Lut8_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to3_Di16_Do8_Lut8_G16_LO"
#include "Engineup.c"

#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		3


CMError LHCalc4to3_Di16_Do8_Lut16_G8_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to3_Di16_Do8_Lut16_G8_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		4


CMError LHCalc4to3_Di16_Do8_Lut16_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to3_Di16_Do8_Lut16_G16_LO"
#include "Engineup.c"
#else
#undef LH_DATA_IN_SIZE_16
#endif

#if LH_CALC_ENGINE_16_BIT_LO
#undef LH_DATA_OUT_SIZE_16
#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		3


CMError LHCalc4to3_Di16_Do16_Lut8_G8_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to3_Di16_Do16_Lut8_G8_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		4


CMError LHCalc4to3_Di16_Do16_Lut8_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to3_Di16_Do16_Lut8_G16_LO"
#include "Engineup.c"

#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		3


CMError LHCalc4to3_Di16_Do16_Lut16_G8_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to3_Di16_Do16_Lut16_G8_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	0
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		4


CMError LHCalc4to3_Di16_Do16_Lut16_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to3_Di16_Do16_Lut16_G16_LO"
#include "Engineup.c"
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


CMError LHCalc4to4_Di8_Do8_Lut8_G8_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to4_Di8_Do8_Lut8_G8_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		4


CMError LHCalc4to4_Di8_Do8_Lut8_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to4_Di8_Do8_Lut8_G16_LO"
#include "Engineup.c"

#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		3


CMError LHCalc4to4_Di8_Do8_Lut16_G8_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to4_Di8_Do8_Lut16_G8_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		4


CMError LHCalc4to4_Di8_Do8_Lut16_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to4_Di8_Do8_Lut16_G16_LO"
#include "Engineup.c"

#if LH_CALC_ENGINE_MIXED_DATAFORMAT
#undef LH_DATA_OUT_SIZE_16
#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		3


CMError LHCalc4to4_Di8_Do16_Lut8_G8_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to4_Di8_Do16_Lut8_G8_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		4


CMError LHCalc4to4_Di8_Do16_Lut8_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to4_Di8_Do16_Lut8_G16_LO"
#include "Engineup.c"

#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		3


CMError LHCalc4to4_Di8_Do16_Lut16_G8_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to4_Di8_Do16_Lut16_G8_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		0
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		4


CMError LHCalc4to4_Di8_Do16_Lut16_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to4_Di8_Do16_Lut16_G16_LO"
#include "Engineup.c"

#undef LH_DATA_IN_SIZE_16
#undef LH_DATA_OUT_SIZE_16
#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		3


CMError LHCalc4to4_Di16_Do8_Lut8_G8_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to4_Di16_Do8_Lut8_G8_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		4


CMError LHCalc4to4_Di16_Do8_Lut8_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to4_Di16_Do8_Lut8_G16_LO"
#include "Engineup.c"

#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		3


CMError LHCalc4to4_Di16_Do8_Lut16_G8_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to4_Di16_Do8_Lut16_G8_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		0
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		4


CMError LHCalc4to4_Di16_Do8_Lut16_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to4_Di16_Do8_Lut16_G16_LO"
#include "Engineup.c"
#else
#undef LH_DATA_IN_SIZE_16
#endif

#if LH_CALC_ENGINE_16_BIT_LO
#undef LH_DATA_OUT_SIZE_16
#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		3


CMError LHCalc4to4_Di16_Do16_Lut8_G8_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to4_Di16_Do16_Lut8_G8_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		0
#define LH_BIT_BREIT_ADR		4


CMError LHCalc4to4_Di16_Do16_Lut8_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to4_Di16_Do16_Lut8_G16_LO"
#include "Engineup.c"

#undef LH_LUT_DATA_SIZE_16
#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		3


CMError LHCalc4to4_Di16_Do16_Lut16_G8_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to4_Di16_Do16_Lut16_G8_LO"
#include "Engineup.c"

#undef LH_BIT_BREIT_ADR

#define LH_DATA_IN_COUNT_4 		1
#define LH_DATA_OUT_COUNT_4 	1
#define LH_DATA_IN_SIZE_16 		1
#define LH_DATA_OUT_SIZE_16		1
#define LH_LUT_DATA_SIZE_16		1
#define LH_BIT_BREIT_ADR		4


CMError LHCalc4to4_Di16_Do16_Lut16_G16_LO( CMCalcParamPtr calcParam, CMLutParamPtr lutParam )
#define LH_CALC_PROC_NAME "LHCalc4to4_Di16_Do16_Lut16_G16_LO"
#include "Engineup.c"
#endif
#endif

