/* $Id: string.h,v 1.3 2004/08/10 10:57:54 weiden Exp $
 */

#ifndef ROSRTL_STRING_H__
#define ROSRTL_STRING_H__

#ifdef __cplusplus
extern "C"
{
#endif

#define RosInitializeString( \
 __PDEST_STRING__, \
 __LENGTH__, \
 __MAXLENGTH__, \
 __BUFFER__ \
) \
{ \
 (__PDEST_STRING__)->Length = (__LENGTH__); \
 (__PDEST_STRING__)->MaximumLength = (__MAXLENGTH__); \
 (__PDEST_STRING__)->Buffer = (__BUFFER__); \
}

#define RtlRosInitStringFromLiteral( \
 __PDEST_STRING__, __SOURCE_STRING__) \
 RosInitializeString( \
  (__PDEST_STRING__), \
  sizeof(__SOURCE_STRING__) - sizeof((__SOURCE_STRING__)[0]), \
  sizeof(__SOURCE_STRING__), \
  (__SOURCE_STRING__) \
 )
 
#define RtlRosInitUnicodeStringFromLiteral \
 RtlRosInitStringFromLiteral

#define ROS_STRING_INITIALIZER(__SOURCE_STRING__) \
{ \
 sizeof(__SOURCE_STRING__) - sizeof((__SOURCE_STRING__)[0]), \
 sizeof(__SOURCE_STRING__), \
 (__SOURCE_STRING__) \
}

#define ROS_EMPTY_STRING {0, 0, NULL}

NTSTATUS NTAPI RosAppendUnicodeString( PUNICODE_STRING ResultFirst,
				       PUNICODE_STRING Second,
				       BOOL Deallocate );

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
