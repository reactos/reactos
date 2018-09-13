/*
	File:		LHGeneralIncs.h

	Contains:	General interfaces for MAC OR 'platfrom independent'. This is the PC- Version !!

	Written by:	U. J. Krabbenhoeft

	Copyright:	й 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

*/

#ifndef LHGeneralIncs_h
#define LHGeneralIncs_h

#if defined(_X86_)
#define ALLOW_MMX
#endif

#ifndef PI_BasicTypes_h
#include "PI_Basic.h"
#endif

#ifndef PI_Machine_h
#include "PI_Mach.h"
#endif

#ifndef PI_Memory_h
#include "PI_Mem.h"
#endif

#define LUTS_ARE_PTR_BASED 1

#ifndef LHDefines_h
#include "Defines.h"
#endif

#ifndef LHICCProfile_h
#include "Profile.h"
#endif

#ifndef PI_Application_h
#include "PI_App.h"
#endif

#ifndef RenderInt
#ifndef PI_PrivateProfAccess_h
#include "PI_Priv.h"
#endif
#endif

#ifndef DEBUG_OUTPUT
#define LH_START_PROC(x)
#define LH_END_PROC(x)
#endif

#define BlockMoveData BlockMove

#ifndef LHTypeDefs_h
#include "TypeDefs.h"
#endif

#ifdef IntelMode
#ifndef PI_SwapMem_h
#include "PI_Swap.h"
#endif
#endif

#define realThing 1

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=mac68k
#endif

#if PRAGMA_IMPORT_SUPPORTED
#pragma import on
#endif


/* our stuff without Core Includes */
#define VOLATILE(x)			if ((x));

enum {
    kCMMNewLinkProfile			= 1
};


#ifndef IntelMode
#define CMHelperICC2int16(a, b) 																\
		  (*((UINT16 *)(a))) = (*((UINT16*)(b)));
#define CMHelperICC2int32(a, b) 																\
		  (*((UINT32 *)(a))) = (*((UINT32*)(b)));
#else
#define CMHelperICC2int16(a, b) 																\
		  (*((UINT16 *)(a))) = ((UINT16)(((UINT8 *)(b))[1]))         | ((UINT16)(((UINT8 *)(b))[0] << 8));
#define CMHelperICC2int32(a, b) 																\
		  (*((UINT32 *)(a))) = ((UINT32)(((UINT8 *)(b))[3]))         | (((UINT32)(((UINT8 *)(b))[2])) << 8) | \
		        (((UINT32)(((UINT8 *)(b))[1])) << 16) | (((UINT32)(((UINT8 *)(b))[0])) << 24);
#endif

/*#define _SIZET */
/*typedef long Size; */
#ifdef __cplusplus
extern "C" {
#endif

void GetDateTime(unsigned long *secs);
extern void SecondsToDate(unsigned long secs, DateTimeRec *d);

void BlockMove(const void *srcPtr, void *destPtr, Size byteCount);
void SetMem(void *bytePtr, size_t numBytes, unsigned char byteValue);

#ifdef __cplusplus
}
#endif

#if PRAGMA_IMPORT_SUPPORTED
#pragma import off
#endif

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=reset
#endif

/*#define LH_CALC_ENGINE_16BIT_DATAFORMAT 1
#define LH_CALC_ENGINE_SMALL 1  */
#define LH_CALC_ENGINE_BIG				0	/* 1 -> Speed optimized code for all data and lut formats */
#define LH_CALC_ENGINE_ALL_FORMATS_LO	1	/* 1 -> Speed optimized code for 'looukup only' for all data and lut formats */
#define LH_CALC_ENGINE_16_BIT_LO		0	/* 1 -> Speed optimized code for 'looukup only' for 8->16 and 16->8 data and all lut formats */
#define LH_CALC_ENGINE_MIXED_DATAFORMAT	0	/* 1 -> Speed optimized code for 'looukup only' for 8->16 and 16->8 data and all lut formats */

#define LH_CALC_USE_ADDITIONAL_OLD_CODE			1	/* 1 turns on the additional generation of the old pixel cache routines for 3 dim input	*/
#define LH_CALC_USE_ADDITIONAL_OLD_CODE_4DIM	0	/* 1 turns on the additional generation of the old pixel cache routines for 4 dim input	*/		
/*			еееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееее			*/
/*			Use LH_CALC_USE_SMALL_ENGINE on BOTH LH_Calc3to3 and LH_Calc3to4 cases !!! 			*/

#define LH_CALC_USE_DO_N_DIM 		0	/* no speed optimized code for this funktion */
#define LH_CALC_USE_SMALL_ENGINE 	1	/* speed optimized code for this funktion */
#define LH_CALC_USE_BIG_ENGINE		2	/* full speed optimized code for this funktion */

#if ! LH_CALC_ENGINE_BIG
/* 		change this part ONLY!!!			*/

#define LH_Calc1toX_Di8_Do8_Lut8_G128	LH_CALC_USE_SMALL_ENGINE	/* use LH_CALC_USE_SMALL_ENGINE for speed optimized code */
#define LH_Calc1toX_Di8_Do8_Lut16_G128	LH_CALC_USE_SMALL_ENGINE 	/* else use LH_CALC_USE_DO_N_DIM for no speed optimization */
#define LH_Calc1toX_Di8_Do16_Lut8_G128	LH_CALC_USE_DO_N_DIM
#define LH_Calc1toX_Di8_Do16_Lut16_G128 LH_CALC_USE_DO_N_DIM
#define LH_Calc1toX_Di16_Do8_Lut8_G128	LH_CALC_USE_DO_N_DIM
#define LH_Calc1toX_Di16_Do8_Lut16_G128 LH_CALC_USE_DO_N_DIM
#define LH_Calc1toX_Di16_Do16_Lut8_G128 LH_CALC_USE_SMALL_ENGINE
#define LH_Calc1toX_Di16_Do16_Lut16_G128 LH_CALC_USE_SMALL_ENGINE

#define LH_Calc3to3_Di8_Do8_Lut8_G16     LH_CALC_USE_BIG_ENGINE
#define LH_Calc3to3_Di8_Do8_Lut8_G32     LH_CALC_USE_BIG_ENGINE
#define LH_Calc3to3_Di8_Do8_Lut16_G16    LH_CALC_USE_BIG_ENGINE
#define LH_Calc3to3_Di8_Do8_Lut16_G32    LH_CALC_USE_BIG_ENGINE

#define LH_Calc3to3_Di8_Do16_Lut8_G16    LH_CALC_USE_DO_N_DIM
#define LH_Calc3to3_Di8_Do16_Lut8_G32    LH_CALC_USE_DO_N_DIM
#define LH_Calc3to3_Di8_Do16_Lut16_G16   LH_CALC_USE_DO_N_DIM
#define LH_Calc3to3_Di8_Do16_Lut16_G32   LH_CALC_USE_DO_N_DIM

#define LH_Calc3to3_Di16_Do8_Lut8_G16    LH_CALC_USE_DO_N_DIM
#define LH_Calc3to3_Di16_Do8_Lut8_G32    LH_CALC_USE_DO_N_DIM
#define LH_Calc3to3_Di16_Do8_Lut16_G16   LH_CALC_USE_DO_N_DIM
#define LH_Calc3to3_Di16_Do8_Lut16_G32   LH_CALC_USE_DO_N_DIM

#define LH_Calc3to3_Di16_Do16_Lut8_G16   LH_CALC_USE_SMALL_ENGINE
#define LH_Calc3to3_Di16_Do16_Lut8_G32   LH_CALC_USE_SMALL_ENGINE
#define LH_Calc3to3_Di16_Do16_Lut16_G16  LH_CALC_USE_SMALL_ENGINE
#define LH_Calc3to3_Di16_Do16_Lut16_G32  LH_CALC_USE_SMALL_ENGINE

#define LH_Calc3to4_Di8_Do8_Lut8_G16     LH_CALC_USE_BIG_ENGINE
#define LH_Calc3to4_Di8_Do8_Lut8_G32     LH_CALC_USE_BIG_ENGINE
#define LH_Calc3to4_Di8_Do8_Lut16_G16    LH_CALC_USE_BIG_ENGINE
#define LH_Calc3to4_Di8_Do8_Lut16_G32    LH_CALC_USE_BIG_ENGINE

#define LH_Calc3to4_Di8_Do16_Lut8_G16    LH_CALC_USE_DO_N_DIM
#define LH_Calc3to4_Di8_Do16_Lut8_G32    LH_CALC_USE_DO_N_DIM
#define LH_Calc3to4_Di8_Do16_Lut16_G16   LH_CALC_USE_DO_N_DIM
#define LH_Calc3to4_Di8_Do16_Lut16_G32   LH_CALC_USE_DO_N_DIM

#define LH_Calc3to4_Di16_Do8_Lut8_G16    LH_CALC_USE_DO_N_DIM
#define LH_Calc3to4_Di16_Do8_Lut8_G32    LH_CALC_USE_DO_N_DIM
#define LH_Calc3to4_Di16_Do8_Lut16_G16   LH_CALC_USE_DO_N_DIM
#define LH_Calc3to4_Di16_Do8_Lut16_G32   LH_CALC_USE_DO_N_DIM

#define LH_Calc3to4_Di16_Do16_Lut8_G16   LH_CALC_USE_SMALL_ENGINE
#define LH_Calc3to4_Di16_Do16_Lut8_G32   LH_CALC_USE_SMALL_ENGINE
#define LH_Calc3to4_Di16_Do16_Lut16_G16  LH_CALC_USE_SMALL_ENGINE
#define LH_Calc3to4_Di16_Do16_Lut16_G32  LH_CALC_USE_SMALL_ENGINE


#define LH_Calc4to3_Di8_Do8_Lut8_G8      LH_CALC_USE_BIG_ENGINE
#define LH_Calc4to3_Di8_Do8_Lut8_G16     LH_CALC_USE_BIG_ENGINE
#define LH_Calc4to3_Di8_Do8_Lut16_G8     LH_CALC_USE_BIG_ENGINE
#define LH_Calc4to3_Di8_Do8_Lut16_G16    LH_CALC_USE_BIG_ENGINE

#define LH_Calc4to3_Di8_Do16_Lut8_G8     LH_CALC_USE_DO_N_DIM
#define LH_Calc4to3_Di8_Do16_Lut8_G16    LH_CALC_USE_DO_N_DIM
#define LH_Calc4to3_Di8_Do16_Lut16_G8    LH_CALC_USE_DO_N_DIM
#define LH_Calc4to3_Di8_Do16_Lut16_G16   LH_CALC_USE_DO_N_DIM

#define LH_Calc4to3_Di16_Do8_Lut8_G8     LH_CALC_USE_DO_N_DIM
#define LH_Calc4to3_Di16_Do8_Lut8_G16    LH_CALC_USE_DO_N_DIM
#define LH_Calc4to3_Di16_Do8_Lut16_G8    LH_CALC_USE_DO_N_DIM
#define LH_Calc4to3_Di16_Do8_Lut16_G16   LH_CALC_USE_DO_N_DIM

#define LH_Calc4to3_Di16_Do16_Lut8_G8    LH_CALC_USE_DO_N_DIM
#define LH_Calc4to3_Di16_Do16_Lut8_G16   LH_CALC_USE_DO_N_DIM
#define LH_Calc4to3_Di16_Do16_Lut16_G8   LH_CALC_USE_DO_N_DIM
#define LH_Calc4to3_Di16_Do16_Lut16_G16  LH_CALC_USE_DO_N_DIM

#define LH_Calc4to4_Di8_Do8_Lut8_G8      LH_CALC_USE_BIG_ENGINE
#define LH_Calc4to4_Di8_Do8_Lut8_G16     LH_CALC_USE_BIG_ENGINE
#define LH_Calc4to4_Di8_Do8_Lut16_G8     LH_CALC_USE_BIG_ENGINE
#define LH_Calc4to4_Di8_Do8_Lut16_G16    LH_CALC_USE_BIG_ENGINE

#define LH_Calc4to4_Di8_Do16_Lut8_G8     LH_CALC_USE_DO_N_DIM
#define LH_Calc4to4_Di8_Do16_Lut8_G16    LH_CALC_USE_DO_N_DIM
#define LH_Calc4to4_Di8_Do16_Lut16_G8    LH_CALC_USE_DO_N_DIM
#define LH_Calc4to4_Di8_Do16_Lut16_G16   LH_CALC_USE_DO_N_DIM

#define LH_Calc4to4_Di16_Do8_Lut8_G8     LH_CALC_USE_DO_N_DIM
#define LH_Calc4to4_Di16_Do8_Lut8_G16    LH_CALC_USE_DO_N_DIM
#define LH_Calc4to4_Di16_Do8_Lut16_G8    LH_CALC_USE_DO_N_DIM
#define LH_Calc4to4_Di16_Do8_Lut16_G16   LH_CALC_USE_DO_N_DIM

#define LH_Calc4to4_Di16_Do16_Lut8_G8    LH_CALC_USE_DO_N_DIM
#define LH_Calc4to4_Di16_Do16_Lut8_G16   LH_CALC_USE_DO_N_DIM
#define LH_Calc4to4_Di16_Do16_Lut16_G8   LH_CALC_USE_DO_N_DIM
#define LH_Calc4to4_Di16_Do16_Lut16_G16  LH_CALC_USE_DO_N_DIM

#else
/* 		do not change this part !!!			*/

#define LH_Calc1toX_Di8_Do8_Lut8_G128	LH_CALC_USE_SMALL_ENGINE	/* use LH_CALC_USE_SMALL_ENGINE for speed optimized code */
#define LH_Calc1toX_Di8_Do8_Lut16_G128	LH_CALC_USE_SMALL_ENGINE 	/* else use LH_CALC_USE_DO_N_DIM for no speed optimization */
#define LH_Calc1toX_Di8_Do16_Lut8_G128	LH_CALC_USE_SMALL_ENGINE
#define LH_Calc1toX_Di8_Do16_Lut16_G128 LH_CALC_USE_SMALL_ENGINE
#define LH_Calc1toX_Di16_Do8_Lut8_G128	LH_CALC_USE_SMALL_ENGINE
#define LH_Calc1toX_Di16_Do8_Lut16_G128 LH_CALC_USE_SMALL_ENGINE
#define LH_Calc1toX_Di16_Do16_Lut8_G128 LH_CALC_USE_SMALL_ENGINE
#define LH_Calc1toX_Di16_Do16_Lut16_G128 LH_CALC_USE_SMALL_ENGINE

#define LH_Calc3to3_Di8_Do8_Lut8_G16     LH_CALC_USE_BIG_ENGINE
#define LH_Calc3to3_Di8_Do8_Lut8_G32     LH_CALC_USE_BIG_ENGINE
#define LH_Calc3to3_Di8_Do8_Lut16_G16    LH_CALC_USE_BIG_ENGINE
#define LH_Calc3to3_Di8_Do8_Lut16_G32    LH_CALC_USE_BIG_ENGINE

#define LH_Calc3to3_Di8_Do16_Lut8_G16    LH_CALC_USE_BIG_ENGINE
#define LH_Calc3to3_Di8_Do16_Lut8_G32    LH_CALC_USE_BIG_ENGINE
#define LH_Calc3to3_Di8_Do16_Lut16_G16   LH_CALC_USE_BIG_ENGINE
#define LH_Calc3to3_Di8_Do16_Lut16_G32   LH_CALC_USE_BIG_ENGINE

#define LH_Calc3to3_Di16_Do8_Lut8_G16    LH_CALC_USE_BIG_ENGINE
#define LH_Calc3to3_Di16_Do8_Lut8_G32    LH_CALC_USE_BIG_ENGINE
#define LH_Calc3to3_Di16_Do8_Lut16_G16   LH_CALC_USE_BIG_ENGINE
#define LH_Calc3to3_Di16_Do8_Lut16_G32   LH_CALC_USE_BIG_ENGINE

#define LH_Calc3to3_Di16_Do16_Lut8_G16   LH_CALC_USE_BIG_ENGINE
#define LH_Calc3to3_Di16_Do16_Lut8_G32   LH_CALC_USE_BIG_ENGINE
#define LH_Calc3to3_Di16_Do16_Lut16_G16  LH_CALC_USE_BIG_ENGINE
#define LH_Calc3to3_Di16_Do16_Lut16_G32  LH_CALC_USE_BIG_ENGINE

#define LH_Calc3to4_Di8_Do8_Lut8_G16     LH_CALC_USE_BIG_ENGINE
#define LH_Calc3to4_Di8_Do8_Lut8_G32     LH_CALC_USE_BIG_ENGINE
#define LH_Calc3to4_Di8_Do8_Lut16_G16    LH_CALC_USE_BIG_ENGINE
#define LH_Calc3to4_Di8_Do8_Lut16_G32    LH_CALC_USE_BIG_ENGINE

#define LH_Calc3to4_Di8_Do16_Lut8_G16    LH_CALC_USE_BIG_ENGINE
#define LH_Calc3to4_Di8_Do16_Lut8_G32    LH_CALC_USE_BIG_ENGINE
#define LH_Calc3to4_Di8_Do16_Lut16_G16   LH_CALC_USE_BIG_ENGINE
#define LH_Calc3to4_Di8_Do16_Lut16_G32   LH_CALC_USE_BIG_ENGINE

#define LH_Calc3to4_Di16_Do8_Lut8_G16    LH_CALC_USE_BIG_ENGINE
#define LH_Calc3to4_Di16_Do8_Lut8_G32    LH_CALC_USE_BIG_ENGINE
#define LH_Calc3to4_Di16_Do8_Lut16_G16   LH_CALC_USE_BIG_ENGINE
#define LH_Calc3to4_Di16_Do8_Lut16_G32   LH_CALC_USE_BIG_ENGINE

#define LH_Calc3to4_Di16_Do16_Lut8_G16   LH_CALC_USE_BIG_ENGINE
#define LH_Calc3to4_Di16_Do16_Lut8_G32   LH_CALC_USE_BIG_ENGINE
#define LH_Calc3to4_Di16_Do16_Lut16_G16  LH_CALC_USE_BIG_ENGINE
#define LH_Calc3to4_Di16_Do16_Lut16_G32  LH_CALC_USE_BIG_ENGINE


#define LH_Calc4to3_Di8_Do8_Lut8_G8      LH_CALC_USE_BIG_ENGINE
#define LH_Calc4to3_Di8_Do8_Lut8_G16     LH_CALC_USE_BIG_ENGINE
#define LH_Calc4to3_Di8_Do8_Lut16_G8     LH_CALC_USE_BIG_ENGINE
#define LH_Calc4to3_Di8_Do8_Lut16_G16    LH_CALC_USE_BIG_ENGINE

#define LH_Calc4to3_Di8_Do16_Lut8_G8     LH_CALC_USE_BIG_ENGINE
#define LH_Calc4to3_Di8_Do16_Lut8_G16    LH_CALC_USE_BIG_ENGINE
#define LH_Calc4to3_Di8_Do16_Lut16_G8    LH_CALC_USE_BIG_ENGINE
#define LH_Calc4to3_Di8_Do16_Lut16_G16   LH_CALC_USE_BIG_ENGINE

#define LH_Calc4to3_Di16_Do8_Lut8_G8     LH_CALC_USE_BIG_ENGINE
#define LH_Calc4to3_Di16_Do8_Lut8_G16    LH_CALC_USE_BIG_ENGINE
#define LH_Calc4to3_Di16_Do8_Lut16_G8    LH_CALC_USE_BIG_ENGINE
#define LH_Calc4to3_Di16_Do8_Lut16_G16   LH_CALC_USE_BIG_ENGINE

#define LH_Calc4to3_Di16_Do16_Lut8_G8    LH_CALC_USE_BIG_ENGINE
#define LH_Calc4to3_Di16_Do16_Lut8_G16   LH_CALC_USE_BIG_ENGINE
#define LH_Calc4to3_Di16_Do16_Lut16_G8   LH_CALC_USE_BIG_ENGINE
#define LH_Calc4to3_Di16_Do16_Lut16_G16  LH_CALC_USE_BIG_ENGINE

#define LH_Calc4to4_Di8_Do8_Lut8_G8      LH_CALC_USE_BIG_ENGINE
#define LH_Calc4to4_Di8_Do8_Lut8_G16     LH_CALC_USE_BIG_ENGINE
#define LH_Calc4to4_Di8_Do8_Lut16_G8     LH_CALC_USE_BIG_ENGINE
#define LH_Calc4to4_Di8_Do8_Lut16_G16    LH_CALC_USE_BIG_ENGINE

#define LH_Calc4to4_Di8_Do16_Lut8_G8     LH_CALC_USE_BIG_ENGINE
#define LH_Calc4to4_Di8_Do16_Lut8_G16    LH_CALC_USE_BIG_ENGINE
#define LH_Calc4to4_Di8_Do16_Lut16_G8    LH_CALC_USE_BIG_ENGINE
#define LH_Calc4to4_Di8_Do16_Lut16_G16   LH_CALC_USE_BIG_ENGINE

#define LH_Calc4to4_Di16_Do8_Lut8_G8     LH_CALC_USE_BIG_ENGINE
#define LH_Calc4to4_Di16_Do8_Lut8_G16    LH_CALC_USE_BIG_ENGINE
#define LH_Calc4to4_Di16_Do8_Lut16_G8    LH_CALC_USE_BIG_ENGINE
#define LH_Calc4to4_Di16_Do8_Lut16_G16   LH_CALC_USE_BIG_ENGINE

#define LH_Calc4to4_Di16_Do16_Lut8_G8    LH_CALC_USE_BIG_ENGINE
#define LH_Calc4to4_Di16_Do16_Lut8_G16   LH_CALC_USE_BIG_ENGINE
#define LH_Calc4to4_Di16_Do16_Lut16_G8   LH_CALC_USE_BIG_ENGINE
#define LH_Calc4to4_Di16_Do16_Lut16_G16  LH_CALC_USE_BIG_ENGINE

#endif

#endif /* } */
