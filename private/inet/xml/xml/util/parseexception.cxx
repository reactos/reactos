/*
 * @(#)ParseException.cxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

#include "parseexception.hxx"

DEFINE_CLASS_MEMBERS_CLASS(ParseException, _T("ParseException"), Exception);

/**
 * This class signals that a parsing exception of some sort has occurred. 
 *
 * @version 1.0, 6/3/97
 */

/**
 * Constructs a <code>ParseException</code> exception with detail about
 * what occurred.
 *
 * @param   s       The detail message.
 * @param   line    The line number of the input where the error was found.
 * @param   column  The position on the line.
 * @param   owner   The context in which the error was encountered.
 * This is either an <code>Entity</code> object or a <code>Parser</code> object. 
 */
ParseException::ParseException(String * s, int line, int column, Object * owner, HRESULT hr)
    : super(s, Exception::ParseException)
{
    this->line = line;
    this->column = column;
    this->owner = owner;
    this->hr = hr;
}
ParseException *
ParseException::newParseException(String * s, int line, int column, Object * owner, HRESULT hr)
{
    return new ParseException(s, line, column, owner, hr);
}

void
ParseException::throwE(String * s, int line, int column, Object * owner, HRESULT hr)
{
    ParseException * e = ParseException::newParseException(s, line, column, owner, hr);

    ((Exception *) e)->throwE();        
}

void
ParseException::throwE(String * s, int line, int column, Object * owner, String * expected, String * found)
{
    ParseException * e = ParseException::newParseException(s, line, column, owner);

    e->expectedToken = expected;
    e->foundToken = found;

    ((Exception *) e)->throwE();
}
