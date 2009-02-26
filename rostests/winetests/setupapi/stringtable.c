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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
/* 
 * TODO:
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

/* Flags for StringTableAddString and StringTableLookUpString */
#define ST_CASE_SENSITIVE_COMPARE	0x00000001

static DWORD    (WINAPI *pStringTableAddString)(HSTRING_TABLE, LPWSTR, DWORD);
static DWORD    (WINAPI *pStringTableAddStringEx)(HSTRING_TABLE, LPWSTR, DWORD, LPVOID, DWORD);
static VOID     (WINAPI *pStringTableDestroy)(HSTRING_TABLE);
static HSTRING_TABLE (WINAPI *pStringTableDuplicate)(HSTRING_TABLE hStringTable);
static HSTRING_TABLE (WINAPI *pStringTableInitialize)(VOID);
static HSTRING_TABLE (WINAPI *pStringTableInitializeEx)(DWORD, DWORD);
static DWORD    (WINAPI *pStringTableLookUpString)(HSTRING_TABLE, LPWSTR, DWORD);
static DWORD    (WINAPI *pStringTableLookUpStringEx)(HSTRING_TABLE, LPWSTR, DWORD, LPVOID, DWORD);
static LPWSTR   (WINAPI *pStringTableStringFromId)(HSTRING_TABLE, DWORD);
#if 0
static BOOL     (WINAPI *pStringTableStringFromIdEx)(HSTRING_TABLE, DWORD, LPWSTR, LPDWORD);
static VOID     (WINAPI *pStringTableTrim)(HSTRING_TABLE);
#endif

HMODULE hdll;
static WCHAR string[] = {'s','t','r','i','n','g',0};
static WCHAR String[] = {'S','t','r','i','n','g',0};
static WCHAR foo[] = {'f','o','o',0};

static void load_it_up(void)
{
    hdll = GetModuleHandleA("setupapi.dll");

    pStringTableInitialize = (void*)GetProcAddress(hdll, "StringTableInitialize");
    if (!pStringTableInitialize)
        pStringTableInitialize = (void*)GetProcAddress(hdll, "pSetupStringTableInitialize");

    pStringTableInitializeEx = (void*)GetProcAddress(hdll, "StringTableInitializeEx");
    if (!pStringTableInitializeEx)
        pStringTableInitializeEx = (void*)GetProcAddress(hdll, "pSetupStringTableInitializeEx");

    pStringTableAddString = (void*)GetProcAddress(hdll, "StringTableAddString");
    if (!pStringTableAddString)
        pStringTableAddString = (void*)GetProcAddress(hdll, "pSetupStringTableAddString");

    pStringTableAddStringEx = (void*)GetProcAddress(hdll, "StringTableAddStringEx");
    if (!pStringTableAddStringEx)
        pStringTableAddStringEx = (void*)GetProcAddress(hdll, "pSetupStringTableAddStringEx");

    pStringTableDuplicate = (void*)GetProcAddress(hdll, "StringTableDuplicate");
    if (!pStringTableDuplicate)
        pStringTableDuplicate = (void*)GetProcAddress(hdll, "pSetupStringTableDuplicate");

    pStringTableDestroy = (void*)GetProcAddress(hdll, "StringTableDestroy");
    if (!pStringTableDestroy)
        pStringTableDestroy = (void*)GetProcAddress(hdll, "pSetupStringTableDestroy");

    pStringTableLookUpString = (void*)GetProcAddress(hdll, "StringTableLookUpString");
    if (!pStringTableLookUpString)
        pStringTableLookUpString = (void*)GetProcAddress(hdll, "pSetupStringTableLookUpString");

    pStringTableLookUpStringEx = (void*)GetProcAddress(hdll, "StringTableLookUpStringEx");
    if (!pStringTableLookUpStringEx)
        pStringTableLookUpStringEx = (void*)GetProcAddress(hdll, "pSetupStringTableLookUpStringEx");

    pStringTableStringFromId = (void*)GetProcAddress(hdll, "StringTableStringFromId");
    if (!pStringTableStringFromId)
        pStringTableStringFromId = (void*)GetProcAddress(hdll, "pSetupStringTableStringFromId");
}

static void test_StringTableAddString(void)
{
    DWORD retval, hstring, hString, hfoo;
    HANDLE table;

    table = pStringTableInitialize();
    ok(table != NULL, "failed to initialize string table\n");

    /* case insensitive */
    hstring=pStringTableAddString(table,string,0);
    ok(hstring!=-1,"Failed to add string to String Table\n");
    
    retval=pStringTableAddString(table,String,0);
    ok(retval!=-1,"Failed to add String to String Table\n");    
    ok(hstring==retval,"string handle %x != String handle %x in String Table\n", hstring, retval);        
    
    hfoo=pStringTableAddString(table,foo,0);
    ok(hfoo!=-1,"Failed to add foo to String Table\n");        
    ok(hfoo!=hstring,"foo and string share the same ID %x in String Table\n", hfoo);            
    
    /* case sensitive */    
    hString=pStringTableAddString(table,String,ST_CASE_SENSITIVE_COMPARE);
    ok(hstring!=hString,"String handle and string share same ID %x in Table\n", hstring);        

    pStringTableDestroy(table);
}

static void test_StringTableAddStringEx(void)
{
    DWORD retval, hstring, hString, hfoo;
    HANDLE table;

    table = pStringTableInitialize();
    ok(table != NULL,"Failed to Initialize String Table\n");

    /* case insensitive */
    hstring = pStringTableAddStringEx(table, string, 0, NULL, 0);
    ok(hstring != ~0u, "Failed to add string to String Table\n");

    retval = pStringTableAddStringEx(table, String, 0, NULL, 0);
    ok(retval != ~0u, "Failed to add String to String Table\n");
    ok(hstring == retval, "string handle %x != String handle %x in String Table\n", hstring, retval);

    hfoo = pStringTableAddStringEx(table, foo, 0, NULL, 0);
    ok(hfoo != ~0u, "Failed to add foo to String Table\n");
    ok(hfoo != hstring, "foo and string share the same ID %x in String Table\n", hfoo);

    /* case sensitive */
    hString = pStringTableAddStringEx(table, String, ST_CASE_SENSITIVE_COMPARE, NULL, 0);
    ok(hstring != hString, "String handle and string share same ID %x in Table\n", hstring);

    pStringTableDestroy(table);
}

static void test_StringTableDuplicate(void)
{
    HANDLE table, table2;

    table = pStringTableInitialize();
    ok(table != NULL,"Failed to Initialize String Table\n");

    table2=pStringTableDuplicate(table);
    ok(table2!=NULL,"Failed to duplicate String Table\n");

    pStringTableDestroy(table);
    pStringTableDestroy(table2);
}

static void test_StringTableLookUpString(void)
{   
    DWORD retval, retval2, hstring, hString, hfoo;
    HANDLE table, table2;

    table = pStringTableInitialize();
    ok(table != NULL,"failed to initialize string table\n");

    hstring = pStringTableAddString(table, string, 0);
    ok(hstring != ~0u, "failed to add 'string' to string table\n");

    hString = pStringTableAddString(table, String, 0);
    ok(hString != ~0u,"failed to add 'String' to string table\n");

    hfoo = pStringTableAddString(table, foo, 0);
    ok(hfoo != ~0u, "failed to add 'foo' to string table\n");

    table2 = pStringTableDuplicate(table);
    ok(table2 != NULL, "Failed to duplicate String Table\n");

    /* case insensitive */
    retval=pStringTableLookUpString(table,string,0);
    ok(retval!=-1,"Failed find string in String Table 1\n");
    ok(retval==hstring,
        "Lookup for string (%x) does not match previous handle (%x) in String Table 1\n",
        retval, hstring);    

    retval=pStringTableLookUpString(table2,string,0);
    ok(retval!=-1,"Failed find string in String Table 2\n");
    
    retval=pStringTableLookUpString(table,String,0);
    ok(retval!=-1,"Failed find String in String Table 1\n");

    retval=pStringTableLookUpString(table2,String,0);
    ok(retval!=-1,"Failed find String in String Table 2\n");    
    
    retval=pStringTableLookUpString(table,foo,0);
    ok(retval!=-1,"Failed find foo in String Table 1\n");    
    ok(retval==hfoo,
        "Lookup for foo (%x) does not match previous handle (%x) in String Table 1\n",
        retval, hfoo);        
    
    retval=pStringTableLookUpString(table2,foo,0);
    ok(retval!=-1,"Failed find foo in String Table 2\n");    
    
    /* case sensitive */
    retval=pStringTableLookUpString(table,string,ST_CASE_SENSITIVE_COMPARE);
    retval2=pStringTableLookUpString(table,String,ST_CASE_SENSITIVE_COMPARE);    
    ok(retval!=retval2,"Lookup of string equals String in Table 1\n");
    ok(retval==hString,
        "Lookup for String (%x) does not match previous handle (%x) in String Table 1\n",
        retval, hString);

    pStringTableDestroy(table);
    pStringTableDestroy(table2);
}

static void test_StringTableLookUpStringEx(void)
{
    static WCHAR uilevel[] = {'U','I','L','E','V','E','L',0};
    DWORD retval, retval2, hstring, hString, hfoo, data;
    HANDLE table, table2;
    char buffer[4];

    table = pStringTableInitialize();
    ok(table != NULL,"Failed to Initialize String Table\n");

    hstring = pStringTableAddString(table, string, 0);
    ok(hstring != ~0u, "failed to add 'string' to string table\n");

    hString = pStringTableAddString(table, String, 0);
    ok(hString != ~0u,"failed to add 'String' to string table\n");

    hfoo = pStringTableAddString(table, foo, 0);
    ok(hfoo != ~0u, "failed to add 'foo' to string table\n");

    table2 = pStringTableDuplicate(table);
    ok(table2 != NULL, "Failed to duplicate String Table\n");

    /* case insensitive */
    retval = pStringTableLookUpStringEx(table, string, 0, NULL, 0);
    ok(retval != ~0u, "Failed find string in String Table 1\n");
    ok(retval == hstring,
        "Lookup for string (%x) does not match previous handle (%x) in String Table 1\n",
        retval, hstring);

    retval = pStringTableLookUpStringEx(table2, string, 0, NULL, 0);
    ok(retval != ~0u, "Failed find string in String Table 2\n");

    retval = pStringTableLookUpStringEx(table, String, 0, NULL, 0);
    ok(retval != ~0u, "Failed find String in String Table 1\n");

    retval = pStringTableLookUpStringEx(table2, String, 0, NULL, 0);
    ok(retval != ~0u, "Failed find String in String Table 2\n");

    retval=pStringTableLookUpStringEx(table, foo, 0, NULL, 0);
    ok(retval != ~0u, "Failed find foo in String Table 1\n");
    ok(retval == hfoo,
        "Lookup for foo (%x) does not match previous handle (%x) in String Table 1\n",
        retval, hfoo);

    retval = pStringTableLookUpStringEx(table2, foo, 0, NULL, 0);
    ok(retval != ~0u, "Failed find foo in String Table 2\n");

    /* case sensitive */
    retval = pStringTableLookUpStringEx(table, string,ST_CASE_SENSITIVE_COMPARE, NULL, 0);
    retval2 = pStringTableLookUpStringEx(table, String, ST_CASE_SENSITIVE_COMPARE, NULL, 0);
    ok(retval != retval2, "Lookup of string equals String in Table 1\n");
    ok(retval == hString,
        "Lookup for String (%x) does not match previous handle (%x) in String Table 1\n",
        retval, hString);

    pStringTableDestroy(table);

    table = pStringTableInitializeEx(0x1000, 0);
    ok(table != NULL, "failed to initialize string table\n");

    data = 0xaaaaaaaa;
    retval = pStringTableAddStringEx(table, uilevel, 0x5, &data, sizeof(data));
    ok(retval != ~0u, "failed to add 'UILEVEL' to string table\n");

    memset(buffer, 0x55, sizeof(buffer));
    retval = pStringTableLookUpStringEx(table, uilevel, ST_CASE_SENSITIVE_COMPARE, buffer, 0);
    ok(retval != ~0u, "failed find 'UILEVEL' in string table\n");
    ok(memcmp(buffer, &data, 4), "unexpected data\n");

    memset(buffer, 0x55, sizeof(buffer));
    retval = pStringTableLookUpStringEx(table, uilevel, ST_CASE_SENSITIVE_COMPARE, buffer, 2);
    ok(retval != ~0u, "failed find 'UILEVEL' in string table\n");
    ok(!memcmp(buffer, &data, 2), "unexpected data\n");

    memset(buffer, 0x55, sizeof(buffer));
    retval = pStringTableLookUpStringEx(table, uilevel, ST_CASE_SENSITIVE_COMPARE, buffer, sizeof(buffer));
    ok(retval != ~0u, "failed find 'UILEVEL' in string table\n");
    ok(!memcmp(buffer, &data, 4), "unexpected data\n");

    pStringTableDestroy(table);
}

static void test_StringTableStringFromId(void)
{
    HANDLE table;
    DWORD hstring;
    WCHAR *string2;
    int result;

    table = pStringTableInitialize();
    ok(table != NULL,"Failed to Initialize String Table\n");

    hstring = pStringTableAddString(table, string, 0);
    ok(hstring != ~0u,"failed to add 'string' to string table\n");

    /* correct */
    string2=pStringTableStringFromId(table,pStringTableLookUpString(table,string,0));
    ok(string2!=NULL,"Failed to look up string by ID from String Table\n");
    
    result=lstrcmpiW(string, string2);
    ok(result==0,"StringID %p does not match requested StringID %p\n",string,string2);

    pStringTableDestroy(table);
}

START_TEST(stringtable)
{
    load_it_up();

    test_StringTableAddString();
    test_StringTableAddStringEx();
    test_StringTableDuplicate();
    test_StringTableLookUpString();
    test_StringTableLookUpStringEx();
    test_StringTableStringFromId();
}
