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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#include "wine/debug.h"
//#include "wine/library.h"

#include "sql.h"
#include "sqltypes.h"
#include "sqlext.h"

#include "proxyodbc.h"

static BOOL ODBC_LoadDriverManager(void);
static BOOL ODBC_LoadDMFunctions(void);

WINE_DEFAULT_DEBUG_CHANNEL(odbc);

static const DM_FUNC template_func[] =
{
    /* 00 */ { SQL_API_SQLALLOCCONNECT,      "SQLAllocConnect", SQLAllocConnect, NULL },
    /* 01 */ { SQL_API_SQLALLOCENV,          "SQLAllocEnv", SQLAllocEnv, NULL  },
    /* 02 */ { SQL_API_SQLALLOCHANDLE,       "SQLAllocHandle", SQLAllocHandle, NULL },
    /* 03 */ { SQL_API_SQLALLOCSTMT,         "SQLAllocStmt", SQLAllocStmt, NULL },
    /* 04 */ { SQL_API_SQLALLOCHANDLESTD,    "SQLAllocHandleStd", SQLAllocHandleStd, NULL },
    /* 05 */ { SQL_API_SQLBINDCOL,           "SQLBindCol", SQLBindCol, NULL },
    /* 06 */ { SQL_API_SQLBINDPARAM,         "SQLBindParam", SQLBindParam, NULL },
    /* 07 */ { SQL_API_SQLBINDPARAMETER,     "SQLBindParameter", SQLBindParameter, NULL },
    /* 08 */ { SQL_API_SQLBROWSECONNECT,     "SQLBrowseConnect", SQLBrowseConnect, NULL },
    /* 09 */ { SQL_API_SQLBULKOPERATIONS,    "SQLBulkOperations", SQLBulkOperations, NULL },
    /* 10 */ { SQL_API_SQLCANCEL,            "SQLCancel", SQLCancel, NULL },
    /* 11 */ { SQL_API_SQLCLOSECURSOR,       "SQLCloseCursor", SQLCloseCursor, NULL },
    /* 12 */ { SQL_API_SQLCOLATTRIBUTE,      "SQLColAttribute", SQLColAttribute, NULL },
    /* 13 */ { SQL_API_SQLCOLATTRIBUTES,     "SQLColAttributes", SQLColAttributes, NULL },
    /* 14 */ { SQL_API_SQLCOLUMNPRIVILEGES,  "SQLColumnPrivileges", SQLColumnPrivileges, NULL },
    /* 15 */ { SQL_API_SQLCOLUMNS,           "SQLColumns", SQLColumns, NULL },
    /* 16 */ { SQL_API_SQLCONNECT,           "SQLConnect", SQLConnect, NULL },
    /* 17 */ { SQL_API_SQLCOPYDESC,          "SQLCopyDesc", SQLCopyDesc, NULL },
    /* 18 */ { SQL_API_SQLDATASOURCES,       "SQLDataSources", SQLDataSources, NULL },
    /* 19 */ { SQL_API_SQLDESCRIBECOL,       "SQLDescribeCol", SQLDescribeCol, NULL },
    /* 20 */ { SQL_API_SQLDESCRIBEPARAM,     "SQLDescribeParam", SQLDescribeParam, NULL },
    /* 21 */ { SQL_API_SQLDISCONNECT,        "SQLDisconnect", SQLDisconnect, NULL },
    /* 22 */ { SQL_API_SQLDRIVERCONNECT,     "SQLDriverConnect", SQLDriverConnect, NULL },
    /* 23 */ { SQL_API_SQLDRIVERS,           "SQLDrivers", SQLDrivers, NULL },
    /* 24 */ { SQL_API_SQLENDTRAN,           "SQLEndTran", SQLEndTran, NULL },
    /* 25 */ { SQL_API_SQLERROR,             "SQLError", SQLError, NULL },
    /* 26 */ { SQL_API_SQLEXECDIRECT,        "SQLExecDirect", SQLExecDirect, NULL },
    /* 27 */ { SQL_API_SQLEXECUTE,           "SQLExecute", SQLExecute, NULL },
    /* 28 */ { SQL_API_SQLEXTENDEDFETCH,     "SQLExtendedFetch", SQLExtendedFetch, NULL },
    /* 29 */ { SQL_API_SQLFETCH,             "SQLFetch", SQLFetch, NULL },
    /* 30 */ { SQL_API_SQLFETCHSCROLL,       "SQLFetchScroll", SQLFetchScroll, NULL },
    /* 31 */ { SQL_API_SQLFOREIGNKEYS,       "SQLForeignKeys", SQLForeignKeys, NULL },
    /* 32 */ { SQL_API_SQLFREEENV,           "SQLFreeEnv", SQLFreeEnv, NULL },
    /* 33 */ { SQL_API_SQLFREEHANDLE,        "SQLFreeHandle", SQLFreeHandle, NULL },
    /* 34 */ { SQL_API_SQLFREESTMT,          "SQLFreeStmt", SQLFreeStmt, NULL },
    /* 35 */ { SQL_API_SQLFREECONNECT,       "SQLFreeConnect", SQLFreeConnect, NULL },
    /* 36 */ { SQL_API_SQLGETCONNECTATTR,    "SQLGetConnectAttr", SQLGetConnectAttr, NULL },
    /* 37 */ { SQL_API_SQLGETCONNECTOPTION,  "SQLGetConnectOption", SQLGetConnectOption, NULL },
    /* 38 */ { SQL_API_SQLGETCURSORNAME,     "SQLGetCursorName", SQLGetCursorName, NULL },
    /* 39 */ { SQL_API_SQLGETDATA,           "SQLGetData", SQLGetData, NULL },
    /* 40 */ { SQL_API_SQLGETDESCFIELD,      "SQLGetDescField", SQLGetDescField, NULL },
    /* 41 */ { SQL_API_SQLGETDESCREC,        "SQLGetDescRec", SQLGetDescRec, NULL },
    /* 42 */ { SQL_API_SQLGETDIAGFIELD,      "SQLGetDiagField", SQLGetDiagField, NULL },
    /* 43 */ { SQL_API_SQLGETENVATTR,        "SQLGetEnvAttr", SQLGetEnvAttr, NULL },
    /* 44 */ { SQL_API_SQLGETFUNCTIONS,      "SQLGetFunctions", SQLGetFunctions, NULL },
    /* 45 */ { SQL_API_SQLGETINFO,           "SQLGetInfo", SQLGetInfo, NULL },
    /* 46 */ { SQL_API_SQLGETSTMTATTR,       "SQLGetStmtAttr", SQLGetStmtAttr, NULL },
    /* 47 */ { SQL_API_SQLGETSTMTOPTION,     "SQLGetStmtOption", SQLGetStmtOption, NULL },
    /* 48 */ { SQL_API_SQLGETTYPEINFO,       "SQLGetTypeInfo", SQLGetTypeInfo, NULL },
    /* 49 */ { SQL_API_SQLMORERESULTS,       "SQLMoreResults", SQLMoreResults, NULL },
    /* 50 */ { SQL_API_SQLNATIVESQL,         "SQLNativeSql", SQLNativeSql, NULL },
    /* 51 */ { SQL_API_SQLNUMPARAMS,         "SQLNumParams", SQLNumParams, NULL },
    /* 52 */ { SQL_API_SQLNUMRESULTCOLS,     "SQLNumResultCols", SQLNumResultCols, NULL },
    /* 53 */ { SQL_API_SQLPARAMDATA,         "SQLParamData", SQLParamData, NULL },
    /* 54 */ { SQL_API_SQLPARAMOPTIONS,      "SQLParamOptions", SQLParamOptions, NULL },
    /* 55 */ { SQL_API_SQLPREPARE,           "SQLPrepare", SQLPrepare, NULL },
    /* 56 */ { SQL_API_SQLPRIMARYKEYS,       "SQLPrimaryKeys", SQLPrimaryKeys, NULL },
    /* 57 */ { SQL_API_SQLPROCEDURECOLUMNS,  "SQLProcedureColumns", SQLProcedureColumns, NULL },
    /* 58 */ { SQL_API_SQLPROCEDURES,        "SQLProcedures", SQLProcedures, NULL },
    /* 59 */ { SQL_API_SQLPUTDATA,           "SQLPutData", SQLPutData, NULL },
    /* 60 */ { SQL_API_SQLROWCOUNT,          "SQLRowCount", SQLRowCount, NULL },
    /* 61 */ { SQL_API_SQLSETCONNECTATTR,    "SQLSetConnectAttr", SQLSetConnectAttr, NULL },
    /* 62 */ { SQL_API_SQLSETCONNECTOPTION,  "SQLSetConnectOption", SQLSetConnectOption, NULL },
    /* 63 */ { SQL_API_SQLSETCURSORNAME,     "SQLSetCursorName", SQLSetCursorName, NULL },
    /* 64 */ { SQL_API_SQLSETDESCFIELD,      "SQLSetDescField", SQLSetDescField, NULL },
    /* 65 */ { SQL_API_SQLSETDESCREC,        "SQLSetDescRec", SQLSetDescRec, NULL },
    /* 66 */ { SQL_API_SQLSETENVATTR,        "SQLSetEnvAttr", SQLSetEnvAttr, NULL },
    /* 67 */ { SQL_API_SQLSETPARAM,          "SQLSetParam", SQLSetParam, NULL },
    /* 68 */ { SQL_API_SQLSETPOS,            "SQLSetPos", SQLSetPos, NULL },
    /* 69 */ { SQL_API_SQLSETSCROLLOPTIONS,  "SQLSetScrollOptions", SQLSetScrollOptions, NULL },
    /* 70 */ { SQL_API_SQLSETSTMTATTR,       "SQLSetStmtAttr", SQLSetStmtAttr, NULL },
    /* 71 */ { SQL_API_SQLSETSTMTOPTION,     "SQLSetStmtOption", SQLSetStmtOption, NULL },
    /* 72 */ { SQL_API_SQLSPECIALCOLUMNS,    "SQLSpecialColumns", SQLSpecialColumns, NULL },
    /* 73 */ { SQL_API_SQLSTATISTICS,        "SQLStatistics", SQLStatistics, NULL },
    /* 74 */ { SQL_API_SQLTABLEPRIVILEGES,   "SQLTablePrivileges", SQLTablePrivileges, NULL },
    /* 75 */ { SQL_API_SQLTABLES,            "SQLTables", SQLTables, NULL },
    /* 76 */ { SQL_API_SQLTRANSACT,          "SQLTransact", SQLTransact, NULL },
    /* 77 */ { SQL_API_SQLGETDIAGREC,        "SQLGetDiagRec", SQLGetDiagRec, NULL },
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

SQLRETURN SQLDummyFunc()
{
    TRACE("SQLDummyFunc: \n");
    return SQL_SUCCESS;
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
    TRACE("Initializing or Finalizing proxy ODBC: %p,%lx,%p\n", hinstDLL, fdwReason, lpvReserved);

    if (fdwReason == DLL_PROCESS_ATTACH)
    {
       TRACE("Loading ODBC...\n");
       DisableThreadLibraryCalls(hinstDLL);
       if (ODBC_LoadDriverManager())
          ODBC_LoadDMFunctions();
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
#ifndef __REACTOS__
      if (gProxyHandle.dmHandle)
      {
         wine_dlclose(gProxyHandle.dmHandle,NULL,0);
         gProxyHandle.dmHandle = NULL;
      }
#endif
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
   gProxyHandle.nErrorType = ERROR_LIBRARY_NOT_FOUND;

   if (s!= NULL && strlen (s) >= sizeof(gProxyHandle.dmLibName))
   {
          ERR("Driver name too long (%s)\n",s);
          return FALSE;
   }
   if (s == NULL || strlen(s) == 0)
          s = "libodbc.so";
   strcpy(gProxyHandle.dmLibName, s);
#ifndef __REACTOS__
   gProxyHandle.dmHandle = wine_dlopen(gProxyHandle.dmLibName, RTLD_LAZY, error, sizeof(error));

   if (gProxyHandle.dmHandle == NULL)           /* fail to load unixODBC driver manager */
   {
           WARN("failed to open library %s: %s\n", gProxyHandle.dmLibName, error);
           gProxyHandle.dmLibName[0] = '\0';
           gProxyHandle.nErrorType = ERROR_LIBRARY_NOT_FOUND;
           return FALSE;
   }
   else
   {
      gProxyHandle.nErrorType = ERROR_FREE;
      return TRUE;
   }
#else
   return FALSE;
#endif
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
#ifndef __REACTOS__
    for ( i = 0; i < NUM_SQLFUNC; i ++ )
    {
        gProxyHandle.functions[i] = template_func[i];
        gProxyHandle.functions[i].func = wine_dlsym(gProxyHandle.dmHandle,
                gProxyHandle.functions[i].name, error, sizeof(error));

        if (error[0])
        {
            ERR("Failed to load function %s\n",gProxyHandle.functions[i].name);
            gProxyHandle.functions[i].func = SQLDummyFunc;
        }
    }

    gProxyHandle.bFunctionReady = TRUE;
#endif
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
                WARN("ProxyODBC: Can not load ODBC driver manager library.\n");

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
        TRACE("ProxyODBC: SQLAllocHandelStd.\n");

        if (!gProxyHandle.bFunctionReady || gProxyHandle.dmHandle == NULL)
        {
            if (gProxyHandle.nErrorType == ERROR_LIBRARY_NOT_FOUND)
                WARN("ProxyODBC: Can not load ODBC driver manager library.\n");

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
                     SQLPOINTER TargetValue, SQLINTEGER BufferLength,
                     SQLINTEGER *StrLen_or_Ind)
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
             SQLSMALLINT ParameterType, SQLUINTEGER LengthPrecision,
             SQLSMALLINT ParameterScale, SQLPOINTER ParameterValue,
             SQLINTEGER *StrLen_or_Ind)
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

        strcpy(gProxyHandle.ServerName, ServerName);
        strcpy(gProxyHandle.UserName, UserName);

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

#ifndef __REACTOS__
        if (TRACE_ON(odbc))
#endif
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
             SQLSMALLINT *DataType, SQLUINTEGER *ColumnSize,
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
SQLRETURN WINAPI SQLFetchScroll(SQLHSTMT StatementHandle, SQLSMALLINT FetchOrientation, SQLINTEGER FetchOffset)
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
             SQLPOINTER TargetValue, SQLINTEGER BufferLength,
             SQLINTEGER *StrLen_or_Ind)
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
             SQLINTEGER *Length, SQLSMALLINT *Precision,
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
SQLRETURN WINAPI SQLPutData(SQLHSTMT StatementHandle, SQLPOINTER Data, SQLINTEGER StrLen_or_Ind)
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
SQLRETURN WINAPI SQLRowCount(SQLHSTMT StatementHandle, SQLINTEGER *RowCount)
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
SQLRETURN WINAPI SQLSetConnectOption(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLUINTEGER Value)
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
             SQLSMALLINT SubType, SQLINTEGER Length,
             SQLSMALLINT Precision, SQLSMALLINT Scale,
             SQLPOINTER Data, SQLINTEGER *StringLength,
             SQLINTEGER *Indicator)
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
             SQLSMALLINT ParameterType, SQLUINTEGER LengthPrecision,
             SQLSMALLINT ParameterScale, SQLPOINTER ParameterValue,
             SQLINTEGER *StrLen_or_Ind)
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
SQLRETURN WINAPI SQLSetStmtOption(SQLHSTMT StatementHandle, SQLUSMALLINT Option, SQLUINTEGER Value)
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
    SQLINTEGER            *pfDesc)
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
    SQLUINTEGER           *pcbParamDef,
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
    SQLUINTEGER        cbColDef,
    SQLSMALLINT        ibScale,
    SQLPOINTER         rgbValue,
    SQLINTEGER         cbValueMax,
    SQLINTEGER            *pcbValue)
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
    SQLINTEGER         crow_keyset,
    SQLUSMALLINT       crow_rowset )
{
        TRACE("\n");

        CHECK_dmHandle();

        assert (gProxyHandle.functions[SQLAPI_INDEX_SQLSETSCROLLOPTIONS].func);
        return (gProxyHandle.functions[SQLAPI_INDEX_SQLSETSCROLLOPTIONS].func)
                   (statement_handle, f_concurrency, crow_keyset, crow_rowset);
}

/* End of file */
