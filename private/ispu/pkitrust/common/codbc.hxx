//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       codbc.hxx
//
//  Contents:   Microsoft Internet Security Common
//
//  History:    08-Sep-1997 pberkman   created
//
//--------------------------------------------------------------------------

#ifndef CODBC_HXX
#define CODBC_HXX

#include    <sql.h>
#include    <sqlext.h>

#undef SQL_SUCCEEDED
//#define SQL_SUCCEEDED(err)              ((err == SQL_SUCCESS) || (err == SQL_SUCCESS_WITH_INFO))

typedef SQLRETURN                       (SQL_API *td_SQLSetStmtOption)(
                                                                    SQLHSTMT        hstmt,
                                                                    SQLUSMALLINT    fOption,
                                                                    SQLUINTEGER     vParam);
typedef SQLRETURN                       (SQL_API *td_SQLExecDirect)(
                                                                    SQLHSTMT        hstmt,
                                                                    SQLCHAR FAR     *szSqlStr,
                                                                    SQLINTEGER      cbSqlStr);
typedef SQLRETURN                       (SQL_API *td_SQLFetch)(
                                                                    SQLHSTMT        hstmt);
typedef SQLRETURN                       (SQL_API *td_SQLFreeConnect)(
                                                                    SQLHDBC         hdbc);
typedef SQLRETURN                       (SQL_API *td_SQLFreeEnv)(
                                                                    SQLHENV         henv);
typedef SQLRETURN                       (SQL_API *td_SQLFreeStmt)(
                                                                    SQLHSTMT        hstmt,
                                                                    SQLUSMALLINT    fOption);
typedef SQLRETURN                       (SQL_API *td_SQLTransact)(
                                                                    SQLHENV         henv,
                                                                    SQLHDBC         hdbc,
                                                                    SQLUSMALLINT    fType);
typedef SQLRETURN                       (SQL_API *td_SQLError)(
                                                                    SQLHENV         henv,
                                                                    SQLHDBC         hdbc,
                                                                    SQLHSTMT        hstmt,
                                                                    SQLCHAR FAR     *szSqlState,
                                                                    SQLINTEGER FAR  *pfNativeError,
                                                                    SQLCHAR FAR     *szErrorMsg,
                                                                    SQLSMALLINT     cbErrorMsgMax,
                                                                    SQLSMALLINT FAR *pcbErrorMsg);
typedef SQLRETURN                       (SQL_API *td_SQLGetData)(
                                                                    SQLHSTMT        hstmt,
                                                                    SQLUSMALLINT    icol,
                                                                    SQLSMALLINT     fCType,
                                                                    SQLPOINTER      rgbValue,
                                                                    SQLINTEGER      cbValueMax,
                                                                    SQLINTEGER FAR  *pcbValue);
typedef SQLRETURN                       (SQL_API *td_SQLAllocConnect)(
                                                                    SQLHENV         henv,
                                                                    SQLHDBC FAR     *phdbc);
typedef SQLRETURN                       (SQL_API *td_SQLAllocEnv)(
                                                                    SQLHENV FAR     *phenv);
typedef SQLRETURN                       (SQL_API *td_SQLAllocStmt)(
                                                                    SQLHDBC         hdbc,
                                                                    SQLHSTMT FAR    *phstmt);
typedef SQLRETURN                       (SQL_API *td_SQLDriverConnect)(
                                                                    SQLHDBC         hdbc,
                                                                    SQLHWND         hwnd,
                                                                    SQLCHAR FAR     *szConnStrIn,
                                                                    SQLSMALLINT     cbConnStrIn,
                                                                    SQLCHAR FAR     *szConnStrOut,
                                                                    SQLSMALLINT     cbConnStrOutMax,
                                                                    SQLSMALLINT FAR *pcbConnStrOut,
                                                                    SQLUSMALLINT    fDriverCompletion);
typedef SQLRETURN                       (SQL_API *td_SQLDisconnect)(
                                                                    SQLHDBC         hdbc);


class cODBC_
{
    public:
        cODBC_(void);
        virtual ~cODBC_(void);

        BOOL                        Initialize(void);

        SQLRETURN                   SQLExecDirect(SQLHSTMT hstmt, SQLCHAR FAR *szSqlStr, SQLINTEGER cbSqlStr)
                                                { return((*fp_SQLExecDirect)(hstmt, szSqlStr, cbSqlStr)); }
        SQLRETURN                   SQLSetStmtOption(SQLHSTMT hstmt, SQLUSMALLINT fOption, SQLUINTEGER vParam)
                                                { return((*fp_SQLSetStmtOption)(hstmt, fOption, vParam)); }
        SQLRETURN                   SQLFetch(SQLHSTMT hstmt)
                                                { return((*fp_SQLFetch)(hstmt)); }
        SQLRETURN                   SQLFreeConnect(SQLHDBC hdbc)
                                                { return((*fp_SQLFreeConnect)(hdbc)); }
        SQLRETURN                   SQLFreeEnv(SQLHENV henv)
                                                { return((*fp_SQLFreeEnv)(henv)); }
        SQLRETURN                   SQLFreeStmt(SQLHSTMT hstmt, SQLUSMALLINT fOption)
                                                { return((*fp_SQLFreeStmt)(hstmt, fOption)); }
        SQLRETURN                   SQLTransact(SQLHENV henv, SQLHDBC hdbc, SQLUSMALLINT fType)
                                                { return((*fp_SQLTransact)(henv, hdbc, fType)); }
        SQLRETURN                   SQLError(SQLHENV henv, SQLHDBC hdbc, SQLHSTMT hstmt, SQLCHAR FAR *szSqlState,
                                             SQLINTEGER FAR *pfNativeError, SQLCHAR FAR *szErrorMsg,
                                             SQLSMALLINT cbErrorMsgMax, SQLSMALLINT FAR *pcbErrorMsg)
                                                { return((*fp_SQLError)(henv, hdbc, hstmt, szSqlState,
                                                                        pfNativeError, szErrorMsg,
                                                                        cbErrorMsgMax, pcbErrorMsg)); }
        SQLRETURN                   SQLGetData(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLSMALLINT fCType,
                                               SQLPOINTER rgbValue, SQLINTEGER cbValueMax, SQLINTEGER FAR *pcbValue)
                                                { return((*fp_SQLGetData)(hstmt, icol, fCType,
                                                                          rgbValue, cbValueMax, pcbValue)); }
        SQLRETURN                   SQLAllocConnect(SQLHENV henv, SQLHDBC FAR *phdbc)
                                                { return((*fp_SQLAllocConnect)(henv, phdbc)); }
        SQLRETURN                   SQLAllocEnv(SQLHENV FAR *phenv)
                                                { return((*fp_SQLAllocEnv)(phenv)); }
        SQLRETURN                   SQLAllocStmt(SQLHDBC hdbc, SQLHSTMT FAR *phstmt)
                                                { return((*fp_SQLAllocStmt)(hdbc, phstmt)); }
        SQLRETURN                   SQLDriverConnect(SQLHDBC hdbc, SQLHWND hwnd, SQLCHAR FAR *szConnStrIn,
                                                     SQLSMALLINT cbConnStrIn, SQLCHAR FAR *szConnStrOut,
                                                     SQLSMALLINT cbConnStrOutMax, SQLSMALLINT FAR *pcbConnStrOut,
                                                     SQLUSMALLINT fDriverCompletion)
                                                { return((*fp_SQLDriverConnect)(hdbc, hwnd, szConnStrIn,
                                                                                cbConnStrIn, szConnStrOut,
                                                                                cbConnStrOutMax, pcbConnStrOut,
                                                                                fDriverCompletion)); }
        SQLRETURN                   SQLDisconnect(SQLHDBC hdbc)
                                                { return((*fp_SQLDisconnect)(hdbc)); }


    private:
        HINSTANCE                   hODBC32;

        td_SQLSetStmtOption         fp_SQLSetStmtOption;
        td_SQLExecDirect            fp_SQLExecDirect;
        td_SQLFetch                 fp_SQLFetch;
        td_SQLFreeConnect           fp_SQLFreeConnect;
        td_SQLFreeEnv               fp_SQLFreeEnv;
        td_SQLFreeStmt              fp_SQLFreeStmt;
        td_SQLTransact              fp_SQLTransact;
        td_SQLError                 fp_SQLError;
        td_SQLGetData               fp_SQLGetData;
        td_SQLAllocConnect          fp_SQLAllocConnect;
        td_SQLAllocEnv              fp_SQLAllocEnv;
        td_SQLAllocStmt             fp_SQLAllocStmt;
        td_SQLDriverConnect         fp_SQLDriverConnect;
        td_SQLDisconnect            fp_SQLDisconnect;
};

#endif // CODBC_HXX

