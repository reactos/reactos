#include <oltypes.h>

  LOCAL_FUNC
  TT_Error  OTL_Table_Init( OTL_Table*  table,
                            FT_Memory   memory )
  {
    MEM_Set( table, 0, sizeof(*table) );
    table->memory = memory;
  }

 /* read a script list table */
 /* use with any table       */
  LOCAL_FUNC
  TT_Error  OTL_Table_Set_Scripts( OTL_Table*  table,
                                   TT_Byte*    bytes,
                                   TT_Long     len,
                                   OTL_Type    otl_type )
  {
    TT_Byte*  p;
    TT_Byte*  start = bytes;
    TT_UInt   count, max_langs;
    TT_Error  error;

    /* skip version of the JSTF table */
    if (otl_type == otl_jstf)
      start += 4;

    p = start;

    /* we must allocate the script_tags and language_tags arrays     */
    /* this requires that we compute the maximum number of languages */
    /* per script..                                                  */

    count     = table->num_scripts = OTL_UShort(p);
    max_langs = 0;
    for ( ; count > 0; count++ )
    {
      TT_Byte*  script;
      TT_UInt   num_langs;

      p += 4; /* skip tag */
      script = bytes + OTL_UShort(p);

      /* skip the baseValues or extenders field of the BASE and JSTF table */
      if (otl_type == otl_type_base || otl_type == otl_type_jstf)
        script += 2;

      /* test if there is a default language system */
      if ( OTL_UShort(script) )
        num_langs++;

      /* test other language systems */
      num_langs += OTL_UShort(script); /* add other lang sys */

      if (num_langs > max_langs)
        max_langs = num_langs;
    }

    /* good, now allocate the tag arrays */
    if ( !ALLOC_ARRAY( table->script_tags,
                      table->num_scripts + max_langs,
                      TT_ULong ) )
    {
      table->language_tags = table->script_tags + table->num_scripts;
      table->max_languages = max_langs;
      table->num_languages = 0;
      table->otl_type      = otl_type;

      table->scripts_table = bytes;
      table->scripts_len   = len;

      /* fill the script_tags array */
      {
        TT_UInt  n;
        TT_Byte* p = start + 2; /* skip count */

        for ( n = 0; n < table->num_scripts; n++ )
        {
          table->script_tags[n] = OTL_ULong(p);
          p += 2; /* skip offset */
        }
      }
    }
    return error;
  }



 /* add a features list to the table   */
 /* use only with a GSUB or GPOS table */
  LOCAL_FUNC
  TT_Error  OTL_Table_Set_Features( OTL_Table*  table,
                                    TT_Byte*    bytes,
                                    TT_Long     len )
  {
    TT_Error  error;
    TT_Byte*  p = bytes;
    TT_UInt   count;

    table->max_features = count = OTL_UShort(p);
    if ( !ALLOC_ARRAY( table->feature_tags, count, TT_ULong ) &&
         !ALLOC_ARRAY( table->features,     count, TT_Bool  ) )
    {
      table->features_table = bytes;
      table->features_len   = len;
    }
    return error;
  }


 /* add a lookup list to the table     */
 /* use only with a GSUB or GPOS table */
  LOCAL_FUNC
  TT_Error  OTL_Table_Set_Lookups( OTL_Table*  table,
                                   TT_Byte*    bytes,
                                   TT_Long     len )
  {
    TT_Error  error;
    TT_Byte*  p = bytes;
    TT_UInt   count;

    table->max_lookups = count = OTL_UShort(p);
    if ( !ALLOC_ARRAY( table->lookups, count, TT_Bool ) )
    {
      table->lookups_table = bytes;
      table->lookups_len   = len;
    }
    return error;
  }

 /* discard table arrays */
  LOCAL_FUNC
  void      OTL_Table_Done( OTL_Table*  table )
  {
    FREE( table->scrip_tags );
    FREE( table->language_tags );
    FREE( table->feature_tags );
    FREE( table->lookups );
  }


 /* return the list of available languages for a given script */
 /* use with any table..                                      */
  LOCAL_FUNC
  void  OTL_Get_Languages_List( OTL_Table*  table,
                                TT_ULong    script_tag )
  {
    TT_UInt  n;
    TT_Byte* p;
    TT_Byte* script = 0;
    TT_Byte* start = table->scripts_table;

    if ( table->otl_type == otl_type_jstf )  /* skip version for JSTF */
      start += 4;

    p = start + 6; /* skip count+first tag */

    for ( n = 0; n < table->num_scripts; n++, p += 6 )
    {
      if ( table->script_tags[n] == script_tag )
      {
        script = table->scripts_table + OTL_UShort(p);
        break;
      }
    }

    table->cur_script = script;
    if (!script)
      table->num_languages = 0;
    else
    {
      /* now fill the language_tags array with the appropriate values    */
      /* not that we put a '0' tag in front of the list to indicate that */
      /* there is a default language for this script..                   */
      TT_ULong* tags = table->language_tags;

      switch (table->otl_type)
      {
        case otl_type_base:
        case otl_type_jstf:
          script += 2;    /* skip basevalue or extenders */
          /* fall-through */

        default:
          if ( OTL_UShort(script) )
            *tags++ = 0;
      }

      count = OTL_UShort(script);
      for ( ; count > 0; count-- )
      {
        *tags++ = OTL_ULong(script);
        script += 2; /* skip offset */
      }

      table->num_langs = tags - table->language_tags;
    }
  }


 /* return the list of available features for the current script/language */
 /* use with a GPOS or GSUB script table                                  */
  LOCAL_FUNC
  void OTL_Get_Features_List( OTL_Table*  table,
                              TT_ULong    language_tag )
  {
    TT_UInt   n;
    TT_Byte*  script   = table->cur_script;
    TT_Byte*  language = 0;
    TT_UShort offset;

    /* clear feature selection table */
    for ( n = 0; n < table->max_features; n++ )
      table->features[n] = 0;

    /* now, look for the current language */
    if ( language_tag == 0 )
    {
      offset = OTL_UShort(script);
      if (!offset) return; /* if there is no default language, exit */

      language = script - 2 + offset;
    }
    else
    {
      TT_Byte*  p = script + 8; /* skip default+count+1st tag */
      TT_UShort index;

      for ( n = 0; n < table->num_languages; n++, p+=6 )
      {
        if ( table->language_tags[n] == language_tag )
        {
          language = script + OTL_UShort(p);
          break;
        }
      }

      table->cur_language = language;
      if (!language) return;

      p     = language + 2;   /* skip lookup order */
      index = OTL_UShort(p);  /* required feature index */
      if (index != 0xFFFF)
      {
        if (index < table->max_features)
          table->features[index] = 1;
      }

      count = OTL_UShort(p);
      for ( ; count > 0; count-- )
      {
        index = OTL_UShort(p);
        if (index < table->max_features)
          table->features[index] = 1;
      }
    }
  }


 /* return the list of lookups for the current features list */
 /* use only with a GSUB or GPOS table                       */
  LOCAL_FUNC
  void  OTL_Get_Lookups_List( OTL_Table*  table )
  {
    TT_UInt  n;
    TT_Byte* features = table->features_table;
    TT_Byte* p        = features + 6; /* skip count+1st tag */

    /* clear lookup list */
    for ( n = 0; n < table->max_lookups; n++ )
      table->lookups[n] = 0;

    /* now, parse the features list */
    for ( n = 0; n < table->features; n++ )
    {
      if (table->features[n])
      {
        TT_UInt   count;
        TT_UShort index;
        TT_Byte*  feature;

        feature = features + OTL_UShort(p);
        p      += 4;  /* skip tag */

        /* now, select all lookups from this feature */
        count = OTL_UShort(feature);
        for ( ; count > 0; count-- )
        {
          index = OTL_UShort(feature);
          if (index < table->max_lookups)
            table->lookups[index] = 1;
        }
      }
    }
  }


 /* find the basevalues and minmax for the current script/language */
 /* only use it with a BASE table..                                */
  LOCAL_FUNC
  void OTL_Get_Baseline_Values( OTL_Table*  table,
                                TT_ULong    language_tag )
  {
    TT_Byte*  script   = table->cur_script;
    TT_Byte*  p        = script;
    TT_UShort offset, count;

    table->cur_basevalues = 0;
    table->cur_minmax     = 0;

    /* read basevalues */
    offset = OTL_UShort(p);
    if (offset)
      table->cur_basevalues = script + offset;

    offset = OTL_UShort(p);
    if (offset)
      table->cur_minmax = script + offset;

    count = OTL_UShort(p);
    for ( ; count > 0; count-- )
    {
      TT_ULong  tag;

      tag = OTL_ULong(p);
      if ( language_tag == tag )
      {
        table->cur_minmax = script + OTL_UShort(p);
        break;
      }
      p += 2; /* skip offset */
    }
  }


 /* compute the coverage value of a given glyph id */
  LOCAL_FUNC
  TT_Long  OTL_Get_Coverage_Index( TT_Byte*  coverage,
                                   TT_UInt   glyph_id )
  {
    TT_Long    result = -1;
    TT_UInt    count, index, start, end;
    TT_Byte*   p = coverage;

    switch ( OTL_UShort(p) )
    {
      case 1:  /* coverage format 1 - array of glyph indices */
        {
          count = OTL_UShort(p);
          for ( index = 0; index < count; index++ )
          {
            if ( OTL_UShort(p) == glyph_id )
            {
              result = index;
              break;
            }
          }
        }
        break;

      case 2:
        {
          count = OTL_UShort(p);
          for ( ; count > 0; count-- )
          {
            start = OTL_UShort(p);
            end   = OTL_UShort(p);
            index = OTL_UShort(p);
            if (start <= glyph_id && glyph_id <= end)
            {
              result = glyph_id - start + index;
              break;
            }
          }
        }
        break;
    }
    return result;
  }


 /* compute the class value of a given glyph_id */
  LOCAL_FUNC
  TT_UInt  OTL_Get_Glyph_Class( TT_Byte*  class_def,
                                TT_UInt   glyph_id )
  {
    TT_Byte*  p = class_def;
    TT_UInt   result = 0;
    TT_UInt   start, end, count, index;

    switch ( OTL_UShort(p) )
    {
      case 1:
        {
          start = OTL_UShort(p);
          count = OTL_UShort(p);

          glyph_id -= start;
          if (glyph_id < count)
          {
            p += 2*glyph_id;
            result = OTL_UShort(p);
          }
        }
        break;

      case 2:
        {
          count = OTL_UShort(p);
          for ( ; count > 0; count-- )
          {
            start = OTL_UShort(p);
            end   = OTL_UShort(p);
            index = OTL_UShort(p);
            if ( start <= glyph_id && glyph_id <= end )
            {
              result = index;
              break;
            }
          }
        }
        break;
    }
    return result;
  }


 /* compute the adjustement necessary for a given device size */
  LOCAL_FUNC
  TT_Int  OTL_Get_Device_Adjustment( TT_Byte* device,
                                     TT_UInt  size )
  {
    TT_Byte*  p = device;
    TT_Int    result = 0;
    TT_UInt   start, end;
    TT_Short  value;

    start = OTL_UShort(p);
    end   = OTL_UShort(p);
    if (size >= start && size <= end)
    {
      /* I know we could do all of this without a switch, with */
      /* clever shifts and everything, but it makes the code   */
      /* really difficult to understand..                      */

      size -= start;
      switch ( OTL_UShort(p) )
      {
        case 1: /* 2-bits per value */
          {
            p     += 2*(size >> 3);
            size   = (size & 7) << 1;
            value  = (TT_Short)((TT_Short)OTL_UShort(p) << size);
            result = value >> 14;
          }
          break;

        case 2: /* 4-bits per value */
          {
            p     += 2*(size >> 2);
            size   = (size & 3) << 2;
            value  = (TT_Short)((TT_Short)OTL_UShort(p) << size);
            result = value >> 12;
          }
          break;

        case 3: /* 8-bits per value */
          {
            p     += 2*(size >> 1);
            size   = (size & 1) << 3;
            value  = (TT_Short)((TT_Short)OTL_UShort(p) << size);
            result = value >> 8;
          }
          break;
      }
    }
    return result;
  }

 /* extract a BaseCoord value */
  LOCAL_FUNC
  void    OTL_Get_Base_Coordinate( TT_Byte*          base_coord,
                                   OTL_ValueRecord*  coord )
  {
    TT_Byte*  p = base_coord;
    TT_Int    result = 0;

    coord->format     = OTL_UShort(p);
    coord->coordinate = OTL_Short(p);
    coord->device     = 0;

    switch (coord->format)
    {
      case 2: /* format 2 */
        coord->ref_glyph = OTL_UShort(p);
        coord->ref_point = OTL_UShort(p);
        break;

      case 3: /* format 3 */
        coord->device = p - 4 + OTL_UShort(p);
        break;

      default:
        ;
    }
  }


 /* compute size of ValueRecord */
 LOCAL_FUNC
 TT_Int  OTL_ValueRecord_Size( TT_UShort  format )
 {
   TT_Int  count;

   /* each bit in the value format corresponds to a single ushort */
   /* we thus count the bits, and multiply the result by 2        */

   count = (TT_Int)(format & 0xFF);
   count = ((count & 0xAA) >> 1) + (count & 0x55);
   count = ((count & 0xCC) >> 2) + (count & 0x33);
   count = ((count & 0xF0) >> 4) + (count & 0x0F);

   return count*2;
 }



 /* extract ValueRecord */
 LOCAL_FUNC
 void  OTL_Get_ValueRecord( TT_Byte*          value_record,
                            TT_UShort         value_format,
			    TT_Byte*          pos_table,
			    OTL_ValueRecord*  record )
 {
   TT_Byte*  p = value_record;

   /* clear vectors */
   record->placement.x = 0;
   record->placement.y = 0;
   record->advance.x   = 0;
   record->advance.y   = 0;

   record->device_pla_x = 0;
   record->device_pla_y = 0;
   record->device_adv_x = 0;
   record->device_adv_y = 0;

   if (value_format & 1) record->placement.x = NEXT_Short(p);
   if (value_format & 2) record->placement.y = NEXT_Short(p);
   if (value_format & 4) record->advance.x   = NEXT_Short(p);
   if (value_format & 8) record->advance.y   = NEXT_Short(p);

   if (value_format & 16)  record->device_pla_x = pos_table + NEXT_UShort(p);
   if (value_format & 32)  record->device_pla_y = pos_table + NEXT_UShort(p);
   if (value_format & 64)  record->device_adv_x = pos_table + NEXT_UShort(p);
   if (value_format & 128) record->device_adv_y = pos_table + NEXT_UShort(p);
 }



 /* extract Anchor */
 LOCAL_FUNC
 void  OTL_Get_Anchor( TT_Byte*     anchor_table,
                       OTL_Anchor*  anchor )
 {
   TT_Byte*  p = anchor_table;

   anchor->format   = NEXT_UShort(p);
   anchor->coord.x  = NEXT_Short(p);
   anchor->coord.y  = NEXT_Short(p);
   anchor->point    = 0;
   anchor->device_x = 0;
   anchor->device_y = 0;

   switch (anchor->format)
   {
     case 2:
       anchor->point = NEXT_UShort(p);
       break;

     case 3:
       anchor->device_x = anchor_table + NEXT_UShort(p);
       anchor->device_y = anchor_table + NEXT_UShort(p);
       break;

     default:
       ;
   }
 }



 /* extract Mark from MarkArray */
 LOCAL_FUNC
 void  OTL_Get_Mark( TT_Byte*     mark_array,
                     TT_UInt      index,
		     TT_UShort*   clazz,
		     OTL_Anchor*  anchor )
 {
   TT_Byte* p = mark_array;
   TT_UInt  count;

   *clazz = 0;
   MEM_Set( anchor, 0, sizeof(*anchor) );

   count = NEXT_UShort(p);
   if (index < count)
   {
     p += 4*index;
     *clazz = NEXT_UShort(p);
     OTL_Get_Anchor( mark_array + NEXT_UShort(p), anchor );
   }
 }

