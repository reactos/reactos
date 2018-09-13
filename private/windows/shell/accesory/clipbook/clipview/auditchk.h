
/******************************************************************************

                        A U D I T   C H E C K

    Name:       auditchk.h
    Date:       1/20/94
    Creator:    John Fu

    Description:
        This is the header file for auditchk.c

******************************************************************************/




#define AUDIT_PRIVILEGE_CHECK 0
#define AUDIT_PRIVILEGE_ON    1
#define AUDIT_PRIVILEGE_OFF   2



BOOL AuditPrivilege(
    int fAudit);
