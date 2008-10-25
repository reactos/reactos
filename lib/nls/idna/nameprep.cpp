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

#define WIN32_LEAN_AND_MEAN
#define STRICT

#include <windows.h>
#include <winnls.h>

#include <unicode/usprep.h>

static
int32_t
NAMEPREP_FlagsToICU
(
	DWORD dwFlags
)
{
	C_ASSERT(IDN_ALLOW_UNASSIGNED == USPREP_ALLOW_UNASSIGNED);
	return dwFlags;
}

static
DWORD
NLS_ErrorFromICU
(
	UErrorCode ErrorCode
)
{
	// TODO
	return ERROR_GEN_FAILURE;
}

int
WINAPI
IdnToNameprepUnicode
(
	DWORD dwFlags,
	LPCWSTR lpUnicodeCharStr,
	int cchUnicodeChar,
	LPWSTR lpNameprepCharStr,
	int cchNameprepChar
)
{
	UErrorCode status;

	UStringPrepProfile * nameprep = usprep_open(NULL, "uidna", &status);

	int32_t retval = usprep_prepare
	(
		nameprep,
		lpUnicodeCharStr,
		cchUnicodeChar,
		lpNameprepCharStr,
		cchNameprepChar,
		NAMEPREP_FlagsToICU(dwFlags),
		NULL,
		&status
	);

	usprep_close(nameprep);

	if(U_FAILURE(status))
		SetLastError(NLS_ErrorFromICU(status));

	return retval;
}

// EOF
