/*
 * Copyright 2011 Samuel Serapion
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

void
NtlmPrintNegotiateFlags(ULONG Flags)
{
	TRACE("negotiateFlags \"0x%08lx\"{\n", Flags);

	if (Flags & NTLMSSP_NEGOTIATE_56)
		TRACE("\tNTLMSSP_NEGOTIATE_56\n");
	if (Flags & NTLMSSP_NEGOTIATE_KEY_EXCH)
		TRACE("\tNTLMSSP_NEGOTIATE_KEY_EXCH\n");
	if (Flags & NTLMSSP_NEGOTIATE_128)
		TRACE("\tNTLMSSP_NEGOTIATE_128\n");
	if (Flags & NTLMSSP_NEGOTIATE_VERSION)
		TRACE("\tNTLMSSP_NEGOTIATE_VERSION\n");
	if (Flags & NTLMSSP_NEGOTIATE_TARGET_INFO)
		TRACE("\tNTLMSSP_NEGOTIATE_TARGET_INFO\n");
	if (Flags & NTLMSSP_REQUEST_NON_NT_SESSION_KEY)
		TRACE("\tNTLMSSP_REQUEST_NON_NT_SESSION_KEY\n");
	if (Flags & NTLMSSP_REQUEST_INIT_RESP)
		TRACE("\tNTLMSSP_REQUEST_INIT_RESP\n");
	if (Flags & NTLMSSP_TARGET_TYPE_SHARE)
		TRACE("\tNTLMSSP_TARGET_TYPE_SHARE\n");
	if (Flags & NTLMSSP_TARGET_TYPE_SERVER)
		TRACE("\tNTLMSSP_TARGET_TYPE_SERVER\n");
	if (Flags & NTLMSSP_TARGET_TYPE_DOMAIN)
		TRACE("\tNTLMSSP_TARGET_TYPE_DOMAIN\n");
	if (Flags & NTLMSSP_NEGOTIATE_ALWAYS_SIGN)
		TRACE("\tNTLMSSP_NEGOTIATE_ALWAYS_SIGN\n");
	if (Flags & NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED)
		TRACE("\tNTLMSSP_NEGOTIATE_WORKSTATION_SUPPLIED\n");
	if (Flags & NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED)
		TRACE("\tNTLMSSP_NEGOTIATE_DOMAIN_SUPPLIED\n");
	if (Flags & NTLMSSP_NEGOTIATE_NTLM)
		TRACE("\tNTLMSSP_NEGOTIATE_NTLM\n");
	if (Flags & NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY)
		TRACE("\tNTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY\n");
	if (Flags & NTLMSSP_NEGOTIATE_LM_KEY)
		TRACE("\tNTLMSSP_NEGOTIATE_LM_KEY\n");
	if (Flags & NTLMSSP_NEGOTIATE_DATAGRAM)
		TRACE("\tNTLMSSP_NEGOTIATE_DATAGRAM\n");
	if (Flags & NTLMSSP_NEGOTIATE_SEAL)
		TRACE("\tNTLMSSP_NEGOTIATE_SEAL\n");
	if (Flags & NTLMSSP_NEGOTIATE_SIGN)
		TRACE("\tNTLMSSP_NEGOTIATE_SIGN\n");
	if (Flags & NTLMSSP_REQUEST_TARGET)
		TRACE("\tNTLMSSP_REQUEST_TARGET\n");
	if (Flags & NTLMSSP_NEGOTIATE_OEM)
		TRACE("\tNTLMSSP_NEGOTIATE_OEM\n");
	if (Flags & NTLMSSP_NEGOTIATE_UNICODE)
		TRACE("\tNTLMSSP_NEGOTIATE_UNICODE\n");
    if (Flags & NTLMSSP_NEGOTIATE_NT_ONLY)
		TRACE("\tNTLMSSP_NEGOTIATE_NT_ONLY\n");
	TRACE("}\n");
}

void NtlmPrintHexDump(PBYTE buffer, DWORD length)
{
    unsigned int i,count,index;
    CHAR rgbDigits[]="0123456789abcdef";
    CHAR rgbLine[100];
    char cbLine;

    for(index = 0; length;
        length -= count, buffer += count, index += count) 
    {
        count = (length > 16) ? 16:length;

        sprintf(rgbLine, "%4.4x  ", index);
        cbLine = 6;
        for(i=0;i<count;i++) 
        {
            rgbLine[cbLine++] = rgbDigits[buffer[i] >> 4];
            rgbLine[cbLine++] = rgbDigits[buffer[i] & 0x0f];
            if(i == 7)
                rgbLine[cbLine++] = ':';
            else
                rgbLine[cbLine++] = ' ';
        }
        for(; i < 16; i++) 
        {
            rgbLine[cbLine++] = ' ';
            rgbLine[cbLine++] = ' ';
            rgbLine[cbLine++] = ' ';
        }
        rgbLine[cbLine++] = ' ';

        for(i = 0; i < count; i++) 
        {
            if(buffer[i] < 32 || buffer[i] > 126) 
                rgbLine[cbLine++] = '.';
            else
                rgbLine[cbLine++] = buffer[i];
        }
        rgbLine[cbLine++] = 0;
        TRACE("%s\n", rgbLine);
    }
}

void
NtlmPrintAvPairs(const PVOID Buffer)
{
    PMSV1_0_AV_PAIR pAvPair = (PMSV1_0_AV_PAIR)Buffer;

    /* warning: the string buffers are not null terminated! */
#define AV_DESC(av_name) TRACE("%s: len: %xl value: %S\n", av_name, pAvPair->AvLen, av_value);
    do
    {
        WCHAR *av_value = (WCHAR*)((PCHAR)pAvPair + sizeof(MSV1_0_AV_PAIR));
        switch(pAvPair->AvId)
        {
        case MsvAvNbComputerName:
            AV_DESC("MsvAvNbComputerName");
            break;
        case MsvAvNbDomainName:
            AV_DESC("MsvAvNbDomainName");
            break;
        case MsvAvDnsComputerName:
            AV_DESC("MsvAvDnsComputerName");
            break;
        case MsvAvDnsDomainName:
            AV_DESC("MsvAvDnsDomainName");
            break;
        case MsvAvDnsTreeName:
            AV_DESC("MsvAvDnsTreeName");
            break;
        case MsvAvFlags:
            AV_DESC("MsvAvFlags");
            break;
        case MsvAvTimestamp:
            TRACE("MsvAvTimestamp");
            break;
        case MsvAvRestrictions:
            TRACE("MsAvRestrictions");
            break;
        case MsvAvTargetName:
            AV_DESC("MsvAvTargetName");
            break;
        case MsvAvChannelBindings:
            TRACE("MsvChannelBindings");
            break;
        }
        pAvPair = (PMSV1_0_AV_PAIR)((PUCHAR)pAvPair + pAvPair->AvLen
            + sizeof(MSV1_0_AV_PAIR));
    }while(pAvPair->AvId != MsvAvEOL);
}
