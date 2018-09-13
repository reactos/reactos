/*
 * guids.cpp - GUID definitions.
 */


/* Headers
 **********/

#include "project.hpp"
#pragma hdrstop


/* GUIDs
 ********/

#pragma data_seg(DATA_SEG_READ_ONLY)

#include <initguid.h>

#define NO_INTSHCUT_GUIDS
#include <shlguid.h>

#include "ftps.hpp"
#include "inetps.hpp"

#pragma data_seg()

