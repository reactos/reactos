/****************************** Module Header ******************************\
* Module Name: keybrdc.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* History:
* 11-11-90 DavidPe      Created.
* 13-Feb-1991 mikeke    Added Revalidation code (None)
* 12-Mar-1993 JerrySh   Pulled functions from user\server.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/**************************************************************************\
* GetKBCodePage
*
* 28-May-1992 IanJa    Created
\**************************************************************************/

UINT GetKBCodePage(VOID)
{
    return GetOEMCP();

}
