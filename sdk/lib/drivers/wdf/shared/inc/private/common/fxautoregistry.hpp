/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxAutoRegistry.hpp

Abstract:

    This is the C++ header for registry related objects which follows the RAII
    (resource acquisition is initialization) pattern where
    it frees the allocated item when the struct goes out of scope.

Author:



Revision History:




--*/
#ifndef _FXAUTOREGISTRY_H_
#define _FXAUTOREGISTRY_H_

struct FxAutoRegKey {
public:
    FxAutoRegKey()
    {
        m_Key = NULL;
    }

    ~FxAutoRegKey()
    {
        if (m_Key != NULL) {
            FxRegKey::_Close(m_Key);
        }
    }

public:
    HANDLE m_Key;
};

#endif // _FXAUTOREGISTRY_H_
