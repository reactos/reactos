/*
 * Copyright (C) 2004, 2006, 2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2002  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* $Id: lwconfig.c,v 1.7 2007/12/14 01:40:42 marka Exp $ */

/*
 * We do this so that we may incorporate everything in the main routines
 * so that we can take advantage of the fixes and changes made there
 * without having to add them twice. We can then call the parse routine
 * if there is a resolv.conf file and fetch our own data from the
 * Windows environment otherwise.
 */

/*
 * Note that on Win32 there is normally no resolv.conf since all information
 * is stored in the registry. Therefore there is no ordering like the
 * contents of resolv.conf. Since the "search" or "domain" keyword, on
 * Win32 if a search list is found it is used, otherwise the domain name
 * is used since they are mutually exclusive. The search list can be entered
 * in the DNS tab of the "Advanced TCP/IP settings" window under the same place
 * that you add your nameserver list.
 */

#define lwres_conf_parse generic_lwres_conf_parse
#include "../lwconfig.c"
#undef lwres_conf_parse

#include <iphlpapi.h>

#define TCPIP_SUBKEY	\
	"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"

void
get_win32_searchlist(lwres_context_t *ctx) {
	HKEY hKey;
	BOOL keyFound = TRUE;
	char searchlist[MAX_PATH];
	DWORD searchlen = MAX_PATH;
	char *cp;
	lwres_conf_t *confdata;

	REQUIRE(ctx != NULL);
	confdata = &ctx->confdata;

	memset(searchlist, 0, MAX_PATH);
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TCPIP_SUBKEY, 0, KEY_READ, &hKey)
		!= ERROR_SUCCESS)
		keyFound = FALSE;
	
	if (keyFound == TRUE) {
		/* Get the named directory */
		if (RegQueryValueEx(hKey, "SearchList", NULL, NULL,
			(LPBYTE)searchlist, &searchlen) != ERROR_SUCCESS)
			keyFound = FALSE;
		RegCloseKey(hKey);
	}

	confdata->searchnxt = 0;
	cp = strtok((char *)searchlist, ", \0");
	while (cp != NULL) {
		if (confdata->searchnxt == LWRES_CONFMAXSEARCH)
			break;
		if (strlen(cp) <= MAX_PATH && strlen(cp) > 0) {
			confdata->search[confdata->searchnxt] = lwres_strdup(ctx, cp);
			if (confdata->search[confdata->searchnxt] != NULL)
				confdata->searchnxt++;
		}
		cp = strtok(NULL, ", \0");
	}
}

lwres_result_t
lwres_conf_parse(lwres_context_t *ctx, const char *filename) {
	lwres_result_t ret = LWRES_R_SUCCESS;
	lwres_result_t res;
	lwres_conf_t *confdata;
	FIXED_INFO * FixedInfo;
	ULONG    BufLen = sizeof(FIXED_INFO);
	DWORD    dwRetVal;
	IP_ADDR_STRING *pIPAddr;

	REQUIRE(ctx != NULL);
	confdata = &ctx->confdata;
	REQUIRE(confdata != NULL);

	/* Use the resolver if there is one */
	ret = generic_lwres_conf_parse(ctx, filename);
	if ((ret != LWRES_R_NOTFOUND && ret != LWRES_R_SUCCESS) ||
		(ret == LWRES_R_SUCCESS && confdata->nsnext > 0))
		return (ret);

	/*
	 * We didn't get any nameservers so we need to do this ourselves
	 */
	FixedInfo = (FIXED_INFO *) GlobalAlloc(GPTR, BufLen);
	dwRetVal = GetNetworkParams(FixedInfo, &BufLen);
	if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
		GlobalFree(FixedInfo);
		FixedInfo = GlobalAlloc(GPTR, BufLen);
		dwRetVal = GetNetworkParams(FixedInfo, &BufLen);
	}
	if (dwRetVal != ERROR_SUCCESS) {
		GlobalFree(FixedInfo);
		return (LWRES_R_FAILURE);
	}

	/* Get the search list from the registry */
	get_win32_searchlist(ctx);

	/* Use only if there is no search list */
	if (confdata->searchnxt == 0 && strlen(FixedInfo->DomainName) > 0) {
		confdata->domainname = lwres_strdup(ctx, FixedInfo->DomainName);
		if (confdata->domainname == NULL) {
			GlobalFree(FixedInfo);
			return (LWRES_R_FAILURE);
		}
	} else
		confdata->domainname = NULL;

	/* Get the list of nameservers */
	pIPAddr = &FixedInfo->DnsServerList;
	while (pIPAddr) {
		if (confdata->nsnext >= LWRES_CONFMAXNAMESERVERS)
			break;

		res = lwres_create_addr(pIPAddr->IpAddress.String,
				&confdata->nameservers[confdata->nsnext++], 1);
		if (res != LWRES_R_SUCCESS) {
			GlobalFree(FixedInfo);
			return (res);
		}
		pIPAddr = pIPAddr ->Next;
	}

	GlobalFree(FixedInfo);
	return (LWRES_R_SUCCESS);
}
