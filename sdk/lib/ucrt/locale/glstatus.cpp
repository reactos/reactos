/***
*glstatus.c - sets the __globallocalestatus flag
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Sets the __globallocalestatus flag to disable per thread locale
*
*******************************************************************************/

#include <corecrt_internal.h>

extern "C" int __globallocalestatus = (~_GLOBAL_LOCALE_BIT);
