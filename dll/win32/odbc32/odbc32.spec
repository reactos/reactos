  1 stdcall SQLAllocConnect(long ptr) ODBC32_SQLAllocConnect
  2 stdcall SQLAllocEnv(ptr) ODBC32_SQLAllocEnv
  3 stdcall SQLAllocStmt(long ptr) ODBC32_SQLAllocStmt
  4 stdcall SQLBindCol(long long long ptr long ptr) ODBC32_SQLBindCol
  5 stdcall SQLCancel(long) ODBC32_SQLCancel
  6 stdcall SQLColAttributes(long long long ptr long ptr ptr) ODBC32_SQLColAttributes
  7 stdcall SQLConnect(long str long str long str long) ODBC32_SQLConnect
  8 stdcall SQLDescribeCol(long long str long ptr ptr ptr ptr ptr) ODBC32_SQLDescribeCol
  9 stdcall SQLDisconnect(long) ODBC32_SQLDisconnect
 10 stdcall SQLError(long long long str ptr str long ptr) ODBC32_SQLError
 11 stdcall SQLExecDirect(long str long) ODBC32_SQLExecDirect
 12 stdcall SQLExecute(long) ODBC32_SQLExecute
 13 stdcall SQLFetch(long) ODBC32_SQLFetch
 14 stdcall SQLFreeConnect(long) ODBC32_SQLFreeConnect
 15 stdcall SQLFreeEnv(long) ODBC32_SQLFreeEnv
 16 stdcall SQLFreeStmt(long long ) ODBC32_SQLFreeStmt
 17 stdcall SQLGetCursorName(long str long ptr) ODBC32_SQLGetCursorName
 18 stdcall SQLNumResultCols(long ptr) ODBC32_SQLNumResultCols
 19 stdcall SQLPrepare(long str long) ODBC32_SQLPrepare
 20 stdcall SQLRowCount(long ptr) ODBC32_SQLRowCount
 21 stdcall SQLSetCursorName(long str long) ODBC32_SQLSetCursorName
 22 stdcall SQLSetParam(long long long long long long ptr ptr) ODBC32_SQLSetParam
 23 stdcall SQLTransact(long long long) ODBC32_SQLTransact
 24 stdcall SQLAllocHandle(long long ptr) ODBC32_SQLAllocHandle
 25 stdcall SQLBindParam(long long long long long long ptr ptr) ODBC32_SQLBindParam
 26 stdcall SQLCloseCursor(long) ODBC32_SQLCloseCursor
 27 stdcall SQLColAttribute(long long long ptr long ptr ptr) ODBC32_SQLColAttribute
 28 stdcall SQLCopyDesc(long long) ODBC32_SQLCopyDesc
 29 stdcall SQLEndTran(long long long) ODBC32_SQLEndTran
 30 stdcall SQLFetchScroll(long long long) ODBC32_SQLFetchScroll
 31 stdcall SQLFreeHandle(long long) ODBC32_SQLFreeHandle
 32 stdcall SQLGetConnectAttr(long long ptr long ptr) ODBC32_SQLGetConnectAttr
 33 stdcall SQLGetDescField(long long long ptr long ptr) ODBC32_SQLGetDescField
 34 stdcall SQLGetDescRec(long long str long ptr ptr ptr ptr ptr ptr ptr) ODBC32_SQLGetDescRec
 35 stdcall SQLGetDiagField(long long long long ptr long ptr) ODBC32_SQLGetDiagField
 36 stdcall SQLGetDiagRec(long long long str ptr str long ptr) ODBC32_SQLGetDiagRec
 37 stdcall SQLGetEnvAttr(long long ptr long ptr) ODBC32_SQLGetEnvAttr
 38 stdcall SQLGetStmtAttr(long long ptr long ptr) ODBC32_SQLGetStmtAttr
 39 stdcall SQLSetConnectAttr(long long ptr long) ODBC32_SQLSetConnectAttr
 40 stdcall SQLColumns(long str long str long str long str long) ODBC32_SQLColumns
 41 stdcall SQLDriverConnect(long long str long str long ptr long) ODBC32_SQLDriverConnect
 42 stdcall SQLGetConnectOption(long long ptr) ODBC32_SQLGetConnectOption
 43 stdcall SQLGetData(long long long ptr long ptr) ODBC32_SQLGetData
 44 stdcall SQLGetFunctions(long long ptr) ODBC32_SQLGetFunctions
 45 stdcall SQLGetInfo(long long ptr long ptr) ODBC32_SQLGetInfo
 46 stdcall SQLGetStmtOption(long long ptr) ODBC32_SQLGetStmtOption
 47 stdcall SQLGetTypeInfo(long long) ODBC32_SQLGetTypeInfo
 48 stdcall SQLParamData(long ptr) ODBC32_SQLParamData
 49 stdcall SQLPutData(long ptr long) ODBC32_SQLPutData
 50 stdcall SQLSetConnectOption(long long long) ODBC32_SQLSetConnectOption
 51 stdcall SQLSetStmtOption(long long long) ODBC32_SQLSetStmtOption
 52 stdcall SQLSpecialColumns(long long str long str long str long long long) ODBC32_SQLSpecialColumns
 53 stdcall SQLStatistics(long str long str long str long long long) ODBC32_SQLStatistics
 54 stdcall SQLTables(long str long str long str long str long) ODBC32_SQLTables
 55 stdcall SQLBrowseConnect(long str long str long ptr) ODBC32_SQLBrowseConnect
 56 stdcall SQLColumnPrivileges(long str long str long str long str long) ODBC32_SQLColumnPrivileges
 57 stdcall SQLDataSources(long long str long ptr str long ptr) ODBC32_SQLDataSources
 58 stdcall SQLDescribeParam(long long ptr ptr ptr ptr) ODBC32_SQLDescribeParam
 59 stdcall SQLExtendedFetch(long long long ptr ptr) ODBC32_SQLExtendedFetch
 60 stdcall SQLForeignKeys(long str long str long str long str long str long str long) ODBC32_SQLForeignKeys
 61 stdcall SQLMoreResults(long) ODBC32_SQLMoreResults
 62 stdcall SQLNativeSql(long str long str long ptr) ODBC32_SQLNativeSql
 63 stdcall SQLNumParams(long ptr) ODBC32_SQLNumParams
 64 stdcall SQLParamOptions(long long ptr) ODBC32_SQLParamOptions
 65 stdcall SQLPrimaryKeys(long str long str long str long) ODBC32_SQLPrimaryKeys
 66 stdcall SQLProcedureColumns(long str long str long str long str long) ODBC32_SQLProcedureColumns
 67 stdcall SQLProcedures(long str long str long str long) ODBC32_SQLProcedures
 68 stdcall SQLSetPos(long long long long) ODBC32_SQLSetPos
 69 stdcall SQLSetScrollOptions(long long long long) ODBC32_SQLSetScrollOptions
 70 stdcall SQLTablePrivileges(long str long str long str long) ODBC32_SQLTablePrivileges
 71 stdcall SQLDrivers(long long str long ptr str long ptr) ODBC32_SQLDrivers
 72 stdcall SQLBindParameter(long long long long long long long ptr long ptr) ODBC32_SQLBindParameter
 73 stdcall SQLSetDescField(long long long ptr long) ODBC32_SQLSetDescField
 74 stdcall SQLSetDescRec(long long long long long long long ptr ptr ptr) ODBC32_SQLSetDescRec
 75 stdcall SQLSetEnvAttr(long long ptr long) ODBC32_SQLSetEnvAttr
 76 stdcall SQLSetStmtAttr(long long ptr long) ODBC32_SQLSetStmtAttr
 77 stdcall SQLAllocHandleStd(long long ptr) ODBC32_SQLAllocHandleStd
 78 stdcall SQLBulkOperations(long long) ODBC32_SQLBulkOperations
 79 stub    CloseODBCPerfData
 80 stub    CollectODBCPerfData
 81 stub    CursorLibLockDbc
 82 stub    CursorLibLockDesc
 83 stub    CursorLibLockStmt
 84 stub    ODBCGetTryWaitValue
 85 stub    CursorLibTransact
 86 stub    ODBSetTryWaitValue
 89 stub    ODBCSharedPerfMon
 90 stub    ODBCSharedVSFlag
106 stdcall SQLColAttributesW(long long long ptr long ptr ptr) ODBC32_SQLColAttributesW
107 stdcall SQLConnectW(long wstr long wstr long wstr long) ODBC32_SQLConnectW
108 stdcall SQLDescribeColW(long long wstr long ptr ptr ptr ptr ptr) ODBC32_SQLDescribeColW
110 stdcall SQLErrorW(long long long wstr ptr wstr long ptr) ODBC32_SQLErrorW
111 stdcall SQLExecDirectW(long wstr long) ODBC32_SQLExecDirectW
117 stdcall SQLGetCursorNameW(long wstr long ptr) ODBC32_SQLGetCursorNameW
119 stdcall SQLPrepareW(long wstr long) ODBC32_SQLPrepareW
121 stdcall SQLSetCursorNameW(long wstr long) ODBC32_SQLSetCursorNameW
127 stdcall SQLColAttributeW(long long long ptr long ptr ptr) ODBC32_SQLColAttributeW
132 stdcall SQLGetConnectAttrW(long long ptr long ptr) ODBC32_SQLGetConnectAttrW
133 stdcall SQLGetDescFieldW(long long long ptr long ptr) ODBC32_SQLGetDescFieldW
134 stdcall SQLGetDescRecW(long long wstr long ptr ptr ptr ptr ptr ptr ptr) ODBC32_SQLGetDescRecW
135 stdcall SQLGetDiagFieldW(long long long long ptr long ptr) ODBC32_SQLGetDiagFieldW
136 stdcall SQLGetDiagRecW(long long long wstr ptr wstr long ptr) ODBC32_SQLGetDiagRecW
138 stdcall SQLGetStmtAttrW(long long ptr long ptr) ODBC32_SQLGetStmtAttrW
139 stdcall SQLSetConnectAttrW(long long ptr long) ODBC32_SQLSetConnectAttrW
140 stdcall SQLColumnsW(long wstr long wstr long wstr long wstr long) ODBC32_SQLColumnsW
141 stdcall SQLDriverConnectW(long long wstr long wstr long ptr long) ODBC32_SQLDriverConnectW
142 stdcall SQLGetConnectOptionW(long long ptr) ODBC32_SQLGetConnectOptionW
145 stdcall SQLGetInfoW(long long ptr long ptr) ODBC32_SQLGetInfoW
147 stdcall SQLGetTypeInfoW(long long) ODBC32_SQLGetTypeInfoW
150 stdcall SQLSetConnectOptionW(long long long) ODBC32_SQLSetConnectOptionW
152 stdcall SQLSpecialColumnsW(long long wstr long wstr long wstr long long long) ODBC32_SQLSpecialColumnsW
153 stdcall SQLStatisticsW(long wstr long wstr long wstr long long long) ODBC32_SQLStatisticsW
154 stdcall SQLTablesW(long wstr long wstr long wstr long wstr long) ODBC32_SQLTablesW
155 stdcall SQLBrowseConnectW(long wstr long wstr long ptr) ODBC32_SQLBrowseConnectW
156 stdcall SQLColumnPrivilegesW(long wstr long wstr long wstr long wstr long) ODBC32_SQLColumnPrivilegesW
157 stdcall SQLDataSourcesW(long long wstr long ptr wstr long ptr) ODBC32_SQLDataSourcesW
160 stdcall SQLForeignKeysW(long wstr long wstr long wstr long wstr long wstr long wstr long) ODBC32_SQLForeignKeysW
162 stdcall SQLNativeSqlW(long wstr long wstr long ptr) ODBC32_SQLNativeSqlW
165 stdcall SQLPrimaryKeysW(long wstr long wstr long wstr long) ODBC32_SQLPrimaryKeysW
166 stdcall SQLProcedureColumnsW(long wstr long wstr long wstr long wstr long) ODBC32_SQLProcedureColumnsW
167 stdcall SQLProceduresW(long wstr long wstr long wstr long) ODBC32_SQLProceduresW
170 stdcall SQLTablePrivilegesW(long wstr long wstr long wstr long) ODBC32_SQLTablePrivilegesW
171 stdcall SQLDriversW(long long wstr long ptr wstr long ptr) ODBC32_SQLDriversW
173 stdcall SQLSetDescFieldW(long long long ptr long) ODBC32_SQLSetDescFieldW
176 stdcall SQLSetStmtAttrW(long long ptr long) ODBC32_SQLSetStmtAttrW
206 stub    SQLColAttributesA
207 stub    SQLConnectA
208 stub    SQLDescribeColA
210 stub    SQLErrorA
211 stub    SQLExecDirectA
217 stub    SQLGetCursorNameA
219 stub    SQLPrepareA
221 stub    SQLSetCursorNameA
227 stub    SQLColAttributeA
232 stub    SQLGetConnectAttrA
233 stub    SQLGetDescFieldA
234 stub    SQLGetDescRecA
235 stub    SQLGetDiagFieldA
236 stdcall SQLGetDiagRecA(long long long ptr ptr ptr long ptr) ODBC32_SQLGetDiagRecA
238 stub    SQLGetStmtAttrA
239 stub    SQLSetConnectAttrA
240 stub    SQLColumnsA
241 stub    SQLDriverConnectA
242 stub    SQLGetConnectOptionA
245 stub    SQLGetInfoA
247 stub    SQLGetTypeInfoA
250 stub    SQLSetConnectOptionA
252 stub    SQLSpecialColumnsA
253 stub    SQLStatisticsA
254 stub    SQLTablesA
255 stub    SQLBrowseConnectA
256 stub    SQLColumnPrivilegesA
257 stdcall SQLDataSourcesA(long long str long ptr str long ptr) ODBC32_SQLDataSourcesA
260 stub    SQLForeignKeysA
262 stub    SQLNativeSqlA
265 stub    SQLPrimaryKeysA
266 stub    SQLProcedureColumnsA
267 stub    SQLProceduresA
270 stub    SQLTablePrivilegesA
271 stub    SQLDriversA
273 stub    SQLSetDescFieldA
276 stub    SQLSetStmtAttrA
300 stub    ODBCSharedTraceFlag
301 stub    ODBCQualifyFileDSNW


 @ stub    LockHandle
 @ stub    ODBCInternalConnectW
 @ stub    OpenODBCPerfData
 @ stub    PostComponentError
 @ stub    PostODBCComponentError
 @ stub    PostODBCError
 @ stub    SearchStatusCode
 @ stub    VFreeErrors
 @ stub    VRetrieveDriverErrorsRowCol
 @ stub    ValidateErrorQueue
