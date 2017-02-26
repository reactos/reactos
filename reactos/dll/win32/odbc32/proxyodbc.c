/*
 * Win32 ODBC functions
 *
 * Copyright 1999 Xiang Li, Corel Corporation
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
 *
 * NOTES:
 *   Proxy ODBC driver manager.  This manager delegates all ODBC 
 *   calls to a real ODBC driver manager named by the environment 
 *   variable LIB_ODBC_DRIVER_MANAGER, or to libodbc.so if the
 *   variable is not set.
 *
 */

#define WIN32_NO_STATUS

#include <config.h>
#include <wine/port.h>

//#include <stdarg.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
#include <assert.h>

#include <windef.h>
//#include "winbase.h"
#include <winreg.h>

//#include "sql.h"
//#include "sqltypes.h"
#include <sqlext.h>

#undef TRACE_ON

#include <wine/debug.h>
#include <wine/library.h>
#include <wine/unicode.h>

static BOOL ODBC_LoadDriverManager(void);
static BOOL ODBC_LoadDMFunctions(void);

WINE_DEFAULT_DEBUG_CHANNEL(odbc);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

static SQLRETURN (*pSQLAllocConnect)(SQLHENV,SQLHDBC*);
static SQLRETURN (*pSQLAllocEnv)(SQLHENV*);
static SQLRETURN (*pSQLAllocHandle)(SQLSMALLINT,SQLHANDLE,SQLHANDLE*);
static SQLRETURN (*pSQLAllocHandleStd)(SQLSMALLINT,SQLHANDLE,SQLHANDLE*);
static SQLRETURN (*pSQLAllocStmt)(SQLHDBC,SQLHSTMT*);
static SQLRETURN (*pSQLBindCol)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT,SQLPOINTER,SQLINTEGER,SQLLEN*);
static SQLRETURN (*pSQLBindParam)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT,SQLSMALLINT,SQLUINTEGER,SQLSMALLINT,SQLPOINTER,SQLINTEGER*);
static SQLRETURN (*pSQLBindParameter)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT,SQLSMALLINT,SQLSMALLINT,SQLUINTEGER,SQLSMALLINT,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLBrowseConnect)(SQLHDBC,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLBrowseConnectW)(SQLHDBC,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLBulkOperations)(SQLHSTMT,SQLSMALLINT);
static SQLRETURN (*pSQLCancel)(SQLHSTMT);
static SQLRETURN (*pSQLCloseCursor)(SQLHSTMT);
static SQLRETURN (*pSQLColAttribute)(SQLHSTMT,SQLUSMALLINT,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*,SQLPOINTER);
static SQLRETURN (*pSQLColAttributeW)(SQLHSTMT,SQLUSMALLINT,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*,SQLPOINTER);
static SQLRETURN (*pSQLColAttributes)(SQLHSTMT,SQLUSMALLINT,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*,SQLINTEGER*);
static SQLRETURN (*pSQLColAttributesW)(SQLHSTMT,SQLUSMALLINT,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*,SQLPOINTER);
static SQLRETURN (*pSQLColumnPrivileges)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLColumnPrivilegesW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLColumns)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLColumnsW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLConnect)(SQLHDBC,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLConnectW)(SQLHDBC,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLCopyDesc)(SQLHDESC,SQLHDESC);
static SQLRETURN (*pSQLDataSources)(SQLHENV,SQLUSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLDataSourcesA)(SQLHENV,SQLUSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLDataSourcesW)(SQLHENV,SQLUSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLDescribeCol)(SQLHSTMT,SQLUSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLSMALLINT*,SQLUINTEGER*,SQLSMALLINT*,SQLSMALLINT*);
static SQLRETURN (*pSQLDescribeColW)(SQLHSTMT,SQLUSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLSMALLINT*,SQLULEN*,SQLSMALLINT*,SQLSMALLINT*);
static SQLRETURN (*pSQLDescribeParam)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT*,SQLUINTEGER*,SQLSMALLINT*,SQLSMALLINT*);
static SQLRETURN (*pSQLDisconnect)(SQLHDBC);
static SQLRETURN (*pSQLDriverConnect)(SQLHDBC,SQLHWND,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLUSMALLINT);
static SQLRETURN (*pSQLDriverConnectW)(SQLHDBC,SQLHWND,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLUSMALLINT);
static SQLRETURN (*pSQLDrivers)(SQLHENV,SQLUSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLDriversW)(SQLHENV,SQLUSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLEndTran)(SQLSMALLINT,SQLHANDLE,SQLSMALLINT);
static SQLRETURN (*pSQLError)(SQLHENV,SQLHDBC,SQLHSTMT,SQLCHAR*,SQLINTEGER*,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLErrorW)(SQLHENV,SQLHDBC,SQLHSTMT,SQLWCHAR*,SQLINTEGER*,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLExecDirect)(SQLHSTMT,SQLCHAR*,SQLINTEGER);
static SQLRETURN (*pSQLExecDirectW)(SQLHSTMT,SQLWCHAR*,SQLINTEGER);
static SQLRETURN (*pSQLExecute)(SQLHSTMT);
static SQLRETURN (*pSQLExtendedFetch)(SQLHSTMT,SQLUSMALLINT,SQLINTEGER,SQLUINTEGER*,SQLUSMALLINT*);
static SQLRETURN (*pSQLFetch)(SQLHSTMT);
static SQLRETURN (*pSQLFetchScroll)(SQLHSTMT,SQLSMALLINT,SQLINTEGER);
static SQLRETURN (*pSQLForeignKeys)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLForeignKeysW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLFreeConnect)(SQLHDBC);
static SQLRETURN (*pSQLFreeEnv)(SQLHENV);
static SQLRETURN (*pSQLFreeHandle)(SQLSMALLINT,SQLHANDLE);
static SQLRETURN (*pSQLFreeStmt)(SQLHSTMT,SQLUSMALLINT);
static SQLRETURN (*pSQLGetConnectAttr)(SQLHDBC,SQLINTEGER,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLGetConnectAttrW)(SQLHDBC,SQLINTEGER,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLGetConnectOption)(SQLHDBC,SQLUSMALLINT,SQLPOINTER);
static SQLRETURN (*pSQLGetConnectOptionW)(SQLHDBC,SQLUSMALLINT,SQLPOINTER);
static SQLRETURN (*pSQLGetCursorName)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLGetCursorNameW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLGetData)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLGetDescField)(SQLHDESC,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLGetDescFieldW)(SQLHDESC,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLGetDescRec)(SQLHDESC,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLSMALLINT*,SQLSMALLINT*,SQLINTEGER*,SQLSMALLINT*,SQLSMALLINT*,SQLSMALLINT*);
static SQLRETURN (*pSQLGetDescRecW)(SQLHDESC,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLSMALLINT*,SQLSMALLINT*,SQLLEN*,SQLSMALLINT*,SQLSMALLINT*,SQLSMALLINT*);
static SQLRETURN (*pSQLGetDiagField)(SQLSMALLINT,SQLHANDLE,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLGetDiagFieldW)(SQLSMALLINT,SQLHANDLE,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLGetDiagRec)(SQLSMALLINT,SQLHANDLE,SQLSMALLINT,SQLCHAR*,SQLINTEGER*,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLGetDiagRecW)(SQLSMALLINT,SQLHANDLE,SQLSMALLINT,SQLWCHAR*,SQLINTEGER*,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLGetEnvAttr)(SQLHENV,SQLINTEGER,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLGetFunctions)(SQLHDBC,SQLUSMALLINT,SQLUSMALLINT*);
static SQLRETURN (*pSQLGetInfo)(SQLHDBC,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLGetInfoW)(SQLHDBC,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLGetStmtAttr)(SQLHSTMT,SQLINTEGER,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLGetStmtAttrW)(SQLHSTMT,SQLINTEGER,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLGetStmtOption)(SQLHSTMT,SQLUSMALLINT,SQLPOINTER);
static SQLRETURN (*pSQLGetTypeInfo)(SQLHSTMT,SQLSMALLINT);
static SQLRETURN (*pSQLGetTypeInfoW)(SQLHSTMT,SQLSMALLINT);
static SQLRETURN (*pSQLMoreResults)(SQLHSTMT);
static SQLRETURN (*pSQLNativeSql)(SQLHDBC,SQLCHAR*,SQLINTEGER,SQLCHAR*,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLNativeSqlW)(SQLHDBC,SQLWCHAR*,SQLINTEGER,SQLWCHAR*,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLNumParams)(SQLHSTMT,SQLSMALLINT*);
static SQLRETURN (*pSQLNumResultCols)(SQLHSTMT,SQLSMALLINT*);
static SQLRETURN (*pSQLParamData)(SQLHSTMT,SQLPOINTER*);
static SQLRETURN (*pSQLParamOptions)(SQLHSTMT,SQLUINTEGER,SQLUINTEGER*);
static SQLRETURN (*pSQLPrepare)(SQLHSTMT,SQLCHAR*,SQLINTEGER);
static SQLRETURN (*pSQLPrepareW)(SQLHSTMT,SQLWCHAR*,SQLINTEGER);
static SQLRETURN (*pSQLPrimaryKeys)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLPrimaryKeysW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLProcedureColumns)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLProcedureColumnsW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLProcedures)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLProceduresW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLPutData)(SQLHSTMT,SQLPOINTER,SQLINTEGER);
static SQLRETURN (*pSQLRowCount)(SQLHSTMT,SQLINTEGER*);
static SQLRETURN (*pSQLSetConnectAttr)(SQLHDBC,SQLINTEGER,SQLPOINTER,SQLINTEGER);
static SQLRETURN (*pSQLSetConnectAttrW)(SQLHDBC,SQLINTEGER,SQLPOINTER,SQLINTEGER);
static SQLRETURN (*pSQLSetConnectOption)(SQLHDBC,SQLUSMALLINT,SQLUINTEGER);
static SQLRETURN (*pSQLSetConnectOptionW)(SQLHDBC,SQLUSMALLINT,SQLULEN);
static SQLRETURN (*pSQLSetCursorName)(SQLHSTMT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLSetCursorNameW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLSetDescField)(SQLHDESC,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLINTEGER);
static SQLRETURN (*pSQLSetDescFieldW)(SQLHDESC,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLINTEGER);
static SQLRETURN (*pSQLSetDescRec)(SQLHDESC,SQLSMALLINT,SQLSMALLINT,SQLSMALLINT,SQLINTEGER,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLINTEGER*,SQLINTEGER*);
static SQLRETURN (*pSQLSetEnvAttr)(SQLHENV,SQLINTEGER,SQLPOINTER,SQLINTEGER);
static SQLRETURN (*pSQLSetParam)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT,SQLSMALLINT,SQLUINTEGER,SQLSMALLINT,SQLPOINTER,SQLINTEGER*);
static SQLRETURN (*pSQLSetPos)(SQLHSTMT,SQLUSMALLINT,SQLUSMALLINT,SQLUSMALLINT);
static SQLRETURN (*pSQLSetScrollOptions)(SQLHSTMT,SQLUSMALLINT,SQLINTEGER,SQLUSMALLINT);
static SQLRETURN (*pSQLSetStmtAttr)(SQLHSTMT,SQLINTEGER,SQLPOINTER,SQLINTEGER);
static SQLRETURN (*pSQLSetStmtAttrW)(SQLHSTMT,SQLINTEGER,SQLPOINTER,SQLINTEGER);
static SQLRETURN (*pSQLSetStmtOption)(SQLHSTMT,SQLUSMALLINT,SQLUINTEGER);
static SQLRETURN (*pSQLSpecialColumns)(SQLHSTMT,SQLUSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLUSMALLINT,SQLUSMALLINT);
static SQLRETURN (*pSQLSpecialColumnsW)(SQLHSTMT,SQLUSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLUSMALLINT,SQLUSMALLINT);
static SQLRETURN (*pSQLStatistics)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLUSMALLINT,SQLUSMALLINT);
static SQLRETURN (*pSQLStatisticsW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLUSMALLINT,SQLUSMALLINT);
static SQLRETURN (*pSQLTablePrivileges)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLTablePrivilegesW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLTables)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLTablesW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLTransact)(SQLHENV,SQLHDBC,SQLUSMALLINT);

#define ERROR_FREE 0
#define ERROR_SQLERROR  1
#define ERROR_LIBRARY_NOT_FOUND 2

static void *dmHandle;
static int nErrorType;

/***********************************************************************
 * ODBC_ReplicateODBCInstToRegistry
 *
 * PARAMS
 *
 * RETURNS
 *
 * Utility to ODBC_ReplicateToRegistry to replicate the drivers of the
 * ODBCINST.INI settings
 *
 * The driver settings are not replicated to the registry.  If we were to 
 * replicate them we would need to decide whether to replicate all settings
 * or to do some translation; whether to remove any entries present only in
 * the windows registry, etc.
 */

static void ODBC_ReplicateODBCInstToRegistry (SQLHENV hEnv)
{
    HKEY hODBCInst;
    LONG reg_ret;
    BOOL success;

    success = FALSE;
    TRACE ("Driver settings are not currently replicated to the registry\n");
    if ((reg_ret = RegCreateKeyExA (HKEY_LOCAL_MACHINE,
            "Software\\ODBC\\ODBCINST.INI", 0, NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS /* a couple more than we need */, NULL,
            &hODBCInst, NULL)) == ERROR_SUCCESS)
    {
        HKEY hDrivers;
        if ((reg_ret = RegCreateKeyExA (hODBCInst, "ODBC Drivers", 0,
                NULL, REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS /* overkill */, NULL, &hDrivers, NULL))
                == ERROR_SUCCESS)
        {
            SQLRETURN sql_ret;
            SQLUSMALLINT dirn;
            CHAR desc [256];
            SQLSMALLINT sizedesc;

            success = TRUE;
            dirn = SQL_FETCH_FIRST;
            while ((sql_ret = SQLDrivers (hEnv, dirn, (SQLCHAR*)desc, sizeof(desc),
                    &sizedesc, NULL, 0, NULL)) == SQL_SUCCESS ||
                    sql_ret == SQL_SUCCESS_WITH_INFO)
            {
                /* FIXME Do some proper handling of the SUCCESS_WITH_INFO */
                dirn = SQL_FETCH_NEXT;
                if (sizedesc == lstrlenA(desc))
                {
                    HKEY hThis;
                    if ((reg_ret = RegQueryValueExA (hDrivers, desc, NULL,
                            NULL, NULL, NULL)) == ERROR_FILE_NOT_FOUND)
                    {
                        if ((reg_ret = RegSetValueExA (hDrivers, desc, 0,
                                REG_SZ, (const BYTE *)"Installed", 10)) != ERROR_SUCCESS)
                        {
                            TRACE ("Error %d replicating driver %s\n",
                                    reg_ret, desc);
                            success = FALSE;
                        }
                    }
                    else if (reg_ret != ERROR_SUCCESS)
                    {
                        TRACE ("Error %d checking for %s in drivers\n",
                                reg_ret, desc);
                        success = FALSE;
                    }
                    if ((reg_ret = RegCreateKeyExA (hODBCInst, desc, 0,
                            NULL, REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS, NULL, &hThis, NULL))
                            == ERROR_SUCCESS)
                    {
                        /* FIXME This is where the settings go.
                         * I suggest that if the disposition says it 
                         * exists then we leave it alone.  Alternatively
                         * include an extra value to flag that it is 
                         * a replication of the unixODBC/iODBC/...
                         */
                        if ((reg_ret = RegCloseKey (hThis)) !=
                                ERROR_SUCCESS)
                            TRACE ("Error %d closing %s key\n", reg_ret,
                                    desc);
                    }
                    else
                    {
                        TRACE ("Error %d ensuring driver key %s\n",
                                reg_ret, desc);
                        success = FALSE;
                    }
                }
                else
                {
                    WARN ("Unusually long driver name %s not replicated\n",
                            desc);
                    success = FALSE;
                }
            }
            if (sql_ret != SQL_NO_DATA)
            {
                TRACE ("Error %d enumerating drivers\n", (int)sql_ret);
                success = FALSE;
            }
            if ((reg_ret = RegCloseKey (hDrivers)) != ERROR_SUCCESS)
            {
                TRACE ("Error %d closing hDrivers\n", reg_ret);
            }
        }
        else
        {
            TRACE ("Error %d opening HKLM\\S\\O\\OI\\Drivers\n", reg_ret);
        }
        if ((reg_ret = RegCloseKey (hODBCInst)) != ERROR_SUCCESS)
        {
            TRACE ("Error %d closing HKLM\\S\\O\\ODBCINST.INI\n", reg_ret);
        }
    }
    else
    {
        TRACE ("Error %d opening HKLM\\S\\O\\ODBCINST.INI\n", reg_ret);
    }
    if (!success)
    {
        WARN ("May not have replicated all ODBC drivers to the registry\n");
    }
}

/***********************************************************************
 * ODBC_ReplicateODBCToRegistry
 *
 * PARAMS
 *
 * RETURNS
 *
 * Utility to ODBC_ReplicateToRegistry to replicate either the USER or 
 * SYSTEM dsns
 *
 * For now simply place the "Driver description" (as returned by SQLDataSources)
 * into the registry as the driver.  This is enough to satisfy Crystal's 
 * requirement that there be a driver entry.  (It doesn't seem to care what
 * the setting is).
 * A slightly more accurate setting would be to access the registry to find
 * the actual driver library for the given description (which appears to map
 * to one of the HKLM/Software/ODBC/ODBCINST.INI keys).  (If you do this note
 * that this will add a requirement that this function be called after
 * ODBC_ReplicateODBCInstToRegistry)
 */
static void ODBC_ReplicateODBCToRegistry (BOOL is_user, SQLHENV hEnv)
{
    HKEY hODBC;
    LONG reg_ret;
    SQLRETURN sql_ret;
    SQLUSMALLINT dirn;
    CHAR dsn [SQL_MAX_DSN_LENGTH + 1];
    SQLSMALLINT sizedsn;
    CHAR desc [256];
    SQLSMALLINT sizedesc;
    BOOL success;
    const char *which = is_user ? "user" : "system";

    success = FALSE;
    if ((reg_ret = RegCreateKeyExA (
            is_user ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE,
            "Software\\ODBC\\ODBC.INI", 0, NULL, REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS /* a couple more than we need */, NULL, &hODBC,
            NULL)) == ERROR_SUCCESS)
    {
        success = TRUE;
        dirn = is_user ? SQL_FETCH_FIRST_USER : SQL_FETCH_FIRST_SYSTEM;
        while ((sql_ret = SQLDataSources (hEnv, dirn,
                (SQLCHAR*)dsn, sizeof(dsn), &sizedsn,
                (SQLCHAR*)desc, sizeof(desc), &sizedesc)) == SQL_SUCCESS
                || sql_ret == SQL_SUCCESS_WITH_INFO)
        {
            /* FIXME Do some proper handling of the SUCCESS_WITH_INFO */
            dirn = SQL_FETCH_NEXT;
            if (sizedsn == lstrlenA(dsn) && sizedesc == lstrlenA(desc))
            {
                HKEY hDSN;
                if ((reg_ret = RegCreateKeyExA (hODBC, dsn, 0,
                        NULL, REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS, NULL, &hDSN, NULL))
                        == ERROR_SUCCESS)
                {
                    static const char DRIVERKEY[] = "Driver";
                    if ((reg_ret = RegQueryValueExA (hDSN, DRIVERKEY,
                            NULL, NULL, NULL, NULL))
                            == ERROR_FILE_NOT_FOUND)
                    {
                        if ((reg_ret = RegSetValueExA (hDSN, DRIVERKEY, 0,
                                REG_SZ, (LPBYTE)desc, sizedesc)) != ERROR_SUCCESS)
                        {
                            TRACE ("Error %d replicating description of "
                                    "%s(%s)\n", reg_ret, dsn, desc);
                            success = FALSE;
                        }
                    }
                    else if (reg_ret != ERROR_SUCCESS)
                    {
                        TRACE ("Error %d checking for description of %s\n",
                                reg_ret, dsn);
                        success = FALSE;
                    }
                    if ((reg_ret = RegCloseKey (hDSN)) != ERROR_SUCCESS)
                    {
                        TRACE ("Error %d closing %s DSN key %s\n",
                                reg_ret, which, dsn);
                    }
                }
                else
                {
                    TRACE ("Error %d opening %s DSN key %s\n",
                            reg_ret, which, dsn);
                    success = FALSE;
                }
            }
            else
            {
                WARN ("Unusually long %s data source name %s (%s) not "
                        "replicated\n", which, dsn, desc);
                success = FALSE;
            }
        }
        if (sql_ret != SQL_NO_DATA)
        {
            TRACE ("Error %d enumerating %s datasources\n",
                    (int)sql_ret, which);
            success = FALSE;
        }
        if ((reg_ret = RegCloseKey (hODBC)) != ERROR_SUCCESS)
        {
            TRACE ("Error %d closing %s ODBC.INI registry key\n", reg_ret,
                    which);
        }
    }
    else
    {
        TRACE ("Error %d creating/opening %s ODBC.INI registry key\n",
                reg_ret, which);
    }
    if (!success)
    {
        WARN ("May not have replicated all %s ODBC DSNs to the registry\n",
                which);
    }
}

/***********************************************************************
 * ODBC_ReplicateToRegistry
 *
 * PARAMS
 *
 * RETURNS
 *
 * Unfortunately some of the functions that Windows documents as being part
 * of the ODBC API it implements directly during compilation or something
 * in terms of registry access functions.
 * e.g. SQLGetInstalledDrivers queries the list at
 * HKEY_LOCAL_MACHINE\Software\ODBC\ODBCINST.INI\ODBC Drivers
 *
 * This function is called when the driver manager is loaded and is used
 * to replicate the appropriate details into the Wine registry
 */

static void ODBC_ReplicateToRegistry (void)
{
    SQLRETURN sql_ret;
    SQLHENV hEnv;

    if ((sql_ret = SQLAllocEnv (&hEnv)) == SQL_SUCCESS)
    {
        ODBC_ReplicateODBCInstToRegistry (hEnv);
        ODBC_ReplicateODBCToRegistry (FALSE /* system dsns */, hEnv);
        ODBC_ReplicateODBCToRegistry (TRUE /* user dsns */, hEnv);

        if ((sql_ret = SQLFreeEnv (hEnv)) != SQL_SUCCESS)
        {
            TRACE ("Error %d freeing the SQL environment.\n", (int)sql_ret);
        }
    }
    else
    {
        TRACE ("Error %d opening an SQL environment.\n", (int)sql_ret);
        WARN ("The external ODBC settings have not been replicated to the"
                " Wine registry\n");
    }
}

/***********************************************************************
 * DllMain [Internal] Initializes the internal 'ODBC32.DLL'.
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD reason, LPVOID reserved)
{
    TRACE("proxy ODBC: %p,%x,%p\n", hinstDLL, reason, reserved);

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
       DisableThreadLibraryCalls(hinstDLL);
       if (ODBC_LoadDriverManager())
       {
          ODBC_LoadDMFunctions();
          ODBC_ReplicateToRegistry();
       }
       break;

    case DLL_PROCESS_DETACH:
      if (reserved) break;
      if (dmHandle) wine_dlclose(dmHandle,NULL,0);
    }

    return TRUE;
}

/***********************************************************************
 * ODBC_LoadDriverManager [Internal] Load ODBC library.
 *
 * PARAMS
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

static BOOL ODBC_LoadDriverManager(void)
{
   const char *s = getenv("LIB_ODBC_DRIVER_MANAGER");
   char error[256];

#ifdef SONAME_LIBODBC
   if (!s || !s[0]) s = SONAME_LIBODBC;
#endif
   if (!s || !s[0]) goto failed;

   dmHandle = wine_dlopen(s, RTLD_LAZY | RTLD_GLOBAL, error, sizeof(error));

   if (dmHandle != NULL)
   {
      TRACE("Opened library %s\n", s);
      nErrorType = ERROR_FREE;
      return TRUE;
   }
failed:
   ERR_(winediag)("failed to open library %s: %s\n", debugstr_a(s), error);
   nErrorType = ERROR_LIBRARY_NOT_FOUND;
   return FALSE;
}


/***********************************************************************
 * ODBC_LoadDMFunctions [Internal] Populate function table.
 *
 * PARAMS
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

static BOOL ODBC_LoadDMFunctions(void)
{
    char error[256];

    if (dmHandle == NULL)
        return FALSE;

#define LOAD_FUNC(name) \
    if ((p##name = wine_dlsym( dmHandle, #name, error, sizeof(error) ))); \
    else WARN( "Failed to load %s: %s\n", #name, error )

    LOAD_FUNC(SQLAllocConnect);
    LOAD_FUNC(SQLAllocEnv);
    LOAD_FUNC(SQLAllocHandle);
    LOAD_FUNC(SQLAllocHandleStd);
    LOAD_FUNC(SQLAllocStmt);
    LOAD_FUNC(SQLBindCol);
    LOAD_FUNC(SQLBindParam);
    LOAD_FUNC(SQLBindParameter);
    LOAD_FUNC(SQLBrowseConnect);
    LOAD_FUNC(SQLBrowseConnectW);
    LOAD_FUNC(SQLBulkOperations);
    LOAD_FUNC(SQLCancel);
    LOAD_FUNC(SQLCloseCursor);
    LOAD_FUNC(SQLColAttribute);
    LOAD_FUNC(SQLColAttributeW);
    LOAD_FUNC(SQLColAttributes);
    LOAD_FUNC(SQLColAttributesW);
    LOAD_FUNC(SQLColumnPrivileges);
    LOAD_FUNC(SQLColumnPrivilegesW);
    LOAD_FUNC(SQLColumns);
    LOAD_FUNC(SQLColumnsW);
    LOAD_FUNC(SQLConnect);
    LOAD_FUNC(SQLConnectW);
    LOAD_FUNC(SQLCopyDesc);
    LOAD_FUNC(SQLDataSources);
    LOAD_FUNC(SQLDataSourcesA);
    LOAD_FUNC(SQLDataSourcesW);
    LOAD_FUNC(SQLDescribeCol);
    LOAD_FUNC(SQLDescribeColW);
    LOAD_FUNC(SQLDescribeParam);
    LOAD_FUNC(SQLDisconnect);
    LOAD_FUNC(SQLDriverConnect);
    LOAD_FUNC(SQLDriverConnectW);
    LOAD_FUNC(SQLDrivers);
    LOAD_FUNC(SQLDriversW);
    LOAD_FUNC(SQLEndTran);
    LOAD_FUNC(SQLError);
    LOAD_FUNC(SQLErrorW);
    LOAD_FUNC(SQLExecDirect);
    LOAD_FUNC(SQLExecDirectW);
    LOAD_FUNC(SQLExecute);
    LOAD_FUNC(SQLExtendedFetch);
    LOAD_FUNC(SQLFetch);
    LOAD_FUNC(SQLFetchScroll);
    LOAD_FUNC(SQLForeignKeys);
    LOAD_FUNC(SQLForeignKeysW);
    LOAD_FUNC(SQLFreeConnect);
    LOAD_FUNC(SQLFreeEnv);
    LOAD_FUNC(SQLFreeHandle);
    LOAD_FUNC(SQLFreeStmt);
    LOAD_FUNC(SQLGetConnectAttr);
    LOAD_FUNC(SQLGetConnectAttrW);
    LOAD_FUNC(SQLGetConnectOption);
    LOAD_FUNC(SQLGetConnectOptionW);
    LOAD_FUNC(SQLGetCursorName);
    LOAD_FUNC(SQLGetCursorNameW);
    LOAD_FUNC(SQLGetData);
    LOAD_FUNC(SQLGetDescField);
    LOAD_FUNC(SQLGetDescFieldW);
    LOAD_FUNC(SQLGetDescRec);
    LOAD_FUNC(SQLGetDescRecW);
    LOAD_FUNC(SQLGetDiagField);
    LOAD_FUNC(SQLGetDiagFieldW);
    LOAD_FUNC(SQLGetDiagRec);
    LOAD_FUNC(SQLGetDiagRecW);
    LOAD_FUNC(SQLGetEnvAttr);
    LOAD_FUNC(SQLGetFunctions);
    LOAD_FUNC(SQLGetInfo);
    LOAD_FUNC(SQLGetInfoW);
    LOAD_FUNC(SQLGetStmtAttr);
    LOAD_FUNC(SQLGetStmtAttrW);
    LOAD_FUNC(SQLGetStmtOption);
    LOAD_FUNC(SQLGetTypeInfo);
    LOAD_FUNC(SQLGetTypeInfoW);
    LOAD_FUNC(SQLMoreResults);
    LOAD_FUNC(SQLNativeSql);
    LOAD_FUNC(SQLNativeSqlW);
    LOAD_FUNC(SQLNumParams);
    LOAD_FUNC(SQLNumResultCols);
    LOAD_FUNC(SQLParamData);
    LOAD_FUNC(SQLParamOptions);
    LOAD_FUNC(SQLPrepare);
    LOAD_FUNC(SQLPrepareW);
    LOAD_FUNC(SQLPrimaryKeys);
    LOAD_FUNC(SQLPrimaryKeysW);
    LOAD_FUNC(SQLProcedureColumns);
    LOAD_FUNC(SQLProcedureColumnsW);
    LOAD_FUNC(SQLProcedures);
    LOAD_FUNC(SQLProceduresW);
    LOAD_FUNC(SQLPutData);
    LOAD_FUNC(SQLRowCount);
    LOAD_FUNC(SQLSetConnectAttr);
    LOAD_FUNC(SQLSetConnectAttrW);
    LOAD_FUNC(SQLSetConnectOption);
    LOAD_FUNC(SQLSetConnectOptionW);
    LOAD_FUNC(SQLSetCursorName);
    LOAD_FUNC(SQLSetCursorNameW);
    LOAD_FUNC(SQLSetDescField);
    LOAD_FUNC(SQLSetDescFieldW);
    LOAD_FUNC(SQLSetDescRec);
    LOAD_FUNC(SQLSetEnvAttr);
    LOAD_FUNC(SQLSetParam);
    LOAD_FUNC(SQLSetPos);
    LOAD_FUNC(SQLSetScrollOptions);
    LOAD_FUNC(SQLSetStmtAttr);
    LOAD_FUNC(SQLSetStmtAttrW);
    LOAD_FUNC(SQLSetStmtOption);
    LOAD_FUNC(SQLSpecialColumns);
    LOAD_FUNC(SQLSpecialColumnsW);
    LOAD_FUNC(SQLStatistics);
    LOAD_FUNC(SQLStatisticsW);
    LOAD_FUNC(SQLTablePrivileges);
    LOAD_FUNC(SQLTablePrivilegesW);
    LOAD_FUNC(SQLTables);
    LOAD_FUNC(SQLTablesW);
    LOAD_FUNC(SQLTransact);
#undef LOAD_FUNC

    return TRUE;
}


/*************************************************************************
 *				SQLAllocConnect           [ODBC32.001]
 */
SQLRETURN WINAPI SQLAllocConnect(SQLHENV EnvironmentHandle, SQLHDBC *ConnectionHandle)
{
        SQLRETURN ret;
        TRACE("(EnvironmentHandle %p)\n",EnvironmentHandle);

        if (!pSQLAllocConnect)
        {
           *ConnectionHandle = SQL_NULL_HDBC;
           TRACE("Not ready\n");
           return SQL_ERROR;
        }

        ret = pSQLAllocConnect(EnvironmentHandle, ConnectionHandle);
        TRACE("Returns %d, Handle %p\n", ret, *ConnectionHandle);
        return ret;
}


/*************************************************************************
 *				SQLAllocEnv           [ODBC32.002]
 */
SQLRETURN WINAPI  SQLAllocEnv(SQLHENV *EnvironmentHandle)
{
        SQLRETURN ret;
        TRACE("\n");

        if (!pSQLAllocEnv)
        {
           *EnvironmentHandle = SQL_NULL_HENV;
           TRACE("Not ready\n");
           return SQL_ERROR;
        }

        ret = pSQLAllocEnv(EnvironmentHandle);
        TRACE("Returns %d, EnvironmentHandle %p\n", ret, *EnvironmentHandle);
        return ret;
}


/*************************************************************************
 *				SQLAllocHandle           [ODBC32.024]
 */
SQLRETURN WINAPI SQLAllocHandle(SQLSMALLINT HandleType, SQLHANDLE InputHandle, SQLHANDLE *OutputHandle)
{
        SQLRETURN ret;
        TRACE("(Type %d, Handle %p)\n", HandleType, InputHandle);

        if (!pSQLAllocHandle)
        {
            if (nErrorType == ERROR_LIBRARY_NOT_FOUND)
                WARN("ProxyODBC: Cannot load ODBC driver manager library.\n");

            if (HandleType == SQL_HANDLE_ENV)
                *OutputHandle = SQL_NULL_HENV;
            else if (HandleType == SQL_HANDLE_DBC)
                *OutputHandle = SQL_NULL_HDBC;
            else if (HandleType == SQL_HANDLE_STMT)
                *OutputHandle = SQL_NULL_HSTMT;
            else if (HandleType == SQL_HANDLE_DESC)
                *OutputHandle = SQL_NULL_HDESC;

            TRACE ("Not ready\n");
            return SQL_ERROR;
        }

        ret = pSQLAllocHandle(HandleType, InputHandle, OutputHandle);
        TRACE("Returns %d, Handle %p\n", ret, *OutputHandle);
        return ret;
}


/*************************************************************************
 *				SQLAllocStmt           [ODBC32.003]
 */
SQLRETURN WINAPI SQLAllocStmt(SQLHDBC ConnectionHandle, SQLHSTMT *StatementHandle)
{
        SQLRETURN ret;

        TRACE("(Connection %p)\n", ConnectionHandle);

        if (!pSQLAllocStmt)
        {
           *StatementHandle = SQL_NULL_HSTMT;
           TRACE ("Not ready\n");
           return SQL_ERROR;
        }

        ret = pSQLAllocStmt(ConnectionHandle, StatementHandle);
        TRACE ("Returns %d, Handle %p\n", ret, *StatementHandle);
        return ret;
}


/*************************************************************************
 *				SQLAllocHandleStd           [ODBC32.077]
 */
SQLRETURN WINAPI SQLAllocHandleStd( SQLSMALLINT HandleType,
                                    SQLHANDLE InputHandle, SQLHANDLE *OutputHandle)
{
        TRACE("ProxyODBC: SQLAllocHandleStd.\n");

        if (!pSQLAllocHandleStd)
        {
            if (nErrorType == ERROR_LIBRARY_NOT_FOUND)
                WARN("ProxyODBC: Cannot load ODBC driver manager library.\n");

            if (HandleType == SQL_HANDLE_ENV)
                *OutputHandle = SQL_NULL_HENV;
            else if (HandleType == SQL_HANDLE_DBC)
                *OutputHandle = SQL_NULL_HDBC;
            else if (HandleType == SQL_HANDLE_STMT)
                *OutputHandle = SQL_NULL_HSTMT;
            else if (HandleType == SQL_HANDLE_DESC)
                *OutputHandle = SQL_NULL_HDESC;

            return SQL_ERROR;
        }

        return pSQLAllocHandleStd(HandleType, InputHandle, OutputHandle);
}


/*************************************************************************
 *				SQLBindCol           [ODBC32.004]
 */
SQLRETURN WINAPI SQLBindCol(SQLHSTMT StatementHandle,
                     SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
                     SQLPOINTER TargetValue, SQLLEN BufferLength,
                     SQLLEN *StrLen_or_Ind)
{
        TRACE("\n");

        if (!pSQLBindCol)
        {
                TRACE ("Not ready\n");
                return SQL_ERROR;
        }

        return pSQLBindCol(StatementHandle, ColumnNumber, TargetType,
                           TargetValue, BufferLength, StrLen_or_Ind);
}


/*************************************************************************
 *				SQLBindParam           [ODBC32.025]
 */
SQLRETURN WINAPI SQLBindParam(SQLHSTMT StatementHandle,
             SQLUSMALLINT ParameterNumber, SQLSMALLINT ValueType,
             SQLSMALLINT ParameterType, SQLULEN LengthPrecision,
             SQLSMALLINT ParameterScale, SQLPOINTER ParameterValue,
             SQLLEN *StrLen_or_Ind)
{
        TRACE("\n");

        if (!pSQLBindParam) return SQL_ERROR;
        return pSQLBindParam(StatementHandle, ParameterNumber, ValueType,
                             ParameterType, LengthPrecision, ParameterScale,
                             ParameterValue, StrLen_or_Ind);
}


/*************************************************************************
 *				SQLCancel           [ODBC32.005]
 */
SQLRETURN WINAPI SQLCancel(SQLHSTMT StatementHandle)
{
        TRACE("\n");

        if (!pSQLCancel) return SQL_ERROR;
        return pSQLCancel(StatementHandle);
}


/*************************************************************************
 *				SQLCloseCursor           [ODBC32.026]
 */
SQLRETURN WINAPI  SQLCloseCursor(SQLHSTMT StatementHandle)
{
        SQLRETURN ret;
        TRACE("(Handle %p)\n", StatementHandle);

        if (!pSQLCloseCursor) return SQL_ERROR;

        ret = pSQLCloseCursor(StatementHandle);
        TRACE("Returns %d\n", ret);
        return ret;
}


/*************************************************************************
 *				SQLColAttribute           [ODBC32.027]
 */
SQLRETURN WINAPI SQLColAttribute (SQLHSTMT StatementHandle,
             SQLUSMALLINT ColumnNumber, SQLUSMALLINT FieldIdentifier,
             SQLPOINTER CharacterAttribute, SQLSMALLINT BufferLength,
             SQLSMALLINT *StringLength, SQLPOINTER NumericAttribute)
{
        TRACE("\n");

        if (!pSQLColAttribute) return SQL_ERROR;
        return pSQLColAttribute(StatementHandle, ColumnNumber, FieldIdentifier,
                                CharacterAttribute, BufferLength, StringLength, NumericAttribute);
}


/*************************************************************************
 *				SQLColumns           [ODBC32.040]
 */
SQLRETURN WINAPI SQLColumns(SQLHSTMT StatementHandle,
             SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
             SQLCHAR *SchemaName, SQLSMALLINT NameLength2,
             SQLCHAR *TableName, SQLSMALLINT NameLength3,
             SQLCHAR *ColumnName, SQLSMALLINT NameLength4)
{
        TRACE("\n");

        if (!pSQLColumns) return SQL_ERROR;
        return pSQLColumns(StatementHandle, CatalogName, NameLength1,
                           SchemaName, NameLength2, TableName, NameLength3, ColumnName, NameLength4);
}


/*************************************************************************
 *				SQLConnect           [ODBC32.007]
 */
SQLRETURN WINAPI SQLConnect(SQLHDBC ConnectionHandle,
             SQLCHAR *ServerName, SQLSMALLINT NameLength1,
             SQLCHAR *UserName, SQLSMALLINT NameLength2,
             SQLCHAR *Authentication, SQLSMALLINT NameLength3)
{
        SQLRETURN ret;
        TRACE("(Server=%.*s)\n",NameLength1, ServerName);

        if (!pSQLConnect) return SQL_ERROR;

        ret = pSQLConnect(ConnectionHandle, ServerName, NameLength1,
                          UserName, NameLength2, Authentication, NameLength3);

        TRACE("Returns %d\n", ret);
        return ret;
}


/*************************************************************************
 *				SQLCopyDesc           [ODBC32.028]
 */
SQLRETURN WINAPI SQLCopyDesc(SQLHDESC SourceDescHandle, SQLHDESC TargetDescHandle)
{
        TRACE("\n");

        if (!pSQLCopyDesc) return SQL_ERROR;
        return pSQLCopyDesc(SourceDescHandle, TargetDescHandle);
}


/*************************************************************************
 *				SQLDataSources           [ODBC32.057]
 */
SQLRETURN WINAPI SQLDataSources(SQLHENV EnvironmentHandle,
             SQLUSMALLINT Direction, SQLCHAR *ServerName,
             SQLSMALLINT BufferLength1, SQLSMALLINT *NameLength1,
             SQLCHAR *Description, SQLSMALLINT BufferLength2,
             SQLSMALLINT *NameLength2)
{
        SQLRETURN ret;

        TRACE("(EnvironmentHandle %p)\n", EnvironmentHandle);

        if (!pSQLDataSources) return SQL_ERROR;

        ret = pSQLDataSources(EnvironmentHandle, Direction, ServerName,
                              BufferLength1, NameLength1, Description, BufferLength2, NameLength2);

        if (TRACE_ON(odbc))
        {
           TRACE("Returns %d \t", ret);
           if (NameLength1 && *NameLength1 > 0)
             TRACE("DataSource = %s,", ServerName);
           if (NameLength2 && *NameLength2 > 0)
             TRACE(" Description = %s", Description);
           TRACE("\n");
        }

        return ret;
}

SQLRETURN WINAPI SQLDataSourcesA(SQLHENV EnvironmentHandle,
             SQLUSMALLINT Direction, SQLCHAR *ServerName,
             SQLSMALLINT BufferLength1, SQLSMALLINT *NameLength1,
             SQLCHAR *Description, SQLSMALLINT BufferLength2,
             SQLSMALLINT *NameLength2)
{
    SQLRETURN ret;

    TRACE("EnvironmentHandle = %p\n", EnvironmentHandle);

    if (!pSQLDataSourcesA) return SQL_ERROR;

    ret = pSQLDataSourcesA(EnvironmentHandle, Direction, ServerName,
                           BufferLength1, NameLength1, Description, BufferLength2, NameLength2);
    if (TRACE_ON(odbc))
    {
       TRACE("Returns %d \t", ret);
       if (NameLength1 && *NameLength1 > 0)
         TRACE("DataSource = %s,", ServerName);
       if (NameLength2 && *NameLength2 > 0)
         TRACE(" Description = %s", Description);
       TRACE("\n");
    }

    return ret;
}

/*************************************************************************
 *				SQLDescribeCol           [ODBC32.008]
 */
SQLRETURN WINAPI SQLDescribeCol(SQLHSTMT StatementHandle,
             SQLUSMALLINT ColumnNumber, SQLCHAR *ColumnName,
             SQLSMALLINT BufferLength, SQLSMALLINT *NameLength,
             SQLSMALLINT *DataType, SQLULEN *ColumnSize,
             SQLSMALLINT *DecimalDigits, SQLSMALLINT *Nullable)
{
        TRACE("\n");

        if (!pSQLDescribeCol) return SQL_ERROR;
        return pSQLDescribeCol(StatementHandle, ColumnNumber, ColumnName,
                               BufferLength, NameLength, DataType, ColumnSize, DecimalDigits, Nullable);
}


/*************************************************************************
 *				SQLDisconnect           [ODBC32.009]
 */
SQLRETURN WINAPI SQLDisconnect(SQLHDBC ConnectionHandle)
{
        SQLRETURN ret;
        TRACE("(Handle %p)\n", ConnectionHandle);

        if (!pSQLDisconnect) return SQL_ERROR;

        ret = pSQLDisconnect(ConnectionHandle);
        TRACE("Returns %d\n", ret);
        return ret;
}


/*************************************************************************
 *				SQLEndTran           [ODBC32.029]
 */
SQLRETURN WINAPI SQLEndTran(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT CompletionType)
{
        TRACE("\n");

        if (!pSQLEndTran) return SQL_ERROR;
        return pSQLEndTran(HandleType, Handle, CompletionType);
}


/*************************************************************************
 *				SQLError           [ODBC32.010]
 */
SQLRETURN WINAPI SQLError(SQLHENV EnvironmentHandle,
             SQLHDBC ConnectionHandle, SQLHSTMT StatementHandle,
             SQLCHAR *Sqlstate, SQLINTEGER *NativeError,
             SQLCHAR *MessageText, SQLSMALLINT BufferLength,
             SQLSMALLINT *TextLength)
{
        SQLRETURN ret;

        TRACE("(EnvironmentHandle %p, ConnectionHandle %p, StatementHandle %p, BufferLength %d)\n",
              EnvironmentHandle, ConnectionHandle, StatementHandle, BufferLength);

        if (!pSQLError) return SQL_ERROR;
        ret = pSQLError(EnvironmentHandle, ConnectionHandle, StatementHandle,
                        Sqlstate, NativeError, MessageText, BufferLength, TextLength);
        if (ret == SQL_SUCCESS)
                TRACE("SQLState %s, Error %d, Text %s, Textlen %d\n",
                       debugstr_an((char *)Sqlstate, 5), *NativeError,
                       debugstr_an((char *)MessageText, *TextLength), *TextLength);
        else
                TRACE("Returns %d\n", ret);
        return ret;
}


/*************************************************************************
 *				SQLExecDirect           [ODBC32.011]
 */
SQLRETURN WINAPI SQLExecDirect(SQLHSTMT StatementHandle, SQLCHAR *StatementText, SQLINTEGER TextLength)
{
        TRACE("\n");

        if (!pSQLExecDirect) return SQL_ERROR;
        return pSQLExecDirect(StatementHandle, StatementText, TextLength);
}


/*************************************************************************
 *				SQLExecute           [ODBC32.012]
 */
SQLRETURN WINAPI SQLExecute(SQLHSTMT StatementHandle)
{
        TRACE("\n");

        if (!pSQLExecute) return SQL_ERROR;
        return pSQLExecute(StatementHandle);
}


/*************************************************************************
 *				SQLFetch           [ODBC32.013]
 */
SQLRETURN WINAPI SQLFetch(SQLHSTMT StatementHandle)
{
        TRACE("\n");

        if (!pSQLFetch) return SQL_ERROR;
        return pSQLFetch(StatementHandle);
}


/*************************************************************************
 *				SQLFetchScroll          [ODBC32.030]
 */
SQLRETURN WINAPI SQLFetchScroll(SQLHSTMT StatementHandle, SQLSMALLINT FetchOrientation, SQLLEN FetchOffset)
{
        TRACE("\n");

        if (!pSQLFetchScroll) return SQL_ERROR;
        return pSQLFetchScroll(StatementHandle, FetchOrientation, FetchOffset);
}


/*************************************************************************
 *				SQLFreeConnect           [ODBC32.014]
 */
SQLRETURN WINAPI SQLFreeConnect(SQLHDBC ConnectionHandle)
{
        SQLRETURN ret;
        TRACE("(Handle %p)\n", ConnectionHandle);

        if (!pSQLFreeConnect) return SQL_ERROR;

        ret = pSQLFreeConnect(ConnectionHandle);
        TRACE("Returns %d\n", ret);
        return ret;
}


/*************************************************************************
 *				SQLFreeEnv           [ODBC32.015]
 */
SQLRETURN WINAPI SQLFreeEnv(SQLHENV EnvironmentHandle)
{
        SQLRETURN ret;
        TRACE("(EnvironmentHandle %p)\n",EnvironmentHandle);

        if (!pSQLFreeEnv) return SQL_ERROR;

        ret = pSQLFreeEnv(EnvironmentHandle);
        TRACE("Returns %d\n", ret);
        return ret;
}


/*************************************************************************
 *				SQLFreeHandle           [ODBC32.031]
 */
SQLRETURN WINAPI SQLFreeHandle(SQLSMALLINT HandleType, SQLHANDLE Handle)
{
        SQLRETURN ret;
        TRACE("(Type %d, Handle %p)\n", HandleType, Handle);

        if (!pSQLFreeHandle) return SQL_ERROR;

        ret = pSQLFreeHandle(HandleType, Handle);
        TRACE ("Returns %d\n", ret);
        return ret;
}


/*************************************************************************
 *				SQLFreeStmt           [ODBC32.016]
 */
SQLRETURN WINAPI SQLFreeStmt(SQLHSTMT StatementHandle, SQLUSMALLINT Option)
{
        SQLRETURN ret;
        TRACE("(Handle %p, Option %d)\n", StatementHandle, Option);

        if (!pSQLFreeStmt) return SQL_ERROR;

        ret = pSQLFreeStmt(StatementHandle, Option);
        TRACE("Returns %d\n", ret);
        return ret;
}


/*************************************************************************
 *				SQLGetConnectAttr           [ODBC32.032]
 */
SQLRETURN WINAPI SQLGetConnectAttr(SQLHDBC ConnectionHandle,
             SQLINTEGER Attribute, SQLPOINTER Value,
             SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
        TRACE("\n");

        if (!pSQLGetConnectAttr) return SQL_ERROR;
        return pSQLGetConnectAttr(ConnectionHandle, Attribute, Value,
                                  BufferLength, StringLength);
}


/*************************************************************************
 *				SQLGetConnectOption       [ODBC32.042]
 */
SQLRETURN WINAPI SQLGetConnectOption(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLPOINTER Value)
{
        TRACE("\n");

        if (!pSQLGetConnectOption) return SQL_ERROR;
        return pSQLGetConnectOption(ConnectionHandle, Option, Value);
}


/*************************************************************************
 *				SQLGetCursorName           [ODBC32.017]
 */
SQLRETURN WINAPI SQLGetCursorName(SQLHSTMT StatementHandle,
             SQLCHAR *CursorName, SQLSMALLINT BufferLength,
             SQLSMALLINT *NameLength)
{
        TRACE("\n");

        if (!pSQLGetCursorName) return SQL_ERROR;
        return pSQLGetCursorName(StatementHandle, CursorName, BufferLength, NameLength);
}


/*************************************************************************
 *				SQLGetData           [ODBC32.043]
 */
SQLRETURN WINAPI SQLGetData(SQLHSTMT StatementHandle,
             SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
             SQLPOINTER TargetValue, SQLLEN BufferLength,
             SQLLEN *StrLen_or_Ind)
{
        TRACE("\n");

        if (!pSQLGetData) return SQL_ERROR;
        return pSQLGetData(StatementHandle, ColumnNumber, TargetType,
                           TargetValue, BufferLength, StrLen_or_Ind);
}


/*************************************************************************
 *				SQLGetDescField           [ODBC32.033]
 */
SQLRETURN WINAPI SQLGetDescField(SQLHDESC DescriptorHandle,
             SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
             SQLPOINTER Value, SQLINTEGER BufferLength,
             SQLINTEGER *StringLength)
{
        TRACE("\n");

        if (!pSQLGetDescField) return SQL_ERROR;
        return pSQLGetDescField(DescriptorHandle, RecNumber, FieldIdentifier,
                                Value, BufferLength, StringLength);
}


/*************************************************************************
 *				SQLGetDescRec           [ODBC32.034]
 */
SQLRETURN WINAPI SQLGetDescRec(SQLHDESC DescriptorHandle,
             SQLSMALLINT RecNumber, SQLCHAR *Name,
             SQLSMALLINT BufferLength, SQLSMALLINT *StringLength,
             SQLSMALLINT *Type, SQLSMALLINT *SubType,
             SQLLEN *Length, SQLSMALLINT *Precision,
             SQLSMALLINT *Scale, SQLSMALLINT *Nullable)
{
        TRACE("\n");

        if (!pSQLGetDescRec) return SQL_ERROR;
        return pSQLGetDescRec(DescriptorHandle, RecNumber, Name, BufferLength,
                              StringLength, Type, SubType, Length, Precision, Scale, Nullable);
}


/*************************************************************************
 *				SQLGetDiagField           [ODBC32.035]
 */
SQLRETURN WINAPI SQLGetDiagField(SQLSMALLINT HandleType, SQLHANDLE Handle,
             SQLSMALLINT RecNumber, SQLSMALLINT DiagIdentifier,
             SQLPOINTER DiagInfo, SQLSMALLINT BufferLength,
             SQLSMALLINT *StringLength)
{
        TRACE("\n");

        if (!pSQLGetDiagField) return SQL_ERROR;
        return pSQLGetDiagField(HandleType, Handle, RecNumber, DiagIdentifier,
                                DiagInfo, BufferLength, StringLength);
}


/*************************************************************************
 *				SQLGetDiagRec           [ODBC32.036]
 */
SQLRETURN WINAPI SQLGetDiagRec(SQLSMALLINT HandleType, SQLHANDLE Handle,
             SQLSMALLINT RecNumber, SQLCHAR *Sqlstate,
             SQLINTEGER *NativeError, SQLCHAR *MessageText,
             SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
        TRACE("\n");

        if (!pSQLGetDiagRec) return SQL_ERROR;
        return pSQLGetDiagRec(HandleType, Handle, RecNumber, Sqlstate, NativeError,
                              MessageText, BufferLength, TextLength);
}


/*************************************************************************
 *				SQLGetEnvAttr           [ODBC32.037]
 */
SQLRETURN WINAPI SQLGetEnvAttr(SQLHENV EnvironmentHandle,
             SQLINTEGER Attribute, SQLPOINTER Value,
             SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
        TRACE("\n");

        if (!pSQLGetEnvAttr) return SQL_ERROR;
        return pSQLGetEnvAttr(EnvironmentHandle, Attribute, Value, BufferLength, StringLength);
}


/*************************************************************************
 *				SQLGetFunctions           [ODBC32.044]
 */
SQLRETURN WINAPI SQLGetFunctions(SQLHDBC ConnectionHandle, SQLUSMALLINT FunctionId, SQLUSMALLINT *Supported)
{
        TRACE("\n");

        if (!pSQLGetFunctions) return SQL_ERROR;
        return pSQLGetFunctions(ConnectionHandle, FunctionId, Supported);
}


/*************************************************************************
 *				SQLGetInfo           [ODBC32.045]
 */
SQLRETURN WINAPI SQLGetInfo(SQLHDBC ConnectionHandle,
             SQLUSMALLINT InfoType, SQLPOINTER InfoValue,
             SQLSMALLINT BufferLength, SQLSMALLINT *StringLength)
{
        TRACE("\n");

        if (!pSQLGetInfo) return SQL_ERROR;
        return pSQLGetInfo(ConnectionHandle, InfoType, InfoValue, BufferLength, StringLength);
}


/*************************************************************************
 *				SQLGetStmtAttr           [ODBC32.038]
 */
SQLRETURN WINAPI SQLGetStmtAttr(SQLHSTMT StatementHandle,
             SQLINTEGER Attribute, SQLPOINTER Value,
             SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
        TRACE("\n");

        if (!pSQLGetStmtAttr) return SQL_ERROR;
        return pSQLGetStmtAttr(StatementHandle, Attribute, Value, BufferLength, StringLength);
}


/*************************************************************************
 *				SQLGetStmtOption           [ODBC32.046]
 */
SQLRETURN WINAPI SQLGetStmtOption(SQLHSTMT StatementHandle, SQLUSMALLINT Option, SQLPOINTER Value)
{
        TRACE("\n");

        if (!pSQLGetStmtOption) return SQL_ERROR;
        return pSQLGetStmtOption(StatementHandle, Option, Value);
}


/*************************************************************************
 *				SQLGetTypeInfo           [ODBC32.047]
 */
SQLRETURN WINAPI SQLGetTypeInfo(SQLHSTMT StatementHandle, SQLSMALLINT DataType)
{
        TRACE("\n");

        if (!pSQLGetTypeInfo) return SQL_ERROR;
        return pSQLGetTypeInfo(StatementHandle, DataType);
}


/*************************************************************************
 *				SQLNumResultCols           [ODBC32.018]
 */
SQLRETURN WINAPI SQLNumResultCols(SQLHSTMT StatementHandle, SQLSMALLINT *ColumnCount)
{
        TRACE("\n");

        if (!pSQLNumResultCols) return SQL_ERROR;
        return pSQLNumResultCols(StatementHandle, ColumnCount);
}


/*************************************************************************
 *				SQLParamData           [ODBC32.048]
 */
SQLRETURN WINAPI SQLParamData(SQLHSTMT StatementHandle, SQLPOINTER *Value)
{
        TRACE("\n");

        if (!pSQLParamData) return SQL_ERROR;
        return pSQLParamData(StatementHandle, Value);
}


/*************************************************************************
 *				SQLPrepare           [ODBC32.019]
 */
SQLRETURN WINAPI SQLPrepare(SQLHSTMT StatementHandle, SQLCHAR *StatementText, SQLINTEGER TextLength)
{
        TRACE("\n");

        if (!pSQLPrepare) return SQL_ERROR;
        return pSQLPrepare(StatementHandle, StatementText, TextLength);
}


/*************************************************************************
 *				SQLPutData           [ODBC32.049]
 */
SQLRETURN WINAPI SQLPutData(SQLHSTMT StatementHandle, SQLPOINTER Data, SQLLEN StrLen_or_Ind)
{
        TRACE("\n");

        if (!pSQLPutData) return SQL_ERROR;
        return pSQLPutData(StatementHandle, Data, StrLen_or_Ind);
}


/*************************************************************************
 *				SQLRowCount           [ODBC32.020]
 */
SQLRETURN WINAPI SQLRowCount(SQLHSTMT StatementHandle, SQLLEN *RowCount)
{
        TRACE("\n");

        if (!pSQLRowCount) return SQL_ERROR;
        return pSQLRowCount(StatementHandle, RowCount);
}


/*************************************************************************
 *				SQLSetConnectAttr           [ODBC32.039]
 */
SQLRETURN WINAPI SQLSetConnectAttr(SQLHDBC ConnectionHandle, SQLINTEGER Attribute,
        SQLPOINTER Value, SQLINTEGER StringLength)
{
        TRACE("\n");

        if (!pSQLSetConnectAttr) return SQL_ERROR;
        return pSQLSetConnectAttr(ConnectionHandle, Attribute, Value, StringLength);
}


/*************************************************************************
 *				SQLSetConnectOption           [ODBC32.050]
 */
SQLRETURN WINAPI SQLSetConnectOption(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLULEN Value)
{
        TRACE("\n");

        if (!pSQLSetConnectOption) return SQL_ERROR;
        return pSQLSetConnectOption(ConnectionHandle, Option, Value);
}


/*************************************************************************
 *				SQLSetCursorName           [ODBC32.021]
 */
SQLRETURN WINAPI SQLSetCursorName(SQLHSTMT StatementHandle, SQLCHAR *CursorName, SQLSMALLINT NameLength)
{
        TRACE("\n");

        if (!pSQLSetCursorName) return SQL_ERROR;
        return pSQLSetCursorName(StatementHandle, CursorName, NameLength);
}


/*************************************************************************
 *				SQLSetDescField           [ODBC32.073]
 */
SQLRETURN WINAPI SQLSetDescField(SQLHDESC DescriptorHandle,
             SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
             SQLPOINTER Value, SQLINTEGER BufferLength)
{
        TRACE("\n");

        if (!pSQLSetDescField) return SQL_ERROR;
        return pSQLSetDescField(DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength);
}


/*************************************************************************
 *				SQLSetDescRec           [ODBC32.074]
 */
SQLRETURN WINAPI SQLSetDescRec(SQLHDESC DescriptorHandle,
             SQLSMALLINT RecNumber, SQLSMALLINT Type,
             SQLSMALLINT SubType, SQLLEN Length,
             SQLSMALLINT Precision, SQLSMALLINT Scale,
             SQLPOINTER Data, SQLLEN *StringLength,
             SQLLEN *Indicator)
{
        TRACE("\n");

        if (!pSQLSetDescRec) return SQL_ERROR;
        return pSQLSetDescRec(DescriptorHandle, RecNumber, Type, SubType, Length,
                              Precision, Scale, Data, StringLength, Indicator);
}


/*************************************************************************
 *				SQLSetEnvAttr           [ODBC32.075]
 */
SQLRETURN WINAPI SQLSetEnvAttr(SQLHENV EnvironmentHandle,
             SQLINTEGER Attribute, SQLPOINTER Value,
             SQLINTEGER StringLength)
{
        TRACE("\n");

        if (!pSQLSetEnvAttr) return SQL_ERROR;
        return pSQLSetEnvAttr(EnvironmentHandle, Attribute, Value, StringLength);
}


/*************************************************************************
 *				SQLSetParam           [ODBC32.022]
 */
SQLRETURN WINAPI SQLSetParam(SQLHSTMT StatementHandle,
             SQLUSMALLINT ParameterNumber, SQLSMALLINT ValueType,
             SQLSMALLINT ParameterType, SQLULEN LengthPrecision,
             SQLSMALLINT ParameterScale, SQLPOINTER ParameterValue,
             SQLLEN *StrLen_or_Ind)
{
        TRACE("\n");

        if (!pSQLSetParam) return SQL_ERROR;
        return pSQLSetParam(StatementHandle, ParameterNumber, ValueType, ParameterType, LengthPrecision,
                            ParameterScale, ParameterValue, StrLen_or_Ind);
}


/*************************************************************************
 *				SQLSetStmtAttr           [ODBC32.076]
 */
SQLRETURN WINAPI SQLSetStmtAttr(SQLHSTMT StatementHandle,
                 SQLINTEGER Attribute, SQLPOINTER Value,
                 SQLINTEGER StringLength)
{
        TRACE("\n");

        if (!pSQLSetStmtAttr) return SQL_ERROR;
        return pSQLSetStmtAttr(StatementHandle, Attribute, Value, StringLength);
}


/*************************************************************************
 *				SQLSetStmtOption           [ODBC32.051]
 */
SQLRETURN WINAPI SQLSetStmtOption(SQLHSTMT StatementHandle, SQLUSMALLINT Option, SQLULEN Value)
{
        TRACE("\n");

        if (!pSQLSetStmtOption) return SQL_ERROR;
        return pSQLSetStmtOption(StatementHandle, Option, Value);
}


/*************************************************************************
 *				SQLSpecialColumns           [ODBC32.052]
 */
SQLRETURN WINAPI SQLSpecialColumns(SQLHSTMT StatementHandle,
             SQLUSMALLINT IdentifierType, SQLCHAR *CatalogName,
             SQLSMALLINT NameLength1, SQLCHAR *SchemaName,
             SQLSMALLINT NameLength2, SQLCHAR *TableName,
             SQLSMALLINT NameLength3, SQLUSMALLINT Scope,
             SQLUSMALLINT Nullable)
{

        if (!pSQLSpecialColumns) return SQL_ERROR;
        return pSQLSpecialColumns(StatementHandle, IdentifierType, CatalogName, NameLength1, SchemaName,
                                  NameLength2, TableName, NameLength3, Scope, Nullable);
}


/*************************************************************************
 *				SQLStatistics           [ODBC32.053]
 */
SQLRETURN WINAPI SQLStatistics(SQLHSTMT StatementHandle,
             SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
             SQLCHAR *SchemaName, SQLSMALLINT NameLength2,
             SQLCHAR *TableName, SQLSMALLINT NameLength3,
             SQLUSMALLINT Unique, SQLUSMALLINT Reserved)
{
        TRACE("\n");

        if (!pSQLStatistics) return SQL_ERROR;
        return pSQLStatistics(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2,
                              TableName, NameLength3, Unique, Reserved);
}


/*************************************************************************
 *				SQLTables           [ODBC32.054]
 */
SQLRETURN WINAPI SQLTables(SQLHSTMT StatementHandle,
             SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
             SQLCHAR *SchemaName, SQLSMALLINT NameLength2,
             SQLCHAR *TableName, SQLSMALLINT NameLength3,
             SQLCHAR *TableType, SQLSMALLINT NameLength4)
{
        TRACE("\n");

        if (!pSQLTables) return SQL_ERROR;
        return pSQLTables(StatementHandle, CatalogName, NameLength1,
                          SchemaName, NameLength2, TableName, NameLength3, TableType, NameLength4);
}


/*************************************************************************
 *				SQLTransact           [ODBC32.023]
 */
SQLRETURN WINAPI SQLTransact(SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle,
        SQLUSMALLINT CompletionType)
{
        TRACE("\n");

        if (!pSQLTransact) return SQL_ERROR;
        return pSQLTransact(EnvironmentHandle, ConnectionHandle, CompletionType);
}


/*************************************************************************
 *				SQLBrowseConnect           [ODBC32.055]
 */
SQLRETURN WINAPI SQLBrowseConnect(
    SQLHDBC            hdbc,
    SQLCHAR               *szConnStrIn,
    SQLSMALLINT        cbConnStrIn,
    SQLCHAR               *szConnStrOut,
    SQLSMALLINT        cbConnStrOutMax,
    SQLSMALLINT       *pcbConnStrOut)
{
        TRACE("\n");

        if (!pSQLBrowseConnect) return SQL_ERROR;
        return pSQLBrowseConnect(hdbc, szConnStrIn, cbConnStrIn, szConnStrOut, cbConnStrOutMax, pcbConnStrOut);
}


/*************************************************************************
 *				SQLBulkOperations           [ODBC32.078]
 */
SQLRETURN WINAPI  SQLBulkOperations(
        SQLHSTMT                        StatementHandle,
        SQLSMALLINT                     Operation)
{
        TRACE("\n");

        if (!pSQLBulkOperations) return SQL_ERROR;
        return pSQLBulkOperations(StatementHandle, Operation);
}


/*************************************************************************
 *				SQLColAttributes           [ODBC32.006]
 */
SQLRETURN WINAPI SQLColAttributes(
    SQLHSTMT           hstmt,
    SQLUSMALLINT       icol,
    SQLUSMALLINT       fDescType,
    SQLPOINTER         rgbDesc,
    SQLSMALLINT        cbDescMax,
    SQLSMALLINT           *pcbDesc,
    SQLLEN            *pfDesc)
{
        TRACE("\n");

        if (!pSQLColAttributes) return SQL_ERROR;
        return pSQLColAttributes(hstmt, icol, fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc);
}


/*************************************************************************
 *				SQLColumnPrivileges           [ODBC32.056]
 */
SQLRETURN WINAPI SQLColumnPrivileges(
    SQLHSTMT           hstmt,
    SQLCHAR               *szCatalogName,
    SQLSMALLINT        cbCatalogName,
    SQLCHAR               *szSchemaName,
    SQLSMALLINT        cbSchemaName,
    SQLCHAR               *szTableName,
    SQLSMALLINT        cbTableName,
    SQLCHAR               *szColumnName,
    SQLSMALLINT        cbColumnName)
{
        TRACE("\n");

        if (!pSQLColumnPrivileges) return SQL_ERROR;
        return pSQLColumnPrivileges(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName,
                                    szTableName, cbTableName, szColumnName, cbColumnName);
}


/*************************************************************************
 *				SQLDescribeParam          [ODBC32.058]
 */
SQLRETURN WINAPI SQLDescribeParam(
    SQLHSTMT           hstmt,
    SQLUSMALLINT       ipar,
    SQLSMALLINT           *pfSqlType,
    SQLULEN           *pcbParamDef,
    SQLSMALLINT           *pibScale,
    SQLSMALLINT           *pfNullable)
{
        TRACE("\n");

        if (!pSQLDescribeParam) return SQL_ERROR;
        return pSQLDescribeParam(hstmt, ipar, pfSqlType, pcbParamDef, pibScale, pfNullable);
}


/*************************************************************************
 *				SQLExtendedFetch           [ODBC32.059]
 */
SQLRETURN WINAPI SQLExtendedFetch(
    SQLHSTMT           hstmt,
    SQLUSMALLINT       fFetchType,
    SQLINTEGER         irow,
    SQLUINTEGER           *pcrow,
    SQLUSMALLINT          *rgfRowStatus)
{
        TRACE("\n");

        if (!pSQLExtendedFetch) return SQL_ERROR;
        return pSQLExtendedFetch(hstmt, fFetchType, irow, pcrow, rgfRowStatus);
}


/*************************************************************************
 *				SQLForeignKeys           [ODBC32.060]
 */
SQLRETURN WINAPI SQLForeignKeys(
    SQLHSTMT           hstmt,
    SQLCHAR               *szPkCatalogName,
    SQLSMALLINT        cbPkCatalogName,
    SQLCHAR               *szPkSchemaName,
    SQLSMALLINT        cbPkSchemaName,
    SQLCHAR               *szPkTableName,
    SQLSMALLINT        cbPkTableName,
    SQLCHAR               *szFkCatalogName,
    SQLSMALLINT        cbFkCatalogName,
    SQLCHAR               *szFkSchemaName,
    SQLSMALLINT        cbFkSchemaName,
    SQLCHAR               *szFkTableName,
    SQLSMALLINT        cbFkTableName)
{
        TRACE("\n");

        if (!pSQLForeignKeys) return SQL_ERROR;
        return pSQLForeignKeys(hstmt, szPkCatalogName, cbPkCatalogName, szPkSchemaName, cbPkSchemaName,
                               szPkTableName, cbPkTableName, szFkCatalogName, cbFkCatalogName,
                               szFkSchemaName, cbFkSchemaName, szFkTableName, cbFkTableName);
}


/*************************************************************************
 *				SQLMoreResults           [ODBC32.061]
 */
SQLRETURN WINAPI SQLMoreResults(SQLHSTMT hstmt)
{
        TRACE("\n");

        if (!pSQLMoreResults) return SQL_ERROR;
        return pSQLMoreResults(hstmt);
}


/*************************************************************************
 *				SQLNativeSql           [ODBC32.062]
 */
SQLRETURN WINAPI SQLNativeSql(
    SQLHDBC            hdbc,
    SQLCHAR               *szSqlStrIn,
    SQLINTEGER         cbSqlStrIn,
    SQLCHAR               *szSqlStr,
    SQLINTEGER         cbSqlStrMax,
    SQLINTEGER            *pcbSqlStr)
{
        TRACE("\n");

        if (!pSQLNativeSql) return SQL_ERROR;
        return pSQLNativeSql(hdbc, szSqlStrIn, cbSqlStrIn, szSqlStr, cbSqlStrMax, pcbSqlStr);
}


/*************************************************************************
 *				SQLNumParams           [ODBC32.063]
 */
SQLRETURN WINAPI SQLNumParams(
    SQLHSTMT           hstmt,
    SQLSMALLINT           *pcpar)
{
        TRACE("\n");

        if (!pSQLNumParams) return SQL_ERROR;
        return pSQLNumParams(hstmt, pcpar);
}


/*************************************************************************
 *				SQLParamOptions           [ODBC32.064]
 */
SQLRETURN WINAPI SQLParamOptions(
    SQLHSTMT           hstmt,
    SQLUINTEGER        crow,
    SQLUINTEGER           *pirow)
{
        TRACE("\n");

        if (!pSQLParamOptions) return SQL_ERROR;
        return pSQLParamOptions(hstmt, crow, pirow);
}


/*************************************************************************
 *				SQLPrimaryKeys           [ODBC32.065]
 */
SQLRETURN WINAPI SQLPrimaryKeys(
    SQLHSTMT           hstmt,
    SQLCHAR               *szCatalogName,
    SQLSMALLINT        cbCatalogName,
    SQLCHAR               *szSchemaName,
    SQLSMALLINT        cbSchemaName,
    SQLCHAR               *szTableName,
    SQLSMALLINT        cbTableName)
{
        TRACE("\n");

        if (!pSQLPrimaryKeys) return SQL_ERROR;
        return pSQLPrimaryKeys(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName,
                               szTableName, cbTableName);
}


/*************************************************************************
 *				SQLProcedureColumns           [ODBC32.066]
 */
SQLRETURN WINAPI SQLProcedureColumns(
    SQLHSTMT           hstmt,
    SQLCHAR               *szCatalogName,
    SQLSMALLINT        cbCatalogName,
    SQLCHAR               *szSchemaName,
    SQLSMALLINT        cbSchemaName,
    SQLCHAR               *szProcName,
    SQLSMALLINT        cbProcName,
    SQLCHAR               *szColumnName,
    SQLSMALLINT        cbColumnName)
{
        TRACE("\n");

        if (!pSQLProcedureColumns) return SQL_ERROR;
        return pSQLProcedureColumns(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName,
                                    szProcName, cbProcName, szColumnName, cbColumnName);
}


/*************************************************************************
 *				SQLProcedures           [ODBC32.067]
 */
SQLRETURN WINAPI SQLProcedures(
    SQLHSTMT           hstmt,
    SQLCHAR               *szCatalogName,
    SQLSMALLINT        cbCatalogName,
    SQLCHAR               *szSchemaName,
    SQLSMALLINT        cbSchemaName,
    SQLCHAR               *szProcName,
    SQLSMALLINT        cbProcName)
{
        TRACE("\n");

        if (!pSQLProcedures) return SQL_ERROR;
        return pSQLProcedures(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName,
                              szProcName, cbProcName);
}


/*************************************************************************
 *				SQLSetPos           [ODBC32.068]
 */
SQLRETURN WINAPI SQLSetPos(
    SQLHSTMT           hstmt,
    SQLUSMALLINT       irow,
    SQLUSMALLINT       fOption,
    SQLUSMALLINT       fLock)
{
        TRACE("\n");

        if (!pSQLSetPos) return SQL_ERROR;
        return pSQLSetPos(hstmt, irow, fOption, fLock);
}


/*************************************************************************
 *				SQLTablePrivileges           [ODBC32.070]
 */
SQLRETURN WINAPI SQLTablePrivileges(
    SQLHSTMT           hstmt,
    SQLCHAR               *szCatalogName,
    SQLSMALLINT        cbCatalogName,
    SQLCHAR               *szSchemaName,
    SQLSMALLINT        cbSchemaName,
    SQLCHAR               *szTableName,
    SQLSMALLINT        cbTableName)
{
        TRACE("\n");

        if (!pSQLTablePrivileges) return SQL_ERROR;
        return pSQLTablePrivileges(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName,
                                   szTableName, cbTableName);
}


/*************************************************************************
 *				SQLDrivers           [ODBC32.071]
 */
SQLRETURN WINAPI SQLDrivers(
    SQLHENV            henv,
    SQLUSMALLINT       fDirection,
    SQLCHAR               *szDriverDesc,
    SQLSMALLINT        cbDriverDescMax,
    SQLSMALLINT           *pcbDriverDesc,
    SQLCHAR               *szDriverAttributes,
    SQLSMALLINT        cbDriverAttrMax,
    SQLSMALLINT           *pcbDriverAttr)
{
        SQLRETURN ret;

        TRACE("(Direction %d)\n", fDirection);

        if (!pSQLDrivers) return SQL_ERROR;
        ret = pSQLDrivers(henv, fDirection, szDriverDesc, cbDriverDescMax, pcbDriverDesc,
                          szDriverAttributes, cbDriverAttrMax, pcbDriverAttr);

        if (ret == SQL_NO_DATA && fDirection == SQL_FETCH_FIRST)
            ERR_(winediag)("No ODBC drivers could be found. "
                           "Check the settings for your libodbc provider.\n");

        return ret;
}


/*************************************************************************
 *				SQLBindParameter           [ODBC32.072]
 */
SQLRETURN WINAPI SQLBindParameter(
    SQLHSTMT           hstmt,
    SQLUSMALLINT       ipar,
    SQLSMALLINT        fParamType,
    SQLSMALLINT        fCType,
    SQLSMALLINT        fSqlType,
    SQLULEN        cbColDef,
    SQLSMALLINT        ibScale,
    SQLPOINTER         rgbValue,
    SQLLEN         cbValueMax,
    SQLLEN            *pcbValue)
{
        TRACE("\n");

        if (!pSQLBindParameter) return SQL_ERROR;
        return pSQLBindParameter(hstmt, ipar, fParamType, fCType, fSqlType, cbColDef, ibScale,
                                 rgbValue, cbValueMax, pcbValue);
}


/*************************************************************************
 *				SQLDriverConnect           [ODBC32.041]
 */
SQLRETURN WINAPI SQLDriverConnect(
    SQLHDBC            hdbc,
    SQLHWND            hwnd,
    SQLCHAR            *conn_str_in,
    SQLSMALLINT        len_conn_str_in,
    SQLCHAR            *conn_str_out,
    SQLSMALLINT        conn_str_out_max,
    SQLSMALLINT        *ptr_conn_str_out,
    SQLUSMALLINT       driver_completion )
{
        SQLRETURN ret;

        TRACE("(ConnectionString %s, Length %d)\n",
              debugstr_a((char *)conn_str_in), len_conn_str_in);

        if (!pSQLDriverConnect) return SQL_ERROR;
        ret = pSQLDriverConnect(hdbc, hwnd, conn_str_in, len_conn_str_in, conn_str_out,
                                 conn_str_out_max, ptr_conn_str_out, driver_completion);
        TRACE("Returns %d\n", ret);
        return ret;
}


/*************************************************************************
 *				SQLSetScrollOptions           [ODBC32.069]
 */
SQLRETURN WINAPI SQLSetScrollOptions(
    SQLHSTMT           statement_handle,
    SQLUSMALLINT       f_concurrency,
    SQLLEN         crow_keyset,
    SQLUSMALLINT       crow_rowset )
{
        TRACE("\n");

        if (!pSQLSetScrollOptions) return SQL_ERROR;
        return pSQLSetScrollOptions(statement_handle, f_concurrency, crow_keyset, crow_rowset);
}

static BOOL SQLColAttributes_KnownStringAttribute(SQLUSMALLINT fDescType)
{
    static const SQLUSMALLINT attrList[] =
    {
        SQL_COLUMN_OWNER_NAME,
        SQL_COLUMN_QUALIFIER_NAME,
        SQL_COLUMN_LABEL,
        SQL_COLUMN_NAME,
        SQL_COLUMN_TABLE_NAME,
        SQL_COLUMN_TYPE_NAME,
        SQL_DESC_BASE_COLUMN_NAME,
        SQL_DESC_BASE_TABLE_NAME,
        SQL_DESC_CATALOG_NAME,
        SQL_DESC_LABEL,
        SQL_DESC_LITERAL_PREFIX,
        SQL_DESC_LITERAL_SUFFIX,
        SQL_DESC_LOCAL_TYPE_NAME,
        SQL_DESC_NAME,
        SQL_DESC_SCHEMA_NAME,
        SQL_DESC_TABLE_NAME,
        SQL_DESC_TYPE_NAME,
    };
    unsigned int i;

    for (i = 0; i < sizeof(attrList) / sizeof(SQLUSMALLINT); i++) {
        if (attrList[i] == fDescType) return TRUE;
    }
    return FALSE;
}

/*************************************************************************
 *				SQLColAttributesW          [ODBC32.106]
 */
SQLRETURN WINAPI SQLColAttributesW(
    SQLHSTMT           hstmt,
    SQLUSMALLINT       icol,
    SQLUSMALLINT       fDescType,
    SQLPOINTER         rgbDesc,
    SQLSMALLINT        cbDescMax,
    SQLSMALLINT           *pcbDesc,
    SQLLEN            *pfDesc)
{
        SQLRETURN iResult;

        TRACE("hstmt=%p icol=%d fDescType=%d rgbDesc=%p cbDescMax=%d pcbDesc=%p pfDesc=%p\n",
            hstmt, icol, fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc);

        if (!pSQLColAttributesW) return SQL_ERROR;

        iResult = pSQLColAttributesW(hstmt, icol, fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc);
        if (iResult == SQL_SUCCESS && rgbDesc != NULL && SQLColAttributes_KnownStringAttribute(fDescType)) {
        /*
            TRACE("Dumping values fetched via SQLColAttributesW:\n");
            TRACE("    Attribute name : %s\n", debugstr_w(rgbDesc));
            TRACE("    Declared length: %d\n", *pcbDesc);
        */
            if (*pcbDesc != lstrlenW(rgbDesc) * 2) {
                TRACE("CHEAT: resetting name length for ADO\n");
                *pcbDesc = lstrlenW(rgbDesc) * 2;
            }
        }
        return iResult;
}

/*************************************************************************
 *				SQLConnectW          [ODBC32.107]
 */
SQLRETURN WINAPI SQLConnectW(SQLHDBC ConnectionHandle,
             WCHAR *ServerName, SQLSMALLINT NameLength1,
             WCHAR *UserName, SQLSMALLINT NameLength2,
             WCHAR *Authentication, SQLSMALLINT NameLength3)
{
        SQLRETURN ret;
        TRACE("(Server=%.*s)\n",NameLength1+3, debugstr_w(ServerName));

        if (!pSQLConnectW) return SQL_ERROR;

        ret = pSQLConnectW(ConnectionHandle, ServerName, NameLength1,
                           UserName, NameLength2, Authentication, NameLength3);

        TRACE("Returns %d\n", ret);
        return ret;
}

/*************************************************************************
 *				SQLDescribeColW          [ODBC32.108]
 */
SQLRETURN WINAPI SQLDescribeColW(SQLHSTMT StatementHandle,
             SQLUSMALLINT ColumnNumber, WCHAR *ColumnName,
             SQLSMALLINT BufferLength, SQLSMALLINT *NameLength,
             SQLSMALLINT *DataType, SQLULEN *ColumnSize,
             SQLSMALLINT *DecimalDigits, SQLSMALLINT *Nullable)
{
        SQLRETURN iResult;
        TRACE("\n");

        if (!pSQLDescribeColW) return SQL_ERROR;

        iResult = pSQLDescribeColW(StatementHandle, ColumnNumber, ColumnName,
                                   BufferLength, NameLength, DataType, ColumnSize,
                                   DecimalDigits, Nullable);
        if (iResult >= 0) {
            TRACE("Successfully recovered the following column information:\n");
            TRACE("\tRequested column index: %d\n", ColumnNumber);
            TRACE("\tAvailable length for column name: %d\n", BufferLength);
            if (NameLength != NULL)
                TRACE("\tActual length for column name: %d\n", *NameLength);
            else TRACE("\tActual length for column name: (null)\n");
            TRACE("\tReturned column name: %s\n", debugstr_w(ColumnName));
        }
        return iResult;
}

/*************************************************************************
 *				SQLErrorW          [ODBC32.110]
 */
SQLRETURN WINAPI SQLErrorW(SQLHENV EnvironmentHandle,
             SQLHDBC ConnectionHandle, SQLHSTMT StatementHandle,
             WCHAR *Sqlstate, SQLINTEGER *NativeError,
             WCHAR *MessageText, SQLSMALLINT BufferLength,
             SQLSMALLINT *TextLength)
{
        TRACE("\n");

        if (!pSQLErrorW) return SQL_ERROR;
        return pSQLErrorW(EnvironmentHandle, ConnectionHandle, StatementHandle,
                          Sqlstate, NativeError, MessageText, BufferLength, TextLength);
}

/*************************************************************************
 *				SQLExecDirectW          [ODBC32.111]
 */
SQLRETURN WINAPI SQLExecDirectW(SQLHSTMT StatementHandle,
    WCHAR *StatementText, SQLINTEGER TextLength)
{
        TRACE("\n");

        if (!pSQLExecDirectW) return SQL_ERROR;
        return pSQLExecDirectW(StatementHandle, StatementText, TextLength);
}

/*************************************************************************
 *				SQLGetCursorNameW          [ODBC32.117]
 */
SQLRETURN WINAPI SQLGetCursorNameW(SQLHSTMT StatementHandle,
             WCHAR *CursorName, SQLSMALLINT BufferLength,
             SQLSMALLINT *NameLength)
{
        TRACE("\n");

        if (!pSQLGetCursorNameW) return SQL_ERROR;
        return pSQLGetCursorNameW(StatementHandle, CursorName, BufferLength, NameLength);
}

/*************************************************************************
 *				SQLPrepareW          [ODBC32.119]
 */
SQLRETURN WINAPI SQLPrepareW(SQLHSTMT StatementHandle,
    WCHAR *StatementText, SQLINTEGER TextLength)
{
        TRACE("\n");

        if (!pSQLPrepareW) return SQL_ERROR;
        return pSQLPrepareW(StatementHandle, StatementText, TextLength);
}

/*************************************************************************
 *				SQLSetCursorNameW          [ODBC32.121]
 */
SQLRETURN WINAPI SQLSetCursorNameW(SQLHSTMT StatementHandle, WCHAR *CursorName, SQLSMALLINT NameLength)
{
        TRACE("\n");

        if (!pSQLSetCursorNameW) return SQL_ERROR;
        return pSQLSetCursorNameW(StatementHandle, CursorName, NameLength);
}

/*************************************************************************
 *				SQLColAttributeW          [ODBC32.127]
 */
SQLRETURN WINAPI SQLColAttributeW (SQLHSTMT StatementHandle,
             SQLUSMALLINT ColumnNumber, SQLUSMALLINT FieldIdentifier,
             SQLPOINTER CharacterAttribute, SQLSMALLINT BufferLength,
             SQLSMALLINT *StringLength, SQLPOINTER NumericAttribute)
{
        SQLRETURN iResult;

        TRACE("StatementHandle=%p ColumnNumber=%d FieldIdentifier=%d CharacterAttribute=%p BufferLength=%d StringLength=%p NumericAttribute=%p\n",
            StatementHandle, ColumnNumber, FieldIdentifier,
            CharacterAttribute, BufferLength, StringLength, NumericAttribute);

        if (!pSQLColAttributeW) return SQL_ERROR;

        iResult = pSQLColAttributeW(StatementHandle, ColumnNumber, FieldIdentifier,
                                    CharacterAttribute, BufferLength, StringLength, NumericAttribute);
        if (iResult == SQL_SUCCESS && CharacterAttribute != NULL && SQLColAttributes_KnownStringAttribute(FieldIdentifier)) {
        /*
            TRACE("Dumping values fetched via SQLColAttributeW:\n");
            TRACE("    Attribute name : %s\n", debugstr_w(rgbDesc));
            TRACE("    Declared length: %d\n", *pcbDesc);
        */
            if (*StringLength != lstrlenW(CharacterAttribute) * 2) {
                TRACE("CHEAT: resetting name length for ADO\n");
                *StringLength = lstrlenW(CharacterAttribute) * 2;
            }
        }
        return iResult;
}

/*************************************************************************
 *				SQLGetConnectAttrW          [ODBC32.132]
 */
SQLRETURN WINAPI SQLGetConnectAttrW(SQLHDBC ConnectionHandle,
             SQLINTEGER Attribute, SQLPOINTER Value,
             SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
        TRACE("\n");

        if (!pSQLGetConnectAttrW) return SQL_ERROR;
        return pSQLGetConnectAttrW(ConnectionHandle, Attribute, Value,
                                   BufferLength, StringLength);
}

/*************************************************************************
 *				SQLGetDescFieldW          [ODBC32.133]
 */
SQLRETURN WINAPI SQLGetDescFieldW(SQLHDESC DescriptorHandle,
             SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
             SQLPOINTER Value, SQLINTEGER BufferLength,
             SQLINTEGER *StringLength)
{
        TRACE("\n");

        if (!pSQLGetDescFieldW) return SQL_ERROR;
        return pSQLGetDescFieldW(DescriptorHandle, RecNumber, FieldIdentifier,
                                 Value, BufferLength, StringLength);
}

/*************************************************************************
 *				SQLGetDescRecW          [ODBC32.134]
 */
SQLRETURN WINAPI SQLGetDescRecW(SQLHDESC DescriptorHandle,
             SQLSMALLINT RecNumber, WCHAR *Name,
             SQLSMALLINT BufferLength, SQLSMALLINT *StringLength,
             SQLSMALLINT *Type, SQLSMALLINT *SubType,
             SQLLEN *Length, SQLSMALLINT *Precision,
             SQLSMALLINT *Scale, SQLSMALLINT *Nullable)
{
        TRACE("\n");

        if (!pSQLGetDescRecW) return SQL_ERROR;
        return pSQLGetDescRecW(DescriptorHandle, RecNumber, Name, BufferLength,
                               StringLength, Type, SubType, Length, Precision, Scale, Nullable);
}

/*************************************************************************
 *				SQLGetDiagFieldW          [ODBC32.135]
 */
SQLRETURN WINAPI SQLGetDiagFieldW(SQLSMALLINT HandleType, SQLHANDLE Handle,
             SQLSMALLINT RecNumber, SQLSMALLINT DiagIdentifier,
             SQLPOINTER DiagInfo, SQLSMALLINT BufferLength,
             SQLSMALLINT *StringLength)
{
        TRACE("\n");

        if (!pSQLGetDiagFieldW) return SQL_ERROR;
        return pSQLGetDiagFieldW(HandleType, Handle, RecNumber, DiagIdentifier,
                                 DiagInfo, BufferLength, StringLength);
}

/*************************************************************************
 *				SQLGetDiagRecW           [ODBC32.136]
 */
SQLRETURN WINAPI SQLGetDiagRecW(SQLSMALLINT HandleType, SQLHANDLE Handle,
             SQLSMALLINT RecNumber, WCHAR *Sqlstate,
             SQLINTEGER *NativeError, WCHAR *MessageText,
             SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
        TRACE("\n");

        if (!pSQLGetDiagRecW) return SQL_ERROR;
        return pSQLGetDiagRecW(HandleType, Handle, RecNumber, Sqlstate, NativeError,
                               MessageText, BufferLength, TextLength);
}

/*************************************************************************
 *				SQLGetStmtAttrW          [ODBC32.138]
 */
SQLRETURN WINAPI SQLGetStmtAttrW(SQLHSTMT StatementHandle,
             SQLINTEGER Attribute, SQLPOINTER Value,
             SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
        SQLRETURN iResult;

        TRACE("Attribute = (%02d) Value = %p BufferLength = (%d) StringLength = %p\n",
            Attribute, Value, BufferLength, StringLength);

        if (Value == NULL) {
            WARN("Unexpected NULL in Value return address\n");
            iResult = SQL_ERROR;
/*
        } else if (StringLength == NULL) {
            WARN("Unexpected NULL in StringLength return address\n");
            iResult = SQL_ERROR;
*/
        } else {
            if (!pSQLGetStmtAttrW) return SQL_ERROR;
            iResult = pSQLGetStmtAttrW(StatementHandle, Attribute, Value, BufferLength, StringLength);
            TRACE("returning %d...\n", iResult);
        }
        return iResult;
}

/*************************************************************************
 *				SQLSetConnectAttrW          [ODBC32.139]
 */
SQLRETURN WINAPI SQLSetConnectAttrW(SQLHDBC ConnectionHandle, SQLINTEGER Attribute,
        SQLPOINTER Value, SQLINTEGER StringLength)
{
        TRACE("\n");

        if (!pSQLSetConnectAttrW) return SQL_ERROR;
        return pSQLSetConnectAttrW(ConnectionHandle, Attribute, Value, StringLength);
}

/*************************************************************************
 *				SQLColumnsW          [ODBC32.140]
 */
SQLRETURN WINAPI SQLColumnsW(SQLHSTMT StatementHandle,
             WCHAR *CatalogName, SQLSMALLINT NameLength1,
             WCHAR *SchemaName, SQLSMALLINT NameLength2,
             WCHAR *TableName, SQLSMALLINT NameLength3,
             WCHAR *ColumnName, SQLSMALLINT NameLength4)
{
        TRACE("\n");

        if (!pSQLColumnsW) return SQL_ERROR;
        return pSQLColumnsW(StatementHandle, CatalogName, NameLength1,
                            SchemaName, NameLength2, TableName, NameLength3, ColumnName, NameLength4);
}

/*************************************************************************
 *				SQLDriverConnectW          [ODBC32.141]
 */
SQLRETURN WINAPI SQLDriverConnectW(
    SQLHDBC            hdbc,
    SQLHWND            hwnd,
    WCHAR              *conn_str_in,
    SQLSMALLINT        len_conn_str_in,
    WCHAR              *conn_str_out,
    SQLSMALLINT        conn_str_out_max,
    SQLSMALLINT        *ptr_conn_str_out,
    SQLUSMALLINT       driver_completion )
{
        TRACE("(ConnectionString %s, Length %d)\n",
              debugstr_w(conn_str_in), len_conn_str_in);

        if (!pSQLDriverConnectW) return SQL_ERROR;
        return pSQLDriverConnectW(hdbc, hwnd, conn_str_in, len_conn_str_in, conn_str_out,
                                  conn_str_out_max, ptr_conn_str_out, driver_completion);
}

/*************************************************************************
 *				SQLGetConnectOptionW      [ODBC32.142]
 */
SQLRETURN WINAPI SQLGetConnectOptionW(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLPOINTER Value)
{
        TRACE("\n");

        if (!pSQLGetConnectOptionW) return SQL_ERROR;
        return pSQLGetConnectOptionW(ConnectionHandle, Option, Value);
}

/*************************************************************************
 *				SQLGetInfoW          [ODBC32.145]
 */
SQLRETURN WINAPI SQLGetInfoW(SQLHDBC ConnectionHandle,
             SQLUSMALLINT InfoType, SQLPOINTER InfoValue,
             SQLSMALLINT BufferLength, SQLSMALLINT *StringLength)
{
        SQLRETURN iResult;

        TRACE("InfoType = (%02u), InfoValue = %p, BufferLength = %d bytes\n", InfoType, InfoValue, BufferLength);
        if (InfoValue == NULL) {
                WARN("Unexpected NULL in InfoValue address\n");
                iResult = SQL_ERROR;
        } else {
                if (!pSQLGetInfoW) return SQL_ERROR;
                iResult = pSQLGetInfoW(ConnectionHandle, InfoType, InfoValue, BufferLength, StringLength);
                TRACE("returning %d...\n", iResult);
        }
        return iResult;
}

/*************************************************************************
 *				SQLGetTypeInfoW          [ODBC32.147]
 */
SQLRETURN WINAPI SQLGetTypeInfoW(SQLHSTMT StatementHandle, SQLSMALLINT DataType)
{
        TRACE("\n");

        if (!pSQLGetTypeInfoW) return SQL_ERROR;
        return pSQLGetTypeInfoW(StatementHandle, DataType);
}

/*************************************************************************
 *				SQLSetConnectOptionW          [ODBC32.150]
 */
SQLRETURN WINAPI SQLSetConnectOptionW(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLULEN Value)
{
        TRACE("\n");

        if (!pSQLSetConnectOptionW) return SQL_ERROR;
        return pSQLSetConnectOptionW(ConnectionHandle, Option, Value);
}

/*************************************************************************
 *				SQLSpecialColumnsW          [ODBC32.152]
 */
SQLRETURN WINAPI SQLSpecialColumnsW(SQLHSTMT StatementHandle,
             SQLUSMALLINT IdentifierType, SQLWCHAR *CatalogName,
             SQLSMALLINT NameLength1, SQLWCHAR *SchemaName,
             SQLSMALLINT NameLength2, SQLWCHAR *TableName,
             SQLSMALLINT NameLength3, SQLUSMALLINT Scope,
             SQLUSMALLINT Nullable)
{
        if (!pSQLSpecialColumnsW) return SQL_ERROR;
        return pSQLSpecialColumnsW(StatementHandle, IdentifierType, CatalogName, NameLength1, SchemaName,
                                   NameLength2, TableName, NameLength3, Scope, Nullable);
}

/*************************************************************************
 *				SQLStatisticsW          [ODBC32.153]
 */
SQLRETURN WINAPI SQLStatisticsW(SQLHSTMT StatementHandle,
             SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
             SQLWCHAR *SchemaName, SQLSMALLINT NameLength2,
             SQLWCHAR *TableName, SQLSMALLINT NameLength3,
             SQLUSMALLINT Unique, SQLUSMALLINT Reserved)
{
        TRACE("\n");

        if (!pSQLStatisticsW) return SQL_ERROR;
        return pSQLStatisticsW(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2,
                               TableName, NameLength3, Unique, Reserved);
}

/*************************************************************************
 *				SQLTablesW          [ODBC32.154]
 */
SQLRETURN WINAPI SQLTablesW(SQLHSTMT StatementHandle,
             SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
             SQLWCHAR *SchemaName, SQLSMALLINT NameLength2,
             SQLWCHAR *TableName, SQLSMALLINT NameLength3,
             SQLWCHAR *TableType, SQLSMALLINT NameLength4)
{
        TRACE("\n");

        if (!pSQLTablesW) return SQL_ERROR;
        return pSQLTablesW(StatementHandle, CatalogName, NameLength1,
                           SchemaName, NameLength2, TableName, NameLength3, TableType, NameLength4);
}

/*************************************************************************
 *				SQLBrowseConnectW          [ODBC32.155]
 */
SQLRETURN WINAPI SQLBrowseConnectW(
    SQLHDBC            hdbc,
    SQLWCHAR               *szConnStrIn,
    SQLSMALLINT        cbConnStrIn,
    SQLWCHAR               *szConnStrOut,
    SQLSMALLINT        cbConnStrOutMax,
    SQLSMALLINT       *pcbConnStrOut)
{
        TRACE("\n");

        if (!pSQLBrowseConnectW) return SQL_ERROR;
        return pSQLBrowseConnectW(hdbc, szConnStrIn, cbConnStrIn, szConnStrOut,
                                  cbConnStrOutMax, pcbConnStrOut);
}

/*************************************************************************
 *				SQLColumnPrivilegesW          [ODBC32.156]
 */
SQLRETURN WINAPI SQLColumnPrivilegesW(
    SQLHSTMT           hstmt,
    SQLWCHAR               *szCatalogName,
    SQLSMALLINT        cbCatalogName,
    SQLWCHAR               *szSchemaName,
    SQLSMALLINT        cbSchemaName,
    SQLWCHAR               *szTableName,
    SQLSMALLINT        cbTableName,
    SQLWCHAR               *szColumnName,
    SQLSMALLINT        cbColumnName)
{
        TRACE("\n");

        if (!pSQLColumnPrivilegesW) return SQL_ERROR;
        return pSQLColumnPrivilegesW(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName,
                                     szTableName, cbTableName, szColumnName, cbColumnName);
}

/*************************************************************************
 *				SQLDataSourcesW          [ODBC32.157]
 */
SQLRETURN WINAPI SQLDataSourcesW(SQLHENV EnvironmentHandle,
             SQLUSMALLINT Direction, WCHAR *ServerName,
             SQLSMALLINT BufferLength1, SQLSMALLINT *NameLength1,
             WCHAR *Description, SQLSMALLINT BufferLength2,
             SQLSMALLINT *NameLength2)
{
        SQLRETURN ret;

        TRACE("EnvironmentHandle = %p\n", EnvironmentHandle);

        if (!pSQLDataSourcesW) return SQL_ERROR;

        ret = pSQLDataSourcesW(EnvironmentHandle, Direction, ServerName,
                               BufferLength1, NameLength1, Description, BufferLength2, NameLength2);

        if (TRACE_ON(odbc))
        {
           TRACE("Returns %d \t", ret);
           if (*NameLength1 > 0)
             TRACE("DataSource = %s,", debugstr_w(ServerName));
           if (*NameLength2 > 0)
             TRACE(" Description = %s", debugstr_w(Description));
           TRACE("\n");
        }

        return ret;
}

/*************************************************************************
 *				SQLForeignKeysW          [ODBC32.160]
 */
SQLRETURN WINAPI SQLForeignKeysW(
    SQLHSTMT           hstmt,
    SQLWCHAR               *szPkCatalogName,
    SQLSMALLINT        cbPkCatalogName,
    SQLWCHAR               *szPkSchemaName,
    SQLSMALLINT        cbPkSchemaName,
    SQLWCHAR               *szPkTableName,
    SQLSMALLINT        cbPkTableName,
    SQLWCHAR               *szFkCatalogName,
    SQLSMALLINT        cbFkCatalogName,
    SQLWCHAR               *szFkSchemaName,
    SQLSMALLINT        cbFkSchemaName,
    SQLWCHAR               *szFkTableName,
    SQLSMALLINT        cbFkTableName)
{
        TRACE("\n");

        if (!pSQLForeignKeysW) return SQL_ERROR;
        return pSQLForeignKeysW(hstmt, szPkCatalogName, cbPkCatalogName, szPkSchemaName, cbPkSchemaName,
                                szPkTableName, cbPkTableName, szFkCatalogName, cbFkCatalogName,
                                szFkSchemaName, cbFkSchemaName, szFkTableName, cbFkTableName);
}

/*************************************************************************
 *				SQLNativeSqlW          [ODBC32.162]
 */
SQLRETURN WINAPI SQLNativeSqlW(
    SQLHDBC            hdbc,
    SQLWCHAR               *szSqlStrIn,
    SQLINTEGER         cbSqlStrIn,
    SQLWCHAR               *szSqlStr,
    SQLINTEGER         cbSqlStrMax,
    SQLINTEGER            *pcbSqlStr)
{
        TRACE("\n");

        if (!pSQLNativeSqlW) return SQL_ERROR;
        return pSQLNativeSqlW(hdbc, szSqlStrIn, cbSqlStrIn, szSqlStr, cbSqlStrMax, pcbSqlStr);
}

/*************************************************************************
 *				SQLPrimaryKeysW          [ODBC32.165]
 */
SQLRETURN WINAPI SQLPrimaryKeysW(
    SQLHSTMT           hstmt,
    SQLWCHAR               *szCatalogName,
    SQLSMALLINT        cbCatalogName,
    SQLWCHAR               *szSchemaName,
    SQLSMALLINT        cbSchemaName,
    SQLWCHAR               *szTableName,
    SQLSMALLINT        cbTableName)
{
        TRACE("\n");

        if (!pSQLPrimaryKeysW) return SQL_ERROR;
        return pSQLPrimaryKeysW(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName,
                                szTableName, cbTableName);
}

/*************************************************************************
 *				SQLProcedureColumnsW          [ODBC32.166]
 */
SQLRETURN WINAPI SQLProcedureColumnsW(
    SQLHSTMT           hstmt,
    SQLWCHAR               *szCatalogName,
    SQLSMALLINT        cbCatalogName,
    SQLWCHAR               *szSchemaName,
    SQLSMALLINT        cbSchemaName,
    SQLWCHAR               *szProcName,
    SQLSMALLINT        cbProcName,
    SQLWCHAR               *szColumnName,
    SQLSMALLINT        cbColumnName)
{
        TRACE("\n");

        if (!pSQLProcedureColumnsW) return SQL_ERROR;
        return pSQLProcedureColumnsW(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName,
                                     szProcName, cbProcName, szColumnName, cbColumnName);
}

/*************************************************************************
 *				SQLProceduresW          [ODBC32.167]
 */
SQLRETURN WINAPI SQLProceduresW(
    SQLHSTMT           hstmt,
    SQLWCHAR               *szCatalogName,
    SQLSMALLINT        cbCatalogName,
    SQLWCHAR               *szSchemaName,
    SQLSMALLINT        cbSchemaName,
    SQLWCHAR               *szProcName,
    SQLSMALLINT        cbProcName)
{
        TRACE("\n");

        if (!pSQLProceduresW) return SQL_ERROR;
        return pSQLProceduresW(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName,
                               szProcName, cbProcName);
}

/*************************************************************************
 *				SQLTablePrivilegesW          [ODBC32.170]
 */
SQLRETURN WINAPI SQLTablePrivilegesW(
    SQLHSTMT           hstmt,
    SQLWCHAR               *szCatalogName,
    SQLSMALLINT        cbCatalogName,
    SQLWCHAR               *szSchemaName,
    SQLSMALLINT        cbSchemaName,
    SQLWCHAR               *szTableName,
    SQLSMALLINT        cbTableName)
{
        TRACE("\n");

        if (!pSQLTablePrivilegesW) return SQL_ERROR;
        return pSQLTablePrivilegesW(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName,
                                    szTableName, cbTableName);
}

/*************************************************************************
 *				SQLDriversW          [ODBC32.171]
 */
SQLRETURN WINAPI SQLDriversW(
    SQLHENV            henv,
    SQLUSMALLINT       fDirection,
    SQLWCHAR               *szDriverDesc,
    SQLSMALLINT        cbDriverDescMax,
    SQLSMALLINT           *pcbDriverDesc,
    SQLWCHAR               *szDriverAttributes,
    SQLSMALLINT        cbDriverAttrMax,
    SQLSMALLINT           *pcbDriverAttr)
{
        SQLRETURN ret;

        TRACE("(Direction %d)\n", fDirection);

        if (!pSQLDriversW) return SQL_ERROR;
        ret = pSQLDriversW(henv, fDirection, szDriverDesc, cbDriverDescMax, pcbDriverDesc,
                           szDriverAttributes, cbDriverAttrMax, pcbDriverAttr);

        if (ret == SQL_NO_DATA && fDirection == SQL_FETCH_FIRST)
            ERR_(winediag)("No ODBC drivers could be found. "
                           "Check the settings for your libodbc provider.\n");

        return ret;
}

/*************************************************************************
 *				SQLSetDescFieldW          [ODBC32.173]
 */
SQLRETURN WINAPI SQLSetDescFieldW(SQLHDESC DescriptorHandle,
             SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
             SQLPOINTER Value, SQLINTEGER BufferLength)
{
        TRACE("\n");

        if (!pSQLSetDescFieldW) return SQL_ERROR;
        return pSQLSetDescFieldW(DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength);
}

/*************************************************************************
 *				SQLSetStmtAttrW          [ODBC32.176]
 */
SQLRETURN WINAPI SQLSetStmtAttrW(SQLHSTMT StatementHandle,
                 SQLINTEGER Attribute, SQLPOINTER Value,
                 SQLINTEGER StringLength)
{
        SQLRETURN iResult;
        TRACE("Attribute = (%02d) Value = %p StringLength = (%d)\n",
            Attribute, Value, StringLength);

        if (!pSQLSetStmtAttrW) return SQL_ERROR;
        iResult = pSQLSetStmtAttrW(StatementHandle, Attribute, Value, StringLength);
        if (iResult == SQL_ERROR && (Attribute == SQL_ROWSET_SIZE || Attribute == SQL_ATTR_ROW_ARRAY_SIZE)) {
            TRACE("CHEAT: returning SQL_SUCCESS to ADO...\n");
            iResult = SQL_SUCCESS;
        } else {
            TRACE("returning %d...\n", iResult);
        }
        return iResult;
}


/* End of file */
