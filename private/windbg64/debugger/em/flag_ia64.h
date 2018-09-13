/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

// IPSR flags
    { szIpsrBN,   ftRegular | ftExtended, 1, CV_IA64_StIPSR, 44 },
    { szIpsrED,   ftRegular | ftExtended, 1, CV_IA64_StIPSR, 43 },
    { szIpsrRI,   ftRegular | ftExtended, 2, CV_IA64_StIPSR, 41 },
    { szIpsrSS,   ftRegular | ftExtended, 1, CV_IA64_StIPSR, 40 },
    { szIpsrDD,   ftRegular | ftExtended, 1, CV_IA64_StIPSR, 39 },
    { szIpsrDA,   ftRegular | ftExtended, 1, CV_IA64_StIPSR, 38 },
    { szIpsrID,   ftRegular | ftExtended, 1, CV_IA64_StIPSR, 37 },
    { szIpsrIT,   ftRegular | ftExtended, 1, CV_IA64_StIPSR, 36 },
	       
    { szIpsrME,   ftRegular | ftExtended, 1, CV_IA64_StIPSR, 35 },
    { szIpsrIS,   ftRegular | ftExtended, 1, CV_IA64_StIPSR, 34 },
    { szIpsrCPL,   ftRegular | ftExtended, 2, CV_IA64_StIPSR, 32 },
    { szIpsrRT,   ftRegular | ftExtended, 1, CV_IA64_StIPSR, 27 },
    { szIpsrTB,   ftRegular | ftExtended, 1, CV_IA64_StIPSR, 26 },
    { szIpsrLP,   ftRegular | ftExtended, 1, CV_IA64_StIPSR, 25 },
    { szIpsrDB,   ftRegular | ftExtended, 1, CV_IA64_StIPSR, 24 },
	       
    { szIpsrSI,   ftRegular | ftExtended, 1, CV_IA64_StIPSR, 23 },
    { szIpsrDI,   ftRegular | ftExtended, 1, CV_IA64_StIPSR, 22 },
    { szIpsrPP,   ftRegular | ftExtended, 1, CV_IA64_StIPSR, 21 },
    { szIpsrSP,   ftRegular | ftExtended, 1, CV_IA64_StIPSR, 20 },
    { szIpsrDFH,   ftRegular | ftExtended, 1, CV_IA64_StIPSR, 19 },
    { szIpsrDFL,   ftRegular | ftExtended, 1, CV_IA64_StIPSR, 18 },
    { szIpsrDT,   ftRegular | ftExtended, 1, CV_IA64_StIPSR, 17 },
	       
    { szIpsrPK,   ftRegular | ftExtended, 1, CV_IA64_StIPSR, 15 },
    { szIpsrI,   ftRegular | ftExtended, 1, CV_IA64_StIPSR, 14 },
    { szIpsrIC,   ftRegular | ftExtended, 1, CV_IA64_StIPSR, 13 },
    { szIpsrAC,   ftRegular | ftExtended, 1, CV_IA64_StIPSR, 3 },
    { szIpsrUP,   ftRegular | ftExtended, 1, CV_IA64_StIPSR, 2 },
    { szIpsrBE,   ftRegular | ftExtended, 1, CV_IA64_StIPSR, 1 },
    { szIpsrOR,   ftRegular | ftExtended, 1, CV_IA64_StIPSR, 0 },

// FPSR flags

    { szFpsrMDH,   ftFloat | ftExtended, 1, CV_IA64_StIPSR, 63 },
    { szFpsrMDL,   ftFloat | ftExtended, 1, CV_IA64_StIPSR, 62 },
    { szFpsrSF3,   ftFloat | ftExtended, 13, CV_IA64_StIPSR, 45 },
    { szFpsrSF2,   ftFloat | ftExtended, 13, CV_IA64_StIPSR, 32 },
    { szFpsrSF1,   ftFloat | ftExtended, 13, CV_IA64_StIPSR, 19 },
    { szFpsrSF0,   ftFloat | ftExtended, 13, CV_IA64_StIPSR, 6 },
    { szFpsrTRAPID,   ftFloat | ftExtended, 1, CV_IA64_StIPSR, 5 },
    { szFpsrTRAPUD,   ftFloat | ftExtended, 1, CV_IA64_StIPSR, 4 },
    { szFpsrTRAPOD,   ftFloat | ftExtended, 1, CV_IA64_StIPSR, 3 },
    { szFpsrTRAPZD,   ftFloat | ftExtended, 1, CV_IA64_StIPSR, 2 },
    { szFpsrTRAPDD,   ftFloat | ftExtended, 1, CV_IA64_StIPSR, 1 },
    { szFpsrTRAPVD,   ftFloat | ftExtended, 1, CV_IA64_StIPSR, 0 }
