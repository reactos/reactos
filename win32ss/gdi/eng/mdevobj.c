/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Support for meta devices
 * FILE:             win32ss/gdi/eng/mdevobj.c
 * PROGRAMER:        Hervé Poussineau
 */

#include <win32k.h>
#define NDEBUG
#include <debug.h>

PMDEVOBJ gpmdev = NULL; /* FIXME: should be stored in gpDispInfo->pmdev */
