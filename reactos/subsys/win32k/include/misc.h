#ifndef _SUBSYS_WIN32K_INCLUDE_CLEANUP_H
#define _SUBSYS_WIN32K_INCLUDE_CLEANUP_H

NTSTATUS FASTCALL InitCleanupImpl(VOID);

NTSTATUS FASTCALL
IntSafeCopyUnicodeString(PUNICODE_STRING Dest,
                         PUNICODE_STRING Source);

NTSTATUS FASTCALL
IntSafeCopyUnicodeStringTerminateNULL(PUNICODE_STRING Dest,
                                      PUNICODE_STRING Source);

NTSTATUS FASTCALL
IntUnicodeStringToNULLTerminated(PWSTR *Dest, PUNICODE_STRING Src);

void FASTCALL
IntFreeNULLTerminatedFromUnicodeString(PWSTR NullTerminated, PUNICODE_STRING UnicodeString);

/*
 * User Locks
 */

VOID FASTCALL
IntInitUserResourceLocks(VOID);

VOID FASTCALL
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
