/*
 * Unit tests for rsabase functions
 *
 * Copyright (c) 2004 Michael Jung
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

#include <string.h>
#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wincrypt.h"

HCRYPTPROV hProv;
static const char szContainer[] = "Wine Test Container";
static const char szProvider[] = MS_DEF_PROV_A;

static int init_environment(void)
{
	hProv = (HCRYPTPROV)INVALID_HANDLE_VALUE;
	
	if (!CryptAcquireContext(&hProv, szContainer, szProvider, PROV_RSA_FULL, 0))
	{
		if (GetLastError()==NTE_BAD_KEYSET)
		{
			if(!CryptAcquireContext(&hProv, szContainer, szProvider, PROV_RSA_FULL, CRYPT_NEWKEYSET)) 
			{
				trace("%08x\n", GetLastError());
				return 0;
			}
		}
		else
		{
			trace("%08x\n", GetLastError());
			return 0;
		}
	}

	return 1;
}

static void clean_up_environment(void)
{
	CryptAcquireContext(&hProv, szContainer, szProvider, PROV_RSA_FULL, CRYPT_DELETEKEYSET);
}

static void test_gen_random(void)
{
	BOOL result;
	BYTE rnd1[16], rnd2[16];

	memset(rnd1, 0, sizeof(rnd1));
	memset(rnd2, 0, sizeof(rnd2));
	
	result = CryptGenRandom(hProv, sizeof(rnd1), rnd1);
	ok(result, "%08x\n", GetLastError());

	result = CryptGenRandom(hProv, sizeof(rnd2), rnd2);
	ok(result, "%08x\n", GetLastError());

	ok(memcmp(rnd1, rnd2, sizeof(rnd1)), "CryptGenRandom generates non random data\n");
}

START_TEST(rsabase)
{
	if (!init_environment()) 
		return;
	test_gen_random();
	clean_up_environment();
}
