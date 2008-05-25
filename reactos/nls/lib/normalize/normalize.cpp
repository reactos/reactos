#define WIN32_LEAN_AND_MEAN
#define STRICT

#define WINVER 0x0600

#include <windows.h>
#include <winnls.h>

#include <unicode/unorm.h>

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
