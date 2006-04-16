/*
 * Copyright 2005 Juan Lang
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __CRYPT32_PRIVATE_H__
#define __CRYPT32_PRIVATE_H__

/* The following aren't defined in wincrypt.h, as they're "reserved" */
#define CERT_CERT_PROP_ID 32
#define CERT_CRL_PROP_ID  33
#define CERT_CTL_PROP_ID  34

/* Returns a handle to the default crypto provider; loads it if necessary.
 * Returns NULL on failure.
 */
HCRYPTPROV CRYPT_GetDefaultProvider(void);

void crypt_oid_init(HINSTANCE hinst);
void crypt_oid_free(void);

/* Helper function for store reading functions and
 * CertAddSerializedElementToStore.  Returns a context of the appropriate type
 * if it can, or NULL otherwise.  Doesn't validate any of the properties in
 * the serialized context (for example, bad hashes are retained.)
 * *pdwContentType is set to the type of the returned context.
 */
const void *CRYPT_ReadSerializedElement(const BYTE *pbElement,
 DWORD cbElement, DWORD dwContextTypeFlags, DWORD *pdwContentType);

/**
 *  Context property list functions
 */
struct _CONTEXT_PROPERTY_LIST;
typedef struct _CONTEXT_PROPERTY_LIST *PCONTEXT_PROPERTY_LIST;

PCONTEXT_PROPERTY_LIST ContextPropertyList_Create(void);

/* Searches for the property with ID id in the context.  Returns TRUE if found,
 * and copies the property's length and a pointer to its data to blob.
 * Otherwise returns FALSE.
 */
BOOL ContextPropertyList_FindProperty(PCONTEXT_PROPERTY_LIST list, DWORD id,
 PCRYPT_DATA_BLOB blob);

BOOL ContextPropertyList_SetProperty(PCONTEXT_PROPERTY_LIST list, DWORD id,
 const BYTE *pbData, size_t cbData);

void ContextPropertyList_RemoveProperty(PCONTEXT_PROPERTY_LIST list, DWORD id);

DWORD ContextPropertyList_EnumPropIDs(PCONTEXT_PROPERTY_LIST list, DWORD id);

void ContextPropertyList_Copy(PCONTEXT_PROPERTY_LIST to,
 PCONTEXT_PROPERTY_LIST from);

void ContextPropertyList_Free(PCONTEXT_PROPERTY_LIST list);

#endif
