#include "../../../../ntoskrnl/include/internal/arm/asmmacro.S"

//
// Exceptions
//
GENERATE_ARM_STUB LdrInitializeThunk
GENERATE_ARM_STUB RtlGetCallersAddress
GENERATE_ARM_STUB RtlUnwind
GENERATE_ARM_STUB RtlpGetExceptionAddress
GENERATE_ARM_STUB RtlDispatchException
GENERATE_ARM_STUB RtlpGetStackLimits
GENERATE_ARM_STUB DbgUserBreakPoint
GENERATE_ARM_STUB KiFastSystemCall
GENERATE_ARM_STUB KiFastSystemCallRet
GENERATE_ARM_STUB KiIntSystemCall
GENERATE_ARM_STUB KiUserApcDispatcher
GENERATE_ARM_STUB RtlInitializeContext

