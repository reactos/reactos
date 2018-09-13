//***************************************************************************
//
// DOSERR.H
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1992-1993
//  All rights reserved
//
// DOS int 21h error codes.
//
//**************************************************************************/

#ifndef DOSERR_INC

#define NO_ERROR                        0

#define ERROR_INVALID_FUNCTION          1
#define ERROR_FILE_NOT_FOUND            2
#define ERROR_PATH_NOT_FOUND            3
#define ERROR_TOO_MANY_OPEN_FILES       4
#define ERROR_ACCESS_DENIED             5
#define ERROR_INVALID_HANDLE            6
#define ERROR_ARENA_TRASHED             7
#define ERROR_NOT_ENOUGH_MEMORY         8
#define ERROR_INVALID_BLOCK             9
#define ERROR_BAD_ENVIRONMENT           10
#define ERROR_BAD_FORMAT                11
#define ERROR_INVALID_ACCESS            12
#define ERROR_INVALID_DATA              13
/* 14 is reserved                        */
#define ERROR_INVALID_DRIVE             15
#define ERROR_CURRENT_DIRECTORY         16
#define ERROR_NOT_SAME_DEVICE           17
#define ERROR_NO_MORE_FILES             18
#define ERROR_WRITE_PROTECT             19
#define ERROR_BAD_UNIT                  20
#define ERROR_NOT_READY                 21
#define ERROR_BAD_COMMAND               22
#define ERROR_CRC                       23
#define ERROR_BAD_LENGTH                24
#define ERROR_SEEK                      25
#define ERROR_NOT_DOS_DISK              26
#define ERROR_SECTOR_NOT_FOUND          27
#define ERROR_OUT_OF_PAPER              28
#define ERROR_WRITE_FAULT               29
#define ERROR_READ_FAULT                30
#define ERROR_GEN_FAILURE               31
#define ERROR_SHARING_VIOLATION         32
#define ERROR_LOCK_VIOLATION            33
#define ERROR_WRONG_DISK                34
#define ERROR_FCB_UNAVAILABLE           35
#define ERROR_SHARING_BUFFER_EXCEEDED   36
#define ERROR_NOT_SUPPORTED             50
#define ERROR_REM_NOT_LIST              51 /* Remote computer not listening   */
#define ERROR_DUP_NAME                  52 /* Duplicate name on network       */
#define ERROR_BAD_NETPATH               53 /* Network path not found          */
#define ERROR_NETWORK_BUSY              54 /* Network busy                    */
#define ERROR_DEV_NOT_EXIST             55 /* Network device no longer exists */
#define ERROR_TOO_MANY_CMDS             56 /* Net BIOS command limit exceeded */
#define ERROR_ADAP_HDW_ERR              57 /* Network adapter hardware error  */
#define ERROR_BAD_NET_RESP              58 /* Incorrect response from network */
#define ERROR_UNEXP_NET_ERR             59 /* Unexpected network error        */
#define ERROR_BAD_REM_ADAP              60 /* Incompatible remote adapter     */
#define ERROR_PRINTQ_FULL               61 /* Print queue full                */
#define ERROR_NO_SPOOL_SPACE            62 /* Not enough space for print file */
#define ERROR_PRINT_CANCELLED           63 /* Print file was cancelled        */
#define ERROR_NETNAME_DELETED           64 /* Network name was deleted        */
#define ERROR_NETWORK_ACCESS_DENIED     65 /* Access denied                   */
#define ERROR_BAD_DEV_TYPE              66 /* Network device type incorrect   */
#define ERROR_BAD_NET_NAME              67 /* Network name not found          */
#define ERROR_TOO_MANY_NAMES            68 /* Network name limit exceeded     */
#define ERROR_TOO_MANY_SESS             69 /* Net BIOS session limit exceeded */
#define ERROR_SHARING_PAUSED            70 /* Sharing temporarily paused      */
#define ERROR_REQ_NOT_ACCEP             71 /* Network request not accepted    */
#define ERROR_REDIR_PAUSED              72 /* Print|disk redirection is paused*/
#define ERROR_FILE_EXISTS               80
#define ERROR_DUP_FCB                   81
#define ERROR_CANNOT_MAKE               82
#define ERROR_FAIL_I24                  83
#define ERROR_OUT_OF_STRUCTURES         84
#define ERROR_ALREADY_ASSIGNED          85
#define ERROR_INVALID_PASSWORD          86
#define ERROR_INVALID_PARAMETER         87
#define ERROR_NET_WRITE_FAULT           88
#define ERROR_SYS_COMP_NOT_LOADED       90


#endif
