/*
 * Performance Data Helper
 *
 * Copyright 2007 Hans Leidekker
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

#ifndef _PDH_MSG_H_
#define _PDH_MSG_H_

#define PDH_CSTATUS_VALID_DATA          0x00000000
#define PDH_CSTATUS_NEW_DATA            0x00000001
#define PDH_CSTATUS_NO_MACHINE          0x800007d0
#define PDH_CSTATUS_NO_INSTANCE         0x800007d1
#define PDH_MORE_DATA                   0x800007d2
#define PDH_CSTATUS_ITEM_NOT_VALIDATED  0x800007d3
#define PDH_RETRY                       0x800007d4
#define PDH_NO_DATA                     0x800007d5
#define PDH_CALC_NEGATIVE_DENOMINATOR   0x800007d6
#define PDH_CALC_NEGATIVE_TIMEBASE      0x800007d7
#define PDH_CALC_NEGATIVE_VALUE         0x800007d8
#define PDH_DIALOG_CANCELLED            0x800007d9
#define PDH_END_OF_LOG_FILE             0x800007da
#define PDH_ASYNC_QUERY_TIMEOUT         0x800007db
#define PDH_CANNOT_SET_DEFAULT_REALTIME_DATASOURCE 0x800007dc
#define PDH_UNABLE_MAP_NAME_FILES       0x80000bd5
#define PDH_PLA_VALIDATION_WARNING      0x80000bf3
#define PDH_CSTATUS_NO_OBJECT           0xc0000bb8
#define PDH_CSTATUS_NO_COUNTER          0xc0000bb9
#define PDH_CSTATUS_INVALID_DATA        0xc0000bba
#define PDH_MEMORY_ALLOCATION_FAILURE   0xc0000bbb
#define PDH_INVALID_HANDLE              0xc0000bbc
#define PDH_INVALID_ARGUMENT            0xc0000bbd
#define PDH_FUNCTION_NOT_FOUND          0xc0000bbe
#define PDH_CSTATUS_NO_COUNTERNAME      0xc0000bbf
#define PDH_CSTATUS_BAD_COUNTERNAME     0xc0000bc0
#define PDH_INVALID_BUFFER              0xc0000bc1
#define PDH_INSUFFICIENT_BUFFER         0xc0000bc2
#define PDH_CANNOT_CONNECT_MACHINE      0xc0000bc3
#define PDH_INVALID_PATH                0xc0000bc4
#define PDH_INVALID_INSTANCE            0xc0000bc5
#define PDH_INVALID_DATA                0xc0000bc6
#define PDH_NO_DIALOG_DATA              0xc0000bc7
#define PDH_CANNOT_READ_NAME_STRINGS    0xc0000bc8
#define PDH_LOG_FILE_CREATE_ERROR       0xc0000bc9
#define PDH_LOG_FILE_OPEN_ERROR         0xc0000bca
#define PDH_LOG_TYPE_NOT_FOUND          0xc0000bcb
#define PDH_NO_MORE_DATA                0xc0000bcc
#define PDH_ENTRY_NOT_IN_LOG_FILE       0xc0000bcd
#define PDH_DATA_SOURCE_IS_LOG_FILE     0xc0000bce
#define PDH_DATA_SOURCE_IS_REAL_TIME    0xc0000bcf
#define PDH_UNABLE_READ_LOG_HEADER      0xc0000bd0
#define PDH_FILE_NOT_FOUND              0xc0000bd1
#define PDH_FILE_ALREADY_EXISTS         0xc0000bd2
#define PDH_NOT_IMPLEMENTED             0xc0000bd3
#define PDH_STRING_NOT_FOUND            0xc0000bd4
#define PDH_UNKNOWN_LOG_FORMAT          0xc0000bd6
#define PDH_UNKNOWN_LOGSVC_COMMAND      0xc0000bd7
#define PDH_LOGSVC_QUERY_NOT_FOUND      0xc0000bd8
#define PDH_LOGSVC_NOT_OPENED           0xc0000bd9
#define PDH_WBEM_ERROR                  0xc0000bda
#define PDH_ACCESS_DENIED               0xc0000bdb
#define PDH_LOG_FILE_TOO_SMALL          0xc0000bdc
#define PDH_INVALID_DATASOURCE          0xc0000bdd
#define PDH_INVALID_SQLDB               0xc0000bde
#define PDH_NO_COUNTERS                 0xc0000bdf
#define PDH_SQL_ALLOC_FAILED            0xc0000be0
#define PDH_SQL_ALLOCCON_FAILED         0xc0000be1
#define PDH_SQL_EXEC_DIRECT_FAILED      0xc0000be2
#define PDH_SQL_FETCH_FAILED            0xc0000be3
#define PDH_SQL_ROWCOUNT_FAILED         0xc0000be4
#define PDH_SQL_MORE_RESULTS_FAILED     0xc0000be5
#define PDH_SQL_CONNECT_FAILED          0xc0000be6
#define PDH_SQL_BIND_FAILED             0xc0000be7
#define PDH_CANNOT_CONNECT_WMI_SERVER   0xc0000be8
#define PDH_PLA_COLLECTION_ALREADY_RUNNING 0xc0000be9
#define PDH_PLA_ERROR_SCHEDULE_OVERLAP  0xc0000bea
#define PDH_PLA_COLLECTION_NOT_FOUND    0xc0000beb
#define PDH_PLA_ERROR_SCHEDULE_ELAPSED  0xc0000bec
#define PDH_PLA_ERROR_NOSTART           0xc0000bed
#define PDH_PLA_ERROR_ALREADY_EXISTS    0xc0000bee
#define PDH_PLA_ERROR_TYPE_MISMATCH     0xc0000bef
#define PDH_PLA_ERROR_FILEPATH          0xc0000bf0
#define PDH_PLA_SERVICE_ERROR           0xc0000bf1
#define PDH_PLA_VALIDATION_ERROR        0xc0000bf2
#define PDH_PLA_ERROR_NAME_TOO_LONG     0xc0000bf4
#define PDH_INVALID_SQL_LOG_FORMAT      0xc0000bf5
#define PDH_COUNTER_ALREADY_IN_QUERY    0xc0000bf6
#define PDH_BINARY_LOG_CORRUPT          0xc0000bf7
#define PDH_LOG_SAMPLE_TOO_SMALL        0xc0000bf8
#define PDH_OS_LATER_VERSION            0xc0000bf9
#define PDH_OS_EARLIER_VERSION          0xc0000bfa
#define PDH_INCORRECT_APPEND_TIME       0xc0000bfb
#define PDH_UNMATCHED_APPEND_COUNTER    0xc0000bfc
#define PDH_SQL_ALTER_DETAIL_FAILED     0xc0000bfd
#define PDH_QUERY_PERF_DATA_TIMEOUT     0xc0000bfe

#endif /* _PDH_MSG_H_ */
