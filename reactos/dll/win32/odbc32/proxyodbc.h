/*
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
 */

#ifndef _PROXYMANAGER_H
#define _PROXYMANAGER_H

#define NUM_SQLFUNC 78
#define ERROR_FREE 0
#define ERROR_SQLERROR  1
#define ERROR_LIBRARY_NOT_FOUND 2
#define ERROR_FUNCTIONS_NOT_LOADED  3

#define ODBCVER 0x0351

typedef struct dm_func                          /* driver manager functions */
{
    int            ordinal;
    const char     *name;
    void           *d_func;
    SQLRETURN      (*func)();
    SQLRETURN      (*funcW)();
} DM_FUNC;

typedef struct proxyhandle
{
	void *dmHandle;				/* driver manager handle */
	BOOL isWindowsDriver;       /* is driver a Windows version*/
	BOOL bCallbackReady;        /* procs taken from Windows driver placed in driver manager as callbacks */
	BOOL bFunctionReady;
	int  nErrorType;
	DM_FUNC functions[NUM_SQLFUNC];			/* entry point for driver manager functions */
	char driverLibName[200];	/* ODBC driver SO name */
	char ServerName[200];       /* keep server name */
	char UserName[50];          /* keep username */
} PROXYHANDLE;

#define    /* 00 */ SQLAPI_INDEX_SQLALLOCCONNECT       0
#define    /* 01 */ SQLAPI_INDEX_SQLALLOCENV           1
#define    /* 02 */ SQLAPI_INDEX_SQLALLOCHANDLE        2
#define    /* 03 */ SQLAPI_INDEX_SQLALLOCSTMT          3
#define    /* 04 */ SQLAPI_INDEX_SQLALLOCHANDLESTD     4
#define    /* 05 */ SQLAPI_INDEX_SQLBINDCOL            5
#define    /* 06 */ SQLAPI_INDEX_SQLBINDPARAM          6
#define    /* 07 */ SQLAPI_INDEX_SQLBINDPARAMETER      7
#define    /* 08 */ SQLAPI_INDEX_SQLBROWSECONNECT      8
#define    /* 09 */ SQLAPI_INDEX_SQLBULKOPERATIONS     9
#define    /* 10 */ SQLAPI_INDEX_SQLCANCEL            10
#define    /* 11 */ SQLAPI_INDEX_SQLCLOSECURSOR       11
#define    /* 12 */ SQLAPI_INDEX_SQLCOLATTRIBUTE      12
#define    /* 13 */ SQLAPI_INDEX_SQLCOLATTRIBUTES     13
#define    /* 14 */ SQLAPI_INDEX_SQLCOLUMNPRIVILEGES  14
#define    /* 15 */ SQLAPI_INDEX_SQLCOLUMNS           15
#define    /* 16 */ SQLAPI_INDEX_SQLCONNECT           16
#define    /* 17 */ SQLAPI_INDEX_SQLCOPYDESC          17
#define    /* 18 */ SQLAPI_INDEX_SQLDATASOURCES       18
#define    /* 19 */ SQLAPI_INDEX_SQLDESCRIBECOL       19
#define    /* 20 */ SQLAPI_INDEX_SQLDESCRIBEPARAM     20
#define    /* 21 */ SQLAPI_INDEX_SQLDISCONNECT        21
#define    /* 22 */ SQLAPI_INDEX_SQLDRIVERCONNECT     22
#define    /* 23 */ SQLAPI_INDEX_SQLDRIVERS           23
#define    /* 24 */ SQLAPI_INDEX_SQLENDTRAN           24
#define    /* 25 */ SQLAPI_INDEX_SQLERROR             25
#define    /* 26 */ SQLAPI_INDEX_SQLEXECDIRECT        26
#define    /* 27 */ SQLAPI_INDEX_SQLEXECUTE           27
#define    /* 28 */ SQLAPI_INDEX_SQLEXTENDEDFETCH     28
#define    /* 29 */ SQLAPI_INDEX_SQLFETCH             29
#define    /* 30 */ SQLAPI_INDEX_SQLFETCHSCROLL       30
#define    /* 31 */ SQLAPI_INDEX_SQLFOREIGNKEYS       31
#define    /* 32 */ SQLAPI_INDEX_SQLFREEENV           32
#define    /* 33 */ SQLAPI_INDEX_SQLFREEHANDLE        33
#define    /* 34 */ SQLAPI_INDEX_SQLFREESTMT          34
#define    /* 35 */ SQLAPI_INDEX_SQLFREECONNECT       35
#define    /* 36 */ SQLAPI_INDEX_SQLGETCONNECTATTR    36
#define    /* 37 */ SQLAPI_INDEX_SQLGETCONNECTOPTION  37
#define    /* 38 */ SQLAPI_INDEX_SQLGETCURSORNAME     38
#define    /* 39 */ SQLAPI_INDEX_SQLGETDATA           39
#define    /* 40 */ SQLAPI_INDEX_SQLGETDESCFIELD      40
#define    /* 41 */ SQLAPI_INDEX_SQLGETDESCREC        41
#define    /* 42 */ SQLAPI_INDEX_SQLGETDIAGFIELD      42
#define    /* 43 */ SQLAPI_INDEX_SQLGETENVATTR        43
#define    /* 44 */ SQLAPI_INDEX_SQLGETFUNCTIONS      44
#define    /* 45 */ SQLAPI_INDEX_SQLGETINFO           45
#define    /* 46 */ SQLAPI_INDEX_SQLGETSTMTATTR       46
#define    /* 47 */ SQLAPI_INDEX_SQLGETSTMTOPTION     47
#define    /* 48 */ SQLAPI_INDEX_SQLGETTYPEINFO       48
#define    /* 49 */ SQLAPI_INDEX_SQLMORERESULTS       49
#define    /* 50 */ SQLAPI_INDEX_SQLNATIVESQL         50
#define    /* 51 */ SQLAPI_INDEX_SQLNUMPARAMS         51
#define    /* 52 */ SQLAPI_INDEX_SQLNUMRESULTCOLS     52
#define    /* 53 */ SQLAPI_INDEX_SQLPARAMDATA         53
#define    /* 54 */ SQLAPI_INDEX_SQLPARAMOPTIONS      54
#define    /* 55 */ SQLAPI_INDEX_SQLPREPARE           55
#define    /* 56 */ SQLAPI_INDEX_SQLPRIMARYKEYS       56
#define    /* 57 */ SQLAPI_INDEX_SQLPROCEDURECOLUMNS  57
#define    /* 58 */ SQLAPI_INDEX_SQLPROCEDURES        58
#define    /* 59 */ SQLAPI_INDEX_SQLPUTDATA           59
#define    /* 60 */ SQLAPI_INDEX_SQLROWCOUNT          60
#define    /* 61 */ SQLAPI_INDEX_SQLSETCONNECTATTR    61
#define    /* 62 */ SQLAPI_INDEX_SQLSETCONNECTOPTION  62
#define    /* 63 */ SQLAPI_INDEX_SQLSETCURSORNAME     63
#define    /* 64 */ SQLAPI_INDEX_SQLSETDESCFIELD      64
#define    /* 65 */ SQLAPI_INDEX_SQLSETDESCREC        65
#define    /* 66 */ SQLAPI_INDEX_SQLSETENVATTR        66
#define    /* 67 */ SQLAPI_INDEX_SQLSETPARAM          67
#define    /* 68 */ SQLAPI_INDEX_SQLSETPOS            68
#define    /* 69 */ SQLAPI_INDEX_SQLSETSCROLLOPTIONS  69
#define    /* 70 */ SQLAPI_INDEX_SQLSETSTMTATTR       70
#define    /* 71 */ SQLAPI_INDEX_SQLSETSTMTOPTION     71
#define    /* 72 */ SQLAPI_INDEX_SQLSPECIALCOLUMNS    72
#define    /* 73 */ SQLAPI_INDEX_SQLSTATISTICS        73
#define    /* 74 */ SQLAPI_INDEX_SQLTABLEPRIVILEGES   74
#define    /* 75 */ SQLAPI_INDEX_SQLTABLES            75
#define    /* 76 */ SQLAPI_INDEX_SQLTRANSACT          76
#define    /* 77 */ SQLAPI_INDEX_SQLGETDIAGREC        77

#endif
