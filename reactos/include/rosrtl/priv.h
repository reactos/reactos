/* $Id: priv.h,v 1.1 2004/08/11 08:28:13 weiden Exp $
 */

#ifndef ROSRTL_SEC_H__
#define ROSRTL_SEC_H__

#ifdef __cplusplus
extern "C"
{
#endif

BOOL
RosEnableThreadPrivileges(HANDLE *hToken, DWORD *Privileges, DWORD PrivilegeCount);
BOOL
RosResetThreadPrivileges(HANDLE hToken);

#ifdef __cplusplus
}
#endif

#endif

/* EOF */
