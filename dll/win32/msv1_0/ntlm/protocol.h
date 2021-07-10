/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ntlm protocol definitions (header)
 * COPYRIGHT:   Copyright 2011 Samuel Serapión
 *              Copyright 2020 Andreas Maier (staubim@quantentunnel.de)
 */
#ifndef _MSV1_0_NTLM_PROTOCOL_H_
#define _MSV1_0_NTLM_PROTOCOL_H_

typedef struct _CYPHER_BLOCK
{
    CHAR data[8];
} CYPHER_BLOCK, *PCYPHER_BLOCK;

typedef struct _USER_SESSION_KEY
{
    CYPHER_BLOCK data[2];
} USER_SESSION_KEY, *PUSER_SESSION_KEY;

typedef struct _LANMAN_SESSION_KEY
{
    UCHAR data[MSV1_0_LANMAN_SESSION_KEY_LENGTH];
} LANMAN_SESSION_KEY, *PLANMAN_SESSION_KEY;

#endif
