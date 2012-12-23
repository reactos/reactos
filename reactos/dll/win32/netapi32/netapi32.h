
#ifndef __WINE_NETAPI32_H__
#define __WINE_NETAPI32_H__

NET_API_STATUS
WINAPI
NetpNtStatusToApiStatus(NTSTATUS Status);

/* misc.c */

NTSTATUS
GetAccountDomainSid(PSID *AccountDomainSid);

NTSTATUS
GetBuiltinDomainSid(PSID *BuiltinDomainSid);

#endif