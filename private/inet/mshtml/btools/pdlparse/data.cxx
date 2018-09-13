// The names of these strings must correspond to the
// enum CACHED_CAA_STORAGE_STRUCT that is defined in cfpf.hxx
const char *rgszCcssString[]=
{
    "CCSS_EXPAND",
    "CCSS_CUSTOMAPPLY",
    "CCSS_NONE",
    "CCSS_CCHARFORMAT",
    "CCSS_CPARAFORMAT",
    "CCSS_CFANCYFORMAT"
};



// This needs to be ordered by DISPID
CCachedAttrArrayInfo rgCachedAttrArrayInfo[] =
{
    {"STDPROPID_XOBJ_LEFT",                 CCSSF_CLEARFF },
    {"STDPROPID_XOBJ_TOP",                  CCSSF_CLEARFF },
    {"STDPROPID_XOBJ_WIDTH",                CCSSF_CLEARCACHES | CCSSF_SIZECHANGED },
    {"STDPROPID_XOBJ_HEIGHT",               CCSSF_CLEARCACHES | CCSSF_SIZECHANGED },
    {"STDPROPID_XOBJ_BLOCKALIGN",           CCSSF_CLEARCACHES | CCSSF_REMEASURECONTENTS },
    {"STDPROPID_XOBJ_CONTROLALIGN",         CCSSF_CLEARCACHES | CCSSF_REMEASURECONTENTS },
    {"STDPROPID_XOBJ_DISABLED",             CCSSF_CLEARCACHES | CCSSF_REMEASURECONTENTS },
    {"STDPROPID_XOBJ_RIGHT",                CCSSF_CLEARFF },
    {"STDPROPID_XOBJ_BOTTOM",               CCSSF_CLEARFF },
    {"DISPID_A_VERTICALALIGN",              CCSSF_CLEARCACHES | CCSSF_SIZECHANGED },
    {"DISPID_A_COLOR",                      CCSSF_CLEARCACHES },
    {"DISPID_A_TEXTTRANSFORM",              CCSSF_CLEARCACHES | CCSSF_REMEASUREALLCONTENTS },
    {"DISPID_A_NOWRAP",                     CCSSF_CLEARCACHES | CCSSF_REMEASUREALLCONTENTS },
    {"DISPID_A_LINEHEIGHT",                 CCSSF_CLEARCACHES | CCSSF_REMEASUREALLCONTENTS },
    {"DISPID_A_TEXTINDENT",                 CCSSF_CLEARCACHES | CCSSF_REMEASUREALLCONTENTS },
    {"DISPID_A_LETTERSPACING",              CCSSF_CLEARCACHES | CCSSF_REMEASUREALLCONTENTS },
    {"DISPID_A_OVERFLOW",                   CCSSF_CLEARFF     | CCSSF_SIZECHANGED },
    {"DISPID_A_PADDINGTOP",                 CCSSF_CLEARCACHES | CCSSF_REMEASURECONTENTS },
    {"DISPID_A_PADDINGRIGHT",               CCSSF_CLEARCACHES | CCSSF_REMEASURECONTENTS },
    {"DISPID_A_PADDINGBOTTOM",              CCSSF_CLEARCACHES | CCSSF_REMEASURECONTENTS },
    {"DISPID_A_PADDINGLEFT",                CCSSF_CLEARCACHES | CCSSF_REMEASURECONTENTS },
    {"DISPID_A_CLEAR",                      CCSSF_CLEARCACHES | CCSSF_SIZECHANGED },
    {"DISPID_A_FONTFACE",                   CCSSF_CLEARCACHES | CCSSF_REMEASUREALLCONTENTS },
    {"DISPID_A_TEXTDECORATION",             CCSSF_CLEARCACHES },
    {"DISPID_A_ACCELERATOR",                CCSSF_CLEARCACHES },
    {"DISPID_A_FONTSIZE",                   CCSSF_CLEARCACHES | CCSSF_REMEASUREALLCONTENTS },
    {"DISPID_A_FONTSTYLE",                  CCSSF_CLEARCACHES | CCSSF_REMEASUREALLCONTENTS },
    {"DISPID_A_FONTVARIANT",                CCSSF_CLEARCACHES | CCSSF_REMEASUREALLCONTENTS },
    {"DISPID_A_BASEFONT",                   CCSSF_CLEARCACHES | CCSSF_REMEASUREALLCONTENTS },
    {"DISPID_A_FONTWEIGHT",                 CCSSF_CLEARCACHES | CCSSF_REMEASUREALLCONTENTS },

    {"DISPID_A_TABLEBORDERCOLOR",           CCSSF_CLEARFF     },
    {"DISPID_A_TABLEBORDERCOLORLIGHT",      CCSSF_CLEARFF     },
    {"DISPID_A_TABLEBORDERCOLORDARK",       CCSSF_CLEARFF     },
    {"DISPID_A_TABLEVALIGN",                CCSSF_CLEARCACHES | CCSSF_REMEASURECONTENTS },

    {"DISPID_BACKCOLOR",                    CCSSF_CLEARCACHES },
    
    {"DISPID_A_MARGINTOP",                  CCSSF_CLEARCACHES | CCSSF_SIZECHANGED },
    {"DISPID_A_MARGINRIGHT",                CCSSF_CLEARCACHES | CCSSF_SIZECHANGED },
    {"DISPID_A_MARGINBOTTOM",               CCSSF_CLEARCACHES | CCSSF_SIZECHANGED },
    {"DISPID_A_MARGINLEFT",                 CCSSF_CLEARCACHES | CCSSF_SIZECHANGED },

    {"DISPID_A_FONT",                       CCSSF_CLEARCACHES | CCSSF_REMEASUREALLCONTENTS },

    {"DISPID_A_BORDERTOPCOLOR",             CCSSF_CLEARCACHES },
    {"DISPID_A_BORDERRIGHTCOLOR",           CCSSF_CLEARCACHES },
    {"DISPID_A_BORDERBOTTOMCOLOR",          CCSSF_CLEARCACHES },
    {"DISPID_A_BORDERLEFTCOLOR",            CCSSF_CLEARCACHES },
    {"DISPID_A_BORDERTOPWIDTH",             CCSSF_CLEARCACHES | CCSSF_SIZECHANGED },
    {"DISPID_A_BORDERRIGHTWIDTH",           CCSSF_CLEARCACHES | CCSSF_SIZECHANGED },
    {"DISPID_A_BORDERBOTTOMWIDTH",          CCSSF_CLEARCACHES | CCSSF_SIZECHANGED },
    {"DISPID_A_BORDERLEFTWIDTH",            CCSSF_CLEARCACHES | CCSSF_SIZECHANGED },
    {"DISPID_A_BORDERTOPSTYLE",             CCSSF_CLEARCACHES | CCSSF_SIZECHANGED },
    {"DISPID_A_BORDERRIGHTSTYLE",           CCSSF_CLEARCACHES | CCSSF_SIZECHANGED },
    {"DISPID_A_BORDERBOTTOMSTYLE",          CCSSF_CLEARCACHES | CCSSF_SIZECHANGED },
    {"DISPID_A_BORDERLEFTSTYLE",            CCSSF_CLEARCACHES | CCSSF_SIZECHANGED },

    {"DISPID_A_FLOAT",                      CCSSF_CLEARCACHES | CCSSF_SIZECHANGED },
    {"DISPID_A_DISPLAY",                    CCSSF_CLEARCACHES | CCSSF_REMEASUREINPARENT },
    {"DISPID_A_LISTTYPE",                   CCSSF_CLEARCACHES | CCSSF_SIZECHANGED },
    {"DISPID_A_LISTSTYLETYPE",              CCSSF_CLEARCACHES | CCSSF_SIZECHANGED },
    {"DISPID_A_LISTSTYLEPOSITION",          CCSSF_CLEARCACHES | CCSSF_REMEASURECONTENTS | CCSSF_SIZECHANGED },
    {"DISPID_A_LISTSTYLEIMAGE",             CCSSF_CLEARCACHES | CCSSF_SIZECHANGED },
    {"DISPID_A_WHITESPACE",                 CCSSF_CLEARCACHES | CCSSF_REMEASUREALLCONTENTS },
    {"DISPID_A_VISIBILITY",                 CCSSF_CLEARCACHES },
    {"DISPID_A_POSITION",                   CCSSF_CLEARCACHES | CCSSF_REMEASUREINPARENT },
    {"DISPID_A_ZINDEX",                     CCSSF_CLEARFF     },
    {"DISPID_A_CLIP",                       CCSSF_CLEARFF     },
    {"DISPID_A_CLIPRECTTOP",                CCSSF_CLEARFF     },
    {"DISPID_A_CLIPRECTRIGHT",              CCSSF_CLEARFF     },
    {"DISPID_A_CLIPRECTBOTTOM",             CCSSF_CLEARFF     },
    {"DISPID_A_CLIPRECTLEFT",               CCSSF_CLEARFF     },
    {"DISPID_A_PAGEBREAKBEFORE",            CCSSF_CLEARCACHES },
    {"DISPID_A_PAGEBREAKAFTER",             CCSSF_CLEARCACHES },
    {"DISPID_A_CURSOR",                     CCSSF_CLEARCACHES },
    {"DISPID_A_FILTER",                     CCSSF_CLEARCACHES },
    {"DISPID_A_BACKGROUNDIMAGE",            CCSSF_CLEARCACHES },
    {"DISPID_A_BACKGROUNDPOSX",             CCSSF_CLEARFF     },
    {"DISPID_A_BACKGROUNDPOSY",             CCSSF_CLEARFF     },
    {"DISPID_A_BACKGROUNDREPEAT",           CCSSF_CLEARFF     },
    {"DISPID_A_BACKGROUNDATTACHMENT",       CCSSF_CLEARFF     | CCSSF_SIZECHANGED },
    {"DISPID_A_LANG",                       CCSSF_CLEARCACHES | CCSSF_REMEASUREALLCONTENTS },
    {"DISPID_A_TABLELAYOUT",                CCSSF_CLEARCACHES | CCSSF_REMEASUREALLCONTENTS },
    {"DISPID_A_BORDERCOLLAPSE",             CCSSF_CLEARCACHES | CCSSF_REMEASUREALLCONTENTS },

    {"DISPID_A_BEHAVIOR",                   CCSSF_CLEARCACHES },
    {"DISPID_A_DIR",                        CCSSF_CLEARCACHES | CCSSF_REMEASUREALLCONTENTS | CCSSF_SIZECHANGED }, 
    {"DISPID_A_UNICODEBIDI",                CCSSF_CLEARCACHES | CCSSF_REMEASUREALLCONTENTS | CCSSF_SIZECHANGED }, 
    {"DISPID_A_DIRECTION",                  CCSSF_CLEARCACHES | CCSSF_REMEASUREALLCONTENTS | CCSSF_SIZECHANGED }, 
    {"DISPID_A_RUBYPOSITION",               CCSSF_CLEARCACHES }, 
    {"DISPID_A_IMEMODE",                    CCSSF_NONE }, 
    {"DISPID_A_RUBYALIGN",                  CCSSF_CLEARCACHES | CCSSF_REMEASURECONTENTS | CCSSF_SIZECHANGED }, 
    {"DISPID_A_RUBYPOSITION",               CCSSF_CLEARCACHES | CCSSF_REMEASURECONTENTS | CCSSF_SIZECHANGED }, 
    {"DISPID_A_RUBYOVERHANG",               CCSSF_CLEARCACHES | CCSSF_REMEASURECONTENTS | CCSSF_SIZECHANGED }, 
    {"DISPID_A_LAYOUTGRIDCHAR",             CCSSF_CLEARCACHES | CCSSF_REMEASURECONTENTS | CCSSF_SIZECHANGED }, 
    {"DISPID_A_LAYOUTGRIDLINE",             CCSSF_CLEARCACHES | CCSSF_REMEASURECONTENTS | CCSSF_SIZECHANGED }, 
    {"DISPID_A_LAYOUTGRIDMODE",             CCSSF_CLEARCACHES | CCSSF_REMEASURECONTENTS | CCSSF_SIZECHANGED }, 
    {"DISPID_A_LAYOUTGRIDTYPE",             CCSSF_CLEARCACHES | CCSSF_REMEASURECONTENTS | CCSSF_SIZECHANGED }, 
    {"DISPID_A_LAYOUTGRID",                 CCSSF_CLEARCACHES | CCSSF_REMEASURECONTENTS | CCSSF_SIZECHANGED }, 
    {"DISPID_A_TEXTAUTOSPACE",              CCSSF_CLEARCACHES | CCSSF_REMEASUREALLCONTENTS | CCSSF_SIZECHANGED }, 
    {"DISPID_A_WORDBREAK",                  CCSSF_CLEARCACHES | CCSSF_REMEASUREALLCONTENTS | CCSSF_SIZECHANGED }, 
    {"DISPID_A_LINEBREAK",                  CCSSF_CLEARCACHES | CCSSF_REMEASUREALLCONTENTS | CCSSF_SIZECHANGED }, 
    {"DISPID_A_TEXTJUSTIFY",                CCSSF_CLEARCACHES | CCSSF_REMEASUREALLCONTENTS | CCSSF_SIZECHANGED }, 
    {"DISPID_A_TEXTJUSTIFYTRIM",            CCSSF_CLEARCACHES | CCSSF_REMEASUREALLCONTENTS | CCSSF_SIZECHANGED }, 
    {"DISPID_A_TEXTKASHIDA",                CCSSF_CLEARCACHES | CCSSF_REMEASUREALLCONTENTS | CCSSF_SIZECHANGED }, 

    {"DISPID_A_OVERFLOWX",                  CCSSF_CLEARFF     | CCSSF_SIZECHANGED },
    {"DISPID_A_OVERFLOWY",                  CCSSF_CLEARFF     | CCSSF_SIZECHANGED },
    {NULL,                                  CCSSF_NONE },
};

AssociateDataType DataTypes[] =
{
    { "void*",                 "Num" ,      "NUM",          STORAGETYPE_NUMBER},
    { "DWORD",                 "Num" ,      "NUM",          STORAGETYPE_NUMBER},
    { "DISPID",                "Num" ,      "NUM",          STORAGETYPE_NUMBER},
    { "SIZEL*",                "Num" ,      "NUM",          STORAGETYPE_NUMBER},
    { "CStr",                  "String" ,   "CSTR",         STORAGETYPE_STRING},
    { "BSTR",                  "String" ,   "CSTR",         STORAGETYPE_STRING},
    { "long",                  "Num" ,      "NUM",          STORAGETYPE_NUMBER},
    { "int",                   "Num" ,      "NUM",          STORAGETYPE_NUMBER},
    { "short",                 "Num" ,      "NUM",          STORAGETYPE_NUMBER},
    { "char",                  "Num" ,      "NUM",          STORAGETYPE_NUMBER},
    { "enum",                  "Enum" ,     "ENUM",         STORAGETYPE_NUMBER},
    { "ULONG",                 "Num" ,      "NUM",          STORAGETYPE_NUMBER},
    { "CStyleComponent",    "StyleComponent" , "CSTR",          STORAGETYPE_STRING},
    { "CStyle",                "Style" ,    "CSTR",         STORAGETYPE_OTHER},
    { "CColorValue",           "Color" ,    "COLOR",        STORAGETYPE_NUMBER},
    { "CUnitValue",            "UnitValue" , "UNITVALUE",   STORAGETYPE_NUMBER},
    { "VARIANT_BOOL",          "Num" ,       "NUM",         STORAGETYPE_NUMBER},
    { "OLE_XPOS_PIXELS",       "Num" ,       "NUM",         STORAGETYPE_NUMBER},
    { "OLE_YPOS_PIXELS",       "Num" ,       "NUM",         STORAGETYPE_NUMBER},
    { "OLE_XSIZE_PIXELS",      "Num" ,       "NUM",         STORAGETYPE_NUMBER},
    { "OLE_YSIZE_PIXELS",      "Num" ,       "NUM",         STORAGETYPE_NUMBER},
    { "OLE_XPOS_HIMETRIC",     "Num" ,       "NUM",         STORAGETYPE_NUMBER},
    { "OLE_YPOS_HIMETRIC",     "Num" ,       "NUM",         STORAGETYPE_NUMBER},
    { "OLE_XSIZE_HIMETRIC",    "Num" ,       "NUM",         STORAGETYPE_NUMBER},
    { "OLE_YSIZE_HIMETRIC",    "Num" ,       "NUM",         STORAGETYPE_NUMBER},
    { "OLE_XPOS_CONTAINER",    "Float" ,     "FLOAT",       STORAGETYPE_NUMBER},
    { "OLE_YPOS_CONTAINER",    "Float" ,     "FLOAT",       STORAGETYPE_NUMBER},
    { "OLE_XSIZE_CONTAINER",   "Float" ,     "FLOAT",       STORAGETYPE_NUMBER},
    { "OLE_YSIZE_CONTAINER",   "Float" ,     "FLOAT",       STORAGETYPE_NUMBER},
    { "OLE_CANCELBOOL",        "Num" ,       "NUM",         STORAGETYPE_NUMBER},
    { "OLE_ENABLEDEFAULTBOOL", "Num" ,       "NUM",         STORAGETYPE_NUMBER},
    { "IDispatch*",            "Object" ,    "OBJECT",      STORAGETYPE_OTHER},
    { "IUnknown*",             "Object"  ,   "OBJECT",      STORAGETYPE_OTHER},
    { "VARIANT",               "Variant"   , "VARIANT",     STORAGETYPE_OTHER},
    { "BYTE",                  "Num" ,       "NUM",         STORAGETYPE_NUMBER},
    { "BOOL",                  "Boolean" ,   "BOOLEAN",     STORAGETYPE_NUMBER},
    { "float",                 "Float" ,     "FLOAT",       STORAGETYPE_NUMBER},
    { "url",                   "Url",        "CSTR",        STORAGETYPE_STRING},
    { "code",                  "Code",       "CSTR",        STORAGETYPE_STRING},
    { NULL,                    NULL ,        NULL,          STORAGETYPE_NUMBER},
};


    //# { ]
    //#   index is argument type
    //#   value is VTS type

Associate vt[] =
{
    // BUBUG rgardner not sure of void* this check with istvanc
    { "void*", "VTS_PI4" },
    { "DWORD", "VTS_I4" },
    { "DISPID", "VTS_I4" },
    // BUBUG rgardner not sure of SIZEL* this check with istvanc
    { "SIZEL*", "VTS_PI4" },
    { "short", "VTS_I2" },
    { "long", "VTS_I4" },
    { "ULONG", "VTS_I4" },
    { "ULONG*", "VTS_PI4" },
    { "int", "VTS_I4" },
    { "float", "VTS_R4" },
    { "double", "VTS_R8" },
    { "CY*", "VTS_CY" },
    { "DATE", "VTS_DATE" },
    { "BSTR", "VTS_BSTR" },
    { "LPCTSTR", "VTS_BSTR" },
    { "IDispatch*", "VTS_DISPATCH" },
    { "SCODE", "VTS_ERROR" },
    { "VARIANT_BOOL", "VTS_BOOL" },
    { "boolean", "VTS_BOOL" },
    { "ERROR", "VTS_ERROR" },
    { "BOOL", "VTS_BOOL" },
    { "VARIANT", "VTS_VARIANT" },
    { "IUnknown*", "VTS_UNKNOWN" },
    { "unsigned char", "VTS_UI1" },
    { "short*", "VTS_PI2" },
    { "long*", "VTS_PI4" },
    { "int*", "VTS_PI4" },
    { "float*", "VTS_PR4" },
    { "double*", "VTS_PR8" },
    { "CY*", "VTS_PCY" },
    { "DATE*", "VTS_PDATE" },
    { "BSTR*", "VTS_PBSTR" },
    { "IDispatch**", "VTS_PDISPATCH" },
    { "SCODE*", "VTS_PERROR" },
    { "VARIANT_BOOL*", "VTS_PBOOL" },
    { "boolean*", "VTS_PBOOL" },
    { "VARIANT*", "VTS_PVARIANT" },
    { "IUnknown**", "VTS_PUNKNOWN" },
    { "unsigned char *", "VTS_PUI1" },
    { "", "VTS_NONE" },
    { "OLE_COLOR", "VTS_COLOR" },
    { "OLE_XPOS_PIXELS", "VTS_XPOS_PIXELS" },
    { "OLE_YPOS_PIXELS", "VTS_YPOS_PIXELS" },
    { "OLE_XSIZE_PIXELS", "VTS_XSIZE_PIXELS" },
    { "OLE_YSIZE_PIXELS", "VTS_YSIZE_PIXELS" },
    { "OLE_XPOS_HIMETRIC", "VTS_XPOS_HIMETRIC" },
    { "OLE_YPOS_HIMETRIC", "VTS_YPOS_HIMETRIC" },
    { "OLE_XSIZE_HIMETRIC", "VTS_XSIZE_HIMETRIC" },
    { "OLE_YSIZE_HIMETRIC", "VTS_YSIZE_HIMETRIC" },
    { "OLE_XPOS_CONTAINER", "VTS_R4" },
    { "OLE_YPOS_CONTAINER", "VTS_R4" },
    { "OLE_XSIZE_CONTAINER", "VTS_R4" },
    { "OLE_YSIZE_CONTAINER", "VTS_R4" },
    { "OLE_TRISTATE", "VTS_TRISTATE" },
    { "OLE_OPTEXCLUSIVE", "VTS_OPTEXCLUSIVE" },
    { "OLE_COLOR*", "VTS_PCOLOR" },
    { "OLE_XPOS_PIXELS*", "VTS_PXPOS_PIXELS" },
    { "OLE_YPOS_PIXELS*", "VTS_PYPOS_PIXELS" },
    { "OLE_XSIZE_PIXELS*", "VTS_PXSIZE_PIXELS" },
    { "OLE_YSIZE_PIXELS*", "VTS_PYSIZE_PIXELS" },
    { "OLE_XPOS_HIMETRIC*", "VTS_PXPOS_HIMETRIC" },
    { "OLE_YPOS_HIMETRIC*", "VTS_PYPOS_HIMETRIC" },
    { "OLE_XSIZE_HIMETRIC*", "VTS_PXSIZE_HIMETRIC" },
    { "OLE_YSIZE_HIMETRIC*", "VTS_PYSIZE_HIMETRIC" },
    { "OLE_TRISTATE*", "VTS_PTRISTATE" },
    { "OLE_OPTEXCLUSIVE*", "VTS_POPTEXCLUSIVE" },
    { "IFontDispatch*", "VTS_FONT" },
    { "IPictureDispatch*", "VTS_PICTURE" },
    { "OLE_HANDLE", "VTS_HANDLE" },
    { "OLE_HANDLE*", "VTS_PHANDLE" },
    { "BYTE", "VTS_BOOL" },
    // Internal Types
    { "CStr", "VTS_BSTR" },
    { "CUnitValue", "VTS_I4" },
    { "CStyleComponent", "VTS_BSTR" },
    { "CStyle", "VTS_BSTR" },
    { "CColorValue", "VTS_BSTR" },
    { NULL, NULL },
};


TokenDescriptor FileDescriptor =
{
    "file", TRUE,
    {
        END_OF_ARG_ARRAY
    }
};


TokenDescriptor InterfaceDescriptor =
{
    "interface", TRUE,
    {
        {
            "super", FALSE, TRUE    // Mandatory
        },
        {
            "guid", FALSE, FALSE
        },
        {
            "abstract", TRUE, FALSE
        },
        {
            "custom", TRUE, FALSE
        },
        {
            "<noprimary>", TRUE, FALSE  // Not settable from pdl, set internally.
        },
        END_OF_ARG_ARRAY
    }
};


TokenDescriptor EvalDescriptor =
{
    "eval", FALSE,
    {
        {
            "value", FALSE, FALSE
        },
        {
            "string", FALSE, FALSE
        },
        {
            "name", FALSE, FALSE
        },
        END_OF_ARG_ARRAY
    }
};


TokenDescriptor EnumDescriptor =
{
    "enum", TRUE,
    {
        {
            "prefix", FALSE, FALSE
        },
        {
            "guid", FALSE, FALSE
        },
        {
            "hidden", TRUE, FALSE
        },
        END_OF_ARG_ARRAY
    },
};


TokenDescriptor ClassDescriptor =
{
    "class", TRUE,
    {

        {
            "super", FALSE, FALSE
        },
        {
            "primaryinterface", FALSE, FALSE
        },
        {
            "guid", FALSE, FALSE
        },
        {
            "abstract", TRUE, FALSE
        },
        {
            "name", FALSE, FALSE
        },
        {
            "events", FALSE, FALSE
        },
        {
            "cascadedmethods", TRUE, FALSE
        },
        {
            "noaamethods", TRUE, FALSE
        },
        {
            "keepnopersist", TRUE, FALSE
        },
        {
            "noconnectionpoints", TRUE, FALSE
        },
        {
            "control", TRUE, FALSE
        },
        {
            "mondoguid", FALSE, FALSE
        },
        {
            "nonprimaryevents", FALSE, FALSE
        },
        {
            "nonprimaryevents2", FALSE, FALSE
        },
        {
            "nonprimaryevents3", FALSE, FALSE
        },
        {
            "nonprimaryevents4", FALSE, FALSE
        },
        END_OF_ARG_ARRAY
    }
};


TokenDescriptor ImplementsDescriptor =
{
    "implements", FALSE,
    {
        {
            "guid", FALSE, FALSE
        },
        END_OF_ARG_ARRAY
    }
};


TokenDescriptor PropertyDescriptor =
{
    "property", FALSE,
    {
        {
            "atype", FALSE, FALSE
        },
        {
            "dispid", FALSE, FALSE
        },
        {
            "type", FALSE, FALSE
        },
        {
            "member", FALSE, FALSE
        },
        {
            "get", TRUE, FALSE
        },
        {
            "set", TRUE, FALSE
        },
        {
            "bindable", TRUE, FALSE
        },
        {
            "displaybind", TRUE, FALSE
        },
        {
            "dwflags", FALSE, FALSE
        },
        {
            "abstract", TRUE, FALSE
        },
        {
            "ppflags", FALSE, FALSE
        },
        {
            "default", FALSE, FALSE
        },
        {
            "noassigndefault", FALSE, FALSE
        },
        {
            "min", FALSE, FALSE
        },
        {
            "max", FALSE, FALSE
        },
        {
            "method", TRUE, FALSE
        },
        {
            "help", FALSE, FALSE
        },
        {
            "vt", FALSE, FALSE
        },
        {
            "caa", FALSE, FALSE
        },
        {
            "object", FALSE, FALSE
        },
/*  I'm creating bizarre names these out for now because currently JScript cannot deal with
    Properties that have parameters */
        {
            "<<index>>", FALSE, FALSE
        },
        {
            "<<index1>>", FALSE, FALSE
        },
        {
            "<<indextype>>", FALSE, FALSE
        },
        {
            "<<indextype1>>", FALSE, FALSE
        },
        {
            "szattribute", FALSE, FALSE
        },
        {
            "hidden", TRUE, FALSE
        },
        {
            "nonbrowsable", TRUE, FALSE
        },
        {
            "restricted", TRUE, FALSE
        },
        {
            "subobject", FALSE, FALSE
        },
        {
            "param1", FALSE, FALSE
        },
        {
            "precallfn", FALSE, FALSE
        },
        {
            "enumref", FALSE, FALSE
        },
        {
            "virtual", TRUE, FALSE
        },
        {
            "getaa", FALSE, FALSE
        },
        {
            "setaahr", FALSE, FALSE
        },
        {
            "source", TRUE, FALSE
        },
        {
            "minout", TRUE, FALSE
        },
        {
            "<cascaded>", TRUE,FALSE
        },
        {
            "updatecollection", TRUE, FALSE
        },
        {
            "scriptlet", TRUE, FALSE
        },
        {
            "clearcaches", TRUE, FALSE
        },
        {
            "stylesheetproperty", TRUE, FALSE
        },
        {
            "noassigntypeonly", TRUE, FALSE
        },
        {
            "resize", TRUE, FALSE
        },
        {
            "remeasure", TRUE, FALSE
        },
        {
            "remeasureall", TRUE, FALSE
        },
        {
            "siteredraw", TRUE, FALSE
        },
        {
            "set_designtime", TRUE, FALSE
        },
        {
            "invalid=noassigndefault", TRUE, FALSE
        },
        {
            "notpresent=default", TRUE, FALSE
        },
        {
            "contextual", TRUE, FALSE
        },
        {
            "nodecontextual", TRUE, FALSE
        },
        {
            "baseimplementation", TRUE, FALSE
        },
        {
            "nopersist", TRUE, FALSE
        },
        {
            "internal", TRUE, FALSE
        },
        {
            "<refdclass>", TRUE, FALSE
        },
        {
            "maxstrlen", FALSE, FALSE
        },
        {
            "nopropdesc", TRUE, FALSE
        },
        {
            "exclusivetoscript", TRUE, FALSE
        },
        {
            "szInterfaceExpose", TRUE, FALSE
        },
        {
            "dataevent", TRUE, FALSE
        },
        {
            "accessibilitystate", FALSE, FALSE
        },
        END_OF_ARG_ARRAY
    }
};


TokenDescriptor MethodArgDescriptor =
{
    "<methodarg>" /* Implied */, FALSE,
    {
        {
            "atype", FALSE, FALSE
        },
        {
            "type", FALSE, FALSE
        },
        {
            "in", TRUE, FALSE
        },
        {
            "out", TRUE, FALSE
        },
        {
            "arg", FALSE, FALSE
        },
        {
            "retval", FALSE, FALSE
        },
        {
            "defaultvalue", FALSE, FALSE
        },
        {
            "optional", TRUE, FALSE
        },
        END_OF_ARG_ARRAY
    }
};


// These are just the tokens for the method, there is a separate descriptor for a method arg
TokenDescriptor MethodDescriptor =
{
    "method", FALSE,
    {
        {
            "<ReturnType>", FALSE, FALSE // Un-Tagged
        },
        {
            "dispid", FALSE, FALSE
        },
        {
            "abstract", TRUE, FALSE
        },
        {
            "virtual", TRUE, FALSE
        },
        {
            "<RefdToClassFlag>", TRUE, FALSE
        },
        {
            "vararg", TRUE, FALSE
        },
        {
            "cancelable", TRUE, FALSE
        },
        {
            "bubbling", TRUE, FALSE
        },
        {
            "restricted", TRUE, FALSE
        },
        {   
            "contextual", TRUE, FALSE
        },
        {   
            "nodecontextual", TRUE, FALSE
        },
        {   
            "maxstrlen", FALSE, FALSE
        },
        {
            "nopropdesc", TRUE, FALSE
        },
        {
            "exclusivetoscript", TRUE, FALSE
        },
        {
            "szInterfaceExpose", TRUE, FALSE
        },
        END_OF_ARG_ARRAY
    }
};


TokenDescriptor RefPropDescriptor =
{
    "refprop", FALSE,
    {
        {
            "<ClassName>", FALSE, FALSE // Un-Tagged
        },
        {
            "<PropertyName>", FALSE, FALSE // Untagged
        },
        END_OF_ARG_ARRAY
    }
};


TokenDescriptor RefMethodDescriptor =
{
    "refmethod", FALSE,
    {
        {
            "<ClassName>", FALSE, FALSE // Un-Tagged
        },
        {
            "<PropertyName>", FALSE, FALSE // Untagged
        },
        END_OF_ARG_ARRAY
    }
};


TokenDescriptor EventDescriptor =
{
    "event", TRUE,
    {
        {
            "super", FALSE, TRUE    // Mandatory
        },
        {
            "guid", FALSE, FALSE
        },
        {
            "abstract", TRUE, FALSE
        },
        END_OF_ARG_ARRAY
    }
};


TokenDescriptor ImportDescriptor =
{
    "import", TRUE, // import just has a name
    {
        END_OF_ARG_ARRAY
    }
};


TokenDescriptor TearoffDescriptor =
{
    "tearoff", TRUE,
    {
        {
            "interface", FALSE, FALSE
        },
        {
            "BaseImpl", FALSE, FALSE
        },
        END_OF_ARG_ARRAY
    }
};


TokenDescriptor TearoffMethod =
{
    "tearmethod", FALSE,
    {
        {
            "mapto", FALSE, TRUE    // mandatory
        },
        END_OF_ARG_ARRAY
    }
};

TokenDescriptor StructDescriptor =
{
    "struct", TRUE,
    {
        END_OF_ARG_ARRAY
    }
};

TokenDescriptor StructMemberDescriptor =
{
    "member", FALSE,
    {
        {
            "type", FALSE, FALSE
        },
        END_OF_ARG_ARRAY
    }
};


TokenDescriptor *AllDescriptors[ NUM_DESCRIPTOR_TYPES ] =
{
    &FileDescriptor,
    &InterfaceDescriptor,
    &EvalDescriptor,
    &EnumDescriptor,
    &ClassDescriptor,
    &ImplementsDescriptor,
    &PropertyDescriptor,
    &MethodDescriptor,
    &MethodArgDescriptor,
    &RefPropDescriptor,
    &RefMethodDescriptor,
    &EventDescriptor,
    &ImportDescriptor,
    &TearoffDescriptor,
    &TearoffMethod,
    &StructDescriptor,
    &StructMemberDescriptor
};
