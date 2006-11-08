#ifndef _WIN32K_NTUSER_H
#define _WIN32K_NTUSER_H


extern char* _file;
extern DWORD _line;
extern DWORD _locked;

extern FAST_MUTEX UserLock;

#define DECLARE_RETURN(type) type _ret_
#define RETURN(value) { _ret_ = value; goto _cleanup_; }
#define CLEANUP /*unreachable*/ ASSERT(FALSE); _cleanup_
#define END_CLEANUP return _ret_;


#define UserEnterCo() UserEnterExclusive()
#define UserLeaveCo() UserLeave()

#define UserEnterShared() UserEnterExclusive()

#define UserEnterExclusive() \
{ \
  /* DPRINT1("try xlock, %s, %i (%i)\n",__FILE__,__LINE__, _locked);*/ \
   if (UserLock.Owner == KeGetCurrentThread()){ \
      DPRINT1("file %s, line %i\n",_file, _line); \
      ASSERT(FALSE); \
   }  \
   UUserEnterExclusive(); \
   ASSERT(InterlockedIncrement((PLONG)(&_locked)) == 1 /*> 0*/); \
   _file = __FILE__; _line = __LINE__; \
  /* DPRINT("got lock, %s, %i (%i)\n",__FILE__,__LINE__, _locked);*/ \
}

#define UserLeave() \
{ \
   ASSERT(InterlockedDecrement((PLONG)(&_locked)) == 0/*>= 0*/); \
   /*DPRINT("unlock, %s, %i (%i)\n",__FILE__,__LINE__, _locked);*/ \
   if (UserLock.Owner != KeGetCurrentThread()) { \
     DPRINT1("file %s, line %i\n",_file, _line); \
     ASSERT(FALSE); \
   } \
   _file = __FILE__; _line = __LINE__; \
   UUserLeave(); \
}
 


NTSTATUS FASTCALL InitUserImpl(VOID);
VOID FASTCALL UninitUser(VOID);
VOID FASTCALL UUserEnterShared(VOID);
VOID FASTCALL UUserEnterExclusive(VOID);
VOID FASTCALL UUserLeave(VOID);
BOOL FASTCALL UserIsEntered();



#endif /* _WIN32K_NTUSER_H */

/* EOF */
