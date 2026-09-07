// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------

//
//  File:       HiResTimer.h
//  Contents:   HiResTimer class
//------------------------------------------------------------------------------
#ifndef UtilLib__HiResTimer_h__INCLUDED
#define UtilLib__HiResTimer_h__INCLUDED
#pragma once

class HiResTimer
{
  public:
    HiResTimer() {}
    virtual ~HiResTimer() {}
    virtual double GetTime()      = 0;
    virtual double GetFrequency() = 0;
    virtual void   Reset()        = 0;
};

HiResTimer* CreateHiResTimer();

#endif // UtilLib__HiResTimer_h__INCLUDED


