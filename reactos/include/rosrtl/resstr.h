/* $Id$
 */

#ifndef ROSRTL_RESSTR_H__
#define ROSRTL_RESSTR_H__

#ifdef __cplusplus
extern "C"
{
#endif

int
RosLenOfStrResource(HINSTANCE hInst, UINT uID);
int
RosAllocAndLoadStringA(LPSTR *lpTarget, HINSTANCE hInst, UINT uID);
int
RosAllocAndLoadStringW(LPWSTR *lpTarget, HINSTANCE hInst, UINT uID);
DWORD
RosFormatStrA(LPSTR *lpTarget, LPSTR lpFormat, ...);
DWORD
RosFormatStrW(LPWSTR *lpTarget, LPWSTR lpFormat, ...);
DWORD
RosLoadAndFormatStrA(HINSTANCE hInst, UINT uID, LPSTR *lpTarget, ...);
DWORD
RosLoadAndFormatStrW(HINSTANCE hInst, UINT uID, LPWSTR *lpTarget, ...);

#ifdef UNICODE
# define RosFmtString RosFmtStringW
# define RosAllocAndLoadString RosAllocAndLoadStringW
# define RosLoadAndFormatStr RosLoadAndFormatStrW
#else
# define RosFmtString RosFmtStringA
# define RosAllocAndLoadString RosAllocAndLoadStringA
# define RosLoadAndFormatStr RosLoadAndFormatStrA
#endif

#ifdef __cplusplus
}
#endif

#endif

/* EOF */
