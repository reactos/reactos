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
#include "protocol.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(ntlm);

SECURITY_STATUS
NtlmGenerateNegotiateMessage(IN ULONG_PTR Context,
                             IN ULONG ContextReq,
                             IN ULONG NegotiateFlags,
                             IN PSecBuffer InputToken,
                             OUT PSecBuffer *OutputToken)
{
    PNTLMSSP_CONTEXT context = (PNTLMSSP_CONTEXT)Context;
    PNEGOTIATE_MESSAGE message;
    ULONG messageSize = 0, offset;
    NTLM_BLOB blobBuffer[2]; //nego contains 2 blobs

    TRACE("NtlmGenerateNegotiateMessage %lx flags %lx\n", Context, NegotiateFlags);

    if(!*OutputToken)
    {
        ERR("No output token!\n");
        return SEC_E_BUFFER_TOO_SMALL;
    }

    if(!((*OutputToken)->pvBuffer))
    {
        /* according to wine test */
        ERR("No output buffer!\n");
        return SEC_E_INTERNAL_ERROR;
    }

    messageSize = sizeof(NEGOTIATE_MESSAGE) +
                  NtlmOemComputerNameString.Length +
                  NtlmOemDomainNameString.Length;

    /* if should not allocate */
    if (!(ContextReq & ISC_REQ_ALLOCATE_MEMORY))
    {
        /* not enough space */
        if(messageSize > (*OutputToken)->cbBuffer)
            return SEC_E_BUFFER_TOO_SMALL;
    }
    else
    {
        /* allocate */
        (*OutputToken)->pvBuffer = NtlmAllocate(messageSize);
        (*OutputToken)->cbBuffer = messageSize;

        if(!(*OutputToken)->pvBuffer)
            return SEC_E_INSUFFICIENT_MEMORY;
    }

    /* allocate a negotiate message */
    message = (PNEGOTIATE_MESSAGE) NtlmAllocate(messageSize);

    if(!message)
        return SEC_E_INSUFFICIENT_MEMORY;

    /* build message */
    strcpy(message->Signature, NTLMSSP_SIGNATURE);
    message->MsgType = NtlmNegotiate;
    message->NegotiateFlags = context->NegotiateFlags;

    offset = PtrToUlong(message+1);

    TRACE("message %p size %lu offset1 %lu offset2 %lu\n",
        message, messageSize, offset, offset+1);

    /* generate payload */
    if(context->isLocal)
    {
        message->NegotiateFlags |= (NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED |
            NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED);

        /* blob1 */
        blobBuffer[0].Length = blobBuffer[0].MaxLength = NtlmOemDomainNameString.Length;
        blobBuffer[0].Offset = offset;
        message->OemDomainName = blobBuffer[0];

        /* copy data to the end of the message */
        memcpy((PVOID)offset, NtlmOemDomainNameString.Buffer, NtlmOemDomainNameString.Length);

        /* blob2 */
        blobBuffer[1].Length = blobBuffer[1].MaxLength = NtlmOemComputerNameString.Length;
        blobBuffer[1].Offset = offset + blobBuffer[0].Length;
        message->OemWorkstationName = blobBuffer[0];

        /* copy data to the end of the message */
        memcpy((PVOID)offset, NtlmOemComputerNameString.Buffer, NtlmOemComputerNameString.Length);
    }
    else
    {
        blobBuffer[0].Length = blobBuffer[0].MaxLength = 0;
        blobBuffer[0].Offset = offset;
        blobBuffer[1].Length =  blobBuffer[1].MaxLength = 0;
        blobBuffer[1].Offset = offset+1;
    }

    memset(&message->Version, 0, sizeof(NTLM_WINDOWS_VERSION));

    /* send it back */
    memcpy((*OutputToken)->pvBuffer, message, messageSize);
    (*OutputToken)->cbBuffer = messageSize;
    context->State = NegotiateSent;

    return SEC_I_CONTINUE_NEEDED;
}

SECURITY_STATUS
NtlmHandleNegotiateMessage(IN ULONG_PTR hCredential,
                           IN OUT PULONG_PTR Context,
                           IN ULONG ContextReq,
                           IN PSecBuffer InputToken,
                           OUT PSecBuffer *pOutputToken,
                           OUT PULONG fContextAttributes,
                           OUT PTimeStamp ptsExpiry)
{

    ERR("NtlmHandleNegotiateMessage called!\n");

    return SEC_E_UNSUPPORTED_FUNCTION;
}

