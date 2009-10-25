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

#include <unicode/unorm.h>

#define WIN32_LEAN_AND_MEAN
#define STRICT

#include <windows.h>
#include <winnls.h>

static
UNormalizationMode
NORMALIZE_ModeToICU
(
	NORM_FORM NormForm
)
{
	switch(NormForm)
	{
	case NormalizationC: return UNORM_NFC;
	case NormalizationD: return UNORM_NFD;
	case NormalizationKC: return UNORM_NFKC;
	case NormalizationKD: return UNORM_NFKD;
	case NormalizationOther: break;
	}

	return UNORM_NONE;
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
NormalizeString
(
	NORM_FORM NormForm,
	LPCWSTR lpSrcString,
	int cwSrcLength,
	LPWSTR lpDstString,
	int cwDstLength
)
{
	UErrorCode ErrorCode = U_ZERO_ERROR;

	int retval = unorm_normalize
	(
		lpSrcString,
		cwSrcLength,
		NORMALIZE_ModeToICU(NormForm),
		0,
		lpDstString,
		cwDstLength,
		&ErrorCode
	);

	if(U_FAILURE(ErrorCode))
		SetLastError(NLS_ErrorFromICU(ErrorCode));

	if(cwDstLength == 0 || ErrorCode == U_BUFFER_OVERFLOW_ERROR)
		retval += 1;

	return retval;
}

BOOL
WINAPI
IsNormalizedString
(
	NORM_FORM NormForm,
	LPCWSTR lpString,
	int cwLength
)
{
	UErrorCode ErrorCode = U_ZERO_ERROR;
	BOOL retval = !!unorm_isNormalized(lpString, cwLength, NORMALIZE_ModeToICU(NormForm), &ErrorCode);

	if(U_FAILURE(ErrorCode))
		SetLastError(NLS_ErrorFromICU(ErrorCode));

	return retval;
}

// EOF
