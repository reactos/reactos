/*
 * Unit tests for Windows String functions
 *
 * Copyright (c) 2014 Martin Storsjo
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winstring.h"

#include "wine/test.h"

#define check_string(str, content, length, has_null) _check_string(__LINE__, str, content, length, has_null)
static void _check_string(int line, HSTRING str, LPCWSTR content, UINT32 length, BOOL has_null)
{
    BOOL out_null;
    BOOL empty = length == 0;
    UINT32 out_length;
    LPCWSTR ptr;

    ok_(__FILE__, line)(WindowsIsStringEmpty(str) == empty, "WindowsIsStringEmpty failed\n");
    ok_(__FILE__, line)(WindowsStringHasEmbeddedNull(str, &out_null) == S_OK, "WindowsStringHasEmbeddedNull failed\n");
    ok_(__FILE__, line)(out_null == has_null, "WindowsStringHasEmbeddedNull failed\n");
    ok_(__FILE__, line)(WindowsGetStringLen(str) == length, "WindowsGetStringLen failed\n");
    ptr = WindowsGetStringRawBuffer(str, &out_length);
    /* WindowsGetStringRawBuffer should return a non-null, null terminated empty string
     * even if str is NULL. */
    ok_(__FILE__, line)(ptr != NULL, "WindowsGetStringRawBuffer returned null\n");
    ok_(__FILE__, line)(out_length == length, "WindowsGetStringRawBuffer returned incorrect length\n");
    ptr = WindowsGetStringRawBuffer(str, NULL);
    ok_(__FILE__, line)(ptr != NULL, "WindowsGetStringRawBuffer returned null\n");
    ok_(__FILE__, line)(ptr[length] == '\0', "WindowsGetStringRawBuffer doesn't return a null terminated buffer\n");
    ok_(__FILE__, line)(memcmp(ptr, content, sizeof(*content) * length) == 0, "Incorrect string content\n");
}

static const WCHAR input_string[] = L"abcdef\0";
static const WCHAR input_string1[] = L"abc";
static const WCHAR input_string2[] = L"def";
static const WCHAR input_embed_null[] = L"a\0c\0ef";
static const WCHAR output_substring[] = L"cdef";

static void test_create_delete(void)
{
    HSTRING str;
    HSTRING_HEADER header;

    /* Test normal creation of a string */
    ok(WindowsCreateString(input_string, 6, &str) == S_OK, "Failed to create string\n");
    check_string(str, input_string, 6, FALSE);
    ok(WindowsDeleteString(str) == S_OK, "Failed to delete string\n");
    /* Test error handling in WindowsCreateString */
    ok(WindowsCreateString(input_string, 6, NULL) == E_INVALIDARG, "Incorrect error handling\n");
    ok(WindowsCreateString(NULL, 6, &str) == E_POINTER, "Incorrect error handling\n");

    /* Test handling of a NULL string */
    ok(WindowsDeleteString(NULL) == S_OK, "Failed to delete null string\n");

    /* Test creation of a string reference */
    ok(WindowsCreateStringReference(input_string, 6, &header, &str) == S_OK, "Failed to create string ref\n");
    check_string(str, input_string, 6, FALSE);
    ok(WindowsDeleteString(str) == S_OK, "Failed to delete string ref\n");

    /* Test error handling in WindowsCreateStringReference */
    /* Strings to CreateStringReference must be null terminated with the correct
     * length. According to MSDN this should be E_INVALIDARG, but it returns
     * 0x80000017 in practice. */
    ok(FAILED(WindowsCreateStringReference(input_string, 5, &header, &str)), "Incorrect error handling\n");
    /* If the input string is non-null, it must be null-terminated even if the
     * length is zero. */
    ok(FAILED(WindowsCreateStringReference(input_string, 0, &header, &str)), "Incorrect error handling\n");
    ok(WindowsCreateStringReference(input_string, 6, NULL, &str) == E_INVALIDARG, "Incorrect error handling\n");
    ok(WindowsCreateStringReference(input_string, 6, &header, NULL) == E_INVALIDARG, "Incorrect error handling\n");
    ok(WindowsCreateStringReference(NULL, 6, &header, &str) == E_POINTER, "Incorrect error handling\n");

    /* Test creating a string without a null-termination at the specified length */
    ok(WindowsCreateString(input_string, 3, &str) == S_OK, "Failed to create string\n");
    check_string(str, input_string, 3, FALSE);
    ok(WindowsDeleteString(str) == S_OK, "Failed to delete string\n");

    /* Test an empty string */
    ok(WindowsCreateString(L"", 0, &str) == S_OK, "Failed to create string\n");
    ok(str == NULL, "Empty string not a null string\n");
    ok(WindowsDeleteString(str) == S_OK, "Failed to delete string\n");

    ok(WindowsCreateString(input_string, 0, &str) == S_OK, "Failed to create string\n");
    ok(str == NULL, "Empty string not a null string\n");
    ok(WindowsDeleteString(str) == S_OK, "Failed to delete string\n");

    ok(WindowsCreateStringReference(L"", 0, &header, &str) == S_OK, "Failed to create string\n");
    ok(str == NULL, "Empty string not a null string\n");
    ok(WindowsDeleteString(str) == S_OK, "Failed to delete string\n");

    ok(WindowsCreateString(NULL, 0, &str) == S_OK, "Failed to create string\n");
    ok(str == NULL, "Empty string not a null string\n");
    ok(WindowsDeleteString(str) == S_OK, "Failed to delete string\n");

    ok(WindowsCreateStringReference(NULL, 0, &header, &str) == S_OK, "Failed to create string\n");
    ok(str == NULL, "Empty string not a null string\n");
    ok(WindowsDeleteString(str) == S_OK, "Failed to delete string\n");
}

static void test_duplicate(void)
{
    HSTRING str, str2;
    HSTRING_HEADER header;
    ok(WindowsCreateString(input_string, 6, &str) == S_OK, "Failed to create string\n");
    ok(WindowsDuplicateString(str, NULL) == E_INVALIDARG, "Incorrect error handling\n");
    ok(WindowsDuplicateString(str, &str2) == S_OK, "Failed to duplicate string\n");
    ok(str == str2, "Duplicated string created new string\n");
    ok(WindowsDeleteString(str) == S_OK, "Failed to delete string\n");
    ok(WindowsDeleteString(str2) == S_OK, "Failed to delete string\n");

    ok(WindowsCreateStringReference(input_string, 6, &header, &str) == S_OK, "Failed to create string ref\n");
    ok(WindowsDuplicateString(str, &str2) == S_OK, "Failed to duplicate string\n");
    ok(str != str2, "Duplicated string ref didn't create new string\n");
    ok(WindowsDeleteString(str) == S_OK, "Failed to delete string\n");
    ok(WindowsDeleteString(str2) == S_OK, "Failed to delete string ref\n");

    ok(WindowsDuplicateString(NULL, &str2) == S_OK, "Failed to duplicate NULL string\n");
    ok(str2 == NULL, "Duplicated string created new string\n");
    ok(WindowsDeleteString(str2) == S_OK, "Failed to delete string\n");
}

static void test_access(void)
{
    HSTRING str;
    HSTRING_HEADER header;

    /* Test handling of a NULL string */
    check_string(NULL, NULL, 0, FALSE);

    /* Test strings with embedded null chars */
    ok(WindowsCreateString(input_embed_null, 6, &str) == S_OK, "Failed to create string\n");
    check_string(str, input_embed_null, 6, TRUE);
    ok(WindowsDeleteString(str) == S_OK, "Failed to delete string\n");

    ok(WindowsCreateStringReference(input_embed_null, 6, &header, &str) == S_OK, "Failed to create string ref\n");
    check_string(str, input_embed_null, 6, TRUE);
    ok(WindowsDeleteString(str) == S_OK, "Failed to delete string ref\n");

    /* Test normal creation of a string with trailing null */
    ok(WindowsCreateString(input_string, 7, &str) == S_OK, "Failed to create string\n");
    check_string(str, input_string, 7, TRUE);
    ok(WindowsDeleteString(str) == S_OK, "Failed to delete string\n");

    ok(WindowsCreateStringReference(input_string, 7, &header, &str) == S_OK, "Failed to create string ref\n");
    check_string(str, input_string, 7, TRUE);
    ok(WindowsDeleteString(str) == S_OK, "Failed to delete string ref\n");
}

static void test_string_buffer(void)
{
    /* Initialize ptr to NULL to make sure it actually is set in the first
     * test below. */
    HSTRING_BUFFER buf = NULL;
    WCHAR *ptr = NULL;
    HSTRING str;

    /* Test creation of an empty buffer */
    ok(WindowsPreallocateStringBuffer(0, &ptr, &buf) == S_OK, "Failed to preallocate string buffer\n");
    ok(ptr != NULL, "Empty string didn't return a buffer pointer\n");
    ok(WindowsPromoteStringBuffer(buf, &str) == S_OK, "Failed to promote string buffer\n");
    ok(str == NULL, "Empty string isn't a null string\n");
    check_string(str, L"", 0, FALSE);
    ok(WindowsDeleteString(str) == S_OK, "Failed to delete string\n");

    ok(WindowsDeleteStringBuffer(NULL) == S_OK, "Failed to delete null string buffer\n");

    /* Test creation and deletion of string buffers */
    ok(WindowsPreallocateStringBuffer(6, &ptr, &buf) == S_OK, "Failed to preallocate string buffer\n");
    ok(WindowsDeleteStringBuffer(buf) == S_OK, "Failed to delete string buffer\n");

    /* Test creation and promotion of string buffers */
    ok(WindowsPreallocateStringBuffer(6, &ptr, &buf) == S_OK, "Failed to preallocate string buffer\n");
    ok(ptr[6] == '\0', "Preallocated string buffer didn't have null termination\n");
    memcpy(ptr, input_string, 6 * sizeof(*input_string));
    ok(WindowsPromoteStringBuffer(buf, NULL) == E_POINTER, "Incorrect error handling\n");
    ok(WindowsPromoteStringBuffer(buf, &str) == S_OK, "Failed to promote string buffer\n");
    check_string(str, input_string, 6, FALSE);
    ok(WindowsDeleteString(str) == S_OK, "Failed to delete string\n");

    /* Test error handling in preallocation */
    ok(WindowsPreallocateStringBuffer(6, NULL, &buf) == E_POINTER, "Incorrect error handling\n");
    ok(WindowsPreallocateStringBuffer(6, &ptr, NULL) == E_POINTER, "Incorrect error handling\n");

    ok(WindowsPreallocateStringBuffer(6, &ptr, &buf) == S_OK, "Failed to preallocate string buffer\n");
    ptr[6] = 'a'; /* Overwrite the buffer's null termination, promotion should fail */
    ok(WindowsPromoteStringBuffer(buf, &str) == E_INVALIDARG, "Incorrect error handling\n");
    ok(WindowsDeleteStringBuffer(buf) == S_OK, "Failed to delete string buffer\n");

    /* Test strings with trailing null chars */
    ok(WindowsPreallocateStringBuffer(7, &ptr, &buf) == S_OK, "Failed to preallocate string buffer\n");
    memcpy(ptr, input_string, 7 * sizeof(*input_string));
    ok(WindowsPromoteStringBuffer(buf, &str) == S_OK, "Failed to promote string buffer\n");
    check_string(str, input_string, 7, TRUE);
    ok(WindowsDeleteString(str) == S_OK, "Failed to delete string\n");
}

static void test_substring(void)
{
    HSTRING str, substr;
    HSTRING_HEADER header;

    /* Test substring of string buffers */
    ok(WindowsCreateString(input_string, 6, &str) == S_OK, "Failed to create string\n");
    ok(WindowsSubstring(str, 2, &substr) == S_OK, "Failed to create substring\n");
    check_string(substr, output_substring, 4, FALSE);
    ok(WindowsDeleteString(substr) == S_OK, "Failed to delete string\n");
    ok(WindowsSubstringWithSpecifiedLength(str, 2, 3, &substr) == S_OK, "Failed to create substring\n");
    check_string(substr, output_substring, 3, FALSE);
    ok(WindowsDeleteString(substr) == S_OK, "Failed to delete string\n");
    ok(WindowsDeleteString(str) == S_OK, "Failed to delete string\n");

    /* Test duplication of string using substring */
    ok(WindowsCreateString(input_string, 6, &str) == S_OK, "Failed to create string\n");
    ok(WindowsSubstring(str, 0, &substr) == S_OK, "Failed to create substring\n");
    ok(str != substr, "Duplicated string didn't create new string\n");
    check_string(substr, input_string, 6, FALSE);
    ok(WindowsDeleteString(substr) == S_OK, "Failed to delete string\n");
    ok(WindowsSubstringWithSpecifiedLength(str, 0, 6, &substr) == S_OK, "Failed to create substring\n");
    ok(str != substr, "Duplicated string didn't create new string\n");
    check_string(substr, input_string, 6, FALSE);
    ok(WindowsDeleteString(substr) == S_OK, "Failed to delete string\n");
    ok(WindowsDeleteString(str) == S_OK, "Failed to delete string\n");

    /* Test substring of string reference */
    ok(WindowsCreateStringReference(input_string, 6, &header, &str) == S_OK, "Failed to create string ref\n");
    ok(WindowsSubstring(str, 2, &substr) == S_OK, "Failed to create substring of string ref\n");
    check_string(substr, output_substring, 4, FALSE);
    ok(WindowsDeleteString(substr) == S_OK, "Failed to delete string\n");
    ok(WindowsSubstringWithSpecifiedLength(str, 2, 3, &substr) == S_OK, "Failed to create substring of string ref\n");
    check_string(substr, output_substring, 3, FALSE);
    ok(WindowsDeleteString(substr) == S_OK, "Failed to delete string\n");
    ok(WindowsDeleteString(str) == S_OK, "Failed to delete string ref\n");

    /* Test duplication of string reference using substring */
    ok(WindowsCreateStringReference(input_string, 6, &header, &str) == S_OK, "Failed to create string ref\n");
    ok(WindowsSubstring(str, 0, &substr) == S_OK, "Failed to create substring of string ref\n");
    ok(str != substr, "Duplicated string ref didn't create new string\n");
    check_string(substr, input_string, 6, FALSE);
    ok(WindowsDeleteString(substr) == S_OK, "Failed to delete string\n");
    ok(WindowsSubstringWithSpecifiedLength(str, 0, 6, &substr) == S_OK, "Failed to create substring of string ref\n");
    ok(str != substr, "Duplicated string ref didn't create new string\n");
    check_string(substr, input_string, 6, FALSE);
    ok(WindowsDeleteString(substr) == S_OK, "Failed to delete string\n");
    ok(WindowsDeleteString(str) == S_OK, "Failed to delete string ref\n");

    /* Test get substring of empty string */
    ok(WindowsSubstring(NULL, 0, &substr) == S_OK, "Failed to duplicate NULL string\n");
    ok(substr == NULL, "Substring created new string\n");
    ok(WindowsSubstringWithSpecifiedLength(NULL, 0, 0, &substr) == S_OK, "Failed to duplicate NULL string\n");
    ok(substr == NULL, "Substring created new string\n");

    /* Test get empty substring of string */
    ok(WindowsCreateString(input_string, 6, &str) == S_OK, "Failed to create string\n");
    ok(WindowsSubstring(str, 6, &substr) == S_OK, "Failed to create substring\n");
    ok(substr == NULL, "Substring created new string\n");
    ok(WindowsSubstringWithSpecifiedLength(str, 6, 0, &substr) == S_OK, "Failed to create substring\n");
    ok(substr == NULL, "Substring created new string\n");
    ok(WindowsDeleteString(str) == S_OK, "Failed to delete string\n");

    /* Test handling of using too high start index or length */
    ok(WindowsCreateString(input_string, 6, &str) == S_OK, "Failed to create string\n");
    ok(WindowsSubstring(str, 7, &substr) == E_BOUNDS, "Incorrect error handling\n");
    ok(WindowsSubstringWithSpecifiedLength(str, 7, 0, &substr) == E_BOUNDS, "Incorrect error handling\n");
    ok(WindowsSubstringWithSpecifiedLength(str, 6, 1, &substr) == E_BOUNDS, "Incorrect error handling\n");
    ok(WindowsSubstringWithSpecifiedLength(str, 7, ~0U, &substr) == E_BOUNDS, "Incorrect error handling\n");
    ok(WindowsDeleteString(str) == S_OK, "Failed to delete string\n");

    /* Test handling of a NULL string  */
    ok(WindowsCreateString(input_string, 6, &str) == S_OK, "Failed to create string\n");
    ok(WindowsSubstring(str, 7, NULL) == E_INVALIDARG, "Incorrect error handling\n");
    ok(WindowsSubstringWithSpecifiedLength(str, 7, 0, NULL) == E_INVALIDARG, "Incorrect error handling\n");
    ok(WindowsDeleteString(str) == S_OK, "Failed to delete string\n");
}

static void test_concat(void)
{
    HSTRING str1, str2, concat;
    HSTRING_HEADER header1, header2;

    /* Test concatenation of string buffers */
    ok(WindowsCreateString(input_string1, 3, &str1) == S_OK, "Failed to create string\n");
    ok(WindowsCreateString(input_string2, 3, &str2) == S_OK, "Failed to create string\n");

    ok(WindowsConcatString(str1, NULL, NULL) == E_INVALIDARG, "Incorrect error handling\n");
    ok(WindowsConcatString(str1, NULL, &concat) == S_OK, "Failed to concatenate string\n");
    ok(str1 == concat, "Concatenate created new string\n");
    check_string(concat, input_string1, 3, FALSE);
    ok(WindowsDeleteString(concat) == S_OK, "Failed to delete string\n");

    ok(WindowsConcatString(NULL, str2, NULL) == E_INVALIDARG, "Incorrect error handling\n");
    ok(WindowsConcatString(NULL, str2, &concat) == S_OK, "Failed to concatenate string\n");
    ok(str2 == concat, "Concatenate created new string\n");
    check_string(concat, input_string2, 3, FALSE);
    ok(WindowsDeleteString(concat) == S_OK, "Failed to delete string\n");

    ok(WindowsConcatString(str1, str2, NULL) == E_INVALIDARG, "Incorrect error handling\n");
    ok(WindowsConcatString(str1, str2, &concat) == S_OK, "Failed to concatenate string\n");
    check_string(concat, input_string, 6, FALSE);
    ok(WindowsDeleteString(concat) == S_OK, "Failed to delete string\n");

    ok(WindowsDeleteString(str2) == S_OK, "Failed to delete string\n");
    ok(WindowsDeleteString(str1) == S_OK, "Failed to delete string\n");

    /* Test concatenation of string references */
    ok(WindowsCreateStringReference(input_string1, 3, &header1, &str1) == S_OK, "Failed to create string ref\n");
    ok(WindowsCreateStringReference(input_string2, 3, &header2, &str2) == S_OK, "Failed to create string ref\n");

    ok(WindowsConcatString(str1, NULL, &concat) == S_OK, "Failed to concatenate string\n");
    ok(str1 != concat, "Concatenate string ref didn't create new string\n");
    check_string(concat, input_string1, 3, FALSE);
    ok(WindowsDeleteString(concat) == S_OK, "Failed to delete string\n");

    ok(WindowsConcatString(NULL, str2, &concat) == S_OK, "Failed to concatenate string\n");
    ok(str2 != concat, "Concatenate string ref didn't create new string\n");
    check_string(concat, input_string2, 3, FALSE);
    ok(WindowsDeleteString(concat) == S_OK, "Failed to delete string\n");

    ok(WindowsConcatString(str1, str2, &concat) == S_OK, "Failed to concatenate string\n");
    check_string(concat, input_string, 6, FALSE);
    ok(WindowsDeleteString(concat) == S_OK, "Failed to delete string\n");

    ok(WindowsDeleteString(str2) == S_OK, "Failed to delete string ref\n");
    ok(WindowsDeleteString(str1) == S_OK, "Failed to delete string ref\n");

    /* Test concatenation of two empty strings */
    ok(WindowsConcatString(NULL, NULL, NULL) == E_INVALIDARG, "Incorrect error handling\n");
    ok(WindowsConcatString(NULL, NULL, &concat) == S_OK, "Failed to concatenate string\n");
    ok(concat == NULL, "Concatenate created new string\n");
}

static void test_compare(void)
{
    HSTRING str1, str2;
    HSTRING_HEADER header1, header2;
    INT32 res;

    /* Test comparison of string buffers */
    ok(WindowsCreateString(input_string1, 3, &str1) == S_OK, "Failed to create string\n");
    ok(WindowsCreateString(input_string2, 3, &str2) == S_OK, "Failed to create string\n");

    ok(WindowsCompareStringOrdinal(str1, str1, &res) == S_OK, "Failed to compare string\n");
    ok(res == 0, "Expected 0, got %d\n", res);
    ok(WindowsCompareStringOrdinal(str1, str2, &res) == S_OK, "Failed to compare string\n");
    ok(res == -1, "Expected -1, got %d\n", res);
    ok(WindowsCompareStringOrdinal(str2, str1, &res) == S_OK, "Failed to compare string\n");
    ok(res == 1, "Expected 1, got %d\n", res);
    ok(WindowsCompareStringOrdinal(str2, str2, &res) == S_OK, "Failed to compare string\n");
    ok(res == 0, "Expected 0, got %d\n", res);
    ok(WindowsCompareStringOrdinal(str1, NULL, &res) == S_OK, "Failed to compare string\n");
    ok(res == 1, "Expected 1, got %d\n", res);
    ok(WindowsCompareStringOrdinal(NULL, str1, &res) == S_OK, "Failed to compare string\n");
    ok(res == -1, "Expected -1, got %d\n", res);
    ok(WindowsCompareStringOrdinal(str2, NULL, &res) == S_OK, "Failed to compare string\n");
    ok(res == 1, "Expected 1, got %d\n", res);
    ok(WindowsCompareStringOrdinal(NULL, str2, &res) == S_OK, "Failed to compare string\n");
    ok(res == -1, "Expected -1, got %d\n", res);

    ok(WindowsDeleteString(str2) == S_OK, "Failed to delete string\n");
    ok(WindowsDeleteString(str1) == S_OK, "Failed to delete string\n");

    /* Test comparison of string references */
    ok(WindowsCreateStringReference(input_string1, 3, &header1, &str1) == S_OK, "Failed to create string ref\n");
    ok(WindowsCreateStringReference(input_string2, 3, &header2, &str2) == S_OK, "Failed to create string ref\n");

    ok(WindowsCompareStringOrdinal(str1, str1, &res) == S_OK, "Failed to compare string\n");
    ok(res == 0, "Expected 0, got %d\n", res);
    ok(WindowsCompareStringOrdinal(str1, str2, &res) == S_OK, "Failed to compare string\n");
    ok(res == -1, "Expected -1, got %d\n", res);
    ok(WindowsCompareStringOrdinal(str2, str1, &res) == S_OK, "Failed to compare string\n");
    ok(res == 1, "Expected 1, got %d\n", res);
    ok(WindowsCompareStringOrdinal(str2, str2, &res) == S_OK, "Failed to compare string\n");
    ok(res == 0, "Expected 0, got %d\n", res);
    ok(WindowsCompareStringOrdinal(str1, NULL, &res) == S_OK, "Failed to compare string\n");
    ok(res == 1, "Expected 1, got %d\n", res);
    ok(WindowsCompareStringOrdinal(NULL, str1, &res) == S_OK, "Failed to compare string\n");
    ok(res == -1, "Expected -1, got %d\n", res);
    ok(WindowsCompareStringOrdinal(str2, NULL, &res) == S_OK, "Failed to compare string\n");
    ok(res == 1, "Expected 1, got %d\n", res);
    ok(WindowsCompareStringOrdinal(NULL, str2, &res) == S_OK, "Failed to compare string\n");
    ok(res == -1, "Expected -1, got %d\n", res);

    ok(WindowsDeleteString(str2) == S_OK, "Failed to delete string ref\n");
    ok(WindowsDeleteString(str1) == S_OK, "Failed to delete string ref\n");

    /* Test comparison of two empty strings */
    ok(WindowsCompareStringOrdinal(NULL, NULL, NULL) == E_INVALIDARG, "Incorrect error handling\n");
    ok(WindowsCompareStringOrdinal(NULL, NULL, &res) == S_OK, "Failed to compare NULL string\n");
    ok(res == 0, "Expected 0, got %d\n", res);
}

static void test_trim(void)
{
    HSTRING str1, str2, trimmed;
    HSTRING_HEADER header1, header2;

    /* Test trimming of string buffers */
    ok(WindowsCreateString(input_string, 6, &str1) == S_OK, "Failed to create string\n");
    ok(WindowsCreateString(input_string1, 3, &str2) == S_OK, "Failed to create string\n");

    ok(WindowsTrimStringStart(str1, str2, &trimmed) == S_OK, "Failed to trim string\n");
    check_string(trimmed, input_string2, 3, FALSE);
    ok(WindowsDeleteString(trimmed) == S_OK, "Failed to delete string\n");
    ok(WindowsTrimStringEnd(str1, str2, &trimmed) == S_OK, "Failed to trim string\n");
    ok(trimmed == str1, "Trimmed string created new string\n");
    check_string(trimmed, input_string, 6, FALSE);
    ok(WindowsDeleteString(trimmed) == S_OK, "Failed to delete string\n");

    ok(WindowsDeleteString(str2) == S_OK, "Failed to delete string\n");
    ok(WindowsCreateString(input_string2, 3, &str2) == S_OK, "Failed to create string\n");

    ok(WindowsTrimStringStart(str1, str2, &trimmed) == S_OK, "Failed to trim string\n");
    ok(trimmed == str1, "Trimmed string created new string\n");
    check_string(trimmed, input_string, 6, FALSE);
    ok(WindowsDeleteString(trimmed) == S_OK, "Failed to delete string\n");
    ok(WindowsTrimStringEnd(str1, str2, &trimmed) == S_OK, "Failed to trim string\n");
    check_string(trimmed, input_string1, 3, FALSE);
    ok(WindowsDeleteString(trimmed) == S_OK, "Failed to delete string\n");

    ok(WindowsDeleteString(str2) == S_OK, "Failed to delete string\n");
    ok(WindowsDeleteString(str1) == S_OK, "Failed to delete string\n");

    /* Test trimming of string references */
    ok(WindowsCreateStringReference(input_string, 6, &header1, &str1) == S_OK, "Failed to create string ref\n");
    ok(WindowsCreateStringReference(input_string1, 3, &header2, &str2) == S_OK, "Failed to create string ref\n");

    ok(WindowsTrimStringStart(str1, str2, &trimmed) == S_OK, "Failed to trim string\n");
    check_string(trimmed, input_string2, 3, FALSE);
    ok(WindowsDeleteString(trimmed) == S_OK, "Failed to delete string\n");
    ok(WindowsTrimStringEnd(str1, str2, &trimmed) == S_OK, "Failed to trim string\n");
    ok(trimmed != str1, "Trimmed string ref didn't create new string\n");
    check_string(trimmed, input_string, 6, FALSE);
    ok(WindowsDeleteString(trimmed) == S_OK, "Failed to delete string\n");

    ok(WindowsDeleteString(str2) == S_OK, "Failed to delete string ref\n");
    ok(WindowsCreateStringReference(input_string2, 3, &header2, &str2) == S_OK, "Failed to create string ref\n");

    ok(WindowsTrimStringStart(str1, str2, &trimmed) == S_OK, "Failed to trim string\n");
    ok(trimmed != str1, "Trimmed string ref didn't create new string\n");
    check_string(trimmed, input_string, 6, FALSE);
    ok(WindowsDeleteString(trimmed) == S_OK, "Failed to delete string\n");
    ok(WindowsTrimStringEnd(str1, str2, &trimmed) == S_OK, "Failed to trim string\n");
    check_string(trimmed, input_string1, 3, FALSE);
    ok(WindowsDeleteString(trimmed) == S_OK, "Failed to delete string\n");

    ok(WindowsDeleteString(str2) == S_OK, "Failed to delete string ref\n");
    ok(WindowsDeleteString(str1) == S_OK, "Failed to delete string ref\n");

    /* Test handling of NULL strings */
    ok(WindowsCreateString(input_string, 6, &str1) == S_OK, "Failed to create string\n");
    ok(WindowsTrimStringStart(NULL, NULL, NULL) == E_INVALIDARG, "Incorrect error handling\n");
    ok(WindowsTrimStringStart(NULL, str1, NULL) == E_INVALIDARG, "Incorrect error handling\n");
    ok(WindowsTrimStringStart(NULL, NULL, &trimmed) == E_INVALIDARG, "Incorrect error handling\n");
    ok(WindowsTrimStringStart(NULL, str1, &trimmed) == S_OK, "Failed to trim empty string\n");
    ok(trimmed == NULL, "Trimming created new string\n");
    ok(WindowsTrimStringEnd(NULL, NULL, NULL) == E_INVALIDARG, "Incorrect error handling\n");
    ok(WindowsTrimStringEnd(NULL, str1, NULL) == E_INVALIDARG, "Incorrect error handling\n");
    ok(WindowsTrimStringEnd(NULL, NULL, &trimmed) == E_INVALIDARG, "Incorrect error handling\n");
    ok(WindowsTrimStringEnd(NULL, str1, &trimmed) == S_OK, "Failed to trim empty string\n");
    ok(trimmed == NULL, "Trimming created new string\n");
    ok(WindowsDeleteString(str1) == S_OK, "Failed to delete string\n");
}

static void test_hstring_struct(void)
{
    struct hstring_header
    {
        UINT32 flags;
        UINT32 length;
        UINT32 padding1;
        UINT32 padding2;
        const WCHAR *str;
    };

    struct hstring_private
    {
        struct hstring_header header;
        LONG refcount;
        WCHAR buffer[1];
    };

    HSTRING str;
    HSTRING str2;
    HSTRING_HEADER hdr;
    struct hstring_private* prv;
    struct hstring_private* prv2;

    BOOL arch64 = (sizeof(void*) == 8);

    ok(arch64 ? (sizeof(prv->header) == 24) : (sizeof(prv->header) == 20), "hstring_header size incorrect.\n");

    ok(WindowsCreateString(input_string, 6, &str) == S_OK, "Failed to create string.\n");

    prv = CONTAINING_RECORD(str, struct hstring_private, header);

    ok(prv->header.flags == 0, "Expected 0 in flags field, got %#x.\n", prv->header.flags);
    ok(prv->header.length == 6, "Expected 6 in length field, got %u.\n", prv->header.length);
    ok(prv->header.str == prv->buffer, "Expected str to point at buffer, instead pointing at %p.\n", prv->header.str);
    ok(prv->refcount == 1, "Expected 1 in refcount, got %lu.\n", prv->refcount);
    ok(wcscmp(input_string, prv->buffer) == 0, "Expected strings to match.\n");
    ok(prv->buffer[prv->header.length] == '\0', "Expected buffer to be null terminated.\n");

    ok(WindowsDuplicateString(str, &str2) == S_OK, "Failed to duplicate string.\n");

    prv2 = CONTAINING_RECORD(str2, struct hstring_private, header);

    ok(prv->refcount == 2, "Expected 2 in refcount, got %lu.\n", prv->refcount);
    ok(prv2->refcount == 2, "Expected 2 in refcount, got %lu.\n", prv2->refcount);
    ok(wcscmp(input_string, prv2->buffer) == 0, "Expected strings to match.\n");

    ok(WindowsDeleteString(str) == S_OK, "Failed to delete string.\n");

    ok(prv->refcount == 1, "Expected 1 in refcount, got %lu.\n", prv->refcount);

    ok(WindowsDeleteString(str) == S_OK, "Failed to delete string.\n");

    ok(WindowsCreateStringReference(input_string, 6, &hdr, &str) == S_OK, "Failed to create string ref.\n");

    prv = CONTAINING_RECORD(&hdr, struct hstring_private, header);
    prv2 = CONTAINING_RECORD(str, struct hstring_private, header);

    ok(prv == prv2, "Pointers not identical.\n");
    ok(prv2->header.flags == 1, "Expected HSTRING_REFERENCE_FLAG to be set, got %#x.\n", prv2->header.flags);
    ok(prv2->header.length == 6, "Expected 6 in length field, got %u.\n", prv2->header.length);
    ok(prv2->header.str == input_string, "Expected str to point at input_string, instead pointing at %p.\n", prv2->header.str);

    ok(WindowsDeleteString(str) == S_OK, "Failed to delete string ref.\n");
}

START_TEST(string)
{
    test_create_delete();
    test_duplicate();
    test_access();
    test_string_buffer();
    test_substring();
    test_concat();
    test_compare();
    test_trim();
    test_hstring_struct();
}
