/***************************************************************************
 * Module: T2EMBAPI.H
 *
 * Copyright (c) Microsoft Corp., 1996, 1997
 *
 * Author: Paul Linnerud (paulli)
 * Date:   May 1996
 *
 * Mods:
 *
 * Header file for the TrueType embedding services dll (T2EMBED.DLL)
 *
 **************************************************************************/

#ifndef I_T2EMBAPI_H_
#define I_T2EMBAPI_H_
#pragma INCMSG("--- Beg 't2embapi.h'")

#if !defined(_T2API_LIB_)
#define T2API __declspec(dllimport)
#else
#define T2API __declspec(dllexport)
#endif

// Charset flags for ulCharSet field of TTEmbedFont
#if !defined(CHARSET_UNICODE)
#define CHARSET_UNICODE                    1
#define CHARSET_DEFAULT                    1
#define CHARSET_SYMBOL                     2
#endif

// Status returned by TTLoadEmbeddedFont
#if !defined(EMBED_PREVIEWPRINT)
#define EMBED_PREVIEWPRINT                 1
#define EMBED_EDITABLE                     2
#define EMBED_INSTALLABLE                  3
#define EMBED_NOEMBEDDING                  4
#endif

// Use restriction flags
#if !defined(LICENSE_INSTALLABLE)
#define LICENSE_INSTALLABLE             0x0000
#define LICENSE_DEFAULT                 0x0000
#define LICENSE_NOEMBEDDING             0x0002
#define LICENSE_PREVIEWPRINT            0x0004
#define LICENSE_EDITABLE                0x0008
#endif

// Options given to TTEmbedFont in uFlags parameter
#if !defined(TTEMBED_RAW)
#define TTEMBED_RAW							0x00000000
#define TTEMBED_SUBSET						0x00000001
#define TTEMBED_TTCOMPRESSED				0x00000004
#define TTEMBED_FAILIFVARIATIONSIMULATED	0x00000010
#define TTEMBED_XORENCRYPTDATA				0x10000000 // internal
#endif

// Bits returned through pulStatus for TTEmbedFont
#if !defined(TTEMBED_VARIATIONSIMULATED)
#define TTEMBED_VARIATIONSIMULATED		0x00000001					
#endif

// Flag options for TTLoadEmbeddedFont 
#if !defined(TTLOAD_PRIVATE)
#define TTLOAD_PRIVATE                  0x00000001 
#endif

// Bits returned through pulStatus for TTLoadEmbeddedFont 
#if !defined(TTLOAD_FONT_SUBSETTED)
#define TTLOAD_FONT_SUBSETTED		0x00000001
#define TTLOAD_FONT_IN_SYSSTARTUP	0x00000002
#endif

// Flag options for TTDeleteEmbeddedFont
#if !defined(TTDELETE_DONTREMOVEFONT)
#define TTDELETE_DONTREMOVEFONT		0x00000001	
#endif

// Error codes
#if !defined(E_NONE)
#define E_NONE                      0x0000L
#endif

// Top level error codes
#if !defined(E_CHARCODECOUNTINVALID)
#define E_CHARCODECOUNTINVALID      0x0002L
#define E_CHARCODESETINVALID        0x0003L
#define E_DEVICETRUETYPEFONT        0x0004L
#define E_HDCINVALID                0x0006L
#define E_NOFREEMEMORY              0x0007L
#define E_FONTREFERENCEINVALID      0x0008L
#define E_NOTATRUETYPEFONT          0x000AL
#define E_ERRORACCESSINGFONTDATA    0x000CL
#define E_ERRORACCESSINGFACENAME    0x000DL
#define E_ERRORUNICODECONVERSION    0x0011L
#define E_ERRORCONVERTINGCHARS      0x0012L
#define E_EXCEPTION					0x0013L
#define E_RESERVEDPARAMNOTNULL		0x0014L	
#define E_CHARSETINVALID			0x0015L
#define E_WIN32S_NOTSUPPORTED		0x0016L
#endif

// Indep level error codes 
#if !defined(E_ERRORCOMPRESSINGFONTDATA)
#define E_ERRORCOMPRESSINGFONTDATA    0x0100L
#define E_FONTDATAINVALID             0x0102L
#define E_NAMECHANGEFAILED            0x0103L
#define E_FONTNOTEMBEDDABLE           0x0104L
#define E_PRIVSINVALID                0x0105L
#define E_SUBSETTINGFAILED            0x0106L
#define E_READFROMSTREAMFAILED        0x0107L
#define E_SAVETOSTREAMFAILED          0x0108L
#define E_NOOS2                       0x0109L
#define E_T2NOFREEMEMORY              0x010AL
#define E_ERRORREADINGFONTDATA        0x010BL
#define E_FLAGSINVALID                0x010CL
#define E_ERRORCREATINGFONTFILE       0x010DL
#define E_FONTALREADYEXISTS           0x010EL
#define E_FONTNAMEALREADYEXISTS       0x010FL
#define E_FONTINSTALLFAILED           0x0110L
#define E_ERRORDECOMPRESSINGFONTDATA  0x0111L
#define E_ERRORACCESSINGEXCLUDELIST   0x0112L
#define E_FACENAMEINVALID			  0x0113L
#define E_STREAMINVALID               0x0114L
#define E_STATUSINVALID				  0x0115L
#define E_PRIVSTATUSINVALID			  0x0116L
#define E_PERMISSIONSINVALID		  0x0117L
#define E_PBENABLEDINVALID			  0x0118L
#define E_SUBSETTINGEXCEPTION		  0x0119L
#define E_SUBSTRING_TEST_FAIL		  0x011AL
#define E_FONTVARIATIONSIMULATED	  0x011BL
#endif

// Bottom level error codes
#if !defined(E_ADDFONTFAILED)
#define E_ADDFONTFAILED             0x0200L
#define E_COULDNTCREATETEMPFILE     0x0201L
#define E_FONTFILECREATEFAILED      0x0203L
#define E_WINDOWSAPI                0x0204L
#define E_FONTFILENOTFOUND          0x0205L
#define E_RESOURCEFILECREATEFAILED  0x0206L
#define E_ERROREXPANDINGFONTDATA    0x0207L
#define E_ERRORGETTINGDC            0x0208L
#define E_EXCEPTIONINDECOMPRESSION	0x0209L
#define E_EXCEPTIONINCOMPRESSION	0x020AL
#endif

#ifdef __cplusplus
extern "C" {
#endif

// 1st argument - Stream identifier (file handle or other) (dwStream) */
// 2nd argument - Address of buffer with data to read or write */
// 3rd argument - Number of bytes to read or write */
typedef unsigned long( __cdecl *READEMBEDPROC ) ( void*, void*, const unsigned long );
typedef unsigned long( __cdecl *WRITEEMBEDPROC ) ( void*, const void*, const unsigned long );

#if !defined(_TTLOADINFO_DEFINED)
typedef struct
{
	unsigned short usStructSize;	// size in bytes of structure client should set to sizeof(TTLOADINFO)
	unsigned short usRefStrSize;	// size in wide characters of pusRefStr including NULL terminator
	LPTSTR  pusRefStr;		// reference or actual string. 
}TTLOADINFO;
#define _TTLOADINFO_DEFINED
#endif

#if !defined(_TTEMBEDINFO_DEFINED)
typedef struct
{
	unsigned short usStructSize;	// size in bytes of structure client should set to sizeof(TTEMBEDINFO)
	unsigned short usRootStrSize;   // size in wide chars of pusSubStr including NULL terminator(s)
	LPTSTR  pusRootStr;		// substring(s) of strings given at load time. can have multiple strings separated
									//  by a NULL terminator. 
}TTEMBEDINFO;
#define _TTEMBEDINFO_DEFINED
#endif

/* Font Embedding APIs ----------------------------------------------------*/

T2API LONG WINAPI TTEmbedFont
(
	HDC       hDC,                    // device-context handle
	ULONG     ulFlags,                // flags specifying the request
	ULONG     ulCharSet,              // flags specifying char set
	ULONG*    pulPrivStatus,          // upon completion contains embedding priv of font
	ULONG*    pulStatus,              // on completion may contain status flags for request
	WRITEEMBEDPROC lpfnWriteToStream, // callback function for doc/disk writes
	LPVOID    lpvWriteStream,         // the output stream tokin
	USHORT*   pusCharCodeSet,         // address of buffer containing optional
									  // character codes for subsetting
	USHORT    usCharCodeCount,        // number of characters in the
									  // lpvCharCodeSet buffer
	USHORT    usLanguage,             // specifies the language in the name table to keep
									  //  set to 0 to keep all
	TTEMBEDINFO* pTTEmbedInfo         // optional security
);


T2API LONG WINAPI TTLoadEmbeddedFont
(
	HANDLE*   phFontReference,			// on completion, contains handle to identify embedded font installed
										// on system
	ULONG	  ulFlags,					// flags specifying the request 
	ULONG*    pulPrivStatus,			// on completion, contains the embedding status
	ULONG     ulPrivs,					// allows for the reduction of licensing privileges
	ULONG*    pulStatus,				// on completion, may contain status flags for request 
	READEMBEDPROC lpfnReadFromStream,	// callback function for doc/disk reads
	LPVOID    lpvReadStream,			// the input stream tokin
	LPWSTR    szWinFamilyName,			// the new 16 bit windows family name can be NULL
	LPSTR	  szMacFamilyName,			// the new 8 bit mac family name can be NULL
	TTLOADINFO* pTTLoadInfo				// optional security
);

T2API LONG WINAPI TTDeleteEmbeddedFont
(
	HANDLE    hFontReference,	// Reference to font value provided by load functions										
	ULONG	  ulFlags,
	ULONG*    pulStatus
);

T2API LONG WINAPI TTGetEmbeddingType
(                                                                       
	HDC         hDC,                   // device context handle
	ULONG*      pulEmbedType           // upon completion, contains the
									   // embedding status
);

T2API LONG WINAPI TTCharToUnicode
(	
	HDC			hDC,				// device context handle
	UCHAR*		pucCharCodes,		// array of 8 bit character codes to convert
	ULONG		ulCharCodeSize,		// size of 8 bit character code array
	USHORT*     pusShortCodes,		// buffer to recieve Unicode code points
	ULONG		ulShortCodeSize,	// size in wide characters of 16 bit character code array
	ULONG		ulFlags				// Control flags
);


/* Font Enabling APIs -----------------------------------------------------*/

T2API LONG WINAPI TTIsEmbeddingEnabled
(                                                                       
	HDC                     hDC,            // device context handle                                                                
	BOOL*           pbEnabled       // upon completion will indicate if enabled
);                                                              

T2API LONG WINAPI TTIsEmbeddingEnabledForFacename
(                                                                       
	LPSTR           lpszFacename,   // facename
	BOOL*           pbEnabled       // upon completion will indicate if enabled
);

T2API LONG WINAPI TTEnableEmbeddingForFacename
(                                   // If fEnable != 0, it removes the indicated
	LPSTR           lpszFacename,   // typeface name from the "embedding
	BOOL            bEnable         // exclusion list".  Else, it enters the
);                                  // indicated typeface name in the "embedding
									// exclusion list". 

#ifdef __cplusplus
}
#endif

#pragma INCMSG("--- End 't2embapi.h'")
#else
#pragma INCMSG("*** Dup 't2embapi.h'")
#endif
