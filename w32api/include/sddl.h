#ifndef _SDDL_H
#define _SDDL_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

BOOL WINAPI ConvertSidToStringSidA(PSID Sid, LPSTR *StringSid);
BOOL WINAPI ConvertSidToStringSidW(PSID Sid, LPWSTR *StringSid);

#ifdef UNICODE
#define ConvertSidToStringSid ConvertSidToStringSidW
#else /* UNICODE */
#define ConvertSidToStringSid ConvertSidToStringSidA
#endif /* UNICODE */

#ifdef __cplusplus
}
#endif
#endif /* ! defined _SDDL_H */
