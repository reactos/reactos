/*
 * @(#)SchemaBuilder.hxx 1.0 8/3/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
#ifndef _SCHEMABUILDER_HXX
#define _SCHEMABUILDER_HXX

/***************************/
/***  Start of #include  ***/
#include <xmlparser.h>

#ifndef _REFERENCE_HXX
#include "core/base/_reference.hxx" 
#endif 

#ifndef _XML_DOM_NAMESPACEMGR
#include "xml/om/namespacemgr.hxx" 
#endif

#ifndef _CORE_UTIL_NAME
#include "core/util/name.hxx" 
#endif

#ifndef _CORE_UTIL_HASHTABLE
#include "core/util/hashtable.hxx"
#endif

#ifndef _CORE_DATATYPE_HXX
#include "core/util/datatype.hxx"
#endif

#ifndef _CORE_UTIL_CHARTYPE_HXX
#include "core/util/chartype.hxx"
#endif

#ifndef _CORE_LANG_STRING
#include "core/lang/string.hxx"       
#endif

#ifndef _XML_PARSER_DTD
#include "xml/dtd/dtd.hxx"  // xml/dtd
#endif

#ifndef _ELEMENTDECL_HXX
#include "xml/dtd/elementdecl.hxx" 
#endif

#ifndef _ATTDEF_HXX
#include "xml/dtd/attdef.hxx"
#endif

#ifndef _XMLNAMES_HXX
#include "xml/dtd/xmlnames.hxx"
#endif

#ifndef _RAWSTACK_HXX
#include "core/util/_rawstack.hxx" 
#endif

#ifndef _SCHEMANAMES_HXX
#include "schemanames.hxx"
#endif

typedef _reference<IXMLNodeFactory> RXMLNodeFactory;
typedef _reference<Name>            RName;
typedef _reference<DTD>             RDTD;
typedef _reference<NamespaceMgr>    RNamespaceMgr;
typedef _reference<Node>            RNode;


typedef void (SchemaBuilder::* BuildFunc)(IXMLNodeSource * pSource, Object * obj);
typedef void (SchemaBuilder::* InitFunc)(Node* pNode);
typedef void (SchemaBuilder::* BeginFunc)(IXMLNodeSource * pSource);
typedef void (SchemaBuilder::* EndFunc)(IXMLNodeSource * pSource);


typedef enum
{
    SNODE_SCHEMAROOT = 0,
    SNODE_ELEMENTTYPE, 
    SNODE_ATTRIBUTETYPE,
    SNODE_ELEMENT,
    SNODE_ATTRIBUTE,
    SNODE_GROUP,
    SNODE_ELEMENTDATATYPE,
    SNODE_ATTRIBUTEDATATYPE,
} SCHEMANODES;


typedef enum
{
    eSequence,
    eChoice,
    eMany,
} EnumOrder;

typedef struct
{
    int                    attribute;      // possible attribute names (in SchemaEnum)
    int                    nSubType;       // Attribute value types 
    BuildFunc              pfnBuilder;     // Corresponding build functions for attribute value
} StateAttributes;

// SchemaEntry controls the states of parsing a schema document
// and calls the corresponding "init", "end" and "build" functions when necessary
typedef struct
{
    int                    eName;            // the name of the object it is comparing to
    int                    iPadding;         // pointers to member functions must be 64 byte aligned.
    const int              *aNextStates;      // possible next states
    const StateAttributes  *aAttributes;
    InitFunc               pfnInit;          // takes care all the "init" functions in SchemaBuilder
    BeginFunc              pfnBC;            // takes care all the "begin" functions in SchemaBuilder for BeginChildren
    EndFunc                pfnEC;            // takes care all the "end" functions in SchemaBuilder for EndChildren
} SchemaEntry;



typedef struct GroupInfo
{
    int             nMinVal;
    int             nMaxVal;
    bool            fGotMax;
    bool            fGotMin;
    EnumOrder       cCurrentOrder;              

    static void copyGroupInfo(const GroupInfo * from, GroupInfo * to)
    {
        to->nMinVal = from->nMinVal;
        to->nMaxVal = from->nMaxVal;

        to->cCurrentOrder = from->cCurrentOrder;
    }
    static GroupInfo * copyGroupInfo(const GroupInfo * other)
    {
        GroupInfo * newGroupInfo = new GroupInfo();
        copyGroupInfo(other, newGroupInfo);

        return newGroupInfo;
    }

} GroupInfo;


typedef struct
{
    // used for <ElementType ...
    RElementDecl    pElementDecl;               // Element Information
    bool            fGotName;                   // flag = true if name attribute appears
    bool            fGotContent;                // if don't get any content, we can then set content=mixed(default)
    bool            fGotModel;
    bool            fGroupDisabled;             // disable if it is textOnly
    bool            fGotOrder;
    bool            fGotContentMixed;
    bool            fMasterGroupRequired;       // In a situation like <!ELEMENT root (e1)> e1 has to have a ()
    bool            fExistTerminal;             // when there exist a terminal, we need to addOrder before
                                                // we can add another terminal

    bool            fAllowDatatype;             // must have textOnly if we have datatype
    bool            fGotDtType;                 // got data type
    // used for <element ...
    bool            fGotType;                   // user must have a type attribute in <element ...
    int             nMinVal;
    int             nMaxVal;                    // -1 means infinity

    RHashtable      pAttDefList;                // a list of current AttDefs for the <ElementType ...
                                                // only the used one will be added
} ElementInfo;
  
  
typedef struct
{
    // used for <AttributeType ...
    RAttDef         pAttDef;

    bool            fGotName;                   // must provide attribute name for AttributeType
    bool            fRequired;                  // true:  when the attribute required="yes"
    bool            fGotDefault;

    // used to store namedef for the AttributeType
    RNameDef        pAttNamedef;

    // used for both AttributeType and attribute
    bool            fBuildDefNode;              // when encounter default, then we must build DefNode
    int             nMinVal;
    int             nMaxVal;                    // -1 means infinity

    // used for datatype 
    bool            fEnumerationRequired;       // when we have dt:value then we must have dt:type="enumeration"
    bool            fGotDtType;

    // used for <attribute ...
    bool            fGotType;
    bool            fGlobal;
    
    bool            fDefault;
    RObject         pObjDef;

    bool            fProcessingDtType;
} AttributeInfo;


class SchemaBuilder
{
public:

    SchemaBuilder(IXMLNodeFactory * fc, DTD * pDTD, NamespaceMgr * pPrevNSMgr, NamespaceMgr * pThisNSMgr, Atom * pURN, Document* pDoc);
    ~SchemaBuilder();

    void start();               // called at START_SCHEMA
    void finish();              // called at END_SCHEMA

public:

    /////////////////////////////////////////////////////////////////////////////////////
    // main public interface
    //  

    HRESULT ProcessElementNode(Node * pNode);
    HRESULT ProcessAttributes(IXMLNodeSource * pSource, Node * pNode);
    HRESULT ProcessPCDATA(Node * pNode, PVOID pParent);
    HRESULT ProcessEndChildren(IXMLNodeSource *pSource, Node * pNode);

public:
    /////////////////////////////////////////////////////////////////////////////////////
    // Notice:
    //    The following functions are really private, but to make it to the table, we have to 
    // declared them as public
    //

    // for each start element calls
    void initElementType(Node* pNode);
    void initElement(Node* pNode);
    
    void initAttributeType(Node* pNode);
    void initAttribute(Node* pNode);
    
    void initElementDatatype(Node* pNode);
    void initAttributeDatatype(Node* pNode);
    void initGroup(Node* pNode);

    // for BeginChildren Calls
    void beginElementType(IXMLNodeSource __RPC_FAR *pSource);
    void beginAttributeType(IXMLNodeSource __RPC_FAR *pSource);
    void beginGroup(IXMLNodeSource __RPC_FAR *pSource);

    // this is the ONLY "end" calls for         <ElementType dt:type
    // an attribute instead of an element       --------------------^
    void endElementDtType(IXMLNodeSource __RPC_FAR *pSource);

    // for EndChildren Calls
    void endElementType(IXMLNodeSource __RPC_FAR *pSource);
    void endElement(IXMLNodeSource __RPC_FAR *pSource);
    void endAttributeType(IXMLNodeSource __RPC_FAR *pSource);
    void endAttribute(IXMLNodeSource __RPC_FAR *pSource);
    void endAttributeDtType(IXMLNodeSource __RPC_FAR *pSource);
    void endGroup(IXMLNodeSource __RPC_FAR *pSource);


    /****************************************************************************************/
    /***  ----------------   ELEMENT & GROUP   ---------------
    /***  ELEMENTS AFFECTED:    <ElementType ; <element ; <group
    /***  FOR <ElementType                     => name, content, order, model
    /***  FOR <element                         => type, minOccurs, maxOccurs
    /***  FOR <group                           => order, minOccurs, maxOccurs  
    /****************************************************************************************/
    void buildSchemaName(IXMLNodeSource * pSource, Object * namedef)
    {};

    void buildElementName(IXMLNodeSource * pSource, Object * namedef);
    void buildElementContent(IXMLNodeSource * pSource, Object * namedef);
    void buildElementOrder(IXMLNodeSource * pSource, Object * namedef);
    void buildElementModel(IXMLNodeSource * pSource, Object * namedef);

    void buildElementType(IXMLNodeSource * pSource, Object * namedef);
    void buildElementMinOccurs(IXMLNodeSource * pSource, Object * str);
    void buildElementMaxOccurs(IXMLNodeSource * pSource, Object * str);

    void buildGroupOrder(IXMLNodeSource * pSource, Object * namedef);
    void buildGroupMinOccurs(IXMLNodeSource * pSource, Object * str);
    void buildGroupMaxOccurs(IXMLNodeSource * pSource, Object * str);

    /****************************************************************************************/
    /***  ----------------   ATTRIBUTE   -------------------
    /***  ELEMENTS AFFECTED:    <AttributeType ; <attribute
    /***  FOR <AttributeType                   => name, minOccurs, maxOccurs, required, default
    /***  FOR <attribute                       => type, required, default
    /***  
    /****************************************************************************************/
    void buildAttributeName(IXMLNodeSource * pSource, Object * namedef);
    void buildAttributeType(IXMLNodeSource * pSource, Object * namedef);
    void buildAttributeRequired(IXMLNodeSource * pSource, Object * namedef);
    void buildAttributeDefault(IXMLNodeSource * pSource, Object * str); 

    /****************************************************************************************/
    /***  ----------------   DATATYPES   -----------------
    /***  ELEMENTS AFFECTED:    <AttributeType ; <datatype ; <ElementType
    /***  FOR <AttributeType && 
    /***      <datatype under <AttributeType   =>   dt:type, dt:value
    /***  FOR <ElementType   &&
    /***      <datatype under <ElementType     =>   dt:type
    /***
    /****************************************************************************************/
    void buildElementDtType(IXMLNodeSource * pSource, Object * str);

    void buildAttributeDtType(IXMLNodeSource * pSource, Object * namedef);
    void buildAttributeDtValues(IXMLNodeSource * pSource, Object * values);

private:

    // wrapper functions for _pStateHistory
    void push();                     
    void pop(); 

    // member functions
    void setOrder(Name * name);  // record the order that the user wishes to add, but this does NOT add to DTD
    void addOrder();                // actual adding to DTD structures.
    void setAttributePresence();
    
    void addDefNode(IXMLNodeSource* pSource, const WCHAR * pwcText, ULONG ulLen);

    // calls addDefNode when conditions of adding the nodes are satisfied.
    void tryToAddDefNode(IXMLNodeSource* pSource);
        
    // used by AttributeType && datatype under AttributeType
    // to see if the given dt:type is one of the NMTOKEN,ENUMERATION ...
    DataType checkDtType(String * name);
    
    HRESULT checkEntity(Entity* en);
    
    // support for multi-level of groups
    void pushGroupInfo();
    void popGroupInfo();
        
    // values="&foo;"
    HRESULT expandEntity(IXMLNodeSource* pSource, Name* name);    

    bool getNextState(Name *name);
    bool isSkipableElement(Name * pname);
    bool isSkipableAttribute(Name * pname);

    void AttributeException(const WCHAR * pwcMsg1, const WCHAR * pwcMsg2);

private:

    SchemaEntry *   _pState;
    SchemaEntry *   _pNextState;
    _rawstack<SchemaEntry *> _pStateHistory;  // the sequence of the stack is as follows
                                              // a begin state for all available elements or attributes
                                              // followed by a matched state. When _fSkip we only keep begin state   

    _rawstack<GroupInfo *> _pGroupHistory;       // used for multi-group support

    RNode _pSkipNode;
    bool _fGotRoot;
    bool _fSkip;
    bool _fValidating;

    RXMLNodeFactory _pFactory;
    RDTD            _pDTD;
    RNamespaceMgr   _pPrevNSMgr;
    RNamespaceMgr   _pThisNSMgr;

    WDocument       _pDoc;
    RAtom           _pURN;
    
    ElementInfo _ElementInfo;
    AttributeInfo _AttributeInfo;
    GroupInfo _GroupInfo;
};

#endif