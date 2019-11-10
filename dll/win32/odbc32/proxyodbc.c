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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#ifndef __REACTOS__
#include "wine/debug.h"
#endif
#include "wine/library.h"
#include "wine/unicode.h"

#include "sql.h"
#include "sqltypes.h"
#include "sqlext.h"

#ifdef __REACTOS__
#undef TRACE_ON
#include <wine/debug.h>
#endif

static BOOL ODBC_LoadDriverManager(void);
static BOOL ODBC_LoadDMFunctions(void);

WINE_DEFAULT_DEBUG_CHANNEL(odbc);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

static SQLRETURN (*pSQLAllocConnect)(SQLHENV,SQLHDBC*);
static SQLRETURN (*pSQLAllocEnv)(SQLHENV*);
static SQLRETURN (*pSQLAllocHandle)(SQLSMALLINT,SQLHANDLE,SQLHANDLE*);
static SQLRETURN (*pSQLAllocHandleStd)(SQLSMALLINT,SQLHANDLE,SQLHANDLE*);
static SQLRETURN (*pSQLAllocStmt)(SQLHDBC,SQLHSTMT*);
static SQLRETURN (*pSQLBindCol)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT,SQLPOINTER,SQLLEN,SQLLEN*);
static SQLRETURN (*pSQLBindParam)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT,SQLSMALLINT,SQLULEN,SQLSMALLINT,SQLPOINTER,SQLLEN*);
static SQLRETURN (*pSQLBindParameter)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT,SQLSMALLINT,SQLSMALLINT,SQLULEN,SQLSMALLINT,SQLPOINTER,SQLLEN,SQLLEN*);
static SQLRETURN (*pSQLBrowseConnect)(SQLHDBC,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLBrowseConnectW)(SQLHDBC,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLBulkOperations)(SQLHSTMT,SQLSMALLINT);
static SQLRETURN (*pSQLCancel)(SQLHSTMT);
static SQLRETURN (*pSQLCloseCursor)(SQLHSTMT);
static SQLRETURN (*pSQLColAttribute)(SQLHSTMT,SQLUSMALLINT,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*,SQLLEN*);
static SQLRETURN (*pSQLColAttributeW)(SQLHSTMT,SQLUSMALLINT,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*,SQLLEN*);
static SQLRETURN (*pSQLColAttributes)(SQLHSTMT,SQLUSMALLINT,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*,SQLLEN*);
static SQLRETURN (*pSQLColAttributesW)(SQLHSTMT,SQLUSMALLINT,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*,SQLLEN*);
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
static SQLRETURN (*pSQLDescribeCol)(SQLHSTMT,SQLUSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLSMALLINT*,SQLULEN*,SQLSMALLINT*,SQLSMALLINT*);
static SQLRETURN (*pSQLDescribeColW)(SQLHSTMT,SQLUSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLSMALLINT*,SQLULEN*,SQLSMALLINT*,SQLSMALLINT*);
static SQLRETURN (*pSQLDescribeParam)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT*,SQLULEN*,SQLSMALLINT*,SQLSMALLINT*);
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
static SQLRETURN (*pSQLExtendedFetch)(SQLHSTMT,SQLUSMALLINT,SQLLEN,SQLULEN*,SQLUSMALLINT*);
static SQLRETURN (*pSQLFetch)(SQLHSTMT);
static SQLRETURN (*pSQLFetchScroll)(SQLHSTMT,SQLSMALLINT,SQLLEN);
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
static SQLRETURN (*pSQLGetData)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT,SQLPOINTER,SQLLEN,SQLLEN*);
static SQLRETURN (*pSQLGetDescField)(SQLHDESC,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLGetDescFieldW)(SQLHDESC,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLGetDescRec)(SQLHDESC,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLSMALLINT*,SQLSMALLINT*,SQLLEN*,SQLSMALLINT*,SQLSMALLINT*,SQLSMALLINT*);
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
static SQLRETURN (*pSQLParamOptions)(SQLHSTMT,SQLULEN,SQLULEN*);
static SQLRETURN (*pSQLPrepare)(SQLHSTMT,SQLCHAR*,SQLINTEGER);
static SQLRETURN (*pSQLPrepareW)(SQLHSTMT,SQLWCHAR*,SQLINTEGER);
static SQLRETURN (*pSQLPrimaryKeys)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLPrimaryKeysW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLProcedureColumns)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLProcedureColumnsW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLProcedures)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLProceduresW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLPutData)(SQLHSTMT,SQLPOINTER,SQLLEN);
static SQLRETURN (*pSQLRowCount)(SQLHSTMT,SQLLEN*);
static SQLRETURN (*pSQLSetConnectAttr)(SQLHDBC,SQLINTEGER,SQLPOINTER,SQLINTEGER);
static SQLRETURN (*pSQLSetConnectAttrW)(SQLHDBC,SQLINTEGER,SQLPOINTER,SQLINTEGER);
static SQLRETURN (*pSQLSetConnectOption)(SQLHDBC,SQLUSMALLINT,SQLULEN);
static SQLRETURN (*pSQLSetConnectOptionW)(SQLHDBC,SQLUSMALLINT,SQLULEN);
static SQLRETURN (*pSQLSetCursorName)(SQLHSTMT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLSetCursorNameW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLSetDescField)(SQLHDESC,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLINTEGER);
static SQLRETURN (*pSQLSetDescFieldW)(SQLHDESC,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLINTEGER);
static SQLRETURN (*pSQLSetDescRec)(SQLHDESC,SQLSMALLINT,SQLSMALLINT,SQLSMALLINT,SQLLEN,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLLEN*,SQLLEN*);
static SQLRETURN (*pSQLSetEnvAttr)(SQLHENV,SQLINTEGER,SQLPOINTER,SQLINTEGER);
static SQLRETURN (*pSQLSetParam)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT,SQLSMALLINT,SQLULEN,SQLSMALLINT,SQLPOINTER,SQLLEN*);
static SQLRETURN (*pSQLSetPos)(SQLHSTMT,SQLSETPOSIROW,SQLUSMALLINT,SQLUSMALLINT);
static SQLRETURN (*pSQLSetScrollOptions)(SQLHSTMT,SQLUSMALLINT,SQLLEN,SQLUSMALLINT);
static SQLRETURN (*pSQLSetStmtAttr)(SQLHSTMT,SQLINTEGER,SQLPOINTER,SQLINTEGER);
static SQLRETURN (*pSQLSetStmtAttrW)(SQLHSTMT,SQLINTEGER,SQLPOINTER,SQLINTEGER);
static SQLRETURN (*pSQLSetStmtOption)(SQLHSTMT,SQLUSMALLINT,SQLULEN);
static SQLRETURN (*pSQLSpecialColumns)(SQLHSTMT,SQLUSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLUSMALLINT,SQLUSMALLINT);
static SQLRETURN (*pSQLSpecialColumnsW)(SQLHSTMT,SQLUSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLUSMALLINT,SQLUSMALLINT);
static SQLRETURN (*pSQLStatistics)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLUSMALLINT,SQLUSMALLINT);
static SQLRETURN (*pSQLStatisticsW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLUSMALLINT,SQLUSMALLINT);
static SQLRETURN (*pSQLTablePrivileges)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLTablePrivilegesW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLTables)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLTablesW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLTransact)(SQLHENV,SQLHDBC,SQLUSMALLINT);
static SQLRETURN (*pSQLGetDiagRecA)(SQLSMALLINT,SQLHANDLE,SQLSMALLINT,SQLCHAR*,SQLINTEGER*,
                                    SQLCHAR*,SQLSMALLINT,SQLSMALLINT*);

#define ERROR_FREE 0
#define ERROR_SQLERROR  1
#define ERROR_LIBRARY_NOT_FOUND 2

static void *dmHandle;
static int nErrorType;

SQLRETURN WINAPI ODBC32_SQLAllocEnv(SQLHENV *);
SQLRETURN WINAPI ODBC32_SQLFreeEnv(SQLHENV);
SQLRETURN WINAPI ODBC32_SQLDataSources(SQLHENV, SQLUSMALLINT, SQLCHAR *, SQLSMALLINT,
                                       SQLSMALLINT *, SQLCHAR *, SQLSMALLINT, SQLSMALLINT *);
SQLRETURN WINAPI ODBC32_SQLDrivers(SQLHENV, SQLUSMALLINT, SQLCHAR *, SQLSMALLINT, SQLSMALLINT *,
                                   SQLCHAR *, SQLSMALLINT, SQLSMALLINT *);

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
            while ((sql_ret = ODBC32_SQLDrivers (hEnv, dirn, (SQLCHAR*)desc, sizeof(desc),
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
        while ((sql_ret = ODBC32_SQLDataSources (hEnv, dirn,
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

    if ((sql_ret = ODBC32_SQLAllocEnv (&hEnv)) == SQL_SUCCESS)
    {
        ODBC_ReplicateODBCInstToRegistry (hEnv);
        ODBC_ReplicateODBCToRegistry (FALSE /* system dsns */, hEnv);
        ODBC_ReplicateODBCToRegistry (TRUE /* user dsns */, hEnv);

        if ((sql_ret = ODBC32_SQLFreeEnv (hEnv)) != SQL_SUCCESS)
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
    LOAD_FUNC(SQLGetDiagRecA);
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
SQLRETURN WINAPI ODBC32_SQLAllocConnect(SQLHENV EnvironmentHandle, SQLHDBC *ConnectionHandle)
{
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, ConnectionHandle %p)\n", EnvironmentHandle, ConnectionHandle);

    if (!pSQLAllocConnect)
    {
        *ConnectionHandle = SQL_NULL_HDBC;
        TRACE("Not ready\n");
        return SQL_ERROR;
    }

    ret = pSQLAllocConnect(EnvironmentHandle, ConnectionHandle);
    TRACE("Returning %d, ConnectionHandle %p\n", ret, *ConnectionHandle);
    return ret;
}

/*************************************************************************
 *				SQLAllocEnv           [ODBC32.002]
 */
SQLRETURN WINAPI ODBC32_SQLAllocEnv(SQLHENV *EnvironmentHandle)
{
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p)\n", EnvironmentHandle);

    if (!pSQLAllocEnv)
    {
        *EnvironmentHandle = SQL_NULL_HENV;
        TRACE("Not ready\n");
        return SQL_ERROR;
    }

    ret = pSQLAllocEnv(EnvironmentHandle);
    TRACE("Returning %d, EnvironmentHandle %p\n", ret, *EnvironmentHandle);
    return ret;
}

/*************************************************************************
 *				SQLAllocHandle           [ODBC32.024]
 */
SQLRETURN WINAPI ODBC32_SQLAllocHandle(SQLSMALLINT HandleType, SQLHANDLE InputHandle, SQLHANDLE *OutputHandle)
{
    SQLRETURN ret;

    TRACE("(HandleType %d, InputHandle %p, OutputHandle %p)\n", HandleType, InputHandle, OutputHandle);

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
    TRACE("Returning %d, Handle %p\n", ret, *OutputHandle);
    return ret;
}

/*************************************************************************
 *				SQLAllocStmt           [ODBC32.003]
 */
SQLRETURN WINAPI ODBC32_SQLAllocStmt(SQLHDBC ConnectionHandle, SQLHSTMT *StatementHandle)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, StatementHandle %p)\n", ConnectionHandle, StatementHandle);

    if (!pSQLAllocStmt)
    {
        *StatementHandle = SQL_NULL_HSTMT;
        TRACE("Not ready\n");
        return SQL_ERROR;
    }

    ret = pSQLAllocStmt(ConnectionHandle, StatementHandle);
    TRACE ("Returning %d, StatementHandle %p\n", ret, *StatementHandle);
    return ret;
}

/*************************************************************************
 *				SQLAllocHandleStd           [ODBC32.077]
 */
SQLRETURN WINAPI ODBC32_SQLAllocHandleStd(SQLSMALLINT HandleType, SQLHANDLE InputHandle, SQLHANDLE *OutputHandle)
{
    SQLRETURN ret;

    TRACE("(HandleType %d, InputHandle %p, OutputHandle %p)\n", HandleType, InputHandle, OutputHandle);

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

    ret = pSQLAllocHandleStd(HandleType, InputHandle, OutputHandle);
    TRACE ("Returning %d, OutputHandle %p\n", ret, *OutputHandle);
    return ret;
}

static const char *debugstr_sqllen( SQLLEN len )
{
#ifdef _WIN64
    return wine_dbg_sprintf( "%ld", len );
#else
    return wine_dbg_sprintf( "%d", len );
#endif
}

/*************************************************************************
 *				SQLBindCol           [ODBC32.004]
 */
SQLRETURN WINAPI ODBC32_SQLBindCol(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
                                   SQLPOINTER TargetValue, SQLLEN BufferLength, SQLLEN *StrLen_or_Ind)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ColumnNumber %d, TargetType %d, TargetValue %p, BufferLength %s, StrLen_or_Ind %p)\n",
          StatementHandle, ColumnNumber, TargetType, TargetValue, debugstr_sqllen(BufferLength), StrLen_or_Ind);

    if (!pSQLBindCol)
    {
        TRACE("Not ready\n");
        return SQL_ERROR;
    }

    ret = pSQLBindCol(StatementHandle, ColumnNumber, TargetType, TargetValue, BufferLength, StrLen_or_Ind);
    TRACE ("Returning %d\n", ret);
    return ret;
}

static const char *debugstr_sqlulen( SQLULEN len )
{
#ifdef _WIN64
    return wine_dbg_sprintf( "%lu", len );
#else
    return wine_dbg_sprintf( "%u", len );
#endif
}

/*************************************************************************
 *				SQLBindParam           [ODBC32.025]
 */
SQLRETURN WINAPI ODBC32_SQLBindParam(SQLHSTMT StatementHandle, SQLUSMALLINT ParameterNumber, SQLSMALLINT ValueType,
                                     SQLSMALLINT ParameterType, SQLULEN LengthPrecision, SQLSMALLINT ParameterScale,
                                     SQLPOINTER ParameterValue, SQLLEN *StrLen_or_Ind)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ParameterNumber %d, ValueType %d, ParameterType %d, LengthPrecision %s,"
          " ParameterScale %d, ParameterValue %p, StrLen_or_Ind %p)\n", StatementHandle, ParameterNumber, ValueType,
          ParameterType, debugstr_sqlulen(LengthPrecision), ParameterScale, ParameterValue, StrLen_or_Ind);

    if (!pSQLBindParam) return SQL_ERROR;

    ret = pSQLBindParam(StatementHandle, ParameterNumber, ValueType, ParameterType, LengthPrecision, ParameterScale,
                        ParameterValue, StrLen_or_Ind);
    TRACE ("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLCancel           [ODBC32.005]
 */
SQLRETURN WINAPI ODBC32_SQLCancel(SQLHSTMT StatementHandle)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p)\n", StatementHandle);

    if (!pSQLCancel) return SQL_ERROR;

    ret = pSQLCancel(StatementHandle);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLCloseCursor           [ODBC32.026]
 */
SQLRETURN WINAPI ODBC32_SQLCloseCursor(SQLHSTMT StatementHandle)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p)\n", StatementHandle);

    if (!pSQLCloseCursor) return SQL_ERROR;

    ret = pSQLCloseCursor(StatementHandle);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLColAttribute           [ODBC32.027]
 */
SQLRETURN WINAPI ODBC32_SQLColAttribute(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber,
                                        SQLUSMALLINT FieldIdentifier, SQLPOINTER CharacterAttribute,
                                        SQLSMALLINT BufferLength, SQLSMALLINT *StringLength,
                                        SQLLEN *NumericAttribute)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ColumnNumber %d, FieldIdentifier %d, CharacterAttribute %p, BufferLength %d,"
          " StringLength %p, NumericAttribute %p)\n", StatementHandle, ColumnNumber, FieldIdentifier,
          CharacterAttribute, BufferLength, StringLength, NumericAttribute);

    if (!pSQLColAttribute) return SQL_ERROR;

    ret = pSQLColAttribute(StatementHandle, ColumnNumber, FieldIdentifier, CharacterAttribute, BufferLength,
                           StringLength, NumericAttribute);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLColumns           [ODBC32.040]
 */
SQLRETURN WINAPI ODBC32_SQLColumns(SQLHSTMT StatementHandle, SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
                                   SQLCHAR *SchemaName, SQLSMALLINT NameLength2, SQLCHAR *TableName,
                                   SQLSMALLINT NameLength3, SQLCHAR *ColumnName, SQLSMALLINT NameLength4)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d, ColumnName %s, NameLength4 %d)\n", StatementHandle,
          debugstr_an((const char *)CatalogName, NameLength1), NameLength1,
          debugstr_an((const char *)SchemaName, NameLength2), NameLength2,
          debugstr_an((const char *)TableName, NameLength3), NameLength3,
          debugstr_an((const char *)ColumnName, NameLength4), NameLength4);

    if (!pSQLColumns) return SQL_ERROR;

    ret = pSQLColumns(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, TableName,
                      NameLength3, ColumnName, NameLength4);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLConnect           [ODBC32.007]
 */
SQLRETURN WINAPI ODBC32_SQLConnect(SQLHDBC ConnectionHandle, SQLCHAR *ServerName, SQLSMALLINT NameLength1,
                                   SQLCHAR *UserName, SQLSMALLINT NameLength2, SQLCHAR *Authentication,
                                   SQLSMALLINT NameLength3)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, ServerName %s, NameLength1 %d, UserName %s, NameLength2 %d, Authentication %s,"
          " NameLength3 %d)\n", ConnectionHandle,
          debugstr_an((const char *)ServerName, NameLength1), NameLength1,
          debugstr_an((const char *)UserName, NameLength2), NameLength2,
          debugstr_an((const char *)Authentication, NameLength3), NameLength3);

    if (!pSQLConnect) return SQL_ERROR;

    ret = pSQLConnect(ConnectionHandle, ServerName, NameLength1, UserName, NameLength2, Authentication, NameLength3);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLCopyDesc           [ODBC32.028]
 */
SQLRETURN WINAPI ODBC32_SQLCopyDesc(SQLHDESC SourceDescHandle, SQLHDESC TargetDescHandle)
{
    SQLRETURN ret;

    TRACE("(SourceDescHandle %p, TargetDescHandle %p)\n", SourceDescHandle, TargetDescHandle);

    if (!pSQLCopyDesc) return SQL_ERROR;

    ret = pSQLCopyDesc(SourceDescHandle, TargetDescHandle);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDataSources           [ODBC32.057]
 */
SQLRETURN WINAPI ODBC32_SQLDataSources(SQLHENV EnvironmentHandle, SQLUSMALLINT Direction, SQLCHAR *ServerName,
                                       SQLSMALLINT BufferLength1, SQLSMALLINT *NameLength1, SQLCHAR *Description,
                                       SQLSMALLINT BufferLength2, SQLSMALLINT *NameLength2)
{
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, Direction %d, ServerName %p, BufferLength1 %d, NameLength1 %p, Description %p,"
          " BufferLength2 %d, NameLength2 %p)\n", EnvironmentHandle, Direction, ServerName, BufferLength1,
          NameLength1, Description, BufferLength2, NameLength2);

    if (!pSQLDataSources) return SQL_ERROR;

    ret = pSQLDataSources(EnvironmentHandle, Direction, ServerName, BufferLength1, NameLength1, Description,
                          BufferLength2, NameLength2);
    if (ret >= 0 && TRACE_ON(odbc))
    {
        if (ServerName && NameLength1 && *NameLength1 > 0)
            TRACE(" DataSource %s", debugstr_an((const char *)ServerName, *NameLength1));
        if (Description && NameLength2 && *NameLength2 > 0)
            TRACE(" Description %s", debugstr_an((const char *)Description, *NameLength2));
        TRACE("\n");
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

SQLRETURN WINAPI ODBC32_SQLDataSourcesA(SQLHENV EnvironmentHandle, SQLUSMALLINT Direction, SQLCHAR *ServerName,
                                        SQLSMALLINT BufferLength1, SQLSMALLINT *NameLength1, SQLCHAR *Description,
                                        SQLSMALLINT BufferLength2, SQLSMALLINT *NameLength2)
{
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, Direction %d, ServerName %p, BufferLength1 %d, NameLength1 %p, Description %p,"
          " BufferLength2 %d, NameLength2 %p)\n", EnvironmentHandle, Direction, ServerName, BufferLength1,
          NameLength1, Description, BufferLength2, NameLength2);

    if (!pSQLDataSourcesA) return SQL_ERROR;

    ret = pSQLDataSourcesA(EnvironmentHandle, Direction, ServerName, BufferLength1, NameLength1, Description,
                           BufferLength2, NameLength2);
    if (TRACE_ON(odbc))
    {
       if (ServerName && NameLength1 && *NameLength1 > 0)
            TRACE(" DataSource %s", debugstr_an((const char *)ServerName, *NameLength1));
       if (Description && NameLength2 && *NameLength2 > 0)
            TRACE(" Description %s", debugstr_an((const char *)Description, *NameLength2));
       TRACE("\n");
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDescribeCol           [ODBC32.008]
 */
SQLRETURN WINAPI ODBC32_SQLDescribeCol(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLCHAR *ColumnName,
                                       SQLSMALLINT BufferLength, SQLSMALLINT *NameLength, SQLSMALLINT *DataType,
                                       SQLULEN *ColumnSize, SQLSMALLINT *DecimalDigits, SQLSMALLINT *Nullable)
{
    SQLSMALLINT dummy;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ColumnNumber %d, ColumnName %p, BufferLength %d, NameLength %p, DataType %p,"
          " ColumnSize %p, DecimalDigits %p, Nullable %p)\n", StatementHandle, ColumnNumber, ColumnName,
          BufferLength, NameLength, DataType, ColumnSize, DecimalDigits, Nullable);

    if (!pSQLDescribeCol) return SQL_ERROR;
    if (!NameLength) NameLength = &dummy; /* workaround for drivers that don't accept NULL NameLength */

    ret = pSQLDescribeCol(StatementHandle, ColumnNumber, ColumnName, BufferLength, NameLength, DataType, ColumnSize,
                          DecimalDigits, Nullable);
    if (ret >= 0)
    {
        if (ColumnName && NameLength) TRACE(" ColumnName %s\n", debugstr_an((const char *)ColumnName, *NameLength));
        if (DataType) TRACE(" DataType %d\n", *DataType);
        if (ColumnSize) TRACE(" ColumnSize %s\n", debugstr_sqlulen(*ColumnSize));
        if (DecimalDigits) TRACE(" DecimalDigits %d\n", *DecimalDigits);
        if (Nullable) TRACE(" Nullable %d\n", *Nullable);
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDisconnect           [ODBC32.009]
 */
SQLRETURN WINAPI ODBC32_SQLDisconnect(SQLHDBC ConnectionHandle)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p)\n", ConnectionHandle);

    if (!pSQLDisconnect) return SQL_ERROR;

    ret = pSQLDisconnect(ConnectionHandle);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLEndTran           [ODBC32.029]
 */
SQLRETURN WINAPI ODBC32_SQLEndTran(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT CompletionType)
{
    SQLRETURN ret;

    TRACE("(HandleType %d, Handle %p, CompletionType %d)\n", HandleType, Handle, CompletionType);

    if (!pSQLEndTran) return SQL_ERROR;

    ret = pSQLEndTran(HandleType, Handle, CompletionType);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLError           [ODBC32.010]
 */
SQLRETURN WINAPI ODBC32_SQLError(SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle, SQLHSTMT StatementHandle,
                                 SQLCHAR *Sqlstate, SQLINTEGER *NativeError, SQLCHAR *MessageText,
                                 SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, ConnectionHandle %p, StatementHandle %p, Sqlstate %p, NativeError %p,"
          " MessageText %p, BufferLength %d, TextLength %p)\n", EnvironmentHandle, ConnectionHandle,
          StatementHandle, Sqlstate, NativeError, MessageText, BufferLength, TextLength);

    if (!pSQLError) return SQL_ERROR;

    ret = pSQLError(EnvironmentHandle, ConnectionHandle, StatementHandle, Sqlstate, NativeError, MessageText,
                    BufferLength, TextLength);

    if (ret == SQL_SUCCESS)
    {
        TRACE(" SQLState %s\n", debugstr_an((const char *)Sqlstate, 5));
        TRACE(" Error %d\n", *NativeError);
        TRACE(" MessageText %s\n", debugstr_an((const char *)MessageText, *TextLength));
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLExecDirect           [ODBC32.011]
 */
SQLRETURN WINAPI ODBC32_SQLExecDirect(SQLHSTMT StatementHandle, SQLCHAR *StatementText, SQLINTEGER TextLength)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, StatementText %s, TextLength %d)\n", StatementHandle,
          debugstr_an((const char *)StatementText, TextLength), TextLength);

    if (!pSQLExecDirect) return SQL_ERROR;

    ret = pSQLExecDirect(StatementHandle, StatementText, TextLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLExecute           [ODBC32.012]
 */
SQLRETURN WINAPI ODBC32_SQLExecute(SQLHSTMT StatementHandle)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p)\n", StatementHandle);

    if (!pSQLExecute) return SQL_ERROR;

    ret = pSQLExecute(StatementHandle);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLFetch           [ODBC32.013]
 */
SQLRETURN WINAPI ODBC32_SQLFetch(SQLHSTMT StatementHandle)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p)\n", StatementHandle);

    if (!pSQLFetch) return SQL_ERROR;

    ret = pSQLFetch(StatementHandle);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLFetchScroll          [ODBC32.030]
 */
SQLRETURN WINAPI ODBC32_SQLFetchScroll(SQLHSTMT StatementHandle, SQLSMALLINT FetchOrientation, SQLLEN FetchOffset)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, FetchOrientation %d, FetchOffset %s)\n", StatementHandle, FetchOrientation,
          debugstr_sqllen(FetchOffset));

    if (!pSQLFetchScroll) return SQL_ERROR;

    ret = pSQLFetchScroll(StatementHandle, FetchOrientation, FetchOffset);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLFreeConnect           [ODBC32.014]
 */
SQLRETURN WINAPI ODBC32_SQLFreeConnect(SQLHDBC ConnectionHandle)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p)\n", ConnectionHandle);

    if (!pSQLFreeConnect) return SQL_ERROR;

    ret = pSQLFreeConnect(ConnectionHandle);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLFreeEnv           [ODBC32.015]
 */
SQLRETURN WINAPI ODBC32_SQLFreeEnv(SQLHENV EnvironmentHandle)
{
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p)\n", EnvironmentHandle);

    if (!pSQLFreeEnv) return SQL_ERROR;

    ret = pSQLFreeEnv(EnvironmentHandle);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLFreeHandle           [ODBC32.031]
 */
SQLRETURN WINAPI ODBC32_SQLFreeHandle(SQLSMALLINT HandleType, SQLHANDLE Handle)
{
    SQLRETURN ret;

    TRACE("(HandleType %d, Handle %p)\n", HandleType, Handle);

    if (!pSQLFreeHandle) return SQL_ERROR;

    ret = pSQLFreeHandle(HandleType, Handle);
    TRACE ("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLFreeStmt           [ODBC32.016]
 */
SQLRETURN WINAPI ODBC32_SQLFreeStmt(SQLHSTMT StatementHandle, SQLUSMALLINT Option)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Option %d)\n", StatementHandle, Option);

    if (!pSQLFreeStmt) return SQL_ERROR;

    ret = pSQLFreeStmt(StatementHandle, Option);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetConnectAttr           [ODBC32.032]
 */
SQLRETURN WINAPI ODBC32_SQLGetConnectAttr(SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                          SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Attribute %d, Value %p, BufferLength %d, StringLength %p)\n", ConnectionHandle,
          Attribute, Value, BufferLength, StringLength);

    if (!pSQLGetConnectAttr) return SQL_ERROR;

    ret = pSQLGetConnectAttr(ConnectionHandle, Attribute, Value, BufferLength, StringLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetConnectOption       [ODBC32.042]
 */
SQLRETURN WINAPI ODBC32_SQLGetConnectOption(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLPOINTER Value)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Option %d, Value %p)\n", ConnectionHandle, Option, Value);

    if (!pSQLGetConnectOption) return SQL_ERROR;

    ret = pSQLGetConnectOption(ConnectionHandle, Option, Value);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetCursorName           [ODBC32.017]
 */
SQLRETURN WINAPI ODBC32_SQLGetCursorName(SQLHSTMT StatementHandle, SQLCHAR *CursorName, SQLSMALLINT BufferLength,
                                         SQLSMALLINT *NameLength)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CursorName %p, BufferLength %d, NameLength %p)\n", StatementHandle, CursorName,
          BufferLength, NameLength);

    if (!pSQLGetCursorName) return SQL_ERROR;

    ret = pSQLGetCursorName(StatementHandle, CursorName, BufferLength, NameLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetData           [ODBC32.043]
 */
SQLRETURN WINAPI ODBC32_SQLGetData(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
                                   SQLPOINTER TargetValue, SQLLEN BufferLength, SQLLEN *StrLen_or_Ind)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ColumnNumber %d, TargetType %d, TargetValue %p, BufferLength %s, StrLen_or_Ind %p)\n",
          StatementHandle, ColumnNumber, TargetType, TargetValue, debugstr_sqllen(BufferLength), StrLen_or_Ind);

    if (!pSQLGetData) return SQL_ERROR;

    ret = pSQLGetData(StatementHandle, ColumnNumber, TargetType, TargetValue, BufferLength, StrLen_or_Ind);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetDescField           [ODBC32.033]
 */
SQLRETURN WINAPI ODBC32_SQLGetDescField(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                        SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    SQLRETURN ret;

    TRACE("(DescriptorHandle %p, RecNumber %d, FieldIdentifier %d, Value %p, BufferLength %d, StringLength %p)\n",
          DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength, StringLength);

    if (!pSQLGetDescField) return SQL_ERROR;

    ret = pSQLGetDescField(DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength, StringLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetDescRec           [ODBC32.034]
 */
SQLRETURN WINAPI ODBC32_SQLGetDescRec(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLCHAR *Name,
                                      SQLSMALLINT BufferLength, SQLSMALLINT *StringLength, SQLSMALLINT *Type,
                                      SQLSMALLINT *SubType, SQLLEN *Length, SQLSMALLINT *Precision,
                                      SQLSMALLINT *Scale, SQLSMALLINT *Nullable)
{
    SQLRETURN ret;

    TRACE("(DescriptorHandle %p, RecNumber %d, Name %p, BufferLength %d, StringLength %p, Type %p, SubType %p,"
          " Length %p, Precision %p, Scale %p, Nullable %p)\n", DescriptorHandle, RecNumber, Name, BufferLength,
          StringLength, Type, SubType, Length, Precision, Scale, Nullable);

    if (!pSQLGetDescRec) return SQL_ERROR;

    ret = pSQLGetDescRec(DescriptorHandle, RecNumber, Name, BufferLength, StringLength, Type, SubType, Length,
                         Precision, Scale, Nullable);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetDiagField           [ODBC32.035]
 */
SQLRETURN WINAPI ODBC32_SQLGetDiagField(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                                        SQLSMALLINT DiagIdentifier, SQLPOINTER DiagInfo, SQLSMALLINT BufferLength,
                                        SQLSMALLINT *StringLength)
{
    SQLRETURN ret;

    TRACE("(HandleType %d, Handle %p, RecNumber %d, DiagIdentifier %d, DiagInfo %p, BufferLength %d,"
          " StringLength %p)\n", HandleType, Handle, RecNumber, DiagIdentifier, DiagInfo, BufferLength, StringLength);

    if (!pSQLGetDiagField) return SQL_ERROR;

    ret = pSQLGetDiagField(HandleType, Handle, RecNumber, DiagIdentifier, DiagInfo, BufferLength, StringLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetDiagRec           [ODBC32.036]
 */
SQLRETURN WINAPI ODBC32_SQLGetDiagRec(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                                      SQLCHAR *Sqlstate, SQLINTEGER *NativeError, SQLCHAR *MessageText,
                                      SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    SQLRETURN ret;

    TRACE("(HandleType %d, Handle %p, RecNumber %d, Sqlstate %p, NativeError %p, MessageText %p, BufferLength %d,"
          " TextLength %p)\n", HandleType, Handle, RecNumber, Sqlstate, NativeError, MessageText, BufferLength,
          TextLength);

    if (!pSQLGetDiagRec) return SQL_ERROR;

    ret = pSQLGetDiagRec(HandleType, Handle, RecNumber, Sqlstate, NativeError, MessageText, BufferLength, TextLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetEnvAttr           [ODBC32.037]
 */
SQLRETURN WINAPI ODBC32_SQLGetEnvAttr(SQLHENV EnvironmentHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                      SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, Attribute %d, Value %p, BufferLength %d, StringLength %p)\n",
          EnvironmentHandle, Attribute, Value, BufferLength, StringLength);

    if (!pSQLGetEnvAttr) return SQL_ERROR;

    ret = pSQLGetEnvAttr(EnvironmentHandle, Attribute, Value, BufferLength, StringLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetFunctions           [ODBC32.044]
 */
SQLRETURN WINAPI ODBC32_SQLGetFunctions(SQLHDBC ConnectionHandle, SQLUSMALLINT FunctionId, SQLUSMALLINT *Supported)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, FunctionId %d, Supported %p)\n", ConnectionHandle, FunctionId, Supported);

    if (!pSQLGetFunctions) return SQL_ERROR;

    ret = pSQLGetFunctions(ConnectionHandle, FunctionId, Supported);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetInfo           [ODBC32.045]
 */
SQLRETURN WINAPI ODBC32_SQLGetInfo(SQLHDBC ConnectionHandle, SQLUSMALLINT InfoType, SQLPOINTER InfoValue,
                                   SQLSMALLINT BufferLength, SQLSMALLINT *StringLength)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle, %p, InfoType %d, InfoValue %p, BufferLength %d, StringLength %p)\n", ConnectionHandle,
          InfoType, InfoValue, BufferLength, StringLength);

    if (!InfoValue)
    {
        WARN("Unexpected NULL InfoValue address\n");
        return SQL_ERROR;
    }

    if (!pSQLGetInfo) return SQL_ERROR;

    ret = pSQLGetInfo(ConnectionHandle, InfoType, InfoValue, BufferLength, StringLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetStmtAttr           [ODBC32.038]
 */
SQLRETURN WINAPI ODBC32_SQLGetStmtAttr(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                       SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Attribute %d, Value %p, BufferLength %d, StringLength %p)\n", StatementHandle,
          Attribute, Value, BufferLength, StringLength);

    if (!Value)
    {
        WARN("Unexpected NULL Value return address\n");
        return SQL_ERROR;
    }

    if (!pSQLGetStmtAttr) return SQL_ERROR;

    ret = pSQLGetStmtAttr(StatementHandle, Attribute, Value, BufferLength, StringLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetStmtOption           [ODBC32.046]
 */
SQLRETURN WINAPI ODBC32_SQLGetStmtOption(SQLHSTMT StatementHandle, SQLUSMALLINT Option, SQLPOINTER Value)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Option %d, Value %p)\n", StatementHandle, Option, Value);

    if (!pSQLGetStmtOption) return SQL_ERROR;

    ret = pSQLGetStmtOption(StatementHandle, Option, Value);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetTypeInfo           [ODBC32.047]
 */
SQLRETURN WINAPI ODBC32_SQLGetTypeInfo(SQLHSTMT StatementHandle, SQLSMALLINT DataType)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, DataType %d)\n", StatementHandle, DataType);

    if (!pSQLGetTypeInfo) return SQL_ERROR;

    ret = pSQLGetTypeInfo(StatementHandle, DataType);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLNumResultCols           [ODBC32.018]
 */
SQLRETURN WINAPI ODBC32_SQLNumResultCols(SQLHSTMT StatementHandle, SQLSMALLINT *ColumnCount)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ColumnCount %p)\n", StatementHandle, ColumnCount);

    if (!pSQLNumResultCols) return SQL_ERROR;

    ret = pSQLNumResultCols(StatementHandle, ColumnCount);
    TRACE("Returning %d ColumnCount %d\n", ret, *ColumnCount);
    return ret;
}

/*************************************************************************
 *				SQLParamData           [ODBC32.048]
 */
SQLRETURN WINAPI ODBC32_SQLParamData(SQLHSTMT StatementHandle, SQLPOINTER *Value)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Value %p)\n", StatementHandle, Value);

    if (!pSQLParamData) return SQL_ERROR;

    ret = pSQLParamData(StatementHandle, Value);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLPrepare           [ODBC32.019]
 */
SQLRETURN WINAPI ODBC32_SQLPrepare(SQLHSTMT StatementHandle, SQLCHAR *StatementText, SQLINTEGER TextLength)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, StatementText %s, TextLength %d)\n", StatementHandle,
          debugstr_an((const char *)StatementText, TextLength), TextLength);

    if (!pSQLPrepare) return SQL_ERROR;

    ret = pSQLPrepare(StatementHandle, StatementText, TextLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLPutData           [ODBC32.049]
 */
SQLRETURN WINAPI ODBC32_SQLPutData(SQLHSTMT StatementHandle, SQLPOINTER Data, SQLLEN StrLen_or_Ind)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Data %p, StrLen_or_Ind %s)\n", StatementHandle, Data, debugstr_sqllen(StrLen_or_Ind));

    if (!pSQLPutData) return SQL_ERROR;

    ret = pSQLPutData(StatementHandle, Data, StrLen_or_Ind);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLRowCount           [ODBC32.020]
 */
SQLRETURN WINAPI ODBC32_SQLRowCount(SQLHSTMT StatementHandle, SQLLEN *RowCount)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, RowCount %p)\n", StatementHandle, RowCount);

    if (!pSQLRowCount) return SQL_ERROR;

    ret = pSQLRowCount(StatementHandle, RowCount);
    if (ret == SQL_SUCCESS && RowCount) TRACE(" RowCount %s\n", debugstr_sqllen(*RowCount));
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetConnectAttr           [ODBC32.039]
 */
SQLRETURN WINAPI ODBC32_SQLSetConnectAttr(SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                          SQLINTEGER StringLength)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Attribute %d, Value %p, StringLength %d)\n", ConnectionHandle, Attribute, Value,
          StringLength);

    if (!pSQLSetConnectAttr) return SQL_ERROR;

    ret = pSQLSetConnectAttr(ConnectionHandle, Attribute, Value, StringLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetConnectOption           [ODBC32.050]
 */
SQLRETURN WINAPI ODBC32_SQLSetConnectOption(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLULEN Value)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Option %d, Value %s)\n", ConnectionHandle, Option, debugstr_sqlulen(Value));

    if (!pSQLSetConnectOption) return SQL_ERROR;

    ret = pSQLSetConnectOption(ConnectionHandle, Option, Value);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetCursorName           [ODBC32.021]
 */
SQLRETURN WINAPI ODBC32_SQLSetCursorName(SQLHSTMT StatementHandle, SQLCHAR *CursorName, SQLSMALLINT NameLength)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CursorName %s, NameLength %d)\n", StatementHandle,
          debugstr_an((const char *)CursorName, NameLength), NameLength);

    if (!pSQLSetCursorName) return SQL_ERROR;

    ret = pSQLSetCursorName(StatementHandle, CursorName, NameLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetDescField           [ODBC32.073]
 */
SQLRETURN WINAPI ODBC32_SQLSetDescField(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                        SQLPOINTER Value, SQLINTEGER BufferLength)
{
    SQLRETURN ret;

    TRACE("(DescriptorHandle %p, RecNumber %d, FieldIdentifier %d, Value %p, BufferLength %d)\n", DescriptorHandle,
          RecNumber, FieldIdentifier, Value, BufferLength);

    if (!pSQLSetDescField) return SQL_ERROR;

    ret = pSQLSetDescField(DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetDescRec           [ODBC32.074]
 */
SQLRETURN WINAPI ODBC32_SQLSetDescRec(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT Type,
                                      SQLSMALLINT SubType, SQLLEN Length, SQLSMALLINT Precision, SQLSMALLINT Scale,
                                      SQLPOINTER Data, SQLLEN *StringLength, SQLLEN *Indicator)
{
    SQLRETURN ret;

    TRACE("(DescriptorHandle %p, RecNumber %d, Type %d, SubType %d, Length %s, Precision %d, Scale %d, Data %p,"
          " StringLength %p, Indicator %p)\n", DescriptorHandle, RecNumber, Type, SubType, debugstr_sqllen(Length),
          Precision, Scale, Data, StringLength, Indicator);

    if (!pSQLSetDescRec) return SQL_ERROR;

    ret = pSQLSetDescRec(DescriptorHandle, RecNumber, Type, SubType, Length, Precision, Scale, Data,
                         StringLength, Indicator);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetEnvAttr           [ODBC32.075]
 */
SQLRETURN WINAPI ODBC32_SQLSetEnvAttr(SQLHENV EnvironmentHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                      SQLINTEGER StringLength)
{
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, Attribute %d, Value %p, StringLength %d)\n", EnvironmentHandle, Attribute, Value,
          StringLength);

    if (!pSQLSetEnvAttr) return SQL_ERROR;

    ret = pSQLSetEnvAttr(EnvironmentHandle, Attribute, Value, StringLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetParam           [ODBC32.022]
 */
SQLRETURN WINAPI ODBC32_SQLSetParam(SQLHSTMT StatementHandle, SQLUSMALLINT ParameterNumber, SQLSMALLINT ValueType,
                                    SQLSMALLINT ParameterType, SQLULEN LengthPrecision, SQLSMALLINT ParameterScale,
                                    SQLPOINTER ParameterValue, SQLLEN *StrLen_or_Ind)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ParameterNumber %d, ValueType %d, ParameterType %d, LengthPrecision %s,"
          " ParameterScale %d, ParameterValue %p, StrLen_or_Ind %p)\n", StatementHandle, ParameterNumber, ValueType,
          ParameterType, debugstr_sqlulen(LengthPrecision), ParameterScale, ParameterValue, StrLen_or_Ind);

    if (!pSQLSetParam) return SQL_ERROR;

    ret = pSQLSetParam(StatementHandle, ParameterNumber, ValueType, ParameterType, LengthPrecision,
                       ParameterScale, ParameterValue, StrLen_or_Ind);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetStmtAttr           [ODBC32.076]
 */
SQLRETURN WINAPI ODBC32_SQLSetStmtAttr(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                       SQLINTEGER StringLength)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Attribute %d, Value %p, StringLength %d)\n", StatementHandle, Attribute, Value,
          StringLength);

    if (!pSQLSetStmtAttr) return SQL_ERROR;

    ret = pSQLSetStmtAttr(StatementHandle, Attribute, Value, StringLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetStmtOption           [ODBC32.051]
 */
SQLRETURN WINAPI ODBC32_SQLSetStmtOption(SQLHSTMT StatementHandle, SQLUSMALLINT Option, SQLULEN Value)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Option %d, Value %s)\n", StatementHandle, Option, debugstr_sqlulen(Value));

    if (!pSQLSetStmtOption) return SQL_ERROR;

    ret = pSQLSetStmtOption(StatementHandle, Option, Value);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSpecialColumns           [ODBC32.052]
 */
SQLRETURN WINAPI ODBC32_SQLSpecialColumns(SQLHSTMT StatementHandle, SQLUSMALLINT IdentifierType, SQLCHAR *CatalogName,
                                          SQLSMALLINT NameLength1, SQLCHAR *SchemaName, SQLSMALLINT NameLength2,
                                          SQLCHAR *TableName, SQLSMALLINT NameLength3, SQLUSMALLINT Scope,
                                          SQLUSMALLINT Nullable)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, IdentifierType %d, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d,"
          " TableName %s, NameLength3 %d, Scope %d, Nullable %d)\n", StatementHandle, IdentifierType,
          debugstr_an((const char *)CatalogName, NameLength1), NameLength1,
          debugstr_an((const char *)SchemaName, NameLength2), NameLength2,
          debugstr_an((const char *)TableName, NameLength3), NameLength3, Scope, Nullable);

    if (!pSQLSpecialColumns) return SQL_ERROR;

    ret = pSQLSpecialColumns(StatementHandle, IdentifierType, CatalogName, NameLength1, SchemaName,
                             NameLength2, TableName, NameLength3, Scope, Nullable);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLStatistics           [ODBC32.053]
 */
SQLRETURN WINAPI ODBC32_SQLStatistics(SQLHSTMT StatementHandle, SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
                                      SQLCHAR *SchemaName, SQLSMALLINT NameLength2, SQLCHAR *TableName,
                                      SQLSMALLINT NameLength3, SQLUSMALLINT Unique, SQLUSMALLINT Reserved)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d SchemaName %s, NameLength2 %d, TableName %s"
          " NameLength3 %d, Unique %d, Reserved %d)\n", StatementHandle,
          debugstr_an((const char *)CatalogName, NameLength1), NameLength1,
          debugstr_an((const char *)SchemaName, NameLength2), NameLength2,
          debugstr_an((const char *)TableName, NameLength3), NameLength3, Unique, Reserved);

    if (!pSQLStatistics) return SQL_ERROR;

    ret = pSQLStatistics(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, TableName,
                         NameLength3, Unique, Reserved);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLTables           [ODBC32.054]
 */
SQLRETURN WINAPI ODBC32_SQLTables(SQLHSTMT StatementHandle, SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
                                  SQLCHAR *SchemaName, SQLSMALLINT NameLength2, SQLCHAR *TableName,
                                  SQLSMALLINT NameLength3, SQLCHAR *TableType, SQLSMALLINT NameLength4)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d, TableType %s, NameLength4 %d)\n", StatementHandle,
          debugstr_an((const char *)CatalogName, NameLength1), NameLength1,
          debugstr_an((const char *)SchemaName, NameLength2), NameLength2,
          debugstr_an((const char *)TableName, NameLength3), NameLength3,
          debugstr_an((const char *)TableType, NameLength4), NameLength4);

    if (!pSQLTables) return SQL_ERROR;

    ret = pSQLTables(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                     TableType, NameLength4);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLTransact           [ODBC32.023]
 */
SQLRETURN WINAPI ODBC32_SQLTransact(SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle, SQLUSMALLINT CompletionType)
{
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, ConnectionHandle %p, CompletionType %d)\n", EnvironmentHandle, ConnectionHandle,
          CompletionType);

    if (!pSQLTransact) return SQL_ERROR;

    ret = pSQLTransact(EnvironmentHandle, ConnectionHandle, CompletionType);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLBrowseConnect           [ODBC32.055]
 */
SQLRETURN WINAPI ODBC32_SQLBrowseConnect(SQLHDBC hdbc, SQLCHAR *szConnStrIn, SQLSMALLINT cbConnStrIn,
                                         SQLCHAR *szConnStrOut, SQLSMALLINT cbConnStrOutMax,
                                         SQLSMALLINT *pcbConnStrOut)
{
    SQLRETURN ret;

    TRACE("(hdbc %p, szConnStrIn %s, cbConnStrIn %d, szConnStrOut %p, cbConnStrOutMax %d, pcbConnStrOut %p)\n",
          hdbc, debugstr_an((const char *)szConnStrIn, cbConnStrIn), cbConnStrIn, szConnStrOut, cbConnStrOutMax,
          pcbConnStrOut);

    if (!pSQLBrowseConnect) return SQL_ERROR;

    ret = pSQLBrowseConnect(hdbc, szConnStrIn, cbConnStrIn, szConnStrOut, cbConnStrOutMax, pcbConnStrOut);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLBulkOperations           [ODBC32.078]
 */
SQLRETURN WINAPI ODBC32_SQLBulkOperations(SQLHSTMT StatementHandle, SQLSMALLINT Operation)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Operation %d)\n", StatementHandle, Operation);

    if (!pSQLBulkOperations) return SQL_ERROR;

    ret = pSQLBulkOperations(StatementHandle, Operation);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLColAttributes           [ODBC32.006]
 */
SQLRETURN WINAPI ODBC32_SQLColAttributes(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLUSMALLINT fDescType,
                                         SQLPOINTER rgbDesc, SQLSMALLINT cbDescMax, SQLSMALLINT *pcbDesc,
                                         SQLLEN *pfDesc)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, icol %d, fDescType %d, rgbDesc %p, cbDescMax %d, pcbDesc %p, pfDesc %p)\n", hstmt, icol,
          fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc);

    if (!pSQLColAttributes) return SQL_ERROR;

    ret = pSQLColAttributes(hstmt, icol, fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLColumnPrivileges           [ODBC32.056]
 */
SQLRETURN WINAPI ODBC32_SQLColumnPrivileges(SQLHSTMT hstmt, SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                            SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR *szTableName,
                                            SQLSMALLINT cbTableName, SQLCHAR *szColumnName, SQLSMALLINT cbColumnName)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, szCatalogName %s, cbCatalogName %d, szSchemaName %s, cbSchemaName %d, szTableName %s,"
          " cbTableName %d, szColumnName %s, cbColumnName %d)\n", hstmt,
          debugstr_an((const char *)szCatalogName, cbCatalogName), cbCatalogName,
          debugstr_an((const char *)szSchemaName, cbSchemaName), cbSchemaName,
          debugstr_an((const char *)szTableName, cbTableName), cbTableName,
          debugstr_an((const char *)szColumnName, cbColumnName), cbColumnName);

    if (!pSQLColumnPrivileges) return SQL_ERROR;

    ret = pSQLColumnPrivileges(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName,
                               szTableName, cbTableName, szColumnName, cbColumnName);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDescribeParam          [ODBC32.058]
 */
SQLRETURN WINAPI ODBC32_SQLDescribeParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, SQLSMALLINT *pfSqlType,
                                         SQLULEN *pcbParamDef, SQLSMALLINT *pibScale, SQLSMALLINT *pfNullable)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, ipar %d, pfSqlType %p, pcbParamDef %p, pibScale %p, pfNullable %p)\n", hstmt, ipar,
          pfSqlType, pcbParamDef, pibScale, pfNullable);

    if (!pSQLDescribeParam) return SQL_ERROR;

    ret = pSQLDescribeParam(hstmt, ipar, pfSqlType, pcbParamDef, pibScale, pfNullable);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLExtendedFetch           [ODBC32.059]
 */
SQLRETURN WINAPI ODBC32_SQLExtendedFetch(SQLHSTMT hstmt, SQLUSMALLINT fFetchType, SQLLEN irow, SQLULEN *pcrow,
                                         SQLUSMALLINT *rgfRowStatus)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, fFetchType %d, irow %s, pcrow %p, rgfRowStatus %p)\n", hstmt, fFetchType, debugstr_sqllen(irow),
          pcrow, rgfRowStatus);

    if (!pSQLExtendedFetch) return SQL_ERROR;

    ret = pSQLExtendedFetch(hstmt, fFetchType, irow, pcrow, rgfRowStatus);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLForeignKeys           [ODBC32.060]
 */
SQLRETURN WINAPI ODBC32_SQLForeignKeys(SQLHSTMT hstmt, SQLCHAR *szPkCatalogName, SQLSMALLINT cbPkCatalogName,
                                       SQLCHAR *szPkSchemaName, SQLSMALLINT cbPkSchemaName, SQLCHAR *szPkTableName,
                                       SQLSMALLINT cbPkTableName, SQLCHAR *szFkCatalogName,
                                       SQLSMALLINT cbFkCatalogName, SQLCHAR *szFkSchemaName,
                                       SQLSMALLINT cbFkSchemaName, SQLCHAR *szFkTableName, SQLSMALLINT cbFkTableName)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, szPkCatalogName %s, cbPkCatalogName %d, szPkSchemaName %s, cbPkSchemaName %d,"
          " szPkTableName %s, cbPkTableName %d, szFkCatalogName %s, cbFkCatalogName %d, szFkSchemaName %s,"
          " cbFkSchemaName %d, szFkTableName %s, cbFkTableName %d)\n", hstmt,
          debugstr_an((const char *)szPkCatalogName, cbPkCatalogName), cbPkCatalogName,
          debugstr_an((const char *)szPkSchemaName, cbPkSchemaName), cbPkSchemaName,
          debugstr_an((const char *)szPkTableName, cbPkTableName), cbPkTableName,
          debugstr_an((const char *)szFkCatalogName, cbFkCatalogName), cbFkCatalogName,
          debugstr_an((const char *)szFkSchemaName, cbFkSchemaName), cbFkSchemaName,
          debugstr_an((const char *)szFkTableName, cbFkTableName), cbFkTableName);

    if (!pSQLForeignKeys) return SQL_ERROR;

    ret = pSQLForeignKeys(hstmt, szPkCatalogName, cbPkCatalogName, szPkSchemaName, cbPkSchemaName, szPkTableName,
                          cbPkTableName, szFkCatalogName, cbFkCatalogName, szFkSchemaName, cbFkSchemaName,
                          szFkTableName, cbFkTableName);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLMoreResults           [ODBC32.061]
 */
SQLRETURN WINAPI ODBC32_SQLMoreResults(SQLHSTMT StatementHandle)
{
    SQLRETURN ret;

    TRACE("(%p)\n", StatementHandle);

    if (!pSQLMoreResults) return SQL_ERROR;

    ret = pSQLMoreResults(StatementHandle);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLNativeSql           [ODBC32.062]
 */
SQLRETURN WINAPI ODBC32_SQLNativeSql(SQLHDBC hdbc, SQLCHAR *szSqlStrIn, SQLINTEGER cbSqlStrIn, SQLCHAR *szSqlStr,
                                     SQLINTEGER cbSqlStrMax, SQLINTEGER *pcbSqlStr)
{
    SQLRETURN ret;

    TRACE("(hdbc %p, szSqlStrIn %s, cbSqlStrIn %d, szSqlStr %p, cbSqlStrMax %d, pcbSqlStr %p)\n", hdbc,
          debugstr_an((const char *)szSqlStrIn, cbSqlStrIn), cbSqlStrIn, szSqlStr, cbSqlStrMax, pcbSqlStr);

    if (!pSQLNativeSql) return SQL_ERROR;

    ret = pSQLNativeSql(hdbc, szSqlStrIn, cbSqlStrIn, szSqlStr, cbSqlStrMax, pcbSqlStr);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLNumParams           [ODBC32.063]
 */
SQLRETURN WINAPI ODBC32_SQLNumParams(SQLHSTMT hstmt, SQLSMALLINT *pcpar)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, pcpar %p)\n", hstmt, pcpar);

    if (!pSQLNumParams) return SQL_ERROR;

    ret = pSQLNumParams(hstmt, pcpar);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLParamOptions           [ODBC32.064]
 */
SQLRETURN WINAPI ODBC32_SQLParamOptions(SQLHSTMT hstmt, SQLULEN crow, SQLULEN *pirow)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, crow %s, pirow %p)\n", hstmt, debugstr_sqlulen(crow), pirow);

    if (!pSQLParamOptions) return SQL_ERROR;

    ret = pSQLParamOptions(hstmt, crow, pirow);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLPrimaryKeys           [ODBC32.065]
 */
SQLRETURN WINAPI ODBC32_SQLPrimaryKeys(SQLHSTMT hstmt, SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                       SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR *szTableName,
                                       SQLSMALLINT cbTableName)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, szCatalogName %s, cbCatalogName %d, szSchemaName %s, cbSchemaName %d, szTableName %s,"
          " cbTableName %d)\n", hstmt,
          debugstr_an((const char *)szCatalogName, cbCatalogName), cbCatalogName,
          debugstr_an((const char *)szSchemaName, cbSchemaName), cbSchemaName,
          debugstr_an((const char *)szTableName, cbTableName), cbTableName);

    if (!pSQLPrimaryKeys) return SQL_ERROR;

    ret = pSQLPrimaryKeys(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName, cbTableName);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLProcedureColumns           [ODBC32.066]
 */
SQLRETURN WINAPI ODBC32_SQLProcedureColumns(SQLHSTMT hstmt, SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                            SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR *szProcName,
                                            SQLSMALLINT cbProcName, SQLCHAR *szColumnName, SQLSMALLINT cbColumnName)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, szCatalogName %s, cbCatalogName %d, szSchemaName %s, cbSchemaName %d, szProcName %s,"
          " cbProcName %d, szColumnName %s, cbColumnName %d)\n", hstmt,
          debugstr_an((const char *)szCatalogName, cbCatalogName), cbCatalogName,
          debugstr_an((const char *)szSchemaName, cbSchemaName), cbSchemaName,
          debugstr_an((const char *)szProcName, cbProcName), cbProcName,
          debugstr_an((const char *)szColumnName, cbColumnName), cbColumnName);

    if (!pSQLProcedureColumns) return SQL_ERROR;

    ret = pSQLProcedureColumns(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szProcName,
                               cbProcName, szColumnName, cbColumnName);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLProcedures           [ODBC32.067]
 */
SQLRETURN WINAPI ODBC32_SQLProcedures(SQLHSTMT hstmt, SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                      SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR *szProcName,
                                      SQLSMALLINT cbProcName)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, szCatalogName %s, cbCatalogName %d, szSchemaName %s, cbSchemaName %d, szProcName %s,"
          " cbProcName %d)\n", hstmt,
          debugstr_an((const char *)szCatalogName, cbCatalogName), cbCatalogName,
          debugstr_an((const char *)szSchemaName, cbSchemaName), cbSchemaName,
          debugstr_an((const char *)szProcName, cbProcName), cbProcName);

    if (!pSQLProcedures) return SQL_ERROR;

    ret = pSQLProcedures(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szProcName, cbProcName);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetPos           [ODBC32.068]
 */
SQLRETURN WINAPI ODBC32_SQLSetPos(SQLHSTMT hstmt, SQLSETPOSIROW irow, SQLUSMALLINT fOption, SQLUSMALLINT fLock)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, irow %s, fOption %d, fLock %d)\n", hstmt, debugstr_sqlulen(irow), fOption, fLock);

    if (!pSQLSetPos) return SQL_ERROR;

    ret = pSQLSetPos(hstmt, irow, fOption, fLock);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLTablePrivileges           [ODBC32.070]
 */
SQLRETURN WINAPI ODBC32_SQLTablePrivileges(SQLHSTMT hstmt, SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                           SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR *szTableName,
                                           SQLSMALLINT cbTableName)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, szCatalogName %s, cbCatalogName %d, szSchemaName %s, cbSchemaName %d, szTableName %s,"
          " cbTableName %d)\n", hstmt,
          debugstr_an((const char *)szCatalogName, cbCatalogName), cbCatalogName,
          debugstr_an((const char *)szSchemaName, cbSchemaName), cbSchemaName,
          debugstr_an((const char *)szTableName, cbTableName), cbTableName);

    if (!pSQLTablePrivileges) return SQL_ERROR;

    ret = pSQLTablePrivileges(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName,
                              cbTableName);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDrivers           [ODBC32.071]
 */
SQLRETURN WINAPI ODBC32_SQLDrivers(SQLHENV EnvironmentHandle, SQLUSMALLINT fDirection, SQLCHAR *szDriverDesc,
                                   SQLSMALLINT cbDriverDescMax, SQLSMALLINT *pcbDriverDesc,
                                   SQLCHAR *szDriverAttributes, SQLSMALLINT cbDriverAttrMax,
                                   SQLSMALLINT *pcbDriverAttr)
{
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, Direction %d, szDriverDesc %p, cbDriverDescMax %d, pcbDriverDesc %p,"
          " DriverAttributes %p, cbDriverAttrMax %d, pcbDriverAttr %p)\n", EnvironmentHandle, fDirection,
          szDriverDesc, cbDriverDescMax, pcbDriverDesc, szDriverAttributes, cbDriverAttrMax, pcbDriverAttr);

    if (!pSQLDrivers) return SQL_ERROR;

    ret = pSQLDrivers(EnvironmentHandle, fDirection, szDriverDesc, cbDriverDescMax, pcbDriverDesc,
                      szDriverAttributes, cbDriverAttrMax, pcbDriverAttr);

    if (ret == SQL_NO_DATA && fDirection == SQL_FETCH_FIRST)
        ERR_(winediag)("No ODBC drivers could be found. Check the settings for your libodbc provider.\n");

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLBindParameter           [ODBC32.072]
 */
SQLRETURN WINAPI ODBC32_SQLBindParameter(SQLHSTMT hstmt, SQLUSMALLINT ipar, SQLSMALLINT fParamType,
                                         SQLSMALLINT fCType, SQLSMALLINT fSqlType, SQLULEN cbColDef,
                                         SQLSMALLINT ibScale, SQLPOINTER rgbValue, SQLLEN cbValueMax,
                                         SQLLEN *pcbValue)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, ipar %d, fParamType %d, fCType %d, fSqlType %d, cbColDef %s, ibScale %d, rgbValue %p,"
          " cbValueMax %s, pcbValue %p)\n", hstmt, ipar, fParamType, fCType, fSqlType, debugstr_sqlulen(cbColDef),
          ibScale, rgbValue, debugstr_sqllen(cbValueMax), pcbValue);

    if (!pSQLBindParameter) return SQL_ERROR;

    ret = pSQLBindParameter(hstmt, ipar, fParamType, fCType, fSqlType, cbColDef, ibScale, rgbValue, cbValueMax,
                            pcbValue);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDriverConnect           [ODBC32.041]
 */
SQLRETURN WINAPI ODBC32_SQLDriverConnect(SQLHDBC hdbc, SQLHWND hwnd, SQLCHAR *ConnectionString, SQLSMALLINT Length,
                                         SQLCHAR *conn_str_out, SQLSMALLINT conn_str_out_max,
                                         SQLSMALLINT *ptr_conn_str_out, SQLUSMALLINT driver_completion)
{
    SQLRETURN ret;

    TRACE("(hdbc %p, hwnd %p, ConnectionString %s, Length %d, conn_str_out %p, conn_str_out_max %d,"
          " ptr_conn_str_out %p, driver_completion %d)\n", hdbc, hwnd,
          debugstr_an((const char *)ConnectionString, Length), Length, conn_str_out, conn_str_out_max,
          ptr_conn_str_out, driver_completion);

    if (!pSQLDriverConnect) return SQL_ERROR;

    ret = pSQLDriverConnect(hdbc, hwnd, ConnectionString, Length, conn_str_out, conn_str_out_max,
                            ptr_conn_str_out, driver_completion);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetScrollOptions           [ODBC32.069]
 */
SQLRETURN WINAPI ODBC32_SQLSetScrollOptions(SQLHSTMT statement_handle, SQLUSMALLINT f_concurrency, SQLLEN crow_keyset,
                                            SQLUSMALLINT crow_rowset)
{
    SQLRETURN ret;

    TRACE("(statement_handle %p, f_concurrency %d, crow_keyset %s, crow_rowset %d)\n", statement_handle,
          f_concurrency, debugstr_sqllen(crow_keyset), crow_rowset);

    if (!pSQLSetScrollOptions) return SQL_ERROR;

    ret = pSQLSetScrollOptions(statement_handle, f_concurrency, crow_keyset, crow_rowset);
    TRACE("Returning %d\n", ret);
    return ret;
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

    for (i = 0; i < ARRAY_SIZE(attrList); i++) {
        if (attrList[i] == fDescType) return TRUE;
    }
    return FALSE;
}

/*************************************************************************
 *				SQLColAttributesW          [ODBC32.106]
 */
SQLRETURN WINAPI ODBC32_SQLColAttributesW(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLUSMALLINT fDescType,
                                          SQLPOINTER rgbDesc, SQLSMALLINT cbDescMax, SQLSMALLINT *pcbDesc,
                                          SQLLEN *pfDesc)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, icol %d, fDescType %d, rgbDesc %p, cbDescMax %d, pcbDesc %p, pfDesc %p)\n", hstmt, icol,
          fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc);

    if (!pSQLColAttributesW) return SQL_ERROR;

    ret = pSQLColAttributesW(hstmt, icol, fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc);

    if (ret == SQL_SUCCESS && SQLColAttributes_KnownStringAttribute(fDescType) && rgbDesc && pcbDesc &&
        *pcbDesc != lstrlenW(rgbDesc) * 2)
    {
        TRACE("CHEAT: resetting name length for ADO\n");
        *pcbDesc = lstrlenW(rgbDesc) * 2;
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLConnectW          [ODBC32.107]
 */
SQLRETURN WINAPI ODBC32_SQLConnectW(SQLHDBC ConnectionHandle, WCHAR *ServerName, SQLSMALLINT NameLength1,
                                    WCHAR *UserName, SQLSMALLINT NameLength2, WCHAR *Authentication,
                                    SQLSMALLINT NameLength3)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, ServerName %s, NameLength1 %d, UserName %s, NameLength2 %d, Authentication %s,"
          " NameLength3 %d)\n", ConnectionHandle, debugstr_wn(ServerName, NameLength1), NameLength1,
          debugstr_wn(UserName, NameLength2), NameLength2, debugstr_wn(Authentication, NameLength3), NameLength3);

    if (!pSQLConnectW) return SQL_ERROR;

    ret = pSQLConnectW(ConnectionHandle, ServerName, NameLength1, UserName, NameLength2, Authentication, NameLength3);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDescribeColW          [ODBC32.108]
 */
SQLRETURN WINAPI ODBC32_SQLDescribeColW(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, WCHAR *ColumnName,
                                        SQLSMALLINT BufferLength, SQLSMALLINT *NameLength, SQLSMALLINT *DataType,
                                        SQLULEN *ColumnSize, SQLSMALLINT *DecimalDigits, SQLSMALLINT *Nullable)
{
    SQLSMALLINT dummy;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ColumnNumber %d, ColumnName %p, BufferLength %d, NameLength %p, DataType %p,"
          " ColumnSize %p, DecimalDigits %p, Nullable %p)\n", StatementHandle, ColumnNumber, ColumnName,
          BufferLength, NameLength, DataType, ColumnSize, DecimalDigits, Nullable);

    if (!pSQLDescribeColW) return SQL_ERROR;
    if (!NameLength) NameLength = &dummy; /* workaround for drivers that don't accept NULL NameLength */

    ret = pSQLDescribeColW(StatementHandle, ColumnNumber, ColumnName, BufferLength, NameLength, DataType, ColumnSize,
                           DecimalDigits, Nullable);
    if (ret >= 0)
    {
        if (ColumnName && NameLength) TRACE("ColumnName %s\n", debugstr_wn(ColumnName, *NameLength));
        if (DataType) TRACE("DataType %d\n", *DataType);
        if (ColumnSize) TRACE("ColumnSize %s\n", debugstr_sqlulen(*ColumnSize));
        if (DecimalDigits) TRACE("DecimalDigits %d\n", *DecimalDigits);
        if (Nullable) TRACE("Nullable %d\n", *Nullable);
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLErrorW          [ODBC32.110]
 */
SQLRETURN WINAPI ODBC32_SQLErrorW(SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle, SQLHSTMT StatementHandle,
                                  WCHAR *Sqlstate, SQLINTEGER *NativeError, WCHAR *MessageText,
                                  SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, ConnectionHandle %p, StatementHandle %p, Sqlstate %p, NativeError %p,"
          " MessageText %p, BufferLength %d, TextLength %p)\n", EnvironmentHandle, ConnectionHandle,
          StatementHandle, Sqlstate, NativeError, MessageText, BufferLength, TextLength);

    if (!pSQLErrorW) return SQL_ERROR;

    ret = pSQLErrorW(EnvironmentHandle, ConnectionHandle, StatementHandle, Sqlstate, NativeError, MessageText,
                     BufferLength, TextLength);

    if (ret == SQL_SUCCESS)
    {
        TRACE(" SQLState %s\n", debugstr_wn(Sqlstate, 5));
        TRACE(" Error %d\n", *NativeError);
        TRACE(" MessageText %s\n", debugstr_wn(MessageText, *TextLength));
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLExecDirectW          [ODBC32.111]
 */
SQLRETURN WINAPI ODBC32_SQLExecDirectW(SQLHSTMT StatementHandle, WCHAR *StatementText, SQLINTEGER TextLength)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, StatementText %s, TextLength %d)\n", StatementHandle,
          debugstr_wn(StatementText, TextLength), TextLength);

    if (!pSQLExecDirectW) return SQL_ERROR;

    ret = pSQLExecDirectW(StatementHandle, StatementText, TextLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetCursorNameW          [ODBC32.117]
 */
SQLRETURN WINAPI ODBC32_SQLGetCursorNameW(SQLHSTMT StatementHandle, WCHAR *CursorName, SQLSMALLINT BufferLength,
                                          SQLSMALLINT *NameLength)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CursorName %p, BufferLength %d, NameLength %p)\n", StatementHandle, CursorName,
          BufferLength, NameLength);

    if (!pSQLGetCursorNameW) return SQL_ERROR;

    ret = pSQLGetCursorNameW(StatementHandle, CursorName, BufferLength, NameLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLPrepareW          [ODBC32.119]
 */
SQLRETURN WINAPI ODBC32_SQLPrepareW(SQLHSTMT StatementHandle, WCHAR *StatementText, SQLINTEGER TextLength)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, StatementText %s, TextLength %d)\n", StatementHandle,
          debugstr_wn(StatementText, TextLength), TextLength);

    if (!pSQLPrepareW) return SQL_ERROR;

    ret = pSQLPrepareW(StatementHandle, StatementText, TextLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetCursorNameW          [ODBC32.121]
 */
SQLRETURN WINAPI ODBC32_SQLSetCursorNameW(SQLHSTMT StatementHandle, WCHAR *CursorName, SQLSMALLINT NameLength)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CursorName %s, NameLength %d)\n", StatementHandle,
          debugstr_wn(CursorName, NameLength), NameLength);

    if (!pSQLSetCursorNameW) return SQL_ERROR;

    ret = pSQLSetCursorNameW(StatementHandle, CursorName, NameLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLColAttributeW          [ODBC32.127]
 */
SQLRETURN WINAPI ODBC32_SQLColAttributeW(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber,
                                         SQLUSMALLINT FieldIdentifier, SQLPOINTER CharacterAttribute,
                                         SQLSMALLINT BufferLength, SQLSMALLINT *StringLength,
                                         SQLLEN *NumericAttribute)
{
    SQLRETURN ret;

    TRACE("StatementHandle %p ColumnNumber %d FieldIdentifier %d CharacterAttribute %p BufferLength %d"
          " StringLength %p NumericAttribute %p\n", StatementHandle, ColumnNumber, FieldIdentifier,
          CharacterAttribute, BufferLength, StringLength, NumericAttribute);

    if (!pSQLColAttributeW) return SQL_ERROR;

    ret = pSQLColAttributeW(StatementHandle, ColumnNumber, FieldIdentifier, CharacterAttribute, BufferLength,
                            StringLength, NumericAttribute);

    if (ret == SQL_SUCCESS && CharacterAttribute != NULL && SQLColAttributes_KnownStringAttribute(FieldIdentifier) &&
        StringLength && *StringLength != lstrlenW(CharacterAttribute) * 2)
    {
        TRACE("CHEAT: resetting name length for ADO\n");
        *StringLength = lstrlenW(CharacterAttribute) * 2;
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetConnectAttrW          [ODBC32.132]
 */
SQLRETURN WINAPI ODBC32_SQLGetConnectAttrW(SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                           SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Attribute %d, Value %p, BufferLength %d, StringLength %p)\n", ConnectionHandle,
          Attribute, Value, BufferLength, StringLength);

    if (!pSQLGetConnectAttrW) return SQL_ERROR;

    ret = pSQLGetConnectAttrW(ConnectionHandle, Attribute, Value, BufferLength, StringLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetDescFieldW          [ODBC32.133]
 */
SQLRETURN WINAPI ODBC32_SQLGetDescFieldW(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                         SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    SQLRETURN ret;

    TRACE("(DescriptorHandle %p, RecNumber %d, FieldIdentifier %d, Value %p, BufferLength %d, StringLength %p)\n",
          DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength, StringLength);

    if (!pSQLGetDescFieldW) return SQL_ERROR;

    ret = pSQLGetDescFieldW(DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength, StringLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetDescRecW          [ODBC32.134]
 */
SQLRETURN WINAPI ODBC32_SQLGetDescRecW(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, WCHAR *Name,
                                       SQLSMALLINT BufferLength, SQLSMALLINT *StringLength, SQLSMALLINT *Type,
                                       SQLSMALLINT *SubType, SQLLEN *Length, SQLSMALLINT *Precision,
                                       SQLSMALLINT *Scale, SQLSMALLINT *Nullable)
{
    SQLRETURN ret;

    TRACE("(DescriptorHandle %p, RecNumber %d, Name %p, BufferLength %d, StringLength %p, Type %p, SubType %p,"
          " Length %p, Precision %p, Scale %p, Nullable %p)\n", DescriptorHandle, RecNumber, Name, BufferLength,
          StringLength, Type, SubType, Length, Precision, Scale, Nullable);

    if (!pSQLGetDescRecW) return SQL_ERROR;

    ret = pSQLGetDescRecW(DescriptorHandle, RecNumber, Name, BufferLength, StringLength, Type, SubType, Length,
                          Precision, Scale, Nullable);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetDiagFieldW          [ODBC32.135]
 */
SQLRETURN WINAPI ODBC32_SQLGetDiagFieldW(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                                         SQLSMALLINT DiagIdentifier, SQLPOINTER DiagInfo, SQLSMALLINT BufferLength,
                                         SQLSMALLINT *StringLength)
{
    SQLRETURN ret;

    TRACE("(HandleType %d, Handle %p, RecNumber %d, DiagIdentifier %d, DiagInfo %p, BufferLength %d,"
          " StringLength %p)\n", HandleType, Handle, RecNumber, DiagIdentifier, DiagInfo, BufferLength, StringLength);

    if (!pSQLGetDiagFieldW) return SQL_ERROR;

    ret = pSQLGetDiagFieldW(HandleType, Handle, RecNumber, DiagIdentifier, DiagInfo, BufferLength, StringLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetDiagRecW           [ODBC32.136]
 */
SQLRETURN WINAPI ODBC32_SQLGetDiagRecW(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                                       WCHAR *Sqlstate, SQLINTEGER *NativeError, WCHAR *MessageText,
                                       SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    SQLRETURN ret;

    TRACE("(HandleType %d, Handle %p, RecNumber %d, Sqlstate %p, NativeError %p, MessageText %p, BufferLength %d,"
          " TextLength %p)\n", HandleType, Handle, RecNumber, Sqlstate, NativeError, MessageText, BufferLength,
          TextLength);

    if (!pSQLGetDiagRecW) return SQL_ERROR;

    ret = pSQLGetDiagRecW(HandleType, Handle, RecNumber, Sqlstate, NativeError, MessageText, BufferLength, TextLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetStmtAttrW          [ODBC32.138]
 */
SQLRETURN WINAPI ODBC32_SQLGetStmtAttrW(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                        SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Attribute %d, Value %p, BufferLength %d, StringLength %p)\n", StatementHandle,
          Attribute, Value, BufferLength, StringLength);

    if (!Value)
    {
        WARN("Unexpected NULL Value return address\n");
        return SQL_ERROR;
    }

    if (!pSQLGetStmtAttrW) return SQL_ERROR;

    ret = pSQLGetStmtAttrW(StatementHandle, Attribute, Value, BufferLength, StringLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetConnectAttrW          [ODBC32.139]
 */
SQLRETURN WINAPI ODBC32_SQLSetConnectAttrW(SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                           SQLINTEGER StringLength)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Attribute %d, Value %p, StringLength %d)\n", ConnectionHandle, Attribute, Value,
          StringLength);

    if (!pSQLSetConnectAttrW) return SQL_ERROR;

    ret = pSQLSetConnectAttrW(ConnectionHandle, Attribute, Value, StringLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLColumnsW          [ODBC32.140]
 */
SQLRETURN WINAPI ODBC32_SQLColumnsW(SQLHSTMT StatementHandle, WCHAR *CatalogName, SQLSMALLINT NameLength1,
                                    WCHAR *SchemaName, SQLSMALLINT NameLength2, WCHAR *TableName,
                                    SQLSMALLINT NameLength3, WCHAR *ColumnName, SQLSMALLINT NameLength4)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d, ColumnName %s, NameLength4 %d)\n", StatementHandle,
          debugstr_wn(CatalogName, NameLength1), NameLength1, debugstr_wn(SchemaName, NameLength2), NameLength2,
          debugstr_wn(TableName, NameLength3), NameLength3, debugstr_wn(ColumnName, NameLength4), NameLength4);

    if (!pSQLColumnsW) return SQL_ERROR;

    ret = pSQLColumnsW(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                       ColumnName, NameLength4);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDriverConnectW          [ODBC32.141]
 */
SQLRETURN WINAPI ODBC32_SQLDriverConnectW(SQLHDBC ConnectionHandle, SQLHWND WindowHandle, WCHAR *InConnectionString,
                                          SQLSMALLINT Length, WCHAR *OutConnectionString, SQLSMALLINT BufferLength,
                                          SQLSMALLINT *Length2, SQLUSMALLINT DriverCompletion)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, WindowHandle %p, InConnectionString %s, Length %d, OutConnectionString %p,"
          " BufferLength %d, Length2 %p, DriverCompletion %d)\n", ConnectionHandle, WindowHandle,
          debugstr_wn(InConnectionString, Length), Length, OutConnectionString, BufferLength, Length2,
          DriverCompletion);

    if (!pSQLDriverConnectW) return SQL_ERROR;

    ret = pSQLDriverConnectW(ConnectionHandle, WindowHandle, InConnectionString, Length, OutConnectionString,
                             BufferLength, Length2, DriverCompletion);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetConnectOptionW      [ODBC32.142]
 */
SQLRETURN WINAPI ODBC32_SQLGetConnectOptionW(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLPOINTER Value)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Option %d, Value %p)\n", ConnectionHandle, Option, Value);

    if (!pSQLGetConnectOptionW) return SQL_ERROR;

    ret = pSQLGetConnectOptionW(ConnectionHandle, Option, Value);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetInfoW          [ODBC32.145]
 */
SQLRETURN WINAPI ODBC32_SQLGetInfoW(SQLHDBC ConnectionHandle, SQLUSMALLINT InfoType, SQLPOINTER InfoValue,
                                    SQLSMALLINT BufferLength, SQLSMALLINT *StringLength)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle, %p, InfoType %d, InfoValue %p, BufferLength %d, StringLength %p)\n", ConnectionHandle,
          InfoType, InfoValue, BufferLength, StringLength);

    if (!InfoValue)
    {
        WARN("Unexpected NULL InfoValue address\n");
        return SQL_ERROR;
    }

    if (!pSQLGetInfoW) return SQL_ERROR;

    ret = pSQLGetInfoW(ConnectionHandle, InfoType, InfoValue, BufferLength, StringLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetTypeInfoW          [ODBC32.147]
 */
SQLRETURN WINAPI ODBC32_SQLGetTypeInfoW(SQLHSTMT StatementHandle, SQLSMALLINT DataType)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, DataType %d)\n", StatementHandle, DataType);

    if (!pSQLGetTypeInfoW) return SQL_ERROR;

    ret = pSQLGetTypeInfoW(StatementHandle, DataType);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetConnectOptionW          [ODBC32.150]
 */
SQLRETURN WINAPI ODBC32_SQLSetConnectOptionW(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLULEN Value)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Option %d, Value %s)\n", ConnectionHandle, Option, debugstr_sqllen(Value));

    if (!pSQLSetConnectOptionW) return SQL_ERROR;

    ret = pSQLSetConnectOptionW(ConnectionHandle, Option, Value);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSpecialColumnsW          [ODBC32.152]
 */
SQLRETURN WINAPI ODBC32_SQLSpecialColumnsW(SQLHSTMT StatementHandle, SQLUSMALLINT IdentifierType,
                                           SQLWCHAR *CatalogName, SQLSMALLINT NameLength1, SQLWCHAR *SchemaName,
                                           SQLSMALLINT NameLength2, SQLWCHAR *TableName, SQLSMALLINT NameLength3,
                                           SQLUSMALLINT Scope, SQLUSMALLINT Nullable)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, IdentifierType %d, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d,"
          " TableName %s, NameLength3 %d, Scope %d, Nullable %d)\n", StatementHandle, IdentifierType,
          debugstr_wn(CatalogName, NameLength1), NameLength1, debugstr_wn(SchemaName, NameLength2), NameLength2,
          debugstr_wn(TableName, NameLength3), NameLength3, Scope, Nullable);

    if (!pSQLSpecialColumnsW) return SQL_ERROR;

    ret = pSQLSpecialColumnsW(StatementHandle, IdentifierType, CatalogName, NameLength1, SchemaName,
                              NameLength2, TableName, NameLength3, Scope, Nullable);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLStatisticsW          [ODBC32.153]
 */
SQLRETURN WINAPI ODBC32_SQLStatisticsW(SQLHSTMT StatementHandle, SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
                                       SQLWCHAR *SchemaName, SQLSMALLINT NameLength2, SQLWCHAR *TableName,
                                       SQLSMALLINT NameLength3, SQLUSMALLINT Unique, SQLUSMALLINT Reserved)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d SchemaName %s, NameLength2 %d, TableName %s"
          " NameLength3 %d, Unique %d, Reserved %d)\n", StatementHandle,
          debugstr_wn(CatalogName, NameLength1), NameLength1, debugstr_wn(SchemaName, NameLength2), NameLength2,
          debugstr_wn(TableName, NameLength3), NameLength3, Unique, Reserved);

    if (!pSQLStatisticsW) return SQL_ERROR;

    ret = pSQLStatisticsW(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, TableName,
                          NameLength3, Unique, Reserved);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLTablesW          [ODBC32.154]
 */
SQLRETURN WINAPI ODBC32_SQLTablesW(SQLHSTMT StatementHandle, SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
                                   SQLWCHAR *SchemaName, SQLSMALLINT NameLength2, SQLWCHAR *TableName,
                                   SQLSMALLINT NameLength3, SQLWCHAR *TableType, SQLSMALLINT NameLength4)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d, TableType %s, NameLength4 %d)\n", StatementHandle,
          debugstr_wn(CatalogName, NameLength1), NameLength1, debugstr_wn(SchemaName, NameLength2), NameLength2,
          debugstr_wn(TableName, NameLength3), NameLength3, debugstr_wn(TableType, NameLength4), NameLength4);

    if (!pSQLTablesW) return SQL_ERROR;

    ret = pSQLTablesW(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                      TableType, NameLength4);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLBrowseConnectW          [ODBC32.155]
 */
SQLRETURN WINAPI ODBC32_SQLBrowseConnectW(SQLHDBC hdbc, SQLWCHAR *szConnStrIn, SQLSMALLINT cbConnStrIn,
                                          SQLWCHAR *szConnStrOut, SQLSMALLINT cbConnStrOutMax,
                                          SQLSMALLINT *pcbConnStrOut)
{
    SQLRETURN ret;

    TRACE("(hdbc %p, szConnStrIn %s, cbConnStrIn %d, szConnStrOut %p, cbConnStrOutMax %d, pcbConnStrOut %p)\n",
          hdbc, debugstr_wn(szConnStrIn, cbConnStrIn), cbConnStrIn, szConnStrOut, cbConnStrOutMax, pcbConnStrOut);

    if (!pSQLBrowseConnectW) return SQL_ERROR;

    ret = pSQLBrowseConnectW(hdbc, szConnStrIn, cbConnStrIn, szConnStrOut, cbConnStrOutMax, pcbConnStrOut);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLColumnPrivilegesW          [ODBC32.156]
 */
SQLRETURN WINAPI ODBC32_SQLColumnPrivilegesW(SQLHSTMT hstmt, SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                             SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR *szTableName,
                                             SQLSMALLINT cbTableName, SQLWCHAR *szColumnName, SQLSMALLINT cbColumnName)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, szCatalogName %s, cbCatalogName %d, szSchemaName %s, cbSchemaName %d, szTableName %s,"
          " cbTableName %d, szColumnName %s, cbColumnName %d)\n", hstmt,
          debugstr_wn(szCatalogName, cbCatalogName), cbCatalogName,
          debugstr_wn(szSchemaName, cbSchemaName), cbSchemaName,
          debugstr_wn(szTableName, cbTableName), cbTableName,
          debugstr_wn(szColumnName, cbColumnName), cbColumnName);

    if (!pSQLColumnPrivilegesW) return SQL_ERROR;

    ret = pSQLColumnPrivilegesW(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName,
                                cbTableName, szColumnName, cbColumnName);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDataSourcesW          [ODBC32.157]
 */
SQLRETURN WINAPI ODBC32_SQLDataSourcesW(SQLHENV EnvironmentHandle, SQLUSMALLINT Direction, WCHAR *ServerName,
                                        SQLSMALLINT BufferLength1, SQLSMALLINT *NameLength1, WCHAR *Description,
                                        SQLSMALLINT BufferLength2, SQLSMALLINT *NameLength2)
{
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, Direction %d, ServerName %p, BufferLength1 %d, NameLength1 %p, Description %p,"
          " BufferLength2 %d, NameLength2 %p)\n", EnvironmentHandle, Direction, ServerName, BufferLength1,
          NameLength1, Description, BufferLength2, NameLength2);

    if (!pSQLDataSourcesW) return SQL_ERROR;

    ret = pSQLDataSourcesW(EnvironmentHandle, Direction, ServerName, BufferLength1, NameLength1, Description,
                           BufferLength2, NameLength2);

    if (ret >= 0 && TRACE_ON(odbc))
    {
        if (ServerName && NameLength1 && *NameLength1 > 0)
            TRACE(" DataSource %s", debugstr_wn(ServerName, *NameLength1));
        if (Description && NameLength2 && *NameLength2 > 0)
            TRACE(" Description %s", debugstr_wn(Description, *NameLength2));
        TRACE("\n");
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLForeignKeysW          [ODBC32.160]
 */
SQLRETURN WINAPI ODBC32_SQLForeignKeysW(SQLHSTMT hstmt, SQLWCHAR *szPkCatalogName, SQLSMALLINT cbPkCatalogName,
                                        SQLWCHAR *szPkSchemaName, SQLSMALLINT cbPkSchemaName, SQLWCHAR *szPkTableName,
                                        SQLSMALLINT cbPkTableName, SQLWCHAR *szFkCatalogName,
                                        SQLSMALLINT cbFkCatalogName, SQLWCHAR *szFkSchemaName,
                                        SQLSMALLINT cbFkSchemaName, SQLWCHAR *szFkTableName, SQLSMALLINT cbFkTableName)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, szPkCatalogName %s, cbPkCatalogName %d, szPkSchemaName %s, cbPkSchemaName %d,"
          " szPkTableName %s, cbPkTableName %d, szFkCatalogName %s, cbFkCatalogName %d, szFkSchemaName %s,"
          " cbFkSchemaName %d, szFkTableName %s, cbFkTableName %d)\n", hstmt,
          debugstr_wn(szPkCatalogName, cbPkCatalogName), cbPkCatalogName,
          debugstr_wn(szPkSchemaName, cbPkSchemaName), cbPkSchemaName,
          debugstr_wn(szPkTableName, cbPkTableName), cbPkTableName,
          debugstr_wn(szFkCatalogName, cbFkCatalogName), cbFkCatalogName,
          debugstr_wn(szFkSchemaName, cbFkSchemaName), cbFkSchemaName,
          debugstr_wn(szFkTableName, cbFkTableName), cbFkTableName);

    if (!pSQLForeignKeysW) return SQL_ERROR;

    ret = pSQLForeignKeysW(hstmt, szPkCatalogName, cbPkCatalogName, szPkSchemaName, cbPkSchemaName, szPkTableName,
                           cbPkTableName, szFkCatalogName, cbFkCatalogName, szFkSchemaName, cbFkSchemaName,
                           szFkTableName, cbFkTableName);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLNativeSqlW          [ODBC32.162]
 */
SQLRETURN WINAPI ODBC32_SQLNativeSqlW(SQLHDBC hdbc, SQLWCHAR *szSqlStrIn, SQLINTEGER cbSqlStrIn, SQLWCHAR *szSqlStr,
                                      SQLINTEGER cbSqlStrMax, SQLINTEGER *pcbSqlStr)
{
    SQLRETURN ret;

    TRACE("(hdbc %p, szSqlStrIn %s, cbSqlStrIn %d, szSqlStr %p, cbSqlStrMax %d, pcbSqlStr %p)\n", hdbc,
          debugstr_wn(szSqlStrIn, cbSqlStrIn), cbSqlStrIn, szSqlStr, cbSqlStrMax, pcbSqlStr);

    if (!pSQLNativeSqlW) return SQL_ERROR;

    ret = pSQLNativeSqlW(hdbc, szSqlStrIn, cbSqlStrIn, szSqlStr, cbSqlStrMax, pcbSqlStr);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLPrimaryKeysW          [ODBC32.165]
 */
SQLRETURN WINAPI ODBC32_SQLPrimaryKeysW(SQLHSTMT hstmt, SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                        SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR *szTableName,
                                        SQLSMALLINT cbTableName)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, szCatalogName %s, cbCatalogName %d, szSchemaName %s, cbSchemaName %d, szTableName %s,"
          " cbTableName %d)\n", hstmt,
          debugstr_wn(szCatalogName, cbCatalogName), cbCatalogName,
          debugstr_wn(szSchemaName, cbSchemaName), cbSchemaName,
          debugstr_wn(szTableName, cbTableName), cbTableName);

    if (!pSQLPrimaryKeysW) return SQL_ERROR;

    ret = pSQLPrimaryKeysW(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName, cbTableName);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLProcedureColumnsW          [ODBC32.166]
 */
SQLRETURN WINAPI ODBC32_SQLProcedureColumnsW(SQLHSTMT hstmt, SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                             SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR *szProcName,
                                             SQLSMALLINT cbProcName, SQLWCHAR *szColumnName, SQLSMALLINT cbColumnName)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, szCatalogName %s, cbCatalogName %d, szSchemaName %s, cbSchemaName %d, szProcName %s,"
          " cbProcName %d, szColumnName %s, cbColumnName %d)\n", hstmt,
          debugstr_wn(szCatalogName, cbCatalogName), cbCatalogName,
          debugstr_wn(szSchemaName, cbSchemaName), cbSchemaName,
          debugstr_wn(szProcName, cbProcName), cbProcName,
          debugstr_wn(szColumnName, cbColumnName), cbColumnName);

    if (!pSQLProcedureColumnsW) return SQL_ERROR;

    ret = pSQLProcedureColumnsW(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szProcName,
                                cbProcName, szColumnName, cbColumnName);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLProceduresW          [ODBC32.167]
 */
SQLRETURN WINAPI ODBC32_SQLProceduresW(SQLHSTMT hstmt, SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                       SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR *szProcName,
                                       SQLSMALLINT cbProcName)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, szCatalogName %s, cbCatalogName %d, szSchemaName %s, cbSchemaName %d, szProcName %s,"
          " cbProcName %d)\n", hstmt, debugstr_wn(szCatalogName, cbCatalogName), cbCatalogName,
          debugstr_wn(szSchemaName, cbSchemaName), cbSchemaName, debugstr_wn(szProcName, cbProcName), cbProcName);

    if (!pSQLProceduresW) return SQL_ERROR;

    ret = pSQLProceduresW(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szProcName, cbProcName);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLTablePrivilegesW          [ODBC32.170]
 */
SQLRETURN WINAPI ODBC32_SQLTablePrivilegesW(SQLHSTMT hstmt, SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                            SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR *szTableName,
                                            SQLSMALLINT cbTableName)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, szCatalogName %s, cbCatalogName %d, szSchemaName %s, cbSchemaName %d, szTableName %s,"
          " cbTableName %d)\n", hstmt, debugstr_wn(szCatalogName, cbCatalogName), cbCatalogName,
          debugstr_wn(szSchemaName, cbSchemaName), cbSchemaName, debugstr_wn(szTableName, cbTableName), cbTableName);

    if (!pSQLTablePrivilegesW) return SQL_ERROR;

    ret = pSQLTablePrivilegesW(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName,
                               cbTableName);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDriversW          [ODBC32.171]
 */
SQLRETURN WINAPI ODBC32_SQLDriversW(SQLHENV EnvironmentHandle, SQLUSMALLINT fDirection, SQLWCHAR *szDriverDesc,
                                    SQLSMALLINT cbDriverDescMax, SQLSMALLINT *pcbDriverDesc,
                                    SQLWCHAR *szDriverAttributes, SQLSMALLINT cbDriverAttrMax,
                                    SQLSMALLINT *pcbDriverAttr)
{
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, Direction %d, szDriverDesc %p, cbDriverDescMax %d, pcbDriverDesc %p,"
          " DriverAttributes %p, cbDriverAttrMax %d, pcbDriverAttr %p)\n", EnvironmentHandle, fDirection,
          szDriverDesc, cbDriverDescMax, pcbDriverDesc, szDriverAttributes, cbDriverAttrMax, pcbDriverAttr);

    if (!pSQLDriversW) return SQL_ERROR;

    ret = pSQLDriversW(EnvironmentHandle, fDirection, szDriverDesc, cbDriverDescMax, pcbDriverDesc,
                       szDriverAttributes, cbDriverAttrMax, pcbDriverAttr);

    if (ret == SQL_NO_DATA && fDirection == SQL_FETCH_FIRST)
        ERR_(winediag)("No ODBC drivers could be found. Check the settings for your libodbc provider.\n");

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetDescFieldW          [ODBC32.173]
 */
SQLRETURN WINAPI ODBC32_SQLSetDescFieldW(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                         SQLPOINTER Value, SQLINTEGER BufferLength)
{
    SQLRETURN ret;

    TRACE("(DescriptorHandle %p, RecNumber %d, FieldIdentifier %d, Value %p, BufferLength %d)\n", DescriptorHandle,
          RecNumber, FieldIdentifier, Value, BufferLength);

    if (!pSQLSetDescFieldW) return SQL_ERROR;

    ret = pSQLSetDescFieldW(DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetStmtAttrW          [ODBC32.176]
 */
SQLRETURN WINAPI ODBC32_SQLSetStmtAttrW(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                        SQLINTEGER StringLength)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Attribute %d, Value %p, StringLength %d)\n", StatementHandle, Attribute, Value,
          StringLength);

    if (!pSQLSetStmtAttrW) return SQL_ERROR;

    ret = pSQLSetStmtAttrW(StatementHandle, Attribute, Value, StringLength);
    if (ret == SQL_ERROR && (Attribute == SQL_ROWSET_SIZE || Attribute == SQL_ATTR_ROW_ARRAY_SIZE))
    {
        TRACE("CHEAT: returning SQL_SUCCESS to ADO\n");
        return SQL_SUCCESS;
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetDiagRecA           [ODBC32.236]
 */
SQLRETURN WINAPI ODBC32_SQLGetDiagRecA(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                                       SQLCHAR *Sqlstate, SQLINTEGER *NativeError, SQLCHAR *MessageText,
                                       SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    SQLRETURN ret;

    TRACE("(HandleType %d, Handle %p, RecNumber %d, Sqlstate %p, NativeError %p, MessageText %p, BufferLength %d,"
          " TextLength %p)\n", HandleType, Handle, RecNumber, Sqlstate, NativeError, MessageText, BufferLength,
          TextLength);

    if (!pSQLGetDiagRecA) return SQL_ERROR;

    ret = pSQLGetDiagRecA(HandleType, Handle, RecNumber, Sqlstate, NativeError, MessageText, BufferLength, TextLength);
    TRACE("Returning %d\n", ret);
    return ret;
}
