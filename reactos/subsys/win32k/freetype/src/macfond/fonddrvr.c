/***************************************************************************/
/*                                                                         */
/*  fonddrvr.c                                                             */
/*                                                                         */
/*    Mac FOND font driver. Written by just@letterror.com.                 */
/*                                                                         */
/*  Copyright 1996-2000 by                                                 */
/*  Just van Rossum, David Turner, Robert Wilhelm, and Werner Lemberg.     */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


/*
    Notes

    Mac suitcase files can (and often do!) contain multiple fonts. To
    support this I use the face_index argument of FT_(Open|New)_Face()
    functions, and pretend the suitcase file is a collection.
    Warning: although the FOND driver sets face->num_faces field to the
    number of available fonts, but the Type 1 driver sets it to 1 anyway.
    So this field is currently not reliable, and I don't see a clean way
    to  resolve that. The face_index argument translates to
      Get1IndResource( 'FOND', face_index + 1 );
    so clients should figure out the resource index of the FOND.
    (I'll try to provide some example code for this at some point.)

    The Mac FOND driver works roughly like this:

    - Check whether the offered stream points to a Mac suitcase file.
      This is done by checking the file type: it has to be 'FFIL' or 'tfil'.
      The stream that gets passed to our init_face() routine is a stdio
      stream, which isn't usable for us, since the FOND resources live
      in the resource fork. So we just grab the stream->pathname field.

    - Read the FOND resource into memory, then check whether there is
      a TrueType font and/or (!) a Type 1 font available.

    - If there is a Type 1 font available (as a separate 'LWFN' file),
      read its data into memory, massage it slightly so it becomes
      PFB data, wrap it into a memory stream, load the Type 1 driver
      and delegate the rest of the work to it, by calling the init_face()
      method of the Type 1 driver.
      (XXX TODO: after this has been done, the kerning data from the FOND
      resource should be appended to the face: on the Mac there are usually
      no AFM files available. However, this is tricky since we need to map
      Mac char codes to ps glyph names to glyph ID's...)

    - If there is a TrueType font (an 'sfnt' resource), read it into
      memory, wrap it into a memory stream, load the TrueType driver
      and delegate the rest of the work to it, by calling the init_face()
      method if the TrueType driver.

    - In both cases, the original stream gets closed and *reinitialized*
      to become a memory stream. Additionally, the face->driver field --
      which is set to the FOND driver upon entering our init_face() --
      gets *reset* to either the TT or the T1 driver. I had to make a minor
      change to ftobjs.c to make this work.

    - We might consider creating an FT_New_Face_Mac() API call, as this
      would avoid some of the mess described above.
*/

#include <truetype/ttobjs.h>
#include <type1z/z1objs.h>

#include <Resources.h>
#include <Fonts.h>
#include <Errors.h>

#include <ctype.h>  /* for isupper() and isalnum() */
#include <stdlib.h> /* for malloc() and free() */


/* set PREFER_LWFN to 1 if LWFN (Type 1) is preferred over
   TrueType in case *both* are available */
#ifndef PREFER_LWFN
#define PREFER_LWFN 1
#endif


  static
  FT_Error init_driver( FT_Driver  driver )
  {
    /* we don't keep no stinkin' state ;-) */
    return FT_Err_Ok;
  }

  static
  FT_Error done_driver( FT_Driver  driver )
  {
    return FT_Err_Ok;
  }


  /* MacRoman glyph names, needed for FOND kerning support. */
  /* XXX which is not implemented yet! */
  static const char*  mac_roman_glyph_names[256] = {
    ".null",
  };


  /* The FOND face object is just a union of TT and T1: both is possible,
     and we don't need anything else. We still need to be able to hold
     either, as the face object is not allocated by us. Again, creating
     an FT_New_Face_Mac() would avoid this kludge. */
  typedef union FOND_FaceRec_
  {
    TT_FaceRec     tt;
    T1_FaceRec     t1;
  } FOND_FaceRec, *FOND_Face;


  /* given a pathname, fill in a File Spec */
  static
  int make_file_spec( char* pathname, FSSpec *spec )
  {
    Str255  p_path;
    int     path_len;

    /* convert path to a pascal string */
    path_len = strlen( pathname );
    if ( path_len > 255 )
      return -1;
    p_path[0] = path_len;
    strncpy( (char*)p_path+1, pathname, path_len );

    if ( FSMakeFSSpec( 0, 0, p_path, spec ) != noErr )
      return -1;
    else
      return 0;
  }


  /* is_suitcase() returns true if the file specified by 'pathname'
     is a Mac suitcase file, and false if it ain't. */
  static
  int is_suitcase( FSSpec  *spec )
  {
    FInfo   finfo;

    if ( FSpGetFInfo( spec, &finfo ) != noErr )
      return 0;
    if ( finfo.fdType == 'FFIL' || finfo.fdType == 'tfil' )
      return 1;
    else
      return 0;
  }


  /* Quick 'n' Dirty Pascal string to C string converter.
     Warning: this call is not thread safe! Use with caution. */
  static
  char * p2c_str( unsigned char *pstr )
  {
    static char cstr[256];

    strncpy( cstr, (char*)pstr+1, pstr[0] );
    cstr[pstr[0]] = '\0';
    return cstr;
  }


  /* Given a PostScript font name, create the Macintosh LWFN file name */
  static
  void create_lwfn_name( char* ps_name, Str255 lwfn_file_name )
  {
    int  max = 5, count = 0;
    unsigned char* p = lwfn_file_name;
    char* q = ps_name;

    lwfn_file_name[0] = 0;

    while ( *q )
    {
      if ( isupper(*q) )
      {
        if ( count )
          max = 3;
        count = 0;
      }
      if ( count < max && (isalnum(*q) || *q == '_' ) )
      {
        *++p = *q;
        lwfn_file_name[0]++;
        count++;
      }
      q++;
    }
  }


  /* Suck the relevant info out of the FOND data */
  static
  FT_Error parse_fond( char*   fond_data,
                       short   *have_sfnt,
                       short   *sfnt_id,
                       Str255  lwfn_file_name )
  {
    AsscEntry*  assoc;
    FamRec*     fond;

    *sfnt_id = *have_sfnt = 0;
    lwfn_file_name[0] = 0;

    fond = (FamRec*)fond_data;
    assoc = (AsscEntry*)(fond_data + sizeof(FamRec) + 2);

    if ( assoc->fontSize == 0 )
    {
      *have_sfnt = 1;
      *sfnt_id = assoc->fontID;
    }

    if ( fond->ffStylOff )
    {
      unsigned char*  p = (unsigned char*)fond_data;
      StyleTable*     style;
      unsigned short  string_count;
      unsigned char*  name_table = 0;
      char            ps_name[256];
      unsigned char*  names[64];
      int             i;

      p += fond->ffStylOff;
      style = (StyleTable*)p;
      p += sizeof(StyleTable);
      string_count = *(unsigned short*)(p);
      p += sizeof(short);

      for ( i=0 ; i<string_count && i<64; i++ )
      {
        names[i] = p;
        p += names[i][0];
        p++;
      }
      strcpy(ps_name, p2c_str(names[0])); /* Family name */

      if ( style->indexes[0] > 1 )
      {
        unsigned char* suffixes = names[style->indexes[0]-1];
        for ( i=1; i<=suffixes[0]; i++ )
          strcat( ps_name, p2c_str(names[suffixes[i]-1]) );
      }
      create_lwfn_name( ps_name, lwfn_file_name );
    }
    return FT_Err_Ok;
  }


  /* Read Type 1 data from the POST resources inside the LWFN file, return a
     PFB buffer -- apparently FT doesn't like a pure binary T1 stream. */
  static
  unsigned char* read_type1_data( FT_Memory memory, FSSpec* lwfn_spec, unsigned long *size )
  {
    short          res_ref, res_id;
    unsigned char  *buffer, *p, *size_p;
    unsigned long  total_size = 0;
    unsigned long  post_size, pfb_chunk_size;
    Handle         post_data;
    char           code, last_code;

    res_ref = FSpOpenResFile( lwfn_spec, fsRdPerm );
    if ( ResError() )
      return NULL;
    UseResFile( res_ref );

    /* first pass: load all POST resources, and determine the size of
       the output buffer */
    res_id = 501;
    last_code = -1;
    for (;;)
    {
      post_data = Get1Resource( 'POST', res_id++ );
      if ( post_data == NULL )
        break;
      code = (*post_data)[0];
      if ( code != last_code )
      {
        if ( code == 5 )
          total_size += 2; /* just the end code */
        else
          total_size += 6; /* code + 4 bytes chunk length */
      }
      total_size += GetHandleSize( post_data ) - 2;
      last_code = code;
    }

    buffer = memory->alloc( memory, total_size );
    if ( !buffer )
      goto error;

    /* second pass: append all POST data to the buffer, add PFB fields */
    p = buffer;
    res_id = 501;
    last_code = -1;
    pfb_chunk_size = 0;
    for (;;)
    {
      post_data = Get1Resource( 'POST', res_id++ );
      if ( post_data == NULL )
        break;
      post_size = GetHandleSize( post_data ) - 2;
      code = (*post_data)[0];
      if ( code != last_code )
      {
        if ( last_code != -1 )
        {
          /* we're done adding a chunk, fill in the size field */
          *size_p++ = pfb_chunk_size & 0xFF;
          *size_p++ = (pfb_chunk_size >> 8) & 0xFF;
          *size_p++ = (pfb_chunk_size >> 16) & 0xFF;
          *size_p++ = (pfb_chunk_size >> 24) & 0xFF;
          pfb_chunk_size = 0;
        }
        *p++ = 0x80;
        if ( code == 5 )
          *p++ = 0x03;  /* the end */
        else if ( code == 2 )
          *p++ = 0x02;  /* binary segment */
        else
          *p++ = 0x01;  /* ASCII segment */
        if ( code != 5 )
        {
          size_p = p;   /* save for later */
          p += 4;       /* make space for size field */
        }
      }
      memcpy( p, *post_data + 2, post_size );
      pfb_chunk_size += post_size;
      p += post_size;
      last_code = code;
    }

    CloseResFile( res_ref );

    *size = total_size;
/*    printf( "XXX %d %d\n", p - buffer, total_size ); */
    return buffer;

error:
    CloseResFile( res_ref );
    return NULL;
  }


  /* Finalizer for the sfnt stream */
  static
  void sfnt_stream_close( FT_Stream  stream )
  {
    Handle sfnt_data = stream->descriptor.pointer;
    HUnlock( sfnt_data );
    DisposeHandle( sfnt_data );

    stream->descriptor.pointer = NULL;
    stream->size               = 0;
    stream->base               = 0;
    stream->close              = 0;
  }


  /* Finalizer for the LWFN stream */
  static
  void lwfn_stream_close( FT_Stream  stream )
  {
    stream->memory->free( stream->memory, stream->base );
    stream->descriptor.pointer = NULL;
    stream->size               = 0;
    stream->base               = 0;
    stream->close              = 0;
  }


  /* Main entry point. Determine whether we're dealing with a Mac
     suitcase or not; then determine if we're dealing with Type 1
     or TrueType; delegate the work to the proper driver. */
  static
  FT_Error init_face( FT_Stream      stream,
                      FT_Face        face,
                      FT_Int         face_index,
                      FT_Int         num_params,
                      FT_Parameter*  parameters )
  {
    FT_Error      err;
    FSSpec        suit_spec, lwfn_spec;
    short         res_ref;
    Handle        fond_data, sfnt_data;
    short         res_index, sfnt_id, have_sfnt;
    Str255        lwfn_file_name;

    if ( !stream->pathname.pointer )
      return FT_Err_Unknown_File_Format;

    if ( make_file_spec( stream->pathname.pointer, &suit_spec ) )
      return FT_Err_Invalid_Argument;

    if ( !is_suitcase( &suit_spec ) )
      return FT_Err_Unknown_File_Format;

    res_ref = FSpOpenResFile( &suit_spec, fsRdPerm );
    if ( ResError() )
      return FT_Err_Invalid_File_Format;
    UseResFile( res_ref );

    /* face_index may be -1, in which case we
       just need to do a sanity check */
    if ( face_index < 0)
      res_index = 1;
    else
    {
      res_index = face_index + 1;
      face_index = 0;
    }
    fond_data = Get1IndResource( 'FOND', res_index );
    if ( ResError() )
    {
      CloseResFile( res_ref );
      return FT_Err_Invalid_File_Format;
    }
    /* Set the number of faces. Not that it helps much: the t1 driver
       just sets it to 1 anyway :-( */
    face->num_faces = Count1Resources('FOND');

    HLock( fond_data );
    err = parse_fond( *fond_data, &have_sfnt, &sfnt_id, lwfn_file_name );
    HUnlock( fond_data );
    if ( err )
    {
      CloseResFile( res_ref );
      return FT_Err_Invalid_Handle;
    }

    if ( lwfn_file_name[0] )
    {
      /* We look for the LWFN file in the same directory as the suitcase
         file. ATM would look in other places, too, but this is the usual
         situation. */
      err = FSMakeFSSpec( suit_spec.vRefNum, suit_spec.parID, lwfn_file_name, &lwfn_spec );
      if ( err != noErr )
        lwfn_file_name[0] = 0;  /* no LWFN file found */
    }

    if ( lwfn_file_name[0] && ( !have_sfnt || PREFER_LWFN ) )
    {
      FT_Driver       t1_driver;
      unsigned char*  type1_data;
      unsigned long   size;

      CloseResFile( res_ref ); /* XXX still need to read kerning! */

      type1_data = read_type1_data( stream->memory, &lwfn_spec, &size );
      if ( !type1_data )
      {
        return FT_Err_Out_Of_Memory;
      }

#if 0
      {
        FILE* f;
        char * path;

        path = p2c_str( lwfn_file_name );
        strcat( path, ".PFB" );
        f = fopen(path, "wb");
        if ( f )
        {
          fwrite( type1_data, 1, size, f );
          fclose( f );
        }
      }
#endif

      /* reinitialize the stream */
      if ( stream->close )
        stream->close( stream );
      stream->close = lwfn_stream_close;
      stream->read = 0; /* it's now memory based */
      stream->base = type1_data;
      stream->size = size;
      stream->pos = 0; /* just in case */

      /* delegate the work to the Type 1 module */
      t1_driver = (FT_Driver)FT_Get_Module( face->driver->root.library, "type1z" );
      if ( t1_driver )
      {
        face->driver = t1_driver;
        return t1_driver->clazz->init_face( stream, face, face_index, 0, NULL );
      }
      else
        return FT_Err_Invalid_Driver_Handle;
    }
    else if ( have_sfnt )
    {
      FT_Driver     tt_driver;

      sfnt_data = Get1Resource( 'sfnt', sfnt_id );
      if ( ResError() )
      {
        CloseResFile( res_ref );
        return FT_Err_Invalid_Handle;
      }
      DetachResource( sfnt_data );
      CloseResFile( res_ref );
      HLockHi( sfnt_data );

      /* reinitialize the stream */
      if ( stream->close )
        stream->close( stream );
      stream->close = sfnt_stream_close;
      stream->descriptor.pointer = sfnt_data;
      stream->read = 0; /* it's now memory based */
      stream->base = (unsigned char *)*sfnt_data;
      stream->size = GetHandleSize( sfnt_data );
      stream->pos = 0; /* just in case */

      /* delegate the work to the TrueType driver */
      tt_driver = (FT_Driver)FT_Get_Module( face->driver->root.library, "truetype" );
      if ( tt_driver )
      {
        face->driver = tt_driver;
        return tt_driver->clazz->init_face( stream, face, face_index, 0, NULL );
      }
      else
        return FT_Err_Invalid_Driver_Handle;
    }
    else
    {
      CloseResFile( res_ref );
    }
    return FT_Err_Invalid_File_Format;
  }


  static
  void  done_face( FT_Face  face )
  {
    /* nothing to do */
  }

  /* The FT_DriverInterface structure is defined in ftdriver.h. */

  const FT_Driver_Class  fond_driver_class =
  {
    {
      ft_module_font_driver | ft_module_driver_scalable,
      sizeof ( FT_DriverRec ),

      "fond",          /* driver name                           */
      0x10000L,        /* driver version == 1.0                 */
      0x20000L,        /* driver requires FreeType 2.0 or above */

      (void*)0,

      (FT_Module_Constructor)     init_driver,
      (FT_Module_Destructor)      done_driver,
      (FT_Module_Requester)       0
    },

    sizeof ( FOND_FaceRec ),
    0,
    0,

    (FTDriver_initFace)          init_face,
    (FTDriver_doneFace)          done_face,
    (FTDriver_initSize)          0,
    (FTDriver_doneSize)          0,
    (FTDriver_initGlyphSlot)     0,
    (FTDriver_doneGlyphSlot)     0,

    (FTDriver_setCharSizes)      0,
    (FTDriver_setPixelSizes)     0,
    (FTDriver_loadGlyph)         0,
    (FTDriver_getCharIndex)      0,

    (FTDriver_getKerning)        0,
    (FTDriver_attachFile)        0,
    (FTDriver_getAdvances)       0
  };



  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    getDriverInterface                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function is used when compiling the FOND driver as a          */
  /*    shared library (`.DLL' or `.so').  It will be used by the          */
  /*    high-level library of FreeType to retrieve the address of the      */
  /*    driver's generic interface.                                        */
  /*                                                                       */
  /*    It shouldn't be implemented in a static build, as each driver must */
  /*    have the same function as an exported entry point.                 */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The address of the TrueType's driver generic interface.  The       */
  /*    format-specific interface can then be retrieved through the method */
  /*    interface->get_format_interface.                                   */
  /*                                                                       */
#ifdef FT_CONFIG_OPTION_DYNAMIC_DRIVERS

  FT_EXPORT_FUNC(const FT_Driver_Class*)  getDriverClass( void )
  {
    return &fond_driver_class;
  }

#endif /* CONFIG_OPTION_DYNAMIC_DRIVERS */


/* END */
