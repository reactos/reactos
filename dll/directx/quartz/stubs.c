
typedef void* ProxyFileInfo;
typedef void* CLSID;
#define RPC_ENTRY

void
RPC_ENTRY
GetProxyDllInfo(const ProxyFileInfo *** pInfo, const CLSID ** pId)
{
    *pInfo  = 0;
    *pId    = 0;
};
