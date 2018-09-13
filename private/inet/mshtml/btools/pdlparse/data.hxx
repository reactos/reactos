enum DESCRIPTOR_TYPE
{
    TYPE_FILE,
    TYPE_INTERFACE,
    TYPE_EVAL,
    TYPE_ENUM,
    TYPE_CLASS,
    TYPE_IMPLEMENTS,
    TYPE_PROPERTY,
    TYPE_METHOD,
    TYPE_METHOD_ARG,
    TYPE_REFPROP,
    TYPE_REFMETHOD,
    TYPE_EVENT,
    TYPE_IMPORT,
    TYPE_TEAROFF,
    TYPE_TEAROFFMETHOD,
    TYPE_STRUCT,
    TYPE_STRUCTMEMBER,
    NUM_DESCRIPTOR_TYPES
};

enum 
{
    FILE_TAG
};

enum 
{
    INTERFACE_NAME,
    INTERFACE_SUPER,
    INTERFACE_GUID,
    INTERFACE_ABSTRACT,
    INTERFACE_CUSTOM,
    INTERFACE_NOPRIMARYTEAROFF
};

enum
{
    EVAL_NAME,
    EVAL_VALUE,
    EVAL_STRING,
    EVAL_ODLNAME
};

enum
{
    ENUM_NAME,
    ENUM_PREFIX,
    ENUM_GUID,
    ENUM_HIDDEN
};

enum
{
    CLASS_NAME,
    CLASS_SUPER,
    CLASS_INTERFACE,
    CLASS_GUID,
    CLASS_ABSTRACT,
    CLASS_COCLASSNAME,
    CLASS_EVENTS,
    CLASS_CASCADEDMETHODS,
    CLASS_NOAAMETHODS,
    CLASS_KEEPNOPERSIST,
    CLASS_NOCPC,
    CLASS_CONTROL,
    CLASS_MONDOGUID,
    CLASS_NONPRIMARYEVENTS1,
    CLASS_NONPRIMARYEVENTS2,
    CLASS_NONPRIMARYEVENTS3,
    CLASS_NONPRIMARYEVENTS4,
};

enum
{
    IMPLEMENTS_NAME,
    IMPLEMENTS_GUID
};

enum
{
    PROPERTY_NAME,
    PROPERTY_ATYPE,
    PROPERTY_DISPID,
    PROPERTY_TYPE,
    PROPERTY_MEMBER,
    PROPERTY_GET,
    PROPERTY_SET,
    PROPERTY_BINDABLE,
    PROPERTY_DISPLAYBIND,
    PROPERTY_DWFLAGS,
    PROPERTY_ABSTRACT,
    PROPERTY_PPFLAGS,
    PROPERTY_NOTPRESENTDEFAULT,
    PROPERTY_NOTSETDEFAULT,
    PROPERTY_MIN,
    PROPERTY_MAX,
    PROPERTY_GETSETMETHODS,
    PROPERTY_HELP,
    PROPERTY_VT,
    PROPERTY_CAA,
    PROPERTY_OBJECT,
    PROPERTY_INDEX,
    PROPERTY_INDEX1,
    PROPERTY_INDEXTYPE,
    PROPERTY_INDEXTYPE1,
    PROPERTY_SZATTRIBUTE,
    PROPERTY_HIDDEN,
    PROPERTY_NONBROWSABLE,
    PROPERTY_RESTRICTED,
    PROPERTY_SUBOBJECT,
    PROPERTY_PARAM1,
    PROPERTY_PRECALLFUNCTION,
    PROPERTY_ENUMREF,
    PROPERTY_VIRTUAL,
    PROPERTY_GETAA,
    PROPERTY_SETAAHR,
    PROPERTY_SOURCE,
    PROPERTY_MINOUT,
    PROPERTY_CASCADED,
    PROPERTY_UPDATECOLLECTION,
    PROPERTY_SCRIPTLET,
    PROPERTY_CLEARCACHES,
    PROPERTY_STYLEPROP,
    PROPERTY_DONTUSENOTASSIGN,
    PROPERTY_RESIZE,            // parent calls calcsize
    PROPERTY_REMEASURE,         // remeasure your contents
    PROPERTY_REMEASUREALL,      // remeasure self and all nested layouts
    PROPERTY_SITEREDRAW,
    PROPERTY_SETDESIGNMODE,     // Writeable only at Design time
    PROPERTY_INVALIDASNOASSIGN,
    PROPERTY_NOTPRESENTASDEFAULT,
    PROPERTY_THUNKCONTEXT,
    PROPERTY_THUNKNODECONTEXT,
    PROPERTY_BASEIMPLEMENTATION,
    PROPERTY_NOPERSIST,         // replaces PPFLAGS:PROPPARAM_NOPERSIST
    PROPERTY_INTERNAL,          // Don't include in propertydesc array
    PROPERTY_REFDTOCLASS,        // Internal to remember the class that the refprop is to.
    PROPERTY_MAXSTRLEN,         // Used in vtable computation.
    PROPERTY_NOPROPDESC,        // indicates this property doesn't participate in the mondo
    PROPERTY_EXCLUSIVETOSCRIPT, // expose this property on primary interface if any name clashes in
                                // other supported interfaces.
    PROPERTY_SZINTERFACEEXPOSE, // Expose this method name for the interface should be used in
                                // conjunction with nopropdesc.
    PROPERTY_DATAEVENT,         // non-abstract property calls put_DataEvent instead of put_Variant
    PROPERTY_ACCESSIBILITYSTATE,// the property is an accessibility state.
};

enum
{
    METHODARG_NAME, // Never used
    METHODARG_ATYPE,
    METHODARG_TYPE,
    METHODARG_IN,
    METHODARG_OUT,
    METHODARG_ARGNAME,
    METHODARG_RETURNVALUE,
    METHODARG_DEFAULTVALUE,
    METHODARG_OPTIONAL
};

enum 
{
    METHOD_NAME, // Name of the Method
    METHOD_RETURNTYPE,
    METHOD_DISPID,
    METHOD_ABSTRACT,
    METHOD_VIRTUAL,
    // This slot is used internaly to store the class that was refproped.
    // Whether the method is refd to a class or not - need this for
    // DISPID Generation.  Also, for vtable computation.
    METHOD_REFDTOCLASS,
    METHOD_VARARG,
    METHOD_CANCELABLE,
    METHOD_BUBBLING,
    METHOD_RESTRICTED,
    METHOD_THUNKCONTEXT,
    METHOD_THUNKNODECONTEXT,
    METHOD_MAXSTRLEN,
    METHOD_NOPROPDESC,          // indicates this method isn't exposed in DISPInterface or primary interface.
    METHOD_EXCLUSIVETOSCRIPT,   // expose this property on primary interface if any name clashes in
                                // other supported interfaces.
    METHOD_SZINTERFACEEXPOSE    // Expose this method name for the interface should be used in
                                // conjunction with nopropdesc.
};

enum
{
    REFPROP_NAME, // Never used
    REFPROP_CLASSNAME,
    REFPROP_PROPERTYNAME
};

enum
{
    REFMETHOD_NAME, // Never used
    REFMETHOD_CLASSNAME,
    REFMETHOD_METHODNAME
};

enum 
{
    EVENT_NAME,
    EVENT_SUPER,
    EVENT_GUID,
    EVENT_ABSTRACT
};

enum
{
    IMPORT_NAME
};

enum
{
    TEAROFF_NAME,
    TEAROFF_INTERFACE,
    TEAROFF_BASEIMPL
};

enum
{
    TEAROFFMETHOD_NAME,
    TEAROFFMETHOD_MAPTO
};

enum
{
    STRUCT_NAME,
};

enum
{
    STRUCTMEMBER_NAME,
    STRUCTMEMBER_TYPE
};

extern AssociateDataType DataTypes[];
extern Associate vt[];
extern TokenDescriptor *AllDescriptors[ NUM_DESCRIPTOR_TYPES ];
extern TokenDescriptor MethodArgDescriptor;
extern CCachedAttrArrayInfo rgCachedAttrArrayInfo[];








