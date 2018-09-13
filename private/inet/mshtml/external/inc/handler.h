#ifndef HANDLER_H
#define HANDLER_H



interface IScriptletHandlerConstructor;
interface IScriptletHandler;

/***************************************************************************
	Scriptlet Interface Handlers
	============================
	This is a preliminary draft. Changes may need to be made based on
	review feedback, support needed for client side security, and support
	for MTS scalability.
	
	The primary responsibility of a scriptlet interface handler is to 
	aggregate a set of COM interfaces with the scriptlet base runtime
	and translate calls made on those COM interfaces into calls to the
	script name space.
	
	Interface handlers are created using a constructor object. The 
	constructor object takes on the role similar to that of class
	objects in most other languages. It is intialized with the XML
	data nested in the implements element and then can be used for:	
		1. Execution - creating handler objects for scriptlet instances
		2. Registration - performing handler specific registration for an object
		3. Type library generation - generating a typelibrary for use with MTS
***************************************************************************/
typedef WORD PK;

#define pkELEMENT	0
#define pkATTRIBUTE	1
#define pkTEXT		2
#define pkCOMMENT	3
#define pkPI		4
#define pkXMLDECL	5
#define pkVALUE		6

#define fcompileIsXML		0x0001
#define fcompileValidate	0x0002
#define fcompileAllowDebug	0x8000
	
struct PNODE
	{
	PK pk;
	ULONG line;
	ULONG column;
	ULONG cchToken;
	LPCOLESTR pstrToken;
	PNODE *pnodeNext;
	union
		{
		struct
			{
			PNODE *pnodeAttr;
			PNODE *pnodeData;
			void *pvLim; // Used to calc amount of memory to allocate
			} element;
		
		struct 
			{
			PNODE *pnodeAttr;
			void *pvLim;
			} xmldecl;
			
		struct
			{
			PNODE *pnodeValue;
			void *pvLim; // Used to calc amount of memory to allocate
			} attribute, pi;

		struct
			{
			void *pvLim; // Used to calc amount of memory to allocate
			} text, comment, value;
		
		};
	};


DEFINE_GUID(IID_IScriptletHandlerConstructor, 0xa3d52a50, 0xb7ff, 0x11d1, 0xa3, 0x5a, 0x0, 0x60, 0x8, 0xc3, 0xfb, 0xfc);
interface IScriptletHandlerConstructor : public IUnknown
	{
    STDMETHOD(Load)(WORD wFlags, PNODE *pnode) PURE;
	STDMETHOD(Create)(IUnknown *punkContext, IUnknown *punkOuter,
			IUnknown **ppunkHandler) PURE;
	STDMETHOD(Register)(LPCOLESTR pstrPath, REFCLSID rclisid, 
			LPCOLESTR pstrProgId) PURE;
	STDMETHOD(Unregister)(REFCLSID rclsid, LPCOLESTR pstrProgId) PURE;
	STDMETHOD(AddInterfaceTypeInfo)(ICreateTypeLib *ptclib, 
			ICreateTypeInfo *pctiCoclass, UINT *puiImplIndex) PURE;
	};

DEFINE_GUID(IID_IScriptletHandler, 0xa001a870, 0xa7df, 0x11d1, 0x89, 0xbe, 0x0, 0x60, 0x8, 0xc3, 0xfb, 0xfc);
interface IScriptletHandler : public IUnknown
	{
	STDMETHOD(GetNameSpaceObject)(IUnknown **ppunk) PURE;
	STDMETHOD(SetScriptNameSpace)(IUnknown *punkNameSpace) PURE;
	};


#define IScriptletHandlerConstructorNew IScriptletHandlerConstructor
#define IID_IScriptletHandlerConstructorNew IID_IScriptletHandlerConstructorNew









/***************************************************************************
	Scriptlet XML Object Model Interfaces
	
	In an ideal world, we would be using a standard IPersistXML interface
	with a standardized XML DOM to load interface handlers from the XML
	data stream. Unfortunately, these interface definitions will not be ready
	in time for our ship date. As a result, we define our own private 
	intefaces which we will use until the official stuff becomes available.
	
	These interfaces are designed to provide the minimal set of methods
	needed to implement persistence support for scriptlet interface handlers.
	Matching the proposed XML interfaces was a consideration, but not an
	overwhelming factor. The main constraint in this design is the time
	required to implement.
	
	The return values of the methods name and data depend on the node 
	type. Here's a table that describes the return values for each of the
	types. NOTHING is signalled by the method returning S_FALSE and setting
	the bstr pointer to NULL.
	
	Type		name method					data method
	====		===========					===========
	ELEMENT		Tag name					NOTHING
	ATTRIBUTE	Attribute name				NOTHING or attribute value if exists
	TEXT		NOTHING						Characters in text
	COMMENT		NOTHING						Characters in comment
	PI			Processing instruction		Data for the PI.
	XMLDECL		NOTHING						NOTHING
	
	The getFirstChild and getAttributes methods are only valid for
	nodes of type ELEMENT. The nodes returned by getFirstAttribute will
	always be of type ScriptletXML_ATTRIBUTE.
	
	The getNext method gets the next sibling. The grfxml parameter allows
	you to filter out the types of nodes you're interested in. The flag
	fxmlText will only return only those text sequences that are not
	all white space. Passing in fxmlAllText will get all text nodes.
***************************************************************************/
interface IScriptletXML;

typedef enum
	{
	ScriptletXML_ELEMENT,
	ScriptletXML_ATTRIBUTE,
	ScriptletXML_TEXT,
	ScriptletXML_COMMENT,
	ScriptletXML_PI,
	ScriptletXML_XMLDECL,
	} ScriptletXMLNodeType;	

#define fxmlElement		(1<<ScriptletXML_ELEMENT)
#define fxmlAttribute 	(1<<ScriptletXML_ATTRIBUTE)
#define fxmlText		(1<<ScriptletXML_TEXT)
#define fxmlComment		(1<<ScriptletXML_COMMENT)
#define fxmlPI			(1<<ScriptletXML_PI)
#define fxmlXMLDecl		(1<<ScriptletXML_XMLDECL)
#define fxmlHasText		0x0100

#define kgrfxmlNormal	(fxmlElement|fxmlHasText)
#define kgrfxmlAll		(fxmlElement|fxmlAttribute|fxmlText|fxmlComment| \
							fxmlPI|fxmlXMLDecl)

#define fattrFailOnUnknown	0x0001
							
							
DEFINE_GUID(IID_IScriptletXML, 0xddd30cc0, 0xa3fe, 0x11d1, 0xb3, 0x82, 0x0, 0xa0, 0xc9, 0x11, 0xe8, 0xb2);

interface IScriptletXML : public IUnknown
    {
	STDMETHOD(getNodeType)(long *ptype) PURE;
	STDMETHOD(getPosition)(ULONG *pline, ULONG *pcolumn) PURE;
	STDMETHOD(getName)(BSTR *pbstrName) PURE;
	STDMETHOD(getData)(BSTR *pbstrValue) PURE; 
	STDMETHOD(getNext)(WORD grfxmlFilter, IScriptletXML **ppxml) PURE;
	STDMETHOD(getFirstChild)(WORD grfxmlFilter, IScriptletXML **ppxml) PURE;
	STDMETHOD(getFirstAttribute)(IScriptletXML **ppxml) PURE;
	STDMETHOD(getAttributes)(WORD grfattr, long cattr, 
			LPCOLESTR *prgpstrAttributes, BSTR *prgbstrValues) PURE;
    };



DEFINE_GUID(IID_IScriptletHandlerConstructorOld, 0x67463cd0, 0xb371, 0x11d1, 0x89, 0xca, 0x0, 0x60, 0x8, 0xc3, 0xfb, 0xfc);
interface IScriptletHandlerConstructorOld : public IUnknown
	{
    STDMETHOD(Load)(WORD wFlags, IScriptletXML *pxmlElement) PURE;
	STDMETHOD(Create)(IUnknown *punkContext, IUnknown *punkOuter,
			IUnknown **ppunkHandler) PURE;
	STDMETHOD(Register)(LPCOLESTR pstrPath, REFCLSID rclisid, 
			LPCOLESTR pstrProgId) PURE;
	STDMETHOD(Unregister)(REFCLSID rclsid, LPCOLESTR pstrProgId) PURE;
	STDMETHOD(AddInterfaceTypeInfo)(ICreateTypeLib *ptclib, 
			ICreateTypeInfo *pctiCoclass, UINT *puiImplIndex) PURE;
	};



#endif // HANDLER_H
