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
106 stub    SQLColAttributesW
107 stub    SQLConnectW
108 stub    SQLDescribeColW
110 stub    SQLErrorW
111 stub    SQLExecDirectW
117 stub    SQLGetCursorNameW
119 stub    SQLPrepareW
121 stub    SQLSetCursorNameW
127 stub    SQLColAttributeW
132 stub    SQLGetConnectAttrW
133 stub    SQLGetDescFieldW
134 stub    SQLGetDescRecW
135 stub    SQLGetDiagFieldW
136 stub    SQLGetDiagRecW
138 stub    SQLGetStmtAttrW
139 stub    SQLSetConnectAttrW
140 stub    SQLColumnsW
141 stub    SQLDriverConnectW
142 stub    SQLGetConnectOptionW
145 stub    SQLGetInfoW
147 stub    SQLGetTypeInfoW
150 stub    SQLSetConnectOptionW
152 stub    SQLSpecialColumnsW
153 stub    SQLStatisticsW
154 stub    SQLTablesW
155 stub    SQLBrowseConnectW
156 stub    SQLColumnPrivilegesW
157 stub    SQLDataSourcesW
160 stub    SQLForeignKeysW
162 stub    SQLNativeSqlW
165 stub    SQLPrimaryKeysW
166 stub    SQLProcedureColumnsW
167 stub    SQLProceduresW
170 stub    SQLTablePrivilegesW
171 stub    SQLDriversW
173 stub    SQLSetDescFieldW
176 stub    SQLSetStmtAttrW
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
