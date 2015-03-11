#pragma once

#ifndef _PROVIDER_H
#define _PROVIDER_H

#include <windows.h>
#include <provexce.h>

class Provider
{
protected:
    virtual void Flush();
    virtual HRESULT ValidateDeletionFlags(long lFlags);
    virtual HRESULT ValidateMethodFlags(long lFlags);
    virtual HRESULT ValidateQueryFlags(long lFlags);
};

#endif
