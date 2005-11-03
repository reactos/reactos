#ifndef __OTL_TABLE_H__
#define __OTL_TABLE_H__

#include "otlayout.h"

OTL_BEGIN_HEADER

  typedef struct OTL_TableRec_*    OTL_Table;

  typedef enum
  {
    OTL_TABLE_TYPE_GDEF = 1,
    OTL_TABLE_TYPE_GSUB,
    OTL_TABLE_TYPE_GPOS,
    OTL_TABLE_TYPE_BASE,
    OTL_TABLE_TYPE_JSTF

  } OTL_TableType;


 /* this may become a private structure later */
  typedef struct OTL_TableRec_
  {
    OTL_TableType  type;
    OTL_Bytes      base;
    OTL_Bytes      limit;

    OTL_Tag        script_tag;
    OTL_Tag        lang_tag;

    OTL_UInt       lookup_count;
    OTL_Byte*      lookup_flags;

    OTL_UInt       feature_count;
    OTL_Tag        feature_tags;
    OTL_Byte*      feature_flags;

  } OTL_TableRec;


  OTL_API( OTL_Error )
  otl_table_validate( OTL_Bytes      table,
                      OTL_Size       size,
                      OTL_TableType  type,
                      OTL_Size      *abyte_size );

  OTL_API( void )
  otl_table_init( OTL_Table      table,
                  OTL_TableType  type,
                  OTL_Bytes      base,
                  OTL_Size       size );

  OTL_API( void )
  otl_table_set_script( OTL_Table      table,
                        OTL_ScriptTag  script,
                        OTL_LangTag    language );

OTL_END_HEADER

#endif /* __OTL_TABLE_H__ */
