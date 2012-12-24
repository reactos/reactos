
#ifndef __WINE_NETAPI32_H__
#define __WINE_NETAPI32_H__

NET_API_STATUS
WINAPI
NetpNtStatusToApiStatus(NTSTATUS Status);

/* misc.c */

NTSTATUS
GetAccountDomainSid(IN PUNICODE_STRING ServerName,
                    OUT PSID *AccountDomainSid);

NTSTATUS
GetBuiltinDomainSid(OUT PSID *BuiltinDomainSid);

#endif