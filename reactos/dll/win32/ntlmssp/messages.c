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
 *             EncryptMessage
 */
SECURITY_STATUS SEC_ENTRY EncryptMessage(PCtxtHandle phContext,
        ULONG fQOP, PSecBufferDesc pMessage, ULONG MessageSeqNo)
{
    SECURITY_STATUS ret = SEC_E_UNSUPPORTED_FUNCTION;
    //int token_idx, data_idx;

    TRACE("(%p %d %p %d)\n", phContext, fQOP, pMessage, MessageSeqNo);

    if(!phContext)
        return SEC_E_INVALID_HANDLE;

    if(fQOP)
        FIXME("Ignoring fQOP\n");

    if(MessageSeqNo)
        FIXME("Ignoring MessageSeqNo\n");

    if(!pMessage || !pMessage->pBuffers || pMessage->cBuffers < 2)
        return SEC_E_INVALID_TOKEN;

    //if((token_idx = GetTokenBufferIndex(pMessage)) == -1)
    //    return SEC_E_INVALID_TOKEN;

    //if((data_idx = GetDataBufferIndex(pMessage)) ==-1 )
    //    return SEC_E_INVALID_TOKEN;

    //if(pMessage->pBuffers[token_idx].cbBuffer < 16)
    //    return SEC_E_BUFFER_TOO_SMALL;

    FIXME("EncryptMessage Unimplemented\n");

    return ret;

}

/***********************************************************************
 *             DecryptMessage
 */
SECURITY_STATUS SEC_ENTRY DecryptMessage(PCtxtHandle phContext,
        PSecBufferDesc pMessage, ULONG MessageSeqNo, PULONG pfQOP)
{
    //int token_idx, data_idx;
    SECURITY_STATUS ret = SEC_E_UNSUPPORTED_FUNCTION;

    TRACE("(%p %p %d %p)\n", phContext, pMessage, MessageSeqNo, pfQOP);

    if(!phContext)
        return SEC_E_INVALID_HANDLE;

    if(MessageSeqNo)
        FIXME("Ignoring MessageSeqNo\n");

    if(!pMessage || !pMessage->pBuffers || pMessage->cBuffers < 2)
        return SEC_E_INVALID_TOKEN;

    //if((token_idx = GetTokenBufferIndex(pMessage)) == -1)
    //    return SEC_E_INVALID_TOKEN;

    //if((data_idx = GetDataBufferIndex(pMessage)) ==-1)
    //    return SEC_E_INVALID_TOKEN;

    //if(pMessage->pBuffers[token_idx].cbBuffer < 16)
    //    return SEC_E_BUFFER_TOO_SMALL;

    FIXME("DecryptMessage Unimplemented\n");

    return ret;
}
