/****************************************************************************
*
*
*	FILENAME:		crypto.h
*
*	PRODUCT NAME:	CRYPTOGRAPHIC TOOLKIT
*
*	FILE STATUS:
*
*	DESCRIPTION:	Cryptographic Toolkit File
*					Common Definitions
*		    
*
*	PUBLIC FUNCTIONS:
*
*
*	REVISION  HISTORY:
*
*
*		10 Feb 96	AK		Created
*
*
* Created for Cylink Corporation by Secant
*
****************************************************************************/


#ifndef CRYPTO_H
#define CRYPTO_H

/* For C++ */
#ifdef __cplusplus
extern "C" {
#endif

/*************************************
*
* Module Defines
*
*************************************/

#define	FALSE		0
#define	TRUE        1
#define	SUCCESS		0

/*-- ANSI-recommended NULL Pointer definition --*/
#ifndef	NULL
#define	NULL		(void *) 0
#endif


/*************************************
*
* Error Definitions
*
*************************************/
#define	ERR_ALLOC		-1



/*************************************
*
* Type Definitions
*
*************************************/
typedef unsigned char	uchar;
typedef unsigned short	ushort;
typedef unsigned long	ulong;
typedef	unsigned char	BYTE;
typedef	unsigned short	USHORT;
typedef	unsigned int	UINT;
typedef	unsigned long	ULONG;
typedef int		BOOL;


#ifdef ORD_16
typedef unsigned short ord;
typedef unsigned long dord;
#endif
#ifdef ORD_32
typedef unsigned long ord;
typedef unsigned long dord;
#endif


#ifdef __cplusplus
}
#endif


#endif     /* CRYPTO_H */

