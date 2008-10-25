/*
 *  Copyright 2004 Hans Leidekker
 *
 *  Based on LMHash.c from libcifs
 *
 *  Copyright (C) 2004 by Christopher R. Hertel
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <advapi32.h>
#include "crypt.h"


static const unsigned char CRYPT_LMhash_Magic[8] =
    { 'K', 'G', 'S', '!', '@', '#', '$', '%' };

static void CRYPT_LMhash(LPSTR dst, LPCSTR pwd, const int len)
{
    int i, max = 14;
    CHAR tmp_pwd[14] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

    max = len > max ? max : len;

    for (i = 0; i < max; i++)
        tmp_pwd[i] = pwd[i];

    CRYPT_DEShash((PUCHAR)dst, (PUCHAR)tmp_pwd, CRYPT_LMhash_Magic);
    CRYPT_DEShash((PUCHAR)&dst[8], (PUCHAR)&tmp_pwd[7], CRYPT_LMhash_Magic);
}

NTSTATUS WINAPI SystemFunction006(LPCSTR password, LPSTR hash)
{
    CRYPT_LMhash(hash, password, strlen(password));

    return STATUS_SUCCESS;
}
