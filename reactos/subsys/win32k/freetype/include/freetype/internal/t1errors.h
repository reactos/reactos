/***************************************************************************/
/*                                                                         */
/*  t1errors.h                                                             */
/*                                                                         */
/*    Type 1 error ID definitions (specification only).                    */
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


#ifndef T1ERRORS_H
#define T1ERRORS_H


  /************************ error codes declaration **************/

  /* The error codes are grouped into `classes' used to indicate the */
  /* `level' at which the error happened.                            */
  /*                                                                 */
  /* The class is given by an error code's high byte.                */


  /* ------------- Success is always 0 -------- */

#define T1_Err_Ok                      FT_Err_Ok

  /* ----------- high level API errors -------- */

#define T1_Err_Invalid_File_Format     FT_Err_Invalid_File_Format
#define T1_Err_Invalid_Argument        FT_Err_Invalid_Argument
#define T1_Err_Invalid_Driver_Handle   FT_Err_Invalid_Driver_Handle
#define T1_Err_Invalid_Face_Handle     FT_Err_Invalid_Face_Handle
#define T1_Err_Invalid_Size_Handle     FT_Err_Invalid_Size_Handle
#define T1_Err_Invalid_Glyph_Handle    FT_Err_Invalid_Slot_Handle
#define T1_Err_Invalid_CharMap_Handle  FT_Err_Invalid_CharMap_Handle
#define T1_Err_Invalid_Glyph_Index     FT_Err_Invalid_Glyph_Index

#define T1_Err_Unimplemented_Feature   FT_Err_Unimplemented_Feature

#define T1_Err_Invalid_Engine          FT_Err_Invalid_Driver_Handle

  /* ------------- internal errors ------------ */

#define T1_Err_Out_Of_Memory           FT_Err_Out_Of_Memory
#define T1_Err_Unlisted_Object         FT_Err_Unlisted_Object

  /* ------------ general glyph outline errors ------ */

#define T1_Err_Invalid_Composite       FT_Err_Invalid_Composite

#define T1_Err_Syntax_Error            FT_Err_Invalid_File_Format
#define T1_Err_Stack_Underflow         FT_Err_Invalid_File_Format
#define T1_Err_Stack_Overflow          FT_Err_Invalid_File_Format


#endif /* T1ERRORS_H */


/* END */
