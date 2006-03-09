/*
 * Unit test suite for StringTable functions
 *
 * Copyright 2005 Steven Edwards for ReactOS
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/* 
 * TODO:
 * Add case sensitivity test for StringTableAddString/StringTableLookupString
 * Add test for StringTableStringFromIdEx
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "setupapi.h"

#include "wine/test.h"


static DWORD    (WINAPI *pStringTableAddString)(HSTRING_TABLE, LPWSTR, DWORD);
static VOID     (WINAPI *pStringTableDestroy)(HSTRING_TABLE);
static HSTRING_TABLE (WINAPI *pStringTableDuplicate)(HSTRING_TABLE hStringTable);
static HSTRING_TABLE (WINAPI *pStringTableInitialize)(VOID);
static DWORD    (WINAPI *pStringTableLookUpString)(HSTRING_TABLE, LPWSTR, DWORD);
static LPWSTR   (WINAPI *pStringTableStringFromId)(HSTRING_TABLE, DWORD);
#if 0
static BOOL     (WINAPI *pStringTableStringFromIdEx)(HSTRING_TABLE, DWORD, LPWSTR, LPDWORD);
static VOID     (WINAPI *pStringTableTrim)(HSTRING_TABLE);
#endif

HMODULE hdll;
static WCHAR string[] = {'s','t','r','i','n','g',0};
HANDLE table, table2;  /* Handles pointing to our tables */

static void load_it_up()
{
    hdll = LoadLibraryA("setupapi.dll");
    if (!hdll)
        return;

    pStringTableInitialize = (void*)GetProcAddress(hdll, "StringTableInitialize");
    if (!pStringTableInitialize)
        pStringTableInitialize = (void*)GetProcAddress(hdll, "pSetupStringTableInitialize");

    pStringTableAddString = (void*)GetProcAddress(hdll, "StringTableAddString");
    if (!pStringTableAddString)
        pStringTableAddString = (void*)GetProcAddress(hdll, "pSetupStringTableAddString");

    pStringTableDuplicate = (void*)GetProcAddress(hdll, "StringTableDuplicate");
    if (!pStringTableDuplicate)
        pStringTableDuplicate = (void*)GetProcAddress(hdll, "pSetupStringTableDuplicate");

    pStringTableDestroy = (void*)GetProcAddress(hdll, "StringTableDestroy");
    if (!pStringTableDestroy)
        pStringTableDestroy = (void*)GetProcAddress(hdll, "pSetupStringTableDestroy");

    pStringTableLookUpString = (void*)GetProcAddress(hdll, "StringTableLookUpString");
    if (!pStringTableLookUpString)
        pStringTableLookUpString = (void*)GetProcAddress(hdll, "pSetupStringTableLookUpString");

    pStringTableStringFromId = (void*)GetProcAddress(hdll, "StringTableStringFromId");
    if (!pStringTableStringFromId)
        pStringTableStringFromId = (void*)GetProcAddress(hdll, "pSetupStringTableStringFromId");
}

static void test_StringTableInitialize()
{
    table=pStringTableInitialize();
    ok(table!=NULL,"Failed to Initialize String Table\n");
}

static void test_StringTableAddString()
{
    DWORD retval;

    retval=pStringTableAddString(table,string,0);
    ok(retval!=-1,"Failed to add string to String Table\n");
}

static void test_StringTableDuplicate()
{
    table2=pStringTableDuplicate(table);
    ok(table2!=NULL,"Failed to duplicate String Table\n");
}

static void test_StringTableLookUpString()
{   
    DWORD retval, retval2;
    
    retval=pStringTableLookUpString(table,string,0);
    ok(retval!=-1,"Failed find string in String Table 1\n");

    retval2=pStringTableLookUpString(table2,string,0);
    ok(retval2!=-1,"Failed find string in String Table 2\n");
}

static void test_StringTableStringFromId()
{
    WCHAR *string2, *string3;
    int result;

    /* correct */
    string2=pStringTableStringFromId(table,pStringTableLookUpString(table,string,0));
    ok(string2!=NULL,"Failed to look up string by ID from String Table\n");
    
    result=lstrcmpiW(string, string2);
    ok(result==0,"String %p does not match requested StringID %p\n",string,string2);

    todo_wine{
        /* This should not pass on Wine but it does */
        string3=pStringTableStringFromId(table,0);
        ok(string3!=NULL,"Failed to look up string by ID from String Table\n");

        result=lstrcmpiW(string, string3);
        ok(result!=0,"String %p does not match requested StringID %p\n",string,string2);
    }
}

START_TEST(stringtable)
{
    load_it_up();

    test_StringTableInitialize();
    test_StringTableAddString();
    test_StringTableDuplicate();
    test_StringTableLookUpString();
    test_StringTableStringFromId();

    /* assume we can always distroy */
    pStringTableDestroy(table);
    pStringTableDestroy(table2);

    FreeLibrary(hdll);
}
