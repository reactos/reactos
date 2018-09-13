/*
 * isguids.c - Internet Shortcut GUID definitions.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop


/* GUIDs
 ********/

#pragma data_seg(DATA_SEG_READ_ONLY)

#pragma warning(disable:4001) /* "single line comment" warning */
#include <initguid.h>

#define NO_SHELL_GUIDS
#include <isguids.h>

#pragma data_seg()

