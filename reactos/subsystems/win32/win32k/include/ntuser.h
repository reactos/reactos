#ifndef _WIN32K_NTUSER_H
#define _WIN32K_NTUSER_H

#define DECLARE_RETURN(type) type _ret_
#define RETURN(value) { _ret_ = value; goto _cleanup_; }
#define CLEANUP /*unreachable*/ ASSERT(FALSE); _cleanup_
#define END_CLEANUP return _ret_;


#define UserEnterCo UserEnterExclusive
#define UserLeaveCo UserLeave


NTSTATUS FASTCALL InitUserImpl(VOID);
VOID FASTCALL CleanupUserImpl(VOID);
VOID FASTCALL UserEnterShared(VOID);
VOID FASTCALL UserEnterExclusive(VOID);
VOID FASTCALL UserLeave(VOID);
BOOL FASTCALL UserIsEntered();
BOOL FASTCALL UserIsEnteredExclusive();


#endif /* _WIN32K_NTUSER_H */

/* EOF */
