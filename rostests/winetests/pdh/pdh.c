/*
 * Tests for pdh.dll (Performance Data Helper)
 *
 * Copyright 2007 Hans Leidekker
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

#include <stdio.h>

#include "windows.h"

#include "pdh.h"
#include "pdhmsg.h"

#include "wine/test.h"

static HMODULE pdh;

static PDH_STATUS   (WINAPI *pPdhAddEnglishCounterA)(PDH_HQUERY, LPCSTR, DWORD_PTR, PDH_HCOUNTER *);
static PDH_STATUS   (WINAPI *pPdhAddEnglishCounterW)(PDH_HQUERY, LPCWSTR, DWORD_PTR, PDH_HCOUNTER *);
static PDH_STATUS   (WINAPI *pPdhCollectQueryDataWithTime)(PDH_HQUERY, LONGLONG *);
static PDH_STATUS   (WINAPI *pPdhValidatePathExA)(PDH_HLOG, LPCSTR);
static PDH_STATUS   (WINAPI *pPdhValidatePathExW)(PDH_HLOG, LPCWSTR);

#define GETFUNCPTR(func) p##func = (void *)GetProcAddress( pdh, #func );

static void init_function_ptrs( void )
{
    pdh = GetModuleHandle( "pdh" );
    GETFUNCPTR( PdhAddEnglishCounterA )
    GETFUNCPTR( PdhAddEnglishCounterW )
    GETFUNCPTR( PdhCollectQueryDataWithTime )
    GETFUNCPTR( PdhValidatePathExA )
    GETFUNCPTR( PdhValidatePathExW )
}

static const WCHAR processor_time[] =
    {'%',' ','P','r','o','c','e','s','s','o','r',' ','T','i','m','e',0};
static const WCHAR uptime[] =
    {'S','y','s','t','e','m',' ','U','p',' ','T','i','m','e',0};

static const WCHAR system_uptime[] =
    {'\\','S','y','s','t','e','m','\\','S','y','s','t','e','m',' ','U','p',' ','T','i','m','e',0};
static const WCHAR system_downtime[] = /* does not exist */
    {'\\','S','y','s','t','e','m','\\','S','y','s','t','e','m',' ','D','o','w','n',' ','T','i','m','e',0};
static const WCHAR percentage_processor_time[] =
    {'\\','P','r','o','c','e','s','s','o','r','(','_','T','o','t','a','l',')',
     '\\','%',' ','P','r','o','c','e','s','s','o','r',' ','T','i','m','e',0};

static void test_PdhOpenQueryA( void )
{
    PDH_STATUS ret;
    PDH_HQUERY query;

    ret = PdhOpenQueryA( NULL, 0, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhOpenQueryA failed 0x%08x\n", ret);

    ret = PdhOpenQueryA( NULL, 0, &query );
    ok(ret == ERROR_SUCCESS, "PdhOpenQueryA failed 0x%08x\n", ret);

    ret = PdhCloseQuery( NULL );
    ok(ret == PDH_INVALID_HANDLE, "PdhCloseQuery failed 0x%08x\n", ret);

    ret = PdhCloseQuery( &query );
    ok(ret == PDH_INVALID_HANDLE, "PdhCloseQuery failed 0x%08x\n", ret);

    ret = PdhCloseQuery( query );
    ok(ret == ERROR_SUCCESS, "PdhCloseQuery failed 0x%08x\n", ret);

    ret = PdhCloseQuery( query );
    ok(ret == PDH_INVALID_HANDLE, "PdhCloseQuery failed 0x%08x\n", ret);
}

static void test_PdhOpenQueryW( void )
{
    PDH_STATUS ret;
    PDH_HQUERY query;

    ret = PdhOpenQueryW( NULL, 0, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhOpenQueryW failed 0x%08x\n", ret);

    ret = PdhOpenQueryW( NULL, 0, &query );
    ok(ret == ERROR_SUCCESS, "PdhOpenQueryW failed 0x%08x\n", ret);

    ret = PdhCloseQuery( NULL );
    ok(ret == PDH_INVALID_HANDLE, "PdhCloseQuery failed 0x%08x\n", ret);

    ret = PdhCloseQuery( &query );
    ok(ret == PDH_INVALID_HANDLE, "PdhCloseQuery failed 0x%08x\n", ret);

    ret = PdhCloseQuery( query );
    ok(ret == ERROR_SUCCESS, "PdhCloseQuery failed 0x%08x\n", ret);

    ret = PdhCloseQuery( query );
    ok(ret == PDH_INVALID_HANDLE, "PdhCloseQuery failed 0x%08x\n", ret);
}

static void test_PdhAddCounterA( void )
{
    PDH_STATUS ret;
    PDH_HQUERY query;
    PDH_HCOUNTER counter;

    ret = PdhOpenQueryA( NULL, 0, &query );
    ok(ret == ERROR_SUCCESS, "PdhOpenQueryA failed 0x%08x\n", ret);

    ret = PdhAddCounterA( NULL, "\\System\\System Up Time", 0, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhAddCounterA failed 0x%08x\n", ret);

    ret = PdhAddCounterA( NULL, "\\System\\System Up Time", 0, &counter );
    ok(ret == PDH_INVALID_HANDLE, "PdhAddCounterA failed 0x%08x\n", ret);

    ret = PdhAddCounterA( query, NULL, 0, &counter );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhAddCounterA failed 0x%08x\n", ret);

    ret = PdhAddCounterA( query, "\\System\\System Up Time", 0, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhAddCounterA failed 0x%08x\n", ret);

    ret = PdhAddCounterA( query, "\\System\\System Down Time", 0, &counter );
    ok(ret == PDH_CSTATUS_NO_COUNTER, "PdhAddCounterA failed 0x%08x\n", ret);
    ok(!counter, "PdhAddCounterA failed %p\n", counter);

    ret = PdhAddCounterA( query, "\\System\\System Up Time", 0, &counter );
    ok(ret == ERROR_SUCCESS, "PdhAddCounterA failed 0x%08x\n", ret);

    ret = PdhCollectQueryData( NULL );
    ok(ret == PDH_INVALID_HANDLE, "PdhCollectQueryData failed 0x%08x\n", ret);

    ret = PdhCollectQueryData( counter );
    ok(ret == PDH_INVALID_HANDLE, "PdhCollectQueryData failed 0x%08x\n", ret);

    ret = PdhCollectQueryData( query );
    ok(ret == ERROR_SUCCESS, "PdhCollectQueryData failed 0x%08x\n", ret);

    ret = PdhRemoveCounter( NULL );
    ok(ret == PDH_INVALID_HANDLE, "PdhRemoveCounter failed 0x%08x\n", ret);

    ret = PdhRemoveCounter( counter );
    ok(ret == ERROR_SUCCESS, "PdhRemoveCounter failed 0x%08x\n", ret);

    ret = PdhCloseQuery( query );
    ok(ret == ERROR_SUCCESS, "PdhCloseQuery failed 0x%08x\n", ret);
}

static void test_PdhAddCounterW( void )
{
    PDH_STATUS ret;
    PDH_HQUERY query;
    PDH_HCOUNTER counter;

    ret = PdhOpenQueryW( NULL, 0, &query );
    ok(ret == ERROR_SUCCESS, "PdhOpenQueryW failed 0x%08x\n", ret);

    ret = PdhAddCounterW( NULL, percentage_processor_time, 0, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhAddCounterW failed 0x%08x\n", ret);

    ret = PdhAddCounterW( NULL, percentage_processor_time, 0, &counter );
    ok(ret == PDH_INVALID_HANDLE, "PdhAddCounterW failed 0x%08x\n", ret);

    ret = PdhAddCounterW( query, NULL, 0, &counter );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhAddCounterW failed 0x%08x\n", ret);

    ret = PdhAddCounterW( query, percentage_processor_time, 0, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhAddCounterW failed 0x%08x\n", ret);

    ret = PdhAddCounterW( query, system_downtime, 0, &counter );
    ok(ret == PDH_CSTATUS_NO_COUNTER, "PdhAddCounterW failed 0x%08x\n", ret);
    ok(!counter, "PdhAddCounterW failed %p\n", counter);

    ret = PdhAddCounterW( query, percentage_processor_time, 0, &counter );
    ok(ret == ERROR_SUCCESS, "PdhAddCounterW failed 0x%08x\n", ret);

    ret = PdhCollectQueryData( NULL );
    ok(ret == PDH_INVALID_HANDLE, "PdhCollectQueryData failed 0x%08x\n", ret);

    ret = PdhCollectQueryData( counter );
    ok(ret == PDH_INVALID_HANDLE, "PdhCollectQueryData failed 0x%08x\n", ret);

    ret = PdhCollectQueryData( query );
    ok(ret == ERROR_SUCCESS, "PdhCollectQueryData failed 0x%08x\n", ret);

    ret = PdhRemoveCounter( NULL );
    ok(ret == PDH_INVALID_HANDLE, "PdhRemoveCounter failed 0x%08x\n", ret);

    ret = PdhRemoveCounter( counter );
    ok(ret == ERROR_SUCCESS, "PdhRemoveCounter failed 0x%08x\n", ret);

    ret = PdhCloseQuery( query );
    ok(ret == ERROR_SUCCESS, "PdhCloseQuery failed 0x%08x\n", ret);
}

static void test_PdhAddEnglishCounterA( void )
{
    PDH_STATUS ret;
    PDH_HQUERY query;
    PDH_HCOUNTER counter;

    ret = PdhOpenQueryA( NULL, 0, &query );
    ok(ret == ERROR_SUCCESS, "PdhOpenQueryA failed 0x%08x\n", ret);

    ret = PdhCollectQueryData( query );
    ok(ret == PDH_NO_DATA, "PdhCollectQueryData failed 0x%08x\n", ret);

    ret = pPdhAddEnglishCounterA( NULL, "\\System\\System Up Time", 0, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhAddEnglishCounterA failed 0x%08x\n", ret);

    ret = pPdhAddEnglishCounterA( NULL, "\\System\\System Up Time", 0, &counter );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhAddEnglishCounterA failed 0x%08x\n", ret);

    ret = pPdhAddEnglishCounterA( query, NULL, 0, &counter );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhAddEnglishCounterA failed 0x%08x\n", ret);

    ret = pPdhAddEnglishCounterA( query, "\\System\\System Up Time", 0, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhAddEnglishCounterA failed 0x%08x\n", ret);

    ret = pPdhAddEnglishCounterA( query, "\\System\\System Down Time", 0, &counter );
    ok(ret == PDH_CSTATUS_NO_COUNTER, "PdhAddEnglishCounterA failed 0x%08x\n", ret);
    ok(!counter, "PdhAddEnglishCounterA failed %p\n", counter);

    ret = pPdhAddEnglishCounterA( query, "\\System\\System Up Time", 0, &counter );
    ok(ret == ERROR_SUCCESS, "PdhAddEnglishCounterA failed 0x%08x\n", ret);

    ret = PdhCollectQueryData( query );
    ok(ret == ERROR_SUCCESS, "PdhCollectQueryData failed 0x%08x\n", ret);

    ret = PdhRemoveCounter( counter );
    ok(ret == ERROR_SUCCESS, "PdhRemoveCounter failed 0x%08x\n", ret);

    ret = PdhCloseQuery( query );
    ok(ret == ERROR_SUCCESS, "PdhCloseQuery failed 0x%08x\n", ret);
}

static void test_PdhAddEnglishCounterW( void )
{
    PDH_STATUS ret;
    PDH_HQUERY query;
    PDH_HCOUNTER counter;

    ret = PdhOpenQueryW( NULL, 0, &query );
    ok(ret == ERROR_SUCCESS, "PdhOpenQueryW failed 0x%08x\n", ret);

    ret = PdhCollectQueryData( query );
    ok(ret == PDH_NO_DATA, "PdhCollectQueryData failed 0x%08x\n", ret);

    ret = pPdhAddEnglishCounterW( NULL, system_uptime, 0, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhAddEnglishCounterW failed 0x%08x\n", ret);

    ret = pPdhAddEnglishCounterW( NULL, system_uptime, 0, &counter );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhAddEnglishCounterW failed 0x%08x\n", ret);

    ret = pPdhAddEnglishCounterW( query, NULL, 0, &counter );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhAddEnglishCounterW failed 0x%08x\n", ret);

    ret = pPdhAddEnglishCounterW( query, system_uptime, 0, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhAddEnglishCounterW failed 0x%08x\n", ret);

    ret = pPdhAddEnglishCounterW( query, system_downtime, 0, &counter );
    ok(ret == PDH_CSTATUS_NO_COUNTER, "PdhAddEnglishCounterW failed 0x%08x\n", ret);
    ok(!counter, "PdhAddEnglishCounterA failed %p\n", counter);

    ret = pPdhAddEnglishCounterW( query, system_uptime, 0, &counter );
    ok(ret == ERROR_SUCCESS, "PdhAddEnglishCounterW failed 0x%08x\n", ret);

    ret = PdhCollectQueryData( query );
    ok(ret == ERROR_SUCCESS, "PdhCollectQueryData failed 0x%08x\n", ret);

    ret = PdhRemoveCounter( counter );
    ok(ret == ERROR_SUCCESS, "PdhRemoveCounter failed 0x%08x\n", ret);

    ret = PdhCloseQuery( query );
    ok(ret == ERROR_SUCCESS, "PdhCloseQuery failed 0x%08x\n", ret);
}

static void test_PdhCollectQueryDataWithTime( void )
{
    PDH_STATUS ret;
    PDH_HQUERY query;
    PDH_HCOUNTER counter;
    LONGLONG time;

    ret = PdhOpenQueryA( NULL, 0, &query );
    ok(ret == ERROR_SUCCESS, "PdhOpenQueryA failed 0x%08x\n", ret);

    ret = PdhCollectQueryData( query );
    ok(ret == PDH_NO_DATA, "PdhCollectQueryData failed 0x%08x\n", ret);

    ret = PdhAddCounterA( query, "\\System\\System Up Time", 0, &counter );
    ok(ret == ERROR_SUCCESS, "PdhAddCounterA failed 0x%08x\n", ret);

    ret = pPdhCollectQueryDataWithTime( NULL, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhCollectQueryDataWithTime failed 0x%08x\n", ret);

    ret = pPdhCollectQueryDataWithTime( query, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhCollectQueryDataWithTime failed 0x%08x\n", ret);

    ret = pPdhCollectQueryDataWithTime( NULL, &time );
    ok(ret == PDH_INVALID_HANDLE, "PdhCollectQueryDataWithTime failed 0x%08x\n", ret);

    ret = pPdhCollectQueryDataWithTime( query, &time );
    ok(ret == ERROR_SUCCESS, "PdhCollectQueryDataWithTime failed 0x%08x\n", ret);

    ret = PdhCloseQuery( query );
    ok(ret == ERROR_SUCCESS, "PdhCloseQuery failed 0x%08x\n", ret);
}

static void test_PdhGetFormattedCounterValue( void )
{
    PDH_STATUS ret;
    PDH_HQUERY query;
    PDH_HCOUNTER counter;
    PDH_FMT_COUNTERVALUE value;

    ret = PdhOpenQueryA( NULL, 0, &query );
    ok(ret == ERROR_SUCCESS, "PdhOpenQueryA failed 0x%08x\n", ret);

    ret = PdhAddCounterA( query, "\\System\\System Up Time", 0, &counter );
    ok(ret == ERROR_SUCCESS, "PdhAddCounterA failed 0x%08x\n", ret);

    ret = PdhGetFormattedCounterValue( NULL, PDH_FMT_LARGE, NULL, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhGetFormattedCounterValue failed 0x%08x\n", ret);

    ret = PdhGetFormattedCounterValue( NULL, PDH_FMT_LARGE, NULL, &value );
    ok(ret == PDH_INVALID_HANDLE, "PdhGetFormattedCounterValue failed 0x%08x\n", ret);

    ret = PdhGetFormattedCounterValue( counter, PDH_FMT_LARGE, NULL, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhGetFormattedCounterValue failed 0x%08x\n", ret);

    ret = PdhGetFormattedCounterValue( counter, PDH_FMT_LARGE, NULL, &value );
    ok(ret == ERROR_SUCCESS, "PdhGetFormattedCounterValue failed 0x%08x\n", ret);

    ret = PdhCollectQueryData( query );
    ok(ret == ERROR_SUCCESS, "PdhCollectQueryData failed 0x%08x\n", ret);

    ret = PdhGetFormattedCounterValue( counter, PDH_FMT_LARGE, NULL, &value );
    ok(ret == ERROR_SUCCESS, "PdhGetFormattedCounterValue failed 0x%08x\n", ret);

    ret = PdhGetFormattedCounterValue( counter, PDH_FMT_LARGE | PDH_FMT_NOSCALE, NULL, &value );
    ok(ret == ERROR_SUCCESS, "PdhGetFormattedCounterValue failed 0x%08x\n", ret);

    ret = PdhGetFormattedCounterValue( counter, PDH_FMT_LARGE | PDH_FMT_NOCAP100, NULL, &value );
    ok(ret == ERROR_SUCCESS, "PdhGetFormattedCounterValue failed 0x%08x\n", ret);

    ret = PdhGetFormattedCounterValue( counter, PDH_FMT_LARGE | PDH_FMT_1000, NULL, &value );
    ok(ret == ERROR_SUCCESS, "PdhGetFormattedCounterValue failed 0x%08x\n", ret);

    ret = PdhSetCounterScaleFactor( counter, 2 );
    ok(ret == ERROR_SUCCESS, "PdhSetCounterScaleFactor failed 0x%08x\n", ret);

    ret = PdhGetFormattedCounterValue( counter, PDH_FMT_LARGE, NULL, &value );
    ok(ret == ERROR_SUCCESS, "PdhGetFormattedCounterValue failed 0x%08x\n", ret);

    ret = PdhCloseQuery( query );
    ok(ret == ERROR_SUCCESS, "PdhCloseQuery failed 0x%08x\n", ret);
}

static void test_PdhGetRawCounterValue( void )
{
    PDH_STATUS ret;
    PDH_HQUERY query;
    PDH_HCOUNTER counter;
    PDH_RAW_COUNTER value;

    ret = PdhOpenQueryA( NULL, 0, &query );
    ok(ret == ERROR_SUCCESS, "PdhOpenQueryA failed 0x%08x\n", ret);

    ret = PdhAddCounterA( query, "\\System\\System Up Time", 0, &counter );
    ok(ret == ERROR_SUCCESS, "PdhAddCounterA failed 0x%08x\n", ret);

    ret = PdhGetRawCounterValue( NULL, NULL, &value );
    ok(ret == PDH_INVALID_HANDLE, "PdhGetRawCounterValue failed 0x%08x\n", ret);

    ret = PdhGetRawCounterValue( counter, NULL, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhGetRawCounterValue failed 0x%08x\n", ret);

    ret = PdhGetRawCounterValue( counter, NULL, &value );
    ok(ret == ERROR_SUCCESS, "PdhGetRawCounterValue failed 0x%08x\n", ret);
    ok(value.CStatus == ERROR_SUCCESS, "expected ERROR_SUCCESS got %x\n", value.CStatus);

    ret = PdhCollectQueryData( query );
    ok(ret == ERROR_SUCCESS, "PdhCollectQueryData failed 0x%08x\n", ret);

    ret = PdhGetRawCounterValue( counter, NULL, &value );
    ok(ret == ERROR_SUCCESS, "PdhGetRawCounterValue failed 0x%08x\n", ret);
    ok(value.CStatus == ERROR_SUCCESS, "expected ERROR_SUCCESS got %x\n", value.CStatus);

    ret = PdhCloseQuery( query );
    ok(ret == ERROR_SUCCESS, "PdhCloseQuery failed 0x%08x\n", ret);
}

static void test_PdhSetCounterScaleFactor( void )
{
    PDH_STATUS ret;
    PDH_HQUERY query;
    PDH_HCOUNTER counter;

    ret = PdhOpenQueryA( NULL, 0, &query );
    ok(ret == ERROR_SUCCESS, "PdhOpenQueryA failed 0x%08x\n", ret);

    ret = PdhAddCounterA( query, "\\System\\System Up Time", 0, &counter );
    ok(ret == ERROR_SUCCESS, "PdhAddCounterA failed 0x%08x\n", ret);

    ret = PdhSetCounterScaleFactor( NULL, 8 );
    ok(ret == PDH_INVALID_HANDLE, "PdhSetCounterScaleFactor failed 0x%08x\n", ret);

    ret = PdhSetCounterScaleFactor( NULL, 1 );
    ok(ret == PDH_INVALID_HANDLE, "PdhSetCounterScaleFactor failed 0x%08x\n", ret);

    ret = PdhSetCounterScaleFactor( counter, 8 );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhSetCounterScaleFactor failed 0x%08x\n", ret);

    ret = PdhSetCounterScaleFactor( counter, -8 );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhSetCounterScaleFactor failed 0x%08x\n", ret);

    ret = PdhSetCounterScaleFactor( counter, 7 );
    ok(ret == ERROR_SUCCESS, "PdhSetCounterScaleFactor failed 0x%08x\n", ret);

    ret = PdhSetCounterScaleFactor( counter, 0 );
    ok(ret == ERROR_SUCCESS, "PdhSetCounterScaleFactor failed 0x%08x\n", ret);

    ret = PdhCloseQuery( query );
    ok(ret == ERROR_SUCCESS, "PdhCloseQuery failed 0x%08x\n", ret);
}

static void test_PdhGetCounterTimeBase( void )
{
    PDH_STATUS ret;
    PDH_HQUERY query;
    PDH_HCOUNTER counter;
    LONGLONG base;

    ret = PdhOpenQueryA( NULL, 0, &query );
    ok(ret == ERROR_SUCCESS, "PdhOpenQueryA failed 0x%08x\n", ret);

    ret = PdhAddCounterA( query, "\\System\\System Up Time", 0, &counter );
    ok(ret == ERROR_SUCCESS, "PdhAddCounterA failed 0x%08x\n", ret);

    ret = PdhGetCounterTimeBase( NULL, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhGetCounterTimeBase failed 0x%08x\n", ret);

    ret = PdhGetCounterTimeBase( NULL, &base );
    ok(ret == PDH_INVALID_HANDLE, "PdhGetCounterTimeBase failed 0x%08x\n", ret);

    ret = PdhGetCounterTimeBase( counter, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhGetCounterTimeBase failed 0x%08x\n", ret);

    ret = PdhGetCounterTimeBase( counter, &base );
    ok(ret == ERROR_SUCCESS, "PdhGetCounterTimeBase failed 0x%08x\n", ret);

    ret = PdhCloseQuery( query );
    ok(ret == ERROR_SUCCESS, "PdhCloseQuery failed 0x%08x\n", ret);
}

static void test_PdhGetCounterInfoA( void )
{
    PDH_STATUS ret;
    PDH_HQUERY query;
    PDH_HCOUNTER counter;
    PDH_COUNTER_INFO_A info;
    DWORD size;

    ret = PdhOpenQueryA( NULL, 0, &query );
    ok(ret == ERROR_SUCCESS, "PdhOpenQueryA failed 0x%08x\n", ret);

    ret = PdhAddCounterA( query, "\\System\\System Up Time", 0, &counter );
    ok(ret == ERROR_SUCCESS, "PdhAddCounterA failed 0x%08x\n", ret);

    ret = PdhGetCounterInfoA( NULL, 0, NULL, NULL );
    ok(ret == PDH_INVALID_HANDLE || ret == PDH_INVALID_ARGUMENT, "PdhGetCounterInfoA failed 0x%08x\n", ret);

    ret = PdhGetCounterInfoA( counter, 0, NULL, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhGetCounterInfoA failed 0x%08x\n", ret);

    ret = PdhGetCounterInfoA( counter, 0, NULL, &info );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhGetCounterInfoA failed 0x%08x\n", ret);

    size = sizeof(info) - 1;
    ret = PdhGetCounterInfoA( counter, 0, &size, NULL );
    ok(ret == PDH_MORE_DATA || ret == PDH_INVALID_ARGUMENT, "PdhGetCounterInfoA failed 0x%08x\n", ret);

    size = sizeof(info);
    ret = PdhGetCounterInfoA( counter, 0, &size, &info );
    ok(ret == ERROR_SUCCESS, "PdhGetCounterInfoA failed 0x%08x\n", ret);
    ok(size == sizeof(info), "PdhGetCounterInfoA failed %d\n", size);

    ret = PdhGetCounterInfoA( counter, 0, &size, &info );
    ok(ret == ERROR_SUCCESS, "PdhGetCounterInfoA failed 0x%08x\n", ret);
    ok(info.lScale == 0, "lScale %d\n", info.lScale);

    ret = PdhSetCounterScaleFactor( counter, 0 );
    ok(ret == ERROR_SUCCESS, "PdhSetCounterScaleFactor failed 0x%08x\n", ret);

    ret = PdhGetCounterInfoA( counter, 0, &size, &info );
    ok(ret == ERROR_SUCCESS, "PdhGetCounterInfoA failed 0x%08x\n", ret);
    ok(info.lScale == 0, "lScale %d\n", info.lScale);

    ret = PdhSetCounterScaleFactor( counter, -5 );
    ok(ret == ERROR_SUCCESS, "PdhSetCounterScaleFactor failed 0x%08x\n", ret);

    ret = PdhGetCounterInfoA( counter, 0, &size, &info );
    ok(ret == ERROR_SUCCESS, "PdhGetCounterInfoA failed 0x%08x\n", ret);
    ok(info.lScale == -5, "lScale %d\n", info.lScale);

    ret = PdhCloseQuery( query );
    ok(ret == ERROR_SUCCESS, "PdhCloseQuery failed 0x%08x\n", ret);
}

static void test_PdhGetCounterInfoW( void )
{
    PDH_STATUS ret;
    PDH_HQUERY query;
    PDH_HCOUNTER counter;
    PDH_COUNTER_INFO_W info;
    DWORD size;

    ret = PdhOpenQueryW( NULL, 0, &query );
    ok(ret == ERROR_SUCCESS, "PdhOpenQueryW failed 0x%08x\n", ret);

    ret = PdhAddCounterW( query, percentage_processor_time, 0, &counter );
    ok(ret == ERROR_SUCCESS, "PdhAddCounterW failed 0x%08x\n", ret);

    ret = PdhGetCounterInfoW( NULL, 0, NULL, NULL );
    ok(ret == PDH_INVALID_HANDLE || ret == PDH_INVALID_ARGUMENT, "PdhGetCounterInfoW failed 0x%08x\n", ret);

    ret = PdhGetCounterInfoW( counter, 0, NULL, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhGetCounterInfoW failed 0x%08x\n", ret);

    ret = PdhGetCounterInfoW( counter, 0, NULL, &info );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhGetCounterInfoW failed 0x%08x\n", ret);

    size = sizeof(info) - 1;
    ret = PdhGetCounterInfoW( counter, 0, &size, NULL );
    ok(ret == PDH_MORE_DATA || ret == PDH_INVALID_ARGUMENT, "PdhGetCounterInfoW failed 0x%08x\n", ret);

    size = sizeof(info);
    ret = PdhGetCounterInfoW( counter, 0, &size, &info );
    ok(ret == ERROR_SUCCESS, "PdhGetCounterInfoW failed 0x%08x\n", ret);
    ok(size == sizeof(info), "PdhGetCounterInfoW failed %d\n", size);

    ret = PdhGetCounterInfoW( counter, 0, &size, &info );
    ok(ret == ERROR_SUCCESS, "PdhGetCounterInfoW failed 0x%08x\n", ret);
    ok(info.lScale == 0, "lScale %d\n", info.lScale);

    ret = PdhSetCounterScaleFactor( counter, 0 );
    ok(ret == ERROR_SUCCESS, "PdhSetCounterScaleFactor failed 0x%08x\n", ret);

    ret = PdhGetCounterInfoW( counter, 0, &size, &info );
    ok(ret == ERROR_SUCCESS, "PdhGetCounterInfoW failed 0x%08x\n", ret);
    ok(info.lScale == 0, "lScale %d\n", info.lScale);

    ret = PdhSetCounterScaleFactor( counter, -5 );
    ok(ret == ERROR_SUCCESS, "PdhSetCounterScaleFactor failed 0x%08x\n", ret);

    ret = PdhGetCounterInfoW( counter, 0, &size, &info );
    ok(ret == ERROR_SUCCESS, "PdhGetCounterInfoW failed 0x%08x\n", ret);
    ok(info.lScale == -5, "lScale %d\n", info.lScale);

    ret = PdhCloseQuery( query );
    ok(ret == ERROR_SUCCESS, "PdhCloseQuery failed 0x%08x\n", ret);
}

static void test_PdhLookupPerfIndexByNameA( void )
{
    PDH_STATUS ret;
    DWORD index;

    ret = PdhLookupPerfIndexByNameA( NULL, NULL, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhLookupPerfIndexByNameA failed 0x%08x\n", ret);

    ret = PdhLookupPerfIndexByNameA( NULL, NULL, &index );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhLookupPerfIndexByNameA failed 0x%08x\n", ret);

    ret = PdhLookupPerfIndexByNameA( NULL, "No Counter", &index );
    ok(ret == PDH_STRING_NOT_FOUND, "PdhLookupPerfIndexByNameA failed 0x%08x\n", ret);

    ret = PdhLookupPerfIndexByNameA( NULL, "% Processor Time", NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhLookupPerfIndexByNameA failed 0x%08x\n", ret);

    ret = PdhLookupPerfIndexByNameA( NULL, "% Processor Time", &index );
    ok(ret == ERROR_SUCCESS, "PdhLookupPerfIndexByNameA failed 0x%08x\n", ret);
    ok(index == 6, "PdhLookupPerfIndexByNameA failed %d\n", index);

    ret = PdhLookupPerfIndexByNameA( NULL, "System Up Time", &index );
    ok(ret == ERROR_SUCCESS, "PdhLookupPerfIndexByNameA failed 0x%08x\n", ret);
    ok(index == 674, "PdhLookupPerfIndexByNameA failed %d\n", index);
}

static void test_PdhLookupPerfIndexByNameW( void )
{
    PDH_STATUS ret;
    DWORD index;

    static const WCHAR no_counter[] = {'N','o',' ','C','o','u','n','t','e','r',0};

    ret = PdhLookupPerfIndexByNameW( NULL, NULL, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhLookupPerfIndexByNameW failed 0x%08x\n", ret);

    ret = PdhLookupPerfIndexByNameW( NULL, NULL, &index );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhLookupPerfIndexByNameW failed 0x%08x\n", ret);

    ret = PdhLookupPerfIndexByNameW( NULL, no_counter, &index );
    ok(ret == PDH_STRING_NOT_FOUND, "PdhLookupPerfIndexByNameW failed 0x%08x\n", ret);

    ret = PdhLookupPerfIndexByNameW( NULL, processor_time, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhLookupPerfIndexByNameW failed 0x%08x\n", ret);

    ret = PdhLookupPerfIndexByNameW( NULL, processor_time, &index );
    ok(ret == ERROR_SUCCESS, "PdhLookupPerfIndexByNameW failed 0x%08x\n", ret);
    ok(index == 6, "PdhLookupPerfIndexByNameW failed %d\n", index);

    ret = PdhLookupPerfIndexByNameW( NULL, uptime, &index );
    ok(ret == ERROR_SUCCESS, "PdhLookupPerfIndexByNameW failed 0x%08x\n", ret);
    ok(index == 674, "PdhLookupPerfIndexByNameW failed %d\n", index);
}

static void test_PdhLookupPerfNameByIndexA( void )
{
    PDH_STATUS ret;
    char buffer[PDH_MAX_COUNTER_NAME] = "!!";
    DWORD size;

    ret = PdhLookupPerfNameByIndexA( NULL, 0, NULL, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhLookupPerfNameByIndexA failed 0x%08x\n", ret);

    size = 0;
    ret = PdhLookupPerfNameByIndexA( NULL, 6, buffer, &size );
    ok(ret == PDH_MORE_DATA || ret == PDH_INSUFFICIENT_BUFFER, "PdhLookupPerfNameByIndexA failed 0x%08x\n", ret);

    size = sizeof(buffer);
    ret = PdhLookupPerfNameByIndexA( NULL, 6, buffer, &size );
    ok(ret == ERROR_SUCCESS, "PdhLookupPerfNameByIndexA failed 0x%08x\n", ret);
    ok(!lstrcmpA( buffer, "% Processor Time" ),
       "PdhLookupPerfNameByIndexA failed, got %s expected \'%% Processor Time\'\n", buffer);
    ok(size == sizeof("% Processor Time"), "PdhLookupPerfNameByIndexA failed %d\n", size);

    size = sizeof(buffer);
    ret = PdhLookupPerfNameByIndexA( NULL, 674, NULL, &size );
    ok(ret == PDH_INVALID_ARGUMENT ||
       ret == PDH_MORE_DATA, /* win2k3 */
       "PdhLookupPerfNameByIndexA failed 0x%08x\n", ret);

    size = sizeof(buffer);
    ret = PdhLookupPerfNameByIndexA( NULL, 674, buffer, &size );
    ok(ret == ERROR_SUCCESS, "PdhLookupPerfNameByIndexA failed 0x%08x\n", ret);
    ok(!lstrcmpA( buffer, "System Up Time" ),
       "PdhLookupPerfNameByIndexA failed, got %s expected \'System Up Time\'\n", buffer);
    ok(size == sizeof("System Up Time"), "PdhLookupPerfNameByIndexA failed %d\n", size);
}

static void test_PdhLookupPerfNameByIndexW( void )
{
    PDH_STATUS ret;
    WCHAR buffer[PDH_MAX_COUNTER_NAME];
    DWORD size;

    ret = PdhLookupPerfNameByIndexW( NULL, 0, NULL, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhLookupPerfNameByIndexW failed 0x%08x\n", ret);

    size = 0;
    ret = PdhLookupPerfNameByIndexW( NULL, 6, buffer, &size );
    ok(ret == PDH_MORE_DATA || ret == PDH_INSUFFICIENT_BUFFER, "PdhLookupPerfNameByIndexW failed 0x%08x\n", ret);

    size = sizeof(buffer) / sizeof(WCHAR);
    ret = PdhLookupPerfNameByIndexW( NULL, 6, buffer, &size );
    ok(ret == ERROR_SUCCESS, "PdhLookupPerfNameByIndexW failed 0x%08x\n", ret);
    ok(size == sizeof(processor_time) / sizeof(WCHAR), "PdhLookupPerfNameByIndexW failed %d\n", size);

    size = sizeof(buffer) / sizeof(WCHAR);
    ret = PdhLookupPerfNameByIndexW( NULL, 674, NULL, &size );
    ok(ret == PDH_INVALID_ARGUMENT ||
       ret == PDH_MORE_DATA, /* win2k3 */
       "PdhLookupPerfNameByIndexW failed 0x%08x\n", ret);

    size = sizeof(buffer) / sizeof(WCHAR);
    ret = PdhLookupPerfNameByIndexW( NULL, 674, buffer, &size );
    ok(ret == ERROR_SUCCESS, "PdhLookupPerfNameByIndexW failed 0x%08x\n", ret);
    ok(size == sizeof(uptime) / sizeof(WCHAR), "PdhLookupPerfNameByIndexW failed %d\n", size);
}

static void test_PdhValidatePathA( void )
{
    PDH_STATUS ret;

    ret = PdhValidatePathA( NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhValidatePathA failed 0x%08x\n", ret);

    ret = PdhValidatePathA( "" );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhValidatePathA failed 0x%08x\n", ret);

    ret = PdhValidatePathA( "\\System" );
    ok(ret == PDH_CSTATUS_BAD_COUNTERNAME, "PdhValidatePathA failed 0x%08x\n", ret);

    ret = PdhValidatePathA( "System Up Time" );
    ok(ret == PDH_CSTATUS_BAD_COUNTERNAME, "PdhValidatePathA failed 0x%08x\n", ret);

    ret = PdhValidatePathA( "\\System\\System Down Time" );
    ok(ret == PDH_CSTATUS_NO_COUNTER, "PdhValidatePathA failed 0x%08x\n", ret);

    ret = PdhValidatePathA( "\\System\\System Up Time" );
    ok(ret == ERROR_SUCCESS, "PdhValidatePathA failed 0x%08x\n", ret);
}

static void test_PdhValidatePathW( void )
{
    PDH_STATUS ret;

    static const WCHAR empty[] = {0};
    static const WCHAR system[] = {'\\','S','y','s','t','e','m',0};

    ret = PdhValidatePathW( NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhValidatePathW failed 0x%08x\n", ret);

    ret = PdhValidatePathW( empty );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhValidatePathW failed 0x%08x\n", ret);

    ret = PdhValidatePathW( system );
    ok(ret == PDH_CSTATUS_BAD_COUNTERNAME, "PdhValidatePathW failed 0x%08x\n", ret);

    ret = PdhValidatePathW( uptime );
    ok(ret == PDH_CSTATUS_BAD_COUNTERNAME, "PdhValidatePathW failed 0x%08x\n", ret);

    ret = PdhValidatePathW( system_downtime );
    ok(ret == PDH_CSTATUS_NO_COUNTER, "PdhValidatePathW failed 0x%08x\n", ret);

    ret = PdhValidatePathW( system_uptime );
    ok(ret == ERROR_SUCCESS, "PdhValidatePathW failed 0x%08x\n", ret);
}

static void test_PdhValidatePathExA( void )
{
    PDH_STATUS ret;

    ret = pPdhValidatePathExA( NULL, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhValidatePathExA failed 0x%08x\n", ret);

    ret = pPdhValidatePathExA( NULL, "" );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhValidatePathExA failed 0x%08x\n", ret);

    ret = pPdhValidatePathExA( NULL, "\\System" );
    ok(ret == PDH_CSTATUS_BAD_COUNTERNAME, "PdhValidatePathExA failed 0x%08x\n", ret);

    ret = pPdhValidatePathExA( NULL, "System Up Time" );
    ok(ret == PDH_CSTATUS_BAD_COUNTERNAME, "PdhValidatePathExA failed 0x%08x\n", ret);

    ret = pPdhValidatePathExA( NULL, "\\System\\System Down Time" );
    ok(ret == PDH_CSTATUS_NO_COUNTER, "PdhValidatePathExA failed 0x%08x\n", ret);

    ret = pPdhValidatePathExA( NULL, "\\System\\System Up Time" );
    ok(ret == ERROR_SUCCESS, "PdhValidatePathExA failed 0x%08x\n", ret);
}

static void test_PdhValidatePathExW( void )
{
    PDH_STATUS ret;

    static const WCHAR empty[] = {0};
    static const WCHAR system[] = {'\\','S','y','s','t','e','m',0};

    ret = pPdhValidatePathExW( NULL, NULL );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhValidatePathExW failed 0x%08x\n", ret);

    ret = pPdhValidatePathExW( NULL, empty );
    ok(ret == PDH_INVALID_ARGUMENT, "PdhValidatePathExW failed 0x%08x\n", ret);

    ret = pPdhValidatePathExW( NULL, system );
    ok(ret == PDH_CSTATUS_BAD_COUNTERNAME, "PdhValidatePathExW failed 0x%08x\n", ret);

    ret = pPdhValidatePathExW( NULL, uptime );
    ok(ret == PDH_CSTATUS_BAD_COUNTERNAME, "PdhValidatePathExW failed 0x%08x\n", ret);

    ret = pPdhValidatePathExW( NULL, system_downtime );
    ok(ret == PDH_CSTATUS_NO_COUNTER, "PdhValidatePathExW failed 0x%08x\n", ret);

    ret = pPdhValidatePathExW( NULL, system_uptime );
    ok(ret == ERROR_SUCCESS, "PdhValidatePathExW failed 0x%08x\n", ret);
}

static void test_PdhCollectQueryDataEx(void)
{
    PDH_STATUS status;
    PDH_HQUERY query;
    PDH_HCOUNTER counter;
    HANDLE event;
    BOOL ret;
    UINT i;

    status = PdhOpenQueryA( NULL, 0, &query );
    ok(status == ERROR_SUCCESS, "PdhOpenQuery failed 0x%08x\n", status);

    event = CreateEventA( NULL, FALSE, FALSE, "winetest" );
    ok(event != NULL, "CreateEvent failed\n");

    status = PdhAddCounterA( query, "\\System\\System Up Time", 0, &counter );
    ok(status == ERROR_SUCCESS, "PdhAddCounterA failed 0x%08x\n", status);

    status = PdhCollectQueryDataEx( NULL, 1, event );
    ok(status == PDH_INVALID_HANDLE, "PdhCollectQueryDataEx failed 0x%08x\n", status);

    status = PdhCollectQueryDataEx( query, 1, NULL );
    ok(status == ERROR_SUCCESS, "PdhCollectQueryDataEx failed 0x%08x\n", status);

    status = PdhCollectQueryDataEx( query, 1, event );
    ok(status == ERROR_SUCCESS, "PdhCollectQueryDataEx failed 0x%08x\n", status);

    status = PdhCollectQueryData( query );
    ok(status == ERROR_SUCCESS, "PdhCollectQueryData failed 0x%08x\n", status);

    for (i = 0; i < 3; i++)
    {
        if (WaitForSingleObject( event, INFINITE ) == WAIT_OBJECT_0)
        {
            PDH_FMT_COUNTERVALUE value;

            status = PdhGetFormattedCounterValue( counter, PDH_FMT_LARGE, NULL, &value );
            ok(status == ERROR_SUCCESS, "PdhGetFormattedCounterValue failed 0x%08x\n", status);

            trace( "uptime %x%08x\n", (DWORD)(U(value).largeValue >> 32), (DWORD)U(value).largeValue );
        }
    }

    ret = CloseHandle( event );
    ok(ret, "CloseHandle failed\n");

    status = PdhCloseQuery( query );
    ok(status == ERROR_SUCCESS, "PdhCloseQuery failed 0x%08x\n", status);
}

static void test_PdhMakeCounterPathA(void)
{
    PDH_STATUS ret;
    PDH_COUNTER_PATH_ELEMENTS_A e;
    char buffer[1024];
    DWORD buflen;

    ret = PdhMakeCounterPathA(NULL, NULL, NULL, 0);
    ok(ret == PDH_INVALID_ARGUMENT, "PdhMakeCounterPathA failed 0x%08x\n", ret);

    buflen = 0;
    ret = PdhMakeCounterPathA(NULL, NULL, &buflen, 0);
    ok(ret == PDH_INVALID_ARGUMENT, "PdhMakeCounterPathA failed 0x%08x\n", ret);

    buflen = 0;
    ret = PdhMakeCounterPathA(NULL, buffer, &buflen, 0);
    ok(ret == PDH_INVALID_ARGUMENT, "PdhMakeCounterPathA failed 0x%08x\n", ret);

    buflen = sizeof(buffer);
    memset(&e, 0, sizeof(e));
    ret = PdhMakeCounterPathA(&e, buffer, &buflen, 0);
    ok(ret == PDH_INVALID_ARGUMENT, "PdhMakeCounterPathA failed 0x%08x\n", ret);

    buffer[0] = 0;
    buflen = sizeof(buffer);
    e.szMachineName = (char *)"machine";
    ret = PdhMakeCounterPathA(&e, buffer, &buflen, 0);
    ok(ret == PDH_INVALID_ARGUMENT, "PdhMakeCounterPathA failed 0x%08x\n", ret);
    ok(!strcmp(buffer, ""), "expected \"machine\" got %s\n", buffer);

    buffer[0] = 0;
    buflen = sizeof(buffer);
    e.szObjectName = (char *)"object";
    ret = PdhMakeCounterPathA(&e, buffer, &buflen, 0);
    ok(ret == PDH_INVALID_ARGUMENT, "PdhMakeCounterPathA failed 0x%08x\n", ret);
    ok(!strcmp(buffer, ""), "expected \"machine\" got %s\n", buffer);

    buffer[0] = 0;
    buflen = sizeof(buffer);
    e.szInstanceName = (char *)"instance";
    ret = PdhMakeCounterPathA(&e, buffer, &buflen, 0);
    ok(ret == PDH_INVALID_ARGUMENT, "PdhMakeCounterPathA failed 0x%08x\n", ret);
    ok(!strcmp(buffer, ""), "expected \"machine\" got %s\n", buffer);

    buffer[0] = 0;
    buflen = sizeof(buffer);
    e.szParentInstance = (char *)"parent";
    ret = PdhMakeCounterPathA(&e, buffer, &buflen, 0);
    ok(ret == PDH_INVALID_ARGUMENT, "PdhMakeCounterPathA failed 0x%08x\n", ret);
    ok(!strcmp(buffer, ""), "expected \"machine\" got %s\n", buffer);

    buffer[0] = 0;
    buflen = sizeof(buffer);
    e.dwInstanceIndex = 1;
    ret = PdhMakeCounterPathA(&e, buffer, &buflen, 0);
    ok(ret == PDH_INVALID_ARGUMENT, "PdhMakeCounterPathA failed 0x%08x\n", ret);
    ok(!strcmp(buffer, ""), "expected \"machine\" got %s\n", buffer);

    buffer[0] = 0;
    buflen = sizeof(buffer);
    e.szCounterName = (char *)"counter";
    ret = PdhMakeCounterPathA(&e, buffer, &buflen, 0);
    ok(ret == ERROR_SUCCESS, "PdhMakeCounterPathA failed 0x%08x\n", ret);
    ok(!strcmp(buffer, "\\\\machine\\object(parent/instance#1)\\counter"),
       "expected \"\\\\machine\\object(parent/instance#1)\\counter\" got %s\n", buffer);

    buffer[0] = 0;
    buflen = sizeof(buffer);
    e.szParentInstance = NULL;
    ret = PdhMakeCounterPathA(&e, buffer, &buflen, 0);
    ok(ret == ERROR_SUCCESS, "PdhMakeCounterPathA failed 0x%08x\n", ret);
    ok(!strcmp(buffer, "\\\\machine\\object(instance#1)\\counter"),
       "expected \"\\\\machine\\object(instance#1)\\counter\" got %s\n", buffer);

    buffer[0] = 0;
    buflen = sizeof(buffer);
    e.szInstanceName = NULL;
    ret = PdhMakeCounterPathA(&e, buffer, &buflen, 0);
    ok(ret == ERROR_SUCCESS, "PdhMakeCounterPathA failed 0x%08x\n", ret);
    ok(!strcmp(buffer, "\\\\machine\\object\\counter"),
       "expected \"\\\\machine\\object\\counter\" got %s\n", buffer);

    buffer[0] = 0;
    buflen = sizeof(buffer);
    e.szMachineName = NULL;
    ret = PdhMakeCounterPathA(&e, buffer, &buflen, 0);
    ok(ret == ERROR_SUCCESS, "PdhMakeCounterPathA failed 0x%08x\n", ret);
    ok(!strcmp(buffer, "\\object\\counter"),
       "expected \"\\object\\counter\" got %s\n", buffer);

    buffer[0] = 0;
    buflen = sizeof(buffer);
    e.szObjectName = NULL;
    ret = PdhMakeCounterPathA(&e, buffer, &buflen, 0);
    ok(ret == PDH_INVALID_ARGUMENT, "PdhMakeCounterPathA failed 0x%08x\n", ret);
}

START_TEST(pdh)
{
    if (PRIMARYLANGID(LANGIDFROMLCID(GetThreadLocale())) != LANG_ENGLISH)
    {
        skip("non-english locale\n");
        return;
    }
    init_function_ptrs();

    test_PdhOpenQueryA();
    test_PdhOpenQueryW();

    test_PdhAddCounterA();
    test_PdhAddCounterW();

    if (pPdhAddEnglishCounterA) test_PdhAddEnglishCounterA();
    if (pPdhAddEnglishCounterW) test_PdhAddEnglishCounterW();
    if (pPdhCollectQueryDataWithTime) test_PdhCollectQueryDataWithTime();

    test_PdhGetFormattedCounterValue();
    test_PdhGetRawCounterValue();
    test_PdhSetCounterScaleFactor();
    test_PdhGetCounterTimeBase();

    test_PdhGetCounterInfoA();
    test_PdhGetCounterInfoW();

    test_PdhLookupPerfIndexByNameA();
    test_PdhLookupPerfIndexByNameW();

    test_PdhLookupPerfNameByIndexA();
    test_PdhLookupPerfNameByIndexW();

    test_PdhValidatePathA();
    test_PdhValidatePathW();

    if (pPdhValidatePathExA) test_PdhValidatePathExA();
    if (pPdhValidatePathExW) test_PdhValidatePathExW();

    test_PdhCollectQueryDataEx();
    test_PdhMakeCounterPathA();
}
