#include <evntrace.h>

#ifndef _TRACE_H_
#define _TRACE_H_

extern "C"
{
	//
	// Tracing Definitions:
	//
	// Control GUID: 
	// {73e3b785-f5fb-423e-94a9-56627fea9053}
	//

#define WPP_CONTROL_GUIDS                           \
    WPP_DEFINE_CONTROL_GUID(                        \
        SpbTestToolTraceGuid,                       \
        (73e3b785,f5fb,423e,94a9,56627fea9053),     \
        WPP_DEFINE_BIT(TRACE_FLAG_WDFLOADING)       \
        WPP_DEFINE_BIT(TRACE_FLAG_SPBAPI)           \
        WPP_DEFINE_BIT(TRACE_FLAG_OTHER)            \
        )
}

#define WPP_LEVEL_FLAGS_LOGGER(level,flags) WPP_LEVEL_LOGGER(flags)
#define WPP_LEVEL_FLAGS_ENABLED(level, flags) (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= level)

#define Trace CyapaPrint
#define FuncEntry 
#define FuncExit 
#define WPP_INIT_TRACING
#define WPP_CLEANUP 
#define TRACE_FLAG_SPBAPI 0
#define TRACE_FLAG_WDFLOADING 0

// begin_wpp config
// FUNC FuncEntry{LEVEL=TRACE_LEVEL_VERBOSE}(FLAGS);
// FUNC FuncExit{LEVEL=TRACE_LEVEL_VERBOSE}(FLAGS);
// USEPREFIX(FuncEntry, "%!STDPREFIX! [%!FUNC!] --> entry");
// USEPREFIX(FuncExit, "%!STDPREFIX! [%!FUNC!] <--");
// end_wpp

#endif _TRACE_H_
