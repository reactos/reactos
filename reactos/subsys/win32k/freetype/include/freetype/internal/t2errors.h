/***************************************************************************/
/*                                                                         */
/*  t2errors.h                                                             */
/*                                                                         */
/*    OpenType error ID definitions (specification only).                  */
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


#ifndef T2ERRORS_H
#define T2ERRORS_H


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

#define T2_Err_Ok                         FT_Err_Ok

  /* High level API errors. */

#define T2_Err_Invalid_File_Format        FT_Err_Invalid_File_Format
#define T2_Err_Invalid_Argument           FT_Err_Invalid_Argument
#define T2_Err_Invalid_Driver_Handle      FT_Err_Invalid_Driver_Handle
#define T2_Err_Invalid_Face_Handle        FT_Err_Invalid_Face_Handle
#define T2_Err_Invalid_Instance_Handle    FT_Err_Invalid_Size_Handle
#define T2_Err_Invalid_Glyph_Handle       FT_Err_Invalid_Slot_Handle
#define T2_Err_Invalid_CharMap_Handle     FT_Err_Invalid_CharMap_Handle
#define T2_Err_Invalid_Glyph_Index        FT_Err_Invalid_Glyph_Index

#define T2_Err_Unimplemented_Feature      FT_Err_Unimplemented_Feature

#define T2_Err_Invalid_Engine             FT_Err_Invalid_Driver_Handle

  /* Internal errors. */

#define T2_Err_Out_Of_Memory              FT_Err_Out_Of_Memory
#define T2_Err_Unlisted_Object            FT_Err_Unlisted_Object

  /* General glyph outline errors. */

#define T2_Err_Invalid_Composite          FT_Err_Invalid_Composite

  /* Bytecode interpreter error codes. */

  /* These error codes are produced by the TrueType */
  /* bytecode interpreter.  They usually indicate a */
  /* broken font file, a broken glyph within a font */
  /* file, or a bug in the interpreter!             */

#define T2_Err_Invalid_Opcode             0x500
#define T2_Err_Too_Few_Arguments          0x501
#define T2_Err_Stack_Overflow             0x502
#define T2_Err_Code_Overflow              0x503
#define T2_Err_Bad_Argument               0x504
#define T2_Err_Divide_By_Zero             0x505
#define T2_Err_Storage_Overflow           0x506
#define T2_Err_Cvt_Overflow               0x507
#define T2_Err_Invalid_Reference          0x508
#define T2_Err_Invalid_Distance           0x509
#define T2_Err_Interpolate_Twilight       0x50A
#define T2_Err_Debug_OpCode               0x50B
#define T2_Err_ENDF_In_Exec_Stream        0x50C
#define T2_Err_Out_Of_CodeRanges          0x50D
#define T2_Err_Nested_DEFS                0x50E
#define T2_Err_Invalid_CodeRange          0x50F
#define T2_Err_Invalid_Displacement       0x510
#define T2_Err_Execution_Too_Long         0x511

#define T2_Err_Too_Many_Instruction_Defs  0x512
#define T2_Err_Too_Many_Function_Defs     0x513

  /* Other TrueType specific error codes. */

#define T2_Err_Table_Missing              0x520
#define T2_Err_Too_Many_Extensions        0x521
#define T2_Err_Extensions_Unsupported     0x522
#define T2_Err_Invalid_Extension_Id       0x523

#define T2_Err_No_Vertical_Data           0x524

#define T2_Err_Max_Profile_Missing        0x530
#define T2_Err_Header_Table_Missing       0x531
#define T2_Err_Horiz_Header_Missing       0x532
#define T2_Err_Locations_Missing          0x533
#define T2_Err_Name_Table_Missing         0x534
#define T2_Err_CMap_Table_Missing         0x535
#define T2_Err_Hmtx_Table_Missing         0x536
#define T2_Err_OS2_Table_Missing          0x537
#define T2_Err_Post_Table_Missing         0x538

#define T2_Err_Invalid_Horiz_Metrics      0x540
#define T2_Err_Invalid_CharMap_Format     0x541
#define T2_Err_Invalid_PPem               0x542
#define T2_Err_Invalid_Vert_Metrics       0x543

#define T2_Err_Could_Not_Find_Context     0x550


#endif /* T2ERRORS_H */


/* END */
