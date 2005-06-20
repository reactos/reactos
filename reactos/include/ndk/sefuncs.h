/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/sefuncs.h
 * PURPOSE:         Defintions for Security Subsystem Functions not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _SEFUNCS_H
#define _SEFUNCS_H

/* DEPENDENCIES **************************************************************/

/* FUNCTION TYPES ************************************************************/

/* PROTOTYPES ****************************************************************/

SECURITY_IMPERSONATION_LEVEL
STDCALL
SeTokenImpersonationLevel(
    IN PACCESS_TOKEN Token
);

#endif
