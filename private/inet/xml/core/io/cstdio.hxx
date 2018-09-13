/*
 * @(#)StdIO.hxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
#ifndef _CORE_IO_STDIO
#define _CORE_IO_STDIO

#include "inputstream.hxx"
#include "outputstream.hxx"
#include "printstream.hxx"
#include "fileinputstream.hxx"
#include "fileoutputstream.hxx"
   
/**
 */
class StdIO : public Base
{
    DECLARE_CLASS_MEMBERS(StdIO, Base);

    /** Don't let anyone instantiate this class */
    private: StdIO() 
    {

    }

public:

    static InputStream* getIn();

    static PrintStream* getOut();

    static PrintStream* getErr();

private:

    /**
     */
    static SRInputStream in;

    /**
     */
    static SRPrintStream out;

    /**
     */
    static SRPrintStream err;
};

#endif _CORE_IO_STDIO
