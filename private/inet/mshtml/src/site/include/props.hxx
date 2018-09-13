#ifndef I_PROPS_HXX_
#define I_PROPS_HXX_
#pragma INCMSG("--- Beg 'props.hxx'")

// This macro is potentially very dangerous as it doesn't take into acount scale factors, some UNIT values would not
// use the scale factor so be careful when using.
#define MAKEUNITVALUE(val,unit) ((((long)val)<<CUnitValue::NUM_TYPEBITS) | CUnitValue::unit)

#define MAKE_PIXEL_UNITVALUE(val) \
    ((((long)val * CUnitValue::TypeNames[CUnitValue::UNIT_PIXELS].wScaleMult)<<CUnitValue::NUM_TYPEBITS) | CUnitValue::UNIT_PIXELS)

#define MAKE_EM_UNITVALUE(val) \
    ((((long)(val * CUnitValue::TypeNames[CUnitValue::UNIT_EM].wScaleMult))<<CUnitValue::NUM_TYPEBITS) | CUnitValue::UNIT_EM)

#define NULL_UNITVALUE MAKEUNITVALUE(0, UNIT_NULLVALUE)
#define ZEROPIXELS MAKEPIXELUNITVALUE(0)


#define BEGIN_DEFINE_ENUMS(CLASS, NAME, NUM)           \
    const ENUMDESC s_enumdesc##NAME = { NUM, {

#define DEFINE_ENUM(NAME, VALUE) { _T(#NAME), VALUE }
#define DEFINE_UNITVALUE_ENUM(NAME, VALUE, UNIT ) { _T(#NAME), MAKEUNITVALUE ( VALUE, UNIT ) }

#define END_DEFINE_ENUMS   } };



#define BEGIN_DEFINE_BITNAMES(CLASS, NAME, NUMVALUES)           \
    const BITMAPDESC s_bitmapdesc##NAME = { NUMVALUES, {
#define DEFINE_BITNAME(NAME) _T(#NAME) 
#define END_DEFINE_BITNAMES } };


#define BEGIN_DEFINE_BITMASK(CLASS, NAME, NUMVALUES)           \
    const BITMASKDESC s_bitmaskdesc##NAME = { NUMVALUES, {
#define DEFINE_BITMASK(NUMBITS, ISOPTIONAL) NUMBITS | ISOPTIONAL
#define END_DEFINE_BITMASK } };


#define BEGIN_BITMASK(CLANUMBITMASKS) \
    }, NUMBITMASKS, {




//+----------------------------------------------------------------------
//
// Attribute bag get/set macros.
//
//+----------------------------------------------------------------------

//
// Number
//
//+----------------------------------------------------------------------
//
// Object (non attribute bag) get/set macros.
//
//+----------------------------------------------------------------------

//
// Number
//

#define IMPLEMENT_NUM_GETPROP(CLASS, PROP, TYPE)                     \
    STDMETHODIMP CLASS::Get##PROP(TYPE * p)                                 \
        {                                                                   \
            return ((NUMPROPPARAMS *)((BYTE *)&s_propdesc##CLASS##PROP + sizeof(PROPERTYDESC)))->GetNumberProperty( p, this, this);    \
        }
#define IMPLEMENT_NUM_SETPROP(CLASS, PROP, TYPE)                     \
    STDMETHODIMP CLASS::Set##PROP(TYPE v)                                   \
        {                                                                   \
            return ((NUMPROPPARAMS *)((BYTE *)&s_propdesc##CLASS##PROP + sizeof(PROPERTYDESC)))->SetNumberProperty( v, this, this, 1, 1);  \
        }
#define IMPLEMENT_NUM_SETGETPROP(CLASS, PROP, TYPE)                  \
    IMPLEMENT_NUM_GETPROP(CLASS, PROP, TYPE)                         \
    IMPLEMENT_NUM_SETPROP(CLASS, PROP, TYPE)

//
// String
//

#define IMPLEMENT_CSTR_GETPROP(CLASS, PROP)                          \
    STDMETHODIMP CLASS::Get##PROP(BSTR *pbstr)                              \
        {                                                                   \
            return ((BASICPROPPARAMS *)((BYTE *)&s_propdesc##CLASS##PROP + sizeof(PROPERTYDESC)))->GetStringProperty(pbstr, this, this);    \
        }        
#define IMPLEMENT_CSTR_SETPROP(CLASS, PROP)                          \
    STDMETHODIMP CLASS::Set##PROP(BSTR bstr)                                \
        {                                                                   \
            return ((BASICPROPPARAMS *)((BYTE *)&s_propdesc##CLASS##PROP + sizeof(PROPERTYDESC)))->SetStringProperty(bstr, this, this);  \
        }
#define IMPLEMENT_CSTR_SETGETPROP(CLASS, PROP)                       \
    IMPLEMENT_CSTR_GETPROP(CLASS, PROP)                              \
    IMPLEMENT_CSTR_SETPROP(CLASS, PROP)


//
// Color
//

#define IMPLEMENT_COLOR_GETPROP(CLASS, PROP, TYPE)                   \
    STDMETHODIMP CLASS::Get##PROP(TYPE * p)                                 \
        {                                                                   \
            return ((BASICPROPPARAMS *)((BYTE *)&s_propdesc##CLASS##PROP + sizeof(PROPERTYDESC)))->GetColorProperty( p, this, this);    \
        }
#define IMPLEMENT_COLOR_SETPROP(CLASS, PROP, TYPE)                   \
    STDMETHODIMP CLASS::Set##PROP(TYPE v)                                   \
        {                                                                   \
            return ((BASICPROPPARAMS *)((BYTE *)&s_propdesc##CLASS##PROP + sizeof(PROPERTYDESC)))->SetColorProperty( v, this, this);    \
        }
#define IMPLEMENT_COLOR_SETGETPROP(CLASS, PROP, TYPE)                \
    IMPLEMENT_COLOR_GETPROP(CLASS, PROP, TYPE)                       \
    IMPLEMENT_COLOR_SETPROP(CLASS, PROP, TYPE)

//
// Enum
//

#define IMPLEMENT_ENUM_GETPROP(CLASS, PROP, TYPE)                    \
    STDMETHODIMP CLASS::Get##PROP(TYPE * p)                                 \
        {                                                                   \
            return ((NUMPROPPARAMS *)((BYTE *)&s_propdesc##CLASS##PROP + sizeof(PROPERTYDESC)))->GetNumberProperty( p, this, this);    \
        }
#define IMPLEMENT_ENUM_SETPROP(CLASS, PROP, TYPE)                    \
    STDMETHODIMP CLASS::Set##PROP(TYPE v)                                   \
        {                                                                   \
            return ((NUMPROPPARAMS *)((BYTE *)&s_propdesc##CLASS##PROP + sizeof(PROPERTYDESC)))->SetNumberProperty( v, this, this, 1, 1);  \
        }
#define IMPLEMENT_ENUM_SETGETPROP(CLASS, PROP, TYPE)                 \
    IMPLEMENT_ENUM_GETPROP(CLASS, PROP, TYPE)                        \
    IMPLEMENT_ENUM_SETPROP(CLASS, PROP, TYPE)

#pragma INCMSG("--- End 'props.hxx'")
#else
#pragma INCMSG("*** Dup 'props.hxx'")
#endif
