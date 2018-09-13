//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       Common header file for the tlmunge application.
//
//----------------------------------------------------------------------------


#define _OLEAUT32_
#define INC_OLE2
#define WIN32_LEAN_AND_MEAN

#define _KERNEL32_
#include <windows.h>
#include <platform.h>
#include <w4warn.h>
#include <stdio.h>
#include <stddef.h>
#include <search.h>
#include <string.h>
//  Allow assignment within conditional
#pragma warning ( disable : 4706 )
