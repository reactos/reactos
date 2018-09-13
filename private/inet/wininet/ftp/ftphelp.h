/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ftphelp.h

Abstract:

    header for private FTP manifests etc

Author:

    Heath Hunnicut (t-heathh) 26-May-1995

Revision History:

    26-May-1995 t-heathh
        Created

--*/

typedef struct {
    int Major;
    int Minor;
    int Detail;
    int Status;
} FTP_RESPONSE_CODE;

//
// response categories (1st digit)
//

#define FTP_RESPONSE_PRELIMINARY        1
#define FTP_RESPONSE_COMPLETE           2
#define FTP_RESPONSE_CONTINUE           3
#define FTP_RESPONSE_TRANSIENT_FAILURE  4
#define FTP_RESPONSE_PERMANENT_FAILURE  5

//
// response codes
//

#define FTP_RESPONSE_RESTART_MARKER     110
#define FTP_RESPONSE_DATA_ALREADY_OPEN  125
#define FTP_RESPONSE_OPENING_DATA       150
#define FTP_RESPONSE_CMD_OK             200
#define FTP_RESPONSE_CMD_EXTRANEOUS     202
#define FTP_RESPONSE_DIRECTORY_STATUS   212
#define FTP_RESPONSE_FILE_STATUS        213
#define FTP_RESPONSE_SYSTEM_TYPE        215
#define FTP_RESPONSE_SEND_USER_CMD      220
#define FTP_RESPONSE_CLOSING_CONTROL    221
#define FTP_RESPONSE_CLOSING_DATA       226
#define FTP_RESPONSE_ENTERING_PASSIVE   227
#define FTP_RESPONSE_LOGGED_IN_PROCEED  230
#define FTP_RESPONSE_FILE_ACTION_OK     250
#define FTP_RESPONSE_PATHNAME_CREATED   257
#define FTP_RESPONSE_SEND_PASS_CMD      331
#define FTP_RESPONSE_NEED_LOGIN_ACCOUNT 332
#define FTP_RESPONSE_FILE_CMD_PENDING   350
#define FTP_RESPONSE_CANT_OPEN_DATA     425
#define FTP_RESPONSE_CMD_SYNTAX_ERROR   500
#define FTP_RESPONSE_ARG_SYNTAX_ERROR   501
#define FTP_RESPONSE_CMD_NOT_IMPL       502
#define FTP_RESPONSE_BAD_CMD_SEQ        503
#define FTP_RESPONSE_NOT_LOGGED_IN      530
#define FTP_RESPONSE_ACTION_NOT_TAKEN   550
