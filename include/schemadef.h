#ifndef SCHEMA_STRINGS

# ifndef SCHEMADEF_H
#  define SCHEMADEF_H

#   define SCHEMADEF_VERSION (1)

struct TMPROPINFO
{
 LPCWSTR pszName;
 SHORT sEnumVal;
 BYTE bPrimVal;
};

struct TMSCHEMAINFO
{
 DWORD dwSize;
 int iSchemaDefVersion;
 int iThemeMgrVersion;
 int iPropCount;
 const struct TMPROPINFO * pPropTable;
};

#   define BEGIN_TM_SCHEMA(NAME__)               
#   define BEGIN_TM_PROPS() enum PropValues { DummyProp = 49,
#   define BEGIN_TM_ENUM(NAME__) enum NAME__ {

#   define BEGIN_TM_CLASS_PARTS(NAME__) \
 enum NAME__##PARTS { NAME__##PartFiller0,

#   define BEGIN_TM_PART_STATES(NAME__) \
 enum NAME__##STATES { NAME__##StateFiller0,


#   define TM_PROP(VAL__, PREFIX__, NAME__, PRIMVAL__) \
 PREFIX__##_##NAME__ = VAL__, 

#   define TM_ENUM(VAL__, PREFIX__, NAME__)  PREFIX__##_##NAME__ = VAL__,
#   define TM_PART(VAL__, PREFIX__, NAME__)  PREFIX__##_##NAME__ = VAL__, 
#   define TM_STATE(VAL__, PREFIX__, NAME__) PREFIX__##_##NAME__ = VAL__, 

#   define END_TM_CLASS_PARTS() };
#   define END_TM_PART_STATES() };
#   define END_TM_PROPS()       };
#   define END_TM_ENUM()        };
#   define END_TM_SCHEMA(NAME__)

#  endif

# else

#  undef BEGIN_TM_SCHEMA
#  undef BEGIN_TM_PROPS
#  undef BEGIN_TM_ENUM
#  undef BEGIN_TM_CLASS_PARTS
#  undef BEGIN_TM_PART_STATES
#  undef TM_PROP
#  undef TM_PART
#  undef TM_STATE
#  undef TM_ENUM
#  undef END_TM_CLASS_PARTS
#  undef END_TM_PART_STATES
#  undef END_TM_PROPS
#  undef END_TM_ENUM
#  undef END_TM_SCHEMA

#  define BEGIN_TM_SCHEMA(NAME__) static const TMPROPINFO NAME__[] = {
#  define BEGIN_TM_PROPS()
#  define BEGIN_TM_ENUM(NAME__) { L#NAME__, TMT_ENUMDEF, TMT_ENUMDEF },

#  define BEGIN_TM_CLASS_PARTS(NAME__) \
 { L#NAME__ L"PARTS", TMT_ENUMDEF, TMT_ENUMDEF },

#  define BEGIN_TM_PART_STATES(NAME__) \
 { L#NAME__ L"STATES", TMT_ENUMDEF, TMT_ENUMDEF },


#  define TM_PROP(VAL__, PREFIX__, NAME__, PRIMVAL__) \
 { L#NAME__, PREFIX__##_##NAME__, TMT_##PRIMVAL__ },

#  define TM_PART(VAL__, PREFIX__, NAME__) \
 { L#NAME__, PREFIX__##_##NAME__, TMT_ENUMVAL },

#  define TM_STATE(VAL__, PREFIX__, NAME__) \
 { L#NAME__, PREFIX__##_##NAME__, TMT_ENUMVAL },

#  define TM_ENUM(VAL__, PREFIX__, NAME__) \
 { L#NAME__, PREFIX__##_##NAME__, TMT_ENUMVAL },


#  define END_TM_CLASS_PARTS() 
#  define END_TM_PART_STATES() 
#  define END_TM_PROPS() 
#  define END_TM_ENUM()
#  define END_TM_SCHEMA(NAME__) \
 }; \
 \
 static const TMSCHEMAINFO * GetSchemaInfo(void) \
 { \
  static TMSCHEMAINFO si = { sizeof(si) }; \
  si.iSchemaDefVersion = SCHEMADEF_VERSION; \
  si.iThemeMgrVersion = THEMEMGR_VERSION; \
  si.iPropCount = sizeof(NAME__) / sizeof(NAME__[0]); \
  si.pPropTable = NAME__; \
 \
  return &si; \
 }

# endif

/* EOF */
