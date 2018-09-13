//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       codbc.cpp
//
//  Contents:   Microsoft Internet Security Common
//
//  History:    08-Sep-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"
#include    "codbc.hxx"

cODBC_::cODBC_(void)
{
    hODBC32 = NULL;
    
    fp_SQLSetStmtOption = NULL;
    fp_SQLExecDirect    = NULL;
    fp_SQLFetch         = NULL;
    fp_SQLFreeConnect   = NULL;
    fp_SQLFreeEnv       = NULL;
    fp_SQLFreeStmt      = NULL;
    fp_SQLTransact      = NULL;
    fp_SQLError         = NULL;
    fp_SQLGetData       = NULL;
    fp_SQLAllocConnect  = NULL;
    fp_SQLAllocEnv      = NULL;
    fp_SQLAllocStmt     = NULL;
    fp_SQLDriverConnect = NULL;
    fp_SQLDisconnect    = NULL;
}

cODBC_::~cODBC_(void)
{
    if (hODBC32)
    {
        FreeLibrary(hODBC32);
    }
}

BOOL cODBC_::Initialize(void)
{
    SetErrorMode(SEM_NOOPENFILEERRORBOX);

    if (!(hODBC32 = LoadLibrary("ODBC32.DLL")))
    {
        goto ErrorDLLLoad;
    }

    fp_SQLSetStmtOption     = (td_SQLSetStmtOption)GetProcAddress(hODBC32,  "SQLSetStmtOption");
    fp_SQLExecDirect        = (td_SQLExecDirect)GetProcAddress(hODBC32,     "SQLExecDirect");
    fp_SQLFetch             = (td_SQLFetch)GetProcAddress(hODBC32,          "SQLFetch");
    fp_SQLFreeConnect       = (td_SQLFreeConnect)GetProcAddress(hODBC32,    "SQLFreeConnect");
    fp_SQLFreeEnv           = (td_SQLFreeEnv)GetProcAddress(hODBC32,        "SQLFreeEnv");
    fp_SQLFreeStmt          = (td_SQLFreeStmt)GetProcAddress(hODBC32,       "SQLFreeStmt");
    fp_SQLTransact          = (td_SQLTransact)GetProcAddress(hODBC32,       "SQLTransact");
    fp_SQLError             = (td_SQLError)GetProcAddress(hODBC32,          "SQLError");
    fp_SQLGetData           = (td_SQLGetData)GetProcAddress(hODBC32,        "SQLGetData");
    fp_SQLAllocConnect      = (td_SQLAllocConnect)GetProcAddress(hODBC32,   "SQLAllocConnect");
    fp_SQLAllocEnv          = (td_SQLAllocEnv)GetProcAddress(hODBC32,       "SQLAllocEnv");
    fp_SQLAllocStmt         = (td_SQLAllocStmt)GetProcAddress(hODBC32,      "SQLAllocStmt");
    fp_SQLDriverConnect     = (td_SQLDriverConnect)GetProcAddress(hODBC32,  "SQLDriverConnect");
    fp_SQLDisconnect        = (td_SQLDisconnect)GetProcAddress(hODBC32,     "SQLDisconnect");

    if (!(fp_SQLSetStmtOption) ||
        !(fp_SQLExecDirect) ||
        !(fp_SQLFetch) ||
        !(fp_SQLFreeConnect) ||
        !(fp_SQLFreeEnv) ||
        !(fp_SQLFreeStmt) ||
        !(fp_SQLTransact) ||
        !(fp_SQLError) ||
        !(fp_SQLGetData) ||
        !(fp_SQLAllocConnect) ||
        !(fp_SQLAllocEnv) ||
        !(fp_SQLAllocStmt) ||
        !(fp_SQLDriverConnect) ||
        !(fp_SQLDisconnect))
    {
        goto ErrorProcAddress;
    }

    return(TRUE);

    ErrorReturn:
        return(FALSE);

    TRACE_ERROR_EX(DBG_SS, ErrorDLLLoad);
    TRACE_ERROR_EX(DBG_SS, ErrorProcAddress);
}
