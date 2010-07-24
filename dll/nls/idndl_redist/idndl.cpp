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

#include <stdlib.h>
#include <string.h>

#include <unicode/uchar.h>
#include <unicode/uclean.h>
#include <unicode/umachine.h>
#include <unicode/uscript.h>

#define WIN32_LEAN_AND_MEAN
#define STRICT

#include <windows.h>

extern "C"
{
#include <idndl.h>
}

#include "scripts.h"
#include "data/idldata.cpp"

int
WINAPI
DownlevelGetLocaleScripts
(
	LPCWSTR lpLocaleName,
	LPWSTR lpScripts,
	int cchScripts
)
{
	void * lpFoundLocale = bsearch
	(
		lpLocaleName,
		IDNDL_Locales,
		//~ ARRAYSIZE(IDNDL_Locales),
		sizeof(IDNDL_Locales) / sizeof(IDNDL_Locales[0]),
		sizeof(IDNDL_Locales[0]),
		(int (*)(const void *, const void *))_stricmp
	);

	if(lpFoundLocale == NULL)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}

	const IDNDL_ScriptSet * pScriptSet = IDNDL_ScriptSets[static_cast<const wchar_t **>(lpFoundLocale) - IDNDL_Locales];

	if(pScriptSet->length > cchScripts)
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
	else
		memcpy(lpScripts, pScriptSet->scripts, pScriptSet->length * sizeof(WCHAR));

	return pScriptSet->length;
}

static int IDNDL_CompareCharRange(const void * x, const void * y)
{
	const IDNDL_CharRangeScript * pRangeX = static_cast<const IDNDL_CharRangeScript *>(x);
	const IDNDL_CharRangeScript * pRangeY = static_cast<const IDNDL_CharRangeScript *>(y);

	assert(pRangeX->lbound <= pRangeX->ubound);
	assert(pRangeY->lbound <= pRangeY->ubound);

	int cmp;

	cmp = pRangeX->ubound - pRangeY->lbound;

	if(cmp < 0)
		return cmp;

	cmp = pRangeX->lbound - pRangeY->ubound;

	if(cmp > 0)
		return cmp;

	assert((pRangeX->lbound >= pRangeY->lbound && pRangeX->ubound <= pRangeY->ubound) || (pRangeY->lbound >= pRangeX->lbound && pRangeY->ubound <= pRangeX->ubound));
	return 0;
}

extern "C" bool SCRIPTS_GetCharScriptCode(UChar32 c, int32_t * code)
{
	assert(c >= UCHAR_MIN_VALUE && c <= UCHAR_MAX_VALUE);

	IDNDL_CharRangeScript character;
	character.lbound = c;
	character.ubound = c;

	void * pRange = bsearch
	(
		&character,
		IDNDL_CharRangeScripts,
		//~ ARRAYSIZE(IDNDL_CharRangeScripts),
		sizeof(IDNDL_CharRangeScripts) / sizeof(IDNDL_CharRangeScripts[0]),
		sizeof(IDNDL_CharRangeScripts[0]),
		&IDNDL_CompareCharRange
	);

	if(pRange == NULL)
		*code = USCRIPT_UNKNOWN;
	else
		*code = static_cast<IDNDL_CharRangeScript *>(pRange)->code;

	return true;
}

extern "C" bool SCRIPTS_GetScriptCode(const SCRIPTS_Script * pScript, int32_t * code)
{
	void * ppScript = bsearch
	(
		pScript,
		IDNDL_ScriptNames,
		//~ ARRAYSIZE(IDNDL_ScriptNames),
		sizeof(IDNDL_ScriptNames) / sizeof(IDNDL_ScriptNames[0]),
		sizeof(IDNDL_ScriptNames[0]),
		(int (*)(const void *, const void *))_stricmp
	);

	bool retval;

	retval = !!ppScript;

	if(!retval)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return retval;
	}

	*code = static_cast<int32_t>(static_cast<const wchar_t **>(ppScript) - IDNDL_ScriptNames);
	return retval;
}

extern "C" void SCRIPTS_GetScriptName(int32_t code, SCRIPTS_Script * pScript)
{
	//~ assert(code >= 0 && static_cast<uint32_t>(code) < ARRAYSIZE(IDNDL_ScriptNames));
	assert(code >= 0 && static_cast<uint32_t>(code) < (sizeof(IDNDL_ScriptNames) / sizeof(IDNDL_ScriptNames[0])));
	memcpy(pScript->ScriptName, IDNDL_ScriptNames[code], sizeof(pScript->ScriptName));
}

// EOF
