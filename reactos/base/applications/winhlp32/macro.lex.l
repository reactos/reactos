%{ /* -*-C-*- */
/*
 * Help Viewer
 *
 * Copyright 1996 Ulrich Schmid
 * Copyright 2002,2008 Eric Pouech
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
%}
%option noinput nounput interactive 8bit
%x quote
%{
#include "config.h"
#include <assert.h>

#ifndef HAVE_UNISTD_H
#define YY_NO_UNISTD_H
#endif

#include "macro.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winhelp);

struct lex_data {
    LPCSTR   macroptr;
    LPSTR    strptr;
    int      quote_stack[32];
    unsigned quote_stk_idx;
    LPSTR    cache_string[32];
    int      cache_used;
};
static struct lex_data* lex_data = NULL;

struct lexret  yylval;

#define YY_INPUT(buf,result,max_size)\
  if ((result = *lex_data->macroptr ? 1 : 0)) buf[0] = *lex_data->macroptr++;

%}
%%

[-+]?[0-9]+             yylval.integer = strtol(yytext, NULL, 10);	return INTEGER;
[-+]?0[xX][0-9a-f]+	yylval.integer = strtol(yytext, NULL, 16);	return INTEGER;

[a-zA-Z][_0-9a-zA-Z]*   return MACRO_Lookup(yytext, &yylval);

\`	    |
\"	    |
\'          |
<quote>\`   |
<quote>\"   |
<quote>\'   {
    if (lex_data->quote_stk_idx == 0 ||
        (yytext[0] == '\"' && lex_data->quote_stack[lex_data->quote_stk_idx - 1] != '\"') ||
        (yytext[0] == '`'))
    {
        /* opening a new one */
        if (lex_data->quote_stk_idx == 0)
        {
            assert(lex_data->cache_used < sizeof(lex_data->cache_string) / sizeof(lex_data->cache_string[0]));
            lex_data->strptr = lex_data->cache_string[lex_data->cache_used] = HeapAlloc(GetProcessHeap(), 0, strlen(lex_data->macroptr) + 1);
            yylval.string = lex_data->strptr;
            lex_data->cache_used++;
            BEGIN(quote);
        }
        else *lex_data->strptr++ = yytext[0];
        lex_data->quote_stack[lex_data->quote_stk_idx++] = yytext[0];
        assert(lex_data->quote_stk_idx < sizeof(lex_data->quote_stack) / sizeof(lex_data->quote_stack[0]));
    }
    else
    {
        if (yytext[0] == '`') assert(0);
        /* close the current quote */
        if (--lex_data->quote_stk_idx == 0)
        {
            BEGIN INITIAL;
            *lex_data->strptr++ = '\0';
            return STRING;
        }
        else *lex_data->strptr++ = yytext[0];
    }
}

<quote>.                *lex_data->strptr++ = yytext[0];
<quote>\\.	        *lex_data->strptr++ = yytext[1];
<quote><<EOF>>	        return 0;

" "
.			return yytext[0];
%%

#if 0
/* all code for testing macros */
#include "winhelp.h"
static CHAR szTestMacro[256];

static LRESULT CALLBACK MACRO_TestDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_COMMAND && wParam == IDOK)
    {
        GetDlgItemText(hDlg, 99, szTestMacro, sizeof(szTestMacro));
        EndDialog(hDlg, IDOK);
        return TRUE;
    }
    return FALSE;
}

void macro_test(void)
{
    WNDPROC lpfnDlg = MakeProcInstance(MACRO_TestDialogProc, Globals.hInstance);
    DialogBox(Globals.hInstance, STRING_DIALOG_TEST, Globals.active_win->hMainWnd, (DLGPROC)lpfnDlg);
    FreeProcInstance(lpfnDlg);
    macro = szTestMacro;
}
#endif

/* small helper function for debug messages */
static const char* ts(int t)
{
    static char c[2] = {0,0};

    switch (t)
    {
    case EMPTY: return "EMPTY";
    case VOID_FUNCTION: return "VOID_FUNCTION";
    case BOOL_FUNCTION: return "BOOL_FUNCTION";
    case INTEGER: return "INTEGER";
    case STRING: return "STRING";
    case IDENTIFIER: return "IDENTIFIER";
    default: c[0] = (char)t; return c;
    }
}

static int MACRO_CallBoolFunc(FARPROC fn, const char* args, void** ret);

/******************************************************************
 *		MACRO_CheckArgs
 *
 * checks number of arguments against prototype, and stores arguments on
 * stack pa for later call
 * returns -1 on error, otherwise the number of pushed parameters
 */
static int MACRO_CheckArgs(void* pa[], unsigned max, const char* args)
{
    int t;
    unsigned int len = 0, idx = 0;

    WINE_TRACE("Checking %s\n", args);

    if (yylex() != '(') {WINE_WARN("missing (\n");return -1;}

    if (*args)
    {
        len = strlen(args);
        for (;;)
        {
            t = yylex();
            WINE_TRACE("Got %s <=> %c\n", ts(t), *args);

            switch (*args)
            {
            case 'S': 
                if (t != STRING)
                {WINE_WARN("missing S\n");return -1;}
                pa[idx] = (void*)yylval.string;  
                break;
            case 'U':
            case 'I':
                if (t != INTEGER)
                {WINE_WARN("missing U\n");return -1;}   
                pa[idx] = LongToPtr(yylval.integer);
                break;
            case 'B':
                if (t != BOOL_FUNCTION) 
                {WINE_WARN("missing B\n");return -1;}   
                if (MACRO_CallBoolFunc(yylval.function, yylval.proto, &pa[idx]) == 0)
                    return -1;
                break;
            default: 
                WINE_WARN("unexpected %s while args is %c\n", ts(t), *args);
                return -1;
            }
            idx++;
            if (*++args == '\0') break;
            t = yylex();
            if (t == ')') goto CheckArgs_end;
            if (t != ',') {WINE_WARN("missing ,\n");return -1;}
            if (idx >= max) {WINE_FIXME("stack overflow (%d)\n", max);return -1;}
        }
    }
    if (yylex() != ')') {WINE_WARN("missing )\n");return -1;}

CheckArgs_end:
    while (len > idx) pa[--len] = NULL;
    return idx;
}

/******************************************************************
 *		MACRO_CallBoolFunc
 *
 * Invokes boolean function fn, which arguments are defined by args
 * stores bool result into ret
 */
static int MACRO_CallBoolFunc(FARPROC fn, const char* args, void** ret)
{
    void*       pa[2];
    int         idx = MACRO_CheckArgs(pa, sizeof(pa)/sizeof(pa[0]), args);

    if (idx < 0) return 0;
    if (!fn)     return 1;

    WINE_TRACE("calling with %u pmts\n", idx);

    switch (strlen(args))
    {
    case 0: *ret = (void*)(fn)();          break;
    case 1: *ret = (void*)(fn)(pa[0]);     break;
    default: WINE_FIXME("NIY\n");
    }

    return 1;
}

/******************************************************************
 *		MACRO_CallVoidFunc
 *
 *
 */
static int MACRO_CallVoidFunc(FARPROC fn, const char* args)
{
    void*       pa[6];
    int         idx = MACRO_CheckArgs(pa, sizeof(pa)/sizeof(pa[0]), args);

    if (idx < 0) return 0;
    if (!fn)     return 1;

    WINE_TRACE("calling %p with %u pmts\n", fn, idx);

    switch (strlen(args))
    {
    case 0: (fn)();                                     break;
    case 1: (fn)(pa[0]);                                break;
    case 2: (fn)(pa[0],pa[1]);                          break;
    case 3: (fn)(pa[0],pa[1],pa[2]);                    break;
    case 4: (fn)(pa[0],pa[1],pa[2],pa[3]);              break;
    case 5: (fn)(pa[0],pa[1],pa[2],pa[3],pa[4]);        break;
    case 6: (fn)(pa[0],pa[1],pa[2],pa[3],pa[4],pa[5]);  break;
    default: WINE_FIXME("NIY\n");
    }

    return 1;
}

BOOL MACRO_ExecuteMacro(LPCSTR macro)
{
    struct lex_data     curr_lex_data, *prev_lex_data;
    BOOL ret = TRUE;
    int t;

    WINE_TRACE("%s\n", wine_dbgstr_a(macro));

    prev_lex_data = lex_data;
    lex_data = &curr_lex_data;

    memset(lex_data, 0, sizeof(*lex_data));
    lex_data->macroptr = macro;

    while ((t = yylex()) != EMPTY)
    {
        switch (t)
        {
        case VOID_FUNCTION:
            WINE_TRACE("got type void func(%s)\n", yylval.proto);
            MACRO_CallVoidFunc(yylval.function, yylval.proto);
            break;
        case BOOL_FUNCTION:
            WINE_WARN("got type bool func(%s)\n", yylval.proto);
            break;
        default:
            WINE_WARN("got unexpected type %s\n", ts(t));
            return 0;
        }
        switch (t = yylex())
        {
        case EMPTY:     goto done;
        case ';':       break;
        default:        ret = FALSE; goto done;
        }
    }

done:
    for (t = 0; t < lex_data->cache_used; t++)
        HeapFree(GetProcessHeap(), 0, lex_data->cache_string[t]);
    lex_data = prev_lex_data;

    return ret;
}

#ifndef yywrap
int yywrap(void) { return 1; }
#endif
