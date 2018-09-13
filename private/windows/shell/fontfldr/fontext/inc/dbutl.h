/***************************************************************************
 * dbutl.h -- Debugging utilities. 
 *
 *
 ***************************************************************************/

#if !defined(__DBUTL_H__)
#define __DBUTL_H__

// ********************************************************************
// Write a debug message to the debugger or a file
//
#define DM_ERROR          0x0001
#define DM_TRACE1         0x0002  // interface call tracing (except for messages)
#define DM_TRACE2         0x0004  // status type stuff.  These messages only
                                  // make sense with DM_TRACE1 enabled
#define DM_MESSAGE_TRACE1  0x0008  // message tracing
#define DM_MESSAGE_TRACE2 0x0010  // trace all messages (processed or not)
#define DM_NOW            0x0020  // Used for temp debugging during dev.
#define DM_NOEOL          0x1000 // don't follow the message with a \r\n

#ifdef _DEBUG
#include <assert.h>

#define DEBUGBREAK      DebugBreak()
#define DEBUGMSG(a)     DebugMessage a
#define DEBUGHRESULT(a) DebugHRESULT a
#define DEBUGREFIID(a)  DebugREFIID a

void DebugMessage(UINT mask, LPCTSTR pszMsg, ... );
void DebugHRESULT(int flags, HRESULT hResult);
void DebugREFIID(int flags, REFIID riid);

#define ASSERT(a)    assert((a))

#else
#define DEBUGBREAK
#define DEBUGMSG(a)
#define DEBUGHRESULT(a)
#define DEBUGREFIID(a)
#define ASSERT(a)
#endif


#endif // __DBUTL_H__
