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
	if (Flags & NTLMSSP_NEGOTIATE_IDENTIFY)
		TRACE("\tNTLMSSP_NEGOTIATE_IDENTIFY\n");
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
	if (Flags & NTLMSSP_NEGOTIATE_NTLM2)
		TRACE("\tNTLMSSP_NEGOTIATE_NTLM2\n");
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
