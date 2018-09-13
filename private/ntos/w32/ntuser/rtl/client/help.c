/****************************** Module Header ******************************\
* Module Name: help.c
*
* Copyright (c) 1985-96, Microsoft Corporation
*
* This file is included in the SOURCES= line of ntuser\rtl\sources.inc.
*  It allows BUILD.EXE and $(NTMAKEENV)\MAKEFILE.DEF to work with source
*  files that are not stored in the current directory or the current
*  directory's parent.
*  Note also that a specific dependency line is included in makefile.inc
*   so the corresponding object is properly rebuilt when the source file
*   included below changes.
*
* History:
* Feb-14-1996 GerardoB Created
\***************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include "..\help.c"

