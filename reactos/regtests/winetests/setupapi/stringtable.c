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
/* TODO:
 * Add case sensitivity test for StringTableAddString/StringTableLookupString
 * Add test for StringTableStringFromIdEx
 *
 * BUGS:
 * These functions are undocumented and exported under another name on 
 * Windows XP and Windows Server 2003. This test assumes the Windows 2000
 * implementation.
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "setupapi.h"

#include "wine/test.h"

HANDLE table, table2;  /* Handles pointing to our tables */
static WCHAR string[] = {'s','t','r','i','n','g',0};

static void test_StringTableInitialize()
{
    table=StringTableInitialize();
    ok(table!=NULL,"Failed to Initialize String Table\n");
}

static void test_StringTableAddString()
{
    DWORD retval;

    retval=StringTableAddString(table,string,0);
    ok(retval!=-1,"Failed to add string to String Table\n");
}

static void test_StringTableDuplicate()
{
    table2=StringTableDuplicate(table);
    ok(table2!=NULL,"Failed to duplicate String Table\n");
}

static void test_StringTableLookUpString()
{   
    DWORD retval, retval2;
    
    retval=StringTableLookUpString(table,string,0);
    ok(retval!=-1,"Failed find string in String Table 1\n");

    retval2=StringTableLookUpString(table2,string,0);
    ok(retval2!=-1,"Failed find string in String Table 2\n");
}

static void test_StringTableStringFromId()
{
    WCHAR *string2;
    int result;
    
    string2=StringTableStringFromId(table,0);
    ok(string2!=NULL,"Failed to look up string by ID from String Table\n");
    
    result=lstrcmpiW(string, string2);
    ok(result==0,"String %p does not match requested StringID %p\n",string,string2);
}

START_TEST(stringtable)
{
    test_StringTableInitialize();
    test_StringTableAddString();
    test_StringTableDuplicate();
    test_StringTableLookUpString();
    test_StringTableStringFromId();

    /* Cleanup */
    StringTableDestroy(table);
}
