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
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include <unicode/uchar.h>
#include <unicode/utf.h>

#define WINVER 0x0600
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "normalizationTest.h"

static int (WINAPI * pfnNormalizeString)(NORM_FORM, LPCWSTR, int, LPWSTR, int);

char * FormatString(const struct TestSuite * test, const WCHAR * str)
{
	int length;
	int codepoints = 0;
	int formattedLength = 0;
	char * formattedPos;
	int formattedRemaining;
	char * formattedString;
	int i;

	if(str == NULL)
		return NULL;

	length = wcslen(str);

	for(i = 0; i < length; )
	{
		UChar32 c;
		U16_NEXT(str, i, length, c);

		++ codepoints;

		assert(c <= 0x10ffff);

		if(c < 0x10000)
			formattedLength += sizeof("U+ffff") - 1;
		else if(c < 0x100000)
			formattedLength += sizeof("U+fffff") - 1;
		else
			formattedLength += sizeof("U+10ffff") - 1;
	}

	if(codepoints)
		formattedLength += (sizeof(" ") - 1) * (codepoints - 1);

	formattedLength += sizeof("");

	formattedString = malloc(formattedLength);

	if(formattedString == NULL)
	{
		fprintf(stderr, "ERROR: %s:%d: @Part%d: out of memory\n", test->filename, test->line, test->part);
		return NULL;
	}

	formattedPos = formattedString;
	formattedRemaining = formattedLength;

	for(i = 0; i < length; )
	{
		int unitLength;
		UChar32 c;

		U16_NEXT(str, i, length, c);

		assert(formattedRemaining == (formattedLength - (formattedPos - formattedString)));

		if(c < 0x10000)
		{
			unitLength = _snprintf(formattedPos, formattedRemaining, "U+%04x", c);
			assert(unitLength == (sizeof("U+ffff") - 1));
		}
		else if(c < 0x100000)
		{
			unitLength = _snprintf(formattedPos, formattedRemaining, "U+%05x", c);
			assert(unitLength == (sizeof("U+fffff") - 1));
		}
		else
		{
			assert(c <= 0x10ffff);
			unitLength = _snprintf(formattedPos, formattedRemaining, "U+%06x", c);
			assert(unitLength == (sizeof("U+10ffff") - 1));
		}

		assert(unitLength >= 0);

		if(unitLength < 0)
		{
			fprintf
			(
				stderr,
				"ERROR: %s:%d: @Part%d: couldn't format codepoint %x\n",
				test->filename, test->line, test->part,
				c
			);

			free(formattedString);
			return NULL;
		}

		formattedPos += unitLength;
		formattedRemaining -= unitLength;

		assert(formattedRemaining >= 0);

		if(i < length)
		{
			*formattedPos = ' ';
			formattedPos += sizeof(" ") - 1;
			formattedRemaining -= sizeof(" ") - 1;
			assert(formattedRemaining > 0);
		}
	}

	formattedString[formattedLength - 1] = 0;
	return formattedString;
}

static WCHAR * Normalize(const struct TestSuite * test, const WCHAR * str, NORM_FORM NormForm)
{
	LPWSTR lpDstString = NULL;
	int cwDstLength = 0;
	int cwRet;

	for(;;)
	{
		if(cwDstLength)
		{
			lpDstString = malloc(cwDstLength * sizeof(WCHAR));

			if(lpDstString == NULL)
			{
				fprintf(stderr, "ERROR: %s:%d: @Part%d: out of memory\n", test->filename, test->line, test->part);
				return NULL;
			}
		}

		cwRet = pfnNormalizeString(NormForm, str, -1, lpDstString, cwDstLength);

		if(cwRet > 0)
		{
			if(cwDstLength)
				break;
			else
			{
				cwDstLength = cwRet;
				continue;
			}
		}

		free(lpDstString);
		lpDstString = NULL;

		switch(GetLastError())
		{
		case ERROR_SUCCESS:
			/* Should never happen (length of the buffer must be at least 1) */
			assert(0);
			return NULL;

		case ERROR_INSUFFICIENT_BUFFER:
			break;

		case ERROR_NO_UNICODE_TRANSLATION:
			{
				int i;
				int invalid;
				WCHAR invalidChar[U16_MAX_LENGTH + 1];
				char * invalidCharFormatted;
				UChar32 c;
				size_t strLen;

				strLen = wcslen(str);
				invalid = -cwRet;

				if(str[invalid] == 0 && invalid > 0 && U_IS_SURROGATE(str[invalid - 1]))
					-- invalid;
				else if(U_IS_TRAIL(str[invalid]) && invalid > 0 && U_IS_LEAD(str[invalid - 1]))
					-- invalid;

				i = invalid;
				U16_NEXT(str, i, strLen, c);

				i = 0;
				U16_APPEND_UNSAFE(invalidChar, i, c);
				invalidChar[i] = 0;

				invalidCharFormatted = FormatString(test, invalidChar);

				fprintf
				(
					stderr,
					"ERROR: %s:%d: @Part%d: invalid Unicode character <%s>\n",
					test->filename, test->line, test->part,
					invalidCharFormatted
				);

				free(invalidCharFormatted);
			}

			return NULL;

		default:
			fprintf
			(
				stderr,
				"ERROR: %s:%d: @Part%d: couldn't normalize: system error %lu\n",
				test->filename, test->line, test->part,
				GetLastError()
			);

			return NULL;
		}

		cwDstLength = -cwRet;
	}

	assert(lpDstString);
	return lpDstString;
}

static void Test
(
	const struct TestSuite * test,
	NORM_FORM NormForm,
	const WCHAR * data,
	const WCHAR * expected
)
{
	const char * normForm = "???";
	WCHAR * actual = NULL;
	char * dataFormatted = NULL;
	char * expectedFormatted = NULL;
	char * actualFormatted = NULL;

	assert(data);
	assert(expected);

	switch(NormForm)
	{
		case NormalizationC: normForm = "NFC"; break;
		case NormalizationD: normForm = "NFD"; break;
		case NormalizationKC: normForm = "NFKC"; break;
		case NormalizationKD: normForm = "NFKD"; break;
		default: assert(0); break;
	}

	dataFormatted = FormatString(test, data);
	expectedFormatted = FormatString(test, expected);

	actual = Normalize(test, data, NormForm);

	if(!expected || !actual)
	{
		fprintf
		(
			stderr,
			"SKIP: %s:%d @Part%d: %s(<%s>): earlier errors\n",
			test->filename, test->line, test->part,
			normForm,
			dataFormatted
		);
	}
	else
	{
		actualFormatted = FormatString(test, actual);

		if(wcscmp(expected, actual))
		{
			fprintf
			(
				stderr,
				"FAIL: %s:%d: @Part%d: %s(<%s>): expected <%s>, got <%s>\n",
				test->filename, test->line, test->part,
				normForm,
				dataFormatted, expectedFormatted, actualFormatted
			);
		}
		else
		{
			/*fprintf
			(
				stdout,
				"PASS: %s:%d @Part%d: %s(<%s>): <%s>\n",
				test->filename, test->line, test->part,
				normForm,
				dataFormatted, expectedFormatted
			);*/
		}
	}

	free(dataFormatted);
	free(expectedFormatted);
	free(actualFormatted);
	free(actual);
}

void NORMTEST_testInvariant(const struct TestSuite * test, const WCHAR * c)
{
	Test(test, NormalizationC, c, c);
	Test(test, NormalizationD, c, c);
	Test(test, NormalizationKC, c, c);
	Test(test, NormalizationKD, c, c);
}

void NORMTEST_test(const struct TestSuite * test, const WCHAR * c1, const WCHAR * c2, const WCHAR * c3, const WCHAR * c4, const WCHAR * c5)
{
	Test(test, NormalizationC, c1, c2);
	Test(test, NormalizationC, c2, c2);
	Test(test, NormalizationC, c3, c2);
	Test(test, NormalizationC, c4, c4);
	Test(test, NormalizationC, c5, c4);
	Test(test, NormalizationD, c1, c3);
	Test(test, NormalizationD, c2, c3);
	Test(test, NormalizationD, c3, c3);
	Test(test, NormalizationD, c4, c5);
	Test(test, NormalizationD, c5, c5);
	Test(test, NormalizationKC, c1, c4);
	Test(test, NormalizationKC, c2, c4);
	Test(test, NormalizationKC, c3, c4);
	Test(test, NormalizationKC, c4, c4);
	Test(test, NormalizationKC, c5, c4);
	Test(test, NormalizationKD, c1, c5);
	Test(test, NormalizationKD, c2, c5);
	Test(test, NormalizationKD, c3, c5);
	Test(test, NormalizationKD, c4, c5);
	Test(test, NormalizationKD, c5, c5);
}

int main(int argc, const char * const * argv)
{
	struct TestSuite testSuite;
	HMODULE hmNormaliz;

	hmNormaliz = LoadLibrary(TEXT("normaliz")); /* TODO: error handling */
	pfnNormalizeString = (void *)GetProcAddress(hmNormaliz, "NormalizeString");

	testSuite.filename = "NormalizationTest.txt";
	testSuite.part = -1;

	if(argc > 1)
		testSuite.filename = argv[1];

	testSuite.file = fopen(testSuite.filename, "rb"); /* TODO: error handling */

	NORMTEST_runTests(&testSuite);

	fclose(testSuite.file);

	return 0;
}

/* EOF */
