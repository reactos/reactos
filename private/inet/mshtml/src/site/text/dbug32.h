#error "@@@ Nuke this file"

#ifndef I_DBUG32_H_
#define I_DBUG32_H_
#pragma INCMSG("--- Beg 'dbug32.h'")

#define ASSERTDATA

//Initiate tracing.  This declares an instance of the CTtrace class
//on the stack.  Subsystem (ss), Scope (sc), and the function name
//(sz) must be specifed.  ss and sc are specified using the macros
//defined in this header (i.e. - TRCSUBSYSTOM, TRCSCOPEEXTERN, etc.).
//sz can be a static string.
#define TRACEBEGIN(ss, sc, sz)

//Same as TRACEBEGIN but it takes the additional param which is interpreted
//by TraceMsg as a Text Message request.
#define TRACEBEGINPARAM(ss, sc, sz, param)


#if DBG == 1

//This is the buffer length used for building messages
//#define MAXDEBUGSTRLEN (MAX_PATH + MAX_PATH)


//The following constants are used to specify and interpret
//packed values in the DWORD flags parameter passed to TraceMsg.
//Each of these is held in a 4-bit field in the DWORD.


#define TraceError(sz, sc)  if(FAILED(sc)) TraceTag((tagError, "%s, error=%ld (%#08lx).", sz, sc, sc))

//Macro for TraceError
#define TRACEERRSZSC(sz, sc) TraceError(sz, sc)

//Warning based on GetLastError or default message if no last error.
//#define TRACEWARN            TraceTag(tagWarning, "Error: %hr", GetLastWin32Error())

//Error based on GetLastError or default message if no last error.
#define TRACEERROR(hr)       TraceTag((tagError, "Error: %hr", hr))

//Warning based on HRESULT hr
#define TRACEWARNHR(hr)      TraceTag((tagWarning, "Error: %hr", hr))

//Test for a failure HR && warn
#define TESTANDTRACEHR(hr)   if( hr < 0 ) { TRACEWARNHR(hr); }

//Error based on HRESULT hr
#define TRACEERRORHR(hr)     TraceTag((tagError, "Error: %hr", hr))

//Warning using string sz
#define TRACEWARNSZ(sz)      TraceTag((tagWarning, sz))

//Error using string sz
#define TRACEERRORSZ(sz)     TraceTag((tagError, sz))

//Error using string sz
#define TRACEINFOSZ(sz)      TraceTag((tagWarning, sz))

#else

#define TraceError(sz, sc)
#define TRACEERRSZSC(sz, sc)
#define TRACEERROR(hr)
#define TRACEWARNHR(hr)
#define TESTANDTRACEHR(hr)
#define TRACEERRORHR(hr)
#define TRACEWARNSZ(sz)
#define TRACEERRORSZ(sz)
#define TRACEINFOSZ(sz)

#endif //DEBUG

#pragma INCMSG("--- End 'dbug32.h'")
#else
#pragma INCMSG("*** Dup 'dbug32.h'")
#endif
