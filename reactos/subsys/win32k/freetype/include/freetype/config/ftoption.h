/***************************************************************************/
/*                                                                         */
/*  ftoption.h                                                             */
/*                                                                         */
/*    User-selectable configuration macros (specification only).           */
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


#ifndef FTOPTION_H
#define FTOPTION_H


  /*************************************************************************/
  /*                                                                       */
  /*                 USER-SELECTABLE CONFIGURATION MACROS                  */
  /*                                                                       */
  /* These macros can be toggled by developers to enable or disable        */
  /* certain aspects of FreeType.  This is a default file, where all major */
  /* options are enabled.                                                  */
  /*                                                                       */
  /* Note that if some modifications are required for your build, we       */
  /* advise you to put a modified copy of this file in your build          */
  /* directory, rather than modifying it in-place.                         */
  /*                                                                       */
  /* The build directory is normally `freetype/builds/<system>' and        */
  /* contains build or system-specific files that are included in          */
  /* priority when building the library.                                   */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /**** G E N E R A L   F R E E T Y P E   2   C O N F I G U R A T I O N ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* Convenience functions support                                         */
  /*                                                                       */
  /*   Some functions of the FreeType 2 API are provided as a convenience  */
  /*   for client applications and developers. However,  they are not      */
  /*   required to build and run the library itself.                       */
  /*                                                                       */
  /*   By defining this configuration macro, you'll disable the            */
  /*   compilation of these functions at build time.  This can be useful   */
  /*   to reduce the library's code size when you don't need any of        */
  /*   these functions.                                                    */
  /*                                                                       */
  /*   All convenience functions are declared as such in their             */
  /*   documentation.                                                      */
  /*                                                                       */
#undef FT_CONFIG_OPTION_NO_CONVENIENCE_FUNCS


  /*************************************************************************/
  /*                                                                       */
  /* Alternate Glyph Image Format support                                  */
  /*                                                                       */
  /*   By default, the glyph images returned by the FreeType glyph loader  */
  /*   can either be a pixmap or a vectorial outline defined through       */
  /*   Bezier control points.  When defining the following configuration   */
  /*   macro, some font drivers will be able to register alternate         */
  /*   glyph image formats.                                                */
  /*                                                                       */
  /*   Unset this macro if you are sure that you will never use a font     */
  /*   driver with an alternate glyph format; this will reduce the size of */
  /*   the base layer code.                                                */
  /*                                                                       */
  /*   Note that a few Type 1 fonts, as well as Windows `vector' fonts     */
  /*   use a vector `plotter' format that isn't supported when this        */
  /*   macro is undefined.                                                 */
  /*                                                                       */
#define FT_CONFIG_OPTION_ALTERNATE_GLYPH_FORMATS


  /*************************************************************************/
  /*                                                                       */
  /* Glyph Postscript Names handling                                       */
  /*                                                                       */
  /*   By default, FreeType 2 is compiled with the `PSNames' module.  This */
  /*   This module is in charge of converting a glyph name string into a   */
  /*   Unicode value, or return a Macintosh standard glyph name for the    */
  /*   use with the TrueType `post' table.                                 */
  /*                                                                       */
  /*   Undefine this macro if you do not want `PSNames' compiled in your   */
  /*   build of FreeType.  This has the following effects:                 */
  /*                                                                       */
  /*   - The TrueType driver will provide its own set of glyph names,      */
  /*     if you build it to support postscript names in the TrueType       */
  /*     `post' table.                                                     */
  /*                                                                       */
  /*   - The Type 1 driver will not be able to synthetize a Unicode        */
  /*     charmap out of the glyphs found in the fonts.                     */
  /*                                                                       */
  /*   You would normally undefine this configuration macro when building  */
  /*   a version of FreeType that doesn't contain a Type 1 or CFF driver.  */
  /*                                                                       */
#define FT_CONFIG_OPTION_POSTSCRIPT_NAMES


  /*************************************************************************/
  /*                                                                       */
  /* Postscript Names to Unicode Values support                            */
  /*                                                                       */
  /*   By default, FreeType 2 is built with the `PSNames' module compiled  */
  /*   in.  Among other things, the module is used to convert a glyph name */
  /*   into a Unicode value.  This is especially useful in order to        */
  /*   synthetize on the fly a Unicode charmap from the CFF/Type 1 driver  */
  /*   through a big table named the `Adobe Glyph List' (AGL).             */
  /*                                                                       */
  /*   Undefine this macro if you do not want the Adobe Glyph List         */
  /*   compiled in your `PSNames' module.  The Type 1 driver will not be   */
  /*   able to synthetize a Unicode charmap out of the glyphs found in the */
  /*   fonts.                                                              */
  /*                                                                       */
#define FT_CONFIG_OPTION_ADOBE_GLYPH_LIST


  /*************************************************************************/
  /*                                                                       */
  /* Many compilers provide the non-ANSI `long long' 64-bit type.  You can */
  /* activate it by defining the FTCALC_USE_LONG_LONG macro.  Note that    */
  /* this will produce many -ansi warnings during library compilation, and */
  /* that in many cases the generated code will not be smaller or faster!  */
  /*                                                                       */
#undef FTCALC_USE_LONG_LONG


  /*************************************************************************/
  /*                                                                       */
  /* DLL export compilation                                                */
  /*                                                                       */
  /*   When compiling FreeType as a DLL, some systems/compilers need a     */
  /*   special keyword in front OR after the return type of function       */
  /*   declarations.                                                       */
  /*                                                                       */
  /*   Two macros are used within the FreeType source code to define       */
  /*   exported library functions: FT_EXPORT_DEF and FT_EXPORT_FUNC.       */
  /*                                                                       */
  /*     FT_EXPORT_DEF( return_type )                                      */
  /*                                                                       */
  /*       is used in a function declaration, as in                        */
  /*                                                                       */
  /*         FT_EXPORT_DEF( FT_Error )                                     */
  /*         FT_Init_FreeType( FT_Library*  alibrary );                    */
  /*                                                                       */
  /*                                                                       */
  /*     FT_EXPORT_FUNC( return_type )                                     */
  /*                                                                       */
  /*       is used in a function definition, as in                         */
  /*                                                                       */
  /*         FT_EXPORT_FUNC( FT_Error )                                    */
  /*         FT_Init_FreeType( FT_Library*  alibrary )                     */
  /*         {                                                             */
  /*           ... some code ...                                           */
  /*           return FT_Err_Ok;                                           */
  /*         }                                                             */
  /*                                                                       */
  /*   You can provide your own implementation of FT_EXPORT_DEF and        */
  /*   FT_EXPORT_FUNC here if you want.  If you leave them undefined, they */
  /*   will be later automatically defined as `extern return_type' to      */
  /*   allow normal compilation.                                           */
  /*                                                                       */
#undef FT_EXPORT_DEF
#undef FT_EXPORT_FUNC


  /*************************************************************************/
  /*                                                                       */
  /* Debug level                                                           */
  /*                                                                       */
  /*   FreeType can be compiled in debug or trace mode.  In debug mode,    */
  /*   errors are reported through the `ftdebug' component.  In trace      */
  /*   mode, additional messages are sent to the standard output during    */
  /*   execution.                                                          */
  /*                                                                       */
  /*   Define FT_DEBUG_LEVEL_ERROR to build the library in debug mode.     */
  /*   Define FT_DEBUG_LEVEL_TRACE to build it in trace mode.              */
  /*                                                                       */
  /*   Don't define any of these macros to compile in `release' mode!      */
  /*                                                                       */
#define FT_DEBUG_LEVEL_ERROR
#define FT_DEBUG_LEVEL_TRACE


  /*************************************************************************/
  /*                                                                       */
  /* Computation Algorithms                                                */
  /*                                                                       */
  /*   Used for debugging, this configuration macro should disappear       */
  /*   soon.                                                               */
  /*                                                                       */
#define FT_CONFIG_OPTION_OLD_CALCS


  /*************************************************************************/
  /*                                                                       */
  /* The size in bytes of the render pool used by the scan-line converter  */
  /* to do all of its work.                                                */
  /*                                                                       */
  /* This must be greater than 4kByte.                                     */
  /*                                                                       */
#define FT_RENDER_POOL_SIZE  16384


  /*************************************************************************/
  /*                                                                       */
  /* FT_MAX_MODULES                                                        */
  /*                                                                       */
  /*   The maximum number of modules that can be registered in a single    */
  /*   FreeType library object.  16 is the default.                        */
  /*                                                                       */
#define FT_MAX_MODULES  16


  /*************************************************************************/
  /*                                                                       */
  /* FT_MAX_EXTENSIONS                                                     */
  /*                                                                       */
  /*   The maximum number of extensions that can be registered in a single */
  /*   font driver.  8 is the default.                                     */
  /*                                                                       */
  /*   If you don't know what this means, you certainly do not need to     */
  /*   change this value.                                                  */
  /*                                                                       */
#define FT_MAX_EXTENSIONS  8


  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****        S F N T   D R I V E R    C O N F I G U R A T I O N       ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* Define TT_CONFIG_OPTION_EMBEDDED_BITMAPS if you want to support       */
  /* embedded bitmaps in all formats using the SFNT module (namely         */
  /* TrueType & OpenType).                                                 */
  /*                                                                       */
#define TT_CONFIG_OPTION_EMBEDDED_BITMAPS


  /*************************************************************************/
  /*                                                                       */
  /* Define TT_CONFIG_OPTION_POSTSCRIPT_NAMES if you want to be able to    */
  /* load and enumerate the glyph Postscript names in a TrueType or        */
  /* OpenType file.                                                        */
  /*                                                                       */
  /* Note that when you do not compile the `PSNames' module by undefining  */
  /* the above FT_CONFIG_OPTION_POSTSCRIPT_NAMES, the `sfnt' module will   */
  /* contain additional code used to read the PS Names table from a font.  */
  /*                                                                       */
  /* (By default, the module uses `PSNames' to extract glyph names.)       */
  /*                                                                       */
#define TT_CONFIG_OPTION_POSTSCRIPT_NAMES


  /*************************************************************************/
  /*                                                                       */
  /* Define TT_CONFIG_OPTION_SFNT_NAMES if your applications need to       */
  /* access the internal name table in a SFNT-based format like TrueType   */
  /* or OpenType.  The name table contains various strings used to         */
  /* describe the font, like family name, copyright, version, etc.  It     */
  /* does not contain any glyph name though.                               */
  /*                                                                       */
  /* Accessing SFNT names is done through the functions declared in        */
  /* `freetype/ftnames.h'.                                                 */
  /*                                                                       */
#define TT_CONFIG_OPTION_SFNT_NAMES


  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****    T R U E T Y P E   D R I V E R    C O N F I G U R A T I O N   ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* Define TT_CONFIG_OPTION_BYTECODE_INTERPRETER if you want to compile   */
  /* a bytecode interpreter in the TrueType driver.  Note that there are   */
  /* important patent issues related to the use of the interpreter.        */
  /*                                                                       */
  /* By undefining this, you will only compile the code necessary to load  */
  /* TrueType glyphs without hinting.                                      */
  /*                                                                       */
#undef  TT_CONFIG_OPTION_BYTECODE_INTERPRETER


  /*************************************************************************/
  /*                                                                       */
  /* Define TT_CONFIG_OPTION_INTERPRETER_SWITCH to compile the TrueType    */
  /* bytecode interpreter with a huge switch statement, rather than a call */
  /* table.  This results in smaller and faster code for a number of       */
  /* architectures.                                                        */
  /*                                                                       */
  /* Note however that on some compiler/processor combinations, undefining */
  /* this macro will generate faster, though larger, code.                 */
  /*                                                                       */
#define TT_CONFIG_OPTION_INTERPRETER_SWITCH


  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****      T Y P E 1   D R I V E R    C O N F I G U R A T I O N       ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* T1_MAX_STACK_DEPTH is the maximal depth of the token stack used by    */
  /* the Type 1 parser (see t1load.c).  A minimum of 16 is required.       */
  /*                                                                       */
#define T1_MAX_STACK_DEPTH  16


  /*************************************************************************/
  /*                                                                       */
  /* T1_MAX_DICT_DEPTH is the maximal depth of nest dictionaries and       */
  /* arrays in the Type 1 stream (see t1load.c).  A minimum of 4 is        */
  /* required.                                                             */
  /*                                                                       */
#define T1_MAX_DICT_DEPTH  5


  /*************************************************************************/
  /*                                                                       */
  /* T1_MAX_SUBRS_CALLS details the maximum number of nested sub-routine   */
  /* calls during glyph loading.                                           */
  /*                                                                       */
#define T1_MAX_SUBRS_CALLS  8


  /*************************************************************************/
  /*                                                                       */
  /* T1_MAX_CHARSTRING_OPERANDS is the charstring stack's capacity.        */
  /*                                                                       */
#define T1_MAX_CHARSTRINGS_OPERANDS  32


  /*************************************************************************/
  /*                                                                       */
  /* Define T1_CONFIG_OPTION_DISABLE_HINTER if you want to generate a      */
  /* driver with no hinter.  This can be useful to debug the parser.       */
  /*                                                                       */
#undef T1_CONFIG_OPTION_DISABLE_HINTER


  /*************************************************************************/
  /*                                                                       */
  /* Define this configuration macro if you want to prevent the            */
  /* compilation of `t1afm', which is in charge of reading Type 1 AFM      */
  /* files into an existing face.  Note that if set, the T1 driver will be */
  /* unable to produce kerning distances.                                  */
  /*                                                                       */
#undef T1_CONFIG_OPTION_NO_AFM


  /*************************************************************************/
  /*                                                                       */
  /* Define this configuration macro if you want to prevent the            */
  /* compilation of the Multiple Masters font support in the Type 1        */
  /* driver.                                                               */
  /*                                                                       */
#undef T1_CONFIG_OPTION_NO_MM_SUPPORT


#endif /* FTOPTION_H */


/* END */
