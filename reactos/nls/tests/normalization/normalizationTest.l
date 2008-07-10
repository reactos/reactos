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
#include <stdlib.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "normalizationTest.h"

#define YY_EXTRA_TYPE struct TestSuite *
#define YY_DECL int yylex (YYSTYPE * pyylval, int * pyylloc, yyscan_t yyscanner)

#include "normalizationTest.tab.h"
%}

%option batch
%option reentrant
%option nounistd
%option nodefault

%%

"#"[^\r\n]*  /* Ignore comments */
[\t\f\v ]+   /* Ignore whitespace */

;            return TOKEN_COLUMN_SEPARATOR;

\r\n         { ++ *pyylloc; return TOKEN_EOL; }
\r           { ++ *pyylloc; return TOKEN_EOL; }
\n           { ++ *pyylloc; return TOKEN_EOL; }

@Part[0-9]+  {
	pyylval->part = strtoul(yytext + 5, NULL, 10);
	return TOKEN_PART_HEADER;
}

[0-9A-Fa-f]+ {
	pyylval->codepoint = strtoul(yytext, NULL, 16);
	return TOKEN_CODEPOINT;
}

.            {
	struct TestSuite * test = yyextra;

	test->line = *pyylloc;

	fprintf
	(
		stderr,
		"FATAL: %s:%d: @Part%d: invalid character \"%s\"\n",
		test->filename, test->line, test->part,
		yytext
	);

	yyterminate();
}

%%
void * NORMTEST_createScanner(struct TestSuite * test)
{
	yyscan_t scanner;
	(void)yyunput;
	yylex_init(&scanner);
	yyset_in(test->file, scanner);
	yyset_extra(test, scanner);
	return (void *)scanner;
}

void NORMTEST_deleteScanner(void * scanner)
{
	yylex_destroy((yyscan_t)scanner);
}

int yywrap(yyscan_t scanner) { return 1; }

/* EOF */
