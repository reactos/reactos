/***********************************************************************
//
// ALIAS.H
//
//		Copyright (c) 1992 - Microsoft Corp.
//		All rights reserved.
//		Microsoft Confidential
//
// Global constants and data types used by the Jaguar file engine.
//
***********************************************************************/

//**********************************************************************
// Normal variable typedefs. These type defs are compatible with OS2
// typedefs.
//**********************************************************************

#ifndef	CHAR
	typedef		char					CHAR;
#endif

#ifndef	UCHAR
	typedef		unsigned char		UCHAR;
#endif

#ifndef	INT
	typedef		int					INT;
#endif

#ifndef	UINT
	typedef		unsigned int		UINT;
#endif

#ifndef	UL
	typedef		unsigned long		UL;
#endif

#if 0
#ifndef	FLOAT
	typedef		float 				FLOAT;
#endif
#endif

#ifndef	DOUBLE
	typedef		double				DOUBLE;
#endif

#ifndef	LONG
	typedef		long					LONG;
#endif

//**********************************************************************
//	ANY_TYPE
//
//	This is a union which can be used to cast any type to a basic
//	data type.
// 
//**********************************************************************

typedef union
{
   CHAR			Byte;
	UCHAR			uByte;
	INT			Word;
	UINT			uWord;
	LONG			Dword;
	UL				uDword;
} ANY_TYPE;

//**********************************************************************
// Standard global constants.
// Don't change the TRUE define because some functions depend on it being
// 1 instead of !FALSE.
//**********************************************************************

#ifndef		FALSE
   #define     FALSE          0
   #define     TRUE           1
#endif

#ifndef		EOL
   #define     EOL            '\0'
#endif

#ifndef	  OK
   #define		OK			      0
#endif

//**********************************************************************
// DeReference macro for unused function arguments
//**********************************************************************

#ifndef	DeReference		
	#ifdef	_lint
		#define	DeReference( x )	x = x
	#else
		#define	DeReference( x )	x = x
	#endif
#endif

//**********************************************************************
// MAX_PATH is the max path string length.
//**********************************************************************
#ifndef WIN32
#ifndef	MAX_PATH
	#define		MAX_PATH		256				// Really 64 but compatible with OS2
#else
	#if			MAX_PATH != 256				// Error check
		#error
	#endif
#endif
#endif
	
#ifndef	MAX_DIR_DEPTH
	#define	MAX_DIR_DEPTH		32				//	Directory levels supported by DOS
#else
	#if			MAX_DIR_DEPTH != 32			// Error check
		#error
	#endif
#endif

#ifndef	DIR_NAME_LEN
	#define		DIR_NAME_LEN	(8+3)			// Len of FCB type file name
#else
	#if			DIR_NAME_LEN != (8+3)		// Error check
		#error
	#endif
#endif
