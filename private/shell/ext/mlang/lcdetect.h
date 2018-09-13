/*
 * Automatic language and codepage detection
 * 
 * Bob Powell, 2/97
 * Copyright (C) 1996, 1997, Microsoft Corp.  All rights reserved.
 */

#if !defined( __LCDETECT_H__ )
#define __LCDETECT_H__

typedef struct LCDScore {
	UINT nLangID;			// Win32 primary language ID
	UINT nCodePage;			// Win32 code page (valid for SBCS input only!)
	int nDocPercent;		// % of doc in this language, 0-100
	int nConfidence;		// Relative confidence measure, approx 0-100
} LCDScore;
typedef struct LCDScore * PLCDScore;

typedef struct LCDConfigure {
	int nMin7BitScore;		// per-char score threshhold for 7-bit detection
	int nMin8BitScore;		// " " for 8-bit
	int nMinUnicodeScore;	// " " for Unicode
	int nRelativeThreshhold;// relative "" as % of the top scoring doc, 0-100
	int nDocPctThreshhold;	// min % of doc in a language to score it, 0-100
	int nChunkSize;			// # of chars to process at a time
} LCDConfigure;
typedef struct LCDConfigure *PLCDConfigure;
typedef struct LCDConfigure const *PCLCDConfigure;

// Pass in rough body text in pStr, length nChars
// Pass in preallocated LCDScore array in paScores, array size in *pnScores.
// On return, *pnScores is set to number of elements containing result data.
//
// If pLCDC is NULL, the default configuration is used.
// To detect with a custom configuration, call LCD_GetConfig() to fill in
// a copy of an LCDConfigure, and then pas it to LCD_Detect().

extern "C" DWORD WINAPI LCD_Detect (LPCSTR pStr, int nChars, 
							PLCDScore paScores, int *pnScores,
							PCLCDConfigure pLCDC);

extern "C" DWORD WINAPI LCD_DetectW (LPCWSTR pwStr, int nChars,
							PLCDScore paScores, int *pnScores,
							PCLCDConfigure pLCDC);

extern "C" void WINAPI LCD_GetConfig (PLCDConfigure pLCDC);

#endif
