#ifndef _SUBSYS_WIN32K_INCLUDE_CLEANUP_H
#define _SUBSYS_WIN32K_INCLUDE_CLEANUP_H

NTSTATUS INTERNAL_CALL InitCleanupImpl(VOID);

NTSTATUS INTERNAL_CALL
IntSafeCopyUnicodeString(PUNICODE_STRING Dest,
                         PUNICODE_STRING Source);

NTSTATUS INTERNAL_CALL
IntSafeCopyUnicodeStringTerminateNULL(PUNICODE_STRING Dest,
                                      PUNICODE_STRING Source);

NTSTATUS INTERNAL_CALL
IntUnicodeStringToNULLTerminated(PWSTR *Dest, PUNICODE_STRING Src);

void INTERNAL_CALL
IntFreeNULLTerminatedFromUnicodeString(PWSTR NullTerminated, PUNICODE_STRING UnicodeString);

/*
 * User Locks
 */

VOID INTERNAL_CALL
IntInitUserResourceLocks(VOID);

VOID INTERNAL_CALL
IntCleanupUserResourceLocks(VOID);

inline VOID
IntUserEnterCritical(VOID);

inline VOID
IntUserEnterCriticalShared(VOID);

inline VOID
IntUserLeaveCritical(VOID);

inline BOOL
IntUserIsInCriticalShared(VOID);

inline BOOL
IntUserIsInCritical(VOID);

inline NTSTATUS
IntConvertThreadToGUIThread(PETHREAD Thread);

#define TempReleaseUserLock(UserLock) \
  if(IntUserIsInCritical()) \
  { \
    (UserLock) = 1; \
    IntUserLeaveCritical(); \
  } \
  else if(IntUserIsInCriticalShared()) \
  { \
    (UserLock) = 2; \
    IntUserLeaveCritical(); \
  } \
  else \
    (UserLock) = 0

#define TempEnterUserLock(UserLock) \
  switch(UserLock) \
  { \
    case 1: \
      IntUserEnterCritical(); \
      break; \
    case 2: \
      IntUserEnterCriticalShared(); \
      break; \
  } \

#endif /* ndef _SUBSYS_WIN32K_INCLUDE_CLEANUP_H */
