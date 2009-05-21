  1 stdcall SQLAllocConnect(long ptr)
  2 stdcall SQLAllocEnv(ptr)
  3 stdcall SQLAllocStmt(long ptr)
  4 stdcall SQLBindCol(long long long ptr long ptr)
  5 stdcall SQLCancel(long)
  6 stdcall SQLColAttributes(long long long ptr long ptr ptr)
  7 stdcall SQLConnect(long str long str long str long)
  8 stdcall SQLDescribeCol(long long str long ptr ptr ptr ptr ptr)
  9 stdcall SQLDisconnect(long)
 10 stdcall SQLError(long long long str ptr str long ptr)
 11 stdcall SQLExecDirect(long str long)
 12 stdcall SQLExecute(long)
 13 stdcall SQLFetch(long)
 14 stdcall SQLFreeConnect(long)
 15 stdcall SQLFreeEnv(long)
 16 stdcall SQLFreeStmt(long long )
 17 stdcall SQLGetCursorName(long str long ptr)
 18 stdcall SQLNumResultCols(long ptr)
 19 stdcall SQLPrepare(long str long)
 20 stdcall SQLRowCount(long ptr)
 21 stdcall SQLSetCursorName(long str long)
 22 stdcall SQLSetParam(long long long long long long ptr ptr)
 23 stdcall SQLTransact(long long long)
 24 stdcall SQLAllocHandle(long long ptr)
 25 stdcall SQLBindParam(long long long long long long ptr ptr)
 26 stdcall SQLCloseCursor(long)
 27 stdcall SQLColAttribute(long long long ptr long ptr ptr)
 28 stdcall SQLCopyDesc(long long)
 29 stdcall SQLEndTran(long long long)
 30 stdcall SQLFetchScroll(long long long)
 31 stdcall SQLFreeHandle(long long)
 32 stdcall SQLGetConnectAttr(long long ptr long ptr)
 33 stdcall SQLGetDescField(long long long ptr long ptr)
 34 stdcall SQLGetDescRec(long long str long ptr ptr ptr ptr ptr ptr ptr)
 35 stdcall SQLGetDiagField(long long long long ptr long ptr)
 36 stdcall SQLGetDiagRec(long long long str ptr str long ptr)
 37 stdcall SQLGetEnvAttr(long long ptr long ptr)
 38 stdcall SQLGetStmtAttr(long long ptr long ptr)
 39 stdcall SQLSetConnectAttr(long long ptr long)
 40 stdcall SQLColumns(long str long str long str long str long)
 41 stdcall SQLDriverConnect(long long str long str long str long)
 42 stdcall SQLGetConnectOption(long long ptr)
 43 stdcall SQLGetData(long long long ptr long ptr)
 44 stdcall SQLGetFunctions(long long ptr)
 45 stdcall SQLGetInfo(long long ptr long ptr)
 46 stdcall SQLGetStmtOption(long long ptr)
 47 stdcall SQLGetTypeInfo(long long)
 48 stdcall SQLParamData(long ptr)
 49 stdcall SQLPutData(long ptr long)
 50 stdcall SQLSetConnectOption(long long long)
 51 stdcall SQLSetStmtOption(long long long)
 52 stdcall SQLSpecialColumns(long long str long str long str long long long)
 53 stdcall SQLStatistics(long str long str long str long long long)
 54 stdcall SQLTables(long str long str long str long str long)
 55 stdcall SQLBrowseConnect(long str long str long ptr)
 56 stdcall SQLColumnPrivileges(long str long str long str long str long)
 57 stdcall SQLDataSources(long long str long ptr str long ptr)
 58 stdcall SQLDescribeParam(long long ptr ptr ptr ptr)
 59 stdcall SQLExtendedFetch(long long long ptr ptr)
 60 stdcall SQLForeignKeys(long str long str long str long str long str long str long)
 61 stdcall SQLMoreResults(long)
 62 stdcall SQLNativeSql(long str long str long ptr)
 63 stdcall SQLNumParams(long ptr)
 64 stdcall SQLParamOptions(long long ptr)
 65 stdcall SQLPrimaryKeys(long str long str long str long)
 66 stdcall SQLProcedureColumns(long str long str long str long str long)
 67 stdcall SQLProcedures(long str long str long str long)
 68 stdcall SQLSetPos(long long long long)
 69 stdcall SQLSetScrollOptions(long long long long)
 70 stdcall SQLTablePrivileges(long str long str long str long)
 71 stdcall SQLDrivers(long long str long ptr str long ptr)
 72 stdcall SQLBindParameter(long long long long long long long ptr long ptr)
 73 stdcall SQLSetDescField(long long long ptr long)
 74 stdcall SQLSetDescRec(long long long long long long long ptr ptr ptr)
 75 stdcall SQLSetEnvAttr(long long ptr long)
 76 stdcall SQLSetStmtAttr(long long ptr long)
 77 stdcall SQLAllocHandleStd(long long ptr)
 78 stdcall SQLBulkOperations(long long)
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
106 stdcall SQLColAttributesW(long long long ptr long ptr ptr)
107 stdcall SQLConnectW(long wstr long wstr long wstr long)
108 stdcall SQLDescribeColW(long long wstr long ptr ptr ptr ptr ptr)
110 stdcall SQLErrorW(long long long wstr ptr wstr long ptr)
111 stdcall SQLExecDirectW(long wstr long)
117 stdcall SQLGetCursorNameW(long wstr long ptr)
119 stdcall SQLPrepareW(long wstr long)
121 stdcall SQLSetCursorNameW(long wstr long)
127 stdcall SQLColAttributeW(long long long ptr long ptr ptr)
132 stdcall SQLGetConnectAttrW(long long ptr long ptr)
133 stdcall SQLGetDescFieldW(long long long ptr long ptr)
134 stdcall SQLGetDescRecW(long long wstr long ptr ptr ptr ptr ptr ptr ptr)
135 stdcall SQLGetDiagFieldW(long long long long ptr long ptr)
136 stdcall SQLGetDiagRecW(long long long wstr ptr wstr long ptr)
138 stdcall SQLGetStmtAttrW(long long ptr long ptr)
139 stdcall SQLSetConnectAttrW(long long ptr long)
140 stdcall SQLColumnsW(long wstr long wstr long wstr long wstr long)
141 stdcall SQLDriverConnectW(long long wstr long wstr long wstr long)
142 stdcall SQLGetConnectOptionW(long long ptr)
145 stdcall SQLGetInfoW(long long ptr long ptr)
147 stdcall SQLGetTypeInfoW(long long)
150 stdcall SQLSetConnectOptionW(long long long)
152 stdcall SQLSpecialColumnsW(long long wstr long wstr long wstr long long long)
153 stdcall SQLStatisticsW(long wstr long wstr long wstr long long long)
154 stdcall SQLTablesW(long wstr long wstr long wstr long wstr long)
155 stdcall SQLBrowseConnectW(long wstr long wstr long ptr)
156 stdcall SQLColumnPrivilegesW(long wstr long wstr long wstr long wstr long)
157 stdcall SQLDataSourcesW(long long wstr long ptr wstr long ptr)
160 stdcall SQLForeignKeysW(long wstr long wstr long wstr long wstr long wstr long wstr long)
162 stdcall SQLNativeSqlW(long wstr long wstr long ptr)
165 stdcall SQLPrimaryKeysW(long wstr long wstr long wstr long)
166 stdcall SQLProcedureColumnsW(long wstr long wstr long wstr long wstr long)
167 stdcall SQLProceduresW(long wstr long wstr long wstr long)
170 stdcall SQLTablePrivilegesW(long wstr long wstr long wstr long)
171 stdcall SQLDriversW(long long wstr long ptr wstr long ptr)
173 stdcall SQLSetDescFieldW(long long long ptr long)
176 stdcall SQLSetStmtAttrW(long long ptr long)
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
236 stub    SQLGetDiagRecA
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
257 stub    SQLDataSourcesA
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
