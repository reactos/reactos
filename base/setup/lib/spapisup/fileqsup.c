/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Setup Library
 * FILE:            base/setup/lib/fileqsup.c
 * PURPOSE:         Interfacing with Setup* API File Queue support functions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"
#include "fileqsup.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/*
 * These externs should be defined by the user of this library.
 * They are kept there for reference and ease of usage.
 */
SPFILE_EXPORTS SpFileExports = {NULL};

/* EOF */
