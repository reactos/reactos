/*
 * @(#)Stdio.cxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

DEFINE_CLASS_MEMBERS(StdIO, _T("StdIO"), Base);

SRInputStream StdIO::in;
SRPrintStream StdIO::out;
SRPrintStream StdIO::err;

InputStream* StdIO::getIn()
{
    if (!in)   
    {
        in = FileInputStream::newFileInputStream(STD_INPUT_HANDLE);
    }
    return in;
}

PrintStream* StdIO::getOut()
{
    if (!out)
    {
        out = PrintStream::newPrintStream(FileOutputStream::newFileOutputStream(STD_OUTPUT_HANDLE));
    }
    return out;
}

PrintStream* StdIO::getErr()
{
    if (!err)
    {
        err = PrintStream::newPrintStream(FileOutputStream::newFileOutputStream(STD_ERROR_HANDLE));
    }
    return err;
}