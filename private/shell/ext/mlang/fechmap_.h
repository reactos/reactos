/*----------------------------------------------------------------------------
	%%File: fechmap_.h
	%%Unit: fechmap
	%%Contact: jpick

	Internal header file for FarEast character conversion module.
----------------------------------------------------------------------------*/

#ifndef FECHMAP__H
#define FECHMAP__H

#include <windows.h>
#include <stdio.h>
#include <stddef.h>

#include "msencode.h"
//#include "assert.h"


// Character encoding types supported by this module.
// Internal version -- broader than the set exposed publicly.
// (Doing this since much of the groundwork is already in
// for future support don't want to remove or ifdef out that
// code).
//
// Main DLL entry points manage the correspondence between
// external and internal encoding types.
//
typedef enum _icet		// Internal Character Encoding Type
	{
	icetNil = -1,
	icetEucCn = 0,
	icetEucJp,
	icetEucKr,
	icetEucTw,
	icetIso2022Cn,
	icetIso2022Jp,
	icetIso2022Kr,
	icetIso2022Tw,
	icetBig5,
	icetGbk,
	icetHz,
	icetShiftJis,
	icetWansung,
	icetUtf7,
	icetUtf8,
	icetCount,
	} ICET;


// Miscellaneous useful definitions
//
#define fTrue	(BOOL) 1
#define fFalse	(BOOL) 0


// MS Code Page Definitions
//
#define nCpJapan		932
#define nCpChina		936
#define nCpKorea		949
#define nCpTaiwan		950

#define FIsFeCp(cp) \
	(((cp) == nCpJapan) || ((cp) == nCpChina) || ((cp) == nCpKorea) || ((cp) == nCpTaiwan))

#define WchFromUchUch(uchLead, uchTrail) \
	(WCHAR) ((((UCHAR)(uchLead)) << 8) | ((UCHAR)(uchTrail)))

// Prototype for internal auto-detection code
//
CCE CceDetermineInputType(
    IStream   *pstmIn,           // input stream
	DWORD     dwFlags,
	EFam      efPref,
	int       nPrefCp,
	ICET     *lpicet,
	BOOL     *lpfGuess
);

// Prototype for ISO-2022 escape sequence interpreter.
//
CCE CceReadEscSeq(IStream *pstmIn, ICET *lpicet);

#endif					// #ifndef FECHMAP__H
