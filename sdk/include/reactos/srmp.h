#ifndef _SRMP_
#define _SRMP_

typedef enum _RM_API_NUMBER
{
    RmAuditSetCommand = 1,
    RmCreateLogonSession = 2,
    RmDeleteLogonSession = 3
} RM_API_NUMBER;

typedef struct _SEP_RM_API_MESSAGE
{
    PORT_MESSAGE Header;
    ULONG ApiNumber;
    union
    {
        UCHAR Fill[PORT_MAXIMUM_MESSAGE_LENGTH - sizeof(PORT_MESSAGE)];
        NTSTATUS ResultStatus;
        struct
        {
            BOOLEAN Enabled;
            ULONG Flags[9];
        } SetAuditEvent;
        LUID LogonLuid;
    } u;
} SEP_RM_API_MESSAGE, *PSEP_RM_API_MESSAGE;


typedef enum _LSAP_API_NUMBER
{
    LsapAdtWriteLogApi = 1,
    LsapComponentTestApi,
    LsapAsyncApi
} LSAP_API_NUMBER;

typedef struct _LSAP_RM_API_MESSAGE
{
    PORT_MESSAGE Header;
    ULONG ApiNumber;
    union
    {
        UCHAR Fill[PORT_MAXIMUM_MESSAGE_LENGTH - sizeof(PORT_MESSAGE)];
        struct
        {
            ULONG Info1;
        } WriteLog;

    } u;
} LSAP_RM_API_MESSAGE, *PLSAP_RM_API_MESSAGE;

#endif /* _SRMP_ */
