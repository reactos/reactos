/*
 * Copyright 2011 Samuel Serapión
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
 *
 */
#include "ntlmssp.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(ntlm);

/***********************************************************************
 *              MakeSignature
 */
SECURITY_STATUS SEC_ENTRY MakeSignature(PCtxtHandle phContext, ULONG fQOP,
 PSecBufferDesc pMessage, ULONG MessageSeqNo)
{
    SECURITY_STATUS ret = SEC_E_UNSUPPORTED_FUNCTION;

    TRACE("%p %p %d %p\n", phContext, pMessage, MessageSeqNo, fQOP);
    if(!phContext)
        return SEC_E_INVALID_HANDLE;

    if(!pMessage || !pMessage->pBuffers || pMessage->cBuffers < 2)
        return SEC_E_INVALID_TOKEN;

    FIXME("MakeSignature unimplemented\n");
    return ret;
}

/***********************************************************************
 *              VerifySignature
 */
SECURITY_STATUS SEC_ENTRY VerifySignature(PCtxtHandle phContext,
 PSecBufferDesc pMessage, ULONG MessageSeqNo, PULONG pfQOP)
{
    SECURITY_STATUS ret = SEC_E_UNSUPPORTED_FUNCTION;

    TRACE("%p %p %d %p\n", phContext, pMessage, MessageSeqNo, pfQOP);
    if(!phContext)
        return SEC_E_INVALID_HANDLE;

    if(!pMessage || !pMessage->pBuffers || pMessage->cBuffers < 2)
        return SEC_E_INVALID_TOKEN;

    FIXME("VerifySignature unimplemented\n");
    return ret;
}

