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
#include "wine/debug.h"
#include "wine/library.h"
#include "wine/unicode.h"

#include "sql.h"
#include "sqltypes.h"
#include "sqlext.h"

#include "proxyodbc.h"

static BOOL ODBC_LoadDriverManager(void);
static BOOL ODBC_LoadDMFunctions(void);

WINE_DEFAULT_DEBUG_CHANNEL(odbc);

static const DM_FUNC template_func[] =
{
    /* 00 */ { SQL_API_SQLALLOCCONNECT,      "SQLAllocConnect", SQLAllocConnect, NULL, NULL },
    /* 01 */ { SQL_API_SQLALLOCENV,          "SQLAllocEnv", SQLAllocEnv, NULL, NULL },
    /* 02 */ { SQL_API_SQLALLOCHANDLE,       "SQLAllocHandle", SQLAllocHandle, NULL, NULL },
    /* 03 */ { SQL_API_SQLALLOCSTMT,         "SQLAllocStmt", SQLAllocStmt, NULL, NULL },
    /* 04 */ { SQL_API_SQLALLOCHANDLESTD,    "SQLAllocHandleStd", SQLAllocHandleStd, NULL, NULL },
    /* 05 */ { SQL_API_SQLBINDCOL,           "SQLBindCol", SQLBindCol, NULL, NULL },
    /* 06 */ { SQL_API_SQLBINDPARAM,         "SQLBindParam", SQLBindParam, NULL, NULL },
    /* 07 */ { SQL_API_SQLBINDPARAMETER,     "SQLBindParameter", SQLBindParameter, NULL, NULL },
    /* 08 */ { SQL_API_SQLBROWSECONNECT,     "SQLBrowseConnect", SQLBrowseConnect, NULL, NULL },
    /* 09 */ { SQL_API_SQLBULKOPERATIONS,    "SQLBulkOperations", SQLBulkOperations, NULL, NULL },
    /* 10 */ { SQL_API_SQLCANCEL,            "SQLCancel", SQLCancel, NULL, NULL },
    /* 11 */ { SQL_API_SQLCLOSECURSOR,       "SQLCloseCursor", SQLCloseCursor, NULL, NULL },
    /* 12 */ { SQL_API_SQLCOLATTRIBUTE,      "SQLColAttribute", SQLColAttribute, NULL, NULL },
    /* 13 */ { SQL_API_SQLCOLATTRIBUTES,     "SQLColAttributes", SQLColAttributes, NULL, NULL },
    /* 14 */ { SQL_API_SQLCOLUMNPRIVILEGES,  "SQLColumnPrivileges", SQLColumnPrivileges, NULL, NULL },
    /* 15 */ { SQL_API_SQLCOLUMNS,           "SQLColumns", SQLColumns, NULL, NULL },
    /* 16 */ { SQL_API_SQLCONNECT,           "SQLConnect", SQLConnect, NULL, NULL },
    /* 17 */ { SQL_API_SQLCOPYDESC,          "SQLCopyDesc", SQLCopyDesc, NULL, NULL },
    /* 18 */ { SQL_API_SQLDATASOURCES,       "SQLDataSources", SQLDataSources, NULL, NULL },
    /* 19 */ { SQL_API_SQLDESCRIBECOL,       "SQLDescribeCol", SQLDescribeCol, NULL, NULL },
    /* 20 */ { SQL_API_SQLDESCRIBEPARAM,     "SQLDescribeParam", SQLDescribeParam, NULL, NULL },
    /* 21 */ { SQL_API_SQLDISCONNECT,        "SQLDisconnect", SQLDisconnect, NULL, NULL },
    /* 22 */ { SQL_API_SQLDRIVERCONNECT,     "SQLDriverConnect", SQLDriverConnect, NULL, NULL },
    /* 23 */ { SQL_API_SQLDRIVERS,           "SQLDrivers", SQLDrivers, NULL, NULL },
    /* 24 */ { SQL_API_SQLENDTRAN,           "SQLEndTran", SQLEndTran, NULL, NULL },
    /* 25 */ { SQL_API_SQLERROR,             "SQLError", SQLError, NULL, NULL },
    /* 26 */ { SQL_API_SQLEXECDIRECT,        "SQLExecDirect", SQLExecDirect, NULL, NULL },
    /* 27 */ { SQL_API_SQLEXECUTE,           "SQLExecute", SQLExecute, NULL, NULL },
    /* 28 */ { SQL_API_SQLEXTENDEDFETCH,     "SQLExtendedFetch", SQLExtendedFetch, NULL, NULL },
    /* 29 */ { SQL_API_SQLFETCH,             "SQLFetch", SQLFetch, NULL, NULL },
    /* 30 */ { SQL_API_SQLFETCHSCROLL,       "SQLFetchScroll", SQLFetchScroll, NULL, NULL },
    /* 31 */ { SQL_API_SQLFOREIGNKEYS,       "SQLForeignKeys", SQLForeignKeys, NULL, NULL },
    /* 32 */ { SQL_API_SQLFREEENV,           "SQLFreeEnv", SQLFreeEnv, NULL, NULL },
    /* 33 */ { SQL_API_SQLFREEHANDLE,        "SQLFreeHandle", SQLFreeHandle, NULL, NULL },
    /* 34 */ { SQL_API_SQLFREESTMT,          "SQLFreeStmt", SQLFreeStmt, NULL, NULL },
    /* 35 */ { SQL_API_SQLFREECONNECT,       "SQLFreeConnect", SQLFreeConnect, NULL, NULL },
    /* 36 */ { SQL_API_SQLGETCONNECTATTR,    "SQLGetConnectAttr", SQLGetConnectAttr, NULL, NULL },
    /* 37 */ { SQL_API_SQLGETCONNECTOPTION,  "SQLGetConnectOption", SQLGetConnectOption, NULL, NULL },
    /* 38 */ { SQL_API_SQLGETCURSORNAME,     "SQLGetCursorName", SQLGetCursorName, NULL, NULL },
    /* 39 */ { SQL_API_SQLGETDATA,           "SQLGetData", SQLGetData, NULL, NULL },
    /* 40 */ { SQL_API_SQLGETDESCFIELD,      "SQLGetDescField", SQLGetDescField, NULL, NULL },
    /* 41 */ { SQL_API_SQLGETDESCREC,        "SQLGetDescRec", SQLGetDescRec, NULL, NULL },
    /* 42 */ { SQL_API_SQLGETDIAGFIELD,      "SQLGetDiagField", SQLGetDiagField, NULL, NULL },
    /* 43 */ { SQL_API_SQLGETENVATTR,        "SQLGetEnvAttr", SQLGetEnvAttr, NULL, NULL },
    /* 44 */ { SQL_API_SQLGETFUNCTIONS,      "SQLGetFunctions", SQLGetFunctions, NULL, NULL },
    /* 45 */ { SQL_API_SQLGETINFO,           "SQLGetInfo", SQLGetInfo, NULL, NULL },
    /* 46 */ { SQL_API_SQLGETSTMTATTR,       "SQLGetStmtAttr", SQLGetStmtAttr, NULL, NULL },
    /* 47 */ { SQL_API_SQLGETSTMTOPTION,     "SQLGetStmtOption", SQLGetStmtOption, NULL, NULL },
    /* 48 */ { SQL_API_SQLGETTYPEINFO,       "SQLGetTypeInfo", SQLGetTypeInfo, NULL, NULL },
    /* 49 */ { SQL_API_SQLMORERESULTS,       "SQLMoreResults", SQLMoreResults, NULL, NULL },
    /* 50 */ { SQL_API_SQLNATIVESQL,         "SQLNativeSql", SQLNativeSql, NULL, NULL },
    /* 51 */ { SQL_API_SQLNUMPARAMS,         "SQLNumParams", SQLNumParams, NULL, NULL },
    /* 52 */ { SQL_API_SQLNUMRESULTCOLS,     "SQLNumResultCols", SQLNumResultCols, NULL, NULL },
    /* 53 */ { SQL_API_SQLPARAMDATA,         "SQLParamData", SQLParamData, NULL, NULL },
    /* 54 */ { SQL_API_SQLPARAMOPTIONS,      "SQLParamOptions", SQLParamOptions, NULL, NULL },
    /* 55 */ { SQL_API_SQLPREPARE,           "SQLPrepare", SQLPrepare, NULL, NULL },
    /* 56 */ { SQL_API_SQLPRIMARYKEYS,       "SQLPrimaryKeys", SQLPrimaryKeys, NULL, NULL },
    /* 57 */ { SQL_API_SQLPROCEDURECOLUMNS,  "SQLProcedureColumns", SQLProcedureColumns, NULL, NULL },
    /* 58 */ { SQL_API_SQLPROCEDURES,        "SQLProcedures", SQLProcedures, NULL, NULL },
    /* 59 */ { SQL_API_SQLPUTDATA,           "SQLPutData", SQLPutData, NULL, NULL },
    /* 60 */ { SQL_API_SQLROWCOUNT,          "SQLRowCount", SQLRowCount, NULL, NULL },
    /* 61 */ { SQL_API_SQLSETCONNECTATTR,    "SQLSetConnectAttr", SQLSetConnectAttr, NULL, NULL },
    /* 62 */ { SQL_API_SQLSETCONNECTOPTION,  "SQLSetConnectOption", SQLSetConnectOption, NULL, NULL },
    /* 63 */ { SQL_API_SQLSETCURSORNAME,     "SQLSetCursorName", SQLSetCursorName, NULL, NULL },
    /* 64 */ { SQL_API_SQLSETDESCFIELD,      "SQLSetDescField", SQLSetDescField, NULL, NULL },
    /* 65 */ { SQL_API_SQLSETDESCREC,        "SQLSetDescRec", SQLSetDescRec, NULL, NULL },
    /* 66 */ { SQL_API_SQLSETENVATTR,        "SQLSetEnvAttr", SQLSetEnvAttr, NULL, NULL },
    /* 67 */ { SQL_API_SQLSETPARAM,          "SQLSetParam", SQLSetParam, NULL, NULL },
    /* 68 */ { SQL_API_SQLSETPOS,            "SQLSetPos", SQLSetPos, NULL, NULL },
    /* 69 */ { SQL_API_SQLSETSCROLLOPTIONS,  "SQLSetScrollOptions", SQLSetScrollOptions, NULL, NULL },
    /* 70 */ { SQL_API_SQLSETSTMTATTR,       "SQLSetStmtAttr", SQLSetStmtAttr, NULL, NULL },
    /* 71 */ { SQL_API_SQLSETSTMTOPTION,     "SQLSetStmtOption", SQLSetStmtOption, NULL, NULL },
    /* 72 */ { SQL_API_SQLSPECIALCOLUMNS,    "SQLSpecialColumns", SQLSpecialColumns, NULL, NULL },
    /* 73 */ { SQL_API_SQLSTATISTICS,        "SQLStatistics", SQLStatistics, NULL, NULL },
    /* 74 */ { SQL_API_SQLTABLEPRIVILEGES,   "SQLTablePrivileges", SQLTablePrivileges, NULL, NULL },
    /* 75 */ { SQL_API_SQLTABLES,            "SQLTables", SQLTables, NULL, NULL },
    /* 76 */ { SQL_API_SQLTRANSACT,          "SQLTransact", SQLTransact, NULL, NULL },
    /* 77 */ { SQL_API_SQLGETDIAGREC,        "SQLGetDiagRec", SQLGetDiagRec, NULL, NULL },
};

static PROXYHANDLE gProxyHandle;

/* What is the difference between these two (dmHandle cf READY_AND_dmHandle)? When does one use one and when the other? */

#define CHECK_dmHandle() \
{ \
        if (gProxyHandle.dmHandle == NULL) \
        { \
                TRACE ("Not ready\n"); \
                return SQL_ERROR; \
        } \
}

#define CHECK_READY_AND_dmHandle() \
{ \
        if (!gProxyHandle.bFunctionReady || gProxyHandle.dmHandle == NULL) \
        { \
                TRACE ("Not ready\n"); \
                return SQL_ERROR; \
        } \
}

static SQLRETURN SQLDummyFunc(void)
{
    TRACE("SQLDummyFunc:\n");
    return SQL_SUCCESS;
}

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
    int success;

    success = 0;
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

            success = 1;
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
                            success = 0;
                        }
                    }
                    else if (reg_ret != ERROR_SUCCESS)
                    {
                        TRACE ("Error %d checking for %s in drivers\n",
                                reg_ret, desc);
                        success = 0;
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
                        success = 0;
                    }
                }
                else
                {
                    WARN ("Unusually long driver name %s not replicated\n",
                            desc);
                    success = 0;
                }
            }
            if (sql_ret != SQL_NO_DATA)
            {
                TRACE ("Error %d enumerating drivers\n", (int)sql_ret);
                success = 0;
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
static void ODBC_ReplicateODBCToRegistry (int is_user, SQLHENV hEnv)
{
    HKEY hODBC;
    LONG reg_ret;
    SQLRETURN sql_ret;
    SQLUSMALLINT dirn;
    CHAR dsn [SQL_MAX_DSN_LENGTH + 1];
    SQLSMALLINT sizedsn;
    CHAR desc [256];
    SQLSMALLINT sizedesc;
    int success;
    const char *which = is_user ? "user" : "system";

    success = 0;
    if ((reg_ret = RegCreateKeyExA (
            is_user ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE,
            "Software\\ODBC\\ODBC.INI", 0, NULL, REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS /* a couple more than we need */, NULL, &hODBC,
            NULL)) == ERROR_SUCCESS)
    {
        success = 1;
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
                            success = 0;
                        }
                    }
                    else if (reg_ret != ERROR_SUCCESS)
                    {
                        TRACE ("Error %d checking for description of %s\n",
                                reg_ret, dsn);
                        success = 0;
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
                    success = 0;
                }
            }
            else
            {
                WARN ("Unusually long %s data source name %s (%s) not "
                        "replicated\n", which, dsn, desc);
                success = 0;
            }
        }
        if (sql_ret != SQL_NO_DATA)
        {
            TRACE ("Error %d enumerating %s datasources\n",
                    (int)sql_ret, which);
            success = 0;
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
        ODBC_ReplicateODBCToRegistry (0 /* system dsns */, hEnv);
        ODBC_ReplicateODBCToRegistry (1 /* user dsns */, hEnv);

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
 *
 * PARAMS
 *     hinstDLL    [I] handle to the DLL's instance
 *     fdwReason   [I]
 *     lpvReserved [I] reserved, must be NULL
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    int i;
    TRACE("Initializing or Finalizing proxy ODBC: %p,%x,%p\n", hinstDLL, fdwReason, lpvReserved);

    if (fdwReason == DLL_PROCESS_ATTACH)
    {
       TRACE("Loading ODBC...\n");
       DisableThreadLibraryCalls(hinstDLL);
       if (ODBC_LoadDriverManager())
       {
          ODBC_LoadDMFunctions();
          ODBC_ReplicateToRegistry();
       }
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
      TRACE("Unloading ODBC...\n");
      if (gProxyHandle.bFunctionReady)
      {
         for ( i = 0; i < NUM_SQLFUNC; i ++ )
         {
            gProxyHandle.functions[i].func = SQLDummyFunc;
         }
      }

      if (gProxyHandle.dmHandle)
      {
         wine_dlclose(gProxyHandle.dmHandle,NULL,0);
         gProxyHandle.dmHandle = NULL;
      }
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

   TRACE("\n");

   gProxyHandle.bFunctionReady = FALSE;

#ifdef SONAME_LIBODBC
   if (!s || !s[0]) s = SONAME_LIBODBC;
#endif
   if (!s || !s[0]) goto failed;

   gProxyHandle.dmHandle = wine_dlopen(s, RTLD_LAZY | RTLD_GLOBAL, error, sizeof(error));

   if (gProxyHandle.dmHandle != NULL)
   {
      gProxyHandle.nErrorType = ERROR_FREE;
      return TRUE;
   }
failed:
   WARN("failed to open library %s: %s\n", debugstr_a(s), error);
   gProxyHandle.nErrorType = ERROR_LIBRARY_NOT_FOUND;
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
    int i;
    char error[256];

    if (gProxyHandle.dmHandle == NULL)
        return FALSE;

    for ( i = 0; i < NUM_SQLFUNC; i ++ )
    {
        char * pFuncNameW;

        gProxyHandle.functions[i] = template_func[i];
        gProxyHandle.functions[i].func = wine_dlsym(gProxyHandle.dmHandle,
                gProxyHandle.functions[i].name, error, sizeof(error));

        if (error[0])
        {
            ERR("Failed to load function %s\n",gProxyHandle.functions[i].name);
            gProxyHandle.functions[i].func = SQLDummyFunc;
        }
        else
        {
            /* Build Unicode function name for this function */
            pFuncNameW = HeapAlloc(GetProcessHeap(), 0, strlen(gProxyHandle.functions[i].name) + 2);
            strcpy(pFuncNameW, gProxyHandle.functions[i].name);
            pFuncNameW[strlen(gProxyHandle.functions[i].name) + 1] = '\0';
            pFuncNameW[strlen(gProxyHandle.functions[i].name)] = 'W';

            gProxyHandle.functions[i].funcW = wine_dlsym(gProxyHandle.dmHandle,
                pFuncNameW, error, sizeof(error));
            if (error[0])
            {
/*                TRACE("Failed to load function %s, possibly no Unicode version is required\n", pFuncNameW); */
                gProxyHandle.functions[i].funcW = NULL;
            }
            HeapFree(GetProcessHeap(), 0, pFuncNameW);
        }
    }

    gProxyHandle.bFunctionReady = TRUE;

    return TRUE;
}


/*************************************************************************
 *				SQLAllocConnect           [ODBC32.001]
 */
SQLRETURN WINAPI SQLAllocConnect(SQLHENV EnvironmentHandle, SQLHDBC *ConnectionHandle)
{
        SQLRETURN ret;
        TRACE("Env=%lx\n",EnvironmentHandle);

        if (!gProxyHandle.bFunctionReady || gProxyHandle.dmHandle == NULL)
        {
           *ConnectionHandle = SQL_NULL_HDBC;
           TRACE("Not ready\n");
           return SQL_ERROR;
        }

        assert(gProxyHandle.functions[SQLAPI_INDEX_SQLALLOCCONNECT].func);
        ret=(gProxyHandle.functions[SQLAPI_INDEX_SQLALLOCCONNECT].func)
            (EnvironmentHandle, ConnectionHandle);
        TRACE("Returns ret=%d, Handle %lx\n",ret, *ConnectionHandle);
        return ret;
}


/*************************************************************************
 *				SQLAllocEnv           [ODBC32.002]
 */
SQLRETURN WINAPI  SQLAllocEnv(SQLHENV *EnvironmentHandle)
{
        SQLRETURN ret;
        TRACE("\n");

        if (!gProxyHandle.bFunctionReady || gProxyHandle.dmHandle == NULL)
        {
           *EnvironmentHandle = SQL_NULL_HENV;
           TRACE("Not ready\n");
           return SQL_ERROR;
        }

        assert(gProxyHandle.functions[SQLAPI_INDEX_SQLALLOCENV].func);
        ret=(gProxyHandle.functions[SQLAPI_INDEX_SQLALLOCENV].func) (EnvironmentHandle);
        TRACE("Returns ret=%d, Env=%lx\n",ret, *EnvironmentHandle);
        return ret;
}


/*************************************************************************
 *				SQLAllocHandle           [ODBC32.024]
 */
SQLRETURN WINAPI SQLAllocHandle(SQLSMALLINT HandleType, SQLHANDLE InputHandle, SQLHANDLE *OutputHandle)
{
        SQLRETURN ret;
        TRACE("(Type=%d, Handle=%lx)\n",HandleType,InputHandle);

        if (!gProxyHandle.bFunctionReady || gProxyHandle.dmHandle == NULL)
        {
            if (gProxyHandle.nErrorType == ERROR_LIBRARY_NOT_FOUND)
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

        assert(gProxyHandle.functions[SQLAPI_INDEX_SQLALLOCHANDLE].func);
        ret=(gProxyHandle.functions[SQLAPI_INDEX_SQLALLOCHANDLE].func)
                   (HandleType, InputHandle, OutputHandle);
        TRACE("Returns ret=%d, Handle=%lx\n",ret, *OutputHandle);
        return ret;
}


/*************************************************************************
 *				SQLAllocStmt           [ODBC32.003]
 */
SQLRETURN WINAPI SQLAllocStmt(SQLHDBC ConnectionHandle, SQLHSTMT *StatementHandle)
{
        SQLRETURN ret;

        TRACE("(Connection=%lx)\n",ConnectionHandle);

        if (!gProxyHandle.bFunctionReady || gProxyHandle.dmHandle == NULL)
        {
           *StatementHandle = SQL_NULL_HSTMT;
           TRACE ("Not ready\n");
           return SQL_ERROR;
        }

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLALLOCSTMT].func);
        ret=(gProxyHandle.functions[SQLAPI_INDEX_SQLALLOCSTMT].func)
            (ConnectionHandle, StatementHandle);
        TRACE ("Returns ret=%d, Handle=%lx\n", ret, *StatementHandle);
        return ret;
}


/*************************************************************************
 *				SQLAllocHandleStd           [ODBC32.077]
 */
SQLRETURN WINAPI SQLAllocHandleStd( SQLSMALLINT HandleType,
                                                         SQLHANDLE InputHandle, SQLHANDLE *OutputHandle)
{
        TRACE("ProxyODBC: SQLAllocHandleStd.\n");

        if (!gProxyHandle.bFunctionReady || gProxyHandle.dmHandle == NULL)
        {
            if (gProxyHandle.nErrorType == ERROR_LIBRARY_NOT_FOUND)
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

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLALLOCHANDLESTD].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLALLOCHANDLESTD].func)
                   (HandleType, InputHandle, OutputHandle);
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

        if (!gProxyHandle.bFunctionReady || gProxyHandle.dmHandle == NULL)
        {
                TRACE ("Not ready\n");
                return SQL_ERROR;
        }

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLBINDCOL].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLBINDCOL].func)
            (StatementHandle, ColumnNumber, TargetType,
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

        CHECK_READY_AND_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLBINDPARAM].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLBINDPARAM].func)
                   (StatementHandle, ParameterNumber, ValueType,
                    ParameterScale, ParameterValue, StrLen_or_Ind);
}


/*************************************************************************
 *				SQLCancel           [ODBC32.005]
 */
SQLRETURN WINAPI SQLCancel(SQLHSTMT StatementHandle)
{
        TRACE("\n");

        CHECK_READY_AND_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLCANCEL].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLCANCEL].func) (StatementHandle);
}


/*************************************************************************
 *				SQLCloseCursor           [ODBC32.026]
 */
SQLRETURN WINAPI  SQLCloseCursor(SQLHSTMT StatementHandle)
{
        SQLRETURN ret;
        TRACE("(Handle=%lx)\n",StatementHandle);

        CHECK_READY_AND_dmHandle();

        assert(gProxyHandle.functions[SQLAPI_INDEX_SQLCLOSECURSOR].func);
        ret=(gProxyHandle.functions[SQLAPI_INDEX_SQLCLOSECURSOR].func) (StatementHandle);
        TRACE("returns %d\n",ret);
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

        CHECK_READY_AND_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLCOLATTRIBUTE].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLCOLATTRIBUTE].func)
            (StatementHandle, ColumnNumber, FieldIdentifier,
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

        CHECK_READY_AND_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLCOLUMNS].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLCOLUMNS].func)
            (StatementHandle, CatalogName, NameLength1,
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

        CHECK_READY_AND_dmHandle();

        strcpy( (LPSTR)gProxyHandle.ServerName, (LPSTR)ServerName );
        strcpy( (LPSTR)gProxyHandle.UserName, (LPSTR)UserName );

        assert(gProxyHandle.functions[SQLAPI_INDEX_SQLCONNECT].func);
        ret=(gProxyHandle.functions[SQLAPI_INDEX_SQLCONNECT].func)
            (ConnectionHandle, ServerName, NameLength1,
            UserName, NameLength2, Authentication, NameLength3);

        TRACE("returns %d\n",ret);
        return ret;
}


/*************************************************************************
 *				SQLCopyDesc           [ODBC32.028]
 */
SQLRETURN WINAPI SQLCopyDesc(SQLHDESC SourceDescHandle, SQLHDESC TargetDescHandle)
{
        TRACE("\n");

        CHECK_READY_AND_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLCOPYDESC].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLCOPYDESC].func)
            (SourceDescHandle, TargetDescHandle);
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

        TRACE("EnvironmentHandle = %p\n", (LPVOID)EnvironmentHandle);

        if (!gProxyHandle.bFunctionReady || gProxyHandle.dmHandle == NULL)
        {
            ERR("Error: empty dm handle (gProxyHandle.dmHandle == NULL)\n");
            return SQL_ERROR;
        }

        assert(gProxyHandle.functions[SQLAPI_INDEX_SQLDATASOURCES].func);
        ret = (gProxyHandle.functions[SQLAPI_INDEX_SQLDATASOURCES].func)
            (EnvironmentHandle, Direction, ServerName,
            BufferLength1, NameLength1, Description, BufferLength2, NameLength2);

        if (TRACE_ON(odbc))
        {
           TRACE("returns: %d \t", ret);
           if (*NameLength1 > 0)
             TRACE("DataSource = %s,", ServerName);
           if (*NameLength2 > 0)
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

        CHECK_READY_AND_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLDESCRIBECOL].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLDESCRIBECOL].func)
            (StatementHandle, ColumnNumber, ColumnName,
            BufferLength, NameLength, DataType, ColumnSize, DecimalDigits, Nullable);
}


/*************************************************************************
 *				SQLDisconnect           [ODBC32.009]
 */
SQLRETURN WINAPI SQLDisconnect(SQLHDBC ConnectionHandle)
{
        SQLRETURN ret;
        TRACE("(Handle=%lx)\n", ConnectionHandle);

        CHECK_READY_AND_dmHandle();

        gProxyHandle.ServerName[0] = '\0';
        gProxyHandle.UserName[0]   = '\0';

        assert(gProxyHandle.functions[SQLAPI_INDEX_SQLDISCONNECT].func);
        ret = (gProxyHandle.functions[SQLAPI_INDEX_SQLDISCONNECT].func) (ConnectionHandle);
        TRACE("returns %d\n",ret);
        return ret;
}


/*************************************************************************
 *				SQLEndTran           [ODBC32.029]
 */
SQLRETURN WINAPI SQLEndTran(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT CompletionType)
{
        TRACE("\n");

        CHECK_READY_AND_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLENDTRAN].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLENDTRAN].func) (HandleType, Handle, CompletionType);
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
        TRACE("\n");

        CHECK_READY_AND_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLERROR].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLERROR].func)
            (EnvironmentHandle, ConnectionHandle, StatementHandle,
            Sqlstate, NativeError, MessageText, BufferLength, TextLength);
}


/*************************************************************************
 *				SQLExecDirect           [ODBC32.011]
 */
SQLRETURN WINAPI SQLExecDirect(SQLHSTMT StatementHandle, SQLCHAR *StatementText, SQLINTEGER TextLength)
{
        TRACE("\n");

        CHECK_READY_AND_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLEXECDIRECT].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLEXECDIRECT].func)
            (StatementHandle, StatementText, TextLength);
}


/*************************************************************************
 *				SQLExecute           [ODBC32.012]
 */
SQLRETURN WINAPI SQLExecute(SQLHSTMT StatementHandle)
{
        TRACE("\n");

        CHECK_READY_AND_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLEXECUTE].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLEXECUTE].func) (StatementHandle);
}


/*************************************************************************
 *				SQLFetch           [ODBC32.013]
 */
SQLRETURN WINAPI SQLFetch(SQLHSTMT StatementHandle)
{
        TRACE("\n");

        CHECK_READY_AND_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLFETCH].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLFETCH].func) (StatementHandle);
}


/*************************************************************************
 *				SQLFetchScroll          [ODBC32.030]
 */
SQLRETURN WINAPI SQLFetchScroll(SQLHSTMT StatementHandle, SQLSMALLINT FetchOrientation, SQLLEN FetchOffset)
{
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLFETCHSCROLL].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLFETCHSCROLL].func)
            (StatementHandle, FetchOrientation, FetchOffset);
}


/*************************************************************************
 *				SQLFreeConnect           [ODBC32.014]
 */
SQLRETURN WINAPI SQLFreeConnect(SQLHDBC ConnectionHandle)
{
        SQLRETURN ret;
        TRACE("(Handle=%lx)\n",ConnectionHandle);

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLFREECONNECT].func);
        ret=(gProxyHandle.functions[SQLAPI_INDEX_SQLFREECONNECT].func) (ConnectionHandle);
        TRACE("Returns %d\n",ret);
        return ret;
}


/*************************************************************************
 *				SQLFreeEnv           [ODBC32.015]
 */
SQLRETURN WINAPI SQLFreeEnv(SQLHENV EnvironmentHandle)
{
        SQLRETURN ret;
        TRACE("(Env=%lx)\n",EnvironmentHandle);

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLFREEENV].func);
        ret = (gProxyHandle.functions[SQLAPI_INDEX_SQLFREEENV].func) (EnvironmentHandle);
        TRACE("Returns %d\n",ret);
        return ret;
}


/*************************************************************************
 *				SQLFreeHandle           [ODBC32.031]
 */
SQLRETURN WINAPI SQLFreeHandle(SQLSMALLINT HandleType, SQLHANDLE Handle)
{
        SQLRETURN ret;
        TRACE("(Type=%d, Handle=%lx)\n",HandleType,Handle);

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLFREEHANDLE].func);
        ret = (gProxyHandle.functions[SQLAPI_INDEX_SQLFREEHANDLE].func)
            (HandleType, Handle);
        TRACE ("Returns %d\n",ret);
        return ret;
}


/*************************************************************************
 *				SQLFreeStmt           [ODBC32.016]
 */
SQLRETURN WINAPI SQLFreeStmt(SQLHSTMT StatementHandle, SQLUSMALLINT Option)
{
        SQLRETURN ret;
        TRACE("(Handle %lx, Option=%d)\n",StatementHandle, Option);

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLFREESTMT].func);
        ret=(gProxyHandle.functions[SQLAPI_INDEX_SQLFREESTMT].func)
            (StatementHandle, Option);
        TRACE("Returns %d\n",ret);
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLGETCONNECTATTR].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLGETCONNECTATTR].func)
            (ConnectionHandle, Attribute, Value,
            BufferLength, StringLength);
}


/*************************************************************************
 *				SQLGetConnectOption       [ODBC32.042]
 */
SQLRETURN WINAPI SQLGetConnectOption(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLPOINTER Value)
{
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLGETCONNECTOPTION].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLGETCONNECTOPTION].func)
            (ConnectionHandle, Option, Value);
}


/*************************************************************************
 *				SQLGetCursorName           [ODBC32.017]
 */
SQLRETURN WINAPI SQLGetCursorName(SQLHSTMT StatementHandle,
             SQLCHAR *CursorName, SQLSMALLINT BufferLength,
             SQLSMALLINT *NameLength)
{
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLGETCURSORNAME].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLGETCURSORNAME].func)
            (StatementHandle, CursorName, BufferLength, NameLength);
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLGETDATA].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLGETDATA].func)
            (StatementHandle, ColumnNumber, TargetType,
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLGETDESCFIELD].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLGETDESCFIELD].func)
            (DescriptorHandle, RecNumber, FieldIdentifier,
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLGETDESCREC].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLGETDESCREC].func)
            (DescriptorHandle, RecNumber, Name, BufferLength,
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLGETDIAGFIELD].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLGETDIAGFIELD].func)
            (HandleType, Handle, RecNumber, DiagIdentifier,
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLGETDIAGREC].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLGETDIAGREC].func)
            (HandleType, Handle, RecNumber, Sqlstate, NativeError,
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLGETENVATTR].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLGETENVATTR].func)
            (EnvironmentHandle, Attribute, Value, BufferLength, StringLength);
}


/*************************************************************************
 *				SQLGetFunctions           [ODBC32.044]
 */
SQLRETURN WINAPI SQLGetFunctions(SQLHDBC ConnectionHandle, SQLUSMALLINT FunctionId, SQLUSMALLINT *Supported)
{
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLGETFUNCTIONS].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLGETFUNCTIONS].func)
            (ConnectionHandle, FunctionId, Supported);
}


/*************************************************************************
 *				SQLGetInfo           [ODBC32.045]
 */
SQLRETURN WINAPI SQLGetInfo(SQLHDBC ConnectionHandle,
             SQLUSMALLINT InfoType, SQLPOINTER InfoValue,
             SQLSMALLINT BufferLength, SQLSMALLINT *StringLength)
{
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLGETINFO].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLGETINFO].func)
            (ConnectionHandle, InfoType, InfoValue, BufferLength, StringLength);
}


/*************************************************************************
 *				SQLGetStmtAttr           [ODBC32.038]
 */
SQLRETURN WINAPI SQLGetStmtAttr(SQLHSTMT StatementHandle,
             SQLINTEGER Attribute, SQLPOINTER Value,
             SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLGETSTMTATTR].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLGETSTMTATTR].func)
            (StatementHandle, Attribute, Value, BufferLength, StringLength);
}


/*************************************************************************
 *				SQLGetStmtOption           [ODBC32.046]
 */
SQLRETURN WINAPI SQLGetStmtOption(SQLHSTMT StatementHandle, SQLUSMALLINT Option, SQLPOINTER Value)
{
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLGETSTMTOPTION].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLGETSTMTOPTION].func)
                (StatementHandle, Option, Value);
}


/*************************************************************************
 *				SQLGetTypeInfo           [ODBC32.047]
 */
SQLRETURN WINAPI SQLGetTypeInfo(SQLHSTMT StatementHandle, SQLSMALLINT DataType)
{
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLGETTYPEINFO].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLGETTYPEINFO].func)
            (StatementHandle, DataType);
}


/*************************************************************************
 *				SQLNumResultCols           [ODBC32.018]
 */
SQLRETURN WINAPI SQLNumResultCols(SQLHSTMT StatementHandle, SQLSMALLINT *ColumnCount)
{
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLNUMRESULTCOLS].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLNUMRESULTCOLS].func)
            (StatementHandle, ColumnCount);
}


/*************************************************************************
 *				SQLParamData           [ODBC32.048]
 */
SQLRETURN WINAPI SQLParamData(SQLHSTMT StatementHandle, SQLPOINTER *Value)
{
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLPARAMDATA].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLPARAMDATA].func)
            (StatementHandle, Value);
}


/*************************************************************************
 *				SQLPrepare           [ODBC32.019]
 */
SQLRETURN WINAPI SQLPrepare(SQLHSTMT StatementHandle, SQLCHAR *StatementText, SQLINTEGER TextLength)
{
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLPREPARE].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLPREPARE].func)
            (StatementHandle, StatementText, TextLength);
}


/*************************************************************************
 *				SQLPutData           [ODBC32.049]
 */
SQLRETURN WINAPI SQLPutData(SQLHSTMT StatementHandle, SQLPOINTER Data, SQLLEN StrLen_or_Ind)
{
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLPUTDATA].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLPUTDATA].func)
            (StatementHandle, Data, StrLen_or_Ind);
}


/*************************************************************************
 *				SQLRowCount           [ODBC32.020]
 */
SQLRETURN WINAPI SQLRowCount(SQLHSTMT StatementHandle, SQLLEN *RowCount)
{
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLROWCOUNT].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLROWCOUNT].func)
            (StatementHandle, RowCount);
}


/*************************************************************************
 *				SQLSetConnectAttr           [ODBC32.039]
 */
SQLRETURN WINAPI SQLSetConnectAttr(SQLHDBC ConnectionHandle, SQLINTEGER Attribute,
        SQLPOINTER Value, SQLINTEGER StringLength)
{
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLSETCONNECTATTR].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLSETCONNECTATTR].func)
            (ConnectionHandle, Attribute, Value, StringLength);
}


/*************************************************************************
 *				SQLSetConnectOption           [ODBC32.050]
 */
SQLRETURN WINAPI SQLSetConnectOption(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLULEN Value)
{
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLSETCONNECTOPTION].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLSETCONNECTOPTION].func)
            (ConnectionHandle, Option, Value);
}


/*************************************************************************
 *				SQLSetCursorName           [ODBC32.021]
 */
SQLRETURN WINAPI SQLSetCursorName(SQLHSTMT StatementHandle, SQLCHAR *CursorName, SQLSMALLINT NameLength)
{
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLSETCURSORNAME].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLSETCURSORNAME].func)
            (StatementHandle, CursorName, NameLength);
}


/*************************************************************************
 *				SQLSetDescField           [ODBC32.073]
 */
SQLRETURN WINAPI SQLSetDescField(SQLHDESC DescriptorHandle,
             SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
             SQLPOINTER Value, SQLINTEGER BufferLength)
{
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLSETDESCFIELD].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLSETDESCFIELD].func)
            (DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength);
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLSETDESCREC].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLSETDESCREC].func)
            (DescriptorHandle, RecNumber, Type, SubType, Length,
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLSETENVATTR].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLSETENVATTR].func)
            (EnvironmentHandle, Attribute, Value, StringLength);
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLSETPARAM].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLSETPARAM].func)
            (StatementHandle, ParameterNumber, ValueType, ParameterType, LengthPrecision,
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLSETSTMTATTR].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLSETSTMTATTR].func)
            (StatementHandle, Attribute, Value, StringLength);
}


/*************************************************************************
 *				SQLSetStmtOption           [ODBC32.051]
 */
SQLRETURN WINAPI SQLSetStmtOption(SQLHSTMT StatementHandle, SQLUSMALLINT Option, SQLULEN Value)
{
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLSETSTMTOPTION].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLSETSTMTOPTION].func)
            (StatementHandle, Option, Value);
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLSPECIALCOLUMNS].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLSPECIALCOLUMNS].func)
            (StatementHandle, IdentifierType, CatalogName, NameLength1, SchemaName,
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLSTATISTICS].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLSTATISTICS].func)
            (StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2,
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLTABLES].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLTABLES].func)
                (StatementHandle, CatalogName, NameLength1,
                SchemaName, NameLength2, TableName, NameLength3, TableType, NameLength4);
}


/*************************************************************************
 *				SQLTransact           [ODBC32.023]
 */
SQLRETURN WINAPI SQLTransact(SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle,
        SQLUSMALLINT CompletionType)
{
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLTRANSACT].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLTRANSACT].func)
            (EnvironmentHandle, ConnectionHandle, CompletionType);
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLBROWSECONNECT].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLBROWSECONNECT].func)
                (hdbc, szConnStrIn, cbConnStrIn, szConnStrOut, cbConnStrOutMax, pcbConnStrOut);
}


/*************************************************************************
 *				SQLBulkOperations           [ODBC32.078]
 */
SQLRETURN WINAPI  SQLBulkOperations(
        SQLHSTMT                        StatementHandle,
        SQLSMALLINT                     Operation)
{
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLBULKOPERATIONS].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLBULKOPERATIONS].func)
                   (StatementHandle, Operation);
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLCOLATTRIBUTES].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLCOLATTRIBUTES].func)
                   (hstmt, icol, fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc);
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLCOLUMNPRIVILEGES].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLCOLUMNPRIVILEGES].func)
                   (hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName,
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLDESCRIBEPARAM].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLDESCRIBEPARAM].func)
                   (hstmt, ipar, pfSqlType, pcbParamDef, pibScale, pfNullable);
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLEXTENDEDFETCH].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLEXTENDEDFETCH].func)
                   (hstmt, fFetchType, irow, pcrow, rgfRowStatus);
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLFOREIGNKEYS].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLFOREIGNKEYS].func)
                   (hstmt, szPkCatalogName, cbPkCatalogName, szPkSchemaName, cbPkSchemaName,
                    szPkTableName, cbPkTableName, szFkCatalogName, cbFkCatalogName, szFkSchemaName,
                        cbFkSchemaName, szFkTableName, cbFkTableName);
}


/*************************************************************************
 *				SQLMoreResults           [ODBC32.061]
 */
SQLRETURN WINAPI SQLMoreResults(SQLHSTMT hstmt)
{
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLMORERESULTS].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLMORERESULTS].func) (hstmt);
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLNATIVESQL].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLNATIVESQL].func)
                   (hdbc, szSqlStrIn, cbSqlStrIn, szSqlStr, cbSqlStrMax, pcbSqlStr);
}


/*************************************************************************
 *				SQLNumParams           [ODBC32.063]
 */
SQLRETURN WINAPI SQLNumParams(
    SQLHSTMT           hstmt,
    SQLSMALLINT           *pcpar)
{
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLNUMPARAMS].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLNUMPARAMS].func) (hstmt, pcpar);
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLPARAMOPTIONS].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLPARAMOPTIONS].func) (hstmt, crow, pirow);
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLPRIMARYKEYS].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLPRIMARYKEYS].func)
                   (hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName,
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLPROCEDURECOLUMNS].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLPROCEDURECOLUMNS].func)
                   (hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName,
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLPROCEDURES].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLPROCEDURES].func)
                   (hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName,
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLSETPOS].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLSETPOS].func)
                   (hstmt, irow, fOption, fLock);
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLTABLEPRIVILEGES].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLTABLEPRIVILEGES].func)
                   (hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName,
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
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLDRIVERS].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLDRIVERS].func)
                (henv, fDirection, szDriverDesc, cbDriverDescMax, pcbDriverDesc,
                 szDriverAttributes, cbDriverAttrMax, pcbDriverAttr);
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLBINDPARAMETER].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLBINDPARAMETER].func)
                (hstmt, ipar, fParamType, fCType, fSqlType, cbColDef, ibScale,
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
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLDRIVERCONNECT].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLDRIVERCONNECT].func)
                 (hdbc, hwnd, conn_str_in, len_conn_str_in, conn_str_out,
                  conn_str_out_max, ptr_conn_str_out, driver_completion);
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLSETSCROLLOPTIONS].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLSETSCROLLOPTIONS].func)
                   (statement_handle, f_concurrency, crow_keyset, crow_rowset);
}

static int SQLColAttributes_KnownStringAttribute(SQLUSMALLINT fDescType)
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
        if (attrList[i] == fDescType) return 1;
    }
    return 0;
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

        TRACE("hstmt=0x%08lx icol=%d fDescType=%d rgbDesc=%p cbDescMax=%d pcbDesc=%p pfDesc=%p\n",
            hstmt, icol, fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc);

        CHECK_dmHandle();
        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLCOLATTRIBUTES].funcW);
        iResult = (gProxyHandle.functions[SQLAPI_INDEX_SQLCOLATTRIBUTES].funcW)
                   (hstmt, icol, fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc);
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

        CHECK_READY_AND_dmHandle();

        WideCharToMultiByte(
            CP_UTF8, 0,
            ServerName, NameLength1,
            gProxyHandle.ServerName, sizeof(gProxyHandle.ServerName),
            NULL, NULL);
        WideCharToMultiByte(
            CP_UTF8, 0,
            UserName, NameLength2,
            gProxyHandle.UserName, sizeof(gProxyHandle.UserName),
            NULL, NULL);

        assert(gProxyHandle.functions[SQLAPI_INDEX_SQLCONNECT].funcW);
        ret=(gProxyHandle.functions[SQLAPI_INDEX_SQLCONNECT].funcW)
            (ConnectionHandle, ServerName, NameLength1,
            UserName, NameLength2, Authentication, NameLength3);

        TRACE("returns %d\n",ret);
        return ret;
}

/*************************************************************************
 *				SQLDescribeColW          [ODBC32.108]
 */
SQLRETURN WINAPI SQLDescribeColW(SQLHSTMT StatementHandle,
             SQLUSMALLINT ColumnNumber, SQLWCHAR *ColumnName,
             SQLSMALLINT BufferLength, SQLSMALLINT *NameLength,
             SQLSMALLINT *DataType, SQLULEN *ColumnSize,
             SQLSMALLINT *DecimalDigits, SQLSMALLINT *Nullable)
{
        SQLRETURN iResult;
        TRACE("\n");

        CHECK_READY_AND_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLDESCRIBECOL].funcW);
        iResult = (gProxyHandle.functions[SQLAPI_INDEX_SQLDESCRIBECOL].funcW)
            (StatementHandle, ColumnNumber, ColumnName,
            BufferLength, NameLength, DataType, ColumnSize, DecimalDigits, Nullable);
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

        CHECK_READY_AND_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLERROR].funcW);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLERROR].funcW)
            (EnvironmentHandle, ConnectionHandle, StatementHandle,
            Sqlstate, NativeError, MessageText, BufferLength, TextLength);
}

/*************************************************************************
 *				SQLExecDirectW          [ODBC32.111]
 */
SQLRETURN WINAPI SQLExecDirectW(SQLHSTMT StatementHandle,
    WCHAR *StatementText, SQLINTEGER TextLength)
{
        TRACE("\n");

        CHECK_READY_AND_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLEXECDIRECT].funcW);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLEXECDIRECT].funcW)
            (StatementHandle, StatementText, TextLength);
}

/*************************************************************************
 *				SQLGetCursorNameW          [ODBC32.117]
 */
SQLRETURN WINAPI SQLGetCursorNameW(SQLHSTMT StatementHandle,
             WCHAR *CursorName, SQLSMALLINT BufferLength,
             SQLSMALLINT *NameLength)
{
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLGETCURSORNAME].funcW);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLGETCURSORNAME].funcW)
            (StatementHandle, CursorName, BufferLength, NameLength);
}

/*************************************************************************
 *				SQLPrepareW          [ODBC32.119]
 */
SQLRETURN WINAPI SQLPrepareW(SQLHSTMT StatementHandle,
    WCHAR *StatementText, SQLINTEGER TextLength)
{
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLPREPARE].funcW);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLPREPARE].funcW)
            (StatementHandle, StatementText, TextLength);
}

/*************************************************************************
 *				SQLSetCursorNameW          [ODBC32.121]
 */
SQLRETURN WINAPI SQLSetCursorNameW(SQLHSTMT StatementHandle, WCHAR *CursorName, SQLSMALLINT NameLength)
{
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLSETCURSORNAME].funcW);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLSETCURSORNAME].funcW)
            (StatementHandle, CursorName, NameLength);
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

        TRACE("StatementHandle=0x%08lx ColumnNumber=%d FieldIdentifier=%d CharacterAttribute=%p BufferLength=%d StringLength=%p NumericAttribute=%p\n",
            StatementHandle, ColumnNumber, FieldIdentifier,
            CharacterAttribute, BufferLength, StringLength, NumericAttribute);

        CHECK_READY_AND_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLCOLATTRIBUTE].funcW);
        iResult = (gProxyHandle.functions[SQLAPI_INDEX_SQLCOLATTRIBUTE].funcW)
            (StatementHandle, ColumnNumber, FieldIdentifier,
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLGETCONNECTATTR].funcW);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLGETCONNECTATTR].funcW)
            (ConnectionHandle, Attribute, Value,
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLGETDESCFIELD].funcW);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLGETDESCFIELD].funcW)
            (DescriptorHandle, RecNumber, FieldIdentifier,
            Value, BufferLength, StringLength);
}

/*************************************************************************
 *				SQLGetDescRecW          [ODBC32.134]
 */
SQLRETURN WINAPI SQLGetDescRecW(SQLHDESC DescriptorHandle,
             SQLSMALLINT RecNumber, SQLWCHAR *Name,
             SQLSMALLINT BufferLength, SQLSMALLINT *StringLength,
             SQLSMALLINT *Type, SQLSMALLINT *SubType,
             SQLLEN *Length, SQLSMALLINT *Precision,
             SQLSMALLINT *Scale, SQLSMALLINT *Nullable)
{
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLGETDESCREC].funcW);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLGETDESCREC].funcW)
            (DescriptorHandle, RecNumber, Name, BufferLength,
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLGETDIAGFIELD].funcW);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLGETDIAGFIELD].funcW)
            (HandleType, Handle, RecNumber, DiagIdentifier,
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLGETDIAGREC].funcW);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLGETDIAGREC].funcW)
            (HandleType, Handle, RecNumber, Sqlstate, NativeError,
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

        TRACE("Attribute = (%02ld) Value = %p BufferLength = (%ld) StringLength = %p\n",
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
            CHECK_dmHandle();

            assert (gProxyHandle.functions[SQLAPI_INDEX_SQLGETSTMTATTR].funcW);
            iResult = (gProxyHandle.functions[SQLAPI_INDEX_SQLGETSTMTATTR].funcW)
                (StatementHandle, Attribute, Value, BufferLength, StringLength);
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLSETCONNECTATTR].funcW);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLSETCONNECTATTR].funcW)
            (ConnectionHandle, Attribute, Value, StringLength);
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

        CHECK_READY_AND_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLCOLUMNS].funcW);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLCOLUMNS].funcW)
            (StatementHandle, CatalogName, NameLength1,
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
        TRACE("ConnStrIn (%d bytes) --> %s\n", len_conn_str_in, debugstr_w(conn_str_in));

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLDRIVERCONNECT].funcW);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLDRIVERCONNECT].funcW)
                 (hdbc, hwnd, conn_str_in, len_conn_str_in, conn_str_out,
                  conn_str_out_max, ptr_conn_str_out, driver_completion);
}

/*************************************************************************
 *				SQLGetConnectOptionW      [ODBC32.142]
 */
SQLRETURN WINAPI SQLGetConnectOptionW(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLPOINTER Value)
{
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLGETCONNECTOPTION].funcW);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLGETCONNECTOPTION].funcW)
            (ConnectionHandle, Option, Value);
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
                CHECK_dmHandle();

                assert (gProxyHandle.functions[SQLAPI_INDEX_SQLGETINFO].funcW);
                iResult = (gProxyHandle.functions[SQLAPI_INDEX_SQLGETINFO].funcW)
                    (ConnectionHandle, InfoType, InfoValue, BufferLength, StringLength);
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLGETTYPEINFO].funcW);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLGETTYPEINFO].funcW)
            (StatementHandle, DataType);
}

/*************************************************************************
 *				SQLSetConnectOptionW          [ODBC32.150]
 */
SQLRETURN WINAPI SQLSetConnectOptionW(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLULEN Value)
{
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLSETCONNECTOPTION].funcW);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLSETCONNECTOPTION].funcW)
            (ConnectionHandle, Option, Value);
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLSPECIALCOLUMNS].funcW);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLSPECIALCOLUMNS].funcW)
            (StatementHandle, IdentifierType, CatalogName, NameLength1, SchemaName,
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLSTATISTICS].funcW);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLSTATISTICS].funcW)
            (StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2,
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLTABLES].funcW);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLTABLES].funcW)
                (StatementHandle, CatalogName, NameLength1,
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLBROWSECONNECT].funcW);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLBROWSECONNECT].funcW)
                (hdbc, szConnStrIn, cbConnStrIn, szConnStrOut, cbConnStrOutMax, pcbConnStrOut);
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLCOLUMNPRIVILEGES].funcW);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLCOLUMNPRIVILEGES].funcW)
                   (hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName,
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

        TRACE("EnvironmentHandle = %p\n", (LPVOID)EnvironmentHandle);

        if (!gProxyHandle.bFunctionReady || gProxyHandle.dmHandle == NULL)
        {
            ERR("Error: empty dm handle (gProxyHandle.dmHandle == NULL)\n");
            return SQL_ERROR;
        }

        assert(gProxyHandle.functions[SQLAPI_INDEX_SQLDATASOURCES].funcW);
        ret = (gProxyHandle.functions[SQLAPI_INDEX_SQLDATASOURCES].funcW)
            (EnvironmentHandle, Direction, ServerName,
            BufferLength1, NameLength1, Description, BufferLength2, NameLength2);

        if (TRACE_ON(odbc))
        {
           TRACE("returns: %d \t", ret);
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLFOREIGNKEYS].funcW);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLFOREIGNKEYS].funcW)
                   (hstmt, szPkCatalogName, cbPkCatalogName, szPkSchemaName, cbPkSchemaName,
                    szPkTableName, cbPkTableName, szFkCatalogName, cbFkCatalogName, szFkSchemaName,
                        cbFkSchemaName, szFkTableName, cbFkTableName);
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLNATIVESQL].funcW);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLNATIVESQL].funcW)
                   (hdbc, szSqlStrIn, cbSqlStrIn, szSqlStr, cbSqlStrMax, pcbSqlStr);
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLPRIMARYKEYS].funcW);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLPRIMARYKEYS].funcW)
                   (hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName,
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLPROCEDURECOLUMNS].funcW);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLPROCEDURECOLUMNS].funcW)
                   (hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName,
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLPROCEDURES].funcW);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLPROCEDURES].funcW)
                   (hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName,
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

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLTABLEPRIVILEGES].funcW);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLTABLEPRIVILEGES].funcW)
                   (hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName,
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
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLDRIVERS].funcW);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLDRIVERS].funcW)
                (henv, fDirection, szDriverDesc, cbDriverDescMax, pcbDriverDesc,
                 szDriverAttributes, cbDriverAttrMax, pcbDriverAttr);
}

/*************************************************************************
 *				SQLSetDescFieldW          [ODBC32.173]
 */
SQLRETURN WINAPI SQLSetDescFieldW(SQLHDESC DescriptorHandle,
             SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
             SQLPOINTER Value, SQLINTEGER BufferLength)
{
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLSETDESCFIELD].funcW);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLSETDESCFIELD].funcW)
            (DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength);
}

/*************************************************************************
 *				SQLSetStmtAttrW          [ODBC32.176]
 */
SQLRETURN WINAPI SQLSetStmtAttrW(SQLHSTMT StatementHandle,
                 SQLINTEGER Attribute, SQLPOINTER Value,
                 SQLINTEGER StringLength)
{
        SQLRETURN iResult;
        TRACE("Attribute = (%02ld) Value = %p StringLength = (%ld)\n",
            Attribute, Value, StringLength);

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLSETSTMTATTR].funcW);
        iResult = (gProxyHandle.functions[SQLAPI_INDEX_SQLSETSTMTATTR].funcW)
            (StatementHandle, Attribute, Value, StringLength);
        if (iResult == SQL_ERROR && (Attribute == SQL_ROWSET_SIZE || Attribute == SQL_ATTR_ROW_ARRAY_SIZE)) {
            TRACE("CHEAT: returning SQL_SUCCESS to ADO...\n");
            iResult = SQL_SUCCESS;
        } else {
            TRACE("returning %d...\n", iResult);
        }
        return iResult;
}


/* End of file */
