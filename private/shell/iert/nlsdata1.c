/***
*nlsdata1.c - globals for international library - small globals
*
*       Copyright (c) 1991-1994, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This module contains the globals:  __mb_cur_max, _decimal_point,
*       _decimal_point_length.  This module is always required.
*       This module is separated from nlsdatax.c for granularity.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdlib.h>
#include <nlsint.h>

/*
 *  Value of MB_CUR_MAX macro.
 */
int __mb_cur_max = 1;

/*
 *  Localized decimal point string.
 */
char __decimal_point[] = ".";

/*
 *  Decimal point length, not including terminating null.
 */
size_t __decimal_point_length = 1;

