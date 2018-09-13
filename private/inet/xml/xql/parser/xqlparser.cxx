/*
 * @(#)XQLParser.cxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#include "core.hxx"
#pragma hdrstop

#include "xqlparser.hxx"

#include "xql/query/elementquery.hxx"
#include "xql/query/treequery.hxx"
#include "xql/query/filterquery.hxx"
#include "xql/query/groupquery.hxx"
#include "xql/query/childrenquery.hxx"
#include "xql/query/orquery.hxx"
#include "xql/query/refquery.hxx"
#include "xql/query/sortedquery.hxx"
#include "xql/query/absolutequery.hxx"
#include "xql/query/ancestorquery.hxx"
#include "xql/query/andexpr.hxx"
#include "xql/query/orexpr.hxx"
#include "xql/query/notexpr.hxx"
#include "xql/query/condition.hxx"
#include "xql/query/constantoperand.hxx"
#include "xql/query/methodoperand.hxx"
#include "xql/query/nodecontextquery.hxx"

#include "core/util/chartype.hxx"
#include "core/util/datatype.hxx"

DEFINE_CLASS_MEMBERS(XQLParser, _T("XQLParser"), Base);

DeclareTag(tagXQLParser, "XQLParser", "Trace XQLParser actions");

#if DBG == 1
extern TAG tagXQLParser;
#endif

void InitTraceTags()
{
    EnableTag(tagXQLParser, FALSE);
}

/**
 * This class implements a parser for XML Query Language according to the 
 * latest Microsoftworking draft of the XQL specification. 
 * @version 1.0, 4/1/98
 */

/**
 * Token matching hashtable
 */    
SRHashtable XQLParser::s_tokens;
SRString XQLParser::s_strMinus;

SRAtom XQLParser::s_atomAttribute;
SRAtom XQLParser::s_atomID;
SRAtom XQLParser::s_atomAnd;
SRAtom XQLParser::s_atomOr;
SRAtom XQLParser::s_atomNot;


/*
    This is the table that maps from a TOKENTYPE to the specific constructor
    function used to construct a operand for that type.
*/
ParamInfo XQLParser::s_ElementParamInfo = {ELEMENT,1,{ASTRING|AOPTIONAL}, Element::ELEMENT, XQLParser::constructChildQuery};

ParamInfo XQLParser::s_methodTable[] =
{
    // Complex queries
    {QCONTEXT,1,{ANUMBER | AOPTIONAL}, MethodOperand::INVALID, XQLParser::constructContextQuery},
    {TEXT,0,{AERROR}, MethodOperand::INVALID, XQLParser::constructTextQuery},
    {ID,1,{AQUERY}, MethodOperand::INVALID, XQLParser::constructRefQuery},
    {ANCESTOR,1,{ASTRING}, MethodOperand::INVALID, XQLParser::constructAncestorQuery},

    // Simple text taking queries
    {TEXTNODE,0,{AERROR}, Element::PCDATA, XQLParser::constructChildQuery},
    {TEXTNODE_ALT,0,{AERROR}, Element::PCDATA, XQLParser::constructChildQuery},
    {NODE,0,{AERROR}, Element::ANY, XQLParser::constructNodeQuery},
    {ATTRIBUTE,1,{ASTRING|AOPTIONAL}, Element::ATTRIBUTE, XQLParser::constructChildQuery},
    {ELEMENT,1,{ASTRING|AOPTIONAL}, Element::ELEMENT, XQLParser::constructChildQuery},
    {CDATA,0,{AERROR}, Element::CDATA, XQLParser::constructChildQuery},
    {PI,1,{ASTRING|AOPTIONAL}, Element::PI, XQLParser::constructChildQuery},
    {COMMENT,0,{AERROR}, Element::COMMENT, XQLParser::constructChildQuery},

    // Simple Single MethodOperand Methods
    {NODEVALUE,0,{AERROR},MethodOperand::VALUE, XQLParser::constructMethod},
    {NODETYPE,0,{AERROR}, MethodOperand::NODETYPE, XQLParser::constructMethod},
    {NODENAME,0,{AERROR}, MethodOperand::NODENAME, XQLParser::constructMethod},
    {INDEX,0,{AERROR}, MethodOperand::INDEX, XQLParser::constructMethod},
    {END,0,{AERROR}, MethodOperand::END, XQLParser::constructMethod},

    {STRING_CAST,1,{ASTRING}, DT_STRING, XQLParser::constructConversion},
    {DATE_CAST,1,{ASTRING}, DT_DATETIME_ISO8601TZ, XQLParser::constructConversion},
    {NUMBER_CAST,1,{ASTRING}, DT_R8, XQLParser::constructConversion},
};


XQLParser::TokenInfo XQLParser::s_tokenTable[] = 
{
    EQ_ALT, null,
    NE_ALT, null,
    LT_ALT, null,
    LE_ALT, null,           
    GE_ALT, null,           
    GT_ALT, null,           
    AND_ALT, (Atom **) &s_atomAnd,          
    OR_ALT, (Atom **) &s_atomOr,          
    NOT_ALT, (Atom **) &s_atomNot,          
    IEQ, null,
    INE, null,
    ILT, null,
    ILE, null,
    IGE, null,
    IGT, null, 
    AND_OLD, null,
    OR_OLD, null,
    NOT_OLD, null,

    NODE, null,
    ELEMENT, null,
    ATTRIBUTE, (Atom **) &s_atomAttribute,
    TEXTNODE, null,
    TEXTNODE_ALT, null,
    TEXT, null,
    COMMENT, null,         
    PI, null,              
    CDATA, null,           

    NODEVALUE, null,       
    NODETYPE, null,        
    NODENAME, null,

    INDEX, null,           
    END, null,
    ANCESTOR, null,
    ID,  (Atom **) &s_atomID,

    STRING_CAST, null,
    NUMBER_CAST, null,
    DATE_CAST, null,

    ANY, null,
    ALL, null,
    
    QCONTEXT, null
};



extern CSMutex * g_pMutex;

void 
XQLParser::classInit()
{
    int i;

    if (!s_tokens)
    {    
        MutexLock lock(g_pMutex);
#ifdef RENTAL_MODEL
        Model model(MultiThread);
#endif

        TRY
        {
            // check again in case an other parser entered before this...
            if (!s_tokens)
            {
                Name::classInit();

                s_strMinus = String::newString(L"-");

                Hashtable * pTokens = Hashtable::newHashtable();
                for (i = 0; i < LENGTH(s_tokenTable); i++)
                {
                    xqlTokenType token = s_tokenTable[i]._token;
                    String * strToken = String::newString(tokenChars(token));
                    pTokens->put(strToken, Integer::newInteger(token));
                    Atom ** pAtom = s_tokenTable[i]._pAtom;
                    if (pAtom)
                    {
                        Atom * atom = Atom::create(strToken);
                        * (SRAtom *) pAtom = atom;
                    }
                }
                s_tokens = pTokens;
            }
        }
        CATCH
        {
            lock.Release();
#ifdef RENTAL_MODEL
            model.Release();
#endif
            Exception::throwAgain();
        }
        ENDTRY
    }
}


XQLParser *
XQLParser::newXQLParser(const bool shouldAddRef)
{
    XQLParser *parser = new XQLParser(shouldAddRef);
    return parser;
}

XQLParser::XQLParser() 
{
    init(false);
}

XQLParser::XQLParser(const bool shouldAddRef) 
{
    init(shouldAddRef);
}


void
XQLParser::finalize()
{
    _nsmgr = null;
}


void XQLParser::init(const bool shouldAddRef)
{
    _shouldAddRef = shouldAddRef;

    if (!s_tokens)
    {
        InitTraceTags();
        classInit();
    }
}

    
/**
 * XQLParser::parse
 *
 * Parse an XQL string and return an exceution plan.
 */
Query * XQLParser::parse(const String * s, NamespaceMgr * nsmgr)
{
     return parse(s->toCharArrayZ(), nsmgr);
}

/**
 * XQLParser::parse
 *
 * Parse an XQL string and return an exceution plan.
 */    

Query * XQLParser::parse(const ATCHAR * chars, NamespaceMgr * nsmgr)
{
     return parse(chars->getData(), nsmgr);
}


/**
 * XQLParser::parse
 *
 * Parse an XQL string and return an exceution plan.
 */    

// BUGBUG - Change to take string length.
//          Remove parse which takes a string.

Query * XQLParser::parse(const TCHAR * pszIn, NamespaceMgr * nsmgr)
{
    TraceTag((tagXQLParser, "begin parsing %s", (char *)AsciiText(String::newString(pszIn))));
    Operand * opnd;
    Query * qy = null;

    _nsmgr = nsmgr;
    if (prepareToParse(pszIn))
    {        
        TraceTag((tagXQLParser, "begin parsing %s",(char *) AsciiText(String::newString(pszIn))));

        const TCHAR * pchOperand = _pchToken;

        opnd = parseXPointer(null, BaseQuery::SCALAR);
 
        // BUGBUG pass in ppszOut so that caller can use XQL to parse a fragment.
        // For now throw an exception is all of the input isn't consumed.
        checkToken(XQL_EOF);

        qy = getQuery(opnd, pchOperand);

        TraceTag((tagXQLParser, "opnd = %s",(char *) AsciiText(opnd ? opnd->toString() : String::nullString())));
    }
    else
    {
        // BUGBUG - throw exception or just return an empty query?
        Exception::throwE(E_INVALIDARG);
    }

    return qy;
}


Query * XQLParser::parseOrderBy(Query * qyInput, const String * s)
{
     return parseOrderBy(qyInput, s->toCharArrayZ());
}

    
Query * XQLParser::parseOrderBy(Query * qyInput, const ATCHAR * chars)
{
    return parseOrderBy(qyInput, chars->getData());
}
    
/**
 * XQLParser::parseOrderBy
 *
 * Parse an XQL string and return an exceution plan.
 */    

Query * XQLParser::parseOrderBy(Query * qyInput, const TCHAR * pszIn)
{
    Query * qy;
    SortedQuery * sqy;

    if (prepareToParse(pszIn))
    {        
        TraceTag((tagXQLParser, "begin parsing %s",(char *) AsciiText(String::newString(pszIn))));

        sqy = SortedQuery::newSortedQuery(qyInput, BaseQuery::SCALAR, _shouldAddRef);

        while (true)
        {
            parseSortKey(sqy);

            if (_token != SEMICOLON)
            {
                break;
            }

            nextToken();
        }

        qy = sqy;

        // BUGBUG pass in ppszOut so that caller can use XQL to parse a fragment.
        // For now throw an exception is all of the input isn't consumed.
        checkToken(XQL_EOF);
    }
    else
    {
        qy = qyInput;
    }

    TraceTag((tagXQLParser, "qy = %s",(char *) AsciiText(qy ? qy->toString() : String::nullString())));
    return qy;
}


const TCHAR *
XQLParser::prepareToParse(const TCHAR * pszIn, int len)
{
    if (pszIn)
    {        
        _pchNext= _pszXQL = pszIn;
        _lookahead = INVALIDTOKEN;
        advance();
        nextToken();
    }
    return pszIn;
}



Query * XQLParser::parseSortKey(SortedQuery * sqy)
{
    bool    fDescending = false;
    Operand * opnd;
    Query * qy;

    if (_token == PLUS)
    {
        nextToken();
    }
    else if (_token == MINUS)
    {
        fDescending = true;
        nextToken();
    }

    const TCHAR * pchOperand = _pchToken;
    opnd = parseXPointer(null, BaseQuery::SCALAR);
    qy = getQuery(opnd, pchOperand);

    sqy->addKey(qy, fDescending, opnd->getDataType());
    return sqy;
}



/**
 * XQLParser::parseXPointer
 *
 * Parse an XPointer
 *
 * XPointer ::= Query (',' Query)*
 */    

Operand * XQLParser::parseXPointer(Query * qyInput, BaseQuery::Cardinality card)
{
    OrQuery * oqy = null;
    const TCHAR * pchOperand = _pchToken;

    Operand * opnd = parseQuery(qyInput, card);

    if (_token == PIPE)
    {
        oqy = OrQuery::newOrQuery(qyInput, card, _shouldAddRef);

        DWORD dwFlags = Query::STAYS_IN_SUBTREE | Query::IS_ABSOLUTE;

        while (true)
        {
            Query * qy = getQuery(opnd, pchOperand, dwFlags);

            oqy->addQuery(qy);

            if (_token != PIPE)
            {
                break;
            }

            pchOperand = _pchToken;

            nextToken();

            opnd = parseQuery(qyInput, card);

            // The first query can either be absolute or in the subtree.  The remaining queries must match the
            // first one.

            if (dwFlags == (Query::STAYS_IN_SUBTREE | Query::IS_ABSOLUTE))
            {
                dwFlags &= qy->getFlags();
            }
        }

        opnd = oqy->toOperand();
    }

    return opnd;
}


/**
 * XQLParser::parseQuery
 *
 * Parse a Query
 *
 * Query ::= TreeQuery
 *          |   '/' TreeQuery
 *          |   '//' TreeQuery
 */    

Operand * XQLParser::parseQuery(Query * qyInput, BaseQuery::Cardinality card)
{
    // BUGBUG - Shouldn't this always return a QUERY???
    // BUGBUG - ParseOperand should return an generic Operand.

    TraceTag((tagXQLParser, "begin query %s", (char *)AsciiText(String::newString(currentPos()))));

    if (_token == SLASH)
    {
        qyInput = AbsoluteQuery::newAbsoluteQuery(_shouldAddRef);
        nextToken();
        if (_token == XQL_EOF)
        {
            return qyInput->toOperand();
        }
    }
    else if (_token == SLASHSLASH)
    {
        qyInput = AbsoluteQuery::newAbsoluteQuery(_shouldAddRef);
        nextToken();
        return constructTreeQuery(qyInput, card);
    }

    return parseTreeQuery(qyInput, false, card);
}


/**
 * XQLParser::parseTreeQuery
 *
 * Parses a TreeQuery
 *
 * TreeQuery ::= ChildrenQuery
 *          |   ChildrenQuery '//' TreeQuery
 */    

Operand * XQLParser::parseTreeQuery(Query * qyInput, bool fInTreeQuery, BaseQuery::Cardinality card)
{
    Query * qy;
    Operand * opndCond;
    const TCHAR * pchOperand = _pchToken;
    Operand * opnd = parseChildrenQuery(qyInput, fInTreeQuery, card);

    if (_token == SLASHSLASH)
    {
        nextToken();

        opnd = constructTreeQuery(getQuery(opnd, pchOperand), card);
    }

    return opnd;
}


Operand *
XQLParser::constructTreeQuery(Query * qyInput, BaseQuery::Cardinality card)
{
    Query * qy;
    Query * qyCond;
    Operand * opndCond;
    Operand * opnd;
    const TCHAR * pchOperand = _pchToken;

    opndCond = parseTreeQuery(null, true, card);

    if (opndCond)
    {
        qyCond = getQuery(opndCond, pchOperand, Query::SUPPORTS_CONTAINS | Query::STAYS_IN_SUBTREE);
    }
    else
    {
        qyCond = null;
    }

    qy = TreeQuery::newTreeQuery(qyInput, qyCond, card, _shouldAddRef);

    if (tokenEndsTreeQuery())
    {
        opnd = parseChildrenQuery(qy, false, card);
    }
    else
    {
        opnd = qy->toOperand();
    }

    return opnd;
}

    // Since we have parenthesis, we will read in 
    // the number of parameters
int 
XQLParser::readParameters(ParamInfo *paraminfo, XQLParser *parser, const TCHAR * pchOperand, const bool hasParens, Operand **aopnd, const TCHAR ** apchOperand, const count)
{
    DataType dt = DT_NONE;
    int cArgs = 0;
    int maxArgs = 0;

    Operand * opnd = null;
    Operand ** popnd = aopnd;
    const TCHAR ** ppchOperand = apchOperand;
    OperandValue opval;

    if (!paraminfo)
        return 0;

    maxArgs = paraminfo->_max_args;            

    if (hasParens)
    {
        while ((parser->_token != RPARENS) && (parser->_token != XQL_EOF))
        {
            if (cArgs < maxArgs)
            {
                *ppchOperand++ = parser->_pchToken;
                opnd = parser->parseOperand(BaseQuery::SCALAR);

                // WAA - last step to do
                // Get data type from operand.
                // See if it compares favorably with the argument types specified
                // for this particular argument.
                //opnd->setDataType(paraminfo->_argTypes[cArgs]);
                //if (opnd->getDataType() != paraminfo->_argTypes[cArgs])
                //{
                //    parser->throwE(XQL_INVALID_CAST, pchOperand);
                //}
            
                Assert(opnd);
                *popnd++ = opnd;
                if (opnd)
                    cArgs++;

                if (parser->_token == COMMA)
                {
                    parser->nextToken();
                }
                else if (parser->_token == XQL_EOF)
                {
                    parser->tokenError(RPARENS);
                }
            }
            else
            {
                // BUGBUG - add error message too many args
                Assert("too many method args");
                parser->tokenError(RPARENS);
            }
        }
        
        // If we don't have a right parenthesis at this point, then
        // there is a syntax error.
        parser->checkToken(RPARENS);

        // Move past the right parenthesis
        parser->nextToken();
    }

    return cArgs;
}

Operand *
XQLParser::constructChildQuery(ParamInfo *paraminfo, XQLParser *parser,
                            const TCHAR * pchOperand, NameAtoms *na, const bool hasParens, 
                            Query * qyInput, const bool isMethod, BaseQuery::Cardinality card)
{
    int cArgs = 0;
    // BUGBUG, what's the right thing to do with _atomXXX?
    //NameAtoms na(parser->_atomURN, parser->_atomPrefix, parser->_atomName);
    Operand * opnd = null;
    Operand * aopnd[MAX_ARGS];
    const TCHAR * apchOperand[MAX_ARGS];
    OperandValue opval;

    Assert(parser);

    if (isMethod)
    {
        parser->throwE(XQL_METHOD_NOT_SUPPORTED, pchOperand);
    }

    // Read in the parameters
    if (hasParens)
    {
        na->init(null, null, null);
        cArgs = XQLParser::readParameters(paraminfo, parser, pchOperand, hasParens, aopnd, apchOperand, MAX_ARGS);
    }
    else
    {
        //na.init(null, null, null);
    }

    if (!na->_atomName && cArgs)
    {
        // BUGBUG - make a helper in BaseOperand
        aopnd[0]->getValue(null, null, null, &opval);
            
        if (!opval.isString())
        {
            // BUGBUG - need to pass apchOperand to readParameters so that it can be filled in.

            parser->throwE(XQL_EXPR_NOT_STRING, apchOperand[0]); 
        }

        parser->scanName(opval._s->getWCHARPtr(), opval._s->length(), na);
    }

    // BUGBUG - pass a NameAtoms to Children?
    opnd = ChildrenQuery::newChildrenQuery(qyInput, na->_atomURN, na->_atomPrefix, na->_atomName, 
        card, Element::NodeType(paraminfo->_type), parser->_shouldAddRef);

    return opnd;
}

Operand *
XQLParser::constructNodeQuery(ParamInfo *paraminfo, XQLParser *parser,
                            const TCHAR * pchOperand, NameAtoms *na, const bool hasParens, 
                            Query * qyInput, const bool isMethod, BaseQuery::Cardinality card)
{
    int cArgs = 0;
    Operand * opnd = null;
    Operand * aopnd[MAX_ARGS];
    const TCHAR * apchOperand[MAX_ARGS];
    OperandValue opval;

    Assert(parser);

    if (isMethod)
    {
        parser->throwE(XQL_METHOD_NOT_SUPPORTED, pchOperand);
    }


    // Read in the parameters
    if (hasParens)
    {
        na->init(null, null, null);
        cArgs = XQLParser::readParameters(paraminfo, parser, pchOperand, hasParens, aopnd, apchOperand, MAX_ARGS);
    }
    

    if (!na->_atomName && cArgs)
    {
        // BUGBUG - make a helper in BaseOperand
        aopnd[0]->getValue(null, null, null, &opval);
            
        if (!opval.isString())
        {
            parser->throwE(XQL_EXPR_NOT_STRING, apchOperand[0]); 
        }

        parser->scanName(opval._s->getWCHARPtr(), opval._s->length(), na);
    }

    // BUGBUG - pass a NameAtoms to Children?
    opnd = ChildrenQuery::newChildrenQuery(qyInput, na->_atomURN, na->_atomPrefix, na->_atomName, 
        card, Element::NodeType(paraminfo->_type), parser->_shouldAddRef);

    return opnd;
}

Operand *
XQLParser::constructMethod(ParamInfo *info, XQLParser *parser,
                            const TCHAR * pchOperand,  NameAtoms *na, const bool hasParens,
                            Query * qyInput, const bool isMethod, BaseQuery::Cardinality card)
{
    Operand *opnd;

    Assert(parser);

    opnd = MethodOperand::newMethodOperand(isMethod ? qyInput : null, MethodOperand::MethodType(info->_type));

    // If we don't have a right parenthesis at this point, then
    // there is a syntax error.
    parser->checkToken(RPARENS);

    parser->nextToken();

    return opnd;
}

/*
    This method is used in those cases where we don't actually support
    a particular method, but we want to be able to parse the parameters
    anyway.  Like date("January, 1997").  This allows us to acknowledge that
    a method exists, and skip over it because we don't implement it.
*/

Operand *
XQLParser::constructNULLMethod(ParamInfo *info, XQLParser *parser,
                            const TCHAR * pchOperand,  NameAtoms *na, const bool hasParens,
                            Query * qyInput, const bool isMethod, BaseQuery::Cardinality card)
{
    int cArgs = 0;
    Operand * opnd = null;
    Operand * aopnd[MAX_ARGS];
    const TCHAR * apchOperand[MAX_ARGS];
    OperandValue opval;

    Assert(parser);

    // Read in the parameters
    cArgs = XQLParser::readParameters(info, parser, pchOperand, hasParens, aopnd, apchOperand, MAX_ARGS);

    return 0;
}

Operand *
XQLParser::constructConversion(ParamInfo *paraminfo, XQLParser *parser, 
                             const TCHAR * pchOperand,  NameAtoms *na, const bool hasParens,
                             Query * qyInput, const bool isMethod, BaseQuery::Cardinality card)
{
    Assert(parser);
    Assert(paraminfo);

    pchOperand = parser->_pchToken;
    Operand * opnd = parser->parseOperand(BaseQuery::SCALAR);

    if (!opnd)
    {
        parser->throwE(XQL_EXPR_NOT_QUERY_OR_STRING, pchOperand);
    }

    // WAA - last step to do
    // Get data type from operand.
    // See if it compares favorably with the argument types specified
    // for this particular argument.
    opnd->setDataType(DataType(paraminfo->_type));
    if (opnd->getDataType() != DataType(paraminfo->_type))
    {
        parser->throwE(XQL_INVALID_CAST, pchOperand);
    }
            
    // If we don't have a right parenthesis at this point, then
    // there is a syntax error.
    parser->checkToken(RPARENS);

    // Move past the right parenthesis
    parser->nextToken();

    return opnd;
}


Operand *
XQLParser::constructTextQuery(ParamInfo *info, XQLParser *parser,
                            const TCHAR * pchOperand,  NameAtoms *na, const bool hasParens,
                            Query * qyInput, const bool isMethod, BaseQuery::Cardinality card)
{
    Operand * opnd;
    OrQuery * oqy;

    Assert(parser);
 
    oqy = OrQuery::newOrQuery(qyInput, card, parser->_shouldAddRef);
    oqy->addQuery(ChildrenQuery::newChildrenQuery(null, null, null, null, BaseQuery::SCALAR, Element::PCDATA, parser->_shouldAddRef));
    oqy->addQuery(ChildrenQuery::newChildrenQuery(null, null, null, null, BaseQuery::SCALAR, Element::CDATA, parser->_shouldAddRef));
    opnd = oqy;

    // If we don't have a right parenthesis at this point, then
    // there is a syntax error.
    parser->checkToken(RPARENS);

    parser->nextToken();

    return opnd;
}

Operand *
XQLParser::constructRefQuery(ParamInfo *info, XQLParser *parser, 
                             const TCHAR * pchOperand,  NameAtoms *na, const bool hasParens,
                             Query * qyInput, const bool isMethod, BaseQuery::Cardinality card)
{
    Operand * opnd;
    Query * qy;
    OperandValue opval;

    Assert(parser);


    opnd = parser->parseXPointer(qyInput, BaseQuery::SCALAR);
    qy = opnd->toQuery();
    
    if (qy)
    {
        opnd = RefQuery::newRefQuery(null, qy, BaseQuery::SCALAR, parser->_shouldAddRef);
    }
    else 
    {
        opnd->getValue(null, null, null, &opval);
        if (!opval.isString())
        {
            parser->throwE(XQL_EXPR_NOT_QUERY_OR_STRING, pchOperand);
        }

        if (qyInput)
        {
            parser->queryInvalidHere(pchOperand);
        }

        opnd = RefQuery::newRefQuery(opval._s, qyInput, BaseQuery::SCALAR, parser->_shouldAddRef);
    }

    // If we don't have a right parenthesis at this point, then
    // there is a syntax error.
    parser->checkToken(RPARENS);

    parser->nextToken();

    return opnd;
}

Operand *
XQLParser::constructAncestorQuery(ParamInfo *info, XQLParser *parser, 
                             const TCHAR * pchOperand,  NameAtoms *na, const bool hasParens,
                             Query * qyInput, const bool isMethod, BaseQuery::Cardinality card)
{
    Operand * opnd = null;
    Operand * opndCond;
    Query * qyCond;
    OperandValue opval;

    Assert(parser);

    opndCond = parser->parseXPointer(null, BaseQuery::SCALAR);
    qyCond = parser->getQuery(opndCond, pchOperand);
    opnd = AncestorQuery::newAncestorQuery(qyInput, qyCond, parser->_shouldAddRef);

    // The reason we call getQuery is that it will throw a exception if the
    // query is not a proper ancestor query.  We ignore the return value because
    // we already have a pointer to the query going in.
    parser->getQuery(opnd, pchOperand, qyInput);

    // If we don't have a right parenthesis at this point, then
    // there is a syntax error.
    parser->checkToken(RPARENS);

    parser->nextToken();

    return opnd;
}

Operand *
XQLParser::constructContextQuery(ParamInfo *info, XQLParser *parser, 
                             const TCHAR * pchOperand,  NameAtoms *na, const bool hasParens,
                             Query * qyInput, const bool isMethod, BaseQuery::Cardinality card)
{
    Operand * opnd = null;
    OperandValue opval;

    Assert(parser);


    if ((NUMBER == parser->_token) || (MINUS == parser->_token))
    {
        opnd = parser->parseOperand(BaseQuery::SCALAR);
        opnd->setDataType(DT_R8);
        if (opnd->getDataType() != DT_R8)
        {
            parser->throwE(XQL_INVALID_CAST, pchOperand);
        }
        
        opval._r8 = 0.0;
        opnd->getValue(null, null, null, &opval);
    }
    else if (parser->_token == RPARENS)
    {
        // If the user didn't specify a parameter, then we
        // default to the context that is above us, -1
        // so, context() == context(-1).  This is probably
        // the most common usage case.
        opval.initR8(-1.0);
    }

    if (!opval.isI4())
    {
        parser->throwE(XQL_EXPR_NOT_INTEGER, pchOperand);
    }
    opnd = NodeContextQuery::newNodeContextQuery((int)opval._r8, parser->_shouldAddRef);

    // If we don't have a right parenthesis at this point, then
    // there is a syntax error.
    parser->checkToken(RPARENS);

    parser->nextToken();

    return opnd;
}


/**
 * XQLParser::parseChildrenQuery
 *
 * Parse a ChildrenQuery
 *
 * ChildrenQuery ::= FilteredQuery 
 *              |   FileredQuery '/' ChildenQuery
 */    

Operand * XQLParser::parseChildrenQuery(Query * qyInput, bool fInTreeQuery, BaseQuery::Cardinality card)
{
    Operand * opnd;
    Query * qy;

    const TCHAR * pchOperand = _pchToken;
    opnd = parseFilteredQuery(qyInput, fInTreeQuery, card);

    if (_token == SLASH)
    {
            nextToken();
            qy = opnd->toQuery();
            if (!qy || !(qy->getFlags() & Query::WRAPS_INPUT))
            {
                getQuery(opnd, pchOperand, qyInput);
            }
            const TCHAR * pchOperand = _pchToken;
            opnd = parseChildrenQuery(qy, fInTreeQuery, card);
            if (opnd || !fInTreeQuery)
            {
                qy = getQuery(opnd, pchOperand);
            }
    }

    return opnd;
}



/**
 * XQLParser::parseFilteredQuery
 *
 * Parse a set
 *
 * Set ::= '^'? ('@'? QualifiedName | . | .. | MethodCall | '(' XPointer ')')
 *          ('[' Condition ']' | '.'
 */    

Operand * XQLParser::parseFilteredQuery(Query * qyInput, bool fInTreeQuery, BaseQuery::Cardinality card)
{
    Operand * opnd = null;
    Query * qy;
    const TCHAR * pchOperand = _pchToken;

    switch (_token)
    {
    case XQL_EOF:
        break;

    case AT:
        _atomURN = _atomPrefix = _atomName = null;
        nextToken();
        pchOperand = _pchToken;
        if (_token == NAME || _token == STAR)
        {
            opnd = ChildrenQuery::newChildrenQuery(qyInput, _atomURN, _atomPrefix, _atomName, card, Element::ATTRIBUTE, _shouldAddRef);
            nextToken();
        }
        else
        {
            tokenError(NAME);
        }

        break;

    case NAME:
        opnd = parseMethod(qyInput, false, fInTreeQuery, card);
        break; 

    case STAR:
        opnd = ChildrenQuery::newChildrenQuery(qyInput, null, null, null, card, Element::ELEMENT, _shouldAddRef);
        nextToken();
        break;

    case DOT:
        nextToken();
        if (qyInput)
        {
            opnd = qyInput->toOperand();
        }
        else
        {
            opnd = ElementQuery::newElementQuery(null, _shouldAddRef);
        }
        break;

    case DOTDOT:
        opnd = AncestorQuery::newAncestorQuery(qyInput, null, _shouldAddRef);
        getQuery(opnd, pchOperand, qyInput);
        nextToken();
        break;

    case LPARENS:
        nextToken();
        opnd = parseXPointer(null, BaseQuery::SCALAR);
        qy = getQuery(opnd, pchOperand, qyInput);
        if (opnd)
        {
            opnd = GroupQuery::newGroupQuery(qyInput, qy, card, _shouldAddRef);
        }
        checkToken(RPARENS);
        nextToken();
        break;

        // BUGBUG - Without this ParseQuery can only return a Query.
        // ParseOperand returns a Query or an Expression.  However,
        // it doesn't work for refquery parsing code.  Fix ParseOperand
        // and remove this.

    case STRING:
    case NUMBER:
        opnd = parseOperand(card);
        break;

    default:
        tokenError(null);
    }

    // parse modifier list

    while (_token != null)
    {
        BaseQuery::Cardinality cardCond = card;
        if (_token == LBRACKET)
        {
            // opnd must be a query for [] to follow it
            OperandValue opval;

            Query * qy = getQuery(opnd, pchOperand);

            nextToken();

            pchOperand = _pchToken;

            Operand * opndArg = parseCondition();

            // Make sure we ended up with a right bracket.
            checkToken(RBRACKET);
            nextToken();

            if (!opndArg)
                return null;

            // if the argument in brackets is a number, treat it as an array index
            // else pass it directly to FilterQuery
            // In this case we don't need real context of query because we're
            // just getting the value out of this thing.  
             opndArg->getValue(null, null, null, &opval);
            if (opval.isI4())
            {
                /*  
                    array indexes are equivalent to .index method
                    (e.g. FOOD[1]/NAME is the same as FOOD[index() = 1]/NAME)
                    so we construct an equivalent expression
                */

                // BUGBUG - compile without EQ_RELOP - have ConstantOperandValue implement indexing directly.
                Operand * opndIndex = MethodOperand::newMethodOperand(null, MethodOperand::INDEX);
                opndArg = Condition::newCondition(OperandValue::EQ_RELOP, opndIndex, opndArg);
                cardCond = BaseQuery::SCALAR;
            }

            opnd = FilterQuery::newFilterQuery(qy, opndArg, cardCond, _shouldAddRef);

        }
        else if (_token == BANG)
        {
            nextToken();
            // BUGBUG - Need a MethodQuery to handle card of [$ANY$ x!value() = "foo"]
            // Add method opcodes to BaseQuery?

            checkToken(NAME);
            opnd = parseMethod(getQuery(opnd, pchOperand), true, fInTreeQuery, card);
        }
        else
        {
            break;
        }
    }

    return opnd;
}


/**
 * XQLParser::parseCondition
 *
 * Parse a condition
 *
 * Condition ::= AndExpr
 *          |    AndExpr '$OR$' Condition    
 */    

Operand * XQLParser::parseCondition()
{
    Operand * opnd1, * opnd2;
    opnd1 = parseAndExpr();
    if (_token == OR || _token == OR_OLD || (_token == NAME && !_atomPrefix && _atomName == s_atomOr))
    {
        nextToken();
        opnd2 = parseCondition();
        opnd1 = OrExpr::newOrExpr(opnd1, opnd2);
    }
    return opnd1;
}    


/**
 * XQLParser::andExpr
 *
 * Parse an AND expression
 *
 * AndExpr::= RelationalExpr
 *          |    RelationalExpr '$AND$' AndExpr    
 */    

Operand * XQLParser::parseAndExpr()
{
    Operand * opnd1, * opnd2;

    opnd1 = parseRelationalExpr();
    if (_token == AND || _token == AND_OLD || (_token == NAME && !_atomPrefix && _atomName == s_atomAnd))
    {
        nextToken();
        opnd2 = parseAndExpr();
        opnd1 = AndExpr::newAndExpr(opnd1, opnd2);
    }

    return opnd1;
}

/**
 * XQLParser::parseRelationalExpr
 *
 * Parse a relation expresion.
 *
 * Condition ::= '$NOT$'? Operand (('=' | '!='| '$LT$' | '$LE$' | '$GE$' | '$GT$') Operand)?
 */    

Operand * XQLParser::parseRelationalExpr()
{
    Operand * opnd1, * opnd2;
    OperandValue::RelOp op;
    BaseQuery::Cardinality card = BaseQuery::SCALAR;
    bool    negate = false;

    skipSpace();
    if (_token == NAME && _lookahead == L'(' && !_atomPrefix && _atomName == s_atomNot)
    {
        negate = true;
        nextToken();
    }
    else if (_token == NOT_OLD)
    {
        negate = true;
        nextToken();
    }

    // set query cardinality
    switch(_token)
    {
    case ANY:
        card = BaseQuery::ANY;
        break;

    case ALL:
        card = BaseQuery::ALL;
        break;
    }

    if (card != BaseQuery::SCALAR)
    {
        nextToken();
    }

    // BUGBUG - Stop passing card around.  After calling parseOperand call setCard() to set the operand's cardinality.

    opnd1 = parseOperand(card);

    // set condition operation
    switch(_token)
    {
    case EQ:
    case EQ_ALT:
        op = OperandValue::EQ_RELOP;
        break;

    case NE:
    case NE_ALT:
        op = OperandValue::NE_RELOP;
        break;

    case LT:
    case LT_ALT:
        op = OperandValue::LT_RELOP;
        break;

    case LE:
    case LE_ALT:
        op = OperandValue::LE_RELOP;
        break;

    case GT:
    case GT_ALT:
        op = OperandValue::GT_RELOP;
        break;

    case GE:
    case GE_ALT:
        op = OperandValue::GE_RELOP;
        break;

    case IEQ:
        op = OperandValue::IEQ_RELOP;
        break;

    case INE:
        op = OperandValue::INE_RELOP;
        break;

    case ILT:
        op = OperandValue::ILT_RELOP;
        break;

    case ILE:
        op = OperandValue::ILE_RELOP;
        break;

    case IGT:
        op = OperandValue::IGT_RELOP;
        break;

    case IGE:
        op = OperandValue::IGE_RELOP;
        break;

    default:
        goto Cleanup;
    }

    nextToken();
    opnd2 = parseOperand(BaseQuery::SCALAR);

    opnd1 = Condition::newCondition(op, opnd1, opnd2);

Cleanup:
    if (negate)
    {
        opnd1 = NotExpr::newNotExpr(opnd1);
    }

    return opnd1;
}


/**
 * XQLParser::parseOperand
 *
 * Parse an Operand
 *
 * Operand ::= IndexOperand
 *          | String
 *          | Query
 *          | '(' Condition ')'
 */ 

Operand * XQLParser::parseOperand(BaseQuery::Cardinality card)
{
    Operand * opnd = null;
    int sign = 1;
    
    switch (_token)
    {
    case MINUS:
        sign = -1;
        nextToken();
        checkToken(NUMBER);
        opnd = ConstantOperand::newR8(String::add(s_strMinus, _string,0), sign * V_R8(&_var));
        nextToken();
        break;

    case STRING:
        opnd = ConstantOperand::newString(_string);
        nextToken();
        break;

    case NUMBER:
        opnd = ConstantOperand::newR8(_string, V_R8(&_var));
        nextToken();
        break;

    case LPARENS:
        nextToken();
        opnd = parseCondition();
        checkToken(RPARENS);
        nextToken();
        break;

    default:
        opnd = parseXPointer(null, card);
        break;
    }

    return opnd;
}


/**
 * XQLParser::parseMethod
 *
 * Parse a Name or a MethodCall
 *
 * Method ::= MethodName ('(' Operand (',' Operand)* ')')?
 */ 

Operand * XQLParser::parseMethod(Query * qyInput, bool isMethod, bool isTreeQuery, BaseQuery::Cardinality card)
{
    Operand * opnd = null;
    String * string = _string;
    xqlTokenType token;
    NameAtoms na(_atomURN, _atomPrefix, _atomName);
    ParamInfo * paraminfo=0;
    bool hasParens = false;


    const TCHAR * pchOperand = _pchToken;

    Assert(_token == NAME);

    skipSpace();

    // if token is LPARENS then this must be a function or method call
    // if token isn't an LPARENS then treat as an element name

    if (_lookahead == LPARENS)
    {
        hasParens = true;

        if (na._atomPrefix)
        {
            throwE(XQL_UNKNOWN_METHOD, pchOperand); 
        }

        if (isTreeQuery && tokenEndsTreeQuery())
        {
            return null;
        }

        advance();

        token = lookup(na._atomName->toString());
        paraminfo = lookupParamInfo(token);


        TraceTag((tagXQLParser, "method call token %s(%d), %s", (char *)AsciiText(String::newString(tokenChars(_token))),_token, (char *)AsciiText(string)));
    }
    else
    {

        // by default, if it's not a method name, then it must
        // be the name of a element, so we construct a child query.
        TraceTag((tagXQLParser, "element %s:%s", (char *)AsciiText(_atomPrefix), (char *)AsciiText(_atomName)));
        
        paraminfo = &s_ElementParamInfo;

    }


    nextToken();

    // In both cases, once we have a paraminfo, we call the constructor
    if (paraminfo)
    {
        Assert(paraminfo->_constructor);

        opnd = (* paraminfo->_constructor)(paraminfo, this, pchOperand, &na, hasParens, qyInput, isMethod, card);
    } 
    else
    {
        throwE(XQL_UNKNOWN_METHOD, pchOperand);
    }

    return opnd;
}


/**
 * expect _token type t
 */    
void XQLParser::checkToken(xqlTokenType t) //throws Exception
{
    if (_token != t)
    {
        tokenError(tokenString(t));
    }
}


void XQLParser::skipSpace()
{
    while (_lookahead != null && isWhiteSpace((TCHAR)_lookahead))
    {
        advance();
    }
}


/**
 * return next _token
 * @exception Exception * when syntax or other error is encountered.
 */    
xqlTokenType XQLParser::nextToken() //throws Exception
{
    skipSpace();

    _pchToken = currentPos();

    switch (_lookahead)
    {
        case 0:
        case _T(','):
        case _T('@'):
        case _T('('):
        case _T(')'):
        case _T('['):
        case _T(']'):
        case _T(':'):
        case _T(';'):
        case _T('*'):
        case _T('+'):
        case _T('-'):
        case _T('='):
        case _T('#'):
            _token = (xqlTokenType) _lookahead;
            advance();
            break;

        case _T('&'):
            // BUGBUG - This parsing patterns recurs a lot.  Can it be made more generic to reduce
            // code size?
            advance();
            if (_lookahead == _T('&'))
            {
                advance();
                _token = AND;
            }
            else
            {
                tokenError(AND);
            }
            break;

        case _T('|'):
            advance();
            if (_lookahead == _T('|'))
            {
                advance();
                _token = OR;
            }
            else
            {
                _token = PIPE;
            }
            break;


        case _T('<'):
            _token = (xqlTokenType) _lookahead;
            advance();
            if (_lookahead == _T('='))
            {
                advance();
                _token = LE;
            }
            break;

        case _T('>'):
            _token = (xqlTokenType) _lookahead;
            advance();
            if (_lookahead == _T('='))
            {
                advance();
                _token = GE;
            }
            break;

        case _T('/'):
            advance();
            if (_lookahead == _T('/'))
            {
                advance();
                _token = SLASHSLASH;
            }
            else
            {
                _token = SLASH;
            }
            break;

        case _T('!'):
            advance();
            if (_lookahead == _T('='))
            {
                advance();
                _token = NE;
            }
            else
            {
                _token = BANG;
            }
            break;

       case _T('.'):
            advance();
            if (_lookahead == _T('.'))
            {
                advance();
                _token = DOTDOT;
            }                
            else
            {
                _token = DOT;
            }
            break;

        case _T('$'):
            advance();
            if (isLetter(_lookahead))
            {
                scanKeyword();
            }
            break;

        case _T('"'):
        case _T('\''):
            scanString();
            break;

        default:
            NameAtoms na;

            const TCHAR * pchNext = scanName(_pchToken, 0, &na);
            if (pchNext > _pchToken)
            {
                _token = NAME;
                _atomURN = na._atomURN;
                _atomPrefix = na._atomPrefix;
                _atomName = na._atomName;
                advance(pchNext);
            }
            else 
            {
                scanNumber();
            }
     }

    TraceTag((tagXQLParser, "found token %s(%d), %s", (char *)AsciiText(String::newString(tokenChars(_token))), _token, (char *)AsciiText(String::newString(_pchToken, 0, (int)(currentPos() - _pchToken)))));

    return _token;
}

void XQLParser::advance(const TCHAR * pchNext)
{
    _pchNext = pchNext;
    advance();
}


const TCHAR * XQLParser::tokenChars(xqlTokenType token)
{
    switch (token) 
    {                   
    case XQL_EOF        : return _T("eof");
    case COMMA          : return _T(",");
    case SLASH          : return _T("/");
    case AT             : return _T("@");
    case DOT            : return _T(".");
    case LPARENS        : return _T("(");
    case RPARENS        : return _T(")");
    case LBRACKET       : return _T("[");
    case RBRACKET       : return _T("]");
    case COLON          : return _T(":");
    case SEMICOLON      : return _T(";");
    case STAR           : return _T("*");
    case PLUS           : return _T("+");
    case MINUS          : return _T("-");
    case EQ             : return _T("=");
    case LT             : return _T("<");
    case GT             : return _T(">");
    case DOLLAR         : return _T("$");
    case APOS           : return _T("'");
    case QUOTE          : return _T("\"");
    case PIPE           : return _T("|");      
    case DOTDOT         : return _T("..");
    case SLASHSLASH     : return _T("//"); 
    case NAME           : return _T("NAME");
    case STRING         : return _T("STRING");
    case NUMBER         : return _T("NUMBER");
    case NE             : return _T("!=");
    case LE             : return _T("<=");
    case GE             : return _T(">=");
    case EQ_ALT         : return _T("$eq$");
    case NE_ALT         : return _T("$ne$");
    case LT_ALT         : return _T("$lt$");
    case LE_ALT         : return _T("$le$");
    case GE_ALT         : return _T("$ge$");
    case GT_ALT         : return _T("$gt$");
    case AND            : return _T("&&");
    case OR             : return _T("||");
    case BANG           : return _T("!");
    case AND_ALT        : return _T("and");
    case OR_ALT         : return _T("or");
    case NOT_ALT        : return _T("not");
    case AND_OLD        : return _T("$and$");
    case OR_OLD         : return _T("$or$");
    case NOT_OLD        : return _T("$not$");
    case TEXTNODE       : return _T("textnode");    
    case TEXTNODE_ALT   : return _T("textNode");
    case TEXT           : return _T("text");        
    case COMMENT        : return _T("comment");
    case PI             : return _T("pi");
    case CDATA          : return _T("cdata");
    case ELEMENT        : return _T("element");
    case ATTRIBUTE      : return _T("attribute");
    case NODE           : return _T("node");
    case NODEVALUE      : return _T("value");
    case NODETYPE       : return _T("nodeType");
    case NODENAME       : return _T("nodeName");
    case INDEX          : return _T("index");
    case END            : return _T("end");
    case ANCESTOR       : return _T("ancestor");
    case ID             : return _T("id");
    case NUMBER_CAST    : return _T("number");
    case STRING_CAST    : return _T("string");
    case DATE_CAST      : return _T("date");
    case ANY            : return _T("$any$");
    case ALL            : return _T("$all$");
    case IEQ            : return _T("$ieq$");
    case INE            : return _T("$ine$");
    case ILT            : return _T("$ilt$");
    case ILE            : return _T("$ile$");
    case IGE            : return _T("$ige$");
    case IGT            : return _T("$igt$");
    case QCONTEXT       : return _T("context");
    default             : return _T("unknown");
    }
}

ParamInfo *
XQLParser::lookupParamInfo(xqlTokenType aToken)
{
    int count = LENGTH(s_methodTable);
    int i;

    for (i = 0; i < count; i++)
    {
        if (s_methodTable[i]._tokenType == aToken)
            return &s_methodTable[i];
    }

    return 0;
}

xqlTokenType XQLParser::lookup(String * n)
{
    Object * o = s_tokens->get(n);
    if (o)
    {
        _token = (xqlTokenType) ((Integer *)o)->intValue();
    }
    else
    {
        _token = NAME;
    }

    return _token;
}



/**
 * Scan a $keyword$
 */    

void XQLParser::scanKeyword() //throws Exception
{
    while (isLetter(_lookahead))
    {
        advance();
    }

    if (_lookahead == _T('$'))
    {             
        advance();
        _string = String::newString(_pchToken, 0, (int)(currentPos() - _pchToken));
        lookup(_string);
    }
    else
    {
	tokenError(DOLLAR);
    }
}



/**
 * Scan a quoted string
 *
 * @param q match the end
 */    
void XQLParser::scanString() //throws Exception
{
    TCHAR   endChar = _lookahead;
    StringBuffer * buffer = StringBuffer::newStringBuffer();
    
    advance();

    while (_lookahead != null && _lookahead != endChar)
    {
        if (_lookahead == '\\')
        {
            advance();
            if (_lookahead == null)
                break;
        }

        buffer->append(_lookahead);
        advance();
    }

    if (_lookahead == null)
    {
        throwE(MSG_E_UNCLOSEDSTRING, _pchToken);
    }

    advance();

    _token = STRING;
    _string = buffer->toString();
}


const TCHAR * XQLParser::scanName(const TCHAR * pchToken, int len, NameAtoms * na)
{
    ULONG ul = 0;
    const TCHAR * pchNext = ParseName(pchToken, &ul);

    if (len && pchNext - pchToken != len)
    {
        if (*pchNext == L':')
            pchNext++;
        //pchNext++;
        throwE(MSG_E_BADNAMECHAR, pchNext, pchNext+1, pchToken);
    }

    if (*pchNext == L':')
    {
        if (pchNext[1] == L'*')
        {
            ul = (ULONG)(pchNext - pchToken);
            pchNext += 2;
        }
    }

    if (ul > 0)
    {
        na->_atomPrefix = Atom::create(pchToken, ul);
        ul++;
    }
    else
    {
        na->_atomPrefix = null;
    }

    const TCHAR * pchName = pchToken + ul;
    if (*pchName != L'*')
    {
        na->_atomName = Atom::create(pchName, (int)(pchNext - pchToken - ul));
    }
    else
    {
        na->_atomName = null;
    }

    if (_nsmgr && na->_atomPrefix)
    {
        na->_atomURN = _nsmgr->findURN(na->_atomPrefix, null);
    }
    else
    {
        na->_atomURN = null;
    }

    return pchNext;
}


void XQLParser::scanNumber()
{
    const WCHAR * pwcNext = null;

    if (FAILED(ParseNumeric(_pchToken, 0, DT_R8, &_var, &pwcNext)))
    {
        if (pwcNext > pwcNext)
        {
            tokenError(NUMBER);
        }
        else
        {
            advance();
            throwE(XQL_UNEXPECTED_CHAR, _pchToken);
        }
    }

    if (pwcNext)
    {
        _token = NUMBER;
        _string = String::newString(_pchToken, 0, (int)(pwcNext - _pchToken));
        advance(pwcNext);
    }
}


bool XQLParser::tokenEndsTreeQuery()
{

    // When parsing a tree query stop parsing if any of the following are found
    //      id(
    // These have to parsed after the query is constructed.

     return _token == NAME && _atomPrefix == null && _atomName == s_atomID && _lookahead == LPARENS;
}


Query *
XQLParser::getQuery(Operand * opnd, const TCHAR * pchOperand, Query * qyInput)
{
    return getQuery(opnd, pchOperand, qyInput ? (qyInput->getFlags() & Query::STAYS_IN_SUBTREE) : 0);
}


Query *
XQLParser::getQuery(Operand * opnd, const TCHAR * pchOperand, DWORD dwFlagsCheck)
{
    Query * qy = opnd ? opnd->toQuery() : null;

    if (!qy)
    {
        throwE(XQL_EXPR_NOT_DOM_NODE, pchOperand);
    }

    if (dwFlagsCheck)
    {
        DWORD dwFlags = qy->getFlags();

        if (!(dwFlags & Query::NOT_FROM_ROOT))
        {
            dwFlags |= Query::IS_ABSOLUTE;
        }            
        
        if (!(dwFlags & dwFlagsCheck & (Query::IS_ABSOLUTE | Query::STAYS_IN_SUBTREE)))
        {
            goto Error;
        }

        if ((dwFlagsCheck & Query::SUPPORTS_CONTAINS))
        {
            if (!(dwFlags & Query::SUPPORTS_CONTAINS))
            {
                goto Error;
            }
        }
    }

Cleanup:

    return qy;

Error:
    queryInvalidHere(pchOperand);
    goto Cleanup;
}


void XQLParser::queryInvalidHere(const TCHAR * pchOperand)
{
    throwE(XQL_QUERY_INVALID_HERE, String::newString(pchOperand, 0, (int)(currentPos() - pchOperand)), pchOperand);
}

    

void XQLParser::tokenError(xqlTokenType t)
{
    tokenError(tokenString(t));
}

/**
 * Throws an error exception in cases where a particular _token was expected.
 * The parameter may be null.
 * @param expected  Name of the expected _token
 */                                 
void XQLParser::tokenError(String * expected) //throws Exception
{
    String* tokenStr = tokenString(_token);
    String* message = expected ? 
        Resources::FormatMessage(MSG_E_EXPECTED_TOKEN, expected, tokenStr, null) :
        Resources::FormatMessage(MSG_E_UNEXPECTED_TOKEN, tokenStr, null);

    error(errorMsg(message, _pchToken));
}

/**
 * throw error
 */                                 
void XQLParser::error(String * s) //throws Exception
{
    // BUGBUG - return E_FAIL or more specific error?

    Exception::throwE(s, E_FAIL);
}

void XQLParser::error(TCHAR * c) //throws Exception
{
    error(String::newString(c));
}


String * XQLParser::tokenString(xqlTokenType token)
{
    const TCHAR * c = tokenChars(token);
    if (c != null)
        return String::add(String::newString(tokenChars(APOS)), String::newString(c), String::newString(tokenChars(APOS)), null);
    else
        return String::newString(_pchToken, 0, (int)(currentPos() - _pchToken));
}


String * XQLParser::errorMsg(const TCHAR * msg, 
                             const TCHAR * pchError, const TCHAR * pchErrorEnd,
                             const TCHAR * pchStart)
{
    return errorMsg(String::newString(msg), pchError, pchErrorEnd, pchStart);
}


String * XQLParser::errorMsg(String * msg, 
                             const TCHAR * pchError, const TCHAR * pchErrorEnd, 
                             const TCHAR * pchStart)
{
    // if we were given a starting point, use that, otherwise use the start of the query
    const TCHAR * pchStartPos = (null == pchStart) ? _pszXQL : pchStart;
    if (!pchErrorEnd)
        pchErrorEnd = currentPos();
    int c1 = (int)(pchError - pchStartPos);
    int c2 = (int)(pchErrorEnd - pchError);
    if (c2)
    {
        StringBuffer * sb = StringBuffer::newStringBuffer();

        sb->append(msg);

        sb->append(_T('\n'));
        sb->append(const_cast<TCHAR *>(pchStartPos), 0, c1);
        sb->append(_T("-->"));
        sb->append(const_cast<TCHAR *>(pchStartPos), c1, c2);
        sb->append(_T("<--"));
        sb->append(const_cast<TCHAR *>(pchStartPos) + c1 + c2);

        msg = sb->toString();
    }

    return msg;
}


void
XQLParser::throwE(ResourceID resID,
                  const TCHAR * pchError, const TCHAR * pchErrorEnd,
                  const TCHAR * pchStart)
{
    throwE(resID, null, pchError, pchErrorEnd, pchStart); 
}

void
XQLParser::throwE(ResourceID resID, String * str, 
                  const TCHAR * pchError, const TCHAR * pchErrorEnd, 
                  const TCHAR * pchStart)
{
    error(errorMsg(Resources::FormatMessage(resID, str, null), pchError, pchErrorEnd, pchStart)); 
}
