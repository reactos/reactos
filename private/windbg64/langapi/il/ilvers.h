//this file is used to hold version constant used by the compiler components
//to check the compatibility of the il.  there are three constants. there is
//a product id, ILID_PRODUCT.  this will express a combination of language
//target.
//
//there are two il ids.  the ILID_P1 is a long that will check consistency
//between p1 and p2/p3/qc il files.  if a change is
//made to the ex, gl, sy, in, or db files so as to introduce an incapability
//then the ILID_P1 should be changed to reflect the date the change was 
//instituted in the form yyyymmddL.
//
//likewise we will maintain an ILID_P2 which will be used to check
//consistency whenever a change is made to the ls, gs, or pr files.


#ifndef _VC_VER_INC
#if VERSP_ALPHA
#include "vcver.h"
#else
#include "..\include\vcver.h"
#endif
#endif

#if (T8086)
#define ILID_PRODUCT  0x0086	// cmerge 8086
#elif (VERSP_PPC || CC_PPC)
#define ILID_PRODUCT  0x0601	// cmerge PPC
#elif (VERSP_68K || CC_ALAR)
#define ILID_PRODUCT  0x6800	// cmerge 68K
#elif (VERSP_P7)
#define ILID_PRODUCT  0x0786    // IL for P7 (i786)
#elif (VERSP_C386 || T386)
#define ILID_PRODUCT  0x0386	// cmerge 386
#elif (VERSP_MIPS || TRX)
#define ILID_PRODUCT  0x0040	// Cmerge il for R4000
#elif (VERSP_ALPHA)
#define ILID_PRODUCT  0xDECA	// cmerge il for DEC Alpha AXP
#elif (VERSP_OMNIVM)
#define ILID_PRODUCT  0xFDE9	// IL for Omni VM O-15-f M-13-d N-14-e I-9-9
#endif

#define ILID_P1_C8	19930414L	// After Dolphin M3 these will be static (with a CC_ flag probably)
#define ILID_P1		19931217L
#define ILID_P2		19930414L
#if _VC_VER >= 300
#define ILID_P1_ICC 19940811L	// P1 ICC IL
#endif
#if _VC_VER >= 500
#define ILID_P1_CVT32          19960416L
#define ILID_P1_CVT32_ICC      19960417L
#define ILID_P1_OMNI           19960701L
#define ILID_P1_OMNI_CVT32     19960702L
#define ILID_P1_OMNI_ICC       19960703L
#define ILID_P1_OMNI_CVT32_ICC 19960704L
#endif
