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

#include <unicode/putil.h>
#include <unicode/uchar.h>
#include <unicode/uloc.h>
#include <unicode/ures.h>
#include <unicode/uscript.h>

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <functional>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

struct script_name
{
private:
	void normalize()
	{
		chars[0] = toupper(chars[0]);
		chars[1] = tolower(chars[1]);
		chars[2] = tolower(chars[2]);
		chars[3] = tolower(chars[3]);
	}

public:
	char chars[4];

	bool operator<(const script_name& Y) const { return strncmp(chars, Y.chars, 4) < 0; }
	bool operator>(const script_name& Y) const { return strncmp(chars, Y.chars, 4) > 0; }
	bool operator==(const script_name& Y) const { return strncmp(chars, Y.chars, 4) == 0; }
	bool operator!=(const script_name& Y) const { return strncmp(chars, Y.chars, 4) != 0; }
	bool operator<=(const script_name& Y) const { return strncmp(chars, Y.chars, 4) <= 0; }
	bool operator>=(const script_name& Y) const { return strncmp(chars, Y.chars, 4) >= 0; }

	script_name(): chars() { }
	script_name(const script_name& Y) { memcpy(chars, Y.chars, sizeof(chars)); }

	const script_name& operator=(const script_name& Y)
	{
		memcpy(chars, Y.chars, sizeof(chars));
		return *this;
	}

	explicit script_name(const char * pChars)
	{
		assert(pChars);
		assert(strlen(pChars) == 4);
		chars[0] = pChars[0];
		chars[1] = pChars[1];
		chars[2] = pChars[2];
		chars[3] = pChars[3];
		normalize();
	}

	explicit script_name(const UChar * pChars)
	{
		assert(pChars);
		assert(u_strlen(pChars) == 4);
		u_UCharsToChars(pChars, chars, 4);
		normalize();
	}
};

struct lessLocaleId: public std::binary_function<std::string, std::string, bool>
{
	result_type operator()(const first_argument_type& x, const second_argument_type& y) const
	{
		return stricmp(x.c_str(), y.c_str()) < 0;
	}
};

std::string convertLocale(const char * locale)
{
	std::string s(locale); // FIXME!!!
	return s;
}

bool validId(const std::string& id)
{
	std::string::const_iterator p = id.begin();

	if(p == id.end() || !u_isIDStart(*p))
		return false;

	++ p;

	for(; p != id.end(); ++ p)
		if(!u_isIDPart(*p))
			return false;

	return true;
}

std::string getLocaleLiteral(const std::string& locale)
{
	std::string lit;
	lit += '"';
	lit += locale; // FIXME!!! escapes
	lit += '"';
	return lit;
}

std::string getScriptLiteral(const script_name& s)
{
	std::string lit;
	lit += '"';
	lit += s.chars[0];
	lit += s.chars[1];
	lit += s.chars[2];
	lit += s.chars[3];
	lit += '"';
	return lit;
}

std::string getScriptId(const script_name& s)
{
	std::string id("IDNDL_Script_");
	id += s.chars[0];
	id += s.chars[1];
	id += s.chars[2];
	id += s.chars[3];
	assert(validId(id));
	return id;
}

std::string getScriptSetId(const std::set<script_name>& s)
{
	std::string id("IDNDL_ScriptSet_");

	for(std::set<script_name>::const_iterator p = s.begin(); p != s.end(); ++ p)
	{
		id += p->chars[0];
		id += p->chars[1];
		id += p->chars[2];
		id += p->chars[3];
	}

	assert(validId(id));
	return id;
}

int main()
{
	UErrorCode status = U_ZERO_ERROR;

	/* Locale -> scripts table */
	int32_t localeCount = uloc_countAvailable();

	typedef std::map<std::string, std::set<script_name>, lessLocaleId> LocalesScripts;
	LocalesScripts localesScripts;

	for(int32_t i = 0; i < localeCount; ++ i)
	{
		const char * locale = uloc_getAvailable(i);
		UResourceBundle * localeRes = ures_open(NULL, locale, &status);

		if(U_SUCCESS(status))
		{
			UErrorCode localStatus = U_ZERO_ERROR;
			UResourceBundle * scriptsRes = ures_getByKey(localeRes, "LocaleScript", NULL, &status);

			if(U_SUCCESS(status))
			{
				std::set<script_name> localeScripts;

				while(ures_hasNext(scriptsRes))
				{
					int32_t scriptLen = 0;
					const UChar * script = ures_getNextString(scriptsRes, &scriptLen, NULL, &localStatus);

					if(U_SUCCESS(localStatus))
						localeScripts.insert(script_name(script));
					else
					{
						fprintf(stderr, "warning: failed reading scripts for locale %s: %s\n", locale, u_errorName(localStatus));
						break;
					}
				}

				if(localeScripts.size())
					localesScripts[convertLocale(locale)].insert(localeScripts.begin(), localeScripts.end());

				ures_close(scriptsRes);
			}
			else
				fprintf(stderr, "warning: failed reading scripts for locale %s: %s\n", locale, u_errorName(localStatus));

			ures_close(localeRes);
		}
		else
			break;
	}

	if(!U_SUCCESS(status))
	{
		fprintf(stderr, "error: failed enumerating locale scripts: %s\n", u_errorName(status));
		return 1;
	}

	typedef std::set<std::set<script_name> > UniqueScriptSets;
	UniqueScriptSets uniqueScriptSets;

	for(LocalesScripts::const_iterator p = localesScripts.begin(); p != localesScripts.end(); ++ p)
		uniqueScriptSets.insert(p->second);

	typedef std::map<std::string, UniqueScriptSets::const_iterator> LocalesScriptsFolded;
	LocalesScriptsFolded localesScriptsFolded;

	for(LocalesScripts::const_iterator p = localesScripts.begin(); p != localesScripts.end(); ++ p)
		localesScriptsFolded.insert(std::make_pair(p->first, uniqueScriptSets.find(p->second)));

	// Unique script sets
	printf("struct %s { wchar_t const * scripts; int length; };\n", "IDNDL_ScriptSet");

	for(UniqueScriptSets::const_iterator p = uniqueScriptSets.begin(); p != uniqueScriptSets.end(); ++ p)
	{
		printf("static const %s %s = {", "IDNDL_ScriptSet", getScriptSetId(*p).c_str());

		for(std::set<script_name>::const_iterator pScript = p->begin(); pScript != p->end(); ++ pScript)
			printf(" L%s L\";\"", getScriptLiteral(*pScript).c_str());

		printf(", %d };\n", static_cast<int>(p->size() * (4 + 1) + 1));
	}

	// Sorted table of locale ids
	printf("static wchar_t const * const %s [] = {\n", "IDNDL_Locales");

	for(LocalesScriptsFolded::const_iterator p = localesScriptsFolded.begin(); p != localesScriptsFolded.end(); ++ p)
		printf("L%s,\n", getLocaleLiteral(p->first).c_str());

	printf("};\n");

	// Locale id index -> script set
	printf("static %s const * const %s [] = {\n", "IDNDL_ScriptSet", "IDNDL_ScriptSets");

	for(LocalesScriptsFolded::const_iterator p = localesScriptsFolded.begin(); p != localesScriptsFolded.end(); ++ p)
		printf("&%s,\n", getScriptSetId(*p->second).c_str());

	printf("};\n");

	/* Codepoint -> script table */
	// Script code -> script name table
	printf("static const wchar_t * const %s[] = {\n", "IDNDL_ScriptNames");

	for(int script = 0; script < USCRIPT_CODE_LIMIT; ++ script)
		printf("L%s,\n", getScriptLiteral(script_name(uscript_getShortName(static_cast<UScriptCode>(script)))).c_str());

	printf("};\n");

	// Codepoint range -> script code
	printf("struct %s { int lbound; int ubound; int code; };\n", "IDNDL_CharRangeScript");
	printf("static const %s %s[] = {\n", "IDNDL_CharRangeScript", "IDNDL_CharRangeScripts");

	int lbound = UCHAR_MIN_VALUE;
	UScriptCode lastScript = uscript_getScript(UCHAR_MIN_VALUE, &status);

	if(!U_SUCCESS(status) || lastScript < 0)
		lastScript = USCRIPT_UNKNOWN;

	for(UChar32 c = UCHAR_MIN_VALUE + 1; c <= UCHAR_MAX_VALUE; ++ c)
	{
		UScriptCode script = uscript_getScript(c, &status);

		if(!U_SUCCESS(status) || script < 0)
			script = USCRIPT_UNKNOWN;

		assert(script >= 0 && script < USCRIPT_CODE_LIMIT);

		if(script != lastScript)
		{
			if(lastScript != USCRIPT_UNKNOWN)
				printf("{ %d, %d, %d },\n", lbound, c - 1, static_cast<int>(lastScript));

			lbound = c;
			lastScript = script;
		}
	}

	if(lastScript != USCRIPT_UNKNOWN)
		printf("{ %d, %d, %d },\n", lbound, UCHAR_MAX_VALUE, static_cast<int>(lastScript));

	printf("};\n");
}

// EOF
