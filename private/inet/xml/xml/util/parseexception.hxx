/*
 * @(#)ParseException.hxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#ifndef _PARSEEXCEPTION_HXX
#define _PARSEEXCEPTION_HXX

#ifndef _CORE_LANG_EXCEPTION
#include "core/lang/exception.hxx"
#endif


DEFINE_CLASS(ParseException);

/**
 * This class signals that a parsing exception of some sort has occurred. 
 *
 * @version 1.0, 6/3/97
 */
class DLLEXPORT ParseException : public Exception
{
friend class Context;
friend class ElementDecl;
friend class ElementDeclEnumeration;
friend class DTD;
friend class ContentModel;
friend class Entity;
friend class EntityReader;
friend class Notation;
friend class AttDef;
friend class Parser;

    DECLARE_CLASS_MEMBERS(ParseException, Exception);
    DECLARE_CLASS_INSTANCE(ParseException, Exception);

    /**
     * Constructs an <code>ParseException</code> with no detail message. 
     */
public: 

    protected: ParseException(String * s, int line, int column, Object * owner, HRESULT hr = E_FAIL); 

    public: static ParseException * newParseException(String * s, int line, int column, Object * owner, HRESULT hr = E_FAIL); 
    
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
    public: static void throwE(String * s, int line = 0, int column = 0, Object * owner = null, HRESULT hr = E_FAIL); 

    public: static void throwE(String * s, int line, int column, Object * owner, String * expected, String * found); 

    /**
     * The line where the error was found.
     */
    public: int line;

    /**
     * The position on the line where the error was found.
     */
    public: int column;

    /**
     * The token that was expected by the parser.
     */
    public: String* expectedToken;

    /**
     * The token that was found.
     */
    public: String* foundToken;

    public: RString contextText;

    public: int filePosition;

    /**
     * The parser context in which the error ocurred.
     */
    public: WObject owner;

    protected: virtual void finalize()
    {
        owner = null;
        contextText = null;
        super::finalize();
    }
};


#endif _PARSEEXCEPTION_HXX

