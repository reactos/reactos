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

#include <assert.h>
#include <stdlib.h>

#include <unicode/uchar.h>
#include <unicode/utf.h>
#include <unicode/uscript.h>

#define WIN32_LEAN_AND_MEAN
#define STRICT

#include <windows.h>
#include <winnls.h>

#include "scripts.h"

struct SCRIPTS_ScriptsSet
{
private:
	uint32_t m_bits[USCRIPT_CODE_LIMIT / 32 + !!(USCRIPT_CODE_LIMIT % 32)];

public:
	SCRIPTS_ScriptsSet(): m_bits() { }

	void set(int index)
	{
		assert(index >= 0 && index < USCRIPT_CODE_LIMIT);
		m_bits[index / 32] |= 1 << (index % 32);
	}

	void reset(int index)
	{
		assert(index >= 0 && index < USCRIPT_CODE_LIMIT);
		m_bits[index / 32] &= ~(1 << (index % 32));
	}

	bool get(int index) const
	{
		assert(index >= 0 && index < USCRIPT_CODE_LIMIT);
		return ((m_bits[index / 32] >> (index % 32)) % 2) != 0;
	}

	bool processCharacter(UChar32 c)
	{
		int32_t code;
		bool retval = SCRIPTS_GetCharScriptCode(c, &code);

		if(retval)
			set(code);

		return retval;
	}

	bool processScript(const SCRIPTS_Script * pScript)
	{
		assert(pScript);

		bool retval;

		retval = pScript->Separator == L';';

		if(!retval)
		{
			SetLastError(ERROR_INVALID_PARAMETER);
			return retval;
		}

		int32_t script;
		retval = SCRIPTS_GetScriptCode(pScript, &script);

		if(retval)
			set(script);

		return retval;
	}

	bool processScripts(LPCWSTR lpScripts, int cchScripts)
	{
		bool retval;
		const SCRIPTS_Script * pScripts = reinterpret_cast<const SCRIPTS_Script *>(lpScripts);

		if(cchScripts < 0)
			cchScripts = wcslen(lpScripts);

		div_t divmod = div(cchScripts, (sizeof(SCRIPTS_Script) / sizeof(WCHAR)));
		retval = !divmod.rem;

		if(!retval)
		{
			SetLastError(ERROR_INVALID_PARAMETER);
			return retval;
		}

		for(int iScript = 0, ccScripts = divmod.quot; retval && iScript < ccScripts; ++ iScript)
			retval = processScript(pScripts + iScript);

		return retval;
	}

	bool operator<=(const SCRIPTS_ScriptsSet& Y) const
	{
		bool retval;

		//~ for(int i = 0; i < ARRAYSIZE(m_bits); ++ i)
		for(unsigned i = 0; i < (sizeof(m_bits) / sizeof(m_bits[0])); ++ i)
		{
			retval = (~m_bits[i] | Y.m_bits[i]) != 0;

			if(!retval)
				return retval;
		}

		return retval;
	}
};

int
WINAPI
GetStringScripts
(
	DWORD dwFlags,
	LPCWSTR lpString,
	int cchString,
	LPWSTR lpScripts,
	int cchScripts
)
{
	if((dwFlags | GSS_ALLOW_INHERITED_COMMON) != GSS_ALLOW_INHERITED_COMMON)
	{
		SetLastError(ERROR_INVALID_FLAGS);
		return 0;
	}

	if((lpString == NULL) != (cchString == 0) || (cchScripts > 0 && lpScripts == NULL) || cchScripts < 0)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}

	SCRIPTS_ScriptsSet StringScripts;

	if(cchString < 0)
	{
		for(int i = 0; lpString[i];)
		{
			UChar32 c = lpString[i ++];

			if(U16_IS_LEAD(c) && U16_IS_TRAIL(lpString[i]))
			{
				assert(lpString[i]);
				c = U16_GET_SUPPLEMENTARY(c, lpString[i ++]);
			}

			if(!StringScripts.processCharacter(c))
				return 0;
		}
	}
	else
	{
		for(int i = 0; i < cchString;)
		{
			UChar32 c;
			U16_NEXT(lpString, i, cchString, c);

			if(!StringScripts.processCharacter(c))
				return 0;
		}
	}

	if(!(dwFlags & GSS_ALLOW_INHERITED_COMMON))
	{
		StringScripts.reset(USCRIPT_COMMON);
		StringScripts.reset(USCRIPT_INHERITED);
	}

	if(cchScripts)
	{
		int ccScripts = 0;
		int ccScriptsCapacity = (cchScripts - 1) / (sizeof(SCRIPTS_Script) / sizeof(WCHAR));
		SCRIPTS_Script * pScripts = reinterpret_cast<SCRIPTS_Script *>(lpScripts);

		for(int iScript = 0; iScript < USCRIPT_CODE_LIMIT; ++ iScript)
		{
			if(StringScripts.get(iScript))
			{
				if(ccScripts == ccScriptsCapacity)
				{
					SetLastError(ERROR_INSUFFICIENT_BUFFER);
					return 0;
				}

				SCRIPTS_GetScriptName(iScript, &pScripts[ccScripts]);
				pScripts[ccScripts].Separator = L';';
			}
		}

		qsort(pScripts, ccScripts, sizeof(SCRIPTS_Script), &SCRIPTS_Script::Compare);

		cchScripts = ccScripts * (sizeof(SCRIPTS_Script) / sizeof(WCHAR));

		if(cchScripts)
			lpScripts[cchScripts - 1] = 0;
	}
	else
	{
		for(int iScript = 0; iScript < USCRIPT_CODE_LIMIT; ++ iScript)
			cchScripts += !!StringScripts.get(iScript);

		cchScripts *= (sizeof(SCRIPTS_Script) / sizeof(WCHAR));
		cchScripts += 1;
		assert(cchScripts);
	}

	return cchScripts;
}

BOOL
WINAPI
VerifyScripts
(
	DWORD dwFlags,
	LPCWSTR lpLocaleScripts,
	int cchLocaleScripts,
	LPCWSTR lpTestScripts,
	int cchTestScripts
)
{
	if((dwFlags | VS_ALLOW_LATIN) != VS_ALLOW_LATIN)
	{
		SetLastError(ERROR_INVALID_FLAGS);
		return FALSE;
	}

	if((lpLocaleScripts == NULL) != (cchLocaleScripts == 0) || (lpTestScripts == NULL) != (cchTestScripts == 0) )
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	SCRIPTS_ScriptsSet LocaleScripts;
	SCRIPTS_ScriptsSet TestScripts;

	if(!LocaleScripts.processScripts(lpLocaleScripts, cchLocaleScripts))
		return FALSE;

	if(!TestScripts.processScripts(lpTestScripts, cchTestScripts))
		return FALSE;

	if(dwFlags & VS_ALLOW_LATIN)
		LocaleScripts.set(USCRIPT_LATIN);

	return !!(TestScripts <= LocaleScripts);
}

// EOF
