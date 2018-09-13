/*----------------------------------------------------------------------------
	%%File: msencode.h
	%%Unit: fechmap
	%%Contact: jpick

	External header file for MsEncode character conversion module.
----------------------------------------------------------------------------*/

#ifndef MSENCODE_H
#define MSENCODE_H
	
	
// ----------------------------------------------------------------------------
//
// Error Returns
// 
// ----------------------------------------------------------------------------

//
// Return Type for API Functions
//
typedef int CCE;
	
//
// Error:       cceSuccess
// Explanation: Function succeeded (no error).
//
#define cceSuccess						 0

//
// Error:       cceRequestedStop
// Explanation: Function succeeded (no error).  Caller 
//				requested function to be run in iterator mode 
//				(stop on each character or stop on ASCII) and 
//				function is making requested stop.  (Stream 
//				conversion functions only).
//
#define cceRequestedStop				(-1)

//
// Error:       cceInsufficientBuffer
// Explanation: Buffer provided to function is too small.
//
#define cceInsufficientBuffer			(-2)

//
// Error:       cceInvalidFlags
// Explanation: An invalid flag or combination of flags was 
//				given to function.
//
#define cceInvalidFlags					(-3)

//
// Error:       cceInvalidParameter
// Explanation: Invalid parameter passed to function (null 
//				pointer, invalid encoding specified, etc.).
//
#define cceInvalidParameter				(-4)

//
// Error:       cceRead
// Explanation: User read-callback function failed.
//
#define cceRead							(-5)

//
// Error:       cceWrite
// Explanation: User write-callback function failed.
//
#define cceWrite						(-6)

//
// Error:       cceUnget
// Explanation: User unget-callback function failed.
//
#define cceUnget						(-7)

//
// Error:       cceNoCodePage
// Explanation: Requested encoding requires an installed
//				code page (NLS file) for conversion.  That
//				file is not installed.
//
#define cceNoCodePage					(-8)

//
// Error:       cceEndOfInput
// Explanation: Unexpected end-of-input occurred within a 
//				multi-byte character in conversion function.
//              (Returned only if user requested errors for
//              invalid characters).
//
#define cceEndOfInput					(-9)

//
// Error:       cceNoTranslation
// Explanation: Character in input stream or string has no 
//				equivalent Unicode (multi-byte to Unicode) or
//              multi-byte (Unicode to multi-byte) character.
//              (Returned only if user requested errors for
//              invalid characters).
//
#define cceNoTranslation				(-10)

//
// Error:       cceInvalidChar
// Explanation: Converter found a single or multi-byte character
//				that is outside the legal range for the given
//				encoding.  (Returned only if user requested 
//				errors for invalid characters).
//
#define cceInvalidChar					(-11)

//
// Error:       cceAmbiguousInput
// Explanation: CceDetectInputCode(), only.  Data matches more
//				than one of the supported encodings types.
//              (Returned only if function told to not resolve
//              ambiguity).
//
#define cceAmbiguousInput				(-12)

//
// Error:       cceUnknownInput
// Explanation:	CceDetectInputCode(), only.  Data matches none
//				of the supported encoding types.
//
#define cceUnknownInput					(-13)

//
// Error:       cceMayBeAscii
// Explanation:	CceDetectInputCode(), only.  Technically, data
//              matches at least one of the supported encoding
//              types, but may not be a true match.  (For example,
//              an ASCII file with only a few scattered extended
//              characters).  (Returned only if function told to
//              resolve ambiguity).
//
//              This is not an error, only a flag to the calling
//              application.  CceDetectInputCode() will still set
//              the encoding type if it returns this value.
//
#define cceMayBeAscii					(-14)

//
// Error:       cceInternal
// Explanation: Unrecoverable internal error.
//
#define cceInternal						(-15)

//
// Error:       cceConvert
// Explanation: Unexpected DBCS function conversion error.
//
#define cceConvert						(-16)

//
// Error:       cceEncodingNotImplemented
// Explanation: Temporary integration error.  Requested encoding
//				is not implemented.
//
#define cceEncodingNotImplemented		(-100)

//
// Error:       cceFunctionNotImplemented
// Explanation: Temporary integration error.  Function
//				is not implemented.
//
#define cceFunctionNotImplemented		(-101)



// ----------------------------------------------------------------------------
//
// General Definitions for Modules Using these Routines
// 
// ----------------------------------------------------------------------------

#define MSENAPI					PASCAL
#define MSENCBACK				PASCAL
#define EXPIMPL(type)			type MSENAPI
#define EXPDECL(type)			extern type MSENAPI

// In case these are not already defined
//
#ifndef FAR
#ifdef _WIN32
#define FAR		__far
#else
#define FAR
#endif
#endif

typedef unsigned char UCHAR;
typedef UCHAR *PUCHAR;
typedef UCHAR FAR *LPUSTR;
typedef const UCHAR FAR *LPCUSTR;

#ifndef UNIX // IEUNIX uses 4 bytes WCHAR, these are already defined in winnt.h
typedef unsigned short WCHAR;
typedef WCHAR *PWCHAR;
typedef WCHAR FAR *LPWSTR;
typedef const WCHAR FAR *LPCWSTR;
#endif

//
// Character encoding types supported by this module.
//
typedef enum _cenc
	{
	ceNil = -1,
	ceEucCn = 0,
	ceEucJp,
	ceEucKr,
	ceIso2022Jp,
	ceIso2022Kr,
	ceBig5,
	ceGbk,
	ceHz,
	ceShiftJis,
	ceWansung,
	ceUtf7,
	ceUtf8,
	ceCount,
	};

typedef short CEnc;
	
//
// Encoding "families" (for CceDetectInputCode() preferences).
//
typedef enum _efam
	{
	efNone = 0,
	efDbcs,
	efEuc,
	efIso2022,
	efUtf8,
	} EFam;

//
// API private/reserved structure.  For most API functions,
// this structure must be zero-filled by calling application.
// See converter function documentation, below, for more
// information.
//
#define cdwReserved		4
typedef struct _ars
	{
	DWORD rgdw[cdwReserved];
	} ARS;

// For GetProcAddress()
typedef void (MSENAPI *PFNMSENCODEVER)(WORD FAR *, WORD FAR *);

// ----------------------------------------------------------------------------
//
// Input Code Auto-Detection Routine
// 
// ----------------------------------------------------------------------------

//
// Configuration Flags for Auto Detection Routine
// 
//   grfDetectResolveAmbiguity
//       The default is to return cceAmbiguousInput if the auto
//       detection code cannot definitely determine the encoding
//       of the input stream.  If this flag is set, the function
//       will use optional user preferences and the system code 
//       page to pick an encoding (note that in this case, the
//       "lpfGuess" flag will be set to fTrue upon return).
// 
//   grfDetectUseCharMapping
//       The default action of the auto-detection code is to 
//       parse the input against the known encoding types.  Legal 
//       character sequences are not analyzed for anything 
//       beyond syntactic correctness.  If this flag is set, 
//       auto-detect will map recognized sequences to flush out
//       invalid characters.
//
//       This option will cause auto-detection to run more 
//       slowly, but also yield more accurate results.
// 
//   grfDetectIgnoreEof
//       Because auto-detect parses byte sequences against the
//       the known encoding types, end-of-input in the middle of a 
//       sequence is obviously an error.  If the calling application
//       will artificially limit the sample size, set this flag
//       to ignore such end-of-input errors.
//
#define	grfDetectResolveAmbiguity		0x1
#define grfDetectUseCharMapping			0x2
#define grfDetectIgnoreEof				0x4

//
// Entry Point -- Attempt to Detect the Encoding
//
//    Return cceAmbiguousInput if input is ambiguous or cceUnknownInput
//    if encoding type matches none of the known types.
//
//    Detected encoding is returned in lpCe.  lpfGuess used to return
//    a flag indicating whether or not the function "guessed" at an
//    encoding (chose default from ambiguous state).
//
//    User preferences for encoding family (efPref) and code page
//    (nPrefCp) are optional, even if caller chooses to have
//    this function attempt to resolve ambiguity.  If either has
//    the value 0, they will be ignored.
//
EXPDECL(CCE)
CceDetectInputCode(
    IStream   *pstmIn,           // input stream
	DWORD     dwFlags,			// configuration flags
	EFam      efPref,			// optional: preferred encoding family
	int       nPrefCp,			// optional: preferred code page
	UINT      *lpCe,				// set to detected encoding
	BOOL      *lpfGuess			// set to fTrue if function "guessed"
);

#endif			// #ifndef MSENCODE_H
