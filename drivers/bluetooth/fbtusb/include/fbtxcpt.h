#ifndef _FBT_EXCEPT_H

#include "fbtLog.h"

#ifdef __cplusplus

#include "fbtSeXcpt.h"

#define FBT_TRY try {
#define FBT_CATCH } catch (fbtSeException *e) { fbtLog(fbtLog_Failure, "Exception %08X caught at line %u in file %s", e->GetSeCode(), __LINE__, __FILE__); fbtLog(fbtLog_Exception, "Exception %08X caught at line %u in file %s", e->GetSeCode(), __LINE__, __FILE__);
#define FBT_CATCH_RETURN(RETVAL) FBT_CATCH return RETVAL;}
#define FBT_CATCH_NORETURN FBT_CATCH return;}

#else

#define FBT_TRY __try{
#define FBT_CATCH } __except(EXCEPTION_EXECUTE_HANDLER) { fbtLog(fbtLog_Failure, "Exception %08X caught at line %u in file %s (%s)", GetExceptionCode(), __LINE__, __FILE__, GetCommandLine()); fbtLog(fbtLog_Exception, "Exception %08X caught at line %u in file %s (%s)", GetExceptionCode(), __LINE__, __FILE__, GetCommandLine());
#define FBT_CATCH_RETURN(RETVAL) FBT_CATCH return RETVAL;}
#define FBT_CATCH_NORETURN FBT_CATCH return;}
#define FBT_CATCH_NODEBUG_RETURN(x) } __except(EXCEPTION_EXECUTE_HANDLER) { return x;}

#endif // __cplusplus

#endif _FBT_EXCEPT_H
