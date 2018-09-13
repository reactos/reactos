/^HRESULT __stdcall I[A-Za-z0-9_]*_Proxy(/,/^$/d
/^void __RPC_STUB I[A-Za-z0-9_]*_Stub(/,/^$/d
/_INTERFACE_DEFINED__/d
/_FWD_DEFINED__/d
/^extern RPC_IF_HANDLE __MIDL/d
/^#include "rpc.h"/d
/^#include "rpcndr.h"/d
/^#include "oleidl.h"/d
/^void __RPC_FAR \* __RPC_USER MIDL_user_allocate(size_t);/d
/^void __RPC_USER /d
s/__RPC_FAR //g
/#define __std_h__/a\
#ifndef _MAC \
#include "oleidl.h" \
#endif
s/#include "oaidl.h"/\
#ifndef _MAC \
#include "oaidl.h"\
#endif/
s/WCHAR/OLECHAR/g
s/LPWSTR/LPOLESTR/g
s/LPCWSTR/LPCOLESTR/g
/typedef struct I.*/,/{/{
/{/a\
#ifdef _MAC \
        BEGIN_INTERFACE \
#endif
}
