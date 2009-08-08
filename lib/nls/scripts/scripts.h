/*
 * Copyright (c) 2008, KJK::Hyperion
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  - Neither the name of the ReactOS Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef REACTOS_SCRIPTS_H_
#define REACTOS_SCRIPTS_H_

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

struct SCRIPTS_Script
{
	WCHAR ScriptName[4];
	WCHAR Separator;

	static int Compare(const SCRIPTS_Script * x, const SCRIPTS_Script * y)
	{
		for(unsigned i = 0; i < 4; ++ i)
		{
			int comparison = (int)x->ScriptName[i] - (int)y->ScriptName[i];

			if(comparison)
				return comparison;
		}

		return 0;
	}

	static int Compare(const void * x, const void * y)
	{
		return Compare(static_cast<const SCRIPTS_Script *>(x), static_cast<const SCRIPTS_Script *>(y));
	}
};

C_ASSERT(sizeof(SCRIPTS_Script) == 5 * sizeof(WCHAR));

extern "C" bool SCRIPTS_GetCharScriptCode(UChar32 c, int32_t * code);
extern "C" bool SCRIPTS_GetScriptCode(const SCRIPTS_Script * pScript, int32_t * code);
extern "C" void SCRIPTS_GetScriptName(int32_t code, SCRIPTS_Script * pScript);

#endif

/* EOF */
