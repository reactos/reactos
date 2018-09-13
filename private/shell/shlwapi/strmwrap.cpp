#include "priv.h"


// Stub - CStreamWrap moved to shdocvw

STDAPI SHCreateStreamWrapper(IStream *aStreams[], UINT cStreams, DWORD grfMode, IStream **ppstm)
{
    *ppstm = NULL;

    return E_FAIL;
}
