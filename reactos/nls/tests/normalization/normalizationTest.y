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

%{
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <unicode/uchar.h>
#include <unicode/utf.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "normalizationTest.h"

static void part1TestInvariants(const struct TestSuite * test, UChar32 from, UChar32 to)
{
	for(; from < to; ++ from)
	{
		int i = 0;
		WCHAR szInvariant[U16_MAX_LENGTH + 1];

		if(U_IS_SURROGATE(from))
			continue;

		U16_APPEND_UNSAFE(szInvariant, i, from);
		szInvariant[i] = 0;

		NORMTEST_testInvariant(test, szInvariant);
	}
}

struct parserState
{
	void * scanner;
	UChar32 lastCodepoint;
	struct TestSuite * test;
};

#define YYLTYPE int
#define YYLLOC_DEFAULT(Current, Rhs, N) for(;;) { (Current) = YYRHSLOC (Rhs, (N)); break; }
#define YY_LOCATION_PRINT(File, Loc) fprintf((File), "%d", (Loc))

#define YYLEX_PARAM state->scanner

#include "normalizationTest.tab.h"

extern int yylex(void *, int *, void *);

static void yyerror(const YYLTYPE *, struct parserState *, const char *);

%}

%pure-parser
%defines

%union {
	long part;
	long codepoint;
	WCHAR * str;
}

%token             TOKEN_COLUMN_SEPARATOR
%token             TOKEN_EOL
%token <part>      TOKEN_PART_HEADER
%token <codepoint> TOKEN_CODEPOINT

%type <str> string
%type <str> column

%destructor { free($$); } string column

%parse-param { struct parserState * state }

%initial-action {
	@$ = 1;
}

%%

testSuite : eol parts

parts :
      | parts part

part : part_header tests {
	if(state->test->part == 1)
		part1TestInvariants(state->test, state->lastCodepoint + 1, UCHAR_MAX_VALUE + 1);
}

part_header : TOKEN_PART_HEADER eol {
	state->test->part = $1;
	state->lastCodepoint = 1;
}

tests : test
      | tests test

test : column column column column column eol {
	@$ = @1;
	state->test->line = @$;

	if(state->test->part == 1)
	{
		UChar32 codepoint;
		U16_GET_UNSAFE($1, 0, codepoint);
		assert(codepoint > state->lastCodepoint);
		part1TestInvariants(state->test, state->lastCodepoint + 1, codepoint);
		state->lastCodepoint = codepoint;
	}

	NORMTEST_test(state->test, $1, $2, $3, $4, $5);

	free($1);
	free($2);
	free($3);
	free($4);
	free($5);
}

column : string TOKEN_COLUMN_SEPARATOR

string :                        { $$ = NULL; }
       | string TOKEN_CODEPOINT {
	size_t cchString = 0;

	if($1)
		cchString = wcslen($1);

	$$ = malloc((cchString + U16_LENGTH($2) + 1) * sizeof(WCHAR));

	memcpy($$, $1, cchString * sizeof(WCHAR));
	U16_APPEND_UNSAFE($$, cchString, $2);
	$$[cchString] = 0;

	free($1);
}

eol :
	| eol TOKEN_EOL

%%

void NORMTEST_runTests(struct TestSuite * test)
{
	struct parserState parser;
	parser.test = test;
	parser.scanner = NORMTEST_createScanner(test);
	yyparse(&parser);
}

static void yyerror(const YYLTYPE * loc, struct parserState * state, const char * message)
{
	struct TestSuite * test = state->test;
	test->line = *loc;
	fprintf(stderr, "FATAL: %s:%d: @Part%d: %s\n", test->filename, test->line, test->part, message);
}

/* EOF */
