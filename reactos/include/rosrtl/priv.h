/* $Id$
 */

#ifndef ROSRTL_SEC_H__
#define ROSRTL_SEC_H__

#ifdef __cplusplus
extern "C"
{
#endif

BOOL
RosEnableThreadPrivileges(HANDLE *hToken, LUID *Privileges, DWORD PrivilegeCount);
BOOL
RosResetThreadPrivileges(HANDLE hToken);

#ifdef __cplusplus
}
#endif

#endif

/* EOF */
