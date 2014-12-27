#pragma once

#ifndef _PROVIDER_H
#define _PROVIDER_H

#include <windows.h>
#include <provexce.h>

class Provider
{
public:
    void Flush();
    HRESULT ValidateDeletionFlags(long lFlags);
    HRESULT ValidateMethodFlags(long lFlags);
    HRESULT ValidateQueryFlags(long lFlags);
};

#endif
