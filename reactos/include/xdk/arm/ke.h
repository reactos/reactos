$if (_WDMDDK_)
#include <armddk.h>
$endif

#define KeMemoryBarrierWithoutFence() _ReadWriteBarrier()
