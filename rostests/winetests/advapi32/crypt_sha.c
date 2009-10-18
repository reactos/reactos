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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
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

static void test_sha_ctx(void)
{
    void (WINAPI *pA_SHAInit)(PSHA_CTX);
    void (WINAPI *pA_SHAUpdate)(PSHA_CTX, const unsigned char *, UINT);
    void (WINAPI *pA_SHAFinal)(PSHA_CTX, PULONG);
    static const unsigned char test_buffer[] = "In our Life there's If"
                                               "In our beliefs there's Lie"
                                               "In our business there is Sin"
                                               "In our bodies, there is Die";
   HMODULE hmod;
   SHA_CTX ctx;
   ULONG result[5];
   ULONG result_correct[5] = {0xe014f93, 0xe09791ec, 0x6dcf96c8, 0x8e9385fc, 0x1611c1bb};

   hmod = GetModuleHandleA("advapi32.dll");
   pA_SHAInit = (void *)GetProcAddress(hmod, "A_SHAInit");
   pA_SHAUpdate = (void *)GetProcAddress(hmod, "A_SHAUpdate");
   pA_SHAFinal = (void *)GetProcAddress(hmod, "A_SHAFinal");

   if (!pA_SHAInit || !pA_SHAUpdate || !pA_SHAFinal)
   {
      win_skip("A_SHAInit and/or A_SHAUpdate and/or A_SHAFinal are not available\n");
      return;
   }

   RtlZeroMemory(&ctx, sizeof(ctx));
   pA_SHAInit(&ctx);
   pA_SHAUpdate(&ctx, test_buffer, sizeof(test_buffer)-1);
   pA_SHAUpdate(&ctx, test_buffer, sizeof(test_buffer)-1);
   pA_SHAFinal(&ctx, result);
   ok(!memcmp(result, result_correct, sizeof(result)), "incorrect result\n");
}

START_TEST(crypt_sha)
{
    test_sha_ctx();
}
