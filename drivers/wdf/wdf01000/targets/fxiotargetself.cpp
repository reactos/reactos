#include "common/fxiotargetself.h"


FxIoTargetSelf::FxIoTargetSelf(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ USHORT ObjectSize
    ) :
    FxIoTarget(FxDriverGlobals, ObjectSize, FX_TYPE_IO_TARGET_SELF),
    m_DispatchQueue(NULL)
{
}

FxIoTargetSelf::~FxIoTargetSelf()
{
}