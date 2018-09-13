/*
 * @(#)XQLParser.hxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _XQL_PARSER_XQLPARSER
#define _XQL_PARSER_XQLPARSER

#ifndef _XQL_QUERY_BASEQUERY
#include "xql/query/basequery.hxx"
#endif

DEFINE_CLASS(XQLParser);
DEFINE_CLASS(Query);
DEFINE_CLASS(BaseQuery);
DEFINE_CLASS(SortedQuery);
DEFINE_CLASS(Operand);


//
// BUGBUG - Clean up public/protected and private declarations
//

#ifdef NEVER
HRESULT __stdcall CreateXQLParser(REFIID iid, void **ppvObj);
#endif

class NameAtoms;
class XQLParser;
struct ParamInfo;



typedef Operand * (* _QUERY_CONSTRUCTOR)(ParamInfo *info, XQLParser *parser, const TCHAR * pchOperand,  NameAtoms *na, const bool hasParens, Query * qyInput, const bool isMethod, BaseQuery::Cardinality);



enum xqlArgType {
    AERROR = 0x00,
    ANAME = 0x01,
    ASTRING = 0x02,
    ANUMBER = 0x04,
    AQUERY = 0x10,
    ADATE = 0x20,
    AANY = 0xff,
    AOPTIONAL = 0x1000,
};


enum xqlTokenType
{
    COMMA           = _T(','),
    SLASH           = _T('/'),
    CARET           = _T('^'),
    AT              = _T('@'),
    DOT             = _T('.'),
    LPARENS         = _T('('),
    RPARENS         = _T(')'),
    LBRACKET        = _T('['),
    RBRACKET        = _T(']'),
    COLON           = _T(':'),
    SEMICOLON       = _T(';'),
    STAR            = _T('*'),
    PLUS            = _T('+'),
    MINUS           = _T('-'),
    EQ              = _T('='),
    LT              = _T('<'),
    GT              = _T('>'),
    BANG            = _T('!'),
    DOLLAR          = _T('$'),
    APOS            = _T('\''),
    QUOTE           = _T('"'),
    PIPE            = _T('|'),
    XQL_EOF         = 0,
    INVALIDTOKEN    = -1,
    DOTDOT          = -2,   // ..
    SLASHSLASH      = -3,   // //
    NAME            = -4,   // XML Name
    STRING          = -5,   // Quoted string constant
    NUMBER          = -6,   // Number constant

    NE              = -7,   // !=
    LE              = -8,   // <=
    GE              = -9,   // >=
    AND             = -10,  // &&
    OR              = -11,  // ||
    AND_ALT         = -12,  // and
    OR_ALT          = -13,  // or
    NOT_ALT         = -14,  // not

    EQ_ALT          = -15,  // $eq$
    NE_ALT          = -16,  // $ne$
    LT_ALT          = -17,  // $lt$
    LE_ALT          = -18,  // $le$
    GE_ALT          = -19,  // $ge$
    GT_ALT          = -20,  // $gt$
    IEQ             = -21,  // $ieq$
    INE             = -22,  // $ine$
    ILT             = -23,  // $ilt$
    ILE             = -24,  // $ile$
    IGE             = -25,  // $ige$
    IGT             = -26,  // $igt$

    ELEMENT         = -30,  // element
    ATTRIBUTE       = -31,  // attribute
    TEXTNODE        = -32,  // textNode
    CDATA           = -33,  // cdata
    COMMENT         = -34,  // comment
    TEXT            = -35,  // textnode | cdata
    PI              = -36,  // pi
    NODE            = -38,  // node

    NODEVALUE       = -40,  // value
    NODETYPE        = -42,  // nodeType
    NODENAME        = -43,  // nodeName

    INDEX           = -44,  // index
    END             = -46,  // end

    ANCESTOR        = -51,  // ancestor

    NUMBER_CAST     = -52,  // number   - cast operand to number
    DATE_CAST       = -53,  // date     - cast operand to date
    STRING_CAST     = -54,  // string   - cast operand to string

    TEXTNODE_ALT    = -56,  // textNode - deprecated name for textnode

    ID              = -58,  // id 
    
    ANY             = -60,  // $any$
    ALL             = -61,  // $all$

    QCONTEXT		= -62,	// context	- context in which the Query is executing

    AND_OLD         = -63,  // $and$
    OR_OLD          = -64,  // $or$
    NOT_OLD         = -65   // $not$
};



class XQLParser: public Base
{    
    DECLARE_CLASS_MEMBERS(XQLParser, Base);

    public: static void classInit();


public:
#ifdef NEVER
    // IUnknown

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppvObject);
#endif

    /**
     * Create new parser object where parser will use the given factory for
     * creating nodes, and the given root as the starting point, and the
     * given flags controlling how to parse.  Next you need to set either an
     * InputStream or a URL and then call ParseDocument.
     */
    static XQLParser * newXQLParser(const bool shouldAddRef);

    public: XQLParser(); //throws Exception
    public: XQLParser(const bool shouldAddRef); //throws Exception

    protected: void finalize();

    private: void init(const bool shouldAddRef=false);

     /**
     * expect XML document
     *
     * Document ::= Prolog Element * Misc*
     */    
    public: Query * parse(const String * s, NamespaceMgr * nsmgr = null); //throws Exception
    public: Query * parse(const ATCHAR * chars, NamespaceMgr * nsmgr = null); //throws Exception
            // BUGBUG - pass in the length - string come from BSTR so we know it!!!
            // BUGBUG - can parser use Query in place of BaseQuery ???
    public: Query * parse(const TCHAR * pszIn, NamespaceMgr * nsmgr = null); //throws Exception

    public: Query * parseOrderBy(Query * qyInput, const String * s);
    public: Query * parseOrderBy(Query * qyInput, const ATCHAR * chars);
    public: Query * parseOrderBy(Query * qyInput, const TCHAR * pszIn); //throws Exception

    const TCHAR * prepareToParse(const TCHAR * pszIn, int len = 0);

    static Operand * constructAncestorQuery(ParamInfo *info, XQLParser *parser, const TCHAR * pchOperand, NameAtoms *na,  const bool hasParens, Query * qyInput, const bool isMethod, BaseQuery::Cardinality card);
    static Operand * constructChildQuery(ParamInfo *info, XQLParser *parser, const TCHAR * pchOperand, NameAtoms *na,  const bool hasParens, Query * qyInput, const bool isMethod, BaseQuery::Cardinality card);
    static Operand * constructContextQuery(ParamInfo *info, XQLParser *parser, const TCHAR * pchOperand, NameAtoms *na,  const bool hasParens, Query * qyInput, const bool isMethod, BaseQuery::Cardinality card);
    static Operand * constructConversion(ParamInfo *info, XQLParser *parser, const TCHAR * pchOperand, NameAtoms *na,  const bool hasParens, Query * qyInput, const bool isMethod, BaseQuery::Cardinality card);
    static Operand * constructMethod(ParamInfo *info, XQLParser *parser, const TCHAR * pchOperand, NameAtoms *na,  const bool hasParens, Query * qyInput, const bool isMethod, BaseQuery::Cardinality card);
    static Operand * constructNodeQuery(ParamInfo *info, XQLParser *parser, const TCHAR * pchOperand, NameAtoms *na,  const bool hasParens, Query * qyInput, const bool isMethod, BaseQuery::Cardinality card);
    static Operand * constructNULLMethod(ParamInfo *info, XQLParser *parser, const TCHAR * pchOperand, NameAtoms *na,  const bool hasParens, Query * qyInput, const bool isMethod, BaseQuery::Cardinality card);
    static Operand * constructTextQuery(ParamInfo *info, XQLParser *parser, const TCHAR * pchOperand, NameAtoms *na,  const bool hasParens, Query * qyInput, const bool isMethod, BaseQuery::Cardinality card);
    static Operand * constructRefQuery(ParamInfo *info, XQLParser *parser, const TCHAR * pchOperand, NameAtoms *na,  const bool hasParens, Query * qyInput, const bool isMethod, BaseQuery::Cardinality card);

    static int readParameters(ParamInfo *info, 
        XQLParser *parser, const TCHAR * pchOperand,  const bool hasParens, 
        Operand **aopnd, const TCHAR ** apchOperand, const count);

    Operand * constructTreeQuery(Query * qyInput, BaseQuery::Cardinality card);

    Query * parseSortKey(SortedQuery * sqy);
    Operand * parseXPointer(Query * qyInput, BaseQuery::Cardinality card);
    Operand * parseQuery(Query * qyInput, BaseQuery::Cardinality card);
    Operand * parseTreeQuery(Query * qyInput, bool fInTreeQuery, BaseQuery::Cardinality card);
    Operand * parseChildrenQuery(Query * qyInput, bool fInTreeQuery, BaseQuery::Cardinality card);
    Operand * parseFilteredQuery(Query * qyInput, bool fInTreeQuery, BaseQuery::Cardinality card);

    Operand * parseCondition();
    Operand * parseAndExpr();
    Operand * parseRelationalExpr();
    Operand * parseOperand(BaseQuery::Cardinality card);
    Operand * parseMethod(Query * qyInput, bool isMethod, bool fInTreeQuery, BaseQuery::Cardinality card);

    /**
     * throw error
     */      
    private: void tokenError(xqlTokenType t); //throws Exception
    private: void tokenError(String * s); //throws Exception
    private: void error(String * s); //throws Exception
    private: void error(TCHAR * c); //throws Exception
    private: void throwE(ResourceID resID, 
                         const TCHAR * pchError, const TCHAR * pchErrorEnd = null,
                         const TCHAR * pchStart = null);
    private: void throwE(ResourceID resID, String * str, 
                         const TCHAR * pchError, const TCHAR * pchErrorEnd = null,
                         const TCHAR * pchStart = null);


    /**
     * get next char and update line number / char position
     */    
    private: void advance() //throws Exception
    {
        if (_lookahead != 0)
            _lookahead = *_pchNext++;
    }

    private: void advance(const TCHAR * pchNext);

    private: const TCHAR * currentPos()
    {
        return _pchNext - 1;
    }

    void skipSpace();

    /**
     * return next token
     * @exception Exception * when syntax or other error is encountered.
     */    
    private: xqlTokenType nextToken(); //throws Exception

    private: String * tokenString(xqlTokenType token);

    private: String * errorMsg(const TCHAR * msg, 
                               const TCHAR * pchError, const TCHAR * pchErrorEnd = null,
                               const TCHAR * pchStart = null);
    private: String * errorMsg(String * msg, 
                               const TCHAR * pchError, const TCHAR * pchErrorEnd = null,
                               const TCHAR * pchStart = null);


    private: Query * getQuery(Operand * opnd, const TCHAR * pchOperand, Query * qyInput = null);    
    private: Query * getQuery(Operand * opnd, const TCHAR * pchOperand, DWORD dwFlags);
    private: void    queryInvalidHere(const TCHAR * pchOperand);

    private: bool tokenEndsTreeQuery();

    private: xqlTokenType lookup(String * str);

    /**
     * Scan a name
     */  

    private: const TCHAR * scanName(const TCHAR * pchToken, int len, NameAtoms * na);

    /**
     * Scan a $keyword$
     */  
    private: void scanKeyword();


    /**
     * Scan a quoted string
     *
     */    
    private: void scanString(); //throws Exception

    /**
     * Scan a number
     *
     */    

    private: void scanNumber(); //throws Exception

    
    /**
     * expect token type t
     */    
    private: void checkToken(xqlTokenType t); //throws Exception
    
    bool    _shouldAddRef;

    /**
     * next character
     */    
    TCHAR _lookahead;
    
    /**
     * input string to parse
     */    
    const TCHAR * _pszXQL;

    /**
     * input length
     */    
    int _len;
    
    /**
     * next char in input to process
     */    

    const TCHAR * _pchNext;

    /**
     * pointer to beginning of token
     */    
    const TCHAR * _pchToken;
    
    /**
     * token type
     */    
    xqlTokenType _token;

    /**
     * String if token is a quoted string
     */    

    RString _string;

    RAtom   _atomURN;

    RAtom   _atomPrefix;

    RAtom   _atomName;

    RNamespaceMgr   _nsmgr;
  
    /**
     * VARIANT if token is a number, date, etc.
     */    

    VARIANT _var;
    
    /**
     * Token matching hashtable
     */
    
    static SRString     s_strMinus;

    static SRHashtable  s_tokens;

    struct TokenInfo
    {
        xqlTokenType    _token;
        Atom **         _pAtom;
    };
    
    static TokenInfo s_tokenTable[];

    static SRAtom s_atomAttribute;
    static SRAtom s_atomID;
    static SRAtom s_atomAnd;
    static SRAtom s_atomOr;
    static SRAtom s_atomNot;


    private: static const TCHAR * tokenChars(xqlTokenType token);

    enum Action 
    {
        NONE,
        QUERY,
        METHOD
    };


    enum 
    {
        MAX_ARGS = 8
    };


    static ParamInfo s_methodTable[];
    static ParamInfo s_ElementParamInfo;

    private: ParamInfo * lookupParamInfo(xqlTokenType aToken);
};

    /**
     * Scan a name
     */  
    class NameAtoms
    {
    public:
        NameAtoms() {}
        NameAtoms(Atom * atomURN, Atom * atomPrefix, Atom * atomName) {init(atomURN, atomPrefix, atomName);}
        void init(Atom * atomURN, Atom * atomPrefix, Atom * atomName) {_atomURN = atomURN; _atomPrefix = atomPrefix; _atomName = atomName;}
     
        Atom * _atomURN;
        Atom * _atomPrefix;
        Atom * _atomName;
    };

    //
    // We want to be able to construct a Operand object for
    // a particular method.  We do this using a table driven
    // approach where a table of ParamInfo is created.  When
    // we run into a token of a type that is in the table, we
    // use the info to construct the appropriate Operand object.
    struct ParamInfo
    {
        xqlTokenType _tokenType;
        int         _max_args;
        DWORD                   _argTypes[8];
        DWORD       _type;
        //MethodOperand::MethodType   _mt;    // Method type, for MethodOperand
        //Node::NodeType           _nt;    // Element node type
        _QUERY_CONSTRUCTOR  _constructor;
    };

#endif _XQL_PARSER_XQLPARSER