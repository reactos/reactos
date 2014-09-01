
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

SECURITY_STATUS WINAPI schan_MakeSignature(PCtxtHandle phContext, ULONG fQOP,
 PSecBufferDesc pMessage, ULONG MessageSeqNo)
{
    return SEC_E_UNSUPPORTED_FUNCTION;
}

SECURITY_STATUS WINAPI schan_QuerySecurityPackageInfoA(SEC_CHAR *pszPackageName,
 PSecPkgInfoA *ppPackageInfo)
{
    return SEC_E_UNSUPPORTED_FUNCTION;
}

SECURITY_STATUS WINAPI schan_QuerySecurityPackageInfoW(SEC_WCHAR *pszPackageName,
 PSecPkgInfoW *ppPackageInfo)
{
    return SEC_E_UNSUPPORTED_FUNCTION;
}

SECURITY_STATUS WINAPI schan_RevertSecurityContext(PCtxtHandle phContext)
{
    return SEC_E_UNSUPPORTED_FUNCTION;
}

SECURITY_STATUS WINAPI schan_VerifySignature(PCtxtHandle phContext,
 PSecBufferDesc pMessage, ULONG MessageSeqNo, PULONG pfQOP)
{
    return SEC_E_UNSUPPORTED_FUNCTION;
}
