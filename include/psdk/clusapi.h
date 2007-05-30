
#ifndef _CLUSAPI_H
#define _CLUSAPI_H

#ifdef __cplusplus
extern "C" {
#endif

DWORD
WINAPI
GetNodeClusterState(
    IN  LPCWSTR lpszNodeName,
    OUT DWORD   *pdwClusterState
    );

#ifdef __cplusplus
}
#endif
#endif // _CLUSAPI_H
