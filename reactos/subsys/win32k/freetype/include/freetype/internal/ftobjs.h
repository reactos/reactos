/***************************************************************************/
/*                                                                         */
/*  ftobjs.h                                                               */
/*                                                                         */
/*    The FreeType private base classes (specification).                   */
/*                                                                         */
/*  Copyright 1996-2000 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /*  This file contains the definition of all internal FreeType classes.  */
  /*                                                                       */
  /*************************************************************************/


#ifndef FTOBJS_H
#define FTOBJS_H

#include <freetype/internal/ftmemory.h>
#include <freetype/ftrender.h>
#include <freetype/internal/ftdriver.h>
#include <freetype/internal/autohint.h>


#ifdef __cplusplus
  extern "C" {
#endif


  /*************************************************************************/
  /*                                                                       */
  /* Some generic definitions.                                             */
  /*                                                                       */
#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE  0
#endif

#ifndef NULL
#define NULL  (void*)0
#endif

#ifndef UNUSED
#define UNUSED( arg )  ( (arg)=(arg) )
#endif


  /*************************************************************************/
  /*                                                                       */
  /* The min and max functions missing in C.  As usual, be careful not to  */
  /* write things like MIN( a++, b++ ) to avoid side effects.              */
  /*                                                                       */
#ifndef MIN
#define MIN( a, b )  ( (a) < (b) ? (a) : (b) )
#endif

#ifndef MAX
#define MAX( a, b )  ( (a) > (b) ? (a) : (b) )
#endif

#ifndef ABS
#define ABS( a )     ( (a) < 0 ? -(a) : (a) )
#endif


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                         M O D U L E S                           ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_ModuleRec                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A module object instance.                                          */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    clazz   :: A pointer to the module's class.                        */
  /*                                                                       */
  /*    library :: A handle to the parent library object.                  */
  /*                                                                       */
  /*    memory  :: A handle to the memory manager.                         */
  /*                                                                       */
  /*    generic :: A generic structure for user-level extensibility (?).   */
  /*                                                                       */
  typedef struct  FT_ModuleRec_
  {
    FT_Module_Class*  clazz;
    FT_Library        library;
    FT_Memory         memory;
    FT_Generic        generic;
    
  } FT_ModuleRec;


  /* typecast an object to a FT_Module */
#define FT_MODULE( x )          ((FT_Module)(x))
#define FT_MODULE_CLASS( x )    FT_MODULE(x)->clazz
#define FT_MODULE_LIBRARY( x )  FT_MODULE(x)->library
#define FT_MODULE_MEMORY( x )   FT_MODULE(x)->memory

#define FT_MODULE_IS_DRIVER( x )  ( FT_MODULE_CLASS( x )->module_flags & \
                                    ft_module_font_driver )

#define FT_MODULE_IS_RENDERER( x )  ( FT_MODULE_CLASS( x )->module_flags & \
                                      ft_module_renderer )

#define FT_MODULE_IS_HINTER( x )  ( FT_MODULE_CLASS( x )->module_flags & \
                                    ft_module_hinter )

#define FT_MODULE_IS_STYLER( x )  ( FT_MODULE_CLASS( x )->module_flags & \
                                    ft_module_styler )

#define FT_DRIVER_IS_SCALABLE( x )  ( FT_MODULE_CLASS(x)->module_flags & \
                                      ft_module_driver_scalable )

#define FT_DRIVER_USES_OUTLINES( x )  !( FT_MODULE_CLASS(x)->module_flags & \
                                         ft_module_driver_no_outlines )

#define FT_DRIVER_HAS_HINTER( x )  ( FT_MODULE_CLASS(x)->module_flags & \
                                     ft_module_driver_has_hinter )


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_Module_Interface                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finds a module and returns its specific interface as a typeless    */
  /*    pointer.                                                           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    library     :: A handle to the library object.                     */
  /*                                                                       */
  /*    module_name :: The module's name (as an ASCII string).             */
  /*                                                                       */
  /* <Return>                                                              */
  /*    A module-specific interface if available, 0 otherwise.             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    You should better be familiar with FreeType internals to know      */
  /*    which module to look for, and what its interface is :-)            */
  /*                                                                       */
  BASE_DEF( const void* )  FT_Get_Module_Interface( FT_Library   library,
                                                    const char*  mod_name );


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****               FACE, SIZE & GLYPH SLOT OBJECTS                   ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/

  /* a few macros used to perform easy typecasts with minimal brain damage */

#define FT_FACE( x )          ((FT_Face)(x))
#define FT_SIZE( x )          ((FT_Size)(x))
#define FT_SLOT( x )          ((FT_GlyphSlot)(x))
  
#define FT_FACE_DRIVER( x )   FT_FACE( x )->driver
#define FT_FACE_LIBRARY( x )  FT_FACE_DRIVER( x )->root.library
#define FT_FACE_MEMORY( x )   FT_FACE( x )->memory

#define FT_SIZE_FACE( x )     FT_SIZE( x )->face
#define FT_SLOT_FACE( x )     FT_SLOT( x )->face

#define FT_FACE_SLOT( x )     FT_FACE( x )->glyph
#define FT_FACE_SIZE( x )     FT_FACE( x )->size


  /* this must be kept exported -- tt will be used later in our own */
  /* high-level caching font manager called SemTex (way after the   */
  /* 2.0 release though                                             */
  FT_EXPORT_DEF( FT_Error )  FT_New_Size( FT_Face   face,
                                          FT_Size*  size );

  FT_EXPORT_DEF( FT_Error )  FT_Done_Size( FT_Size  size );


  FT_EXPORT_DEF( FT_Error )  FT_New_GlyphSlot( FT_Face        face,
                                               FT_GlyphSlot*  aslot );

  FT_EXPORT_DEF( void )      FT_Done_GlyphSlot( FT_GlyphSlot  slot );


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                   G L Y P H   L O A D E R                       ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


#define FT_SUBGLYPH_FLAG_ARGS_ARE_WORDS          1
#define FT_SUBGLYPH_FLAG_ARGS_ARE_XY_VALUES      2
#define FT_SUBGLYPH_FLAG_ROUND_XY_TO_GRID        4
#define FT_SUBGLYPH_FLAG_SCALE                   8
#define FT_SUBGLYPH_FLAG_XY_SCALE             0x40
#define FT_SUBGLYPH_FLAG_2X2                  0x80
#define FT_SUBGLYPH_FLAG_USE_MY_METRICS      0x200


  enum
  {
    ft_glyph_own_bitmap = 1
  };


  struct  FT_SubGlyph_
  {
    FT_Int     index;
    FT_UShort  flags;
    FT_Int     arg1;
    FT_Int     arg2;
    FT_Matrix  transform;
  };


  typedef struct  FT_GlyphLoad_
  {
    FT_Outline    outline;       /* outline             */
    FT_UInt       num_subglyphs; /* number of subglyphs */
    FT_SubGlyph*  subglyphs;     /* subglyphs           */
    FT_Vector*    extra_points;  /* extra points table  */
  
  } FT_GlyphLoad;


  struct  FT_GlyphLoader_
  {
    FT_Memory     memory;
    FT_UInt       max_points;
    FT_UInt       max_contours;
    FT_UInt       max_subglyphs;
    FT_Bool       use_extra;

    FT_GlyphLoad  base;
    FT_GlyphLoad  current;

    void*         other;            /* for possible future extension? */
    
  };


  BASE_DEF( FT_Error )  FT_GlyphLoader_New( FT_Memory         memory,
                                            FT_GlyphLoader**  aloader );
                                          
  BASE_DEF( FT_Error )  FT_GlyphLoader_Create_Extra(
                          FT_GlyphLoader*  loader );                                        
  
  BASE_DEF( void )  FT_GlyphLoader_Done( FT_GlyphLoader*  loader );
  
  BASE_DEF( void )  FT_GlyphLoader_Reset( FT_GlyphLoader*  loader );
  
  BASE_DEF( void )  FT_GlyphLoader_Rewind( FT_GlyphLoader*  loader );
  
  BASE_DEF( FT_Error )  FT_GlyphLoader_Check_Points(
                          FT_GlyphLoader*  loader,
                          FT_UInt          n_points,
                          FT_UInt          n_contours );
                               
  BASE_DEF( FT_Error )  FT_GlyphLoader_Check_Subglyphs(
                          FT_GlyphLoader*  loader,
                          FT_UInt          n_subs );

  BASE_DEF( void )  FT_GlyphLoader_Prepare( FT_GlyphLoader*  loader );

  BASE_DEF( void )  FT_GlyphLoader_Add( FT_GlyphLoader*  loader );

  BASE_DEF( FT_Error )  FT_GlyphLoader_Copy_Points( FT_GlyphLoader*  target,
                                                    FT_GlyphLoader*  source );


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                        R E N D E R E R S                        ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


#define FT_RENDERER( x )      ((FT_Renderer)( x ))
#define FT_GLYPH( x )         ((FT_Glyph)( x ))
#define FT_BITMAP_GLYPH( x )  ((FT_BitmapGlyph)( x ))
#define FT_OUTLINE_GLYPH( x ) ((FT_OutlineGlyph)( x ))


  typedef struct  FT_RendererRec_
  {
    FT_ModuleRec           root;
    FT_Renderer_Class*     clazz;
    FT_Glyph_Format        glyph_format;
    const FT_Glyph_Class   glyph_class;

    FT_Raster              raster;
    FT_Raster_Render_Func  raster_render;
    FTRenderer_render      render;
  
  } FT_RendererRec;


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                    F O N T   D R I V E R S                      ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /* typecast a module into a driver easily */
#define FT_DRIVER( x )        ((FT_Driver)(x))

  /* typecast a module as a driver, and get its driver class */
#define FT_DRIVER_CLASS( x )  FT_DRIVER( x )->clazz


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_DriverRec                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The root font driver class.  A font driver is responsible for      */
  /*    managing and loading font files of a given format.                 */
  /*                                                                       */
  /*  <Fields>                                                             */
  /*     root         :: Contains the fields of the root module class.     */
  /*                                                                       */
  /*     clazz        :: A pointer to the font driver's class.  Note that  */
  /*                     this is NOT root.clazz.  `class' wasn't used      */
  /*                     as it is a reserved word in C++.                  */
  /*                                                                       */
  /*     faces_list   :: The list of faces currently opened by this        */
  /*                     driver.                                           */
  /*                                                                       */
  /*     extensions   :: A typeless pointer to the driver's extensions     */
  /*                     registry, if they are supported through the       */
  /*                     configuration macro FT_CONFIG_OPTION_EXTENSIONS.  */
  /*                                                                       */
  /*     glyph_loader :: The glyph loader for all faces managed by this    */
  /*                     driver.  This object isn't defined for unscalable */
  /*                     formats.                                          */
  /*                                                                       */
  typedef struct  FT_DriverRec_
  {
    FT_ModuleRec      root;
    FT_Driver_Class*  clazz;
    
    FT_ListRec        faces_list;
    void*             extensions;
    
    FT_GlyphLoader*   glyph_loader;

  } FT_DriverRec;


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                       L I B R A R I E S                         ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


#define FT_DEBUG_HOOK_TRUETYPE   0
#define FT_DEBUG_HOOK_TYPE1      1


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_LibraryRec                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The FreeType library class.  This is the root of all FreeType      */
  /*    data.  Use FT_New_Library() to create a library object, and        */
  /*    FT_Done_Library() to discard it and all child objects.             */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    memory           :: The library's memory object.  Manages memory   */
  /*                        allocation.                                    */
  /*                                                                       */
  /*    generic          :: Client data variable.  Used to extend the      */
  /*                        Library class by higher levels and clients.    */
  /*                                                                       */
  /*    num_modules      :: The number of modules currently registered     */
  /*                        within this library.  This is set to 0 for new */
  /*                        libraries.  New modules are added through the  */
  /*                        FT_Add_Module() API function.                  */
  /*                                                                       */
  /*    modules          :: A table used to store handles to the currently */
  /*                        registered modules. Note that each font driver */
  /*                        contains a list of its opened faces.           */
  /*                                                                       */
  /*    renderers        :: The list of renderers currently registered     */
  /*                        within the library.                            */
  /*                                                                       */
  /*    cur_renderer     :: The current outline renderer.  This is a       */
  /*                        shortcut used to avoid parsing the list on     */
  /*                        each call to FT_Outline_Render().  It is a     */
  /*                        handle to the current renderer for the         */
  /*                        ft_glyph_format_outline format.                */
  /*                                                                       */
  /*    auto_hinter      :: XXX                                            */
  /*                                                                       */
  /*    raster_pool      :: The raster object's render pool.  This can     */
  /*                        ideally be changed dynamically at run-time.    */
  /*                                                                       */
  /*    raster_pool_size :: The size of the render pool in bytes.          */
  /*                                                                       */
  /*    debug_hooks      :: XXX                                            */
  /*                                                                       */
  typedef struct  FT_LibraryRec_
  {
    FT_Memory           memory;           /* library's memory manager */

    FT_Generic          generic;

    FT_UInt             num_modules;
    FT_Module           modules[FT_MAX_MODULES];  /* module objects  */

    FT_ListRec          renderers;        /* list of renderers        */
    FT_Renderer         cur_renderer;     /* current outline renderer */
    FT_Module           auto_hinter;

    FT_Byte*            raster_pool;      /* scan-line conversion */
                                          /* render pool          */
    FT_ULong            raster_pool_size; /* size of render pool in bytes */

    FT_DebugHook_Func   debug_hooks[4];

  } FT_LibraryRec;


  BASE_DEF( FT_Renderer )  FT_Lookup_Renderer( FT_Library       library,
                                               FT_Glyph_Format  format,
                                               FT_ListNode*     node );

  BASE_DEF( FT_Error )  FT_Render_Glyph_Internal( FT_Library    library,
                                                  FT_GlyphSlot  slot,
                                                  FT_UInt       render_mode );

  typedef FT_Error  (*FT_Glyph_Name_Requester)( FT_Face     face,
                                                FT_UInt     glyph_index,
                                                FT_Pointer  buffer,
                                                FT_UInt     buffer_max );


#ifndef FT_CONFIG_OPTION_NO_DEFAULT_SYSTEM


  FT_EXPORT_DEF( FT_Error )   FT_New_Stream( const char*  filepathname,
                                             FT_Stream    astream );

  FT_EXPORT_DEF( void )       FT_Done_Stream( FT_Stream  stream );

  FT_EXPORT_DEF( FT_Memory )  FT_New_Memory( void );


#endif /* !FT_CONFIG_OPTION_NO_DEFAULT_SYSTEM */


  /* Define default raster's interface.  The default raster is located in  */
  /* `src/base/ftraster.c'                                                 */
  /*                                                                       */
  /* Client applications can register new rasters through the              */
  /* FT_Set_Raster() API.                                                  */

#ifndef FT_NO_DEFAULT_RASTER
  FT_EXPORT_VAR( FT_Raster_Funcs )  ft_default_raster;
#endif


#ifdef __cplusplus
  }
#endif


#endif /* FTOBJS_H */


/* END */
