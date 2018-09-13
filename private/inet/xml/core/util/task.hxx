/*
 * @(#)Task.hxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#ifndef _CORE_UTIL_TASK
#define _CORE_UTIL_TASK

#ifndef _CORE_LANG_EXCEPION
#include "core/lang/exception.hxx"
#endif

class NOVTABLE Task : public Object
{
public:

    virtual void run();
    virtual void abort(Exception * e);
};

#endif _CORE_UTIL_TASK