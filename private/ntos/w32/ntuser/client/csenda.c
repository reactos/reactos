/**************************************************************************\
* Module Name: csenda.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* client side sending stubs for ANSI text
*
* History:
* 06-Jan-1992 IanJa
\**************************************************************************/

#define CLIENTSIDE 1

#undef UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "ntcsrdll.h"
#include <ntos.h>
#include "usercli.h"
#include <stdlib.h>

/**************************************************************************\
*
* include the stub definition file
*
\**************************************************************************/

#include "ntcftxt.h"
