
#include "precomp.h"

SECURITY_STATUS WINAPI schan_AcceptSecurityContext(
 PCredHandle phCredential, PCtxtHandle phContext, PSecBufferDesc pInput,
 ULONG fContextReq, ULONG TargetDataRep, PCtxtHandle phNewContext,
 PSecBufferDesc pOutput, ULONG *pfContextAttr, PTimeStamp ptsExpiry)
{
    return SEC_E_UNSUPPORTED_FUNCTION;
}

SECURITY_STATUS WINAPI schan_ApplyControlToken(PCtxtHandle phContext,
 PSecBufferDesc pInput)
{
    return SEC_E_UNSUPPORTED_FUNCTION;
}

SECURITY_STATUS WINAPI schan_CompleteAuthToken(PCtxtHandle phContext,
 PSecBufferDesc pToken)
{
    return SEC_E_UNSUPPORTED_FUNCTION;
}

SECURITY_STATUS WINAPI schan_ImpersonateSecurityContext(PCtxtHandle phContext)
{
    return SEC_E_UNSUPPORTED_FUNCTION;
}
