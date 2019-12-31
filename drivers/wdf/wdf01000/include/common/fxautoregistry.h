#ifndef _FXAUTOREGISTRY_H_
#define _FXAUTOREGISTRY_H_

#include "fxregkey.h"

struct FxAutoRegKey {
public:
    FxAutoRegKey()
    {
        m_Key = NULL;
    }

    ~FxAutoRegKey()
    {
        if (m_Key != NULL)
        {
            FxRegKey::_Close(m_Key);
        }
    }

public:
    HANDLE m_Key;
};

#endif // _FXAUTOREGISTRY_H_