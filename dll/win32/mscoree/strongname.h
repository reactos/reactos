/*
 * Copyright (C) 2019 Zebediah Figura for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __STRONG_NAME_H
#define __STRONG_NAME_H

#include <windows.h>
#include <wincrypt.h>
#include <ole2.h>
#include <corerror.h>

#define SN_INFLAG_FORCE_VER     0x00000001
#define SN_INFLAG_INSTALL       0x00000002
#define SN_INFLAG_ADMIN_ACCESS  0x00000004
#define SN_INFLAG_USER_ACCESS   0x00000008
#define SN_INFLAG_ALL_ACCESS    0x00000010
#define SN_INFLAG_RUNTIME       0x80000000

#define SN_OUTFLAG_WAS_VERIFIED 0x00000001

BOOLEAN __stdcall StrongNameSignatureVerification(const WCHAR *path, DWORD flags, DWORD *ret_flags);
BOOLEAN __stdcall StrongNameSignatureVerificationEx(const WCHAR *path, BOOLEAN force, BOOLEAN *verified);
BOOLEAN __stdcall StrongNameTokenFromAssembly(const WCHAR *path, BYTE **token, ULONG *size);

#endif
