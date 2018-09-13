/***************************************************************************\
* Module Name: kbdjpn.c
*
* Copyright (c) 1985-92, Microsoft Corporation
*
* History:
\***************************************************************************/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#if ((BASE_KEYBOARD_LAYOUT) == 101)
/*
 * include kbd101.c (PC/AT 101 English keyboard layout driver)
 */
#include "..\101\kbd101.c"
#elif ((BASE_KEYBOARD_LAYOUT) == 106)
/*
 * include kbd106.c (PC/AT 106 Japanese keyboard layout driver)
 */
#include "..\106\kbd106.c"
#else
#error "BASE_KEYBOARD_LAYOUT should be 101 or 106."
#endif

