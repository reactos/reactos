/*
 * Unit tests for SHA functions
 *
 * Copyright (c) 2004 Filip Navara
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"

#include "wine/test.h"

typedef struct {
   ULONG Unknown[6];
   ULONG State[5];
   ULONG Count[2];
   UCHAR Buffer[64];
} SHA_CTX, *PSHA_CTX;

#define ctxcmp(a,b) memcmp((char*)a, (char*)b, FIELD_OFFSET(SHA_CTX, Buffer))

static void test_sha_ctx(void)
{
   FARPROC pA_SHAInit, pA_SHAUpdate, pA_SHAFinal;
   static const char test_buffer[] = "In our Life there's If"
                       "In our beliefs there's Lie"
                       "In our business there is Sin"
                       "In our bodies, there is Die";
   ULONG test_buffer_size = strlen(test_buffer);
   HMODULE hmod;
   SHA_CTX ctx;
   SHA_CTX ctx_initialized = {{0, 0, 0, 0, 0}, {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0}, {0, 0}};
   SHA_CTX ctx_update1 = {{0, 0, 0, 0, 0}, {0xdbe5eba8, 0x6b4335ca, 0xf7c94abe, 0xc9f34e31, 0x311023f0}, {0, 0x67}};
   SHA_CTX ctx_update2 = {{0, 0, 0, 0, 0}, {0x5ecc818d, 0x52498169, 0xf6758559, 0xd035a164, 0x871dd125}, {0, 0xce}};
   ULONG result[5];
   ULONG result_correct[5] = {0xe014f93, 0xe09791ec, 0x6dcf96c8, 0x8e9385fc, 0x1611c1bb};

   hmod = LoadLibrary("advapi32.dll");
   pA_SHAInit = GetProcAddress(hmod, "A_SHAInit");
   pA_SHAUpdate = GetProcAddress(hmod, "A_SHAUpdate");
   pA_SHAFinal = GetProcAddress(hmod, "A_SHAFinal");

   if (!pA_SHAInit || !pA_SHAUpdate || !pA_SHAFinal) return;

   RtlZeroMemory(&ctx, sizeof(ctx));
   pA_SHAInit(&ctx);
   ok(!ctxcmp(&ctx, &ctx_initialized), "invalid initialization\n");

   pA_SHAUpdate(&ctx, test_buffer, test_buffer_size);
   ok(!ctxcmp(&ctx, &ctx_update1), "update doesn't work correctly\n");

   pA_SHAUpdate(&ctx, test_buffer, test_buffer_size);
   ok(!ctxcmp(&ctx, &ctx_update2), "update doesn't work correctly\n");

   pA_SHAFinal(&ctx, result);
   ok(!ctxcmp(&ctx, &ctx_initialized), "context hasn't been reinitialized\n");
   ok(!memcmp(result, result_correct, sizeof(result)), "incorrect result\n");

   FreeLibrary(hmod);
}

START_TEST(crypt_sha)
{
    test_sha_ctx();
}
