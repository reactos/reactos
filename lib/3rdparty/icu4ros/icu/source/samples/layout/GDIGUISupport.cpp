/*
 *******************************************************************************
 *
 *   Copyright (C) 1999-2001, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 *   file name:  GDIGUISupport.h
 *
 *   created on: 11/06/2001
 *   created by: Eric R. Mader
 */

#include <windows.h>

#include "GDIGUISupport.h"

void GDIGUISupport::postErrorMessage(const char *message, const char *title)
{
    MessageBoxA(NULL, message, title, MB_ICONERROR);
}

