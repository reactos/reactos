/*
 * schemas.c : implementation of the XML Schema handling and
 *             schema validity checking
 *
 * See Copyright for the status of this software.
 *
 * Daniel Veillard <veillard@redhat.com>
 */

/*
 * TODO:
 *   - when types are redefined in includes, check that all
 *     types in the redef list are equal
 *     -> need a type equality operation.
 *   - if we don't intend to use the schema for schemas, we
 *     need to validate all schema attributes (ref, type, name)
 *     against their types.
 *   - Eliminate item creation for: ??
 *
 * NOTES:
 *   - Elimated item creation for: <restriction>, <extension>,
 *     <simpleContent>, <complexContent>, <list>, <union>
 *
 */
#define IN_LIBXML
#include "libxml.h"

#ifdef LIBXML_SCHEMAS_ENABLED

#include <string.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/hash.h>
#include <libxml/uri.h>
#include <libxml/xmlschemas.h>
#include <libxml/schemasInternals.h>
#include <libxml/xmlschemastypes.h>
#include <libxml/xmlautomata.h>
#include <libxml/xmlregexp.h>
#include <libxml/dict.h>
#include <libxml/encoding.h>
#include <libxml/xmlIO.h>
#ifdef LIBXML_PATTERN_ENABLED
#include <libxml/pattern.h>
#endif
#ifdef LIBXML_READER_ENABLED
#include <libxml/xmlreader.h>
#endif

/* #define DEBUG 1 */

/* #define DEBUG_CONTENT 1 */

/* #define DEBUG_TYPE 1 */

/* #define DEBUG_CONTENT_REGEXP 1 */

/* #define DEBUG_AUTOMATA 1 */

#define DEBUG_ATTR_VALIDATION 0

/* #define DEBUG_IDC 1 */

/* #define DEBUG_INCLUDES 1 */

/* #define ENABLE_PARTICLE_RESTRICTION 1 */

#define DUMP_CONTENT_MODEL

#define XML_SCHEMA_SAX_ENABLED

#ifdef LIBXML_READER_ENABLED
/* #define XML_SCHEMA_READER_ENABLED */
#endif

#define UNBOUNDED (1 << 30)
#define TODO 								\
    xmlGenericError(xmlGenericErrorContext,				\
	    "Unimplemented block at %s:%d\n",				\
            __FILE__, __LINE__);

#define XML_SCHEMAS_NO_NAMESPACE (const xmlChar *) "##"

/*
 * The XML Schemas namespaces
 */
static const xmlChar *xmlSchemaNs = (const xmlChar *)
    "http://www.w3.org/2001/XMLSchema";

static const xmlChar *xmlSchemaInstanceNs = (const xmlChar *)
    "http://www.w3.org/2001/XMLSchema-instance";

static const xmlChar *xmlNamespaceNs = (const xmlChar *)
    "http://www.w3.org/2000/xmlns/";

static const xmlChar *xmlSchemaElemDesElemDecl = (const xmlChar *)
    "element decl.";
static const xmlChar *xmlSchemaElemDesAttrDecl = (const xmlChar *)
    "attribute decl.";
static const xmlChar *xmlSchemaElemDesAttrRef = (const xmlChar *)
    "attribute use";
static const xmlChar *xmlSchemaElemDesCT = (const xmlChar *)
    "complex type";
static const xmlChar *xmlSchemaElemModelGrDef = (const xmlChar *)
    "model group";
#if 0
static const xmlChar *xmlSchemaElemModelGrRef = (const xmlChar *)
    "model group ref.";
#endif

#define IS_SCHEMA(node, type)						\
   ((node != NULL) && (node->ns != NULL) &&				\
    (xmlStrEqual(node->name, (const xmlChar *) type)) &&		\
    (xmlStrEqual(node->ns->href, xmlSchemaNs)))

#define FREE_AND_NULL(str)						\
    if (str != NULL) {							\
	xmlFree((xmlChar *) str);							\
	str = NULL;							\
    }

#define IS_ANYTYPE(item)                           \
     ((item->type == XML_SCHEMA_TYPE_BASIC) &&     \
      (item->builtInType == XML_SCHEMAS_ANYTYPE))

#define IS_COMPLEX_TYPE(item)                      \
    ((item->type == XML_SCHEMA_TYPE_COMPLEX) ||    \
     (item->builtInType == XML_SCHEMAS_ANYTYPE))

#define IS_SIMPLE_TYPE(item)                       \
    ((item->type == XML_SCHEMA_TYPE_SIMPLE) ||     \
     ((item->type == XML_SCHEMA_TYPE_BASIC) &&     \
      (item->builtInType != XML_SCHEMAS_ANYTYPE)))

#define IS_ANY_SIMPLE_TYPE(item)                   \
    ((item->type == XML_SCHEMA_TYPE_BASIC) &&      \
      (item->builtInType == XML_SCHEMAS_ANYSIMPLETYPE))

#define IS_NOT_TYPEFIXED(item)                      \
    ((item->type != XML_SCHEMA_TYPE_BASIC) &&       \
     ((item->flags & XML_SCHEMAS_TYPE_INTERNAL_RESOLVED) == 0))

#define HAS_COMPLEX_CONTENT(item)			 \
    ((item->contentType == XML_SCHEMA_CONTENT_MIXED) ||  \
     (item->contentType == XML_SCHEMA_CONTENT_EMPTY) ||  \
     (item->contentType == XML_SCHEMA_CONTENT_ELEMENTS))

#define HAS_SIMPLE_CONTENT(item)			 \
    ((item->contentType == XML_SCHEMA_CONTENT_SIMPLE) ||  \
     (item->contentType == XML_SCHEMA_CONTENT_BASIC))

#define HAS_MIXED_CONTENT(item)	(item->contentType == XML_SCHEMA_CONTENT_MIXED)

#define IS_PARTICLE_EMPTIABLE(item) \
    (xmlSchemaIsParticleEmptiable((xmlSchemaParticlePtr) item->subtypes))

#define GET_NODE(item) xmlSchemaGetComponentNode((xmlSchemaBasicItemPtr) item)

#define GET_LIST_ITEM_TYPE(item) item->subtypes

#define VARIETY_ATOMIC(item) (item->flags & XML_SCHEMAS_TYPE_VARIETY_ATOMIC)
#define VARIETY_LIST(item) (item->flags & XML_SCHEMAS_TYPE_VARIETY_LIST)
#define VARIETY_UNION(item) (item->flags & XML_SCHEMAS_TYPE_VARIETY_UNION)

#define IS_MODEL_GROUP(item)                     \
    ((item->type == XML_SCHEMA_TYPE_SEQUENCE) || \
     (item->type == XML_SCHEMA_TYPE_CHOICE) ||   \
     (item->type == XML_SCHEMA_TYPE_ALL))

#define INODE_NILLED(item) (item->flags & XML_SCHEMA_ELEM_INFO_NILLED)

#define ELEM_TYPE(item) item->subtypes

#define GET_PARTICLE(item) (xmlSchemaParticlePtr) item->subtypes;

#define SUBST_GROUP_AFF(item) item->refDecl

#if 0
#define WXS_GET_NEXT(item) xmlSchemaGetNextComponent((xmlSchemaBasicItemPtr) item)
#endif

#define SUBSET_RESTRICTION  1<<0
#define SUBSET_EXTENSION    1<<1
#define SUBSET_SUBSTITUTION 1<<2
#define SUBSET_LIST         1<<3
#define SUBSET_UNION        1<<4

#define XML_SCHEMAS_PARSE_ERROR		1

#define SCHEMAS_PARSE_OPTIONS XML_PARSE_NOENT

typedef struct _xmlSchemaNodeInfo xmlSchemaNodeInfo;
typedef xmlSchemaNodeInfo *xmlSchemaNodeInfoPtr;


typedef struct _xmlSchemaItemList xmlSchemaAssemble;
typedef xmlSchemaAssemble *xmlSchemaAssemblePtr;

typedef struct _xmlSchemaItemList xmlSchemaItemList;
typedef xmlSchemaItemList *xmlSchemaItemListPtr;

struct _xmlSchemaItemList {
    void **items;  /* used for dynamic addition of schemata */
    int nbItems; /* used for dynamic addition of schemata */
    int sizeItems; /* used for dynamic addition of schemata */
};

typedef struct _xmlSchemaAbstractCtxt xmlSchemaAbstractCtxt;
typedef xmlSchemaAbstractCtxt *xmlSchemaAbstractCtxtPtr;
struct _xmlSchemaAbstractCtxt {
    int type;
};

#define XML_SCHEMA_CTXT_PARSER 1
#define XML_SCHEMA_CTXT_VALIDATOR 2

struct _xmlSchemaParserCtxt {
    int type;
    void *userData;             /* user specific data block */
    xmlSchemaValidityErrorFunc error;   /* the callback in case of errors */
    xmlSchemaValidityWarningFunc warning;       /* the callback in case of warning */
    xmlSchemaValidError err;
    int nberrors;
    xmlStructuredErrorFunc serror;

    xmlSchemaPtr topschema;	/* The main schema */
    xmlHashTablePtr namespaces;	/* Hash table of namespaces to schemas */

    xmlSchemaPtr schema;        /* The schema in use */
    const xmlChar *container;   /* the current element, group, ... */
    int counter;

    const xmlChar *URL;
    xmlDocPtr doc;
    int preserve;		/* Whether the doc should be freed  */

    const char *buffer;
    int size;

    /*
     * Used to build complex element content models
     */
    xmlAutomataPtr am;
    xmlAutomataStatePtr start;
    xmlAutomataStatePtr end;
    xmlAutomataStatePtr state;

    xmlDictPtr dict;		/* dictionnary for interned string names */
    int        includes;	/* the inclusion level, 0 for root or imports */
    xmlSchemaTypePtr ctxtType; /* The current context simple/complex type */
    xmlSchemaTypePtr parentItem; /* The current parent schema item */
    xmlSchemaAssemblePtr assemble;
    int options;
    xmlSchemaValidCtxtPtr vctxt;
    const xmlChar **localImports; /* list of locally imported namespaces */
    int sizeLocalImports;
    int nbLocalImports;
    xmlHashTablePtr substGroups;
    int isS4S;
};

#define XML_SCHEMAS_ATTR_UNKNOWN 1
#define XML_SCHEMAS_ATTR_ASSESSED 2
#define XML_SCHEMAS_ATTR_PROHIBITED 3
#define XML_SCHEMAS_ATTR_ERR_MISSING 4
#define XML_SCHEMAS_ATTR_INVALID_VALUE 5
#define XML_SCHEMAS_ATTR_ERR_NO_TYPE 6
#define XML_SCHEMAS_ATTR_ERR_FIXED_VALUE 7
#define XML_SCHEMAS_ATTR_DEFAULT 8
#define XML_SCHEMAS_ATTR_VALIDATE_VALUE 9
#define XML_SCHEMAS_ATTR_ERR_WILD_STRICT_NO_DECL 10
#define XML_SCHEMAS_ATTR_HAS_ATTR_USE 11
#define XML_SCHEMAS_ATTR_HAS_ATTR_DECL 12
#define XML_SCHEMAS_ATTR_WILD_SKIP 13
#define XML_SCHEMAS_ATTR_WILD_LAX_NO_DECL 14
#define XML_SCHEMAS_ATTR_ERR_WILD_DUPLICATE_ID 15
#define XML_SCHEMAS_ATTR_ERR_WILD_AND_USE_ID 16
#define XML_SCHEMAS_ATTR_META 17

/**
 * xmlSchemaBasicItem:
 *
 * The abstract base type for schema components.
 */
typedef struct _xmlSchemaBasicItem xmlSchemaBasicItem;
typedef xmlSchemaBasicItem *xmlSchemaBasicItemPtr;
struct _xmlSchemaBasicItem {
    xmlSchemaTypeType type;
};

/**
 * xmlSchemaAnnotItem:
 *
 * The abstract base type for annotated schema components.
 * (Extends xmlSchemaBasicItem)
 */
typedef struct _xmlSchemaAnnotItem xmlSchemaAnnotItem;
typedef xmlSchemaAnnotItem *xmlSchemaAnnotItemPtr;
struct _xmlSchemaAnnotItem {
    xmlSchemaTypeType type;
    xmlSchemaAnnotPtr annot;
};

/**
 * xmlSchemaTreeItem:
 *
 * The abstract base type for tree-like structured schema components.
 * (Extends xmlSchemaAnnotItem)
 */
typedef struct _xmlSchemaTreeItem xmlSchemaTreeItem;
typedef xmlSchemaTreeItem *xmlSchemaTreeItemPtr;
struct _xmlSchemaTreeItem {
    xmlSchemaTypeType type;
    xmlSchemaAnnotPtr annot;
    xmlSchemaTreeItemPtr next;
    xmlSchemaTreeItemPtr children;
};

/**
 * xmlSchemaQNameRef:
 *
 * A component reference item (not a schema component)
 * (Extends xmlSchemaBasicItem)
 */
typedef struct _xmlSchemaQNameRef xmlSchemaQNameRef;
typedef xmlSchemaQNameRef *xmlSchemaQNameRefPtr;
struct _xmlSchemaQNameRef {
    xmlSchemaTypeType type;
    xmlSchemaBasicItemPtr item;
    xmlSchemaTypeType itemType;
    const xmlChar *name;
    const xmlChar *targetNamespace;
};

/**
 * xmlSchemaParticle:
 *
 * A particle component.
 * (Extends xmlSchemaTreeItem)
 */
typedef struct _xmlSchemaParticle xmlSchemaParticle;
typedef xmlSchemaParticle *xmlSchemaParticlePtr;
struct _xmlSchemaParticle {
    xmlSchemaTypeType type;
    xmlSchemaAnnotPtr annot;
    xmlSchemaTreeItemPtr next; /* next particle (OR "element decl" OR "wildcard") */
    xmlSchemaTreeItemPtr children; /* the "term" ("model group" OR "group definition") */
    int minOccurs;
    int maxOccurs;
    xmlNodePtr node;
};

/**
 * xmlSchemaModelGroup:
 *
 * A model group component.
 * (Extends xmlSchemaTreeItem)
 */
typedef struct _xmlSchemaModelGroup xmlSchemaModelGroup;
typedef xmlSchemaModelGroup *xmlSchemaModelGroupPtr;
struct _xmlSchemaModelGroup {
    xmlSchemaTypeType type; /* XML_SCHEMA_TYPE_SEQUENCE, XML_SCHEMA_TYPE_CHOICE, XML_SCHEMA_TYPE_ALL */
    xmlSchemaAnnotPtr annot;
    xmlSchemaTreeItemPtr next; /* not used */
    xmlSchemaTreeItemPtr children; /* first particle (OR "element decl" OR "wildcard") */
    xmlNodePtr node;
};

#define XML_SCHEMA_MODEL_GROUP_DEF_MARKED 1<<0
/**
 * xmlSchemaModelGroupDef:
 *
 * A model group definition component.
 * (Extends xmlSchemaTreeItem)
 */
typedef struct _xmlSchemaModelGroupDef xmlSchemaModelGroupDef;
typedef xmlSchemaModelGroupDef *xmlSchemaModelGroupDefPtr;
struct _xmlSchemaModelGroupDef {
    xmlSchemaTypeType type; /* XML_SCHEMA_TYPE_GROUP */
    xmlSchemaAnnotPtr annot;
    xmlSchemaTreeItemPtr next; /* not used */
    xmlSchemaTreeItemPtr children; /* the "model group" */
    const xmlChar *name;
    const xmlChar *targetNamespace;
    xmlNodePtr node;
    int flags;
};

typedef struct _xmlSchemaIDC xmlSchemaIDC;
typedef xmlSchemaIDC *xmlSchemaIDCPtr;

/**
 * xmlSchemaIDCSelect:
 *
 * The identity-constraint "field" and "selector" item, holding the
 * XPath expression.
 */
typedef struct _xmlSchemaIDCSelect xmlSchemaIDCSelect;
typedef xmlSchemaIDCSelect *xmlSchemaIDCSelectPtr;
struct _xmlSchemaIDCSelect {
    xmlSchemaIDCSelectPtr next;
    xmlSchemaIDCPtr idc;
    int index; /* an index position if significant for IDC key-sequences */
    const xmlChar *xpath; /* the XPath expression */
    void *xpathComp; /* the compiled XPath expression */
};

/**
 * xmlSchemaIDC:
 *
 * The identity-constraint definition component.
 * (Extends xmlSchemaAnnotItem)
 */

struct _xmlSchemaIDC {
    xmlSchemaTypeType type;
    xmlSchemaAnnotPtr annot;
    xmlSchemaIDCPtr next;
    xmlNodePtr node;
    const xmlChar *name;
    const xmlChar *targetNamespace;
    xmlSchemaIDCSelectPtr selector;
    xmlSchemaIDCSelectPtr fields;
    int nbFields;
    xmlSchemaQNameRefPtr ref;
};

/**
 * xmlSchemaIDCAug:
 *
 * The augmented IDC information used for validation.
 */
typedef struct _xmlSchemaIDCAug xmlSchemaIDCAug;
typedef xmlSchemaIDCAug *xmlSchemaIDCAugPtr;
struct _xmlSchemaIDCAug {
    xmlSchemaIDCAugPtr next; /* next in a list */
    xmlSchemaIDCPtr def; /* the IDC definition */
    int bubbleDepth; /* the lowest tree level to which IDC
                        tables need to be bubbled upwards */
};

/**
 * xmlSchemaPSVIIDCKeySequence:
 *
 * The key sequence of a node table item.
 */
typedef struct _xmlSchemaPSVIIDCKey xmlSchemaPSVIIDCKey;
typedef xmlSchemaPSVIIDCKey *xmlSchemaPSVIIDCKeyPtr;
struct _xmlSchemaPSVIIDCKey {
    xmlSchemaTypePtr type;
    xmlSchemaValPtr val;
};

/**
 * xmlSchemaPSVIIDCNode:
 *
 * The node table item of a node table.
 */
typedef struct _xmlSchemaPSVIIDCNode xmlSchemaPSVIIDCNode;
typedef xmlSchemaPSVIIDCNode *xmlSchemaPSVIIDCNodePtr;
struct _xmlSchemaPSVIIDCNode {
    xmlNodePtr node;
    xmlSchemaPSVIIDCKeyPtr *keys;
};

/**
 * xmlSchemaPSVIIDCBinding:
 *
 * The identity-constraint binding item of the [identity-constraint table].
 */
typedef struct _xmlSchemaPSVIIDCBinding xmlSchemaPSVIIDCBinding;
typedef xmlSchemaPSVIIDCBinding *xmlSchemaPSVIIDCBindingPtr;
struct _xmlSchemaPSVIIDCBinding {
    xmlSchemaPSVIIDCBindingPtr next; /* next binding of a specific node */
    xmlSchemaIDCPtr definition; /* the IDC definition */
    xmlSchemaPSVIIDCNodePtr *nodeTable; /* array of key-sequences */
    int nbNodes; /* number of entries in the node table */
    int sizeNodes; /* size of the node table */
    int nbDupls; /* number of already identified duplicates in the node
                    table */
    /* int nbKeys; number of keys in each key-sequence */
};

#define XPATH_STATE_OBJ_TYPE_IDC_SELECTOR 1
#define XPATH_STATE_OBJ_TYPE_IDC_FIELD 2

#define XPATH_STATE_OBJ_MATCHES -2
#define XPATH_STATE_OBJ_BLOCKED -3

typedef struct _xmlSchemaIDCMatcher xmlSchemaIDCMatcher;
typedef xmlSchemaIDCMatcher *xmlSchemaIDCMatcherPtr;

/**
 * xmlSchemaIDCStateObj:
 *
 * The state object used to evaluate XPath expressions.
 */
typedef struct _xmlSchemaIDCStateObj xmlSchemaIDCStateObj;
typedef xmlSchemaIDCStateObj *xmlSchemaIDCStateObjPtr;
struct _xmlSchemaIDCStateObj {
    int type;
    xmlSchemaIDCStateObjPtr next; /* next if in a list */
    int depth; /* depth of creation */
    int *history; /* list of (depth, state-id) tuples */
    int nbHistory;
    int sizeHistory;
    xmlSchemaIDCMatcherPtr matcher; /* the correspondent field/selector
                                       matcher */
    xmlSchemaIDCSelectPtr sel;
    void *xpathCtxt;
};

#define IDC_MATCHER 0

/**
 * xmlSchemaIDCMatcher:
 *
 * Used to  IDC selectors (and fields) successively.
 */
struct _xmlSchemaIDCMatcher {
    int type;
    int depth; /* the tree depth at creation time */
    xmlSchemaIDCMatcherPtr next; /* next in the list */
    xmlSchemaIDCAugPtr aidc; /* the augmented IDC item */
    xmlSchemaPSVIIDCKeyPtr **keySeqs; /* the key-sequences of the target
                                         elements */
    int sizeKeySeqs;
    int targetDepth;
};

/*
* Element info flags.
*/
#define XML_SCHEMA_NODE_INFO_FLAG_OWNED_NAMES  1<<0
#define XML_SCHEMA_NODE_INFO_FLAG_OWNED_VALUES 1<<1
#define XML_SCHEMA_ELEM_INFO_NILLED	       1<<2
#define XML_SCHEMA_ELEM_INFO_LOCAL_TYPE	       1<<3

#define XML_SCHEMA_NODE_INFO_VALUE_NEEDED      1<<4
#define XML_SCHEMA_ELEM_INFO_EMPTY             1<<5
#define XML_SCHEMA_ELEM_INFO_HAS_CONTENT       1<<6

#define XML_SCHEMA_ELEM_INFO_HAS_ELEM_CONTENT  1<<7
#define XML_SCHEMA_ELEM_INFO_ERR_BAD_CONTENT  1<<8
#define XML_SCHEMA_NODE_INFO_ERR_NOT_EXPECTED  1<<9
#define XML_SCHEMA_NODE_INFO_ERR_BAD_TYPE  1<<10

/**
 * xmlSchemaNodeInfo:
 *
 * Holds information of an element node.
 */
struct _xmlSchemaNodeInfo {
    xmlNodePtr node;
    int nodeType;
    const xmlChar *localName;
    const xmlChar *nsName;
    const xmlChar *value;
    xmlSchemaValPtr val; /* the pre-computed value if any */
    xmlSchemaTypePtr typeDef; /* the complex/simple type definition if any */
    int flags; /* combination of node info flags */
    int valNeeded;
    int normVal;

    xmlSchemaElementPtr decl; /* the element/attribute declaration */
    int depth;
    xmlSchemaPSVIIDCBindingPtr idcTable; /* the table of PSVI IDC bindings
                                            for the scope element*/
    xmlSchemaIDCMatcherPtr idcMatchers; /* the IDC matchers for the scope
                                           element */
    xmlRegExecCtxtPtr regexCtxt;

    const xmlChar **nsBindings; /* Namespace bindings on this element */
    int nbNsBindings;
    int sizeNsBindings;
};

/*
* @metaType values of xmlSchemaAttrInfo.
*/
#define XML_SCHEMA_ATTR_INFO_META_XSI_TYPE 1
#define XML_SCHEMA_ATTR_INFO_META_XSI_NIL 2
#define XML_SCHEMA_ATTR_INFO_META_XSI_SCHEMA_LOC 3
#define XML_SCHEMA_ATTR_INFO_META_XSI_NO_NS_SCHEMA_LOC 4
#define XML_SCHEMA_ATTR_INFO_META_XMLNS 5

typedef struct _xmlSchemaAttrInfo xmlSchemaAttrInfo;
typedef xmlSchemaAttrInfo *xmlSchemaAttrInfoPtr;
struct _xmlSchemaAttrInfo {
    xmlNodePtr node;
    int nodeType;
    const xmlChar *localName;
    const xmlChar *nsName;
    const xmlChar *value;
    xmlSchemaValPtr val; /* the pre-computed value if any */
    xmlSchemaTypePtr typeDef; /* the complex/simple type definition if any */
    int flags; /* combination of node info flags */

    xmlSchemaAttributePtr decl; /* the attribute declaration */
    xmlSchemaAttributePtr use;  /* the attribute use */
    int state;
    int metaType;
    const xmlChar *vcValue; /* the value constraint value */
    xmlSchemaNodeInfoPtr parent;
};


#define XML_SCHEMA_VALID_CTXT_FLAG_STREAM 1
/**
 * xmlSchemaValidCtxt:
 *
 * A Schemas validation context
 */
struct _xmlSchemaValidCtxt {
    int type;
    void *userData;             /* user specific data block */
    xmlSchemaValidityErrorFunc error;   /* the callback in case of errors */
    xmlSchemaValidityWarningFunc warning; /* the callback in case of warning */
    xmlStructuredErrorFunc serror;

    xmlSchemaPtr schema;        /* The schema in use */
    xmlDocPtr doc;
    xmlParserInputBufferPtr input;
    xmlCharEncoding enc;
    xmlSAXHandlerPtr sax;
    xmlParserCtxtPtr parserCtxt;
    void *user_data;

    int err;
    int nberrors;

    xmlNodePtr node;
    xmlNodePtr cur;
    /* xmlSchemaTypePtr type; */

    xmlRegExecCtxtPtr regexp;
    xmlSchemaValPtr value;

    int valueWS;
    int options;
    xmlNodePtr validationRoot;
    xmlSchemaParserCtxtPtr pctxt;
    int xsiAssemble;

    int depth;
    xmlSchemaNodeInfoPtr *elemInfos; /* array of element informations */
    int sizeElemInfos;
    xmlSchemaNodeInfoPtr inode; /* the current element information */

    xmlSchemaIDCAugPtr aidcs; /* a list of augmented IDC informations */

    xmlSchemaIDCStateObjPtr xpathStates; /* first active state object. */
    xmlSchemaIDCStateObjPtr xpathStatePool; /* first stored state object. */

    xmlSchemaPSVIIDCNodePtr *idcNodes; /* list of all IDC node-table entries*/
    int nbIdcNodes;
    int sizeIdcNodes;

    xmlSchemaPSVIIDCKeyPtr *idcKeys; /* list of all IDC node-table entries */
    int nbIdcKeys;
    int sizeIdcKeys;

    int flags;

    xmlDictPtr dict;

#ifdef LIBXML_READER_ENABLED
    xmlTextReaderPtr reader;
#endif

    xmlSchemaAttrInfoPtr *attrInfos;
    int nbAttrInfos;
    int sizeAttrInfos;

    int skipDepth;
};

/*
 * These are the entries in the schemas importSchemas hash table
 */
typedef struct _xmlSchemaImport xmlSchemaImport;
typedef xmlSchemaImport *xmlSchemaImportPtr;
struct _xmlSchemaImport {
    const xmlChar *schemaLocation;
    xmlSchemaPtr schema; /* not used any more */
    xmlDocPtr doc;
    int isMain;
};

/*
 * These are the entries associated to includes in a schemas
 */
typedef struct _xmlSchemaInclude xmlSchemaInclude;
typedef xmlSchemaInclude *xmlSchemaIncludePtr;
struct _xmlSchemaInclude {
    xmlSchemaIncludePtr next;
    const xmlChar *schemaLocation;
    xmlDocPtr doc;
    const xmlChar *origTargetNamespace;
    const xmlChar *targetNamespace;
};

/**
 * xmlSchemaSubstGroup:
 *
 *
 */
typedef struct _xmlSchemaSubstGroup xmlSchemaSubstGroup;
typedef xmlSchemaSubstGroup *xmlSchemaSubstGroupPtr;
struct _xmlSchemaSubstGroup {
    xmlSchemaElementPtr head;
    xmlSchemaItemListPtr members;
};

/************************************************************************
 * 									*
 * 			Some predeclarations				*
 * 									*
 ************************************************************************/

static int xmlSchemaParseInclude(xmlSchemaParserCtxtPtr ctxt,
                                 xmlSchemaPtr schema,
                                 xmlNodePtr node);
static void
xmlSchemaTypeFixup(xmlSchemaTypePtr typeDecl,
                   xmlSchemaParserCtxtPtr ctxt, const xmlChar * name);
static const xmlChar *
xmlSchemaFacetTypeToString(xmlSchemaTypeType type);
static int
xmlSchemaParseImport(xmlSchemaParserCtxtPtr ctxt, xmlSchemaPtr schema,
                     xmlNodePtr node);
static void
xmlSchemaCheckFacetValues(xmlSchemaTypePtr typeDecl,
                       xmlSchemaParserCtxtPtr ctxt);
static void
xmlSchemaClearValidCtxt(xmlSchemaValidCtxtPtr vctxt);
static xmlSchemaWhitespaceValueType
xmlSchemaGetWhiteSpaceFacetValue(xmlSchemaTypePtr type);
static xmlSchemaTreeItemPtr
xmlSchemaParseModelGroup(xmlSchemaParserCtxtPtr ctxt, xmlSchemaPtr schema,
			 xmlNodePtr node, xmlSchemaTypeType type,
			 int withParticle);
static const xmlChar *
xmlSchemaCompTypeToString(xmlSchemaTypeType type);
static xmlSchemaTypeLinkPtr
xmlSchemaGetUnionSimpleTypeMemberTypes(xmlSchemaTypePtr type);
static void
xmlSchemaInternalErr(xmlSchemaAbstractCtxtPtr actxt,
		     const char *funcName,
		     const char *message);
static int
xmlSchemaCheckCOSSTDerivedOK(xmlSchemaTypePtr type,
			     xmlSchemaTypePtr baseType,
			     int subset);
static void
xmlSchemaCheckElementDeclComponent(xmlSchemaElementPtr elemDecl,
				   xmlSchemaParserCtxtPtr ctxt,
				   const xmlChar * name ATTRIBUTE_UNUSED);

/************************************************************************
 *									*
 * 			Helper functions			        *
 *									*
 ************************************************************************/

/**
 * xmlSchemaCompTypeToString:
 * @type: the type of the schema item
 *
 * Returns the component name of a schema item.
 */
static const xmlChar *
xmlSchemaCompTypeToString(xmlSchemaTypeType type)
{
    switch (type) {
	case XML_SCHEMA_TYPE_SIMPLE:
	    return(BAD_CAST "simple type definition");
	case XML_SCHEMA_TYPE_COMPLEX:
	    return(BAD_CAST "complex type definition");
	case XML_SCHEMA_TYPE_ELEMENT:
	    return(BAD_CAST "element declaration");
	case XML_SCHEMA_TYPE_ATTRIBUTE:
	    return(BAD_CAST "attribute declaration");
	case XML_SCHEMA_TYPE_GROUP:
	    return(BAD_CAST "model group definition");
	case XML_SCHEMA_TYPE_ATTRIBUTEGROUP:
	    return(BAD_CAST "attribute group definition");
	case XML_SCHEMA_TYPE_NOTATION:
	    return(BAD_CAST "notation declaration");
	case XML_SCHEMA_TYPE_SEQUENCE:
	    return(BAD_CAST "model group (sequence)");
	case XML_SCHEMA_TYPE_CHOICE:
	    return(BAD_CAST "model group (choice)");
	case XML_SCHEMA_TYPE_ALL:
	    return(BAD_CAST "model group (all)");
	case XML_SCHEMA_TYPE_PARTICLE:
	    return(BAD_CAST "particle");
	default:
	    return(BAD_CAST "Not a schema component");
    }
}

/**
 * xmlSchemaGetComponentNode:
 * @item: a schema component
 *
 * Returns node associated with the schema component.
 * NOTE that such a node need not be available; plus, a component's
 * node need not to reflect the component directly, since there is no
 * one-to-one relationship between the XML Schema representation and
 * the component representation.
 */
static xmlNodePtr
xmlSchemaGetComponentNode(xmlSchemaBasicItemPtr item)
{
    switch (item->type) {
	case XML_SCHEMA_TYPE_ELEMENT:
	    return (((xmlSchemaElementPtr) item)->node);
	case XML_SCHEMA_TYPE_ATTRIBUTE:
	    return (((xmlSchemaAttributePtr) item)->node);
	case XML_SCHEMA_TYPE_COMPLEX:
	case XML_SCHEMA_TYPE_SIMPLE:
	    return (((xmlSchemaTypePtr) item)->node);
	case XML_SCHEMA_TYPE_ANY:
	case XML_SCHEMA_TYPE_ANY_ATTRIBUTE:
	    return (((xmlSchemaWildcardPtr) item)->node);
	case XML_SCHEMA_TYPE_PARTICLE:
	    return (((xmlSchemaParticlePtr) item)->node);
	case XML_SCHEMA_TYPE_SEQUENCE:
	case XML_SCHEMA_TYPE_CHOICE:
	case XML_SCHEMA_TYPE_ALL:
	    return (((xmlSchemaModelGroupPtr) item)->node);
	case XML_SCHEMA_TYPE_GROUP:
	    return (((xmlSchemaModelGroupDefPtr) item)->node);
	case XML_SCHEMA_TYPE_ATTRIBUTEGROUP:
	    return (((xmlSchemaAttributeGroupPtr) item)->node);
	case XML_SCHEMA_TYPE_IDC_UNIQUE:
	case XML_SCHEMA_TYPE_IDC_KEY:
	case XML_SCHEMA_TYPE_IDC_KEYREF:
	    return (((xmlSchemaIDCPtr) item)->node);
	default:
	    return (NULL);
    }
}

#if 0
/**
 * xmlSchemaGetNextComponent:
 * @item: a schema component
 *
 * Returns the next sibling of the schema component.
 */
static xmlSchemaBasicItemPtr
xmlSchemaGetNextComponent(xmlSchemaBasicItemPtr item)
{
    switch (item->type) {
	case XML_SCHEMA_TYPE_ELEMENT:
	    return ((xmlSchemaBasicItemPtr) ((xmlSchemaElementPtr) item)->next);
	case XML_SCHEMA_TYPE_ATTRIBUTE:
	    return ((xmlSchemaBasicItemPtr) ((xmlSchemaAttributePtr) item)->next);
	case XML_SCHEMA_TYPE_COMPLEX:
	case XML_SCHEMA_TYPE_SIMPLE:
	    return ((xmlSchemaBasicItemPtr) ((xmlSchemaTypePtr) item)->next);
	case XML_SCHEMA_TYPE_ANY:
	case XML_SCHEMA_TYPE_ANY_ATTRIBUTE:
	    return (NULL);
	case XML_SCHEMA_TYPE_PARTICLE:
	    return ((xmlSchemaBasicItemPtr) ((xmlSchemaParticlePtr) item)->next);
	case XML_SCHEMA_TYPE_SEQUENCE:
	case XML_SCHEMA_TYPE_CHOICE:
	case XML_SCHEMA_TYPE_ALL:
	    return (NULL);
	case XML_SCHEMA_TYPE_GROUP:
	    return (NULL);
	case XML_SCHEMA_TYPE_ATTRIBUTEGROUP:
	    return ((xmlSchemaBasicItemPtr) ((xmlSchemaAttributeGroupPtr) item)->next);
	case XML_SCHEMA_TYPE_IDC_UNIQUE:
	case XML_SCHEMA_TYPE_IDC_KEY:
	case XML_SCHEMA_TYPE_IDC_KEYREF:
	    return ((xmlSchemaBasicItemPtr) ((xmlSchemaIDCPtr) item)->next);
	default:
	    return (NULL);
    }
}
#endif

/**
 * xmlSchemaGetAttrName:
 * @attr:  the attribute declaration/use
 *
 * Returns the name of the attribute; if the attribute
 * is a reference, the name of the referenced global type will be returned.
 */
static const xmlChar *
xmlSchemaGetAttrName(xmlSchemaAttributePtr attr)
{
    if (attr->ref != NULL)
	return(attr->ref);
    else
	return(attr->name);
}

/**
 * xmlSchemaGetAttrTargetNsURI:
 * @type:  the type (element or attribute)
 *
 * Returns the target namespace URI of the type; if the type is a reference,
 * the target namespace of the referenced type will be returned.
 */
static const xmlChar *
xmlSchemaGetAttrTargetNsURI(xmlSchemaAttributePtr attr)
{
    if (attr->ref != NULL)
	return (attr->refNs);
    else
	return(attr->targetNamespace);
}

/**
 * xmlSchemaFormatQName:
 * @buf: the string buffer
 * @namespaceName:  the namespace name
 * @localName: the local name
 *
 * Returns the given QName in the format "{namespaceName}localName" or
 * just "localName" if @namespaceName is NULL.
 *
 * Returns the localName if @namespaceName is NULL, a formatted
 * string otherwise.
 */
static const xmlChar*
xmlSchemaFormatQName(xmlChar **buf,
		     const xmlChar *namespaceName,
		     const xmlChar *localName)
{
    FREE_AND_NULL(*buf)
    if (namespaceName == NULL)
	return(localName);

    *buf = xmlStrdup(BAD_CAST "{");
    *buf = xmlStrcat(*buf, namespaceName);
    *buf = xmlStrcat(*buf, BAD_CAST "}");
    *buf = xmlStrcat(*buf, localName);

    return ((const xmlChar *) *buf);
}

static const xmlChar*   
xmlSchemaFormatQNameNs(xmlChar **buf, xmlNsPtr ns, const xmlChar *localName)
{
    if (ns != NULL)
	return (xmlSchemaFormatQName(buf, ns->href, localName));
    else
	return (xmlSchemaFormatQName(buf, NULL, localName));
}

static const xmlChar *
xmlSchemaGetComponentName(xmlSchemaBasicItemPtr item)
{
    switch (item->type) {
	case XML_SCHEMA_TYPE_ELEMENT:
	    return (((xmlSchemaElementPtr) item)->name);
	case XML_SCHEMA_TYPE_ATTRIBUTE:
	    return (((xmlSchemaAttributePtr) item)->name);
	case XML_SCHEMA_TYPE_ATTRIBUTEGROUP:
	    return (((xmlSchemaAttributeGroupPtr) item)->name);
	case XML_SCHEMA_TYPE_BASIC:
	case XML_SCHEMA_TYPE_SIMPLE:
	case XML_SCHEMA_TYPE_COMPLEX:
	    return (((xmlSchemaTypePtr) item)->name);
	case XML_SCHEMA_TYPE_GROUP:
	    return (((xmlSchemaModelGroupDefPtr) item)->name);
	case XML_SCHEMA_TYPE_IDC_KEY:
	case XML_SCHEMA_TYPE_IDC_UNIQUE:
	case XML_SCHEMA_TYPE_IDC_KEYREF:
	    return (((xmlSchemaIDCPtr) item)->name);
	default:
	    /*
	    * Other components cannot have names.
	    */
	    break;
    }
    return (NULL);
}

static const xmlChar *
xmlSchemaGetComponentTargetNs(xmlSchemaBasicItemPtr item)
{
    switch (item->type) {
	case XML_SCHEMA_TYPE_ELEMENT:
	    return (((xmlSchemaElementPtr) item)->targetNamespace);
	case XML_SCHEMA_TYPE_ATTRIBUTE:
	    return (((xmlSchemaAttributePtr) item)->targetNamespace);
	case XML_SCHEMA_TYPE_ATTRIBUTEGROUP:
	    return (((xmlSchemaAttributeGroupPtr) item)->targetNamespace);
	case XML_SCHEMA_TYPE_BASIC:
	    return (BAD_CAST "http://www.w3.org/2001/XMLSchema");
	case XML_SCHEMA_TYPE_SIMPLE:
	case XML_SCHEMA_TYPE_COMPLEX:
	    return (((xmlSchemaTypePtr) item)->targetNamespace);
	case XML_SCHEMA_TYPE_GROUP:
	    return (((xmlSchemaModelGroupDefPtr) item)->targetNamespace);
	case XML_SCHEMA_TYPE_IDC_KEY:
	case XML_SCHEMA_TYPE_IDC_UNIQUE:
	case XML_SCHEMA_TYPE_IDC_KEYREF:
	    return (((xmlSchemaIDCPtr) item)->targetNamespace);
	default:
	    /*
	    * Other components cannot have names.
	    */
	    break;
    }
    return (NULL);
}

static const xmlChar*
xmlSchemaGetComponentQName(xmlChar **buf,
			   void *item)
{
    return (xmlSchemaFormatQName(buf,
	xmlSchemaGetComponentTargetNs((xmlSchemaBasicItemPtr) item),
	xmlSchemaGetComponentName((xmlSchemaBasicItemPtr) item)));
}

/**
 * xmlSchemaWildcardPCToString:
 * @pc: the type of processContents
 *
 * Returns a string representation of the type of
 * processContents.
 */
static const xmlChar *
xmlSchemaWildcardPCToString(int pc)
{
    switch (pc) {
	case XML_SCHEMAS_ANY_SKIP:
	    return (BAD_CAST "skip");
	case XML_SCHEMAS_ANY_LAX:
	    return (BAD_CAST "lax");
	case XML_SCHEMAS_ANY_STRICT:
	    return (BAD_CAST "strict");
	default:
	    return (BAD_CAST "invalid process contents");
    }
}

/**
 * xmlSchemaGetCanonValueWhtspExt:
 * @val: the precomputed value
 * @retValue: the returned value
 * @ws: the whitespace type of the value
 *
 * Get a the cononical representation of the value.
 * The caller has to free the returned retValue.
 *
 * Returns 0 if the value could be built and -1 in case of
 *         API errors or if the value type is not supported yet.
 */
static int
xmlSchemaGetCanonValueWhtspExt(xmlSchemaValPtr val,
			       xmlSchemaWhitespaceValueType ws,
			       xmlChar **retValue)
{
    int list;
    xmlSchemaValType valType;
    const xmlChar *value, *value2 = NULL;
    

    if ((retValue == NULL) || (val == NULL))
	return (-1);
    list = xmlSchemaValueGetNext(val) ? 1 : 0;
    *retValue = NULL;
    do {
	value = NULL;	
	valType = xmlSchemaGetValType(val);    
	switch (valType) {	    
	    case XML_SCHEMAS_STRING:
	    case XML_SCHEMAS_NORMSTRING:
	    case XML_SCHEMAS_ANYSIMPLETYPE:
		value = xmlSchemaValueGetAsString(val);
		if (value != NULL) {
		    if (ws == XML_SCHEMA_WHITESPACE_COLLAPSE)
			value2 = xmlSchemaCollapseString(value);
		    else if (ws == XML_SCHEMA_WHITESPACE_REPLACE)
			value2 = xmlSchemaWhiteSpaceReplace(value);
		    if (value2 != NULL)
			value = value2;
		}
		break;	   
	    default:
		if (xmlSchemaGetCanonValue(val, &value2) == -1) {
		    if (value2 != NULL)
			xmlFree((xmlChar *) value2);
		    goto internal_error;
		}
		value = value2;
	}
	if (*retValue == NULL)
	    if (value == NULL) {
		if (! list)
		    *retValue = xmlStrdup(BAD_CAST "");
	    } else
		*retValue = xmlStrdup(value);
	else if (value != NULL) {
	    /* List. */
	    *retValue = xmlStrcat((xmlChar *) *retValue, BAD_CAST " ");
	    *retValue = xmlStrcat((xmlChar *) *retValue, value);
	}
	FREE_AND_NULL(value2)
	val = xmlSchemaValueGetNext(val);
    } while (val != NULL);

    return (0);
internal_error:
    if (*retValue != NULL)
	xmlFree((xmlChar *) (*retValue));
    if (value2 != NULL)
	xmlFree((xmlChar *) value2);
    return (-1);
}

/**
 * xmlSchemaFormatItemForReport:
 * @buf: the string buffer
 * @itemDes: the designation of the item
 * @itemName: the name of the item
 * @item: the item as an object 
 * @itemNode: the node of the item
 * @local: the local name
 * @parsing: if the function is used during the parse
 *
 * Returns a representation of the given item used
 * for error reports. 
 *
 * The following order is used to build the resulting 
 * designation if the arguments are not NULL:
 * 1a. If itemDes not NULL -> itemDes
 * 1b. If (itemDes not NULL) and (itemName not NULL)
 *     -> itemDes + itemName
 * 2. If the preceding was NULL and (item not NULL) -> item
 * 3. If the preceding was NULL and (itemNode not NULL) -> itemNode
 * 
 * If the itemNode is an attribute node, the name of the attribute
 * will be appended to the result.
 *
 * Returns the formatted string and sets @buf to the resulting value.
 */  
static xmlChar*   
xmlSchemaFormatItemForReport(xmlChar **buf,		     
		     const xmlChar *itemDes,
		     xmlSchemaTypePtr item,
		     xmlNodePtr itemNode)
{
    xmlChar *str = NULL;
    int named = 1;

    if (*buf != NULL) {
	xmlFree(*buf);
	*buf = NULL;
    }
            
    if (itemDes != NULL) {
	*buf = xmlStrdup(itemDes);	
    } else if (item != NULL) {
	switch (item->type) {
	case XML_SCHEMA_TYPE_BASIC:
	    if (VARIETY_ATOMIC(item))
		*buf = xmlStrdup(BAD_CAST "atomic type 'xs:");
	    else if (VARIETY_LIST(item))
		*buf = xmlStrdup(BAD_CAST "list type 'xs:");
	    else if (VARIETY_UNION(item))
		*buf = xmlStrdup(BAD_CAST "union type 'xs:");
	    else
		*buf = xmlStrdup(BAD_CAST "simple type 'xs:");
	    *buf = xmlStrcat(*buf, item->name);
	    *buf = xmlStrcat(*buf, BAD_CAST "'");
	    break;
	case XML_SCHEMA_TYPE_SIMPLE:
	    if (item->flags & XML_SCHEMAS_TYPE_GLOBAL) {
		*buf = xmlStrdup(BAD_CAST"");
	    } else {
		*buf = xmlStrdup(BAD_CAST "local ");
	    }
	    if (VARIETY_ATOMIC(item))
		*buf = xmlStrcat(*buf, BAD_CAST "atomic type");
	    else if (VARIETY_LIST(item))
		*buf = xmlStrcat(*buf, BAD_CAST "list type");
	    else if (VARIETY_UNION(item))
		*buf = xmlStrcat(*buf, BAD_CAST "union type");
	    else
		*buf = xmlStrcat(*buf, BAD_CAST "simple type");
	    if (item->flags & XML_SCHEMAS_TYPE_GLOBAL) {
		*buf = xmlStrcat(*buf, BAD_CAST " '");
		*buf = xmlStrcat(*buf, item->name);
		*buf = xmlStrcat(*buf, BAD_CAST "'");
	    }
	    break;
	case XML_SCHEMA_TYPE_COMPLEX:
	    if (item->flags & XML_SCHEMAS_TYPE_GLOBAL)
		*buf = xmlStrdup(BAD_CAST "");
	    else
		*buf = xmlStrdup(BAD_CAST "local ");
	    *buf = xmlStrcat(*buf, BAD_CAST "complex type");
	    if (item->flags & XML_SCHEMAS_TYPE_GLOBAL) {
		*buf = xmlStrcat(*buf, BAD_CAST " '");
		*buf = xmlStrcat(*buf, item->name);
		*buf = xmlStrcat(*buf, BAD_CAST "'");
	    }
	    break;
	case XML_SCHEMA_TYPE_ATTRIBUTE: {
		xmlSchemaAttributePtr attr;
	    
		attr = (xmlSchemaAttributePtr) item;	    
		if ((attr->flags & XML_SCHEMAS_ATTR_GLOBAL) ||
		    (attr->ref == NULL)) {
		    *buf = xmlStrdup(xmlSchemaElemDesAttrDecl);
		    *buf = xmlStrcat(*buf, BAD_CAST " '");
		    *buf = xmlStrcat(*buf, xmlSchemaFormatQName(&str,
			attr->targetNamespace, attr->name));
		    FREE_AND_NULL(str)
		    *buf = xmlStrcat(*buf, BAD_CAST "'");
		} else {
		    *buf = xmlStrdup(xmlSchemaElemDesAttrRef);
		    *buf = xmlStrcat(*buf, BAD_CAST " '");
		    *buf = xmlStrcat(*buf, xmlSchemaFormatQName(&str,
			attr->refNs, attr->ref));
		    FREE_AND_NULL(str)
		    *buf = xmlStrcat(*buf, BAD_CAST "'");
		}	
	    }
	    break;
	case XML_SCHEMA_TYPE_ELEMENT: {
		xmlSchemaElementPtr elem;

		elem = (xmlSchemaElementPtr) item;	    
		if ((elem->flags & XML_SCHEMAS_ELEM_GLOBAL) || 
		    (elem->ref == NULL)) {
		    *buf = xmlStrdup(xmlSchemaElemDesElemDecl);
		    *buf = xmlStrcat(*buf, BAD_CAST " '");
		    *buf = xmlStrcat(*buf, xmlSchemaFormatQName(&str,
			elem->targetNamespace, elem->name));
		    *buf = xmlStrcat(*buf, BAD_CAST "'");
		}
	    }
	    break;
	case XML_SCHEMA_TYPE_IDC_UNIQUE:
	case XML_SCHEMA_TYPE_IDC_KEY:
	case XML_SCHEMA_TYPE_IDC_KEYREF:		
	    if (item->type == XML_SCHEMA_TYPE_IDC_UNIQUE)
		*buf = xmlStrdup(BAD_CAST "unique '");
	    else if (item->type == XML_SCHEMA_TYPE_IDC_KEY)
		*buf = xmlStrdup(BAD_CAST "key '");
	    else
		*buf = xmlStrdup(BAD_CAST "keyRef '");
	    *buf = xmlStrcat(*buf, ((xmlSchemaIDCPtr) item)->name);
	    *buf = xmlStrcat(*buf, BAD_CAST "'");
	    break;
	case XML_SCHEMA_TYPE_ANY:
	case XML_SCHEMA_TYPE_ANY_ATTRIBUTE:
	    *buf = xmlStrdup(xmlSchemaWildcardPCToString(
		    ((xmlSchemaWildcardPtr) item)->processContents));
	    *buf = xmlStrcat(*buf, BAD_CAST " wildcard");
	    break;
	case XML_SCHEMA_FACET_MININCLUSIVE:
	case XML_SCHEMA_FACET_MINEXCLUSIVE:
	case XML_SCHEMA_FACET_MAXINCLUSIVE:
	case XML_SCHEMA_FACET_MAXEXCLUSIVE:
	case XML_SCHEMA_FACET_TOTALDIGITS:
	case XML_SCHEMA_FACET_FRACTIONDIGITS:
	case XML_SCHEMA_FACET_PATTERN:
	case XML_SCHEMA_FACET_ENUMERATION:
	case XML_SCHEMA_FACET_WHITESPACE:
	case XML_SCHEMA_FACET_LENGTH:
	case XML_SCHEMA_FACET_MAXLENGTH:
	case XML_SCHEMA_FACET_MINLENGTH:
	    *buf = xmlStrdup(BAD_CAST "facet '");
	    *buf = xmlStrcat(*buf, xmlSchemaFacetTypeToString(item->type));
	    *buf = xmlStrcat(*buf, BAD_CAST "'");
	    break;
	case XML_SCHEMA_TYPE_NOTATION:
	    *buf = xmlStrdup(BAD_CAST "notation");
	    break;
	case XML_SCHEMA_TYPE_GROUP: {
		*buf = xmlStrdup(xmlSchemaElemModelGrDef);
		*buf = xmlStrcat(*buf, BAD_CAST " '");
		*buf = xmlStrcat(*buf, xmlSchemaFormatQName(&str,
		    ((xmlSchemaModelGroupDefPtr) item)->targetNamespace,
		    ((xmlSchemaModelGroupDefPtr) item)->name));
		*buf = xmlStrcat(*buf, BAD_CAST "'");
		FREE_AND_NULL(str)
	    }
	    break;
	case XML_SCHEMA_TYPE_SEQUENCE:
	case XML_SCHEMA_TYPE_CHOICE:
	case XML_SCHEMA_TYPE_ALL:
	case XML_SCHEMA_TYPE_PARTICLE:
	    *buf = xmlStrdup(xmlSchemaCompTypeToString(item->type));
	    break;	
	default:
	    named = 0;
	}
    } else 
	named = 0;

    if ((named == 0) && (itemNode != NULL)) {
	xmlNodePtr elem;

	if (itemNode->type == XML_ATTRIBUTE_NODE)
	    elem = itemNode->parent;
	else 
	    elem = itemNode;
	*buf = xmlStrdup(BAD_CAST "Element '");
	if (elem->ns != NULL) {
	    *buf = xmlStrcat(*buf,
		xmlSchemaFormatQName(&str, elem->ns->href, elem->name));
	    FREE_AND_NULL(str)
	} else
	    *buf = xmlStrcat(*buf, elem->name);
	*buf = xmlStrcat(*buf, BAD_CAST "'");
	
    }
    if ((itemNode != NULL) && (itemNode->type == XML_ATTRIBUTE_NODE)) {
	*buf = xmlStrcat(*buf, BAD_CAST ", attribute '");
	if (itemNode->ns != NULL) {
	    *buf = xmlStrcat(*buf, xmlSchemaFormatQName(&str,
		itemNode->ns->href, itemNode->name));
	    FREE_AND_NULL(str)
	} else
	    *buf = xmlStrcat(*buf, itemNode->name);
	*buf = xmlStrcat(*buf, BAD_CAST "'");
    }
    FREE_AND_NULL(str)
    
    return (*buf);
}

/**
 * xmlSchemaFormatFacetEnumSet:
 * @buf: the string buffer
 * @type: the type holding the enumeration facets
 *
 * Builds a string consisting of all enumeration elements.
 *
 * Returns a string of all enumeration elements.
 */
static const xmlChar *
xmlSchemaFormatFacetEnumSet(xmlSchemaAbstractCtxtPtr actxt,
			    xmlChar **buf, xmlSchemaTypePtr type)
{
    xmlSchemaFacetPtr facet;
    xmlSchemaWhitespaceValueType ws;
    xmlChar *value = NULL;
    int res;

    if (*buf != NULL)
	xmlFree(*buf);    
    *buf = NULL;

    do {
	/*
	* Use the whitespace type of the base type.
	*/	
	ws = xmlSchemaGetWhiteSpaceFacetValue(type->baseType);
	for (facet = type->facets; facet != NULL; facet = facet->next) {
	    if (facet->type != XML_SCHEMA_FACET_ENUMERATION)
		continue;
	    res = xmlSchemaGetCanonValueWhtspExt(facet->val,
		ws, &value);
	    if (res == -1) {
		xmlSchemaInternalErr(actxt,
		    "xmlSchemaFormatFacetEnumSet",
		    "compute the canonical lexical representation");
		if (*buf != NULL)
		    xmlFree(*buf);
		*buf = NULL;
		return (NULL);
	    }
	    if (*buf == NULL)
		*buf = xmlStrdup(BAD_CAST "'");
	    else
		*buf = xmlStrcat(*buf, BAD_CAST ", '");
	    *buf = xmlStrcat(*buf, BAD_CAST value);
	    *buf = xmlStrcat(*buf, BAD_CAST "'");
	    if (value != NULL) {
		xmlFree((xmlChar *)value);
		value = NULL;
	    }
	}
	type = type->baseType;
    } while ((type != NULL) && (type->type != XML_SCHEMA_TYPE_BASIC));

    return ((const xmlChar *) *buf);
}

/************************************************************************
 *									*
 * 			Error functions				        *
 *									*
 ************************************************************************/

#if 0
static void
xmlSchemaErrMemory(const char *msg)
{
    __xmlSimpleError(XML_FROM_SCHEMASP, XML_ERR_NO_MEMORY, NULL, NULL,
                     msg);
}
#endif

/**
 * xmlSchemaPErrMemory:
 * @node: a context node
 * @extra:  extra informations
 *
 * Handle an out of memory condition
 */
static void
xmlSchemaPErrMemory(xmlSchemaParserCtxtPtr ctxt,
                    const char *extra, xmlNodePtr node)
{
    if (ctxt != NULL)
        ctxt->nberrors++;
    __xmlSimpleError(XML_FROM_SCHEMASP, XML_ERR_NO_MEMORY, node, NULL,
                     extra);
}

/**
 * xmlSchemaPErr:
 * @ctxt: the parsing context
 * @node: the context node
 * @error: the error code
 * @msg: the error message
 * @str1: extra data
 * @str2: extra data
 * 
 * Handle a parser error
 */
static void
xmlSchemaPErr(xmlSchemaParserCtxtPtr ctxt, xmlNodePtr node, int error,
              const char *msg, const xmlChar * str1, const xmlChar * str2)
{
    xmlGenericErrorFunc channel = NULL;
    xmlStructuredErrorFunc schannel = NULL;
    void *data = NULL;

    if (ctxt != NULL) {
        ctxt->nberrors++;
        channel = ctxt->error;
        data = ctxt->userData;
	schannel = ctxt->serror;
    }
    __xmlRaiseError(schannel, channel, data, ctxt, node, XML_FROM_SCHEMASP,
                    error, XML_ERR_ERROR, NULL, 0,
                    (const char *) str1, (const char *) str2, NULL, 0, 0,
                    msg, str1, str2);
}

/**
 * xmlSchemaPErr2:
 * @ctxt: the parsing context
 * @node: the context node
 * @node: the current child
 * @error: the error code
 * @msg: the error message
 * @str1: extra data
 * @str2: extra data
 * 
 * Handle a parser error
 */
static void
xmlSchemaPErr2(xmlSchemaParserCtxtPtr ctxt, xmlNodePtr node,
               xmlNodePtr child, int error,
               const char *msg, const xmlChar * str1, const xmlChar * str2)
{
    if (child != NULL)
        xmlSchemaPErr(ctxt, child, error, msg, str1, str2);
    else
        xmlSchemaPErr(ctxt, node, error, msg, str1, str2);
}


/**
 * xmlSchemaPErrExt:
 * @ctxt: the parsing context
 * @node: the context node
 * @error: the error code 
 * @strData1: extra data
 * @strData2: extra data
 * @strData3: extra data
 * @msg: the message
 * @str1:  extra parameter for the message display
 * @str2:  extra parameter for the message display
 * @str3:  extra parameter for the message display
 * @str4:  extra parameter for the message display
 * @str5:  extra parameter for the message display
 * 
 * Handle a parser error
 */
static void
xmlSchemaPErrExt(xmlSchemaParserCtxtPtr ctxt, xmlNodePtr node, int error,
		const xmlChar * strData1, const xmlChar * strData2, 
		const xmlChar * strData3, const char *msg, const xmlChar * str1, 
		const xmlChar * str2, const xmlChar * str3, const xmlChar * str4,
		const xmlChar * str5)
{

    xmlGenericErrorFunc channel = NULL;
    xmlStructuredErrorFunc schannel = NULL;
    void *data = NULL;

    if (ctxt != NULL) {
        ctxt->nberrors++;
        channel = ctxt->error;
        data = ctxt->userData;
	schannel = ctxt->serror;
    }
    __xmlRaiseError(schannel, channel, data, ctxt, node, XML_FROM_SCHEMASP,
                    error, XML_ERR_ERROR, NULL, 0,
                    (const char *) strData1, (const char *) strData2, 
		    (const char *) strData3, 0, 0, msg, str1, str2, 
		    str3, str4, str5);
}

/************************************************************************
 *									*
 * 			Allround error functions			*
 *									*
 ************************************************************************/

/**
 * xmlSchemaVTypeErrMemory:
 * @node: a context node
 * @extra:  extra informations
 *
 * Handle an out of memory condition
 */
static void
xmlSchemaVErrMemory(xmlSchemaValidCtxtPtr ctxt,
                    const char *extra, xmlNodePtr node)
{
    if (ctxt != NULL) {
        ctxt->nberrors++;
        ctxt->err = XML_SCHEMAV_INTERNAL;
    }
    __xmlSimpleError(XML_FROM_SCHEMASV, XML_ERR_NO_MEMORY, node, NULL,
                     extra);
}

/**
 * xmlSchemaErr3:
 * @ctxt: the validation context
 * @node: the context node
 * @error: the error code
 * @msg: the error message
 * @str1: extra data
 * @str2: extra data
 * @str3: extra data
 * 
 * Handle a validation error
 */
static void
xmlSchemaErr3(xmlSchemaAbstractCtxtPtr ctxt,  
	      int error, xmlNodePtr node, const char *msg,
	      const xmlChar *str1, const xmlChar *str2, const xmlChar *str3)
{
    xmlStructuredErrorFunc schannel = NULL;
    xmlGenericErrorFunc channel = NULL;
    void *data = NULL;
    
    if (ctxt != NULL) {
	if (ctxt->type == XML_SCHEMA_CTXT_VALIDATOR) {
	    xmlSchemaValidCtxtPtr vctxt = (xmlSchemaValidCtxtPtr) ctxt;
	    int line = 0;
	    const char *file = NULL;

	    vctxt->nberrors++;
	    vctxt->err = error;
	    channel = vctxt->error;
	    schannel = vctxt->serror;
	    data = vctxt->userData;
	    if ((node == NULL) && (vctxt->depth >= 0) &&
		(vctxt->inode != NULL)) {
		node = vctxt->inode->node;
	    }
	    if ((node == NULL) && (vctxt->parserCtxt != NULL) &&
	        (vctxt->parserCtxt->input != NULL)) {
		file = vctxt->parserCtxt->input->filename;
		line = vctxt->parserCtxt->input->line;
	    }
	    __xmlRaiseError(schannel, channel, data, ctxt,
		node, XML_FROM_SCHEMASV,
		error, XML_ERR_ERROR, file, line,
		(const char *) str1, (const char *) str2,
		(const char *) str3, 0, 0, msg, str1, str2, str3);

	} else if (ctxt->type == XML_SCHEMA_CTXT_PARSER) {
	    xmlSchemaParserCtxtPtr pctxt = (xmlSchemaParserCtxtPtr) ctxt;

	    pctxt->nberrors++;
	    pctxt->err = error;
	    channel = pctxt->error;
	    schannel = pctxt->serror;
	    data = pctxt->userData;
	    __xmlRaiseError(schannel, channel, data, ctxt,
		node, XML_FROM_SCHEMASP,
		error, XML_ERR_ERROR, NULL, 0,
		(const char *) str1, (const char *) str2,
		(const char *) str3, 0, 0, msg, str1, str2, str3);
	} else {
	    TODO
	}
    }       
}

static void
xmlSchemaErr(xmlSchemaAbstractCtxtPtr actxt,
	     int error, xmlNodePtr node, const char *msg,
	     const xmlChar *str1, const xmlChar *str2)
{
    xmlSchemaErr3(actxt, error, node, msg, str1, str2, NULL);
}

static xmlChar *
xmlSchemaFormatNodeForError(xmlChar ** msg,
			    xmlSchemaAbstractCtxtPtr actxt,
			    xmlNodePtr node)
{
    xmlChar *str = NULL;

    if (node != NULL) {
	/*
	* Work on tree nodes.
	*/
	if (node->type == XML_ATTRIBUTE_NODE) {
	    xmlNodePtr elem = node->parent;
	    
	    *msg = xmlStrdup(BAD_CAST "Element '");
	    if (elem->ns != NULL)
		*msg = xmlStrcat(*msg, xmlSchemaFormatQName(&str,
		    elem->ns->href, elem->name));
	    else
		*msg = xmlStrcat(*msg, xmlSchemaFormatQName(&str,
		    NULL, elem->name));
	    FREE_AND_NULL(str);
	    *msg = xmlStrcat(*msg, BAD_CAST "', ");
	    *msg = xmlStrcat(*msg, BAD_CAST "attribute '");	    
	} else {
	    *msg = xmlStrdup(BAD_CAST "Element '");
	}
	if (node->ns != NULL)
	    *msg = xmlStrcat(*msg, xmlSchemaFormatQName(&str,
	    node->ns->href, node->name));
	else
	    *msg = xmlStrcat(*msg, xmlSchemaFormatQName(&str,
	    NULL, node->name));
	FREE_AND_NULL(str);
	*msg = xmlStrcat(*msg, BAD_CAST "': ");
    } else if (actxt->type == XML_SCHEMA_CTXT_VALIDATOR) {
	xmlSchemaValidCtxtPtr vctxt = (xmlSchemaValidCtxtPtr) actxt;
	/*
	* Work on node infos.
	*/	
	if (vctxt->inode->nodeType == XML_ATTRIBUTE_NODE) {
	    xmlSchemaNodeInfoPtr ielem =
		vctxt->elemInfos[vctxt->depth];

	    *msg = xmlStrdup(BAD_CAST "Element '");
	    *msg = xmlStrcat(*msg, xmlSchemaFormatQName(&str,
		ielem->nsName, ielem->localName));
	    FREE_AND_NULL(str);
	    *msg = xmlStrcat(*msg, BAD_CAST "', ");
	    *msg = xmlStrcat(*msg, BAD_CAST "attribute '");	    
	} else {
	    *msg = xmlStrdup(BAD_CAST "Element '");
	}
	*msg = xmlStrcat(*msg, xmlSchemaFormatQName(&str,
	    vctxt->inode->nsName, vctxt->inode->localName));
	FREE_AND_NULL(str);
	*msg = xmlStrcat(*msg, BAD_CAST "': ");
    } else {
	TODO
	return (NULL);
    }
    /*
    * VAL TODO: The output of the given schema component is currently
    * disabled.
    */
#if 0    
    if ((type != NULL) && (xmlSchemaIsGlobalItem(type))) {
	*msg = xmlStrcat(*msg, BAD_CAST " [");
	*msg = xmlStrcat(*msg, xmlSchemaFormatItemForReport(&str,
	    NULL, type, NULL, 0));
	FREE_AND_NULL(str)
	*msg = xmlStrcat(*msg, BAD_CAST "]");
    }
#endif
    return (*msg);
}

static void
xmlSchemaInternalErr(xmlSchemaAbstractCtxtPtr actxt,
		     const char *funcName,
		     const char *message)
{
    xmlChar *msg = NULL;

    msg = xmlStrdup(BAD_CAST "Internal error: ");
    msg = xmlStrcat(msg, BAD_CAST funcName);
    msg = xmlStrcat(msg, BAD_CAST ", ");    
    msg = xmlStrcat(msg, BAD_CAST message);
    msg = xmlStrcat(msg, BAD_CAST ".\n");

    if (actxt->type == XML_SCHEMA_CTXT_VALIDATOR)
	xmlSchemaErr(actxt, XML_SCHEMAV_INTERNAL, NULL,
	    (const char *) msg, NULL, NULL);

    else if (actxt->type == XML_SCHEMA_CTXT_PARSER)
	xmlSchemaErr(actxt, XML_SCHEMAP_INTERNAL, NULL,
	    (const char *) msg, NULL, NULL);

    FREE_AND_NULL(msg)
}

static void
xmlSchemaCustomErr(xmlSchemaAbstractCtxtPtr actxt,
		   xmlParserErrors error,
		   xmlNodePtr node,
		   xmlSchemaTypePtr type ATTRIBUTE_UNUSED,
		   const char *message,
		   const xmlChar *str1,
		   const xmlChar *str2)
{
    xmlChar *msg = NULL;

    xmlSchemaFormatNodeForError(&msg, actxt, node);
    msg = xmlStrcat(msg, (const xmlChar *) message);
    msg = xmlStrcat(msg, BAD_CAST ".\n");   
    xmlSchemaErr(actxt, error, node,
	(const char *) msg, str1, str2);
    FREE_AND_NULL(msg)
}

static int
xmlSchemaEvalErrorNodeType(xmlSchemaAbstractCtxtPtr actxt,
			   xmlNodePtr node)
{
    if (node != NULL)
	return (node->type);
    if ((actxt->type == XML_SCHEMA_CTXT_VALIDATOR) &&
	(((xmlSchemaValidCtxtPtr) actxt)->inode != NULL))
	return ( ((xmlSchemaValidCtxtPtr) actxt)->inode->nodeType);
    return (-1);
}

static int
xmlSchemaIsGlobalItem(xmlSchemaTypePtr item)
{
    switch (item->type) {
	case XML_SCHEMA_TYPE_COMPLEX:
	case XML_SCHEMA_TYPE_SIMPLE:
	    if (item->flags & XML_SCHEMAS_TYPE_GLOBAL)
		return(1);
	    break;
	case XML_SCHEMA_TYPE_GROUP:
	    return (1);
	case XML_SCHEMA_TYPE_ELEMENT:
	    if ( ((xmlSchemaElementPtr) item)->flags &
		XML_SCHEMAS_ELEM_GLOBAL)
		return(1);
	    break;
	case XML_SCHEMA_TYPE_ATTRIBUTE:
	    if ( ((xmlSchemaAttributePtr) item)->flags &
		XML_SCHEMAS_ATTR_GLOBAL)
		return(1);
	    break;
	/* Note that attribute groups are always global. */
	default:
	    return(1);
    }
    return (0);
}

static void
xmlSchemaSimpleTypeErr(xmlSchemaAbstractCtxtPtr actxt,
		       xmlParserErrors error,
		       xmlNodePtr node,
		       const xmlChar *value,
		       xmlSchemaTypePtr type,
		       int displayValue)
{
    xmlChar *msg = NULL;

    xmlSchemaFormatNodeForError(&msg, actxt, node);

    if (displayValue || (xmlSchemaEvalErrorNodeType(actxt, node) ==
	    XML_ATTRIBUTE_NODE))
	msg = xmlStrcat(msg, BAD_CAST "'%s' is not a valid value of ");
    else
	msg = xmlStrcat(msg, BAD_CAST "The character content is not a valid "
	    "value of ");

    if (! xmlSchemaIsGlobalItem(type))
	msg = xmlStrcat(msg, BAD_CAST "the local ");
    else
	msg = xmlStrcat(msg, BAD_CAST "the ");

    if (VARIETY_ATOMIC(type))
	msg = xmlStrcat(msg, BAD_CAST "atomic type");
    else if (VARIETY_LIST(type))
	msg = xmlStrcat(msg, BAD_CAST "list type");
    else if (VARIETY_UNION(type))
	msg = xmlStrcat(msg, BAD_CAST "union type");

    if (xmlSchemaIsGlobalItem(type)) {
	xmlChar *str = NULL;
	msg = xmlStrcat(msg, BAD_CAST " '");
	if (type->builtInType != 0) {
	    msg = xmlStrcat(msg, BAD_CAST "xs:");
	    msg = xmlStrcat(msg, type->name);
	} else 
	    msg = xmlStrcat(msg,
		xmlSchemaFormatQName(&str,
		    type->targetNamespace, type->name));
	msg = xmlStrcat(msg, BAD_CAST "'");
	FREE_AND_NULL(str);
    }
    msg = xmlStrcat(msg, BAD_CAST ".\n");
    if (displayValue || (xmlSchemaEvalErrorNodeType(actxt, node) ==
	    XML_ATTRIBUTE_NODE))
	xmlSchemaErr(actxt, error, node, (const char *) msg, value, NULL);
    else
	xmlSchemaErr(actxt, error, node, (const char *) msg, NULL, NULL);
    FREE_AND_NULL(msg)
}

static const xmlChar *
xmlSchemaFormatErrorNodeQName(xmlChar ** str,
			      xmlSchemaNodeInfoPtr ni,
			      xmlNodePtr node)
{
    if (node != NULL) {
	if (node->ns != NULL)
	    return (xmlSchemaFormatQName(str, node->ns->href, node->name));
	else
	    return (xmlSchemaFormatQName(str, NULL, node->name));
    } else if (ni != NULL)
	return (xmlSchemaFormatQName(str, ni->nsName, ni->localName));
    return (NULL);
}

static void
xmlSchemaIllegalAttrErr(xmlSchemaAbstractCtxtPtr actxt,
			xmlParserErrors error,
			xmlSchemaAttrInfoPtr ni,
			xmlNodePtr node)
{
    xmlChar *msg = NULL, *str = NULL;
    
    xmlSchemaFormatNodeForError(&msg, actxt, node);
    msg = xmlStrcat(msg, BAD_CAST "The attribute '%s' is not allowed.\n");
    xmlSchemaErr(actxt, error, node, (const char *) msg,
	xmlSchemaFormatErrorNodeQName(&str, (xmlSchemaNodeInfoPtr) ni, node),
	NULL);        
    FREE_AND_NULL(str)
    FREE_AND_NULL(msg)
}

static void
xmlSchemaComplexTypeErr(xmlSchemaAbstractCtxtPtr actxt,
		        xmlParserErrors error,
		        xmlNodePtr node,
			xmlSchemaTypePtr type ATTRIBUTE_UNUSED,
			const char *message,
			int nbval,
			int nbneg,
			xmlChar **values)
{
    xmlChar *str = NULL, *msg = NULL;
    xmlChar *localName, *nsName;
    const xmlChar *cur, *end;
    int i;
    
    xmlSchemaFormatNodeForError(&msg, actxt, node);
    msg = xmlStrcat(msg, (const xmlChar *) message);
    msg = xmlStrcat(msg, BAD_CAST ".");
    /*
    * Note that is does not make sense to report that we have a
    * wildcard here, since the wildcard might be unfolded into
    * multiple transitions.
    */
    if (nbval + nbneg > 0) {
	if (nbval + nbneg > 1) {
	    str = xmlStrdup(BAD_CAST " Expected is one of ( ");
	} else
	    str = xmlStrdup(BAD_CAST " Expected is ( ");
	nsName = NULL;
    	    
	for (i = 0; i < nbval + nbneg; i++) {
	    cur = values[i];
	    /*
	    * Get the local name.
	    */
	    localName = NULL;
	    
	    end = cur;
	    if (*end == '*') {
		localName = xmlStrdup(BAD_CAST "*");
		end++;
	    } else {
		while ((*end != 0) && (*end != '|'))
		    end++;
		localName = xmlStrncat(localName, BAD_CAST cur, end - cur);
	    }		
	    if (*end != 0) {		    
		end++;
		/*
		* Skip "*|*" if they come with negated expressions, since
		* they represent the same negated wildcard.
		*/
		if ((nbneg == 0) || (*end != '*') || (*localName != '*')) {
		    /*
		    * Get the namespace name.
		    */
		    cur = end;
		    if (*end == '*') {
			nsName = xmlStrdup(BAD_CAST "{*}");
		    } else {
			while (*end != 0)
			    end++;
			
			if (i >= nbval)
			    nsName = xmlStrdup(BAD_CAST "{##other:");
			else
			    nsName = xmlStrdup(BAD_CAST "{");
			
			nsName = xmlStrncat(nsName, BAD_CAST cur, end - cur);
			nsName = xmlStrcat(nsName, BAD_CAST "}");
		    }
		    str = xmlStrcat(str, BAD_CAST nsName);
		    FREE_AND_NULL(nsName)
		} else {
		    FREE_AND_NULL(localName);
		    continue;
		}
	    }	        
	    str = xmlStrcat(str, BAD_CAST localName);
	    FREE_AND_NULL(localName);
		
	    if (i < nbval + nbneg -1)
		str = xmlStrcat(str, BAD_CAST ", ");
	}	
	str = xmlStrcat(str, BAD_CAST " ).\n");
	msg = xmlStrcat(msg, BAD_CAST str);
	FREE_AND_NULL(str)
    } else
      msg = xmlStrcat(msg, BAD_CAST "\n");
    xmlSchemaErr(actxt, error, node, (const char *) msg, NULL, NULL);
    xmlFree(msg);
}

static void
xmlSchemaFacetErr(xmlSchemaAbstractCtxtPtr actxt,
		  xmlParserErrors error,
		  xmlNodePtr node,
		  const xmlChar *value,
		  unsigned long length,
		  xmlSchemaTypePtr type,
		  xmlSchemaFacetPtr facet,
		  const char *message,
		  const xmlChar *str1,
		  const xmlChar *str2)
{
    xmlChar *str = NULL, *msg = NULL;
    xmlSchemaTypeType facetType;
    int nodeType = xmlSchemaEvalErrorNodeType(actxt, node);

    xmlSchemaFormatNodeForError(&msg, actxt, node);
    if (error == XML_SCHEMAV_CVC_ENUMERATION_VALID) {
	facetType = XML_SCHEMA_FACET_ENUMERATION;
	/*
	* If enumerations are validated, one must not expect the
	* facet to be given.
	*/	
    } else	
	facetType = facet->type;
    msg = xmlStrcat(msg, BAD_CAST "[");
    msg = xmlStrcat(msg, BAD_CAST "facet '");
    msg = xmlStrcat(msg, xmlSchemaFacetTypeToString(facetType));
    msg = xmlStrcat(msg, BAD_CAST "'] ");
    if (message == NULL) {
	/*
	* Use a default message.
	*/
	if ((facetType == XML_SCHEMA_FACET_LENGTH) ||
	    (facetType == XML_SCHEMA_FACET_MINLENGTH) ||
	    (facetType == XML_SCHEMA_FACET_MAXLENGTH)) {

	    char len[25], actLen[25];

	    /* FIXME, TODO: What is the max expected string length of the
	    * this value?
	    */
	    if (nodeType == XML_ATTRIBUTE_NODE)
		msg = xmlStrcat(msg, BAD_CAST "The value '%s' has a length of '%s'; ");
	    else
		msg = xmlStrcat(msg, BAD_CAST "The value has a length of '%s'; ");

	    snprintf(len, 24, "%lu", xmlSchemaGetFacetValueAsULong(facet));
	    snprintf(actLen, 24, "%lu", length);

	    if (facetType == XML_SCHEMA_FACET_LENGTH)
		msg = xmlStrcat(msg, 
		BAD_CAST "this differs from the allowed length of '%s'.\n");     
	    else if (facetType == XML_SCHEMA_FACET_MAXLENGTH)
		msg = xmlStrcat(msg, 
		BAD_CAST "this exceeds the allowed maximum length of '%s'.\n");
	    else if (facetType == XML_SCHEMA_FACET_MINLENGTH)
		msg = xmlStrcat(msg, 
		BAD_CAST "this underruns the allowed minimum length of '%s'.\n");
	    
	    if (nodeType == XML_ATTRIBUTE_NODE)
		xmlSchemaErr3(actxt, error, node, (const char *) msg,
		    value, (const xmlChar *) actLen, (const xmlChar *) len);
	    else 
		xmlSchemaErr(actxt, error, node, (const char *) msg,
		    (const xmlChar *) actLen, (const xmlChar *) len);
	
	} else if (facetType == XML_SCHEMA_FACET_ENUMERATION) {
	    msg = xmlStrcat(msg, BAD_CAST "The value '%s' is not an element "
		"of the set {%s}.\n");
	    xmlSchemaErr(actxt, error, node, (const char *) msg, value, 
		xmlSchemaFormatFacetEnumSet(actxt, &str, type));
	} else if (facetType == XML_SCHEMA_FACET_PATTERN) {
	    msg = xmlStrcat(msg, BAD_CAST "The value '%s' is not accepted "
		"by the pattern '%s'.\n");
	    xmlSchemaErr(actxt, error, node, (const char *) msg, value, 
		facet->value);
	} else if (facetType == XML_SCHEMA_FACET_MININCLUSIVE) {
	    msg = xmlStrcat(msg, BAD_CAST "The value '%s' is less than the "
		"minimum value allowed ('%s').\n");
	    xmlSchemaErr(actxt, error, node, (const char *) msg, value,
		facet->value);
	} else if (facetType == XML_SCHEMA_FACET_MAXINCLUSIVE) {
	    msg = xmlStrcat(msg, BAD_CAST "The value '%s' is greater than the "
		"maximum value allowed ('%s').\n");
	    xmlSchemaErr(actxt, error, node, (const char *) msg, value,
		facet->value);
	} else if (facetType == XML_SCHEMA_FACET_MINEXCLUSIVE) {
	    msg = xmlStrcat(msg, BAD_CAST "The value '%s' must be less than "
		"'%s'.\n");
	    xmlSchemaErr(actxt, error, node, (const char *) msg, value,
		facet->value);
	} else if (facetType == XML_SCHEMA_FACET_MAXEXCLUSIVE) {
	    msg = xmlStrcat(msg, BAD_CAST "The value '%s' must be more than "
		"'%s'.\n");
	    xmlSchemaErr(actxt, error, node, (const char *) msg, value,
		facet->value);
	} else if (facetType == XML_SCHEMA_FACET_TOTALDIGITS) {
	    msg = xmlStrcat(msg, BAD_CAST "The value '%s' has more "
		"digits than are allowed ('%s').\n");
	    xmlSchemaErr(actxt, error, node, (const char*) msg, value,
		facet->value);
	} else if (facetType == XML_SCHEMA_FACET_FRACTIONDIGITS) {
	    msg = xmlStrcat(msg, BAD_CAST "The value '%s' has more fractional "
		"digits than are allowed ('%s').\n");
	    xmlSchemaErr(actxt, error, node, (const char*) msg, value,
		facet->value);
	} else if (nodeType == XML_ATTRIBUTE_NODE) {		
	    msg = xmlStrcat(msg, BAD_CAST "The value '%s' is not facet-valid.\n");
	    xmlSchemaErr(actxt, error, node, (const char *) msg, value, NULL);	
	} else {	    
	    msg = xmlStrcat(msg, BAD_CAST "The value is not facet-valid.\n");
	    xmlSchemaErr(actxt, error, node, (const char *) msg, NULL, NULL);
	}
    } else {
	msg = xmlStrcat(msg, (const xmlChar *) message);
	msg = xmlStrcat(msg, BAD_CAST ".\n");
	xmlSchemaErr(actxt, error, node, (const char *) msg, str1, str2);
    }        
    FREE_AND_NULL(str)
    xmlFree(msg);
}

#define VERROR(err, type, msg) \
    xmlSchemaCustomErr((xmlSchemaAbstractCtxtPtr) vctxt, err, NULL, type, msg, NULL, NULL);

#define VERROR_INT(func, msg) xmlSchemaInternalErr((xmlSchemaAbstractCtxtPtr) vctxt, func, msg);

#define PERROR_INT(func, msg) xmlSchemaInternalErr((xmlSchemaAbstractCtxtPtr) pctxt, func, msg);

#define AERROR_INT(func, msg) xmlSchemaInternalErr(actxt, func, msg);


/**
 * xmlSchemaPMissingAttrErr:
 * @ctxt: the schema validation context
 * @ownerDes: the designation of  the owner
 * @ownerName: the name of the owner
 * @ownerItem: the owner as a schema object
 * @ownerElem: the owner as an element node
 * @node: the parent element node of the missing attribute node
 * @type: the corresponding type of the attribute node
 *
 * Reports an illegal attribute.
 */
static void
xmlSchemaPMissingAttrErr(xmlSchemaParserCtxtPtr ctxt,
			 xmlParserErrors error,
			 xmlSchemaTypePtr ownerItem,
			 xmlNodePtr ownerElem,
			 const char *name,
			 const char *message)
{
    xmlChar *des = NULL;

    xmlSchemaFormatItemForReport(&des, NULL, ownerItem, ownerElem);

    if (message != NULL)
	xmlSchemaPErr(ctxt, ownerElem, error, "%s: %s.\n", BAD_CAST des, BAD_CAST message);
    else
	xmlSchemaPErr(ctxt, ownerElem, error,
	    "%s: The attribute '%s' is required but missing.\n",
	    BAD_CAST des, BAD_CAST name);
    FREE_AND_NULL(des);
}


/**
 * xmlSchemaPResCompAttrErr:
 * @ctxt: the schema validation context
 * @error: the error code
 * @ownerDes: the designation of  the owner
 * @ownerItem: the owner as a schema object
 * @ownerElem: the owner as an element node
 * @name: the name of the attribute holding the QName
 * @refName: the referenced local name
 * @refURI: the referenced namespace URI
 * @message: optional message
 *
 * Used to report QName attribute values that failed to resolve
 * to schema components.
 */
static void
xmlSchemaPResCompAttrErr(xmlSchemaParserCtxtPtr ctxt,
			 xmlParserErrors error,
			 xmlSchemaTypePtr ownerItem,
			 xmlNodePtr ownerElem,
			 const char *name,
			 const xmlChar *refName,
			 const xmlChar *refURI,
			 xmlSchemaTypeType refType,
			 const char *refTypeStr)
{
    xmlChar *des = NULL, *strA = NULL;

    xmlSchemaFormatItemForReport(&des, NULL, ownerItem, ownerElem);
    if (refTypeStr == NULL)
	refTypeStr = (const char *) xmlSchemaCompTypeToString(refType);
	xmlSchemaPErrExt(ctxt, ownerElem, error,
	    NULL, NULL, NULL,
	    "%s, attribute '%s': The QName value '%s' does not resolve to a(n) "
	    "%s.\n", BAD_CAST des, BAD_CAST name,
	    xmlSchemaFormatQName(&strA, refURI, refName),
	    BAD_CAST refTypeStr, NULL);
    FREE_AND_NULL(des)
    FREE_AND_NULL(strA)
}

/**
 * xmlSchemaPCustomAttrErr:
 * @ctxt: the schema parser context
 * @error: the error code
 * @ownerDes: the designation of the owner
 * @ownerItem: the owner as a schema object
 * @attr: the illegal attribute node
 *
 * Reports an illegal attribute during the parse.
 */
static void
xmlSchemaPCustomAttrErr(xmlSchemaParserCtxtPtr ctxt,
			xmlParserErrors error,
			xmlChar **ownerDes,
			xmlSchemaTypePtr ownerItem,
			xmlAttrPtr attr,
			const char *msg)
{
    xmlChar *des = NULL;

    if (ownerDes == NULL)
	xmlSchemaFormatItemForReport(&des, NULL, ownerItem, attr->parent);
    else if (*ownerDes == NULL) {
	xmlSchemaFormatItemForReport(ownerDes, NULL, ownerItem, attr->parent);
	des = *ownerDes;
    } else
	des = *ownerDes;
    xmlSchemaPErrExt(ctxt, (xmlNodePtr) attr, error, NULL, NULL, NULL,
	"%s, attribute '%s': %s.\n",
	BAD_CAST des, attr->name, (const xmlChar *) msg, NULL, NULL);
    if (ownerDes == NULL)
	FREE_AND_NULL(des);
}

/**
 * xmlSchemaPIllegalAttrErr:
 * @ctxt: the schema parser context
 * @error: the error code
 * @ownerDes: the designation of the attribute's owner
 * @ownerItem: the attribute's owner item
 * @attr: the illegal attribute node
 *
 * Reports an illegal attribute during the parse.
 */
static void
xmlSchemaPIllegalAttrErr(xmlSchemaParserCtxtPtr ctxt,
			 xmlParserErrors error,
			 xmlChar **ownerDes,
			 xmlSchemaTypePtr ownerItem,
			 xmlAttrPtr attr)
{
    xmlChar *des = NULL, *strA = NULL;

    if (ownerDes == NULL)
	xmlSchemaFormatItemForReport(&des, NULL, ownerItem, attr->parent);
    else if (*ownerDes == NULL) {
	xmlSchemaFormatItemForReport(ownerDes, NULL, ownerItem, attr->parent);
	des = *ownerDes;
    } else
	des = *ownerDes;
    xmlSchemaPErr(ctxt, (xmlNodePtr) attr, error,
	"%s: The attribute '%s' is not allowed.\n", BAD_CAST des,
	xmlSchemaFormatQNameNs(&strA, attr->ns, attr->name));
    if (ownerDes == NULL)
	FREE_AND_NULL(des);
    FREE_AND_NULL(strA);
}

/**
 * xmlSchemaPAquireDes:
 * @des: the first designation
 * @itemDes: the second designation
 * @item: the schema item
 * @itemElem: the node of the schema item
 *
 * Creates a designation for an item.
 */
static void
xmlSchemaPAquireDes(xmlChar **des,
		    xmlChar **itemDes,
		    xmlSchemaTypePtr item,
		    xmlNodePtr itemElem)
{
    if (itemDes == NULL)
	xmlSchemaFormatItemForReport(des, NULL, item, itemElem);
    else if (*itemDes == NULL) {
	xmlSchemaFormatItemForReport(itemDes, NULL, item, itemElem);
	*des = *itemDes;
    } else
	*des = *itemDes;
}

/**
 * xmlSchemaPCustomErr:
 * @ctxt: the schema parser context
 * @error: the error code
 * @itemDes: the designation of the schema item
 * @item: the schema item
 * @itemElem: the node of the schema item
 * @message: the error message
 * @str1: an optional param for the error message
 * @str2: an optional param for the error message
 * @str3: an optional param for the error message
 *
 * Reports an error during parsing.
 */
static void
xmlSchemaPCustomErrExt(xmlSchemaParserCtxtPtr ctxt,
		    xmlParserErrors error,
		    xmlChar **itemDes,
		    xmlSchemaTypePtr item,
		    xmlNodePtr itemElem,
		    const char *message,
		    const xmlChar *str1,
		    const xmlChar *str2,
		    const xmlChar *str3)
{
    xmlChar *des = NULL, *msg = NULL;

    xmlSchemaPAquireDes(&des, itemDes, item, itemElem);
    msg = xmlStrdup(BAD_CAST "%s: ");
    msg = xmlStrcat(msg, (const xmlChar *) message);
    msg = xmlStrcat(msg, BAD_CAST ".\n");
    if ((itemElem == NULL) && (item != NULL))
	itemElem = item->node;
    xmlSchemaPErrExt(ctxt, itemElem, error, NULL, NULL, NULL,
	(const char *) msg, BAD_CAST des, str1, str2, str3, NULL);
    if (itemDes == NULL)
	FREE_AND_NULL(des);
    FREE_AND_NULL(msg);
}

/**
 * xmlSchemaPCustomErr:
 * @ctxt: the schema parser context
 * @error: the error code
 * @itemDes: the designation of the schema item
 * @item: the schema item
 * @itemElem: the node of the schema item
 * @message: the error message
 * @str1: the optional param for the error message
 *
 * Reports an error during parsing.
 */
static void
xmlSchemaPCustomErr(xmlSchemaParserCtxtPtr ctxt,
		    xmlParserErrors error,
		    xmlChar **itemDes,
		    xmlSchemaTypePtr item,
		    xmlNodePtr itemElem,
		    const char *message,
		    const xmlChar *str1)
{
    xmlSchemaPCustomErrExt(ctxt, error, itemDes, item, itemElem, message,
	str1, NULL, NULL);
}

/**
 * xmlSchemaPAttrUseErr:
 * @ctxt: the schema parser context
 * @error: the error code
 * @itemDes: the designation of the schema type
 * @item: the schema type
 * @itemElem: the node of the schema type
 * @attr: the invalid schema attribute
 * @message: the error message
 * @str1: the optional param for the error message
 *
 * Reports an attribute use error during parsing.
 */
static void
xmlSchemaPAttrUseErr(xmlSchemaParserCtxtPtr ctxt,
		    xmlParserErrors error,
		    xmlSchemaTypePtr item,
		    const xmlSchemaAttributePtr attr,
		    const char *message,
		    const xmlChar *str1)
{
    xmlChar *str = NULL, *msg = NULL;
    xmlSchemaFormatItemForReport(&msg, NULL, item, NULL);
    msg = xmlStrcat(msg, BAD_CAST ", ");
    msg = xmlStrcat(msg,
	BAD_CAST xmlSchemaFormatItemForReport(&str, NULL,
	(xmlSchemaTypePtr) attr, NULL));
    FREE_AND_NULL(str);
    msg = xmlStrcat(msg, BAD_CAST ": ");
    msg = xmlStrcat(msg, (const xmlChar *) message);
    msg = xmlStrcat(msg, BAD_CAST ".\n");
    xmlSchemaPErr(ctxt, attr->node, error,
	(const char *) msg, str1, NULL);
    xmlFree(msg);
}

/**
 * xmlSchemaPIllegalFacetAtomicErr:
 * @ctxt: the schema parser context
 * @error: the error code
 * @itemDes: the designation of the type
 * @item: the schema type
 * @baseItem: the base type of type
 * @facet: the illegal facet
 *
 * Reports an illegal facet for atomic simple types.
 */
static void
xmlSchemaPIllegalFacetAtomicErr(xmlSchemaParserCtxtPtr ctxt,
			  xmlParserErrors error,
			  xmlChar **itemDes,
			  xmlSchemaTypePtr item,
			  xmlSchemaTypePtr baseItem,
			  xmlSchemaFacetPtr facet)
{
    xmlChar *des = NULL, *strT = NULL;

    xmlSchemaPAquireDes(&des, itemDes, item, item->node);
    xmlSchemaPErrExt(ctxt, item->node, error, NULL, NULL, NULL,
	"%s: The facet '%s' is not allowed on types derived from the "
	"type %s.\n",
	BAD_CAST des, xmlSchemaFacetTypeToString(facet->type),
	xmlSchemaFormatItemForReport(&strT, NULL, baseItem, NULL),
	NULL, NULL);
    if (itemDes == NULL)
	FREE_AND_NULL(des);
    FREE_AND_NULL(strT);
}

/**
 * xmlSchemaPIllegalFacetListUnionErr:
 * @ctxt: the schema parser context
 * @error: the error code
 * @itemDes: the designation of the schema item involved
 * @item: the schema item involved
 * @facet: the illegal facet
 *
 * Reports an illegal facet for <list> and <union>.
 */
static void
xmlSchemaPIllegalFacetListUnionErr(xmlSchemaParserCtxtPtr ctxt,
			  xmlParserErrors error,
			  xmlChar **itemDes,
			  xmlSchemaTypePtr item,
			  xmlSchemaFacetPtr facet)
{
    xmlChar *des = NULL, *strT = NULL;

    xmlSchemaPAquireDes(&des, itemDes, item, item->node);
    xmlSchemaPErr(ctxt, item->node, error,
	"%s: The facet '%s' is not allowed.\n",
	BAD_CAST des, xmlSchemaFacetTypeToString(facet->type));
    if (itemDes == NULL)
	FREE_AND_NULL(des);
    FREE_AND_NULL(strT);
}

/**
 * xmlSchemaPMutualExclAttrErr:
 * @ctxt: the schema validation context
 * @error: the error code
 * @elemDes: the designation of the parent element node
 * @attr: the bad attribute node
 * @type: the corresponding type of the attribute node
 *
 * Reports an illegal attribute.
 */
static void
xmlSchemaPMutualExclAttrErr(xmlSchemaParserCtxtPtr ctxt,
			 xmlParserErrors error,
			 xmlChar **ownerDes,
			 xmlSchemaTypePtr ownerItem,
			 xmlAttrPtr attr,
			 const char *name1,
			 const char *name2)
{
    xmlChar *des = NULL;

    if (ownerDes == NULL)
	xmlSchemaFormatItemForReport(&des, NULL, ownerItem, attr->parent);
    else if (*ownerDes == NULL) {
	xmlSchemaFormatItemForReport(ownerDes, NULL, ownerItem, attr->parent);
	des = *ownerDes;
    } else
	des = *ownerDes;
    xmlSchemaPErrExt(ctxt, (xmlNodePtr) attr, error, NULL, NULL, NULL,
	"%s: The attributes '%s' and '%s' are mutually exclusive.\n",
	BAD_CAST des, BAD_CAST name1, BAD_CAST name2, NULL, NULL);
    if (ownerDes == NULL)
	FREE_AND_NULL(des)
}

/**
 * xmlSchemaPSimpleTypeErr:
 * @ctxt:  the schema validation context
 * @error: the error code
 * @type: the type specifier
 * @ownerDes: the designation of the owner
 * @ownerItem: the schema object if existent 
 * @node: the validated node
 * @value: the validated value
 *
 * Reports a simple type validation error.
 * TODO: Should this report the value of an element as well?
 */
static void
xmlSchemaPSimpleTypeErr(xmlSchemaParserCtxtPtr ctxt, 
			xmlParserErrors error,
			xmlSchemaTypePtr ownerItem ATTRIBUTE_UNUSED,
			xmlNodePtr node,
			xmlSchemaTypePtr type,
			const char *expected,
			const xmlChar *value,
			const char *message,
			const xmlChar *str1,
			const xmlChar *str2)
{
    xmlChar *msg = NULL;
    
    xmlSchemaFormatNodeForError(&msg, (xmlSchemaAbstractCtxtPtr) ctxt, node);
    if (message == NULL) {
	/*
	* Use default messages.
	*/	
	if (type != NULL) {
	    if (node->type == XML_ATTRIBUTE_NODE)
		msg = xmlStrcat(msg, BAD_CAST "'%s' is not a valid value of ");
	    else
		msg = xmlStrcat(msg, BAD_CAST "The character content is not a "
		"valid value of ");	
	    if (! xmlSchemaIsGlobalItem(type))
		msg = xmlStrcat(msg, BAD_CAST "the local ");
	    else
		msg = xmlStrcat(msg, BAD_CAST "the ");
	    
	    if (VARIETY_ATOMIC(type))
		msg = xmlStrcat(msg, BAD_CAST "atomic type");
	    else if (VARIETY_LIST(type))
		msg = xmlStrcat(msg, BAD_CAST "list type");
	    else if (VARIETY_UNION(type))
		msg = xmlStrcat(msg, BAD_CAST "union type");
	    
	    if (xmlSchemaIsGlobalItem(type)) {
		xmlChar *str = NULL;
		msg = xmlStrcat(msg, BAD_CAST " '");
		if (type->builtInType != 0) {
		    msg = xmlStrcat(msg, BAD_CAST "xs:");
		    msg = xmlStrcat(msg, type->name);
		} else 
		    msg = xmlStrcat(msg,
			xmlSchemaFormatQName(&str,
			    type->targetNamespace, type->name));
		msg = xmlStrcat(msg, BAD_CAST "'.");
		FREE_AND_NULL(str);
	    }
	} else {
	    if (node->type == XML_ATTRIBUTE_NODE)
		msg = xmlStrcat(msg, BAD_CAST "The value '%s' is not valid.");
	    else
		msg = xmlStrcat(msg, BAD_CAST "The character content is not "
		"valid.");
	}	
	if (expected) {
	    msg = xmlStrcat(msg, BAD_CAST " Expected is '");
	    msg = xmlStrcat(msg, BAD_CAST expected);
	    msg = xmlStrcat(msg, BAD_CAST "'.\n");
	} else
	    msg = xmlStrcat(msg, BAD_CAST "\n");
	if (node->type == XML_ATTRIBUTE_NODE)
	    xmlSchemaPErr(ctxt, node, error, (const char *) msg, value, NULL);
	else
	    xmlSchemaPErr(ctxt, node, error, (const char *) msg, NULL, NULL);
    } else {
	xmlSchemaPErrExt(ctxt, node, error, NULL, NULL, NULL,
	     "%s%s.\n", msg, BAD_CAST message, str1, str2, NULL);
    }
    /* Cleanup. */    
    FREE_AND_NULL(msg)
}

/**
 * xmlSchemaPContentErr:
 * @ctxt: the schema parser context
 * @error: the error code
 * @onwerDes: the designation of the holder of the content
 * @ownerItem: the owner item of the holder of the content
 * @ownerElem: the node of the holder of the content
 * @child: the invalid child node
 * @message: the optional error message
 * @content: the optional string describing the correct content
 *
 * Reports an error concerning the content of a schema element.
 */
static void
xmlSchemaPContentErr(xmlSchemaParserCtxtPtr ctxt,
		     xmlParserErrors error,
		     xmlChar **ownerDes,
		     xmlSchemaTypePtr ownerItem,
		     xmlNodePtr ownerElem,
		     xmlNodePtr child,
		     const char *message,
		     const char *content)
{
    xmlChar *des = NULL;

    if (ownerDes == NULL)
	xmlSchemaFormatItemForReport(&des, NULL, ownerItem, ownerElem);
    else if (*ownerDes == NULL) {
	xmlSchemaFormatItemForReport(ownerDes, NULL, ownerItem, ownerElem);
	des = *ownerDes;
    } else
	des = *ownerDes;
    if (message != NULL)
	xmlSchemaPErr2(ctxt, ownerElem, child, error,
	    "%s: %s.\n",
	    BAD_CAST des, BAD_CAST message);
    else {
	if (content != NULL) {
	    xmlSchemaPErr2(ctxt, ownerElem, child, error,
		"%s: The content is not valid. Expected is %s.\n",
		BAD_CAST des, BAD_CAST content);
	} else {
	    xmlSchemaPErr2(ctxt, ownerElem, child, error,
		"%s: The content is not valid.\n",
		BAD_CAST des, NULL);
	}
    }
    if (ownerDes == NULL)
	FREE_AND_NULL(des)
}

/************************************************************************
 * 									*
 * 			Streamable error functions                      *
 * 									*
 ************************************************************************/




/************************************************************************
 * 									*
 * 			Validation helper functions			*
 * 									*
 ************************************************************************/


/************************************************************************
 * 									*
 * 			Allocation functions				*
 * 									*
 ************************************************************************/

/**
 * xmlSchemaNewSchemaForParserCtxt:
 * @ctxt:  a schema validation context
 *
 * Allocate a new Schema structure.
 *
 * Returns the newly allocated structure or NULL in case or error
 */
static xmlSchemaPtr
xmlSchemaNewSchema(xmlSchemaParserCtxtPtr ctxt)
{
    xmlSchemaPtr ret;

    ret = (xmlSchemaPtr) xmlMalloc(sizeof(xmlSchema));
    if (ret == NULL) {
        xmlSchemaPErrMemory(ctxt, "allocating schema", NULL);
        return (NULL);
    }
    memset(ret, 0, sizeof(xmlSchema));
    ret->dict = ctxt->dict;
    xmlDictReference(ret->dict);

    return (ret);
}

/**
 * xmlSchemaNewSchema:
 * @ctxt:  a schema validation context
 *
 * Allocate a new Schema structure.
 *
 * Returns the newly allocated structure or NULL in case or error
 */
static xmlSchemaAssemblePtr
xmlSchemaNewAssemble(void)
{
    xmlSchemaAssemblePtr ret;

    ret = (xmlSchemaAssemblePtr) xmlMalloc(sizeof(xmlSchemaAssemble));
    if (ret == NULL) {
        /* xmlSchemaPErrMemory(ctxt, "allocating assemble info", NULL); */
        return (NULL);
    }
    memset(ret, 0, sizeof(xmlSchemaAssemble));
    ret->items = NULL;
    return (ret);
}

/**
 * xmlSchemaNewFacet:
 *
 * Allocate a new Facet structure.
 *
 * Returns the newly allocated structure or NULL in case or error
 */
xmlSchemaFacetPtr
xmlSchemaNewFacet(void)
{
    xmlSchemaFacetPtr ret;

    ret = (xmlSchemaFacetPtr) xmlMalloc(sizeof(xmlSchemaFacet));
    if (ret == NULL) {
        return (NULL);
    }
    memset(ret, 0, sizeof(xmlSchemaFacet));

    return (ret);
}

/**
 * xmlSchemaNewAnnot:
 * @ctxt:  a schema validation context
 * @node:  a node
 *
 * Allocate a new annotation structure.
 *
 * Returns the newly allocated structure or NULL in case or error
 */
static xmlSchemaAnnotPtr
xmlSchemaNewAnnot(xmlSchemaParserCtxtPtr ctxt, xmlNodePtr node)
{
    xmlSchemaAnnotPtr ret;

    ret = (xmlSchemaAnnotPtr) xmlMalloc(sizeof(xmlSchemaAnnot));
    if (ret == NULL) {
        xmlSchemaPErrMemory(ctxt, "allocating annotation", node);
        return (NULL);
    }
    memset(ret, 0, sizeof(xmlSchemaAnnot));
    ret->content = node;
    return (ret);
}

static xmlSchemaItemListPtr
xmlSchemaNewItemList(void)
{
    xmlSchemaItemListPtr ret;

    ret = xmlMalloc(sizeof(xmlSchemaItemList));
    if (ret == NULL) {
	xmlSchemaPErrMemory(NULL,
	    "allocating an item list structure", NULL);
	return (NULL);
    }
    memset(ret, 0, sizeof(xmlSchemaItemList));
    return (ret);
}

/**
 * xmlSchemaAddElementSubstitutionMember:
 * @pctxt:  a schema parser context
 * @head:  the head of the substitution group
 * @member: the new member of the substitution group
 *
 * Allocate a new annotation structure.
 *
 * Returns the newly allocated structure or NULL in case or error
 */
static int
xmlSchemaAddElementSubstitutionMember(xmlSchemaParserCtxtPtr pctxt,
				      xmlSchemaElementPtr head,
				      xmlSchemaElementPtr member)
{
    xmlSchemaSubstGroupPtr substGroup;

    if (pctxt == NULL)
	return (-1);

    if (pctxt->substGroups == NULL) {
	pctxt->substGroups = xmlHashCreateDict(10, pctxt->dict);
	if (pctxt->substGroups == NULL)
	    return (-1);
    }
    substGroup = xmlHashLookup2(pctxt->substGroups, head->name,
	head->targetNamespace);
    if (substGroup == NULL) {
	int res;

	substGroup = (xmlSchemaSubstGroupPtr) xmlMalloc(sizeof(xmlSchemaSubstGroup));
	if (substGroup == NULL) {
	    xmlSchemaPErrMemory(NULL,
		"xmlSchemaAddElementSubstitution, allocating a substitution "
		"group container",
		NULL);
	    return (-1);
	}
	substGroup->members = xmlSchemaNewItemList();
	if (substGroup->members == NULL) {
	    xmlFree(substGroup);
	    return (-1);
	}
	substGroup->head = head;

	res = xmlHashAddEntry2(pctxt->substGroups,
	    head->name, head->targetNamespace, substGroup);
	if (res != 0) {
	    xmlFree(substGroup->members);
	    xmlFree(substGroup);
	    xmlSchemaPErr(pctxt, member->node,
		XML_SCHEMAP_INTERNAL,
		"Internal error: xmlSchemaAddElementSubstitution, "
		"failed to add a new substitution group container for "
		"'%s'.\n", head->name, NULL);
	    return (-1);
	}
    }
    if (substGroup->members->items == NULL) {
	substGroup->members->items = (void **) xmlMalloc(
	    5 * sizeof(xmlSchemaElementPtr));
	if (substGroup->members->items == NULL) {
	    xmlSchemaPErrMemory(NULL,
		"allocating list of substitution group members", NULL);
	    return (-1);
	}
	substGroup->members->sizeItems = 5;
    } else if (substGroup->members->sizeItems <=
	    substGroup->members->nbItems) {
	substGroup->members->sizeItems *= 2;
	substGroup->members->items = (void **) xmlRealloc(
	    substGroup->members->items,
	    substGroup->members->sizeItems * sizeof(xmlSchemaElementPtr));
	if (substGroup->members->items == NULL) {
	    xmlSchemaPErrMemory(NULL,
		"re-allocating list of substitution group members", NULL);
	    substGroup->members->sizeItems = 0;
	    return (-1);
	}
    }
    ((xmlSchemaElementPtr *) substGroup->members->items)
	[substGroup->members->nbItems++] = (void *) member;
    return (0);
}

/**
 * xmlSchemaGetElementSubstitutionGroup:
 * @pctxt:  a schema parser context
 * @head:  the head of the substitution group
 * @member: the new member of the substitution group
 *
 * Allocate a new annotation structure.
 *
 * Returns the newly allocated structure or NULL in case or error
 */
static xmlSchemaSubstGroupPtr
xmlSchemaGetElementSubstitutionGroup(xmlSchemaParserCtxtPtr pctxt,
				     xmlSchemaElementPtr head)
{
    if (pctxt == NULL)
	return (NULL);

    if (pctxt->substGroups == NULL)
	return (NULL);

    return ((xmlSchemaSubstGroupPtr) xmlHashLookup2(pctxt->substGroups,
	head->name, head->targetNamespace));
}

/**
 * xmlSchemaFreeItemList:
 * @annot:  a schema type structure
 *
 * Deallocate a annotation structure
 */
static void
xmlSchemaFreeItemList(xmlSchemaItemListPtr list)
{
    if (list == NULL)
	return;
    if (list->items != NULL)
	xmlFree(list->items);
    xmlFree(list);
}

/**
 * xmlSchemaFreeAnnot:
 * @annot:  a schema type structure
 *
 * Deallocate a annotation structure
 */
static void
xmlSchemaFreeAnnot(xmlSchemaAnnotPtr annot)
{
    if (annot == NULL)
        return;
    xmlFree(annot);
}

/**
 * xmlSchemaFreeImport:
 * @import:  a schema import structure
 *
 * Deallocate an import structure
 */
static void
xmlSchemaFreeImport(xmlSchemaImportPtr import)
{
    if (import == NULL)
        return;

    xmlSchemaFree(import->schema);
    xmlFreeDoc(import->doc);
    xmlFree(import);
}

/**
 * xmlSchemaFreeInclude:
 * @include:  a schema include structure
 *
 * Deallocate an include structure
 */
static void
xmlSchemaFreeInclude(xmlSchemaIncludePtr include)
{
    if (include == NULL)
        return;

    xmlFreeDoc(include->doc);
    xmlFree(include);
}

/**
 * xmlSchemaFreeIncludeList:
 * @includes:  a schema include list
 *
 * Deallocate an include structure
 */
static void
xmlSchemaFreeIncludeList(xmlSchemaIncludePtr includes)
{
    xmlSchemaIncludePtr next;

    while (includes != NULL) {
        next = includes->next;
	xmlSchemaFreeInclude(includes);
	includes = next;
    }
}

/**
 * xmlSchemaFreeNotation:
 * @schema:  a schema notation structure
 *
 * Deallocate a Schema Notation structure.
 */
static void
xmlSchemaFreeNotation(xmlSchemaNotationPtr nota)
{
    if (nota == NULL)
        return;
    xmlFree(nota);
}

/**
 * xmlSchemaFreeAttribute:
 * @schema:  a schema attribute structure
 *
 * Deallocate a Schema Attribute structure.
 */
static void
xmlSchemaFreeAttribute(xmlSchemaAttributePtr attr)
{
    if (attr == NULL)
        return;
    if (attr->annot != NULL)
	xmlSchemaFreeAnnot(attr->annot);
    if (attr->defVal != NULL)
	xmlSchemaFreeValue(attr->defVal);
    xmlFree(attr);
}

/**
 * xmlSchemaFreeWildcardNsSet:
 * set:  a schema wildcard namespace
 *
 * Deallocates a list of wildcard constraint structures.
 */
static void
xmlSchemaFreeWildcardNsSet(xmlSchemaWildcardNsPtr set)
{
    xmlSchemaWildcardNsPtr next;

    while (set != NULL) {
	next = set->next;
	xmlFree(set);
	set = next;
    }
}

/**
 * xmlSchemaFreeWildcard:
 * @wildcard:  a wildcard structure
 *
 * Deallocates a wildcard structure.
 */
void
xmlSchemaFreeWildcard(xmlSchemaWildcardPtr wildcard)
{
    if (wildcard == NULL)
        return;
    if (wildcard->annot != NULL)
        xmlSchemaFreeAnnot(wildcard->annot);
    if (wildcard->nsSet != NULL)
	xmlSchemaFreeWildcardNsSet(wildcard->nsSet);
    if (wildcard->negNsSet != NULL)
	xmlFree(wildcard->negNsSet);
    xmlFree(wildcard);
}

/**
 * xmlSchemaFreeAttributeGroup:
 * @schema:  a schema attribute group structure
 *
 * Deallocate a Schema Attribute Group structure.
 */
static void
xmlSchemaFreeAttributeGroup(xmlSchemaAttributeGroupPtr attr)
{
    if (attr == NULL)
        return;
    if (attr->annot != NULL)
        xmlSchemaFreeAnnot(attr->annot);
    xmlFree(attr);
}

/**
 * xmlSchemaFreeAttributeUseList:
 * @attrUse:  an attribute link
 *
 * Deallocate a list of schema attribute uses.
 */
static void
xmlSchemaFreeAttributeUseList(xmlSchemaAttributeLinkPtr attrUse)
{
    xmlSchemaAttributeLinkPtr next;

    while (attrUse != NULL) {
	next = attrUse->next;
	xmlFree(attrUse);
	attrUse = next;
    }
}

/**
 * xmlSchemaFreeQNameRef:
 * @item: a QName reference structure
 *
 * Deallocatea a QName reference structure.
 */
static void
xmlSchemaFreeQNameRef(xmlSchemaQNameRefPtr item)
{
    xmlFree(item);
}

/**
 * xmlSchemaFreeQNameRef:
 * @item: a QName reference structure
 *
 * Deallocatea a QName reference structure.
 */
static void
xmlSchemaFreeSubstGroup(xmlSchemaSubstGroupPtr item)
{
    if (item == NULL)
	return;
    if (item->members != NULL)
	xmlSchemaFreeItemList(item->members);
    xmlFree(item);
}

static int
xmlSchemaAddVolatile(xmlSchemaPtr schema,
		     xmlSchemaBasicItemPtr item)
{
    xmlSchemaItemListPtr list;

    if (schema->volatiles == NULL) {
	schema->volatiles = (void *) xmlSchemaNewItemList();
	if (schema->volatiles == NULL) {
	    xmlSchemaPErrMemory(NULL,
		"allocating list of volatiles", NULL);
	    return (-1);
	}
    }
    list = (xmlSchemaItemListPtr) schema->volatiles;
    if (list->items == NULL) {
	list->items = (void **) xmlMalloc(
	    20 * sizeof(xmlSchemaBasicItemPtr));
	if (list->items == NULL) {
	    xmlSchemaPErrMemory(NULL,
		"allocating new volatile item buffer", NULL);
	    return (-1);
	}
	list->sizeItems = 20;
    } else if (list->sizeItems <= list->nbItems) {
	list->sizeItems *= 2;
	list->items = (void **) xmlRealloc(list->items,
	    list->sizeItems * sizeof(xmlSchemaTypePtr));
	if (list->items == NULL) {
	    xmlSchemaPErrMemory(NULL,
		"growing volatile item buffer", NULL);
	    list->sizeItems = 0;
	    return (-1);
	}
    }
    ((xmlSchemaBasicItemPtr *) list->items)[list->nbItems++] = (void *) item;
    return (0);
}

/**
 * xmlSchemaFreeTypeLinkList:
 * @alink: a type link
 *
 * Deallocate a list of types.
 */
static void
xmlSchemaFreeTypeLinkList(xmlSchemaTypeLinkPtr link)
{
    xmlSchemaTypeLinkPtr next;

    while (link != NULL) {
	next = link->next;
	xmlFree(link);
	link = next;
    }
}

static void
xmlSchemaFreeIDCStateObjList(xmlSchemaIDCStateObjPtr sto)
{
    xmlSchemaIDCStateObjPtr next;
    while (sto != NULL) {
	next = sto->next;
	if (sto->history != NULL)
	    xmlFree(sto->history);
	if (sto->xpathCtxt != NULL)
	    xmlFreeStreamCtxt((xmlStreamCtxtPtr) sto->xpathCtxt);
	xmlFree(sto);
	sto = next;
    }
}

/**
 * xmlSchemaFreeIDC:
 * @idc: a identity-constraint definition
 *
 * Deallocates an identity-constraint definition.
 */
static void
xmlSchemaFreeIDC(xmlSchemaIDCPtr idcDef)
{
    xmlSchemaIDCSelectPtr cur, prev;

    if (idcDef == NULL)
	return;
    if (idcDef->annot != NULL)
        xmlSchemaFreeAnnot(idcDef->annot);
    /* Selector */
    if (idcDef->selector != NULL) {
	if (idcDef->selector->xpathComp != NULL)
	    xmlFreePattern((xmlPatternPtr) idcDef->selector->xpathComp);
	xmlFree(idcDef->selector);
    }
    /* Fields */
    if (idcDef->fields != NULL) {
	cur = idcDef->fields;
	do {
	    prev = cur;
	    cur = cur->next;
	    if (prev->xpathComp != NULL)
		xmlFreePattern((xmlPatternPtr) prev->xpathComp);
	    xmlFree(prev);
	} while (cur != NULL);
    }
    xmlFree(idcDef);
}

/**
 * xmlSchemaFreeElement:
 * @schema:  a schema element structure
 *
 * Deallocate a Schema Element structure.
 */
static void
xmlSchemaFreeElement(xmlSchemaElementPtr elem)
{
    if (elem == NULL)
        return;
    if (elem->annot != NULL)
        xmlSchemaFreeAnnot(elem->annot);
    if (elem->contModel != NULL)
        xmlRegFreeRegexp(elem->contModel);
    if (elem->defVal != NULL)
	xmlSchemaFreeValue(elem->defVal);
    xmlFree(elem);
}

/**
 * xmlSchemaFreeFacet:
 * @facet:  a schema facet structure
 *
 * Deallocate a Schema Facet structure.
 */
void
xmlSchemaFreeFacet(xmlSchemaFacetPtr facet)
{
    if (facet == NULL)
        return;
    if (facet->val != NULL)
        xmlSchemaFreeValue(facet->val);
    if (facet->regexp != NULL)
        xmlRegFreeRegexp(facet->regexp);
    if (facet->annot != NULL)
        xmlSchemaFreeAnnot(facet->annot);
    xmlFree(facet);
}

/**
 * xmlSchemaFreeType:
 * @type:  a schema type structure
 *
 * Deallocate a Schema Type structure.
 */
void
xmlSchemaFreeType(xmlSchemaTypePtr type)
{
    if (type == NULL)
        return;
    if (type->annot != NULL)
        xmlSchemaFreeAnnot(type->annot);
    if (type->facets != NULL) {
        xmlSchemaFacetPtr facet, next;

        facet = type->facets;
        while (facet != NULL) {
            next = facet->next;
            xmlSchemaFreeFacet(facet);
            facet = next;
        }
    }
    if (type->type != XML_SCHEMA_TYPE_BASIC) {
	if (type->attributeUses != NULL)
	    xmlSchemaFreeAttributeUseList(type->attributeUses);
    }
    if (type->memberTypes != NULL)
	xmlSchemaFreeTypeLinkList(type->memberTypes);
    if (type->facetSet != NULL) {
	xmlSchemaFacetLinkPtr next, link;

	link = type->facetSet;
	do {
	    next = link->next;
	    xmlFree(link);
	    link = next;
	} while (link != NULL);
    }
    if (type->contModel != NULL)
        xmlRegFreeRegexp(type->contModel);
    xmlFree(type);
}

/**
 * xmlSchemaFreeModelGroupDef:
 * @item:  a schema model group definition
 *
 * Deallocates a schema model group definition.
 */
static void
xmlSchemaFreeModelGroupDef(xmlSchemaModelGroupDefPtr item)
{
    if (item->annot != NULL)
	xmlSchemaFreeAnnot(item->annot);
    xmlFree(item);
}

/**
 * xmlSchemaFreeModelGroup:
 * @item:  a schema model group
 *
 * Deallocates a schema model group structure.
 */
static void
xmlSchemaFreeModelGroup(xmlSchemaModelGroupPtr item)
{
    if (item->annot != NULL)
	xmlSchemaFreeAnnot(item->annot);
    xmlFree(item);
}

/**
 * xmlSchemaFreeParticle:
 * @type:  a schema type structure
 *
 * Deallocate a Schema Type structure.
 */
static void
xmlSchemaFreeParticle(xmlSchemaParticlePtr item)
{
    if (item->annot != NULL)
	xmlSchemaFreeAnnot(item->annot);
    xmlFree(item);
}

/**
 * xmlSchemaFreeMiscComponents:
 * @item:  a schema component
 *
 * Deallocates misc. schema component structures.
 */
static void
xmlSchemaFreeMiscComponents(xmlSchemaTreeItemPtr item)
{
    if (item == NULL)
        return;
    switch (item->type) {
	case XML_SCHEMA_TYPE_PARTICLE:
	    xmlSchemaFreeParticle((xmlSchemaParticlePtr) item);
	    return;
	case XML_SCHEMA_TYPE_SEQUENCE:
	case XML_SCHEMA_TYPE_CHOICE:
	case XML_SCHEMA_TYPE_ALL:
	    xmlSchemaFreeModelGroup((xmlSchemaModelGroupPtr) item);
	    return;
	case XML_SCHEMA_TYPE_ANY:
	case XML_SCHEMA_TYPE_ANY_ATTRIBUTE:
	    xmlSchemaFreeWildcard((xmlSchemaWildcardPtr) item);
	    break;
	default:
	    /* TODO: This should never be hit. */
	    TODO
	    return;
    }
}

static void
xmlSchemaFreeVolatiles(xmlSchemaPtr schema)
{
    if (schema->volatiles == NULL)
	return;
    {
	xmlSchemaItemListPtr list = (xmlSchemaItemListPtr) schema->volatiles;
	xmlSchemaTreeItemPtr item;
	int i;

	for (i = 0; i < list->nbItems; i++) {
	    if (list->items[i] != NULL) {
		item = (xmlSchemaTreeItemPtr) list->items[i];
		switch (item->type) {
		    case XML_SCHEMA_EXTRA_QNAMEREF:
			xmlSchemaFreeQNameRef((xmlSchemaQNameRefPtr) item);
			break;
		    default:
			xmlSchemaFreeMiscComponents(item);
		}
	    }
	}
	xmlSchemaFreeItemList(list);
    }
}
/**
 * xmlSchemaFreeTypeList:
 * @type:  a schema type structure
 *
 * Deallocate a Schema Type structure.
 */
static void
xmlSchemaFreeTypeList(xmlSchemaTypePtr type)
{
    xmlSchemaTypePtr next;

    while (type != NULL) {
        next = type->redef;
	xmlSchemaFreeType(type);
	type = next;
    }
}

/**
 * xmlSchemaFree:
 * @schema:  a schema structure
 *
 * Deallocate a Schema structure.
 */
void
xmlSchemaFree(xmlSchemaPtr schema)
{
    if (schema == NULL)
        return;

    if (schema->volatiles != NULL)
	xmlSchemaFreeVolatiles(schema);
    if (schema->notaDecl != NULL)
        xmlHashFree(schema->notaDecl,
                    (xmlHashDeallocator) xmlSchemaFreeNotation);
    if (schema->attrDecl != NULL)
        xmlHashFree(schema->attrDecl,
                    (xmlHashDeallocator) xmlSchemaFreeAttribute);
    if (schema->attrgrpDecl != NULL)
        xmlHashFree(schema->attrgrpDecl,
                    (xmlHashDeallocator) xmlSchemaFreeAttributeGroup);
    if (schema->elemDecl != NULL)
        xmlHashFree(schema->elemDecl,
                    (xmlHashDeallocator) xmlSchemaFreeElement);
    if (schema->typeDecl != NULL)
        xmlHashFree(schema->typeDecl,
                    (xmlHashDeallocator) xmlSchemaFreeTypeList);
    if (schema->groupDecl != NULL)
        xmlHashFree(schema->groupDecl,
                    (xmlHashDeallocator) xmlSchemaFreeModelGroupDef);
    if (schema->idcDef != NULL)
        xmlHashFree(schema->idcDef,
                    (xmlHashDeallocator) xmlSchemaFreeIDC);
    if (schema->schemasImports != NULL)
	xmlHashFree(schema->schemasImports,
		    (xmlHashDeallocator) xmlSchemaFreeImport);
    if (schema->includes != NULL) {
        xmlSchemaFreeIncludeList((xmlSchemaIncludePtr) schema->includes);
    }
    if (schema->annot != NULL)
        xmlSchemaFreeAnnot(schema->annot);
    if (schema->doc != NULL && !schema->preserve)
        xmlFreeDoc(schema->doc);
    xmlDictFree(schema->dict);
    xmlFree(schema);
}

/************************************************************************
 * 									*
 * 			Debug functions					*
 * 									*
 ************************************************************************/

#ifdef LIBXML_OUTPUT_ENABLED

/**
 * xmlSchemaElementDump:
 * @elem:  an element
 * @output:  the file output
 *
 * Dump the element
 */
static void
xmlSchemaElementDump(xmlSchemaElementPtr elem, FILE * output,
                     const xmlChar * name ATTRIBUTE_UNUSED,
		     const xmlChar * namespace ATTRIBUTE_UNUSED,
                     const xmlChar * context ATTRIBUTE_UNUSED)
{
    if (elem == NULL)
        return;

    if (elem->flags & XML_SCHEMAS_ELEM_REF) {
	fprintf(output, "Particle: %s", name);
	fprintf(output, ", term element: %s", elem->ref);
	if (elem->refNs != NULL)
	    fprintf(output, " ns %s", elem->refNs);
    } else {
	fprintf(output, "Element");
	if (elem->flags & XML_SCHEMAS_ELEM_GLOBAL)
	    fprintf(output, " (global)");
	fprintf(output, ": %s ", elem->name);
	if (namespace != NULL)
	    fprintf(output, "ns %s", namespace);
    }
    fprintf(output, "\n");
    if ((elem->minOccurs != 1) || (elem->maxOccurs != 1)) {
	fprintf(output, "  min %d ", elem->minOccurs);
        if (elem->maxOccurs >= UNBOUNDED)
            fprintf(output, "max: unbounded\n");
        else if (elem->maxOccurs != 1)
            fprintf(output, "max: %d\n", elem->maxOccurs);
        else
            fprintf(output, "\n");
    }
    /*
    * Misc other properties.
    */
    if ((elem->flags & XML_SCHEMAS_ELEM_NILLABLE) ||
	(elem->flags & XML_SCHEMAS_ELEM_ABSTRACT) ||
	(elem->flags & XML_SCHEMAS_ELEM_FIXED) ||
	(elem->flags & XML_SCHEMAS_ELEM_DEFAULT) ||
	(elem->id != NULL)) {
	fprintf(output, "  props: ");
	if (elem->flags & XML_SCHEMAS_ELEM_FIXED)
	    fprintf(output, "[fixed] ");
	if (elem->flags & XML_SCHEMAS_ELEM_DEFAULT)
	    fprintf(output, "[default] ");
	if (elem->flags & XML_SCHEMAS_ELEM_ABSTRACT)
	    fprintf(output, "[abstract] ");
	if (elem->flags & XML_SCHEMAS_ELEM_NILLABLE)
	    fprintf(output, "[nillable] ");
	if (elem->id != NULL)
	    fprintf(output, "[id: '%s'] ", elem->id);
	fprintf(output, "\n");
    }
    /*
    * Default/fixed value.
    */
    if (elem->value != NULL)
	fprintf(output, "  value: '%s'\n", elem->value);
    /*
    * Type.
    */
    if (elem->namedType != NULL) {
	fprintf(output, "  type: %s ", elem->namedType);
	if (elem->namedTypeNs != NULL)
	    fprintf(output, "ns %s\n", elem->namedTypeNs);
	else
	    fprintf(output, "\n");
    }
    /*
    * Substitution group.
    */
    if (elem->substGroup != NULL) {
	fprintf(output, "  substitutionGroup: %s ", elem->substGroup);
	if (elem->substGroupNs != NULL)
	    fprintf(output, "ns %s\n", elem->substGroupNs);
	else
	    fprintf(output, "\n");
    }
}

/**
 * xmlSchemaAnnotDump:
 * @output:  the file output
 * @annot:  a annotation
 *
 * Dump the annotation
 */
static void
xmlSchemaAnnotDump(FILE * output, xmlSchemaAnnotPtr annot)
{
    xmlChar *content;

    if (annot == NULL)
        return;

    content = xmlNodeGetContent(annot->content);
    if (content != NULL) {
        fprintf(output, "  Annot: %s\n", content);
        xmlFree(content);
    } else
        fprintf(output, "  Annot: empty\n");
}

/**
 * xmlSchemaTypeDump:
 * @output:  the file output
 * @type:  a type structure
 *
 * Dump a SchemaType structure
 */
static void
xmlSchemaContentModelDump(xmlSchemaParticlePtr particle, FILE * output, int depth)
{
    xmlChar *str = NULL;
    xmlSchemaTreeItemPtr term;
    char shift[100];
    int i;

    if (particle == NULL)
	return;
    for (i = 0;((i < depth) && (i < 25));i++)
        shift[2 * i] = shift[2 * i + 1] = ' ';
    shift[2 * i] = shift[2 * i + 1] = 0;
    fprintf(output, shift);
    if (particle->children == NULL) {
	fprintf(output, "MISSING particle term\n");
	return;
    }
    term = particle->children;
    switch (term->type) {
	case XML_SCHEMA_TYPE_ELEMENT:
	    fprintf(output, "ELEM '%s'", xmlSchemaFormatQName(&str,
		((xmlSchemaElementPtr)term)->targetNamespace,
		((xmlSchemaElementPtr)term)->name));
	    break;
	case XML_SCHEMA_TYPE_SEQUENCE:
	    fprintf(output, "SEQUENCE");
	    break;
	case XML_SCHEMA_TYPE_CHOICE:
	    fprintf(output, "CHOICE");
	    break;
	case XML_SCHEMA_TYPE_ALL:
	    fprintf(output, "ALL");
	    break;
	case XML_SCHEMA_TYPE_ANY:
	    fprintf(output, "ANY");
	    break;
	default:
	    fprintf(output, "UNKNOWN\n");
	    return;
    }
    if (particle->minOccurs != 1)
	fprintf(output, " min: %d", particle->minOccurs);
    if (particle->maxOccurs >= UNBOUNDED)
	fprintf(output, " max: unbounded");
    else if (particle->maxOccurs != 1)
	fprintf(output, " max: %d", particle->maxOccurs);
    fprintf(output, "\n");
    if (((term->type == XML_SCHEMA_TYPE_SEQUENCE) ||
	(term->type == XML_SCHEMA_TYPE_CHOICE) ||
	(term->type == XML_SCHEMA_TYPE_ALL)) &&
	(term->children != NULL)) {
	xmlSchemaContentModelDump((xmlSchemaParticlePtr) term->children,
	    output, depth +1);
    }
    if (particle->next != NULL)
	xmlSchemaContentModelDump((xmlSchemaParticlePtr) particle->next,
		output, depth);
}
/**
 * xmlSchemaTypeDump:
 * @output:  the file output
 * @type:  a type structure
 *
 * Dump a SchemaType structure
 */
static void
xmlSchemaTypeDump(xmlSchemaTypePtr type, FILE * output)
{
    if (type == NULL) {
        fprintf(output, "Type: NULL\n");
        return;
    }
    fprintf(output, "Type: ");
    if (type->name != NULL)
        fprintf(output, "%s ", type->name);
    else
        fprintf(output, "no name ");
    if (type->targetNamespace != NULL)
	fprintf(output, "ns %s ", type->targetNamespace);
    switch (type->type) {
        case XML_SCHEMA_TYPE_BASIC:
            fprintf(output, "[basic] ");
            break;
        case XML_SCHEMA_TYPE_SIMPLE:
            fprintf(output, "[simple] ");
            break;
        case XML_SCHEMA_TYPE_COMPLEX:
            fprintf(output, "[complex] ");
            break;
        case XML_SCHEMA_TYPE_SEQUENCE:
            fprintf(output, "[sequence] ");
            break;
        case XML_SCHEMA_TYPE_CHOICE:
            fprintf(output, "[choice] ");
            break;
        case XML_SCHEMA_TYPE_ALL:
            fprintf(output, "[all] ");
            break;
        case XML_SCHEMA_TYPE_UR:
            fprintf(output, "[ur] ");
            break;
        case XML_SCHEMA_TYPE_RESTRICTION:
            fprintf(output, "[restriction] ");
            break;
        case XML_SCHEMA_TYPE_EXTENSION:
            fprintf(output, "[extension] ");
            break;
        default:
            fprintf(output, "[unknown type %d] ", type->type);
            break;
    }
    fprintf(output, "content: ");
    switch (type->contentType) {
        case XML_SCHEMA_CONTENT_UNKNOWN:
            fprintf(output, "[unknown] ");
            break;
        case XML_SCHEMA_CONTENT_EMPTY:
            fprintf(output, "[empty] ");
            break;
        case XML_SCHEMA_CONTENT_ELEMENTS:
            fprintf(output, "[element] ");
            break;
        case XML_SCHEMA_CONTENT_MIXED:
            fprintf(output, "[mixed] ");
            break;
        case XML_SCHEMA_CONTENT_MIXED_OR_ELEMENTS:
	/* not used. */
            break;
        case XML_SCHEMA_CONTENT_BASIC:
            fprintf(output, "[basic] ");
            break;
        case XML_SCHEMA_CONTENT_SIMPLE:
            fprintf(output, "[simple] ");
            break;
        case XML_SCHEMA_CONTENT_ANY:
            fprintf(output, "[any] ");
            break;
    }
    fprintf(output, "\n");
    if (type->base != NULL) {
        fprintf(output, "  base type: %s", type->base);
	if (type->baseNs != NULL)
	    fprintf(output, " ns %s\n", type->baseNs);
	else
	    fprintf(output, "\n");
    }
    if (type->annot != NULL)
        xmlSchemaAnnotDump(output, type->annot);
#ifdef DUMP_CONTENT_MODEL
    if ((type->type == XML_SCHEMA_TYPE_COMPLEX) &&
	(type->subtypes != NULL)) {
	xmlSchemaContentModelDump((xmlSchemaParticlePtr) type->subtypes,
	    output, 1);
    }
#endif
}

/**
 * xmlSchemaDump:
 * @output:  the file output
 * @schema:  a schema structure
 *
 * Dump a Schema structure.
 */
void
xmlSchemaDump(FILE * output, xmlSchemaPtr schema)
{
    if (output == NULL)
        return;
    if (schema == NULL) {
        fprintf(output, "Schemas: NULL\n");
        return;
    }
    fprintf(output, "Schemas: ");
    if (schema->name != NULL)
        fprintf(output, "%s, ", schema->name);
    else
        fprintf(output, "no name, ");
    if (schema->targetNamespace != NULL)
        fprintf(output, "%s", (const char *) schema->targetNamespace);
    else
        fprintf(output, "no target namespace");
    fprintf(output, "\n");
    if (schema->annot != NULL)
        xmlSchemaAnnotDump(output, schema->annot);

    xmlHashScan(schema->typeDecl, (xmlHashScanner) xmlSchemaTypeDump,
                output);
    xmlHashScanFull(schema->elemDecl,
                    (xmlHashScannerFull) xmlSchemaElementDump, output);
}

#ifdef DEBUG_IDC
/**
 * xmlSchemaDebugDumpIDCTable:
 * @vctxt: the WXS validation context
 *
 * Displays the current IDC table for debug purposes.
 */
static void
xmlSchemaDebugDumpIDCTable(FILE * output,
			   const xmlChar *namespaceName,
			   const xmlChar *localName,
			   xmlSchemaPSVIIDCBindingPtr bind)
{
    xmlChar *str = NULL, *value;
    xmlSchemaPSVIIDCNodePtr tab;
    xmlSchemaPSVIIDCKeyPtr key;
    int i, j, res;

    fprintf(output, "IDC: TABLES on %s\n",
	xmlSchemaFormatQName(&str, namespaceName, localName));
    FREE_AND_NULL(str)

    if (bind == NULL)
	return;
    do {
	fprintf(output, "IDC:   BINDING %s\n",
	    xmlSchemaFormatQName(&str, bind->definition->targetNamespace,
	    bind->definition->name));
	FREE_AND_NULL(str)
	for (i = 0; i < bind->nbNodes; i++) {
	    tab = bind->nodeTable[i];
	    fprintf(output, "         ( ");
	    for (j = 0; j < bind->definition->nbFields; j++) {
		key = tab->keys[j];
		if ((key != NULL) && (key->val != NULL)) {
		    res = xmlSchemaGetCanonValue(key->val, &value);
		    if (res >= 0)
			fprintf(output, "\"%s\" ", value);
		    else
			fprintf(output, "CANON-VALUE-FAILED ");
		    if (res == 0)
			FREE_AND_NULL(value)
		} else if (key != NULL)
		    fprintf(output, "(no val), ");
		else
		    fprintf(output, "(key missing), ");
	    }
	    fprintf(output, ")\n");
	}
	bind = bind->next;
    } while (bind != NULL);
}
#endif /* DEBUG_IDC */
#endif /* LIBXML_OUTPUT_ENABLED */

/************************************************************************
 *									*
 * 			Utilities					*
 *									*
 ************************************************************************/

/**
 * xmlSchemaGetPropNode:
 * @node: the element node
 * @name: the name of the attribute
 *
 * Seeks an attribute with a name of @name in
 * no namespace.
 *
 * Returns the attribute or NULL if not present.
 */
static xmlAttrPtr
xmlSchemaGetPropNode(xmlNodePtr node, const char *name)
{
    xmlAttrPtr prop;

    if ((node == NULL) || (name == NULL))
	return(NULL);
    prop = node->properties;
    while (prop != NULL) {
        if ((prop->ns == NULL) && xmlStrEqual(prop->name, BAD_CAST name))
	    return(prop);
	prop = prop->next;
    }
    return (NULL);
}

/**
 * xmlSchemaGetPropNodeNs:
 * @node: the element node
 * @uri: the uri
 * @name: the name of the attribute
 *
 * Seeks an attribute with a local name of @name and
 * a namespace URI of @uri.
 *
 * Returns the attribute or NULL if not present.
 */
static xmlAttrPtr
xmlSchemaGetPropNodeNs(xmlNodePtr node, const char *uri, const char *name)
{
    xmlAttrPtr prop;

    if ((node == NULL) || (name == NULL))
	return(NULL);
    prop = node->properties;
    while (prop != NULL) {
	if ((prop->ns != NULL) &&
	    xmlStrEqual(prop->name, BAD_CAST name) &&
	    xmlStrEqual(prop->ns->href, BAD_CAST uri))
	    return(prop);
	prop = prop->next;
    }
    return (NULL);
}

static const xmlChar *
xmlSchemaGetNodeContent(xmlSchemaParserCtxtPtr ctxt, xmlNodePtr node)
{
    xmlChar *val;
    const xmlChar *ret;

    val = xmlNodeGetContent(node);
    if (val == NULL)
	val = xmlStrdup((xmlChar *)"");
    ret = xmlDictLookup(ctxt->dict, val, -1);
    xmlFree(val);
    return(ret);
}

/**
 * xmlSchemaGetProp:
 * @ctxt: the parser context
 * @node: the node
 * @name: the property name
 *
 * Read a attribute value and internalize the string
 *
 * Returns the string or NULL if not present.
 */
static const xmlChar *
xmlSchemaGetProp(xmlSchemaParserCtxtPtr ctxt, xmlNodePtr node,
                 const char *name)
{
    xmlChar *val;
    const xmlChar *ret;

    val = xmlGetProp(node, BAD_CAST name);
    if (val == NULL)
        return(NULL);
    ret = xmlDictLookup(ctxt->dict, val, -1);
    xmlFree(val);
    return(ret);
}

/************************************************************************
 * 									*
 * 			Parsing functions				*
 * 									*
 ************************************************************************/

/**
 * xmlSchemaGetElem:
 * @schema:  the schema context
 * @name:  the element name
 * @ns:  the element namespace
 *
 * Lookup a global element declaration in the schema.
 *
 * Returns the element declaration or NULL if not found.
 */
static xmlSchemaElementPtr
xmlSchemaGetElem(xmlSchemaPtr schema, const xmlChar * name,
                 const xmlChar * namespace)
{
    xmlSchemaElementPtr ret;

    if ((name == NULL) || (schema == NULL))
        return (NULL);

        ret = xmlHashLookup2(schema->elemDecl, name, namespace);
        if ((ret != NULL) &&
	    (ret->flags & XML_SCHEMAS_ELEM_GLOBAL)) {
            return (ret);
    } else
	ret = NULL;
    /*
     * This one was removed, since top level element declarations have
     * the target namespace specified in targetNamespace of the <schema>
     * information element, even if elementFormDefault is "unqualified".
     */

    /* else if ((schema->flags & XML_SCHEMAS_QUALIF_ELEM) == 0) {
        if (xmlStrEqual(namespace, schema->targetNamespace))
	    ret = xmlHashLookup2(schema->elemDecl, name, NULL);
	else
	    ret = xmlHashLookup2(schema->elemDecl, name, namespace);
        if ((ret != NULL) &&
	    ((level == 0) || (ret->flags & XML_SCHEMAS_ELEM_TOPLEVEL))) {
            return (ret);
	}
    */

    /*
    * Removed since imported components will be hold by the main schema only.
    *
    if (namespace == NULL)
	import = xmlHashLookup(schema->schemasImports, XML_SCHEMAS_NO_NAMESPACE);
    else
    import = xmlHashLookup(schema->schemasImports, namespace);
    if (import != NULL) {
	ret = xmlSchemaGetElem(import->schema, name, namespace, level + 1);
	if ((ret != NULL) && (ret->flags & XML_SCHEMAS_ELEM_GLOBAL)) {
	    return (ret);
	} else
	    ret = NULL;
    }
    */
#ifdef DEBUG
    if (ret == NULL) {
        if (namespace == NULL)
            fprintf(stderr, "Unable to lookup element decl. %s", name);
        else
            fprintf(stderr, "Unable to lookup element decl. %s:%s", name,
                    namespace);
    }
#endif
    return (ret);
}

/**
 * xmlSchemaGetType:
 * @schema:  the schemas context
 * @name:  the type name
 * @ns:  the type namespace
 *
 * Lookup a type in the schemas or the predefined types
 *
 * Returns the group definition or NULL if not found.
 */
static xmlSchemaTypePtr
xmlSchemaGetType(xmlSchemaPtr schema, const xmlChar * name,
                 const xmlChar * namespace)
{
    xmlSchemaTypePtr ret;

    if (name == NULL)
        return (NULL);
    if (schema != NULL) {
        ret = xmlHashLookup2(schema->typeDecl, name, namespace);
        if ((ret != NULL) && (ret->flags & XML_SCHEMAS_TYPE_GLOBAL))
            return (ret);
    }
    ret = xmlSchemaGetPredefinedType(name, namespace);
    if (ret != NULL)
	return (ret);
    /*
    * Removed, since the imported components will be grafted on the
    * main schema only.
    if (namespace == NULL)
	import = xmlHashLookup(schema->schemasImports, XML_SCHEMAS_NO_NAMESPACE);
    else
    import = xmlHashLookup(schema->schemasImports, namespace);
    if (import != NULL) {
	ret = xmlSchemaGetType(import->schema, name, namespace);
	if ((ret != NULL) && (ret->flags & XML_SCHEMAS_TYPE_GLOBAL)) {
	    return (ret);
	} else
	    ret = NULL;
    }
    */
#ifdef DEBUG
    if (ret == NULL) {
        if (namespace == NULL)
            fprintf(stderr, "Unable to lookup type %s", name);
        else
            fprintf(stderr, "Unable to lookup type %s:%s", name,
                    namespace);
    }
#endif
    return (ret);
}

/**
 * xmlSchemaGetAttributeDecl:
 * @schema:  the context of the schema
 * @name:  the name of the attribute
 * @ns:  the target namespace of the attribute
 *
 * Lookup a an attribute in the schema or imported schemas
 *
 * Returns the attribute declaration or NULL if not found.
 */
static xmlSchemaAttributePtr
xmlSchemaGetAttributeDecl(xmlSchemaPtr schema, const xmlChar * name,
                 const xmlChar * namespace)
{
    xmlSchemaAttributePtr ret;

    if ((name == NULL) || (schema == NULL))
        return (NULL);


    ret = xmlHashLookup2(schema->attrDecl, name, namespace);
    if ((ret != NULL) && (ret->flags & XML_SCHEMAS_ATTR_GLOBAL))
	return (ret);
    else
	ret = NULL;
    /*
    * Removed, since imported components will be hold by the main schema only.
    *
    if (namespace == NULL)
	import = xmlHashLookup(schema->schemasImports, XML_SCHEMAS_NO_NAMESPACE);
    else
	import = xmlHashLookup(schema->schemasImports, namespace);
    if (import != NULL) {
	ret = xmlSchemaGetAttributeDecl(import->schema, name, namespace);
	if ((ret != NULL) && (ret->flags & XML_SCHEMAS_ATTR_GLOBAL)) {
	    return (ret);
	} else
	    ret = NULL;
    }
    */
#ifdef DEBUG
    if (ret == NULL) {
        if (namespace == NULL)
            fprintf(stderr, "Unable to lookup attribute %s", name);
        else
            fprintf(stderr, "Unable to lookup attribute %s:%s", name,
                    namespace);
    }
#endif
    return (ret);
}

/**
 * xmlSchemaGetAttributeGroup:
 * @schema:  the context of the schema
 * @name:  the name of the attribute group
 * @ns:  the target namespace of the attribute group
 *
 * Lookup a an attribute group in the schema or imported schemas
 *
 * Returns the attribute group definition or NULL if not found.
 */
static xmlSchemaAttributeGroupPtr
xmlSchemaGetAttributeGroup(xmlSchemaPtr schema, const xmlChar * name,
                 const xmlChar * namespace)
{
    xmlSchemaAttributeGroupPtr ret;

    if ((name == NULL) || (schema == NULL))
        return (NULL);


    ret = xmlHashLookup2(schema->attrgrpDecl, name, namespace);
    if ((ret != NULL) && (ret->flags & XML_SCHEMAS_ATTRGROUP_GLOBAL))
	return (ret);
    else
	ret = NULL;
    /*
    * Removed since imported components will be hold by the main schema only.
    *
    if (namespace == NULL)
	import = xmlHashLookup(schema->schemasImports, XML_SCHEMAS_NO_NAMESPACE);
    else
	import = xmlHashLookup(schema->schemasImports, namespace);
    if (import != NULL) {
	ret = xmlSchemaGetAttributeGroup(import->schema, name, namespace);
	if ((ret != NULL) && (ret->flags & XML_SCHEMAS_ATTRGROUP_GLOBAL))
	    return (ret);
	else
	    ret = NULL;
    }
    */
#ifdef DEBUG
    if (ret == NULL) {
        if (namespace == NULL)
            fprintf(stderr, "Unable to lookup attribute group %s", name);
        else
            fprintf(stderr, "Unable to lookup attribute group %s:%s", name,
                    namespace);
    }
#endif
    return (ret);
}

/**
 * xmlSchemaGetGroup:
 * @schema:  the context of the schema
 * @name:  the name of the group
 * @ns:  the target namespace of the group
 *
 * Lookup a group in the schema or imported schemas
 *
 * Returns the group definition or NULL if not found.
 */
static xmlSchemaTypePtr
xmlSchemaGetGroup(xmlSchemaPtr schema, const xmlChar * name,
                 const xmlChar * namespace)
{
    xmlSchemaTypePtr ret;

    if ((name == NULL) || (schema == NULL))
        return (NULL);

    ret = xmlHashLookup2(schema->groupDecl, name, namespace);
    /*
    * Removed since imported components will be hold by the main schema only.
    *
    if (namespace == NULL)
	import = xmlHashLookup(schema->schemasImports, XML_SCHEMAS_NO_NAMESPACE);
    else
	import = xmlHashLookup(schema->schemasImports, namespace);
    if (import != NULL) {
	ret = xmlSchemaGetGroup(import->schema, name, namespace);
	if ((ret != NULL) && (ret->flags & XML_SCHEMAS_TYPE_GLOBAL))
	    return (ret);
	else
	    ret = NULL;
    }
    */
#ifdef DEBUG
    if (ret == NULL) {
        if (namespace == NULL)
            fprintf(stderr, "Unable to lookup group %s", name);
        else
            fprintf(stderr, "Unable to lookup group %s:%s", name,
                    namespace);
    }
#endif
    return (ret);
}

/**
 * xmlSchemaGetNamedComponent:
 * @schema:  the schema
 * @name:  the name of the group
 * @ns:  the target namespace of the group
 *
 * Lookup a group in the schema or imported schemas
 *
 * Returns the group definition or NULL if not found.
 */
static xmlSchemaTreeItemPtr
xmlSchemaGetNamedComponent(xmlSchemaPtr schema,
			   xmlSchemaTypeType itemType,
			   const xmlChar *name,
			   const xmlChar *targetNs)
{
    switch (itemType) {
	case XML_SCHEMA_TYPE_GROUP:
	    return ((xmlSchemaTreeItemPtr) xmlSchemaGetGroup(schema,
		name, targetNs));
	case XML_SCHEMA_TYPE_ELEMENT:
	    return ((xmlSchemaTreeItemPtr) xmlSchemaGetElem(schema,
		name, targetNs));
	default:
	    return (NULL);
    }
}

/************************************************************************
 * 									*
 * 			Parsing functions				*
 * 									*
 ************************************************************************/

#define IS_BLANK_NODE(n)						\
    (((n)->type == XML_TEXT_NODE) && (xmlSchemaIsBlank((n)->content, -1)))

/**
 * xmlSchemaIsBlank:
 * @str:  a string
 * @len: the length of the string or -1
 *
 * Check if a string is ignorable
 *
 * Returns 1 if the string is NULL or made of blanks chars, 0 otherwise
 */
static int
xmlSchemaIsBlank(xmlChar * str, int len)
{
    if (str == NULL)
        return (1);
    if (len < 0) {
	while (*str != 0) {
	    if (!(IS_BLANK_CH(*str)))
		return (0);
	    str++;
	}
    } else while ((*str != 0) && (len != 0)) {
	if (!(IS_BLANK_CH(*str)))
	    return (0);
	str++;
	len--;
    }
    
    return (1);
}

/**
 * xmlSchemaAddAssembledItem:
 * @ctxt:  a schema parser context
 * @schema:  the schema being built
 * @item:  the item
 *
 * Add a item to the schema's list of current items.
 * This is used if the schema was already constructed and
 * new schemata need to be added to it.
 * *WARNING* this interface is highly subject to change.
 *
 * Returns 0 if suceeds and -1 if an internal error occurs.
 */
static int
xmlSchemaAddAssembledItem(xmlSchemaParserCtxtPtr ctxt,
			   xmlSchemaTypePtr item)
{
    static int growSize = 100;
    xmlSchemaAssemblePtr ass;

    ass = ctxt->assemble;
    if (ass->sizeItems < 0) {
	/* If disabled. */
	return (0);
    }
    if (ass->sizeItems <= 0) {
	ass->items = (void **) xmlMalloc(growSize * sizeof(xmlSchemaTypePtr));
	if (ass->items == NULL) {
	    xmlSchemaPErrMemory(ctxt,
		"allocating new item buffer", NULL);
	    return (-1);
	}
	ass->sizeItems = growSize;
    } else if (ass->sizeItems <= ass->nbItems) {
	ass->sizeItems *= 2;
	ass->items = (void **) xmlRealloc(ass->items,
	    ass->sizeItems * sizeof(xmlSchemaTypePtr));
	if (ass->items == NULL) {
	    xmlSchemaPErrMemory(ctxt,
		"growing item buffer", NULL);
	    ass->sizeItems = 0;
	    return (-1);
	}
    }
    /* ass->items[ass->nbItems++] = (void *) item; */
    ((xmlSchemaTypePtr *) ass->items)[ass->nbItems++] = (void *) item;
    return (0);
}

/**
 * xmlSchemaAddNotation:
 * @ctxt:  a schema parser context
 * @schema:  the schema being built
 * @name:  the item name
 *
 * Add an XML schema annotation declaration
 * *WARNING* this interface is highly subject to change
 *
 * Returns the new struture or NULL in case of error
 */
static xmlSchemaNotationPtr
xmlSchemaAddNotation(xmlSchemaParserCtxtPtr ctxt, xmlSchemaPtr schema,
                     const xmlChar *name)
{
    xmlSchemaNotationPtr ret = NULL;
    int val;

    if ((ctxt == NULL) || (schema == NULL) || (name == NULL))
        return (NULL);

    if (schema->notaDecl == NULL)
        schema->notaDecl = xmlHashCreateDict(10, ctxt->dict);
    if (schema->notaDecl == NULL)
        return (NULL);

    ret = (xmlSchemaNotationPtr) xmlMalloc(sizeof(xmlSchemaNotation));
    if (ret == NULL) {
        xmlSchemaPErrMemory(ctxt, "add annotation", NULL);
        return (NULL);
    }
    memset(ret, 0, sizeof(xmlSchemaNotation));
    ret->name = xmlDictLookup(ctxt->dict, name, -1);
    val = xmlHashAddEntry2(schema->notaDecl, name, schema->targetNamespace,
                           ret);
    if (val != 0) {
	/*
	* TODO: This should never happen, since a unique name will be computed.
	* If it fails, then an other internal error must have occured.
	*/
	xmlSchemaPErr(ctxt, (xmlNodePtr) ctxt->doc,
		      XML_SCHEMAP_REDEFINED_NOTATION,
                      "Annotation declaration '%s' is already declared.\n",
                      name, NULL);
        xmlFree(ret);
        return (NULL);
    }
    return (ret);
}


/**
 * xmlSchemaAddAttribute:
 * @ctxt:  a schema parser context
 * @schema:  the schema being built
 * @name:  the item name
 * @namespace:  the namespace
 *
 * Add an XML schema Attrribute declaration
 * *WARNING* this interface is highly subject to change
 *
 * Returns the new struture or NULL in case of error
 */
static xmlSchemaAttributePtr
xmlSchemaAddAttribute(xmlSchemaParserCtxtPtr ctxt, xmlSchemaPtr schema,
                      const xmlChar * name, const xmlChar * namespace,
		      xmlNodePtr node, int topLevel)
{
    xmlSchemaAttributePtr ret = NULL;
    int val;

    if ((ctxt == NULL) || (schema == NULL) || (name == NULL))
        return (NULL);

#ifdef DEBUG
    fprintf(stderr, "Adding attribute %s\n", name);
    if (namespace != NULL)
	fprintf(stderr, "  target namespace %s\n", namespace);
#endif

    if (schema->attrDecl == NULL)
        schema->attrDecl = xmlHashCreateDict(10, ctxt->dict);
    if (schema->attrDecl == NULL)
        return (NULL);

    ret = (xmlSchemaAttributePtr) xmlMalloc(sizeof(xmlSchemaAttribute));
    if (ret == NULL) {
        xmlSchemaPErrMemory(ctxt, "allocating attribute", NULL);
        return (NULL);
    }
    memset(ret, 0, sizeof(xmlSchemaAttribute));
    ret->name = xmlDictLookup(ctxt->dict, name, -1);
    ret->targetNamespace = namespace;
    val = xmlHashAddEntry3(schema->attrDecl, name,
                           namespace, ctxt->container, ret);
    if (val != 0) {
	if (topLevel) {
	    xmlSchemaPCustomErr(ctxt,
		XML_SCHEMAP_REDEFINED_ATTR,
		NULL, NULL, node,
		"A global attribute declaration with the name '%s' does "
		"already exist", name);
	    xmlFree(ret);
	    return (NULL);
	} else {
	    char buf[30];
	    /*
	    * Using the ctxt->container for xmlHashAddEntry3 is ambigious
	    * in the scenario:
	    * 1. multiple top-level complex types have different target
	    *    namespaces but have the SAME NAME; this can happen if
	    *	 schemata are  imported
	    * 2. those complex types contain attributes with an equal name
	    * 3. those attributes are in no namespace
	    * We will compute a new context string.
	    */
	    snprintf(buf, 29, "#aCont%d", ctxt->counter++ + 1);
	    val = xmlHashAddEntry3(schema->attrDecl, name,
		namespace, xmlDictLookup(ctxt->dict, BAD_CAST buf, -1), ret);

	    if (val != 0) {
		xmlSchemaPCustomErr(ctxt,
		    XML_SCHEMAP_INTERNAL,
		    NULL, NULL, node,
		    "Internal error: xmlSchemaAddAttribute, "
		    "a dublicate attribute declaration with the name '%s' "
		    "could not be added to the hash.", name);
		xmlFree(ret);
		return (NULL);
	    }
	}
    }
    if (ctxt->assemble != NULL)
	xmlSchemaAddAssembledItem(ctxt, (xmlSchemaTypePtr) ret);
    return (ret);
}

/**
 * xmlSchemaAddAttributeGroup:
 * @ctxt:  a schema parser context
 * @schema:  the schema being built
 * @name:  the item name
 *
 * Add an XML schema Attrribute Group declaration
 *
 * Returns the new struture or NULL in case of error
 */
static xmlSchemaAttributeGroupPtr
xmlSchemaAddAttributeGroup(xmlSchemaParserCtxtPtr ctxt,
                           xmlSchemaPtr schema, const xmlChar * name,
			   xmlNodePtr node)
{
    xmlSchemaAttributeGroupPtr ret = NULL;
    int val;

    if ((ctxt == NULL) || (schema == NULL) || (name == NULL))
        return (NULL);

    if (schema->attrgrpDecl == NULL)
        schema->attrgrpDecl = xmlHashCreateDict(10, ctxt->dict);
    if (schema->attrgrpDecl == NULL)
        return (NULL);

    ret =
        (xmlSchemaAttributeGroupPtr)
        xmlMalloc(sizeof(xmlSchemaAttributeGroup));
    if (ret == NULL) {
        xmlSchemaPErrMemory(ctxt, "allocating attribute group", NULL);
        return (NULL);
    }
    memset(ret, 0, sizeof(xmlSchemaAttributeGroup));
    ret->name = xmlDictLookup(ctxt->dict, name, -1);
    val = xmlHashAddEntry3(schema->attrgrpDecl, name,
                           schema->targetNamespace, ctxt->container, ret);
    if (val != 0) {
	xmlSchemaPCustomErr(ctxt,
	    XML_SCHEMAP_REDEFINED_ATTRGROUP,
	    NULL, NULL, node,
	    "A global attribute group definition with the name '%s' does already exist", name);
        xmlFree(ret);
        return (NULL);
    }
    if (ctxt->assemble != NULL)
	xmlSchemaAddAssembledItem(ctxt, (xmlSchemaTypePtr) ret);
    return (ret);
}

/**
 * xmlSchemaAddElement:
 * @ctxt:  a schema parser context
 * @schema:  the schema being built
 * @name:  the type name
 * @namespace:  the type namespace
 *
 * Add an XML schema Element declaration
 * *WARNING* this interface is highly subject to change
 *
 * Returns the new struture or NULL in case of error
 */
static xmlSchemaElementPtr
xmlSchemaAddElement(xmlSchemaParserCtxtPtr ctxt, xmlSchemaPtr schema,
                    const xmlChar * name, const xmlChar * namespace,
		    xmlNodePtr node, int topLevel)
{
    xmlSchemaElementPtr ret = NULL;
    int val;

    if ((ctxt == NULL) || (schema == NULL) || (name == NULL))
        return (NULL);

#ifdef DEBUG
    fprintf(stderr, "Adding element %s\n", name);
    if (namespace != NULL)
	fprintf(stderr, "  target namespace %s\n", namespace);
#endif

    if (schema->elemDecl == NULL)
        schema->elemDecl = xmlHashCreateDict(10, ctxt->dict);
    if (schema->elemDecl == NULL)
        return (NULL);

    ret = (xmlSchemaElementPtr) xmlMalloc(sizeof(xmlSchemaElement));
    if (ret == NULL) {
        xmlSchemaPErrMemory(ctxt, "allocating element", NULL);
        return (NULL);
    }
    memset(ret, 0, sizeof(xmlSchemaElement));
    ret->name = xmlDictLookup(ctxt->dict, name, -1);
    val = xmlHashAddEntry3(schema->elemDecl, name,
                           namespace, ctxt->container, ret);
    if (val != 0) {
	if (topLevel) {
	    xmlSchemaPCustomErr(ctxt,
		XML_SCHEMAP_REDEFINED_ELEMENT,
		NULL, NULL, node,
		"A global element declaration with the name '%s' does "
		"already exist", name);
            xmlFree(ret);
            return (NULL);
	} else {
	    char buf[30];

	    snprintf(buf, 29, "#eCont%d", ctxt->counter++ + 1);
	    val = xmlHashAddEntry3(schema->elemDecl, name,
		namespace, (xmlChar *) buf, ret);
	    if (val != 0) {
		xmlSchemaPCustomErr(ctxt,
		    XML_SCHEMAP_INTERNAL,
		    NULL, NULL, node,
		    "Internal error: xmlSchemaAddElement, "
		    "a dublicate element declaration with the name '%s' "
		    "could not be added to the hash.", name);
		xmlFree(ret);
		return (NULL);
	    }
	}

    }
    if (ctxt->assemble != NULL)
	xmlSchemaAddAssembledItem(ctxt, (xmlSchemaTypePtr) ret);
    return (ret);
}

/**
 * xmlSchemaAddType:
 * @ctxt:  a schema parser context
 * @schema:  the schema being built
 * @name:  the item name
 * @namespace:  the namespace
 *
 * Add an XML schema item
 * *WARNING* this interface is highly subject to change
 *
 * Returns the new struture or NULL in case of error
 */
static xmlSchemaTypePtr
xmlSchemaAddType(xmlSchemaParserCtxtPtr ctxt, xmlSchemaPtr schema,
                 const xmlChar * name, const xmlChar * namespace,
		 xmlNodePtr node)
{
    xmlSchemaTypePtr ret = NULL;
    int val;

    if ((ctxt == NULL) || (schema == NULL) || (name == NULL))
        return (NULL);

#ifdef DEBUG
    fprintf(stderr, "Adding type %s\n", name);
    if (namespace != NULL)
	fprintf(stderr, "  target namespace %s\n", namespace);
#endif

    if (schema->typeDecl == NULL)
        schema->typeDecl = xmlHashCreateDict(10, ctxt->dict);
    if (schema->typeDecl == NULL)
        return (NULL);

    ret = (xmlSchemaTypePtr) xmlMalloc(sizeof(xmlSchemaType));
    if (ret == NULL) {
        xmlSchemaPErrMemory(ctxt, "allocating type", NULL);
        return (NULL);
    }
    memset(ret, 0, sizeof(xmlSchemaType));
    ret->name = xmlDictLookup(ctxt->dict, name, -1);
    ret->redef = NULL;
    val = xmlHashAddEntry2(schema->typeDecl, name, namespace, ret);
    if (val != 0) {
        if (ctxt->includes == 0) {
	    xmlSchemaPCustomErr(ctxt,
		XML_SCHEMAP_REDEFINED_TYPE,
		NULL, NULL, node,
		"A global type definition with the name '%s' does already exist", name);
	    xmlFree(ret);
	    return (NULL);
	} else {
	    xmlSchemaTypePtr prev;

	    prev = xmlHashLookup2(schema->typeDecl, name, namespace);
	    if (prev == NULL) {
		xmlSchemaPErr(ctxt, (xmlNodePtr) ctxt->doc,
		    XML_ERR_INTERNAL_ERROR,
		    "Internal error: xmlSchemaAddType, on type "
		    "'%s'.\n",
		    name, NULL);
		xmlFree(ret);
		return (NULL);
	    }
	    ret->redef = prev->redef;
	    prev->redef = ret;
	}
    }
    ret->node = node;
    ret->minOccurs = 1;
    ret->maxOccurs = 1;
    ret->attributeUses = NULL;
    ret->attributeWildcard = NULL;
    if (ctxt->assemble != NULL)
	xmlSchemaAddAssembledItem(ctxt,ret);
    return (ret);
}

static xmlSchemaQNameRefPtr
xmlSchemaNewQNameRef(xmlSchemaPtr schema,
		     xmlSchemaTypeType refType,
		     const xmlChar *refName,
		     const xmlChar *refNs)
{
    xmlSchemaQNameRefPtr ret;

    ret = (xmlSchemaQNameRefPtr)
	xmlMalloc(sizeof(xmlSchemaQNameRef));
    if (ret == NULL) {
	xmlSchemaPErrMemory(NULL, "allocating QName reference item",
	    NULL);
	return (NULL);
    }
    ret->type = XML_SCHEMA_EXTRA_QNAMEREF;
    ret->name = refName;
    ret->targetNamespace = refNs;
    ret->item = NULL;
    ret->itemType = refType;
    /*
    * Store the reference item in the schema.
    */
    xmlSchemaAddVolatile(schema, (xmlSchemaBasicItemPtr) ret);
    return (ret);
}

/**
 * xmlSchemaAddModelGroup:
 * @ctxt:  a schema parser context
 * @schema:  the schema being built
 * @type: the "compositor" type of the model group
 * @container:  the internal component name
 * @node: the node in the schema doc
 *
 * Adds a schema model group
 * *WARNING* this interface is highly subject to change
 *
 * Returns the new struture or NULL in case of error
 */
static xmlSchemaModelGroupPtr
xmlSchemaAddModelGroup(xmlSchemaParserCtxtPtr ctxt, xmlSchemaPtr schema,
		 xmlSchemaTypeType type, const xmlChar **container,
		 xmlNodePtr node)
{
    xmlSchemaModelGroupPtr ret = NULL;
    xmlChar buf[30];

    if ((ctxt == NULL) || (schema == NULL))
        return (NULL);

#ifdef DEBUG
    fprintf(stderr, "Adding model group component\n");
#endif
    ret = (xmlSchemaModelGroupPtr)
	xmlMalloc(sizeof(xmlSchemaModelGroup));
    if (ret == NULL) {
	xmlSchemaPErrMemory(ctxt, "allocating model group component",
	    NULL);
	return (NULL);
    }
    ret->type = type;
    ret->annot = NULL;
    ret->node = node;
    ret->children = NULL;
    ret->next = NULL;
    if (type == XML_SCHEMA_TYPE_SEQUENCE) {
	if (container != NULL)
	    snprintf((char *) buf, 29, "#seq%d", ctxt->counter++ + 1);
    } else if (type == XML_SCHEMA_TYPE_CHOICE) {
	if (container != NULL)
	    snprintf((char *) buf, 29, "#cho%d", ctxt->counter++ + 1);
    } else {
	if (container != NULL)
	    snprintf((char *) buf, 29, "#all%d", ctxt->counter++ + 1);
    }
    if (container != NULL)
	*container = xmlDictLookup(ctxt->dict, BAD_CAST buf, -1);
    /*
    * Add to volatile items.
    * TODO: this should be changed someday.
    */
    if (xmlSchemaAddVolatile(schema, (xmlSchemaBasicItemPtr) ret) != 0) {
	xmlFree(ret);
	return (NULL);
    }
    return (ret);
}


/**
 * xmlSchemaAddParticle:
 * @ctxt:  a schema parser context
 * @schema:  the schema being built
 * @node: the corresponding node in the schema doc
 * @min: the minOccurs
 * @max: the maxOccurs
 *
 * Adds an XML schema particle component.
 * *WARNING* this interface is highly subject to change
 *
 * Returns the new struture or NULL in case of error
 */
static xmlSchemaParticlePtr
xmlSchemaAddParticle(xmlSchemaParserCtxtPtr ctxt, xmlSchemaPtr schema,
		     xmlNodePtr node, int min, int max)
{
    xmlSchemaParticlePtr ret = NULL;
    if ((ctxt == NULL) || (schema == NULL))
        return (NULL);

#ifdef DEBUG
    fprintf(stderr, "Adding particle component\n");
#endif
    ret = (xmlSchemaParticlePtr)
	xmlMalloc(sizeof(xmlSchemaParticle));
    if (ret == NULL) {
	xmlSchemaPErrMemory(ctxt, "allocating particle component",
	    NULL);
	return (NULL);
    }
    ret->type = XML_SCHEMA_TYPE_PARTICLE;
    ret->annot = NULL;
    ret->node = node;
    ret->minOccurs = min;
    ret->maxOccurs = max;
    ret->next = NULL;
    ret->children = NULL;

    if (xmlSchemaAddVolatile(schema, (xmlSchemaBasicItemPtr) ret) != 0) {
	xmlFree(ret);
	return (NULL);
    }
    return (ret);
}

/**
 * xmlSchemaAddGroup:
 * @ctxt:  a schema validation context
 * @schema:  the schema being built
 * @name:  the group name
 *
 * Add an XML schema Group definition
 *
 * Returns the new struture or NULL in case of error
 */
static xmlSchemaModelGroupDefPtr
xmlSchemaAddGroup(xmlSchemaParserCtxtPtr ctxt, xmlSchemaPtr schema,
                  const xmlChar *name, const xmlChar *namespaceName,
		  xmlNodePtr node)
{
    xmlSchemaModelGroupDefPtr ret = NULL;
    int val;

    if ((ctxt == NULL) || (schema == NULL) || (name == NULL))
        return (NULL);

    if (schema->groupDecl == NULL)
        schema->groupDecl = xmlHashCreateDict(10, ctxt->dict);
    if (schema->groupDecl == NULL)
        return (NULL);

    ret = (xmlSchemaModelGroupDefPtr) xmlMalloc(sizeof(xmlSchemaModelGroupDef));
    if (ret == NULL) {
        xmlSchemaPErrMemory(ctxt, "adding group", NULL);
        return (NULL);
    }
    memset(ret, 0, sizeof(xmlSchemaModelGroupDef));
    ret->name = xmlDictLookup(ctxt->dict, name, -1);
    ret->type = XML_SCHEMA_TYPE_GROUP;
    ret->node = node;
    ret->targetNamespace = namespaceName;
    val = xmlHashAddEntry2(schema->groupDecl, ret->name, namespaceName, ret);
    if (val != 0) {
	xmlSchemaPCustomErr(ctxt,
	    XML_SCHEMAP_REDEFINED_GROUP,
	    NULL, NULL, node,
	    "A global model group definition with the name '%s' does already "
	    "exist", name);
        xmlFree(ret);
        return (NULL);
    }
    if (ctxt->assemble != NULL)
	xmlSchemaAddAssembledItem(ctxt, (xmlSchemaTypePtr) ret);
    return (ret);
}

/**
 * xmlSchemaNewWildcardNs:
 * @ctxt:  a schema validation context
 *
 * Creates a new wildcard namespace constraint.
 *
 * Returns the new struture or NULL in case of error
 */
static xmlSchemaWildcardNsPtr
xmlSchemaNewWildcardNsConstraint(xmlSchemaParserCtxtPtr ctxt)
{
    xmlSchemaWildcardNsPtr ret;

    ret = (xmlSchemaWildcardNsPtr)
	xmlMalloc(sizeof(xmlSchemaWildcardNs));
    if (ret == NULL) {
	xmlSchemaPErrMemory(ctxt, "creating wildcard namespace constraint", NULL);
	return (NULL);
    }
    ret->value = NULL;
    ret->next = NULL;
    return (ret);
}

/**
 * xmlSchemaAddWildcard:
 * @ctxt:  a schema validation context
 * @schema: a schema
 *
 * Adds a wildcard.
 * It corresponds to a xsd:anyAttribute and xsd:any.
 *
 * Returns the new struture or NULL in case of error
 */
static xmlSchemaWildcardPtr
xmlSchemaAddWildcard(xmlSchemaParserCtxtPtr ctxt, xmlSchemaPtr schema,
		     xmlSchemaTypeType type, xmlNodePtr node)
{
    xmlSchemaWildcardPtr ret = NULL;

    if ((ctxt == NULL) || (schema == NULL))
        return (NULL);

#ifdef DEBUG
    fprintf(stderr, "Adding wildcard component\n");
#endif

    ret = (xmlSchemaWildcardPtr) xmlMalloc(sizeof(xmlSchemaWildcard));
    if (ret == NULL) {
        xmlSchemaPErrMemory(ctxt, "adding wildcard", NULL);
        return (NULL);
    }
    memset(ret, 0, sizeof(xmlSchemaWildcard));
    ret->type = type;
    ret->minOccurs = 1;
    ret->maxOccurs = 1;

    if (xmlSchemaAddVolatile(schema, (xmlSchemaBasicItemPtr) ret) != 0) {
	xmlSchemaPCustomErr(ctxt, XML_SCHEMAP_INTERNAL, NULL, NULL, node,
	    "Failed to add a wildcard component to the list", NULL);
	xmlFree(ret);
	return (NULL);
    }
    return (ret);
}

/************************************************************************
 * 									*
 *		Utilities for parsing					*
 * 									*
 ************************************************************************/

#if 0
/**
 * xmlGetQNameProp:
 * @ctxt:  a schema validation context
 * @node:  a subtree containing XML Schema informations
 * @name:  the attribute name
 * @namespace:  the result namespace if any
 *
 * Extract a QName Attribute value
 *
 * Returns the NCName or NULL if not found, and also update @namespace
 *    with the namespace URI
 */
static const xmlChar *
xmlGetQNameProp(xmlSchemaParserCtxtPtr ctxt, xmlNodePtr node,
                const char *name, const xmlChar ** namespace)
{
    const xmlChar *val;
    xmlNsPtr ns;
    const xmlChar *ret, *prefix;
    int len;
    xmlAttrPtr attr;

    *namespace = NULL;
    attr = xmlSchemaGetPropNode(node, name);
    if (attr == NULL)
	return (NULL);
    val = xmlSchemaGetNodeContent(ctxt, (xmlNodePtr) attr);

    if (val == NULL)
        return (NULL);

    if (!strchr((char *) val, ':')) {
	ns = xmlSearchNs(node->doc, node, 0);
	if (ns) {
	    *namespace = xmlDictLookup(ctxt->dict, ns->href, -1);
	    return (val);
	}
    }
    ret = xmlSplitQName3(val, &len);
    if (ret == NULL) {
        return (val);
    }
    ret = xmlDictLookup(ctxt->dict, ret, -1);
    prefix = xmlDictLookup(ctxt->dict, val, len);

    ns = xmlSearchNs(node->doc, node, prefix);
    if (ns == NULL) {
        xmlSchemaPSimpleTypeErr(ctxt, XML_SCHEMAP_PREFIX_UNDEFINED,
	    NULL, NULL, (xmlNodePtr) attr,
	    xmlSchemaGetBuiltInType(XML_SCHEMAS_QNAME), NULL, val,
	    "The QName value '%s' has no corresponding namespace "
	    "declaration in scope", val, NULL);
    } else {
        *namespace = xmlDictLookup(ctxt->dict, ns->href, -1);
    }
    return (ret);
}
#endif

/**
 * xmlSchemaPValAttrNodeQNameValue:
 * @ctxt:  a schema parser context
 * @schema: the schema context
 * @ownerDes: the designation of the parent element
 * @ownerItem: the parent as a schema object
 * @value:  the QName value
 * @local: the resulting local part if found, the attribute value otherwise
 * @uri:  the resulting namespace URI if found
 *
 * Extracts the local name and the URI of a QName value and validates it.
 * This one is intended to be used on attribute values that
 * should resolve to schema components.
 *
 * Returns 0, in case the QName is valid, a positive error code
 * if not valid and -1 if an internal error occurs.
 */
static int
xmlSchemaPValAttrNodeQNameValue(xmlSchemaParserCtxtPtr ctxt,
				       xmlSchemaPtr schema,
				       xmlChar **ownerDes ATTRIBUTE_UNUSED,
				       xmlSchemaTypePtr ownerItem,
				       xmlAttrPtr attr,
				       const xmlChar *value,
				       const xmlChar **uri,
				       const xmlChar **local)
{
    const xmlChar *pref;
    xmlNsPtr ns;
    int len, ret;

    *uri = NULL;
    *local = NULL;
    ret = xmlValidateQName(value, 1);
    if (ret > 0) {
	xmlSchemaPSimpleTypeErr(ctxt,
	    XML_SCHEMAP_S4S_ATTR_INVALID_VALUE,
	    ownerItem, (xmlNodePtr) attr,
	    xmlSchemaGetBuiltInType(XML_SCHEMAS_QNAME),
	    NULL, value, NULL, NULL, NULL);
	*local = value;
	return (ctxt->err);
    } else if (ret < 0)
	return (-1);

    if (!strchr((char *) value, ':')) {
	ns = xmlSearchNs(attr->doc, attr->parent, 0);
	if (ns)
	    *uri = xmlDictLookup(ctxt->dict, ns->href, -1);
	else if (schema->flags & XML_SCHEMAS_INCLUDING_CONVERT_NS) {
	    /*
	    * This one takes care of included schemas with no
	    * target namespace.
	    */
	    *uri = schema->targetNamespace;
	}
	*local = xmlDictLookup(ctxt->dict, value, -1);
	return (0);
    }
    /*
    * At this point xmlSplitQName3 has to return a local name.
    */
    *local = xmlSplitQName3(value, &len);
    *local = xmlDictLookup(ctxt->dict, *local, -1);
    pref = xmlDictLookup(ctxt->dict, value, len);
    ns = xmlSearchNs(attr->doc, attr->parent, pref);
    if (ns == NULL) {
	xmlSchemaPSimpleTypeErr(ctxt,
	    XML_SCHEMAP_S4S_ATTR_INVALID_VALUE,
	    ownerItem, (xmlNodePtr) attr,
	    xmlSchemaGetBuiltInType(XML_SCHEMAS_QNAME), NULL, value,
	    "The value '%s' of simple type 'xs:QName' has no "
	    "corresponding namespace declaration in scope", value, NULL);
	return (ctxt->err);
    } else {
        *uri = xmlDictLookup(ctxt->dict, ns->href, -1);
    }
    return (0);
}

/**
 * xmlSchemaPValAttrNodeQName:
 * @ctxt:  a schema parser context
 * @schema: the schema context
 * @ownerDes: the designation of the owner element
 * @ownerItem: the owner as a schema object
 * @attr:  the attribute node
 * @local: the resulting local part if found, the attribute value otherwise
 * @uri:  the resulting namespace URI if found
 *
 * Extracts and validates the QName of an attribute value.
 * This one is intended to be used on attribute values that
 * should resolve to schema components.
 *
 * Returns 0, in case the QName is valid, a positive error code
 * if not valid and -1 if an internal error occurs.
 */
static int
xmlSchemaPValAttrNodeQName(xmlSchemaParserCtxtPtr ctxt,
				       xmlSchemaPtr schema,
				       xmlChar **ownerDes,
				       xmlSchemaTypePtr ownerItem,
				       xmlAttrPtr attr,
				       const xmlChar **uri,
				       const xmlChar **local)
{
    const xmlChar *value;

    value = xmlSchemaGetNodeContent(ctxt, (xmlNodePtr) attr);
    return (xmlSchemaPValAttrNodeQNameValue(ctxt, schema,
	ownerDes, ownerItem, attr, value, uri, local));
}

/**
 * xmlSchemaPValAttrQName:
 * @ctxt:  a schema parser context
 * @schema: the schema context
 * @ownerDes: the designation of the parent element
 * @ownerItem: the owner as a schema object
 * @ownerElem:  the parent node of the attribute
 * @name:  the name of the attribute
 * @local: the resulting local part if found, the attribute value otherwise
 * @uri:  the resulting namespace URI if found
 *
 * Extracts and validates the QName of an attribute value.
 *
 * Returns 0, in case the QName is valid, a positive error code
 * if not valid and -1 if an internal error occurs.
 */
static int
xmlSchemaPValAttrQName(xmlSchemaParserCtxtPtr ctxt,
				   xmlSchemaPtr schema,
				   xmlChar **ownerDes,
				   xmlSchemaTypePtr ownerItem,
				   xmlNodePtr ownerElem,
				   const char *name,
				   const xmlChar **uri,
				   const xmlChar **local)
{
    xmlAttrPtr attr;

    attr = xmlSchemaGetPropNode(ownerElem, name);
    if (attr == NULL) {
	*local = NULL;
	*uri = NULL;
	return (0);
    }
    return (xmlSchemaPValAttrNodeQName(ctxt, schema,
	ownerDes, ownerItem, attr, uri, local));
}

/**
 * xmlSchemaPValAttrID:
 * @ctxt:  a schema parser context
 * @schema: the schema context
 * @ownerDes: the designation of the parent element
 * @ownerItem: the owner as a schema object
 * @ownerElem:  the parent node of the attribute
 * @name:  the name of the attribute
 *
 * Extracts and validates the ID of an attribute value.
 *
 * Returns 0, in case the ID is valid, a positive error code
 * if not valid and -1 if an internal error occurs.
 */
static int
xmlSchemaPValAttrID(xmlSchemaParserCtxtPtr ctxt,
		    xmlChar **ownerDes ATTRIBUTE_UNUSED,
		    xmlSchemaTypePtr ownerItem,
		    xmlNodePtr ownerElem,
		    const xmlChar *name)
{
    int ret;
    xmlChar *value;
    xmlAttrPtr attr;

    value = xmlGetNoNsProp(ownerElem, name);
    if (value == NULL)
	return (0);

    attr = xmlSchemaGetPropNode(ownerElem, (const char *) name);
    if (attr == NULL)
	return (-1);

    ret = xmlValidateNCName(BAD_CAST value, 1);
    if (ret == 0) {
	/*
	* NOTE: the IDness might have already be declared in the DTD
	*/
	if (attr->atype != XML_ATTRIBUTE_ID) {
	    xmlIDPtr res;
	    xmlChar *strip;

	    /*
	    * TODO: Use xmlSchemaStrip here; it's not exported at this
	    * moment.
	    */
	    strip = xmlSchemaCollapseString(BAD_CAST value);
	    if (strip != NULL)
		value = strip;
    	    res = xmlAddID(NULL, ownerElem->doc, BAD_CAST value, attr);
	    if (res == NULL) {
		ret = XML_SCHEMAP_S4S_ATTR_INVALID_VALUE;
		xmlSchemaPSimpleTypeErr(ctxt,
		    XML_SCHEMAP_S4S_ATTR_INVALID_VALUE,
		    ownerItem, (xmlNodePtr) attr,
		    xmlSchemaGetBuiltInType(XML_SCHEMAS_ID),
		    NULL, NULL, "Duplicate value '%s' of simple "
		    "type 'xs:ID'", BAD_CAST value, NULL);
	    } else
		attr->atype = XML_ATTRIBUTE_ID;
	    if (strip != NULL)
		xmlFree(strip);
	}
    } else if (ret > 0) {
	ret = XML_SCHEMAP_S4S_ATTR_INVALID_VALUE;
	xmlSchemaPSimpleTypeErr(ctxt,
	    XML_SCHEMAP_S4S_ATTR_INVALID_VALUE,
	    ownerItem, (xmlNodePtr) attr,
	    xmlSchemaGetBuiltInType(XML_SCHEMAS_ID),
	    NULL, NULL, "The value '%s' of simple type 'xs:ID' is "
	    "not a valid 'xs:NCName'",
	    BAD_CAST value, NULL);
    }
    xmlFree(value);

    return (ret);
}

/**
 * xmlGetMaxOccurs:
 * @ctxt:  a schema validation context
 * @node:  a subtree containing XML Schema informations
 *
 * Get the maxOccurs property
 *
 * Returns the default if not found, or the value
 */
static int
xmlGetMaxOccurs(xmlSchemaParserCtxtPtr ctxt, xmlNodePtr node,
		int min, int max, int def, const char *expected)
{
    const xmlChar *val, *cur;
    int ret = 0;
    xmlAttrPtr attr;

    attr = xmlSchemaGetPropNode(node, "maxOccurs");
    if (attr == NULL)
	return (def);
    val = xmlSchemaGetNodeContent(ctxt, (xmlNodePtr) attr);

    if (xmlStrEqual(val, (const xmlChar *) "unbounded")) {
	if (max != UNBOUNDED) {
	    xmlSchemaPSimpleTypeErr(ctxt,
		XML_SCHEMAP_S4S_ATTR_INVALID_VALUE,
		/* XML_SCHEMAP_INVALID_MINOCCURS, */
		NULL, (xmlNodePtr) attr, NULL, expected,
		val, NULL, NULL, NULL);
	    return (def);
	} else
	    return (UNBOUNDED);  /* encoding it with -1 might be another option */
    }

    cur = val;
    while (IS_BLANK_CH(*cur))
        cur++;
    if (*cur == 0) {
        xmlSchemaPSimpleTypeErr(ctxt,
	    XML_SCHEMAP_S4S_ATTR_INVALID_VALUE,
	    /* XML_SCHEMAP_INVALID_MINOCCURS, */
	    NULL, (xmlNodePtr) attr, NULL, expected,
	    val, NULL, NULL, NULL);
	return (def);
    }
    while ((*cur >= '0') && (*cur <= '9')) {
        ret = ret * 10 + (*cur - '0');
        cur++;
    }
    while (IS_BLANK_CH(*cur))
        cur++;
    /*
    * TODO: Restrict the maximal value to Integer.
    */
    if ((*cur != 0) || (ret < min) || ((max != -1) && (ret > max))) {
	xmlSchemaPSimpleTypeErr(ctxt,
	    XML_SCHEMAP_S4S_ATTR_INVALID_VALUE,
	    /* XML_SCHEMAP_INVALID_MINOCCURS, */
	    NULL, (xmlNodePtr) attr, NULL, expected,
	    val, NULL, NULL, NULL);
        return (def);
    }
    return (ret);
}

/**
 * xmlGetMinOccurs:
 * @ctxt:  a schema validation context
 * @node:  a subtree containing XML Schema informations
 *
 * Get the minOccurs property
 *
 * Returns the default if not found, or the value
 */
static int
xmlGetMinOccurs(xmlSchemaParserCtxtPtr ctxt, xmlNodePtr node,
		int min, int max, int def, const char *expected)
{
    const xmlChar *val, *cur;
    int ret = 0;
    xmlAttrPtr attr;

    attr = xmlSchemaGetPropNode(node, "minOccurs");
    if (attr == NULL)
	return (def);
    val = xmlSchemaGetNodeContent(ctxt, (xmlNodePtr) attr);
    cur = val;
    while (IS_BLANK_CH(*cur))
        cur++;
    if (*cur == 0) {
        xmlSchemaPSimpleTypeErr(ctxt,
	    XML_SCHEMAP_S4S_ATTR_INVALID_VALUE,
	    /* XML_SCHEMAP_INVALID_MINOCCURS, */
	    NULL, (xmlNodePtr) attr, NULL, expected,
	    val, NULL, NULL, NULL);
        return (def);
    }
    while ((*cur >= '0') && (*cur <= '9')) {
        ret = ret * 10 + (*cur - '0');
        cur++;
    }
    while (IS_BLANK_CH(*cur))
        cur++;
    /*
    * TODO: Restrict the maximal value to Integer.
    */
    if ((*cur != 0) || (ret < min) || ((max != -1) && (ret > max))) {
	xmlSchemaPSimpleTypeErr(ctxt,
	    XML_SCHEMAP_S4S_ATTR_INVALID_VALUE,
	    /* XML_SCHEMAP_INVALID_MINOCCURS, */
	    NULL, (xmlNodePtr) attr, NULL, expected,
	    val, NULL, NULL, NULL);
        return (def);
    }
    return (ret);
}

/**
 * xmlSchemaPGetBoolNodeValue:
 * @ctxt:  a schema validation context
 * @ownerDes:  owner designation
 * @ownerItem:  the owner as a schema item
 * @node: the node holding the value
 *
 * Converts a boolean string value into 1 or 0.
 *
 * Returns 0 or 1.
 */
static int
xmlSchemaPGetBoolNodeValue(xmlSchemaParserCtxtPtr ctxt,
			   xmlChar **ownerDes ATTRIBUTE_UNUSED,
			   xmlSchemaTypePtr ownerItem,
			   xmlNodePtr node)
{
    xmlChar *value = NULL;
    int res = 0;

    value = xmlNodeGetContent(node);
    /*
    * 3.2.2.1 Lexical representation
    * An instance of a datatype that is defined as ·boolean·
    * can have the following legal literals {true, false, 1, 0}.
    */
    if (xmlStrEqual(BAD_CAST value, BAD_CAST "true"))
        res = 1;
    else if (xmlStrEqual(BAD_CAST value, BAD_CAST "false"))
        res = 0;
    else if (xmlStrEqual(BAD_CAST value, BAD_CAST "1"))
	res = 1;
    else if (xmlStrEqual(BAD_CAST value, BAD_CAST "0"))
        res = 0;
    else {
        xmlSchemaPSimpleTypeErr(ctxt,
	    XML_SCHEMAP_INVALID_BOOLEAN,
	    ownerItem, node,
	    xmlSchemaGetBuiltInType(XML_SCHEMAS_BOOLEAN),
	    NULL, BAD_CAST value,
	    NULL, NULL, NULL);
    }
    if (value != NULL)
	xmlFree(value);
    return (res);
}

/**
 * xmlGetBooleanProp:
 * @ctxt:  a schema validation context
 * @node:  a subtree containing XML Schema informations
 * @name:  the attribute name
 * @def:  the default value
 *
 * Evaluate if a boolean property is set
 *
 * Returns the default if not found, 0 if found to be false,
 * 1 if found to be true
 */
static int
xmlGetBooleanProp(xmlSchemaParserCtxtPtr ctxt,
		  xmlChar **ownerDes ATTRIBUTE_UNUSED,
		  xmlSchemaTypePtr ownerItem,
		  xmlNodePtr node,
                  const char *name, int def)
{
    const xmlChar *val;

    val = xmlSchemaGetProp(ctxt, node, name);
    if (val == NULL)
        return (def);
    /*
    * 3.2.2.1 Lexical representation
    * An instance of a datatype that is defined as ·boolean·
    * can have the following legal literals {true, false, 1, 0}.
    */
    if (xmlStrEqual(val, BAD_CAST "true"))
        def = 1;
    else if (xmlStrEqual(val, BAD_CAST "false"))
        def = 0;
    else if (xmlStrEqual(val, BAD_CAST "1"))
	def = 1;
    else if (xmlStrEqual(val, BAD_CAST "0"))
        def = 0;
    else {
        xmlSchemaPSimpleTypeErr(ctxt,
	    XML_SCHEMAP_INVALID_BOOLEAN,
	    ownerItem,
	    (xmlNodePtr) xmlSchemaGetPropNode(node, name),
	    xmlSchemaGetBuiltInType(XML_SCHEMAS_BOOLEAN),
	    NULL, val, NULL, NULL, NULL);
    }
    return (def);
}

/************************************************************************
 * 									*
 *		Shema extraction from an Infoset			*
 * 									*
 ************************************************************************/
static xmlSchemaTypePtr xmlSchemaParseSimpleType(xmlSchemaParserCtxtPtr
                                                 ctxt, xmlSchemaPtr schema,
                                                 xmlNodePtr node,
						 int topLevel);
static xmlSchemaTypePtr xmlSchemaParseComplexType(xmlSchemaParserCtxtPtr
                                                  ctxt,
                                                  xmlSchemaPtr schema,
                                                  xmlNodePtr node,
						  int topLevel);
static xmlSchemaTypePtr xmlSchemaParseRestriction(xmlSchemaParserCtxtPtr
                                                  ctxt,
                                                  xmlSchemaPtr schema,
                                                  xmlNodePtr node,
						  xmlSchemaTypeType parentType);
static xmlSchemaAttributePtr xmlSchemaParseAttribute(xmlSchemaParserCtxtPtr
                                                     ctxt,
                                                     xmlSchemaPtr schema,
                                                     xmlNodePtr node,
						     int topLevel);
static xmlSchemaAttributeGroupPtr
xmlSchemaParseAttributeGroup(xmlSchemaParserCtxtPtr ctxt,
                             xmlSchemaPtr schema, xmlNodePtr node,
			     int topLevel);
static xmlSchemaTypePtr xmlSchemaParseList(xmlSchemaParserCtxtPtr ctxt,
                                           xmlSchemaPtr schema,
                                           xmlNodePtr node);
static xmlSchemaWildcardPtr
xmlSchemaParseAnyAttribute(xmlSchemaParserCtxtPtr ctxt,
                           xmlSchemaPtr schema, xmlNodePtr node);

/**
 * xmlSchemaPValAttrNodeValue:
 *
 * @ctxt:  a schema parser context
 * @ownerDes: the designation of the parent element
 * @ownerItem: the schema object owner if existent
 * @attr:  the schema attribute node being validated
 * @value: the value
 * @type: the built-in type to be validated against
 *
 * Validates a value against the given built-in type.
 * This one is intended to be used internally for validation
 * of schema attribute values during parsing of the schema.
 *
 * Returns 0 if the value is valid, a positive error code
 * number otherwise and -1 in case of an internal or API error.
 */
static int
xmlSchemaPValAttrNodeValue(xmlSchemaParserCtxtPtr pctxt,
			   xmlChar **ownerDes ATTRIBUTE_UNUSED,
			   xmlSchemaTypePtr ownerItem,
			   xmlAttrPtr attr,
			   const xmlChar *value,
			   xmlSchemaTypePtr type)
{

    int ret = 0;

    /*
    * NOTE: Should we move this to xmlschematypes.c? Hmm, but this
    * one is really meant to be used internally, so better not.
    */
    if ((pctxt == NULL) || (type == NULL) || (attr == NULL))
	return (-1);
    if (type->type != XML_SCHEMA_TYPE_BASIC) {
	PERROR_INT("xmlSchemaPValAttrNodeValue",
	    "the given type is not a built-in type");
	return (-1);
    }
    switch (type->builtInType) {
	case XML_SCHEMAS_NCNAME:
	case XML_SCHEMAS_QNAME:
	case XML_SCHEMAS_ANYURI:
	case XML_SCHEMAS_TOKEN:
	case XML_SCHEMAS_LANGUAGE:
	    ret = xmlSchemaValPredefTypeNode(type, value, NULL,
		(xmlNodePtr) attr);
	    break;
	default: {
	    PERROR_INT("xmlSchemaPValAttrNodeValue",
		"validation using the given type is not supported");
	    return (-1);
	}
    }
    /*
    * TODO: Should we use the S4S error codes instead?
    */
    if (ret < 0) {
	PERROR_INT("xmlSchemaPValAttrNodeValue",
	    "failed to validate a schema attribute value");
	return (-1);
    } else if (ret > 0) {
	if (VARIETY_LIST(type))
	    ret = XML_SCHEMAV_CVC_DATATYPE_VALID_1_2_2;
	else
	    ret = XML_SCHEMAV_CVC_DATATYPE_VALID_1_2_1;
	xmlSchemaPSimpleTypeErr(pctxt, 
	    ret, ownerItem, (xmlNodePtr) attr,
	    type, NULL, value, NULL, NULL, NULL);
    }
    return (ret);
}

/**
 * xmlSchemaPValAttrNode:
 *
 * @ctxt:  a schema parser context
 * @ownerDes: the designation of the parent element
 * @ownerItem: the schema object owner if existent
 * @attr:  the schema attribute node being validated
 * @type: the built-in type to be validated against
 * @value: the resulting value if any
 *
 * Extracts and validates a value against the given built-in type.
 * This one is intended to be used internally for validation
 * of schema attribute values during parsing of the schema.
 *
 * Returns 0 if the value is valid, a positive error code
 * number otherwise and -1 in case of an internal or API error.
 */
static int
xmlSchemaPValAttrNode(xmlSchemaParserCtxtPtr ctxt,
			   xmlChar **ownerDes,
			   xmlSchemaTypePtr ownerItem,
			   xmlAttrPtr attr,
			   xmlSchemaTypePtr type,
			   const xmlChar **value)
{
    const xmlChar *val;

    if ((ctxt == NULL) || (type == NULL) || (attr == NULL))
	return (-1);

    val = xmlSchemaGetNodeContent(ctxt, (xmlNodePtr) attr);
    if (value != NULL)
	*value = val;

    return (xmlSchemaPValAttrNodeValue(ctxt, ownerDes, ownerItem, attr,
	val, type));
}

/**
 * xmlSchemaPValAttr:
 *
 * @ctxt:  a schema parser context
 * @node: the element node of the attribute
 * @ownerDes: the designation of the parent element
 * @ownerItem: the schema object owner if existent
 * @ownerElem: the owner element node
 * @name:  the name of the schema attribute node
 * @type: the built-in type to be validated against
 * @value: the resulting value if any
 *
 * Extracts and validates a value against the given built-in type.
 * This one is intended to be used internally for validation
 * of schema attribute values during parsing of the schema.
 *
 * Returns 0 if the value is valid, a positive error code
 * number otherwise and -1 in case of an internal or API error.
 */
static int
xmlSchemaPValAttr(xmlSchemaParserCtxtPtr ctxt,
		       xmlChar **ownerDes,
		       xmlSchemaTypePtr ownerItem,
		       xmlNodePtr ownerElem,
		       const char *name,
		       xmlSchemaTypePtr type,
		       const xmlChar **value)
{
    xmlAttrPtr attr;

    if ((ctxt == NULL) || (type == NULL)) {
	if (value != NULL)
	    *value = NULL;
	return (-1);
    }
    if (type->type != XML_SCHEMA_TYPE_BASIC) {
	if (value != NULL)
	    *value = NULL;
	xmlSchemaPErr(ctxt, ownerElem,
	    XML_SCHEMAP_INTERNAL,
	    "Internal error: xmlSchemaPValAttr, the given "
	    "type '%s' is not a built-in type.\n",
	    type->name, NULL);
	return (-1);
    }
    attr = xmlSchemaGetPropNode(ownerElem, name);
    if (attr == NULL) {
	if (value != NULL)
	    *value = NULL;
	return (0);
    }
    return (xmlSchemaPValAttrNode(ctxt, ownerDes, ownerItem, attr,
	type, value));
}

static int
xmlSchemaCheckReference(xmlSchemaParserCtxtPtr pctxt,
		  xmlSchemaPtr schema,
		  xmlNodePtr node,
		  xmlSchemaBasicItemPtr item,
		  const xmlChar *namespaceName)
{
    if (xmlStrEqual(schema->targetNamespace, namespaceName))
	return (1);
    if (xmlStrEqual(xmlSchemaNs, namespaceName))
	return (1);
    if (pctxt->localImports != NULL) {
	int i;
	for (i = 0; i < pctxt->nbLocalImports; i++)
	    if (xmlStrEqual(namespaceName, pctxt->localImports[i]))
		return (1);
    }
    if (namespaceName == NULL)
	xmlSchemaPCustomErr(pctxt, XML_SCHEMAP_SRC_RESOLVE,
	    NULL, (xmlSchemaTypePtr) item, node,
	    "References from this schema to components in no "
	    "namespace are not valid, since not indicated by an import "
	    "statement", NULL);
    else
	xmlSchemaPCustomErr(pctxt, XML_SCHEMAP_SRC_RESOLVE,
	    NULL, (xmlSchemaTypePtr) item, node,
	    "References from this schema to components in the "
	    "namespace '%s' are not valid, since not indicated by an import "
	    "statement", namespaceName);
    return (0);
}

/**
 * xmlSchemaParseAttrDecls:
 * @ctxt:  a schema validation context
 * @schema:  the schema being built
 * @node:  a subtree containing XML Schema informations
 * @type:  the hosting type where the attributes will be anchored
 *
 * parse a XML schema attrDecls declaration corresponding to
 * <!ENTITY % attrDecls
 *       '((%attribute;| %attributeGroup;)*,(%anyAttribute;)?)'>
 */
static xmlNodePtr
xmlSchemaParseAttrDecls(xmlSchemaParserCtxtPtr ctxt, xmlSchemaPtr schema,
                        xmlNodePtr child, xmlSchemaTypePtr type)
{
    xmlSchemaAttributePtr lastattr = NULL, attr;

    while ((IS_SCHEMA(child, "attribute")) ||
           (IS_SCHEMA(child, "attributeGroup"))) {
        attr = NULL;
        if (IS_SCHEMA(child, "attribute")) {
            attr = xmlSchemaParseAttribute(ctxt, schema, child, 0);
        } else if (IS_SCHEMA(child, "attributeGroup")) {
            attr = (xmlSchemaAttributePtr)
                xmlSchemaParseAttributeGroup(ctxt, schema, child, 0);
        }
        if (attr != NULL) {
            if (lastattr == NULL) {
		if (type->type == XML_SCHEMA_TYPE_ATTRIBUTEGROUP)
		    ((xmlSchemaAttributeGroupPtr) type)->attributes = attr;
		else
		    type->attributes = attr;
                lastattr = attr;
            } else {
                lastattr->next = attr;
                lastattr = attr;
            }
        }
        child = child->next;
    }
    return (child);
}

/**
 * xmlSchemaParseAnnotation:
 * @ctxt:  a schema validation context
 * @schema:  the schema being built
 * @node:  a subtree containing XML Schema informations
 *
 * parse a XML schema Attrribute declaration
 * *WARNING* this interface is highly subject to change
 *
 * Returns -1 in case of error, 0 if the declaration is improper and
 *         1 in case of success.
 */
static xmlSchemaAnnotPtr
xmlSchemaParseAnnotation(xmlSchemaParserCtxtPtr ctxt, xmlSchemaPtr schema,
                         xmlNodePtr node)
{
    xmlSchemaAnnotPtr ret;
    xmlNodePtr child = NULL;
    xmlAttrPtr attr;
    int barked = 0;

    /*
    * INFO: S4S completed.
    */
    /*
    * id = ID
    * {any attributes with non-schema namespace . . .}>
    * Content: (appinfo | documentation)*
    */
    if ((ctxt == NULL) || (schema == NULL) || (node == NULL))
        return (NULL);
    ret = xmlSchemaNewAnnot(ctxt, node);
    attr = node->properties;
    while (attr != NULL) {
	if (((attr->ns == NULL) &&
	    (!xmlStrEqual(attr->name, BAD_CAST "id"))) ||
	    ((attr->ns != NULL) &&
	    xmlStrEqual(attr->ns->href, xmlSchemaNs))) {

	    xmlSchemaPIllegalAttrErr(ctxt,
		XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		NULL, NULL, attr);
	}
	attr = attr->next;
    }
    xmlSchemaPValAttrID(ctxt, NULL, NULL, node, BAD_CAST "id");
    /*
    * And now for the children...
    */
    child = node->children;
    while (child != NULL) {
	if (IS_SCHEMA(child, "appinfo")) {
	    /* TODO: make available the content of "appinfo". */
	    /*
	    * source = anyURI
	    * {any attributes with non-schema namespace . . .}>
	    * Content: ({any})*
	    */
	    attr = child->properties;
	    while (attr != NULL) {
		if (((attr->ns == NULL) &&
		     (!xmlStrEqual(attr->name, BAD_CAST "source"))) ||
		     ((attr->ns != NULL) &&
		      xmlStrEqual(attr->ns->href, xmlSchemaNs))) {

		    xmlSchemaPIllegalAttrErr(ctxt,
			XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
			NULL, NULL, attr);
		}
		attr = attr->next;
	    }
	    xmlSchemaPValAttr(ctxt, NULL, NULL, child, "source",
		xmlSchemaGetBuiltInType(XML_SCHEMAS_ANYURI), NULL);
	    child = child->next;
	} else if (IS_SCHEMA(child, "documentation")) {
	    /* TODO: make available the content of "documentation". */
	    /*
	    * source = anyURI
	    * {any attributes with non-schema namespace . . .}>
	    * Content: ({any})*
	    */
	    attr = child->properties;
	    while (attr != NULL) {
		if (attr->ns == NULL) {
		    if (!xmlStrEqual(attr->name, BAD_CAST "source")) {
			xmlSchemaPIllegalAttrErr(ctxt,
			    XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
			    NULL, NULL, attr);
		    }
		} else {
		    if (xmlStrEqual(attr->ns->href, xmlSchemaNs) ||
			(xmlStrEqual(attr->name, BAD_CAST "lang") &&
			(!xmlStrEqual(attr->ns->href, XML_XML_NAMESPACE)))) {

			xmlSchemaPIllegalAttrErr(ctxt,
			    XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
			    NULL, NULL, attr);
		    }
		}
		attr = attr->next;
	    }
	    /*
	    * Attribute "xml:lang".
	    */
	    attr = xmlSchemaGetPropNodeNs(child, (const char *) XML_XML_NAMESPACE, "lang");
	    if (attr != NULL)
		xmlSchemaPValAttrNode(ctxt, NULL, NULL, attr,
		xmlSchemaGetBuiltInType(XML_SCHEMAS_LANGUAGE), NULL);
	    child = child->next;
	} else {
	    if (!barked)
		xmlSchemaPContentErr(ctxt,
		    XML_SCHEMAP_S4S_ELEM_NOT_ALLOWED,
		    NULL, NULL, node, child, NULL, "(appinfo | documentation)*");
	    barked = 1;
	    child = child->next;
	}
    }

    return (ret);
}

/**
 * xmlSchemaParseFacet:
 * @ctxt:  a schema validation context
 * @schema:  the schema being built
 * @node:  a subtree containing XML Schema informations
 *
 * parse a XML schema Facet declaration
 * *WARNING* this interface is highly subject to change
 *
 * Returns the new type structure or NULL in case of error
 */
static xmlSchemaFacetPtr
xmlSchemaParseFacet(xmlSchemaParserCtxtPtr ctxt, xmlSchemaPtr schema,
                    xmlNodePtr node)
{
    xmlSchemaFacetPtr facet;
    xmlNodePtr child = NULL;
    const xmlChar *value;

    if ((ctxt == NULL) || (schema == NULL) || (node == NULL))
        return (NULL);

    facet = xmlSchemaNewFacet();
    if (facet == NULL) {
        xmlSchemaPErrMemory(ctxt, "allocating facet", node);
        return (NULL);
    }
    facet->node = node;
    value = xmlSchemaGetProp(ctxt, node, "value");
    if (value == NULL) {
        xmlSchemaPErr2(ctxt, node, child, XML_SCHEMAP_FACET_NO_VALUE,
                       "Facet %s has no value\n", node->name, NULL);
        xmlSchemaFreeFacet(facet);
        return (NULL);
    }
    if (IS_SCHEMA(node, "minInclusive")) {
        facet->type = XML_SCHEMA_FACET_MININCLUSIVE;
    } else if (IS_SCHEMA(node, "minExclusive")) {
        facet->type = XML_SCHEMA_FACET_MINEXCLUSIVE;
    } else if (IS_SCHEMA(node, "maxInclusive")) {
        facet->type = XML_SCHEMA_FACET_MAXINCLUSIVE;
    } else if (IS_SCHEMA(node, "maxExclusive")) {
        facet->type = XML_SCHEMA_FACET_MAXEXCLUSIVE;
    } else if (IS_SCHEMA(node, "totalDigits")) {
        facet->type = XML_SCHEMA_FACET_TOTALDIGITS;
    } else if (IS_SCHEMA(node, "fractionDigits")) {
        facet->type = XML_SCHEMA_FACET_FRACTIONDIGITS;
    } else if (IS_SCHEMA(node, "pattern")) {
        facet->type = XML_SCHEMA_FACET_PATTERN;
    } else if (IS_SCHEMA(node, "enumeration")) {
        facet->type = XML_SCHEMA_FACET_ENUMERATION;
    } else if (IS_SCHEMA(node, "whiteSpace")) {
        facet->type = XML_SCHEMA_FACET_WHITESPACE;
    } else if (IS_SCHEMA(node, "length")) {
        facet->type = XML_SCHEMA_FACET_LENGTH;
    } else if (IS_SCHEMA(node, "maxLength")) {
        facet->type = XML_SCHEMA_FACET_MAXLENGTH;
    } else if (IS_SCHEMA(node, "minLength")) {
        facet->type = XML_SCHEMA_FACET_MINLENGTH;
    } else {
        xmlSchemaPErr2(ctxt, node, child, XML_SCHEMAP_UNKNOWN_FACET_TYPE,
                       "Unknown facet type %s\n", node->name, NULL);
        xmlSchemaFreeFacet(facet);
        return (NULL);
    }
    xmlSchemaPValAttrID(ctxt, NULL,
	(xmlSchemaTypePtr) facet, node, BAD_CAST "id");
    facet->value = value;
    if ((facet->type != XML_SCHEMA_FACET_PATTERN) &&
	(facet->type != XML_SCHEMA_FACET_ENUMERATION)) {
	const xmlChar *fixed;

	fixed = xmlSchemaGetProp(ctxt, node, "fixed");
	if (fixed != NULL) {
	    if (xmlStrEqual(fixed, BAD_CAST "true"))
		facet->fixed = 1;
	}
    }
    child = node->children;

    if (IS_SCHEMA(child, "annotation")) {
        facet->annot = xmlSchemaParseAnnotation(ctxt, schema, child);
        child = child->next;
    }
    if (child != NULL) {
        xmlSchemaPErr2(ctxt, node, child, XML_SCHEMAP_UNKNOWN_FACET_CHILD,
                       "Facet %s has unexpected child content\n",
                       node->name, NULL);
    }
    return (facet);
}

/**
 * xmlSchemaParseWildcardNs:
 * @ctxt:  a schema parser context
 * @wildc:  the wildcard, already created
 * @node:  a subtree containing XML Schema informations
 *
 * Parses the attribute "processContents" and "namespace"
 * of a xsd:anyAttribute and xsd:any.
 * *WARNING* this interface is highly subject to change
 *
 * Returns 0 if everything goes fine, a positive error code
 * if something is not valid and -1 if an internal error occurs.
 */
static int
xmlSchemaParseWildcardNs(xmlSchemaParserCtxtPtr ctxt,
			 xmlSchemaPtr schema,
			 xmlSchemaWildcardPtr wildc,
			 xmlNodePtr node)
{
    const xmlChar *pc, *ns, *dictnsItem;
    int ret = 0;
    xmlChar *nsItem;
    xmlSchemaWildcardNsPtr tmp, lastNs = NULL;
    xmlAttrPtr attr;

    pc = xmlSchemaGetProp(ctxt, node, "processContents");
    if ((pc == NULL)
        || (xmlStrEqual(pc, (const xmlChar *) "strict"))) {
        wildc->processContents = XML_SCHEMAS_ANY_STRICT;
    } else if (xmlStrEqual(pc, (const xmlChar *) "skip")) {
        wildc->processContents = XML_SCHEMAS_ANY_SKIP;
    } else if (xmlStrEqual(pc, (const xmlChar *) "lax")) {
        wildc->processContents = XML_SCHEMAS_ANY_LAX;
    } else {
        xmlSchemaPSimpleTypeErr(ctxt,
	    XML_SCHEMAP_S4S_ATTR_INVALID_VALUE,
	    NULL, node,
	    NULL, "(strict | skip | lax)", pc,
	    NULL, NULL, NULL);
        wildc->processContents = XML_SCHEMAS_ANY_STRICT;
	ret = XML_SCHEMAP_S4S_ATTR_INVALID_VALUE;
    }
    /*
     * Build the namespace constraints.
     */
    attr = xmlSchemaGetPropNode(node, "namespace");
    ns = xmlSchemaGetNodeContent(ctxt, (xmlNodePtr) attr);
    if ((attr == NULL) || (xmlStrEqual(ns, BAD_CAST "##any")))
	wildc->any = 1;
    else if (xmlStrEqual(ns, BAD_CAST "##other")) {
	wildc->negNsSet = xmlSchemaNewWildcardNsConstraint(ctxt);
	if (wildc->negNsSet == NULL) {
	    return (-1);
	}
	wildc->negNsSet->value = schema->targetNamespace;
    } else {
	const xmlChar *end, *cur;

	cur = ns;
	do {
	    while (IS_BLANK_CH(*cur))
		cur++;
	    end = cur;
	    while ((*end != 0) && (!(IS_BLANK_CH(*end))))
		end++;
	    if (end == cur)
		break;
	    nsItem = xmlStrndup(cur, end - cur);
	    if ((xmlStrEqual(nsItem, BAD_CAST "##other")) ||
		    (xmlStrEqual(nsItem, BAD_CAST "##any"))) {
		xmlSchemaPSimpleTypeErr(ctxt,
		    XML_SCHEMAP_WILDCARD_INVALID_NS_MEMBER,
		    NULL, (xmlNodePtr) attr,
		    NULL,
		    "((##any | ##other) | List of (xs:anyURI | "
		    "(##targetNamespace | ##local)))",
		    nsItem, NULL, NULL, NULL);
		ret = XML_SCHEMAP_WILDCARD_INVALID_NS_MEMBER;
	    } else {
		if (xmlStrEqual(nsItem, BAD_CAST "##targetNamespace")) {
		    dictnsItem = schema->targetNamespace;
		} else if (xmlStrEqual(nsItem, BAD_CAST "##local")) {
		    dictnsItem = NULL;
		} else {
		    /*
		    * Validate the item (anyURI).
		    */
		    xmlSchemaPValAttrNodeValue(ctxt, NULL, NULL, attr,
			nsItem, xmlSchemaGetBuiltInType(XML_SCHEMAS_ANYURI));
		    dictnsItem = xmlDictLookup(ctxt->dict, nsItem, -1);
		}
		/*
		* Avoid dublicate namespaces.
		*/
		tmp = wildc->nsSet;
		while (tmp != NULL) {
		    if (dictnsItem == tmp->value)
			break;
		    tmp = tmp->next;
		}
		if (tmp == NULL) {
		    tmp = xmlSchemaNewWildcardNsConstraint(ctxt);
		    if (tmp == NULL) {
			xmlFree(nsItem);
			return (-1);
		    }
		    tmp->value = dictnsItem;
		    tmp->next = NULL;
		    if (wildc->nsSet == NULL)
			wildc->nsSet = tmp;
		    else
			lastNs->next = tmp;
		    lastNs = tmp;
		}

	    }
	    xmlFree(nsItem);
	    cur = end;
	} while (*cur != 0);
    }
    return (ret);
}

static int
xmlSchemaPCheckParticleCorrect_2(xmlSchemaParserCtxtPtr ctxt,
				 xmlSchemaParticlePtr item ATTRIBUTE_UNUSED,
				 xmlNodePtr node,
				 int minOccurs,
				 int maxOccurs) {

    if ((maxOccurs == 0) && ( minOccurs == 0))
	return (0);
    if (maxOccurs != UNBOUNDED) {
	/*
	* TODO: Maybe we should better not create the particle,
	* if min/max is invalid, since it could confuse the build of the
	* content model.
	*/
	/*
	* 3.9.6 Schema Component Constraint: Particle Correct
	*
	*/
	if (maxOccurs < 1) {
	    /*
	    * 2.2 {max occurs} must be greater than or equal to 1.
	    */
	    xmlSchemaPCustomAttrErr(ctxt,
		XML_SCHEMAP_P_PROPS_CORRECT_2_2,
		NULL, NULL,
		xmlSchemaGetPropNode(node, "maxOccurs"),
		"The value must be greater than or equal to 1");
	    return (XML_SCHEMAP_P_PROPS_CORRECT_2_2);
	} else if (minOccurs > maxOccurs) {
	    /*
	    * 2.1 {min occurs} must not be greater than {max occurs}.
	    */
	    xmlSchemaPCustomAttrErr(ctxt,
		XML_SCHEMAP_P_PROPS_CORRECT_2_1,
		NULL, NULL,
		xmlSchemaGetPropNode(node, "minOccurs"),
		"The value must not be greater than the value of 'maxOccurs'");
	    return (XML_SCHEMAP_P_PROPS_CORRECT_2_1);
	}
    }
    return (0);
}

/**
 * xmlSchemaParseAny:
 * @ctxt:  a schema validation context
 * @schema:  the schema being built
 * @node:  a subtree containing XML Schema informations
 *
 * Parsea a XML schema <any> element. A particle and wildcard
 * will be created (except if minOccurs==maxOccurs==0, in this case
 * nothing will be created).
 * *WARNING* this interface is highly subject to change
 *
 * Returns the particle or NULL in case of error or if minOccurs==maxOccurs==0
 */
static xmlSchemaParticlePtr
xmlSchemaParseAny(xmlSchemaParserCtxtPtr ctxt, xmlSchemaPtr schema,
                  xmlNodePtr node)
{
    xmlSchemaParticlePtr particle;
    xmlNodePtr child = NULL;
    xmlSchemaWildcardPtr wild;
    int min, max;
    xmlAttrPtr attr;
    xmlSchemaAnnotPtr annot = NULL;

    if ((ctxt == NULL) || (schema == NULL) || (node == NULL))
        return (NULL);
    /*
    * Check for illegal attributes.
    */
    attr = node->properties;
    while (attr != NULL) {
	if (attr->ns == NULL) {
	    if ((!xmlStrEqual(attr->name, BAD_CAST "id")) &&
		(!xmlStrEqual(attr->name, BAD_CAST "minOccurs")) &&
		(!xmlStrEqual(attr->name, BAD_CAST "maxOccurs")) &&
	        (!xmlStrEqual(attr->name, BAD_CAST "namespace")) &&
		(!xmlStrEqual(attr->name, BAD_CAST "processContents"))) {
		xmlSchemaPIllegalAttrErr(ctxt,
		    XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		    NULL, NULL, attr);
	    }
	} else if (xmlStrEqual(attr->ns->href, xmlSchemaNs)) {
	    xmlSchemaPIllegalAttrErr(ctxt,
		XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		NULL, NULL, attr);
	}
	attr = attr->next;
    }
    xmlSchemaPValAttrID(ctxt, NULL, NULL, node, BAD_CAST "id");
    /*
    * minOccurs/maxOccurs.
    */
    max = xmlGetMaxOccurs(ctxt, node, 0, UNBOUNDED, 1,
	"(xs:nonNegativeInteger | unbounded)");
    min = xmlGetMinOccurs(ctxt, node, 0, -1, 1,
	"xs:nonNegativeInteger");
    xmlSchemaPCheckParticleCorrect_2(ctxt, NULL, node, min, max);
    /*
    * Create & parse the wildcard.
    */
    wild = xmlSchemaAddWildcard(ctxt, schema, XML_SCHEMA_TYPE_ANY, node);
    if (wild == NULL)
	return (NULL);
    xmlSchemaParseWildcardNs(ctxt, schema, wild, node);
    /*
    * And now for the children...
    */
    child = node->children;
    if (IS_SCHEMA(child, "annotation")) {
        annot = xmlSchemaParseAnnotation(ctxt, schema, child);
        child = child->next;
    }
    if (child != NULL) {
	xmlSchemaPContentErr(ctxt,
	    XML_SCHEMAP_S4S_ELEM_NOT_ALLOWED,
	    NULL, NULL, node, child,
	    NULL, "(annotation?)");
    }
    /*
    * No component if minOccurs==maxOccurs==0.
    */
    if ((min == 0) && (max == 0)) {
	/* Don't free the wildcard, since it's already on the list. */
	return (NULL);
    }
    /*
    * Create the particle.
    */
    particle = xmlSchemaAddParticle(ctxt, schema, node, min, max);
    if (particle == NULL)
        return (NULL);
    particle->annot = annot;
    wild->minOccurs = min;
    wild->maxOccurs = max;
    particle->children = (xmlSchemaTreeItemPtr) wild;

    return (particle);
}

/**
 * xmlSchemaParseNotation:
 * @ctxt:  a schema validation context
 * @schema:  the schema being built
 * @node:  a subtree containing XML Schema informations
 *
 * parse a XML schema Notation declaration
 *
 * Returns the new structure or NULL in case of error
 */
static xmlSchemaNotationPtr
xmlSchemaParseNotation(xmlSchemaParserCtxtPtr ctxt, xmlSchemaPtr schema,
                       xmlNodePtr node)
{
    const xmlChar *name;
    xmlSchemaNotationPtr ret;
    xmlNodePtr child = NULL;

    if ((ctxt == NULL) || (schema == NULL) || (node == NULL))
        return (NULL);
    name = xmlSchemaGetProp(ctxt, node, "name");
    if (name == NULL) {
        xmlSchemaPErr2(ctxt, node, child, XML_SCHEMAP_NOTATION_NO_NAME,
                       "Notation has no name\n", NULL, NULL);
        return (NULL);
    }
    ret = xmlSchemaAddNotation(ctxt, schema, name);
    if (ret == NULL) {
        return (NULL);
    }
    ret->targetNamespace = schema->targetNamespace;

    xmlSchemaPValAttrID(ctxt, NULL, (xmlSchemaTypePtr) ret,
	node, BAD_CAST "id");

     if (IS_SCHEMA(child, "annotation")) {
        ret->annot = xmlSchemaParseAnnotation(ctxt, schema, child);
        child = child->next;
    }

    child = node->children;
    if (IS_SCHEMA(child, "annotation")) {
        ret->annot = xmlSchemaParseAnnotation(ctxt, schema, child);
        child = child->next;
    }
    if (child != NULL) {
	xmlSchemaPContentErr(ctxt,
	    XML_SCHEMAP_S4S_ELEM_NOT_ALLOWED,
	    NULL, NULL, node, child,
	    NULL, "(annotation?)");
    }

    return (ret);
}

/**
 * xmlSchemaParseAnyAttribute:
 * @ctxt:  a schema validation context
 * @schema:  the schema being built
 * @node:  a subtree containing XML Schema informations
 *
 * parse a XML schema AnyAttrribute declaration
 * *WARNING* this interface is highly subject to change
 *
 * Returns a wildcard or NULL.
 */
static xmlSchemaWildcardPtr
xmlSchemaParseAnyAttribute(xmlSchemaParserCtxtPtr ctxt,
                           xmlSchemaPtr schema, xmlNodePtr node)
{
    xmlSchemaWildcardPtr ret;
    xmlNodePtr child = NULL;
    xmlAttrPtr attr;

    if ((ctxt == NULL) || (schema == NULL) || (node == NULL))
        return (NULL);

    ret = xmlSchemaAddWildcard(ctxt, schema, XML_SCHEMA_TYPE_ANY_ATTRIBUTE,
	node);
    if (ret == NULL) {
        return (NULL);
    }
    /*
    * Check for illegal attributes.
    */
    attr = node->properties;
    while (attr != NULL) {
	if (attr->ns == NULL) {
	    if ((!xmlStrEqual(attr->name, BAD_CAST "id")) &&
	        (!xmlStrEqual(attr->name, BAD_CAST "namespace")) &&
		(!xmlStrEqual(attr->name, BAD_CAST "processContents"))) {
		xmlSchemaPIllegalAttrErr(ctxt,
		    XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		    NULL, NULL, attr);
	    }
	} else if (xmlStrEqual(attr->ns->href, xmlSchemaNs)) {
	    xmlSchemaPIllegalAttrErr(ctxt,
		XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		NULL, NULL, attr);
	}
	attr = attr->next;
    }
    xmlSchemaPValAttrID(ctxt, NULL, (xmlSchemaTypePtr) ret,
	node, BAD_CAST "id");
    /*
    * Parse the namespace list.
    */
    if (xmlSchemaParseWildcardNs(ctxt, schema, ret, node) != 0)
	return (NULL);
    /*
    * And now for the children...
    */
    child = node->children;
    if (IS_SCHEMA(child, "annotation")) {
        ret->annot = xmlSchemaParseAnnotation(ctxt, schema, child);
        child = child->next;
    }
    if (child != NULL) {
	xmlSchemaPContentErr(ctxt,
	    XML_SCHEMAP_S4S_ELEM_NOT_ALLOWED,
	    NULL, NULL, node, child,
	    NULL, "(annotation?)");
    }

    return (ret);
}


/**
 * xmlSchemaParseAttribute:
 * @ctxt:  a schema validation context
 * @schema:  the schema being built
 * @node:  a subtree containing XML Schema informations
 *
 * parse a XML schema Attrribute declaration
 * *WARNING* this interface is highly subject to change
 *
 * Returns the attribute declaration.
 */
static xmlSchemaAttributePtr
xmlSchemaParseAttribute(xmlSchemaParserCtxtPtr ctxt, xmlSchemaPtr schema,
                        xmlNodePtr node, int topLevel)
{
    const xmlChar *name, *attrValue;
    xmlChar *repName = NULL; /* The reported designation. */
    xmlSchemaAttributePtr ret;
    xmlNodePtr child = NULL;
    xmlAttrPtr attr, nameAttr;
    int isRef = 0;

    /*
     * Note that the w3c spec assumes the schema to be validated with schema
     * for schemas beforehand.
     *
     * 3.2.3 Constraints on XML Representations of Attribute Declarations
     */

    if ((ctxt == NULL) || (schema == NULL) || (node == NULL))
        return (NULL);
    attr = xmlSchemaGetPropNode(node, "ref");
    nameAttr = xmlSchemaGetPropNode(node, "name");

    if ((attr == NULL) && (nameAttr == NULL)) {
	/*
	* 3.2.3 : 3.1
	* One of ref or name must be present, but not both
	*/
	xmlSchemaPMissingAttrErr(ctxt, XML_SCHEMAP_SRC_ATTRIBUTE_3_1,
	    NULL, node, NULL,
	    "One of the attributes 'ref' or 'name' must be present");
	return (NULL);
    }
    if ((topLevel) || (attr == NULL)) {
	if (nameAttr == NULL) {
	    xmlSchemaPMissingAttrErr(ctxt, XML_SCHEMAP_S4S_ATTR_MISSING,
		NULL, node, "name", NULL);
	    return (NULL);
	}
    } else
	isRef = 1;

    if (isRef) {
	char buf[50];
	const xmlChar *refNs = NULL, *ref = NULL;

	/*
	* Parse as attribute reference.
	*/
	if (xmlSchemaPValAttrNodeQName(ctxt, schema,
	    (xmlChar **) &xmlSchemaElemDesAttrRef, NULL, attr, &refNs,
	    &ref) != 0) {
	    return (NULL);
	}
        snprintf(buf, 49, "#aRef%d", ctxt->counter++ + 1);
        name = (const xmlChar *) buf;
	ret = xmlSchemaAddAttribute(ctxt, schema, name, NULL, node, 0);
	if (ret == NULL) {
	    if (repName != NULL)
		xmlFree(repName);
	    return (NULL);
	}
	ret->type = XML_SCHEMA_TYPE_ATTRIBUTE;
	ret->node = node;
	ret->refNs = refNs;
	ret->ref = ref;
	xmlSchemaCheckReference(ctxt, schema, node, (xmlSchemaBasicItemPtr) ret,
	    refNs);
	/*
	xmlSchemaFormatTypeRep(&repName, (xmlSchemaTypePtr) ret, NULL, NULL);
	*/
	if (nameAttr != NULL)
	    xmlSchemaPMutualExclAttrErr(ctxt, XML_SCHEMAP_SRC_ATTRIBUTE_3_1,
		&repName, (xmlSchemaTypePtr) ret, nameAttr,
		"ref", "name");
	/*
	* Check for illegal attributes.
	*/
	attr = node->properties;
	while (attr != NULL) {
	    if (attr->ns == NULL) {
		if (xmlStrEqual(attr->name, BAD_CAST "type") ||
		    xmlStrEqual(attr->name, BAD_CAST "form")) {
		    /*
		    * 3.2.3 : 3.2
		    * If ref is present, then all of <simpleType>,
		    * form and type must be absent.
		    */
		    xmlSchemaPIllegalAttrErr(ctxt,
			XML_SCHEMAP_SRC_ATTRIBUTE_3_2, &repName,
			(xmlSchemaTypePtr) ret, attr);
		} else if ((!xmlStrEqual(attr->name, BAD_CAST "ref")) &&
		    (!xmlStrEqual(attr->name, BAD_CAST "use")) &&
		    (!xmlStrEqual(attr->name, BAD_CAST "id")) &&
		    (!xmlStrEqual(attr->name, BAD_CAST "name")) &&
		    (!xmlStrEqual(attr->name, BAD_CAST "fixed")) &&
		    (!xmlStrEqual(attr->name, BAD_CAST "default"))) {
		    xmlSchemaPIllegalAttrErr(ctxt,
			XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
			&repName, (xmlSchemaTypePtr) ret, attr);
		}
	    } else if (xmlStrEqual(attr->ns->href, xmlSchemaNs)) {
		xmlSchemaPIllegalAttrErr(ctxt,
		    XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		    &repName, (xmlSchemaTypePtr) ret, attr);
	    }
	    attr = attr->next;
	}
    } else {
        const xmlChar *ns = NULL;

	/*
	* Parse as attribute declaration.
	*/
	if (xmlSchemaPValAttrNode(ctxt,
	    (xmlChar **) &xmlSchemaElemDesAttrDecl, NULL, nameAttr,
	    xmlSchemaGetBuiltInType(XML_SCHEMAS_NCNAME), &name) != 0) {
	    return (NULL);
	}
	/*
	xmlSchemaFormatTypeRep(&repName, NULL, xmlSchemaElemDesAttrDecl, name);
	*/
	/*
	* 3.2.6 Schema Component Constraint: xmlns Not Allowed
	* TODO: Move this to the component layer.
	*/
	if (xmlStrEqual(name, BAD_CAST "xmlns")) {
	    xmlSchemaPSimpleTypeErr(ctxt,
		XML_SCHEMAP_NO_XMLNS,
		NULL, (xmlNodePtr) nameAttr,
		xmlSchemaGetBuiltInType(XML_SCHEMAS_NCNAME), NULL, NULL,
		"The value of type 'xs:NCName' must not match 'xmlns'",
		NULL, NULL);
	    if (repName != NULL)
		xmlFree(repName);
	    return (NULL);
	}
	/*
	* Evaluate the target namespace
	*/
	if (topLevel) {
	    ns = schema->targetNamespace;
	} else {
	    attr = xmlSchemaGetPropNode(node, "form");
	    if (attr != NULL) {
		attrValue = xmlSchemaGetNodeContent(ctxt, (xmlNodePtr) attr);
		if (xmlStrEqual(attrValue, BAD_CAST "qualified")) {
		    ns = schema->targetNamespace;
		} else if (!xmlStrEqual(attrValue, BAD_CAST "unqualified")) {
		    xmlSchemaPSimpleTypeErr(ctxt,
			XML_SCHEMAP_S4S_ATTR_INVALID_VALUE,
			NULL, (xmlNodePtr) attr,
			NULL, "(qualified | unqualified)",
			attrValue, NULL, NULL, NULL);
		}
	    } else if (schema->flags & XML_SCHEMAS_QUALIF_ATTR)
		ns = schema->targetNamespace;
	}
        ret = xmlSchemaAddAttribute(ctxt, schema, name, ns, node, topLevel);
	if (ret == NULL) {
	    if (repName != NULL)
		xmlFree(repName);
	    return (NULL);
	}
	ret->type = XML_SCHEMA_TYPE_ATTRIBUTE;
	ret->node = node;
	if (topLevel)
	    ret->flags |= XML_SCHEMAS_ATTR_GLOBAL;
	/*
	* 3.2.6 Schema Component Constraint: xsi: Not Allowed
	* TODO: Move this to the component layer.
	*/
	if (xmlStrEqual(ret->targetNamespace, xmlSchemaInstanceNs)) {
	    xmlSchemaPCustomErr(ctxt,
		XML_SCHEMAP_NO_XSI,
		&repName, (xmlSchemaTypePtr) ret, node,
		"The target namespace must not match '%s'",
		xmlSchemaInstanceNs);
	}
	/*
	* Check for illegal attributes.
	*/
	attr = node->properties;
	while (attr != NULL) {
	    if (attr->ns == NULL) {
		if ((!xmlStrEqual(attr->name, BAD_CAST "id")) &&
		    (!xmlStrEqual(attr->name, BAD_CAST "default")) &&
		    (!xmlStrEqual(attr->name, BAD_CAST "fixed")) &&
		    (!xmlStrEqual(attr->name, BAD_CAST "name")) &&
		    (!xmlStrEqual(attr->name, BAD_CAST "type"))) {
		    if ((topLevel) ||
		        ((!xmlStrEqual(attr->name, BAD_CAST "form")) &&
			 (!xmlStrEqual(attr->name, BAD_CAST "use")))) {
			xmlSchemaPIllegalAttrErr(ctxt,
			    XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
			    &repName, (xmlSchemaTypePtr) ret, attr);
		    }
		}
	    } else if (xmlStrEqual(attr->ns->href, xmlSchemaNs)) {
		xmlSchemaPIllegalAttrErr(ctxt, XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		    &repName, (xmlSchemaTypePtr) ret, attr);
	    }
	    attr = attr->next;
	}
	xmlSchemaPValAttrQName(ctxt, schema, &repName, (xmlSchemaTypePtr) ret,
	    node, "type", &ret->typeNs, &ret->typeName);
    }
    xmlSchemaPValAttrID(ctxt, NULL, (xmlSchemaTypePtr) ret,
	node, BAD_CAST "id");
    /*
    * Attribute "fixed".
    */
    ret->defValue = xmlSchemaGetProp(ctxt, node, "fixed");
    if (ret->defValue != NULL)
	ret->flags |= XML_SCHEMAS_ATTR_FIXED;
    /*
    * Attribute "default".
    */
    attr = xmlSchemaGetPropNode(node, "default");
    if (attr != NULL) {
	/*
	* 3.2.3 : 1
	* default and fixed must not both be present.
	*/
	if (ret->flags & XML_SCHEMAS_ATTR_FIXED) {
	    xmlSchemaPMutualExclAttrErr(ctxt, XML_SCHEMAP_SRC_ATTRIBUTE_1,
		&repName, (xmlSchemaTypePtr) ret, attr, "default", "fixed");
	} else
	    ret->defValue = xmlSchemaGetNodeContent(ctxt, (xmlNodePtr) attr);
    }
    if (topLevel == 0) {
	/*
	* Attribute "use".
	*/
	attr = xmlSchemaGetPropNode(node, "use");
	if (attr != NULL) {
	    attrValue = xmlSchemaGetNodeContent(ctxt, (xmlNodePtr) attr);
	    if (xmlStrEqual(attrValue, BAD_CAST "optional"))
		ret->occurs = XML_SCHEMAS_ATTR_USE_OPTIONAL;
	    else if (xmlStrEqual(attrValue, BAD_CAST "prohibited"))
		ret->occurs = XML_SCHEMAS_ATTR_USE_PROHIBITED;
	    else if (xmlStrEqual(attrValue, BAD_CAST "required"))
		ret->occurs = XML_SCHEMAS_ATTR_USE_REQUIRED;
	    else
		xmlSchemaPSimpleTypeErr(ctxt,
		    XML_SCHEMAP_INVALID_ATTR_USE,
		    (xmlSchemaTypePtr) ret, (xmlNodePtr) attr,
		    NULL, "(optional | prohibited | required)",
		    attrValue, NULL, NULL, NULL);
	} else
	    ret->occurs = XML_SCHEMAS_ATTR_USE_OPTIONAL;
	/*
	* 3.2.3 : 2
	* If default and use are both present, use must have
	* the actual value optional.
	*/
	if ((ret->occurs != XML_SCHEMAS_ATTR_USE_OPTIONAL) &&
	    (ret->defValue != NULL) &&
	    ((ret->flags & XML_SCHEMAS_ATTR_FIXED) == 0)) {
	    xmlSchemaPSimpleTypeErr(ctxt,
		XML_SCHEMAP_SRC_ATTRIBUTE_2,
		(xmlSchemaTypePtr) ret, (xmlNodePtr) attr,
		NULL, "(optional | prohibited | required)", NULL,
		"The value must be 'optional' if the attribute "
		"'default' is present as well", NULL, NULL);
	}
    }
    /*
    * And now for the children...
    */
    child = node->children;
    if (IS_SCHEMA(child, "annotation")) {
        ret->annot = xmlSchemaParseAnnotation(ctxt, schema, child);
        child = child->next;
    }
    if (isRef) {
	if (child != NULL) {
	    if (IS_SCHEMA(child, "simpleType"))
		/*
		* 3.2.3 : 3.2
		* If ref is present, then all of <simpleType>,
		* form and type must be absent.
		*/
		xmlSchemaPContentErr(ctxt, XML_SCHEMAP_SRC_ATTRIBUTE_3_2,
		    &repName, (xmlSchemaTypePtr) ret, node, child, NULL,
		    "(annotation?)");
	    else
		xmlSchemaPContentErr(ctxt, XML_SCHEMAP_S4S_ELEM_NOT_ALLOWED,
		    &repName, (xmlSchemaTypePtr) ret, node, child, NULL,
		    "(annotation?)");
	}
    } else {
	if (IS_SCHEMA(child, "simpleType")) {
	    if (ret->typeName != NULL) {
		/*
		* 3.2.3 : 4
		* type and <simpleType> must not both be present.
		*/
		xmlSchemaPContentErr(ctxt, XML_SCHEMAP_SRC_ATTRIBUTE_4,
		    &repName,  (xmlSchemaTypePtr) ret, node, child,
		    "The attribute 'type' and the <simpleType> child "
		    "are mutually exclusive", NULL);
	    } else
		ret->subtypes = xmlSchemaParseSimpleType(ctxt, schema, child, 0);
	    child = child->next;
	}
	if (child != NULL)
	    xmlSchemaPContentErr(ctxt, XML_SCHEMAP_S4S_ELEM_NOT_ALLOWED,
		&repName, (xmlSchemaTypePtr) ret, node, child, NULL,
		"(annotation?, simpleType?)");
    }
    /*
    * Cleanup.
    */
    if (repName != NULL)
	xmlFree(repName);
    return (ret);
}

/**
 * xmlSchemaParseAttributeGroup:
 * @ctxt:  a schema validation context
 * @schema:  the schema being built
 * @node:  a subtree containing XML Schema informations
 *
 * parse a XML schema Attribute Group declaration
 * *WARNING* this interface is highly subject to change
 *
 * Returns the attribute group or NULL in case of error.
 */
static xmlSchemaAttributeGroupPtr
xmlSchemaParseAttributeGroup(xmlSchemaParserCtxtPtr ctxt,
                             xmlSchemaPtr schema, xmlNodePtr node,
			     int topLevel)
{
    const xmlChar *name;
    xmlSchemaAttributeGroupPtr ret;
    xmlNodePtr child = NULL;
    const xmlChar *oldcontainer;
    xmlAttrPtr attr, nameAttr;

    if ((ctxt == NULL) || (schema == NULL) || (node == NULL))
        return (NULL);

    nameAttr = xmlSchemaGetPropNode(node, "name");
    attr = xmlSchemaGetPropNode(node, "ref");
    if ((topLevel) || (attr == NULL)) {
	/*
	* Parse as an attribute group definition.
	* Note that those are allowed at top level only.
	*/
	if (nameAttr == NULL) {
	    xmlSchemaPMissingAttrErr(ctxt,
		XML_SCHEMAP_S4S_ATTR_MISSING,
		NULL, node, "name", NULL);
	    return (NULL);
	}
	/* REDUNDANT: name = xmlSchemaGetNodeContent(ctxt,
	* (xmlNodePtr) nameAttr);
	*/
	/*
	* The name is crucial, exit if invalid.
	*/
	if (xmlSchemaPValAttrNode(ctxt,
	    NULL, NULL, nameAttr,
	    xmlSchemaGetBuiltInType(XML_SCHEMAS_NCNAME), &name) != 0) {
	    return (NULL);
	}
	ret = xmlSchemaAddAttributeGroup(ctxt, schema, name, node);
	if (ret == NULL)
	    return (NULL);
	ret->type = XML_SCHEMA_TYPE_ATTRIBUTEGROUP;
	ret->flags |= XML_SCHEMAS_ATTRGROUP_GLOBAL;
	ret->node = node;
	ret->targetNamespace = schema->targetNamespace;
    } else {
	char buf[50];
	const xmlChar *refNs = NULL, *ref = NULL;

	/*
	* Parse as an attribute group definition reference.
	*/
	if (attr == NULL) {
	    xmlSchemaPMissingAttrErr(ctxt,
		XML_SCHEMAP_S4S_ATTR_MISSING,
		NULL, node, "ref", NULL);
	}
	xmlSchemaPValAttrNodeQName(ctxt, schema,
	    NULL, NULL, attr, &refNs,&ref);

        snprintf(buf, 49, "#agRef%d", ctxt->counter++ + 1);
	name = (const xmlChar *) buf;
	if (name == NULL) {
	    xmlSchemaPErrMemory(ctxt, "creating internal name for an "
		"attribute group definition reference", node);
            return (NULL);
        }
	ret = xmlSchemaAddAttributeGroup(ctxt, schema, name, node);
	if (ret == NULL)
	    return (NULL);
	ret->type = XML_SCHEMA_TYPE_ATTRIBUTEGROUP;
	ret->ref = ref;
	ret->refNs = refNs;
	ret->node = node;
	xmlSchemaCheckReference(ctxt, schema, node,
	    (xmlSchemaBasicItemPtr) ret, refNs);
    }
    /*
    * Check for illegal attributes.
    */
    attr = node->properties;
    while (attr != NULL) {
	if (attr->ns == NULL) {
	    if ((((topLevel == 0) && (!xmlStrEqual(attr->name, BAD_CAST "ref"))) ||
		 (topLevel && (!xmlStrEqual(attr->name, BAD_CAST "name")))) &&
		(!xmlStrEqual(attr->name, BAD_CAST "id")))
	    {
		xmlSchemaPIllegalAttrErr(ctxt,
		    XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		    NULL, NULL, attr);
	    }
	} else if (xmlStrEqual(attr->ns->href, xmlSchemaNs)) {
	    xmlSchemaPIllegalAttrErr(ctxt,
		XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		NULL, NULL, attr);
	}
	attr = attr->next;
    }
    /* TODO: Validate "id" ? */
    /*
    * And now for the children...
    */
    oldcontainer = ctxt->container;
    ctxt->container = name;
    child = node->children;
    if (IS_SCHEMA(child, "annotation")) {
        ret->annot = xmlSchemaParseAnnotation(ctxt, schema, child);
        child = child->next;
    }
    if (topLevel) {
	child = xmlSchemaParseAttrDecls(ctxt, schema, child, (xmlSchemaTypePtr) ret);
	if (IS_SCHEMA(child, "anyAttribute")) {
	    ret->attributeWildcard = xmlSchemaParseAnyAttribute(ctxt, schema, child);
	    child = child->next;
	}
    }
    if (child != NULL) {
	xmlSchemaPContentErr(ctxt,
	    XML_SCHEMAP_S4S_ELEM_NOT_ALLOWED,
	    NULL, NULL, node, child, NULL,
	    "(annotation?)");
    }
    ctxt->container = oldcontainer;
    return (ret);
}

/**
 * xmlSchemaPValAttrFormDefault:
 * @value:  the value
 * @flags: the flags to be modified
 * @flagQualified: the specific flag for "qualified"
 *
 * Returns 0 if the value is valid, 1 otherwise.
 */
static int
xmlSchemaPValAttrFormDefault(const xmlChar *value,
			     int *flags,
			     int flagQualified)
{
    if (xmlStrEqual(value, BAD_CAST "qualified")) {
	if  ((*flags & flagQualified) == 0)
	    *flags |= flagQualified;
    } else if (!xmlStrEqual(value, BAD_CAST "unqualified"))
	return (1);

    return (0);
}

/**
 * xmlSchemaPValAttrBlockFinal:
 * @value:  the value
 * @flags: the flags to be modified
 * @flagAll: the specific flag for "#all"
 * @flagExtension: the specific flag for "extension"
 * @flagRestriction: the specific flag for "restriction"
 * @flagSubstitution: the specific flag for "substitution"
 * @flagList: the specific flag for "list"
 * @flagUnion: the specific flag for "union"
 *
 * Validates the value of the attribute "final" and "block". The value
 * is converted into the specified flag values and returned in @flags.
 *
 * Returns 0 if the value is valid, 1 otherwise.
 */

static int
xmlSchemaPValAttrBlockFinal(const xmlChar *value,
			    int *flags,
			    int flagAll,
			    int flagExtension,
			    int flagRestriction,
			    int flagSubstitution,
			    int flagList,
			    int flagUnion)
{
    int ret = 0;

    /*
    * TODO: This does not check for dublicate entries.
    */
    if ((flags == NULL) || (value == NULL))
	return (-1);
    if (value[0] == 0)
	return (0);
    if (xmlStrEqual(value, BAD_CAST "#all")) {
	if (flagAll != -1)
	    *flags |= flagAll;
	else {
	    if (flagExtension != -1)
		*flags |= flagExtension;
	    if (flagRestriction != -1)
		*flags |= flagRestriction;
	    if (flagSubstitution != -1)
		*flags |= flagSubstitution;
	    if (flagList != -1)
		*flags |= flagList;
	    if (flagUnion != -1)
		*flags |= flagUnion;
	}
    } else {
	const xmlChar *end, *cur = value;
	xmlChar *item;

	do {
	    while (IS_BLANK_CH(*cur))
		cur++;
	    end = cur;
	    while ((*end != 0) && (!(IS_BLANK_CH(*end))))
		end++;
	    if (end == cur)
		break;
	    item = xmlStrndup(cur, end - cur);
	    if (xmlStrEqual(item, BAD_CAST "extension")) {
		if (flagExtension != -1) {
		    if ((*flags & flagExtension) == 0)
			*flags |= flagExtension;
		} else
		    ret = 1;
	    } else if (xmlStrEqual(item, BAD_CAST "restriction")) {
		if (flagRestriction != -1) {
		    if ((*flags & flagRestriction) == 0)
			*flags |= flagRestriction;
		} else
		    ret = 1;
	    } else if (xmlStrEqual(item, BAD_CAST "substitution")) {
		if (flagSubstitution != -1) {
		    if ((*flags & flagSubstitution) == 0)
			*flags |= flagSubstitution;
		} else
		    ret = 1;
	    } else if (xmlStrEqual(item, BAD_CAST "list")) {
		if (flagList != -1) {
		    if ((*flags & flagList) == 0)
			*flags |= flagList;
		} else
		    ret = 1;
	    } else if (xmlStrEqual(item, BAD_CAST "union")) {
		if (flagUnion != -1) {
		    if ((*flags & flagUnion) == 0)
			*flags |= flagUnion;
		} else
		    ret = 1;
	    } else
		ret = 1;
	    if (item != NULL)
		xmlFree(item);
	    cur = end;
	} while ((ret == 0) && (*cur != 0));
    }

    return (ret);
}

static int
xmlSchemaCheckCSelectorXPath(xmlSchemaParserCtxtPtr ctxt,
			     xmlSchemaIDCPtr idc,
			     xmlSchemaIDCSelectPtr selector,
			     xmlAttrPtr attr,
			     int isField)
{
    xmlNodePtr node;

    /*
    * c-selector-xpath:
    * Schema Component Constraint: Selector Value OK
    *
    * TODO: 1 The {selector} must be a valid XPath expression, as defined
    * in [XPath].
    */
    if (selector == NULL) {
	xmlSchemaPErr(ctxt, idc->node,
	    XML_SCHEMAP_INTERNAL,
	    "Internal error: xmlSchemaCheckCSelectorXPath, "
	    "the selector is not specified.\n", NULL, NULL);
	return (-1);
    }
    if (attr == NULL)
	node = idc->node;
    else
	node = (xmlNodePtr) attr;
    if (selector->xpath == NULL) {
	xmlSchemaPCustomErr(ctxt,
	    /* TODO: Adjust error code. */
	    XML_SCHEMAP_S4S_ATTR_INVALID_VALUE,
	    NULL, NULL, node,
	    "The XPath expression of the selector is not valid", NULL);
	return (XML_SCHEMAP_S4S_ATTR_INVALID_VALUE);
    } else {
	const xmlChar **nsArray = NULL;
	xmlNsPtr *nsList = NULL;
	/*
	* Compile the XPath expression.
	*/
	/*
	* TODO: We need the array of in-scope namespaces for compilation.
	* TODO: Call xmlPatterncompile with different options for selector/
	* field.
	*/
	nsList = xmlGetNsList(attr->doc, attr->parent);
	/*
	* Build an array of prefixes and namespaces.
	*/
	if (nsList != NULL) {
	    int i, count = 0;
	    xmlNsPtr ns;

	    for (i = 0; nsList[i] != NULL; i++)
		count++;

	    nsArray = (const xmlChar **) xmlMalloc(
		(count * 2 + 1) * sizeof(const xmlChar *));
	    if (nsArray == NULL) {
		xmlSchemaPErrMemory(ctxt, "allocating a namespace array",
		    NULL);
		return (-1);
	    }
	    for (i = 0; i < count; i++) {
		ns = nsList[i];
		nsArray[2 * i] = nsList[i]->href;
		nsArray[2 * i + 1] = nsList[i]->prefix;
	    }
	    nsArray[count * 2] = NULL;
	    xmlFree(nsList);
	}
	/*
	* TODO: Differentiate between "selector" and "field".
	*/
	if (isField)
	    selector->xpathComp = (void *) xmlPatterncompile(selector->xpath,
		NULL, XML_PATTERN_XSFIELD, nsArray);
	else
	    selector->xpathComp = (void *) xmlPatterncompile(selector->xpath,
		NULL, XML_PATTERN_XSSEL, nsArray);
	if (nsArray != NULL)
	    xmlFree((xmlChar **) nsArray);

	if (selector->xpathComp == NULL) {
	    xmlSchemaPCustomErr(ctxt,
		/* TODO: Adjust error code? */
		XML_SCHEMAP_S4S_ATTR_INVALID_VALUE,
		NULL, NULL, node,
		"The XPath expression '%s' could not be "
		"compiled", selector->xpath);
	    return (XML_SCHEMAP_S4S_ATTR_INVALID_VALUE);
	}
    }
    return (0);
}

#define ADD_ANNOTATION(annot)   \
    xmlSchemaAnnotPtr cur = item->annot; \
    if (item->annot == NULL) {  \
	item->annot = annot;    \
	return (annot);         \
    }                           \
    cur = item->annot;          \
    if (cur->next != NULL) {    \
	cur = cur->next;	\
    }                           \
    cur->next = annot;

/**
 * xmlSchemaAssignAnnotation:
 * @item: the schema component
 * @annot: the annotation
 *
 * Adds the annotation to the given schema component.
 *
 * Returns the given annotaion.
 */
static xmlSchemaAnnotPtr
xmlSchemaAddAnnotation(xmlSchemaAnnotItemPtr annItem,
		       xmlSchemaAnnotPtr annot)
{
    if ((annItem == NULL) || (annot == NULL))
	return (NULL);
    switch (annItem->type) {
	case XML_SCHEMA_TYPE_ELEMENT: {
		xmlSchemaElementPtr item = (xmlSchemaElementPtr) annItem;
		ADD_ANNOTATION(annot)
	    }
	    break;
	case XML_SCHEMA_TYPE_ATTRIBUTE: {
		xmlSchemaAttributePtr item = (xmlSchemaAttributePtr) annItem;
		ADD_ANNOTATION(annot)
	    }
	    break;
	case XML_SCHEMA_TYPE_ANY_ATTRIBUTE:
	case XML_SCHEMA_TYPE_ANY: {
		xmlSchemaWildcardPtr item = (xmlSchemaWildcardPtr) annItem;
		ADD_ANNOTATION(annot)
	    }
	    break;
	case XML_SCHEMA_TYPE_PARTICLE:
	case XML_SCHEMA_TYPE_IDC_KEY:
	case XML_SCHEMA_TYPE_IDC_KEYREF:
	case XML_SCHEMA_TYPE_IDC_UNIQUE: {
		xmlSchemaAnnotItemPtr item = (xmlSchemaAnnotItemPtr) annItem;
		ADD_ANNOTATION(annot)
	    }
	    break;
	case XML_SCHEMA_TYPE_ATTRIBUTEGROUP: {
		xmlSchemaAttributeGroupPtr item =
		    (xmlSchemaAttributeGroupPtr) annItem;
		ADD_ANNOTATION(annot)
	    }
	    break;
	case XML_SCHEMA_TYPE_NOTATION: {
		xmlSchemaNotationPtr item = (xmlSchemaNotationPtr) annItem;
		ADD_ANNOTATION(annot)
	    }
	    break;
	case XML_SCHEMA_FACET_MININCLUSIVE:
	case XML_SCHEMA_FACET_MINEXCLUSIVE:
	case XML_SCHEMA_FACET_MAXINCLUSIVE:
	case XML_SCHEMA_FACET_MAXEXCLUSIVE:
	case XML_SCHEMA_FACET_TOTALDIGITS:
	case XML_SCHEMA_FACET_FRACTIONDIGITS:
	case XML_SCHEMA_FACET_PATTERN:
	case XML_SCHEMA_FACET_ENUMERATION:
	case XML_SCHEMA_FACET_WHITESPACE:
	case XML_SCHEMA_FACET_LENGTH:
	case XML_SCHEMA_FACET_MAXLENGTH:
	case XML_SCHEMA_FACET_MINLENGTH: {
		xmlSchemaFacetPtr item = (xmlSchemaFacetPtr) annItem;
		ADD_ANNOTATION(annot)
	    }
	    break;
	case XML_SCHEMA_TYPE_SIMPLE:
	case XML_SCHEMA_TYPE_COMPLEX: {
		xmlSchemaTypePtr item = (xmlSchemaTypePtr) annItem;
		ADD_ANNOTATION(annot)
	    }
	    break;
	case XML_SCHEMA_TYPE_GROUP: {
		xmlSchemaModelGroupDefPtr item = (xmlSchemaModelGroupDefPtr) annItem;
		ADD_ANNOTATION(annot)
	    }
	    break;
	case XML_SCHEMA_TYPE_SEQUENCE:
	case XML_SCHEMA_TYPE_CHOICE:
	case XML_SCHEMA_TYPE_ALL: {
		xmlSchemaModelGroupPtr item = (xmlSchemaModelGroupPtr) annItem;
		ADD_ANNOTATION(annot)
	    }
	    break;
	default:
	     xmlSchemaPCustomErr(NULL,
		XML_SCHEMAP_INTERNAL,
		NULL, NULL, NULL,
		"Internal error: xmlSchemaAddAnnotation, "
		"The item is not a annotated schema component", NULL);
	     break;
    }
    return (annot);
}

/**
 * xmlSchemaParseIDCSelectorAndField:
 * @ctxt:  a schema validation context
 * @schema:  the schema being built
 * @node:  a subtree containing XML Schema informations
 *
 * Parses a XML Schema identity-contraint definition's
 * <selector> and <field> elements.
 *
 * Returns the parsed identity-constraint definition.
 */
static xmlSchemaIDCSelectPtr
xmlSchemaParseIDCSelectorAndField(xmlSchemaParserCtxtPtr ctxt,
			  xmlSchemaPtr schema,
			  xmlSchemaIDCPtr idc,
			  xmlNodePtr node,
			  int isField)
{
    xmlSchemaIDCSelectPtr item;
    xmlNodePtr child = NULL;
    xmlAttrPtr attr;

    /*
    * Check for illegal attributes.
    */
    attr = node->properties;
    while (attr != NULL) {
	if (attr->ns == NULL) {
	    if ((!xmlStrEqual(attr->name, BAD_CAST "id")) &&
		(!xmlStrEqual(attr->name, BAD_CAST "xpath"))) {
		xmlSchemaPIllegalAttrErr(ctxt,
		    XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		    NULL, NULL, attr);
	    }
	} else if (xmlStrEqual(attr->ns->href, xmlSchemaNs)) {
	    xmlSchemaPIllegalAttrErr(ctxt,
		XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		NULL, NULL, attr);
	}
	attr = attr->next;
    }
    /*
    * Create the item.
    */
    item = (xmlSchemaIDCSelectPtr) xmlMalloc(sizeof(xmlSchemaIDCSelect));
    if (item == NULL) {
        xmlSchemaPErrMemory(ctxt,
	    "allocating a 'selector' of an identity-constraint definition",
	    NULL);
        return (NULL);
    }
    memset(item, 0, sizeof(xmlSchemaIDCSelect));
    /*
    * Attribute "xpath" (mandatory).
    */
    attr = xmlSchemaGetPropNode(node, "xpath");
    if (attr == NULL) {
    	xmlSchemaPMissingAttrErr(ctxt,
	    XML_SCHEMAP_S4S_ATTR_MISSING,
	    NULL, node,
	    "name", NULL);
    } else {
	item->xpath = xmlSchemaGetNodeContent(ctxt, (xmlNodePtr) attr);
	/*
	* URGENT TODO: "field"s have an other syntax than "selector"s.
	*/

	if (xmlSchemaCheckCSelectorXPath(ctxt, idc, item, attr,
	    isField) == -1) {
	    xmlSchemaPErr(ctxt,
		(xmlNodePtr) attr,
		XML_SCHEMAP_INTERNAL,
		"Internal error: xmlSchemaParseIDCSelectorAndField, "
		"validating the XPath expression of a IDC selector.\n",
		NULL, NULL);
	}

    }
    xmlSchemaPValAttrID(ctxt, NULL, NULL, node, BAD_CAST "id");
    /*
    * And now for the children...
    */
    child = node->children;
    if (IS_SCHEMA(child, "annotation")) {
	/*
	* Add the annotation to the parent IDC.
	*/
	xmlSchemaAddAnnotation((xmlSchemaAnnotItemPtr) idc,
	    xmlSchemaParseAnnotation(ctxt, schema, child));
	child = child->next;
    }
    if (child != NULL) {
	xmlSchemaPContentErr(ctxt,
	    XML_SCHEMAP_S4S_ELEM_NOT_ALLOWED,
	    NULL, NULL, node, child,
	    NULL, "(annotation?)");
    }

    return (item);
}

/**
 * xmlSchemaParseIDC:
 * @ctxt:  a schema validation context
 * @schema:  the schema being built
 * @node:  a subtree containing XML Schema informations
 *
 * Parses a XML Schema identity-contraint definition.
 *
 * Returns the parsed identity-constraint definition.
 */
static xmlSchemaIDCPtr
xmlSchemaParseIDC(xmlSchemaParserCtxtPtr ctxt,
		  xmlSchemaPtr schema,
		  xmlNodePtr node,
		  xmlSchemaTypeType idcCategory,
		  const xmlChar *targetNamespace)
{
    xmlSchemaIDCPtr item = NULL;
    xmlNodePtr child = NULL;
    xmlAttrPtr attr;
    const xmlChar *name = NULL;
    xmlSchemaIDCSelectPtr field = NULL, lastField = NULL;
    int resAdd;

    /*
    * Check for illegal attributes.
    */
    attr = node->properties;
    while (attr != NULL) {
	if (attr->ns == NULL) {
	    if ((!xmlStrEqual(attr->name, BAD_CAST "id")) &&
		(!xmlStrEqual(attr->name, BAD_CAST "name")) &&
		((idcCategory != XML_SCHEMA_TYPE_IDC_KEYREF) ||
		 (!xmlStrEqual(attr->name, BAD_CAST "refer")))) {
		xmlSchemaPIllegalAttrErr(ctxt,
		    XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		    NULL, NULL, attr);
	    }
	} else if (xmlStrEqual(attr->ns->href, xmlSchemaNs)) {
	    xmlSchemaPIllegalAttrErr(ctxt,
		XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		NULL, NULL, attr);
	}
	attr = attr->next;
    }
    /*
    * Attribute "name" (mandatory).
    */
    attr = xmlSchemaGetPropNode(node, "name");
    if (attr == NULL) {
	xmlSchemaPMissingAttrErr(ctxt,
	    XML_SCHEMAP_S4S_ATTR_MISSING,
	    NULL, node,
	    "name", NULL);
	return (NULL);
    } else if (xmlSchemaPValAttrNode(ctxt,
	NULL, NULL, attr,
	xmlSchemaGetBuiltInType(XML_SCHEMAS_NCNAME), &name) != 0) {
	return (NULL);
    }
    /*
    * Create the component.
    */
    if (schema->idcDef == NULL)
        schema->idcDef = xmlHashCreateDict(10, ctxt->dict);
    if (schema->idcDef == NULL)
        return (NULL);

    item = (xmlSchemaIDCPtr) xmlMalloc(sizeof(xmlSchemaIDC));
    if (item == NULL) {
        xmlSchemaPErrMemory(ctxt,
	    "allocating an identity-constraint definition", NULL);
        return (NULL);
    }
    /*
    * Add the IDC to the list of IDCs on the schema component.
    */
    resAdd = xmlHashAddEntry2(schema->idcDef, name, targetNamespace, item);
    if (resAdd != 0) {
	xmlSchemaPCustomErrExt(ctxt,
	    XML_SCHEMAP_REDEFINED_TYPE,
	    NULL, NULL, node,
	    "An identity-constraint definition with the name '%s' "
	    "and targetNamespace '%s' does already exist",
	    name, targetNamespace, NULL);
	xmlFree(item);
	return (NULL);
    }
    memset(item, 0, sizeof(xmlSchemaIDC));
    item->name = name;
    item->type = idcCategory;
    item->node = node;
    if (ctxt->assemble != NULL)
	xmlSchemaAddAssembledItem(ctxt, (xmlSchemaTypePtr) item);
    /*
    * The target namespace of the parent element declaration.
    */
    item->targetNamespace = targetNamespace;
    xmlSchemaPValAttrID(ctxt, NULL, (xmlSchemaTypePtr) item,
	node, BAD_CAST "id");
    if (idcCategory == XML_SCHEMA_TYPE_IDC_KEYREF) {
	/*
	* Attribute "refer" (mandatory).
	*/
	attr = xmlSchemaGetPropNode(node, "refer");
	if (attr == NULL) {
	    xmlSchemaPMissingAttrErr(ctxt,
		XML_SCHEMAP_S4S_ATTR_MISSING,
		NULL, node,
		"refer", NULL);
	} else {
	    /*
	    * Create a reference item.
	    */
	    item->ref = xmlSchemaNewQNameRef(schema, XML_SCHEMA_TYPE_IDC_KEY,
		NULL, NULL);
	    if (item->ref == NULL)
		return (NULL);
	    xmlSchemaPValAttrNodeQName(ctxt, schema,
		NULL, NULL, attr,
		&(item->ref->targetNamespace),
		&(item->ref->name));
	    xmlSchemaCheckReference(ctxt, schema, node,
		(xmlSchemaBasicItemPtr) item,
		item->ref->targetNamespace);
	}
    }
    /*
    * And now for the children...
    */
    child = node->children;
    if (IS_SCHEMA(child, "annotation")) {
	item->annot = xmlSchemaParseAnnotation(ctxt, schema, child);
	child = child->next;
    }
    if (child == NULL) {
	xmlSchemaPContentErr(ctxt,
		XML_SCHEMAP_S4S_ELEM_MISSING,
		NULL, NULL, node, child,
		"A child element is missing",
		"(annotation?, (selector, field+))");
    }
    /*
    * Child element <selector>.
    */
    if (IS_SCHEMA(child, "selector")) {
	item->selector = xmlSchemaParseIDCSelectorAndField(ctxt, schema,
	    item, child, 0);
	child = child->next;
	/*
	* Child elements <field>.
	*/
	if (IS_SCHEMA(child, "field")) {
	    do {
		field = xmlSchemaParseIDCSelectorAndField(ctxt, schema,
		    item, child, 1);
		if (field != NULL) {
		    field->index = item->nbFields;
		    item->nbFields++;
		    if (lastField != NULL)
			lastField->next = field;
		    else
			item->fields = field;
		    lastField = field;
		}
		child = child->next;
	    } while (IS_SCHEMA(child, "field"));
	} else {
	    xmlSchemaPContentErr(ctxt,
		XML_SCHEMAP_S4S_ELEM_NOT_ALLOWED,
		NULL, NULL, node, child,
		NULL, "(annotation?, (selector, field+))");
	}
    }
    if (child != NULL) {
	xmlSchemaPContentErr(ctxt,
	    XML_SCHEMAP_S4S_ELEM_NOT_ALLOWED,
	    NULL, NULL, node, child,
	    NULL, "(annotation?, (selector, field+))");
    }

    return (item);
}

/**
 * xmlSchemaParseElement:
 * @ctxt:  a schema validation context
 * @schema:  the schema being built
 * @node:  a subtree containing XML Schema informations
 * @topLevel: indicates if this is global declaration
 *
 * Parses a XML schema element declaration.
 * *WARNING* this interface is highly subject to change
 *
 * Returns the element declaration or a particle; NULL in case
 * of an error or if the particle has minOccurs==maxOccurs==0.
 */
static xmlSchemaBasicItemPtr
xmlSchemaParseElement(xmlSchemaParserCtxtPtr ctxt, xmlSchemaPtr schema,
                      xmlNodePtr node, int topLevel)
{
    xmlSchemaElementPtr decl = NULL;
    xmlSchemaParticlePtr particle = NULL;
    xmlSchemaAnnotPtr annot = NULL;
    xmlNodePtr child = NULL;
    xmlAttrPtr attr, nameAttr;
    int min, max, isRef = 0;
    xmlChar *des = NULL;

    /* 3.3.3 Constraints on XML Representations of Element Declarations */
    /* TODO: Complete implementation of 3.3.6 */

    if ((ctxt == NULL) || (schema == NULL) || (node == NULL))
        return (NULL);
    /*
    * If we get a "ref" attribute on a local <element> we will assume it's
    * a reference - even if there's a "name" attribute; this seems to be more
    * robust.
    */
    nameAttr = xmlSchemaGetPropNode(node, "name");
    attr = xmlSchemaGetPropNode(node, "ref");
    if ((topLevel) || (attr == NULL)) {
	if (nameAttr == NULL) {
	    xmlSchemaPMissingAttrErr(ctxt,
		XML_SCHEMAP_S4S_ATTR_MISSING,
		NULL, node, "name", NULL);
	    return (NULL);
	}
    } else
	isRef = 1;

    xmlSchemaPValAttrID(ctxt, NULL, NULL, node, BAD_CAST "id");
    child = node->children;
    if (IS_SCHEMA(child, "annotation")) {
	annot = xmlSchemaParseAnnotation(ctxt, schema, child);
	child = child->next;
    }
    /*
    * Skip particle part if a global declaration.
    */
    if (topLevel)
	goto declaration_part;
    /*
    * The particle part ==================================================
    */
    min = xmlGetMinOccurs(ctxt, node, 0, -1, 1, "xs:nonNegativeInteger");
    max = xmlGetMaxOccurs(ctxt, node, 0, UNBOUNDED, 1, "(xs:nonNegativeInteger | unbounded)");
    xmlSchemaPCheckParticleCorrect_2(ctxt, NULL, node, min, max);
    particle = xmlSchemaAddParticle(ctxt, schema, node, min, max);
    if (particle == NULL)
	goto return_null;

    /* ret->flags |= XML_SCHEMAS_ELEM_REF; */

    if (isRef) {
	const xmlChar *refNs = NULL, *ref = NULL;
	xmlSchemaQNameRefPtr refer = NULL;
	/*
	* The reference part =============================================
	*/
	xmlSchemaPValAttrNodeQName(ctxt, schema,
	    NULL, NULL, attr, &refNs, &ref);
	xmlSchemaCheckReference(ctxt, schema, node, NULL, refNs);
	/*
	* SPEC (3.3.3 : 2.1) "One of ref or name must be present, but not both"
	*/
	if (nameAttr != NULL) {
	    xmlSchemaPMutualExclAttrErr(ctxt,
		XML_SCHEMAP_SRC_ELEMENT_2_1,
		NULL, NULL, nameAttr, "ref", "name");
	}
	/*
	* Check for illegal attributes.
	*/
	attr = node->properties;
	while (attr != NULL) {
	    if (attr->ns == NULL) {
		if (xmlStrEqual(attr->name, BAD_CAST "ref") ||
		    xmlStrEqual(attr->name, BAD_CAST "name") ||
		    xmlStrEqual(attr->name, BAD_CAST "id") ||
		    xmlStrEqual(attr->name, BAD_CAST "maxOccurs") ||
		    xmlStrEqual(attr->name, BAD_CAST "minOccurs"))
		{
		    attr = attr->next;
		    continue;
		} else {
		    /* SPEC (3.3.3 : 2.2) */
		    xmlSchemaPCustomAttrErr(ctxt,
			XML_SCHEMAP_SRC_ELEMENT_2_2,
			NULL, NULL, attr,
			"Only the attributes 'minOccurs', 'maxOccurs' and "
			"'id' are allowed in addition to 'ref'");
		    break;
		}
	    } else if (xmlStrEqual(attr->ns->href, xmlSchemaNs)) {
		xmlSchemaPIllegalAttrErr(ctxt,
		    XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		    NULL, NULL, attr);
	    }
	    attr = attr->next;
	}
	/*
	* No children except <annotation> expected.
	*/
	if (child != NULL) {
	    xmlSchemaPContentErr(ctxt, XML_SCHEMAP_S4S_ELEM_NOT_ALLOWED,
		NULL, NULL, node, child, NULL, "(annotation?)");
	}
	if ((min == 0) && (max == 0))
	    goto return_null;
	/*
	* Create the reference item.
	*/
	refer = xmlSchemaNewQNameRef(schema, XML_SCHEMA_TYPE_ELEMENT,
	    ref, refNs);
	if (refer == NULL)
	    goto return_null;
	particle->children = (xmlSchemaTreeItemPtr) refer;
	particle->annot = annot;
	/*
	* Add to assembled items; the reference need to be resolved.
	*/
	if (ctxt->assemble != NULL)
	    xmlSchemaAddAssembledItem(ctxt, (xmlSchemaTypePtr) particle);

	return ((xmlSchemaBasicItemPtr) particle);
    }
    /*
    * The declaration part ===============================================
    */
declaration_part:
    {
	const xmlChar *ns = NULL, *fixed, *name, *oldcontainer, *attrValue;
	xmlSchemaIDCPtr curIDC = NULL, lastIDC = NULL;

	if (xmlSchemaPValAttrNode(ctxt, NULL, NULL, nameAttr,
	    xmlSchemaGetBuiltInType(XML_SCHEMAS_NCNAME), &name) != 0)
	    goto return_null;
	/*
	* Evaluate the target namespace.
	*/
	if (topLevel) {
	    ns = schema->targetNamespace;
	} else {
	    attr = xmlSchemaGetPropNode(node, "form");
	    if (attr != NULL) {
		attrValue = xmlSchemaGetNodeContent(ctxt, (xmlNodePtr) attr);
		if (xmlStrEqual(attrValue, BAD_CAST "qualified")) {
		    ns = schema->targetNamespace;
		} else if (!xmlStrEqual(attrValue, BAD_CAST "unqualified")) {
		    xmlSchemaPSimpleTypeErr(ctxt,
			XML_SCHEMAP_S4S_ATTR_INVALID_VALUE,
			NULL, (xmlNodePtr) attr,
			NULL, "(qualified | unqualified)",
			attrValue, NULL, NULL, NULL);
		}
	    } else if (schema->flags & XML_SCHEMAS_QUALIF_ELEM)
		ns = schema->targetNamespace;
	}
	decl = xmlSchemaAddElement(ctxt, schema, name, ns, node, topLevel);
	if (decl == NULL) {
	    goto return_null;
	}
	decl->type = XML_SCHEMA_TYPE_ELEMENT;
	decl->node = node;
	decl->targetNamespace = ns;
	/*
	* Check for illegal attributes.
	*/
	attr = node->properties;
	while (attr != NULL) {
	    if (attr->ns == NULL) {
		if ((!xmlStrEqual(attr->name, BAD_CAST "name")) &&
		    (!xmlStrEqual(attr->name, BAD_CAST "type")) &&
		    (!xmlStrEqual(attr->name, BAD_CAST "id")) &&
		    (!xmlStrEqual(attr->name, BAD_CAST "default")) &&
		    (!xmlStrEqual(attr->name, BAD_CAST "fixed")) &&
		    (!xmlStrEqual(attr->name, BAD_CAST "block")) &&
		    (!xmlStrEqual(attr->name, BAD_CAST "nillable")))
		{
		    if (topLevel == 0) {
			if ((!xmlStrEqual(attr->name, BAD_CAST "maxOccurs")) &&
			    (!xmlStrEqual(attr->name, BAD_CAST "minOccurs")) &&
			    (!xmlStrEqual(attr->name, BAD_CAST "form")))
			{
			    xmlSchemaPIllegalAttrErr(ctxt,
				XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
				NULL, (xmlSchemaTypePtr) decl, attr);
			}
		    } else if ((!xmlStrEqual(attr->name, BAD_CAST "final")) &&
			(!xmlStrEqual(attr->name, BAD_CAST "abstract")) &&
			(!xmlStrEqual(attr->name, BAD_CAST "substitutionGroup"))) {

			xmlSchemaPIllegalAttrErr(ctxt,
			    XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
			    NULL, (xmlSchemaTypePtr) decl, attr);
		    }
		}
	    } else if (xmlStrEqual(attr->ns->href, xmlSchemaNs)) {

		xmlSchemaPIllegalAttrErr(ctxt,
		    XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		    NULL, (xmlSchemaTypePtr) decl, attr);
	    }
	    attr = attr->next;
	}
	/*
	* Extract/validate attributes.
	*/
	if (topLevel) {
	    /*
	    * Process top attributes of global element declarations here.
	    */
	    decl->flags |= XML_SCHEMAS_ELEM_GLOBAL;
	    decl->flags |= XML_SCHEMAS_ELEM_TOPLEVEL;
	    xmlSchemaPValAttrQName(ctxt, schema, NULL,
		(xmlSchemaTypePtr) decl, node, "substitutionGroup",
		&(decl->substGroupNs), &(decl->substGroup));
	    if (xmlGetBooleanProp(ctxt, NULL, (xmlSchemaTypePtr) decl,
		node, "abstract", 0))
		decl->flags |= XML_SCHEMAS_ELEM_ABSTRACT;
	    /*
	    * Attribute "final".
	    */
	    attr = xmlSchemaGetPropNode(node, "final");
	    if (attr == NULL) {
		if (schema->flags & XML_SCHEMAS_FINAL_DEFAULT_EXTENSION)
		    decl->flags |= XML_SCHEMAS_ELEM_FINAL_EXTENSION;
		if (schema->flags & XML_SCHEMAS_FINAL_DEFAULT_RESTRICTION)
		    decl->flags |= XML_SCHEMAS_ELEM_FINAL_RESTRICTION;
	    } else {
		attrValue = xmlSchemaGetNodeContent(ctxt, (xmlNodePtr) attr);
		if (xmlSchemaPValAttrBlockFinal(attrValue, &(decl->flags),
		    -1,
		    XML_SCHEMAS_ELEM_FINAL_EXTENSION,
		    XML_SCHEMAS_ELEM_FINAL_RESTRICTION, -1, -1, -1) != 0) {
		    xmlSchemaPSimpleTypeErr(ctxt,
			XML_SCHEMAP_S4S_ATTR_INVALID_VALUE,
			(xmlSchemaTypePtr) decl, (xmlNodePtr) attr,
			NULL, "(#all | List of (extension | restriction))",
			attrValue, NULL, NULL, NULL);
		}
	    }
	}
	/*
	* Attribute "block".
	*/
	attr = xmlSchemaGetPropNode(node, "block");
	if (attr == NULL) {
	    /*
	    * Apply default "block" values.
	    */
	    if (schema->flags & XML_SCHEMAS_BLOCK_DEFAULT_RESTRICTION)
		decl->flags |= XML_SCHEMAS_ELEM_BLOCK_RESTRICTION;
	    if (schema->flags & XML_SCHEMAS_BLOCK_DEFAULT_EXTENSION)
		decl->flags |= XML_SCHEMAS_ELEM_BLOCK_EXTENSION;
	    if (schema->flags & XML_SCHEMAS_BLOCK_DEFAULT_SUBSTITUTION)
		decl->flags |= XML_SCHEMAS_ELEM_BLOCK_SUBSTITUTION;
	} else {
	    attrValue = xmlSchemaGetNodeContent(ctxt, (xmlNodePtr) attr);
	    if (xmlSchemaPValAttrBlockFinal(attrValue, &(decl->flags),
		-1,
		XML_SCHEMAS_ELEM_BLOCK_EXTENSION,
		XML_SCHEMAS_ELEM_BLOCK_RESTRICTION,
		XML_SCHEMAS_ELEM_BLOCK_SUBSTITUTION, -1, -1) != 0) {
		xmlSchemaPSimpleTypeErr(ctxt,
		    XML_SCHEMAP_S4S_ATTR_INVALID_VALUE,
		    (xmlSchemaTypePtr) decl, (xmlNodePtr) attr,
		    NULL, "(#all | List of (extension | "
		    "restriction | substitution))", attrValue,
		    NULL, NULL, NULL);
	    }
	}
	if (xmlGetBooleanProp(ctxt, NULL, (xmlSchemaTypePtr) decl,
	    node, "nillable", 0))
	    decl->flags |= XML_SCHEMAS_ELEM_NILLABLE;

	attr = xmlSchemaGetPropNode(node, "type");
	if (attr != NULL) {
	    xmlSchemaPValAttrNodeQName(ctxt, schema,
		NULL, (xmlSchemaTypePtr) decl, attr,
		&(decl->namedTypeNs), &(decl->namedType));
	    xmlSchemaCheckReference(ctxt, schema, node,
	    (xmlSchemaBasicItemPtr) decl, decl->namedTypeNs);
	}
	decl->value = xmlSchemaGetProp(ctxt, node, "default");
	attr = xmlSchemaGetPropNode(node, "fixed");
	if (attr != NULL) {
	    fixed = xmlSchemaGetNodeContent(ctxt, (xmlNodePtr) attr);
	    if (decl->value != NULL) {
		/*
		* 3.3.3 : 1
		* default and fixed must not both be present.
		*/
		xmlSchemaPMutualExclAttrErr(ctxt,
		    XML_SCHEMAP_SRC_ELEMENT_1,
		    NULL, (xmlSchemaTypePtr) decl, attr,
		    "default", "fixed");
	    } else {
		decl->flags |= XML_SCHEMAS_ELEM_FIXED;
		decl->value = fixed;
	    }
	}
	/*
	* And now for the children...
	*/
	oldcontainer = ctxt->container;
	ctxt->container = decl->name;
	if (IS_SCHEMA(child, "complexType")) {
	    /*
	    * 3.3.3 : 3
	    * "type" and either <simpleType> or <complexType> are mutually
	    * exclusive
	    */
	    if (decl->namedType != NULL) {
		xmlSchemaPContentErr(ctxt,
		    XML_SCHEMAP_SRC_ELEMENT_3,
		    NULL, (xmlSchemaTypePtr) decl, node, child,
		    "The attribute 'type' and the <complexType> child are "
		    "mutually exclusive", NULL);
	    } else
		ELEM_TYPE(decl) = xmlSchemaParseComplexType(ctxt, schema, child, 0);
	    child = child->next;
	} else if (IS_SCHEMA(child, "simpleType")) {
	    /*
	    * 3.3.3 : 3
	    * "type" and either <simpleType> or <complexType> are
	    * mutually exclusive
	    */
	    if (decl->namedType != NULL) {
		xmlSchemaPContentErr(ctxt,
		    XML_SCHEMAP_SRC_ELEMENT_3,
		    NULL, (xmlSchemaTypePtr) decl, node, child,
		    "The attribute 'type' and the <simpleType> child are "
		    "mutually exclusive", NULL);
	    } else
		ELEM_TYPE(decl) = xmlSchemaParseSimpleType(ctxt, schema, child, 0);
	    child = child->next;
	}
	while ((IS_SCHEMA(child, "unique")) ||
	    (IS_SCHEMA(child, "key")) || (IS_SCHEMA(child, "keyref"))) {
	    if (IS_SCHEMA(child, "unique")) {
		curIDC = xmlSchemaParseIDC(ctxt, schema, child,
		    XML_SCHEMA_TYPE_IDC_UNIQUE, decl->targetNamespace);
	    } else if (IS_SCHEMA(child, "key")) {
		curIDC = xmlSchemaParseIDC(ctxt, schema, child,
		    XML_SCHEMA_TYPE_IDC_KEY, decl->targetNamespace);
	    } else if (IS_SCHEMA(child, "keyref")) {
		curIDC = xmlSchemaParseIDC(ctxt, schema, child,
		    XML_SCHEMA_TYPE_IDC_KEYREF, decl->targetNamespace);
	    }
	    if (lastIDC != NULL)
		lastIDC->next = curIDC;
	    else
		decl->idcs = (void *) curIDC;
	    lastIDC = curIDC;
	    child = child->next;
	}
	if (child != NULL) {
	    xmlSchemaPContentErr(ctxt,
		XML_SCHEMAP_S4S_ELEM_NOT_ALLOWED,
		NULL, (xmlSchemaTypePtr) decl, node, child,
		NULL, "(annotation?, ((simpleType | complexType)?, "
		"(unique | key | keyref)*))");
	}
	ctxt->container = oldcontainer;
	decl->annot = annot;
    }
    /*
    * NOTE: Element Declaration Representation OK 4. will be checked at a
    * different layer.
    */
    FREE_AND_NULL(des)
    if (topLevel)
	return ((xmlSchemaBasicItemPtr) decl);
    else {
	particle->children = (xmlSchemaTreeItemPtr) decl;
	return ((xmlSchemaBasicItemPtr) particle);
    }

return_null:
    FREE_AND_NULL(des);
    if (annot != NULL) {
	if (particle != NULL)
	    particle->annot = NULL;
	if (decl != NULL)
	    decl->annot = NULL;
	xmlSchemaFreeAnnot(annot);
    }
    return (NULL);
}

/**
 * xmlSchemaParseUnion:
 * @ctxt:  a schema validation context
 * @schema:  the schema being built
 * @node:  a subtree containing XML Schema informations
 *
 * parse a XML schema Union definition
 * *WARNING* this interface is highly subject to change
 *
 * Returns -1 in case of internal error, 0 in case of success and a positive
 * error code otherwise.
 */
static int
xmlSchemaParseUnion(xmlSchemaParserCtxtPtr ctxt, xmlSchemaPtr schema,
                    xmlNodePtr node)
{
    xmlSchemaTypePtr type;
    xmlNodePtr child = NULL;
    xmlAttrPtr attr;
    const xmlChar *cur = NULL;

    if ((ctxt == NULL) || (schema == NULL) || (node == NULL))
        return (-1);
    /* Not a component, don't create it. */
    type = ctxt->ctxtType;
    /*
    * Mark the simple type as being of variety "union".
    */
    type->flags |= XML_SCHEMAS_TYPE_VARIETY_UNION;
    /*
    * SPEC (Base type) (2) "If the <list> or <union> alternative is chosen,
    * then the ·simple ur-type definition·."
    */
    type->baseType = xmlSchemaGetBuiltInType(XML_SCHEMAS_ANYSIMPLETYPE);
    /*
    * Check for illegal attributes.
    */
    attr = node->properties;
    while (attr != NULL) {
	if (attr->ns == NULL) {
	    if ((!xmlStrEqual(attr->name, BAD_CAST "id")) &&
		(!xmlStrEqual(attr->name, BAD_CAST "memberTypes"))) {
		xmlSchemaPIllegalAttrErr(ctxt,
		    XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		    NULL, NULL, attr);
	    }
	} else if (xmlStrEqual(attr->ns->href, xmlSchemaNs)) {
	    xmlSchemaPIllegalAttrErr(ctxt,
		XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		NULL, NULL, attr);
	}
	attr = attr->next;
    }
    xmlSchemaPValAttrID(ctxt, NULL, NULL, node, BAD_CAST "id");
    /*
    * Attribute "memberTypes". This is a list of QNames.
    * TODO: Check the value to contain anything.
    */
    attr = xmlSchemaGetPropNode(node, "memberTypes");
    if (attr != NULL) {
	const xmlChar *end;
	xmlChar *tmp;
	const xmlChar *localName, *nsName;
	xmlSchemaTypeLinkPtr link, lastLink = NULL;
	xmlSchemaQNameRefPtr ref;

	cur = xmlSchemaGetNodeContent(ctxt, (xmlNodePtr) attr);
	type->ref = cur;
	do {
	    while (IS_BLANK_CH(*cur))
		cur++;
	    end = cur;
	    while ((*end != 0) && (!(IS_BLANK_CH(*end))))
		end++;
	    if (end == cur)
		break;
	    tmp = xmlStrndup(cur, end - cur);
	    if (xmlSchemaPValAttrNodeQNameValue(ctxt, schema, NULL,
		NULL, attr, BAD_CAST tmp, &nsName, &localName) == 0) {
		/*
		* Create the member type link.
		*/
		link = (xmlSchemaTypeLinkPtr)
		    xmlMalloc(sizeof(xmlSchemaTypeLink));
		if (link == NULL) {
		    xmlSchemaPErrMemory(ctxt, "xmlSchemaParseUnion, "
			"allocating a type link", NULL);
		    return (-1);
		}
		link->type = NULL;
		link->next = NULL;
		if (lastLink == NULL)
		    type->memberTypes = link;
		else
		    lastLink->next = link;
		lastLink = link;
		/*
		* Create a reference item.
		*/
		ref = xmlSchemaNewQNameRef(schema, XML_SCHEMA_TYPE_SIMPLE,
		    localName, nsName);
		if (ref == NULL) {
		    FREE_AND_NULL(tmp)
		    return (-1);
		}
		/*
		* Assign the reference to the link, it will be resolved
		* later during fixup of the union simple type.
		*/
		link->type = (xmlSchemaTypePtr) ref;
	    }
	    FREE_AND_NULL(tmp)
	    cur = end;
	} while (*cur != 0);

    }
    /*
    * And now for the children...
    */
    child = node->children;
    if (IS_SCHEMA(child, "annotation")) {
	/*
	* Add the annotation to the simple type ancestor.
	*/
	xmlSchemaAddAnnotation((xmlSchemaAnnotItemPtr) type,
	    xmlSchemaParseAnnotation(ctxt, schema, child));
        child = child->next;
    }
    if (IS_SCHEMA(child, "simpleType")) {
	xmlSchemaTypePtr subtype, last = NULL;

	/*
	* Anchor the member types in the "subtypes" field of the
	* simple type.
	*/
	while (IS_SCHEMA(child, "simpleType")) {
	    subtype = (xmlSchemaTypePtr)
		xmlSchemaParseSimpleType(ctxt, schema, child, 0);
	    if (subtype != NULL) {
		if (last == NULL) {
		    type->subtypes = subtype;
		    last = subtype;
		} else {
		    last->next = subtype;
		    last = subtype;
		}
		last->next = NULL;
	    }
	    child = child->next;
	}
    }
    if (child != NULL) {
	xmlSchemaPContentErr(ctxt,
	    XML_SCHEMAP_S4S_ELEM_NOT_ALLOWED,
	    NULL, NULL, node, child, NULL, "(annotation?, simpleType*)");
    }
    if ((attr == NULL) && (type->subtypes == NULL)) {
	 /*
	* src-union-memberTypes-or-simpleTypes
	* Either the memberTypes [attribute] of the <union> element must
	* be non-empty or there must be at least one simpleType [child].
	*/
	xmlSchemaPCustomErr(ctxt,
	    XML_SCHEMAP_SRC_UNION_MEMBERTYPES_OR_SIMPLETYPES,
	    NULL, NULL, node,
	    "Either the attribute 'memberTypes' or "
	    "at least one <simpleType> child must be present", NULL);
    }
    return (0);
}

/**
 * xmlSchemaParseList:
 * @ctxt:  a schema validation context
 * @schema:  the schema being built
 * @node:  a subtree containing XML Schema informations
 *
 * parse a XML schema List definition
 * *WARNING* this interface is highly subject to change
 *
 * Returns -1 in case of error, 0 if the declaration is improper and
 *         1 in case of success.
 */
static xmlSchemaTypePtr
xmlSchemaParseList(xmlSchemaParserCtxtPtr ctxt, xmlSchemaPtr schema,
                   xmlNodePtr node)
{
    xmlSchemaTypePtr type;
    xmlNodePtr child = NULL;
    xmlAttrPtr attr;

    if ((ctxt == NULL) || (schema == NULL) || (node == NULL))
        return (NULL);
    /* Not a component, don't create it. */
    type = ctxt->ctxtType;
    /*
    * Mark the type as being of variety "list".
    */
    type->flags |= XML_SCHEMAS_TYPE_VARIETY_LIST;
    /*
    * SPEC (Base type) (2) "If the <list> or <union> alternative is chosen,
    * then the ·simple ur-type definition·."
    */
    type->baseType = xmlSchemaGetBuiltInType(XML_SCHEMAS_ANYSIMPLETYPE);
    /*
    * Check for illegal attributes.
    */
    attr = node->properties;
    while (attr != NULL) {
	if (attr->ns == NULL) {
	    if ((!xmlStrEqual(attr->name, BAD_CAST "id")) &&
		(!xmlStrEqual(attr->name, BAD_CAST "itemType"))) {
		xmlSchemaPIllegalAttrErr(ctxt,
		    XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		    NULL, NULL, attr);
	    }
	} else if (xmlStrEqual(attr->ns->href, xmlSchemaNs)) {
	    xmlSchemaPIllegalAttrErr(ctxt,
		XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		NULL, NULL, attr);
	}
	attr = attr->next;
    }

    xmlSchemaPValAttrID(ctxt, NULL, NULL, node, BAD_CAST "id");

    /*
    * Attribute "itemType". NOTE that we will use the "ref" and "refNs"
    * fields for holding the reference to the itemType.
    */
    xmlSchemaPValAttrQName(ctxt, schema, NULL, NULL,
	node, "itemType", &(type->refNs), &(type->ref));
    /*
    * And now for the children...
    */
    child = node->children;
    if (IS_SCHEMA(child, "annotation")) {
	xmlSchemaAddAnnotation((xmlSchemaAnnotItemPtr) type,
	    xmlSchemaParseAnnotation(ctxt, schema, child));
        child = child->next;
    }
    if (IS_SCHEMA(child, "simpleType")) {
	/*
	* src-list-itemType-or-simpleType
	* Either the itemType [attribute] or the <simpleType> [child] of
	* the <list> element must be present, but not both.
	*/
	if (type->ref != NULL) {
	    xmlSchemaPCustomErr(ctxt,
		XML_SCHEMAP_SRC_SIMPLE_TYPE_1,
		NULL, NULL, node,
		"The attribute 'itemType' and the <simpleType> child "
		"are mutually exclusive", NULL);
	} else {
	    type->subtypes = xmlSchemaParseSimpleType(ctxt, schema, child, 0);
	}
        child = child->next;
    } else if (type->ref == NULL) {
	xmlSchemaPCustomErr(ctxt,
	    XML_SCHEMAP_SRC_SIMPLE_TYPE_1,
	    NULL, NULL, node,
	    "Either the attribute 'itemType' or the <simpleType> child "
	    "must be present", NULL);
    }
    if (child != NULL) {
	xmlSchemaPContentErr(ctxt,
	    XML_SCHEMAP_S4S_ELEM_NOT_ALLOWED,
	    NULL, NULL, node, child, NULL, "(annotation?, simpleType?)");
    }
    if ((type->ref == NULL) &&
	(type->subtypes == NULL) &&
	(xmlSchemaGetPropNode(node, "itemType") == NULL)) {
	xmlSchemaPCustomErr(ctxt,
	    XML_SCHEMAP_SRC_SIMPLE_TYPE_1,
	    NULL, NULL, node,
	    "Either the attribute 'itemType' or the <simpleType> child "
	    "must be present", NULL);
    }
    return (NULL);
}

/**
 * xmlSchemaParseSimpleType:
 * @ctxt:  a schema validation context
 * @schema:  the schema being built
 * @node:  a subtree containing XML Schema informations
 *
 * parse a XML schema Simple Type definition
 * *WARNING* this interface is highly subject to change
 *
 * Returns -1 in case of error, 0 if the declaration is improper and
 * 1 in case of success.
 */
static xmlSchemaTypePtr
xmlSchemaParseSimpleType(xmlSchemaParserCtxtPtr ctxt, xmlSchemaPtr schema,
                         xmlNodePtr node, int topLevel)
{
    xmlSchemaTypePtr type, oldCtxtType, oldParentItem;
    xmlNodePtr child = NULL;
    const xmlChar *attrValue = NULL;
    xmlAttrPtr attr;

    if ((ctxt == NULL) || (schema == NULL) || (node == NULL))
        return (NULL);

    if (topLevel) {
	attr = xmlSchemaGetPropNode(node, "name");
	if (attr == NULL) {
	    xmlSchemaPMissingAttrErr(ctxt,
		XML_SCHEMAP_S4S_ATTR_MISSING,
		NULL, node,
		"name", NULL);
	    return (NULL);
	} else {
	    if (xmlSchemaPValAttrNode(ctxt,
		NULL, NULL, attr,
		xmlSchemaGetBuiltInType(XML_SCHEMAS_NCNAME), &attrValue) != 0)
		return (NULL);
	    /*
	    * Skip built-in types.
	    */
	    if (ctxt->isS4S) {
		xmlSchemaTypePtr biType;

		biType = xmlSchemaGetPredefinedType(attrValue, xmlSchemaNs);
		if (biType != NULL)
		    return (biType);
	    }
	}
    }

    if (topLevel == 0) {
        char buf[40];

	/*
	* Parse as local simple type definition.
	*/
        snprintf(buf, 39, "#ST%d", ctxt->counter++ + 1);
	type = xmlSchemaAddType(ctxt, schema, (const xmlChar *)buf, NULL, node);
	if (type == NULL)
	    return (NULL);
	type->node = node;
	type->type = XML_SCHEMA_TYPE_SIMPLE;
	type->contentType = XML_SCHEMA_CONTENT_SIMPLE;
	/*
	* Check for illegal attributes.
	*/
	attr = node->properties;
	while (attr != NULL) {
	    if (attr->ns == NULL) {
		if (!xmlStrEqual(attr->name, BAD_CAST "id")) {
		    xmlSchemaPIllegalAttrErr(ctxt,
			XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
			NULL, type, attr);
		}
	    } else if (xmlStrEqual(attr->ns->href, xmlSchemaNs)) {
		    xmlSchemaPIllegalAttrErr(ctxt,
			XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
			NULL, type, attr);
	    }
	    attr = attr->next;
	}
    } else {
	/*
	* Parse as global simple type definition.
	*
	* Note that attrValue is the value of the attribute "name" here.
	*/
	type = xmlSchemaAddType(ctxt, schema, attrValue, schema->targetNamespace, node);
	if (type == NULL)
	    return (NULL);
	type->node = node;
	type->type = XML_SCHEMA_TYPE_SIMPLE;
	type->contentType = XML_SCHEMA_CONTENT_SIMPLE;
	type->flags |= XML_SCHEMAS_TYPE_GLOBAL;
	/*
	* Check for illegal attributes.
	*/
	attr = node->properties;
	while (attr != NULL) {
	    if (attr->ns == NULL) {
		if ((!xmlStrEqual(attr->name, BAD_CAST "id")) &&
		    (!xmlStrEqual(attr->name, BAD_CAST "name")) &&
		    (!xmlStrEqual(attr->name, BAD_CAST "final"))) {
		    xmlSchemaPIllegalAttrErr(ctxt,
			XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
			NULL, type, attr);
		}
	    } else if (xmlStrEqual(attr->ns->href, xmlSchemaNs)) {
		xmlSchemaPIllegalAttrErr(ctxt,
		    XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		    NULL, type, attr);
	    }
	    attr = attr->next;
	}
	/*
	* Attribute "final".
	*/
	attr = xmlSchemaGetPropNode(node, "final");
	if (attr == NULL) {
	    if (schema->flags & XML_SCHEMAS_FINAL_DEFAULT_RESTRICTION)
		type->flags |= XML_SCHEMAS_TYPE_FINAL_RESTRICTION;
	    if (schema->flags & XML_SCHEMAS_FINAL_DEFAULT_LIST)
		type->flags |= XML_SCHEMAS_TYPE_FINAL_LIST;
	    if (schema->flags & XML_SCHEMAS_FINAL_DEFAULT_UNION)
		type->flags |= XML_SCHEMAS_TYPE_FINAL_UNION;
	} else {
	    attrValue = xmlSchemaGetProp(ctxt, node, "final");
	    if (xmlSchemaPValAttrBlockFinal(attrValue, &(type->flags),
		-1, -1, XML_SCHEMAS_TYPE_FINAL_RESTRICTION, -1,
		XML_SCHEMAS_TYPE_FINAL_LIST,
		XML_SCHEMAS_TYPE_FINAL_UNION) != 0) {

		xmlSchemaPSimpleTypeErr(ctxt,
		    XML_SCHEMAP_S4S_ATTR_INVALID_VALUE,
		    type, (xmlNodePtr) attr,
		    NULL, "(#all | List of (list | union | restriction)",
		    attrValue, NULL, NULL, NULL);
	    }
	}
    }
    type->targetNamespace = schema->targetNamespace;
    xmlSchemaPValAttrID(ctxt, NULL, type, node, BAD_CAST "id");
    /*
    * And now for the children...
    */
    oldCtxtType = ctxt->ctxtType;
    oldParentItem = ctxt->parentItem;
    ctxt->ctxtType = type;
    ctxt->parentItem = type;
    child = node->children;
    if (IS_SCHEMA(child, "annotation")) {
        type->annot = xmlSchemaParseAnnotation(ctxt, schema, child);
        child = child->next;
    }
    if (child == NULL) {
	xmlSchemaPContentErr(ctxt, XML_SCHEMAP_S4S_ELEM_MISSING,
	    NULL, type, node, child, NULL,
	    "(annotation?, (restriction | list | union))");
    } else if (IS_SCHEMA(child, "restriction")) {
        xmlSchemaParseRestriction(ctxt, schema, child,
		XML_SCHEMA_TYPE_SIMPLE);
        child = child->next;
    } else if (IS_SCHEMA(child, "list")) {
        xmlSchemaParseList(ctxt, schema, child);
        child = child->next;
    } else if (IS_SCHEMA(child, "union")) {
        xmlSchemaParseUnion(ctxt, schema, child);
        child = child->next;
    }
    if (child != NULL) {
	xmlSchemaPContentErr(ctxt, XML_SCHEMAP_S4S_ELEM_NOT_ALLOWED,
	    NULL, type, node, child, NULL,
	    "(annotation?, (restriction | list | union))");
    }
    ctxt->parentItem = oldParentItem;
    ctxt->ctxtType = oldCtxtType;

    return (type);
}

/**
 * xmlSchemaParseModelGroupDefRef:
 * @ctxt:  a schema validation context
 * @schema:  the schema being built
 * @node:  a subtree containing XML Schema informations
 *
 * Parses a XML schema particle (reference to a model group definition).
 * *WARNING* this interface is highly subject to change
 *
 * Returns -1 in case of error, 0 if the declaration is improper and
 *         1 in case of success.
 */
static xmlSchemaTreeItemPtr
xmlSchemaParseModelGroupDefRef(xmlSchemaParserCtxtPtr ctxt,
			       xmlSchemaPtr schema,
			       xmlNodePtr node)
{
    xmlSchemaParticlePtr item;
    xmlNodePtr child = NULL;
    xmlAttrPtr attr;
    const xmlChar *ref = NULL, *refNs = NULL;
    int min, max;

    if ((ctxt == NULL) || (schema == NULL) || (node == NULL))
        return (NULL);

    attr = xmlSchemaGetPropNode(node, "ref");
    if (attr == NULL) {
	xmlSchemaPMissingAttrErr(ctxt,
	    XML_SCHEMAP_S4S_ATTR_MISSING,
	    NULL, node,
	    "ref", NULL);
	return (NULL);
    } else if (xmlSchemaPValAttrNodeQName(ctxt, schema, NULL, NULL,
	attr, &refNs, &ref) != 0) {
	return (NULL);
    }
    min = xmlGetMinOccurs(ctxt, node, 0, -1, 1, "xs:nonNegativeInteger");
    max = xmlGetMaxOccurs(ctxt, node, 0, UNBOUNDED, 1,
	"(xs:nonNegativeInteger | unbounded)");
    /*
    * Check for illegal attributes.
    */
    attr = node->properties;
    while (attr != NULL) {
	if (attr->ns == NULL) {
	    if ((!xmlStrEqual(attr->name, BAD_CAST "ref")) &&
		(!xmlStrEqual(attr->name, BAD_CAST "id")) &&
		(!xmlStrEqual(attr->name, BAD_CAST "minOccurs")) &&
		(!xmlStrEqual(attr->name, BAD_CAST "maxOccurs"))) {
		xmlSchemaPIllegalAttrErr(ctxt,
		    XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		    NULL, NULL, attr);
	    }
	} else if (xmlStrEqual(attr->ns->href, xmlSchemaNs)) {
	    xmlSchemaPIllegalAttrErr(ctxt,
		XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		NULL, NULL, attr);
	}
	attr = attr->next;
    }
    xmlSchemaPValAttrID(ctxt, NULL, NULL, node, BAD_CAST "id");
    item = xmlSchemaAddParticle(ctxt, schema, node, min, max);
    if (item == NULL)
	return (NULL);
    /*
    * Create a reference item as the term; it will be substituted for
    * the model group after the reference has been resolved.
    */
    item->children = (xmlSchemaTreeItemPtr)
	xmlSchemaNewQNameRef(schema, XML_SCHEMA_TYPE_GROUP, ref, refNs);
    xmlSchemaCheckReference(ctxt, schema, node, (xmlSchemaBasicItemPtr) item, refNs);
    xmlSchemaPCheckParticleCorrect_2(ctxt, item, node, min, max);
    /*
    * And now for the children...
    */
    child = node->children;
    /* TODO: Is annotation even allowed for a model group reference? */
    if (IS_SCHEMA(child, "annotation")) {
	/*
	* TODO: What to do exactly with the annotation?
	*/
	item->annot = xmlSchemaParseAnnotation(ctxt, schema, child);
	child = child->next;
    }
    if (child != NULL) {
	xmlSchemaPContentErr(ctxt,
	    XML_SCHEMAP_S4S_ELEM_NOT_ALLOWED,
	    NULL, NULL, node, child, NULL,
	    "(annotation?)");
    }
    /*
    * Corresponds to no component at all if minOccurs==maxOccurs==0.
    */
    if ((min == 0) && (max == 0))
	return (NULL);
    if (ctxt->assemble != NULL)
	xmlSchemaAddAssembledItem(ctxt, (xmlSchemaTypePtr) item);
    return ((xmlSchemaTreeItemPtr) item);
}

/**
 * xmlSchemaParseModelGroupDefinition:
 * @ctxt:  a schema validation context
 * @schema:  the schema being built
 * @node:  a subtree containing XML Schema informations
 *
 * Parses a XML schema model group definition.
 * *WARNING* this interface is highly subject to change
 *
 * Returns -1 in case of error, 0 if the declaration is improper and
 *         1 in case of success.
 */
static xmlSchemaModelGroupDefPtr
xmlSchemaParseModelGroupDefinition(xmlSchemaParserCtxtPtr ctxt,
				   xmlSchemaPtr schema,
				   xmlNodePtr node)
{
    xmlSchemaModelGroupDefPtr item;
    xmlNodePtr child = NULL;
    xmlAttrPtr attr;
    const xmlChar *name;

    if ((ctxt == NULL) || (schema == NULL) || (node == NULL))
        return (NULL);

    attr = xmlSchemaGetPropNode(node, "name");
    if (attr == NULL) {
	xmlSchemaPMissingAttrErr(ctxt,
	    XML_SCHEMAP_S4S_ATTR_MISSING,
	    NULL, node,
	    "name", NULL);
	return (NULL);
    } else if (xmlSchemaPValAttrNode(ctxt,
	NULL, NULL, attr,
	xmlSchemaGetBuiltInType(XML_SCHEMAS_NCNAME), &name) != 0) {
	return (NULL);
    }
    item = xmlSchemaAddGroup(ctxt, schema, name, schema->targetNamespace, node);
    if (item == NULL)
	return (NULL);
    /*
    * Check for illegal attributes.
    */
    attr = node->properties;
    while (attr != NULL) {
	if (attr->ns == NULL) {
	    if ((!xmlStrEqual(attr->name, BAD_CAST "name")) &&
		(!xmlStrEqual(attr->name, BAD_CAST "id"))) {
		xmlSchemaPIllegalAttrErr(ctxt,
		    XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		    NULL, NULL, attr);
	    }
	} else if (xmlStrEqual(attr->ns->href, xmlSchemaNs)) {
	    xmlSchemaPIllegalAttrErr(ctxt,
		XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		NULL, NULL, attr);
	}
	attr = attr->next;
    }
    xmlSchemaPValAttrID(ctxt, NULL, NULL, node, BAD_CAST "id");
    /*
    * And now for the children...
    */
    child = node->children;
    if (IS_SCHEMA(child, "annotation")) {
	item->annot = xmlSchemaParseAnnotation(ctxt, schema, child);
	child = child->next;
    }
    if (IS_SCHEMA(child, "all")) {
	item->children = xmlSchemaParseModelGroup(ctxt, schema, child,
	    XML_SCHEMA_TYPE_ALL, 0);
	child = child->next;
    } else if (IS_SCHEMA(child, "choice")) {
	item->children = xmlSchemaParseModelGroup(ctxt, schema, child,
	    XML_SCHEMA_TYPE_CHOICE, 0);
	child = child->next;
    } else if (IS_SCHEMA(child, "sequence")) {
	item->children = xmlSchemaParseModelGroup(ctxt, schema, child,
	    XML_SCHEMA_TYPE_SEQUENCE, 0);
	child = child->next;
    }
    if (child != NULL) {
	xmlSchemaPContentErr(ctxt,
	    XML_SCHEMAP_S4S_ELEM_NOT_ALLOWED,
	    NULL, NULL, node, child, NULL,
	    "(annotation?, (all | choice | sequence)?)");
    }

    return (item);
}

/**
 * xmlSchemaCleanupDoc:
 * @ctxt:  a schema validation context
 * @node:  the root of the document.
 *
 * removes unwanted nodes in a schemas document tree
 */
static void
xmlSchemaCleanupDoc(xmlSchemaParserCtxtPtr ctxt, xmlNodePtr root)
{
    xmlNodePtr delete, cur;

    if ((ctxt == NULL) || (root == NULL)) return;

    /*
     * Remove all the blank text nodes
     */
    delete = NULL;
    cur = root;
    while (cur != NULL) {
        if (delete != NULL) {
            xmlUnlinkNode(delete);
            xmlFreeNode(delete);
            delete = NULL;
        }
        if (cur->type == XML_TEXT_NODE) {
            if (IS_BLANK_NODE(cur)) {
                if (xmlNodeGetSpacePreserve(cur) != 1) {
                    delete = cur;
                }
            }
        } else if ((cur->type != XML_ELEMENT_NODE) &&
                   (cur->type != XML_CDATA_SECTION_NODE)) {
            delete = cur;
            goto skip_children;
        }

        /*
         * Skip to next node
         */
        if (cur->children != NULL) {
            if ((cur->children->type != XML_ENTITY_DECL) &&
                (cur->children->type != XML_ENTITY_REF_NODE) &&
                (cur->children->type != XML_ENTITY_NODE)) {
                cur = cur->children;
                continue;
            }
        }
      skip_children:
        if (cur->next != NULL) {
            cur = cur->next;
            continue;
        }

        do {
            cur = cur->parent;
            if (cur == NULL)
                break;
            if (cur == root) {
                cur = NULL;
                break;
            }
            if (cur->next != NULL) {
                cur = cur->next;
                break;
            }
        } while (cur != NULL);
    }
    if (delete != NULL) {
        xmlUnlinkNode(delete);
        xmlFreeNode(delete);
        delete = NULL;
    }
}


/**
 * xmlSchemaImportSchema
 *
 * @ctxt:  a schema validation context
 * @schemaLocation:  an URI defining where to find the imported schema
 *
 * import a XML schema
 * *WARNING* this interface is highly subject to change
 *
 * Returns -1 in case of error and 1 in case of success.
 */
#if 0
static xmlSchemaImportPtr
xmlSchemaImportSchema(xmlSchemaParserCtxtPtr ctxt,
                      const xmlChar *schemaLocation)
{
    xmlSchemaImportPtr import;
    xmlSchemaParserCtxtPtr newctxt;

    newctxt = (xmlSchemaParserCtxtPtr) xmlMalloc(sizeof(xmlSchemaParserCtxt));
    if (newctxt == NULL) {
        xmlSchemaPErrMemory(ctxt, "allocating schema parser context",
                            NULL);
        return (NULL);
    }
    memset(newctxt, 0, sizeof(xmlSchemaParserCtxt));
    /* Keep the same dictionnary for parsing, really */
    xmlDictReference(ctxt->dict);
    newctxt->dict = ctxt->dict;
    newctxt->includes = 0;
    newctxt->URL = xmlDictLookup(newctxt->dict, schemaLocation, -1);

    xmlSchemaSetParserErrors(newctxt, ctxt->error, ctxt->warning,
	                     ctxt->userData);

    import = (xmlSchemaImport*) xmlMalloc(sizeof(xmlSchemaImport));
    if (import == NULL) {
        xmlSchemaPErrMemory(NULL, "allocating imported schema",
                            NULL);
	xmlSchemaFreeParserCtxt(newctxt);
        return (NULL);
    }

    memset(import, 0, sizeof(xmlSchemaImport));
    import->schemaLocation = xmlDictLookup(ctxt->dict, schemaLocation, -1);
    import->schema = xmlSchemaParse(newctxt);

    if (import->schema == NULL) {
        /* FIXME use another error enum here ? */
        xmlSchemaPErr(ctxt, NULL, XML_SCHEMAP_INTERNAL,
	              "Failed to import schema from location \"%s\".\n",
		      schemaLocation, NULL);

	xmlSchemaFreeParserCtxt(newctxt);
	/* The schemaLocation is held by the dictionary.
	if (import->schemaLocation != NULL)
	    xmlFree((xmlChar *)import->schemaLocation);
	*/
	xmlFree(import);
	return NULL;
    }

    xmlSchemaFreeParserCtxt(newctxt);
    return import;
}
#endif

static void
xmlSchemaClearSchemaDefaults(xmlSchemaPtr schema)
{
    if (schema->flags & XML_SCHEMAS_QUALIF_ELEM)
	schema->flags ^= XML_SCHEMAS_QUALIF_ELEM;

    if (schema->flags & XML_SCHEMAS_QUALIF_ATTR)
	schema->flags ^= XML_SCHEMAS_QUALIF_ATTR;

    if (schema->flags & XML_SCHEMAS_FINAL_DEFAULT_EXTENSION)
	schema->flags ^= XML_SCHEMAS_FINAL_DEFAULT_EXTENSION;
    if (schema->flags & XML_SCHEMAS_FINAL_DEFAULT_RESTRICTION)
	schema->flags ^= XML_SCHEMAS_FINAL_DEFAULT_RESTRICTION;
    if (schema->flags & XML_SCHEMAS_FINAL_DEFAULT_LIST)
	schema->flags ^= XML_SCHEMAS_FINAL_DEFAULT_LIST;
    if (schema->flags & XML_SCHEMAS_FINAL_DEFAULT_UNION)
	schema->flags ^= XML_SCHEMAS_FINAL_DEFAULT_UNION;

    if (schema->flags & XML_SCHEMAS_BLOCK_DEFAULT_EXTENSION)
	schema->flags ^= XML_SCHEMAS_BLOCK_DEFAULT_EXTENSION;
    if (schema->flags & XML_SCHEMAS_BLOCK_DEFAULT_RESTRICTION)
	schema->flags ^= XML_SCHEMAS_BLOCK_DEFAULT_RESTRICTION;
    if (schema->flags & XML_SCHEMAS_BLOCK_DEFAULT_SUBSTITUTION)
	schema->flags ^= XML_SCHEMAS_BLOCK_DEFAULT_SUBSTITUTION;
}

static void
xmlSchemaParseSchemaDefaults(xmlSchemaParserCtxtPtr ctxt,
			     xmlSchemaPtr schema,
			     xmlNodePtr node)
{
    xmlAttrPtr attr;
    const xmlChar *val;

    xmlSchemaPValAttrID(ctxt, NULL, NULL, node, BAD_CAST "id");
    if (schema->version == NULL)
	xmlSchemaPValAttr(ctxt, NULL, NULL, node, "version",
	    xmlSchemaGetBuiltInType(XML_SCHEMAS_TOKEN), &(schema->version));
    else
	xmlSchemaPValAttr(ctxt, NULL, NULL, node, "version",
	    xmlSchemaGetBuiltInType(XML_SCHEMAS_TOKEN), NULL);

    attr = xmlSchemaGetPropNode(node, "elementFormDefault");
    if (attr != NULL) {
	val = xmlSchemaGetNodeContent(ctxt, (xmlNodePtr) attr);
	if (xmlSchemaPValAttrFormDefault(val, &schema->flags,
	    XML_SCHEMAS_QUALIF_ELEM) != 0) {
	    xmlSchemaPSimpleTypeErr(ctxt,
		XML_SCHEMAP_ELEMFORMDEFAULT_VALUE,
		NULL, (xmlNodePtr) attr, NULL,
		"(qualified | unqualified)", val, NULL, NULL, NULL);
	}
    }

    attr = xmlSchemaGetPropNode(node, "attributeFormDefault");
    if (attr != NULL) {
	val = xmlSchemaGetNodeContent(ctxt, (xmlNodePtr) attr);
	if (xmlSchemaPValAttrFormDefault(val, &schema->flags,
	    XML_SCHEMAS_QUALIF_ATTR) != 0) {
	    xmlSchemaPSimpleTypeErr(ctxt,
		XML_SCHEMAP_ATTRFORMDEFAULT_VALUE,
		NULL, (xmlNodePtr) attr, NULL,
		"(qualified | unqualified)", val, NULL, NULL, NULL);
	}
    }

    attr = xmlSchemaGetPropNode(node, "finalDefault");
    if (attr != NULL) {
	val = xmlSchemaGetNodeContent(ctxt, (xmlNodePtr) attr);
	if (xmlSchemaPValAttrBlockFinal(val, &(schema->flags), -1,
	    XML_SCHEMAS_FINAL_DEFAULT_EXTENSION,
	    XML_SCHEMAS_FINAL_DEFAULT_RESTRICTION,
	    -1,
	    XML_SCHEMAS_FINAL_DEFAULT_LIST,
	    XML_SCHEMAS_FINAL_DEFAULT_UNION) != 0) {
	    xmlSchemaPSimpleTypeErr(ctxt,
		XML_SCHEMAP_S4S_ATTR_INVALID_VALUE,
		NULL, (xmlNodePtr) attr, NULL,
		"(#all | List of (extension | restriction | list | union))",
		val, NULL, NULL, NULL);
	}
    }

    attr = xmlSchemaGetPropNode(node, "blockDefault");
    if (attr != NULL) {
	val = xmlSchemaGetNodeContent(ctxt, (xmlNodePtr) attr);
	if (xmlSchemaPValAttrBlockFinal(val, &(schema->flags), -1,
	    XML_SCHEMAS_BLOCK_DEFAULT_EXTENSION,
	    XML_SCHEMAS_BLOCK_DEFAULT_RESTRICTION,
	    XML_SCHEMAS_BLOCK_DEFAULT_SUBSTITUTION, -1, -1) != 0) {
	     xmlSchemaPSimpleTypeErr(ctxt,
		XML_SCHEMAP_S4S_ATTR_INVALID_VALUE,
		NULL, (xmlNodePtr) attr, NULL,
		"(#all | List of (extension | restriction | substitution))",
		val, NULL, NULL, NULL);
	}
    }
}

/**
 * xmlSchemaParseSchemaTopLevel:
 * @ctxt:  a schema validation context
 * @schema:  the schemas
 * @nodes:  the list of top level nodes
 *
 * Returns the internal XML Schema structure built from the resource or
 *         NULL in case of error
 */
static void
xmlSchemaParseSchemaTopLevel(xmlSchemaParserCtxtPtr ctxt,
                             xmlSchemaPtr schema, xmlNodePtr nodes)
{
    xmlNodePtr child;
    xmlSchemaAnnotPtr annot;

    if ((ctxt == NULL) || (schema == NULL) || (nodes == NULL))
        return;

    child = nodes;
    while ((IS_SCHEMA(child, "include")) ||
	   (IS_SCHEMA(child, "import")) ||
	   (IS_SCHEMA(child, "redefine")) ||
	   (IS_SCHEMA(child, "annotation"))) {
	if (IS_SCHEMA(child, "annotation")) {
	    annot = xmlSchemaParseAnnotation(ctxt, schema, child);
	    if (schema->annot == NULL)
		schema->annot = annot;
	    else
		xmlSchemaFreeAnnot(annot);
	} else if (IS_SCHEMA(child, "import")) {
	    xmlSchemaParseImport(ctxt, schema, child);
	} else if (IS_SCHEMA(child, "include")) {
	    ctxt->includes++;
	    xmlSchemaParseInclude(ctxt, schema, child);
	    ctxt->includes--;
	} else if (IS_SCHEMA(child, "redefine")) {
	    TODO
	}
	child = child->next;
    }
    while (child != NULL) {
	if (IS_SCHEMA(child, "complexType")) {
	    xmlSchemaParseComplexType(ctxt, schema, child, 1);
	    child = child->next;
	} else if (IS_SCHEMA(child, "simpleType")) {
	    xmlSchemaParseSimpleType(ctxt, schema, child, 1);
	    child = child->next;
	} else if (IS_SCHEMA(child, "element")) {
	    xmlSchemaParseElement(ctxt, schema, child, 1);
	    child = child->next;
	} else if (IS_SCHEMA(child, "attribute")) {
	    xmlSchemaParseAttribute(ctxt, schema, child, 1);
	    child = child->next;
	} else if (IS_SCHEMA(child, "attributeGroup")) {
	    xmlSchemaParseAttributeGroup(ctxt, schema, child, 1);
	    child = child->next;
	} else if (IS_SCHEMA(child, "group")) {
	    xmlSchemaParseModelGroupDefinition(ctxt, schema, child);
	    child = child->next;
	} else if (IS_SCHEMA(child, "notation")) {
	    xmlSchemaParseNotation(ctxt, schema, child);
	    child = child->next;
	} else {
	    xmlSchemaPErr2(ctxt, NULL, child,
			   XML_SCHEMAP_UNKNOWN_SCHEMAS_CHILD,
			   "Unexpected element \"%s\" as child of <schema>.\n",
			   child->name, NULL);
	    child = child->next;
	}
	while (IS_SCHEMA(child, "annotation")) {
	    annot = xmlSchemaParseAnnotation(ctxt, schema, child);
	    if (schema->annot == NULL)
		schema->annot = annot;
	    else
		xmlSchemaFreeAnnot(annot);
	    child = child->next;
	}
    }
    ctxt->parentItem = NULL;
    ctxt->ctxtType = NULL;
}

static xmlSchemaImportPtr
xmlSchemaAddImport(xmlSchemaParserCtxtPtr ctxt,
		   xmlHashTablePtr *imports,
		   const xmlChar *nsName)
{
    xmlSchemaImportPtr ret;

    if (*imports == NULL) {
	*imports = xmlHashCreateDict(10, ctxt->dict);
	if (*imports == NULL) {
	    xmlSchemaPCustomErr(ctxt,
		XML_SCHEMAP_FAILED_BUILD_IMPORT,
		NULL, NULL, (xmlNodePtr) ctxt->doc,
		"Internal error: failed to build the import table",
		NULL);
	    return (NULL);
	}
    }
    ret = (xmlSchemaImport*) xmlMalloc(sizeof(xmlSchemaImport));
    if (ret == NULL) {
	xmlSchemaPErrMemory(NULL, "allocating import struct", NULL);
	return (NULL);
    }
    memset(ret, 0, sizeof(xmlSchemaImport));
    if (nsName == NULL)
	nsName = XML_SCHEMAS_NO_NAMESPACE;
    xmlHashAddEntry(*imports, nsName, ret);

    return (ret);
}

/**
 * xmlSchemaNewParserCtxtUseDict:
 * @URL:  the location of the schema
 * @dict: the dictionary to be used
 *
 * Create an XML Schemas parse context for that file/resource expected
 * to contain an XML Schemas file.
 *
 * Returns the parser context or NULL in case of error
 */
static xmlSchemaParserCtxtPtr
xmlSchemaNewParserCtxtUseDict(const char *URL, xmlDictPtr dict)
{
    xmlSchemaParserCtxtPtr ret;
    /*
    if (URL == NULL)
        return (NULL);
	*/

    ret = (xmlSchemaParserCtxtPtr) xmlMalloc(sizeof(xmlSchemaParserCtxt));
    if (ret == NULL) {
        xmlSchemaPErrMemory(NULL, "allocating schema parser context",
                            NULL);
        return (NULL);
    }
    memset(ret, 0, sizeof(xmlSchemaParserCtxt));
    ret->dict = dict;
    xmlDictReference(dict);    
    if (URL != NULL)
	ret->URL = xmlDictLookup(dict, (const xmlChar *) URL, -1);
    ret->includes = 0;
    return (ret);
}

static int
xmlSchemaCreatePCtxtOnVCtxt(xmlSchemaValidCtxtPtr vctxt)
{
    if (vctxt->pctxt == NULL) {
        if (vctxt->schema != NULL)
	    vctxt->pctxt = xmlSchemaNewParserCtxtUseDict("*", vctxt->schema->dict);
	else
	    vctxt->pctxt = xmlSchemaNewParserCtxt("*");
	if (vctxt->pctxt == NULL) {
	    VERROR_INT("xmlSchemaCreatePCtxtOnVCtxt",
		"failed to create a temp. parser context");
	    return (-1);
	}
	/* TODO: Pass user data. */
	xmlSchemaSetParserErrors(vctxt->pctxt, vctxt->error, vctxt->warning, NULL);	
    }
    return (0);
}

static int
xmlSchemaAcquireSchemaDoc(xmlSchemaAbstractCtxtPtr actxt,
			  xmlSchemaPtr schema,
			  xmlNodePtr node,
			  const xmlChar *nsName,
			  const xmlChar *location,
			  xmlDocPtr *doc,
			  const xmlChar **targetNamespace,
			  int absolute)
{
    xmlSchemaParserCtxtPtr pctxt;
    xmlParserCtxtPtr parserCtxt;
    xmlSchemaImportPtr import;
    const xmlChar *ns;
    xmlNodePtr root;

    /*
    * NOTE: This will be used for <import>, <xsi:schemaLocation> and
    * <xsi:noNamespaceSchemaLocation>.
    */
    *doc = NULL;
    /*
    * Given that the schemaLocation [attribute] is only a hint, it is open
    * to applications to ignore all but the first <import> for a given
    * namespace, regardless of the ·actual value· of schemaLocation, but
    * such a strategy risks missing useful information when new
    * schemaLocations are offered.
    *
    * XSV (ver 2.5-2) does use the first <import> which resolves to a valid schema.
    * Xerces-J (ver 2.5.1) ignores all but the first given <import> - regardless if
    * valid or not.
    * We will follow XSV here.
    */
    if (location == NULL) {
	/*
	* Schema Document Location Strategy:
	*
	* 3 Based on the namespace name, identify an existing schema document,
	* either as a resource which is an XML document or a <schema> element
	* information item, in some local schema repository;
	*
	* 5 Attempt to resolve the namespace name to locate such a resource.
	*
	* NOTE: Those stategies are not supported, so we will skip.
	*/
	return (0);
    }
    if (nsName == NULL)
	ns = XML_SCHEMAS_NO_NAMESPACE;
    else
	ns = nsName;

    import = xmlHashLookup(schema->schemasImports, ns);
    if (import != NULL) {
	/*
	* There was a valid resource for the specified namespace already
	* defined, so skip.
	* TODO: This might be changed someday to allow import of
	* components from multiple documents for a single target namespace.
	*/
	return (0);
    }
    if (actxt->type == XML_SCHEMA_CTXT_PARSER)
	pctxt = (xmlSchemaParserCtxtPtr) actxt;
    else {
	xmlSchemaCreatePCtxtOnVCtxt((xmlSchemaValidCtxtPtr) actxt);
	pctxt = ((xmlSchemaValidCtxtPtr) actxt)->pctxt;
    }
    /*
    * Schema Document Location Strategy:
    *
    * 2 Based on the location URI, identify an existing schema document,
    * either as a resource which is an XML document or a <schema> element
    * information item, in some local schema repository;
    *
    * 4 Attempt to resolve the location URI, to locate a resource on the
    * web which is or contains or references a <schema> element;
    * TODO: Hmm, I don't know if the reference stuff in 4. will work.
    *
    */
    if ((absolute == 0) && (node != NULL)) {
	xmlChar *base, *URI;

	base = xmlNodeGetBase(node->doc, node);
	if (base == NULL) {
	    URI = xmlBuildURI(location, node->doc->URL);
	} else {
	    URI = xmlBuildURI(location, base);
	    xmlFree(base);
	}
	if (URI != NULL) {
	    location = xmlDictLookup(pctxt->dict, URI, -1);
	    xmlFree(URI);
	}
    }
    parserCtxt = xmlNewParserCtxt();
    if (parserCtxt == NULL) {
	xmlSchemaPErrMemory(NULL, "xmlSchemaParseImport: "
	    "allocating a parser context", NULL);
	return(-1);
    }
    if ((pctxt->dict != NULL) && (parserCtxt->dict != NULL)) {
	xmlDictFree(parserCtxt->dict);
	parserCtxt->dict = pctxt->dict;
	xmlDictReference(parserCtxt->dict);
    }
    *doc = xmlCtxtReadFile(parserCtxt, (const char *) location,
	    NULL, SCHEMAS_PARSE_OPTIONS);
    /*
    * 2.1 The referent is (a fragment of) a resource which is an
    * XML document (see clause 1.1), which in turn corresponds to
    * a <schema> element information item in a well-formed information
    * set, which in turn corresponds to a valid schema.
    * TODO: What to do with the "fragment" stuff?
    *
    * 2.2 The referent is a <schema> element information item in
    * a well-formed information set, which in turn corresponds
    * to a valid schema.
    * NOTE: 2.2 won't apply, since only XML documents will be processed
    * here.
    */
    if (*doc == NULL) {
	xmlErrorPtr lerr;
	/*
	* It is *not* an error for the application schema reference
	* strategy to fail.
	*
	* If the doc is NULL and the parser error is an IO error we
	* will assume that the resource could not be located or accessed.
	*
	* TODO: Try to find specific error codes to react only on
	* localisation failures.
	*
	* TODO, FIXME: Check the spec: is a namespace added to the imported
	* namespaces, even if the schemaLocation did not provide
	* a resource? I guess so, since omitting the "schemaLocation"
	* attribute, imports a namespace as well.
	*/
	lerr = xmlGetLastError();
	if ((lerr != NULL) && (lerr->domain == XML_FROM_IO)) {
	    xmlFreeParserCtxt(parserCtxt);
	    return(0);
	}
	xmlSchemaCustomErr(actxt,
	    XML_SCHEMAP_SRC_IMPORT_2_1,
	    node, NULL,
	    "Failed to parse the resource '%s' for import",
	    location, NULL);
	xmlFreeParserCtxt(parserCtxt);
	return(XML_SCHEMAP_SRC_IMPORT_2_1);
    }
    xmlFreeParserCtxt(parserCtxt);

    root = xmlDocGetRootElement(*doc);
    if (root == NULL) {
	xmlSchemaCustomErr(actxt,
	    XML_SCHEMAP_SRC_IMPORT_2_1,
	    node, NULL,
	    "The XML document '%s' to be imported has no document "
	    "element", location, NULL);
	xmlFreeDoc(*doc);
	*doc = NULL;
	return (XML_SCHEMAP_SRC_IMPORT_2_1);
    }

    xmlSchemaCleanupDoc(pctxt, root);

    if (!IS_SCHEMA(root, "schema")) {
	xmlSchemaCustomErr(actxt,
	    XML_SCHEMAP_SRC_IMPORT_2_1,
	    node, NULL,
	    "The XML document '%s' to be imported is not a XML schema document",
	    location, NULL);
	xmlFreeDoc(*doc);
	*doc = NULL;
	return (XML_SCHEMAP_SRC_IMPORT_2_1);
    }
    *targetNamespace = xmlSchemaGetProp(pctxt, root, "targetNamespace");
    /*
    * Schema Representation Constraint: Import Constraints and Semantics
    */
    if (nsName == NULL) {
	if (*targetNamespace != NULL) {
	    xmlSchemaCustomErr(actxt,
		XML_SCHEMAP_SRC_IMPORT_3_2,
		node, NULL,
		"The XML schema to be imported is not expected "
		"to have a target namespace; this differs from "
		"its target namespace of '%s'", *targetNamespace, NULL);
	    xmlFreeDoc(*doc);
	    *doc = NULL;
	    return (XML_SCHEMAP_SRC_IMPORT_3_2);
	}
    } else {
	if (*targetNamespace == NULL) {
	    xmlSchemaCustomErr(actxt,
		XML_SCHEMAP_SRC_IMPORT_3_1,
		node, NULL,
		"The XML schema to be imported is expected to have a target "
		"namespace of '%s'", nsName, NULL);
	    xmlFreeDoc(*doc);
	    *doc = NULL;
	    return (XML_SCHEMAP_SRC_IMPORT_3_1);
	} else if (!xmlStrEqual(*targetNamespace, nsName)) {
	    xmlSchemaCustomErr(actxt,
		XML_SCHEMAP_SRC_IMPORT_3_1,
		node, NULL,
		"The XML schema to be imported is expected to have a "
		"target namespace of '%s'; this differs from "
		"its target namespace of '%s'",
		nsName, *targetNamespace);
	    xmlFreeDoc(*doc);
	    *doc = NULL;
	    return (XML_SCHEMAP_SRC_IMPORT_3_1);
	}
    }
    import = xmlSchemaAddImport(pctxt, &(schema->schemasImports), nsName);
    if (import == NULL) {
	AERROR_INT("xmlSchemaAcquireSchemaDoc",
	    "failed to build import table");
	xmlFreeDoc(*doc);
	*doc = NULL;
	return (-1);
    }
    import->schemaLocation = location;
    import->doc = *doc;
    return (0);
}

static void
xmlSchemaParseForImpInc(xmlSchemaParserCtxtPtr pctxt,
			xmlSchemaPtr schema,
			const xmlChar *targetNamespace,
			xmlNodePtr node)
{
    const xmlChar *oldURL, **oldLocImps, *oldTNS;
    int oldFlags, oldNumLocImps, oldSizeLocImps, oldIsS4S;

    /*
    * Save and reset the context & schema.
    */
    oldURL = pctxt->URL;
    /* TODO: Is using the doc->URL here correct? */
    pctxt->URL = node->doc->URL;
    oldLocImps = pctxt->localImports;
    pctxt->localImports = NULL;
    oldNumLocImps = pctxt->nbLocalImports;
    pctxt->nbLocalImports = 0;
    oldSizeLocImps = pctxt->sizeLocalImports;
    pctxt->sizeLocalImports = 0;
    oldFlags = schema->flags;
    oldIsS4S = pctxt->isS4S;
    xmlSchemaClearSchemaDefaults(schema);
    oldTNS = schema->targetNamespace;
    schema->targetNamespace = targetNamespace;
    if ((targetNamespace != NULL) &&
	xmlStrEqual(targetNamespace, xmlSchemaNs)) {
	/*
	* We are parsing the schema for schema!
	*/
	pctxt->isS4S = 1;
    }
    /*
    * Parse the schema.
    */
    xmlSchemaParseSchemaDefaults(pctxt, schema, node);
    xmlSchemaParseSchemaTopLevel(pctxt, schema, node->children);
    /*
    * Restore the context & schema.
    */
    schema->flags = oldFlags;
    schema->targetNamespace = oldTNS;
    if (pctxt->localImports != NULL)
	xmlFree((xmlChar *) pctxt->localImports);
    pctxt->localImports = oldLocImps;
    pctxt->nbLocalImports = oldNumLocImps;
    pctxt->sizeLocalImports = oldSizeLocImps;
    pctxt->URL = oldURL;
    pctxt->isS4S = oldIsS4S;
}

/**
 * xmlSchemaParseImport:
 * @ctxt:  a schema validation context
 * @schema:  the schema being built
 * @node:  a subtree containing XML Schema informations
 *
 * parse a XML schema Import definition
 * *WARNING* this interface is highly subject to change
 *
 * Returns 0 in case of success, a positive error code if
 * not valid and -1 in case of an internal error.
 */
static int
xmlSchemaParseImport(xmlSchemaParserCtxtPtr ctxt, xmlSchemaPtr schema,
                     xmlNodePtr node)
{
    xmlNodePtr child;
    const xmlChar *namespaceName = NULL;
    const xmlChar *schemaLocation = NULL;
    const xmlChar *targetNamespace;
    xmlAttrPtr attr;
    xmlDocPtr doc;
    int ret = 0;

    if ((ctxt == NULL) || (schema == NULL) || (node == NULL))
        return (-1);

    /*
    * Check for illegal attributes.
    */
    attr = node->properties;
    while (attr != NULL) {
	if (attr->ns == NULL) {
	    if ((!xmlStrEqual(attr->name, BAD_CAST "id")) &&
		(!xmlStrEqual(attr->name, BAD_CAST "namespace")) &&
		(!xmlStrEqual(attr->name, BAD_CAST "schemaLocation"))) {
		xmlSchemaPIllegalAttrErr(ctxt,
		    XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		    NULL, NULL, attr);
	    }
	} else if (xmlStrEqual(attr->ns->href, xmlSchemaNs)) {
	    xmlSchemaPIllegalAttrErr(ctxt,
		XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		NULL, NULL, attr);
	}
	attr = attr->next;
    }
    /*
    * Extract and validate attributes.
    */
    if (xmlSchemaPValAttr(ctxt, NULL, NULL, node,
	"namespace", xmlSchemaGetBuiltInType(XML_SCHEMAS_ANYURI),
	&namespaceName) != 0) {
	xmlSchemaPSimpleTypeErr(ctxt,
	    XML_SCHEMAP_IMPORT_NAMESPACE_NOT_URI,
	    NULL, node,
	    xmlSchemaGetBuiltInType(XML_SCHEMAS_ANYURI),
	    NULL, namespaceName, NULL, NULL, NULL);
	return (XML_SCHEMAP_IMPORT_NAMESPACE_NOT_URI);
    }

    if (xmlSchemaPValAttr(ctxt, NULL, NULL, node,
	"schemaLocation", xmlSchemaGetBuiltInType(XML_SCHEMAS_ANYURI),
	&schemaLocation) != 0) {
	xmlSchemaPSimpleTypeErr(ctxt,
	    XML_SCHEMAP_IMPORT_SCHEMA_NOT_URI,
	    NULL, node,
	    xmlSchemaGetBuiltInType(XML_SCHEMAS_ANYURI),
	    NULL, namespaceName, NULL, NULL, NULL);
	return (XML_SCHEMAP_IMPORT_SCHEMA_NOT_URI);
    }
    /*
    * And now for the children...
    */
    child = node->children;
    if (IS_SCHEMA(child, "annotation")) {
        /*
         * the annotation here is simply discarded ...
	 * TODO: really?
         */
        child = child->next;
    }
    if (child != NULL) {
	xmlSchemaPContentErr(ctxt,
	    XML_SCHEMAP_UNKNOWN_IMPORT_CHILD,
	    NULL, NULL, node, child, NULL,
	    "(annotation?)");
    }
    /*
    * Apply additional constraints.
    */
    if (namespaceName != NULL) {
	/*
	* 1.1 If the namespace [attribute] is present, then its ·actual value·
	* must not match the ·actual value· of the enclosing <schema>'s
	* targetNamespace [attribute].
	*/
	if (xmlStrEqual(schema->targetNamespace, namespaceName)) {
	    xmlSchemaPCustomErr(ctxt,
		XML_SCHEMAP_SRC_IMPORT_1_1,
		NULL, NULL, node,
		"The value of the attribute 'namespace' must not match "
		"the target namespace '%s' of the importing schema",
		schema->targetNamespace);
	    return (XML_SCHEMAP_SRC_IMPORT_1_1);
	}
    } else {
	/*
	* 1.2 If the namespace [attribute] is not present, then the enclosing
	* <schema> must have a targetNamespace [attribute].
	*/
	if (schema->targetNamespace == NULL) {
	    xmlSchemaPCustomErr(ctxt,
		XML_SCHEMAP_SRC_IMPORT_1_2,
		NULL, NULL, node,
		"The attribute 'namespace' must be existent if "
		"the importing schema has no target namespace",
		NULL);
	    return (XML_SCHEMAP_SRC_IMPORT_1_2);
	}
    }
    /*
    * Add the namespace to the list of locally imported namespace.
    */
    if (ctxt->localImports == NULL) {
	ctxt->localImports = (const xmlChar **) xmlMalloc(10 *
	    sizeof(const xmlChar*));
	ctxt->sizeLocalImports = 10;
	ctxt->nbLocalImports = 0;
    } else if (ctxt->sizeLocalImports <= ctxt->nbLocalImports) {
	ctxt->sizeLocalImports *= 2;
	ctxt->localImports = (const xmlChar **) xmlRealloc(
	    (xmlChar **) ctxt->localImports,
	    ctxt->sizeLocalImports * sizeof(const xmlChar*));
    }
    ctxt->localImports[ctxt->nbLocalImports++] = namespaceName;
    /*
    * Locate and aquire the schema document.
    */
    ret = xmlSchemaAcquireSchemaDoc((xmlSchemaAbstractCtxtPtr) ctxt,
	schema, node, namespaceName,
	schemaLocation, &doc, &targetNamespace, 0);
    if (ret != 0) {
	if (doc != NULL)
	    xmlFreeDoc(doc);
	return (ret);
    } else if (doc != NULL) {
       	xmlSchemaParseForImpInc(ctxt, schema, targetNamespace,
	    xmlDocGetRootElement(doc));
    }

    return (0);
}

/**
 * xmlSchemaParseInclude:
 * @ctxt:  a schema validation context
 * @schema:  the schema being built
 * @node:  a subtree containing XML Schema informations
 *
 * parse a XML schema Include definition
 *
 * Returns -1 in case of error, 0 if the declaration is improper and
 *         1 in case of success.
 */
static int
xmlSchemaParseInclude(xmlSchemaParserCtxtPtr ctxt, xmlSchemaPtr schema,
                      xmlNodePtr node)
{
    xmlNodePtr child = NULL;
    const xmlChar *schemaLocation, *targetNamespace;
    xmlDocPtr doc = NULL;
    xmlNodePtr root = NULL;
    xmlSchemaIncludePtr include = NULL;
    int wasConvertingNs = 0;
    xmlAttrPtr attr;
    xmlParserCtxtPtr parserCtxt;


    if ((ctxt == NULL) || (schema == NULL) || (node == NULL))
        return (-1);

    /*
    * Check for illegal attributes.
    */
    attr = node->properties;
    while (attr != NULL) {
	if (attr->ns == NULL) {
	    if ((!xmlStrEqual(attr->name, BAD_CAST "id")) &&
		(!xmlStrEqual(attr->name, BAD_CAST "schemaLocation"))) {
		xmlSchemaPIllegalAttrErr(ctxt,
		    XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		    NULL, NULL, attr);
	    }
	} else if (xmlStrEqual(attr->ns->href, xmlSchemaNs)) {
	    xmlSchemaPIllegalAttrErr(ctxt,
		XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		NULL, NULL, attr);
	}
	attr = attr->next;
    }
    /*
    * Extract and validate attributes.
    */
    xmlSchemaPValAttrID(ctxt, NULL, NULL, node, BAD_CAST "id");
    /*
    * Preliminary step, extract the URI-Reference for the include and
    * make an URI from the base.
    */
    attr = xmlSchemaGetPropNode(node, "schemaLocation");
    if (attr != NULL) {
        xmlChar *base = NULL;
        xmlChar *uri = NULL;

	if (xmlSchemaPValAttrNode(ctxt, NULL, NULL, attr,
	    xmlSchemaGetBuiltInType(XML_SCHEMAS_ANYURI), &schemaLocation) != 0)
	    goto exit_invalid;
	base = xmlNodeGetBase(node->doc, node);
	if (base == NULL) {
	    uri = xmlBuildURI(schemaLocation, node->doc->URL);
	} else {
	    uri = xmlBuildURI(schemaLocation, base);
	    xmlFree(base);
	}
	if (uri == NULL) {
	    xmlSchemaPErr(ctxt,
		node,
		XML_SCHEMAP_INTERNAL,
		"Internal error: xmlSchemaParseInclude, "
		"could not build an URI from the schemaLocation.\n",
		NULL, NULL);
	    goto exit_failure;
	}
	schemaLocation = xmlDictLookup(ctxt->dict, uri, -1);
	xmlFree(uri);
    } else {
	xmlSchemaPMissingAttrErr(ctxt,
	    XML_SCHEMAP_INCLUDE_SCHEMA_NO_URI,
	    NULL, node, "schemaLocation", NULL);
	goto exit_invalid;
    }
    /*
    * And now for the children...
    */
    child = node->children;
    while (IS_SCHEMA(child, "annotation")) {
        /*
         * the annotations here are simply discarded ...
	 * TODO: really?
         */
        child = child->next;
    }
    if (child != NULL) {
	xmlSchemaPContentErr(ctxt,
	    XML_SCHEMAP_UNKNOWN_INCLUDE_CHILD,
	    NULL, NULL, node, child, NULL,
	    "(annotation?)");
    }
    /*
    * Report self-inclusion.
    */
    if (xmlStrEqual(schemaLocation, ctxt->URL)) {
	xmlSchemaPCustomErr(ctxt,
	    XML_SCHEMAP_SRC_INCLUDE,
	    NULL, NULL, node,
	    "The schema document '%s' cannot include itself.",
	    schemaLocation);
	return (XML_SCHEMAP_SRC_INCLUDE);
    }
    /*
    * Check if this one was already processed to avoid incorrect
    * duplicate component errors and infinite circular inclusion.
    */
    include = schema->includes;
    while (include != NULL) {
	if (xmlStrEqual(include->schemaLocation, schemaLocation)) {
	    targetNamespace = include->origTargetNamespace;
	    if (targetNamespace == NULL) {
		/*
		* Chameleon include: skip only if it was build for
		* the targetNamespace of the including schema.
		*/
		if (xmlStrEqual(schema->targetNamespace,
		    include->targetNamespace)) {
		    goto check_targetNamespace;
		}
	    } else {
		goto check_targetNamespace;
	    }
	}
	include = include->next;
    }
    /*
    * First step is to parse the input document into an DOM/Infoset
    * TODO: Use xmlCtxtReadFile to share the dictionary?
    */
    parserCtxt = xmlNewParserCtxt();
    if (parserCtxt == NULL) {
	xmlSchemaPErrMemory(NULL, "xmlSchemaParseInclude: "
	    "allocating a parser context", NULL);
	goto exit_failure;
    }

    if ((ctxt->dict != NULL) && (parserCtxt->dict != NULL)) {
	xmlDictFree(parserCtxt->dict);
	parserCtxt->dict = ctxt->dict;
	xmlDictReference(parserCtxt->dict);
    }

    doc = xmlCtxtReadFile(parserCtxt, (const char *) schemaLocation,
	    NULL, SCHEMAS_PARSE_OPTIONS);
    xmlFreeParserCtxt(parserCtxt);
    if (doc == NULL) {
	/*
	* TODO: It is not an error for the ·actual value· of the
	* schemaLocation [attribute] to fail to resolve it all, in which
	* case no corresponding inclusion is performed.
	* So do we need a warning report here?
	*/
	xmlSchemaPCustomErr(ctxt,
	    XML_SCHEMAP_FAILED_LOAD,
	    NULL, NULL, node,
	    "Failed to load the document '%s' for inclusion", schemaLocation);
	goto exit_invalid;
    }

    /*
     * Then extract the root of the schema
     */
    root = xmlDocGetRootElement(doc);
    if (root == NULL) {
	xmlSchemaPCustomErr(ctxt,
	    XML_SCHEMAP_NOROOT,
	    NULL, NULL, node,
	    "The included document '%s' has no document "
	    "element", schemaLocation);
	goto exit_invalid;
    }

    /*
     * Remove all the blank text nodes
     */
    xmlSchemaCleanupDoc(ctxt, root);

    /*
     * Check the schemas top level element
     */
    if (!IS_SCHEMA(root, "schema")) {
	xmlSchemaPCustomErr(ctxt,
	    XML_SCHEMAP_NOT_SCHEMA,
	    NULL, NULL, node,
	    "The document '%s' to be included is not a schema document",
	    schemaLocation);
	goto exit_invalid;
    }

    targetNamespace = xmlSchemaGetProp(ctxt, root, "targetNamespace");
    /*
    * 2.1 SII has a targetNamespace [attribute], and its ·actual
    * value· is identical to the ·actual value· of the targetNamespace
    * [attribute] of SII’ (which must have such an [attribute]).
    */
check_targetNamespace:
    if (targetNamespace != NULL) {
	if (schema->targetNamespace == NULL) {
	    xmlSchemaPCustomErr(ctxt,
		XML_SCHEMAP_SRC_INCLUDE,
		NULL, NULL, node,
		"The target namespace of the included schema "
		"'%s' has to be absent, since the including schema "
		"has no target namespace",
		schemaLocation);
	    goto exit_invalid;
	} else if (!xmlStrEqual(targetNamespace, schema->targetNamespace)) {
	    xmlSchemaPCustomErrExt(ctxt,
		XML_SCHEMAP_SRC_INCLUDE,
		NULL, NULL, node,
		"The target namespace '%s' of the included schema '%s' "
		"differs from '%s' of the including schema",
		targetNamespace, schemaLocation, schema->targetNamespace);
	    goto exit_invalid;
	}
    } else if (schema->targetNamespace != NULL) {
	if ((schema->flags & XML_SCHEMAS_INCLUDING_CONVERT_NS) == 0) {
	    schema->flags |= XML_SCHEMAS_INCLUDING_CONVERT_NS;
	} else
	    wasConvertingNs = 1;
    }

    if (include != NULL)
	goto exit;

    /*
    * URGENT TODO: If the schema is a chameleon-include then copy the
    * components into the including schema and modify the targetNamespace
    * of those components, do nothing otherwise.
    * NOTE: This is currently worked-around by compiling the chameleon
    * for every destinct including targetNamespace; thus not performant at
    * the moment.
    * TODO: Check when the namespace in wildcards for chameleons needs
    * to be converted: before we built wildcard intersections or after.
    */
    /*
    * Register the include.
    */
    include = (xmlSchemaIncludePtr) xmlMalloc(sizeof(xmlSchemaInclude));
    if (include == NULL) {
        xmlSchemaPErrMemory(ctxt, "allocating include entry", NULL);
	goto exit_failure;
    }
    memset(include, 0, sizeof(xmlSchemaInclude));
    include->next = schema->includes;
    schema->includes = include;
    /*
    * TODO: Use the resolved URI for the this location, since it might
    * differ if using filenames/URIs simultaneosly.
    */
    include->schemaLocation = schemaLocation;
    include->doc = doc;
    /*
    * In case of chameleons, the original target namespace will differ
    * from the resulting namespace.
    */
    include->origTargetNamespace = targetNamespace;
    include->targetNamespace = schema->targetNamespace;
#ifdef DEBUG_INCLUDES
    if (targetNamespace != schema->targetNamespace)
	xmlGenericError(xmlGenericErrorContext,
	    "INCLUDING CHAMELEON '%s'\n  orig TNS '%s'\n"
	    "  into TNS '%s'\n", schemaLocation,
	    targetNamespace, schema->targetNamespace);
    else
	xmlGenericError(xmlGenericErrorContext,
	    "INCLUDING '%s'\n  orig-TNS '%s'\n", schemaLocation,
	    targetNamespace);
#endif
    /*
    * Compile the included schema.
    */
    xmlSchemaParseForImpInc(ctxt, schema, schema->targetNamespace, root);

exit:
    /*
    * Remove the converting flag.
    */
    if ((wasConvertingNs == 0) &&
	(schema->flags & XML_SCHEMAS_INCLUDING_CONVERT_NS))
	schema->flags ^= XML_SCHEMAS_INCLUDING_CONVERT_NS;
    return (1);

exit_invalid:
    if (doc != NULL) {
	if (include != NULL)
	    include->doc = NULL;
	xmlFreeDoc(doc);
    }
    return (ctxt->err);

exit_failure:
    if (doc != NULL) {
	if (include != NULL)
	    include->doc = NULL;
	xmlFreeDoc(doc);
    }
    return (-1);
}

/**
 * xmlSchemaParseModelGroup:
 * @ctxt:  a schema validation context
 * @schema:  the schema being built
 * @node:  a subtree containing XML Schema informations
 * @type: the "compositor" type
 * @particleNeeded: if a a model group with a particle
 *
 * parse a XML schema Sequence definition
 * *WARNING* this interface is highly subject to change
 *
 * Returns -1 in case of error, 0 if the declaration is improper and
 *         1 in case of success.
 */
static xmlSchemaTreeItemPtr
xmlSchemaParseModelGroup(xmlSchemaParserCtxtPtr ctxt, xmlSchemaPtr schema,
			 xmlNodePtr node, xmlSchemaTypeType type,
			 int withParticle)
{
    xmlSchemaModelGroupPtr item;
    xmlSchemaParticlePtr particle = NULL;
    xmlNodePtr child = NULL;
    xmlAttrPtr attr;
    const xmlChar *oldcontainer, *container;
    int min = 0, max = 0;

    if ((ctxt == NULL) || (schema == NULL) || (node == NULL))
        return (NULL);
    /*
    * Create a model group with the given compositor.
    */
    item = xmlSchemaAddModelGroup(ctxt, schema, type, &container, node);
    if (item == NULL)
	return (NULL);

    if (withParticle) {
	if (type == XML_SCHEMA_TYPE_ALL) {
	    min = xmlGetMinOccurs(ctxt, node, 0, 1, 1, "(0 | 1)");
	    max = xmlGetMaxOccurs(ctxt, node, 1, 1, 1, "1");
	} else {
	    /* choice + sequence */
	    min = xmlGetMinOccurs(ctxt, node, 0, -1, 1, "xs:nonNegativeInteger");
	    max = xmlGetMaxOccurs(ctxt, node, 0, UNBOUNDED, 1,
		"(xs:nonNegativeInteger | unbounded)");
	}
	xmlSchemaPCheckParticleCorrect_2(ctxt, NULL, node, min, max);
	/*
	* Create a particle
	*/
	particle = xmlSchemaAddParticle(ctxt, schema, node, min, max);
	if (particle == NULL)
	    return (NULL);
	particle->children = (xmlSchemaTreeItemPtr) item;
	/*
	* Check for illegal attributes.
	*/
	attr = node->properties;
	while (attr != NULL) {
	    if (attr->ns == NULL) {
		if ((!xmlStrEqual(attr->name, BAD_CAST "id")) &&
		    (!xmlStrEqual(attr->name, BAD_CAST "maxOccurs")) &&
		    (!xmlStrEqual(attr->name, BAD_CAST "minOccurs"))) {
		    xmlSchemaPIllegalAttrErr(ctxt,
			XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
			NULL, NULL, attr);
		}
	    } else if (xmlStrEqual(attr->ns->href, xmlSchemaNs)) {
		xmlSchemaPIllegalAttrErr(ctxt,
		    XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		    NULL, NULL, attr);
	    }
	    attr = attr->next;
	}
    } else {
	/*
	* Check for illegal attributes.
	*/
	attr = node->properties;
	while (attr != NULL) {
	    if (attr->ns == NULL) {
		if (!xmlStrEqual(attr->name, BAD_CAST "id")) {
		    xmlSchemaPIllegalAttrErr(ctxt,
			XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
			NULL, NULL, attr);
		}
	    } else if (xmlStrEqual(attr->ns->href, xmlSchemaNs)) {
		xmlSchemaPIllegalAttrErr(ctxt,
		    XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		    NULL, NULL, attr);
	    }
	    attr = attr->next;
	}

    }

    /*
    * Extract and validate attributes.
    */
    xmlSchemaPValAttrID(ctxt, NULL, NULL, node, BAD_CAST "id");
    /*
    * And now for the children...
    */
    child = node->children;
    if (IS_SCHEMA(child, "annotation")) {
        item->annot = xmlSchemaParseAnnotation(ctxt, schema, child);
        child = child->next;
    }
    oldcontainer = ctxt->container;
    ctxt->container = container;
    if (type == XML_SCHEMA_TYPE_ALL) {
	xmlSchemaParticlePtr part, last = NULL;

	while (IS_SCHEMA(child, "element")) {
	    part = (xmlSchemaParticlePtr) xmlSchemaParseElement(ctxt,
		schema, child, 0);
	    if (part != NULL) {
		if (part->minOccurs > 1)
		    xmlSchemaPCustomErr(ctxt, XML_SCHEMAP_INVALID_MINOCCURS,
			NULL, NULL, child,
			"Invalid value for minOccurs (must be 0 or 1)", NULL);
		if (part->maxOccurs > 1)
		    xmlSchemaPCustomErr(ctxt, XML_SCHEMAP_INVALID_MAXOCCURS,
			NULL, NULL, child,
			"Invalid value for maxOccurs (must be 0 or 1)",
			NULL);
		if (last == NULL)
		    item->children = (xmlSchemaTreeItemPtr) part;
		else
		    last->next = (xmlSchemaTreeItemPtr) part;
		last = part;
	    }
	    child = child->next;
	}
	if (child != NULL) {
	    xmlSchemaPContentErr(ctxt,
		XML_SCHEMAP_S4S_ELEM_NOT_ALLOWED,
		NULL, NULL, node, child, NULL,
		"(annotation?, (annotation?, element*)");
	}
    } else {
	/* choice + sequence */
	xmlSchemaTreeItemPtr part = NULL, last = NULL;

	while ((IS_SCHEMA(child, "element")) ||
	    (IS_SCHEMA(child, "group")) ||
	    (IS_SCHEMA(child, "any")) ||
	    (IS_SCHEMA(child, "choice")) ||
	    (IS_SCHEMA(child, "sequence"))) {

	    if (IS_SCHEMA(child, "element")) {
		part = (xmlSchemaTreeItemPtr)
		    xmlSchemaParseElement(ctxt, schema, child, 0);
	    } else if (IS_SCHEMA(child, "group")) {
		part =
		    xmlSchemaParseModelGroupDefRef(ctxt, schema, child);
	    } else if (IS_SCHEMA(child, "any")) {
		part = (xmlSchemaTreeItemPtr)
		    xmlSchemaParseAny(ctxt, schema, child);
	    } else if (IS_SCHEMA(child, "choice")) {
		part = xmlSchemaParseModelGroup(ctxt, schema, child,
		    XML_SCHEMA_TYPE_CHOICE, 1);
	    } else if (IS_SCHEMA(child, "sequence")) {
		part = xmlSchemaParseModelGroup(ctxt, schema, child,
		    XML_SCHEMA_TYPE_SEQUENCE, 1);
	    }
	    if (part != NULL) {
		if (last == NULL)
		    item->children = part;
		else
		    last->next = part;
		last = part;
	    }
	    child = child->next;
	}
	if (child != NULL) {
	    xmlSchemaPContentErr(ctxt,
		XML_SCHEMAP_S4S_ELEM_NOT_ALLOWED,
		NULL, NULL, node, child, NULL,
		"(annotation?, (element | group | choice | sequence | any)*)");
	}
    }
    ctxt->container = oldcontainer;
    if (withParticle) {
	if ((min == 0) && (max == 0))
	    return (NULL);
	else
	    return ((xmlSchemaTreeItemPtr) particle);
    } else
	return ((xmlSchemaTreeItemPtr) item);
}

/**
 * xmlSchemaParseRestriction:
 * @ctxt:  a schema validation context
 * @schema:  the schema being built
 * @node:  a subtree containing XML Schema informations
 *
 * parse a XML schema Restriction definition
 * *WARNING* this interface is highly subject to change
 *
 * Returns the type definition or NULL in case of error
 */
static xmlSchemaTypePtr
xmlSchemaParseRestriction(xmlSchemaParserCtxtPtr ctxt, xmlSchemaPtr schema,
                          xmlNodePtr node, xmlSchemaTypeType parentType)
{
    xmlSchemaTypePtr type;
    xmlNodePtr child = NULL;
    char buf[30];
    const xmlChar *oldcontainer, *container;
    xmlAttrPtr attr;

    if ((ctxt == NULL) || (schema == NULL) || (node == NULL))
        return (NULL);
    /* Not a component, don't create it. */
    type = ctxt->ctxtType;
    type->flags |= XML_SCHEMAS_TYPE_DERIVATION_METHOD_RESTRICTION;

    /*
    * TODO: Is the container needed at all? the anonymous
    * items inside should generate unique names already.
    */
    snprintf(buf, 29, "#restr%d", ctxt->counter++ + 1);
    container = xmlDictLookup(ctxt->dict, BAD_CAST buf, -1);
    /*
    * Check for illegal attributes.
    */
    attr = node->properties;
    while (attr != NULL) {
	if (attr->ns == NULL) {
	    if ((!xmlStrEqual(attr->name, BAD_CAST "id")) &&
		(!xmlStrEqual(attr->name, BAD_CAST "base"))) {
		xmlSchemaPIllegalAttrErr(ctxt,
		    XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		    NULL, NULL, attr);
	    }
	} else if (xmlStrEqual(attr->ns->href, xmlSchemaNs)) {
	    xmlSchemaPIllegalAttrErr(ctxt,
		XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		NULL, NULL, attr);
	}
	attr = attr->next;
    }
    /*
    * Extract and validate attributes.
    */
    xmlSchemaPValAttrID(ctxt, NULL, NULL, node, BAD_CAST "id");
    /*
    * Attribute "base" - mandatory if inside a complex type.
    */
    /*
    * SPEC (1.2) "otherwise (<restriction> has no <simpleType> "
    * among its [children]), the simple type definition which is
    * the {content type} of the type definition ·resolved· to by
    * the ·actual value· of the base [attribute]"
    */
    if ((xmlSchemaPValAttrQName(ctxt, schema,
	NULL, NULL, node, "base",
	&(type->baseNs), &(type->base)) == 0) &&
	(type->base == NULL) &&
	(type->type == XML_SCHEMA_TYPE_COMPLEX)) {
	xmlSchemaPMissingAttrErr(ctxt,
	    XML_SCHEMAP_S4S_ATTR_MISSING,
	    type, node, "base", NULL);
    }
    /*
    * And now for the children...
    */
    child = node->children;
    if (IS_SCHEMA(child, "annotation")) {
	/*
	* Add the annotation to the simple type ancestor.
	*/
	xmlSchemaAddAnnotation((xmlSchemaAnnotItemPtr) type,
	    xmlSchemaParseAnnotation(ctxt, schema, child));
        child = child->next;
    }
    oldcontainer = ctxt->container;
    ctxt->container = container;
    if (parentType == XML_SCHEMA_TYPE_SIMPLE) {
	/*
	* Corresponds to <simpleType><restriction><simpleType>.
	*/
	if (IS_SCHEMA(child, "simpleType")) {
	    if (type->base != NULL) {
		/*
		* src-restriction-base-or-simpleType
		* Either the base [attribute] or the simpleType [child] of the
		* <restriction> element must be present, but not both.
		*/
		xmlSchemaPContentErr(ctxt,
		    XML_SCHEMAP_SRC_RESTRICTION_BASE_OR_SIMPLETYPE,
		    NULL, NULL, node, child,
		    "The attribute 'base' and the <simpleType> child are "
		    "mutually exclusive", NULL);
	    } else {
		type->baseType = (xmlSchemaTypePtr)
		    xmlSchemaParseSimpleType(ctxt, schema, child, 0);
	    }
	    child = child->next;
	} else if (type->base == NULL) {
	    xmlSchemaPContentErr(ctxt,
		XML_SCHEMAP_SRC_RESTRICTION_BASE_OR_SIMPLETYPE,
		NULL, NULL, node, child,
		"Either the attribute 'base' or a <simpleType> child "
		"must be present", NULL);
	}
    } else if (parentType == XML_SCHEMA_TYPE_COMPLEX_CONTENT) {
	/*
	* Corresponds to <complexType><complexContent><restriction>...
	* followed by:
	*
	* Model groups <all>, <choice> and <sequence>.
	*/
	if (IS_SCHEMA(child, "all")) {
	    type->subtypes = (xmlSchemaTypePtr)
		xmlSchemaParseModelGroup(ctxt, schema, child,
		    XML_SCHEMA_TYPE_ALL, 1);
	    child = child->next;
	} else if (IS_SCHEMA(child, "choice")) {
	    type->subtypes = (xmlSchemaTypePtr)
		xmlSchemaParseModelGroup(ctxt,
		    schema, child, XML_SCHEMA_TYPE_CHOICE, 1);
	    child = child->next;
	} else if (IS_SCHEMA(child, "sequence")) {
	    type->subtypes = (xmlSchemaTypePtr)
		xmlSchemaParseModelGroup(ctxt, schema, child,
		    XML_SCHEMA_TYPE_SEQUENCE, 1);
	    child = child->next;
	/*
	* Model group reference <group>.
	*/
	} else if (IS_SCHEMA(child, "group")) {
	    type->subtypes = (xmlSchemaTypePtr)
		xmlSchemaParseModelGroupDefRef(ctxt, schema, child);
	    child = child->next;
	}
    } else if (parentType == XML_SCHEMA_TYPE_SIMPLE_CONTENT) {
	/*
	* Corresponds to <complexType><simpleContent><restriction>...
	*
	* "1.1 the simple type definition corresponding to the <simpleType>
	* among the [children] of <restriction> if there is one;"
	*/
	if (IS_SCHEMA(child, "simpleType")) {
	    /*
	    * We will store the to-be-restricted simple type in
	    * type->contentTypeDef *temporarily*.
	    */
	    type->contentTypeDef = (xmlSchemaTypePtr)
		xmlSchemaParseSimpleType(ctxt, schema, child, 0);
	    if ( type->contentTypeDef == NULL)
		return (NULL);
	    child = child->next;
	}
    }

    if ((parentType == XML_SCHEMA_TYPE_SIMPLE) ||
	(parentType == XML_SCHEMA_TYPE_SIMPLE_CONTENT)) {
	xmlSchemaFacetPtr facet, lastfacet = NULL;
	/*
	* Corresponds to <complexType><simpleContent><restriction>...
	* <simpleType><restriction>...
	*/

	/*
	* Add the facets to the simple type ancestor.
	*/
	/*
	* TODO: Datatypes: 4.1.3 Constraints on XML Representation of
	* Simple Type Definition Schema Representation Constraint:
	* *Single Facet Value*
	*/
	while ((IS_SCHEMA(child, "minInclusive")) ||
	    (IS_SCHEMA(child, "minExclusive")) ||
	    (IS_SCHEMA(child, "maxInclusive")) ||
	    (IS_SCHEMA(child, "maxExclusive")) ||
	    (IS_SCHEMA(child, "totalDigits")) ||
	    (IS_SCHEMA(child, "fractionDigits")) ||
	    (IS_SCHEMA(child, "pattern")) ||
	    (IS_SCHEMA(child, "enumeration")) ||
	    (IS_SCHEMA(child, "whiteSpace")) ||
	    (IS_SCHEMA(child, "length")) ||
	    (IS_SCHEMA(child, "maxLength")) ||
	    (IS_SCHEMA(child, "minLength"))) {
	    facet = xmlSchemaParseFacet(ctxt, schema, child);
	    if (facet != NULL) {
		if (lastfacet == NULL)
		    type->facets = facet;
		else
		    lastfacet->next = facet;
		lastfacet = facet;
		lastfacet->next = NULL;
	    }
	    child = child->next;
	}
	/*
	* Create links for derivation and validation.
	*/
	if (type->facets != NULL) {
	    xmlSchemaFacetLinkPtr facetLink, lastFacetLink = NULL;

	    facet = type->facets;
	    do {
		facetLink = (xmlSchemaFacetLinkPtr)
		    xmlMalloc(sizeof(xmlSchemaFacetLink));
		if (facetLink == NULL) {
		    xmlSchemaPErrMemory(ctxt, "allocating a facet link", NULL);
		    xmlFree(facetLink);
		    return (NULL);
		}
		facetLink->facet = facet;
		facetLink->next = NULL;
		if (lastFacetLink == NULL)
		    type->facetSet = facetLink;
		else
		    lastFacetLink->next = facetLink;
		lastFacetLink = facetLink;
		facet = facet->next;
	    } while (facet != NULL);
	}
    }
    if (type->type == XML_SCHEMA_TYPE_COMPLEX) {
	/*
	* Attribute uses/declarations.
	*/
	child = xmlSchemaParseAttrDecls(ctxt, schema, child, type);
	/*
	* Attribute wildcard.
	*/
	if (IS_SCHEMA(child, "anyAttribute")) {
	    type->attributeWildcard =
		xmlSchemaParseAnyAttribute(ctxt, schema, child);
	    child = child->next;
	}
    }
    if (child != NULL) {
	if (parentType == XML_SCHEMA_TYPE_COMPLEX_CONTENT) {
	    xmlSchemaPContentErr(ctxt,
		XML_SCHEMAP_S4S_ELEM_NOT_ALLOWED,
		NULL, NULL, node, child, NULL,
		"annotation?, (group | all | choice | sequence)?, "
		"((attribute | attributeGroup)*, anyAttribute?))");
	} else if (parentType == XML_SCHEMA_TYPE_SIMPLE_CONTENT) {
	     xmlSchemaPContentErr(ctxt,
		XML_SCHEMAP_S4S_ELEM_NOT_ALLOWED,
		NULL, NULL, node, child, NULL,
		"(annotation?, (simpleType?, (minExclusive | minInclusive | "
		"maxExclusive | maxInclusive | totalDigits | fractionDigits | "
		"length | minLength | maxLength | enumeration | whiteSpace | "
		"pattern)*)?, ((attribute | attributeGroup)*, anyAttribute?))");
	} else {
	    /* Simple type */
	    xmlSchemaPContentErr(ctxt,
		XML_SCHEMAP_S4S_ELEM_NOT_ALLOWED,
		NULL, NULL, node, child, NULL,
		"(annotation?, (simpleType?, (minExclusive | minInclusive | "
		"maxExclusive | maxInclusive | totalDigits | fractionDigits | "
		"length | minLength | maxLength | enumeration | whiteSpace | "
		"pattern)*))");
	}
    }
    ctxt->container = oldcontainer;
    return (NULL);
}

/**
 * xmlSchemaParseExtension:
 * @ctxt:  a schema validation context
 * @schema:  the schema being built
 * @node:  a subtree containing XML Schema informations
 *
 * Parses an <extension>, which is found inside a
 * <simpleContent> or <complexContent>.
 * *WARNING* this interface is highly subject to change.
 *
 * TODO: Returns the type definition or NULL in case of error
 */
static xmlSchemaTypePtr
xmlSchemaParseExtension(xmlSchemaParserCtxtPtr ctxt, xmlSchemaPtr schema,
                        xmlNodePtr node, xmlSchemaTypeType parentType)
{
    xmlSchemaTypePtr type;
    xmlNodePtr child = NULL;
    char buf[30];
    const xmlChar *oldcontainer, *container;
    xmlAttrPtr attr;

    if ((ctxt == NULL) || (schema == NULL) || (node == NULL))
        return (NULL);
    /* Not a component, don't create it. */
    type = ctxt->ctxtType;
    type->flags |= XML_SCHEMAS_TYPE_DERIVATION_METHOD_EXTENSION;

    snprintf(buf, 29, "#ext%d", ctxt->counter++ + 1);
    container = xmlDictLookup(ctxt->dict, BAD_CAST buf, -1);
    /*
    * Check for illegal attributes.
    */
    attr = node->properties;
    while (attr != NULL) {
	if (attr->ns == NULL) {
	    if ((!xmlStrEqual(attr->name, BAD_CAST "id")) &&
		(!xmlStrEqual(attr->name, BAD_CAST "base"))) {
		xmlSchemaPIllegalAttrErr(ctxt,
		    XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		    NULL, NULL, attr);
	    }
	} else if (xmlStrEqual(attr->ns->href, xmlSchemaNs)) {
	    xmlSchemaPIllegalAttrErr(ctxt,
		XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		NULL, NULL, attr);
	}
	attr = attr->next;
    }

    xmlSchemaPValAttrID(ctxt, NULL, NULL, node, BAD_CAST "id");

    /*
    * Attribute "base" - mandatory.
    */
    if ((xmlSchemaPValAttrQName(ctxt, schema,
	NULL, NULL, node, "base", &(type->baseNs), &(type->base)) == 0) &&
	(type->base == NULL)) {
	xmlSchemaPMissingAttrErr(ctxt,
	    XML_SCHEMAP_S4S_ATTR_MISSING,
	    NULL, node, "base", NULL);
    }
    /*
    * And now for the children...
    */
    child = node->children;
    if (IS_SCHEMA(child, "annotation")) {
	/*
	* Add the annotation to the type ancestor.
	*/
	xmlSchemaAddAnnotation((xmlSchemaAnnotItemPtr) type,
	    xmlSchemaParseAnnotation(ctxt, schema, child));
        child = child->next;
    }
    oldcontainer = ctxt->container;
    ctxt->container = container;
    if (parentType == XML_SCHEMA_TYPE_COMPLEX_CONTENT) {
	/*
	* Corresponds to <complexType><complexContent><extension>... and:
	*
	* Model groups <all>, <choice>, <sequence> and <group>.
	*/
	if (IS_SCHEMA(child, "all")) {
	    type->subtypes = (xmlSchemaTypePtr)
		xmlSchemaParseModelGroup(ctxt, schema,
		    child, XML_SCHEMA_TYPE_ALL, 1);
	    child = child->next;
	} else if (IS_SCHEMA(child, "choice")) {
	    type->subtypes = (xmlSchemaTypePtr)
		xmlSchemaParseModelGroup(ctxt, schema,
		    child, XML_SCHEMA_TYPE_CHOICE, 1);
	    child = child->next;
	} else if (IS_SCHEMA(child, "sequence")) {
	    type->subtypes = (xmlSchemaTypePtr)
		xmlSchemaParseModelGroup(ctxt, schema,
		child, XML_SCHEMA_TYPE_SEQUENCE, 1);
	    child = child->next;
	} else if (IS_SCHEMA(child, "group")) {
	    type->subtypes = (xmlSchemaTypePtr)
		xmlSchemaParseModelGroupDefRef(ctxt, schema, child);
	    child = child->next;
	}
    }
    if (child != NULL) {
	/*
	* Attribute uses/declarations.
	*/
	child = xmlSchemaParseAttrDecls(ctxt, schema, child, type);
	/*
	* Attribute wildcard.
	*/
	if (IS_SCHEMA(child, "anyAttribute")) {
	    ctxt->ctxtType->attributeWildcard =
		xmlSchemaParseAnyAttribute(ctxt, schema, child);
	    child = child->next;
	}
    }
    if (child != NULL) {
	if (parentType == XML_SCHEMA_TYPE_COMPLEX_CONTENT) {
	    /* Complex content extension. */
	    xmlSchemaPContentErr(ctxt,
		XML_SCHEMAP_S4S_ELEM_NOT_ALLOWED,
		NULL, NULL, node, child, NULL,
		"(annotation?, ((group | all | choice | sequence)?, "
		"((attribute | attributeGroup)*, anyAttribute?)))");
	} else {
	    /* Simple content extension. */
	    xmlSchemaPContentErr(ctxt,
		XML_SCHEMAP_S4S_ELEM_NOT_ALLOWED,
		NULL, NULL, node, child, NULL,
		"(annotation?, ((attribute | attributeGroup)*, "
		"anyAttribute?))");
	}
    }
    ctxt->container = oldcontainer;
    return (NULL);
}

/**
 * xmlSchemaParseSimpleContent:
 * @ctxt:  a schema validation context
 * @schema:  the schema being built
 * @node:  a subtree containing XML Schema informations
 *
 * parse a XML schema SimpleContent definition
 * *WARNING* this interface is highly subject to change
 *
 * Returns the type definition or NULL in case of error
 */
static int
xmlSchemaParseSimpleContent(xmlSchemaParserCtxtPtr ctxt,
                            xmlSchemaPtr schema, xmlNodePtr node)
{
    xmlSchemaTypePtr type;
    xmlNodePtr child = NULL;
    xmlAttrPtr attr;

    if ((ctxt == NULL) || (schema == NULL) || (node == NULL))
        return (-1);
    /* Not a component, don't create it. */
    type = ctxt->ctxtType;
    type->contentType = XML_SCHEMA_CONTENT_SIMPLE;
    /*
    * Check for illegal attributes.
    */
    attr = node->properties;
    while (attr != NULL) {
	if (attr->ns == NULL) {
	    if ((!xmlStrEqual(attr->name, BAD_CAST "id"))) {
		xmlSchemaPIllegalAttrErr(ctxt,
		    XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		    NULL, NULL, attr);
	    }
	} else if (xmlStrEqual(attr->ns->href, xmlSchemaNs)) {
	    xmlSchemaPIllegalAttrErr(ctxt,
		XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		NULL, NULL, attr);
	}
	attr = attr->next;
    }

    xmlSchemaPValAttrID(ctxt, NULL, NULL, node, BAD_CAST "id");

    /*
    * And now for the children...
    */
    child = node->children;
    if (IS_SCHEMA(child, "annotation")) {
	/*
	* Add the annotation to the complex type ancestor.
	*/
	xmlSchemaAddAnnotation((xmlSchemaAnnotItemPtr) type,
	    xmlSchemaParseAnnotation(ctxt, schema, child));
        child = child->next;
    }
    if (IS_SCHEMA(child, "restriction")) {
        xmlSchemaParseRestriction(ctxt, schema, child,
	    XML_SCHEMA_TYPE_SIMPLE_CONTENT);
        child = child->next;
    } else if (IS_SCHEMA(child, "extension")) {
        xmlSchemaParseExtension(ctxt, schema, child,
	    XML_SCHEMA_TYPE_SIMPLE_CONTENT);
        child = child->next;
    }
    if (child != NULL) {
	xmlSchemaPContentErr(ctxt,
	    XML_SCHEMAP_S4S_ELEM_NOT_ALLOWED,
	    NULL, NULL, node, child, NULL,
	    "(annotation?, (restriction | extension))");
    }
    return (0);
}

/**
 * xmlSchemaParseComplexContent:
 * @ctxt:  a schema validation context
 * @schema:  the schema being built
 * @node:  a subtree containing XML Schema informations
 *
 * parse a XML schema ComplexContent definition
 * *WARNING* this interface is highly subject to change
 *
 * Returns the type definition or NULL in case of error
 */
static int
xmlSchemaParseComplexContent(xmlSchemaParserCtxtPtr ctxt,
                             xmlSchemaPtr schema, xmlNodePtr node)
{
    xmlSchemaTypePtr type;
    xmlNodePtr child = NULL;
    xmlAttrPtr attr;

    if ((ctxt == NULL) || (schema == NULL) || (node == NULL))
        return (-1);
    /* Not a component, don't create it. */
    type = ctxt->ctxtType;
    /*
    * Check for illegal attributes.
    */
    attr = node->properties;
    while (attr != NULL) {
	if (attr->ns == NULL) {
	    if ((!xmlStrEqual(attr->name, BAD_CAST "id")) &&
		(!xmlStrEqual(attr->name, BAD_CAST "mixed")))
	    {
		xmlSchemaPIllegalAttrErr(ctxt,
		    XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		    NULL, NULL, attr);
	    }
	} else if (xmlStrEqual(attr->ns->href, xmlSchemaNs)) {
	    xmlSchemaPIllegalAttrErr(ctxt,
		XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		NULL, NULL, attr);
	}
	attr = attr->next;
    }

    xmlSchemaPValAttrID(ctxt, NULL, NULL, node, BAD_CAST "id");

    /*
    * Set the 'mixed' on the complex type ancestor.
    */
    if (xmlGetBooleanProp(ctxt, NULL, NULL, node, "mixed", 0))  {
	if ((type->flags & XML_SCHEMAS_TYPE_MIXED) == 0)
	    type->flags |= XML_SCHEMAS_TYPE_MIXED;
    }
    child = node->children;
    if (IS_SCHEMA(child, "annotation")) {
	/*
	* Add the annotation to the complex type ancestor.
	*/
	xmlSchemaAddAnnotation((xmlSchemaAnnotItemPtr) type,
	    xmlSchemaParseAnnotation(ctxt, schema, child));
        child = child->next;
    }
    if (IS_SCHEMA(child, "restriction")) {
        xmlSchemaParseRestriction(ctxt, schema, child,
	    XML_SCHEMA_TYPE_COMPLEX_CONTENT);
        child = child->next;
    } else if (IS_SCHEMA(child, "extension")) {
        xmlSchemaParseExtension(ctxt, schema, child,
	    XML_SCHEMA_TYPE_COMPLEX_CONTENT);
        child = child->next;
    }
    if (child != NULL) {
	xmlSchemaPContentErr(ctxt,
	    XML_SCHEMAP_S4S_ELEM_NOT_ALLOWED,
	    NULL, NULL, node, child,
	    NULL, "(annotation?, (restriction | extension))");
    }
    return (0);
}

/**
 * xmlSchemaParseComplexType:
 * @ctxt:  a schema validation context
 * @schema:  the schema being built
 * @node:  a subtree containing XML Schema informations
 *
 * parse a XML schema Complex Type definition
 * *WARNING* this interface is highly subject to change
 *
 * Returns the type definition or NULL in case of error
 */
static xmlSchemaTypePtr
xmlSchemaParseComplexType(xmlSchemaParserCtxtPtr ctxt, xmlSchemaPtr schema,
                          xmlNodePtr node, int topLevel)
{
    xmlSchemaTypePtr type, ctxtType;
    xmlNodePtr child = NULL;
    const xmlChar *oldcontainer, *name = NULL;
    xmlAttrPtr attr;
    const xmlChar *attrValue;
    xmlChar *des = NULL; /* The reported designation. */
    char buf[40];
    int final = 0, block = 0;


    if ((ctxt == NULL) || (schema == NULL) || (node == NULL))
        return (NULL);

    ctxtType = ctxt->ctxtType;

    if (topLevel) {
	attr = xmlSchemaGetPropNode(node, "name");
	if (attr == NULL) {
	    xmlSchemaPMissingAttrErr(ctxt,
		XML_SCHEMAP_S4S_ATTR_MISSING, NULL, node, "name", NULL);
	    return (NULL);
	} else if (xmlSchemaPValAttrNode(ctxt,
	    (xmlChar **) &xmlSchemaElemDesCT, NULL, attr,
	    xmlSchemaGetBuiltInType(XML_SCHEMAS_NCNAME), &name) != 0) {
	    return (NULL);
	}
    }

    if (topLevel == 0) {
	/*
	* Parse as local complex type definition.
	*/
        snprintf(buf, 39, "#CT%d", ctxt->counter++ + 1);
	type = xmlSchemaAddType(ctxt, schema, (const xmlChar *)buf, NULL, node);
	if (type == NULL)
	    return (NULL);
	name = type->name;
	type->node = node;
	type->type = XML_SCHEMA_TYPE_COMPLEX;
	/*
	* TODO: We need the target namespace.
	*/
    } else {
	/*
	* Parse as global complex type definition.
	*/
	type = xmlSchemaAddType(ctxt, schema, name, schema->targetNamespace, node);
	if (type == NULL)
	    return (NULL);
	type->node = node;
	type->type = XML_SCHEMA_TYPE_COMPLEX;
	type->flags |= XML_SCHEMAS_TYPE_GLOBAL;
    }
    type->targetNamespace = schema->targetNamespace;
    /*
    * Handle attributes.
    */
    attr = node->properties;
    while (attr != NULL) {
	if (attr->ns == NULL) {
	    if (xmlStrEqual(attr->name, BAD_CAST "id")) {
		/*
		* Attribute "id".
		*/
		xmlSchemaPValAttrID(ctxt, NULL, type, node,
		    BAD_CAST "id");
	    } else if (xmlStrEqual(attr->name, BAD_CAST "mixed")) {
		/*
		* Attribute "mixed".
		*/
		if (xmlSchemaPGetBoolNodeValue(ctxt, &des, type,
		    (xmlNodePtr) attr))
		    type->flags |= XML_SCHEMAS_TYPE_MIXED;
	    } else if (topLevel) {
		/*
		* Attributes of global complex type definitions.
		*/
		if (xmlStrEqual(attr->name, BAD_CAST "name")) {
		    /* Pass. */
		} else if (xmlStrEqual(attr->name, BAD_CAST "abstract")) {
		    /*
		    * Attribute "abstract".
		    */
		    if (xmlSchemaPGetBoolNodeValue(ctxt, &des, type,
			(xmlNodePtr) attr))
			type->flags |= XML_SCHEMAS_TYPE_ABSTRACT;
		} else if (xmlStrEqual(attr->name, BAD_CAST "final")) {
		    /*
		    * Attribute "final".
		    */
		    attrValue = xmlSchemaGetNodeContent(ctxt,
			(xmlNodePtr) attr);
		    if (xmlSchemaPValAttrBlockFinal(attrValue,
			&(type->flags),
			-1,
			XML_SCHEMAS_TYPE_FINAL_EXTENSION,
			XML_SCHEMAS_TYPE_FINAL_RESTRICTION,
			-1, -1, -1) != 0)
		    {
			xmlSchemaPSimpleTypeErr(ctxt,
			    XML_SCHEMAP_S4S_ATTR_INVALID_VALUE,
			    type, (xmlNodePtr) attr, NULL,
			    "(#all | List of (extension | restriction))",
			    attrValue, NULL, NULL, NULL);
		    } else 
			final = 1;
		} else if (xmlStrEqual(attr->name, BAD_CAST "block")) {
		    /*
		    * Attribute "block".
		    */
		    attrValue = xmlSchemaGetNodeContent(ctxt,
			(xmlNodePtr) attr);
		    if (xmlSchemaPValAttrBlockFinal(attrValue, &(type->flags),
			-1,
			XML_SCHEMAS_TYPE_BLOCK_EXTENSION,
			XML_SCHEMAS_TYPE_BLOCK_RESTRICTION,
			-1, -1, -1) != 0) {
			xmlSchemaPSimpleTypeErr(ctxt,
			    XML_SCHEMAP_S4S_ATTR_INVALID_VALUE,
			    type, (xmlNodePtr) attr, NULL,
			    "(#all | List of (extension | restriction)) ",
			    attrValue, NULL, NULL, NULL);
		    } else 
			block = 1;
		} else {
			xmlSchemaPIllegalAttrErr(ctxt,
			    XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
			    &des, type, attr);
		}
	    } else {
		xmlSchemaPIllegalAttrErr(ctxt,
		    XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		    &des, type, attr);
	    }
	} else if (xmlStrEqual(attr->ns->href, xmlSchemaNs)) {
	    xmlSchemaPIllegalAttrErr(ctxt,
		XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED,
		&des, type, attr);
	}
	attr = attr->next;
    }
    if (! block) {
	/*
	* Apply default "block" values.
	*/
	if (schema->flags & XML_SCHEMAS_BLOCK_DEFAULT_RESTRICTION)
	    type->flags |= XML_SCHEMAS_TYPE_BLOCK_RESTRICTION;
	if (schema->flags & XML_SCHEMAS_BLOCK_DEFAULT_EXTENSION)
	    type->flags |= XML_SCHEMAS_TYPE_BLOCK_EXTENSION;
    }
    if (! final) {
	/*
	* Apply default "block" values.
	*/
	if (schema->flags & XML_SCHEMAS_FINAL_DEFAULT_RESTRICTION)
	    type->flags |= XML_SCHEMAS_TYPE_FINAL_RESTRICTION;
	if (schema->flags & XML_SCHEMAS_FINAL_DEFAULT_EXTENSION)
	    type->flags |= XML_SCHEMAS_TYPE_FINAL_EXTENSION;
    }
    /*
    * And now for the children...
    */
    oldcontainer = ctxt->container;
    ctxt->container = name;
    child = node->children;
    if (IS_SCHEMA(child, "annotation")) {
        type->annot = xmlSchemaParseAnnotation(ctxt, schema, child);
        child = child->next;
    }
    ctxt->ctxtType = type;
    if (IS_SCHEMA(child, "simpleContent")) {
	/*
	* 3.4.3 : 2.2
	* Specifying mixed='true' when the <simpleContent>
	* alternative is chosen has no effect
	*/
	if (type->flags & XML_SCHEMAS_TYPE_MIXED)
	    type->flags ^= XML_SCHEMAS_TYPE_MIXED;
        xmlSchemaParseSimpleContent(ctxt, schema, child);
        child = child->next;
    } else if (IS_SCHEMA(child, "complexContent")) {
	type->contentType = XML_SCHEMA_CONTENT_EMPTY;
        xmlSchemaParseComplexContent(ctxt, schema, child);
        child = child->next;
    } else {
	/*
	* SPEC
	* "...the third alternative (neither <simpleContent> nor
	* <complexContent>) is chosen. This case is understood as shorthand
	* for complex content restricting the ·ur-type definition·, and the
	* details of the mappings should be modified as necessary.
	*/
	type->baseType = xmlSchemaGetBuiltInType(XML_SCHEMAS_ANYTYPE);
	type->flags |= XML_SCHEMAS_TYPE_DERIVATION_METHOD_RESTRICTION;
	/*
	* Parse model groups.
	*/
        if (IS_SCHEMA(child, "all")) {
            type->subtypes = (xmlSchemaTypePtr)
		xmlSchemaParseModelGroup(ctxt, schema, child,
		    XML_SCHEMA_TYPE_ALL, 1);
            child = child->next;
        } else if (IS_SCHEMA(child, "choice")) {
            type->subtypes = (xmlSchemaTypePtr)
		xmlSchemaParseModelGroup(ctxt, schema, child,
		    XML_SCHEMA_TYPE_CHOICE, 1);
            child = child->next;
        } else if (IS_SCHEMA(child, "sequence")) {
            type->subtypes = (xmlSchemaTypePtr)
		xmlSchemaParseModelGroup(ctxt, schema, child,
		    XML_SCHEMA_TYPE_SEQUENCE, 1);
            child = child->next;
        } else if (IS_SCHEMA(child, "group")) {
            type->subtypes = (xmlSchemaTypePtr)
		xmlSchemaParseModelGroupDefRef(ctxt, schema, child);
            child = child->next;
        }
	/*
	* Parse attribute decls/refs.
	*/
        child = xmlSchemaParseAttrDecls(ctxt, schema, child, type);
	/*
	* Parse attribute wildcard.
	*/
	if (IS_SCHEMA(child, "anyAttribute")) {
	    type->attributeWildcard = xmlSchemaParseAnyAttribute(ctxt, schema, child);
	    child = child->next;
	}
    }
    if (child != NULL) {
	xmlSchemaPContentErr(ctxt,
	    XML_SCHEMAP_S4S_ELEM_NOT_ALLOWED,
	    &des, type, node, child,
	    NULL, "(annotation?, (simpleContent | complexContent | "
	    "((group | all | choice | sequence)?, ((attribute | "
	    "attributeGroup)*, anyAttribute?))))");
    }
    FREE_AND_NULL(des);
    ctxt->container = oldcontainer;
    ctxt->ctxtType = ctxtType;
    return (type);
}

/**
 * xmlSchemaParseSchema:
 * @ctxt:  a schema validation context
 * @node:  a subtree containing XML Schema informations
 *
 * parse a XML schema definition from a node set
 * *WARNING* this interface is highly subject to change
 *
 * Returns the internal XML Schema structure built from the resource or
 *         NULL in case of error
 */
static xmlSchemaPtr
xmlSchemaParseSchema(xmlSchemaParserCtxtPtr ctxt, xmlNodePtr node)
{
    xmlSchemaPtr schema = NULL;
    const xmlChar *val;
    int nberrors;
    xmlAttrPtr attr;

    /*
    * This one is called by xmlSchemaParse only and is used if
    * the schema to be parsed was specified via the API; i.e. not
    * automatically by the validated instance document.
    */
    if ((ctxt == NULL) || (node == NULL))
        return (NULL);
    nberrors = ctxt->nberrors;
    ctxt->nberrors = 0;
    ctxt->isS4S = 0;
    if (IS_SCHEMA(node, "schema")) {
	xmlSchemaImportPtr import;

        schema = xmlSchemaNewSchema(ctxt);
        if (schema == NULL)
            return (NULL);
	attr = xmlSchemaGetPropNode(node, "targetNamespace");
	if (attr != NULL) {
	    xmlSchemaPValAttrNode(ctxt, NULL, NULL, attr,
		xmlSchemaGetBuiltInType(XML_SCHEMAS_ANYURI), &val);
	    /*
	    * TODO: Should we proceed with an invalid target namespace?
	    */
	    schema->targetNamespace = xmlDictLookup(ctxt->dict, val, -1);
	    if (xmlStrEqual(schema->targetNamespace, xmlSchemaNs)) {
		/*
		* We are parsing the schema for schema!
		*/
		ctxt->isS4S = 1;
	    }
	} else {
	    schema->targetNamespace = NULL;
	}
	/*
	* Add the current ns name and location to the import table;
	* this is needed to have a consistent mechanism, regardless
	* if all schemata are constructed dynamically fired by the
	* instance or if the schema to be used was specified via
	* the API.
	*/
	import = xmlSchemaAddImport(ctxt, &(schema->schemasImports),
	    schema->targetNamespace);
	if (import == NULL) {
	    xmlSchemaPCustomErr(ctxt, XML_SCHEMAP_FAILED_BUILD_IMPORT,
		NULL, NULL, (xmlNodePtr) ctxt->doc,
		"Internal error: xmlSchemaParseSchema, "
		"failed to add an import entry", NULL);
	    xmlSchemaFree(schema);
	    schema = NULL;
	    return (NULL);
	}
	import->schemaLocation = ctxt->URL;
	/*
	* NOTE: We won't set the doc here, otherwise it will be freed
	* if the import struct is freed.
	* import->doc = ctxt->doc;
	*/
	xmlSchemaParseSchemaDefaults(ctxt, schema, node);
        xmlSchemaParseSchemaTopLevel(ctxt, schema, node->children);
    } else {
        xmlDocPtr doc;

	doc = node->doc;

        if ((doc != NULL) && (doc->URL != NULL)) {
	    xmlSchemaPErr(ctxt, (xmlNodePtr) doc,
		      XML_SCHEMAP_NOT_SCHEMA,
		      "The file \"%s\" is not a XML schema.\n", doc->URL, NULL);
	} else {
	    xmlSchemaPErr(ctxt, (xmlNodePtr) doc,
		      XML_SCHEMAP_NOT_SCHEMA,
		      "The file is not a XML schema.\n", NULL, NULL);
	}
	return(NULL);
    }
    if (ctxt->nberrors != 0) {
        if (schema != NULL) {
            xmlSchemaFree(schema);
            schema = NULL;
        }
    }
    if (schema != NULL)
	schema->counter = ctxt->counter;
    ctxt->nberrors = nberrors;
#ifdef DEBUG
    if (schema == NULL)
        xmlGenericError(xmlGenericErrorContext,
                        "xmlSchemaParse() failed\n");
#endif
    return (schema);
}

/************************************************************************
 * 									*
 * 			Validating using Schemas			*
 * 									*
 ************************************************************************/

/************************************************************************
 * 									*
 * 			Reading/Writing Schemas				*
 * 									*
 ************************************************************************/

#if 0 /* Will be enabled if it is clear what options are needed. */
/**
 * xmlSchemaParserCtxtSetOptions:
 * @ctxt:	a schema parser context
 * @options: a combination of xmlSchemaParserOption
 *
 * Sets the options to be used during the parse.
 *
 * Returns 0 in case of success, -1 in case of an
 * API error.
 */
static int
xmlSchemaParserCtxtSetOptions(xmlSchemaParserCtxtPtr ctxt,
			      int options)

{
    int i;

    if (ctxt == NULL)
	return (-1);
    /*
    * WARNING: Change the start value if adding to the
    * xmlSchemaParseOption.
    */
    for (i = 1; i < (int) sizeof(int) * 8; i++) {
        if (options & 1<<i) {
	    return (-1);
        }
    }
    ctxt->options = options;
    return (0);
}

/**
 * xmlSchemaValidCtxtGetOptions:
 * @ctxt: a schema parser context
 *
 * Returns the option combination of the parser context.
 */
static int
xmlSchemaParserCtxtGetOptions(xmlSchemaParserCtxtPtr ctxt)

{
    if (ctxt == NULL)
	return (-1);
    else
	return (ctxt->options);
}
#endif

/**
 * xmlSchemaNewParserCtxt:
 * @URL:  the location of the schema
 *
 * Create an XML Schemas parse context for that file/resource expected
 * to contain an XML Schemas file.
 *
 * Returns the parser context or NULL in case of error
 */
xmlSchemaParserCtxtPtr
xmlSchemaNewParserCtxt(const char *URL)
{
    xmlSchemaParserCtxtPtr ret;

    if (URL == NULL)
        return (NULL);

    ret = (xmlSchemaParserCtxtPtr) xmlMalloc(sizeof(xmlSchemaParserCtxt));
    if (ret == NULL) {
        xmlSchemaPErrMemory(NULL, "allocating schema parser context",
                            NULL);
        return (NULL);
    }
    memset(ret, 0, sizeof(xmlSchemaParserCtxt));
    ret->type = XML_SCHEMA_CTXT_PARSER;
    ret->dict = xmlDictCreate();
    ret->URL = xmlDictLookup(ret->dict, (const xmlChar *) URL, -1);
    ret->includes = 0;
    return (ret);
}

/**
 * xmlSchemaNewMemParserCtxt:
 * @buffer:  a pointer to a char array containing the schemas
 * @size:  the size of the array
 *
 * Create an XML Schemas parse context for that memory buffer expected
 * to contain an XML Schemas file.
 *
 * Returns the parser context or NULL in case of error
 */
xmlSchemaParserCtxtPtr
xmlSchemaNewMemParserCtxt(const char *buffer, int size)
{
    xmlSchemaParserCtxtPtr ret;

    if ((buffer == NULL) || (size <= 0))
        return (NULL);

    ret = (xmlSchemaParserCtxtPtr) xmlMalloc(sizeof(xmlSchemaParserCtxt));
    if (ret == NULL) {
        xmlSchemaPErrMemory(NULL, "allocating schema parser context",
                            NULL);
        return (NULL);
    }
    memset(ret, 0, sizeof(xmlSchemaParserCtxt));
    ret->buffer = buffer;
    ret->size = size;
    ret->dict = xmlDictCreate();
    return (ret);
}

/**
 * xmlSchemaNewDocParserCtxt:
 * @doc:  a preparsed document tree
 *
 * Create an XML Schemas parse context for that document.
 * NB. The document may be modified during the parsing process.
 *
 * Returns the parser context or NULL in case of error
 */
xmlSchemaParserCtxtPtr
xmlSchemaNewDocParserCtxt(xmlDocPtr doc)
{
    xmlSchemaParserCtxtPtr ret;

    if (doc == NULL)
      return (NULL);

    ret = (xmlSchemaParserCtxtPtr) xmlMalloc(sizeof(xmlSchemaParserCtxt));
    if (ret == NULL) {
      xmlSchemaPErrMemory(NULL, "allocating schema parser context",
			  NULL);
      return (NULL);
    }
    memset(ret, 0, sizeof(xmlSchemaParserCtxt));
    ret->doc = doc;
    ret->dict = xmlDictCreate();
    /* The application has responsibility for the document */
    ret->preserve = 1;

    return (ret);
}

/**
 * xmlSchemaFreeParserCtxt:
 * @ctxt:  the schema parser context
 *
 * Free the resources associated to the schema parser context
 */
void
xmlSchemaFreeParserCtxt(xmlSchemaParserCtxtPtr ctxt)
{
    if (ctxt == NULL)
        return;
    if (ctxt->doc != NULL && !ctxt->preserve)
        xmlFreeDoc(ctxt->doc);
    if (ctxt->assemble != NULL) {
	xmlFree((xmlSchemaTypePtr *) ctxt->assemble->items);
	xmlFree(ctxt->assemble);
    }
    if (ctxt->vctxt != NULL) {
	xmlSchemaFreeValidCtxt(ctxt->vctxt);
    }
    if (ctxt->localImports != NULL)
	xmlFree((xmlChar *) ctxt->localImports);
    if (ctxt->substGroups != NULL)
	xmlHashFree(ctxt->substGroups,
	    (xmlHashDeallocator) xmlSchemaFreeSubstGroup);
    xmlDictFree(ctxt->dict);
    xmlFree(ctxt);
}

/************************************************************************
 *									*
 *			Building the content models			*
 *									*
 ************************************************************************/

static void
xmlSchemaBuildContentModelForSubstGroup(xmlSchemaParserCtxtPtr pctxt,
					xmlSchemaParticlePtr particle)
{
    xmlAutomataStatePtr start;
    xmlSchemaElementPtr elemDecl, member;
    xmlAutomataStatePtr end;
    xmlSchemaSubstGroupPtr substGroup;
    int i;

    elemDecl = (xmlSchemaElementPtr) particle->children;
    /*
    * Wrap the substitution group with a CHOICE.
    */
    start = pctxt->state;
    end = xmlAutomataNewState(pctxt->am);
    substGroup = xmlSchemaGetElementSubstitutionGroup(pctxt, elemDecl);
    if (substGroup == NULL) {
	xmlSchemaPErr(pctxt, GET_NODE(particle),
	    XML_SCHEMAP_INTERNAL,
	    "Internal error: xmlSchemaBuildContentModelForSubstGroup, "
	    "declaration is marked having a subst. group but none "
	    "available.\n", elemDecl->name, NULL);
	return;
    }
    if (particle->maxOccurs == 1) {
	/*
	* NOTE that we put the declaration in, even if it's abstract,
	*/
	xmlAutomataNewEpsilon(pctxt->am,
	    xmlAutomataNewTransition2(pctxt->am,
	    start, NULL,
	    elemDecl->name, elemDecl->targetNamespace, elemDecl), end);
	/*
	* Add subst. group members.
	*/
	for (i = 0; i < substGroup->members->nbItems; i++) {
	    member = (xmlSchemaElementPtr) substGroup->members->items[i];
	    xmlAutomataNewEpsilon(pctxt->am,
		xmlAutomataNewTransition2(pctxt->am,
		start, NULL,
		member->name, member->targetNamespace, member),
		end);
	}
    } else {
	int counter;
	xmlAutomataStatePtr hop;
	int maxOccurs = particle->maxOccurs == UNBOUNDED ?
	    UNBOUNDED : particle->maxOccurs - 1;
	int minOccurs = particle->minOccurs < 1 ? 0 : particle->minOccurs - 1;

	counter =
	    xmlAutomataNewCounter(pctxt->am, minOccurs,
	    maxOccurs);
	hop = xmlAutomataNewState(pctxt->am);

	xmlAutomataNewEpsilon(pctxt->am,
	    xmlAutomataNewTransition2(pctxt->am,
	    start, NULL,
	    elemDecl->name, elemDecl->targetNamespace, elemDecl),
	    hop);
	/*
	* Add subst. group members.
	*/
	for (i = 0; i < substGroup->members->nbItems; i++) {
	    member = (xmlSchemaElementPtr) substGroup->members->items[i];
	    xmlAutomataNewEpsilon(pctxt->am,
		xmlAutomataNewTransition2(pctxt->am,
		start, NULL,
		member->name, member->targetNamespace, member),
		hop);
	}
	xmlAutomataNewCountedTrans(pctxt->am, hop, start, counter);
	xmlAutomataNewCounterTrans(pctxt->am, hop, end, counter);
    }
    if (particle->minOccurs == 0)
	xmlAutomataNewEpsilon(pctxt->am, start, end);
    pctxt->state = end;
}

static void
xmlSchemaBuildContentModelForElement(xmlSchemaParserCtxtPtr ctxt,
				     xmlSchemaParticlePtr particle)
{
    if (((xmlSchemaElementPtr) particle->children)->flags &
	XML_SCHEMAS_ELEM_SUBST_GROUP_HEAD) {
	/*
	* Substitution groups.
	*/
	xmlSchemaBuildContentModelForSubstGroup(ctxt, particle);
    } else {
	xmlSchemaElementPtr elemDecl;
	xmlAutomataStatePtr start;

	elemDecl = (xmlSchemaElementPtr) particle->children;

	if (elemDecl->flags & XML_SCHEMAS_ELEM_ABSTRACT)
	    return;
	if (particle->maxOccurs == 1) {
	    start = ctxt->state;
	    ctxt->state = xmlAutomataNewTransition2(ctxt->am, start, NULL,
		elemDecl->name, elemDecl->targetNamespace, elemDecl);
	} else if ((particle->maxOccurs >= UNBOUNDED) && (particle->minOccurs < 2)) {
	    /* Special case. */
	    start = ctxt->state;
	    ctxt->state = xmlAutomataNewTransition2(ctxt->am, start, NULL,
		elemDecl->name, elemDecl->targetNamespace, elemDecl);
	    xmlAutomataNewEpsilon(ctxt->am, ctxt->state, start);
	} else {
	    int counter;
	    int maxOccurs = particle->maxOccurs == UNBOUNDED ?
			    UNBOUNDED : particle->maxOccurs - 1;
	    int minOccurs = particle->minOccurs < 1 ?
			    0 : particle->minOccurs - 1;

	    start = xmlAutomataNewEpsilon(ctxt->am, ctxt->state, NULL);
	    counter = xmlAutomataNewCounter(ctxt->am, minOccurs, maxOccurs);
	    ctxt->state = xmlAutomataNewTransition2(ctxt->am, start, NULL,
		elemDecl->name, elemDecl->targetNamespace, elemDecl);
	    xmlAutomataNewCountedTrans(ctxt->am, ctxt->state, start, counter);
	    ctxt->state = xmlAutomataNewCounterTrans(ctxt->am, ctxt->state,
		NULL, counter);
	}
	if (particle->minOccurs == 0)
	    xmlAutomataNewEpsilon(ctxt->am, start, ctxt->state);
    }
}

/**
 * xmlSchemaBuildAContentModel:
 * @ctxt:  the schema parser context
 * @particle:  the particle component
 * @name:  the complex type's name whose content is being built
 *
 * Generate the automata sequence needed for that type
 */
static void
xmlSchemaBuildAContentModel(xmlSchemaParserCtxtPtr ctxt,
			    xmlSchemaParticlePtr particle,
                            const xmlChar * name)
{
    if (particle == NULL) {
	xmlSchemaPErr(ctxt, NULL,
	    XML_SCHEMAP_INTERNAL,
	    "Internal error: xmlSchemaBuildAContentModel, "
	    "particle is NULL.\n", NULL, NULL);
	return;
    }
    if (particle->children == NULL) {
	xmlSchemaPErr(ctxt, GET_NODE(particle),
	    XML_SCHEMAP_INTERNAL,
	    "Internal error: xmlSchemaBuildAContentModel, "
	    "no term on particle.\n", NULL, NULL);
	return;
    }

    switch (particle->children->type) {
	case XML_SCHEMA_TYPE_ANY: {
	    xmlAutomataStatePtr start, end;
	    xmlSchemaWildcardPtr wild;
	    xmlSchemaWildcardNsPtr ns;

	    wild = (xmlSchemaWildcardPtr) particle->children;

	    start = ctxt->state;
	    end = xmlAutomataNewState(ctxt->am);

	    if (particle->maxOccurs == 1) {
		if (wild->any == 1) {
		    /*
		    * We need to add both transitions:
		    *
		    * 1. the {"*", "*"} for elements in a namespace.
		    */
		    ctxt->state =
			xmlAutomataNewTransition2(ctxt->am,
			start, NULL, BAD_CAST "*", BAD_CAST "*", wild);
		    xmlAutomataNewEpsilon(ctxt->am, ctxt->state, end);
		    /*
		    * 2. the {"*"} for elements in no namespace.
		    */
		    ctxt->state =
			xmlAutomataNewTransition2(ctxt->am,
			start, NULL, BAD_CAST "*", NULL, wild);
		    xmlAutomataNewEpsilon(ctxt->am, ctxt->state, end);

		} else if (wild->nsSet != NULL) {
		    ns = wild->nsSet;
		    do {
			ctxt->state = start;
			ctxt->state = xmlAutomataNewTransition2(ctxt->am,
			    ctxt->state, NULL, BAD_CAST "*", ns->value, wild);
			xmlAutomataNewEpsilon(ctxt->am, ctxt->state, end);
			ns = ns->next;
		    } while (ns != NULL);

		} else if (wild->negNsSet != NULL) {

		    /*
		    * Lead nodes with the negated namespace to the sink-state
		    * {"*", "##other"}.
		    */
		    ctxt->state = xmlAutomataNewTransition2(ctxt->am, start, NULL,
			BAD_CAST "*", wild->negNsSet->value, wild);
		    /*
		    * Open a door for nodes with any other namespace
		    * {"*", "*"}
		    */
		    ctxt->state = xmlAutomataNewTransition2(ctxt->am,
			start, NULL, BAD_CAST "*", BAD_CAST "*", wild);
		    xmlAutomataNewEpsilon(ctxt->am, ctxt->state, end);
		}
	    } else {
		int counter;
		xmlAutomataStatePtr hop;
		int maxOccurs =
		    particle->maxOccurs == UNBOUNDED ? UNBOUNDED : particle->maxOccurs - 1;
		int minOccurs =
		    particle->minOccurs < 1 ? 0 : particle->minOccurs - 1;

		counter = xmlAutomataNewCounter(ctxt->am, minOccurs, maxOccurs);
		hop = xmlAutomataNewState(ctxt->am);
		if (wild->any == 1) {
		    ctxt->state =
			xmlAutomataNewTransition2(ctxt->am,
			start, NULL, BAD_CAST "*", BAD_CAST "*", wild);
		    xmlAutomataNewEpsilon(ctxt->am, ctxt->state, hop);
		    ctxt->state =
			xmlAutomataNewTransition2(ctxt->am,
			start, NULL, BAD_CAST "*", NULL, wild);
		    xmlAutomataNewEpsilon(ctxt->am, ctxt->state, hop);
		} else if (wild->nsSet != NULL) {
		    ns = wild->nsSet;
		    do {
			ctxt->state =
			    xmlAutomataNewTransition2(ctxt->am,
				start, NULL, BAD_CAST "*", ns->value, wild);
			xmlAutomataNewEpsilon(ctxt->am, ctxt->state, hop);
			ns = ns->next;
		    } while (ns != NULL);

		} else if (wild->negNsSet != NULL) {
		    xmlAutomataStatePtr deadEnd;

		    deadEnd = xmlAutomataNewState(ctxt->am);
		    ctxt->state = xmlAutomataNewTransition2(ctxt->am,
			start, deadEnd, BAD_CAST "*", wild->negNsSet->value, wild);
		    ctxt->state = xmlAutomataNewTransition2(ctxt->am,
			start, NULL, BAD_CAST "*", BAD_CAST "*", wild);
		    xmlAutomataNewEpsilon(ctxt->am, ctxt->state, hop);
		}
		xmlAutomataNewCountedTrans(ctxt->am, hop, start, counter);
		xmlAutomataNewCounterTrans(ctxt->am, hop, end, counter);
	    }
	    if (particle->minOccurs == 0) {
		xmlAutomataNewEpsilon(ctxt->am, start, end);
	    }
	    ctxt->state = end;
            break;
	}
        case XML_SCHEMA_TYPE_ELEMENT:
	    xmlSchemaBuildContentModelForElement(ctxt, particle);
	    break;
        case XML_SCHEMA_TYPE_SEQUENCE:{
                xmlSchemaTreeItemPtr sub;

                /*
                 * If max and min occurances are default (1) then
                 * simply iterate over the particles of the <sequence>.
                 */
                if ((particle->minOccurs == 1) && (particle->maxOccurs == 1)) {
                    sub = particle->children->children;
                    while (sub != NULL) {
                        xmlSchemaBuildAContentModel(ctxt,
			    (xmlSchemaParticlePtr) sub, name);
                        sub = sub->next;
                    }
                } else {
                    xmlAutomataStatePtr oldstate = ctxt->state;

                    if (particle->maxOccurs >= UNBOUNDED) {
                        if (particle->minOccurs > 1) {
                            xmlAutomataStatePtr tmp;
                            int counter;

                            ctxt->state = xmlAutomataNewEpsilon(ctxt->am,
				oldstate, NULL);
                            oldstate = ctxt->state;

                            counter = xmlAutomataNewCounter(ctxt->am,
				particle->minOccurs - 1, UNBOUNDED);

                            sub = particle->children->children;
                            while (sub != NULL) {
                                xmlSchemaBuildAContentModel(ctxt,
				    (xmlSchemaParticlePtr) sub, name);
                                sub = sub->next;
                            }
                            tmp = ctxt->state;
                            xmlAutomataNewCountedTrans(ctxt->am, tmp,
                                                       oldstate, counter);
                            ctxt->state =
                                xmlAutomataNewCounterTrans(ctxt->am, tmp,
                                                           NULL, counter);

                        } else {
			    sub = particle->children->children;
                            while (sub != NULL) {
                                xmlSchemaBuildAContentModel(ctxt,
				    (xmlSchemaParticlePtr) sub, name);
                                sub = sub->next;
                            }
                            xmlAutomataNewEpsilon(ctxt->am, ctxt->state,
                                                  oldstate);
                            if (particle->minOccurs == 0) {
                                xmlAutomataNewEpsilon(ctxt->am,
				    oldstate, ctxt->state);
                            }
                        }
                    } else if ((particle->maxOccurs > 1)
                               || (particle->minOccurs > 1)) {
                        xmlAutomataStatePtr tmp;
                        int counter;

                        ctxt->state = xmlAutomataNewEpsilon(ctxt->am,
			    oldstate, NULL);
                        oldstate = ctxt->state;

                        counter = xmlAutomataNewCounter(ctxt->am,
			    particle->minOccurs - 1,
			    particle->maxOccurs - 1);

                        sub = particle->children->children;
                        while (sub != NULL) {
                            xmlSchemaBuildAContentModel(ctxt,
				(xmlSchemaParticlePtr) sub, name);
                            sub = sub->next;
                        }
                        tmp = ctxt->state;
                        xmlAutomataNewCountedTrans(ctxt->am,
			    tmp, oldstate, counter);
                        ctxt->state =
                            xmlAutomataNewCounterTrans(ctxt->am, tmp, NULL,
                                                       counter);
                        if (particle->minOccurs == 0) {
                            xmlAutomataNewEpsilon(ctxt->am,
				oldstate, ctxt->state);
                        }
                    } else {
                        sub = particle->children->children;
                        while (sub != NULL) {
                            xmlSchemaBuildAContentModel(ctxt,
				(xmlSchemaParticlePtr) sub, name);
                            sub = sub->next;
                        }
                        if (particle->minOccurs == 0) {
                            xmlAutomataNewEpsilon(ctxt->am, oldstate,
                                                  ctxt->state);
                        }
                    }
                }
                break;
            }
        case XML_SCHEMA_TYPE_CHOICE:{
                xmlSchemaTreeItemPtr sub;
                xmlAutomataStatePtr start, end;

                start = ctxt->state;
                end = xmlAutomataNewState(ctxt->am);

                /*
                 * iterate over the subtypes and remerge the end with an
                 * epsilon transition
                 */
                if (particle->maxOccurs == 1) {
		    sub = particle->children->children;
                    while (sub != NULL) {
                        ctxt->state = start;
                        xmlSchemaBuildAContentModel(ctxt,
			    (xmlSchemaParticlePtr) sub, name);
                        xmlAutomataNewEpsilon(ctxt->am, ctxt->state, end);
                        sub = sub->next;
                    }
                } else {
                    int counter;
                    xmlAutomataStatePtr hop;
                    int maxOccurs = particle->maxOccurs == UNBOUNDED ?
                        UNBOUNDED : particle->maxOccurs - 1;
                    int minOccurs =
                        particle->minOccurs < 1 ? 0 : particle->minOccurs - 1;

                    /*
                     * use a counter to keep track of the number of transtions
                     * which went through the choice.
                     */
                    counter =
                        xmlAutomataNewCounter(ctxt->am, minOccurs,
                                              maxOccurs);
                    hop = xmlAutomataNewState(ctxt->am);

		    sub = particle->children->children;
                    while (sub != NULL) {
                        ctxt->state = start;
                        xmlSchemaBuildAContentModel(ctxt,
			    (xmlSchemaParticlePtr) sub, name);
                        xmlAutomataNewEpsilon(ctxt->am, ctxt->state, hop);
                        sub = sub->next;
                    }
                    xmlAutomataNewCountedTrans(ctxt->am, hop, start,
                                               counter);
                    xmlAutomataNewCounterTrans(ctxt->am, hop, end,
                                               counter);
                }
                if (particle->minOccurs == 0) {
                    xmlAutomataNewEpsilon(ctxt->am, start, end);
                }
                ctxt->state = end;
                break;
            }
        case XML_SCHEMA_TYPE_ALL:{
                xmlAutomataStatePtr start;
		xmlSchemaParticlePtr sub;
		xmlSchemaElementPtr elemDecl;
                int lax;

		sub = (xmlSchemaParticlePtr) particle->children->children;
                if (sub == NULL)
                    break;
                start = ctxt->state;
                while (sub != NULL) {
                    ctxt->state = start;

		    elemDecl = (xmlSchemaElementPtr) sub->children;
		    if (elemDecl == NULL) {
			xmlSchemaPErr(ctxt, NULL,
			    XML_SCHEMAP_INTERNAL,
			    "Internal error: xmlSchemaBuildAContentModel, "
			    "<element> particle a NULL term.\n", NULL, NULL);
			return;
		    };
		    /*
		    * NOTE: The {max occurs} of all the particles in the
		    * {particles} of the group must be 0 or 1; this is
		    * already ensured during the parse of the content of
		    * <all>.
		    */
                    if ((sub->minOccurs == 1) &&
			(sub->maxOccurs == 1)) {
                        xmlAutomataNewOnceTrans2(ctxt->am, ctxt->state,
                                                ctxt->state,
						elemDecl->name,
						elemDecl->targetNamespace,
						1, 1, elemDecl);
                    } else if ((sub->minOccurs == 0) &&
			(sub->maxOccurs == 1)) {

                        xmlAutomataNewCountTrans2(ctxt->am, ctxt->state,
                                                 ctxt->state,
						 elemDecl->name,
						 elemDecl->targetNamespace,
                                                 0,
                                                 1,
                                                 elemDecl);
                    }
                    sub = (xmlSchemaParticlePtr) sub->next;
                }
                lax = particle->minOccurs == 0;
                ctxt->state =
                    xmlAutomataNewAllTrans(ctxt->am, ctxt->state, NULL, lax);
                break;
            }
        default:
            xmlGenericError(xmlGenericErrorContext,
		"Internal error: xmlSchemaBuildAContentModel, found "
		"unexpected term of type %d in content model of complex "
		"type '%s'.\n",
		particle->children->type, name);
            return;
    }
}

/**
 * xmlSchemaBuildContentModel:
 * @ctxt:  the schema parser context
 * @type:  the complex type definition
 * @name:  the element name
 *
 * Builds the content model of the complex type.
 */
static void
xmlSchemaBuildContentModel(xmlSchemaTypePtr type,
			   xmlSchemaParserCtxtPtr ctxt,
                           const xmlChar * name)
{
    xmlAutomataStatePtr start;

    if ((type->type != XML_SCHEMA_TYPE_COMPLEX) ||
	(type->contModel != NULL) ||
	((type->contentType != XML_SCHEMA_CONTENT_ELEMENTS) &&
	(type->contentType != XML_SCHEMA_CONTENT_MIXED)))
	return;

#ifdef DEBUG_CONTENT
    xmlGenericError(xmlGenericErrorContext,
                    "Building content model for %s\n", name);
#endif

    ctxt->am = xmlNewAutomata();
    if (ctxt->am == NULL) {
        xmlGenericError(xmlGenericErrorContext,
	    "Cannot create automata for complex type %s\n", name);
        return;
    }
    start = ctxt->state = xmlAutomataGetInitState(ctxt->am);
    xmlSchemaBuildAContentModel(ctxt, (xmlSchemaParticlePtr) type->subtypes, name);
    xmlAutomataSetFinalState(ctxt->am, ctxt->state);
    type->contModel = xmlAutomataCompile(ctxt->am);
    if (type->contModel == NULL) {
        xmlSchemaPCustomErr(ctxt,
	    XML_SCHEMAP_INTERNAL,
	    NULL, type, type->node,
	    "Failed to compile the content model", NULL);
    } else if (xmlRegexpIsDeterminist(type->contModel) != 1) {
        xmlSchemaPCustomErr(ctxt,
	    XML_SCHEMAP_NOT_DETERMINISTIC,
	    /* XML_SCHEMAS_ERR_NOTDETERMINIST, */
	    NULL, type, type->node,
	    "The content model is not determinist", NULL);
    } else {
#ifdef DEBUG_CONTENT_REGEXP
        xmlGenericError(xmlGenericErrorContext,
                        "Content model of %s:\n", type->name);
        xmlRegexpPrint(stderr, type->contModel);
#endif
    }
    ctxt->state = NULL;
    xmlFreeAutomata(ctxt->am);
    ctxt->am = NULL;
}

/**
 * xmlSchemaElementFixup:
 * @elem:  the schema element context
 * @ctxt:  the schema parser context
 *
 * Resolves the references of an element declaration
 * or particle, which has an element declaration as it's
 * term.
 */
static void
xmlSchemaElementFixup(xmlSchemaElementPtr elemDecl,
                          xmlSchemaParserCtxtPtr ctxt,
                          const xmlChar * name ATTRIBUTE_UNUSED,
                          const xmlChar * context ATTRIBUTE_UNUSED,
                          const xmlChar * namespace ATTRIBUTE_UNUSED)
{
    if ((ctxt == NULL) || (elemDecl == NULL) ||
	((elemDecl != NULL) &&
	(elemDecl->flags & XML_SCHEMAS_ELEM_INTERNAL_RESOLVED)))
        return;
    elemDecl->flags |= XML_SCHEMAS_ELEM_INTERNAL_RESOLVED;

    if ((elemDecl->subtypes == NULL) && (elemDecl->namedType != NULL)) {
	xmlSchemaTypePtr type;

	/* (type definition) ... otherwise the type definition ·resolved·
	* to by the ·actual value· of the type [attribute] ...
	*/
	type = xmlSchemaGetType(ctxt->schema, elemDecl->namedType,
	    elemDecl->namedTypeNs);
	if (type == NULL) {
	    xmlSchemaPResCompAttrErr(ctxt,
		XML_SCHEMAP_SRC_RESOLVE,
		(xmlSchemaTypePtr) elemDecl, elemDecl->node,
		"type", elemDecl->namedType, elemDecl->namedTypeNs,
		XML_SCHEMA_TYPE_BASIC, "type definition");
	} else
	    elemDecl->subtypes = type;
    }
    if (elemDecl->substGroup != NULL) {
	xmlSchemaElementPtr substHead;

	/*
	* FIXME TODO: Do we need a new field in _xmlSchemaElement for
	* substitutionGroup?
	*/
	substHead = xmlSchemaGetElem(ctxt->schema, elemDecl->substGroup,
	    elemDecl->substGroupNs);
	if (substHead == NULL) {
	    xmlSchemaPResCompAttrErr(ctxt,
		XML_SCHEMAP_SRC_RESOLVE,
		(xmlSchemaTypePtr) elemDecl, NULL,
		"substitutionGroup", elemDecl->substGroup,
		elemDecl->substGroupNs, XML_SCHEMA_TYPE_ELEMENT, NULL);
	} else {
	    xmlSchemaElementFixup(substHead, ctxt, NULL, NULL, NULL);
	    /*
	    * Set the "substitution group affiliation".
	    * NOTE that now we use the "refDecl" field for this.
	    */
	    elemDecl->refDecl = substHead;
	    /*
	    * (type definition)...otherwise the {type definition} of the
	    * element declaration ·resolved· to by the ·actual value· of
	    * the substitutionGroup [attribute], if present
	    */
	    if (elemDecl->subtypes == NULL)
		elemDecl->subtypes = substHead->subtypes;
	}
    }
    if ((elemDecl->subtypes == NULL) && (elemDecl->namedType == NULL) &&
	(elemDecl->substGroup == NULL))
	elemDecl->subtypes = xmlSchemaGetBuiltInType(XML_SCHEMAS_ANYTYPE);
}

/**
 * xmlSchemaResolveUnionMemberTypes:
 * @ctxt:  the schema parser context
 * @type:  the schema simple type definition
 *
 * Checks and builds the "member type definitions" property of the union
 * simple type. This handles part (1), part (2) is done in
 * xmlSchemaFinishMemberTypeDefinitionsProperty()
 *
 * Returns -1 in case of an internal error, 0 otherwise.
 */
static int
xmlSchemaResolveUnionMemberTypes(xmlSchemaParserCtxtPtr ctxt,
				 xmlSchemaTypePtr type)
{

    xmlSchemaTypeLinkPtr link, lastLink, newLink;
    xmlSchemaTypePtr memberType;

    /*
    * SPEC (1) "If the <union> alternative is chosen, then [Definition:]
    * define the explicit members as the type definitions ·resolved·
    * to by the items in the ·actual value· of the memberTypes [attribute],
    * if any, followed by the type definitions corresponding to the
    * <simpleType>s among the [children] of <union>, if any."
    */
    /*
    * Resolve references.
    */
    link = type->memberTypes;
    lastLink = NULL;
    while (link != NULL) {
	const xmlChar *name, *nsName;

	name = ((xmlSchemaQNameRefPtr) link->type)->name;
	nsName = ((xmlSchemaQNameRefPtr) link->type)->targetNamespace;

	memberType = xmlSchemaGetType(ctxt->schema, name, nsName);
	if ((memberType == NULL) || (! IS_SIMPLE_TYPE(memberType))) {
	    xmlSchemaPResCompAttrErr(ctxt, XML_SCHEMAP_SRC_RESOLVE,
		type, type->node, "memberTypes",
		name, nsName, XML_SCHEMA_TYPE_SIMPLE, NULL);
	    /*
	    * Remove the member type link.
	    */
	    if (lastLink == NULL)
		type->memberTypes = link->next;
	    else
		lastLink->next = link->next;
	    newLink = link;
	    link = link->next;
	    xmlFree(newLink);
	} else {
	    link->type = memberType;
	    lastLink = link;
	    link = link->next;
	}
    }
    /*
    * Add local simple types,
    */
    memberType = type->subtypes;
    while (memberType != NULL) {
	link = (xmlSchemaTypeLinkPtr) xmlMalloc(sizeof(xmlSchemaTypeLink));
	if (link == NULL) {
	    xmlSchemaPErrMemory(ctxt, "allocating a type link", NULL);
	    return (-1);
	}
	link->type = memberType;
	link->next = NULL;
	if (lastLink == NULL)
	    type->memberTypes = link;
	else
	    lastLink->next = link;
	lastLink = link;
	memberType = memberType->next;
    }
    return (0);
}

/**
 * xmlSchemaIsDerivedFromBuiltInType:
 * @ctxt:  the schema parser context
 * @type:  the type definition
 * @valType: the value type
 *
 *
 * Returns 1 if the type has the given value type, or
 * is derived from such a type.
 */
static int
xmlSchemaIsDerivedFromBuiltInType(xmlSchemaTypePtr type, int valType)
{
    if (type == NULL)
	return (0);
    if (IS_COMPLEX_TYPE(type))
	return (0);
    if (type->type == XML_SCHEMA_TYPE_BASIC) {
	if (type->builtInType == valType)
	    return(1);
	if ((type->builtInType == XML_SCHEMAS_ANYSIMPLETYPE) ||
	    (type->builtInType == XML_SCHEMAS_ANYTYPE))
	    return (0);
	return(xmlSchemaIsDerivedFromBuiltInType(type->subtypes, valType));
    } else
	return(xmlSchemaIsDerivedFromBuiltInType(type->subtypes, valType));

    return (0);
}

#if 0
/**
 * xmlSchemaIsDerivedFromBuiltInType:
 * @ctxt:  the schema parser context
 * @type:  the type definition
 * @valType: the value type
 *
 *
 * Returns 1 if the type has the given value type, or
 * is derived from such a type.
 */
static int
xmlSchemaIsUserDerivedFromBuiltInType(xmlSchemaTypePtr type, int valType)
{
    if (type == NULL)
	return (0);
    if (IS_COMPLEX_TYPE(type))
	return (0);
    if (type->type == XML_SCHEMA_TYPE_BASIC) {
	if (type->builtInType == valType)
	    return(1);
	return (0);
    } else
	return(xmlSchemaIsDerivedFromBuiltInType(type->subtypes, valType));

    return (0);
}
#endif

static xmlSchemaTypePtr
xmlSchemaQueryBuiltInType(xmlSchemaTypePtr type)
{
    if (type == NULL)
	return (NULL);
    if (IS_COMPLEX_TYPE(type))
	return (NULL);
    if (type->type == XML_SCHEMA_TYPE_BASIC)
	    return(type);
    else
	return(xmlSchemaQueryBuiltInType(type->subtypes));

    return (NULL);
}

/**
 * xmlSchemaGetPrimitiveType:
 * @type:  the simpleType definition
 *
 * Returns the primitive type of the given type or
 * NULL in case of error.
 */
static xmlSchemaTypePtr
xmlSchemaGetPrimitiveType(xmlSchemaTypePtr type)
{

    while (type != NULL) {
	/*
	* Note that anySimpleType is actually not a primitive type
	* but we need that here.
	*/
	if ((type->builtInType == XML_SCHEMAS_ANYSIMPLETYPE) ||
	   (type->flags & XML_SCHEMAS_TYPE_BUILTIN_PRIMITIVE))
	    return (type);
	type = type->baseType;
    }

    return (NULL);
}

#if 0
/**
 * xmlSchemaGetBuiltInTypeAncestor:
 * @type:  the simpleType definition
 *
 * Returns the primitive type of the given type or
 * NULL in case of error.
 */
static xmlSchemaTypePtr
xmlSchemaGetBuiltInTypeAncestor(xmlSchemaTypePtr type)
{
    if (VARIETY_LIST(type) || VARIETY_UNION(type))
	return (0);
    while (type != NULL) {
	if (type->type == XML_SCHEMA_TYPE_BASIC)
	    return (type);
	type = type->baseType;
    }

    return (NULL);
}
#endif

/**
 * xmlSchemaBuildAttributeUsesOwned:
 * @ctxt:  the schema parser context
 * @type:  the complex type definition
 * @cur: the attribute declaration list
 * @lastUse: the top of the attribute use list
 *
 * Builds the attribute uses list on the given complex type.
 * This one is supposed to be called by
 * xmlSchemaBuildAttributeValidation only.
 */
static int
xmlSchemaBuildAttributeUsesOwned(xmlSchemaParserCtxtPtr ctxt,
				 xmlSchemaAttributePtr cur,
				 xmlSchemaAttributeLinkPtr *uses,
				 xmlSchemaAttributeLinkPtr *lastUse)
{
    xmlSchemaAttributeLinkPtr tmp;
    while (cur != NULL) {
	if (cur->type == XML_SCHEMA_TYPE_ATTRIBUTEGROUP) {
	    /*
	     * W3C: "2 The {attribute uses} of the attribute groups ·resolved·
	     * to by the ·actual value·s of the ref [attribute] of the
	     * <attributeGroup> [children], if any."
	     */
	    if (xmlSchemaBuildAttributeUsesOwned(ctxt,
		((xmlSchemaAttributeGroupPtr) cur)->attributes, uses,
		lastUse) == -1) {
		return (-1);
	    }
	} else {
	    /* W3C: "1 The set of attribute uses corresponding to the
	     * <attribute> [children], if any."
	     */
	    tmp = (xmlSchemaAttributeLinkPtr)
		xmlMalloc(sizeof(xmlSchemaAttributeLink));
	    if (tmp == NULL) {
		xmlSchemaPErrMemory(ctxt, "building attribute uses", NULL);
		return (-1);
	    }
	    tmp->attr = cur;
	    tmp->next = NULL;
	    if (*uses == NULL)
		*uses = tmp;
	    else
		(*lastUse)->next = tmp;
	    *lastUse = tmp;
	}
	cur = cur->next;
    }
    return (0);
}

/**
 * xmlSchemaCloneWildcardNsConstraints:
 * @ctxt:  the schema parser context
 * @dest:  the destination wildcard
 * @source: the source wildcard
 *
 * Clones the namespace constraints of source
 * and assignes them to dest.
 * Returns -1 on internal error, 0 otherwise.
 */
static int
xmlSchemaCloneWildcardNsConstraints(xmlSchemaParserCtxtPtr ctxt,
				    xmlSchemaWildcardPtr *dest,
				    xmlSchemaWildcardPtr source)
{
    xmlSchemaWildcardNsPtr cur, tmp, last;

    if ((source == NULL) || (*dest == NULL))
	return(-1);
    (*dest)->any = source->any;
    cur = source->nsSet;
    last = NULL;
    while (cur != NULL) {
	tmp = xmlSchemaNewWildcardNsConstraint(ctxt);
	if (tmp == NULL)
	    return(-1);
	tmp->value = cur->value;
	if (last == NULL)
	    (*dest)->nsSet = tmp;
	else
	    last->next = tmp;
	last = tmp;
	cur = cur->next;
    }
    if ((*dest)->negNsSet != NULL)
	xmlSchemaFreeWildcardNsSet((*dest)->negNsSet);
    if (source->negNsSet != NULL) {
	(*dest)->negNsSet = xmlSchemaNewWildcardNsConstraint(ctxt);
	if ((*dest)->negNsSet == NULL)
	    return(-1);
	(*dest)->negNsSet->value = source->negNsSet->value;
    } else
	(*dest)->negNsSet = NULL;
    return(0);
}

/**
 * xmlSchemaUnionWildcards:
 * @ctxt:  the schema parser context
 * @completeWild:  the first wildcard
 * @curWild: the second wildcard
 *
 * Unions the namespace constraints of the given wildcards.
 * @completeWild will hold the resulting union.
 * Returns a positive error code on failure, -1 in case of an
 * internal error, 0 otherwise.
 */
static int
xmlSchemaUnionWildcards(xmlSchemaParserCtxtPtr ctxt,
			    xmlSchemaWildcardPtr completeWild,
			    xmlSchemaWildcardPtr curWild)
{
    xmlSchemaWildcardNsPtr cur, curB, tmp;

    /*
    * 1 If O1 and O2 are the same value, then that value must be the
    * value.
    */
    if ((completeWild->any == curWild->any) &&
	((completeWild->nsSet == NULL) == (curWild->nsSet == NULL)) &&
	((completeWild->negNsSet == NULL) == (curWild->negNsSet == NULL))) {

	if ((completeWild->negNsSet == NULL) ||
	    (completeWild->negNsSet->value == curWild->negNsSet->value)) {

	    if (completeWild->nsSet != NULL) {
		int found = 0;

		/*
		* Check equality of sets.
		*/
		cur = completeWild->nsSet;
		while (cur != NULL) {
		    found = 0;
		    curB = curWild->nsSet;
		    while (curB != NULL) {
			if (cur->value == curB->value) {
			    found = 1;
			    break;
			}
			curB = curB->next;
		    }
		    if (!found)
			break;
		    cur = cur->next;
		}
		if (found)
		    return(0);
	    } else
		return(0);
	}
    }
    /*
    * 2 If either O1 or O2 is any, then any must be the value
    */
    if (completeWild->any != curWild->any) {
	if (completeWild->any == 0) {
	    completeWild->any = 1;
	    if (completeWild->nsSet != NULL) {
		xmlSchemaFreeWildcardNsSet(completeWild->nsSet);
		completeWild->nsSet = NULL;
	    }
	    if (completeWild->negNsSet != NULL) {
		xmlFree(completeWild->negNsSet);
		completeWild->negNsSet = NULL;
	    }
	}
	return (0);
    }
    /*
    * 3 If both O1 and O2 are sets of (namespace names or ·absent·),
    * then the union of those sets must be the value.
    */
    if ((completeWild->nsSet != NULL) && (curWild->nsSet != NULL)) {
	int found;
	xmlSchemaWildcardNsPtr start;

	cur = curWild->nsSet;
	start = completeWild->nsSet;
	while (cur != NULL) {
	    found = 0;
	    curB = start;
	    while (curB != NULL) {
		if (cur->value == curB->value) {
		    found = 1;
		    break;
		}
		curB = curB->next;
	    }
	    if (!found) {
		tmp = xmlSchemaNewWildcardNsConstraint(ctxt);
		if (tmp == NULL)
		    return (-1);
		tmp->value = cur->value;
		tmp->next = completeWild->nsSet;
		completeWild->nsSet = tmp;
	    }
	    cur = cur->next;
	}

	return(0);
    }
    /*
    * 4 If the two are negations of different values (namespace names
    * or ·absent·), then a pair of not and ·absent· must be the value.
    */
    if ((completeWild->negNsSet != NULL) &&
	(curWild->negNsSet != NULL) &&
	(completeWild->negNsSet->value != curWild->negNsSet->value)) {
	completeWild->negNsSet->value = NULL;

	return(0);
    }
    /*
     * 5.
     */
    if (((completeWild->negNsSet != NULL) &&
	(completeWild->negNsSet->value != NULL) &&
	(curWild->nsSet != NULL)) ||
	((curWild->negNsSet != NULL) &&
	(curWild->negNsSet->value != NULL) &&
	(completeWild->nsSet != NULL))) {

	int nsFound, absentFound = 0;

	if (completeWild->nsSet != NULL) {
	    cur = completeWild->nsSet;
	    curB = curWild->negNsSet;
	} else {
	    cur = curWild->nsSet;
	    curB = completeWild->negNsSet;
	}
	nsFound = 0;
	while (cur != NULL) {
	    if (cur->value == NULL)
		absentFound = 1;
	    else if (cur->value == curB->value)
		nsFound = 1;
	    if (nsFound && absentFound)
		break;
	    cur = cur->next;
	}

	if (nsFound && absentFound) {
	    /*
	    * 5.1 If the set S includes both the negated namespace
	    * name and ·absent·, then any must be the value.
	    */
	    completeWild->any = 1;
	    if (completeWild->nsSet != NULL) {
		xmlSchemaFreeWildcardNsSet(completeWild->nsSet);
		completeWild->nsSet = NULL;
	    }
	    if (completeWild->negNsSet != NULL) {
		xmlFree(completeWild->negNsSet);
		completeWild->negNsSet = NULL;
	    }
	} else if (nsFound && (!absentFound)) {
	    /*
	    * 5.2 If the set S includes the negated namespace name
	    * but not ·absent·, then a pair of not and ·absent· must
	    * be the value.
	    */
	    if (completeWild->nsSet != NULL) {
		xmlSchemaFreeWildcardNsSet(completeWild->nsSet);
		completeWild->nsSet = NULL;
	    }
	    if (completeWild->negNsSet == NULL) {
		completeWild->negNsSet = xmlSchemaNewWildcardNsConstraint(ctxt);
		if (completeWild->negNsSet == NULL)
		    return (-1);
	    }
	    completeWild->negNsSet->value = NULL;
	} else if ((!nsFound) && absentFound) {
	    /*
	    * 5.3 If the set S includes ·absent· but not the negated
	    * namespace name, then the union is not expressible.
	    */
	    xmlSchemaPErr(ctxt, completeWild->node,
		XML_SCHEMAP_UNION_NOT_EXPRESSIBLE,
		"The union of the wilcard is not expressible.\n",
		NULL, NULL);
	    return(XML_SCHEMAP_UNION_NOT_EXPRESSIBLE);
	} else if ((!nsFound) && (!absentFound)) {
	    /*
	    * 5.4 If the set S does not include either the negated namespace
	    * name or ·absent·, then whichever of O1 or O2 is a pair of not
	    * and a namespace name must be the value.
	    */
	    if (completeWild->negNsSet == NULL) {
		if (completeWild->nsSet != NULL) {
		    xmlSchemaFreeWildcardNsSet(completeWild->nsSet);
		    completeWild->nsSet = NULL;
		}
		completeWild->negNsSet = xmlSchemaNewWildcardNsConstraint(ctxt);
		if (completeWild->negNsSet == NULL)
		    return (-1);
		completeWild->negNsSet->value = curWild->negNsSet->value;
	    }
	}
	return (0);
    }
    /*
     * 6.
     */
    if (((completeWild->negNsSet != NULL) &&
	(completeWild->negNsSet->value == NULL) &&
	(curWild->nsSet != NULL)) ||
	((curWild->negNsSet != NULL) &&
	(curWild->negNsSet->value == NULL) &&
	(completeWild->nsSet != NULL))) {

	if (completeWild->nsSet != NULL) {
	    cur = completeWild->nsSet;
	} else {
	    cur = curWild->nsSet;
	}
	while (cur != NULL) {
	    if (cur->value == NULL) {
		/*
		* 6.1 If the set S includes ·absent·, then any must be the
		* value.
		*/
		completeWild->any = 1;
		if (completeWild->nsSet != NULL) {
		    xmlSchemaFreeWildcardNsSet(completeWild->nsSet);
		    completeWild->nsSet = NULL;
		}
		if (completeWild->negNsSet != NULL) {
		    xmlFree(completeWild->negNsSet);
		    completeWild->negNsSet = NULL;
		}
		return (0);
	    }
	    cur = cur->next;
	}
	if (completeWild->negNsSet == NULL) {
	    /*
	    * 6.2 If the set S does not include ·absent·, then a pair of not
	    * and ·absent· must be the value.
	    */
	    if (completeWild->nsSet != NULL) {
		xmlSchemaFreeWildcardNsSet(completeWild->nsSet);
		completeWild->nsSet = NULL;
	    }
	    completeWild->negNsSet = xmlSchemaNewWildcardNsConstraint(ctxt);
	    if (completeWild->negNsSet == NULL)
		return (-1);
	    completeWild->negNsSet->value = NULL;
	}
	return (0);
    }
    return (0);

}

/**
 * xmlSchemaIntersectWildcards:
 * @ctxt:  the schema parser context
 * @completeWild:  the first wildcard
 * @curWild: the second wildcard
 *
 * Intersects the namespace constraints of the given wildcards.
 * @completeWild will hold the resulting intersection.
 * Returns a positive error code on failure, -1 in case of an
 * internal error, 0 otherwise.
 */
static int
xmlSchemaIntersectWildcards(xmlSchemaParserCtxtPtr ctxt,
			    xmlSchemaWildcardPtr completeWild,
			    xmlSchemaWildcardPtr curWild)
{
    xmlSchemaWildcardNsPtr cur, curB, prev,  tmp;

    /*
    * 1 If O1 and O2 are the same value, then that value must be the
    * value.
    */
    if ((completeWild->any == curWild->any) &&
	((completeWild->nsSet == NULL) == (curWild->nsSet == NULL)) &&
	((completeWild->negNsSet == NULL) == (curWild->negNsSet == NULL))) {

	if ((completeWild->negNsSet == NULL) ||
	    (completeWild->negNsSet->value == curWild->negNsSet->value)) {

	    if (completeWild->nsSet != NULL) {
		int found = 0;

		/*
		* Check equality of sets.
		*/
		cur = completeWild->nsSet;
		while (cur != NULL) {
		    found = 0;
		    curB = curWild->nsSet;
		    while (curB != NULL) {
			if (cur->value == curB->value) {
			    found = 1;
			    break;
			}
			curB = curB->next;
		    }
		    if (!found)
			break;
		    cur = cur->next;
		}
		if (found)
		    return(0);
	    } else
		return(0);
	}
    }
    /*
    * 2 If either O1 or O2 is any, then the other must be the value.
    */
    if ((completeWild->any != curWild->any) && (completeWild->any)) {
	if (xmlSchemaCloneWildcardNsConstraints(ctxt, &completeWild, curWild) == -1)
	    return(-1);
	return(0);
    }
    /*
    * 3 If either O1 or O2 is a pair of not and a value (a namespace
    * name or ·absent·) and the other is a set of (namespace names or
    * ·absent·), then that set, minus the negated value if it was in
    * the set, minus ·absent· if it was in the set, must be the value.
    */
    if (((completeWild->negNsSet != NULL) && (curWild->nsSet != NULL)) ||
	((curWild->negNsSet != NULL) && (completeWild->nsSet != NULL))) {
	const xmlChar *neg;

	if (completeWild->nsSet == NULL) {
	    neg = completeWild->negNsSet->value;
	    if (xmlSchemaCloneWildcardNsConstraints(ctxt, &completeWild, curWild) == -1)
		return(-1);
	} else
	    neg = curWild->negNsSet->value;
	/*
	* Remove absent and negated.
	*/
	prev = NULL;
	cur = completeWild->nsSet;
	while (cur != NULL) {
	    if (cur->value == NULL) {
		if (prev == NULL)
		    completeWild->nsSet = cur->next;
		else
		    prev->next = cur->next;
		xmlFree(cur);
		break;
	    }
	    prev = cur;
	    cur = cur->next;
	}
	if (neg != NULL) {
	    prev = NULL;
	    cur = completeWild->nsSet;
	    while (cur != NULL) {
		if (cur->value == neg) {
		    if (prev == NULL)
			completeWild->nsSet = cur->next;
		    else
			prev->next = cur->next;
		    xmlFree(cur);
		    break;
		}
		prev = cur;
		cur = cur->next;
	    }
	}

	return(0);
    }
    /*
    * 4 If both O1 and O2 are sets of (namespace names or ·absent·),
    * then the intersection of those sets must be the value.
    */
    if ((completeWild->nsSet != NULL) && (curWild->nsSet != NULL)) {
	int found;

	cur = completeWild->nsSet;
	prev = NULL;
	while (cur != NULL) {
	    found = 0;
	    curB = curWild->nsSet;
	    while (curB != NULL) {
		if (cur->value == curB->value) {
		    found = 1;
		    break;
		}
		curB = curB->next;
	    }
	    if (!found) {
		if (prev == NULL)
		    completeWild->nsSet = cur->next;
		else
		    prev->next = cur->next;
		tmp = cur->next;
		xmlFree(cur);
		cur = tmp;
		continue;
	    }
	    prev = cur;
	    cur = cur->next;
	}

	return(0);
    }
    /* 5 If the two are negations of different namespace names,
    * then the intersection is not expressible
    */
    if ((completeWild->negNsSet != NULL) &&
	(curWild->negNsSet != NULL) &&
	(completeWild->negNsSet->value != curWild->negNsSet->value) &&
	(completeWild->negNsSet->value != NULL) &&
	(curWild->negNsSet->value != NULL)) {

	xmlSchemaPErr(ctxt, completeWild->node, XML_SCHEMAP_INTERSECTION_NOT_EXPRESSIBLE,
	    "The intersection of the wilcard is not expressible.\n",
	    NULL, NULL);
	return(XML_SCHEMAP_INTERSECTION_NOT_EXPRESSIBLE);
    }
    /*
    * 6 If the one is a negation of a namespace name and the other
    * is a negation of ·absent·, then the one which is the negation
    * of a namespace name must be the value.
    */
    if ((completeWild->negNsSet != NULL) && (curWild->negNsSet != NULL) &&
	(completeWild->negNsSet->value != curWild->negNsSet->value) &&
	(completeWild->negNsSet->value == NULL)) {
	completeWild->negNsSet->value =  curWild->negNsSet->value;
    }
    return(0);
}

/**
 * xmlSchemaIsWildcardNsConstraintSubset:
 * @ctxt:  the schema parser context
 * @sub:  the first wildcard
 * @super: the second wildcard
 *
 * Schema Component Constraint: Wildcard Subset (cos-ns-subset)
 *
 * Returns 0 if the namespace constraint of @sub is an intensional
 * subset of @super, 1 otherwise.
 */
static int
xmlSchemaCheckCOSNSSubset(xmlSchemaWildcardPtr sub,
			  xmlSchemaWildcardPtr super)
{
    /*
    * 1 super must be any.
    */
    if (super->any)
	return (0);
    /*
    * 2.1 sub must be a pair of not and a namespace name or ·absent·.
    * 2.2 super must be a pair of not and the same value.
    */
    if ((sub->negNsSet != NULL) &&
	(super->negNsSet != NULL) &&
	(sub->negNsSet->value == sub->negNsSet->value))
	return (0);
    /*
    * 3.1 sub must be a set whose members are either namespace names or ·absent·.
    */
    if (sub->nsSet != NULL) {
	/*
	* 3.2.1 super must be the same set or a superset thereof.
	*/
	if (super->nsSet != NULL) {
	    xmlSchemaWildcardNsPtr cur, curB;
	    int found = 0;

	    cur = sub->nsSet;
	    while (cur != NULL) {
		found = 0;
		curB = super->nsSet;
		while (curB != NULL) {
		    if (cur->value == curB->value) {
			found = 1;
			break;
		    }
		    curB = curB->next;
		}
		if (!found)
		    return (1);
		cur = cur->next;
	    }
	    if (found)
		return (0);
	} else if (super->negNsSet != NULL) {
	    xmlSchemaWildcardNsPtr cur;
	    /*
	    * 3.2.2 super must be a pair of not and a namespace name or
	    * ·absent· and that value must not be in sub's set.
	    */
	    cur = sub->nsSet;
	    while (cur != NULL) {
		if (cur->value == super->negNsSet->value)
		    return (1);
		cur = cur->next;
	    }
	    return (0);
	}
    }
    return (1);
}

/**
 * xmlSchemaBuildCompleteAttributeWildcard:
 * @ctxt:  the schema parser context
 * @attrs: the attribute list
 * @completeWild: the resulting complete wildcard
 *
 * Returns -1 in case of an internal error, 0 otherwise.
 */
static int
xmlSchemaBuildCompleteAttributeWildcard(xmlSchemaParserCtxtPtr ctxt,
				   xmlSchemaAttributePtr attrs,
				   xmlSchemaWildcardPtr *completeWild)
{
    while (attrs != NULL) {
	if (attrs->type == XML_SCHEMA_TYPE_ATTRIBUTEGROUP) {
	    xmlSchemaAttributeGroupPtr group;

	    group = (xmlSchemaAttributeGroupPtr) attrs;
	    /*
	    * Handle attribute group references.
	    */
	    if (group->ref != NULL) {
		if (group->refItem == NULL) {
		    /*
		    * TODO: Should we raise a warning here?
		    */
		    /*
		    * The referenced attribute group definition could not
		    * be resolved beforehand, so skip.
		    */
		    attrs = attrs->next;
		    continue;
		} else
		    group = group->refItem;
	    }
	    /*
	    * For every attribute group definition, an intersected wildcard
	    * will be created (assumed that a wildcard exists on the
	    * particular attr. gr. def. or on any contained attr. gr. def
	    * at all).
	    * The flag XML_SCHEMAS_ATTRGROUP_WILDCARD_BUILDED ensures
	    * that the intersection will be performed only once.
	    */
	    if ((group->flags & XML_SCHEMAS_ATTRGROUP_WILDCARD_BUILDED) == 0) {
		if (group->attributes != NULL) {
		    if (xmlSchemaBuildCompleteAttributeWildcard(ctxt,
			group->attributes, &group->attributeWildcard) == -1)
			return (-1);
		}
		group->flags |= XML_SCHEMAS_ATTRGROUP_WILDCARD_BUILDED;
	    }
	    if (group->attributeWildcard != NULL) {
		if (*completeWild == NULL) {
		    /*
		    * Copy the first encountered wildcard as context, except for the annotation.
		    *
		    * Although the complete wildcard might not correspond to any
		    * node in the schema, we will save this context node.
		    */
		    *completeWild = xmlSchemaAddWildcard(ctxt, ctxt->schema,
			XML_SCHEMA_TYPE_ANY_ATTRIBUTE,
			group->attributeWildcard->node);
		    if (xmlSchemaCloneWildcardNsConstraints(ctxt,
			completeWild, group->attributeWildcard) == -1)
			return (-1);
		    (*completeWild)->processContents = group->attributeWildcard->processContents;
		    (*completeWild)->node = group->attributeWildcard->node;
		} else if (xmlSchemaIntersectWildcards(ctxt, *completeWild, group->attributeWildcard) == -1)
		    return (-1);
	    }
	}
	attrs = attrs->next;
    }

    return (0);
}

static int
xmlSchemaGetEffectiveValueConstraint(xmlSchemaAttributePtr item,
				     int *fixed,
				     const xmlChar **value,
				     xmlSchemaValPtr *val)
{
    *fixed = 0;
    *value = NULL;
    if (val != 0)
	*val = NULL;

    if (item->defValue == NULL)
	item = item->refDecl;

    if (item == NULL)
	return (0);

    if (item->defValue != NULL) {
	*value = item->defValue;
	if (val != 0)
	    *val = item->defVal;
	if (item->flags & XML_SCHEMAS_ATTR_FIXED)
	    *fixed = 1;
	return (1);
    }
    return (0);
}
/**
 * xmlSchemaCheckCVCWildcardNamespace:
 * @wild:  the wildcard
 * @ns:  the namespace
 *
 * Validation Rule: Wildcard allows Namespace Name
 * (cvc-wildcard-namespace)
 *
 *
 * Returns 1 if the given namespace matches the wildcard,
 * 0 otherwise.
 */
static int
xmlSchemaCheckCVCWildcardNamespace(xmlSchemaWildcardPtr wild,
				   const xmlChar* ns)
{
    if (wild == NULL)
	return(-1);

    if (wild->any)
	return(1);
    else if (wild->nsSet != NULL) {
	xmlSchemaWildcardNsPtr cur;

	cur = wild->nsSet;
	while (cur != NULL) {
	    if (xmlStrEqual(cur->value, ns))
		return(1);
	    cur = cur->next;
	}
    } else if ((wild->negNsSet != NULL) && (ns != NULL) &&
	(!xmlStrEqual(wild->negNsSet->value, ns)))
	return(1);

    return(0);
}

/**
 * xmlSchemaBuildAttributeValidation:
 * @ctxt:  the schema parser context
 * @type:  the complex type definition
 *
 *
 * Builds the wildcard and the attribute uses on the given complex type.
 * Returns -1 if an internal error occurs, 0 otherwise.
 */
static int
xmlSchemaBuildAttributeValidation(xmlSchemaParserCtxtPtr pctxt,
				  xmlSchemaTypePtr type)
{
    xmlSchemaTypePtr baseType = NULL;
    xmlSchemaAttributeLinkPtr cur, base, tmp, id = NULL,
	prev = NULL, uses = NULL, lastUse = NULL, lastBaseUse = NULL;
    xmlSchemaAttributePtr attrs;
    xmlSchemaTypePtr anyType;
    xmlChar *str = NULL;
    int err = 0;

    anyType = xmlSchemaGetBuiltInType(XML_SCHEMAS_ANYTYPE);
    /*
     * Complex Type Definition with complex content Schema Component.
     *
     * Attribute uses.
     * TODO: Add checks for absent referenced attribute declarations and
     * simple types.
     */
    if (type->attributeUses != NULL) {
	PERROR_INT("xmlSchemaBuildAttributeValidation",
	    "attribute uses already builded");
        return (-1);
    }
    if (type->baseType == NULL) {
	PERROR_INT("xmlSchemaBuildAttributeValidation",
	    "no base type");
        return (-1);
    }
    baseType = type->baseType;
    /*
     * Inherit the attribute uses of the base type.
     */
    /*
     * NOTE: It is allowed to "extend" the anyType complex type.
     */
    if (! IS_ANYTYPE(baseType)) {
	if (baseType != NULL) {
	    for (cur = baseType->attributeUses; cur != NULL;
		cur = cur->next) {
		tmp = (xmlSchemaAttributeLinkPtr)
		    xmlMalloc(sizeof(xmlSchemaAttributeLink));
		if (tmp == NULL) {
		    xmlSchemaPErrMemory(pctxt,
			"building attribute uses of complexType", NULL);
		    return (-1);
		}
		tmp->attr = cur->attr;
		tmp->next = NULL;
		if (type->attributeUses == NULL) {
		    type->attributeUses = tmp;
		} else
		    lastBaseUse->next = tmp;
		lastBaseUse = tmp;
	    }
	}
    }
    attrs = type->attributes;    
    /*
    * Handle attribute wildcards.
    */
    err = xmlSchemaBuildCompleteAttributeWildcard(pctxt,
	attrs, &type->attributeWildcard);
    /*
    * NOTE: During the parse time, the wildcard is created on the complexType
    * directly, if encountered in a <restriction> or <extension> element.
    */
    if (err == -1) {
	PERROR_INT("xmlSchemaBuildAttributeValidation",
	    "failed to build an intersected attribute wildcard");
	return (-1);
    }

    if ((type->flags & XML_SCHEMAS_TYPE_DERIVATION_METHOD_EXTENSION) &&
	((IS_ANYTYPE(baseType)) ||
	 ((baseType != NULL) &&
	  (baseType->type == XML_SCHEMA_TYPE_COMPLEX) &&
	  (baseType->attributeWildcard != NULL)))) {
	if (type->attributeWildcard != NULL) {
	    /*
	    * Union the complete wildcard with the base wildcard.
	    */
	    if (xmlSchemaUnionWildcards(pctxt, type->attributeWildcard,
		baseType->attributeWildcard) == -1)
		return (-1);
	} else {
	    /*
	    * Just inherit the wildcard.
	    */
	    /*
	    * NOTE: This is the only case where an attribute
            * wildcard is shared.
            */
	    type->attributeWildcard = baseType->attributeWildcard;
	}
    }

    if (type->flags & XML_SCHEMAS_TYPE_DERIVATION_METHOD_RESTRICTION) {
	if (type->attributeWildcard != NULL) {
	    /*
	    * Derivation Valid (Restriction, Complex)
	    * 4.1 The {base type definition} must also have one.
	    */
	    if (baseType->attributeWildcard == NULL) {
		xmlSchemaPCustomErr(pctxt,
		    XML_SCHEMAP_DERIVATION_OK_RESTRICTION_4_1,
		    NULL, type, NULL,
		    "The type has an attribute wildcard, "
		    "but the base type %s does not have one",
		    xmlSchemaFormatItemForReport(&str, NULL, baseType, NULL));
		FREE_AND_NULL(str)
		return (1);
	    } else if (xmlSchemaCheckCOSNSSubset(
		type->attributeWildcard, baseType->attributeWildcard)) {
		/* 4.2 */
		xmlSchemaPCustomErr(pctxt,
		    XML_SCHEMAP_DERIVATION_OK_RESTRICTION_4_2,
		    NULL, type, NULL,
		    "The attribute wildcard is not a valid "
		    "subset of the wildcard in the base type %s",
		    xmlSchemaFormatItemForReport(&str, NULL, baseType, NULL));
		FREE_AND_NULL(str)
		return (1);
	    }
	    /* 4.3 Unless the {base type definition} is the ·ur-type
	    * definition·, the complex type definition's {attribute
	    * wildcard}'s {process contents} must be identical to or
	    * stronger than the {base type definition}'s {attribute
	    * wildcard}'s {process contents}, where strict is stronger
	    * than lax is stronger than skip.
	    */
	    if ((! IS_ANYTYPE(baseType)) &&
		(type->attributeWildcard->processContents <
		baseType->attributeWildcard->processContents)) {
		xmlSchemaPCustomErr(pctxt,
		    XML_SCHEMAP_DERIVATION_OK_RESTRICTION_4_3,
		    NULL, type, NULL,
		    "The 'process contents' of the attribute wildcard is "
		    "weaker than the one in the base type %s",
		    xmlSchemaFormatItemForReport(&str, NULL, baseType, NULL));
		FREE_AND_NULL(str)
		return (1);
	    }
	}
    } else if (type->flags & XML_SCHEMAS_TYPE_DERIVATION_METHOD_EXTENSION) {
	/*
	* Derivation Valid (Extension)
	* At this point the type and the base have both, either
	* no wildcard or a wildcard.
	*/
	if ((baseType->attributeWildcard != NULL) &&
	    (baseType->attributeWildcard != type->attributeWildcard)) {
	    /* 1.3 */
	    if (xmlSchemaCheckCOSNSSubset(
		baseType->attributeWildcard, type->attributeWildcard)) {
		xmlSchemaPCustomErr(pctxt,
		    XML_SCHEMAP_COS_CT_EXTENDS_1_3,
		    NULL, type, NULL,
		    "The attribute wildcard is not a valid "
		    "superset of the one in the base type %s",
		    xmlSchemaFormatItemForReport(&str, NULL, baseType, NULL));
		FREE_AND_NULL(str)
		return (1);
	    }
	}
    }

    /*
     * Gather attribute uses defined by this type.
     */
    if (attrs != NULL) {
	if (xmlSchemaBuildAttributeUsesOwned(pctxt, attrs,
	    &uses, &lastUse) == -1) {
	    return (-1);
	}
    }
    /* 3.4.6 -> Complex Type Definition Properties Correct 4.
     * "Two distinct attribute declarations in the {attribute uses} must
     * not have identical {name}s and {target namespace}s."
     *
     * For "extension" this is done further down.
     */
    if ((uses != NULL) && ((type->flags &
	    XML_SCHEMAS_TYPE_DERIVATION_METHOD_EXTENSION) == 0)) {
	cur = uses;
	while (cur != NULL) {
	    tmp = cur->next;
	    while (tmp != NULL) {
		if ((xmlStrEqual(xmlSchemaGetAttrName(cur->attr),
		    xmlSchemaGetAttrName(tmp->attr))) &&
		    (xmlStrEqual(xmlSchemaGetAttrTargetNsURI(cur->attr),
		    xmlSchemaGetAttrTargetNsURI(tmp->attr)))) {

		    xmlSchemaPAttrUseErr(pctxt,
			XML_SCHEMAP_CT_PROPS_CORRECT_4,
			type, cur->attr,
			"Duplicate attribute use %s specified",
			xmlSchemaFormatQName(&str,
			    xmlSchemaGetAttrTargetNsURI(tmp->attr),
			    xmlSchemaGetAttrName(tmp->attr)));
		    FREE_AND_NULL(str)
		    break;
		}
		tmp = tmp->next;
	    }
	    cur = cur->next;
	}
    }
    if (type->flags & XML_SCHEMAS_TYPE_DERIVATION_METHOD_RESTRICTION) {
	/*
	 * Derive by restriction.
	 */
	if (IS_ANYTYPE(baseType)) {
	    type->attributeUses = uses;
	} else {
	    int found, valid;
	    const xmlChar *bEffValue;
	    int effFixed;

	    cur = uses;
	    while (cur != NULL) {
		found = 0;
		valid = 1;
		base = type->attributeUses;
		while (base != NULL) {
		    if (xmlStrEqual(xmlSchemaGetAttrName(cur->attr),
			xmlSchemaGetAttrName(base->attr)) &&
			xmlStrEqual(xmlSchemaGetAttrTargetNsURI(cur->attr),
			xmlSchemaGetAttrTargetNsURI(base->attr))) {

			found = 1;
			
			if ((cur->attr->occurs ==
			    XML_SCHEMAS_ATTR_USE_PROHIBITED) &&
			    (base->attr->occurs ==
			    XML_SCHEMAS_ATTR_USE_OPTIONAL)) {
			    /*
			    * NOOP.
			    */
			} else if ((cur->attr->occurs ==
			    XML_SCHEMAS_ATTR_USE_OPTIONAL) &&
			    (base->attr->occurs ==
			    XML_SCHEMAS_ATTR_USE_REQUIRED)) {
			    /*
			    * derivation-ok-restriction 2.1.1
			    */
			    xmlSchemaPAttrUseErr(pctxt,
				XML_SCHEMAP_DERIVATION_OK_RESTRICTION_2_1_1,
				type, cur->attr,
				"The 'optional' use is inconsistent with a "
				"matching 'required' use of the base type",
				NULL);
			} else if ((cur->attr->occurs ==
			    XML_SCHEMAS_ATTR_USE_PROHIBITED) &&
			    (base->attr->occurs ==
			    XML_SCHEMAS_ATTR_USE_REQUIRED)) {
			    /*
			    * derivation-ok-restriction 3
			    */
			    xmlSchemaPCustomErr(pctxt,
				XML_SCHEMAP_DERIVATION_OK_RESTRICTION_3,
				NULL, type, NULL,
				"A matching attribute use for the 'required' "
				"attribute use '%s' of the base type is "
				"missing",
				xmlSchemaFormatQName(&str,
				xmlSchemaGetAttrTargetNsURI(base->attr),
				xmlSchemaGetAttrName(base->attr)));
			    FREE_AND_NULL(str)
			} else if (xmlSchemaCheckCOSSTDerivedOK(
			    cur->attr->subtypes, base->attr->subtypes, 0) != 0) {
			    			    
			    /*
			    * SPEC (2.1.2) "R's {attribute declaration}'s
			    * {type definition} must be validly derived from
			    * B's {type definition} given the empty set as
			    * defined in Type Derivation OK (Simple) (§3.14.6)."
			    */
			    xmlSchemaPAttrUseErr(pctxt,
				XML_SCHEMAP_DERIVATION_OK_RESTRICTION_2_1_2,
				type, cur->attr,
				"The attribute declaration's type "
				"definition is not validly derived from "
				"the corresponding definition in the "
				"base type", NULL);			    
			} else {
			    /*
			    * 2.1.3 [Definition:]  Let the effective value
			    * constraint of an attribute use be its {value
			    * constraint}, if present, otherwise its {attribute
			    * declaration}'s {value constraint} .
			    */
			    xmlSchemaGetEffectiveValueConstraint(base->attr,
				&effFixed, &bEffValue, 0);
			    /*
			    * 2.1.3 ... one of the following must be true
			    *
			    * 2.1.3.1 B's ·effective value constraint· is
			    * ·absent· or default.
			    */
			    if ((bEffValue != NULL) &&
				(effFixed == 1)) {
				const xmlChar *rEffValue = NULL;
				
				xmlSchemaGetEffectiveValueConstraint(base->attr,
				    &effFixed, &rEffValue, 0);
				    /*
				    * 2.1.3.2 R's ·effective value constraint· is
				    * fixed with the same string as B's.
				    * TODO: Compare the computed values.
				*/
				if ((effFixed == 0) ||
				    (! xmlStrEqual(rEffValue, bEffValue))) {
				    xmlSchemaPAttrUseErr(pctxt,
					XML_SCHEMAP_DERIVATION_OK_RESTRICTION_2_1_3,
					type, cur->attr,
					"The effective value constraint of the "
					"attribute use is inconsistent with "
					"its correspondent of the base type",
					NULL);
				} else {
				    /*
				    * Override the attribute use.
				    */
				    base->attr = cur->attr;
				}
			    } else				
				base->attr = cur->attr;
			}

			break;
		    }
		    base = base->next;
		}

		if ((!found) && (cur->attr->occurs !=
			XML_SCHEMAS_ATTR_USE_PROHIBITED)) {
		    /*
		    * derivation-ok-restriction  2.2
		    */
		    if ((baseType->attributeWildcard == NULL) ||
			(xmlSchemaCheckCVCWildcardNamespace(
			baseType->attributeWildcard,
			cur->attr->targetNamespace) != 1)) {
			xmlSchemaPAttrUseErr(pctxt,
			    XML_SCHEMAP_DERIVATION_OK_RESTRICTION_2_2,
			    type, cur->attr,
			    "Neither a matching attribute use, "
			    "nor a matching wildcard in the base type does exist",
			    NULL);
		    } else {
			/*
			* Add the attribute use.
			*
			* Note that this may lead to funny derivation error reports, if
			* multiple equal attribute uses exist; but this is not
			* allowed anyway, and it will be reported beforehand.
			*/
			tmp = cur;
			if (prev != NULL)
			    prev->next = cur->next;
			else
			    uses = cur->next;
			cur = cur->next;
			tmp->next = NULL;
			if (type->attributeUses == NULL) {
			    type->attributeUses = tmp;
			} else
			    lastBaseUse->next = tmp;
			lastBaseUse = tmp;
			
			continue;
		    }
		}
		prev = cur;
		cur = cur->next;
	    }
	    if (uses != NULL)
		xmlSchemaFreeAttributeUseList(uses);
	}
    } else if (type->flags & XML_SCHEMAS_TYPE_DERIVATION_METHOD_EXTENSION) {
	/*
	 * The spec allows only appending, and not other kinds of extensions.
	 *
	 * This ensures: Schema Component Constraint: Derivation Valid (Extension) : 1.2
	 */
	if (uses != NULL) {
	    if (type->attributeUses == NULL) {
		type->attributeUses = uses;
	    } else
		lastBaseUse->next = uses;
	}
    } else {
	PERROR_INT("xmlSchemaBuildAttributeValidation",
	    "no derivation method");
	return (-1);
    }
    /*
     * 3.4.6 -> Complex Type Definition Properties Correct
     */
    if (type->attributeUses != NULL) {
	cur = type->attributeUses;
	prev = NULL;
	while (cur != NULL) {
	    /*
	    * 4. Two distinct attribute declarations in the {attribute uses} must
	    * not have identical {name}s and {target namespace}s.
	    *
	    * Note that this was already done for "restriction" and types derived from
	    * the ur-type.
	    */
	    if (type->flags & XML_SCHEMAS_TYPE_DERIVATION_METHOD_EXTENSION) {
		tmp = cur->next;
		while (tmp != NULL) {
		    if ((xmlStrEqual(xmlSchemaGetAttrName(cur->attr),
			xmlSchemaGetAttrName(tmp->attr))) &&
			(xmlStrEqual(xmlSchemaGetAttrTargetNsURI(cur->attr ),
			xmlSchemaGetAttrTargetNsURI(tmp->attr)))) {

			xmlSchemaPAttrUseErr(pctxt,
			    XML_SCHEMAP_CT_PROPS_CORRECT_4,
			    type, tmp->attr,
			    "Duplicate attribute use specified", NULL);
			break;
		    }
		    tmp = tmp->next;
		}
	    }
	    /*
	    * 5. Two distinct attribute declarations in the {attribute uses} must
	    * not have {type definition}s which are or are derived from ID.
	    */
	    if ((cur->attr->subtypes != NULL) &&
		(xmlSchemaIsDerivedFromBuiltInType(cur->attr->subtypes,
		    XML_SCHEMAS_ID))) {
		if (id != NULL) {
		    xmlSchemaPAttrUseErr(pctxt,
			XML_SCHEMAP_CT_PROPS_CORRECT_5,
			type, cur->attr,
			"There must not exist more than one attribute use, "
			"declared of type 'ID' or derived from it",
			NULL);
		    FREE_AND_NULL(str)
		}
		id = cur;
	    }
	    /*
	    * Remove "prohibited" attribute uses. The reason this is done at this late
	    * stage is to be able to catch dublicate attribute uses. So we had to keep
	    * prohibited uses in the list as well.
	    */
	    if (cur->attr->occurs == XML_SCHEMAS_ATTR_USE_PROHIBITED) {
		tmp = cur;
		if (prev == NULL)
		    type->attributeUses = cur->next;
		else
		    prev->next = cur->next;
		cur = cur->next;
		xmlFree(tmp);
	    } else {
		prev = cur;
		cur = cur->next;
	    }
	}
    }
    /*
     * TODO: This check should be removed if we are 100% sure of
     * the base type attribute uses already being built.
     */
    if ((baseType != NULL) && (! IS_ANYTYPE(baseType)) &&
	(baseType->type == XML_SCHEMA_TYPE_COMPLEX) &&
	(IS_NOT_TYPEFIXED(baseType))) {
	PERROR_INT("xmlSchemaBuildAttributeValidation",
	    "attribute uses not builded on base type");
    }
    return (0);
}

/**
 * xmlSchemaTypeFinalContains:
 * @schema:  the schema
 * @type:  the type definition
 * @final: the final
 *
 * Evaluates if a type definition contains the given "final".
 * This does take "finalDefault" into account as well.
 *
 * Returns 1 if the type does containt the given "final",
 * 0 otherwise.
 */
static int
xmlSchemaTypeFinalContains(xmlSchemaTypePtr type, int final)
{
    if (type == NULL)
	return (0);
    if (type->flags & final)
	return (1);
    else
	return (0);
}

/**
 * xmlSchemaGetUnionSimpleTypeMemberTypes:
 * @type:  the Union Simple Type
 *
 * Returns a list of member types of @type if existing,
 * returns NULL otherwise.
 */
static xmlSchemaTypeLinkPtr
xmlSchemaGetUnionSimpleTypeMemberTypes(xmlSchemaTypePtr type)
{
    while ((type != NULL) && (type->type == XML_SCHEMA_TYPE_SIMPLE)) {
	if (type->memberTypes != NULL)
	    return (type->memberTypes);
	else
	    type = type->baseType;
    }
    return (NULL);
}

/**
 * xmlSchemaGetParticleTotalRangeMin:
 * @particle: the particle
 *
 * Schema Component Constraint: Effective Total Range
 * (all and sequence) + (choice)
 *
 * Returns the minimun Effective Total Range.
 */
static int
xmlSchemaGetParticleTotalRangeMin(xmlSchemaParticlePtr particle)
{
    if ((particle->children == NULL) ||
	(particle->minOccurs == 0))
	return (0);
    if (particle->children->type == XML_SCHEMA_TYPE_CHOICE) {
	int min = -1, cur;
	xmlSchemaParticlePtr part =
	    (xmlSchemaParticlePtr) particle->children->children;

	if (part == NULL)
	    return (0);
	while (part != NULL) {
	    if ((part->children->type == XML_SCHEMA_TYPE_ELEMENT) ||
		(part->children->type == XML_SCHEMA_TYPE_ANY))
		cur = part->minOccurs;
	    else
		cur = xmlSchemaGetParticleTotalRangeMin(part);
	    if (cur == 0)
		return (0);
	    if ((min > cur) || (min == -1))
		min = cur;
	    part = (xmlSchemaParticlePtr) part->next;
	}
	return (particle->minOccurs * min);
    } else {
	/* <all> and <sequence> */
	int sum = 0;
	xmlSchemaParticlePtr part =
	    (xmlSchemaParticlePtr) particle->children->children;

	if (part == NULL)
	    return (0);
	do {
	    if ((part->children->type == XML_SCHEMA_TYPE_ELEMENT) ||
		(part->children->type == XML_SCHEMA_TYPE_ANY))
		sum += part->minOccurs;
	    else
		sum += xmlSchemaGetParticleTotalRangeMin(part);
	    part = (xmlSchemaParticlePtr) part->next;
	} while (part != NULL);
	return (particle->minOccurs * sum);
    }
}

/**
 * xmlSchemaGetParticleTotalRangeMax:
 * @particle: the particle
 *
 * Schema Component Constraint: Effective Total Range
 * (all and sequence) + (choice)
 *
 * Returns the maximum Effective Total Range.
 */
static int
xmlSchemaGetParticleTotalRangeMax(xmlSchemaParticlePtr particle)
{
    if ((particle->children == NULL) ||
	(particle->children->children == NULL))
	return (0);
    if (particle->children->type == XML_SCHEMA_TYPE_CHOICE) {
	int max = -1, cur;
	xmlSchemaParticlePtr part =
	    (xmlSchemaParticlePtr) particle->children->children;

	for (; part != NULL; part = (xmlSchemaParticlePtr) part->next) {
	    if (part->children == NULL)
		continue;
	    if ((part->children->type == XML_SCHEMA_TYPE_ELEMENT) ||
		(part->children->type == XML_SCHEMA_TYPE_ANY))
		cur = part->maxOccurs;
	    else
		cur = xmlSchemaGetParticleTotalRangeMax(part);
	    if (cur == UNBOUNDED)
		return (UNBOUNDED);
	    if ((max < cur) || (max == -1))
		max = cur;
	}
	/* TODO: Handle overflows? */
	return (particle->maxOccurs * max);
    } else {
	/* <all> and <sequence> */
	int sum = 0, cur;
	xmlSchemaParticlePtr part =
	    (xmlSchemaParticlePtr) particle->children->children;

	for (; part != NULL; part = (xmlSchemaParticlePtr) part->next) {
	    if (part->children == NULL)
		continue;
	    if ((part->children->type == XML_SCHEMA_TYPE_ELEMENT) ||
		(part->children->type == XML_SCHEMA_TYPE_ANY))
		cur = part->maxOccurs;
	    else
		cur = xmlSchemaGetParticleTotalRangeMax(part);
	    if (cur == UNBOUNDED)
		return (UNBOUNDED);
	    if ((cur > 0) && (particle->maxOccurs == UNBOUNDED))
		return (UNBOUNDED);
	    sum += cur;
	}
	/* TODO: Handle overflows? */
	return (particle->maxOccurs * sum);
    }
}

/**
 * xmlSchemaIsParticleEmptiable:
 * @particle: the particle
 *
 * Schema Component Constraint: Particle Emptiable
 * Checks whether the given particle is emptiable.
 *
 * Returns 1 if emptiable, 0 otherwise.
 */
static int
xmlSchemaIsParticleEmptiable(xmlSchemaParticlePtr particle)
{
    /*
    * SPEC (1) "Its {min occurs} is 0."
    */
    if ((particle == NULL) || (particle->minOccurs == 0) ||
	(particle->children == NULL))
	return (1);
    /*
    * SPEC (2) "Its {term} is a group and the minimum part of the
    * effective total range of that group, [...] is 0."
    */
    if (IS_MODEL_GROUP(particle->children)) {
	if (xmlSchemaGetParticleTotalRangeMin(particle) == 0)
	    return (1);
    }
    return (0);
}

/**
 * xmlSchemaCheckCOSSTDerivedOK:
 * @type:  the derived simple type definition
 * @baseType:  the base type definition
 *
 * Schema Component Constraint:
 * Type Derivation OK (Simple) (cos-st-derived-OK)
 *
 * Checks wheter @type can be validly
 * derived from @baseType.
 *
 * Returns 0 on success, an positive error code otherwise.
 */
static int
xmlSchemaCheckCOSSTDerivedOK(xmlSchemaTypePtr type,
			     xmlSchemaTypePtr baseType,
			     int subset)
{
    /*
    * 1 They are the same type definition.
    * TODO: The identy check might have to be more complex than this.
    */
    if (type == baseType)
	return (0);
    /*
    * 2.1 restriction is not in the subset, or in the {final}
    * of its own {base type definition};
    */
    if ((subset & SUBSET_RESTRICTION) ||
	(xmlSchemaTypeFinalContains(type->baseType,
	    XML_SCHEMAS_TYPE_FINAL_RESTRICTION))) {
	return (XML_SCHEMAP_COS_ST_DERIVED_OK_2_1);
    }
    /* 2.2 */
    if (type->baseType == baseType) {
	/*
	* 2.2.1 D's ·base type definition· is B.
	*/
	return (0);
    }
    /*
    * 2.2.2 D's ·base type definition· is not the ·ur-type definition·
    * and is validly derived from B given the subset, as defined by this
    * constraint.
    */
    if ((! IS_ANYTYPE(type->baseType)) &&
	(xmlSchemaCheckCOSSTDerivedOK(type->baseType,
	    baseType, subset) == 0)) {
	return (0);
    }
    /*
    * 2.2.3 D's {variety} is list or union and B is the ·simple ur-type
    * definition·.
    */
    if (IS_ANY_SIMPLE_TYPE(baseType) &&
	(VARIETY_LIST(type) || VARIETY_UNION(type))) {
	return (0);
    }
    /*
    * 2.2.4 B's {variety} is union and D is validly derived from a type
    * definition in B's {member type definitions} given the subset, as
    * defined by this constraint.
    *
    * NOTE: This seems not to involve built-in types, since there is no
    * built-in Union Simple Type.
    */
    if (VARIETY_UNION(baseType)) {
	xmlSchemaTypeLinkPtr cur;

	cur = baseType->memberTypes;
	while (cur != NULL) {
	    if (xmlSchemaCheckCOSSTDerivedOK(type, cur->type, subset) == 0)
		return (0);
	    cur = cur->next;
	}
    }

    return (XML_SCHEMAP_COS_ST_DERIVED_OK_2_2);
}

/**
 * xmlSchemaCheckTypeDefCircularInternal:
 * @pctxt:  the schema parser context
 * @ctxtType:  the type definition
 * @ancestor: an ancestor of @ctxtType
 *
 * Checks st-props-correct (2) + ct-props-correct (3).
 * Circular type definitions are not allowed.
 *
 * Returns XML_SCHEMAP_ST_PROPS_CORRECT_2 if the given type is
 * circular, 0 otherwise.
 */
static int
xmlSchemaCheckTypeDefCircularInternal(xmlSchemaParserCtxtPtr pctxt,
			   xmlSchemaTypePtr ctxtType,
			   xmlSchemaTypePtr ancestor)
{
    int ret;

    if ((ancestor == NULL) || (ancestor->type == XML_SCHEMA_TYPE_BASIC))
	return (0);

    if (ctxtType == ancestor) {
	xmlSchemaPCustomErr(pctxt,
	    XML_SCHEMAP_ST_PROPS_CORRECT_2,
	    NULL, ctxtType, GET_NODE(ctxtType),
	    "The definition is circular", NULL);
	return (XML_SCHEMAP_ST_PROPS_CORRECT_2);
    }
    if (ancestor->flags & XML_SCHEMAS_TYPE_MARKED) {
	/*
	* Avoid inifinite recursion on circular types not yet checked.
	*/
	return (0);
    }
    ancestor->flags |= XML_SCHEMAS_TYPE_MARKED;
    ret = xmlSchemaCheckTypeDefCircularInternal(pctxt, ctxtType,
	ancestor->baseType);
    ancestor->flags ^= XML_SCHEMAS_TYPE_MARKED;
    return (ret);
}

/**
 * xmlSchemaCheckTypeDefCircular:
 * @item:  the complex/simple type definition
 * @ctxt:  the parser context
 * @name:  the name
 *
 * Checks for circular type definitions.
 */
static void
xmlSchemaCheckTypeDefCircular(xmlSchemaTypePtr item,
			      xmlSchemaParserCtxtPtr ctxt,
			      const xmlChar * name ATTRIBUTE_UNUSED)
{
    if ((item == NULL) ||
	((item->type != XML_SCHEMA_TYPE_COMPLEX) &&
	(item->type != XML_SCHEMA_TYPE_SIMPLE)))
	return;
    xmlSchemaCheckTypeDefCircularInternal(ctxt, item, item->baseType);

}

/**
 * xmlSchemaResolveTypeDefs:
 * @item:  the complex/simple type definition
 * @ctxt:  the parser context
 * @name:  the name
 *
 * Checks for circular type definitions.
 */
static void
xmlSchemaResolveTypeDefs(xmlSchemaTypePtr typeDef,
			 xmlSchemaParserCtxtPtr ctxt,
			 const xmlChar * name ATTRIBUTE_UNUSED)
{
    if (typeDef == NULL)
	return;

    /*
    * Resolve the base type.
    */
    if (typeDef->baseType == NULL) {
	typeDef->baseType = xmlSchemaGetType(ctxt->schema,
	    typeDef->base, typeDef->baseNs);
	if (typeDef->baseType == NULL) {
	    xmlSchemaPResCompAttrErr(ctxt,
		XML_SCHEMAP_SRC_RESOLVE,
		typeDef, typeDef->node,
		"base", typeDef->base, typeDef->baseNs,
		XML_SCHEMA_TYPE_SIMPLE, NULL);
	    return;
	}
    }
    if (IS_SIMPLE_TYPE(typeDef)) {
	if (VARIETY_UNION(typeDef)) {
	    /*
	    * Resolve the memberTypes.
	    */
	    xmlSchemaResolveUnionMemberTypes(ctxt, typeDef);
	    return;
	} else if (VARIETY_LIST(typeDef)) {
	    /*
	    * Resolve the itemType.
	    */
	    if ((typeDef->subtypes == NULL) && (typeDef->ref != NULL)) {
		typeDef->subtypes = xmlSchemaGetType(ctxt->schema,
		    typeDef->ref, typeDef->refNs);
		if ((typeDef->subtypes == NULL) ||
		    (! IS_SIMPLE_TYPE(typeDef->subtypes))) {
		    typeDef->subtypes = NULL;
		    xmlSchemaPResCompAttrErr(ctxt,
			XML_SCHEMAP_SRC_RESOLVE,
			typeDef, typeDef->node,
			"itemType", typeDef->ref, typeDef->refNs,
			XML_SCHEMA_TYPE_SIMPLE, NULL);
		}
	    }
	    return;
	}
    }
}



/**
 * xmlSchemaCheckSTPropsCorrect:
 * @ctxt:  the schema parser context
 * @type:  the simple type definition
 *
 * Checks st-props-correct.
 *
 * Returns 0 if the properties are correct,
 * if not, a positive error code and -1 on internal
 * errors.
 */
static int
xmlSchemaCheckSTPropsCorrect(xmlSchemaParserCtxtPtr ctxt,
			     xmlSchemaTypePtr type)
{
    xmlSchemaTypePtr baseType = type->baseType, anySimpleType,
	anyType;
    xmlChar *str = NULL;

    /* STATE: error funcs converted. */
    /*
    * Schema Component Constraint: Simple Type Definition Properties Correct
    *
    * NOTE: This is somehow redundant, since we actually built a simple type
    * to have all the needed information; this acts as an self test.
    */
    anySimpleType = xmlSchemaGetBuiltInType(XML_SCHEMAS_ANYSIMPLETYPE);
    anyType = xmlSchemaGetBuiltInType(XML_SCHEMAS_ANYTYPE);
    /* Base type: If the datatype has been ·derived· by ·restriction·
    * then the Simple Type Definition component from which it is ·derived·,
    * otherwise the Simple Type Definition for anySimpleType (§4.1.6).
    */
    if (baseType == NULL) {
	/*
	* TODO: Think about: "modulo the impact of Missing
	* Sub-components (§5.3)."
	*/
	xmlSchemaPCustomErr(ctxt,
	    XML_SCHEMAP_ST_PROPS_CORRECT_1,
	    NULL, type, NULL,
	    "No base type existent", NULL);
	return (XML_SCHEMAP_ST_PROPS_CORRECT_1);

    }
    if (! IS_SIMPLE_TYPE(baseType)) {
	xmlSchemaPCustomErr(ctxt,
	    XML_SCHEMAP_ST_PROPS_CORRECT_1,
	    NULL, type, NULL,
	    "The base type '%s' is not a simple type",
	    xmlSchemaGetComponentQName(&str, baseType));
	FREE_AND_NULL(str)
	return (XML_SCHEMAP_ST_PROPS_CORRECT_1);
    }
    if ( (VARIETY_LIST(type) || VARIETY_UNION(type)) &&
	 ((type->flags & XML_SCHEMAS_TYPE_DERIVATION_METHOD_RESTRICTION) == 0) &&
	 (! IS_ANY_SIMPLE_TYPE(baseType))) {
	xmlSchemaPCustomErr(ctxt,
	    XML_SCHEMAP_ST_PROPS_CORRECT_1,
	    NULL, type, NULL,
	    "A type, derived by list or union, must have"
	    "the simple ur-type definition as base type, not '%s'",
	    xmlSchemaGetComponentQName(&str, baseType));
	FREE_AND_NULL(str)
	return (XML_SCHEMAP_ST_PROPS_CORRECT_1);
    }
    /*
    * Variety: One of {atomic, list, union}.
    */
    if ((! VARIETY_ATOMIC(type)) && (! VARIETY_UNION(type)) &&
	(! VARIETY_LIST(type))) {
	xmlSchemaPCustomErr(ctxt,
	    XML_SCHEMAP_ST_PROPS_CORRECT_1,
	    NULL, type, NULL,
	    "The variety is absent", NULL);
	return (XML_SCHEMAP_ST_PROPS_CORRECT_1);
    }
    /* TODO: Finish this. Hmm, is this finished? */

    /*
    * 3 The {final} of the {base type definition} must not contain restriction.
    */
    if (xmlSchemaTypeFinalContains(baseType,
	XML_SCHEMAS_TYPE_FINAL_RESTRICTION)) {
	xmlSchemaPCustomErr(ctxt,
	    XML_SCHEMAP_ST_PROPS_CORRECT_3,
	    NULL, type, NULL,
	    "The 'final' of its base type '%s' must not contain "
	    "'restriction'",
	    xmlSchemaGetComponentQName(&str, baseType));
	FREE_AND_NULL(str)
	return (XML_SCHEMAP_ST_PROPS_CORRECT_3);
    }

    /*
    * 2 All simple type definitions must be derived ultimately from the ·simple
    * ur-type definition (so· circular definitions are disallowed). That is, it
    * must be possible to reach a built-in primitive datatype or the ·simple
    * ur-type definition· by repeatedly following the {base type definition}.
    *
    * NOTE: this is done in xmlSchemaCheckTypeDefCircular().
    */
    return (0);
}

/**
 * xmlSchemaCheckCOSSTRestricts:
 * @ctxt:  the schema parser context
 * @type:  the simple type definition
 *
 * Schema Component Constraint:
 * Derivation Valid (Restriction, Simple) (cos-st-restricts)

 * Checks if the given @type (simpleType) is derived validly by restriction.
 * STATUS:
 *
 * Returns -1 on internal errors, 0 if the type is validly derived,
 * a positive error code otherwise.
 */
static int
xmlSchemaCheckCOSSTRestricts(xmlSchemaParserCtxtPtr pctxt,
			     xmlSchemaTypePtr type)
{
    xmlChar *str = NULL;

    if (type->type != XML_SCHEMA_TYPE_SIMPLE) {
	PERROR_INT("xmlSchemaCheckCOSSTRestricts",
	    "given type is not a user-derived simpleType");
	return (-1);
    }

    if (VARIETY_ATOMIC(type)) {
	xmlSchemaTypePtr primitive;
	/*
	* 1.1 The {base type definition} must be an atomic simple
	* type definition or a built-in primitive datatype.
	*/
	if (! VARIETY_ATOMIC(type->baseType)) {
	    xmlSchemaPCustomErr(pctxt,
		XML_SCHEMAP_COS_ST_RESTRICTS_1_1,
		NULL, type, NULL,
		"The base type '%s' is not an atomic simple type",
		xmlSchemaGetComponentQName(&str, type->baseType));
	    FREE_AND_NULL(str)
	    return (XML_SCHEMAP_COS_ST_RESTRICTS_1_1);
	}
	/* 1.2 The {final} of the {base type definition} must not contain
	* restriction.
	*/
	/* OPTIMIZE TODO : This is already done in xmlSchemaCheckStPropsCorrect */
	if (xmlSchemaTypeFinalContains(type->baseType,
	    XML_SCHEMAS_TYPE_FINAL_RESTRICTION)) {
	    xmlSchemaPCustomErr(pctxt,
		XML_SCHEMAP_COS_ST_RESTRICTS_1_2,
		NULL, type, NULL,
		"The final of its base type '%s' must not contain 'restriction'",
		xmlSchemaGetComponentQName(&str, type->baseType));
	    FREE_AND_NULL(str)
	    return (XML_SCHEMAP_COS_ST_RESTRICTS_1_2);
	}

	/*
	* 1.3.1 DF must be an allowed constraining facet for the {primitive
	* type definition}, as specified in the appropriate subsection of 3.2
	* Primitive datatypes.
	*/
	if (type->facets != NULL) {
	    xmlSchemaFacetPtr facet;
	    int ok = 1;

	    primitive = xmlSchemaGetPrimitiveType(type);
	    if (primitive == NULL) {
		PERROR_INT("xmlSchemaCheckCOSSTRestricts",
		    "failed to get primitive type");
		return (-1);
	    }
	    facet = type->facets;
	    do {
		if (xmlSchemaIsBuiltInTypeFacet(primitive, facet->type) == 0) {
		    ok = 0;
		    xmlSchemaPIllegalFacetAtomicErr(pctxt,
			XML_SCHEMAP_COS_ST_RESTRICTS_1_3_1,
			NULL, type, primitive, facet);
		}
		facet = facet->next;
	    } while (facet != NULL);
	    if (ok == 0)
		return (XML_SCHEMAP_COS_ST_RESTRICTS_1_3_1);
	}
	/*
	* SPEC (1.3.2) "If there is a facet of the same kind in the {facets}
	* of the {base type definition} (call this BF),then the DF's {value}
	* must be a valid restriction of BF's {value} as defined in
	* [XML Schemas: Datatypes]."
	*
	* NOTE (1.3.2) Facet derivation constraints are currently handled in
	* xmlSchemaDeriveAndValidateFacets()
	*/
    } else if (VARIETY_LIST(type)) {
	xmlSchemaTypePtr itemType = NULL;

	itemType = type->subtypes;
	if ((itemType == NULL) || (! IS_SIMPLE_TYPE(itemType))) {
	    PERROR_INT("xmlSchemaCheckCOSSTRestricts",
		"failed to evaluate the item type");
	    return (-1);
	}
	if (IS_NOT_TYPEFIXED(itemType))
	    xmlSchemaTypeFixup(itemType, pctxt, NULL);
	/*
	* 2.1 The {item type definition} must have a {variety} of atomic or
	* union (in which case all the {member type definitions}
	* must be atomic).
	*/
	if ((! VARIETY_ATOMIC(itemType)) &&
	    (! VARIETY_UNION(itemType))) {
	    xmlSchemaPCustomErr(pctxt,
		XML_SCHEMAP_COS_ST_RESTRICTS_2_1,
		NULL, type, NULL,
		"The item type '%s' does not have a variety of atomic or union",
		xmlSchemaGetComponentQName(&str, itemType));
	    FREE_AND_NULL(str)
	    return (XML_SCHEMAP_COS_ST_RESTRICTS_2_1);
	} else if (VARIETY_UNION(itemType)) {
	    xmlSchemaTypeLinkPtr member;

	    member = itemType->memberTypes;
	    while (member != NULL) {
		if (! VARIETY_ATOMIC(member->type)) {
		    xmlSchemaPCustomErr(pctxt,
			XML_SCHEMAP_COS_ST_RESTRICTS_2_1,
			NULL, type, NULL,
			"The item type is a union type, but the "
			"member type '%s' of this item type is not atomic",
			xmlSchemaGetComponentQName(&str, member->type));
		    FREE_AND_NULL(str)
		    return (XML_SCHEMAP_COS_ST_RESTRICTS_2_1);
		}
		member = member->next;
	    }
	}

	if (IS_ANY_SIMPLE_TYPE(type->baseType)) {
	    xmlSchemaFacetPtr facet;
	    /*
	    * This is the case if we have: <simpleType><list ..
	    */
	    /*
	    * 2.3.1
	    * 2.3.1.1 The {final} of the {item type definition} must not
	    * contain list.
	    */
	    if (xmlSchemaTypeFinalContains(itemType,
		XML_SCHEMAS_TYPE_FINAL_LIST)) {
		xmlSchemaPCustomErr(pctxt,
		    XML_SCHEMAP_COS_ST_RESTRICTS_2_3_1_1,
		    NULL, type, NULL,
		    "The final of its item type '%s' must not contain 'list'",
		    xmlSchemaGetComponentQName(&str, itemType));
		FREE_AND_NULL(str)
		return (XML_SCHEMAP_COS_ST_RESTRICTS_2_3_1_1);
	    }
	    /*
	    * 2.3.1.2 The {facets} must only contain the whiteSpace
	    * facet component.
	    * OPTIMIZE TODO: the S4S already disallows any facet
	    * to be specified.
	    */
	    if (type->facets != NULL) {
		facet = type->facets;
		do {
		    if (facet->type != XML_SCHEMA_FACET_WHITESPACE) {
			xmlSchemaPIllegalFacetListUnionErr(pctxt,
			    XML_SCHEMAP_COS_ST_RESTRICTS_2_3_1_2,
			    NULL, type, facet);
			return (XML_SCHEMAP_COS_ST_RESTRICTS_2_3_1_2);
		    }
		    facet = facet->next;
		} while (facet != NULL);
	    }
	    /*
	    * MAYBE TODO: (Hmm, not really) Datatypes states:
	    * A ·list· datatype can be ·derived· from an ·atomic· datatype
	    * whose ·lexical space· allows space (such as string or anyURI)or
	    * a ·union· datatype any of whose {member type definitions}'s
	    * ·lexical space· allows space.
	    */
	} else {
	    /*
	    * This is the case if we have: <simpleType><restriction ...
	    * I.e. the variety of "list" is inherited.
	    */
	    /*
	    * 2.3.2
	    * 2.3.2.1 The {base type definition} must have a {variety} of list.
	    */
	    if (! VARIETY_LIST(type->baseType)) {
		xmlSchemaPCustomErr(pctxt,
		    XML_SCHEMAP_COS_ST_RESTRICTS_2_3_2_1,
		    NULL, type, NULL,
		    "The base type '%s' must be a list type",
		    xmlSchemaGetComponentQName(&str, type->baseType));
		FREE_AND_NULL(str)
		return (XML_SCHEMAP_COS_ST_RESTRICTS_2_3_2_1);
	    }
	    /*
	    * 2.3.2.2 The {final} of the {base type definition} must not
	    * contain restriction.
	    */
	    if (xmlSchemaTypeFinalContains(type->baseType,
		XML_SCHEMAS_TYPE_FINAL_RESTRICTION)) {
		xmlSchemaPCustomErr(pctxt,
		    XML_SCHEMAP_COS_ST_RESTRICTS_2_3_2_2,
		    NULL, type, NULL,
		    "The 'final' of the base type '%s' must not contain 'restriction'",
		    xmlSchemaGetComponentQName(&str, type->baseType));
		FREE_AND_NULL(str)
		return (XML_SCHEMAP_COS_ST_RESTRICTS_2_3_2_2);
	    }
	    /*
	    * 2.3.2.3 The {item type definition} must be validly derived
	    * from the {base type definition}'s {item type definition} given
	    * the empty set, as defined in Type Derivation OK (Simple) (§3.14.6).
	    */
	    {
		xmlSchemaTypePtr baseItemType;

		baseItemType = type->baseType->subtypes;
		if ((baseItemType == NULL) || (! IS_SIMPLE_TYPE(baseItemType))) {
		    PERROR_INT("xmlSchemaCheckCOSSTRestricts",
			"failed to eval the item type of a base type");
		    return (-1);
		}
		if ((itemType != baseItemType) &&
		    (xmlSchemaCheckCOSSTDerivedOK(itemType,
			baseItemType, 0) != 0)) {
		    xmlChar *strBIT = NULL, *strBT = NULL;
		    xmlSchemaPCustomErrExt(pctxt,
			XML_SCHEMAP_COS_ST_RESTRICTS_2_3_2_3,
			NULL, type, NULL,
			"The item type '%s' is not validly derived from "
			"the item type '%s' of the base type '%s'",
			xmlSchemaGetComponentQName(&str, itemType),
			xmlSchemaGetComponentQName(&strBIT, baseItemType),
			xmlSchemaGetComponentQName(&strBT, type->baseType));

		    FREE_AND_NULL(str)
		    FREE_AND_NULL(strBIT)
		    FREE_AND_NULL(strBT)
		    return (XML_SCHEMAP_COS_ST_RESTRICTS_2_3_2_3);
		}
	    }

	    if (type->facets != NULL) {
		xmlSchemaFacetPtr facet;
		int ok = 1;
		/*
		* 2.3.2.4 Only length, minLength, maxLength, whiteSpace, pattern
		* and enumeration facet components are allowed among the {facets}.
		*/
		facet = type->facets;
		do {
		    switch (facet->type) {
			case XML_SCHEMA_FACET_LENGTH:
			case XML_SCHEMA_FACET_MINLENGTH:
			case XML_SCHEMA_FACET_MAXLENGTH:
			case XML_SCHEMA_FACET_WHITESPACE:
			    /*
			    * TODO: 2.5.1.2 List datatypes
			    * The value of ·whiteSpace· is fixed to the value collapse.
			    */
			case XML_SCHEMA_FACET_PATTERN:
			case XML_SCHEMA_FACET_ENUMERATION:
			    break;
			default: {
			    xmlSchemaPIllegalFacetListUnionErr(pctxt,
				XML_SCHEMAP_COS_ST_RESTRICTS_2_3_2_4,
				NULL, type, facet);
			    /*
			    * We could return, but it's nicer to report all
			    * invalid facets.
			    */
			    ok = 0;
			}
		    }
		    facet = facet->next;
		} while (facet != NULL);
		if (ok == 0)
		    return (XML_SCHEMAP_COS_ST_RESTRICTS_2_3_2_4);
		/*
		* SPEC (2.3.2.5) (same as 1.3.2)
		*
		* NOTE (2.3.2.5) This is currently done in
		* xmlSchemaDeriveAndValidateFacets()
		*/
	    }
	}
    } else if (VARIETY_UNION(type)) {
	/*
	* 3.1 The {member type definitions} must all have {variety} of
	* atomic or list.
	*/
	xmlSchemaTypeLinkPtr member;

	member = type->memberTypes;
	while (member != NULL) {
	    if (IS_NOT_TYPEFIXED(member->type))
		xmlSchemaTypeFixup(member->type, pctxt, NULL);

	    if ((! VARIETY_ATOMIC(member->type)) &&
		(! VARIETY_LIST(member->type))) {
		xmlSchemaPCustomErr(pctxt,
		    XML_SCHEMAP_COS_ST_RESTRICTS_3_1,
		    NULL, type, NULL,
		    "The member type '%s' is neither an atomic, nor a list type",
		    xmlSchemaGetComponentQName(&str, member->type));
		FREE_AND_NULL(str)
		return (XML_SCHEMAP_COS_ST_RESTRICTS_3_1);
	    }
	    member = member->next;
	}
	/*
	* 3.3.1 If the {base type definition} is the ·simple ur-type
	* definition·
	*/
	if (type->baseType->builtInType == XML_SCHEMAS_ANYSIMPLETYPE) {
	    /*
	    * 3.3.1.1 All of the {member type definitions} must have a
	    * {final} which does not contain union.
	    */
	    member = type->memberTypes;
	    while (member != NULL) {
		if (xmlSchemaTypeFinalContains(member->type,
		    XML_SCHEMAS_TYPE_FINAL_UNION)) {
		    xmlSchemaPCustomErr(pctxt,
			XML_SCHEMAP_COS_ST_RESTRICTS_3_3_1,
			NULL, type, NULL,
			"The 'final' of member type '%s' contains 'union'",
			xmlSchemaGetComponentQName(&str, member->type));
		    FREE_AND_NULL(str)
		    return (XML_SCHEMAP_COS_ST_RESTRICTS_3_3_1);
		}
		member = member->next;
	    }
	    /*
	    * 3.3.1.2 The {facets} must be empty.
	    */
	    if (type->facetSet != NULL) {
		xmlSchemaPCustomErr(pctxt,
		    XML_SCHEMAP_COS_ST_RESTRICTS_3_3_1_2,
		    NULL, type, NULL,
		    "No facets allowed", NULL);
		return (XML_SCHEMAP_COS_ST_RESTRICTS_3_3_1_2);
	    }
	} else {
	    /*
	    * 3.3.2.1 The {base type definition} must have a {variety} of union.
	    * I.e. the variety of "list" is inherited.
	    */
	    if (! VARIETY_UNION(type->baseType)) {
		xmlSchemaPCustomErr(pctxt,
		    XML_SCHEMAP_COS_ST_RESTRICTS_3_3_2_1,
		    NULL, type, NULL,
		    "The base type '%s' is not a union type",
		    xmlSchemaGetComponentQName(&str, type->baseType));
		FREE_AND_NULL(str)
		return (XML_SCHEMAP_COS_ST_RESTRICTS_3_3_2_1);
	    }
	    /*
	    * 3.3.2.2 The {final} of the {base type definition} must not contain restriction.
	    */
	    if (xmlSchemaTypeFinalContains(type->baseType,
		XML_SCHEMAS_TYPE_FINAL_RESTRICTION)) {
		xmlSchemaPCustomErr(pctxt,
		    XML_SCHEMAP_COS_ST_RESTRICTS_3_3_2_2,
		    NULL, type, NULL,
		    "The 'final' of its base type '%s' must not contain 'restriction'",
		    xmlSchemaGetComponentQName(&str, type->baseType));
		FREE_AND_NULL(str)
		return (XML_SCHEMAP_COS_ST_RESTRICTS_3_3_2_2);
	    }
	    /*
	    * 3.3.2.3 The {member type definitions}, in order, must be validly
	    * derived from the corresponding type definitions in the {base
	    * type definition}'s {member type definitions} given the empty set,
	    * as defined in Type Derivation OK (Simple) (§3.14.6).
	    */
	    {
		xmlSchemaTypeLinkPtr baseMember;

		/*
		* OPTIMIZE: if the type is restricting, it has no local defined
		* member types and inherits the member types of the base type;
		* thus a check for equality can be skipped.
		*/
		/*
		* Even worse: I cannot see a scenario where a restricting
		* union simple type can have other member types as the member
		* types of it's base type. This check seems not necessary with
		* respect to the derivation process in libxml2.
		* But necessary if constructing types with an API.
		*/
		if (type->memberTypes != NULL) {
		    member = type->memberTypes;
		    baseMember = xmlSchemaGetUnionSimpleTypeMemberTypes(type->baseType);
		    if ((member == NULL) && (baseMember != NULL)) {
			PERROR_INT("xmlSchemaCheckCOSSTRestricts",
			    "different number of member types in base");
		    }
		    while (member != NULL) {
			if (baseMember == NULL) {
			    PERROR_INT("xmlSchemaCheckCOSSTRestricts",
			    "different number of member types in base");
			}
			if ((member->type != baseMember->type) &&
			    (xmlSchemaCheckCOSSTDerivedOK(
				member->type, baseMember->type, 0) != 0)) {
			    xmlChar *strBMT = NULL, *strBT = NULL;

			    xmlSchemaPCustomErrExt(pctxt,
				XML_SCHEMAP_COS_ST_RESTRICTS_3_3_2_3,
				NULL, type, NULL,
				"The member type %s is not validly "
				"derived from its corresponding member "
				"type %s of the base type %s",
				xmlSchemaGetComponentQName(&str, member->type),
				xmlSchemaGetComponentQName(&strBMT, baseMember->type),
				xmlSchemaGetComponentQName(&strBT, type->baseType));
			    FREE_AND_NULL(str)
			    FREE_AND_NULL(strBMT)
			    FREE_AND_NULL(strBT)
			    return (XML_SCHEMAP_COS_ST_RESTRICTS_3_3_2_3);
			}
			member = member->next;
			baseMember = baseMember->next;
		    }
		}
	    }
	    /*
	    * 3.3.2.4 Only pattern and enumeration facet components are
	    * allowed among the {facets}.
	    */
	    if (type->facets != NULL) {
		xmlSchemaFacetPtr facet;
		int ok = 1;

		facet = type->facets;
		do {
		    if ((facet->type != XML_SCHEMA_FACET_PATTERN) &&
			(facet->type != XML_SCHEMA_FACET_ENUMERATION)) {
			xmlSchemaPIllegalFacetListUnionErr(pctxt,
				XML_SCHEMAP_COS_ST_RESTRICTS_3_3_2_4,
				NULL, type, facet);
			ok = 0;
		    }
		    facet = facet->next;
		} while (facet != NULL);
		if (ok == 0)
		    return (XML_SCHEMAP_COS_ST_RESTRICTS_3_3_2_4);

	    }
	    /*
	    * SPEC (3.3.2.5) (same as 1.3.2)
	    *
	    * NOTE (3.3.2.5) This is currently done in
	    * xmlSchemaDeriveAndValidateFacets()
	    */
	}
    }

    return (0);
}

/**
 * xmlSchemaCheckSRCSimpleType:
 * @ctxt:  the schema parser context
 * @type:  the simple type definition
 *
 * Checks crc-simple-type constraints.
 *
 * Returns 0 if the constraints are satisfied,
 * if not a positive error code and -1 on internal
 * errors.
 */
static int
xmlSchemaCheckSRCSimpleType(xmlSchemaParserCtxtPtr ctxt,
			    xmlSchemaTypePtr type)
{
    /*
    * src-simple-type.1 The corresponding simple type definition, if any,
    * must satisfy the conditions set out in Constraints on Simple Type
    * Definition Schema Components (§3.14.6).
    */
    if ((xmlSchemaCheckSTPropsCorrect(ctxt, type) != 0) ||
	(xmlSchemaCheckCOSSTRestricts(ctxt, type) != 0)) {
	/*
	* TODO: Removed this, since it got annoying to get an
	* extra error report, if anything failed until now.
	* Enable this if needed.
	*/
	/*
	xmlSchemaPErr(ctxt, type->node,
	    XML_SCHEMAP_SRC_SIMPLE_TYPE_1,
	    "Simple type '%s' does not satisfy the constraints "
	    "on simple type definitions.\n",
	    type->name, NULL);
	*/
	return (XML_SCHEMAP_SRC_SIMPLE_TYPE_1);
    }

    if (type->flags & XML_SCHEMAS_TYPE_DERIVATION_METHOD_RESTRICTION) {
	/*
	* src-simple-type.2 If the <restriction> alternative is chosen,
	* either it must have a base [attribute] or a <simpleType> among its
	* [children], but not both.
	*/
	/*
	* XML_SCHEMAP_SRC_SIMPLE_TYPE_2
	* NOTE: This is checked in the parse function of <restriction>.
	*/
    } else if (VARIETY_LIST(type)) {
	/* src-simple-type.3 If the <list> alternative is chosen, either it must have
	* an itemType [attribute] or a <simpleType> among its [children],
	* but not both.
	*
	* REMOVED: This is checked in the parse function of <list>.
	*/
    } else if (VARIETY_UNION(type)) {
	xmlSchemaTypeLinkPtr member;
	xmlSchemaTypePtr ancestor, anySimpleType;

	anySimpleType = xmlSchemaGetBuiltInType(XML_SCHEMAS_ANYSIMPLETYPE);

	/* src-simple-type.4 Circular union type definition is disallowed. That is, if
	* the <union> alternative is chosen, there must not be any entries
	* in the memberTypes [attribute] at any depth which resolve to the
	* component corresponding to the <simpleType>.
	*/
	member = type->memberTypes;
	while (member != NULL) {
	    ancestor = member->type;
	    while ((ancestor != NULL) && (ancestor->type != XML_SCHEMA_TYPE_BASIC)) {
		if (ancestor == type) {
		    xmlSchemaPCustomErr(ctxt,
			XML_SCHEMAP_SRC_SIMPLE_TYPE_4,
			NULL, type, NULL,
			"The definition is circular", NULL);
		    return (XML_SCHEMAP_SRC_SIMPLE_TYPE_4);
		}
		if (IS_NOT_TYPEFIXED(ancestor))
		    xmlSchemaTypeFixup(ancestor, ctxt,  NULL);
		if (VARIETY_LIST(ancestor)) {
		    /*
		    * TODO, FIXME: Although a list simple type must not have a union ST
		    * type as item type, which in turn has a list ST as member
		    * type, we will assume this here as well, since this check
		    * was not yet performed.
		    */
		}

		ancestor = ancestor->baseType;
	    }
	    member = member->next;
	}
    }

    return (0);
}

static int
xmlSchemaCreateVCtxtOnPCtxt(xmlSchemaParserCtxtPtr ctxt)
{
   if (ctxt->vctxt == NULL) {
	ctxt->vctxt = xmlSchemaNewValidCtxt(NULL);
	if (ctxt->vctxt == NULL) {
	    xmlSchemaPErr(ctxt, NULL,
		XML_SCHEMAP_INTERNAL,
		"Internal error: xmlSchemaCreateVCtxtOnPCtxt, "
		"failed to create a temp. validation context.\n",
		NULL, NULL);
	    return (-1);
	}
	/* TODO: Pass user data. */
	xmlSchemaSetValidErrors(ctxt->vctxt, ctxt->error, ctxt->warning, NULL);
    }
    return (0);
}

static int
xmlSchemaVCheckCVCSimpleType(xmlSchemaAbstractCtxtPtr actxt,
			     xmlNodePtr node,
			     xmlSchemaTypePtr type,
			     const xmlChar *value,
			     xmlSchemaValPtr *retVal,
			     int fireErrors,
			     int normalize,
			     int isNormalized);

/**
 * xmlSchemaParseCheckCOSValidDefault:
 * @pctxt:  the schema parser context
 * @type:  the simple type definition
 * @value: the default value
 * @node: an optional node (the holder of the value)
 *
 * Schema Component Constraint: Element Default Valid (Immediate)
 * (cos-valid-default)
 * This will be used by the parser only. For the validator there's
 * an other version.
 *
 * Returns 0 if the constraints are satisfied,
 * if not, a positive error code and -1 on internal
 * errors.
 */
static int
xmlSchemaParseCheckCOSValidDefault(xmlSchemaParserCtxtPtr pctxt,
				   xmlNodePtr node,
				   xmlSchemaTypePtr type,
				   const xmlChar *value,
				   xmlSchemaValPtr *val)
{
    int ret = 0;

    /*
    * cos-valid-default:
    * Schema Component Constraint: Element Default Valid (Immediate)
    * For a string to be a valid default with respect to a type
    * definition the appropriate case among the following must be true:
    */
    if IS_COMPLEX_TYPE(type) {
	/*
	* Complex type.
	*
	* SPEC (2.1) "its {content type} must be a simple type definition
	* or mixed."
	* SPEC (2.2.2) "If the {content type} is mixed, then the {content
	* type}'s particle must be ·emptiable· as defined by
	* Particle Emptiable (§3.9.6)."
	*/
	if ((! HAS_SIMPLE_CONTENT(type)) &&
	    ((! HAS_MIXED_CONTENT(type)) || (! IS_PARTICLE_EMPTIABLE(type)))) {
	    /* NOTE that this covers (2.2.2) as well. */
	    xmlSchemaPCustomErr(pctxt,
		XML_SCHEMAP_COS_VALID_DEFAULT_2_1,
		NULL, type, type->node,
		"For a string to be a valid default, the type definition "
		"must be a simple type or a complex type with mixed content "
		"and a particle emptiable", NULL);
	    return(XML_SCHEMAP_COS_VALID_DEFAULT_2_1);
	}
    }
    /*
    * 1 If the type definition is a simple type definition, then the string
    * must be ·valid· with respect to that definition as defined by String
    * Valid (§3.14.4).
    *
    * AND
    *
    * 2.2.1 If the {content type} is a simple type definition, then the
    * string must be ·valid· with respect to that simple type definition
    * as defined by String Valid (§3.14.4).
    */
    if (IS_SIMPLE_TYPE(type))
	ret = xmlSchemaVCheckCVCSimpleType((xmlSchemaAbstractCtxtPtr) pctxt, node,
	    type, value, val, 1, 1, 0);
    else if (HAS_SIMPLE_CONTENT(type))
	ret = xmlSchemaVCheckCVCSimpleType((xmlSchemaAbstractCtxtPtr) pctxt, node,
	    type->contentTypeDef, value, val, 1, 1, 0);
    else
	return (ret);

    if (ret < 0) {
	PERROR_INT("xmlSchemaParseCheckCOSValidDefault",
	    "calling xmlSchemaVCheckCVCSimpleType()");
    }

    return (ret);
}

/**
 * xmlSchemaCheckCTPropsCorrect:
 * @ctxt:  the schema parser context
 * @type:  the complex type definition
 *
 *.(4.6) Constraints on Complex Type Definition Schema Components
 * Schema Component Constraint:
 * Complex Type Definition Properties Correct (ct-props-correct)
 * STATUS: (seems) complete
 *
 * Returns 0 if the constraints are satisfied, a positive
 * error code if not and -1 if an internal error occured.
 */
static int
xmlSchemaCheckCTPropsCorrect(xmlSchemaParserCtxtPtr pctxt,
			     xmlSchemaTypePtr type)
{
    /*
    * TODO: Correct the error code; XML_SCHEMAP_SRC_CT_1 is used temporarily.
    *
    * SPEC (1) "The values of the properties of a complex type definition must
    * be as described in the property tableau in The Complex Type Definition
    * Schema Component (§3.4.1), modulo the impact of Missing
    * Sub-components (§5.3)."
    */
    if ((type->baseType != NULL) &&
	(IS_SIMPLE_TYPE(type->baseType)) &&
	((type->flags & XML_SCHEMAS_TYPE_DERIVATION_METHOD_EXTENSION) == 0)) {
	/*
	* SPEC (2) "If the {base type definition} is a simple type definition,
	* the {derivation method} must be extension."
	*/
	xmlSchemaPCustomErr(pctxt,
	    XML_SCHEMAP_SRC_CT_1,
	    NULL, type, NULL,
	    "If the base type is a simple type, the derivation method must be "
	    "'extension'", NULL);
	return (XML_SCHEMAP_SRC_CT_1);
    }
    /*
    * SPEC (3) "Circular definitions are disallowed, except for the ·ur-type
    * definition·. That is, it must be possible to reach the ·ur-type
    * definition by repeatedly following the {base type definition}."
    *
    * NOTE (3) is done in xmlSchemaCheckTypeDefCircular().
    *
    * SPEC (4) "Two distinct attribute declarations in the {attribute uses}
    * must not have identical {name}s and {target namespace}s."
    * SPEC (5) "Two distinct attribute declarations in the {attribute uses}
    * must not have {type definition}s which are or are derived from ID."
    *
    * NOTE (4) and (5) are done in xmlSchemaBuildAttributeValidation().
    */
    return (0);
}

static int
xmlSchemaAreEqualTypes(xmlSchemaTypePtr typeA,
		       xmlSchemaTypePtr typeB)
{
    /*
    * TODO: This should implement component-identity
    * in the future.
    */
    if ((typeA == NULL) || (typeB == NULL))
	return (0);
    return (typeA == typeB);
}

/**
 * xmlSchemaCheckCOSCTDerivedOK:
 * @ctxt:  the schema parser context
 * @type:  the to-be derived complex type definition
 * @baseType:  the base complex type definition
 * @set: the given set
 *
 * Schema Component Constraint:
 * Type Derivation OK (Complex) (cos-ct-derived-ok)
 *
 * STATUS: completed
 *
 * Returns 0 if the constraints are satisfied, or 1
 * if not.
 */
static int
xmlSchemaCheckCOSCTDerivedOK(xmlSchemaTypePtr type,
			     xmlSchemaTypePtr baseType,
			     int set)
{
    int equal = xmlSchemaAreEqualTypes(type, baseType);
    /* TODO: Error codes. */
    /*
    * SPEC "For a complex type definition (call it D, for derived)
    * to be validly derived from a type definition (call this
    * B, for base) given a subset of {extension, restriction}
    * all of the following must be true:"
    */
    if (! equal) {
	/*
	* SPEC (1) "If B and D are not the same type definition, then the
	* {derivation method} of D must not be in the subset."
	*/
	if (((set & SUBSET_EXTENSION) &&
	    (type->flags & XML_SCHEMAS_TYPE_DERIVATION_METHOD_EXTENSION)) ||
	    ((set & SUBSET_RESTRICTION) &&
	    (type->flags & XML_SCHEMAS_TYPE_DERIVATION_METHOD_RESTRICTION)))
	    return (1);
    } else {
	/*
	* SPEC (2.1) "B and D must be the same type definition."
	*/
	return (0);
    }
    /*
    * SPEC (2.2) "B must be D's {base type definition}."
    */
    if (type->baseType == baseType)
	return (0);
    /*
    * SPEC (2.3.1) "D's {base type definition} must not be the ·ur-type
    * definition·."
    */
    if (IS_ANYTYPE(type->baseType))
	return (1);

    if (IS_COMPLEX_TYPE(type->baseType)) {
	/*
	* SPEC (2.3.2.1) "If D's {base type definition} is complex, then it
	* must be validly derived from B given the subset as defined by this
	* constraint."
	*/
	return (xmlSchemaCheckCOSCTDerivedOK(type->baseType,
	    baseType, set));
    } else {
	/*
	* SPEC (2.3.2.2) "If D's {base type definition} is simple, then it
	* must be validly derived from B given the subset as defined in Type
	* Derivation OK (Simple) (§3.14.6).
	*/
	return (xmlSchemaCheckCOSSTDerivedOK(type->baseType, baseType, set));
    }
}

/**
 * xmlSchemaCheckCOSDerivedOK:
 * @type:  the derived simple type definition
 * @baseType:  the base type definition
 *
 * Calls:
 * Type Derivation OK (Simple) AND Type Derivation OK (Complex)
 *
 * Checks wheter @type can be validly derived from @baseType.
 *
 * Returns 0 on success, an positive error code otherwise.
 */
static int
xmlSchemaCheckCOSDerivedOK(xmlSchemaTypePtr type,
			   xmlSchemaTypePtr baseType,
			   int set)
{
    if (IS_SIMPLE_TYPE(type))
	return (xmlSchemaCheckCOSSTDerivedOK(type, baseType, set));
    else
	return (xmlSchemaCheckCOSCTDerivedOK(type, baseType, set));
}

/**
 * xmlSchemaCheckCOSCTExtends:
 * @ctxt:  the schema parser context
 * @type:  the complex type definition
 *
 * (3.4.6) Constraints on Complex Type Definition Schema Components
 * Schema Component Constraint:
 * Derivation Valid (Extension) (cos-ct-extends)
 *
 * STATUS:
 *   missing:
 *     (1.5)
 *     (1.4.3.2.2.2) "Particle Valid (Extension)", which is not really needed.
 *
 * Returns 0 if the constraints are satisfied, a positive
 * error code if not and -1 if an internal error occured.
 */
static int
xmlSchemaCheckCOSCTExtends(xmlSchemaParserCtxtPtr ctxt,
			   xmlSchemaTypePtr type)
{
    xmlSchemaTypePtr base = type->baseType;
    /*
    * TODO: Correct the error code; XML_SCHEMAP_COS_CT_EXTENDS_1_1 is used
    * temporarily only.
    */
    /*
    * SPEC (1) "If the {base type definition} is a complex type definition,
    * then all of the following must be true:"
    */
    if (base->type == XML_SCHEMA_TYPE_COMPLEX) {
	/*
	* SPEC (1.1) "The {final} of the {base type definition} must not
	* contain extension."
	*/
	if (base->flags & XML_SCHEMAS_TYPE_FINAL_EXTENSION) {
	    xmlSchemaPCustomErr(ctxt,
		XML_SCHEMAP_COS_CT_EXTENDS_1_1,
		NULL, type, NULL,
		"The 'final' of the base type definition "
		"contains 'extension'", NULL);
	    return (XML_SCHEMAP_COS_CT_EXTENDS_1_1);
	}
	/*
	* SPEC (1.2) "Its {attribute uses} must be a subset of the {attribute
	* uses}
	* of the complex type definition itself, that is, for every attribute
	* use in the {attribute uses} of the {base type definition}, there
	* must be an attribute use in the {attribute uses} of the complex
	* type definition itself whose {attribute declaration} has the same
	* {name}, {target namespace} and {type definition} as its attribute
	* declaration"
	*
	* NOTE (1.2): This will be already satisfied by the way the attribute
	* uses are extended in xmlSchemaBuildAttributeValidation(); thus this
	* check is not needed.
	*/

	/*
	* SPEC (1.3) "If it has an {attribute wildcard}, the complex type
	* definition must also have one, and the base type definition's
	* {attribute  wildcard}'s {namespace constraint} must be a subset
	* of the complex  type definition's {attribute wildcard}'s {namespace
	* constraint}, as defined by Wildcard Subset (§3.10.6)."
	*
	* NOTE (1.3) This is already checked in
	* xmlSchemaBuildAttributeValidation; thus this check is not needed.
	*
	* SPEC (1.4) "One of the following must be true:"
	*/
	if ((type->contentTypeDef != NULL) &&
	    (type->contentTypeDef == base->contentTypeDef)) {
	    /*
	    * SPEC (1.4.1) "The {content type} of the {base type definition}
	    * and the {content type} of the complex type definition itself
	    * must be the same simple type definition"
	    * PASS
	    */
	} else if ((type->contentType == XML_SCHEMA_CONTENT_EMPTY) &&
	    (base->contentType == XML_SCHEMA_CONTENT_EMPTY) ) {
	    /*
	    * SPEC (1.4.2) "The {content type} of both the {base type
	    * definition} and the complex type definition itself must
	    * be empty."
	    * PASS
	    */
	} else {
	    /*
	    * SPEC (1.4.3) "All of the following must be true:"
	    */
	    if (type->subtypes == NULL) {
		/*
		* SPEC 1.4.3.1 The {content type} of the complex type
		* definition itself must specify a particle.
		*/
		xmlSchemaPCustomErr(ctxt,
		    XML_SCHEMAP_COS_CT_EXTENDS_1_1,
		    NULL, type, NULL,
		    "The content type must specify a particle", NULL);
		return (XML_SCHEMAP_COS_CT_EXTENDS_1_1);
	    }
	    /*
	    * SPEC (1.4.3.2) "One of the following must be true:"
	    */
	    if (base->contentType == XML_SCHEMA_CONTENT_EMPTY) {
		/*
		* SPEC (1.4.3.2.1) "The {content type} of the {base type
		* definition} must be empty.
		* PASS
		*/
	    } else {
		/*
		* SPEC (1.4.3.2.2) "All of the following must be true:"
		*/
		if ((type->contentType != base->contentType) ||
		    ((type->contentType != XML_SCHEMA_CONTENT_MIXED) &&
		    (type->contentType != XML_SCHEMA_CONTENT_ELEMENTS))) {
		    /*
		    * SPEC (1.4.3.2.2.1) "Both {content type}s must be mixed
		    * or both must be element-only."
		    */
		    xmlSchemaPCustomErr(ctxt,
			XML_SCHEMAP_COS_CT_EXTENDS_1_1,
			NULL, type, NULL,
			"The content type of both, the type and its base "
			"type, must either 'mixed' or 'element-only'", NULL);
		    return (XML_SCHEMAP_COS_CT_EXTENDS_1_1);
		}
		/*
		* FUTURE TODO SPEC (1.4.3.2.2.2) "The particle of the
		* complex type definition must be a ·valid extension·
		* of the {base type definition}'s particle, as defined
		* in Particle Valid (Extension) (§3.9.6)."
		*
		* NOTE that we won't check "Particle Valid (Extension)",
		* since it is ensured by the derivation process in
		* xmlSchemaTypeFixup(). We need to implement this when heading
		* for a construction API
		*/
	    }
	    /*
	    * TODO (1.5)
	    */
	}
    } else {
	/*
	* SPEC (2) "If the {base type definition} is a simple type definition,
	* then all of the following must be true:"
	*/
	if (type->contentTypeDef != base) {
	    /*
	    * SPEC (2.1) "The {content type} must be the same simple type
	    * definition."
	    */
	    xmlSchemaPCustomErr(ctxt,
		XML_SCHEMAP_COS_CT_EXTENDS_1_1,
		NULL, type, NULL,
		"The content type must be the simple base type", NULL);
	    return (XML_SCHEMAP_COS_CT_EXTENDS_1_1);
	}
	if (base->flags & XML_SCHEMAS_TYPE_FINAL_EXTENSION) {
	    /*
	    * SPEC (2.2) "The {final} of the {base type definition} must not
	    * contain extension"
	    * NOTE that this is the same as (1.1).
	    */
	    xmlSchemaPCustomErr(ctxt,
		XML_SCHEMAP_COS_CT_EXTENDS_1_1,
		NULL, type, NULL,
		"The 'final' of the base type definition "
		"contains 'extension'", NULL);
	    return (XML_SCHEMAP_COS_CT_EXTENDS_1_1);
	}
    }
    return (0);
}

/**
 * xmlSchemaCheckDerivationOKRestriction:
 * @ctxt:  the schema parser context
 * @type:  the complex type definition
 *
 * (3.4.6) Constraints on Complex Type Definition Schema Components
 * Schema Component Constraint:
 * Derivation Valid (Restriction, Complex) (derivation-ok-restriction)
 *
 * STATUS:
 *   missing:
 *     (5.4.2), (5.2.2.1)
 *
 * Returns 0 if the constraints are satisfied, a positive
 * error code if not and -1 if an internal error occured.
 */
static int
xmlSchemaCheckDerivationOKRestriction(xmlSchemaParserCtxtPtr ctxt,
				      xmlSchemaTypePtr type)
{
    xmlSchemaTypePtr base;

    /*
    * TODO: Correct the error code; XML_SCHEMAP_COS_CT_EXTENDS_1_1 is used
    * temporarily only.
    */
    base = type->baseType;
    if (base->flags & XML_SCHEMAS_TYPE_FINAL_RESTRICTION) {
	/*
	* SPEC (1) "The {base type definition} must be a complex type
	* definition whose {final} does not contain restriction."
	*/
	xmlSchemaPCustomErr(ctxt,
	    XML_SCHEMAP_COS_CT_EXTENDS_1_1,
	    NULL, type, NULL,
	    "The 'final' of the base type definition "
	    "contains 'restriction'", NULL);
	return (XML_SCHEMAP_COS_CT_EXTENDS_1_1);
    }
    /*
    * NOTE (3) and (4) are done in xmlSchemaBuildAttributeValidation().
    *
    * SPEC (5) "One of the following must be true:"
    */
    if (base->builtInType == XML_SCHEMAS_ANYTYPE) {
	/*
	* SPEC (5.1) "The {base type definition} must be the
	* ·ur-type definition·."
	* PASS
	*/
    } else if ((type->contentType == XML_SCHEMA_CONTENT_SIMPLE) ||
	    (type->contentType == XML_SCHEMA_CONTENT_BASIC)) {
	/*
	* SPEC (5.2.1) "The {content type} of the complex type definition
	* must be a simple type definition"
	*
	* SPEC (5.2.2) "One of the following must be true:"
	*/
	if ((base->contentType == XML_SCHEMA_CONTENT_SIMPLE) ||
	    (base->contentType == XML_SCHEMA_CONTENT_BASIC)) {
	    /*
	    * SPEC (5.2.2.1) "The {content type} of the {base type
	    * definition} must be a simple type definition from which
	    * the {content type} is validly derived given the empty
	    * set as defined in Type Derivation OK (Simple) (§3.14.6)."
	    * URGENT TODO
	    */
	} else if ((base->contentType == XML_SCHEMA_CONTENT_MIXED) &&
	    (xmlSchemaIsParticleEmptiable(
		(xmlSchemaParticlePtr) base->subtypes))) {
	    /*
	    * SPEC (5.2.2.2) "The {base type definition} must be mixed
	    * and have a particle which is ·emptiable· as defined in
	    * Particle Emptiable (§3.9.6)."
	    * PASS
	    */
	} else {
	    xmlSchemaPCustomErr(ctxt,
		XML_SCHEMAP_COS_CT_EXTENDS_1_1,
		NULL, type, NULL,
		"The content type of the base type must be either "
		"a simple type or 'mixed' and an emptiable particle", NULL);
	    return (XML_SCHEMAP_COS_CT_EXTENDS_1_1);
	}
    } else if (type->contentType == XML_SCHEMA_CONTENT_EMPTY) {
	/*
	* SPEC (5.3.1) "The {content type} of the complex type itself must
	* be empty"
	*/
	if (base->contentType == XML_SCHEMA_CONTENT_EMPTY) {
	    /*
	    * SPEC (5.3.2.1) "The {content type} of the {base type
	    * definition} must also be empty."
	    * PASS
	    */
	} else if (((base->contentType == XML_SCHEMA_CONTENT_ELEMENTS) ||
	    (base->contentType == XML_SCHEMA_CONTENT_MIXED)) &&
	    xmlSchemaIsParticleEmptiable(
		(xmlSchemaParticlePtr) base->subtypes)) {
	    /*
	    * SPEC (5.3.2.2) "The {content type} of the {base type
	    * definition} must be elementOnly or mixed and have a particle
	    * which is ·emptiable· as defined in Particle Emptiable (§3.9.6)."
	    * PASS
	    */
	} else {
	    xmlSchemaPCustomErr(ctxt,
		XML_SCHEMAP_COS_CT_EXTENDS_1_1,
		NULL, type, NULL,
		"The content type of the base type must be either "
		"empty or 'mixed' (or 'elements-only') and an emptiable "
		"particle", NULL);
	    return (XML_SCHEMAP_COS_CT_EXTENDS_1_1);
	}
    } else if ((type->contentType == XML_SCHEMA_CONTENT_ELEMENTS) ||
	HAS_MIXED_CONTENT(type)) {
	/*
	* SPEC (5.4.1.1) "The {content type} of the complex type definition
	* itself must be element-only"
	*/	 
	if (HAS_MIXED_CONTENT(type) && (! HAS_MIXED_CONTENT(base))) {
	    /*
	    * SPEC (5.4.1.2) "The {content type} of the complex type
	    * definition itself and of the {base type definition} must be
	    * mixed"
	    */
	    xmlSchemaPCustomErr(ctxt,
		XML_SCHEMAP_COS_CT_EXTENDS_1_1,
		NULL, type, NULL,
		"If the content type is 'mixed', then the content type of the "
		"base type must also be 'mixed'", NULL);
	    return (XML_SCHEMAP_COS_CT_EXTENDS_1_1);
	}
	/*
	* SPEC (5.4.2) "The particle of the complex type definition itself
	* must be a ·valid restriction· of the particle of the {content
	* type} of the {base type definition} as defined in Particle Valid
	* (Restriction) (§3.9.6).
	*
	* URGENT TODO: (5.4.2)
	*/
    } else {
	xmlSchemaPCustomErr(ctxt,
	    XML_SCHEMAP_COS_CT_EXTENDS_1_1,
	    NULL, type, NULL,
	    "The type is not a valid restriction of its base type", NULL);
	return (XML_SCHEMAP_COS_CT_EXTENDS_1_1);
    }
    return (0);
}

/**
 * xmlSchemaCheckCTComponent:
 * @ctxt:  the schema parser context
 * @type:  the complex type definition
 *
 * (3.4.6) Constraints on Complex Type Definition Schema Components
 *
 * Returns 0 if the constraints are satisfied, a positive
 * error code if not and -1 if an internal error occured.
 */
static int
xmlSchemaCheckCTComponent(xmlSchemaParserCtxtPtr ctxt,
			  xmlSchemaTypePtr type)
{
    int ret;
    /*
    * Complex Type Definition Properties Correct
    */
    ret = xmlSchemaCheckCTPropsCorrect(ctxt, type);
    if (ret != 0)
	return (ret);
    if (type->flags & XML_SCHEMAS_TYPE_DERIVATION_METHOD_EXTENSION)
	ret = xmlSchemaCheckCOSCTExtends(ctxt, type);
    else
	ret = xmlSchemaCheckDerivationOKRestriction(ctxt, type);
    return (ret);
}

/**
 * xmlSchemaCheckSRCCT:
 * @ctxt:  the schema parser context
 * @type:  the complex type definition
 *
 * (3.4.3) Constraints on XML Representations of Complex Type Definitions:
 * Schema Representation Constraint:
 * Complex Type Definition Representation OK (src-ct)
 *
 * Returns 0 if the constraints are satisfied, a positive
 * error code if not and -1 if an internal error occured.
 */
static int
xmlSchemaCheckSRCCT(xmlSchemaParserCtxtPtr ctxt,
		    xmlSchemaTypePtr type)
{
    xmlSchemaTypePtr base;
    int ret = 0;

    /*
    * TODO: Adjust the error codes here, as I used
    * XML_SCHEMAP_SRC_CT_1 only yet.
    */
    base = type->baseType;
    if (! HAS_SIMPLE_CONTENT(type)) {
	/*
	* 1 If the <complexContent> alternative is chosen, the type definition
	* ·resolved· to by the ·actual value· of the base [attribute]
	* must be a complex type definition;
	*/
	if (! IS_COMPLEX_TYPE(base)) {
	    xmlChar *str = NULL;
	    xmlSchemaPCustomErr(ctxt,
		XML_SCHEMAP_SRC_CT_1,
		NULL, type, type->node,
		"If using <complexContent>, the base type is expected to be "
		"a complex type. The base type '%s' is a simple type",
		xmlSchemaFormatQName(&str, base->targetNamespace,
		base->name));
	    FREE_AND_NULL(str)
	    return (XML_SCHEMAP_SRC_CT_1);
	}
    } else {
	/*
	* SPEC
	* 2 If the <simpleContent> alternative is chosen, all of the
	* following must be true:
	* 2.1 The type definition ·resolved· to by the ·actual value· of the
	* base [attribute] must be one of the following:
	*/
	if (IS_SIMPLE_TYPE(base)) {
	    if ((type->flags &
		XML_SCHEMAS_TYPE_DERIVATION_METHOD_EXTENSION) == 0) {
		xmlChar *str = NULL;
		/*
		* 2.1.3 only if the <extension> alternative is also
		* chosen, a simple type definition.
		*/
		/* TODO: Change error code to ..._SRC_CT_2_1_3. */
		xmlSchemaPCustomErr(ctxt,
		    XML_SCHEMAP_SRC_CT_1,
		    NULL, type, NULL,
		    "If using <simpleContent> and <restriction>, the base "
		    "type must be a complex type. The base type '%s' is "
		    "a simple type",
		    xmlSchemaFormatQName(&str, base->targetNamespace,
			base->name));
		FREE_AND_NULL(str)
		return (XML_SCHEMAP_SRC_CT_1);
	    }
	} else {
	    /* Base type is a complex type. */
	    if ((base->contentType == XML_SCHEMA_CONTENT_SIMPLE) ||
		(base->contentType == XML_SCHEMA_CONTENT_BASIC)) {
		/*
		* 2.1.1 a complex type definition whose {content type} is a
		* simple type definition;
		* PASS
		*/
		if (base->contentTypeDef == NULL) {
		    xmlSchemaPCustomErr(ctxt, XML_SCHEMAP_INTERNAL,
			NULL, type, NULL,
			"Internal error: xmlSchemaCheckSRCCT, "
			"'%s', base type has no content type",
			type->name);
		    return (-1);
		}
	    } else if ((base->contentType == XML_SCHEMA_CONTENT_MIXED) &&
		(type->flags &
		    XML_SCHEMAS_TYPE_DERIVATION_METHOD_RESTRICTION)) {

		/*
		* 2.1.2 only if the <restriction> alternative is also
		* chosen, a complex type definition whose {content type}
		* is mixed and a particle emptiable.
		*/
		if (! xmlSchemaIsParticleEmptiable(
		    (xmlSchemaParticlePtr) base->subtypes)) {
		    ret = XML_SCHEMAP_SRC_CT_1;
		} else 
		    /*
		    * Attention: at this point the <simpleType> child is in
		    * ->contentTypeDef (put there during parsing).
		    */		    
		    if (type->contentTypeDef == NULL) {
		    xmlChar *str = NULL;
		    /*
		    * 2.2 If clause 2.1.2 above is satisfied, then there
		    * must be a <simpleType> among the [children] of
		    * <restriction>.
		    */
		    /* TODO: Change error code to ..._SRC_CT_2_2. */
		    xmlSchemaPCustomErr(ctxt,
			XML_SCHEMAP_SRC_CT_1,
			NULL, type, NULL,
			"A <simpleType> is expected among the children "
			"of <restriction>, if <simpleContent> is used and "
			"the base type '%s' is a complex type",
			xmlSchemaFormatQName(&str, base->targetNamespace,
			base->name));
		    FREE_AND_NULL(str)
		    return (XML_SCHEMAP_SRC_CT_1);
		}
	    } else {
		ret = XML_SCHEMAP_SRC_CT_1;
	    }
	}
	if (ret > 0) {
	    xmlChar *str = NULL;
	    if (type->flags & XML_SCHEMAS_TYPE_DERIVATION_METHOD_RESTRICTION) {
		xmlSchemaPCustomErr(ctxt,
		    XML_SCHEMAP_SRC_CT_1,
		    NULL, type, NULL,
		    "If <simpleContent> and <restriction> is used, the "
		    "base type must be a simple type or a complex type with "
		    "mixed content and particle emptiable. The base type "
		    "'%s' is none of those",
		    xmlSchemaFormatQName(&str, base->targetNamespace,
		    base->name));
	    } else {
		xmlSchemaPCustomErr(ctxt,
		    XML_SCHEMAP_SRC_CT_1,
		    NULL, type, NULL,
		    "If <simpleContent> and <extension> is used, the "
		    "base type must be a simple type. The base type '%s' "
		    "is a complex type",
		    xmlSchemaFormatQName(&str, base->targetNamespace,
		    base->name));
	    }
	    FREE_AND_NULL(str)
	}
    }
    /*
    * SPEC (3) "The corresponding complex type definition component must
    * satisfy the conditions set out in Constraints on Complex Type
    * Definition Schema Components (§3.4.6);"
    * NOTE (3) will be done in xmlSchemaTypeFixup().
    */
    /*
    * SPEC (4) If clause 2.2.1 or clause 2.2.2 in the correspondence specification
    * above for {attribute wildcard} is satisfied, the intensional
    * intersection must be expressible, as defined in Attribute Wildcard
    * Intersection (§3.10.6).
    * NOTE (4) is done in xmlSchemaBuildAttributeValidation().
    */
    return (ret);
}

#ifdef ENABLE_PARTICLE_RESTRICTION
/**
 * xmlSchemaCheckParticleRangeOK:
 * @ctxt:  the schema parser context
 * @type:  the complex type definition
 *
 * (3.9.6) Constraints on Particle Schema Components
 * Schema Component Constraint:
 * Occurrence Range OK (range-ok)
 *
 * STATUS: complete
 *
 * Returns 0 if the constraints are satisfied, a positive
 * error code if not and -1 if an internal error occured.
 */
static int
xmlSchemaCheckParticleRangeOK(int rmin, int rmax,
			      int bmin, int bmax)
{
    if (rmin < bmin)
	return (1);
    if ((bmax != UNBOUNDED) &&
	(rmax > bmax))
	return (1);
    return (0);
}

/**
 * xmlSchemaCheckRCaseNameAndTypeOK:
 * @ctxt:  the schema parser context
 * @r: the restricting element declaration particle
 * @b: the base element declaration particle
 *
 * (3.9.6) Constraints on Particle Schema Components
 * Schema Component Constraint:
 * Particle Restriction OK (Elt:Elt -- NameAndTypeOK)
 * (rcase-NameAndTypeOK)
 *
 * STATUS:
 *   MISSING (3.2.3)
 *   CLARIFY: (3.2.2)
 *
 * Returns 0 if the constraints are satisfied, a positive
 * error code if not and -1 if an internal error occured.
 */
static int
xmlSchemaCheckRCaseNameAndTypeOK(xmlSchemaParserCtxtPtr ctxt,
				 xmlSchemaParticlePtr r,
				 xmlSchemaParticlePtr b)
{
    xmlSchemaElementPtr elemR, elemB;

    /* TODO: Error codes (rcase-NameAndTypeOK). */
    elemR = (xmlSchemaElementPtr) r->children;
    elemB = (xmlSchemaElementPtr) b->children;
    /*
    * SPEC (1) "The declarations' {name}s and {target namespace}s are
    * the same."
    */
    if ((elemR != elemB) &&
	((! xmlStrEqual(elemR->name, elemB->name)) ||
	(! xmlStrEqual(elemR->targetNamespace, elemB->targetNamespace))))
	return (1);
    /*
    * SPEC (2) "R's occurrence range is a valid restriction of B's
    * occurrence range as defined by Occurrence Range OK (§3.9.6)."
    */
    if (xmlSchemaCheckParticleRangeOK(r->minOccurs, r->maxOccurs,
	    b->minOccurs, b->maxOccurs) != 0)
	return (1);
    /*
    * SPEC (3.1) "Both B's declaration's {scope} and R's declaration's
    * {scope} are global."
    */
    if (elemR == elemB)
	return (0);
    /*
    * SPEC (3.2.1) "Either B's {nillable} is true or R's {nillable} is false."
    */
    if (((elemB->flags & XML_SCHEMAS_ELEM_NILLABLE) == 0) &&
	(elemR->flags & XML_SCHEMAS_ELEM_NILLABLE))
	 return (1);
    /*
    * SPEC (3.2.2) "either B's declaration's {value constraint} is absent,
    * or is not fixed, or R's declaration's {value constraint} is fixed
    * with the same value."
    */
    if ((elemB->value != NULL) && (elemB->flags & XML_SCHEMAS_ELEM_FIXED) &&
	((elemR->value == NULL) ||
	 ((elemR->flags & XML_SCHEMAS_ELEM_FIXED) == 0) ||
	 /* TODO: Equality of the initial value or normalized or canonical? */
	 (! xmlStrEqual(elemR->value, elemB->value))))
	 return (1);
    /*
    * TODO: SPEC (3.2.3) "R's declaration's {identity-constraint
    * definitions} is a subset of B's declaration's {identity-constraint
    * definitions}, if any."
    */
    if (elemB->idcs != NULL) {
	/* TODO */
    }
    /*
    * SPEC (3.2.4) "R's declaration's {disallowed substitutions} is a
    * superset of B's declaration's {disallowed substitutions}."
    */
    if (((elemB->flags & XML_SCHEMAS_ELEM_BLOCK_EXTENSION) &&
	 ((elemR->flags & XML_SCHEMAS_ELEM_BLOCK_EXTENSION) == 0)) ||
	((elemB->flags & XML_SCHEMAS_ELEM_BLOCK_RESTRICTION) &&
	 ((elemR->flags & XML_SCHEMAS_ELEM_BLOCK_RESTRICTION) == 0)) ||
	((elemB->flags & XML_SCHEMAS_ELEM_BLOCK_SUBSTITUTION) &&
	 ((elemR->flags & XML_SCHEMAS_ELEM_BLOCK_SUBSTITUTION) == 0)))
	 return (1);
    /*
    * SPEC (3.2.5) "R's {type definition} is validly derived given
    * {extension, list, union} from B's {type definition}"
    *
    * BADSPEC TODO: What's the point of adding "list" and "union" to the
    * set, if the corresponding constraints handle "restriction" and
    * "extension" only?
    *
    */
    {
	int set = 0;

	set |= SUBSET_EXTENSION;
	set |= SUBSET_LIST;
	set |= SUBSET_UNION;
	if (xmlSchemaCheckCOSDerivedOK(elemR->subtypes,
	    elemB->subtypes, set) != 0)
	    return (1);
    }
    return (0);
}

/**
 * xmlSchemaCheckRCaseNSCompat:
 * @ctxt:  the schema parser context
 * @r: the restricting element declaration particle
 * @b: the base wildcard particle
 *
 * (3.9.6) Constraints on Particle Schema Components
 * Schema Component Constraint:
 * Particle Derivation OK (Elt:Any -- NSCompat)
 * (rcase-NSCompat)
 *
 * STATUS: complete
 *
 * Returns 0 if the constraints are satisfied, a positive
 * error code if not and -1 if an internal error occured.
 */
static int
xmlSchemaCheckRCaseNSCompat(xmlSchemaParserCtxtPtr ctxt,
			    xmlSchemaParticlePtr r,
			    xmlSchemaParticlePtr b)
{
    /* TODO:Error codes (rcase-NSCompat). */
    /*
    * SPEC "For an element declaration particle to be a ·valid restriction·
    * of a wildcard particle all of the following must be true:"
    *
    * SPEC (1) "The element declaration's {target namespace} is ·valid·
    * with respect to the wildcard's {namespace constraint} as defined by
    * Wildcard allows Namespace Name (§3.10.4)."
    */
    if (xmlSchemaCheckCVCWildcardNamespace((xmlSchemaWildcardPtr) b->children,
	((xmlSchemaElementPtr) r->children)->targetNamespace) != 0)
	return (1);
    /*
    * SPEC (2) "R's occurrence range is a valid restriction of B's
    * occurrence range as defined by Occurrence Range OK (§3.9.6)."
    */
    if (xmlSchemaCheckParticleRangeOK(r->minOccurs, r->maxOccurs,
	    b->minOccurs, b->maxOccurs) != 0)
	return (1);

    return (0);
}

/**
 * xmlSchemaCheckRCaseRecurseAsIfGroup:
 * @ctxt:  the schema parser context
 * @r: the restricting element declaration particle
 * @b: the base model group particle
 *
 * (3.9.6) Constraints on Particle Schema Components
 * Schema Component Constraint:
 * Particle Derivation OK (Elt:All/Choice/Sequence -- RecurseAsIfGroup)
 * (rcase-RecurseAsIfGroup)
 *
 * STATUS: TODO
 *
 * Returns 0 if the constraints are satisfied, a positive
 * error code if not and -1 if an internal error occured.
 */
static int
xmlSchemaCheckRCaseRecurseAsIfGroup(xmlSchemaParserCtxtPtr ctxt,
				    xmlSchemaParticlePtr r,
				    xmlSchemaParticlePtr b)
{
    /* TODO: Error codes (rcase-RecurseAsIfGroup). */
    TODO
    return (0);
}

/**
 * xmlSchemaCheckRCaseNSSubset:
 * @ctxt:  the schema parser context
 * @r: the restricting wildcard particle
 * @b: the base wildcard particle
 *
 * (3.9.6) Constraints on Particle Schema Components
 * Schema Component Constraint:
 * Particle Derivation OK (Any:Any -- NSSubset)
 * (rcase-NSSubset)
 *
 * STATUS: complete
 *
 * Returns 0 if the constraints are satisfied, a positive
 * error code if not and -1 if an internal error occured.
 */
static int
xmlSchemaCheckRCaseNSSubset(xmlSchemaParserCtxtPtr ctxt,
				    xmlSchemaParticlePtr r,
				    xmlSchemaParticlePtr b,
				    int isAnyTypeBase)
{
    /* TODO: Error codes (rcase-NSSubset). */
    /*
    * SPEC (1) "R's occurrence range is a valid restriction of B's
    * occurrence range as defined by Occurrence Range OK (§3.9.6)."
    */
    if (xmlSchemaCheckParticleRangeOK(r->minOccurs, r->maxOccurs,
	    b->minOccurs, b->maxOccurs))
	return (1);
    /*
    * SPEC (2) "R's {namespace constraint} must be an intensional subset
    * of B's {namespace constraint} as defined by Wildcard Subset (§3.10.6)."
    */
    if (xmlSchemaCheckCOSNSSubset((xmlSchemaWildcardPtr) r->children,
	(xmlSchemaWildcardPtr) b->children))
	return (1);
    /*
    * SPEC (3) "Unless B is the content model wildcard of the ·ur-type
    * definition·, R's {process contents} must be identical to or stronger
    * than B's {process contents}, where strict is stronger than lax is
    * stronger than skip."
    */
    if (! isAnyTypeBase) {
	if ( ((xmlSchemaWildcardPtr) r->children)->processContents <
	    ((xmlSchemaWildcardPtr) b->children)->processContents)
	    return (1);
    }

    return (0);
}

/**
 * xmlSchemaCheckCOSParticleRestrict:
 * @ctxt:  the schema parser context
 * @type:  the complex type definition
 *
 * (3.9.6) Constraints on Particle Schema Components
 * Schema Component Constraint:
 * Particle Valid (Restriction) (cos-particle-restrict)
 *
 * STATUS: TODO
 *
 * Returns 0 if the constraints are satisfied, a positive
 * error code if not and -1 if an internal error occured.
 */
static int
xmlSchemaCheckCOSParticleRestrict(xmlSchemaParserCtxtPtr ctxt,
				  xmlSchemaParticlePtr r,
				  xmlSchemaParticlePtr b)
{
    int ret = 0;

    /*part = GET_PARTICLE(type);
    basePart = GET_PARTICLE(base);
    */

    TODO

    /*
    * SPEC (1) "They are the same particle."
    */
    if (r == b)
	return (0);


    return (0);
}

/**
 * xmlSchemaCheckRCaseNSRecurseCheckCardinality:
 * @ctxt:  the schema parser context
 * @r: the model group particle
 * @b: the base wildcard particle
 *
 * (3.9.6) Constraints on Particle Schema Components
 * Schema Component Constraint:
 * Particle Derivation OK (All/Choice/Sequence:Any --
 *                         NSRecurseCheckCardinality)
 * (rcase-NSRecurseCheckCardinality)
 *
 * STATUS: TODO: subst-groups
 *
 * Returns 0 if the constraints are satisfied, a positive
 * error code if not and -1 if an internal error occured.
 */
static int
xmlSchemaCheckRCaseNSRecurseCheckCardinality(xmlSchemaParserCtxtPtr ctxt,
					     xmlSchemaParticlePtr r,
					     xmlSchemaParticlePtr b)
{
    xmlSchemaParticlePtr part;
    /* TODO: Error codes (rcase-NSRecurseCheckCardinality). */
    if ((r->children == NULL) || (r->children->children == NULL))
	return (-1);
    /*
    * SPEC "For a group particle to be a ·valid restriction· of a
    * wildcard particle..."
    *
    * SPEC (1) "Every member of the {particles} of the group is a ·valid
    * restriction· of the wildcard as defined by
    * Particle Valid (Restriction) (§3.9.6)."
    */
    part = (xmlSchemaParticlePtr) r->children->children;
    do {
	if (xmlSchemaCheckCOSParticleRestrict(ctxt, part, b))
	    return (1);
	part = (xmlSchemaParticlePtr) part->next;
    } while (part != NULL);
    /*
    * SPEC (2) "The effective total range of the group [...] is a
    * valid restriction of B's occurrence range as defined by
    * Occurrence Range OK (§3.9.6)."
    */
    if (xmlSchemaCheckParticleRangeOK(
	    xmlSchemaGetParticleTotalRangeMin(r),
	    xmlSchemaGetParticleTotalRangeMax(r),
	    b->minOccurs, b->maxOccurs) != 0)
	return (1);
    return (0);
}

/**
 * xmlSchemaCheckRCaseRecurse:
 * @ctxt:  the schema parser context
 * @r: the <all> or <sequence> model group particle
 * @b: the base <all> or <sequence> model group particle
 *
 * (3.9.6) Constraints on Particle Schema Components
 * Schema Component Constraint:
 * Particle Derivation OK (All:All,Sequence:Sequence --
                           Recurse)
 * (rcase-Recurse)
 *
 * STATUS:  ?
 * TODO: subst-groups
 *
 * Returns 0 if the constraints are satisfied, a positive
 * error code if not and -1 if an internal error occured.
 */
static int
xmlSchemaCheckRCaseRecurse(xmlSchemaParserCtxtPtr ctxt,
			   xmlSchemaParticlePtr r,
			   xmlSchemaParticlePtr b)
{
    /* xmlSchemaParticlePtr part; */
    /* TODO: Error codes (rcase-Recurse). */
    if ((r->children == NULL) || (b->children == NULL) ||
	(r->children->type != b->children->type))
	return (-1);
    /*
    * SPEC "For an all or sequence group particle to be a ·valid
    * restriction· of another group particle with the same {compositor}..."
    *
    * SPEC (1) "R's occurrence range is a valid restriction of B's
    * occurrence range as defined by Occurrence Range OK (§3.9.6)."
    */
    if (xmlSchemaCheckParticleRangeOK(r->minOccurs, r->maxOccurs,
	    b->minOccurs, b->maxOccurs))
	return (1);


    return (0);
}

#endif

#define FACET_RESTR_MUTUAL_ERR(fac1, fac2) \
    xmlSchemaPCustomErrExt(pctxt,      \
	XML_SCHEMAP_INVALID_FACET_VALUE, \
	NULL, (xmlSchemaTypePtr) fac1, fac1->node, \
	"It is an error for both '%s' and '%s' to be specified on the "\
	"same type definition", \
	BAD_CAST xmlSchemaFacetTypeToString(fac1->type), \
	BAD_CAST xmlSchemaFacetTypeToString(fac2->type), NULL);

#define FACET_RESTR_ERR(fac1, msg) \
    xmlSchemaPCustomErr(pctxt,      \
	XML_SCHEMAP_INVALID_FACET_VALUE, \
	NULL, (xmlSchemaTypePtr) fac1, fac1->node, \
	msg, NULL);

#define FACET_RESTR_FIXED_ERR(fac) \
    xmlSchemaPCustomErr(pctxt, \
	XML_SCHEMAP_INVALID_FACET_VALUE, \
	NULL, (xmlSchemaTypePtr) fac, fac->node, \
	"The base type's facet is 'fixed', thus the value must not " \
	"differ", NULL);

static void
xmlSchemaDeriveFacetErr(xmlSchemaParserCtxtPtr pctxt,
			xmlSchemaFacetPtr facet1,
			xmlSchemaFacetPtr facet2,
			int lessGreater,
			int orEqual,
			int ofBase)
{
    xmlChar *msg = NULL;

    msg = xmlStrdup(BAD_CAST "'");
    msg = xmlStrcat(msg, xmlSchemaFacetTypeToString(facet1->type));
    msg = xmlStrcat(msg, BAD_CAST "' has to be");
    if (lessGreater == 0)
	msg = xmlStrcat(msg, BAD_CAST " equal to");
    if (lessGreater == 1)
	msg = xmlStrcat(msg, BAD_CAST " greater than");
    else
	msg = xmlStrcat(msg, BAD_CAST " less than");

    if (orEqual)
	msg = xmlStrcat(msg, BAD_CAST " or equal to");
    msg = xmlStrcat(msg, BAD_CAST " '");
    msg = xmlStrcat(msg, xmlSchemaFacetTypeToString(facet2->type));
    if (ofBase)
	msg = xmlStrcat(msg, BAD_CAST "' of the base type");
    else
	msg = xmlStrcat(msg, BAD_CAST "'");

    xmlSchemaPCustomErr(pctxt,
	XML_SCHEMAP_INVALID_FACET_VALUE,
	NULL, (xmlSchemaTypePtr) facet1, facet1->node,
	(const char *) msg, NULL);

    if (msg != NULL)
	xmlFree(msg);
}

static int
xmlSchemaDeriveAndValidateFacets(xmlSchemaParserCtxtPtr pctxt,
				 xmlSchemaTypePtr type)
{
    xmlSchemaTypePtr base = type->baseType;
    xmlSchemaFacetLinkPtr link, cur, last = NULL;
    xmlSchemaFacetPtr facet, bfacet,
	flength = NULL, ftotdig = NULL, ffracdig = NULL,
	fmaxlen = NULL, fminlen = NULL, /* facets of the current type */
	fmininc = NULL, fmaxinc = NULL,
	fminexc = NULL, fmaxexc = NULL,
	bflength = NULL, bftotdig = NULL, bffracdig = NULL,
	bfmaxlen = NULL, bfminlen = NULL, /* facets of the base type */
	bfmininc = NULL, bfmaxinc = NULL,
	bfminexc = NULL, bfmaxexc = NULL;
    int res, err = 0, fixedErr;
    /*
    * 3 The {facets} of R are the union of S and the {facets}
    * of B, eliminating duplicates. To eliminate duplicates,
    * when a facet of the same kind occurs in both S and the
    * {facets} of B, the one in the {facets} of B is not
    * included, with the exception of enumeration and pattern
    * facets, for which multiple occurrences with distinct values
    * are allowed.
    */

    if ((type->facetSet == NULL) && (base->facetSet == NULL))
	return (0);

    last = type->facetSet;
    if (last != NULL)
	while (last->next != NULL)
	    last = last->next;

    for (cur = type->facetSet; cur != NULL; cur = cur->next) {
	facet = cur->facet;
	switch (facet->type) {
	    case XML_SCHEMA_FACET_LENGTH:
		flength = facet; break;
	    case XML_SCHEMA_FACET_MINLENGTH:
		fminlen = facet; break;
	    case XML_SCHEMA_FACET_MININCLUSIVE:
		fmininc = facet; break;
	    case XML_SCHEMA_FACET_MINEXCLUSIVE:
		fminexc = facet; break;
	    case XML_SCHEMA_FACET_MAXLENGTH:
		fmaxlen = facet; break;
	    case XML_SCHEMA_FACET_MAXINCLUSIVE:
		fmaxinc = facet; break;
	    case XML_SCHEMA_FACET_MAXEXCLUSIVE:
		fmaxexc = facet; break;
	    case XML_SCHEMA_FACET_TOTALDIGITS:
		ftotdig = facet; break;
	    case XML_SCHEMA_FACET_FRACTIONDIGITS:
		ffracdig = facet; break;
	    default:
		break;
	}
    }
    for (cur = base->facetSet; cur != NULL; cur = cur->next) {
	facet = cur->facet;
	switch (facet->type) {
	    case XML_SCHEMA_FACET_LENGTH:
		bflength = facet; break;
	    case XML_SCHEMA_FACET_MINLENGTH:
		bfminlen = facet; break;
	    case XML_SCHEMA_FACET_MININCLUSIVE:
		bfmininc = facet; break;
	    case XML_SCHEMA_FACET_MINEXCLUSIVE:
		bfminexc = facet; break;
	    case XML_SCHEMA_FACET_MAXLENGTH:
		bfmaxlen = facet; break;
	    case XML_SCHEMA_FACET_MAXINCLUSIVE:
		bfmaxinc = facet; break;
	    case XML_SCHEMA_FACET_MAXEXCLUSIVE:
		bfmaxexc = facet; break;
	    case XML_SCHEMA_FACET_TOTALDIGITS:
		bftotdig = facet; break;
	    case XML_SCHEMA_FACET_FRACTIONDIGITS:
		bffracdig = facet; break;
	    default:
		break;
	}
    }
    err = 0;
    /*
    * length and minLength or maxLength (2.2) + (3.2)
    */
    if (flength && (fminlen || fmaxlen)) {
	FACET_RESTR_ERR(flength, "It is an error for both 'length' and "
	    "either of 'minLength' or 'maxLength' to be specified on "
	    "the same type definition")
    }
    /*
    * Mutual exclusions in the same derivation step.
    */
    if ((fmaxinc) && (fmaxexc)) {
	/*
	* SCC "maxInclusive and maxExclusive"
	*/
	FACET_RESTR_MUTUAL_ERR(fmaxinc, fmaxexc)
    }
    if ((fmininc) && (fminexc)) {
	/*
	* SCC "minInclusive and minExclusive"
	*/
	FACET_RESTR_MUTUAL_ERR(fmininc, fminexc)
    }

    if (flength && bflength) {
	/*
	* SCC "length valid restriction"
	* The values have to be equal.
	*/
	res = xmlSchemaCompareValues(flength->val, bflength->val);
	if (res == -2)
	    goto internal_error;
	if (res != 0)
	    xmlSchemaDeriveFacetErr(pctxt, flength, bflength, 0, 0, 1);
	if ((res != 0) && (bflength->fixed)) {
	    FACET_RESTR_FIXED_ERR(flength)
	}

    }
    if (fminlen && bfminlen) {
	/*
	* SCC "minLength valid restriction"
	* minLength >= BASE minLength
	*/
	res = xmlSchemaCompareValues(fminlen->val, bfminlen->val);
	if (res == -2)
	    goto internal_error;
	if (res == -1)
	    xmlSchemaDeriveFacetErr(pctxt, fminlen, bfminlen, 1, 1, 1);
	if ((res != 0) && (bfminlen->fixed)) {
	    FACET_RESTR_FIXED_ERR(fminlen)
	}
    }
    if (fmaxlen && bfmaxlen) {
	/*
	* SCC "maxLength valid restriction"
	* maxLength <= BASE minLength
	*/
	res = xmlSchemaCompareValues(fmaxlen->val, bfmaxlen->val);
	if (res == -2)
	    goto internal_error;
	if (res == 1)
	    xmlSchemaDeriveFacetErr(pctxt, fmaxlen, bfmaxlen, -1, 1, 1);
	if ((res != 0) && (bfmaxlen->fixed)) {
	    FACET_RESTR_FIXED_ERR(fmaxlen)
	}
    }
    /*
    * SCC "length and minLength or maxLength"
    */
    if (! flength)
	flength = bflength;
    if (flength) {
	if (! fminlen)
	    flength = bflength;
	if (fminlen) {
	    /* (1.1) length >= minLength */
	    res = xmlSchemaCompareValues(flength->val, fminlen->val);
	    if (res == -2)
		goto internal_error;
	    if (res == -1)
		xmlSchemaDeriveFacetErr(pctxt, flength, fminlen, 1, 1, 0);
	}
	if (! fmaxlen)
	    fmaxlen = bfmaxlen;
	if (fmaxlen) {
	    /* (2.1) length <= maxLength */
	    res = xmlSchemaCompareValues(flength->val, fmaxlen->val);
	    if (res == -2)
		goto internal_error;
	    if (res == 1)
		xmlSchemaDeriveFacetErr(pctxt, flength, fmaxlen, -1, 1, 0);
	}
    }
    if (fmaxinc) {
	/*
	* "maxInclusive"
	*/
	if (fmininc) {
	    /* SCC "maxInclusive >= minInclusive" */
	    res = xmlSchemaCompareValues(fmaxinc->val, fmininc->val);
	    if (res == -2)
		goto internal_error;
	    if (res == -1) {
		xmlSchemaDeriveFacetErr(pctxt, fmaxinc, fmininc, 1, 1, 0);
	    }
	}
	/*
	* SCC "maxInclusive valid restriction"
	*/
	if (bfmaxinc) {
	    /* maxInclusive <= BASE maxInclusive */
	    res = xmlSchemaCompareValues(fmaxinc->val, bfmaxinc->val);
	    if (res == -2)
		goto internal_error;
	    if (res == 1)
		xmlSchemaDeriveFacetErr(pctxt, fmaxinc, bfmaxinc, -1, 1, 1);
	    if ((res != 0) && (bfmaxinc->fixed)) {
		FACET_RESTR_FIXED_ERR(fmaxinc)
	    }
	}
	if (bfmaxexc) {
	    /* maxInclusive < BASE maxExclusive */
	    res = xmlSchemaCompareValues(fmaxinc->val, bfmaxexc->val);
	    if (res == -2)
		goto internal_error;
	    if (res != -1) {
		xmlSchemaDeriveFacetErr(pctxt, fmaxinc, bfmaxexc, -1, 0, 1);
	    }
	}
	if (bfmininc) {
	    /* maxInclusive >= BASE minInclusive */
	    res = xmlSchemaCompareValues(fmaxinc->val, bfmininc->val);
	    if (res == -2)
		goto internal_error;
	    if (res == -1) {
		xmlSchemaDeriveFacetErr(pctxt, fmaxinc, bfmininc, 1, 1, 1);
	    }
	}
	if (bfminexc) {
	    /* maxInclusive > BASE minExclusive */
	    res = xmlSchemaCompareValues(fmaxinc->val, bfminexc->val);
	    if (res == -2)
		goto internal_error;
	    if (res != 1) {
		xmlSchemaDeriveFacetErr(pctxt, fmaxinc, bfminexc, 1, 0, 1);
	    }
	}
    }
    if (fmaxexc) {
	/*
	* "maxExclusive >= minExclusive"
	*/
	if (fminexc) {
	    res = xmlSchemaCompareValues(fmaxexc->val, fminexc->val);
	    if (res == -2)
		goto internal_error;
	    if (res == -1) {
		xmlSchemaDeriveFacetErr(pctxt, fmaxexc, fminexc, 1, 1, 0);
	    }
	}
	/*
	* "maxExclusive valid restriction"
	*/
	if (bfmaxexc) {
	    /* maxExclusive <= BASE maxExclusive */
	    res = xmlSchemaCompareValues(fmaxexc->val, bfmaxexc->val);
	    if (res == -2)
		goto internal_error;
	    if (res == 1) {
		xmlSchemaDeriveFacetErr(pctxt, fmaxexc, bfmaxexc, -1, 1, 1);
	    }
	    if ((res != 0) && (bfmaxexc->fixed)) {
		FACET_RESTR_FIXED_ERR(fmaxexc)
	    }
	}
	if (bfmaxinc) {
	    /* maxExclusive <= BASE maxInclusive */
	    res = xmlSchemaCompareValues(fmaxexc->val, bfmaxinc->val);
	    if (res == -2)
		goto internal_error;
	    if (res == 1) {
		xmlSchemaDeriveFacetErr(pctxt, fmaxexc, bfmaxinc, -1, 1, 1);
	    }
	}
	if (bfmininc) {
	    /* maxExclusive > BASE minInclusive */
	    res = xmlSchemaCompareValues(fmaxexc->val, bfmininc->val);
	    if (res == -2)
		goto internal_error;
	    if (res != 1) {
		xmlSchemaDeriveFacetErr(pctxt, fmaxexc, bfmininc, 1, 0, 1);
	    }
	}
	if (bfminexc) {
	    /* maxExclusive > BASE minExclusive */
	    res = xmlSchemaCompareValues(fmaxexc->val, bfminexc->val);
	    if (res == -2)
		goto internal_error;
	    if (res != 1) {
		xmlSchemaDeriveFacetErr(pctxt, fmaxexc, bfminexc, 1, 0, 1);
	    }
	}
    }
    if (fminexc) {
	/*
	* "minExclusive < maxInclusive"
	*/
	if (fmaxinc) {
	    res = xmlSchemaCompareValues(fminexc->val, fmaxinc->val);
	    if (res == -2)
		goto internal_error;
	    if (res != -1) {
		xmlSchemaDeriveFacetErr(pctxt, fminexc, fmaxinc, -1, 0, 0);
	    }
	}
	/*
	* "minExclusive valid restriction"
	*/
	if (bfminexc) {
	    /* minExclusive >= BASE minExclusive */
	    res = xmlSchemaCompareValues(fminexc->val, bfminexc->val);
	    if (res == -2)
		goto internal_error;
	    if (res == -1) {
		xmlSchemaDeriveFacetErr(pctxt, fminexc, bfminexc, 1, 1, 1);
	    }
	    if ((res != 0) && (bfminexc->fixed)) {
		FACET_RESTR_FIXED_ERR(fminexc)
	    }
	}
	if (bfmaxinc) {
	    /* minExclusive <= BASE maxInclusive */
	    res = xmlSchemaCompareValues(fminexc->val, bfmaxinc->val);
	    if (res == -2)
		goto internal_error;
	    if (res == 1) {
		xmlSchemaDeriveFacetErr(pctxt, fminexc, bfmaxinc, -1, 1, 1);
	    }
	}
	if (bfmininc) {
	    /* minExclusive >= BASE minInclusive */
	    res = xmlSchemaCompareValues(fminexc->val, bfmininc->val);
	    if (res == -2)
		goto internal_error;
	    if (res == -1) {
		xmlSchemaDeriveFacetErr(pctxt, fminexc, bfmininc, 1, 1, 1);
	    }
	}
	if (bfmaxexc) {
	    /* minExclusive < BASE maxExclusive */
	    res = xmlSchemaCompareValues(fminexc->val, bfmaxexc->val);
	    if (res == -2)
		goto internal_error;
	    if (res != -1) {
		xmlSchemaDeriveFacetErr(pctxt, fminexc, bfmaxexc, -1, 0, 1);
	    }
	}
    }
    if (fmininc) {
	/*
	* "minInclusive < maxExclusive"
	*/
	if (fmaxexc) {
	    res = xmlSchemaCompareValues(fmininc->val, fmaxexc->val);
	    if (res == -2)
		goto internal_error;
	    if (res != -1) {
		xmlSchemaDeriveFacetErr(pctxt, fmininc, fmaxexc, -1, 0, 0);
	    }
	}
	/*
	* "minExclusive valid restriction"
	*/
	if (bfmininc) {
	    /* minInclusive >= BASE minInclusive */
	    res = xmlSchemaCompareValues(fmininc->val, bfmininc->val);
	    if (res == -2)
		goto internal_error;
	    if (res == -1) {
		xmlSchemaDeriveFacetErr(pctxt, fmininc, bfmininc, 1, 1, 1);
	    }
	    if ((res != 0) && (bfmininc->fixed)) {
		FACET_RESTR_FIXED_ERR(fmininc)
	    }
	}
	if (bfmaxinc) {
	    /* minInclusive <= BASE maxInclusive */
	    res = xmlSchemaCompareValues(fmininc->val, bfmaxinc->val);
	    if (res == -2)
		goto internal_error;
	    if (res == -1) {
		xmlSchemaDeriveFacetErr(pctxt, fmininc, bfmaxinc, -1, 1, 1);
	    }
	}
	if (bfminexc) {
	    /* minInclusive > BASE minExclusive */
	    res = xmlSchemaCompareValues(fmininc->val, bfminexc->val);
	    if (res == -2)
		goto internal_error;
	    if (res != 1)
		xmlSchemaDeriveFacetErr(pctxt, fmininc, bfminexc, 1, 0, 1);
	}
	if (bfmaxexc) {
	    /* minInclusive < BASE maxExclusive */
	    res = xmlSchemaCompareValues(fmininc->val, bfmaxexc->val);
	    if (res == -2)
		goto internal_error;
	    if (res != -1)
		xmlSchemaDeriveFacetErr(pctxt, fmininc, bfmaxexc, -1, 0, 1);
	}
    }
    if (ftotdig && bftotdig) {
	/*
	* SCC " totalDigits valid restriction"
	* totalDigits <= BASE totalDigits
	*/
	res = xmlSchemaCompareValues(ftotdig->val, bftotdig->val);
	if (res == -2)
	    goto internal_error;
	if (res == 1)
	    xmlSchemaDeriveFacetErr(pctxt, ftotdig, bftotdig,
	    -1, 1, 1);
	if ((res != 0) && (bftotdig->fixed)) {
	    FACET_RESTR_FIXED_ERR(ftotdig)
	}
    }
    if (ffracdig && bffracdig) {
	/*
	* SCC  "fractionDigits valid restriction"
	* fractionDigits <= BASE fractionDigits
	*/
	res = xmlSchemaCompareValues(ffracdig->val, bffracdig->val);
	if (res == -2)
	    goto internal_error;
	if (res == 1)
	    xmlSchemaDeriveFacetErr(pctxt, ffracdig, bffracdig,
	    -1, 1, 1);
	if ((res != 0) && (bffracdig->fixed)) {
	    FACET_RESTR_FIXED_ERR(ffracdig)
	}
    }
    /*
    * SCC "fractionDigits less than or equal to totalDigits"
    */
    if (! ftotdig)
	ftotdig = bftotdig;
    if (! ffracdig)
	ffracdig = bffracdig;
    if (ftotdig && ffracdig) {
	res = xmlSchemaCompareValues(ffracdig->val, ftotdig->val);
	if (res == -2)
	    goto internal_error;
	if (res == 1)
	    xmlSchemaDeriveFacetErr(pctxt, ffracdig, ftotdig,
		-1, 1, 0);
    }
    /*
    * *Enumerations* won' be added here, since only the first set
    * of enumerations in the ancestor-or-self axis is used
    * for validation, plus we need to use the base type of those
    * enumerations for whitespace.
    *
    * *Patterns*: won't be add here, since they are ORed at
    * type level and ANDed at ancestor level. This will
    * happed during validation by walking the base axis
    * of the type.
    */
    for (cur = base->facetSet; cur != NULL; cur = cur->next) {
	bfacet = cur->facet;
	/*
	* Special handling of enumerations and patterns.
	* TODO: hmm, they should not appear in the set, so remove this.
	*/
	if ((bfacet->type == XML_SCHEMA_FACET_PATTERN) ||
	    (bfacet->type == XML_SCHEMA_FACET_ENUMERATION))
	    continue;
	/*
	* Search for a duplicate facet in the current type.
	*/
	link = type->facetSet;
	err = 0;
	fixedErr = 0;
	while (link != NULL) {
	    facet = link->facet;
	    if (facet->type == bfacet->type) {
		switch (facet->type) {
		    case XML_SCHEMA_FACET_WHITESPACE:
			/*
			* The whitespace must be stronger.
			*/
			if (facet->whitespace < bfacet->whitespace) {
			    FACET_RESTR_ERR(flength,
				"The 'whitespace' value has to be equal to "
				"or stronger than the 'whitespace' value of "
				"the base type")
			}
			if ((bfacet->fixed) &&
			    (facet->whitespace != bfacet->whitespace)) {
			    FACET_RESTR_FIXED_ERR(facet)
			}
			break;
		    default:
			break;
		}
		/* Duplicate found. */
		break;
	    }
	    link = link->next;
	}
	/*
	* If no duplicate was found: add the base types's facet
	* to the set.
	*/
	if (link == NULL) {
	    link = (xmlSchemaFacetLinkPtr)
		xmlMalloc(sizeof(xmlSchemaFacetLink));
	    if (link == NULL) {
		xmlSchemaPErrMemory(pctxt,
		    "deriving facets, creating a facet link", NULL);
		return (-1);
	    }
	    link->facet = cur->facet;
	    link->next = NULL;
	    if (last == NULL)
		type->facetSet = link;
	    else
		last->next = link;
	    last = link;
	}

    }

    return (0);
internal_error:
    xmlSchemaPCustomErr(pctxt,
	XML_SCHEMAP_INVALID_FACET_VALUE,
	NULL, type, NULL,
	"Internal error: xmlSchemaDeriveAndValidateFacets", NULL);
    return (-1);
}

static int
xmlSchemaFinishMemberTypeDefinitionsProperty(xmlSchemaParserCtxtPtr pctxt,
					     xmlSchemaTypePtr type)
{
    xmlSchemaTypeLinkPtr link, lastLink, prevLink, subLink, newLink;
    /*
    * The actual value is then formed by replacing any union type
    * definition in the ·explicit members· with the members of their
    * {member type definitions}, in order.
    */
    link = type->memberTypes;
    while (link != NULL) {

	if (IS_NOT_TYPEFIXED(link->type))
	    xmlSchemaTypeFixup(link->type, pctxt, NULL);

	if (VARIETY_UNION(link->type)) {
	    subLink = xmlSchemaGetUnionSimpleTypeMemberTypes(link->type);
	    if (subLink != NULL) {
		link->type = subLink->type;
		if (subLink->next != NULL) {
		    lastLink = link->next;
		    subLink = subLink->next;
		    prevLink = link;
		    while (subLink != NULL) {
			newLink = (xmlSchemaTypeLinkPtr)
			    xmlMalloc(sizeof(xmlSchemaTypeLink));
			if (newLink == NULL) {
			    xmlSchemaPErrMemory(pctxt, "allocating a type link",
				NULL);
			    return (-1);
			}
			newLink->type = subLink->type;
			prevLink->next = newLink;
			prevLink = newLink;
			newLink->next = lastLink;

			subLink = subLink->next;
		    }
		}
	    }
	}
	link = link->next;
    }
    return (0);
}

static void
xmlSchemaTypeFixupOptimFacets(xmlSchemaTypePtr type)
{       
    int has = 0, needVal = 0, normVal = 0;

    has	= (type->baseType->flags & XML_SCHEMAS_TYPE_HAS_FACETS) ? 1 : 0;
    if (has) {
	needVal = (type->baseType->flags &
	    XML_SCHEMAS_TYPE_FACETSNEEDVALUE) ? 1 : 0;
	normVal = (type->baseType->flags &
	    XML_SCHEMAS_TYPE_NORMVALUENEEDED) ? 1 : 0;
    }
    if (type->facets != NULL) {
	xmlSchemaFacetPtr fac;
	
	for (fac = type->facets; fac != NULL; fac = fac->next) {
	    switch (fac->type) {
		case XML_SCHEMA_FACET_WHITESPACE:
		    break;
		case XML_SCHEMA_FACET_PATTERN:
		    normVal = 1;
		    has = 1;
		    break;
		case XML_SCHEMA_FACET_ENUMERATION:
		    needVal = 1;
		    normVal = 1;
		    has = 1;
		    break;
		default:
		    has = 1;
		    break;
	    }
	}	
    }
    if (normVal)
	type->flags |= XML_SCHEMAS_TYPE_NORMVALUENEEDED;
    if (needVal)
	type->flags |= XML_SCHEMAS_TYPE_FACETSNEEDVALUE;
    if (has)
	type->flags |= XML_SCHEMAS_TYPE_HAS_FACETS;

    if (has && (! needVal) && VARIETY_ATOMIC(type)) {
	xmlSchemaTypePtr prim = xmlSchemaGetPrimitiveType(type);
	/*
	* OPTIMIZE VAL TODO: Some facets need a computed value.
	*/
	if ((prim->builtInType != XML_SCHEMAS_ANYSIMPLETYPE) &&
	    (prim->builtInType != XML_SCHEMAS_STRING)) {
	    type->flags |= XML_SCHEMAS_TYPE_FACETSNEEDVALUE;
	} 	
    }       
}

static int
xmlSchemaTypeFixupWhitespace(xmlSchemaTypePtr type)
{
    
    
    /*
    * Evaluate the whitespace-facet value.
    */    
    if (VARIETY_LIST(type)) {
	type->flags |= XML_SCHEMAS_TYPE_WHITESPACE_COLLAPSE;
	return (0);
    } else if (VARIETY_UNION(type))
	return (0);
    
    if (type->facetSet != NULL) {
	xmlSchemaFacetLinkPtr lin;

	for (lin = type->facetSet; lin != NULL; lin = lin->next) {
	    if (lin->facet->type == XML_SCHEMA_FACET_WHITESPACE) {
		switch (lin->facet->whitespace) {
		case XML_SCHEMAS_FACET_PRESERVE:
		    type->flags |= XML_SCHEMAS_TYPE_WHITESPACE_PRESERVE;
		    break;
		case XML_SCHEMAS_FACET_REPLACE:
		    type->flags |= XML_SCHEMAS_TYPE_WHITESPACE_REPLACE;
		    break;
		case XML_SCHEMAS_FACET_COLLAPSE:
		    type->flags |= XML_SCHEMAS_TYPE_WHITESPACE_COLLAPSE;
		    break;
		default:
		    return (-1);
		}
		return (0);
	    }
	}
    }
    /*
    * For all ·atomic· datatypes other than string (and types ·derived· 
    * by ·restriction· from it) the value of whiteSpace is fixed to 
    * collapse
    */
    {
	xmlSchemaTypePtr anc;

	for (anc = type->baseType; anc != NULL && 
		anc->builtInType != XML_SCHEMAS_ANYTYPE;
		anc = anc->baseType) {

	    if (anc->type == XML_SCHEMA_TYPE_BASIC) {
		if (anc->builtInType == XML_SCHEMAS_NORMSTRING) {	    
		    type->flags |= XML_SCHEMAS_TYPE_WHITESPACE_REPLACE;

		} else if ((anc->builtInType == XML_SCHEMAS_STRING) ||
		    (anc->builtInType == XML_SCHEMAS_ANYSIMPLETYPE)) {		    
		    type->flags |= XML_SCHEMAS_TYPE_WHITESPACE_PRESERVE;

		} else
		    type->flags |= XML_SCHEMAS_TYPE_WHITESPACE_COLLAPSE;
		break;
	    }
	}
	return (0);
    }
    return (0);
}

/**
 * xmlSchemaTypeFixup:
 * @typeDecl:  the schema type definition
 * @ctxt:  the schema parser context
 *
 * Fixes the content model of the type.
 */
static void
xmlSchemaTypeFixup(xmlSchemaTypePtr type,
                   xmlSchemaParserCtxtPtr pctxt, const xmlChar * name)
{
    if (type == NULL)
        return;
    if ((type->type != XML_SCHEMA_TYPE_COMPLEX) &&
	(type->type != XML_SCHEMA_TYPE_SIMPLE))
	return;
    if (! IS_NOT_TYPEFIXED(type))
	return;
    type->flags |= XML_SCHEMAS_TYPE_INTERNAL_RESOLVED;
    if (name == NULL)
        name = type->name;

    if (type->baseType == NULL) {
	xmlSchemaPCustomErr(pctxt,
	    XML_SCHEMAP_INTERNAL,
	    NULL, type, NULL,
	    "Internal error: xmlSchemaTypeFixup, "
	    "baseType is missing on '%s'", type->name);
	return;
    }

    if (type->type == XML_SCHEMA_TYPE_COMPLEX) {
	xmlSchemaTypePtr baseType = type->baseType;

	/*
	* Type-fix the base type.
	*/
	if (IS_NOT_TYPEFIXED(baseType))
	    xmlSchemaTypeFixup(baseType, pctxt, NULL);
	if (baseType->flags & XML_SCHEMAS_TYPE_INTERNAL_INVALID) {
	    /*
	    * Skip fixup if the base type is invalid.
	    * TODO: Generate a warning!
	    */
	    return;
	}	
	/*
	* This basically checks if the base type can be derived.
	*/
	if (xmlSchemaCheckSRCCT(pctxt, type) != 0) {
	    type->flags |= XML_SCHEMAS_TYPE_INTERNAL_INVALID;
	    return;
	}
	/*
	* Fixup the content type.
	*/
	if (type->contentType == XML_SCHEMA_CONTENT_SIMPLE) {
	    /*
	    * Corresponds to <complexType><simpleContent>...
	    */
	    if ((IS_COMPLEX_TYPE(baseType)) &&
		(baseType->contentTypeDef != NULL) &&
		(type->flags & XML_SCHEMAS_TYPE_DERIVATION_METHOD_RESTRICTION)) {
		xmlSchemaTypePtr contentBase, content;
		char buf[30];
		const xmlChar *tmpname;
		/*
		* SPEC (1) If <restriction> + base type is <complexType>,
		* "whose own {content type} is a simple type..."
		*/
		if (type->contentTypeDef != NULL) {
		    /*
		    * SPEC (1.1) "the simple type definition corresponding to the
		    * <simpleType> among the [children] of <restriction> if there
		    * is one;"
		    * Note that this "<simpleType> among the [children]" was put
		    * into ->contentTypeDef during parsing.
		    */
		    contentBase = type->contentTypeDef;
		    type->contentTypeDef = NULL;
		} else {
		    /*
		    * (1.2) "...otherwise (<restriction> has no <simpleType>
		    * among its [children]), the simple type definition which
		    * is the {content type} of the ... base type."
		    */
		    contentBase = baseType->contentTypeDef;
		}
		/*
		* SPEC
		* "... a simple type definition which restricts the simple
		* type definition identified in clause 1.1 or clause 1.2
		* with a set of facet components"
		*
		* Create the anonymous simple type, which will be the content
		* type of the complex type.
		*/		
		snprintf(buf, 29, "#scST%d", ++(pctxt->counter));
		tmpname = xmlDictLookup(pctxt->dict, BAD_CAST buf, -1);
		content = xmlSchemaAddType(pctxt,
		    pctxt->schema, tmpname, tmpname, type->node);
		if (content == NULL)
		    return;
		/*
		* We will use the same node as for the <complexType>
		* to have it somehow anchored in the schema doc.
		*/
		content->node = type->node;
		content->type = XML_SCHEMA_TYPE_SIMPLE;
		content->contentType = XML_SCHEMA_CONTENT_SIMPLE;
		content->baseType = contentBase;
		/*
		* Move the facets, previously anchored on the complexType.
		*/
		content->facets = type->facets;
		type->facets = NULL;
		content->facetSet = type->facetSet;
		type->facetSet = NULL;

		type->contentTypeDef = content;
		if (IS_NOT_TYPEFIXED(contentBase))
		    xmlSchemaTypeFixup(contentBase, pctxt, NULL);
		xmlSchemaTypeFixup(content, pctxt, NULL);

	    } else if ((IS_COMPLEX_TYPE(baseType)) &&
		(baseType->contentType == XML_SCHEMA_CONTENT_MIXED) &&
		(type->flags & XML_SCHEMAS_TYPE_DERIVATION_METHOD_RESTRICTION)) {
		/*
		* SPEC (2) If <restriction> + base is a mixed <complexType> with
		* an emptiable particle, then a simple type definition which
		* restricts the <restriction>'s <simpleType> child.
		*/
		if ((type->contentTypeDef == NULL) ||
		    (type->contentTypeDef->baseType == NULL)) {
		    /*
		    * TODO: Check if this ever happens.
		    */
		    xmlSchemaPCustomErr(pctxt,
			XML_SCHEMAP_INTERNAL,
			NULL, type, NULL,
			"Internal error: xmlSchemaTypeFixup, "
			"complex type '%s': the <simpleContent><restriction> "
			"is missing a <simpleType> child, but was not catched "
			"by xmlSchemaCheckSRCCT()", type->name);
		}
	    } else if ((IS_COMPLEX_TYPE(baseType)) &&
		(type->flags & XML_SCHEMAS_TYPE_DERIVATION_METHOD_EXTENSION)) {
		/*
		* SPEC (3) If <extension> + base is <complexType> with
		* <simpleType> content, "...then the {content type} of that
		* complex type definition"
		*/
		if (baseType->contentTypeDef == NULL) {
		    /*
		    * TODO: Check if this ever happens. xmlSchemaCheckSRCCT
		    * should have catched this already.
		    */
		    xmlSchemaPCustomErr(pctxt,
			XML_SCHEMAP_INTERNAL,
			NULL, type, NULL,
			"Internal error: xmlSchemaTypeFixup, "
			"complex type '%s': the <extension>ed base type is "
			"a complex type with no simple content type",
			type->name);
		}
		type->contentTypeDef = baseType->contentTypeDef;
	    } else if ((IS_SIMPLE_TYPE(baseType)) &&
		(type->flags & XML_SCHEMAS_TYPE_DERIVATION_METHOD_EXTENSION)) {
		/*
		* SPEC (4) <extension> + base is <simpleType>
		* "... then that simple type definition"
		*/
		type->contentTypeDef = baseType;
	    } else {
		/*
		* TODO: Check if this ever happens.
		*/
		xmlSchemaPCustomErr(pctxt,
		    XML_SCHEMAP_INTERNAL,
		    NULL, type, NULL,
		    "Internal error: xmlSchemaTypeFixup, "
		    "complex type '%s' with <simpleContent>: unhandled "
		    "derivation case", type->name);
	    }
	} else {
	    int dummySequence = 0;
	    xmlSchemaParticlePtr particle =
		(xmlSchemaParticlePtr) type->subtypes;
	    /*
	    * Corresponds to <complexType><complexContent>...
	    *
	    * NOTE that the effective mixed was already set during parsing of
	    * <complexType> and <complexContent>; its flag value is
	    * XML_SCHEMAS_TYPE_MIXED.
	    *
	    * Compute the "effective content":
	    * (2.1.1) + (2.1.2) + (2.1.3)
	    */
	    if ((particle == NULL) ||
		((particle->type == XML_SCHEMA_TYPE_PARTICLE) &&
		 ((particle->children->type == XML_SCHEMA_TYPE_ALL) ||
		  (particle->children->type == XML_SCHEMA_TYPE_SEQUENCE) ||
		  ((particle->children->type == XML_SCHEMA_TYPE_CHOICE) &&
		   (particle->minOccurs == 0))) &&
		   ( ((xmlSchemaTreeItemPtr) particle->children)->children == NULL))) {
		if (type->flags & XML_SCHEMAS_TYPE_MIXED) {
		    /*
		    * SPEC (2.1.4) "If the ·effective mixed· is true, then
		    * a particle whose properties are as follows:..."
		    *
		    * Empty sequence model group with
		    * minOccurs/maxOccurs = 1 (i.e. a "particle emptiable").
		    * NOTE that we sill assign it the <complexType> node to
		    * somehow anchor it in the doc.
		    */
		    if ((particle == NULL) ||
			(particle->children->type != XML_SCHEMA_TYPE_SEQUENCE)) {
			/*
			* Create the particle.
			*/
			particle = xmlSchemaAddParticle(pctxt, pctxt->schema,
			    type->node, 1, 1);
			if (particle == NULL)
			    return;
			/*
			* Create the model group.
			*/
			particle->children = (xmlSchemaTreeItemPtr)
			    xmlSchemaAddModelGroup(pctxt, pctxt->schema,
				XML_SCHEMA_TYPE_SEQUENCE, NULL, type->node);
			if (particle->children == NULL)
			    return;

			type->subtypes = (xmlSchemaTypePtr) particle;
		    }
		    dummySequence = 1;
		    type->contentType = XML_SCHEMA_CONTENT_ELEMENTS;
		} else {
		    /*
		    * SPEC (2.1.5) "otherwise empty"
		    */
		    type->contentType = XML_SCHEMA_CONTENT_EMPTY;
		}
	    } else {
		/*
	 	* SPEC (2.2) "otherwise the particle corresponding to the
		* <all>, <choice>, <group> or <sequence> among the
		* [children]."
		*/
		type->contentType = XML_SCHEMA_CONTENT_ELEMENTS;
	    }
	    /*
	    * Compute the "content type".
	    */
	    if (type->flags & XML_SCHEMAS_TYPE_DERIVATION_METHOD_RESTRICTION) {
		/*
		* SPEC (3.1) "If <restriction>..."
		* (3.1.1) + (3.1.2) */
		if (type->contentType != XML_SCHEMA_CONTENT_EMPTY) {
		    if (type->flags & XML_SCHEMAS_TYPE_MIXED)
			type->contentType = XML_SCHEMA_CONTENT_MIXED;
		}
	    } else {
		/*
		* SPEC (3.2) "If <extension>..."
		*/
		if (type->contentType == XML_SCHEMA_CONTENT_EMPTY) {
		    /*
		    * SPEC (3.2.1)
		    */
		    type->contentType = baseType->contentType;
		    type->subtypes = baseType->subtypes;
		    /*
		    * NOTE that the effective mixed is ignored here.
		    */
		} else if (baseType->contentType == XML_SCHEMA_CONTENT_EMPTY) {
		    /*
		    * SPEC (3.2.2)
		    */
		    if (type->flags & XML_SCHEMAS_TYPE_MIXED)
			type->contentType = XML_SCHEMA_CONTENT_MIXED;
		} else {
		    /*
		    * SPEC (3.2.3)
		    */
		    if (type->flags & XML_SCHEMAS_TYPE_MIXED)
			type->contentType = XML_SCHEMA_CONTENT_MIXED;
		    /*
		    * "A model group whose {compositor} is sequence and whose
		    * {particles} are..."
		    */
		    if (! dummySequence) {
			xmlSchemaTreeItemPtr effectiveContent =
			    (xmlSchemaTreeItemPtr) type->subtypes;
			/*
			* Create the particle.
			*/
			particle = xmlSchemaAddParticle(pctxt, pctxt->schema,
			    type->node, 1, 1);
			if (particle == NULL)
			    return;
			/*
			* Create the "sequence" model group.
			*/
			particle->children = (xmlSchemaTreeItemPtr)
			    xmlSchemaAddModelGroup(pctxt, pctxt->schema,
				XML_SCHEMA_TYPE_SEQUENCE, NULL, type->node);
			if (particle->children == NULL)
			    return;
			type->subtypes = (xmlSchemaTypePtr) particle;
			/*
			* SPEC "the particle of the {content type} of
			* the ... base ..."
			* Create a duplicate of the base type's particle
			* and assign its "term" to it.
			*/
			particle->children->children =
			    (xmlSchemaTreeItemPtr) xmlSchemaAddParticle(pctxt,
				pctxt->schema, type->node,
				((xmlSchemaParticlePtr) type->subtypes)->minOccurs,
				((xmlSchemaParticlePtr) type->subtypes)->maxOccurs);
			if (particle->children->children == NULL)
			    return;
			particle = (xmlSchemaParticlePtr)
			    particle->children->children;
			particle->children =
				((xmlSchemaParticlePtr) baseType->subtypes)->children;
			/*
			* SPEC "followed by the ·effective content·."
			*/
			particle->next = effectiveContent;
		    } else {
			/*
			* This is the case when there is already an empty
			* <sequence> with minOccurs==maxOccurs==1.
			* Just add the base types's content type.
			* NOTE that, although we miss to add an intermediate
			* <sequence>, this should produce no difference to
			* neither the regex compilation of the content model,
			* nor to the complex type contraints.
			*/
			particle->children->children =
			    (xmlSchemaTreeItemPtr) baseType->subtypes;
		    }
		}
	    }
	}
	/*
	* Apply the complex type component constraints; this will not
	* check attributes, since this is done in
	* xmlSchemaBuildAttributeValidation().
	*/
	if (xmlSchemaCheckCTComponent(pctxt, type) != 0)
	    return;
	/*
	* Inherit & check constraints for attributes.
	*/
	xmlSchemaBuildAttributeValidation(pctxt, type);
    } else if (type->type == XML_SCHEMA_TYPE_SIMPLE) {
	/*
	* Simple Type Definition Schema Component
	*/
	type->contentType = XML_SCHEMA_CONTENT_SIMPLE;
	if (VARIETY_LIST(type)) {
	    /*
	    * Corresponds to <simpleType><list>...
	    */
	    if (type->subtypes == NULL) {
		/*
		* This one is really needed, so get out.
		*/
		PERROR_INT("xmlSchemaTypeFixup",
		"list type has no item-type assigned");
		return;
	    }
	    if (IS_NOT_TYPEFIXED(type->subtypes))
		xmlSchemaTypeFixup(type->subtypes, pctxt, NULL);
	} else if (VARIETY_UNION(type)) {
	    /*
	    * Corresponds to <simpleType><union>...
	    */
	    if (type->memberTypes == NULL) {
		/*
		* This one is really needed, so get out.
		*/
		return;
	    }
	    if (xmlSchemaFinishMemberTypeDefinitionsProperty(pctxt, type) == -1)
		return;
	} else {
	    xmlSchemaTypePtr baseType = type->baseType;
	    /*
	    * Corresponds to <simpleType><restriction>...
	    */
	    if (IS_NOT_TYPEFIXED(baseType))
		xmlSchemaTypeFixup(baseType, pctxt, NULL);
	    /*
	    * Variety
	    * If the <restriction> alternative is chosen, then the
	    * {variety} of the {base type definition}.
	    */
	    if (VARIETY_ATOMIC(baseType))
		type->flags |= XML_SCHEMAS_TYPE_VARIETY_ATOMIC;
	    else if (VARIETY_LIST(baseType)) {
		type->flags |= XML_SCHEMAS_TYPE_VARIETY_LIST;
		/*
		* Inherit the itemType.
		*/
		type->subtypes = baseType->subtypes;
	    } else if (VARIETY_UNION(baseType)) {
		type->flags |= XML_SCHEMAS_TYPE_VARIETY_UNION;
		/*
		* NOTE that we won't assign the memberTypes of the base,
		* since this will make trouble when freeing them; we will
		* use a lookup function to access them instead.
		*/
	    }
	}
	/*
	* Check constraints.
	*
	* TODO: Split this somehow, we need to know first if we can derive
	* from the base type at all!
	*/
	if (type->baseType != NULL) {
	    /*
	    * Schema Component Constraint: Simple Type Restriction
	    * (Facets)
	    * NOTE: Satisfaction of 1 and 2 arise from the fixup
	    * applied beforehand.
	    */
	    xmlSchemaCheckSRCSimpleType(pctxt, type);
	    xmlSchemaCheckFacetValues(type, pctxt);
	    if ((type->facetSet != NULL) ||
		(type->baseType->facetSet != NULL))
		xmlSchemaDeriveAndValidateFacets(pctxt, type);
	    /*
	    * Whitespace value.
	    */
	    xmlSchemaTypeFixupWhitespace(type);
	    xmlSchemaTypeFixupOptimFacets(type);
	}
    }

#ifdef DEBUG_TYPE
    if (type->node != NULL) {
        xmlGenericError(xmlGenericErrorContext,
                        "Type of %s : %s:%d :", name,
                        type->node->doc->URL,
                        xmlGetLineNo(type->node));
    } else {
        xmlGenericError(xmlGenericErrorContext, "Type of %s :", name);
    }
    if ((IS_SIMPLE_TYPE(type)) || (IS_COMPLEX_TYPE(type))) {
	switch (type->contentType) {
	    case XML_SCHEMA_CONTENT_SIMPLE:
		xmlGenericError(xmlGenericErrorContext, "simple\n");
		break;
	    case XML_SCHEMA_CONTENT_ELEMENTS:
		xmlGenericError(xmlGenericErrorContext, "elements\n");
		break;
	    case XML_SCHEMA_CONTENT_UNKNOWN:
		xmlGenericError(xmlGenericErrorContext, "unknown !!!\n");
		break;
	    case XML_SCHEMA_CONTENT_EMPTY:
		xmlGenericError(xmlGenericErrorContext, "empty\n");
		break;
	    case XML_SCHEMA_CONTENT_MIXED:
		if (xmlSchemaIsParticleEmptiable((xmlSchemaParticlePtr)
		    type->subtypes))
		    xmlGenericError(xmlGenericErrorContext,
			"mixed as emptiable particle\n");
		else
		    xmlGenericError(xmlGenericErrorContext, "mixed\n");
		break;
		/* Removed, since not used. */
		/*
		case XML_SCHEMA_CONTENT_MIXED_OR_ELEMENTS:
		xmlGenericError(xmlGenericErrorContext, "mixed or elems\n");
		break;
		*/
	    case XML_SCHEMA_CONTENT_BASIC:
		xmlGenericError(xmlGenericErrorContext, "basic\n");
		break;
	    default:
		xmlGenericError(xmlGenericErrorContext,
		    "not registered !!!\n");
		break;
	}
    }
#endif
}

/**
 * xmlSchemaCheckFacet:
 * @facet:  the facet
 * @typeDecl:  the schema type definition
 * @pctxt:  the schema parser context or NULL
 * @name: the optional name of the type
 *
 * Checks and computes the values of facets.
 *
 * Returns 0 if valid, a positive error code if not valid and
 *         -1 in case of an internal or API error.
 */
int
xmlSchemaCheckFacet(xmlSchemaFacetPtr facet,
                    xmlSchemaTypePtr typeDecl,
                    xmlSchemaParserCtxtPtr pctxt,
		    const xmlChar * name ATTRIBUTE_UNUSED)
{
    int ret = 0, ctxtGiven;

    if ((facet == NULL) || (typeDecl == NULL))
        return(-1);
    /*
    * TODO: will the parser context be given if used from
    * the relaxNG module?
    */
    if (pctxt == NULL)
	ctxtGiven = 0;
    else
	ctxtGiven = 1;

    switch (facet->type) {
        case XML_SCHEMA_FACET_MININCLUSIVE:
        case XML_SCHEMA_FACET_MINEXCLUSIVE:
        case XML_SCHEMA_FACET_MAXINCLUSIVE:
        case XML_SCHEMA_FACET_MAXEXCLUSIVE:
	case XML_SCHEMA_FACET_ENUMERATION: {
                /*
                 * Okay we need to validate the value
                 * at that point.
                 */
		xmlSchemaTypePtr base;

		/* 4.3.5.5 Constraints on enumeration Schema Components
		* Schema Component Constraint: enumeration valid restriction
		* It is an ·error· if any member of {value} is not in the
		* ·value space· of {base type definition}.
		*
		* minInclusive, maxInclusive, minExclusive, maxExclusive:
		* The value ·must· be in the
		* ·value space· of the ·base type·.
		*/
		/*
		* This function is intended to deliver a compiled value
		* on the facet. In this implementation of XML Schemata the
		* type holding a facet, won't be a built-in type.
		* Thus to ensure that other API
		* calls (relaxng) do work, if the given type is a built-in
		* type, we will assume that the given built-in type *is
		* already* the base type.
		*/
		if (typeDecl->type != XML_SCHEMA_TYPE_BASIC) {
		    base = typeDecl->baseType;
		    if (base == NULL) {
			PERROR_INT("xmlSchemaCheckFacet",
			    "a type user derived type has no base type");
			return (-1);
		    }
		} else
		    base = typeDecl;
	                 
		if (! ctxtGiven) {
		    /*
		    * A context is needed if called from RelaxNG.
		    */		    
		    pctxt = xmlSchemaNewParserCtxt("*");
		    if (pctxt == NULL)
			return (-1);
		}
		/*
		* NOTE: This call does not check the content nodes,
		* since they are not available:
		* facet->node is just the node holding the facet
		* definition, *not* the attribute holding the *value*
		* of the facet.
		*/		
		ret = xmlSchemaVCheckCVCSimpleType(
		    (xmlSchemaAbstractCtxtPtr) pctxt, facet->node, base,
		    facet->value, &(facet->val), 1, 1, 0);
                if (ret != 0) {
		    if (ret < 0) {
			/* No error message for RelaxNG. */
			if (ctxtGiven) {			    
			    xmlSchemaCustomErr((xmlSchemaAbstractCtxtPtr) pctxt,
				XML_SCHEMAP_INTERNAL, facet->node, NULL,
				"Internal error: xmlSchemaCheckFacet, " 
				"failed to validate the value '%s' of the "
				"facet '%s' against the base type",
				facet->value, xmlSchemaFacetTypeToString(facet->type));
			}
			goto internal_error;
		    }
		    ret = XML_SCHEMAP_INVALID_FACET_VALUE;
		    /* No error message for RelaxNG. */
		    if (ctxtGiven) {
			xmlChar *str = NULL;

			xmlSchemaCustomErr((xmlSchemaAbstractCtxtPtr) pctxt,
			    ret, facet->node, (xmlSchemaTypePtr) facet,
			    "The value '%s' of the facet does not validate "
			    "against the base type '%s'",
			    facet->value,
			    xmlSchemaFormatQName(&str,
				base->targetNamespace, base->name));
			FREE_AND_NULL(str);
		    }
		    goto exit;
                } else if (facet->val == NULL) {
		    if (ctxtGiven) {
			PERROR_INT("xmlSchemaCheckFacet",
			    "value was not computed");
		    }
		    TODO
		}
                break;
            }
        case XML_SCHEMA_FACET_PATTERN:
            facet->regexp = xmlRegexpCompile(facet->value);
            if (facet->regexp == NULL) {
		ret = XML_SCHEMAP_REGEXP_INVALID;
		/* No error message for RelaxNG. */
		if (ctxtGiven) {
		    xmlSchemaCustomErr((xmlSchemaAbstractCtxtPtr) pctxt,
			ret, facet->node, typeDecl,
			"The value '%s' of the facet 'pattern' is not a "
			"valid regular expression",
			facet->value, NULL);
		}
            }
            break;
        case XML_SCHEMA_FACET_TOTALDIGITS:
        case XML_SCHEMA_FACET_FRACTIONDIGITS:
        case XML_SCHEMA_FACET_LENGTH:
        case XML_SCHEMA_FACET_MAXLENGTH:
        case XML_SCHEMA_FACET_MINLENGTH:{
		ret = xmlSchemaValidatePredefinedType(
		    xmlSchemaGetBuiltInType(XML_SCHEMAS_NNINTEGER),
		    facet->value, &(facet->val));
                if (ret != 0) {
		    if (ret < 0) {
			/* No error message for RelaxNG. */
			if (ctxtGiven) {
			    PERROR_INT("xmlSchemaCheckFacet",
				"validating facet value");
			}
			goto internal_error;
		    }
		    ret = XML_SCHEMAP_INVALID_FACET_VALUE;
		    /* No error message for RelaxNG. */
		    if (ctxtGiven) {
			/* error code */
                        xmlSchemaCustomErr((xmlSchemaAbstractCtxtPtr) pctxt,
			    ret, facet->node, typeDecl,
			    "The value '%s' of the facet '%s' is not a valid "
			    "'nonNegativeInteger'",
			    facet->value,
			    xmlSchemaFacetTypeToString(facet->type));
                    }
                }
                break;
            }
        case XML_SCHEMA_FACET_WHITESPACE:{
                if (xmlStrEqual(facet->value, BAD_CAST "preserve")) {
                    facet->whitespace = XML_SCHEMAS_FACET_PRESERVE;
                } else if (xmlStrEqual(facet->value, BAD_CAST "replace")) {
                    facet->whitespace = XML_SCHEMAS_FACET_REPLACE;
                } else if (xmlStrEqual(facet->value, BAD_CAST "collapse")) {
                    facet->whitespace = XML_SCHEMAS_FACET_COLLAPSE;
                } else {
		    ret = XML_SCHEMAP_INVALID_FACET_VALUE;
                    /* No error message for RelaxNG. */
		    if (ctxtGiven) {
			/* error was previously: XML_SCHEMAP_INVALID_WHITE_SPACE */
			xmlSchemaCustomErr((xmlSchemaAbstractCtxtPtr) pctxt,
			    ret, facet->node, typeDecl,
			    "The value '%s' of the facet 'whitespace' is not "
			    "valid", facet->value, NULL);
                    }
                }
            }
        default:
            break;
    }
exit:
    if ((! ctxtGiven) && (pctxt != NULL))
	xmlSchemaFreeParserCtxt(pctxt);
    return (ret);
internal_error:
    if ((! ctxtGiven) && (pctxt != NULL))
	xmlSchemaFreeParserCtxt(pctxt);
    return (-1);
}

/**
 * xmlSchemaCheckFacetValues:
 * @typeDecl:  the schema type definition
 * @ctxt:  the schema parser context
 *
 * Checks the default values types, especially for facets
 */
static void
xmlSchemaCheckFacetValues(xmlSchemaTypePtr typeDecl,
			  xmlSchemaParserCtxtPtr ctxt)
{
    const xmlChar *name = typeDecl->name;
    /*
    * NOTE: It is intended to use the facets list, instead
    * of facetSet.
    */
    if (typeDecl->facets != NULL) {
	xmlSchemaFacetPtr facet = typeDecl->facets;

	/*
	* Temporarily assign the "schema" to the validation context
	* of the parser context. This is needed for NOTATION validation.
	*/
	if (ctxt->vctxt == NULL) {
	    if (xmlSchemaCreateVCtxtOnPCtxt(ctxt) == -1)
		return;
	}
	ctxt->vctxt->schema = ctxt->schema;

	while (facet != NULL) {
	    xmlSchemaCheckFacet(facet, typeDecl, ctxt, name);
	    facet = facet->next;
	}

	ctxt->vctxt->schema = NULL;
    }
}

/**
 * xmlSchemaGetCircModelGrDefRef:
 * @ctxtMGroup: the searched model group
 * @selfMGroup: the second searched model group
 * @particle: the first particle
 *
 * This one is intended to be used by
 * xmlSchemaCheckGroupDefCircular only.
 *
 * Returns the particle with the circular model group definition reference,
 * otherwise NULL.
 */
static xmlSchemaTreeItemPtr
xmlSchemaGetCircModelGrDefRef(xmlSchemaModelGroupDefPtr groupDef,
			      xmlSchemaTreeItemPtr particle)
{
    xmlSchemaTreeItemPtr circ = NULL;
    xmlSchemaTreeItemPtr term;
    xmlSchemaModelGroupDefPtr gdef;

    for (; particle != NULL; particle = particle->next) {
	term = particle->children;
	if (term == NULL)
	    continue;
	switch (term->type) {
	    case XML_SCHEMA_TYPE_GROUP:
		gdef = (xmlSchemaModelGroupDefPtr) term;
		if (gdef == groupDef)
		    return (particle);
		/*
		* Mark this model group definition to avoid infinite
		* recursion on circular references not yet examined.
		*/
		if (gdef->flags & XML_SCHEMA_MODEL_GROUP_DEF_MARKED)
		    continue;
		if (gdef->children != NULL) {
		    gdef->flags |= XML_SCHEMA_MODEL_GROUP_DEF_MARKED;
		    circ = xmlSchemaGetCircModelGrDefRef(groupDef,
			gdef->children->children);
		    gdef->flags ^= XML_SCHEMA_MODEL_GROUP_DEF_MARKED;
		    if (circ != NULL)
			return (circ);
		}
		break;
	    case XML_SCHEMA_TYPE_SEQUENCE:
	    case XML_SCHEMA_TYPE_CHOICE:
	    case XML_SCHEMA_TYPE_ALL:
		circ = xmlSchemaGetCircModelGrDefRef(groupDef, term->children);
		if (circ != NULL)
		    return (circ);
		break;
	    default:
		break;
	}
    }
    return (NULL);
}

/**
 * xmlSchemaCheckGroupDefCircular:
 * @item:  the model group definition
 * @ctxt:  the parser context
 * @name:  the name
 *
 * Checks for circular references to model group definitions.
 */
static void
xmlSchemaCheckGroupDefCircular(xmlSchemaModelGroupDefPtr item,
			       xmlSchemaParserCtxtPtr ctxt,
			       const xmlChar * name ATTRIBUTE_UNUSED)
{
    /*
    * Schema Component Constraint: Model Group Correct
    * 2 Circular groups are disallowed. That is, within the {particles}
    * of a group there must not be at any depth a particle whose {term}
    * is the group itself.
    */
    if ((item == NULL) ||
	(item->type != XML_SCHEMA_TYPE_GROUP) ||
	(item->children == NULL))
	return;
    {
	xmlSchemaTreeItemPtr circ;

	circ = xmlSchemaGetCircModelGrDefRef(item, item->children->children);
	if (circ != NULL) {
	    xmlChar *str = NULL;
	    /*
	    * TODO: The error report is not adequate: this constraint
	    * is defined for model groups but not definitions, but since
	    * there cannot be any circular model groups without a model group
	    * definition (if not using a construction API), we check those
	    * defintions only.
	    */
	    xmlSchemaPCustomErr(ctxt,
		XML_SCHEMAP_MG_PROPS_CORRECT_2,
		NULL, NULL, GET_NODE(circ),
		"Circular reference to the model group definition '%s' "
		"defined", xmlSchemaFormatQName(&str,
		    item->targetNamespace, item->name));
	    FREE_AND_NULL(str)
	    /*
	    * NOTE: We will cut the reference to avoid further
	    * confusion of the processor. This is a fatal error.
	    */
	    circ->children = NULL;
	}
    }
}

/**
 * xmlSchemaGroupDefTermFixup:
 * @item:  the particle with a model group definition as term
 * @ctxt:  the parser context
 * @name:  the name
 *
 * Checks cos-all-limited.
 *
 * Assigns the model group of model group definitions to the "term"
 * of the referencing particle.
 * In xmlSchemaMiscRefFixup the model group definitions was assigned
 * to the "term", since needed for the circularity check. 
 */
static void
xmlSchemaGroupDefTermFixup(xmlSchemaParticlePtr item,
			   xmlSchemaParserCtxtPtr ctxt ATTRIBUTE_UNUSED,
			   const xmlChar * name ATTRIBUTE_UNUSED)
{
    if ((item == NULL) ||
	(item->type != XML_SCHEMA_TYPE_PARTICLE) ||
	(item->children == NULL) ||
	(item->children->type != XML_SCHEMA_TYPE_GROUP) ||
	(item->children->children == NULL))
	return;
    item->children = item->children->children;
    /*
    * TODO: Not nice, but we will anchor cos-all-limited here.
    */
    if ((item->children->type == XML_SCHEMA_TYPE_ALL) &&
	(item->maxOccurs != 1)) {
	/*
	* SPEC (1.2) "the {term} property of a particle with
	* {max occurs}=1which is part of a pair which constitutes the
	* {content type} of a complex type definition."
	*/
	xmlSchemaPCustomErr(ctxt,
	    XML_SCHEMAP_SRC_ATTRIBUTE_GROUP_3,
	    NULL, (xmlSchemaTypePtr) item, item->node,
	    "The particle's 'maxOccurs' must be 1, since an xs:all model "
	    "group is its term", NULL);
    }
}

/**
 * xmlSchemaGetCircAttrGrRef:
 * @ctxtGr: the searched attribute group
 * @attr: the current attribute list to be processed
 *
 * This one is intended to be used by
 * xmlSchemaCheckSRCAttributeGroupCircular only.
 *
 * Returns the circular attribute grou reference, otherwise NULL.
 */
static xmlSchemaAttributeGroupPtr
xmlSchemaGetCircAttrGrRef(xmlSchemaAttributeGroupPtr ctxtGr,
			  xmlSchemaAttributePtr attr)
{
    xmlSchemaAttributeGroupPtr circ = NULL, gr;
    int marked;
    /*
    * We will search for an attribute group reference which
    * references the context attribute group.
    */
    while (attr != NULL) {
	marked = 0;
	if (attr->type == XML_SCHEMA_TYPE_ATTRIBUTEGROUP) {
	    gr = (xmlSchemaAttributeGroupPtr) attr;
	    if (gr->refItem != NULL)  {
		if (gr->refItem == ctxtGr)
		    return (gr);
		else if (gr->refItem->flags &
		    XML_SCHEMAS_ATTRGROUP_MARKED) {
		    attr = attr->next;
		    continue;
		} else {
		    /*
		    * Mark as visited to avoid infinite recursion on
		    * circular references not yet examined.
		    */
		    gr->refItem->flags |= XML_SCHEMAS_ATTRGROUP_MARKED;
		    marked = 1;
		}
	    }
	    if (gr->attributes != NULL)
		circ = xmlSchemaGetCircAttrGrRef(ctxtGr, gr->attributes);
	    /*
	    * Unmark the visited group's attributes.
	    */
	    if (marked)
		gr->refItem->flags ^= XML_SCHEMAS_ATTRGROUP_MARKED;
	    if (circ != NULL)
		return (circ);
	}
	attr = attr->next;
    }
    return (NULL);
}

/**
 * xmlSchemaCheckSRCAttributeGroupCircular:
 * attrGr:  the attribute group definition
 * @ctxt:  the parser context
 * @name:  the name
 *
 * Checks for circular references of attribute groups.
 */
static void
xmlSchemaCheckAttributeGroupCircular(xmlSchemaAttributeGroupPtr attrGr,
					xmlSchemaParserCtxtPtr ctxt,
					const xmlChar * name ATTRIBUTE_UNUSED)
{
    /*
    * Schema Representation Constraint:
    * Attribute Group Definition Representation OK
    * 3 Circular group reference is disallowed outside <redefine>.
    * That is, unless this element information item's parent is
    * <redefine>, then among the [children], if any, there must
    * not be an <attributeGroup> with ref [attribute] which resolves
    * to the component corresponding to this <attributeGroup>. Indirect
    * circularity is also ruled out. That is, when QName resolution
    * (Schema Document) (§3.15.3) is applied to a ·QName· arising from
    * any <attributeGroup>s with a ref [attribute] among the [children],
    * it must not be the case that a ·QName· is encountered at any depth
    * which resolves to the component corresponding to this <attributeGroup>.
    */
    /*
    * Only global components can be referenced.
    */
    if (((attrGr->flags & XML_SCHEMAS_ATTRGROUP_GLOBAL) == 0) ||
	(attrGr->attributes == NULL))
	return;
    else {
	xmlSchemaAttributeGroupPtr circ;

	circ = xmlSchemaGetCircAttrGrRef(attrGr, attrGr->attributes);
	if (circ != NULL) {
	    /*
	    * TODO: Report the referenced attr group as QName.
	    */
	    xmlSchemaPCustomErr(ctxt,
		XML_SCHEMAP_SRC_ATTRIBUTE_GROUP_3,
		NULL, NULL, circ->node,
		"Circular reference to the attribute group '%s' "
		"defined", attrGr->name);
	    /*
	    * NOTE: We will cut the reference to avoid further
	    * confusion of the processor.
	    * BADSPEC: The spec should define how to process in this case.
	    */
	    circ->attributes = NULL;
	    circ->refItem = NULL;
	}
    }
}

/**
 * xmlSchemaAttrGrpFixup:
 * @attrgrpDecl:  the schema attribute definition
 * @ctxt:  the schema parser context
 * @name:  the attribute name
 *
 * Fixes finish doing the computations on the attributes definitions
 */
static void
xmlSchemaAttrGrpFixup(xmlSchemaAttributeGroupPtr attrgrp,
                      xmlSchemaParserCtxtPtr ctxt, const xmlChar * name)
{
    if (name == NULL)
        name = attrgrp->name;
    if (attrgrp->attributes != NULL)
        return;
    if (attrgrp->ref != NULL) {
        xmlSchemaAttributeGroupPtr ref;

        ref = xmlSchemaGetAttributeGroup(ctxt->schema, attrgrp->ref,
	    attrgrp->refNs);
        if (ref == NULL) {
	    xmlSchemaPResCompAttrErr(ctxt,
		XML_SCHEMAP_SRC_RESOLVE,
		(xmlSchemaTypePtr) attrgrp, attrgrp->node,
		"ref", attrgrp->ref, attrgrp->refNs,
		XML_SCHEMA_TYPE_ATTRIBUTEGROUP, NULL);
            return;
        }
	attrgrp->refItem = ref;
	/*
	* Check for self reference!
	*/
        xmlSchemaAttrGrpFixup(ref, ctxt, NULL);
        attrgrp->attributes = ref->attributes;
	attrgrp->attributeWildcard = ref->attributeWildcard;
    }
}

/**
 * xmlSchemaAttrCheckValConstr:
 * @item:  an schema attribute declaration/use
 * @ctxt:  a schema parser context
 * @name:  the name of the attribute
 *
 *
 * Schema Component Constraint: Attribute Declaration Properties Correct
 *   (a-props-correct)
 * Validates the value constraints of an attribute declaration/use.
 *
 * Fixes finish doing the computations on the attributes definitions
 */
static void
xmlSchemaCheckAttrValConstr(xmlSchemaAttributePtr item,
			    xmlSchemaParserCtxtPtr pctxt,
			    const xmlChar * name ATTRIBUTE_UNUSED)
{

    /*
    * 2 if there is a {value constraint}, the canonical lexical
    * representation of its value must be ·valid· with respect
    * to the {type definition} as defined in String Valid (§3.14.4).
    */
    if (item->defValue != NULL) {
	int ret;

	if (item->subtypes == NULL) {
	    PERROR_INT("xmlSchemaCheckAttrValConstr",
		"type is missing");
	    return;
	}
	ret = xmlSchemaVCheckCVCSimpleType((xmlSchemaAbstractCtxtPtr) pctxt,
	    item->node, item->subtypes, item->defValue, &(item->defVal),
	    1, 1, 0);
	if (ret != 0) {
	    if (ret < 0) {
		PERROR_INT("xmlSchemaAttrCheckValConstr",
		    "calling xmlSchemaVCheckCVCSimpleType()");
		return;
	    }
	    ret = XML_SCHEMAP_A_PROPS_CORRECT_2;
	    xmlSchemaCustomErr((xmlSchemaAbstractCtxtPtr) pctxt,
		ret, item->node, (xmlSchemaTypePtr) item,
		"The value of the value constraint is not valid", NULL, NULL);
	    return;
	}
    }
}

static xmlSchemaElementPtr
xmlSchemaCheckSubstGroupCircular(xmlSchemaElementPtr elemDecl,
				 xmlSchemaElementPtr ancestor)
{
    xmlSchemaElementPtr ret;

    if (SUBST_GROUP_AFF(ancestor) == NULL)
	return (NULL);
    if (SUBST_GROUP_AFF(ancestor) == elemDecl)
	return (ancestor);

    if (SUBST_GROUP_AFF(ancestor)->flags & XML_SCHEMAS_ELEM_CIRCULAR)
	return (NULL);
    SUBST_GROUP_AFF(ancestor)->flags |= XML_SCHEMAS_ELEM_CIRCULAR;
    ret = xmlSchemaCheckSubstGroupCircular(elemDecl,
	SUBST_GROUP_AFF(ancestor));
    SUBST_GROUP_AFF(ancestor)->flags ^= XML_SCHEMAS_ELEM_CIRCULAR;

    return (ret);
}

/**
 * xmlSchemaCheckElemPropsCorrect:
 * @ctxt:  a schema parser context
 * @decl: the element declaration
 * @name:  the name of the attribute
 *
 * Schema Component Constraint:
 * Element Declaration Properties Correct (e-props-correct)
 *
 * STATUS:
 *   missing: (6)
 */
static int
xmlSchemaCheckElemPropsCorrect(xmlSchemaParserCtxtPtr pctxt,
			       xmlSchemaElementPtr elemDecl)
{
    int ret = 0;
    xmlSchemaTypePtr typeDef = ELEM_TYPE(elemDecl);
    /*
    * SPEC (1) "The values of the properties of an element declaration
    * must be as described in the property tableau in The Element
    * Declaration Schema Component (§3.3.1), modulo the impact of Missing
    * Sub-components (§5.3)."
    */
    if (SUBST_GROUP_AFF(elemDecl) != NULL) {
	xmlSchemaElementPtr head = SUBST_GROUP_AFF(elemDecl), circ;

	xmlSchemaCheckElementDeclComponent(head, pctxt, NULL);
	/*
	* SPEC (3) "If there is a non-·absent· {substitution group
	* affiliation}, then {scope} must be global."
	*/
	if ((elemDecl->flags & XML_SCHEMAS_ELEM_GLOBAL) == 0) {
	    xmlSchemaPCustomErr(pctxt,
		XML_SCHEMAP_E_PROPS_CORRECT_3,
		NULL, (xmlSchemaTypePtr) elemDecl, elemDecl->node,
		"Only global element declarations can have a "
		"substitution group affiliation", NULL);
	    ret = XML_SCHEMAP_E_PROPS_CORRECT_3;
	}
	/*
	* TODO: SPEC (6) "Circular substitution groups are disallowed.
	* That is, it must not be possible to return to an element declaration
	* by repeatedly following the {substitution group affiliation}
	* property."
	*/
	if (head == elemDecl)
	    circ = head;
	else if (SUBST_GROUP_AFF(head) != NULL)
	    circ = xmlSchemaCheckSubstGroupCircular(head, head);
	else
	    circ = NULL;
	if (circ != NULL) {
	    xmlChar *strA = NULL, *strB = NULL;

	    xmlSchemaPCustomErrExt(pctxt,
		XML_SCHEMAP_E_PROPS_CORRECT_6,
		NULL, (xmlSchemaTypePtr) circ, circ->node,
		"The element declaration '%s' defines a circular "
		"substitution group to element declaration '%s'",
		xmlSchemaGetComponentQName(&strA, circ),
		xmlSchemaGetComponentQName(&strB, head),
		NULL);
	    FREE_AND_NULL(strA)
	    FREE_AND_NULL(strB)
	    ret = XML_SCHEMAP_E_PROPS_CORRECT_6;
	}
	/*
	* SPEC (4) "If there is a {substitution group affiliation},
	* the {type definition}
	* of the element declaration must be validly derived from the {type
	* definition} of the {substitution group affiliation}, given the value
	* of the {substitution group exclusions} of the {substitution group
	* affiliation}, as defined in Type Derivation OK (Complex) (§3.4.6)
	* (if the {type definition} is complex) or as defined in
	* Type Derivation OK (Simple) (§3.14.6) (if the {type definition} is
	* simple)."
	*
	* NOTE: {substitution group exclusions} means the values of the
	* attribute "final".
	*/

	if (typeDef != ELEM_TYPE(SUBST_GROUP_AFF(elemDecl))) {
	    int set = 0;

	    if (head->flags & XML_SCHEMAS_ELEM_FINAL_EXTENSION)
		set |= SUBSET_EXTENSION;
	    if (head->flags & XML_SCHEMAS_ELEM_FINAL_RESTRICTION)
		set |= SUBSET_RESTRICTION;

	    if (xmlSchemaCheckCOSDerivedOK(typeDef,
		ELEM_TYPE(head), set) != 0) {
		xmlChar *strA = NULL, *strB = NULL, *strC = NULL;

		ret = XML_SCHEMAP_E_PROPS_CORRECT_4;
		xmlSchemaPCustomErrExt(pctxt,
		    XML_SCHEMAP_E_PROPS_CORRECT_4,
		    NULL, (xmlSchemaTypePtr) elemDecl, elemDecl->node,
		    "The type definition '%s' was "
		    "either rejected by the substitution group "
		    "affiliation '%s', or not validly derived from its type "
		    "definition '%s'",
		    xmlSchemaGetComponentQName(&strA, typeDef),
		    xmlSchemaGetComponentQName(&strB, head),
		    xmlSchemaGetComponentQName(&strC, ELEM_TYPE(head)));
		FREE_AND_NULL(strA)
		FREE_AND_NULL(strB)
		FREE_AND_NULL(strC)
	    }
	}
    }
    /*
    * SPEC (5) "If the {type definition} or {type definition}'s
    * {content type}
    * is or is derived from ID then there must not be a {value constraint}.
    * Note: The use of ID as a type definition for elements goes beyond
    * XML 1.0, and should be avoided if backwards compatibility is desired"
    */
    if ((elemDecl->value != NULL) &&
	((IS_SIMPLE_TYPE(typeDef) &&
	  xmlSchemaIsDerivedFromBuiltInType(typeDef, XML_SCHEMAS_ID)) ||
	 (IS_COMPLEX_TYPE(typeDef) &&
	  HAS_SIMPLE_CONTENT(typeDef) &&
	  xmlSchemaIsDerivedFromBuiltInType(typeDef->contentTypeDef,
	    XML_SCHEMAS_ID)))) {

	ret = XML_SCHEMAP_E_PROPS_CORRECT_5;
	xmlSchemaPCustomErr(pctxt,
	    XML_SCHEMAP_E_PROPS_CORRECT_5,
	    NULL, (xmlSchemaTypePtr) elemDecl, elemDecl->node,
	    "The type definition (or type definition's content type) is or "
	    "is derived from ID; value constraints are not allowed in "
	    "conjunction with such a type definition", NULL);
    } else if (elemDecl->value != NULL) {
	int vcret;
	xmlNodePtr node = NULL;

	/*
	* SPEC (2) "If there is a {value constraint}, the canonical lexical
	* representation of its value must be ·valid· with respect to the
	* {type definition} as defined in Element Default Valid (Immediate)
	* (§3.3.6)."
	*/
	if (typeDef == NULL) {
	    xmlSchemaPErr(pctxt, elemDecl->node,
		XML_SCHEMAP_INTERNAL,
		"Internal error: xmlSchemaCheckElemPropsCorrect, "
		"type is missing... skipping validation of "
		"the value constraint", NULL, NULL);
	    return (-1);
	}
	if (elemDecl->node != NULL) {
	    if (elemDecl->flags & XML_SCHEMAS_ELEM_FIXED)
		node = (xmlNodePtr) xmlHasProp(elemDecl->node,
		    BAD_CAST "fixed");
	    else
		node = (xmlNodePtr) xmlHasProp(elemDecl->node,
		    BAD_CAST "default");
	}
	vcret = xmlSchemaParseCheckCOSValidDefault(pctxt, node,
	    typeDef, elemDecl->value, &(elemDecl->defVal));
	if (vcret != 0) {
	    if (vcret < 0) {
		PERROR_INT("xmlSchemaElemCheckValConstr",
		    "failed to validate the value constraint of an "
		    "element declaration");
		return (-1);
	    }
	    return (vcret);
	}
    }

    return (ret);
}

/**
 * xmlSchemaCheckElemSubstGroup:
 * @ctxt:  a schema parser context
 * @decl: the element declaration
 * @name:  the name of the attribute
 *
 * Schema Component Constraint:
 * Substitution Group (cos-equiv-class)
 *
 * In Libxml2 the subst. groups will be precomputed, in terms of that
 * a list will be built for each subst. group head, holding all direct
 * referents to this head.
 * NOTE that this function needs:
 *   1. circular subst. groups to be checked beforehand
 *   2. the declaration's type to be derived from the head's type
 *
 * STATUS:
 *
 */
static void
xmlSchemaCheckElemSubstGroup(xmlSchemaParserCtxtPtr ctxt,
			     xmlSchemaElementPtr elemDecl)
{
    if ((SUBST_GROUP_AFF(elemDecl) == NULL) ||
	/* SPEC (1) "Its {abstract} is false." */
	(elemDecl->flags & XML_SCHEMAS_ELEM_ABSTRACT))
	return;
    {
	xmlSchemaElementPtr head;
	xmlSchemaTypePtr headType, type;
	int set, methSet;
	/*
	* SPEC (2) "It is validly substitutable for HEAD subject to HEAD's
	* {disallowed substitutions} as the blocking constraint, as defined in
	* Substitution Group OK (Transitive) (§3.3.6)."
	*/
	for (head = SUBST_GROUP_AFF(elemDecl); head != NULL;
	    head = SUBST_GROUP_AFF(head)) {
	    set = 0;
	    methSet = 0;
	    /*
	    * The blocking constraints.
	    */
	    if (head->flags & XML_SCHEMAS_ELEM_BLOCK_SUBSTITUTION)
		continue;
	    headType = head->subtypes;
	    type = elemDecl->subtypes;
	    if (headType == type)
		goto add_member;
	    if (head->flags & XML_SCHEMAS_ELEM_BLOCK_RESTRICTION)
		set |= XML_SCHEMAS_TYPE_BLOCK_RESTRICTION;
	    if (head->flags & XML_SCHEMAS_ELEM_BLOCK_EXTENSION)
		set |= XML_SCHEMAS_TYPE_BLOCK_EXTENSION;
	    /*
	    * SPEC: Substitution Group OK (Transitive) (2.3)
	    * "The set of all {derivation method}s involved in the
	    * derivation of D's {type definition} from C's {type definition}
	    * does not intersect with the union of the blocking constraint,
	    * C's {prohibited substitutions} (if C is complex, otherwise the
	    * empty set) and the {prohibited substitutions} (respectively the
	    * empty set) of any intermediate {type definition}s in the
	    * derivation of D's {type definition} from C's {type definition}."
	    */
	    /*
	    * OPTIMIZE TODO: Optimize this a bit, since, if traversing the
	    * subst.head axis, the methSet does not need to be computed for
	    * the full depth over and over.
	    */
	    /*
	    * The set of all {derivation method}s involved in the derivation
	    */
	    while ((type != NULL) && (type != headType)) {
		if ((type->flags &
			XML_SCHEMAS_TYPE_DERIVATION_METHOD_EXTENSION) &&
		    ((methSet & XML_SCHEMAS_TYPE_BLOCK_RESTRICTION) == 0))
		    methSet |= XML_SCHEMAS_TYPE_BLOCK_EXTENSION;

		if ((type->flags &
			XML_SCHEMAS_TYPE_DERIVATION_METHOD_RESTRICTION) &&
		    ((methSet & XML_SCHEMAS_TYPE_BLOCK_RESTRICTION) == 0))
		    methSet |= XML_SCHEMAS_TYPE_BLOCK_RESTRICTION;

		type = type->baseType;
	    }
	    /*
	    * The {prohibited substitutions} of all intermediate types +
	    * the head's type.
	    */
	    type = elemDecl->subtypes->baseType;
	    while (type != NULL) {
		if (IS_COMPLEX_TYPE(type)) {
		    if ((type->flags &
			    XML_SCHEMAS_TYPE_BLOCK_EXTENSION) &&
			((set & XML_SCHEMAS_TYPE_BLOCK_EXTENSION) == 0))
		    set |= XML_SCHEMAS_TYPE_BLOCK_EXTENSION;
		    if ((type->flags &
			    XML_SCHEMAS_TYPE_BLOCK_RESTRICTION) &&
			((set & XML_SCHEMAS_TYPE_BLOCK_RESTRICTION) == 0))
		    set |= XML_SCHEMAS_TYPE_BLOCK_RESTRICTION;
		} else
		    break;
		if (type == headType)
		    break;
		type = type->baseType;
	    }
	    if ((set != 0) &&
		(((set & XML_SCHEMAS_TYPE_BLOCK_EXTENSION) &&
		(methSet & XML_SCHEMAS_TYPE_BLOCK_EXTENSION)) ||
		((set & XML_SCHEMAS_TYPE_BLOCK_RESTRICTION) &&
		(methSet & XML_SCHEMAS_TYPE_BLOCK_RESTRICTION)))) {
		continue;
	    }
add_member:
	    xmlSchemaAddElementSubstitutionMember(ctxt, head, elemDecl);
	    if ((head->flags & XML_SCHEMAS_ELEM_SUBST_GROUP_HEAD) == 0)
		head->flags |= XML_SCHEMAS_ELEM_SUBST_GROUP_HEAD;
	}
    }
}

/**
 * xmlSchemaCheckElementDeclComponent
 * @item:  an schema element declaration/particle
 * @ctxt:  a schema parser context
 * @name:  the name of the attribute
 *
 * Validates the value constraints of an element declaration.
 *
 * Fixes finish doing the computations on the element declarations.
 */
static void
xmlSchemaCheckElementDeclComponent(xmlSchemaElementPtr elemDecl,
				   xmlSchemaParserCtxtPtr ctxt,
				   const xmlChar * name ATTRIBUTE_UNUSED)
{
    if (elemDecl == NULL)
	return;
    if (elemDecl->flags & XML_SCHEMAS_ELEM_INTERNAL_CHECKED)
	return;
    elemDecl->flags |= XML_SCHEMAS_ELEM_INTERNAL_CHECKED;
    if (xmlSchemaCheckElemPropsCorrect(ctxt, elemDecl) == 0)
	xmlSchemaCheckElemSubstGroup(ctxt, elemDecl);
}

/**
 * xmlSchemaMiscRefFixup:
 * @item:  an schema component
 * @ctxt:  a schema parser context
 * @name:  the internal name of the component
 *
 * Resolves references of misc. schema components.
 */
static void
xmlSchemaMiscRefFixup(xmlSchemaTreeItemPtr item,
                   xmlSchemaParserCtxtPtr ctxt,
		   const xmlChar * name ATTRIBUTE_UNUSED)
{
    if (item->type == XML_SCHEMA_TYPE_PARTICLE) {
	if ((item->children != NULL) &&
	    (item->children->type == XML_SCHEMA_EXTRA_QNAMEREF)) {
	    xmlSchemaQNameRefPtr ref = (xmlSchemaQNameRefPtr) item->children;
	    xmlSchemaTreeItemPtr refItem;
	    /*
	    * Resolve the reference.
	    */
	    item->children = NULL;
	    refItem = xmlSchemaGetNamedComponent(ctxt->schema,
		ref->itemType, ref->name, ref->targetNamespace);
	    if (refItem == NULL) {
		xmlSchemaPResCompAttrErr(ctxt, XML_SCHEMAP_SRC_RESOLVE,
		    NULL, GET_NODE(item), "ref", ref->name,
		    ref->targetNamespace, ref->itemType, NULL);
	    } else {
		if (refItem->type == XML_SCHEMA_TYPE_GROUP) {
		    /*
		    * NOTE that we will assign the model group definition
		    * itself to the "term" of the particle. This will ease
		    * the check for circular model group definitions. After
		    * that the "term" will be assigned the model group of the
		    * model group definition.
		    */
		    item->children = refItem;
		} else
		    item->children = refItem;
	    }
	}
    }
}

static int
xmlSchemaAreValuesEqual(xmlSchemaValPtr x,
		       xmlSchemaValPtr y) 
{   
    xmlSchemaTypePtr tx, ty, ptx, pty;    
    int ret;

    while (x != NULL) {
	/* Same types. */
	tx = xmlSchemaGetBuiltInType(xmlSchemaGetValType(x));
	ty = xmlSchemaGetBuiltInType(xmlSchemaGetValType(y));
	ptx = xmlSchemaGetPrimitiveType(tx);
	pty = xmlSchemaGetPrimitiveType(ty);
	/*
	* (1) if a datatype T' is ·derived· by ·restriction· from an
	* atomic datatype T then the ·value space· of T' is a subset of
	* the ·value space· of T. */
	/*
	* (2) if datatypes T' and T'' are ·derived· by ·restriction·
	* from a common atomic ancestor T then the ·value space·s of T'
	* and T'' may overlap.
	*/
	if (ptx != pty)
	    return(0);
	/*
	* We assume computed values to be normalized, so do a fast
	* string comparison for string based types.
	*/
	if ((ptx->builtInType == XML_SCHEMAS_STRING) ||
	    IS_ANY_SIMPLE_TYPE(ptx)) {
	    if (! xmlStrEqual(
		xmlSchemaValueGetAsString(x),
		xmlSchemaValueGetAsString(y)))
		return (0);
	} else {
	    ret = xmlSchemaCompareValuesWhtsp(
		x, XML_SCHEMA_WHITESPACE_PRESERVE,
		y, XML_SCHEMA_WHITESPACE_PRESERVE);
	    if (ret == -2)
		return(-1);
	    if (ret != 0)
		return(0);
	}
	/*
	* Lists.
	*/
	x = xmlSchemaValueGetNext(x);
	if (x != NULL) {
	    y = xmlSchemaValueGetNext(y);
	    if (y == NULL)
		return (0);	    
	} else if (xmlSchemaValueGetNext(y) != NULL)
	    return (0);
	else
	    return (1);
    }
    return (0);
}

/**
 * xmlSchemaAttrFixup:
 * @item:  an schema attribute declaration/use.
 * @ctxt:  a schema parser context
 * @name:  the name of the attribute
 *
 * Fixes finish doing the computations on attribute declarations/uses.
 */
static void
xmlSchemaAttrFixup(xmlSchemaAttributePtr item,
                   xmlSchemaParserCtxtPtr ctxt,
		   const xmlChar * name ATTRIBUTE_UNUSED)
{
    /*
    * TODO: If including this is done twice (!) for every attribute.
    *       -> Hmm, check if this is still done.
    */
    /*
    * The simple type definition corresponding to the <simpleType> element
    * information item in the [children], if present, otherwise the simple
    * type definition ·resolved· to by the ·actual value· of the type
    * [attribute], if present, otherwise the ·simple ur-type definition·.
    */
    if (item->flags & XML_SCHEMAS_ATTR_INTERNAL_RESOLVED)
	return;
    item->flags |= XML_SCHEMAS_ATTR_INTERNAL_RESOLVED;
    if (item->subtypes != NULL)
        return;
    if (item->typeName != NULL) {
        xmlSchemaTypePtr type;

	type = xmlSchemaGetType(ctxt->schema, item->typeName,
	    item->typeNs);
	if ((type == NULL) || (! IS_SIMPLE_TYPE(type))) {
	    xmlSchemaPResCompAttrErr(ctxt,
		XML_SCHEMAP_SRC_RESOLVE,
		(xmlSchemaTypePtr) item, item->node,
		"type", item->typeName, item->typeNs,
		XML_SCHEMA_TYPE_SIMPLE, NULL);
	} else
	    item->subtypes = type;

    } else if (item->ref != NULL) {
        xmlSchemaAttributePtr decl;

	/*
	* We have an attribute use here; assign the referenced
	* attribute declaration.
	*/
	/*
	* TODO: Evaluate, what errors could occur if the declaration is not
	* found. It might be possible that the "typefixup" might crash if
	* no ref declaration was found.
	*/
	decl = xmlSchemaGetAttributeDecl(ctxt->schema, item->ref, item->refNs);
        if (decl == NULL) {
	    xmlSchemaPResCompAttrErr(ctxt,
	    	XML_SCHEMAP_SRC_RESOLVE,
		(xmlSchemaTypePtr) item, item->node,
		"ref", item->ref, item->refNs,
		XML_SCHEMA_TYPE_ATTRIBUTE, NULL);
            return;
        }
	item->refDecl = decl;
        xmlSchemaAttrFixup(decl, ctxt, NULL);
        item->subtypes = decl->subtypes;
	/*
	* Attribute Use Correct
	* au-props-correct.2: If the {attribute declaration} has a fixed
	* {value constraint}, then if the attribute use itself has a
	* {value constraint}, it must also be fixed and its value must match
	* that of the {attribute declaration}'s {value constraint}.
	*/
	if ((decl->flags & XML_SCHEMAS_ATTR_FIXED) &&
	    (item->defValue != NULL)) {
	    if ((item->flags & XML_SCHEMAS_ATTR_FIXED) == 0) {
		xmlSchemaPCustomErr(ctxt,
		    XML_SCHEMAP_AU_PROPS_CORRECT_2,
		    NULL, NULL, item->node,
		    "The attribute declaration has a 'fixed' value constraint "
		    ", thus it must be 'fixed' in attribute use as well",
		    NULL);
	    } else {
		if (! xmlSchemaAreValuesEqual(item->defVal, decl->defVal)) {
		    xmlSchemaPCustomErr(ctxt,
			XML_SCHEMAP_AU_PROPS_CORRECT_2,
			NULL, NULL, item->node,
			"The 'fixed' value constraint of the attribute use "
			"must match the attribute declaration's value "
			"constraint '%s'",
			decl->defValue);
		}
	    }
	    /*
	    * FUTURE: One should change the values of the attr. use
	    * if ever validation should be attempted even if the
	    * schema itself was not fully valid.
	    */
	}
    } else {
	item->subtypes = xmlSchemaGetBuiltInType(XML_SCHEMAS_ANYSIMPLETYPE);
    }
}

/**
 * xmlSchemaResolveIDCKeyRef:
 * @idc:  the identity-constraint definition
 * @ctxt:  the schema parser context
 * @name:  the attribute name
 *
 * Resolve keyRef references to key/unique IDCs.
 */
static void
xmlSchemaResolveIDCKeyRef(xmlSchemaIDCPtr idc,
			  xmlSchemaParserCtxtPtr pctxt,
			  const xmlChar * name ATTRIBUTE_UNUSED)
{
    if (idc->type != XML_SCHEMA_TYPE_IDC_KEYREF)
        return;
    if (idc->ref->name != NULL) {
	idc->ref->item = (xmlSchemaBasicItemPtr) xmlHashLookup2(
	    pctxt->schema->idcDef,
	    idc->ref->name,
	    idc->ref->targetNamespace);
        if (idc->ref->item == NULL) {
	    /*
	    * TODO: It is actually not an error to fail to resolve.
	    */
	    xmlSchemaPResCompAttrErr(pctxt,
		XML_SCHEMAP_SRC_RESOLVE,
		(xmlSchemaTypePtr) idc, idc->node,
		"refer", idc->ref->name,
		idc->ref->targetNamespace,
		XML_SCHEMA_TYPE_IDC_KEYREF, NULL);
            return;
	} else {
	    if (idc->nbFields !=
		((xmlSchemaIDCPtr) idc->ref->item)->nbFields) {
		xmlChar *str = NULL;
		xmlSchemaIDCPtr refer;
		
		refer = (xmlSchemaIDCPtr) idc->ref->item;
		/*
		* SPEC c-props-correct(2)
		* "If the {identity-constraint category} is keyref,
		* the cardinality of the {fields} must equal that of
		* the {fields} of the {referenced key}.
		*/
		xmlSchemaPCustomErr(pctxt,
		    XML_SCHEMAP_C_PROPS_CORRECT,
		    NULL, (xmlSchemaTypePtr) idc, idc->node,
		    "The cardinality of the keyref differs from the "
		    "cardinality of the referenced key '%s'",
		    xmlSchemaFormatQName(&str, refer->targetNamespace,
			refer->name) 
		);
		FREE_AND_NULL(str)
	    }
	}
    }
}

/**
 * xmlSchemaParse:
 * @ctxt:  a schema validation context
 *
 * parse a schema definition resource and build an internal
 * XML Shema struture which can be used to validate instances.
 * *WARNING* this interface is highly subject to change
 *
 * Returns the internal XML Schema structure built from the resource or
 *         NULL in case of error
 */
xmlSchemaPtr
xmlSchemaParse(xmlSchemaParserCtxtPtr ctxt)
{
    xmlSchemaPtr ret = NULL;
    xmlDocPtr doc;
    xmlNodePtr root;
    int preserve = 0;

    /*
    * This one is used if the schema to be parsed was specified via
    * the API; i.e. not automatically by the validated instance document.
    */

    xmlSchemaInitTypes();

    if (ctxt == NULL)
        return (NULL);

    ctxt->nberrors = 0;
    ctxt->counter = 0;
    ctxt->container = NULL;

    /*
     * First step is to parse the input document into an DOM/Infoset
     */
    if (ctxt->URL != NULL) {
        doc = xmlReadFile((const char *) ctxt->URL, NULL,
	                  SCHEMAS_PARSE_OPTIONS);
        if (doc == NULL) {
	    xmlSchemaPErr(ctxt, NULL,
			  XML_SCHEMAP_FAILED_LOAD,
                          "xmlSchemaParse: could not load '%s'.\n",
                          ctxt->URL, NULL);
            return (NULL);
        }
    } else if (ctxt->buffer != NULL) {
        doc = xmlReadMemory(ctxt->buffer, ctxt->size, NULL, NULL,
	                    SCHEMAS_PARSE_OPTIONS);
        if (doc == NULL) {
	    xmlSchemaPErr(ctxt, NULL,
			  XML_SCHEMAP_FAILED_PARSE,
                          "xmlSchemaParse: could not parse.\n",
                          NULL, NULL);
            return (NULL);
        }
        doc->URL = xmlStrdup(BAD_CAST "in_memory_buffer");
        ctxt->URL = xmlDictLookup(ctxt->dict, BAD_CAST "in_memory_buffer", -1);
    } else if (ctxt->doc != NULL) {
        doc = ctxt->doc;
	preserve = 1;
    } else {
	xmlSchemaPErr(ctxt, NULL,
		      XML_SCHEMAP_NOTHING_TO_PARSE,
		      "xmlSchemaParse: could not parse.\n",
		      NULL, NULL);
        return (NULL);
    }

    /*
     * Then extract the root and Schema parse it
     */
    root = xmlDocGetRootElement(doc);
    if (root == NULL) {
	xmlSchemaPErr(ctxt, (xmlNodePtr) doc,
		      XML_SCHEMAP_NOROOT,
		      "The schema has no document element.\n", NULL, NULL);
	if (!preserve) {
	    xmlFreeDoc(doc);
	}
        return (NULL);
    }

    /*
     * Remove all the blank text nodes
     */
    xmlSchemaCleanupDoc(ctxt, root);

    /*
     * Then do the parsing for good
     */
    ret = xmlSchemaParseSchema(ctxt, root);
    if (ret == NULL) {
        if (!preserve) {
	    xmlFreeDoc(doc);
	}
        return (NULL);
    }
    ret->doc = doc;
    ret->preserve = preserve;
    ctxt->schema = ret;
    ctxt->ctxtType = NULL;
    ctxt->parentItem = NULL;

    /*
    * Resolve base types of simple/complex types.
    */
    xmlHashScan(ret->typeDecl, (xmlHashScanner) xmlSchemaResolveTypeDefs, ctxt);

    if (ctxt->nberrors != 0)
	goto exit;

    if (ret->volatiles != NULL) {
	xmlSchemaItemListPtr list = (xmlSchemaItemListPtr) ret->volatiles;
	int i;
	xmlSchemaTreeItemPtr item;

	for (i = 0; i < list->nbItems; i++) {
	    item = (xmlSchemaTreeItemPtr) list->items[i];
	    if (item->type == XML_SCHEMA_TYPE_PARTICLE)
		xmlSchemaMiscRefFixup(item, ctxt, NULL);
	}
    }
    /*
     * Then fixup all attributes declarations
     */
    xmlHashScan(ret->attrDecl, (xmlHashScanner) xmlSchemaAttrFixup, ctxt);
    /*
     * Then fixup all attributes group declarations
     */
    xmlHashScan(ret->attrgrpDecl, (xmlHashScanner) xmlSchemaAttrGrpFixup,
                ctxt);
    /*
    * Resolve identity-constraint keyRefs.
    */
    xmlHashScan(ret->idcDef, (xmlHashScanner) xmlSchemaResolveIDCKeyRef, ctxt);
    /*
    * Check type defnitions for circular references.
    */
    xmlHashScan(ret->typeDecl, (xmlHashScanner)
	xmlSchemaCheckTypeDefCircular, ctxt);
    /*
    * Check model groups defnitions for circular references.
    */
    xmlHashScan(ret->groupDecl, (xmlHashScanner)
	xmlSchemaCheckGroupDefCircular, ctxt);
    /*
    * Set the "term" of particles pointing to model group definitions
    * to the contained model group.
    */
    if (ret->volatiles != NULL) {
	xmlSchemaItemListPtr list = (xmlSchemaItemListPtr) ret->volatiles;
	int i;
	xmlSchemaParticlePtr item;

	for (i = 0; i < list->nbItems; i++) {
	    item = (xmlSchemaParticlePtr) list->items[i];
	    if (item->type == XML_SCHEMA_TYPE_PARTICLE)
		xmlSchemaGroupDefTermFixup(item, ctxt, NULL);
	}
    }
    /*
    * Check attribute groups for circular references.
    */
    xmlHashScan(ret->attrgrpDecl, (xmlHashScanner)
	xmlSchemaCheckAttributeGroupCircular, ctxt);
    /*
     * Then fix references of element declaration; apply constraints.
     */
    xmlHashScanFull(ret->elemDecl,
                    (xmlHashScannerFull) xmlSchemaElementFixup, ctxt);
    /*
    * We will stop here if the schema was not valid to avoid internal errors
    * on missing sub-components. This is not conforming to the spec, since it
    * allows missing components, but it might make further processing crash.
    * So see it as a very strict handling, which might be made more lax in the
    * future.
    */
    if (ctxt->nberrors != 0)
	goto exit;
    /*
     * Then fixup all types properties
     */
    xmlHashScan(ret->typeDecl, (xmlHashScanner) xmlSchemaTypeFixup, ctxt);
    /*
    * Validate the value constraint of attribute declarations/uses.
    */
    xmlHashScan(ret->attrDecl, (xmlHashScanner) xmlSchemaCheckAttrValConstr, ctxt);
    /*
    * Validate the value constraint of element declarations.
    */
    xmlHashScan(ret->elemDecl, (xmlHashScanner) xmlSchemaCheckElementDeclComponent, ctxt);

    if (ctxt->nberrors != 0)
	goto exit;

    /*
    * TODO: cos-element-consistent, cos-all-limited
    *
    * Then build the content model for all complex types
    */
    xmlHashScan(ret->typeDecl,
                (xmlHashScanner) xmlSchemaBuildContentModel, ctxt);

exit:
    if (ctxt->nberrors != 0) {
        xmlSchemaFree(ret);
        ret = NULL;
    }
    return (ret);
}

/**
 * xmlSchemaSetParserErrors:
 * @ctxt:  a schema validation context
 * @err:  the error callback
 * @warn:  the warning callback
 * @ctx:  contextual data for the callbacks
 *
 * Set the callback functions used to handle errors for a validation context
 */
void
xmlSchemaSetParserErrors(xmlSchemaParserCtxtPtr ctxt,
                         xmlSchemaValidityErrorFunc err,
                         xmlSchemaValidityWarningFunc warn, void *ctx)
{
    if (ctxt == NULL)
        return;
    ctxt->error = err;
    ctxt->warning = warn;
    ctxt->userData = ctx;
}

/**
 * xmlSchemaGetParserErrors:
 * @ctxt:  a XMl-Schema parser context
 * @err: the error callback result
 * @warn: the warning callback result
 * @ctx: contextual data for the callbacks result
 *
 * Get the callback information used to handle errors for a parser context
 *
 * Returns -1 in case of failure, 0 otherwise
 */
int
xmlSchemaGetParserErrors(xmlSchemaParserCtxtPtr ctxt,
							 xmlSchemaValidityErrorFunc * err,
							 xmlSchemaValidityWarningFunc * warn, void **ctx)
{
	if (ctxt == NULL)
		return(-1);
	if (err != NULL)
		*err = ctxt->error;
	if (warn != NULL)
		*warn = ctxt->warning;
	if (ctx != NULL)
		*ctx = ctxt->userData;
	return(0);
}

/**
 * xmlSchemaFacetTypeToString:
 * @type:  the facet type
 *
 * Convert the xmlSchemaTypeType to a char string.
 *
 * Returns the char string representation of the facet type if the
 *     type is a facet and an "Internal Error" string otherwise.
 */
static const xmlChar *
xmlSchemaFacetTypeToString(xmlSchemaTypeType type)
{
    switch (type) {
        case XML_SCHEMA_FACET_PATTERN:
            return (BAD_CAST "pattern");
        case XML_SCHEMA_FACET_MAXEXCLUSIVE:
            return (BAD_CAST "maxExclusive");
        case XML_SCHEMA_FACET_MAXINCLUSIVE:
            return (BAD_CAST "maxInclusive");
        case XML_SCHEMA_FACET_MINEXCLUSIVE:
            return (BAD_CAST "minExclusive");
        case XML_SCHEMA_FACET_MININCLUSIVE:
            return (BAD_CAST "minInclusive");
        case XML_SCHEMA_FACET_WHITESPACE:
            return (BAD_CAST "whiteSpace");
        case XML_SCHEMA_FACET_ENUMERATION:
            return (BAD_CAST "enumeration");
        case XML_SCHEMA_FACET_LENGTH:
            return (BAD_CAST "length");
        case XML_SCHEMA_FACET_MAXLENGTH:
            return (BAD_CAST "maxLength");
        case XML_SCHEMA_FACET_MINLENGTH:
            return (BAD_CAST "minLength");
        case XML_SCHEMA_FACET_TOTALDIGITS:
            return (BAD_CAST "totalDigits");
        case XML_SCHEMA_FACET_FRACTIONDIGITS:
            return (BAD_CAST "fractionDigits");
        default:
            break;
    }
    return (BAD_CAST "Internal Error");
}

static xmlSchemaWhitespaceValueType
xmlSchemaGetWhiteSpaceFacetValue(xmlSchemaTypePtr type)
{
    /*
    * The normalization type can be changed only for types which are derived
    * from xsd:string.
    */
    if (type->type == XML_SCHEMA_TYPE_BASIC) {
	/*
	* Note that we assume a whitespace of preserve for anySimpleType.
	*/
	if ((type->builtInType == XML_SCHEMAS_STRING) ||
	    (type->builtInType == XML_SCHEMAS_ANYSIMPLETYPE))
	    return(XML_SCHEMA_WHITESPACE_PRESERVE);
	else if (type->builtInType == XML_SCHEMAS_NORMSTRING)
	    return(XML_SCHEMA_WHITESPACE_REPLACE);
	else {
	    /*
	    * For all ·atomic· datatypes other than string (and types ·derived·
	    * by ·restriction· from it) the value of whiteSpace is fixed to
	    * collapse
	    * Note that this includes built-in list datatypes.
	    */
	    return(XML_SCHEMA_WHITESPACE_COLLAPSE);
	}
    } else if (VARIETY_LIST(type)) {
	/*
	* For list types the facet "whiteSpace" is fixed to "collapse".
	*/
	return (XML_SCHEMA_WHITESPACE_COLLAPSE);
    } else if (VARIETY_UNION(type)) {
	return (XML_SCHEMA_WHITESPACE_UNKNOWN);
    } else if (VARIETY_ATOMIC(type)) {
	if (type->flags & XML_SCHEMAS_TYPE_WHITESPACE_PRESERVE)
	    return (XML_SCHEMA_WHITESPACE_PRESERVE);
	else if (type->flags & XML_SCHEMAS_TYPE_WHITESPACE_REPLACE)
	    return (XML_SCHEMA_WHITESPACE_REPLACE);
	else
	    return (XML_SCHEMA_WHITESPACE_COLLAPSE);
    }
    return (-1);
}

/************************************************************************
 * 									*
 * 			Simple type validation				*
 * 									*
 ************************************************************************/


/************************************************************************
 * 									*
 * 			DOM Validation code				*
 * 									*
 ************************************************************************/

static void
xmlSchemaPostSchemaAssembleFixup(xmlSchemaParserCtxtPtr ctxt)
{
    int i, nbItems;
    xmlSchemaTypePtr item, *items;


    /*
    * During the Assemble of the schema ctxt->curItems has
    * been filled with the relevant new items. Fix those up.
    */
    nbItems = ctxt->assemble->nbItems;
    items = (xmlSchemaTypePtr *) ctxt->assemble->items;

    for (i = 0; i < nbItems; i++) {
	item = items[i];
	switch (item->type) {
	    case XML_SCHEMA_TYPE_COMPLEX:
	    case XML_SCHEMA_TYPE_SIMPLE:
		xmlSchemaResolveTypeDefs(item, ctxt, NULL);
		break;
	    case XML_SCHEMA_TYPE_ATTRIBUTE:
		xmlSchemaAttrFixup((xmlSchemaAttributePtr) item, ctxt, NULL);
		break;
	    case XML_SCHEMA_TYPE_ATTRIBUTEGROUP:
		xmlSchemaAttrGrpFixup((xmlSchemaAttributeGroupPtr) item,
		    ctxt, NULL);
		break;
	    case XML_SCHEMA_TYPE_PARTICLE:
		xmlSchemaMiscRefFixup((xmlSchemaTreeItemPtr) item, ctxt, NULL);
		break;
	    case XML_SCHEMA_TYPE_IDC_KEY:
	    case XML_SCHEMA_TYPE_IDC_UNIQUE:
	    case XML_SCHEMA_TYPE_IDC_KEYREF:
		xmlSchemaResolveIDCKeyRef((xmlSchemaIDCPtr) item, ctxt, NULL);
		break;
	    default:
		break;
	}
    }
    if (ctxt->nberrors != 0)
	return;
    /*
    * Circularity checks.
    */
    for (i = 0; i < nbItems; i++) {
	item = items[i];
	switch (item->type) {
	    case XML_SCHEMA_TYPE_COMPLEX:
	    case XML_SCHEMA_TYPE_SIMPLE:
		xmlSchemaCheckTypeDefCircular(
		    (xmlSchemaTypePtr) item, ctxt, NULL);
		break;
	    case XML_SCHEMA_TYPE_GROUP:
		xmlSchemaCheckGroupDefCircular(
		    (xmlSchemaModelGroupDefPtr) item, ctxt, NULL);
		break;
	    case XML_SCHEMA_TYPE_ATTRIBUTEGROUP:
		xmlSchemaCheckAttributeGroupCircular(
		    (xmlSchemaAttributeGroupPtr) item, ctxt, NULL);
		break;
	    default:
		break;
	}
    }
    if (ctxt->nberrors != 0)
	return;
    /*
    * Set the "term" of particles pointing to model group definitions
    * to the contained model group.
    */
    for (i = 0; i < nbItems; i++) {
	item = items[i];
	if ((item->type == XML_SCHEMA_TYPE_PARTICLE) &&
	    (((xmlSchemaParticlePtr) item)->children != NULL) &&
	    (((xmlSchemaParticlePtr) item)->children->type ==
	    XML_SCHEMA_TYPE_GROUP)) {
	    xmlSchemaGroupDefTermFixup((xmlSchemaParticlePtr) item,
		ctxt, NULL);
	}
    }
    if (ctxt->nberrors != 0)
	return;
    for (i = 0; i < nbItems; i++) {
	item = items[i];
	switch (item->type) {
	    case XML_SCHEMA_TYPE_ELEMENT:
		xmlSchemaElementFixup((xmlSchemaElementPtr) item, ctxt,
		    NULL, NULL, NULL);
		break;
	    default:
		break;
	}
    }
    if (ctxt->nberrors != 0)
	return;

    /*
    * Fixup for simple/complex types.
    */
    for (i = 0; i < nbItems; i++) {
	item = items[i];
	switch (item->type) {
            case XML_SCHEMA_TYPE_SIMPLE:
	    case XML_SCHEMA_TYPE_COMPLEX:
		xmlSchemaTypeFixup(item, ctxt, NULL);
		break;
	    default:
		break;
	}
    }
    if (ctxt->nberrors != 0)
	return;
    /*
    * Validate value contraint values.
    */
    for (i = 0; i < nbItems; i++) {
	item = items[i];
	switch (item->type) {
	    case XML_SCHEMA_TYPE_ATTRIBUTE:
		xmlSchemaCheckAttrValConstr((xmlSchemaAttributePtr) item,
		    ctxt, NULL);
		break;
	    case XML_SCHEMA_TYPE_ELEMENT:
		xmlSchemaCheckElementDeclComponent((xmlSchemaElementPtr) item,
		    ctxt, NULL);
		break;
	    default:
		break;
	}
    }
    if (ctxt->nberrors != 0)
	return;
    /*
    * Build the content model for complex types.
    */
    for (i = 0; i < nbItems; i++) {
	item = items[i];
	switch (item->type) {
	    case XML_SCHEMA_TYPE_COMPLEX:
		xmlSchemaBuildContentModel(item, ctxt, NULL);
		break;
	    default:
		break;
	}
    }
}

/**
 * xmlSchemaAssembleByLocation:
 * @pctxt:  a schema parser context
 * @vctxt:  a schema validation context
 * @schema: the existing schema
 * @node: the node that fired the assembling
 * @nsName: the namespace name of the new schema
 * @location: the location of the schema
 *
 * Expands an existing schema by an additional schema.
 *
 * Returns 0 if the new schema is correct, a positive error code
 * number otherwise and -1 in case of an internal or API error.
 */
static int
xmlSchemaAssembleByLocation(xmlSchemaValidCtxtPtr vctxt,
			    xmlSchemaPtr schema,
			    xmlNodePtr node,
			    const xmlChar *nsName,
			    const xmlChar *location)
{
    const xmlChar *targetNs, *oldtns;
    xmlDocPtr doc, olddoc;
    int oldflags, ret = 0, oldIsS4S;
    xmlNodePtr docElem;
    xmlSchemaParserCtxtPtr pctxt;

    /*
    * This should be used:
    * 1. on <import>(s)
    * 2. if requested by the validated instance
    * 3. if requested via the API
    */
    if ((vctxt == NULL) || (schema == NULL))
	return (-1);
    /*
    * Create a temporary parser context.
    */
    if ((vctxt->pctxt == NULL) &&
	(xmlSchemaCreatePCtxtOnVCtxt(vctxt) == -1))
	return (-1);
    pctxt = vctxt->pctxt;
    /*
    * Set the counter to produce unique names for anonymous items.
    */
    pctxt->counter = schema->counter;
    /*
    * Acquire the schema document.
    */
    ret = xmlSchemaAcquireSchemaDoc((xmlSchemaAbstractCtxtPtr) vctxt, schema,
	node, nsName, location, &doc, &targetNs, 0);
    if (ret != 0) {
	if (doc != NULL)
	    xmlFreeDoc(doc);
    } else if (doc != NULL) {
	docElem = xmlDocGetRootElement(doc);
	/*
	* Create new assemble info.
	*/
	if (pctxt->assemble == NULL) {
	    pctxt->assemble = xmlSchemaNewAssemble();
	    if (pctxt->assemble == NULL) {
		xmlSchemaVErrMemory(vctxt,
		    "Memory error: xmlSchemaAssembleByLocation, "
		    "allocating assemble info", NULL);
		xmlFreeDoc(doc);
		return (-1);
	    }
	}
	/*
	* Save and reset the context & schema.
	*/
	oldflags = schema->flags;
	oldtns = schema->targetNamespace;
	olddoc = schema->doc;
	oldIsS4S = vctxt->pctxt->isS4S;

	xmlSchemaClearSchemaDefaults(schema);
	schema->targetNamespace = targetNs;
	if ((targetNs != NULL) &&
	    xmlStrEqual(targetNs, xmlSchemaNs)) {
	    /*
	    * We are parsing the schema for schema!
	    */
	    vctxt->pctxt->isS4S = 1;
	}
	/* schema->nbCurItems = 0; */
	pctxt->schema = schema;
	pctxt->ctxtType = NULL;
	pctxt->parentItem = NULL;

	xmlSchemaParseSchemaDefaults(pctxt, schema, docElem);
	if (pctxt->nberrors != 0) {
	    vctxt->nberrors += pctxt->nberrors;
	    goto finally;
	}
	xmlSchemaParseSchemaTopLevel(pctxt, schema, docElem->children);
	if (pctxt->nberrors != 0) {
	    vctxt->nberrors += pctxt->nberrors;
	    goto finally;
	}
	xmlSchemaPostSchemaAssembleFixup(pctxt);
	if (pctxt->nberrors != 0)
	    vctxt->nberrors += pctxt->nberrors;
finally:
	/*
	* Set the counter of items.
	*/
	schema->counter = pctxt->counter;
	/*
	* Free the list of assembled components.
	*/
	pctxt->assemble->nbItems = 0;
	/*
	* Restore the context & schema.
	*/
	vctxt->pctxt->isS4S = oldIsS4S;
	schema->flags = oldflags;
	schema->targetNamespace = oldtns;
	schema->doc = olddoc;
	ret = pctxt->err;
    }
    return (ret);
}

static xmlSchemaAttrInfoPtr
xmlSchemaGetMetaAttrInfo(xmlSchemaValidCtxtPtr vctxt,		      
			 int metaType)
{
    if (vctxt->nbAttrInfos == 0)
	return (NULL);
    {
	int i;
	xmlSchemaAttrInfoPtr iattr;

	for (i = 0; i < vctxt->nbAttrInfos; i++) {
	    iattr = vctxt->attrInfos[i];
	    if (iattr->metaType == metaType)
		return (iattr);
	}

    }
    return (NULL);
}

/**
 * xmlSchemaAssembleByXSI:
 * @vctxt:  a schema validation context
 *
 * Expands an existing schema by an additional schema using
 * the xsi:schemaLocation or xsi:noNamespaceSchemaLocation attribute
 * of an instance. If xsi:noNamespaceSchemaLocation is used, @noNamespace
 * must be set to 1.
 *
 * Returns 0 if the new schema is correct, a positive error code
 * number otherwise and -1 in case of an internal or API error.
 */
static int
xmlSchemaAssembleByXSI(xmlSchemaValidCtxtPtr vctxt)
{
    const xmlChar *cur, *end;
    const xmlChar *nsname = NULL, *location;
    int count = 0;
    int ret = 0;
    xmlSchemaAttrInfoPtr iattr;

    /*
    * Parse the value; we will assume an even number of values
    * to be given (this is how Xerces and XSV work).
    */
    iattr = xmlSchemaGetMetaAttrInfo(vctxt,
	XML_SCHEMA_ATTR_INFO_META_XSI_SCHEMA_LOC);
    if (iattr == NULL)
	xmlSchemaGetMetaAttrInfo(vctxt,
	XML_SCHEMA_ATTR_INFO_META_XSI_NO_NS_SCHEMA_LOC);
    if (iattr == NULL)
	return (0);
    cur = iattr->value;
    do {
	if (iattr->metaType == XML_SCHEMA_ATTR_INFO_META_XSI_SCHEMA_LOC) {
	    /*
	    * Get the namespace name.
	    */
	    while (IS_BLANK_CH(*cur))
		cur++;
	    end = cur;
	    while ((*end != 0) && (!(IS_BLANK_CH(*end))))
		end++;
	    if (end == cur)
		break;
	    count++;
	    nsname = xmlDictLookup(vctxt->schema->dict, cur, end - cur);
	    cur = end;
	}
	/*
	* Get the URI.
	*/
	while (IS_BLANK_CH(*cur))
	    cur++;
	end = cur;
	while ((*end != 0) && (!(IS_BLANK_CH(*end))))
	    end++;
	if (end == cur)
	    break;
	count++;
	location = xmlDictLookup(vctxt->schema->dict, cur, end - cur);
	cur = end;
	ret = xmlSchemaAssembleByLocation(vctxt, vctxt->schema,
	    iattr->node, nsname, location);
	if (ret == -1) {
	    VERROR_INT("xmlSchemaAssembleByXSI",
		"assembling schemata");
	    return (-1);
	}
    } while (*cur != 0);
    return (ret);
}

#define VAL_CREATE_DICT if (vctxt->dict == NULL) vctxt->dict = xmlDictCreate();

static const xmlChar *
xmlSchemaLookupNamespace(xmlSchemaValidCtxtPtr vctxt,
			 const xmlChar *prefix)
{
    if (vctxt->sax != NULL) {
	int i, j;
	xmlSchemaNodeInfoPtr inode;
	
	for (i = vctxt->depth; i >= 0; i--) {
	    if (vctxt->elemInfos[i]->nbNsBindings != 0) {
		inode = vctxt->elemInfos[i];
		for (j = 0; j < inode->nbNsBindings * 2; j += 2) {
		    if (((prefix == NULL) &&
			    (inode->nsBindings[j] == NULL)) ||
			((prefix != NULL) && xmlStrEqual(prefix,
			    inode->nsBindings[j]))) {

			/*
			* Note that the namespace bindings are already
			* in a string dict.
			*/
			return (inode->nsBindings[j+1]);			
		    }
		}
	    }
	}
	return (NULL);
#ifdef LIBXML_WRITER_ENABLED
    } else if (vctxt->reader != NULL) {
	xmlChar *nsName;
	
	nsName = xmlTextReaderLookupNamespace(vctxt->reader, prefix);
	if (nsName != NULL) {
	    const xmlChar *ret;

	    VAL_CREATE_DICT;
	    ret = xmlDictLookup(vctxt->dict, nsName, -1);
	    xmlFree(nsName);
	    return (ret);
	} else
	    return (NULL);
#endif
    } else {
	xmlNsPtr ns;

	if ((vctxt->inode->node == NULL) ||
	    (vctxt->inode->node->doc == NULL)) {
	    VERROR_INT("xmlSchemaLookupNamespace",
		"no node or node's doc avaliable");
	    return (NULL);
	}
	ns = xmlSearchNs(vctxt->inode->node->doc,
	    vctxt->inode->node, prefix);
	if (ns != NULL)
	    return (ns->href);
	return (NULL);
    }
}

/*
* This one works on the schema of the validation context.
*/
static int
xmlSchemaValidateNotation(xmlSchemaValidCtxtPtr vctxt, 			  
			  xmlSchemaPtr schema,
			  xmlNodePtr node,
			  const xmlChar *value,
			  xmlSchemaValPtr *val,
			  int valNeeded)
{
    int ret;

    if (vctxt && (vctxt->schema == NULL)) {
	VERROR_INT("xmlSchemaValidateNotation",
	    "a schema is needed on the validation context");
	return (-1);
    }
    ret = xmlValidateQName(value, 1);
    if (ret != 0)
	return (ret);
    {
	xmlChar *localName = NULL;
	xmlChar *prefix = NULL;

	localName = xmlSplitQName2(value, &prefix);
	if (prefix != NULL) {
	    const xmlChar *nsName = NULL;

	    if (vctxt != NULL) 
		nsName = xmlSchemaLookupNamespace(vctxt, BAD_CAST prefix);
	    else if (node != NULL) {
		xmlNsPtr ns = xmlSearchNs(node->doc, node, prefix);
		if (ns != NULL)
		    nsName = ns->href;
	    } else {
		xmlFree(prefix);
		xmlFree(localName);
		return (1);
	    }
	    if (nsName == NULL) {
		xmlFree(prefix);
		xmlFree(localName);
		return (1);
	    }
	    if (xmlHashLookup2(schema->notaDecl, localName,
		nsName) != NULL) {
		if (valNeeded && (val != NULL)) {
		    (*val) = xmlSchemaNewNOTATIONValue(BAD_CAST localName,
			BAD_CAST xmlStrdup(nsName));
		    if (*val == NULL)
			ret = -1;
		}
	    } else
		ret = 1;
	    xmlFree(prefix);
	    xmlFree(localName);
	} else {
	    if (xmlHashLookup2(schema->notaDecl, value, NULL) != NULL) {
		if (valNeeded && (val != NULL)) {
		    (*val) = xmlSchemaNewNOTATIONValue(
			BAD_CAST xmlStrdup(value), NULL);
		    if (*val == NULL)
			ret = -1;
		}
	    } else
		return (1);
	}
    }
    return (ret);
}

/************************************************************************
 * 									*
 *  Validation of identity-constraints (IDC)                            *
 * 									*
 ************************************************************************/

/**
 * xmlSchemaAugmentIDC:
 * @idcDef: the IDC definition
 *
 * Creates an augmented IDC definition item.
 *
 * Returns the item, or NULL on internal errors.
 */
static void
xmlSchemaAugmentIDC(xmlSchemaIDCPtr idcDef,
		    xmlSchemaValidCtxtPtr vctxt)
{
    xmlSchemaIDCAugPtr aidc;

    aidc = (xmlSchemaIDCAugPtr) xmlMalloc(sizeof(xmlSchemaIDCAug));
    if (aidc == NULL) {
	xmlSchemaVErrMemory(vctxt,
	    "xmlSchemaAugmentIDC: allocating an augmented IDC definition",
	    NULL);
	return;
    }
    aidc->bubbleDepth = -1;
    aidc->def = idcDef;
    aidc->next = NULL;
    if (vctxt->aidcs == NULL)
	vctxt->aidcs = aidc;
    else {
	aidc->next = vctxt->aidcs;
	vctxt->aidcs = aidc;
    }
}

/**
 * xmlSchemaIDCNewBinding:
 * @idcDef: the IDC definition of this binding
 *
 * Creates a new IDC binding.
 *
 * Returns the new binding in case of succeeded, NULL on internal errors.
 */
static xmlSchemaPSVIIDCBindingPtr
xmlSchemaIDCNewBinding(xmlSchemaIDCPtr idcDef)
{
    xmlSchemaPSVIIDCBindingPtr ret;

    ret = (xmlSchemaPSVIIDCBindingPtr) xmlMalloc(
	    sizeof(xmlSchemaPSVIIDCBinding));
    if (ret == NULL) {
	xmlSchemaVErrMemory(NULL,
	    "allocating a PSVI IDC binding item", NULL);
	return (NULL);
    }
    memset(ret, 0, sizeof(xmlSchemaPSVIIDCBinding));
    ret->definition = idcDef;
    return (ret);
}

/**
 * xmlSchemaIDCStoreNodeTableItem:
 * @vctxt: the WXS validation context
 * @item: the IDC node table item
 *
 * The validation context is used to store an IDC node table items.
 * They are stored to avoid copying them if IDC node-tables are merged
 * with corresponding parent IDC node-tables (bubbling).
 *
 * Returns 0 if succeeded, -1 on internal errors.
 */
static int
xmlSchemaIDCStoreNodeTableItem(xmlSchemaValidCtxtPtr vctxt,
			       xmlSchemaPSVIIDCNodePtr item)
{
    /*
    * Add to gobal list.
    */
    if (vctxt->idcNodes == NULL) {
	vctxt->idcNodes = (xmlSchemaPSVIIDCNodePtr *)
	    xmlMalloc(20 * sizeof(xmlSchemaPSVIIDCNodePtr));
	if (vctxt->idcNodes == NULL) {
	    xmlSchemaVErrMemory(vctxt,
		"allocating the IDC node table item list", NULL);
	    return (-1);
	}
	vctxt->sizeIdcNodes = 20;
    } else if (vctxt->sizeIdcNodes <= vctxt->nbIdcNodes) {
	vctxt->sizeIdcNodes *= 2;
	vctxt->idcNodes = (xmlSchemaPSVIIDCNodePtr *)
	    xmlRealloc(vctxt->idcNodes, vctxt->sizeIdcNodes *
	    sizeof(xmlSchemaPSVIIDCNodePtr));
	if (vctxt->idcNodes == NULL) {
	    xmlSchemaVErrMemory(vctxt,
		"re-allocating the IDC node table item list", NULL);
	    return (-1);
	}
    }
    vctxt->idcNodes[vctxt->nbIdcNodes++] = item;

    return (0);
}

/**
 * xmlSchemaIDCStoreKey:
 * @vctxt: the WXS validation context
 * @item: the IDC key
 *
 * The validation context is used to store an IDC key.
 *
 * Returns 0 if succeeded, -1 on internal errors.
 */
static int
xmlSchemaIDCStoreKey(xmlSchemaValidCtxtPtr vctxt,
		     xmlSchemaPSVIIDCKeyPtr key)
{
    /*
    * Add to gobal list.
    */
    if (vctxt->idcKeys == NULL) {
	vctxt->idcKeys = (xmlSchemaPSVIIDCKeyPtr *)
	    xmlMalloc(40 * sizeof(xmlSchemaPSVIIDCKeyPtr));
	if (vctxt->idcKeys == NULL) {
	    xmlSchemaVErrMemory(vctxt,
		"allocating the IDC key storage list", NULL);
	    return (-1);
	}
	vctxt->sizeIdcKeys = 40;
    } else if (vctxt->sizeIdcKeys <= vctxt->nbIdcKeys) {
	vctxt->sizeIdcKeys *= 2;
	vctxt->idcKeys = (xmlSchemaPSVIIDCKeyPtr *)
	    xmlRealloc(vctxt->idcKeys, vctxt->sizeIdcKeys *
	    sizeof(xmlSchemaPSVIIDCKeyPtr));
	if (vctxt->idcKeys == NULL) {
	    xmlSchemaVErrMemory(vctxt,
		"re-allocating the IDC key storage list", NULL);
	    return (-1);
	}
    }
    vctxt->idcKeys[vctxt->nbIdcKeys++] = key;

    return (0);
}

/**
 * xmlSchemaIDCAppendNodeTableItem:
 * @bind: the IDC binding
 * @ntItem: the node-table item
 *
 * Appends the IDC node-table item to the binding.
 *
 * Returns 0 on success and -1 on internal errors.
 */
static int
xmlSchemaIDCAppendNodeTableItem(xmlSchemaPSVIIDCBindingPtr bind,
				xmlSchemaPSVIIDCNodePtr ntItem)
{
    if (bind->nodeTable == NULL) {
	bind->sizeNodes = 10;
	bind->nodeTable = (xmlSchemaPSVIIDCNodePtr *)
	    xmlMalloc(10 * sizeof(xmlSchemaPSVIIDCNodePtr));
	if (bind->nodeTable == NULL) {
	    xmlSchemaVErrMemory(NULL,
		"allocating an array of IDC node-table items", NULL);
	    return(-1);
	}
    } else if (bind->sizeNodes <= bind->nbNodes) {
	bind->sizeNodes *= 2;
	bind->nodeTable = (xmlSchemaPSVIIDCNodePtr *)
	    xmlRealloc(bind->nodeTable, bind->sizeNodes *
		sizeof(xmlSchemaPSVIIDCNodePtr));
	if (bind->nodeTable == NULL) {
	    xmlSchemaVErrMemory(NULL,
		"re-allocating an array of IDC node-table items", NULL);
	    return(-1);
	}
    }
    bind->nodeTable[bind->nbNodes++] = ntItem;
    return(0);
}

/**
 * xmlSchemaIDCAquireBinding:
 * @vctxt: the WXS validation context
 * @matcher: the IDC matcher
 *
 * Looks up an PSVI IDC binding, for the IDC definition and
 * of the given matcher. If none found, a new one is created
 * and added to the IDC table.
 *
 * Returns an IDC binding or NULL on internal errors.
 */
static xmlSchemaPSVIIDCBindingPtr
xmlSchemaIDCAquireBinding(xmlSchemaValidCtxtPtr vctxt,
			  xmlSchemaIDCMatcherPtr matcher)
{
    xmlSchemaNodeInfoPtr info;

    info = vctxt->elemInfos[matcher->depth];

    if (info->idcTable == NULL) {
	info->idcTable = xmlSchemaIDCNewBinding(matcher->aidc->def);
	if (info->idcTable == NULL)
	    return (NULL);
	return(info->idcTable);
    } else {
	xmlSchemaPSVIIDCBindingPtr bind = NULL;

	bind = info->idcTable;
	do {
	    if (bind->definition == matcher->aidc->def)
		return(bind);
	    if (bind->next == NULL) {
		bind->next = xmlSchemaIDCNewBinding(matcher->aidc->def);
		if (bind->next == NULL)
		    return (NULL);
		return(bind->next);
	    }
	    bind = bind->next;
	} while (bind != NULL);
    }
    return (NULL);
}

/**
 * xmlSchemaIDCFreeKey:
 * @key: the IDC key
 *
 * Frees an IDC key together with its compiled value.
 */
static void
xmlSchemaIDCFreeKey(xmlSchemaPSVIIDCKeyPtr key)
{
    if (key->val != NULL)
	xmlSchemaFreeValue(key->val);
    xmlFree(key);
}

/**
 * xmlSchemaIDCFreeBinding:
 *
 * Frees an IDC binding. Note that the node table-items
 * are not freed.
 */
static void
xmlSchemaIDCFreeBinding(xmlSchemaPSVIIDCBindingPtr bind)
{
    if (bind->nodeTable != NULL) {
	if (bind->definition->type == XML_SCHEMA_TYPE_IDC_KEYREF) {
	    int i;
	    /*
	    * Node-table items for keyrefs are not stored globally
	    * to the validation context, since they are not bubbled.
	    * We need to free them here.
	    */
	    for (i = 0; i < bind->nbNodes; i++) {
		xmlFree(bind->nodeTable[i]->keys);
		xmlFree(bind->nodeTable[i]);
	    }
	}
	xmlFree(bind->nodeTable);
    }
    xmlFree(bind);
}

/**
 * xmlSchemaIDCFreeIDCTable:
 * @bind: the first IDC binding in the list
 *
 * Frees an IDC table, i.e. all the IDC bindings in the list.
 */
static void
xmlSchemaIDCFreeIDCTable(xmlSchemaPSVIIDCBindingPtr bind)
{
    xmlSchemaPSVIIDCBindingPtr prev;

    while (bind != NULL) {
	prev = bind;
	bind = bind->next;
	xmlSchemaIDCFreeBinding(prev);
    }
}

/**
 * xmlSchemaIDCFreeMatcherList:
 * @matcher: the first IDC matcher in the list
 *
 * Frees a list of IDC matchers.
 */
static void
xmlSchemaIDCFreeMatcherList(xmlSchemaIDCMatcherPtr matcher)
{
    xmlSchemaIDCMatcherPtr next;

    while (matcher != NULL) {
	next = matcher->next;
	if (matcher->keySeqs != NULL) {
	    int i;
	    for (i = 0; i < matcher->sizeKeySeqs; i++)
		if (matcher->keySeqs[i] != NULL)
		    xmlFree(matcher->keySeqs[i]);
	    xmlFree(matcher->keySeqs);
	}
	xmlFree(matcher);
	matcher = next;
    }
}

/**
 * xmlSchemaIDCAddStateObject:
 * @vctxt: the WXS validation context
 * @matcher: the IDC matcher
 * @sel: the XPath information
 * @parent: the parent "selector" state object if any
 * @type: "selector" or "field"
 *
 * Creates/reuses and activates state objects for the given
 * XPath information; if the XPath expression consists of unions,
 * multiple state objects are created for every unioned expression.
 *
 * Returns 0 on success and -1 on internal errors.
 */
static int
xmlSchemaIDCAddStateObject(xmlSchemaValidCtxtPtr vctxt,
			xmlSchemaIDCMatcherPtr matcher,
			xmlSchemaIDCSelectPtr sel,
			int type)
{
    xmlSchemaIDCStateObjPtr sto;

    /*
    * Reuse the state objects from the pool.
    */
    if (vctxt->xpathStatePool != NULL) {
	sto = vctxt->xpathStatePool;
	vctxt->xpathStatePool = sto->next;
	sto->next = NULL;
    } else {	
	/*
	* Create a new state object.
	*/
	sto = (xmlSchemaIDCStateObjPtr) xmlMalloc(sizeof(xmlSchemaIDCStateObj));
	if (sto == NULL) {
	    xmlSchemaVErrMemory(NULL,
		"allocating an IDC state object", NULL);
	    return (-1);
	}
	memset(sto, 0, sizeof(xmlSchemaIDCStateObj));
    }	
    /*
    * Add to global list. 
    */	
    if (vctxt->xpathStates != NULL)
	sto->next = vctxt->xpathStates;
    vctxt->xpathStates = sto;

    /*
    * Free the old xpath validation context.
    */
    if (sto->xpathCtxt != NULL)
	xmlFreeStreamCtxt((xmlStreamCtxtPtr) sto->xpathCtxt);

    /*
    * Create a new XPath (pattern) validation context.
    */
    sto->xpathCtxt = (void *) xmlPatternGetStreamCtxt(
	(xmlPatternPtr) sel->xpathComp);
    if (sto->xpathCtxt == NULL) {
	VERROR_INT("xmlSchemaIDCAddStateObject",
	    "failed to create an XPath validation context");
	return (-1);
    }    
    sto->type = type;
    sto->depth = vctxt->depth;
    sto->matcher = matcher;
    sto->sel = sel;
    sto->nbHistory = 0;
    
#if DEBUG_IDC
    xmlGenericError(xmlGenericErrorContext, "IDC:   STO push '%s'\n",
	sto->sel->xpath);
#endif
    return (0);
}

/**
 * xmlSchemaXPathEvaluate:
 * @vctxt: the WXS validation context
 * @nodeType: the nodeType of the current node
 *
 * Evaluates all active XPath state objects.
 *
 * Returns the number of IC "field" state objects which resolved to
 * this node, 0 if none resolved and -1 on internal errors.
 */
static int
xmlSchemaXPathEvaluate(xmlSchemaValidCtxtPtr vctxt,
		       xmlElementType nodeType)
{
    xmlSchemaIDCStateObjPtr sto, head = NULL, first;
    int res, resolved = 0, depth = vctxt->depth;
        
    if (vctxt->xpathStates == NULL)
	return (0);

    if (nodeType == XML_ATTRIBUTE_NODE)
	depth++;
#if DEBUG_IDC
    {
	xmlChar *str = NULL;
	xmlGenericError(xmlGenericErrorContext, 
	    "IDC: EVAL on %s, depth %d, type %d\n",	    
	    xmlSchemaFormatQName(&str, vctxt->inode->nsName,
		vctxt->inode->localName), depth, nodeType);
	FREE_AND_NULL(str)
    }
#endif
    /*
    * Process all active XPath state objects.
    */
    first = vctxt->xpathStates;
    sto = first;
    while (sto != head) {
#if DEBUG_IDC
	if (sto->type == XPATH_STATE_OBJ_TYPE_IDC_SELECTOR)
	    xmlGenericError(xmlGenericErrorContext, "IDC:   ['%s'] selector '%s'\n", 
		sto->matcher->aidc->def->name, sto->sel->xpath);
	else
	    xmlGenericError(xmlGenericErrorContext, "IDC:   ['%s'] field '%s'\n", 
		sto->matcher->aidc->def->name, sto->sel->xpath);
#endif
	if (nodeType == XML_ELEMENT_NODE)
	    res = xmlStreamPush((xmlStreamCtxtPtr) sto->xpathCtxt,
		vctxt->inode->localName, vctxt->inode->nsName);
	else
	    res = xmlStreamPushAttr((xmlStreamCtxtPtr) sto->xpathCtxt,
		vctxt->inode->localName, vctxt->inode->nsName);

	if (res == -1) {
	    VERROR_INT("xmlSchemaXPathEvaluate",
		"calling xmlStreamPush()");
	    return (-1);
	}
	if (res == 0)
	    goto next_sto;
	/*
	* Full match.
	*/
#if DEBUG_IDC
	xmlGenericError(xmlGenericErrorContext, "IDC:     "
	    "MATCH\n");
#endif
	/*
	* Register a match in the state object history.
	*/
	if (sto->history == NULL) {
	    sto->history = (int *) xmlMalloc(5 * sizeof(int));
	    if (sto->history == NULL) {
		xmlSchemaVErrMemory(NULL, 
		    "allocating the state object history", NULL);
		return(-1);
	    }
	    sto->sizeHistory = 10;
	} else if (sto->sizeHistory <= sto->nbHistory) {
	    sto->sizeHistory *= 2;
	    sto->history = (int *) xmlRealloc(sto->history,
		sto->sizeHistory * sizeof(int));
	    if (sto->history == NULL) {
		xmlSchemaVErrMemory(NULL, 
		    "re-allocating the state object history", NULL);
		return(-1);
	    }
	}		
	sto->history[sto->nbHistory++] = depth;

#ifdef DEBUG_IDC
	xmlGenericError(xmlGenericErrorContext, "IDC:       push match '%d'\n",
	    vctxt->depth);
#endif

	if (sto->type == XPATH_STATE_OBJ_TYPE_IDC_SELECTOR) {
	    xmlSchemaIDCSelectPtr sel;
	    /*
	    * Activate state objects for the IDC fields of
	    * the IDC selector.
	    */
#if DEBUG_IDC
	    xmlGenericError(xmlGenericErrorContext, "IDC:     "
		"activating field states\n");
#endif
	    sel = sto->matcher->aidc->def->fields;
	    while (sel != NULL) {
		if (xmlSchemaIDCAddStateObject(vctxt, sto->matcher,
		    sel, XPATH_STATE_OBJ_TYPE_IDC_FIELD) == -1)
		    return (-1);
		sel = sel->next;
	    }
	} else if (sto->type == XPATH_STATE_OBJ_TYPE_IDC_FIELD) {
	    /*
	    * An IDC key node was found by the IDC field.
	    */
#if DEBUG_IDC
	    xmlGenericError(xmlGenericErrorContext,
		"IDC:     key found\n");
#endif
	    /*
	    * Notify that the character value of this node is
	    * needed.
	    */
	    if (resolved == 0) {
		if ((vctxt->inode->flags &
		    XML_SCHEMA_NODE_INFO_VALUE_NEEDED) == 0)
		vctxt->inode->flags |= XML_SCHEMA_NODE_INFO_VALUE_NEEDED;
	    }
	    resolved++;
	}
next_sto:
	if (sto->next == NULL) {
	    /*
	    * Evaluate field state objects created on this node as well.
	    */
	    head = first;
	    sto = vctxt->xpathStates;
	} else
	    sto = sto->next;
    }
    return (resolved);
}

static const xmlChar *
xmlSchemaFormatIDCKeySequence(xmlSchemaValidCtxtPtr vctxt,
			      xmlChar **buf,
			      xmlSchemaPSVIIDCKeyPtr *seq,
			      int count)
{
    int i, res;
    const xmlChar *value = NULL;

    *buf = xmlStrdup(BAD_CAST "[");
    for (i = 0; i < count; i++) {
	*buf = xmlStrcat(*buf, BAD_CAST "'");
	res = xmlSchemaGetCanonValueWhtsp(seq[i]->val, &value,
	    xmlSchemaGetWhiteSpaceFacetValue(seq[i]->type));
	if (res == 0)
	    *buf = xmlStrcat(*buf, value);
	else {
	    VERROR_INT("xmlSchemaFormatIDCKeySequence",
		"failed to compute a canonical value");
	    *buf = xmlStrcat(*buf, BAD_CAST "???");
	}
	if (i < count -1)
	    *buf = xmlStrcat(*buf, BAD_CAST "', ");
	else
	    *buf = xmlStrcat(*buf, BAD_CAST "'");
	if (value != NULL) {
	    xmlFree((xmlChar *) value);
	    value = NULL;
	}
    }
    *buf = xmlStrcat(*buf, BAD_CAST "]");

    return (BAD_CAST *buf);
}

/**
 * xmlSchemaXPathProcessHistory:
 * @vctxt: the WXS validation context
 * @type: the simple/complex type of the current node if any at all
 * @val: the precompiled value
 *
 * Processes and pops the history items of the IDC state objects.
 * IDC key-sequences are validated/created on IDC bindings.
 * 
 * Returns 0 on success and -1 on internal errors.
 */
static int
xmlSchemaXPathProcessHistory(xmlSchemaValidCtxtPtr vctxt,
			     int depth)
{
    xmlSchemaIDCStateObjPtr sto, nextsto;
    int res, matchDepth;
    xmlSchemaPSVIIDCKeyPtr key = NULL;
    xmlSchemaTypePtr type = vctxt->inode->typeDef;

    if (vctxt->xpathStates == NULL)
	return (0);
    sto = vctxt->xpathStates;

#if DEBUG_IDC
    {
	xmlChar *str = NULL;
	xmlGenericError(xmlGenericErrorContext, 
	    "IDC: BACK on %s, depth %d\n",
	    xmlSchemaFormatQName(&str, vctxt->inode->nsName,
		vctxt->inode->localName), vctxt->depth);
	FREE_AND_NULL(str)
    }
#endif    
    /*
    * Evaluate the state objects.
    */
    while (sto != NULL) {
	res = xmlStreamPop((xmlStreamCtxtPtr) sto->xpathCtxt);
	if (res == -1) {
	    VERROR_INT("xmlSchemaXPathProcessHistory",
		"calling xmlStreamPop()");
	    return (-1);
	}
#if DEBUG_IDC
	xmlGenericError(xmlGenericErrorContext, "IDC:   stream pop '%s'\n",
	    sto->sel->xpath);
#endif
	if (sto->nbHistory == 0)
	    goto deregister_check;

	matchDepth = sto->history[sto->nbHistory -1];

	/*
	* Only matches at the current depth are of interest.
	*/
	if (matchDepth != depth) {
	    sto = sto->next;
	    continue;
	}	
	if (sto->type == XPATH_STATE_OBJ_TYPE_IDC_FIELD) {
	    if (! IS_SIMPLE_TYPE(type)) {
		/*
		* Not qualified if the field resolves to a node of non
		* simple type.
		*/	
		xmlSchemaCustomErr((xmlSchemaAbstractCtxtPtr) vctxt,
		    XML_SCHEMAV_CVC_IDC, NULL,		    
		    (xmlSchemaTypePtr) sto->matcher->aidc->def,
		    "The field '%s' does evaluate to a node of "
		    "non-simple type", sto->sel->xpath, NULL);
		
		sto->nbHistory--;
		goto deregister_check;
	    }
	    if ((key == NULL) && (vctxt->inode->val == NULL)) {
		/*
		* Failed to provide the normalized value; maybe
		* the value was invalid.
		*/
		VERROR(XML_SCHEMAV_CVC_IDC,
		    (xmlSchemaTypePtr) sto->matcher->aidc->def,
		    "Warning: No precomputed value available, the value "
		    "was either invalid or something strange happend");
		sto->nbHistory--;
		goto deregister_check;
	    } else {
		xmlSchemaIDCMatcherPtr matcher = sto->matcher;
		xmlSchemaPSVIIDCKeyPtr *keySeq;
		int pos, idx;
		
		/*
		* The key will be anchored on the matcher's list of
		* key-sequences. The position in this list is determined
		* by the target node's depth relative to the matcher's
		* depth of creation (i.e. the depth of the scope element).
		*/		    
		pos = sto->depth - matcher->depth;
		idx = sto->sel->index;
		
		/*
		* Create/grow the array of key-sequences.
		*/
		if (matcher->keySeqs == NULL) {
		    if (pos > 9) 
			matcher->sizeKeySeqs = pos * 2;
		    else
			matcher->sizeKeySeqs = 10;
		    matcher->keySeqs = (xmlSchemaPSVIIDCKeyPtr **) 
			xmlMalloc(matcher->sizeKeySeqs *
			sizeof(xmlSchemaPSVIIDCKeyPtr *));			
		    if (matcher->keySeqs == NULL) {		
			xmlSchemaVErrMemory(NULL,
			    "allocating an array of key-sequences",
			    NULL);
			return(-1);
		    }
		    memset(matcher->keySeqs, 0,
			matcher->sizeKeySeqs *
			sizeof(xmlSchemaPSVIIDCKeyPtr *));
		} else if (pos >= matcher->sizeKeySeqs) {	
		    int i = matcher->sizeKeySeqs;
		    
		    matcher->sizeKeySeqs *= 2;
		    matcher->keySeqs = (xmlSchemaPSVIIDCKeyPtr **)
			xmlRealloc(matcher->keySeqs,
			matcher->sizeKeySeqs *
			sizeof(xmlSchemaPSVIIDCKeyPtr *));
		    if (matcher->keySeqs == NULL) {
			xmlSchemaVErrMemory(NULL,
			    "reallocating an array of key-sequences",
			    NULL);
			return (-1);
		    }
		    /*
		    * The array needs to be NULLed.
		    * TODO: Use memset?
		    */
		    for (; i < matcher->sizeKeySeqs; i++) 
			matcher->keySeqs[i] = NULL;			
		}
		
		/*
		* Get/create the key-sequence.
		*/
		keySeq = matcher->keySeqs[pos];		    
		if (keySeq == NULL) {	
		    goto create_sequence;
		} else {
		    if (keySeq[idx] != NULL) {
			/*
			* cvc-identity-constraint:
			* 3 For each node in the ·target node set· all
			* of the {fields}, with that node as the context
			* node, evaluate to either an empty node-set or
			* a node-set with exactly one member, which must
			* have a simple type.
			* 
			* The key was already set; report an error.
			*/
			xmlSchemaCustomErr((xmlSchemaAbstractCtxtPtr) vctxt, 
			    XML_SCHEMAV_CVC_IDC, NULL,
			    (xmlSchemaTypePtr) matcher->aidc->def,
			    "The field '%s' evaluates to a node-set "
			    "with more than one member",
			    sto->sel->xpath, NULL);
			sto->nbHistory--;
			goto deregister_check;
		    } else {
			goto create_key;
		    }
		}
		
create_sequence:
		/*
		* Create a key-sequence.
		*/
		keySeq = (xmlSchemaPSVIIDCKeyPtr *) xmlMalloc(
		    matcher->aidc->def->nbFields * 
		    sizeof(xmlSchemaPSVIIDCKeyPtr));
		if (keySeq == NULL) {
		    xmlSchemaVErrMemory(NULL, 
			"allocating an IDC key-sequence", NULL);
		    return(-1);			
		}	
		memset(keySeq, 0, matcher->aidc->def->nbFields * 
		    sizeof(xmlSchemaPSVIIDCKeyPtr));
		matcher->keySeqs[pos] = keySeq;
create_key:
		/*
		* Created a key once per node only.
		*/  
		if (key == NULL) {
		    key = (xmlSchemaPSVIIDCKeyPtr) xmlMalloc(
			sizeof(xmlSchemaPSVIIDCKey));
		    if (key == NULL) {
			xmlSchemaVErrMemory(NULL,
			    "allocating a IDC key", NULL);
			xmlFree(keySeq);
			matcher->keySeqs[pos] = NULL;
			return(-1);			
		    }
		    /*
		    * Consume the compiled value.
		    */
		    key->type = type;
		    key->val = vctxt->inode->val;
		    vctxt->inode->val = NULL;
		    /*
		    * Store the key in a global list.
		    */
		    if (xmlSchemaIDCStoreKey(vctxt, key) == -1) {
			xmlSchemaIDCFreeKey(key);
			return (-1);
		    }
		}
		keySeq[idx] = key;		    
	    }
	} else if (sto->type == XPATH_STATE_OBJ_TYPE_IDC_SELECTOR) {
		
	    xmlSchemaPSVIIDCKeyPtr **keySeq = NULL;
	    xmlSchemaPSVIIDCBindingPtr bind;
	    xmlSchemaPSVIIDCNodePtr ntItem;
	    xmlSchemaIDCMatcherPtr matcher;
	    xmlSchemaIDCPtr idc;
	    int pos, i, j, nbKeys;
	    /*
	    * Here we have the following scenario:
	    * An IDC 'selector' state object resolved to a target node,
	    * during the time this target node was in the 
	    * ancestor-or-self axis, the 'field' state object(s) looked 
	    * out for matching nodes to create a key-sequence for this 
	    * target node. Now we are back to this target node and need
	    * to put the key-sequence, together with the target node 
	    * itself, into the node-table of the corresponding IDC 
	    * binding.
	    */
	    matcher = sto->matcher;
	    idc = matcher->aidc->def;
	    nbKeys = idc->nbFields;
	    pos = depth - matcher->depth;		
	    /*
	    * Check if the matcher has any key-sequences at all, plus
	    * if it has a key-sequence for the current target node.
	    */		
	    if ((matcher->keySeqs == NULL) ||
		(matcher->sizeKeySeqs <= pos)) {
		if (idc->type == XML_SCHEMA_TYPE_IDC_KEY)
		    goto selector_key_error;
		else
		    goto selector_leave;
	    }
	    
	    keySeq = &(matcher->keySeqs[pos]);		
	    if (*keySeq == NULL) {
		if (idc->type == XML_SCHEMA_TYPE_IDC_KEY)
		    goto selector_key_error;
		else
		    goto selector_leave;
	    }
	    
	    for (i = 0; i < nbKeys; i++) {
		if ((*keySeq)[i] == NULL) {
		    /*
		    * Not qualified, if not all fields did resolve.
		    */
		    if (idc->type == XML_SCHEMA_TYPE_IDC_KEY) {
			/*
			* All fields of a "key" IDC must resolve.
			*/
			goto selector_key_error;
		    }		    
		    goto selector_leave;
		}
	    }
	    /*
	    * All fields did resolve.
	    */
	    
	    /*
	    * 4.1 If the {identity-constraint category} is unique(/key),
	    * then no two members of the ·qualified node set· have
	    * ·key-sequences· whose members are pairwise equal, as
	    * defined by Equal in [XML Schemas: Datatypes].
	    *
	    * Get the IDC binding from the matcher and check for
	    * duplicate key-sequences.
	    */
	    bind = xmlSchemaIDCAquireBinding(vctxt, matcher);
	    if ((idc->type != XML_SCHEMA_TYPE_IDC_KEYREF) && 
		(bind->nbNodes != 0)) {
		xmlSchemaPSVIIDCKeyPtr ckey, bkey, *bkeySeq;
		
		i = 0;
		res = 0;
		/*
		* Compare the key-sequences, key by key.
		*/
		do {
		    bkeySeq = bind->nodeTable[i]->keys;		    
		    for (j = 0; j < nbKeys; j++) {
			ckey = (*keySeq)[j];
			bkey = bkeySeq[j];							
			res = xmlSchemaAreValuesEqual(ckey->val, bkey->val);
			if (res == -1) {
			    return (-1);
			} else if (res == 0)
			    break;
		    }
		    if (res == 1) {
			/*
			* Duplicate found.
			*/
			break;
		    }
		    i++;
		} while (i < bind->nbNodes);
		if (i != bind->nbNodes) {
		    xmlChar *str = NULL;
		    /*   
		    * TODO: Try to report the key-sequence.
		    */
		    xmlSchemaCustomErr((xmlSchemaAbstractCtxtPtr) vctxt, 
			XML_SCHEMAV_CVC_IDC, NULL,
			(xmlSchemaTypePtr) idc,
			"Duplicate key-sequence %s",
			xmlSchemaFormatIDCKeySequence(vctxt, &str,
			    (*keySeq), nbKeys), NULL);
		    FREE_AND_NULL(str)
		    goto selector_leave;
		}
	    }
	    /*
	    * Add a node-table item to the IDC binding.
	    */
	    ntItem = (xmlSchemaPSVIIDCNodePtr) xmlMalloc(
		sizeof(xmlSchemaPSVIIDCNode));
	    if (ntItem == NULL) {
		xmlSchemaVErrMemory(NULL, 
		    "allocating an IDC node-table item", NULL);
		xmlFree(*keySeq);
		*keySeq = NULL;
		return(-1);
	    }	
	    memset(ntItem, 0, sizeof(xmlSchemaPSVIIDCNode));		
	    
	    /* 
	    * Store the node-table item on global list.
	    */
	    if (idc->type != XML_SCHEMA_TYPE_IDC_KEYREF) {
		if (xmlSchemaIDCStoreNodeTableItem(vctxt, ntItem) == -1) {
		    xmlFree(ntItem);
		    xmlFree(*keySeq);
		    *keySeq = NULL;
		    return (-1);
		}
	    }
	    /*
	    * Init the node-table item. Consume the key-sequence.
	    */
	    ntItem->node = vctxt->node;
	    ntItem->keys = *keySeq;
	    *keySeq = NULL;
	    if (xmlSchemaIDCAppendNodeTableItem(bind, ntItem) == -1) {		    
		if (idc->type == XML_SCHEMA_TYPE_IDC_KEYREF) {
		    /* 
		    * Free the item, since keyref items won't be
		    * put on a global list.
		    */
		    xmlFree(ntItem->keys);
		    xmlFree(ntItem);
		}
		return (-1);
	    }
	    
	    goto selector_leave;
selector_key_error:
	    /*
	    * 4.2.1 (KEY) The ·target node set· and the 
	    * ·qualified node set· are equal, that is, every 
	    * member of the ·target node set· is also a member
	    * of the ·qualified node set· and vice versa.
	    */
	    VERROR(XML_SCHEMAV_CVC_IDC, (xmlSchemaTypePtr) idc,
		"All 'key' fields must evaluate to a node");
selector_leave:
	    /*
	    * Free the key-sequence if not added to the IDC table.
	    */
	    if ((keySeq != NULL) && (*keySeq != NULL)) {
		xmlFree(*keySeq);
		*keySeq = NULL;
	    }
	} /* if selector */
	
	sto->nbHistory--;

deregister_check:
	/*
	* Deregister state objects if they reach the depth of creation.
	*/
	if ((sto->nbHistory == 0) && (sto->depth == depth)) {
#if DEBUG_IDC
	    xmlGenericError(xmlGenericErrorContext, "IDC:   STO pop '%s'\n",
		sto->sel->xpath);
#endif
	    if (vctxt->xpathStates != sto) {
		VERROR_INT("xmlSchemaXPathProcessHistory",
		    "The state object to be removed is not the first "
		    "in the list");
	    }
	    nextsto = sto->next;
	    /*
	    * Unlink from the list of active XPath state objects.
	    */
	    vctxt->xpathStates = sto->next;
	    sto->next = vctxt->xpathStatePool;
	    /*
	    * Link it to the pool of reusable state objects.
	    */
	    vctxt->xpathStatePool = sto;	    
	    sto = nextsto;
	} else
	    sto = sto->next;
    } /* while (sto != NULL) */
    return (0);
}

/**
 * xmlSchemaIDCRegisterMatchers:
 * @vctxt: the WXS validation context
 * @elemDecl: the element declaration
 *
 * Creates helper objects to evaluate IDC selectors/fields
 * successively.
 *
 * Returns 0 if OK and -1 on internal errors.
 */
static int
xmlSchemaIDCRegisterMatchers(xmlSchemaValidCtxtPtr vctxt,
			     xmlSchemaElementPtr elemDecl)
{
    xmlSchemaIDCMatcherPtr matcher, last = NULL;
    xmlSchemaIDCPtr idc, refIdc;
    xmlSchemaIDCAugPtr aidc;
        
    idc = (xmlSchemaIDCPtr) elemDecl->idcs;
    if (idc == NULL)
	return (0);
    
#if DEBUG_IDC
    {
	xmlChar *str = NULL;
	xmlGenericError(xmlGenericErrorContext, 
	    "IDC: REGISTER on %s, depth %d\n",
	    (char *) xmlSchemaFormatQName(&str, vctxt->inode->nsName,
		vctxt->inode->localName), vctxt->depth);
	FREE_AND_NULL(str)
    }
#endif
    if (vctxt->inode->idcMatchers != NULL) {
	VERROR_INT("xmlSchemaIDCRegisterMatchers",
	    "The chain of IDC matchers is expected to be empty");
	return (-1);
    }
    do {
	if (idc->type == XML_SCHEMA_TYPE_IDC_KEYREF) {
	    /*
	    * Since IDCs bubbles are expensive we need to know the
	    * depth at which the bubbles should stop; this will be
	    * the depth of the top-most keyref IDC. If no keyref
	    * references a key/unique IDC, the bubbleDepth will
	    * be -1, indicating that no bubbles are needed.
	    */
	    refIdc = (xmlSchemaIDCPtr) idc->ref->item;
	    if (refIdc != NULL) {
		/*
		* Lookup the augmented IDC.
		*/
		aidc = vctxt->aidcs;
		while (aidc != NULL) {
		    if (aidc->def == refIdc)
			break;
		    aidc = aidc->next;
		}
		if (aidc == NULL) {
		    VERROR_INT("xmlSchemaIDCRegisterMatchers",
			"Could not find an augmented IDC item for an IDC "
			"definition");
		    return (-1);
		}		
		if ((aidc->bubbleDepth == -1) ||
		    (vctxt->depth < aidc->bubbleDepth))
		    aidc->bubbleDepth = vctxt->depth;
	    }
	}
	/*
	* Lookup the augmented IDC item for the IDC definition.
	*/
	aidc = vctxt->aidcs;
	while (aidc != NULL) {
	    if (aidc->def == idc)
		break;
	    aidc = aidc->next;
	}
	if (aidc == NULL) {
	    VERROR_INT("xmlSchemaIDCRegisterMatchers",
		"Could not find an augmented IDC item for an IDC definition");
	    return (-1);
	}
	/*
	* Create an IDC matcher for every IDC definition.
	*/
	matcher = (xmlSchemaIDCMatcherPtr) 
	    xmlMalloc(sizeof(xmlSchemaIDCMatcher));
	if (matcher == NULL) {
	    xmlSchemaVErrMemory(vctxt, 
		"allocating an IDC matcher", NULL);
	    return (-1);
	}
	memset(matcher, 0, sizeof(xmlSchemaIDCMatcher));
	if (last == NULL)
	    vctxt->inode->idcMatchers = matcher;
	else
	    last->next = matcher;
	last = matcher;

	matcher->type = IDC_MATCHER;
	matcher->depth = vctxt->depth;	
	matcher->aidc = aidc;
#if DEBUG_IDC	
	xmlGenericError(xmlGenericErrorContext, "IDC:   register matcher\n");
#endif 
	/*
	* Init the automaton state object. 
	*/
	if (xmlSchemaIDCAddStateObject(vctxt, matcher, 
	    idc->selector, XPATH_STATE_OBJ_TYPE_IDC_SELECTOR) == -1)
	    return (-1);

	idc = idc->next;
    } while (idc != NULL);
    return (0);
}

/**
 * xmlSchemaBubbleIDCNodeTables: 
 * @depth: the current tree depth
 *
 * Merges IDC bindings of an element at @depth into the corresponding IDC 
 * bindings of its parent element. If a duplicate note-table entry is found, 
 * both, the parent node-table entry and child entry are discarded from the 
 * node-table of the parent.
 *
 * Returns 0 if OK and -1 on internal errors.
 */
static int
xmlSchemaBubbleIDCNodeTables(xmlSchemaValidCtxtPtr vctxt)
{
    xmlSchemaPSVIIDCBindingPtr bind; /* IDC bindings of the current node. */
    xmlSchemaPSVIIDCBindingPtr *parTable, parBind = NULL, lastParBind = NULL; /* parent IDC bindings. */
    xmlSchemaPSVIIDCNodePtr node, parNode = NULL; /* node-table entries. */
    xmlSchemaPSVIIDCKeyPtr key, parKey; /* keys of in a key-sequence. */
    xmlSchemaIDCAugPtr aidc;
    int i, j, k, ret = 0, oldNum, newDupls;
    int duplTop;

    /*
    * The node table has the following sections:
    *
    *  O --> old node-table entries (first)
    *  O 
    *  + --> new node-table entries
    *  + 
    *  % --> new duplicate node-table entries    
    *  % 
    *  # --> old duplicate node-table entries    
    *  # (last)
    *
    */
    bind = vctxt->inode->idcTable;        
    if (bind == NULL) {
	/* Fine, no table, no bubbles. */
	return (0);
    }
    
    parTable = &(vctxt->elemInfos[vctxt->depth -1]->idcTable);
    /*
    * Walk all bindings; create new or add to existing bindings.
    * Remove duplicate key-sequences.
    */
start_binding:
    while (bind != NULL) {
	/*
	* Skip keyref IDCs.
	*/
	if (bind->definition->type == XML_SCHEMA_TYPE_IDC_KEYREF) {
	    bind = bind->next;
	    continue;
	}
	/*
	* Check if the key/unique IDC table needs to be bubbled.
	*/
	aidc = vctxt->aidcs;
	do {
	    if (aidc->def == bind->definition) {
		if ((aidc->bubbleDepth == -1) || 
		    (aidc->bubbleDepth >= vctxt->depth)) {
		    bind = bind->next;
		    goto start_binding;
		}
		break;
	    }
	    aidc = aidc->next;
	} while (aidc != NULL);

	if (parTable != NULL)
	    parBind = *parTable;
	while (parBind != NULL) {
	    /*
	    * Search a matching parent binding for the
	    * IDC definition.
	    */
	    if (parBind->definition == bind->definition) {

		/*
		* Compare every node-table entry of the child node, 
		* i.e. the key-sequence within, ...
		*/
		oldNum = parBind->nbNodes; /* Skip newly added items. */
		duplTop = oldNum + parBind->nbDupls;
		newDupls = 0;

		for (i = 0; i < bind->nbNodes; i++) {
		    node = bind->nodeTable[i];
		    if (node == NULL)
			continue;
		    /*
		    * ...with every key-sequence of the parent node, already
		    * evaluated to be a duplicate key-sequence.
		    */
		    if (parBind->nbDupls != 0) {
			j = bind->nbNodes + newDupls; 
			while (j < duplTop) {
			    parNode = parBind->nodeTable[j];
			    for (k = 0; k < bind->definition->nbFields; k++) {
				key = node->keys[k];
				parKey = parNode->keys[k];
				ret = xmlSchemaAreValuesEqual(key->val,
				    parKey->val);
				if (ret == -1) {
				    /* TODO: Internal error */
				    return(-1);
				} else if (ret == 0)
				    break;

			    }
			    if (ret == 1)
				/* Duplicate found. */
				break;
			    j++;
			}
			if (j != duplTop) {
			    /* Duplicate found. */
			    continue;
			}
		    }		    
		    /*
		    * ... and with every key-sequence of the parent node.
		    */		    		    
		    j = 0;
		    while (j < oldNum) {
			parNode = parBind->nodeTable[j];
			/*
			* Compare key by key. 
			*/
			for (k = 0; k < parBind->definition->nbFields; k++) {
			    key = node->keys[k];
			    parKey = parNode->keys[k];			

			    ret = xmlSchemaAreValuesEqual(key->val,
				parKey->val);
			    if (ret == -1) {
				/* TODO: Internal error */
			    } else if (ret == 0)
				break;

			}
			if (ret == 1)
			    /*
			    * The key-sequences are equal.
			    */
			    break;
			j++;
		    }
		    if (j != oldNum) {
			/*
			* Handle duplicates.
			*/
			newDupls++;
			oldNum--;
			parBind->nbNodes--;
			/*
			* Move last old item to pos of duplicate.
			*/
			parBind->nodeTable[j] = 
			    parBind->nodeTable[oldNum];
			
			if (parBind->nbNodes != oldNum) {
			    /*
			    * If new items exist, move last new item to 
			    * last of old items.
			    */
			    parBind->nodeTable[oldNum] = 
				parBind->nodeTable[parBind->nbNodes];
			}
			/*
			* Move duplicate to last pos of new/old items.
			*/
			parBind->nodeTable[parBind->nbNodes] = parNode;			
			
		    } else {
			/*
			* Add the node-table entry (node and key-sequence) of 
			* the child node to the node table of the parent node.
			*/
			if (parBind->nodeTable == NULL) {			
			    parBind->nodeTable = (xmlSchemaPSVIIDCNodePtr *) 
				xmlMalloc(10 * sizeof(xmlSchemaPSVIIDCNodePtr));
			    if (parBind->nodeTable == NULL) {
				xmlSchemaVErrMemory(NULL, 
				    "allocating IDC list of node-table items", NULL);
				return(-1);
			    }
			    parBind->sizeNodes = 1;
			} else if (duplTop >= parBind->sizeNodes) {
			    parBind->sizeNodes *= 2;
			    parBind->nodeTable = (xmlSchemaPSVIIDCNodePtr *) 
				xmlRealloc(parBind->nodeTable, parBind->sizeNodes * 
				sizeof(xmlSchemaPSVIIDCNodePtr));
			    if (parBind->nodeTable == NULL) {
				xmlSchemaVErrMemory(NULL, 
				    "re-allocating IDC list of node-table items", NULL);
				return(-1);
			    }
			}
			
			/*
			* Move first old duplicate to last position
			* of old duplicates +1.
			*/
			if (parBind->nbDupls != 0) {
			    parBind->nodeTable[duplTop] =
				parBind->nodeTable[parBind->nbNodes + newDupls];
			}
			/*
			* Move first new duplicate to last position of
			* new duplicates +1.
			*/
			if (newDupls != 0) {
			    parBind->nodeTable[parBind->nbNodes + newDupls] =
				parBind->nodeTable[parBind->nbNodes];
			}
			/*
			* Append the new node-table entry to the 'new node-table
			* entries' section.
			*/
			parBind->nodeTable[parBind->nbNodes] = node;
			parBind->nbNodes++;
			duplTop++;
		    }
		}
		parBind->nbDupls += newDupls;
		break;
	    }
	    if (parBind->next == NULL)
		lastParBind = parBind;
	    parBind = parBind->next;
	}
	if ((parBind == NULL) && (bind->nbNodes != 0)) {
	    /*
	    * No binding for the IDC was found: create a new one and
	    * copy all node-tables.
	    */
	    parBind = xmlSchemaIDCNewBinding(bind->definition);
	    if (parBind == NULL)
		return(-1);

	    parBind->nodeTable = (xmlSchemaPSVIIDCNodePtr *) 
		xmlMalloc(bind->nbNodes * sizeof(xmlSchemaPSVIIDCNodePtr));
	    if (parBind->nodeTable == NULL) {
		xmlSchemaVErrMemory(NULL, 
		    "allocating an array of IDC node-table items", NULL);
		xmlSchemaIDCFreeBinding(parBind);
		return(-1);
	    }
	    parBind->sizeNodes = bind->nbNodes;
	    parBind->nbNodes = bind->nbNodes;
	    memcpy(parBind->nodeTable, bind->nodeTable,
		bind->nbNodes * sizeof(xmlSchemaPSVIIDCNodePtr));
	    if (*parTable == NULL)
		*parTable = parBind;
	    else
		lastParBind->next = parBind;
	}
	bind = bind->next;
    }  
    return (0);
}

/**
 * xmlSchemaCheckCVCIDCKeyRef:
 * @vctxt: the WXS validation context
 * @elemDecl: the element declaration
 *
 * Check the cvc-idc-keyref constraints.
 */
static int
xmlSchemaCheckCVCIDCKeyRef(xmlSchemaValidCtxtPtr vctxt)
{
    xmlSchemaPSVIIDCBindingPtr refbind, bind;

    refbind = vctxt->inode->idcTable;
    /*
    * Find a keyref.
    */
    while (refbind != NULL) {
	if (refbind->definition->type == XML_SCHEMA_TYPE_IDC_KEYREF) {
	    int i, j, k, res;
	    xmlSchemaPSVIIDCKeyPtr *refKeys, *keys;
	    xmlSchemaPSVIIDCKeyPtr refKey, key;

	    /*
	    * Find the referred key/unique.
	    */
	    bind = vctxt->inode->idcTable;
	    do {
		if ((xmlSchemaIDCPtr) refbind->definition->ref->item == 
		    bind->definition)
		    break;
		bind = bind->next;
	    } while (bind != NULL);

	    /*
	    * Search for a matching key-sequences.
	    */
	    for (i = 0; i < refbind->nbNodes; i++) {
		res = 0;
		if (bind != NULL) {		    
		    refKeys = refbind->nodeTable[i]->keys;
		    for (j = 0; j < bind->nbNodes; j++) {
			keys = bind->nodeTable[j]->keys;
			for (k = 0; k < bind->definition->nbFields; k++) {
			    refKey = refKeys[k];
			    key = keys[k];
			    res = xmlSchemaAreValuesEqual(key->val,
				refKey->val);
			    if (res == 0)
				break;
			    else if (res == -1) {
				return (-1);
			    }
			}
			if (res == 1) {
			    /*
			    * Match found.
			    */
			    break;
			}
		    }
		}
		if (res == 0) {
		    xmlChar *str = NULL, *strB = NULL;
		    /* TODO: Report the key-sequence. */
		    xmlSchemaCustomErr((xmlSchemaAbstractCtxtPtr) vctxt,
			XML_SCHEMAV_CVC_IDC, NULL,
			(xmlSchemaTypePtr) refbind->definition,
			"No match found for key-sequence %s of key "
			"reference '%s'",
			xmlSchemaFormatIDCKeySequence(vctxt, &str,
			    refbind->nodeTable[i]->keys,
			    refbind->definition->nbFields),
			xmlSchemaFormatQName(&strB,
			    refbind->definition->targetNamespace,
			    refbind->definition->name));
		    FREE_AND_NULL(str);
		    FREE_AND_NULL(strB);
		}
	    }
	}
	refbind = refbind->next;
    }
    return (0);
}

/************************************************************************
 * 									*
 * 			XML Reader validation code                      *
 * 									*
 ************************************************************************/

static xmlSchemaAttrInfoPtr
xmlSchemaGetFreshAttrInfo(xmlSchemaValidCtxtPtr vctxt)
{
    xmlSchemaAttrInfoPtr iattr;
    /*
    * Grow/create list of attribute infos.
    */
    if (vctxt->attrInfos == NULL) {
	vctxt->attrInfos = (xmlSchemaAttrInfoPtr *)
	    xmlMalloc(sizeof(xmlSchemaAttrInfoPtr));
	vctxt->sizeAttrInfos = 1;
	if (vctxt->attrInfos == NULL) {
	    xmlSchemaVErrMemory(vctxt,
		"allocating attribute info list", NULL);
	    return (NULL);
	}
    } else if (vctxt->sizeAttrInfos <= vctxt->nbAttrInfos) {
	vctxt->sizeAttrInfos++;
	vctxt->attrInfos = (xmlSchemaAttrInfoPtr *)
	    xmlRealloc(vctxt->attrInfos,
		vctxt->sizeAttrInfos * sizeof(xmlSchemaAttrInfoPtr));
	if (vctxt->attrInfos == NULL) {
	    xmlSchemaVErrMemory(vctxt,
		"re-allocating attribute info list", NULL);
	    return (NULL);
	}
    } else {
	iattr = vctxt->attrInfos[vctxt->nbAttrInfos++];
	if (iattr->localName != NULL) {
	    VERROR_INT("xmlSchemaGetFreshAttrInfo",
		"attr info not cleared");
	    return (NULL);
	}
	iattr->nodeType = XML_ATTRIBUTE_NODE;
	return (iattr);
    }
    /*
    * Create an attribute info.
    */
    iattr = (xmlSchemaAttrInfoPtr)
	xmlMalloc(sizeof(xmlSchemaAttrInfo));
    if (iattr == NULL) {
	xmlSchemaVErrMemory(vctxt, "creating new attribute info", NULL);
	return (NULL);
    }
    memset(iattr, 0, sizeof(xmlSchemaAttrInfo));
    iattr->nodeType = XML_ATTRIBUTE_NODE;
    vctxt->attrInfos[vctxt->nbAttrInfos++] = iattr;

    return (iattr);
}

static int
xmlSchemaValidatorPushAttribute(xmlSchemaValidCtxtPtr vctxt,
			xmlNodePtr attrNode,
			const xmlChar *localName,
			const xmlChar *nsName,
			int ownedNames,
			xmlChar *value,
			int ownedValue)
{
    xmlSchemaAttrInfoPtr attr;

    attr = xmlSchemaGetFreshAttrInfo(vctxt);
    if (attr == NULL) {
	VERROR_INT("xmlSchemaPushAttribute",
	    "calling xmlSchemaGetFreshAttrInfo()");
	return (-1);
    }
    attr->node = attrNode;
    attr->state = XML_SCHEMAS_ATTR_UNKNOWN;
    attr->localName = localName;
    attr->nsName = nsName;
    if (ownedNames)
	attr->flags |= XML_SCHEMA_NODE_INFO_FLAG_OWNED_NAMES;
    /*
    * Evaluate if it's an XSI attribute.
    */
    if (nsName != NULL) {
	if (xmlStrEqual(localName, BAD_CAST "nil")) {
	    if (xmlStrEqual(attr->nsName, xmlSchemaInstanceNs)) {
		attr->metaType = XML_SCHEMA_ATTR_INFO_META_XSI_NIL;		
	    }
	} else if (xmlStrEqual(localName, BAD_CAST "type")) {
	    if (xmlStrEqual(attr->nsName, xmlSchemaInstanceNs)) {
		attr->metaType = XML_SCHEMA_ATTR_INFO_META_XSI_TYPE;
	    }
	} else if (xmlStrEqual(localName, BAD_CAST "schemaLocation")) {
	    if (xmlStrEqual(attr->nsName, xmlSchemaInstanceNs)) {
		attr->metaType = XML_SCHEMA_ATTR_INFO_META_XSI_SCHEMA_LOC;
	    }
	} else if (xmlStrEqual(localName, BAD_CAST "noNamespaceSchemaLocation")) {
	    if (xmlStrEqual(attr->nsName, xmlSchemaInstanceNs)) {
		attr->metaType = XML_SCHEMA_ATTR_INFO_META_XSI_NO_NS_SCHEMA_LOC;
	    }
	} else if (xmlStrEqual(attr->nsName, xmlNamespaceNs)) {
	    attr->metaType = XML_SCHEMA_ATTR_INFO_META_XMLNS;
	}
    }
    attr->value = value;
    if (ownedValue)
	attr->flags |= XML_SCHEMA_NODE_INFO_FLAG_OWNED_VALUES;
    if (attr->metaType != 0)
	attr->state = XML_SCHEMAS_ATTR_META;
    return (0);
}

static void
xmlSchemaClearElemInfo(xmlSchemaNodeInfoPtr ielem)
{
    if (ielem->flags & XML_SCHEMA_NODE_INFO_FLAG_OWNED_NAMES) {
	FREE_AND_NULL(ielem->localName);
	FREE_AND_NULL(ielem->nsName);
    } else {
	ielem->localName = NULL;
	ielem->nsName = NULL;
    }
    if (ielem->flags & XML_SCHEMA_NODE_INFO_FLAG_OWNED_VALUES) {
	FREE_AND_NULL(ielem->value);
    } else {
	ielem->value = NULL;
    }
    if (ielem->val != NULL) {
	xmlSchemaFreeValue(ielem->val);
	ielem->val = NULL;
    }
    if (ielem->idcMatchers != NULL) {
	xmlSchemaIDCFreeMatcherList(ielem->idcMatchers);
	ielem->idcMatchers = NULL;
    }
    if (ielem->idcTable != NULL) {
	xmlSchemaIDCFreeIDCTable(ielem->idcTable);
	ielem->idcTable = NULL;
    }
    if (ielem->regexCtxt != NULL) {
	xmlRegFreeExecCtxt(ielem->regexCtxt);
	ielem->regexCtxt = NULL;
    }
    if (ielem->nsBindings != NULL) {
	xmlFree((xmlChar **)ielem->nsBindings);
	ielem->nsBindings = NULL;
	ielem->nbNsBindings = 0;
	ielem->sizeNsBindings = 0;
    }
}

/**
 * xmlSchemaGetFreshElemInfo:
 * @vctxt: the schema validation context
 *
 * Creates/reuses and initializes the element info item for
 * the currect tree depth.
 *
 * Returns the element info item or NULL on API or internal errors.
 */
static xmlSchemaNodeInfoPtr
xmlSchemaGetFreshElemInfo(xmlSchemaValidCtxtPtr vctxt)
{
    xmlSchemaNodeInfoPtr info = NULL;

    if (vctxt->depth > vctxt->sizeElemInfos) {
	VERROR_INT("xmlSchemaGetFreshElemInfo",
	    "inconsistent depth encountered");
	return (NULL);
    }
    if (vctxt->elemInfos == NULL) {
	vctxt->elemInfos = (xmlSchemaNodeInfoPtr *)
	    xmlMalloc(10 * sizeof(xmlSchemaNodeInfoPtr));
	if (vctxt->elemInfos == NULL) {
	    xmlSchemaVErrMemory(vctxt,
		"allocating the element info array", NULL);
	    return (NULL);
	}
	memset(vctxt->elemInfos, 0, 10 * sizeof(xmlSchemaNodeInfoPtr));
	vctxt->sizeElemInfos = 10;
    } else if (vctxt->sizeElemInfos <= vctxt->depth) {
	int i = vctxt->sizeElemInfos;

	vctxt->sizeElemInfos *= 2;
	vctxt->elemInfos = (xmlSchemaNodeInfoPtr *)
	    xmlRealloc(vctxt->elemInfos, vctxt->sizeElemInfos *
	    sizeof(xmlSchemaNodeInfoPtr));
	if (vctxt->elemInfos == NULL) {
	    xmlSchemaVErrMemory(vctxt,
		"re-allocating the element info array", NULL);
	    return (NULL);
	}
	/*
	* We need the new memory to be NULLed.
	* TODO: Use memset instead?
	*/
	for (; i < vctxt->sizeElemInfos; i++)
	    vctxt->elemInfos[i] = NULL;
    } else
	info = vctxt->elemInfos[vctxt->depth];

    if (info == NULL) {
	info = (xmlSchemaNodeInfoPtr)
	    xmlMalloc(sizeof(xmlSchemaNodeInfo));
	if (info == NULL) {
	    xmlSchemaVErrMemory(vctxt,
		"allocating an element info", NULL);
	    return (NULL);
	}
	vctxt->elemInfos[vctxt->depth] = info;
    } else {
	if (info->localName != NULL) {
	    VERROR_INT("xmlSchemaGetFreshElemInfo",
		"elem info has not been cleared");
	    return (NULL);
	}
    }
    memset(info, 0, sizeof(xmlSchemaNodeInfo));
    info->nodeType = XML_ELEMENT_NODE;
    info->depth = vctxt->depth;

    return (info);
}

#define ACTIVATE_ATTRIBUTE(item) vctxt->inode = (xmlSchemaNodeInfoPtr) item;
#define ACTIVATE_ELEM vctxt->inode = vctxt->elemInfos[vctxt->depth];
#define ACTIVATE_PARENT_ELEM vctxt->inode = vctxt->elemInfos[vctxt->depth -1];

static int
xmlSchemaValidateFacets(xmlSchemaAbstractCtxtPtr actxt,
			xmlNodePtr node,
			xmlSchemaTypePtr type,
			xmlSchemaValType valType,
			const xmlChar * value,
			xmlSchemaValPtr val,
			unsigned long length,
			int fireErrors)
{
    int ret, error = 0;

    xmlSchemaTypePtr tmpType;
    xmlSchemaFacetLinkPtr facetLink;
    xmlSchemaFacetPtr facet;
    unsigned long len = 0;
    xmlSchemaWhitespaceValueType ws;

    /*
    * In Libxml2, derived built-in types have currently no explicit facets.
    */
    if (type->type == XML_SCHEMA_TYPE_BASIC)
	return (0);

    /*
    * NOTE: Do not jump away, if the facetSet of the given type is
    * empty: until now, "pattern" and "enumeration" facets of the
    * *base types* need to be checked as well.
    */
    if (type->facetSet == NULL)
	goto pattern_and_enum;

    if (! VARIETY_ATOMIC(type)) {
	if (VARIETY_LIST(type))
	    goto variety_list;
	else
	    goto pattern_and_enum;
    }
    /*
    * Whitespace handling is only of importance for string-based
    * types.
    */
    tmpType = xmlSchemaGetPrimitiveType(type);
    if ((tmpType->builtInType == XML_SCHEMAS_STRING) ||
	IS_ANY_SIMPLE_TYPE(tmpType)) {
	ws = xmlSchemaGetWhiteSpaceFacetValue(type);
    } else
	ws = XML_SCHEMA_WHITESPACE_COLLAPSE;
    /*
    * If the value was not computed (for string or
    * anySimpleType based types), then use the provided
    * type.
    */
    if (val == NULL)
	valType = valType;
    else
	valType = xmlSchemaGetValType(val);
    
    ret = 0;
    for (facetLink = type->facetSet; facetLink != NULL;
	facetLink = facetLink->next) {
	/*
	* Skip the pattern "whiteSpace": it is used to
	* format the character content beforehand.
	*/
	switch (facetLink->facet->type) {
	    case XML_SCHEMA_FACET_WHITESPACE:
	    case XML_SCHEMA_FACET_PATTERN:
	    case XML_SCHEMA_FACET_ENUMERATION:
		continue;
	    case XML_SCHEMA_FACET_LENGTH:
	    case XML_SCHEMA_FACET_MINLENGTH:
	    case XML_SCHEMA_FACET_MAXLENGTH:
		ret = xmlSchemaValidateLengthFacetWhtsp(facetLink->facet,
		    valType, value, val, &len, ws);
		break;
	    default:
		ret = xmlSchemaValidateFacetWhtsp(facetLink->facet, ws,
		    valType, value, val, ws);
		break;
	}
	if (ret < 0) {
	    AERROR_INT("xmlSchemaValidateFacets",
		"validating against a atomic type facet");
	    return (-1);
	} else if (ret > 0) {
	    if (fireErrors)
		xmlSchemaFacetErr(actxt, ret, node,
		value, len, type, facetLink->facet, NULL, NULL, NULL);
	    else
		return (ret);
	    if (error == 0)
		error = ret;
	}
	ret = 0;
    }

variety_list:
    if (! VARIETY_LIST(type))
	goto pattern_and_enum;
    /*
    * "length", "minLength" and "maxLength" of list types.
    */
    ret = 0;
    for (facetLink = type->facetSet; facetLink != NULL;
	facetLink = facetLink->next) {
	
	switch (facetLink->facet->type) {
	    case XML_SCHEMA_FACET_LENGTH:
	    case XML_SCHEMA_FACET_MINLENGTH:
	    case XML_SCHEMA_FACET_MAXLENGTH:		    
		ret = xmlSchemaValidateListSimpleTypeFacet(facetLink->facet,
		    value, length, NULL);
		break;
	    default:
		continue;
	}
	if (ret < 0) {
	    AERROR_INT("xmlSchemaValidateFacets",
		"validating against a list type facet");
	    return (-1);
	} else if (ret > 0) {
	    if (fireErrors)		
		xmlSchemaFacetErr(actxt, ret, node,
		value, length, type, facetLink->facet, NULL, NULL, NULL);
	    else
		return (ret);
	    if (error == 0)
		error = ret;
	}
	ret = 0;
    }

pattern_and_enum:
    if (error >= 0) {
	int found = 0;
	/*
	* Process enumerations. Facet values are in the value space
	* of the defining type's base type. This seems to be a bug in the
	* XML Schema 1.0 spec. Use the whitespace type of the base type.
	* Only the first set of enumerations in the ancestor-or-self axis
	* is used for validation.
	*/
	ret = 0;
	tmpType = type;
	do {
	    for (facet = tmpType->facets; facet != NULL; facet = facet->next) {
		if (facet->type != XML_SCHEMA_FACET_ENUMERATION)
		    continue;
		found = 1;
		ret = xmlSchemaAreValuesEqual(facet->val, val);
		if (ret == 1)
		    break;
		else if (ret < 0) {
		    AERROR_INT("xmlSchemaValidateFacets",
			"validating against an enumeration facet");
		    return (-1);
		}
	    }
	    if (ret != 0)
		break;
	    tmpType = tmpType->baseType;
	} while ((tmpType != NULL) &&
	    (tmpType->type != XML_SCHEMA_TYPE_BASIC));
	if (found && (ret == 0)) {
	    ret = XML_SCHEMAV_CVC_ENUMERATION_VALID;
	    if (fireErrors) {
		xmlSchemaFacetErr(actxt, ret, node,
		    value, 0, type, NULL, NULL, NULL, NULL);
	    } else
		return (ret);
	    if (error == 0)
		error = ret;
	}
    }

    if (error >= 0) {
	int found;
	/*
	* Process patters. Pattern facets are ORed at type level
	* and ANDed if derived. Walk the base type axis.
	*/
	tmpType = type;
	facet = NULL;
	do {
	    found = 0;
	    for (facetLink = tmpType->facetSet; facetLink != NULL;
		facetLink = facetLink->next) {
		if (facetLink->facet->type != XML_SCHEMA_FACET_PATTERN)
		    continue;
		found = 1;
		/* 
		* NOTE that for patterns, @value needs to be the
		* normalized vaule.
		*/
		ret = xmlRegexpExec(facetLink->facet->regexp, value);
		if (ret == 1)
		    break;
		else if (ret < 0) {
		    AERROR_INT("xmlSchemaValidateFacets",
			"validating against a pattern facet");
		    return (-1);
		} else {
		    /* 
		    * Save the last non-validating facet.
		    */
		    facet = facetLink->facet;
		}
	    }
	    if (found && (ret != 1)) {
		ret = XML_SCHEMAV_CVC_PATTERN_VALID;
		if (fireErrors) {
		    xmlSchemaFacetErr(actxt, ret, node,
			value, 0, type, facet, NULL, NULL, NULL);
		} else
		    return (ret);
		if (error == 0)
		    error = ret;
		break;
	    }
	    tmpType = tmpType->baseType;
	} while ((tmpType != NULL) && (tmpType->type != XML_SCHEMA_TYPE_BASIC));
    }

    return (error);
}
 
static xmlChar *
xmlSchemaNormalizeValue(xmlSchemaTypePtr type,
			const xmlChar *value)
{
    switch (xmlSchemaGetWhiteSpaceFacetValue(type)) {	
	case XML_SCHEMA_WHITESPACE_COLLAPSE:
	    return (xmlSchemaCollapseString(value));
	case XML_SCHEMA_WHITESPACE_REPLACE:
	    return (xmlSchemaWhiteSpaceReplace(value));
	default:
	    return (NULL);
    }
}

static int
xmlSchemaValidateQName(xmlSchemaValidCtxtPtr vctxt,
		       const xmlChar *value,
		       xmlSchemaValPtr *val,
		       int valNeeded)
{
    int ret;
    const xmlChar *nsName;
    xmlChar *local, *prefix = NULL;
    
    ret = xmlValidateQName(value, 1);
    if (ret != 0) {
	if (ret == -1) {
	    VERROR_INT("xmlSchemaValidateQName",
		"calling xmlValidateQName()");
	    return (-1);
	}
	return( XML_SCHEMAV_CVC_DATATYPE_VALID_1_2_1);
    }
    /*
    * NOTE: xmlSplitQName2 will always return a duplicated
    * strings.
    */
    local = xmlSplitQName2(value, &prefix);
    if (local == NULL)
	local = xmlStrdup(value);
    /*
    * OPTIMIZE TODO: Use flags for:
    *  - is there any namespace binding?
    *  - is there a default namespace?
    */
    nsName = xmlSchemaLookupNamespace(vctxt, prefix);
    
    if (prefix != NULL) {
	xmlFree(prefix);
	/*
	* A namespace must be found if the prefix is
	* NOT NULL.
	*/
	if (nsName == NULL) {
	    ret = XML_SCHEMAV_CVC_DATATYPE_VALID_1_2_1;
	    xmlSchemaCustomErr((xmlSchemaAbstractCtxtPtr) vctxt, ret, NULL,
		xmlSchemaGetBuiltInType(XML_SCHEMAS_QNAME),
		"The QName value '%s' has no "
		"corresponding namespace declaration in "
		"scope", value, NULL);
	    if (local != NULL)
		xmlFree(local);
	    return (ret);
	}
    }
    if (valNeeded && val) {
	if (nsName != NULL)
	    *val = xmlSchemaNewQNameValue(
		BAD_CAST xmlStrdup(nsName), BAD_CAST local);
	else
	    *val = xmlSchemaNewQNameValue(NULL,
		BAD_CAST local);
    } else
	xmlFree(local);
    return (0);
}

/*
* cvc-simple-type
*/
static int
xmlSchemaVCheckCVCSimpleType(xmlSchemaAbstractCtxtPtr actxt,
			     xmlNodePtr node,
			     xmlSchemaTypePtr type,
			     const xmlChar *value,
			     xmlSchemaValPtr *retVal,
			     int fireErrors,
			     int normalize,
			     int isNormalized)
{
    int ret = 0, valNeeded = (retVal) ? 1 : 0;
    xmlSchemaValPtr val = NULL;
    xmlSchemaWhitespaceValueType ws;
    xmlChar *normValue = NULL;

#define NORMALIZE(atype) \
    if ((! isNormalized) && \
    (normalize || (type->flags & XML_SCHEMAS_TYPE_NORMVALUENEEDED))) { \
	normValue = xmlSchemaNormalizeValue(atype, value); \
	if (normValue != NULL) \
	    value = normValue; \
	isNormalized = 1; \
    }
    
    if ((retVal != NULL) && (*retVal != NULL)) {
	xmlSchemaFreeValue(*retVal);
	*retVal = NULL;
    }
    /*
    * 3.14.4 Simple Type Definition Validation Rules
    * Validation Rule: String Valid
    */
    /*
    * 1 It is schema-valid with respect to that definition as defined
    * by Datatype Valid in [XML Schemas: Datatypes].
    */
    /*
    * 2.1 If The definition is ENTITY or is validly derived from ENTITY given
    * the empty set, as defined in Type Derivation OK (Simple) (§3.14.6), then
    * the string must be a ·declared entity name·.
    */
    /*
    * 2.2 If The definition is ENTITIES or is validly derived from ENTITIES
    * given the empty set, as defined in Type Derivation OK (Simple) (§3.14.6),
    * then every whitespace-delimited substring of the string must be a ·declared
    * entity name·.
    */
    /*
    * 2.3 otherwise no further condition applies.
    */
    if ((! valNeeded) && (type->flags & XML_SCHEMAS_TYPE_FACETSNEEDVALUE))
	valNeeded = 1;
    if (value == NULL)
	value = BAD_CAST "";
    if (IS_ANY_SIMPLE_TYPE(type) || VARIETY_ATOMIC(type)) {
	xmlSchemaTypePtr biType; /* The built-in type. */
	/*
	* SPEC (1.2.1) "if {variety} is ·atomic· then the string must ·match·
	* a literal in the ·lexical space· of {base type definition}"
	*/
	/*
	* Whitespace-normalize.
	*/
	NORMALIZE(type);
	if (type->type != XML_SCHEMA_TYPE_BASIC) {
	    /*
	    * Get the built-in type.
	    */
	    biType = type->baseType;
	    while ((biType != NULL) &&
		(biType->type != XML_SCHEMA_TYPE_BASIC))
		biType = biType->baseType;

	    if (biType == NULL) {
		AERROR_INT("xmlSchemaVCheckCVCSimpleType",
		    "could not get the built-in type");
		goto internal_error;
	    }
	} else
	    biType = type;
	/*
	* NOTATIONs need to be processed here, since they need
	* to lookup in the hashtable of NOTATION declarations of the schema.
	*/
	if (actxt->type == XML_SCHEMA_CTXT_VALIDATOR) {	    
	    switch (biType->builtInType) {		
		case XML_SCHEMAS_NOTATION:		    
		    ret = xmlSchemaValidateNotation(
			(xmlSchemaValidCtxtPtr) actxt,
			((xmlSchemaValidCtxtPtr) actxt)->schema,
			NULL, value, &val, valNeeded);
		    break;
		case XML_SCHEMAS_QNAME:
		    ret = xmlSchemaValidateQName((xmlSchemaValidCtxtPtr) actxt,
			value, &val, valNeeded);
		    break;
		default:
		    ws = xmlSchemaGetWhiteSpaceFacetValue(type);
		    if (valNeeded)
			ret = xmlSchemaValPredefTypeNodeNoNorm(biType,
			    value, &val, NULL);
		    else
			ret = xmlSchemaValPredefTypeNodeNoNorm(biType,
			    value, NULL, NULL);
		    break;
	    }
	} else if (actxt->type == XML_SCHEMA_CTXT_PARSER) {	    
	    switch (biType->builtInType) {		    
		case XML_SCHEMAS_NOTATION:
		    ret = xmlSchemaValidateNotation(NULL,
			((xmlSchemaParserCtxtPtr) actxt)->schema, node,
			value, &val, valNeeded);
		    break;
		default:
		    ws = xmlSchemaGetWhiteSpaceFacetValue(type);
		    if (valNeeded)
			ret = xmlSchemaValPredefTypeNodeNoNorm(biType,
			    value, &val, node);
		    else
			ret = xmlSchemaValPredefTypeNodeNoNorm(biType,
			    value, NULL, node);
		    break;
	    }	   
	} else {
	    /*
	    * Validation via a public API is not implemented yet.
	    */
	    TODO
	    goto internal_error;
	}
	if (ret != 0) {
	    if (ret < 0) {
		AERROR_INT("xmlSchemaVCheckCVCSimpleType",
		    "validating against a built-in type");
		goto internal_error;
	    }
	    if (VARIETY_LIST(type))
		ret = XML_SCHEMAV_CVC_DATATYPE_VALID_1_2_2;
	    else
		ret = XML_SCHEMAV_CVC_DATATYPE_VALID_1_2_1;	    
	}
	if ((ret == 0) && (type->flags & XML_SCHEMAS_TYPE_HAS_FACETS)) {
	    /*
	    * Check facets.
	    */
	    ret = xmlSchemaValidateFacets(actxt, node, type,
		(xmlSchemaValType) biType->builtInType, value, val,
		0, fireErrors);
	    if (ret != 0) {
		if (ret < 0) {
		    AERROR_INT("xmlSchemaVCheckCVCSimpleType",
			"validating facets of atomic simple type");
		    goto internal_error;
		}
		if (VARIETY_LIST(type)) 
		    ret = XML_SCHEMAV_CVC_DATATYPE_VALID_1_2_2;
		else
		    ret = XML_SCHEMAV_CVC_DATATYPE_VALID_1_2_1;		
	    }
	}
	if (fireErrors && (ret > 0))
	    xmlSchemaSimpleTypeErr(actxt, ret, node, value, type, 1);
    } else if (VARIETY_LIST(type)) {

	xmlSchemaTypePtr itemType;
	const xmlChar *cur, *end;
	xmlChar *tmpValue = NULL;
	unsigned long len = 0;
	xmlSchemaValPtr prevVal = NULL, curVal = NULL;
	/* 1.2.2 if {variety} is ·list· then the string must be a sequence
	* of white space separated tokens, each of which ·match·es a literal
	* in the ·lexical space· of {item type definition}
	*/
	/*
	* Note that XML_SCHEMAS_TYPE_NORMVALUENEEDED will be set if
	* the list type has an enum or pattern facet.
	*/
	NORMALIZE(type);
	/*
	* VAL TODO: Optimize validation of empty values.
	* VAL TODO: We do not have computed values for lists.
	*/
	itemType = GET_LIST_ITEM_TYPE(type);	
	cur = value;
	do {
	    while (IS_BLANK_CH(*cur))
		cur++;
	    end = cur;
	    while ((*end != 0) && (!(IS_BLANK_CH(*end))))
		end++;
	    if (end == cur)
		break;
	    tmpValue = xmlStrndup(cur, end - cur);
	    len++;

	    if (valNeeded)
		ret = xmlSchemaVCheckCVCSimpleType(actxt, node, itemType,
		    tmpValue, &curVal, fireErrors, 0, 1);
	    else
		ret = xmlSchemaVCheckCVCSimpleType(actxt, node, itemType,
		    tmpValue, NULL, fireErrors, 0, 1);
	    FREE_AND_NULL(tmpValue);
	    if (curVal != NULL) {
		/*
		* Add to list of computed values.
		*/
		if (val == NULL)
		    val = curVal;
		else
		    xmlSchemaValueAppend(prevVal, curVal);
		prevVal = curVal;
		curVal = NULL;
	    }
	    if (ret != 0) {
		if (ret < 0) {
		    AERROR_INT("xmlSchemaVCheckCVCSimpleType",
			"validating an item of list simple type");
		    goto internal_error;
		}
		ret = XML_SCHEMAV_CVC_DATATYPE_VALID_1_2_2;
		break;
	    }	    
	    cur = end;
	} while (*cur != 0);
	FREE_AND_NULL(tmpValue);
	if ((ret == 0) && (type->flags & XML_SCHEMAS_TYPE_HAS_FACETS)) {
	    /*
	    * Apply facets (pattern, enumeration).
	    */
	    ret = xmlSchemaValidateFacets(actxt, node, type,
		XML_SCHEMAS_UNKNOWN, value, val,
		len, fireErrors);
	    if (ret != 0) {
		if (ret < 0) {
		    AERROR_INT("xmlSchemaVCheckCVCSimpleType",
			"validating facets of list simple type");
		    goto internal_error;
		}
		ret = XML_SCHEMAV_CVC_DATATYPE_VALID_1_2_2;
	    }
	}
	if (fireErrors && (ret > 0)) {
	    /* 
	    * Report the normalized value.
	    */
	    normalize = 1;
	    NORMALIZE(type);
	    xmlSchemaSimpleTypeErr(actxt, ret, node, value, type, 1);
	}
    } else if (VARIETY_UNION(type)) {
	xmlSchemaTypeLinkPtr memberLink;
	/*
	* TODO: For all datatypes ·derived· by ·union·  whiteSpace does
	* not apply directly; however, the normalization behavior of ·union·
	* types is controlled by the value of whiteSpace on that one of the
	* ·memberTypes· against which the ·union· is successfully validated.
	*
	* This means that the value is normalized by the first validating
	* member type, then the facets of the union type are applied. This
	* needs changing of the value!
	*/

	/*
	* 1.2.3 if {variety} is ·union· then the string must ·match· a
	* literal in the ·lexical space· of at least one member of
	* {member type definitions}
	*/
	memberLink = xmlSchemaGetUnionSimpleTypeMemberTypes(type);
	if (memberLink == NULL) {
	    AERROR_INT("xmlSchemaVCheckCVCSimpleType",
		"union simple type has no member types");
	    goto internal_error;
	}	
	/*
	* Always normalize union type values, since we currently
	* cannot store the whitespace information with the value
	* itself; otherwise a later value-comparison would be
	* not possible.
	*/
	while (memberLink != NULL) {
	    if (valNeeded) 
		ret = xmlSchemaVCheckCVCSimpleType(actxt, node,
		    memberLink->type, value, &val, 0, 1, 0);
	    else
		ret = xmlSchemaVCheckCVCSimpleType(actxt, node,
		    memberLink->type, value, NULL, 0, 1, 0);
	    if (ret <= 0)
		break;
	    memberLink = memberLink->next;
	}
	if (ret != 0) {
	    if (ret < 0) {
		AERROR_INT("xmlSchemaVCheckCVCSimpleType",
		    "validating members of union simple type");
		goto internal_error;
	    }
	    ret = XML_SCHEMAV_CVC_DATATYPE_VALID_1_2_3;
	}
	/*
	* Apply facets (pattern, enumeration).
	*/
	if ((ret == 0) && (type->flags & XML_SCHEMAS_TYPE_HAS_FACETS)) {
	    /*
	    * The normalization behavior of ·union· types is controlled by
	    * the value of whiteSpace on that one of the ·memberTypes·
	    * against which the ·union· is successfully validated.
	    */
	    NORMALIZE(memberLink->type);
	    ret = xmlSchemaValidateFacets(actxt, node, type,
		XML_SCHEMAS_UNKNOWN, value, val,
		0, fireErrors);
	    if (ret != 0) {
		if (ret < 0) {
		    AERROR_INT("xmlSchemaVCheckCVCSimpleType",
			"validating facets of union simple type");
		    goto internal_error;
		}
		ret = XML_SCHEMAV_CVC_DATATYPE_VALID_1_2_3;		
	    }
	}
	if (fireErrors && (ret > 0))
	    xmlSchemaSimpleTypeErr(actxt, ret, node, value, type, 1);
    }

    if (normValue != NULL)
	xmlFree(normValue);
    if (ret == 0) {
	if (retVal != NULL)
	    *retVal = val;
	else if (val != NULL)
	    xmlSchemaFreeValue(val);
    } else if (val != NULL)
	xmlSchemaFreeValue(val);
    return (ret);
internal_error:
    if (normValue != NULL)
	xmlFree(normValue);
    if (val != NULL)
	xmlSchemaFreeValue(val);
    return (-1);
}

static int
xmlSchemaVExpandQName(xmlSchemaValidCtxtPtr vctxt,
			   const xmlChar *value,
			   const xmlChar **nsName,
			   const xmlChar **localName)
{
    int ret = 0;

    if ((nsName == NULL) || (localName == NULL))
	return (-1);
    *nsName = NULL;
    *localName = NULL;

    ret = xmlValidateQName(value, 1);
    if (ret == -1)
	return (-1);
    if (ret > 0) {
	xmlSchemaSimpleTypeErr((xmlSchemaAbstractCtxtPtr) vctxt,
	    XML_SCHEMAV_CVC_DATATYPE_VALID_1_2_1, NULL,
	    value, xmlSchemaGetBuiltInType(XML_SCHEMAS_QNAME), 1);
	return (1);
    }
    {
	xmlChar *local = NULL;
	xmlChar *prefix;

	/*
	* NOTE: xmlSplitQName2 will return a duplicated
	* string.
	*/
	local = xmlSplitQName2(value, &prefix);
	VAL_CREATE_DICT;
	if (local == NULL)
	    *localName = xmlDictLookup(vctxt->dict, value, -1);
	else {
	    *localName = xmlDictLookup(vctxt->dict, local, -1);
	    xmlFree(local);
	}

	*nsName = xmlSchemaLookupNamespace(vctxt, prefix);

	if (prefix != NULL) {
	    xmlFree(prefix);
	    /*
	    * A namespace must be found if the prefix is NOT NULL.
	    */
	    if (*nsName == NULL) {
		xmlSchemaCustomErr((xmlSchemaAbstractCtxtPtr) vctxt,
		    XML_SCHEMAV_CVC_DATATYPE_VALID_1_2_1, NULL,
		    xmlSchemaGetBuiltInType(XML_SCHEMAS_QNAME),
		    "The QName value '%s' has no "
		    "corresponding namespace declaration in scope",
		    value, NULL);
		return (2);
	    }
	}
    }
    return (0);
}

static int
xmlSchemaProcessXSIType(xmlSchemaValidCtxtPtr vctxt,
			xmlSchemaAttrInfoPtr iattr,
			xmlSchemaTypePtr *localType,
			xmlSchemaElementPtr elemDecl)
{
    int ret = 0;
    /*
    * cvc-elt (3.3.4) : (4)
    * AND
    * Schema-Validity Assessment (Element) (cvc-assess-elt)
    *   (1.2.1.2.1) - (1.2.1.2.4)
    * Handle 'xsi:type'.
    */
    if (localType == NULL)
	return (-1);
    *localType = NULL;
    if (iattr == NULL)
	return (0);
    else {
	const xmlChar *nsName = NULL, *local = NULL;
	/*
	* TODO: We should report a *warning* that the type was overriden
	* by the instance.
	*/
	ACTIVATE_ATTRIBUTE(iattr);
	/*
	* (cvc-elt) (3.3.4) : (4.1)
	* (cvc-assess-elt) (1.2.1.2.2)
	*/
	ret = xmlSchemaVExpandQName(vctxt, iattr->value,
	    &nsName, &local);
	if (ret != 0) {
	    if (ret < 0) {
		VERROR_INT("xmlSchemaValidateElementByDeclaration",
		    "calling xmlSchemaQNameExpand() to validate the "
		    "attribute 'xsi:type'");
		goto internal_error;
	    }
	    goto exit;
	}
	/*
	* (cvc-elt) (3.3.4) : (4.2)
	* (cvc-assess-elt) (1.2.1.2.3)
	*/
	*localType = xmlSchemaGetType(vctxt->schema, local, nsName);
	if (*localType == NULL) {
	    xmlChar *str = NULL;

	    xmlSchemaCustomErr((xmlSchemaAbstractCtxtPtr) vctxt,
		XML_SCHEMAV_CVC_ELT_4_2, NULL,
		xmlSchemaGetBuiltInType(XML_SCHEMAS_QNAME),
		"The QName value '%s' of the xsi:type attribute does not "
		"resolve to a type definition",
		xmlSchemaFormatQName(&str, nsName, local), NULL);
	    FREE_AND_NULL(str);
	    ret = vctxt->err;
	    goto exit;
	}
	if (elemDecl != NULL) {
	    int set = 0;

	    /*
	    * SPEC cvc-elt (3.3.4) : (4.3) (Type Derivation OK)
	    * "The ·local type definition· must be validly
	    * derived from the {type definition} given the union of
	    * the {disallowed substitutions} and the {type definition}'s
	    * {prohibited substitutions}, as defined in
	    * Type Derivation OK (Complex) (§3.4.6)
	    * (if it is a complex type definition),
	    * or given {disallowed substitutions} as defined in Type
	    * Derivation OK (Simple) (§3.14.6) (if it is a simple type
	    * definition)."
	    *
	    * {disallowed substitutions}: the "block" on the element decl.
	    * {prohibited substitutions}: the "block" on the type def.
	    */
	    if ((elemDecl->flags & XML_SCHEMAS_ELEM_BLOCK_EXTENSION) ||
		(elemDecl->subtypes->flags &
		    XML_SCHEMAS_TYPE_BLOCK_EXTENSION))
		set |= SUBSET_EXTENSION;

	    if ((elemDecl->flags & XML_SCHEMAS_ELEM_BLOCK_RESTRICTION) ||
		(elemDecl->subtypes->flags &
		    XML_SCHEMAS_TYPE_BLOCK_RESTRICTION))
		set |= SUBSET_RESTRICTION;

	    if (xmlSchemaCheckCOSDerivedOK(*localType,
		elemDecl->subtypes, set) != 0) {
		xmlChar *str = NULL;

		xmlSchemaCustomErr((xmlSchemaAbstractCtxtPtr) vctxt,
		    XML_SCHEMAV_CVC_ELT_4_3, NULL, NULL,
		    "The type definition '%s', specified by xsi:type, is "
		    "blocked or not validly derived from the type definition "
		    "of the element declaration",
		    xmlSchemaFormatQName(&str,
			(*localType)->targetNamespace,
			(*localType)->name),
		    NULL);
		FREE_AND_NULL(str);
		ret = vctxt->err;
		*localType = NULL;
	    }
	}
    }
exit:
    ACTIVATE_ELEM;
    return (ret);
internal_error:
    ACTIVATE_ELEM;
    return (-1);
}

static int
xmlSchemaValidateElemDecl(xmlSchemaValidCtxtPtr vctxt)
{
    xmlSchemaElementPtr elemDecl = vctxt->inode->decl;
    xmlSchemaTypePtr actualType = ELEM_TYPE(elemDecl);

    /*
    * cvc-elt (3.3.4) : 1
    */
    if (elemDecl == NULL) {
	VERROR(XML_SCHEMAV_CVC_ELT_1, NULL,
	    "No matching declaration available");
        return (vctxt->err);
    }
    /*
    * cvc-elt (3.3.4) : 2
    */
    if (elemDecl->flags & XML_SCHEMAS_ELEM_ABSTRACT) {
	VERROR(XML_SCHEMAV_CVC_ELT_2, NULL,
	    "The element declaration is abstract");
        return (vctxt->err);
    }
    if (actualType == NULL) {
    	VERROR(XML_SCHEMAV_CVC_TYPE_1, NULL,
    	    "The type definition is absent");
    	return (XML_SCHEMAV_CVC_TYPE_1);
    }
    if (vctxt->nbAttrInfos != 0) {
	int ret;
	xmlSchemaAttrInfoPtr iattr;
	/*
	* cvc-elt (3.3.4) : 3
	* Handle 'xsi:nil'.
	*/
	iattr = xmlSchemaGetMetaAttrInfo(vctxt,
	    XML_SCHEMA_ATTR_INFO_META_XSI_NIL);
	if (iattr) {
	    ACTIVATE_ATTRIBUTE(iattr);
	    /*
	    * Validate the value.
	    */
	    ret = xmlSchemaVCheckCVCSimpleType(
		(xmlSchemaAbstractCtxtPtr) vctxt, NULL,
		xmlSchemaGetBuiltInType(XML_SCHEMAS_BOOLEAN),
		iattr->value, &(iattr->val), 1, 0, 0);
	    ACTIVATE_ELEM;
	    if (ret < 0) {
		VERROR_INT("xmlSchemaValidateElemDecl",
		    "calling xmlSchemaVCheckCVCSimpleType() to "
		    "validate the attribute 'xsi:nil'");
		return (-1);
	    }
	    if (ret == 0) {
		if ((elemDecl->flags & XML_SCHEMAS_ELEM_NILLABLE) == 0) {
		    /*
		    * cvc-elt (3.3.4) : 3.1
		    */
		    VERROR(XML_SCHEMAV_CVC_ELT_3_1, NULL,
			"The element is not 'nillable'");
		    /* Does not return an error on purpose. */
		} else {
		    if (xmlSchemaValueGetAsBoolean(iattr->val)) {
			/*
			* cvc-elt (3.3.4) : 3.2.2
			*/
			if ((elemDecl->flags & XML_SCHEMAS_ELEM_FIXED) &&
			    (elemDecl->value != NULL)) {
			    VERROR(XML_SCHEMAV_CVC_ELT_3_2_2, NULL,
				"The element cannot be 'nilled' because "
				"there is a fixed value constraint defined "
				"for it");
			     /* Does not return an error on purpose. */
			} else
			    vctxt->inode->flags |=
				XML_SCHEMA_ELEM_INFO_NILLED;
		    }
		}
	    }
	}
	/*
	* cvc-elt (3.3.4) : 4
	* Handle 'xsi:type'.
	*/
	iattr = xmlSchemaGetMetaAttrInfo(vctxt,
	    XML_SCHEMA_ATTR_INFO_META_XSI_TYPE);
	if (iattr) {
	    xmlSchemaTypePtr localType = NULL;

	    ret = xmlSchemaProcessXSIType(vctxt, iattr, &localType,
		elemDecl);
	    if (ret != 0) {
		if (ret == -1) {
		    VERROR_INT("xmlSchemaValidateElemDecl",
			"calling xmlSchemaProcessXSIType() to "
			"process the attribute 'xsi:type'");
		    return (-1);
		}
		/* Does not return an error on purpose. */
	    }
	    if (localType != NULL) {
		vctxt->inode->flags |= XML_SCHEMA_ELEM_INFO_LOCAL_TYPE;
		actualType = localType;
	    }
	}
    }
    /*
    * IDC: Register identity-constraint XPath matchers.
    */
    if ((elemDecl->idcs != NULL) &&
	(xmlSchemaIDCRegisterMatchers(vctxt, elemDecl) == -1))
	    return (-1);
    /*
    * No actual type definition.
    */
    if (actualType == NULL) {
    	VERROR(XML_SCHEMAV_CVC_TYPE_1, NULL,
    	    "The type definition is absent");
    	return (XML_SCHEMAV_CVC_TYPE_1);
    }
    /*
    * Remember the actual type definition.
    */
    vctxt->inode->typeDef = actualType;

    return (0);
}

static int
xmlSchemaVAttributesSimple(xmlSchemaValidCtxtPtr vctxt)
{
    xmlSchemaAttrInfoPtr iattr;
    int ret = 0, i;

    /*
    * SPEC cvc-type (3.1.1)
    * "The attributes of must be empty, excepting those whose namespace
    * name is identical to http://www.w3.org/2001/XMLSchema-instance and
    * whose local name is one of type, nil, schemaLocation or
    * noNamespaceSchemaLocation."
    */
    if (vctxt->nbAttrInfos == 0)
	return (0);
    for (i = 0; i < vctxt->nbAttrInfos; i++) {
	iattr = vctxt->attrInfos[i];
	if (! iattr->metaType) {
	    ACTIVATE_ATTRIBUTE(iattr)
	    xmlSchemaIllegalAttrErr((xmlSchemaAbstractCtxtPtr) vctxt,
		XML_SCHEMAV_CVC_TYPE_3_1_1, iattr, NULL);
	    ret = XML_SCHEMAV_CVC_TYPE_3_1_1;
        }
    }
    ACTIVATE_ELEM
    return (ret);
}

/*
* Cleanup currently used attribute infos.
*/
static void
xmlSchemaClearAttrInfos(xmlSchemaValidCtxtPtr vctxt)
{
    int i;
    xmlSchemaAttrInfoPtr attr;

    if (vctxt->nbAttrInfos == 0)
	return;
    for (i = 0; i < vctxt->nbAttrInfos; i++) {
	attr = vctxt->attrInfos[i];
	if (attr->flags & XML_SCHEMA_NODE_INFO_FLAG_OWNED_NAMES) {
	    if (attr->localName != NULL)
		xmlFree((xmlChar *) attr->localName);
	    if (attr->nsName != NULL)
		xmlFree((xmlChar *) attr->nsName);
	}
	if (attr->flags & XML_SCHEMA_NODE_INFO_FLAG_OWNED_VALUES) {
	    if (attr->value != NULL)
		xmlFree((xmlChar *) attr->value);
	}
	if (attr->val != NULL) {
	    xmlSchemaFreeValue(attr->val);
	    attr->val = NULL;
	}
	memset(attr, 0, sizeof(xmlSchemaAttrInfo));
    }
    vctxt->nbAttrInfos = 0;
}

/*
* 3.4.4 Complex Type Definition Validation Rules
*   Element Locally Valid (Complex Type) (cvc-complex-type)
* 3.2.4 Attribute Declaration Validation Rules
*   Validation Rule: Attribute Locally Valid (cvc-attribute)
*   Attribute Locally Valid (Use) (cvc-au)
*
* Only "assessed" attribute information items will be visible to
* IDCs. I.e. not "lax" (without declaration) and "skip" wild attributes.
*/
static int
xmlSchemaVAttributesComplex(xmlSchemaValidCtxtPtr vctxt)
{
    xmlSchemaTypePtr type = vctxt->inode->typeDef;
    xmlSchemaAttributeLinkPtr attrUseLink;
    xmlSchemaAttributePtr attrUse = NULL, attrDecl = NULL;
    xmlSchemaAttrInfoPtr attr, tmpAttr;
    int i, found, nbAttrs;
    int xpathRes = 0, res, wildIDs = 0, fixed;

    /*
    * SPEC (cvc-attribute)
    * (1) "The declaration must not be ·absent· (see Missing
    * Sub-components (§5.3) for how this can fail to be
    * the case)."
    * (2) "Its {type definition} must not be absent."
    *
    * NOTE (1) + (2): This is not handled here, since we currently do not
    * allow validation against schemas which have missing sub-components.
    *
    * SPEC (cvc-complex-type)
    * (3) "For each attribute information item in the element information
    * item's [attributes] excepting those whose [namespace name] is
    * identical to http://www.w3.org/2001/XMLSchema-instance and whose
    * [local name] is one of type, nil, schemaLocation or
    * noNamespaceSchemaLocation, the appropriate case among the following
    * must be true:
    *
    */  
    nbAttrs = vctxt->nbAttrInfos;
    for (attrUseLink = type->attributeUses; attrUseLink != NULL;
	attrUseLink = attrUseLink->next) {

        found = 0;
	attrUse = attrUseLink->attr;
	/*
	* VAL TODO: Implement a real "attribute use" component.
	*/
	if (attrUse->refDecl != NULL)
	    attrDecl = attrUse->refDecl;
	else
	    attrDecl = attrUse;
        for (i = 0; i < nbAttrs; i++) {
	    attr = vctxt->attrInfos[i];
	    /*
	    * SPEC (cvc-complex-type) (3)
	    * Skip meta attributes.
	    */
	    if (attr->metaType)
		continue;
	    if (attr->localName[0] != attrDecl->name[0])
		continue;
	    if (!xmlStrEqual(attr->localName, attrDecl->name))
		continue;
	    if (!xmlStrEqual(attr->nsName, attrDecl->targetNamespace))
		continue;
	    found = 1;
	    /*
	    * SPEC (cvc-complex-type)
	    * (3.1) "If there is among the {attribute uses} an attribute
	    * use with an {attribute declaration} whose {name} matches
	    * the attribute information item's [local name] and whose
	    * {target namespace} is identical to the attribute information
	    * item's [namespace name] (where an ·absent· {target namespace}
	    * is taken to be identical to a [namespace name] with no value),
	    * then the attribute information must be ·valid· with respect
	    * to that attribute use as per Attribute Locally Valid (Use)
	    * (§3.5.4). In this case the {attribute declaration} of that
	    * attribute use is the ·context-determined declaration· for the
	    * attribute information item with respect to Schema-Validity
	    * Assessment (Attribute) (§3.2.4) and
	    * Assessment Outcome (Attribute) (§3.2.5).
	    */
	    attr->state = XML_SCHEMAS_ATTR_ASSESSED;
	    attr->use = attrUse;
	    /*
	    * Context-determined declaration.
	    */
	    attr->decl = attrDecl;
	    attr->typeDef = attrDecl->subtypes;
	    break;
	}

	if (found)
	    continue;

	if (attrUse->occurs == XML_SCHEMAS_ATTR_USE_REQUIRED) {
	    /*
	    * Handle non-existent, required attributes.
	    *
	    * SPEC (cvc-complex-type)
	    * (4) "The {attribute declaration} of each attribute use in
	    * the {attribute uses} whose {required} is true matches one
	    * of the attribute information items in the element information
	    * item's [attributes] as per clause 3.1 above."
	    */
	    tmpAttr = xmlSchemaGetFreshAttrInfo(vctxt);
	    if (tmpAttr == NULL) {
		VERROR_INT(
		    "xmlSchemaVAttributesComplex",
		    "calling xmlSchemaGetFreshAttrInfo()");
		return (-1);
	    }
	    tmpAttr->state = XML_SCHEMAS_ATTR_ERR_MISSING;
	    tmpAttr->use = attrUse;
	    tmpAttr->decl = attrDecl;	    
	} else if ((attrUse->occurs == XML_SCHEMAS_ATTR_USE_OPTIONAL) &&
	    ((attrUse->defValue != NULL) ||
	     (attrDecl->defValue != NULL))) {
	    /*
	    * Handle non-existent, optional, default/fixed attributes.
	    */
	    tmpAttr = xmlSchemaGetFreshAttrInfo(vctxt);
	    if (tmpAttr == NULL) {
		VERROR_INT(
		    "xmlSchemaVAttributesComplex",
		    "calling xmlSchemaGetFreshAttrInfo()");
		return (-1);
	    }
	    tmpAttr->state = XML_SCHEMAS_ATTR_DEFAULT;
	    tmpAttr->use = attrUse;
	    tmpAttr->decl = attrDecl;
	    tmpAttr->typeDef = attrDecl->subtypes;
	    tmpAttr->localName = attrDecl->name;
	    tmpAttr->nsName = attrDecl->targetNamespace;
	}
    }
    if (vctxt->nbAttrInfos == 0)
	return (0);
    /*
    * Validate against the wildcard.
    */
    if (type->attributeWildcard != NULL) {
	/*
	* SPEC (cvc-complex-type)
	* (3.2.1) "There must be an {attribute wildcard}."
	*/
	for (i = 0; i < nbAttrs; i++) {
	    attr = vctxt->attrInfos[i];
	    /*
	    * SPEC (cvc-complex-type) (3)
	    * Skip meta attributes.
	    */
	    if (attr->state != XML_SCHEMAS_ATTR_UNKNOWN)
		continue;
	    /*
	    * SPEC (cvc-complex-type)
	    * (3.2.2) "The attribute information item must be ·valid· with
	    * respect to it as defined in Item Valid (Wildcard) (§3.10.4)."
	    *
	    * SPEC Item Valid (Wildcard) (cvc-wildcard)
	    * "... its [namespace name] must be ·valid· with respect to
	    * the wildcard constraint, as defined in Wildcard allows
	    * Namespace Name (§3.10.4)."
	    */
	    if (xmlSchemaCheckCVCWildcardNamespace(type->attributeWildcard,
		    attr->nsName)) {
		/*
		* Handle processContents.
		*
		* SPEC (cvc-wildcard):
		* processContents | context-determined declaration:
		* "strict"          "mustFind"
		* "lax"             "none"
		* "skip"            "skip"
		*/
		if (type->attributeWildcard->processContents ==
		    XML_SCHEMAS_ANY_SKIP) {
		     /*
		    * context-determined declaration = "skip"
		    *
		    * SPEC PSVI Assessment Outcome (Attribute)
		    * [validity] = "notKnown"
		    * [validation attempted] = "none"
		    */
		    attr->state = XML_SCHEMAS_ATTR_WILD_SKIP;
		    continue;
		}
		/*
		* Find an attribute declaration.
		*/
		attr->decl = xmlSchemaGetAttributeDecl(vctxt->schema,
		    attr->localName, attr->nsName);
		if (attr->decl != NULL) {
		    attr->state = XML_SCHEMAS_ATTR_ASSESSED;
		    /*
		    * SPEC (cvc-complex-type)
		    * (5) "Let [Definition:]  the wild IDs be the set of
		    * all attribute information item to which clause 3.2
		    * applied and whose ·validation· resulted in a
		    * ·context-determined declaration· of mustFind or no
		    * ·context-determined declaration· at all, and whose
		    * [local name] and [namespace name] resolve (as
		    * defined by QName resolution (Instance) (§3.15.4)) to
		    * an attribute declaration whose {type definition} is
		    * or is derived from ID. Then all of the following
		    * must be true:"
		    */
		    attr->typeDef = attr->decl->subtypes;
		    if (xmlSchemaIsDerivedFromBuiltInType(
			attr->typeDef, XML_SCHEMAS_ID)) {
			/*
			* SPEC (5.1) "There must be no more than one
			* item in ·wild IDs·."
			*/
			if (wildIDs != 0) {
			    /* VAL TODO */
			    attr->state = XML_SCHEMAS_ATTR_ERR_WILD_DUPLICATE_ID;
			    TODO
			    continue;
			}
			wildIDs++;
			/*
			* SPEC (cvc-complex-type)
			* (5.2) "If ·wild IDs· is non-empty, there must not
			* be any attribute uses among the {attribute uses}
			* whose {attribute declaration}'s {type definition}
			* is or is derived from ID."
			*/
			for (attrUseLink = type->attributeUses;
			    attrUseLink != NULL;
			    attrUseLink = attrUseLink->next) {
			    if (xmlSchemaIsDerivedFromBuiltInType(
				attrUseLink->attr->subtypes,
				XML_SCHEMAS_ID)) {
				/* VAL TODO */
				attr->state = XML_SCHEMAS_ATTR_ERR_WILD_AND_USE_ID;
				TODO
			    }
			}
		    }
		} else if (type->attributeWildcard->processContents ==
		    XML_SCHEMAS_ANY_LAX) {
		    attr->state = XML_SCHEMAS_ATTR_WILD_LAX_NO_DECL;
		    /*
		    * SPEC PSVI Assessment Outcome (Attribute)
		    * [validity] = "notKnown"
		    * [validation attempted] = "none"
		    */
		} else {
		    attr->state = XML_SCHEMAS_ATTR_ERR_WILD_STRICT_NO_DECL;
		}
	    }
	}
    }


    if (vctxt->nbAttrInfos == 0)
	return (0);

    /*
    * Validate values, create default attributes, evaluate IDCs.
    */
    for (i = 0; i < vctxt->nbAttrInfos; i++) {
	attr = vctxt->attrInfos[i];
	/*
	* VAL TODO: Note that we won't try to resolve IDCs to
	* "lax" and "skip" validated attributes. Check what to
	* do in this case.
	*/
	if ((attr->state != XML_SCHEMAS_ATTR_ASSESSED) &&
	    (attr->state != XML_SCHEMAS_ATTR_DEFAULT))
	    continue;
	/*
	* VAL TODO: What to do if the type definition is missing?
	*/
	if (attr->typeDef == NULL) {
	    attr->state = XML_SCHEMAS_ATTR_ERR_NO_TYPE;
	    continue;
	}

	ACTIVATE_ATTRIBUTE(attr);
	fixed = 0;
	xpathRes = 0;

	if (vctxt->xpathStates != NULL) {
	    /*
	    * Evaluate IDCs.
	    */
	    xpathRes = xmlSchemaXPathEvaluate(vctxt,
		XML_ATTRIBUTE_NODE);
	    if (xpathRes == -1) {
		VERROR_INT("xmlSchemaVAttributesComplex",
		    "calling xmlSchemaXPathEvaluate()");
		goto internal_error;
	    }
	}

	if (attr->state == XML_SCHEMAS_ATTR_DEFAULT) {
	    /*
	    * Default/fixed attributes.
	    */
	    if (xpathRes) {
		if (attr->use->defValue == NULL) {
		    attr->value = (xmlChar *) attr->use->defValue;
		    attr->val = attr->use->defVal;
		} else {
		    attr->value = (xmlChar *) attr->decl->defValue;
		    attr->val = attr->decl->defVal;
		}
		/*
		* IDCs will consume the precomputed default value,
		* so we need to clone it.
		*/
		if (attr->val == NULL) {
		    VERROR_INT("xmlSchemaVAttributesComplex",
			"default/fixed value on an attribute use was "
			"not precomputed");
		    goto internal_error;
		}
		attr->val = xmlSchemaCopyValue(attr->val);
		if (attr->val == NULL) {
		    VERROR_INT("xmlSchemaVAttributesComplex",
			"calling xmlSchemaCopyValue()");
		    goto internal_error;
		}
	    }
	    /*
	    * PSVI: Add the default attribute to the current element.
	    * VAL TODO: Should we use the *normalized* value? This currently
	    *   uses the *initial* value.
	    */
	    if ((vctxt->options & XML_SCHEMA_VAL_VC_I_CREATE) &&
		(attr->node != NULL) && (attr->node->doc != NULL)) {
		xmlChar *normValue;
		const xmlChar *value;

		value = attr->value;
		/*
		* Normalize the value.
		*/
		normValue = xmlSchemaNormalizeValue(attr->typeDef,
		    attr->value);
		if (normValue != NULL)
		    value = BAD_CAST normValue;

		if (attr->nsName == NULL) {
		    if (xmlNewProp(attr->node->parent,
			attr->localName, value) == NULL) {
			VERROR_INT("xmlSchemaVAttributesComplex",
			    "callling xmlNewProp()");
			if (normValue != NULL)
			    xmlFree(normValue);
			goto internal_error;
		    }
		} else {
		    xmlNsPtr ns;

		    ns = xmlSearchNsByHref(attr->node->doc,
			attr->node->parent, attr->nsName);
		    if (ns == NULL) {
			xmlChar prefix[12];
			int counter = 0;

			/*
			* Create a namespace declaration on the validation
			* root node if no namespace declaration is in scope.
			*/
			do {
			    snprintf((char *) prefix, 12, "p%d", counter++);
			    ns = xmlSearchNs(attr->node->doc,
				attr->node->parent, BAD_CAST prefix);
			    if (counter > 1000) {
				VERROR_INT(
				    "xmlSchemaVAttributesComplex",
				    "could not compute a ns prefix for a "
				    "default/fixed attribute");
				if (normValue != NULL)
				    xmlFree(normValue);
				goto internal_error;
			    }
			} while (ns != NULL);
			ns = xmlNewNs(vctxt->validationRoot,
			    attr->nsName, BAD_CAST prefix);
		    }
		    xmlNewNsProp(attr->node->parent, ns,
			attr->localName, value);
		}
		if (normValue != NULL)
		    xmlFree(normValue);
	    }
	    /*
	    * Go directly to IDC evaluation.
	    */
	    goto eval_idcs;
	}
	/*
	* Validate the value.
	*/
	if (vctxt->value != NULL) {
	    /*
	    * Free last computed value; just for safety reasons.
	    */
	    xmlSchemaFreeValue(vctxt->value);
	    vctxt->value = NULL;
	}
	/*
	* Note that the attribute *use* can be unavailable, if
	* the attribute was a wild attribute.
	*/
	if ((attr->decl->flags & XML_SCHEMAS_ATTR_FIXED) ||
	    ((attr->use != NULL) &&
	     (attr->use->flags & XML_SCHEMAS_ATTR_FIXED)))
	    fixed = 1;
	else
	    fixed = 0;
	/*
	* SPEC (cvc-attribute)
	* (3) "The item's ·normalized value· must be locally ·valid·
	* with respect to that {type definition} as per 
	* String Valid (§3.14.4)."
	*
	* VAL TODO: Do we already have the
	* "normalized attribute value" here?
	*/
	if (xpathRes || fixed) {
	    attr->flags |= XML_SCHEMA_NODE_INFO_VALUE_NEEDED;
	    /*
	    * Request a computed value.
	    */
	    res = xmlSchemaVCheckCVCSimpleType(
		(xmlSchemaAbstractCtxtPtr) vctxt,
		attr->node, attr->typeDef, attr->value, &(attr->val),
		1, 1, 0);
	} else {
	    res = xmlSchemaVCheckCVCSimpleType(
		(xmlSchemaAbstractCtxtPtr) vctxt,
		attr->node, attr->typeDef, attr->value, NULL,
		1, 0, 0);
	}
	    
	if (res != 0) {
	    if (res == -1) {
		VERROR_INT("xmlSchemaVAttributesComplex",
		    "calling xmlSchemaStreamValidateSimpleTypeValue()");
		goto internal_error;
	    }
	    attr->state = XML_SCHEMAS_ATTR_INVALID_VALUE;
	    /*
	    * SPEC PSVI Assessment Outcome (Attribute)
	    * [validity] = "invalid"
	    */
	    goto eval_idcs;
	}

	if (fixed) {
	    int ws;
	    /*
	    * SPEC Attribute Locally Valid (Use) (cvc-au)
	    * "For an attribute information item to be·valid·
	    * with respect to an attribute use its *normalized*
	    * value· must match the *canonical* lexical
	    * representation of the attribute use's {value
	    * constraint}value, if it is present and fixed."
	    *
	    * VAL TODO: The requirement for the *canonical* value
	    * will be removed in XML Schema 1.1.
	    */
	    /*
	    * SPEC Attribute Locally Valid (cvc-attribute)
	    * (4) "The item's *actual* value· must match the *value* of
	    * the {value constraint}, if it is present and fixed."
	    */
	    ws = xmlSchemaGetWhiteSpaceFacetValue(attr->typeDef);
	    if (attr->val == NULL) {
		/* VAL TODO: A value was not precomputed. */
		TODO
		goto eval_idcs;
	    }
	    if ((attr->use != NULL) &&
		(attr->use->defValue != NULL)) {
		if (attr->use->defVal == NULL) {
		    /* VAL TODO: A default value was not precomputed. */
		    TODO
		    goto eval_idcs;
		}
		attr->vcValue = attr->use->defValue;
		/*
		if (xmlSchemaCompareValuesWhtsp(attr->val,
		    (xmlSchemaWhitespaceValueType) ws,
		    attr->use->defVal,
		    (xmlSchemaWhitespaceValueType) ws) != 0) {
		*/
		if (! xmlSchemaAreValuesEqual(attr->val, attr->use->defVal))
		    attr->state = XML_SCHEMAS_ATTR_ERR_FIXED_VALUE;
	    } else {
		if (attr->decl->defVal == NULL) {
		    /* VAL TODO: A default value was not precomputed. */
		    TODO
		    goto eval_idcs;
		}
		attr->vcValue = attr->decl->defValue;
		/*
		if (xmlSchemaCompareValuesWhtsp(attr->val,
		    (xmlSchemaWhitespaceValueType) ws,
		    attrDecl->defVal,
		    (xmlSchemaWhitespaceValueType) ws) != 0) {
		*/
		if (! xmlSchemaAreValuesEqual(attr->val, attr->decl->defVal))
		    attr->state = XML_SCHEMAS_ATTR_ERR_FIXED_VALUE;
	    }
	    /*
	    * [validity] = "valid"
	    */
	}
eval_idcs:
	/*
	* Evaluate IDCs.
	*/
	if (xpathRes) {
	    if (xmlSchemaXPathProcessHistory(vctxt,
		vctxt->depth +1) == -1) {
		VERROR_INT("xmlSchemaVAttributesComplex",
		    "calling xmlSchemaXPathEvaluate()");
		goto internal_error;
	    }
	}
    }

    /*
    * Report errors.
    */
    for (i = 0; i < vctxt->nbAttrInfos; i++) {
	attr = vctxt->attrInfos[i];
	if ((attr->state == XML_SCHEMAS_ATTR_META) ||
	    (attr->state == XML_SCHEMAS_ATTR_ASSESSED) ||
	    (attr->state == XML_SCHEMAS_ATTR_WILD_SKIP) ||
	    (attr->state == XML_SCHEMAS_ATTR_WILD_LAX_NO_DECL))
	    continue;
	ACTIVATE_ATTRIBUTE(attr);
	switch (attr->state) {
	    case XML_SCHEMAS_ATTR_ERR_MISSING: {
		    xmlChar *str = NULL;
		    ACTIVATE_ELEM;
		    xmlSchemaCustomErr((xmlSchemaAbstractCtxtPtr) vctxt,
			XML_SCHEMAV_CVC_COMPLEX_TYPE_4, NULL, NULL,
			"The attribute '%s' is required but missing",
			xmlSchemaFormatQName(&str,
			    attr->decl->targetNamespace,
			    attr->decl->name),
			NULL);
		    FREE_AND_NULL(str)
		    break;
		}
	    case XML_SCHEMAS_ATTR_ERR_NO_TYPE:
		VERROR(XML_SCHEMAV_CVC_ATTRIBUTE_2, NULL,
		    "The type definition is absent");
		break;
	    case XML_SCHEMAS_ATTR_ERR_FIXED_VALUE:
		xmlSchemaCustomErr((xmlSchemaAbstractCtxtPtr) vctxt,
		    XML_SCHEMAV_CVC_AU, NULL, NULL,
		    "The value '%s' does not match the fixed "
		    "value constraint '%s'", 
		    attr->value, attr->vcValue);		
		break;
	    case XML_SCHEMAS_ATTR_ERR_WILD_STRICT_NO_DECL:
		VERROR(XML_SCHEMAV_CVC_WILDCARD, NULL,
		    "No matching global attribute declaration available, but "
		    "demanded by the strict wildcard");
		break;
	    case XML_SCHEMAS_ATTR_UNKNOWN:
		if (attr->metaType)
		    break;
		/*
		* MAYBE VAL TODO: One might report different error messages
		* for the following errors.
		*/
		if (type->attributeWildcard == NULL) {
		    xmlSchemaIllegalAttrErr((xmlSchemaAbstractCtxtPtr) vctxt,
			XML_SCHEMAV_CVC_COMPLEX_TYPE_3_2_1, attr, NULL);
		} else {
		    xmlSchemaIllegalAttrErr((xmlSchemaAbstractCtxtPtr) vctxt,
			XML_SCHEMAV_CVC_COMPLEX_TYPE_3_2_2, attr, NULL);
		}
		break;
	    default:
		break;
	}
    }

    ACTIVATE_ELEM;
    return (0);
internal_error:
    ACTIVATE_ELEM;
    return (-1);
}

static int
xmlSchemaValidateElemWildcard(xmlSchemaValidCtxtPtr vctxt,
			      int *skip)
{
    xmlSchemaWildcardPtr wild = (xmlSchemaWildcardPtr) vctxt->inode->decl;
    /*
    * The namespace of the element was already identified to be
    * matching the wildcard.
    */
    if ((skip == NULL) || (wild == NULL) ||
	(wild->type != XML_SCHEMA_TYPE_ANY)) {
	VERROR_INT("xmlSchemaValidateElemWildcard",
	    "bad arguments");
	return (-1);
    }
    *skip = 0;
    if (wild->negNsSet != NULL) {
	/*
	* URGENT VAL TODO: Fix the content model to reject
	* "##other" wildcards.
	*/
	if (xmlSchemaCheckCVCWildcardNamespace(wild,
	    vctxt->inode->nsName) != 0) {
	    if ((wild->minOccurs == 1) && (wild->maxOccurs == 1)) {
		xmlSchemaNodeInfoPtr pinode = vctxt->elemInfos[vctxt->depth -1];
		/*
		* VAL TODO: Workaround possible *only* if minOccurs and
		* maxOccurs are 1.
		*/
		xmlSchemaComplexTypeErr((xmlSchemaAbstractCtxtPtr) vctxt,
		    /* VAL TODO: error code? */
		    XML_SCHEMAV_ELEMENT_CONTENT, NULL,
		    (xmlSchemaTypePtr) wild,
		    "This element is not accepted by the wildcard",
		    0, 0, NULL);
		vctxt->skipDepth = vctxt->depth;
		if ((pinode->flags &
		    XML_SCHEMA_ELEM_INFO_ERR_BAD_CONTENT) == 0)
		    pinode->flags |= XML_SCHEMA_ELEM_INFO_ERR_BAD_CONTENT;
		vctxt->inode->flags |= XML_SCHEMA_NODE_INFO_ERR_NOT_EXPECTED;
		return (XML_SCHEMAV_ELEMENT_CONTENT);
	    }
	    if (wild->processContents == XML_SCHEMAS_ANY_SKIP) {
		*skip = 1;
		return (0);
	    }
	    vctxt->inode->typeDef =
		xmlSchemaGetBuiltInType(XML_SCHEMAS_ANYTYPE);
	    return (0);
	}
    }
    if (wild->processContents == XML_SCHEMAS_ANY_SKIP) {
	/*
	* URGENT VAL TODO: Either we need to position the stream to the
	* next sibling, or walk the whole subtree.
	*/
	*skip = 1;
	return (0);
    }
    {
	xmlSchemaElementPtr decl = NULL;

	decl = xmlHashLookup3(vctxt->schema->elemDecl,
	    vctxt->inode->localName, vctxt->inode->nsName,
	    NULL);
	if (decl != NULL) {
	    vctxt->inode->decl = decl;
	    return (0);
	}
    }
    if (wild->processContents == XML_SCHEMAS_ANY_STRICT) {
	/* VAL TODO: Change to proper error code. */
	VERROR(XML_SCHEMAV_CVC_ELT_1, (xmlSchemaTypePtr) wild,
	    "No matching global element declaration available, but "
	    "demanded by the strict wildcard");
	return (vctxt->err);
    }
    if (vctxt->nbAttrInfos != 0) {
	xmlSchemaAttrInfoPtr iattr;
	/*
	* SPEC Validation Rule: Schema-Validity Assessment (Element)
	* (1.2.1.2.1) - (1.2.1.2.3 )
	*
	* Use the xsi:type attribute for the type definition.
	*/
	iattr = xmlSchemaGetMetaAttrInfo(vctxt,
	    XML_SCHEMA_ATTR_INFO_META_XSI_TYPE);
	if (iattr != NULL) {
	    if (xmlSchemaProcessXSIType(vctxt, iattr,
		&(vctxt->inode->typeDef), NULL) == -1) {
		VERROR_INT("xmlSchemaValidateElemWildcard",
		    "calling xmlSchemaProcessXSIType() to "
		    "process the attribute 'xsi:nil'");
		return (-1);
	    }
	    /*
	    * Don't return an error on purpose.
	    */
	    return (0);
	}
    }
    /*
    * SPEC Validation Rule: Schema-Validity Assessment (Element)
    *
    * Fallback to "anyType".
    */
    vctxt->inode->typeDef =
	xmlSchemaGetBuiltInType(XML_SCHEMAS_ANYTYPE);
    return (0);
}

/*
* xmlSchemaCheckCOSValidDefault:
*
* This will be called if: not nilled, no content and a default/fixed
* value is provided.
*/

static int
xmlSchemaCheckCOSValidDefault(xmlSchemaValidCtxtPtr vctxt,
			      const xmlChar *value,
			      xmlSchemaValPtr *val)
{   
    int ret = 0;
    xmlSchemaNodeInfoPtr inode = vctxt->inode;

    /*
    * cos-valid-default:
    * Schema Component Constraint: Element Default Valid (Immediate)
    * For a string to be a valid default with respect to a type 
    * definition the appropriate case among the following must be true:
    */    
    if IS_COMPLEX_TYPE(inode->typeDef) {
	/*
	* Complex type.
	*
	* SPEC (2.1) "its {content type} must be a simple type definition
	* or mixed."
	* SPEC (2.2.2) "If the {content type} is mixed, then the {content
	* type}'s particle must be ·emptiable· as defined by 
	* Particle Emptiable (§3.9.6)."
	*/
	if ((! HAS_SIMPLE_CONTENT(inode->typeDef)) &&
	    ((! HAS_MIXED_CONTENT(inode->typeDef)) ||
	     (! IS_PARTICLE_EMPTIABLE(inode->typeDef)))) {
	    ret = XML_SCHEMAP_COS_VALID_DEFAULT_2_1;
	    /* NOTE that this covers (2.2.2) as well. */
	    VERROR(ret, NULL,
		"For a string to be a valid default, the type definition "
		"must be a simple type or a complex type with simple content "
		"or mixed content and a particle emptiable");
	    return(ret);
	}
    }	
    /*
    * 1 If the type definition is a simple type definition, then the string 
    * must be ·valid· with respect to that definition as defined by String 
    * Valid (§3.14.4).
    *
    * AND
    *
    * 2.2.1 If the {content type} is a simple type definition, then the 
    * string must be ·valid· with respect to that simple type definition 
    * as defined by String Valid (§3.14.4).
    */  
    if (IS_SIMPLE_TYPE(inode->typeDef)) {

	ret = xmlSchemaVCheckCVCSimpleType((xmlSchemaAbstractCtxtPtr) vctxt,
	    NULL, inode->typeDef, value, val, 1, 1, 0);

    } else if (HAS_SIMPLE_CONTENT(inode->typeDef)) {

	ret = xmlSchemaVCheckCVCSimpleType((xmlSchemaAbstractCtxtPtr) vctxt,
	    NULL, inode->typeDef->contentTypeDef, value, val, 1, 1, 0);
    }
    if (ret < 0) {
	VERROR_INT("xmlSchemaCheckCOSValidDefault",
	    "calling xmlSchemaVCheckCVCSimpleType()");
    }    
    return (ret);
}

static void
xmlSchemaVContentModelCallback(xmlSchemaValidCtxtPtr vctxt ATTRIBUTE_UNUSED,
			       const xmlChar * name ATTRIBUTE_UNUSED,
			       xmlSchemaElementPtr item,
			       xmlSchemaNodeInfoPtr inode)
{
    inode->decl = item;
#ifdef DEBUG_CONTENT
    {
	xmlChar *str = NULL;

	if (item->type == XML_SCHEMA_TYPE_ELEMENT) {
	    xmlGenericError(xmlGenericErrorContext,
		"AUTOMATON callback for '%s' [declaration]\n",
		xmlSchemaFormatQName(&str,
		inode->localName, inode->nsName));
	} else {
	    xmlGenericError(xmlGenericErrorContext,
		    "AUTOMATON callback for '%s' [wildcard]\n",
		    xmlSchemaFormatQName(&str,
		    inode->localName, inode->nsName));

	}
	FREE_AND_NULL(str)
    }
#endif
}

static int
xmlSchemaValidatorPushElem(xmlSchemaValidCtxtPtr vctxt)
{    
    vctxt->inode = xmlSchemaGetFreshElemInfo(vctxt);
    if (vctxt->inode == NULL) {
	VERROR_INT("xmlSchemaValidatorPushElem",
	    "calling xmlSchemaGetFreshElemInfo()");
	return (-1);
    }   
    vctxt->nbAttrInfos = 0;
    return (0);
}

static int
xmlSchemaVCheckINodeDataType(xmlSchemaValidCtxtPtr vctxt,
			     xmlSchemaNodeInfoPtr inode,
			     xmlSchemaTypePtr type,
			     const xmlChar *value)
{
    if (inode->flags & XML_SCHEMA_NODE_INFO_VALUE_NEEDED)
	return (xmlSchemaVCheckCVCSimpleType(
	    (xmlSchemaAbstractCtxtPtr) vctxt, NULL,
	    type, value, &(inode->val), 1, 1, 0));
    else
	return (xmlSchemaVCheckCVCSimpleType(
	    (xmlSchemaAbstractCtxtPtr) vctxt, NULL,
	    type, value, NULL, 1, 0, 0));
}



/* 
* Process END of element.
*/
static int
xmlSchemaValidatorPopElem(xmlSchemaValidCtxtPtr vctxt)
{
    int ret = 0;
    xmlSchemaNodeInfoPtr inode = vctxt->inode;

    if (vctxt->nbAttrInfos != 0)
	xmlSchemaClearAttrInfos(vctxt);
    if (inode->flags & XML_SCHEMA_NODE_INFO_ERR_NOT_EXPECTED) {
	/*
	* This element was not expected;
	* we will not validate child elements of broken parents.
	* Skip validation of all content of the parent.
	*/
	vctxt->skipDepth = vctxt->depth -1;
	goto end_elem;
    }    
    if ((inode->typeDef == NULL) ||
	(inode->flags & XML_SCHEMA_NODE_INFO_ERR_BAD_TYPE)) {
	/*
	* 1. the type definition might be missing if the element was
	*    error prone
	* 2. it might be abstract.
	*/
	goto end_elem;
    }
    /*
    * Check the content model.
    */
    if ((inode->typeDef->contentType == XML_SCHEMA_CONTENT_MIXED) ||
	(inode->typeDef->contentType == XML_SCHEMA_CONTENT_ELEMENTS)) {

	/*
	* Workaround for "anyType".
	*/
	if (inode->typeDef->builtInType == XML_SCHEMAS_ANYTYPE)
	    goto character_content;			
	
	if ((inode->flags & XML_SCHEMA_ELEM_INFO_ERR_BAD_CONTENT) == 0) {
	    xmlChar *values[10];
	    int terminal, nbval = 10, nbneg;

	    if (inode->regexCtxt == NULL) {
		/*
		* Create the regex context.
		*/
		inode->regexCtxt =
		    xmlRegNewExecCtxt(inode->typeDef->contModel,
		    (xmlRegExecCallbacks) xmlSchemaVContentModelCallback,
		    vctxt);
		if (inode->regexCtxt == NULL) {
		    VERROR_INT("xmlSchemaValidatorPopElem",
			"failed to create a regex context");
		    goto internal_error;
		}
#ifdef DEBUG_AUTOMATA
		xmlGenericError(xmlGenericErrorContext,
		    "AUTOMATON create on '%s'\n", inode->localName);
#endif	    
	    }
	    /*
	    * Get hold of the still expected content, since a further
	    * call to xmlRegExecPushString() will loose this information.
	    */ 
	    xmlRegExecNextValues(inode->regexCtxt,
		&nbval, &nbneg, &values[0], &terminal);
	    ret = xmlRegExecPushString(inode->regexCtxt, NULL, NULL);
	    if (ret <= 0) {		
		/*
		* Still missing something.
		*/
		ret = 1;
		inode->flags |=
		    XML_SCHEMA_ELEM_INFO_ERR_BAD_CONTENT;
		xmlSchemaComplexTypeErr((xmlSchemaAbstractCtxtPtr) vctxt,
		    XML_SCHEMAV_ELEMENT_CONTENT, NULL, NULL,
		    "Missing child element(s)",
		    nbval, nbneg, values);
#ifdef DEBUG_AUTOMATA
		xmlGenericError(xmlGenericErrorContext,
		    "AUTOMATON missing ERROR on '%s'\n",
		    inode->localName);
#endif
	    } else {
		/*
		* Content model is satisfied.
		*/
		ret = 0;
#ifdef DEBUG_AUTOMATA
		xmlGenericError(xmlGenericErrorContext,
		    "AUTOMATON succeeded on '%s'\n",
		    inode->localName);
#endif
	    }

	}
    }
    if (inode->typeDef->contentType == XML_SCHEMA_CONTENT_ELEMENTS)
	goto end_elem;

character_content:

    if (vctxt->value != NULL) {
	xmlSchemaFreeValue(vctxt->value);
	vctxt->value = NULL;
    }
    /*
    * Check character content.
    */
    if (inode->decl == NULL) {
	/*
	* Speedup if no declaration exists.
	*/
	if (IS_SIMPLE_TYPE(inode->typeDef)) {	    
	    ret = xmlSchemaVCheckINodeDataType(vctxt,
		inode, inode->typeDef, inode->value);
	} else if (HAS_SIMPLE_CONTENT(inode->typeDef)) {
	    ret = xmlSchemaVCheckINodeDataType(vctxt,
		inode, inode->typeDef->contentTypeDef,
		inode->value);
	}		
	if (ret < 0) {
	    VERROR_INT("xmlSchemaValidatorPopElem",
		"calling xmlSchemaVCheckCVCSimpleType()");
	    goto internal_error;
	}
	goto end_elem;
    }
    /*
    * cvc-elt (3.3.4) : 5 
    * The appropriate case among the following must be true:
    */
    /*
    * cvc-elt (3.3.4) : 5.1 
    * If the declaration has a {value constraint}, 
    * the item has neither element nor character [children] and 
    * clause 3.2 has not applied, then all of the following must be true:
    */
    if ((inode->decl->value != NULL) &&
	(inode->flags & XML_SCHEMA_ELEM_INFO_EMPTY) && 
	(! INODE_NILLED(inode))) {
	/*
	* cvc-elt (3.3.4) : 5.1.1 
	* If the ·actual type definition· is a ·local type definition·
	* then the canonical lexical representation of the {value constraint}
	* value must be a valid default for the ·actual type definition· as 
	* defined in Element Default Valid (Immediate) (§3.3.6). 
	*/
	/* 
	* NOTE: 'local' above means types aquired by xsi:type.
	* NOTE: Although the *canonical* value is stated, it is not
	* relevant if canonical or not. Additionally XML Schema 1.1
	* will removed this requirement as well.
	*/
	if (inode->flags & XML_SCHEMA_ELEM_INFO_LOCAL_TYPE) {

	    ret = xmlSchemaCheckCOSValidDefault(vctxt,
		inode->decl->value, &(inode->val));
	    if (ret != 0) {
		if (ret < 0) {
		    VERROR_INT("xmlSchemaValidatorPopElem",
			"calling xmlSchemaCheckCOSValidDefault()");
		    goto internal_error;
		}
		goto end_elem;
	    }
	    /*
	    * Stop here, to avoid redundant validation of the value
	    * (see following).
	    */
	    goto default_psvi;
	}	
	/*
	* cvc-elt (3.3.4) : 5.1.2 
	* The element information item with the canonical lexical 
	* representation of the {value constraint} value used as its 
	* ·normalized value· must be ·valid· with respect to the 
	* ·actual type definition· as defined by Element Locally Valid (Type)
	* (§3.3.4).
	*/	    
	if (IS_SIMPLE_TYPE(inode->typeDef)) {
	    ret = xmlSchemaVCheckINodeDataType(vctxt,
		inode, inode->typeDef, inode->decl->value);
	} else if (HAS_SIMPLE_CONTENT(inode->typeDef)) {
	    ret = xmlSchemaVCheckINodeDataType(vctxt,
		inode, inode->typeDef->contentTypeDef,
		inode->decl->value);	    
	}
	if (ret != 0) {
	    if (ret < 0) {
		VERROR_INT("xmlSchemaValidatorPopElem",
		    "calling xmlSchemaVCheckCVCSimpleType()");
		goto internal_error;
	    }
	    goto end_elem;
	}

default_psvi:
	/*
	* PSVI: Create a text node on the instance element.
	*/
	if ((vctxt->options & XML_SCHEMA_VAL_VC_I_CREATE) &&
	    (inode->node != NULL)) {
	    xmlNodePtr textChild;
	    xmlChar *normValue;
	    /*
	    * VAL TODO: Normalize the value.
	    */	    
	    normValue = xmlSchemaNormalizeValue(inode->typeDef,
		inode->decl->value);
	    if (normValue != NULL) {
		textChild = xmlNewText(BAD_CAST normValue);
		xmlFree(normValue);
	    } else
		textChild = xmlNewText(inode->decl->value);
	    if (textChild == NULL) {
		VERROR_INT("xmlSchemaValidatorPopElem",
		    "calling xmlNewText()");
		goto internal_error;
	    } else
		xmlAddChild(inode->node, textChild);	    
	}
	
    } else if (! INODE_NILLED(inode)) {	
	/*
	* 5.2.1 The element information item must be ·valid· with respect 
	* to the ·actual type definition· as defined by Element Locally 
	* Valid (Type) (§3.3.4).
	*/	
	if (IS_SIMPLE_TYPE(inode->typeDef)) {
	     /*
	    * SPEC (cvc-type) (3.1)
	    * "If the type definition is a simple type definition, ..."
	    * (3.1.3) "If clause 3.2 of Element Locally Valid
	    * (Element) (§3.3.4) did not apply, then the ·normalized value·
	    * must be ·valid· with respect to the type definition as defined
	    * by String Valid (§3.14.4).
	    */	    
	    ret = xmlSchemaVCheckINodeDataType(vctxt,
		    inode, inode->typeDef, inode->value);
	} else if (HAS_SIMPLE_CONTENT(inode->typeDef)) {
	    /*
	    * SPEC (cvc-type) (3.2) "If the type definition is a complex type
	    * definition, then the element information item must be
	    * ·valid· with respect to the type definition as per
	    * Element Locally Valid (Complex Type) (§3.4.4);"
	    *
	    * SPEC (cvc-complex-type) (2.2)
	    * "If the {content type} is a simple type definition, ... 
	    * the ·normalized value· of the element information item is
	    * ·valid· with respect to that simple type definition as
	    * defined by String Valid (§3.14.4)."
	    */
	    ret = xmlSchemaVCheckINodeDataType(vctxt,
		inode, inode->typeDef->contentTypeDef, inode->value);
	}	
	if (ret != 0) {
	    if (ret < 0) {
		VERROR_INT("xmlSchemaValidatorPopElem",
		    "calling xmlSchemaVCheckCVCSimpleType()");
		goto internal_error;
	    }
	    goto end_elem;
	}
	/*
	* 5.2.2 If there is a fixed {value constraint} and clause 3.2 has 
	* not applied, all of the following must be true:
	*/
	if ((inode->decl->value != NULL) &&
	    (inode->decl->flags & XML_SCHEMAS_ELEM_FIXED)) {

	    /*
	    * TODO: We will need a computed value, when comparison is
	    * done on computed values.
	    */
	    /*
	    * 5.2.2.1 The element information item must have no element 
	    * information item [children].
	    */
	    if (inode->flags &
		    XML_SCHEMA_ELEM_INFO_HAS_ELEM_CONTENT) {
		ret = XML_SCHEMAV_CVC_ELT_5_2_2_1;
		VERROR(ret, NULL,
		    "The content must not containt element nodes since "
		    "there is a fixed value constraint");
		goto end_elem;
	    } else {
		/*
		* 5.2.2.2 The appropriate case among the following must 
		* be true:
		*/		
		if (HAS_MIXED_CONTENT(inode->typeDef)) {
		    /*
		    * 5.2.2.2.1 If the {content type} of the ·actual type 
		    * definition· is mixed, then the *initial value* of the 
		    * item must match the canonical lexical representation 
		    * of the {value constraint} value.
		    *
		    * ... the *initial value* of an element information 
		    * item is the string composed of, in order, the 
		    * [character code] of each character information item in 
		    * the [children] of that element information item.
		    */		   
		    if (! xmlStrEqual(inode->value, inode->decl->value)){
			/* 
			* VAL TODO: Report invalid & expected values as well.
			* VAL TODO: Implement the canonical stuff.
			*/
			ret = XML_SCHEMAV_CVC_ELT_5_2_2_2_1;
			xmlSchemaCustomErr((xmlSchemaAbstractCtxtPtr) vctxt, 
			    ret, NULL, NULL,
			    "The initial value '%s' does not match the fixed "
			    "value constraint '%s'",
			    inode->value, inode->decl->value);
			goto end_elem;
		    }
		} else if (HAS_SIMPLE_CONTENT(inode->typeDef)) {
		    /*
		    * 5.2.2.2.2 If the {content type} of the ·actual type 
		    * definition· is a simple type definition, then the 
		    * *actual value* of the item must match the canonical 
		    * lexical representation of the {value constraint} value.
		    */
		    /*
		    * VAL TODO: *actual value* is the normalized value, impl.
		    *           this.
		    * VAL TODO: Report invalid & expected values as well.
		    * VAL TODO: Implement a comparison with the computed values.
		    */
		    if (! xmlStrEqual(inode->value,
			    inode->decl->value)) {
			ret = XML_SCHEMAV_CVC_ELT_5_2_2_2_2;
			xmlSchemaCustomErr((xmlSchemaAbstractCtxtPtr) vctxt,
			    ret, NULL, NULL,
			    "The actual value '%s' does not match the fixed "
			    "value constraint '%s'", 
			    inode->value,
			    inode->decl->value);
			goto end_elem;
		    }		    
		}
	    }	    
	}
    }
    
end_elem:
    if (vctxt->depth < 0) {
	/* TODO: raise error? */
	return (0);
    }
    if (vctxt->depth == vctxt->skipDepth)
	vctxt->skipDepth = -1;
    /*
    * Evaluate the history of XPath state objects.
    */    
    if (xmlSchemaXPathProcessHistory(vctxt, vctxt->depth) == -1)
	goto internal_error;
    /*
    * TODO: 6 The element information item must be ·valid· with respect to each of 
    * the {identity-constraint definitions} as per Identity-constraint 
    * Satisfied (§3.11.4).
    */
    /*
    * Validate IDC keyrefs.
    */
    if (xmlSchemaCheckCVCIDCKeyRef(vctxt) == -1)
	goto internal_error;
    /*
    * Merge/free the IDC table.
    */
    if (inode->idcTable != NULL) {
#ifdef DEBUG_IDC
	xmlSchemaDebugDumpIDCTable(stdout,
	    inode->nsName,
	    inode->localName,
	    inode->idcTable);
#endif
	if (vctxt->depth > 0) {
	    /*
	    * Merge the IDC node table with the table of the parent node.
	    */
	    if (xmlSchemaBubbleIDCNodeTables(vctxt) == -1)
		goto internal_error;
	}	
    }
    /*
    * Clear the current ielem.
    * VAL TODO: Don't free the PSVI IDC tables if they are
    * requested for the PSVI.
    */
    xmlSchemaClearElemInfo(inode);
    /*
    * Skip further processing if we are on the validation root.
    */
    if (vctxt->depth == 0) {
	vctxt->depth--;
	vctxt->inode = NULL;
	return (0);
    }
    /*
    * Reset the bubbleDepth if needed.
    */
    if (vctxt->aidcs != NULL) {
	xmlSchemaIDCAugPtr aidc = vctxt->aidcs;
	do {
	    if (aidc->bubbleDepth == vctxt->depth) {
		/*
		* A bubbleDepth of a key/unique IDC matches the current
		* depth, this means that we are leaving the scope of the
		* top-most keyref IDC.
		*/
		aidc->bubbleDepth = -1;
	    }
	    aidc = aidc->next;
	} while (aidc != NULL);
    }
    vctxt->depth--;        
    vctxt->inode = vctxt->elemInfos[vctxt->depth];
    /*
    * VAL TODO: 7 If the element information item is the ·validation root·, it must be 
    * ·valid· per Validation Root Valid (ID/IDREF) (§3.3.4).
    */
    return (ret);

internal_error:
    vctxt->err = -1;
    return (-1);
}

/*
* 3.4.4 Complex Type Definition Validation Rules
* Validation Rule: Element Locally Valid (Complex Type) (cvc-complex-type)
*/
static int
xmlSchemaValidateChildElem(xmlSchemaValidCtxtPtr vctxt)
{
    xmlSchemaNodeInfoPtr pielem;
    xmlSchemaTypePtr ptype;
    int ret = 0;

    if (vctxt->depth <= 0) {
	VERROR_INT("xmlSchemaValidateChildElem",
	    "not intended for the validation root");
	return (-1);
    }
    pielem = vctxt->elemInfos[vctxt->depth -1];
    if (pielem->flags & XML_SCHEMA_ELEM_INFO_EMPTY)
	pielem->flags ^= XML_SCHEMA_ELEM_INFO_EMPTY;
    /*
    * Handle 'nilled' elements.
    */
    if (INODE_NILLED(pielem)) {
	/*
	* SPEC (cvc-elt) (3.3.4) : (3.2.1)
	*/
	ACTIVATE_PARENT_ELEM;
	ret = XML_SCHEMAV_CVC_ELT_3_2_1;
	VERROR(ret, NULL,
	    "Neither character nor element content is allowed, "
	    "because the element was 'nilled'");
	ACTIVATE_ELEM;
	goto unexpected_elem;
    }

    ptype = pielem->typeDef;

    if (ptype->builtInType == XML_SCHEMAS_ANYTYPE) {
	/*
	* Workaround for "anyType": we have currently no content model
	* assigned for "anyType", so handle it explicitely.
	* "anyType" has an unbounded, lax "any" wildcard.
	*/
	vctxt->inode->decl = xmlSchemaGetElem(vctxt->schema,
	    vctxt->inode->localName,
	    vctxt->inode->nsName);

	if (vctxt->inode->decl == NULL) {
	    xmlSchemaAttrInfoPtr iattr;
	    /*
	    * Process "xsi:type".
	    * SPEC (cvc-assess-elt) (1.2.1.2.1) - (1.2.1.2.3)
	    */
	    iattr = xmlSchemaGetMetaAttrInfo(vctxt,
		XML_SCHEMA_ATTR_INFO_META_XSI_TYPE);
	    if (iattr != NULL) {
		ret = xmlSchemaProcessXSIType(vctxt, iattr,
		    &(vctxt->inode->typeDef), NULL);
		if (ret != 0) {
		    if (ret == -1) {
			VERROR_INT("xmlSchemaValidateChildElem",
			    "calling xmlSchemaProcessXSIType() to "
			    "process the attribute 'xsi:nil'");
			return (-1);
		    }
		    return (ret);
		}
	    } else {
		 /*
		 * Fallback to "anyType".
		 *
		 * SPEC (cvc-assess-elt)
		 * "If the item cannot be ·strictly assessed·, [...]
		 * an element information item's schema validity may be laxly
		 * assessed if its ·context-determined declaration· is not
		 * skip by ·validating· with respect to the ·ur-type
		 * definition· as per Element Locally Valid (Type) (§3.3.4)."
		*/
		vctxt->inode->typeDef =
		    xmlSchemaGetBuiltInType(XML_SCHEMAS_ANYTYPE);
	    }
	}
	return (0);
    }

    switch (ptype->contentType) {
	case XML_SCHEMA_CONTENT_EMPTY:
	    /*
	    * SPEC (2.1) "If the {content type} is empty, then the
	    * element information item has no character or element
	    * information item [children]."
	    */
	    ACTIVATE_PARENT_ELEM
	    ret = XML_SCHEMAV_CVC_COMPLEX_TYPE_2_1;
	    VERROR(ret, NULL,
		"Element content is not allowed, "
		"because the content type is empty");
	    ACTIVATE_ELEM
	    goto unexpected_elem;
	    break;

	case XML_SCHEMA_CONTENT_MIXED:
        case XML_SCHEMA_CONTENT_ELEMENTS: {
	    xmlRegExecCtxtPtr regexCtxt;
	    xmlChar *values[10];
	    int terminal, nbval = 10, nbneg;

	    /* VAL TODO: Optimized "anyType" validation.*/

	    if (ptype->contModel == NULL) {
		VERROR_INT("xmlSchemaValidateChildElem",
		    "type has elem content but no content model");
		return (-1);
	    }
	    /*
	    * Safety belf for evaluation if the cont. model was already
	    * examined to be invalid.
	    */
	    if (pielem->flags & XML_SCHEMA_ELEM_INFO_ERR_BAD_CONTENT) {
		VERROR_INT("xmlSchemaValidateChildElem",
		    "validating elem, but elem content is already invalid");
		return (-1);
	    }

	    regexCtxt = pielem->regexCtxt;
	    if (regexCtxt == NULL) {
		/*
		* Create the regex context.
		*/
		regexCtxt = xmlRegNewExecCtxt(ptype->contModel,
		    (xmlRegExecCallbacks) xmlSchemaVContentModelCallback,
		    vctxt);
		if (regexCtxt == NULL) {
		    VERROR_INT("xmlSchemaValidateChildElem",
			"failed to create a regex context");
		    return (-1);
		}
		pielem->regexCtxt = regexCtxt;
#ifdef DEBUG_AUTOMATA
		xmlGenericError(xmlGenericErrorContext, "AUTOMATA create on '%s'\n",
		    pielem->localName);
#endif
	    }

	    /*
	    * SPEC (2.4) "If the {content type} is element-only or mixed,
	    * then the sequence of the element information item's
	    * element information item [children], if any, taken in
	    * order, is ·valid· with respect to the {content type}'s
	    * particle, as defined in Element Sequence Locally Valid
	    * (Particle) (§3.9.4)."
	    */
	    ret = xmlRegExecPushString2(regexCtxt,
		vctxt->inode->localName,
		vctxt->inode->nsName,
		vctxt->inode);
#ifdef DEBUG_AUTOMATA
	    if (ret < 0)
		xmlGenericError(xmlGenericErrorContext,
		"AUTOMATON push ERROR for '%s' on '%s'\n",
		vctxt->inode->localName, pielem->localName);
	    else
		xmlGenericError(xmlGenericErrorContext,
		"AUTOMATON push OK for '%s' on '%s'\n",
		vctxt->inode->localName, pielem->localName);
#endif
	    if (vctxt->err == XML_SCHEMAV_INTERNAL) {
		VERROR_INT("xmlSchemaValidateChildElem",
		    "calling xmlRegExecPushString2()");
		return (-1);
	    }
	    if (ret < 0) {
		xmlRegExecErrInfo(regexCtxt, NULL, &nbval, &nbneg,
		    &values[0], &terminal);
		xmlSchemaComplexTypeErr((xmlSchemaAbstractCtxtPtr) vctxt,
		    XML_SCHEMAV_ELEMENT_CONTENT, NULL,NULL,
		    "This element is not expected",
		    nbval, nbneg, values);
		ret = vctxt->err;
		goto unexpected_elem;
	    } else
		ret = 0;
	}
	    break;
	case XML_SCHEMA_CONTENT_SIMPLE:
	case XML_SCHEMA_CONTENT_BASIC:
	    ACTIVATE_PARENT_ELEM
	    if (IS_COMPLEX_TYPE(ptype)) {
		/*
		* SPEC (cvc-complex-type) (2.2)
		* "If the {content type} is a simple type definition, then
		* the element information item has no element information
		* item [children], ..."
		*/
		ret = XML_SCHEMAV_CVC_COMPLEX_TYPE_2_2;
		VERROR(ret, NULL, "Element content is not allowed, "
		    "because the content type is a simple type definition");
	    } else {
		/*
		* SPEC (cvc-type) (3.1.2) "The element information item must
		* have no element information item [children]."
		*/
		ret = XML_SCHEMAV_CVC_TYPE_3_1_2;
		VERROR(ret, NULL, "Element content is not allowed, "
		    "because the type definition is simple");
	    }
	    ACTIVATE_ELEM
	    ret = vctxt->err;
	    goto unexpected_elem;
	    break;

	default:
	    break;
    }
    return (ret);
unexpected_elem:
    /*
    * Pop this element and set the skipDepth to skip
    * all further content of the parent element.
    */
    vctxt->skipDepth = vctxt->depth;
    vctxt->inode->flags |= XML_SCHEMA_NODE_INFO_ERR_NOT_EXPECTED;
    pielem->flags |= XML_SCHEMA_ELEM_INFO_ERR_BAD_CONTENT;
    return (ret);
}

#define XML_SCHEMA_PUSH_TEXT_PERSIST 1
#define XML_SCHEMA_PUSH_TEXT_CREATED 2
#define XML_SCHEMA_PUSH_TEXT_VOLATILE 3

static int
xmlSchemaVPushText(xmlSchemaValidCtxtPtr vctxt,
		  int nodeType, const xmlChar *value, int len,
		  int mode, int *consumed)
{
    /*
    * Unfortunately we have to duplicate the text sometimes.
    * OPTIMIZE: Maybe we could skip it, if:
    *   1. content type is simple
    *   2. whitespace is "collapse"
    *   3. it consists of whitespace only
    *
    * Process character content.
    */
    if (consumed != NULL)
	*consumed = 0;
    if (INODE_NILLED(vctxt->inode)) {
	/* 
	* SPEC cvc-elt (3.3.4 - 3.2.1)
	* "The element information item must have no character or
	* element information item [children]."
	*/
	VERROR(XML_SCHEMAV_CVC_ELT_3_2_1, NULL,
	    "Neither character nor element content is allowed "
	    "because the element is 'nilled'");
	return (vctxt->err);
    }
    /*
    * SPEC (2.1) "If the {content type} is empty, then the
    * element information item has no character or element
    * information item [children]."
    */
    if (vctxt->inode->typeDef->contentType ==
	    XML_SCHEMA_CONTENT_EMPTY) {    
	VERROR(XML_SCHEMAV_CVC_COMPLEX_TYPE_2_1, NULL,
	    "Character content is not allowed, "
	    "because the content type is empty");
	return (vctxt->err);
    }

    if (vctxt->inode->typeDef->contentType ==
	    XML_SCHEMA_CONTENT_ELEMENTS) {
	if ((nodeType != XML_TEXT_NODE) ||
	    (! xmlSchemaIsBlank((xmlChar *) value, len))) {
	    /* 
	    * SPEC cvc-complex-type (2.3) 
	    * "If the {content type} is element-only, then the 
	    * element information item has no character information 
	    * item [children] other than those whose [character 
	    * code] is defined as a white space in [XML 1.0 (Second 
	    * Edition)]."
	    */
	    VERROR(XML_SCHEMAV_CVC_COMPLEX_TYPE_2_3, NULL,
		"Character content other than whitespace is not allowed "
		"because the content type is 'element-only'");
	    return (vctxt->err);
	}
	return (0);
    }
    
    if ((value == NULL) || (value[0] == 0))
	return (0);
    /*
    * Save the value.
    * NOTE that even if the content type is *mixed*, we need the
    * *initial value* for default/fixed value constraints.
    */
    if ((vctxt->inode->typeDef->contentType == XML_SCHEMA_CONTENT_MIXED) &&
	((vctxt->inode->decl == NULL) ||
	(vctxt->inode->decl->value == NULL)))
	return (0);
    
    if (vctxt->inode->value == NULL) {
	/*
	* Set the value.
	*/
	switch (mode) {
	    case XML_SCHEMA_PUSH_TEXT_PERSIST:
		/*
		* When working on a tree.
		*/
		vctxt->inode->value = value;
		break;
	    case XML_SCHEMA_PUSH_TEXT_CREATED:
		/*
		* When working with the reader.
		* The value will be freed by the element info.
		*/
		vctxt->inode->value = value;
		if (consumed != NULL)
		    *consumed = 1;
		vctxt->inode->flags |=
		    XML_SCHEMA_NODE_INFO_FLAG_OWNED_VALUES;
		break;
	    case XML_SCHEMA_PUSH_TEXT_VOLATILE:
		/*
		* When working with SAX.
		* The value will be freed by the element info.
		*/
		if (len != -1)
		    vctxt->inode->value = BAD_CAST xmlStrndup(value, len);
		else
		    vctxt->inode->value = BAD_CAST xmlStrdup(value);
		vctxt->inode->flags |=
		    XML_SCHEMA_NODE_INFO_FLAG_OWNED_VALUES;
		break;
	    default:
		break;
	}
    } else {	
	/*
	* Concat the value.
	*/	
	if (vctxt->inode->flags & XML_SCHEMA_NODE_INFO_FLAG_OWNED_VALUES) {
	    vctxt->inode->value = BAD_CAST xmlStrncat(
		(xmlChar *) vctxt->inode->value, value, len);
	} else {
	    vctxt->inode->value =
		BAD_CAST xmlStrncatNew(vctxt->inode->value, value, len);
	    vctxt->inode->flags |= XML_SCHEMA_NODE_INFO_FLAG_OWNED_VALUES;
	}
    }	

    return (0);
}

static int
xmlSchemaValidateElem(xmlSchemaValidCtxtPtr vctxt)
{
    int ret = 0;

    if ((vctxt->skipDepth != -1) &&
	(vctxt->depth >= vctxt->skipDepth)) {
	VERROR_INT("xmlSchemaValidateElem",
	    "in skip-state");
	goto internal_error;
    }
    if (vctxt->xsiAssemble) {
	if (xmlSchemaAssembleByXSI(vctxt) == -1)
	    goto internal_error;
    }
    if (vctxt->depth > 0) {
	/*
	* Validate this element against the content model
	* of the parent.
	*/
	ret = xmlSchemaValidateChildElem(vctxt);
	if (ret != 0) {
	    if (ret < 0) {
		VERROR_INT("xmlSchemaValidateElem",
		    "calling xmlSchemaStreamValidateChildElement()");
		goto internal_error;
	    }
	    goto exit;
	}
	if (vctxt->depth == vctxt->skipDepth)
	    goto exit;
	if ((vctxt->inode->decl == NULL) &&
	    (vctxt->inode->typeDef == NULL)) {
	    VERROR_INT("xmlSchemaValidateElem",
		"the child element was valid but neither the "
		"declaration nor the type was set");
	    goto internal_error;
	}
    } else {
	/*
	* Get the declaration of the validation root.
	*/
	vctxt->inode->decl = xmlSchemaGetElem(vctxt->schema,
	    vctxt->inode->localName,
	    vctxt->inode->nsName);
	if (vctxt->inode->decl == NULL) {
	    ret = XML_SCHEMAV_CVC_ELT_1;
	    VERROR(ret, NULL,
		"No matching global declaration available "
		"for the validation root");
	    goto exit;
	}
    }

    if (vctxt->inode->decl == NULL)
	goto type_validation;

    if (vctxt->inode->decl->type == XML_SCHEMA_TYPE_ANY) {
	int skip;
	/*
	* Wildcards.
	*/
	ret = xmlSchemaValidateElemWildcard(vctxt, &skip);
	if (ret != 0) {
	    if (ret < 0) {
		VERROR_INT("xmlSchemaValidateElem",
		    "calling xmlSchemaValidateElemWildcard()");
		goto internal_error;
	    }
	    goto exit;
	}
	if (skip) {
	    vctxt->skipDepth = vctxt->depth;
	    goto exit;
	}
	/*
	* The declaration might be set by the wildcard validation,
	* when the processContents is "lax" or "strict".
	*/
	if (vctxt->inode->decl->type != XML_SCHEMA_TYPE_ELEMENT) {
	    /*
	    * Clear the "decl" field to not confuse further processing.
	    */
	    vctxt->inode->decl = NULL;
	    goto type_validation;
	}
    }
    /*
    * Validate against the declaration.
    */
    ret = xmlSchemaValidateElemDecl(vctxt);
    if (ret != 0) {
	if (ret < 0) {
	    VERROR_INT("xmlSchemaValidateElem",
		"calling xmlSchemaValidateElemDecl()");
	    goto internal_error;
	}
	goto exit;
    }
    /*
    * Validate against the type definition.
    */
type_validation:

    if (vctxt->inode->typeDef == NULL) {
	vctxt->inode->flags |= XML_SCHEMA_NODE_INFO_ERR_BAD_TYPE;
	ret = XML_SCHEMAV_CVC_TYPE_1;
    	VERROR(ret, NULL,
    	    "The type definition is absent");
	goto exit;
    }    
    if (vctxt->inode->typeDef->flags & XML_SCHEMAS_TYPE_ABSTRACT) {
	vctxt->inode->flags |= XML_SCHEMA_NODE_INFO_ERR_BAD_TYPE;
	ret = XML_SCHEMAV_CVC_TYPE_2;
    	    VERROR(ret, NULL,
    	    "The type definition is abstract");	
	goto exit;
    }
    /*
    * Evaluate IDCs. Do it here, since new IDC matchers are registered
    * during validation against the declaration. This must be done
    * _before_ attribute validation.
    */
    ret = xmlSchemaXPathEvaluate(vctxt, XML_ELEMENT_NODE);
    if (ret == -1) {
	VERROR_INT("xmlSchemaValidateElem",
	    "calling xmlSchemaXPathEvaluate()");
	goto internal_error;
    }
    /*
    * Validate attributes.
    */
    if (IS_COMPLEX_TYPE(vctxt->inode->typeDef)) {
	if ((vctxt->nbAttrInfos != 0) ||
	    (vctxt->inode->typeDef->attributeUses != NULL)) {

	    ret = xmlSchemaVAttributesComplex(vctxt);
	}
    } else if (vctxt->nbAttrInfos != 0) {

	ret = xmlSchemaVAttributesSimple(vctxt);
    }
    /*
    * Clear registered attributes.
    */
    if (vctxt->nbAttrInfos != 0)
	xmlSchemaClearAttrInfos(vctxt);
    if (ret == -1) {
	VERROR_INT("xmlSchemaValidateElem",
	    "calling attributes validation");
	goto internal_error;
    }
    /*
    * Don't return an error if attributes are invalid on purpose.
    */
    ret = 0;

exit:
    if (ret != 0)
	vctxt->skipDepth = vctxt->depth;
    return (ret);
internal_error:
    return (-1);
}

#ifdef XML_SCHEMA_READER_ENABLED
static int
xmlSchemaVReaderWalk(xmlSchemaValidCtxtPtr vctxt)
{
    const int WHTSP = 13, SIGN_WHTSP = 14, END_ELEM = 15;
    int depth, nodeType, ret = 0, consumed;
    xmlSchemaNodeInfoPtr ielem;

    vctxt->depth = -1;
    ret = xmlTextReaderRead(vctxt->reader);
    /*
    * Move to the document element.
    */
    while (ret == 1) {
	nodeType = xmlTextReaderNodeType(vctxt->reader);
	if (nodeType == XML_ELEMENT_NODE)
	    goto root_found;
	ret = xmlTextReaderRead(vctxt->reader);
    }
    goto exit;

root_found:

    do {
	depth = xmlTextReaderDepth(vctxt->reader);
	nodeType = xmlTextReaderNodeType(vctxt->reader);

	if (nodeType == XML_ELEMENT_NODE) {
	    
	    vctxt->depth++;
	    if (xmlSchemaValidatorPushElem(vctxt) == -1) {
		VERROR_INT("xmlSchemaVReaderWalk",
		    "calling xmlSchemaValidatorPushElem()");
		goto internal_error;
	    }
	    ielem = vctxt->inode;
	    ielem->localName = xmlTextReaderLocalName(vctxt->reader);
	    ielem->nsName = xmlTextReaderNamespaceUri(vctxt->reader);
	    ielem->flags |= XML_SCHEMA_NODE_INFO_FLAG_OWNED_NAMES;
	    /*
	    * Is the element empty?
	    */
	    ret = xmlTextReaderIsEmptyElement(vctxt->reader);
	    if (ret == -1) {
		VERROR_INT("xmlSchemaVReaderWalk",
		    "calling xmlTextReaderIsEmptyElement()");
		goto internal_error;
	    }
	    if (ret) {
		ielem->flags |= XML_SCHEMA_ELEM_INFO_EMPTY;
	    }
	    /*
	    * Register attributes.
	    */
	    vctxt->nbAttrInfos = 0;
	    ret = xmlTextReaderMoveToFirstAttribute(vctxt->reader);
	    if (ret == -1) {
		VERROR_INT("xmlSchemaVReaderWalk",
		    "calling xmlTextReaderMoveToFirstAttribute()");
		goto internal_error;
	    }
	    if (ret == 1) {
		do {
		    /*
		    * VAL TODO: How do we know that the reader works on a
		    * node tree, to be able to pass a node here?
		    */
		    if (xmlSchemaValidatorPushAttribute(vctxt, NULL,
			(const xmlChar *) xmlTextReaderLocalName(vctxt->reader),
			xmlTextReaderNamespaceUri(vctxt->reader), 1,
			xmlTextReaderValue(vctxt->reader), 1) == -1) {

			VERROR_INT("xmlSchemaVReaderWalk",
			    "calling xmlSchemaValidatorPushAttribute()");
			goto internal_error;
		    }
		    ret = xmlTextReaderMoveToNextAttribute(vctxt->reader);
		    if (ret == -1) {
			VERROR_INT("xmlSchemaVReaderWalk",
			    "calling xmlTextReaderMoveToFirstAttribute()");
			goto internal_error;
		    }
		} while (ret == 1);
		/*
		* Back to element position.
		*/
		ret = xmlTextReaderMoveToElement(vctxt->reader);
		if (ret == -1) {
		    VERROR_INT("xmlSchemaVReaderWalk",
			"calling xmlTextReaderMoveToElement()");
		    goto internal_error;
		}
	    }
	    /*
	    * Validate the element.
	    */
	    ret= xmlSchemaValidateElem(vctxt);
	    if (ret != 0) {
		if (ret == -1) {
		    VERROR_INT("xmlSchemaVReaderWalk",
			"calling xmlSchemaValidateElem()");
		    goto internal_error;
		}
		goto exit;
	    }
	    if (vctxt->depth == vctxt->skipDepth) {
		int curDepth;
		/*
		* Skip all content.
		*/
		if ((ielem->flags & XML_SCHEMA_ELEM_INFO_EMPTY) == 0) {
		    ret = xmlTextReaderRead(vctxt->reader);
		    curDepth = xmlTextReaderDepth(vctxt->reader);
		    while ((ret == 1) && (curDepth != depth)) {
			ret = xmlTextReaderRead(vctxt->reader);
			curDepth = xmlTextReaderDepth(vctxt->reader);
		    }
		    if (ret < 0) {
			/*
			* VAL TODO: A reader error occured; what to do here?
			*/
			ret = 1;
			goto exit;
		    }
		}
		goto leave_elem;
	    }
	    /*
	    * READER VAL TODO: Is an END_ELEM really never called
	    * if the elem is empty?
	    */
	    if (ielem->flags & XML_SCHEMA_ELEM_INFO_EMPTY)
		goto leave_elem;
	} else if (nodeType == END_ELEM) {
	    /*
	    * Process END of element.
	    */
leave_elem:
	    ret = xmlSchemaValidatorPopElem(vctxt);
	    if (ret != 0) {
		if (ret < 0) {
		    VERROR_INT("xmlSchemaVReaderWalk",
			"calling xmlSchemaValidatorPopElem()");
		    goto internal_error;
		}
		goto exit;
	    }
	    if (vctxt->depth >= 0)
		ielem = vctxt->inode;
	    else
		ielem = NULL;
	} else if ((nodeType == XML_TEXT_NODE) ||
	    (nodeType == XML_CDATA_SECTION_NODE) ||
	    (nodeType == WHTSP) ||
	    (nodeType == SIGN_WHTSP)) {
	    /*
	    * Process character content.
	    */
	    xmlChar *value;

	    if ((nodeType == WHTSP) || (nodeType == SIGN_WHTSP))
		nodeType = XML_TEXT_NODE;

	    value = xmlTextReaderValue(vctxt->reader);
	    ret = xmlSchemaVPushText(vctxt, nodeType, BAD_CAST value,
		-1, XML_SCHEMA_PUSH_TEXT_CREATED, &consumed);
	    if (! consumed)
		xmlFree(value);
	    if (ret == -1) {
		VERROR_INT("xmlSchemaVReaderWalk",
		    "calling xmlSchemaVPushText()");
		goto internal_error;
	    }
	} else if ((nodeType == XML_ENTITY_NODE) ||
	    (nodeType == XML_ENTITY_REF_NODE)) {
	    /*
	    * VAL TODO: What to do with entities?
	    */
	    TODO
	}
	/*
	* Read next node.
	*/
	ret = xmlTextReaderRead(vctxt->reader);
    } while (ret == 1);

exit:
    return (ret);
internal_error:
    return (-1);
}
#endif

/************************************************************************
 * 									*
 * 			SAX validation handlers				*
 * 									*
 ************************************************************************/

#ifdef XML_SCHEMA_SAX_ENABLED
/*
* Process text content.
*/
static void
xmlSchemaSAXHandleText(void *ctx, 
		       const xmlChar * ch, 
		       int len)
{
    xmlSchemaValidCtxtPtr vctxt = (xmlSchemaValidCtxtPtr) ctx;

    if (vctxt->depth < 0)
	return;
    if ((vctxt->skipDepth != -1) && (vctxt->depth >= vctxt->skipDepth))
	return;
    if (vctxt->inode->flags & XML_SCHEMA_ELEM_INFO_EMPTY)
	vctxt->inode->flags ^= XML_SCHEMA_ELEM_INFO_EMPTY;
    if (xmlSchemaVPushText(vctxt, XML_TEXT_NODE, ch, len,
	XML_SCHEMA_PUSH_TEXT_VOLATILE, NULL) == -1) {
	VERROR_INT("xmlSchemaSAXHandleCDataSection",
	    "calling xmlSchemaVPushText()");
	vctxt->err = -1;
	xmlStopParser(vctxt->parserCtxt);
    }
}

/*
* Process CDATA content.
*/
static void
xmlSchemaSAXHandleCDataSection(void *ctx, 
			     const xmlChar * ch, 
			     int len)
{   
    xmlSchemaValidCtxtPtr vctxt = (xmlSchemaValidCtxtPtr) ctx;

    if (vctxt->depth < 0)
	return;
    if ((vctxt->skipDepth != -1) && (vctxt->depth >= vctxt->skipDepth))
	return;
    if (vctxt->inode->flags & XML_SCHEMA_ELEM_INFO_EMPTY)
	vctxt->inode->flags ^= XML_SCHEMA_ELEM_INFO_EMPTY;
    if (xmlSchemaVPushText(vctxt, XML_CDATA_SECTION_NODE, ch, len,
	XML_SCHEMA_PUSH_TEXT_VOLATILE, NULL) == -1) {
	VERROR_INT("xmlSchemaSAXHandleCDataSection",
	    "calling xmlSchemaVPushText()");
	vctxt->err = -1;
	xmlStopParser(vctxt->parserCtxt);
    }
}

static void
xmlSchemaSAXHandleReference(void *ctx ATTRIBUTE_UNUSED,
			    const xmlChar * name ATTRIBUTE_UNUSED)
{
    xmlSchemaValidCtxtPtr vctxt = (xmlSchemaValidCtxtPtr) ctx;

    if (vctxt->depth < 0)
	return;
    if ((vctxt->skipDepth != -1) && (vctxt->depth >= vctxt->skipDepth))
	return;
    /* SAX VAL TODO: What to do here? */
    TODO
}

static void
xmlSchemaSAXHandleStartElementNs(void *ctx,
				 const xmlChar * localname, 
				 const xmlChar * prefix ATTRIBUTE_UNUSED, 
				 const xmlChar * URI, 
				 int nb_namespaces, 
				 const xmlChar ** namespaces, 
				 int nb_attributes, 
				 int nb_defaulted ATTRIBUTE_UNUSED, 
				 const xmlChar ** attributes)
{  
    xmlSchemaValidCtxtPtr vctxt = (xmlSchemaValidCtxtPtr) ctx;
    int ret;
    xmlSchemaNodeInfoPtr ielem;
    int i, j;
    
    /*
    * SAX VAL TODO: What to do with nb_defaulted?
    */
    /*
    * Skip elements if inside a "skip" wildcard or invalid.
    */
    vctxt->depth++;
    if ((vctxt->skipDepth != -1) && (vctxt->depth >= vctxt->skipDepth))
	return;
    /*
    * Push the element.
    */
    if (xmlSchemaValidatorPushElem(vctxt) == -1) {
	VERROR_INT("xmlSchemaSAXHandleStartElementNs",
	    "calling xmlSchemaValidatorPushElem()");
	goto internal_error;
    }
    ielem = vctxt->inode;
    ielem->localName = localname;
    ielem->nsName = URI;
    ielem->flags |= XML_SCHEMA_ELEM_INFO_EMPTY;
    /*
    * Register namespaces on the elem info.
    */    
    if (nb_namespaces != 0) {
	/*
	* Although the parser builds its own namespace list,
	* we have no access to it, so we'll use an own one.
	*/
        for (i = 0, j = 0; i < nb_namespaces; i++, j += 2) {	    
	    /*
	    * Store prefix and namespace name.
	    */	   
	    if (ielem->nsBindings == NULL) {
		ielem->nsBindings =
		    (const xmlChar **) xmlMalloc(10 *
			sizeof(const xmlChar *));
		if (ielem->nsBindings == NULL) {
		    xmlSchemaVErrMemory(vctxt,
			"allocating namespace bindings for SAX validation",
			NULL);
		    goto internal_error;
		}
		ielem->nbNsBindings = 0;
		ielem->sizeNsBindings = 5;
	    } else if (ielem->sizeNsBindings <= ielem->nbNsBindings) {
		ielem->sizeNsBindings *= 2;
		ielem->nsBindings =
		    (const xmlChar **) xmlRealloc(
			(void *) ielem->nsBindings,
			ielem->sizeNsBindings * 2 * sizeof(const xmlChar *));
		if (ielem->nsBindings == NULL) {
		    xmlSchemaVErrMemory(vctxt,
			"re-allocating namespace bindings for SAX validation",
			NULL);
		    goto internal_error;
		}
	    }

	    ielem->nsBindings[ielem->nbNsBindings * 2] = namespaces[j];
	    if (namespaces[j+1][0] == 0) {
		/*
		* Handle xmlns="".
		*/
		ielem->nsBindings[ielem->nbNsBindings * 2 + 1] = NULL;
	    } else
		ielem->nsBindings[ielem->nbNsBindings * 2 + 1] =
		    namespaces[j+1];
	    ielem->nbNsBindings++;	    	    
	}
    }
    /*
    * Register attributes.
    * SAX VAL TODO: We are not adding namespace declaration
    * attributes yet.
    */
    if (nb_attributes != 0) {
	xmlChar *value;

        for (j = 0, i = 0; i < nb_attributes; i++, j += 5) {
	    /*
	    * Duplicate the value.
	    */	 
	    value = xmlStrndup(attributes[j+3],
		attributes[j+4] - attributes[j+3]);
	    ret = xmlSchemaValidatorPushAttribute(vctxt,
		NULL, attributes[j], attributes[j+2], 0,
		value, 1);
	    if (ret == -1) {
		VERROR_INT("xmlSchemaSAXHandleStartElementNs",
		    "calling xmlSchemaValidatorPushAttribute()");
		goto internal_error;
	    }
	}
    }
    /*
    * Validate the element.
    */
    ret = xmlSchemaValidateElem(vctxt);
    if (ret != 0) {
	if (ret == -1) {
	    VERROR_INT("xmlSchemaSAXHandleStartElementNs",
		"calling xmlSchemaValidateElem()");
	    goto internal_error;
	}
	goto exit;
    }    

exit:
    return;
internal_error:
    vctxt->err = -1;
    xmlStopParser(vctxt->parserCtxt);
    return;
}

static void
xmlSchemaSAXHandleEndElementNs(void *ctx,
			       const xmlChar * localname ATTRIBUTE_UNUSED,
			       const xmlChar * prefix ATTRIBUTE_UNUSED,
			       const xmlChar * URI ATTRIBUTE_UNUSED)
{
    xmlSchemaValidCtxtPtr vctxt = (xmlSchemaValidCtxtPtr) ctx;
    int res;

    /*
    * Skip elements if inside a "skip" wildcard or if invalid.
    */
    if (vctxt->skipDepth != -1) {
	if (vctxt->depth > vctxt->skipDepth) {
	    vctxt->depth--;
	    return;
	} else
	    vctxt->skipDepth = -1;
    }
    /*
    * SAX VAL TODO: Just a temporary check.
    */
    if ((!xmlStrEqual(vctxt->inode->localName, localname)) ||
	(!xmlStrEqual(vctxt->inode->nsName, URI))) {
	VERROR_INT("xmlSchemaSAXHandleEndElementNs",
	    "elem pop mismatch");
    }
    res = xmlSchemaValidatorPopElem(vctxt);
    if (res != 0) {
	if (res < 0) {
	    VERROR_INT("xmlSchemaSAXHandleEndElementNs",
		"calling xmlSchemaValidatorPopElem()");
	    goto internal_error;
	}
	goto exit;
    }
exit:
    return;
internal_error:
    vctxt->err = -1;
    xmlStopParser(vctxt->parserCtxt);
    return;
}
#endif

/************************************************************************
 * 									*
 * 			Validation interfaces				*
 * 									*
 ************************************************************************/

/**
 * xmlSchemaNewValidCtxt:
 * @schema:  a precompiled XML Schemas
 *
 * Create an XML Schemas validation context based on the given schema.
 *
 * Returns the validation context or NULL in case of error
 */
xmlSchemaValidCtxtPtr
xmlSchemaNewValidCtxt(xmlSchemaPtr schema)
{
    xmlSchemaValidCtxtPtr ret;

    ret = (xmlSchemaValidCtxtPtr) xmlMalloc(sizeof(xmlSchemaValidCtxt));
    if (ret == NULL) {
        xmlSchemaVErrMemory(NULL, "allocating validation context", NULL);
        return (NULL);
    }
    memset(ret, 0, sizeof(xmlSchemaValidCtxt));
    ret->type = XML_SCHEMA_CTXT_VALIDATOR;
    ret->schema = schema;
    return (ret);
}

/**
 * xmlSchemaClearValidCtxt:
 * @ctxt: the schema validation context
 *
 * Free the resources associated to the schema validation context;
 * leaves some fields alive intended for reuse of the context.
 */
static void
xmlSchemaClearValidCtxt(xmlSchemaValidCtxtPtr vctxt)
{
    if (vctxt == NULL)
        return;

    vctxt->flags = 0;
    vctxt->validationRoot = NULL;
    vctxt->doc = NULL;
#ifdef LIBXML_READER_ENABLED
    vctxt->reader = NULL;
#endif
    if (vctxt->value != NULL) {
        xmlSchemaFreeValue(vctxt->value);
	vctxt->value = NULL;
    }
    /*
    * Augmented IDC information.
    */
    if (vctxt->aidcs != NULL) {
	xmlSchemaIDCAugPtr cur = vctxt->aidcs, next;
	do {
	    next = cur->next;
	    xmlFree(cur);
	    cur = next;
	} while (cur != NULL);
	vctxt->aidcs = NULL;
    }
    if (vctxt->idcNodes != NULL) {
	int i;
	xmlSchemaPSVIIDCNodePtr item;

	for (i = 0; i < vctxt->nbIdcNodes; i++) {
	    item = vctxt->idcNodes[i];
	    xmlFree(item->keys);
	    xmlFree(item);
	}
	xmlFree(vctxt->idcNodes);
	vctxt->idcNodes = NULL;
    }
    /*
    * Note that we won't delete the XPath state pool here.
    */
    if (vctxt->xpathStates != NULL) {
	xmlSchemaFreeIDCStateObjList(vctxt->xpathStates);
	vctxt->xpathStates = NULL;
    }
    /*
    * Attribute info.
    */
    if (vctxt->nbAttrInfos != 0) {
	xmlSchemaClearAttrInfos(vctxt);
    }
    /*
    * Element info.
    */
    if (vctxt->elemInfos != NULL) {
	int i;
	xmlSchemaNodeInfoPtr ei;

	for (i = 0; i < vctxt->sizeElemInfos; i++) {
	    ei = vctxt->elemInfos[i];
	    if (ei == NULL)
		break;
	    xmlSchemaClearElemInfo(ei);
	}
    }
}

/**
 * xmlSchemaFreeValidCtxt:
 * @ctxt:  the schema validation context
 *
 * Free the resources associated to the schema validation context
 */
void
xmlSchemaFreeValidCtxt(xmlSchemaValidCtxtPtr ctxt)
{
    if (ctxt == NULL)
        return;
    if (ctxt->value != NULL)
        xmlSchemaFreeValue(ctxt->value);
    if (ctxt->pctxt != NULL)
	xmlSchemaFreeParserCtxt(ctxt->pctxt);
    if (ctxt->idcNodes != NULL) {
	int i;
	xmlSchemaPSVIIDCNodePtr item;

	for (i = 0; i < ctxt->nbIdcNodes; i++) {
	    item = ctxt->idcNodes[i];
	    xmlFree(item->keys);
	    xmlFree(item);
	}
	xmlFree(ctxt->idcNodes);
    }
    if (ctxt->idcKeys != NULL) {
	int i;
	for (i = 0; i < ctxt->nbIdcKeys; i++)
	    xmlSchemaIDCFreeKey(ctxt->idcKeys[i]);
	xmlFree(ctxt->idcKeys);
    }

    if (ctxt->xpathStates != NULL)
	xmlSchemaFreeIDCStateObjList(ctxt->xpathStates);
    if (ctxt->xpathStatePool != NULL)
	xmlSchemaFreeIDCStateObjList(ctxt->xpathStatePool);

    /*
    * Augmented IDC information.
    */
    if (ctxt->aidcs != NULL) {
	xmlSchemaIDCAugPtr cur = ctxt->aidcs, next;
	do {
	    next = cur->next;
	    xmlFree(cur);
	    cur = next;
	} while (cur != NULL);
    }
    if (ctxt->attrInfos != NULL) {
	int i;
	xmlSchemaAttrInfoPtr attr;

	/* Just a paranoid call to the cleanup. */
	if (ctxt->nbAttrInfos != 0)
	    xmlSchemaClearAttrInfos(ctxt);
	for (i = 0; i < ctxt->sizeAttrInfos; i++) {
	    attr = ctxt->attrInfos[i];
	    xmlFree(attr);
	}
	xmlFree(ctxt->attrInfos);
    }
    if (ctxt->elemInfos != NULL) {
	int i;
	xmlSchemaNodeInfoPtr ei;

	for (i = 0; i < ctxt->sizeElemInfos; i++) {
	    ei = ctxt->elemInfos[i];
	    if (ei == NULL)
		break;
	    xmlSchemaClearElemInfo(ei);
	    xmlFree(ei);
	}
	xmlFree(ctxt->elemInfos);
    }
    if (ctxt->dict != NULL)
	xmlDictFree(ctxt->dict);
    xmlFree(ctxt);
}

/**
 * xmlSchemaIsValid:
 * @ctxt: the schema validation context
 *
 * Check if any error was detected during validation.
 * 
 * Returns 1 if valid so far, 0 if errors were detected, and -1 in case
 *         of internal error.
 */
int
xmlSchemaIsValid(xmlSchemaValidCtxtPtr ctxt)
{
    if (ctxt == NULL)
        return(-1);
    return(ctxt->err == 0);
}

/**
 * xmlSchemaSetValidErrors:
 * @ctxt:  a schema validation context
 * @err:  the error function
 * @warn: the warning function
 * @ctx: the functions context
 *
 * Set the error and warning callback informations
 */
void
xmlSchemaSetValidErrors(xmlSchemaValidCtxtPtr ctxt,
                        xmlSchemaValidityErrorFunc err,
                        xmlSchemaValidityWarningFunc warn, void *ctx)
{
    if (ctxt == NULL)
        return;
    ctxt->error = err;
    ctxt->warning = warn;
    ctxt->userData = ctx;
    if (ctxt->pctxt != NULL)
	xmlSchemaSetParserErrors(ctxt->pctxt, err, warn, ctx);
}

/**
 * xmlSchemaGetValidErrors:
 * @ctxt:	a XML-Schema validation context
 * @err: the error function result
 * @warn: the warning function result
 * @ctx: the functions context result
 *
 * Get the error and warning callback informations
 *
 * Returns -1 in case of error and 0 otherwise
 */
int
xmlSchemaGetValidErrors(xmlSchemaValidCtxtPtr ctxt,
						xmlSchemaValidityErrorFunc * err,
						xmlSchemaValidityWarningFunc * warn, void **ctx)
{
	if (ctxt == NULL)
		return (-1);
	if (err != NULL)
		*err = ctxt->error;
	if (warn != NULL)
		*warn = ctxt->warning;
	if (ctx != NULL)
		*ctx = ctxt->userData;
	return (0);
}


/**
 * xmlSchemaSetValidOptions:
 * @ctxt:	a schema validation context
 * @options: a combination of xmlSchemaValidOption
 *
 * Sets the options to be used during the validation.
 *
 * Returns 0 in case of success, -1 in case of an
 * API error.
 */
int
xmlSchemaSetValidOptions(xmlSchemaValidCtxtPtr ctxt,
			 int options)

{
    int i;

    if (ctxt == NULL)
	return (-1);
    /*
    * WARNING: Change the start value if adding to the
    * xmlSchemaValidOption.
    * TODO: Is there an other, more easy to maintain,
    * way?
    */
    for (i = 1; i < (int) sizeof(int) * 8; i++) {
        if (options & 1<<i)
	    return (-1);
    }
    ctxt->options = options;
    return (0);
}

/**
 * xmlSchemaValidCtxtGetOptions:
 * @ctxt:	a schema validation context
 *
 * Get the validation context options.
 *
 * Returns the option combination or -1 on error.
 */
int
xmlSchemaValidCtxtGetOptions(xmlSchemaValidCtxtPtr ctxt)

{
    if (ctxt == NULL)
	return (-1);
    else
	return (ctxt->options);
}

static int
xmlSchemaVDocWalk(xmlSchemaValidCtxtPtr vctxt)
{
    xmlAttrPtr attr;
    int ret = 0;
    xmlSchemaNodeInfoPtr ielem = NULL;
    xmlNodePtr node, valRoot;
    const xmlChar *nsName;

    /* DOC VAL TODO: Move this to the start function. */
    valRoot = xmlDocGetRootElement(vctxt->doc);
    if (valRoot == NULL) {
	/* VAL TODO: Error code? */
	VERROR(1, NULL, "The document has no document element");
	return (1);
    }
    vctxt->depth = -1;
    vctxt->validationRoot = valRoot;
    node = valRoot;
    while (node != NULL) {
	if ((vctxt->skipDepth != -1) && (vctxt->depth >= vctxt->skipDepth))
	    goto next_sibling;
	if (node->type == XML_ELEMENT_NODE) {

	    /*
	    * Init the node-info.
	    */
	    vctxt->depth++;
	    if (xmlSchemaValidatorPushElem(vctxt) == -1)
		goto internal_error;
	    ielem = vctxt->inode;
	    ielem->node = node;
	    ielem->localName = node->name;
	    if (node->ns != NULL)
		ielem->nsName = node->ns->href;
	    ielem->flags |= XML_SCHEMA_ELEM_INFO_EMPTY;
	    /*
	    * Register attributes.
	    * DOC VAL TODO: We do not register namespace declaration
	    * attributes yet.
	    */
	    vctxt->nbAttrInfos = 0;
	    if (node->properties != NULL) {
		attr = node->properties;
		do {
		    if (attr->ns != NULL)
			nsName = attr->ns->href;
		    else
			nsName = NULL;
		    ret = xmlSchemaValidatorPushAttribute(vctxt,
			(xmlNodePtr) attr,
			attr->name, nsName, 0,
			xmlNodeListGetString(attr->doc, attr->children, 1), 1);
		    if (ret == -1) {
			VERROR_INT("xmlSchemaDocWalk",
			    "calling xmlSchemaValidatorPushAttribute()");
			goto internal_error;
		    }
		    attr = attr->next;
		} while (attr);
	    }
	    /*
	    * Validate the element.
	    */
	    ret = xmlSchemaValidateElem(vctxt);
	    if (ret != 0) {
		if (ret == -1) {
		    VERROR_INT("xmlSchemaDocWalk",
			"calling xmlSchemaValidateElem()");
		    goto internal_error;
		}
		/*
		* Don't stop validation; just skip the content
		* of this element.
		*/
		goto leave_node;
	    }
	    if ((vctxt->skipDepth != -1) &&
		(vctxt->depth >= vctxt->skipDepth))
		goto leave_node;
	} else if ((node->type == XML_TEXT_NODE) ||
	    (node->type == XML_CDATA_SECTION_NODE)) {
	    /*
	    * Process character content.
	    */
	    if (ielem->flags & XML_SCHEMA_ELEM_INFO_EMPTY)
		ielem->flags ^= XML_SCHEMA_ELEM_INFO_EMPTY;
	    ret = xmlSchemaVPushText(vctxt, node->type, node->content,
		-1, XML_SCHEMA_PUSH_TEXT_PERSIST, NULL);
	    if (ret < 0) {
		VERROR_INT("xmlSchemaVDocWalk",
		    "calling xmlSchemaVPushText()");
		goto internal_error;
	    }
	    /*
	    * DOC VAL TODO: Should we skip further validation of the
	    * element content here?
	    */
	} else if ((node->type == XML_ENTITY_NODE) ||
	    (node->type == XML_ENTITY_REF_NODE)) {
	    /*
	    * DOC VAL TODO: What to do with entities?
	    */
	    TODO
	} else {
	    goto leave_node;
	    /*
	    * DOC VAL TODO: XInclude nodes, etc.
	    */
	}
	/*
	* Walk the doc.
	*/
	if (node->children != NULL) {
	    node = node->children;
	    continue;
	}
leave_node:
	if (node->type == XML_ELEMENT_NODE) {
	    /*
	    * Leaving the scope of an element.
	    */
	    if (node != vctxt->inode->node) {
		VERROR_INT("xmlSchemaVDocWalk",
		    "element position mismatch");
		goto internal_error;
	    }
	    ret = xmlSchemaValidatorPopElem(vctxt);
	    if (ret != 0) {
		if (ret < 0) {
		    VERROR_INT("xmlSchemaVDocWalk",
			"calling xmlSchemaValidatorPopElem()");
		    goto internal_error;
		}
	    }
	    if (node == valRoot)
		goto exit;
	}
next_sibling:
	if (node->next != NULL)
	    node = node->next;
	else {
	    node = node->parent;
	    goto leave_node;
	}
    }

exit:
    return (ret);
internal_error:
    return (-1);
}

static int
xmlSchemaPreRun(xmlSchemaValidCtxtPtr vctxt) {
    /*
    * Some initialization.
    */
    vctxt->err = 0;
    vctxt->nberrors = 0;
    vctxt->depth = -1;
    vctxt->skipDepth = -1;
    /*
    * Create a schema + parser if necessary.
    */
    if (vctxt->schema == NULL) {

	if ((vctxt->pctxt == NULL) &&
	   (xmlSchemaCreatePCtxtOnVCtxt(vctxt) == -1))
	   return (-1);

	vctxt->schema = xmlSchemaNewSchema(vctxt->pctxt);
	if (vctxt->schema == NULL) {
	    VERROR_INT("xmlSchemaVStartValidation",
		    "creating a schema");
	    return (-1);
	}
	vctxt->xsiAssemble = 1;
    } else
	vctxt->xsiAssemble = 0;
    /*
    * Augment the IDC definitions.
    */
    if (vctxt->schema->idcDef != NULL) {
	xmlHashScan(vctxt->schema->idcDef,
	    (xmlHashScanner) xmlSchemaAugmentIDC, vctxt);
    }
    return(0);
}

static void
xmlSchemaPostRun(xmlSchemaValidCtxtPtr vctxt) {
    if (vctxt->xsiAssemble) {
	if (vctxt->schema != NULL) {
	    xmlSchemaFree(vctxt->schema);
	    vctxt->schema = NULL;
	}
    }
    xmlSchemaClearValidCtxt(vctxt);
}

static int
xmlSchemaVStart(xmlSchemaValidCtxtPtr vctxt)
{
    int ret = 0;

    if (xmlSchemaPreRun(vctxt) < 0)
        return(-1);

    if (vctxt->doc != NULL) {
	/*
	 * Tree validation.
	 */
	ret = xmlSchemaVDocWalk(vctxt);
#ifdef LIBXML_READER_ENABLED
    } else if (vctxt->reader != NULL) {
	/*
	 * XML Reader validation.
	 */
#ifdef XML_SCHEMA_READER_ENABLED
	ret = xmlSchemaVReaderWalk(vctxt);
#endif
#endif
    } else if ((vctxt->sax != NULL) && (vctxt->parserCtxt != NULL)) {
	/*
	 * SAX validation.
	 */
	ret = xmlParseDocument(vctxt->parserCtxt);
    } else {
	VERROR_INT("xmlSchemaVStartValidation",
	    "no instance to validate");
	ret = -1;
    }

    xmlSchemaPostRun(vctxt);
    if (ret == 0)
	ret = vctxt->err;
    return (ret);
}

/**
 * xmlSchemaValidateOneElement:
 * @ctxt:  a schema validation context
 * @elem:  an element node
 *
 * Validate a branch of a tree, starting with the given @elem.
 *
 * Returns 0 if the element and its subtree is valid, a positive error
 * code number otherwise and -1 in case of an internal or API error.
 */
int
xmlSchemaValidateOneElement(xmlSchemaValidCtxtPtr ctxt, xmlNodePtr elem)
{
    if ((ctxt == NULL) || (elem == NULL) || (elem->type != XML_ELEMENT_NODE))
	return (-1);

    if (ctxt->schema == NULL)
	return (-1);

    ctxt->doc = elem->doc;
    ctxt->node = elem;
    ctxt->validationRoot = elem;
    return(xmlSchemaVStart(ctxt));
}

/**
 * xmlSchemaValidateDoc:
 * @ctxt:  a schema validation context
 * @doc:  a parsed document tree
 *
 * Validate a document tree in memory.
 *
 * Returns 0 if the document is schemas valid, a positive error code
 *     number otherwise and -1 in case of internal or API error.
 */
int
xmlSchemaValidateDoc(xmlSchemaValidCtxtPtr ctxt, xmlDocPtr doc)
{
    if ((ctxt == NULL) || (doc == NULL))
        return (-1);

    ctxt->doc = doc;
    ctxt->node = xmlDocGetRootElement(doc);
    if (ctxt->node == NULL) {
        xmlSchemaCustomErr((xmlSchemaAbstractCtxtPtr) ctxt,
	    XML_SCHEMAV_DOCUMENT_ELEMENT_MISSING,
	    (xmlNodePtr) doc, NULL,
	    "The document has no document element", NULL, NULL);
        return (ctxt->err);
    }
    ctxt->validationRoot = ctxt->node;
    return (xmlSchemaVStart(ctxt));
}


/************************************************************************
 * 									*
 * 		Function and data for SAX streaming API			*
 * 									*
 ************************************************************************/
typedef struct _xmlSchemaSplitSAXData xmlSchemaSplitSAXData;
typedef xmlSchemaSplitSAXData *xmlSchemaSplitSAXDataPtr;

struct _xmlSchemaSplitSAXData {
    xmlSAXHandlerPtr      user_sax;
    void                 *user_data;
    xmlSchemaValidCtxtPtr ctxt;
    xmlSAXHandlerPtr      schemas_sax;
};

#define XML_SAX_PLUG_MAGIC 0xdc43ba21

struct _xmlSchemaSAXPlug {
    unsigned int magic;

    /* the original callbacks informations */
    xmlSAXHandlerPtr     *user_sax_ptr;
    xmlSAXHandlerPtr      user_sax;
    void                **user_data_ptr;
    void                 *user_data;

    /* the block plugged back and validation informations */
    xmlSAXHandler         schemas_sax;
    xmlSchemaValidCtxtPtr ctxt;
};

/* All those functions just bounces to the user provided SAX handlers */
static void
internalSubsetSplit(void *ctx, const xmlChar *name,
	       const xmlChar *ExternalID, const xmlChar *SystemID)
{
    xmlSchemaSAXPlugPtr ctxt = (xmlSchemaSAXPlugPtr) ctx;
    if ((ctxt != NULL) && (ctxt->user_sax != NULL) &&
        (ctxt->user_sax->internalSubset != NULL))
	ctxt->user_sax->internalSubset(ctxt->user_data, name, ExternalID,
	                               SystemID);
}

static int
isStandaloneSplit(void *ctx)
{
    xmlSchemaSAXPlugPtr ctxt = (xmlSchemaSAXPlugPtr) ctx;
    if ((ctxt != NULL) && (ctxt->user_sax != NULL) &&
        (ctxt->user_sax->isStandalone != NULL))
	return(ctxt->user_sax->isStandalone(ctxt->user_data));
    return(0);
}

static int
hasInternalSubsetSplit(void *ctx)
{
    xmlSchemaSAXPlugPtr ctxt = (xmlSchemaSAXPlugPtr) ctx;
    if ((ctxt != NULL) && (ctxt->user_sax != NULL) &&
        (ctxt->user_sax->hasInternalSubset != NULL))
	return(ctxt->user_sax->hasInternalSubset(ctxt->user_data));
    return(0);
}

static int
hasExternalSubsetSplit(void *ctx)
{
    xmlSchemaSAXPlugPtr ctxt = (xmlSchemaSAXPlugPtr) ctx;
    if ((ctxt != NULL) && (ctxt->user_sax != NULL) &&
        (ctxt->user_sax->hasExternalSubset != NULL))
	return(ctxt->user_sax->hasExternalSubset(ctxt->user_data));
    return(0);
}

static void
externalSubsetSplit(void *ctx, const xmlChar *name,
	       const xmlChar *ExternalID, const xmlChar *SystemID)
{
    xmlSchemaSAXPlugPtr ctxt = (xmlSchemaSAXPlugPtr) ctx;
    if ((ctxt != NULL) && (ctxt->user_sax != NULL) &&
        (ctxt->user_sax->internalSubset != NULL))
	ctxt->user_sax->internalSubset(ctxt->user_data, name, ExternalID,
	                               SystemID);
}

static xmlParserInputPtr
resolveEntitySplit(void *ctx, const xmlChar *publicId, const xmlChar *systemId)
{
    xmlSchemaSAXPlugPtr ctxt = (xmlSchemaSAXPlugPtr) ctx;
    if ((ctxt != NULL) && (ctxt->user_sax != NULL) &&
        (ctxt->user_sax->resolveEntity != NULL))
	return(ctxt->user_sax->resolveEntity(ctxt->user_data, publicId,
	                                     systemId));
    return(NULL);
}

static xmlEntityPtr
getEntitySplit(void *ctx, const xmlChar *name)
{
    xmlSchemaSAXPlugPtr ctxt = (xmlSchemaSAXPlugPtr) ctx;
    if ((ctxt != NULL) && (ctxt->user_sax != NULL) &&
        (ctxt->user_sax->getEntity != NULL))
	return(ctxt->user_sax->getEntity(ctxt->user_data, name));
    return(NULL);
}

static xmlEntityPtr
getParameterEntitySplit(void *ctx, const xmlChar *name)
{
    xmlSchemaSAXPlugPtr ctxt = (xmlSchemaSAXPlugPtr) ctx;
    if ((ctxt != NULL) && (ctxt->user_sax != NULL) &&
        (ctxt->user_sax->getParameterEntity != NULL))
	return(ctxt->user_sax->getParameterEntity(ctxt->user_data, name));
    return(NULL);
}


static void
entityDeclSplit(void *ctx, const xmlChar *name, int type,
          const xmlChar *publicId, const xmlChar *systemId, xmlChar *content)
{
    xmlSchemaSAXPlugPtr ctxt = (xmlSchemaSAXPlugPtr) ctx;
    if ((ctxt != NULL) && (ctxt->user_sax != NULL) &&
        (ctxt->user_sax->entityDecl != NULL))
	ctxt->user_sax->entityDecl(ctxt->user_data, name, type, publicId,
	                           systemId, content);
}

static void
attributeDeclSplit(void *ctx, const xmlChar * elem,
                   const xmlChar * name, int type, int def,
                   const xmlChar * defaultValue, xmlEnumerationPtr tree)
{
    xmlSchemaSAXPlugPtr ctxt = (xmlSchemaSAXPlugPtr) ctx;
    if ((ctxt != NULL) && (ctxt->user_sax != NULL) &&
        (ctxt->user_sax->attributeDecl != NULL)) {
	ctxt->user_sax->attributeDecl(ctxt->user_data, elem, name, type,
	                              def, defaultValue, tree);
    } else {
	xmlFreeEnumeration(tree);
    }
}

static void
elementDeclSplit(void *ctx, const xmlChar *name, int type,
	    xmlElementContentPtr content)
{
    xmlSchemaSAXPlugPtr ctxt = (xmlSchemaSAXPlugPtr) ctx;
    if ((ctxt != NULL) && (ctxt->user_sax != NULL) &&
        (ctxt->user_sax->elementDecl != NULL))
	ctxt->user_sax->elementDecl(ctxt->user_data, name, type, content);
}

static void
notationDeclSplit(void *ctx, const xmlChar *name,
	     const xmlChar *publicId, const xmlChar *systemId)
{
    xmlSchemaSAXPlugPtr ctxt = (xmlSchemaSAXPlugPtr) ctx;
    if ((ctxt != NULL) && (ctxt->user_sax != NULL) &&
        (ctxt->user_sax->notationDecl != NULL))
	ctxt->user_sax->notationDecl(ctxt->user_data, name, publicId,
	                             systemId);
}

static void
unparsedEntityDeclSplit(void *ctx, const xmlChar *name,
		   const xmlChar *publicId, const xmlChar *systemId,
		   const xmlChar *notationName)
{
    xmlSchemaSAXPlugPtr ctxt = (xmlSchemaSAXPlugPtr) ctx;
    if ((ctxt != NULL) && (ctxt->user_sax != NULL) &&
        (ctxt->user_sax->unparsedEntityDecl != NULL))
	ctxt->user_sax->unparsedEntityDecl(ctxt->user_data, name, publicId,
	                                   systemId, notationName);
}

static void
setDocumentLocatorSplit(void *ctx, xmlSAXLocatorPtr loc)
{
    xmlSchemaSAXPlugPtr ctxt = (xmlSchemaSAXPlugPtr) ctx;
    if ((ctxt != NULL) && (ctxt->user_sax != NULL) &&
        (ctxt->user_sax->setDocumentLocator != NULL))
	ctxt->user_sax->setDocumentLocator(ctxt->user_data, loc);
}

static void
startDocumentSplit(void *ctx)
{
    xmlSchemaSAXPlugPtr ctxt = (xmlSchemaSAXPlugPtr) ctx;
    if ((ctxt != NULL) && (ctxt->user_sax != NULL) &&
        (ctxt->user_sax->startDocument != NULL))
	ctxt->user_sax->startDocument(ctxt->user_data);
}

static void
endDocumentSplit(void *ctx)
{
    xmlSchemaSAXPlugPtr ctxt = (xmlSchemaSAXPlugPtr) ctx;
    if ((ctxt != NULL) && (ctxt->user_sax != NULL) &&
        (ctxt->user_sax->endDocument != NULL))
	ctxt->user_sax->endDocument(ctxt->user_data);
}

static void
processingInstructionSplit(void *ctx, const xmlChar *target,
                      const xmlChar *data)
{
    xmlSchemaSAXPlugPtr ctxt = (xmlSchemaSAXPlugPtr) ctx;
    if ((ctxt != NULL) && (ctxt->user_sax != NULL) &&
        (ctxt->user_sax->processingInstruction != NULL))
	ctxt->user_sax->processingInstruction(ctxt->user_data, target, data);
}

static void
commentSplit(void *ctx, const xmlChar *value)
{
    xmlSchemaSAXPlugPtr ctxt = (xmlSchemaSAXPlugPtr) ctx;
    if ((ctxt != NULL) && (ctxt->user_sax != NULL) &&
        (ctxt->user_sax->comment != NULL))
	ctxt->user_sax->comment(ctxt->user_data, value);
}

/*
 * Varargs error callbacks to the user application, harder ...
 */

static void
warningSplit(void *ctx, const char *msg, ...) {
    xmlSchemaSAXPlugPtr ctxt = (xmlSchemaSAXPlugPtr) ctx;
    if ((ctxt != NULL) && (ctxt->user_sax != NULL) &&
        (ctxt->user_sax->warning != NULL)) {
	TODO
    }
}
static void
errorSplit(void *ctx, const char *msg, ...) {
    xmlSchemaSAXPlugPtr ctxt = (xmlSchemaSAXPlugPtr) ctx;
    if ((ctxt != NULL) && (ctxt->user_sax != NULL) &&
        (ctxt->user_sax->error != NULL)) {
	TODO
    }
}
static void
fatalErrorSplit(void *ctx, const char *msg, ...) {
    xmlSchemaSAXPlugPtr ctxt = (xmlSchemaSAXPlugPtr) ctx;
    if ((ctxt != NULL) && (ctxt->user_sax != NULL) &&
        (ctxt->user_sax->fatalError != NULL)) {
	TODO
    }
}

/*
 * Those are function where both the user handler and the schemas handler
 * need to be called.
 */
static void
charactersSplit(void *ctx, const xmlChar *ch, int len)
{
    xmlSchemaSAXPlugPtr ctxt = (xmlSchemaSAXPlugPtr) ctx;
    if (ctxt == NULL)
        return;
    if ((ctxt->user_sax != NULL) && (ctxt->user_sax->characters != NULL))
	ctxt->user_sax->characters(ctxt->user_data, ch, len);
    if (ctxt->ctxt != NULL)
	xmlSchemaSAXHandleText(ctxt->ctxt, ch, len);
}

static void
ignorableWhitespaceSplit(void *ctx, const xmlChar *ch, int len)
{
    xmlSchemaSAXPlugPtr ctxt = (xmlSchemaSAXPlugPtr) ctx;
    if (ctxt == NULL)
        return;
    if ((ctxt->user_sax != NULL) &&
        (ctxt->user_sax->ignorableWhitespace != NULL))
	ctxt->user_sax->ignorableWhitespace(ctxt->user_data, ch, len);
    if (ctxt->ctxt != NULL)
	xmlSchemaSAXHandleText(ctxt->ctxt, ch, len);
}

static void
cdataBlockSplit(void *ctx, const xmlChar *value, int len)
{
    xmlSchemaSAXPlugPtr ctxt = (xmlSchemaSAXPlugPtr) ctx;
    if (ctxt == NULL)
        return;
    if ((ctxt->user_sax != NULL) &&
        (ctxt->user_sax->ignorableWhitespace != NULL))
	ctxt->user_sax->ignorableWhitespace(ctxt->user_data, value, len);
    if (ctxt->ctxt != NULL)
	xmlSchemaSAXHandleCDataSection(ctxt->ctxt, value, len);
}

static void
referenceSplit(void *ctx, const xmlChar *name)
{
    xmlSchemaSAXPlugPtr ctxt = (xmlSchemaSAXPlugPtr) ctx;
    if ((ctxt != NULL) && (ctxt->user_sax != NULL) &&
        (ctxt->user_sax->reference != NULL))
	ctxt->user_sax->reference(ctxt->user_data, name);
    if (ctxt->ctxt != NULL)
        xmlSchemaSAXHandleReference(ctxt->user_data, name);
}

static void
startElementNsSplit(void *ctx, const xmlChar * localname, 
		    const xmlChar * prefix, const xmlChar * URI, 
		    int nb_namespaces, const xmlChar ** namespaces, 
		    int nb_attributes, int nb_defaulted, 
		    const xmlChar ** attributes) {
    xmlSchemaSAXPlugPtr ctxt = (xmlSchemaSAXPlugPtr) ctx;
    if (ctxt == NULL)
        return;
    if ((ctxt->user_sax != NULL) &&
        (ctxt->user_sax->startElementNs != NULL))
	ctxt->user_sax->startElementNs(ctxt->user_data, localname, prefix,
	                               URI, nb_namespaces, namespaces,
				       nb_attributes, nb_defaulted,
				       attributes);
    if (ctxt->ctxt != NULL)
	xmlSchemaSAXHandleStartElementNs(ctxt->ctxt, localname, prefix,
	                                 URI, nb_namespaces, namespaces,
					 nb_attributes, nb_defaulted,
					 attributes);
}

static void
endElementNsSplit(void *ctx, const xmlChar * localname, 
		    const xmlChar * prefix, const xmlChar * URI) {
    xmlSchemaSAXPlugPtr ctxt = (xmlSchemaSAXPlugPtr) ctx;
    if (ctxt == NULL)
        return;
    if ((ctxt->user_sax != NULL) &&
        (ctxt->user_sax->endElementNs != NULL))
	ctxt->user_sax->endElementNs(ctxt->user_data, localname, prefix, URI);
    if (ctxt->ctxt != NULL)
	xmlSchemaSAXHandleEndElementNs(ctxt->ctxt, localname, prefix, URI);
}

/**
 * xmlSchemaSAXPlug:
 * @ctxt:  a schema validation context
 * @sax:  a pointer to the original xmlSAXHandlerPtr
 * @user_data:  a pointer to the original SAX user data pointer
 *
 * Plug a SAX based validation layer in a SAX parsing event flow.
 * The original @saxptr and @dataptr data are replaced by new pointers
 * but the calls to the original will be maintained.
 *
 * Returns a pointer to a data structure needed to unplug the validation layer
 *         or NULL in case of errors.
 */
xmlSchemaSAXPlugPtr
xmlSchemaSAXPlug(xmlSchemaValidCtxtPtr ctxt,
		 xmlSAXHandlerPtr *sax, void **user_data)
{
    xmlSchemaSAXPlugPtr ret;
    xmlSAXHandlerPtr old_sax;

    if ((ctxt == NULL) || (sax == NULL) || (user_data == NULL))
        return(NULL);

    /*
     * We only allow to plug into SAX2 event streams
     */
    old_sax = *sax;
    if ((old_sax != NULL) && (old_sax->initialized != XML_SAX2_MAGIC))
        return(NULL);
    if ((old_sax != NULL) && 
        (old_sax->startElementNs == NULL) && (old_sax->endElementNs == NULL) &&
        ((old_sax->startElement != NULL) || (old_sax->endElement != NULL)))
        return(NULL);

    /*
     * everything seems right allocate the local data needed for that layer
     */
    ret = (xmlSchemaSAXPlugPtr) xmlMalloc(sizeof(xmlSchemaSAXPlugStruct));
    if (ret == NULL) {
        return(NULL);
    }
    memset(ret, 0, sizeof(xmlSchemaSAXPlugStruct));
    ret->magic = XML_SAX_PLUG_MAGIC;
    ret->schemas_sax.initialized = XML_SAX2_MAGIC;
    ret->ctxt = ctxt;
    ret->user_sax_ptr = sax;
    ret->user_sax = old_sax;
    if (old_sax == NULL) {	
        /*
	 * go direct, no need for the split block and functions.
	 */
	ret->schemas_sax.startElementNs = xmlSchemaSAXHandleStartElementNs;
	ret->schemas_sax.endElementNs = xmlSchemaSAXHandleEndElementNs;
	/*
	 * Note that we use the same text-function for both, to prevent
	 * the parser from testing for ignorable whitespace.
	 */
	ret->schemas_sax.ignorableWhitespace = xmlSchemaSAXHandleText;
	ret->schemas_sax.characters = xmlSchemaSAXHandleText;

	ret->schemas_sax.cdataBlock = xmlSchemaSAXHandleCDataSection;
	ret->schemas_sax.reference = xmlSchemaSAXHandleReference;

	ret->user_data = ctxt;
	*user_data = ctxt;
    } else {
       /*
        * for each callback unused by Schemas initialize it to the Split
	* routine only if non NULL in the user block, this can speed up 
	* things at the SAX level.
	*/
        if (old_sax->internalSubset != NULL)
            ret->schemas_sax.internalSubset = internalSubsetSplit;
        if (old_sax->isStandalone != NULL)
            ret->schemas_sax.isStandalone = isStandaloneSplit;
        if (old_sax->hasInternalSubset != NULL)
            ret->schemas_sax.hasInternalSubset = hasInternalSubsetSplit;
        if (old_sax->hasExternalSubset != NULL)
            ret->schemas_sax.hasExternalSubset = hasExternalSubsetSplit;
        if (old_sax->resolveEntity != NULL)
            ret->schemas_sax.resolveEntity = resolveEntitySplit;
        if (old_sax->getEntity != NULL)
            ret->schemas_sax.getEntity = getEntitySplit;
        if (old_sax->entityDecl != NULL)
            ret->schemas_sax.entityDecl = entityDeclSplit;
        if (old_sax->notationDecl != NULL)
            ret->schemas_sax.notationDecl = notationDeclSplit;
        if (old_sax->attributeDecl != NULL)
            ret->schemas_sax.attributeDecl = attributeDeclSplit;
        if (old_sax->elementDecl != NULL)
            ret->schemas_sax.elementDecl = elementDeclSplit;
        if (old_sax->unparsedEntityDecl != NULL)
            ret->schemas_sax.unparsedEntityDecl = unparsedEntityDeclSplit;
        if (old_sax->setDocumentLocator != NULL)
            ret->schemas_sax.setDocumentLocator = setDocumentLocatorSplit;
        if (old_sax->startDocument != NULL)
            ret->schemas_sax.startDocument = startDocumentSplit;
        if (old_sax->endDocument != NULL)
            ret->schemas_sax.endDocument = endDocumentSplit;
        if (old_sax->processingInstruction != NULL)
            ret->schemas_sax.processingInstruction = processingInstructionSplit;
        if (old_sax->comment != NULL)
            ret->schemas_sax.comment = commentSplit;
        if (old_sax->warning != NULL)
            ret->schemas_sax.warning = warningSplit;
        if (old_sax->error != NULL)
            ret->schemas_sax.error = errorSplit;
        if (old_sax->fatalError != NULL)
            ret->schemas_sax.fatalError = fatalErrorSplit;
        if (old_sax->getParameterEntity != NULL)
            ret->schemas_sax.getParameterEntity = getParameterEntitySplit;
        if (old_sax->externalSubset != NULL)
            ret->schemas_sax.externalSubset = externalSubsetSplit;

	/*
	 * the 6 schemas callback have to go to the splitter functions
	 * Note that we use the same text-function for ignorableWhitespace
	 * if possible, to prevent the parser from testing for ignorable
	 * whitespace.
	 */
        ret->schemas_sax.characters = charactersSplit;
	if ((old_sax->ignorableWhitespace != NULL) &&
	    (old_sax->ignorableWhitespace != old_sax->characters))
	    ret->schemas_sax.ignorableWhitespace = ignorableWhitespaceSplit;
	else
	    ret->schemas_sax.ignorableWhitespace = charactersSplit;
        ret->schemas_sax.cdataBlock = cdataBlockSplit;
        ret->schemas_sax.reference = referenceSplit;
        ret->schemas_sax.startElementNs = startElementNsSplit;
        ret->schemas_sax.endElementNs = endElementNsSplit;

	ret->user_data_ptr = user_data;
	ret->user_data = *user_data;
	*user_data = ret;
    }

    /*
     * plug the pointers back.
     */
    *sax = &(ret->schemas_sax);
    ctxt->sax = *sax;
    ctxt->flags |= XML_SCHEMA_VALID_CTXT_FLAG_STREAM;
    xmlSchemaPreRun(ctxt);
    return(ret);
}

/**
 * xmlSchemaSAXUnplug:
 * @plug:  a data structure returned by xmlSchemaSAXPlug
 *
 * Unplug a SAX based validation layer in a SAX parsing event flow.
 * The original pointers used in the call are restored.
 *
 * Returns 0 in case of success and -1 in case of failure.
 */
int
xmlSchemaSAXUnplug(xmlSchemaSAXPlugPtr plug)
{
    xmlSAXHandlerPtr *sax;
    void **user_data;

    if ((plug == NULL) || (plug->magic != XML_SAX_PLUG_MAGIC))
        return(-1);
    plug->magic = 0;

    xmlSchemaPostRun(plug->ctxt);
    /* restore the data */
    sax = plug->user_sax_ptr;
    *sax = plug->user_sax;
    if (plug->user_sax != NULL) {
	user_data = plug->user_data_ptr;
	*user_data = plug->user_data;
    }

    /* free and return */
    xmlFree(plug);
    return(0);
}

/**
 * xmlSchemaValidateStream:
 * @ctxt:  a schema validation context
 * @input:  the input to use for reading the data
 * @enc:  an optional encoding information
 * @sax:  a SAX handler for the resulting events
 * @user_data:  the context to provide to the SAX handler.
 *
 * Validate an input based on a flow of SAX event from the parser
 * and forward the events to the @sax handler with the provided @user_data
 * the user provided @sax handler must be a SAX2 one.
 *
 * Returns 0 if the document is schemas valid, a positive error code
 *     number otherwise and -1 in case of internal or API error.
 */
int
xmlSchemaValidateStream(xmlSchemaValidCtxtPtr ctxt,
                        xmlParserInputBufferPtr input, xmlCharEncoding enc,
                        xmlSAXHandlerPtr sax, void *user_data)
{
    xmlSchemaSAXPlugPtr plug = NULL;
    xmlSAXHandlerPtr old_sax = NULL;
    xmlParserCtxtPtr pctxt = NULL;
    xmlParserInputPtr inputStream = NULL;
    int ret;

    if ((ctxt == NULL) || (input == NULL))
        return (-1);

    /*
     * prepare the parser
     */
    pctxt = xmlNewParserCtxt();
    if (pctxt == NULL)
        return (-1);
    old_sax = pctxt->sax;
    pctxt->sax = sax;
    pctxt->userData = user_data;
#if 0
    if (options)
        xmlCtxtUseOptions(pctxt, options);
#endif
    pctxt->linenumbers = 1;    

    inputStream = xmlNewIOInputStream(pctxt, input, enc);;
    if (inputStream == NULL) {
        ret = -1;
	goto done;
    }
    inputPush(pctxt, inputStream);
    ctxt->parserCtxt = pctxt;
    ctxt->input = input;

    /*
     * Plug the validation and launch the parsing
     */
    plug = xmlSchemaSAXPlug(ctxt, &(pctxt->sax), &(pctxt->userData));
    if (plug == NULL) {
        ret = -1;
	goto done;
    }
    ctxt->input = input;
    ctxt->enc = enc;
    ctxt->sax = pctxt->sax;
    ctxt->flags |= XML_SCHEMA_VALID_CTXT_FLAG_STREAM;
    ret = xmlSchemaVStart(ctxt);

    if ((ret == 0) && (! ctxt->parserCtxt->wellFormed)) {
	ret = ctxt->parserCtxt->errNo;
	if (ret == 0)
	    ret = 1;
    }    

done:
    ctxt->parserCtxt = NULL;
    ctxt->sax = NULL;
    ctxt->input = NULL;
    if (plug != NULL) {
        xmlSchemaSAXUnplug(plug);
    }
    /* cleanup */
    if (pctxt != NULL) {
	pctxt->sax = old_sax;
	xmlFreeParserCtxt(pctxt);
    }
    return (ret);
}

/**
 * xmlSchemaValidateFile:
 * @ctxt: a schema validation context
 * @filename: the URI of the instance
 * @options: a future set of options, currently unused
 *
 * Do a schemas validation of the given resource, it will use the
 * SAX streamable validation internally.
 *
 * Returns 0 if the document is valid, a positive error code
 *     number otherwise and -1 in case of an internal or API error.
 */
int
xmlSchemaValidateFile(xmlSchemaValidCtxtPtr ctxt,
                      const char * filename,
		      int options ATTRIBUTE_UNUSED)
{
#ifdef XML_SCHEMA_SAX_ENABLED
    int ret;
    xmlParserInputBufferPtr input;

    if ((ctxt == NULL) || (filename == NULL))
        return (-1);
    
    input = xmlParserInputBufferCreateFilename(filename,
	XML_CHAR_ENCODING_NONE);
    if (input == NULL)
	return (-1);
    ret = xmlSchemaValidateStream(ctxt, input, XML_CHAR_ENCODING_NONE,
	NULL, NULL);    
    return (ret);
#else
    return (-1);
#endif /* XML_SCHEMA_SAX_ENABLED */
}

#define bottom_xmlschemas
#include "elfgcchack.h"
#endif /* LIBXML_SCHEMAS_ENABLED */
