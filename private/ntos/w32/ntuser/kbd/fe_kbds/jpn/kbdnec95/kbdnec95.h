#if defined(_PC98_)
/****************************** Module Header ******************************\
* Module Name: kbdnec95.h
*
* Copyright (c) 1985-98, Microsoft Corporation
*
* Various defines for use by keyboard input code.
*
* History:
* 27-May-1992 KazuM
\***************************************************************************/
#else
/****************************** Module Header ******************************\
* Module Name: kbd106.h
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* Various defines for use by keyboard input code.
*
* History:
\***************************************************************************/
#endif

/*
 * kbd type should be controlled by cl command-line argument
 */
#if defined(_PC98_)
#define KBD_TYPE 37
#else
#define KBD_TYPE 8
#endif

/*
 * Include the basis of all keyboard table values
 */
#include "kbd.h"
