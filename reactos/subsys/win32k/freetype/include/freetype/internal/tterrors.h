/***************************************************************************/
/*                                                                         */
/*  tterrors.h                                                             */
/*                                                                         */
/*    TrueType error ID definitions (specification only).                  */
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


#ifndef TTERRORS_H
#define TTERRORS_H


  /*************************************************************************/
  /*                                                                       */
  /* Error codes declaration                                               */
  /*                                                                       */
  /* The error codes are grouped in `classes' used to indicate the `level' */
  /* at which the error happened.  The class is given by an error code's   */
  /* high byte.                                                            */
  /*                                                                       */
  /*************************************************************************/


  /* Success is always 0. */

#define TT_Err_Ok                         FT_Err_Ok

  /* High level API errors. */

#define TT_Err_Invalid_File_Format        FT_Err_Invalid_File_Format
#define TT_Err_Invalid_Argument           FT_Err_Invalid_Argument
#define TT_Err_Invalid_Driver_Handle      FT_Err_Invalid_Driver_Handle
#define TT_Err_Invalid_Face_Handle        FT_Err_Invalid_Face_Handle
#define TT_Err_Invalid_Instance_Handle    FT_Err_Invalid_Size_Handle
#define TT_Err_Invalid_Glyph_Handle       FT_Err_Invalid_Slot_Handle
#define TT_Err_Invalid_CharMap_Handle     FT_Err_Invalid_CharMap_Handle
#define TT_Err_Invalid_Glyph_Index        FT_Err_Invalid_Glyph_Index

#define TT_Err_Unimplemented_Feature      FT_Err_Unimplemented_Feature

#define TT_Err_Invalid_Engine             FT_Err_Invalid_Driver_Handle

  /* Internal errors. */

#define TT_Err_Out_Of_Memory              FT_Err_Out_Of_Memory
#define TT_Err_Unlisted_Object            FT_Err_Unlisted_Object

  /* General glyph outline errors. */

#define TT_Err_Too_Many_Ins               FT_Err_Too_Many_Hints
#define TT_Err_Invalid_Composite          FT_Err_Invalid_Composite

  /* Bytecode interpreter error codes. */

  /* These error codes are produced by the TrueType */
  /* bytecode interpreter.  They usually indicate a */
  /* broken font file, a broken glyph within a font */
  /* file, or a bug in the interpreter!             */

#define TT_Err_Invalid_Opcode             0x400
#define TT_Err_Too_Few_Arguments          0x401
#define TT_Err_Stack_Overflow             0x402
#define TT_Err_Code_Overflow              0x403
#define TT_Err_Bad_Argument               0x404
#define TT_Err_Divide_By_Zero             0x405
#define TT_Err_Storage_Overflow           0x406
#define TT_Err_Cvt_Overflow               0x407
#define TT_Err_Invalid_Reference          0x408
#define TT_Err_Invalid_Distance           0x409
#define TT_Err_Interpolate_Twilight       0x40A
#define TT_Err_Debug_OpCode               0x40B
#define TT_Err_ENDF_In_Exec_Stream        0x40C
#define TT_Err_Out_Of_CodeRanges          0x40D
#define TT_Err_Nested_DEFS                0x40E
#define TT_Err_Invalid_CodeRange          0x40F
#define TT_Err_Invalid_Displacement       0x410
#define TT_Err_Execution_Too_Long         0x411
#define TT_Err_Too_Many_Function_Defs     0x412
#define TT_Err_Too_Many_Instruction_Defs  0x413

  /* Other TrueType specific error codes. */

#define TT_Err_Table_Missing              0x420
#define TT_Err_Too_Many_Extensions        0x421
#define TT_Err_Extensions_Unsupported     0x422
#define TT_Err_Invalid_Extension_Id       0x423

#define TT_Err_No_Vertical_Data           0x424

#define TT_Err_Max_Profile_Missing        0x430
#define TT_Err_Header_Table_Missing       0x431
#define TT_Err_Horiz_Header_Missing       0x432
#define TT_Err_Locations_Missing          0x433
#define TT_Err_Name_Table_Missing         0x434
#define TT_Err_CMap_Table_Missing         0x435
#define TT_Err_Hmtx_Table_Missing         0x436
#define TT_Err_OS2_Table_Missing          0x437
#define TT_Err_Post_Table_Missing         0x438

#define TT_Err_Invalid_Horiz_Metrics      0x440
#define TT_Err_Invalid_CharMap_Format     0x441
#define TT_Err_Invalid_PPem               0x442
#define TT_Err_Invalid_Vert_Metrics       0x443

#define TT_Err_Could_Not_Find_Context     0x450


#endif /* TTERRORS_H */


/* END */
