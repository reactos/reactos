#include "common/fxcxpnppowercallbacks.h"


BOOLEAN
FxCxPnpPowerCallbackContext::IsPreCallbackPresent(
    VOID
    )
{
    return IsCallbackPresent(FxCxPreCallback);
}

BOOLEAN
FxCxPnpPowerCallbackContext::IsPostCallbackPresent(
    VOID
    )
{
    return IsCallbackPresent(FxCxPostCallback);
}

BOOLEAN
FxCxPnpPowerCallbackContext::IsCleanupCallbackPresent(
    VOID
    )
{
    return IsCallbackPresent(FxCxCleanupCallback);
}

BOOLEAN
FxCxPnpPowerCallbackContext::IsCallbackPresent(
    FxCxCallbackSubType SubType
    )
{
    BOOLEAN present = FALSE;
    
    /*switch(m_CallbackType) {
    case FxCxCallbackPrepareHardware:
    {
        switch(SubType) {
        case FxCxPreCallback:
            present = (NULL != u.PrepareHardware.PreCallback);
            break;
        case FxCxPostCallback:
            present = (NULL != u.PrepareHardware.PostCallback);
            break;
        case FxCxCleanupCallback:
            present = (NULL != u.PrepareHardware.CleanupCallback);
            break;
        default:
            ASSERT(0);
            break;
        }
        break;
    }
    case FxCxCallbackReleaseHardware:
    {
        switch(SubType) {
        case FxCxPreCallback:
            present = (NULL != u.ReleaseHardware.PreCallback);
            break;
        case FxCxPostCallback:
            present = (NULL != u.ReleaseHardware.PostCallback);
            break;
        case FxCxCleanupCallback:
            __fallthrough;
        default:
            ASSERT(0);
            break;
        }
        break;
    }
    case FxCxCallbackD0Entry:
    {
        switch(SubType) {
        case FxCxPreCallback:
            present = (NULL != u.D0Entry.PreCallback);
            break;
        case FxCxPostCallback:
            present = (NULL != u.D0Entry.PostCallback);
            break;
        case FxCxCleanupCallback:
            present = (NULL != u.D0Entry.CleanupCallback);
            break;
        default:
            ASSERT(0);
            break;
        }
        break;
    }
    case FxCxCallbackD0Exit:
    {
        switch(SubType) {
        case FxCxPreCallback:
            present = (NULL != u.D0Exit.PreCallback);
            break;
        case FxCxPostCallback:
            present = (NULL != u.D0Exit.PostCallback);
            break;
        case FxCxCleanupCallback:
            __fallthrough;
        default:
            ASSERT(0);
            break;
        }
        break;
    }
    case FxCxCallbackSmIoInit:
    {
        switch(SubType) {
        case FxCxPreCallback:
            present = (NULL != u.SmIoInit.PreCallback);
            break;
        case FxCxPostCallback:
            present = (NULL != u.SmIoInit.PostCallback);
            break;
        case FxCxCleanupCallback:
            present = (NULL != u.SmIoInit.CleanupCallback);
            break;
        default:
            ASSERT(0);
            break;
        }
        break;
    }
    case FxCxCallbackSmIoRestart:
    {
        switch(SubType) {
        case FxCxPreCallback:
            present = (NULL != u.SmIoRestart.PreCallback);
            break;
        case FxCxPostCallback:
            present = (NULL != u.SmIoRestart.PostCallback);
            break;
        case FxCxCleanupCallback:
            present = (NULL != u.SmIoRestart.CleanupCallback);
            break;
        default:
            ASSERT(0);
            break;
        }
        break;
    }
    case FxCxCallbackSmIoSuspend:
    {
        switch(SubType) {
        case FxCxPreCallback:
            present = (NULL != u.SmIoSuspend.PreCallback);
            break;
        case FxCxPostCallback:
            present = (NULL != u.SmIoSuspend.PostCallback);
            break;
        case FxCxCleanupCallback:
            __fallthrough;
        default:
            ASSERT(0);
            break;
        }
        break;
    }
    case FxCxCallbackSmIoFlush:
    {
        switch(SubType) {
        case FxCxPreCallback:
            present = (NULL != u.SmIoFlush.PreCallback);
            break;
        case FxCxPostCallback:
            present = (NULL != u.SmIoFlush.PostCallback);
            break;
        case FxCxCleanupCallback:
            __fallthrough;
        default:
            ASSERT(0);
            break;
        }
        break;
    }
    case FxCxCallbackSmIoCleanup:
    {
        switch(SubType) {
        case FxCxPreCallback:
            present = (NULL != u.SmIoCleanup.PreCallback);
            break;
        case FxCxPostCallback:
            present = (NULL != u.SmIoCleanup.PostCallback);
            break;
        case FxCxCleanupCallback:
            __fallthrough;
        default:
            ASSERT(0);
            break;
        }
        break;
    }
    case FxCxCallbackSurpriseRemoval:
    {
        switch(SubType) {
        case FxCxPreCallback:
            present = (NULL != u.SurpriseRemoval.PreCallback);
            break;
        case FxCxPostCallback:
            present = (NULL != u.SurpriseRemoval.PostCallback);
            break;
        case FxCxCleanupCallback:
            __fallthrough;
        default:
            ASSERT(0);
            break;
        }
        break;
    }
    default:
        ASSERT(0);
        break;
    }*/

    return present;
}
