/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "nt.h"

#ifdef UNIX
#include "teb.h"
#endif // UNIX


#ifdef UNIX
//
// UNIXREVIEW - This is a hack and will only work for single thread programs right now.
//
struct TEB g_teb;

#endif // UNIX

TEB * getTEB()
{
// BUGBUG - need to do something different under HP
#ifdef UNIX
#ifdef ux10
    g_teb.StackLimit = (int*)MwGetStackBase();               
    g_teb.StackBase  = (int*)MwGetCurrentStackPointer();     
#else
    g_teb.StackBase = (PVOID)MwGetStackBase();
    g_teb.StackLimit = (PVOID)MwGetCurrentStackPointer();
    return (TEB *)&g_teb;
#endif
#else
    return (TEB *)NtCurrentTeb();
#endif
}
