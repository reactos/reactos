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
#include "winnls.h"
#include "setupapi.h"

#include "wine/test.h"

DECLARE_HANDLE(HSTRING_TABLE);

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
static BOOL     (WINAPI *pStringTableGetExtraData)(HSTRING_TABLE, ULONG, void*, ULONG);

static WCHAR string[] = {'s','t','r','i','n','g',0};
static WCHAR String[] = {'S','t','r','i','n','g',0};
static WCHAR foo[] = {'f','o','o',0};

static void load_it_up(void)
{
    HMODULE hdll = GetModuleHandleA("setupapi.dll");

#define X(f) if (!(p##f = (void*)GetProcAddress(hdll, #f))) \
                 p##f = (void*)GetProcAddress(hdll, "pSetup"#f);
    X(StringTableInitialize);
    X(StringTableInitializeEx);
    X(StringTableAddString);
    X(StringTableAddStringEx);
    X(StringTableDuplicate);
    X(StringTableDestroy);
    X(StringTableLookUpString);
    X(StringTableLookUpStringEx);
    X(StringTableStringFromId);
    X(StringTableGetExtraData);
#undef X
}

static void test_StringTableAddString(void)
{
    DWORD retval, hstring, hString, hfoo;
    HSTRING_TABLE table;

    table = pStringTableInitialize();
    ok(table != NULL, "failed to initialize string table\n");

    /* case insensitive */
    hstring=pStringTableAddString(table,string,0);
    ok(hstring!=-1,"Failed to add string to String Table\n");

    retval=pStringTableAddString(table,String,0);
    ok(retval!=-1,"Failed to add String to String Table\n");    
    ok(hstring==retval,"string handle %lx != String handle %lx in String Table\n", hstring, retval);
    
    hfoo=pStringTableAddString(table,foo,0);
    ok(hfoo!=-1,"Failed to add foo to String Table\n");        
    ok(hfoo!=hstring,"foo and string share the same ID %lx in String Table\n", hfoo);
    
    /* case sensitive */    
    hString=pStringTableAddString(table,String,ST_CASE_SENSITIVE_COMPARE);
    ok(hstring!=hString,"String handle and string share same ID %lx in Table\n", hstring);

    pStringTableDestroy(table);
}

static void test_StringTableAddStringEx(void)
{
    DWORD retval, hstring, hString, hfoo, extra;
    HANDLE table;
    BOOL ret;

    table = pStringTableInitialize();
    ok(table != NULL,"Failed to Initialize String Table\n");

    /* case insensitive */
    hstring = pStringTableAddStringEx(table, string, 0, NULL, 0);
    ok(hstring != -1, "Failed to add string to String Table\n");

    retval = pStringTableAddStringEx(table, String, 0, NULL, 0);
    ok(retval != -1, "Failed to add String to String Table\n");
    ok(hstring == retval, "string handle %lx != String handle %lx in String Table\n", hstring, retval);

    hfoo = pStringTableAddStringEx(table, foo, 0, NULL, 0);
    ok(hfoo != -1, "Failed to add foo to String Table\n");
    ok(hfoo != hstring, "foo and string share the same ID %lx in String Table\n", hfoo);

    /* case sensitive */
    hString = pStringTableAddStringEx(table, String, ST_CASE_SENSITIVE_COMPARE, NULL, 0);
    ok(hstring != hString, "String handle and string share same ID %lx in Table\n", hstring);

    pStringTableDestroy(table);

    /* set same string twice but with different extra */
    table = pStringTableInitializeEx(4, 0);
    ok(table != NULL, "Failed to Initialize String Table\n");

    extra = 10;
    hstring = pStringTableAddStringEx(table, string, 0, &extra, 4);
    ok(hstring != -1, "failed to add string, %ld\n", hstring);

    extra = 0;
    ret = pStringTableGetExtraData(table, hstring, &extra, 4);
    ok(ret && extra == 10, "got %d, extra %ld\n", ret, extra);

    extra = 11;
    hstring = pStringTableAddStringEx(table, string, 0, &extra, 4);
    ok(hstring != -1, "failed to add string, %ld\n", hstring);

    extra = 0;
    ret = pStringTableGetExtraData(table, hstring, &extra, 4);
    ok(ret && extra == 10, "got %d, extra %ld\n", ret, extra);

    pStringTableDestroy(table);
}

static void test_StringTableDuplicate(void)
{
    HSTRING_TABLE table, table2;

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
    HSTRING_TABLE table, table2;

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
        "Lookup for string (%lx) does not match previous handle (%lx) in String Table 1\n",
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
        "Lookup for foo (%lx) does not match previous handle (%lx) in String Table 1\n",
        retval, hfoo);        
    
    retval=pStringTableLookUpString(table2,foo,0);
    ok(retval!=-1,"Failed find foo in String Table 2\n");    
    
    /* case sensitive */
    retval=pStringTableLookUpString(table,string,ST_CASE_SENSITIVE_COMPARE);
    retval2=pStringTableLookUpString(table,String,ST_CASE_SENSITIVE_COMPARE);    
    ok(retval!=retval2,"Lookup of string equals String in Table 1\n");
    ok(retval==hString,
        "Lookup for String (%lx) does not match previous handle (%lx) in String Table 1\n",
        retval, hString);

    pStringTableDestroy(table);
    pStringTableDestroy(table2);
}

static void test_StringTableLookUpStringEx(void)
{
    static WCHAR uilevel[] = {'U','I','L','E','V','E','L',0};
    DWORD retval, retval2, hstring, hString, hfoo, data;
    HSTRING_TABLE table, table2;
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
        "Lookup for string (%lx) does not match previous handle (%lx) in String Table 1\n",
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
        "Lookup for foo (%lx) does not match previous handle (%lx) in String Table 1\n",
        retval, hfoo);

    retval = pStringTableLookUpStringEx(table2, foo, 0, NULL, 0);
    ok(retval != ~0u, "Failed find foo in String Table 2\n");

    /* case sensitive */
    retval = pStringTableLookUpStringEx(table, string,ST_CASE_SENSITIVE_COMPARE, NULL, 0);
    retval2 = pStringTableLookUpStringEx(table, String, ST_CASE_SENSITIVE_COMPARE, NULL, 0);
    ok(retval != retval2, "Lookup of string equals String in Table 1\n");
    ok(retval == hString,
        "Lookup for String (%lx) does not match previous handle (%lx) in String Table 1\n",
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
    pStringTableDestroy(table2);
}

static void test_StringTableStringFromId(void)
{
    HSTRING_TABLE table;
    WCHAR *string2;
    DWORD id, id2;

    table = pStringTableInitialize();
    ok(table != NULL, "Failed to Initialize String Table\n");

    id = pStringTableAddString(table, string, 0);
    ok(id != -1, "failed to add 'string' to string table\n");

    /* correct */
    id2 = pStringTableLookUpString(table, string, 0);
    ok(id2 == id, "got %ld and %ld\n", id2, id);

    string2 = pStringTableStringFromId(table, id2);
    ok(string2 != NULL, "failed to lookup string %ld\n", id2);
    ok(!lstrcmpiW(string, string2), "got %s, expected %s\n", wine_dbgstr_w(string2), wine_dbgstr_w(string));

    pStringTableDestroy(table);
}

struct stringtable {
    char     *data;
    ULONG     nextoffset;
    ULONG     allocated;
    DWORD_PTR unk[2];
    ULONG     max_extra_size;
    LCID      lcid;
};

static void test_stringtable_layout(void)
{
    struct stringtable *ptr;
    HSTRING_TABLE table;

    table = pStringTableInitialize();
    ok(table != NULL,"failed to initialize string table\n");

    ptr = (struct stringtable*)table;
    ok(ptr->data != NULL, "got %p\n", ptr->data);
    /* first data offset is right after bucket area */
    ok(ptr->nextoffset == 509*sizeof(DWORD), "got %ld\n", ptr->nextoffset);
    ok(ptr->allocated != 0, "got %ld\n", ptr->allocated);
todo_wine {
    ok(ptr->unk[0] != 0, "got %Ix\n", ptr->unk[0]);
    ok(ptr->unk[1] != 0, "got %Ix\n", ptr->unk[1]);
}
    ok(ptr->max_extra_size == 0, "got %ld\n", ptr->max_extra_size);
    ok(ptr->lcid == GetThreadLocale(), "got %lx, thread lcid %lx\n", ptr->lcid, GetThreadLocale());

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
    test_stringtable_layout();
}
