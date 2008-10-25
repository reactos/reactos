#ifndef _SQLTYPES_H
#define _SQLTYPES_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif
#define SQL_API __stdcall
#ifndef RC_INVOKED
#define __need_wchar_t
#include <stddef.h>
typedef signed char SCHAR;
typedef long SDWORD;
typedef short SWORD;
typedef ULONG UDWORD;
typedef USHORT UWORD;
typedef signed long SLONG;
typedef signed short SSHORT;
typedef double SDOUBLE;
typedef double LDOUBLE;
typedef float SFLOAT;
typedef PVOID PTR;
typedef PVOID HENV;
typedef PVOID HDBC;
typedef PVOID HSTMT;
typedef short RETCODE;
typedef UCHAR SQLCHAR;
typedef SCHAR SQLSCHAR;
typedef SDWORD SQLINTEGER;
typedef SWORD SQLSMALLINT;
#ifndef __WIN64
typedef UDWORD SQLUINTEGER;
#endif
typedef UWORD SQLUSMALLINT;
typedef PVOID SQLPOINTER;
#if (ODBCVER >= 0x0300)
typedef void* SQLHANDLE;
typedef SQLHANDLE SQLHENV;
typedef SQLHANDLE SQLHDBC;
typedef SQLHANDLE SQLHSTMT;
typedef SQLHANDLE SQLHDESC;
#else
typedef void* SQLHENV;
typedef void* SQLHDBC;
typedef void* SQLHSTMT;
#endif
typedef SQLSMALLINT SQLRETURN;
typedef HWND SQLHWND;
typedef ULONG BOOKMARK;
#ifdef _WIN64
typedef INT64 SQLLEN;
typedef INT64 SQLROWOFFSET;
typedef UINT64 SQLROWCOUNT;
typedef UINT64 SQLULEN;
typedef UINT64 SQLTRANSID;
typedef unsigned long SQLSETPOSIROW;
#else
#define SQLLEN SQLINTEGER
#define SQLROWOFFSET SQLINTEGER
#define SQLROWCOUNT SQLUINTEGER
#define SQLULEN SQLUINTEGER
#define SQLTRANSID DWORD
#define SQLSETPOSIROW SQLUSMALLINT
#endif
typedef wchar_t SQLWCHAR;
#ifdef UNICODE
typedef SQLWCHAR        SQLTCHAR;
#else
typedef SQLCHAR         SQLTCHAR;
#endif  /* UNICODE */
#if (ODBCVER >= 0x0300)
typedef unsigned char   SQLDATE;
typedef unsigned char   SQLDECIMAL;
typedef double          SQLDOUBLE;
typedef double          SQLFLOAT;
typedef unsigned char   SQLNUMERIC;
typedef float           SQLREAL;
typedef unsigned char   SQLTIME;
typedef unsigned char   SQLTIMESTAMP;
typedef unsigned char   SQLVARCHAR;
#define ODBCINT64	__int64
typedef __int64 SQLBIGINT;
typedef unsigned __int64 SQLUBIGINT;
#endif

typedef struct tagDATE_STRUCT {
	SQLSMALLINT year;
	SQLUSMALLINT month;
	SQLUSMALLINT day;
} DATE_STRUCT;
typedef struct tagTIME_STRUCT {
	SQLUSMALLINT hour;
	SQLUSMALLINT minute;
	SQLUSMALLINT second;
} TIME_STRUCT;
typedef struct tagTIMESTAMP_STRUCT {
	SQLSMALLINT year;
	SQLUSMALLINT month;
	SQLUSMALLINT day;
	SQLUSMALLINT hour;
	SQLUSMALLINT minute;
	SQLUSMALLINT second;
	SQLUINTEGER fraction;
} TIMESTAMP_STRUCT;
#if (ODBCVER >= 0x0300)
typedef DATE_STRUCT	SQL_DATE_STRUCT;
typedef TIME_STRUCT	SQL_TIME_STRUCT;
typedef TIMESTAMP_STRUCT SQL_TIMESTAMP_STRUCT;
typedef enum {
	SQL_IS_YEAR = 1,SQL_IS_MONTH,SQL_IS_DAY,SQL_IS_HOUR,
	SQL_IS_MINUTE,SQL_IS_SECOND,SQL_IS_YEAR_TO_MONTH,SQL_IS_DAY_TO_HOUR,
	SQL_IS_DAY_TO_MINUTE,SQL_IS_DAY_TO_SECOND,SQL_IS_HOUR_TO_MINUTE,
	SQL_IS_HOUR_TO_SECOND,SQL_IS_MINUTE_TO_SECOND
} SQLINTERVAL;
typedef struct tagSQL_YEAR_MONTH {
	SQLUINTEGER year;
	SQLUINTEGER month;
} SQL_YEAR_MONTH_STRUCT;
typedef struct tagSQL_DAY_SECOND {
	SQLUINTEGER day;
	SQLUINTEGER	hour;
	SQLUINTEGER minute;
	SQLUINTEGER second;
	SQLUINTEGER fraction;
} SQL_DAY_SECOND_STRUCT;
typedef struct tagSQL_INTERVAL_STRUCT {
	SQLINTERVAL interval_type;
	SQLSMALLINT interval_sign;
	union {
		SQL_YEAR_MONTH_STRUCT year_month;
		SQL_DAY_SECOND_STRUCT day_second;
	} intval;
} SQL_INTERVAL_STRUCT;
#define SQL_MAX_NUMERIC_LEN 16
typedef struct tagSQL_NUMERIC_STRUCT {
	SQLCHAR precision;
	SQLSCHAR scale;
	SQLCHAR sign;
	SQLCHAR val[SQL_MAX_NUMERIC_LEN];
} SQL_NUMERIC_STRUCT;
#endif  /* ODBCVER >= 0x0300 */
#if (ODBCVER >= 0x0350)

#ifdef _GUID_DEFINED
# warning _GUID_DEFINED is deprecated, use GUID_DEFINED instead
#endif

#if defined _GUID_DEFINED || defined GUID_DEFINED
typedef GUID SQLGUID;
#else
typedef struct tagSQLGUID{
    DWORD Data1;
    WORD Data2;
    WORD Data3;
    BYTE Data4[ 8 ];
} SQLGUID;
#endif  /* GUID_DEFINED */
#endif  /* ODBCVER >= 0x0350 */
#endif     /* RC_INVOKED */
#ifdef __cplusplus
}
#endif
#endif
