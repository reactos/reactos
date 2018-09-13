/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef TEB_H_
#define TEB_H_

#ifdef UNIX
#ifndef LPCONTEXT
#define LPCONTEXT void*
#endif // LPCONTEXT
#endif // UNIX

#ifdef UNIX
inline LONG GETTHREADCONTEXT(HANDLE hThread, LPCONTEXT lpContext)
{
    LONG fRet = FALSE;
    // There is no Thread Context under Unix right now,
    // as registers are included on the stack for Solarius
#else
inline bool GETTHREADCONTEXT(HANDLE hThread, LPCONTEXT lpContext)
{
    bool fRet = FALSE;
    fRet = (0 != ::GetThreadContext(hThread, lpContext));
    Assert(fRet && "Couldn't get thread context"); 
#endif // UNIX
    return fRet;
}

#ifdef UNIX
// Special functions for Solaris only
EXTERN_C const void * MwGetStackBase(void);
EXTERN_C const void * MwGetCurrentStackPointer(void);
#endif // UNIX



// we define the TEB here since we need it in Win95 as well
struct TEB
{
    PVOID ExceptionList;    // this is a real structure pointer but we don't care...
    PVOID StackBase;
    PVOID StackLimit;
};



#ifdef UNIX
// Workaround for GetThreadContext not being available under Unix
EXTERN_C CONTEXT g_context;
#endif // UNIX


// BUGBUG we should really get this from core\base\core.hxx
// but teb.c needs to be renamed teb.cxx
#ifdef MSXML_EXPORT
#define DLLEXPORT   __declspec( dllexport )
#else
#define DLLEXPORT
#endif

EXTERN_C DLLEXPORT TEB * getTEB();

#endif // TEB_H_
